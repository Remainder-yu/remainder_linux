# futex
# linux futex实现原理

## futex机制

futex是fast userspace mutex，该机制实际上是一种用于用户空间应用程序的通用同步工具（基于futex可以在userspace实现互斥锁、读写锁、条件变量等同步机制）。
futex组成包括：
* 内核空间的等待队列
* 用户空间的32-bit futex word

在没有竞争的场景下，锁的获取和释放性能都非常高，不需要内核的参与，仅仅是通过用户空间的原子操作来修改futex word的状态即可。

在有竞争的场景下，如果线程无法获取futex锁，那么把自己放入到wait queue（陷入内核，有系统调用开销），而在owner task释放锁的时候，如果检测到有竞争（等待队列中有阻塞任务），就会通过系统调用来唤醒等待队列中的任务，使其恢复执行，继续去持有锁。

### futex同步机制
所有futex同步操作都是从用户空间开始，首先创建一个futex同步变量，也就是位于共享内存的计数器。当进程尝试持有锁或者要进入互斥区的时候，对futex执行down操作，即原子性的给futex同步变量减1.
如果同步变量变为0，则没有发生竞争，进程照常执行。
如果同步变量是个负数，则意味着有竞争发生，需要调用futex系统调用的futex_wait操作休眠当前进程。

当进程释放锁或者离开互斥区时候，对futex进行up操作，即原子性的futex加1。如果同步变量由0变成1，则没有竞争发生，进程照常执行，如果加之前为负数，则意味着竞争发生，需要调用futex系统调用的futex_wake操作唤醒一个或者多个等待进程。

这里使用CAS完成的，CAS的基本形式是：CAS(addr,old,new),当addr中存放的值等于old时，用new对其替换。在x86平台上有专门的一条指令来完成它: cmpxchg。

Todo： futex机制--zhangz

目标：
尽可能的避免系统调用

uaddr：是用户态，生成futex_key.



### 进程/线程利用futex同步
进程和线程都可以利用futex进行同步。对于线程，情况比较简单，因为线程共享虚拟内存空间，虚拟地址就可以唯一的标识出futex变量，即线程用同样的虚拟地址来访问futex变量。进程由于虚拟地址不同，所以获取物理地址。

### futex对象

futex是32bit值，其地址提供的系统调用sys_futex在所有平台都是32bit。所有futex操作都受这个值管理。为了在进程间共性一个futex值，futex通常放在共享内存中。

## futex用户和内核空间接口api

```c
   #include <linux/futex.h>      /* Definition of FUTEX_* constants */
       #include <sys/syscall.h>      /* Definition of SYS_* constants */
       #include <unistd.h>

       long syscall(SYS_futex, uint32_t *uaddr, int futex_op, uint32_t val,
                    const struct timespec *timeout,   /* or: uint32_t val2 */
                    uint32_t *uaddr2, uint32_t val3);

       Note: glibc provides no wrapper for futex(), necessitating the
       use of syscall(2).
```

参数：
uaddr： 32-bit futex word的地址，futex word等于0的时候，表示处于空锁状态。当owner tid设定为具体的thread id的时候表示持锁状态。如果有任务持锁失败登入等待队列 
futex_op：指定操作类型，可以是以下值之一：

FUTEX_WAIT：等待futex变量的值等于val，如果不等于，则当前线程进入睡眠状态。
FUTEX_WAKE：唤醒等待在futex变量上的线程，最多唤醒val个线程。
FUTEX_FD：将futex变量与一个文件描述符绑定。
其他操作类型用于特定的扩展功能。

val: 与futex_op相关的操作值，具体含义取决于操作类型。
timeout：等待超时时间，指向struct timespec结构体指针，用于指定等待的时间
uaadr2和val3：用于特定的扩展功能。

返回值：
成功执行时，返回值为0。
如果被中断（如收到信号）时，返回值为-1，并将errno设置为EINTR。
如果超时等待时，返回值为-1，并将errno设置为ETIMEDOUT。
其他错误情况下，返回值为-1，并将errno设置为相应的错误码。

通过调用sys_futex系统调用，可以实现对futex变量的等待和唤醒操作，从而实现用户态线程的同步。

### 实现源码：
include/linux/futex.h
kernel/futex.c
sys_futex的定义在include/linux/futex.h头文件中。它是一个系统调用，用于实现用户空间和内核空间之间的互斥锁。
参数：sys_futex的参数包括用户空间的futex地址、操作类型、期望值和超时等。
内核处理：当用户空间调用sys_futex系统调用时，内核会根据操作类型来执行相应的操作。常见的操作类型包括等待（FUTEX_WAIT）和唤醒（FUTEX_WAKE）。
**等待操作**：当操作类型为FUTEX_WAIT时，内核会将当前线程置于等待状态，直到futex的值与期望值相等。这个过程中，内核会进行一些调度和等待队列管理的操作。
**唤醒操作**：当操作类型为FUTEX_WAKE时，内核会唤醒等待在futex上的线程。

## linux数据结构分析

![futex相关的数据结构](vx_images/522840805836618.png)

### 背景知识
逻辑上的实现，通过futex实现的互斥锁和内核中的互斥锁mutex是一样的，只不过futex互斥锁是分裂开的：futex word和等待队列分别在用户空间和内核空间，内核的mutex互斥锁可以将等待队列头防止在mutex对象上，但是对于futex，我们没有对应的内核锁对象，因此我们需要一个算法将futex word和其等待队列映射起来。为了管理挂入队列的futex阻塞任务，内核建立了一个hash table：

```c
/*
 * The base of the bucket array and its size are always used together
 * (after initialization only in hash_futex()), so ensure that they
 * reside in the same cacheline.
 */
static struct {
	struct futex_hash_bucket *queues;
	unsigned long            hashsize;
} __futex_data __read_mostly __aligned(2*sizeof(long));
```


在初始化的时候，内核会构建hashsize个futex hash buket结构，每个bucket用来管理futex链表hash key相同。
```c
/*
 * Hash buckets are shared by all the futex_keys that hash to the same
 * location.  Each key may have multiple futex_q structures, one for each task
 * waiting on a futex.
 */
struct futex_hash_bucket {
	atomic_t waiters; // 在等待队列上阻塞的任务个数
	spinlock_t lock; // 保护队列的锁
	struct plist_head chain;// hash key相同的futex阻塞任务对象会挂在这个链表头上
} ____cacheline_aligned_in_smp;

```
每一个等待在futex word的task都有一个futex_q对象（后文称之为futex阻塞任务对象），根据其哈希值挂入不同的队列：
```c
struct futex_q {
	struct plist_node list; //挂入队列的节点，按照优先级排序

	struct task_struct *task;// 等待futex word的任务
	spinlock_t *lock_ptr; // 等待队列操作的自旋锁
	union futex_key key;//计算hash key的数据结构
	struct futex_pi_state *pi_state; // 优先级继承相关的
	struct rt_mutex_waiter *rt_waiter; // rt-mutex实现
	union futex_key *requeue_pi_key;
	u32 bitset; // futex wake的时候用来判断是否唤醒任务
};
```

```c
/*
 * Priority Inheritance state:
 */
struct futex_pi_state {
	/*
	 * list of 'owned' pi_state instances - these have to be
	 * cleaned up in do_exit() if the task exits prematurely:
	 */
	struct list_head list;

	/*
	 * The PI object:
	 */
	struct rt_mutex pi_mutex;

	struct task_struct *owner;
	atomic_t refcount;

	union futex_key key;
};
```
通过以上数据结构，只要有了futex word，那么我们就能根据hash key定位其挂入的链表。当然，为了精准的匹配，还需要其futex key完全匹配。

### futex_wait
futex_wait函数主要流程是准备等待，并将当前线程加入到等待队列中，然后等待唤醒、超时和信号。等到成功返回0。，收到信号重新启动系统调用。

如果uaddr上指向的futex值和val值相同，即当前线程/进程会阻塞在uaddr所指向的futex值上，并等待uaddr的futex_wake操作；如果uaddr上的值！=val，那么futex(2)失败并返回伴着EAGAIN错误码。
如果参数超时时间非空，那么超时时间指向的结构体会指定休眠时间。

```cpp
static int futex_wait(u32 __user *uaddr, unsigned int flags, u32 val,
		      ktime_t *abs_time, u32 bitset)
{
	struct hrtimer_sleeper timeout, *to = NULL;
	struct restart_block *restart;
	struct futex_hash_bucket *hb;
	struct futex_q q = futex_q_init;
	int ret;

	if (!bitset)
		return -EINVAL;
	q.bitset = bitset;
        // 如果参数中给定了timeout，
	if (abs_time) {
		to = &timeout;

		hrtimer_init_on_stack(&to->timer, (flags & FLAGS_CLOCKRT) ?
				      CLOCK_REALTIME : CLOCK_MONOTONIC,
				      HRTIMER_MODE_ABS);
		hrtimer_init_sleeper(to, current);
		hrtimer_set_expires_range_ns(&to->timer, *abs_time,
					     current->timer_slack_ns);
	}

retry:
	/*
	 * Prepare to wait on uaddr. On success, holds hb lock and increments
	 * q.key refs.
	 */
	ret = futex_wait_setup(uaddr, val, flags, &q, &hb);
	if (ret)
		goto out;

	/* queue_me and wait for wakeup, timeout, or a signal. */
	futex_wait_queue_me(hb, &q, to);

	/* If we were woken (and unqueued), we succeeded, whatever. */
	ret = 0;
	/* unqueue_me() drops q.key ref */
	if (!unqueue_me(&q))
		goto out;
	ret = -ETIMEDOUT;
	if (to && !to->task)
		goto out;

	/*
	 * We expect signal_pending(current), but we might be the
	 * victim of a spurious wakeup as well.
	 */
	if (!signal_pending(current))
		goto retry;

	ret = -ERESTARTSYS;
	if (!abs_time)
		goto out;

	restart = &current->restart_block;
	restart->fn = futex_wait_restart;
	restart->futex.uaddr = uaddr;
	restart->futex.val = val;
	restart->futex.time = abs_time->tv64;
	restart->futex.bitset = bitset;
	restart->futex.flags = flags | FLAGS_HAS_TIMEOUT;

	ret = -ERESTART_RESTARTBLOCK;

out:
	if (to) {
		hrtimer_cancel(&to->timer);
		destroy_hrtimer_on_stack(&to->timer);
	}
	return ret;
}
```

### futex_wake
该操作是唤醒最多val个的uaddr指向futex字段上的等待者。最常见的是val为1，即表示唤醒一个等待者，或者唤醒所有等待者。比如说不保证高调度优先级的等待者先于低优先级等待者被唤醒。

# 测试工具

计算sender释放test_mutex锁，然后receiver获取锁的延时时间。作为性能测试标准。



![](vx_images/330944423279099.png =810x)

### 核心算法

```c
void *semathread(void *param)
{
    ...
    while (!par->shutdown) {
        // 如果是发送者
        if (par->sender) {
            // 同步锁 上所
            pthread_mutex_lock(&syncmutex[par->num]);
            // 获取时间 发送时间
            gettimeofday(&par->unblocked, NULL);
            // 解锁
            pthread_mutex_unlock(&testmutex[par->num]);
            // 当前循环 ++
            par->samples++;
            // 结束
            if(par->max_cycles && par->samples >= par->max_cycles)
                par->shutdown = 1;
        } else {
            /* Receiver */
            // 接收者
            // 上锁 因为一开始 都上了锁 ，所以 接收者 等待 ，直到 sender 解锁
            pthread_mutex_lock(&testmutex[par->num]);
            // 获得 时间  接收时间
            gettimeofday(&par->received, NULL);
            // 当前循环 ++ 
            par->samples++;
            // 计算差 值  接收-发送 = 锁的延迟
            timersub(&par->received, &par->neighbor->unblocked,
                &par->diff);
            // 更新 min  延迟 最小值
            if (par->diff.tv_usec < par->mindiff)
                par->mindiff = par->diff.tv_usec;
            // 更新 max  延迟 最大值
            if (par->diff.tv_usec > par->maxdiff)
                par->maxdiff = par->diff.tv_usec;
            par->sumdiff += (double) par->diff.tv_usec;
            
            // 判断是否结束
            if (par->max_cycles && par->samples >= par->max_cycles)
                par->shutdown = 1;
            // 解开 sender 的 锁
            pthread_mutex_unlock(&syncmutex[par->num]);
        }
    }
    par->stopped = 1;
    return NULL;
    // ...
}
```

1. 首先设置子线程的优先级和调度策略(FIFO)。
2. 如用户使用了-a或-smp选项，则par->cpu != -1，此时需设置亲和性。
3. 发送者子线程的工作循环
    * 发送者首先要锁定同步互斥锁，目的是不要让自己循环的太快，让接收者有时间处理数据。
    * 用gettimeofday()把时间记录在par->unblocked。
    * 释放测试互斥锁，让接收者可以锁定测试互斥锁。(第一轮循环的时候，测试互斥锁是主线程给锁上的；在随后的循环里，测试互斥锁是接收者子线程给锁上的)
    * 用par->samples记录测试循环发生了多少轮。
接收者子线程的工作循环
if分支永远都不可能发生，因为first的值始终都是1。
锁定测试互斥锁。
用gettimeofday()把时间记录在par->received。
用par->diff记录下解锁到加锁这段时间，即receiver->receive - sender->unblocked。
分别记录下par->diff的最小值，最大值，累加和。
此if分支关于追踪指令，忽略。
睡眠par->delay的时间，给其它子线程一些机会去运行。
解锁同步互斥锁，让发送者子线程可以继续下一轮循环。



参考：
[什么是futex](https://zhuanlan.zhihu.com/p/568678633)
[119546730](https://blog.csdn.net/www_dong/article/details/119546730)