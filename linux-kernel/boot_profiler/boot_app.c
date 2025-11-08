/*
 * 一次性拉取全部原始 tick，换算成绝对时间轴
 * gcc -o user_boot_time user_boot_time.c
 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

#define IOCTL_MAGIC 'B'
#define IOCTL_GET_NR_POINTS _IOR(IOCTL_MAGIC, 1, unsigned int)
#define IOCTL_GET_FREQ      _IOR(IOCTL_MAGIC, 2, unsigned long long)
#define IOCTL_GET_POINTS    _IOR(IOCTL_MAGIC, 3, struct boot_point_user *)

struct boot_point_user {
    char name[64];
    unsigned long long tick;
};

static inline unsigned long long tick_to_ns(unsigned long long tick,
                                            unsigned long long freq)
{
    return (tick * 1000000000ULL) / freq;
}

int main(void)
{
    int fd, i, n;
    unsigned long long freq, last_ns = 0;
    struct boot_point_user *pts;

    fd = open("/dev/boot_profiler", O_RDONLY);
    if (fd < 0) { perror("/dev/boot_profiler"); return 1; }

    ioctl(fd, IOCTL_GET_NR_POINTS, &n);
    ioctl(fd, IOCTL_GET_FREQ, &freq);
    pts = calloc(n, sizeof(*pts));
    if (!pts) { close(fd); return 1; }

    ioctl(fd, IOCTL_GET_POINTS, pts);

    printf("ARM64 Boot Timeline (cntfrq=%llu Hz)\n", freq);
    printf("%-50s %15s  %10s  %10s\n",
           "Name", "AbsTime(ns)", "Delta(ms)", "Cumul(ms)");
    printf("--------------------------------------------------------"
           "--------------------------\n");

    for (i = 0; i < n; i++) {
        unsigned long long abs_ns = tick_to_ns(pts[i].tick, freq);
        unsigned long long delta  = abs_ns - last_ns;
        last_ns = abs_ns;

        printf("%-50s %15llu  %7llu.%03llu  %7llu.%03llu\n",
               pts[i].name,
               abs_ns,
               delta / 1000000, (delta % 1000000) / 1000,
               abs_ns / 1000000, (abs_ns % 1000000) / 1000);
    }

    free(pts);
    close(fd);
    return 0;
}