源码地址：https://e.coding.net/weidongshan/linux/doc_and_source_for_drivers.git

地址映射

物理内粗只有512G，虚拟内存有4GB，那么肯定会多个虚拟地址映射到同一个物理地址上去。linux内核启动的时候会初始化MMU，设置好内存映射，设置好以后CPU访问的都是虚拟地址。

im6u资料：
https://e.coding.net/weidongshan/01_all_series_quickstart.git
驱动资料im6ull：https://e.coding.net/weidongshan/linux/doc_and_source_for_drivers.git
