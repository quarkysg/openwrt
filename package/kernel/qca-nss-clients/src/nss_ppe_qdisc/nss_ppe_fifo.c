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
#include "nss_ppe_fifo.h"

/*
 * PPE HW has memory in blocks of 256bytes.
 */
#define NSS_PPE_FIFO_BLOCK_SIZE	256

/*
 * nss_ppe_fifo qdisc instance structure
 */
struct nss_ppe_fifo_sched_data {
	struct nss_ppe_qdisc npq;	/* Common base class for all ppe qdiscs */
	u32 limit;			/* Queue length in packets */
	u8 set_default;			/* Flag to set qdisc as default qdisc for enqueue */
};

/*
 * nss_ppe_fifo_enqueue()
 *	Enqueues a skb to fifo qdisc.
 */
static int nss_ppe_fifo_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	return nss_qdisc_enqueue(skb, sch);
}

/*
 * nss_ppe_fifo_dequeue()
 *	Dequeues a skb from fifo qdisc.
 */
static struct sk_buff *nss_ppe_fifo_dequeue(struct Qdisc *sch)
{
	return nss_qdisc_dequeue(sch);
}

/*
 * nss_ppe_fifo_drop()
 *	Drops a single skb from linux queue, if not empty.
 *
 * Does not drop packets that are queued in the NSS.
 */
static unsigned int nss_ppe_fifo_drop(struct Qdisc *sch)
{
	nss_ppe_qdisc_info("ppe_fifo dropping packets qdisc:%x\n", sch->handle);
	return nss_qdisc_drop(sch);
}

/*
 * nss_ppe_fifo_reset()
 *	Resets fifo qdisc.
 */
static void nss_ppe_fifo_reset(struct Qdisc *sch)
{
	nss_ppe_qdisc_info("ppe_fifo resetting qdisc:%x\n", sch->handle);
	nss_qdisc_reset(sch);
}

/*
 * nss_ppe_fifo_destroy()
 *	Call to destroy a fifo qdisc.
 */
static void nss_ppe_fifo_destroy(struct Qdisc *sch)
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
	nss_ppe_qdisc_info("ppe_fifo destroyed qdisc:%x\n", sch->handle);
}

/*
 * nss_ppe_fifo_policy structure
 */
static const struct nla_policy nss_ppe_fifo_policy[TCA_PPEFIFO_MAX + 1] = {
	[TCA_PPEFIFO_PARMS] = { .len = sizeof(struct tc_ppefifo_qopt) },
};

/*
 * nss_ppe_fifo_change()
 *	Can be used to configure a fifo qdisc.
 */
static int nss_ppe_fifo_change(struct Qdisc *sch, struct nlattr *opt)
{
	struct nss_ppe_fifo_sched_data *q;
	struct nlattr *na[TCA_PPEFIFO_MAX + 1];
	struct tc_ppefifo_qopt *qopt;
	bool is_bfifo = sch->ops == &nss_ppe_bfifo_qdisc_ops;
	int err;

	q = qdisc_priv(sch);

	if (!opt) {
		return -EINVAL;
	}

	err = nla_parse_nested(na, TCA_PPEFIFO_MAX, opt, nss_ppe_fifo_policy);
	if (err < 0)
		return err;

	if (!na[TCA_PPEFIFO_PARMS])
		return -EINVAL;

	qopt = nla_data(na[TCA_PPEFIFO_PARMS]);

	if (!qopt->limit) {
		qopt->limit = qdisc_dev(sch)->tx_queue_len ? : 1;

		if (is_bfifo) {
			qopt->limit *= psched_mtu(qdisc_dev(sch));
		}
	} else {
		q->limit = qopt->limit;
	}

	/*
	 * In case of bfifo, change the queue limit to number of blocks
	 * as PPE HW has memory in blocks of 256 bytes.
	 */
	if (is_bfifo) {
		q->limit = qopt->limit / NSS_PPE_FIFO_BLOCK_SIZE;
	}

	/*
	 * Required for basic stats display
	 */
	sch->limit = qopt->limit;

	q->set_default = qopt->set_default;
	nss_ppe_qdisc_info("qdisc:%x limit:%u set_default:%u\n", sch->handle, qopt->limit, qopt->set_default);

	q->npq.q.qlimit = q->limit;
	q->npq.q.color_en = false;
	q->npq.q.red_en = false;

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
		nss_ppe_qdisc_warning("ppe_fifo %p set_default failed\n", sch);
		return -EINVAL;
	}

	nss_ppe_qdisc_info("ppe_fifo %p queue (qos_tag:%u) set as default\n", sch, q->npq.nq.qos_tag);
	return 0;
}

/*
 * nss_ppe_fifo_init()
 *	Initializes the fifo qdisc.
 */
static int nss_ppe_fifo_init(struct Qdisc *sch, struct nlattr *opt)
{
	struct nss_ppe_qdisc *npq = qdisc_priv(sch);

	if (!opt)
		return -EINVAL;

	/*
	 * First delete all packets pending in the output queue and reset stats
	 */
	nss_ppe_fifo_reset(sch);

	if (nss_ppe_qdisc_init(sch, npq, NSS_SHAPER_NODE_TYPE_PPE_SN, NSS_SHAPER_CONFIG_PPE_SN_TYPE_FIFO, 0) < 0) {
		nss_ppe_qdisc_warning("ppe_fifo %p init failed\n", sch);
		return -EINVAL;
	}
	nss_ppe_qdisc_info("PPE fifo initialized - handle %x parent %x\n", sch->handle, sch->parent);

	/*
	 * Set the PPE configuration
	 * For root qdisc, first SSDK schedulers and NSS configuration is done
	 * and then limit is set while for non-root qdisc, the SSDK scheduler and NSS
	 * configuration is done in corresponding graft class.
	 */
	if (sch->parent == TC_H_ROOT) {
		if (nss_ppe_qdisc_configure(npq) < 0) {
			nss_ppe_qdisc_warning("ppe_fifo %p configuration failed\n", sch);
			nss_qdisc_destroy(&npq->nq);
			return -EINVAL;
		}
	}

	if (nss_ppe_fifo_change(sch, opt) < 0) {
		nss_ppe_qdisc_warning("ppe_fifo %p change failed\n", sch);
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
 * nss_ppe_fifo_dump()
 *	Dumps fifo qdisc's configurable parameters.
 */
static int nss_ppe_fifo_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct nss_ppe_fifo_sched_data *q;
	struct nlattr *opts = NULL;
	struct tc_ppefifo_qopt opt;

	nss_ppe_qdisc_info("Ppefifo Dumping qdisc:%x\n", sch->handle);

	q = qdisc_priv(sch);
	if (!q) {
		return -1;
	}

	opt.limit = sch->limit;
	opt.set_default = q->set_default;

	opts = nla_nest_start(skb, TCA_OPTIONS);
	if (!opts) {
		goto nla_put_failure;
	}
	if (nla_put(skb, TCA_PPEFIFO_PARMS, sizeof(opt), &opt))
		goto nla_put_failure;

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -EMSGSIZE;
}

/*
 * nss_ppe_fifo_peek()
 *	Peeks fifo qdisc.
 */
static struct sk_buff *nss_ppe_fifo_peek(struct Qdisc *sch)
{
	nss_ppe_qdisc_info("Ppefifo Peeking qdisc:%x\n", sch->handle);
	return nss_qdisc_peek(sch);
}

/*
 * Registration structure for pfifo qdisc
 */
struct Qdisc_ops nss_ppe_pfifo_qdisc_ops __read_mostly = {
	.id		=	"ppepfifo",
	.priv_size	=	sizeof(struct nss_ppe_fifo_sched_data),
	.enqueue	=	nss_ppe_fifo_enqueue,
	.dequeue	=	nss_ppe_fifo_dequeue,
	.peek		=	nss_ppe_fifo_peek,
	.drop		=	nss_ppe_fifo_drop,
	.init		=	nss_ppe_fifo_init,
	.reset		=	nss_ppe_fifo_reset,
	.destroy	=	nss_ppe_fifo_destroy,
	.change		=	nss_ppe_fifo_change,
	.dump		=	nss_ppe_fifo_dump,
	.owner		=	THIS_MODULE,
};

/*
 * Registration structure for bfifo qdisc
 */
struct Qdisc_ops nss_ppe_bfifo_qdisc_ops __read_mostly = {
	.id		=	"ppebfifo",
	.priv_size	=	sizeof(struct nss_ppe_fifo_sched_data),
	.enqueue	=	nss_ppe_fifo_enqueue,
	.dequeue	=	nss_ppe_fifo_dequeue,
	.peek		=	nss_ppe_fifo_peek,
	.drop		=	nss_ppe_fifo_drop,
	.init		=	nss_ppe_fifo_init,
	.reset		=	nss_ppe_fifo_reset,
	.destroy	=	nss_ppe_fifo_destroy,
	.change		=	nss_ppe_fifo_change,
	.dump		=	nss_ppe_fifo_dump,
	.owner		=	THIS_MODULE,
};
