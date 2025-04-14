# 1. bitbake

bitbake执行第一步读取bb文件：`yocto/meta-soc/meta-xxx/meta-SOC-version/recipes-core/images/core-image-SOC-version.bb`

举例分析：

```shell
yocto/poky/meta/recipes-extended/images/core-image-xxx.bb
IMAGE_FEATURES += "splash ssh-server-openssh"
# 这些功能是什么加上的？
```

## 1.1. bitbake命令
会在deploy中路径下输出manifest文件，这些文件里面对应的文件系统包含的软件包。
bitbake  target :

## 1.2. 构建过程的主要流程分析
bitbake构建的第一步是解析Metedata基本配置文件，这些基本配置文件确定了所构建嵌入式Linux系统发行版的一些功能及特征。
### 1.2.1. Metadata
yocto是由一些metadata组成的，在source目录下，有一些文件夹，这些文件夹就是一个个metadata。例如目录：yocto/meta-soc/meta-nxp/meta-freescale。
用于构建嵌入式linux系统。
针对不同的版本，需要不同的metadata，所以在source bitbake之后，就会创建一个build文件夹，在build/conf路径下，生成了一个bblayers.conf的配置文件，bitbake工具会根据bblayers.conf文件中的定义，确定使用哪些metadata用于构建linux发行版。

```shell
SOC-version:
remainder@ubuntu2024:~/release_gp/path_build_linux$ cat ./work/build_m1/B1/update/conf/bblayers.conf
# POKY_BBLAYERS_CONF_VERSION is increased each time build/conf/bblayers.conf
# changes incompatibly
POKY_BBLAYERS_CONF_VERSION = "2"

BBPATH = "${TOPDIR}"
BBFILES ?= ""

BBLAYERS ?= " \
  /home/remainder/release_gp/path_build_linux/yocto/poky/meta \
  /home/remainder/release_gp/path_build_linux/yocto/poky/meta-poky \
  /home/remainder/release_gp/path_build_linux/yocto/poky/meta-yocto-bsp \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-linaro/meta-optee \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-filesystems \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-gnome \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-multimedia \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-networking \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-oe \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-perl \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-python \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-python2 \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-webserver \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-openembedded/meta-xfce \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-security \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-selinux \
  /home/remainder/release_gp/path_build_linux/yocto/meta-community/meta-virtualization \
  /home/remainder/release_gp/path_build_linux/yocto/meta-soc/meta-xxx/meta-SOC-version \
  /home/remainder/release_gp/path_build_linux/yocto/meta-extended/meta-xxx-core/meta-prebuilt-toolchain \
  /home/remainder/release_gp/path_build_linux/yocto/meta-extended/meta-xxx-core/meta-xxx \
  "

```

![alt text](./picture/yocto-system.png)

直白来说就是对应的每个目录，写入到bblayer中，然后这些目录参与编译。

针对recipes\conf\classes:

- classes:该文件夹下的.bbclass文件，包含了一些在不同的metadata之间的共享信息，以及编译信息的抽象，例如：如何编译linux内核
- conf：该conf文件夹下的layer.conf文件定义了该metadata中使用的哪些.bb、.bbappend文件等参与构建嵌入式linux系统
- recipes-xxx：该文件夹中有许多的.bb或.bbappend文件，这些文件定义了构建嵌入式linux系统需要的软件包和源码等，主要包括信息：
  - 软件包信息
  - 版本信息
  - 依赖文件
  - 下载源，以及如何下载软件包

bb文件中常用就是，把当前目录下的bb文件都找到，然后参与整个构建过程。

metadata解析过程，Bitbake根据build/conf/bblayers.conf中定义的所使能的layers（meta-xxx）,找到对应的meta-xxx文件下的layers.conf文件，根据layer.conf文件中所使用的.bb或.bbappend文件中定义的软件包和源码下载、配置、编译方式、安装目录等，将需要的软件包或源码编译进根文件系统。

可以通过bitbake提供的命令查看当前使用的配置文件和class文件：
`bitbake -e > mybb.log`

### 1.2.2. 解析recipes

### 1.2.3. Providers供应者


### 1.2.4. Preferences优先级

对于确定所使用的目标recipe，PROVIDES列表仅仅是其中的一部分。由于目标recipe有可能会很多个PROVIDES，bitbake通过识别PROVIDES的优先级确定使用的provides。
优先级的对比

### 1.2.5. Dependencies（依赖关系）

### 1.2.6. 任务列表

### 1.2.7. 执行任务

### 1.2.8. 校验（签名）

# 2. Yocto开发环境
定制化，先参考官方平台。
## 2.1. yocto系统组成
yocto系统框图：
- User Configuration：
- Metadata Layers:
- Source file
- Build system
- Images
- 交叉编译工具链


开发过程中，主要修改编译封装的标准，不应该去修改*.conf文件，对项目完整性保证。
local.conf提供了一些基本的变量，定义了构建环境，例如：MACHINE\DL_DIR，这些变量可以通过脚本确定设置。


### 2.1.1. metadata
元数据，是Bitbake构建嵌入式linux系统过程中所使用的最基础的数据。
Metadata一定程度上说就是指的Layers，两个概念在一定的场景下表示的同一个含义。Metadata具体表现为一个meta-xxx文件夹。

源码：Upstream Project Release、Local Projects、SCMs。

在每个recipe中存在一个SRC_URI变量，该变量定义了recipes所使用到的软件包或源码地址。
Bitbake就是通过SRC_URI所指向地址获取相应的软件包或源码的。
DL_DLR变量，用于指定下载的source file存放的位置，构建系统搜索source files时，bitbake首先搜索download路径下时候有相应的source files。如果没有，再在通过其他路径搜索。减少由于网络原因导致的构建时间过长。

### 2.1.2. Bitbake

主要变量找到指定的软件包：

### 2.1.3. bitbake离线构建的方法：


## 2.2. yocto开发的一般过程

创建layers、添加新的软件包、扩展或定制系统镜像以及移植到新的硬件平台（增加新的MACHINE等）。
以meta-m1创建流程举例分析：`yocto/meta-soc/meta-xxx/meta-SOC-version`。
文件meta-m1目录就是一个m1硬件文件夹，也就是layers。
一个layers包含了一些特定功能所需要的源数据，各layers之间互不干扰，当所构建的系统需要的功能发生变化时，只需要修改该功能对应的layer即可，保持了功能的独立性及模块化设计。

### 2.2.1. 创建Layers
统一的框架，按照这个框架去做即可:
yocto/meta-soc/meta-xxx/meta-SOC-version/conf/layer.conf：
```shell

# We have a conf and classes directory, add to BBPATH
# 定义了一个BBPATH路径名
BBPATH .= ":${LAYERDIR}"

# We have recipes-* directories, add to BBFILES
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"

BBFILE_COLLECTIONS += "SOC-version"
BBFILE_PATTERN_m1 = "^${LAYERDIR}/"
# 优先级作用：比如说现在存在相同的recipes文件夹，就会根据优先级，同优先级根据顺序。
BBFILE_PRIORITY_m1 = "6"

LAYERDEPENDS_m1 = "core"
LAYERSERIES_COMPAT_m1 = "gatesgarth"

```
增加内容：根据layers的类型，有的layers会增加machine和distro配置，因此需要在conf/machine文件夹中添加机器配置，在该层的conf/distro添加发行版配置。
例如：yocto/meta-extended/meta-xxx-core/meta-xxx/conf/distro。

#### 2.2.1.1. 创建layers时的一些原则
考虑易于维护，且不会影响其他的layer，创建layer：
1. 避免到创建的layers覆盖其他layer中的recipe。尽量不要将其他layer中的整个recipe复制到新创建的layer中，并对其进行修改。而是采用追加文件.bbappend文件方式，覆盖仅需要修改的部分。
2. 避免重复包含文件，对于需要包含的文件recipe，使用.bbappend文件或者使用相对于原始layer的相对路径在进行应用。具体应用场景？require recipes-core/images/core-image-minimal.bb代替 require core-image-minimal.bb。举例：require recipes-bsp/u-boot/u-boot.inc 而不是 require u-boot.inc

#### 2.2.1.2. 使能layer
创建了新的layer需要使能之后才能参与到系统构建中，在build/conf/bblayers.conf中定义了参与构建系统使用到的layers。
将metadata-xxx写入到对应的bblayers.conf中，新增加meta-xxx。

#### 2.2.1.3. 使用bbappend文件
用于将metadata附加到其他的recipes的recipes称为Bitbake附加文件。
通过bbappend文件，可以使得创建layer在不拷贝其他recipe到新建的layer中的情况下，以附加或改写的方式改变其他layer中的内容，直观来看，就是新建的layer中的bbappend文件，与需要修改或附加额外内容的.bb文件处于不同的layer。
附件文件bbappend必须和对应的recipes（.bb）文件名（或文件名与版本号）一致。
#### 2.2.1.4. 设置layer优先级


#### 2.2.1.5. layer管理

### 2.2.2. 自定义系统镜像文件

#### 2.2.2.1. 通过local.conf增加软件包
通过直接修改build/conf路径下的local.conf配置文件增加软件包是最简单的定制系统的方法之一。同时，由于是直接修改local.conf文件，这种方法一般来说仅使用与在系统image文件中增加软件包。

#### 2.2.2.2. 通过bb文件定制系统镜像

### 2.2.3. 创建Recipes

# 3. 定制化嵌入式linux系统
实际操作：第七讲定制化嵌入式linux系统。
## 3.1. 创建Metadata

运行脚本，会自动生成build及work目录等。

manifest表示构建的系统的主要软件工程。

# 4. 描述性元数据

描述性元数据（Descriptive Metadata）提供关于菜谱和它构建的软件包信息。
- SUMMARY：包的简短描述
- DESCRIPTION:扩展的（可能多行长度），包和其所提供的东西的细节描述
- AUTHOR:
- HOMEPAGE：以http://开头的URL，软件包被托管在这里。
-
许可元数据：

构建元数据：
- PROVIDES：通常用于抽象配置的一个或者多个额外包名的空格分隔列表
- DEPENDS:依赖，在这个包可以被构建前必须被构建的包名的空格分隔列表
- PN:包名，这个变量的值由bitbake从菜谱文件的基名称获取的
- PV:包版本
- PR:包修订
- S：在构建环境中的、构建系统把未解压的源代码放在其中的目录位置。默认位置依赖于菜谱名和版本：${WORKDIR}/${PN}-${PV}。默认位置对于几乎所有从文件包中构建的包来说都是合适的。对于直接从软件配置管理中构建的包来说，你需要显式地设置这个变量，例如用于GIT仓库的${WORKDIR}/git。
- B：在构建环境中的、构建系统把在构建中创建的对象放入其中的目录位置。
-

# 5. bb文件等语法

`${@bb.utils.contains("MACHINE_FEATURES", "acpi", "packagegroup-base-acpi", "",d)}`
示例：
```shell
在Yocto项目中，`${@bb.utils.contains("MACHINE_FEATURES", "acpi", "packagegroup-base-acpi", "",d)}` 这行代码的主要作用是条件性地包含一个包组（package group），具体分析如下：

1. **`bb.utils.contains` 函数**：
   - `bb.utils.contains` 是一个BitBake实用函数，用于检查某个变量是否包含特定的值。
   - 语法：`bb.utils.contains(variable, value, true_value, false_value, d)`
     - `variable`：要检查的变量名。
     - `value`：要在变量中查找的值。
     - `true_value`：如果找到值，则返回这个值。
     - `false_value`：如果没有找到值，则返回这个值。
     - `d`：当前的数据对象。

2. **参数解释**：
   - `"MACHINE_FEATURES"`：这是要检查的变量名。通常在机器配置文件（如 `local.conf` 或 `machine.conf`）中定义。
   - `"acpi"`：这是要在 `MACHINE_FEATURES` 中查找的值。
   - `"packagegroup-base-acpi"`：如果 `MACHINE_FEATURES` 包含 `"acpi"`，则返回这个字符串。
   - `""`：如果 `MACHINE_FEATURES` 不包含 `"acpi"`，则返回空字符串。

3. **整体功能**：
   - 这行代码检查 `MACHINE_FEATURES` 是否包含 `"acpi"`。
   - 如果包含，则返回 `"packagegroup-base-acpi"`，这意味着会包含名为 `packagegroup-base-acpi` 的包组。
   - 如果不包含，则返回空字符串，不会包含任何包组。

总结来说，这行代码的作用是根据机器特性动态地包含或排除特定的包组，从而确保只有在需要时才会包含与 ACPI 相关的包组。
```


## 参考文献
[bitbake执行流程](https://blog.csdn.net/zz2633105/article/details/122336873)
