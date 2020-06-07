/*
 **************************************************************************
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
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
 * nss_gre_tunnel.h
 *	NSS TO HLOS interface definitions.
 */

#ifndef __NSS_GRE_TUNNEL_H
#define __NSS_GRE_TUNNEL_H

/**
 * Maximum no of gre_tunnel sessions supported
 */
#define NSS_MAX_GRE_TUNNEL_SESSIONS 16

/**
 * GRE Tunnel Rule Messages
 */
enum nss_gre_tunnel_message_types {
	NSS_GRE_TUNNEL_MSG_CONFIGURE,			/**< Tunnel Configure Message */
	NSS_GRE_TUNNEL_MSG_SESSION_DESTROY,		/**< Tunnel Session Destroy Message */
	NSS_GRE_TUNNEL_MSG_STATS,			/**< Tunnel Statistics Message */
	NSS_GRE_TUNNEL_MSG_MAX,				/**< Tunnel Max Messages */
};

/**
 * GRE Tunnel Encrypt Types
 */
enum nss_gre_tunnel_encrypt_types {
	NSS_GRE_TUNNEL_ENCRYPT_NONE,			/**< Tunnel set to use no encryption */
	NSS_GRE_TUNNEL_ENCRYPT_AES128_CBC,		/**< Tunnel set to use AES128 */
	NSS_GRE_TUNNEL_ENCRYPT_AES256_CBC,		/**< Tunnel set to use AES256 */
	NSS_GRE_TUNNEL_ENCRYPT_MAX,			/**< Tunnel Max Encryption */
};

/**
 * GRE Tunnel Mode Types
 */
enum nss_gre_tunnel_mode_types {
	NSS_GRE_TUNNEL_MODE_GRE,			/**< Tunnel mode is IP, then GRE Header */
	NSS_GRE_TUNNEL_MODE_GRE_UDP,			/**< Tunnel mode is IP, UDP, then GRE Header */
	NSS_GRE_TUNNEL_MODE_MAX,			/**< Tunnel Max support Modes */
};

/**
 * GRE Tunnel IP Types
 */
enum nss_gre_tunnel_ip_types {
	NSS_GRE_TUNNEL_IP_IPV4,				/**< Tunnel is set for IPv4 */
	NSS_GRE_TUNNEL_IP_IPV6,				/**< Tunnel is set for IPv6 */
	NSS_GRE_TUNNEL_IP_MAX,				/**< Tunnel Max IP Types */
};

/**
 * GRE Tunnel errors
 */
enum nss_gre_tunnel_error_types {
	NSS_GRE_TUNNEL_ERR_UNKNOWN_MSG = 1,			/**< Unknown message */
	NSS_GRE_TUNNEL_ERR_IF_INVALID = 2,			/**< Invalid interface */
	NSS_GRE_TUNNEL_ERR_CPARAM_INVALID = 3,			/**< Invalid crypto parameter */
	NSS_GRE_TUNNEL_ERR_MODE_INVALID = 4,			/**< Invalid mode type */
	NSS_GRE_TUNNEL_ERR_ENCRYPT_INVALID = 5,			/**< Invalid encrypt type */
	NSS_GRE_TUNNEL_ERR_IP_INVALID = 6,			/**< Invalid ip */
	NSS_GRE_TUNNEL_ERR_ENCRYPT_IDX_INVALID = 7,		/**< Invalid encrypt idx */
	NSS_GRE_TUNNEL_ERR_NOMEM = 8,				/**< Failed memory allocation */
	NSS_GRE_TUNNEL_ERR_PROTO_TEB_INVALID = 9,		/**< Invalid GRE TEB protocol */
	NSS_GRE_TUNNEL_ERR_MAX,					/**< Maximum GRE TUNNEL error */
};

/**
 * Configure Message Structure
 */
struct nss_gre_tunnel_configure {
	uint8_t gre_mode;		/**< GRE or GRE plus UDP. */
	uint8_t ip_type;		/**< IPv4 or IPv6. */
	uint16_t encrypt_type;		/**< Encryption type. */
	uint32_t src_ip[4];		/**< Source IPv4 or IPv6 address. */
	uint32_t dest_ip[4];		/**< Destination IPv4 or IPv6 address. */
	uint16_t src_port;		/**< GRE plus UDP only for the source. */
	uint16_t dest_port;		/**< GRE plus UDP only for the destination. */
	uint32_t crypto_idx_encrypt;	/**< Crypto index for encryption. */
	uint32_t crypto_idx_decrypt;	/**< Crypto index for decryption. */
	uint32_t word0;			/**< Word0 header. */
	uint8_t iv_val[16];		/**< Initialization vector value. */
	uint8_t ttl;			/**< Time-to-live value of the IP header. */
	uint8_t mh_version;		/**< Meta header version */
};

/**
 * Statistic Message Structure
 */
struct nss_gre_tunnel_stats {
	struct nss_cmn_node_stats node_stats;	/**< Common node statistics. */
	uint32_t rx_malformed;			/**< Malformed packet was received. */
	uint32_t rx_invalid_prot;		/**< Invalid protocol was received. */
	uint32_t decap_queue_full;		/**< Decapsulation queue is full. */
	uint32_t rx_single_rec_dgram;		/**< Single fragment was received. */
	uint32_t rx_invalid_rec_dgram;		/**< Invalid fragment was received. */
	uint32_t buffer_alloc_fail;		/**< Buffer memory allocation failed. */
	uint32_t buffer_copy_fail;		/**< Buffer memory copy failed. */
	uint32_t outflow_queue_full;		/**< Outflow queue is full. */
	uint32_t rx_dropped_hroom;
				/**< Packets dropped because of insufficent headroom. */
	uint32_t rx_cbuf_alloc_fail;
				/**< Rx crypto buffer allocation failed. */
	uint32_t rx_cenqueue_fail;		/**< Rx enqueue-to-crypto failed. */
	uint32_t rx_decrypt_done;		/**< Rx decryption is complete. */
	uint32_t rx_forward_enqueue_fail;	/**< Rx forward enqueue failed. */
	uint32_t tx_cbuf_alloc_fail;
				/**< Rx crypto buffer allocation failed. */
	uint32_t tx_cenqueue_fail;		/**< Tx enqueue-to-crypto failed. */
	uint32_t rx_dropped_troom;
				/**< Packets dropped because of insufficent tailroom. */
	uint32_t tx_forward_enqueue_fail;	/**< Tx forward enqueue failed. */
	uint32_t tx_cipher_done;		/**< Tx cipher is complete. */
	uint32_t crypto_nosupp;
				/**< Error count for non-supported crypto packets. */
	uint32_t rx_dropped_mh_ver;		/**< Rx drop: bad meta header */
};

/**
 * Message Structure to Send/Receive GRE Tunnel Commands
 */
struct nss_gre_tunnel_msg {
	struct nss_cmn_msg cm;					/**< Message Header */
	union {
		struct nss_gre_tunnel_configure configure;	/**< msg: configure tunnel */
		struct nss_gre_tunnel_stats stats;		/**< msg: tunnel stats */
	} msg;
};

/**
 * @brief Callback to receive gre_tunnel messages
 *
 * @param app_data Application context of the message
 * @param msg NSS GRE Tunnel message
 *
 * @return void
 */
typedef void (*nss_gre_tunnel_msg_callback_t)(void *app_data, struct nss_gre_tunnel_msg *msg);

/**
 * @brief Callback when gre_tunnel session data is received
 *
 * @param netdev netdevice of gre_tunnel session
 * @param skb Pointer to data buffer
 * @param napi pointer
 *
 * @return void
 */
typedef void (*nss_gre_tunnel_data_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief Send gre_tunnel packet
 *
 * @param skb Pointer to data buffer
 * @param if_num GRE Tunnel i/f number
 * @param nss_ctx NSS context
 *
 * @return NSS TX Status
 */
extern nss_tx_status_t nss_gre_tunnel_tx_buf(struct sk_buff *skb, uint32_t if_num, struct nss_ctx_instance *nss_ctx);

/**
 * @brief Send gre_tunnel messages
 *
 * @param nss_ctx NSS Context
 * @param msg GRE Tunnel Message
 *
 * @return NSS TX Status
 */
extern nss_tx_status_t nss_gre_tunnel_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_gre_tunnel_msg *msg);

/**
 * @brief Send gre_tunnel messages synchronously
 *
 * @param nss_ctx NSS Context
 * @param msg GRE Tunnel Message
 *
 * @return NSS TX Status
 */
extern nss_tx_status_t nss_gre_tunnel_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_gre_tunnel_msg *msg);

/**
 * @brief Initalize GRE Tunnel message
 *
 * @param ngtm GRE Tunnel Message
 * @param if_num GRE Tunnel i/f number
 * @param type Message Type
 * @param len Message Length
 * @param cb Message Callback
 * @param app_data App_data
 *
 * @return void
 */
extern void nss_gre_tunnel_msg_init(struct nss_gre_tunnel_msg *ngtm, uint16_t if_num, uint32_t type, uint32_t len, void *cb, void *app_data);

/**
 * @brief Return the NSS CTX of which core
 *
 * @param void Void
 *
 * @return nss_ctx
 */
extern struct nss_ctx_instance *nss_gre_tunnel_get_ctx(void);

/**
 * @brief Register netdev to send/receive gre tunnel messages to NSS
 *
 * @param if_num NSS interface number
 * @param cb Callback for gre tunnel data
 * @param event Callback for gre tunnel messages
 * @param netdev netdev to register
 * @param features features for the netdev
 * @param app_ctx application context
 *
 * @return nss_ctx
 */
extern struct nss_ctx_instance *nss_gre_tunnel_register_if(uint32_t if_num,
						nss_gre_tunnel_data_callback_t cb,
						nss_gre_tunnel_msg_callback_t ev_cb,
						struct net_device *netdev,
						uint32_t features,
						void *app_ctx);

/**
 * @brief Unregister netdev
 *
 * @param if_num Interface Number
 *
 * @return void
 */
extern void nss_gre_tunnel_unregister_if(uint32_t if_num);
#endif /* __NSS_GRE_TUNNEL_H */
