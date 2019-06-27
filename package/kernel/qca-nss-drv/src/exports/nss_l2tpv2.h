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
 * nss_l2tpv2.h
 *	NSS TO HLOS interface definitions.
 */
#ifndef _NSS_L2TP_V2_H_
#define _NSS_L2TP_V2_H_

/*
 * Maximum no of l2tpv2 sessions supported
 */
#define NSS_MAX_L2TPV2_DYNAMIC_INTERFACES 4

/**
 *  request/response types
 */
enum nss_l2tpv2_metadata_types {
	NSS_L2TPV2_MSG_SESSION_CREATE,		/**< session create message */
	NSS_L2TPV2_MSG_SESSION_DESTROY,		/**< session delete message */
	NSS_L2TPV2_MSG_SYNC_STATS,		/**< session stats sync message */
	NSS_L2TPV2_MSG_MAX
};

/**
 * l2tpv2 configuration create message structure
 */
struct nss_l2tpv2_session_create_msg {
	uint16_t local_tunnel_id;	/**< local tunnel id */
	uint16_t local_session_id;	/**< local session id */
	uint16_t peer_tunnel_id;	/**< peer tunnel id */
	uint16_t peer_session_id;	/**< peer session id */

	uint32_t sip;		/**< local tunnel end point */
	uint32_t dip;		/**< remote tunnel end point */
	uint32_t reorder_timeout;	/**< reorder timeout configured */

	uint16_t sport;		/**< local port */
	uint16_t dport;		/**< remote port */

	uint8_t recv_seq;	/**< sequnce number configured ? */
	uint8_t oip_ttl;	/**< max ttl value */
	uint8_t udp_csum;	/**< udp checksum */
	uint8_t reserved;	/**< reserved field */
};

/**
 * l2tpv2 configuration delete message structure
 */
struct nss_l2tpv2_session_destroy_msg {
	uint16_t local_tunnel_id;	/**< local tunnel id */
	uint16_t local_session_id;	/**< local session id */
};

/**
 * l2tpv2 statistics sync message structure.
 */
struct nss_l2tpv2_sync_session_stats_msg {
	struct nss_cmn_node_stats node_stats;	/**< common node stats */
	uint32_t tx_errors;	/**< tx errors */
	uint32_t rx_seq_discards;	/**< rx discarded pkts due to seq number check */
	uint32_t rx_oos_packets;	/**< rx out of order packets */
	uint32_t rx_errors;		/**< rx errors */
	uint32_t tx_dropped;		/**< tx dropped */
	struct {
		uint32_t rx_ppp_lcp_pkts;	/**< PPP LCP packets received */
		uint32_t rx_exception_data_pkts;/**< Data packets exceptioned to host */
		uint32_t encap_pbuf_alloc_fail; /**< Buffer alloc failure during encap */
		uint32_t decap_pbuf_alloc_fail; /**< Buffer alloc failure during decap */
	} debug_stats;
};

/**
 * Message structure to send/receive l2tpv2 messages
 */
struct nss_l2tpv2_msg {
	struct nss_cmn_msg cm;	/**< Message Header */
	union {
		struct nss_l2tpv2_session_create_msg session_create_msg; /**< session create message */
		struct nss_l2tpv2_session_destroy_msg session_destroy_msg; /**< session delete message */
		struct nss_l2tpv2_sync_session_stats_msg stats;		/**< session stats message */
	} msg;
};

/**
 * @brief Callback to receive l2tpv2  messages
 *
 * @return void
 */
typedef void (*nss_l2tpv2_msg_callback_t)(void *app_data, struct nss_l2tpv2_msg *msg);

/**
 * @brief Send l2tpv2 messages
 *
 * @param nss_ctx NSS context
 * @param msg NSS l2tp tunnel message
 *
 * @return nss_tx_status_t Tx status
 */
extern nss_tx_status_t nss_l2tpv2_tx(struct nss_ctx_instance *nss_ctx, struct nss_l2tpv2_msg *msg);

/**
 * @brief Get the l2tpv2 context used in the nss_l2tpv2_tx
 *
 * @return struct nss_ctx_instance *NSS context
 */
extern struct nss_ctx_instance *nss_l2tpv2_get_context(void);

/**
 * @brief Callback when l2tpv2 tunnel data is received
 *
 * @param netdevice of l2tpv2 session
 * @param skb Pointer to data buffer
 * @param napi pointer
 *
 * @return void
 */
typedef void (*nss_l2tpv2_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief Register to send/receive l2tpv2 tunnel messages to NSS
 *
 * @param if_num NSS interface number
 * @param l2tpv2_callback Callback for l2tp tunnel data
 * @param msg_callback Callback for l2tp tunnel messages
 * @param netdev netdevice associated with the l2tp tunnel
 * @param features denotes the skb types supported by this interface
 *
 * @return nss_ctx_instance* NSS context
 */
extern struct nss_ctx_instance *nss_register_l2tpv2_if(uint32_t if_num, nss_l2tpv2_callback_t l2tpv2_callback,
					nss_l2tpv2_msg_callback_t msg_callback, struct net_device *netdev, uint32_t features);

/**
 * @brief Unregister l2tpv2 tunnel interface with NSS
 *
 * @param if_num NSS interface number
 *
 * @return void
 */
extern void nss_unregister_l2tpv2_if(uint32_t if_num);

/**
 * @brief Initialize l2tpv2 msg
 *
 * @param nss_l2tpv2_msg
 * @param if_num Interface number
 * @param type Message type
 * @param len Message length
 * @param cb message callback
 * @param app_data
 *
 * @return None
 */
extern void nss_l2tpv2_msg_init(struct nss_l2tpv2_msg *ncm, uint16_t if_num, uint32_t type,  uint32_t len, void *cb, void *app_data);

/**
 * @brief register l2tpv2 nss debug stats handler
 *
 * @return None
 */
extern void nss_l2tpv2_register_handler(void);

/**
 * @brief get l2tpv2 nss session debug stats. stats_mem should be large enought to hold all stats.
 *
 * @param memory address to be copied to
 *
 * @return None
 */
extern void nss_l2tpv2_session_debug_stats_get(void  *stats_mem);

#endif /* _NSS_L2TP_V2_H_ */
