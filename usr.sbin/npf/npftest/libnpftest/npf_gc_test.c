/*
 * NPF connection tests.
 *
 * Public Domain.
 */

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/kmem.h>
#endif

#include "npf.h"
#include "npf_impl.h"
#include "npf_conn.h"
#include "npf_test.h"

static bool	lverbose = false;

static unsigned
count_conns(npf_conndb_t *cd)
{
	npf_conn_t *head = npf_conndb_getlist(cd), *conn = head;
	unsigned n = 0;

	while (conn) {
		n++;
		conn = npf_conndb_getnext(cd, conn);
		if (conn == head) {
			break;
		}
	}
	return n;
}

static struct mbuf *
get_packet(unsigned i)
{
	struct mbuf *m;
	struct ip *ip;

	m = mbuf_get_pkt(AF_INET, IPPROTO_UDP,
	    "10.0.0.1", "172.16.0.1", 9000, 9000);
	(void)mbuf_return_hdrs(m, false, &ip);
	ip->ip_src.s_addr += i;
	return m;
}

static bool
enqueue_connection(unsigned i, bool expire)
{
	struct mbuf *m = get_packet(i);
	npf_cache_t *npc = get_cached_pkt(m, NULL);
	npf_conn_t *con;

	con = npf_conn_establish(npc, PFIL_IN, true);
	CHECK_TRUE(con != NULL);
	if (expire) {
		npf_conn_expire(con);
	}
	npf_conn_release(con);
	put_cached_pkt(npc);
	return true;
}

static bool
run_conn_gc(unsigned active, unsigned expired, unsigned expected)
{
	npf_t *npf = npf_getkernctx();
	npf_conndb_t *cd = npf_conndb_create();
	unsigned total, n = 0;

	npf->conn_db = cd;

	/*
	 * Insert the given number of active and expired connections..
	 */
	total = active + expired;

	while (active || expired) {
		if (active) {
			enqueue_connection(n++, false);
			active--;
		}
		if (expired) {
			enqueue_connection(n++, true);
			expired--;
		}
	}

	/* Verify the count. */
	n = count_conns(cd);
	CHECK_TRUE(n == total);

	/*
	 * Run G/C.  Check the remaining.
	 */
	npf_conndb_gc(npf, cd, false, false);
	n = count_conns(cd);
	if (lverbose) {
		printf("in conndb -- %u (expected %u)\n", n, expected);
	}
	CHECK_TRUE(n == expected);

	/* Flush and destroy. */
	npf_conndb_gc(npf, cd, true, false);
	npf_conndb_destroy(cd);
	npf->conn_db = NULL;
	return true;
}

static bool
run_gc_tests(void)
{
	bool ok;
	int val;

	/* Check the default value. */
	npfk_param_get(npf_getkernctx(), "gc.step", &val);
	CHECK_TRUE(val == 256);

	/* Empty => GC => 0 in conndb. */
	ok = run_conn_gc(0, 0, 0);
	CHECK_TRUE(ok);

	/* 1 active => GC => 1 in conndb. */
	ok = run_conn_gc(1, 0, 1);
	CHECK_TRUE(ok);

	/* 1 expired => GC => 0 in conndb. */
	ok = run_conn_gc(0, 1, 0);
	CHECK_TRUE(ok);

	/* 1 active and 1 expired => GC => 1 in conndb. */
	ok = run_conn_gc(1, 1, 1);
	CHECK_TRUE(ok);

	/* 2 expired => GC => 0 in conndb. */
	ok = run_conn_gc(0, 2, 0);
	CHECK_TRUE(ok);

	/* 128 expired => GC => 0 in conndb. */
	ok = run_conn_gc(0, 128, 0);
	CHECK_TRUE(ok);

	/* 512 expired => GC => 256 in conndb. */
	ok = run_conn_gc(0, 512, 256);
	CHECK_TRUE(ok);

	/* 512 expired => GC => 127 in conndb. */
	npfk_param_set(npf_getkernctx(), "gc.step", 128);
	ok = run_conn_gc(0, 512, 384);
	CHECK_TRUE(ok);

	return true;
}

static bool
run_conndb_tests(npf_t *npf)
{
	npf_conndb_t *orig_cd = npf->conn_db;
	bool ok;

	npf_config_enter(npf);
	npf_conn_tracking(npf, true);
	npf_config_exit(npf);

	ok = run_gc_tests();

	/* We *MUST* restore the valid conndb. */
	npf->conn_db = orig_cd;
	return ok;
}


static void
worker_test_task(npf_t *npf)
{
	bool *done = atomic_load_acquire(&npf->arg);
	atomic_store_release(done, true);
}

static bool
run_worker_tests(npf_t *npf)
{
	unsigned n = 100;
	int error;

	/* Spawn a worker thread. */
	error = npf_worker_sysinit(1);
	assert(error == 0);

	/*
	 * Enlist/discharge an instance, trying to trigger a race.
	 */
	while (n--) {
		bool task_done = false;
		unsigned retry = 100;
		npf_t *test_npf;

		/*
		 * Initialize a dummy NPF instance and add a test task.
		 * We will (ab)use npf_t::arg here.
		 *
		 * XXX/TODO: We should use:
		 *
		 *	npfk_create(NPF_NO_GC, &npftest_mbufops,
		 *	    &npftest_ifops, &task_done);
		 *
		 * However, it resets the interface state and breaks
		 * other tests; to be refactor.
		 */
		test_npf = kmem_zalloc(sizeof(npf_t), KM_SLEEP);
		atomic_store_release(&test_npf->arg, &task_done);
		test_npf->ebr = npf_ebr_create();

		error = npf_worker_addfunc(test_npf, worker_test_task);
		assert(error == 0);

		/* Enlist the NPF instance. */
		npf_worker_enlist(test_npf);

		/* Wait for the task to be done. */
		while (!atomic_load_acquire(&task_done) && retry--) {
			npf_worker_signal(test_npf);
			kpause("gctest", false, MAX(1, mstohz(1)), NULL);
		}

		CHECK_TRUE(atomic_load_acquire(&task_done));
		npf_worker_discharge(test_npf);

		/* Clear the parameter and signal again. */
		atomic_store_release(&test_npf->arg, NULL);
		npf_worker_signal(test_npf);

		npf_ebr_destroy(test_npf->ebr);
		kmem_free(test_npf, sizeof(npf_t)); // npfk_destroy()
	}

	/*
	 * Destroy the worker.
	 *
	 * Attempts to enlist, discharge or signal should have no effect.
	 */

	npf_worker_sysfini();
	npf_worker_enlist(npf);
	npf_worker_signal(npf);
	npf_worker_discharge(npf);
	return true;
}

bool
npf_gc_test(bool verbose)
{
	npf_t *npf = npf_getkernctx();
	bool ok;

	lverbose = verbose;

	ok = run_conndb_tests(npf);
	CHECK_TRUE(ok);

	ok = run_worker_tests(npf);
	CHECK_TRUE(ok);

	return ok;
}
