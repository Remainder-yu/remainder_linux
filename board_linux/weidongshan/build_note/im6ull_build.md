
交叉编译工具链环境变量设置：
```bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export PATH=$PATH:/home/yujuncheng/learn_remainder/weidongshan/100ask_imx6ull-qemu/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin
```

https://gitee.com/weidongshan/manifests


# uboot编译：
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

#!/bin/bash
# configs/100ask_imx6ull_defconfig
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export PATH=$PATH:/home/yujuncheng/data/manifests/100ask_imx6ull_old/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CROSS_COMPILE=arm-linux-gnueabihf- distclean
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CROSS_COMPILE=arm-linux-gnueabihf- 100ask_imx6ull_pro_defconfig
make -s ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j16

```

# linux编译：
```shell
# /home/yujuncheng/remainder_linux_learn/weidongshan/ToolChain
book@100ask:~/100ask_imx6ull-qemu$ cd linux-4.9.88
book@100ask:~/100ask_imx6ull-qemu/linux-4.9.88$ make mrproper
book@100ask:~/100ask_imx6ull-qemu/linux-4.9.88$ make 100ask_imx6ull_qemu_defconfig
book@100ask:~/100ask_imx6ull-qemu/linux-4.9.88$ make zImage -jN //编译zImage 内核镜像，其中N参数可以根据CPU个数，来加速编译系统。
book@100ask:~/100ask_imx6ull-qemu/linux-4.9.88$ make dtbs   //编译设备树文件
make 100ask_imx6ull_defconfig

#!/bin/bash
# configs/mx6ul_14x14_evk_emmc_defconfig
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export PATH=$PATH:/home/yujuncheng/remainder_linux_learn/weidongshan/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin

make mrproper
cp /home/yujuncheng/remainder_linux_learn/qemu-arm-mcimx6ul-evk/configs/linux_v5.4_defconfig .config
make linux_v5.4_defconfig
make zImage -j36

#!/bin/bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export PATH=$PATH:/home/yujuncheng/data/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin

make mrproper
make 100ask_imx6ull_defconfig
make zImage -j36
```

# qemu-imx6ull官方

```shell
# 仓库 ，该仓库基于开源的qemu即可运行
https://gitee.com/pingwuu/qemu-arm-mcimx6ul-evk.git

# qemu启动时，对镜像大小报错，修改为2的倍数
qemu-img resize rootfs.img 512M

```

# 165

```shell
# /home2/yujuncheng/linux_build_and_weidongshan/100ask_imx6ull-sdk

#!/bin/bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export PATH=$PATH:/home2/yujuncheng/linux_build_and_weidongshan/100ask_imx6ull-sdk/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin

# make mrproper
# cp /home/yujuncheng/remainder_linux_learn/qemu-arm-mcimx6ul-evk/configs/linux_v5.4_defconfig .config
make linux_v5.4_defconfig
make zImage -j36
```

# imx6ull-100ask-board

```shell
make 100ask_imx6ull_defconfig
/home/yujuncheng/data/manifests/100ask_imx6ull_old/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf

#!/bin/bash
# configs/mx6ul_14x14_evk_emmc_defconfig
CROSS_PATH="/home/yujuncheng/data/manifests/100ask_imx6ull_old/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf"
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export PATH=$PATH:$CROSS_PATH/bin
make mrproper
make 100ask_imx6ull_defconfig
make zImage -j8

git clone https://e.coding.net/codebug8/repo.git
mkdir -p 100ask_imx6ull-sdk && cd 100ask_imx6ull-sdk
../repo/repo init -u https://gitee.com/weidongshan/manifests.git -b linux-sdk -m imx6ull/100ask_imx6ull_linux4.9.88_release.xml --no-repo-verify
../repo/repo sync -j4


/home/yujuncheng/data/manifests/100ask_imx6ull_old/Linux-4.9.88
```