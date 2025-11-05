# 1. 基于网络单板烧写
什么是 NFS 协议?
NFS 实现了一个跨越网络的文件访问功能,如下图可以简要说明其原理。其整个架构为 Client-Server 架构,客户端和服务端通过 RPC 协议进行通信, RPC 协议可以简单的理解为一个基于 TCP 的应用层协议,它简化命令和数据的传输。NFS 最大的特点是将服务端的文件系统目录树映射到客户端,而在客户端访问该目录树与访问本地文件系统没有任何差别,客户端并不知道这个文件系统目录树是本地的还是远在另外一台服务器。


## 1.1. ubuntu-单板网络连接

### 1.1.1. nfs--单板与ubuntu连接

在开发板上执行mount nfs命令：

```shell
[root@100ask:~]# mount -t nfs -o nolock,vers=3 192.168.5.11:/home/book/nfs_rootfs /mnt
```
mount 命令用来挂载各种支持的文件系统协议到某个目录下。
mount 成功之后,开发板在/mnt 目录下读写文件时,实际上访问的就是 Ubuntu 中的/home/book/nfs_rootfs 目录,所以开发板和 Ubuntu 之间通过NFS 可以很方便地共享文件。
在开发过程中,在 Ubuntu 中编译好程序后放入/home/book/nfs_rootfs目录,开发板 mount nfs 后就可以直接通过/mnt 访问 Ubuntu 中的文件。

### 1.1.2. 使用tftp服务
开发板上可以使用tftp命令从ubuntu传输文件。

#### 1.1.2.1. 在ubuntu安装tftp服务
在 Ubuntu 中执行以下命令安装 TFTP 服务:
```shell
sudo apt-get install tftp-hpa tftpd-hpa
# 然后,创建 TFTP 服务器工作目录,并打开 TFTP 服务配置文件,如下:
mkdir -p /home/book/tftpboot
sudo chmod 777 /home/book/tftpboot
sudo vim /etc/default/tftpd-hpa
# 在配置文件/etc/default/tftpd-hpa 中,添加以下字段:
TFTP_DIRECTORY="/home/book/tftpboot"
TFTP_OPTIONS="-l -c -s"
# 最后,重启 TFTP 服务:
sudo service tftpd-hpa restart
# 查看 tftp 服务是否在运行,运行如下命令,即可查看是否在后台运行。
ps -aux | grep “tftp”
```

### 1.1.3. 开发板通过tftp传输文件
比如ubuntu的 /home/book/tftpboot 目录下存在linux内核镜像，下载ubuntu服务器的zImage文件，则在开发板上执行如下命令：
```shell
# 其中IP为ubuntu的网卡IP
$ tftp -g -r zImage 192.168.5.11
# 执行后单板的当前目录存在zImage镜像文件，当前建议在boot目录下执行

# 在开发板执行如下命令上传此文件至ubuntu的tftp服务目录下：
$ tftp -p -l test.txt 192.168.5.11
```

## 1.2. 烧写uboot到emmc

### 1.2.1. nfs服务连接并mount
```shell
# 替换ip，ip为ubuntu的ip
 mount -t nfs -o nolock,vers=3 192.168.5.11:/home/book/nfs_rootfs /mnt
```
### 1.2.2. 编译uboot并放置/home/book/nfs_rootfs
```shell
cp u-boot-dtb.imx /home/book/nfs_rootfs
```
### 1.2.3. 单板复制ubuntu-NFS 目录中的 u-boot-dtb.imx
```shell
cp /mnt/u-boot-dtb.imx .
```
### 1.2.4. 烧写至EMMC
```shell
拷贝成功后,如果要烧写 EMMC,执行如下命令:
[root@100ask:~]# echo 0 > /sys/block/mmcblk1boot0/force_ro # 取消此分区的只读保护
[root@100ask:~]# dd if=u-boot-dtb.imx of=/dev/mmcblk1boot0 bs=512 seek=2 # 实际烧写命令
[root@100ask:~]# echo 1 > /sys/block/mmcblk1boot0/force_ro  # 打开此分区的只读保护
```
烧写完成后,确保开发板设置为 EMMC 启动,可以观察到刚烧写的 U-boot 的启动信息。


### 1.2.5. 开发板更新内核和设备树
通过tftp和nfs，将对应更新的镜像文件放置开发板：boot目录。
reboot重启即可更新。
挂载成功后将已经拷贝到 Ubuntu nfs 目录中的内核、设备树文件拷贝到开
发板/boot 目录下,替换掉原来的文件:
[root@100ask:~]# cp /mnt/zImage /boot
[root@100ask:~]# cp /mnt/100ask_imx6ull-14x14.dtb /boot
[root@100ask:~]# sync


### 1.2.6. 开发板使用NFS根文件系统（更新文件系统）
Buildroot 编译完成之后生成的 rootfs.tar.bz2,可以解压之后放到 NFS 服务器上作为 NFS 文件系统供开发板使用。使用 NFS 文件系统,便于程序的开发调试。所谓 NFS 服务器,就是我们在 VMWare 上运行的 Ubuntu。
使用 NFS 根文件文件系统时,我们一般还会在 u-boot 使用 tftpboot 命令从 Ubuntu 下载内核文件 zImage 和设备树文件。这时,Ubuntu 上既要配置 NFS 服务,也要配置 TFTP 服务。
在 U-Boot 中通过 tftpboot 命令从 Ubuntu/Windows 中下载内核文件zImage、100ask_imx6ull-14x14.dtb , 设置Uboot启动参数使用Ubuntu的某个目录(比如/home/book/nfs_rootfs)作为根文件系统。
把使用buildroot构建得到的根文件系统 rootfs.tar.bz2( 在 buildroot 系 统output/images 目录下),复制、解压到 Ubuntu 的/etc/exports 文件中指定的目录里,即复制到/home/book/nfs_rootfs 目录下,并解压(注意:解压时要用 sudo):


# 2. uboot
单板启动进入uboot方法：
串口连接时，此时将开发板上电，在打印u-boot时按下任意键进入u-boot界面。
## 测试开发板与ubuntu网络是否正常

先在u-boot中设置开发板IP为同网段,然后在 u-boot 中使用 ping 命令测试开发板与 Ubuntu 系统是否连通(出现“alive”就表示联通):命令如下:
```shell
=> setenv ipaddr 192.168.5.9
=> ping 192.168.5.11
```
如果u-boot中提示 host is alive 就表示开发板和 Ubuntu 系统可以互通。

## 使用网络启动文件系统
先 在 Ubuntu 的 TFTP 目 录 中 放 入 zImage 和 设 备 树 文 件 , 再 在
/home/book/nfs_rootfs 目录下解压好根文件系统。
然 后 在 U-Boot 控 制 台 执 行 以 下 命 令 启 动 单 板 , 假 设 Ubuntu IP 是
192.168.5.11:

```shell
=> setenv serverip 192.168.5.11
 # 设置服务器的 IP 地址,这里指的是 Ubuntu 主机 IP
=> setenv ipaddr 192.168.5.9
 # 设置开发板的 IP 地址。
=> setenv nfsroot /home/book/nfs_rootfs
 # 设置 nfs 文件系统所在目录。
=> run netboot
 # 设置完成后,运行网络启动系统命令
```

使用tftp在u-boot环境更新linux内核和设备树，但是需要tftp指定地址。

例如最终实现命令：
```shell
setenv bootargs 'console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw'
setenv bootcmd 'tftp 80800000 zImage; tftp 83000000 imx6ull-alientek-emmc.dtb; bootz
80800000 - 83000000'
saveenv

# boot 目录起：
$ setenv bootargs 'console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw'
$ setenv bootcmd 'mmc dev 1; ext4load mmc 1:2 80800000 boot/zImage; ext4load mmc 1:2 83000000 boot/100ask_imx6ull-14x14.dtb; bootz 80800000 - 83000000;'
$ saveenv
```

问题1：为什么会更新boot目录下的镜像？
A： boot命令参数设置？


参考文章：
[通过tftp烧录linux内核](https://blog.csdn.net/qq_36347513/article/details/127479539)
[移植原厂uboot](https://zhuanlan.zhihu.com/p/338691518)

备注： 当前更新设备树和linux内核即可，学习linux驱动及主要编译构建方法。
uboot相关技术，更应该看做一个裸机程序，去理解arm架构。
