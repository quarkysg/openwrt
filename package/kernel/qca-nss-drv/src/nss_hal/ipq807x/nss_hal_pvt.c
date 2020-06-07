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

/**
 * nss_hal_pvt.c
 *	NSS HAL private APIs.
 */

#include <linux/err.h>
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/reset.h>
#include "nss_hal.h"
#include "nss_core.h"

#define NSS_QGIC_IPC_REG_OFFSET 0x8

#define NSS0_H2N_INTR_BASE 13
#define NSS1_H2N_INTR_BASE 19

/*
 * Interrupt type to cause vector.
 */
static uint32_t intr_cause[NSS_MAX_CORES][NSS_H2N_INTR_TYPE_MAX] = {
				/* core0 */
				{(1 << (NSS0_H2N_INTR_BASE + NSS_H2N_INTR_EMPTY_BUFFER_QUEUE)),
				(1 << (NSS0_H2N_INTR_BASE + NSS_H2N_INTR_DATA_COMMAND_QUEUE)),
				(1 << (NSS0_H2N_INTR_BASE + NSS_H2N_INTR_TX_UNBLOCKED)),
				(1 << (NSS0_H2N_INTR_BASE + NSS_H2N_INTR_TRIGGER_COREDUMP))},
				/* core 1 */
				{(1 << (NSS1_H2N_INTR_BASE + NSS_H2N_INTR_EMPTY_BUFFER_QUEUE)),
				(1 << (NSS1_H2N_INTR_BASE + NSS_H2N_INTR_DATA_COMMAND_QUEUE)),
				(1 << (NSS1_H2N_INTR_BASE + NSS_H2N_INTR_TX_UNBLOCKED)),
				(1 << (NSS1_H2N_INTR_BASE + NSS_H2N_INTR_TRIGGER_COREDUMP))}
};

/*
 * nss_hal_handle_data_cmd_irq()
 */
static irqreturn_t nss_hal_handle_data_cmd_queue_irq(int irq, void *ctx)
{
	struct int_ctx_instance *int_ctx = (struct int_ctx_instance *) ctx;

	int_ctx->cause |= int_ctx->queue_cause;

	if (napi_schedule_prep(&int_ctx->napi))
		__napi_schedule(&int_ctx->napi);

	return IRQ_HANDLED;
}

/*
 * nss_hal_handle_empty_buff_sos_irq()
 */
static irqreturn_t nss_hal_handle_empty_buff_sos_irq(int irq, void *ctx)
{
	struct int_ctx_instance *int_ctx = (struct int_ctx_instance *) ctx;

	int_ctx->cause |= NSS_N2H_INTR_EMPTY_BUFFERS_SOS;

	if (napi_schedule_prep(&int_ctx->napi))
		__napi_schedule(&int_ctx->napi);

	return IRQ_HANDLED;
}

/*
 * nss_hal_handle_empty_buff_queue_irq()
 */
static irqreturn_t nss_hal_handle_empty_buff_queue_irq(int irq, void *ctx)
{
	struct int_ctx_instance *int_ctx = (struct int_ctx_instance *) ctx;

	int_ctx->cause |= NSS_N2H_INTR_EMPTY_BUFFER_QUEUE;

	if (napi_schedule_prep(&int_ctx->napi))
		__napi_schedule(&int_ctx->napi);

	return IRQ_HANDLED;
}

/*
 * nss_hal_handle_tx_unblock_irq()
 */
static irqreturn_t nss_hal_handle_tx_unblock_irq(int irq, void *ctx)
{
	struct int_ctx_instance *int_ctx = (struct int_ctx_instance *) ctx;

	int_ctx->cause |= NSS_N2H_INTR_TX_UNBLOCKED;

	if (napi_schedule_prep(&int_ctx->napi))
		__napi_schedule(&int_ctx->napi);
	return IRQ_HANDLED;
}

/*
 * __nss_hal_of_get_pdata()
 *	Retrieve platform data from device node.
 */
static struct nss_platform_data *__nss_hal_of_get_pdata(struct platform_device *pdev)
{
	struct device_node *np = of_node_get(pdev->dev.of_node);
	struct nss_platform_data *npd;
	struct nss_ctx_instance *nss_ctx = NULL;
	struct nss_top_instance *nss_top = &nss_top_main;
	struct resource res_nphys, res_vphys, res_qgic_phys;
	int32_t i;

	npd = devm_kzalloc(&pdev->dev, sizeof(struct nss_platform_data), GFP_KERNEL);
	if (!npd) {
		return NULL;
	}

	if (of_property_read_u32(np, "qcom,id", &npd->id)
	    || of_property_read_u32(np, "qcom,load-addr", &npd->load_addr)
	    || of_property_read_u32(np, "qcom,num-queue", &npd->num_queue)
	    || of_property_read_u32(np, "qcom,num-irq", &npd->num_irq)) {
		pr_err("%s: error reading critical device node properties\n", np->name);
		goto out;
	}

	/*
	 * Read frequencies. If failure, load default values.
	 */
	of_property_read_u32(np, "qcom,low-frequency", &nss_runtime_samples.freq_scale[NSS_FREQ_LOW_SCALE].frequency);
	of_property_read_u32(np, "qcom,mid-frequency", &nss_runtime_samples.freq_scale[NSS_FREQ_MID_SCALE].frequency);
	of_property_read_u32(np, "qcom,max-frequency", &nss_runtime_samples.freq_scale[NSS_FREQ_HIGH_SCALE].frequency);

	nss_ctx = &nss_top->nss[npd->id];
	nss_ctx->id = npd->id;

	if (of_address_to_resource(np, 0, &res_nphys) != 0) {
		nss_info_always("%p: nss%d: of_address_to_resource() fail for nphys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	if (of_address_to_resource(np, 1, &res_vphys) != 0) {
		nss_info_always("%p: nss%d: of_address_to_resource() fail for vphys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	if (of_address_to_resource(np, 2, &res_qgic_phys) != 0) {
		nss_info_always("%p: nss%d: of_address_to_resource() fail for qgic_phys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	/*
	 * Save physical addresses
	 */
	npd->nphys = res_nphys.start;
	npd->vphys = res_vphys.start;
	npd->qgic_phys = res_qgic_phys.start;

	npd->nmap = ioremap_nocache(npd->nphys, resource_size(&res_nphys));
	if (!npd->nmap) {
		nss_info_always("%p: nss%d: ioremap() fail for nphys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	npd->vmap = ioremap_nocache(npd->vphys, resource_size(&res_vphys));
	if (!npd->vmap) {
		nss_info_always("%p: nss%d: ioremap() fail for vphys\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	npd->qgic_map = ioremap_nocache(npd->qgic_phys, resource_size(&res_qgic_phys));
	if (!npd->qgic_map) {
		nss_info_always("%p: nss%d: ioremap() fail for qgic map\n", nss_ctx, nss_ctx->id);
		goto out;
	}

	/*
	 * Clear TCM memory used by this core
	 */
	for (i = 0; i < resource_size(&res_vphys) ; i += 4) {
		nss_write_32(npd->vmap, i, 0);
	}

	/*
	 * Get IRQ numbers
	 */
	for (i = 0 ; i < npd->num_irq; i++) {
		npd->irq[i] = irq_of_parse_and_map(np, i);
		if (!npd->irq[i]) {
			nss_info_always("%p: nss%d: irq_of_parse_and_map() fail for irq %d\n", nss_ctx, nss_ctx->id, i);
			goto out;
		}
	}

	nss_hal_dt_parse_features(np, npd);

	of_node_put(np);
	return npd;

out:
	if (npd->nmap) {
		iounmap(npd->nmap);
	}

	if (npd->vmap) {
		iounmap(npd->vmap);
	}

	devm_kfree(&pdev->dev, npd);
	of_node_put(np);
	return NULL;
}

/*
 * __nss_hal_core_reset()
 */
static int __nss_hal_core_reset(struct platform_device *nss_dev, void __iomem *map, uint32_t addr, uint32_t clk_src)
{
	/*
	 * Todo: AHB/AXI/ubi32 core reset is done in the T32 scripts for RUMI.
	 * Revisit when corebsp supports clock/reset framework
	 */

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

	return 0;
}

/*
 * __nss_hal_debug_enable()
 *	Enable NSS debug
 */
static void __nss_hal_debug_enable(void)
{

}

/*
 * __nss_hal_common_reset
 *	Do reset/clock configuration common to all cores
 */
static int __nss_hal_common_reset(struct platform_device *nss_dev)
{
	struct clk *nss_tcm_src = NULL;
	struct clk *nss_tcm_clk = NULL;
	int err;

	/*
	 * Todo: TLMM is not available on RUMI. Revisit when it is included
	 */

	/*
	 * NSS TCM CLOCK
	 */
	nss_tcm_src = clk_get(&nss_dev->dev, NSS_TCM_SRC_CLK);
	if (IS_ERR(nss_tcm_src)) {
		pr_err("%p: cannot get clock: %s\n", nss_dev, NSS_TCM_SRC_CLK);
		return -EFAULT;
	}

	err = clk_set_rate(nss_tcm_src, NSSTCM_FREQ);
	if (err) {
		pr_err("%p: cannot set clock: %s\n", nss_dev, NSS_TCM_SRC_CLK);
		return -EFAULT;
	}

	err = clk_prepare_enable(nss_tcm_src);
	if (err) {
		pr_err("%p: cannot enable clock: %s\n", nss_dev, NSS_TCM_SRC_CLK);
		return -EFAULT;
	}

	nss_tcm_clk = clk_get(&nss_dev->dev, NSS_TCM_CLK);
	if (IS_ERR(nss_tcm_clk)) {
		pr_err("%p: cannot get clock: %s\n", nss_dev, NSS_TCM_CLK);
		return -EFAULT;
	}

	err = clk_prepare_enable(nss_tcm_clk);
	if (err) {
		pr_err("%p: cannot enable clock: %s\n", nss_dev, NSS_TCM_CLK);
		return -EFAULT;
	}

	/*
	 * NSS Fabric Clocks.
	 */
	nss_fab0_clk = clk_get(&nss_dev->dev, NSS_FABRIC0_CLK);
	if (IS_ERR(nss_fab0_clk)) {
		pr_err("%p: cannot get clock: %s\n", nss_dev, NSS_FABRIC0_CLK);
		nss_fab0_clk = NULL;
	} else {
		err = clk_prepare_enable(nss_fab0_clk);
		if (err) {
			pr_err("%p: cannot enable nss_fab0_clk\n", nss_dev);
			return -EFAULT;
		}
	}

	nss_fab1_clk = clk_get(&nss_dev->dev, NSS_FABRIC1_CLK);
	if (IS_ERR(nss_fab1_clk)) {
		pr_err("%p: cannot get clock: %s\n", nss_dev, NSS_FABRIC1_CLK);
		nss_fab1_clk = NULL;
	} else {
		err = clk_prepare_enable(nss_fab1_clk);
		if (err) {
			pr_err("%p: cannot enable nss_fab1_clk\n", nss_dev);
			return -EFAULT;
		}
	}

	nss_top_main.nss_hal_common_init_done = true;
	nss_info("nss_hal_common_reset Done\n");
	return 0;
}

/*
 * __nss_hal_clock_configure()
 */
static int __nss_hal_clock_configure(struct nss_ctx_instance *nss_ctx, struct platform_device *nss_dev, struct nss_platform_data *npd)
{
	/*
	 * Todo: Clocks are not available from Corebsp yet.
	 */

	return 0;
}

/*
 * __nss_hal_read_interrupt_cause()
 */
static void __nss_hal_read_interrupt_cause(struct nss_ctx_instance *nss_ctx, uint32_t shift_factor, uint32_t *cause)
{
}

/*
 * __nss_hal_clear_interrupt_cause()
 */
static void __nss_hal_clear_interrupt_cause(struct nss_ctx_instance *nss_ctx, uint32_t shift_factor, uint32_t cause)
{
}

/*
 * __nss_hal_disable_interrupt()
 */
static void __nss_hal_disable_interrupt(struct nss_ctx_instance *nss_ctx, uint32_t shift_factor, uint32_t cause)
{
}

/*
 * __nss_hal_enable_interrupt()
 */
static void __nss_hal_enable_interrupt(struct nss_ctx_instance *nss_ctx, uint32_t shift_factor, uint32_t cause)
{
}

/*
 * __nss_hal_send_interrupt()
 */
static void __nss_hal_send_interrupt(struct nss_ctx_instance *nss_ctx, uint32_t type)
{
	/*
	 * Check if core and type is Valid
	 */
	nss_assert(nss_ctx->id < NSS_MAX_CORES);
	nss_assert(type < NSS_H2N_INTR_TYPE_MAX);

	nss_write_32(nss_ctx->qgic_map, NSS_QGIC_IPC_REG_OFFSET, intr_cause[nss_ctx->id][type]);
}

/*
 * __nss_hal_request_irq_for_queue()
 */
static int __nss_hal_request_irq_for_queue(struct nss_ctx_instance *nss_ctx, struct nss_platform_data *npd, int qnum)
{
	struct int_ctx_instance *int_ctx = &nss_ctx->int_ctx[qnum];
	int err;

	/*
	 * Queue0-3 use the IRQ #4 to #7, and are mapped to cause bit 1 to 4
	 */
	snprintf(int_ctx->irq_name, 11, "nss_queue%d", qnum);
	int_ctx->queue_cause = (1 << (qnum+1));
	err = request_irq(npd->irq[qnum+3], nss_hal_handle_data_cmd_queue_irq, 0, int_ctx->irq_name, int_ctx);
	if (err) {
		nss_info_always("%p: IRQ%d request failed", int_ctx, npd->irq[qnum+3]);
		return err;
	}

	int_ctx->irq[0] = npd->irq[qnum+3];

	if (qnum) {
		return 0;
	}

	err = request_irq(npd->irq[0], nss_hal_handle_empty_buff_sos_irq, 0, "nss_empty_buf_sos", int_ctx);
	if (err) {
		nss_info_always("%p: IRQ%d request failed", int_ctx, npd->irq[0]);
		return err;
	}

	int_ctx->irq[1] = npd->irq[0];

	err = request_irq(npd->irq[1], nss_hal_handle_empty_buff_queue_irq, 0, "nss_empty_buf_queue", int_ctx);
	if (err) {
		nss_info_always("%p: IRQ%d request failed", int_ctx, npd->irq[1]);
		return err;
	}

	int_ctx->irq[2] = npd->irq[1];

	err = request_irq(npd->irq[2], nss_hal_handle_tx_unblock_irq, 0, "nss-tx-unblock", int_ctx);
	if (err) {
		nss_info_always("%p: IRQ%d request failed", int_ctx, npd->irq[2]);
		return err;
	}

	int_ctx->irq[3] = npd->irq[2];

	return 0;
}

/*
 * nss_hal_ipq807x_ops
 */
struct nss_hal_ops nss_hal_ipq807x_ops = {
	.common_reset = __nss_hal_common_reset,
	.core_reset = __nss_hal_core_reset,
	.clock_configure = __nss_hal_clock_configure,
	.firmware_load = nss_hal_firmware_load,
	.debug_enable = __nss_hal_debug_enable,
	.of_get_pdata = __nss_hal_of_get_pdata,
	.request_irq_for_queue = __nss_hal_request_irq_for_queue,
	.send_interrupt = __nss_hal_send_interrupt,
	.enable_interrupt = __nss_hal_enable_interrupt,
	.disable_interrupt = __nss_hal_disable_interrupt,
	.clear_interrupt_cause = __nss_hal_clear_interrupt_cause,
	.read_interrupt_cause = __nss_hal_read_interrupt_cause,
};

