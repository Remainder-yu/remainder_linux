# 1. xvisor设备树文件分析
arch/arm/dts/arm/foundation-v8-gicv2.dts


## 1.1. 内存配置功能
验证过程：
```bash
qemu-system-aarch64 -m 2048 -nographic -smp 4 -cpu cortex-a57 -machine virt,virtualization=on,gic-version=2 -kernel ./build/vmm.bin -initrd ./build/disk.img
```

启动过程：

```bash
XVisor# vfs run /boot.xscript
Created default shared memory
vmm_host_va2pa: == remainder  vmm_host_va2pa 269332480 
guest0: Created
guest0: Parsing /images/arm64/virt-v8/nor_flash.list
guest0: Loading 0x0000000000000000 with file ./firmware.bin
guest0: Loaded 0x0000000000000000 with 99616 bytes
guest0: Loading 0x00000000000FF000 with file ./cmdlist
guest0: Loaded 0x00000000000FF000 with 167 bytes
guest0: Loading 0x0000000000100000 with file ./Image
guest0: Loaded 0x0000000000100000 with 23067136 bytes
guest0: Loading 0x00000000017F0000 with file ./virt-v8.dtb
guest0: Loaded 0x00000000017F0000 with 2921 bytes
guest0: Loading 0x0000000001800000 with file ./rootfs.img
guest0: Loaded 0x0000000001800000 with 973806 bytes

XVisor# guest list            
-------------------------------------------------------------------------------
 ID     Name              Endianness    Device Path                            
-------------------------------------------------------------------------------
 0      guest0            little        /guests/guest0                         
-------------------------------------------------------------------------------
```

针对如何进行内存管理验证，其实就是修改设备树tests/arm64/virt-v8/virt-v8-guest.dts：
```bash
		nor_flash0 {
			manifest_type = "real";
			address_type = "memory";
			guest_physical_addr = <0x00000000>;
			physical_size = <0x04000000>; // 64M size 
			device_type = "alloced_rom";
			align_order = <21>; /* Align alloced memory to 2MB */
		};
```
如何进行该设备树的节点的加载，内存分配？然后又是如何释放？
guest destroy guest0之后，该设备节点也会被释放。


# 2. 内存管理模块

## 2.1. host ram物理内存管理

```bash
# 打印xvisor管理识别的内存信息
XVisor# host ram info
Frame Size        : 4096 (0x00001000)
Color Operations  : default
Color Order       : 16 (0x00000010)
Color Count       : 4294967295 (0xffffffff)
Bank Count        : 1 (0x00000001)
Total Free Frames : 504266 (0x0007b1ca)
Total Frame Count : 524288 (0x00080000)

# 每个bank分别表示出来：Bank00表示bank第00个
Bank00 Start      : 0x0000000040000000
Bank00 Size       : 0x0000000080000000
Bank00 Free Frames: 504266 (0x0007b1ca)
Bank00 Frame Count: 524288 (0x00080000)
```

主要数据结构分析：

```cpp
// 每个bank
struct vmm_host_ram_bank {
	physical_addr_t start;
	physical_size_t size;
	u32 frame_count;

	vmm_spinlock_t bmap_lock;
	unsigned long *bmap;
	u32 bmap_sz;
	u32 bmap_free;

	struct vmm_resource res;
};

// 总的ram内存管理控制数据结构，全局唯一rctrl
struct vmm_host_ram_ctrl {
	struct vmm_host_ram_color_ops *ops;
	void *ops_priv;
	u32 bank_count;
	struct vmm_host_ram_bank banks[CONFIG_MAX_RAM_BANK_COUNT];
};
static struct vmm_host_ram_ctrl rctrl;

/** Host RAM cache color operations */
struct vmm_host_ram_color_ops {
	char name[VMM_FIELD_NAME_SIZE];
	u32 (*num_colors)(void *priv);
	u32 (*color_order)(void *priv);
	bool (*color_match)(physical_addr_t pa, physical_size_t sz,
			    u32 color, void *priv);
};

/*
 * Resources are tree-like, allowing
 * nesting etc..
 */
struct vmm_resource {
	resource_size_t start;
	resource_size_t end;
	const char *name;
	unsigned long flags;
	struct vmm_resource *parent, *sibling, *child;
};
```

主要函数分析：

## 2.2. 内存管理模块的功能主要包括：
（1）内存空间的分配与回收。
（2）地址映射。
（3）内存空间保护。
（4）虚拟内存技术对内存进行扩充。

本模块的源码组成如下：
源文件	功能描述
vmm_shmem.c	共享内存
vmm_heap.c	堆管理
mempool.c	内存池管理
vmm_pagepool.c	页池管理
vmm_host_aspace.c	主机内存管理（内存分配、初始化、巨页）
vmm_host_vapool.c	虚拟内存管理
vmm_host_ram.c	物理内存管理
buddy.c	伙伴内存管理
memcpy.c	内存拷贝

### 2.2.1. 总体结构概述
整个分为3个层级，最上面的部分是对外公开的接口，中间是内存的基础组件的实现及内存初始化、分配等。最下面是ARCH实现部分。

内存管理模块作为操作系统重要模块之一，主要为系统解决管理和利用内存的功能。内存管理包含多个子模块，从功能类别上主要分为以下几类：

1、对其他模块或者对外提供基础操作，包含①、堆管理；②、内存池管理；③、页池管理；④、共享内存；
2、内存管理的基础操作，包含①、虚拟内存管理；②、物理内存管理；③、伙伴系统；④、主机内存管理；⑤、内存映射；
3、除以上模块之外，内存管理在实现上还依Cache color、基础组件、原子操作、页表映射等，每个模块的功能做一个描述：

### 2.2.2. 共享内存	
1、对外提供共享创建、查找、读、写等操作；
2、对shell提供创建、遍历、获取共享内存信息等操作；

#### 2.2.2.1. 共享内存函数分析
vmm_shmem_count	返回共享内存数量
vmm_shmem_create	创建一个共享内存
vmm_shmem_dref	
vmm_shmem_find_byname	通过名字查共享内存
vmm_shmem_init	初始化共享内存管理
vmm_shmem_iterate	遍历共享内存
vmm_shmem_read	往共享内存读数据
vmm_shmem_ref	
vmm_shmem_set	按照字节模式写数据
vmm_shmem_write	往共享内存写数据

### 2.2.3. 堆管理	
1、对外提供通用、DMA堆栈的分配接口，DMA堆与普通堆都是普通内存，只是在DMA堆会对普通的COHERENT进行关闭，这样做的目的是为了保存DMA设备与内存一致性的问题。
2、对shell提供查看堆信息，例如（堆基址、堆大小、剩余空间）

#### 2.2.3.1. 堆管理函数分析

vmm_alloc_size	计算指定地址正常堆栈大小
vmm_calloc	按照指定大小从堆栈中分配数组
vmm_dma_alloc_size	计算指定地址DMA堆栈大小
vmm_dma_free	释放DMA堆栈
vmm_dma_heap_free_size	获取DMA堆栈剩余空间
vmm_dma_heap_hksize	获取DMA堆栈结构体占用空间大小
vmm_dma_heap_init	DMA堆栈初始化
vmm_dma_heap_print_state	打印DMA堆栈状态
vmm_dma_heap_size	返回DMA堆栈大小包含管理结构体占用大小
vmm_dma_heap_start_va	返回DMA堆栈虚拟地址基址
vmm_dma_malloc	给DMA分配指定大小的堆栈内存
vmm_dma_map	
vmm_dma_pa2va	DMA堆栈物理地址转虚拟地址
vmm_dma_sync_for_cpu	
vmm_dma_sync_for_device	
vmm_dma_unmap	
vmm_dma_va2pa	DMA堆栈虚拟地址转物理地址
vmm_dma_zalloc	给DMA分配指定大小的堆栈内存并清零
vmm_dma_zalloc_phy	给DMA分配指定大小的堆栈内存、清零返回物理地址
vmm_free	释放空间
vmm_heap_init	堆栈初始化
vmm_is_dma	判断栈类型是否为dma
vmm_malloc	按照指定大小从堆栈中分配空间
vmm_normal_heap_free_size	分配DMA内存并返回物理地址
vmm_normal_heap_hksize	返回通用堆栈的管理结构体大小
vmm_normal_heap_print_state	打印通用堆栈的状态
vmm_normal_heap_size	返回通用堆栈大小包含管理结构体占用大小
vmm_normal_heap_start_va	返回通用堆栈虚拟地址基址
vmm_strdup	
vmm_zalloc	分配指定大小的堆栈内存并清零

### 2.2.4. 内存池管理	
1、内存池主要是通过FIFO的功能来减少由于频繁申请和释放带来的系统开销提高效率；
2、对外提供内存池创建、分配、释放；
3、内存池提供三种类型方式申请内存池①、堆的方式；②、物理内存方式；③、通用内存方式

#### 2.2.4.1. 内存池管理函数分析	
memcpy	内存拷贝
mempool_check_ptr	
mempool_destroy	内存池销毁
mempool_free	内存池释放
mempool_free_entities	获取内存池中可用数量
mempool_get_type	获取内存池类型（RAM、HEAP、DEV）
mempool_heap_create	创建一个HEAP类型的内存池
mempool_malloc	从内存中分配一个空间
mempool_ram_create	创建一个RAM类型的内存池
mempool_raw_create	创建一个DEV类型的内存池
mempool_total_entities	
mempool_zalloc	从内存中分配一个空间并初始化

### 2.2.5. 页池管理	
1、页池适用于连续大物理内存分配的场景；
2、对外提供内存池的分配、释放操作；
3、对shell提供基础的查询，例如页池的总大小、剩余空间等；
#### 2.2.5.1. 页池管理函数分析	
vmm_pagepool_alloc	从页池中分配指定类型和大小的页
vmm_pagepool_entry_count	返回指定类型页个数
vmm_pagepool_free	从页池中释放指定类型和大小的页
vmm_pagepool_hugepage_count	返回巨页总数（和直接返回页没有区别）
vmm_pagepool_init	页池初始化
vmm_pagepool_name	根据类型获取页池名称
vmm_pagepool_page_avail_count	页池中剩余可用页
vmm_pagepool_page_count	返回页总数（和返回巨页没有区别）
vmm_pagepool_space	返回页池总大小

### 2.2.6. 主机内存管理	
1、主机内存初始化；
2、提供基本内存映射、分配及巨页分配的操作；
3、组件为shell提供查看映射信息，RAM信息等；

### 2.2.7. 虚拟内存管理	
1、为xvisor 提供虚拟地址管理功能；
2、为shell提供虚拟地址查询功能；

#### 2.2.7.1. 虚拟内存管理函数分析	
vmm_host_vapool_alloc	分配虚拟内存
vmm_host_vapool_base	获取虚拟内存基址
vmm_host_vapool_estimate_hksize	
vmm_host_vapool_find	根据虚拟地址查找已分配的空间及大小
vmm_host_vapool_free	根据虚拟地址及大小释放空间
vmm_host_vapool_free_page_count	虚拟地址空闲页数
vmm_host_vapool_init	虚拟内存初始化
vmm_host_vapool_isvalid	判断给的虚拟地址是否有效
vmm_host_vapool_page_isfree	检查虚拟地址是否可用
vmm_host_vapool_print_state	打印虚拟地址空间状态
vmm_host_vapool_reserve	保留虚拟地址空间
vmm_host_vapool_size	虚拟地址池总大小
vmm_host_vapool_total_page_count	虚拟地址池的总页数

### 2.2.8. 物理内存管理	
1、为host、guest提供分配、释放物理内存的基本操作
3、为shell 提供查询物理内存信息的基本功能；

物理内存管理：

#### 2.2.8.1. 物理内存管理函数分析
vmm_host_ram_alloc	分配物理内存
vmm_host_ram_bank_count	获取物理内存设备数量
vmm_host_ram_bank_frame_count	返回frame数量
vmm_host_ram_bank_free_frames	返回可用frame数量
vmm_host_ram_bank_size	返回指定内存设备的大小
vmm_host_ram_bank_start	返回指定内存设备的基址
vmm_host_ram_color_alloc	从RAM中分配带有缓存颜色的物理空间
vmm_host_ram_color_count	从RAM中获取Cache color数量
vmm_host_ram_color_ops_name	从RAM中获取Cache color名字
vmm_host_ram_color_order	
vmm_host_ram_end			返回指定内存设备的结束地址
vmm_host_ram_estimate_hksize	返回ram管理结构体大小
vmm_host_ram_frame_isfree	判断指定的地址是否空闲
vmm_host_ram_free	释放指定的地址空间
vmm_host_ram_init	初始化ram管理器
vmm_host_ram_reserve	保留空间（不被分配）

### 2.2.9. 内存映射	
1、提供内存映射，建立页表功能。

### 2.2.10. 伙伴系统
1、给heap和vapool提供基础的分配、释放、查找算法；
     
#### 2.2.10.1. 伙伴系统函数分析	
buddy_allocator_init	初始化伙伴分配器
buddy_bins_area_count	获取指定块AREA数量
buddy_bins_block_count	获取指定块数量
buddy_bins_free_space	根据块返回可用空间
buddy_hk_area_free	获取可用伙伴管理结构大小
buddy_hk_area_total	获取伙伴管理结构大小
buddy_mem_aligned_alloc	
buddy_mem_alloc	从伙伴系统分配空间
buddy_mem_find	从伙伴系统中查找已分配的空间
buddy_mem_free	从伙伴系统释放空间
buddy_mem_partial_free	从伙伴系统释放部分空间
buddy_mem_reserve	从伙伴系统保留空间




## 2.3. 调用关系：
```cpp
static int __init host_aspace_init_primary(void)
    -> arch_devtree_ram_bank_setup(); // Setup RAM banks
        -> 

```

# 内存分配释放研究

基于主机内存分配全部的物理内存为什么会系统crash？

## vapool内存池

vapool分配的大小需要与物理内存映射，所以分配物理内存，实际对应虚拟内存，而本身就VAPOOL House-Keeping Size 用于管理vapool。
```cpp
virtual_size_t __init vmm_host_vapool_estimate_hksize(virtual_size_t size)
{
	/* VAPOOL House-Keeping Size = (Total VAPOOL Size / 256);
	 * 12MB VAPOOL   => 48KB House-Keeping
	 * 16MB VAPOOL   => 64KB House-Keeping
	 * 32MB VAPOOL   => 128KB House-Keeping
	 * 64MB VAPOOL   => 256KB House-Keeping
	 * 128MB VAPOOL  => 512KB House-Keeping
	 * 256MB VAPOOL  => 1024KB House-Keeping
	 * 512MB VAPOOL  => 2048KB House-Keeping
	 * 1024MB VAPOOL => 4096KB House-Keeping
	 * ..... and so on .....
	 */
	return size >> 8;
}
```
存在的问题：
Q1：分配内存时按照所有空闲大小进行分配，就会出现分配物理内存成功，然后进行host_memap映射错误？
分析：主要原因是，分配全部物理内存，就会导致映射虚拟内存池本身需要管理空间，所以xvisor会崩，而分配内存的大小应该按照vapool内存池允许分配上限作为标准，而不是实际物理内存空闲大小。