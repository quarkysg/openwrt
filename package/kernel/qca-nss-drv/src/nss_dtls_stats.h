/*
 ******************************************************************************
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
 * ****************************************************************************
 */

/*
 * DTLS session debug statistic counters
 */
enum nss_stats_dtls_session {
	NSS_STATS_DTLS_SESSION_RX_PKTS,
			/* Rx packets */
	NSS_STATS_DTLS_SESSION_TX_PKTS,
			/* Tx packets */
	NSS_STATS_DTLS_SESSION_RX_DROPPED,
			/* Rx dropped */
	NSS_STATS_DTLS_SESSION_RX_AUTH_DONE,
			/* Rx successful authentication */
	NSS_STATS_DTLS_SESSION_TX_AUTH_DONE,
			/* Tx authentication done */
	NSS_STATS_DTLS_SESSION_RX_CIPHER_DONE,
			/* Rx cipher done */
	NSS_STATS_DTLS_SESSION_TX_CIPHER_DONE,
			/* Tx cipher done */
	NSS_STATS_DTLS_SESSION_RX_CBUF_ALLOC_FAIL,
			/* Rx crypto buffer alloc fail */
	NSS_STATS_DTLS_SESSION_TX_CBUF_ALLOC_FAIL,
			/* Tx crypto buffer alloc fail */
	NSS_STATS_DTLS_SESSION_TX_CENQUEUE_FAIL,
			/* Tx enqueue to crypto fail */
	NSS_STATS_DTLS_SESSION_RX_CENQUEUE_FAIL,
			/* Rx enqueue to crypto fail */
	NSS_STATS_DTLS_SESSION_TX_DROPPED_HROOM,
			/* Tx drop due to insufficient headroom */
	NSS_STATS_DTLS_SESSION_TX_DROPPED_TROOM,
			/* Tx drop due to insufficient tailroom */
	NSS_STATS_DTLS_SESSION_TX_FORWARD_ENQUEUE_FAIL,
			/* Enqueue failed to Tx node after encap */
	NSS_STATS_DTLS_SESSION_RX_FORWARD_ENQUEUE_FAIL,
			/* Enqueue failed to Rx node after decap */
	NSS_STATS_DTLS_SESSION_RX_INVALID_VERSION,
			/* Rx invalid DTLS version */
	NSS_STATS_DTLS_SESSION_RX_INVALID_EPOCH,
			/* Rx invalid DTLS epoch */
	NSS_STATS_DTLS_SESSION_RX_MALFORMED,
			/* Rx malformed DTLS record */
	NSS_STATS_DTLS_SESSION_RX_CIPHER_FAIL,
			/* Rx cipher fail */
	NSS_STATS_DTLS_SESSION_RX_AUTH_FAIL,
			/* Rx authentication fail */
	NSS_STATS_DTLS_SESSION_RX_CAPWAP_CLASSIFY_FAIL,
			/* Rx CAPWAP classification fail */
	NSS_STATS_DTLS_SESSION_RX_SINGLE_REC_DGRAM,
			/* Rx single record datagrams processed */
	NSS_STATS_DTLS_SESSION_RX_MULTI_REC_DGRAM,
			/* Rx multi record datagrams processed */
	NSS_STATS_DTLS_SESSION_RX_REPLAY_FAIL,
			/* Rx anti-replay failures */
	NSS_STATS_DTLS_SESSION_RX_REPLAY_DUPLICATE,
			/* Rx anti-replay fail due to duplicate record */
	NSS_STATS_DTLS_SESSION_RX_REPLAY_OUT_OF_WINDOW,
			/* Rx anti-replay fail due to out of window record */
	NSS_STATS_DTLS_SESSION_OUTFLOW_QUEUE_FULL,
			/* Tx drop due to encap queue full */
	NSS_STATS_DTLS_SESSION_DECAP_QUEUE_FULL,
			/* Rx drop due to decap queue full */
	NSS_STATS_DTLS_SESSION_PBUF_ALLOC_FAIL,
			/* Drops due to buffer allocation failure */
	NSS_STATS_DTLS_SESSION_PBUF_COPY_FAIL,
			/* Drops due to buffer copy failure */
	NSS_STATS_DTLS_SESSION_EPOCH,
			/* Current Epoch */
	NSS_STATS_DTLS_SESSION_TX_SEQ_HIGH,
			/* Upper 16-bits of current sequence number */
	NSS_STATS_DTLS_SESSION_TX_SEQ_LOW,
			/* Lower 32-bits of current sequence number */
	NSS_STATS_DTLS_SESSION_MAX,
};

/*
 * DTLS session debug statistics
 */
struct nss_stats_dtls_session_debug {
	uint64_t stats[NSS_STATS_DTLS_SESSION_MAX];
	int32_t if_index;
	uint32_t if_num; /* nss interface number */
	bool valid;
};

/*
 * Stats APIs provided by nss_dtls.c
 */
extern void nss_dtls_session_debug_stats_get(struct nss_stats_dtls_session_debug *s);

