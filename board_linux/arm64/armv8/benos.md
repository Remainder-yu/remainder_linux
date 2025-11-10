
交叉工具链：
/home/yujuncheng/learn_remainder/xvisor_learn/aarch64-linux-gnu/bin/aarch64-linux-gnu

```shell
yujuncheng@yujuncheng:~/learn_remainder/weidongshan/arm64_programming_practice$ git diff  chapter_2/lab01_hello_benos/BenOS/Makefile
diff --git a/chapter_2/lab01_hello_benos/BenOS/Makefile b/chapter_2/lab01_hello_benos/BenOS/Makefile
index f0f2a7d..acc4f7f 100644
--- a/chapter_2/lab01_hello_benos/BenOS/Makefile
+++ b/chapter_2/lab01_hello_benos/BenOS/Makefile
@@ -1,10 +1,10 @@
-ARMGNU ?= aarch64-linux-gnu
+ARMGNU = /home/yujuncheng/learn_remainder/xvisor_learn/aarch64-linux-gnu/bin/aarch64-linux-gnu

-board ?= rpi4
+board = rpi3

 ifeq ($(board), rpi3)
 COPS += -DCONFIG_BOARD_PI3B
-QEMU_FLAGS  += -machine raspi3
+QEMU_FLAGS  += -machine raspi3b
 else ifeq ($(board), rpi4)
 COPS += -DCONFIG_BOARD_PI4B
 QEMU_FLAGS  += -machine raspi4
```

# 《ARM64体系结构：编程与实践》例程学习
```shell
git clone https://gitee.com/benshushu/arm64_programming_practice.git
```
## 修改 makefile
```shell
[remainder@ubuntu lab12-1]$ git diff Makefile
diff --git a/chapter_12/lab12-1/Makefile b/chapter_12/lab12-1/Makefile
index 77736a4..921cf17 100644
--- a/chapter_12/lab12-1/Makefile
+++ b/chapter_12/lab12-1/Makefile
@@ -1,17 +1,19 @@
-ARMGNU ?= aarch64-linux-gnu
+ARMGNU = /home/yujuncheng/aarch64_gcc12.2.0_glibc2.36.0_fp/bin/aarch64-linux-gnu
+QEMU_PATH = /home/yujuncheng/qemu_path/
+board = rpi3

 board ?= rpi4

 ifeq ($(board), rpi3)
 COPS += -DCONFIG_BOARD_PI3B
-QEMU_FLAGS  += -machine raspi3
+QEMU_FLAGS  += -machine raspi3b
 else ifeq ($(board), rpi4)
 COPS += -DCONFIG_BOARD_PI4B
 QEMU_FLAGS  += -machine raspi4
 endif

 COPS += -g -Wall -nostdlib -Iinclude -mgeneral-regs-only
-ASMOPS = -g -Iinclude
+ASMOPS = -g -Iinclude

 BUILD_DIR = build
 SRC_DIR = src
@@ -19,7 +21,7 @@ SRC_DIR = src
 all : benos.bin

 clean :
-       rm -rf $(BUILD_DIR) *.bin *.elf *.map
+       rm -rf $(BUILD_DIR) *.bin *.elf *.map

 $(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
        mkdir -p $(@D)
@@ -44,6 +46,6 @@ benos.bin: $(SRC_DIR)/linker.ld $(OBJ_FILES)
 QEMU_FLAGS  += -nographic

 run:
-       qemu-system-aarch64 $(QEMU_FLAGS) -kernel benos.bin
+       $(QEMU_PATH)/qemu-system-aarch64 $(QEMU_FLAGS) -kernel benos.bin
 debug:
-       qemu-system-aarch64 $(QEMU_FLAGS) -kernel benos.bin -S -s
+       $(QEMU_PATH)/qemu-system-aarch64 $(QEMU_FLAGS) -kernel benos.bin -S -s
[remainder@ubuntu lab12-1]$
```

## GDB调试手段

```shell
make debug
/home/yujuncheng/aarch64_gcc12.2.0_glibc2.36.0_fp/bin/aarch64-linux-gnu-gdb --tui build/benos.elf
gdb --tui build/benos.elf

# GDB
target remote localhost:1234
b _start
c
```

![alt text](./image/qemu环境gdb调试.png)

## qemu+gdb调试

我们可以使用GDB+QEMU虚拟机单步调试裸机程序。

在终端启动QEMU虚拟机的gdbserver：make debug
在另一个终端输入如下命令来启动GDB，可以使用gdb-multiarch命令：
```shell
gdb-multiarch --tui ./benos.elf
# 在GDB的命令行中输入以下命令：
target remote localhost:1234
# 设置断点
b _start
# 单步进行调试
c
# 单步
s / step
# 不进入函数
n
# info reg
# bt

```

基础代码解析：
```shell
  1 SECTIONS
  2 {
  3         . = 0x80000,
  4         .text.boot : { *(.text.boot) }
  5         .text : { *(.text) }
  6         .rodata : { *(.rodata) }
  7         .data : { *(.data) }
  8         . = ALIGN(0x8);
  9         bss_begin = .;
 10         .bss : { *(.bss*) }
 11         bss_end = .;
 12 }
```

1、在第一行中，SECTIONS是LS（linker script）语法中的关键命令，用来描述输出文件的内存布局。SECTIONS命令告诉链接文件如何把输入文件的段映射到输出文件的各个段，如何将输入段整合为输出段，以及如何把输出段放入程序地址空间和进程地址空间。
在第3行中，`.`非常关键，他代表位置技术（Location Counter，LC），这里是把.text段的链接地址设备为0x80000，这里的链接地址指的是加载地址（Load address).
在第4行中，输出文件的.text.boot段内容由所有输入文件（其中的`*`可理解为所有.o文件，也就是二进制文件）的.text.boot段组成。

由上定义了如下几个段：
.text.boot段：启动首先要执行的代码
.text段：代码段
.rodata段：只读数据段
.data段：数据段
.bss段：包含未初始化的全局变量和静态变量

下面开始编写启动用的汇编代码，将代码保存为boot.S文件：

在汇编代码中主要完成三件事情：
1. 只让第一个CPU内核运行，让其他CPU内核进入死循环
2. 初始化.bss段
3. 设置栈，跳转到C语言入口

# 第3章 A64指令集1-加载与存储指令
## 3.1 A64指令集介绍
A64指令汇编需要注意的地方如下：
* [x] A64支持指令助记符和寄存器名全是大写字母或者全是小写字母。但是程序和数据标签是区分大小写的。
* [ ] 在使用立即操作数时前面可以使用`#`或者不使用`#`。
* [ ] `//`符号可以用于汇编代码的注释
* [ ] 通用寄存器前面使用`w`表示仅使用通用寄存器低32位，`x`表示64位通用寄存器。

A64指令集可以分成如下几类：
* [ ] 内存加载和存储指令
* [ ] 多字节内存加载和存储指令
* [ ] 算术和移位指令
* [ ] 移位操作指令
* [ ] 位操作指令
* [ ] 条件操作指令
* [ ] 跳转指令
* [ ] 独占访存指令
* [ ] 内存屏障指令
* [ ] 异常处理指令
