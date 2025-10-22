## u-boot的完整启动流程
阶段1：入口函数
    * _start:uboot程序的第一个执行点
    * reset:处理系统复位，初始化CPU状态
    * save_boot_params：保存从BL31传递过来的启动参数
阶段2：早期初始化
    * board_init_f：执行板级早期初始化，包括内存设备和设备树重定位
阶段3：重定位
    * uboot在ATF阶段已经拷贝到DDR，无需任何动作
阶段4：运行时初始化
    * board_init_f：执行运行时初始化，包括设备驱动、环境变量、网络等
阶段5：主循环
    * run_main_loop：启动主循环
    * main_loop:核心循环函数
    * autoboot_command：处理自动启动
    * cli_loop:命令行接口循环
    * parse_file_outer:文件命令解析
    * parse_stream_outer：流命令解析
    * run_command:命令行执行
阶段6：启动应用程序
    * bootdelay_process: 处理启动延迟
    * do_bootm:处理bootm命令
    * fit_image_load:加载FIT镜像
    * 验证镜像签名：安全启动验证
    * 启动linux内核

## uboot加载内核流程

主要流程：
```shell
run_command
-> bootdelay_process
    -> bootcmd处理
        -> do_bootm
            -> fit_image_load
                -> 加载fit镜像
                    -> 验证镜像签名
                        -> 启动linux内核

```

从存储设备或网络加载设备：

在启动过程中，通过FIP（Firmware Image Package）格式加载内核和内核设备树，实现安全启动和系统引导。

安全启动开启：
内核镜像验签：
1. 提取内核镜像
2. 计算内核哈希
3. SHA256哈希计算
4. RSA_PSS签名验证
5. 内核镜像验证通过
同理，设备树验签也是如此。


```cpp
boot/bootm.c:
static int bootm_find_os(struct cmd_tbl *cmdtp, int flag, int argc,
			 char *const argv[])
    -> 	os_hdr = boot_get_kernel(cmdtp, flag, argc, argv,
			&images, &images.os.image_start, &images.os.image_len);
        -> 

```
