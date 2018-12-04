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

/* * nss_ipsecmgr.h
 *	NSS to HLOS IPSec Manager interface definitions.
 */
#ifndef __NSS_IPSECMGR_H
#define __NSS_IPSECMGR_H

#define NSS_IPSECMGR_DEBUG_LVL_ERROR 1
#define NSS_IPSECMGR_DEBUG_LVL_WARN 2
#define NSS_IPSECMGR_DEBUG_LVL_INFO 3
#define NSS_IPSECMGR_DEBUG_LVL_TRACE 4

#define NSS_IPSECMGR_TUN_NAME "ipsectun%d"
#define NSS_IPSECMGR_MAX_TUNNELS (NSS_CRYPTO_MAX_IDXS/2)

/*
 * This is a rough estimate need to be accurate but large enough to
 * accommodate most use cases
 */
#define NSS_IPSECMGR_TUN_MAX_HDR_LEN 96

/*
 * Space required in the head and tail of the buffer
 */
#define NSS_IPSECMGR_TUN_HEADROOM 128
#define NSS_IPSECMGR_TUN_TAILROOM 192

#define NSS_IPSECMGR_TUN_MTU(x) (x - NSS_IPSECMGR_TUN_MAX_HDR_LEN)

#define NSS_IPSECMGR_NATT_PORT_DATA 4500

#define NSS_IPSECMGR_MIN_REPLAY_WIN 32
#define NSS_IPSECMGR_MAX_REPLAY_WIN 1024
#define NSS_IPSECMGR_MAX_ICV_LEN 32
#define NSS_IPSECMGR_MAX_DSCP 63

/**
 * @brief Flow types
 */
enum nss_ipsecmgr_flow_type {
	NSS_IPSECMGR_FLOW_TYPE_NONE = 0,	/**< None */
	NSS_IPSECMGR_FLOW_TYPE_V4_TUPLE = 1,	/**< IPv4 tuple */
	NSS_IPSECMGR_FLOW_TYPE_V6_TUPLE = 2,	/**< IPv6 tuple */
	NSS_IPSECMGR_FLOW_TYPE_V4_SUBNET = 3,	/**< IPv4 Subnet */
	NSS_IPSECMGR_FLOW_TYPE_V6_SUBNET = 4,	/**< IPv6 Subnet */
	NSS_IPSECMGR_FLOW_TYPE_MAX
};

/**
 * @brief SA types
 */
enum nss_ipsecmgr_sa_type {
	NSS_IPSECMGR_SA_TYPE_NONE = 0,	/**< None */
	NSS_IPSECMGR_SA_TYPE_V4 = 1,	/**< IPv4 SA */
	NSS_IPSECMGR_SA_TYPE_V6 = 2,	/**< IPv6 SA */
	NSS_IPSECMGR_SA_TYPE_MAX
};

/**
 * @brief NSS IPsec manager event type
 */
enum nss_ipsecmgr_event_type {
	NSS_IPSECMGR_EVENT_NONE = 0,	/**< invalid event type */
	NSS_IPSECMGR_EVENT_SA_STATS,	/**< statistics sync */
	NSS_IPSECMGR_EVENT_MAX
};

/**
 * @brief IPv4 Security Association
 */
struct nss_ipsecmgr_sa_v4 {
	uint32_t src_ip;		/**< IPv4 source IP */
	uint32_t dst_ip;		/**< IPv4 destination IP */
	uint32_t ttl;			/**< IPv4 time-to-live*/

	uint32_t spi_index;		/**< ESP SPI index */
};

/**
 * @brief IPv6 Security Association
 */
struct nss_ipsecmgr_sa_v6 {
	uint32_t src_ip[4];		/**< IPv6 source IP */
	uint32_t dst_ip[4];		/**< IPv6 destination IP */
	uint32_t hop_limit;		/**< IPv6 hop limit*/
	uint32_t spi_index;		/**< ESP SPI index */
};

/**
 * @brief IPsec SA data
 *
 * Note: for DSCP marking the following should be used
 *
 * +----------------------+---------------+------------------+
 * | Copy inner to outer  | dscp_copy = 1 | dscp = 0         |
 * +----------------------+---------------+------------------+
 * | Fixed mark on outer  | dscp_copy = 0 | dscp = <0 .. 63> |
 * +----------------------+---------------+------------------+
 */
struct nss_ipsecmgr_sa_data {
	uint32_t crypto_index;		/**< crypto session index returned by the driver */

	struct {
		uint16_t replay_win;	/**< sequence number window size for anti-replay */
		uint8_t icv_len;	/**< Hash Length */
		uint8_t dscp;		/**< default DSCP value of the SA */

		bool dscp_copy;		/**< copy DSCP from inner header to outer */
		bool nat_t_req;		/**< NAT-T required */
		bool seq_skip;		/**< Skip ESP sequence for ENCAP */
		bool trailer_skip;	/**< Skip ESP trailer for ENCAP */
	} esp;

	bool enable_esn;		/**< Enable Extended Sequence Number */
	bool use_pattern;		/**< Use random pattern in hash calculation */
};

/**
 * @brief IPv4 encap flow tuple
 */
struct nss_ipsecmgr_encap_v4_tuple {
	uint32_t src_ip;		/**< source IP */
	uint32_t dst_ip;		/**< destination IP */
	uint32_t protocol;		/**< protocol */
};

/**
 * @brief IPv6 encap flow tuple
 */
struct nss_ipsecmgr_encap_v6_tuple {
	uint32_t src_ip[4];		/**< source IP */
	uint32_t dst_ip[4];		/**< destination IP */
	uint32_t next_hdr;		/**< next header */
};

/**
 * @brief IPv4 encap flow subnet
 */
struct nss_ipsecmgr_encap_v4_subnet {
	uint32_t dst_subnet;		/**< destination subnet */
	uint32_t dst_mask;		/**< destination subnet mask */
	uint32_t protocol;		/**< protocol */
};

/**
 * @brief IPv6 encap flow subnet
 *
 * We will store least significant word in dst_subnet[0] and
 * most significant in dst_subnet[3]
 */
struct nss_ipsecmgr_encap_v6_subnet {
	uint32_t dst_subnet[4];		/**< destination subnet */
	uint32_t dst_mask[4];		/**< destination subnet mask */
	uint32_t next_hdr;		/**< next header */
};

/**
 * @brief NSS IPsec manager SA
 */
struct nss_ipsecmgr_sa {
	enum nss_ipsecmgr_sa_type type;
	union {
		struct nss_ipsecmgr_sa_v4 v4;	/**< IPv4 SA */
		struct nss_ipsecmgr_sa_v6 v6;	/**< IPv6 SA */
	} data;
};

/**
 * @brief SA stats exported by NSS IPsec manager
 */
struct nss_ipsecmgr_sa_stats {
	struct nss_ipsecmgr_sa sa;	/**< SA information */
	uint32_t crypto_index;		/**< crypto session index */

	struct {
		uint32_t bytes;		/**< bytes processed */
		uint32_t count;		/**< packets processed */
	} pkts;

	uint64_t seq_num;		/**< Curr seq number */
	uint64_t window_max;		/**< window top */
	uint32_t window_size;		/**< window size */

	bool esn_enabled;		/**< Is ESN enabled */
};

/**
 * @brief NSS IPsec manager event
 */
struct nss_ipsecmgr_event {
	enum nss_ipsecmgr_event_type type;		/**< Event type */
	union {
		struct nss_ipsecmgr_sa_stats stats;	/**< Event: SA statistics */
	} data;
};

/**
 * @brief NSS IPsec manger encap flow
 */
struct nss_ipsecmgr_encap_flow {
	enum nss_ipsecmgr_flow_type type;				/**< type */
	union {
		struct nss_ipsecmgr_encap_v4_tuple v4_tuple;		/**< IPv4 tuple */
		struct nss_ipsecmgr_encap_v4_subnet v4_subnet;		/**< IPv4 subnet */
		struct nss_ipsecmgr_encap_v6_tuple v6_tuple;		/**< IPv6 tuple */
		struct nss_ipsecmgr_encap_v6_subnet v6_subnet;		/**< IPv6 subnet */
	} data;
};

#ifdef __KERNEL__ /* only kernel will use */

typedef void (*nss_ipsecmgr_data_cb_t) (void *ctx, struct sk_buff *skb);
typedef void (*nss_ipsecmgr_event_cb_t) (void *ctx, struct nss_ipsecmgr_event *ev);

/**
 * @brief IPsec manager callback information
 */
struct nss_ipsecmgr_callback {
	void *ctx;				/**< context of caller */
	nss_ipsecmgr_data_cb_t data_fn;		/**< data callback function */
	nss_ipsecmgr_event_cb_t event_fn;	/**< event callback function */
};

/**
 * @brief Add a new IPsec tunnel
 *
 * @param cb[IN] Callback info
 *
 * @return Linux NETDEVICE or NULL
 */
struct net_device *nss_ipsecmgr_tunnel_add(struct nss_ipsecmgr_callback *cb);

/**
 * @brief Delete an existing IPsec tunnel
 *
 * @param tun[IN] Linux NETDEVICE
 *
 * @return success or failure
 */
bool nss_ipsecmgr_tunnel_del(struct net_device *tun);

/**
 * @brief update the binding of netdevice & callback
 *
 * @param tun[IN] IPsec NETDEVICE
 * @param cur[IN] Linux NETDEVICE
 */
void nss_ipsecmgr_tunnel_update_callback(struct net_device *tun, struct net_device *cur);

/**
 * @brief Add an ENCAP flow rule to the IPsec offload database
 *
 * @param tun[IN] IPsec tunnel
 * @param flow[IN] Flow or Subnet to add
 * @param sa[IN] Flow Security Association for the flow
 * @param data[IN] Additional SA Data
 *
 * @return success or failure
 */
bool nss_ipsecmgr_encap_add(struct net_device *tun, struct nss_ipsecmgr_encap_flow *flow, struct nss_ipsecmgr_sa *sa,
				struct nss_ipsecmgr_sa_data *data);

/**
 * @brief Delete an ENCAP flow rule from the IPsec offload database
 *
 * @param tun[IN] IPsec tunnel
 * @param flow[IN] Flow or Subnet to delete
 *
 * @return sucess or failure
 */
bool nss_ipsecmgr_encap_del(struct net_device *tun, struct nss_ipsecmgr_encap_flow *flow, struct nss_ipsecmgr_sa *sa);

/**
 * @brief Add a DECAP SA to offload database
 *
 * @param tun[IN] IPsec tunnel
 * @param sa[IN] Security Association for the DECAP
 * @param data[IN] SA data
 *
 * @return success or failure
 */
bool nss_ipsecmgr_decap_add(struct net_device *tun, struct nss_ipsecmgr_sa *sa, struct nss_ipsecmgr_sa_data *data);

/**
 * @brief Flush the SA and all associated flows/subnets
 *
 * @param tun[IN] IPsec tunnel
 * @param sa[IN] SA to flush
 *
 * @return success or failure
 */
bool nss_ipsecmgr_sa_flush(struct net_device *tun, struct nss_ipsecmgr_sa *sa);

#endif /* __KERNEL__ */
#endif /* __NSS_IPSECMGR_H */
