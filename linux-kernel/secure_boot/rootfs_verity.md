根文件系统主要依赖内核Device Mapper及Dm-verity进行验证，下面进行详细描述。
![alt text](./image/image-2.png)
它包含三个重要的对象概念，Mapped Device、Mapping Table、Target device。

Mapped Device：是一个抽象的逻辑设备，通过Mapping Table描述的映射关系（Mapped Device逻辑的起始地址、范围和表示在Target Device所在物理设备的地址偏移量以及Target类型等信息）和Target Device建立映射关系。
Target Device：是Mapped Device所映射的物理空间段。
Mapping Rable：DeviceMapper在内核中通过一个一个模块化的target Driver插件实现对IO请求的过滤或者重新定向等工作，当前已经实现的插件包括：dm-drypt、dm-linear、dm-verity、dm-bow、dm-raid等。
通过下述三个主要步骤，device mapper在内核中就建立一个可以提供给用户使用的mapped device逻辑块设备。

![alt text](./image/image-3.png)

对映射设备的读操作被映射成对目标设备的读操作，在

## linux DM-verity架构
Dm-verity 是 device-mapper 架构下的一个目标设备类型， 通过它来保障设备或者设备分区的完整性。
Dm-verity类型的目标设备有两个底层设备，一个是数据设备(data device), 是用来存储实际数据的，另一个是hash设备(hash device), 用来存储hash数据的，这个是用来校验data device数据的完整性的。其架构图如下所示：
![alt text](./image/image-4.png)
图中映射设备(Mapper Device)和目标设备(Target Device)是一对一关系，对映射设备的读操作被映射成对目标设备的读操作，在目标设备中，dm-verity又将读操作映射为数据设备（Data Device）的读操作。但是在读操作的结束处，dm-verity加了一个额外的校验操作，对读到的数据计算一个hash值，用这个哈希值和存储在哈希设备(Hash Device)的值进行比较，如果不同，则本次读操作被标记为错误。

## 支持dm-verity Rootfs镜像构建
综合体积以及安全性考虑，本系统使用的根文件系统为squash rootfs。为了支持文件系统的安全启动及挂载读取过程中放置数据被恶意篡改，需要制作特殊支持dm-verity的rootfs镜像，其制作流程如下所示：


## rootfs镜像验证
roothash放在boot-param.bin中，boot-param.bin则由uboot进行加载，验证过程中由HSE/HSM执行。uboot通过访问HSE/HSM对boot-param.bin验签来校验rootfs根哈希是否可信，校验通过后进行加载启动内核及设备树。
rootfs由kernel进行加载，Kernel通过dm-verity机制并根据uboot传入参数值验证dm-verity table签名完整性，并挂载具有dm-verity功能的rootfs的映射分区，根文件系统为squashfs格式的只读根文件系统。在系统运行过程中，对根文件系统的读操作会对读数据进行hash校验，确保读取的数据没有被修改过。
内核启动后在内核里加载根文件系统，首先进入内核为挂载squashfs格式的根文件系统做准备工作，在内核中，根据rootfs镜像和编译时生成的hashtree文件创建映射设备，再将映射设备挂载为新的根文件系统，流程如下图所示：

现在需要uboot校验roothash，内核加载rootfs hashtree，校验rootfs  hashtree，创建rootfs mapping device ，mountrootfs。
