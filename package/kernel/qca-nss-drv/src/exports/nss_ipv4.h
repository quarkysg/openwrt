/*
 **************************************************************************
 * Copyright (c) 2014 - 2017, The Linux Foundation. All rights reserved.
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
 * nss_ipv4.h
 *	NSS to HLOS interface definitions.
 */

/*
 * TODO: Since this is the now the public header file for writting an
 * IPv4 message, we need to convert the entire file to doxygen.
 */

#ifndef __NSS_IPV4_H
#define __NSS_IPV4_H

/**
 * IPv4 bridge/route rule messages
 */
enum nss_ipv4_message_types {
	NSS_IPV4_TX_CREATE_RULE_MSG,		/**< IPv4 create rule message */
	NSS_IPV4_TX_DESTROY_RULE_MSG,		/**< IPv4 destroy rule message */
	NSS_IPV4_RX_DEPRECATED0,		/**< Deprecated: NSS_IPV4_RX_ESTABLISH_RULE_MSG */
	NSS_IPV4_RX_CONN_STATS_SYNC_MSG,	/**< IPv4 connection stats sync message */
	NSS_IPV4_RX_NODE_STATS_SYNC_MSG,	/**< IPv4 generic statistics sync message */
	NSS_IPV4_TX_CONN_CFG_RULE_MSG,		/**< IPv4 number of connections supported rule message */
	NSS_IPV4_TX_CREATE_MC_RULE_MSG,		/**< IPv4 multicast create rule message */
	NSS_IPV4_TX_CONN_STATS_SYNC_MANY_MSG,	/**< IPv4 request FW to send many conn sync message */
	NSS_IPV4_TX_ACCEL_MODE_CFG_MSG,		/**< IPv4 acceleration mode message */
	NSS_IPV4_TX_CONN_TABLE_SIZE_MSG,	/** IPv4 connection table size message */
	NSS_IPV4_TX_DSCP2PRI_CFG_MSG,	/** IPv4 dscp2pri mapping message */
	NSS_IPV4_MAX_MSG_TYPES,			/**< IPv4 message max type number */
};

/**
 * nss_ipv4_dscp_map_actions
 *	Action types mapped to DSCP values.
 */
enum nss_ipv4_dscp_map_actions {
	NSS_IPV4_DSCP_MAP_ACTION_ACCEL,
	NSS_IPV4_DSCP_MAP_ACTION_DONT_ACCEL,
	NSS_IPV4_DSCP_MAP_ACTION_MAX,
};

/**
 * NA IPv4 rule creation & rule update flags.
 */
#define NSS_IPV4_RULE_CREATE_FLAG_NO_SEQ_CHECK 0x01
					/**< Do not perform TCP sequence number checks */
#define NSS_IPV4_RULE_CREATE_FLAG_BRIDGE_FLOW 0x02
					/**< This is a pure bridge forwarding flow */
#define NSS_IPV4_RULE_CREATE_FLAG_ROUTED 0x04
					/**< Rule is for a routed connection */
#define NSS_IPV4_RULE_CREATE_FLAG_DSCP_MARKING 0x08
					/**< Rule has for a DSCP marking configured*/
#define NSS_IPV4_RULE_CREATE_FLAG_VLAN_MARKING 0x10
					/**< Rule has for a VLAN marking configured*/
#define NSS_IPV4_RULE_UPDATE_FLAG_CHANGE_MTU 0x20
					/**< Update MTU of connection interfaces */
#define NSS_IPV4_RULE_CREATE_FLAG_ICMP_NO_CME_FLUSH 0x40
					/**< Rule for not flushing CME on ICMP pkt */
#define NSS_IPV4_RULE_CREATE_FLAG_L2_ENCAP 0x80
					/**< The L2 payload is not IPv4 but consists of an encapsulating protocol that carries an IPv4 payload within it. */


/**
 * IPv4 rule creation validity flags.
 */
#define NSS_IPV4_RULE_CREATE_CONN_VALID 0x01		/**< IPv4 Connection is valid */
#define NSS_IPV4_RULE_CREATE_TCP_VALID 0x02		/**< TCP Protocol fields are valid */
#define NSS_IPV4_RULE_CREATE_PPPOE_VALID 0x04		/**< PPPoE fields are valid */
#define NSS_IPV4_RULE_CREATE_QOS_VALID 0x08		/**< QoS fields are valid */
#define NSS_IPV4_RULE_CREATE_VLAN_VALID 0x10		/**< VLAN fields are valid */
#define NSS_IPV4_RULE_CREATE_DSCP_MARKING_VALID 0x20	/**< DSCP marking fields are valid */
#define NSS_IPV4_RULE_CREATE_VLAN_MARKING_VALID 0x40	/**< VLAN marking fields are valid */
#define NSS_IPV4_RULE_CREATE_SRC_MAC_VALID 0x80		/**< Src MAC address fields are valid */
#define NSS_IPV4_RULE_CREATE_NEXTHOP_VALID 0x100	/**< Nexthop interface number fields are valid */

/**
 * IPv4 multicast command rule flags
 */
#define NSS_IPV4_MC_RULE_CREATE_FLAG_MC_UPDATE 0x01		/**< Multicast Rule update */

/**
 * IPv4 multicast command validity flags
 */
#define NSS_IPV4_MC_RULE_CREATE_FLAG_QOS_VALID 0x01		/**< QoS fields are valid */
#define NSS_IPV4_MC_RULE_CREATE_FLAG_DSCP_MARKING_VALID 0x02	/**< DSCP fields are valid */
#define NSS_IPV4_MC_RULE_CREATE_FLAG_INGRESS_VLAN_VALID 0x04	/**< Ingress VLAN fields are valid */
#define NSS_IPV4_MC_RULE_CREATE_FLAG_INGRESS_PPPOE 0x08		/**< Ingress PPPoE fields are valid */

/**
 * IPv4 multicast connection per-interface rule flags (to be used with rule_flags field of nss_ipv4_mc_if_rule structure)
 */
#define NSS_IPV4_MC_RULE_CREATE_IF_FLAG_BRIDGE_FLOW 0x01	/**< Bridge Flow */
#define NSS_IPV4_MC_RULE_CREATE_IF_FLAG_ROUTED_FLOW 0x02	/**< Routed flow */
#define NSS_IPV4_MC_RULE_CREATE_IF_FLAG_JOIN 0x04		/**< Interface has joined the flow */
#define NSS_IPV4_MC_RULE_CREATE_IF_FLAG_LEAVE 0x08		/**< Interface has left the flow */

/**
 * IPv4 multicast connection per-interface valid flags (to be used with valid_flags field of nss_ipv4_mc_if_rule structure)
 */
#define NSS_IPV4_MC_RULE_CREATE_IF_FLAG_VLAN_VALID 0x01		/**< VLAN fields are valid */
#define NSS_IPV4_MC_RULE_CREATE_IF_FLAG_PPPOE_VALID 0x02	/**< PPPoE fields are valid */
#define NSS_IPV4_MC_RULE_CREATE_IF_FLAG_NAT_VALID 0x4		/**< Interface is configured with Source-NAT */

/**
 * Source MAC address valid flags (to be used with mac_valid_flags field of nss_ipv4_src_mac_rule structure)
 */
#define NSS_IPV4_SRC_MAC_FLOW_VALID 0x01		/**< FLOW interface MAC address is valid */
#define NSS_IPV4_SRC_MAC_RETURN_VALID 0x02		/**< Return interface MAC address is valid */

/**
 * Common 5 tuple structure
 */
struct nss_ipv4_5tuple {
	uint32_t flow_ip;		/**< Flow IP address */
	uint32_t flow_ident;		/**< Flow ident (e.g. TCP/UDP port) */
	uint32_t return_ip;		/**< Return IP address */
	uint32_t return_ident;		/**< Return ident (e.g. TCP/UDP port) */
	uint8_t protocol;		/**< Protocol number */
	uint8_t reserved[3];		/**< Padded for alignment */
};

/**
 * Connection create structure
 */
struct nss_ipv4_connection_rule {
	uint16_t flow_mac[3];		/**< Flow MAC address */
	uint16_t return_mac[3];		/**< Return MAC address */
	int32_t flow_interface_num;	/**< Flow interface number */
	int32_t return_interface_num;	/**< Return interface number */
	uint32_t flow_mtu;		/**< Flow interface`s MTU */
	uint32_t return_mtu;		/**< Return interface`s MTU */
	uint32_t flow_ip_xlate;		/**< Translated flow IP address */
	uint32_t return_ip_xlate;	/**< Translated return IP address */
	uint32_t flow_ident_xlate;	/**< Translated flow ident (e.g. port) */
	uint32_t return_ident_xlate;	/**< Translated return ident (e.g. port) */
};

/**
 * PPPoE connection rules structure
 */
struct nss_ipv4_pppoe_rule {
	uint16_t flow_pppoe_session_id;		/**< Flow direction`s PPPoE session ID. */
	uint16_t flow_pppoe_remote_mac[3];	/**< Flow direction`s PPPoE Server MAC address */
	uint16_t return_pppoe_session_id;	/**< Return direction's PPPoE session ID. */
	uint16_t return_pppoe_remote_mac[3];	/**< Return direction's PPPoE Server MAC address */
};

/**
 * DSCP connection rule structure
 */
struct nss_ipv4_dscp_rule {
	uint8_t flow_dscp;		/**< Egress DSCP value for flow direction */
	uint8_t return_dscp;		/**< Egress DSCP value for return direction */
	uint8_t reserved[2];		/**< Padded for alignment */
};

/**
 * VLAN connection rule structure
 */
struct nss_ipv4_vlan_rule {
	uint32_t ingress_vlan_tag;	/**< VLAN Tag for the ingress packets */
	uint32_t egress_vlan_tag;	/**< VLAN Tag for egress packets */
};

/**
 * Nexthop interface numbers
 *
 * The next_hop is the very next interface that will recevied the
 * packet (as opposed to the final interface that the packet
 * will go out on).
 */
struct nss_ipv4_nexthop {
	int32_t flow_nexthop;		/**< Flow Nexthop interface number */
	int32_t return_nexthop;		/**< Return Nexthop interface number */
};

/**
 * TCP connection rule structure
 */
struct nss_ipv4_protocol_tcp_rule {
	uint32_t flow_max_window;	/**< Flow direction's largest seen window */
	uint32_t return_max_window;	/**< Return direction's largest seen window */
	uint32_t flow_end;		/**< Flow direction's largest seen sequence + segment length */
	uint32_t return_end;		/**< Return direction's largest seen sequence + segment length */
	uint32_t flow_max_end;		/**< Flow direction's largest seen ack + max(1, win) */
	uint32_t return_max_end;	/**< Return direction's largest seen ack + max(1, win) */
	uint8_t flow_window_scale;	/**< Flow direction's window scaling factor */
	uint8_t return_window_scale;	/**< Return direction's window scaling factor */
	uint16_t reserved;		/**< Padded for alignment */
};

/**
 * QoS connection rule structure
 */
struct nss_ipv4_qos_rule {
	uint32_t flow_qos_tag;		/**< QoS tag associated with this rule for flow direction */
	uint32_t return_qos_tag;	/**< QoS tag associated with this rule for return direction */
};

/**
 * Src MAC address rule structure
 */
struct nss_ipv4_src_mac_rule {
	uint32_t mac_valid_flags;	/**< MAC address valid flags */
	uint16_t flow_src_mac[3];	/**< Source MAC address for flow direction */
	uint16_t return_src_mac[3];	/**< Source MAC address for return direction */
};

/**
 * Error types for ipv4 messages
 */
enum nss_ipv4_error_response_types {
	NSS_IPV4_UNKNOWN_MSG_TYPE = 1,				/**< NSS Error: Unknown error */
	NSS_IPV4_CR_INVALID_PNODE_ERROR,			/**< NSS Error: Invalid interface number */
	NSS_IPV4_CR_MISSING_CONNECTION_RULE_ERROR, 		/**< NSS Error: Missing connection rule */
	NSS_IPV4_CR_BUFFER_ALLOC_FAIL_ERROR,			/**< NSS Error: Buffer allocation failure */
	NSS_IPV4_CR_PPPOE_SESSION_CREATION_ERROR,		/**< NSS Error: Unable to create PPPoE session */
	NSS_IPV4_DR_NO_CONNECTION_ENTRY_ERROR,			/**< NSS Error: No connection found to delete */
	NSS_IPV4_CR_CONN_CFG_ALREADY_CONFIGURED_ERROR,		/**< NSS Error: Conn cfg already done once */
	NSS_IPV4_CR_CONN_CFG_NOT_MULTIPLE_OF_QUANTA_ERROR,	/**< NSS Error: Conn cfg input is not multiple of quanta */
	NSS_IPV4_CR_CONN_CFG_EXCEEDS_LIMIT_ERROR,		/**< NSS Error: Conn cfg input exceeds max supported connections*/
	NSS_IPV4_CR_CONN_CFG_MEM_ALLOC_FAIL_ERROR,		/**< NSS Error: Conn cfg mem alloc fail at NSS FW */
	NSS_IPV4_CR_MULTICAST_INVALID_PROTOCOL,			/**< NSS Error: Invalid L4 protocol for multicast rule create */
	NSS_IPV4_CR_MULTICAST_UPDATE_INVALID_FLAGS,		/**< NSS Error: Invalid multicast flags for multicast update */
	NSS_IPV4_CR_MULTICAST_UPDATE_INVALID_IF,		/**< NSS Error: Invalid interface for multicast update */
	NSS_IPV4_CR_ACCEL_MODE_CONFIG_INVALID,			/**< NSS Error: Invalid config value for acceleration mode */
	NSS_IPV4_LAST						/**< NSS IPv4 max error response type */
};

/**
 * The IPv4 rule create sub-message structure.
 */
struct nss_ipv4_rule_create_msg {
	/* Request */
	uint16_t valid_flags;				/**< Bit flags associated with the validity of parameters */
	uint16_t rule_flags;				/**< Bit flags associated with the rule */

	struct nss_ipv4_5tuple tuple;			/**< Holds values of the 5 tuple */

	struct nss_ipv4_connection_rule conn_rule;	/**< Basic connection specific data */
	struct nss_ipv4_protocol_tcp_rule tcp_rule;	/**< TCP related accleration parameters */
	struct nss_ipv4_pppoe_rule pppoe_rule;		/**< PPPoE related accleration parameters */
	struct nss_ipv4_qos_rule qos_rule;		/**< QoS related accleration parameters */
	struct nss_ipv4_dscp_rule dscp_rule;		/**< DSCP related accleration parameters */
	struct nss_ipv4_vlan_rule vlan_primary_rule;	/**< Primary VLAN related accleration parameters */
	struct nss_ipv4_vlan_rule vlan_secondary_rule;	/**< Secondary VLAN related accleration parameters */
	struct nss_ipv4_src_mac_rule src_mac_rule;	/**< Source MAC address related acceleration parameters */
	struct nss_ipv4_nexthop nexthop_rule;		/**< Nexthop related parameters */

	/* Response */
	uint32_t reserved;					/**< Reserved field */
};

/**
 * The IPv4 multicast rule create per-interface information
 */
struct nss_ipv4_mc_if_rule {
	uint16_t rule_flags;				/**< Bit flags associated with the rule */
	uint16_t valid_flags;				/**< Bit flags associated with the validity of parameters */
	uint32_t xlate_src_ip;				/**< Translated flow IP address */
	uint32_t xlate_src_ident;			/**< Translated flow ident (e.g. port) */
	uint32_t egress_vlan_tag[MAX_VLAN_DEPTH];	/**< VLAN Tag stack for the egress packets */
	uint16_t pppoe_session_id;			/**< PPPoE session ID. */
	uint16_t pppoe_remote_mac[3];			/**< PPPoE Server MAC address */
	uint32_t if_num;				/**< Interface number */
	uint32_t if_mtu;				/**< Interface MTU */
	uint16_t if_mac[3];				/**< Interface MAC address */
	uint8_t reserved[2];
};

/**
 * The IPv4 multicast rule create sub-message structure.
 */
struct nss_ipv4_mc_rule_create_msg {
	struct nss_ipv4_5tuple tuple;			/**< Holds values of the 5 tuple */

	uint32_t rule_flags;				/**< Multicast command rule flags */
	uint32_t valid_flags;				/**< Multicast command validity flags */
	uint32_t src_interface_num;			/**< Source i/f number (virtual/physical) */
	uint32_t ingress_vlan_tag[MAX_VLAN_DEPTH];	/**< VLAN Tag stack for the ingress packets */
	uint16_t ingress_pppoe_session_id;		/**< PPPoE session ID at ingress */
	uint16_t ingress_pppoe_remote_mac[3];		/**< PPPoE Server MAC address */
	uint32_t qos_tag;				/**< Qos Tag for the rule */
	uint16_t dest_mac[3];				/**< Destination Multicast MAC address */
	uint16_t if_count;				/**< Number of destination interfaces */
	uint8_t egress_dscp;				/**< Egress DSCP value for flow */
	uint8_t reserved[3];

	struct nss_ipv4_mc_if_rule if_rule[NSS_MC_IF_MAX];	/**< Per if information */
};

/**
 * The IPv4 rule destroy sub-message structure.
 */
struct nss_ipv4_rule_destroy_msg {
	struct nss_ipv4_5tuple tuple;	/**< Holds values of the 5 tuple */
};

/**
 * nss_ipv4_rule_conn_get_table_size_msg
 *	IPv4 rule for fetching connection tables size.
 */
struct nss_ipv4_rule_conn_get_table_size_msg {
	uint32_t num_conn;		/**< Number of supported IPv4 connections. */
	uint32_t ce_table_size;		/**< Size of the connection entry table in NSS firmware. */
	uint32_t cme_table_size;	/**< Size of the connection match entry table in NSS firmware. */
};

/**
 * The IPv4 rule number of supported connections sub-message structure.
 */
struct nss_ipv4_rule_conn_cfg_msg {
	uint32_t num_conn;	/**< Number of supported IPv4 connections */
	uint32_t ce_mem;		/**< Memory allocated by host for connection entries table. */
	uint32_t cme_mem;		/**< Memory allocated by host for connection match entries table. */
};

/**
 * IPv4 rule sync reasons.
 */
#define NSS_IPV4_RULE_SYNC_REASON_STATS 0
					/**< Sync is to synchronize stats */
#define NSS_IPV4_RULE_SYNC_REASON_FLUSH 1
					/**< Sync is to flush a cache entry */
#define NSS_IPV4_RULE_SYNC_REASON_EVICT 2
					/**< Sync is to evict a cache entry */
#define NSS_IPV4_RULE_SYNC_REASON_DESTROY 3
					/**< Sync is to destroy a cache entry (requested by host OS) */
#define NSS_IPV4_RULE_SYNC_REASON_PPPOE_DESTROY 4
					/**< Sync is to destroy a cache entry which belongs to a particular PPPoE session */

/**
 * The NSS IPv4 rule sync structure.
 */
struct nss_ipv4_conn_sync {
	uint32_t reserved;			/**< Reserved field */
	uint8_t protocol;		/**< Protocol number */
	uint32_t flow_ip;		/**< Flow IP address */
	uint32_t flow_ip_xlate;		/**< Translated flow IP address */
	uint32_t flow_ident;		/**< Flow ident (e.g. port) */
	uint32_t flow_ident_xlate;	/**< Translated flow ident (e.g. port) */
	uint32_t flow_max_window;	/**< Flow direction's largest seen window */
	uint32_t flow_end;		/**< Flow direction's largest seen sequence + segment length */
	uint32_t flow_max_end;		/**< Flow direction's largest seen ack + max(1, win) */
	uint32_t flow_rx_packet_count;	/**< Flow interface's RX packet count */
	uint32_t flow_rx_byte_count;	/**< Flow interface's RX byte count */
	uint32_t flow_tx_packet_count;	/**< Flow interface's TX packet count */
	uint32_t flow_tx_byte_count;	/**< Flow interface's TX byte count */
	uint16_t flow_pppoe_session_id; /**< Flow interface`s PPPoE session ID. */
	uint16_t flow_pppoe_remote_mac[3];
					/**< Flow interface's PPPoE remote server MAC address if there is any */
	uint32_t return_ip;		/**< Return IP address */
	uint32_t return_ip_xlate;	/**< Translated return IP address */
	uint32_t return_ident;		/**< Return ident (e.g. port) */
	uint32_t return_ident_xlate;	/**< Translated return ident (e.g. port) */
	uint32_t return_max_window;	/**< Return direction's largest seen window */
	uint32_t return_end;		/**< Return direction's largest seen sequence + segment length */
	uint32_t return_max_end;	/**< Return direction's largest seen ack + max(1, win) */
	uint32_t return_rx_packet_count;
					/**< Return interface's RX packet count */
	uint32_t return_rx_byte_count;	/**< Return interface's RX byte count */
	uint32_t return_tx_packet_count;
					/**< Return interface's TX packet count */
	uint32_t return_tx_byte_count;	/**< Return interface's TX byte count */
	uint16_t return_pppoe_session_id;
					/**< Return interface`s PPPoE session ID. */
	uint16_t return_pppoe_remote_mac[3];
					/**< Return interface's PPPoE remote server MAC address if there is any */
	uint32_t inc_ticks;		/**< Number of ticks since the last sync */
	uint32_t reason;		/**< Reason for the sync */

	uint8_t flags;			/**< Bit flags associated with the rule */
	uint32_t qos_tag;		/**< QoS Tag */
	uint32_t cause;			/**< Flush Cause */
};

/**
 * The NSS IPv4 conn sync many structure.
 */
struct nss_ipv4_conn_sync_many_msg {
	/* Request */
	uint16_t index;		/**< Request conn stats from index */
	uint16_t size;		/**< The buf size of this msg */

	/* Response */
	uint16_t next;		/**< FW response the next conn to be requested */
	uint16_t count;		/**< How many conn_sync included in this msg */
	struct nss_ipv4_conn_sync conn_sync[]; /**< Array for the stats */
};

/**
 * The NSS IPv4 acceleration mode configure message.
 */
struct nss_ipv4_accel_mode_cfg_msg {
	uint32_t mode;	/**< Acceleration mode */
};

/**
 * nss_ipv4_dscp2pri_cfg_msg
 *	IPv4 dscp2pri configuration msg.
 */
struct nss_ipv4_dscp2pri_cfg_msg {
	uint8_t dscp;		/**< Value of DSCP. */
	uint8_t priority;	/**< Corresponding priority. */
};

/**
 * Exception events from bridge/route handler
 */
enum exception_events_ipv4 {
	NSS_EXCEPTION_EVENT_IPV4_ICMP_HEADER_INCOMPLETE,		/**<  NSS Exception event: ICMP protocol header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_ICMP_UNHANDLED_TYPE,			/**<  NSS Exception event: ICMP protocol unhandled type */
	NSS_EXCEPTION_EVENT_IPV4_ICMP_IPV4_HEADER_INCOMPLETE,		/**<  NSS Exception event: ICMP IPv4 header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_ICMP_IPV4_UDP_HEADER_INCOMPLETE,	/**<  NSS Exception event: ICMP IPv4 UDP header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_ICMP_IPV4_TCP_HEADER_INCOMPLETE,	/**<  NSS Exception event: ICMP IPv4 TCP header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_ICMP_IPV4_UNKNOWN_PROTOCOL,		/**<  NSS Exception event: ICMP IPv4 unknown protocol */
	NSS_EXCEPTION_EVENT_IPV4_ICMP_NO_ICME,				/**<  NSS Exception event: ICMP no IPv4 connection match entry */
	NSS_EXCEPTION_EVENT_IPV4_ICMP_FLUSH_TO_HOST,			/**<  NSS Exception event: ICMP flush the torn down connection to host*/
	NSS_EXCEPTION_EVENT_IPV4_TCP_HEADER_INCOMPLETE,			/**<  NSS Exception event: TCP protocol header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_TCP_NO_ICME,				/**<  NSS Exception event: TCP protocol no IPv4 connection match entry */
	NSS_EXCEPTION_EVENT_IPV4_TCP_IP_OPTION,				/**<  NSS Exception event: TCP protocol ip option */
	NSS_EXCEPTION_EVENT_IPV4_TCP_IP_FRAGMENT,			/**<  NSS Exception event: TCP protocol ip fragment */
	NSS_EXCEPTION_EVENT_IPV4_TCP_SMALL_TTL,				/**<  NSS Exception event: TCP protocol small ttl */
	NSS_EXCEPTION_EVENT_IPV4_TCP_NEEDS_FRAGMENTATION,		/**<  NSS Exception event: TCP protocol needs fragmentation */
	NSS_EXCEPTION_EVENT_IPV4_TCP_FLAGS,				/**<  NSS Exception event: TCP protocol flags */
	NSS_EXCEPTION_EVENT_IPV4_TCP_SEQ_EXCEEDS_RIGHT_EDGE,		/**<  NSS Exception event: TCP protocol sequential exceeds right edge */
	NSS_EXCEPTION_EVENT_IPV4_TCP_SMALL_DATA_OFFS,			/**<  NSS Exception event: TCP protocol small data offs */
	NSS_EXCEPTION_EVENT_IPV4_TCP_BAD_SACK,				/**<  NSS Exception event: TCP protocol bad sack */
	NSS_EXCEPTION_EVENT_IPV4_TCP_BIG_DATA_OFFS,			/**<  NSS Exception event: TCP protocol big data offs */
	NSS_EXCEPTION_EVENT_IPV4_TCP_SEQ_BEFORE_LEFT_EDGE,		/**<  NSS Exception event: TCP protocol sequential before left edge */
	NSS_EXCEPTION_EVENT_IPV4_TCP_ACK_EXCEEDS_RIGHT_EDGE,		/**<  NSS Exception event: TCP protocol ack exceeds right edge */
	NSS_EXCEPTION_EVENT_IPV4_TCP_ACK_BEFORE_LEFT_EDGE,		/**<  NSS Exception event: TCP protocol ack before left edge */
	NSS_EXCEPTION_EVENT_IPV4_UDP_HEADER_INCOMPLETE,			/**<  NSS Exception event: UDP protocol header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_UDP_NO_ICME,				/**<  NSS Exception event: UDP protocol no IPv4 connection match entry */
	NSS_EXCEPTION_EVENT_IPV4_UDP_IP_OPTION,				/**<  NSS Exception event: UDP protocol no ip option */
	NSS_EXCEPTION_EVENT_IPV4_UDP_IP_FRAGMENT,			/**<  NSS Exception event: UDP protocol no ip fragment */
	NSS_EXCEPTION_EVENT_IPV4_UDP_SMALL_TTL,				/**<  NSS Exception event: UDP protocol small ttl */
	NSS_EXCEPTION_EVENT_IPV4_UDP_NEEDS_FRAGMENTATION,		/**<  NSS Exception event: UDP protocol needs fragmentation */
	NSS_EXCEPTION_EVENT_IPV4_WRONG_TARGET_MAC,			/**<  NSS Exception event: IPv4 wrong target MAC address */
	NSS_EXCEPTION_EVENT_IPV4_HEADER_INCOMPLETE,			/**<  NSS Exception event: Incomplete IPv4 header */
	NSS_EXCEPTION_EVENT_IPV4_BAD_TOTAL_LENGTH,			/**<  NSS Exception event: IPv4 bad total length */
	NSS_EXCEPTION_EVENT_IPV4_BAD_CHECKSUM,				/**<  NSS Exception event: IPv4 bad checksum */
	NSS_EXCEPTION_EVENT_IPV4_NON_INITIAL_FRAGMENT,			/**<  NSS Exception event: IPv4 non initial fragment */
	NSS_EXCEPTION_EVENT_IPV4_DATAGRAM_INCOMPLETE,			/**<  NSS Exception event: IPv4 datagram incomplete */
	NSS_EXCEPTION_EVENT_IPV4_OPTIONS_INCOMPLETE,			/**<  NSS Exception event: IPv4 options incomplete */
	NSS_EXCEPTION_EVENT_IPV4_UNKNOWN_PROTOCOL,			/**<  NSS Exception event: IPv4 unknown protocol */
	NSS_EXCEPTION_EVENT_IPV4_ESP_HEADER_INCOMPLETE,			/**<  NSS Exception event: IPv4 esp header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_ESP_NO_ICME,				/**<  NSS Exception event: IPv4 esp no connection match entry */
	NSS_EXCEPTION_EVENT_IPV4_ESP_IP_OPTION,				/**<  NSS Exception event: IPv4 esp ip option */
	NSS_EXCEPTION_EVENT_IPV4_ESP_IP_FRAGMENT,			/**<  NSS Exception event: IPv4 esp ip fragment */
	NSS_EXCEPTION_EVENT_IPV4_ESP_SMALL_TTL,				/**<  NSS Exception event: IPv4 esp small ttl */
	NSS_EXCEPTION_EVENT_IPV4_ESP_NEEDS_FRAGMENTATION,		/**<  NSS Exception event: IPv4 esp needs fragmentation */
	NSS_EXCEPTION_EVENT_IPV4_IVID_MISMATCH,				/**<  NSS Exception event: IPv4 ivid mismatch */
	NSS_EXCEPTION_EVENT_IPV4_IVID_MISSING,				/**<  NSS Exception event: IPv4 ivid missing */
	NSS_EXCEPTION_EVENT_IPV4_6RD_NO_ICME,				/**<  NSS Exception event: IPv4 6RD no connection match entry */
	NSS_EXCEPTION_EVENT_IPV4_6RD_IP_OPTION,				/**<  NSS Exception event: IPv4 6RD ip option */
	NSS_EXCEPTION_EVENT_IPV4_6RD_IP_FRAGMENT,			/**<  NSS Exception event: IPv4 6RD ip fragment */
	NSS_EXCEPTION_EVENT_IPV4_6RD_NEEDS_FRAGMENTATION,		/**<  NSS Exception event: IPv4 6RD needs fragmentation */
	NSS_EXCEPTION_EVENT_IPV4_DSCP_MARKING_MISMATCH,			/**<  NSS Exception event: IPv4 dscp marking mismatch */
	NSS_EXCEPTION_EVENT_IPV4_VLAN_MARKING_MISMATCH,			/**<  NSS Exception event: IPv4 vlan marking mismatch */
	NSS_EXCEPTION_EVENT_IPV4_DEPRECATED,				/**<  NSS Exception event: IPv4 DEPRECATED TYPE */
	NSS_EXCEPTION_EVENT_GRE_HEADER_INCOMPLETE,			/**<  NSS Exception event: IPv4 GRE header is incomplete */
	NSS_EXCEPTION_EVENT_GRE_NO_ICME,				/**<  NSS Exception event: IPV4 GRE no connection match entry */
	NSS_EXCEPTION_EVENT_GRE_IP_OPTION,				/**<  NSS Exception event: IPV4 GRE ip option */
	NSS_EXCEPTION_EVENT_GRE_IP_FRAGMENT,				/**<  NSS Exception event: IPV4 GRE ip fragment */
	NSS_EXCEPTION_EVENT_GRE_SMALL_TTL,				/**<  NSS Exception event: IPV4 GRE small ttl */
	NSS_EXCEPTION_EVENT_GRE_NEEDS_FRAGMENTATION,			/**<  NSS Exception event: IPV4 GRE needs fragmentation */
	NSS_EXCEPTION_EVENT_IPV4_PPTP_GRE_SESSION_MATCH_FAIL,		/**<  NSS Exception event: IPv4 PPTP GRE session match fail */
	NSS_EXCEPTION_EVENT_IPV4_PPTP_GRE_INVALID_PROTO,		/**<  NSS Exception event: IPv4 PPTP GRE Invalid/Unknown Protocol */
	NSS_EXCEPTION_EVENT_IPV4_PPTP_GRE_NO_CME,			/**<  NSS Exception event: IPv4 PPTP GRE No ICME */
	NSS_EXCEPTION_EVENT_IPV4_PPTP_GRE_IP_OPTION,			/**<  NSS Exception event: IPv4 PPTP GRE IP Option */
	NSS_EXCEPTION_EVENT_IPV4_PPTP_GRE_IP_FRAGMENT,			/**<  NSS Exception event: IPv4 PPTP GRE IP Fragment */
	NSS_EXCEPTION_EVENT_IPV4_PPTP_GRE_SMALL_TTL,			/**<  NSS Exception event: IPv4 PPTP GRE Small TTL */
	NSS_EXCEPTION_EVENT_IPV4_PPTP_GRE_NEEDS_FRAGMENTATION,		/**<  NSS Exception event: IPv4 PPTP GRE Needs Fragmentation */
	NSS_EXCEPTION_EVENT_IPV4_DESTROY,				/**<  NSS Exception event: IPv4 Destroy */
	NSS_EXCEPTION_EVENT_IPV4_FRAG_DF_SET,				/**<  NSS Exception event: IPv4 fragmentation needed, but DF set */
	NSS_EXCEPTION_EVENT_IPV4_FRAG_FAIL,				/**<  NSS Exception event: IPv4 fragmentation failure */
	NSS_EXCEPTION_EVENT_IPV4_ICMP_IPV4_UDPLITE_HEADER_INCOMPLETE,	/**<  NSS Exception event: ICMP IPv4 UDPLite header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_UDPLITE_HEADER_INCOMPLETE,		/**<  NSS Exception event: UDPLite protocol header incomplete */
	NSS_EXCEPTION_EVENT_IPV4_UDPLITE_NO_ICME,			/**<  NSS Exception event: UDPLite protocol no IPv6 connection match entry */
	NSS_EXCEPTION_EVENT_IPV4_UDPLITE_IP_OPTION,			/**<  NSS Exception event: UDPLite protocol no ip option */
	NSS_EXCEPTION_EVENT_IPV4_UDPLITE_IP_FRAGMENT,			/**<  NSS Exception event: UDPLite protocol no ip fragment */
	NSS_EXCEPTION_EVENT_IPV4_UDPLITE_SMALL_TTL,			/**<  NSS Exception event: UDPLite protocol small ttl */
	NSS_EXCEPTION_EVENT_IPV4_UDPLITE_NEEDS_FRAGMENTATION,		/**<  NSS Exception event: UDPLite protocol needs fragmentation */
	NSS_EXCEPTION_EVENT_IPV4_MC_UDP_NO_ICME,			/**<  NSS Exception event: UDP protocol no multicast IPv4 connection match entry */
	NSS_EXCEPTION_EVENT_IPV4_MC_MEM_ALLOC_FAILURE,			/**<  NSS Exception event: IPv4 Multicast Memory allocation failure */
	NSS_EXCEPTION_EVENT_IPV4_MC_UPDATE_FAILURE,			/**<  NSS Exception event: IPv4 Multicast rule update failure */
	NSS_EXCEPTION_EVENT_IPV4_MC_PBUF_ALLOC_FAILURE,			/**<  NSS Exception event: IPv4 Multicast buffer allocation failure */
	NSS_EXCEPTION_EVENT_IPV4_MAX					/**<  IPv4 exception events max type number */
};

/**
 * IPv4 node statistics structure
 */
struct nss_ipv4_node_sync {
	struct nss_cmn_node_stats node_stats;
				/**< Common node stats for IPv4 */
	uint32_t ipv4_connection_create_requests;
				/**< Number of IPv4 connection create requests */
	uint32_t ipv4_connection_create_collisions;
				/**< Number of IPv4 connection create requests that collided with existing entries */
	uint32_t ipv4_connection_create_invalid_interface;
				/**< Number of IPv4 connection create requests that had invalid interface */
	uint32_t ipv4_connection_destroy_requests;
				/**< Number of IPv4 connection destroy requests */
	uint32_t ipv4_connection_destroy_misses;
				/**< Number of IPv4 connection destroy requests that missed the cache */
	uint32_t ipv4_connection_hash_hits;
				/**< Number of IPv4 connection hash hits */
	uint32_t ipv4_connection_hash_reorders;
				/**< Number of IPv4 connection hash reorders */
	uint32_t ipv4_connection_flushes;
				/**< Number of IPv4 connection flushes */
	uint32_t ipv4_connection_evictions;
				/**< Number of IPv4 connection evictions */
	uint32_t ipv4_fragmentations;
				/**< Number of successful IPv4 fragmentations performed*/
	uint32_t ipv4_mc_connection_create_requests;
				/**< Number of IPv4 Multicast connection create requests */
	uint32_t ipv4_mc_connection_update_requests;
				/**< Number of IPv4 Multicast connection update requests */
	uint32_t ipv4_mc_connection_create_invalid_interface;
				/**< Number of IPv4 Multicast connection create requests that had invalid interface */
	uint32_t ipv4_mc_connection_destroy_requests;
				/**< Number of IPv4 Multicast connection destroy requests */
	uint32_t ipv4_mc_connection_destroy_misses;
				/**< Number of IPv4 Multicast connection destroy requests that missed the cache */
	uint32_t ipv4_mc_connection_flushes;
				/**< Number of IPv4 Multicast connection flushes */
	uint32_t exception_events[NSS_EXCEPTION_EVENT_IPV4_MAX];
				/**< Number of IPv4 exception events */
};

/**
 * Message structure to send/receive IPv4 bridge/route commands
 */
struct nss_ipv4_msg {
	struct nss_cmn_msg cm;		/**< Message Header */
	union {
		struct nss_ipv4_rule_create_msg rule_create;	/**< Message: rule create */
		struct nss_ipv4_rule_destroy_msg rule_destroy;	/**< Message: rule destroy */
		struct nss_ipv4_conn_sync conn_stats;	/**< Message: connection stats sync */
		struct nss_ipv4_node_sync node_stats;	/**< Message: node stats sync */
		struct nss_ipv4_rule_conn_get_table_size_msg size;
				/**< Get the size for connection tables. */
		struct nss_ipv4_rule_conn_cfg_msg rule_conn_cfg;	/**< Message: rule connections supported */
		struct nss_ipv4_mc_rule_create_msg mc_rule_create; /**< Message: Multicast rule create */
		struct nss_ipv4_conn_sync_many_msg conn_stats_many;	/**< Message: connection stats sync */
		struct nss_ipv4_accel_mode_cfg_msg accel_mode_cfg;	/**< Message: Accel mode */
		struct nss_ipv4_dscp2pri_cfg_msg dscp2pri_cfg;	/**< Message: dscp2pri mapping. */
	} msg;
};

extern int nss_ipv6_conn_cfg;

#ifdef __KERNEL__ /* only kernel will use */

/**
 * @brief Return the maximum number of IPv4 connections that the NSS acceleration engine supports
 *
 * @return int The number of connections that can be accelerated by the NSS
 */
int nss_ipv4_max_conn_count(void);

/**
 * Callback to be called when IPv4 message is received
 */
typedef void (*nss_ipv4_msg_callback_t)(void *app_data, struct nss_ipv4_msg *msg);

/**
 * @brief Transmit an IPv4 message to the NSS
 *
 * @param nss_ctx NSS context
 * @param msg The IPv4 message
 *
 * @return nss_tx_status_t The status of the Tx operation
 */
extern nss_tx_status_t nss_ipv4_tx(struct nss_ctx_instance *nss_ctx, struct nss_ipv4_msg *msg);

/**
 * @brief Transmit an IPv4 message to the NSS with specified size
 *
 * @param nss_ctx NSS context
 * @param msg The IPv4 message
 * @param size Actual size of this msg
 *
 * @return nss_tx_status_t The status of the Tx operation
 */
extern nss_tx_status_t nss_ipv4_tx_with_size(struct nss_ctx_instance *nss_ctx, struct nss_ipv4_msg *msg, uint32_t size);

/**
 * @brief Register a notifier callback for IPv4 messages from NSS
 *
 * @param cb The callback pointer
 * @param app_data The application context for this message
 *
 * @return struct nss_ctx_instance * The NSS context
 */
extern struct nss_ctx_instance *nss_ipv4_notify_register(nss_ipv4_msg_callback_t cb, void *app_data);

/**
 * @brief Un-Register a notifier callback for IPv4 messages from NSS
 *
 * @return None
 */
extern void nss_ipv4_notify_unregister(void);

/**
 * @brief Register a notifier callback for IPv4 conn sync messages response from NSS
 *
 * @param cb The callback pointer
 *
 * @return None
 */
extern void nss_ipv4_conn_sync_many_notify_register(nss_ipv4_msg_callback_t cb);

/**
 * @brief Un-Register a notifier callback for IPv4 conn sync messages response from NSS
 *
 * @return None
 */
extern void nss_ipv4_conn_sync_many_notify_unregister(void);

/**
 * @brief Get the NSS context which is managing IPv4
 *
 * @return struct nss_ctx_instance * The NSS context
 */
extern struct nss_ctx_instance *nss_ipv4_get_mgr(void);

/**
 * @brief IPv4 Register Handler
 *
 * @return None
 */
extern void nss_ipv4_register_handler(void);

/**
 * @brief IPv4 sysctl register
 *
 * @return None
 */
extern void nss_ipv4_register_sysctl(void);

/**
 * @brief IPv4 sysctl unregister
 *
 * @return None
 */
extern void nss_ipv4_unregister_sysctl(void);

/**
 * @brief IPv4 message init
 *
 * @return None
 */
extern void nss_ipv4_msg_init(struct nss_ipv4_msg *nim, uint16_t if_num, uint32_t type, uint32_t len,
			nss_ipv4_msg_callback_t cb, void *app_data);

/**
 * @brief Set max number of IPv4 connections.
 *
 * @return 0 on success
 */
extern int nss_ipv4_update_conn_count(int ipv4_max_conn);

/**
 * nss_ipv4_free_conn_tables
 *	Frees memory allocated for connection tables.
 *
 * @return
 * None.
 */
extern void nss_ipv4_free_conn_tables(void);

/**
 * nss_ipv4_dscp_action_get
 *	Gets the action value of the DSCP.
 *
 * @param[in]	dscp	Value of the DSCP field.
 *
 * @return
 * Action value of the DSCP field.
 */
enum nss_ipv4_dscp_map_actions nss_ipv4_dscp_action_get(uint8_t dscp);

/*
 * Logger APIs
 */
void nss_ipv4_log_tx_msg(struct nss_ipv4_msg *nim);
void nss_ipv4_log_rx_msg(struct nss_ipv4_msg *nim);

#endif /*__KERNEL__ */
#endif /* __NSS_IPV4_H */
