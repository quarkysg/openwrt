/*
 **************************************************************************
 * Copyright (c) 2014-2017 The Linux Foundation. All rights reserved.
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
  * nss_dynamic_interface.h
  * 	dynamic interface definitions.
  */

#ifndef __NSS_DYNAMIC_INTERFACE_H
#define __NSS_DYNAMIC_INTERFACE_H

#define NSS_MAX_DYNAMIC_INTERFACES 64

/**
 * Dynamic Interface types
 */
enum nss_dynamic_interface_type {
	NSS_DYNAMIC_INTERFACE_TYPE_NONE,
	NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR,	/* GRE_REDIR Interface type */
	NSS_DYNAMIC_INTERFACE_TYPE_CAPWAP,		/* CAPWAP Interface type */
	NSS_DYNAMIC_INTERFACE_TYPE_TUN6RD,		/* TUN6RD Interface type */
	NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR,	/* 802.3 redirect Interface type */
	NSS_DYNAMIC_INTERFACE_TYPE_WIFI,		/* Wifi redirect Interface type */
	NSS_DYNAMIC_INTERFACE_TYPE_VAP,			/* virtual AP dynamic interface type */
	NSS_DYNAMIC_INTERFACE_TYPE_RESERVED_1,		/* Reserved indexe for backward compatability of other types */
	NSS_DYNAMIC_INTERFACE_TYPE_RESERVED_2,		/* Reserved indexes for backward compatability of other types */
	NSS_DYNAMIC_INTERFACE_TYPE_VIRTIF_DEPRECATED,
	NSS_DYNAMIC_INTERFACE_TYPE_L2TPV2,		/* L2TPV2 Interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_PPTP,		/* PPTP VPN Interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_PORTID,		/* PortID Interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_DTLS,                /* DTLS Interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_MAP_T,		/* MAP-T Interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_GRE_TUNNEL,		/* GRE Tunnel Interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_BRIDGE,		/* Bridge Interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_VLAN,		/* VLAN Interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_GRE,			/* GRE interface Type */
	NSS_DYNAMIC_INTERFACE_TYPE_MAX
};

typedef enum nss_dynamic_interface_type nss_dynamic_interface_assigned;

/*
 * Dynamic Interface request types
 */
enum nss_dynamic_interface_message_types {
	NSS_DYNAMIC_INTERFACE_ALLOC_NODE,	/* Alloc node message type */
	NSS_DYNAMIC_INTERFACE_DEALLOC_NODE,	/* Dealloc node message type */
	NSS_DYNAMIC_INTERFACE_MAX,
};

/*
 * Dynamic interface alloc node msg
 */
struct nss_dynamic_interface_alloc_node_msg {
	enum nss_dynamic_interface_type type;	/* Dynamic Interface type */
	/*
	 * Response
	 */
	int if_num;				/* Interface number */
};

/*
 * Dynamic interface dealloc node msg
 */
struct nss_dynamic_interface_dealloc_node_msg {
	enum nss_dynamic_interface_type type;	/* Dynamic Interface type */
	int if_num;				/* Interface number */
};

/*
 * Message structure to send/receive Dynamic Interface messages
 */
struct nss_dynamic_interface_msg {
	struct nss_cmn_msg cm;							/* Common Message */
	union {
		struct nss_dynamic_interface_alloc_node_msg alloc_node;		/* Msg: Allocate dynamic node */
		struct nss_dynamic_interface_dealloc_node_msg dealloc_node;	/* Msg: deallocate dynamic node */
	} msg;
};

/*
 * Private data structure of dynamic interface
 */
struct nss_dynamic_interface_pvt {
	struct semaphore sem;			/* Semaphore structure */
	struct completion complete;		/* completion structure */
	int current_if_num;			/* Current interface number */
	enum nss_cmn_response response;		/* Message response */
	nss_dynamic_interface_assigned type[NSS_MAX_DYNAMIC_INTERFACES]; /* Array of assigned interface types */
};

/**
 * @brief allocate node for dynamic interface on NSS
 *
 * @param type nss dynamic interface type
 *
 * @return interface number for dynamic interface created on NSS or -1 in case of failure
 */
extern int nss_dynamic_interface_alloc_node(enum nss_dynamic_interface_type type);

/**
 * @brief Deallocate node created for dynamic interface on NSS
 *
 * @param if_num interface number of dynamic interface
 * @param type nss dynamic interface type
 *
 * @return nss_tx_status_t Tx status
 */
extern nss_tx_status_t nss_dynamic_interface_dealloc_node(int if_num, enum nss_dynamic_interface_type type);

/**
 * @brief The inferface number belong to the dynamic interface
 *
 * @param if_num interface number of dynamic interface
 *
 * @return bool true or false
 */
extern bool nss_is_dynamic_interface(int if_num);

/**
 * @brief Returns the type of dynamic interface
 *
 * @param if_num interface number of dynamic interface
 *
 * @return type nss dynamic interface type
 */
extern enum nss_dynamic_interface_type nss_dynamic_interface_get_type(int if_num);

/**
 * @brief Transmits an asynchronous message to firmware.
 *
 * @param nss_ctx context
 * @param msg dynamic interface message to be sent to the firmware.
 *
 * @return message Tx status
 */
extern nss_tx_status_t nss_dynamic_interface_tx(struct nss_ctx_instance *nss_ctx, struct nss_dynamic_interface_msg *msg);

/**
 * @brief Dynamic interface message callback type
 *
 * @param app_data app_data returned to callback fn.
 * @param msg nss common message
 *
 * @return void
 */
typedef void (*nss_dynamic_interface_msg_callback_t)(void *app_data, struct nss_cmn_msg *msg);

/**
 * @brief Initialize dynamic interface message
 *
 * @param ndm dynamic interface message to be initialized.
 * @param if_num Interface number.
 * @param type dynamic interface type
 * @param len message length
 * @param cb callback to be invoked
 * @param app_data app_data passed to the callback fn.
 *
 * @return void
 */
void nss_dynamic_interface_msg_init(struct nss_dynamic_interface_msg *ndm, uint16_t if_num, uint32_t type, uint32_t len,
						void *cb, void *app_data);
#endif /* __NSS_DYNAMIC_INTERFACE_H*/
