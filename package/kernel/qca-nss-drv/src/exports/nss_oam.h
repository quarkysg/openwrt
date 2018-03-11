/*
 **************************************************************************
 * Copyright (c) 2016 The Linux Foundation. All rights reserved.
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
 * nss_oam.h
 *	OAM - Operations, Administration and Maintenance Service
 *
 * This adapter module is responsible for sending and
 * receiving to and from NSS FW
 * This file contains NSS to HLOS interface and msg definitions.
 */

#ifndef __NSS_OAM_H
#define __NSS_OAM_H

#define NSS_OAM_FW_VERSION_LEN	132

/*
 * @brief NSS oam command types
 *
 * @note These are the command types from oam proxy to nss oam server via oam adapter
 */
enum nss_oam_msg_types {
	NSS_OAM_MSG_TYPE_NONE,		/**< none */
	NSS_OAM_MSG_TYPE_GET_FW_VER,	/**< FW Version command*/
	NSS_OAM_MSG_TYPE_MAX,		/**< Max command */
};

/*
 * @brief NSS oam errors
 *
 * @note NSS OAM Errors with response message
 */
enum nss_oam_error {
	NSS_OAM_ERROR_NONE,		/**< none */
	NSS_OAM_ERROR_INVAL_MSG_TYPE,	/**< Invalid message type */
	NSS_OAM_ERROR_INVAL_MSG_LEN,	/**< Invalid message length */
	NSS_OAM_ERROR_MAX,		/**< Max command */
};

/*
 * @brief nss version
 *
 * @note NSS FW version
 */
struct nss_oam_fw_ver {
	uint8_t string[NSS_OAM_FW_VERSION_LEN];	/**< null terminated string*/
};

/*
 * @brief NSS oam message structure
 *
 * @note This is common structure shared between
 * netlink and drv for sending and receiving the
 *	commands and messages between NSS and hlos
 *	commands - sent by hlos to nss (Mostly a set, get request)
 *	messages - sent by nss to hlos (Mostly a response to get set or a notification)
 */
struct nss_oam_msg {
	struct nss_cmn_msg cm;
	union {
		struct nss_oam_fw_ver fw_ver;
	} msg;
};

/*
 * Callback to be called when oam message is received
 */
typedef void (*nss_oam_msg_callback_t)(void *app_data, struct nss_oam_msg *msg);

/*
 * @brief Transmit an OAM message to the NSS
 *
 * @param nss_ctx NSS context
 * @param msg The OAM message
 *
 * @return nss_tx_status_t The status of the Tx operation
 */
extern nss_tx_status_t nss_oam_tx_msg(struct nss_ctx_instance *nss_ctx, struct nss_oam_msg *msg);

/*
 * @brief Register a notifier callback for OAM messages from NSS
 *
 * @param cb The callback pointer
 * @param app_data The application context for this message
 *
 * @return struct nss_ctx_instance * The NSS context
 */
extern struct nss_ctx_instance *nss_oam_notify_register(nss_oam_msg_callback_t cb, void *app_data);

/*
 * @brief Un-Register a notifier callback for OAM messages from NSS
 *
 * @return None
 */
extern void nss_oam_notify_unregister(void);

/*
 * @brief OAM interface Handler
 *
 * @return Boolean status of handler registration
 */
extern bool nss_register_oam_if(uint16_t if_number);

#endif /* __NSS_OAM_H */
