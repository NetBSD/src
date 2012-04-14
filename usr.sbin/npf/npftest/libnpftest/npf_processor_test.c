/*	$NetBSD: npf_processor_test.c,v 1.1 2012/04/14 21:57:29 rmind Exp $	*/

/*
 * NPF n-code processor test.
 *
 * Public Domain.
 */

#include <sys/types.h>

#include "npf_impl.h"
#include "npf_ncode.h"
#include "npf_test.h"

/*
 * In network byte order:
 *	192.168.2.0				== 0x0002a8c0
 *	192.168.2.1				== 0x0102a8c0
 *	192.168.2.100				== 0x6402a8c0
 *	htons(ETHERTYPE_IP)			== 0x08
 *	(htons(80) << 16) | htons(80)		== 0x50005000
 *	(htons(80) << 16) | htons(15000)	== 0x5000983a
 */

static uint32_t nc_match[ ] __aligned(4) = {
	NPF_OPCODE_CMP,		NPF_LAYER_3,	0,
	NPF_OPCODE_BEQ,		0x0c,
	NPF_OPCODE_ETHER,	0x00,		0x00,		0x08,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_ADVR,	3,
	NPF_OPCODE_IP4MASK,	0x01,		0x0002a8c0,	24,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_TCP_PORTS,	0x00,		0x50005000,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x00
};

static uint32_t nc_nmatch[ ] __aligned(4) = {
	NPF_OPCODE_CMP,		NPF_LAYER_3,	0,
	NPF_OPCODE_BEQ,		0x0c,
	NPF_OPCODE_ETHER,	0x00,		0x00,		0x08,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_ADVR,	3,
	NPF_OPCODE_IP4MASK,	0x01,		0x0102a8c0,	32,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x00
};

static uint32_t nc_rmatch[ ] __aligned(4) = {
	NPF_OPCODE_MOVE,	offsetof(struct ip, ip_src),	1,
	NPF_OPCODE_ADVR,	1,
	NPF_OPCODE_LW,		sizeof(in_addr_t),		0,
	NPF_OPCODE_CMP,		0x6402a8c0,			0,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_MOVE,	sizeof(struct ip) - offsetof(struct ip, ip_src)
				+ offsetof(struct tcphdr, th_sport),	1,
	NPF_OPCODE_ADVR,	1,
	NPF_OPCODE_LW,		2 * sizeof(in_port_t),		0,
	NPF_OPCODE_CMP,		0x5000983a,			0,
	NPF_OPCODE_BEQ,		0x04,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x01
};

static uint32_t nc_inval[ ] __aligned(4) = {
	NPF_OPCODE_BEQ,		0x05,
	NPF_OPCODE_RET,		0xff,
	NPF_OPCODE_RET,		0x01
};

static void
fill_packet(struct mbuf *m, bool ether)
{
	struct ip *ip;
	struct tcphdr *th;

	th = mbuf_return_hdrs(m, ether, &ip);
	ip->ip_src.s_addr = inet_addr("192.168.2.100");
	ip->ip_dst.s_addr = inet_addr("10.0.0.1");
	th->th_sport = htons(15000);
	th->th_dport = htons(80);
}

static bool
validate_retcode(const char *msg, bool verbose, int ret, int expected)
{
	bool ok = (ret == expected);

	if (verbose) {
		printf("%-25s\t%-4d == %4d\t-> %s\n",
		    msg, ret, expected, ok ? "ok" : "fail");
	}
	return ok;
}

bool
npf_processor_test(bool verbose)
{
	npf_cache_t npc;
	struct mbuf *m;
	int errat, ret;

	/* Layer 2 (Ethernet + IP + TCP). */
	m = mbuf_construct_ether(IPPROTO_TCP);
	fill_packet(m, true);
	ret = npf_ncode_validate(nc_match, sizeof(nc_match), &errat);
	if (!validate_retcode("Ether validation", verbose, ret, 0)) {
		return false;
	}
	memset(&npc, 0, sizeof(npf_cache_t));
	ret = npf_ncode_process(&npc, nc_match, m, NPF_LAYER_2);
	if (!validate_retcode("Ether", verbose, ret, 0)) {
		return false;
	}
	m_freem(m);

	/* Layer 3 (IP + TCP). */
	m = mbuf_construct(IPPROTO_TCP);
	fill_packet(m, false);
	memset(&npc, 0, sizeof(npf_cache_t));
	ret = npf_ncode_process(&npc, nc_match, m, NPF_LAYER_3);
	if (!validate_retcode("IPv4 mask 1", verbose, ret, 0)) {
		return false;
	}

	/* Non-matching IPv4 case. */
	ret = npf_ncode_validate(nc_nmatch, sizeof(nc_nmatch), &errat);
	if (!validate_retcode("IPv4 mask 2 validation", verbose, ret, 0)) {
		return false;
	}
	memset(&npc, 0, sizeof(npf_cache_t));
	ret = npf_ncode_process(&npc, nc_nmatch, m, NPF_LAYER_3);
	if (!validate_retcode("IPv4 mask 2", verbose, ret, 255)) {
		return false;
	}

	/* Invalid n-code case. */
	ret = npf_ncode_validate(nc_inval, sizeof(nc_inval), &errat);
	if (!validate_retcode("Invalid n-code", verbose, ret, NPF_ERR_JUMP)) {
		return false;
	}

	/* RISC-like insns. */
	ret = npf_ncode_validate(nc_rmatch, sizeof(nc_rmatch), &errat);
	if (!validate_retcode("RISC-like n-code validation", verbose, ret, 0)) {
		return false;
	}
	memset(&npc, 0, sizeof(npf_cache_t));
	ret = npf_ncode_process(&npc, nc_rmatch, m, NPF_LAYER_3);
	if (!validate_retcode("RISC-like n-code", verbose, ret, 1)) {
		return false;
	}

	m_freem(m);

	return true;
}
