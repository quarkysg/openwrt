/*
 **************************************************************************
 * Copyright (c) 2015, 2016, The Linux Foundation. All rights reserved.
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
 * nss_tstamp.c
 *	NSS Tstamp APIs
 */

#include "nss_tx_rx_common.h"
#include <linux/etherdevice.h>

static struct net_device_stats *nss_tstamp_ndev_stats(struct net_device *ndev);

static const struct net_device_ops nss_tstamp_ndev_ops = {
	.ndo_get_stats = nss_tstamp_ndev_stats,
};

struct nss_tstamp_data {
	uint32_t ts_ifnum;	/* time stamp interface number */
	uint32_t ts_data_lo;	/* time stamp lower order bits */
	uint32_t ts_data_hi;	/* time stamp higher order bits */

	uint8_t ts_tx;		/* time stamp direction */
	uint8_t ts_hdr_sz;	/* padding bytes */
};

/*
 * nss_tstamp_ndev_setup()
 *	Dummy setup for net_device handler
 */
static void nss_tstamp_ndev_setup(struct net_device *ndev)
{
	return;
}

/*
 * nss_tstamp_ndev_stats()
 *	Return net device stats
 */
static struct net_device_stats *nss_tstamp_ndev_stats(struct net_device *ndev)
{
	return &ndev->stats;
};

/*
 * nss_tstamp_copy_data()
 *	Copy timestamps from received nss frame into skb
 */
static void nss_tstamp_copy_data(struct nss_tstamp_data *ntm, struct sk_buff *skb)
{
	struct skb_shared_hwtstamps *tstamp;

	tstamp = skb_hwtstamps(skb);
	tstamp->hwtstamp = ktime_set(ntm->ts_data_hi, ntm->ts_data_lo);
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3, 16, 0))
	tstamp->syststamp = ktime_set(ntm->ts_data_hi, ntm->ts_data_lo);
#endif
}

/*
 * nss_tstamp_buf_receive()
 * 	Receive nss exception packets.
 */
static void nss_tstamp_buf_receive(struct net_device *ndev, struct sk_buff *skb, struct napi_struct *napi)
{
	struct nss_tstamp_data *ntm = (struct nss_tstamp_data *)skb->data;
	struct nss_ctx_instance *nss_ctx;
	struct net_device *dev;
	uint32_t tstamp_sz;

	BUG_ON(!ntm);
	tstamp_sz = ntm->ts_hdr_sz;

	nss_ctx = &nss_top_main.nss[nss_top_main.tstamp_handler_id];
	BUG_ON(!nss_ctx);

	skb_pull(skb, tstamp_sz);

	dev = nss_cmn_get_interface_dev(nss_ctx, ntm->ts_ifnum);
	if (!dev) {
		nss_warning("Tstamp: Invalid net device\n");
		dev_kfree_skb_any(skb);
		return;
	}

	skb->dev = dev;

	/*
	 * copy the time stamp and convert into ktime_t
	 */
	nss_tstamp_copy_data(ntm, skb);

	if (unlikely(ntm->ts_tx)) {
		/*
		 * We are in TX Path
		 */
		skb_tstamp_tx(skb, skb_hwtstamps(skb));

		ndev->stats.tx_packets++;
		ndev->stats.tx_bytes += skb->len;

		dev_kfree_skb_any(skb);
		return;
	}

	/*
	 * We are in RX Path
	 */
	switch(dev->type) {
	case NSS_IPSEC_ARPHRD_IPSEC:
		/*
		 * It seems like the data came over IPsec, hence indicate
		 * it to the Linux over this interface
		 */
		skb_reset_network_header(skb);
		skb_reset_mac_header(skb);

		skb->pkt_type = PACKET_HOST;
		skb->protocol = cpu_to_be16(ETH_P_IP);
		skb->skb_iif = dev->ifindex;
		break;

	default:
		/*
		 * This is a plain non-encrypted data packet.
		 */
		skb->protocol = eth_type_trans(skb, dev);
		break;
	}

	netif_receive_skb(skb);

	ndev->stats.rx_packets++;
	ndev->stats.rx_bytes += skb->len;
}

/*
 * nss_tstamp_register_netdev()
 */
struct net_device *nss_tstamp_register_netdev(void)
{
	struct net_device *ndev;
	uint32_t err = 0;

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3, 16, 0))
	ndev = alloc_netdev(sizeof(struct netdev_priv_instance), "qca-nss-tstamp", nss_tstamp_ndev_setup);
#else
	ndev = alloc_netdev(sizeof(struct netdev_priv_instance), "qca-nss-tstamp", NET_NAME_ENUM, nss_tstamp_ndev_setup);
#endif
	if (!ndev) {
		nss_warning("Tstamp: Could not allocate tstamp net_device ");
		return NULL;
	}

	ndev->netdev_ops = &nss_tstamp_ndev_ops;

	err = register_netdev(ndev);
	if (err) {
		nss_warning("Tstamp: Could not register tstamp net_device ");
		free_netdev(ndev);
		return NULL;
	}

	return ndev;
}

/*
 * nss_tstamp_register_handler()
 */
void nss_tstamp_register_handler(struct net_device *ndev)
{
	uint32_t features = 0;
	struct nss_ctx_instance *nss_ctx;

	nss_ctx = &nss_top_main.nss[nss_top_main.tstamp_handler_id];
	nss_ctx->subsys_dp_register[NSS_TSTAMP_INTERFACE].cb = nss_tstamp_buf_receive;
	nss_ctx->subsys_dp_register[NSS_TSTAMP_INTERFACE].app_data = NULL;
	nss_ctx->subsys_dp_register[NSS_TSTAMP_INTERFACE].ndev = ndev;
	nss_ctx->subsys_dp_register[NSS_TSTAMP_INTERFACE].features = features;
}


