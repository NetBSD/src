/*	$NetBSD: t_aes.c,v 1.4 2020/08/17 16:26:02 riastradh Exp $	*/

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

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_bear.h>
#include <crypto/aes/aes_impl.h>

#if defined(__i386__) || defined(__x86_64__)
#include <crypto/aes/arch/x86/aes_ni.h>
#include <crypto/aes/arch/x86/aes_sse2.h>
#include <crypto/aes/arch/x86/aes_ssse3.h>
#include <crypto/aes/arch/x86/aes_via.h>
#endif

#ifdef __aarch64__
#include <crypto/aes/arch/arm/aes_armv8.h>
#endif

#if __ARM_ARCH >= 7
#include <crypto/aes/arch/arm/aes_neon.h>
#endif

#include <atf-c.h>

ATF_TC(aes_ct_selftest);
ATF_TC_HEAD(aes_ct_selftest, tc)
{

	atf_tc_set_md_var(tc, "descr", "BearSSL aes_ct tests");
}

ATF_TC_BODY(aes_ct_selftest, tc)
{

	if (aes_bear_impl.ai_probe()) {
		/*
		 * aes_ct is the portable software fallback, so probe
		 * should never fail.
		 */
		atf_tc_fail("BearSSL aes_ct probe failed");
	}

	if (aes_selftest(&aes_bear_impl))
		atf_tc_fail("BearSSL aes_ct self-test failed");
}

#define	AES_SELFTEST(name, impl, descr)					      \
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
	if ((impl)->ai_probe())						      \
		atf_tc_skip("%s not supported on this hardware",	      \
		    (impl)->ai_name);					      \
	if (aes_selftest(impl))						      \
		atf_tc_fail("%s self-test failed", (impl)->ai_name);	      \
}

#ifdef __aarch64__
AES_SELFTEST(aes_armv8_selftest, &aes_armv8_impl, "ARMv8.0-AES self-test")
#endif

#if __ARM_ARCH >= 7
AES_SELFTEST(aes_neon_selftest, &aes_neon_impl, "ARM NEON vpaes self-test")
#endif

#ifdef __x86_64__
AES_SELFTEST(aes_ni_selftest, &aes_ni_impl, "Intel AES-NI self-test")
#endif

#if defined(__i386__) || defined(__x86_64__)
AES_SELFTEST(aes_sse2_selftest, &aes_sse2_impl,
    "Intel SSE2 bitsliced self-test")
AES_SELFTEST(aes_ssse3_selftest, &aes_ssse3_impl,
    "Intel SSSE3 vpaes self-test")
AES_SELFTEST(aes_via_selftest, &aes_via_impl, "VIA ACE AES self-test")
#endif

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, aes_ct_selftest);

#ifdef __aarch64__
	ATF_TP_ADD_TC(tp, aes_armv8_selftest);
#endif

#if __ARM_ARCH >= 7
	ATF_TP_ADD_TC(tp, aes_neon_selftest);
#endif

#ifdef __x86_64__
	ATF_TP_ADD_TC(tp, aes_ni_selftest);
#endif

#if defined(__i386__) || defined(__x86_64__)
	ATF_TP_ADD_TC(tp, aes_sse2_selftest);
	ATF_TP_ADD_TC(tp, aes_ssse3_selftest);
	ATF_TP_ADD_TC(tp, aes_via_selftest);
#endif

	return atf_no_error();
}
