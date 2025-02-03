/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Nyarko.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <net/if.h>
#include <netinet/in.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "npf.h"
#include <altq/altq.h>
#include <altq/altq_cbq.h>
#include <altq/altq_priq.h>
#include <altq/altq_hfsc.h>
#include "npfctl.h"

LIST_HEAD(gen_sc, segment) rtsc, lssc;
static int	npf_add_root_queue(struct npf_altq, const char *,
			char *, struct node_queue_opt *);
static int	eval_npfqueue_cbq(struct npf_altq *);
static int	cbq_compute_idletime(struct npf_altq *);
static int	check_commit_cbq(struct npf_altq *);
static int	eval_npfqueue_priq(struct npf_altq *);
static int	check_commit_priq(struct npf_altq *);
static int	eval_npfqueue_hfsc(struct npf_altq *);
static int	check_commit_hfsc(struct npf_altq *);
static void 	altq_append_queues(struct npf_altq pa, const char *,
			char *, struct node_queue *);
static void 	queue_append_queues(struct npf_altq *, struct node_queue *,
				struct node_queue *);
static int 		scheduler_check(struct npf_altq *, struct node_queue *,
				struct node_queue *, struct node_queue_bw);
static void		 gsc_add_sc(struct gen_sc *, struct service_curve *);
static int		 is_gsc_under_sc(struct gen_sc *,
			     struct service_curve *);
static void		 gsc_destroy(struct gen_sc *);
static struct segment	*gsc_getentry(struct gen_sc *, double);
static int		 gsc_add_seg(struct gen_sc *, double, double, double,
			     double);
static double		 sc_x2y(struct service_curve *, double);
int ifdisc_lookup(struct npf_altq *);


int npfdev;
int altqsupport;

TAILQ_HEAD(altqs, npf_altq) altqs = TAILQ_HEAD_INITIALIZER(altqs);
#define is_sc_null(sc)	(((sc) == NULL) || ((sc)->m1 == 0 && (sc)->m2 == 0))

struct node_queue *queues = NULL;


#define FREE_LIST(T,r) \
	do { \
		T *p, *node = r; \
		while (node != NULL) { \
			p = node; \
			node = node->next; \
			free(p); \
		} \
	} while (0)

#define LOOP_THROUGH(T,n,r,C) \
	do { \
		T *n; \
		if (r == NULL) { \
			r = calloc(1, sizeof(*r)); \
			if (r == NULL) \
				err(EXIT_FAILURE, "LOOP: calloc"); \
			r->next = NULL; \
		} \
		n = r; \
		while (n != NULL) { \
			do { \
				C; \
			} while (0); \
			n = n->next; \
		} \
	} while (0)

int
npfctl_test_altqsupport(int dev)
{
	struct npfioc_altq pa;
	if (ioctl(dev, IOC_NPF_GET_ALTQS, &pa) == -1) {
		if (errno == ENODEV) {
			warnx("No ALTQ support in kernel\n"
				"ALTQ related functions disabled\n");
			return 0;
		} else
		err(EXIT_FAILURE, "IOC_GET_ALTQS");
	}
	return 1;
}

/* evaluate bandwidth */
int
npfctl_eval_bw(struct node_queue_bw *bw, char *bw_spec)
{
	double	 bps;
	char	*cp;
	bw->bw_percent = 0;
	bps = strtod(bw_spec, &cp);
	if (cp != NULL) {
		if (!strcmp(cp, "b"))
			; /* nothing */
		else if (!strcmp(cp, "Kb"))
			bps *= 1000;
		else if (!strcmp(cp, "Mb"))
			bps *= 1000 * 1000;
		else if (!strcmp(cp, "Gb"))
			bps *= 1000 * 1000 * 1000;
		else if (!strcmp(cp, "%")) {
			if (bps < 0 || bps > 100) {
				yyerror("bandwidth spec "
					"out of range");
				return -1;
			}
			bw->bw_percent = bps;
			bps = 0;
		} else {
			yyerror("unknown unit %s", cp);
			return -1;
		}
	}
	bw->bw_absolute = (uint32_t)bps;
	return 0;
}

/* create root queue for cbq or hfsc */
static int
npf_add_root_queue(struct npf_altq pa, const char * ifname,
	char qname[], struct node_queue_opt *opts)
{
	struct node_queue_bw	bw;
	struct npf_altq pb;
	int errs = 0;

	/*
	 * we cannot use sizeof(qname) directly here as it will give sizeof(char*)
	 * so the copyably bytes is manually hack the  using sizeof(char) * qname max size
	 */
	memset(&pb, 0, sizeof(pb));
	if (strlcpy(qname, "root_", (sizeof(char) * NPF_QNAME_SIZE)) >=
		(sizeof(char) * NPF_QNAME_SIZE))
		errx(EXIT_FAILURE, "add_root: strlcpy");
	if (strlcat(qname, ifname, (sizeof(char) * NPF_QNAME_SIZE)) >=
		(sizeof(char) * NPF_QNAME_SIZE))
		errx(EXIT_FAILURE, "add_root: strlcat");
	if (strlcpy(pb.qname, qname,
		sizeof(pb.qname)) >= sizeof(pb.qname))
		errx(EXIT_FAILURE, "add_root: strlcpy");
	if (strlcpy(pb.ifname, ifname,
		sizeof(pb.ifname)) >= sizeof(pb.ifname))
		errx(EXIT_FAILURE, "add_root: strlcpy");
	pb.qlimit = pa.qlimit;
	pb.scheduler = pa.scheduler;
	bw.bw_absolute = pa.ifbandwidth;
	bw.bw_percent = 0;
	if (eval_npfqueue(&pb, &bw, opts))
		errs++;
	else
		if (npfctl_add_altq(&pb))
			errs++;
	return errs;
}

/*
 * child queues set on altq decl will be appended to global queue here:
 * altq on .... queue {a,b ,c }
 */
static void
altq_append_queues(struct npf_altq pa, const char *ifname,
	char qname[], struct node_queue *queue)
{
	struct node_queue	*n;
	n = calloc(1, sizeof(*n));
	if (n == NULL)
		err(EXIT_FAILURE, "append_queue: calloc");
	if (pa.scheduler == ALTQT_CBQ ||
		pa.scheduler == ALTQT_HFSC)
		if (strlcpy(n->parent, qname,
			sizeof(n->parent)) >=
			sizeof(n->parent))
			errx(EXIT_FAILURE, "append_queue: strlcpy");
	if (strlcpy(n->queue, queue->queue,
		sizeof(n->queue)) >= sizeof(n->queue))
		errx(EXIT_FAILURE, "append_queue: strlcpy");
	if (strlcpy(n->ifname, ifname,
		sizeof(n->ifname)) >= sizeof(n->ifname))
		errx(EXIT_FAILURE, "append_queue: strlcpy");
	n->scheduler = pa.scheduler;
	n->next = NULL;
	n->tail = n;
	if (queues == NULL)
		queues = n;
	else {
		queues->tail->next = n;
		queues->tail = n;
	}
}

/*
 * child queues set on new defining child queues appended on a global queue:
 * queue .... queue {a, b, b}
 */
static void
queue_append_queues(struct npf_altq *a, struct node_queue *tqueue,
	struct node_queue *nq)
{
	struct node_queue	*n;
	n = calloc(1,
		sizeof(*n));
	if (n == NULL)
		err(EXIT_FAILURE, "expand_queue: calloc");
	if (strlcpy(n->parent, a->qname,
		sizeof(n->parent)) >=
		sizeof(n->parent))
		errx(EXIT_FAILURE, "expand_queue strlcpy");
	if (strlcpy(n->queue, nq->queue,
		sizeof(n->queue)) >=
		sizeof(n->queue))
		errx(EXIT_FAILURE, "expand_queue strlcpy");
	if (strlcpy(n->ifname, tqueue->ifname,
		sizeof(n->ifname)) >=
		sizeof(n->ifname))
		errx(EXIT_FAILURE, "expand_queue strlcpy");
	n->scheduler = tqueue->scheduler;
	n->next = NULL;
	n->tail = n;
	if (queues == NULL)
		queues = n;
	else {
		queues->tail->next = n;
		queues->tail = n;
	}
}

static int
scheduler_check(struct npf_altq *pa, struct node_queue *tqueue,
	struct node_queue *nqueues, struct node_queue_bw bwspec)
{
	if (pa->scheduler != ALTQT_NONE &&
		pa->scheduler != tqueue->scheduler) {
		yyerror("exactly one scheduler type "
			"per interface allowed");
		return -1;
	}
	pa->scheduler = tqueue->scheduler;
	/* scheduler dependent error checking */
	switch (pa->scheduler) {
	case ALTQT_PRIQ:
		if (nqueues != NULL) {
			yyerror("priq queues cannot "
				"have child queues");
			return -1;
		}
		if (bwspec.bw_absolute > 0 ||
			bwspec.bw_percent < 100) {
			yyerror("priq doesn't take "
				"bandwidth");
			return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}

int
expand_altq(struct npf_altq *a, const char *ifname,
    struct node_queue *nqueues, struct node_queue_bw bwspec,
    struct node_queue_opt *opts)
{
	struct npf_altq		 pa;
	char			 qname[NPF_QNAME_SIZE];
	int			 errs = 0;
	npfdev = npfctl_open_dev(NPF_DEV_PATH);

	memcpy(&pa, a, sizeof(pa));
	if (strlcpy(pa.ifname, ifname,
		sizeof(pa.ifname)) >= sizeof(pa.ifname))
		errx(1, "expand_altq: strlcpy");
	if (ifdisc_lookup(&pa)) {
		yyerror("only one scheduler per interface.\n altq already defined on %s", pa.ifname);
		errs++;
	} else {
		if (eval_npfaltq(&pa, &bwspec, opts))
			errs++;
		else
			if (ioctl(npfdev, IOC_NPF_BEGIN_ALTQ) == 0) {
				if (npfctl_add_altq(&pa)){
					yyerror("cannot add parent queue");
					errs++;
				}
			} else
				errx(EXIT_FAILURE, "cannot begin altq: altq_begin");

		if (pa.scheduler == ALTQT_CBQ ||
			pa.scheduler == ALTQT_HFSC) {
			/* now create a root queue */
			if (npf_add_root_queue(pa, ifname, qname, opts))
				errx(EXIT_FAILURE, "cannot add root queue");
			}
		LOOP_THROUGH(struct node_queue, queue, nqueues,
			altq_append_queues(pa, ifname, qname, queue));
	}
	FREE_LIST(struct node_queue, nqueues);

	return errs;
}

int
expand_queue(struct npf_altq *a, const char *ifname,
    struct node_queue *nqueues, struct node_queue_bw bwspec,
    struct node_queue_opt *opts)
{
	struct node_queue	*nq;
	struct npf_altq		 pa;
	uint8_t		 found = 0;
	uint8_t		 errs = 0;

	if (queues == NULL) {
		yyerror("queue %s has no parent", a->qname);
		FREE_LIST(struct node_queue, nqueues);
		return 1;
	}
	LOOP_THROUGH(struct node_queue, tqueue, queues,
		if (!strncmp(a->qname, tqueue->queue, NPF_QNAME_SIZE) &&
			(ifname == 0 ||
			(!strncmp(ifname, tqueue->ifname, IFNAMSIZ)) ||
			(strncmp(ifname, tqueue->ifname, IFNAMSIZ)))){
			/* found ourself in the child queues */
			found++;
			memcpy(&pa, a, sizeof(pa));
			if (scheduler_check(&pa, tqueue, nqueues, bwspec) == -1)
				goto out;

			if (strlcpy(pa.ifname, tqueue->ifname,
				sizeof(pa.ifname)) >= sizeof(pa.ifname))
				errx(1, "expand_queue: strlcpy");
			if (strlcpy(pa.parent, tqueue->parent,
				sizeof(pa.parent)) >= sizeof(pa.parent))
				errx(1, "expand_queue: strlcpy");
			if (eval_npfqueue(&pa, &bwspec, opts))
				errs++;
			else
				if (npfctl_add_altq(&pa))
					errs++;
			for (nq = nqueues; nq != NULL; nq = nq->next) {
				if (!strcmp(a->qname, nq->queue)) {
					yyerror("queue cannot have "
						"itself as child");
					errs++;
					continue;
				}
				queue_append_queues(a, tqueue, nq);
			}
		}
	);
out:
	FREE_LIST(struct node_queue, nqueues);
	if (!found) {
		yyerror("queue %s has no parent", a->qname);
		errs++;
	}
	if (errs)
		return 1;
	else
		return 0;
}

/*
 * eval_npfaltq computes the discipline parameters.
 */
int
eval_npfaltq(struct npf_altq *pa, struct node_queue_bw *bw,
    struct node_queue_opt *opts)
{
	u_int	rate, size, errors = 0;
	if (bw->bw_absolute > 0)
		pa->ifbandwidth = bw->bw_absolute;
	else
		if ((rate = get_ifspeed(pa->ifname)) == 0) {
			fprintf(stderr, "interface %s does not know its bandwidth, "
			    "please specify an absolute bandwidth\n",
			    pa->ifname);
			errors++;
		} else if ((pa->ifbandwidth = npf_eval_bwspec(bw, rate)) == 0)
			pa->ifbandwidth = rate;
	errors += npf_eval_queue_opts(pa, opts, pa->ifbandwidth);
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
		size = size * get_ifmtu(pa->ifname);
		if (size > 0xffff)
			size = 0xffff;
		pa->tbrsize = size;
	}
	return errors;
}

int
npf_eval_queue_opts(struct npf_altq *pa, struct node_queue_opt *opts,
    uint32_t ref_bw)
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
			    npf_eval_bwspec(&opts->data.hfsc_opts.linkshare.m1,
			    ref_bw);
			pa->pq_u.hfsc_opts.lssc_m2 =
			    npf_eval_bwspec(&opts->data.hfsc_opts.linkshare.m2,
			    ref_bw);
			pa->pq_u.hfsc_opts.lssc_d =
			    opts->data.hfsc_opts.linkshare.d;
		}
		if (opts->data.hfsc_opts.realtime.used) {
			pa->pq_u.hfsc_opts.rtsc_m1 =
			    npf_eval_bwspec(&opts->data.hfsc_opts.realtime.m1,
			    ref_bw);
			pa->pq_u.hfsc_opts.rtsc_m2 =
			    npf_eval_bwspec(&opts->data.hfsc_opts.realtime.m2,
			    ref_bw);
			pa->pq_u.hfsc_opts.rtsc_d =
			    opts->data.hfsc_opts.realtime.d;
		}
		if (opts->data.hfsc_opts.upperlimit.used) {
			pa->pq_u.hfsc_opts.ulsc_m1 =
			    npf_eval_bwspec(&opts->data.hfsc_opts.upperlimit.m1,
			    ref_bw);
			pa->pq_u.hfsc_opts.ulsc_m2 =
			    npf_eval_bwspec(&opts->data.hfsc_opts.upperlimit.m2,
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
	return errors;
}

int
npfctl_add_altq(struct npf_altq *a)
{
	struct npfioc_altq *npaltq;
	if ((npaltq =  malloc(sizeof(*npaltq))) == NULL)
		err(EXIT_FAILURE, "malloc");
	memcpy(&npaltq->altq, a, sizeof(npaltq->altq));
	if (ioctl(npfdev, IOC_NPF_ADD_ALTQ, npaltq)) {
		if (errno == ENXIO)
			errx(1, "qtype not configured");
		else if (errno == ENODEV)
			errx(1, "%s: driver does not support "
				"altq", a->ifname);
		else
			err(EXIT_FAILURE, "NPFADDALTQ");
	}
	npfaltq_store(&npaltq->altq);
	free(npaltq);
	return 0;
}

void
npfaltq_store(struct npf_altq *a)
{
	struct npf_altq	*altq;
	if ((altq = malloc(sizeof(*altq))) == NULL)
		err(EXIT_FAILURE, "malloc");
	memcpy(altq, a, sizeof(*altq));
	TAILQ_INSERT_TAIL(&altqs, altq, entries);
	/* check altq presence in config */
}

uint32_t
npf_eval_bwspec(struct node_queue_bw *bw, uint32_t ref_bw)
{
	if (bw->bw_absolute > 0)
		return (bw->bw_absolute);
	if (bw->bw_percent > 0)
		return (ref_bw / 100 * bw->bw_percent);
	return 0;
}

uint32_t
get_ifspeed(char *ifname)
{
	int			 s;
	struct ifdatareq	 ifdr;
	struct if_data		*ifrdat;
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(EXIT_FAILURE, "getifspeed: socket");
	memset(&ifdr, 0, sizeof(ifdr));
	if (strlcpy(ifdr.ifdr_name, ifname, sizeof(ifdr.ifdr_name)) >=
	    sizeof(ifdr.ifdr_name))
		errx(1, "getifspeed: strlcpy");
	if (ioctl(s, SIOCGIFDATA, &ifdr) == -1)
		err(EXIT_FAILURE, "getifspeed: SIOCGIFDATA");
	ifrdat = &ifdr.ifdr_data;
	if (close(s) == -1)
		err(EXIT_FAILURE, "getifspeed: close");
	return ((uint32_t)ifrdat->ifi_baudrate);
}

u_long
get_ifmtu(char *ifname)
{
	int		s;
	struct ifreq	ifr;
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(EXIT_FAILURE, "socket");
	bzero(&ifr, sizeof(ifr));
	if (strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)) >=
	    sizeof(ifr.ifr_name))
		errx(1, "getifmtu: strlcpy");
	if (ioctl(s, SIOCGIFMTU, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCGIFMTU");
	if (close(s) == -1)
		err(EXIT_FAILURE, "close");
	if (ifr.ifr_mtu > 0)
		return (ifr.ifr_mtu);
	else {
		warnx("could not get mtu for %s, assuming 1500", ifname);
		return 1500;
	}
}

/*
 * eval_npfqueue computes the queue parameters.
 */
int
eval_npfqueue(struct npf_altq *pa, struct node_queue_bw *bw,
    struct node_queue_opt *opts)
{
	/* should be merged with expand_queue */
	struct npf_altq	*if_pa, *parent, *altq;
	uint32_t	 bwsum;
	int		 error = 0;
	/* find the corresponding interface and copy fields used by queues */
	if ((if_pa = npfaltq_lookup(pa->ifname)) == NULL) {
		fprintf(stderr, "altq not defined on %s\n", pa->ifname);
		return 1;
	}
	pa->scheduler = if_pa->scheduler;
	pa->ifbandwidth = if_pa->ifbandwidth;
	if (qname_to_npfaltq(pa->qname, pa->ifname) != NULL) {
		fprintf(stderr, "queue %s already exists on interface %s\n",
		    pa->qname, pa->ifname);
		return 1;
	}
	pa->qid = qname_to_qid(pa->qname);
	parent = NULL;
	if (pa->parent[0] != 0) {
		parent = qname_to_npfaltq(pa->parent, pa->ifname);
		if (parent == NULL) {
			fprintf(stderr, "parent %s not found for %s\n",
			    pa->parent, pa->qname);
			return 1;
		}
		pa->parent_qid = parent->qid;
	}
	if (pa->qlimit == 0)
		pa->qlimit = DEFAULT_QLIMIT;
	if (pa->scheduler == ALTQT_CBQ || pa->scheduler == ALTQT_HFSC) {
		pa->bandwidth = npf_eval_bwspec(bw,
		    parent == NULL ? 0 : parent->bandwidth);
		if (pa->bandwidth > pa->ifbandwidth) {
			fprintf(stderr, "bandwidth for %s higher than "
			    "interface\n", pa->qname);
			return 1;
		}
		/* check the sum of the child bandwidth is under parent's */
		if (parent != NULL) {
			if (pa->bandwidth > parent->bandwidth) {
				warnx("bandwidth for %s higher than parent",
				    pa->qname);
				return 1;
			}
			bwsum = 0;
			TAILQ_FOREACH(altq, &altqs, entries) {
				if (strncmp(altq->ifname, pa->ifname,
				    IFNAMSIZ) == 0 &&
				    altq->qname[0] != 0 &&
				    strncmp(altq->parent, pa->parent,
				    NPF_QNAME_SIZE) == 0)
					bwsum += altq->bandwidth;
			}
			bwsum += pa->bandwidth;
			if (bwsum > parent->bandwidth) {
				warnx("the sum of the child bandwidth higher"
				    " than parent \"%s\"", parent->qname);
			}
		}
	}
	if (npf_eval_queue_opts(pa, opts, parent == NULL? 0 : parent->bandwidth))
		return 1;
	switch (pa->scheduler) {
	case ALTQT_CBQ:
		error = eval_npfqueue_cbq(pa);
		break;
	case ALTQT_PRIQ:
		error = eval_npfqueue_priq(pa);
		break;
	case ALTQT_HFSC:
		error = eval_npfqueue_hfsc(pa);
		break;
	default:
		break;
	}
	return error;
}

struct npf_altq *
qname_to_npfaltq(const char *qname, const char *ifname)
{
	struct npf_altq	*altq;
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(ifname, altq->ifname, IFNAMSIZ) == 0 &&
		    strncmp(qname, altq->qname, NPF_QNAME_SIZE) == 0)
			return altq;
	}
	return NULL;
}

uint32_t
qname_to_qid(const char *qname)
{
	struct npf_altq	*altq;
	/*
	 * We guarantee that same named queues on different interfaces
	 * have the same qid, so we do NOT need to limit matching on
	 * one interface!
	 */
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(qname, altq->qname, NPF_QNAME_SIZE) == 0)
			return (altq->qid);
	}
	return 0;
}

/*define only one discipline on one interface */
int
ifdisc_lookup(struct npf_altq * altq)
{
	struct npf_altq *a;
	if ((a = TAILQ_FIRST(&altqs)) != NULL) {
		if ((a = npfaltq_lookup(altq->ifname)) != NULL) {
			if (a->scheduler != altq->scheduler) {
				return -1;
			}
		}
	}
	return 0;
}

struct npf_altq *
npfaltq_lookup(const char *ifname)
{
	struct npf_altq	*altq;
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(ifname, altq->ifname, IFNAMSIZ) == 0 &&
		    altq->qname[0] == 0)
			return altq;
	}
	return NULL;
}

/*
 * CBQ support functions
 */
#define	RM_FILTER_GAIN	5	/* log2 of gain, e.g., 5 => 31/32 */
#define	RM_NS_PER_SEC	(1000000000)
static int
eval_npfqueue_cbq(struct npf_altq *pa)
{
	struct npf_cbq_opts	*opts;
	u_int		 ifmtu;
	if (pa->priority >= CBQ_MAXPRI) {
		warnx("priority out of range: max %d", CBQ_MAXPRI - 1);
		return -1;
	}
	ifmtu = get_ifmtu(pa->ifname);
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
	cbq_compute_idletime(pa);
	return 0;
}

/*
 * compute ns_per_byte, maxidle, minidle, and offtime
 */
static int
cbq_compute_idletime(struct npf_altq *pa)
{
	struct npf_cbq_opts	*opts;
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
		if (pa->bandwidth != 0) {
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
	return 0;
}

/*
 * PRIQ support functions
 */
static int
eval_npfqueue_priq(struct npf_altq *pa)
{
	struct npf_altq	*altq;
	if (pa->priority >= PRIQ_MAXPRI) {
		warnx("priority out of range: max %d", PRIQ_MAXPRI - 1);
		return -1;
	}
	/* the priority should be unique for the interface */
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(altq->ifname, pa->ifname, IFNAMSIZ) == 0 &&
		    altq->qname[0] != 0 && altq->priority == pa->priority) {
			warnx("%s and %s have the same priority",
			    altq->qname, pa->qname);
			return -1;
		}
	}
	return 0;
}

/*
 * HFSC support functions
 */
static int
eval_npfqueue_hfsc(struct npf_altq *pa)
{
	struct npf_altq		*altq, *parent;
	struct npf_hfsc_opts	*opts;
	struct service_curve	 sc;
	opts = &pa->pq_u.hfsc_opts;
	if (pa->parent[0] == 0) {
		/* root queue */
		opts->lssc_m1 = pa->ifbandwidth;
		opts->lssc_m2 = pa->ifbandwidth;
		opts->lssc_d = 0;
		return 0;
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
		return -1;
	}
	if ((opts->rtsc_m1 < opts->rtsc_m2 && opts->rtsc_m1 != 0) ||
	    (opts->lssc_m1 < opts->lssc_m2 && opts->lssc_m1 != 0) ||
	    (opts->ulsc_m1 < opts->ulsc_m2 && opts->ulsc_m1 != 0)) {
		warnx("m1 must be zero for convex curve: %s", pa->qname);
		return -1;
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
	parent = qname_to_npfaltq(pa->parent, pa->ifname);
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
		if (strncmp(altq->parent, pa->parent, NPF_QNAME_SIZE) != 0)
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
	return 0;
err_ret:
	gsc_destroy(&rtsc);
	gsc_destroy(&lssc);
	return -1;
}

#define	R2S_BUFS	8
#define	RATESTR_MAX	16
char *
rate2str(double rate)
{
	char		*buf;
	static char	 r2sbuf[R2S_BUFS][RATESTR_MAX];  /* ring buffer */
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
	return buf;
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
			return 1;
		LIST_FOREACH(s, gsc, _next) {
			if (s->m != 0)
				return 0;
		}
		return 1;
	}
	/*
	 * gsc has a dummy entry at the end with x = HUGE_VAL.
	 * loop through up to this dummy entry.
	 */
	end = gsc_getentry(gsc, HUGE_VAL);
	if (end == NULL)
		return 1;
	last = NULL;
	for (s = LIST_FIRST(gsc); s != end; s = LIST_NEXT(s, _next)) {
		if (s->y > sc_x2y(sc, s->x))
			return 0;
		last = s;
	}
	/* last now holds the real last segment */
	if (last == NULL)
		return 1;
	if (last->m > sc->m2)
		return 0;
	if (last->x < sc->d && last->m > sc->m1) {
		y = last->y + (sc->d - last->x) * last->m;
		if (y > sc_x2y(sc, sc->d))
			return 0;
	}
	return 1;
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
			return s;	/* matching entry found */
		else if (s->x < x)
			prev = s;
		else
			break;
	}
	/* we have to create a new entry */
	if ((new = calloc(1, sizeof(*new))) == NULL)
		return NULL;
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
	return new;
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
		return -1;
	for (s = start; s != end; s = LIST_NEXT(s, _next)) {
		s->m += m;
		s->y += y + (s->x - x) * m;
	}
	end = gsc_getentry(gsc, HUGE_VAL);
	for (; s != end; s = LIST_NEXT(s, _next)) {
		s->y += m * d;
	}
	return 0;
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
	return y;
}

/*
 * check_commit_altq does consistency check for each interface
 */
int
check_commit_altq(void)
{
	struct npf_altq	*altq;
	int		 error = 0;
	/* call the discipline check for each interface. */
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (altq->qname[0] == 0) {
			switch (altq->scheduler) {
			case ALTQT_CBQ:
				error = check_commit_cbq(altq);
				break;
			case ALTQT_PRIQ:
				error = check_commit_priq(altq);
				break;
			case ALTQT_HFSC:
				error = check_commit_hfsc(altq);
				break;
			default:
				break;
			}
		}
	}
	return error;
}

static int
check_commit_cbq(struct npf_altq *pa)
{
	struct npf_altq	*altq;
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
	return error;
}

static int
check_commit_priq(struct npf_altq *pa)
{
	struct npf_altq	*altq;
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
	return error;
}

static int
check_commit_hfsc(struct npf_altq *pa)
{
	struct npf_altq	*altq, *def = NULL;
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
		return 1;
	}
	/* make sure the default queue is a leaf */
	TAILQ_FOREACH(altq, &altqs, entries) {
		if (strncmp(altq->ifname, pa->ifname, IFNAMSIZ) != 0)
			continue;
		if (altq->qname[0] == 0)  /* this is for interface */
			continue;
		if (strncmp(altq->parent, def->qname, NPF_QNAME_SIZE) == 0) {
			warnx("default queue is not a leaf");
			error++;
		}
	}
	return error;
}
