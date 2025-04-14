
# 1. 设备树概述
Hypervisor和虚拟机的配置通过dts文件进行配置，dtc工具将dts文件编译为dtb二进制文件，在Hypervisor加载虚拟机时，通过读取dtb文件获取虚拟机的配置。
以Xvisor的QEMU环境为例，Hypervisor的配置由 `qemu-aarch-virt-gicv2.dts 和 qemu-aarch-virt-gicv2.dtsi` 进行配置，虚拟机serverVM通过 `virt-v8-server.dts` 进行配置（个人理解该设备树为提供的硬件实际环境，类似serverVM看到的所有硬件资源）。它们解析的过程如下：

1. 编译工程时，通过dtc工具将dts编译为dtb文件，对应为 `qemu-aarch-virt-gicv2.dtb 和 virt-v8-server.dtb`。
2. `qemu-aarch-virt-gicv2.dtb` 启动时拷贝至uboot，uboot 将其放入一块内存，并将该内存的地址返回给系统；virt-v8-server.dtb 在构建时被写入文件系统。
3. Hypervisor启动后，通过 `vmm_devtree_init` 接口初始化设备树信息，它通过读取uboot返回的地址获取到qemu-aarch-virt-gicv2.dtb，将dtb二进制进行解析，并转换为 `fdt_fileinfo` 以 `vmm_devtree_node` 的形式存储在内存中，后续通过 `vmm_devtree_node` 进行 host 各个配置节点的初始化。
4. 文件系统初始化后并挂载后，虚拟机serverVM通过执行 `vfs guest_fdt_load serverVM /virt-v8-server.dtb` 命令，将 server.dtb 转换为 fdt_fileinfo 以 vmm_devtree_node 存储在内容中，并将解析出的虚拟机配置插入至hypervisor配置中，形成一个总的配置信息表。

// TODO:: 需要补充设备树流程图

如下通过在 `host shell` 中使用` devtree node dump `命令查看的设备树信息，根节点下既有hypervisor的配置，也有虚拟机的配置，虚拟机的配置在节点`/guests/serverVM` 下。

## 1.1. dts语法相关问题
dtc为编译工具，dtc编译器可以把dts文件编译成为dtb，如果发现语法或格式上有错误，在该阶段就会被编译器报错。因此配置文件的格式完整性由dtc工具保证。
xvisor中的主要结构体数据：
```cpp
// hypervisor/libs/include/libs/libfdt.h
struct fdt_fileinfo {
	struct fdt_header header;
	char *data;
	size_t data_size;
	char *str;
	size_t str_size;
	char *mem_rsvmap;
	u32 mem_rsvcnt;
};
// 
struct fdt_header {
    u32 magic;               /* 设备树的魔数，魔数其实就是一个用于识别的数字，表示设备树的开始，linux dtb的魔数为   0xd00dfeed. [大端对齐] */
    u32 totalsize;           /*  这个设备树的size，也可以理解为所占用的实际内存空间*/ 
    u32 off_dt_struct;       /*  表示整个dtb中structure block 部分所在内存相对头部的偏移地址 */ 
    u32 off_dt_strings;      /*  表示整个dtb中string block 部分所在内存相对头部的偏移地址 */ 
    u32 off_mem_rsvmap;      /*  表示整个dtb中memory reserve map block 部分所在内存相对头部的偏移地址 */ 
    u32 version;             /*  表示设备树版本 */ 
    u32 last_comp_version;   /*  表示最新的兼容版本 */ 
    /* version 2 fields below */
    u32 boot_cpuid_phys;     /*  系统引导CPU的物理ID，这部分仅在版本2中存在，后续版本不再使用*/ 
    /*  version 3 fields below */ 
    u32 size_dt_strings;     /*  表示整个dtb中string block 部分的大小 */ 
    /*  version 17 fields below */ 
    u32 size_dt_struct;      /*  表示整个dtb中struct block 部分的大小 */ 
};
// 内存的保留信息块（memory reservation block）是由多个fdt_reserve_entry构成的：
struct fdt_reserve_entry {
    u64 address; /*  每个保留块的地址 */ 
    u64 size;  /*  每个保留块的大小 */ 
};
// 节点块（structure block）存放了各个节点的信息，dts 文件中 node 和 property 按照以下格式存储在 dtb 文件，以tag进行区分。
#define FDT_BEGIN_NODE  0x1 /*  Start node: full name */ 
#define FDT_END_NODE    0x2 /*  End node */ 
#define FDT_PROP        0x3 /*  Property: name off,size, content */ 
#define FDT_NOP         0x4 /*  nop */ 
#define FDT_END         0x9
struct fdt_node_header {
    u32 tag;
    char name[0]; /*  是个指针，存放的node name的地址 */ 
};
struct fdt_property {
    u32 tag;
    u32 len;      /*  记录value占用的字节长度 */ 
    u32 nameoff;  /*  记录属性name的偏移 */ 
    char data[0]; /*  是个指针，记录属性值 */ 
};
```
### 1.1.1. xvisor获取自己dtb地址
dtb解析依赖以下四个地址：
```cpp
virtual_addr_t devtree_virt; /*  设备树的虚拟地址 */ 
virtual_addr_t devtree_virt_base; /*  设备树的虚拟地址基址 */ 
physical_addr_t devtree_phys_base; /*  设备树的物理地址基址 */ 
virtual_size_t devtree_virt_size; /*  设备树的虚拟大小 */ 
```
这四个地址都是通过初始页表设置进行初始化的。用于在内存管理单元（MMU）的转换表中正确映射设备树的地址信息。
```cpp
// hypervisor/arch/arm/cpu/common/mmu_lpae_entry_ttbl.c
void __attribute__ ((section(".entry")))
    _setup_initial_ttbl(virtual_addr_t load_start, virtual_addr_t load_end,
			virtual_addr_t exec_start, virtual_addr_t dtb_start)
{
	u32 i;
	u64 *root_ttbl;
	virtual_addr_t exec_end = exec_start + (load_end - load_start);
#ifdef CONFIG_ARCH_GENERIC_DEFTERM_EARLY
	virtual_addr_t defterm_early_va;
#endif
	virtual_addr_t *dt_virt =
		(virtual_addr_t *)to_load_pa((virtual_addr_t)&devtree_virt);
	virtual_addr_t *dt_virt_base =
		(virtual_addr_t *)to_load_pa((virtual_addr_t)&devtree_virt_base);
	virtual_size_t *dt_virt_size =
		(virtual_size_t *)to_load_pa((virtual_addr_t)&devtree_virt_size);
	physical_addr_t *dt_phys_base =
		(physical_addr_t *)to_load_pa((virtual_addr_t)&devtree_phys_base);
	struct mmu_lpae_entry_ctrl lpae_entry = { 0, NULL, 0 };

    ............................

	/* Map rest of logical addresses which are
	 * not covered by read-only linker sections
	 * Note: This mapping is used at runtime
	 */
	__setup_initial_ttbl(&lpae_entry, exec_start, exec_end, load_start,
			     AINDEX_NORMAL_WB, TRUE);

	/* Compute and save devtree addresses */
	*dt_phys_base = dtb_start & TTBL_L3_MAP_MASK;
	*dt_virt_base = exec_start - _fdt_size(dtb_start);
	*dt_virt_base &= TTBL_L3_MAP_MASK;
	*dt_virt_size = exec_start - *dt_virt_base;
	*dt_virt = *dt_virt_base + (dtb_start & (TTBL_L3_BLOCK_SIZE - 1));

	/* Map device tree */
	__setup_initial_ttbl(&lpae_entry, *dt_virt_base,
			     *dt_virt_base + *dt_virt_size, *dt_phys_base,
			     AINDEX_NORMAL_WB, TRUE);
}
```
以上具体信息还可以进行打印验证，深入分析。

# 2. xvisor--硬件dtb资源的源文件解析

设备树的初始化在init_bootcpu时通过vmm_devtree_init接口完成的，该函数用于初始化vmm_devtree_control结构，并执行设备树的填充和节点表的填充。
```cpp
int __init vmm_devtree_init(void){
    int rc;
    u32 nidtbl_cnt;
    virtual_addr_t ca, nidtbl_va;
    virtual_size_t nidtbl_sz;
    struct vmm_devtree_nidtbl_entry *nide, *tnide;
    
    /*  使用memset函数将dtree_ctrl结构重置为零，以初始化控制结构 */ 
    memset(&dtree_ctrl, 0, sizeof(dtree_ctrl));
    /*  调用arch_devtree_populate函数，将设备树填充到dtree_ctrl.root中。这个函数的作用是获取设备树信息并进行解析，将解析得到的设备树存储在dtree_ctrl.root中 */ 
    rc = arch_devtree_populate(&dtree_ctrl.root);
    if (rc) {
        return rc;
    }
    ...
}
```
其中arch_devtree_populate函数用于初始化设备树（device tree）并将其填充到vmm_devtree_node结构体root中：
```cpp
int __init arch_devtree_populate(struct vmm_devtree_node *root){
    int rc = VMM_OK;
    virtual_addr_t off;
    struct fdt_fileinfo fdt;
    if (!devtree_virt_size) {
        return VMM_ENOTAVAIL;
    }
    
    /*  使用libfdt_parse_fileinfo函数解析devtree_virt中的设备树信息，并将解析结果保存到fdt结构体中 */ 
    rc = libfdt_parse_fileinfo(devtree_virt, &fdt);
    if (rc) {
        return rc;
    }
    
    /*   使用libfdt_parse_devtree函数将设备树解析为vmm_devtree_node结构体，并将解析结果保存到root指向的指针中 */ 
    rc = libfdt_parse_devtree(&fdt, root, "0", NULL);
    if (rc) {
        return rc;
    }
    
    for (off = 0; off \u003C devtree_virt_size; off += VMM_PAGE_SIZE) {
        arch_cpu_aspace_unmap(devtree_virt_base + off);
    }
    return vmm_host_ram_free(devtree_phys_base, devtree_virt_size);
}
```
 `devtree_virt, devtree_virt_base, devtree_phys_base 和 devtree_virt_size `用于存储与虚拟机监视器（VMM）中的设备树相关的信息。在初始页表设置过程中进行初始化`(_start_mmu_init ---> _setup_initial_ttbl ---> 设置设备树地址）`
通过devtree_virt获取设备树信息后，使用libfdt库对设备树文件进行解析，递归遍历每个节点，解析节点属性和设备信息，并将解析结果存储在设备树的数据结构中供后续使用。
xvisor的VMM的设备树解析过程为获取设备树信息后，使用libfdt库对设备树文件进行解析，递归遍历每个节点，解析节点属性和设备信息，并将解析结果存储在设备树的数据结构中供后续使用。这个过程使得xvisor能够根据设备树的描述来进行设备的初始化和配置。
# 3. xvisor-设备树模块源代码
```cpp
vmm_devtree.c
vmm_error.h //错误码，返回码的定义
vmm_compiler.h //devtree的初始化函数放入特殊的段(__init)
vmm_heap.h //内存分配函数(vmm_zalloc), vmm_free
vmm_stdio.h // WARN_ON，vmm_printf
vmm_host_io.h // 交换字节顺序
vmm_devtree.h
arch_sections.h // 用于从ld脚本获取nidtbl的初始地址和size
arch_devtree.h // 用于引用arch_devtree_populate，进行设备数解析
libs/mathlib.h // 数学库函数udiv32等
libs/stringlib.h // string库函数mmecpy等

generic_devtree.c
vmm_error.h // 错误码，返回码的定义
vmm_host_aspace.h // VMM_PAGE_SIZE定义
vmm_host_ram.h // vmm_host_ram_free引用
libs/stringlib.h // string库函数mmecpy等
libs/libfdt.h // libfdt函数libfdt_get_property、libfdt_parse_fileinfo、libfdt_parse_devtree等
arch_cpu_aspace.h // arch_cpu_aspace_unmap引用
arch_devtree.h
arch_sections.h
generic_devtree.h

libfdt.c
vmm_error.h // 错误码，返回码的定义
vmm_heap.h // 内存分配函数(vmm_zalloc)
vmm_host_io.h // 交换字节顺序
libs/stringlib.h // string库函数mmecpy等
libs/mathlib.h // 数学库函数udiv32等
libs/libfdt.h
```
# 4. xvisor提供虚拟化资源设备树--xxx_guest.dts
xvisor的cmd命令创建guest时，会通过传入guest的name找到关于该guest的设备树父节点，然后通过以下调用链：
```bash
cmd_guest_create 
        ------>  vmm_manager_guest_create 
                -------->  vmm_guest_aspace_init 
                        -------->  region_add：
```
将guest的aspace空间完成初始化后，为对应的设备树节点创建好对应的地址空间即完成设备树的绑定。
region结构体
分析：region，每一个设备对应一个region。
Region结构体的填充都是依据分配给guest的设备树来的，其中可以读取各个设备对应的gphys_addr（gpa），flags（由设备树多个属性通过比特位实现）等。flag表示的region读取的设备到底是哪种设备，比如ram，是需要vmm在内存中真正的给guest分配的内存空间，所以是real类型的。而对于rtc这种，vmm采用的方式是使用软件emulator模拟的方式，并不是一个真实分配的物理硬件，就是virtual虚拟的。`vmm_region_mapping *maps`结构体中存放的是`hphys_addr（hpa）`，也就是设备的真实的物理地址。

```cpp
struct vmm_region {
	struct rb_node head;
	struct dlist phead;
	struct vmm_devtree_node *node;
	struct vmm_guest_aspace *aspace;
	u32 flags;
	physical_addr_t gphys_addr;
	physical_addr_t aphys_addr;
	physical_size_t phys_size;
	u32 first_color;
	u32 num_colors;
	struct vmm_shmem *shm;
	u32 align_order;
	u32 map_order;
	u32 maps_count;
	struct vmm_region_mapping *maps;
	void *devemu_priv;
	void *priv;
};
```

设备树是随OS镜像一起打包好的，在`xvisor/tests/arm64/virt-v8/virt64-guest.dts`中有关于arm架构给linux提供的设备树，描述了给guest分配各个设备的信息。打包镜像的时候，将设备树dts通过dtc设备树编译器编译成二进制dtb。在guest创建时，调用`vmm_guest_aspace_init ----> region_add`，通过循环遍历设备树的方式初始化guest的地址空间。
```cpp
int vmm_guest_aspace_init(struct vmm_guest *guest) {
    ...
    /*  Create regions */ 
    vmm_devtree_for_each_child(rnode, aspace->node) {
        rc = region_add(guest, rnode, NULL, NULL, TRUE);
        if (rc) {
            vmm_printf("create regions [%s] rc = [%d]\n",rnode->name, rc);
            vmm_devtree_dref_node(rnode);
            return rc;
        }
    }
    ...}
```
region的初始化，地址的绑定的是在vmm_guest_aspace_init完成的
```bash
vmm_manager.c
    ->vmm_manager_guest_create
        ->vmm_guest_aspace_init
```
vmm_devtree_for_each_child去每个设备树获得设备树的节点，然后根据设备树节点的结构体描述符去填充region，并将region结构体以树的形式添加到guest的aspace中形成region_list。
```cpp
vmm_devtree_for_each_child(rnode, aspace->node) {               
    rc = region_add(guest, rnode, NULL, NULL, TRUE);
    if (rc) {
       vmm_devtree_dref_node(rnode);
       return rc;
    }
}
```
region_add中传入的参数是对应的guest结构体，对应的设备树节点node，一个新的也是空的，即将被填充的region结构体变量new_reg。


