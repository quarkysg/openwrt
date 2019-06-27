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


/**
 * @defgroup
 * @{
 */
#include "sw.h"
#include "hppe_portctrl_reg.h"
#include "hppe_portctrl.h"
#include "hppe_portvlan_reg.h"
#include "hppe_portvlan.h"
#include "hppe_vsi_reg.h"
#include "hppe_vsi.h"
#include "hppe_policer_reg.h"
#include "hppe_policer.h"
#include "hppe_qm_reg.h"
#include "hppe_qm.h"
#include "adpt.h"

#define VP_PORT_MAX_ID	255
#define VP_PORT_MIN_ID	64
#define PHYSICAL_PORT_MAX_ID	7


char dropcode[128][100] = {
"None",
"Unkown L2 protocol exception drop",
"PPPoE wrong version or wrong type exception drop",
"PPPoE wrong code exception drop",
"PPPoE unsupported PPP protocol exception drop",
"IPv4 wrong version exception drop",
"IPv4 small IHL exception drop",
"IPv4 with option exception drop",
"IPv4 header incomplete exception drop",
"IPv4 bad total length exception drop",
"IPv4 data incomplete exception drop",
"IPv4 fragment exception drop",
"IPv4 ping of death exception drop",
"IPv4 small TTL exception drop",
"IPv4 unknown IP protocol exception drop",
"IPv4 checksum error exception drop",
"IPv4 invalid SIP exception drop",
"IPv4 invalid DIP exception drop",
"IPv4 LAND attack exception drop",
"IPv4 AH header incomplete exception drop",
"IPv4 AH header cross 128-byte exception drop",
"IPv4 ESP header incomplete exception drop",
"IPv6 wrong version exception drop",
"IPv6 header incomplete exception drop",
"IPv6 bad total length exception drop",
"IPv6 data incomplete exception drop",
"IPv6 with extension header exception drop",
"IPv6 small hop limit exception drop",
"IPv6 invalid SIP exception drop",
"IPv6 invalid DIP exception drop",
"IPv6 LAND attack exception drop",
"IPv6 fragment exception drop",
"IPv6 ping of death exception drop",
"IPv6 with more than 2 extension headers exception drop",
"IPv6 unknown last next header exception drop",
"IPv6 mobility header incomplete exception drop",
"IPv6 mobility header cross 128-byte exception drop",
"IPv6 AH header incomplete exception drop",
"IPv6 AH header cross 128-byte exception drop",
"IPv6 ESP header incomplete exception drop",
"IPv6 ESP header cross 128-byte exception drop",
"IPv6 other extension header incomplete exception drop",
"IPv6 other extension header cross 128-byte exception drop",
"TCP header incomplete exception drop",
"TCP header cross 128-byte exception drop",
"TCP same SP and DP exception drop",
"TCP small data offset drop",
"TCP flags VALUE/MASK group 0 exception drop",
"TCP flags VALUE/MASK group 1 exception drop",
"TCP flags VALUE/MASK group 2 exception drop",
"TCP flags VALUE/MASK group 3 exception drop",
"TCP flags VALUE/MASK group 4 exception drop",
"TCP flags VALUE/MASK group 5 exception drop",
"TCP flags VALUE/MASK group 6 exception drop",
"TCP flags VALUE/MASK group 7 exception drop",
"TCP checksum error exception drop",
"UDP header incomplete exception drop",
"UDP header cross 128-byte exception drop",
"UDP same SP and DP exception drop",
"UDP bad length exception drop",
"UDP data incomplete exception drop",
"UDP checksum error exception drop",
"UDP-Lite header incomplete exception drop",
"UDP-Lite header cross 128-byte exception drop",
"UDP-Lite same SP and DP exception drop",
"UDP-Lite checksum coverage value 0-7 exception drop",
"UDP-Lite checksum coverage value too big exception drop",
"UDP-Lite checksum coverage value cross 128-byte exception drop",
"UDP-Lite checksum error exception drop",
"L3 multicast bridging action",
"L3 no route with Preheader NAT action",
"L3 no route with Preheader NAT action error configuration",
"L3 route action drop",
"L3 no route action drop",
"L3 no route next hop invalid action drop",
"L3 no route preheader action drop",
"L3 bridge action drop",
"L3 flow action drop",
"L3 flow miss action drop",
"L2 MRU checking fail drop",
"L2 MTU checking fail drop",
"L3 IP prefix broadcast drop",
"L3 MTU checking fail drop",
"L3 MRU checking fail drop",
"L3 ICMP redirect drop",
"Fake MAC header indicated packet not routing or bypass L3 edit drop",
"L3 IP route TTL zero drop",
"L3 flow service code loop drop",
"L3 flow de-accelerate drop",
"L3 flow source interface check fail drop",
"Flow toggle mismatch exception drop",
"MTU check exception if DF set drop",
"PPPoE multicast packet with IP routing enabled drop",
"IPv4 SG unkown drop",
"IPv6 SG unkown drop",
"ARP SG unkown drop",
"ND SG unkown drop",
"IPv4 SG violation drop",
"IPv6 SG violation drop",
"ARP SG violation drop",
"ND SG violation drop",
"L2 new MAC address drop",
"L2 hash violation drop",
"L2 station move drop",
"L2 learn limit drop",
"L2 SA lookup action drop",
"L2 DA lookup action drop",
"APP_CTRL action drop",
"Ingress VLAN filtering action drop",
"Ingress VLAN translation miss drop",
"Egress VLAN filtering drop",
"Pre-IPO entry hit action drop",
"Post-IPO entry hit action drop",
"Multicast SA or broadcast SA drop",
"No destination drop",
"STG ingress filtering drop",
"STG egress filtering drop",
"Source port filter drop",
"Trunk select fail drop",
"TX MAC disable drop",
"Ingress VLAN tag format drop",
"CRC error drop",
"PAUSE frame drop",
"Promisc drop",
"Isolation drop",
"Magagement packet APP_CTRL drop",
"Fake L2 protocol indicated packet not routing or bypass L3 edit drop",
"Policing drop"
};

sw_error_t
adpt_hppe_debug_port_counter_enable(a_uint32_t dev_id, fal_port_t port_id, fal_counter_en_t * cnt_en)
{
	union mru_mtu_ctrl_tbl_u mru_mtu_ctrl_tbl;
	union mc_mtu_ctrl_tbl_u mc_mtu_ctrl_tbl;
	union port_eg_vlan_u port_eg_vlan;

	ADPT_DEV_ID_CHECK(dev_id);

	port_id = FAL_PORT_ID_VALUE(port_id);

	if (port_id > VP_PORT_MAX_ID)
		return SW_OUT_OF_RANGE;

	if (port_id > PHYSICAL_PORT_MAX_ID)
	{
		SW_RTN_ON_ERROR(hppe_mru_mtu_ctrl_tbl_get(dev_id, port_id, &mru_mtu_ctrl_tbl));
		mru_mtu_ctrl_tbl.bf.rx_cnt_en = cnt_en->rx_counter_en;
		mru_mtu_ctrl_tbl.bf.tx_cnt_en = cnt_en->vp_uni_tx_counter_en;
		SW_RTN_ON_ERROR(hppe_mru_mtu_ctrl_tbl_set(dev_id, port_id, &mru_mtu_ctrl_tbl));
	}
	else
	{
		SW_RTN_ON_ERROR(hppe_mru_mtu_ctrl_tbl_get(dev_id, port_id, &mru_mtu_ctrl_tbl));
		SW_RTN_ON_ERROR(hppe_mc_mtu_ctrl_tbl_get(dev_id, port_id, &mc_mtu_ctrl_tbl));
		SW_RTN_ON_ERROR(hppe_port_eg_vlan_get(dev_id, port_id, &port_eg_vlan));
		mru_mtu_ctrl_tbl.bf.rx_cnt_en = cnt_en->rx_counter_en;
		mru_mtu_ctrl_tbl.bf.tx_cnt_en = cnt_en->vp_uni_tx_counter_en;
		mc_mtu_ctrl_tbl.bf.tx_cnt_en = cnt_en->port_mc_tx_counter_en;
		port_eg_vlan.bf.tx_counting_en = cnt_en->port_tx_counter_en;
		SW_RTN_ON_ERROR(hppe_mru_mtu_ctrl_tbl_set(dev_id, port_id, &mru_mtu_ctrl_tbl));
		SW_RTN_ON_ERROR(hppe_mc_mtu_ctrl_tbl_set(dev_id, port_id, &mc_mtu_ctrl_tbl));
		SW_RTN_ON_ERROR(hppe_port_eg_vlan_set(dev_id, port_id, &port_eg_vlan));
	}

	return SW_OK;
}

sw_error_t
adpt_hppe_debug_port_counter_status_get(a_uint32_t dev_id, fal_port_t port_id, fal_counter_en_t * cnt_en)
{
	union mru_mtu_ctrl_tbl_u mru_mtu_ctrl_tbl;
	union mc_mtu_ctrl_tbl_u mc_mtu_ctrl_tbl;
	union port_eg_vlan_u port_eg_vlan;

	ADPT_DEV_ID_CHECK(dev_id);
	ADPT_NULL_POINT_CHECK(cnt_en);

	port_id = FAL_PORT_ID_VALUE(port_id);

	if (port_id >= MRU_MTU_CTRL_TBL_MAX_ENTRY)
		return SW_OUT_OF_RANGE;

	if (port_id > PHYSICAL_PORT_MAX_ID)
	{
		SW_RTN_ON_ERROR(hppe_mru_mtu_ctrl_tbl_get(dev_id, port_id, &mru_mtu_ctrl_tbl));
		cnt_en->rx_counter_en = mru_mtu_ctrl_tbl.bf.rx_cnt_en;
		cnt_en->vp_uni_tx_counter_en = mru_mtu_ctrl_tbl.bf.tx_cnt_en;
	}
	else
	{
		SW_RTN_ON_ERROR(hppe_mru_mtu_ctrl_tbl_get(dev_id, port_id, &mru_mtu_ctrl_tbl));
		SW_RTN_ON_ERROR(hppe_mc_mtu_ctrl_tbl_get(dev_id, port_id, &mc_mtu_ctrl_tbl));
		SW_RTN_ON_ERROR(hppe_port_eg_vlan_get(dev_id, port_id, &port_eg_vlan));
		cnt_en->rx_counter_en = mru_mtu_ctrl_tbl.bf.rx_cnt_en;
		cnt_en->vp_uni_tx_counter_en = mru_mtu_ctrl_tbl.bf.tx_cnt_en;
		cnt_en->port_mc_tx_counter_en = mc_mtu_ctrl_tbl.bf.tx_cnt_en;
		cnt_en->port_tx_counter_en = port_eg_vlan.bf.tx_counting_en;
	}

	return SW_OK;
}

sw_error_t
adpt_hppe_debug_counter_set(void)
{
	union vlan_cnt_tbl_u vlan_cnt_tbl = {0};
	union pre_l2_cnt_tbl_u pre_l2_cnt_tbl = {0};
	union port_tx_drop_cnt_tbl_u port_tx_drop_cnt_tbl = {0};
	union eg_vsi_counter_tbl_u eg_vsi_counter_tbl = {0};
	union port_tx_counter_tbl_reg_u port_tx_counter_tbl = {0};
	union vp_tx_counter_tbl_reg_u vp_tx_counter_tbl = {0};
	union queue_tx_counter_tbl_u queue_tx_counter_tbl = {0};
	union epe_dbg_in_cnt_reg_u epe_dbg_in_cnt = {0};
	union epe_dbg_out_cnt_reg_u epe_dbg_out_cnt = {0};
	union vp_tx_drop_cnt_tbl_u vp_tx_drop_cnt_tbl = {0};
	union drop_cpu_cnt_tbl_u drop_cpu_cnt_tbl = {0};

	a_uint32_t i;

	/* clear VLAN_CNT_TBL */
	for (i = 0; i < 32; i++)
		hppe_vlan_cnt_tbl_set(0, i, &vlan_cnt_tbl);

	/* clear PRE_L2_CNT_TBL */
	for (i = 0; i < 32; i++)
		hppe_pre_l2_cnt_tbl_set(0, i, &pre_l2_cnt_tbl);

	/* clear PORT_TX_DROP_CNT_TBL */
	for (i = 0; i < 8; i++)
		hppe_port_tx_drop_cnt_tbl_set(0, i, &port_tx_drop_cnt_tbl);

	/* clear EG_VSI_COUNTER_TBL */
	for (i = 0; i < 32; i++)
		hppe_eg_vsi_counter_tbl_set(0, i, &eg_vsi_counter_tbl);

	/* clear PORT_TX_COUNTER_TBL */
	for (i = 0; i < 8; i++)
		hppe_port_tx_counter_tbl_reg_set(0, i, &port_tx_counter_tbl);

	/* clear VP_TX_COUNTER_TBL */
	for (i = 0; i < 256; i++)
		hppe_vp_tx_counter_tbl_reg_set(0, i, &vp_tx_counter_tbl);

	/* clear QUEUE_TX_COUNTER_TBL */
	for (i = 0; i < 300; i++)
		hppe_queue_tx_counter_tbl_set(0, i, &queue_tx_counter_tbl);

	/* clear EPE_DBG_IN_CNT & EPE_DBG_OUT_CNT */
	hppe_epe_dbg_in_cnt_reg_set(0, &epe_dbg_in_cnt);
	hppe_epe_dbg_out_cnt_reg_set(0, &epe_dbg_out_cnt);

	/* clear VP_TX_DROP_CNT_TBL */
	for (i = 0; i < 256; i++)
		hppe_vp_tx_drop_cnt_tbl_set(0, i, &vp_tx_drop_cnt_tbl);

	/* clear DROP_CPU_CNT_TBL */
	for (i = 0; i < 1280; i++)
		hppe_drop_cpu_cnt_tbl_set(0, i, &drop_cpu_cnt_tbl);

	return SW_OK;
}

void
adpt_hppe_debug_vlan_counter_get(a_bool_t show_type)
{
	union vlan_cnt_tbl_u vlan_cnt_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "VLAN_CNT_TBL RX:");
	for (i = 0; i < 32; i++)
	{
		hppe_vlan_cnt_tbl_get(0, i, &vlan_cnt_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)vlan_cnt_tbl.bf.rx_pkt_cnt;
		else
			value = vlan_cnt_tbl.bf.rx_byte_cnt_0 | ((a_uint64_t)vlan_cnt_tbl.bf.rx_byte_cnt_1 << 32);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(vsi=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");
}

void
adpt_hppe_debug_pre_l2_counter_get(a_bool_t show_type)
{
	union pre_l2_cnt_tbl_u pre_l2_cnt_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "PRE_L2_CNT_TBL RX:");
	for (i = 0; i < 32; i++)
	{
		hppe_pre_l2_cnt_tbl_get(0, i, &pre_l2_cnt_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)pre_l2_cnt_tbl.bf.rx_pkt_cnt;
		else
			value = pre_l2_cnt_tbl.bf.rx_byte_cnt_0 | ((a_uint64_t)pre_l2_cnt_tbl.bf.rx_byte_cnt_1 << 32);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(vsi=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");

	sign = tags = 0;
	printk("%-35s", "PRE_L2_CNT_TBL RX_DROP:");
	for (i = 0; i < 32; i++)
	{
		hppe_pre_l2_cnt_tbl_get(0, i, &pre_l2_cnt_tbl);
		if (show_type == A_FALSE)
			value = pre_l2_cnt_tbl.bf.rx_drop_pkt_cnt_0 | ((a_uint64_t)pre_l2_cnt_tbl.bf.rx_drop_pkt_cnt_1 << 24);
		else
			value = pre_l2_cnt_tbl.bf.rx_drop_byte_cnt_0 | ((a_uint64_t)pre_l2_cnt_tbl.bf.rx_drop_byte_cnt_1 << 24);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(vsi=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");
}

void adpt_hppe_debug_port_tx_drop_counter_get(a_bool_t show_type)
{
	union port_tx_drop_cnt_tbl_u port_tx_drop_cnt_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "PORT_TX_DROP_CNT_TBL TX_DROP:");
	for (i = 0; i < 8; i++)
	{
		hppe_port_tx_drop_cnt_tbl_get(0, i, &port_tx_drop_cnt_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)port_tx_drop_cnt_tbl.bf.tx_drop_pkt_cnt;
		else
			value = port_tx_drop_cnt_tbl.bf.tx_drop_byte_cnt_0 | ((a_uint64_t)port_tx_drop_cnt_tbl.bf.tx_drop_byte_cnt_1 << 32);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(port=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");
}

void
adpt_hppe_debug_eg_vsi_counter_get(a_bool_t show_type)
{
	union eg_vsi_counter_tbl_u eg_vsi_counter_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "EG_VSI_COUNTER_TBL TX:");
	for (i = 0; i < 32; i++)
	{
		hppe_eg_vsi_counter_tbl_get(0, i, &eg_vsi_counter_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)eg_vsi_counter_tbl.bf.tx_packets;
		else
			value = eg_vsi_counter_tbl.bf.tx_bytes_0 | ((a_uint64_t)eg_vsi_counter_tbl.bf.tx_bytes_1 << 32);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(vsi=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");
}

void
adpt_hppe_debug_port_tx_counter_get(a_bool_t show_type)
{
	union port_tx_counter_tbl_reg_u port_tx_counter_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "PORT_TX_COUNTER_TBL TX:");
	for (i = 0; i < 8; i++)
	{
		hppe_port_tx_counter_tbl_reg_get(0, i, &port_tx_counter_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)port_tx_counter_tbl.bf.tx_packets;
		else
			value = port_tx_counter_tbl.bf.tx_bytes_0 | ((a_uint64_t)port_tx_counter_tbl.bf.tx_bytes_1 << 32);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(port=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");
}

void
adpt_hppe_debug_vp_tx_counter_get(a_bool_t show_type)
{
	union vp_tx_counter_tbl_reg_u vp_tx_counter_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "VP_TX_COUNTER_TBL TX:");
	for (i = 0; i < 256; i++)
	{
		hppe_vp_tx_counter_tbl_reg_get(0, i, &vp_tx_counter_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)vp_tx_counter_tbl.bf.tx_packets;
		else
			value = vp_tx_counter_tbl.bf.tx_bytes_0 | ((a_uint64_t)vp_tx_counter_tbl.bf.tx_bytes_1 << 32);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(port=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");
}

void
adpt_hppe_debug_queue_tx_counter_get(a_bool_t show_type)
{
	union queue_tx_counter_tbl_u queue_tx_counter_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "QUEUE_TX_COUNTER_TBL TX:");
	for (i = 0; i < 300; i++)
	{
		hppe_queue_tx_counter_tbl_get(0, i, &queue_tx_counter_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)queue_tx_counter_tbl.bf.tx_packets;
		else
			value = queue_tx_counter_tbl.bf.tx_bytes_0 | ((a_uint64_t)queue_tx_counter_tbl.bf.tx_bytes_1 << 32);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(queue=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");
}

void
adpt_hppe_debug_vp_tx_drop_counter_get(a_bool_t show_type)
{
	union vp_tx_drop_cnt_tbl_u vp_tx_drop_cnt_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "VP_TX_DROP_CNT_TBL TX_DROP:");
	for (i = 0; i < 256; i++)
	{
		hppe_vp_tx_drop_cnt_tbl_get(0, i, &vp_tx_drop_cnt_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)vp_tx_drop_cnt_tbl.bf.tx_drop_pkt_cnt;
		else
			value = vp_tx_drop_cnt_tbl.bf.tx_drop_byte_cnt_0 | ((a_uint64_t)vp_tx_drop_cnt_tbl.bf.tx_drop_byte_cnt_1 << 32);
		if (value > 0)
		{
			if (sign) {
				printk("\n");
				printk("%-35s", "");
			}
			sign = 0;
			printk("%15llu(port=%04d)", value, i);
			if (++tags % 3 == 0)
				sign = 1;
		}
	}
	printk("\n");
}

void
adpt_hppe_debug_drop_cpu_counter_get(a_bool_t show_type)
{
	union drop_cpu_cnt_tbl_u drop_cpu_cnt_tbl;
	a_uint64_t value;
	int i, tags, sign;

	sign = tags = 0;
	printk("%-35s", "DROP_CPU_CNT_TBL:");
	for (i = 256; i < 1280; i++)
	{
		hppe_drop_cpu_cnt_tbl_get(0, i, &drop_cpu_cnt_tbl);
		if (show_type == A_FALSE)
			value = (a_uint64_t)drop_cpu_cnt_tbl.bf.pkt_cnt;
		else
			value = drop_cpu_cnt_tbl.bf.byte_cnt_0 | ((a_uint64_t)drop_cpu_cnt_tbl.bf.byte_cnt_1 << 32);
		if (value > 0)
		{
			printk("\n");
			printk("%-35s", "");
			printk("%15llu(port=%d:%s)", value, (i - 256) % 8, dropcode[(i - 256) / 8]);
		}
	}
	printk("\n");
}

/* if show_type = A_FALSE, show packets.
 * if show_type = A_TRUE, show bytes.
 */
sw_error_t
adpt_hppe_debug_counter_get(a_bool_t show_type)
{
	/* show VLAN_CNT_TBL */
	adpt_hppe_debug_vlan_counter_get(show_type);

	/* show PRE_L2_CNT_TBL */
	adpt_hppe_debug_pre_l2_counter_get(show_type);

	/* show PORT_TX_DROP_CNT_TBL */
	adpt_hppe_debug_port_tx_drop_counter_get(show_type);

	/* show EG_VSI_COUNTER_TBL */
	adpt_hppe_debug_eg_vsi_counter_get(show_type);

	/* show PORT_TX_COUNTER_TBL */
	adpt_hppe_debug_port_tx_counter_get(show_type);

	/* show VP_TX_COUNTER_TBL */
	adpt_hppe_debug_vp_tx_counter_get(show_type);

	/* show QUEUE_TX_COUNTER_TBL */
	adpt_hppe_debug_queue_tx_counter_get(show_type);

	/* show VP_TX_DROP_CNT_TBL */
	adpt_hppe_debug_vp_tx_drop_counter_get(show_type);

	/* show DROP_CPU_CNT_TBL */
	adpt_hppe_debug_drop_cpu_counter_get(show_type);

	return SW_OK;
}

sw_error_t adpt_hppe_misc_init(a_uint32_t dev_id)
{
	adpt_api_t *p_adpt_api = NULL;

	p_adpt_api = adpt_api_ptr_get(dev_id);

	if(p_adpt_api == NULL)
		return SW_FAIL;

	p_adpt_api->adpt_debug_port_counter_enable = adpt_hppe_debug_port_counter_enable;
	p_adpt_api->adpt_debug_port_counter_status_get = adpt_hppe_debug_port_counter_status_get;

	p_adpt_api->adpt_debug_counter_set = adpt_hppe_debug_counter_set;
	p_adpt_api->adpt_debug_counter_get = adpt_hppe_debug_counter_get;

	return SW_OK;
}

/**
 * @}
 */
