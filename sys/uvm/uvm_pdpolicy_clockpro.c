/*	$NetBSD: uvm_pdpolicy_clockpro.c,v 1.15.16.1 2012/02/09 03:05:01 matt Exp $	*/

/*-
 * Copyright (c)2005, 2006 YAMAMOTO Takashi,
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

/*
 * CLOCK-Pro replacement policy:
 *	http://www.cs.wm.edu/hpcs/WWW/HTML/publications/abs05-3.html
 *
 * approximation of the list of non-resident pages using hash:
 *	http://linux-mm.org/ClockProApproximation
 */

/* #define	CLOCKPRO_DEBUG */

#if defined(PDSIM)

#include "pdsim.h"

#else /* defined(PDSIM) */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_pdpolicy_clockpro.c,v 1.15.16.1 2012/02/09 03:05:01 matt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/hash.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>
#include <uvm/uvm_pdpolicy_impl.h>

#if ((__STDC_VERSION__ - 0) >= 199901L)
#define	DPRINTF(...)	/* nothing */
#define	WARN(...)	printf(__VA_ARGS__)
#else /* ((__STDC_VERSION__ - 0) >= 199901L) */
#define	DPRINTF(a...)	/* nothing */	/* GCC */
#define	WARN(a...)	printf(a)
#endif /* ((__STDC_VERSION__ - 0) >= 199901L) */

#define	dump(a)		/* nothing */

#undef	USEONCE2
#define	LISTQ
#undef	ADAPTIVE

#endif /* defined(PDSIM) */

#if !defined(CLOCKPRO_COLDPCT)
#define	CLOCKPRO_COLDPCT	10
#endif /* !defined(CLOCKPRO_COLDPCT) */

#define	CLOCKPRO_COLDPCTMAX	90

#if !defined(CLOCKPRO_HASHFACTOR)
#define	CLOCKPRO_HASHFACTOR	2
#endif /* !defined(CLOCKPRO_HASHFACTOR) */

#define	CLOCKPRO_NEWQMIN	((1024 * 1024) >> PAGE_SHIFT)	/* XXX */

int clockpro_hashfactor = CLOCKPRO_HASHFACTOR;

PDPOL_EVCNT_DEFINE(nresrecordobj)
PDPOL_EVCNT_DEFINE(nresrecordanon)
PDPOL_EVCNT_DEFINE(nreslookupobj)
PDPOL_EVCNT_DEFINE(nreslookupanon)
PDPOL_EVCNT_DEFINE(nresfoundobj)
PDPOL_EVCNT_DEFINE(nresfoundanon)
PDPOL_EVCNT_DEFINE(nresanonfree)
PDPOL_EVCNT_DEFINE(nresconflict)
PDPOL_EVCNT_DEFINE(nresoverwritten)
PDPOL_EVCNT_DEFINE(nreshandhot)

PDPOL_EVCNT_DEFINE(hhottakeover)
PDPOL_EVCNT_DEFINE(hhotref)
PDPOL_EVCNT_DEFINE(hhotunref)
PDPOL_EVCNT_DEFINE(hhotcold)
PDPOL_EVCNT_DEFINE(hhotcoldtest)

PDPOL_EVCNT_DEFINE(hcoldtakeover)
PDPOL_EVCNT_DEFINE(hcoldref)
PDPOL_EVCNT_DEFINE(hcoldunref)
PDPOL_EVCNT_DEFINE(hcoldreftest)
PDPOL_EVCNT_DEFINE(hcoldunreftest)
PDPOL_EVCNT_DEFINE(hcoldunreftestspeculative)
PDPOL_EVCNT_DEFINE(hcoldhot)

PDPOL_EVCNT_DEFINE(speculativeenqueue)
PDPOL_EVCNT_DEFINE(speculativehit1)
PDPOL_EVCNT_DEFINE(speculativehit2)
PDPOL_EVCNT_DEFINE(speculativemiss)

#define	PQ_REFERENCED	PQ_PRIVATE1
#define	PQ_HOT		PQ_PRIVATE2
#define	PQ_TEST		PQ_PRIVATE3
#define	PQ_INITIALREF	PQ_PRIVATE4
#if PQ_PRIVATE6 != PQ_PRIVATE5 * 2 || PQ_PRIVATE7 != PQ_PRIVATE6 * 2
#error PQ_PRIVATE
#endif
#define	PQ_QMASK	(PQ_PRIVATE5|PQ_PRIVATE6|PQ_PRIVATE7)
#define	PQ_QFACTOR	PQ_PRIVATE5
#define	PQ_SPECULATIVE	PQ_PRIVATE8

#define	CLOCKPRO_NOQUEUE	0
#define	CLOCKPRO_NEWQ		1	/* small queue to clear initial ref. */
#if defined(LISTQ)
#define	CLOCKPRO_COLDQ(gs)	2
#define	CLOCKPRO_HOTQ(gs)	3
#else /* defined(LISTQ) */
#define	CLOCKPRO_COLDQ(gs)	(2 + (gs)->gs_coldqidx)	/* XXX */
#define	CLOCKPRO_HOTQ(gs)	(3 - (gs)->gs_coldqidx)	/* XXX */
#endif /* defined(LISTQ) */
#define	CLOCKPRO_LISTQ		4
#define	CLOCKPRO_NQUEUE		4

static inline void
clockpro_setq(struct vm_page *pg, int qidx)
{
	KASSERT(qidx >= CLOCKPRO_NOQUEUE);
	KASSERT(qidx <= CLOCKPRO_NQUEUE);

	pg->pqflags = (pg->pqflags & ~PQ_QMASK) | (qidx * PQ_QFACTOR);
}

static inline int
clockpro_getq(struct vm_page *pg)
{
	int qidx;

	qidx = (pg->pqflags & PQ_QMASK) / PQ_QFACTOR;
	KASSERT(qidx >= CLOCKPRO_NOQUEUE);
	KASSERT(qidx <= CLOCKPRO_NQUEUE);
	return qidx;
}

typedef struct {
	struct pglist q_q;
	u_int q_len;
} pageq_t;

typedef uint32_t nonres_cookie_t;
#define	NONRES_COOKIE_INVAL	0

#define	BUCKETSIZE	14
struct bucket {
	u_int cycle;
	u_int cur;
	nonres_cookie_t pages[BUCKETSIZE];
};

static size_t cycle_target;
static size_t cycle_target_frac;
static size_t hashsize;
static struct bucket *buckets;

struct uvmpdpol_groupstate {
	pageq_t gs_q[CLOCKPRO_NQUEUE];
	struct uvm_pggroup *gs_pgrp;
	u_int gs_npages;
	u_int gs_coldtarget;
	u_int gs_ncold;
	u_int gs_newqlenmax;
#if !defined(LISTQ)
	u_int gs_coldqidx;
#endif
	u_int gs_nscanned;
	u_int gs_coldadj;
};

struct clockpro_state {
	struct uvmpdpol_groupstate *s_gs;
	struct uvm_pctparam s_coldtargetpct;
};

static inline pageq_t *
clockpro_queue(struct uvmpdpol_groupstate *gs, u_int qidx)
{

	KASSERT(CLOCKPRO_NOQUEUE < qidx);
	KASSERT(qidx <= CLOCKPRO_NQUEUE);

	return &gs->gs_q[qidx - 1];
}

#if !defined(LISTQ)

static inline void
clockpro_switchqueue(struct uvmpdpol_groupstate *gs)
{

	gs->gs_coldqidx ^= 1;
}

#endif /* !defined(LISTQ) */

static struct clockpro_state clockpro;

/* ---------------------------------------- */

static void
pageq_init(pageq_t *q)
{

	TAILQ_INIT(&q->q_q);
	q->q_len = 0;
}

static u_int
pageq_len(const pageq_t *q)
{

	return q->q_len;
}

static struct vm_page *
pageq_first(const pageq_t *q)
{

	return TAILQ_FIRST(&q->q_q);
}

static void
pageq_insert_tail(struct uvmpdpol_groupstate *gs, pageq_t *q, struct vm_page *pg)
{
	KASSERT(clockpro_queue(gs, clockpro_getq(pg)) == q);

	TAILQ_INSERT_TAIL(&q->q_q, pg, pageq.queue);
	q->q_len++;
}

#if defined(LISTQ)
static void
pageq_insert_head(struct uvmpdpol_groupstate *gs, pageq_t *q, struct vm_page *pg)
{
	KASSERT(clockpro_queue(gs, clockpro_getq(pg)) == q);

	TAILQ_INSERT_HEAD(&q->q_q, pg, pageq.queue);
	q->q_len++;
}
#endif

static void
pageq_remove(struct uvmpdpol_groupstate *gs, pageq_t *q, struct vm_page *pg)
{
	KASSERT(clockpro_queue(gs, clockpro_getq(pg)) == q);
	KASSERT(q->q_len > 0);
	TAILQ_REMOVE(&q->q_q, pg, pageq.queue);
	q->q_len--;
}

static struct vm_page *
pageq_remove_head(struct uvmpdpol_groupstate *gs, pageq_t *q)
{
	struct vm_page *pg;

	pg = TAILQ_FIRST(&q->q_q);
	if (pg == NULL) {
		KASSERT(q->q_len == 0);
		return NULL;
	}

	pageq_remove(gs, q, pg);
	return pg;
}

/* ---------------------------------------- */

static void
clockpro_insert_tail(struct uvmpdpol_groupstate *gs, u_int qidx, struct vm_page *pg)
{
	pageq_t *q = clockpro_queue(gs, qidx);
	
	clockpro_setq(pg, qidx);
	pageq_insert_tail(gs, q, pg);
}

#if defined(LISTQ)
static void
clockpro_insert_head(struct uvmpdpol_groupstate *gs, u_int qidx, struct vm_page *pg)
{
	pageq_t *q = clockpro_queue(gs, qidx);
	
	clockpro_setq(pg, qidx);
	pageq_insert_head(gs, q, pg);
}

#endif
/* ---------------------------------------- */

typedef uintptr_t objid_t;

/*
 * XXX maybe these hash functions need reconsideration,
 * given that hash distribution is critical here.
 */

static uint32_t
pageidentityhash1(objid_t obj, off_t idx)
{
	uint32_t hash = HASH32_BUF_INIT;

#if 1
	hash = hash32_buf(&idx, sizeof(idx), hash);
	hash = hash32_buf(&obj, sizeof(obj), hash);
#else
	hash = hash32_buf(&obj, sizeof(obj), hash);
	hash = hash32_buf(&idx, sizeof(idx), hash);
#endif
	return hash;
}

static uint32_t
pageidentityhash2(objid_t obj, off_t idx)
{
	uint32_t hash = HASH32_BUF_INIT;

	hash = hash32_buf(&obj, sizeof(obj), hash);
	hash = hash32_buf(&idx, sizeof(idx), hash);
	return hash;
}

static nonres_cookie_t
calccookie(objid_t obj, off_t idx)
{
	uint32_t hash = pageidentityhash2(obj, idx);
	nonres_cookie_t cookie = hash;

	if (__predict_false(cookie == NONRES_COOKIE_INVAL)) {
		cookie++; /* XXX */
	}
	return cookie;
}

#define	COLDTARGET_ADJ(gs, d)	((gs)->gs_coldadj += (d))

#if defined(PDSIM)

static void *
clockpro_hashalloc(u_int n)
{
	size_t allocsz = sizeof(struct bucket) * n;

	return malloc(allocsz);
}

static void
clockpro_hashfree(void *p, int n)
{

	free(p);
}

#else /* defined(PDSIM) */

static void *
clockpro_hashalloc(u_int n)
{
	size_t allocsz = round_page(sizeof(struct bucket) * n);

	return (void *)uvm_km_alloc(kernel_map, allocsz, 0, UVM_KMF_WIRED);
}

static void
clockpro_hashfree(void *p, u_int n)
{
	size_t allocsz = round_page(sizeof(struct bucket) * n);

	uvm_km_free(kernel_map, (vaddr_t)p, allocsz, UVM_KMF_WIRED);
}

#endif /* defined(PDSIM) */

static void
clockpro_hashinit(uint64_t n)
{
	struct bucket *newbuckets;
	struct bucket *oldbuckets;
	size_t sz;
	size_t oldsz;
	int i;

	sz = howmany(n, BUCKETSIZE);
	sz *= clockpro_hashfactor;
	newbuckets = clockpro_hashalloc(sz);
	if (newbuckets == NULL) {
		panic("%s: allocation failure", __func__);
	}
	for (i = 0; i < sz; i++) {
		struct bucket *b = &newbuckets[i];
		int j;

		b->cycle = cycle_target;
		b->cur = 0;
		for (j = 0; j < BUCKETSIZE; j++) {
			b->pages[j] = NONRES_COOKIE_INVAL;
		}
	}
	/* XXX lock */
	oldbuckets = buckets;
	oldsz = hashsize;
	buckets = newbuckets;
	hashsize = sz;
	/* XXX unlock */
	if (oldbuckets) {
		clockpro_hashfree(oldbuckets, oldsz);
	}
}

static struct bucket *
nonresident_getbucket(objid_t obj, off_t idx)
{
	uint32_t hash;

	hash = pageidentityhash1(obj, idx);
	return &buckets[hash % hashsize];
}

static void
nonresident_rotate(struct uvmpdpol_groupstate *gs, struct bucket *b)
{
	const int target = cycle_target;
	const int cycle = b->cycle;
	int cur;
	int todo;

	todo = target - cycle;
	if (todo >= BUCKETSIZE * 2) {
		todo = (todo % BUCKETSIZE) + BUCKETSIZE;
	}
	cur = b->cur;
	while (todo > 0) {
		if (b->pages[cur] != NONRES_COOKIE_INVAL) {
			PDPOL_EVCNT_INCR(nreshandhot);
			if (gs != NULL)
				COLDTARGET_ADJ(gs, -1);
		}
		b->pages[cur] = NONRES_COOKIE_INVAL;
		cur++;
		if (cur == BUCKETSIZE) {
			cur = 0;
		}
		todo--;
	}
	b->cycle = target;
	b->cur = cur;
}

static bool
nonresident_lookupremove(struct uvmpdpol_groupstate *gs, objid_t obj, off_t idx)
{
	struct bucket *b = nonresident_getbucket(obj, idx);
	nonres_cookie_t cookie = calccookie(obj, idx);

	nonresident_rotate(gs, b);
	for (u_int i = 0; i < BUCKETSIZE; i++) {
		if (b->pages[i] == cookie) {
			b->pages[i] = NONRES_COOKIE_INVAL;
			return true;
		}
	}
	return false;
}

static objid_t
pageobj(struct vm_page *pg)
{
	const void *obj;

	/*
	 * XXX object pointer is often freed and reused for unrelated object.
	 * for vnodes, it would be better to use something like
	 * a hash of fsid/fileid/generation.
	 */

	obj = pg->uobject;
	if (obj == NULL) {
		obj = pg->uanon;
		KASSERT(obj != NULL);
		KASSERT(pg->offset == 0);
	}

	return (objid_t)obj;
}

static off_t
pageidx(struct vm_page *pg)
{

	KASSERT((pg->offset & PAGE_MASK) == 0);
	return pg->offset >> PAGE_SHIFT;
}

static bool
nonresident_pagelookupremove(struct uvmpdpol_groupstate *gs, struct vm_page *pg)
{
	bool found = nonresident_lookupremove(gs, pageobj(pg), pageidx(pg));

	if (pg->uobject) {
		PDPOL_EVCNT_INCR(nreslookupobj);
	} else {
		PDPOL_EVCNT_INCR(nreslookupanon);
	}
	if (found) {
		if (pg->uobject) {
			PDPOL_EVCNT_INCR(nresfoundobj);
		} else {
			PDPOL_EVCNT_INCR(nresfoundanon);
		}
	}
	return found;
}

static void
nonresident_pagerecord(struct uvmpdpol_groupstate *gs, struct vm_page *pg)
{
	const objid_t obj = pageobj(pg);
	const off_t idx = pageidx(pg);
	struct bucket * const b = nonresident_getbucket(obj, idx);
	nonres_cookie_t cookie = calccookie(obj, idx);

#if defined(DEBUG)
	for (u_int i = 0; i < BUCKETSIZE; i++) {
		if (b->pages[i] == cookie) {
			PDPOL_EVCNT_INCR(nresconflict);
		}
	}
#endif /* defined(DEBUG) */

	if (pg->uobject) {
		PDPOL_EVCNT_INCR(nresrecordobj);
	} else {
		PDPOL_EVCNT_INCR(nresrecordanon);
	}
	nonresident_rotate(gs, b);
	if (b->pages[b->cur] != NONRES_COOKIE_INVAL) {
		PDPOL_EVCNT_INCR(nresoverwritten);
		COLDTARGET_ADJ(gs, -1);
	}
	b->pages[b->cur] = cookie;
	b->cur = (b->cur + 1) % BUCKETSIZE;
}

/* ---------------------------------------- */

#if defined(CLOCKPRO_DEBUG)
static void
check_sanity(void)
{
}
#else /* defined(CLOCKPRO_DEBUG) */
#define	check_sanity()	/* nothing */
#endif /* defined(CLOCKPRO_DEBUG) */

static void
clockpro_reinit(void)
{

	clockpro_hashinit(uvmexp.npages);
}

static void
clockpro_recolor(void *new_gs, struct uvm_pggroup *grparray,
	size_t npggroup, size_t old_ncolors)
{
	struct uvmpdpol_groupstate *old_gs = clockpro.s_gs;
	struct uvm_pggroup *grp = uvm.pggroups;
	struct uvmpdpol_groupstate *gs = new_gs;
	const size_t old_npggroup = VM_NPGGROUP(old_ncolors);

	clockpro.s_gs = gs;

	for (size_t pggroup = 0; pggroup < npggroup; pggroup++, gs++, grp++) {
		grp->pgrp_gs = gs;
		gs->gs_pgrp = grp;
		for (u_int i = 0; i < CLOCKPRO_NQUEUE; i++) {
			pageq_init(&gs->gs_q[i]);
		}
		gs->gs_newqlenmax = 1;
		gs->gs_coldtarget = 1;
	}

	for (size_t pggroup = 0; pggroup < old_npggroup; pggroup++, old_gs++) {
		pageq_t *oldq = old_gs->gs_q;
		for (u_int i = 0; i < CLOCKPRO_NQUEUE; i++, oldq++) {
			while (pageq_len(oldq) > 0) {
				struct vm_page *pg = pageq_remove_head(old_gs, oldq);
				KASSERT(pg != NULL);
				grp = uvm_page_to_pggroup(pg);
				gs = grp->pgrp_gs;
				pageq_insert_tail(gs, &gs->gs_q[i], pg);
#if defined(USEONCE2)
#else
				gs->gs_npages++;
				if (pg->pqflags & (PQ_TEST|PQ_SPECULATIVE)) {
					gs->gs_ncold++;
				}
#endif
			}
		}
	}

	uvm_pctparam_init(&clockpro.s_coldtargetpct, CLOCKPRO_COLDPCT, NULL);

}

static void
clockpro_init(void *new_gs, size_t npggroup)
{
	struct uvm_pggroup *grp = uvm.pggroups;
	struct uvmpdpol_groupstate *gs = new_gs;

	for (size_t pggroup = 0; pggroup < npggroup; pggroup++, gs++, grp++) {
		grp->pgrp_gs = gs;
		gs->gs_pgrp = grp;
		for (u_int i = 0; i < CLOCKPRO_NQUEUE; i++) {
			pageq_init(&gs->gs_q[i]);
		}
		gs->gs_newqlenmax = 1;
		gs->gs_coldtarget = 1;
	}

	uvm_pctparam_init(&clockpro.s_coldtargetpct, CLOCKPRO_COLDPCT, NULL);
}

static void
clockpro_tune(struct uvmpdpol_groupstate *gs)
{
	int coldtarget;

#if defined(ADAPTIVE)
	u_int coldmax = gs->s_npages * CLOCKPRO_COLDPCTMAX / 100;
	u_int coldmin = 1;

	coldtarget = gs->gs_coldtarget;
	if (coldtarget + gs->gs_coldadj < coldmin) {
		gs->gs_coldadj = coldmin - coldtarget;
	} else if (coldtarget + gs->gs_coldadj > coldmax) {
		gs->gs_coldadj = coldmax - coldtarget;
	}
	coldtarget += gs->gs_coldadj;
#else /* defined(ADAPTIVE) */
	coldtarget = UVM_PCTPARAM_APPLY(&clockpro.s_coldtargetpct,
	    gs->gs_npages);
	if (coldtarget < 1) {
		coldtarget = 1;
	}
#endif /* defined(ADAPTIVE) */

	gs->gs_coldtarget = coldtarget;
	gs->gs_newqlenmax = coldtarget / 4;
	if (gs->gs_newqlenmax < CLOCKPRO_NEWQMIN) {
		gs->gs_newqlenmax = CLOCKPRO_NEWQMIN;
	}
}

static void
clockpro_movereferencebit(struct vm_page *pg)
{
	bool referenced;

	referenced = pmap_clear_reference(pg);
	if (referenced) {
		pg->pqflags |= PQ_REFERENCED;
	}
}

static void
clockpro_clearreferencebit(struct vm_page *pg)
{

	clockpro_movereferencebit(pg);
	pg->pqflags &= ~PQ_REFERENCED;
}

static void
clockpro___newqrotate(struct uvmpdpol_groupstate * const gs, int len)
{
	pageq_t * const newq = clockpro_queue(gs, CLOCKPRO_NEWQ);

	while (pageq_len(newq) > len) {
		struct vm_page *pg = pageq_remove_head(gs, newq);
		KASSERT(pg != NULL);
		KASSERT(clockpro_getq(pg) == CLOCKPRO_NEWQ);
		if ((pg->pqflags & PQ_INITIALREF) != 0) {
			clockpro_clearreferencebit(pg);
			pg->pqflags &= ~PQ_INITIALREF;
		}
		/* place at the list head */
		clockpro_insert_tail(gs, CLOCKPRO_COLDQ(gs), pg);
	}
}

static void
clockpro_newqrotate(struct uvmpdpol_groupstate * const gs)
{

	check_sanity();
	clockpro___newqrotate(gs, gs->gs_newqlenmax);
	check_sanity();
}

static void
clockpro_newqflush(struct uvmpdpol_groupstate * const gs, int n)
{

	check_sanity();
	clockpro___newqrotate(gs, n);
	check_sanity();
}

static void
clockpro_newqflushone(struct uvmpdpol_groupstate *gs)
{

	clockpro_newqflush(gs,
	    MAX(pageq_len(clockpro_queue(gs, CLOCKPRO_NEWQ)) - 1, 0));
}

/*
 * our "tail" is called "list-head" in the paper.
 */

static void
clockpro___enqueuetail(struct uvmpdpol_groupstate *gs, struct vm_page *pg)
{

	KASSERT(clockpro_getq(pg) == CLOCKPRO_NOQUEUE);

	check_sanity();
#if !defined(USEONCE2)
	clockpro_insert_tail(gs, CLOCKPRO_NEWQ, pg);
	clockpro_newqrotate(gs);
#else /* !defined(USEONCE2) */
#if defined(LISTQ)
	KASSERT((pg->pqflags & PQ_REFERENCED) == 0);
#endif /* defined(LISTQ) */
	clockpro_insert_tail(gs, CLOCKPRO_COLDQ(gs), pg);
#endif /* !defined(USEONCE2) */
	check_sanity();
}

static void
clockpro_pageenqueue(struct vm_page *pg)
{
	struct uvm_pggroup *grp = uvm_page_to_pggroup(pg);
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
	bool hot;
	bool speculative = (pg->pqflags & PQ_SPECULATIVE) != 0; /* XXX */

	KASSERT((~pg->pqflags & (PQ_INITIALREF|PQ_SPECULATIVE)) != 0);
	KASSERT(mutex_owned(&uvm_pageqlock));
	check_sanity();
	KASSERT(clockpro_getq(pg) == CLOCKPRO_NOQUEUE);
	gs->gs_npages++;
	pg->pqflags &= ~(PQ_HOT|PQ_TEST);
	if (speculative) {
		hot = false;
		PDPOL_EVCNT_INCR(speculativeenqueue);
	} else {
		hot = nonresident_pagelookupremove(gs, pg);
		if (hot) {
			COLDTARGET_ADJ(gs, 1);
		}
	}

	/*
	 * consider mmap'ed file:
	 *
	 * - read-ahead enqueues a page.
	 *
	 * - on the following read-ahead hit, the fault handler activates it.
	 *
	 * - finally, the userland code which caused the above fault
	 *   actually accesses the page.  it makes its reference bit set.
	 *
	 * we want to count the above as a single access, rather than
	 * three accesses with short reuse distances.
	 */

#if defined(USEONCE2)
	pg->pqflags &= ~PQ_INITIALREF;
	if (hot) {
		pg->pqflags |= PQ_TEST;
	}
	gs->gs_ncold++;
	clockpro_clearreferencebit(pg);
	clockpro___enqueuetail(gs, pg);
#else /* defined(USEONCE2) */
	if (speculative) {
		gs->gs_ncold++;
	} else if (hot) {
		pg->pqflags |= PQ_HOT;
	} else {
		pg->pqflags |= PQ_TEST;
		gs->gs_ncold++;
	}
	clockpro___enqueuetail(gs, pg);
#endif /* defined(USEONCE2) */
	grp->pgrp_inactive = gs->gs_ncold;
	grp->pgrp_active = gs->gs_npages - gs->gs_ncold;
	KASSERT(gs->gs_ncold <= gs->gs_npages);
}

static pageq_t *
clockpro_pagequeue(struct vm_page *pg)
{
	struct uvm_pggroup *grp = uvm_page_to_pggroup(pg);
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
	u_int qidx;

	qidx = clockpro_getq(pg);
	KASSERT(qidx != CLOCKPRO_NOQUEUE);

	return clockpro_queue(gs, qidx);
}

static void
clockpro_pagedequeue(struct vm_page *pg)
{
	struct uvm_pggroup *grp = uvm_page_to_pggroup(pg);
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
	pageq_t *q;

	KASSERT(gs->gs_npages > 0);
	check_sanity();
	q = clockpro_pagequeue(pg);
	pageq_remove(gs, q, pg);
	check_sanity();
	clockpro_setq(pg, CLOCKPRO_NOQUEUE);
	if ((pg->pqflags & PQ_HOT) == 0) {
		KASSERT(gs->gs_ncold > 0);
		gs->gs_ncold--;
	}
	KASSERT(gs->gs_npages > 0);
	gs->gs_npages--;
	grp->pgrp_inactive = gs->gs_ncold;
	grp->pgrp_active = gs->gs_npages - gs->gs_ncold;
	check_sanity();
}

static void
clockpro_pagerequeue(struct vm_page *pg)
{
	struct uvm_pggroup *grp = uvm_page_to_pggroup(pg);
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
	u_int qidx;

	qidx = clockpro_getq(pg);
	KASSERT(qidx == CLOCKPRO_HOTQ(gs) || qidx == CLOCKPRO_COLDQ(gs));
	pageq_remove(gs, clockpro_queue(gs, qidx), pg);
	check_sanity();
	clockpro_setq(pg, CLOCKPRO_NOQUEUE);

	clockpro___enqueuetail(gs, pg);
}

static void
handhot_endtest(struct uvmpdpol_groupstate * const gs, struct vm_page *pg)
{

	KASSERT((pg->pqflags & PQ_HOT) == 0);
	if ((pg->pqflags & PQ_TEST) != 0) {
		PDPOL_EVCNT_INCR(hhotcoldtest);
		COLDTARGET_ADJ(gs, -1);
		pg->pqflags &= ~PQ_TEST;
	} else {
		PDPOL_EVCNT_INCR(hhotcold);
	}
}

static void
handhot_advance(struct uvmpdpol_groupstate * const gs)
{
	struct vm_page *pg;
	pageq_t *hotq;
	u_int hotqlen;

	clockpro_tune(gs);

	dump("hot called");
	if (gs->gs_ncold >= gs->gs_coldtarget) {
		return;
	}
	hotq = clockpro_queue(gs, CLOCKPRO_HOTQ(gs));
again:
	pg = pageq_first(hotq);
	if (pg == NULL) {
		DPRINTF("%s: HHOT TAKEOVER\n", __func__);
		dump("hhottakeover");
		PDPOL_EVCNT_INCR(hhottakeover);
#if defined(LISTQ)
		while (/* CONSTCOND */ 1) {
			pageq_t *coldq = clockpro_queue(gs, CLOCKPRO_COLDQ(gs));

			pg = pageq_first(coldq);
			if (pg == NULL) {
				clockpro_newqflushone(gs);
				pg = pageq_first(coldq);
				if (pg == NULL) {
					WARN("hhot: no page?\n");
					return;
				}
			}
			KASSERT(clockpro_pagequeue(pg) == coldq);
			pageq_remove(gs, coldq, pg);
			check_sanity();
			if ((pg->pqflags & PQ_HOT) == 0) {
				handhot_endtest(gs, pg);
				clockpro_insert_tail(gs, CLOCKPRO_LISTQ, pg);
			} else {
				clockpro_insert_head(gs, CLOCKPRO_HOTQ(gs), pg);
				break;
			}
		}
#else /* defined(LISTQ) */
		clockpro_newqflush(gs, 0); /* XXX XXX */
		clockpro_switchqueue(gs);
		hotq = clockpro_queue(gs, CLOCKPRO_HOTQ(gs));
		goto again;
#endif /* defined(LISTQ) */
	}

	KASSERT(clockpro_pagequeue(pg) == hotq);

	/*
	 * terminate test period of nonresident pages by cycling them.
	 */

	cycle_target_frac += BUCKETSIZE;
	hotqlen = pageq_len(hotq);
	while (cycle_target_frac >= hotqlen) {
		cycle_target++;
		cycle_target_frac -= hotqlen;
	}

	if ((pg->pqflags & PQ_HOT) == 0) {
#if defined(LISTQ)
		panic("cold page in hotq: %p", pg);
#else /* defined(LISTQ) */
		handhot_endtest(gs, pg);
		goto next;
#endif /* defined(LISTQ) */
	}
	KASSERT((pg->pqflags & PQ_TEST) == 0);
	KASSERT((pg->pqflags & PQ_INITIALREF) == 0);
	KASSERT((pg->pqflags & PQ_SPECULATIVE) == 0);

	/*
	 * once we met our target,
	 * stop at a hot page so that no cold pages in test period
	 * have larger recency than any hot pages.
	 */

	if (gs->gs_ncold >= gs->gs_coldtarget) {
		dump("hot done");
		return;
	}
	clockpro_movereferencebit(pg);
	if ((pg->pqflags & PQ_REFERENCED) == 0) {
		struct uvm_pggroup *grp = gs->gs_pgrp;
		PDPOL_EVCNT_INCR(hhotunref);
		grp->pgrp_pddeact++;
		pg->pqflags &= ~PQ_HOT;
		gs->gs_ncold++;
		grp->pgrp_inactive = gs->gs_ncold;
		grp->pgrp_active = gs->gs_npages - gs->gs_ncold;
		KASSERT(gs->gs_ncold <= gs->gs_npages);
	} else {
		PDPOL_EVCNT_INCR(hhotref);
	}
	pg->pqflags &= ~PQ_REFERENCED;
#if !defined(LISTQ)
next:
#endif /* !defined(LISTQ) */
	clockpro_pagerequeue(pg);
	dump("hot");
	goto again;
}

static struct vm_page *
handcold_advance(struct uvmpdpol_groupstate * const gs)
{
	struct uvm_pggroup * const grp = gs->gs_pgrp;
	struct vm_page *pg;

	for (;;) {
#if defined(LISTQ)
		pageq_t *listq = clockpro_queue(gs, CLOCKPRO_LISTQ);
#endif /* defined(LISTQ) */
		pageq_t *coldq;

		clockpro_newqrotate(gs);
		handhot_advance(gs);
#if defined(LISTQ)
		pg = pageq_first(listq);
		if (pg != NULL) {
			KASSERT(clockpro_getq(pg) == CLOCKPRO_LISTQ);
			KASSERT((pg->pqflags & PQ_TEST) == 0);
			KASSERT((pg->pqflags & PQ_HOT) == 0);
			KASSERT((pg->pqflags & PQ_INITIALREF) == 0);
			pageq_remove(gs, listq, pg);
			check_sanity();
			clockpro_insert_head(gs, CLOCKPRO_COLDQ(gs), pg); /* XXX */
			goto gotcold;
		}
#endif /* defined(LISTQ) */
		check_sanity();
		coldq = clockpro_queue(gs, CLOCKPRO_COLDQ(gs));
		pg = pageq_first(coldq);
		if (pg == NULL) {
			clockpro_newqflushone(gs);
			pg = pageq_first(coldq);
		}
		if (pg == NULL) {
			DPRINTF("%s: HCOLD TAKEOVER\n", __func__);
			dump("hcoldtakeover");
			PDPOL_EVCNT_INCR(hcoldtakeover);
			KASSERT(
			    pageq_len(clockpro_queue(gs, CLOCKPRO_NEWQ)) == 0);
#if defined(LISTQ)
			KASSERT(
			    pageq_len(clockpro_queue(gs, CLOCKPRO_HOTQ(gs))) == 0);
#else /* defined(LISTQ) */
			clockpro_switchqueue(gs);
			coldq = clockpro_queue(gs, CLOCKPRO_COLDQ(gs));
			pg = pageq_first(coldq);
#endif /* defined(LISTQ) */
		}
		if (pg == NULL) {
			WARN("hcold: no page?\n");
			return NULL;
		}
		KASSERT((pg->pqflags & PQ_INITIALREF) == 0);
		if ((pg->pqflags & PQ_HOT) != 0) {
			PDPOL_EVCNT_INCR(hcoldhot);
			pageq_remove(gs, coldq, pg);
			clockpro_insert_tail(gs, CLOCKPRO_HOTQ(gs), pg);
			check_sanity();
			KASSERT((pg->pqflags & PQ_TEST) == 0);
			grp->pgrp_pdscans++;
			continue;
		}
#if defined(LISTQ)
gotcold:
#endif /* defined(LISTQ) */
		KASSERT((pg->pqflags & PQ_HOT) == 0);
		grp->pgrp_pdscans++;
		clockpro_movereferencebit(pg);
		if ((pg->pqflags & PQ_SPECULATIVE) != 0) {
			KASSERT((pg->pqflags & PQ_TEST) == 0);
			if ((pg->pqflags & PQ_REFERENCED) != 0) {
				PDPOL_EVCNT_INCR(speculativehit2);
				pg->pqflags &= ~(PQ_SPECULATIVE|PQ_REFERENCED);
				clockpro_pagedequeue(pg);
				clockpro_pageenqueue(pg);
				continue;
			}
			PDPOL_EVCNT_INCR(speculativemiss);
		}
		switch (pg->pqflags & (PQ_REFERENCED|PQ_TEST)) {
		case PQ_TEST:
			PDPOL_EVCNT_INCR(hcoldunreftest);
			nonresident_pagerecord(gs, pg);
			goto gotit;
		case 0:
			PDPOL_EVCNT_INCR(hcoldunref);
gotit:
			KASSERT(gs->gs_ncold > 0);
			clockpro_pagerequeue(pg); /* XXX */
			dump("cold done");
			/* XXX "pg" is still in queue */
			handhot_advance(gs);
			goto done;

		case PQ_REFERENCED|PQ_TEST:
			PDPOL_EVCNT_INCR(hcoldreftest);
			gs->gs_ncold--;
			grp->pgrp_inactive = gs->gs_ncold;
			grp->pgrp_active = gs->gs_npages - gs->gs_ncold;
			COLDTARGET_ADJ(gs, 1);
			pg->pqflags |= PQ_HOT;
			pg->pqflags &= ~PQ_TEST;
			break;

		case PQ_REFERENCED:
			PDPOL_EVCNT_INCR(hcoldref);
			pg->pqflags |= PQ_TEST;
			break;
		}
		pg->pqflags &= ~PQ_REFERENCED;
		grp->pgrp_pdreact++;
		/* move to the list head */
		clockpro_pagerequeue(pg);
		dump("cold");
	}
done:;
	return pg;
}

void
uvmpdpol_pageactivate(struct vm_page *pg)
{

	if (!uvmpdpol_pageisqueued_p(pg)) {
		KASSERT((pg->pqflags & PQ_SPECULATIVE) == 0);
		pg->pqflags |= PQ_INITIALREF;
		clockpro_pageenqueue(pg);
	} else if ((pg->pqflags & PQ_SPECULATIVE)) {
		PDPOL_EVCNT_INCR(speculativehit1);
		pg->pqflags &= ~PQ_SPECULATIVE;
		pg->pqflags |= PQ_INITIALREF;
		clockpro_pagedequeue(pg);
		clockpro_pageenqueue(pg);
	}
	pg->pqflags |= PQ_REFERENCED;
}

void
uvmpdpol_pagedeactivate(struct vm_page *pg)
{

	clockpro_clearreferencebit(pg);
}

void
uvmpdpol_pagedequeue(struct vm_page *pg)
{

	if (!uvmpdpol_pageisqueued_p(pg)) {
		return;
	}
	clockpro_pagedequeue(pg);
	pg->pqflags &= ~(PQ_INITIALREF|PQ_SPECULATIVE);
}

void
uvmpdpol_pageenqueue(struct vm_page *pg)
{

#if 1
	if (uvmpdpol_pageisqueued_p(pg)) {
		return;
	}
	clockpro_clearreferencebit(pg);
	pg->pqflags |= PQ_SPECULATIVE;
	clockpro_pageenqueue(pg);
#else
	uvmpdpol_pageactivate(pg);
#endif
}

void
uvmpdpol_anfree(struct vm_anon *an)
{

	KASSERT(an->an_page == NULL);
	if (nonresident_lookupremove(NULL, (objid_t)an, 0)) {
		PDPOL_EVCNT_INCR(nresanonfree);
	}
}

void
uvmpdpol_init(void *new_gs, size_t npggroup)
{

	clockpro_init(new_gs, npggroup);
}

void
uvmpdpol_reinit(void)
{

	clockpro_reinit();
}

size_t
uvmpdpol_space(void)
{

	return sizeof(struct uvmpdpol_groupstate);
}

void
uvmpdpol_recolor(void *new_gs, struct uvm_pggroup *grparray,
	size_t npggroup, size_t old_ncolors)
{

	clockpro_recolor(new_gs, grparray, npggroup, old_ncolors);
}

void
uvmpdpol_estimatepageable(u_int *activep, u_int *inactivep)
{
	u_int active = 0;
	u_int inactive = 0;

	struct uvm_pggroup *grp;
	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
		active += gs->gs_npages - gs->gs_ncold;
		inactive += gs->gs_ncold;
	}
	if (activep) {
		*activep = active;
	}
	if (inactive) {
		*inactivep = inactive;
	}
}

bool
uvmpdpol_pageisqueued_p(struct vm_page *pg)
{

	return clockpro_getq(pg) != CLOCKPRO_NOQUEUE;
}

void
uvmpdpol_scaninit(struct uvm_pggroup *grp)
{

	grp->pgrp_gs->gs_nscanned = 0;
}

struct vm_page *
uvmpdpol_selectvictim(struct uvm_pggroup *grp)
{
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
	struct vm_page *pg;

	if (gs->gs_nscanned > gs->gs_npages) {
		DPRINTF("scan too much\n");
		return NULL;
	}
	pg = handcold_advance(gs);
	gs->gs_nscanned++;
	return pg;
}

static void
clockpro_dropswap(pageq_t *q, int *todo)
{
	struct vm_page *pg;

	TAILQ_FOREACH_REVERSE(pg, &q->q_q, pglist, pageq.queue) {
		if (*todo <= 0) {
			break;
		}
		if ((pg->pqflags & PQ_HOT) == 0) {
			continue;
		}
		if ((pg->pqflags & PQ_SWAPBACKED) == 0) {
			continue;
		}
		if (uvmpd_trydropswap(pg)) {
			(*todo)--;
		}
	}
}

void
uvmpdpol_balancequeue(struct uvm_pggroup *grp, u_int swap_shortage)
{
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
	u_int todo = swap_shortage;

	if (todo == 0) {
		return;
	}

	/*
	 * reclaim swap slots from hot pages
	 */

	DPRINTF("%s: [%zd] swap_shortage=%u\n",
	    __func__, grp - uvm.pggroups, swap_shortage);

	clockpro_dropswap(clockpro_queue(gs, CLOCKPRO_NEWQ), &todo);
	clockpro_dropswap(clockpro_queue(gs, CLOCKPRO_COLDQ(gs)), &todo);
	clockpro_dropswap(clockpro_queue(gs, CLOCKPRO_HOTQ(gs)), &todo);

	DPRINTF("%s: [%zd]: done=%u\n",
	    __func__, grp - uvm.pggroups, swap_shortage - todo);
}

bool
uvmpdpol_needsscan_p(struct uvm_pggroup *grp)
{
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;

	return (gs->gs_ncold < gs->gs_coldtarget);
}

void
uvmpdpol_tune(struct uvm_pggroup *grp)
{

	clockpro_tune(grp->pgrp_gs);
}

#if !defined(PDSIM)

#include <sys/sysctl.h>	/* XXX SYSCTL_DESCR */

void
uvmpdpol_sysctlsetup(void)
{
#if !defined(ADAPTIVE)
	struct clockpro_state * const s = &clockpro;

	uvm_pctparam_createsysctlnode(&s->s_coldtargetpct, "coldtargetpct",
	    SYSCTL_DESCR("Percentage cold target queue of the entire queue"));
#endif /* !defined(ADAPTIVE) */
}

#endif /* !defined(PDSIM) */

#if defined(DDB)

void clockpro_dump(void);

void
clockpro_dump(void)
{
	struct uvm_pggroup *grp;
	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		struct uvmpdpol_groupstate *gs = grp->pgrp_gs;
		struct vm_page *pg;
		int ncold, nhot, ntest, nspeculative, ninitialref, nref;
		int newqlen, coldqlen, hotqlen, listqlen;

		newqlen = coldqlen = hotqlen = listqlen = 0;
		printf(" [%zd]: npages=%d, ncold=%d, coldtarget=%d, newqlenmax=%d\n",
		    grp - uvm.pggroups, gs->gs_npages, gs->gs_ncold,
		    gs->gs_coldtarget, gs->gs_newqlenmax);

#define	INITCOUNT()	\
	ncold = nhot = ntest = nspeculative = ninitialref = nref = 0

#define	COUNT(pg)	\
	if ((pg->pqflags & PQ_HOT) != 0) { \
		nhot++; \
	} else { \
		ncold++; \
		if ((pg->pqflags & PQ_TEST) != 0) { \
			ntest++; \
		} \
		if ((pg->pqflags & PQ_SPECULATIVE) != 0) { \
			nspeculative++; \
		} \
		if ((pg->pqflags & PQ_INITIALREF) != 0) { \
			ninitialref++; \
		} else if ((pg->pqflags & PQ_REFERENCED) != 0 || \
		    pmap_is_referenced(pg)) { \
			nref++; \
		} \
	}

#define	PRINTCOUNT(name)	\
	printf("%s#%zd hot=%d, cold=%d, test=%d, speculative=%d, " \
	    "initialref=%d, nref=%d\n", \
	    (name), grp - uvm.pggroups, nhot, ncold, ntest, nspeculative, ninitialref, nref)

		INITCOUNT();
		TAILQ_FOREACH(pg, &clockpro_queue(gs, CLOCKPRO_NEWQ)->q_q, pageq.queue) {
			if (clockpro_getq(pg) != CLOCKPRO_NEWQ) {
				printf("newq corrupt %p\n", pg);
			}
			COUNT(pg)
			newqlen++;
		}
		PRINTCOUNT("newq");

		INITCOUNT();
		TAILQ_FOREACH(pg, &clockpro_queue(gs, CLOCKPRO_COLDQ(gs))->q_q, pageq.queue) {
			if (clockpro_getq(pg) != CLOCKPRO_COLDQ(gs)) {
				printf("coldq corrupt %p\n", pg);
			}
			COUNT(pg)
			coldqlen++;
		}
		PRINTCOUNT("coldq");

		INITCOUNT();
		TAILQ_FOREACH(pg, &clockpro_queue(gs, CLOCKPRO_HOTQ(gs))->q_q, pageq.queue) {
			if (clockpro_getq(pg) != CLOCKPRO_HOTQ(gs)) {
				printf("hotq corrupt %p\n", pg);
			}
#if defined(LISTQ)
			if ((pg->pqflags & PQ_HOT) == 0) {
				printf("cold page in hotq: %p\n", pg);
			}
#endif /* defined(LISTQ) */
			COUNT(pg)
			hotqlen++;
		}
		PRINTCOUNT("hotq");

		INITCOUNT();
		TAILQ_FOREACH(pg, &clockpro_queue(gs, CLOCKPRO_LISTQ)->q_q, pageq.queue) {
#if !defined(LISTQ)
			printf("listq %p\n", pg);
#endif /* !defined(LISTQ) */
			if (clockpro_getq(pg) != CLOCKPRO_LISTQ) {
				printf("listq corrupt %p\n", pg);
			}
			COUNT(pg)
			listqlen++;
		}
		PRINTCOUNT("listq");

		printf("#%zd: newqlen=%u/%u, coldqlen=%u/%u, hotqlen=%u/%u, listqlen=%d/%d\n",
		    grp - uvm.pggroups,
		    newqlen, pageq_len(clockpro_queue(gs, CLOCKPRO_NEWQ)),
		    coldqlen, pageq_len(clockpro_queue(gs, CLOCKPRO_COLDQ(gs))),
		    hotqlen, pageq_len(clockpro_queue(gs, CLOCKPRO_HOTQ(gs))),
		    listqlen, pageq_len(clockpro_queue(gs, CLOCKPRO_LISTQ)));
	}
}
#endif /* defined(DDB) */

#if defined(PDSIM)
#if defined(DEBUG)
static void
pdsim_dumpq(struct uvmpdpol_groupstate *gs, int qidx)
{
	pageq_t *q = clockpro_queue(gs, qidx);
	struct vm_page *pg;

	TAILQ_FOREACH(pg, &q->q_q, pageq.queue) {
		DPRINTF(" %" PRIu64 "%s%s%s%s%s%s",
		    pg->offset >> PAGE_SHIFT,
		    (pg->pqflags & PQ_HOT) ? "H" : "",
		    (pg->pqflags & PQ_TEST) ? "T" : "",
		    (pg->pqflags & PQ_REFERENCED) ? "R" : "",
		    pmap_is_referenced(pg) ? "r" : "",
		    (pg->pqflags & PQ_INITIALREF) ? "I" : "",
		    (pg->pqflags & PQ_SPECULATIVE) ? "S" : ""
		    );
	}
}
#endif /* defined(DEBUG) */

void
pdsim_dump(const char *id)
{
#if defined(DEBUG)
	struct clockpro_state * const s = &clockpro;

	DPRINTF("  %s L(", id);
	pdsim_dumpq(gs, CLOCKPRO_LISTQ);
	DPRINTF(" ) H(");
	pdsim_dumpq(gs, CLOCKPRO_HOTQ(gs));
	DPRINTF(" ) C(");
	pdsim_dumpq(gs, CLOCKPRO_COLDQ(gs));
	DPRINTF(" ) N(");
	pdsim_dumpq(gs, CLOCKPRO_NEWQ);
	DPRINTF(" ) ncold=%d/%d, coldadj=%d\n",
	    gs->gs_ncold, gs->gs_coldtarget, gs->gs_coldadj);
#endif /* defined(DEBUG) */
}
#endif /* defined(PDSIM) */
