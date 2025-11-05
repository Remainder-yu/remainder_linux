## led 


```shell
# 加载模块
root@ATK-DLRK3568:/userdata# insmod led.ko
root@ATK-DLRK3568:/userdata# lsmod
Module                  Size  Used by
led                    16384  0
hci_uart               45056  1
8852bs               3862528  0
# 驱动加载成功后创建/dev/led节点
root@ATK-DLRK3568:/userdata# mknod /dev/led c 200 0
root@ATK-DLRK3568:/userdata# lsmod
Module                  Size  Used by
led                    16384  0
hci_uart               45056  1
8852bs               3862528  0
root@ATK-DLRK3568:/userdata# ls /dev/l
led           loop-control  loop2         loop5         
lightsensor   loop0         loop3         loop6         
log           loop1         loop4         loop7         
# 关闭心跳灯
root@ATK-DLRK3568:/userdata# echo none > /sys/class/leds/work/trigger
root@ATK-DLRK3568:/userdata# chmod 777 ledApp
# 打开LED
root@ATK-DLRK3568:/userdata# ./ledApp /dev/led 1
# 关闭LED
root@ATK-DLRK3568:/userdata# ./ledApp /dev/led 0
```