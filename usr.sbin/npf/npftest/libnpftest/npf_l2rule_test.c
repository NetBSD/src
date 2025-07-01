/*
 * NPF layer 2 ruleset tests.
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
	const char *	src;
	const char *	dst;
	uint16_t		etype;
	const char *	ifname;
	int		di;
	int		ret;
} test_cases[] = {
	{
		/* pass ether in final from $mac1 to $mac2 type $E_IPv6 */
		.src = "00:00:5E:00:53:00",	.dst = "00:00:5E:00:53:01",
		.ifname = IFNAME_INT,		.etype = htons(ETHERTYPE_IPV6),
		.di = PFIL_IN,			.ret = RESULT_PASS
	},
	{
		/* block ether in final from $mac2 */
		.src = "00:00:5E:00:53:01",	.dst = "00:00:5E:00:53:02",
		.ifname = IFNAME_INT,		.etype = htons(ETHERTYPE_IP),
		.di = PFIL_IN,			.ret = RESULT_BLOCK
	},

		/* pass ether out final to $mac3 $Apple talk */
	{
		.src = "00:00:5E:00:53:00",	.dst = "00:00:5E:00:53:02",
		.ifname = IFNAME_INT,		.etype = htons(ETHERTYPE_ATALK),
		.di = PFIL_OUT,			.ret = RESULT_PASS
	},
	{
		/* goto default: block all (since direction is not matching ) */
		.src = "00:00:5E:00:53:00",	.dst = "00:00:5E:00:53:02",
		.ifname = IFNAME_INT,		.etype = htons(ETHERTYPE_IP),
		.di = PFIL_IN,			.ret = RESULT_BLOCK
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

	m = mbuf_get_frame(t->src, t->dst, t->etype);
	npc = get_cached_pkt(m, t->ifname, NPF_RULE_LAYER_2);

	slock = npf_config_read_enter(npf);
	rl = npf_ruleset_inspect(npc, npf_config_ruleset(npf), t->di, NPF_RULE_LAYER_2);
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

/* for dynamic testing */
static int
run_handler_testcase(unsigned i)
{
	const struct test_case *t = &test_cases[i];
	ifnet_t *ifp = npf_test_getif(t->ifname);
	npf_t *npf = npf_getkernctx();
	struct mbuf *m;
	int error;

	m = mbuf_get_frame(t->src, t->dst, t->etype);
	error = npfk_layer2_handler(npf, &m, ifp, t->di);
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
		NPF_RULE_IN | NPF_RULE_OUT | NPF_RULE_DYNAMIC | NPF_RULE_LAYER_2);
	rl = npf_rule_alloc(npf, rule);
	nvlist_destroy(rule);
	return rl;
}

static bool
test_static(bool verbose)
{
	for (unsigned i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		int error;

		if (npf_test_getif(t->ifname) == NULL) {
			printf("Interface %s is not configured.\n", t->ifname);
			return false;
		}

		error = run_handler_testcase(i);

		if (verbose) {
			printf("rule test %d:\texpected %d\n"
				"\t\t-> returned %d\n",
				i + 1, t->ret, error);
		}
		CHECK_TRUE(error == t->ret);
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
	error = npf_ruleset_add(rlset, "l2-ruleset", rl);
	CHECK_TRUE(error == 0);

	error = run_raw_testcase(0);
	CHECK_TRUE(error == RESULT_BLOCK);

	id = npf_rule_getid(rl);
	error = npf_ruleset_remove(rlset, "l2-ruleset", id);
	CHECK_TRUE(error == 0);

	npf_config_exit(npf);

	error = run_raw_testcase(0);
	CHECK_TRUE(error == RESULT_PASS);

	return true;
}

bool
npf_layer2_rule_test(bool verbose)
{
	bool ok;

	ok = test_static(verbose);
	CHECK_TRUE(ok);

	ok = test_dynamic();
	CHECK_TRUE(ok);

	return true;
}
