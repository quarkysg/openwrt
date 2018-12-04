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
 * nss_trustsec_tx.h
 *	NSS TO HLOS interface definitions.
 */

#ifndef __NSS_TRUSTSEC_TX_H
#define __NSS_TRUSTSEC_TX_H

/**
 * nss trustsec_tx messages
 */

/**
 * @brief trustsec_tx request/response types
 */
enum nss_trustsec_tx_msg_types {
	NSS_TRUSTSEC_TX_CONFIGURE_MSG,		/**< TRUSTSEC_TX configuration msg */
	NSS_TRUSTSEC_TX_UNCONFIGURE_MSG,	/**< TRUSTSEC_TX unconfiguration msg */
	NSS_TRUSTSEC_TX_STATS_SYNC_MSG,		/**< TRUSTSEC_TX stats sync msg */
	NSS_TRUSTSEC_TX_MAX_MSG_TYPE
};

/**
 * @brief trustsec_tx errors
 */
enum nss_trustsec_tx_error_types {
	NSS_TRUSTSEC_TX_ERR_INVAL_SRC_IF,	/**< Source interface invalid */
	NSS_TRUSTSEC_TX_ERR_RECONFIGURE_SRC_IF,	/**< Source interface already configured */
	NSS_TRUSTSEC_TX_ERR_DEST_IF_NOT_FOUND,	/**< Destination interface not found */
	NSS_TRUSTSEC_TX_ERR_NOT_CONFIGURED,	/**< Source interface not configured */
	NSS_TRUSTSEC_TX_ERR_UNKNOWN,		/**< Unknown trustsec message */
};

/**
 * @brief trustsec_tx configuration message
 */
struct nss_trustsec_tx_configure_msg {
	uint32_t src;		/**< if_num of the src tunnel interface */
	uint32_t dest;		/**< Outgoing if num */
	uint16_t sgt;		/**< Security Group Tag value to be embedded to the Trustsec Header */
	uint8_t reserved[2];	/**< Reserved */
};

/**
 * @brief trustsec_tx uncofigure message
 */
struct nss_trustsec_tx_unconfigure_msg {
	uint32_t src;		/**< if num of the src tunnel interface */
	uint16_t sgt;		/**< Security Group Tag value configured for this interface */
	uint8_t reserved[2];	/**< Reserved */
};

/**
 * @brief trustsec_tx statistics sync message
 */
struct nss_trustsec_tx_stats_sync_msg {
	struct nss_cmn_node_stats node_stats;	/**< port node/interface stats sync */
	uint32_t invalid_src;			/**< RX with invalid src interface */
	uint32_t unconfigured_src;		/**< RX with unconfigured src interface */
	uint32_t headroom_not_enough;		/**< Not enough headroom to insert trustsec hdr */
};

/**
 * @brief Message structure to send/receive trustsec_tx messages.
 */
struct nss_trustsec_tx_msg {
	struct nss_cmn_msg cm;						/**< Message Header */
	union {
		struct nss_trustsec_tx_configure_msg configure;		/**< msg: configure trustsec_tx */
		struct nss_trustsec_tx_unconfigure_msg unconfigure;	/**< msg: unconfigure trustsec_tx */
		struct nss_trustsec_tx_stats_sync_msg stats_sync;	/**< msg: trustsec_tx stats sync */
	} msg;
};

/**
 * @brief Callback to receive trustsec_tx messages
 *
 * @param app_data Application context of the message
 * @param msg Message data
 *
 * @return void
 */
typedef void (*nss_trustsec_tx_msg_callback_t)(void *app_data, struct nss_trustsec_tx_msg *npm);

/**
 * @brief Initialize trustsec_tx msg
 *
 * @param npm trustsec_tx msg structure
 * @param if_num trustsec_tx interface number
 * @param type msg type
 * @param len msg length
 * @param cb msg return callback
 * @param app_data application context
 *
 * @return void
 */
extern void nss_trustsec_tx_msg_init(struct nss_trustsec_tx_msg *npm, uint16_t if_num, uint32_t type, uint32_t len,
							nss_trustsec_tx_msg_callback_t cb, void *app_data);

/**
 * @brief Send trustsec_tx messages to NSS
 *
 * @param nss_ctx NSS context
 * @param msg nss_trustsec_tx_msg structure
 *
 * @return Tx status
 */
extern nss_tx_status_t nss_trustsec_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_trustsec_tx_msg *msg);

/**
 * @brief Send trustsec_tx messages to NSS and wait for response
 *
 * @param nss_ctx NSS context
 * @param msg nss_trustsec_tx_msg structure
 *
 * @return Tx status
 */
extern nss_tx_status_t nss_trustsec_tx_msg_sync(struct nss_ctx_instance *nss_ctx, struct nss_trustsec_tx_msg *msg);

/**
 * @brief Gets NSS context
 *
 * @param None
 *
 * @return Pointer to struct nss_ctx_instance
 */
extern struct nss_ctx_instance *nss_trustsec_tx_get_ctx(void);

/**
 * @brief Configure Security Group Tag value for a source interface
 *
 * @param src_if_nim Source interface number
 * @param dest_if_nim Destination interface number
 * @param sgt SGT value
 *
 * @return Pointer to struct nss_ctx_instance
 */
extern nss_tx_status_t nss_trustsec_tx_configure_sgt(uint32_t src, uint32_t dest, uint16_t sgt);

/**
 * @brief Unconfigure Securit Group Tag value for a source interface
 *
 * @param src_if_nim Source interface number
 * @param sgt SGT value
 *
 * @return Pointer to struct nss_ctx_instance
 */
extern nss_tx_status_t nss_trustsec_tx_unconfigure_sgt(uint32_t src, uint16_t sgt);

#endif /* __NSS_TRUSTSEC_TX_H */
