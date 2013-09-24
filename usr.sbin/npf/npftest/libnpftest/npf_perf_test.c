/*	$NetBSD: npf_perf_test.c,v 1.1 2013/09/24 02:04:21 rmind Exp $	*/

/*
 * NPF benchmarking.
 *
 * Public Domain.
 */

#include <sys/types.h>
#include <sys/param.h>

#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>

#include "npf_impl.h"
#include "npf_test.h"

#define	NSECS		1 /* seconds */

static volatile int	run;
static volatile int	done;

static struct mbuf *
fill_packet(void)
{
	struct mbuf *m;
	struct ip *ip;
	struct tcphdr *th;

	m = mbuf_construct(IPPROTO_TCP);
	th = mbuf_return_hdrs(m, false, &ip);
	ip->ip_src.s_addr = inet_addr(PUB_IP1);
	ip->ip_dst.s_addr = inet_addr(LOCAL_IP3);
	th->th_sport = htons(80);
	th->th_dport = htons(15000);
	return m;
}

static void
worker(void *arg)
{
	ifnet_t *ifp = ifunit(IFNAME_INT);
	uint64_t n = 0, *npackets = arg;
	struct mbuf *m = fill_packet();

	while (!run)
		/* spin-wait */;
	while (!done) {
		int error;

		error = npf_packet_handler(NULL, &m, ifp, PFIL_OUT);
		KASSERT(error == 0);
		n++;
	}
	*npackets = n;
	kthread_exit(0);
}

void
npf_test_conc(unsigned nthreads)
{
	uint64_t total = 0, *npackets;
	int error;
	lwp_t **l;

	npackets = kmem_zalloc(sizeof(uint64_t) * nthreads, KM_SLEEP);
	l = kmem_zalloc(sizeof(lwp_t *) * nthreads, KM_SLEEP);

	printf("THREADS\tPKTS\n");
	done = false;
	run = false;

	for (unsigned i = 0; i < nthreads; i++) {
		const int flags = KTHREAD_MUSTJOIN | KTHREAD_MPSAFE;
		error = kthread_create(PRI_NONE, flags, NULL,
		    worker, &npackets[i], &l[i], "npfperf");
		KASSERT(error == 0);
	}

	/* Let them spin! */
	run = true;
	kpause("perf", false, NSECS * hz, NULL);
	done = true;

	/* Wait until all threads exit and sum the counts. */
	for (unsigned i = 0; i < nthreads; i++) {
		kthread_join(l[i]);
		total += npackets[i];
	}
	kmem_free(npackets, sizeof(uint64_t) * nthreads);
	kmem_free(l, sizeof(lwp_t *) * nthreads);

	printf("%u\t%" PRIu64 "\n", nthreads, total);
}
