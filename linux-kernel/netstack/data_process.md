# netstack数据包接收主要流程
## 链路层接收数据

### 网卡(PCI 设备的注册)
网卡的net_device就是在这个函数里面初始化的并注册到内核的。

### 网卡链路状态检测
当网卡链路状态变化时（如断开或连上），网卡会通知驱动程序或者由驱动程序去查询网卡的相关寄存器位（例如在timeout时区查询这些位），然后再netif_carrier_on/off去通知内核这个变化。

```c
net/sched/sch_generic.c
void netif_carrier_on(struct net_device *dev)
{
	if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &dev->state)) {
		if (dev->reg_state == NETREG_UNINITIALIZED)
			return;
		atomic_inc(&dev->carrier_up_count);
		linkwatch_fire_event(dev);
		if (netif_running(dev))
			__netdev_watchdog_up(dev);
	}
}
EXPORT_SYMBOL(netif_carrier_on);

```

```c
net/core/link_watch.c
void linkwatch_fire_event(struct net_device *dev)
{
	bool urgent = linkwatch_urgent_event(dev);

	if (!test_and_set_bit(__LINK_STATE_LINKWATCH_PENDING, &dev->state)) {
		linkwatch_add_event(dev);
	} else if (!urgent)
		return;

	linkwatch_schedule_work(urgent);
}
EXPORT_SYMBOL(linkwatch_fire_event);

```

```c
static void linkwatch_schedule_work(int urgent)
{
	unsigned long delay = linkwatch_nextevent - jiffies;

	if (test_bit(LW_URGENT, &linkwatch_flags))
		return;

	/* Minimise down-time: drop delay for up event. */
	if (urgent) {
		if (test_and_set_bit(LW_URGENT, &linkwatch_flags))
			return;
		delay = 0;
	}

	/* If we wrap around we'll delay it by at most HZ. */
	if (delay > HZ)
		delay = 0;

	/*
	 * If urgent, schedule immediate execution; otherwise, don't
	 * override the existing timer.
	 */
	if (test_bit(LW_URGENT, &linkwatch_flags))
		mod_delayed_work(system_wq, &linkwatch_work, 0);
	else
		schedule_delayed_work(&linkwatch_work, delay);
}

static void linkwatch_event(struct work_struct *dummy);
static DECLARE_DELAYED_WORK(linkwatch_work, linkwatch_event);

```

然后调用linkwatch_schedule_work(urgent);-->调用schedule_delayed_work(&linkwatch_work, delay);由内核线程去处理这些事件。最终由linkwatch_run_queue(void)去完成这些处理工作。

```c
/* Must be called with the rtnl semaphore held */
void linkwatch_run_queue(void)
{
	__linkwatch_run_queue(0);
}


static void linkwatch_event(struct work_struct *dummy)
{
	rtnl_lock();
	__linkwatch_run_queue(time_after(linkwatch_nextevent, jiffies));
	rtnl_unlock();
}

```


```c
static void __linkwatch_run_queue(int urgent_only)
{
	struct net_device *dev;
	LIST_HEAD(wrk);

	/*
	 * Limit the number of linkwatch events to one
	 * per second so that a runaway driver does not
	 * cause a storm of messages on the netlink
	 * socket.  This limit does not apply to up events
	 * while the device qdisc is down.
	 */
	if (!urgent_only)
		linkwatch_nextevent = jiffies + HZ;
	/* Limit wrap-around effect on delay. */
	else if (time_after(linkwatch_nextevent, jiffies + HZ))
		linkwatch_nextevent = jiffies;

	clear_bit(LW_URGENT, &linkwatch_flags);

	spin_lock_irq(&lweventlist_lock);
	list_splice_init(&lweventlist, &wrk);

	while (!list_empty(&wrk)) {

		dev = list_first_entry(&wrk, struct net_device, link_watch_list);
		list_del_init(&dev->link_watch_list);

		if (urgent_only && !linkwatch_urgent_event(dev)) {
			list_add_tail(&dev->link_watch_list, &lweventlist);
			continue;
		}
		spin_unlock_irq(&lweventlist_lock);
		linkwatch_do_dev(dev);
		spin_lock_irq(&lweventlist_lock);
	}

	if (!list_empty(&lweventlist))
		linkwatch_schedule_work(0);
	spin_unlock_irq(&lweventlist_lock);
}

```

```c
static void linkwatch_do_dev(struct net_device *dev)
{
	/*
	 * Make sure the above read is complete since it can be
	 * rewritten as soon as we clear the bit below.
	 */
	smp_mb__before_atomic();

	/* We are about to handle this device,
	 * so new events can be accepted
	 */
	clear_bit(__LINK_STATE_LINKWATCH_PENDING, &dev->state);

	rfc2863_policy(dev);
	if (dev->flags & IFF_UP && netif_device_present(dev)) {
		if (netif_carrier_ok(dev))
			dev_activate(dev);
		else
			dev_deactivate(dev);

		netdev_state_change(dev);
	}
	dev_put(dev);
}

```

可以看到,它的最主要工作之一就是 netdev_state_change(dev):

```c
/**
 *	netdev_state_change - device changes state
 *	@dev: device to cause notification
 *
 *	Called to indicate a device has changed state. This function calls
 *	the notifier chains for netdev_chain and sends a NEWLINK message
 *	to the routing socket.
 */
void netdev_state_change(struct net_device *dev)
{
	if (dev->flags & IFF_UP) {
		struct netdev_notifier_change_info change_info = {
			.info.dev = dev,
		};

		call_netdevice_notifiers_info(NETDEV_CHANGE,
					      &change_info.info);
		rtmsg_ifinfo(RTM_NEWLINK, dev, 0, GFP_KERNEL);
	}
}
EXPORT_SYMBOL(netdev_state_change);

```
这个函数通知注册到 netdev_chain 链表的所有子系统,这个网卡的链路状态有了变化。就是说,如果某个子系统对网卡的链路状态变化感兴趣,它就可以注册到进这个链表,在变化产生时,内核便会通知这些子系统。
它会调用"rtmsg_ifinfo"函数，向路由守护进程发送一个新的链路信息。这个函数可能会更新路由表，以反映设备状态的变化。

注意:
a. 它只会在网卡状态为 UP 时,才会发出通知,因为,如果状态为 DOWN,网卡链路的状态改变也没什么意义。
b. 每个见网卡的这些状态变化的事件 lw_event 是不会队列的,即每个网卡只有一个事件的实例在队列中。还有由上面看到的 lw_event 结构,它只是包含发生状态变化的网卡设备,而没有包含它是链上或是断开的状状参数。

### 数据包的接收
这个数据结构同时用于接收与发送数据包。

```c
struct softnet_data {
    struct list_head    poll_list;
    struct sk_buff_head process_queue;

    /* stats */
    unsigned int        processed;
    unsigned int        time_squeeze;
    unsigned int        received_rps;
#ifdef CONFIG_RPS
    struct softnet_data *rps_ipi_list;
#endif
#ifdef CONFIG_NET_FLOW_LIMIT
    struct sd_flow_limit __rcu *flow_limit;
#endif
    struct Qdisc        *output_queue;
    struct Qdisc        **output_queue_tailp;
    struct sk_buff      *completion_queue;
#ifdef CONFIG_XFRM_OFFLOAD
    struct sk_buff_head xfrm_backlog;
#endif
#ifdef CONFIG_RPS
    /* input_queue_head should be written by cpu owning this struct,
     * and only read by other cpus. Worth using a cache line.
     */
    unsigned int        input_queue_head ____cacheline_aligned_in_smp;

    /* Elements below can be accessed between CPUs for RPS/RFS */
    call_single_data_t  csd ____cacheline_aligned_in_smp;
    struct softnet_data *rps_ipi_next;
    unsigned int        cpu;
    unsigned int        input_queue_tail;
#endif
    unsigned int        dropped;
    struct sk_buff_head input_pkt_queue;
    struct napi_struct  backlog;

};

```

```c
// input_pkt_queue是一个sk_buff队列，用于存放输入的网络数据包。
struct sk_buff_head input_pkt_queue;
// poll_list是一个链表，用于存放等待被处理的网络数据包。
struct list_head poll_list;
//
struct net_device backlog_dev;
```
#### NON-NAPI方式

#### NAPI方式

#### net_rx_action()

中断下半部执行：

```c
net/core/dev.c
static int __init net_dev_init(void)
{
	open_softirq(NET_TX_SOFTIRQ, net_tx_action);
	open_softirq(NET_RX_SOFTIRQ, net_rx_action);
}
```
`static __latent_entropy void net_rx_action(struct softirq_action *h){}`
这段代码是Linux内核中用于处理网络接收的函数。它主要做以下几件事情：
* 获取当前CPU上的软网络数据（softnet_data），并禁用本地中断。
* 将当前CPU上的软中断动作列表（softirq_action）移动到一个新的列表中。
* 对列表中的每一项（napi_struct）进行轮询，也就是调用napi_poll函数。如果没有足够的时间或者budget来完成所有的工作，就跳出循环。
* 重新启用本地中断，然后将剩余的任务添加回原来的列表。
* 最后，检查是否还有待处理的任务，如果有，就再次引发NET_RX_SOFTIRQ软中断。
* 释放可能被napi_poll函数分配的任何未使用的skb

#### poll函数
每个驱动都自己实现：
然后调用int netif_receive_skb(struct sk_buff *skb)

####  netif_receive_skb
net/core/dev.c文件中。这是一个辅助函数,用于在 poll 中处理接收到的帧。它主要是向各个已注册的协议处理例程发送一个 SKB。
每个协议的类型由一个 packet_type 结构表示:
```c
/**
 *	netif_receive_skb - process receive buffer from network
 *	@skb: buffer to process
 *
 *	netif_receive_skb() is the main receive data processing function.
 *	It always succeeds. The buffer may be dropped during processing
 *	for congestion control or by the protocol layers.
 *
 *	This function may only be called from softirq context and interrupts
 *	should be enabled.
 *
 *	Return values (usually ignored):
 *	NET_RX_SUCCESS: no congestion
 *	NET_RX_DROP: packet was dropped
 */
int netif_receive_skb(struct sk_buff *skb)
{
	int ret;

	trace_netif_receive_skb_entry(skb);

	ret = netif_receive_skb_internal(skb);
	trace_netif_receive_skb_exit(ret);

	return ret;
}
EXPORT_SYMBOL(netif_receive_skb);

static int netif_receive_skb_internal(struct sk_buff *skb)
{
	int ret;

	net_timestamp_check(netdev_tstamp_prequeue, skb);

	if (skb_defer_rx_timestamp(skb))
		return NET_RX_SUCCESS;

	if (static_branch_unlikely(&generic_xdp_needed_key)) {
		int ret;

		preempt_disable();
		rcu_read_lock();
		ret = do_xdp_generic(rcu_dereference(skb->dev->xdp_prog), skb);
		rcu_read_unlock();
		preempt_enable();

		if (ret != XDP_PASS)
			return NET_RX_DROP;
	}

	rcu_read_lock();
#ifdef CONFIG_RPS
	if (static_key_false(&rps_needed)) {
		struct rps_dev_flow voidflow, *rflow = &voidflow;
		int cpu = get_rps_cpu(skb->dev, skb, &rflow);

		if (cpu >= 0) {
			ret = enqueue_to_backlog(skb, cpu, &rflow->last_qtail);
			rcu_read_unlock();
			return ret;
		}
	}
#endif
	ret = __netif_receive_skb(skb);
	rcu_read_unlock();
	return ret;
}


static int __netif_receive_skb(struct sk_buff *skb)
{
	int ret;

	if (sk_memalloc_socks() && skb_pfmemalloc(skb)) {
		unsigned int noreclaim_flag;

		/*
		 * PFMEMALLOC skbs are special, they should
		 * - be delivered to SOCK_MEMALLOC sockets only
		 * - stay away from userspace
		 * - have bounded memory usage
		 *
		 * Use PF_MEMALLOC as this saves us from propagating the allocation
		 * context down to all allocation sites.
		 */
		noreclaim_flag = memalloc_noreclaim_save();
		ret = __netif_receive_skb_one_core(skb, true);
		memalloc_noreclaim_restore(noreclaim_flag);
	} else
		ret = __netif_receive_skb_one_core(skb, false);

	return ret;
}

static int __netif_receive_skb_one_core(struct sk_buff *skb, bool pfmemalloc)
{
	struct net_device *orig_dev = skb->dev;
	struct packet_type *pt_prev = NULL;
	int ret;

	ret = __netif_receive_skb_core(skb, pfmemalloc, &pt_prev);
	if (pt_prev)
		ret = pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
	return ret;
}

static int __netif_receive_skb_core(struct sk_buff *skb, bool pfmemalloc,
				    struct packet_type **ppt_prev)
{


	list_for_each_entry_rcu(ptype, &skb->dev->ptype_all, list) {
		if (pt_prev)
			ret = deliver_skb(skb, pt_prev, orig_dev);
		pt_prev = ptype;
	}

}

static inline int deliver_skb(struct sk_buff *skb,
			      struct packet_type *pt_prev,
			      struct net_device *orig_dev)
{
	if (unlikely(skb_orphan_frags_rx(skb, GFP_ATOMIC)))
		return -ENOMEM;
	refcount_inc(&skb->users);
	return pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
}

```

netif_receive_skb()的主要作用体现在两个遍历链表的操作中,其中之一为遍历 ptype_all 链,这些为注册到内核的一些 sniffer,将上传给这些 sniffer,另一个就是遍历 ptype_base,这个就是具体的协议类型。假高如上图如示,当 eth1 接收到一个 IP 数据包时,它首先分别发送一份副本给两个 ptype_all 链表中的 packet_type,它们都由 package_rcv 处理,然后再根据HASH 值,在遍历另一个 HASH 表时,发送一份给类型为 ETH_P_IP 的类型,它由 ip_rcv处理。如果这个链中还注册有其它 IP 层的协议,它也会同时发送一个副本给它。
deliver_skb只是一个包装函数,它只去执行相应 packet_type 里的 func 处理函数,如对于ETH_P_IP 类型,由上面可以看到,它执行的就是 ip_rcv 了。

所用到的协议在系统或模块加载的时候初始化,如 IP 协议:

```c
static int __init inet_init(void)
{
    dev_add_pack(&ip_packet_type);

}

void dev_add_pack(struct packet_type *pt)
{
	struct list_head *head = ptype_head(pt);

	spin_lock(&ptype_lock);
	list_add_rcu(&pt->list, head);
	spin_unlock(&ptype_lock);
}
EXPORT_SYMBOL(dev_add_pack);

```
这段代码是一个网络驱动程序的部分，用于向一个数据结构添加一个包。这个数据结构是一个双向循环链表，每个节点都是一个packet_type类型的结构体。
首先，定义了一个函数dev_add_pack，该函数接收一个指向packet_type结构体的指针pt作为参数。然后，通过调用ptype_head函数，获取到链表的头指针，并将其赋值给head。
然后，进行了一个关键操作，spin_lock函数用于获取一个内核自旋锁，确保在此期间，不会有其他进程或中断处理程序对数据结构进行修改。这是因为在多处理器系统中，对共享数据结构的并发访问可能会导致数据不一致。
获取到锁之后，就可以安全地对数据结构进行操作了。这里使用的是list_add_rcu函数，该函数将新的packet_type结构体添加到链表中。
最后，调用spin_unlock函数释放锁，表示已经完成对数据结构的操作，其他进程或中断处理程序可以继续访问数据结构了。
这段代码的主要功能是向一个数据结构添加一个包，同时确保了在操作过程中的并发安全性。


### 中断处理接收网卡数据流程

```c

__do_softirq
		--> h->action(h);
在int __init net_dev_init(void)
	--> open_softirq(NET_RX_SOFTIRQ, net_rx_action); // 注册软中断

include/linux/interrupt.h
enum
{
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	BLOCK_SOFTIRQ,
	IRQ_POLL_SOFTIRQ,
	TASKLET_SOFTIRQ,
	SCHED_SOFTIRQ,
	HRTIMER_SOFTIRQ, /* Unused, but kept as tools rely on the
			    numbering. Sigh! */
	RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

	NR_SOFTIRQS
};

struct softirq_action
{
	void	(*action)(struct softirq_action *);
};

extern void open_softirq(int nr, void (*action)(struct softirq_action *));

/**
这段代码定义了一个名为open_softirq的函数，该函数接收两个参数：一个整数nr和一个指向函数的指针action。函数softirq_vec是一个数组，每个元素都是一个结构体softirq_action。这段代码的作用是将参数action赋值给数组softirq_vec中索引为nr的元素的action成员。

在Linux内核中， softirq 是一种特殊类型的中断，它不直接由硬件设备产生，而是由软件自行生成的中断。open_softirq函数就是用来注册一个softirq处理函数的。当softirq被触发时，就会调用对应的处理函数。

注意：以上解释基于Linux内核中的实现，其他操作系统或者编程语言可能有所不同。
*/
void open_softirq(int nr, void (*action)(struct softirq_action *))
{
	softirq_vec[nr].action = action;
}

就会调用对应的处理函数：net_rx_action
这个函数中剩下的核心逻辑是获取当前CPU变量softnet_data，对其poll_list进行遍历，然后执行到网卡驱动注册的poll函数。

```

调用对应驱动的igb_poll函数，例如：
```c
static __latent_entropy void net_rx_action(struct softirq_action *h)
--> budget -= napi_poll(n, &repoll);
		--> work = n->poll(n, weight);
			--> static int igb_poll(struct napi_struct *napi, int budget) :例如对应的pull函数调用igb_poll:drivers/net/ethernet/intel/igb/igb_main.c
				--> int cleaned = igb_clean_rx_irq(q_vector, budget);
					--> rx_buffer = igb_get_rx_buffer(rx_ring, size);
					--> igb_is_non_eop(rx_ring, rx_desc) :数据帧从RingBuffer取下来。
					--> napi_gro_receive(&q_vector->napi, skb);//net/core/dev.c
```

然后驱动转发数据到
```c
//net/core/dev.c
gro_result_t napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
	gro_result_t ret;

	skb_mark_napi_id(skb, napi);
	trace_napi_gro_receive_entry(skb);

	skb_gro_reset_offset(skb);

	ret = napi_skb_finish(dev_gro_receive(napi, skb), skb);
	trace_napi_gro_receive_exit(ret);

	return ret;
}
EXPORT_SYMBOL(napi_gro_receive);

static gro_result_t napi_skb_finish(gro_result_t ret, struct sk_buff *skb)
{
	switch (ret) {
	case GRO_NORMAL:
		if (netif_receive_skb_internal(skb))
			ret = GRO_DROP;
		break;

	case GRO_DROP:
		kfree_skb(skb);
		break;

	case GRO_MERGED_FREE:
		if (NAPI_GRO_CB(skb)->free == NAPI_GRO_FREE_STOLEN_HEAD)
			napi_skb_free_stolen_head(skb);
		else
			__kfree_skb(skb);
		break;

	case GRO_HELD:
	case GRO_MERGED:
	case GRO_CONSUMED:
		break;
	}

	return ret;
}
```

napi_skb_finish调用怎么进入协议栈处理：

```c
// net/core/dev.c
napi_skb_finish(gro_result_t ret, struct sk_buff *skb)
|--> int netif_receive_skb_internal(struct sk_buff *skb)
	|--> ret = __netif_receive_skb(skb);
		|--> int __netif_receive_skb_one_core(struct sk_buff *skb, bool pfmemalloc)
			|--> list_for_each_entry_rcu(ptype, &ptype_all, list)
			|--> ret = deliver_skb(skb, pt_prev, orig_dev);
				--> return pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
			|-->ret = pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
```
`net/ipv4/af_inet.c`文件中将`.func = ip_rcv`,所以在调用`pt_prev->func(skb, skb->dev, pt_prev, orig_dev)`;就可以直接调用`ip_rcv`,或`arp_rcv`等协议处理函数中进行处理。再通过`.handler =	udp_rcv`,传输到`udp_rcv`进一步处理。
```c
static struct packet_type ip_packet_type __read_mostly = {
	.type = cpu_to_be16(ETH_P_IP),
	.func = ip_rcv,
	.list_func = ip_list_rcv,
};

static struct packet_type arp_packet_type __read_mostly = {
	.type =	cpu_to_be16(ETH_P_ARP),
	.func =	arp_rcv,
};

static struct net_protocol udp_protocol = {
	.early_demux =	udp_v4_early_demux,
	.early_demux_handler =	udp_v4_early_demux,
	.handler =	udp_rcv,
	.err_handler =	udp_err,
	.no_policy =	1,
	.netns_ok =	1,
};
```

## 网络协议栈处理（接收数据流程）
在函数中netif_receive_skb会根据包的协议进行处理，假如是UDP包，将包依次送到ip_rcv或者arp_rcv等进行协议对应处理函数中。
```c
__netif_receive_skb
	---->  __netif_receive_skb_one_core // 网络层根据不同协议处理
			----> arp_rcv
			----> ip_rcv//根据协议进行不同的传输层协议处理
				----> tcp_v4_rcv()
				----> udp_rcv()
					---> 用户进程skb接收队列，用户进程被唤醒。
```

tcpdump抓包实现：list_for_each_entry_rcu(ptype, &ptype_all, list) （net/core/dev.c）函数中实现，这就是将数据送入抓包点，tcpdump就是从这个入口获取包的。

`net/ipv4/af_inet.c`文件中将`.func = ip_rcv`,所以在调用`pt_prev->func(skb, skb->dev, pt_prev, orig_dev)`;就可以直接调用`ip_rcv`,或`arp_rcv`等协议处理函数中进行处理。再通过`.handler =	udp_rcv`,传输到`udp_rcv`进一步处理。
```c
static struct packet_type ip_packet_type __read_mostly = {
	.type = cpu_to_be16(ETH_P_IP),
	.func = ip_rcv,
	.list_func = ip_list_rcv,
};
```

### IP层处理
主要流程：
```c
 ip_rcv()
 ---> int ip_rcv_finish(struct net *net, struct sock *sk, struct sk_buff *skb)
	---> ret = ip_rcv_finish_core(net, sk, skb, dev);//net/ipv4/ip_input.c
		---> rt = skb_rtable(skb);
			---> struct dst_entry *skb_dst(const struct sk_buff *skb)
		---> ret = dst_input(skb);// Input packet from network to transport.
			---> return skb_dst(skb)->input(skb); // rt->dst.input = ip_local_deliver;
				---> int ip_local_deliver(struct sk_buff *skb) // Deliver IP Packets to the higher protocol layers.
					---> int ip_local_deliver_finish(struct net *net, struct sock *sk, struct sk_buff *skb)
						---> void ip_protocol_deliver_rcu(struct net *net, struct sk_buff *skb, int protocol)
							---> raw = raw_local_deliver(skb, protocol);
								---> if (raw_sk && !raw_v4_input(skb, ip_hdr(skb), hash))
									---> sock *__raw_v4_lookup()
										---> raw_sk_bound_dev_eq(net, sk->sk_bound_dev_if, dif, sdif)
							---> ret = ipprot->handler(skb); //.handler =	udp_rcv,至此到了udp层协议栈处理

struct net_protocol {
	int			(*early_demux)(struct sk_buff *skb);
	int			(*early_demux_handler)(struct sk_buff *skb);
	int			(*handler)(struct sk_buff *skb);

	/* This returns an error if we weren't able to handle the error. */
	int			(*err_handler)(struct sk_buff *skb, u32 info);

	unsigned int		no_policy:1,
				netns_ok:1,
				/* does the protocol do more stringent
				 * icmp tag validation than simple
				 * socket lookup?
				 */
				icmp_strict_tag_validation:1;
};

static struct net_protocol udp_protocol = {
	.early_demux =	udp_v4_early_demux,
	.early_demux_handler =	udp_v4_early_demux,
	.handler =	udp_rcv,
	.err_handler =	udp_err,
	.no_policy =	1,
	.netns_ok =	1,
};
```
rt->dst.input = ip_local_deliver;
```c
struct rtable *rt_dst_alloc(struct net_device *dev,
			    unsigned int flags, u16 type,
			    bool nopolicy, bool noxfrm, bool will_cache)
{
	struct rtable *rt;

	rt = dst_alloc(&ipv4_dst_ops, dev, 1, DST_OBSOLETE_FORCE_CHK,
		       (will_cache ? 0 : DST_HOST) |
		       (nopolicy ? DST_NOPOLICY : 0) |
		       (noxfrm ? DST_NOXFRM : 0));

	if (rt) {
		rt->rt_genid = rt_genid_ipv4(dev_net(dev));
		rt->rt_flags = flags;
		rt->rt_type = type;
		rt->rt_is_input = 0;
		rt->rt_iif = 0;
		rt->rt_pmtu = 0;
		rt->rt_mtu_locked = 0;
		rt->rt_gateway = 0;
		rt->rt_uses_gateway = 0;
		INIT_LIST_HEAD(&rt->rt_uncached);

		rt->dst.output = ip_output;
		if (flags & RTCF_LOCAL)
			rt->dst.input = ip_local_deliver;
	}

	return rt;
}
EXPORT_SYMBOL(rt_dst_alloc);

net/ipv4/ip_input.c
int ip_local_deliver(struct sk_buff *skb)
{
	/*
	 *	Reassemble IP fragments.
	 */
	struct net *net = dev_net(skb->dev);

	if (ip_is_fragment(ip_hdr(skb))) {
		if (ip_defrag(net, skb, IP_DEFRAG_LOCAL_DELIVER))
			return 0;
	}

	return NF_HOOK(NFPROTO_IPV4, NF_INET_LOCAL_IN,
		       net, NULL, skb, skb->dev, NULL,
		       ip_local_deliver_finish);
}

void ip_protocol_deliver_rcu(struct net *net, struct sk_buff *skb, int protocol)
{
	const struct net_protocol *ipprot;
	int raw, ret;

resubmit:
	raw = raw_local_deliver(skb, protocol);

	ipprot = rcu_dereference(inet_protos[protocol]);
	if (ipprot) {
		if (!ipprot->no_policy) {
			if (!xfrm4_policy_check(NULL, XFRM_POLICY_IN, skb)) {
				kfree_skb(skb);
				return;
			}
			nf_reset(skb);
		}
		ret = ipprot->handler(skb); // 对应协议栈注册，struct net_protocol *ipprot，对应udp_rcv，tcp_v4_rcv
		if (ret < 0) {
			protocol = -ret;
			goto resubmit;
		}
		······
	}
}

```
#### udp路由转发的过程
如果查找dst不是本机的udp包，则会查找路由：rth->dst.input = ip_forward;
```c
ip_rcv_finish_core(net, sk, skb, dev);
	---> if (!skb_valid_dst(skb)) {
		err = ip_route_input_noref(skb, iph->daddr, iph->saddr,
					   iph->tos, dev);}
		---> err = ip_route_input_rcu(skb, daddr, saddr, tos, dev, &res);
			---> return ip_route_input_slow(skb, daddr, saddr, tos, dev, res);
				---> err = fib_lookup(net, &fl4, res, 0);
				---> if (res->type == RTN_BROADCAST) {
						if (IN_DEV_BFORWARD(in_dev))
						goto make_route;
					goto brd_input;
					}
					---> int ip_mkroute_input()
						---> return __mkroute_input(skb, res, in_dev, daddr, saddr, tos); // create a routing cache entry
							---> rth->dst.input = ip_forward;
								---> ip_forward_finish
									---> return dst_output(net, sk, skb);
										---> return skb_dst(skb)->output(net, sk, skb); // 接下面令居子系统分析
							---> rt_set_nexthop(rth, daddr, res, fnhe, res->fi, res->type, itag,
		       						do_cache);
```
找到这个skb的路由表条目，然后调用路由表的output方法。skb_dst(skb)->output(net, sk, skb);
```c
net/ipv4/route.c
struct rtable *rt_dst_alloc(struct net_device *dev,
			    unsigned int flags, u16 type,
			    bool nopolicy, bool noxfrm, bool will_cache)
{
	struct rtable *rt;
		rt->dst.output = ip_output;
}

net/ipv4/ip_output.c
int ip_output(struct net *net, struct sock *sk, struct sk_buff *skb)
{
	struct net_device *dev = skb_dst(skb)->dev;

	IP_UPD_PO_STATS(net, IPSTATS_MIB_OUT, skb->len);

	skb->dev = dev;
	skb->protocol = htons(ETH_P_IP);

	return NF_HOOK_COND(NFPROTO_IPV4, NF_INET_POST_ROUTING,
			    net, sk, skb, NULL, dev,
			    ip_finish_output,
			    !(IPCB(skb)->flags & IPSKB_REROUTED));
}

static int ip_finish_output(struct net *net, struct sock *sk, struct sk_buff *skb)
--->static int ip_finish_output2(struct net *net, struct sock *sk, struct sk_buff *skb)
	---> int neigh_output(struct neighbour *n, struct sk_buff *skb)
		---> return n->output(n, skb); //.output =		neigh_resolve_output,
			---> int neigh_resolve_output(struct neighbour *neigh, struct sk_buff *skb)
				---> rc = dev_queue_xmit(skb);//通过邻居子系统进入网络设备子系统，然后再到驱动，本地没有继续分析
```
#### 主要数据结构
##### struct sk_buff分析
```c
struct sk_buff {
	union {
		struct {
			/* These two members must be first. */
			struct sk_buff		*next;
			struct sk_buff		*prev;

			union {
				struct net_device	*dev;
				/* Some protocols might use this space to store information,
				 * while device pointer would be NULL.
				 * UDP receive path is one user.
				 */
				unsigned long		dev_scratch;
			};
		};
		struct rb_node		rbnode; /* used in netem, ip4 defrag, and tcp stack */
		struct list_head	list;
	};

	union {
		struct sock		*sk;
		int			ip_defrag_offset;
	};
	.......
};
```
##### struct net_device
struct net_device 结构体表示网络设备，用于管理和控制系统中的物理或虚拟网络设备。它包含了网络设备的各种属性和方法，如MAC地址、IP地址、设备名称等。每个物理或虚拟网卡都对应着一个 struct net_device 结构体。该结构体中包含了一些与网络设备相关的信息和功能，如网络设备的状态、统计信息、操作函数指针等。通过 struct net_device 结构体，内核能够对网络设备进行配置、管理和控制。

##### struct net
struct net 结构体表示网络命名空间，用于将网络资源进行隔离和管理。它是 Linux 内核中的一个重要概念，用于实现多个网络堆栈的并存。在一个系统中可能存在多个网络命名空间，在每个网络命名空间中都有一个独立的 struct net 对象。struct net 结构体中包含了各个网络命名空间的网络资源，如路由表、套接字、网络设备等。通过 struct net 结构体，内核能够对每个网络命名空间中的网络资源进行隔离和管理，并使得多个网络堆栈能够共存和独立运作。

#### ip_rcv函数
首先分析：ip_rcv函数。
这段代码是一个网络接收函数，它接收到的数据包会经过一系列处理后再返回。
首先，定义了一些变量：`skb` 是 socket buffer，用于存放接收到的数据包；`dev` 是设备，可能是物理设备或者虚拟设备；`pt` 是 packet type，表示数据包类型；`orig_dev` 是原始设备，可能与 `dev` 相同，也可能不同。
然后，获取网络设备的网络命名空间 `net`。
接下来，调用 `ip_rcv_core` 函数处理数据包，并将结果赋值给 `skb`。如果处理后 `skb` 为空，则返回 `NET_RX_DROP`，表示丢弃此数据包。
最后，调用 `NF_HOOK` 函数进行网络过滤。这个函数会根据配置的规则对数据包进行过滤和修改，然后调用 `ip_rcv_finish` 函数进行最后的处理。
总的来说，这段代码的主要工作是接收网络数据包，并进行一系列的处理。

```c
net/ipv4/ip_input.c
int ip_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt,
	   struct net_device *orig_dev)
{
	struct net *net = dev_net(dev);

	skb = ip_rcv_core(skb, net);
	if (skb == NULL)
		return NET_RX_DROP;
	return NF_HOOK(NFPROTO_IPV4, NF_INET_PRE_ROUTING,
		       net, NULL, skb, dev, NULL,
		       ip_rcv_finish);
}

static inline int
NF_HOOK(uint8_t pf, unsigned int hook, struct net *net, struct sock *sk, struct sk_buff *skb,
	struct net_device *in, struct net_device *out,
	int (*okfn)(struct net *, struct sock *, struct sk_buff *))
{
	int ret = nf_hook(pf, hook, net, sk, skb, in, out, okfn);
	if (ret == 1)
		ret = okfn(net, sk, skb);
	return ret;
}
```
在 Linux 内核中，ip_rcv_core 函数是用于处理接收的IP数据包的核心函数。它是 IP 层的一个重要函数，位于网络层实现的一系列函数中。
ip_rcv_core 函数的功能主要包括以下几个方面：
1. 解析和验证接收到的 IP 数据包的头部。它会检查 IP 头部的版本号、IP 头部长度、校验和等字段的正确性，以确保接收到的数据包是符合 IP 协议规范的。
2. 根据目标 IP 地址和系统网络配置，确定接收到的数据包应该交给哪个上层协议处理。例如，根据目标 IP 地址判断是应该交给 TCP 协议或 UDP 协议处理。
3. 进行数据包的分片和重新组装。在 IP 协议中，当数据包过大时会进行分片，ip_rcv_core 函数会负责将这些分片数据包进行重新组装，还原成原始的数据包。
4. 处理 IP 数据包的路由。根据接收到的 IP 数据包的目标 IP 地址，ip_rcv_core 函数会进行路由查找，确定数据包的下一跳和出接口，以便进行转发或本地处理。
5. 将数据包传递给上层协议进行处理。一旦确定了数据包的上层协议（如 TCP、UDP），ip_rcv_core 函数会将数据包传递给相应的上层协议处理函数，继续完成后续的处理逻辑。
总之，ip_rcv_core 函数是 Linux 内核中负责接收、验证、路由和传递 IP 数据包的核心函数，它在网络层起着重要的作用，保证了网络通信的正常进行。

### UDP层处理流程
udp协议层处理：
```c
int udp_rcv(struct sk_buff *skb)
---> int __udp4_lib_rcv()
	---> ret = udp_unicast_rcv_skb(sk, skb, uh);
		---> ret = udp_queue_rcv_skb(sk, skb); //net/ipv4/udp.c
			---> return udp_queue_rcv_one_skb(sk, skb);
				---> return __udp_queue_rcv_skb(sk, skb);
					---> rc = __udp_enqueue_schedule_skb(sk, skb);
						---> __skb_queue_tail(list, skb);
							---> __skb_queue_before(list, (struct sk_buff *)list, newsk);
								---> __skb_insert(newsk, next->prev, next, list); //include/linux/skbuff.h
```
### tcp处理流程
void ip_protocol_deliver_rcu(struct net *net, struct sk_buff *skb, int protocol)函数中调用ret = ipprot->handler(skb)；对应的是.handler	=	tcp_v4_rcv,如下：
```c
static struct net_protocol tcp_protocol = {
	.early_demux	=	tcp_v4_early_demux,
	.early_demux_handler =  tcp_v4_early_demux,
	.handler	=	tcp_v4_rcv,
	.err_handler	=	tcp_v4_err,
	.no_policy	=	1,
	.netns_ok	=	1,
	.icmp_strict_tag_validation = 1,
};

```
`tcp_v4_rcv`主要流程:（将接受的数据包放入对应的接收队列中）
```c
int tcp_v4_rcv(struct sk_buff *skb)
	---> int tcp_v4_do_rcv(struct sock *sk, struct sk_buff *skb)
		---> tcp_rcv_established(sk, skb);// 处理已经连接的tcp数据
			---> eaten = tcp_queue_rcv(sk, skb, &fragstolen); // 接收数据到队列中
				---> __skb_queue_tail(&sk->sk_receive_queue, skb);
					---> __skb_queue_before(list, (struct sk_buff *)list, newsk);
						---> __skb_insert(newsk, next->prev, next, list);
		---> int tcp_child_process(struct sock *parent, struct sock *child,) //主要功能是处理TCP连接接收到的数据，更新连接的状态，并在需要时发送信号给父socket。
			---> tcp_segs_in(tcp_sk(child), skb);
			---> int tcp_rcv_state_process(struct sock *sk, struct sk_buff *skb)

				---> tcp_data_queue(sk, skb);// tcp_data_queue 函数的主要目的是将数据包加入到相应的 TCP 接收队列中，以供接收端进程使用。
```
在tcp_rcv_established中通过调用tcp_queue_rcv函数中完成了将接收数据放到socket的接收队列上。
调用tcp_queue_rcv接收完成之后，解这再调用sk_data_ready来唤醒socket上等待的用户进程。
```c
static inline void __skb_insert(struct sk_buff *newsk,
				struct sk_buff *prev, struct sk_buff *next,
				struct sk_buff_head *list)
{
	newsk->next = next;
	newsk->prev = prev;
	next->prev  = prev->next = newsk;
	list->qlen++;
}
```

系统调用流程：
```c
int tcp_recvmsg(struct sock *sk, ····)://net/ipv4/tcp.c
---> skb_queue_walk(&sk->sk_receive_queue, skb)
---> err = skb_copy_datagram_msg(skb, offset, msg, used);
---> put_cmsg(msg, SOL_TCP, TCP_CM_INQ, sizeof(inq), &inq);

	if (has_cmsg) {
		if (has_tss)
			tcp_recv_timestamp(msg, sk, &tss);
		if (tp->recvmsg_inq) {
			inq = tcp_inq_hint(sk);
			put_cmsg(msg, SOL_TCP, TCP_CM_INQ, sizeof(inq), &inq);
		}
	}
	---> 从这里开始获取skb处理，如何理解skb_queue_walk(&sk->sk_receive_queue, skb)？ 最终用户进程发现有可用数据，就会读取对应的队列数据。
```


## socket用户层系统调用

### udp流程：
```c
net/ipv4/raw.c：
int raw_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int noblock, int flags, int *addr_len)
---> sk_buff *skb_recv_datagram(struct sock *sk, unsigned int flags, int noblock, int *err): net/core/datagram.c
	---> struct sk_buff *__skb_recv_datagram():net/core/datagram.c
	 	---> struct sk_buff *__skb_try_recv_datagram():net/core/datagram.c
			----> struct sk_buff *__skb_try_recv_from_queue():net/core/datagram.c
				----> *last = queue->prev;
				----> skb_queue_walk(queue, skb) :// include/linux/skbuff.h

#define skb_queue_walk(queue, skb) \
		for (skb = (queue)->next;					\
		     skb != (struct sk_buff *)(queue);				\
		     skb = skb->next)
```

其中的公共流程分析：


# 数据发送流程
数据从用户进程到网卡详细分析，以tcp协议栈的系统调用为例：

## IP层及以下发送数据包的流程简述：
当L4层有数据包待发送的时候，它将调用ip_append_data/ip_push_pengding_frams(udp,icmp,Raw IP),或ip_append_page(UDP),ip_queue_xmit(TCP,SCTP),或者raw_send_hdrinc(Raw IP,IGMP),它们将这些包交流NETFILTER处理后，然后交给dst_output,这些根据是多播或单播选择合适的发送函数。如果是单播，它会调用ip_output，然后调用ip_finish_output,这个函数主要是检查待发送的数据包是否超过MTU，如果是，则要首先调用ip_fragment将其分片，然后再传给ip_finish_output2，由它交给链路层处理了。

### ip_queue_xmit()
1. 查找路由信息
2. 填充IP头信息，这里用skb_push为IP头留出空间
3. 创建选项部分和计算校验
4. 调用dst_output

### ip_append_data(ip_append_page)/ ip_push_pending_frames
这是一对由 UDP 等协议使用的一对函数,ip_append_data 是一个比较复杂的函数,它主要将收到的大数据包分成多个小于 MTU(1500)的skb,为L3层要实现的 IP 分片做准备。

例如如果待发送的数据包大小为4000字节,假设先前sock中的队列又非空(sk->sk_write_queue!=NULL),(因为 ip_append_data 可以被 L4 层多次调用,用于添加数据。)并且之前一个 skb 还没填满,剩余大小为 500 字节。这时,当 L4 层调用 ip_append_data 时,它首先将这个剩余的 skb 填满,这里还有一个问题就是关于 scatter/gather IO 的,当 NIC 不支持时,它会直接将数据写到这个 skb->tail 处,但是,如果 NIC 支持这种 IO,它便会将数据写到 frags 所指向的指针中,如果相关的 page 已经填满,它会再分配一个新的 page 用于这个 skb。这一步完成之后,ip_append_data 再次进入下三次循环,每次循环都分配一个 skb,并将数据通过 getfrag 从 L4 层复制下来。在循环结束之前,它通过__skb_queue_tail(&sk->sk_write_queue,skb),将这个 skb 链入这个 sock 的 sk_write_queue 队列中去。待到这个循环结束时,所有的数据都从 L4 复制到各个 skb,并链入了它的 sk_write_queue 队列。换言之,待发送数据已经在sk_write_queue 队列中了。

ip_append_page() 跟 ip_append_data()是实现同样功能的函数, 它们的不同在于ip_append_data()需要将用户空间的数据复制到内核空间,而 ip_append_page()则不需要这个复制,它直接使用用户提供的数据,从而实现了数据的“零拷贝”。

### dst_output()
由开始的全景图可以看到,对于单播 IP 来说,它执行的是 ip_output():

由 ip_finish_output 可以看到,如果 IP 数据包的长度大于 MTU,它便会先将 IP 分成合适大小的分片,最后调用 ip_finish_output2(),将这个包路由出去。

# IP的分片与重装

## IP分片
IP分片主要是协议栈IP层实现。通常情况下，IP数据包是有大小限制的，这是由网络设备、协议和网络环境等因素决定的。如果IP数据包的大小超过了网络限制，那么它就需要进行分片，以便能够在网络中正确传输。

在发生需要分片的情况下，IP层负责进行数据包的分片和重组。发送端的IP层根据网络的MTU值限制进行分片，将这些独立的传输给目的地址。而接收端的IP层则负载接收分片，并将这些分片按照顺序进行重组，还原为原始的数据包。

```c
static int ip_fragment(struct net *net, struct sock *sk, struct sk_buff *skb,
		       unsigned int mtu,
		       int (*output)(struct net *, struct sock *, struct sk_buff *))
{
	struct iphdr *iph = ip_hdr(skb);

	if ((iph->frag_off & htons(IP_DF)) == 0)
		return ip_do_fragment(net, sk, skb, output);

	if (unlikely(!skb->ignore_df ||
		     (IPCB(skb)->frag_max_size &&
		      IPCB(skb)->frag_max_size > mtu))) {
		IP_INC_STATS(net, IPSTATS_MIB_FRAGFAILS);
		icmp_send(skb, ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED,
			  htonl(mtu));
		kfree_skb(skb);
		return -EMSGSIZE;
	}

	return ip_do_fragment(net, sk, skb, output);
}
```

## IP重装
ip_defrag函数
ip_defrag当L3层在将数据包上传给L4层时候，它如果发现这个数据包只是一个IP分片，那么将调用ip_defrag这个函数，将这个分片暂时队列起来，如果所有的分片到齐，它将这些分片重组一个完整的IP包上传给L4层。
