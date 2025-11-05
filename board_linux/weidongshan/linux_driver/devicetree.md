# 编译调试：

```bash
dtc -I dtb -O dts 100ask_imx6ull_qemu.dtb -o 100ask_imx6ull_qemu_re.dts
dtc -I dts -O dtb 100ask_imx6ull_qemu.dts -o 100ask_imx6ull_qemu_re.dtb


dtc -I dtb -O dts vexpress-v2p-ca9.dtb -o vexpress-v2p-ca9_0603.dts
dtc -I dts -O dtb vexpress-v2p-ca9_0603.dts -o vexpress-v2p-ca9_0603.dtb

# 100ask_board
arch/arm/boot/dts/100ask_imx6ull-14x14.dts

```

启动linux：
```bash
#!/bin/bash

workdir=$(dirname $0);

        $workdir/qemu/bin/qemu-system-arm -M mcimx6ul-evk    -m 512M -kernel $workdir/imx6ull-system-image/zImage \
                -dtb $workdir/imx6ull-system-image/100ask_imx6ull_qemu_re.dtb  \
 -nographic -serial mon:stdio \
 -drive  file=$workdir/imx6ull-system-image/rootfs.img,format=raw,id=mysdcard -device sd-card,drive=mysdcard \
 -append "console=ttymxc0,115200 rootfstype=ext4 root=/dev/mmcblk1  rw rootwait init=/sbin/init  loglevel=8" \
 -nic user 

```


## 1. 驱动程序的三种方法：
1. 编码
2. 使用总线设备驱动模型
3. 使用设备树

这三种的差别：如何指定硬件资源，比如如何指定LED引脚是哪个。

1. 传统方法：在驱动程序中写死硬件资源，代码简单/不易扩展
2. 总线设备驱动模型：把驱动分为两部分（platform_driver,platform_device）
    在platfrom_driver中指定硬件资源
    在platform_driver中分配/设置/注册file_operations结构体，并从platform_device获取硬件资源

   特点：
    易于扩展，但是有很多冗余代码(每种配置都对应一个platform_device结构体), 
    硬件有变动时需要重新编译内核或驱动程序。
3. 使用设备树指定硬件资源: 驱动程序也分为两部分(platform_driver, 设备树*.dts)
    在设备树*.dts中指定硬件资源, dts被编译为dtb文件, 在启动单板时会将dtb文件传给内核,
    内核根据dtb文件分配/设置/注册多个platform_device
    
    platform_driver的编写方法跟"总线设备驱动模型"一样。
    
    特点：
    易于扩展，没有冗余代码
    硬件有变动时不需要重新编译内核或驱动程序，只需要提供不一样的dtb文件

    注: dts  - device tree source  // 设备树源文件
        dtb  - device tree blob    // 设备树二进制文件, 由dts编译得来
        blob - binary large object

实际上platform_device也可以来自设备树文件.dts
   dts文件被编译为dtb文件, 
   dtb文件会传给内核, 
   内核会解析dtb文件, 构造出一系列的device_node结构体,
   device_node结构体会转换为platform_device结构体

   所以: 我们可以在dts文件中指定资源, 不再需要在.c文件中设置platform_device结构体

"来自dts的platform_device结构体" 与 "我们写的platform_driver" 的匹配过程:
    "来自dts的platform_device结构体"里面有成员".dev.of_node", 它里面含有各种属性, 比如 compatible, reg, pin
    "我们写的platform_driver"里面有成员".driver.of_match_table", 它表示能支持哪些来自于dts的platform_device
    
    如果"of_node中的compatible" 跟 "of_match_table中的compatible" 一致, 就表示匹配成功, 则调用 platform_driver中的probe函数;
    在probe函数中, 可以继续从of_node中获得各种属性来确定硬件资源

## 2. 设备树描述

```bash

```

## 3. 内核对设备树的处理


