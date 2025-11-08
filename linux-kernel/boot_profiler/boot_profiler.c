#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include "boot_profiler.h"

struct boot_profiler g_boot_profiler = {
    .magic = BOOT_PROFILER_MAGIC,
    .count = 0,
    .lock = __SPIN_LOCK_UNLOCKED(g_boot_profiler.lock),
};

EXPORT_SYMBOL_GPL(g_boot_profiler);
/* 1. 内部实现：真正可被导出的函数 */
void boot_profile_point(const char *name)
{
    unsigned long flags;
    spin_lock_irqsave(&g_boot_profiler.lock, flags);
    if (g_boot_profiler.count < MAX_BOOT_POINTS) {
        struct boot_point *p = &g_boot_profiler.points[g_boot_profiler.count++];
        p->name = name;
        p->tick = read_sysreg_cntvct();
    }
    spin_unlock_irqrestore(&g_boot_profiler.lock, flags);
}
EXPORT_SYMBOL_GPL(boot_profile_point);

static int __init boot_profiler_core_init(void)
{
    /* 一次性读取频率 */
    asm volatile("mrs %0, cntfrq_el0" : "=r" (g_boot_profiler.cntfrq));
    pr_info("boot_profiler: cntfrq=%llu Hz\n", g_boot_profiler.cntfrq);
    return 0;
}
early_initcall(boot_profiler_core_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ARM64 boot profiler core");