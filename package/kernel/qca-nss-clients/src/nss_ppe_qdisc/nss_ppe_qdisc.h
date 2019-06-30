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

#include <linux/module.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/if_bridge.h>
#include <linux/list.h>
#include <linux/version.h>
#include <br_private.h>
#include <nss_api_if.h>
#include <nss_qdisc.h>
#include <fal_qm.h>
#include <fal_qos.h>
#include <fal_shaper.h>

#define NSS_PPE_QDISC_DEBUG_LEVEL_ERROR 1
#define NSS_PPE_QDISC_DEBUG_LEVEL_WARN 2
#define NSS_PPE_QDISC_DEBUG_LEVEL_INFO 3
#define NSS_PPE_QDISC_DEBUG_LEVEL_TRACE 4

/*
 * Debug message for module init and exit
 */
#define nss_ppe_qdisc_info_always(s, ...) printk(KERN_INFO"%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)

/*
 * Error and warn message will be enabled by default in Makefile
 */
#if (NSS_PPE_QDISC_DEBUG_LEVEL < NSS_PPE_QDISC_DEBUG_LEVEL_ERROR)
#define nss_ppe_qdisc_assert(s, ...)
#define nss_ppe_qdisc_error(s, ...)
#else
#define nss_ppe_qdisc_assert(c, s, ...) { if (!(c)) { pr_emerg("ASSERT: %s:%d:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__); BUG(); } }
#define nss_ppe_qdisc_error(s, ...) pr_err("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#if (NSS_PPE_QDISC_DEBUG_LEVEL < NSS_PPE_QDISC_DEBUG_LEVEL_WARN)
#define nss_ppe_qdisc_warning(s, ...)
#else
#define nss_ppe_qdisc_warning(s, ...) pr_warn("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#if defined(CONFIG_DYNAMIC_DEBUG)
/*
 * Compile messages for dynamic enable/disable
 */
#define nss_ppe_qdisc_info(s, ...) pr_debug("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#define nss_ppe_qdisc_trace(s, ...) pr_debug("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)

#else
/*
 * Statically compile messages at different levels
 */
#if (NSS_PPE_QDISC_DEBUG_LEVEL < NSS_PPE_QDISC_DEBUG_LEVEL_INFO)
#define nss_ppe_qdisc_info(s, ...)
#else
#define nss_ppe_qdisc_info(s, ...) pr_notice("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#if (NSS_PPE_QDISC_DEBUG_LEVEL < NSS_PPE_QDISC_DEBUG_LEVEL_TRACE)
#define nss_ppe_qdisc_trace(s, ...)
#else
#define nss_ppe_qdisc_trace(s, ...) pr_info("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif
#endif

/*
 * Queues in PPE are assigned blocks of memory (not packets)
 * Each block is 256B in size.
 */
#define NSS_PPE_MEM_BLOCK_SIZE 256

/*
 * Shaper/Scheduler levels.
 */
enum nss_ppe_qdisc_level {
	NSS_PPE_QDISC_QUEUE_LEVEL,
	NSS_PPE_QDISC_FLOW_LEVEL,
	NSS_PPE_QDISC_PORT_LEVEL,
	NSS_PPE_MAX_LEVEL,
};
typedef enum nss_ppe_qdisc_level nss_ppe_qdisc_level_t;

/*
 * Resource type.
 */
enum nss_ppe_qdisc_res_type {
	NSS_PPE_QDISC_QUEUE,
	NSS_PPE_QDISC_L0_CDRR,
	NSS_PPE_QDISC_L0_EDRR,
	NSS_PPE_QDISC_L0_SP,
	NSS_PPE_QDISC_L1_CDRR,
	NSS_PPE_QDISC_L1_EDRR,
	NSS_PPE_QDISC_MAX_RES_TYPE,
};
typedef enum nss_ppe_qdisc_res_type nss_ppe_qdisc_res_type_t;

/*
 * Supported colors
 */
enum nss_ppe_color_types {
	NSS_PPE_COLOR_GREEN,
	NSS_PPE_COLOR_YELLOW,
	NSS_PPE_COLOR_RED,
	NSS_PPE_COLOR_MAX,
};

/*
 * Resource Structure.
 */
struct nss_ppe_qdisc_res {
	nss_ppe_qdisc_res_type_t type;	/* resource type */
	uint32_t offset;		/* Resource offset */
	struct nss_ppe_qdisc_res *next; /* Pointer to next resource */
};

/*
 * nss_ppe_qdisc_port structure
 */
struct nss_ppe_qdisc_port {
	uint32_t base[NSS_PPE_QDISC_MAX_RES_TYPE];	/* Base Id */
	uint32_t max[NSS_PPE_QDISC_MAX_RES_TYPE];	/* Max resources */
	struct nss_ppe_qdisc_res *res_used[NSS_PPE_QDISC_MAX_RES_TYPE];	/* Used res list */
	struct nss_ppe_qdisc_res *res_free[NSS_PPE_QDISC_MAX_RES_TYPE];	/* Free res list */

	bool def_conf_enable;				/* Default queue configuration enabled */
	spinlock_t lock;				/* Lock to protect the port structure */
};

/*
 * nss_ppe_queue structure
 */
struct nss_ppe_queue {
	uint32_t qid;				/* Queue ID */
	uint32_t qlimit;			/* Queue limit */
	uint32_t min_th[NSS_PPE_COLOR_MAX];	/* Min threshold */
	uint32_t max_th[NSS_PPE_COLOR_MAX];	/* Max threshold */
	bool color_en;				/* Enable color mode */
	bool red_en;				/* Enable red algorithm */
	bool qid_valid;				/* Queue ID valid */
};

/*
 * nss_ppe_shaper structure
 */
struct nss_ppe_shaper {
	uint32_t rate;		/* Allowed bandwidth */
	uint32_t burst;		/* Allowed burst */
	uint32_t crate;		/* Ceil bandwidth */
	uint32_t cburst;	/* Ceil burst */
	uint32_t quantum;	/* Quantum allocation for DRR */
	uint32_t priority;	/* Priority value */
	uint32_t overhead;	/* Overhead in bytes to be added for each packet */
	bool is_valid;		/* Shaper valid */
};

/*
 * nss_ppe_qdisc structure
 */
struct nss_ppe_qdisc {
	struct nss_qdisc nq;	/* Handy pointer back to containing nss_qdisc */
	struct nss_ppe_qdisc *parent;			/* Pointer to parent nss ppe qdisc */
	enum nss_shaper_config_ppe_sn_type sub_type;	/* Type of PPE Qdisc */
				/* PPE Qdisc type */
	struct nss_ppe_queue q;	/* PPE queue related parameters */
	struct nss_ppe_shaper shaper;	/* PPE shaper parameters */
	nss_ppe_qdisc_level_t level;	/* Level at which qdisc is configured */
	uint32_t l0spid;	/* Level 0 SP Id configured in SSDK */
	uint32_t l0c_drrid;	/* Level 0 c_drr Id configured in SSDK */
	uint32_t l0e_drrid;	/* Level 0 e_drr Id configured in SSDK */
	uint32_t l1c_drrid;	/* Level 1 c_drr Id configured in SSDK */
	uint32_t l1e_drrid;	/* Level 1 e_drr Id configured in SSDK */
};

/*
 * nss_ppe_qdisc_default_conf_set()
 *	Used to set default queue scheduler in SSDK.
 */
extern int nss_ppe_qdisc_default_conf_set(uint32_t port_num);

/*
 * nss_ppe_qdisc_queue_limit_set()
 *	Used to set queue size in SSDK.
 */
extern int nss_ppe_qdisc_queue_limit_set(struct nss_ppe_qdisc *npq);

/*
 * nss_ppe_qdisc_scheduler_reset()
 *	Resets a configured scheduler in SSDK.
 */
extern int nss_ppe_qdisc_scheduler_reset(struct nss_ppe_qdisc *npq);

/*
 * nss_ppe_qdisc_scheduler_set()
 *	Configures a scheduler in SSDK.
 */
extern int nss_ppe_qdisc_scheduler_set(struct nss_ppe_qdisc *npq);

/*
 * nss_ppe_qdisc_shaper_reset()
 *	Resets a configured shaper in SSDK.
 */
extern int nss_ppe_qdisc_shaper_reset(struct nss_ppe_qdisc *npq);

/*
 * nss_ppe_qdisc_shaper_set()
 *	Configures a shaper in SSDK.
 */
extern int nss_ppe_qdisc_shaper_set(struct nss_ppe_qdisc *npq, bool is_exist);

/*
 * nss_ppe_qdisc_port_num_get()
 *	Returns the port number.
 */
extern int nss_ppe_qdisc_port_num_get(struct nss_ppe_qdisc *npq);

/*
 * nss_ppe_qdisc_is_depth_valid()
 *	Checks the depth of Qdisc tree.
 */
extern int nss_ppe_qdisc_is_depth_valid(struct nss_ppe_qdisc *npq);

/*
 * nss_ppe_qdisc_node_detach()
 *	Configuration function that helps detach a child shaper node from a parent.
 */
extern int nss_ppe_qdisc_node_detach(struct nss_ppe_qdisc *npq, struct Qdisc *old);

/*
 * nss_ppe_qdisc_node_attach()
 *	Configuration function that helps attach a child shaper node to a parent.
 */
extern int nss_ppe_qdisc_node_attach(struct nss_ppe_qdisc *npq, struct Qdisc *new);

/*
 * nss_ppe_qdisc_node_attach()
 *	Configures the SSDK schedulers and NSS shapers.
 */
extern int nss_ppe_qdisc_configure(struct nss_ppe_qdisc *npq);

/*
 * nss_ppe_qdisc_init()
 *	Initializes NSS Qdisc and sets the level of this qdisc.
 */
extern int nss_ppe_qdisc_init(struct Qdisc *sch, struct nss_ppe_qdisc *npq, nss_shaper_node_type_t type,
				enum nss_shaper_config_ppe_sn_type sub_type, uint32_t classid);
