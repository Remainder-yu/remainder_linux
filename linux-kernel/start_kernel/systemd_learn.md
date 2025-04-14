# systemd基础

## Systemd概述
Systemd是Linux系统下的一个系统和服务管理器，它作为Linux系统启动时的初始化进程，负责启动和管理系统中的服务和进程。Systemd的主要目标是将传统的init进程替换为一个新的初始化进程，以提供更高效、更灵活的进程管理方式。

Systemd的主要特点包括：
- 支持并行启动和停止服务，提高了系统启动速度。
- 接管后台服务、结束、状态查询
- 日志归档
- 设备管理
- 电源管理
- 定时任务

Systemd的Unit文件：
Systemd可以管理所有系统资源，不同的资源统称为Unit。
在Systemd中，Unit文件同意了过去各种不同系统资源配置格式，例如服务的启停、定时任务、设备自动挂载、网络配置、虚拟内存配置等。
> systemd通过不同的我呢见后缀来区分这些配置文件；

12种Unit文件后缀：
- automount：用于控制自动挂载文件系统，相当于 SysV-init 的 autofs 服务
- .device：对于 /dev 目录下的设备，主要用于定义设备之间的依赖关系
- .mount：定义系统结构层次中的一个挂载点，可以替代过去的 /etc/fstab 配置文件
- .path：用于监控指定目录或文件的变化，并触发其它 Unit 运行
- .scope：这种 Unit 文件不是用户创建的，而是 Systemd 运行时产生的，描述一些系统服务的分组信息
- .service：封装守护进程的启动、停止、重启和重载操作，是最常见的一种 Unit 文件
- .slice：用于表示一个 CGroup 的树，通常用户不会自己创建这样的 Unit 文件
- .snapshot：用于表示一个由 systemctl snapshot 命令创建的 Systemd Units 运行状态快照
- .socket：监控来自于系统或网络的数据消息，用于实现基于数据自动触发服务启动
- .swap：定义一个用户做虚拟内存的交换分区
- .target：用于对 Unit 文件进行逻辑分组，引导其它 Unit 的执行。它替代了 SysV-init 运行级别的作用，并提供更灵活的基于特定设备事件的启动方式
- .timer：用于配置在特定时间触发的任务，替代了 Crontab 的功能

## systemd目录
因此，在三个目录中有同名文件的时候，只有优先级最高的目录里的那个文件会被使用。

/etc/systemd/system：系统或用户自定义的配置文件
/run/systemd/system：软件运行时生成的配置文件
/usr/lib/systemd/system：系统或第三方软件安装时添加的配置文件。

Systemd 默认从目录 /etc/systemd/system/ 读取配置文件。但是，里面存放的大部分文件都是符号链接，指向目录 /usr/lib/systemd/system/，真正的配置文件存放在那个目录。

通过 systemctl list-units --type=target 命令可以获取当前正在使用的运行目标

```shell
benshushu:~# systemctl list-units --type=target
UNIT                  LOAD   ACTIVE SUB    DESCRIPTION                         
basic.target          loaded active active Basic System                        
cryptsetup.target     loaded active active Local Encrypted Volumes             
getty.target          loaded active active Login Prompts                       
graphical.target      loaded active active Graphical Interface                 
local-fs-pre.target   loaded active active Local File Systems (Pre)            
local-fs.target       loaded active active Local File Systems                  
multi-user.target     loaded active active Multi-User System                   
network-online.target loaded active active Network is Online                   
network.target        loaded active active Network                             
paths.target          loaded active active Paths                               
remote-fs.target      loaded active active Remote File Systems                 
slices.target         loaded active active Slices                              
sockets.target        loaded active active Sockets                             
swap.target           loaded active active Swap                                
sysinit.target        loaded active active System Initialization               
time-sync.target      loaded active active System Time Synchronized            
timers.target         loaded active active Timers                              

LOAD   = Reflects whether the unit definition was properly loaded.
ACTIVE = The high-level unit activation state, i.e. generalization of SUB.
SUB    = The low-level unit activation state, values depend on unit type.
```

## systemctl的资源管理

```shell
# 列出正在运行的 Unit
$ systemctl list-units

# 列出所有Unit，包括没有找到配置文件的或者启动失败的
$ systemctl list-units --all

# 列出所有没有运行的 Unit
$ systemctl list-units --all --state=inactive

# 列出所有加载失败的 Unit
$ systemctl list-units --failed

# 列出所有正在运行的、类型为 service 的 Unit
$ systemctl list-units --type=service

# 查看 Unit 配置文件的内容
$ systemctl cat docker.service
```


## Target 管理

Target就是一个Unit组，包含许多相关的unit。启动target的时候，Systemd就会启动里面所有的unit。
在传统的SysV-init中，有运行级别这个概念，每个级别都定义了一系列应该启动的服务。systemd的target概念类似，而且功能更强大。它可以包含任意类型的unit。且多个target可以同时启动。

```shell
# 查看当前系统的所有 Target
$ systemctl list-targets

# 查看一个 Target 包含的所有 Unit
$ systemctl list-dependencies multi-user.target

# 设置当前系统的运行级别
# 实际上就是更改当前系统的 Target
$ systemctl set-default multi-user.target

# 查看启动时的默认 Target
$ systemctl get-default
```
Target 与 SysV-init 进程的主要区别：
默认的 RunLevel（在 /etc/inittab 文件设置）现在被默认的 Target 取代，位置是 /etc/systemd/system/default.target，通常符号链接到graphical.target（图形界面）或者multi-user.target（多用户命令行）。
启动脚本的位置，以前是 /etc/init.d 目录，符号链接到不同的 RunLevel 目录 （比如 /etc/rc3.d、/etc/rc5.d 等），现在则存放在 /lib/systemd/system 和 /etc/systemd/system 目录。
配置文件的位置，以前 init 进程的配置文件是 /etc/inittab，各种服务的配置文件存放在 /etc/sysconfig 目录。现在的配置文件主要存放在 /lib/systemd 目录，在 /etc/systemd 目录里面的修改可以覆盖原始设置。

# 参考：
[Systemd最全教程](https://cloud.tencent.com/developer/article/1516125)
