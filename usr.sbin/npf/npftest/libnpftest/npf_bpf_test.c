/*	$NetBSD: npf_bpf_test.c,v 1.7 2014/07/20 00:37:41 rmind Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NPF test of BPF coprocessor.
 */

#include <sys/types.h>
#include <sys/endian.h>

#define	NPF_BPFCOP
#include "npf_impl.h"
#include "npf_test.h"

static bool	lverbose = false;

static struct mbuf *
fill_packet(int proto)
{
	struct mbuf *m;
	struct ip *ip;
	struct tcphdr *th;

	m = mbuf_construct(IPPROTO_TCP);
	th = mbuf_return_hdrs(m, false, &ip);
	ip->ip_src.s_addr = inet_addr("192.168.2.100");
	ip->ip_dst.s_addr = inet_addr("10.0.0.1");
	th->th_sport = htons(15000);
	th->th_dport = htons(80);
	return m;
}

static int
test_bpf_code(void *code, size_t size)
{
	ifnet_t *dummy_ifp = npf_test_addif(IFNAME_TEST, false, false);
	npf_cache_t npc = { .npc_info = 0 };
	uint32_t memstore[BPF_MEMWORDS];
	bpf_args_t bc_args;
	struct mbuf *m;
	nbuf_t nbuf;
	int ret, jret;
	void *jcode;

	/* Layer 3 (IP + TCP). */
	m = fill_packet(IPPROTO_TCP);
	nbuf_init(&nbuf, m, dummy_ifp);
	npc.npc_nbuf = &nbuf;
	npf_cache_all(&npc);

	bc_args.pkt = (const uint8_t *)m;
	bc_args.buflen = m_length(m);
	bc_args.wirelen = bc_args.buflen;
	bc_args.mem = memstore;
	bc_args.arg = &npc;

	ret = npf_bpf_filter(&bc_args, code, NULL);

	/* JIT-compiled code. */
	jcode = npf_bpf_compile(code, size);
	if (jcode) {
		jret = npf_bpf_filter(&bc_args, NULL, jcode);
		assert(ret == jret);
		bpf_jit_freecode(jcode);
	} else if (lverbose) {
		printf("JIT-compilation failed\n");
	}
	m_freem(m);

	return ret;
}

static uint32_t
npf_bpfcop_run(u_int reg)
{
	struct bpf_insn insns_npf_bpfcop[] = {
		BPF_STMT(BPF_MISC+BPF_COP, NPF_COP_L3),
		BPF_STMT(BPF_LD+BPF_W+BPF_MEM, reg),
		BPF_STMT(BPF_RET+BPF_A, 0),
	};
	return test_bpf_code(&insns_npf_bpfcop, sizeof(insns_npf_bpfcop));
}

static bool
npf_bpfcop_test(void)
{
	bool fail = false;

	/* A <- IP version (4 or 6) */
	struct bpf_insn insns_ipver[] = {
		BPF_STMT(BPF_MISC+BPF_COP, NPF_COP_L3),
		BPF_STMT(BPF_RET+BPF_A, 0),
	};
	fail |= (test_bpf_code(&insns_ipver, sizeof(insns_ipver)) != IPVERSION);

	/* BPF_MW_IPVERI <- IP version */
	fail |= (npf_bpfcop_run(BPF_MW_IPVER) != IPVERSION);

	/* BPF_MW_L4OFF <- L4 header offset */
	fail |= (npf_bpfcop_run(BPF_MW_L4OFF) != sizeof(struct ip));

	/* BPF_MW_L4PROTO <- L4 protocol */
	fail |= (npf_bpfcop_run(BPF_MW_L4PROTO) != IPPROTO_TCP);

	return fail;
}

bool
npf_bpf_test(bool verbose)
{
	bool fail = false;

	lverbose = verbose;

	fail |= npf_bpfcop_test();

	return !fail;
}
