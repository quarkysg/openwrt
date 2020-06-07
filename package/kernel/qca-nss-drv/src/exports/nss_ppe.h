/*
 **************************************************************************
 * Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
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
 * nss_ppe.h
 *	NSS TO HLOS interface definitions.
 */
#ifndef _NSS_PPE_H_
#define _NSS_PPE_H_

/**
 *  request/response types
 */
enum nss_ppe_metadata_types {
	NSS_PPE_MSG_SYNC_STATS,		/**< session stats sync message */
	NSS_PPE_MSG_MAX,		/**< Max message type */
};

/**
 * Message error types
 */
enum nss_ppe_msg_error_type {
	PPE_MSG_ERROR_OK,			/**< No error */
	PPE_MSG_ERROR_UNKNOWN_TYPE,		/**< Message is of unknown type */
};

/**
 * ppe statistics sync message structure.
 */
struct nss_ppe_sync_stats_msg {
	uint32_t nss_ppe_v4_l3_flows;		/**< No of v4 routed flows */
	uint32_t nss_ppe_v4_l2_flows;		/**< No of v4 bridge flows */
	uint32_t nss_ppe_v4_create_req;		/**< No of v4 create requests */
	uint32_t nss_ppe_v4_create_fail;	/**< No of v4 create failure */
	uint32_t nss_ppe_v4_destroy_req;	/**< No of v4 delete requests */
	uint32_t nss_ppe_v4_destroy_fail;	/**< No of v4 delete failure */

	uint32_t nss_ppe_v6_l3_flows;		/**< No of v6 routed flows */
	uint32_t nss_ppe_v6_l2_flows;		/**< No of v6 bridge flows */
	uint32_t nss_ppe_v6_create_req;		/**< No of v6 create requests */
	uint32_t nss_ppe_v6_create_fail;	/**< No of v6 create failure */
	uint32_t nss_ppe_v6_destroy_req;	/**< No of v6 delete requests */
	uint32_t nss_ppe_v6_destroy_fail;	/**< No of v6 delete failure */

	uint32_t nss_ppe_fail_nh_full;		/**< Create req fail due to nexthop table full */
	uint32_t nss_ppe_fail_flow_full;	/**< Create req fail due to flow table full */
	uint32_t nss_ppe_fail_host_full;	/**< Create req fail due to host table full */
	uint32_t nss_ppe_fail_pubip_full;	/**< Create req fail due to pub-ip table full */
	uint32_t nss_ppe_fail_port_setup;	/**< Create req fail due to PPE port not setup */
	uint32_t nss_ppe_fail_rw_fifo_full;	/**< Create req fail due to rw fifo full */
	uint32_t nss_ppe_fail_flow_command;	/**< Create req fail due to PPE flow command failure */
	uint32_t nss_ppe_fail_unknown_proto;	/**< Create req fail due to unknown protocol */
	uint32_t nss_ppe_fail_ppe_unresponsive;	/**< Create req fail due to PPE not responding */
	uint32_t nss_ppe_fail_fqg_full;		/**< Create req fail due to flow qos group full */
};

/**
 * Message structure for Host-NSS information exchange
 */
struct nss_ppe_msg {
	struct nss_cmn_msg cm;
	union {
		struct nss_ppe_sync_stats_msg stats;
	} msg;
};

/**
 * @brief register ppe interface with nss.
 *
 * @param None
 *
 * @return None
 */
extern void nss_ppe_register_handler(void);

/**
 * @brief get ppe connection stats.
 *
 * @param memory address to be copied to
 *
 * @return None
 */
void nss_ppe_stats_conn_get(uint32_t *stats);

/**
 * @brief get ppe l3 debug stats.
 *
 * @param memory address to be copied to
 *
 * @return None
 */
void nss_ppe_stats_l3_get(uint32_t *stats);

/**
 * @brief get ppe packet code stats.
 *
 * @param memory address to be copied to
 *
 * @return None
 */
void nss_ppe_stats_code_get(uint32_t *stats);

#endif /* _NSS_PPE_H_ */
