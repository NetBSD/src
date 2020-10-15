/*	$NetBSD: t_convert_xmm_s87.c,v 1.1 2020/10/15 17:44:44 mgorny Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michał Górny for Moritz Systems Technology Company Sp. z o.o.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_convert_xmm_s87.c,v 1.1 2020/10/15 17:44:44 mgorny Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <x86/cpu.h>
#include <x86/cpu_extended_state.h>

#include <stdint.h>
#include <string.h>

#include <atf-c.h>

#include "arch/x86/x86/convert_xmm_s87.c"

static const struct fpaccfx ST_INPUTS[] = {
	{{0x8000000000000000, 0x4000}}, /* +2.0 */
	{{0x3f00000000000000, 0x0000}}, /* 1.654785e-4932 */
	{{0x0000000000000000, 0x0000}}, /* +0 */
	{{0x0000000000000000, 0x8000}}, /* -0 */
	{{0x8000000000000000, 0x7fff}}, /* +inf */
	{{0x8000000000000000, 0xffff}}, /* -inf */
	{{0xc000000000000000, 0xffff}}, /* nan */
	{{0x8000000000000000, 0xc000}}, /* -2.0 */
};
__CTASSERT(sizeof(*ST_INPUTS) == 16);

ATF_TC(fsave_fxsave_hw);
ATF_TC_HEAD(fsave_fxsave_hw, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "converting between FSAVE/FXSAVE comparing to actual results");
}

ATF_TC_BODY(fsave_fxsave_hw, tc)
{
	struct save87 fsave, fsave_conv;
	struct fxsave fxsave, fxsave_conv;
	int i, j;

	int mib[] = { CTL_MACHDEP, CPU_OSFXSR };
	int has_fxsave;
	size_t has_fxsave_size = sizeof(has_fxsave);
	if (sysctl(mib, __arraycount(mib), &has_fxsave, &has_fxsave_size,
	    NULL, 0) == -1 || !has_fxsave)
		atf_tc_skip("FXSAVE not supported");

	for (i = 1; i <= __arraycount(ST_INPUTS); i++) {
		unsigned long unused1, unused2;
		__asm__ __volatile__(
			"finit\n"
			".loadfp:\n\t"
			"fldt (%2)\n\t"
			"add $0x10, %2\n\t"
			"loop .loadfp\n\t"
			"fxsave %5\n\t"
			"fsave %4\n\t"
			: "=b"(unused1), "=c"(unused2)
			: "b"(ST_INPUTS), "c"(i), "m"(fsave), "m"(fxsave)
			: "st"
		);

		/* Self-assertion for working FSAVE/FXSAVE */
		ATF_REQUIRE_EQ(fsave.s87_cw, fxsave.fx_cw);
		ATF_REQUIRE_EQ(fsave.s87_sw, fxsave.fx_sw);

		/* Test process_xmm_to_s87() */
		process_xmm_to_s87(&fxsave, &fsave_conv);
		ATF_CHECK_EQ(fsave_conv.s87_cw, fsave.s87_cw);
		ATF_CHECK_EQ(fsave_conv.s87_sw, fsave.s87_sw);
		ATF_CHECK_EQ(fsave_conv.s87_tw, fsave.s87_tw);
		for (j = 0; j < i; j++) {
			ATF_CHECK_EQ(fsave_conv.s87_ac[j].f87_exp_sign,
			    fsave.s87_ac[j].f87_exp_sign);
			ATF_CHECK_EQ(fsave_conv.s87_ac[j].f87_mantissa,
			    fsave.s87_ac[j].f87_mantissa);
		}

		/* Test process_s87_to_xmm() */
		process_s87_to_xmm(&fsave, &fxsave_conv);
		ATF_CHECK_EQ(fxsave_conv.fx_cw, fxsave.fx_cw);
		ATF_CHECK_EQ(fxsave_conv.fx_sw, fxsave.fx_sw);
		ATF_CHECK_EQ(fxsave_conv.fx_tw, fxsave.fx_tw);
		for (j = 0; j < i; j++) {
			ATF_CHECK_EQ(fxsave_conv.fx_87_ac[j].r.f87_exp_sign,
			    fxsave.fx_87_ac[j].r.f87_exp_sign);
			ATF_CHECK_EQ(fxsave_conv.fx_87_ac[j].r.f87_mantissa,
			    fxsave.fx_87_ac[j].r.f87_mantissa);
		}
	}
}

struct s87_xmm_test_vector {
	uint16_t sw;
	uint16_t tw;
	uint8_t tw_abridged;
};

struct s87_xmm_test_vector FIXED_TEST_VECTORS[] = {
	{0x3800, 0x3fff, 0x80},
	{0x3000, 0x2fff, 0xc0},
	{0x2800, 0x27ff, 0xe0},
	{0x2000, 0x25ff, 0xf0},
	{0x1800, 0x25bf, 0xf8},
	{0x1000, 0x25af, 0xfc},
	{0x0800, 0x25ab, 0xfe},
	{0x0000, 0x25a8, 0xff},
};

ATF_TC(s87_to_xmm);
ATF_TC_HEAD(s87_to_xmm, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "converting from FSAVE to FXSAVE using fixed test vectors");
}

ATF_TC_BODY(s87_to_xmm, tc)
{
	struct save87 fsave;
	struct fxsave fxsave;
	int i, j;

	memset(&fsave, 0, sizeof(fsave));
	for (i = 0; i < __arraycount(ST_INPUTS); i++) {
		fsave.s87_sw = FIXED_TEST_VECTORS[i].sw;
		fsave.s87_tw = FIXED_TEST_VECTORS[i].tw;
		for (j = 0; j <= i; j++)
			fsave.s87_ac[i - j] = ST_INPUTS[j].r;

		process_s87_to_xmm(&fsave, &fxsave);
		ATF_CHECK_EQ(fxsave.fx_sw, FIXED_TEST_VECTORS[i].sw);
		ATF_CHECK_EQ(fxsave.fx_tw, FIXED_TEST_VECTORS[i].tw_abridged);
		for (j = 0; j < i; j++) {
			ATF_CHECK_EQ(fxsave.fx_87_ac[i - j].r.f87_exp_sign,
			    ST_INPUTS[j].r.f87_exp_sign);
			ATF_CHECK_EQ(fxsave.fx_87_ac[i - j].r.f87_mantissa,
			    ST_INPUTS[j].r.f87_mantissa);
		}
	}
}

ATF_TC(xmm_to_s87);
ATF_TC_HEAD(xmm_to_s87, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "converting from FSAVE to FXSAVE using fixed test vectors");
}

ATF_TC_BODY(xmm_to_s87, tc)
{
	struct save87 fsave;
	struct fxsave fxsave;
	int i, j;

	memset(&fxsave, 0, sizeof(fxsave));
	for (i = 0; i < __arraycount(ST_INPUTS); i++) {
		fxsave.fx_sw = FIXED_TEST_VECTORS[i].sw;
		fxsave.fx_tw = FIXED_TEST_VECTORS[i].tw_abridged;
		for (j = 0; j <= i; j++)
			fxsave.fx_87_ac[i - j] = ST_INPUTS[j];

		process_xmm_to_s87(&fxsave, &fsave);
		ATF_CHECK_EQ(fsave.s87_sw, FIXED_TEST_VECTORS[i].sw);
		ATF_CHECK_EQ(fsave.s87_tw, FIXED_TEST_VECTORS[i].tw);
		for (j = 0; j < i; j++) {
			ATF_CHECK_EQ(fsave.s87_ac[i - j].f87_exp_sign,
			    ST_INPUTS[j].r.f87_exp_sign);
			ATF_CHECK_EQ(fsave.s87_ac[i - j].f87_mantissa,
			    ST_INPUTS[j].r.f87_mantissa);
		}
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, fsave_fxsave_hw);
	ATF_TP_ADD_TC(tp, s87_to_xmm);
	ATF_TP_ADD_TC(tp, xmm_to_s87);
	return atf_no_error();
}
