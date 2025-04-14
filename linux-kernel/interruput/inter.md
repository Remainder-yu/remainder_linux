## arm64反编译设备树
```cpp
        pl011@9000000 {
                clock-names = "uartclk\0apb_pclk";
                clocks = <0x8000 0x8000>;
                interrupts = <0x00 0x01 0x04>;
                reg = <0x00 0x9000000 0x00 0x1000>;
                compatible = "arm,pl011\0arm,primecell";
        };

```
arm,pl011和arm,primecell是该外设的兼容字符串,用于和驱动程序进行匹配工作。

interrupts域描述相关属性，这里分别使用3个属性来表示。
1. 中断类型。对于GIC，它主要分成两种类型的中断，分别如下。
* GIC_SPI：共享外设中断，该值在设备树中用0表示
* GIC_PPI:私有外设中断。该值在设备树中用1来表示。
2. 中断ID
3. 触发类型：对应,中断触发类型为高电平触发(IRQ_TYPE_LEVEL_HIGH)。


分析：

首先从根目录下查找“simple-bus”，从设备树可以看出指向smb设备。

用of_platform_bus_create中遍历所有设备。

```cpp
const struct of_device_id of_default_bus_match_table[] = {
	{ .compatible = "simple-bus", },
	{ .compatible = "simple-mfd", },
	{ .compatible = "isa", },
#ifdef CONFIG_ARM_AMBA
	{ .compatible = "arm,amba-bus", },
#endif /* CONFIG_ARM_AMBA */
	{} /* Empty terminated list */
};

static int __init customize_machine(void)
{
	/*
	 * customizes platform devices, or adds new ones
	 * On DT based machines, we fall back to populating the
	 * machine from the device tree, if no callback is provided,
	 * otherwise we would always need an init_machine callback.
	 */
	if (machine_desc->init_machine)
		machine_desc->init_machine();

	return 0;
}
arch_initcall(customize_machine);

// !TODO : 什么时候init_machine完成初始化
DT_MACHINE_START(IMX6UL, "Freescale i.MX6 Ultralite (Device Tree)")
	.init_irq	= imx6ul_init_irq,
	.init_machine	= imx6ul_init_machine,  // 完成初始化
	.init_late	= imx6ul_init_late,
	.dt_compat	= imx6ul_dt_compat,
MACHINE_END


static void __init imx6ul_init_machine(void)
{
	imx_print_silicon_rev(cpu_is_imx6ull() ? "i.MX6ULL" : "i.MX6UL",
		imx_get_soc_revision());

	of_platform_default_populate(NULL, NULL, NULL);
	imx6ul_enet_init();
	imx_anatop_init();
	imx6ul_pm_init();
}

int of_platform_default_populate(struct device_node *root,
				 const struct of_dev_auxdata *lookup,
				 struct device *parent)
{
	return of_platform_populate(root, of_default_bus_match_table, lookup,
				    parent);
}
EXPORT_SYMBOL_GPL(of_platform_default_populate);

of_platform_default_populate

int of_platform_populate(struct device_node *root,
			const struct of_device_id *matches,
			const struct of_dev_auxdata *lookup,
			struct device *parent)
{
	struct device_node *child;
	int rc = 0;

	root = root ? of_node_get(root) : of_find_node_by_path("/");
	if (!root)
		return -EINVAL;

	pr_debug("%s()\n", __func__);
	pr_debug(" starting at: %pOF\n", root);

	device_links_supplier_sync_state_pause();
	for_each_child_of_node(root, child) {
		rc = of_platform_bus_create(child, matches, lookup, parent, true); // 这里的root执行设备树根目录
		if (rc) {
			of_node_put(child);
			break;
		}
	}
	device_links_supplier_sync_state_resume();

	of_node_set_flag(root, OF_POPULATED_BUS);

	of_node_put(root);
	return rc;
}
EXPORT_SYMBOL_GPL(of_platform_populate);


of_amba_device_create(bus, bus_id, platform_data, parent)
-> dev = amba_device_alloc(NULL, 0, 0);
-> ret = amba_device_add(dev, &iomem_resource);



```

## 如何解析设备树中的节点--带有中断属性

```cpp
unsigned int irq_of_parse_and_map(struct device_node *dev, int index)
{
	struct of_phandle_args oirq;

	if (of_irq_parse_one(dev, index, &oirq))
		return 0;

	return irq_create_of_mapping(&oirq);
}
EXPORT_SYMBOL_GPL(irq_of_parse_and_map);

unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data)
{
	struct irq_fwspec fwspec;

	of_phandle_args_to_fwspec(irq_data->np, irq_data->args,
				  irq_data->args_count, &fwspec);

	return irq_create_fwspec_mapping(&fwspec);
}
EXPORT_SYMBOL_GPL(irq_create_of_mapping);

return irq_create_fwspec_mapping(&fwspec);
-> virq = irq_find_mapping(domain, hwirq);
-> virq = irq_domain_alloc_irqs(domain, 1, NUMA_NO_NODE, fwspec);
    -> irq_domain_alloc_descs
    -> return __irq_domain_alloc_irqs(domain, -1, nr_irqs, node, arg, false,
				       NULL);

return domain->ops->alloc(domain, irq_base, nr_irqs, arg);
static const struct irq_domain_ops gic_irq_domain_hierarchy_ops = {
	.translate = gic_irq_domain_translate,
	.alloc = gic_irq_domain_alloc,
	.free = irq_domain_free_irqs_top,
};

static int gic_irq_domain_alloc(struct irq_domain *domain, unsigned int virq,
				unsigned int nr_irqs, void *arg)
{
	int i, ret;
	irq_hw_number_t hwirq;
	unsigned int type = IRQ_TYPE_NONE;
	struct irq_fwspec *fwspec = arg;

	ret = gic_irq_domain_translate(domain, fwspec, &hwirq, &type); // 获取args翻译出来中断号和中断类型
	if (ret)
		return ret;

	for (i = 0; i < nr_irqs; i++) {
		ret = gic_irq_domain_map(domain, virq + i, hwirq + i); // 执行软硬件的映射，并根据中断类型设置struct irq_desc->handle_irq处理函数。
		if (ret)
			return ret;
	}

	return 0;
}

int __irq_domain_alloc_irqs(struct irq_domain *domain, int irq_base,
			    unsigned int nr_irqs, int node, void *arg,
			    bool realloc, const struct irq_affinity_desc *affinity)
{
	int i, ret, virq;

	if (domain == NULL) {
		domain = irq_default_domain;
		if (WARN(!domain, "domain is NULL; cannot allocate IRQ\n"))
			return -EINVAL;
	}

	if (realloc && irq_base >= 0) {
		virq = irq_base;
	} else {
		virq = irq_domain_alloc_descs(irq_base, nr_irqs, 0, node, // 从allocated_irqs位图中查找第一个nr_irqs个空闲的比特位，最终调用__irq_alloc_descs
					      affinity);
		if (virq < 0) {
			pr_debug("cannot allocate IRQ(base %d, count %d)\n",
				 irq_base, nr_irqs);
			return virq;
		}
	}

	if (irq_domain_alloc_irq_data(domain, virq, nr_irqs)) { // 分配struct irq_data
		pr_debug("cannot allocate memory for IRQ%d\n", virq);
		ret = -ENOMEM;
		goto out_free_desc;
	}

	mutex_lock(&irq_domain_mutex);
	ret = irq_domain_alloc_irqs_hierarchy(domain, virq, nr_irqs, arg); // 调用struct irq_domain中的alloc回调函数进行硬件中断号和软件中断号的映射
	if (ret < 0) {
		mutex_unlock(&irq_domain_mutex);
		goto out_free_irq_data;
	}

	for (i = 0; i < nr_irqs; i++) {
		ret = irq_domain_trim_hierarchy(virq + i);
		if (ret) {
			mutex_unlock(&irq_domain_mutex);
			goto out_free_irq_data;
		}
	}

	for (i = 0; i < nr_irqs; i++)
		irq_domain_insert_irq(virq + i);
	mutex_unlock(&irq_domain_mutex);

	return virq;

out_free_irq_data:
	irq_domain_free_irq_data(virq, nr_irqs);
out_free_desc:
	irq_free_descs(virq, nr_irqs);
	return ret;
}
EXPORT_SYMBOL_GPL(__irq_domain_alloc_irqs);
```
以上完成了中断设备树的解析，各数据结构的初始化，以及最主要的硬件中断号到linux中断号的映射。

### ARM底层中断处理
ARM底层中断处理的范围是从中断异常触发，到irq_handler。


参考文档：
[Linux中断管理 (1)Linux中断管理机制](https://www.cnblogs.com/arnoldlu/p/8659981.html)


## 中断控制器初始化
qemu环境学习：
```cpp
        intc@8000000 {
                phandle = <0x8001>;
                interrupts = <0x01 0x09 0x04>;
                reg = <0x00 0x8000000 0x00 0x10000 0x00 0x8010000 0x00 0x10000 0x00 0x8030000 0x00 0x10000 0x00 0x8040000 0x00 0x10000>;
                compatible = "arm,cortex-a15-gic";
                ranges;
                #size-cells = <0x02>;
                #address-cells = <0x02>;
                interrupt-controller;
                #interrupt-cells = <0x03>;

                v2m@8020000 {
                        phandle = <0x8002>;
                        reg = <0x00 0x8020000 0x00 0x1000>;
                        msi-controller;
                        compatible = "arm,gic-v2m-frame";
                };
        };


        pl011@9000000 {
                clock-names = "uartclk\0apb_pclk";
                clocks = <0x8000 0x8000>;
                interrupts = <0x00 0x01 0x04>;
                reg = <0x00 0x9000000 0x00 0x1000>;
                compatible = "arm,pl011\0arm,primecell";
        };

root@qemuarm64:~# cat /proc/interrupts
           CPU0       CPU1       CPU2       CPU3
 11:       4260       5007       5102       4536     GIC-0  27 Level     arch_timer
 43:       1212          0          0          0     GIC-0  78 Edge      virtio0
 44:         35          0          0          0     GIC-0  79 Edge      virtio1
 46:          0          0          0          0     GIC-0  34 Level     rtc-pl031
 47:         22          0          0          0     GIC-0  33 Level     uart-pl011
 48:          0          0          0          0     GIC-0  23 Level     arm-pmu
 51:          0          0          0          0  9030000.pl061   3 Edge      GPIO Key Poweroff
IPI0:       563       1015       1351       1041       Rescheduling interrupts
IPI1:      1279        479        460        375       Function call interrupts
IPI2:         0          0          0          0       CPU stop interrupts
IPI3:         0          0          0          0       CPU stop (for crash dump) interrupts
IPI4:         0          0          0          0       Timer broadcast interrupts
IPI5:       138        559        616        562       IRQ work interrupts
IPI6:         0          0          0          0       CPU wake-up interrupts
Err:          0

```
struct irq_domain用于描述一个中断控制器。GIC中断控制器在初始化时解析DTS信息中定义了几个GIC控制器，每个GIC控制器注册一个struct irq_domain数据结构。



系统初始化时,do_initcalls()函数会调用系统中所有的initcall 回调函数进行初始化,其中of_platform_default_populate_init()函数定义为arch_initcall_sync类型的初始化函数。

```cpp
static int __init of_platform_default_populate_init(void)
{
	struct device_node *node;

	device_links_supplier_sync_state_pause();

	if (!of_have_populated_dt())
		return -ENODEV;

	/*
	 * Handle certain compatibles explicitly, since we don't want to create
	 * platform_devices for every node in /reserved-memory with a
	 * "compatible",
	 */
	for_each_matching_node(node, reserved_mem_matches)
		of_platform_device_create(node, NULL, NULL);

	node = of_find_node_by_path("/firmware");
	if (node) {
		of_platform_populate(node, NULL, NULL, NULL);
		of_node_put(node);
	}

	/* Populate everything else. */
	of_platform_default_populate(NULL, NULL, NULL);

	return 0;
}
arch_initcall_sync(of_platform_default_populate_init);  // 系统初始化时，会调用系统中所有的initcall回调函数进行初始化

int of_platform_default_populate(struct device_node *root,
				 const struct of_dev_auxdata *lookup,
				 struct device *parent)
{
	return of_platform_populate(root, of_default_bus_match_table, lookup,
				    parent);
}
EXPORT_SYMBOL_GPL(of_platform_default_populate);

-> of_platform_populate
	-> of_platform_bus_create(child, matches, lookup, parent, true);
		-> of_device_is_compatible(bus, "arm,primecell")) { // 当匹配"arm,primecell"设备
		-> of_amba_device_create(bus, bus_id, platform_data, parent);
			-> 继续分析调用链？
```

上述代码是怎么解析DTS？完成串口0硬件中断号，并返回linux内核的IRQ号，并保存到amab_device数据结构的irq数组中。串口驱动在pl011_probe函数中直接从
dev->irq[0]中获取IRQ号。

```cpp
drivers/tty/serial/amba-pl011.c
static int pl011_probe(struct amba_device *dev, const struct amba_id *id)
-> uap->port.irq = dev->irq[0]; // 获取对应的软件中断号
// 可以添加调试信息，查看软件中断号

```

分析硬件中断号怎么映射至Linux内核的IRQ。
SOC内部有多个中断控制器。在一些复杂的soc中，多个中断控制器还可以级联成一个树状结构，因此irq_domain管理框架。irq_domain可以支持多个中断控制器，并且完美地支持设备树机制，解决硬件中断号至Linux内核的IRQ号的问题。

### irq_domain数据结构描述
```cpp
struct irq_domain {
	struct list_head link; // 用于将irq_domain连接到全局链表irq_domain_list
	const char *name; // 中断控制器名字
	const struct irq_domain_ops *ops; // irq_domain 映射操作使用的方法集合
	void *host_data;
	unsigned int flags;
	unsigned int mapcount;

	/* Optional data */
	struct fwnode_handle *fwnode;
	enum irq_domain_bus_token bus_token;
	struct irq_domain_chip_generic *gc;
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	struct irq_domain *parent;
#endif

	/* reverse map data. The linear map gets appended to the irq_domain */
	irq_hw_number_t hwirq_max; // 该irq domain支持中断数量的最大值
	unsigned int revmap_size; // 线性映射的大小
	struct radix_tree_root revmap_tree;  // Redix Tree 映射的根节点
	struct mutex revmap_mutex;
	struct irq_data __rcu *revmap[]; // 线性映射用到的lookup table
};
```

GIC在初始化解析DTS信息定义了几个GIC，每个GIC注册一个irq_domain数据结构。中断控制器的驱动代码在drivers/irqchip目录下。

```cpp
IRQCHIP_DECLARE(cortex_a15_gic, "arm,cortex-a15-gic", gic_of_init);
int __init
gic_of_init(struct device_node *node, struct device_node *parent)
{
	struct gic_chip_data *gic;
	int irq, ret;

	if (WARN_ON(!node))
		return -ENODEV;

	if (WARN_ON(gic_cnt >= CONFIG_ARM_GIC_MAX_NR))
		return -EINVAL;

	gic = &gic_data[gic_cnt];

	ret = gic_of_setup(gic, node);
	if (ret)
		return ret;

	/*
	 * Disable split EOI/Deactivate if either HYP is not available
	 * or the CPU interface is too small.
	 */
	if (gic_cnt == 0 && !gic_check_eoimode(node, &gic->raw_cpu_base))
		static_branch_disable(&supports_deactivate_key);

	ret = __gic_init_bases(gic, &node->fwnode);
	if (ret) {
		gic_teardown(gic);
		return ret;
	}

	if (!gic_cnt) {
		gic_init_physaddr(node);
		gic_of_setup_kvm_info(node);
	}

	if (parent) {
		irq = irq_of_parse_and_map(node, 0);
		gic_cascade_irq(gic_cnt, irq);
	}

	if (IS_ENABLED(CONFIG_ARM_GIC_V2M))
		gicv2m_init(&node->fwnode, gic_data[gic_cnt].domain);

	gic_cnt++;
	return 0;
}
// 调用链
-> ret = __gic_init_bases(gic, &node->fwnode);
	-> ret = gic_init_bases(gic, handle);
		-> 	if (handle) {		/* DT/ACPI */
		gic->domain = irq_domain_create_linear(handle, gic_irqs,
						       &gic_irq_domain_hierarchy_ops,
						       gic); //主要是向系统中注册一个irq domain的数据结构
			-> return __irq_domain_add(fwnode, size, size, 0, ops, host_data);
				-> list_add(&domain->link, &irq_domain_list);
static const struct irq_domain_ops gic_irq_domain_hierarchy_ops = {
	.translate = gic_irq_domain_translate,
	.alloc = gic_irq_domain_alloc,
	.free = irq_domain_free_irqs_top,
};

static int gic_irq_domain_translate
-> 		case 0:			/* SPI */
			*hwirq = fwspec->param[1] + 32;
			break;

// 实现IRQ与硬件中断号映射的核心
virq = irq_domain_alloc_irqs(domain, 1, NUMA_NO_NODE, fwspec);
-> __irq_domain_alloc_irqs
	-> virq = irq_domain_alloc_descs(irq_base, nr_irqs, 0, node,
		->  __irq_alloc_descs
			-> alloc_descs(start, cnt, node, affinity, owner);
				->

```


### 中断处理

```cpp
arch/arm/kernel/entry-armv.S
/*
 * Interrupt handling.
 */
	.macro	irq_handler
#ifdef CONFIG_GENERIC_IRQ_MULTI_HANDLER
	mov	r0, sp
	bl	generic_handle_arch_irq
#else
	arch_irq_handler_default
#endif

/**
 * generic_handle_arch_irq - root irq handler for architectures which do no
 *                           entry accounting themselves
 * @regs:	Register file coming from the low-level handling code
 */
asmlinkage void noinstr generic_handle_arch_irq(struct pt_regs *regs)
{
	struct pt_regs *old_regs;

	irq_enter();
	old_regs = set_irq_regs(regs);
	handle_arch_irq(regs);
	set_irq_regs(old_regs);
	irq_exit();
}

handle_arch_irq(regs);

//
void (*handle_arch_irq)(struct pt_regs *) __ro_after_init;
```

在GICV2驱动初始化时使handle_arch_irq指向gic_handle_irq函数：
```cpp
IRQCHIP_DECLARE(cortex_a15_gic, "arm,cortex-a15-gic", gic_of_init);
gic_of_init
 __gic_init_bases
 -> set_handle_irq(gic_handle_irq);
	-> generic_handle_domain_irq(gic->domain, irqnr);
		-> handle_irq_desc(irq_resolve_mapping(domain, hwirq));
			-> generic_handle_irq_desc(desc);
				-> desc->handle_irq(desc);
// set_handle_irq函数主要是调用desc->handle_irq(desc);
// 对于GIC的SPI类型中断来说，调用handle_fasteoi_irq函数

		irq_domain_set_info(d, irq, hw, &gic->chip, d->host_data,
				    handle_fasteoi_irq, NULL, NULL);
-> void handle_fasteoi_irq(struct irq_desc *desc)
	-> handle_irq_event(desc);
		-> ret = handle_irq_event_percpu(desc);`
			-> retval = __handle_irq_event_percpu(desc, &flags);
				-> retval = __handle_irq_event_percpu(desc, &flags);
					-> __irq_wake_thread(desc, action);
						-> wake_up_process(action->thread);

```

### 中断线程化
中断线程化就是使用内核线程处理中断，目的是减少系统关中断的时间，增强系统的实时性。
在上面中断处理程序中，函数__handle_irq_enevt_percpu遍历中断描述符的中断处理链表，执行每个中断处理描述符的处理函数。如果处理函数返回IRQ_WAKE_THREAD，说明是线程化的中断，name唤醒中断处理线程。

中断处理线程的处理函数是irq_thread(),调用函数irq_thread_fn,然后irq_thread_fn调用注册的线程处理函数。
```cpp
static int
setup_irq_thread(struct irqaction *new, unsigned int irq, bool secondary)
{
	struct task_struct *t;

	if (!secondary) {
		t = kthread_create(irq_thread, new, "irq/%d-%s", irq,
				   new->name);
	} else {
		t = kthread_create(irq_thread, new, "irq/%d-s-%s", irq,
				   new->name);
	}
	................
}

static int irq_thread(void *data)
-> handler_fn = irq_thread_fn;
	->

static irqreturn_t irq_thread_fn(struct irq_desc *desc,
		struct irqaction *action)
{
	irqreturn_t ret;

	ret = action->thread_fn(action->irq, action->dev_id);
	if (ret == IRQ_HANDLED)
		atomic_inc(&desc->threads_handled);

	irq_finalize_oneshot(desc, action);
	return ret;
}

// 主要是：ret = action->thread_fn(action->irq, action->dev_id);

```


参考文献：
https://blog.csdn.net/weixin_43512663/article/details/123221060
https://blog.csdn.net/weixin_43512663/article/details/123307434
