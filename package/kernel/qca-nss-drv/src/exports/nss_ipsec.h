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
 * nss_ipsec.h
 *	NSS to HLOS IPSec interface definitions.
 */

#ifndef __NSS_IPSEC_H
#define __NSS_IPSEC_H

/*
 * For some reason Linux doesn't define this in if_arp.h,
 * refer http://www.iana.org/assignments/arp-parameters/arp-parameters.xhtml
 * for the full list
 */
#define NSS_IPSEC_ARPHRD_IPSEC 31	/**< iana.org ARP Hardware type for IPsec tunnel*/
#define NSS_IPSEC_MAX_RULES 256 	/**< maximum rules supported */
#define NSS_IPSEC_MAX_SA NSS_CRYPTO_MAX_IDXS /**< maximum SAs supported */

#if (~(NSS_IPSEC_MAX_RULES - 1) & (NSS_IPSEC_MAX_RULES >> 1))
#error "NSS Max SA should be a power of 2"
#endif

#define NSS_IPSEC_MSG_LEN (sizeof(struct nss_ipsec_msg) - sizeof(struct nss_cmn_msg))

/**
 * @brief IPsec rule types
 */
enum nss_ipsec_msg_type {
	NSS_IPSEC_MSG_TYPE_NONE = 0,		/**< nothing to do */
	NSS_IPSEC_MSG_TYPE_ADD_RULE = 1,	/**< add rule to the table */
	NSS_IPSEC_MSG_TYPE_DEL_RULE = 2,	/**< delete rule from the table */
	NSS_IPSEC_MSG_TYPE_FLUSH_TUN = 3,	/**< delete all SA(s) for a tunnel */
	NSS_IPSEC_MSG_TYPE_SYNC_SA_STATS = 4,	/**< stats sync message */
	NSS_IPSEC_MSG_TYPE_SYNC_FLOW_STATS = 5,	/**< stats per flow */
	NSS_IPSEC_MSG_TYPE_SYNC_NODE_STATS = 6,	/**< stats per node [encap/decap] */
	NSS_IPSEC_MSG_TYPE_MAX
};

/**
 * @brief NSS IPsec status
 */
typedef enum nss_ipsec_status {
	NSS_IPSEC_STATUS_OK = 0,
	NSS_IPSEC_STATUS_ENOMEM = 1,
	NSS_IPSEC_STATUS_ENOENT = 2,
	NSS_IPSEC_STATUS_MAX
} nss_ipsec_status_t;

/**
 * @brief NSS IPsec rule error rypes
 */
enum nss_ipsec_error_type {
	NSS_IPSEC_ERROR_TYPE_NONE = 0,			/**< No Error */
	NSS_IPSEC_ERROR_TYPE_HASH_DUPLICATE = 1,	/**< Duplicate Entry request */
	NSS_IPSEC_ERROR_TYPE_HASH_COLLISION = 2,	/**< New request conflicts with existing request */
	NSS_IPSEC_ERROR_TYPE_UNHANDLED_MSG = 3,		/**< Unhandles message type */
	NSS_IPSEC_ERROR_TYPE_INVALID_RULE = 4,		/**< Invalid flow rule */
	NSS_IPSEC_ERROR_TYPE_MAX_SA = 5,		/**< SA unavailable */
	NSS_IPSEC_ERROR_TYPE_MAX_FLOW = 6,		/**< Flow table full */
	NSS_IPSEC_ERROR_TYPE_INVALID_CINDEX = 7,	/**< invalid crypto index */
	NSS_IPSEC_ERROR_TYPE_MAX
};

/**
 * @brief IPsec operation Type
 */
enum nss_ipsec_type {
	NSS_IPSEC_TYPE_NONE = 0,
	NSS_IPSEC_TYPE_ENCAP = 1,	/**< Encap */
	NSS_IPSEC_TYPE_DECAP = 2,	/**< Decap */
	NSS_IPSEC_TYPE_MAX
};

/**
 * @brief IPsec rule selector tuple for encap & decap
 *
 * @note This is a common selector which is used for preparing
 * a lookup tuple for incoming packets. The tuple is used
 * for deriving the index into the rule table. Choosing the
 * selector fields is dependent upon IPsec encap or
 * decap package itself. Host has zero understanding of
 * these index derivation from the selector fields and
 * hence provides information for all entries in the structure.
 * The packages {Encap or Decap} returns the index into their
 * respective tables to the Host for storing the rule which can
 * be referenced by host in future
 */
struct nss_ipsec_tuple {
	uint32_t dst_addr[4];	/**< destination IP */
	uint32_t src_addr[4];	/**< source IP */

	uint32_t esp_spi;	/**< SPI index */

	uint16_t dst_port;	/**< destination port (UDP or TCP) */
	uint16_t src_port;	/**< source port (UDP or TCP) */

	uint8_t proto_next_hdr;	/**< IP  protocol types */
	uint8_t ip_ver;		/**< IP version */
	uint8_t res[2];
};

/**
 * @brief Common structure for IPsec rule outer IP header info
 */
struct nss_ipsec_rule_oip {
	uint32_t dst_addr[4];		/**< IPv4 destination address to apply */
	uint32_t src_addr[4];		/**< IPv4 source address to apply */

	uint32_t esp_spi;		/**< ESP SPI index to apply */

	uint8_t ttl_hop_limit;		/**< IPv4 Time-to-Live value to apply */
	uint8_t ip_ver;			/**< IP version */
	uint8_t res[2];			/**< reserve for 4-byte alignment */
};

/**
 * @brief IPsec rule data to be used for per packet transformation
 */
struct nss_ipsec_rule_data {

	uint16_t crypto_index;		/**< crypto index for the SA */
	uint16_t window_size;		/**< ESP sequence number window */

	uint8_t cipher_blk_len;
	uint8_t iv_len;
	uint8_t nat_t_req;		/**< NAT-T required */
	uint8_t esp_icv_len;		/**< ESP trailers ICV length to apply */

	uint8_t esp_seq_skip;		/**< Skip ESP sequence number */
	uint8_t esp_tail_skip;		/**< Skip ESP trailer */
	uint8_t use_pattern;		/**< Use random pattern in hash calculation */
	uint8_t enable_esn;		/**< Enable Extended Sequence Number */

	uint8_t dscp;                   /**< Default dscp value of the SA */
	uint8_t sa_dscp_mask;		/**< Mask for the SA DSCP */
	uint8_t flow_dscp_mask;         /**< Mask for flow DSCP */
	uint8_t res1;

	uint32_t res2[4];
};

/**
 * @brief IPsec rule push message, sent from Host --> NSS for
 * 	  performing a operation on NSS rule tables
 */
struct nss_ipsec_rule {
	struct nss_ipsec_rule_oip oip;		/**< per rule outer IP info */
	struct nss_ipsec_rule_data data;	/**< per rule data */

	uint32_t index;				/**< rule index provided by NSS */
	uint32_t sa_idx;			/**< index into SA table */
};

/**
 * @brief NSS IPsec per SA statistics
 */
struct nss_ipsec_sa_stats {
	uint32_t count;			/**< packets processed */
	uint32_t bytes;			/**< bytes processed */
	uint32_t no_headroom;		/**< insufficient headroom */
	uint32_t no_tailroom;		/**< insufficient tailroom */
	uint32_t no_resource;		/**< no crypto buffer */
	uint32_t fail_queue;		/**< failed to enqueue */
	uint32_t fail_hash;		/**< hash mismatch */
	uint32_t fail_replay;		/**< replay chaeck failed */
	uint64_t seq_num;		/**< curr seq number */
	uint64_t window_max;		/**< window top */
	uint32_t window_size;		/**< window size */
	uint8_t esn_enabled;		/**< is ESN enabled */
	uint8_t res[3];
} __attribute__((packed));

/**
 * @brief NSS IPsec per flow statsistics
 */
struct nss_ipsec_flow_stats {
	uint32_t processed;			/**< packets processed for this flow */

	uint8_t use_pattern;			/**< use random pattern */
	uint8_t res[3];
};

/**
 * @brief NSS IPsec stats per node
 */
struct nss_ipsec_node_stats {
	uint32_t enqueued;			/**< packets enqueued to the node */
	uint32_t completed;			/**< packets processed by the node */
	uint32_t linearized;			/**< linearized the packet */
	uint32_t exceptioned;			/**< packets exception from NSS */
	uint32_t fail_enqueue;			/**< packets failed to enqueue */
};

/**
 * @brief common statistics structure
 */
union nss_ipsec_stats {
	struct nss_ipsec_sa_stats sa;           /**< SA statistics */
	struct nss_ipsec_flow_stats flow;       /**< flow statistics */
	struct nss_ipsec_node_stats node;       /**< pnode statistics */
};

/**
 * @brief Message structure to send/receive ipsec messages
 */
struct nss_ipsec_msg {
	struct nss_cmn_msg cm;				/**< Message Header */

	uint32_t tunnel_id;				/**< tunnel index associated with the message */
	struct nss_ipsec_tuple tuple;			/**< tuple for lookup */
	enum nss_ipsec_type type;			/**< Encap / Decap operation */

	union {
		struct nss_ipsec_rule rule;		/**< Message: IPsec rule */
		union nss_ipsec_stats stats;		/**< Message: Retreive stats for tunnel */
	} msg;
};

/**
 * @brief Message notification callback
 *
 * @param app_data[IN] context of the callback user
 * @param msg[IN] notification event data
 *
 * @return
 */
typedef void (*nss_ipsec_msg_callback_t)(void *app_data, struct nss_ipsec_msg *msg);

/**
 * @brief data callback
 *
 * @param app_data[IN] context of the callback user
 * @param skb[IN] data buffer
 *
 * @return
 */
typedef void (*nss_ipsec_buf_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief send an IPsec message
 *
 * @param nss_ctx[IN] NSS HLOS driver's context
 * @param msg[IN] control message
 *
 * @return
 */
extern nss_tx_status_t nss_ipsec_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_ipsec_msg *msg);

/**
 * @brief send an IPsec process request
 *
 * @param skb Data buffer
 * @param if_num NSS interface number
 *
 * @return Status
 */
extern nss_tx_status_t nss_ipsec_tx_buf(struct sk_buff *skb, uint32_t if_num);

/**
 * @brief register a event callback handler with HLOS driver
 *
 * @param if_num[IN] receive events from this interface (Encap, Decap or C2C)
 * @param cb[IN] event callback function
 * @param app_data[IN] context of the callback user
 *
 * @return
 */
extern struct nss_ctx_instance *nss_ipsec_notify_register(uint32_t if_num, nss_ipsec_msg_callback_t cb, void *app_data);

/**
 * @brief register a data callback handler with HLOS driver
 *
 * @param if_num[IN] receive data from this interface (Encap, Decap or C2C)
 * @param cb[IN] data callback function
 * @param netdev associated netdevice.
 * @param features denote the skb types supported by this interface.
 *
 * @return
 */
extern struct nss_ctx_instance *nss_ipsec_data_register(uint32_t if_num, nss_ipsec_buf_callback_t cb, struct net_device *netdev, uint32_t features);

/**
 * @brief unregister the message notifier
 *
 * @param ctx[IN] HLOS driver's context
 * @param if_num[IN] interface number to unregister from
 *
 * @return
 */
extern void nss_ipsec_notify_unregister(struct nss_ctx_instance *ctx, uint32_t if_num);

/**
 * @brief unregister the data notifier
 *
 * @param ctx[IN] HLOS driver's context
 * @param if_num[IN] interface number to unregister from
 *
 * @return
 */
extern void nss_ipsec_data_unregister(struct nss_ctx_instance *ctx, uint32_t if_num);

/**
 * @brief get the NSS context for the IPsec handle
 *
 * @return nss_ctx_instance
 */
extern struct nss_ctx_instance *nss_ipsec_get_context(void);

/**
 * @brief Initialize ipsec message
 *
 * @return void
 */
extern void nss_ipsec_msg_init(struct nss_ipsec_msg *nim, uint16_t if_num, uint32_t type, uint32_t len,
				nss_ipsec_msg_callback_t cb, void *app_data);

/**
 * @brief get the NSS interface number to be used for IPsec encap message
 *
 * @return encap interface number
 */
extern int32_t nss_ipsec_get_encap_interface(void);

/**
 * @brief get the NSS interface number to be used for IPsec decap message
 *
 * @return decap interface number
 */
extern int32_t nss_ipsec_get_decap_interface(void);

/**
 * @brief get the NSS interface number to be used for IPsec data trasnfer
 *
 * @return data interface number
 */
extern int32_t nss_ipsec_get_data_interface(void);
#endif /* __NSS_IPSEC_H */
