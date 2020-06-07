/*
 **************************************************************************
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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
 * nss_pptp.h
 *	NSS TO HLOS interface definitions.
 */
#ifndef _NSS_PPTP_H_
#define _NSS_PPTP_H_

/*
 * Maximum no of pptp sessions supported
 */
#define NSS_MAX_PPTP_DYNAMIC_INTERFACES 4

/**
 *  request/response types
 */
enum nss_pptp_metadata_types {
	NSS_PPTP_MSG_SESSION_CONFIGURE,		/**< session create message */
	NSS_PPTP_MSG_SESSION_DECONFIGURE,		/**< session delete message */
	NSS_PPTP_MSG_SYNC_STATS,		/**< session stats sync message */
	NSS_PPTP_MSG_MAX
};

/**
 * PPTP encap/decap packet exception events.
 *
 */
enum nss_pptp_exception_events {
	PPTP_EXCEPTION_EVENT_ENCAP_HEADROOM_ERR,
	PPTP_EXCEPTION_EVENT_ENCAP_SMALL_SIZE,
	PPTP_EXCEPTION_EVENT_ENCAP_PNODE_ENQUEUE_FAIL,
	PPTP_EXCEPTION_EVENT_DECAP_NO_SEQ_NOR_ACK,
	PPTP_EXCEPTION_EVENT_DECAP_INVAL_GRE_FLAGS,
	PPTP_EXCEPTION_EVENT_DECAP_INVAL_GRE_PROTO,
	PPTP_EXCEPTION_EVENT_DECAP_WRONG_SEQ,
	PPTP_EXCEPTION_EVENT_DECAP_INVAL_PPP_HDR,
	PPTP_EXCEPTION_EVENT_DECAP_PPP_LCP,
	PPTP_EXCEPTION_EVENT_DECAP_UNSUPPORTED_PPP_PROTO,
	PPTP_EXCEPTION_EVENT_DECAP_PNODE_ENQUEUE_FAIL,
	PPTP_EXCEPTION_EVENT_MAX,
};

/**
 * PPTP session configuration message structure
 */
struct nss_pptp_session_configure_msg {
	uint16_t src_call_id;	/**< local call id */
	uint16_t dst_call_id;	/**< peer call id */
	uint32_t sip;		/**< local tunnel end point */
	uint32_t dip;		/**< remote tunnel end point */
};

/**
 * PPTP session deconfiguration message structure
 */
struct nss_pptp_session_deconfigure_msg {
	uint16_t src_call_id;	/**< local call id */
};

/**
 * pptp statistics sync message structure.
 */
struct nss_pptp_sync_session_stats_msg {
	struct nss_cmn_node_stats encap_stats;	/**< common node stats for encap direction */
	struct nss_cmn_node_stats decap_stats;	/**< common node stats for decap direction */
	uint32_t exception_events[PPTP_EXCEPTION_EVENT_MAX];	/**< Expception events */
};

/**
 * Message structure to send/receive pptp messages
 */
struct nss_pptp_msg {
	struct nss_cmn_msg cm;	/**< Message Header */
	union {
		struct nss_pptp_session_configure_msg session_configure_msg; /**< session configure message */
		struct nss_pptp_session_deconfigure_msg session_deconfigure_msg; /**< session deconfigure message */
		struct nss_pptp_sync_session_stats_msg stats;		/**< session stats message */
	} msg;
};

/**
 * @brief Callback to receive pptp  messages
 *
 * @return void
 */
typedef void (*nss_pptp_msg_callback_t)(void *app_data, struct nss_pptp_msg *msg);

/**
 * @brief Send pptp messages synchronously
 *
 * @param nss_ctx NSS context
 * @param msg NSS pptp tunnel message
 *
 * @return nss_tx_status_t Tx status
 */
extern nss_tx_status_t nss_pptp_tx_msg_sync(struct nss_ctx_instance *nss_ctx,
					    struct nss_pptp_msg *msg);
/**
 * @brief Send data packet to FW
 *
 * @param nss_ctx NSS context
 * @param if_num NSS dynamic interface number
 * @param skb packet buffer
 *
 * @return Tx status
 */
extern nss_tx_status_t nss_pptp_tx_buf(struct nss_ctx_instance *nss_ctx, uint32_t if_num, struct sk_buff *skb);

/**
 * @brief Get the pptp context used in the nss_pptp_tx
 *
 * @return struct nss_ctx_instance *NSS context
 */
extern struct nss_ctx_instance *nss_pptp_get_context(void);

/**
 * @brief Callback when pptp tunnel data is received
 *
 * @param netdevice of pptp session
 * @param skb Pointer to data buffer
 * @param napi pointer
 *
 * @return void
 */
typedef void (*nss_pptp_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief Register to send/receive pptp tunnel messages to NSS
 *
 * @param if_num NSS interface number
 * @param pptp_callback Callback for pptp tunnel data
 * @param msg_callback Callback for pptp tunnel messages
 * @param netdev netdevice associated with the pptp tunnel
 * @param features denotes the skb types supported by this interface
 *
 * @return nss_ctx_instance* NSS context
 */
extern struct nss_ctx_instance *nss_register_pptp_if(uint32_t if_num, nss_pptp_callback_t pptp_data_callback,
					nss_pptp_msg_callback_t notification_callback, struct net_device *netdev, uint32_t features, void *app_ctx);

/**
 * @brief Unregister pptp tunnel interface with NSS
 *
 * @param if_num NSS interface number
 *
 * @return void
 */
extern void nss_unregister_pptp_if(uint32_t if_num);

/**
 * @brief Initialize pptp msg
 *
 * @param nss_pptp_msg PPTP session info i.e Configure/Deconfigure
 * @param if_num Interface number
 * @param type Message type
 * @param len Message length
 * @param cb message callback
 * @param app_data
 *
 * @return None
 */
extern void nss_pptp_msg_init(struct nss_pptp_msg *ncm, uint16_t if_num, uint32_t type,  uint32_t len, void *cb, void *app_data);

/**
 * @brief register pptp nss debug stats handler
 *
 * @return None
 */
extern void nss_pptp_register_handler(void);

/**
 * @brief get pptp nss session debug stats. stats_mem should be large enought to hold all stats.
 *
 * @param memory address to be copied to
 *
 * @return None
 */
extern void nss_pptp_session_debug_stats_get(void *stats_mem);

#endif /* _NSS_PPTP_H_ */
