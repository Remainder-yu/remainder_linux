
## 常规启动流程
当成功挂载了"root="内核引导选项指定的根文件系统之后，内核将启动由"init=“内核引导选项指定的init程序， 从这个时间点开始，即进入了"常规启动流程”： 检测硬件设备并加载驱动、挂载必要的文件系统、启动所有必要的服务，等等。对于 systemd(1) 系统来说，上述"init程序"就是 systemd 进程， 而整个"常规启动流程"也以几个特殊的 target 单元(详见 systemd.target(5)) 作为节点，被划分为几个阶段性步骤。 在每个阶段性步骤内部，任务是高度并行的， 所以无法准确预测同一阶段内单元的先后顺序， 但是不同阶段之间的先后顺序总是固定的。

## 流程图
这些具有特定含义的 target 单元之间的依赖关系 以及各自在启动流程中的位置。 图中的箭头表示了单元之间的依赖关系与先后顺序， 整个图表按照自上而下的时间顺序执行。·

```shell

 (各个 mounts 与       (各个 swap
 fsck services……)      devices……)                              (各个底层服务:      (各个底层虚拟
         |                  |                                udevd, tmpfiles,    文件系统 mounts:
         v                  v                                  random seed,        mqueue, configfs,
  local-fs.target      swap.target      getty.target          sysctl, ……)      debugfs, ……)
         |                  |                  |                    |                    |
         \__________________|_________________ | ___________________|____________________/
                                              \|/
                                               v
                                        sysinit.target
                                               |
          ____________________________________/|\________________________________________
         /                  |                  |                    |                    \
         |                  |                  |                    |                    |
         v                  v                  |                    v                    v
      (各个               (各个                |                  (各个           rescue.service
    timers……)           paths……)               |               sockets……)                |
         |                  |                  |                    |                    v
         v                  v                  |                    v              rescue.target
   timers.target      paths.target             |             sockets.target
         .                  |                  |                    |
         .                  \_________________ | ___________________/
         .................................... \|/
                                               v
                                         basic.target
                                               |
                                               |                                 emergency.service
                                               |                                         |
                                               |                                         v
                                               v                                 emergency.target
                                          (各个系统服务)
                                               |
                                               |
                                               v
                                        multi-user.target
```

关机时。init程序将会停止所有服务、卸载所有文件系统、（可选的）返回initrd环境卸载根文件系统，最后关闭电源。

##

```shell
remainder@ubuntu:~$ systemd-analyze critical-chain -H "root@10.234.84.59"
root@10.234.84.59's password:
The time after the unit is active or started is printed after the "@" character.
The time the unit takes to start is printed after the "+" character.

multi-user.target @4.195s
└─systemd-logind.service @3.296s +305ms
  └─basic.target @3.085s
    └─sockets.target @3.069s
      └─dbus.socket @3.051s
        └─sysinit.target @2.958s
          └─systemd-update-utmp.service @2.898s +43ms
            └─auditd.service @2.299s +64ms
              └─systemd-tmpfiles-setup.service @2.158s +72ms
                └─local-fs.target @2.062s
                  └─var-volatile-spool.service @1.856s +183ms
                    └─var-volatile.mount @1.321s +296ms
                      └─systemd-journald.socket @1.130s
                        └─-.mount @792ms
                          └─system.slice @792ms
                            └─-.slice @792ms
remainder@ubuntu:~$ systemd-analyze critical-chain  local-fs.target  -H "root@10.234.84.59"
root@10.234.84.59's password:
The time after the unit is active or started is printed after the "@" character.
The time the unit takes to start is printed after the "+" character.

local-fs.target @2.062s
└─var-volatile-spool.service @1.856s +183ms
  └─var-volatile.mount @1.321s +296ms
    └─systemd-journald.socket @1.130s
      └─-.mount @792ms
        └─system.slice @792ms
          └─-.slice @792ms
remainder@ubuntu:~$ systemd-analyze critical-chain  getty.target  -H "root@10.234.84.59"
root@10.234.84.59's password:
The time after the unit is active or started is printed after the "@" character.
The time the unit takes to start is printed after the "+" character.

getty.target @3.562s
└─getty@tty1.service @3.496s
  └─systemd-user-sessions.service @3.319s +113ms
    └─basic.target @3.085s
      └─sockets.target @3.069s
        └─dbus.socket @3.051s
          └─sysinit.target @2.958s
            └─systemd-update-utmp.service @2.898s +43ms
              └─auditd.service @2.299s +64ms
                └─systemd-tmpfiles-setup.service @2.158s +72ms
                  └─local-fs.target @2.062s
                    └─var-volatile-spool.service @1.856s +183ms
                      └─var-volatile.mount @1.321s +296ms
                        └─systemd-journald.socket @1.130s
                          └─-.mount @792ms
                            └─system.slice @792ms
                              └─-.slice @792ms
remainder@ubuntu:~$ systemd-analyze critical-chain  remote-fs.target  -H "root@10.234.84.59"
root@10.234.84.59's password:
The time after the unit is active or started is printed after the "@" character.
The time the unit takes to start is printed after the "+" character.

remote-fs.target @973ms
remainder@ubuntu:~$ systemd-analyze critical-chain  basic.target  -H "root@10.234.84.59"
root@10.234.84.59's password:
The time after the unit is active or started is printed after the "@" character.
The time the unit takes to start is printed after the "+" character.

basic.target @3.085s
└─sockets.target @3.069s
  └─dbus.socket @3.051s
    └─sysinit.target @2.958s
      └─systemd-update-utmp.service @2.898s +43ms
        └─auditd.service @2.299s +64ms
          └─systemd-tmpfiles-setup.service @2.158s +72ms
            └─local-fs.target @2.062s
              └─var-volatile-spool.service @1.856s +183ms
                └─var-volatile.mount @1.321s +296ms
                  └─systemd-journald.socket @1.130s
                    └─-.mount @792ms
                      └─system.slice @792ms
                        └─-.slice @792ms
remainder@ubuntu:~$ systemd-analyze critical-chain  sysinit.target  -H "root@10.234.84.59"
root@10.234.84.59's password:
The time after the unit is active or started is printed after the "@" character.
The time the unit takes to start is printed after the "+" character.

sysinit.target @2.958s
└─systemd-update-utmp.service @2.898s +43ms
  └─auditd.service @2.299s +64ms
    └─systemd-tmpfiles-setup.service @2.158s +72ms
      └─local-fs.target @2.062s
        └─var-volatile-spool.service @1.856s +183ms
          └─var-volatile.mount @1.321s +296ms
            └─systemd-journald.socket @1.130s
              └─-.mount @792ms
                └─system.slice @792ms
                  └─-.slice @792ms

```
