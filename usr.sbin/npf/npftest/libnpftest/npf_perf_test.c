/*
 * NPF benchmarking.
 *
 * Public Domain.
 */

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/param.h>

#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#endif

#include "npf_impl.h"
#include "npf_test.h"

#define	NSECS		10 /* seconds */

static volatile int	run;
static volatile int	done;

static uint64_t *	npackets;
static bool		stateful;

__dead static void
worker(void *arg)
{
	npf_t *npf = npf_getkernctx();
	ifnet_t *ifp = npf_test_getif(IFNAME_INT);
	unsigned int i = (uintptr_t)arg;
	struct mbuf *m;
	uint64_t n = 0;

	m = mbuf_get_pkt(AF_INET, IPPROTO_UDP,
	    PUB_IP1, stateful ? LOCAL_IP2 : LOCAL_IP3,
	    80, 15000 + i);

	while (!run)
		/* spin-wait */;
	while (!done) {
		int error;

		error = npfk_packet_handler(npf, &m, ifp, PFIL_OUT);
		KASSERT(error == 0); (void)error;
		n++;
	}
	npackets[i] = n;
	m_freem(m);
	kthread_exit(0);
}

void
npf_test_conc(bool st, unsigned nthreads)
{
	uint64_t total = 0;
	int error;
	lwp_t **l;

	printf("THREADS\tPKTS\n");
	stateful = st;
	done = false;
	run = false;

	npackets = kmem_zalloc(sizeof(uint64_t) * nthreads, KM_SLEEP);
	l = kmem_zalloc(sizeof(lwp_t *) * nthreads, KM_SLEEP);

	for (unsigned i = 0; i < nthreads; i++) {
		error = kthread_create(PRI_NONE, KTHREAD_MUSTJOIN |
		    KTHREAD_MPSAFE, NULL, worker, (void *)(uintptr_t)i,
		    &l[i], "npfperf");
		KASSERT(error == 0); (void)error;
	}

	/* Let them spin! */
	run = true;
	kpause("perf", false, mstohz(NSECS * 1000), NULL);
	done = true;

	/* Wait until all threads exit and sum the counts. */
	for (unsigned i = 0; i < nthreads; i++) {
		kthread_join(l[i]);
		total += npackets[i];
	}
	kmem_free(npackets, sizeof(uint64_t) * nthreads);
	kmem_free(l, sizeof(lwp_t *) * nthreads);

	printf("%u\t%" PRIu64 "\n", nthreads, total / NSECS);
}
