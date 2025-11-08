export CROSS_PATH=/home/yujuncheng/data/rk_3568_510/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin
KERNELDIR := /home/yujuncheng/data/rk_3568_510/kernel
# 目标架构为ARM64
export ARCH := arm64
export CROSS_COMPILE=$(CROSS_PATH)/aarch64-none-linux-gnu-

obj-m += boot_profiler.o
obj-m += boot_profiler_drv.o

CURRENT_PATH := $(shell pwd)
PWD := $(shell pwd)

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
