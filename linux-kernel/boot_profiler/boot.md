## 核心方案
1. 打点阶段只保存 tick 计数值，不做任何频率换算；
2. 最终用户空间读取 一次性 完成：
3. 读取所有原始 tick 值
4. 根据 cntfrq_el0 换算成绝对时间（ns）
5. 计算相邻点 delta 及累计绝对时间
6. 内核接口统一为 ioctl，避免多次 read() 往返；
最后提供 用户空间参考代码 一键展示绝对时间轴。

### 使用实例

```shell
/* 在 init/main.c 等位置 */
#include <boot_profiler.h>

asmlinkage __visible void __init start_kernel(void)
{
    BOOT_PROFILE_POINT("start_kernel");
    ...
    BOOT_PROFILE_POINT("mm_init_done");
    ...
    BOOT_PROFILE_POINT("rest_init");
}

```

### 

例如：
```shell
ARM64 Boot Timeline (cntfrq=24000000 Hz)
Name                                               AbsTime(ns)   Delta(ms)  Cumul(ms)
------------------------------------------------------------------------------------------
start_kernel                                         12345678      0.514      0.514
mm_init_done                                        234567890      9.255      9.769
rest_init                                          3456789012    133.842    143.611
...

```

### 待测试验证