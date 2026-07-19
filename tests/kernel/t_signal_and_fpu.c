/*	$NetBSD: t_signal_and_fpu.c,v 1.3.2.2 2026/07/19 15:57:27 martin Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_signal_and_fpu.c,v 1.3.2.2 2026/07/19 15:57:27 martin Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>

#include <atf-c.h>
#include <float.h>
#include <pthread.h>
#include <signal.h>

#include "h_macros.h"

#ifdef HAVE_SIG_FPU_H
#include "sig_fpu.h"
#endif

static volatile bool ready_for_signal;
static volatile bool signal_delivered;

static pthread_t meddler_thread;
static pthread_t tester_thread;

static void (*current_trashfn)(void);
static int (*current_testfn)(volatile bool *, const volatile bool *);

static void
sigusr1_handler(int signo)
{

	(*current_trashfn)();
	signal_delivered = true;
}

static void *
start_meddling(void *cookie)
{

	while (!ready_for_signal)
		membar_consumer();
	RZ(pthread_kill(tester_thread, SIGUSR1));
	return NULL;
}

static void *
start_testing(void *cookie)
{
	struct sigaction sa;
	int error;

	/*
	 * Arrange to have SIGUSR1 trash the FPU state we're testing
	 * and then notify the tester that it's done.
	 */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &sigusr1_handler;
	RL(sigfillset(&sa.sa_mask));
	sa.sa_flags = 0;
	RL(sigaction(SIGUSR1, &sa, NULL));

	/*
	 * Run the test.  It will set ready_for_signal = true when it's
	 * ready for the meddling thread to send a signal, and the
	 * signal handler will set signal_delivered = true so the
	 * tester will know when to stop.
	 */
	error = (*current_testfn)(&ready_for_signal, &signal_delivered);

	return (void *)(intptr_t)error;
}

static void
test_signal_fpu(bool (*supportfn)(void),
    int (*testfn)(volatile bool *, const volatile bool *),
    void (*trashfn)(void),
    const char *xfail)
{
	unsigned i;
	void *test_result;

	if (supportfn && !(*supportfn)())
		atf_tc_skip("not supported on this machine");
	if (xfail)
		atf_tc_expect_fail("%s", xfail);

	/*
	 * Prepare global state.
	 */
	current_testfn = testfn;
	current_trashfn = trashfn;

	/*
	 * Do ten trials of each test, since they're often randomized,
	 * and each one should be quick.
	 */
	for (i = 0; i < 10; i++) {
		/*
		 * Reset the state.
		 */
		ready_for_signal = false;
		signal_delivered = false;

		/*
		 * Create tester and meddler threads.  As soon as the tester
		 * thread sets ready_for_signal, the meddler thread will send
		 * it a signal.
		 */
		RZ(pthread_create(&tester_thread, NULL, &start_testing, NULL));
		RZ(pthread_create(&meddler_thread, NULL, &start_meddling,
			NULL));

		/*
		 * Verify both threads complete within 1sec, and verify the
		 * tester returned zero error.  The error number can be used
		 * for machine-dependent diagnostics.
		 */
		REQUIRE_LIBC(alarm(1), (unsigned)-1);
		RZ(pthread_join(meddler_thread, NULL));
		RZ(pthread_join(tester_thread, &test_result));
		ATF_REQUIRE_MSG((int)(intptr_t)test_result == 0,
		    "test_result=0x%x", (int)(intptr_t)test_result);
	}
}

static int
test_double(volatile bool *ready, const volatile bool *done)
{
	long long i;
	volatile double one = 1;
	double f0, f;
	int error = 0;

	i = 1;
	f0 = one;
	*ready = true;
	f = f0;
	while (!*done) {
		for (i = 1, f = f0;
		     !*done && i < MIN(1LL << DBL_MANT_DIG, INT_MAX);
		     i++, f++)
			continue;
	}
	if (f != (double)i)
		error = i;
	return error;
}

static void
trash_double(void)
{
	volatile double f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12,
	    f13, f14, f15, f16;

	f0 = (double)arc4random();
	f1 = f0 + (double)arc4random();
	f2 = f0 + f1 + (double)arc4random();
	f3 = f0 + f1 + f2 + (double)arc4random();
	f4 = f0 + f1 + f2 + f3 + (double)arc4random();
	f5 = f0 + f1 + f2 + f3 + f4 + (double)arc4random();
	f6 = f0 + f1 + f2 + f3 + f4 + f5 + (double)arc4random();
	f7 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + (double)arc4random();
	f8 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + (double)arc4random();
	f9 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + (double)arc4random();
	f10 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 +
	    (double)arc4random();
	f11 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 +
	    (double)arc4random();
	f12 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    (double)arc4random();
	f13 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + (double)arc4random();
	f14 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + (double)arc4random();
	f15 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + f14 + (double)arc4random();
	f16 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + f14 + f15 + (double)arc4random();
	(void)f16;
}

ATF_TC(double);
ATF_TC_HEAD(double, tc)
{
	atf_tc_set_md_var(tc, "descr", "double");
}
ATF_TC_BODY(double, tc)
{
	test_signal_fpu(NULL, &test_double, &trash_double, NULL);
}

static int
test_float(volatile bool *ready, const volatile bool *done)
{
	int i;
	volatile float one = 1;
	float f0, f;
	int error = 0;

	i = 1;
	f0 = one;
	*ready = true;
	f = f0;
	while (!*done) {
		for (i = 1, f = f0;
		     !*done && i < MIN(1 << FLT_MANT_DIG, INT_MAX);
		     i++, f++)
			continue;
	}
	if (f != (float)i)
		error = i;
	return error;
}

static void
trash_float(void)
{
	volatile float f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12,
	    f13, f14, f15, f16;

	f0 = (float)arc4random();
	f1 = f0 + (float)arc4random();
	f2 = f0 + f1 + (float)arc4random();
	f3 = f0 + f1 + f2 + (float)arc4random();
	f4 = f0 + f1 + f2 + f3 + (float)arc4random();
	f5 = f0 + f1 + f2 + f3 + f4 + (float)arc4random();
	f6 = f0 + f1 + f2 + f3 + f4 + f5 + (float)arc4random();
	f7 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + (float)arc4random();
	f8 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + (float)arc4random();
	f9 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + (float)arc4random();
	f10 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 +
	    (float)arc4random();
	f11 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 +
	    (float)arc4random();
	f12 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    (float)arc4random();
	f13 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + (float)arc4random();
	f14 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + (float)arc4random();
	f15 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + f14 + (float)arc4random();
	f16 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + f14 + f15 + (float)arc4random();
	(void)f16;
}

ATF_TC(float);
ATF_TC_HEAD(float, tc)
{
	atf_tc_set_md_var(tc, "descr", "float");
}
ATF_TC_BODY(float, tc)
{
	test_signal_fpu(NULL, &test_float, &trash_float, NULL);
}

static int
test_ldouble(volatile bool *ready, const volatile bool *done)
{
	long long i;
	volatile long double one = 1;
	long double f0, f;
	int error = 0;

	i = 1;
	f0 = one;
	*ready = true;
	f = f0;
	while (!*done) {
		/*
		 * LDBL_MANT_DIG is too big, but we won't reach past
		 * 2^DBL_MANT_DIG anyway, so just use DBL_MANT_DIG.
		 */
		for (i = 1, f = f0;
		     !*done && i < MIN(1LL << DBL_MANT_DIG, LONG_MAX);
		     i++, f++)
			continue;
	}
	if (f != (long double)i)
		error = i;
	return error;
}

static void
trash_ldouble(void)
{
	volatile long double f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11,
	    f12, f13, f14, f15, f16;

	f0 = (long double)arc4random();
	f1 = f0 + (long double)arc4random();
	f2 = f0 + f1 + (long double)arc4random();
	f3 = f0 + f1 + f2 + (long double)arc4random();
	f4 = f0 + f1 + f2 + f3 + (long double)arc4random();
	f5 = f0 + f1 + f2 + f3 + f4 + (long double)arc4random();
	f6 = f0 + f1 + f2 + f3 + f4 + f5 + (long double)arc4random();
	f7 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + (long double)arc4random();
	f8 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + (long double)arc4random();
	f9 = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 +
	    (long double)arc4random();
	f10 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 +
	    (long double)arc4random();
	f11 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 +
	    (long double)arc4random();
	f12 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    (long double)arc4random();
	f13 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + (long double)arc4random();
	f14 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + (long double)arc4random();
	f15 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + f14 + (long double)arc4random();
	f16 = f0 + f1 + f2 + f3 + f4 + f4 + f6 + f7 + f8 + f9 + f10 + f11 +
	    f12 + f13 + f14 + f15 + (long double)arc4random();
	(void)f16;
}

ATF_TC(ldouble);
ATF_TC_HEAD(ldouble, tc)
{
	atf_tc_set_md_var(tc, "descr", "long double");
}
ATF_TC_BODY(ldouble, tc)
{
	test_signal_fpu(NULL, &test_ldouble, &trash_ldouble, NULL);
}

#if defined __i386__ || defined __x86_64__

ATF_TC(x87);
ATF_TC_HEAD(x87, tc)
{
	atf_tc_set_md_var(tc, "descr", "x87");
}
ATF_TC_BODY(x87, tc)
{
	test_signal_fpu(&x87_supported, &test_x87, &trash_x87, NULL);
}

ATF_TC(xmm);
ATF_TC_HEAD(xmm, tc)
{
	atf_tc_set_md_var(tc, "descr", "xmm");
}
ATF_TC_BODY(xmm, tc)
{
	test_signal_fpu(&xmm_supported, &test_xmm, &trash_xmm, NULL);
}

ATF_TC(ymm);
ATF_TC_HEAD(ymm, tc)
{
	atf_tc_set_md_var(tc, "descr", "ymm");
}
ATF_TC_BODY(ymm, tc)
{
	test_signal_fpu(&ymm_supported, &test_ymm, &trash_ymm, NULL);
}

#endif

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, double);
	ATF_TP_ADD_TC(tp, float);
	ATF_TP_ADD_TC(tp, ldouble);
#if defined __i386__ || defined __x86_64__
	ATF_TP_ADD_TC(tp, x87);
	ATF_TP_ADD_TC(tp, xmm);
	ATF_TP_ADD_TC(tp, ymm);
#endif
	return atf_no_error();
}
