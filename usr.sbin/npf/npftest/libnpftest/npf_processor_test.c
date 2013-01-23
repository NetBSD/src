/*	$NetBSD: npf_processor_test.c,v 1.1.2.4 2013/01/23 00:06:44 yamt Exp $	*/

/*
 * NPF n-code processor test.
 *
 * Public Domain.
 */

#include <sys/types.h>
#include <sys/endian.h>

#include "npf_impl.h"
#include "npf_ncode.h"
#include "npf_test.h"

#if BYTE_ORDER == LITTLE_ENDIAN
#define	IP4(a, b, c, d)	((a << 0) | (b << 8) | (c << 16) | (d << 24))
#elif BYTE_ORDER == BIG_ENDIAN
#define	IP4(a, b, c, d)	((a << 24) | (b << 16) | (c << 8) | (d << 0))
#endif

#define	PORTS(a, b)	((htons(a) << 16) | htons(b))

static const uint32_t nc_match[] = {
	NPF_OPCODE_CMP,		NPF_LAYER_3,	0,
	NPF_OPCODE_BEQ,		0x0c,
	NPF_OPCODE_ETHER,	0x00,	0x00,	htons(ETHERTYPE_IP),
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_ADVR,	3,
	NPF_OPCODE_IP4MASK,	0x01,	IP4(192,168,2,0),	24,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_TCP_PORTS,	0x00,	PORTS(80, 80),
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x00
};

static const uint32_t nc_nmatch[] = {
	NPF_OPCODE_CMP,		NPF_LAYER_3,	0,
	NPF_OPCODE_BEQ,		0x0c,
	NPF_OPCODE_ETHER,	0x00,	0x00,	htons(ETHERTYPE_IP),
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_ADVR,	3,
	NPF_OPCODE_IP4MASK,	0x01,	IP4(192,168,2,1),	32,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x00
};

static const uint32_t nc_rmatch[] = {
	NPF_OPCODE_MOVE,	offsetof(struct ip, ip_src),	1,
	NPF_OPCODE_ADVR,	1,
	NPF_OPCODE_LW,		sizeof(in_addr_t),		0,
	NPF_OPCODE_CMP,		IP4(192,168,2,100),		0,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_MOVE,	sizeof(struct ip) - offsetof(struct ip, ip_src)
				+ offsetof(struct tcphdr, th_sport),	1,
	NPF_OPCODE_ADVR,	1,
	NPF_OPCODE_LW,		2 * sizeof(in_port_t),		0,
	NPF_OPCODE_CMP,		htonl((15000 << 16) | 80),	0,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x01
};

static const uint32_t nc_inval[] = {
	NPF_OPCODE_BEQ,		0x05,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x01
};

static const uint32_t nc_match6[] = {
	NPF_OPCODE_IP6MASK,	0x01,	htonl(0xfe80 << 16), 0x0, 0x0, 0x0, 10,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_TCP_PORTS,	0x00,	PORTS(80, 80),
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x00
};

static struct mbuf *
fill_packet(int proto, bool ether)
{
	struct mbuf *m;
	struct ip *ip;
	struct tcphdr *th;

	if (ether) {
		m = mbuf_construct_ether(IPPROTO_TCP);
	} else {
		m = mbuf_construct(IPPROTO_TCP);
	}
	th = mbuf_return_hdrs(m, ether, &ip);
	ip->ip_src.s_addr = inet_addr("192.168.2.100");
	ip->ip_dst.s_addr = inet_addr("10.0.0.1");
	th->th_sport = htons(15000);
	th->th_dport = htons(80);
	return m;
}

static struct mbuf *
fill_packet6(int proto)
{
	uint16_t src[] = {
	    htons(0xfe80), 0x0, 0x0, 0x0,
	    htons(0x2a0), htons(0xc0ff), htons(0xfe10), htons(0x1234)
	};
	uint16_t dst[] = {
	    htons(0xfe80), 0x0, 0x0, 0x0,
	    htons(0x2a0), htons(0xc0ff), htons(0xfe10), htons(0x1111)
	};
	struct mbuf *m;
	struct ip6_hdr *ip;
	struct tcphdr *th;

	m = mbuf_construct6(proto);
	(void)mbuf_return_hdrs(m, false, (struct ip **)&ip);
	memcpy(&ip->ip6_src, src, sizeof(ip->ip6_src));
	memcpy(&ip->ip6_dst, dst, sizeof(ip->ip6_src));

	th = (void *)(ip + 1);
	th->th_sport = htons(15000);
	th->th_dport = htons(80);
	return m;
}

static bool
retcode_fail_p(const char *msg, bool verbose, int ret, int expected)
{
	bool fail = (ret != expected);

	if (verbose) {
		printf("%-25s\t%-4d == %4d\t-> %s\n",
		    msg, ret, expected, fail ? "fail" : "ok");
	}
	return fail;
}

static void
npf_nc_cachetest(struct mbuf *m, npf_cache_t *npc, nbuf_t *nbuf)
{
	const void *dummy_ifp = (void *)0xdeadbeef;

	nbuf_init(nbuf, m, dummy_ifp);
	memset(npc, 0, sizeof(npf_cache_t));
	npf_cache_all(npc, nbuf);
}

bool
npf_processor_test(bool verbose)
{
	npf_cache_t npc;
	struct mbuf *m;
	nbuf_t nbuf;
	int errat, ret;
	bool fail = false;

#if 0
	/* Layer 2 (Ethernet + IP + TCP). */
	ret = npf_ncode_validate(nc_match, sizeof(nc_match), &errat);
	fail |= retcode_fail_p("Ether validation", verbose, ret, 0);

	m = fill_packet(IPPROTO_TCP, true);
	npf_nc_cachetest(m, &npc, &nbuf);
	ret = npf_ncode_process(&npc, nc_match, &nbuf, NPF_LAYER_2);
	fail |= retcode_fail_p("Ether", verbose, ret, 0);
	m_freem(m);
#endif

	/* Layer 3 (IP + TCP). */
	m = fill_packet(IPPROTO_TCP, false);
	npf_nc_cachetest(m, &npc, &nbuf);
	ret = npf_ncode_process(&npc, nc_match, &nbuf, NPF_LAYER_3);
	fail |= retcode_fail_p("IPv4 mask 1", verbose, ret, 0);

	/* Non-matching IPv4 case. */
	ret = npf_ncode_validate(nc_nmatch, sizeof(nc_nmatch), &errat);
	fail |= retcode_fail_p("IPv4 mask 2 validation", verbose, ret, 0);

	npf_nc_cachetest(m, &npc, &nbuf);
	ret = npf_ncode_process(&npc, nc_nmatch, &nbuf, NPF_LAYER_3);
	fail |= retcode_fail_p("IPv4 mask 2", verbose, ret, 255);

	/* Invalid n-code case. */
	ret = npf_ncode_validate(nc_inval, sizeof(nc_inval), &errat);
	fail |= retcode_fail_p("Invalid n-code", verbose, ret, NPF_ERR_JUMP);

	/* RISC-like insns. */
	ret = npf_ncode_validate(nc_rmatch, sizeof(nc_rmatch), &errat);
	fail |= retcode_fail_p("RISC-like n-code validation", verbose, ret, 0);

	npf_nc_cachetest(m, &npc, &nbuf);
	ret = npf_ncode_process(&npc, nc_rmatch, &nbuf, NPF_LAYER_3);
	fail |= retcode_fail_p("RISC-like n-code", verbose, ret, 1);
	m_freem(m);

	/* IPv6 matching. */
	ret = npf_ncode_validate(nc_match6, sizeof(nc_match6), &errat);
	fail |= retcode_fail_p("IPv6 mask validation", verbose, ret, 0);

	m = fill_packet6(IPPROTO_TCP);
	npf_nc_cachetest(m, &npc, &nbuf);
	ret = npf_ncode_process(&npc, nc_match6, &nbuf, NPF_LAYER_3);
	fail |= retcode_fail_p("IPv6 mask", verbose, ret, 0);
	m_freem(m);

	return !fail;
}
