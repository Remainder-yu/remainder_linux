# 1. 物理内存初始化--伙伴系统前夕

本文主要内容：
介绍memblock建立过程及分页机制初始化，主要步骤包括：
* setup_machine_fdt:解析设备树，收集内存信息及bootargs
  * hossen node
* early_fixmap_init:对保留的fixmap区域创建映射
* early_ioremap_init:初始化early_ioremap机制
* arm64_memblock_init:初始化memblock机制
* paging_init:初始化内核页表，内存节点、内存域及页帧page，此函数功能较为复杂
* request_standard_resources：将memblock.memory挂载到iomem_resource资源树下
* early_standard_resources:将memblock，memory挂载到iomem_resource资源树下
* early_ioremap_reset: 结束early_ioremap机制
* unflatten_device_tree: dtb转换为device_node tree
* 根据device node tree初始化CPU，psci

## 1.1. linux内存管理--概述

linux内存管理主要分为3个阶段：
1. mmu未打开，汇编时代
2. fixmap\memblock时代：此时伙伴系统还没有成形，一直到mm_init函数中的mem_init将空闲内存加载到zone中。
3. 伙伴系统建立

### 1.1.1. start_kernel

start_kernel中内存管理相关的系统初始化函数主要如下：
```shell
—->setup_arch: 体系机构的设置函数，还负责初始化自举分配器

—->setup_per_cpu_areas: 定义per-cpu变量,为各个cpu分别创建一份这些变量副本

—->build_all_zonelists: 建立节点(node)和内存域(zone)的数据结构

—->mem_init: 停用bootmem分配器并迁移到实际的内存管理函数

—->kmem_cache_init: 初始化内核内部用于小块内存区的分配器

—->setup_per_cpu_pageset: 为各个cpu的zone的pageset数组的第一个数组元素分配内存
```

本篇笔记主要分析setup_arch:

```shell
—->machine_specific_memory_setup: 创建一个列表，包括系统占据的内存区和空闲内存区

—->parse_early_param: 解析dtb树命令行

—->setup_memory: 确定每个节点可用的物理内存也数目，初始化bootmem分配器，分配各种内存区(目前是否还应用？已经被memblock替代)

—->paging_init: 初始化内核页表并启动内存分页
	---->pagetable_init: 确保直接映射到内核地址空间的物理内存被初始化

—->zone_size_init:初始化系统中所有节点的pgdat_t实例
 ---->add_active_range: 对可用的物理内存建立一个相对简单的列表
 ---->free_area_init_nodes: 建立完备的内核数据结构

```



## 1.2. 设备树解析，收集内存信息
内核启动过程中，需要解析这些DTS文件，实现代码如下：
```c
/*
__fdt_pointer: fdt phy, 在__mmap_switched中被赋值
setup_machine_fdt: 校验fdt合法性，并通过下面3个回调函数扫描fdt中预留的memory及传入内核参数
1. 扫描device tree函数：of_scan_flat_dt
2. 3个回调函数：
early_init_dt_scan_chosen： 解析chosen信息，其中一般包括initrd、bootargs，保留命令行参数到boot_command_line全局变量中
early_init_dt_scan_root: 从fdt中初始化{size, address}结构信息，保存到dt_root_addr_cells,dt_root_size_cells中
early_init_dt_scan_memory：扫描fdt中memory区域，寻找device_type="memory"的节点，并通过以下函数将该区域添加到memblock管理系统
	early_init_dt_add_memory_arch
		memblock_add：将region添加到memblock.memory中
*/
--> setup_arch
--> setup_machine_fdt
void __init early_init_dt_scan_nodes(void)
--> of_scan_flat_dt(early_init_dt_scan_memory, NULL);
    --> 如下：

// drivers/of/fdt.c
// 该函数就是解析memory描述信息：获得内存的base和size，
int __init early_init_dt_scan_memory(unsigned long node, const char *uname,
				     int depth, void *data)
--> early_init_dt_add_memory_arch(base, size);
    --> memblock_add(base, size); // 通过调用添加到memblock子系统中

int __init_memblock memblock_add(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("memblock_add: [%pa-%pa] %pF\n",
		     &base, &end, (void *)_RET_IP_);

	return memblock_add_range(&memblock.memory, base, size, MAX_NUMNODES, 0);
}
```



## 1.3. 物理内存初始化
参考文献：
[linux物理内存初始化](https://www.cnblogs.com/LoyenWang/p/11440957.html)
奔跑吧，linux内核
linux内核深度解析--余华兵

在内核启动时，内核知道DDR物理内存的大小。
1. 系统是怎么知道物理内存的？
2. 在内存管理真正初始化之前，内核的代码执行需要分配的内存该怎么处理？
3.

分析：
```shell
memory@80000000 {
		device_type = "memory";
		reg = <0x00000000 0x80000000 0 0x80000000>;
		      /* DRAM space - 1, size : 2 GB DRAM */
	};

```
dts文件的同学应该见过memory的节点，这个节点描述了内存的起始地址及大小，事实上内核在解析dtb文件时会去读取该memory节点的内容，从而将检测到内存注册进系统。

Uboot会将kernel image和dtb拷贝至内存中，并且将dtb物理地址告知kernel，kernel需要从该物理地址上读取到dtb文件解析，才能得到最终的内存信息，dtb的物理地址需要映射到虚拟地址上才能访问，但是这个时候paging_init还没有调用，也就是说物理地址的映射还没有完成。此时，使用什么机制实现呢？---fixed map
在物理内存添加进系统后，在mm_init之前，系统会使用memblock模块对内存进行管理。

### 1.3.1. memblock分配器

#### 1.3.1.1. 数据结构

```cpp
/**
 * struct memblock - memblock allocator metadata
 * @bottom_up: is bottom up direction?
 * @current_limit: physical address of the current allocation limit
 * @memory: usabe memory regions
 * @reserved: reserved memory regions
 * @physmem: all physical memory
 */
struct memblock {
	bool bottom_up;  /* is bottom up direction? */ // 表示分配方式，值为true：表示从地址向上分配
	phys_addr_t current_limit; // 成员current_limit是可分配内存的最大物理地址
	struct memblock_type memory; // 内存类型，包括已分配内存和未分配内存
	struct memblock_type reserved; // reserved 是预留类型（已分配内存）
#ifdef CONFIG_HAVE_MEMBLOCK_PHYS_MAP
	struct memblock_type physmem; // physmem 是物理内存类型，内存类型是物理内存类型的子集。通过配置参数可以指定可用内存大小，内核看不到所有内存
#endif
};
// 物理内存类型总是看不到所有内存范围
```

内存块类型结构体：
```cpp
struct memblock_type {
	unsigned long cnt;// 区域数量
	unsigned long max; // 已分配数组的大小
	phys_addr_t total_size; // 所有区域的长度
	struct memblock_region *regions;
	char *name;
};

// 内存块类型使用数组存放内存块区域，成员regions指向内存区域数组，cnt是内存块区域的数量，max是数组的元素个数，total_size是所有内存区域的长度，name是内存块类型名称
```
内存块区域的数据结构

```cpp

struct memblock_region {
	phys_addr_t base;
	phys_addr_t size;
	enum memblock_flags flags;
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
	int nid;
#endif
};

enum memblock_flags {
	MEMBLOCK_NONE		= 0x0,	/* No special request */
	MEMBLOCK_HOTPLUG	= 0x1,	/* hotpluggable region */ // 支持热插拔的区域
	MEMBLOCK_MIRROR		= 0x2,	/* mirrored region */
	MEMBLOCK_NOMAP		= 0x4,	/* don't add to kernel direct mapping */
};
```
成员base是起始物理地址，size是长度，nid是节点编号。成员flags是标志，可以是以上枚举结构体内的值。

#### 1.3.1.2. memblock主要接口及算法

```shell
memblock_add：添加新的内存块区域到memblock.memory中。
memblock_remove:删除内存块区域
memblock_alloc:分配内存
memblock_free:释放内存
```
为了兼容 bootmem 分配器, memblock 分配器也实现 了 bootmem 分配器提供的接口 。如果开启 配置宏 CONFIG _NO_BOOTMEM, memblock 分配器就完全替 代了 bootrnem 分配器 。

memblock分配器把所有内存添加到memblock.memory中，把分配出去的内存块添加至memblock.reserved中。内存块类型中的内存块域数组按起始物理地址从小到大排序。
memblock_alloc负责分配内存，把主要工作委托给函数memblock_alloc_range_nid，算法如下：
（1）调用函数memblock_find_in_range_node以找到没有分配的内存块区域，默认从高地址向下分配
（2）调用函数memblock_reserve，把分配出去的内存块区域添加到memblock.reserved中

函数memblock_free负责释放内存，只需要把内存块区域从memblock.reserved中删除。


#### 1.3.1.3. 初始化

在/mm/memblock.c定义了全局变量memblock，把成员bottom_up初始化为假，表示从高地址向下分配。

```cpp
struct memblock memblock __initdata_memblock = {
	.memory.regions		= memblock_memory_init_regions,
	.memory.cnt		= 1,	/* empty dummy entry */
	.memory.max		= INIT_MEMBLOCK_REGIONS,
	.memory.name		= "memory",

	.reserved.regions	= memblock_reserved_init_regions,
	.reserved.cnt		= 1,	/* empty dummy entry */
	.reserved.max		= INIT_MEMBLOCK_RESERVED_REGIONS,
	.reserved.name		= "reserved",

#ifdef CONFIG_HAVE_MEMBLOCK_PHYS_MAP
	.physmem.regions	= memblock_physmem_init_regions,
	.physmem.cnt		= 1,	/* empty dummy entry */
	.physmem.max		= INIT_PHYSMEM_REGIONS,
	.physmem.name		= "physmem",
#endif

	.bottom_up		= false,
	.current_limit		= MEMBLOCK_ALLOC_ANYWHERE,
};

```
ARM64内核初始化memblock分配器的过程是：
（1）解析设备树二进制文件中的节点`/memory`，把所有物理内存添加到memblock.memory。
（2）在函数arm64_memblock_init的主要代码如下：

```cpp

```

当物理内存都添加进系统之后，arm64_memblock_init会对整个物理内存进行整理，主要工作就是将一些特殊的区域添加进reserved内存中。
##### 1.3.1.3.1. 在arm64_memblock_init接口中主要执行动作：
1. 调用fdt_enforce_memory_region解析设备树二进制文件中节点`/chosen`的属性“linux,usable-memory-range”,得到可用内存范围，超出这个范围的物理内存范围从memblock.memory中删除。
2. 把线性映射区域不能覆盖的物理内存范围从memblock.memory中删除。
3. 设备树中节点`/chosen`属性`bootargs`指定的命令行中，可以使用参数`mem`指定可用内存大小。如果指定了内存的大小，name把超过可用长度的物理地址范围从memblock.memory中删除。因为内核镜像可以加载到内存的高地址部分，并且内核镜像必须是可以通过线性映射区域访问的，所以需要把内核镜像占用的物理内存重新添加到memblock.memory中。
4. 把内核镜像占用的物理内存范围添加到memblock.reserved中
5. 从设备树二进制文件中的内存区域（memory reserve map，对应设备树源文件的字段“memreserve/”）和节点/reserved-memory读取保留的物理内存范围，再添加到memblock.reserved中。


##### 1.3.1.3.2. 解析设备树流程：

核心调用流程：
```cpp
start_kernel
-> setup_arch
	-> early_fixmap_init  // fixmap区域页表映射初始化
	-> early_ioremap_init // IO映射初始化
	-> setup_machine_fdt // 读取DTB文件
		-> fixmap_remap_fdt // 建立PTE entry映射才能访问物理地址
		-> early_init_dt_scan // 扫描DTB中的节点，主要是/root，/chosen
			-> early_init_dt_scan_nodes
				-> early_init_dt_scan_memory // 获取DTB中内存大小，添加至系统
					-> early_init_dt_add_memory_arch
						-> memblock_add


/* setup_machine_fdt: 校验fdt合法性，并通过下面3个回调函数扫描fdt中预留的memory及传入内核参数
1. 扫描device tree函数：of_scan_flat_dt
2. 3个回调函数：
early_init_dt_scan_chosen： 解析chosen信息，其中一般包括initrd、bootargs，保留命令行参数到boot_command_line全局变量中
early_init_dt_scan_root: 从fdt中初始化{size, address}结构信息，保存到dt_root_addr_cells,dt_root_size_cells中
early_init_dt_scan_memory：扫描fdt中memory区域，寻找device_type="memory"的节点，并通过以下函数将该区域添加到memblock管理系统
	early_init_dt_add_memory_arch
		memblock_add：将region添加到memblock.memory中
		*/
void __init setup_arch(char **cmdline_p)
{
	init_mm.start_code = (unsigned long) _text;
	init_mm.end_code   = (unsigned long) _etext;
	init_mm.end_data   = (unsigned long) _edata;
	init_mm.brk	   = (unsigned long) _end;

	*cmdline_p = boot_command_line;

	early_fixmap_init();
	early_ioremap_init();

	setup_machine_fdt(__fdt_pointer);
}
```

设备树节点举例：
```shell
/{
	#address-cells= <2> ;
	#size-cells = <2> ;

	memory@80000000 {
		device_type = "memory";
		reg = < OxOOOOOOOO Ox80000000 0 Ox80000000>,
			  < Ox00000008 Ox80000000 0 Ox80000000>;
	}；
}；
```
`#address-cells= <2> ; #size-cells = <2> ;`分别定义了每个地址包含的单元数量，每个长度包含的单元地址数量
该节点，第一个物理内存范围的起始地址是“0x00000000 0x80000000”,长度是“0 0x80000000”,及起始地址是2GB，长度是2GB；第二个地址内存范围起始地址是34GB，长度是2GB。

内核在初始化的时候调用函数early_init_dt_scan_nodes以解析设备树二进制文件，从而得到物理内存信息；

```cpp
void __init early_init_dt_scan_nodes(void)
{
	int rc = 0;

	/* Retrieve various information from the /chosen node */
	rc = of_scan_flat_dt(early_init_dt_scan_chosen, boot_command_line);
	if (!rc)
		pr_warn("No chosen node found, continuing without\n");
	// 初始化size-cells和address-cells
	/* Initialize {size,address}-cells info */
	of_scan_flat_dt(early_init_dt_scan_root, NULL);
	// 调用函数 early_init_dt_scan_memory设置内存
	/* Setup memory, calling early_init_dt_add_memory_arch */
	of_scan_flat_dt(early_init_dt_scan_memory, NULL);
}

```
解析memory描述的信息从而得到内存的base和size信息，最后内存块信息通过early_init_dt_add_memory_arch() -> memblock_add()添加到memblock子系统中。
然后结合上述的物理内存映射分析。


## 1.4. 物理内存映射--（在物理内存初始化流程之后）

在内核使用内存前，需要初始化内核的页表，初始化页表主要由`pageing_init`函数实现。

```c
/*
 * paging_init() sets up the page tables, initialises the zone memory
 * maps and sets up the zero page.
 */
void __init paging_init(void)
{
	pgd_t *pgdp = pgd_set_fixmap(__pa_symbol(swapper_pg_dir));

	map_kernel(pgdp); // 映射内核映像到内核空间的虚拟地址
	map_mem(pgdp); // 另外一次是map_mem，做物理内存的线性映射。

	pgd_clear_fixmap();

	cpu_replace_ttbr1(lm_alias(swapper_pg_dir));
	init_mm.pgd = swapper_pg_dir;

	memblock_free(__pa_symbol(init_pg_dir),
		      __pa_symbol(init_pg_end) - __pa_symbol(init_pg_dir));

	memblock_allow_resize();
}
// 它们都是建立物理内存到内核空间虚拟地址的线程映射
// 但是映射的地址不一样

```
### 1.4.1. map_kernel函数分析：
该函数对内核的各个段分别进行映射，映射至内核空间的虚拟地址为vmalloc区域。
例如：vmalloc区域的范围从0xFFFF 0000 1000 0000到0xFFFF 7DFF BFFF 0000。

```c
// arch/arm64/mm/mmu.c
static void __init map_kernel(pgd_t *pgdp)
{
	static struct vm_struct vmlinux_text, vmlinux_rodata, vmlinux_inittext,
				vmlinux_initdata, vmlinux_data;

	/*
	 * External debuggers may need to write directly to the text
	 * mapping to install SW breakpoints. Allow this (only) when
	 * explicitly requested with rodata=off.
	 */
	pgprot_t text_prot = rodata_enabled ? PAGE_KERNEL_ROX : PAGE_KERNEL_EXEC;

	/*
	 * Only rodata will be remapped with different permissions later on,
	 * all other segments are allowed to use contiguous mappings.
	 */
	map_kernel_segment(pgdp, _text, _etext, text_prot, &vmlinux_text, 0,
			   VM_NO_GUARD);
	map_kernel_segment(pgdp, __start_rodata, __inittext_begin, PAGE_KERNEL,
			   &vmlinux_rodata, NO_CONT_MAPPINGS, VM_NO_GUARD);
	map_kernel_segment(pgdp, __inittext_begin, __inittext_end, text_prot,
			   &vmlinux_inittext, 0, VM_NO_GUARD);
	map_kernel_segment(pgdp, __initdata_begin, __initdata_end, PAGE_KERNEL,
			   &vmlinux_initdata, 0, VM_NO_GUARD);
	map_kernel_segment(pgdp, _data, _end, PAGE_KERNEL, &vmlinux_data, 0, 0);
}
```
### 1.4.2. map_mem函数分析：
映射物理内存至线性映射区：



## 1.5. 伙伴子系统

memblock初始化后，此时有两个结构体记录了所有的区域。接下来就是memblock如何将这些区域通过调用伙伴系统的接口释放给伙伴系统的链表。同时将保留的页面属性进行设置reserved，从而保证不被访问。


内核初始化完，使用分配器管理物理页，当前使用的也分配器是伙伴分配器。
伙伴分配器分配和释放物理页的数量单位是阶。

```cpp
void __init mem_init(void) //arch/arm64/mm/init.c
-> memblock_free_all(); // 释放所有低端内促到伙伴系统中，计算释放总的页数
	-> pages = free_low_memory_core_early();
	 -> count += __free_memory_core(start, end);
	 	-> __free_pages_memory(start_pfn, end_pfn);
			-> memblock_free_pages(pfn_to_page(start), start, order);
				-> return __free_pages_boot_core(page, order);
					-> __free_pages(page, order);
```
Q:物理地址页面如何添加至伙伴系统的链表上？
A：释放内存用到的接口：__free_pages(page, order);函数只需要将物理地址start通过pfn_to_page(start)函数进行转换获取到page的指针和order，然后根据order和page以及页面的迁移类型，然后添加到各自的链表上。

### 1.5.1. 伙伴分配器



## 1.6. 学习技巧

early_init_dt_scan_memory -- 作为回调函数调用方法：

```cpp
/**
 * of_scan_flat_dt - scan flattened tree blob and call callback on each.
 * @it: callback function
 * @data: context data pointer
 *
 * This function is used to scan the flattened device-tree, it is
 * used to extract the memory information at boot before we can
 * unflatten the tree
 */
int __init of_scan_flat_dt(int (*it)(unsigned long node,
				     const char *uname, int depth,
				     void *data),
			   void *data)
{
	const void *blob = initial_boot_params;
	const char *pathp;
	int offset, rc = 0, depth = -1;

	if (!blob)
		return 0;

	for (offset = fdt_next_node(blob, -1, &depth);
	     offset >= 0 && depth >= 0 && !rc;
	     offset = fdt_next_node(blob, offset, &depth)) {

		pathp = fdt_get_name(blob, offset, NULL);
		if (*pathp == '/')
			pathp = kbasename(pathp);
		rc = it(offset, pathp, depth, data);
	}
	return rc;
}

//
// of_scan_flat_dt(early_init_dt_scan_memory, NULL); 如何调用early_init_dt_scan_memory函数执行，并传入参数？
// 调用时，输入的函数指针就是：early_init_dt_scan_memory

```


## 1.7. memblock如何释放内存给伙伴系统？

https://blog.csdn.net/LuckyDog0623/article/details/141573584

```cpp
mm_init
-> mem_init
	-> memblock_free_all

/**
 * for_each_free_mem_range - iterate through free memblock areas
 * @i: u64 used as loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock.  Available as
 * soon as memblock is initialized.
 */
#define for_each_free_mem_range(i, nid, flags, p_start, p_end, p_nid)	\
	for_each_mem_range(i, &memblock.memory, &memblock.reserved,	\
			   nid, flags, p_start, p_end, p_nid)


			   /**
 * memblock_free_all - release free pages to the buddy allocator
 *
 * Return: the number of pages actually released.
 */
unsigned long __init memblock_free_all(void)
{
	unsigned long pages;

	reset_all_zones_managed_pages();

	pages = free_low_memory_core_early();
	totalram_pages_add(pages);

	return pages;
}

static unsigned long __init free_low_memory_core_early(void)
{
	unsigned long count = 0;
	phys_addr_t start, end;
	u64 i;

	memblock_clear_hotplug(0, -1);

	for_each_reserved_mem_region(i, &start, &end)
		reserve_bootmem_region(start, end);

	/*
	 * We need to use NUMA_NO_NODE instead of NODE_DATA(0)->node_id
	 *  because in some case like Node0 doesn't have RAM installed
	 *  low ram will be on Node1
	 */
	for_each_free_mem_range(i, NUMA_NO_NODE, MEMBLOCK_NONE, &start, &end,
				NULL)
		count += __free_memory_core(start, end);

	return count;
}


__free_memory_core
-> __free_pages_memory(start_pfn, end_pfn);
	-> memblock_free_pages(pfn_to_page(start), start, order);
		-> return __free_pages_boot_core(page, order);
			-> __free_pages(page, order);
```

### 1.7.1. 如何释放内存给伙伴系统？

为什么说memblock_free_all函数调用可以直接将可用的内存区域释放给伙伴系统？
mem_init -> memblock_free_all ->释放所有低端内存到伙伴系统中，计算释放总的页数。在这个过程中，通过for_each_reserved_mem_range函数遍历所有的reserved区。memory区也同理。
获取了每个区域的起始结束地址之后，通过reserve_bootmem_region函数将每个region的页面属性设置为reserved，通过__free_memory_core函数将free的region区域释放到伙伴系统。所以这里主要是__free_memory_core函数的调用进行释放（最终通过free_page函数释放）。
前面我们释放内存用到的接口free_pages(vir_addr, PAGE_ORDER); __free_pages(you_page, PAGE_ORDER); 函数内部只需要将物理地址start通过pfn_to_page(start)函数进行转换获取到page的指针和order。然后根据order和page以及页面的迁移类型，然后添加到各自的链表上。
