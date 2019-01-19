/*
 * NPF connection tests.
 *
 * Public Domain.
 */

#ifdef _KERNEL
#include <sys/types.h>
#endif

#include "npf_impl.h"
#include "npf_conn.h"
#include "npf_test.h"

static bool	lverbose = false;

#define	CHECK_TRUE(x)	\
    if (!(x)) { printf("FAIL: %s line %d\n", __func__, __LINE__); return 0; }

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
fill_packet(unsigned i)
{
	struct mbuf *m;
	struct ip *ip;
	struct udphdr *uh;

	m = mbuf_construct(IPPROTO_UDP);
	uh = mbuf_return_hdrs(m, false, &ip);
	ip->ip_src.s_addr = inet_addr("10.0.0.1") + i;
	ip->ip_dst.s_addr = inet_addr("172.16.0.1");
	uh->uh_sport = htons(9000);
	uh->uh_dport = htons(9000);
	return m;
}

static bool
enqueue_connection(unsigned i, bool expire)
{
	ifnet_t *dummy_ifp = npf_test_addif(IFNAME_TEST, false, false);
	npf_cache_t npc = { .npc_info = 0, .npc_ctx = npf_getkernctx() };
	struct mbuf *m = fill_packet(i);
	npf_conn_t *con;
	nbuf_t nbuf;

	nbuf_init(npf_getkernctx(), &nbuf, m, dummy_ifp);
	npc.npc_nbuf = &nbuf;
	npf_cache_all(&npc);

	con = npf_conn_establish(&npc, PFIL_IN, true);
	CHECK_TRUE(con != NULL);
	if (expire) {
		npf_conn_expire(con);
	}
	npf_conn_release(con);
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

	return true;
}

bool
npf_conn_test(bool verbose)
{
	npf_t *npf = npf_getkernctx();
	npf_conndb_t *orig_cd = npf->conn_db;
	bool ok;

	lverbose = verbose;

	npf_config_enter(npf);
	npf_conn_tracking(npf, true);
	npf_config_exit(npf);

	ok = run_gc_tests();

	/* We *MUST* restore the valid conndb. */
	npf->conn_db = orig_cd;
	return ok;
}
