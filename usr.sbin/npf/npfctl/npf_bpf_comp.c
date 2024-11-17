/*-
 * Copyright (c) 2010-2020 The NetBSD Foundation, Inc.
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
 *
 * Overview
 *
 *	Each NPF rule is compiled into a BPF micro-program.  There is a
 *	BPF byte-code fragment for each higher-level filtering logic,
 *	e.g. to match L4 protocol, IP/mask, etc.  The generation process
 *	combines multiple BPF-byte code fragments into one program.
 *
 * Basic case
 *
 *	Consider a basic case where all filters should match.  They
 *	are expressed as logical conjunction, e.g.:
 *
 *		A and B and C and D
 *
 *	Each test (filter) criterion can be evaluated to true (match) or
 *	false (no match) and the logic is as follows:
 *
 *	- If the value is true, then jump to the "next" test (offset 0).
 *
 *	- If the value is false, then jump to the JUMP_MAGIC value (0xff).
 *	This "magic" value is used to indicate that it will have to be
 *	patched at a later stage.
 *
 *	Once all byte-code fragments are combined into one, then there
 *	are two additional steps:
 *
 *	- Two instructions are appended at the end of the program: "return
 *	success" followed by "return failure".
 *
 *	- All jumps with the JUMP_MAGIC value are patched to point to the
 *	"return failure" instruction.
 *
 *	Therefore, if all filter criteria will match, then the first
 *	instruction will be reached, indicating a successful match of the
 *	rule.  Otherwise, if any of the criteria will not match, it will
 *	take the failure path and the rule will not be matching.
 *
 * Grouping
 *
 *	Filters can have groups, which have an effect of logical
 *	disjunction, e.g.:
 *
 *		A and B and (C or D)
 *
 *	In such case, the logic inside the group has to be inverted i.e.
 *	the jump values swapped.  If the test value is true, then jump
 *	out of the group; if false, then jump "next".  At the end of the
 *	group, an addition failure path is appended and the JUMP_MAGIC
 *	uses within the group are patched to jump past the said path.
 *
 *	For multi-word comparisons (IPv6 addresses), there is another
 *	layer of grouping:
 *
 *		A and B and ((C and D) or (E and F))
 *
 *	This strains the simple-minded JUMP_MAGIC logic, so for now,
 *	when generating the jump-if-false targets for (C and D), we
 *	simply count the number of instructions left to skip over.
 *
 *	A better architecture might be to create asm-type labels for
 *	the jt and jf continuations in the first pass, and then, once
 *	their offsets are determined, go back and fill them in in the
 *	second pass.  This would simplify the logic (no need to compute
 *	exactly how many instructions we're about to generate in a
 *	chain of conditionals) and eliminate redundant RET #0
 *	instructions which are currently generated after some groups.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_bpf_comp.c,v 1.16.6.1 2024/11/17 13:18:58 martin Exp $");

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <err.h>
#include <assert.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#define	__FAVOR_BSD
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
#define	CHECKED_L4_PROTO	0x02
#define	X_EQ_L4OFF		0x04

struct npf_bpf {
	/*
	 * BPF program code, the allocated length (in bytes), the number
	 * of logical blocks and the flags.
	 */
	struct bpf_program	prog;
	size_t			alen;
	unsigned		nblocks;
	sa_family_t		af;
	uint32_t		flags;

	/*
	 * Indicators whether we are inside the group and whether this
	 * group is implementing inverted logic.
	 *
	 * The current group offset (counted in BPF instructions)
	 * and block number at the start of the group.
	 */
	unsigned		ingroup;
	bool			invert;
	bool			multiword;
	unsigned		goff;
	unsigned		gblock;

	/* Track inversion (excl. mark). */
	uint32_t		invflags;

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

#ifndef IPV6_VERSION
#define	IPV6_VERSION		0x60
#endif

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
		bool seen_magic = false;

		if (fail_off >= JUMP_MAGIC) {
			errx(EXIT_FAILURE, "BPF generation error: "
			    "the number of instructions is over the limit");
		}
		if (BPF_CLASS(insn->code) != BPF_JMP) {
			continue;
		}
		if (BPF_OP(insn->code) == BPF_JA) {
			/*
			 * BPF_JA can be used to jump to the failure path.
			 * If we are swapping i.e. inside the group, then
			 * jump "next"; groups have a failure path appended
			 * at their end.
			 */
			if (insn->k == JUMP_MAGIC) {
				insn->k = swap ? 0 : fail_off;
			}
			continue;
		}

		/*
		 * Fixup the "magic" value.  Swap only the "magic" jumps.
		 */

		if (insn->jt == JUMP_MAGIC) {
			insn->jt = fail_off;
			seen_magic = true;
		}
		if (insn->jf == JUMP_MAGIC) {
			insn->jf = fail_off;
			seen_magic = true;
		}

		if (seen_magic && swap) {
			uint8_t jt = insn->jt;
			insn->jt = insn->jf;
			insn->jf = jt;
		}
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
add_bmarks(npf_bpf_t *ctx, const uint32_t *m, size_t len)
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
	add_bmarks(ctx, m, len);
	ctx->nblocks++;
}

struct bpf_program *
npfctl_bpf_complete(npf_bpf_t *ctx)
{
	struct bpf_program *bp = &ctx->prog;
	const u_int retoff = bp->bf_len;

	/* No instructions (optimised out). */
	if (!bp->bf_len)
		return NULL;

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
 * npfctl_bpf_group_enter: begin a logical group.  It merely uses logical
 * disjunction (OR) for comparisons within the group.
 */
void
npfctl_bpf_group_enter(npf_bpf_t *ctx, bool invert)
{
	struct bpf_program *bp = &ctx->prog;

	assert(ctx->goff == 0);
	assert(ctx->gblock == 0);

	ctx->goff = bp->bf_len;
	ctx->gblock = ctx->nblocks;
	ctx->invert = invert;
	ctx->multiword = false;
	ctx->ingroup++;
}

void
npfctl_bpf_group_exit(npf_bpf_t *ctx)
{
	struct bpf_program *bp = &ctx->prog;
	const size_t curoff = bp->bf_len;

	assert(ctx->ingroup);
	ctx->ingroup--;

	/*
	 * If we're not inverting, there were only zero or one options,
	 * and the last comparison was not a multi-word comparison
	 * requiring a fallthrough failure -- nothing to do.
	 */
	if (!ctx->invert &&
	    (ctx->nblocks - ctx->gblock) <= 1 &&
	    !ctx->multiword) {
		ctx->goff = ctx->gblock = 0;
		return;
	}

	/*
	 * If inverting, then prepend a jump over the statement below.
	 * On match, it will skip-through and the fail path will be taken.
	 */
	if (ctx->invert) {
		struct bpf_insn insns_ret[] = {
			BPF_STMT(BPF_JMP+BPF_JA, 1),
		};
		add_insns(ctx, insns_ret, __arraycount(insns_ret));
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
fetch_l3(npf_bpf_t *ctx, sa_family_t af, unsigned flags)
{
	unsigned ver;

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
		const bool ingroup = ctx->ingroup != 0;
		const bool invert = ctx->invert;

		/*
		 * L3 block cannot be inserted in the middle of a group.
		 * In fact, it never is.  Check and start the group after.
		 */
		if (ingroup) {
			assert(ctx->nblocks == ctx->gblock);
			npfctl_bpf_group_exit(ctx);
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
			add_bmarks(ctx, mwords, sizeof(mwords));
		}
		if (ingroup) {
			npfctl_bpf_group_enter(ctx, invert);
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

static void
bm_invert_checkpoint(npf_bpf_t *ctx, const unsigned opts)
{
	uint32_t bm = 0;

	if (ctx->ingroup && ctx->invert) {
		const unsigned seen = ctx->invflags;

		if ((opts & MATCH_SRC) != 0 && (seen & MATCH_SRC) == 0) {
			bm = BM_SRC_NEG;
		}
		if ((opts & MATCH_DST) != 0 && (seen & MATCH_DST) == 0) {
			bm = BM_DST_NEG;
		}
		ctx->invflags |= opts & (MATCH_SRC | MATCH_DST);
	}
	if (bm) {
		uint32_t mwords[] = { bm, 0 };
		add_bmarks(ctx, mwords, sizeof(mwords));
	}
}

/*
 * npfctl_bpf_ipver: match the IP version.
 */
void
npfctl_bpf_ipver(npf_bpf_t *ctx, sa_family_t af)
{
	fetch_l3(ctx, af, 0);
}

/*
 * npfctl_bpf_proto: code block to match IP version and L4 protocol.
 */
void
npfctl_bpf_proto(npf_bpf_t *ctx, unsigned proto)
{
	struct bpf_insn insns_proto[] = {
		/* A <- L4 protocol; A == expected-protocol? */
		BPF_STMT(BPF_LD+BPF_W+BPF_MEM, BPF_MW_L4PROTO),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, proto, 0, JUMP_MAGIC),
	};
	add_insns(ctx, insns_proto, __arraycount(insns_proto));

	uint32_t mwords[] = { BM_PROTO, 1, proto };
	done_block(ctx, mwords, sizeof(mwords));
	ctx->flags |= CHECKED_L4_PROTO;
}

/*
 * npfctl_bpf_cidr: code block to match IPv4 or IPv6 CIDR.
 *
 * => IP address shall be in the network byte order.
 */
void
npfctl_bpf_cidr(npf_bpf_t *ctx, unsigned opts, sa_family_t af,
    const npf_addr_t *addr, const npf_netmask_t mask)
{
	const uint32_t *awords = (const uint32_t *)addr;
	unsigned nwords, origlength, length, maxmask, off;

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

	length = origlength = (mask == NPF_NO_NETMASK) ? maxmask : mask;

	/* CAUTION: BPF operates in host byte-order. */
	for (unsigned i = 0; i < nwords; i++) {
		const unsigned woff = i * sizeof(uint32_t);
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

		/*
		 * Determine how many instructions we have to jump
		 * ahead if the match fails.
		 *
		 * - If this is the last word, we jump to the final
                 *   failure, JUMP_MAGIC.
		 *
		 * - If this is not the last word, we jump past the
		 *   remaining instructions to match this sequence.
		 *   Each 32-bit word in the sequence takes two
		 *   instructions (BPF_LD and BPF_JMP).  If there is a
		 *   partial-word mask ahead, there will be one
		 *   additional instruction (BPF_ALU).
		 */
		uint8_t jf;
		if (i + 1 == (origlength + 31)/32) {
			jf = JUMP_MAGIC;
		} else {
			jf = 2*((origlength + 31)/32 - i - 1);
			if (origlength % 32 != 0 && wordmask == 0)
				jf += 1;
		}

		/* A == expected-IP-word ? */
		struct bpf_insn insns_cmp[] = {
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, word, 0, jf),
		};
		add_insns(ctx, insns_cmp, __arraycount(insns_cmp));
	}

	/*
	 * If we checked a chain of words in sequence, mark this as a
	 * multi-word comparison so if this is in a group there will be
	 * a fallthrough case.
	 *
	 * XXX This is a little silly; the compiler should really just
	 * record holes where conditional jumps need success/failure
	 * continuations, and go back to fill in the holes when the
	 * locations of the continuations are determined later.  But
	 * that requires restructuring this code a little more.
	 */
	ctx->multiword = (origlength + 31)/32 > 1;

	uint32_t mwords[] = {
		(opts & MATCH_SRC) ? BM_SRC_CIDR: BM_DST_CIDR, 6,
		af, mask, awords[0], awords[1], awords[2], awords[3],
	};
	bm_invert_checkpoint(ctx, opts);
	done_block(ctx, mwords, sizeof(mwords));
}

/*
 * npfctl_bpf_ports: code block to match TCP/UDP port range.
 *
 * => Port numbers shall be in the network byte order.
 */
void
npfctl_bpf_ports(npf_bpf_t *ctx, unsigned opts, in_port_t from, in_port_t to)
{
	const unsigned sport_off = offsetof(struct udphdr, uh_sport);
	const unsigned dport_off = offsetof(struct udphdr, uh_dport);
	unsigned off;

	/* TCP and UDP port offsets are the same. */
	assert(sport_off == offsetof(struct tcphdr, th_sport));
	assert(dport_off == offsetof(struct tcphdr, th_dport));
	assert(ctx->flags & CHECKED_L4_PROTO);

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
			BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, from, 0, 1),
			BPF_JUMP(BPF_JMP+BPF_JGT+BPF_K, to, 0, 1),
			BPF_STMT(BPF_JMP+BPF_JA, JUMP_MAGIC),
		};
		add_insns(ctx, insns_range, __arraycount(insns_range));
	}

	uint32_t mwords[] = {
		(opts & MATCH_SRC) ? BM_SRC_PORTS : BM_DST_PORTS, 2, from, to
	};
	done_block(ctx, mwords, sizeof(mwords));
}

/*
 * npfctl_bpf_tcpfl: code block to match TCP flags.
 */
void
npfctl_bpf_tcpfl(npf_bpf_t *ctx, uint8_t tf, uint8_t tf_mask)
{
	const unsigned tcpfl_off = offsetof(struct tcphdr, th_flags);
	const bool usingmask = tf_mask != tf;

	/* X <- IP header length */
	fetch_l3(ctx, AF_UNSPEC, X_EQ_L4OFF);

	if ((ctx->flags & CHECKED_L4_PROTO) == 0) {
		const unsigned jf = usingmask ? 3 : 2;
		assert(ctx->ingroup == 0);

		/*
		 * A <- L4 protocol; A == TCP?  If not, jump out.
		 *
		 * Note: the TCP flag matching might be without 'proto tcp'
		 * when using a plain 'stateful' rule.  In such case it also
		 * handles other protocols, thus no strict TCP check.
		 */
		struct bpf_insn insns_tcp[] = {
			BPF_STMT(BPF_LD+BPF_W+BPF_MEM, BPF_MW_L4PROTO),
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IPPROTO_TCP, 0, jf),
		};
		add_insns(ctx, insns_tcp, __arraycount(insns_tcp));
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

	uint32_t mwords[] = { BM_TCPFL, 2, tf, tf_mask };
	done_block(ctx, mwords, sizeof(mwords));
}

/*
 * npfctl_bpf_icmp: code block to match ICMP type and/or code.
 * Note: suitable for both the ICMPv4 and ICMPv6.
 */
void
npfctl_bpf_icmp(npf_bpf_t *ctx, int type, int code)
{
	const u_int type_off = offsetof(struct icmp, icmp_type);
	const u_int code_off = offsetof(struct icmp, icmp_code);

	assert(ctx->flags & CHECKED_L4_PROTO);
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
npfctl_bpf_table(npf_bpf_t *ctx, unsigned opts, unsigned tid)
{
	const bool src = (opts & MATCH_SRC) != 0;

	struct bpf_insn insns_table[] = {
		BPF_STMT(BPF_LD+BPF_IMM, (src ? SRC_FLAG_BIT : 0) | tid),
		BPF_STMT(BPF_MISC+BPF_COP, NPF_COP_TABLE),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0, JUMP_MAGIC, 0),
	};
	add_insns(ctx, insns_table, __arraycount(insns_table));

	uint32_t mwords[] = { src ? BM_SRC_TABLE: BM_DST_TABLE, 1, tid };
	bm_invert_checkpoint(ctx, opts);
	done_block(ctx, mwords, sizeof(mwords));
}
