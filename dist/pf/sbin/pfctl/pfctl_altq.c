/*	$NetBSD: pfctl_altq.c,v 1.10 2018/02/04 08:44:36 mrg Exp $	*/
/*	$OpenBSD: pfctl_altq.c,v 1.92 2007/05/27 05:15:17 claudio Exp $	*/

/*
 * Copyright (c) 2002
 *	Sony Computer Science Laboratories Inc.
 * Copyright (c) 2002, 2003 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef __NetBSD__
#include <sys/param.h>
#include <sys/mbuf.h>
#endif

#include <net/if.h>
#include <netinet/in.h>
#include <net/pfvar.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <altq/altq.h>
#include <altq/altq_cbq.h>
#include <altq/altq_priq.h>
#include <altq/altq_hfsc.h>

#include "pfctl_parser.h"
#include "pfctl.h"

#define is_sc_null(sc)	(((sc) == NULL) || ((sc)->m1 == 0 && (sc)->m2 == 0))

TAILQ_HEAD(altqs, pf_altq) altqs = TAILQ_HEAD_INITIALIZER(altqs);
LIST_HEAD(gen_sc, segment) rtsc, lssc;

struct pf_altq	*qname_to_pfaltq(const char *, const char *);
u_int32_t	 qname_to_qid(const char *);

static int	eval_pfqueue_cbq(struct pfctl *, struct pf_altq *);
static int	cbq_compute_idletime(struct pfctl *, struct pf_altq *);
static int	check_commit_cbq(int, int, struct pf_altq *);
static int	print_cbq_opts(const struct pf_altq *);

static int	eval_pfqueue_priq(struct pfctl *, struct pf_altq *);
static int	check_commit_priq(int, int, struct pf_altq *);
static int	print_priq_opts(const struct pf_altq *);

static int	eval_pfqueue_hfsc(struct pfctl *, struct pf_altq *);
static int	check_commit_hfsc(int, int, struct pf_altq *);
static int	print_hfsc_opts(const struct pf_altq *,
		    const struct node_queue_opt *);

static void		 gsc_add_sc(struct gen_sc *, struct service_curve *);
static int		 is_gsc_under_sc(struct gen_sc *,
			     struct service_curve *);
static void		 gsc_destroy(struct gen_sc *);
static struct segment	*gsc_getentry(struct gen_sc *, double);
static int		 gsc_add_seg(struct gen_sc *, double, double, double,
			     double);
static double		 sc_x2y(struct service_curve *, double);

u_int32_t	 getifspeed(char *);
u_long		 getifmtu(char *);
int		 eval_queue_opts(struct pf_altq *, struct node_queue_opt *,
		     u_int32_t);
u_int32_t	 eval_bwspec(struct node_queue_bw *, u_int32_t);
void		 print_hfsc_sc(const char *, u_int, u_int, u_int,
		     const struct node_hfsc_sc *);

void
pfaltq_store(struct pf_altq *a)
{
	struct pf_altq	*altq;

	if ((altq = malloc(sizeof(*altq))) == NULL)
		err(1, "malloc");
	memcpy(altq, a, sizeof(struct pf_altq));
	TAILQ_INSERT_TAIL(&altqs, altq, entries);
}

struct pf_altq *
pfaltq_lookup(const char *ifname)
{
	struct pf_altq	*altq;

	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(ifname, altq->ifname, IFNAMSIZ) == 0 &&
		    altq->qname[0] == 0)
			return (altq);
	}
	return (NULL);
}

struct pf_altq *
qname_to_pfaltq(const char *qname, const char *ifname)
{
	struct pf_altq	*altq;

	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(ifname, altq->ifname, IFNAMSIZ) == 0 &&
		    strncmp(qname, altq->qname, PF_QNAME_SIZE) == 0)
			return (altq);
	}
	return (NULL);
}

u_int32_t
qname_to_qid(const char *qname)
{
	struct pf_altq	*altq;

	/*
	 * We guarantee that same named queues on different interfaces
	 * have the same qid, so we do NOT need to limit matching on
	 * one interface!
	 */

	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(qname, altq->qname, PF_QNAME_SIZE) == 0)
			return (altq->qid);
	}
	return (0);
}

void
print_altq(const struct pf_altq *a, unsigned level, struct node_queue_bw *bw,
	struct node_queue_opt *qopts)
{
	if (a->qname[0] != 0) {
		print_queue(a, level, bw, 1, qopts);
		return;
	}

	printf("altq on %s ", a->ifname);

	switch (a->scheduler) {
	case ALTQT_CBQ:
		if (!print_cbq_opts(a))
			printf("cbq ");
		break;
	case ALTQT_PRIQ:
		if (!print_priq_opts(a))
			printf("priq ");
		break;
	case ALTQT_HFSC:
		if (!print_hfsc_opts(a, qopts))
			printf("hfsc ");
		break;
	}

	if (bw != NULL && bw->bw_percent > 0) {
		if (bw->bw_percent < 100)
			printf("bandwidth %u%% ", bw->bw_percent);
	} else
		printf("bandwidth %s ", rate2str((double)a->ifbandwidth));

	if (a->qlimit != DEFAULT_QLIMIT)
		printf("qlimit %u ", a->qlimit);
	printf("tbrsize %u ", a->tbrsize);
}

void
print_queue(const struct pf_altq *a, unsigned level, struct node_queue_bw *bw,
    int print_interface, struct node_queue_opt *qopts)
{
	unsigned	i;

	printf("queue ");
	for (i = 0; i < level; ++i)
		printf(" ");
	printf("%s ", a->qname);
	if (print_interface)
		printf("on %s ", a->ifname);
	if (a->scheduler == ALTQT_CBQ || a->scheduler == ALTQT_HFSC) {
		if (bw != NULL && bw->bw_percent > 0) {
			if (bw->bw_percent < 100)
				printf("bandwidth %u%% ", bw->bw_percent);
		} else
			printf("bandwidth %s ", rate2str((double)a->bandwidth));
	}
	if (a->priority != DEFAULT_PRIORITY)
		printf("priority %u ", a->priority);
	if (a->qlimit != DEFAULT_QLIMIT)
		printf("qlimit %u ", a->qlimit);
	switch (a->scheduler) {
	case ALTQT_CBQ:
		print_cbq_opts(a);
		break;
	case ALTQT_PRIQ:
		print_priq_opts(a);
		break;
	case ALTQT_HFSC:
		print_hfsc_opts(a, qopts);
		break;
	}
}

/*
 * eval_pfaltq computes the discipline parameters.
 */
int
eval_pfaltq(struct pfctl *pf, struct pf_altq *pa, struct node_queue_bw *bw,
    struct node_queue_opt *opts)
{
	u_int	rate, size, errors = 0;

	if (bw->bw_absolute > 0)
		pa->ifbandwidth = bw->bw_absolute;
	else
		if ((rate = getifspeed(pa->ifname)) == 0) {
			fprintf(stderr, "interface %s does not know its bandwidth, "
			    "please specify an absolute bandwidth\n",
			    pa->ifname);
			errors++;
		} else if ((pa->ifbandwidth = eval_bwspec(bw, rate)) == 0)
			pa->ifbandwidth = rate;

	errors += eval_queue_opts(pa, opts, pa->ifbandwidth);

	/* if tbrsize is not specified, use heuristics */
	if (pa->tbrsize == 0) {
		rate = pa->ifbandwidth;
		if (rate <= 1 * 1000 * 1000)
			size = 1;
		else if (rate <= 10 * 1000 * 1000)
			size = 4;
		else if (rate <= 200 * 1000 * 1000)
			size = 8;
		else
			size = 24;
		size = size * getifmtu(pa->ifname);
		if (size > 0xffff)
			size = 0xffff;
		pa->tbrsize = size;
	}
	return (errors);
}

/*
 * check_commit_altq does consistency check for each interface
 */
int
check_commit_altq(int dev, int opts)
{
	struct pf_altq	*altq;
	int		 error = 0;

	/* call the discipline check for each interface. */
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (altq->qname[0] == 0) {
			switch (altq->scheduler) {
			case ALTQT_CBQ:
				error = check_commit_cbq(dev, opts, altq);
				break;
			case ALTQT_PRIQ:
				error = check_commit_priq(dev, opts, altq);
				break;
			case ALTQT_HFSC:
				error = check_commit_hfsc(dev, opts, altq);
				break;
			default:
				break;
			}
		}
	}
	return (error);
}

/*
 * eval_pfqueue computes the queue parameters.
 */
int
eval_pfqueue(struct pfctl *pf, struct pf_altq *pa, struct node_queue_bw *bw,
    struct node_queue_opt *opts)
{
	/* should be merged with expand_queue */
	struct pf_altq	*if_pa, *parent, *altq;
	u_int32_t	 bwsum;
	int		 error = 0;

	/* find the corresponding interface and copy fields used by queues */
	if ((if_pa = pfaltq_lookup(pa->ifname)) == NULL) {
		fprintf(stderr, "altq not defined on %s\n", pa->ifname);
		return (1);
	}
	pa->scheduler = if_pa->scheduler;
	pa->ifbandwidth = if_pa->ifbandwidth;

	if (qname_to_pfaltq(pa->qname, pa->ifname) != NULL) {
		fprintf(stderr, "queue %s already exists on interface %s\n",
		    pa->qname, pa->ifname);
		return (1);
	}
	pa->qid = qname_to_qid(pa->qname);

	parent = NULL;
	if (pa->parent[0] != 0) {
		parent = qname_to_pfaltq(pa->parent, pa->ifname);
		if (parent == NULL) {
			fprintf(stderr, "parent %s not found for %s\n",
			    pa->parent, pa->qname);
			return (1);
		}
		pa->parent_qid = parent->qid;
	}
	if (pa->qlimit == 0)
		pa->qlimit = DEFAULT_QLIMIT;

	if (pa->scheduler == ALTQT_CBQ || pa->scheduler == ALTQT_HFSC) {
		pa->bandwidth = eval_bwspec(bw,
		    parent == NULL ? 0 : parent->bandwidth);

		if (pa->bandwidth > pa->ifbandwidth) {
			fprintf(stderr, "bandwidth for %s higher than "
			    "interface\n", pa->qname);
			return (1);
		}
		/* check the sum of the child bandwidth is under parent's */
		if (parent != NULL) {
			if (pa->bandwidth > parent->bandwidth) {
				warnx("bandwidth for %s higher than parent",
				    pa->qname);
				return (1);
			}
			bwsum = 0;
			TAILQ_FOREACH(altq, &altqs, entries) {
				if (strncmp(altq->ifname, pa->ifname,
				    IFNAMSIZ) == 0 &&
				    altq->qname[0] != 0 &&
				    strncmp(altq->parent, pa->parent,
				    PF_QNAME_SIZE) == 0)
					bwsum += altq->bandwidth;
			}
			bwsum += pa->bandwidth;
			if (bwsum > parent->bandwidth) {
				warnx("the sum of the child bandwidth higher"
				    " than parent \"%s\"", parent->qname);
			}
		}
	}

	if (eval_queue_opts(pa, opts, parent == NULL? 0 : parent->bandwidth))
		return (1);

	switch (pa->scheduler) {
	case ALTQT_CBQ:
		error = eval_pfqueue_cbq(pf, pa);
		break;
	case ALTQT_PRIQ:
		error = eval_pfqueue_priq(pf, pa);
		break;
	case ALTQT_HFSC:
		error = eval_pfqueue_hfsc(pf, pa);
		break;
	default:
		break;
	}
	return (error);
}

/*
 * CBQ support functions
 */
#define	RM_FILTER_GAIN	5	/* log2 of gain, e.g., 5 => 31/32 */
#define	RM_NS_PER_SEC	(1000000000)

static int
eval_pfqueue_cbq(struct pfctl *pf, struct pf_altq *pa)
{
	struct cbq_opts	*opts;
	u_int		 ifmtu;

	if (pa->priority >= CBQ_MAXPRI) {
		warnx("priority out of range: max %d", CBQ_MAXPRI - 1);
		return (-1);
	}

	ifmtu = getifmtu(pa->ifname);
	opts = &pa->pq_u.cbq_opts;

	if (opts->pktsize == 0) {	/* use default */
		opts->pktsize = ifmtu;
		if (opts->pktsize > MCLBYTES)	/* do what TCP does */
			opts->pktsize &= ~MCLBYTES;
	} else if (opts->pktsize > ifmtu)
		opts->pktsize = ifmtu;
	if (opts->maxpktsize == 0)	/* use default */
		opts->maxpktsize = ifmtu;
	else if (opts->maxpktsize > ifmtu)
		opts->pktsize = ifmtu;

	if (opts->pktsize > opts->maxpktsize)
		opts->pktsize = opts->maxpktsize;

	if (pa->parent[0] == 0)
		opts->flags |= (CBQCLF_ROOTCLASS | CBQCLF_WRR);

	cbq_compute_idletime(pf, pa);
	return (0);
}

/*
 * compute ns_per_byte, maxidle, minidle, and offtime
 */
static int
cbq_compute_idletime(struct pfctl *pf, struct pf_altq *pa)
{
	struct cbq_opts	*opts;
	double		 maxidle_s, maxidle, minidle;
	double		 offtime, nsPerByte, ifnsPerByte, ptime, cptime;
	double		 z, g, f, gton, gtom;
	u_int		 minburst, maxburst;

	opts = &pa->pq_u.cbq_opts;
	ifnsPerByte = (1.0 / (double)pa->ifbandwidth) * RM_NS_PER_SEC * 8;
	minburst = opts->minburst;
	maxburst = opts->maxburst;

	if (pa->bandwidth == 0)
		f = 0.0001;	/* small enough? */
	else
		f = ((double) pa->bandwidth / (double) pa->ifbandwidth);

	nsPerByte = ifnsPerByte / f;
	ptime = (double)opts->pktsize * ifnsPerByte;
	cptime = ptime * (1.0 - f) / f;

	if (nsPerByte * (double)opts->maxpktsize > (double)INT_MAX) {
		/*
		 * this causes integer overflow in kernel!
		 * (bandwidth < 6Kbps when max_pkt_size=1500)
		 */
		if (pa->bandwidth != 0 && (pf->opts & PF_OPT_QUIET) == 0) {
			warnx("queue bandwidth must be larger than %s",
			    rate2str(ifnsPerByte * (double)opts->maxpktsize /
			    (double)INT_MAX * (double)pa->ifbandwidth));
			fprintf(stderr, "cbq: queue %s is too slow!\n",
			    pa->qname);
		}
		nsPerByte = (double)(INT_MAX / opts->maxpktsize);
	}

	if (maxburst == 0) {  /* use default */
		if (cptime > 10.0 * 1000000)
			maxburst = 4;
		else
			maxburst = 16;
	}
	if (minburst == 0)  /* use default */
		minburst = 2;
	if (minburst > maxburst)
		minburst = maxburst;

	z = (double)(1 << RM_FILTER_GAIN);
	g = (1.0 - 1.0 / z);
	gton = pow(g, (double)maxburst);
	gtom = pow(g, (double)(minburst-1));
	maxidle = ((1.0 / f - 1.0) * ((1.0 - gton) / gton));
	maxidle_s = (1.0 - g);
	if (maxidle > maxidle_s)
		maxidle = ptime * maxidle;
	else
		maxidle = ptime * maxidle_s;
	offtime = cptime * (1.0 + 1.0/(1.0 - g) * (1.0 - gtom) / gtom);
	minidle = -((double)opts->maxpktsize * (double)nsPerByte);

	/* scale parameters */
	maxidle = ((maxidle * 8.0) / nsPerByte) *
	    pow(2.0, (double)RM_FILTER_GAIN);
	offtime = (offtime * 8.0) / nsPerByte *
	    pow(2.0, (double)RM_FILTER_GAIN);
	minidle = ((minidle * 8.0) / nsPerByte) *
	    pow(2.0, (double)RM_FILTER_GAIN);

	maxidle = maxidle / 1000.0;
	offtime = offtime / 1000.0;
	minidle = minidle / 1000.0;

	opts->minburst = minburst;
	opts->maxburst = maxburst;
	opts->ns_per_byte = (u_int)nsPerByte;
	opts->maxidle = (u_int)fabs(maxidle);
	opts->minidle = (int)minidle;
	opts->offtime = (u_int)fabs(offtime);

	return (0);
}

static int
check_commit_cbq(int dev, int opts, struct pf_altq *pa)
{
	struct pf_altq	*altq;
	int		 root_class, default_class;
	int		 error = 0;

	/*
	 * check if cbq has one root queue and one default queue
	 * for this interface
	 */
	root_class = default_class = 0;
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(altq->ifname, pa->ifname, IFNAMSIZ) != 0)
			continue;
		if (altq->qname[0] == 0)  /* this is for interface */
			continue;
		if (altq->pq_u.cbq_opts.flags & CBQCLF_ROOTCLASS)
			root_class++;
		if (altq->pq_u.cbq_opts.flags & CBQCLF_DEFCLASS)
			default_class++;
	}
	if (root_class != 1) {
		warnx("should have one root queue on %s", pa->ifname);
		error++;
	}
	if (default_class != 1) {
		warnx("should have one default queue on %s", pa->ifname);
		error++;
	}
	return (error);
}

static int
print_cbq_opts(const struct pf_altq *a)
{
	const struct cbq_opts	*opts;

	opts = &a->pq_u.cbq_opts;
	if (opts->flags) {
		printf("cbq(");
		if (opts->flags & CBQCLF_RED)
			printf(" red");
		if (opts->flags & CBQCLF_ECN)
			printf(" ecn");
		if (opts->flags & CBQCLF_RIO)
			printf(" rio");
		if (opts->flags & CBQCLF_CLEARDSCP)
			printf(" cleardscp");
		if (opts->flags & CBQCLF_FLOWVALVE)
			printf(" flowvalve");
#ifdef CBQCLF_BORROW
		if (opts->flags & CBQCLF_BORROW)
			printf(" borrow");
#endif
		if (opts->flags & CBQCLF_WRR)
			printf(" wrr");
		if (opts->flags & CBQCLF_EFFICIENT)
			printf(" efficient");
		if (opts->flags & CBQCLF_ROOTCLASS)
			printf(" root");
		if (opts->flags & CBQCLF_DEFCLASS)
			printf(" default");
		printf(" ) ");

		return (1);
	} else
		return (0);
}

/*
 * PRIQ support functions
 */
static int
eval_pfqueue_priq(struct pfctl *pf, struct pf_altq *pa)
{
	struct pf_altq	*altq;

	if (pa->priority >= PRIQ_MAXPRI) {
		warnx("priority out of range: max %d", PRIQ_MAXPRI - 1);
		return (-1);
	}
	/* the priority should be unique for the interface */
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(altq->ifname, pa->ifname, IFNAMSIZ) == 0 &&
		    altq->qname[0] != 0 && altq->priority == pa->priority) {
			warnx("%s and %s have the same priority",
			    altq->qname, pa->qname);
			return (-1);
		}
	}

	return (0);
}

static int
check_commit_priq(int dev, int opts, struct pf_altq *pa)
{
	struct pf_altq	*altq;
	int		 default_class;
	int		 error = 0;

	/*
	 * check if priq has one default class for this interface
	 */
	default_class = 0;
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(altq->ifname, pa->ifname, IFNAMSIZ) != 0)
			continue;
		if (altq->qname[0] == 0)  /* this is for interface */
			continue;
		if (altq->pq_u.priq_opts.flags & PRCF_DEFAULTCLASS)
			default_class++;
	}
	if (default_class != 1) {
		warnx("should have one default queue on %s", pa->ifname);
		error++;
	}
	return (error);
}

static int
print_priq_opts(const struct pf_altq *a)
{
	const struct priq_opts	*opts;

	opts = &a->pq_u.priq_opts;

	if (opts->flags) {
		printf("priq(");
		if (opts->flags & PRCF_RED)
			printf(" red");
		if (opts->flags & PRCF_ECN)
			printf(" ecn");
		if (opts->flags & PRCF_RIO)
			printf(" rio");
		if (opts->flags & PRCF_CLEARDSCP)
			printf(" cleardscp");
		if (opts->flags & PRCF_DEFAULTCLASS)
			printf(" default");
		printf(" ) ");

		return (1);
	} else
		return (0);
}

/*
 * HFSC support functions
 */
static int
eval_pfqueue_hfsc(struct pfctl *pf, struct pf_altq *pa)
{
	struct pf_altq		*altq, *parent;
	struct hfsc_opts	*opts;
	struct service_curve	 sc;

	opts = &pa->pq_u.hfsc_opts;

	if (pa->parent[0] == 0) {
		/* root queue */
		opts->lssc_m1 = pa->ifbandwidth;
		opts->lssc_m2 = pa->ifbandwidth;
		opts->lssc_d = 0;
		return (0);
	}

	LIST_INIT(&rtsc);
	LIST_INIT(&lssc);

	/* if link_share is not specified, use bandwidth */
	if (opts->lssc_m2 == 0)
		opts->lssc_m2 = pa->bandwidth;

	if ((opts->rtsc_m1 > 0 && opts->rtsc_m2 == 0) ||
	    (opts->lssc_m1 > 0 && opts->lssc_m2 == 0) ||
	    (opts->ulsc_m1 > 0 && opts->ulsc_m2 == 0)) {
		warnx("m2 is zero for %s", pa->qname);
		return (-1);
	}

	if ((opts->rtsc_m1 < opts->rtsc_m2 && opts->rtsc_m1 != 0) ||
	    (opts->lssc_m1 < opts->lssc_m2 && opts->lssc_m1 != 0) ||
	    (opts->ulsc_m1 < opts->ulsc_m2 && opts->ulsc_m1 != 0)) {
		warnx("m1 must be zero for convex curve: %s", pa->qname);
		return (-1);
	}

	/*
	 * admission control:
	 * for the real-time service curve, the sum of the service curves
	 * should not exceed 80% of the interface bandwidth.  20% is reserved
	 * not to over-commit the actual interface bandwidth.
	 * for the linkshare service curve, the sum of the child service
	 * curve should not exceed the parent service curve.
	 * for the upper-limit service curve, the assigned bandwidth should
	 * be smaller than the interface bandwidth, and the upper-limit should
	 * be larger than the real-time service curve when both are defined.
	 */
	parent = qname_to_pfaltq(pa->parent, pa->ifname);
	if (parent == NULL)
		errx(1, "parent %s not found for %s", pa->parent, pa->qname);

	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(altq->ifname, pa->ifname, IFNAMSIZ) != 0)
			continue;
		if (altq->qname[0] == 0)  /* this is for interface */
			continue;

		/* if the class has a real-time service curve, add it. */
		if (opts->rtsc_m2 != 0 && altq->pq_u.hfsc_opts.rtsc_m2 != 0) {
			sc.m1 = altq->pq_u.hfsc_opts.rtsc_m1;
			sc.d = altq->pq_u.hfsc_opts.rtsc_d;
			sc.m2 = altq->pq_u.hfsc_opts.rtsc_m2;
			gsc_add_sc(&rtsc, &sc);
		}

		if (strncmp(altq->parent, pa->parent, PF_QNAME_SIZE) != 0)
			continue;

		/* if the class has a linkshare service curve, add it. */
		if (opts->lssc_m2 != 0 && altq->pq_u.hfsc_opts.lssc_m2 != 0) {
			sc.m1 = altq->pq_u.hfsc_opts.lssc_m1;
			sc.d = altq->pq_u.hfsc_opts.lssc_d;
			sc.m2 = altq->pq_u.hfsc_opts.lssc_m2;
			gsc_add_sc(&lssc, &sc);
		}
	}

	/* check the real-time service curve.  reserve 20% of interface bw */
	if (opts->rtsc_m2 != 0) {
		/* add this queue to the sum */
		sc.m1 = opts->rtsc_m1;
		sc.d = opts->rtsc_d;
		sc.m2 = opts->rtsc_m2;
		gsc_add_sc(&rtsc, &sc);
		/* compare the sum with 80% of the interface */
		sc.m1 = 0;
		sc.d = 0;
		sc.m2 = pa->ifbandwidth / 100 * 80;
		if (!is_gsc_under_sc(&rtsc, &sc)) {
			warnx("real-time sc exceeds 80%% of the interface "
			    "bandwidth (%s)", rate2str((double)sc.m2));
			goto err_ret;
		}
	}

	/* check the linkshare service curve. */
	if (opts->lssc_m2 != 0) {
		/* add this queue to the child sum */
		sc.m1 = opts->lssc_m1;
		sc.d = opts->lssc_d;
		sc.m2 = opts->lssc_m2;
		gsc_add_sc(&lssc, &sc);
		/* compare the sum of the children with parent's sc */
		sc.m1 = parent->pq_u.hfsc_opts.lssc_m1;
		sc.d = parent->pq_u.hfsc_opts.lssc_d;
		sc.m2 = parent->pq_u.hfsc_opts.lssc_m2;
		if (!is_gsc_under_sc(&lssc, &sc)) {
			warnx("linkshare sc exceeds parent's sc");
			goto err_ret;
		}
	}

	/* check the upper-limit service curve. */
	if (opts->ulsc_m2 != 0) {
		if (opts->ulsc_m1 > pa->ifbandwidth ||
		    opts->ulsc_m2 > pa->ifbandwidth) {
			warnx("upper-limit larger than interface bandwidth");
			goto err_ret;
		}
		if (opts->rtsc_m2 != 0 && opts->rtsc_m2 > opts->ulsc_m2) {
			warnx("upper-limit sc smaller than real-time sc");
			goto err_ret;
		}
	}

	gsc_destroy(&rtsc);
	gsc_destroy(&lssc);

	return (0);

err_ret:
	gsc_destroy(&rtsc);
	gsc_destroy(&lssc);
	return (-1);
}

static int
check_commit_hfsc(int dev, int opts, struct pf_altq *pa)
{
	struct pf_altq	*altq, *def = NULL;
	int		 default_class;
	int		 error = 0;

	/* check if hfsc has one default queue for this interface */
	default_class = 0;
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(altq->ifname, pa->ifname, IFNAMSIZ) != 0)
			continue;
		if (altq->qname[0] == 0)  /* this is for interface */
			continue;
		if (altq->parent[0] == 0)  /* dummy root */
			continue;
		if (altq->pq_u.hfsc_opts.flags & HFCF_DEFAULTCLASS) {
			default_class++;
			def = altq;
		}
	}
	if (default_class != 1) {
		warnx("should have one default queue on %s", pa->ifname);
		return (1);
	}
	/* make sure the default queue is a leaf */
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(altq->ifname, pa->ifname, IFNAMSIZ) != 0)
			continue;
		if (altq->qname[0] == 0)  /* this is for interface */
			continue;
		if (strncmp(altq->parent, def->qname, PF_QNAME_SIZE) == 0) {
			warnx("default queue is not a leaf");
			error++;
		}
	}
	return (error);
}

static int
print_hfsc_opts(const struct pf_altq *a, const struct node_queue_opt *qopts)
{
	const struct hfsc_opts		*opts;
	const struct node_hfsc_sc	*rtsc, *lssc, *ulsc;

	opts = &a->pq_u.hfsc_opts;
	if (qopts == NULL)
		rtsc = lssc = ulsc = NULL;
	else {
		rtsc = &qopts->data.hfsc_opts.realtime;
		lssc = &qopts->data.hfsc_opts.linkshare;
		ulsc = &qopts->data.hfsc_opts.upperlimit;
	}

	if (opts->flags || opts->rtsc_m2 != 0 || opts->ulsc_m2 != 0 ||
	    (opts->lssc_m2 != 0 && (opts->lssc_m2 != a->bandwidth ||
	    opts->lssc_d != 0))) {
		printf("hfsc(");
		if (opts->flags & HFCF_RED)
			printf(" red");
		if (opts->flags & HFCF_ECN)
			printf(" ecn");
		if (opts->flags & HFCF_RIO)
			printf(" rio");
		if (opts->flags & HFCF_CLEARDSCP)
			printf(" cleardscp");
		if (opts->flags & HFCF_DEFAULTCLASS)
			printf(" default");
		if (opts->rtsc_m2 != 0)
			print_hfsc_sc("realtime", opts->rtsc_m1, opts->rtsc_d,
			    opts->rtsc_m2, rtsc);
		if (opts->lssc_m2 != 0 && (opts->lssc_m2 != a->bandwidth ||
		    opts->lssc_d != 0))
			print_hfsc_sc("linkshare", opts->lssc_m1, opts->lssc_d,
			    opts->lssc_m2, lssc);
		if (opts->ulsc_m2 != 0)
			print_hfsc_sc("upperlimit", opts->ulsc_m1, opts->ulsc_d,
			    opts->ulsc_m2, ulsc);
		printf(" ) ");

		return (1);
	} else
		return (0);
}

/*
 * admission control using generalized service curve
 */

/* add a new service curve to a generalized service curve */
static void
gsc_add_sc(struct gen_sc *gsc, struct service_curve *sc)
{
	if (is_sc_null(sc))
		return;
	if (sc->d != 0)
		gsc_add_seg(gsc, 0.0, 0.0, (double)sc->d, (double)sc->m1);
	gsc_add_seg(gsc, (double)sc->d, 0.0, HUGE_VAL, (double)sc->m2);
}

/*
 * check whether all points of a generalized service curve have
 * their y-coordinates no larger than a given two-piece linear
 * service curve.
 */
static int
is_gsc_under_sc(struct gen_sc *gsc, struct service_curve *sc)
{
	struct segment	*s, *last, *end;
	double		 y;

	if (is_sc_null(sc)) {
		if (LIST_EMPTY(gsc))
			return (1);
		LIST_FOREACH(s, gsc, _next) {
			if (s->m != 0)
				return (0);
		}
		return (1);
	}
	/*
	 * gsc has a dummy entry at the end with x = HUGE_VAL.
	 * loop through up to this dummy entry.
	 */
	end = gsc_getentry(gsc, HUGE_VAL);
	if (end == NULL)
		return (1);
	last = NULL;
	for (s = LIST_FIRST(gsc); s != end; s = LIST_NEXT(s, _next)) {
		if (s->y > sc_x2y(sc, s->x))
			return (0);
		last = s;
	}
	/* last now holds the real last segment */
	if (last == NULL)
		return (1);
	if (last->m > sc->m2)
		return (0);
	if (last->x < sc->d && last->m > sc->m1) {
		y = last->y + (sc->d - last->x) * last->m;
		if (y > sc_x2y(sc, sc->d))
			return (0);
	}
	return (1);
}

static void
gsc_destroy(struct gen_sc *gsc)
{
	struct segment	*s;

	while ((s = LIST_FIRST(gsc)) != NULL) {
		LIST_REMOVE(s, _next);
		free(s);
	}
}

/*
 * return a segment entry starting at x.
 * if gsc has no entry starting at x, a new entry is created at x.
 */
static struct segment *
gsc_getentry(struct gen_sc *gsc, double x)
{
	struct segment	*new, *prev, *s;

	prev = NULL;
	LIST_FOREACH(s, gsc, _next) {
		if (s->x == x)
			return (s);	/* matching entry found */
		else if (s->x < x)
			prev = s;
		else
			break;
	}

	/* we have to create a new entry */
	if ((new = calloc(1, sizeof(struct segment))) == NULL)
		return (NULL);

	new->x = x;
	if (x == HUGE_VAL || s == NULL)
		new->d = 0;
	else if (s->x == HUGE_VAL)
		new->d = HUGE_VAL;
	else
		new->d = s->x - x;
	if (prev == NULL) {
		/* insert the new entry at the head of the list */
		new->y = 0;
		new->m = 0;
		LIST_INSERT_HEAD(gsc, new, _next);
	} else {
		/*
		 * the start point intersects with the segment pointed by
		 * prev.  divide prev into 2 segments
		 */
		if (x == HUGE_VAL) {
			prev->d = HUGE_VAL;
			if (prev->m == 0)
				new->y = prev->y;
			else
				new->y = HUGE_VAL;
		} else {
			prev->d = x - prev->x;
			new->y = prev->d * prev->m + prev->y;
		}
		new->m = prev->m;
		LIST_INSERT_AFTER(prev, new, _next);
	}
	return (new);
}

/* add a segment to a generalized service curve */
static int
gsc_add_seg(struct gen_sc *gsc, double x, double y, double d, double m)
{
	struct segment	*start, *end, *s;
	double		 x2;

	if (d == HUGE_VAL)
		x2 = HUGE_VAL;
	else
		x2 = x + d;
	start = gsc_getentry(gsc, x);
	end = gsc_getentry(gsc, x2);
	if (start == NULL || end == NULL)
		return (-1);

	for (s = start; s != end; s = LIST_NEXT(s, _next)) {
		s->m += m;
		s->y += y + (s->x - x) * m;
	}

	end = gsc_getentry(gsc, HUGE_VAL);
	for (; s != end; s = LIST_NEXT(s, _next)) {
		s->y += m * d;
	}

	return (0);
}

/* get y-projection of a service curve */
static double
sc_x2y(struct service_curve *sc, double x)
{
	double	y;

	if (x <= (double)sc->d)
		/* y belongs to the 1st segment */
		y = x * (double)sc->m1;
	else
		/* y belongs to the 2nd segment */
		y = (double)sc->d * (double)sc->m1
			+ (x - (double)sc->d) * (double)sc->m2;
	return (y);
}

/*
 * misc utilities
 */
#define	R2S_BUFS	8
#define	RATESTR_MAX	16

char *
rate2str(double rate)
{
	char		*buf;
	static char	 r2sbuf[R2S_BUFS][RATESTR_MAX];  /* ring bufer */
	static int	 idx = 0;
	int		 i;
	static const char unit[] = " KMG";

	buf = r2sbuf[idx++];
	if (idx == R2S_BUFS)
		idx = 0;

	for (i = 0; rate >= 1000 && i <= 3; i++)
		rate /= 1000;

	if ((int)(rate * 100) % 100)
		snprintf(buf, RATESTR_MAX, "%.2f%cb", rate, unit[i]);
	else
		snprintf(buf, RATESTR_MAX, "%d%cb", (int)rate, unit[i]);

	return (buf);
}

u_int32_t
getifspeed(char *ifname)
{
#ifdef __NetBSD__
	int			 s;
	struct ifdatareq	 ifdr;
	struct if_data		*ifrdat;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "getifspeed: socket");
	memset(&ifdr, 0, sizeof(ifdr));
	if (strlcpy(ifdr.ifdr_name, ifname, sizeof(ifdr.ifdr_name)) >=
	    sizeof(ifdr.ifdr_name))
		errx(1, "getifspeed: strlcpy");
	if (ioctl(s, SIOCGIFDATA, &ifdr) == -1)
		err(1, "getifspeed: SIOCGIFDATA");
	ifrdat = &ifdr.ifdr_data;
	if (close(s) == -1)
		err(1, "getifspeed: close");
	return ((u_int32_t)ifrdat->ifi_baudrate);
#else
	int		s;
	struct ifreq	ifr;
	struct if_data	ifrdat;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");
	bzero(&ifr, sizeof(ifr));
	if (strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)) >=
	    sizeof(ifr.ifr_name))
		errx(1, "getifspeed: strlcpy");
	ifr.ifr_data = (caddr_t)&ifrdat;
	if (ioctl(s, SIOCGIFDATA, (caddr_t)&ifr) == -1)
		err(1, "SIOCGIFDATA");
	if (close(s))
		err(1, "close");
	return ((u_int32_t)ifrdat.ifi_baudrate);
#endif /* !__NetBSD__ */
}

u_long
getifmtu(char *ifname)
{
	int		s;
	struct ifreq	ifr;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");
	bzero(&ifr, sizeof(ifr));
	if (strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)) >=
	    sizeof(ifr.ifr_name))
		errx(1, "getifmtu: strlcpy");
	if (ioctl(s, SIOCGIFMTU, (caddr_t)&ifr) == -1)
		err(1, "SIOCGIFMTU");
	if (close(s) == -1)
		err(1, "close");
	if (ifr.ifr_mtu > 0)
		return (ifr.ifr_mtu);
	else {
		warnx("could not get mtu for %s, assuming 1500", ifname);
		return (1500);
	}
}

int
eval_queue_opts(struct pf_altq *pa, struct node_queue_opt *opts,
    u_int32_t ref_bw)
{
	int	errors = 0;

	switch (pa->scheduler) {
	case ALTQT_CBQ:
		pa->pq_u.cbq_opts = opts->data.cbq_opts;
		break;
	case ALTQT_PRIQ:
		pa->pq_u.priq_opts = opts->data.priq_opts;
		break;
	case ALTQT_HFSC:
		pa->pq_u.hfsc_opts.flags = opts->data.hfsc_opts.flags;
		if (opts->data.hfsc_opts.linkshare.used) {
			pa->pq_u.hfsc_opts.lssc_m1 =
			    eval_bwspec(&opts->data.hfsc_opts.linkshare.m1,
			    ref_bw);
			pa->pq_u.hfsc_opts.lssc_m2 =
			    eval_bwspec(&opts->data.hfsc_opts.linkshare.m2,
			    ref_bw);
			pa->pq_u.hfsc_opts.lssc_d =
			    opts->data.hfsc_opts.linkshare.d;
		}
		if (opts->data.hfsc_opts.realtime.used) {
			pa->pq_u.hfsc_opts.rtsc_m1 =
			    eval_bwspec(&opts->data.hfsc_opts.realtime.m1,
			    ref_bw);
			pa->pq_u.hfsc_opts.rtsc_m2 =
			    eval_bwspec(&opts->data.hfsc_opts.realtime.m2,
			    ref_bw);
			pa->pq_u.hfsc_opts.rtsc_d =
			    opts->data.hfsc_opts.realtime.d;
		}
		if (opts->data.hfsc_opts.upperlimit.used) {
			pa->pq_u.hfsc_opts.ulsc_m1 =
			    eval_bwspec(&opts->data.hfsc_opts.upperlimit.m1,
			    ref_bw);
			pa->pq_u.hfsc_opts.ulsc_m2 =
			    eval_bwspec(&opts->data.hfsc_opts.upperlimit.m2,
			    ref_bw);
			pa->pq_u.hfsc_opts.ulsc_d =
			    opts->data.hfsc_opts.upperlimit.d;
		}
		break;
	default:
		warnx("eval_queue_opts: unknown scheduler type %u",
		    opts->qtype);
		errors++;
		break;
	}

	return (errors);
}

u_int32_t
eval_bwspec(struct node_queue_bw *bw, u_int32_t ref_bw)
{
	if (bw->bw_absolute > 0)
		return (bw->bw_absolute);

	if (bw->bw_percent > 0)
		return (ref_bw / 100 * bw->bw_percent);

	return (0);
}

void
print_hfsc_sc(const char *scname, u_int m1, u_int d, u_int m2,
    const struct node_hfsc_sc *sc)
{
	printf(" %s", scname);

	if (d != 0) {
		printf("(");
		if (sc != NULL && sc->m1.bw_percent > 0)
			printf("%u%%", sc->m1.bw_percent);
		else
			printf("%s", rate2str((double)m1));
		printf(" %u", d);
	}

	if (sc != NULL && sc->m2.bw_percent > 0)
		printf(" %u%%", sc->m2.bw_percent);
	else
		printf(" %s", rate2str((double)m2));

	if (d != 0)
		printf(")");
}
