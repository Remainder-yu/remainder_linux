## 1. 阻塞型IO

引入问题：如果驱动程序无法立即满足请求，该如何响应？当数据不可用时，用户可能调用read；或者进程试图写入数据，但因为输出缓冲区已满，设备还未准备好接收数据。调用进程通常不会关系这类问题，程序员只会简单调用read\write，然后等待必要的工作结束后返回调用，因此，在这种情况下，我们的驱动程序应该（默认）阻塞该进程，将其置入休眠状态直到请求可继续。

### 1.1. 休眠的简单介绍
当一个进程被置入休眠时，他会被标记为一种特殊状态并从调度器的运行队列中移走。直到某些情况下修改了这个状态，进程才会在任意CPU上调度。休眠的进程会被搁置在一边，等待将来的某个事件发生。

如何安全的进入休眠：
1. 永远不在原子上下文中进入休眠：不能在拥有自旋锁、seqlock或者RCU锁时休眠
2. 禁止中断，也不能休眠；
3. 我们对唤醒之后的状态不能做任何假定，因此必须检查以确保我们等待的条件真正为真；
4. 除非直到其他地方会唤醒当前休眠操作，否则不能休眠；

### 1.2. 简单休眠
休眠唤醒的配对使用，不同休眠唤醒函数针对不同场景；

下面是一个sleepy的模块，任何视图从该设备上读取的进程均被置于休眠。只要某个进程向该设备写入，所有休眠的进程就会被唤醒。
```cpp
ssize_t sleepy_read (struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) going to sleep\n",
			current->pid, current->comm);
	wait_event_interruptible(wq, flag != 0);
	flag = 0;
	printk(KERN_DEBUG "awoken %i (%s)\n", current->pid, current->comm);
	return 0; /* EOF */
}

ssize_t sleepy_write (struct file *filp, const char __user *buf, size_t count,
		loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) awakening the readers...\n",
			current->pid, current->comm);
	flag = 1;
	wake_up_interruptible(&wq);
	return count; /* succeed, to avoid retrial */
}
```
注意上面例子中的flag变量的使用，因为wait_event_interruptible要检查改变为真的条件，因此我们使用flag来构造这个条件。
注意：上面存在什么样的问题？
思考：如果当前两个进程等待调用sleepy_write，会发生什么情况？当调用wake_up_interruptible，两个进程都会看到flag为1标志切换，针对某些重要的模块，这就存在竞态，可能导致很难诊断的偶然的崩溃。如果要确保只有一个进程能看到非零值，则必须以原子方式进行检查。

### 1.3. 阻塞与非阻塞操作


## 2. 异步通知
Linux通用方法基于一个数据结构和两个函数。
异步通知的核心就是信号signal：参考代码arch/xtensa/include/uapi/asm/signal.h。
信号相当于中断号，不同的中断号代表了不同的中断，不同的中断所做得处理不同，因此驱动程序可以通过向应用程序发送不同的信号来实现不同的功能。

在应用程序中使用信号，那么必须设置信号的信号处理函数，在应用程序中使用signal函数来设置指定信号的处理函数，signal函数原型如下所示：
```cpp
sighandler_t signal(int signum, sighandler_t handler)
/**
 * signum:要设置处理函数的信号
 * handler：信号的处理函数
 * 返回值：设置成功的话返回信号的前一个处理函数
*/
typedef void (*sighandler_t)(int)

#include <stdlib.h>
#include <sdtio.h>
#include <signal.h>

void sigint_handler(int num) {
    printf("SIGINT signale!\n");
    exit(0);
}

int main() {
    signal(SIGINT,sigint_handler);
    while(1);
    return 0;
}
```

### 2.1. 驱动中的信号处理
#### 2.1.1. fasync_struct结构体
```cpp


```

#### 2.1.2. fasync函数
驱动程序要调用的两个函数的原型如下：
```cpp
int fasync_helper(int fd, struct file *filep,
    int mode, struct fasync_struct **faa);
// 实现异步机制
static int scull_p_fasync(int fd, struct file *filp, int mode)
{
	struct scull_pipe *dev = filp->private_data;

	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

void kill_fasync(struct fasync_struct **fa, int sig, int band);

// eg:
	/* and signal asynchronous readers, explained late in chapter 5 */
	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);

```

当一个打开的文件的FASYNC标志被修改时，调用fasync_helper以便从相关的进程列表中增加或者删除文件。
某些设备针对设备可写入而实现了异步通知，在这种情况下，kill_fasync必须以POLL_OUT为模式调用。

注意：当文件关闭时必须调用fasync方法，以便从活动的异步读取进程列表中删除该文件。尽管这个调用只在filp->f_flags设置了FASYNC标志时才是必须得。
```cpp
//eg: 开启异步flag的file close的方法：
static int key_release(struct inode *inode, struct file *filp)
{
    return key_fasync(-1, filp, 0);
}
// eg:
/* remove this filp from the asynchronously notified filp's */
	scull_p_fasync(-1, filp, 0);

```

异步通知所使用的数据结构和struct wait_queue使用的几乎是相同的，因为两种情况都涉及等待事件。不同之处在于前者用struct file替换了struct task_struct。队列中的file结构用来获取f_owner，以免给进程发送信号。

参考：
《linux设备驱动程序--马书》--实例代码linuxdevice3/scull/pipe.c
