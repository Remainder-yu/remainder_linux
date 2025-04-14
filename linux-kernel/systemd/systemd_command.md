## 0.1. 参考文献：
[CSDN-systemd服务配置文件详解](https://blog.csdn.net/hjx020/article/details/120978201)
[配置文件字段官方解释说明](https://docs.fedoraproject.org/en-US/quick-docs/systemd-understanding-and-administering/)
[systemd时代的开机启动流程](https://www.junmajinlong.com/linux/systemd/systemd_bootup/index.html)
https://www.junmajinlong.com/linux/index/#systemd

# 1. 配置文件
## 1.1. systemd开机启动原理
支持systemd的软件，安装的时候，会自动在/lib/systemd/system目录下添加一个配置文件
如果你想开机自启动，可以使用systemctl enable。该命令就会在/etc/systemd/system目录下添加一个符号链接，指向/lib/systemd/system目录下的对应文件。
开机时，Systemd只执行/etc/systemd/system目录里面的配置文件。这也意味着，如果把修改后的配置文件放在该目录，就可以达到覆盖原始配置的效果。

## 1.2. 读懂配置文件
一个服务怎么启动，完全由它的配置文件决定。

```shell
$ systemctl cat sshd.service
# /usr/lib/systemd/system/sshd.service
[Unit]  # 启动顺序与依赖关系
Description=OpenSSH server daemon
Documentation=man:sshd(8) man:sshd_config(5)
After=network.target sshd-keygen.target # 表示如果network.target或sshd-keygen.service需要启动，那么sshd.service应该在它们之后启动
Wants=sshd-keygen.target
# After和Before字段只涉及启动顺序，不涉及依赖关系。
# 设置依赖关系，需要使用Wants字段和Requires字段。
# Wants字段：表示sshd.service与sshd-keygen.service之间存在"弱依赖"关系，即如果"sshd-keygen.service"启动失败或停止运行，不影响sshd.service继续执行。
# Requires字段则表示"强依赖"关系，即如果该服务启动失败或异常退出，那么sshd.service也必须退出。
# 注意，Wants字段与Requires字段只涉及依赖关系，与启动顺序无关，默认情况下是同时启动的

# Service区块定义如何启动当前服务。
[Service]  # 启动行为
Type=notify
EnvironmentFile=-/etc/sysconfig/sshd # 指定当前服务的环境参数文件
ExecStart=/usr/sbin/sshd -D $OPTIONS  # 定义启动进程时执行的命令。
ExecReload=/bin/kill -HUP $MAINPID
KillMode=process
Restart=on-failure
RestartSec=42s

# 区块，定义如何安装这个配置文件，即怎样做到开机启动。
[Install]
WantedBy=multi-user.target

```

所有的启动设置之前，都可以加上一个连词号（-），表示"抑制错误"，即发生错误的时候，不影响其他命令的执行。比如，EnvironmentFile=-/etc/sysconfig/sshd（注意等号后面的那个连词号），就表示即使/etc/sysconfig/sshd文件不存在，也不会抛出错误

### 1.2.1. unit
OnFailure 和 OnFailureJobMode 是用来定义服务失败时的行为的指令。
OnFailure 指令指定了当服务单元文件失败时应该触发的目标（target）或服务单元文件。
OnFailureJobMode 指令定义了如何处理与 OnFailure 指定的目标相冲突的现有作业（jobs）。

### 1.2.2. Service

Type字段定义启动类型，它可以设置的值如下：
- simple（默认值）：Execstart字段启动的进程为主进程
- forking：ExecStart字段将以fork()方式启动，此时父进程将会退出，子进程成为主进程
- oneshot：类似于simple，但只执行一次，Systemd等他执行完，才启动其他服务
- dbus：
- notify：类似于simple，启动结束后会发出通知信号
- idle：等到其他任务都执行完，才启动该服务。

Restart字段：
KillMode字段：


### 1.2.3. [Install]区块

Target含义是服务组，表示一组服务。WantedBy=multi-user.target指的是，sshd 所在的 Target 是multi-user.target。
这个设置非常重要，因为执行systemctl enable sshd.service命令时，sshd.service的一个符号链接，就会放在/etc/systemd/system目录下面的multi-user.target.wants子目录之中。

```shell
#查看 multi-user.target 包含的所有服务
$ systemctl list-dependencies multi-user.target

#切换到另一个 target
#shutdown.target 就是关机状态
$ sudo systemctl isolate shutdown.target
```
一般来说，常用的 Target 有两个：一个是multi-user.target，表示多用户命令行状态；另一个是graphical.target，表示图形用户状态，它依赖于multi-user.target。官方文档有一张非常清晰的 Target 依赖关系图。


## 1.3. Target的配置文件

```shell

$ systemctl cat multi-user.target
# /usr/lib/systemd/system/multi-user.target
#  SPDX-License-Identifier: LGPL-2.1+
#
#  This file is part of systemd.
#
#  systemd is free software; you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation; either version 2.1 of the License, or
#  (at your option) any later version.

[Unit]
Description=Multi-User System
Documentation=man:systemd.special(7)
Requires=basic.target # 要求basic.target一起运行。
Conflicts=rescue.service rescue.target # 冲突字段。如果rescue.service或rescue.target正在运行，multi-user.target就不能运行，反之亦然。
After=basic.target rescue.service rescue.target # 表示multi-user.target在basic.target 、 rescue.service、 rescue.target之后启动，如果它们有启动的话。
AllowIsolate=yes # 允许使用systemctl isolate命令切换到multi-user.target。
```

修改配置文件：
```shell
#重新加载配置文件
$ sudo systemctl daemon-reload
#重启相关服务
$ sudo systemctl restart foobar
```

# 2. systemd命令详解

## 2.1. systemd工具集
```shell
systemctl：用于检查和控制各种系统服务和资源的状态
bootctl：用于查看和管理系统启动分区
hostnamectl：用于查看和修改系统的主机名和主机信息
journalctl：用于查看系统日志和各类应用服务日志
localectl：用于查看和管理系统的地区信息
loginctl：用于管理系统已登录用户和 Session 的信息
machinectl：用于操作 Systemd 容器
timedatectl：用于查看和管理系统的时间和时区信息
systemd-analyze 显示此次系统启动时运行每个服务所消耗的时间，可以用于分析系统启动过程中的性能瓶颈
systemd-ask-password：辅助性工具，用星号屏蔽用户的任意输入，然后返回实际输入的内容
systemd-cat：用于将其他命令的输出重定向到系统日志
systemd-cgls：递归地显示指定 CGroup 的继承链
systemd-cgtop：显示系统当前最耗资源的 CGroup 单元
systemd-escape：辅助性工具，用于去除指定字符串中不能作为 Unit 文件名的字符
systemd-hwdb：Systemd 的内部工具，用于更新硬件数据库
systemd-delta：对比当前系统配置与默认系统配置的差异
systemd-detect-virt：显示主机的虚拟化类型
systemd-inhibit：用于强制延迟或禁止系统的关闭、睡眠和待机事件
systemd-machine-id-setup：Systemd 的内部工具，用于给 Systemd 容器生成 ID
systemd-notify：Systemd 的内部工具，用于通知服务的状态变化
systemd-nspawn：用于创建 Systemd 容器
systemd-path：Systemd 的内部工具，用于显示系统上下文中的各种路径配置
systemd-run：用于将任意指定的命令包装成一个临时的后台服务运行
systemd-stdio- bridge：Systemd 的内部 工具，用于将程序的标准输入输出重定向到系统总线
systemd-tmpfiles：Systemd 的内部工具，用于创建和管理临时文件目录
systemd-tty-ask-password-agent：用于响应后台服务进程发出的输入密码请求
```


## 2.2. systemctl命令--管理服务的命令

文件分析：
```shell
## 依赖关系
systemctl list-dependencies ​auditd.service
```
## 2.3. systemd-analyze--系统启动性能分析工具

### 2.3.1. 命令参数详解

### 2.3.2. 用例举例

#### 2.3.2.1. 显示启动时间
#### 2.3.2.2. 列出启动过程中各个服务的启动时间
#### 2.3.2.3. 显示系统启动过程的关键链
#### 2.3.2.4. 生成启动过程的SVG图像
#### 2.3.2.5. 生成依赖关系图-dot

### 2.3.3. 性能分析


# 3. 日志系统

