/*
 **************************************************************************
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
 **************************************************************************
 */

/*
 * nss_vlan_mgr.c
 *	NSS to HLOS vlan Interface manager
 */
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/module.h>
#include <nss_api_if.h>
#ifdef NSS_VLAN_MGR_PPE_SUPPORT
#include <ref/ref_vsi.h>
#include <fal/fal_portvlan.h>
#endif

#if (NSS_VLAN_MGR_DEBUG_LEVEL < 1)
#define nss_vlan_mgr_assert(fmt, args...)
#else
#define nss_vlan_mgr_assert(c) BUG_ON(!(c))
#endif /* NSS_VLAN_MGR_DEBUG_LEVEL */

/*
 * Compile messages for dynamic enable/disable
 */
#if defined(CONFIG_DYNAMIC_DEBUG)
#define nss_vlan_mgr_warn(s, ...) \
		pr_debug("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#define nss_vlan_mgr_info(s, ...) \
		pr_debug("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#define nss_vlan_mgr_trace(s, ...) \
		pr_debug("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#else /* CONFIG_DYNAMIC_DEBUG */
/*
 * Statically compile messages at different levels
 */
#if (NSS_VLAN_MGR_DEBUG_LEVEL < 2)
#define nss_vlan_mgr_warn(s, ...)
#else
#define nss_vlan_mgr_warn(s, ...) \
		pr_warn("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#endif

#if (NSS_VLAN_MGR_DEBUG_LEVEL < 3)
#define nss_vlan_mgr_info(s, ...)
#else
#define nss_vlan_mgr_info(s, ...) \
		pr_notice("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#endif

#if (NSS_VLAN_MGR_DEBUG_LEVEL < 4)
#define nss_vlan_mgr_trace(s, ...)
#else
#define nss_vlan_mgr_trace(s, ...) \
		pr_info("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#endif
#endif /* CONFIG_DYNAMIC_DEBUG */

#define NSS_VLAN_PHY_PORT_MIN 1
#define NSS_VLAN_PHY_PORT_MAX 6
#define NSS_VLAN_PHY_PORT_NUM 8
#define NSS_VLAN_PHY_PORT_CHK(n) ((n) >= NSS_VLAN_PHY_PORT_MIN && (n) <= NSS_VLAN_PHY_PORT_MAX)
#define NSS_VLAN_MGR_TAG_CNT(v) ((v->parent) ? NSS_VLAN_TYPE_DOUBLE : NSS_VLAN_TYPE_SINGLE)
#define NSS_VLAN_TPID_SHIFT 16
#define NSS_VLAN_PORT_ROLE_CHANGED 1

/*
 * vlan client context
 */
struct nss_vlan_mgr_context {
	int ctpid;				/* Customer TPID */
	int stpid;				/* Service TPID */
	int port_role[NSS_VLAN_PHY_PORT_NUM];	/* Role of physical ports */
	struct list_head list;			/* List of vlan private instance */
	spinlock_t lock;			/* Lock to protect vlan private instance */
	struct ctl_table_header *sys_hdr;	/* "/pro/sys/nss/vlan_client" directory */
} vlan_mgr_ctx;

/*
 * vlan manager private structure
 */
struct nss_vlan_pvt {
	struct list_head list;			/* list head */
	struct nss_vlan_pvt *parent;		/* parent vlan instance */
	uint32_t nss_if;			/* nss interface number of this vlan device */

	/*
	 * Fields for Linux information
	 */
	int ifindex;				/* netdev ifindex */
	int32_t port;				/* real physical port of this vlan device */
	uint32_t vid;				/* vid info */
	uint32_t tpid;				/* tpid info */
	uint32_t mtu;				/* mtu info */
	uint8_t dev_addr[ETH_ALEN];		/* mac address */

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	/*
	 * Fields for PPE information
	 */
	uint32_t ppe_vsi;			/* VLAN VSI info */
	uint32_t bridge_vsi;			/* Bridge's VSI when vlan is a member of a bridge */
	uint32_t ppe_cvid;			/* ppe_cvid info */
	uint32_t ppe_svid;			/* ppe_svid info */
	fal_vlan_trans_entry_t eg_xlt_entry;	/* VLAN translation entry */
#endif
	int refs;				/* reference count */
};

/*
 * nss_vlan_mgr_instance_find_and_ref()
 */
static struct nss_vlan_pvt *nss_vlan_mgr_instance_find_and_ref(
						struct net_device *dev)
{
	struct nss_vlan_pvt *v;

	if (!is_vlan_dev(dev)) {
		return NULL;
	}

	spin_lock(&vlan_mgr_ctx.lock);
	list_for_each_entry(v, &vlan_mgr_ctx.list, list) {
		if (v->ifindex == dev->ifindex) {
			v->refs++;
			spin_unlock(&vlan_mgr_ctx.lock);
			return v;
		}
	}
	spin_unlock(&vlan_mgr_ctx.lock);

	return NULL;
}

/*
 * nss_vlan_mgr_instance_deref()
 */
static void nss_vlan_mgr_instance_deref(struct nss_vlan_pvt *v)
{
	spin_lock(&vlan_mgr_ctx.lock);
	BUG_ON(!(--v->refs));
	spin_unlock(&vlan_mgr_ctx.lock);
}

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
/*
 * nss_vlan_mgr_calculate_new_port_role()
 *	check if we can change this port to edge port
 */
static bool nss_vlan_mgr_calculate_new_port_role(int32_t port)
{
	struct nss_vlan_pvt *vif;
	bool to_edge_port = true;

	if (vlan_mgr_ctx.port_role[port] == FAL_QINQ_EDGE_PORT)
		return false;

	if (vlan_mgr_ctx.ctpid != vlan_mgr_ctx.stpid)
		return false;

	/*
	 * If no other double VLAN interface on the same physcial port,
	 * we set physical port as edge port
	 */
	spin_lock(&vlan_mgr_ctx.lock);
	list_for_each_entry(vif, &vlan_mgr_ctx.list, list) {
		if ((vif->port == port) && (vif->parent)) {
			to_edge_port = false;
			break;
		}
	}
	spin_unlock(&vlan_mgr_ctx.lock);

	if (to_edge_port) {
		fal_port_qinq_role_t mode;

		mode.mask = FAL_PORT_QINQ_ROLE_INGRESS_EN | FAL_PORT_QINQ_ROLE_EGRESS_EN;
		mode.ingress_port_role = FAL_QINQ_EDGE_PORT;
		mode.egress_port_role = FAL_QINQ_EDGE_PORT;

		if (fal_port_qinq_mode_set(0, port, &mode)) {
			nss_vlan_mgr_warn("failed to set %d as edge port\n", port);
			return false;
		}

		vlan_mgr_ctx.port_role[port] = FAL_QINQ_EDGE_PORT;
	}

	return to_edge_port;
}

/*
 * nss_vlan_mgr_port_role_update()
 */
static void nss_vlan_mgr_port_role_update(struct nss_vlan_pvt *vif, uint32_t new_ppe_cvid, uint32_t new_ppe_svid)
{
	/*
	 * Delete old ingress vlan translation rule
	 */
	if (ppe_port_vlan_vsi_set(0, vif->port, vif->ppe_svid, vif->ppe_cvid, 0xffff)) {
		nss_vlan_mgr_warn("Failed to delete old ingress vlan translation rule of port %d\n", vif->port);
		return;
	}

	/*
	 * Delete old egress vlan translation rule
	 */
	if (fal_port_vlan_trans_del(0, vif->port, &vif->eg_xlt_entry)) {
		nss_vlan_mgr_warn("Failed to delete old egress vlan translation of port %d\n", vif->port);
		return;
	}

	/*
	 * Update ppe_civd and ppe_svid
	 */
	vif->ppe_cvid = new_ppe_cvid;
	vif->ppe_svid = new_ppe_svid;

	/*
	 * Add new ingress vlan translation rule
	 */
	if (ppe_port_vlan_vsi_set(0, vif->port, vif->ppe_svid, vif->ppe_cvid,
				  (vif->bridge_vsi ? vif->bridge_vsi : vif->ppe_vsi))) {
		nss_vlan_mgr_warn("Failed to update ingress vlan translation of port %d\n", vif->port);
		return;
	}

	/*
	 * Add new egress vlan translation rule
	 */
	vif->eg_xlt_entry.cvid_xlt_cmd = (vif->ppe_cvid == 0xffff) ? 0 : FAL_VID_XLT_CMD_ADDORREPLACE;
	vif->eg_xlt_entry.cvid_xlt = vif->ppe_cvid;
	vif->eg_xlt_entry.svid_xlt_cmd = (vif->ppe_svid == 0xffff) ? 0 : FAL_VID_XLT_CMD_ADDORREPLACE;
	vif->eg_xlt_entry.svid_xlt = vif->ppe_svid;

	if (fal_port_vlan_trans_add(0, vif->port, &vif->eg_xlt_entry)) {
		nss_vlan_mgr_warn("Failed to update egress vlan translation of port %d\n", vif->port);
	}
}

/*
 * nss_vlan_mgr_port_role_event()
 */
static void nss_vlan_mgr_port_role_event(int32_t port)
{
	struct nss_vlan_pvt *vif;

	if (vlan_mgr_ctx.ctpid != vlan_mgr_ctx.stpid)
		return;

	spin_lock(&vlan_mgr_ctx.lock);
	list_for_each_entry(vif, &vlan_mgr_ctx.list, list) {
		if ((vif->port == port) && (!vif->parent)) {
			if ((vlan_mgr_ctx.port_role[port] == FAL_QINQ_EDGE_PORT) &&
			    (vif->vid != vif->ppe_cvid)) {
				nss_vlan_mgr_port_role_update(vif, vif->vid, 0xffff);
			}

			if ((vlan_mgr_ctx.port_role[port] == FAL_QINQ_CORE_PORT) &&
			    (vif->vid != vif->ppe_svid)) {
				nss_vlan_mgr_port_role_update(vif, 0xffff, vif->vid);
			}
		}
	}
	spin_unlock(&vlan_mgr_ctx.lock);
}

/*
 * nss_vlan_mgr_configure_ppe()
 */
static int nss_vlan_mgr_configure_ppe(struct nss_vlan_pvt *v, struct net_device *dev)
{
	uint32_t vsi;
	int ret = 0;

	if (ppe_vsi_alloc(0, &vsi)) {
		nss_vlan_mgr_warn("%s: failed to allocate VSI for vlan device", dev->name);
		return -1;
	}

	if (nss_vlan_tx_vsi_attach_msg(v->nss_if, vsi) != NSS_TX_SUCCESS) {
		nss_vlan_mgr_warn("%s: failed to attach VSI to vlan interface\n", dev->name);
		goto free_vsi;
	}

	/*
	 * Calculate ppe cvid and svid
	 */
	if (NSS_VLAN_MGR_TAG_CNT(v) == NSS_VLAN_TYPE_DOUBLE) {
		v->ppe_cvid = v->vid;
		v->ppe_svid = v->parent->vid;
	} else {
		if (((vlan_mgr_ctx.ctpid != vlan_mgr_ctx.stpid) && (v->tpid == vlan_mgr_ctx.ctpid)) ||
		    ((vlan_mgr_ctx.ctpid == vlan_mgr_ctx.stpid) && (vlan_mgr_ctx.port_role[v->port] == FAL_QINQ_EDGE_PORT))) {
			v->ppe_cvid = v->vid;
			v->ppe_svid = 0xffff;
		} else {
			v->ppe_cvid = 0xffff;
			v->ppe_svid = v->vid;
		}
	}

	/*
	 * Add ingress vlan translation rule
	 */
	if (ppe_port_vlan_vsi_set(0, v->port, v->ppe_svid, v->ppe_cvid, vsi)) {
		nss_vlan_mgr_warn("%s: failed to set ingress vlan translation\n", dev->name);
		goto detach_vsi;
	}

	/*
	 * Add egress vlan translation rule
	 */
	memset(&v->eg_xlt_entry, 0, sizeof(v->eg_xlt_entry));

	/*
	 * Fields for match
	 */
	v->eg_xlt_entry.trans_direction = 1;		/* Egress vlan translation rule*/
	v->eg_xlt_entry.vsi_valid = true;		/* Use vsi as search key*/
	v->eg_xlt_entry.vsi_enable = true;		/* Use vsi as search key*/
	v->eg_xlt_entry.vsi = vsi;			/* Use vsi as search key*/
	v->eg_xlt_entry.s_tagged = 0x7;			/* Accept tagged/untagged/priority tagged svlan */
	v->eg_xlt_entry.c_tagged = 0x7;			/* Accept tagged/untagged/priority tagged cvlan */
	v->eg_xlt_entry.port_bitmap = (1 << v->port);	/* Use port as search key*/

	/*
	 * Fields for action
	 */
	v->eg_xlt_entry.cvid_xlt_cmd = (v->ppe_cvid == 0xffff) ? 0 : FAL_VID_XLT_CMD_ADDORREPLACE;
	v->eg_xlt_entry.cvid_xlt = v->ppe_cvid;
	v->eg_xlt_entry.svid_xlt_cmd = (v->ppe_svid == 0xffff) ? 0 : FAL_VID_XLT_CMD_ADDORREPLACE;
	v->eg_xlt_entry.svid_xlt = v->ppe_svid;

	if (fal_port_vlan_trans_add(0, v->port, &v->eg_xlt_entry)) {
		nss_vlan_mgr_warn("%s: failed to set egress vlan translation\n", dev->name);
		goto delete_ingress_rule;
	}

	if ((v->ppe_svid != 0xffff) && (vlan_mgr_ctx.port_role[v->port] != FAL_QINQ_CORE_PORT)) {
		fal_port_qinq_role_t mode;

		/*
		 * If double tag, we should set physical port as core port
		 */
		vlan_mgr_ctx.port_role[v->port] = FAL_QINQ_CORE_PORT;

		/*
		 * Update port role in PPE
		 */
		mode.mask = 0x3;
		mode.ingress_port_role = FAL_QINQ_CORE_PORT;
		mode.egress_port_role = FAL_QINQ_CORE_PORT;

		if (fal_port_qinq_mode_set(0, v->port, &mode)) {
			nss_vlan_mgr_warn("%s: failed to set %d as core port\n", dev->name, v->port);
			goto delete_egress_rule;
		}

		ret = NSS_VLAN_PORT_ROLE_CHANGED;
	}

	v->ppe_vsi = vsi;
	return ret;

delete_egress_rule:
	if (fal_port_vlan_trans_del(0, v->port, &v->eg_xlt_entry)) {
		nss_vlan_mgr_warn("%p: Failed to delete egress translation rule\n", v);
	}

delete_ingress_rule:
	if (ppe_port_vlan_vsi_set(0, v->port, v->ppe_svid, v->ppe_cvid, 0xffff)) {
		nss_vlan_mgr_warn("%p: Failed to delete ingress translation rule\n", v);
	}

detach_vsi:
	if (nss_vlan_tx_vsi_detach_msg(v->nss_if, vsi)) {
		nss_vlan_mgr_warn("%p: Failed to detach vsi %d\n", v, vsi);
	}

free_vsi:
	if (ppe_vsi_free(0, vsi)) {
		nss_vlan_mgr_warn("%p: Failed to free VLAN VSI\n", v);
	}

	return -1;
}
#endif

/*
 * nss_vlan_mgr_create_instance()
 */
static struct nss_vlan_pvt *nss_vlan_mgr_create_instance(
						struct net_device *dev)
{
	struct nss_vlan_pvt *v;
	struct vlan_dev_priv *vlan;
	struct net_device *real_dev;

	if (!is_vlan_dev(dev)) {
		return NULL;
	}

	v = kzalloc(sizeof(*v), GFP_KERNEL);
	if (!v) {
		return NULL;
	}

	INIT_LIST_HEAD(&v->list);

	vlan = vlan_dev_priv(dev);
	real_dev = vlan->real_dev;
	v->vid = vlan->vlan_id;
	v->tpid = ntohs(vlan->vlan_proto);
	v->parent = nss_vlan_mgr_instance_find_and_ref(real_dev);
	if (!v->parent) {
		v->port = nss_cmn_get_interface_number_by_dev(real_dev);
		if (!NSS_VLAN_PHY_PORT_CHK(v->port)) {
			nss_vlan_mgr_warn("%s: %d is not valid physical port\n", dev->name, v->port);
			kfree(v);
			return NULL;
		}
	} else if (!v->parent->parent) {
		v->port = v->parent->port;
	} else {
		nss_vlan_mgr_warn("%s: don't support more than 2 vlans\n", dev->name);
		nss_vlan_mgr_instance_deref(v->parent);
		kfree(v);
		return NULL;
	}

	/*
	 * Check if TPID is permited
	 */
	if ((NSS_VLAN_MGR_TAG_CNT(v) == NSS_VLAN_TYPE_DOUBLE) &&
	    ((v->tpid != vlan_mgr_ctx.ctpid) || (v->parent->tpid != vlan_mgr_ctx.stpid))) {
		nss_vlan_mgr_warn("%s: double tag: tpid %04x not match global tpid(%04x, %04x)\n",
				  dev->name, v->tpid, vlan_mgr_ctx.ctpid, vlan_mgr_ctx.stpid);
		nss_vlan_mgr_instance_deref(v->parent);
		kfree(v);
		return NULL;
	}

	if ((NSS_VLAN_MGR_TAG_CNT(v) == NSS_VLAN_TYPE_SINGLE) &&
	    ((v->tpid != vlan_mgr_ctx.ctpid) && (v->tpid != vlan_mgr_ctx.stpid))) {
		nss_vlan_mgr_warn("%s: single tag: tpid %04x not match global tpid(%04x, %04x)\n",
				  dev->name, v->tpid, vlan_mgr_ctx.ctpid, vlan_mgr_ctx.stpid);
		kfree(v);
		return NULL;
	}

	v->mtu = dev->mtu;
	ether_addr_copy(v->dev_addr, dev->dev_addr);
	v->ifindex = dev->ifindex;
	v->refs = 1;

	return v;
}

/*
 * nss_vlan_mgr_instance_free()
 */
static void nss_vlan_mgr_instance_free(struct nss_vlan_pvt *v)
{
	spin_lock(&vlan_mgr_ctx.lock);
	BUG_ON(--v->refs);
	if (!list_empty(&v->list)) {
		list_del(&v->list);
	}
	spin_unlock(&vlan_mgr_ctx.lock);

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	if (v->ppe_vsi) {
		/*
		 * Detach VSI
		 */
		if (nss_vlan_tx_vsi_detach_msg(v->nss_if, v->ppe_vsi)) {
			nss_vlan_mgr_warn("%p: Failed to detach vsi %d\n", v, v->ppe_vsi);
		}

		/*
		 * Delete ingress vlan translation rule
		 */
		if (ppe_port_vlan_vsi_set(0, v->port, v->ppe_svid, v->ppe_cvid, 0xffff)) {
			nss_vlan_mgr_warn("%p: Failed to delete old ingress translation rule\n", v);
		}

		/*
		 * Delete egress vlan translation rule
		 */
		if (fal_port_vlan_trans_del(0, v->port, &v->eg_xlt_entry)) {
			nss_vlan_mgr_warn("%p: Failed to delete vlan translation rule\n", v);
		}

		/*
		 * Free PPE VSI
		 */
		if (ppe_vsi_free(0, v->ppe_vsi)) {
			nss_vlan_mgr_warn("%p: Failed to free VLAN VSI\n", v);
		}
	}

	if (nss_vlan_mgr_calculate_new_port_role(v->port)) {
		nss_vlan_mgr_port_role_event(v->port);
	}
#endif

	if (v->nss_if) {
		nss_unregister_vlan_if(v->nss_if);
		if (nss_dynamic_interface_dealloc_node(v->nss_if, NSS_DYNAMIC_INTERFACE_TYPE_VLAN) != NSS_TX_SUCCESS) {
			nss_vlan_mgr_warn("%p: Failed to dealloc vlan dynamic interface\n", v);
		}
	}

	if (v->parent)
		nss_vlan_mgr_instance_deref(v->parent);

	kfree(v);
}

/*
 * nss_vlan_mgr_changemtu_event()
 */
static int nss_vlan_mgr_changemtu_event(struct netdev_notifier_info *info)
{
	struct net_device *dev = netdev_notifier_info_to_dev(info);
	struct nss_vlan_pvt *v_pvt = nss_vlan_mgr_instance_find_and_ref(dev);

	if (!v_pvt)
		return NOTIFY_DONE;

	spin_lock(&vlan_mgr_ctx.lock);
	if (v_pvt->mtu == dev->mtu) {
		spin_unlock(&vlan_mgr_ctx.lock);
		nss_vlan_mgr_instance_deref(v_pvt);
		return NOTIFY_DONE;
	}
	spin_unlock(&vlan_mgr_ctx.lock);

	if (nss_vlan_tx_set_mtu_msg(v_pvt->nss_if, dev->mtu) != NSS_TX_SUCCESS) {
		nss_vlan_mgr_warn("%s: Failed to send change MTU(%d) message to NSS\n", dev->name, dev->mtu);
		nss_vlan_mgr_instance_deref(v_pvt);
		return NOTIFY_BAD;
	}

	spin_lock(&vlan_mgr_ctx.lock);
	v_pvt->mtu = dev->mtu;
	spin_unlock(&vlan_mgr_ctx.lock);
	nss_vlan_mgr_trace("%s: MTU changed to %d, NSS updated\n", dev->name, dev->mtu);
	nss_vlan_mgr_instance_deref(v_pvt);
	return NOTIFY_DONE;
}

/*
 * int nss_vlan_mgr_changeaddr_event()
 */
static int nss_vlan_mgr_changeaddr_event(struct netdev_notifier_info *info)
{
	struct net_device *dev = netdev_notifier_info_to_dev(info);
	struct nss_vlan_pvt *v_pvt = nss_vlan_mgr_instance_find_and_ref(dev);

	if (!v_pvt)
		return NOTIFY_DONE;

	spin_lock(&vlan_mgr_ctx.lock);
	if (!memcmp(v_pvt->dev_addr, dev->dev_addr, ETH_ALEN)) {
		spin_unlock(&vlan_mgr_ctx.lock);
		nss_vlan_mgr_instance_deref(v_pvt);
		return NOTIFY_DONE;
	}
	spin_unlock(&vlan_mgr_ctx.lock);

	if (nss_vlan_tx_set_mac_addr_msg(v_pvt->nss_if, dev->dev_addr) != NSS_TX_SUCCESS) {
		nss_vlan_mgr_warn("%s: Failed to send change MAC address message to NSS\n", dev->name);
		nss_vlan_mgr_instance_deref(v_pvt);
		return NOTIFY_BAD;
	}

	spin_lock(&vlan_mgr_ctx.lock);
	ether_addr_copy(v_pvt->dev_addr, dev->dev_addr);
	spin_unlock(&vlan_mgr_ctx.lock);
	nss_vlan_mgr_trace("%s: MAC changed to %pM, updated NSS\n", dev->name, dev->dev_addr);
	nss_vlan_mgr_instance_deref(v_pvt);
	return NOTIFY_DONE;
}

/*
 * nss_vlan_mgr_register_event()
 */
static int nss_vlan_mgr_register_event(struct netdev_notifier_info *info)
{
	struct net_device *dev = netdev_notifier_info_to_dev(info);
	struct nss_vlan_pvt *v;
	int if_num;
#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	int ret;
#endif
	uint32_t vlan_tag;

	v = nss_vlan_mgr_create_instance(dev);
	if (!v)
		return NOTIFY_DONE;

	if_num = nss_dynamic_interface_alloc_node(NSS_DYNAMIC_INTERFACE_TYPE_VLAN);
	if (if_num < 0) {
		nss_vlan_mgr_warn("%s: failed to alloc NSS dynamic interface\n", dev->name);
		nss_vlan_mgr_instance_free(v);
		return NOTIFY_DONE;
	}

	if (!nss_register_vlan_if(if_num, NULL, dev, 0, v)) {
		nss_vlan_mgr_warn("%s: failed to register NSS dynamic interface", dev->name);
		if (nss_dynamic_interface_dealloc_node(if_num, NSS_DYNAMIC_INTERFACE_TYPE_VLAN) != NSS_TX_SUCCESS) {
			nss_vlan_mgr_warn("%p: Failed to dealloc vlan dynamic interface\n", v);
		}
		nss_vlan_mgr_instance_free(v);
		return NOTIFY_DONE;
	}
	v->nss_if = if_num;

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	ret = nss_vlan_mgr_configure_ppe(v, dev);
	if (ret < 0) {
		nss_vlan_mgr_instance_free(v);
		return NOTIFY_DONE;
	}
#endif

	if (nss_vlan_tx_set_mac_addr_msg(v->nss_if, v->dev_addr) != NSS_TX_SUCCESS) {
		nss_vlan_mgr_warn("%s: failed to set mac_addr msg\n", dev->name);
		nss_vlan_mgr_instance_free(v);
		return NOTIFY_DONE;
	}

	if (nss_vlan_tx_set_mtu_msg(v->nss_if, v->mtu) != NSS_TX_SUCCESS) {
		nss_vlan_mgr_warn("%s: failed to set mtu msg\n", dev->name);
		nss_vlan_mgr_instance_free(v);
		return NOTIFY_DONE;
	}

	vlan_tag = (v->tpid << NSS_VLAN_TPID_SHIFT | v->vid);
	if (nss_vlan_tx_add_tag_msg(v->nss_if, vlan_tag, (v->parent ? v->parent->nss_if : v->port), v->port) != NSS_TX_SUCCESS) {
		nss_vlan_mgr_warn("%s: failed to add vlan in nss\n", dev->name);
		nss_vlan_mgr_instance_free(v);
		return NOTIFY_DONE;
	}

	spin_lock(&vlan_mgr_ctx.lock);
	list_add(&v->list, &vlan_mgr_ctx.list);
	spin_unlock(&vlan_mgr_ctx.lock);

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	if (ret == NSS_VLAN_PORT_ROLE_CHANGED)
		nss_vlan_mgr_port_role_event(v->port);
#endif
	return NOTIFY_DONE;
}

/*
 * nss_vlan_mgr_unregister_event()
 */
static int nss_vlan_mgr_unregister_event(struct netdev_notifier_info *info)
{
	struct net_device *dev = netdev_notifier_info_to_dev(info);
	struct nss_vlan_pvt *v = nss_vlan_mgr_instance_find_and_ref(dev);

	/*
	 * Do we have it on record?
	 */
	if (!v)
		return NOTIFY_DONE;

	nss_vlan_mgr_trace("Vlan %s unregsitered. Freeing NSS dynamic interface %d\n", dev->name, v->nss_if);

	/*
	 * Release reference got by "nss_vlan_mgr_instance_find_and_ref"
	 */
	nss_vlan_mgr_instance_deref(v);

	/*
	 * Free instance
	 */
	nss_vlan_mgr_instance_free(v);

	return NOTIFY_DONE;
}

/*
 * nss_vlan_mgr_netdevice_event()
 */
static int nss_vlan_mgr_netdevice_event(struct notifier_block *unused,
				unsigned long event, void *ptr)
{
	struct netdev_notifier_info *info = (struct netdev_notifier_info *)ptr;

	switch (event) {
	case NETDEV_CHANGEADDR:
		return nss_vlan_mgr_changeaddr_event(info);
	case NETDEV_CHANGEMTU:
		return nss_vlan_mgr_changemtu_event(info);
	case NETDEV_REGISTER:
		return nss_vlan_mgr_register_event(info);
	case NETDEV_UNREGISTER:
		return nss_vlan_mgr_unregister_event(info);
	}

	/*
	 * Notify done for all the events we don't care
	 */
	return NOTIFY_DONE;
}

static struct notifier_block nss_vlan_mgr_netdevice_nb __read_mostly = {
	.notifier_call = nss_vlan_mgr_netdevice_event,
};

/*
 * nss_vlan_mgr_join_bridge()
 *	update ingress and egress vlan translation rule to use bridge VSI
 */
int nss_vlan_mgr_join_bridge(struct net_device *dev, uint32_t bridge_vsi)
{
	struct nss_vlan_pvt *v = nss_vlan_mgr_instance_find_and_ref(dev);

	if (!v)
		return 0;

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	if ((v->bridge_vsi == bridge_vsi) || v->bridge_vsi) {
		nss_vlan_mgr_warn("%s is already in bridge VSI %d, can't change to %d\n",
				  dev->name, v->bridge_vsi, bridge_vsi);
		nss_vlan_mgr_instance_deref(v);
		return 0;
	}

	/*
	 * Delete old ingress vlan translation rule
	 */
	ppe_port_vlan_vsi_set(0, v->port, v->ppe_svid, v->ppe_cvid, 0xffff);

	/*
	 * Delete old egress vlan translation rule
	 */
	fal_port_vlan_trans_del(0, v->port, &v->eg_xlt_entry);

	/*
	 * Add new ingress vlan translation rule to use bridge VSI
	 */
	if (ppe_port_vlan_vsi_set(0, v->port, v->ppe_svid, v->ppe_cvid, bridge_vsi)) {
		nss_vlan_mgr_warn("%s: failed to change ingress vlan translation\n", dev->name);
		nss_vlan_mgr_instance_deref(v);
		return -1;
	}

	/*
	 * Add new egress vlan translation rule to use bridge VSI
	 */
	v->eg_xlt_entry.vsi = bridge_vsi;
	if (fal_port_vlan_trans_add(0, v->port, &v->eg_xlt_entry)) {
		nss_vlan_mgr_warn("%s: failed to change egress vlan translation\n", dev->name);
		nss_vlan_mgr_instance_deref(v);
		return -1;
	}

	v->bridge_vsi = bridge_vsi;
#endif
	nss_vlan_mgr_instance_deref(v);
	return 0;
}
EXPORT_SYMBOL(nss_vlan_mgr_join_bridge);

/*
 * nss_vlan_mgr_leave_bridge()
 *	update ingress and egress vlan translation rule to restore vlan VSI
 */
int nss_vlan_mgr_leave_bridge(struct net_device *dev, uint32_t bridge_vsi)
{
	struct nss_vlan_pvt *v = nss_vlan_mgr_instance_find_and_ref(dev);

	if (!v)
		return 0;

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	if (v->bridge_vsi != bridge_vsi) {
		nss_vlan_mgr_warn("%s is not in bridge VSI %d, ignore\n", dev->name, bridge_vsi);
		nss_vlan_mgr_instance_deref(v);
		return 0;
	}

	/*
	 * Delete old ingress vlan translation rule
	 */
	ppe_port_vlan_vsi_set(0, v->port, v->ppe_svid, v->ppe_cvid, 0xffff);

	/*
	 * Delete old egress vlan translation rule
	 */
	fal_port_vlan_trans_del(0, v->port, &v->eg_xlt_entry);

	/*
	 * Record this change
	 */
	v->bridge_vsi = 0;

	/*
	 * Add new ingress vlan translation rule to use vlan VSI
	 */
	if (ppe_port_vlan_vsi_set(0, v->port, v->ppe_svid, v->ppe_cvid, v->ppe_vsi)) {
		nss_vlan_mgr_warn("%s: failed to change ingress vlan translation\n", dev->name);
		nss_vlan_mgr_instance_deref(v);
		return -1;
	}

	/*
	 * Add new egress vlan translation rule to use vlan VSI
	 */
	v->eg_xlt_entry.vsi = v->ppe_vsi;
	if (fal_port_vlan_trans_add(0, v->port, &v->eg_xlt_entry)) {
		nss_vlan_mgr_warn("%s: failed to change egress vlan translation\n", dev->name);
		nss_vlan_mgr_instance_deref(v);
		return -1;
	}
#endif
	nss_vlan_mgr_instance_deref(v);
	return 0;
}
EXPORT_SYMBOL(nss_vlan_mgr_leave_bridge);

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
/*
 * int nss_vlan_mgr_update_ppe_tpid()
 */
static int nss_vlan_mgr_update_ppe_tpid(void)
{
	fal_tpid_t tpid;

	tpid.mask = FAL_TPID_CTAG_EN | FAL_TPID_STAG_EN;
	tpid.ctpid = vlan_mgr_ctx.ctpid;
	tpid.stpid = vlan_mgr_ctx.stpid;

	if (fal_ingress_tpid_set(0, &tpid) || fal_egress_tpid_set(0, &tpid)) {
		nss_vlan_mgr_warn("failed to set ctpid %d stpid %d\n", tpid.ctpid, tpid.stpid);
		return -1;
	}

	return 0;
}

/*
 * nss_vlan_mgr_tpid_proc_handler()
 *	Sets customer TPID and service TPID
 */
static int nss_vlan_mgr_tpid_proc_handler(struct ctl_table *ctl,
					  int write, void __user *buffer,
					  size_t *lenp, loff_t *ppos)
{
	int ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (write)
		nss_vlan_mgr_update_ppe_tpid();

	return ret;
}

/*
 * nss_vlan sysctl table
 */
static struct ctl_table nss_vlan_table[] = {
	{
		.procname	= "ctpid",
		.data		= &vlan_mgr_ctx.ctpid,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_vlan_mgr_tpid_proc_handler,
	},
	{
		.procname	= "stpid",
		.data		= &vlan_mgr_ctx.stpid,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &nss_vlan_mgr_tpid_proc_handler,
	},
	{ }
};

/*
 * nss_vlan sysctl dir
 */
static struct ctl_table nss_vlan_dir[] = {
	{
		.procname		= "vlan_client",
		.mode			= 0555,
		.child			= nss_vlan_table,
	},
	{ }
};

/*
 * nss_vlan systel root dir
 */
static struct ctl_table nss_vlan_root_dir[] = {
	{
		.procname		= "nss",
		.mode			= 0555,
		.child			= nss_vlan_dir,
	},
	{ }
};
#endif

/*
 * nss_vlan_mgr_init_module()
 *	vlan_mgr module init function
 */
int __init nss_vlan_mgr_init_module(void)
{
#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	int idx;
#endif

	INIT_LIST_HEAD(&vlan_mgr_ctx.list);
	spin_lock_init(&vlan_mgr_ctx.lock);

	vlan_mgr_ctx.ctpid = ETH_P_8021Q;
	vlan_mgr_ctx.stpid = ETH_P_8021Q;

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	vlan_mgr_ctx.sys_hdr = register_sysctl_table(nss_vlan_root_dir);
	if (!vlan_mgr_ctx.sys_hdr) {
		nss_vlan_mgr_warn("Unabled to register sysctl table for vlan manager\n");
		return -EFAULT;
	}

	if (nss_vlan_mgr_update_ppe_tpid()) {
		unregister_sysctl_table(vlan_mgr_ctx.sys_hdr);
		return -EFAULT;
	}

	for (idx = 0; idx < NSS_VLAN_PHY_PORT_NUM; idx++) {
		vlan_mgr_ctx.port_role[idx] = FAL_QINQ_EDGE_PORT;
	}
#endif
	register_netdevice_notifier(&nss_vlan_mgr_netdevice_nb);

	nss_vlan_mgr_info("Module (Build %s) loaded\n", NSS_CLIENT_BUILD_ID);
	return 0;
}

/*
 * nss_vlan_mgr_exit_module()
 *	vlan_mgr module exit function
 */
void __exit nss_vlan_mgr_exit_module(void)
{
	unregister_netdevice_notifier(&nss_vlan_mgr_netdevice_nb);

#ifdef NSS_VLAN_MGR_PPE_SUPPORT
	if (vlan_mgr_ctx.sys_hdr)
		unregister_sysctl_table(vlan_mgr_ctx.sys_hdr);
#endif
	nss_vlan_mgr_info("Module unloaded\n");
}

module_init(nss_vlan_mgr_init_module);
module_exit(nss_vlan_mgr_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("NSS vlan manager");
