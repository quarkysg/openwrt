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
 * nss_map_t.h
 *	NSS TO HLOS interface definitions.
 */
#ifndef _NSS_MAP_T_H_
#define _NSS_MAP_T_H_

/**
 * Maximum no of map_t instance supported
 */
#define NSS_MAX_MAP_T_DYNAMIC_INTERFACES 4

/**
 * request/response types
 */
enum nss_map_t_msg_types {
	NSS_MAP_T_MSG_INSTANCE_RULE_CONFIGURE,		/**< map-t instance rule config message */
	NSS_MAP_T_MSG_INSTANCE_RULE_DECONFIGURE,	/**< map-t instance rule deconfig message */
	NSS_MAP_T_MSG_SYNC_STATS,			/**< map-t instance stats sync message */
	NSS_MAP_T_MSG_MAX
};

/**
 * map_t instance create message structure
 */
struct nss_map_t_instance_rule_config_msg {
	uint32_t rule_num;			/**< Rule number */
	uint32_t total_rules;			/**< Total number of Rules */
	uint32_t local_ipv6_prefix_len;		/**< IPv6 prefix length */
	uint32_t local_ipv4_prefix;		/**< IPv4 prefix */
	uint32_t local_ipv4_prefix_len;		/**< IPv4 prefix length */
	uint32_t local_ea_len;			/**< EA bits length */
	uint32_t local_psid_offset;		/**< PSID offset */

	uint32_t reserve_a;			/**< Reserved */

	uint32_t remote_ipv6_prefix_len;	/**< IPv6 prefix length */
	uint32_t remote_ipv4_prefix;		/**< IPv4 prefix */
	uint32_t remote_ipv4_prefix_len;	/**< IPv4 prefix length */
	uint32_t remote_ea_len;			/**< EA bits length */
	uint32_t remote_psid_offset;		/**< PSID offset */

	uint32_t local_map_style;		/**< Local MAP style */
	uint32_t remote_map_style;		/**< Remote MAP style */

	uint8_t local_ipv6_prefix[16];		/**< IPv6 prefix */
	uint8_t reserve_b[16];			/**< Reserve */
	uint8_t remote_ipv6_prefix[16];		/**< IPv6 prefix */

	uint8_t valid_rule;			/**< Correctness of map-t rule */
	uint8_t reserved[3];			/**< Padding */
};

/**
 * map_t instance delete message structure
 */
struct nss_map_t_instance_rule_deconfig_msg {
	int32_t if_number;			/**< Interface number */
};

/**
 * map_t statistics sync message structure.
 */
struct nss_map_t_sync_stats_msg {
	struct nss_cmn_node_stats node_stats;			/**< common node stats */
	uint32_t tx_dropped;					/**< Tx dropped packets */
	struct {
		struct {
			uint32_t exception_pkts;		/**< Exceptioned packets */
			uint32_t no_matching_rule;		/**< Not matching any rule */
			uint32_t not_tcp_or_udp;		/**< TCP or UDP ?  */
			uint32_t rule_err_local_psid;		/**< calculate local psid err */
			uint32_t rule_err_local_ipv6;		/**< Calculate local ipv6 err */
			uint32_t rule_err_remote_psid;		/**< Remote psid err */
			uint32_t rule_err_remote_ea_bits;	/**< Remote ea bits err */
			uint32_t rule_err_remote_ipv6;		/**< Remote ipv6 err */
		} v4_to_v6;

		struct {
			uint32_t exception_pkts;		/**< Exceptioned packets */
			uint32_t no_matching_rule;		/**< Not matching any rule */
			uint32_t not_tcp_or_udp;		/**< TCP or UDP ?  */
			uint32_t rule_err_local_ipv4;		/**< Calculate local ipv4 err */
			uint32_t rule_err_remote_ipv4;		/**< Calculate remote ipv4 err */
		} v6_to_v4;

	} debug_stats;
};

/**
 * Message structure to send/receive map_t messages
 */
struct nss_map_t_msg {
	struct nss_cmn_msg cm;	/**< Message Header */
	union {
		struct nss_map_t_instance_rule_config_msg create_msg;  /**< create message */
		struct nss_map_t_instance_rule_deconfig_msg destroy_msg; /**< destroy message */
		struct nss_map_t_sync_stats_msg stats;			   /**< stats message */
	} msg;
};

/**
 * @brief Callback to receive map_t messages
 *
 * @param app_data data
 * @param msg NSS map-t message
 *
 * @return void
 */
typedef void (*nss_map_t_msg_callback_t)(void *app_data, struct nss_map_t_msg *msg);

/**
 * @brief Send map_t messages
 *
 * @param nss_ctx NSS context
 * @param msg NSS map-t message
 *
 * @return nss_tx_status_t Tx status
 */
extern nss_tx_status_t nss_map_t_tx(struct nss_ctx_instance *nss_ctx, struct nss_map_t_msg *msg);

/**
 * @brief Send map_t messages synchronously
 *
 * @param nss_ctx NSS context
 * @param msg NSS map-t message
 *
 * @return nss_tx_status_t Tx status
 */
extern nss_tx_status_t nss_map_t_tx_sync(struct nss_ctx_instance *nss_ctx, struct nss_map_t_msg *msg);

/**
 * @brief Get the map_t context used in the nss_map_t_tx
 *
 * @return struct nss_ctx_instance *NSS context
 */
extern struct nss_ctx_instance *nss_map_t_get_context(void);

/**
 * @brief Callback when map_t tunnel data is received
 *
 * @param netdev netdevice of map_t instance
 * @param skb Pointer to data buffer
 * @param napi napi pointer
 *
 * @return void
 */
typedef void (*nss_map_t_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief Register to send/receive map_t tunnel messages to NSS
 *
 * @param if_num NSS interface number
 * @param map_t_callback Callback for map-t data
 * @param msg_callback Callback for map-t messages
 * @param netdev netdevice associated with the map-t
 * @param features denotes the skb types supported by this interface
 *
 * @return nss_ctx_instance* NSS context
 */
extern struct nss_ctx_instance *nss_map_t_register_if(uint32_t if_num, nss_map_t_callback_t map_t_callback,
					nss_map_t_msg_callback_t msg_callback, struct net_device *netdev, uint32_t features);

/**
 * @brief Unregister map_t tunnel interface with NSS
 *
 * @param if_num NSS interface number
 *
 * @return void
 */
extern void nss_map_t_unregister_if(uint32_t if_num);

/**
 * @brief Initialize map_t msg
 *
 * @param ncm nss_map_t_msg
 * @param if_num Interface number
 * @param type Message type
 * @param len Message length
 * @param cb message callback
 * @param app_data data
 *
 * @return None
 */
extern void nss_map_t_msg_init(struct nss_map_t_msg *ncm, uint16_t if_num, uint32_t type,  uint32_t len, void *cb, void *app_data);

/**
 * @brief register map_t nss debug stats handler
 *
 * @return None
 */
extern void nss_map_t_register_handler(void);

/**
 * @brief get map_t nss instance debug stats. stats_mem should be large enought to hold all stats.
 *
 * @param stats_mem memory address to be copied to
 *
 * @return None
 */
extern void nss_map_t_instance_debug_stats_get(void *stats_mem);

#endif /* _NSS_MAP_T_H_ */
