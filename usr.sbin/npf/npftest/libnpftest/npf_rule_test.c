/*	$NetBSD: npf_rule_test.c,v 1.2.4.3 2013/01/23 00:06:44 yamt Exp $	*/

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

	npf_core_enter();
	rl = npf_ruleset_inspect(&npc, &nbuf, npf_core_ruleset(),
	    di, NPF_LAYER_3);
	if (rl) {
		if (verbose) {
			npf_rulenc_dump(rl);
		}
		error = npf_rule_apply(&npc, &nbuf, rl, &retfl);
	} else {
		npf_core_exit();
		error = ENOENT;
	}
	return error;
}

bool
npf_rule_test(bool verbose)
{
	bool fail = false;

	for (unsigned i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		ifnet_t *ifp = ifunit(t->ifname);
		int serror, error;

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
	return !fail;
}
