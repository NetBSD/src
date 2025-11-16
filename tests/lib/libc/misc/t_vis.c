/*	$NetBSD: t_vis.c,v 1.2 2025/11/16 12:43:25 nia Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
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

#include <atf-c.h>
#include <string.h>
#include <stdio.h>
#include "vis_types.h"
#include "vis_proto.h"

ATF_TC(vis_test_addsub);

ATF_TC_HEAD(vis_test_addsub, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test 32-bit packed add/subtract");
}

ATF_TC_BODY(vis_test_addsub, tc)
{
	vis_d64 v1, v2, v3;
	vis_f32 f1, f2;
	vis_u32 u1, u2;

	v1 = vis_to_double(8, 16);
	v2 = vis_to_double(16, 8);

	v3 = vis_fpadd32(v1, v2);

	f1 = vis_read_lo(v3);
	memcpy(&u1, &f1, sizeof(f1));
	f2 = vis_read_hi(v3);
	memcpy(&u2, &f2, sizeof(f2));

	ATF_REQUIRE(u1 == 24 && u2 == 24);

	v2 = vis_to_double(4, 4);
	v3 = vis_fpsub32(v3, v2);

	f1 = vis_read_lo(v3);
	memcpy(&u1, &f1, sizeof(f1));
	f2 = vis_read_hi(v3);
	memcpy(&u2, &f2, sizeof(f2));

	ATF_REQUIRE(u1 == 20 && u2 == 20);
}

ATF_TC(vis_test_bitwise);

ATF_TC_HEAD(vis_test_bitwise, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test 32-bit packed bitwise");
}

ATF_TC_BODY(vis_test_bitwise, tc)
{
	static vis_u8 testbytes1[8] = { 1, 0, 1, 1, 1, 0, 1, 1 };
	static vis_u8 testbytes2[8] = { 1, 1, 0, 1, 1, 1, 0, 1 };
	static vis_u8 test_and[8] = { 1, 0, 0, 1, 1, 0, 0, 1 };
	static vis_u8 test_or[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
	static vis_u8 test_zero[8] = { 0 };
	static vis_u64 test_ones = 0xffffffffffffffff;
	double v1, v2, v3;

	memcpy(&v1, testbytes1, sizeof(v1));
	memcpy(&v2, testbytes2, sizeof(v2));

	v3 = vis_fand(v1, v2);

	ATF_REQUIRE(memcmp(&v3, test_and, sizeof(v3)) == 0);

	v3 = vis_fone();

	ATF_REQUIRE(memcmp(&v3, &test_ones, sizeof(v3)) == 0);

	v3 = vis_for(v1, v2);

	ATF_REQUIRE(memcmp(&v3, test_or, sizeof(v3)) == 0);

	v3 = vis_fzero();

	ATF_REQUIRE(memcmp(&v3, test_zero, sizeof(v3)) == 0);
}

ATF_TC(vis_test_fcmpeq16);

ATF_TC_HEAD(vis_test_fcmpeq16, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test 16-bit packed compare");
}

ATF_TC_BODY(vis_test_fcmpeq16, tc)
{
	static vis_u16 testshort1[4] = { 16000, 16000, 16000, 16000 };
	static vis_u16 testshort2[4] = { 32000, 16000, 32000, 16000 };
	static vis_u16 testshort3[4] = { 48000, 48000, 48000, 48000 };
	vis_d64 v1, v2, v3;

	memcpy(&v1, testshort1, sizeof(v1));
	memcpy(&v2, testshort2, sizeof(v2));
	memcpy(&v3, testshort3, sizeof(v3));

	ATF_REQUIRE((!!vis_fcmpeq16(v1, v2)) != 0);
	ATF_REQUIRE((!!vis_fcmpeq16(v1, v3)) == 0);
	ATF_REQUIRE((!!vis_fcmpne16(v1, v3)) != 0);
}

ATF_TC(vis_test_fcmpeq32);

ATF_TC_HEAD(vis_test_fcmpeq32, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test 32-bit packed compare");
}

ATF_TC_BODY(vis_test_fcmpeq32, tc)
{
	static vis_u32 testlong1[2] = { 16000, 16000 };
	static vis_u32 testlong2[2] = { 32000, 16000 };
	static vis_u32 testlong3[2] = { 48000, 48000 };
	vis_d64 v1, v2, v3;

	memcpy(&v1, testlong1, sizeof(v1));
	memcpy(&v2, testlong2, sizeof(v2));
	memcpy(&v3, testlong3, sizeof(v3));

	ATF_REQUIRE((!!vis_fcmpeq32(v1, v2)) != 0);
	ATF_REQUIRE((!!vis_fcmpeq32(v1, v3)) == 0);
	ATF_REQUIRE((!!vis_fcmpne32(v1, v3)) != 0);
}

ATF_TP_ADD_TCS(tp) 
{
	ATF_TP_ADD_TC(tp, vis_test_addsub);
	ATF_TP_ADD_TC(tp, vis_test_bitwise);
	ATF_TP_ADD_TC(tp, vis_test_fcmpeq16);
	ATF_TP_ADD_TC(tp, vis_test_fcmpeq32);

	return atf_no_error();
}
