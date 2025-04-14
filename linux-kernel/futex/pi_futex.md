# linux -pi_futex

对应源文件：kernel/futex.c

## 在ubuntustrace ./pi_test 
```bash
yujuncheng@yujuncheng:~/learn_remainder/linux_learn/remainder_learn_kernel/linux-kernel/IPC$ strace ./pi_test 
execve("./pi_test", ["./pi_test"], 0x7ffe5e0f8120 /* 33 vars */) = 0
brk(NULL)                               = 0x5c9826a3f000
arch_prctl(0x3001 /* ARCH_??? */, 0x7fff2f7dc510) = -1 EINVAL (Invalid argument)
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x718bfe597000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
newfstatat(3, "", {st_mode=S_IFREG|0644, st_size=70335, ...}, AT_EMPTY_PATH) = 0
mmap(NULL, 70335, PROT_READ, MAP_PRIVATE, 3, 0) = 0x718bfe585000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0P\237\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
pread64(3, "\4\0\0\0 \0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0"..., 48, 848) = 48
pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0I\17\357\204\3$\f\221\2039x\324\224\323\236S"..., 68, 896) = 68
newfstatat(3, "", {st_mode=S_IFREG|0755, st_size=2220400, ...}, AT_EMPTY_PATH) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
mmap(NULL, 2264656, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x718bfe200000
mprotect(0x718bfe228000, 2023424, PROT_NONE) = 0
mmap(0x718bfe228000, 1658880, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x718bfe228000
mmap(0x718bfe3bd000, 360448, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1bd000) = 0x718bfe3bd000
mmap(0x718bfe416000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x215000) = 0x718bfe416000
mmap(0x718bfe41c000, 52816, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x718bfe41c000
close(3)                                = 0
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x718bfe582000
arch_prctl(ARCH_SET_FS, 0x718bfe582740) = 0
set_tid_address(0x718bfe582a10)         = 254184
set_robust_list(0x718bfe582a20, 24)     = 0
rseq(0x718bfe5830e0, 0x20, 0, 0x53053053) = 0
mprotect(0x718bfe416000, 16384, PROT_READ) = 0
mprotect(0x5c98261a7000, 4096, PROT_READ) = 0
mprotect(0x718bfe5d1000, 8192, PROT_READ) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
munmap(0x718bfe585000, 70335)           = 0
futex(0x7fff2f7dc554, FUTEX_UNLOCK_PI_PRIVATE) = -1 EPERM (Operation not permitted)
rt_sigaction(SIGRT_1, {sa_handler=0x718bfe291870, sa_mask=[], sa_flags=SA_RESTORER|SA_ONSTACK|SA_RESTART|SA_SIGINFO, sa_restorer=0x718bfe242520}, NULL, 8) = 0
rt_sigprocmask(SIG_UNBLOCK, [RTMIN RT_1], NULL, 8) = 0
mmap(NULL, 8392704, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0) = 0x718bfd9ff000
mprotect(0x718bfda00000, 8388608, PROT_READ|PROT_WRITE) = 0
getrandom("\x9f\xce\x79\x39\x51\x7b\xda\x25", 8, GRND_NONBLOCK) = 8
brk(NULL)                               = 0x5c9826a3f000
brk(0x5c9826a60000)                     = 0x5c9826a60000
rt_sigprocmask(SIG_BLOCK, ~[], [], 8)   = 0
clone3({flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, child_tid=0x718bfe1ff910, parent_tid=0x718bfe1ff910, exit_signal=0, stack=0x718bfd9ff000, stack_size=0x7fff00, tls=0x718bfe1ff640} => {parent_tid=[254185]}, 88) = 254185
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
mmap(NULL, 8392704, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0) = 0x718bfd1fe000
mprotect(0x718bfd1ff000, 8388608, PROT_READ|PROT_WRITE) = 0
rt_sigprocmask(SIG_BLOCK, ~[], [], 8)   = 0
clone3({flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, child_tid=0x718bfd9fe910, parent_tid=0x718bfd9fe910, exit_signal=0, stack=0x718bfd1fe000, stack_size=0x7fff00, tls=0x718bfd9fe640} => {parent_tid=[254186]}, 88) = 254186
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
clock_nanosleep(CLOCK_REALTIME, 0, {tv_sec=1, tv_nsec=0}, 0x7fff2f7dc550) = 0
futex(0x718bfe1ff910, FUTEX_WAIT_BITSET|FUTEX_CLOCK_REALTIME, 254185, NULL, FUTEX_BITSET_MATCH_ANY) = -1 EAGAIN (Resource temporarily unavailable)
newfstatat(1, "", {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}, AT_EMPTY_PATH) = 0
write(1, "Test executed successfully only "..., 41Test executed successfully only mutex_m.
) = 41
write(1, " 406447 mutex lock and unlock we"..., 41 406447 mutex lock and unlock were done.
) = 41
exit_group(0)                           = ?
+++ exited with 0 +++
```


linux-pi-futex分析

PI-futex操作的特殊规则：
未获取锁时，futex字的值应为0。
获取锁时，futex字的值应为拥有线程的线程ID（TID）。
如果锁被拥有且有线程正在竞争锁，则在futex字的值中设置FUTEX_WAITERS位；即值是FUTEX_WAITERS | TID。

用户空间操作：
