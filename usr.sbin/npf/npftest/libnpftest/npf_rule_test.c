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

#define	CHECK_TRUE(x)	\
    if (!(x)) { printf("FAIL: %s line %d\n", __func__, __LINE__); return 0; }

#define	RESULT_PASS	0
#define	RESULT_BLOCK	ENETUNREACH

static const struct test_case {
	const char *	src;
	const char *	dst;
	const char *	ifname;
	int		di;
	int		stateful_ret;
	int		ret;
} test_cases[] = {

	/* Stateful pass. */
	{
		.src = "10.1.1.1",		.dst = "10.1.1.2",
		.ifname = IFNAME_INT,		.di = PFIL_OUT,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{
		.src = "10.1.1.2",		.dst = "10.1.1.1",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_BLOCK
	},

	/* Pass forwards stream only. */
	{
		.src = "10.1.1.1",		.dst = "10.1.1.3",
		.ifname = IFNAME_INT,		.di = PFIL_OUT,
		.stateful_ret = RESULT_PASS,	.ret = RESULT_PASS
	},
	{
		.src = "10.1.1.3",		.dst = "10.1.1.1",
		.ifname = IFNAME_INT,		.di = PFIL_IN,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},

	/* Block. */
	{	.src = "10.1.1.1",		.dst = "10.1.1.4",
		.ifname = IFNAME_INT,		.di = PFIL_OUT,
		.stateful_ret = RESULT_BLOCK,	.ret = RESULT_BLOCK
	},

};

static struct mbuf *
fill_packet(const struct test_case *t)
{
	struct mbuf *m;
	struct ip *ip;
	struct udphdr *uh;

	m = mbuf_construct(IPPROTO_UDP);
	uh = mbuf_return_hdrs(m, false, &ip);
	ip->ip_src.s_addr = inet_addr(t->src);
	ip->ip_dst.s_addr = inet_addr(t->dst);
	uh->uh_sport = htons(9000);
	uh->uh_dport = htons(9000);
	return m;
}

static int
npf_rule_raw_test(struct mbuf *m, ifnet_t *ifp, int di)
{
	npf_t *npf = npf_getkernctx();
	npf_cache_t npc = { .npc_info = 0, .npc_ctx = npf };
	nbuf_t nbuf;
	npf_rule_t *rl;
	npf_match_info_t mi;
	int error;

	nbuf_init(npf, &nbuf, m, ifp);
	npc.npc_nbuf = &nbuf;
	npf_cache_all(&npc);

	int slock = npf_config_read_enter();
	rl = npf_ruleset_inspect(&npc, npf_config_ruleset(npf),
	    di, NPF_LAYER_3);
	if (rl) {
		error = npf_rule_conclude(rl, &mi);
	} else {
		error = ENOENT;
	}
	npf_config_read_exit(slock);
	return error;
}

static int
npf_test_case(unsigned i)
{
	const struct test_case *t = &test_cases[i];
	ifnet_t *ifp = npf_test_getif(t->ifname);
	int error;

	struct mbuf *m = fill_packet(t);
	error = npf_rule_raw_test(m, ifp, t->di);
	m_freem(m);
	return error;
}

static npf_rule_t *
npf_blockall_rule(void)
{
	npf_t *npf = npf_getkernctx();
	nvlist_t *rule = nvlist_create(0);

	nvlist_add_number(rule, "attr",
	    NPF_RULE_IN | NPF_RULE_OUT | NPF_RULE_DYNAMIC);
	return npf_rule_alloc(npf, rule);
}

bool
npf_rule_test(bool verbose)
{
	npf_t *npf = npf_getkernctx();
	npf_ruleset_t *rlset;
	npf_rule_t *rl;
	uint64_t id;
	int error;

	for (unsigned i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		ifnet_t *ifp = npf_test_getif(t->ifname);
		int serror;

		if (ifp == NULL) {
			printf("Interface %s is not configured.\n", t->ifname);
			return false;
		}

		struct mbuf *m = fill_packet(t);
		error = npf_rule_raw_test(m, ifp, t->di);
		serror = npf_packet_handler(npf, &m, ifp, t->di);

		if (m) {
			m_freem(m);
		}

		if (verbose) {
			printf("rule test %d:\texpected %d (stateful) and %d\n"
			    "\t\t-> returned %d and %d\n",
			    i + 1, t->stateful_ret, t->ret, serror, error);
		}
		CHECK_TRUE(serror == t->stateful_ret && error == t->ret);
	}

	/*
	 * Test dynamic NPF rules.
	 */

	error = npf_test_case(0);
	CHECK_TRUE(error == RESULT_PASS);

	npf_config_enter(npf);
	rlset = npf_config_ruleset(npf);

	rl = npf_blockall_rule();
	error = npf_ruleset_add(rlset, "test-rules", rl);
	CHECK_TRUE(error == 0);

	error = npf_test_case(0);
	CHECK_TRUE(error == RESULT_BLOCK);

	id = npf_rule_getid(rl);
	error = npf_ruleset_remove(rlset, "test-rules", id);
	CHECK_TRUE(error == 0);

	npf_config_exit(npf);

	error = npf_test_case(0);
	CHECK_TRUE(error == RESULT_PASS);

	return true;
}
