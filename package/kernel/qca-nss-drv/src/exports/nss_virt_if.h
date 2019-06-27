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
 * nss_virt_if
 *	Virtual interface message Structure and APIs
 */

#ifndef __NSS_VIRT_IF_H
#define __NSS_VIRT_IF_H

#include "nss_if.h"

/*
 * Virtual IF/Redirect
 */

/**
 * @brief Request/Response types
 */
enum nss_virt_if_msg_types {
	NSS_VIRT_IF_OPEN = NSS_IF_OPEN,
	NSS_VIRT_IF_CLOSE = NSS_IF_CLOSE,
	NSS_VIRT_IF_LINK_STATE_NOTIFY = NSS_IF_LINK_STATE_NOTIFY,
	NSS_VIRT_IF_MTU_CHANGE = NSS_IF_MTU_CHANGE,
	NSS_VIRT_IF_MAC_ADDR_SET = NSS_IF_MAC_ADDR_SET,
	NSS_VIRT_IF_STATS_SYNC = NSS_IF_STATS,
	NSS_VIRT_IF_ISHAPER_ASSIGN = NSS_IF_ISHAPER_ASSIGN,
	NSS_VIRT_IF_BSHAPER_ASSIGN = NSS_IF_BSHAPER_ASSIGN,
	NSS_VIRT_IF_ISHAPER_UNASSIGN = NSS_IF_ISHAPER_UNASSIGN,
	NSS_VIRT_IF_BSHAPER_UNASSIGN = NSS_IF_BSHAPER_UNASSIGN,
	NSS_VIRT_IF_ISHAPER_CONFIG = NSS_IF_ISHAPER_CONFIG,
	NSS_VIRT_IF_BSHAPER_CONFIG = NSS_IF_BSHAPER_CONFIG,
	NSS_VIRT_IF_VSI_ASSIGN = NSS_IF_VSI_ASSIGN,
	NSS_VIRT_IF_VSI_UNASSIGN = NSS_IF_VSI_UNASSIGN,
	NSS_VIRT_IF_TX_CREATE_MSG = NSS_IF_MAX_MSG_TYPES + 1,
	NSS_VIRT_IF_TX_DESTROY_MSG,
	NSS_VIRT_IF_STATS_SYNC_MSG,
	NSS_VIRT_IF_MAX_MSG_TYPES,
};

/**
 * virt_if error types
 */
enum nss_virt_if_error_types {
	NSS_VIRT_IF_SUCCESS,			/*< Success */
	NSS_VIRT_IF_CORE_FAILURE,		/*< nss core failure */
	NSS_VIRT_IF_ALLOC_FAILURE,		/*< Memory allocation failure */
	NSS_VIRT_IF_DYNAMIC_IF_FAILURE,		/*< Dynamic interface failure */
	NSS_VIRT_IF_MSG_TX_FAILURE,		/*< Message transmission failure */
	NSS_VIRT_IF_REG_FAILURE,		/*< Registration failure */
	NSS_VIRT_IF_CORE_NOT_INITIALIZED,	/*< NSS core not intialized */
};

/**
 * Structure which contains stats received from NSS.
 */
struct nss_virt_if_stats {
	struct nss_if_stats node_stats;	/**< common stats */
	uint32_t tx_enqueue_failed;	/**< tx enqueue failures in the FW */
	uint32_t shaper_enqueue_failed;	/**< shaper enqueue failures in the FW */
};

/**
 * The NSS virtual interface creation structure.
 */
struct nss_virt_if_create_msg {
	uint32_t flags;			/**< Interface flags */
	uint8_t mac_addr[ETH_ALEN];	/**< MAC address */
};

/**
 * The NSS virtual interface destruction structure.
 */
struct nss_virt_if_destroy_msg {
	int32_t reserved;		/**< place holder */
};

/**
 * Message structure to send/receive virtual interface commands
 */
struct nss_virt_if_msg {
	struct nss_cmn_msg cm;
				/**< Message Header */
	union {
		union nss_if_msgs if_msgs;
		struct nss_virt_if_create_msg if_create;
				/**< Message: create virt if rule */
		struct nss_virt_if_destroy_msg if_destroy;
				/**< Message: destroy virt if rule */
		struct nss_virt_if_stats stats;
				/**< Message: stats */
	} msg;
};

/*
 * Private data structure for virt_if interface
 */
struct nss_virt_if_pvt {
	struct semaphore sem;
	struct completion complete;
	int response;
	int sem_init_done;
};

typedef void (*nss_virt_if_data_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);
typedef void (*nss_virt_if_msg_callback_t)(void *app_data, struct nss_cmn_msg *msg);

/**
 * Context for WLAN to NSS communication
 */
struct nss_virt_if_handle {
	struct nss_ctx_instance *nss_ctx;	/*< NSS context */
	int32_t if_num;				/*< interface number */
	struct net_device *ndev;		/*< Associated netdevice */
	struct nss_virt_if_pvt *pvt;		/*< Private data structure */
	struct nss_virt_if_stats stats;		/*< virt_if stats */
	atomic_t refcnt;			/*< Reference count */
	nss_virt_if_msg_callback_t cb;		/*< callback */
	void *app_data;				/*< app_data to be passed to callback */
};

/**
 * @brief Create a virtual interface, asynchronously
 *
 * @param netdev net device associated with client
 * @param cb callback to be invoked when the response from the FW is received
 * @param app_data app_data to be passed to the callback.
 *
 * @return int command tx status
 */
extern int nss_virt_if_create(struct net_device *netdev, nss_virt_if_msg_callback_t cb, void *app_data);

/**
 * @brief Create a virtual interface, synchronously.
 *
 * @param netdev net device associated with WiFi
 *
 * @return Pointer to nss_virt_if_handle struct
 */
extern struct nss_virt_if_handle *nss_virt_if_create_sync(struct net_device *netdev);

/**
 * @brief Destroy the virtual interface associated with the if_num, asynchronously.
 *
 * @param handle virtual interface handle (provided during dynamic_interface allocation)
 * @param cb callback to be invoked when the response from the FW is received
 * @param app_data app_data to be passed to the callback.
 *
 * @return command Tx status
 */
extern nss_tx_status_t nss_virt_if_destroy(struct nss_virt_if_handle *handle, nss_virt_if_msg_callback_t cb, void *app_data);

/**
 * @brief Destroy the virtual interface associated with the if_num, synchronously.
 *
 * @param handle virtual interface handle (provided during dynamic_interface allocation)
 *
 * @return command Tx status
 */
extern nss_tx_status_t nss_virt_if_destroy_sync(struct nss_virt_if_handle *handle);

/**
 * @brief Send message to virtual interface
 *
 * @param nss_ctx NSS context (provided during registration)
 * @param nvim Virtual interface message
 *
 * @return command Tx status
 */
extern nss_tx_status_t nss_virt_if_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_virt_if_msg *nvim);

/**
 * @brief Forward virtual interface packets to NSS
 *
 * @param handle virtual interface handle (provided during registration)
 * @param skb HLOS data buffer (sk_buff in Linux)
 *
 * @return command Tx status
 */
extern nss_tx_status_t nss_virt_if_tx_buf(struct nss_virt_if_handle *handle,
						struct sk_buff *skb);

/**
 * @brief Register Virtual Interface with NSS driver
 *
 * @param handle virtual interface handle(provided during dynamic_interface allocation)
 * @param data_callback Callback handler for virtual data packets
 * @param netdev netdevice structure associated with WiFi
 *
 * @return command Tx status
 */
extern void nss_virt_if_register(struct nss_virt_if_handle *handle,
					nss_virt_if_data_callback_t data_callback,
					struct net_device *netdev);

/**
 * @brief Unregister virtual interface from NSS driver
 *
 * @param handle virtual interface handle
 *
 * @return void
 */
extern void nss_virt_if_unregister(struct nss_virt_if_handle *handle);

/**
 * @brief Get stats for virtual interface from NSS driver
 *
 * @param if_num Interface number (provided during dynamic_interface allocation)
 * @param i index of stats
 * @param line buffer into which the stats will be copied.
 *
 * @return int32_t Returns 0 if if_num is not in range or the number of bytes copied.
 */
extern int32_t nss_virt_if_copy_stats(int32_t if_num, int i, char *line);

/**
 * @brief Returns the virtual interface number associated with the handle.
 *
 * @param handle virtual interface handle(provided during dynamic_interface allocation)
 *
 * @return if_num virtual interface number.
 */
extern int32_t nss_virt_if_get_interface_num(struct nss_virt_if_handle *handle);

#endif /* __NSS_VIRT_IF_H */
