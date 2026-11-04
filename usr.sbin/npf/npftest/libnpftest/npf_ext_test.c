


#ifdef _KERNEL
#include <sys/types.h>
#endif

#include "npf_impl.h"
#include "npf_test.h"

static const struct test_case {
	const char *	src;
	in_port_t	sport;
	const char *	dst;
	in_port_t	dport;
	const char *	ifname;
	int		di;
	int		af;
} test_cases[] = {
	{
		LOCAL_IP60,	15000,		LOCAL_IP70,	7000,
		IFNAME_EXT,	PFIL_OUT,
		AF_INET
	},
	{
		LOCAL_IP60,	15040,		LOCAL_IP70,	6000,
		IFNAME_EXT,	PFIL_OUT,
		AF_INET
	},

};

#define TCP_OPT_LEN 4
static struct mbuf *
mbuf_get_pkt_with_option(int af, int proto, const char *src, const char *dst,
    int sport, int dport)
{
	struct mbuf *m;
	struct ip *ip;
	struct tcphdr *th;
	void *ipsrc, *ipdst;
	int size = 0;
	uint8_t *opt;
	uint16_t mss = 1400;

	m = m_gethdr(M_WAITOK, MT_HEADER);
	ip = mtod(m, struct ip *);

	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_off = 0;
	ip->ip_ttl = 64;
	ip->ip_p = proto;

	ipsrc = &ip->ip_src.s_addr;
	ipdst = &ip->ip_dst.s_addr;

	npf_inet_pton(af, src, ipsrc);
	npf_inet_pton(af, dst, ipdst);

	th = mbuf_return_hdrs(m, false, &ip);
	size = sizeof(struct ip) + sizeof(struct tcphdr) + TCP_OPT_LEN;
	th->th_flags |= TH_SYN;
	th->th_off = (sizeof(struct tcphdr) + TCP_OPT_LEN)  >> 2;
	th->th_sport = htons(sport);
	th->th_dport = htons(dport);

	opt = (uint8_t *)th + sizeof(struct tcphdr);
	opt[0] = TCPOPT_MAXSEG;         /* Kind = 2 */
	opt[1] = TCPOLEN_MAXSEG;        /* Length = 4 */
	opt[2] = (mss >> 8) & 0xff;    /* 0x05 */
	opt[3] = mss & 0xff;           /* 0x29 */

	ip->ip_len = htons(size);

	m->m_pkthdr.len = size;
	m->m_len = size;
	m->m_next = NULL;
	return m;
}

static int
run_raw_testcase(npf_cache_t *npc, unsigned i, bool verbose)
{
	const struct test_case *t = &test_cases[i];
	npf_t *npf = npf_getkernctx();
	int slock, error;
	npf_rproc_t *rp;
	npf_rule_t *rl;
	npf_match_info_t mi;
	int decision = 1;

	slock = npf_config_read_enter(npf);
	rl = npf_ruleset_inspect(npc, npf_config_ruleset(npf), t->di, NPF_RULE_LAYER_3);
	if (rl) {
		error = npf_rule_conclude(rl, &mi);
	} else {
		error = ENOENT;
		return error;
	}
	npf_config_read_exit(npf, slock);

	rp = npf_rule_getrproc(rl);
	assert(rp != NULL);

	error = npf_rproc_run(npc, rp, &mi, &decision);

	if (error == 1) {
		npf_rproc_release(rp);
	}

	return error;
}

static void
npf_get_normalize_params(npf_cache_t *npc, uint16_t *ip_ttl, uint8_t *ip_id, int *df, uint16_t *maxmss)
{
    int wscale;
    struct ip *ip = npc->npc_ip.v4;
    *ip_id = ip->ip_id;
    *ip_ttl = ip->ip_ttl;
    *df = ip->ip_off & IP_DF;
    npf_fetch_tcpopts(npc, maxmss, &wscale);
}

bool
npf_procedure_test(bool verbose)
{
	int ret;
	for (unsigned i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		npf_cache_t *npc;
		struct mbuf *m;
		uint16_t ip_ttl;
    	uint8_t ip_id;
    	int df;
    	uint16_t maxmss;

		if (npf_test_getif(t->ifname) == NULL) {
			printf("Interface %s is not configured.\n", t->ifname);
			return false;
		}

		m = mbuf_get_pkt_with_option(t->af, IPPROTO_TCP, t->src, t->dst, 9000, 9000);
		npc = get_cached_pkt(m, t->ifname,  NPF_RULE_LAYER_3);

		ret = run_raw_testcase(npc, i, verbose);
		if (verbose) {
			printf("%s: expected return in raw testcase 1, returned %d\n", __func__, ret);
		}
		CHECK_TRUE(ret == 1);

		npf_get_normalize_params(npc, &ip_ttl, &ip_id, &df, &maxmss);
		maxmss = ntohs(maxmss);

		if (verbose) {
			printf("Normalization:\texpected 164 ttl but got %u\n", ip_ttl);
			printf("Normalization:\texpected 1321 maxmss but got %u\n", maxmss);
		}
		CHECK_TRUE((df & IP_DF) == 0);
    	CHECK_TRUE(maxmss == 1321);
		CHECK_TRUE(ip_ttl == 164);
		put_cached_pkt(npc);
	}

	return true;
}
