/*
 **************************************************************************
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
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

#ifndef __NSS_DEF_H
#define __NSS_DEF_H

#define NSS_ETH_NORMAL_FRAME_MTU 1500
#define NSS_ETH_MINI_JUMBO_FRAME_MTU 1978
#define NSS_ETH_FULL_JUMBO_FRAME_MTU 9600

/**
 * VLAN C-TAG TPID
 */
#define VLAN_CTAG_TPID 0x8100

/**
 * Number of ingress/egress VLANS suppored in a connection entry
 */
#define MAX_VLAN_DEPTH 2

/**
 * Number of egress interfaces suppored in a multicast connection entry
 */
#define NSS_MC_IF_MAX 16

#endif /** __NSS_DEF_H */
