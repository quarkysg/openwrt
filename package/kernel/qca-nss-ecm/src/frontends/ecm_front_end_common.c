/*
 **************************************************************************
 * Copyright (c) 2015 The Linux Foundation.  All rights reserved.
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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/inet.h>
#include <linux/etherdevice.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/ip.h>
#include <net/ipv6.h>

/*
 * Debug output levels
 * 0 = OFF
 * 1 = ASSERTS / ERRORS
 * 2 = 1 + WARN
 * 3 = 2 + INFO
 * 4 = 3 + TRACE
 */
#define DEBUG_LEVEL ECM_FRONT_END_COMMON_DEBUG_LEVEL

#include "ecm_types.h"
#include "ecm_db_types.h"
#include "ecm_state.h"
#include "ecm_tracker.h"
#include "ecm_classifier.h"
#include "ecm_front_end_types.h"
#include "ecm_tracker_datagram.h"
#include "ecm_tracker_udp.h"
#include "ecm_tracker_tcp.h"
#include "ecm_db.h"
#include "ecm_front_end_common.h"

/*
 * ecm_front_end_conntrack_notifier_stop()
 */
void ecm_front_end_conntrack_notifier_stop(int num)
{
	/*
	 * If the device tree is used, check which accel engine's conntrack notifier
	 * will be stopped.
	 * For ipq8064 platforms, we will stop NSS version.
	 */
#ifdef CONFIG_OF
	/*
	 * Check the other platforms and use the correct APIs for those platforms.
	 */
	if (!of_machine_is_compatible("qcom,ipq8064") &&
		!of_machine_is_compatible("qcom,ipq8062")) {
		ecm_sfe_conntrack_notifier_stop(num);
	} else {
		ecm_nss_conntrack_notifier_stop(num);
	}
#else
	ecm_nss_conntrack_notifier_stop(num);
#endif
}

/*
 * ecm_front_end_conntrack_notifier_init()
 */
int ecm_front_end_conntrack_notifier_init(struct dentry *dentry)
{
	/*
	 * If the device tree is used, check which accel engine's conntrack notifier
	 * can be used.
	 * For ipq8064 platform, we will use NSS.
	 */
#ifdef CONFIG_OF
	/*
	 * Check the other platforms and use the correct APIs for those platforms.
	 */
	if (!of_machine_is_compatible("qcom,ipq8064") &&
		!of_machine_is_compatible("qcom,ipq8062")) {
		return ecm_sfe_conntrack_notifier_init(dentry);
	} else {
		return ecm_nss_conntrack_notifier_init(dentry);
	}
#else
	return ecm_nss_conntrack_notifier_init(dentry);
#endif
}

/*
 * ecm_front_end_conntrack_notifier_exit()
 */
void ecm_front_end_conntrack_notifier_exit(void)
{
	/*
	 * If the device tree is used, check which accel engine's conntack notifier
	 * will be exited.
	 * For ipq8064 platforms, we will exit NSS.
	 */
#ifdef CONFIG_OF
	/*
	 * Check the other platforms and use the correct APIs for those platforms.
	 */
	if (!of_machine_is_compatible("qcom,ipq8064") &&
		!of_machine_is_compatible("qcom,ipq8062")) {
		ecm_sfe_conntrack_notifier_exit();
	} else {
		ecm_nss_conntrack_notifier_exit();
	}
#else
	ecm_nss_conntrack_notifier_exit();
#endif
}

#ifdef ECM_INTERFACE_BOND_ENABLE

/*
 * ecm_front_end_bond_notifier_stop()
 */
void ecm_front_end_bond_notifier_stop(int num)
{
	/*
	 * If the device tree is used, check which accel engine's bond notifier
	 * will be stopped.
	 * For ipq8064 platforms, we will stop NSS version.
	 */
#ifdef CONFIG_OF
	/*
	 * Check the other platforms and use the correct APIs for those platforms.
	 */
	if (!of_machine_is_compatible("qcom,ipq8064") &&
		!of_machine_is_compatible("qcom,ipq8062"))
		return;
#endif
	ecm_nss_bond_notifier_stop(num);
}

/*
 * ecm_front_end_bond_notifier_init()
 */
int ecm_front_end_bond_notifier_init(struct dentry *dentry)
{
	/*
	 * If the device tree is used, check which accel engine's bond notifier
	 * can be used.
	 * For ipq8064 platform, we will use NSS.
	 */
#ifdef CONFIG_OF
	/*
	 * Check the other platforms and use the correct APIs for those platforms.
	 */
	if (!of_machine_is_compatible("qcom,ipq8064") &&
		!of_machine_is_compatible("qcom,ipq8062"))
		return 0;
#endif
	return ecm_nss_bond_notifier_init(dentry);
}

/*
 * ecm_front_end_bond_notifier_exit()
 */
void ecm_front_end_bond_notifier_exit(void)
{
	/*
	 * If the device tree is used, check which accel engine's conntack notifier
	 * will be exited.
	 * For ipq8064 platforms, we will exit NSS.
	 */
#ifdef CONFIG_OF
	/*
	 * Check the other platforms and use the correct APIs for those platforms.
	 */
	if (!of_machine_is_compatible("qcom,ipq8064") &&
		!of_machine_is_compatible("qcom,ipq8062"))
		return;
#endif
	ecm_nss_bond_notifier_exit();
}

#endif


