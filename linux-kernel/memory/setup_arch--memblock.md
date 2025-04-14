# fixmap, memblock时代
## memblock建立过程及分页机制化
主要有如下几个步骤：
- setup_machine_fdt: 解析dtb，收集内存信息及bootargs
  1. hoosen node。该节点有一个bootargs属性，该属性定义了内核的启动参数，而在启动参数中，可能包括了mem=nn[KMG]这样的参数项。initrd-start和initrd-end参数定义了initial ramdisk image的物理地址范围。
  2. memory node。这个节点主要定义了系统中的物理内存布局。主要的布局信息是通过reg属性来定义的，该属性定义了若干的起始地址和size条目。
  3. DTB header中的memreserve域。对于dts而言，这个域是定义在root node之外的一行字符串，例如：/memreserve/ 0x05e00000 0x00100000。
  4. reserved-memory node。这个节点及其子节点定义了系统中保留的内存地址区域。保留内存有两种(1. 静态定义，用reg属性定义的address和size; 2. 动态定义，通过size属性定义了保留内存区域的长度，或者通过alignment属性定义对齐属性)
- early_fixmap_init: 对保留的fixmap区域创建映射
- early_ioremap_init: 初始化early_ioremap机制
- arm64_memblock_init: 初始化memblock机制
- paging_init: 初始化内核页表，内存节点，内存域及页帧page, 此函数功能较为复杂
- request_standard_resources：将memblock.memory挂载到iomem_resource资源树下
- early_ioremap_reset: 结束early_ioremap机制
- unflatten_device_tree: dtb转换为device_node tree
- 根据device node tree初始化CPU，psci

## 关键函数源码及流程分析
###  setup_arch
```cpp
void __init __no_sanitize_address setup_arch(char **cmdline_p)
{
	setup_initial_init_mm(_stext, _etext, _edata, _end);

	*cmdline_p = boot_command_line; // /* 将fdt中解析的boot_command_line参数传出setup_arch函数*/

	/*
	 * If know now we are going to need KPTI then use non-global
	 * mappings from the start, avoiding the cost of rewriting
	 * everything later.
	 */
	arm64_use_ng_mappings = kaslr_requires_kpti();
    // 对保留的fixmap区域创建映射，并对fixmao校验是否在同一pmd内，否则报错
	early_fixmap_init();
    //
	early_ioremap_init();
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
	setup_machine_fdt(__fdt_pointer);

	/*
	 * Initialise the static keys early as they may be enabled by the
	 * cpufeature code and early parameters.
	 */
	jump_label_init();
	parse_early_param();

	/*
	 * Unmask asynchronous aborts and fiq after bringing up possible
	 * earlycon. (Report possible System Errors once we can report this
	 * occurred).
	 */
	local_daif_restore(DAIF_PROCCTX_NOIRQ);

	/*
	 * TTBR0 is only used for the identity mapping at this stage. Make it
	 * point to zero page to avoid speculatively fetching new entries.
	 */
	cpu_uninstall_idmap();

	xen_early_init();
    /*
	 * 1. 使用回调函数fdt_find_uefi_params获取fdt中关于uefi的信息，所有要扫描的参数包含在dt_param这个全局数组中
	 *         例如：system table, memmap address, memmap size, memmap desc. size, memmap desc. version
	 * 2. 将解析出的内存区域加入memblock.reserve区域
	 * 3. 调用uefi_init(), reserve_regions()等函数进行uefi模式初始化
	 * 4. 初始化完成后，将前面reserve的区域unreserve
	 *
	 */
	efi_init();

	if (!efi_enabled(EFI_BOOT) && ((u64)_text % MIN_KIMG_ALIGN) != 0)
	     pr_warn(FW_BUG "Kernel image misaligned at boot, please fix your bootloader!");
    // 初始化memblock
	arm64_memblock_init();
    // 该函数需要分析
	paging_init();

	acpi_table_upgrade();

	/* Parse the ACPI tables for possible boot-time configuration */
	acpi_boot_table_init();

	if (acpi_disabled)
		unflatten_device_tree();
    // bootmem_init：初始化内存节点，内存域及页帧page
	bootmem_init();

	kasan_init();

	request_standard_resources();

	early_ioremap_reset();

	if (acpi_disabled)
		psci_dt_init();
	else
		psci_acpi_init();

	init_bootcpu_ops();
	smp_init_cpus();
	smp_build_mpidr_hash();

	/* Init percpu seeds for random tags after cpus are set up. */
	kasan_init_sw_tags();

#ifdef CONFIG_ARM64_SW_TTBR0_PAN
	/*
	 * Make sure init_thread_info.ttbr0 always generates translation
	 * faults in case uaccess_enable() is inadvertently called by the init
	 * thread.
	 */
	init_task.thread_info.ttbr0 = phys_to_ttbr(__pa_symbol(reserved_pg_dir));
#endif

	if (boot_args[1] || boot_args[2] || boot_args[3]) {
		pr_err("WARNING: x1-x3 nonzero in violation of boot protocol:\n"
			"\tx1: %016llx\n\tx2: %016llx\n\tx3: %016llx\n"
			"This indicates a broken bootloader or old kernel\n",
			boot_args[1], boot_args[2], boot_args[3]);
	}
}
```
### arm64_memblock_init

```cpp
setup_arch
-> arm64_memblock_init // arch/arm64/mm/init.c

void __init arm64_memblock_init(void)
{
	s64 linear_region_size = PAGE_END - _PAGE_OFFSET(vabits_actual);

    ............
}



```




## 源码流程分析--从memblock释放page至伙伴系统
Bootmem机制（memblock）--（此时伙伴系统还未成型，一直到mm_init()函数中mem_init()将空闲内存加载到zone中）

在zone_size_init之后，各个node，zone的page总数已知，但是此时每个order的空闲链表是空的，也就是无法通过alloc_page这种接口来分配。
此时page还在memblock掌控，需要memblock释放。释放函数是free_low_memory_core_early：

```cpp
// 启动流程调用
start_kernel // init/main.c
-> void __init __no_sanitize_address setup_arch(char **cmdline_p)
    ->machine_specific_memory_setup: 创建一个列表，包括系统占据的内存区和空闲内存区
    ->parse_early_param: 解析dtb树命令行
    ->setup_memory: 确定每个节点可用的物理内存也数目，初始化bootmem分配器，分配各种内存区

    -> paging_init: 初始化内核页表并启动内存分页
    -> bootmem_init
        -> zone_size_init :初始化系统中所有节点的pgdat_t实例
-> mm_init   //init/main.c
	mem_init(void)   //arch/arm64/mm/init.c
		memblock_free_all(); // 在memblock中释放所有至伙伴系统管理


void __init memblock_free_all(void)
{
	unsigned long pages;

	free_unused_memmap();
	reset_all_zones_managed_pages();

	pages = free_low_memory_core_early();
	totalram_pages_add(pages);
}

// 函数调用流程
free_low_memory_core_early
	for_each_free_mem_range
		 __free_memory_core(start, end);
			__free_pages_memory(start_pfn, end_pfn);
				memblock_free_pages(pfn_to_page(start), start, order);
					__free_pages_core
						__free_pages_ok
							__free_one_page
```

memblock_free_all释放page到buddy，前后nr_free的情况
调试补丁如下：
```cpp
diff --git a/arch/arm64/mm/init.c b/arch/arm64/mm/init.c
index a883443..4a0ffbe 100755
--- a/arch/arm64/mm/init.c
+++ b/arch/arm64/mm/init.c
@@ -174,6 +174,42 @@ EXPORT_SYMBOL(pfn_is_map_memory);

 static phys_addr_t memory_limit = PHYS_ADDR_MAX;

+static void show_zone_info(struct zone *zone) {
+       int i;
+       struct free_area * area;
+       if (!zone || !zone->name) {
+               return;
+       }
+       printk("===show_zone_info :\n"
+                        "       name : %s\n"
+                        "       managed_pages:%llx\n"
+                        "       spanned_pages:%llx\n"
+                        "       present_pages:%llx\n",
+                       zone->name, zone->managed_pages, zone->spanned_pages, zone->present_pages);
+       for (i = 0; i <= MAX_ORDER; i++) {
+               area = &zone->free_area[i];
+               printk(" MAX_ORAER %d of order: %d has nr_free %llx \n",
+                                       MAX_ORDER, i, zone->free_area[i].nr_free);
+       }
+}
+
+static void show_pgdata_info(struct pglist_data *pgdat) {
+       if (!pgdat)
+               return;
+        printk("===show_pgdata_info nodeid %d \n",pgdat->node_id);
+        struct zone *z;
+        for (z = pgdat->node_zones; z < pgdat->node_zones + MAX_NR_ZONES; z++)
+                show_zone_info(z);
+}
+
+static void show_mem_info(const char *info)
+{
+        printk("===show_mem_info  %s\n",info);
+        struct pglist_data *pgdat;
+        for_each_online_pgdat(pgdat)
+                show_pgdata_info(pgdat);
+}
+
 /*
  * Limit the memory size that was specified via FDT.
  */
@@ -351,7 +387,7 @@ void __init bootmem_init(void)
         */
        sparse_init();
        zone_sizes_init(min, max);
-
+       show_mem_info("after zone_sizes_init\n");
        /*
         * Reserve the CMA area after arm64_dma_phys_limit was initialised.
         */
@@ -381,7 +417,7 @@ void __init mem_init(void)

        /* this will put all unused low memory onto the freelists */
        memblock_free_all();
-
+       show_mem_info("after memblock_free_all\n");
        /*
         * Check boundaries twice: Some fundamental inconsistencies can be
         * detected at build time already.

```
