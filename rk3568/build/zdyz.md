

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