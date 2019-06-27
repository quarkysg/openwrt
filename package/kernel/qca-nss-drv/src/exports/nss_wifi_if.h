/*
 **************************************************************************
 * Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
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
 * nss_wifi_if
 *	Virtual interface message Structure and APIs
 */

#ifndef __NSS_WIFI_IF_H
#define __NSS_WIFI_IF_H

#include "nss_if.h"

/**
 * @brief Request/Response types
 */
enum nss_wifi_if_msg_types {
	NSS_WIFI_IF_OPEN = NSS_IF_OPEN,
	NSS_WIFI_IF_CLOSE = NSS_IF_CLOSE,
	NSS_WIFI_IF_LINK_STATE_NOTIFY = NSS_IF_LINK_STATE_NOTIFY,
	NSS_WIFI_IF_MTU_CHANGE = NSS_IF_MTU_CHANGE,
	NSS_WIFI_IF_MAC_ADDR_SET = NSS_IF_MAC_ADDR_SET,
	NSS_WIFI_IF_STATS_SYNC = NSS_IF_STATS,
	NSS_WIFI_IF_ISHAPER_ASSIGN = NSS_IF_ISHAPER_ASSIGN,
	NSS_WIFI_IF_BSHAPER_ASSIGN = NSS_IF_BSHAPER_ASSIGN,
	NSS_WIFI_IF_ISHAPER_UNASSIGN = NSS_IF_ISHAPER_UNASSIGN,
	NSS_WIFI_IF_BSHAPER_UNASSIGN = NSS_IF_BSHAPER_UNASSIGN,
	NSS_WIFI_IF_ISHAPER_CONFIG = NSS_IF_ISHAPER_CONFIG,
	NSS_WIFI_IF_BSHAPER_CONFIG = NSS_IF_BSHAPER_CONFIG,
	NSS_WIFI_IF_VSI_ASSIGN = NSS_IF_VSI_ASSIGN,
	NSS_WIFI_IF_VSI_UNASSIGN = NSS_IF_VSI_UNASSIGN,
	NSS_WIFI_IF_TX_CREATE_MSG = NSS_IF_MAX_MSG_TYPES + 1,
	NSS_WIFI_IF_TX_DESTROY_MSG,
	NSS_WIFI_IF_STATS_SYNC_MSG,
	NSS_WIFI_IF_MAX_MSG_TYPES,
};

/**
 * @brief Error types
 */
enum nss_wifi_if_error_types {
	NSS_WIFI_IF_SUCCESS,			/**< Success */
	NSS_WIFI_IF_CORE_FAILURE,		/**< NSS Core failure */
	NSS_WIFI_IF_ALLOC_FAILURE,		/**< Memory allocation failure */
	NSS_WIFI_IF_DYNAMIC_IF_FAILURE,		/**< Dynamic interface failure */
	NSS_WIFI_IF_MSG_TX_FAILURE,		/**< Message transmission failure */
	NSS_WIFI_IF_REG_FAILURE,		/**< Registration failure */
	NSS_WIFI_IF_CORE_NOT_INITIALIZED,	/**< NSS core is not initialized */
};

/**
 * The NSS wifi interface creation structure.
 */
struct nss_wifi_if_create_msg {
	uint32_t flags;			/**< Interface flags */
	uint8_t mac_addr[ETH_ALEN];	/**< MAC address */
};

/**
 * The NSS wifi interface destruction structure.
 */
struct nss_wifi_if_destroy_msg {
	int32_t reserved;		/**< place holder */
};

/**
 * Structure which contains stats received from NSS.
 */
struct nss_wifi_if_stats {
	struct nss_if_stats node_stats;		/**< common stats */
	uint32_t tx_enqueue_failed;		/**< enqueue failed for TX */
	uint32_t shaper_enqueue_failed;		/**< enqueue to shaper node failed */
};

/**
 * Message structure to send/receive wifi interface commands
 */
struct nss_wifi_if_msg {
	struct nss_cmn_msg cm;				/**< Message Header */
	union {
		union nss_if_msgs if_msgs;		/**< NSS Interface messages */
		struct nss_wifi_if_create_msg create;	/**< Message: create wifi if rule */
		struct nss_wifi_if_destroy_msg destroy;	/**< Message: destroy wifi if rule */
		struct nss_wifi_if_stats stats;		/**< stats */
	} msg;
};

/*
 * Private data structure for wifi_if interface
 */
struct nss_wifi_if_pvt {
	struct semaphore sem;		/**< Semaphore for a given wifi if_num*/
	struct completion complete;	/**< Completion for a given wifi if_num */
	int response;			/**< Response received on a wifi if_num */
	int sem_init_done;		/**< Denotes if the semaphore is initialized or not */
};

typedef void (*nss_wifi_if_data_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);
typedef void (*nss_wifi_if_msg_callback_t)(void *app_data, struct nss_cmn_msg *msg);

/**
 * Context for WLAN to NSS communication
 */
struct nss_wifi_if_handle {
	struct nss_ctx_instance *nss_ctx;	/**< NSS context */
	int32_t if_num;				/**< Interface number */
	struct nss_wifi_if_pvt *pvt;		/**< Private data structure */
	struct nss_wifi_if_stats stats;		/**< stats corresponding to this handle */
	nss_wifi_if_msg_callback_t cb;		/**< Callback registered by other modules */
	void *app_data;				/**< app_data to be passed to that callback */
};

/**
 * @brief Send message to wifi interface
 *
 * @param nss_ctx NSS context (provided during registration)
 * @param nwim Wifi interface message
 *
 * @return command Tx status
 */
extern nss_tx_status_t nss_wifi_if_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_wifi_if_msg *nwim);

/**
 * @brief Register wifi Interface with NSS driver
 *
 * @param handle wifi context(provided during wifi interface allocation)
 * @param rx_callback Callback handler for wifi data packets
 * @param netdev netdevice structure of VAP
 *
 * @return void
 */
extern void nss_wifi_if_register(struct nss_wifi_if_handle *handle,
				nss_wifi_if_data_callback_t rx_callback,
				struct net_device *netdev);

/**
 * @brief Unregister wifi interface from NSS driver
 *
 * @param handle wifi interface context (provided during wifi if allocation)
 *
 * @return void
 */
extern void nss_wifi_if_unregister(struct nss_wifi_if_handle *handle);

/**
 * @brief Create a wifi interface
 *
 * @param netdev net device associated with Wifi
 *
 * @return pointer to wifi handle
 */
extern struct nss_wifi_if_handle *nss_wifi_if_create(struct net_device *netdev);

/**
 * @brief Destroy the wifi interface associated with the if_num
 *
 * @param if_num Interface number (provided during dynamic_interface allocation)
 *
 * @return command Tx status
 */
extern nss_tx_status_t nss_wifi_if_destroy(struct nss_wifi_if_handle *handle);

/**
 * @brief Send the packet/buffer to NSS
 *
 * @param ctx Context associated with the interface.
 * @param skb Packet to be sent to the NSS.
 *
 * @return command Tx status
 */
extern nss_tx_status_t nss_wifi_if_tx_buf(struct nss_wifi_if_handle *handle, struct sk_buff *skb);

/**
 * @brief Copy wifi_if stats for display
 *
 * @param if_num Interface number for which the stats should be populated
 * @param index Denotes the specific stats
 * @param line Buffer into which the stats should be copied.
 *
 * @return Number of bytes copied
 */
int32_t nss_wifi_if_copy_stats(int32_t if_num, int index, char *line);

#endif /* __NSS_WIFI_IF_H */
