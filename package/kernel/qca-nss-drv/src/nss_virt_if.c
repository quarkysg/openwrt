/*
 **************************************************************************
 * Copyright (c) 2014-2017, The Linux Foundation. All rights reserved.
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
 * nss_virt_if.c
 *	NSS virtual/redirect handler APIs
 */

#include "nss_tx_rx_common.h"
#include <net/arp.h>

#define NSS_VIRT_IF_TX_TIMEOUT			3000 /* 3 Seconds */
#define NSS_VIRT_IF_GET_INDEX(if_num)	(if_num-NSS_DYNAMIC_IF_START)

extern int nss_ctl_redirect;

/*
 * Data structure that holds the virtual interface context.
 */
static struct nss_virt_if_handle *nss_virt_if_handle_t[NSS_MAX_DYNAMIC_INTERFACES];

/*
 * Spinlock to protect the global data structure virt_handle.
 */
DEFINE_SPINLOCK(nss_virt_if_lock);

/*
 * nss_virt_if_stats_sync()
 *	Sync stats from the NSS FW
 */
static void nss_virt_if_stats_sync(struct nss_virt_if_handle *handle,
					struct nss_virt_if_stats *nwis)
{
	struct nss_virt_if_stats *stats = &handle->stats;

	stats->node_stats.rx_packets += nwis->node_stats.rx_packets;
	stats->node_stats.rx_bytes += nwis->node_stats.rx_bytes;
	stats->node_stats.rx_dropped += nwis->node_stats.rx_dropped;
	stats->node_stats.tx_packets += nwis->node_stats.tx_packets;
	stats->node_stats.tx_bytes += nwis->node_stats.tx_bytes;
	stats->tx_enqueue_failed += nwis->tx_enqueue_failed;
	stats->shaper_enqueue_failed += nwis->shaper_enqueue_failed;
}

/*
 * nss_virt_if_msg_handler()
 *	Handle msg responses from the FW on virtual interfaces
 */
static void nss_virt_if_msg_handler(struct nss_ctx_instance *nss_ctx,
					struct nss_cmn_msg *ncm,
					void *app_data)
{
	struct nss_virt_if_msg *nvim = (struct nss_virt_if_msg *)ncm;
	int32_t if_num;

	nss_virt_if_msg_callback_t cb;
	struct nss_virt_if_handle *handle = NULL;

	/*
	 * Sanity check the message type
	 */
	if (ncm->type > NSS_VIRT_IF_MAX_MSG_TYPES) {
		nss_warning("%p: message type out of range: %d", nss_ctx, ncm->type);
		return;
	}

	/*
	 * Messages value that are within the base class are handled by the base class.
	 */
	if (ncm->type < NSS_IF_MAX_MSG_TYPES) {
		return nss_if_msg_handler(nss_ctx, ncm, app_data);
	}

	if (!NSS_IS_IF_TYPE(DYNAMIC, ncm->interface)) {
		nss_warning("%p: response for another interface: %d", nss_ctx, ncm->interface);
		return;
	}

	if_num = NSS_VIRT_IF_GET_INDEX(ncm->interface);

	spin_lock_bh(&nss_virt_if_lock);
	if (!nss_virt_if_handle_t[if_num]) {
		spin_unlock_bh(&nss_virt_if_lock);
		nss_warning("%p: virt_if handle is NULL\n", nss_ctx);
		return;
	}

	handle = nss_virt_if_handle_t[if_num];
	spin_unlock_bh(&nss_virt_if_lock);

	switch (nvim->cm.type) {
	case NSS_VIRT_IF_STATS_SYNC_MSG:
		nss_virt_if_stats_sync(handle, &nvim->msg.stats);
		break;
	}

	/*
	 * Log failures
	 */
	nss_core_log_msg_failures(nss_ctx, ncm);

	/*
	 * Update the callback and app_data for NOTIFY messages, IPv4 sends all notify messages
	 * to the same callback/app_data.
	 */
	if (ncm->response == NSS_CMM_RESPONSE_NOTIFY) {
		ncm->cb = (nss_ptr_t)nss_ctx->nss_top->if_rx_msg_callback[ncm->interface];
		ncm->app_data = (nss_ptr_t)nss_ctx->subsys_dp_register[ncm->interface].ndev;
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
	cb = (nss_virt_if_msg_callback_t)ncm->cb;
	cb((void *)ncm->app_data, ncm);
}

/*
 * nss_virt_if_callback
 *	Callback to handle the completion of NSS ->HLOS messages.
 */
static void nss_virt_if_callback(void *app_data, struct nss_cmn_msg *ncm)
{
	struct nss_virt_if_handle *handle = (struct nss_virt_if_handle *)app_data;
	struct nss_virt_if_pvt *nvip = handle->pvt;

	if (ncm->response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("%p: virt_if Error response %d\n", handle->nss_ctx, ncm->response);
		nvip->response = NSS_TX_FAILURE;
		complete(&nvip->complete);
		return;
	}

	nvip->response = NSS_TX_SUCCESS;
	complete(&nvip->complete);
}

/*
 * nss_virt_if_tx_msg_sync
 *	Send a message from HLOS to NSS synchronously.
 */
static nss_tx_status_t nss_virt_if_tx_msg_sync(struct nss_virt_if_handle *handle,
						struct nss_virt_if_msg *nvim)
{
	nss_tx_status_t status;
	int ret = 0;
	struct nss_virt_if_pvt *nwip = handle->pvt;
	struct nss_ctx_instance *nss_ctx = handle->nss_ctx;

	down(&nwip->sem);

	status = nss_virt_if_tx_msg(nss_ctx, nvim);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: nss_virt_if_msg failed\n", nss_ctx);
		up(&nwip->sem);
		return status;
	}

	ret = wait_for_completion_timeout(&nwip->complete,
						msecs_to_jiffies(NSS_VIRT_IF_TX_TIMEOUT));
	if (!ret) {
		nss_warning("%p: virt_if tx failed due to timeout\n", nss_ctx);
		nwip->response = NSS_TX_FAILURE;
	}

	status = nwip->response;
	up(&nwip->sem);

	return status;
}

/*
 * nss_virt_if_msg_init()
 *	Initialize virt specific message structure.
 */
static void nss_virt_if_msg_init(struct nss_virt_if_msg *nvim,
					uint16_t if_num,
					uint32_t type,
					uint32_t len,
					nss_virt_if_msg_callback_t cb,
					struct nss_virt_if_handle *app_data)
{
	nss_cmn_msg_init(&nvim->cm, if_num, type, len, (void *)cb, (void *)app_data);
}

/*
 * nss_virt_if_register_handler()
 * 	register msg handler for virtual interface.
 */
static uint32_t nss_virt_if_register_handler(struct nss_virt_if_handle *handle)
{
	uint32_t ret;
	int32_t if_num = handle->if_num;

	ret = nss_core_register_handler(if_num, nss_virt_if_msg_handler, NULL);
	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("%d: Message handler failed to be registered for interface\n", if_num);
		return NSS_VIRT_IF_CORE_FAILURE;
	}

	return NSS_VIRT_IF_SUCCESS;
}

/*
 * nss_virt_if_handle_destroy_cb()
 *	Callback to handle the response of destroy message.
 */
static void nss_virt_if_handle_destroy_cb(void *app_data, struct nss_dynamic_interface_msg *ndim)
{
	nss_virt_if_msg_callback_t cb;
	void *data;
	uint32_t index;
	struct nss_virt_if_handle *handle = (struct nss_virt_if_handle *)app_data;

	cb = handle->cb;
	data = handle->app_data;

	if (ndim->cm.response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("%p: Received NACK from DI\n", handle->nss_ctx);
		goto callback;
	}

	index = NSS_VIRT_IF_GET_INDEX(handle->if_num);
	spin_lock_bh(&nss_virt_if_lock);
	nss_virt_if_handle_t[index] = NULL;
	spin_unlock_bh(&nss_virt_if_lock);

	kfree(handle);
callback:
	if (!cb) {
		nss_warning("callback is NULL\n");
		return;
	}

	cb(data, &ndim->cm);
}

/*
 * nss_virt_if_handle_destroy()
 *	Destroy the virt handle either due to request from WLAN or due to error.
 */
static int nss_virt_if_handle_destroy(struct nss_virt_if_handle *handle)
{
	nss_tx_status_t status;
	int32_t if_num = handle->if_num;
	struct nss_ctx_instance *nss_ctx = handle->nss_ctx;
	struct nss_dynamic_interface_msg ndim;
	struct nss_dynamic_interface_dealloc_node_msg *ndid;

	nss_dynamic_interface_msg_init(&ndim, NSS_DYNAMIC_INTERFACE, NSS_DYNAMIC_INTERFACE_DEALLOC_NODE,
				sizeof(struct nss_dynamic_interface_dealloc_node_msg), nss_virt_if_handle_destroy_cb, (void *)handle);

	ndid = &ndim.msg.dealloc_node;
	ndid->type = NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR;
	ndid->if_num = if_num;

	/*
	 * Send an asynchronous message to the firmware.
	 */
	status = nss_dynamic_interface_tx(nss_ctx, &ndim);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: Dynamic interface destroy failed status %d\n", nss_ctx, status);
		return status;
	}

	return status;
}

/*
 * nss_virt_if_handle_destroy_sync()
 *	Destroy the virt handle either due to request from user or due to error, synchronously.
 */
static int nss_virt_if_handle_destroy_sync(struct nss_virt_if_handle *handle)
{
	nss_tx_status_t status;
	int32_t if_num = handle->if_num;
	int32_t index = NSS_VIRT_IF_GET_INDEX(if_num);

	status = nss_dynamic_interface_dealloc_node(if_num, NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: Dynamic interface destroy failed status %d\n", handle->nss_ctx, status);
		return status;
	}

	spin_lock_bh(&nss_virt_if_lock);
	nss_virt_if_handle_t[index] = NULL;
	spin_unlock_bh(&nss_virt_if_lock);

	kfree(handle->pvt);
	kfree(handle);

	return status;
}

/*
 * nss_virt_if_handle_create_cb()
 *	Callback to handle the response from dynamic interface for an alloc node msg.
 */
static void nss_virt_if_handle_create_cb(void *app_data, struct nss_dynamic_interface_msg *ndim)
{
	struct nss_virt_if_handle *handle = (struct nss_virt_if_handle *)app_data;
	struct nss_ctx_instance *nss_ctx = handle->nss_ctx;
	nss_virt_if_msg_callback_t cb;
	struct nss_virt_if_msg nvim;
	struct nss_virt_if_create_msg *nvcm;
	void *data;
	int32_t if_num;
	uint32_t index;
	int ret;

	cb = handle->cb;
	data = handle->app_data;

	if (ndim->cm.response != NSS_CMN_RESPONSE_ACK) {
		nss_warning("%p: received NACK from NSS %d\n", handle->nss_ctx, ndim->cm.response);
		kfree(handle);
		goto fail;
	}

	if_num = ndim->msg.alloc_node.if_num;

	spin_lock_bh(&nss_virt_if_lock);
	handle->if_num = if_num;
	index = NSS_VIRT_IF_GET_INDEX(if_num);
	nss_virt_if_handle_t[index] = handle;
	spin_unlock_bh(&nss_virt_if_lock);

	/* Register the msg handler for if_num */
	ret = nss_virt_if_register_handler(handle);
	if (ret != NSS_VIRT_IF_SUCCESS) {
		nss_warning("%p: Registration handler failed reason: %d\n", handle->nss_ctx, ret);
		nss_virt_if_handle_destroy(handle);
		goto fail;
	}

	/*
	 * Send virt_if message to the Firmware to copy the netdev's macaddr
	 */
	nss_virt_if_msg_init(&nvim, handle->if_num, NSS_VIRT_IF_TX_CREATE_MSG,
				sizeof(struct nss_virt_if_create_msg), NULL, NULL);

	nvcm = &nvim.msg.if_create;
	nvcm->flags = 0;
	memcpy(nvcm->mac_addr, handle->ndev->dev_addr, ETH_ALEN);

	ret = nss_virt_if_tx_msg(handle->nss_ctx, &nvim);
	if (ret != NSS_TX_SUCCESS) {
		nss_warning("%p: nss_virt_if_tx_msg failed %u\n", handle->nss_ctx, ret);
		nss_virt_if_handle_destroy(handle);
		goto fail;
	}

	spin_lock_bh(&nss_top_main.lock);
	if (!nss_ctx->subsys_dp_register[handle->if_num].ndev) {
		nss_ctx->subsys_dp_register[handle->if_num].ndev = handle->ndev;
	}
	spin_unlock_bh(&nss_top_main.lock);

	/*
	 * Hold a reference to the net_device
	 */
	dev_hold(handle->ndev);

	/*
	 * The context returned is the handle which contains all the info related to
	 * the interface if_num.
	 */

fail:
	if (!cb) {
		nss_warning("cb is NULL\n");
		return;
	}

	cb(data, &ndim->cm);
}

/*
 * nss_virt_if_handle_create()
 *	Create and initialize virt_if handle.
 */
static struct nss_virt_if_handle *nss_virt_if_handle_create(struct nss_ctx_instance *nss_ctx,
							struct net_device *ndev,
							nss_virt_if_msg_callback_t cb,
							void *app_data,
							int *cmd_rsp)
{
	struct nss_virt_if_handle *handle;

	handle = (struct nss_virt_if_handle *)kzalloc(sizeof(struct nss_virt_if_handle),
									GFP_ATOMIC);
	if (!handle) {
		nss_warning("%p: handle memory alloc failed\n", nss_ctx);
		*cmd_rsp = NSS_VIRT_IF_ALLOC_FAILURE;
		goto error;
	}

	/*
	 * Save the callback & appdata of the user. The callback will be invoked from
	 * nss_virt_if_handle_create_cb.
	 */
	handle->nss_ctx = nss_ctx;
	handle->ndev = ndev;
	handle->cb = cb;
	handle->app_data = app_data;

	*cmd_rsp = NSS_VIRT_IF_SUCCESS;

	return handle;

error:
	return NULL;
}

/*
 * nss_virt_if_handle_create_sync()
 *	Initialize virt handle which holds the if_num and stats per interface.
 */
static struct nss_virt_if_handle *nss_virt_if_handle_create_sync(struct nss_ctx_instance *nss_ctx, int32_t if_num, int32_t *cmd_rsp)
{
	int32_t index;
	struct nss_virt_if_handle *handle;

	index = NSS_VIRT_IF_GET_INDEX(if_num);

	handle = (struct nss_virt_if_handle *)kzalloc(sizeof(struct nss_virt_if_handle),
									GFP_KERNEL);
	if (!handle) {
		nss_warning("%p: handle memory alloc failed\n", nss_ctx);
		*cmd_rsp = NSS_VIRT_IF_ALLOC_FAILURE;
		goto error1;
	}

	handle->nss_ctx = nss_ctx;
	handle->if_num = if_num;
	handle->pvt = (struct nss_virt_if_pvt *)kzalloc(sizeof(struct nss_virt_if_pvt),
								GFP_KERNEL);
	if (!handle->pvt) {
		nss_warning("%p: failure allocating memory for nss_virt_if_pvt\n", nss_ctx);
		*cmd_rsp = NSS_VIRT_IF_ALLOC_FAILURE;
		goto error2;
	}

	handle->cb = NULL;
	handle->app_data = NULL;

	spin_lock_bh(&nss_virt_if_lock);
	nss_virt_if_handle_t[index] = handle;
	spin_unlock_bh(&nss_virt_if_lock);

	*cmd_rsp = NSS_VIRT_IF_SUCCESS;

	return handle;

error2:
	kfree(handle);
error1:
	return NULL;
}

/*
 * nss_virt_if_register_handler_sync()
 * 	register msg handler for virtual interface and initialize semaphore and completion.
 */
static uint32_t nss_virt_if_register_handler_sync(struct nss_virt_if_handle *handle)
{
	uint32_t ret;
	struct nss_virt_if_pvt *nvip = NULL;
	int32_t if_num = handle->if_num;

	ret = nss_core_register_handler(if_num, nss_virt_if_msg_handler, NULL);
	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("%d: Message handler failed to be registered for interface\n", if_num);
		return NSS_VIRT_IF_CORE_FAILURE;
	}

	nvip = handle->pvt;
	if (!nvip->sem_init_done) {
		sema_init(&nvip->sem, 1);
		init_completion(&nvip->complete);
		nvip->sem_init_done = 1;
	}

	return NSS_VIRT_IF_SUCCESS;
}

/*
 * nss_virt_if_create()
 *	Create a virtual interface and associate it with the netdev, asynchronously.
 */
int nss_virt_if_create(struct net_device *netdev, nss_virt_if_msg_callback_t cb, void *app_data)
{
	struct nss_ctx_instance *nss_ctx = &nss_top_main.nss[nss_top_main.wlan_handler_id];
	struct nss_dynamic_interface_msg ndim;
	struct nss_dynamic_interface_alloc_node_msg *ndia;
	int ret;
	struct nss_virt_if_handle *handle = NULL;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: Interface could not be created as core not ready\n", nss_ctx);
		return NSS_VIRT_IF_CORE_FAILURE;
	}

	/*
	 * Create the virt_if handle and associate the cb and app_data with it.
	 */
	handle = nss_virt_if_handle_create(nss_ctx, netdev, cb, app_data, &ret);
	if (!handle) {
		nss_warning("%p:virt_if handle create failed ret %d\n", nss_ctx, ret);
		return ret;
	}

	ndia = &ndim.msg.alloc_node;
	ndia->type = NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR;

	/*
	 * Construct a dynamic interface message and send it to the firmware.
	 */
	nss_dynamic_interface_msg_init(&ndim, NSS_DYNAMIC_INTERFACE, NSS_DYNAMIC_INTERFACE_ALLOC_NODE,
				sizeof(struct nss_dynamic_interface_alloc_node_msg), nss_virt_if_handle_create_cb, (void *)handle);

	ret = nss_dynamic_interface_tx(nss_ctx, &ndim);
	if (ret != NSS_TX_SUCCESS) {
		nss_warning("%p: failure allocating virt if\n", nss_ctx);
		ret = NSS_VIRT_IF_DYNAMIC_IF_FAILURE;
		kfree(handle);
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(nss_virt_if_create);

/*
 * nss_virt_if_create_sync()
 *	Create a virt interface, synchronously and associate it with the netdev
 */
struct nss_virt_if_handle *nss_virt_if_create_sync(struct net_device *netdev)
{
	struct nss_ctx_instance *nss_ctx = &nss_top_main.nss[nss_top_main.wlan_handler_id];
	struct nss_virt_if_msg nvim;
	struct nss_virt_if_create_msg *nvcm;
	uint32_t ret;
	struct nss_virt_if_handle *handle = NULL;
	int32_t if_num;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: Interface could not be created as core not ready\n", nss_ctx);
		return NULL;
	}

	if_num = nss_dynamic_interface_alloc_node(NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR);
	if (if_num < 0) {
		nss_warning("%p: failure allocating virt if\n", nss_ctx);
		return NULL;
	}

	handle = nss_virt_if_handle_create_sync(nss_ctx, if_num, &ret);
	if (!handle) {
		nss_warning("%p:virt_if handle creation failed ret %d\n", nss_ctx, ret);
		nss_dynamic_interface_dealloc_node(if_num, NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR);
		return NULL;
	}

	/* Initializes the semaphore and also sets the msg handler for if_num */
	ret = nss_virt_if_register_handler_sync(handle);
	if (ret != NSS_VIRT_IF_SUCCESS) {
		nss_warning("%p: Registration handler failed reason: %d\n", nss_ctx, ret);
		goto error;
	}

	nss_virt_if_msg_init(&nvim, handle->if_num, NSS_VIRT_IF_TX_CREATE_MSG,
				sizeof(struct nss_virt_if_create_msg), nss_virt_if_callback, handle);

	nvcm = &nvim.msg.if_create;
	nvcm->flags = 0;
	memcpy(nvcm->mac_addr, netdev->dev_addr, ETH_ALEN);

	ret = nss_virt_if_tx_msg_sync(handle, &nvim);
	if (ret != NSS_TX_SUCCESS) {
		nss_warning("%p: nss_virt_if_tx_msg_sync failed %u\n", nss_ctx, ret);
		goto error;
	}

	spin_lock_bh(&nss_top_main.lock);
	if (!nss_ctx->subsys_dp_register[handle->if_num].ndev) {
		nss_ctx->subsys_dp_register[handle->if_num].ndev = netdev;
	}
	spin_unlock_bh(&nss_top_main.lock);

	/*
	 * Hold a reference to the net_device
	 */
	dev_hold(netdev);

	/*
	 * The context returned is the handle interface # which contains all the info related to
	 * the interface if_num.
	 */

	return handle;

error:
	nss_virt_if_handle_destroy_sync(handle);
	return NULL;
}
EXPORT_SYMBOL(nss_virt_if_create_sync);

/*
 * nss_virt_if_destroy()
 *	Destroy the virt interface associated with the interface number, by sending
 * sending an asynchronous message to dynamic interface.
 */
nss_tx_status_t nss_virt_if_destroy(struct nss_virt_if_handle *handle, nss_virt_if_msg_callback_t cb, void *app_data)
{
	nss_tx_status_t status;
	struct net_device *dev;
	int32_t if_num;
	struct nss_ctx_instance *nss_ctx;
	uint32_t ret;

	if (!handle) {
		nss_warning("handle is NULL\n");
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	if_num = handle->if_num;
	nss_ctx = handle->nss_ctx;
	handle->cb = cb;
	handle->app_data = app_data;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: Interface could not be destroyed as core not ready\n", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	spin_lock_bh(&nss_top_main.lock);
	if (!nss_ctx->subsys_dp_register[if_num].ndev) {
		spin_unlock_bh(&nss_top_main.lock);
		nss_warning("%p: Unregister virt interface %d: no context\n", nss_ctx, if_num);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	dev = nss_ctx->subsys_dp_register[if_num].ndev;
	nss_ctx->subsys_dp_register[if_num].ndev = NULL;
	spin_unlock_bh(&nss_top_main.lock);
	dev_put(dev);

	status = nss_virt_if_handle_destroy(handle);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: handle destroy tx failed %d\n", nss_ctx, status);
		return NSS_TX_FAILURE;
	}

	ret = nss_core_unregister_handler(if_num);
	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("Not able to unregister handler for virt_if interface %d with NSS core\n", if_num);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	return status;
}
EXPORT_SYMBOL(nss_virt_if_destroy);

/*
 * nss_virt_if_destroy_sync()
 *	Destroy the virt interface associated with the interface number, synchronously.
 */
nss_tx_status_t nss_virt_if_destroy_sync(struct nss_virt_if_handle *handle)
{
	nss_tx_status_t status;
	struct net_device *dev;
	int32_t if_num;
	struct nss_ctx_instance *nss_ctx;
	uint32_t ret;

	if (!handle) {
		nss_warning("handle is NULL\n");
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	if_num = handle->if_num;
	nss_ctx = handle->nss_ctx;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: Interface could not be destroyed as core not ready\n", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	spin_lock_bh(&nss_top_main.lock);
	if (!nss_ctx->subsys_dp_register[if_num].ndev) {
		spin_unlock_bh(&nss_top_main.lock);
		nss_warning("%p: Unregister virt interface %d: no context\n", nss_ctx, if_num);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	dev = nss_ctx->subsys_dp_register[if_num].ndev;
	nss_ctx->subsys_dp_register[if_num].ndev = NULL;
	spin_unlock_bh(&nss_top_main.lock);
	dev_put(dev);

	status = nss_virt_if_handle_destroy_sync(handle);
	if (status != NSS_TX_SUCCESS) {
		nss_warning("%p: handle destroy failed for if_num %d\n", nss_ctx, if_num);
		return NSS_TX_FAILURE;
	}

	ret = nss_core_unregister_handler(if_num);
	if (ret != NSS_CORE_STATUS_SUCCESS) {
		nss_warning("%p: Not able to unregister handler for virt_if interface %d with NSS core\n", nss_ctx, if_num);
		return NSS_TX_FAILURE_BAD_PARAM;
	}

	return status;
}
EXPORT_SYMBOL(nss_virt_if_destroy_sync);

/*
 * nss_virt_if_tx_buf()
 *	HLOS interface has received a packet which we redirect to the NSS, if appropriate to do so.
 */
nss_tx_status_t nss_virt_if_tx_buf(struct nss_virt_if_handle *handle,
						struct sk_buff *skb)
{
	int32_t status;
	int32_t if_num = handle->if_num;
	struct nss_ctx_instance *nss_ctx = handle->nss_ctx;

	if (unlikely(nss_ctl_redirect == 0)) {
		return NSS_TX_FAILURE_NOT_ENABLED;
	}

	if (unlikely(skb->vlan_tci)) {
		return NSS_TX_FAILURE_NOT_SUPPORTED;
	}

	nss_assert(NSS_IS_IF_TYPE(DYNAMIC, if_num));
	nss_trace("%p: Virtual Rx packet, if_num:%d, skb:%p", nss_ctx, if_num, skb);

	/*
	 * Get the NSS context that will handle this packet and check that it is initialised and ready
	 */
	NSS_VERIFY_CTX_MAGIC(nss_ctx);
	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("%p: Virtual Rx packet dropped as core not ready", nss_ctx);
		return NSS_TX_FAILURE_NOT_READY;
	}

	/*
	 * Sanity check the SKB to ensure that it's suitable for us
	 */
	if (unlikely(skb->len <= ETH_HLEN)) {
		nss_warning("%p: Virtual Rx packet: %p too short", nss_ctx, skb);
		return NSS_TX_FAILURE_TOO_SHORT;
	}

	if (unlikely(skb_shinfo(skb)->nr_frags != 0)) {
		/*
		 * TODO: If we have a connection matching rule for this skbuff,
		 * do we need to flush it??
		 */
		nss_warning("%p: Delivering the packet to Linux because of fragmented skb: %p\n", nss_ctx, skb);
		return NSS_TX_FAILURE_NOT_SUPPORTED;
	}

	/*
	 * Direct the buffer to the NSS
	 */
	status = nss_core_send_buffer(nss_ctx, if_num, skb, NSS_IF_DATA_QUEUE_0,
					H2N_BUFFER_PACKET, H2N_BIT_FLAG_VIRTUAL_BUFFER);
	if (unlikely(status != NSS_CORE_STATUS_SUCCESS)) {
		nss_warning("%p: Virtual Rx packet unable to enqueue\n", nss_ctx);
		return NSS_TX_FAILURE_QUEUE;
	}

	/*
	 * Kick the NSS awake so it can process our new entry.
	 */
	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);
	NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_TX_PACKET]);
	return NSS_TX_SUCCESS;
}
EXPORT_SYMBOL(nss_virt_if_tx_buf);

/*
 * nss_virt_if_tx_msg()
 */
nss_tx_status_t nss_virt_if_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_virt_if_msg *nvim)
{
	int32_t status;
	struct sk_buff *nbuf;
	struct nss_cmn_msg *ncm = &nvim->cm;
	struct nss_virt_if_msg *nvim2;

	if (unlikely(nss_ctx->state != NSS_CORE_STATE_INITIALIZED)) {
		nss_warning("Interface could not be created as core not ready");
		return NSS_TX_FAILURE;
	}

	/*
	 * Sanity check the message
	 */
	if (!NSS_IS_IF_TYPE(DYNAMIC, ncm->interface)) {
		nss_warning("%p: tx request for another interface: %d", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	if (ncm->type > NSS_VIRT_IF_MAX_MSG_TYPES) {
		nss_warning("%p: message type out of range: %d", nss_ctx, ncm->type);
		return NSS_TX_FAILURE;
	}

	if (nss_cmn_get_msg_len(ncm) > sizeof(struct nss_virt_if_msg)) {
		nss_warning("%p: invalid length: %d. Length of virt msg is %d",
				nss_ctx, nss_cmn_get_msg_len(ncm), (int)sizeof(struct nss_virt_if_msg));
		return NSS_TX_FAILURE;
	}

	nbuf = dev_alloc_skb(NSS_NBUF_PAYLOAD_SIZE);
	if (unlikely(!nbuf)) {
		NSS_PKT_STATS_INCREMENT(nss_ctx, &nss_ctx->nss_top->stats_drv[NSS_STATS_DRV_NBUF_ALLOC_FAILS]);
		nss_warning("%p: virtual interface %d: command allocation failed", nss_ctx, ncm->interface);
		return NSS_TX_FAILURE;
	}

	nvim2 = (struct nss_virt_if_msg *)skb_put(nbuf, sizeof(struct nss_virt_if_msg));
	memcpy(nvim2, nvim, sizeof(struct nss_virt_if_msg));

	status = nss_core_send_buffer(nss_ctx, 0, nbuf, NSS_IF_CMD_QUEUE, H2N_BUFFER_CTRL, 0);
	if (status != NSS_CORE_STATUS_SUCCESS) {
		dev_kfree_skb_any(nbuf);
		nss_warning("%p: Unable to enqueue 'virtual interface' command\n", nss_ctx);
		return NSS_TX_FAILURE;
	}

	nss_hal_send_interrupt(nss_ctx, NSS_H2N_INTR_DATA_COMMAND_QUEUE);

	/*
	 * The context returned is the virtual interface # which is, essentially, the index into the if_ctx
	 * array that is holding the net_device pointer
	 */
	return NSS_TX_SUCCESS;
}
EXPORT_SYMBOL(nss_virt_if_tx_msg);

/*
 * nss_virt_if_register()
 */
void nss_virt_if_register(struct nss_virt_if_handle *handle,
				nss_virt_if_data_callback_t data_callback,
				struct net_device *netdev)
{
	struct nss_ctx_instance *nss_ctx;
	int32_t if_num;

	if (!handle) {
		nss_warning("handle is NULL\n");
		return;
	}

	nss_ctx = handle->nss_ctx;
	if_num = handle->if_num;

	nss_ctx->subsys_dp_register[if_num].ndev = netdev;
	nss_ctx->subsys_dp_register[if_num].cb = data_callback;
	nss_ctx->subsys_dp_register[if_num].app_data = NULL;
	nss_ctx->subsys_dp_register[if_num].features = (uint32_t)netdev->features;

	nss_top_main.if_rx_msg_callback[if_num] = NULL;
}
EXPORT_SYMBOL(nss_virt_if_register);

/*
 * nss_virt_if_unregister()
 */
void nss_virt_if_unregister(struct nss_virt_if_handle *handle)
{
	struct nss_ctx_instance *nss_ctx;
	int32_t if_num;

	if (!handle) {
		nss_warning("handle is NULL\n");
		return;
	}

	nss_ctx = handle->nss_ctx;
	if_num = handle->if_num;

	nss_ctx->subsys_dp_register[if_num].ndev = NULL;
	nss_ctx->subsys_dp_register[if_num].cb = NULL;
	nss_ctx->subsys_dp_register[if_num].app_data = NULL;
	nss_ctx->subsys_dp_register[if_num].features = 0;

	nss_top_main.if_rx_msg_callback[if_num] = NULL;
}
EXPORT_SYMBOL(nss_virt_if_unregister);

/*
 * nss_virt_if_copy_stats()
 *	Copy stats from the virt_if handle to buffer(line)
 */
int32_t nss_virt_if_copy_stats(int32_t if_num, int i, char *line)
{
	int32_t bytes = 0;
	struct nss_virt_if_stats *stats;
	int32_t ifnum;
	uint32_t len = 80;
	struct nss_virt_if_handle *handle = NULL;

	if (if_num < 0) {
		nss_warning("invalid if_num\n");
		return 0;
	}

	ifnum = NSS_VIRT_IF_GET_INDEX(if_num);

	spin_lock_bh(&nss_virt_if_lock);
	if (!nss_virt_if_handle_t[ifnum]) {
		spin_unlock_bh(&nss_virt_if_lock);
		goto end;
	}

	handle = nss_virt_if_handle_t[ifnum];
	spin_unlock_bh(&nss_virt_if_lock);

	stats = &handle->stats;

	switch (i) {
	case 0:
		bytes = scnprintf(line, len, "rx_packets=%d\n",
					stats->node_stats.rx_packets);
		break;

	case 1:
		bytes = scnprintf(line, len, "rx_bytes=%d\n",
					stats->node_stats.rx_bytes);
		break;

	case 2:
		bytes = scnprintf(line, len, "rx_dropped=%d\n",
					stats->node_stats.rx_dropped);
		break;

	case 3:
		bytes = scnprintf(line, len, "tx_packets=%d\n",
					stats->node_stats.tx_packets);
		break;

	case 4:
		bytes = scnprintf(line, len, "tx_bytes=%d\n",
					stats->node_stats.tx_bytes);
		break;

	case 5:
		bytes = scnprintf(line, len, "tx_enqueue_failed=%d\n",
						stats->tx_enqueue_failed);
		break;

	case 6:
		bytes = scnprintf(line, len, "shaper_enqueue_failed=%d\n",
						stats->shaper_enqueue_failed);
		break;
	}

end:
	return bytes;
}

/*
 * nss_virt_if_get_interface_num()
 * Get interface number for a virtual interface
 */
int32_t nss_virt_if_get_interface_num(struct nss_virt_if_handle *handle)
{
	if (!handle) {
		nss_warning("virt_if handle is NULL\n");
		return -1;
	}

	return handle->if_num;
}
EXPORT_SYMBOL(nss_virt_if_get_interface_num);
