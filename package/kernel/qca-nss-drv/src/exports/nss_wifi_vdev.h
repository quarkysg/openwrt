/*
 **************************************************************************
 * Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
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
  * nss_wifivdev.h
  * 	NSS TO HLOS interface definitions.
  */
#ifndef __NSS_WIFI_VDEV_H
#define __NSS_WIFI_VDEV_H

#define NSS_WIFI_HTT_TRANSFER_HDRSIZE_WORD 6
#define NSS_WIFI_VDEV_PER_PACKET_METADATA_OFFSET 4
#define NSS_WIFI_VDEV_DSCP_MAP_LEN 64
#define NSS_WIFI_VDEV_IPV6_ADDR_LENGTH 16
#define NSS_WIFI_MAX_SRCS 4
#define NSS_WIFI_VDEV_MAX_ME_ENTRIES 32

/**
 * WIFI VDEV messages
 */
enum nss_wifi_vdev_msg_types {
	NSS_WIFI_VDEV_INTERFACE_CONFIGURE_MSG = NSS_IF_MAX_MSG_TYPES + 1,
	NSS_WIFI_VDEV_INTERFACE_UP_MSG,
	NSS_WIFI_VDEV_INTERFACE_DOWN_MSG,
	NSS_WIFI_VDEV_INTERFACE_CMD_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_GRP_LIST_CREATE_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_GRP_LIST_DELETE_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_GRP_MEMBER_ADD_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_GRP_MEMBER_REMOVE_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_GRP_MEMBER_UPDATE_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_DENY_MEMBER_ADD_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_DENY_LIST_DELETE_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_DENY_LIST_DUMP_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_DUMP_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_RESET_MSG,
	NSS_WIFI_VDEV_SPECIAL_DATA_TX_MSG,
	NSS_WIFI_VDEV_VOW_DBG_CFG_MSG,
	NSS_WIFI_VDEV_VOW_DBG_STATS_REQ_MSG,
	NSS_WIFI_VDEV_DSCP_TID_MAP_MSG,
	NSS_WIFI_VDEV_SNOOPLIST_TOGGLE_MSG,
	NSS_WIFI_VDEV_UPDATECHDR_MSG,
	NSS_WIFI_VDEV_ME_SYNC_MSG,
	NSS_WIFI_VDEV_STATS_MSG,
	NSS_WIFI_VDEV_SET_NEXT_HOP,
	NSS_WIFI_VDEV_MAX_MSG
};

/**
 * wifi vdev error types
 */
enum {
	NSS_WIFI_VDEV_ENONE,				/**< no error */
	NSS_WIFI_VDEV_EUNKNOWN_MSG,			/**< unknown message */
	NSS_WIFI_VDEV_EINV_VID_CONFIG,			/**< error in vid configuration information passed */
	NSS_WIFI_VDEV_EINV_EPID_CONFIG,			/**< error in epid configuration information passed */
	NSS_WIFI_VDEV_EINV_DL_CONFIG,			/**< error in download length configuration information passed */
	NSS_WIFI_VDEV_EINV_CMD,				/**< unknown command */
	NSS_WIFI_VDEV_EINV_ENCAP,			/**< invalid encap mode for vap */
	NSS_WIFI_VDEV_EINV_DECAP,			/**< invalid decap mode for vap */
	NSS_WIFI_VDEV_EINV_RX_NXTN,			/**< invalid next hop(node) in rx path */
	NSS_WIFI_VDEV_EINV_VID_INDEX,			/**< error in vid index */
	NSS_WIFI_VDEV_EINV_MC_CFG,			/**< error in multicast config */
	NSS_WIFI_VDEV_SNOOPTABLE_FULL,			/**< snooptable full error */
	NSS_WIFI_VDEV_SNOOPTABLE_ENOMEM,		/**< error in allocating memory for snooptable */
	NSS_WIFI_VDEV_SNOOPTABLE_GRP_LIST_UNAVAILABLE,	/**< grp_list is unavailable in snooplist */
	NSS_WIFI_VDEV_SNOOPTABLE_GRP_MEMBER_UNAVAILABLE,/**< grp_member is unavailable in grp_list inside snooplist */
	NSS_WIFI_VDEV_SNOOPTABLE_PEER_UNAVAILABLE,	/**< peer is unavailable */
	NSS_WIFI_VDEV_SNOOPTABLE_GRP_LIST_ENOMEM,	/**< error in allocating memory for grplist in snooptable */
	NSS_WIFI_VDEV_SNOOPTABLE_GRP_LIST_EXIST,	/**< grp_list already exists in snooplist */
	NSS_WIFI_VDEV_ME_ENOMEM,			/**< error in allocating memory for multicast enhancement instance */
	NSS_WIFI_VDEV_EINV_NAWDS_CFG,			/**< error in nawds config */
	NSS_WIFI_VDEV_EINV_EXTAP_CFG,			/**< error in extap config */
	NSS_WIFI_VDEV_EINV_VOW_DBG_CFG,			/**< error in VOW Debug stats config */
	NSS_WIFI_VDEV_EINV_DSCP_TID_MAP,		/**< inavlid dscp to tid map message */
	NSS_WIFI_VDEV_INVALID_ETHER_TYPE,		/**< Ether type is invalid */
	NSS_WIFI_VDEV_SNOOPTABLE_GRP_MEMBER_EXIST,	/**< grp member already exists in snooplist */
	NSS_WIFI_VDEV_ME_INVALID_NSRCS,			/**< invalid number of sources received from host */
	NSS_WIFI_VDEV_EINV_RADIO_ID,			/**< Invalid wifi radio id */
	NSS_WIFI_VDEV_RADIO_NOT_PRESENT,		/**< Radio pnode is not present */
	NSS_WIFI_VDEV_CHDRUPD_FAIL,			/**< Unable to update cached hdr */
	NSS_WIFI_VDEV_ME_DENY_GRP_MAX_RCHD,		/**< Unable to add new entry to deny group */
	NSS_WIFI_VDEV_EINV_NEXT_HOP,			/**< Unable to set next hop */
	NSS_WIFI_VDEV_EINV_MAX_CFG
};

/**
 * Extended data plane pkt types sent from NSS to host.
 */
enum nss_wifi_vdev_ext_data_pkt_type {
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_NONE = 0,
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_IGMP = 1,	/**< igmp packets */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_MESH = 2,	/**< mesh packets */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_INSPECT = 3,	/**< host inspect packets */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_TXINFO = 4,	/**< tx completion info packets */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_MPSTA_TX = 5,	/**< mpsta tx meta data */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_MPSTA_RX = 6,	/**< mpsta rx meta data */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_RX_ERR = 7,	/**< rx error packets meta data */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_EXTAP_TX = 8,	/**< extap tx meta data */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_EXTAP_RX = 9,	/**< extap rx meta data */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_WNM_TFS = 10,	/**< wnm tfs related meta data */
	NSS_WIFI_VDEV_EXT_TX_COMPL_PKT_TYPE = 11,	/**< tx completion */
	NSS_WIFI_VDEV_EXT_DATA_PKT_TYPE_MAX
};

/**
 * wifi vdev commands
 */
enum nss_wifi_vdev_cmd {
	NSS_WIFI_VDEV_DROP_UNENC_CMD,		/**< command to set/reset encryption on vap */
	NSS_WIFI_VDEV_ENCAP_TYPE_CMD,		/**< command to configure encap mode of vap */
	NSS_WIFI_VDEV_DECAP_TYPE_CMD,		/**< command to configure decap mode of vap */
	NSS_WIFI_VDEV_ENABLE_ME_CMD,		/**< command to enable multicast enhancement */
	NSS_WIFI_VDEV_NAWDS_MODE_CMD,		/**< command to configure NAWDS on vap */
	NSS_WIFI_VDEV_EXTAP_CONFIG_CMD,		/**< command to configure EXTAP mode on ap */
	NSS_WIFI_VDEV_CFG_BSTEER_CMD,		/**< command to configure BSTEER related reporting on vap */
	NSS_WIFI_VDEV_VOW_DBG_MODE_CMD,		/**< command to enable VOW DEBUG on vap */
	NSS_WIFI_VDEV_VOW_DBG_RST_STATS_CMD,	/**< command to reset VOW DEBUG stats on vap */
	NSS_WIFI_VDEV_CFG_DSCP_OVERRIDE_CMD,	/**< command to set dscp override */
	NSS_WIFI_VDEV_CFG_WNM_CAP_CMD,		/**< command to set wnm capability */
	NSS_WIFI_VDEV_CFG_WNM_TFS_CMD,		/**< command to set wnm tfs */
	NSS_WIFI_VDEV_CFG_WDS_EXT_ENABLE_CMD,	/**< command to enable wds vendor extension */
	NSS_WIFI_VDEV_MAX_CMD
};

/**
 * wifi vdev config message
 */
struct nss_wifi_vdev_config_msg {
	uint8_t mac_addr[ETH_ALEN];	/**< MAC address */
	uint16_t radio_ifnum;		/**< Corresponding radio interface number */
	uint32_t vdev_id;		/**< vap id */
	uint32_t epid;			/**< CE endpoint id */
	uint32_t downloadlen;		/**< Header download length */
	uint32_t hdrcachelen;		/**< Header cache length */
	uint32_t hdrcache[NSS_WIFI_HTT_TRANSFER_HDRSIZE_WORD];
	uint32_t opmode;		/**< VAP Opmode - AP or STA? */
	uint32_t mesh_mode_en;		/**< Mesh mode enabled */
	uint8_t is_mpsta;		/**< is this vap is mpsta */
	uint8_t is_psta;		/**< this is a proxy station*/
	uint8_t special_vap_mode;	/**< special vap used for monitoring rx mgmt packets */
	uint8_t smartmesh_mode_en;	/**< vap is configures as smart monitor vap */
};

/**
 * wifi vdev enable message
 */
struct nss_wifi_vdev_enable_msg {
	uint8_t mac_addr[ETH_ALEN];	/**< MAC address */
	uint8_t reserved[2];		/**< reserved bytes */
};

/**
 * wifi vdev disable message
 */
struct nss_wifi_vdev_disable_msg {
	uint32_t reserved;		/**< place holder */
};

/** wifi vdev command message
 *
 */
struct nss_wifi_vdev_cmd_msg {
	uint32_t cmd;				/**< command type */
	uint32_t value;				/**< command value */
};

/**
 * Set wifi vdev set next hop
 */
struct nss_wifi_vdev_set_next_hop_msg {
	uint32_t ifnumber;	/**< next hop interface number */
};

/**
 * wifi snooplist create grp_list
 */
struct nss_wifi_vdev_me_snptbl_grp_create_msg {
	uint32_t ether_type;					/**< multicast group ether_type */
	union {
		uint32_t grpaddr_ip4;				/**< ipv4 address */
		uint8_t grpaddr_ip6[NSS_WIFI_VDEV_IPV6_ADDR_LENGTH];	/**< ipv6 address */
	} u;							/**< multicast group ip address */
	uint8_t grp_addr[ETH_ALEN];				/**< multicast group mac address */
};

/**
 * wifi snooplist delete grp_list
 */
struct nss_wifi_vdev_me_snptbl_grp_delete_msg {
	uint32_t ether_type;					/**< multicast group ether_type */
	union {
		uint32_t grpaddr_ip4;				/**< ipv4 address */
		uint8_t grpaddr_ip6[NSS_WIFI_VDEV_IPV6_ADDR_LENGTH];	/**< ipv6 address */
	} u;							/**< multicast group ip address */
	uint8_t grp_addr[ETH_ALEN];				/**< multicast group mac address */
};

/**
 * wifi snooplist add grp_member
 */
struct nss_wifi_vdev_me_snptbl_grp_mbr_add_msg {
	uint32_t ether_type;					/**< multicast group ether_type */
	union {
		uint32_t grpaddr_ip4;				/**< ipv4 address */
		uint8_t grpaddr_ip6[NSS_WIFI_VDEV_IPV6_ADDR_LENGTH];	/**< ipv6 address */
	} u;							/**< multicast group ip address */
	uint32_t peer_id;					/**< peer_id */
	uint8_t grp_addr[ETH_ALEN];				/**< multicast group mac address */
	uint8_t grp_member_addr[ETH_ALEN];			/**< multicast group member mac address */
	uint8_t mode;						/**< mode */
	uint8_t nsrcs;						/**< no of src ip addresses in case of SSM */
	uint8_t src_ip_addr[NSS_WIFI_VDEV_IPV6_ADDR_LENGTH * NSS_WIFI_MAX_SRCS];	/**< source ip address */
};

/**
 * wifi snooplist remove grp_member
 */
struct nss_wifi_vdev_me_snptbl_grp_mbr_delete_msg {
	uint32_t ether_type;					/**< multicast group ether_type */
	union {
		uint32_t grpaddr_ip4;				/**< ipv4 address */
		uint8_t grpaddr_ip6[NSS_WIFI_VDEV_IPV6_ADDR_LENGTH];	/**< ipv6 address */
	}u;							/**< multicast group ip address */
	uint8_t grp_addr[ETH_ALEN];				/**< multicast group mac address */
	uint8_t grp_member_addr[ETH_ALEN];			/**< multicast group member mac address */
};

/**
 * Wifi snooplist update grp_member.
 */
struct nss_wifi_vdev_me_snptbl_grp_mbr_update_msg {
	uint32_t ether_type;					/**< multicast group ether_type */
	union {
		uint32_t grpaddr_ip4;				/**< ipv4 address */
		uint8_t grpaddr_ip6[NSS_WIFI_VDEV_IPV6_ADDR_LENGTH];	/**< ipv6 address */
	}u;							/**< multicast group ip address */
	uint8_t grp_addr[ETH_ALEN];				/**< multicast group mac address */
	uint8_t grp_member_addr[ETH_ALEN];			/**< multicast group member mac address */
	uint8_t mode;						/**< mode */
	uint8_t nsrcs;						/**< no of src ip addresses in case of SSM */
	uint8_t src_ip_addr[NSS_WIFI_VDEV_IPV6_ADDR_LENGTH * NSS_WIFI_MAX_SRCS];	/**< source ip address */
};

/**
 * Wifi snooplist add member to deny list.
 */
struct nss_wifi_vdev_me_snptbl_deny_grp_add_msg {
	uint32_t grpaddr;			/**< multicast group ip address */
};

/**
 * WiFi Tx special data message
 */
struct nss_wifi_vdev_txmsg {
	uint16_t peer_id;	/**< peer id */
	uint16_t tid;		/**< tid */
};

/**
 * Wifi VoW debug stats config.
 */
struct nss_wifi_vdev_vow_dbg_stats {
	uint32_t rx_vow_dbg_counters;		/**< VoW rx debug counter */
	uint32_t tx_vow_dbg_counters[8];	/**< VoW tx debug counter */
};

/**
 * Wifi VoW debug stats config.
 */
struct nss_wifi_vdev_vow_dbg_cfg_msg {
	uint8_t vow_peer_list_idx;
	uint8_t tx_dbg_vow_peer_mac4;
	uint8_t tx_dbg_vow_peer_mac5;
};

/**
 * Wifi dscp to tid mapping.
 */
struct nss_wifi_vdev_dscp_tid_map {
	uint32_t dscp_tid_map[NSS_WIFI_VDEV_DSCP_MAP_LEN];
				/**< array holding the dscp to tid mapping */
};

/**
 * Wifi per packet metadata for IGMP packets.
 */
struct nss_wifi_vdev_igmp_per_packet_metadata {
	uint32_t tid;				/**< tid value */
	uint32_t tsf32;				/**< tsf value */
	uint8_t peer_mac_addr[ETH_ALEN];	/**< peer mac address */
	uint8_t reserved[2];			/**< reserved */
};

/**
 * Wifi per packet metadata for MESH packets.
 */
struct nss_wifi_vdev_mesh_per_packet_metadata {
	uint32_t status;			/**< status */
	uint32_t rssi;				/**< rssi */
};

/**
 * Wifi per packet metadata for Tx completion info packets
 */
struct nss_wifi_vdev_txinfo_per_packet_metadata {
	uint32_t status;			/**< tx completion status */
	uint16_t msdu_count;			/**< count of msdu in msdu list */
	uint16_t num_msdu;			/**< number of msdu in msdu list */
	uint32_t msdu_q_time;			/**< time spend by msdu in wifi fw */
	uint32_t ppdu_rate;			/**< ppdu rate in ratecode */
	uint8_t ppdu_num_mpdus_success;		/**< number of successful mpdus */
	uint8_t ppdu_num_mpdus_fail;		/**< number of failed mpdus */
	uint16_t ppdu_num_msdus_success;	/**< number of successfull msdus */
	uint32_t ppdu_bytes_success;		/**< number of successfull bytes */
	uint32_t ppdu_duration;			/**< ppdu estimated air time */
	uint8_t ppdu_retries;			/**< number of times ppdu is retried */
	uint8_t ppdu_is_aggregate;		/**< flag to chack if ppdu is aggregate or not */
	uint16_t start_seq_num;			/**< starting msdu id for this ppdu */
	uint16_t version;				/**< ppdu stats version */
	uint32_t ppdu_ack_timestamp;	/**< Timestamp(us) when ack was received */
	uint32_t ppdu_bmap_enqueued_lo;	/**< Bitmap of packets enqueued to HW (LSB) */
	uint32_t ppdu_bmap_enqueued_hi;	/**< Bitmap of packets enqueued to HW (MSB) */
	uint32_t ppdu_bmap_tried_lo;	/**< Bitmap of packets sent OTA (LSB) */
	uint32_t ppdu_bmap_tried_hi;	/**< Bitmap of packets sent OTA (MSB) */
	uint32_t ppdu_bmap_failed_lo;	/**< Bitmap of packets failed to get acked (LSB) */
	uint32_t ppdu_bmap_failed_hi;	/**< Bitmap of packets failed to get acked (MSB) */
};

/**
 * wifi per packet metadata types for qwrap tx packets
 */
enum nss_wifi_vdev_qwrap_tx_metadata_types {
	NSS_WIFI_VDEV_QWRAP_TYPE_NONE = 0,	/**< qwrap type none */
	NSS_WIFI_VDEV_QWRAP_TYPE_TX = 1,	/**< qwrap tx frame to be sent over wifi */
	NSS_WIFI_VDEV_QWRAP_TYPE_RX_TO_TX = 2	/**< qwrap rx to tx frame to be sent over eth_rx */
};

/**
 * wifi per packet metadata types for extap tx packets
 */
enum nss_wifi_vdev_extap_pkt_types {
	NSS_WIFI_VDEV_EXTAP_PKT_TYPE_NONE = 0,		/**< extap pkt type none */
	NSS_WIFI_VDEV_EXTAP_PKT_TYPE_TX = 1,		/**< extap tx frame to be sent over wifi */
	NSS_WIFI_VDEV_EXTAP_PKT_TYPE_RX_TO_TX = 2	/**< extap rx to tx frame to be sent over eth_rx */
};

/**
 * wifi transmit meta data for mpsta
 */
struct nss_wifi_vdev_mpsta_per_packet_tx_metadata {
	uint16_t vdev_id;		/**< vdev_id */
	uint16_t metadata_type;		/**< tx metadata type */
};

/**
 * wifi recieve meta data for mpsta
 */
struct nss_wifi_vdev_mpsta_per_packet_rx_metadata {
	uint16_t vdev_id;		/**< vdev_id */
	uint16_t peer_id;		/**< peer_id */
};

/*
 * Wifi per packet metadata for rx_err packets.
 */
struct nss_wifi_vdev_rx_err_per_packet_metadata {
	uint8_t peer_mac_addr[ETH_ALEN];	/*< peer mac address */
	uint8_t tid;    			/*< tid */
	uint8_t vdev_id;			/*< vdev id */
	uint8_t err_type;			/*< Error Type */
	uint8_t rsvd[3];			/*< Reserved for alignment */
};

/*
 * Wifi extap per packet metadata
 */
struct nss_wifi_vdev_extap_per_packet_metadata {
	uint16_t pkt_type;	/**< extap pkt type */
	uint8_t res[2];		/**< res */
};

/**
 * Wifi Tx Compl per packet metadata.
 */
struct nss_wifi_vdev_tx_compl_metadata {
	uint8_t ta[ETH_ALEN];	/**< transmitter mac address */
	uint8_t ra[ETH_ALEN];	/**< receiver mac address */
	uint16_t ppdu_id;	/**< ppdu id */
	uint16_t peer_id;	/**< peer id */
};

/**
 * wifi per packet metadata content
 */
struct nss_wifi_vdev_per_packet_metadata {
	uint32_t pkt_type;
	union {
		struct nss_wifi_vdev_igmp_per_packet_metadata igmp_metadata;
		struct nss_wifi_vdev_mesh_per_packet_metadata mesh_metadata;
		struct nss_wifi_vdev_txinfo_per_packet_metadata txinfo_metadata;
		struct nss_wifi_vdev_mpsta_per_packet_tx_metadata mpsta_tx_metadata;
		struct nss_wifi_vdev_mpsta_per_packet_rx_metadata mpsta_rx_metadata;
		struct nss_wifi_vdev_rx_err_per_packet_metadata rx_err_metadata;
		struct nss_wifi_vdev_tx_compl_metadata tx_compl_metadata;
	} metadata;
};

/**
 * wifi receive meta data for meshmode rx
 */
struct nss_wifi_vdev_meshmode_rx_metadata {
	uint16_t vdev_id;	/**< vdev_id */
	uint16_t peer_id;	/**< peer_id */
};

/**
 * wifi receive meta data for rawmode rx
 */
struct nss_wifi_vdev_rawmode_rx_metadata {
	uint16_t vdev_id;	/**< vdev_id */
	uint16_t peer_id;	/**< peer_id */
};

/**
 * wifi update cache hdr msg
 */
struct nss_wifi_vdev_updchdr_msg {
	uint32_t hdrcache[NSS_WIFI_HTT_TRANSFER_HDRSIZE_WORD];
					/**< Updated hdr cache */
	uint32_t vdev_id;		/**< vdev_id */
};

/**
 * wifi_vdev_me_host_sync_grp_entry
 *
 * ME Group table host sync
 */
struct nss_wifi_vdev_me_host_sync_grp_entry {
	uint8_t group_addr[ETH_ALEN];		/**< group address for this list*/
	uint8_t grp_member_addr[ETH_ALEN];	/**< multicast group member mac address */
	union {
		uint32_t grpaddr_ip4;
		uint8_t  grpaddr_ip6[NSS_WIFI_VDEV_IPV6_ADDR_LENGTH];
	} u;
	uint32_t src_ip_addr;	/**< source ip address */
};

/**
 * wifi_vdev_me_host_sync_msg
 *
 * ME Group table host sync message
 */
struct nss_wifi_vdev_me_host_sync_msg {
	uint16_t vdev_id;	/**< vap id */
	uint8_t nentries;	/**< number of group entries carried by this msg */
	uint8_t radio_ifnum;	/**< wifi radio if_num */
	struct nss_wifi_vdev_me_host_sync_grp_entry grp_entry[NSS_WIFI_VDEV_MAX_ME_ENTRIES];
};

/**
 * nss_wifi_vdev_mcast_enhance_stats
 *
 * Multicast enhancement related statistics
 */
struct nss_wifi_vdev_mcast_enhance_stats {
	uint32_t mcast_rcvd;			/**< number of mcast pkts recieved for conversion for mc enhancement*/
	uint32_t mcast_ucast_converted;		/**< number of unicast pkts sent as part of mc enhancement conversion */
	uint32_t mcast_alloc_fail;		/**< number of mcast enhancement frames dropped due to pbuf allocation failure */
	uint32_t mcast_pbuf_enq_fail;		/**< number of mcast enhancement frames dropped due to pbuf enqueue failure */
	uint32_t mcast_pbuf_copy_fail;		/**< number of mcast enhancement frames dropped due to pbuf copy failure */
	uint32_t mcast_peer_flow_ctrl_send_fail;/**< number of mcast enhancement frames dropped due to peer flow ctrl send failure */
	uint32_t mcast_loopback_err;		/**< number of mcast enhancement buf frames dropped when dst_mac is the same as src_mac */
	uint32_t mcast_dst_address_err;		/**< number of mcast enhancement buf frames dropped due to empty dst_mac */
	uint32_t mcast_no_enhance_drop_cnt;	/**< number of mcast enhancement buf frames dropped due to no memebers listening on group */
};

/**
 * wifi_vdev_stats_sync_msg
 *
 * vdev statistics sync message
 */
struct nss_wifi_vdev_stats_sync_msg {
	uint32_t dropped;				/**< dropped packet count */
	struct nss_wifi_vdev_mcast_enhance_stats wvmes;	/**< multicast enhancement stats */
};

/**
 * wifi vdev specific messages
 */
struct nss_wifi_vdev_msg {
	struct nss_cmn_msg cm;          /* Message Header */
	union {
		struct nss_wifi_vdev_config_msg vdev_config;
		struct nss_wifi_vdev_enable_msg vdev_enable;
		struct nss_wifi_vdev_cmd_msg vdev_cmd;
		struct nss_wifi_vdev_me_snptbl_grp_create_msg vdev_grp_list_create;
		struct nss_wifi_vdev_me_snptbl_grp_delete_msg vdev_grp_list_delete;
		struct nss_wifi_vdev_me_snptbl_grp_mbr_add_msg vdev_grp_member_add;
		struct nss_wifi_vdev_me_snptbl_grp_mbr_delete_msg vdev_grp_member_remove;
		struct nss_wifi_vdev_me_snptbl_grp_mbr_update_msg vdev_grp_member_update;
		struct nss_wifi_vdev_me_snptbl_deny_grp_add_msg vdev_deny_member_add;
		struct nss_wifi_vdev_txmsg vdev_txmsgext;
		struct nss_wifi_vdev_vow_dbg_cfg_msg vdev_vow_dbg_cfg;
		struct nss_wifi_vdev_vow_dbg_stats vdev_vow_dbg_stats;
		struct nss_wifi_vdev_dscp_tid_map vdev_dscp_tid_map;
		struct nss_wifi_vdev_updchdr_msg vdev_updchdr;
		struct nss_wifi_vdev_me_host_sync_msg vdev_me_sync;
		struct nss_wifi_vdev_stats_sync_msg vdev_stats;
		struct nss_wifi_vdev_set_next_hop_msg next_hop;
	} msg;
};

/**
 * @brief Send WIFI message to NSS
 *
 * @param nss_ctx NSS core context
 * @param msg  NSS wifi message
 *
 * @return status
 */
nss_tx_status_t nss_wifi_vdev_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_wifi_vdev_msg *msg);

/**
 * @brief Send WIFI data packet to NSS
 *
 * @param nss_ctx NSS core context
 * @param os_buf data buffer
 * @param if_num NSS interface number
 *
 * @return status
 */
nss_tx_status_t nss_wifi_vdev_tx_buf(struct nss_ctx_instance *nss_ctx, struct sk_buff *os_buf, uint32_t if_num);

/**
 * @brief Callback to receive wifi vdev messages
 *
 * @param app_data Application context of the message
 * @param msg Message data
 *
 * @return void
 */
typedef void (*nss_wifi_vdev_msg_callback_t)(void *app_data, struct nss_cmn_msg *msg);

/**
 * @brief Callback to receive wifi vdev data
 *
 * @param app_data Application context of the message
 * @param os_buf Pointer to data buffer
 *
 * @return void
 */
typedef void (*nss_wifi_vdev_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief Callback to receive extended data plane wifi vdev data
 *
 * @param app_data Application context of the message
 * @param os_buf  Pointer to data buffer
 *
 * @return void
 */
typedef void (*nss_wifi_vdev_ext_data_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief nss_wifi messages init function
 *
 * @return void
 */
void nss_wifi_vdev_msg_init(struct nss_wifi_vdev_msg *nim, uint16_t if_num, uint32_t type, uint32_t len,
				nss_wifi_vdev_msg_callback_t *cb, void *app_data);

/**
 * @brief Register wifi Vdev with NSS
 *
 * @param if_num NSS interface number
 * @param wifi_data_callback Callback for wifi vdev data
 * @param wifi_event_callback Callback for wifi vdev messages
 * @param netdev netdevice associated with the wifi vdev
 *
 * @return status
 */
uint32_t nss_register_wifi_vdev_if(struct nss_ctx_instance *nss_ctx, int32_t if_num, nss_wifi_vdev_callback_t wifi_data_callback,
			nss_wifi_vdev_ext_data_callback_t vdev_ext_data_callback, nss_wifi_vdev_msg_callback_t wifi_event_callback,
			struct net_device *netdev, uint32_t features);

/**
 * @brief Unregister wifi vdev interface with NSS
 *
 * @param if_num NSS interface number
 *
 * @return void
 */
void nss_unregister_wifi_vdev_if(uint32_t if_num);

/**
 * @brief Send WIFI data packet along with metadata as msg to NSS
 *
 * @param nss_ctx NSS core context
 * @param os_buf data buffer
 * @param msg message
 *
 * @return status
 */
nss_tx_status_t nss_wifi_vdev_tx_msg_ext(struct nss_ctx_instance *nss_ctx, struct sk_buff *os_buf);

/**
 * nss_wifi_vdev_set_next_hop
 *	Send next hop message to WIFI vdev.
 *
 * @datatypes
 * nss_ctx_instance
 *
 * @param[in] nss_ctx   Pointer to the NSS core context.
 * @param[in] if_num    Nss interface number.
 * @param[in] next_hop  Next hop interface number.
 *
 * @return
 * Status of the Tx operation.
 */
nss_tx_status_t nss_wifi_vdev_set_next_hop(struct nss_ctx_instance *nss_ctx, int if_num, int next_hop);

/**
 * @}
 */

#endif /* __NSS_WIFI_VDEV_H */
