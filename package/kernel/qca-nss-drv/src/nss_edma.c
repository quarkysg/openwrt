/*
 **************************************************************************
 * Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
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
 * nss_edma.c
 *	NSS EDMA APIs
 */

#include "nss_tx_rx_common.h"

/*
 **********************************
 Rx APIs
 **********************************
 */

/*
 * nss_edma_metadata_port_stats_sync()
 *	Handle the syncing of EDMA port statistics.
 */
static void nss_edma_metadata_port_stats_sync(struct nss_ctx_instance *nss_ctx, struct nss_edma_port_stats_sync *nepss)
{
	uint16_t i, j = 0;
	struct nss_top_instance *nss_top = nss_ctx->nss_top;

	spin_lock_bh(&nss_top->stats_lock);

	/*
	 * edma port stats
	 * We process a subset of port stats since msg payload is not enough to hold all ports at once.
	 */
	for (i = nepss->start_port; i < nepss->end_port; i++) {
		nss_top->stats_edma.port[i].port_stats[NSS_STATS_NODE_RX_PKTS] += nepss->port_stats[j].node_stats.rx_packets;
		nss_top->stats_edma.port[i].port_stats[NSS_STATS_NODE_RX_BYTES] += nepss->port_stats[j].node_stats.rx_bytes;
		nss_top->stats_edma.port[i].port_stats[NSS_STATS_NODE_RX_DROPPED] += nepss->port_stats[j].node_stats.rx_dropped;
		nss_top->stats_edma.port[i].port_stats[NSS_STATS_NODE_TX_PKTS] += nepss->port_stats[j].node_stats.tx_packets;
		nss_top->stats_edma.port[i].port_stats[NSS_STATS_NODE_TX_BYTES] += nepss->port_stats[j].node_stats.tx_bytes;

		nss_top->stats_edma.port[i].port_type = nepss->port_stats[j].port_type;
		nss_top->stats_edma.port[i].port_ring_map[NSS_EDMA_PORT_RX_RING] = nepss->port_stats[j].edma_rx_ring;
		nss_top->stats_edma.port[i].port_ring_map[NSS_EDMA_PORT_TX_RING] = nepss->port_stats[j].edma_tx_ring;
		j++;
	}

	spin_unlock_bh(&nss_top->stats_lock);
}

/*
 * nss_edma_metadata_ring_stats_sync()
 *	Handle the syncing of EDMA ring statistics.
 */
static void nss_edma_metadata_ring_stats_sync(struct nss_ctx_instance *nss_ctx, struct nss_edma_ring_stats_sync *nerss)
{
	int32_t i;
	struct nss_top_instance *nss_top = nss_ctx->nss_top;

	spin_lock_bh(&nss_top->stats_lock);

	/*
	 * edma tx ring stats
	 */
	for (i = 0; i < NSS_EDMA_NUM_TX_RING_MAX; i++) {
		nss_top->stats_edma.tx_stats[i][NSS_STATS_EDMA_TX_ERR] += nerss->tx_ring[i].tx_err;
		nss_top->stats_edma.tx_stats[i][NSS_STATS_EDMA_TX_DROPPED] += nerss->tx_ring[i].tx_dropped;
		nss_top->stats_edma.tx_stats[i][NSS_STATS_EDMA_TX_DESC] += nerss->tx_ring[i].desc_cnt;
	}

	/*
	 * edma rx ring stats
	 */
	for (i = 0; i < NSS_EDMA_NUM_RX_RING_MAX; i++) {
		nss_top->stats_edma.rx_stats[i][NSS_STATS_EDMA_RX_CSUM_ERR] += nerss->rx_ring[i].rx_csum_err;
		nss_top->stats_edma.rx_stats[i][NSS_STATS_EDMA_RX_DESC] += nerss->rx_ring[i].desc_cnt;
	}

	/*
	 * edma tx cmpl ring stats
	 */
	for (i = 0; i < NSS_EDMA_NUM_TXCMPL_RING_MAX; i++) {
		nss_top->stats_edma.txcmpl_stats[i][NSS_STATS_EDMA_TXCMPL_DESC] += nerss->txcmpl_ring[i].desc_cnt;
	}

	/*
	 * edma rx fill ring stats
	 */
	for (i = 0; i < NSS_EDMA_NUM_RXFILL_RING_MAX; i++) {
		nss_top->stats_edma.rxfill_stats[i][NSS_STATS_EDMA_RXFILL_DESC] += nerss->rxfill_ring[i].desc_cnt;
	}

	spin_unlock_bh(&nss_top->stats_lock);
}

/*
 * nss_edma_interface_handler()
 *	Handle NSS -> HLOS messages for EDMA node
 */
static void nss_edma_interface_handler(struct nss_ctx_instance *nss_ctx, struct nss_cmn_msg *ncm, __attribute__((unused))void *app_data)
{
	struct nss_edma_msg *nem = (struct nss_edma_msg *)ncm;
	nss_edma_msg_callback_t cb;

	/*
	 * Is this a valid request/response packet?
	 */
	if (nem->cm.type >= NSS_METADATA_TYPE_EDMA_MAX) {
		nss_warning("%p: received invalid message %d for edma interface", nss_ctx, nem->cm.type);
		return;
	}

	/*
	 * Handle different types of messages
	 */
	switch (nem->cm.type) {
	case NSS_METADATA_TYPE_EDMA_PORT_STATS_SYNC:
		nss_edma_metadata_port_stats_sync(nss_ctx, &nem->msg.port_stats);
		break;

	case NSS_METADATA_TYPE_EDMA_RING_STATS_SYNC:
		nss_edma_metadata_ring_stats_sync(nss_ctx, &nem->msg.ring_stats);
		break;

	default:
		if (ncm->response != NSS_CMN_RESPONSE_ACK) {
			/*
			 * Check response
			 */
			nss_info("%p: Received response %d for type %d, interface %d",
						nss_ctx, ncm->response, ncm->type, ncm->interface);
		}
	}

	/*
	 * Update the callback and app_data for NOTIFY messages, edma sends all notify messages
	 * to the same callback/app_data.
	 */
	if (nem->cm.response == NSS_CMM_RESPONSE_NOTIFY) {
		ncm->cb = (nss_ptr_t)nss_ctx->nss_top->edma_callback;
		ncm->app_data = (nss_ptr_t)nss_ctx->nss_top->edma_ctx;
	}

	/*
	 * Do we have a callback?
	 */
	if (!ncm->cb) {
		return;
	}

	/*
	 * Callback
	 */
	cb = (nss_edma_msg_callback_t)ncm->cb;
	cb((void *)ncm->app_data, nem);
}

/*
 * nss_edma_notify_register()
 *	Register to received EDMA events.
 */
struct nss_ctx_instance *nss_edma_notify_register(nss_edma_msg_callback_t cb, void *app_data)
{
	nss_top_main.edma_callback = cb;
	nss_top_main.edma_ctx = app_data;
	return &nss_top_main.nss[nss_top_main.edma_handler_id];
}
EXPORT_SYMBOL(nss_edma_notify_register);

/*
 * nss_edma_notify_unregister()
 *	Unregister to received EDMA events.
 */
void nss_edma_notify_unregister(void)
{
	nss_top_main.edma_callback = NULL;
}
EXPORT_SYMBOL(nss_edma_notify_unregister);

/*
 * nss_edma_register_handler()
 */
void nss_edma_register_handler(void)
{
	nss_core_register_handler(NSS_EDMA_INTERFACE, nss_edma_interface_handler, NULL);
}
