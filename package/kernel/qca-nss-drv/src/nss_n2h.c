/*
 **************************************************************************
 * Copyright (c) 2013-2017, The Linux Foundation. All rights reserved.
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
 * nss_n2h.c
 *	NSS N2H node APIs
 */

#include "nss_tx_rx_common.h"

#define NSS_N2H_MAX_BUF_POOL_SIZE (1024 * 1024 * 8) /* 8MB */
#define NSS_N2H_MIN_EMPTY_POOL_BUF_SZ		32
#define NSS_N2H_MAX_EMPTY_POOL_BUF_SZ		65536
#define NSS_N2H_DEFAULT_EMPTY_POOL_BUF_SZ	8192

int nss_n2h_empty_pool_buf_cfg[NSS_MAX_CORES] __read_mostly = {-1, -1};
int nss_n2h_water_mark[NSS_MAX_CORES][2] __read_mostly = {{-1, -1}, {-1, -1} };
int nss_n2h_wifi_pool_buf_cfg __read_mostly = -1;
int nss_n2h_rps_config __read_mostly;
int nss_n2h_core0_mitigation_cfg __read_mostly = 1;
int nss_n2h_core1_mitigation_cfg __read_mostly = 1;
int nss_n2h_core0_add_buf_pool_size __read_mostly;
int nss_n2h_core1_add_buf_pool_size __read_mostly;

struct nss_n2h_registered_data {
	nss_n2h_msg_callback_t n2h_callback;
	void *app_data;
};

static struct nss_n2h_cfg_pvt nss_n2h_nepbcfgp[NSS_MAX_CORES];
static struct nss_n2h_registered_data nss_n2h_rd[NSS_MAX_CORES];
static struct nss_n2h_cfg_pvt nss_n2h_rcp;
static struct nss_n2h_cfg_pvt nss_n2h_mitigationcp[NSS_CORE_MAX];
static struct nss_n2h_cfg_pvt nss_n2h_bufcp[NSS_CORE_MAX];
static struct nss_n2h_cfg_pvt nss_n2h_wp;

/*
 * nss_n2h_stats_sync()
 *	Handle the syncing of NSS statistics.
 */
static void nss_n2h_stats_sync(struct nss_ctx_instance *nss_ctx, struct nss_n2h_stats_sync *nnss)
{
	struct nss_top_instance *nss_top = nss_ctx->nss_top;

	spin_lock_bh(&nss_top->stats_lock);

	/*
	 * common node stats
	 */
	nss_ctx->stats_n2h[NSS_STATS_NODE_RX_PKTS] += nnss->node_stats.rx_packets;
	nss_ctx->stats_n2h[NSS_STATS_NODE_RX_BYTES] += nnss->node_stats.rx_bytes;
	nss_ctx->stats_n2h[NSS_STATS_NODE_RX_DROPPED] += nnss->node_stats.rx_dropped;
	nss_ctx->stats_n2h[NSS_STATS_NODE_TX_PKTS] += nnss->node_stats.tx_packets;
	nss_ctx->stats_n2h[NSS_STATS_NODE_TX_BYTES] += nnss->node_stats.tx_bytes;

	/*
	 * General N2H stats
	 */
	nss_ctx->stats_n2h[NSS_STATS_N2H_QUEUE_DROPPED] += nnss->queue_dropped;
	nss_ctx->stats_n2h[NSS_STATS_N2H_TOTAL_TICKS] += nnss->total_ticks;
	nss_ctx->stats_n2h[NSS_STATS_N2H_WORST_CASE_TICKS] += nnss->worst_case_ticks;
	nss_ctx->stats_n2h[NSS_STATS_N2H_ITERATIONS] += nnss->iterations;

	/*
	 * pbuf manager ocm and default pool stats
	 */
	nss_ctx->stats_n2h[NSS_STATS_N2H_PBUF_OCM_ALLOC_FAILS] += nnss->pbuf_ocm_stats.pbuf_alloc_fails;
	nss_ctx->stats_n2h[NSS_STATS_N2H_PBUF_OCM_FREE_COUNT] = nnss->pbuf_ocm_stats.pbuf_free_count;
	nss_ctx->stats_n2h[NSS_STATS_N2H_PBUF_OCM_TOTAL_COUNT] = nnss->pbuf_ocm_stats.pbuf_total_count;

	nss_ctx->stats_n2h[NSS_STATS_N2H_PBUF_DEFAULT_ALLOC_FAILS] += nnss->pbuf_default_stats.pbuf_alloc_fails;
	nss_ctx->stats_n2h[NSS_STATS_N2H_PBUF_DEFAULT_FREE_COUNT] = nnss->pbuf_default_stats.pbuf_free_count;
	nss_ctx->stats_n2h[NSS_STATS_N2H_PBUF_DEFAULT_TOTAL_COUNT] = nnss->pbuf_default_stats.pbuf_total_count;

	/*
	 * payload mgr stats
	 */
	nss_ctx->stats_n2h[NSS_STATS_N2H_PAYLOAD_ALLOC_FAILS] += nnss->payload_alloc_fails;
	nss_ctx->stats_n2h[NSS_STATS_N2H_PAYLOAD_FREE_COUNT] = nnss->payload_free_count;

	/*
	 * Host <=> NSS control traffic stats
	 */
	nss_ctx->stats_n2h[NSS_STATS_N2H_H2N_CONTROL_PACKETS] += nnss->h2n_ctrl_pkts;
	nss_ctx->stats_n2h[NSS_STATS_N2H_H2N_CONTROL_BYTES] += nnss->h2n_ctrl_bytes;
	nss_ctx->stats_n2h[NSS_STATS_N2H_N2H_CONTROL_PACKETS] += nnss->n2h_ctrl_pkts;
	nss_ctx->stats_n2h[NSS_STATS_N2H_N2H_CONTROL_BYTES] += nnss->n2h_ctrl_bytes;

	/*
	 * Host <=> NSS control data traffic stats
	 */
	nss_ctx->stats_n2h[NSS_STATS_N2H_H2N_DATA_PACKETS] += nnss->h2n_data_pkts;
	nss_ctx->stats_n2h[NSS_STATS_N2H_H2N_DATA_BYTES] += nnss->h2n_data_bytes;
	nss_ctx->stats_n2h[NSS_STATS_N2H_N2H_DATA_PACKETS] += nnss->n2h_data_pkts;
	nss_ctx->stats_n2h[NSS_STATS_N2H_N2H_DATA_BYTES] += nnss->n2h_data_bytes;

	/*
	 * Payloads related stats
	 */
	nss_ctx->stats_n2h[NSS_STATS_N2H_N2H_TOT_PAYLOADS] = nnss->tot_payloads;

	nss_ctx->stats_n2h[NSS_STATS_N2H_N2H_INTERFACE_INVALID] += nnss->data_interface_invalid;

	spin_unlock_bh(&nss_top->stats_lock);
}

/*
 * nss_n2h_interface_handler()
 *	Handle NSS -> HLOS messages for N2H node
 */
static void nss_n2h_interface_handler(struct nss_ctx_instance *nss_ctx,
					struct nss_cmn_msg *ncm,
					void *app_data)
{
	struct nss_n2h_msg *nnm = (struct nss_n2h_msg *)ncm;
	nss_n2h_msg_callback_t cb;

	BUG_ON(ncm->interface != NSS_N2H_INTERFACE);

	/*
	 * Is this a valid request/response packet?
	 */
	if (nnm->cm.type >= NSS_METADATA_TYPE_N2H_MAX) {
		nss_warning("%p: received invalid message %d for Offload stats interface", nss_ctx, nnm->cm.type);
		return;
	}

	switch (nnm->cm.type) {
	case NSS_TX_METADATA_TYPE_N2H_RPS_CFG:
		nss_info("NSS N2H rps_en %d \n",nnm->msg.rps_cfg.enable);
		break;

	case NSS_TX_METADATA_TYPE_N2H_MITIGATION_CFG:
		nss_info("NSS N2H mitigation_dis %d \n",nnm->msg.mitigation_cfg.enable);
		break;

	case NSS_TX_METADATA_TYPE_N2H_EMPTY_POOL_BUF_CFG:
		nss_info("%p: empty pool buf cfg response from FW", nss_ctx);
		break;

	case NSS_TX_METADATA_TYPE_N2H_FLUSH_PAYLOADS:
		nss_info("%p: flush payloads cmd response from FW", nss_ctx);
		break;

	case NSS_RX_METADATA_TYPE_N2H_STATS_SYNC:
		nss_n2h_stats_sync(nss_ctx, &nnm->msg.stats_sync);
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
	 * Update the callback and app_data for NOTIFY messages, n2h sends all notify messages
	 * to the same callback/app_data.
	 */
	if (nnm->cm.response == NSS_CMM_RESPONSE_NOTIFY) {
		/*
		 * Place holder for the user to create right call
		 * back and app data when response is NSS_CMM_RESPONSE_NOTIFY
		 */
		ncm->cb = (nss_ptr_t)nss_n2h_rd[nss_ctx->id].n2h_callback;
		ncm->app_data = (nss_ptr_t)nss_n2h_rd[nss_ctx->id].app_data;
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
	cb = (nss_n2h_msg_callback_t)ncm->cb;
	cb((void *)ncm->app_data, nnm);
}

/*
 * nss_n2h_rps_cfg_callback()
 *	call back function for rps configuration
 */
static void nss_n2h_rps_cfg_callback(void *app_data, struct nss_n2h_msg *nnm)
{
	struct nss_ctx_instance *nss_ctx =  (struct nss_ctx_instance *)app_data;
	if (nnm->cm.response != NSS_CMN_RESPONSE_ACK) {

		/*
		 * Error, hence we are not updating the nss_n2h_empty_pool_buf
		 * Restore the current_value to its previous state
		 */
		nss_n2h_rcp.response = NSS_FAILURE;
		complete(&nss_n2h_rcp.complete);
		nss_warning("%p: RPS configuration failed : %d\n", nss_ctx,
								   nnm->cm.error);
		return;
	}

	nss_info("%p: RPS configuration succeeded: %d\n", nss_ctx,
							   nnm->cm.error);
	nss_ctx->n2h_rps_en = nnm->msg.rps_cfg.enable;
	nss_n2h_rcp.response = NSS_SUCCESS;
	complete(&nss_n2h_rcp.complete);
}

/*
 * nss_n2h_mitigation_cfg_callback()
 *	call back function for mitigation configuration
 */
static void nss_n2h_mitigation_cfg_callback(void *app_data, struct nss_n2h_msg *nnm)
{
	uint32_t core_num = (uint32_t)(nss_ptr_t)app_data;
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[core_num];

	if (nnm->cm.response != NSS_CMN_RESPONSE_ACK) {

		/*
		 * Error, hence we are not updating the nss_n2h_mitigate_en
		 */
		nss_n2h_mitigationcp[core_num].response = NSS_FAILURE;
		complete(&nss_n2h_mitigationcp[core_num].complete);
		nss_warning("core%d: MITIGATION configuration failed : %d\n", core_num, nnm->cm.error);
		return;
	}

	nss_info("core%d: MITIGATION configuration succeeded: %d\n", core_num, nnm->cm.error);

	nss_ctx->n2h_mitigate_en = nnm->msg.mitigation_cfg.enable;
	nss_n2h_mitigationcp[core_num].response = NSS_SUCCESS;
	complete(&nss_n2h_mitigationcp[core_num].complete);
}

/*
 * nss_n2h_buf_cfg_callback()
 *	call back function for pbuf configuration
 */
static void nss_n2h_bufs_cfg_callback(void *app_data, struct nss_n2h_msg *nnm)
{
	uint32_t core_num = (uint32_t)(nss_ptr_t)app_data;
	unsigned int allocated_sz;

	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[core_num];

	if (nnm->cm.response != NSS_CMN_RESPONSE_ACK) {
		nss_n2h_bufcp[core_num].response = NSS_FAILURE;
		nss_warning("core%d: buf configuration failed : %d\n", core_num, nnm->cm.error);
		goto done;
	}

	nss_info("core%d: buf configuration succeeded: %d\n", core_num, nnm->cm.error);

	allocated_sz = nnm->msg.buf_pool.nss_buf_page_size * nnm->msg.buf_pool.nss_buf_num_pages;
	nss_ctx->buf_sz_allocated += allocated_sz;

	nss_n2h_bufcp[core_num].response = NSS_SUCCESS;

done:
	complete(&nss_n2h_bufcp[core_num].complete);
}

/*
 * nss_n2h_payload_stats_callback()
 *	It gets called response to payload accounting.
 */
static void nss_n2h_payload_stats_callback(void *app_data,
					struct nss_n2h_msg *nnm)
{
	uint32_t core_num = (uint32_t)(nss_ptr_t)app_data;

	if (nnm->cm.response != NSS_CMN_RESPONSE_ACK) {
		struct nss_n2h_empty_pool_buf *nnepbcm;
		nnepbcm = &nnm->msg.empty_pool_buf_cfg;

		nss_warning("%d: core empty pool buf set failure: %d\n",
				core_num, nnm->cm.error);
		nss_n2h_nepbcfgp[core_num].response = NSS_FAILURE;
		complete(&nss_n2h_nepbcfgp[core_num].complete);
		return;
	}

	if (nnm->cm.type == NSS_TX_METADATA_TYPE_GET_PAYLOAD_INFO) {
		nss_n2h_nepbcfgp[core_num].empty_buf_pool =
			ntohl(nnm->msg.payload_info.pool_size);
		nss_n2h_nepbcfgp[core_num].low_water =
			ntohl(nnm->msg.payload_info.low_water);
		nss_n2h_nepbcfgp[core_num].high_water =
			ntohl(nnm->msg.payload_info.high_water);
	}

	nss_n2h_nepbcfgp[core_num].response = NSS_SUCCESS;
	complete(&nss_n2h_nepbcfgp[core_num].complete);
}

/*
 * nss_n2h_set_wifi_payloads_callback()
 * 	call back function for response to wifi pool configuration
 *
 */
static void nss_n2h_set_wifi_payloads_callback(void *app_data,
					struct nss_n2h_msg *nnm)
{
	struct nss_ctx_instance *nss_ctx __maybe_unused = (struct nss_ctx_instance *)app_data;
	if (nnm->cm.response != NSS_CMN_RESPONSE_ACK) {

		nss_n2h_wp.response = NSS_FAILURE;
		complete(&nss_n2h_wp.complete);
		nss_warning("%p: wifi pool configuration failed : %d\n", nss_ctx,
				nnm->cm.error);
		return;
	}

	nss_info("%p: wifi payload configuration succeeded: %d\n", nss_ctx,
			nnm->cm.error);
	nss_n2h_wp.response = NSS_SUCCESS;
	complete(&nss_n2h_wp.complete);
}

/*
 * nss_n2h_get_payload_info()
 *	Gets Payload information
 */
static int nss_n2h_get_payload_info(struct ctl_table *ctl, int write,
			void __user *buffer, size_t *lenp, loff_t *ppos,
			nss_ptr_t core_num)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[core_num];
	struct nss_n2h_msg nnm;
	struct nss_n2h_payload_info *nnepbcm;
	nss_tx_status_t nss_tx_status;
	int ret = NSS_FAILURE;

	/*
	 * Note that semaphore should be already held.
	 */

	nss_n2h_msg_init(&nnm, NSS_N2H_INTERFACE,
			NSS_TX_METADATA_TYPE_GET_PAYLOAD_INFO,
			sizeof(struct nss_n2h_payload_info),
			nss_n2h_payload_stats_callback,
			(void *)core_num);

	nnepbcm = &nnm.msg.payload_info;
	nss_tx_status = nss_n2h_tx_msg(nss_ctx, &nnm);

	if (nss_tx_status != NSS_TX_SUCCESS) {
		nss_warning("%p: core %d nss_tx error errorn", nss_ctx, (int)core_num);
		return NSS_FAILURE;
	}

	/*
	 * Blocking call, wait till we get ACK for this msg.
	 */
	ret = wait_for_completion_timeout(&nss_n2h_nepbcfgp[core_num].complete,
			msecs_to_jiffies(NSS_CONN_CFG_TIMEOUT));
	if (ret == 0) {
		nss_warning("%p: core %d waiting for ack timed out\n", nss_ctx, (int)core_num);
		return NSS_FAILURE;
	}

	if (NSS_FAILURE == nss_n2h_nepbcfgp[core_num].response) {
		nss_warning("%p: core %d response returned failure\n", nss_ctx, (int)core_num);
		return NSS_FAILURE;
	}

	return NSS_SUCCESS;
}

/*
 * nss_n2h_set_empty_pool_buf()
 *	Sets empty pool buffer
 */
static int nss_n2h_set_empty_pool_buf(struct ctl_table *ctl, int write,
				void __user *buffer,
				size_t *lenp, loff_t *ppos,
				nss_ptr_t core_num, int *new_val)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[core_num];
	struct nss_n2h_msg nnm;
	struct nss_n2h_empty_pool_buf *nnepbcm;
	nss_tx_status_t nss_tx_status;
	int ret = NSS_FAILURE;

	/*
	 * Acquiring semaphore
	 */
	down(&nss_n2h_nepbcfgp[core_num].sem);

	/*
	 * Take snap shot of current value
	 */
	nss_n2h_nepbcfgp[core_num].empty_buf_pool = *new_val;

	if (!write) {
		ret = nss_n2h_get_payload_info(ctl, write, buffer, lenp, ppos,
				core_num);
		*new_val = nss_n2h_nepbcfgp[core_num].empty_buf_pool;
		if (ret == NSS_FAILURE) {
			up(&nss_n2h_nepbcfgp[core_num].sem);
			return -EBUSY;
		}

		up(&nss_n2h_nepbcfgp[core_num].sem);

		ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
		return ret;
	}

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (ret) {
		up(&nss_n2h_nepbcfgp[core_num].sem);
		return ret;
	}

	if ((*new_val < NSS_N2H_MIN_EMPTY_POOL_BUF_SZ)) {
		nss_warning("%p: core %d setting %d < min number of buffer",
				nss_ctx, (int)core_num, *new_val);
		goto failure;
	}

	nss_info("%p: core %d number of empty pool buffer is : %d\n",
		nss_ctx, (int)core_num, *new_val);

	nss_n2h_msg_init(&nnm, NSS_N2H_INTERFACE,
			NSS_TX_METADATA_TYPE_N2H_EMPTY_POOL_BUF_CFG,
			sizeof(struct nss_n2h_empty_pool_buf),
			nss_n2h_payload_stats_callback,
			(nss_ptr_t *)core_num);

	nnepbcm = &nnm.msg.empty_pool_buf_cfg;
	nnepbcm->pool_size = htonl(*new_val);
	nss_tx_status = nss_n2h_tx_msg(nss_ctx, &nnm);

	if (nss_tx_status != NSS_TX_SUCCESS) {
		nss_warning("%p: core %d nss_tx error empty pool buffer: %d\n",
				nss_ctx, (int)core_num, *new_val);
		goto failure;
	}

	/*
	 * Blocking call, wait till we get ACK for this msg.
	 */
	ret = wait_for_completion_timeout(&nss_n2h_nepbcfgp[core_num].complete,
			msecs_to_jiffies(NSS_CONN_CFG_TIMEOUT));
	if (ret == 0) {
		nss_warning("%p: core %d Waiting for ack timed out\n", nss_ctx, (int)core_num);
		goto failure;
	}

	/*
	 * ACK/NACK received from NSS FW
	 * If ACK: Callback function will update nss_n2h_empty_pool_buf with
	 * nss_n2h_nepbcfgp.num_conn_valid, which holds the user input
	 */
	if (NSS_FAILURE == nss_n2h_nepbcfgp[core_num].response) {
		goto failure;
	}

	up(&nss_n2h_nepbcfgp[core_num].sem);
	return 0;

failure:
	/*
	 * Restore the current_value to its previous state
	 */
	*new_val = nss_n2h_nepbcfgp[core_num].empty_buf_pool;
	up(&nss_n2h_nepbcfgp[core_num].sem);
	return NSS_FAILURE;
}

/*
 * nss_n2h_set_water_mark()
 *	Sets water mark for N2H SOS
 */
static int nss_n2h_set_water_mark(struct ctl_table *ctl, int write,
					void __user *buffer,
					size_t *lenp, loff_t *ppos,
					uint32_t core_num, int *low, int *high)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[core_num];
	struct nss_n2h_msg nnm;
	struct nss_n2h_water_mark *wm;
	nss_tx_status_t nss_tx_status;
	int ret = NSS_FAILURE;

	/*
	 * Acquiring semaphore
	 */
	down(&nss_n2h_nepbcfgp[core_num].sem);

	/*
	 * Take snap shot of current value
	 */
	nss_n2h_nepbcfgp[core_num].low_water = *low;
	nss_n2h_nepbcfgp[core_num].high_water = *high;

	if (!write) {
		ret = nss_n2h_get_payload_info(ctl, write, buffer, lenp, ppos,
				core_num);
		*low = nss_n2h_nepbcfgp[core_num].low_water;
		*high = nss_n2h_nepbcfgp[core_num].high_water;

		if (ret == NSS_FAILURE) {
			up(&nss_n2h_nepbcfgp[core_num].sem);
			return -EBUSY;
		}

		up(&nss_n2h_nepbcfgp[core_num].sem);
		ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
		return ret;
	}

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (ret) {
		up(&nss_n2h_nepbcfgp[core_num].sem);
		return ret;
	}

	/*
	 * If either low or high water mark is not set then we do
	 * nothing.
	 */
	if (*low == -1 || *high == -1)
		goto failure;

	if ((*low < NSS_N2H_MIN_EMPTY_POOL_BUF_SZ) ||
		(*high < NSS_N2H_MIN_EMPTY_POOL_BUF_SZ)) {
		nss_warning("%p: core %d setting %d, %d < min number of buffer",
				nss_ctx, core_num, *low, *high);
		goto failure;
	}

	if ((*low > NSS_N2H_MAX_EMPTY_POOL_BUF_SZ) ||
		(*high > NSS_N2H_MAX_EMPTY_POOL_BUF_SZ)) {
		nss_warning("%p: core %d setting %d, %d is > upper limit",
				nss_ctx, core_num, *low, *high);
		goto failure;
	}

	if (*low > *high) {
		nss_warning("%p: core %d setting low %d is more than high %d",
				nss_ctx, core_num, *low, *high);
		goto failure;
	}

	nss_info("%p: core %d number of low : %d and high : %d\n",
		nss_ctx, core_num, *low, *high);

	nss_n2h_msg_init(&nnm, NSS_N2H_INTERFACE,
			NSS_TX_METADATA_TYPE_SET_WATER_MARK,
			sizeof(struct nss_n2h_water_mark),
			nss_n2h_payload_stats_callback,
			(void *)(nss_ptr_t)core_num);

	wm = &nnm.msg.wm;
	wm->low_water = htonl(*low);
	wm->high_water = htonl(*high);
	nss_tx_status = nss_n2h_tx_msg(nss_ctx, &nnm);

	if (nss_tx_status != NSS_TX_SUCCESS) {
		nss_warning("%p: core %d nss_tx error setting : %d, %d\n",
				nss_ctx, core_num, *low, *high);
		goto failure;
	}

	/*
	 * Blocking call, wait till we get ACK for this msg.
	 */
	ret = wait_for_completion_timeout(&nss_n2h_nepbcfgp[core_num].complete,
			msecs_to_jiffies(NSS_CONN_CFG_TIMEOUT));
	if (ret == 0) {
		nss_warning("%p: core %d Waiting for ack timed out\n", nss_ctx,
			core_num);
		goto failure;
	}

	/*
	 * ACK/NACK received from NSS FW
	 */
	if (NSS_FAILURE == nss_n2h_nepbcfgp[core_num].response)
		goto failure;

	up(&nss_n2h_nepbcfgp[core_num].sem);
	return NSS_SUCCESS;

failure:
	/*
	 * Restore the current_value to its previous state
	 */
	*low = nss_n2h_nepbcfgp[core_num].low_water;
	*high = nss_n2h_nepbcfgp[core_num].high_water;
	up(&nss_n2h_nepbcfgp[core_num].sem);
	return -EINVAL;
}

/*
 * nss_n2h_cfg_wifi_pool()
 *	Sets number of wifi payloads to adjust high water mark for N2H SoS
 */
static int nss_n2h_cfg_wifi_pool(struct ctl_table *ctl, int write,
					void __user *buffer,
					size_t *lenp, loff_t *ppos,
					int *payloads)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[0];
	struct nss_n2h_msg nnm;
	struct nss_n2h_wifi_payloads *wp;
	nss_tx_status_t nss_tx_status;
	int ret = NSS_FAILURE;

	/*
	 * Acquiring semaphore
	 */
	down(&nss_n2h_wp.sem);

	if (!write) {
		*payloads = nss_n2h_wp.wifi_pool;

		up(&nss_n2h_wp.sem);
		ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
		return ret;
	}

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (ret) {
		up(&nss_n2h_wp.sem);
		return ret;
	}

	/*
	 * If payloads parameter is not set, we do
	 * nothing.
	 */
	if (*payloads == -1)
		goto failure;

	if ((*payloads < NSS_N2H_MIN_EMPTY_POOL_BUF_SZ)) {
		nss_warning("%p: wifi setting %d < min number of buffer",
				nss_ctx, *payloads);
		goto failure;
	}

	if ((*payloads > NSS_N2H_MAX_EMPTY_POOL_BUF_SZ)) {
		nss_warning("%p: wifi setting %d > max number of buffer",
				nss_ctx, *payloads);
		goto failure;
	}

	nss_info("%p: wifi payloads : %d\n",
		nss_ctx, *payloads);

	nss_n2h_msg_init(&nnm, NSS_N2H_INTERFACE,
			NSS_TX_METADATA_TYPE_N2H_WIFI_POOL_BUF_CFG,
			sizeof(struct nss_n2h_wifi_payloads),
			nss_n2h_set_wifi_payloads_callback,
			(void *)nss_ctx);

	wp = &nnm.msg.wp;
	wp->payloads = htonl(*payloads);
	nss_tx_status = nss_n2h_tx_msg(nss_ctx, &nnm);

	if (nss_tx_status != NSS_TX_SUCCESS) {
		nss_warning("%p: wifi setting %d nss_tx error",
				nss_ctx, *payloads);
		goto failure;
	}

	/*
	 * Blocking call, wait till we get ACK for this msg.
	 */
	ret = wait_for_completion_timeout(&nss_n2h_wp.complete,
			msecs_to_jiffies(NSS_CONN_CFG_TIMEOUT));
	if (ret == 0) {
		nss_warning("%p: Waiting for ack timed out\n", nss_ctx);
		goto failure;
	}

	/*
	 * ACK/NACK received from NSS FW
	 */
	if (NSS_FAILURE == nss_n2h_wp.response)
		goto failure;

	up(&nss_n2h_wp.sem);
	return NSS_SUCCESS;

failure:
	up(&nss_n2h_wp.sem);
	return -EINVAL;
}

/*
 * nss_n2h_empty_pool_buf_core1_handler()
 *	Sets the number of empty buffer for core 1
 */
static int nss_n2h_empty_pool_buf_cfg_core1_handler(struct ctl_table *ctl,
				int write, void __user *buffer,
				size_t *lenp, loff_t *ppos)
{
	return nss_n2h_set_empty_pool_buf(ctl, write, buffer, lenp, ppos,
			NSS_CORE_1, &nss_n2h_empty_pool_buf_cfg[NSS_CORE_1]);
}

/*
 * nss_n2h_empty_pool_buf_core0_handler()
 *	Sets the number of empty buffer for core 0
 */
static int nss_n2h_empty_pool_buf_cfg_core0_handler(struct ctl_table *ctl,
				int write, void __user *buffer,
				size_t *lenp, loff_t *ppos)
{
	return nss_n2h_set_empty_pool_buf(ctl, write, buffer, lenp, ppos,
			NSS_CORE_0, &nss_n2h_empty_pool_buf_cfg[NSS_CORE_0]);
}

/*
 * nss_n2h_water_mark_core1_handler()
 *	Sets water mark for core 1
 */
static int nss_n2h_water_mark_core1_handler(struct ctl_table *ctl,
			int write, void __user *buffer,
			size_t *lenp, loff_t *ppos)
{
	return nss_n2h_set_water_mark(ctl, write, buffer, lenp, ppos,
			NSS_CORE_1, &nss_n2h_water_mark[NSS_CORE_1][0],
			&nss_n2h_water_mark[NSS_CORE_1][1]);
}

/*
 * nss_n2h_water_mark_core0_handler()
 *	Sets water mark for core 0
 */
static int nss_n2h_water_mark_core0_handler(struct ctl_table *ctl,
			int write, void __user *buffer,
			size_t *lenp, loff_t *ppos)
{
	return nss_n2h_set_water_mark(ctl, write, buffer, lenp, ppos,
			NSS_CORE_0, &nss_n2h_water_mark[NSS_CORE_0][0],
			&nss_n2h_water_mark[NSS_CORE_0][1]);
}

/*
 * nss_n2h_wifi_payloads_handler()
 *	Sets number of wifi payloads
 */
static int nss_n2h_wifi_payloads_handler(struct ctl_table *ctl,
			int write, void __user *buffer,
			size_t *lenp, loff_t *ppos)
{
	return nss_n2h_cfg_wifi_pool(ctl, write, buffer, lenp, ppos,
			&nss_n2h_wifi_pool_buf_cfg);
}

/*
 * nss_n2h_rps_cfg()
 *	Send Message to NSS to enable RPS.
 */
static nss_tx_status_t nss_n2h_rps_cfg(struct nss_ctx_instance *nss_ctx, int enable_rps)
{
	struct nss_n2h_msg nnm;
	struct nss_n2h_rps *rps_cfg;
	nss_tx_status_t nss_tx_status;
	int ret;

	down(&nss_n2h_rcp.sem);
	nss_n2h_msg_init(&nnm, NSS_N2H_INTERFACE, NSS_TX_METADATA_TYPE_N2H_RPS_CFG,
			sizeof(struct nss_n2h_rps),
			nss_n2h_rps_cfg_callback,
			(void *)nss_ctx);

	rps_cfg = &nnm.msg.rps_cfg;
	rps_cfg->enable = enable_rps;

	nss_tx_status = nss_n2h_tx_msg(nss_ctx, &nnm);

	if (nss_tx_status != NSS_TX_SUCCESS) {
		nss_warning("%p: nss_tx error setting rps\n", nss_ctx);

		up(&nss_n2h_rcp.sem);
		return NSS_FAILURE;
	}

	/*
	 * Blocking call, wait till we get ACK for this msg.
	 */
	ret = wait_for_completion_timeout(&nss_n2h_rcp.complete, msecs_to_jiffies(NSS_CONN_CFG_TIMEOUT));
	if (ret == 0) {
		nss_warning("%p: Waiting for ack timed out\n", nss_ctx);
		up(&nss_n2h_rcp.sem);
		return NSS_FAILURE;
	}

	/*
	 * ACK/NACK received from NSS FW
	 * If ACK: Callback function will update nss_n2h_empty_pool_buf with
	 * nss_n2h_nepbcfgp.num_conn_valid, which holds the user input
	 */
	if (NSS_FAILURE == nss_n2h_rcp.response) {
		up(&nss_n2h_rcp.sem);
		return NSS_FAILURE;
	}

	up(&nss_n2h_rcp.sem);
	return NSS_SUCCESS;
}

/*
 * nss_n2h_mitigation_cfg()
 *	Send Message to NSS to disable MITIGATION.
 */
static nss_tx_status_t nss_n2h_mitigation_cfg(struct nss_ctx_instance *nss_ctx, int enable_mitigation, nss_core_id_t core_num)
{
	struct nss_n2h_msg nnm;
	struct nss_n2h_mitigation *mitigation_cfg;
	nss_tx_status_t nss_tx_status;
	int ret;

	nss_assert(core_num < NSS_CORE_MAX);

	down(&nss_n2h_mitigationcp[core_num].sem);
	nss_n2h_msg_init(&nnm, NSS_N2H_INTERFACE, NSS_TX_METADATA_TYPE_N2H_MITIGATION_CFG,
			sizeof(struct nss_n2h_mitigation),
			nss_n2h_mitigation_cfg_callback,
			(void *)core_num);

	mitigation_cfg = &nnm.msg.mitigation_cfg;
	mitigation_cfg->enable = enable_mitigation;

	nss_tx_status = nss_n2h_tx_msg(nss_ctx, &nnm);

	if (nss_tx_status != NSS_TX_SUCCESS) {
		nss_warning("%p: nss_tx error setting mitigation\n", nss_ctx);
		goto failure;
	}

	/*
	 * Blocking call, wait till we get ACK for this msg.
	 */
	ret = wait_for_completion_timeout(&nss_n2h_mitigationcp[core_num].complete, msecs_to_jiffies(NSS_CONN_CFG_TIMEOUT));
	if (ret == 0) {
		nss_warning("%p: Waiting for ack timed out\n", nss_ctx);
		goto failure;
	}

	/*
	 * ACK/NACK received from NSS FW
	 */
	if (NSS_FAILURE == nss_n2h_mitigationcp[core_num].response) {
		goto failure;
	}

	up(&nss_n2h_mitigationcp[core_num].sem);
	return NSS_SUCCESS;

failure:
	up(&nss_n2h_mitigationcp[core_num].sem);
	return NSS_FAILURE;
}

static inline void nss_n2h_buf_pool_free(struct nss_n2h_buf_pool *buf_pool)
{
	int page_count;
	for (page_count = 0; page_count < buf_pool->nss_buf_num_pages; page_count++) {
		kfree(buf_pool->nss_buf_pool_vaddr[page_count]);
	}
}

/*
 * nss_n2h_buf_cfg()
 *	Send Message to NSS to enable pbufs.
 */
static nss_tx_status_t nss_n2h_buf_pool_cfg(struct nss_ctx_instance *nss_ctx,
					int buf_pool_size, nss_core_id_t core_num)
{
	static struct nss_n2h_msg nnm;
	struct nss_n2h_buf_pool *buf_pool;
	nss_tx_status_t nss_tx_status;
	int ret;
	int page_count;
	int num_pages = ALIGN(buf_pool_size, PAGE_SIZE)/PAGE_SIZE;

	nss_assert(core_num < NSS_CORE_MAX);

	nss_n2h_msg_init(&nnm, NSS_N2H_INTERFACE, NSS_METADATA_TYPE_N2H_ADD_BUF_POOL,
			sizeof(struct nss_n2h_buf_pool),
			nss_n2h_bufs_cfg_callback,
			(void *)core_num);

	do {

		down(&nss_n2h_bufcp[core_num].sem);

		buf_pool = &nnm.msg.buf_pool;
		buf_pool->nss_buf_page_size = PAGE_SIZE;

		for (page_count = 0; page_count < MAX_PAGES_PER_MSG && num_pages; page_count++, num_pages--) {
			void *kern_addr = kzalloc(PAGE_SIZE, GFP_ATOMIC);
			if (!kern_addr) {
				BUG_ON(!page_count);
				break;
			}

			kmemleak_not_leak(kern_addr);
			buf_pool->nss_buf_pool_vaddr[page_count] = kern_addr;
			buf_pool->nss_buf_pool_addr[page_count] = dma_map_single(nss_ctx->dev, kern_addr, PAGE_SIZE, DMA_TO_DEVICE);
		}

		buf_pool->nss_buf_num_pages = page_count;
		nss_tx_status = nss_n2h_tx_msg(nss_ctx, &nnm);
		if (nss_tx_status != NSS_TX_SUCCESS) {

			nss_n2h_buf_pool_free(buf_pool);
			nss_warning("%p: nss_tx error setting pbuf\n", nss_ctx);
			goto failure;
		}

		/*
	 	 * Blocking call, wait till we get ACK for this msg.
	 	 */
		ret = wait_for_completion_timeout(&nss_n2h_bufcp[core_num].complete, msecs_to_jiffies(NSS_CONN_CFG_TIMEOUT));
		if (ret == 0) {
			nss_warning("%p: Waiting for ack timed out\n", nss_ctx);
			goto failure;
		}

		/*
		 * ACK/NACK received from NSS FW
		 */
		if (NSS_FAILURE == nss_n2h_bufcp[core_num].response) {

			nss_n2h_buf_pool_free(buf_pool);
			goto failure;
		}

		up(&nss_n2h_bufcp[core_num].sem);
	} while(num_pages);

	return NSS_SUCCESS;
failure:
	up(&nss_n2h_bufcp[core_num].sem);
	return NSS_FAILURE;
}

/*
 * nss_rps_handler()
 *	Enable NSS RPS
 */
static int nss_n2h_rpscfg_handler(struct ctl_table *ctl, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[0];
	int ret;

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (!ret) {
		if ((write) && (nss_n2h_rps_config == 1)) {
			printk(KERN_INFO "Enabling NSS RPS\n");

			return nss_n2h_rps_cfg(nss_ctx, 1);
		}

		if ((write) && (nss_n2h_rps_config == 0)) {
			printk(KERN_INFO "Runtime disabling of NSS RPS not supported\n");
			return ret;
		}

		if (write) {
			printk(KERN_INFO "Invalid input value.Valid values are 0 and 1\n");
		}

	}

	return ret;
}

/*
 * nss_mitigation_handler()
 * Enable NSS MITIGATION
 */
static int nss_n2h_mitigationcfg_core0_handler(struct ctl_table *ctl, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[NSS_CORE_0];
	int ret;

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (ret) {
		return ret;
	}

	/*
	 * It's a read operation
	 */
	if (!write) {
		return ret;
	}

	if (!nss_n2h_core0_mitigation_cfg) {
		printk(KERN_INFO "Disabling NSS MITIGATION\n");
		nss_n2h_mitigation_cfg(nss_ctx, 0, NSS_CORE_0);
		return 0;
	}
	printk(KERN_INFO "Invalid input value.Valid value is 0, Runtime re-enabling not supported\n");
	return -EINVAL;
}

/*
 * nss_mitigation_handler()
 * Enable NSS MITIGATION
 */
static int nss_n2h_mitigationcfg_core1_handler(struct ctl_table *ctl, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[NSS_CORE_1];
	int ret;

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (ret) {
		return ret;
	}

	/*
	 * It's a read operation
	 */
	if (!write) {
		return ret;
	}

	if (!nss_n2h_core1_mitigation_cfg) {
		printk(KERN_INFO "Disabling NSS MITIGATION\n");
		nss_n2h_mitigation_cfg(nss_ctx, 0, NSS_CORE_1);
		return 0;
	}
	printk(KERN_INFO "Invalid input value.Valid value is 0, Runtime re-enabling not supported\n");
	return -EINVAL;
}

/*
 * nss_buf_handler()
 *	Add extra NSS bufs from host memory
 */
static int nss_n2h_buf_cfg_core0_handler(struct ctl_table *ctl, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[NSS_CORE_0];
	int ret;

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (ret) {
		return ret;
	}

	/*
	 * It's a read operation
	 */
	if (!write) {
		return ret;
	}

	if (nss_ctx->buf_sz_allocated) {
		nss_n2h_core0_add_buf_pool_size = nss_ctx->buf_sz_allocated;
		return -EPERM;
	}

	if ((nss_n2h_core0_add_buf_pool_size >= 1) && (nss_n2h_core0_add_buf_pool_size <= NSS_N2H_MAX_BUF_POOL_SIZE)) {
		printk(KERN_INFO "configuring additional NSS pbufs\n");
		ret = nss_n2h_buf_pool_cfg(nss_ctx, nss_n2h_core0_add_buf_pool_size, NSS_CORE_0);
		nss_n2h_core0_add_buf_pool_size = nss_ctx->buf_sz_allocated;
		printk(KERN_INFO "additional pbufs of size %d got added to NSS\n", nss_ctx->buf_sz_allocated);
		return ret;
	}

	printk(KERN_INFO "Invalid input value. should be greater than 1 and less than %d\n", NSS_N2H_MAX_BUF_POOL_SIZE);
	return -EINVAL;
}

/*
 * nss_n2h_buf_handler()
 *	Add extra NSS bufs from host memory
 */
static int nss_n2h_buf_cfg_core1_handler(struct ctl_table *ctl, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[NSS_CORE_1];
	int ret;

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (ret) {
		return ret;
	}

	/*
	 * It's a read operation
	 */
	if (!write) {
		return ret;
	}

	if (nss_ctx->buf_sz_allocated) {
		nss_n2h_core1_add_buf_pool_size = nss_ctx->buf_sz_allocated;
		return -EPERM;
	}

	if ((nss_n2h_core1_add_buf_pool_size >= 1) && (nss_n2h_core1_add_buf_pool_size <= NSS_N2H_MAX_BUF_POOL_SIZE)) {
		printk(KERN_INFO "configuring additional NSS pbufs\n");
		ret = nss_n2h_buf_pool_cfg(nss_ctx, nss_n2h_core1_add_buf_pool_size, NSS_CORE_1);
		nss_n2h_core1_add_buf_pool_size = nss_ctx->buf_sz_allocated;
		printk(KERN_INFO "additional pbufs of size %d got added to NSS\n", nss_ctx->buf_sz_allocated);
		return ret;
	}

	printk(KERN_INFO "Invalid input value. should be greater than 1 and less than %d\n", NSS_N2H_MAX_BUF_POOL_SIZE);
	return -EINVAL;
}

static struct ctl_table nss_n2h_table[] = {
	{
		.procname	= "n2h_empty_pool_buf_core0",
		.data		= &nss_n2h_empty_pool_buf_cfg[NSS_CORE_0],
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_empty_pool_buf_cfg_core0_handler,
	},
	{
		.procname	= "n2h_empty_pool_buf_core1",
		.data		= &nss_n2h_empty_pool_buf_cfg[NSS_CORE_1],
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_empty_pool_buf_cfg_core1_handler,
	},
	{
		.procname	= "n2h_low_water_core0",
		.data		= &nss_n2h_water_mark[NSS_CORE_0][0],
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_water_mark_core0_handler,
	},
	{
		.procname	= "n2h_low_water_core1",
		.data		= &nss_n2h_water_mark[NSS_CORE_1][0],
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_water_mark_core1_handler,
	},
	{
		.procname	= "n2h_high_water_core0",
		.data		= &nss_n2h_water_mark[NSS_CORE_0][1],
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_water_mark_core0_handler,
	},
	{
		.procname	= "n2h_high_water_core1",
		.data		= &nss_n2h_water_mark[NSS_CORE_1][1],
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_water_mark_core1_handler,
	},
	{
		.procname	= "n2h_wifi_pool_buf",
		.data		= &nss_n2h_wifi_pool_buf_cfg,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_wifi_payloads_handler,
	},
	{
		.procname	= "rps",
		.data		= &nss_n2h_rps_config,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_rpscfg_handler,
	},
	{
		.procname	= "mitigation_core0",
		.data		= &nss_n2h_core0_mitigation_cfg,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_mitigationcfg_core0_handler,
	},
	{
		.procname	= "mitigation_core1",
		.data		= &nss_n2h_core1_mitigation_cfg,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_mitigationcfg_core1_handler,
	},
	{
		.procname	= "extra_pbuf_core0",
		.data		= &nss_n2h_core0_add_buf_pool_size,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_buf_cfg_core0_handler,
	},
	{
		.procname	= "extra_pbuf_core1",
		.data		= &nss_n2h_core1_add_buf_pool_size,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_n2h_buf_cfg_core1_handler,
	},

	{ }
};

static struct ctl_table nss_n2h_dir[] = {
	{
		.procname		= "n2hcfg",
		.mode			= 0555,
		.child			= nss_n2h_table,
	},
	{ }
};


static struct ctl_table nss_n2h_root_dir[] = {
	{
		.procname		= "nss",
		.mode			= 0555,
		.child			= nss_n2h_dir,
	},
	{ }
};

static struct ctl_table nss_n2h_root[] = {
	{
		.procname		= "dev",
		.mode			= 0555,
		.child			= nss_n2h_root_dir,
	},
	{ }
};

static struct ctl_table_header *nss_n2h_header;

/*
 * nss_n2h_flush_payloads()
 * 	Sends a command down to NSS for flushing all payloads
 */
nss_tx_status_t nss_n2h_flush_payloads(struct nss_ctx_instance *nss_ctx)
{
	struct nss_n2h_msg nnm;
	struct nss_n2h_flush_payloads *nnflshpl;
	nss_tx_status_t nss_tx_status;

	nnflshpl = &nnm.msg.flush_payloads;

	/*
	 * TODO: No additional information sent in message
	 * as of now. Need to initialize message content accordingly
	 * if needed.
	 */
	nss_n2h_msg_init(&nnm, NSS_N2H_INTERFACE,
			NSS_TX_METADATA_TYPE_N2H_FLUSH_PAYLOADS,
			sizeof(struct nss_n2h_flush_payloads),
			NULL,
			NULL);

	nss_tx_status = nss_n2h_tx_msg(nss_ctx, &nnm);
	if (nss_tx_status != NSS_TX_SUCCESS) {
		nss_warning("%p: failed to send flush payloads command to NSS\n",
				nss_ctx);

		return NSS_TX_FAILURE;
	}

	return NSS_TX_SUCCESS;
}

/*
 * nss_n2h_msg_init()
 *	Initialize n2h message.
 */
void nss_n2h_msg_init(struct nss_n2h_msg *nim, uint16_t if_num, uint32_t type,
		      uint32_t len, nss_n2h_msg_callback_t cb, void *app_data)
{
	nss_cmn_msg_init(&nim->cm, if_num, type, len, (void *)cb, app_data);
}

/*
 * nss_n2h_tx_msg()
 *	Send messages to NSS n2h pacakge
 */
nss_tx_status_t nss_n2h_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_n2h_msg *nnm)
{
	struct nss_n2h_msg *nnm2;
	struct nss_cmn_msg *ncm = &nnm->cm;
	struct sk_buff *nbuf;
	nss_tx_status_t status;

	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		return NSS_TX_FAILURE_NOT_READY;
	}

	/*
	 * Sanity check the message
	 */
	if (ncm->interface != NSS_N2H_INTERFACE) {
		nss_warning("%p: tx request for another interface: %d", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	if (ncm->type >= NSS_METADATA_TYPE_N2H_MAX) {
		nss_warning("%p: message type out of range: %d", nss_ctx, ncm->type);
		return NSS_TX_FAILURE;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_n2h_msg)) {
		nss_warning("%p: tx request for another interface: %d", nss_ctx, nss_cmn_get_msg_len(ncm));
		return NSS_TX_FAILURE;
	}


	nbuf = dev_alloc_skb(NSS_NBUF_PAYLOAD_SIZE);
	if (unlikely(!nbuf)) {
		NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_NBUF_ALLOC_FAILS]);
		return NSS_TX_FAILURE;
	}

	/*
	 * Copy the message to our skb.
	 */
	nnm2 = (struct nss_n2h_msg *)skb_put(nbuf, sizeof(struct nss_n2h_msg));
	memcpy(nnm2, nnm, sizeof(struct nss_n2h_msg));
	status = nss_core_send_buffer(nss_ctx, 0, nbuf, NSS_IF_CMD_QUEUE, H2N_BUFFER_CTRL, 0);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		dev_kfree_skb_any(nbuf);
		nss_info("%p: unable to enqueue 'nss frequency change' - marked as stopped\n", nss_ctx);
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);
	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_CMD_REQ]);
	return NSS_TX_SUCCESS;
}

/*
 * nss_n2h_notify_register()
 *	Register to received N2H events.
 *
 * NOTE: Do we want to pass an nss_ctx here so that we can register for n2h on any core?
 */
struct nss_ctx_instance *nss_n2h_notify_register(int core, nss_n2h_msg_callback_t cb, void *app_data)
{
	if (core >= NSS_MAX_CORES) {
		nss_warning("Input core number %d is wrong \n", core);
		return NULL;
	}
	/*
	 * TODO: We need to have a new array in support of the new API
	 * TODO: If we use a per-context array, we would move the array into nss_ctx based.
	 */
	nss_n2h_rd[core].n2h_callback = cb;
	nss_n2h_rd[core].app_data = app_data;
	return &nss_top_main.nss[core];
}

/*
 * nss_n2h_register_handler()
 */
void nss_n2h_register_handler()
{
	nss_core_register_handler(NSS_N2H_INTERFACE, nss_n2h_interface_handler, NULL);
}

/*
 * nss_n2h_register_sysctl()
 */
void nss_n2h_register_sysctl(void)
{
	/*
	 * RPS sema init
	 */
	sema_init(&nss_n2h_rcp.sem, 1);
	init_completion(&nss_n2h_rcp.complete);

	/*
	 * MITIGATION sema init for core0
	 */
	sema_init(&nss_n2h_mitigationcp[NSS_CORE_0].sem, 1);
	init_completion(&nss_n2h_mitigationcp[NSS_CORE_0].complete);

	/*
	 * MITIGATION sema init for core1
	 */
	sema_init(&nss_n2h_mitigationcp[NSS_CORE_1].sem, 1);
	init_completion(&nss_n2h_mitigationcp[NSS_CORE_1].complete);

	/*
	 * PBUF addition sema init for core0
	 */
	sema_init(&nss_n2h_bufcp[NSS_CORE_0].sem, 1);
	init_completion(&nss_n2h_bufcp[NSS_CORE_0].complete);

	/*
	 * PBUF addition sema init for core1
	 */
	sema_init(&nss_n2h_bufcp[NSS_CORE_1].sem, 1);
	init_completion(&nss_n2h_bufcp[NSS_CORE_1].complete);

	/*
	 * Core0
	 */
	sema_init(&nss_n2h_nepbcfgp[NSS_CORE_0].sem, 1);
	init_completion(&nss_n2h_nepbcfgp[NSS_CORE_0].complete);
	nss_n2h_nepbcfgp[NSS_CORE_0].empty_buf_pool =
		nss_n2h_empty_pool_buf_cfg[NSS_CORE_0];
	nss_n2h_nepbcfgp[NSS_CORE_0].low_water =
		nss_n2h_water_mark[NSS_CORE_0][0];
	nss_n2h_nepbcfgp[NSS_CORE_0].high_water =
		nss_n2h_water_mark[NSS_CORE_0][1];

	/*
	 * Core1
	 */
	sema_init(&nss_n2h_nepbcfgp[NSS_CORE_1].sem, 1);
	init_completion(&nss_n2h_nepbcfgp[NSS_CORE_1].complete);
	nss_n2h_nepbcfgp[NSS_CORE_1].empty_buf_pool =
		nss_n2h_empty_pool_buf_cfg[NSS_CORE_1];
	nss_n2h_nepbcfgp[NSS_CORE_1].low_water =
		nss_n2h_water_mark[NSS_CORE_1][0];
	nss_n2h_nepbcfgp[NSS_CORE_1].high_water =
		nss_n2h_water_mark[NSS_CORE_1][1];

	/*
	 * WiFi pool buf cfg sema init
	 */
	sema_init(&nss_n2h_wp.sem, 1);
	init_completion(&nss_n2h_wp.complete);

	nss_n2h_notify_register(NSS_CORE_0, NULL, NULL);
	nss_n2h_notify_register(NSS_CORE_1, NULL, NULL);

	/*
	 * Register sysctl table.
	 */
	nss_n2h_header = register_sysctl_table(nss_n2h_root);
}

/*
 * nss_n2h_unregister_sysctl()
 *	Unregister sysctl specific to n2h
 */
void nss_n2h_unregister_sysctl(void)
{
	/*
	 * Unregister sysctl table.
	 */
	if (nss_n2h_header) {
		unregister_sysctl_table(nss_n2h_header);
	}
}

EXPORT_SYMBOL(nss_n2h_notify_register);
