/*	$NetBSD: bufq_priocscan.c,v 1.7.14.1 2006/06/19 04:07:15 chap Exp $	*/

/*-
 * Copyright (c)2004 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bufq_priocscan.c,v 1.7.14.1 2006/06/19 04:07:15 chap Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/bufq_impl.h>
#include <sys/malloc.h>

/*
 * Cyclical scan (CSCAN)
 */
TAILQ_HEAD(bqhead, buf);
struct cscan_queue {
	struct bqhead cq_head[2];	/* actual lists of buffers */
	int cq_idx;			/* current list index */
	int cq_lastcylinder;		/* b_cylinder of the last request */
	daddr_t cq_lastrawblkno;	/* b_rawblkno of the last request */
};

static inline int cscan_empty(const struct cscan_queue *);
static void cscan_put(struct cscan_queue *, struct buf *, int);
static struct buf *cscan_get(struct cscan_queue *, int);
static void cscan_init(struct cscan_queue *);

static inline int
cscan_empty(const struct cscan_queue *q)
{

	return TAILQ_EMPTY(&q->cq_head[0]) && TAILQ_EMPTY(&q->cq_head[1]);
}

static void
cscan_put(struct cscan_queue *q, struct buf *bp, int sortby)
{
	struct buf tmp;
	struct buf *it;
	struct bqhead *bqh;
	int idx;

	tmp.b_cylinder = q->cq_lastcylinder;
	tmp.b_rawblkno = q->cq_lastrawblkno;

	if (buf_inorder(bp, &tmp, sortby))
		idx = 1 - q->cq_idx;
	else
		idx = q->cq_idx;

	bqh = &q->cq_head[idx];

	TAILQ_FOREACH(it, bqh, b_actq)
		if (buf_inorder(bp, it, sortby))
			break;

	if (it != NULL)
		TAILQ_INSERT_BEFORE(it, bp, b_actq);
	else
		TAILQ_INSERT_TAIL(bqh, bp, b_actq);
}

static struct buf *
cscan_get(struct cscan_queue *q, int remove)
{
	int idx = q->cq_idx;
	struct bqhead *bqh;
	struct buf *bp;

	bqh = &q->cq_head[idx];
	bp = TAILQ_FIRST(bqh);

	if (bp == NULL) {
		/* switch queue */
		idx = 1 - idx;
		bqh = &q->cq_head[idx];
		bp = TAILQ_FIRST(bqh);
	}

	KDASSERT((bp != NULL && !cscan_empty(q)) ||
	         (bp == NULL && cscan_empty(q)));

	if (bp != NULL && remove) {
		q->cq_idx = idx;
		TAILQ_REMOVE(bqh, bp, b_actq);

		q->cq_lastcylinder = bp->b_cylinder;
		q->cq_lastrawblkno =
		    bp->b_rawblkno + (bp->b_bcount >> DEV_BSHIFT);
	}

	return (bp);
}

static void
cscan_init(struct cscan_queue *q)
{

	TAILQ_INIT(&q->cq_head[0]);
	TAILQ_INIT(&q->cq_head[1]);
}


/*
 * Per-prioritiy CSCAN.
 *
 * XXX probably we should have a way to raise
 * priority of the on-queue requests.
 */
#define	PRIOCSCAN_NQUEUE	3

struct priocscan_queue {
	struct cscan_queue q_queue;
	int q_burst;
};

struct bufq_priocscan {
	struct priocscan_queue bq_queue[PRIOCSCAN_NQUEUE];

#if 0
	/*
	 * XXX using "global" head position can reduce positioning time
	 * when switching between queues.
	 * although it might affect against fairness.
	 */
	daddr_t bq_lastrawblkno;
	int bq_lastcylinder;
#endif
};

/*
 * how many requests to serve when having pending requests on other queues.
 *
 * XXX tune
 */
const int priocscan_burst[] = {
	64, 16, 4
};

static void bufq_priocscan_init(struct bufq_state *);
static void bufq_priocscan_put(struct bufq_state *, struct buf *);
static struct buf *bufq_priocscan_get(struct bufq_state *, int);

BUFQ_DEFINE(priocscan, 40, bufq_priocscan_init);

static inline struct cscan_queue *bufq_priocscan_selectqueue(
    struct bufq_priocscan *, const struct buf *);

static inline struct cscan_queue *
bufq_priocscan_selectqueue(struct bufq_priocscan *q, const struct buf *bp)
{
	static const int priocscan_priomap[] = {
		[BPRIO_TIMENONCRITICAL] = 2,
		[BPRIO_TIMELIMITED] = 1,
		[BPRIO_TIMECRITICAL] = 0
	};

	return &q->bq_queue[priocscan_priomap[BIO_GETPRIO(bp)]].q_queue;
}

static void
bufq_priocscan_put(struct bufq_state *bufq, struct buf *bp)
{
	struct bufq_priocscan *q = bufq->bq_private;
	struct cscan_queue *cq;
	const int sortby = bufq->bq_flags & BUFQ_SORT_MASK;

	cq = bufq_priocscan_selectqueue(q, bp);
	cscan_put(cq, bp, sortby);
}

static struct buf *
bufq_priocscan_get(struct bufq_state *bufq, int remove)
{
	struct bufq_priocscan *q = bufq->bq_private;
	struct priocscan_queue *pq, *npq;
	struct priocscan_queue *first; /* first non-empty queue */
	const struct priocscan_queue *epq;
	const struct cscan_queue *cq;
	struct buf *bp;
	boolean_t single; /* true if there's only one non-empty queue */

	pq = &q->bq_queue[0];
	epq = pq + PRIOCSCAN_NQUEUE;
	for (; pq < epq; pq++) {
		cq = &pq->q_queue;
		if (!cscan_empty(cq))
			break;
	}
	if (pq == epq) {
		/* there's no requests */
		return NULL;
	}

	first = pq;
	single = TRUE;
	for (npq = first + 1; npq < epq; npq++) {
		cq = &npq->q_queue;
		if (!cscan_empty(cq)) {
			single = FALSE;
			if (pq->q_burst > 0)
				break;
			pq = npq;
		}
	}
	if (single) {
		/*
		 * there's only a non-empty queue.  just serve it.
		 */
		pq = first;
	} else if (pq->q_burst > 0) {
		/*
		 * XXX account only by number of requests.  is it good enough?
		 */
		if (remove) {
			pq->q_burst--;
		}
	} else {
		/*
		 * no queue was selected due to burst counts
		 */
		int i;
#ifdef DEBUG
		for (i = 0; i < PRIOCSCAN_NQUEUE; i++) {
			pq = &q->bq_queue[i];
			cq = &pq->q_queue;
			if (!cscan_empty(cq) && pq->q_burst)
				panic("%s: inconsist", __func__);
		}
#endif /* DEBUG */

		/*
		 * reset burst counts
		 */
		if (remove) {
			for (i = 0; i < PRIOCSCAN_NQUEUE; i++) {
				pq = &q->bq_queue[i];
				pq->q_burst = priocscan_burst[i];
			}
		}

		/*
		 * serve first non-empty queue.
		 */
		pq = first;
	}

	KDASSERT(!cscan_empty(&pq->q_queue));
	bp = cscan_get(&pq->q_queue, remove);
	KDASSERT(bp != NULL);
	KDASSERT(&pq->q_queue == bufq_priocscan_selectqueue(q, bp));

	return bp;
}

static void
bufq_priocscan_init(struct bufq_state *bufq)
{
	struct bufq_priocscan *q;
	int i;

	bufq->bq_get = bufq_priocscan_get;
	bufq->bq_put = bufq_priocscan_put;
	bufq->bq_private = malloc(sizeof(struct bufq_priocscan),
	    M_DEVBUF, M_ZERO);

	q = bufq->bq_private;
	for (i = 0; i < PRIOCSCAN_NQUEUE; i++) {
		struct cscan_queue *cq = &q->bq_queue[i].q_queue;

		cscan_init(cq);
	}
}
