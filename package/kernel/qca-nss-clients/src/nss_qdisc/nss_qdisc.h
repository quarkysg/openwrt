/*
 **************************************************************************
 * Copyright (c) 2014-2017 The Linux Foundation. All rights reserved.
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

/*
 * NSS QDisc debug macros
 */
#if (NSS_QDISC_DEBUG_LEVEL < 1)
#define nss_qdisc_assert(c, s, args...)
#define nss_qdisc_error(fmt, args...)
#else
#define nss_qdisc_assert(c, s, args...) if (!(c)) { printk("%d:ASSERT:"s, __LINE__, ##args); BUG(); }
#define nss_qdisc_error(fmt, args...) printk(KERN_ERR "%d:ERROR:"fmt, __LINE__, ##args)
#endif

#if (NSS_QDISC_DEBUG_LEVEL < 2)
#define nss_qdisc_warning(fmt, args...)
#else
#define nss_qdisc_warning(fmt, args...) printk(KERN_WARNING "%d:WARN:"fmt, __LINE__, ##args)
#endif

#if (NSS_QDISC_DEBUG_LEVEL < 3)
#define nss_qdisc_info(fmt, args...)
#else
#define nss_qdisc_info(fmt, args...) printk(KERN_INFO "%d:INFO:"fmt, __LINE__, ##args)
#endif

#if (NSS_QDISC_DEBUG_LEVEL < 4)
#define nss_qdisc_trace(fmt, args...)
#else
#define nss_qdisc_trace(fmt, args...) printk(KERN_DEBUG "%d:TRACE:"fmt, __LINE__, ##args)
#endif

/*
 * State values
 */
#define NSS_QDISC_STATE_IDLE 0
#define NSS_QDISC_STATE_READY 1

#define NSS_QDISC_STATE_INIT_FAILED -1
#define NSS_QDISC_STATE_ASSIGN_SHAPER_SEND_FAIL -2
#define NSS_QDISC_STATE_SHAPER_ASSIGN_FAILED -3
#define NSS_QDISC_STATE_NODE_ALLOC_SEND_FAIL -4
#define NSS_QDISC_STATE_NODE_ALLOC_FAIL -5
#define NSS_QDISC_STATE_ROOT_SET_SEND_FAIL -6
#define NSS_QDISC_STATE_ROOT_SET_FAIL -7
#define NSS_QDISC_STATE_DEFAULT_SET_SEND_FAIL -8
#define NSS_QDISC_STATE_DEFAULT_SET_FAIL -9
#define NSS_QDISC_STATE_CHILD_ALLOC_SEND_FAIL -10
#define NSS_QDISC_STATE_NODE_ALLOC_FAIL_CHILD -11
#define NSS_QDISC_STATE_FAILED_RESPONSE -12
#define NSS_QDISC_STATE_UNASSIGN_SHAPER_SEND_FAIL -13
#define NSS_QDISC_STATE_UNASSIGN_SHAPER_FAIL -14
#define NSS_QDISC_STATE_NODE_FREE_FAIL -15
#define NSS_QDISC_STATE_NODE_FREE_SEND_FAIL -16

#define NSS_QDISC_BRIDGE_PORT_MAX 100

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,8,0))
#define nss_qdisc_hlist_for_each_entry(tpos, pos, head, member) hlist_for_each_entry(tpos, pos, head, member)
#define nss_qdisc_hlist_for_each_entry_safe(tpos, pos, n, head, member) hlist_for_each_entry_safe(tpos, pos, n, head, member)
#define nss_qdisc_get_dev_master(dev) (dev->master)
#define nss_qdisc_get_dev(ptr) (struct net_device *)ptr
#else
#define nss_qdisc_hlist_for_each_entry(tpos, pos, head, member) hlist_for_each_entry(tpos, head, member)
#define nss_qdisc_hlist_for_each_entry_safe(tpos, pos, n, head, member) hlist_for_each_entry_safe(tpos, n, head, member)
#define nss_qdisc_get_dev_master(dev) netdev_master_upper_dev_get(dev)
#define nss_qdisc_get_dev(ptr) netdev_notifier_info_to_dev(ptr)
#endif

/*
 * Mode of Qdisc/class
 * These are defined as Magic numbers to avoid false positives
 */
#define NSS_QDISC_MODE_NSS 0x6243
#define NSS_QDISC_MODE_PPE 0x6245

struct nss_qdisc {
	struct Qdisc *qdisc;			/* Handy pointer back to containing qdisc */
	struct nss_qdisc *parent;		/* Pointer to parent nss qdisc */
	void *nss_shaping_ctx;			/* NSS context for general operations */
	int32_t nss_interface_number;		/* NSS Interface number we are shaping on */
	nss_shaper_node_type_t type;		/* Type of shaper node */
	bool is_class;				/* True if this represents a class and not a qdisc */
	bool is_root;				/* True if root qdisc on a net device */
	bool is_bridge;				/* True when qdisc is a bridge */
	bool is_virtual;			/* True when the device is represented as a virtual in
						 * the NSS e.g. perhaps operating on a wifi interface
						 * or bridge.
						 */
	bool destroy_virtual_interface;		/* Set if the interface is first registered in NSS by
						 * us. This means it needs to be un-regisreted when the
						 * module goes down.
						 */
	atomic_t state;				/* < 0: Signal that qdisc has 'failed'. 0
						 * indicates 'pending' setup.  > 0 is READY.
						 * NOTE: volatile AND atomic - this is polled
						 * AND is used for syncronisation.
						 */
	uint32_t shaper_id;			/* Used when is_root. Child qdiscs use this
						 * information to know what shaper under
						 * which to create shaper nodes
						 */
	uint32_t qos_tag;			/* QoS tag of this node */
	volatile int32_t pending_final_state;	/* Used to let the callback cycle know what
						 * state to set the qdisc in on successful
						 * completion.
						 */
	struct nss_virt_if_handle *virt_if_ctx;	/* Context provided by the NSS driver for
						 * new interfaces that are registered.
						 */
	void *bounce_context;			/* Context for bounce registration. Bounce
						 * enables packets to be sent to NSS for
						 * shaping purposes, and is returned to
						 * Linux for transmit.
						 */
	spinlock_t bounce_protection_lock;	/* Lock to protect the enqueue and dequeue
						 * operation on skb lists triggeret by bounce
						 * callbacks.
						 */
	void (*stats_update_callback)(void *, struct nss_shaper_configure *);
						/* Stats update callback function for qdisc specific
						 * stats update. Currently unused.
						 */
	struct gnet_stats_basic_packed bstats;	/* Basic class statistics */
	struct gnet_stats_queue qstats;		/* Qstats for use by classes */
	atomic_t refcnt;			/* Reference count for class use */
	struct timer_list stats_get_timer;	/* Timer used to poll for stats */
	atomic_t pending_stat_requests;		/* Number of pending stats responses */
	struct nss_shaper_shaper_node_basic_stats_get basic_stats_latest;
						/* Latest stats obtained */
	wait_queue_head_t wait_queue;		/* Wait queue used to wait on responses from the NSS */
	spinlock_t lock;			/* Lock to protect the nss qdisc structure */
	uint16_t mode;				/* Mode of Qdisc/class */
};

/*
 * nss_qdisc bridge update structure
 */
struct nss_qdisc_bridge_update {
	int port_list[NSS_QDISC_BRIDGE_PORT_MAX];
	int port_list_count;
	int unassign_count;
};

/*
 * Task types for bridge scanner.
 */
enum nss_qdisc_bshaper_tasks {
	NSS_QDISC_SCAN_AND_ASSIGN_BSHAPER,
	NSS_QDISC_SCAN_AND_UNASSIGN_BSHAPER,
};

/*
 * Types of messages sent down to NSS interfaces
 */
enum nss_qdisc_interface_msgs {
	NSS_QDISC_IF_SHAPER_ASSIGN,
	NSS_QDISC_IF_SHAPER_UNASSIGN,
	NSS_QDISC_IF_SHAPER_CONFIG,
};

/*
 * Types of mode for hybrid configuration.
 */
enum nss_qdisc_hybrid_mode {
	NSS_QDISC_HYBRID_MODE_DISABLE,
	NSS_QDISC_HYBRID_MODE_ENABLE,
};

/*
 * nss_qdisc_peek()
 *	Called to peek at the head of an nss qdisc
 */
extern struct sk_buff *nss_qdisc_peek(struct Qdisc *sch);

/*
 * nss_qdisc_drop()
 *	Called to drop the packet at the head of queue
 */
extern unsigned int nss_qdisc_drop(struct Qdisc *sch);

/*
 * nss_qdisc_reset()
 *	Called when a qdisc is reset
 */
extern void nss_qdisc_reset(struct Qdisc *sch);

/*
 * nss_qdisc_enqueue()
 *	Generic enqueue call for enqueuing packets into NSS for shaping
 */
extern int nss_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *sch);

/*
 * nss_qdisc_dequeue()
 *	Generic dequeue call for dequeuing bounced packets.
 */
extern struct sk_buff *nss_qdisc_dequeue(struct Qdisc *sch);

/*
 * nss_qdisc_set_hybrid_mode()
 *	Configuration function that enables/disables hybrid mode
 */
extern int nss_qdisc_set_hybrid_mode(struct nss_qdisc *nq, enum nss_qdisc_hybrid_mode mode, uint32_t offset);

/*
 * nss_qdisc_node_set_default()
 *	Configuration function that sets shaper node as default for packet enqueue
 */
extern int nss_qdisc_set_default(struct nss_qdisc *nq);

/*
 * nss_qdisc_node_attach()
 *	Configuration function that helps attach a child shaper node to a parent.
 */
extern int nss_qdisc_node_attach(struct nss_qdisc *nq, struct nss_qdisc *nq_child,
					struct nss_if_msg *nim, int32_t attach_type);

/*
 * nss_qdisc_node_detach()
 *	Configuration function that helps detach a child shaper node to a parent.
 */
extern int nss_qdisc_node_detach(struct nss_qdisc *nq, struct nss_qdisc *nq_child,
					struct nss_if_msg *nim, int32_t detach_type);

/*
 * nss_qdisc_configure()
 *	Configuration function that aids in tuning of queuing parameters.
 */
extern int nss_qdisc_configure(struct nss_qdisc *nq,
	struct nss_if_msg *nim, int32_t config_type);

/*
 * nss_qdisc_destroy()
 *	Destroys a shaper in NSS, and the sequence is based on the position of
 *	this qdisc (child or root) and the interface to which it is attached to.
 */
extern void nss_qdisc_destroy(struct nss_qdisc *nq);

/*
 * nss_qdisc_init()
 *	Initializes a shaper in NSS, based on the position of this qdisc (child or root)
 *	and if its a normal interface or a bridge interface.
 */
extern int nss_qdisc_init(struct Qdisc *sch, struct nss_qdisc *nq, uint16_t mode, nss_shaper_node_type_t type, uint32_t classid);

/*
 * nss_qdisc_start_basic_stats_polling()
 *	Call to initiate the stats polling timer
 */
extern void nss_qdisc_start_basic_stats_polling(struct nss_qdisc *nq);

/*
 * nss_qdisc_stop_basic_stats_polling()
 *	Call to stop polling of basic stats
 */
extern void nss_qdisc_stop_basic_stats_polling(struct nss_qdisc *nq);

/*
 * nss_qdisc_gnet_stats_copy_basic()
 *  Wrapper around gnet_stats_copy_basic()
 */
extern int nss_qdisc_gnet_stats_copy_basic(struct gnet_dump *d,
				struct gnet_stats_basic_packed *b);

/*
 * nss_qdisc_gnet_stats_copy_queue()
 *  Wrapper around gnet_stats_copy_queue()
 */
extern int nss_qdisc_gnet_stats_copy_queue(struct gnet_dump *d,
				struct gnet_stats_queue *q);

/*
 * nss_qdisc_replace()
 *	Used to replace old qdisc with a new one
 */
extern struct Qdisc *nss_qdisc_replace(struct Qdisc *sch, struct Qdisc *new,
					struct Qdisc **pold);

/*
 * nss_qdisc_ppe_mod_owner_set()()
 *	Sets the nss_qdisc_ppe module owner.
 */
extern void nss_qdisc_ppe_mod_owner_set(struct module *owner);
