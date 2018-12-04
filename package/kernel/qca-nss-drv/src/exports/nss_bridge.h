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
 * nss_bridge.h
 *	NSS TO HLOS interface definitions.
 */

#ifndef __NSS_BRIDGE_H
#define __NSS_BRIDGE_H

/**
 * Bridge message types
 */
enum nss_bridge_msg_types {
	NSS_BRIDGE_MSG_JOIN = NSS_IF_MAX_MSG_TYPES + 1,
					/**< Msg to join interface to bridge */
	NSS_BRIDGE_MSG_LEAVE,		/**< Msg to remove interface from bridge */
	NSS_BRIDGE_MSG_TYPE_MAX,	/**< Max message types */
};

/**
 * Bridge error types
 */
enum nss_bridge_error_types {
	NSS_BRIDGE_ERROR_UNKNOWN_MSG = NSS_IF_ERROR_TYPE_MAX + 1,
					/**< Message type is unknown */
	NSS_BRIDGE_ERROR_TYPE_MAX,	/**< Max message types */
};

/**
 * Join bridge message
 */
struct nss_bridge_join_msg {
	uint32_t if_num;	/**< NSS interface to add to bridge */
};

/**
 * Leave bridge message
 */
struct nss_bridge_leave_msg {
	uint32_t if_num;	/**< NSS interface to remove from bridge */
};

/**
 * Message structure to send/receive bridge messages
 */
struct nss_bridge_msg {
	struct nss_cmn_msg cm;					/**< Message Header */
	union {
		union nss_if_msgs if_msg;			/**< nss_if base messages */
		struct nss_bridge_join_msg br_join;		/**< Bridge join message */
		struct nss_bridge_leave_msg br_leave;		/**< Bridge leave message */
	} msg;
};

/**
 * @brief Send bridge messages
 *
 * @param nss_ctx NSS context
 * @param msg NSS bridge message
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_bridge_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_bridge_msg *msg);

/**
 * @brief Send bridge messages synchronously
 *
 * @param nss_ctx NSS context
 * @param msg NSS bridge message
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_bridge_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_bridge_msg *msg);

/**
 * @brief Initialize bridge msg
 *
 * @param nss_bridge_msg
 * @param if_num Interface number
 * @param type Message type
 * @param len Message length
 * @param cb message callback
 * @param app_data
 *
 * @return None
 */
void nss_bridge_msg_init(struct nss_bridge_msg *ncm, uint16_t if_num, uint32_t type,  uint32_t len, void *cb, void *app_data);

/**
 * @brief Get the bridge context used in the nss_bridge_tx
 *
 * @return struct nss_ctx_instance *NSS context
 */
struct nss_ctx_instance *nss_bridge_get_context(void);

/**
 * @brief Callback when bridge data is received
 *
 * @param netdevice of bridge interface
 * @param skb pointer to data buffer
 * @param napi pointer
 *
 * @return void
 */
typedef void (*nss_bridge_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief Callback to receive bridge messages
 *
 * @param app_data Application context of the message
 * @param msg Message data
 *
 * @return void
 */
typedef void (*nss_bridge_msg_callback_t)(void *app_data, struct nss_bridge_msg *msg);

/**
 * @brief Register to send/receive bridge messages to NSS
 *
 * @return nss_ctx_instance* NSS context
 */
struct nss_ctx_instance *nss_bridge_register(uint32_t if_num, struct net_device *netdev, nss_bridge_callback_t bridge_data_cb, nss_bridge_msg_callback_t bridge_msg_cb, uint32_t features, void *app_data);

/**
 * @brief Unregister bridge interface with NSS
 *
 * @return void
 */
void nss_bridge_unregister(uint32_t if_num);

/**
 * @brief Register a notifier callback for bridge messages from NSS
 *
 * @param cb The callback pointer
 * @param app_data The application context for this message
 *
 * @return struct nss_ctx_instance * The NSS context
 */
struct nss_ctx_instance *nss_bridge_notify_register(nss_bridge_msg_callback_t cb, void *app_data);

/**
 * @brief Un-Register a notifier callback for bridge messages from NSS
 *
 * @return None
 */
void nss_bridge_notify_unregister(void);

/*
 * @brief Send bridge set mtu message
 *
 * @param nss_ctx NSS context
 * @param mtu MTU value to be set
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_bridge_tx_set_mtu_msg(uint32_t bridge_if_num, uint32_t mtu);

/**
 * @brief Send bridge set mac address message
 *
 * @param nss_ctx NSS context
 * @param addr MAC addr to be set
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_bridge_tx_set_mac_addr_msg(uint32_t bridge_if_num, uint8_t *addr);

/**
 * @brief Send bridge join message
 *
 * @param nss_ctx NSS context
 * @param netdev Slave interface joining bridge
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_bridge_tx_join_msg(uint32_t bridge_if_num, struct net_device *netdev);

/**
 * @brief Send bridge leave message
 *
 * @param nss_ctx NSS context
 * @param netdev Slave interface leaving bridge
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_bridge_tx_leave_msg(uint32_t bridge_if_num, struct net_device *netdev);

/**
 * @brief Send bridge vsi assign message
 *
 * @param if_num Bridge interface number
 * @param vsi vsi to assign
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_bridge_tx_vsi_assign_msg(uint32_t if_num, uint32_t vsi);

/**
 * @brief Send bridge vsi unassign message
 *
 * @param if_num Bridge interface number
 * @param vsi vsi to unassign
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_bridge_tx_vsi_unassign_msg(uint32_t if_num, uint32_t vsi);

/**
 * @brief Initialize bridge
 *
 * @return None
 */
void nss_bridge_init(void);

#endif /* __NSS_BRIDGE_H */
