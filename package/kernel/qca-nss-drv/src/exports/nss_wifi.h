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
  * nss_wifi.h
  * 	NSS TO HLOS interface definitions.
  */

#ifndef __NSS_WIFI_H
#define __NSS_WIFI_H

#define NSS_WIFI_MGMT_DATA_LEN  128
#define NSS_WIFI_FW_STATS_DATA_LEN  480
#define NSS_WIFI_RAWDATA_MAX_LEN  64
#define NSS_WIFI_RX_EXT_INV_PEER_TYPE 0
#define NSS_WIFI_RX_EXT_PKTLOG_TYPE 1
#define NSS_WIFI_TX_NUM_TOS_TIDS 8
#define NSS_WIFI_PEER_STATS_DATA_LEN 232
#define NSS_WIFI_IPV6_ADDR_LEN 16

/**
 * max no of wifi peers per radio is the sum of max no of station peers (513),
 * max no of AP vap peers(16), and max no of monitor vap peers (1)
 */
#define NSS_WIFI_MAX_PEER 530

/**
 * wifi interface request/response types
 */
enum nss_wifi_metadata_types {
	NSS_WIFI_INIT_MSG,
	NSS_WIFI_POST_RECV_MSG,
	NSS_WIFI_HTT_INIT_MSG,
	NSS_WIFI_TX_INIT_MSG,
	NSS_WIFI_RAW_SEND_MSG,
	NSS_WIFI_MGMT_SEND_MSG,
	NSS_WIFI_WDS_PEER_ADD_MSG,
	NSS_WIFI_WDS_PEER_DEL_MSG,
	NSS_WIFI_STOP_MSG,
	NSS_WIFI_RESET_MSG,
	NSS_WIFI_STATS_MSG,
	NSS_WIFI_PEER_FREELIST_APPEND_MSG,
	NSS_WIFI_RX_REORDER_ARRAY_FREELIST_APPEND_MSG,
	NSS_WIFI_SEND_PEER_MEMORY_REQUEST_MSG,
	NSS_WIFI_SEND_RRA_MEMORY_REQUEST_MSG,
	NSS_WIFI_FW_STATS_MSG,
	NSS_WIFI_MONITOR_FILTER_SET_MSG,
	NSS_WIFI_PEER_BS_STATE_MSG,
	NSS_WIFI_MSDU_TTL_SET_MSG,
	NSS_WIFI_RX_VOW_EXTSTATS_SET_MSG,
	NSS_WIFI_PKTLOG_CFG_MSG,
	NSS_WIFI_ENABLE_PERPKT_TXSTATS_MSG,
	NSS_WIFI_IGMP_MLD_TOS_OVERRIDE_MSG,
	NSS_WIFI_OL_STATS_CFG_MSG,
	NSS_WIFI_OL_STATS_MSG,
	NSS_WIFI_TX_QUEUE_CFG_MSG,
	NSS_WIFI_TX_MIN_THRESHOLD_CFG_MSG,
	NSS_WIFI_DBDC_PROCESS_ENABLE_MSG,
	NSS_WIFI_PRIMARY_RADIO_SET_MSG,
	NSS_WIFI_FORCE_CLIENT_MCAST_TRAFFIC_SET_MSG,
	NSS_WIFI_STORE_OTHER_PDEV_STAVAP_MSG,
	NSS_WIFI_STA_KICKOUT_MSG,
	NSS_WIFI_WNM_PEER_RX_ACTIVITY_MSG,
	NSS_WIFI_PEER_STATS_MSG,
	NSS_WIFI_ME_SYNC_MSG,
	NSS_WIFI_MAX_MSG
};

/*
 * wifi msg error types
 */
enum wifi_error_types {
	NSS_WIFI_EMSG_NONE = 0,
	NSS_WIFI_EMSG_UNKNOWN,
	NSS_WIFI_EMSG_MGMT_DLEN,			/**< invalid management data length */
	NSS_WIFI_EMSG_MGMT_SEND,			/**< error in sending management data */
	NSS_WIFI_EMSG_CE_INIT_FAIL,			/**< error in ce init */
	NSS_WIFI_EMSG_PDEV_INIT_FAIL,			/**< error in pdev init */
	NSS_WIFI_EMSG_HTT_INIT_FAIL,			/**< error in htt dev init */
	NSS_WIFI_EMSG_PEER_ADD,					/**< error in wds peer add */
	NSS_WIFI_EMSG_TARGET_NOT_SUPPORTED,		/**< invalid traget type passed */
	NSS_WIFI_EMSG_STATE_NOT_RESET,			/**< reset failed */
	NSS_WIFI_EMSG_STATE_NOT_INIT_DONE,		/**< init failed */
	NSS_WIFI_EMSG_STATE_NULL_CE_HANDLE,		/**< invalid ce handle */
	NSS_WIFI_EMSG_STATE_NOT_CE_READY,		/**< ce is not ready */
	NSS_WIFI_EMSG_STATE_NOT_HTT_READY,		/**< htt is not ready */
	NSS_WIFI_EMSG_FW_STATS_DLEN,			/**< invalid wifi fw stats data length */
	NSS_WIFI_EMSG_FW_STATS_SEND,			/**< error in sending wifi fw stats data */
	NSS_WIFI_EMSG_STATE_TX_INIT_FAILED,		/**< Tx init failed */
	NSS_WIFI_EMSG_IGMP_MLD_TOS_OVERRIDE_CFG,/**< Invalid IGMP/MLD tos override config */
	NSS_WIFI_EMSG_PDEV_INVALID,			/**< Invalid pdev */
	NSS_WIFI_EMSG_OTHER_PDEV_STAVAP_INVALID,	/**< Invalid ifnum for other pdev stavap */

};

/*
 * Copy engine ring internal state
 */
struct nss_wifi_ce_ring_state_msg {
	uint32_t nentries;			/**< Number of entries in the CE ring */
	uint32_t nentries_mask;			/**< Number of entry mask */
	uint32_t sw_index;			/**< Initial SW index start*/
	uint32_t write_index;			/**< Initial write index start */
	uint32_t hw_index;			/**< Initial h/w index */
	uint32_t base_addr_CE_space;		/**< CE h/w ring physical address */
	uint32_t base_addr_owner_space;		/**< CE h/w ring virtual  address */
};

/**
 *  Copy engine internal state
 */
struct nss_wifi_ce_state_msg {
	struct nss_wifi_ce_ring_state_msg src_ring;
						/**< Source ring info */
	struct nss_wifi_ce_ring_state_msg dest_ring;
						/**< Destination ring info */
	uint32_t ctrl_addr;			/**< Relative to BAR */
};

/**
 * wifi init message
 */
struct nss_wifi_init_msg {
	uint32_t radio_id ;			/**< Radio index */
	uint32_t pci_mem;			/**< PCI memory  address */
	uint32_t target_type;			/**< WiFi Target type */
	uint32_t mu_mimo_enhancement_en;	/**< enable mu mimo enhancement */
	struct nss_wifi_ce_state_msg ce_tx_state;
						/**< Transmit CE info */
	struct nss_wifi_ce_state_msg ce_rx_state;
						/**< Recieve CE info */
	uint32_t bypass_nw_process;		/**< Is nw processing to be bypassed in NSS for this radio */
};

/**
 * wifi htt init configuration data
 */
struct nss_wifi_htt_init_msg {
	uint32_t radio_id;			/**< Radio Index */
	uint32_t ringsize;			/**< WLAN h/w mac ring size */
	uint32_t fill_level;			/**< Initial fill_level */
	uint32_t paddrs_ringptr;		/**< Phyical address of WLAN mac h/w ring */
	uint32_t paddrs_ringpaddr;		/**< Virtual  address of WLAN mac h/w ring */
	uint32_t alloc_idx_vaddr;		/**< Virtual addres of h/w Ring Index */
	uint32_t alloc_idx_paddr;		/**< Physical address of h/w ring index */
};

/**
 * wifi tx init configuration data
 */
struct nss_wifi_tx_init_msg {
	uint32_t radio_id;			/**< Radio Index */
	uint32_t desc_pool_size;		/**< Number of descripor  pool allocated */
	uint32_t tx_desc_array;			/**< Host initialized s/w wlan desc pool memory */
	uint32_t wlanextdesc_addr;		/**< WLAN Mac Extenstion descriptor pool starting address*/
	uint32_t wlanextdesc_size;		/**< WLAN Mac Extenstion descriptor size*/
	uint32_t htt_tx_desc_base_vaddr;	/**< Firmware shared HTT trasmit desc memory start virtual addres */
	uint32_t htt_tx_desc_base_paddr; 	/**< Firmware shared HTT trasmit desc memory start physical address */
	uint32_t htt_tx_desc_offset; 		/**< Firmware shared HTT trasmit each desc size */
	uint32_t pmap_addr;			/**< Firmware shared peer/TID map */
};

/*
 * wifi tx queue configuration data
 */
struct nss_wifi_tx_queue_cfg_msg {
	uint32_t size;          /**< Tx queue size */
	uint32_t range;          /**< Peer Range */
};

/*
 * wifi tx queuing min threshold configuration
 */
struct nss_wifi_tx_min_threshold_cfg_msg {
	uint32_t min_threshold;          /**< Minimum threshold for Tx queuing */
};


/**
 * wifi raw data send message structure
 */
struct nss_wifi_rawsend_msg {
	uint32_t radio_id ;			/**< Radio Index */
	uint32_t len;				/**< Length of the raw data */
	uint32_t array[NSS_WIFI_RAWDATA_MAX_LEN];
						/**< Raw data */
};

/**
 *  wifi management data message structure
 */
struct nss_wifi_mgmtsend_msg {
	uint32_t desc_id;			/**< Radio Index */
	uint32_t len;				/**< Length of the management data */
	uint8_t array[NSS_WIFI_MGMT_DATA_LEN];
						/**< Management data */
};

/**
 *  wifi fw-stats data message structure
 */
struct nss_wifi_fw_stats_msg {
	uint32_t len;					/**< Length of the stats data */
	uint8_t array[NSS_WIFI_FW_STATS_DATA_LEN];	/**< Stats data */
};

/**
 *  wifi monitor mode set filter message structure
 */
struct nss_wifi_monitor_set_filter_msg {
	uint32_t filter_type;			/**< filter type */
};

/**
 * wifi pdev wds peer specific messages
 */
struct nss_wifi_wds_peer_msg {
	uint8_t dest_mac[ETH_ALEN];		/**< Mac address of destination */
	uint8_t reserved[2];
	uint8_t peer_mac[ETH_ALEN];		/**< Mac address of base peer */
	uint8_t reserved1[2];
};

/**
 * wifi reset message
 */
struct nss_wifi_reset_msg {
	uint32_t radio_id;			/**< Radio index */
};

/**
 * wifi stop message
 */
struct nss_wifi_stop_msg {
	uint32_t radio_id;			/**< Radio index */
};

/**
 * wifi pktlog cfg message
 */
struct nss_wifi_pktlog_cfg_msg {
	uint32_t enable;			/**< enable/disable */
	uint32_t bufsize;			/**< pkt log buffer size */
	uint32_t hdrsize;			/**< pktlog header size */
	uint32_t msdu_id_offset;		/**< offset for msdu id in msg */
};

/**
 * wifi ol_stats cfg message
 */
struct nss_wifi_ol_stats_cfg_msg {
	uint32_t stats_cfg;			/**< enable/disable*/
};

/**
 * wifi enable/disable perpkt txstats msg
 */
struct nss_wifi_enable_perpkt_txstats_msg {
	uint32_t perpkt_txstats_flag;		/**< flag to enable/disable txstats */
};

/**
 * wifi dbdc process enable msg
 */
struct nss_wifi_dbdc_process_enable_msg {
	uint32_t dbdc_process_enable;		/**< flag to enable/disable dbdc repeater process */
};

/**
 * wifi_primary_radio_set_msg
 */
struct nss_wifi_primary_radio_set_msg {
	uint32_t flag;				/**< flag to set pdev as primary radio */
};

/**
 * wifi_force_client_mcast_traffic_set_msg
 */
struct nss_wifi_force_client_mcast_traffic_set_msg {
	uint32_t flag;				/**< flag to set force_client_mcast_traffic in pdev */
};

/**
 * wifi_store_other_pdev_stavap_msg
 */
struct nss_wifi_store_other_pdev_stavap_msg {
	int stavap_ifnum;               	/**< other pdev's stavap if_num */
};

/**
 * wifi pktlog metadata info
 */
struct nss_wifi_pl_metadata {
	uint32_t len;				/**< length of single buffer in msdu */
	uint32_t msdu_len;			/**< total msdu length */
	uint16_t da_tail;			/**< dest address tail bytes */
	uint16_t sa_tail;			/**< source address tail bytes */
	uint8_t vdev_id;			/**< vdev id */
	uint8_t res1;				/**< reserved 1 */
	uint16_t res2;				/**< reserved 2 */
};

/**
 * wifi ext data plane recieve common meta data
 */
struct nss_wifi_rx_ext_metadata{
	uint16_t peer_id;				/**< peer_id */
	uint8_t htt_rx_status;				/**< htt_rx_status*/
	uint8_t type;					/**< reserve field */
};

/**
 * wifi me statistics message structure.
 */
struct nss_wifi_mc_enhance_stats {
	uint32_t rcvd;				/**< number of mcast frames rcvd for conversion */
	uint32_t ucast_converted;		/**< number of ucast frames sent as part of mcast enhancement conversion */
	uint32_t alloc_fail;			/**< number of mcast enhancement frames dropped due to allocation failure */
	uint32_t enqueue_fail;			/**< number of mcast enhancement frames dropped due to enqueue failure */
	uint32_t copy_fail;			/**< number of mcast enhancement frames dropped due to copy failure */
	uint32_t peer_flow_ctrl_send_fail;	/**< number of mcast enhancement frames dropped due to peer flow ctrl send failure */
	uint32_t loopback_err;			/**< number of mcast enhancement frames dropped when dst_mac is the same as src_mac */
	uint32_t dst_addr_err;			/**< number of mcast enhancement buf frames dropped due to empty dst_mac */
};

/**
 * wifi statistics sync message structure.
 */
struct nss_wifi_stats_sync_msg {
	struct nss_cmn_node_stats node_stats;	/**< node statistics */
	uint32_t tx_transmit_dropped;           /**< number of packets dropped during transmission */
	uint32_t tx_transmit_completions;       /**< number of packets for which transmission completion received */
	uint32_t tx_mgmt_rcv_cnt;               /**< number of management packets received from host for transmission */
	uint32_t tx_mgmt_pkts;                  /**< number of management packets transmitted over wifi */
	uint32_t tx_mgmt_dropped;               /**< number of management packets dropped because of transmission failure */
	uint32_t tx_mgmt_completions;           /**< number of management packets for which tx completions are received */
	uint32_t tx_inv_peer_enq_cnt;           /**< number of packets for which tx enqueue failed because of invalid peer */
	uint32_t rx_inv_peer_rcv_cnt;           /**< number of packets received from wifi with invalid peer id */
	uint32_t rx_pn_check_failed;            /**< number of rx packets which failed packet number check */
	uint32_t rx_pkts_deliverd;              /**< number of rx packets that NSS wifi driver could successfully process */
	uint32_t rx_bytes_deliverd;		/**< number of rx bytes that NSS wifi driver could successfully process */
	uint32_t tx_bytes_transmit_completions;	/**< number of bytes for which transmission completion received */
	uint32_t rx_deliver_unaligned_drop_cnt;	/**< number of unaligned data packets that were dropped at wifi receive */
	uint32_t tidq_enqueue_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Number of packets enqueued to  TIDQ */
	uint32_t tidq_dequeue_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Number of packets dequeued from  TIDQ */
	uint32_t tidq_enqueue_fail_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Enqueue fail count */
	uint32_t tidq_ttl_expire_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Number of packets expired from  TIDQ */
	uint32_t tidq_dequeue_req_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Dequeue reuest count from wifi fw */
	uint32_t total_tidq_depth;	/**< Current Queue Depth */
	uint32_t rx_htt_fetch_cnt;	/**< Total number of HTT Fetch Messages received from wifi fw */
	uint32_t total_tidq_bypass_cnt;	/**< Total number of packets which have bypassed tidq and sent to wifi fw */
	uint32_t global_q_full_cnt;	/**< Total number of packets dropped due to global queue full condition */
	uint32_t tidq_full_cnt;	/**< Total number of packets dropped due to TID queue full condition */
	struct nss_wifi_mc_enhance_stats mc_enhance_stats;	/**< mcast enhancement stats */
	uint32_t mc_enhance_group_entry_miss;		/**< number of times group entry was not present for mcast enhancement */
	uint32_t mc_enhance_denylist_hit;		/**< number of times deny list was hit during mcast enhancement */
};

/**
 * wifi_peer_freelist_create message
 */
struct nss_wifi_peer_freelist_append_msg {
	uint32_t addr;				/**< starting address of peer_freelist pool */
	uint32_t length;			/**< length of peer_freelist pool */
	uint32_t num_peers;			/**< max number of peer entries supported in pool */
};

/**
 * wifi_rx_reorder_tidq_freelist_create message
 */
struct nss_wifi_rx_reorder_array_freelist_append_msg {
	uint32_t addr;				/**< starting address of tidq_freelist pool */
	uint32_t length;			/**< length of tidq_freelist pool */
	uint32_t num_rra;			/**< max number of rx_reorder array entries supported in pool */
};

/**
 *  wifi_bs_peer_inactivity
 *   peer state related info to denote active state of peer
 */
struct nss_wifi_bs_peer_activity {
	uint16_t nentries;	/**< number of entries in the peer_id array */
	uint16_t peer_id[1];	/**< array holding the peer id's */
};

/**
 * nss_wifi_msdu_ttl_set message
 */
struct nss_wifi_msdu_ttl_set_msg {
	uint32_t msdu_ttl;			/**< ttl value to be set */
};

/**
 * wifi VoW extended stats set message structure
 */
struct nss_wifi_rx_vow_extstats_set_msg {
	uint32_t vow_extstats_en;                       /**< vow ext stats */
};

/**
 * nss_wifi_igmp_mld_override_tos_msg
 */
struct nss_wifi_igmp_mld_override_tos_msg {
	uint8_t igmp_mld_ovride_tid_en;		/**< igmp/mld tid override cfg enable */
	uint8_t igmp_mld_ovride_tid_val;	/**< igmp/mld tid override tid value */
	uint8_t res[2];						/**< reserved */
};

/**
 * nss_wifi_peer_ol_stats
 */
struct nss_wifi_peer_ol_stats {
	uint32_t peer_id;            /**< peer id */
	uint32_t seq_num;            /**< sequence number of ppdu */
	uint32_t tx_unaggr;          /**< count of unaggregated pkts txed */
	uint32_t tx_aggr;            /**< count of aggregated pkts txed */
	uint32_t tx_mcast;           /**< no of mcast pkts sent */
	uint32_t tx_ucast;           /**< no of ucat pkts sent */
	uint32_t tx_data;            /**< no of data pkts sent */
	uint32_t tx_bytes;           /**< no of bytes sent */
	uint32_t tx_fail;            /**< no of failed tx pkts */
	uint32_t thrup_bytes;        /**< trhuput bytes */
	uint32_t tx_bcast_pkts;      /**< no of bcast pkts sent */
};

/**
 * wifi_ol_stats
 */
struct nss_wifi_ol_stats_msg {
	uint32_t bawadv_cnt;    /**< block-ack window advancement count */
	uint32_t bcn_cnt;       /**< beacon count */
	uint32_t npeers;        /**< number of entries of peer stats */
	struct nss_wifi_peer_ol_stats peer_ol_stats[1]; /**< array to hold the peer ol stats */
};

/**
 * nss_wifi_sta_kickout message
 */
struct nss_wifi_sta_kickout_msg {
	uint32_t peer_id;	/**< peer id */
};

/**
 * wifi_wnm_peer_rx_activity
 *
 * peer state related info to denote rx activity for peer
 */
struct nss_wifi_wnm_peer_rx_activity_msg {
	uint16_t nentries;			/**< number of entries */
	uint16_t peer_id[NSS_WIFI_MAX_PEER];	/**< array to hold the peer_id's for which the activity is reported */
};

/**
 * wifi peer statistics
 */
struct nss_wifi_peer_stats_msg {
	uint32_t peer_id;					/**< Peer ID */
	uint32_t tidq_byte_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Number of bytes in each TIDQ */
	uint32_t tidq_queue_max[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Maximum depth for TID queue */
	uint32_t tidq_enqueue_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Number of packets enqueued to  TIDQ */
	uint32_t tidq_dequeue_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Number of packets dequeued from  TIDQ */
	uint32_t tidq_ttl_expire_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Number of packets expired from  TIDQ */
	uint32_t tidq_dequeue_req_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Dequeue request count from wifi fw */
	uint32_t tidq_full_cnt[NSS_WIFI_TX_NUM_TOS_TIDS];	/**< Total number of packets dropped due to TID queue full condition */
};

/**
 * ME Group Entry information for host sync
 */
struct nss_wifi_tx_me_host_sync_grp_entry {
	uint8_t group_addr[ETH_ALEN];		/**< group address for this list */
	uint8_t grp_member_addr[ETH_ALEN];	/**< multicast group member mac address */
	union {
		uint32_t grpaddr_ip4;		/**< Group ip address ipv4 */
		uint8_t  grpaddr_ip6[NSS_WIFI_IPV6_ADDR_LEN]; /**< Group ip address ipv6 */
	} u;
	uint32_t src_ip_addr;			/**< source ip address */
};

/**
 * wifi_tx_me_host_sync_msg
 *
 * ME Group table host Sync message
 */
struct nss_wifi_tx_me_host_sync_msg {
	uint16_t vdev_id;		/**< vdev_id to which the group entries belong to  */
	uint8_t  nentries;		/**< number of valid group entries */
	uint8_t  reserved;
	struct nss_wifi_tx_me_host_sync_grp_entry grp_entry[32]; /**< multicast group entry */
};

/**
 * Message structure to send/receive wifi messages
 */
struct nss_wifi_msg {
	struct nss_cmn_msg cm;			/**< Message Header */
	union {
		struct nss_wifi_init_msg initmsg;
		struct nss_wifi_stop_msg stopmsg;
		struct nss_wifi_reset_msg resetmsg;
		struct nss_wifi_htt_init_msg httinitmsg;
		struct nss_wifi_tx_init_msg pdevtxinitmsg;
		struct nss_wifi_rawsend_msg rawmsg;
		struct nss_wifi_mgmtsend_msg mgmtmsg;
		struct nss_wifi_wds_peer_msg pdevwdspeermsg;
		struct nss_wifi_stats_sync_msg statsmsg;
		struct nss_wifi_peer_freelist_append_msg peer_freelist_append;
		struct nss_wifi_rx_reorder_array_freelist_append_msg rx_reorder_array_freelist_append;
		struct nss_wifi_fw_stats_msg fwstatsmsg;
		struct nss_wifi_monitor_set_filter_msg monitor_filter_msg;
		struct nss_wifi_bs_peer_activity peer_activity;
		struct nss_wifi_msdu_ttl_set_msg msdu_ttl_set_msg;
		struct nss_wifi_rx_vow_extstats_set_msg vow_extstats_msg;
		struct nss_wifi_pktlog_cfg_msg pcm_msg;
		struct nss_wifi_enable_perpkt_txstats_msg ept_msg;
		struct nss_wifi_igmp_mld_override_tos_msg wigmpmldtm_msg;
		struct nss_wifi_ol_stats_cfg_msg scm_msg;
		struct nss_wifi_ol_stats_msg ol_stats_msg;
		struct nss_wifi_tx_queue_cfg_msg wtxqcm;
		struct nss_wifi_tx_min_threshold_cfg_msg wtx_min_threshold_cm;
		struct nss_wifi_dbdc_process_enable_msg dbdcpe_msg;
		struct nss_wifi_primary_radio_set_msg wprs_msg;
		struct nss_wifi_force_client_mcast_traffic_set_msg wfcmts_msg;
		struct nss_wifi_store_other_pdev_stavap_msg wsops_msg;
		struct nss_wifi_sta_kickout_msg sta_kickout_msg;
		struct nss_wifi_wnm_peer_rx_activity_msg wprm;
		struct nss_wifi_peer_stats_msg peer_stats_msg;
		struct nss_wifi_tx_me_host_sync_msg wmehsync;
	} msg;
};

/**
 * @brief Send wifi messages
 *
 * @param nss_ctx NSS context
 * @param msg NSS wifi message
 *
 * @return nss_tx_status_t Tx status
 */
extern nss_tx_status_t nss_wifi_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_wifi_msg *msg);

/**
 * @brief Callback to receive wifi messages
 *
 * @param app_data Application context of the message
 * @param msg Message data
 *
 * @return void
 */
typedef void (*nss_wifi_msg_callback_t)(void *app_data, struct nss_wifi_msg *msg);

/**
 * @brief Callback to receive wifi data
 *
 * @param app_data Application context of the message
 * @param os_buf  Pointer to data buffer
 *
 * @return void
 */
typedef void (*nss_wifi_callback_t)(struct net_device *netdev, struct sk_buff *skb, struct napi_struct *napi);

/**
 * @brief Register to send/receive wifi messages to NSS
 *
 * @param if_num NSS interface number
 * @param wifi_callback Callback for wifi data
 * @param msg_callback Callback for wifi messages
 * @param netdev netdevice associated with the wifi
 *
 * @return nss_ctx_instance* NSS context
 */
struct nss_ctx_instance *nss_register_wifi_if(uint32_t if_num, nss_wifi_callback_t wifi_callback,
					      nss_wifi_callback_t wifi_ext_callback, nss_wifi_msg_callback_t event_callback, struct net_device *netdev, uint32_t features);

/**
 * @brief Unregister wifi interface with NSS
 *
 * @param if_num NSS interface number
 *
 * @return void
 */
void nss_unregister_wifi_if(uint32_t if_num);
#endif /* __NSS_WIFI_H */
