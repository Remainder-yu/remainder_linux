## 编译构建

### 编译前说明

### qemu


```shell
sudo ip tuntap add dev tap_yujuncheng mode tap user yujuncheng
sudo ifconfig tap_yujuncheng 192.168.3.10 up

```

run_qemu.sh

```shell
#!/bin/sh
CUR_DIR=`pwd`
qemutool="qemu-system-aarch64"
netdevice="-device virtio-net-device,netdev=net0,mac=52:54:00:12:34:04 -netdev tap,id=net0,ifname=tap_yujuncheng,script=no,downscript=no"
kernel="${CUR_DIR}/Image"
kernel_params="root=/dev/vda rw mem=1024M ip=192.168.3.100::192.168.3.99:255.255.255.0 console=ttyAMA0 console=hvc0"
 
qemu_params=""
gdb="false"
 
usage() {
    echo "Usage:"
    echo "runqemu.sh [-r rootfs_ext4] [-k kernel_params] [-g with_gdb]"
    echo "eg: runqemu.sh -r xxxx/rootfs_ext4 -k nokaslr -g"
    exit -1
}
 
while getopts 'r:k:g' OPT; do
    case $OPT in
        r) rootfs="$OPTARG";;
        k) kernel_params=$kernel_params"$OPTARG";;
        g) qemu_params="-S -s";;
        h) usage;;
        ?) usage;;
    esac
done
 
echo "start qemuarm64 use rootfs: $rootfs"
echo "kernel: $kernel"
echo "kernel params: $kernel_params"
echo "qemu params: $qemu_params"
 
echo "cmdline: $qemutool $netdevice -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 -drive id=disk0,file=$rootfs,if=none,format=raw -device virtio-blk-device,drive=disk0 -device qemu-xhci -device usb-tablet -device usb-kbd  -machine virt -cpu cortex-a53 -smp 4 -m 256 -serial mon:stdio -serial null -nographic -device virtio-gpu-pci -kernel $kernel -append "$kernel_params" $qemu_params"
 
$qemutool $netdevice -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 -drive id=disk0,file=$rootfs,if=none,format=raw -device virtio-blk-device,drive=disk0 -device qemu-xhci -device usb-tablet -device usb-kbd  -machine virt -cpu cortex-a53 -smp 4 -m 1024 -serial mon:stdio -serial null -nographic -device virtio-gpu-pci -kernel $kernel -append "$kernel_params" $qemu_params


# yujuncheng123
```


