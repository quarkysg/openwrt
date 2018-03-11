/*
 **************************************************************************
 * Copyright (c) 2013, 2015-2016, The Linux Foundation. All rights reserved.
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
 * nss_hal_pvt.c
 *	NSS HAL private APIs.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/clk.h>

#if (NSS_DT_SUPPORT != 1)
#include <mach/gpiomux.h>
#include <mach/msm_nss.h>
#else
#if (NSS_FABRIC_SCALING_SUPPORT == 1)
#include <linux/fab_scaling.h>
#endif
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/reset.h>
#endif

#include "nss_hal_pvt.h"
#include "nss_clocks.h"
#include "nss_core.h"
#include "nss_tx_rx_common.h"
#include "nss_data_plane.h"
#if (NSS_PM_SUPPORT == 1)
#include "nss_pm.h"
#endif

extern struct clk *nss_core0_clk;
extern struct nss_runtime_sampling nss_runtime_samples;

#if (NSS_DT_SUPPORT == 1)
extern struct clk *nss_fab0_clk;
extern struct clk *nss_fab1_clk;
#endif

/*
 * File local/Static variables/functions
 */
static const struct net_device_ops nss_netdev_ops;
static const struct ethtool_ops nss_ethtool_ops;

/*
 * Macros
 */
#define MIN_IMG_SIZE (64*1024)
#define NETAP0_IMAGE "qca-nss0.bin"
#define NETAP1_IMAGE "qca-nss1.bin"

#if (NSS_FW_DBG_SUPPORT == 1)
/*
 * NSS debug pins configuration
 */

/*
 * Core 0, Data
 * No pull up, Function 2
 */
static struct gpiomux_setting nss_spi_data_0 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

/*
 * Core 0, CLK, CS
 * Pull up high, Function 2
 */
static struct gpiomux_setting nss_spi_cs_clk_0 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

/*
 * Core 1, CS
 * Pull up high, Function 4
 */
static struct gpiomux_setting nss_spi_cs_1 = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

/*
 * Core 1, CLK
 * Pull up high, Function 5
 */
static struct gpiomux_setting nss_spi_clk_1 = {
	.func = GPIOMUX_FUNC_5,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

/*
 * Core 1, Data
 * Pull up none, Function 5
 */
static struct gpiomux_setting nss_spi_data_1 = {
	.func = GPIOMUX_FUNC_5,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config nss_spi_gpiomux[] = {
	{
		.gpio = 14,
		.settings = {
			[GPIOMUX_ACTIVE] = &nss_spi_data_0,
			[GPIOMUX_SUSPENDED] = &nss_spi_data_0,
		},
	},
	{
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE] = &nss_spi_data_0,
			[GPIOMUX_SUSPENDED] = &nss_spi_data_0,
		},
	},
	{
		.gpio = 16,
		.settings = {
			[GPIOMUX_ACTIVE] = &nss_spi_cs_clk_0,
			[GPIOMUX_SUSPENDED] = &nss_spi_cs_clk_0,
		},
	},
	{
		.gpio = 17,
		.settings = {
			[GPIOMUX_ACTIVE] = &nss_spi_cs_clk_0,
			[GPIOMUX_SUSPENDED] = &nss_spi_cs_clk_0,
		},
	},
	{
		.gpio = 55,
		.settings = {
			[GPIOMUX_ACTIVE] = &nss_spi_data_1,
			[GPIOMUX_SUSPENDED] = &nss_spi_data_1,
		},
	},
	{
		.gpio = 56,
		.settings = {
			[GPIOMUX_ACTIVE] = &nss_spi_data_1,
			[GPIOMUX_SUSPENDED] = &nss_spi_data_1,
		},
	},
	{
		.gpio = 57,
		.settings = {
			[GPIOMUX_ACTIVE] = &nss_spi_cs_1,
			[GPIOMUX_SUSPENDED] = &nss_spi_cs_1,
		},
	},
	{
		.gpio = 58,
		.settings = {
			[GPIOMUX_ACTIVE] = &nss_spi_clk_1,
			[GPIOMUX_SUSPENDED] = &nss_spi_clk_1,
		},
	},
};
#endif /* NSS_FW_DBG_SUPPORT */

/*
 * __nss_hal_debug_enable()
 *	Enable NSS debug
 */
void __nss_hal_debug_enable(void)
{
#if (NSS_FW_DBG_SUPPORT == 1)
	msm_gpiomux_install(nss_spi_gpiomux,
				ARRAY_SIZE(nss_spi_gpiomux));
#endif
}

#if (NSS_DT_SUPPORT != 1)
/*
 * nss_hal_pvt_pll_change
 *	Change the Pll between 11(400mhz) or 18(1066 or 1466)
 */
void nss_hal_pvt_pll_change(uint32_t pll)
{
	uint32_t ctl_reg0;
	uint32_t ctl_reg1;

	uint32_t pll11_mask = 0x1;
	uint32_t pll18_mask = 0x0;

	uint32_t pll_cl_mask = 0x1;

	nss_trace("Picking PLL%d\n", pll);

	if (pll == 11) {

		ctl_reg0 = readl(UBI32_COREn_CLK_CTL(0));
		ctl_reg1 = readl(UBI32_COREn_CLK_CTL(1));

		ctl_reg0 &= ~pll_cl_mask;
		ctl_reg1 &= ~pll_cl_mask;

		ctl_reg0 |= pll11_mask;
		ctl_reg1 |= pll11_mask;

		writel(ctl_reg0, UBI32_COREn_CLK_CTL(0));
		writel(ctl_reg1, UBI32_COREn_CLK_CTL(1));

	} else if (pll == 18) {

		ctl_reg0 = readl(UBI32_COREn_CLK_CTL(0));
		ctl_reg1 = readl(UBI32_COREn_CLK_CTL(1));

		ctl_reg0 &= ~pll_cl_mask;
		ctl_reg1 &= ~pll_cl_mask;

		ctl_reg0 |= pll18_mask;
		ctl_reg1 |= pll18_mask;

		writel(ctl_reg0, UBI32_COREn_CLK_CTL(0));
		writel(ctl_reg1, UBI32_COREn_CLK_CTL(1));
	} else {
		BUG_ON(1);
	}

	return;
}

/*
 * nss_hal_pvt_register_dump
 *	Dump Registers Regarding NSS
 */
void nss_hal_pvt_register_dump(void) {
	nss_trace("NSSFB0_CLK_SRC_CTL	: %x\n", readl(NSSFB0_CLK_SRC_CTL));
	nss_trace("NSSFB1_CLK_SRC_CTL	: %x\n", readl(NSSFB1_CLK_SRC_CTL));
	nss_trace("NSSFB0_CLK_SRC0_NS	: %x\n", readl(NSSFB0_CLK_SRC0_NS));
	nss_trace("NSSFB0_CLK_SRC1_NS	: %x\n", readl(NSSFB0_CLK_SRC1_NS));
	nss_trace("NSSFB1_CLK_SRC0_NS	: %x\n", readl(NSSFB1_CLK_SRC0_NS));
	nss_trace("NSSFB1_CLK_SRC1_NS	: %x\n", readl(NSSFB1_CLK_SRC1_NS));
	nss_trace("\n");
	nss_trace("PLL_ENA_NSS	: %x\n", readl(PLL_ENA_NSS));
	nss_trace("PLL18_L_VAL	: %x\n", readl(PLL18_L_VAL));
	nss_trace("PLL18_M_VAL	: %x\n", readl(PLL18_M_VAL));
	nss_trace("PLL18_N_VAL	: %x\n", readl(PLL18_N_VAL));
	nss_trace("PLL18_CONFIG	: %x\n", readl(PLL18_CONFIG));
	nss_trace("PLL18_TEST_CTL: %x\n", readl(PLL18_TEST_CTL));
	nss_trace("\n");
	nss_trace("UBI32_COREn_CLK_SRC0_CTL Core 0: %x\n", readl(UBI32_COREn_CLK_SRC_CTL(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_CTL Core 1: %x\n", readl(UBI32_COREn_CLK_SRC_CTL(1)));
	nss_trace("UBI32_COREn_CLK_SRC0_NS Core 0: %x\n", readl(UBI32_COREn_CLK_SRC0_NS(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_NS Core 1: %x\n", readl(UBI32_COREn_CLK_SRC0_NS(1)));
	nss_trace("UBI32_COREn_CLK_SRC0_MD Core 0: %x\n", readl(UBI32_COREn_CLK_SRC0_MD(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_MD Core 1: %x\n", readl(UBI32_COREn_CLK_SRC0_MD(1)));
	nss_trace("\n\n\n");
}

/*
 * nss_hal_pvt_divide_pll
 *	Divide PLL by int val
 */
uint32_t nss_hal_pvt_divide_pll18(uint32_t core_id, uint32_t divider)
{
	uint32_t ns_mask 	= 0x00ff01ff;
	uint32_t ns_mask_1	= 0x00ff0001;
	uint32_t ns_mask_2	= 0x00fe0141;
	uint32_t ns_mask_5 	= 0x00fb0141;
	uint32_t ns_reg0;
	uint32_t ns_reg1;

	uint32_t md_mask 	= 0x00ff00ff;
	uint32_t md_mask_2	= 0x000100fd;
	uint32_t md_mask_5 	= 0x000100fa;
	uint32_t md_reg0;
	uint32_t md_reg1;

	nss_trace("NSSFB0_CLK_SRC_CTL  : %x\n", readl(NSSFB0_CLK_SRC_CTL));
	nss_trace("NSSFB1_CLK_SRC_CTL  : %x\n", readl(NSSFB1_CLK_SRC_CTL));
	nss_trace("NSSFB0_CLK_SRC0_NS  : %x\n", readl(NSSFB0_CLK_SRC0_NS));
	nss_trace("NSSFB0_CLK_SRC1_NS  : %x\n", readl(NSSFB0_CLK_SRC1_NS));
	nss_trace("NSSFB1_CLK_SRC0_NS  : %x\n", readl(NSSFB1_CLK_SRC0_NS));
	nss_trace("NSSFB1_CLK_SRC1_NS  : %x\n", readl(NSSFB1_CLK_SRC1_NS));
	nss_trace("\n");
	nss_trace("PLL_ENA_NSS	: %x\n", readl(PLL_ENA_NSS));
	nss_trace("PLL18_L_VAL  : %x\n", readl(PLL18_L_VAL));
	nss_trace("PLL18_M_VAL  : %x\n", readl(PLL18_M_VAL));
	nss_trace("PLL18_N_VAL  : %x\n", readl(PLL18_N_VAL));
	nss_trace("PLL18_CONFIG : %x\n", readl(PLL18_CONFIG));
	nss_trace("PLL18_TEST_CTL: %x\n", readl(PLL18_TEST_CTL));
	nss_trace("\n");
	nss_trace("UBI32_COREn_CLK_SRC0_CTL Core 0: %x\n", readl(UBI32_COREn_CLK_SRC_CTL(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_CTL Core 1: %x\n", readl(UBI32_COREn_CLK_SRC_CTL(1)));
	nss_trace("UBI32_COREn_CLK_SRC0_NS Core 0: %x\n", readl(UBI32_COREn_CLK_SRC0_NS(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_NS Core 1: %x\n", readl(UBI32_COREn_CLK_SRC0_NS(1)));
	nss_trace("UBI32_COREn_CLK_SRC0_MD Core 0: %x\n", readl(UBI32_COREn_CLK_SRC0_MD(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_MD Core 1: %x\n", readl(UBI32_COREn_CLK_SRC0_MD(1)));
	nss_trace("\n\n\n");

	md_reg0 = readl(UBI32_COREn_CLK_SRC0_MD(0));
	md_reg1 = readl(UBI32_COREn_CLK_SRC0_MD(1));
	ns_reg0 = readl(UBI32_COREn_CLK_SRC0_NS(0));
	ns_reg1 = readl(UBI32_COREn_CLK_SRC0_NS(1));

	/*
	 * Bypass
	 */
	if (divider == 1) {
		nss_trace("Bypass PLL Output\n");

		/*
		 * Clear M and D ( Not2*D ) and Set Bits
		 */

		md_reg0 &= ~md_mask;
		md_reg1 &= ~md_mask;

		/*
		 * PLL Source/ Pre Divide/ Counter Mode/ Counter Reset/ Counter Enable/ N Value
		 */

		ns_reg0 &= ~ns_mask;
		ns_reg1 &= ~ns_mask;

		ns_reg0 |= ns_mask_1;
		ns_reg1 |= ns_mask_1;
	} else if (divider == 2) {
		nss_trace("Divide PLL Output by 2\n");

		/*
		 * Clear M and D ( Not2*D ) and Set Bits
		 */

		md_reg0 &= ~md_mask;
		md_reg1 &= ~md_mask;

		md_reg0 |= md_mask_2;
		md_reg1 |= md_mask_2;

		/*
		 * PLL Source/ Pre Divide/ Counter Mode/ Counter Reset/ Counter Enable/ N Value
		 */

		ns_reg0 &= ~ns_mask;
		ns_reg1 &= ~ns_mask;

		ns_reg0 |= ns_mask_2;
		ns_reg1 |= ns_mask_2;
	} else if (divider == 5) {
		nss_trace("Divide PLL Output by 5\n");

		/*
		 * Clear M and D ( Not2*D ) and Set Bits
		 */

		md_reg0 &= ~md_mask;
		md_reg1 &= ~md_mask;

		md_reg0 |= md_mask_5;
		md_reg1 |= md_mask_5;

		/*
		 * PLL Source/ Pre Divide/ Counter Mode/ Counter Reset/ Counter Enable/ N Value
		 */

		ns_reg0 &= ~ns_mask;
		ns_reg1 &= ~ns_mask;

		ns_reg0 |= ns_mask_5;
		ns_reg1 |= ns_mask_5;
	} else {
		return 0;
	}

	writel(md_reg0, UBI32_COREn_CLK_SRC0_MD(0));
	writel(md_reg1, UBI32_COREn_CLK_SRC0_MD(1));
	writel(ns_reg0, UBI32_COREn_CLK_SRC0_NS(0));
	writel(ns_reg1, UBI32_COREn_CLK_SRC0_NS(1));

	nss_trace("UBI32_COREn_CLK_SRC0_CTL Core 0: %x\n", readl(UBI32_COREn_CLK_SRC_CTL(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_CTL Core 1: %x\n", readl(UBI32_COREn_CLK_SRC_CTL(1)));
	nss_trace("UBI32_COREn_CLK_SRC0_NS Core 0: %x\n", readl(UBI32_COREn_CLK_SRC0_NS(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_NS Core 1: %x\n", readl(UBI32_COREn_CLK_SRC0_NS(1)));
	nss_trace("UBI32_COREn_CLK_SRC0_MD Core 0: %x\n", readl(UBI32_COREn_CLK_SRC0_MD(0)));
	nss_trace("UBI32_COREn_CLK_SRC0_MD Core 1: %x\n", readl(UBI32_COREn_CLK_SRC0_MD(1)));

	return 1;
}

/*
 * nss_hal_pvt_enable_pll18()
 *	Enable PLL18
 */
uint32_t nss_hal_pvt_enable_pll18(uint32_t speed)
{
	uint32_t retries = 100;

	/*
	 * Prevent Compiler from commenting out the loop.
	 */
	uint32_t value;
	uint32_t mask = (1 << 2);

	/*
	 * Start with clean slate
	 */
	writel(0, PLL18_MODE);

	/*
	 * Effective VCO Frequency = 1100 MHz Post Divide 2
	 */
	if (speed == 1100) {
		writel(0x4000042C, PLL18_L_VAL);
		writel(0x0, PLL18_M_VAL);
		writel(0x1, PLL18_N_VAL);

		/*
		 * PLL configuration (as provided by HW team)
		 */
		writel(0x01495625, PLL18_CONFIG);
		writel(0x00003080, PLL18_TEST_CTL);
	} else if (speed == 1466) {
		/*
		 * Effective VCO Frequency = 1466 MHz Post Divide 2
		 */

		writel(0x4000043A, PLL18_L_VAL);
		writel(0x10, PLL18_M_VAL);
		writel(0x19, PLL18_N_VAL);

		/*
		 * PLL configuration (as provided by HW team)
		 */
		writel(0x014B5625, PLL18_CONFIG);
		writel(0x00003080, PLL18_TEST_CTL);
	} else {
		BUG_ON(1);
	}

	/*
	 * Enable PLL18 output (sequence provided by HW team)
	 */
	writel(0x2, PLL18_MODE);
	mdelay(1);
	writel(0x6, PLL18_MODE);
	writel(0x7, PLL18_MODE);

	/*
	 * Enable NSS Vote for PLL18.
	 */
	writel(mask, PLL_ENA_NSS);
	do {
		value = readl(PLL_LOCK_DET_STATUS);
		if (value & mask) {
			return PLL_LOCKED;
		}

		mdelay(1);
	} while (retries-- > 0);

	return PLL_NOT_LOCKED;
}

/*
 * __nss_hal_common_reset
 *	Do reset/clock configuration common to all cores
 */
void __nss_hal_common_reset(uint32_t *clk_src)
{
	uint32_t i;
	uint32_t value;
	uint32_t status_mask = 0x1;
	uint32_t wait_cycles = 100;

#if defined(NSS_ENABLE_CLK)

	/*
	 * NSS FPB CLOCK
	 */

	/*
	 * Enable clock root and Divider 0
	 * NOTE: Default value is good so no work here
	 */

	/*
	 * PLL0 (800 MHZ). SRC_SEL is 2 (3'b010)
	 * src_div selected is Div-6 (4'b0101).
	 *
	 * Effective frequency (Divider 0) = 133 MHz
	 */
	writel(0x2a, NSSFPB_CLK_SRC0_NS);

	/*
	 * Enable clock branch
	 */
	writel(0x50, NSSFPB_CLK_CTL);

	/*
	 * NSS FABRIC0 CLOCK
	 */

	/*
	 * Enable clock root and Divider 0
	 * NOTE: Default value is good so no work here
	 */

	/*
	 * PLL0 (800 MHZ) and div is set to 2.
	 * Effective frequency = 400 MHZ.
	 */
	writel(0x0a, NSSFB0_CLK_SRC0_NS);

	/*
	 * NSS Fabric0 Branch and dynamic clock gating enabled.
	 */
	writel(0x50, NSSFB0_CLK_CTL);

	/*
	 * Enable clock root and Divider 0
	 * NOTE: Default value is good so no work here
	 */

	/*
	 * PLL0 (800 MHZ) and div is set to 4.
	 * Effective frequency = 200 MHZ.
	 */
	writel(0x1a, NSSFB1_CLK_SRC0_NS);

	/*
	 * NSS Fabric1 Branch enable and fabric clock gating enabled.
	 */
	writel(0x50, NSSFB1_CLK_CTL);

	/*
	 * NSS TCM CLOCK
	 */

	/*
	 * Enable NSS TCM clock root source and select divider 0.
	 *
	 * NOTE: Default value is not good here
	 */
	writel(0x2, NSSTCM_CLK_SRC_CTL);

	/*
	 * PLL0 (800 MHZ) and div is set to 2.
	 * Effective frequency = 400 MHZ
	 */
	writel(0xa, NSSTCM_CLK_SRC0_NS);

	/*
	 * NSS TCM Branch enable and fabric clock gating enabled.
	 */
	writel(0x50, NSSTCM_CLK_CTL);

	/*
	 * Enable global NSS clock branches.
	 * NSS global Fab Branch enable and fabric clock gating enabled.
	 */
	writel(0xf, NSSFAB_GLOBAL_BUS_NS);

	/*
	 * Send reset interrupt to NSS
	 */
	writel(0x0, NSS_RESET);

	/*
	 * Enable PLL18
	 */
	pll18_status = nss_hal_pvt_enable_pll18();
	if (!pll18_status) {
		/*
		 * Select alternate good source (Src1/pll0)
		 */
		*clk_src = NSS_REGS_CLK_SRC_ALTERNATE;
		return;
	}

	/*
	 * Select default source (Src0/pll18)
	 */
	*clk_src = NSS_REGS_CLK_SRC_DEFAULT;
#endif

	/*
	 * Attach debug interface to TLMM
	 */
	nss_write_32((uint32_t)MSM_NSS_FPB_BASE, NSS_REGS_FPB_CSR_CFG_OFFSET, 0x360);

	/*
	 * NSS TCM CLOCK
	 */

	/*
	 * Enable NSS TCM clock root source - SRC1.
	 *
	 */
	writel(0x3, NSSTCM_CLK_SRC_CTL);

	/* Enable PLL Voting for 0 */
	writel((readl(PLL_ENA_NSS) | 0x1), PLL_ENA_NSS);
	do {
		value = readl(PLL_LOCK_DET_STATUS);
		if (value & status_mask) {
			break;
		}
		mdelay(1);
	} while (wait_cycles-- > 0);

	/*
	 * PLL0 (800 MHZ) and div is set to 3/4.
	 * Effective frequency = 266/400 Mhz for SRC0/1
	 */
	writel(0x12, NSSTCM_CLK_SRC0_NS);
	writel(0xa, NSSTCM_CLK_SRC1_NS);

	/*
	 * NSS TCM Branch enable and fabric clock gating enabled.
	 */
	writel(0x50, NSSTCM_CLK_CTL);

	/*
	 * Clear TCM memory
	 */
	for (i = 0; i < IPQ806X_NSS_TCM_SIZE; i += 4) {
		nss_write_32((uint32_t)MSM_NSS_TCM_BASE, i, 0);
	}

	return;
}
#endif /* NSS_DT_SUPPORT */

/*
 * __nss_hal_core_reset
 */
#if (NSS_DT_SUPPORT == 1)
void __nss_hal_core_reset(uint32_t map, uint32_t addr)
#else
void __nss_hal_core_reset(uint32_t core_id, uint32_t map, uint32_t addr, uint32_t clk_src)
#endif
{
#if (NSS_DT_SUPPORT != 1)
#if defined(NSS_ENABLE_CLOCK)
	/*
	 * Enable mpt clock
	 */
	writel(0x10, UBI32_MPT0_CLK_CTL);

	/*
	 * UBI coren clock root enable
	 */
	if (clk_src == NSS_REGS_CLK_SRC_DEFAULT) {
		/* select Src0 */
		writel(0x02, UBI32_COREn_CLK_SRC_CTL(core_id));
	} else {
		/* select Src1 */
		writel(0x03, UBI32_COREn_CLK_SRC_CTL(core_id));
	}

	/*
	 * Src0: Bypass M value configuration.
	 */

	/*
	 * Src1: M val is 0x01 and NOT_2D value is 0xfd, 400 MHz with PLL0.
	 */
	writel(0x100fd, UBI32_COREn_CLK_SRC1_MD(core_id));

	/*
	 * Bypass, pll18
	 * Effective frequency = 550 MHz
	 */
	writel(0x00000001, UBI32_COREn_CLK_SRC0_NS(core_id));

	/*
	 * Dual edge, pll0, NOT(N_M) = 0xfe.
	 * Effective frequency = 400 MHz
	 */
	writel(0x00fe0142, UBI32_COREn_CLK_SRC1_NS(core_id));

	/*
	 * UBI32 coren clock control branch.
	 */
	writel(0x4f, UBI32_COREn_CLK_FS(core_id));

	/*
	 * UBI32 coren clock control branch.
	 */
	writel(0x10, UBI32_COREn_CLK_CTL(core_id));
#endif
	/*
	 * Remove UBI32 reset clamp
	 */
	writel(0xB, UBI32_COREn_RESET_CLAMP(core_id));

	/*
	 * Busy wait for few cycles
	 */
	mdelay(1);

	/*
	 * Remove UBI32 core clamp
	 */
	writel(0x3, UBI32_COREn_RESET_CLAMP(core_id));

	mdelay(1);

	/*
	 * Remove UBI32 AHB reset
	 */
	writel(0x1, UBI32_COREn_RESET_CLAMP(core_id));

	mdelay(1);

	/*
	 * Remove UBI32 AXI reset
	 */
	writel(0x0, UBI32_COREn_RESET_CLAMP(core_id));

	mdelay(1);
#endif /* NSS_DT_SUPPORT */

	/*
	* Apply ubi32 core reset
	*/
	nss_write_32(map, NSS_REGS_RESET_CTRL_OFFSET, 1);

	/*
	 * Program address configuration
	 */
	nss_write_32(map, NSS_REGS_CORE_AMC_OFFSET, 1);
	nss_write_32(map, NSS_REGS_CORE_BAR_OFFSET, 0x3c000000);
	nss_write_32(map, NSS_REGS_CORE_BOOT_ADDR_OFFSET, addr);

	/*
	 * C2C interrupts are level sensitive
	 */
	nss_write_32(map, NSS_REGS_CORE_INT_STAT2_TYPE_OFFSET, 0xFFFF);

	/*
	 * Set IF check value
	 */
	nss_write_32(map, NSS_REGS_CORE_IFETCH_RANGE_OFFSET, 0xBF004001);

	/*
	 * De-assert ubi32 core reset
	 */
	nss_write_32(map, NSS_REGS_RESET_CTRL_OFFSET, 0);
}

#if (NSS_DT_SUPPORT == 1)
/*
 * Platform Device ID for NSS core.
 */
struct of_device_id nss_dt_ids[] = {
	{ .compatible =  "qcom,nss0" },
	{ .compatible =  "qcom,nss1" },
	{},
};
MODULE_DEVICE_TABLE(of, nss_dt_ids);

/*
 * nss_hal_of_get_pdata()
 *	Retrieve platform data from device node.
 */
static struct nss_platform_data *nss_hal_of_get_pdata(struct device_node *np,
						      struct platform_device *pdev)
{
	struct nss_platform_data *npd = NULL;
	struct nss_ctx_instance *nss_ctx = NULL;
	struct nss_top_instance *nss_top = &nss_top_main;
	uint32_t val;
	struct resource res_nphys, res_vphys;
	int32_t i;

	npd = devm_kzalloc(&pdev->dev, sizeof(struct nss_platform_data), GFP_KERNEL);
	if (!npd) {
		return NULL;
	}

	if (of_property_read_u32(np, "qcom,id", &npd->id)
	    || of_property_read_u32(np, "qcom,rst_addr", &npd->rst_addr)
	    || of_property_read_u32(np, "qcom,load_addr", &npd->load_addr)
	    || of_property_read_u32(np, "qcom,turbo_frequency", &npd->turbo_frequency)
	    || of_property_read_u32(np, "qcom,gmac0_enabled", &npd->gmac_enabled[0])
	    || of_property_read_u32(np, "qcom,gmac1_enabled", &npd->gmac_enabled[1])
	    || of_property_read_u32(np, "qcom,gmac2_enabled", &npd->gmac_enabled[2])
	    || of_property_read_u32(np, "qcom,gmac3_enabled", &npd->gmac_enabled[3])
	    || of_property_read_u32(np, "qcom,num_irq", &npd->num_irq)) {
		pr_err("%s: error reading critical device node properties\n", np->name);
		goto out;
	}

	/*
	 * Read frequencies. If failure, load default values.
	 */
	of_property_read_u32(np, "qcom,low_frequency", &nss_runtime_samples.freq_scale[NSS_FREQ_LOW_SCALE].frequency);
	of_property_read_u32(np, "qcom,mid_frequency", &nss_runtime_samples.freq_scale[NSS_FREQ_MID_SCALE].frequency);
	of_property_read_u32(np, "qcom,max_frequency", &nss_runtime_samples.freq_scale[NSS_FREQ_HIGH_SCALE].frequency);

	nss_ctx = &nss_top->nss[npd->id];
	nss_ctx->id = npd->id;

	if (of_address_to_resource(np, 0, &res_nphys) != 0) {
		nss_info("%p: nss%d: of_address_to_resource() fail for nphys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	if (of_address_to_resource(np, 1, &res_vphys) != 0) {
		nss_info("%p: nss%d: of_address_to_resource() fail for vphys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	/*
	 * Save physical addresses
	 */
	npd->nphys = res_nphys.start;
	npd->vphys = res_vphys.start;

	npd->nmap = (uint32_t)ioremap_nocache(npd->nphys, resource_size(&res_nphys));
	if (!npd->nmap) {
		nss_info("%p: nss%d: ioremap() fail for nphys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	npd->vmap = (uint32_t)ioremap_nocache(npd->vphys, resource_size(&res_vphys));
	if (!npd->vmap) {
		nss_info("%p: nss%d: ioremap() fail for vphys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	/*
	 * Clear TCM memory used by this core
	 */
	for (i = 0; i < resource_size(&res_vphys) ; i += 4) {
		nss_write_32((uint32_t)npd->vmap, i, 0);
	}

	/*
	 * Get IRQ numbers
	 */
	for (val = 0 ; val < npd->num_irq ; val++) {
		npd->irq[val] = irq_of_parse_and_map(np, val);
		if (!npd->irq[val]) {
			nss_info("%p: nss%d: irq_of_parse_and_map() fail for irq %d\n",
				 nss_ctx, nss_ctx->id, val);
			goto out;
		}
	}

	of_property_read_u32(np, "qcom,ipv4_enabled", &npd->ipv4_enabled);
	of_property_read_u32(np, "qcom,ipv4_reasm_enabled", &npd->ipv4_reasm_enabled);
	of_property_read_u32(np, "qcom,ipv6_enabled", &npd->ipv6_enabled);
	of_property_read_u32(np, "qcom,ipv6_reasm_enabled", &npd->ipv6_reasm_enabled);
	of_property_read_u32(np, "qcom,l2switch_enabled", &npd->l2switch_enabled);
	of_property_read_u32(np, "qcom,crypto_enabled", &npd->crypto_enabled);
	of_property_read_u32(np, "qcom,ipsec_enabled", &npd->ipsec_enabled);
	of_property_read_u32(np, "qcom,wlanredirect_enabled", &npd->wlanredirect_enabled);
	of_property_read_u32(np, "qcom,tun6rd_enabled", &npd->tun6rd_enabled);
	of_property_read_u32(np, "qcom,l2tpv2_enabled", &npd->l2tpv2_enabled);
#if (NSS_MAP_T_SUPPORT == 1)
	of_property_read_u32(np, "qcom,map_t_enabled", &npd->map_t_enabled);
#endif
	of_property_read_u32(np, "qcom,tunipip6_enabled", &npd->tunipip6_enabled);
	of_property_read_u32(np, "qcom,pptp_enabled", &npd->pptp_enabled);
	of_property_read_u32(np, "qcom,shaping_enabled", &npd->shaping_enabled);
	of_property_read_u32(np, "qcom,wlan_dataplane_offload_enabled", &npd->wifioffload_enabled);
	of_property_read_u32(np, "qcom,portid_enabled", &npd->portid_enabled);
	of_property_read_u32(np, "qcom,dtls_enabled", &npd->dtls_enabled);
	of_property_read_u32(np, "qcom,capwap_enabled", &npd->capwap_enabled);
	of_property_read_u32(np, "qcom,gre_tunnel_enabled", &npd->gre_tunnel_enabled);

	return npd;

out:
	if (npd->nmap) {
		iounmap((void *)npd->nmap);
	}

	if (npd->vmap) {
		iounmap((void *)npd->vmap);
	}

	devm_kfree(&pdev->dev, npd);

	return NULL;
}
#endif

/*
 * nss_hal_dummy_netdev_setup()
 *	Dummy setup for net_device handler
 */
static void nss_hal_dummy_netdev_setup(struct net_device *ndev)
{

}

/*
 * nss_hal_handle_irq()
 *	HLOS interrupt handler for nss interrupts
 */
static irqreturn_t nss_hal_handle_irq(int irq, void *ctx)
{
	struct int_ctx_instance *int_ctx = (struct int_ctx_instance *) ctx;
	struct nss_ctx_instance *nss_ctx = int_ctx->nss_ctx;

	/*
	 * Mask interrupt until our bottom half re-enables it
	 */
	nss_hal_disable_interrupt(nss_ctx->nmap, int_ctx->irq,
			int_ctx->shift_factor, NSS_HAL_SUPPORTED_INTERRUPTS);

	/*
	 * Schedule tasklet to process interrupt cause
	 */
	napi_schedule(&int_ctx->napi);
	return IRQ_HANDLED;
}

/*
 * nss_hal_probe()
 *	HLOS device probe callback
 */
int nss_hal_probe(struct platform_device *nss_dev)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = NULL;
	struct nss_platform_data *npd = NULL;
	struct netdev_priv_instance *ndev_priv;
#if (NSS_DT_SUPPORT == 1)
	struct reset_control *rstctl = NULL;
#if (NSS_FABRIC_SCALING_SUPPORT == 1)
	struct fab_scaling_info fab_data;
#endif
#endif
	int i, err = 0;

	const struct firmware *nss_fw = NULL;
	int rc = -ENODEV;
	void __iomem *load_mem;

#if (NSS_DT_SUPPORT == 1)
	struct device_node *np = NULL;

	if (nss_top_main.nss_hal_common_init_done == false) {
		/*
		 * Perform clock init common to all NSS cores
		 */
		struct clk *nss_tcm_src = NULL;
		struct clk *nss_tcm_clk = NULL;

		/*
		 * Attach debug interface to TLMM
		 */
		nss_write_32((uint32_t)nss_top_main.nss_fpb_base, NSS_REGS_FPB_CSR_CFG_OFFSET, 0x360);

		/*
		 * NSS TCM CLOCK
		 */
		nss_tcm_src = clk_get(&nss_dev->dev, NSS_TCM_SRC_CLK);
		if (IS_ERR(nss_tcm_src)) {
			pr_err("nss-driver: cannot get clock: %s\n", NSS_TCM_SRC_CLK);
			return -EFAULT;
		}

		clk_set_rate(nss_tcm_src, NSSTCM_FREQ);
		clk_prepare(nss_tcm_src);
		clk_enable(nss_tcm_src);

		nss_tcm_clk = clk_get(&nss_dev->dev, NSS_TCM_CLK);
		if (IS_ERR(nss_tcm_clk)) {
			pr_err("nss-driver: cannot get clock: %s\n", NSS_TCM_CLK);
			return -EFAULT;
		}

		clk_prepare(nss_tcm_clk);
		clk_enable(nss_tcm_clk);

		/*
		 * DT - Fabric Clocks.
		 */

		nss_fab0_clk = clk_get(&nss_dev->dev, NSS_FABRIC0_CLK);
		if (IS_ERR(nss_fab0_clk)) {
			printk("nss-driver: cannot get clock: %s\n", NSS_FABRIC0_CLK);
			nss_fab0_clk = NULL;
		} else {
			printk("nss-driver: fabric 0 handler\n");
			clk_prepare(nss_fab0_clk);
			clk_enable(nss_fab0_clk);
		}

		nss_fab1_clk = clk_get(&nss_dev->dev, NSS_FABRIC1_CLK);
		if (IS_ERR(nss_fab1_clk)) {
			printk("nss-driver: cannot get clock: %s\n", NSS_FABRIC1_CLK);
			nss_fab1_clk = NULL;
		} else {
			printk("nss-driver: fabric 1 handler\n");
			clk_prepare(nss_fab1_clk);
			clk_enable(nss_fab1_clk);
		}

		nss_top_main.nss_hal_common_init_done = true;
		nss_info("nss_hal_common_reset Done.\n");
	}

	if (!nss_dev->dev.of_node) {
		/*
		 * Device node not present
		 */
		pr_err("nss-driver: Device tree not available\n");
		return -ENODEV;
	}

	/*
	 * Device Tree based init
	 */
	np = of_node_get(nss_dev->dev.of_node);
	npd = nss_hal_of_get_pdata(np, nss_dev);

	of_node_put(np);

	if (!npd) {
		return -EFAULT;
	}

	nss_ctx = &nss_top->nss[npd->id];
	nss_ctx->id = npd->id;
	nss_dev->id = nss_ctx->id;

#else
	npd = (struct nss_platform_data *) nss_dev->dev.platform_data;
	nss_ctx = &nss_top->nss[nss_dev->id];
	nss_ctx->id = nss_dev->id;
#endif
	nss_ctx->nss_top = nss_top;

	nss_info("%p: NSS_DEV_ID %s \n", nss_ctx, dev_name(&nss_dev->dev));

	/*
	 * F/W load from NSS Driver
	 */
	if (nss_ctx->id == 0) {
		rc = request_firmware(&nss_fw, NETAP0_IMAGE, &(nss_dev->dev));
	} else if (nss_ctx->id == 1) {
		rc = request_firmware(&nss_fw, NETAP1_IMAGE, &(nss_dev->dev));
	} else {
		nss_warning("%p: Invalid nss dev: %d \n", nss_ctx, nss_ctx->id);
	}

	/*
	 *  Check if the file read is successful
	 */
	if (rc) {
		nss_info_always("%p: request_firmware failed with err code: %d", nss_ctx, rc);
		err = rc;
		goto err_init_0;
	}

	if (nss_fw->size < MIN_IMG_SIZE) {
		nss_info_always("%p: nss firmware is truncated, size:%d", nss_ctx, nss_fw->size);
		goto err_init_0;
	}

	load_mem = ioremap_nocache(npd->load_addr, nss_fw->size);
	if (load_mem == NULL) {
		nss_info_always("%p: ioremap_nocache failed: %x", nss_ctx, npd->load_addr);
		release_firmware(nss_fw);
		goto err_init_0;
	}

	printk("nss_driver - fw of size %u  bytes copied to load addr: %x, nss_id : %d\n", nss_fw->size, npd->load_addr, nss_dev->id);
	memcpy_toio(load_mem, nss_fw->data, nss_fw->size);
	release_firmware(nss_fw);
	iounmap(load_mem);

	/*
	 * Both NSS cores controlled by same regulator, Hook only Once
	 */
	if (!nss_ctx->id) {
		nss_core0_clk = clk_get(&nss_dev->dev, "nss_core_clk");
		if (IS_ERR(nss_core0_clk)) {
			err = PTR_ERR(nss_core0_clk);
			nss_info("%p: Regulator %s get failed, err=%d\n", nss_ctx, dev_name(&nss_dev->dev), err);
			return err;
		}

		/*
		 * Check if turbo is supported
		 */
		if (npd->turbo_frequency) {
			/*
			 * Turbo is supported
			 */
			printk("nss_driver - Turbo Support %d\n", npd->turbo_frequency);
#if (NSS_PM_SUPPORT == 1)
			nss_pm_set_turbo();
#endif
		} else {
			printk("nss_driver - Turbo No Support %d\n", npd->turbo_frequency);
		}

		/*
		 * If valid entries - from dtsi - then just init clks.
		 * Otherwise query for clocks.
		 */
		if ((nss_runtime_samples.freq_scale[NSS_FREQ_LOW_SCALE].frequency != 0) &&
			(nss_runtime_samples.freq_scale[NSS_FREQ_MID_SCALE].frequency != 0) &&
			(nss_runtime_samples.freq_scale[NSS_FREQ_HIGH_SCALE].frequency != 0)) {
			goto clk_complete;
		}

		/*
		 * Load default scales, then query for higher.
		 * If basic set cannot be set, then go to error, and abort
		 * Two set of defaults, 110, 550, 733 or 110, 275 and 550
		 */
		if (clk_set_rate(nss_core0_clk, NSS_FREQ_110) != 0) {
			goto err_init_0;
		}
		nss_runtime_samples.freq_scale[NSS_FREQ_LOW_SCALE].frequency = NSS_FREQ_110;

		if (npd->turbo_frequency) {
			/*
			 * Figure out the middle scale
			 */
			if (clk_set_rate(nss_core0_clk, NSS_FREQ_600) == 0) {
				nss_runtime_samples.freq_scale[NSS_FREQ_MID_SCALE].frequency = NSS_FREQ_600;
			} else if (clk_set_rate(nss_core0_clk, NSS_FREQ_550) == 0) {
				nss_runtime_samples.freq_scale[NSS_FREQ_MID_SCALE].frequency = NSS_FREQ_550;
			} else {
				goto err_init_0;
			}

			/*
			 * Figure out the max scale
			 */
			if (clk_set_rate(nss_core0_clk, NSS_FREQ_800) == 0) {
				nss_runtime_samples.freq_scale[NSS_FREQ_HIGH_SCALE].frequency = NSS_FREQ_800;
			} else if (clk_set_rate(nss_core0_clk, NSS_FREQ_733) == 0) {
				nss_runtime_samples.freq_scale[NSS_FREQ_HIGH_SCALE].frequency = NSS_FREQ_733;
			} else {
				goto err_init_0;
			}

		} else {
			if (clk_set_rate(nss_core0_clk, NSS_FREQ_275) != 0) {
				goto err_init_0;
			}
			nss_runtime_samples.freq_scale[NSS_FREQ_MID_SCALE].frequency = NSS_FREQ_275;

			if (clk_set_rate(nss_core0_clk, NSS_FREQ_550) != 0) {
				goto err_init_0;
			}
			nss_runtime_samples.freq_scale[NSS_FREQ_HIGH_SCALE].frequency = NSS_FREQ_550;
		}

clk_complete:
#if (NSS_DT_SUPPORT == 1)
#if (NSS_FABRIC_SCALING_SUPPORT == 1)
		if (npd->turbo_frequency) {
			fab_data.idle_freq = nss_runtime_samples.freq_scale[NSS_FREQ_MID_SCALE].frequency;
		} else {
			fab_data.idle_freq = nss_runtime_samples.freq_scale[NSS_FREQ_HIGH_SCALE].frequency;
		}
		fab_data.clk = nss_core0_clk;
		fab_scaling_register(&fab_data);
#endif
#endif

		/*
		 * Setup Ranges
		 */
		for (i = 0; i < NSS_FREQ_MAX_SCALE; i++) {
			if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_110) {
				nss_runtime_samples.freq_scale[i].minimum = NSS_FREQ_110_MIN;
				nss_runtime_samples.freq_scale[i].maximum = NSS_FREQ_110_MAX;
			}
			if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_275) {
				nss_runtime_samples.freq_scale[i].minimum = NSS_FREQ_275_MIN;
				nss_runtime_samples.freq_scale[i].maximum = NSS_FREQ_275_MAX;
			}
			if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_550) {
				nss_runtime_samples.freq_scale[i].minimum = NSS_FREQ_550_MIN;
				nss_runtime_samples.freq_scale[i].maximum = NSS_FREQ_550_MAX;
			}
			if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_600) {
				nss_runtime_samples.freq_scale[i].minimum = NSS_FREQ_600_MIN;
				nss_runtime_samples.freq_scale[i].maximum = NSS_FREQ_600_MAX;
			}
			if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_733) {
				nss_runtime_samples.freq_scale[i].minimum = NSS_FREQ_733_MIN;
				nss_runtime_samples.freq_scale[i].maximum = NSS_FREQ_733_MAX;
			}
			if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_800) {
				nss_runtime_samples.freq_scale[i].minimum = NSS_FREQ_800_MIN;
				nss_runtime_samples.freq_scale[i].maximum = NSS_FREQ_800_MAX;
			}
		}

		printk("Supported Frequencies - ");
		for (i = 0; i < NSS_FREQ_MAX_SCALE; i++) {
			if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_110) {
				printk("110Mhz ");
			} else if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_275) {
				printk("275Mhz ");
			} else if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_550) {
				printk("550Mhz ");
			} else if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_600) {
				printk("600Mhz ");
			} else if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_733) {
				printk("733Mhz ");
			} else if (nss_runtime_samples.freq_scale[i].frequency == NSS_FREQ_800) {
				printk("800Mhz ");
			} else {
				printk("Error \nNo Table/Invalid Frequency Found - Loading Old Tables - ");
				goto err_init_0;
			}
		}
		printk("\n");

		/*
		 * Set default frequency
		 */
		clk_set_rate(nss_core0_clk, nss_runtime_samples.freq_scale[NSS_FREQ_MID_SCALE].frequency);
		clk_prepare(nss_core0_clk);
		clk_enable(nss_core0_clk);
	}

	/*
	 * Get load address of NSS firmware
	 */
	nss_info("%p: Setting NSS%d Firmware load address to %x\n", nss_ctx, nss_ctx->id, npd->load_addr);
	nss_top->nss[nss_ctx->id].load = npd->load_addr;

	/*
	 * Get virtual and physical memory addresses for nss logical/hardware address maps
	 */

	/*
	 * Virtual address of CSM space
	 */
	nss_ctx->nmap = npd->nmap;
	nss_assert(nss_ctx->nmap);

	/*
	 * Physical address of CSM space
	 */
	nss_ctx->nphys = npd->nphys;
	nss_assert(nss_ctx->nphys);

	/*
	 * Virtual address of logical registers space
	 */
	nss_ctx->vmap = npd->vmap;
	nss_assert(nss_ctx->vmap);

	/*
	 * Physical address of logical registers space
	 */
	nss_ctx->vphys = npd->vphys;
	nss_assert(nss_ctx->vphys);
	nss_info("%d:ctx=%p, vphys=%x, vmap=%x, nphys=%x, nmap=%x",
			nss_ctx->id, nss_ctx, nss_ctx->vphys, nss_ctx->vmap, nss_ctx->nphys, nss_ctx->nmap);

	/*
	 * Register netdevice handlers
	 */
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3, 16, 0))
	nss_ctx->int_ctx[0].ndev = alloc_netdev(sizeof(struct netdev_priv_instance),
					"qca-nss-dev%d", nss_hal_dummy_netdev_setup);
#else
	nss_ctx->int_ctx[0].ndev = alloc_netdev(sizeof(struct netdev_priv_instance),
					"qca-nss-dev%d", NET_NAME_ENUM, nss_hal_dummy_netdev_setup);
#endif
	if (nss_ctx->int_ctx[0].ndev == NULL) {
		nss_warning("%p: Could not allocate net_device #0", nss_ctx);
		err = -ENOMEM;
		goto err_init_0;
	}

	nss_ctx->int_ctx[0].ndev->netdev_ops = &nss_netdev_ops;
	nss_ctx->int_ctx[0].ndev->ethtool_ops = &nss_ethtool_ops;
	err = register_netdev(nss_ctx->int_ctx[0].ndev);
	if (err) {
		nss_warning("%p: Could not register net_device #0", nss_ctx);
		goto err_init_1;
	}

	/*
	 * request for IRQs
	 *
	 * WARNING: CPU affinities should be set using OS supported methods
	 */
	nss_ctx->int_ctx[0].nss_ctx = nss_ctx;
	nss_ctx->int_ctx[0].shift_factor = 0;
	nss_ctx->int_ctx[0].irq = npd->irq[0];
	err = request_irq(npd->irq[0], nss_hal_handle_irq, 0, "nss", &nss_ctx->int_ctx[0]);
	if (err) {
		nss_warning("%d: IRQ0 request failed", nss_dev->id);
		goto err_init_2;
	}

	/*
	 * Register NAPI for NSS core interrupt #0
	 */
	ndev_priv = netdev_priv(nss_ctx->int_ctx[0].ndev);
	ndev_priv->int_ctx = &nss_ctx->int_ctx[0];
	netif_napi_add(nss_ctx->int_ctx[0].ndev, &nss_ctx->int_ctx[0].napi, nss_core_handle_napi, 64);
	napi_enable(&nss_ctx->int_ctx[0].napi);
	nss_ctx->int_ctx[0].napi_active = true;

	/*
	 * Check if second interrupt is supported on this nss core
	 */
	if (npd->num_irq > 1) {
		nss_info("%d: This NSS core supports two interrupts", nss_dev->id);

		/*
		 * Register netdevice handlers
		 */
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3, 16, 0))
		nss_ctx->int_ctx[1].ndev = alloc_netdev(sizeof(struct netdev_priv_instance),
						"qca-nss-dev%d", nss_hal_dummy_netdev_setup);
#else
		nss_ctx->int_ctx[1].ndev = alloc_netdev(sizeof(struct netdev_priv_instance),
						"qca-nss-dev%d", NET_NAME_ENUM, nss_hal_dummy_netdev_setup);
#endif
		if (nss_ctx->int_ctx[1].ndev == NULL) {
			nss_warning("%p: Could not allocate net_device #1", nss_ctx);
			err = -ENOMEM;
			goto err_init_3;
		}

		nss_ctx->int_ctx[1].ndev->netdev_ops = &nss_netdev_ops;
		nss_ctx->int_ctx[1].ndev->ethtool_ops = &nss_ethtool_ops;
		err = register_netdev(nss_ctx->int_ctx[1].ndev);
		if (err) {
			nss_warning("%p: Could not register net_device #1", nss_ctx);
			goto err_init_4;
		}

		nss_ctx->int_ctx[1].nss_ctx = nss_ctx;
		nss_ctx->int_ctx[1].shift_factor = 15;
		nss_ctx->int_ctx[1].irq = npd->irq[1];
		err = request_irq(npd->irq[1], nss_hal_handle_irq, 0, "nss", &nss_ctx->int_ctx[1]);
		if (err) {
			nss_warning("%d: IRQ1 request failed for nss", nss_dev->id);
			goto err_init_5;
		}

		/*
		 * Register NAPI for NSS core interrupt #1
		 */
		ndev_priv = netdev_priv(nss_ctx->int_ctx[1].ndev);
		ndev_priv->int_ctx = &nss_ctx->int_ctx[1];
		netif_napi_add(nss_ctx->int_ctx[1].ndev, &nss_ctx->int_ctx[1].napi, nss_core_handle_napi, 64);
		napi_enable(&nss_ctx->int_ctx[1].napi);
		nss_ctx->int_ctx[1].napi_active = true;
	}

	spin_lock_bh(&(nss_top->lock));

	/*
	 * Check functionalities are supported by this NSS core
	 */
	if (npd->shaping_enabled == NSS_FEATURE_ENABLED) {
		nss_top->shaping_handler_id = nss_dev->id;
		nss_info("%d: NSS shaping is enabled", nss_dev->id);
	}

	if (npd->ipv4_enabled == NSS_FEATURE_ENABLED) {
		nss_top->ipv4_handler_id = nss_dev->id;
		nss_ipv4_register_handler();
		nss_pppoe_register_handler();
		nss_eth_rx_register_handler();
		nss_n2h_register_handler();
		nss_lag_register_handler();
		nss_dynamic_interface_register_handler();

		for (i = 0; i < NSS_MAX_VIRTUAL_INTERFACES; i++) {
			nss_top->virt_if_handler_id[i] = nss_dev->id;
		}

		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR] = nss_dev->id;
	}

	if (npd->capwap_enabled == NSS_FEATURE_ENABLED) {
		nss_top->capwap_handler_id = nss_dev->id;
		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_CAPWAP] = nss_dev->id;
	}

	if (npd->ipv4_reasm_enabled == NSS_FEATURE_ENABLED) {
		nss_top->ipv4_reasm_handler_id = nss_dev->id;
		nss_ipv4_reasm_register_handler();
	}

	if (npd->ipv6_enabled == NSS_FEATURE_ENABLED) {
		nss_top->ipv6_handler_id = nss_dev->id;
		nss_ipv6_register_handler();
	}

	if (npd->ipv6_reasm_enabled == NSS_FEATURE_ENABLED) {
		nss_top->ipv6_reasm_handler_id = nss_dev->id;
		nss_ipv6_reasm_register_handler();
	}

	if (npd->crypto_enabled == NSS_FEATURE_ENABLED) {
		nss_top->crypto_enabled = 1;
		nss_top->crypto_handler_id = nss_dev->id;
		nss_crypto_register_handler();
	}

	if (npd->ipsec_enabled == NSS_FEATURE_ENABLED) {
		nss_top->ipsec_handler_id = nss_dev->id;
		nss_ipsec_register_handler();
	}

	if (npd->wlanredirect_enabled == NSS_FEATURE_ENABLED) {
		nss_top->wlan_handler_id = nss_dev->id;
	}

	if (npd->tun6rd_enabled == NSS_FEATURE_ENABLED) {
		nss_top->tun6rd_handler_id = nss_dev->id;
	}

	if (npd->pptp_enabled == NSS_FEATURE_ENABLED) {
		nss_top->pptp_handler_id = nss_dev->id;
		nss_pptp_register_handler();
	}

	if (npd->l2tpv2_enabled == NSS_FEATURE_ENABLED) {
		nss_top->l2tpv2_handler_id = nss_dev->id;
		nss_l2tpv2_register_handler();
	}

	if (npd->dtls_enabled == NSS_FEATURE_ENABLED) {
		nss_top->dtls_handler_id = nss_dev->id;
		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_DTLS] = nss_dev->id;
		nss_dtls_register_handler();
	}

#if (NSS_MAP_T_SUPPORT == 1)
	if (npd->map_t_enabled == NSS_FEATURE_ENABLED) {
		nss_top->map_t_handler_id = nss_dev->id;
		nss_map_t_register_handler();
	}
#endif

	if (npd->tunipip6_enabled == NSS_FEATURE_ENABLED) {
		nss_top->tunipip6_handler_id = nss_dev->id;
		nss_tunipip6_register_handler();
	}

	if (npd->gre_redir_enabled == NSS_FEATURE_ENABLED) {
		nss_top->gre_redir_handler_id = nss_dev->id;
		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_GRE_REDIR] =  nss_dev->id;
		nss_gre_redir_register_handler();
		nss_sjack_register_handler();
	}

	if (npd->portid_enabled == NSS_FEATURE_ENABLED) {
		nss_top->portid_handler_id = nss_dev->id;
		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_PORTID] = nss_dev->id;
		nss_portid_register_handler();
	}

	if (npd->wifioffload_enabled == NSS_FEATURE_ENABLED) {
		nss_top->wifi_handler_id = nss_dev->id;
		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_RADIO_0] = nss_dev->id;
		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_RADIO_1] = nss_dev->id;
		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_RADIO_2] = nss_dev->id;
		nss_wifi_register_handler();
	}

	if (npd->gre_tunnel_enabled == NSS_FEATURE_ENABLED) {
		nss_top->gre_tunnel_handler_id = nss_dev->id;
		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_GRE_TUNNEL] = nss_dev->id;
	}

	/*
	 * Mark data plane enabled so when nss core init done we call register to nss-gmac
	 */
	for (i = 0 ; i < NSS_MAX_PHYSICAL_INTERFACES ; i++) {
		if (npd->gmac_enabled[i] == NSS_FEATURE_ENABLED) {
			nss_data_plane_set_enabled(i);
		}
	}

#if (NSS_FREQ_SCALE_SUPPORT == 1)
	nss_freq_register_handler();
#endif
	nss_lso_rx_register_handler();

	nss_top->frequency_handler_id = nss_dev->id;

	spin_unlock_bh(&(nss_top->lock));

	/*
	 * Initialize decongestion callbacks to NULL
	 */
	for (i = 0; i < NSS_MAX_CLIENTS; i++) {
		nss_ctx->queue_decongestion_callback[i] = 0;
		nss_ctx->queue_decongestion_ctx[i] = 0;
	}

	spin_lock_init(&(nss_ctx->decongest_cb_lock));
	nss_ctx->magic = NSS_CTX_MAGIC;

	nss_info("%p: Reseting NSS core %d now", nss_ctx, nss_ctx->id);

	/*
	 * Enable clocks and bring NSS core out of reset
	 */
#if (NSS_DT_SUPPORT == 1)
	/*
	 * Remove UBI32 reset clamp
	 */
	rstctl = devm_reset_control_get(&nss_dev->dev, "clkrst_clamp");
	if (IS_ERR(rstctl)) {
		nss_info("%p: Deassert UBI32 core%d reset clamp failed", nss_ctx, nss_ctx->id);
		err = -EFAULT;
		goto err_init_5;
	}
	reset_control_deassert(rstctl);

	/*
	 * Remove UBI32 core clamp
	 */
	rstctl = devm_reset_control_get(&nss_dev->dev, "clamp");
	if (IS_ERR(rstctl)) {
		nss_info("%p: Deassert UBI32 core%d clamp failed", nss_ctx, nss_ctx->id);
		err = -EFAULT;
		goto err_init_5;
	}
	reset_control_deassert(rstctl);

	/*
	 * Remove UBI32 AHB reset
	 */
	rstctl = devm_reset_control_get(&nss_dev->dev, "ahb");
	if (IS_ERR(rstctl)) {
		nss_info("%p: Deassert AHB core%d reset failed", nss_ctx, nss_ctx->id);
		err = -EFAULT;
		goto err_init_5;
	}
	reset_control_deassert(rstctl);

	/*
	 * Remove UBI32 AXI reset
	 */
	rstctl = devm_reset_control_get(&nss_dev->dev, "axi");
	if (IS_ERR(rstctl)) {
		nss_info("%p: Deassert core%d AXI reset failed", nss_ctx, nss_ctx->id);
		err = -EFAULT;
		goto err_init_5;
	}
	reset_control_deassert(rstctl);

	nss_hal_core_reset(nss_ctx->nmap, nss_ctx->load);
#else
	nss_hal_core_reset(nss_dev->id, nss_ctx->nmap, nss_ctx->load, nss_top->clk_src);
#endif
	/*
	 * Initialize max buffer size for NSS core
	 */
	nss_ctx->max_buf_size = NSS_NBUF_PAYLOAD_SIZE;

	/*
	 * Initialize S/G status pointers to NULL
	 */
	for (i = 0; i < NSS_N2H_DESC_RING_NUM; i++) {
		nss_ctx->n2h_desc_ring[i].head = NULL;
		nss_ctx->n2h_desc_ring[i].tail = NULL;
		nss_ctx->n2h_desc_ring[i].jumbo_start = NULL;
	}

        /*
	 * Increment number of cores
	 */
	nss_top->num_nss++;

	/*
	 * Enable interrupts for NSS core
	 */
	nss_hal_enable_interrupt(nss_ctx->nmap, nss_ctx->int_ctx[0].irq,
					nss_ctx->int_ctx[0].shift_factor, NSS_HAL_SUPPORTED_INTERRUPTS);

	if (npd->num_irq > 1) {
		nss_hal_enable_interrupt(nss_ctx->nmap, nss_ctx->int_ctx[1].irq,
					nss_ctx->int_ctx[1].shift_factor, NSS_HAL_SUPPORTED_INTERRUPTS);
	}

	nss_info("%p: All resources initialized and nss core%d has been brought out of reset", nss_ctx, nss_dev->id);
	goto err_init_0;

err_init_5:
	unregister_netdev(nss_ctx->int_ctx[1].ndev);
err_init_4:
	free_netdev(nss_ctx->int_ctx[1].ndev);
err_init_3:
	free_irq(npd->irq[0], &nss_ctx->int_ctx[0]);
err_init_2:
	unregister_netdev(nss_ctx->int_ctx[0].ndev);
err_init_1:
	free_netdev(nss_ctx->int_ctx[0].ndev);

#if (NSS_DT_SUPPORT == 1)
	if (nss_dev->dev.of_node) {
		if (npd->nmap) {
			iounmap((void *)npd->nmap);
		}

		if (npd->vmap) {
			iounmap((void *)npd->vmap);
		}
	}
#endif

err_init_0:

#if (NSS_DT_SUPPORT == 1)
	if (nss_dev->dev.of_node) {
		devm_kfree(&nss_dev->dev, npd);
	}
#endif
	return err;
}

/*
 * nss_hal_remove()
 *	HLOS device remove callback
 */
int nss_hal_remove(struct platform_device *nss_dev)
{
	struct nss_top_instance *nss_top = &nss_top_main;
	struct nss_ctx_instance *nss_ctx = &nss_top->nss[nss_dev->id];
	int i;

	/*
	 * Clean-up debugfs
	 */
	nss_stats_clean();

	/*
	 * Disable interrupts and bottom halves in HLOS
	 * Disable interrupts from NSS to HLOS
	 */
	nss_hal_disable_interrupt(nss_ctx->nmap, nss_ctx->int_ctx[0].irq,
					nss_ctx->int_ctx[0].shift_factor, NSS_HAL_SUPPORTED_INTERRUPTS);

	free_irq(nss_ctx->int_ctx[0].irq, &nss_ctx->int_ctx[0]);
	unregister_netdev(nss_ctx->int_ctx[0].ndev);
	free_netdev(nss_ctx->int_ctx[0].ndev);

	/*
	 * Check if second interrupt is supported
	 * If so then clear resources for second interrupt as well
	 */
	if (nss_ctx->int_ctx[1].irq) {
		nss_hal_disable_interrupt(nss_ctx->nmap, nss_ctx->int_ctx[1].irq,
					nss_ctx->int_ctx[1].shift_factor, NSS_HAL_SUPPORTED_INTERRUPTS);
		free_irq(nss_ctx->int_ctx[1].irq, &nss_ctx->int_ctx[1]);
		unregister_netdev(nss_ctx->int_ctx[1].ndev);
		free_netdev(nss_ctx->int_ctx[1].ndev);
	}

	/*
	 * nss-drv is exiting, remove from nss-gmac
	 */
	for (i = 0 ; i < NSS_MAX_PHYSICAL_INTERFACES ; i++) {
		if (nss_ctx->subsys_dp_register[i].ndev) {
			nss_data_plane_unregister_from_nss_gmac(i);
			nss_ctx->subsys_dp_register[i].ndev = NULL;
		}
	}
#if (NSS_DT_SUPPORT == 1)
#if (NSS_FABRIC_SCALING_SUPPORT == 1)
	fab_scaling_unregister(nss_core0_clk);
#endif
	if (nss_dev->dev.of_node) {
		if (nss_ctx->nmap) {
			iounmap((void *)nss_ctx->nmap);
			nss_ctx->nmap = 0;
		}

		if (nss_ctx->vmap) {
			iounmap((void *)nss_ctx->vmap);
			nss_ctx->vmap = 0;
		}
	}
#endif

	nss_info("%p: All resources freed for nss core%d", nss_ctx, nss_dev->id);
	return 0;
}

