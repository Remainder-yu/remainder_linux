

## 编译
```shell
./build.sh lunch
./build.sh kernel

# 详细案例
yujuncheng@ubuntu:~/rk3568_linux_sdk$ ./build.sh lunch
processing option: lunch

You're building on Linux
Lunch menu...pick a combo:

0. default BoardConfig.mk
1. BoardConfig-rk3566-evb2-lp4x-v10-32bit.mk
2. BoardConfig-rk3566-evb2-lp4x-v10.mk
3. BoardConfig-rk3568-atk-atompi-ca1.mk
4. BoardConfig-rk3568-atk-evb1-ddr4-v10.mk
5. BoardConfig-rk3568-evb1-ddr4-v10-32bit.mk
6. BoardConfig-rk3568-evb1-ddr4-v10-spi-nor-64M.mk
7. BoardConfig-rk3568-evb1-ddr4-v10.mk
8. BoardConfig-rk3568-nvr-spi-nand.mk
9. BoardConfig-rk3568-nvr.mk
10. BoardConfig-rk3568-uvc-evb1-ddr4-v10.mk
11. BoardConfig.mk
Which would you like? [0]: 4
switching to board: /home/yujuncheng/rk3568_linux_sdk/device/rockchip/rk356x/BoardConfig-rk3568-atk-evb1-ddr4-v10.mk
yujuncheng@ubuntu:~/rk3568_linux_sdk$ ./build.sh kernel 

'
# 编写5.10内核的代码方式
yujuncheng@ubuntu:~/data/rk_3568_510$ ./build.sh lunch

############### Rockchip Linux SDK ###############

Manifest: atk-rk3568_linux5.10_release_v1.2_20250104.xml

Log saved at /home/yujuncheng/data/rk_3568_510/output/sessions/2025-11-05_22-33-24

Pick a defconfig:

1. rockchip_defconfig
2. alientek_rk3568_defconfig
3. rockchip_rk3566_evb2_lp4x_v10_32bit_defconfig
4. rockchip_rk3566_evb2_lp4x_v10_defconfig
5. rockchip_rk3568_evb1_ddr4_v10_32bit_defconfig
6. rockchip_rk3568_evb1_ddr4_v10_defconfig
7. rockchip_rk3568_evb8_lp4_v10_32bit_defconfig
8. rockchip_rk3568_evb8_lp4_v10_defconfig
9. rockchip_rk3568_uvc_evb1_ddr4_v10_defconfig
Which would you like? [1]: 2
Switching to defconfig: /home/yujuncheng/data/rk_3568_510/device/rockchip/.chip/alientek_rk3568_defconfig
make: Entering directory '/home/yujuncheng/data/rk_3568_510/device/rockchip/common'
#
# configuration written to /home/yujuncheng/data/rk_3568_510/output/.config
#
make: Leaving directory '/home/yujuncheng/data/rk_3568_510/device/rockchip/common'

# 选择alientek_rk3568_defconfig配置即可，编译指定镜像或者全编译

```


## 内核镜像烧写--boot.img

```shell
# uboot
setenv serverip 192.168.3.10
setenv ipaddr 192.168.3.100
tftp 0x30000000 boot.img
boot_fit 0x30000000
# tftp文件夹： /home/yujuncheng/tftpboot
# ubuntu传输文件至rk3568
adb push file /root/
# 网络传输：scp 密码root
```

## ubuntu环境工具烧写
1. 连接OTG数据线束
2. 进入Maskrom模式
3. 执行烧写命令或脚本

```shell
# 进入Maskrom模式
# OTG连线接好，按住开发板update按键，然后给开发板上电或复位，然后进行烧写

# 单个镜像烧写
sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool UL MiniLoaderAll.bin -noreset
/home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool
sudo /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool UL MiniLoaderAll.bin -noreset
sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool DI -p parameter.txt

sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool DI -uboot uboot.img
sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool DI -misc misc.img
sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool DI -boot boot.img
sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool DI -recovery recovery.img
sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool DI -oem oem.img
sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool DI -rootfs rootfs.img
sudo ../tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/upgrade_tool DI -userdata userdata.img

# 脚本烧写
yujuncheng@ubuntu:~/data/rk_3568_510$ sudo ./rkflash.sh 
flash all images as default
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
Loading loader...
Support Type:RK3568     Loader ver:1.01 Loader Time:2025-11-05 21:51:11
Start to upgrade loader...
Download Boot Start
Download Boot Success
Wait For Maskrom Start
Wait For Maskrom Success
Test Device Start
Test Device Success
Check Chip Start
Check Chip Success
Get FlashInfo Start
Get FlashInfo Success
Prepare IDB Start
Prepare IDB Success
Download IDB Start
Download IDB Success
Upgrade loader ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
directlba=1,first4access=1,gpt=1
Write gpt...
Write gpt ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
directlba=1,first4access=1,gpt=1
Download uboot start...(0x00004000)
Write file...(100%)
Download image ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
check download item failed!
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
directlba=1,first4access=1,gpt=1
Download boot start...(0x00008000)
Write file...(100%)
Download image ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
directlba=1,first4access=1,gpt=1
Download recovery start...(0x00028000)
Write file...(100%)
Download image ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
directlba=1,first4access=1,gpt=1
Download misc start...(0x00006000)
Write file...(100%)
Download image ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
directlba=1,first4access=1,gpt=1
Download oem start...(0x00c78000)
Write file...(100%)
Download image ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
directlba=1,first4access=1,gpt=1
Download userdata start...(0x00cb8000)
Write file...(100%)
Download image ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
directlba=1,first4access=1,gpt=1
Download rootfs start...(0x00078000)
Write file...(100%)
Download image ok.
Using /home/yujuncheng/data/rk_3568_510/tools/linux/Linux_Upgrade_Tool/Linux_Upgrade_Tool/config.ini
Reset Device OK.
```