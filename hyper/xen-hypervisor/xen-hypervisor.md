## xen的虚拟机间通信研究
xen虚拟环境由一下部件构成，虚拟机监控器VMM、特权虚拟机和客户虚拟机。
虚拟机监控器VMM直接安装在已有的硬件环境上，它的主要任务是实现虚拟机技术的各种底层机制，如虚拟化CPU、虚拟化设备、虚拟化内存和虚拟化网络等机制都在HMM中实现的。
特权虚拟机是最先被创建的虚拟机，同时，它作为虚拟机监控器的扩充而存在，它直接拥有系统的硬件输入和输出设备的控制权，提供对这些IO设备响应的驱动程序，并且支持对客户虚拟机的管理功能。

### xen概述
xen对虚拟机的虚拟化分为两大类，半虚拟化（Para virtualization）和完全虚拟化（Hardware VirtualMachine）。

#### 半虚拟化（Para virtualization）
这种技术允许虚拟机操作系统感知到自己运行在Xen Hypervisor上，而不是直接运行在硬件上，同时也可以识别出其他运行在相同环境中的客户虚拟机。
在Xen Hypervisor上运行的半虚拟化的操作系统，为了调用系统管理程序（Xen Hypervisor），要有选择地修改操作系统，然而却不需要修改操作系统上运行的应用程序。由于 Xen 需要修改操作系统内核，所以您不能直接让当前的 Linux 内核在 Xen 系统管理程序中运行，除非它已经移植到了Xen 架构。不过，如果当前系统可以使用新的已经移植到 Xen 架构的Linux 内核，那么您就可以不加修改地运行现有的系统。

![](vx_images/69795340095953.png =500x)

#### 完全虚拟化(HVM)
完全虚拟化（Hardware Virtual Machine）又称“硬件虚拟化”，简称HVM，是指运行在虚拟环境上的虚拟机在运行过程中始终感觉自己是直接运行在硬件之上的，并且感知不到在相同硬件环境下运行着其他虚拟机的虚拟技术。

在Xen Hypervisor运行的完全虚拟化虚拟机，所运行的操作系统都是标准的操作系统，即：无需任何修改的操作系统版本。同时也需要提供特殊的硬件设备。

值的注意的是，在Xen上虚拟的Windows虚拟机必须采用完全虚拟化技术。
![](vx_images/536085206047169.png =500x)

#### CPU完全虚拟化，IO半虚拟化(PVHVM)
为了提高性能，完全虚拟化的Guests可以使用特殊的半虚拟设备驱动程序（PVHVM或PV-on-HVM驱动）。这些驱动程序在HVM环境下优化你的 PV驱动，模拟的磁盘和网络IO旁路运行，从而让你的PV在HVM中有更好的性能。这意味着你在用户窗口操作方面可以得到最佳的性能。

注意，Xen项目PV（半虚拟化）的Guest自动使用PV驱动：因此不需要这些驱动程序,因为你已经自动使用优化的驱动程序。PVHVM只会在HVM（全虚拟化）guest虚拟机中需要。

![](vx_images/147147252906763.png =500x)


### xen基本组件

1. Xen Hypervisor：直接运行于硬件之上是Xen客户操作系统与硬件资源之间的访问接口(如：)。通过将客户操作系统与硬件进行分类，Xen管理系统可以允许客户操作系统安全，独立的运行在相同硬件环境之上。
2. Domain 0：运行在Xen管理程序之上，具有直接访问硬件和管理其他客户操作系统的特权的客户操作系统。
3. DomainU：运行在Xen管理程序之上的普通客户操作系统或业务操作系统，不能直接访问硬件资源（如：内存，硬盘等），但可以独立并行的存在多个。

1、Xen Hypervisor
Xen Hypervisor是直接运行在硬件与所有操作系统之间的基本软件层。它负责为运行在硬件设备上的不同种类的虚拟机（不同操作系统）进行CPU调度和内 存分配。Xen Hypervisor对虚拟机来说不单单是硬件的抽象接口，同时也控制虚拟机的执行，让他们之间共享通用的处理环境。
Xen Hypervisor不负责处理诸如网络、外部存储设备、视频或其他通用的I/O处理。

2、Domain 0
Domain 0 是经过修改的Linux内核，是运行在Xen Hypervisor之上独一无二的虚拟机，拥有访问物理I/O资源的特权，并且可以与其他运行在Xen Hypervisor之上的其他虚拟机进行交互。所有的Xen虚拟环境都需要先运行Domain 0，然后才能运行其他的虚拟客户机。
Domain 0 在Xen中担任管理员的角色，它负责管理其他虚拟客户机。
在Domain 0中包含两个驱动程序，用于支持其他客户虚拟机对于网络和硬盘的访问请求。这两个驱动分别是Network Backend Driver和Block Backend Driver。
Network Backend Driver直接与本地的网络硬件进行通信，用于处理来自Domain U客户机的所有关于网络的虚拟机请求。根据Domain U发出的请求Block Backend Driver直接与本地的存储设备进行通信然后，将数据读写到存储设备上。

3、Domain U
Domain U客户虚拟机没有直接访问物理硬件的权限。所有在Xen Hypervisor上运行的半虚拟化客户虚拟机（简称：Domain U PV Guests）都是被修改过的基于Linux的操作系统、Solaris、FreeBSD和其他基于UNIX的操作系统。所有完全虚拟化客户虚拟机（简称：Domain U HVM Guests）则是标准的Windows和其他任何一种未被修改过的操作系统。
无论是半虚拟化Domain U还是完全虚拟化Domain U，作为客户虚拟机系统，Domain U在Xen Hypervisor上运行并行的存在多个，他们之间相互独立，每个Domain U都拥有自己所能操作的虚拟资源（如：内存，磁盘等）。而且允许单独一个Domain U进行重启和关机操作而不影响其他Domain U。

![](vx_images/424636576247179.png)

### Xen基本体系架构及运行原理

#### Xen体系架构
Xen 的 VMM ( Xen Hyperviso ) 位于操作系统和硬件之间，负责为上层运行的操作系统内核提供虚拟化的硬件资源，负责管理和分配这些资源，并确保上层虚拟机（称为域 Domain）之间的相互隔离。Xen采用混合模式，因而设定了一个特权域用以辅助Xen管理其他的域，并提供虚拟的资源服务，该特权域称为Domain 0，而其余的域则称为Domain U。

Xen向Domain提供了一个抽象层，其中包含了管理和虚拟硬件的API。Domain 0内部包含了真实的设备驱动（原生设备驱动），可直接访问物理硬件，负责与 Xen 提供的管理 API 交互，并通过用户模式下的管理工具来管理 Xen 的虚拟机环境。

xen2.0之后，引入了分离设备驱动模式。该模式在每个用户域中建立前端（front end）设备，在特权域Dom0中建立后端backend设备。所有的用户域操作系统像使用普通设备一样向前端设备发送请求，而前端设备通过IO请求描述符(IO descripror ring)和设备通道（device channel）将这些请求以及用户域的身份信息发送到处于特权域中的后端设备。这种体系将控制信息传递和数据传递分开处理。

#### 不同虚拟技术的运行机制

#### Domain管理和控制
开源社区将一些列的Linux精灵程序分类为“管理”和“控制”两大类。这些服务支撑着整个虚拟环境的管理和控制操作，并且存在于Domain 0虚拟机中。

xend
XM
Xenstored
Libxenctl
Qemu-DM
Xen Virtual Firmware

### xen的网络架构

支持三种模式：
Bridge模式
Route模式
Nat模式：

#### Xen Domain U Guests 发送数据包处理流程

#### Xen中虚拟网卡与物理网卡之间的关系

### xen中DomU与Dom0之间的通信机制
首先介绍用于Dom0和DomU通信的相关技术

1. 事件通道：用于Dom和Xen之间、Dom和Dom之间异步事件通知机制
2. I/O 共享环：I/O 共享环是在不同Domain 之间存在的一块固定的共享内存，用于在DomainU 和Domain0 之间传递I/O 请求和响应。I/O 共享环利用生产者和消费者的机理，来产生发送以及响应I/O 请求。
3. 授权表(Grant Table)：授权表(Grant Table)是在不同Domain 之间高效传输I/O 数据的机制。
![](https://img-blog.csdn.net/20171129225138796)

详细通信过程还需分析学习：


### 参考文章：
[关于Xen虚拟化基本原理详解](https://zhuanlan.zhihu.com/p/549570851)
[基于共享内存的Xen虚拟机间通信的研究](https://wenku.baidu.com/view/b484cb88d6bbfd0a79563c1ec5da50e2524dd1dc.html?_wkts_=1704866925157)
[xen中DomU和Dom0之间的通信机制](https://blog.csdn.net/yzy1103203312/article/details/78670937)
[RPMsg：协议简介](https://www.cnblogs.com/sky-heaven/p/13624371.html)
[Rpmsg与Virtio介绍](https://blog.csdn.net/weixin_42813232/article/details/125577142)
[A核与M核异构通信过程解析](https://blog.csdn.net/weixin_43717839/article/details/129119174?utm_medium=distribute.pc_relevant.none-task-blog-2~default~baidujs_baidulandingword~default-5-129119174-blog-86601694.235%5Ev40%5Epc_relevant_3m_sort_dl_base1&spm=1001.2101.3001.4242.4&utm_relevant_index=8)

## Qemu启动XEN
