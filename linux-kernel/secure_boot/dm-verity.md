
## 1. device-mapper概述
device-mapper是一种块设备虚拟化技术，下面分别介绍device-mapper的背景、在linux块设备框架中位置、内核配置、相关文件、核心数据结构、相关module初始化、支持的target

### 1.1. dm说明
device mapper设备映射器是linux内核中一个块设备虚拟化框架，旨在提供灵活的块设备抽象和管理能力；
将物理块设备（如磁盘、分区）映射为逻辑设备，支持对底层设备的透明扩展、分割或功能增强；

### 1.2. dm内核配置

```cpp
Device Drivers
　　->Multiple devices driver support (RAID and LVM)
　　　　->Device mapper support
　　　　　　->Device mapper debugging support
　　　　　　->DM "dm-mod.create=" parameter support
　　　　　　->Verity target support
　　　　　　　　->Verity data device root hash signature verification support
　　　　　　　　->Verity forward error correction support


```

### 1.3. dm相关文件

dm涉及到文件主要有：


### 1.4. dm代码及主要数据结构

#### 1.4.1. dm数据结构和API



## 2. dm-verity设备创建流程

verity_target是linux内核device Mapper中的一种目标类型，用于实现dm-verity功能，主要作用是确保数据的完整性和安全性，防止数据在存储和传输过程中被篡改。通过对数据块和哈希值验证，确保从存储设备读取的数据与预先计算的哈希值匹配。如果数据被篡改，哈希值验证失败，从而检测到数据完整性问题。

```cpp

module_init(dm_verity_init);


```

### 2.1. 源码分析
```cpp
// src/kernel/kernel-5.10/drivers/md/dm-verity-target.c
static struct target_type verity_target = {
	.name		= "verity",
	.features	= DM_TARGET_IMMUTABLE, // 表示此类target是不可变的
	.version	= {1, 7, 0},
	.module		= THIS_MODULE,
	.ctr		= verity_ctr, // 构造函数指针，负责解析taget参数，分配必要的资源，并设置target初始状态
	.dtr		= verity_dtr, // 析构函数，调用此函数释放target所占用的资源
	.map		= verity_map, // 将上层IO请求映射到底层的物理设备
	.status		= verity_status, // 返回target的状态和进行调试
	.prepare_ioctl	= verity_prepare_ioctl, // 用于确定是否对该target进行ioctl操作，并且返回一个底层设备，ioctl·tl操作将针对该设备执行
	.iterate_devices = verity_iterate_devices, // 编译target所使用的底层设备
	.io_hints	= verity_io_hints,
};




```
verity_ctr解析传入的命令行参数，更新struct dm_target 参数或根据参数创建资源：

```cpp
/*
 * Target parameters:
 *	<version>	The current format is version 1.
 *			Vsn 0 is compatible with original Chromium OS releases.
 *	<data device>
 *	<hash device>
 *	<data block size>
 *	<hash block size>
 *	<the number of data blocks>
 *	<hash start block>
 *	<algorithm>
 *	<digest>
 *	<salt>		Hex string or "-" if no salt.
 */
static int verity_ctr(struct dm_target *ti, unsigned argc, char **argv)

// 各个参数解析：
/*
　　argv[0]--表示struct dm_verity->version。
　　dm_get_device--argv[1]和argv[2]分别获取struct dm_verity->data_dev和struct dm_verity->hash_dev对应设备名。
　　argv[3]和argv[4]--表示数据块和hash块大小。
　　argv[5]--表示数据块数量。
　　argv[6]--表示hash值在hash块设备上起始块。
　　argv[7]--表示算法名称。
　　argv[8]--对应struct dm_verity->root_digest。
　　argv[9]--对应struct dm_verity->salt。
*/
```



#### 2.1.1. verity_map函数分析

verity_map是linux内核中的device mapper（DM）层一部分,具体用于dm-verity驱动模块。dm-verity是一种基于验证块设备数据完整性的机制，并提交IO到下层设备；这里主要用于验证根文件系统分区；

```cpp
/*
 * Bio map function. It allocates dm_verity_io structure and bio vector and
 * fills them. Then it issues prefetches and the I/O.
 */
static int verity_map(struct dm_target *ti, struct bio *bio)
{
	struct dm_verity *v = ti->private;
	struct dm_verity_io *io;

	bio_set_dev(bio, v->data_dev->bdev);
    // 将逻辑扇区映射到物理扇区号
	bio->bi_iter.bi_sector = verity_map_sector(v, bio->bi_iter.bi_sector);
    // 确保IO请求的起始扇区和长度data_dev_block_size 的整数倍。
	if (((unsigned)bio->bi_iter.bi_sector | bio_sectors(bio)) &
	    ((1 << (v->data_dev_block_bits - SECTOR_SHIFT)) - 1)) {
		DMERR_LIMIT("unaligned io");
		return DM_MAPIO_KILL;
	}

	if (bio_end_sector(bio) >>
	    (v->data_dev_block_bits - SECTOR_SHIFT) > v->data_blocks) {
		DMERR_LIMIT("io out of range");
		return DM_MAPIO_KILL;
	}

	if (bio_data_dir(bio) == WRITE)
		return DM_MAPIO_KILL;

	io = dm_per_bio_data(bio, ti->per_io_data_size);
	io->v = v;
	io->orig_bi_end_io = bio->bi_end_io;
	io->block = bio->bi_iter.bi_sector >> (v->data_dev_block_bits - SECTOR_SHIFT);
	io->n_blocks = bio->bi_iter.bi_size >> v->data_dev_block_bits;

	bio->bi_end_io = verity_end_io;
	bio->bi_private = io;
	io->iter = bio->bi_iter;

	verity_fec_init_io(io);

	verity_submit_prefetch(v, io);

	submit_bio_noacct(bio);

	return DM_MAPIO_SUBMITTED;
}

```



## 3. 参考文献：
[device-mapper(1)：概述](https://www.cnblogs.com/arnoldlu/p/18845072)
[device-mapper(2)：块设备数据完整性验证功能dm-verity](https://www.cnblogs.com/arnoldlu/p/18882608)
[dm-verity使能](https://geekdaxue.co/read/tiehichi@kernel/aodg7g)
