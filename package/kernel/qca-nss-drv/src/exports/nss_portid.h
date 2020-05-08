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

/**
 * nss_portid.h
 *	NSS TO HLOS interface definitions.
 */

#ifndef __NSS_PORTID_H
#define __NSS_PORTID_H

/*
 * Max switch port of S17c is 7, which is the maxium in the QCA switch family, if
 * there is a new switch that has more ports than S17c, this needs to be updated
 */
#define NSS_PORTID_MAX_SWITCH_PORT 7

/**
 * nss portid messages
 */

/**
 * @brief portid request/response types
 */
enum nss_portid_msg_types {
	NSS_PORTID_CONFIGURE_MSG,	/**< PORTID configuration msg */
	NSS_PORTID_UNCONFIGURE_MSG,	/**< PORTID unconfiguration msg */
	NSS_PORTID_STATS_SYNC_MSG,	/**< PORTID stats sync msg */
	NSS_PORTID_MAX_MSG_TYPE
};

/**
 * @brief portid configuration message
 */
struct nss_portid_configure_msg {
	uint32_t port_if_num;	/**< if num corrosponding to the portid port device */
	uint8_t port_id;	/**< Mapped switch port */
	uint8_t gmac_id;	/**< Mapped gmac interface id */
	uint8_t reserved[2];	/**< Reserved */
};

/**
 * @brief portid uncofigure message
 */
struct nss_portid_unconfigure_msg {
	uint32_t port_if_num;	/**< if num corrosponding to the portid port device */
	uint8_t port_id;	/**< Mapped switch port */
	uint8_t reserved[3];	/**< Reserved */
};

/**
 * @brief portid statistics sync message
 */
struct nss_portid_stats_sync_msg {
	struct nss_cmn_node_stats node_stats;	/**< port node/interface stats sync */
	uint32_t rx_invalid_header;		/**< RX with invalid header */
	uint8_t port_id;			/**< Mapped switch port */
	uint8_t reserved[3];			/**< Reserved */
};

/**
 * @brief Message structure to send/receive portid messages.
 */
struct nss_portid_msg {
	struct nss_cmn_msg cm;					/**< Message Header */
	union {
		struct nss_portid_configure_msg configure;	/**< msg: configure portid */
		struct nss_portid_unconfigure_msg unconfigure;	/**< msg: unconfigure portid */
		struct nss_portid_stats_sync_msg stats_sync;	/**< msg: portid stats sync */
	} msg;
};

/**
 * @brief Callback to receive portid messages
 *
 * @param app_data Application context of the message
 * @param msg Message data
 *
 * @return void
 */
typedef void (*nss_portid_msg_callback_t)(void *app_data, struct nss_portid_msg *npm);

/**
 * @brief Callback to receive portid port interface data
 *
 * @param app_data Application context of the message
 * @param skb Pointer to data buffer
 * @param napi NAPI structure pointer
 *
 * @return void
 */
typedef void (*nss_portid_buf_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief API to get stats from a port interface
 *
 * @param if_num portid interface number
 * @param stats container for the statistic counters
 *
 * @return true or false
 */
bool nss_portid_get_stats(uint32_t if_num, struct rtnl_link_stats64 *stats);

/**
 * @brief Initialize portid msg
 *
 * @param npm portid msg structure
 * @param if_num portid interface number
 * @param type msg type
 * @param len msg length
 * @param cb msg return callback
 * @param app_data application context
 *
 * @return void
 */
extern void nss_portid_msg_init(struct nss_portid_msg *npm, uint16_t if_num, uint32_t type, uint32_t len,
							nss_portid_msg_callback_t cb, void *app_data);

/**
 * @brief Transmit a data packet to NSS portid interface
 *
 * @param nss_ctx NSS context
 * @param sk_buff SKB to be transmit
 * @param if_num target interface number
 *
 * @return Tx status
 */
extern nss_tx_status_t nss_portid_if_tx_data(struct nss_ctx_instance *nss_ctx, struct sk_buff *os_buf, uint32_t if_num);

/**
 * @brief Send portid messages to NSS
 *
 * @param nss_ctx NSS context
 * @param msg nss_portid_msg structure
 *
 * @return Tx status
 */
extern nss_tx_status_t nss_portid_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_portid_msg *msg);

/**
 * @brief Send portid messages to NSS and wait for response
 *
 * @param nss_ctx NSS context
 * @param msg nss_portid_msg structure
 *
 * @return Tx status
 */
extern nss_tx_status_t nss_portid_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_portid_msg *msg);

/**
 * @brief Register Port interface to NSS
 *
 * @param if_num NSS interface number
 * @param port_id physicall port id of this interface
 * @param netdev associated netdevice
 * @param buf_cb Callback for Port interface data
 *
 * @return nss_ctx_instance* NSS context
 */
extern struct nss_ctx_instance *nss_portid_register_port_if(uint32_t if_num, uint32_t port_id, struct net_device *ndev, nss_portid_buf_callback_t buf_cb);

/**
 * @brief Register Port interface to NSS
 *
 * @param if_num NSS interface number
 *
 * @return true or false
 */
extern bool nss_portid_unregister_port_if(uint32_t if_num);

/**
 * @brief Send port interface configure message to NSS
 *
 * @param nss_ctx NSS context
 * @param port_if_num port node interface number
 * @param port_id mapped switch port id
 * @param gmac_id mapped gmac interface id
 *
 * @return Tx status
 */
extern nss_tx_status_t nss_portid_tx_configure_port_if_msg(struct nss_ctx_instance *nss_ctx, uint32_t port_if_num, uint8_t port_id, uint8_t gmac_id);

/**
 * @brief Send port interface unconfigure message to NSS
 *
 * @param nss_ctx NSS context
 * @param port_if_num port node interface number
 * @param port_id mapped switch port id
 *
 * @return Tx status
 */
extern nss_tx_status_t nss_portid_tx_unconfigure_port_if_msg(struct nss_ctx_instance *nss_ctx, uint32_t port_if_num, uint8_t port_id);

#endif /* __NSS_PORTID_H */
