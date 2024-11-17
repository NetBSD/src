/*
 * NPF ruleset tests.
 *
 * Public Domain.
 */

#ifdef _KERNEL
#include <sys/types.h>
#endif

#include "npf_impl.h"
#include "npf_test.h"

#define	RESULT_PASS	0
#define	RESULT_BLOCK	ENETUNREACH

static const struct test_case {
	int		af;
	const char *	src;
	const char *	dst;
	const char *	ifname;
	int		di;
	int		stateful_ret;
	int		ret;
} test_cases[] = {

	/* Stateful pass. */
	{
		.af = AF_INET,
		.src = "10.1.1.1",		.dst = "10.1.1.2",
		.ifname = IFNAME_INT,		.di = PFIL_OUT,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{
		.af = AF_INET,
		.src = "10.1.1.2",		.dst = "10.1.1.1",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_BLOCK
	},

	/* Pass forwards stream only. */
	{
		.af = AF_INET,
		.src = "10.1.1.1",		.dst = "10.1.1.3",
		.ifname = IFNAME_INT,		.di = PFIL_OUT,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{
		.af = AF_INET,
		.src = "10.1.1.3",		.dst = "10.1.1.1",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},

	/*
	 * Pass in from any of the { fe80::1, fe80:1000:0:0/95,
	 * fe80::2, fe80::2000:0:0/96, fe80::3, fe80::3000:0:0/97 }
	 * group.
	 */
	{			/* fe80::1 */
		.af = AF_INET6,
		.src = "fe80::1", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::1000:0:0/95 */
		.af = AF_INET6,
		.src = "fe80::1001:0:0", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::1000:0:0/95, one bit off */
		.af = AF_INET6,
		.src = "fe80::1003:0:0", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},
	{			/* fe80::2 */
		.af = AF_INET6,
		.src = "fe80::2", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::2000:0:0/96 */
		.af = AF_INET6,
		.src = "fe80::2000:8000:0", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::2000:0:0/96, one bit off */
		.af = AF_INET6,
		.src = "fe80::2001:8000:0", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},
	{			/* fe80::3 */
		.af = AF_INET6,
		.src = "fe80::3", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::3000:0:0/97 */
		.af = AF_INET6,
		.src = "fe80::3000:7fff:0", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::3000:0:0/97, one bit off */
		.af = AF_INET6,
		.src = "fe80::3000:ffff:0", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},
	{
		.af = AF_INET6,
		.src = "fe80::4", .dst = "fe80::adec:c91c:d116:7592",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},

	/*
	 * Pass in from anywhere _not_ in that group, as long as it is
	 * to that group.
	 */
	{			/* fe80::1 */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::1",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::1000:0:0/95 */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::1001:0:0",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::1000:0:0/95, one bit off */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::1003:0:0",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},
	{			/* fe80::2 */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::2",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::2000:0:0/96 */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::2000:8000:0",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::2000:0:0/96, one bit off */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::2001:8000:0",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},
	{			/* fe80::3 */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::3",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::3000:0:0/97 */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::3000:7fff:0",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{			/* fe80::3000:0:0/97, one bit off */
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::3000:ffff:0",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},
	{
		.af = AF_INET6,
		.src = "fe80::adec:c91c:d116:7592", .dst = "fe80::4",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},

	/* Block. */
	{
		.af = AF_INET,
		.src = "10.1.1.1",		.dst = "10.1.1.4",
		.ifname = IFNAME_INT,		.di = PFIL_OUT,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},

};

static int
run_raw_testcase(unsigned i)
{
	const struct test_case *t = &test_cases[i];
	npf_t *npf = npf_getkernctx();
	npf_cache_t *npc;
	struct mbuf *m;
	npf_rule_t *rl;
	int slock, error;

	m = mbuf_get_pkt(t->af, IPPROTO_UDP, t->src, t->dst, 9000, 9000);
	npc = get_cached_pkt(m, t->ifname);

	slock = npf_config_read_enter(npf);
	rl = npf_ruleset_inspect(npc, npf_config_ruleset(npf), t->di, NPF_LAYER_3);
	if (rl) {
		npf_match_info_t mi;
		error = npf_rule_conclude(rl, &mi);
	} else {
		error = ENOENT;
	}
	npf_config_read_exit(npf, slock);

	put_cached_pkt(npc);
	return error;
}

static int
run_handler_testcase(unsigned i)
{
	const struct test_case *t = &test_cases[i];
	ifnet_t *ifp = npf_test_getif(t->ifname);
	npf_t *npf = npf_getkernctx();
	struct mbuf *m;
	int error;

	m = mbuf_get_pkt(t->af, IPPROTO_UDP, t->src, t->dst, 9000, 9000);
	error = npfk_packet_handler(npf, &m, ifp, t->di);
	if (m) {
		m_freem(m);
	}
	return error;
}

static npf_rule_t *
npf_blockall_rule(void)
{
	npf_t *npf = npf_getkernctx();
	nvlist_t *rule = nvlist_create(0);
	npf_rule_t *rl;

	nvlist_add_number(rule, "attr",
	    NPF_RULE_IN | NPF_RULE_OUT | NPF_RULE_DYNAMIC);
	rl = npf_rule_alloc(npf, rule);
	nvlist_destroy(rule);
	return rl;
}

static bool
test_static(bool verbose)
{
	for (unsigned i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		int error, serror;

		if (npf_test_getif(t->ifname) == NULL) {
			printf("Interface %s is not configured.\n", t->ifname);
			return false;
		}

		error = run_raw_testcase(i);
		serror = run_handler_testcase(i);

		if (verbose) {
			printf("rule test %d:\texpected %d (stateful) and %d\n"
			    "\t\t-> returned %d and %d\n",
			    i + 1, t->stateful_ret, t->ret, serror, error);
		}
		CHECK_TRUE(error == t->ret);
		CHECK_TRUE(serror == t->stateful_ret)
	}
	return true;
}

static bool
test_dynamic(void)
{
	npf_t *npf = npf_getkernctx();
	npf_ruleset_t *rlset;
	npf_rule_t *rl;
	uint64_t id;
	int error;

	/*
	 * Test dynamic NPF rules.
	 */

	error = run_raw_testcase(0);
	CHECK_TRUE(error == RESULT_PASS);

	npf_config_enter(npf);
	rlset = npf_config_ruleset(npf);

	rl = npf_blockall_rule();
	error = npf_ruleset_add(rlset, "test-rules", rl);
	CHECK_TRUE(error == 0);

	error = run_raw_testcase(0);
	CHECK_TRUE(error == RESULT_BLOCK);

	id = npf_rule_getid(rl);
	error = npf_ruleset_remove(rlset, "test-rules", id);
	CHECK_TRUE(error == 0);

	npf_config_exit(npf);

	error = run_raw_testcase(0);
	CHECK_TRUE(error == RESULT_PASS);

	return true;
}

bool
npf_rule_test(bool verbose)
{
	bool ok;

	ok = test_static(verbose);
	CHECK_TRUE(ok);

	ok = test_dynamic();
	CHECK_TRUE(ok);

	return true;
}
