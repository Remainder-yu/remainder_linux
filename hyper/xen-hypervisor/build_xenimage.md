# xen_images
编译步骤
要交叉编译ARM版本的dom0镜像，你可以使用Xen项目提供的工具链和Yocto Project进行构建。以下是一个基本的步骤指南：

参考：https://wiki.xenproject.org/wiki/Xen_on_ARM_and_Yocto

http://www.prtos.org/xen_on_arm64_and_qemu/

1. 安装工具链：
首先，需要安装Xen项目提供的ARM交叉编译工具链，用于构建ARM目标系统。你可以从Xen项目的官方网站或GitHub仓库获取工具链，并按照其提供的安装说明进行安装。

$ git clone -b dunfell http://git.yoctoproject.org/git/poky
$ cd poky
$ git clone -b dunfell http://git.openembedded.org/meta-openembedded
$ git clone -b dunfell http://git.yoctoproject.org/git/meta-virtualization
2. 设置环境：
进入Yocto工作目录并执行source poky/oe-init-build-env命令，以初始化构建环境。

3. Edit conf/bblayers.conf
Edit conf/bblayers.conf, to add the source code layers needed. /scratch/repos/poky is the directory where you cloned the poky source code to - you will need to adjust the example paths here to match your directory layout:

BBLAYERS ?= " \
/scratch/repos/poky/meta \
/scratch/repos/poky/meta-poky \
/scratch/repos/poky/meta-yocto-bsp \
/scratch/repos/poky/meta-openembedded/meta-oe \
/scratch/repos/poky/meta-openembedded/meta-filesystems \
/scratch/repos/poky/meta-openembedded/meta-python \
/scratch/repos/poky/meta-openembedded/meta-networking \
/scratch/repos/poky/meta-virtualization \
 "
4. 配置构建：
打开conf/local.conf文件，在其中进行以下配置：

设置MACHINE变量为目标ARM硬件平台的名称，例如MACHINE = "raspberrypi4-64"。

设置TOOLCHAIN变量，指定为你安装的ARM交叉编译工具链的路径。

根据需要，进行其他配置，如设置软件包、选项等。



The conf/local.conf file contains instructions for the variables that it sets. You should review and make sure to set:

DL_DIR           -- set this to a local download directory for retrieved tarballs or other source code files
SSTATE_DIR       -- set to a local directory for build cache files to speed up subsequent builds
PACKAGE_CLASSES  -- package_ipk can be a good choice for package format
Then add the following to the same file, or make sure that the values here match if the variables are already present in the file:

MACHINE = "qemuarm64"
DISTRO = "poky"
IMAGE_FSTYPES += "cpio.gz"
QEMU_TARGETS = "i386 aarch64"
DISTRO_FEATURES += " virtualization xen"
BUILD_REPRODUCIBLE_BINARIES = "1"
This configuration will enable OpenEmbedded's support for reproducible builds. It also reduces the number of emulation platforms for QEMU to significantly reduce build time.

If you would like to build QEMU to provide PV backends, such as disk and 9pfs, then you need to add:

PACKAGECONFIG_pn-qemu += " virtfs xen fdt"
Sdl is enabled by default in the Xen build of QEMU but it is not actually necessary and can be disabled with:

PACKAGECONFIG_remove_pn-qemu += " sdl"


5. 添加dom0组件：
你可以通过添加所需的dom0功能组件的层来进行配置。

打开conf/bblayers.conf文件，在其中添加dom0功能组件的层路径。



6. 开始构建：
在终端中执行

bitbake xen-image-minimal命令，以开始构建dom0镜像。

构建过程可能需要一些时间，具体时间取决于硬件和网络速度。

构建输出：

构建完成后，生成的ARM版本的dom0镜像位于build目录下的tmp/deploy/images目录中。



在云桌面实际编译流程：
# 第一步：拷贝源文件至media：

```shell

source ./poky/oe-init-build-env

bblayers.conf
/media/xenonarm/poky/meta-openembedded/meta-oe \
/media/xenonarm/poky/meta-openembedded/meta-filesystems \
/media/xenonarm/poky/meta-openembedded/meta-python \
/media/xenonarm/poky/meta-openembedded/meta-networking \
/media/xenonarm/poky/meta-virtualization \

/home/remainder/xenonarm/
/home/remainder/xenonarm/poky/meta-openembedded/meta-oe \
/home/remainder/xenonarm/poky/meta-openembedded/meta-filesystems \
/home/remainder/xenonarm/poky/meta-openembedded/meta-python \
/home/remainder/xenonarm/poky/meta-openembedded/meta-networking \
/home/remainder/xenonarm/poky/meta-virtualization \

local.conf

MACHINE = "qemuarm64"
DISTRO = "poky"
IMAGE_FSTYPES += "cpio.gz"
QEMU_TARGETS = "x86_64 aarch64"
DISTRO_FEATURES += " virtualization xen"
BUILD_REPRODUCIBLE_BINARIES = "1"
PACKAGECONFIG_pn-qemu += " virtfs xen fdt"
PACKAGECONFIG_remove_pn-qemu += " sdl"
IMAGE_INSTALL_append = " openssl"
TOOLCHAIN = "/media/xenonarm/aarch64_eabi_gcc9.2.0_glibc2.31.0_fp/bin"

DL_DIR = "/home/remainder/xenonarm/downloads"
SSTATE_DIR = "/home/remainder/xenonarm/sstate-cache"
PACKAGE_CLASSES = "package_ipk"

# INHERIT_pn-xen += "externalsrc"
# INHERIT_pn-xen-tools += "externalsrc"
# EXTERNALSRC_pn-xen = "/media/xen"
# EXTERNALSRC_pn-xen-tools = "/media/xen"
# EXTERNALSRC_BUILD_pn-xen = "/media/xen"
# EXTERNALSRC_BUILD_pn-xen-tools = "/media/xen"

```

在/media/xenonarm/poky/build/conf执行：
bitbake xen-image-minimal

## xenonarm1

```shell

source ./poky/oe-init-build-env

bblayers.conf
/media/xenonarm/poky/meta-openembedded/meta-oe \
/media/xenonarm/poky/meta-openembedded/meta-filesystems \
/media/xenonarm/poky/meta-openembedded/meta-python \
/media/xenonarm/poky/meta-openembedded/meta-networking \
/media/xenonarm/poky/meta-virtualization \

/home/remainder/xenonarm/
/home/remainder/xenonarm1/poky/meta-openembedded/meta-oe \
/home/remainder/xenonarm1/poky/meta-openembedded/meta-filesystems \
/home/remainder/xenonarm1/poky/meta-openembedded/meta-python \
/home/remainder/xenonarm1/poky/meta-openembedded/meta-networking \
/home/remainder/xenonarm1/poky/meta-virtualization \
# /mydisk

/home/remainder/xenonarm1/mydisk/poky/meta-openembedded/meta-oe \
/home/remainder/xenonarm1/mydisk/poky/meta-openembedded/meta-filesystems \
/home/remainder/xenonarm1/mydisk/poky/meta-openembedded/meta-python \
/home/remainder/xenonarm1/mydisk/poky/meta-openembedded/meta-networking \
/home/remainder/xenonarm1/mydisk/poky/meta-virtualization \

local.conf

MACHINE = "qemuarm64"
DISTRO = "poky"
IMAGE_FSTYPES += "cpio.gz"
QEMU_TARGETS = "i386 aarch64"
DISTRO_FEATURES += " virtualization xen"
BUILD_REPRODUCIBLE_BINARIES = "1"
PACKAGECONFIG_pn-qemu += " virtfs xen fdt"
PACKAGECONFIG_remove_pn-qemu += " sdl"
IMAGE_INSTALL_append = " openssl"
# TOOLCHAIN = "/media/xenonarm/aarch64_eabi_gcc9.2.0_glibc2.31.0_fp/bin"

DL_DIR = "/home/remainder/xenonarm1/downloads"
SSTATE_DIR = "/home/remainder/xenonarm1/sstate-cache"
PACKAGE_CLASSES = "package_ipk"

# INHERIT_pn-xen += "externalsrc"
# INHERIT_pn-xen-tools += "externalsrc"
# EXTERNALSRC_pn-xen = "/media/xen"
# EXTERNALSRC_pn-xen-tools = "/media/xen"
# EXTERNALSRC_BUILD_pn-xen = "/media/xen"
# EXTERNALSRC_BUILD_pn-xen-tools = "/media/xen"

```

在/media/xenonarm/poky/build/conf执行：
bitbake xen-image-minimal


# xen-image：

bblayers.conf
/home/yujuncheng/remainder/
/home/yujuncheng/remainder/poky/meta-openembedded/meta-oe \
/home/yujuncheng/remainder/poky/meta-openembedded/meta-filesystems \
/home/yujuncheng/remainder/poky/meta-openembedded/meta-python \
/home/yujuncheng/remainder/poky/meta-openembedded/meta-networking \
/home/yujuncheng/remainder/poky/meta-virtualization \


local.conf

```shell
MACHINE = "qemuarm64"
DISTRO = "poky"
IMAGE_FSTYPES += "cpio.gz"
QEMU_TARGETS = "i386 aarch64"
DISTRO_FEATURES += " virtualization xen"
BUILD_REPRODUCIBLE_BINARIES = "1"
PACKAGECONFIG_pn-qemu += " virtfs xen fdt"
PACKAGECONFIG_remove_pn-qemu += " sdl"
IMAGE_INSTALL_append = " openssl"

DL_DIR = "/home/yujuncheng/remainder/downloads"
SSTATE_DIR = "/home/yujuncheng/remainder/sstate-cache"
PACKAGE_CLASSES = "package_ipk"
SERIAL_CONSOLES_CHECK = "${SERIAL_CONSOLES}"

INHERIT_pn-xen += "externalsrc"
INHERIT_pn-xen-tools += "externalsrc"
EXTERNALSRC_pn-xen = "/home/yujuncheng/remainder/xen"
EXTERNALSRC_pn-xen-tools = "/home/yujuncheng/remainder/xen"
EXTERNALSRC_BUILD_pn-xen = "/home/yujuncheng/remainder/xen"
EXTERNALSRC_BUILD_pn-xen-tools = "/home/yujuncheng/remainder/xen"

bitbake xen-image
```



最新分支：
Scarthgap

[yocto项目release](https://wiki.yoctoproject.org/wiki/Releases)


```shell
# file://0001-python-pygrub-pass-DISTUTILS-xen.4.18.patch \
poky/meta-virtualization/recipes-extended/xen/xen-tools_git.bb 

file://xen-tools-update-python-scripts-to-py3.patch \
    file://xen-tools-libxl-gentypes-py3.patch \
    file://xen-tools-python-fix-Wsign-compare-warnings.patch \
    file://xen-tools-pygrub-change-tabs-into-spaces.patch \
    file://xen-tools-pygrub-make-python-scripts-work-with-2.6-and-up.patch \
    file://xen-tools-pygrub-py3.patch \
```