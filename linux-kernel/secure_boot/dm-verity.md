# 1. device-mapper原理

## 1.1. device-mapper概述
device-mapper是一种块设备虚拟化技术，下面分别介绍device-mapper的背景、在linux块设备框架中位置、内核配置、相关文件、核心数据结构、相关module初始化、支持的target
### 1.1.1. dm说明
device mapper设备映射器是linux内核中一个块设备虚拟化框架，旨在提供灵活的块设备抽象和管理能力；
将物理块设备（如磁盘、分区）映射为逻辑设备，支持对底层设备的透明扩展、分割或功能增强；
### 1.1.2. dm内核配置
```cpp
Device Drivers
　　->Multiple devices driver support (RAID and LVM)
　　　　->Device mapper support
　　　　　　->Device mapper debugging support
　　　　　　->DM "dm-mod.create=" parameter support
　　　　　　->Verity target support
　　　　　　　　->Verity data device root hash signature verification support
　　　　　　　　->Verity forward error correction support
CONFIG_MD=y
CONFIG_DM_DEBUG=y
CONFIG_BLK_DEV_DM_BUILTIN=y
CONFIG_BLK_DEV_DM=y
CONFIG_DM_BUFIO=y
CONFIG_DM_VERITY=y
CONFIG_DM_INIT=y
```
### 1.1.3. dm相关文件
dm涉及到文件主要有：

### 1.1.4. dm代码及主要数据结构
#### 1.1.4.1. dm数据结构和API


## 1.2. dm-verity设备创建流程
verity_target是linux内核device Mapper中的一种目标类型，用于实现dm-verity功能，主要作用是确保数据的完整性和安全性，防止数据在存储和传输过程中被篡改。通过对数据块和哈希值验证，确保从存储设备读取的数据与预先计算的哈希值匹配。如果数据被篡改，哈希值验证失败，从而检测到数据完整性问题。
```cpp
module_init(dm_verity_init);

```
### 1.2.1. 源码分析
```cpp
// src/kernel/kernel-5.10/drivers/md/dm-verity-target.c
static struct target_type verity_target = {
    .name       = "verity",
    .features   = DM_TARGET_IMMUTABLE, // 表示此类target是不可变的
    .version    = {1, 7, 0},
    .module     = THIS_MODULE,
    .ctr        = verity_ctr, // 构造函数指针，负责解析taget参数，分配必要的资源，并设置target初始状态
    .dtr        = verity_dtr, // 析构函数，调用此函数释放target所占用的资源
    .map        = verity_map, // 将上层IO请求映射到底层的物理设备
    .status     = verity_status, // 返回target的状态和进行调试
    .prepare_ioctl  = verity_prepare_ioctl, // 用于确定是否对该target进行ioctl操作，并且返回一个底层设备，ioctl·tl操作将针对该设备执行
    .iterate_devices = verity_iterate_devices, // 编译target所使用的底层设备
    .io_hints   = verity_io_hints,
};



```
verity_ctr解析传入的命令行参数，更新struct dm_target 参数或根据参数创建资源：
```cpp
/*
 * Target parameters:
 *  <version>   The current format is version 1.
 *          Vsn 0 is compatible with original Chromium OS releases.
 *  <data device>
 *  <hash device>
 *  <data block size>
 *  <hash block size>
 *  <the number of data blocks>
 *  <hash start block>
 *  <algorithm>
 *  <digest>
 *  <salt>      Hex string or "-" if no salt.
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


#### 1.2.1.1. verity_map函数分析
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
# 2. rootfs
## 2.1. rootfs验签主要流程
当前rootfs是squashfs时，通常基于dm-verity实现根文件系统的验签。主要实现流程：
1. 基于rootfs.squashfs生成hash_device(data_device  + hash_device)
2. 开启内核dm-verity配置
3. 获取hash_table参数等,基于hash table作为参数验签块设备（即根文件系统对应的块设备分区）
4. 验签成功创建块设备对应映射的dm-verity虚拟设备
5. 操作虚拟设备进行mount操作等，进入rootfs
备注：请注意针对需要对文件系统的分区修改的目录，挂载其他分区，以免导致系统异常。
待补充：流程图。

## 2.2. rootfs验签原理

## 2.3. roothash值生成及rootfs的签名流程
1. 生成根文件系统镜像
2. 生成镜像的hash tree
3. 生成hahs的dm-verity table
4. 对dm-verity签名：首先验证表签名。该验证是针对镜像上某个固定位置额密钥来完成的。
5. 将签名、dm-verity table打包到verity metadata
6. 将verity metadata，hashtree添加到根文件系统镜像末尾。
待补充流程图：

## 2.4. roothash的可信验证
roothash放在initramfs的脚本中，rootfs由kernel进行加载，kernel通过dm-verity机制并根据initramfs的init传入参数值验证dm-verity table签名完整性，并挂载具有dm-verity功能的rootfs的映射分区，根文件系统为squashfs格式的只读根文件系统。在系统运行过程中，对根文件系统的读操作会读数据进行hash校验，确保读取的数据没有被修改过。
内核在启动后，再initramfs中加载根文件系统，首先进入ramfs针对挂载squashfs格式的根文件系统做准备工作，在内核中根据rootfs镜像和编译时生成的hash tree文件创建映射设备，再将映射设备挂载为新的根文件系统。


## 2.5. 实现流程代码分析
### 2.5.1. 生成带hash tree的根文件系统
编译阶段，编译生成squashfs根文件系统镜像，同时计算镜像的hashtree和root hash数据存储在rootfs的末端中。
```shell
    SETUP_ARGS=" \
        --data-block-size=${DM_VERITY_IMAGE_DATA_BLOCK_SIZE} \
        --hash-block-size=${DM_VERITY_IMAGE_HASH_BLOCK_SIZE} \
        $HASH_OFFSET format $OUTPUT $OUTPUT_HASH \
    "
    echo "veritysetup $SETUP_ARGS" > ${SAVED_ARGS}
    veritysetup $SETUP_ARGS | tail -n +2
```
### 2.5.2. 获取hash table
```shell
#!/bin/bash
DM_VERITY_TXT="${HOME}/rootfs_script_debug/new_dm-verity.env"
RAMFS_INIT="init"
# 从txt文件中读取每个值
data_blocks=$(grep "^DATA_BLOCKS=" "$DM_VERITY_TXT" | cut -d'=' -f2)
data_block_size=$(grep "^DATA_BLOCK_SIZE=" "$DM_VERITY_TXT" | cut -d'=' -f2)
hash_block_size=$(grep "^HASH_BLOCK_SIZE=" "$DM_VERITY_TXT" | cut -d'=' -f2)
hash_algorithm=$(grep "^HASH_ALGORITHM=" "$DM_VERITY_TXT" | cut -d'=' -f2)
root_hash=$(grep "^ROOT_HASH=" "$DM_VERITY_TXT" | cut -d'=' -f2)
data_size=$(grep "^HASH_OFFSET=" "$DM_VERITY_TXT" | cut -d'=' -f2)
# 替换init脚本中的值
sed -i "s/^DATA_BLOCKS=.*/DATA_BLOCKS=$data_blocks/" "$RAMFS_INIT"
sed -i "s/^DATA_BLOCK_SIZE=.*/DATA_BLOCK_SIZE=$data_block_size/" "$RAMFS_INIT"
sed -i "s/^HASH_BLOCK_SIZE=.*/HASH_BLOCK_SIZE=$hash_block_size/" "$RAMFS_INIT"
sed -i "s/^HASH_ALGORITHM=.*/HASH_ALGORITHM=$hash_algorithm/" "$RAMFS_INIT"
sed -i "s/^ROOT_HASH=.*/ROOT_HASH=$root_hash/" "$RAMFS_INIT"
sed -i "s/^HASH_OFFSET=.*/HASH_OFFSET=$data_size/" "$RAMFS_INIT"
echo "数值更新完成"
```
### 2.5.3. dm-verity校验挂载
```shell
# eg: dm-verity
# veritysetup open /dev/mmcblk0p2 vroot /dev/mmcblk0p2 743023cc2f6847fc9a28ac2f682f648899603b6f4e4a4c1b4cbf4a51e81b0f9d \
#             --hash=sha256 --data-block-size=1024 --hash-block-size=4096 --data-blocks=105100  --hash-offset=107622400
veritysetup open    $ROOTFS_DEV  \
                    $VERITY_DEV  \
                    $ROOTFS_DEV  \
                    $ROOT_HASH   \
                    --hash=$HASH_ALGORITHM              \
                    --data-block-size=$DATA_BLOCK_SIZE \
                    --hash-block-size=$HASH_BLOCK_SIZE \
                    --data-blocks=$DATA_BLOCKS          \
                    --hash-offset=$HASH_OFFSET
mount $VERITY_DEV_PATH /mnt
```
## 2.6. 参考文献：
[device-mapper(1)：概述](https://www.cnblogs.com/arnoldlu/p/18845072)
[device-mapper(2)：块设备数据完整性验证功能dm-verity](https://www.cnblogs.com/arnoldlu/p/18882608)
[dm-verity使能](https://geekdaxue.co/read/tiehichi@kernel/aodg7g)