## 设备树
```cpp
			fec1: ethernet@02188000 {
				compatible = "fsl,imx6ul-fec", "fsl,imx6q-fec";  // 
				reg = <0x02188000 0x4000>;  // soc网络外设寄存器地址范围
				interrupts = <GIC_SPI 118 IRQ_TYPE_LEVEL_HIGH>,
					     <GIC_SPI 119 IRQ_TYPE_LEVEL_HIGH>;
				clocks = <&clks IMX6UL_CLK_ENET>,
					 <&clks IMX6UL_CLK_ENET_AHB>,
					 <&clks IMX6UL_CLK_ENET_PTP>,
					 <&clks IMX6UL_CLK_ENET_REF>,
					 <&clks IMX6UL_CLK_ENET_REF>;
				clock-names = "ipg", "ahb", "ptp",
					      "enet_clk_ref", "enet_out";
				stop-mode = <&gpr 0x10 3>;
				fsl,num-tx-queues=<1>; // 指定发送队列的数量
				fsl,num-rx-queues=<1>; // 指定接收队列的数量
				fsl,magic-packet;
				fsl,wakeup_irq = <0>; // 此属性设置唤醒中断索引
				status = "disabled";
                        };

			fec2: ethernet@020b4000 {
				compatible = "fsl,imx6ul-fec", "fsl,imx6q-fec";
				reg = <0x020b4000 0x4000>;
				interrupts = <GIC_SPI 120 IRQ_TYPE_LEVEL_HIGH>, // 网络中断
					     <GIC_SPI 121 IRQ_TYPE_LEVEL_HIGH>;
				clocks = <&clks IMX6UL_CLK_ENET>,
					 <&clks IMX6UL_CLK_ENET_AHB>,
					 <&clks IMX6UL_CLK_ENET_PTP>,
					 <&clks IMX6UL_CLK_ENET2_REF_125M>,
					 <&clks IMX6UL_CLK_ENET2_REF_125M>;
				clock-names = "ipg", "ahb", "ptp",
					      "enet_clk_ref", "enet_out";
				stop-mode = <&gpr 0x10 4>;
				fsl,num-tx-queues=<1>;
				fsl,num-rx-queues=<1>;
				fsl,magic-packet;
				fsl,wakeup_irq = <0>;
				status = "disabled";
			};

&fec1 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_enet1>;
    phy-mode = "rmii";
    phy-handle = <&ethphy0>;
    phy-reset-gpios = <&gpio5 9 GPIO_ACTIVE_LOW>;
    phy-reset-duration = <26>;
    status = "okay";
};

&fec2 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_enet2>;
    phy-mode = "rmii"; // 网络所使用的PHY接口模式
    phy-handle = <&ethphy1>; // 连接到此网络设备的PHY芯片句柄
    phy-reset-gpios = <&gpio5 6 GPIO_ACTIVE_LOW>; // PHY芯片的复位引脚
    phy-reset-duration = <26>; // PHY复位引脚复位持续时间，单位为ms
    status = "okay";
    // 用于指定网络外设所使用的MDIO总线，狐妖作为PHY节点的容器，指定PHY相关的属性信息
    mdio { 
        #address-cells = <1>;
        #size-cells = <0>;

        ethphy0: ethernet-phy@0 {
            compatible = "ethernet-phy-ieee802.3-c22";
            smsc,disable-energy-detect;
            reg = <0>;
        };
        ethphy1: ethernet-phy@1 {
            compatible = "ethernet-phy-ieee802.3-c22";
            smsc,disable-energy-detect;
            reg = <1>; // PHY芯片地址
        };
    };
};


        pinctrl_enet2: enet2grp {
            fsl,pins = <
                MX6UL_PAD_GPIO1_IO06__ENET2_MDIO    0x1b0b0
                MX6UL_PAD_GPIO1_IO07__ENET2_MDC     0x1b0b0
                MX6UL_PAD_ENET2_RX_EN__ENET2_RX_EN  0x1b0b0
                MX6UL_PAD_ENET2_RX_ER__ENET2_RX_ER  0x1b0b0
                MX6UL_PAD_ENET2_RX_DATA0__ENET2_RDATA00 0x1b0b0
                MX6UL_PAD_ENET2_RX_DATA1__ENET2_RDATA01 0x1b0b0
                MX6UL_PAD_ENET2_TX_EN__ENET2_TX_EN  0x1b0b0
                MX6UL_PAD_ENET2_TX_DATA0__ENET2_TDATA00 0x1b0b0
                MX6UL_PAD_ENET2_TX_DATA1__ENET2_TDATA01 0x1b0b0
                MX6UL_PAD_ENET2_TX_CLK__ENET2_REF_CLK2  0x4001b031
                MX6UL_PAD_ENET1_RX_EN__ENET1_RX_EN  0x1b0b0
                MX6UL_PAD_ENET1_RX_ER__ENET1_RX_ER  0x1b0b0
                MX6UL_PAD_ENET1_RX_DATA0__ENET1_RDATA00 0x1b0b0
                MX6UL_PAD_ENET1_RX_DATA1__ENET1_RDATA01 0x1b0b0
                MX6UL_PAD_ENET1_TX_EN__ENET1_TX_EN  0x1b0b0
                MX6UL_PAD_ENET1_TX_DATA0__ENET1_TDATA00 0x1b0b0
                MX6UL_PAD_ENET1_TX_DATA1__ENET1_TDATA01 0x1b0b0
                MX6UL_PAD_ENET1_TX_CLK__ENET1_REF_CLK1  0x4001b031
            >;
        };
```

### 驱动源码分析

正点原子69章节
