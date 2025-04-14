

yocto主要目录及编译框架流程\
仓库同步：
git init 和git fetch
参考：https://phoenixnap.com/kb/git-fetch

目录结构

编译框架

主要流程

主要版本：
https://wiki.yoctoproject.org/wiki/Releases

如何实现yocto项目跨平台拷贝依赖之前编译：
需要独立的终端shell使能source：

解决错误打印：
https://support.xilinx.com/s/article/69410?language=en_US
```shell
DESCRIPTION
In the 2017.1 release, petalinux build returns the following error message:
INIT: Id "hvc0" respawning too fast: disabled for 5 minutes

How can I prevent this message from occurring?

SOLUTION
In order to remove this message in kernel logs, add the following bitbake variable in <plnx-proj-root>/project-spec/meta-user/conf/petalinuxbsp.conf
SERIAL_CONSOLES_CHECK = "${SERIAL_CONSOLES}"
```



主要指令：
bitbake -c cleansstate xxxx

参考文章：
https://www.cnblogs.com/arnoldlu/p/17216015.html

https://zhuanlan.zhihu.com/p/345167866

https://blog.csdn.net/faihung/article/details/82699268

https://blog.csdn.net/wangweiqiang1325/article/details/122195713?spm=1001.2101.3001.6650.2&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-2-122195713-blog-82699268.235%5Ev40%5Epc_relevant_3m_sort_dl_base1&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-2-122195713-blog-82699268.235%5Ev40%5Epc_relevant_3m_sort_dl_base1&utm_relevant_index=5

https://fulinux.blog.csdn.net/article/details/108910914?spm=1001.2014.3001.5502


指定自己的交叉编译工具链：
参考官网。
