/*
 **************************************************************************
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
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
 * nss_ipv6_reasm.c
 *	NSS IPv6 Reassembly APIs
 */
#include "nss_tx_rx_common.h"

/*
 * nss_ipv6_reasm_stats_sync()
 *	Update driver specific information from the messsage.
 */
static void nss_ipv6_reasm_stats_sync(struct nss_ctx_instance *nss_ctx, struct nss_ipv6_reasm_stats_sync *nirs)
{
	struct nss_top_instance *nss_top = nss_ctx->nss_top;

	spin_lock_bh(&nss_top->stats_lock);

	/*
	 * Common node stats
	 */
	nss_top->stats_node[NSS_IPV6_REASM_INTERFACE][NSS_STATS_NODE_RX_PKTS] += nirs->node_stats.rx_packets;
	nss_top->stats_node[NSS_IPV6_REASM_INTERFACE][NSS_STATS_NODE_RX_BYTES] += nirs->node_stats.rx_bytes;
	nss_top->stats_node[NSS_IPV6_REASM_INTERFACE][NSS_STATS_NODE_RX_DROPPED] += nirs->node_stats.rx_dropped;
	nss_top->stats_node[NSS_IPV6_REASM_INTERFACE][NSS_STATS_NODE_TX_PKTS] += nirs->node_stats.tx_packets;
	nss_top->stats_node[NSS_IPV6_REASM_INTERFACE][NSS_STATS_NODE_TX_BYTES] += nirs->node_stats.tx_bytes;

	/*
	 * IPv6 reasm node stats
	 */
	nss_top->stats_ipv6_reasm[NSS_STATS_IPV6_REASM_ALLOC_FAILS] += nirs->ipv6_reasm_alloc_fails;
	nss_top->stats_ipv6_reasm[NSS_STATS_IPV6_REASM_TIMEOUTS] += nirs->ipv6_reasm_timeouts;
	nss_top->stats_ipv6_reasm[NSS_STATS_IPV6_REASM_DISCARDS] += nirs->ipv6_reasm_discards;

	spin_unlock_bh(&nss_top->stats_lock);
}

/*
 * nss_ipv6_reasm_msg_handler()
 *	Handle NSS -> HLOS messages for IPv6 reasm
 */
static void nss_ipv6_reasm_msg_handler(struct nss_ctx_instance *nss_ctx, struct nss_cmn_msg *ncm, __attribute__((unused))void *app_data)
{
	struct nss_ipv6_reasm_msg *nim = (struct nss_ipv6_reasm_msg *)ncm;

	BUG_ON(ncm->interface != NSS_IPV6_REASM_INTERFACE);

	switch (nim->cm.type) {
	case NSS_IPV6_REASM_STATS_SYNC_MSG:
		/*
		* Update driver statistics on node sync.
		*/
		nss_ipv6_reasm_stats_sync(nss_ctx, &nim->msg.stats_sync);
		break;
	default:
		nss_warning("IPv6 reasm received an unknown message type");
	}
}

/*
 * nss_ipv6_reasm_register_handler()
 *	Register our handler to receive messages for this interface
 */
void nss_ipv6_reasm_register_handler(void)
{
	if (nss_core_register_handler(NSS_IPV6_REASM_INTERFACE, nss_ipv6_reasm_msg_handler, NULL) != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("IPv6 reasm handler failed to register");
	}
}
