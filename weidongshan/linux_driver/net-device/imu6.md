外围硬件连接架构图
![alt text](./picture/MAC_PHY.png)
## RMII接口

![alt text](./picture/RMII.png)
## MDIO
MDIO全称是 Management Data Input/Output，管理数据输入输出接口，是一个简单的两线串行接口，一根是MDIO数据线，一根是MDC时钟线。驱动程序可以通过MDIO和MDC这两根线访问PHY芯片的任意一个寄存器。

## imx6ull ENET接口简介
详细可以查看《I.MX6ULL 参考手册》的“Chapter22 10/100-Mbps Ethernet MAC(ENET)”章节。

## PHY-LAN8720A
MAC层通过MDIO/MDC总线对PHY进行读写操作，MDIO最多可以控制32个PHY芯片，通过不同的PHY芯片地址对对不同的PHY操作。

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

## 驱动源码分析

正点原子69章节

### net_device结构体
include/linux/netdevice.h
linux内核使用net_device结构体表示一个具体的网络设备，net_device是整个网络驱动的灵魂。网络驱动的核心就是初始化net_device结构体中的各个成员变量，然后将初始化完成以后的net_device注册到Linux内核中。net_device结构体 详细分析：


### imx6ull 网络驱动分析

fec_probe函数分析


```cpp
// 网络平台驱动匹配表
// 设备树与驱动匹配上，然后就会执行fec_probe
static const struct of_device_id fec_dt_ids[] = {
	{ .compatible = "fsl,imx6ul-fec", .data = &fec_devtype[IMX6UL_FEC], },

};
//
static struct platform_driver fec_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.pm	= &fec_pm_ops,
		.of_match_table = fec_dt_ids,
		.suppress_bind_attrs = true,
	},
	.id_table = fec_devtype,
	.probe	= fec_probe,
	.remove	= fec_drv_remove,
};
```

```cpp
static int
fec_probe(struct platform_device *pdev)
{
	struct fec_enet_private *fep;
	struct fec_platform_data *pdata;
	phy_interface_t interface;
	struct net_device *ndev;
	int i, irq, ret = 0;
	const struct of_device_id *of_id;
	static int dev_id;
	struct device_node *np = pdev->dev.of_node, *phy_node;
	int num_tx_qs;
	int num_rx_qs;
	char irq_name[8];
	int irq_cnt;
	struct fec_devinfo *dev_info;

	fec_enet_get_queue_num(pdev, &num_tx_qs, &num_rx_qs); // 获取设备树中的“fsl,num-tx-queues”和“fsl,num-rx-queues”这两个属性值,也就是发送队列和接收队列的大小,

	/* Init network device */ // 申请net_device设备
	ndev = alloc_etherdev_mqs(sizeof(struct fec_enet_private) +
				  FEC_STATS_SIZE, num_tx_qs, num_rx_qs);
	if (!ndev)
		return -ENOMEM;

	SET_NETDEV_DEV(ndev, &pdev->dev);

	/* setup board info structure */
	fep = netdev_priv(ndev); // 获取net_device中私有数据内存首地址，net_device中的私有数据用来存放网络设备结构体，此结构体为fec_enet_private

	of_id = of_match_device(fec_dt_ids, &pdev->dev); //
	if (of_id)
		pdev->id_entry = of_id->data;
	dev_info = (struct fec_devinfo *)pdev->id_entry->driver_data;
	if (dev_info)
		fep->quirks = dev_info->quirks;

	fep->netdev = ndev; // 进行网络设备结构初始化各个成员变量
	fep->num_rx_queues = num_rx_qs;
	fep->num_tx_queues = num_tx_qs;

#if !defined(CONFIG_M5272)
	/* default enable pause frame auto negotiation */
	if (fep->quirks & FEC_QUIRK_HAS_GBIT)
		fep->pause_flag |= FEC_PAUSE_FLAG_AUTONEG;
#endif

	/* Select default pin state */
	pinctrl_pm_select_default_state(&pdev->dev);
     // 获取设备树中的网络外设网卡ENET相关寄存器起始地址，例如ENET1的寄存器起始地址为 0X02188000
	fep->hwp = devm_platform_ioremap_resource(pdev, 0); // 做虚拟地址缓缓，转换后的ENET 虚拟寄存器起始地址保存在fep的hwp成员中
	if (IS_ERR(fep->hwp)) {
		ret = PTR_ERR(fep->hwp);
		goto failed_ioremap;
	}

	fep->pdev = pdev;
	fep->dev_id = dev_id++;

	platform_set_drvdata(pdev, ndev);

	if ((of_machine_is_compatible("fsl,imx6q") ||
	     of_machine_is_compatible("fsl,imx6dl")) &&
	    !of_property_read_bool(np, "fsl,err006687-workaround-present"))
		fep->quirks |= FEC_QUIRK_ERR006687;

	if (of_get_property(np, "fsl,magic-packet", NULL))
		fep->wol_flag |= FEC_WOL_HAS_MAGIC_PACKET;

	ret = fec_enet_init_stop_mode(fep, np);
	if (ret)
		goto failed_stop_mode;

	phy_node = of_parse_phandle(np, "phy-handle", 0);
	if (!phy_node && of_phy_is_fixed_link(np)) {
		ret = of_phy_register_fixed_link(np);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"broken fixed-link specification\n");
			goto failed_phy;
		}
		phy_node = of_node_get(np);
	}
	fep->phy_node = phy_node;

	ret = of_get_phy_mode(pdev->dev.of_node, &interface);
	if (ret) {
		pdata = dev_get_platdata(&pdev->dev);
		if (pdata)
			fep->phy_interface = pdata->phy;
		else
			fep->phy_interface = PHY_INTERFACE_MODE_MII;
	} else {
		fep->phy_interface = interface;
	}

	ret = fec_enet_parse_rgmii_delay(fep, np);
	if (ret)
		goto failed_rgmii_delay;

	fep->clk_ipg = devm_clk_get(&pdev->dev, "ipg");
	if (IS_ERR(fep->clk_ipg)) {
		ret = PTR_ERR(fep->clk_ipg);
		goto failed_clk;
	}

	fep->clk_ahb = devm_clk_get(&pdev->dev, "ahb");
	if (IS_ERR(fep->clk_ahb)) {
		ret = PTR_ERR(fep->clk_ahb);
		goto failed_clk;
	}

	fep->itr_clk_rate = clk_get_rate(fep->clk_ahb);

	/* enet_out is optional, depends on board */
	fep->clk_enet_out = devm_clk_get(&pdev->dev, "enet_out");
	if (IS_ERR(fep->clk_enet_out))
		fep->clk_enet_out = NULL;

	fep->ptp_clk_on = false;
	mutex_init(&fep->ptp_clk_mutex);

	/* clk_ref is optional, depends on board */
	fep->clk_ref = devm_clk_get(&pdev->dev, "enet_clk_ref");
	if (IS_ERR(fep->clk_ref))
		fep->clk_ref = NULL;
	fep->clk_ref_rate = clk_get_rate(fep->clk_ref);

	/* clk_2x_txclk is optional, depends on board */
	if (fep->rgmii_txc_dly || fep->rgmii_rxc_dly) {
		fep->clk_2x_txclk = devm_clk_get(&pdev->dev, "enet_2x_txclk");
		if (IS_ERR(fep->clk_2x_txclk))
			fep->clk_2x_txclk = NULL;
	}

	fep->bufdesc_ex = fep->quirks & FEC_QUIRK_HAS_BUFDESC_EX;
	fep->clk_ptp = devm_clk_get(&pdev->dev, "ptp");
	if (IS_ERR(fep->clk_ptp)) {
		fep->clk_ptp = NULL;
		fep->bufdesc_ex = false;
	}

	ret = fec_enet_clk_enable(ndev, true);
	if (ret)
		goto failed_clk;

	ret = clk_prepare_enable(fep->clk_ipg);
	if (ret)
		goto failed_clk_ipg;
	ret = clk_prepare_enable(fep->clk_ahb);
	if (ret)
		goto failed_clk_ahb;

	fep->reg_phy = devm_regulator_get_optional(&pdev->dev, "phy");
	if (!IS_ERR(fep->reg_phy)) {
		ret = regulator_enable(fep->reg_phy);
		if (ret) {
			dev_err(&pdev->dev,
				"Failed to enable phy regulator: %d\n", ret);
			goto failed_regulator;
		}
	} else {
		if (PTR_ERR(fep->reg_phy) == -EPROBE_DEFER) {
			ret = -EPROBE_DEFER;
			goto failed_regulator;
		}
		fep->reg_phy = NULL;
	}

	pm_runtime_set_autosuspend_delay(&pdev->dev, FEC_MDIO_PM_TIMEOUT);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	ret = fec_reset_phy(pdev);
	if (ret)
		goto failed_reset;

	irq_cnt = fec_enet_get_irq_cnt(pdev);
	if (fep->bufdesc_ex)
		fec_ptp_init(pdev, irq_cnt);

	ret = fec_enet_init(ndev);
	if (ret)
		goto failed_init;

	for (i = 0; i < irq_cnt; i++) {
		snprintf(irq_name, sizeof(irq_name), "int%d", i);
		irq = platform_get_irq_byname_optional(pdev, irq_name);
		if (irq < 0)
			irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			ret = irq;
			goto failed_irq;
		}
		ret = devm_request_irq(&pdev->dev, irq, fec_enet_interrupt,
				       0, pdev->name, ndev);
		if (ret)
			goto failed_irq;

		fep->irq[i] = irq;
	}

	/* Decide which interrupt line is wakeup capable */
	fec_enet_get_wakeup_irq(pdev);

	ret = fec_enet_mii_init(pdev); // 完成RMII接口的初始化，其中主要是初始化SOC读写PHY寄存器函数
	if (ret)
		goto failed_mii_init;

	/* Carrier starts down, phylib will bring it up */
	netif_carrier_off(ndev);
	fec_enet_clk_enable(ndev, false); // 使能网络时钟
	pinctrl_pm_select_sleep_state(&pdev->dev);

	ndev->max_mtu = PKT_MAXBUF_SIZE - ETH_HLEN - ETH_FCS_LEN;

	ret = register_netdev(ndev);  // 注册net_device
	if (ret)
		goto failed_register;

	device_init_wakeup(&ndev->dev, fep->wol_flag &
			   FEC_WOL_HAS_MAGIC_PACKET);

	if (fep->bufdesc_ex && fep->ptp_clock)
		netdev_info(ndev, "registered PHC device %d\n", fep->dev_id);

	fep->rx_copybreak = COPYBREAK_DEFAULT;
	INIT_WORK(&fep->tx_timeout_work, fec_enet_timeout_work);

	pm_runtime_mark_last_busy(&pdev->dev);
	pm_runtime_put_autosuspend(&pdev->dev);

	return 0;

failed_register:
	fec_enet_mii_remove(fep);
failed_mii_init:
failed_irq:
failed_init:
	fec_ptp_stop(pdev);
failed_reset:
	pm_runtime_put_noidle(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	if (fep->reg_phy)
		regulator_disable(fep->reg_phy);
failed_regulator:
	clk_disable_unprepare(fep->clk_ahb);
failed_clk_ahb:
	clk_disable_unprepare(fep->clk_ipg);
failed_clk_ipg:
	fec_enet_clk_enable(ndev, false);
failed_clk:
failed_rgmii_delay:
	if (of_phy_is_fixed_link(np))
		of_phy_deregister_fixed_link(np);
	of_node_put(phy_node);
failed_stop_mode:
failed_phy:
	dev_id--;
failed_ioremap:
	free_netdev(ndev);

	return ret;
}


```


```cpp
static int fec_enet_mii_init(struct platform_device *pdev)
{
	static struct mii_bus *fec0_mii_bus;
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct fec_enet_private *fep = netdev_priv(ndev);
	bool suppress_preamble = false;
	struct phy_device *phydev;
	struct device_node *node;
	int err = -ENXIO;
	u32 mii_speed, holdtime;
	u32 bus_freq;
	int addr;

	/*
	 * The i.MX28 dual fec interfaces are not equal.
	 * Here are the differences:
	 *
	 *  - fec0 supports MII & RMII modes while fec1 only supports RMII
	 *  - fec0 acts as the 1588 time master while fec1 is slave
	 *  - external phys can only be configured by fec0
	 *
	 * That is to say fec1 can not work independently. It only works
	 * when fec0 is working. The reason behind this design is that the
	 * second interface is added primarily for Switch mode.
	 *
	 * Because of the last point above, both phys are attached on fec0
	 * mdio interface in board design, and need to be configured by
	 * fec0 mii_bus.
	 */
	if ((fep->quirks & FEC_QUIRK_SINGLE_MDIO) && fep->dev_id > 0) {
		/* fec1 uses fec0 mii_bus */
		if (mii_cnt && fec0_mii_bus) {
			fep->mii_bus = fec0_mii_bus;
			mii_cnt++;
			return 0;
		}
		return -ENOENT;
	}

	bus_freq = 2500000; /* 2.5MHz by default */
    // 从设备树中获取MDIO节点，如果节点存在的话就会通过of_mdiobus_register(fep->mii_bus, node);注册
	node = of_get_child_by_name(pdev->dev.of_node, "mdio");
	if (node) {
		of_property_read_u32(node, "clock-frequency", &bus_freq);
		suppress_preamble = of_property_read_bool(node,
							  "suppress-preamble");
	}

	/*
	 * Set MII speed (= clk_get_rate() / 2 * phy_speed)
	 *
	 * The formula for FEC MDC is 'ref_freq / (MII_SPEED x 2)' while
	 * for ENET-MAC is 'ref_freq / ((MII_SPEED + 1) x 2)'.  The i.MX28
	 * Reference Manual has an error on this, and gets fixed on i.MX6Q
	 * document.
	 */
	mii_speed = DIV_ROUND_UP(clk_get_rate(fep->clk_ipg), bus_freq * 2);
	if (fep->quirks & FEC_QUIRK_ENET_MAC)
		mii_speed--;
	if (mii_speed > 63) {
		dev_err(&pdev->dev,
			"fec clock (%lu) too fast to get right mii speed\n",
			clk_get_rate(fep->clk_ipg));
		err = -EINVAL;
		goto err_out;
	}

	/*
	 * The i.MX28 and i.MX6 types have another filed in the MSCR (aka
	 * MII_SPEED) register that defines the MDIO output hold time. Earlier
	 * versions are RAZ there, so just ignore the difference and write the
	 * register always.
	 * The minimal hold time according to IEE802.3 (clause 22) is 10 ns.
	 * HOLDTIME + 1 is the number of clk cycles the fec is holding the
	 * output.
	 * The HOLDTIME bitfield takes values between 0 and 7 (inclusive).
	 * Given that ceil(clkrate / 5000000) <= 64, the calculation for
	 * holdtime cannot result in a value greater than 3.
	 */
	holdtime = DIV_ROUND_UP(clk_get_rate(fep->clk_ipg), 100000000) - 1;

	fep->phy_speed = mii_speed << 1 | holdtime << 8;

	if (suppress_preamble)
		fep->phy_speed |= BIT(7);

	if (fep->quirks & FEC_QUIRK_CLEAR_SETUP_MII) {
		/* Clear MMFR to avoid to generate MII event by writing MSCR.
		 * MII event generation condition:
		 * - writing MSCR:
		 *	- mmfr[31:0]_not_zero & mscr[7:0]_is_zero &
		 *	  mscr_reg_data_in[7:0] != 0
		 * - writing MMFR:
		 *	- mscr[7:0]_not_zero
		 */
		writel(0, fep->hwp + FEC_MII_DATA);
	}

	writel(fep->phy_speed, fep->hwp + FEC_MII_SPEED);

	/* Clear any pending transaction complete indication */
	writel(FEC_ENET_MII, fep->hwp + FEC_IEVENT);

	fep->mii_bus = mdiobus_alloc();
	if (fep->mii_bus == NULL) {
		err = -ENOMEM;
		goto err_out;
	}

	fep->mii_bus->name = "fec_enet_mii_bus";
	fep->mii_bus->read = fec_enet_mdio_read;
	fep->mii_bus->write = fec_enet_mdio_write;
	snprintf(fep->mii_bus->id, MII_BUS_ID_SIZE, "%s-%x",
		pdev->name, fep->dev_id + 1);
	fep->mii_bus->priv = fep;
	fep->mii_bus->parent = &pdev->dev;

	err = of_mdiobus_register(fep->mii_bus, node);
	if (err)
		goto err_out_free_mdiobus;
	of_node_put(node);

	/* find all the PHY devices on the bus and set mac_managed_pm to true */
	for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
		phydev = mdiobus_get_phy(fep->mii_bus, addr);
		if (phydev)
			phydev->mac_managed_pm = true;
	}

	mii_cnt++;

	/* save fec0 mii_bus */
	if (fep->quirks & FEC_QUIRK_SINGLE_MDIO)
		fec0_mii_bus = fep->mii_bus;

	return 0;

err_out_free_mdiobus:
	mdiobus_free(fep->mii_bus);
err_out:
	of_node_put(node);
	return err;
}


```

### MDIO总线注册

MDIO用来管理PHY芯片的，分为MDIO和MDC两根线，Linux内核专门为MDIO准备一个总线，叫做MDIO总线，采用mii_bus结构体表示。
```cpp
struct mii_bus {
	struct module *owner;
	const char *name;
	char id[MII_BUS_ID_SIZE];
	void *priv;
	/** @read: Perform a read transfer on the bus */
	int (*read)(struct mii_bus *bus, int addr, int regnum);
	/** @write: Perform a write transfer on the bus */
	int (*write)(struct mii_bus *bus, int addr, int regnum, u16 val);
	/** @reset: Perform a reset of the bus */
	int (*reset)(struct mii_bus *bus);

	/** @stats: Statistic counters per device on the bus */
	struct mdio_bus_stats stats[PHY_MAX_ADDR];

	/**
	 * @mdio_lock: A lock to ensure that only one thing can read/write
	 * the MDIO bus at a time
	 */
	struct mutex mdio_lock;

	/** @parent: Parent device of this bus */
	struct device *parent;
	/** @state: State of bus structure */
	enum {
		MDIOBUS_ALLOCATED = 1,
		MDIOBUS_REGISTERED,
		MDIOBUS_UNREGISTERED,
		MDIOBUS_RELEASED,
	} state;

	/** @dev: Kernel device representation */
	struct device dev;

	/** @mdio_map: list of all MDIO devices on bus */
	struct mdio_device *mdio_map[PHY_MAX_ADDR];

	/** @phy_mask: PHY addresses to be ignored when probing */
	u32 phy_mask;

	/** @phy_ignore_ta_mask: PHY addresses to ignore the TA/read failure */
	u32 phy_ignore_ta_mask;

	/**
	 * @irq: An array of interrupts, each PHY's interrupt at the index
	 * matching its address
	 */
	int irq[PHY_MAX_ADDR];

	/** @reset_delay_us: GPIO reset pulse width in microseconds */
	int reset_delay_us;
	/** @reset_post_delay_us: GPIO reset deassert delay in microseconds */
	int reset_post_delay_us;
	/** @reset_gpiod: Reset GPIO descriptor pointer */
	struct gpio_desc *reset_gpiod;

	/** @probe_capabilities: bus capabilities, used for probing */
	enum {
		MDIOBUS_NO_CAP = 0,
		MDIOBUS_C22,
		MDIOBUS_C45,
		MDIOBUS_C22_C45,
	} probe_capabilities;

	/** @shared_lock: protect access to the shared element */
	struct mutex shared_lock;

	/** @shared: shared state across different PHYs */
	struct phy_package_shared *shared[PHY_MAX_ADDR];
};
#define to_mii_bus(d) container_of(d, struct mii_bus, dev)

```

其中重点是read write函数，不同的soc和mdio是不一样的。



fec_netdev_ops操作集


### 存在问题：
1. 在probe中register_netdev 注册 net_device详细流程？
