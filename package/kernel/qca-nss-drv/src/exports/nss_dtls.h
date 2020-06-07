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
 * nss_dtls.h
 *	NSS TO HLOS interface definitions.
 */
#ifndef _NSS_DTLS_H_
#define _NSS_DTLS_H_

/**
 * Maximum no of dtls sessions supported
 */
#define NSS_MAX_DTLS_SESSIONS 8

/**
 *  DTLS request/response types
 */
enum nss_dtls_metadata_types {
	NSS_DTLS_MSG_SESSION_CONFIGURE,		/**< session configure message */
	NSS_DTLS_MSG_SESSION_DESTROY,		/**< session delete message */
	NSS_DTLS_MSG_SESSION_STATS,		/**< session stats sync message */
	NSS_DTLS_MSG_REKEY_ENCAP_CIPHER_UPDATE,	/**< Update pending encap cipher */
	NSS_DTLS_MSG_REKEY_ENCAP_CIPHER_SWITCH,	/**< Copy pending encap cipher to current */
	NSS_DTLS_MSG_REKEY_DECAP_CIPHER_UPDATE,	/**< Update pending decap cipher */
	NSS_DTLS_MSG_REKEY_DECAP_CIPHER_SWITCH,	/**< Copy pending decap cipher to current */
	NSS_DTLS_MSG_MAX
};

/**
 * DTLS error response types
 */
enum nss_dtls_error_response_types {
	NSS_DTLS_ERR_UNKNOWN_MSG = 1,		/**< Unknown message */
	NSS_DTLS_ERR_INVALID_APP_IF = 2,	/**< Invalid application
						     interface */
	NSS_DTLS_ERR_INVALID_CPARAM = 3,	/**< Invalid crypto parameter */
	NSS_DTLS_ERR_INVALID_VER = 4,		/**< Invalid DTLS version */
	NSS_DTLS_ERR_NOMEM = 5,			/**< Failed memory allocation */
	NSS_DTLS_ERR_MAX,			/**< Maximum DTLS error */
};

/**
 *  DTLS session statistics
 */
struct nss_dtls_session_stats {
	struct nss_cmn_node_stats node_stats;	/**< Common node stats */
	uint32_t tx_auth_done;			/**< Tx authentication done */
	uint32_t rx_auth_done;			/**< Rx successful authentication */
	uint32_t tx_cipher_done;		/**< Tx cipher done */
	uint32_t rx_cipher_done;		/**< Rx cipher done */
	uint32_t tx_cbuf_alloc_fail;		/**< Tx crypto buffer allocation fail */
	uint32_t rx_cbuf_alloc_fail;		/**< Rx crypto buffer allocation fail */
	uint32_t tx_cenqueue_fail;		/**< Tx enqueue to crypto fail */
	uint32_t rx_cenqueue_fail;		/**< Rx enqueue to crypto fail */
	uint32_t tx_dropped_hroom;		/**< Tx drop due to insufficient headroom */
	uint32_t tx_dropped_troom;		/**< Tx drop due to insufficient tailroom */
	uint32_t tx_forward_enqueue_fail;	/**< Enqueue failed to forwarding node after encap */
	uint32_t rx_forward_enqueue_fail;	/**< Enqueue failed to receiving node after decap */
	uint32_t rx_invalid_version;		/**< Rx invalid DTLS version */
	uint32_t rx_invalid_epoch;		/**< Rx invalid DTLS epoch */
	uint32_t rx_malformed;			/**< Rx malformed DTLS record */
	uint32_t rx_cipher_fail;		/**< Rx cipher fail */
	uint32_t rx_auth_fail;			/**< Rx authentication fail */
	uint32_t rx_capwap_classify_fail;	/**< Rx CAPWAP classification fail */
	uint32_t rx_single_rec_dgram;		/**< Rx single record datagrams processed */
	uint32_t rx_multi_rec_dgram;		/**< Rx multi-record datagrams processed */
	uint32_t rx_replay_fail;		/**< Rx anti-replay failures */
	uint32_t rx_replay_duplicate;		/**< Rx anti-replay fail due to duplicate record */
	uint32_t rx_replay_out_of_window;	/**< Rx anti-replay fail due to out of window record */
	uint32_t outflow_queue_full;		/**< Tx drop due to encap queue full */
	uint32_t decap_queue_full;		/**< Rx drop due to decap queue full */
	uint32_t pbuf_alloc_fail;		/**< Drops due to buffer allocation failure */
	uint32_t pbuf_copy_fail;		/**< Drops due to buffer copy failure */
	uint16_t epoch;				/**< Current Epoch */
	uint16_t tx_seq_high;			/**< Upper 16-bits of current sequence number */
	uint32_t tx_seq_low;			/**< Lower 32-bits of current sequence number */
};

/**
 * DTLS session cipher update message structure
 */
struct nss_dtls_session_cipher_update {
	uint32_t crypto_idx;		/**< Crypto index for encap */
	uint32_t hash_len;		/**< Auth hash length for encap */
	uint32_t iv_len;		/**< Crypto IV length for encap */
	uint32_t cipher_algo;		/**< Encap cipher */
	uint32_t auth_algo;		/**< Encap auth algo */
	uint16_t epoch;			/**< Epoch */
	uint16_t reserved;		/**< Reserved */
};

/**
 * DTLS session configure message structure
 */
struct nss_dtls_session_configure {
	uint32_t ver;			/**< DTLS version */
	uint32_t flags;			/**< DTLS flags */
	uint32_t crypto_idx_encap;	/**< Crypto index for encap */
	uint32_t crypto_idx_decap;	/**< Crypto index for decap */
	uint32_t iv_len_encap;		/**< Crypto IV length for encap */
	uint32_t iv_len_decap;		/**< Crypto IV length for decap */
	uint32_t hash_len_encap;	/**< Auth hash length for encap */
	uint32_t hash_len_decap;	/**< Auth hash length for decap */
	uint32_t cipher_algo_encap;	/**< Cipher algorithm for encap */
	uint32_t auth_algo_encap;	/**< Auth algorithm encap */
	uint32_t cipher_algo_decap;	/**< Cipher algorithm for decap */
	uint32_t auth_algo_decap;	/**< Auth algorithm decap */
	uint32_t nss_app_if;
		/**< NSS I/F of node that receives decapsulated packets */
	uint16_t sport;			/**< Source UDP/UDPLite port */
	uint16_t dport;			/**< Destination UDP/UDPLite port */
	uint32_t sip[4];		/**< Source IPv4/IPv6 address */
	uint32_t dip[4];		/**< Destination IPv4/IPv6 address */
	uint16_t window_size;		/**< Anti-replay window size */
	uint16_t epoch;			/**< Epoch */
	uint8_t oip_ttl;		/**< Max IP ttl value */
	uint8_t reserved1;		/**< Reserved */
	uint16_t reserved2;		/**< Reserved */
};

/**
 * Message structure to send/receive dtls messages
 */
struct nss_dtls_msg {
	struct nss_cmn_msg cm;		/**< Message Header */
	union {
		struct nss_dtls_session_configure cfg;
		struct nss_dtls_session_cipher_update cipher_update;
		struct nss_dtls_session_stats stats;
	} msg;
};

/**
 * @brief Send DTLS data packet to NSS
 *
 * @param os_buf data buffer
 * @param if_num NSS interface number
 * @param nss_ctx NSS core context
 *
 * @return status
 */
nss_tx_status_t nss_dtls_tx_buf(struct sk_buff *os_buf, uint32_t if_num,
				struct nss_ctx_instance *nss_ctx);

/**
 * @brief Send dtls messages
 *
 * @param nss_ctx NSS context
 * @param msg NSS dtls tunnel message
 *
 * @return nss_tx_status_t Tx status
 */
extern nss_tx_status_t nss_dtls_tx_msg(struct nss_ctx_instance *nss_ctx,
				       struct nss_dtls_msg *msg);

/**
 * @brief Send dtls messages synchronously
 *
 * @param nss_ctx NSS context
 * @param msg NSS dtls tunnel message
 *
 * @return nss_tx_status_t Tx status
 */
extern nss_tx_status_t nss_dtls_tx_msg_sync(struct nss_ctx_instance *nss_ctx,
					    struct nss_dtls_msg *msg);
/**
 * @brief Callback to receive dtls messages
 *
 * @param app_data Application context of the message
 * @param msg NSS DTLS message
 *
 * @return void
 */
typedef void (*nss_dtls_msg_callback_t)(void *app_data,
					struct nss_dtls_msg *msg);

/**
 * @brief Callback when dtls session data is received
 *
 * @param netdev netdevice of dtls session
 * @param skb Pointer to data buffer
 * @param napi pointer
 *
 * @return void
 */
typedef void (*nss_dtls_data_callback_t)(struct net_device *netdev,
				    struct sk_buff *skb,
				    struct napi_struct *napi);

/**
 * @brief Register to send/receive dtls session messages to NSS
 *
 * @param if_num NSS interface number
 * @param cb Callback for dtls tunnel data
 * @param msg_callback Callback for dtls tunnel messages
 * @param netdev netdevice associated with the dtls tunnel
 * @param features denotes the skb types supported by this interface
 * @param app_ctx Application context
 *
 * @return NSS core context for DTLS
 */
extern struct nss_ctx_instance *nss_dtls_register_if(uint32_t if_num,
						     nss_dtls_data_callback_t cb,
						     nss_dtls_msg_callback_t msg_callback,
						     struct net_device *netdev,
						     uint32_t features,
						     void *app_ctx);

/**
 * @brief Unregister dtls session interface with NSS
 *
 * @param if_num NSS interface number
 *
 * @return void
 */
extern void nss_dtls_unregister_if(uint32_t if_num);

/**
 * @brief Initialize dtls msg
 *
 * @param ncm nss_dtls_msg
 * @param if_num Interface number
 * @param type Message type
 * @param len Message length
 * @param cb message callback
 * @param app_data
 *
 * @return None
 */
extern void nss_dtls_msg_init(struct nss_dtls_msg *ncm, uint16_t if_num,
			      uint32_t type, uint32_t len, void *cb,
			      void *app_data);

/**
 * @brief Gets NSS core context for DTLS
 *
 * @return NSS core context
 */
extern struct nss_ctx_instance *nss_dtls_get_context(void);

/**
 * @brief Gets DTLS interface number with core id
 *
 * @param if_num interface number
 *
 * @return interface number with core id
 */
extern int32_t nss_dtls_get_ifnum_with_coreid(int32_t if_num);
#endif /* _NSS_DTLS_H_ */
