
### rk3568中断分析--设备树篇

kernel/arch/arm64/boot/dts/rockchip/rk3568.dtsi
```shell
	gic: interrupt-controller@fd400000 {
		compatible = "arm,gic-v3";
		#interrupt-cells = <3>;
		#address-cells = <2>;
		#size-cells = <2>;
		ranges;
		interrupt-controller;

		reg = <0x0 0xfd400000 0 0x10000>, /* GICD */
		      <0x0 0xfd460000 0 0xc0000>; /* GICR */
		interrupts = <GIC_PPI 9 IRQ_TYPE_LEVEL_HIGH>;
		its: interrupt-controller@fd440000 {
			compatible = "arm,gic-v3-its";
			msi-controller;
			#msi-cells = <1>;
			reg = <0x0 0xfd440000 0x0 0x20000>;
		};
	};
```

## keyirq-demo

### API-中断相关

request_irq