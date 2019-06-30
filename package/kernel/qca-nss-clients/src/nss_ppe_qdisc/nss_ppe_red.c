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

/*
 * nss_ppe_red qdisc instance structure
 */
struct nss_ppe_red_sched_data {
	struct nss_ppe_qdisc npq;	/* Common base class for all ppe qdiscs */
	u8 set_default;			/* Flag to set qdisc as default qdisc for enqueue */
};

/*
 * nss_ppe_red_enqueue()
 *	Enqueues a skb to red qdisc.
 */
static int nss_ppe_red_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	return nss_qdisc_enqueue(skb, sch);
}

/*
 * nss_ppe_red_dequeue()
 *	Dequeues a skb from red qdisc.
 */
static struct sk_buff *nss_ppe_red_dequeue(struct Qdisc *sch)
{
	return nss_qdisc_dequeue(sch);
}

/*
 * nss_ppe_red_drop()
 *	Drops a single skb from linux queue, if not empty.
 *
 * Does not drop packets that are queued in the NSS.
 */
static unsigned int nss_ppe_red_drop(struct Qdisc *sch)
{
	nss_ppe_qdisc_info("ppe_red dropping  packets from qdisc:%x\n", sch->handle);
	return nss_qdisc_drop(sch);
}

/*
 * nss_ppe_red_reset()
 *	Resets red qdisc.
 */
static void nss_ppe_red_reset(struct Qdisc *sch)
{
	nss_ppe_qdisc_info("ppe_red resetting qdisc:%x\n", sch->handle);
	nss_qdisc_reset(sch);
}

/*
 * nss_ppe_red_destroy()
 *	Call to destroy a red qdisc.
 */
static void nss_ppe_red_destroy(struct Qdisc *sch)
{
	struct nss_ppe_qdisc *npq = (struct nss_ppe_qdisc *)qdisc_priv(sch);

	if (sch->parent == TC_H_ROOT) {
		nss_ppe_qdisc_scheduler_reset(npq);
		nss_ppe_qdisc_default_conf_set(nss_ppe_qdisc_port_num_get(npq));
	}

	/*
	 * Stop the polling of basic stats
	 */
	nss_qdisc_stop_basic_stats_polling(&npq->nq);
	nss_qdisc_destroy(&npq->nq);
	nss_ppe_qdisc_info("ppe_red destroyed qdisc:%x\n", sch->handle);
}

/*
 * nss_ppe_red_policy structure
 */
static const struct nla_policy nss_ppe_red_policy[TCA_PPERED_MAX + 1] = {
	[TCA_PPERED_PARMS] = { .len = sizeof(struct tc_ppered_qopt) },
};

/*
 * nss_ppe_red_change()
 *	Can be used to configure a red qdisc.
 */
static int nss_ppe_red_change(struct Qdisc *sch, struct nlattr *opt)
{
	struct nss_ppe_red_sched_data *q;
	struct nlattr *na[TCA_PPERED_MAX + 1];
	struct tc_ppered_qopt *qopt;
	int err;

	q = qdisc_priv(sch);

	if (!opt) {
		return -EINVAL;
	}

	err = nla_parse_nested(na, TCA_PPERED_MAX, opt, nss_ppe_red_policy);
	if (err < 0)
		return err;

	if (!na[TCA_PPERED_PARMS])
		return -EINVAL;

	qopt = nla_data(na[TCA_PPERED_PARMS]);

	if (qopt->limit < NSS_PPE_MEM_BLOCK_SIZE) {
		nss_qdisc_error("ppered %x limit %u too small (min 256B)",
					 sch->handle, qopt->limit);
		return -EINVAL;
	}

	if (qopt->rap.max > qopt->limit) {
		nss_qdisc_error("ppered %x limit %u smaller than max %u",
					 sch->handle, qopt->limit,
					qopt->rap.max);
		return -EINVAL;
	}

	if ((qopt->rap.min > qopt->limit) || (qopt->rap.min > qopt->rap.max)) {
		nss_qdisc_error("ppered %x min %u not valid",
					 sch->handle, qopt->rap.min);
		return -EINVAL;
	}

	/*
	 * If min/max is not specified, calculated it.
	 */
	if (!qopt->rap.max) {
		qopt->rap.max = qopt->rap.min ? qopt->rap.min * 3 : qopt->limit / 4;
		nss_qdisc_info("ppered %x max not specified, setting to %u",
					 sch->handle, qopt->rap.max);
	}
	if (!qopt->rap.min) {
		qopt->rap.min = qopt->rap.max / 3;
		nss_qdisc_info("ppered %x min not specified, setting to %u",
					 sch->handle, qopt->rap.min);
	}

	if (qopt->rap.min < NSS_PPE_MEM_BLOCK_SIZE) {
		nss_qdisc_error("ppered %x min %u too small (min 256B)",
					 sch->handle, qopt->limit);
	}

	if (qopt->rap.max < NSS_PPE_MEM_BLOCK_SIZE) {
		nss_qdisc_error("ppered %x max %u too small (min 256B)",
					 sch->handle, qopt->limit);
	}

	/*
	 * PPE operates in terms of memory blocks, and each block
	 * is NSS_PPE_MEM_BLOCK_SIZE bytes in size. Therefore we divide the
	 * input parameters which are in bytes by NSS_PPE_MEM_BLOCK_SIZE to get
	 * the number of memory blocks to assign.
	 */
	q->npq.q.red_en = true;
	q->npq.q.color_en = false;
	q->npq.q.qlimit = qopt->limit / NSS_PPE_MEM_BLOCK_SIZE;
	q->npq.q.min_th[NSS_PPE_COLOR_GREEN] = qopt->rap.min / NSS_PPE_MEM_BLOCK_SIZE;
	q->npq.q.max_th[NSS_PPE_COLOR_GREEN] = qopt->rap.max / NSS_PPE_MEM_BLOCK_SIZE;
	q->set_default = qopt->set_default;

	/*
	 * Required for basic stats display
	 */
	sch->limit = qopt->limit;

	nss_ppe_qdisc_info("qdisc:%x limit:%uB min:%uB max:%uB set_default:%u\n",
				 sch->handle, qopt->limit, qopt->rap.min,
				qopt->rap.max, qopt->set_default);

	/*
	 * Program PPE queue parameters
	 */
	nss_ppe_qdisc_queue_limit_set(&q->npq);

	/*
	 * There is nothing we need to do if the qdisc is not
	 * set as default qdisc.
	 */
	if (q->set_default == 0) {
		return 0;
	}

	/*
	 * Set this qdisc to be the default qdisc for enqueuing packets.
	 */
	if (nss_qdisc_set_default(&q->npq.nq) < 0) {
		nss_ppe_qdisc_warning("ppe_red %p set_default failed\n", sch);
		return -EINVAL;
	}

	nss_ppe_qdisc_info("ppe_red %p queue (qos_tag:%u) set as default\n", sch, q->npq.nq.qos_tag);
	return 0;
}

/*
 * nss_ppe_red_init()
 *	Initializes the red qdisc.
 */
static int nss_ppe_red_init(struct Qdisc *sch, struct nlattr *opt)
{
	struct nss_ppe_qdisc *npq = qdisc_priv(sch);

	if (!opt)
		return -EINVAL;

	/*
	 * First delete all packets pending in the output queue and reset stats
	 */
	nss_ppe_red_reset(sch);

	if (nss_ppe_qdisc_init(sch, npq, NSS_SHAPER_NODE_TYPE_PPE_SN, NSS_SHAPER_CONFIG_PPE_SN_TYPE_RED, 0) < 0) {
		nss_ppe_qdisc_warning("ppe_red %p init failed\n", sch);
		return -EINVAL;
	}
	nss_ppe_qdisc_info("PPE red initialized - handle %x parent %x\n", sch->handle, sch->parent);

	/*
	 * Set the PPE configuration
	 * For root qdisc, first SSDK schedulers and NSS configuration is done
	 * and then limit is set while for non-root qdisc, the SSDK scheduler and NSS
	 * configuration is done in corresponding graft class.
	 */
	if (sch->parent == TC_H_ROOT) {
		if (nss_ppe_qdisc_configure(npq) < 0) {
			nss_ppe_qdisc_warning("ppe_red %p configuration failed\n", sch);
			nss_qdisc_destroy(&npq->nq);
			return -EINVAL;
		}
	}

	if (nss_ppe_red_change(sch, opt) < 0) {
		nss_ppe_qdisc_warning("ppe_red %p change failed\n", sch);
		nss_qdisc_destroy(&npq->nq);
		if (sch->parent == TC_H_ROOT) {
			nss_ppe_qdisc_scheduler_reset(npq);
		}
		return -EINVAL;
	}

	/*
	 * Start the stats polling timer
	 */
	nss_qdisc_start_basic_stats_polling(&npq->nq);
	return 0;
}

/*
 * nss_ppe_red_dump()
 *	Dumps red qdisc's configurable parameters.
 */
static int nss_ppe_red_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct nss_ppe_red_sched_data *q;
	struct nlattr *opts = NULL;
	struct tc_ppered_qopt opt;

	nss_ppe_qdisc_info("ppered Dumping qdisc:%x\n", sch->handle);

	q = qdisc_priv(sch);
	if (!q) {
		return -1;
	}

	/*
	 * Multiply the number of blocks by 256 since each block
	 * is 256B in size.
	 */
	opt.limit = q->npq.q.qlimit * NSS_PPE_MEM_BLOCK_SIZE;
	opt.rap.min = q->npq.q.min_th[NSS_PPE_COLOR_GREEN] * NSS_PPE_MEM_BLOCK_SIZE;
	opt.rap.max = q->npq.q.max_th[NSS_PPE_COLOR_GREEN] * NSS_PPE_MEM_BLOCK_SIZE;
	opt.set_default = q->set_default;

	opts = nla_nest_start(skb, TCA_OPTIONS);
	if (!opts || nla_put(skb, TCA_PPERED_PARMS, sizeof(opt), &opt)) {
		goto nla_put_failure;
	}

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -EMSGSIZE;
}

/*
 * nss_ppe_red_peek()
 *	Peeks red qdisc.
 */
static struct sk_buff *nss_ppe_red_peek(struct Qdisc *sch)
{
	nss_ppe_qdisc_info("ppered Peeking");
	return nss_qdisc_peek(sch);
}

/*
 * Registration structure for red qdisc
 */
struct Qdisc_ops nss_ppe_red_qdisc_ops __read_mostly = {
	.id		=	"ppered",
	.priv_size	=	sizeof(struct nss_ppe_red_sched_data),
	.enqueue	=	nss_ppe_red_enqueue,
	.dequeue	=	nss_ppe_red_dequeue,
	.peek		=	nss_ppe_red_peek,
	.drop		=	nss_ppe_red_drop,
	.init		=	nss_ppe_red_init,
	.reset		=	nss_ppe_red_reset,
	.destroy	=	nss_ppe_red_destroy,
	.change		=	nss_ppe_red_change,
	.dump		=	nss_ppe_red_dump,
	.owner		=	THIS_MODULE,
};
