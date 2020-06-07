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
 * nss_ppe.h
 *      NSS PPE header file
 */

#include <net/sock.h>
#include "nss_tx_rx_common.h"

#define PPE_BASE_ADDR			0x3a000000
#define PPE_REG_SIZE			0x1000000

#define PPE_L3_DBG_WR_OFFSET		0x200c04
#define PPE_L3_DBG_RD_OFFSET		0x200c0c
#define PPE_L3_DBG0_OFFSET		0x10001
#define PPE_L3_DBG1_OFFSET		0x10002
#define PPE_L3_DBG2_OFFSET		0x10003
#define PPE_L3_DBG3_OFFSET		0x10004
#define PPE_L3_DBG4_OFFSET		0x10005
#define PPE_L3_DBG_PORT_OFFSET		0x11e80

#define PPE_PKT_CODE_WR_OFFSET		0x100080
#define PPE_PKT_CODE_RD_OFFSET		0x100084
#define PPE_PKT_CODE_DROP0_OFFSET	0xf000000
#define PPE_PKT_CODE_DROP1_OFFSET	0x10000000
#define PPE_PKT_CODE_CPU_OFFSET		0x40000000

#define PPE_PKT_CODE_DROP0_GET(x)	(((x) & 0xe0000000) >> 29)
#define PPE_PKT_CODE_DROP1_GET(x)	(((x) & 0x7) << 3)
#define PPE_PKT_CODE_DROP_GET(d0, d1)	(PPE_PKT_CODE_DROP0_GET(d0) + PPE_PKT_CODE_DROP1_GET(d1))

#define PPE_PKT_CODE_CPU_GET(x)		(((x) >> 3) & 0xff)


/*
 * Data structures to store ppe nss debug stats
 */
static DEFINE_SPINLOCK(nss_ppe_stats_lock);
static struct nss_stats_ppe_debug nss_ppe_debug_stats;

/*
 * Private data structure
 */
static struct nss_ppe_pvt {
	void * __iomem ppe_base;
} ppe_pvt;

/*
 * nss_ppe_reg_read()
 */
static inline void nss_ppe_reg_read(u32 reg, u32 *val)
{
	*val = readl((ppe_pvt.ppe_base + reg));
}

/*
 * nss_ppe_reg_write()
 */
static inline void nss_ppe_reg_write(u32 reg, u32 val)
{
	writel(val, (ppe_pvt.ppe_base + reg));
}
