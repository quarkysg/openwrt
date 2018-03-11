/*
 **************************************************************************
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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
#include <linux/firmware.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "nss_hal_pvt.h"
#include "nss_core.h"
#include "nss_tx_rx_common.h"
#include "nss_data_plane.h"

/*
 * Table of Compatible devices
 */
struct of_device_id nss_dt_ids[] = {
	{ .compatible =  "qcom,nss" },
	{},
};
MODULE_DEVICE_TABLE(of, nss_dt_ids);

/*
 * __nss_hal_debug_enable()
 *	Enable NSS debug
 */
void __nss_hal_debug_enable(void)
{
	return;
}

/*
 * __nss_hal_core_reset
 */
void __nss_hal_core_reset(uint32_t map, uint32_t addr)
{
	return;
}

/*
 * __nss_hal_clock_pre_init()
 * 	Initialize NSS TCM Clock
 */
int32_t __nss_hal_clock_pre_init(struct platform_device *nss_dev)
{
	return NSS_SUCCESS;
}

/*
 * nss_hal_update_freq_scale()
 * Update frequency scale table
 */
int32_t nss_hal_update_freq_scale(struct nss_platform_data *npd)
{
	return NSS_SUCCESS;
}

/*
 * __nss_hal_load_fw()
 * 	Load NSS firmware
 */
int32_t __nss_hal_load_fw(struct platform_device *nss_dev, struct nss_platform_data *npd)
{
	return NSS_SUCCESS;
}

/*
 * __nss_hal_clock_post_init()
 * 	Configure Core clock
 */
int32_t __nss_hal_clock_post_init(struct platform_device *nss_dev, struct nss_platform_data *npd)
{
	return NSS_SUCCESS;
}

/*
 * __nss_hal_reset_control()
 * 	Initialize HW resources for UBI core and pull out-of-reset
 */
int32_t __nss_hal_reset_control(struct platform_device *nss_dev)
{
	return NSS_SUCCESS;
}

/*
 * File local/Static variables/functions
 */

static const struct net_device_ops nss_netdev_ops;
static const struct ethtool_ops nss_ethtool_ops;

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
 * nss_hal_get_num_irqs()
 * 	get number of irqs from interrupt resource of device tree
 */
static inline int nss_hal_get_num_irqs(struct device_node *np)
{
	int num_irqs = 0;

	while (of_irq_to_resource(np, num_irqs, NULL)) {
		num_irqs++;
	}

	return num_irqs;
}

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

	npd = devm_kzalloc(&pdev->dev, sizeof(struct nss_platform_data), GFP_KERNEL);
	if (!npd) {
		return NULL;
	}

	if (of_property_read_u32(np, "qcom,id", &npd->id)) {
		pr_err("%s: error reading critical device node properties\n", np->name);
		goto out;
	}

	if (of_property_read_u32(np, "qcom,num-irq", &npd->num_irq)) {
		npd->num_irq = nss_hal_get_num_irqs(np);
	}

	npd->gmac_enabled[0] = of_property_read_bool(np, "qcom,gmac0-enabled");
	npd->gmac_enabled[1] = of_property_read_bool(np, "qcom,gmac1-enabled");
	npd->turbo_frequency = of_property_read_bool(np, "qcom,turbo-frequency");
	npd->tstamp_enabled = of_property_read_bool(np, "qcom,tstamp-enabled");

	nss_ctx = &nss_top->nss[npd->id];
	nss_ctx->id = npd->id;

	if (of_address_to_resource(np, 0, &res_nphys) != 0) {
		nss_info("%p: nss%d: of_address_to_resource() fail for nphys \n", nss_ctx, nss_ctx->id);
		goto out;
	}

	if (of_address_to_resource(np, 1, &res_vphys) != 0) {
		nss_info("%p: nss%d: of_address_to_resource() fail for vphys \n", nss_ctx, nss_ctx->id);
		goto out;
	}

	/*
	 * Save physical addresses
	 */
	npd->nphys = res_nphys.start;
	npd->vphys = res_vphys.start;

	npd->nmap = (uint32_t)ioremap_nocache(npd->nphys, resource_size(&res_nphys));
	if (!npd->nmap) {
		nss_info("%p: nss%d: ioremap() fail for nphys \n", nss_ctx, nss_ctx->id);
		goto out;
	}

	npd->vmap = (uint32_t)ioremap_nocache(npd->vphys, resource_size(&res_vphys));
	if (!npd->vmap) {
		nss_info("%p: nss%d: ioremap() fail for vphys \n", nss_ctx, nss_ctx->id);
		goto out;
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

	npd->ipv4_enabled = of_property_read_bool(np, "qcom,ipv4-enabled");
	npd->ipv4_reasm_enabled = of_property_read_bool(np, "qcom,ipv4-reasm-enabled");
	npd->ipv6_enabled = of_property_read_bool(np, "qcom,ipv6-enabled");
	npd->ipv6_reasm_enabled = of_property_read_bool(np, "qcom,ipv6-reasm-enabled");
	npd->crypto_enabled = of_property_read_bool(np, "qcom,crypto-enabled");
	npd->l2switch_enabled = of_property_read_bool(np, "qcom,l2switch-enabled");
	npd->ipsec_enabled = of_property_read_bool(np, "qcom,ipsec-enabled");
	npd->wlanredirect_enabled = of_property_read_bool(np, "qcom,wlan-enabled");
	npd->tun6rd_enabled = of_property_read_bool(np, "qcom,tun6rd-enabled");
	npd->l2tpv2_enabled = of_property_read_bool(np, "qcom,l2tpv2-enabled");
	npd->tunipip6_enabled = of_property_read_bool(np, "qcom,tunipip6-enabled");
	npd->shaping_enabled = of_property_read_bool(np, "qcom,shaping-enabled");
	npd->oam_enabled = of_property_read_bool(np, "qcom,oam-enabled");

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
	struct net_device *tstamp_ndev = NULL;
	int i, err = 0;

	struct device_node *np = NULL;

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

	nss_ctx->nss_top = nss_top;

	nss_info("%p: NSS_DEV_ID %s \n", nss_ctx, dev_name(&nss_dev->dev));
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

	/*
	 * Allocate tstamp net_device and register the net_device
	 */
	if (npd->tstamp_enabled == NSS_FEATURE_ENABLED) {
		tstamp_ndev = nss_tstamp_register_netdev();
		if (!tstamp_ndev) {
			nss_warning("%p: Unable to register the TSTAMP net_device", nss_ctx);
			npd->tstamp_enabled = NSS_FEATURE_NOT_ENABLED;
		}
	}

	spin_lock_bh(&(nss_top->lock));

	/*
	 * Check functionalities are supported by this NSS core
	 */
	if (npd->shaping_enabled == NSS_FEATURE_ENABLED) {
		nss_top->shaping_handler_id = nss_dev->id;
		nss_info("%d: NSS shaping is enabled", nss_dev->id);
	}

	if (npd->tstamp_enabled == NSS_FEATURE_ENABLED) {
		nss_top->tstamp_handler_id = nss_dev->id;
		nss_tstamp_register_handler(tstamp_ndev);
	}

	if (npd->ipv4_enabled == NSS_FEATURE_ENABLED) {
		nss_top->ipv4_handler_id = nss_dev->id;
		nss_ipv4_register_handler();
#if (NSS_PPP_SUPPORT == 1)
		nss_pppoe_register_handler();
#endif
		nss_eth_rx_register_handler();
		nss_n2h_register_handler();
		nss_lag_register_handler();
		nss_dynamic_interface_register_handler();

		for (i = 0; i < NSS_MAX_VIRTUAL_INTERFACES; i++) {
			nss_top->virt_if_handler_id[i] = nss_dev->id;
		}

		nss_top->dynamic_interface_table[NSS_DYNAMIC_INTERFACE_TYPE_802_3_REDIR] = nss_dev->id;
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

	if (npd->l2tpv2_enabled == NSS_FEATURE_ENABLED) {
		nss_top->l2tpv2_handler_id = nss_dev->id;
		nss_l2tpv2_register_handler();
	}

	if (npd->tunipip6_enabled == NSS_FEATURE_ENABLED) {
		nss_top->tunipip6_handler_id = nss_dev->id;
		nss_tunipip6_register_handler();
	}

	if (npd->oam_enabled == NSS_FEATURE_ENABLED) {
		nss_top->oam_handler_id = nss_dev->id;
		nss_oam_register_handler();
	}

	/*
	 * Mark data plane enabled so when nss core init done we call register to nss-gmac
	 */
	for (i = 0 ; i < NSS_MAX_PHYSICAL_INTERFACES ; i++) {
		if (npd->gmac_enabled[i] == NSS_FEATURE_ENABLED) {
			nss_data_plane_set_enabled(i);
		}
	}

#if (NSS_PM_SUPPORT == 1)
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

	if (nss_dev->dev.of_node) {
		if (npd->nmap) {
			iounmap((void *)npd->nmap);
		}

		if (npd->vmap) {
			iounmap((void *)npd->vmap);
		}
	}

err_init_0:

	if (nss_dev->dev.of_node) {
		devm_kfree(&nss_dev->dev, npd);
	}

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

	nss_info("%p: All resources freed for nss core%d", nss_ctx, nss_dev->id);
	return 0;
}

