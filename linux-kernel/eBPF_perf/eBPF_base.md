## 1. eBPF概述

从eBPF官方图可知：eBPF程序运行在内核态，无需重新编译内核，也不需要编译内核模块并挂载，eBPF可以动态注入到内核中运行并随时卸载。一旦进入内核、eBPF便拥有了上帝视角，既可以监控内核，又可以管窥用户态程序。

从本质上说，BPF技术其实是kernel为用户态开的口子(内核已经做好了埋点)！通过注入eBPF程序并注册要关注事件、事件触发(内核回调你注入的eBPF程序)、内核态与用户态的数据交换实现你想要的逻辑。

![alt text](./images/eBPF框架图.png)

### 1.1. 钩子概览

eBPF程序是事件驱动的，当内核或应用程序通过某个钩子点时运行。预定义的钩子包括系统调用、函数入口/退出、内核跟踪点、网络事件等。
eBPF 程序只要不加载、不 attach（不运行），对系统性能 = 0 损耗。
eBPF 对系统的消耗只来自运行时，来源只有 3 个：
* 被 attach 到内核钩子（kprobe/tracepoint/perf event/XDP/tc）
* 事件触发时执行 BPF 指令
* 往 ringbuffer/perf buffer 写数据

## 2. eBPF架构

- 本章参考文档[3]

eBPF包括用户空间程序和内核程序两部分，用户空间程序负责加载BPF字节码至内核，内核中的BPF程序负责在内核中执行特定事件，用户空间程序与内核BPF程序可以使用map结构实现双向通信。
eBPF的工作逻辑如下：

1. eBPF Program通过LLVM/Clang编译成eBPF定义的字节码；
2. 通过系统调用bpf将字节码传入内核中；
3. 由verifier检验字节码的安全性、合规性；
4. 在确认字节码程序的安全性后，JIT Compiler会将其转换成可以在当前系统运行的机器码；
5. 根据程序类型的不同，把可运行机器码挂载到内核不同的位置/HOOK点；
6. 等待触发执行，其中不同的程序类型、不同的HOOK点的触发时机是不同的；并且在eBPF程序运行过程中，用户空间可通过eBPF map与内核进行双向通信；

### 2.1. eBPF加载过程

编译得到的BPF字节码文件，经过字节码读取、重定位和Verifier等过程，加载到内核；

### 2.2. eBPF开发工具链

要充分发挥eBPF的潜力，需要一套完整的开发工具链来支持其编写、编译、调试和部署。

#### 2.2.1. BCC

bcc是一个框架，它允许用户编写python程序，并将eBPF程序嵌入其中。该框架主要用于应用程序和系统的分析/跟踪等场景，其中eBPF程序用于收集统计数据或生成事件，而用户空间中的对应程序收集数据并以易理解的形式展示。运行python程序将生成eBPF字节码并将其加载到内核中。

#### 2.2.2. bpftrace

bpftrace是一种用于Linux eBPF的高级跟踪语言。bpftrace使用LLVM作为后端，将脚本编译为eBPF字节码，并利用BCC与linux eBPF子系统以及现有的Linux跟踪功能（内核动态跟踪（kprobes）、用户级动态跟踪（uprobes）和跟踪点）进行交互。

#### 2.2.3. libbpf C/C++库

libbpf库是一个基于C/C++的通用eBPF库，它可以帮助解耦将clang/LLVM编译器生成的eBPF对象文件的加载到内核中的这个过程，并通过为应用程序提供易于使用的库API来抽象与BPF系统调用的交互。



## 3. 如何开发eBPF程序

![alt text](./images/编码加载流程.png)

BPF形态的程序主要由两类源文件构成：
1. 一类是运行于内核态的eBPF的源代码文件
2. 另一类是用于向内核加载BPF程序、从内核加载、卸载BPF程序，与内核进行数据交互，展现用户态程序逻辑的源码文件。
目前运行于内核态的BPF程序只能用C语言开发(对应于第一类源代码文件，如下图bpf_program.bpf.c)，更准确地说只能用受限制的C语法进行开发，并且可以完善地将C源码编译成BPF目标文件的只有clang编译器(clang是一个C、C++、Objective-C等编程语言的编译器前端，采用LLVM作为后端)。


## 4. 参考文献及开源项目

1. [什么是 eBPF ？](https://ebpf.io/zh-hans/what-is-ebpf/)
2. [Linux 性能可观测性工具全解析：从基础到实践](https://geek-blogs.com/blog/linux-performance-observability-tools/)
3. [ebpf技术实践白皮书](https://openanolis.cn/assets/static/eBPF_technical_practice.pdf)
4. [github开源学习项目](https://github.com/eunomia-bpf/bpf-developer-tutorial/blob/main/src/9-runqlat/README.zh.md)
5. [eBPF核心技术与实践](https://uaxe.github.io/geektime-docs/%E8%BF%90%E7%BB%B4-%E6%B5%8B%E8%AF%95/eBPF%E6%A0%B8%E5%BF%83%E6%8A%80%E6%9C%AF%E4%B8%8E%E5%AE%9E%E6%88%98/eBPF%E6%A0%B8%E5%BF%83%E6%8A%80%E6%9C%AF%E4%B8%8E%E5%AE%9E%E6%88%98/)

### 4.1. 开源项目
[lmp-学习资料](https://gitee.com/linuxkerneltravel/lmp/tree/develop)
[perf-prof-系统级性能分析工具](https://gitee.com/OpenCloudOS/perf-prof.git)
[Ketones-内核 eBPF 追踪与可观测性原生引擎套件](https://gitee.com/openkylin/ketones/tree/master)
