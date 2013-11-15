/*	$NetBSD: npf_bpf.c,v 1.3 2013/11/15 00:12:44 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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
 * NPF byte-code processing.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_bpf.c,v 1.3 2013/11/15 00:12:44 rmind Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <sys/mbuf.h>

#define NPF_BPFCOP
#include "npf_impl.h"

/*
 * BPF context and the coprocessor.
 */

static bpf_ctx_t *npf_bpfctx __read_mostly;

static uint32_t	npf_cop_l3(bpf_ctx_t *, bpf_args_t *, uint32_t);
static uint32_t	npf_cop_table(bpf_ctx_t *, bpf_args_t *, uint32_t);

static const bpf_copfunc_t npf_bpfcop[] = {
	[NPF_COP_L3]	= npf_cop_l3,
	[NPF_COP_TABLE]	= npf_cop_table,
};

void
npf_bpf_sysinit(void)
{
	npf_bpfctx = bpf_create();
	KASSERT(npf_bpfctx != NULL);
	bpf_set_cop(npf_bpfctx, npf_bpfcop, __arraycount(npf_bpfcop));
}

void
npf_bpf_sysfini(void)
{
	bpf_destroy(npf_bpfctx);
}

int
npf_bpf_filter(npf_cache_t *npc, nbuf_t *nbuf,
    const void *code, bpfjit_func_t jcode)
{
	const struct mbuf *m = nbuf_head_mbuf(nbuf);
	const size_t pktlen = m_length(m);
	bpf_args_t args = {
		.pkt = m,
		.wirelen = pktlen,
		.buflen = 0,
		.arg = npc
	};

	memset(args.mem, 0, sizeof(uint32_t) * BPF_MEMWORDS);

	/* Execute JIT code. */
	if (__predict_true(jcode)) {
		return jcode((const unsigned char *)m, pktlen, 0);
	}

	/* Execute BPF byte-code. */
	return bpf_filter_ext(npf_bpfctx, code, &args);
}

bool
npf_bpf_validate(const void *code, size_t len)
{
	const size_t icount = len / sizeof(struct bpf_insn);
	return bpf_validate_ext(npf_bpfctx, code, icount) != 0;
}

/*
 * NPF_COP_L3: fetches layer 3 information.
 *
 * Output words in the memory store:
 *	BPF_MW_IPVER	IP version (4 or 6).
 *	BPF_MW_L4OFF	L4 header offset.
 *	BPF_MW_L4PROTO	L4 protocol.
 */
static uint32_t
npf_cop_l3(bpf_ctx_t *bc, bpf_args_t *args, uint32_t A)
{
	const npf_cache_t * const npc = (const npf_cache_t *)args->arg;
	uint32_t * const M = args->mem;

	/*
	 * Convert address length to IP version.  Just mask out
	 * number 4 or set 6 if higher bits set, such that:
	 *
	 *	0	=>	0
	 *	4	=>	4 (IPVERSION)
	 *	16	=>	6 (IPV6_VERSION >> 4)
	 */
	const u_int alen = npc->npc_alen;
	const uint32_t ver = (alen & 4) | ((alen >> 4) * 6);

	M[BPF_MW_IPVER] = ver;
	M[BPF_MW_L4OFF] = npc->npc_hlen;
	M[BPF_MW_L4PROTO] = npc->npc_proto;

	/* A <- IP version */
	return ver;
}

#define	SRC_FLAG_BIT	(1U << 31)

/*
 * NPF_COP_TABLE: perform NPF table lookup.
 *
 *	A <- non-zero (true) if found and zero (false) otherwise
 */
static uint32_t
npf_cop_table(bpf_ctx_t *bc, bpf_args_t *args, uint32_t A)
{
	const npf_cache_t * const npc = (const npf_cache_t *)args->arg;
	npf_tableset_t *tblset = npf_config_tableset();
	const uint32_t tid = A & (SRC_FLAG_BIT - 1);
	const npf_addr_t *addr;
	npf_table_t *t;

	KASSERT(npf_iscached(npc, NPC_IP46));

	if ((t = npf_tableset_getbyid(tblset, tid)) == NULL) {
		return 0;
	}
	addr = (A & SRC_FLAG_BIT) ? npc->npc_srcip : npc->npc_dstip;
	return npf_table_lookup(t, npc->npc_alen, addr) == 0;
}
