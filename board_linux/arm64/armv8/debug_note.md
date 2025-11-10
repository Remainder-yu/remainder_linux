## 调试环境准备
1. 安装`qemu-system-arm-pi4_4.2.50-3_with_GIC_amd64.deb`，及支持rpi4的qemu版本。然后ubuntu中安装交叉编译工具链。
2. aarch64-linux-gnu-gdb工具可以通过映射：`sudo ln -sf /usr/bin/gdb-multiarch /usr/local/bin/aarch64-linux-gnu-gdb`
3. 编译运行测试用例，可能需要根据实际环境修改Makefile。

## qemu+gdb的debug技巧

使用GDB与QEMU虚拟机调试BenOS，以下介绍如何搭建单步调试裸机程序。

### 在终端启动QEMU虚拟机的gdbserver
终端1中执行：
```shell
qemu-system-aarch64 -machine raspi4 -serial null -serial mon:stdio -nographic -kernel benos.bin -S -s
```
终端2中执行：
在另一个终端输入如下命令来启动 GDB,可以使用 aarch64-linux-gnu-gdb 命令或者 gdb-multiarch命令。
```shell
# 若gdb工具问题，请参考《调试环境准备》
$ aarch64-linux-gnu-gdb --tui build/benos.elf
```
在 GDB 的命令行中输入如下命令。
```shell
(gdb) target remote localhost:1234
(gdb) b _start
Breakpoint 1 at 0x0: file src/boot.S, line 7.
(gdb) c
```
gdb命令调试单独学习。


## qemu使用技巧

### 如何切换至qemu?
按 Ctrl+A C
→ 出现 (qemu) 提示符，此时你在 QEMU monitor
在 monitor 里可执行：
```shell
(qemu) info registers
(qemu) info mem
(qemu) xp /10wx 0x80080000
```
返回 Linux：
```shell
(qemu) c
```
终端立即切回 Linux shell，系统继续运行。
·
## gdb调试技巧

gdb调试命令对应表格（待补充）

## 参考
[树莓派4-qemu下载](https://chenlongos.com/raspi4-with-arceos-doc/chapter_1.1.html)
