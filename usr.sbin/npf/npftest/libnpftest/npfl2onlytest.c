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

/*
 * in this module, we run tests on layer 2 packets for configs that has only layer 3 rules
 * All incoming frames at layer 2 should pass so we ensure that
 * npf config with no layer 2 rules should for no chance be blocked by npf
 * at layer 2
 * config to be loaded is ../npfl3test.conf
 */

static const struct test_case {
	const char *src;
	const char *dst;
	uint16_t    etype;
	const char *ifname;
	int	    di;
	int	    ret;
} test_cases[] = {
	{
		.src = "00:00:5E:00:53:00",	.dst = "00:00:5E:00:53:01",
		.ifname = IFNAME_INT,		.etype = ETHERTYPE_IPV6,
		.di = PFIL_IN,			.ret = RESULT_PASS
	},
	{
		.src = "00:00:5E:00:53:01",	.dst = "00:00:5E:00:53:02",
		.ifname = IFNAME_INT,		.etype = ETHERTYPE_IP,
		.di = PFIL_OUT,			.ret = RESULT_PASS
	},
	{
		.src = "00:00:5E:00:53:00",	.dst = "00:00:5E:00:53:02",
		.ifname = IFNAME_INT,		.etype = ETHERTYPE_IP,
		.di = PFIL_IN,			.ret = RESULT_PASS
	},
};

static int
run_handler_testcase(unsigned i)
{
	const struct test_case *t = &test_cases[i];
	ifnet_t *ifp = npf_test_getif(t->ifname);
	npf_t *npf = npf_getkernctx();
	struct mbuf *m;
	int error;

	m = mbuf_get_frame(t->src, t->dst, htons(t->etype));
	error = npfk_layer2_handler(npf, &m, ifp, t->di);
	if (m) {
		m_freem(m);
	}
	return error;
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

/* sorry for long function name */
bool
npf_layer2only_test(bool verbose)
{
	bool ok;

	ok = test_static(verbose);
	CHECK_TRUE(ok);

	return true;
}
