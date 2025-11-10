# VIM跳转
首先要安装ctags， 在ubuntu下直接输入
`sudo apt-get install exuberant-ctags`
接着，在源文件目录树（这里是在/home/ballack/test/目录下）执行如下命令：
`ctags -R .`
即可在/home/yujuncheng/test目录下生成一个tags文件， 这个文件就是所有函数和变量的索引列表。
tags文件:

接着打开用vim打开任一文件（在此打开dhd_linux.c）， 如下图：

找到模块定义函数， module_init，如下图：
此时将光标移到想要跳转的函数或变量上（在此以函数dhd_module_init为例），通过快捷键 " CTRL + ] "，  即可快速跳转到函数dhd_module_init定义处， 如图：

此时如果想要回到跳转之前的位置， 只需要通过快捷键“ CTRL + T ”即可。这种方式不局限于同一文件中的跳转，也适合于不同文件之间的跳转，而且按了多少次“ CTRL + ] ”，就可以按多少次“ CTRL + T ”原路返回.

