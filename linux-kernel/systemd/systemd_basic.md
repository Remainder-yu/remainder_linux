
# systemd原理

## systemd概述
Systemd 是一系列工具的集合，其作用也远远不仅是启动操作系统，它还接管了后台服务、结束、状态查询，以及日志归档、设备管理、电源管理、定时任务等许多职责，并支持通过特定事件（如插入特定 USB 设备）和特定端口数据触发的 On-demand（按需）任务。
Systemd 的后台服务还有一个特殊的身份——它是系统中 PID 值为 1 的进程。
- 更少的进程
Systemd提供了服务按需启动的能力，使得特定的服务只有在真定被请求时才启动。
- 允许更多的进程并行启动

##  systemd时代的开机启动流程




## 参考文献
[systemd时代的开机启动流程(UEFI+systemd)](https://www.junmajinlong.com/linux/systemd/systemd_bootup/index.html)
[可能是史上最全面易懂的 Systemd 服务管理教程](https://cloud.tencent.com/developer/article/1516125)


# bootup 系统启动流程

## 背景-计算机启动流程
- 内核加载前
  - 本阶段与操作系统无关，任何操作系统在这个阶段的顺序都是一致
- 内核加载中-> 内核启动完成
- 内核加载后-> 系统环境初始化完成
- 终端加载、用户登录
这几个阶段中又有很多小阶段，每个阶段都各司其职。

firmware -> kernel launch -> ramfs -> OS launch

### 按下电源和固件阶段

### 内核启动阶段
内核已经被加载到内核掌握了控制前，且收到了u-boot最后传递的内核启动参数，包括init ramdisk镜像的路径
u-boot会对内核进行解压操作？解压操作基于哪个函数接口？startup_32初始化内核启动环境，然后跳转linux-kernel汇编代码，然后再跳转至start_kernel函数？
什么是PID=0进程？


### init ramdisk
如何找到init ramdisk？
为什么要用init ramdisk？
内核中根分区的来源？

### PID=1的进程来源




## 流程图
整理流程图的方法：
```shell
# 查看一个 Target 包含的所有 Unit
$ systemctl list-dependencies multi-user.target
#
systemd-analyze critical-chain xxx.service/target
# 按照以上指令，就可以分析出基本的target的基础service的关系，整理处流程图。
```

##


## 参考文献
[bootup 中文手册](https://www.jinbuguo.com/systemd/bootup.html)
