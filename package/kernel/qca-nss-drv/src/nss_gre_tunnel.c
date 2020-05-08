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

#include "nss_tx_rx_common.h"
#include "nss_gre_tunnel_stats.h"

#define NSS_GRE_TUNNEL_TX_TIMEOUT 3000 /* 3 Seconds */

/*
 * Data structures to store GRE Tunnel nss debug stats
 */
static DEFINE_SPINLOCK(nss_gre_tunnel_session_debug_stats_lock);
static struct nss_stats_gre_tunnel_session_debug nss_gre_tunnel_session_debug_stats[NSS_MAX_GRE_TUNNEL_SESSIONS];

/*
 * Private data structure
 */
static struct nss_gre_tunnel_pvt {
	struct semaphore sem;
	struct completion complete;
	int response;
	void *cb;
	void *app_data;
} gre_tunnel_pvt;

/*
 * nss_gre_tunnel_verify_if_num()
 *	Verify if_num passed to us.
 */
static bool nss_gre_tunnel_verify_if_num(uint32_t if_num)
{
	if (nss_is_dynamic_interface(if_num) == false)
		return false;

	if (nss_dynamic_interface_get_type(if_num) != NSS_DYNAMIC_INTERFACE_TYPE_GRE_TUNNEL)
		return false;

	return true;
}

/*
 * nss_gre_tunnel_session_stats_sync()
 *	Sync function for statistics
 */
static void nss_gre_tunnel_session_stats_sync(struct nss_ctx_instance *nss_ctx, struct nss_gre_tunnel_stats *stats_msg,
					uint16_t if_num)
{
	int i;
	struct nss_stats_gre_tunnel_session_debug *s = NULL;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);

	spin_lock_bh(&nss_gre_tunnel_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_GRE_TUNNEL_SESSIONS; i++) {
		if (nss_gre_tunnel_session_debug_stats[i].if_num == if_num) {
			s = &nss_gre_tunnel_session_debug_stats[i];
			break;
		}
	}

	if (!s) {
		spin_unlock_bh(&nss_gre_tunnel_session_debug_stats_lock);
		nss_warning("%p: Session not found: %u", nss_ctx, if_num);
		return;
	}

	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_PKTS] += stats_msg->node_stats.rx_packets;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_TX_PKTS] += stats_msg->node_stats.tx_packets;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_DROPPED] += stats_msg->node_stats.rx_dropped;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_MALFORMED] += stats_msg->rx_malformed;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_INVALID_PROT] += stats_msg->rx_invalid_prot;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_DECAP_QUEUE_FULL] += stats_msg->decap_queue_full;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_SINGLE_REC_DGRAM] += stats_msg->rx_single_rec_dgram;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_INVALID_REC_DGRAM] += stats_msg->rx_invalid_rec_dgram;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_BUFFER_ALLOC_FAIL] += stats_msg->buffer_alloc_fail;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_BUFFER_COPY_FAIL] += stats_msg->buffer_copy_fail;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_OUTFLOW_QUEUE_FULL] += stats_msg->outflow_queue_full;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_DROPPED_HROOM] += stats_msg->rx_dropped_hroom;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_CBUFFER_ALLOC_FAIL] += stats_msg->rx_cbuf_alloc_fail;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_CENQUEUE_FAIL] += stats_msg->rx_cenqueue_fail;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_DECRYPT_DONE] += stats_msg->rx_decrypt_done;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_FORWARD_ENQUEUE_FAIL] += stats_msg->rx_forward_enqueue_fail;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_TX_CBUFFER_ALLOC_FAIL] += stats_msg->tx_cbuf_alloc_fail;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_TX_CENQUEUE_FAIL] += stats_msg->tx_cenqueue_fail;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_DROPPED_TROOM] += stats_msg->rx_dropped_troom;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_TX_FORWARD_ENQUEUE_FAIL] += stats_msg->tx_forward_enqueue_fail;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_TX_CIPHER_DONE] += stats_msg->tx_cipher_done;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_CRYPTO_NOSUPP] += stats_msg->crypto_nosupp;
	s->stats[NSS_STATS_GRE_TUNNEL_SESSION_RX_DROPPED_MH_VERSION] += stats_msg->rx_dropped_mh_ver;
	spin_unlock_bh(&nss_gre_tunnel_session_debug_stats_lock);
}

/*
 * nss_gre_tunnel_session_debug_stats_get()
 *	Get session GRE Tunnel statitics.
 */
void nss_gre_tunnel_session_debug_stats_get(struct nss_stats_gre_tunnel_session_debug *stats)
{
	int i;

	if (!stats) {
		nss_warning("No memory to copy gre_tunnel session stats");
		return;
	}

	spin_lock_bh(&nss_gre_tunnel_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_GRE_TUNNEL_SESSIONS; i++) {
		if (nss_gre_tunnel_session_debug_stats[i].valid) {
			memcpy(stats, &nss_gre_tunnel_session_debug_stats[i],
			       sizeof(struct nss_stats_gre_tunnel_session_debug));
			stats++;
		}
	}
	spin_unlock_bh(&nss_gre_tunnel_session_debug_stats_lock);
}

/*
 * nss_gre_tunnel_handler()
 *	Handle NSS to HLOS messages for gre_tunnel
 */
static void nss_gre_tunnel_handler(struct nss_ctx_instance *nss_ctx, struct nss_cmn_msg *ncm, __attribute__((unused))void *app_data)
{
	struct nss_gre_tunnel_msg *ngtm = (struct nss_gre_tunnel_msg *)ncm;
	void *ctx;

	nss_gre_tunnel_msg_callback_t cb;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	BUG_ON(!nss_gre_tunnel_verify_if_num(ncm->interface));

	/*
	 * Is this a valid request/response packet?
	 */
	if (ncm->type >= NSS_GRE_TUNNEL_MSG_MAX) {
		nss_warning("%p: received invalid message %d for GRE_TUNNEL interface %d", nss_ctx, ncm->type, ncm->interface);
		return;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_gre_tunnel_msg)) {
		nss_warning("%p: gre_tunnel message length is invalid: %d", nss_ctx, ncm->len);
		return;
	}

	/*
	 * Check messages
	 */
	switch (ngtm->cm.type) {
	case NSS_GRE_TUNNEL_MSG_STATS:
		nss_gre_tunnel_session_stats_sync(nss_ctx, &ngtm->msg.stats, ncm->interface);
		break;
	}

	/*
	 * Update the callback and app_data for NOTIFY messages
	 */
	if (ncm->response == NSS_CMM_RESPONSE_NOTIFY) {
		ncm->cb = (nss_ptr_t)nss_ctx->nss_top->gre_tunnel_msg_callback;
		ncm->app_data = (nss_ptr_t)nss_ctx->subsys_dp_register[ncm->interface].app_data;
	}

	nss_core_log_msg_failures(nss_ctx, ncm);

	/*
	 * callback
	 */
	cb = (nss_gre_tunnel_msg_callback_t)ncm->cb;
	ctx = (void *)ncm->app_data;

	/*
	 * call GRE Tunnel session callback
	 */
	if (!cb) {
		return;
	}

	cb(ctx, ngtm);
}

/*
 * nss_get_gre_tunnel_context()
 *	Return the core ctx which the feature is on
 */
struct nss_ctx_instance *nss_gre_tunnel_get_ctx(void)
{
	return (struct nss_ctx_instance *)&nss_top_main.nss[nss_top_main.gre_tunnel_handler_id];
}
EXPORT_SYMBOL(nss_gre_tunnel_get_ctx);

/*
 * nss_gre_tunnel_ifnum_with_core_id()
 *	Append core id to GRE tunnel interface num
 */
int nss_gre_tunnel_ifnum_with_core_id(int if_num)
{
	struct nss_ctx_instance *nss_ctx = nss_gre_tunnel_get_ctx();
	BUG_ON(!nss_gre_tunnel_verify_if_num(if_num));
	NSS_VERIFY_CTX_MAGIC(nss_ctx);

	if (nss_is_dynamic_interface(if_num) == false) {
		nss_info("%p: Invalid if_num: %d, must be a dynamic interface\n", nss_ctx, if_num);
		return 0;
	}

	return NSS_INTERFACE_NUM_APPEND_COREID(nss_ctx, if_num);
}
EXPORT_SYMBOL(nss_gre_tunnel_ifnum_with_core_id);

/*
 * nss_gre_tunnel_tx_buf()
 *	Transmit buffer over GRE Tunnel interface
 */
nss_tx_status_t nss_gre_tunnel_tx_buf(struct sk_buff *skb, uint32_t if_num,
				struct nss_ctx_instance *nss_ctx)
{
	int32_t status;

	BUG_ON(!nss_gre_tunnel_verify_if_num(if_num));
	NSS_VERIFY_CTX_MAGIC(nss_ctx);

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: 'GRE TUNNEL If Tx' core not ready", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	nss_info("%p: Sending to %d\n", nss_ctx, if_num);

	status = nss_core_send_buffer(nss_ctx, if_num, skb,
				      NSS_IF_DATA_QUEUE_0,
				      H2N_BUFFER_PACKET,
				      H2N_BIT_FLAG_VIRTUAL_BUFFER);
	if (unlikely(status != NSS_CORE_STATUS_SUCCESS)) {
		nss_warning("%p: Unable to enqueue 'GRE TUNNEL If Tx' packet\n", nss_ctx);
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_PACKET]);
	return NSS_TX_SUCCESS;
}
EXPORT_SYMBOL(nss_gre_tunnel_tx_buf);

/*
 * nss_gre_tunnel_tx_msg()
 *	Transmit a gre_tunnel message to NSS firmware
 */
nss_tx_status_t nss_gre_tunnel_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_gre_tunnel_msg *msg)
{
	struct nss_gre_tunnel_msg *ngtm;
	struct nss_cmn_msg *ncm = &msg->cm;
	struct sk_buff *nbuf;
	int32_t status;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: gre_tunnel msg dropped as core not ready", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	/*
	 * Sanity check message
	 */
	if (ncm->type >= NSS_GRE_TUNNEL_MSG_MAX) {
		nss_warning("%p: gre_tunnel message type out of range: %d",
			nss_ctx, ncm->type);
		return NSS_TX_FAILURE;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_gre_tunnel_msg)) {
		nss_warning("%p: GRE Tunnel message length is invalid: %d",
			    nss_ctx, ncm->len);
		return NSS_TX_FAILURE;
	}

	/*
	 * Alloc and copy message to skb
	 */
	nbuf = dev_alloc_skb(NSS_NBUF_PAYLOAD_SIZE);
	if (unlikely(!nbuf)) {
		NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_NBUF_ALLOC_FAILS]);
		nss_warning("%p: msg dropped as command allocation failed", nss_ctx);
		return NSS_TX_FAILURE;
	}

	ngtm = (struct nss_gre_tunnel_msg *)skb_put(nbuf, sizeof(struct nss_gre_tunnel_msg));
	memcpy(ngtm, msg, sizeof(struct nss_gre_tunnel_msg));

	/*
	 * Send Message
	 */
	status = nss_core_send_buffer(nss_ctx, 0, nbuf, NSS_IF_CMD_QUEUE, H2N_BUFFER_CTRL, 0);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		dev_kfree_skb_any(nbuf);
		nss_warning("%p: Unable to enqueue 'gre tunnel message'\n", nss_ctx);
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_CMD_REQ]);
	return NSS_TX_SUCCESS;
}
EXPORT_SYMBOL(nss_gre_tunnel_tx_msg);

/*
 * nss_gre_tunnel_callback()
 *	Callback to handle the completion of NSS->HLOS messages.
 */
static void nss_gre_tunnel_callback(void *app_data, struct nss_gre_tunnel_msg *ngtm)
{
	nss_gre_tunnel_msg_callback_t callback = (nss_gre_tunnel_msg_callback_t)gre_tunnel_pvt.cb;
	void *data = gre_tunnel_pvt.app_data;

	gre_tunnel_pvt.response = NSS_TX_SUCCESS;
	gre_tunnel_pvt.cb = NULL;
	gre_tunnel_pvt.app_data = NULL;

	if (ngtm->cm.response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("gre tunnel Error response %d\n", ngtm->cm.response);
		gre_tunnel_pvt.response = ngtm->cm.response;
	}

	if (callback) {
		callback(data, ngtm);
	}
	complete(&gre_tunnel_pvt.complete);
}

/*
 * nss_gre_tunnel_tx_msg()
 *	Transmit a GRE Tunnel message to NSS firmware synchronously.
 */
nss_tx_status_t nss_gre_tunnel_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_gre_tunnel_msg *ngtm)
{
	nss_tx_status_t status;
	int ret = 0;

	down(&gre_tunnel_pvt.sem);
	gre_tunnel_pvt.cb = (void *)ngtm->cm.cb;
	gre_tunnel_pvt.app_data = (void *)ngtm->cm.app_data;

	ngtm->cm.cb = (nss_ptr_t)nss_gre_tunnel_callback;
	ngtm->cm.app_data = (nss_ptr_t)NULL;

	status = nss_gre_tunnel_tx_msg(nss_ctx, ngtm);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: gre_tunnel_tx_msg failed\n", nss_ctx);
		up(&gre_tunnel_pvt.sem);
		return status;
	}

	ret = wait_for_completion_timeout(&gre_tunnel_pvt.complete, msecs_to_jiffies(NSS_GRE_TUNNEL_TX_TIMEOUT));

	if (!ret) {
		nss_warning("%p: GRE Tunnel msg tx failed due to timeout\n", nss_ctx);
		gre_tunnel_pvt.response = NSS_TX_FAILURE;
	}

	status = gre_tunnel_pvt.response;
	up(&gre_tunnel_pvt.sem);
	return status;
}
EXPORT_SYMBOL(nss_gre_tunnel_tx_msg_sync);

/*
 * nss_gre_tunnel_msg_init()
 *	Initialize gre_tunnel msg.
 */
void nss_gre_tunnel_msg_init(struct nss_gre_tunnel_msg *ngtm, uint16_t if_num,
			uint32_t type, uint32_t len, void *cb, void *app_data)
{
	nss_cmn_msg_init(&ngtm->cm, if_num, type, len, cb, app_data);
}
EXPORT_SYMBOL(nss_gre_tunnel_msg_init);

/*
 * nss_gre_tunnel_register_if()
 *	Register netdev
 */
struct nss_ctx_instance *nss_gre_tunnel_register_if(uint32_t if_num,
					      nss_gre_tunnel_data_callback_t cb,
					      nss_gre_tunnel_msg_callback_t ev_cb,
					      struct net_device *netdev,
					      uint32_t features,
					      void *app_ctx)
{
	int32_t i;

	struct nss_ctx_instance *nss_ctx = nss_gre_tunnel_get_ctx();

	BUG_ON(!nss_gre_tunnel_verify_if_num(if_num));

	spin_lock_bh(&nss_gre_tunnel_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_GRE_TUNNEL_SESSIONS; i++) {
		if (!nss_gre_tunnel_session_debug_stats[i].valid) {
			nss_gre_tunnel_session_debug_stats[i].valid = true;
			nss_gre_tunnel_session_debug_stats[i].if_num = if_num;
			nss_gre_tunnel_session_debug_stats[i].if_index = netdev->ifindex;
			break;
		}
	}
	spin_unlock_bh(&nss_gre_tunnel_session_debug_stats_lock);

	if (i == NSS_MAX_GRE_TUNNEL_SESSIONS) {
		nss_warning("%p: Cannot find free slot for GRE Tunnel session stats, I/F:%u\n", nss_ctx, if_num);
		return NULL;
	}

	if (nss_ctx->subsys_dp_register[if_num].ndev) {
		nss_warning("%p: Cannot find free slot for GRE Tunnel NSS I/F:%u\n", nss_ctx, if_num);
		nss_gre_tunnel_session_debug_stats[i].valid = false;
		nss_gre_tunnel_session_debug_stats[i].if_num = 0;
		nss_gre_tunnel_session_debug_stats[i].if_index = 0;
		return NULL;
	}

	nss_ctx->subsys_dp_register[if_num].ndev = netdev;
	nss_ctx->subsys_dp_register[if_num].cb = cb;
	nss_ctx->subsys_dp_register[if_num].app_data = app_ctx;
	nss_ctx->subsys_dp_register[if_num].features = features;
	nss_top_main.gre_tunnel_msg_callback = ev_cb;
	nss_core_register_handler(if_num, nss_gre_tunnel_handler, app_ctx);

	return nss_ctx;
}
EXPORT_SYMBOL(nss_gre_tunnel_register_if);

/*
 * nss_gre_tunnel_unregister_if()
 *	Unregister netdev
 */
void nss_gre_tunnel_unregister_if(uint32_t if_num)
{
	int32_t i;
	struct nss_ctx_instance *nss_ctx = nss_gre_tunnel_get_ctx();

	BUG_ON(!nss_gre_tunnel_verify_if_num(if_num));

	spin_lock_bh(&nss_gre_tunnel_session_debug_stats_lock);
	for (i = 0; i < NSS_MAX_GRE_TUNNEL_SESSIONS; i++) {
		if (nss_gre_tunnel_session_debug_stats[i].if_num == if_num) {
			memset(&nss_gre_tunnel_session_debug_stats[i], 0,
			       sizeof(struct nss_stats_gre_tunnel_session_debug));
			break;
		}
	}
	spin_unlock_bh(&nss_gre_tunnel_session_debug_stats_lock);

	if (i == NSS_MAX_GRE_TUNNEL_SESSIONS) {
		nss_warning("%p: Cannot find debug stats for GRE Tunnel session: %d\n", nss_ctx, if_num);
		return;
	}

	if (!nss_ctx->subsys_dp_register[if_num].ndev) {
		nss_warning("%p: Cannot find registered netdev for GRE Tunnel NSS I/F: %d\n", nss_ctx, if_num);

		return;
	}

	nss_ctx->subsys_dp_register[if_num].ndev = NULL;
	nss_ctx->subsys_dp_register[if_num].cb = NULL;
	nss_ctx->subsys_dp_register[if_num].app_data = NULL;
	nss_ctx->subsys_dp_register[if_num].features = 0;
	nss_top_main.gre_tunnel_msg_callback = NULL;
	nss_core_unregister_handler(if_num);
}
EXPORT_SYMBOL(nss_gre_tunnel_unregister_if);
