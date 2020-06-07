/*
 **************************************************************************
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
 * nss_vlan.h
 *	NSS TO HLOS interface definitions.
 */

#ifndef __NSS_VLAN_H
#define __NSS_VLAN_H

/**
 * Vlan message types
 */
enum nss_vlan_msg_types {
	NSS_VLAN_MSG_ADD_TAG = NSS_IF_MAX_MSG_TYPES + 1,
					/**< Msg to add vlan tag on physical interface */
	NSS_VLAN_MSG_TYPE_MAX,	/**< Max message types */
};

/**
 * Vlan error types
 */
enum nss_vlan_error_types {
	NSS_VLAN_ERROR_UNKNOWN_MSG = NSS_IF_ERROR_TYPE_MAX + 1,
					/**< Message type is unknown */
	NSS_VLAN_ERROR_TYPE_MAX,	/**< Max message types */
};

#define NSS_VLAN_TYPE_SINGLE 0
#define NSS_VLAN_TYPE_DOUBLE 1

/**
 * VLAN message - add vlan tag
 */
struct nss_vlan_msg_add_tag {
	uint32_t vlan_tag;	/* VLAN tag information */
	uint32_t next_hop;	/* Parent interface */
	uint32_t if_num;	/* Real Physical Interface */
};

/**
 * Message structure to send/receive vlan messages
 */
struct nss_vlan_msg {
	struct nss_cmn_msg cm;					/**< Message Header */
	union {
		union nss_if_msgs if_msg;			/**< nss_if base messages */
		struct nss_vlan_msg_add_tag add_tag;		/**< Vlan add tag message */
	} msg;
};

/**
 * @brief Send vlan messages
 *
 * @param nss_ctx NSS context
 * @param msg NSS vlan message
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_vlan_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_vlan_msg *msg);

/**
 * @brief Send vlan messages synchronously
 *
 * @param nss_ctx NSS context
 * @param msg NSS vlan message
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_vlan_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_vlan_msg *msg);

/**
 * @brief Initialize vlan msg
 *
 * @param nss_vlan_msg
 * @param if_num Interface number
 * @param type Message type
 * @param len Message length
 * @param cb message callback
 * @param app_data
 *
 * @return None
 */
void nss_vlan_msg_init(struct nss_vlan_msg *ncm, uint16_t if_num, uint32_t type,  uint32_t len, void *cb, void *app_data);

/**
 * @brief Get the vlan context used in the nss_vlan_tx
 *
 * @return struct nss_ctx_instance *NSS context
 */
struct nss_ctx_instance *nss_vlan_get_context(void);

/**
 * @brief Callback when vlan data is received
 *
 * @param netdevice of vlan interface
 * @param skb Pointer to data buffer
 * @param napi pointer
 *
 * @return void
 */
typedef void (*nss_vlan_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief Callback to receive vlan messages
 *
 * @param app_data Application context of the message
 * @param msg Message data
 *
 * @return void
 */
typedef void (*nss_vlan_msg_callback_t)(void *app_data, struct nss_vlan_msg *msg);

/**
 * @brief Register to send/receive vlan messages to NSS
 *
 * @param if_num NSS interface number
 * @param vlan_data_callback Callback for vlan data
 * @param netdev netdevice associated with the vlan interface
 * @param features denotes the skb types supported by this interface
 *
 * @return nss_ctx_instance* NSS context
 */
struct nss_ctx_instance *nss_register_vlan_if(uint32_t if_num, nss_vlan_callback_t vlan_data_callback,
					      struct net_device *netdev, uint32_t features, void *app_ctx);

/**
 * @brief Unregister vlan interface with NSS
 *
 * @return void
 */
void nss_unregister_vlan_if(uint32_t if_num);

/*
 * @brief Send vlan set mtu message
 *
 * @param vlan_if_num NSS dynamic interface
 * @param mtu MTU value to be set
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_vlan_tx_set_mtu_msg(uint32_t vlan_if_num, uint32_t mtu);

/**
 * @brief Send vlan set mac address message
 *
 * @param vlan_if_num NSS dynamic interface
 * @param addr MAC addr to be set
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_vlan_tx_set_mac_addr_msg(uint32_t vlan_if_num, uint8_t *addr);

/**
 * @brief Send vlan attach VSI message
 *
 * @param vlan_if_num NSS dynamic interface
 * @param vsi PPE VSI to attach
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_vlan_tx_vsi_attach_msg(uint32_t vlan_if_num, uint32_t vsi);

/**
 * @brief Send vlan detach VSI message
 *
 * @param vlan_if_num NSS dynamic interface
 * @param vsi PPE VSI to detach
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_vlan_tx_vsi_detach_msg(uint32_t vlan_if_num, uint32_t vsi);

/**
 * @brief Send vlan add tag message
 *
 * @param vlan_if_num NSS dynamic interface
 * @param vlan_tag vlan tag info
 * @param next_hop parent interface
 * @param physical_dev physical port which to add vlan tag
 *
 * @return nss_tx_status_t Tx status
 */
nss_tx_status_t nss_vlan_tx_add_tag_msg(uint32_t vlan_if_num, uint32_t vlan_tag, uint32_t next_hop, uint32_t physical_dev);

/**
 * @brief Initialize vlan
 *
 * @return None
 */
void nss_vlan_register_handler(void);

#endif /* __NSS_VLAN_H */
