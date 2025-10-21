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

