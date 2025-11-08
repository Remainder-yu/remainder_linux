#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include "boot_profiler.h"

#define DEV_NAME   "boot_profiler"
#define IOCTL_MAGIC 'B'
#define IOCTL_GET_NR_POINTS   _IOR(IOCTL_MAGIC, 1, u32)
#define IOCTL_GET_FREQ        _IOR(IOCTL_MAGIC, 2, u64)
#define IOCTL_GET_POINTS      _IOR(IOCTL_MAGIC, 3, struct boot_point_user *)

/* 用户空间版本：名字换成 64B 缓冲区，tick 保持 u64 */
struct boot_point_user {
    char name[64];
    u64  tick;
};

static dev_t          devt;
static struct cdev    cdev;
static struct class  *cls;

static long profiler_ioctl(struct file *filp, unsigned int cmd,
                           unsigned long arg)
{
    u32 n;
    u64 f;

    switch (cmd) {
    case IOCTL_GET_NR_POINTS:
        n = g_boot_profiler.count;
        return copy_to_user((u32 __user *)arg, &n, sizeof(n)) ? -EFAULT : 0;

    case IOCTL_GET_FREQ:
        f = g_boot_profiler.cntfrq;
        return copy_to_user((u64 __user *)arg, &f, sizeof(f)) ? -EFAULT : 0;

    case IOCTL_GET_POINTS: {
        struct boot_point_user __user *up = (struct boot_point_user __user *)arg;
        struct boot_point *kp;
        struct boot_point_user tmp;
        unsigned int i;

        for (i = 0; i < g_boot_profiler.count; i++) {
            kp = &g_boot_profiler.points[i];
            memset(&tmp, 0, sizeof(tmp));
            strncpy(tmp.name, kp->name, sizeof(tmp.name) - 1);
            tmp.tick = kp->tick;
            if (copy_to_user(&up[i], &tmp, sizeof(tmp)))
                return -EFAULT;
        }
        return 0;
    }
    default:
        return -ENOTTY;
    }
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = profiler_ioctl,
    .compat_ioctl   = profiler_ioctl,
};

static int __init boot_profiler_drv_init(void)
{
    int err;

    err = alloc_chrdev_region(&devt, 0, 1, DEV_NAME);
    if (err) return err;

    cdev_init(&cdev, &fops);
    cdev_add(&cdev, devt, 1);

    cls = class_create(THIS_MODULE, DEV_NAME);
    device_create(cls, NULL, devt, NULL, DEV_NAME);
    return 0;
}

static void __exit boot_profiler_drv_exit(void)
{
    device_destroy(cls, devt);
    class_destroy(cls);
    cdev_del(&cdev);
    unregister_chrdev_region(devt, 1);
}

module_init(boot_profiler_drv_init);
module_exit(boot_profiler_drv_exit);
MODULE_LICENSE("GPL");