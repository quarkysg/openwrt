/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#include "sw.h"
#include "ssdk_init.h"
#include "fal_init.h"
#include "fal.h"
#include "hsl.h"
#include "hsl_dev.h"
#include "ssdk_init.h"
#ifdef HPPE
#include "hppe_init.h"
#endif
#include <linux/kconfig.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/types.h>
//#include <asm/mach-types.h>
#include <generated/autoconf.h>
#include <linux/if_arp.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/string.h>
#if defined(ISIS) ||defined(ISISC) ||defined(GARUDA)
#include <f1_phy.h>
#endif
#if defined(ATHENA) ||defined(SHIVA) ||defined(HORUS)
#include <f2_phy.h>
#endif
#ifdef IN_MALIBU_PHY
#include <malibu_phy.h>
#endif
#if defined(CONFIG_OF) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0))
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>
#elif defined(CONFIG_OF) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <drivers/leds/leds-ipq40xx.h>
#include <linux/of_platform.h>
#include <linux/reset.h>
#else
#include <linux/ar8216_platform.h>
#include <drivers/net/phy/ar8216.h>
#include <drivers/net/ethernet/atheros/ag71xx/ag71xx.h>
#endif
#include "ssdk_plat.h"
#include "ssdk_clk.h"
#include "ref_vlan.h"
#include "ref_fdb.h"
#include "ref_mib.h"
#include "ref_port_ctrl.h"
#include "ref_misc.h"
#include "ref_uci.h"
#include "shell.h"
#ifdef BOARD_AR71XX
#include "ssdk_uci.h"
#endif

#ifdef IN_IP
#if defined (CONFIG_NF_FLOW_COOKIE)
#include "fal_flowcookie.h"
#ifdef IN_SFE
#include <shortcut-fe/sfe.h>
#endif
#endif
#endif

#ifdef IN_RFS
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
#include <linux/if_vlan.h>
#endif
#include <qca-rfs/rfs_dev.h>
#ifdef IN_IP
#include "fal_rfs.h"
#endif
#endif

#include "adpt.h"

static struct mii_bus *miibus = NULL;

extern struct qca_phy_priv **qca_phy_priv_global;
extern ssdk_dt_global_t ssdk_dt_global;
extern ssdk_chip_type SSDK_CURRENT_CHIP_TYPE;
void __iomem *hppe_uniphy_addr = NULL;

static struct mutex switch_mdio_lock;

#ifdef BOARD_IPQ806X
#define IPQ806X_MDIO_BUS_NAME			"mdio-gpio"
#endif

#ifdef BOARD_AR71XX
#define IPQ806X_MDIO_BUS_NAME			"ag71xx-mdio"
#endif
#define MDIO_BUS_0						0
#define MDIO_BUS_1						1
#define IPQ806X_MDIO_BUS_NUM			MDIO_BUS_0

#define ISIS_CHIP_ID 0x18
#define ISIS_CHIP_REG 0
#define SHIVA_CHIP_ID 0x1f
#define SHIVA_CHIP_REG 0x10

/*
 * Using ISIS's address as default
  */
static int switch_chip_id = ISIS_CHIP_ID;
static int switch_chip_reg = ISIS_CHIP_REG;

static int ssdk_dev_id = 0;

a_uint32_t ssdk_log_level = SSDK_LOG_LEVEL_DEFAULT;

#if defined(CONFIG_OF) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
struct ag71xx_mdio {
	struct mii_bus		*mii_bus;
	int			mii_irq[PHY_MAX_ADDR];
	void __iomem		*mdio_base;
};
#endif

#ifdef BOARD_AR71XX
static uint32_t switch_chip_id_adjuest(a_uint32_t dev_id)
{
	uint32_t chip_version = 0;
	chip_version = (qca_ar8216_mii_read(dev_id, 0)&0xff00)>>8;
	if((chip_version !=0) && (chip_version !=0xff))
		return 0;

	switch_chip_id = SHIVA_CHIP_ID;
	switch_chip_reg = SHIVA_CHIP_REG;

	chip_version = (qca_ar8216_mii_read(dev_id, 0)&0xff00)>>8;
	printk("chip_version:0x%x\n", chip_version);
	return 1;
}
#endif

static inline void
split_addr(uint32_t regaddr, uint16_t *r1, uint16_t *r2, uint16_t *page)
{
	regaddr >>= 1;
	*r1 = regaddr & 0x1e;

	regaddr >>= 5;
	*r2 = regaddr & 0x7;

	regaddr >>= 3;
	*page = regaddr & 0x3ff;
}

a_uint32_t
qca_ar8216_mii_read(a_uint32_t dev_id, a_uint32_t reg)
{
        struct mii_bus *bus = miibus;
        uint16_t r1, r2, page;
        uint16_t lo, hi;

	if (!bus)
		bus = qca_phy_priv_global[dev_id]->miibus;

        split_addr((uint32_t) reg, &r1, &r2, &page);
        mutex_lock(&switch_mdio_lock);
        mdiobus_write(bus, switch_chip_id, switch_chip_reg, page);
	udelay(100);
        lo = mdiobus_read(bus, 0x10 | r2, r1);
        hi = mdiobus_read(bus, 0x10 | r2, r1 + 1);
        mutex_unlock(&switch_mdio_lock);
        return (hi << 16) | lo;
}

void
qca_ar8216_mii_write(a_uint32_t dev_id, a_uint32_t reg, a_uint32_t val)
{
        struct mii_bus *bus = miibus;
        uint16_t r1, r2, r3;
        uint16_t lo, hi;

	if (!bus)
		bus = qca_phy_priv_global[dev_id]->miibus;

        split_addr((a_uint32_t) reg, &r1, &r2, &r3);
        lo = val & 0xffff;
        hi = (a_uint16_t) (val >> 16);

        mutex_lock(&switch_mdio_lock);
        mdiobus_write(bus, switch_chip_id, switch_chip_reg, r3);
	udelay(100);
        if(SSDK_CURRENT_CHIP_TYPE != CHIP_SHIVA) {
            mdiobus_write(bus, 0x10 | r2, r1, lo);
            mdiobus_write(bus, 0x10 | r2, r1 + 1, hi);
        } else {
            mdiobus_write(bus, 0x10 | r2, r1 + 1, hi);
            mdiobus_write(bus, 0x10 | r2, r1, lo);
        }
        mutex_unlock(&switch_mdio_lock);
}

a_bool_t
phy_addr_validation_check(a_uint32_t phy_addr)
{

	if ((phy_addr > SSDK_PHY_BCAST_ID) || (phy_addr < SSDK_PHY_MIN_ID))
		return A_FALSE;
	else
		return A_TRUE;
}

sw_error_t
qca_ar8327_phy_read(a_uint32_t dev_id, a_uint32_t phy_addr,
                           a_uint32_t reg, a_uint16_t* data)
{
	struct mii_bus *bus = miibus;

	if (!bus)
		bus = qca_phy_priv_global[dev_id]->miibus;

	if (A_TRUE != phy_addr_validation_check (phy_addr))
	{
		return SW_BAD_PARAM;
	}

	*data = mdiobus_read(bus, phy_addr, reg);
	return 0;
}

sw_error_t
qca_ar8327_phy_write(a_uint32_t dev_id, a_uint32_t phy_addr,
                            a_uint32_t reg, a_uint16_t data)
{
	struct mii_bus *bus = miibus;

	if (!bus)
		bus = qca_phy_priv_global[dev_id]->miibus;

	if (A_TRUE != phy_addr_validation_check (phy_addr))
	{
		return SW_BAD_PARAM;
	}

	mdiobus_write(bus, phy_addr, reg, data);
	return 0;
}

void
qca_ar8327_phy_dbg_write(a_uint32_t dev_id, a_uint32_t phy_addr,
                                a_uint16_t dbg_addr, a_uint16_t dbg_data)
{
	struct mii_bus *bus = miibus;

	if (!bus)
		bus = qca_phy_priv_global[dev_id]->miibus;

	if (A_TRUE != phy_addr_validation_check (phy_addr))
	{
		return;
	}

	mdiobus_write(bus, phy_addr, QCA_MII_DBG_ADDR, dbg_addr);
	mdiobus_write(bus, phy_addr, QCA_MII_DBG_DATA, dbg_data);
}

void
qca_ar8327_phy_dbg_read(a_uint32_t dev_id, a_uint32_t phy_addr,
		                a_uint16_t dbg_addr, a_uint16_t *dbg_data)
{
	struct mii_bus *bus = miibus;

	if (!bus)
		bus = qca_phy_priv_global[dev_id]->miibus;

	if (A_TRUE != phy_addr_validation_check (phy_addr))
	{
		return;
	}

	mdiobus_write(bus, phy_addr, QCA_MII_DBG_ADDR, dbg_addr);
	*dbg_data = mdiobus_read(bus, phy_addr, QCA_MII_DBG_DATA);
}


void
qca_ar8327_mmd_write(a_uint32_t dev_id, a_uint32_t phy_addr,
                          a_uint16_t addr, a_uint16_t data)
{
	struct mii_bus *bus = miibus;

	if (!bus)
		bus = qca_phy_priv_global[dev_id]->miibus;

	if (A_TRUE != phy_addr_validation_check (phy_addr))
	{
		return;
	}

	mdiobus_write(bus, phy_addr, QCA_MII_MMD_ADDR, addr);
	mdiobus_write(bus, phy_addr, QCA_MII_MMD_DATA, data);
}

void qca_phy_mmd_write(u32 dev_id, u32 phy_id,
                     u16 mmd_num, u16 reg_id, u16 reg_val)
{
	qca_ar8327_phy_write(dev_id, phy_id,
			QCA_MII_MMD_ADDR, mmd_num);
	qca_ar8327_phy_write(dev_id, phy_id,
			QCA_MII_MMD_DATA, reg_id);
	qca_ar8327_phy_write(dev_id, phy_id,
			QCA_MII_MMD_ADDR,
			0x4000 | mmd_num);
	qca_ar8327_phy_write(dev_id, phy_id,
		QCA_MII_MMD_DATA, reg_val);
}

u16 qca_phy_mmd_read(u32 dev_id, u32 phy_id,
		u16 mmd_num, u16 reg_id)
{
	u16 value = 0;
	qca_ar8327_phy_write(dev_id, phy_id,
			QCA_MII_MMD_ADDR, mmd_num);
	qca_ar8327_phy_write(dev_id, phy_id,
			QCA_MII_MMD_DATA, reg_id);
	qca_ar8327_phy_write(dev_id, phy_id,
			QCA_MII_MMD_ADDR,
			0x4000 | mmd_num);
	qca_ar8327_phy_read(dev_id, phy_id,
			QCA_MII_MMD_DATA, &value);
	return value;
}

sw_error_t
qca_xgphy_read(a_uint32_t dev_id, a_uint32_t phy_addr,
                           a_uint32_t reg, a_uint16_t* data)
{
	struct mii_bus *bus = miibus;

	reg = MII_PHYADDR_C45 | reg;
	*data = mdiobus_read(bus, phy_addr, reg);

	return 0;
}
sw_error_t
qca_xgphy_write(a_uint32_t dev_id, a_uint32_t phy_addr,
                            a_uint32_t reg, a_uint16_t data)
{
	struct mii_bus *bus = miibus;

	reg = MII_PHYADDR_C45 | reg;
	mdiobus_write(bus, phy_addr, reg, data);

	return 0;
}
sw_error_t
qca_switch_reg_read(a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t * reg_data, a_uint32_t len)
{
	uint32_t reg_val = 0;

	if (len != sizeof (a_uint32_t))
        return SW_BAD_LEN;

	if ((reg_addr%4)!= 0)
	return SW_BAD_PARAM;

	reg_val = readl(qca_phy_priv_global[dev_id]->hw_addr + reg_addr);

	aos_mem_copy(reg_data, &reg_val, sizeof (a_uint32_t));
	return 0;
}

sw_error_t
qca_switch_reg_write(a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t * reg_data, a_uint32_t len)
{
	uint32_t reg_val = 0;
	if (len != sizeof (a_uint32_t))
        return SW_BAD_LEN;

	if ((reg_addr%4)!= 0)
	return SW_BAD_PARAM;

	aos_mem_copy(&reg_val, reg_data, sizeof (a_uint32_t));
	writel(reg_val, qca_phy_priv_global[dev_id]->hw_addr + reg_addr);
	return 0;
}

sw_error_t
qca_psgmii_reg_read(a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t * reg_data, a_uint32_t len)
{
	uint32_t reg_val = 0;

	if (len != sizeof (a_uint32_t))
        return SW_BAD_LEN;

	if((reg_addr%4)!=0)
	return SW_BAD_PARAM;

	if (qca_phy_priv_global[dev_id]->psgmii_hw_addr == NULL)
		return SW_NOT_SUPPORTED;

	reg_val = readl(qca_phy_priv_global[dev_id]->psgmii_hw_addr + reg_addr);

	aos_mem_copy(reg_data, &reg_val, sizeof (a_uint32_t));
	return 0;
}

sw_error_t
qca_psgmii_reg_write(a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t * reg_data, a_uint32_t len)
{
	uint32_t reg_val = 0;
	if (len != sizeof (a_uint32_t))
        return SW_BAD_LEN;

	if((reg_addr%4)!=0)
	return SW_BAD_PARAM;

	if (qca_phy_priv_global[dev_id]->psgmii_hw_addr == NULL)
		return SW_NOT_SUPPORTED;

	aos_mem_copy(&reg_val, reg_data, sizeof (a_uint32_t));
	writel(reg_val, qca_phy_priv_global[dev_id]->psgmii_hw_addr + reg_addr);
	return 0;
}

sw_error_t
qca_uniphy_reg_read(a_uint32_t dev_id, a_uint32_t uniphy_index,
				a_uint32_t reg_addr, a_uint8_t * reg_data, a_uint32_t len)
{
#ifdef HPPE
	uint32_t reg_val = 0;
	void __iomem *hppe_uniphy_base = NULL;
	a_uint32_t reg_addr1, reg_addr2;

	if (len != sizeof (a_uint32_t))
        return SW_BAD_LEN;

	if (SSDK_UNIPHY_INSTANCE0 == uniphy_index)
		hppe_uniphy_base = hppe_uniphy_addr;
	else if (SSDK_UNIPHY_INSTANCE1 == uniphy_index)
		hppe_uniphy_base = hppe_uniphy_addr + HPPE_UNIPHY_BASE1;

	else if (SSDK_UNIPHY_INSTANCE2 == uniphy_index)
		hppe_uniphy_base = hppe_uniphy_addr + HPPE_UNIPHY_BASE2;
	else
		return SW_BAD_PARAM;

	if ( reg_addr > HPPE_UNIPHY_MAX_DIRECT_ACCESS_REG)
	{
		// uniphy reg indireclty access
		reg_addr1 = (reg_addr & 0xffffff) >> 8;
		writel(reg_addr1, hppe_uniphy_base + HPPE_UNIPHY_INDIRECT_REG_ADDR);

		reg_addr2 = reg_addr & HPPE_UNIPHY_INDIRECT_LOW_ADDR;
		reg_addr = (HPPE_UNIPHY_INDIRECT_DATA << 10) | (reg_addr2 << 2);

		reg_val = readl(hppe_uniphy_base + reg_addr);
		aos_mem_copy(reg_data, &reg_val, sizeof (a_uint32_t));
	}
	else
	{	// uniphy reg directly access
		reg_val = readl(hppe_uniphy_base + reg_addr);
		aos_mem_copy(reg_data, &reg_val, sizeof (a_uint32_t));
	}
#endif
	return 0;
}

sw_error_t
qca_uniphy_reg_write(a_uint32_t dev_id, a_uint32_t uniphy_index,
				a_uint32_t reg_addr, a_uint8_t * reg_data, a_uint32_t len)
{
#ifdef HPPE
	void __iomem *hppe_uniphy_base = NULL;
	a_uint32_t reg_addr1, reg_addr2;
	uint32_t reg_val = 0;

	if (len != sizeof (a_uint32_t))
        return SW_BAD_LEN;

	if (SSDK_UNIPHY_INSTANCE0 == uniphy_index)
		hppe_uniphy_base = hppe_uniphy_addr;
	else if (SSDK_UNIPHY_INSTANCE1 == uniphy_index)
		hppe_uniphy_base = hppe_uniphy_addr + HPPE_UNIPHY_BASE1;

	else if (SSDK_UNIPHY_INSTANCE2 == uniphy_index)
		hppe_uniphy_base = hppe_uniphy_addr + HPPE_UNIPHY_BASE2;
	else
		return SW_BAD_PARAM;

	if ( reg_addr > HPPE_UNIPHY_MAX_DIRECT_ACCESS_REG)
	{
		// uniphy reg indireclty access
		reg_addr1 = (reg_addr & 0xffffff) >> 8;
		writel(reg_addr1, hppe_uniphy_base + HPPE_UNIPHY_INDIRECT_REG_ADDR);

		reg_addr2 = reg_addr & HPPE_UNIPHY_INDIRECT_LOW_ADDR;
		reg_addr = (HPPE_UNIPHY_INDIRECT_DATA << 10) | (reg_addr2 << 2);
		aos_mem_copy(&reg_val, reg_data, sizeof (a_uint32_t));
		writel(reg_val, hppe_uniphy_base + reg_addr);
	}
	else
	{	// uniphy reg directly access
		aos_mem_copy(&reg_val, reg_data, sizeof (a_uint32_t));
		writel(reg_val, hppe_uniphy_base + reg_addr);
	}
#endif
	return 0;
}

static int miibus_get(a_uint32_t dev_id)
{
#if defined(CONFIG_OF) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
	#ifndef BOARD_AR71XX
	struct device_node *mdio_node = NULL;
	struct device_node *switch_node = NULL;
	struct platform_device *mdio_plat = NULL;
	struct ipq40xx_mdio_data *mdio_data = NULL;
	struct qca_phy_priv *priv;

	priv = qca_phy_priv_global[dev_id];
	switch_node = qca_phy_priv_global[dev_id]->of_node;
	if (priv && switch_node) {
		mdio_node = of_parse_phandle(switch_node, "mdio-bus", 0);
		if (mdio_node) {
			priv->miibus = of_mdio_find_bus(mdio_node);
			return 0;
		}
	}

	if(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switch_reg_access_mode == HSL_REG_LOCAL_BUS)
		mdio_node = of_find_compatible_node(NULL, NULL, "qcom,ipq40xx-mdio");
	else
		mdio_node = of_find_compatible_node(NULL, NULL, "virtual,mdio-gpio");

	if (!mdio_node) {
		printk("No MDIO node found in DTS!\n");
		return 0;
	}

	mdio_plat = of_find_device_by_node(mdio_node);
	if (!mdio_plat) {
		printk("cannot find platform device from mdio node\n");
		return 1;
	}

	if(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switch_reg_access_mode == HSL_REG_LOCAL_BUS)
	{
		mdio_data = dev_get_drvdata(&mdio_plat->dev);
		if (!mdio_data) {
                	printk("cannot get mdio_data reference from device data\n");
                	return 0;
        	}
		miibus = mdio_data->mii_bus;
	}
	else
		miibus = dev_get_drvdata(&mdio_plat->dev);

	if (!miibus) {
		printk("cannot get mii bus reference from device data\n");
		return 1;
	}

	#else
	struct ag71xx_mdio *am;
	struct device_node *mdio_node = NULL;
	struct platform_device *mdio_plat = NULL;

	mdio_node = of_find_compatible_node(NULL, NULL, "qca,ag71xx-mdio");
	if (!mdio_node) {
		printk("No MDIO node found in DTS!\n");
		return 1;
	}
	mdio_plat = of_find_device_by_node(mdio_node);
	if (!mdio_plat) {
		printk("cannot find platform device from mdio node\n");
		return 1;
	}
	am = dev_get_drvdata(&mdio_plat->dev);
	if (!am) {
                	printk("cannot get mdio_data reference from device data\n");
                	return 1;
	}
	miibus = am->mii_bus;
	if (!miibus) {
		printk("cannot get mii bus reference from device data\n");
		return 1;
	}
	switch_chip_id_adjuest(dev_id);
	#endif
#else
#ifdef BOARD_AR71XX
	struct ag71xx_mdio *am;
#endif
	struct device *miidev;
	char busid[MII_BUS_ID_SIZE];
	snprintf(busid, MII_BUS_ID_SIZE, "%s.%d",
		IPQ806X_MDIO_BUS_NAME, IPQ806X_MDIO_BUS_NUM);

	miidev = bus_find_device_by_name(&platform_bus_type, NULL, busid);
	if (!miidev) {
		printk("cannot get mii bus\n");
		return 1;
	}

#ifdef BOARD_AR71XX
	am = dev_get_drvdata(miidev);
	miibus = am->mii_bus;
#else
	miibus = dev_get_drvdata(miidev);
#endif

#ifdef BOARD_AR71XX
	if(switch_chip_id_adjuest(dev_id)) {

		snprintf(busid, MII_BUS_ID_SIZE, "%s.%d",
		IPQ806X_MDIO_BUS_NAME, MDIO_BUS_1);

		miidev = bus_find_device_by_name(&platform_bus_type, NULL, busid);
		if (!miidev) {
			printk("cannot get mii bus\n");
			return 1;
		}

		am = dev_get_drvdata(miidev);
		miibus = am->mii_bus;
		printk("chip_version:0x%x\n", (qca_ar8216_mii_read(dev_id, 0)&0xff00)>>8);
	}
#endif

	if(!miidev){
		printk("mdio bus '%s' get FAIL\n", busid);
		return 1;
	}
#endif

	return 0;
}

static ssize_t ssdk_dev_id_get(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t count;
	a_uint32_t num;

	num = (a_uint32_t)ssdk_dev_id;

	count = snprintf(buf, (ssize_t)PAGE_SIZE, "%u", num);
	return count;
}

static ssize_t ssdk_dev_id_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char num_buf[12];
	a_uint32_t num;

	if (count >= sizeof(num_buf)) return 0;
	memcpy(num_buf, buf, count);
	num_buf[count] = '\0';
	sscanf(num_buf, "%u", &num);

	ssdk_dev_id = num;

	return count;
}

static ssize_t ssdk_log_level_get(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t count;
	a_uint32_t num;

	num = ssdk_log_level;

	count = snprintf(buf, (ssize_t)PAGE_SIZE, "%u", num);
	return count;
}

static ssize_t ssdk_log_level_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char num_buf[12];
	a_uint32_t num;

	if (count >= sizeof(num_buf))
		return 0;
	memcpy(num_buf, buf, count);
	num_buf[count] = '\0';
	sscanf(num_buf, "%u", &num);

	ssdk_log_level = (a_uint32_t)num;

	return count;
}

static ssize_t ssdk_packet_counter_get(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t count;
	adpt_api_t *p_api;

	p_api = adpt_api_ptr_get(0);
	if (!p_api)
	{
		count = snprintf(buf, (ssize_t)PAGE_SIZE, "just support hppe");
		return count;
	}

	count = snprintf(buf, (ssize_t)PAGE_SIZE, " ");

	p_api->adpt_debug_counter_get(A_FALSE);

	return count;
}

static ssize_t ssdk_packet_counter_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char num_buf[12];
	adpt_api_t *p_api;

	p_api = adpt_api_ptr_get(0);
	if (!p_api)
		return count;

	p_api->adpt_debug_counter_set();

	if (count >= sizeof(num_buf))
		return 0;
	memcpy(num_buf, buf, count);
	num_buf[count] = '\0';


	return count;
}

static ssize_t ssdk_byte_counter_get(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t count;
	adpt_api_t *p_api;

	p_api = adpt_api_ptr_get(0);
	if (!p_api)
	{
		count = snprintf(buf, (ssize_t)PAGE_SIZE, "just support hppe");
		return count;
	}

	count = snprintf(buf, (ssize_t)PAGE_SIZE, " ");

	p_api->adpt_debug_counter_get(A_TRUE);

	return count;
}

static ssize_t ssdk_byte_counter_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char num_buf[12];
	adpt_api_t *p_api;

	p_api = adpt_api_ptr_get(0);
	if (!p_api)
		return count;

	p_api->adpt_debug_counter_set();

	if (count >= sizeof(num_buf))
		return 0;
	memcpy(num_buf, buf, count);
	num_buf[count] = '\0';


	return count;
}

void ssdk_dts_port_scheduler_dump(a_uint32_t dev_id)
{
	a_uint32_t i;
	ssdk_dt_portscheduler_cfg *portscheduler_cfg;
	a_uint8_t srcmsg[7][16];
	printk("===============================port_scheduler_resource===========================\n");
	printk("portid     ucastq     mcastq     10sp     10cdrr     10edrr     11cdrr     11edrr\n");
	for (i = 0; i < SSDK_MAX_PORT_NUM; i++)
	{
		portscheduler_cfg = &ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->scheduler_cfg.pool[i];
		snprintf(srcmsg[0], sizeof(srcmsg[0]), "<%d %d>", portscheduler_cfg->ucastq_start,
				portscheduler_cfg->ucastq_end);
		snprintf(srcmsg[1], sizeof(srcmsg[1]), "<%d %d>", portscheduler_cfg->mcastq_start,
				portscheduler_cfg->mcastq_end);
		snprintf(srcmsg[2], sizeof(srcmsg[2]), "<%d %d>", portscheduler_cfg->l0sp_start,
				portscheduler_cfg->l0sp_end);
		snprintf(srcmsg[3], sizeof(srcmsg[3]), "<%d %d>", portscheduler_cfg->l0cdrr_start,
				portscheduler_cfg->l0cdrr_end);
		snprintf(srcmsg[4], sizeof(srcmsg[4]), "<%d %d>", portscheduler_cfg->l0edrr_start,
				portscheduler_cfg->l0edrr_end);
		snprintf(srcmsg[5], sizeof(srcmsg[5]), "<%d %d>", portscheduler_cfg->l1cdrr_start,
				portscheduler_cfg->l1cdrr_end);
		snprintf(srcmsg[6], sizeof(srcmsg[6]), "<%d %d>", portscheduler_cfg->l1edrr_start,
				portscheduler_cfg->l1edrr_end);
		printk("%6d%11s%11s%9s%11s%11s%11s%11s\n", i, srcmsg[0], srcmsg[1], srcmsg[2], srcmsg[3],
				srcmsg[4], srcmsg[5], srcmsg[6]);
	}
}

void ssdk_dts_l0scheduler_dump(a_uint32_t dev_id)
{
	a_uint32_t i;
	ssdk_dt_l0scheduler_cfg *scheduler_cfg;
	printk("==========================l0scheduler_cfg===========================\n");
	printk("queue     portid     cpri     cdrr_id     epri     edrr_id     sp_id\n");
	for (i = 0; i < SSDK_L0SCHEDULER_CFG_MAX; i++)
	{
		scheduler_cfg = &ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->scheduler_cfg.l0cfg[i];
		if (scheduler_cfg->valid == 1)
			printk("%5d%11d%9d%12d%9d%12d%10d\n", i, scheduler_cfg->port_id,
				scheduler_cfg->cpri, scheduler_cfg->cdrr_id, scheduler_cfg->epri,
				scheduler_cfg->edrr_id, scheduler_cfg->sp_id);
	}
}

void ssdk_dts_l1scheduler_dump(a_uint32_t dev_id)
{
	a_uint32_t i;
	ssdk_dt_l1scheduler_cfg *scheduler_cfg;
	printk("=====================l1scheduler_cfg=====================\n");
	printk("flow     portid     cpri     cdrr_id     epri     edrr_id\n");
	for (i = 0; i < SSDK_L1SCHEDULER_CFG_MAX; i++)
	{
		scheduler_cfg = &ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->scheduler_cfg.l1cfg[i];
		if (scheduler_cfg->valid == 1)
			printk("%4d%11d%9d%12d%9d%12d\n", i, scheduler_cfg->port_id,
				scheduler_cfg->cpri, scheduler_cfg->cdrr_id,
				scheduler_cfg->epri, scheduler_cfg->edrr_id);
	}
}

static ssize_t ssdk_dts_dump(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t count;
	a_uint32_t dev_id;
	ssdk_dt_cfg *dt_cfg;

	count = snprintf(buf, (ssize_t)PAGE_SIZE, " ");

	for (dev_id = 0; dev_id < ssdk_dt_global.num_devices; dev_id ++)
	{
		dt_cfg = ssdk_dt_global.ssdk_dt_switch_nodes[dev_id];
		printk("=======================================================\n");
		printk("ess-switch\n");
		printk("        reg = <0x%x 0x%x>\n", dt_cfg->switchreg_base_addr, dt_cfg->switchreg_size);
		printk("        switch_access_mode = <%s>\n", dt_cfg->reg_access_mode);
		printk("        switch_cpu_bmp = <0x%x>\n", dt_cfg->port_cfg.cpu_bmp);
		printk("        switch_lan_bmp = <0x%x>\n", dt_cfg->port_cfg.lan_bmp);
		printk("        switch_wan_bmp = <0x%x>\n", dt_cfg->port_cfg.wan_bmp);
		printk("        switch_mac_mode = <0x%x>\n", dt_cfg->mac_mode);
		printk("        switch_mac_mode1 = <0x%x>\n", dt_cfg->mac_mode1);
		printk("        switch_mac_mode2 = <0x%x>\n", dt_cfg->mac_mode2);
		printk("        bm_tick_mode = <0x%x>\n", dt_cfg->bm_tick_mode);
		printk("        tm_tick_mode = <0x%x>\n", dt_cfg->tm_tick_mode);

		printk("ess-psgmii\n");
		printk("        reg = <0x%x 0x%x>\n", dt_cfg->psgmiireg_base_addr, dt_cfg->psgmiireg_size);
		printk("        psgmii_access_mode = <%s>\n", dt_cfg->psgmii_reg_access_str);

		printk("ess-uniphy\n");
		printk("        reg = <0x%x 0x%x>\n", dt_cfg->uniphyreg_base_addr, dt_cfg->uniphyreg_size);
		printk("        uniphy_access_mode = <%s>\n", dt_cfg->uniphy_access_mode);

		printk("\n");
		ssdk_dts_port_scheduler_dump(dev_id);
		printk("\n");
		ssdk_dts_l0scheduler_dump(dev_id);
		printk("\n");
		ssdk_dts_l1scheduler_dump(dev_id);
	}

	return count;
}

static a_uint16_t phy_reg_val = 0;
static ssize_t ssdk_phy_write_reg_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char phy_buf[32];
	char *this_opt;
	char *options = phy_buf;
	unsigned int phy_addr, reg_addr, reg_value;

	if (count >= sizeof(phy_buf))
		return 0;
	memcpy(phy_buf, buf, count);
	phy_buf[count] = '\0';

	this_opt = strsep(&options, " ");
	if (!this_opt)
		goto fail;

	kstrtouint(this_opt, 0, &phy_addr);
	if ((options - phy_buf) >= (count - 1))
		goto fail;

	this_opt = strsep(&options, " ");
	if (!this_opt)
		goto fail;

	kstrtouint(this_opt, 0, &reg_addr);
	if ((options - phy_buf) >= (count - 1))
		goto fail;

	this_opt = strsep(&options, " ");
	if (!this_opt)
		goto fail;

	kstrtouint(this_opt, 0, &reg_value);

	qca_ar8327_phy_write(0, phy_addr, reg_addr, reg_value);

	return count;

fail:
	printk("Format: phy_addr reg_addr reg_value\n");
	return -EINVAL;
}

static ssize_t ssdk_phy_read_reg_get(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t count;

	count = snprintf(buf, (ssize_t)PAGE_SIZE, "reg_val = 0x%x\n", phy_reg_val);
	return count;
}

static ssize_t ssdk_phy_read_reg_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char phy_buf[32];
	char *this_opt;
	char *options = phy_buf;
	unsigned int phy_addr, reg_addr;

	if (count >= sizeof(phy_buf))
		return 0;
	memcpy(phy_buf, buf, count);
	phy_buf[count] = '\0';

	this_opt = strsep(&options, " ");
	if (!this_opt)
		goto fail;

	kstrtouint(this_opt, 0, &phy_addr);
	if ((options - phy_buf) >= (count - 1))
		goto fail;

	this_opt = strsep(&options, " ");
	if (!this_opt)
		goto fail;

	kstrtouint(this_opt, 0, &reg_addr);

	qca_ar8327_phy_read(0, phy_addr, reg_addr, &phy_reg_val);

	return count;

fail:
	printk("Format: phy_addr reg_addr\n");
	return -EINVAL;
}

static const struct device_attribute ssdk_dev_id_attr =
	__ATTR(dev_id, 0660, ssdk_dev_id_get, ssdk_dev_id_set);
static const struct device_attribute ssdk_log_level_attr =
	__ATTR(log_level, 0660, ssdk_log_level_get, ssdk_log_level_set);
static const struct device_attribute ssdk_packet_counter_attr =
	__ATTR(packet_counter, 0660, ssdk_packet_counter_get, ssdk_packet_counter_set);
static const struct device_attribute ssdk_byte_counter_attr =
	__ATTR(byte_counter, 0660, ssdk_byte_counter_get, ssdk_byte_counter_set);
static const struct device_attribute ssdk_dts_dump_attr =
	__ATTR(dts_dump, 0660, ssdk_dts_dump, NULL);
static const struct device_attribute ssdk_phy_write_reg_attr =
	__ATTR(phy_write_reg, 0660, NULL, ssdk_phy_write_reg_set);
static const struct device_attribute ssdk_phy_read_reg_attr =
	__ATTR(phy_read_reg, 0660, ssdk_phy_read_reg_get, ssdk_phy_read_reg_set);
struct kobject *ssdk_sys = NULL;

int ssdk_sysfs_init (void)
{
	int ret = 0;

	/* create /sys/ssdk/ dir */
	ssdk_sys = kobject_create_and_add("ssdk", NULL);
	if (!ssdk_sys) {
		printk("Failed to register SSDK sysfs\n");
		return ret;
	}

	/* create /sys/ssdk/dev_id file */
	ret = sysfs_create_file(ssdk_sys, &ssdk_dev_id_attr.attr);
	if (ret) {
		printk("Failed to register SSDK dev id SysFS file\n");
		goto CLEANUP_1;
	}

	/* create /sys/ssdk/log_level file */
	ret = sysfs_create_file(ssdk_sys, &ssdk_log_level_attr.attr);
	if (ret) {
		printk("Failed to register SSDK log level SysFS file\n");
		goto CLEANUP_2;
	}

	/* create /sys/ssdk/packet_counter file */
	ret = sysfs_create_file(ssdk_sys, &ssdk_packet_counter_attr.attr);
	if (ret) {
		printk("Failed to register SSDK switch counter SysFS file\n");
		goto CLEANUP_3;
	}

	/* create /sys/ssdk/byte_counter file */
	ret = sysfs_create_file(ssdk_sys, &ssdk_byte_counter_attr.attr);
	if (ret) {
		printk("Failed to register SSDK switch counter bytes SysFS file\n");
		goto CLEANUP_4;
	}

	/* create /sys/ssdk/dts_dump file */
	ret = sysfs_create_file(ssdk_sys, &ssdk_dts_dump_attr.attr);
	if (ret) {
		printk("Failed to register SSDK switch show dts SysFS file\n");
		goto CLEANUP_5;
	}

	/* create /sys/ssdk/phy_write_reg file */
	ret = sysfs_create_file(ssdk_sys, &ssdk_phy_write_reg_attr.attr);
	if (ret) {
		printk("Failed to register SSDK phy write reg file\n");
		goto CLEANUP_6;
	}

	/* create /sys/ssdk/phy_read_reg file */
	ret = sysfs_create_file(ssdk_sys, &ssdk_phy_read_reg_attr.attr);
	if (ret) {
		printk("Failed to register SSDK phy read reg file\n");
		goto CLEANUP_7;
	}

	return 0;

CLEANUP_7:
	sysfs_remove_file(ssdk_sys, &ssdk_phy_write_reg_attr.attr);
CLEANUP_6:
	sysfs_remove_file(ssdk_sys, &ssdk_dts_dump_attr.attr);
CLEANUP_5:
	sysfs_remove_file(ssdk_sys, &ssdk_byte_counter_attr.attr);
CLEANUP_4:
	sysfs_remove_file(ssdk_sys, &ssdk_packet_counter_attr.attr);
CLEANUP_3:
	sysfs_remove_file(ssdk_sys, &ssdk_log_level_attr.attr);
CLEANUP_2:
	sysfs_remove_file(ssdk_sys, &ssdk_dev_id_attr.attr);
CLEANUP_1:
	kobject_put(ssdk_sys);

	return ret;
}

void ssdk_sysfs_exit (void)
{
	sysfs_remove_file(ssdk_sys, &ssdk_phy_read_reg_attr.attr);
	sysfs_remove_file(ssdk_sys, &ssdk_phy_write_reg_attr.attr);
	sysfs_remove_file(ssdk_sys, &ssdk_dts_dump_attr.attr);
	sysfs_remove_file(ssdk_sys, &ssdk_byte_counter_attr.attr);
	sysfs_remove_file(ssdk_sys, &ssdk_packet_counter_attr.attr);
	sysfs_remove_file(ssdk_sys, &ssdk_log_level_attr.attr);
	sysfs_remove_file(ssdk_sys, &ssdk_dev_id_attr.attr);
	kobject_put(ssdk_sys);
}


int
ssdk_plat_init(ssdk_init_cfg *cfg, a_uint32_t dev_id)
{
	#ifdef BOARD_AR71XX
	int rv = 0;
	#endif
	printk("ssdk_plat_init start\n");
	mutex_init(&switch_mdio_lock);

	if(miibus_get(dev_id))
		return -ENODEV;

	if(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->uniphy_reg_access_mode == HSL_REG_LOCAL_BUS) {
		hppe_uniphy_addr = ioremap_nocache(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->uniphyreg_base_addr,
					ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->uniphyreg_size);
		if (!hppe_uniphy_addr) {
			printk("%s ioremap fail.", __func__);
			cfg->reg_func.uniphy_reg_set = NULL;
			cfg->reg_func.uniphy_reg_get = NULL;
			return -1;
		}
		cfg->reg_func.uniphy_reg_set = qca_uniphy_reg_write;
		cfg->reg_func.uniphy_reg_get = qca_uniphy_reg_read;
	}
	if(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switch_reg_access_mode == HSL_REG_LOCAL_BUS) {
		qca_phy_priv_global[dev_id]->hw_addr = ioremap_nocache(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switchreg_base_addr,
					ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switchreg_size);
		if (!qca_phy_priv_global[dev_id]->hw_addr) {
			printk("%s ioremap fail.", __func__);
			return -1;
		}

		if (!IS_ERR(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->ess_clk)) {
			/* Enable ess clock here */
			printk("enable ess clk\n");
			clk_prepare_enable(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->ess_clk);
		} else if (!IS_ERR(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->cmnblk_clk)) {
#if defined(HPPE)
			ssdk_ppe_clock_init();
#endif
			return 0;
		}

		cfg->reg_mode = HSL_HEADER;
	}

	if(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->psgmii_reg_access_mode == HSL_REG_LOCAL_BUS) {
		if (!request_mem_region(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->psgmiireg_base_addr,
				ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->psgmiireg_size, "psgmii_mem")) {
			printk("%s Unable to request psgmii resource.", __func__);
			return -1;
		}

		qca_phy_priv_global[dev_id]->psgmii_hw_addr = ioremap_nocache(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->psgmiireg_base_addr,
				ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->psgmiireg_size);
		if (!qca_phy_priv_global[dev_id]->psgmii_hw_addr) {
			printk("%s ioremap fail.", __func__);
			cfg->reg_func.psgmii_reg_set = NULL;
			cfg->reg_func.psgmii_reg_get = NULL;
			return -1;
		}

		cfg->reg_func.psgmii_reg_set = qca_psgmii_reg_write;
		cfg->reg_func.psgmii_reg_get = qca_psgmii_reg_read;
	}

	if(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switch_reg_access_mode == HSL_REG_MDIO) {
		cfg->reg_mode = HSL_MDIO;
	} else
		return 0;

	return 0;
}

void
ssdk_plat_exit(a_uint32_t dev_id)
{
	printk("ssdk_plat_exit\n");

	if (ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switch_reg_access_mode == HSL_REG_LOCAL_BUS) {
		iounmap(qca_phy_priv_global[dev_id]->hw_addr);
		iounmap(qca_phy_priv_global[dev_id]->psgmii_hw_addr);
		release_mem_region(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switchreg_base_addr,
					ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->switchreg_size);
		release_mem_region(ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->psgmiireg_base_addr,
					ssdk_dt_global.ssdk_dt_switch_nodes[dev_id]->psgmiireg_size);
	}

}

