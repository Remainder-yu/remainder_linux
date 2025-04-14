## 线程池是什么
线程池是基于池化思想管理线程的工具，经常出现在多线程服务器中。
线程过多会带来额外的开销，其中包括创建销毁线程的开销、调度线程的开销等等，同事也降低了计算机的整体性能。线程池维护多个线程，等待监督管理者分配可并发执行的任务。 
这种做法，一方面避免了处理任务时创建销毁开销代价，另一方面避免了线程数量膨胀导致过分调度问题，保证了对内核的充分利用。

![](vx_images/109035113625597.png)

使用线程池可以带来一系列好处：
* 降低资源消耗（系统资源）：通过池化技术重复利用已创建的线程，降低线程创建和销毁造成的损耗。
* 提高线程的可管理性（系统资源）：线程时稀缺资源，如果无限制,不仅会消耗系统资源，还会因为线程的不合理分布导致资源调度失衡，降低系统的稳定性。使用线程池可以进行统一的分配、调优和监控。
* 提高响应速度（任务响应）：任务到达时，无需等待线程创建即可立即执行。
* 提供更多更强大的功能（功能扩展）：线程池具备可拓展性，允许开发人员向其中增加更多的功能。比如延时定时线程池。

## 线程池使用场景
某类任务特别耗时，严重影响其他线程处理其他任务；
在其他线程异步执行该函数
线程资源的开销与CPU核数之间平衡选择；

![](vx_images/513120700278996.png =500x)

### 线程池的作用
复用线程资源
减少线程创建和销毁的开销
可异步处理生产者线程任务
减少了多个任务的执行时间

### 如何创建一个线程池
![](vx_images/150361082836519.png =800x)

[线程池](https://blog.csdn.net/qq_36359022/article/details/78796784)

Q1：为什么线程池创建一定数量的线程后，然后添加任务到工作队列，线程池会从工作队列中拿取执行？

# 线程池
分为带锁和不带锁example：
线程池主要概念：
线程池技术是指能够保证所创建的任意线程都处于繁忙的状态，而不需要频繁地为了某一个任务而创建和销毁。
因为线程在创建和销毁时所消耗的CPU资源很大。

## 线程池主要技术原理
在创建一定数量的线程以队列的形式存在，并为之分配一个工作队列，当工作队列为空时，表示没有任务，此时所有线程挂起等待新的工作到来。
当新的工作到来时，线程队列头开始执行这个任务，然后依次执行其他线程完成到来的新任务。
当某个任务处理完成后，那么该线程立马开始接受任务分配，从而让所有其他线程都处于忙碌的状态，提高并行效率。

线程池技术是一种典型的生产者-消费者模型。因此，无论用哪种语言实现，只要遵循其原理本身就能够很好的工作了。那么实现线程池技术我们需要考虑到哪些技术性的问题？

## C语言执行
1. 线程池线程数量？max_thread_num
2. 如何表示一个线程池是否已经关闭？如果关闭那就需要释放资源？shutdown
3. 创建线程需要id？id数组
4. 线程锁，保证线程操作的互斥性？queue_lock
5. 条件变量？为了广播任务到来的消息给所有线程，当处于空闲线程，则由此线程接受任务分派。queue_ready
6. 最为重要的就是任务本身，也就是工作。那么工作本身又需要哪几个成员变量？首先肯定是任务入口，routine函数；其次是routine函数的参数args；再次任务是以队列存在着的，所以任务本身应该包含一个next。

* 一、创建线程池，create_tpool
* 二、销毁线程池，destroy_tpool
* 三、分派任务，add_task_2_tpool

### 参考：
https://blog.csdn.net/weixin_44847746/article/details/106870641?spm=1001.2101.3001.6650.2&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-2-106870641-blog-52958104.235%5Ev38%5Epc_relevant_default_base&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-2-106870641-blog-52958104.235%5Ev38%5Epc_relevant_default_base&utm_relevant_index=3

https://zhuanlan.zhihu.com/p/44971598

https://blog.csdn.net/u014338577/article/details/52958104?utm_medium=distribute.pc_relevant.none-task-blog-2~default~baidujs_baidulandingword~default-0-52958104-blog-109779578.235^v38^pc_relevant_default_base&spm=1001.2101.3001.4242.1&utm_relevant_index=3
示意图参考：
https://blog.csdn.net/jcjc918/article/details/50395528

# 条件变量--pthread_cond_wait
条件变量是利用线程间共享全局变量的同步机制，主要包括两个动作：
1. 一个线程等待“条件变量的条件成立”；
2. 另一个线程使“条件成立”（给出条件成立信号）；
3. 为了防止竞争，条件变量使用总是和一个互斥锁结合在一起；

注销一个条件变量需要调用pthread_cond_destroy()，只有在没有线程在该条件变量上等待的时候才能注销这个条件变量，否则返回EBUSY。因为Linux实现的条件变量没有分配什么资源，所以注销动作只包括检查是否有等待线程。
## 主要接口
### 等待和激发
```c
int   pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
int   pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
```
#### 子线程中pthread_cond_wait()等待
```c
pthread_mutex_t qlock;
pthread_cond_t  qready；
/************pthread_cond_wait()的使用方法**********/
pthread_mutex_lock(&qlock);    /*lock*/
//等待某资源，并以qready作为条件通知我们
pthread_cond_wait(&qready, &qlock); /*block-->unlock-->wait() return-->lock*/
//do something
pthread_mutex_unlock(&qlock); /*unlock*/
```
#### 其他线程中pthread_cond_signal()唤醒

```c
pthread_mutex_lock(&mtx);
//这个mutex_lock主要是用来保护wait等待临界资源，
//为何这里要有一个while (head == NULL)呢？
//因为如果有很多线程同时等待某资源，pthread_cond_wait里的线程可能会被意外唤醒，
//那么这个时候仍然head == NULL，这就是“惊群效应”
//这个时候，应该让线程继续进入pthread_cond_wait
while (head == NULL)　　pthread_cond_wait(&cond, &mtx);
// pthread_cond_wait会先解除之前的pthread_mutex_lock锁定的mtx
//然后阻塞在等待队列里休眠，直到再次被唤醒
//（大多数情况下是等待的条件成立而被唤醒）
//唤醒后，该进程会先锁定先pthread_mutex_lock(&mtx); 再读取资源
// 用这个流程是比较清楚的/*block-->unlock-->wait() return-->lock*/
pthread_mutex_unlock(&mtx); //临界区数据操作完毕，释放互斥锁
```
解析：
1、pthread_cond_signal在多处理器上可能同时唤醒多个线程，当你只能让一个线程处理某个任务时，其它被唤醒的线程就需要继续 wait, while循环的意义就体现在这里了，而且规范要求pthread_cond_signal至少唤醒一个pthread_cond_wait上的线程，其实有些实现为了简单在单处理器上也会唤醒多个线程.。
2、某些应用，如线程池，pthread_cond_broadcast唤醒全部线程，但我们通常只需要一部分线程去做执行任务，所以其它的线程需要继续wait.所以强烈推荐此处使用while循环. (惊群效应)

什么是惊群效应？
　　有人觉得此处既然是被唤醒的，肯定是满足条件了，其实不然。如果是多个线程都在等待这个条件，而同时只能有一个线程进行处理，此时由于多处理器同时进行，也就是说，不同的线程要对同一个共享变量做操作。此时就必须要再次条件判断，以使只有一个线程进入临界区处理。

`pthread_cond_wait()`  用于阻塞当前线程，等待别的线程使用 `pthread_cond_signal()` 或`pthread_cond_broadcast`来唤醒它 。  `pthread_cond_wait() `  必须与`pthread_mutex` 配套使用。`pthread_cond_wait()` 函数一进入wait状态就会自动`release mutex`。当其他线程通过 `pthread_cond_signal()` 或`pthread_cond_broadcast` ，把该线程唤醒，使 `pthread_cond_wait()`通过（返回）时，该线程又自动获得该`mutex` 。

pthread_cond_signal函数的作用是发送一个信号给另外一个正在处于阻塞等待状态的线程，使其脱离阻塞状态。如果没有线程处于阻塞等待状态，pthread_cond_signal也会返回成功。
使用pthread_cond_signal一般不会有“惊群现象”产生，他最多只给一个线程发信号。假如有多个线程正在阻塞等待着这个条件变量的话，那么是根据各等待线程优先级的高低确定哪个线程接收到信号开始继续执行。如果各线程优先级相同，则根据等待时间的长短来确定哪个线程获得信号。但无论如何一个pthread_cond_signal调用最多发信一次。

  但是 pthread_cond_signal 在多处理器上可能同时唤醒多个线程，当你只能让一个线程处理某个任务时，其它被唤醒的线程就需要继续 wait，


## 用法详解
pthread_cond_wait必须放在pthread_mutex_lock和pthread_mutex_unlock之间，因为他要根据共享变量的状态来决定是否要等待，而为了不永远等待下去所以必须要在lock/unlock队中。共享变量的状态改变必须遵守lock/unlock的规则。
> pthread_cond_signal既可以放在pthread_mutex_lock和pthread_mutex_unlock之间，也可以放在pthread_mutex_lock和pthread_mutex_unlock之后，但是各有各缺点。

### pthread_cond_signal放在pthread_mutex_lock和pthread_mutex_unlock之间

 缺点：在某下线程的实现中，会造成等待线程从内核中唤醒（由于cond_signal)然后又回到内核空间（因为cond_wait返回后会有原子加锁的行为），所以一来一回会有性能的问题，造成低效。

我们假设系统中有线程1和线程2，他们都想获取mutex后处理共享数据，再释放mutex。请看这种序列：
1)线程1获取mutex，在进行数据处理的时候，线程2也想获取mutex，但是此时被线程1所占用，线程2进入休眠，等待mutex被释放。
2)线程1做完数据处理后，调用pthread_cond_signal（）唤醒等待队列中某个线程，在本例中也就是线程2。线程1在调用pthread_mutex_unlock（）前，因为系统调度的原因，线程2获取使用CPU的权利，那么它就想要开始处理数据，但是在开始处理之前，mutex必须被获取，很遗憾，线程1正在使用mutex，所以线程2被迫再次进入休眠。
3)然后就是线程1执行pthread_mutex_unlock（）后，线程2方能被再次唤醒。
从这里看，使用的效率是比较低的，如果再多线程环境中，这种情况频繁发生的话，是一件比较痛苦的事情。

但是在LinuxThreads或者NPTL里面，就不会有这个问题，因为在Linux 线程中，有两个队列，分别是cond_wait队列和mutex_lock队列， cond_signal只是让线程从cond_wait队列移到mutex_lock队列，而不用返回到用户空间，不会有性能的损耗。推荐使用何种模式。posix1标准说，pthread_cond_signal与pthread_cond_broadcast无需考虑调用线程是否是mutex的拥有者，也就是说，可以在lock与unlock以外的区域调用。如果我们对调用行为不关心，那么请在lock区域之外调用吧。

### pthread_cond_signal放在pthread_mutex_lock和pthread_mutex_unlock之后

```c
pthread_mutex_lock
xxxxxxx
pthread_mutex_unlock
pthread_cond_signal
```
优点：不会出现之前说的那个潜在的性能损耗，因为在signal之前就已经释放锁了
缺点：如果unlock和signal之前，有个低优先级的线程正在mutex上等待的话，那么这个低优先级的线程就会抢占高优先级的线程（cond_wait的线程)，而这在上面的放中间的模式下是不会出现的。

```c
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// 生产者 -- 消费者模型

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct node {
    int n_number;
    struct node *n_next;
} *head = NULL;

/*[thread_func]*/
static void cleanup_handler(void *arg)
{
    printf("Cleanup handler of second thread.\n");
    free(arg);
    (void)pthread_mutex_unlock(&mtx);
}

static void *thread_func(void *arg)
{
    struct node *p = NULL;

    pthread_cleanup_push(cleanup_handler, p);
    while (1) {
        //这个mutex主要是用来保证pthread_cond_wait的并发性
        pthread_mutex_lock(&mtx);
        while (head == NULL) {
            // 这个while要特别说明一下，单个pthread_cond_wait功能很完善，为何这里要有一个while (head == NULL)呢？
            // 因为pthread_cond_wait里的线程可能会被意外唤醒，如果这个时候head != NULL，则不是我们想要的情况。
            // 这个时候，应该让线程继续进入pthread_cond_wait
            pthread_cond_wait(&cond, &mtx);
            // pthread_cond_wait会先解除之前的pthread_mutex_lock锁定的mtx，然后阻塞在等待对列里休眠，
            // 直到再次被唤醒（大多数情况下是等待的条件成立而被唤醒，唤醒后，该进程会先锁定先pthread_mutex_lock(&mtx);，
            // 再读取资源
            // 用这个流程是比较清楚的/*block-->unlock-->wait() return-->lock*/
        }
        p = head;
        head = head->n_next;
        printf("Got %d from front of queue\n", p->n_number);
        free(p);
        pthread_mutex_unlock(&mtx);             //临界区数据操作完毕，释放互斥锁
    }
    pthread_cleanup_pop(0);
    return 0;
}

int main(void)
{
    pthread_t tid;
    int i;
    struct node *p;
    //子线程会一直等待资源，类似生产者和消费者，但是这里的消费者可以是多个消费者，而不仅仅支持普通的单个消费者，这个模型虽然简单，但是很强大
    pthread_create(&tid, NULL, thread_func, NULL);

    /*[tx6-main]*/
    for (i = 0; i < 10; i++) {
        p = malloc(sizeof(struct node));
        p->n_number = i;
        pthread_mutex_lock(&mtx);             //需要操作head这个临界资源，先加锁，
        p->n_next = head;
        head = p;
        printf("main thread send num = %d\n", p->n_number);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mtx);           //解锁
        sleep(1);
    }
    printf("thread 1 wanna end the line.So cancel thread 2.\n");
    // 关于pthread_cancel，有一点额外的说明，它是从外部终止子线程，子线程会在最近的取消点，退出线程，
    // 而在我们的代码里，最近的取消点肯定就是pthread_cond_wait()了。
    pthread_cancel(tid);
    pthread_join(tid, NULL);
    printf("All done -- exiting\n");
    return 0;
}
```

## 参考：
https://www.cnblogs.com/cthon/p/9084735.html

# 优先级相关知识
## 主要概念
### 优先级反转
高优先级任务被低优先级任务阻塞，导致高优先级任务迟迟得不到调度。但其他中等优先级的任务却能抢到CPU资源。

![优先级反转图解](imags/优先级反转.png)
#### for example：
进程A，进程B，进程C。优先级依次递增。
进程C优先级最低，但进程C最先运行（占有锁），
此时进程B运行，因为优先级比C高，因此抢占运行，
过了一会进程A运行，原本进程A想要抢占进程B，但拿不到锁（被C占有锁），必须等待锁释放。但C又因为被进程B抢占，需要等待进程B结束...即使进程B结束后，C也会被A抢占，且A又等待C释放资源...出现死锁。
Ps：当发现高优先级的任务因为低优先级任务占用资源而阻塞时，就将低优先级任务的优先级提升到等待它所占有的资源的最高优先级任务的优先级。
这个过程叫做优先级反转。

Ps：如何解决优先级反转?
解决优先级反转方法有多种，包括禁止中断、禁止抢占、优先级继承、优先级天花板等。


### 优先级继承

优先级天花板


参考：
https://blog.csdn.net/Ivan804638781/article/details/112698932
gitee：文档说明，示意图理解
https://gitee.com/aosp-riscv/working-group/blob/master/articles/20230804-linux-pi-pi.md

