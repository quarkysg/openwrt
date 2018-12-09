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

#include "nss_ppe_qdisc.h"

#define NSS_PPE_HTB_MAX_PRIORITY 4

/*
 * nss_ppe_htb class instance structure
 */
struct nss_ppe_htb_class_data {
	struct nss_ppe_qdisc npq;		/* Base class used by nss_ppe_qdisc */
	struct Qdisc_class_common sch_common;	/* Common class structure for scheduler use */
	struct nss_ppe_htb_class_data *parent;	/* Pointer to our parent class */
	struct Qdisc *qdisc;			/* Child qdisc, used by leaf classes */
	uint children;				/* Count of number of attached child classes */
	bool is_leaf;				/* True if leaf class */
};

/*
 * nss_ppe_htb qdisc instance structure
 */
struct nss_ppe_htb_sched_data {
	struct nss_ppe_qdisc npq;		/* Base class used by nss_ppe_qdisc */
	u16 r2q;				/* The rate to quantum conversion ratio */
	struct nss_ppe_htb_class_data root;	/* Root class */
	struct Qdisc_class_hash clhash;		/* Class hash */
};

/*
 * nss_ppe_htb_find_class()
 *	Returns a pointer to class if classid matches with a class under this qdisc.
 */
static inline struct nss_ppe_htb_class_data *nss_ppe_htb_find_class(u32 classid, struct Qdisc *sch)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	struct Qdisc_class_common *clc;
	clc = qdisc_class_find(&q->clhash, classid);
	if (!clc) {
		nss_ppe_qdisc_info("cannot find class with classid %x in qdisc %x hash\n",
				classid, sch->handle);
		return NULL;
	}
	return container_of(clc, struct nss_ppe_htb_class_data, sch_common);
}

/*
 * nss_ppe_htb_policy structure
 */
static const struct nla_policy nss_ppe_htb_policy[TCA_PPEHTB_MAX + 1] = {
	[TCA_PPEHTB_CLASS_PARMS] = { .len = sizeof(struct tc_ppehtb_class_qopt) },
};

/*
 * nss_ppe_htb_params_validate_and_save
 *	Validates and saves the qdisc configuration parameters.
 */
static int nss_ppe_htb_params_validate_and_save(struct Qdisc *sch,
		struct nlattr *opt, struct nss_ppe_shaper *shaper)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	struct nlattr *na[TCA_PPEHTB_MAX + 1];
	struct tc_ppehtb_class_qopt *qopt;
	struct net_device *dev = qdisc_dev(sch);
	unsigned int mtu = psched_mtu(dev);
	int err;

	nss_ppe_qdisc_info("validating parameters for ppehtb class of qdisc:%x\n", sch->handle);

	if (!opt) {
		nss_ppe_qdisc_warning("passing null opt for configuring ppehtb class of qdisc:%x\n", sch->handle);
		return -EINVAL;
	}

	err = nla_parse_nested(na, TCA_PPEHTB_MAX, opt, nss_ppe_htb_policy);
	if (err < 0) {
		nss_ppe_qdisc_warning("failed to parse configuration parameters for ppehtb class of qdisc:%x\n",
					sch->handle);
		return err;
	}

	if (na[TCA_PPEHTB_CLASS_PARMS] == NULL) {
		nss_ppe_qdisc_warning("parsed values have no content - ppehtb class of qdisc:%x\n", sch->handle);
		return -EINVAL;
	}

	qopt = nla_data(na[TCA_PPEHTB_CLASS_PARMS]);

	sch_tree_lock(sch);
	if (qopt->rate && !qopt->burst) {
		nss_ppe_qdisc_warning("burst needed if rate is non zero - ppehtb class of qdisc:%x\n", sch->handle);
		sch_tree_unlock(sch);
		return -EINVAL;
	}

	if (!qopt->crate || !qopt->cburst) {
		nss_ppe_qdisc_warning("crate and cburst need to be non zero - ppehtb class of qdisc:%x\n", sch->handle);
		sch_tree_unlock(sch);
		return -EINVAL;
	}

	if (!(qopt->priority < NSS_PPE_HTB_MAX_PRIORITY)) {
		nss_ppe_qdisc_warning("priority %u of htb class greater than max prio %u for ppehtb class of qdisc:%x",
					qopt->priority, NSS_PPE_HTB_MAX_PRIORITY, sch->handle);
		sch_tree_unlock(sch);
		return -EINVAL;
	}

	if ((qopt->quantum % 1024) != 0) {
		nss_ppe_qdisc_warning("quantum %u of htb class of qdisc %x should be a multiple of 1024 with the max 1024*1024\n",
					qopt->quantum, sch->handle);
		sch_tree_unlock(sch);
		return -EINVAL;
	}

	memset(shaper, 0, sizeof(*shaper));
	shaper->rate = qopt->rate;
	shaper->burst = qopt->burst;
	shaper->crate = qopt->crate;
	shaper->cburst = qopt->cburst;
	shaper->overhead = qopt->overhead;
	shaper->quantum = qopt->quantum;
	shaper->priority = qopt->priority;

	/*
	 * If quantum value is not provided, set it to
	 * the interface's MTU value.
	 */
	if (!shaper->quantum) {
		/*
		 * If quantum was not provided, we have two options.
		 * One, use r2q and rate to figure out the quantum. Else,
		 * use the interface's MTU as the value of quantum.
		 */
		if (q->r2q && shaper->rate) {
			shaper->quantum = (shaper->rate / q->r2q) / 8;
			nss_ppe_qdisc_info("quantum not provided for htb class of qdisc %x on interface.\n"
					"Setting quantum to %uB based on r2q %u and rate %uBps\n", sch->handle,
					 shaper->quantum, q->r2q, shaper->rate / 8);
		} else {
			shaper->quantum = mtu;
			nss_ppe_qdisc_info("quantum value not provided for htb class of qdisc %x on interface.\n"
					"Setting quantum to MTU %uB\n", sch->handle, shaper->quantum);
		}
	}
	sch_tree_unlock(sch);
	return 0;
}

/*
 * nss_ppe_htb_change_class()
 *	Allocates a new class.
 */
static struct nss_ppe_htb_class_data *nss_ppe_htb_class_alloc(struct Qdisc *sch, struct nss_ppe_htb_class_data *parent, u32 classid)
{
	struct nss_ppe_htb_class_data *cl;

	nss_ppe_qdisc_info("creating a new ppehtb class of qdisc:%x\n", sch->handle);

	/*
	 * check for valid classid
	 */
	if (!classid || TC_H_MAJ(classid ^ sch->handle) || nss_ppe_htb_find_class(classid, sch)) {
		nss_ppe_qdisc_warning("ppehtb: invalid classid\n");
		return NULL;
	}

	/*
	 * check maximal depth
	 */
	if (parent && !nss_ppe_qdisc_is_depth_valid(&parent->npq)) {
		nss_ppe_qdisc_warning("ppehtb: tree is too deep\n");
		return NULL;
	}

	nss_ppe_qdisc_trace("ppehtb class %x not found. Allocating a new class.\n", classid);
	cl = kzalloc(sizeof(struct nss_ppe_htb_class_data), GFP_KERNEL);

	if (!cl) {
		nss_ppe_qdisc_warning("class allocation failed for classid %x\n", classid);
		return NULL;
	}

	cl->parent = parent;
	cl->sch_common.classid = classid;

	/*
	 * Set this class as leaf. If a new class is attached as
	 * child, it will set this value to false during the attach
	 * process.
	 */
	cl->is_leaf = true;

	/*
	 * We make the child qdisc a noop qdisc, and
	 * set reference count to 1. This is important,
	 * reference count should not be 0.
	 */
	cl->qdisc = &noop_qdisc;
	atomic_set(&cl->npq.nq.refcnt, 1);

	return cl;
}

/*
 * nss_ppe_htb_change_class()
 *	Configures a new class.
 */
static int nss_ppe_htb_change_class(struct Qdisc *sch, u32 classid, u32 parentid,
			struct nlattr **tca, unsigned long *arg)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	struct nss_ppe_htb_class_data *cl = (struct nss_ppe_htb_class_data *)*arg;
	struct nss_ppe_htb_class_data *parent;
	struct nss_qdisc *nq_parent;
	struct nlattr *opt = tca[TCA_OPTIONS];
	struct nss_ppe_shaper shaper;
	bool is_exist = true;
	struct nss_if_msg nim_attach;

	nss_ppe_qdisc_info("configuring ppehtb class %x of qdisc %x\n", classid, sch->handle);

	if (nss_ppe_htb_params_validate_and_save(sch, opt, &shaper) < 0) {
		nss_ppe_qdisc_warning("validation of configuration parameters for htb class %x failed\n",
					classid);
		return -EINVAL;
	}

	parent = (parentid == TC_H_ROOT) ? NULL : nss_ppe_htb_find_class(parentid, sch);

	/*
	 * The parent could be the ppehtb qdisc, or a class. We set nq_parent pointer
	 * accordingly.
	 */
	if (parent) {
		nq_parent = &parent->npq.nq;
	} else {
		nq_parent = &q->npq.nq;
	}

	/*
	 * If class with a given classid is not found, we allocate a new one
	 */
	if (!cl) {
		is_exist = false;

		nss_ppe_qdisc_trace("ppehtb class %x not found. Allocating a new class.\n", classid);
		cl = nss_ppe_htb_class_alloc(sch, parent, classid);

		if (!cl) {
			nss_ppe_qdisc_warning("class allocation failed for classid %x\n", classid);
			goto failure;
		}

		*arg = (unsigned long)cl;
		nss_ppe_qdisc_trace("ppehtb class %x allocated - addr %p\n", classid, cl);

		/*
		 * This is where a class gets initialized. Classes do not have a init function
		 * that is registered to Linux. Therefore we initialize the PPEHTB_GROUP shaper
		 * here.
		 */
		if (nss_ppe_qdisc_init(sch, &cl->npq, NSS_SHAPER_NODE_TYPE_PPE_SN, NSS_SHAPER_CONFIG_PPE_SN_TYPE_HTB_GROUP, classid) < 0) {
			nss_ppe_qdisc_warning("ppe_init for ppehtb class %x failed\n", classid);
			goto failure;
		}

		cl->npq.shaper = shaper;
		cl->npq.shaper.is_valid = true;
		if (parent) {
			cl->npq.parent = &parent->npq;
			cl->npq.level = parent->npq.level - 1;
		}

		if (parentid != TC_H_ROOT) {
			if (nss_ppe_qdisc_shaper_set(&cl->npq, false) != 0)  {
				nss_ppe_qdisc_warning("ppe_htb %x SSDK shaper configuration failed\n", sch->handle);
				nss_qdisc_destroy(&cl->npq.nq);
				goto failure;
			}
		}

		/*
		 * Set qos_tag of parent to which the class needs to be attached to,
		 * set the child to be this class and send node_attach command down to the NSS
		 */
		nim_attach.msg.shaper_configure.config.msg.shaper_node_config.qos_tag = nq_parent->qos_tag;
		nim_attach.msg.shaper_configure.config.msg.shaper_node_config.snc.ppe_sn_attach.child_qos_tag = cl->npq.nq.qos_tag;
		if (nss_qdisc_node_attach(nq_parent, &cl->npq.nq, &nim_attach,
			NSS_SHAPER_CONFIG_TYPE_SHAPER_NODE_ATTACH) < 0) {
			nss_ppe_qdisc_warning("ppe_attach for class %x failed\n", classid);
			goto failure1;
		}

		/*
		 * We have successfully attached ourselves in the NSS. We can therefore
		 * add this class to qdisc hash tree, and increment parent's child count
		 * (if parent exists)
		 */
		sch_tree_lock(sch);
		qdisc_class_hash_insert(&q->clhash, &cl->sch_common);
		if (parent) {
			parent->children++;

			/*
			 * Parent can no longer be leaf. Set flag to false.
			 */
			parent->is_leaf = false;
		}
		sch_tree_unlock(sch);

		/*
		 * Hash grow should not come within the tree lock
		 */
		qdisc_class_hash_grow(sch, &q->clhash);

		/*
		 * Start the stats polling timer
		 */
		nss_qdisc_start_basic_stats_polling(&cl->npq.nq);
		nss_ppe_qdisc_trace("class %x successfully allocated and initialized\n", classid);
	}

	cl->npq.shaper = shaper;
	cl->npq.shaper.is_valid = true;

	/*
	 * Set configuration in SSDK
	 */
	if ((parentid != TC_H_ROOT) && (is_exist)) {
		if (nss_ppe_qdisc_shaper_set(&cl->npq, true) != 0) {
			nss_ppe_qdisc_warning("ppe_htb %x SSDK shaper configuration failed\n", sch->handle);
			return -EINVAL;
		}
	}

	nss_ppe_qdisc_info("ppehtb class %x configured successfully\n", classid);
	return 0;

failure1:
	/*
	 * Reset the SSDK configuration done while allocating the class.
	 */
	if (parentid != TC_H_ROOT) {
		nss_ppe_qdisc_shaper_reset(&cl->npq);
	}

	nss_qdisc_destroy(&cl->npq.nq);

failure:
	if (cl) {
		kfree(cl);
	}
	return -EINVAL;
}

/*
 * nss_ppe_htb_destroy_class()
 *	Detaches all child nodes and destroys the class.
 */
static void nss_ppe_htb_destroy_class(struct Qdisc *sch, struct nss_ppe_htb_class_data *cl)
{
	struct nss_ppe_htb_sched_data *q __maybe_unused = qdisc_priv(sch);

	nss_ppe_qdisc_trace("destroying ppehtb class %x from qdisc %x\n",
				cl->npq.nq.qos_tag, sch->handle);

	if (cl == &q->root) {
		nss_ppe_qdisc_info("We do not destroy ppe_htb class %x here since this is "
				"the qdisc %p\n", cl, sch->handle);
		return;
	}

	/*
	 * We always have to detach the child qdisc, before destroying it.
	 */
	if (cl->qdisc != &noop_qdisc) {
		if (nss_ppe_qdisc_node_detach(&cl->npq, cl->qdisc) < 0) {
			nss_ppe_qdisc_warning("detach of child qdisc %p failed\n", cl->qdisc);
			return;
		}
	}

	/*
	 * And now we destroy the child.
	 */
	qdisc_destroy(cl->qdisc);

	/*
	 * Reset the SSDK configuration done while allocating the class.
	 */
	if (nss_ppe_qdisc_shaper_reset(&cl->npq) != 0) {
		nss_ppe_qdisc_warning("ppe_htb %x SSDK reset shaper configuration failed\n", sch->handle);
	}

	/*
	 * Stop the stats polling timer and free class
	 */
	nss_qdisc_stop_basic_stats_polling(&cl->npq.nq);

	/*
	 * Destroy the shaper in NSS
	 */
	nss_qdisc_destroy(&cl->npq.nq);

	/*
	 * Free class
	 */
	kfree(cl);
}

/*
 * nss_ppe_htb_delete_class()
 *	Detaches a class from operation, but does not destroy it.
 */
static int nss_ppe_htb_delete_class(struct Qdisc *sch, unsigned long arg)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	struct nss_ppe_htb_class_data *cl = (struct nss_ppe_htb_class_data *)arg;
	struct nss_if_msg nim;

	/*
	 * If the class still has child nodes, then we do not
	 * support deleting it.
	 */
	if (cl->children) {
		nss_ppe_qdisc_warning("ppe_htb %x Class has child qdisc attached\n", sch->handle);
		return -EBUSY;
	}

	/*
	 * Check if we are root class or not (parent pointer is NULL for a root class)
	 */
	if (cl->parent) {
		/*
		 * The ppehtb class to be detached has a parent class (i.e. not the root class),
		 * so we need to send a detach msg to its parent class.
		 */
		nss_ppe_qdisc_info("detaching from parent ppehtb class %x\n", cl->parent->npq.nq.qos_tag);
		nim.msg.shaper_configure.config.msg.shaper_node_config.qos_tag = cl->parent->npq.nq.qos_tag;
	} else {
		/*
		 * The message to NSS should be sent to the root qdisc of this class
		 */
		nss_ppe_qdisc_info("detaching from parent ppehtb qdisc %x\n", q->npq.nq.qos_tag);
		nim.msg.shaper_configure.config.msg.shaper_node_config.qos_tag = q->npq.nq.qos_tag;
	}
	nim.msg.shaper_configure.config.msg.shaper_node_config.snc.ppe_sn_detach.child_qos_tag = cl->npq.nq.qos_tag;
	if (nss_qdisc_node_detach(&q->npq.nq, &cl->npq.nq, &nim, NSS_SHAPER_CONFIG_TYPE_SHAPER_NODE_DETACH) < 0) {
		nss_ppe_qdisc_error("ppe_htb %x Detaching from parent failed\n", sch->handle);
		return -EINVAL;
	}

	sch_tree_lock(sch);
	qdisc_reset(cl->qdisc);
	qdisc_class_hash_remove(&q->clhash, &cl->sch_common);

	/*
	 * If we are root class, we dont have to update our parent.
	 * We simply deduct refcnt and return.
	 */
	if (!cl->parent) {
		atomic_sub(1, &cl->npq.nq.refcnt);
		sch_tree_unlock(sch);
		return 0;
	}

	/*
	 * We are not root class. Therefore we reduce the children count
	 * for our parent and also update its 'is_leaf' status.
	 */
	cl->parent->children--;
	if (!cl->parent->children) {
		cl->parent->is_leaf = true;
	}

	/*
	 * Decrement refcnt and return
	 */
	atomic_sub(1, &cl->npq.nq.refcnt);
	sch_tree_unlock(sch);

	return 0;
}

/*
 * nss_ppe_htb_graft_class()
 *	Replaces the qdisc attached to the provided class.
 */
static int nss_ppe_htb_graft_class(struct Qdisc *sch, unsigned long arg, struct Qdisc *new, struct Qdisc **old)
{
	struct nss_ppe_htb_class_data *cl = (struct nss_ppe_htb_class_data *)arg;

	nss_ppe_qdisc_trace("grafting ppehtb class %x\n", cl->npq.nq.qos_tag);

	if (!new) {
		new = &noop_qdisc;
	}

	sch_tree_lock(sch);
	*old = cl->qdisc;
	sch_tree_unlock(sch);

	/*
	 * Since we initially attached a noop qdisc as child (in Linux),
	 * we do not perform a detach in the NSS if its a noop qdisc.
	 */
	nss_ppe_qdisc_info("grafting old: %x with new: %x\n", (*old)->handle, new->handle);
	if (*old != &noop_qdisc) {
		nss_ppe_qdisc_trace("detaching old: %x\n", (*old)->handle);
		if (nss_ppe_qdisc_node_detach(&cl->npq, *old) < 0) {
			nss_ppe_qdisc_warning("detach of old qdisc %x failed\n", (*old)->handle);
			return -EINVAL;
		}
	}

	/*
	 * If the new qdisc is a noop qdisc, we do not send down an attach command
	 * to the NSS.
	 */
	if (new != &noop_qdisc) {
		nss_ppe_qdisc_trace("attaching new: %x\n", new->handle);
		if (nss_ppe_qdisc_node_attach(&cl->npq, new) < 0) {
			nss_ppe_qdisc_warning("attach of new qdisc %x failed\n", new->handle);
			return -EINVAL;
		}

	}

	/*
	 * Replaced in NSS, now replace in Linux.
	 */
	nss_qdisc_replace(sch, new, &cl->qdisc);

	return 0;
}

/*
 * nss_ppe_htb_leaf_class()
 *	Returns pointer to qdisc if leaf class.
 */
static struct Qdisc *nss_ppe_htb_leaf_class(struct Qdisc *sch, unsigned long arg)
{
	struct nss_ppe_htb_class_data *cl = (struct nss_ppe_htb_class_data *)arg;
	nss_ppe_qdisc_trace("ppehtb class %x is leaf %d\n", cl->npq.nq.qos_tag, cl->is_leaf);

	/*
	 * Return qdisc pointer if this is level 0 class
	 */
	return cl->is_leaf ? cl->qdisc : NULL;
}

/*
 * nss_ppe_htb_qlen_notify()
 *	We dont maintain a live set of stats in linux, so this function is not implemented.
 */
static void nss_ppe_htb_qlen_notify(struct Qdisc *sch, unsigned long arg)
{
	nss_ppe_qdisc_trace("qlen notify called for ppehtb qdisc %x\n", sch->handle);

	/*
	 * Gets called when qlen of child changes (Useful for deactivating)
	 * Not useful for us here.
	 */
}

/*
 * nss_ppe_htb_get_class()
 *	Fetches the class pointer if provided the classid.
 */
static unsigned long nss_ppe_htb_get_class(struct Qdisc *sch, u32 classid)
{
	struct nss_ppe_htb_class_data *cl = nss_ppe_htb_find_class(classid, sch);

	if (cl != NULL) {
		nss_ppe_qdisc_trace("fetched ppehtb class %x from qdisc %x\n",
				cl->npq.nq.qos_tag, sch->handle);
		atomic_add(1, &cl->npq.nq.refcnt);
	}

	return (unsigned long)cl;
}

/*
 * nss_ppe_htb_put_class()
 *	Reduces reference count for this class.
 */
static void nss_ppe_htb_put_class(struct Qdisc *sch, unsigned long arg)
{
	struct nss_ppe_htb_class_data *cl = (struct nss_ppe_htb_class_data *)arg;
	nss_ppe_qdisc_trace("executing put on ppehtb class %x in qdisc %x\n",
			cl->npq.nq.qos_tag, sch->handle);

	/*
	 * We are safe to destroy the qdisc if the reference count
	 * goes down to 0.
	 */
	if (atomic_sub_return(1, &cl->npq.nq.refcnt) == 0) {
		nss_ppe_htb_destroy_class(sch, cl);
	}
}

/*
 * nss_ppe_htb_dump_class()
 *	Dumps all configurable parameters pertaining to this class.
 */
static int nss_ppe_htb_dump_class(struct Qdisc *sch, unsigned long arg, struct sk_buff *skb, struct tcmsg *tcm)
{
	struct nss_ppe_htb_class_data *cl = (struct nss_ppe_htb_class_data *)arg;
	struct nlattr *opts;
	struct tc_ppehtb_class_qopt qopt;

	nss_ppe_qdisc_trace("dumping ppehtb class %x of qdisc %x\n", cl->npq.nq.qos_tag, sch->handle);

	qopt.burst = cl->npq.shaper.burst;
	qopt.rate = cl->npq.shaper.rate;
	qopt.crate = cl->npq.shaper.crate;
	qopt.cburst = cl->npq.shaper.cburst;
	qopt.overhead = cl->npq.shaper.overhead;
	qopt.quantum = cl->npq.shaper.quantum;
	qopt.priority = cl->npq.shaper.priority;

	/*
	 * All ppehtb group nodes are root nodes. i.e. they dont
	 * have any mode ppehtb groups attached beneath them.
	 */
	tcm->tcm_parent = TC_H_ROOT;
	tcm->tcm_handle = cl->sch_common.classid;
	tcm->tcm_info = cl->qdisc->handle;

	opts = nla_nest_start(skb, TCA_OPTIONS);

	if (!opts || nla_put(skb, TCA_PPEHTB_CLASS_PARMS, sizeof(qopt), &qopt)) {
		goto nla_put_failure;
	}

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	nss_ppe_qdisc_warning("ppehtb class %x dumo failed\n", cl->npq.nq.qos_tag);
	return -EMSGSIZE;
}

/*
 * nss_ppe_htb_dump_class_stats()
 *	Dumps class statistics.
 */
static int nss_ppe_htb_dump_class_stats(struct Qdisc *sch, unsigned long arg, struct gnet_dump *d)
{
	struct nss_qdisc *nq = (struct nss_qdisc *)arg;

	if (nss_qdisc_gnet_stats_copy_basic(d, &nq->bstats) < 0 ||
			nss_qdisc_gnet_stats_copy_queue(d, &nq->qstats) < 0) {
		nss_ppe_qdisc_error("ppehtb class %x stats dump failed\n", nq->qos_tag);
		return -1;
	}

	return 0;
}

/*
 * nss_ppe_htb_walk()
 *	Used to walk the tree.
 */
static void nss_ppe_htb_walk(struct Qdisc *sch, struct qdisc_walker *arg)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	struct hlist_node *n __maybe_unused;
	struct nss_ppe_htb_class_data *cl;
	unsigned int i;

	nss_ppe_qdisc_trace("walking ppehtb qdisc %x\n", sch->handle);

	if (arg->stop)
		return;

	for (i = 0; i < q->clhash.hashsize; i++) {
		nss_qdisc_hlist_for_each_entry(cl, n, &q->clhash.hash[i],
				sch_common.hnode) {
			if (arg->count < arg->skip) {
				arg->count++;
				continue;
			}
			if (arg->fn(sch, (unsigned long)cl, arg) < 0) {
				arg->stop = 1;
				return;
			}
			arg->count++;
		}
	}
}

/*
 * nss_ppe_htb_change_qdisc()
 *	Can be used to configure a ppehtb qdisc.
 */
static int nss_ppe_htb_change_qdisc(struct Qdisc *sch, struct nlattr *opt)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	struct tc_ppehtb_qopt *qopt;

	/*
	 * Since ppehtb can be created with no arguments, opt might be NULL
	 * (depending on the kernel version). This is still a valid create
	 * request.
	 */
	if (!opt) {

		/*
		 * If no parameter is passed, set it to 0 and continue
		 * creating the qdisc.
		 */
		sch_tree_lock(sch);
		q->r2q = 0;
		sch_tree_unlock(sch);
		return 0;
	}

	/*
	 * If it is not NULL, check if the size of message is valid.
	 */
	if (nla_len(opt) < sizeof(*qopt)) {
		nss_ppe_qdisc_warning("Invalid message length: size %d expected >= %u\n", nla_len(opt), sizeof(*qopt));
		return -EINVAL;
	}
	qopt = nla_data(opt);

	sch_tree_lock(sch);
	q->r2q = qopt->r2q;
	sch_tree_unlock(sch);

	/*
	 * The r2q parameter is not needed in the firmware. So we do not
	 * send down a configuration message.
	 */

	return 0;
}

/*
 * nss_ppe_htb_reset_class()
 *	Resets child qdisc of class to be reset.
 */
static void nss_ppe_htb_reset_class(struct nss_ppe_htb_class_data *cl)
{
	nss_qdisc_reset(cl->qdisc);
	nss_ppe_qdisc_trace("ppehtb class %x reset\n", cl->npq.nq.qos_tag);
}

/*
 * nss_ppe_htb_reset_qdisc()
 *	Resets ppehtb qdisc and its classes.
 */
static void nss_ppe_htb_reset_qdisc(struct Qdisc *sch)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	struct nss_ppe_htb_class_data *cl;
	struct hlist_node *n __maybe_unused;
	unsigned int i;

	for (i = 0; i < q->clhash.hashsize; i++) {
		nss_qdisc_hlist_for_each_entry(cl, n, &q->clhash.hash[i], sch_common.hnode)
			nss_ppe_htb_reset_class(cl);
	}

	nss_qdisc_reset(sch);
	nss_ppe_qdisc_trace("ppe ppehtb qdisc %x reset\n", sch->handle);
}

/*
 * nss_ppe_htb_destroy_qdisc()
 *	Call to destroy a ppehtb qdisc and its associated classes.
 */
static void nss_ppe_htb_destroy_qdisc(struct Qdisc *sch)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	struct hlist_node *n __maybe_unused;
	struct hlist_node *next;
	struct nss_ppe_htb_class_data *cl;
	unsigned int i = 0;

	/*
	 * Destroy all the child classes before the parent is destroyed.
	 */
	while (q->clhash.hashelems) {
		nss_qdisc_hlist_for_each_entry_safe(cl, n, next, &q->clhash.hash[i], sch_common.hnode) {

			if (cl->children) {
				continue;
			}

			/*
			 * Reduce refcnt by 1 before destroying. This is to
			 * ensure that polling of stat stops properly.
			 */
			atomic_sub(1, &cl->npq.nq.refcnt);

			/*
			 * We are not root class. Therefore we reduce the children count
			 * for our parent.
			 */
			sch_tree_lock(sch);
			if (cl->parent) {
				cl->parent->children--;
			}
			qdisc_class_hash_remove(&q->clhash, &cl->sch_common);
			sch_tree_unlock(sch);

			/*
			 * Now we can destroy the class.
			 */
			nss_ppe_htb_destroy_class(sch, cl);
		}
		i++;

		/*
		 * In the first iteration, all the leaf nodes will be removed.
		 * Now the intermediate nodes (one above the leaf nodes) are
		 * leaf nodes. So, to delete the entire tree level wise,
		 * wrap around the index.
		 */
		if (i == q->clhash.hashsize) {
			i = 0;
		}
	}
	qdisc_class_hash_destroy(&q->clhash);

	/*
	 * Stop the polling of basic stats
	 */
	nss_qdisc_stop_basic_stats_polling(&q->npq.nq);

	/*
	 * Reset the original SSDK queue configuration
	 */
	nss_ppe_qdisc_default_conf_set(nss_ppe_qdisc_port_num_get(&q->npq));

	/*
	 * Now we can go ahead and destroy the qdisc.
	 * Note: We dont have to detach ourself from our parent because this
	 * will be taken care of by the graft call.
	 */
	nss_qdisc_destroy(&q->npq.nq);
	nss_ppe_qdisc_info("ppehtb qdisc %x destroyed\n", sch->handle);
}

/*
 * nss_ppe_htb_init_qdisc()
 *	Initializes the ppehtb qdisc.
 */
static int nss_ppe_htb_init_qdisc(struct Qdisc *sch, struct nlattr *opt)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	int err;

	nss_ppe_qdisc_trace("initializing ppehtb qdisc %x\n", sch->handle);

	/*
	 * Currently we only allow one ppehtb qdisc and
	 * that should be at the root.
	 */
	if (sch->parent != TC_H_ROOT) {
		nss_ppe_qdisc_error("ppehtb qdisc needs to be root %x\n", sch->handle);
		return -EINVAL;
	}

	err = qdisc_class_hash_init(&q->clhash);
	if (err < 0) {
		nss_ppe_qdisc_error("hash init failed for ppehtb qdisc %x\n", sch->handle);
		return err;
	}

	/*
	 * Initialize the PPEHTB shaper in NSS
	 */
	if (nss_ppe_qdisc_init(sch, &q->npq, NSS_SHAPER_NODE_TYPE_PPE_SN, NSS_SHAPER_CONFIG_PPE_SN_TYPE_HTB, 0) < 0) {
		nss_ppe_qdisc_error("failed to initialize ppehtb qdisc %x in ppe\n", sch->handle);
		return -EINVAL;
	}
	nss_ppe_qdisc_info("ppehtb qdisc initialized with handle %x\n", sch->handle);

	/*
	 * Tune ppehtb parameters
	 */
	if (nss_ppe_htb_change_qdisc(sch, opt) < 0) {
		nss_qdisc_destroy(&q->npq.nq);
		return -EINVAL;
	}

	/*
	 * Start the stats polling timer
	 */
	nss_qdisc_start_basic_stats_polling(&q->npq.nq);
	return 0;
}

/*
 * nss_ppe_htb_dump_qdisc()
 *	Dumps ppehtb qdisc's configurable parameters.
 */
static int nss_ppe_htb_dump_qdisc(struct Qdisc *sch, struct sk_buff *skb)
{
	struct nss_ppe_htb_sched_data *q = qdisc_priv(sch);
	unsigned char *b = skb_tail_pointer(skb);
	struct tc_ppehtb_qopt qopt;
	struct nlattr *nest;

	nss_ppe_qdisc_trace("dumping ppehtb qdisc %x\n", sch->handle);
	qopt.r2q = q->r2q;

	nest = nla_nest_start(skb, TCA_OPTIONS);
	if (!nest || nla_put(skb, TCA_PPEHTB_QDISC_PARMS, sizeof(qopt), &qopt)) {
		goto nla_put_failure;
	}

	nla_nest_end(skb, nest);
	return skb->len;

nla_put_failure:
	nlmsg_trim(skb, b);
	return -1;
}

/*
 * nss_ppe_htb_enqueue()
 *	Enqueues a skb to ppehtb qdisc.
 */
static int nss_ppe_htb_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	return nss_qdisc_enqueue(skb, sch);
}

/*
 * nss_ppe_htb_dequeue()
 *	Dequeues a skb from ppehtb qdisc.
 */
static struct sk_buff *nss_ppe_htb_dequeue(struct Qdisc *sch)
{
	return nss_qdisc_dequeue(sch);
}

/*
 * nss_ppe_htb_drop()
 *	Drops a single skb from linux queue, if not empty.
 *
 * Does not drop packets that are queued in the NSS.
 */
static unsigned int nss_ppe_htb_drop(struct Qdisc *sch)
{
	nss_ppe_qdisc_trace("drop called on ppehtb qdisc %x\n", sch->handle);
	return nss_qdisc_drop(sch);
}

/*
 * Registration structure for ppehtb class
 */
const struct Qdisc_class_ops nss_ppe_htb_class_ops = {
	.change		= nss_ppe_htb_change_class,
	.delete		= nss_ppe_htb_delete_class,
	.graft		= nss_ppe_htb_graft_class,
	.leaf		= nss_ppe_htb_leaf_class,
	.qlen_notify = nss_ppe_htb_qlen_notify,
	.get		= nss_ppe_htb_get_class,
	.put		= nss_ppe_htb_put_class,
	.dump		= nss_ppe_htb_dump_class,
	.dump_stats	= nss_ppe_htb_dump_class_stats,
	.walk		= nss_ppe_htb_walk
};

/*
 * Registration structure for ppehtb qdisc
 */
struct Qdisc_ops nss_ppe_htb_qdisc_ops __read_mostly = {
	.id		= "ppehtb",
	.init		= nss_ppe_htb_init_qdisc,
	.change		= nss_ppe_htb_change_qdisc,
	.reset		= nss_ppe_htb_reset_qdisc,
	.destroy	= nss_ppe_htb_destroy_qdisc,
	.dump		= nss_ppe_htb_dump_qdisc,
	.enqueue	= nss_ppe_htb_enqueue,
	.dequeue	= nss_ppe_htb_dequeue,
	.peek		= qdisc_peek_dequeued,
	.drop		= nss_ppe_htb_drop,
	.cl_ops		= &nss_ppe_htb_class_ops,
	.priv_size	= sizeof(struct nss_ppe_htb_sched_data),
	.owner		= THIS_MODULE
};
