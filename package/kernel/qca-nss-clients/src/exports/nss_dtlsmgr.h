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
 * nss_dtlsmgr.h
 *	DTLS manager interface definitions.
 */
#ifndef _NSS_DTLSMGR_H_
#define _NSS_DTLSMGR_H_

/**
 * @brief NSS DTLS manager status
 */
typedef enum nss_dtlsmgr_status {
	NSS_DTLSMGR_OK,			/**< DTLS manager status ok */
	NSS_DTLSMGR_FAIL,		/**< DTLS manager status fail */
	NSS_DTLSMGR_INVALID_VERSION,	/**< Invalid DTLS version */
	NSS_DTLSMGR_INVALID_CIPHER,	/**< Invalid cipher algorithm */
	NSS_DTLSMGR_INVALID_AUTH,	/**< Invalid auth algorithm */
	NSS_DTLSMGR_INVALID_KEY,	/**< Invalid key for cipher/auth */
	NSS_DTLSMGR_CRYPTO_FAILED,	/**< Failed to allocate/initialize
					     crypto session */
} nss_dtlsmgr_status_t;

/**
 * @brief DTLS Version
 */
enum nss_dtlsmgr_dtlsver {
	NSS_DTLSMGR_VERSION_1_0,	/**< DTLS v1.0 */
	NSS_DTLSMGR_VERSION_1_2,	/**< DTLS v1.2 */
	NSS_DTLSMGR_VERSION_INVALID,	/**< DTLS version invalid */
};

/**
 * @brief NSS DTLS manager flags
 */
enum nss_dtlsmgr_flags {
	NSS_DTLSMGR_IPV6_ENCAP = 0x00000001,
					/**< Outer IP header is IPv6 */
	NSS_DTLSMGR_UDPLITE_ENCAP = 0x00000002,
					/**< UDPLite encapsulation */
	NSS_DTLSMGR_CAPWAP = 0x00000004,
					/**< CAPWAP-DTLS session */
	NSS_DTLSMGR_CAPWAP_UDPLITE_HDR_CSUM = 0x00000008,
					/**< Generate only UDP-Lite
					     header checksum */
	NSS_DTLSMGR_PKT_DEBUG = 0x00000010,
					/* DTLS packet debug */
};

/**
 * @brief IPv4/IPv6 address
 */
union nss_dtlsmgr_ip {
	uint32_t ipv4;			/**< IPv4 address */
	uint32_t ipv6[4];		/**< IPv6 address */
};

/**
 * @brief NSS DTLS session encap data
 */
struct nss_dtlsmgr_encap_config {
	enum nss_dtlsmgr_dtlsver ver;	/**< Version used in DTLS header */
	enum nss_crypto_cipher cipher_type;
					/**< Cipher key for encap */
	uint32_t cipher_key_len;	/**< Cipher key length */
	uint8_t *cipher_key;		/**< Location of cipher key */
	enum nss_crypto_auth auth_type;	/**< Auth key for encap */
	uint32_t auth_key_len;		/**< Auth key length */
	uint8_t *auth_key;		/**< Location of auth key */
	uint16_t sport;			/**< Source UDP port */
	uint16_t dport;			/**< Destination UDP port */
	uint16_t epoch;			/**< Epoch */
	union nss_dtlsmgr_ip sip;	/**< Source IP address */
	union nss_dtlsmgr_ip dip;	/**< Destination IP address */
	uint8_t ip_ttl;			/**< IP time to live */
};

/**
 * @brief NSS DTLS session decap data
 */
struct nss_dtlsmgr_decap_config {
	enum nss_crypto_cipher cipher_type;
					/**< Cipher key for decap */
	uint32_t cipher_key_len;	/**< Cipher key length */
	uint8_t *cipher_key;		/**< Location of cipher key */
	enum nss_crypto_auth auth_type;	/**< Auth key for decap */
	uint32_t auth_key_len;		/**< Auth key length */
	uint8_t *auth_key;		/**< Location of auth key */
	uint32_t app_if;		/**< NSS I/F number of tunnel
					     associated with DTLS session.
					     For CAPWAP, this will be the
					     CAPWAP tunnel NSS I/F. */
	uint16_t replay_window_size;	/**< Anti-Replay window size */
};

/**
 * @brief NSS DTLS session create return parameters
 */
struct nss_dtlsmgr_return_data {
	uint32_t dtls_if;		/**< NSS I/F number of DTLS session */
	uint32_t mtu_adjust;		/**< MTU adjustment */
};

/**
 * NSS DTLS session stats update
 */
struct nss_dtlsmgr_session_stats_update {
	uint32_t tx_pkts;		/**< Tx packets */
	uint32_t rx_pkts;		/**< Rx packets */
	uint32_t rx_dropped;		/**< Rx drops */
	uint32_t tx_auth_done;		/**< Tx authentication done */
	uint32_t rx_auth_done;		/**< Rx successful authentication */
	uint32_t tx_cipher_done;	/**< Tx cipher done */
	uint32_t rx_cipher_done;	/**< Rx cipher done */
	uint32_t tx_cbuf_alloc_fail;	/**< Tx crypto buffer allocation fail */
	uint32_t rx_cbuf_alloc_fail;	/**< Rx crypto buffer allocation fail */
	uint32_t tx_cenqueue_fail;	/**< Tx crypto enqueue fail */
	uint32_t rx_cenqueue_fail;	/**< Rx crypto enqueue fail */
	uint32_t tx_dropped_hroom;	/**< Tx drop due to
					     insufficient headroom */
	uint32_t tx_dropped_troom;	/**< Tx drop due to
					     insufficient tailroom */
	uint32_t tx_forward_enqueue_fail;
					/**< Enqueue failed to forwarding
					     node after encap */
	uint32_t rx_forward_enqueue_fail;
					/**< Enqueue failed to receiving
					     node after decap */
	uint32_t rx_invalid_version;	/**< Rx invalid DTLS version */
	uint32_t rx_invalid_epoch;	/**< Rx invalid DTLS epoch */
	uint32_t rx_malformed;		/**< Rx malformed DTLS record */
	uint32_t rx_cipher_fail;	/**< Rx cipher fail */
	uint32_t rx_auth_fail;		/**< Rx authentication fail */
	uint32_t rx_capwap_classify_fail;
					/**< Rx CAPWAP classification fail */
	uint32_t rx_replay_fail;	/**< Rx anti-replay failures */
	uint32_t rx_replay_duplicate;	/**< Rx anti-replay fail for
					     duplicate record */
	uint32_t rx_replay_out_of_window;
					/**< Rx anti-replay fail for out
					     of window record */
	uint32_t outflow_queue_full;	/**< Tx drop due to encap queue full */
	uint32_t decap_queue_full;	/**< Rx drop due to decap queue full */
	uint32_t pbuf_alloc_fail;	/**< Buffer allocation fail */
	uint32_t pbuf_copy_fail;	/**< Buffer copy fail */
	uint16_t epoch;			/**< Current Epoch */
	uint16_t tx_seq_high;		/**< Upper 16-bits of current
					     sequence number */
	uint32_t tx_seq_low;		/**< Lower 32-bits of current
					     sequence number */
};

/**
 * NSS DTLS session stats update callback
 */
typedef void (*nss_dtlsmgr_session_stats_update_cb_t)(uint32_t dtls_if, struct nss_dtlsmgr_session_stats_update *supdate);


/**
 * @brief NSS DTLS session definition
 */
struct nss_dtlsmgr_session_create_config {
	uint32_t flags;				/**< Bit mask of
						     enum nss_dtlsmgr_flags */
	nss_dtlsmgr_session_stats_update_cb_t cb;
						/**< Stats update callback */
	struct nss_dtlsmgr_encap_config encap;	/**< Encap data */
	struct nss_dtlsmgr_decap_config decap;	/**< Decap data */
};

/**
 * @brief NSS DTLS session Tx/Rx cipher update parameters
 */
struct nss_dtlsmgr_session_update_config {
	enum nss_crypto_cipher cipher_type;	/**< Cipher algo */
	uint32_t cipher_key_len;		/**< Cipher key */
	uint8_t *cipher_key;			/**< Location of cipher key */
	enum nss_crypto_auth auth_type;		/**< Auth algo */
	uint32_t auth_key_len;			/**< Auth key length */
	uint8_t *auth_key;			/**< Location of auth key */
	uint16_t epoch;				/**< Epoch */
};

/**
 * @brief Create a NSS DTLS session and associated crypto sessions
 *
 * @param in_data[IN] DTLS session create data
 * @param out_data[out] Return parameters from DTLS session create operation
 *
 * @return NSS_DTLSMGR_OK for success
 */
nss_dtlsmgr_status_t nss_dtlsmgr_session_create_with_crypto(struct nss_dtlsmgr_session_create_config *in_data, struct nss_dtlsmgr_return_data *out_data);

/**
 * @brief Destroy a NSS DTLS session and associated crypto sessions
 *
 * @param dtls_if[IN] NSS DTLS I/F
 *
 * @return NSS_DTLSMGR_OK for success
 */
nss_dtlsmgr_status_t nss_dtlsmgr_session_destroy(uint32_t dtls_if);

/**
 * @brief Update pending Rx cipher state of a DTLS session
 *
 * @param dtls_if[IN] NSS DTLS I/F
 * @param udata[IN] DTLS session update parameters
 *
 * @return NSS_DTLSMGR_OK for success
 *
 * Configures new parameters into the pending Rx cipher
 * state of a DTLS session. This has no effect on the current
 * cipher state and its processing of Rx packets.
 */
nss_dtlsmgr_status_t nss_dtlsmgr_rekey_rx_cipher_update(uint32_t dtls_if, struct nss_dtlsmgr_session_update_config *udata);

/**
 * @brief Update pending Tx cipher state of a DTLS session
 *
 * @param dtls_if[IN] NSS DTLS I/F
 * @param udata[IN] DTLS session update parameters
 *
 * @return NSS_DTLSMGR_OK for success
 *
 * Configures new parameters into the pending Tx cipher
 * state of a DTLS session. This has no effect on the current
 * cipher state and its processing of Tx packets.
 */
nss_dtlsmgr_status_t nss_dtlsmgr_rekey_tx_cipher_update(uint32_t dtls_if, struct nss_dtlsmgr_session_update_config *udata);

/**
 * @brief Switch Rx to new cipher state
 *
 * @param dtls_if[IN] NSS DTLS I/F
 *
 * @return NSS_DTLSMGR_OK for success
 *
 * Causes the Rx pending cipher state of a DTLS session
 * to become current effectively rekeying the Rx path.
 */
nss_dtlsmgr_status_t nss_dtlsmgr_rekey_rx_cipher_switch(uint32_t dtls_if);

/**
 * @brief Switch Tx to new cipher state
 *
 * @param dtls_if[IN] NSS DTLS I/F
 *
 * @return NSS_DTLSMGR_OK for success
 *
 * Causes the Tx pending cipher state of a DTLS session
 * to become current effectively rekeying the Tx path.
 */
nss_dtlsmgr_status_t nss_dtlsmgr_rekey_tx_cipher_switch(uint32_t dtls_if);
#endif /* _NSS_DTLSMGR_H_ */
