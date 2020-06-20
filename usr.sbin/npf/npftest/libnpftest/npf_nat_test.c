/*
 * NPF NAT tests.
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

#define	NPF_BINAT	(NPF_NATIN | NPF_NATOUT)

#define	RANDOM_PORT	18791

static const struct test_case {
	const char *	src;
	in_port_t	sport;
	const char *	dst;
	in_port_t	dport;
	int		ttype;
	const char *	ifname;
	int		di;
	int		ret;
	int		af;
	const char *	taddr;
	in_port_t	tport;
} test_cases[] = {

	/*
	 * Traditional NAPT (outbound NAT):
	 *	map $ext_if dynamic $local_net -> $pub_ip1
	 */
	{
		LOCAL_IP1,	15000,		REMOTE_IP1,	7000,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	AF_INET,	PUB_IP1,	RANDOM_PORT
	},
	{
		LOCAL_IP1,	15000,		REMOTE_IP1,	7000,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	AF_INET,	PUB_IP1,	RANDOM_PORT
	},
	{
		LOCAL_IP1,	15000,		REMOTE_IP1,	7000,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_IN,
		RESULT_BLOCK,	AF_INET,	NULL,		0
	},
	{
		REMOTE_IP1,	7000,		LOCAL_IP1,	15000,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_IN,
		RESULT_BLOCK,	AF_INET,	NULL,		0
	},
	{
		REMOTE_IP1,	7000,		PUB_IP1,	RANDOM_PORT,
		NPF_NATOUT,	IFNAME_INT,	PFIL_IN,
		RESULT_BLOCK,	AF_INET,	NULL,		0
	},
	{
		REMOTE_IP1,	7000,		PUB_IP1,	RANDOM_PORT,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	AF_INET,	LOCAL_IP1,	15000
	},

	/*
	 * NAT redirect (inbound NAT):
	 *	map $ext_if dynamic $local_ip1 port 6000 <- $pub_ip1 port 8000
	 */
	{
		REMOTE_IP2,	16000,		PUB_IP1,	8000,
		NPF_NATIN,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	AF_INET,	LOCAL_IP1,	6000
	},
	{
		LOCAL_IP1,	6000,		REMOTE_IP2,	16000,
		NPF_NATIN,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	AF_INET,	PUB_IP1,	8000
	},

	/*
	 * Bi-directional NAT (inbound + outbound NAT):
	 *	map $ext_if dynamic $local_ip2 <-> $pub_ip2
	 */
	{
		REMOTE_IP2,	17000,		PUB_IP2,	9000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	AF_INET,	LOCAL_IP2,	9000
	},
	{
		LOCAL_IP2,	9000,		REMOTE_IP2,	17000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	AF_INET,	PUB_IP2,	9000
	},
	{
		LOCAL_IP2,	18000,		REMOTE_IP2,	9000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	AF_INET,	PUB_IP2,	18000
	},
	{
		REMOTE_IP2,	9000,		PUB_IP2,	18000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	AF_INET,	LOCAL_IP2,	18000
	},

	/*
	 * Static NAT: plain translation both ways.
	 *	map $ext_if static $local_ip3 <-> $pub_ip3
	 */
	{
		LOCAL_IP3,	19000,		REMOTE_IP3,	10000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	AF_INET,	PUB_IP3,	19000
	},
	{
		REMOTE_IP3,	10000,		PUB_IP3,	19000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	AF_INET,	LOCAL_IP3,	19000
	},

	/*
	 * NETMAP case:
	 *	map $ext_if static algo netmap $net_a <-> $net_b
	 */
	{
		NET_A_IP1,	12345,		REMOTE_IP4,	12345,
		NPF_BINAT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	AF_INET,	NET_B_IP1,	12345
	},

	/*
	 * NPTv6 case:
	 *	map $ext_if static algo npt66 $net6_inner <-> $net6_outer
	 */
	{
		LOCAL_IP6,	1000,		REMOTE_IP6,	1001,
		NPF_BINAT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	AF_INET6,	EXPECTED_IP6,	1000
	},
	{
		REMOTE_IP6,	1001,		EXPECTED_IP6,	1000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	AF_INET6,	LOCAL_IP6,	1000
	},

};

static bool
match_addr(int af, const char *saddr, const npf_addr_t *addr2)
{
	npf_addr_t addr1;
	size_t len;

	npf_inet_pton(af, saddr, &addr1);
	len = af == AF_INET ? sizeof(struct in_addr) : sizeof(struct in6_addr);
	return memcmp(&addr1, addr2, len) == 0;
}

static bool
checkresult(bool verbose, unsigned i, struct mbuf *m, ifnet_t *ifp, int error)
{
	const struct test_case *t = &test_cases[i];
	const int af = t->af;
	npf_cache_t npc;
	nbuf_t nbuf;

	if (verbose) {
		printf("packet %d (expected %d ret %d)\n", i+1, t->ret, error);
	}
	if (error) {
		return error == t->ret;
	}

	nbuf_init(npf_getkernctx(), &nbuf, m, ifp);
	memset(&npc, 0, sizeof(npf_cache_t));
	npc.npc_ctx = npf_getkernctx();
	npc.npc_nbuf = &nbuf;

	if (!npf_cache_all(&npc)) {
		printf("error: could not fetch the packet data");
		return false;
	}

	const struct udphdr *uh = npc.npc_l4.udp;

	if (verbose) {
		char sbuf[64], dbuf[64];

		npf_inet_ntop(af, npc.npc_ips[NPF_SRC], sbuf, sizeof(sbuf));
		npf_inet_ntop(af, npc.npc_ips[NPF_DST], dbuf, sizeof(dbuf));

		printf("\tpost-translation: ");
		printf("src %s (%d) ", sbuf, ntohs(uh->uh_sport));
		printf("dst %s (%d)\n", dbuf, ntohs(uh->uh_dport));
	}
	if (error != t->ret) {
		return false;
	}

	const bool forw = t->di == PFIL_OUT;
	const char *saddr = forw ? t->taddr : t->src;
	const char *daddr = forw ? t->dst : t->taddr;
	in_addr_t sport = forw ? t->tport : t->sport;
	in_addr_t dport = forw ? t->dport : t->tport;

	CHECK_TRUE(match_addr(af, saddr, npc.npc_ips[NPF_SRC]));
	CHECK_TRUE(sport == ntohs(uh->uh_sport));
	CHECK_TRUE(match_addr(af, daddr, npc.npc_ips[NPF_DST]));
	CHECK_TRUE(dport == ntohs(uh->uh_dport));

	return true;
}

bool
npf_nat_test(bool verbose)
{
	npf_t *npf = npf_getkernctx();

	for (unsigned i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		ifnet_t *ifp = npf_test_getif(t->ifname);
		struct mbuf *m;
		int error;
		bool ret;

		if (ifp == NULL) {
			printf("Interface %s is not configured.\n", t->ifname);
			return false;
		}
		m = mbuf_get_pkt(t->af, IPPROTO_UDP,
		    t->src, t->dst, t->sport, t->dport);
		error = npfk_packet_handler(npf, &m, ifp, t->di);
		ret = checkresult(verbose, i, m, ifp, error);
		if (m) {
			m_freem(m);
		}
		CHECK_TRUE(ret);
	}
	return true;
}
