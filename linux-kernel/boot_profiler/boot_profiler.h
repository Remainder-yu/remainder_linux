#ifndef __BOOT_PROFILER_H
#define __BOOT_PROFILER_H

#include <linux/types.h>
#include <linux/time.h>

#define MAX_BOOT_POINTS 128
#define BOOT_PROFILER_MAGIC 0x424F4F54 // "BOOT"

struct boot_point {
    const char* name;
    u64 tick;
};

struct boot_profiler {
    u32 magic;
    u32 count;
    u64 cntfrq;
    struct boot_point points[MAX_BOOT_POINTS];
    spinlock_t lock;
};

extern struct boot_profiler g_boot_profiler;
/*
// #define BOOT_PROFILE_POINT(_name)  \
// do {                                                                   \
//     unsigned long _fl;                                                 \
//     spin_lock_irqsave(&g_boot_profiler.lock, _fl);                     \
//     if (g_boot_profiler.count < MAX_BOOT_POINTS) {                     \
//         struct boot_point *_p = &g_boot_profiler.points[g_boot_profiler.count++]; \
//         _p->name = (_name);                                            \
//         _p->tick = read_sysreg_cntvct();                               \
//     }                                                                  \
//     spin_unlock_irqrestore(&g_boot_profiler.lock, _fl);                \
// } while (0)
// EXPORT_SYMBOL_GPL(BOOT_PROFILE_POINT);
*/

// 读取tick寄存器的值
static inline u64 read_sysreg_cntvct(void){
    u64 val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}
#endif