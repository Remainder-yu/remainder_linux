## 需要补充的文档

1. 内核自检层：基于Linux内核内置机制，检测内核级异常；
2. 系统存活层：通过硬件/软件看门狗检查系统、进程、线程基本运行状态；

### 文档补充内容：
3.2 内核自检方案
内核自检基于内核本身机制，在启动脚本中自动配置内核检查功能，实现内核级检查。
3.3 watchdog_manager.ko实现
看门狗管理内核模块提供清硬狗定时器、注册设备文件接口 /dev/softdog，当不具备FIQ或硬件狗机制时，提供软狗检查功能。

### OS系统存活机制--看门狗机制
1）硬件上提供硬狗机制。
2）Uboot阶段使能硬狗机制，并开始清狗。
3）内核启动阶段穿插清狗操作。
4）实现内核模块，启动阶段默认插入ko，ko中 注册100ms定时器，定时清硬狗。注册FIQ的处理函数。
5）硬件支持咬狗后拉CPU复位前送FIQ给每个核， 软件在FIQ中记录本核上下文，打印到Dmesg，保存/dev/kmsg到/var/log， 然后硬件拉cpu复位（如果不支持硬件狗，或者硬件狗没有复位前送FIQ的能力，参考用备注中的的软件自检方案）

[软件看门狗机制](https://geek-blogs.com/blog/linux-software-watchdog/)
[linux内核系统的看门狗](https://blog.csdn.net/weixin_44376670/article/details/127228065)

内核相关配置：
CONFIG_WATCHDOG=y
CONFIG_WATCHDOG_CORE=y

### 内核自检方案
目标：内核自检基于内核本身机制，在启动脚本中自动配置内核检查功能，实现内核级检查。
[内核自检异常方案](https://sai-seres.feishu.cn/wiki/BUUSwT2yqiX2FNkdCoJcDBSmnnh?fromScene=spaceOverview&sheet=1LlDty)
内核自检当前基础方案：
![alt text](./image/os自检基础方案.png)

通过基础配置实现。

#### 详细分析

##### softlockup和hardlockup -- CPU层面

内核本身机制（CONFIG_LOCKUP_DETECTOR），启动脚本中
/proc/sys/kernel/soft_watchdog配置为1
/proc/sys/kernel/softlockup_panic配置为1
/proc/sys/kernel/nmi_watchdog配置为1
/proc/sys/kernel/watchdog_thresh配置为20

分析：
Soft lockup是指CPU被某内核代码占据，以至于无法执行其它进程。检测soft lockup的原理是给每个CPU分配一个定时执行的内核线程[watchdog/x]，如果该线程在设定的期限内没有得到执行的话就意味着发生了soft lockup，[watchdog/x]是SCHED_FIFO实时进程，优先级为最高的99，拥有优先运行的特权。

Hard lockup比soft lockup更加严重，CPU不仅无法执行其它进程，而且不再响应中断。检测hard lockup的原理利用了PMU的NMI perf event，因为NMI中断是不可屏蔽的，在CPU不再响应中断的情况下仍然可以得到执行，它再去检查时钟中断的计数器hrtimer_interrupts是否在保持递增，如果停滞就意味着时钟中断未得到响应，也就是发生了hard lockup。

##### hungtask -- task层面

"内核本身机制（CONFIG_DETECT_HUNG_TASK），启动脚本中
/proc/sys/kernel/hung_task_timeout_secs配置为120
/proc/sys/kernel/hung_task_panic配置为1
/proc/sys/kernel/hung_task_all_cpu_backtrace配置为1"

Hung Task（挂起任务）是指 处于 D 状态（不可中断睡眠）超过特定时间阈值 的内核任务。D 状态任务通常是在等待某些内核资源（如 I/O 完成、锁释放等），但如果等待时间过长，可能表示系统存在死锁或资源饥饿问题。

当前配置会导致panic重启。
```shell
# 配置文件：/proc/sys/kernel/hung_task_*

# 1. 超时阈值（秒）
/proc/sys/kernel/hung_task_timeout_secs = 120
# 作用：定义任务处于D状态多长时间被认为是"挂起"

# 2. 触发panic开关
/proc/sys/kernel/hung_task_panic = 1
# 作用：检测到hung task时是否触发内核panic
# 0: 只打印警告
# 1: 触发panic重启系统

# 3. 全CPU回溯开关
/proc/sys/kernel/hung_task_all_cpu_backtrace = 1
# 作用：是否收集所有CPU的堆栈信息
# 0: 只打印挂起任务的堆栈
# 1: 打印所有CPU的堆栈，帮助分析系统范围问题

# 4. 最大警告次数（默认10）
/proc/sys/kernel/hung_task_warnings = 10
# 作用：限制警告信息数量，避免日志风暴

```


##### OOM	-- 内存层面

"首先需要配置内存分配策略。按照嵌入式设备的典型配置策略建议启动脚本中
/proc/sys/vm/overcommit_memory配置为2
/proc/sys/vm/overcommit_ratio配置为150
OOM本身故障后，配置为复位系统。
/proc/sys/vm/panic_on_oom配置为2"

建议：
```shell
配置项	推荐值	说明
overcommit_memory	2	严格检查，避免内存超配
overcommit_ratio	150	150%超配比例，平衡安全和效率
panic_on_oom	2	任何OOM都panic，确保系统复位
oom_dump_tasks	1	OOM时转储任务信息,现场可用于分析
swappiness	10	低交换倾向（嵌入式）
```

##### 其他
workqueue stall	内核本身机制(CONFIG_WQ_WATCHDOG)，不需要额外配置
lockdep	暂时先不做，遗留


