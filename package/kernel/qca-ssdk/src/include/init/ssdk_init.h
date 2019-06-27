/*
 * Copyright (c) 2012, 2015-2017, The Linux Foundation. All rights reserved.
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
 */



#ifndef _SSDK_INIT_H_
#define _SSDK_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "common/sw.h"
#include "fal_led.h"

#define SSDK_MAX_PORT_NUM		8
#define SSDK_MAX_VIRTUAL_PORT_NUM	256
#define SSDK_MAX_SERVICE_CODE_NUM	256
#define SSDK_MAX_CPU_CODE_NUM		256
#define SSDK_L0SCHEDULER_CFG_MAX	300
#define SSDK_L0SCHEDULER_UCASTQ_CFG_MAX	256
#define SSDK_L1SCHEDULER_CFG_MAX	64
#define SSDK_SP_MAX_PRIORITY		8
#define SSDK_MAX_FRAME_SIZE	16383

#define PORT_GMAC_TYPE	1
#define PORT_XGMAC_TYPE	2

    typedef enum {
        HSL_MDIO = 1,
        HSL_HEADER,
    }
    hsl_access_mode;

    typedef enum
    {
        HSL_NO_CPU = 0,
        HSL_CPU_1,
        HSL_CPU_2,
        HSL_CPU_1_PLUS,
    } hsl_init_mode;

    typedef enum
    {
        HSL_REG_MDIO = 0,
        HSL_REG_LOCAL_BUS,
    } hsl_reg_mode;

    typedef sw_error_t
    (*mdio_reg_set) (a_uint32_t dev_id, a_uint32_t phy_addr, a_uint32_t reg,
                     a_uint16_t data);

    typedef sw_error_t
    (*mdio_reg_get) (a_uint32_t dev_id, a_uint32_t phy_addr, a_uint32_t reg,
                     a_uint16_t * data);

    typedef sw_error_t
    (*hdr_reg_set) (a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t *reg_data, a_uint32_t len);

    typedef sw_error_t
    (*hdr_reg_get) (a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t *reg_data, a_uint32_t len);

    typedef sw_error_t
    (*psgmii_reg_set) (a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t *reg_data, a_uint32_t len);

    typedef sw_error_t
    (*psgmii_reg_get) (a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t *reg_data, a_uint32_t len);

	typedef sw_error_t
	(*uniphy_reg_set) (a_uint32_t dev_id, a_uint32_t index, a_uint32_t reg_addr, a_uint8_t *reg_data, a_uint32_t len);

	typedef sw_error_t
	(*uniphy_reg_get) (a_uint32_t dev_id, a_uint32_t index, a_uint32_t reg_addr, a_uint8_t *reg_data, a_uint32_t len);

	typedef void (*mii_reg_set)(a_uint32_t dev_id, a_uint32_t reg, a_uint32_t val);

	typedef a_uint32_t (*mii_reg_get)(a_uint32_t dev_id, a_uint32_t reg);

enum ssdk_port_wrapper_cfg {
	PORT_WRAPPER_PSGMII = 0,
	PORT_WRAPPER_PSGMII_RGMII5,
	PORT_WRAPPER_SGMII0_RGMII5,
	PORT_WRAPPER_SGMII1_RGMII5,
	PORT_WRAPPER_PSGMII_RMII0,
	PORT_WRAPPER_PSGMII_RMII1,
	PORT_WRAPPER_PSGMII_RMII0_RMII1,
	PORT_WRAPPER_PSGMII_RGMII4,
	PORT_WRAPPER_SGMII0_RGMII4,
	PORT_WRAPPER_SGMII1_RGMII4,
	PORT_WRAPPER_SGMII4_RGMII4,
	PORT_WRAPPER_QSGMII,
	PORT_WRAPPER_SGMII_PLUS,
	PORT_WRAPPER_USXGMII,
	PORT_WRAPPER_10GBASE_R,
	PORT_WRAPPER_SGMII_CHANNEL0,
	PORT_WRAPPER_SGMII_CHANNEL1,
	PORT_WRAPPER_SGMII_CHANNEL4,
	PORT_WRAPPER_RGMII,
	PORT_WRAPPER_MAX = 0xFF
};

    typedef struct
    {
        mdio_reg_set    mdio_set;
        mdio_reg_get    mdio_get;
        hdr_reg_set     header_reg_set;
        hdr_reg_get     header_reg_get;
        psgmii_reg_set     psgmii_reg_set;
        psgmii_reg_get     psgmii_reg_get;
        uniphy_reg_set     uniphy_reg_set;
        uniphy_reg_get     uniphy_reg_get;
	mii_reg_set	mii_reg_set;
	mii_reg_get	mii_reg_get;
    } hsl_reg_func;

    typedef struct
    {
        a_bool_t  mac0_rgmii;
        a_bool_t  mac5_rgmii;
        a_bool_t  rx_delay_s0;
        a_bool_t  rx_delay_s1;
        a_bool_t  tx_delay_s0;
        a_bool_t  tx_delay_s1;
        a_bool_t  rgmii_rxclk_delay;
        a_bool_t  rgmii_txclk_delay;
        a_bool_t  phy4_rx_delay;
        a_bool_t  phy4_tx_delay;
    } garuda_init_spec_cfg;

    typedef enum
    {
        CHIP_UNSPECIFIED = 0,
        CHIP_ATHENA,
        CHIP_GARUDA,
        CHIP_SHIVA,
        CHIP_HORUS,
        CHIP_ISIS,
        CHIP_ISISC,
        CHIP_DESS,
        CHIP_HPPE,
    } ssdk_chip_type;

	typedef struct
	{
		a_uint32_t cpu_bmp;
		a_uint32_t lan_bmp;
		a_uint32_t wan_bmp;
	} ssdk_port_cfg;

	typedef struct
	{
		a_uint32_t led_num;
		a_uint32_t led_source_id;
		led_ctrl_pattern_t led_pattern;

	} led_source_cfg_t;


typedef struct
{
	hsl_init_mode   cpu_mode;
	hsl_access_mode reg_mode;
	hsl_reg_func    reg_func;

	ssdk_chip_type  chip_type;

	/* os specific parameter */
	/* when uk_if based on netlink, it's netlink protocol type*/
	/* when uk_if based on ioctl, it's minor device number, major number
	is always 10(misc device) */
	a_uint32_t      nl_prot;

	/* chip specific parameter */
	void *          chip_spec_cfg;
	/* port cfg */
	ssdk_port_cfg   port_cfg;
	a_uint32_t      mac_mode;
	a_uint32_t led_source_num;
	led_source_cfg_t led_source_cfg[15];
	a_uint32_t      phy_id;
	a_uint32_t      mac_mode1;
	a_uint32_t      mac_mode2;
} ssdk_init_cfg;

	typedef struct {
		a_uint16_t ucastq_start;
		a_uint16_t ucastq_end;
		a_uint16_t mcastq_start;
		a_uint16_t mcastq_end;
		a_uint8_t l0sp_start;
		a_uint8_t l0sp_end;
		a_uint8_t l0cdrr_start;
		a_uint8_t l0cdrr_end;
		a_uint8_t l0edrr_start;
		a_uint8_t l0edrr_end;
		a_uint8_t l1cdrr_start;
		a_uint8_t l1cdrr_end;
		a_uint8_t l1edrr_start;
		a_uint8_t l1edrr_end;
	} ssdk_dt_portscheduler_cfg;

	typedef struct {
		a_uint8_t valid;
		a_uint8_t port_id;
		a_uint8_t cpri;
		a_uint8_t cdrr_id;
		a_uint8_t epri;
		a_uint8_t edrr_id;
		a_uint8_t sp_id;
	} ssdk_dt_l0scheduler_cfg;

	typedef struct {
		a_uint8_t valid;
		a_uint8_t port_id;
		a_uint8_t cpri;
		a_uint8_t cdrr_id;
		a_uint8_t epri;
		a_uint8_t edrr_id;
	} ssdk_dt_l1scheduler_cfg;

	typedef struct {
		ssdk_dt_portscheduler_cfg pool[SSDK_MAX_PORT_NUM];
		ssdk_dt_l0scheduler_cfg l0cfg[SSDK_L0SCHEDULER_CFG_MAX];
		ssdk_dt_l1scheduler_cfg l1cfg[SSDK_L1SCHEDULER_CFG_MAX];
	} ssdk_dt_scheduler_cfg;

	typedef struct
	{
		a_uint32_t switchreg_base_addr;
		a_uint32_t switchreg_size;
		a_uint32_t psgmiireg_base_addr;
		a_uint32_t psgmiireg_size;
		a_uint8_t *reg_access_mode;
		a_uint8_t *psgmii_reg_access_str;
		hsl_reg_mode switch_reg_access_mode;
		hsl_reg_mode psgmii_reg_access_mode;
		struct clk *ess_clk;
		struct clk *cmnblk_clk;
		ssdk_port_cfg   port_cfg;
		a_uint32_t      mac_mode;
		a_uint32_t      mac_mode1;
		a_uint32_t      mac_mode2;
		a_uint32_t uniphyreg_base_addr;
		a_uint32_t uniphyreg_size;
		a_uint8_t *uniphy_access_mode;
		hsl_reg_mode uniphy_reg_access_mode;
		ssdk_dt_scheduler_cfg scheduler_cfg;
		a_uint8_t bm_tick_mode;
		a_uint8_t tm_tick_mode;
		a_bool_t ess_switch_flag;
		a_uint32_t device_id;
		struct device_node *of_node;
	} ssdk_dt_cfg;

	typedef struct
	{
		a_uint32_t num_devices;
		ssdk_dt_cfg **ssdk_dt_switch_nodes;
	} ssdk_dt_global_t;

#if defined ATHENA
#define def_init_cfg  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_2};
#elif defined GARUDA

#define def_init_cfg_cpu2  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_2,};

#define def_init_spec_cfg_cpu2 {.mac0_rgmii = A_TRUE, .mac5_rgmii = A_TRUE, \
                    .rx_delay_s0 = A_FALSE, .rx_delay_s1 = A_FALSE, \
                    .tx_delay_s0 = A_TRUE,  .tx_delay_s1 = A_FALSE,\
                    .rgmii_rxclk_delay = A_TRUE, .rgmii_txclk_delay = A_TRUE,\
                    .phy4_rx_delay = A_TRUE, .phy4_tx_delay = A_TRUE,}

#define def_init_cfg_cpu1  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_1,};

#define def_init_spec_cfg_cpu1 {.mac0_rgmii = A_TRUE, .mac5_rgmii = A_FALSE, \
                    .rx_delay_s0 = A_FALSE, .rx_delay_s1 = A_FALSE, \
                    .tx_delay_s0 = A_TRUE,  .tx_delay_s1 = A_FALSE,\
                    .rgmii_rxclk_delay = A_TRUE, .rgmii_txclk_delay = A_TRUE, \
                    .phy4_rx_delay = A_TRUE, .phy4_tx_delay = A_TRUE,}

#define def_init_cfg_cpu1plus  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_1_PLUS,};

#define def_init_spec_cfg_cpu1plus {.mac0_rgmii = A_TRUE, .mac5_rgmii = A_FALSE, \
                    .rx_delay_s0 = A_FALSE, .rx_delay_s1 = A_FALSE, \
                    .tx_delay_s0 = A_FALSE,  .tx_delay_s1 = A_FALSE,\
                    .rgmii_rxclk_delay = A_TRUE, .rgmii_txclk_delay = A_TRUE, \
                    .phy4_rx_delay = A_TRUE, .phy4_tx_delay = A_TRUE,}

#define def_init_cfg_nocpu  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_NO_CPU,};

#define def_init_spec_cfg_nocpu { .mac0_rgmii = A_FALSE, .mac5_rgmii = A_FALSE, \
                    .rx_delay_s0 = A_FALSE, .rx_delay_s1 = A_FALSE, \
                    .tx_delay_s0 = A_FALSE,  .tx_delay_s1 = A_FALSE,\
                    .rgmii_rxclk_delay = A_TRUE, .rgmii_txclk_delay = A_TRUE, \
                    .phy4_rx_delay = A_TRUE, .phy4_tx_delay = A_TRUE,}

#define def_init_cfg_cpu1_gmii  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_1,};

#define def_init_spec_cfg_cpu1_gmii {.mac0_rgmii = A_FALSE, .mac5_rgmii = A_FALSE, \
                    .rx_delay_s0 = A_FALSE, .rx_delay_s1 = A_FALSE, \
                    .tx_delay_s0 = A_TRUE,  .tx_delay_s1 = A_FALSE,\
                    .rgmii_rxclk_delay = A_TRUE, .rgmii_txclk_delay = A_TRUE, \
                    .phy4_rx_delay = A_TRUE, .phy4_tx_delay = A_TRUE,}

#define def_init_cfg def_init_cfg_cpu2
#define def_init_spec_cfg def_init_spec_cfg_cpu2

#elif defined SHIVA
#define def_init_cfg  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_2};
#elif defined HORUS
#define def_init_cfg  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_2};
#elif defined ISIS
#define def_init_cfg  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_2};
#elif defined ISISC
#define def_init_cfg  {.reg_mode = HSL_MDIO, .cpu_mode = HSL_CPU_2};
#endif

    typedef struct
    {
        a_bool_t in_acl;
        a_bool_t in_fdb;
        a_bool_t in_igmp;
        a_bool_t in_leaky;
        a_bool_t in_led;
        a_bool_t in_mib;
        a_bool_t in_mirror;
        a_bool_t in_misc;
        a_bool_t in_portcontrol;
        a_bool_t in_portvlan;
        a_bool_t in_qos;
        a_bool_t in_rate;
        a_bool_t in_stp;
        a_bool_t in_vlan;
        a_bool_t in_reduced_acl;
        a_bool_t in_ip;
        a_bool_t in_nat;
        a_bool_t in_cosmap;
        a_bool_t in_sec;
        a_bool_t in_trunk;
        a_bool_t in_nathelper;
        a_bool_t in_interfacectrl;
    } ssdk_features;

#define CFG_STR_SIZE 20
    typedef struct
    {
        a_uint8_t build_ver[CFG_STR_SIZE];
        a_uint8_t build_date[CFG_STR_SIZE];

        a_uint8_t chip_type[CFG_STR_SIZE]; //GARUDA
        a_uint8_t cpu_type[CFG_STR_SIZE];  //mips
        a_uint8_t os_info[CFG_STR_SIZE];   //OS=linux OS_VER=2_6

        a_bool_t  fal_mod;
        a_bool_t  kernel_mode;
        a_bool_t  uk_if;

        ssdk_features features;
        ssdk_init_cfg init_cfg;
    } ssdk_cfg_t;

sw_error_t
ssdk_init(a_uint32_t dev_id, ssdk_init_cfg *cfg);

sw_error_t
ssdk_hsl_access_mode_set(a_uint32_t dev_id, hsl_access_mode reg_mode);

a_uint32_t ssdk_dt_global_get_mac_mode(a_uint32_t dev_id, a_uint32_t index);
a_uint32_t ssdk_dt_global_set_mac_mode(a_uint32_t dev_id, a_uint32_t index, a_uint32_t mode);

a_uint32_t
qca_hppe_port_mac_type_get(a_uint32_t dev_id, a_uint32_t port_id);
a_uint32_t
qca_hppe_port_mac_type_set(a_uint32_t dev_id, a_uint32_t port_id, a_uint32_t port_type);

void
qca_mac_sw_sync_port_status_init(a_uint32_t dev_id);

ssdk_dt_scheduler_cfg *ssdk_bootup_shceduler_cfg_get(a_uint32_t dev_id);
struct qca_phy_priv* ssdk_phy_priv_data_get(a_uint32_t dev_id);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* _SSDK_INIT_H */
