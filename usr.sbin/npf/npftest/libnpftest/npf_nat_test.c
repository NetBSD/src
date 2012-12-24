/*	$NetBSD: npf_nat_test.c,v 1.2 2012/12/24 19:05:47 rmind Exp $	*/

/*
 * NPF NAT test.
 *
 * Public Domain.
 */

#include <sys/types.h>

#include "npf_impl.h"
#include "npf_test.h"

#define	IFNAME_EXT	"npftest0"
#define	IFNAME_INT	"npftest1"

#define	LOCAL_IP1	"10.1.1.1"
#define	LOCAL_IP2	"10.1.1.2"

/* Note: RFC 5737 compliant addresses. */
#define	PUB_IP1		"192.0.2.1"
#define	PUB_IP2		"192.0.2.2"
#define	REMOTE_IP1	"192.0.2.3"
#define	REMOTE_IP2	"192.0.2.4"

#define	RESULT_PASS	0
#define	RESULT_BLOCK	ENETUNREACH

#define	NPF_BINAT	(NPF_NATIN | NPF_NATOUT)

static const struct test_case {
	const char *	src;
	in_port_t	sport;
	const char *	dst;
	in_port_t	dport;
	int		ttype;
	const char *	ifname;
	int		di;
	int		ret;
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
		RESULT_PASS,	PUB_IP1,	53472
	},
	{
		LOCAL_IP1,	15000,		REMOTE_IP1,	7000,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	PUB_IP1,	53472
	},
	{
		LOCAL_IP1,	15000,		REMOTE_IP1,	7000,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_IN,
		RESULT_BLOCK,	NULL,		0
	},
	{
		REMOTE_IP1,	7000,		LOCAL_IP1,	15000,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_IN,
		RESULT_BLOCK,	NULL,		0
	},
	{
		REMOTE_IP1,	7000,		PUB_IP1,	53472,
		NPF_NATOUT,	IFNAME_INT,	PFIL_IN,
		RESULT_BLOCK,	NULL,		0
	},
	{
		REMOTE_IP1,	7000,		PUB_IP1,	53472,
		NPF_NATOUT,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	LOCAL_IP1,	15000
	},

	/*
	 * NAT redirect (inbound NAT):
	 *	map $ext_if dynamic $local_ip1 port 8000 <- $pub_ip1 port 8000
	 */
	{
		REMOTE_IP2,	16000,		PUB_IP1,	8000,
		NPF_NATIN,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	LOCAL_IP1,	6000
	},
	{
		LOCAL_IP1,	6000,		REMOTE_IP2,	16000,
		NPF_NATIN,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	PUB_IP1,	8000
	},

	/*
	 * Bi-directional NAT (inbound + outbound NAT):
	 *	map $ext_if dynamic $local_ip2 <-> $pub_ip2
	 */
	{
		REMOTE_IP2,	17000,		PUB_IP2,	9000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	LOCAL_IP2,	9000
	},
	{
		LOCAL_IP2,	9000,		REMOTE_IP2,	17000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	PUB_IP2,	9000
	},
	{
		LOCAL_IP2,	18000,		REMOTE_IP2,	9000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_OUT,
		RESULT_PASS,	PUB_IP2,	18000
	},
	{
		REMOTE_IP2,	9000,		PUB_IP2,	18000,
		NPF_BINAT,	IFNAME_EXT,	PFIL_IN,
		RESULT_PASS,	LOCAL_IP2,	18000
	},

};

static bool
nmatch_addr(const char *saddr, const struct in_addr *addr2)
{
	const in_addr_t addr1 = inet_addr(saddr);
	return memcmp(&addr1, &addr2->s_addr, sizeof(in_addr_t)) != 0;
}

static bool
checkresult(bool verbose, unsigned i, struct mbuf *m, ifnet_t *ifp, int error)
{
	const struct test_case *t = &test_cases[i];
	npf_cache_t npc = { .npc_info = 0 };
	nbuf_t nbuf;

	if (verbose) {
		printf("packet %d (expected %d ret %d)\n", i+1, t->ret, error);
	}
	if (error) {
		return error == t->ret;
	}

	nbuf_init(&nbuf, m, ifp);
	if (!npf_cache_all(&npc, &nbuf)) {
		printf("error: could not fetch the packet data");
		return false;
	}

	const struct ip *ip = npc.npc_ip.v4;
	const struct udphdr *uh = npc.npc_l4.udp;

	if (verbose) {
		printf("\tpost-translation: src %s (%d)",
		    inet_ntoa(ip->ip_src), ntohs(uh->uh_sport));
		printf(" dst %s (%d)\n",
		    inet_ntoa(ip->ip_dst), ntohs(uh->uh_dport));
	}

	const bool forw = t->di == PFIL_OUT;
	const char *saddr = forw ? t->taddr : t->src;
	const char *daddr = forw ? t->dst : t->taddr;
	in_addr_t sport = forw ? t->tport : t->sport;
	in_addr_t dport = forw ? t->dport : t->tport;

	bool defect = false;
	defect |= nmatch_addr(saddr, &ip->ip_src);
	defect |= sport != ntohs(uh->uh_sport);
	defect |= nmatch_addr(daddr, &ip->ip_dst);
	defect |= dport != ntohs(uh->uh_dport);

	return !defect && error == t->ret;
}

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
	uh->uh_sport = htons(t->sport);
	uh->uh_dport = htons(t->dport);
	return m;
}

bool
npf_nat_test(bool verbose)
{
	for (unsigned i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		ifnet_t *ifp = ifunit(t->ifname);
		struct mbuf *m = fill_packet(t);
		int error;
		bool ret;

		if (ifp == NULL) {
			printf("Interface %s is not configured.\n", t->ifname);
			return false;
		}
		error = npf_packet_handler(NULL, &m, ifp, t->di);
		ret = checkresult(verbose, i, m, ifp, error);
		if (m) {
			m_freem(m);
		}
		if (!ret) {
			return false;
		}
	}
	return true;
}
