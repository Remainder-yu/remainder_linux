
以串口0设备为例，设备名称为“uart-pl011",该设备的硬件中断是GIC-33，Linux内核分配的中断号IRQ是39，122表示已经发生了这么多次中断。
```cpp
benshushu:~# cat /proc/interrupts
           CPU0       CPU1       CPU2       CPU3
  3:       7807       6971       6448       7202     GICv3  27 Level     arch_timer
 36:       2649          0          0          0     GICv3  79 Edge      virtio0
 38:          0          0          0          0     GICv3 106 Edge      arm-smmu-v3-evtq
 41:          0          0          0          0     GICv3 109 Edge      arm-smmu-v3-gerror
 42:          0          0          0          0     GICv3  34 Level     rtc-pl031
 43:        122          0          0          0     GICv3  33 Level     uart-pl011
 44:          0          0          0          0     GICv3  23 Level     arm-pmu
 46:          0          0          0          0   ITS-MSI 16384 Edge      virtio1-config
 47:         19          0          0          0   ITS-MSI 16385 Edge      virtio1-input.0
 48:          1          0          0          0   ITS-MSI 16386 Edge      virtio1-output.0
 50:          0          0          0          0   ITS-MSI 32768 Edge      virtio2-config
 51:          6          0          0          0   ITS-MSI 32769 Edge      virtio2-requests
IPI0:      1900       1805       1805       1172       Rescheduling interrupts
IPI1:       261        210        238        276       Function call interrupts
IPI2:         0          0          0          0       CPU stop interrupts
IPI3:         0          0          0          0       CPU stop (for crash dump) interrupts
IPI4:         0          0          0          0       Timer broadcast interrupts
IPI5:         0          0          0          0       IRQ work interrupts
IPI6:         0          0          0          0       CPU wake-up interrupts

```

linux中断分层
1. 硬件层：如CPU与中断控制器的连接
2. 处理器架构管理层，如CPU中断异常处理
3. 中断控制器管理层，如IRQ号的映射
4. Linux内核通用中断处理层，如中断注册和中断处理

## 硬件中断号和linux中断号的映射
注册中断接口函数：request_irq\request_threaded_irq(),使用linux内核软件中断号（俗称软件中断号或IRQ号），而不是硬件中断号。

```cpp
int request_threaded_irq(unsigned int irq, irq_handler_t handler,
			 irq_handler_t thread_fn, unsigned long irqflags,
			 const char *devname, void *dev_id)
```

以串口为例：
```cpp
	pl011@9000000 {
		clock-names = "uartclk\0apb_pclk";
		interrupts = <0x00 0x01 0x04>;
		clocks = <0x8000 0x8000>;
		compatible = "arm,pl011\0arm,primecell";
		reg = <0x00 0x9000000 0x00 0x1000>;
	};

```

interrupts域描述的是3个属性：
1. 中断类型： PPI私有中断中断，该值为1，SPI共享外设中断。该值在设备树中为0；
2. 中断ID
3. 触发类型

linux内核有多种触发类型：
```cpp
include/linux/irq.h
enum {
	IRQ_TYPE_NONE		= 0x00000000,
	IRQ_TYPE_EDGE_RISING	= 0x00000001,
	IRQ_TYPE_EDGE_FALLING	= 0x00000002,
	IRQ_TYPE_EDGE_BOTH	= (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
	IRQ_TYPE_LEVEL_HIGH	= 0x00000004,
	IRQ_TYPE_LEVEL_LOW	= 0x00000008,
	IRQ_TYPE_LEVEL_MASK	= (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH),
	IRQ_TYPE_SENSE_MASK	= 0x0000000f,
	IRQ_TYPE_DEFAULT	= IRQ_TYPE_SENSE_MASK,

```

因此通过上述就可以知道这个串口设备的中断属性，它属于共享SPI类型，中断ID为1，中断触发类型为高电平触发.



# arm-gic

## GicV2总体架构

## Gic-V2中断类型和中断号分配
### 外设中断
SPIs
* 共享的外设中断，可以通过Distributor分发到指定PE的CPU接口
* 中断ID 32-1019
* 中断指向的目标CPU可以通过软件设置
PPIs
* PE专属的外设中断
* 中断ID 16-31 直接连接到PE的cpu接口部分
* 每个PE都有相同的专有PPIs，即目标CPU不需要编程确定
## 软件产生的中断 SGIs
* CPU产生的软件中断，用于核间中断
* 中断ID 0-15
* 通过Distributor转发到其他CPU
### 虚拟化使用的中断
* 虚拟中断
* 维护中断

## GIC-V2 Distributor
全局唯一
基于内存映射的寄存器方式访问GICD_接口寄存器
主要功能如下：
全局中断使能控制
每个中断的使能控制
每个中断的目标处理器设置
外设中断的触发模式设置（边沿或电平）
每个中断分组设置（Group0和1）
设置每个中断的可见性
外设中断pending状态的设置和清除

## GIC-V2 CPU接口
每CPU都有一套
基于内存映射的寄存器方式访问GICC_接口寄存器
主要功能：
cpu级别的中断使能
中断识别
中断处理结束标识
设置针对处理器的中断优先级掩码
定义中断在处理器上的抢占策略
确定在处理器上挂起的中断的最高优先级

## Gicv2中断处理流程：
中断状态
* Inactive 处于挂起或者未激活状态的中断
* Pengding 中断刚由硬件或者软件产生，等待目标处理器提供服务的状态
* Active 中断由目标处理器识别正在进行中断处理服务而还未完成的状态
* Active and Pending 处理器正在处理某一中断源的过程，该中断源产生的新的中断
状态转换图待补充（数据手册）：
A1-A2：外设或者软件产生中断
B1-B2：电平触发的中断被deasserted或者由软件改变了中断的Pending状态
C:边沿触发SPI,SGI,PPI被识别，或者软件从ICC_IAR0_EL1或者ICC_IAR1_EL1读取到INTID
D:电平触发的中断被识别后在处理器过程中又发生了新的中断
E1-E2:软件deactvies了中断

### 通用的中断处理流程
* GIC确定中断处于使能状态
* 对于每个处于pending态的中断，GIC确定其目标处理器或者处理器组
* 针对每个CPU接口，由Distributor转发该接口需要处理的最高优先级中断到接口
* 由每个处理器接口确定当前是否可以响应中断请求
* 目标处理器识别中断，GIC返回中断ID并更新中断的状态
* 目标处理器完成中断处理，发送中断结束EOI信号
* 1-N处理模型：当一个中断同时路由到多个PE时，该中断最多只能由一个PE处理，在该处理器识别中断之后，它同时清除掉该终端在其他处理上的Pengding状态。此时其他处理器读取GICC_IAR返回SpuriousID（1023）

### CICV2中断优先级
* arm GICv2实现16-256个优先级
* 优先级个数由具体处理器实现决定
* 优先级编码占用一个字节的高n位
* 数值越低，优先级越高
* 寄存器GIC_IPRIORITYRn
	* 写入该寄存器是否改变Active状态由实现决定
	* 寄存器偏移0x400+4*n
	* ID为m则n=m/4，字节便宜m%4,偏移0对应最低字节
	* 写入0xFF到GIC_IPRIORITYRn并读取返回值，可以获取实现定义的优先级编码位数

#### 优先级屏蔽掩码
用于在处理器接口上基于优先级范围屏蔽中断
CPU接口寄存器 GICC_PMR
INT Priority > GICC_PMR.Mask 则GIC可以转发中断的目标处理器

#### 优先级分组
CPU接口寄存器GICC_BRP，控制将优先级设置为两个字段
组优先级 （抢占级别）和子优先级字段
Bit[2:0]确定的优先级编码控制表

#### 优先级抢占
* Pendig中断抢占正在服务的Active中断
* 基于组优先级和优先级屏蔽掩码确定抢占条件
	* Pending优先级高于CPU接口的优先级屏蔽掩码的设置
	* Pending的组优先级高于CPU接口上的Active中断的组优先级
	* CPU接口的运行优先级 ：正在服务的最高优先级的active中断组优先级
	* 如果CPU接口上没有active中断，则最高优先级的Pending中断signal到该cpu，无需关注组优先级

### 中断分组

Group0和Group1两组
由GICD_IGROUPRn寄存器设置分组
由中断ID确定n和字节偏移
和中断优先级寄存器n和字节偏移的算法一致
各分组寄存器名如表（分组待补充）：
寄存器功能	       Group0 寄存器名	   Group1 寄存器名
优先级分组控制	        GICC_BPR	    GICC_ABPR
中断识别(Acknowledge)	GICC_IAR	      GICC_AIAR
中断结束（EOI)	        GICC_EOIR	  GICC_AEOIR
最高优先级的Pending中断	  GICC_HPPIR	GICC_AHPPIR

#### 支持中断分组时的中断识别
非安全扩展支持情况下
* 读取GICC_IAR识别group0中断
* 读取GICC_AIAR识别Group1中断
安全扩展支持情况下
* Secure读取GICC_IAR识别group0中断
* Non-secure读取GICC_AIAR识别Group1中断
#### 支持中断分组时的中断识别寄存器返回的特殊中断号
1020-1021 保留
* 1022 只在中断分组时才使用，表示有group1的中断需要signal到指定的处理器，满足以下所有条件返回该ID
	* 通过读取GICC_IAR寄存器识别中断
	* 挂起的最高优先级中断是Group1的中断
	* GICC_CTLR.AckCtl的设置为0
	* 挂起的中断的优先级满足能够signal到指定处理器的条件
#### 中断分组时的组优先级设置
部分系统实现中不允许Group0的中断抢占Group1的中断
Group0中的中断优先级使用所支持的优先级范围的低半部分
Group1中的中断优先级使用所支持的优先级范围的高半部分
Power up & Reset时的中断分组设置
所有的中断在Group0
FIQ中断请求被禁止
#### Group0和Group1优先级的软件视图
手册可以补充

Group1中断的抢占控制
非安全扩展支持情况下
GICC_CTRL.CBPR确定由哪个寄存器控制确定分组的中断优先级位数
CBPR=1，GICC_BPR同时控制Group0和Group1的抢占
CBRP=0，GICC_BPR控制Group0， GICC_ABPR控制Group1
安全扩展支持情况下
GICC_BPR总是用来确定Group0的组优先级位数
GICC_ABPR是一个只能在Secure模式下访问的寄存器，它是Non-secure模式下的GICC_CTRL寄存器的拷贝。

### 主要寄存及使用介绍

## GicV2虚拟化
### 虚拟化模型
虚拟化系统框架图（手册）

### 虚拟化概述
* 为虚拟机提供虚拟化扩展
* 提供虚拟CPU接口实现（虚拟机访问CPU接口不用陷出）
* Hypervisor处理所有的外部中断IRQs,按需注入到虚拟CPU
* 通过虚拟接口控制器GICH控制GIC虚拟CPU接口的行为，其中List register用于虚拟中断的注入
* GICH_寄存器由Hypervisor访问
* GICV_寄存器由虚拟CPU直接访问

### GIC虚拟CPU接口管理
#### 由Hypervisor完成虚拟机CPU接口的管理
* 使用List REGISTERs定义虚拟机CPU接口上挂起和激活的中断，当前虚拟CPU通过虚拟接口间接使用该寄存器
* 配置运行于虚拟CPU模式时Hypervisor可以捕获所有的中断按需进行处理和注入虚拟CPU
* 实现Distributor的软件模拟
  * 通过Stage-2的地址转换机制将虚拟CPU对于虚拟的Distributor地址空间的访问陷出到Hypervisor进行模拟
  * 确保虚拟机不能访问虚拟接口控制器GICH_
  * 重新映射GIC CPU接口寄存器的地址空间指向GIC虚拟CPU接口寄存器地址空间
  * 配置维护中断用于虚拟中断的EOI管理
### hypervisor中的中断处理--接收物理中断
当Hypervisor接收到物理中断时可以
	* 在hypervisor中完成处理，如对于由虚拟CPU接口产生的维护中断，则deactive对应的物理中断
	* 依据中断优先级和目标虚拟机产生虚拟中断，行为如下：
    	* 如果虚拟中断作用于当前虚拟机，则直接更新List register重新定义当前虚拟机可见的中断
    	* 如果虚拟中断不属于当前虚拟机，则更新对应虚拟机的list registers在hypervisor中的内存缓冲
    	* 虚拟机切换时通过将对应虚拟机的缓冲区的list registers寄存器缓冲更新到真实的List register寄存器来完成中断注入

### hypervisor中的中断处理--结束物理中断
虚拟化的物理中断结束处理
* Priority drop功能允许分两步deactive中断
	* 由GICC_CTLR.EOIModeNS 位控制
	* 该位设置为1时，写入GICC_EOIR寄存器只是降低CPU接口的运行优先级而不deactive中断，CPU接口的运行优先级降低后，就在该处理器上解除了对其他低优先级中断的响应阻塞
中断结束分为两步
1. EOI :hypervsior收到物理中断后，通过写入GICC_EOIR或者GICC_AEOIR寄存器执行EOI，中断EOI之后即使虚拟机未处理虚拟中断，hypervisor仍然可以接收新的中断（Drop Priority)
2. interrupt deactive:虚拟机处理了中断后通过写入GICV_EOIR或GICV_AEOIR deactive中断（包括虚拟中断和对应的物理中断）或者通过写入GICV_DIR寄存器deactive中断。

### 虚拟中断处理--系统设置要求
1. 为了确保系统正确处理虚拟中断，需要满足下述条件之一
* 所有Group0的中断优先级高于Group1的中断优先级
* GICV_CTLR.AckCtl=1
2. 同时针对GICV2推荐使用不同的寄存器管理Group0和Group1的中断
* 使用GICV_IAR,GICV_EOIR和GICV_HPPIR管理group0中断
* 使用GICV_AIAR,GICV_AEOIR和GICV_AHPPIR管理group1中断

### 虚拟中断处理--虚拟中断的识别

虚拟中断的识别--虚拟机通过读取GICV_IAR或者GICV_AIAR是识别虚拟中断，在如下条件下返回spurious INTID
* 未识别到中断
* 高优先级的中断已经在其他组中被识别

### 虚拟中断处理--虚拟中断结束
由GICV_CTLR.EOIMode控制结束模式
* 0-写入GICV_EOIR直接结束
* 1- 基于Priority drop机制两阶段结束
* 在两阶段模式下，GICH_LRn.HW确定虚拟中断是否和物理中断相关，从而确定deactive操作是否前向到物理的Distributor。
结束过程
* 在GICV_EOIR或者GICV_AEOIR寄存器中写入INTID和CPUID来完成对于虚拟中断的EOI操作
	* 当最高优先级中断是Group0中断时，写入GICV_IAR中读取的值到GICV_EOIR
		* 清除GICH_APR中的抢占位
		* 如果GICV_CTLR.EOIMode=0，清除对应中断的List register寄存器中的active状态
		* 如果GICV_CTLR.EOIMode=0且GICH_LRn.HW=1,需要在Distributor中deactive对应的物理中断
	* 当最高优先级是Goup1中断时，写入GICV_AIAR中读取的值到GICV_AEOIR
		* 清除GICH_APR中的抢占位
		* 如果GICV_CTLR.EOIMode=0，清除对应中断的List register寄存器中的active状态
		* 如果GICV_CTLR.EOIMode=0且GICH_LRn.HW=1,需要在Distributor中deactive对应的物理中断

### 虚拟中断处理----维护中断
由虚拟机处理完物理中断对应的虚拟中断后产生并由hypervisor进行处理
GIC虚拟化主机控制寄存器GICH_HCR.En=1才产生维护中断
电平触发
产生的时机：
	* Group0的虚拟中断的使能设置改变
	* Group1的虚拟中断的使能设置改变
	* 列表寄存器（List Registers）中无挂起的中断
    * 对应的中断在List registers中不存在，但是虚发起了至少一次EOI请求
	* 列表寄存器中的中断至少有一个收到了EOI请求
专门的维护中断状态寄存器GICH_MISR
Hyperversior借助于维护中断完成两阶段EOI的物理中断的deactive操作和缓存的虚拟中断注入到虚拟机的操作准备

### 虚拟中断处理----软件产生的虚拟中断
####  Hypervisor软件产生的中断
Hypervisor可以产生一个针对vCPU的虚拟中断而无需对应的物理中断支持
hypervisor可以控制虚拟机如何通过GICV_IAR或者GICV_AIAR寄存器来识别虚拟中断。该中断在虚拟机的呈现形式如下
* 针对SGI，在INTID的基础上附加一个CPUID字段
* 针对PPI和SPI，CPUID字段设置为0
#### Distributor产生的中断
中断可以单独deactive的机制不适用于SGIs
hypervisor必须在Distributor中使用上述的方法产生虚拟的SGI中断
hypervisor可以虚拟GICH_LRn.CPUID字段，该虚拟字段不需要和原始的SGI的CPUID值一致
在如下情况通过写入GICC_DIR寄存器deactive SGIs
* SGI的N-N处理模式
* 在支持虚拟设备时存在hypervisor产生的中断

### GIC虚拟化扩展的寄存器映射
GIC虚拟化扩展支持虚拟CPU接口
虚拟机访问虚拟CPU接口寄存器（GICV_)不会陷出
虚拟接口控制寄存器（GICH_)在hypervisor中访问，需要进行寄存器地址映射，两种映射模式
通过通用的基地址直接映射
所有处理器的GICH_寄存器区域使用一个统一的基地址
每个处理器通过该基地址访问自己的GIC虚拟接口控制寄存器内存区
通过CPUID在统一的地址映射区找到属于自己CPU的GICH_寄存器映射区
处理器专有的基地址映射
每个处理器都有自己的GICH_寄存器区的基地址
任何处理器既可以访问自己的GICH_寄存器区，也可以访问其他处理器的GICH_寄存器


### GIC虚拟化扩展的寄存器映射
补充示意图： Use of the processor-specific base addresses is shown in full only for accesses by processor 0
Figure 5-2 GIC virtual interface control register mappings

### 虚拟机接口控制寄存器--概述

#### 虚拟机接口控制寄存器---Hypervisor控制寄存器GICH_HCR
提供给hypervisor软件用于虚拟化接口的控制设置
32位寄存器
#### 虚拟机接口控制寄存器---虚拟中断支持信息
GICH_VTR
只读寄存器，vGIC信息
支持的虚拟中断的优先级的个数
可抢占的优先级个数
实现的List register的个数

#### 虚拟机接口控制寄存器---虚拟机控制
GICH_VMCR
用于hypervisor保存和恢复虚拟机的GIC状态

虚拟机接口控制寄存器---维护中断状态
GICH_MISR
hypervisor处理维护中断时获取维护中断状态信息

虚拟机接口控制寄存器---EOI状态
GICH_EIS0和GICH_EISR1
当产生维护中断时，标识哪一个列表寄存器中的中断产生了EOI请求
两个32位寄存器，每一位64个列表寄存器中的一个
hypervisor可以利用该寄存器的值在维护中断处理流程中完成对应的物理中断的deactive

虚拟机接口控制寄存器---列表寄存器使用状态
GICH_ELRS0和GICH_ELRS1
标识可用的列表寄存器位（列表寄存器空）
两个32位寄存器，每一位64个列表寄存器中的一个
hypervisor可以利用该寄存器的值找到可用的列表寄存器空字段用于中断注入

虚拟机接口控制寄存器---Active优先级跟踪
GICH_APR
当虚拟中断被识别后，hypervisor通过它获取虚拟CPU接口的优先级抢占level以及当前的active优先级
寄存器的返回值基于GICH_LRn.Priority中的设置，并且在GICH_LRn寄存器的最低位被EOI清除后
32位每一位代表一个中断优先级，bit0为0，bit31为31

虚拟机接口控制寄存器---中断注入控制
GICH_LRn
虚拟CPU接口虚拟中断注入和监控的寄存器
最多可以支持64个LR寄存器，每个寄存器可以用于一个虚拟中断注入和跟踪

### 虚拟机接口
虚拟CPU直接访问的接口寄存器
具有和物理CPU相似的格式
在虚拟机中访问GICC_*寄存器时实际上访问的是CPU虚拟接口寄存器GICV_*
虚拟中断总是由虚拟CPU通过虚拟接口寄存器处理
虚拟接口不需要电源管理功能，因此GICV_CTLR没有实现物理的GICC_CTLR中类似的IRQBypDisGrp1, FIQBypDisGrp1, IRQBypDisGrp0, and FIQBypDisGrp0位
需要在hypervisor中完成虚拟机中的GICC_*寄存器区接口地址（0x800001000）到实现定义的GICV内存地址之间的映射
