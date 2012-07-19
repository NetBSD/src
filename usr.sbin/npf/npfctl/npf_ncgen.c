/*	$NetBSD: npf_ncgen.c,v 1.13 2012/07/19 21:52:29 spz Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
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
 * N-code generation interface.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_ncgen.c,v 1.13 2012/07/19 21:52:29 spz Exp $");

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <err.h>

#include "npfctl.h"

/* Reduce re-allocations by expanding in 64 byte blocks. */
#define	NC_ALLOC_MASK		(64 - 1)
#define	NC_ALLOC_ROUND(x)	(((x) + NC_ALLOC_MASK) & ~NC_ALLOC_MASK)

struct nc_ctx {
	/*
	 * Original buffer address, size of the buffer and instruction
	 * pointer for appending n-code fragments.
	 */
	void *			nc_buf;
	void *			nc_iptr;
	size_t			nc_len;
	/* Expected number of words for diagnostic check. */
	size_t			nc_expected;
	/* List of jump values, length of the memory and iterator. */
	ptrdiff_t *		nc_jmp_list;
	size_t			nc_jmp_len;
	size_t			nc_jmp_it;
	/* Current logical operation for a group and saved iterator. */
	size_t			nc_saved_it;
};

/*
 * npfctl_ncgen_getptr: return the instruction pointer and make sure that
 * buffer is large enough to add a new fragment of a given size.
 */
static uint32_t *
npfctl_ncgen_getptr(nc_ctx_t *ctx, size_t nwords)
{
	size_t offset, reqlen;

	/* Save the number of expected words for diagnostic check. */
	assert(ctx->nc_expected == 0);
	ctx->nc_expected = (sizeof(uint32_t) * nwords);

	/*
	 * Calculate the required length.  If buffer size is large enough,
	 * just return the pointer.
	 */
	offset = (uintptr_t)ctx->nc_iptr - (uintptr_t)ctx->nc_buf;
	assert(offset <= ctx->nc_len);
	reqlen = offset + ctx->nc_expected;
	if (reqlen < ctx->nc_len) {
		return ctx->nc_iptr;
	}

	/* Otherwise, re-allocate the buffer and update the pointers. */
	ctx->nc_len = NC_ALLOC_ROUND(reqlen);
	ctx->nc_buf = xrealloc(ctx->nc_buf, ctx->nc_len);
	ctx->nc_iptr = (uint8_t *)ctx->nc_buf + offset;
	return ctx->nc_iptr;
}

/*
 * npfctl_ncgen_putptr: perform a diagnostic check whether expected words
 * were appended and save the instruction pointer.
 */
static void
npfctl_ncgen_putptr(nc_ctx_t *ctx, void *nc)
{
	ptrdiff_t diff = (uintptr_t)nc - (uintptr_t)ctx->nc_iptr;

	if ((ptrdiff_t)ctx->nc_expected != diff) {
		errx(EXIT_FAILURE, "unexpected n-code fragment size "
		    "(expected words %zu, diff %td)", ctx->nc_expected, diff);
	}
	ctx->nc_expected = 0;
	ctx->nc_iptr = nc;
}

/*
 * npfctl_ncgen_addjmp: add the compare/jump opcode, dummy value and
 * its pointer into the list.
 */
static void
npfctl_ncgen_addjmp(nc_ctx_t *ctx, uint32_t **nc_ptr)
{
	size_t reqlen, i = ctx->nc_jmp_it++;
	uint32_t *nc = *nc_ptr;

	reqlen = NC_ALLOC_ROUND(ctx->nc_jmp_it * sizeof(ptrdiff_t));

	if (reqlen > NC_ALLOC_ROUND(ctx->nc_jmp_len)) {
		ctx->nc_jmp_list = xrealloc(ctx->nc_jmp_list, reqlen);
		ctx->nc_jmp_len = reqlen;
	}

	/* Save the offset (note: we cannot save the pointer). */
	ctx->nc_jmp_list[i] = (uintptr_t)nc - (uintptr_t)ctx->nc_buf;

	/* Note: if OR grouping case, BNE will be replaced with BEQ. */
	*nc++ = NPF_OPCODE_BNE;
	*nc++ = 0xdeadbeef;
	*nc_ptr = nc;
}

/*
 * npfctl_ncgen_create: new n-code generation context.
 */
nc_ctx_t *
npfctl_ncgen_create(void)
{
	return zalloc(sizeof(nc_ctx_t));
}

/*
 * npfctl_ncgen_complete: complete generation, destroy the context and
 * return a pointer to the final buffer containing n-code.
 */
void *
npfctl_ncgen_complete(nc_ctx_t *ctx, size_t *sz)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 4 /* words */);
	ptrdiff_t foff;
	size_t i;

	assert(ctx->nc_saved_it == 0);

	/* Success path (return 0x0). */
	*nc++ = NPF_OPCODE_RET;
	*nc++ = 0x0;

	/* Failure path (return 0xff). */
	foff = ((uintptr_t)nc - (uintptr_t)ctx->nc_buf) / sizeof(uint32_t);
	*nc++ = NPF_OPCODE_RET;
	*nc++ = 0xff;

	/* + 4 words. */
	npfctl_ncgen_putptr(ctx, nc);

	/* Change the jump values. */
	for (i = 0; i < ctx->nc_jmp_it; i++) {
		ptrdiff_t off = ctx->nc_jmp_list[i] / sizeof(uint32_t);
		uint32_t *jmpop = (uint32_t *)ctx->nc_buf + off;
		uint32_t *jmpval = jmpop + 1;

		assert(foff > off);
		assert(*jmpop == NPF_OPCODE_BNE);
		assert(*jmpval == 0xdeadbeef);
		*jmpval = foff - off;
	}

	/* Return the buffer, destroy the context. */
	void *buf = ctx->nc_buf;
	*sz = (uintptr_t)ctx->nc_iptr - (uintptr_t)ctx->nc_buf;
	free(ctx->nc_jmp_list);
	free(ctx);
	return buf;
}

/*
 * npfctl_ncgen_group: begin a logical group.
 */
void
npfctl_ncgen_group(nc_ctx_t *ctx)
{
	assert(ctx->nc_expected == 0);
	assert(ctx->nc_saved_it == 0);
	ctx->nc_saved_it = ctx->nc_jmp_it;
}

/*
 * npfctl_ncgen_endgroup: end a logical group, fix up the code accordingly.
 */
void
npfctl_ncgen_endgroup(nc_ctx_t *ctx)
{
	uint32_t *nc;

	/* If there are no fragments or only one - nothing to do. */
	if ((ctx->nc_jmp_it - ctx->nc_saved_it) <= 1) {
		ctx->nc_saved_it = 0;
		return;
	}

	/* Append failure return for OR grouping. */
	nc = npfctl_ncgen_getptr(ctx, 2 /* words */);
	*nc++ = NPF_OPCODE_RET;
	*nc++ = 0xff;
	npfctl_ncgen_putptr(ctx, nc);

	/* Update any group jumps values on success to the current point. */
	for (size_t i = ctx->nc_saved_it; i < ctx->nc_jmp_it; i++) {
		ptrdiff_t off = ctx->nc_jmp_list[i] / sizeof(uint32_t);
		uint32_t *jmpop = (uint32_t *)ctx->nc_buf + off;
		uint32_t *jmpval = jmpop + 1;

		assert(*jmpop == NPF_OPCODE_BNE);
		assert(*jmpval == 0xdeadbeef);

		*jmpop = NPF_OPCODE_BEQ;
		*jmpval = nc - jmpop;
		ctx->nc_jmp_list[i] = 0;
	}

	/* Reset the iterator. */
	ctx->nc_jmp_it = ctx->nc_saved_it;
	ctx->nc_saved_it = 0;
}

/*
 * npfctl_gennc_v6cidr: fragment to match IPv6 CIDR.
 */
void
npfctl_gennc_v6cidr(nc_ctx_t *ctx, int opts, const npf_addr_t *netaddr,
    const npf_netmask_t mask)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 9 /* words */);
	const uint32_t *addr = (const uint32_t *)netaddr;

	assert(((opts & NC_MATCH_SRC) != 0) ^ ((opts & NC_MATCH_DST) != 0));
	assert((mask && mask <= NPF_MAX_NETMASK) || mask == NPF_NO_NETMASK);

	/* OP, direction, netaddr/subnet (7 words) */
	*nc++ = NPF_OPCODE_IP6MASK;
	*nc++ = (opts & (NC_MATCH_DST | NC_MATCH_SRC)) >> 1;
	*nc++ = addr[0];
	*nc++ = addr[1];
	*nc++ = addr[2];
	*nc++ = addr[3];
	*nc++ = mask;

	/* Comparison block (2 words). */
	npfctl_ncgen_addjmp(ctx, &nc);

	/* + 9 words. */
	npfctl_ncgen_putptr(ctx, nc);
}

/*
 * npfctl_gennc_v4cidr: fragment to match IPv4 CIDR.
 */
void
npfctl_gennc_v4cidr(nc_ctx_t *ctx, int opts, const npf_addr_t *netaddr,
    const npf_netmask_t mask)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 6 /* words */);
	const uint32_t *addr = (const uint32_t *)netaddr;

	assert(((opts & NC_MATCH_SRC) != 0) ^ ((opts & NC_MATCH_DST) != 0));
	assert((mask && mask <= NPF_MAX_NETMASK) || mask == NPF_NO_NETMASK);

	/* OP, direction, netaddr/subnet (4 words) */
	*nc++ = NPF_OPCODE_IP4MASK;
	*nc++ = (opts & (NC_MATCH_DST | NC_MATCH_SRC)) >> 1;
	*nc++ = addr[0];
	*nc++ = mask;

	/* Comparison block (2 words). */
	npfctl_ncgen_addjmp(ctx, &nc);

	/* + 6 words. */
	npfctl_ncgen_putptr(ctx, nc);
}

/*
 * npfctl_gennc_ports: fragment to match TCP or UDP ports.
 */
void
npfctl_gennc_ports(nc_ctx_t *ctx, int opts, in_port_t from, in_port_t to)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 5 /* words */);

	assert(((opts & NC_MATCH_SRC) != 0) ^ ((opts & NC_MATCH_DST) != 0));
	assert(((opts & NC_MATCH_TCP) != 0) ^ ((opts & NC_MATCH_UDP) != 0));

	/* OP, direction, port range (3 words). */
	*nc++ = (opts & NC_MATCH_TCP) ?
	    NPF_OPCODE_TCP_PORTS : NPF_OPCODE_UDP_PORTS;
	*nc++ = (opts & (NC_MATCH_DST | NC_MATCH_SRC)) >> 1;
	*nc++ = ((uint32_t)from << 16) | to;

	/* Comparison block (2 words). */
	npfctl_ncgen_addjmp(ctx, &nc);

	/* + 5 words. */
	npfctl_ncgen_putptr(ctx, nc);
}

/*
 * npfctl_gennc_icmp: fragment to match ICMP type and code.
 */
void
npfctl_gennc_icmp(nc_ctx_t *ctx, int type, int code)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 4 /* words */);

	/* OP, code, type (2 words) */
	*nc++ = NPF_OPCODE_ICMP4;
	*nc++ = (type == -1 ? 0 : (1 << 31) | ((type & 0xff) << 8)) |
		(code == -1 ? 0 : (1 << 30) | (code & 0xff));

	/* Comparison block (2 words). */
	npfctl_ncgen_addjmp(ctx, &nc);

	/* + 4 words. */
	npfctl_ncgen_putptr(ctx, nc);
}

/*
 * npfctl_gennc_icmp6: fragment to match ICMPV6 type and code.
 */
void
npfctl_gennc_icmp6(nc_ctx_t *ctx, int type, int code)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 4 /* words */);

	/* OP, code, type (2 words) */
	*nc++ = NPF_OPCODE_ICMP6;
	*nc++ = (type == -1 ? 0 : (1 << 31) | ((type & 0xff) << 8)) |
		(code == -1 ? 0 : (1 << 30) | (code & 0xff));

	/* Comparison block (2 words). */
	npfctl_ncgen_addjmp(ctx, &nc);

	/* + 4 words. */
	npfctl_ncgen_putptr(ctx, nc);
}

/*
 * npfctl_gennc_tbl: fragment to match IPv4 source/destination address of
 * the packet against table specified by ID.
 */
void
npfctl_gennc_tbl(nc_ctx_t *ctx, int opts, u_int tableid)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 5 /* words */);

	assert(((opts & NC_MATCH_SRC) != 0) ^ ((opts & NC_MATCH_DST) != 0));

	/* OP, direction, table ID (3 words). */
	*nc++ = NPF_OPCODE_TABLE;
	*nc++ = (opts & (NC_MATCH_DST | NC_MATCH_SRC)) >> 1;
	*nc++ = tableid;

	/* Comparison block (2 words). */
	npfctl_ncgen_addjmp(ctx, &nc);

	/* + 5 words. */
	npfctl_ncgen_putptr(ctx, nc);
}

/*
 * npfctl_gennc_tcpfl: fragment to match TCP flags/mask.
 */
void
npfctl_gennc_tcpfl(nc_ctx_t *ctx, uint8_t tf, uint8_t tf_mask)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 4 /* words */);

	/* OP, code, type (2 words) */
	*nc++ = NPF_OPCODE_TCP_FLAGS;
	*nc++ = (tf << 8) | tf_mask;

	/* Comparison block (2 words). */
	npfctl_ncgen_addjmp(ctx, &nc);

	/* + 4 words. */
	npfctl_ncgen_putptr(ctx, nc);
}

/*
 * npfctl_gennc_proto: fragment to match the protocol.
 */
void
npfctl_gennc_proto(nc_ctx_t *ctx, uint8_t addrlen, uint8_t proto)
{
	uint32_t *nc = npfctl_ncgen_getptr(ctx, 4 /* words */);

	/* OP, code, type (2 words) */
	*nc++ = NPF_OPCODE_PROTO;
	*nc++ = ((addrlen & 0xff) << 8) | (proto & 0xff);

	/* Comparison block (2 words). */
	npfctl_ncgen_addjmp(ctx, &nc);

	/* + 4 words. */
	npfctl_ncgen_putptr(ctx, nc);
}

void
npfctl_ncgen_print(const void *code, size_t len)
{
#if 0
	const uint32_t *op = code;

	while (len) {
		printf("\t> |0x%02x|\n", (u_int)*op++);
		len -= sizeof(*op);
	}
#else
	nc_inf_t *ni = npfctl_ncode_disinf(stdout);
	npfctl_ncode_disassemble(ni, code, len);
	free(ni);
#endif
}
