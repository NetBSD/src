/*	$NetBSD: t_chacha.c,v 1.4 2020/08/17 16:26:02 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#include <sys/types.h>

#include <crypto/chacha/chacha.h>
#include <crypto/chacha/chacha_ref.h>

#if defined(__i386__) || defined(__x86_64__)
#include <crypto/chacha/arch/x86/chacha_sse2.h>
#endif

#if __ARM_ARCH >= 7
#include <crypto/chacha/arch/arm/chacha_neon.h>
#endif

#include <atf-c.h>

ATF_TC(chacha_ref_selftest);
ATF_TC_HEAD(chacha_ref_selftest, tc)
{

	atf_tc_set_md_var(tc, "descr", "Portable C chacha_ref tests");
}

ATF_TC_BODY(chacha_ref_selftest, tc)
{

	if (chacha_ref_impl.ci_probe()) {
		/*
		 * chacha_ref is the portable software fallback, so
		 * probe should never fail.
		 */
		atf_tc_fail("Portable C chacha_ref probe failed");
	}

	if (chacha_selftest(&chacha_ref_impl))
		atf_tc_fail("Portable C chacha_ref self-test failed");
}

#define	CHACHA_SELFTEST(name, impl, descr)				      \
ATF_TC(name);								      \
ATF_TC_HEAD(name, tc)							      \
{									      \
									      \
	atf_tc_set_md_var(tc, "descr", descr);				      \
}									      \
									      \
ATF_TC_BODY(name, tc)							      \
{									      \
									      \
	if ((impl)->ci_probe())						      \
		atf_tc_skip("%s not supported on this hardware",	      \
		    (impl)->ci_name);					      \
	if (chacha_selftest(impl))					      \
		atf_tc_fail("%s self-test failed", (impl)->ci_name);	      \
}

#if __ARM_ARCH >= 7
CHACHA_SELFTEST(chacha_neon_selftest, &chacha_neon_impl,
    "ARM NEON ChaCha self-test")
#endif

#if defined(__i386__) || defined(__x86_64__)
CHACHA_SELFTEST(chacha_sse2_selftest, &chacha_sse2_impl,
    "x86 SSE2 ChaCha self-test")
#endif

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, chacha_ref_selftest);

#if __ARM_ARCH >= 7
	ATF_TP_ADD_TC(tp, chacha_neon_selftest);
#endif

#if defined(__i386__) || defined(__x86_64__)
	ATF_TP_ADD_TC(tp, chacha_sse2_selftest);
#endif

	return atf_no_error();
}
