
- [1. epoll篇](#1-epoll篇)

# 1. epoll篇

源码分析
```c
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

```c
// /usr/include/x86_64-linux-gnu/sys/epoll.h
typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event
{
  uint32_t events;	/* Epoll events */
  epoll_data_t data;	/* User data variable */
} __EPOLL_PACKED;
```

`epoll`相关的内核代码在 `fs/eventpoll.c`文件中。内核实现方式？

每创建一个epollfd，内额就会分配一个eventpoll与之对应，可以硕士内核态的epollfd。

```c
struct eventpoll {
    /* Protect the this structure access */
    spinlock_t lock;
    /*
     * This mutex is used to ensure that files are not removed
     * while epoll is using them. This is held during the event
     * collection loop, the file cleanup path, the epoll file exit
     * code and the ctl operations.
     */
    /* 添加, 修改或者删除监听fd的时候, 以及epoll_wait返回, 向用户空间
     * 传递数据时都会持有这个互斥锁, 所以在用户空间可以放心的在多个线程
     * 中同时执行epoll相关的操作, 内核级已经做了保护. */
    struct mutex mtx;
    /* Wait queue used by sys_epoll_wait() */
    /* 调用epoll_wait()时, 我们就是"睡"在了这个等待队列上... */
    wait_queue_head_t wq;
    /* Wait queue used by file->poll() */
    /* 这个用于epollfd本事被poll的时候... */
    wait_queue_head_t poll_wait;
    /* List of ready file descriptors */
    /* 所有已经ready的epitem都在这个链表里面 */
    struct list_head rdllist;
    /* RB tree root used to store monitored fd structs */
    /* 所有要监听的epitem都在这里 */
    struct rb_root rbr;
    /*
        这是一个单链表链接着所有的struct epitem当event转移到用户空间时
     * This is a single linked list that chains all the "struct epitem" that
     * happened while transfering ready events to userspace w/out
     * holding ->lock.
     */
    struct epitem *ovflist;
    /* The user that created the eventpoll descriptor */
    /* 这里保存了一些用户变量, 比如fd监听数量的最大值等等 */
    struct user_struct *user;
};
```

```c

```
