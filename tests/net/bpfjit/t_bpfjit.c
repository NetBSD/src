/*	$NetBSD: t_bpfjit.c,v 1.1 2014/06/30 21:30:51 alnsn Exp $	*/

/*-
 * Copyright (c) 2014 Alexander Nasonov.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_bpfjit.c,v 1.1 2014/06/30 21:30:51 alnsn Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>
#include <unistd.h>

#include <net/bpf.h>
#include <net/bpfjit.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

/* XXX: atf-c.h has collisions with mbuf */
#undef m_type
#undef m_data
#include <atf-c.h>

#include "../../h_macros.h"

/* XXX These declarations don't look kosher. */
bpfjit_func_t rumpns_bpfjit_generate_code(const bpf_ctx_t *,
    const struct bpf_insn *, size_t);
void rumpns_bpfjit_free_code(bpfjit_func_t);

static bool
test_ldb_abs(size_t split)
{
	static char P[] = { 1, 2, 3, 4, 5 };

	/* Return a product of all packet bytes. */
	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LDX+BPF_W+BPF_IMM, 1), /* X <- 1     */

		BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 0),  /* A <- P[0]  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X */
		BPF_STMT(BPF_MISC+BPF_TAX, 0),      /* X <- A     */

		BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 1),  /* A <- P[1]  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X */
		BPF_STMT(BPF_MISC+BPF_TAX, 0),      /* X <- A     */

		BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 2),  /* A <- P[2]  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X */
		BPF_STMT(BPF_MISC+BPF_TAX, 0),      /* X <- A     */

		BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 3),  /* A <- P[3]  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X */
		BPF_STMT(BPF_MISC+BPF_TAX, 0),      /* X <- A     */

		BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 4),  /* A <- P[4]  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X */
		BPF_STMT(BPF_RET+BPF_A, 0),         /* ret A      */
	};

	const size_t insn_count = sizeof(insns) / sizeof(insns[0]);

	bpfjit_func_t fn;
	bpf_args_t args;
	struct mbuf mb1, mb2;
	unsigned int res;

	(void)memset(&mb1, 0, sizeof(mb1));
	mb1.m_hdr.mh_data = P;
	mb1.m_next = (split < sizeof(P)) ? &mb2 : NULL;
	mb1.m_len = (split < sizeof(P)) ? split : sizeof(P);

	if (split < sizeof(P)) {
		(void)memset(&mb2, 0, sizeof(mb2));
		mb2.m_next = NULL;
		mb2.m_hdr.mh_data = &P[split];
		mb2.m_len = sizeof(P) - split;
	}

	args.pkt = (const uint8_t*)&mb1;
	args.buflen = 0;
	args.wirelen = sizeof(P);

	rump_schedule();
	fn = rumpns_bpfjit_generate_code(NULL, insns, insn_count);
	rump_unschedule();

	ATF_REQUIRE(fn != NULL);

	res = fn(NULL, &args);

	rump_schedule();
	rumpns_bpfjit_free_code(fn);
	rump_unschedule();

	return res == 120;
}

static bool
test_ldh_abs(size_t split)
{
	static char P[] = { 1, 2, 3, 4, 5 };

	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 0),  /* A <- P[0:2]  */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X   */
		BPF_STMT(BPF_MISC+BPF_TAX, 0),      /* X <- A       */

		BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 1),  /* A <- P[1:2]  */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X   */
		BPF_STMT(BPF_MISC+BPF_TAX, 0),      /* X <- A       */

		BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 2),  /* A <- P[2:2]  */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X   */
		BPF_STMT(BPF_MISC+BPF_TAX, 0),      /* X <- A       */

		BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 3),  /* A <- P[3:2]  */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X   */
		BPF_STMT(BPF_RET+BPF_A, 0),         /* ret A        */
	};

	const size_t insn_count = sizeof(insns) / sizeof(insns[0]);

	bpfjit_func_t fn;
	bpf_args_t args;
	struct mbuf mb1, mb2;
	unsigned int res;

	(void)memset(&mb1, 0, sizeof(mb1));
	mb1.m_hdr.mh_data = P;
	mb1.m_next = (split < sizeof(P)) ? &mb2 : NULL;
	mb1.m_len = (split < sizeof(P)) ? split : sizeof(P);

	if (split < sizeof(P)) {
		(void)memset(&mb2, 0, sizeof(mb2));
		mb2.m_next = NULL;
		mb2.m_hdr.mh_data = &P[split];
		mb2.m_len = sizeof(P) - split;
	}

	args.pkt = (const uint8_t*)&mb1;
	args.buflen = 0;
	args.wirelen = sizeof(P);

	rump_schedule();
	fn = rumpns_bpfjit_generate_code(NULL, insns, insn_count);
	rump_unschedule();

	ATF_REQUIRE(fn != NULL);

	res = fn(NULL, &args);

	rump_schedule();
	rumpns_bpfjit_free_code(fn);
	rump_unschedule();

	return res == 0x0a0e; /* 10 14 */
}

static bool
test_ldw_abs(size_t split)
{
	static char P[] = { 1, 2, 3, 4, 5 };

	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 0),  /* A <- P[0:4] */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X  */
		BPF_STMT(BPF_MISC+BPF_TAX, 0),      /* X <- A       */

		BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 1),  /* A <- P[1:4] */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X  */
		BPF_STMT(BPF_RET+BPF_A, 0),         /* ret A       */
	};

	const size_t insn_count = sizeof(insns) / sizeof(insns[0]);

	bpfjit_func_t fn;
	bpf_args_t args;
	struct mbuf mb1, mb2;
	unsigned int res;

	(void)memset(&mb1, 0, sizeof(mb1));
	mb1.m_hdr.mh_data = P;
	mb1.m_next = (split < sizeof(P)) ? &mb2 : NULL;
	mb1.m_len = (split < sizeof(P)) ? split : sizeof(P);

	if (split < sizeof(P)) {
		(void)memset(&mb2, 0, sizeof(mb2));
		mb2.m_next = NULL;
		mb2.m_hdr.mh_data = &P[split];
		mb2.m_len = sizeof(P) - split;
	}

	args.pkt = (const uint8_t*)&mb1;
	args.buflen = 0;
	args.wirelen = sizeof(P);

	rump_schedule();
	fn = rumpns_bpfjit_generate_code(NULL, insns, insn_count);
	rump_unschedule();

	ATF_REQUIRE(fn != NULL);

	res = fn(NULL, &args);
	printf("0x%08x\n", res);

	rump_schedule();
	rumpns_bpfjit_free_code(fn);
	rump_unschedule();

	return res == 0x03050709;
}

static bool
test_ldb_ind(size_t split)
{
	static char P[] = { 1, 2, 3, 4, 5 };

	/* Return a sum of all packet bytes. */
	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LD+BPF_B+BPF_IND, 0),  /* A <- P[0+X] */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A   */

		BPF_STMT(BPF_LD+BPF_B+BPF_IND, 1),  /* A <- P[1+X] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]   */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X  */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A   */

		BPF_STMT(BPF_LDX+BPF_W+BPF_IMM, 1), /* X <- 1      */
		BPF_STMT(BPF_LD+BPF_B+BPF_IND, 1),  /* A <- P[1+X] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]   */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X  */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A   */

		BPF_STMT(BPF_LDX+BPF_W+BPF_IMM, 1), /* X <- 1      */
		BPF_STMT(BPF_LD+BPF_B+BPF_IND, 2),  /* A <- P[2+X] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]   */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X  */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A   */

		BPF_STMT(BPF_LDX+BPF_W+BPF_IMM, 1), /* X <- 1      */
		BPF_STMT(BPF_LD+BPF_B+BPF_IND, 3),  /* A <- P[3+X] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]   */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X  */
		BPF_STMT(BPF_RET+BPF_A, 0),         /* ret A       */
	};

	const size_t insn_count = sizeof(insns) / sizeof(insns[0]);

	bpfjit_func_t fn;
	bpf_args_t args;
	struct mbuf mb1, mb2;
	unsigned int res;

	(void)memset(&mb1, 0, sizeof(mb1));
	mb1.m_hdr.mh_data = P;
	mb1.m_next = (split < sizeof(P)) ? &mb2 : NULL;
	mb1.m_len = (split < sizeof(P)) ? split : sizeof(P);

	if (split < sizeof(P)) {
		(void)memset(&mb2, 0, sizeof(mb2));
		mb2.m_next = NULL;
		mb2.m_hdr.mh_data = &P[split];
		mb2.m_len = sizeof(P) - split;
	}

	args.pkt = (const uint8_t*)&mb1;
	args.buflen = 0;
	args.wirelen = sizeof(P);

	rump_schedule();
	fn = rumpns_bpfjit_generate_code(NULL, insns, insn_count);
	rump_unschedule();

	ATF_REQUIRE(fn != NULL);

	res = fn(NULL, &args);

	rump_schedule();
	rumpns_bpfjit_free_code(fn);
	rump_unschedule();

	return res == 15;
}

static bool
test_ldw_ind(size_t split)
{
	static char P[] = { 1, 2, 3, 4, 5 };

	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_IND, 0),  /* A <- P[X+0:4] */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A     */

		BPF_STMT(BPF_LDX+BPF_W+BPF_IMM, 1), /* X <- 1        */
		BPF_STMT(BPF_LD+BPF_W+BPF_IND, 0),  /* A <- P[X+0:4] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]     */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X    */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A     */

		BPF_STMT(BPF_LDX+BPF_W+BPF_IMM, 0), /* X <- 0        */
		BPF_STMT(BPF_LD+BPF_W+BPF_IND, 1),  /* A <- P[X+1:4] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]     */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X    */
		BPF_STMT(BPF_RET+BPF_A, 0),         /* ret A         */
	};

	const size_t insn_count = sizeof(insns) / sizeof(insns[0]);

	bpfjit_func_t fn;
	bpf_args_t args;
	struct mbuf mb1, mb2;
	unsigned int res;

	(void)memset(&mb1, 0, sizeof(mb1));
	mb1.m_hdr.mh_data = P;
	mb1.m_next = (split < sizeof(P)) ? &mb2 : NULL;
	mb1.m_len = (split < sizeof(P)) ? split : sizeof(P);

	if (split < sizeof(P)) {
		(void)memset(&mb2, 0, sizeof(mb2));
		mb2.m_next = NULL;
		mb2.m_hdr.mh_data = &P[split];
		mb2.m_len = sizeof(P) - split;
	}

	args.pkt = (const uint8_t*)&mb1;
	args.buflen = 0;
	args.wirelen = sizeof(P);

	rump_schedule();
	fn = rumpns_bpfjit_generate_code(NULL, insns, insn_count);
	rump_unschedule();

	ATF_REQUIRE(fn != NULL);

	res = fn(NULL, &args);

	rump_schedule();
	rumpns_bpfjit_free_code(fn);
	rump_unschedule();

	return res == 0x05080b0e;
}

static bool
test_ldh_ind(size_t split)
{
	static char P[] = { 1, 2, 3, 4, 5 };

	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LD+BPF_H+BPF_IND, 0),  /* A <- P[X+0:2] */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A     */

		BPF_STMT(BPF_LD+BPF_H+BPF_IND, 1),  /* A <- P[X+1:2] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]     */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X    */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A     */

		BPF_STMT(BPF_LDX+BPF_W+BPF_IMM, 1), /* X <- 1        */
		BPF_STMT(BPF_LD+BPF_H+BPF_IND, 1),  /* A <- P[X+1:2] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]     */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X    */
		BPF_STMT(BPF_ST, 0),                /* M[0] <- A     */

		BPF_STMT(BPF_LDX+BPF_W+BPF_IMM, 1), /* X <- 1        */
		BPF_STMT(BPF_LD+BPF_H+BPF_IND, 2),  /* A <- P[X+2:2] */
		BPF_STMT(BPF_LDX+BPF_W+BPF_MEM, 0), /* X <- M[0]     */
		BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0), /* A <- A + X    */
		BPF_STMT(BPF_RET+BPF_A, 0),         /* ret A         */
	};

	const size_t insn_count = sizeof(insns) / sizeof(insns[0]);

	bpfjit_func_t fn;
	bpf_args_t args;
	struct mbuf mb1, mb2;
	unsigned int res;

	(void)memset(&mb1, 0, sizeof(mb1));
	mb1.m_hdr.mh_data = P;
	mb1.m_next = (split < sizeof(P)) ? &mb2 : NULL;
	mb1.m_len = (split < sizeof(P)) ? split : sizeof(P);

	if (split < sizeof(P)) {
		(void)memset(&mb2, 0, sizeof(mb2));
		mb2.m_next = NULL;
		mb2.m_hdr.mh_data = &P[split];
		mb2.m_len = sizeof(P) - split;
	}

	args.pkt = (const uint8_t*)&mb1;
	args.buflen = 0;
	args.wirelen = sizeof(P);

	rump_schedule();
	fn = rumpns_bpfjit_generate_code(NULL, insns, insn_count);
	rump_unschedule();

	ATF_REQUIRE(fn != NULL);

	res = fn(NULL, &args);

	rump_schedule();
	rumpns_bpfjit_free_code(fn);
	rump_unschedule();

	return res == 0x0a0e; /* 10 14 */
}

static bool
test_msh(size_t split)
{
	static char P[] = { 1, 2, 3, 4, 5 };

	/* Return a product of all packet bytes. */
	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LD+BPF_IMM, 1),        /* A <- 1     */

		BPF_STMT(BPF_LDX+BPF_B+BPF_MSH, 0), /* X <- 4*(P[0]&0xf)  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X         */
		BPF_STMT(BPF_ALU+BPF_DIV+BPF_K, 4), /* A <- A / 4         */

		BPF_STMT(BPF_LDX+BPF_B+BPF_MSH, 1), /* X <- 4*(P[1]&0xf)  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X         */
		BPF_STMT(BPF_ALU+BPF_DIV+BPF_K, 4), /* A <- A / 4         */

		BPF_STMT(BPF_LDX+BPF_B+BPF_MSH, 2), /* X <- 4*(P[2]&0xf)  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X         */
		BPF_STMT(BPF_ALU+BPF_DIV+BPF_K, 4), /* A <- A / 4         */

		BPF_STMT(BPF_LDX+BPF_B+BPF_MSH, 3), /* X <- 4*(P[3]&0xf)  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X         */
		BPF_STMT(BPF_ALU+BPF_DIV+BPF_K, 4), /* A <- A / 4         */

		BPF_STMT(BPF_LDX+BPF_B+BPF_MSH, 4), /* X <- 4*(P[4]&0xf)  */
		BPF_STMT(BPF_ALU+BPF_MUL+BPF_X, 0), /* A <- A * X         */
		BPF_STMT(BPF_ALU+BPF_DIV+BPF_K, 4), /* A <- A / 4         */

		BPF_STMT(BPF_RET+BPF_A, 0),         /* ret A      */
	};

	const size_t insn_count = sizeof(insns) / sizeof(insns[0]);

	bpfjit_func_t fn;
	bpf_args_t args;
	struct mbuf mb1, mb2;
	unsigned int res;

	(void)memset(&mb1, 0, sizeof(mb1));
	mb1.m_hdr.mh_data = P;
	mb1.m_next = (split < sizeof(P)) ? &mb2 : NULL;
	mb1.m_len = (split < sizeof(P)) ? split : sizeof(P);

	if (split < sizeof(P)) {
		(void)memset(&mb2, 0, sizeof(mb2));
		mb2.m_next = NULL;
		mb2.m_hdr.mh_data = &P[split];
		mb2.m_len = sizeof(P) - split;
	}

	args.pkt = (const uint8_t*)&mb1;
	args.buflen = 0;
	args.wirelen = sizeof(P);

	rump_schedule();
	fn = rumpns_bpfjit_generate_code(NULL, insns, insn_count);
	rump_unschedule();

	res = fn(NULL, &args);

	rump_schedule();
	rumpns_bpfjit_free_code(fn);
	rump_unschedule();

	return res == 120;
}

ATF_TC(bpfjit_mbuf_ldb_abs);
ATF_TC_HEAD(bpfjit_mbuf_ldb_abs, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that BPF_LD+BPF_B+BPF_ABS "
	    "loads bytes from mbuf correctly");
}

ATF_TC_BODY(bpfjit_mbuf_ldb_abs, tc)
{

	RZ(rump_init());

	ATF_CHECK(test_ldb_abs(0));
	ATF_CHECK(test_ldb_abs(1));
	ATF_CHECK(test_ldb_abs(2));
	ATF_CHECK(test_ldb_abs(3));
	ATF_CHECK(test_ldb_abs(4));
	ATF_CHECK(test_ldb_abs(5));
}

ATF_TC(bpfjit_mbuf_ldh_abs);
ATF_TC_HEAD(bpfjit_mbuf_ldh_abs, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that BPF_LD+BPF_H+BPF_ABS "
	    "loads halfwords from mbuf correctly");
}

ATF_TC_BODY(bpfjit_mbuf_ldh_abs, tc)
{

	RZ(rump_init());

	ATF_CHECK(test_ldh_abs(0));
	ATF_CHECK(test_ldh_abs(1));
	ATF_CHECK(test_ldh_abs(2));
	ATF_CHECK(test_ldh_abs(3));
	ATF_CHECK(test_ldh_abs(4));
	ATF_CHECK(test_ldh_abs(5));
}

ATF_TC(bpfjit_mbuf_ldw_abs);
ATF_TC_HEAD(bpfjit_mbuf_ldw_abs, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that BPF_LD+BPF_W+BPF_ABS "
	    "loads words from mbuf correctly");
}

ATF_TC_BODY(bpfjit_mbuf_ldw_abs, tc)
{

	RZ(rump_init());

	ATF_CHECK(test_ldw_abs(0));
	ATF_CHECK(test_ldw_abs(1));
	ATF_CHECK(test_ldw_abs(2));
	ATF_CHECK(test_ldw_abs(3));
	ATF_CHECK(test_ldw_abs(4));
	ATF_CHECK(test_ldw_abs(5));
}

ATF_TC(bpfjit_mbuf_ldb_ind);
ATF_TC_HEAD(bpfjit_mbuf_ldb_ind, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that BPF_LD+BPF_B+BPF_IND "
	    "loads bytes from mbuf correctly");
}

ATF_TC_BODY(bpfjit_mbuf_ldb_ind, tc)
{

	RZ(rump_init());

	ATF_CHECK(test_ldb_ind(0));
	ATF_CHECK(test_ldb_ind(1));
	ATF_CHECK(test_ldb_ind(2));
	ATF_CHECK(test_ldb_ind(3));
	ATF_CHECK(test_ldb_ind(4));
	ATF_CHECK(test_ldb_ind(5));
}

ATF_TC(bpfjit_mbuf_ldh_ind);
ATF_TC_HEAD(bpfjit_mbuf_ldh_ind, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that BPF_LD+BPF_H+BPF_IND "
	    "loads halfwords from mbuf correctly");
}

ATF_TC_BODY(bpfjit_mbuf_ldh_ind, tc)
{

	RZ(rump_init());

	ATF_CHECK(test_ldh_ind(0));
	ATF_CHECK(test_ldh_ind(1));
	ATF_CHECK(test_ldh_ind(2));
	ATF_CHECK(test_ldh_ind(3));
	ATF_CHECK(test_ldh_ind(4));
	ATF_CHECK(test_ldh_ind(5));
}

ATF_TC(bpfjit_mbuf_ldw_ind);
ATF_TC_HEAD(bpfjit_mbuf_ldw_ind, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that BPF_LD+BPF_W+BPF_IND "
	    "loads words from mbuf correctly");
}

ATF_TC_BODY(bpfjit_mbuf_ldw_ind, tc)
{

	RZ(rump_init());

	ATF_CHECK(test_ldw_ind(0));
	ATF_CHECK(test_ldw_ind(1));
	ATF_CHECK(test_ldw_ind(2));
	ATF_CHECK(test_ldw_ind(3));
	ATF_CHECK(test_ldw_ind(4));
	ATF_CHECK(test_ldw_ind(5));
}

ATF_TC(bpfjit_mbuf_msh);
ATF_TC_HEAD(bpfjit_mbuf_msh, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that BPF_LDX+BPF_B+BPF_MSH "
	    "loads bytes from mbuf correctly");
}

ATF_TC_BODY(bpfjit_mbuf_msh, tc)
{

	RZ(rump_init());

	ATF_CHECK(test_msh(0));
	ATF_CHECK(test_msh(1));
	ATF_CHECK(test_msh(2));
	ATF_CHECK(test_msh(3));
	ATF_CHECK(test_msh(4));
	ATF_CHECK(test_msh(5));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, bpfjit_mbuf_ldb_abs);
	ATF_TP_ADD_TC(tp, bpfjit_mbuf_ldh_abs);
	ATF_TP_ADD_TC(tp, bpfjit_mbuf_ldw_abs);
	ATF_TP_ADD_TC(tp, bpfjit_mbuf_ldb_ind);
	ATF_TP_ADD_TC(tp, bpfjit_mbuf_ldh_ind);
	ATF_TP_ADD_TC(tp, bpfjit_mbuf_ldw_ind);
	ATF_TP_ADD_TC(tp, bpfjit_mbuf_msh);

	return atf_no_error();
}
