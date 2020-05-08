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

/*
 * nss_edma.h
 *	NSS to HLOS interface definitions.
 */

/*
 * TODO: Since this is the now the public header file for writting an
 * EDMA message, we need to convert the entire file to doxygen.
 */

#ifndef __NSS_EDMA_H
#define __NSS_EDMA_H

/**
 * NSS EDMA port and ring defines
 */
#define NSS_EDMA_NUM_PORTS_MAX		256	/**< Max EDMA ports */
#define NSS_EDMA_NUM_RX_RING_MAX	16	/**< Max physical EDMA Rx rings */
#define NSS_EDMA_NUM_RXFILL_RING_MAX	8	/**< Max physical EDMA Rx fill rings */
#define NSS_EDMA_NUM_TX_RING_MAX	24	/**< Max physical EDMA Tx rings */
#define NSS_EDMA_NUM_TXCMPL_RING_MAX	8	/**< Max physical EDMA tx compl rings */
#define NSS_EDMA_STATS_MSG_MAX_PORTS	16	/**< Max port processed per stats msg */

/**
 * Request/Response types
 */
enum nss_edma_metadata_types {
	NSS_METADATA_TYPE_EDMA_PORT_STATS_SYNC,		/**< Port stats sync message */
	NSS_METADATA_TYPE_EDMA_RING_STATS_SYNC,		/**< Ring stats sync message */
	NSS_METADATA_TYPE_EDMA_MAX			/**< Max message types */
};

/**
 * NSS EDMA port types
 */
enum nss_edma_port_t {
	NSS_EDMA_PORT_PHYSICAL,		/**< Physical port */
	NSS_EDMA_PORT_VIRTUAL,		/**< Virtual port */
	NSS_EDMA_PORT_TYPE_MAX		/**< Max port type */
};

/**
 * EDMA Rx ring stats
 */
struct nss_edma_rx_ring_stats {
	uint32_t rx_csum_err;	/**< Number of RX checksum errors */
	uint32_t desc_cnt;	/**< Number of descriptors processed */
};

/**
 * EDMA Tx ring stats
 */
struct nss_edma_tx_ring_stats {
	uint32_t tx_err;	/**< Number of Tx errors */
	uint32_t tx_dropped;	/**< Number of TX dropped packets */
	uint32_t desc_cnt;	/**< Number of descriptors processed */
};

/**
 * EDMA Rx fill ring stats
 */
struct nss_edma_rxfill_ring_stats {
	uint32_t desc_cnt;	/**< Number of descriptors processed */
};

/**
 * EDMA Tx complete ring stats
 */
struct nss_edma_txcmpl_ring_stats {
	uint32_t desc_cnt;	/**< Number of descriptors processed */
};

/**
 * EDMA port stats
 */
struct nss_edma_port_stats {
	struct nss_cmn_node_stats node_stats;	/**< Node stats */
	enum nss_edma_port_t port_type;		/**< Port type */
	uint16_t edma_rx_ring;			/**< Rx ring for the port */
	uint16_t edma_tx_ring;			/**< Tx ring for the port */
};

/**
 * EDMA port statistics.
 */
struct nss_edma_port_stats_sync {
	uint16_t start_port;							/**< Start index of the subset */
	uint16_t end_port;							/**< End index of the subset */
	struct nss_edma_port_stats port_stats[];				/**< Subset of EDMA port statistics */
};

/**
 * EDMA ring stats
 */
struct nss_edma_ring_stats_sync {
	struct nss_edma_tx_ring_stats tx_ring[NSS_EDMA_NUM_TX_RING_MAX];	/**< EDMA TX ring stats */
	struct nss_edma_rx_ring_stats rx_ring[NSS_EDMA_NUM_RX_RING_MAX];	/**< EDMA RX ring stats */
	struct nss_edma_txcmpl_ring_stats txcmpl_ring[NSS_EDMA_NUM_TXCMPL_RING_MAX];	/**< EDMA Tx cmpl ring stats */
	struct nss_edma_rxfill_ring_stats rxfill_ring[NSS_EDMA_NUM_RXFILL_RING_MAX];	/**< EDMA Rx fill ring stats */
};

/**
 * NSS EDMA message to sync with FW EDMA
 */
struct nss_edma_msg {
	struct nss_cmn_msg cm;					/**< Message Header */
	union {
		struct nss_edma_port_stats_sync port_stats;	/**< Message: port stats sync */
		struct nss_edma_ring_stats_sync ring_stats;	/**< Message: ring statistics sync */
	} msg;
};

#ifdef __KERNEL__ /* only kernel will use */

/**
 * Callback to be called when EDMA message is received
 */
typedef void (*nss_edma_msg_callback_t)(void *app_data, struct nss_edma_msg *msg);

/**
 * @brief Register a notifier callback for EDMA messages from NSS
 *
 * @param cb The callback pointer
 * @param app_data The application context for this message
 *
 * @return struct nss_ctx_instance * The NSS context
 */
extern struct nss_ctx_instance *nss_edma_notify_register(nss_edma_msg_callback_t cb, void *app_data);

/**
 * @brief Un-Register a notifier callback for EDMA messages from NSS
 *
 * @return None
 */
extern void nss_edma_notify_unregister(void);

#endif /* __KERNEL__ */
#endif /* __NSS_EDMA_H */
