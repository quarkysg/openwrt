/*
 **************************************************************************
 * Copyright (c) 2014, 2016, The Linux Foundation. All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

/*
 * nss_tun6rd.c
 *
 * This file is the NSS 6rd tunnel module
 * ------------------------REVISION HISTORY-----------------------------
 * Qualcomm Atheros         15/sep/2013              Created
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/of.h>
#include <linux/tcp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/ipv6.h>
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,9,0))
#include <net/ipip.h>
#else
#include <net/ip_tunnels.h>
#endif
#include <linux/if_arp.h>
#include <nss_api_if.h>
#include <nss_dynamic_interface.h>

/*
 * NSS tun6rd debug macros
 */
#if (NSS_TUN6RD_DEBUG_LEVEL < 1)
#define nss_tun6rd_assert(fmt, args...)
#else
#define nss_tun6rd_assert(c) if (!(c)) { BUG_ON(!(c)); }
#endif

#if defined(CONFIG_DYNAMIC_DEBUG)
/*
 * Compile messages for dynamic enable/disable
 */
#define nss_tun6rd_warning(s, ...) pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define nss_tun6rd_info(s, ...) pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define nss_tun6rd_trace(s, ...) pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else

/*
 * Statically compile messages at different levels
 */
#if (NSS_TUN6RD_DEBUG_LEVEL < 2)
#define nss_tun6rd_warning(s, ...)
#else
#define nss_tun6rd_warning(s, ...) pr_warn("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#if (NSS_TUN6RD_DEBUG_LEVEL < 3)
#define nss_tun6rd_info(s, ...)
#else
#define nss_tun6rd_info(s, ...)   pr_notice("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#if (NSS_TUN6RD_DEBUG_LEVEL < 4)
#define nss_tun6rd_trace(s, ...)
#else
#define nss_tun6rd_trace(s, ...)  pr_info("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif
#endif

/*
 * 6rd tunnel stats
 */
struct nss_tun6rd_stats {
	uint32_t rx_packets;	/* Number of received packets */
	uint32_t rx_bytes;	/* Number of received bytes */
	uint32_t tx_packets;	/* Number of transmitted packets */
	uint32_t tx_bytes;	/* Number of transmitted bytes */
};

/*
 * nss_tun6rd_update_dev_stats
 *	Update the Dev stats received from NetAp
 */
static void nss_tun6rd_update_dev_stats(struct net_device *dev,
					struct nss_tun6rd_sync_stats_msg *sync_stats)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
	struct pcpu_sw_netstats stats;

	u64_stats_init(&stats.syncp);
	u64_stats_update_begin(&stats.syncp);
	stats.rx_packets = sync_stats->node_stats.rx_packets;
	stats.rx_bytes = sync_stats->node_stats.rx_bytes;
	stats.tx_packets = sync_stats->node_stats.tx_packets;
	stats.tx_bytes = sync_stats->node_stats.tx_bytes;
	u64_stats_update_end(&stats.syncp);
#else
	struct nss_tun6rd_stats stats;

	stats.rx_packets = sync_stats->node_stats.rx_packets;
	stats.rx_bytes = sync_stats->node_stats.rx_bytes;
	stats.tx_packets = sync_stats->node_stats.tx_packets;
	stats.tx_bytes = sync_stats->node_stats.tx_bytes;
#endif

	ipip6_update_offload_stats(dev, (void *)&stats);
}

/*
 * nss_tun6rd_event_receive()
 *	Event Callback to receive events from NSS
 */
static void nss_tun6rd_event_receive(void *if_ctx, struct nss_tun6rd_msg *tnlmsg)
{
	struct net_device *netdev = if_ctx;

	switch (tnlmsg->cm.type) {
	case NSS_TUN6RD_RX_STATS_SYNC:
		nss_tun6rd_update_dev_stats(netdev, (struct nss_tun6rd_sync_stats_msg *)&tnlmsg->msg.stats);
		break;

	default:
		nss_tun6rd_info("Unknown Event from NSS\n");
		break;
	}
}

/*
 * nss_tun6rd_exception()
 *	Exception handler registered to NSS driver
 */
static void nss_tun6rd_exception(struct net_device *dev, struct sk_buff *skb, __attribute__((unused)) struct napi_struct *napi)
{
	const struct iphdr *iph;

	skb->dev = dev;
	nss_tun6rd_info("received - %d bytes name %s ver %x\n",
			skb->len,dev->name,skb->data[0]);

	iph = (const struct iphdr *)skb->data;

	/*
	 * Packet after Decap/Encap Did not find the Rule.
	 */
	if (iph->version != 4) {
		skb->protocol = htons(ETH_P_IPV6);
	} else {
		if (iph->protocol == IPPROTO_IPV6) {
			skb_pull(skb, sizeof(struct iphdr));
			skb->protocol = htons(ETH_P_IPV6);
			skb_reset_network_header(skb);
			skb->pkt_type = PACKET_HOST;
			skb->ip_summed = CHECKSUM_NONE;
			dev_queue_xmit(skb);
			return;
		}
		skb->protocol = htons(ETH_P_IP);
	}

	skb_reset_network_header(skb);
	skb->pkt_type = PACKET_HOST;
	skb->skb_iif = dev->ifindex;
	skb->ip_summed = CHECKSUM_NONE;
	netif_receive_skb(skb);
}

/*
 * nss_tun6rd_dev_up()
 *	6RD Tunnel device i/f up handler
 */
static int nss_tun6rd_dev_up(struct net_device *netdev)
{
	struct ip_tunnel *tunnel;
	struct ip_tunnel_6rd_parm *ip6rd;
	const struct iphdr  *tiph;
	struct nss_tun6rd_msg tun6rdmsg;
	struct nss_tun6rd_attach_tunnel_msg *tun6rdcfg;
	uint32_t if_number;
	nss_tx_status_t status;
	struct nss_ctx_instance *nss_ctx;
	uint32_t features = 0; /* features denote the skb types supported by this interface */

	/*
	 * Validate netdev for ipv6-in-ipv4  Tunnel
	 */
	if (netdev->type != ARPHRD_SIT) {
		return NOTIFY_DONE;
	}

	tunnel = (struct ip_tunnel *)netdev_priv(netdev);
	ip6rd =  &tunnel->ip6rd;

	/*
	 * Valid 6rd Tunnel Check
	 * 1. 6rd Prefix len should be non zero
	 * 2. Relay prefix length should not be greater then 32
	 * 3. To allow for stateless address auto-configuration on the CE LAN side,
	 *    6rd delegated prefix SHOULD be /64 or shorter.
	 */
	if ((ip6rd->prefixlen == 0 )
			|| (ip6rd->relay_prefixlen > 32)
			|| (ip6rd->prefixlen
				+ (32 - ip6rd->relay_prefixlen) > 64)) {

		nss_tun6rd_warning("Invalid 6rd argument prefix len %d relayprefix len %d \n",
				ip6rd->prefixlen,ip6rd->relay_prefixlen);
		return NOTIFY_BAD;
	}

	nss_tun6rd_info("Valid 6rd Tunnel Prefix %x %x %x %x\n Prefix len %d relay_prefix %d relay_prefixlen %d \n",
			ip6rd->prefix.s6_addr32[0],ip6rd->prefix.s6_addr32[1],
			ip6rd->prefix.s6_addr32[2],ip6rd->prefix.s6_addr32[3],
			ip6rd->prefixlen, ip6rd->relay_prefix,
			ip6rd->relay_prefixlen);

	/*
	 * Find the Tunnel device IP header info
	 */
	tiph = &tunnel->parms.iph ;
	nss_tun6rd_trace("Tunnel Param srcaddr %x daddr %x ttl %d tos %x\n",
			tiph->saddr, tiph->daddr,tiph->ttl,tiph->tos);

	if (tiph->saddr == 0) {
		nss_tun6rd_warning("Tunnel src address not configured %x\n",
				tiph->saddr);
		return NOTIFY_BAD;
	}

	if_number = nss_dynamic_interface_alloc_node(NSS_DYNAMIC_INTERFACE_TYPE_TUN6RD);
	if (-1 == if_number) {
		nss_tun6rd_warning("Request interface number failed\n");
		return NOTIFY_BAD;
	}

	if (!nss_is_dynamic_interface(if_number)) {
		nss_tun6rd_warning("Invalid NSS dynamic I/F "
				   "number %d \n", if_number);

		return NOTIFY_BAD;
	}

	/*
	 * Register 6rd tunnel with NSS
	 */
	nss_ctx = nss_register_tun6rd_if(if_number,
				nss_tun6rd_exception,
				nss_tun6rd_event_receive,
				netdev,
				features);
	if (!nss_ctx) {
		status = nss_dynamic_interface_dealloc_node(if_number, NSS_DYNAMIC_INTERFACE_TYPE_TUN6RD);
		if (status != NSS_TX_SUCCESS) {
			nss_tun6rd_warning("Unable to dealloc the node[%d] in the NSS fw!\n", if_number);
		}
		nss_tun6rd_trace("nss_register_tun6rd_if failed \n");
		return NOTIFY_BAD;
	}

	/*
	 * Prepare The Tunnel configuration parameter to send to nss
	 */
	memset(&tun6rdmsg, 0, sizeof(struct nss_tun6rd_msg));
	tun6rdcfg = &tun6rdmsg.msg.tunnel;
	tun6rdcfg->prefixlen = ip6rd->prefixlen;
	tun6rdcfg->relay_prefix = ip6rd->relay_prefix;
	tun6rdcfg->relay_prefixlen = ip6rd->relay_prefixlen;
	tun6rdcfg->saddr = ntohl(tiph->saddr);
	tun6rdcfg->daddr = ntohl(tiph->daddr);
	tun6rdcfg->prefix[0] = ntohl(ip6rd->prefix.s6_addr32[0]);
	tun6rdcfg->prefix[1] = ntohl(ip6rd->prefix.s6_addr32[1]);
	tun6rdcfg->prefix[2] = ntohl(ip6rd->prefix.s6_addr32[2]);
	tun6rdcfg->prefix[3] = ntohl(ip6rd->prefix.s6_addr32[3]);
	tun6rdcfg->ttl = tiph->ttl;
	tun6rdcfg->tos = tiph->tos;

	nss_tun6rd_trace(" 6rd Tunnel info\n");
	nss_tun6rd_trace(" saddr %x daddr %d ttl %x  tos %x\n",
			tiph->saddr, tiph->daddr, tiph->ttl, tiph->tos);
	nss_tun6rd_trace(" Prefix %x:%x:%x:%x  Prefix len %d\n",
			ip6rd->prefix.s6_addr32[0], ip6rd->prefix.s6_addr32[1],
			ip6rd->prefix.s6_addr32[2], ip6rd->prefix.s6_addr32[3],
			ip6rd->prefixlen);
	nss_tun6rd_trace("Relay Prefix %x Len %d\n",
			ip6rd->relay_prefix, ip6rd->relay_prefixlen);

	nss_tun6rd_trace("Sending 6rd tunnel i/f up command to NSS %x\n",
			(int)nss_ctx);

	/*
	 * Send 6rd Tunnel UP command to NSS
	 */
	nss_tun6rd_msg_init(&tun6rdmsg, if_number, NSS_TUN6RD_ATTACH_PNODE,
			sizeof(struct nss_tun6rd_attach_tunnel_msg), NULL, NULL);

	status = nss_tun6rd_tx(nss_ctx, &tun6rdmsg);
	if (status != NSS_TX_SUCCESS) {
		nss_unregister_tun6rd_if(if_number);
		status = nss_dynamic_interface_dealloc_node(if_number, NSS_DYNAMIC_INTERFACE_TYPE_TUN6RD);
		if (status != NSS_TX_SUCCESS) {
			nss_tun6rd_warning("Unable to dealloc the node[%d] in the NSS fw!\n", if_number);
		}
		nss_tun6rd_warning("Tunnel up command error %d\n", status);
		return NOTIFY_BAD;
	}

	return NOTIFY_DONE;
}

/*
 * nss_tun6rd_dev_down()
 *	6RD Tunnel device i/f down handler
 */
static int nss_tun6rd_dev_down(struct net_device *netdev)
{
	struct ip_tunnel *tunnel;
	struct ip_tunnel_6rd_parm *ip6rd;
	int32_t if_number;
	nss_tx_status_t status;

	/*
	 * Validate netdev for ipv6-in-ipv4  Tunnel
	 */
	if (netdev->type != ARPHRD_SIT) {
		return NOTIFY_DONE;
	}

	/*
	 * Check if tunnel 6rd is registered ?
	 */
	if_number = nss_cmn_get_interface_number_by_dev(netdev);
	if (if_number < 0) {
		nss_tun6rd_warning("Net device:%p is not registered \n",netdev);
		return NOTIFY_BAD;
	}


	tunnel = (struct ip_tunnel *)netdev_priv(netdev);
	ip6rd =  &tunnel->ip6rd;

	/*
	 * Valid 6rd Tunnel Check
	 */
	if ((ip6rd->prefixlen == 0 )
			|| (ip6rd->relay_prefixlen > 32 )
			|| (ip6rd->prefixlen
				+ (32 - ip6rd->relay_prefixlen) > 64)) {

		nss_tun6rd_warning("Invalid 6rd argument prefix len %d relayprefix len %d \n",
				ip6rd->prefixlen,ip6rd->relay_prefixlen);
		return NOTIFY_BAD;
	}

	/*
	 * Un-Register 6rd tunnel with NSS
	 */
	nss_unregister_tun6rd_if(if_number);
	status = nss_dynamic_interface_dealloc_node(if_number, NSS_DYNAMIC_INTERFACE_TYPE_TUN6RD);
	if (status != NSS_TX_SUCCESS) {
		nss_tun6rd_warning("Dealloc node failure\n");
		return NOTIFY_BAD;
	}

	return NOTIFY_DONE;
}


/*
 * nss_tun6rd_dev_event()
 *	Net device notifier for 6rd module
 */
static int nss_tun6rd_dev_event(struct notifier_block  *nb,
		unsigned long event, void  *dev)
{
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0))
	struct net_device *netdev = (struct net_device *)dev;
#else
	struct net_device *netdev = netdev_notifier_info_to_dev(dev);
#endif

	switch (event) {
	case NETDEV_UP:
		nss_tun6rd_trace("NETDEV_UP: event %lu name %s\n", event, netdev->name);
		return nss_tun6rd_dev_up(netdev);

	case NETDEV_DOWN:
		nss_tun6rd_trace("NETDEV_DOWN: event %lu name %s\n", event, netdev->name);
		return nss_tun6rd_dev_down(netdev);

	default:
		nss_tun6rd_trace("Unhandled notifier event %lu name %s\n",event, netdev->name);
		break;
	}

	return NOTIFY_DONE;
}

/*
 * Linux Net device Notifier
 */
struct notifier_block nss_tun6rd_notifier = {
	.notifier_call = nss_tun6rd_dev_event,
};


/*
 * nss_tun6rd_init_module()
 *	Tunnel 6rd module init function
 */
int __init nss_tun6rd_init_module(void)
{
#ifdef CONFIG_OF
	/*
	 * If the node is not compatible, don't do anything.
	 */
	if (!of_find_node_by_name(NULL, "nss-common")) {
		return 0;
	}
#endif
	nss_tun6rd_info("module (platform - IPQ806x , %s) loaded\n",
			NSS_CLIENT_BUILD_ID);

	register_netdevice_notifier(&nss_tun6rd_notifier);
	nss_tun6rd_trace("Netdev Notifier registerd \n");

	return 0;
}

/*
 * nss_tun6rd_exit_module()
 *	Tunnel 6rd module exit function
 */
void __exit nss_tun6rd_exit_module(void)
{
#ifdef CONFIG_OF
	/*
	 * If the node is not compatible, don't do anything.
	 */
	if (!of_find_node_by_name(NULL, "nss-common")) {
		return;
	}
#endif
	unregister_netdevice_notifier(&nss_tun6rd_notifier);
	nss_tun6rd_info("module unloaded\n");
}

module_init(nss_tun6rd_init_module);
module_exit(nss_tun6rd_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("NSS tun6rd offload manager");
