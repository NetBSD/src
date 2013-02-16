/*	$NetBSD: npf_rule_test.c,v 1.6 2013/02/16 21:11:16 rmind Exp $	*/

/*
 * NPF ruleset test.
 *
 * Public Domain.
 */

#include <sys/types.h>

#include "npf_impl.h"
#include "npf_test.h"

#define	IFNAME_EXT	"npftest0"
#define	IFNAME_INT	"npftest1"

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
npf_rule_raw_test(bool verbose, struct mbuf *m, ifnet_t *ifp, int di)
{
	npf_cache_t npc = { .npc_info = 0 };
	nbuf_t nbuf;
	npf_rule_t *rl;
	int retfl, error;

	nbuf_init(&nbuf, m, ifp);
	npf_cache_all(&npc, &nbuf);

	int slock = npf_config_read_enter();
	rl = npf_ruleset_inspect(&npc, &nbuf, npf_config_ruleset(),
	    di, NPF_LAYER_3);
	if (rl) {
		if (verbose) {
			npf_rulenc_dump(rl);
		}
		error = npf_rule_conclude(rl, &retfl);
	} else {
		error = ENOENT;
	}
	npf_config_read_exit(slock);
	return error;
}

static int
npf_test_first(bool verbose)
{
	const struct test_case *t = &test_cases[0];
	ifnet_t *ifp = ifunit(t->ifname);
	int error;

	struct mbuf *m = fill_packet(t);
	error = npf_rule_raw_test(verbose, m, ifp, t->di);
	m_freem(m);
	return error;
}

static npf_rule_t *
npf_blockall_rule(void)
{
	prop_dictionary_t rldict;

	rldict = prop_dictionary_create();
	prop_dictionary_set_uint32(rldict, "attributes",
	    NPF_RULE_IN | NPF_RULE_OUT);
	return npf_rule_alloc(rldict);
}

bool
npf_rule_test(bool verbose)
{
	npf_ruleset_t *rlset;
	npf_rule_t *rl;
	bool fail = false;
	uint64_t id;
	int error;

	for (unsigned i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		ifnet_t *ifp = ifunit(t->ifname);
		int serror;

		if (ifp == NULL) {
			printf("Interface %s is not configured.\n", t->ifname);
			return false;
		}

		struct mbuf *m = fill_packet(t);
		error = npf_rule_raw_test(verbose, m, ifp, t->di);
		serror = npf_packet_handler(NULL, &m, ifp, t->di);

		if (m) {
			m_freem(m);
		}

		if (verbose) {
			printf("Rule test %d, expected %d (stateful) and %d \n"
			    "-> returned %d and %d.\n",
			    i + 1, t->stateful_ret, t->ret, serror, error);
		}
		fail |= (serror != t->stateful_ret || error != t->ret);
	}

	error = npf_test_first(verbose);
	assert(error == RESULT_PASS);

	npf_config_enter();
	rlset = npf_config_ruleset();

	rl = npf_blockall_rule();
	error = npf_ruleset_add(rlset, "test-rules", rl);
	fail |= error != 0;

	error = npf_test_first(verbose);
	fail |= (error != RESULT_BLOCK);

	id = npf_rule_getid(rl);
	error = npf_ruleset_remove(rlset, "test-rules", id);
	fail |= error != 0;

	npf_config_exit();

	error = npf_test_first(verbose);
	fail |= (error != RESULT_PASS);

	return !fail;
}
