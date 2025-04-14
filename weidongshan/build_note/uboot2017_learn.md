# 学习目的：
- 学习uboot，了解主要功能及原理；
- 学习裸机程序，等价于功能强大的uboot程序，学习arm架构；
- 学习Make、config、kconfig等相关技巧

# 背景知识
## SPL
spl：Secondary Program Loader。二级加载。

BootRom：初始化系统，CPU的配置、关闭看门狗，初始化时钟，初始化一些外设。
BootRom：可以看做一级启动程序，而排在后面的就叫二级启动，这就是SPL名字的由来。

[SPL](https://blog.csdn.net/zhoutaopower/article/details/123133291)

uboot-- struct global_data类型变量的gd各成员含义
uboot启动代码中，函数board_init_f的一个功能就是配置变量gd的各个成员变量。首先获得DRAN得最高地址，然后每占用一块内存，往下减。

gd->relocaddr：uboot第一次运行的地址是0x87800000，之后需要为kernel腾空间，需要把uboot搬移到高地址处，此变量就是搬移后的uboot首地址
gd->new_gd：老变量gd是放在内部RAM，后面需要搬移到DRAM上，此变量是新gd的首地址
gd->start_addr_sp：栈首地址，栈是从高地址向低地址生长的
gd->reloc_off：uboot的新首地址 - 老首地址


# uboot编译

```shell
cd /home/book/100ask_imx6ull-sdk
book@100ask: ~/100ask_imx6ull-sdk$cd Uboot-2017.03
book@100ask: ~/100ask_imx6ull-sdk/Uboot-2017.03$ make distclean
book@100ask: ~/100ask_imx6ull-sdk/Uboot-2017.03$ make mx6ull_14x14_evk_defconfig
book@100ask: ~/100ask_imx6ull-sdk/Uboot-2017.03$ make

#!/bin/bash
# configs/mx6ul_14x14_evk_emmc_defconfig
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export PATH=$PATH:/home/yujuncheng/weidongshan/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CROSS_COMPILE=arm-linux-gnueabihf- distclean
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CROSS_COMPILE=arm-linux-gnueabihf- mx6ul_14x14_evk_defconfig
make -s ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j36
```


# uboot启动流程详解
要分析uboot的启动流程，首先要找到“入口”，找到第一行程序的位置。
程序的链接是由链接脚本决定的，所以通过链接脚本可以找到程序的入口。
最终会在根目录生成u-boot.lds文件，链接脚本。

在该文件的第三行：ENTRY(_start)为代码的当前入口点。
_start 在文件 arch/arm/lib/vectors.S 中有定义

```shell
.globl _start

/*
 *************************************************************************
 *
 * Vectors have their own section so linker script can map them easily
 *
 *************************************************************************
 */

	.section ".vectors", "ax"

/*
 *************************************************************************
 *
 * Exception vectors as described in ARM reference manuals
 *
 * Uses indirect branch to allow reaching handlers anywhere in memory.
 *
 *************************************************************************
 */

_start:

#ifdef CONFIG_SYS_DV_NOR_BOOT_CFG
	.word	CONFIG_SYS_DV_NOR_BOOT_CFG
#endif

	b	reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq
```
在vector.S中，_start的后面就是中断向量表，

u-boot.map

```shell

.text           0x0000000087800000    0x49fd0
 *(.__image_copy_start)
 .__image_copy_start
                0x0000000087800000        0x0 arch/arm/lib/built-in.o
                0x0000000087800000                __image_copy_start


```

分析save_boot_params：
```shell
	.globl	reset
	.globl	save_boot_params_ret
#ifdef CONFIG_ARMV7_LPAE
	.global	switch_to_hypervisor_ret
#endif

reset:
	/* Allow the board to save important registers */
	b	save_boot_params
save_boot_params_ret:
#ifdef CONFIG_ARMV7_LPAE
/*
 * check for Hypervisor support
 */
	mrc	p15, 0, r0, c0, c1, 1		@ read ID_PFR1
	and	r0, r0, #CPUID_ARM_VIRT_MASK	@ mask virtualization bits
	cmp	r0, #(1 << CPUID_ARM_VIRT_SHIFT)
```





问题1：为什么看到__image_copy_start 为 0X87800000，而.text 的起始地址也是0X87800000？
分析：为什么裸机程序的链接起始地址也是0X87800000。说明整个uboot的起始地址就是0X87800000。
##  lowlevel_init 函数详解

start.S (arch\arm\cpu\armv7)
reset:
  1. 设置CPSR：CPU为SVC模式，禁止IRQ/FIQ；
  2. 通过SCTLR和VBAR设置异常向量表的地址到_start；
  3. cpu_init_cp15: 失效TLB、L1 icache、BP数组；关MMU、dcache，开icache和分支预测；将CPU的
   variant + revision存于R2；
  4. cpu_init_crit: 调用lowlevel_init(arch/arm/cpu/armv7/lowlevel_init.S)：
     ① 关闭看门狗；
     ② 设置SRAM；
     ③ 禁止所有中断线，并设为IRQ及清除标志位；
     ④ 初始化uart的引脚；
     ⑤ 初始化tzpc为关闭；
  5. 跳到_main。

## _main -- crt0.S(arch/arm/lib)
### 核心流程
```c
//
//  sp 指针为 CONFIG_SYS_INIT_SP_ADDR,也就是 sp 指向 0X0091FF00
// 以下流程就是布局C语言运行环境
_main
	-> sp =(CONFIG_SYS_INIT_SP_ADDR)
	-> board_init_f_alloc_reserve //
	-> board_init_f_init_reserve //
	-> board_init_f

// 主要是留出早期的malloc内存区域和gd内存区域，其中：
// CONFIG_SYS_MALLOC_F_LEN=0X400( 在 文 件 include/generated/autoconf.h 中 定 义 )
// sizeof(struct global_data)=248(GD_SIZE 值),
// 返回值为新的top值，参考正点原子图32.2.4.1 内存分布图。
ulong board_init_f_alloc_reserve(ulong top)
{
	/* Reserve early malloc arena */
#if defined(CONFIG_SYS_MALLOC_F)
	top -= CONFIG_SYS_MALLOC_F_LEN;
#endif
	/* LAST : reserve GD (rounded up to a multiple of 16 bytes) */
	top = rounddown(top-sizeof(struct global_data), 16);

	return top;
}

// 将r0寄存器的值写到寄存器r9里面，因为r9寄存器存放着全局变量gd的地址，
	/* set up gd here, outside any C code */
	mov	r9, r0
	bl	board_init_f_init_reserve
// 从图 32.2.4.2 可以看出,uboot 中定义了一个指向 gd_t 的指针 gd,gd 存放在寄存器 r9 里面
// 的,因此 gd 是个全局变量。 gd_t 是个结构体,在 include/asm-generic/global_data.h 里面有定义,
```

board_init_f_init_reserve函数主要用于初始化gd，其实就是清零处理。
```c
void board_init_f_init_reserve(ulong base)
{
	struct global_data *gd_ptr;

	/*
	 * clear GD entirely and set it up.
	 * Use gd_ptr, as gd may not be properly set yet.
	 */

	gd_ptr = (struct global_data *)base;
	/* zero the area */
	memset(gd_ptr, '\0', sizeof(*gd));
	/* set GD unless architecture did it already */
#if !defined(CONFIG_ARM)
	arch_setup_gd(gd_ptr);
#endif
	/* next alloc will be higher by one GD plus 16-byte alignment */
	base += roundup(sizeof(struct global_data), 16);

	/*
	 * record early malloc arena start.
	 * Use gd as it is now properly set for all architectures.
	 */

#if defined(CONFIG_SYS_MALLOC_F)
	/* go down one 'early malloc arena' */
	// 为gd基地址 + gd大小：=0X0091FA00+248=0X0091FAF8，然后16字节对齐
	// ,最终 gd->malloc_base=0X0091FB00,这个也就是 early malloc 的起始地址,这就是堆区
	gd->malloc_base = base;
	/* next alloc will be higher by one 'early malloc arena' size */
	base += CONFIG_SYS_MALLOC_F_LEN;
#endif
}
```
在_main函数中，调用 board_init_f函数，此函数定义在文件 · common/board_f.c 中!主要用来初始化 DDR,定时器,完成代码拷贝等等,此函数我们后面在详细的分析。

然后重新设置环境(sp 和 gd)、获取 gd->start_addr_sp 的值赋给 sp,在函数 board_init_f
中会初始化 gd 的所有成员变量,其中 gd->start_addr_sp=0X9EF44E90, 所以这里相当于设置
sp=gd->start_addr_sp=0X9EF44E90。0X9EF44E90 是 DDR 中的地址,说明新的 sp 和 gd 将会存
放到 DDR 中,而不是内部的 RAM 了。


### 建立C语言运行环境的内存分布
_main早期做的事情，就是进行malloc，sp等赋值。此处应有到bootRom。


## board_init_f 函数详解

_main函数会调用board_init_f,该函数主要作用就是两个工作：
1. 初始化一系列外设，比如：串口、定时器或者打印一些信息；
2. 初始化gd的各个成员变量，uboot将自己重定位到DRAM（DDR）最后面的地址区域，也就是将自己拷贝到DRAM最后面的内存区域中。这么做的目的就是给linux腾出空间，方式linux覆盖掉uboot，将DRAM前面区域完成的空出来。

Q:在拷贝之前需要做些什么？
A:分配好内存位置和大小，比如gd应该存放到哪个位置，malloc内存池应该放在哪个位置等。这些信息都保存在 gd 的成员变量中,因此要对 gd 的这些成员变量做初始化。最终形成一个完整的内存“分配图”,在后面重定位 uboot 的时候就会用到这个内存“分配图”。

```c
void board_init_f(ulong boot_flags)
{
#ifdef CONFIG_SYS_GENERIC_GLOBAL_DATA
	/*
	 * For some architectures, global data is initialized and used before
	 * calling this function. The data should be preserved. For others,
	 * CONFIG_SYS_GENERIC_GLOBAL_DATA should be defined and use the stack
	 * here to host global data until relocation.
	 */
	gd_t data;

	gd = &data;

	/*
	 * Clear global data before it is accessed at debug print
	 * in initcall_run_list. Otherwise the debug print probably
	 * get the wrong value of gd->have_console.
	 */
	zero_global_data();
#endif

	gd->flags = boot_flags;
	gd->have_console = 0;

	if (initcall_run_list(init_sequence_f))
		hang();

#if !defined(CONFIG_ARM) && !defined(CONFIG_SANDBOX) && \
		!defined(CONFIG_EFI_APP) && !CONFIG_IS_ENABLED(X86_64)
	/* NOTREACHED - jump_to_copy() does not return */
	hang();
#endif
}

// common/board_f.c ：
static const init_fnc_t init_sequence_f[] = {
#ifdef CONFIG_SANDBOX
	setup_ram_buf,
#endif
	setup_mon_len,
#ifdef CONFIG_OF_CONTROL
	fdtdec_setup,
#endif
#ifdef CONFIG_TRACE
	trace_early_init,
#endif
	initf_malloc,
	initf_console_record,
#if defined(CONFIG_X86) && defined(CONFIG_HAVE_FSP)
	x86_fsp_init,
#endif
	arch_cpu_init,		/* basic arch cpu dependent setup */
	mach_cpu_init,		/* SoC/machine dependent CPU setup */
	initf_dm,
	arch_cpu_init_dm,
	mark_bootstage,		/* need timer, go after init dm */
#if defined(CONFIG_BOARD_EARLY_INIT_F)
	board_early_init_f,
	...
}
```


![alt text](image.png)


## relocate_code函数详解
函数主要用于代码拷贝,此函数定义在文件 arch/arm/lib/relocate.S 中。

调用：
```shell
	ldr	r9, [r9, #GD_BD]		/* r9 = gd->bd */
	sub	r9, r9, #GD_SIZE		/* new GD is below bd */

	adr	lr, here
	ldr	r0, [r9, #GD_RELOC_OFF]		/* r0 = gd->reloc_off */
	add	lr, lr, r0
#if defined(CONFIG_CPU_V7M)
	orr	lr, #1				/* As required by Thumb-only */
#endif
	ldr	r0, [r9, #GD_RELOCADDR]		/* r0 = gd->relocaddr */
	b	relocate_code

```

```shell

ENTRY(relocate_code)
	# r1 寄存器保存源地址，image_copy_start的地址为0x8780 0000
	ldr	r1, =__image_copy_start	/* r1 <- SRC &__image_copy_start */
	# r0=0x9FF4700，这个地址就是uboot拷贝的目标首地址。r4=r0-r1=0x9FF4700-0x87800000=0x18747000
	# 因此r4保存偏移量
	subs	r4, r0, r1		/* r4 <- relocation offset */
	beq	relocate_done		/* skip relocation */
	# r2中保存拷贝之前的代码结束地址，__image_copy_end =0x8785dd54
	ldr	r2, =__image_copy_end	/* r2 <- SRC &__image_copy_end */
	# 拷贝，从r1=__image_copy_start开始，读取uboot代码保存到r10和r11中，一次就只拷贝这2个32位的数据。
	# 拷贝完成以后r1的值会更新，保存下一个要拷贝的数据地址
copy_loop:
	ldmia	r1!, {r10-r11}		/* copy from source address [r1]    */
	# 将r10和r11的数据写到r0开始的地方，也就是目的地址。协议后r0的值会更新，保存下一个要写入的数据地址
	stmia	r0!, {r10-r11}		/* copy to   target address [r0]    */
	# 比较r1和r2是否相等，也就是检查是否拷贝完成，如果不相等说明没拷贝完成，继续执行copy_loop
	cmp	r1, r2			/* until source end address [r2]    */
	blo	copy_loop

	/*
	 * fix .rel.dyn relocations
	 */
	# 重定位.rel.dyn段，.rel.dyn.段时存放.text段中需要重定位地址的集合
	# 重定位是uboot将自身拷贝到DRAM的另一个地放去继续运行（DRAM的高地址处）
	ldr	r2, =__rel_dyn_start	/* r2 <- SRC &__rel_dyn_start */
	ldr	r3, =__rel_dyn_end	/* r3 <- SRC &__rel_dyn_end */
fixloop:
	ldmia	r2!, {r0-r1}		/* (r0,r1) <- (SRC location,fixup) */
	and	r1, r1, #0xff
	cmp	r1, #R_ARM_RELATIVE
	bne	fixnext

	/* relative fix: increase location by offset */
	add	r0, r0, r4
	ldr	r1, [r0]
	add	r1, r1, r4
	str	r1, [r0]
fixnext:
	cmp	r2, r3
	blo	fixloop

```
1. 将地址__image_copy_start至__image_copy_end的u-boot code重定位到地址gd->relocaddr
2. 通过.rel.dyn段确定uboot code中所有符号索引的内存地址，用重定位偏移校正符号索引的值

## relocate_vectors函数详解
 调用relocate_vectors()设置VBAR重定位异常向量表地址到gd->relocaddr；c_runtime_cpu_setup()
   （arch\arm\cpu\armv7\start.S）失效icache内容，数据同步内存屏障(DSB)，指令同步内存屏障(ISB)；执行
   memset(__bss_start,__bss_end,__bss_end-__bss_start)清零BSS段；
```shell
# arch/arm/lib/crt0.S
here:
/*
 * now relocate vectors
 */

	bl	relocate_vectors

# arch/arm/lib/relocate.S
ENTRY(relocate_vectors)

#ifdef CONFIG_CPU_V7M
	/*
	 * On ARMv7-M we only have to write the new vector address
	 * to VTOR register.
	 */
	ldr	r0, [r9, #GD_RELOCADDR]	/* r0 = gd->relocaddr */
	ldr	r1, =V7M_SCB_BASE
	str	r0, [r1, V7M_SCB_VTOR]
#else
#ifdef CONFIG_HAS_VBAR
	/*
	 * If the ARM processor has the security extensions,
	 * use VBAR to relocate the exception vectors.
	 */
	ldr	r0, [r9, #GD_RELOCADDR]	/* r0 = gd->relocaddr */
	mcr     p15, 0, r0, c12, c0, 0  /* Set VBAR */
#else
	/*
	 * Copy the relocated exception vectors to the
	 * correct address
	 * CP15 c1 V bit gives us the location of the vectors:
	 * 0x00000000 or 0xFFFF0000.
	 */
	ldr	r0, [r9, #GD_RELOCADDR]	/* r0 = gd->relocaddr */
	mrc	p15, 0, r2, c1, c0, 0	/* V bit (bit[13]) in CP15 c1 */
	ands	r2, r2, #(1 << 13)
	ldreq	r1, =0x00000000		/* If V=0 */
	ldrne	r1, =0xFFFF0000		/* If V=1 */
	ldmia	r0!, {r2-r8,r10}
	stmia	r1!, {r2-r8,r10}
	ldmia	r0!, {r2-r8,r10}
	stmia	r1!, {r2-r8,r10}
#endif
#endif
	bx	lr

ENDPROC(relocate_vectors)
```


## board_init_r 函数主要流程 -- 正式进入bootloader第二阶段


## run_main_loop函数详解
初始化hush解析器（CONFIG_HUSH_PARSER），用环境变量“bootdelay”或设备树config的属性bootdelay作为启动延迟时间，通过hush解析控制台输入的内容打断倒计时进入命名行；倒计时期间控制台无输入则执行环境变量或设备树/config节点的bootcmd，最后执行命令bootm address.

## bootm ${address} 启动linux内核
```shell
  1. do_bootm(...)执行该命令，作命令的解析；
  2. do_bootm_states(...)，如下内容：
  3. bootm_start()：环境变量verify决定后续是否对kernel镜像进行校验和检查，lmb(logical memory blocks)相关内
   容的初始化；
  4. bootm_find_os()：
   (1) boot_get_kernel()：获取kernel镜像格式为IMAGE_FORMAT_LEGACY，验证镜像hcrc，打印镜像的名字、类型、数
    据大小、加载地址和入口地址，验证dcrc(依赖env的verify)，判断arch是否支持；
   (2) 解析镜像的结果填充images.os的成员，kernel入口地址记到images.ep，镜像头部地址记到images.os.start；
  5. bootm_find_other()：
   (1) boot_get_ramdisk()：解析ramdisk镜像，bootm第三个参数为其地址(如bootm xxx yyy，yyy为对应地址)；
   (2) boot_get_fdt()：获取和解析设备树镜像内容，设备树镜像的起始地址需在bootm命令第四个参数指明，如
    bootm xxx yyy zzz，zzz为对应地址；
  6. bootm_load_os()：解压os数据或移动到images->os.load地址，所以kernel应有Load Address=Entry Point；
  7. boot_ramdisk_high()：重新定位并初始化ramdisk，需定义CONFIG_SYS_BOOT_RAMDISK_HIGH；
  8. bootm_os_get_boot_func(images->os.os)根据os类型获得启动函数boot_fn = do_bootm_linux；
  9. do_bootm_linux(BOOTM_STATE_OS_PREP, argc, argv, images)： boot_prep_linux()：若未指定传递给kernel的设
    备树地址，则建立各种tag到地址gd->bd->bi_boot_params；
  10. boot_selected_os()：通过函数指针boot_fn调用do_bootm_linux(BOOTM_STATE_OS_GO, ...)，进而调用
    boot_jump_linux(images, BOOTM_STATE_OS_GO)：
   (1) 通过gd->bd->bi_arch_number或者环境变量machid获得机器码；
   (2) announce_and_cleanup()：打印提示开始启动内核，注销驱动模型下设备驱动；调用cleanup_before_linux()：
    关L1/2 D-cache和MMU，冲刷掉dcache内数据；关I-cache，失效I-cache内条目，失效整个分支预测器阵列；
    执行数据和指令内存屏障，确保前面的操作完成；
   (3) kernel_entry(0, machid, r2)：参数r2传递启动参数（tag或设备树）的内存地址，正式跳转到kernel。
```
### do_bootm_linux函数
kernel_entry， 并不是uboot定义的，

```shell
kernel_entry = (void (*)(int, int, uint))images->ep;
# Linux 内核镜像文件的第一行代码就是函数 kernel_entry,而 images->ep 保存着 Linux
# 内核镜像的起始地址,起始地址保存的正是 Linux 内核第一行代码
```

# 参考文献
[uboot-代码启动流程](https://blog.csdn.net/weixin_39655765/article/details/80058644)
