/*	$NetBSD: pktqueue.c,v 1.16 2021/12/21 04:09:32 knakahara Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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

/*
 * The packet queue (pktqueue) interface is a lockless IP input queue
 * which also abstracts and handles network ISR scheduling.  It provides
 * a mechanism to enable receiver-side packet steering (RPS).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pktqueue.c,v 1.16 2021/12/21 04:09:32 knakahara Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/pcq.h>
#include <sys/intr.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/percpu.h>
#include <sys/xcall.h>

#include <net/pktqueue.h>
#include <net/rss_config.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

struct pktqueue {
	/*
	 * The lock used for a barrier mechanism.  The barrier counter,
	 * as well as the drop counter, are managed atomically though.
	 * Ensure this group is in a separate cache line.
	 */
	union {
		struct {
			kmutex_t	pq_lock;
			volatile u_int	pq_barrier;
		};
		uint8_t	 _pad[COHERENCY_UNIT];
	};

	/* The size of the queue, counters and the interrupt handler. */
	u_int		pq_maxlen;
	percpu_t *	pq_counters;
	void *		pq_sih;

	/* Finally, per-CPU queues. */
	struct percpu *	pq_pcq;	/* struct pcq * */
};

/* The counters of the packet queue. */
#define	PQCNT_ENQUEUE	0
#define	PQCNT_DEQUEUE	1
#define	PQCNT_DROP	2
#define	PQCNT_NCOUNTERS	3

typedef struct {
	uint64_t	count[PQCNT_NCOUNTERS];
} pktq_counters_t;

/* Special marker value used by pktq_barrier() mechanism. */
#define	PKTQ_MARKER	((void *)(~0ULL))

static void
pktq_init_cpu(void *vqp, void *vpq, struct cpu_info *ci)
{
	struct pcq **qp = vqp;
	struct pktqueue *pq = vpq;

	*qp = pcq_create(pq->pq_maxlen, KM_SLEEP);
}

static void
pktq_fini_cpu(void *vqp, void *vpq, struct cpu_info *ci)
{
	struct pcq **qp = vqp, *q = *qp;

	KASSERT(pcq_peek(q) == NULL);
	pcq_destroy(q);
	*qp = NULL;		/* paranoia */
}

static struct pcq *
pktq_pcq(struct pktqueue *pq, struct cpu_info *ci)
{
	struct pcq **qp, *q;

	/*
	 * As long as preemption is disabled, the xcall to swap percpu
	 * buffers can't complete, so it is safe to read the pointer.
	 */
	KASSERT(kpreempt_disabled());

	qp = percpu_getptr_remote(pq->pq_pcq, ci);
	q = *qp;

	return q;
}

pktqueue_t *
pktq_create(size_t maxlen, void (*intrh)(void *), void *sc)
{
	const u_int sflags = SOFTINT_NET | SOFTINT_MPSAFE | SOFTINT_RCPU;
	pktqueue_t *pq;
	percpu_t *pc;
	void *sih;

	pc = percpu_alloc(sizeof(pktq_counters_t));
	if ((sih = softint_establish(sflags, intrh, sc)) == NULL) {
		percpu_free(pc, sizeof(pktq_counters_t));
		return NULL;
	}

	pq = kmem_zalloc(sizeof(*pq), KM_SLEEP);
	mutex_init(&pq->pq_lock, MUTEX_DEFAULT, IPL_NONE);
	pq->pq_maxlen = maxlen;
	pq->pq_counters = pc;
	pq->pq_sih = sih;
	pq->pq_pcq = percpu_create(sizeof(struct pcq *),
	    pktq_init_cpu, pktq_fini_cpu, pq);

	return pq;
}

void
pktq_destroy(pktqueue_t *pq)
{

	percpu_free(pq->pq_pcq, sizeof(struct pcq *));
	percpu_free(pq->pq_counters, sizeof(pktq_counters_t));
	softint_disestablish(pq->pq_sih);
	mutex_destroy(&pq->pq_lock);
	kmem_free(pq, sizeof(*pq));
}

/*
 * - pktq_inc_counter: increment the counter given an ID.
 * - pktq_collect_counts: handler to sum up the counts from each CPU.
 * - pktq_getcount: return the effective count given an ID.
 */

static inline void
pktq_inc_count(pktqueue_t *pq, u_int i)
{
	percpu_t *pc = pq->pq_counters;
	pktq_counters_t *c;

	c = percpu_getref(pc);
	c->count[i]++;
	percpu_putref(pc);
}

static void
pktq_collect_counts(void *mem, void *arg, struct cpu_info *ci)
{
	const pktq_counters_t *c = mem;
	pktq_counters_t *sum = arg;

	int s = splnet();

	for (u_int i = 0; i < PQCNT_NCOUNTERS; i++) {
		sum->count[i] += c->count[i];
	}

	splx(s);
}

uint64_t
pktq_get_count(pktqueue_t *pq, pktq_count_t c)
{
	pktq_counters_t sum;

	if (c != PKTQ_MAXLEN) {
		memset(&sum, 0, sizeof(sum));
		percpu_foreach_xcall(pq->pq_counters,
		    XC_HIGHPRI_IPL(IPL_SOFTNET), pktq_collect_counts, &sum);
	}
	switch (c) {
	case PKTQ_NITEMS:
		return sum.count[PQCNT_ENQUEUE] - sum.count[PQCNT_DEQUEUE];
	case PKTQ_DROPS:
		return sum.count[PQCNT_DROP];
	case PKTQ_MAXLEN:
		return pq->pq_maxlen;
	}
	return 0;
}

uint32_t
pktq_rps_hash(pktq_rps_hash_func_t *funcp, const struct mbuf *m)
{
	pktq_rps_hash_func_t func = atomic_load_relaxed(funcp);

	KASSERT(func != NULL);

	return (*func)(m);
}

static uint32_t
pktq_rps_hash_zero(const struct mbuf *m __unused)
{

	return 0;
}

static uint32_t
pktq_rps_hash_curcpu(const struct mbuf *m __unused)
{

	return cpu_index(curcpu());
}

static uint32_t
pktq_rps_hash_toeplitz(const struct mbuf *m)
{
	struct ip *ip;
	/*
	 * Disable UDP port - IP fragments aren't currently being handled
	 * and so we end up with a mix of 2-tuple and 4-tuple
	 * traffic.
	 */
	const u_int flag = RSS_TOEPLITZ_USE_TCP_PORT;

	/* glance IP version */
	if ((m->m_flags & M_PKTHDR) == 0)
		return 0;

	ip = mtod(m, struct ip *);
	if (ip->ip_v == IPVERSION) {
		if (__predict_false(m->m_len < sizeof(struct ip)))
			return 0;
		return rss_toeplitz_hash_from_mbuf_ipv4(m, flag);
	} else if (ip->ip_v == 6) {
		if (__predict_false(m->m_len < sizeof(struct ip6_hdr)))
			return 0;
		return rss_toeplitz_hash_from_mbuf_ipv6(m, flag);
	}

	return 0;
}

/*
 * toeplitz without curcpu.
 * Generally, this has better performance than toeplitz.
 */
static uint32_t
pktq_rps_hash_toeplitz_othercpus(const struct mbuf *m)
{
	uint32_t hash;

	if (ncpu == 1)
		return 0;

	hash = pktq_rps_hash_toeplitz(m);
	hash %= ncpu - 1;
	if (hash >= cpu_index(curcpu()))
		return hash + 1;
	else
		return hash;
}

static struct pktq_rps_hash_table {
	const char* prh_type;
	pktq_rps_hash_func_t prh_func;
} const pktq_rps_hash_tab[] = {
	{ "zero", pktq_rps_hash_zero },
	{ "curcpu", pktq_rps_hash_curcpu },
	{ "toeplitz", pktq_rps_hash_toeplitz },
	{ "toeplitz-othercpus", pktq_rps_hash_toeplitz_othercpus },
};
const pktq_rps_hash_func_t pktq_rps_hash_default =
#ifdef NET_MPSAFE
	pktq_rps_hash_curcpu;
#else
	pktq_rps_hash_zero;
#endif

static const char *
pktq_get_rps_hash_type(pktq_rps_hash_func_t func)
{

	for (int i = 0; i < __arraycount(pktq_rps_hash_tab); i++) {
		if (func == pktq_rps_hash_tab[i].prh_func) {
			return pktq_rps_hash_tab[i].prh_type;
		}
	}

	return NULL;
}

static int
pktq_set_rps_hash_type(pktq_rps_hash_func_t *func, const char *type)
{

	if (strcmp(type, pktq_get_rps_hash_type(*func)) == 0)
		return 0;

	for (int i = 0; i < __arraycount(pktq_rps_hash_tab); i++) {
		if (strcmp(type, pktq_rps_hash_tab[i].prh_type) == 0) {
			atomic_store_relaxed(func, pktq_rps_hash_tab[i].prh_func);
			return 0;
		}
	}

	return ENOENT;
}

int
sysctl_pktq_rps_hash_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	pktq_rps_hash_func_t *func;
	int error;
	char type[PKTQ_RPS_HASH_NAME_LEN];

	node = *rnode;
	func = node.sysctl_data;

	strlcpy(type, pktq_get_rps_hash_type(*func), PKTQ_RPS_HASH_NAME_LEN);

	node.sysctl_data = &type;
	node.sysctl_size = sizeof(type);
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	error = pktq_set_rps_hash_type(func, type);

	return error;
 }

/*
 * pktq_enqueue: inject the packet into the end of the queue.
 *
 * => Must be called from the interrupt or with the preemption disabled.
 * => Consumes the packet and returns true on success.
 * => Returns false on failure; caller is responsible to free the packet.
 */
bool
pktq_enqueue(pktqueue_t *pq, struct mbuf *m, const u_int hash __unused)
{
#if defined(_RUMPKERNEL) || defined(_RUMP_NATIVE_ABI)
	struct cpu_info *ci = curcpu();
#else
	struct cpu_info *ci = cpu_lookup(hash % ncpu);
#endif

	KASSERT(kpreempt_disabled());

	if (__predict_false(!pcq_put(pktq_pcq(pq, ci), m))) {
		pktq_inc_count(pq, PQCNT_DROP);
		return false;
	}
	softint_schedule_cpu(pq->pq_sih, ci);
	pktq_inc_count(pq, PQCNT_ENQUEUE);
	return true;
}

/*
 * pktq_dequeue: take a packet from the queue.
 *
 * => Must be called with preemption disabled.
 * => Must ensure there are not concurrent dequeue calls.
 */
struct mbuf *
pktq_dequeue(pktqueue_t *pq)
{
	struct cpu_info *ci = curcpu();
	struct mbuf *m;

	KASSERT(kpreempt_disabled());

	m = pcq_get(pktq_pcq(pq, ci));
	if (__predict_false(m == PKTQ_MARKER)) {
		/* Note the marker entry. */
		atomic_inc_uint(&pq->pq_barrier);
		return NULL;
	}
	if (__predict_true(m != NULL)) {
		pktq_inc_count(pq, PQCNT_DEQUEUE);
	}
	return m;
}

/*
 * pktq_barrier: waits for a grace period when all packets enqueued at
 * the moment of calling this routine will be processed.  This is used
 * to ensure that e.g. packets referencing some interface were drained.
 */
void
pktq_barrier(pktqueue_t *pq)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	u_int pending = 0;

	mutex_enter(&pq->pq_lock);
	KASSERT(pq->pq_barrier == 0);

	for (CPU_INFO_FOREACH(cii, ci)) {
		struct pcq *q;

		kpreempt_disable();
		q = pktq_pcq(pq, ci);
		kpreempt_enable();

		/* If the queue is empty - nothing to do. */
		if (pcq_peek(q) == NULL) {
			continue;
		}
		/* Otherwise, put the marker and entry. */
		while (!pcq_put(q, PKTQ_MARKER)) {
			kpause("pktqsync", false, 1, NULL);
		}
		kpreempt_disable();
		softint_schedule_cpu(pq->pq_sih, ci);
		kpreempt_enable();
		pending++;
	}

	/* Wait for each queue to process the markers. */
	while (pq->pq_barrier != pending) {
		kpause("pktqsync", false, 1, NULL);
	}
	pq->pq_barrier = 0;
	mutex_exit(&pq->pq_lock);
}

/*
 * pktq_flush: free mbufs in all queues.
 *
 * => The caller must ensure there are no concurrent writers or flush calls.
 */
void
pktq_flush(pktqueue_t *pq)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct mbuf *m;

	for (CPU_INFO_FOREACH(cii, ci)) {
		struct pcq *q;

		kpreempt_disable();
		q = pktq_pcq(pq, ci);
		kpreempt_enable();

		/*
		 * XXX This can't be right -- if the softint is running
		 * then pcq_get isn't safe here.
		 */
		while ((m = pcq_get(q)) != NULL) {
			pktq_inc_count(pq, PQCNT_DEQUEUE);
			m_freem(m);
		}
	}
}

static void
pktq_set_maxlen_cpu(void *vpq, void *vqs)
{
	struct pktqueue *pq = vpq;
	struct pcq **qp, *q, **qs = vqs;
	unsigned i = cpu_index(curcpu());
	int s;

	s = splnet();
	qp = percpu_getref(pq->pq_pcq);
	q = *qp;
	*qp = qs[i];
	qs[i] = q;
	percpu_putref(pq->pq_pcq);
	splx(s);
}

/*
 * pktq_set_maxlen: create per-CPU queues using a new size and replace
 * the existing queues without losing any packets.
 *
 * XXX ncpu must remain stable throughout.
 */
int
pktq_set_maxlen(pktqueue_t *pq, size_t maxlen)
{
	const u_int slotbytes = ncpu * sizeof(pcq_t *);
	pcq_t **qs;

	if (!maxlen || maxlen > PCQ_MAXLEN)
		return EINVAL;
	if (pq->pq_maxlen == maxlen)
		return 0;

	/* First, allocate the new queues. */
	qs = kmem_zalloc(slotbytes, KM_SLEEP);
	for (u_int i = 0; i < ncpu; i++) {
		qs[i] = pcq_create(maxlen, KM_SLEEP);
	}

	/*
	 * Issue an xcall to replace the queue pointers on each CPU.
	 * This implies all the necessary memory barriers.
	 */
	mutex_enter(&pq->pq_lock);
	xc_wait(xc_broadcast(XC_HIGHPRI, pktq_set_maxlen_cpu, pq, qs));
	pq->pq_maxlen = maxlen;
	mutex_exit(&pq->pq_lock);

	/*
	 * At this point, the new packets are flowing into the new
	 * queues.  However, the old queues may have some packets
	 * present which are no longer being processed.  We are going
	 * to re-enqueue them.  This may change the order of packet
	 * arrival, but it is not considered an issue.
	 *
	 * There may be in-flight interrupts calling pktq_dequeue()
	 * which reference the old queues.  Issue a barrier to ensure
	 * that we are going to be the only pcq_get() callers on the
	 * old queues.
	 */
	pktq_barrier(pq);

	for (u_int i = 0; i < ncpu; i++) {
		struct pcq *q;
		struct mbuf *m;

		kpreempt_disable();
		q = pktq_pcq(pq, cpu_lookup(i));
		kpreempt_enable();

		while ((m = pcq_get(qs[i])) != NULL) {
			while (!pcq_put(q, m)) {
				kpause("pktqrenq", false, 1, NULL);
			}
		}
		pcq_destroy(qs[i]);
	}

	/* Well, that was fun. */
	kmem_free(qs, slotbytes);
	return 0;
}

int
sysctl_pktq_maxlen(SYSCTLFN_ARGS, pktqueue_t *pq)
{
	u_int nmaxlen = pktq_get_count(pq, PKTQ_MAXLEN);
	struct sysctlnode node = *rnode;
	int error;

	node.sysctl_data = &nmaxlen;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	return pktq_set_maxlen(pq, nmaxlen);
}

int
sysctl_pktq_count(SYSCTLFN_ARGS, pktqueue_t *pq, u_int count_id)
{
	uint64_t count = pktq_get_count(pq, count_id);
	struct sysctlnode node = *rnode;

	node.sysctl_data = &count;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}
