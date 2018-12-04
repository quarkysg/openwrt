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
 * nss_n2h.h
 *	NSS to HLOS interface definitions.
 */

/*
 * TODO: Since this is the now the public header file for writting an
 * IPv4 message, we need to convert the entire file to doxygen.
 */

#ifndef __NSS_N2H_H
#define __NSS_N2H_H

#define MAX_PAGES_PER_MSG 32

/*
 * Private data structure for configure general configs
 */
struct nss_n2h_cfg_pvt {
	struct semaphore sem;			/* Semaphore structure */
	struct completion complete;		/* completion structure */
	int empty_buf_pool;			/* valid entry */
	int low_water;				/* valid entry */
	int high_water;				/* valid entry */
	int wifi_pool;				/* valid entry */
	int response;				/* Response from FW */
};

/*
 * Request/Response types
 */
enum nss_n2h_metadata_types {
	NSS_RX_METADATA_TYPE_N2H_STATS_SYNC=0,
	NSS_TX_METADATA_TYPE_N2H_RPS_CFG,
	NSS_TX_METADATA_TYPE_N2H_EMPTY_POOL_BUF_CFG,
	NSS_TX_METADATA_TYPE_N2H_FLUSH_PAYLOADS,
	NSS_TX_METADATA_TYPE_N2H_MITIGATION_CFG,
	NSS_METADATA_TYPE_N2H_ADD_BUF_POOL,
	NSS_TX_METADATA_TYPE_SET_WATER_MARK,
	NSS_TX_METADATA_TYPE_GET_PAYLOAD_INFO,
	NSS_TX_METADATA_TYPE_N2H_WIFI_POOL_BUF_CFG,
	NSS_TX_DDR_INFO_VIA_N2H_CFG,
	NSS_METADATA_TYPE_N2H_MAX,
};

/*
 * n2h errors -- reference only.
 */
enum nss_n2h_error_types {
	N2H_EUNKNOWN = 1,
	N2H_ALREADY_CFG,		/* Already configured */
	N2H_LOW_WATER_MIN_INVALID,	/* Low water's is lower than min */
	N2H_HIGH_WATER_LESS_THAN_LOW,	/* High water is less than low water */
	N2H_HIGH_WATER_LIMIT_INVALID,	/* High water limit is more than allowed */
	N2H_LOW_WATER_LIMIT_INVALID,	/* Low water limit is more than allowed */
	N2H_WATER_MARK_INVALID,		/* High-Low water is not at least in ring size difference */
	N2H_EMPTY_BUFFER_TOO_HIGH,	/* Empty buffer size is more than allowed */
	N2H_EMPTY_BUFFER_TOO_LOW,	/* Empty buffer size is too low */
	N2H_MMU_ENTRY_IS_INVALID,	/* mmu DDR range entry is not ok to change */
};

struct nss_n2h_rps {
	uint32_t enable; /* Enable NSS RPS */
};

struct nss_n2h_mitigation {
	uint32_t enable; /* Enable NSS MITIGATION */
};

struct nss_n2h_buf_pool {
	uint32_t nss_buf_page_size;
	uint32_t nss_buf_num_pages;
	void *nss_buf_pool_vaddr[MAX_PAGES_PER_MSG];
	uint32_t nss_buf_pool_addr[MAX_PAGES_PER_MSG];
};

/*
 * Old way of setting number of empty pool buffers (payloads).
 * NSS FW then sets low water mark to 'n - ring_size' and
 * high water mark to 'n + ring_size'.
 */
struct nss_n2h_empty_pool_buf {
	uint32_t pool_size;	/* Empty buffer pool size */
};

/*
 * New way of setting low and high water marks in the NSS FW.
 */
struct nss_n2h_water_mark {
	/*
	 * Low water mark. Set it to 0 for system to determine automatically.
	 */
	uint32_t low_water;

	/*
	 * High water mark. Set it to 0 for system to determine automatically.
	 */
	uint32_t high_water;
};

struct nss_n2h_payload_info {
	uint32_t pool_size;	/* Empty buffer pool size */
	/*
	 * Low water mark. Set it to 0 for system to determine automatically.
	 */
	uint32_t low_water;
	/*
	 * High water mark. Set it to 0 for system to determine automatically.
	 */
	uint32_t high_water;
};

struct nss_n2h_flush_payloads {
	uint32_t flag;	/* place holder */
};

/*
 * Number of payloads required for wifi offload
 */
struct nss_n2h_wifi_payloads {
	uint32_t payloads;	/* number of payloads required for wifi */
};

/*
 * NSS Pbuf mgr stats
 */
struct nss_n2h_pbuf_mgr_stats {
	uint32_t pbuf_alloc_fails;		/* Pbuf alloc fail */
	uint32_t pbuf_free_count;		/* Pbuf free count */
	uint32_t pbuf_total_count;		/* Pbuf total count */
};

/*
 * The NSS N2H statistics sync structure.
 */
struct nss_n2h_stats_sync {
	struct nss_cmn_node_stats node_stats;
					/* Common node stats for N2H */
	uint32_t queue_dropped;		/* Number of packets dropped because the PE queue is too full */
	uint32_t total_ticks;		/* Total clock ticks spend inside the PE */
	uint32_t worst_case_ticks;	/* Worst case iteration of the PE in ticks */
	uint32_t iterations;		/* Number of iterations around the PE */

	struct nss_n2h_pbuf_mgr_stats pbuf_ocm_stats;
					/* Pbuf OCM Stats */
	struct nss_n2h_pbuf_mgr_stats pbuf_default_stats;
					/* Pbuf Default Stats */

	uint32_t payload_alloc_fails;	/* Number of payload alloc failures */
	uint32_t payload_free_count;	/* Number of payload alloc failures */

	uint32_t h2n_ctrl_pkts;		/* Control packets received from HLOS */
	uint32_t h2n_ctrl_bytes;	/* Control bytes received from HLOS */
	uint32_t n2h_ctrl_pkts;		/* Control packets sent to HLOS */
	uint32_t n2h_ctrl_bytes;	/* Control bytes sent to HLOS */

	uint32_t h2n_data_pkts;		/* Data packets received from HLOS */
	uint32_t h2n_data_bytes;	/* Data bytes received from HLOS */
	uint32_t n2h_data_pkts;		/* Data packets sent to HLOS */
	uint32_t n2h_data_bytes;	/* Data bytes sent to HLOS */
	uint32_t tot_payloads;		/* Total number of payloads in NSS FW */
	uint32_t data_interface_invalid;	/* Number of data packets received from host with an invalid interface */
};

/*
 * system DDR memory info needed by FW MMU to set range guardian
 */
struct nss_mmu_ddr_info {
	uint32_t ddr_size;	/* total DDR size */
	uint32_t start_address;	/* system DDR start address */
};

/*
 * Message structure to send/receive phys i/f commands
 */
struct nss_n2h_msg {
	struct nss_cmn_msg cm;			/* Message Header */
	union {
		struct nss_n2h_stats_sync stats_sync;	/* Message: N2H stats sync */
		struct nss_n2h_rps rps_cfg; 		/* Message: RPS configuration */
		struct nss_n2h_empty_pool_buf empty_pool_buf_cfg;
							/* Message: empty pool buf configuration */
		struct nss_n2h_flush_payloads flush_payloads;
							/* Message: flush payloads present in NSS */
		struct nss_n2h_mitigation mitigation_cfg;
							/* Message: Mitigation configuration */
		struct nss_n2h_buf_pool buf_pool;	/* Message: pbuf coniguration */
		struct nss_n2h_water_mark wm;
				/* Message: Sets low and high water marks */
		struct nss_n2h_payload_info payload_info;
				/* Message: Gets payload info */
		struct nss_n2h_wifi_payloads wp;
				/* Message: Sets number of wifi payloads */
		struct nss_mmu_ddr_info mmu;	/* use N2H for carrier, will change later */
	} msg;
};

/**
 * Callback to be called when n2h message is received
 */
typedef void (*nss_n2h_msg_callback_t)(void *app_data, struct nss_n2h_msg *msg);

/*
 * nss_n2h_tx_msg()
 * 	API to send messaged to n2h package.
 */
extern nss_tx_status_t nss_n2h_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_n2h_msg *nnm);

/*
 * nss_n2h_register_sysctl()
 *	API to register sysctl for n2h.
 */
extern void nss_n2h_register_sysctl(void);

/*
 * nss_n2h_unregister_sysctl()
 *	API to unregister sysctl for n2h.
 */
extern void nss_n2h_unregister_sysctl(void);

/*
 * nss_n2h_flush_payloads()
 *	API to send flush payloads message to NSS
 */
extern nss_tx_status_t nss_n2h_flush_payloads(struct nss_ctx_instance *nss_ctx);

/*
 * nss_n2h_msg_init()
 *	API to initialize the message for N2H package from Host to NSS
 */
extern void nss_n2h_msg_init(struct nss_n2h_msg *nim, uint16_t if_num, uint32_t type, uint32_t len,
			nss_n2h_msg_callback_t cb, void *app_data);

#endif // __NSS_N2H_H


