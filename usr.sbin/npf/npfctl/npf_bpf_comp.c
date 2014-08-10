/*	$NetBSD: npf_bpf_comp.c,v 1.4.2.1 2014/08/10 07:00:01 tls Exp $	*/

/*-
 * Copyright (c) 2010-2014 The NetBSD Foundation, Inc.
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
 * BPF byte-code generation for NPF rules.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_bpf_comp.c,v 1.4.2.1 2014/08/10 07:00:01 tls Exp $");

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <err.h>
#include <assert.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

#include <net/bpf.h>

#include "npfctl.h"

/*
 * Note: clear X_EQ_L4OFF when register X is invalidated i.e. it stores
 * something other than L4 header offset.  Generally, when BPF_LDX is used.
 */
#define	FETCHED_L3		0x01
#define	CHECKED_L4		0x02
#define	X_EQ_L4OFF		0x04

struct npf_bpf {
	/*
	 * BPF program code, the allocated length (in bytes), the number
	 * of logical blocks and the flags.
	 */
	struct bpf_program	prog;
	size_t			alen;
	u_int			nblocks;
	sa_family_t		af;
	uint32_t		flags;

	/* The current group offset and block number. */
	bool			ingroup;
	u_int			goff;
	u_int			gblock;

	/* BPF marks, allocated length and the real length. */
	uint32_t *		marks;
	size_t			malen;
	size_t			mlen;
};

/*
 * NPF success and failure values to be returned from BPF.
 */
#define	NPF_BPF_SUCCESS		((u_int)-1)
#define	NPF_BPF_FAILURE		0

/*
 * Magic value to indicate the failure path, which is fixed up on completion.
 * Note: this is the longest jump offset in BPF, since the offset is one byte.
 */
#define	JUMP_MAGIC		0xff

/* Reduce re-allocations by expanding in 64 byte blocks. */
#define	ALLOC_MASK		(64 - 1)
#define	ALLOC_ROUND(x)		(((x) + ALLOC_MASK) & ~ALLOC_MASK)

npf_bpf_t *
npfctl_bpf_create(void)
{
	return ecalloc(1, sizeof(npf_bpf_t));
}

static void
fixup_jumps(npf_bpf_t *ctx, u_int start, u_int end, bool swap)
{
	struct bpf_program *bp = &ctx->prog;

	for (u_int i = start; i < end; i++) {
		struct bpf_insn *insn = &bp->bf_insns[i];
		const u_int fail_off = end - i;

		if (fail_off >= JUMP_MAGIC) {
			errx(EXIT_FAILURE, "BPF generation error: "
			    "the number of instructions is over the limit");
		}
		if (BPF_CLASS(insn->code) != BPF_JMP) {
			continue;
		}
		if (swap) {
			uint8_t jt = insn->jt;
			insn->jt = insn->jf;
			insn->jf = jt;
		}
		if (insn->jt == JUMP_MAGIC)
			insn->jt = fail_off;
		if (insn->jf == JUMP_MAGIC)
			insn->jf = fail_off;
	}
}

static void
add_insns(npf_bpf_t *ctx, struct bpf_insn *insns, size_t count)
{
	struct bpf_program *bp = &ctx->prog;
	size_t offset, len, reqlen;

	/* Note: bf_len is the count of instructions. */
	offset = bp->bf_len * sizeof(struct bpf_insn);
	len = count * sizeof(struct bpf_insn);

	/* Ensure the memory buffer for the program. */
	reqlen = ALLOC_ROUND(offset + len);
	if (reqlen > ctx->alen) {
		bp->bf_insns = erealloc(bp->bf_insns, reqlen);
		ctx->alen = reqlen;
	}

	/* Add the code block. */
	memcpy((uint8_t *)bp->bf_insns + offset, insns, len);
	bp->bf_len += count;
}

static void
done_raw_block(npf_bpf_t *ctx, const uint32_t *m, size_t len)
{
	size_t reqlen, nargs = m[1];

	if ((len / sizeof(uint32_t) - 2) != nargs) {
		errx(EXIT_FAILURE, "invalid BPF block description");
	}
	reqlen = ALLOC_ROUND(ctx->mlen + len);
	if (reqlen > ctx->malen) {
		ctx->marks = erealloc(ctx->marks, reqlen);
		ctx->malen = reqlen;
	}
	memcpy((uint8_t *)ctx->marks + ctx->mlen, m, len);
	ctx->mlen += len;
}

static void
done_block(npf_bpf_t *ctx, const uint32_t *m, size_t len)
{
	done_raw_block(ctx, m, len);
	ctx->nblocks++;
}

struct bpf_program *
npfctl_bpf_complete(npf_bpf_t *ctx)
{
	struct bpf_program *bp = &ctx->prog;
	const u_int retoff = bp->bf_len;

	/* Add the return fragment (success and failure paths). */
	struct bpf_insn insns_ret[] = {
		BPF_STMT(BPF_RET+BPF_K, NPF_BPF_SUCCESS),
		BPF_STMT(BPF_RET+BPF_K, NPF_BPF_FAILURE),
	};
	add_insns(ctx, insns_ret, __arraycount(insns_ret));

	/* Fixup all jumps to the main failure path. */
	fixup_jumps(ctx, 0, retoff, false);

	return &ctx->prog;
}

const void *
npfctl_bpf_bmarks(npf_bpf_t *ctx, size_t *len)
{
	*len = ctx->mlen;
	return ctx->marks;
}

void
npfctl_bpf_destroy(npf_bpf_t *ctx)
{
	free(ctx->prog.bf_insns);
	free(ctx->marks);
	free(ctx);
}

/*
 * npfctl_bpf_group: begin a logical group.  It merely uses logical
 * disjunction (OR) for compares within the group.
 */
void
npfctl_bpf_group(npf_bpf_t *ctx)
{
	struct bpf_program *bp = &ctx->prog;

	assert(ctx->goff == 0);
	assert(ctx->gblock == 0);

	ctx->goff = bp->bf_len;
	ctx->gblock = ctx->nblocks;
	ctx->ingroup = true;
}

void
npfctl_bpf_endgroup(npf_bpf_t *ctx)
{
	struct bpf_program *bp = &ctx->prog;
	const size_t curoff = bp->bf_len;

	/* If there are no blocks or only one - nothing to do. */
	if ((ctx->nblocks - ctx->gblock) <= 1) {
		ctx->goff = ctx->gblock = 0;
		return;
	}

	/*
	 * Append a failure return as a fall-through i.e. if there is
	 * no match within the group.
	 */
	struct bpf_insn insns_ret[] = {
		BPF_STMT(BPF_RET+BPF_K, NPF_BPF_FAILURE),
	};
	add_insns(ctx, insns_ret, __arraycount(insns_ret));

	/*
	 * Adjust jump offsets: on match - jump outside the group i.e.
	 * to the current offset.  Otherwise, jump to the next instruction
	 * which would lead to the fall-through code above if none matches.
	 */
	fixup_jumps(ctx, ctx->goff, curoff, true);
	ctx->goff = ctx->gblock = 0;
}

static void
fetch_l3(npf_bpf_t *ctx, sa_family_t af, u_int flags)
{
	u_int ver;

	switch (af) {
	case AF_INET:
		ver = IPVERSION;
		break;
	case AF_INET6:
		ver = IPV6_VERSION >> 4;
		break;
	case AF_UNSPEC:
		ver = 0;
		break;
	default:
		abort();
	}

	/*
	 * The memory store is populated with:
	 * - BPF_MW_IPVER: IP version (4 or 6).
	 * - BPF_MW_L4OFF: L4 header offset.
	 * - BPF_MW_L4PROTO: L4 protocol.
	 */
	if ((ctx->flags & FETCHED_L3) == 0 || (af && ctx->af == 0)) {
		const uint8_t jt = ver ? 0 : JUMP_MAGIC;
		const uint8_t jf = ver ? JUMP_MAGIC : 0;
		bool ingroup = ctx->ingroup;

		/*
		 * L3 block cannot be inserted in the middle of a group.
		 * In fact, it never is.  Check and start the group after.
		 */
		if (ingroup) {
			assert(ctx->nblocks == ctx->gblock);
			npfctl_bpf_endgroup(ctx);
		}

		/*
		 * A <- IP version; A == expected-version?
		 * If no particular version specified, check for non-zero.
		 */
		struct bpf_insn insns_af[] = {
			BPF_STMT(BPF_LD+BPF_W+BPF_MEM, BPF_MW_IPVER),
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ver, jt, jf),
		};
		add_insns(ctx, insns_af, __arraycount(insns_af));
		ctx->flags |= FETCHED_L3;
		ctx->af = af;

		if (af) {
			uint32_t mwords[] = { BM_IPVER, 1, af };
			done_raw_block(ctx, mwords, sizeof(mwords));
		}
		if (ingroup) {
			npfctl_bpf_group(ctx);
		}

	} else if (af && af != ctx->af) {
		errx(EXIT_FAILURE, "address family mismatch");
	}

	if ((flags & X_EQ_L4OFF) != 0 && (ctx->flags & X_EQ_L4OFF) == 0) {
		/* X <- IP header length */
		struct bpf_insn insns_hlen[] = {
			BPF_STMT(BPF_LDX+BPF_MEM, BPF_MW_L4OFF),
		};
		add_insns(ctx, insns_hlen, __arraycount(insns_hlen));
		ctx->flags |= X_EQ_L4OFF;
	}
}

/*
 * npfctl_bpf_proto: code block to match IP version and L4 protocol.
 */
void
npfctl_bpf_proto(npf_bpf_t *ctx, sa_family_t af, int proto)
{
	assert(af != AF_UNSPEC || proto != -1);

	/* Note: fails if IP version does not match. */
	fetch_l3(ctx, af, 0);
	if (proto == -1) {
		return;
	}

	struct bpf_insn insns_proto[] = {
		/* A <- L4 protocol; A == expected-protocol? */
		BPF_STMT(BPF_LD+BPF_W+BPF_MEM, BPF_MW_L4PROTO),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, proto, 0, JUMP_MAGIC),
	};
	add_insns(ctx, insns_proto, __arraycount(insns_proto));

	uint32_t mwords[] = { BM_PROTO, 1, proto };
	done_block(ctx, mwords, sizeof(mwords));
	ctx->flags |= CHECKED_L4;
}

/*
 * npfctl_bpf_cidr: code block to match IPv4 or IPv6 CIDR.
 *
 * => IP address shall be in the network byte order.
 */
void
npfctl_bpf_cidr(npf_bpf_t *ctx, u_int opts, sa_family_t af,
    const npf_addr_t *addr, const npf_netmask_t mask)
{
	const uint32_t *awords = (const uint32_t *)addr;
	u_int nwords, length, maxmask, off;

	assert(((opts & MATCH_SRC) != 0) ^ ((opts & MATCH_DST) != 0));
	assert((mask && mask <= NPF_MAX_NETMASK) || mask == NPF_NO_NETMASK);

	switch (af) {
	case AF_INET:
		maxmask = 32;
		off = (opts & MATCH_SRC) ?
		    offsetof(struct ip, ip_src) :
		    offsetof(struct ip, ip_dst);
		nwords = sizeof(struct in_addr) / sizeof(uint32_t);
		break;
	case AF_INET6:
		maxmask = 128;
		off = (opts & MATCH_SRC) ?
		    offsetof(struct ip6_hdr, ip6_src) :
		    offsetof(struct ip6_hdr, ip6_dst);
		nwords = sizeof(struct in6_addr) / sizeof(uint32_t);
		break;
	default:
		abort();
	}

	/* Ensure address family. */
	fetch_l3(ctx, af, 0);

	length = (mask == NPF_NO_NETMASK) ? maxmask : mask;

	/* CAUTION: BPF operates in host byte-order. */
	for (u_int i = 0; i < nwords; i++) {
		const u_int woff = i * sizeof(uint32_t);
		uint32_t word = ntohl(awords[i]);
		uint32_t wordmask;

		if (length >= 32) {
			/* The mask is a full word - do not apply it. */
			wordmask = 0;
			length -= 32;
		} else if (length) {
			wordmask = 0xffffffff << (32 - length);
			length = 0;
		} else {
			/* The mask became zero - skip the rest. */
			break;
		}

		/* A <- IP address (or one word of it) */
		struct bpf_insn insns_ip[] = {
			BPF_STMT(BPF_LD+BPF_W+BPF_ABS, off + woff),
		};
		add_insns(ctx, insns_ip, __arraycount(insns_ip));

		/* A <- (A & MASK) */
		if (wordmask) {
			struct bpf_insn insns_mask[] = {
				BPF_STMT(BPF_ALU+BPF_AND+BPF_K, wordmask),
			};
			add_insns(ctx, insns_mask, __arraycount(insns_mask));
		}

		/* A == expected-IP-word ? */
		struct bpf_insn insns_cmp[] = {
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, word, 0, JUMP_MAGIC),
		};
		add_insns(ctx, insns_cmp, __arraycount(insns_cmp));
	}

	uint32_t mwords[] = {
		(opts & MATCH_SRC) ? BM_SRC_CIDR: BM_DST_CIDR, 6,
		af, mask, awords[0], awords[1], awords[2], awords[3],
	};
	done_block(ctx, mwords, sizeof(mwords));
}

/*
 * npfctl_bpf_ports: code block to match TCP/UDP port range.
 *
 * => Port numbers shall be in the network byte order.
 */
void
npfctl_bpf_ports(npf_bpf_t *ctx, u_int opts, in_port_t from, in_port_t to)
{
	const u_int sport_off = offsetof(struct udphdr, uh_sport);
	const u_int dport_off = offsetof(struct udphdr, uh_dport);
	u_int off;

	/* TCP and UDP port offsets are the same. */
	assert(sport_off == offsetof(struct tcphdr, th_sport));
	assert(dport_off == offsetof(struct tcphdr, th_dport));
	assert(ctx->flags & CHECKED_L4);

	assert(((opts & MATCH_SRC) != 0) ^ ((opts & MATCH_DST) != 0));
	off = (opts & MATCH_SRC) ? sport_off : dport_off;

	/* X <- IP header length */
	fetch_l3(ctx, AF_UNSPEC, X_EQ_L4OFF);

	struct bpf_insn insns_fetch[] = {
		/* A <- port */
		BPF_STMT(BPF_LD+BPF_H+BPF_IND, off),
	};
	add_insns(ctx, insns_fetch, __arraycount(insns_fetch));

	/* CAUTION: BPF operates in host byte-order. */
	from = ntohs(from);
	to = ntohs(to);

	if (from == to) {
		/* Single port case. */
		struct bpf_insn insns_port[] = {
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, from, 0, JUMP_MAGIC),
		};
		add_insns(ctx, insns_port, __arraycount(insns_port));
	} else {
		/* Port range case. */
		struct bpf_insn insns_range[] = {
			BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, from, 0, JUMP_MAGIC),
			BPF_JUMP(BPF_JMP+BPF_JGT+BPF_K, to, JUMP_MAGIC, 0),
		};
		add_insns(ctx, insns_range, __arraycount(insns_range));
	}

	uint32_t mwords[] = {
		opts & MATCH_SRC ? BM_SRC_PORTS : BM_DST_PORTS, 2, from, to
	};
	done_block(ctx, mwords, sizeof(mwords));
}

/*
 * npfctl_bpf_tcpfl: code block to match TCP flags.
 */
void
npfctl_bpf_tcpfl(npf_bpf_t *ctx, uint8_t tf, uint8_t tf_mask, bool checktcp)
{
	const u_int tcpfl_off = offsetof(struct tcphdr, th_flags);
	const bool usingmask = tf_mask != tf;

	/* X <- IP header length */
	fetch_l3(ctx, AF_UNSPEC, X_EQ_L4OFF);
	if (checktcp) {
		const u_int jf = usingmask ? 3 : 2;
		assert(ctx->ingroup == false);

		/* A <- L4 protocol; A == TCP?  If not, jump out. */
		struct bpf_insn insns_tcp[] = {
			BPF_STMT(BPF_LD+BPF_W+BPF_MEM, BPF_MW_L4PROTO),
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IPPROTO_TCP, 0, jf),
		};
		add_insns(ctx, insns_tcp, __arraycount(insns_tcp));
	} else {
		assert(ctx->flags & CHECKED_L4);
	}

	struct bpf_insn insns_tf[] = {
		/* A <- TCP flags */
		BPF_STMT(BPF_LD+BPF_B+BPF_IND, tcpfl_off),
	};
	add_insns(ctx, insns_tf, __arraycount(insns_tf));

	if (usingmask) {
		/* A <- (A & mask) */
		struct bpf_insn insns_mask[] = {
			BPF_STMT(BPF_ALU+BPF_AND+BPF_K, tf_mask),
		};
		add_insns(ctx, insns_mask, __arraycount(insns_mask));
	}

	struct bpf_insn insns_cmp[] = {
		/* A == expected-TCP-flags? */
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, tf, 0, JUMP_MAGIC),
	};
	add_insns(ctx, insns_cmp, __arraycount(insns_cmp));

	if (!checktcp) {
		uint32_t mwords[] = { BM_TCPFL, 2, tf, tf_mask};
		done_block(ctx, mwords, sizeof(mwords));
	}
}

/*
 * npfctl_bpf_icmp: code block to match ICMP type and/or code.
 * Note: suitable both for the ICMPv4 and ICMPv6.
 */
void
npfctl_bpf_icmp(npf_bpf_t *ctx, int type, int code)
{
	const u_int type_off = offsetof(struct icmp, icmp_type);
	const u_int code_off = offsetof(struct icmp, icmp_code);

	assert(ctx->flags & CHECKED_L4);
	assert(offsetof(struct icmp6_hdr, icmp6_type) == type_off);
	assert(offsetof(struct icmp6_hdr, icmp6_code) == code_off);
	assert(type != -1 || code != -1);

	/* X <- IP header length */
	fetch_l3(ctx, AF_UNSPEC, X_EQ_L4OFF);

	if (type != -1) {
		struct bpf_insn insns_type[] = {
			BPF_STMT(BPF_LD+BPF_B+BPF_IND, type_off),
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, type, 0, JUMP_MAGIC),
		};
		add_insns(ctx, insns_type, __arraycount(insns_type));

		uint32_t mwords[] = { BM_ICMP_TYPE, 1, type };
		done_block(ctx, mwords, sizeof(mwords));
	}

	if (code != -1) {
		struct bpf_insn insns_code[] = {
			BPF_STMT(BPF_LD+BPF_B+BPF_IND, code_off),
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, code, 0, JUMP_MAGIC),
		};
		add_insns(ctx, insns_code, __arraycount(insns_code));

		uint32_t mwords[] = { BM_ICMP_CODE, 1, code };
		done_block(ctx, mwords, sizeof(mwords));
	}
}

#define	SRC_FLAG_BIT	(1U << 31)

/*
 * npfctl_bpf_table: code block to match source/destination IP address
 * against NPF table specified by ID.
 */
void
npfctl_bpf_table(npf_bpf_t *ctx, u_int opts, u_int tid)
{
	const bool src = (opts & MATCH_SRC) != 0;

	struct bpf_insn insns_table[] = {
		BPF_STMT(BPF_LD+BPF_IMM, (src ? SRC_FLAG_BIT : 0) | tid),
		BPF_STMT(BPF_MISC+BPF_COP, NPF_COP_TABLE),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0, JUMP_MAGIC, 0),
	};
	add_insns(ctx, insns_table, __arraycount(insns_table));

	uint32_t mwords[] = { src ? BM_SRC_TABLE: BM_DST_TABLE, 1, tid };
	done_block(ctx, mwords, sizeof(mwords));
}
