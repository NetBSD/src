/*	$NetBSD: t_tls_alignment.c,v 1.1.2.2 2025/12/18 18:14:22 martin Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <atf-c.h>

#define ALIGNMENT	64

#define MAGIC1	0xaa55aa55aa55aa55
#define MAGIC2	0xc0ffeeeeeeeeeeee
#define MAGIC3	0xff00ff00ff00ff00

__thread
struct {
	uint64_t magic1;
	uint64_t magic2 __attribute__((aligned(ALIGNMENT)));
	uint64_t magic3;
} tls_data = {
	.magic1 = MAGIC1,
	.magic2 = MAGIC2,
	.magic3 = MAGIC3,
};

ATF_TC(tls_alignment);
ATF_TC_HEAD(tls_alignment, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "TLS alignment requirements met");
}

ATF_TC_BODY(tls_alignment, tc)
{
#ifdef __HAVE_TLS_VARIANT_I
	atf_tc_expect_fail("PR toolchain/59652");
#endif
	
	ATF_CHECK(tls_data.magic1 == MAGIC1);
	ATF_CHECK(tls_data.magic2 == MAGIC2);
	ATF_CHECK(tls_data.magic3 == MAGIC3);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, tls_alignment);

	return atf_no_error();
}
