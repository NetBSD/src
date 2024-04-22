/* $NetBSD: t_signal_and_sp.c,v 1.1 2024/04/22 07:24:22 pho Exp $ */

/*
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#include <atf-c.h>

#if defined(HAVE_STACK_POINTER_H)
#  include <signal.h>
#  include <string.h>
#  include <sys/stdint.h>
#  include <sys/time.h>
#  include "stack_pointer.h"

static volatile void* stack_pointer = NULL;
static void on_alarm(int sig __attribute__((__unused__)))
{
	/*
	 * Store the stack pointer into a variable so that we can test if
	 * it's aligned.
	 */
	LOAD_SP(stack_pointer);

	/*
	 * Now we are going to return from a signal
	 * handler. __sigtramp_siginfo_2 will call setcontext(2) with a
	 * ucontext provided by the kernel. When that fails it will call
	 * _Exit(2) with the errno, and the test will fail.
	 */
}
#endif

ATF_TC(misaligned_sp_and_signal);
ATF_TC_HEAD(misaligned_sp_and_signal, tc)
{
	atf_tc_set_md_var(tc, "descr", "process can return from a signal"
	    " handler even if the stack pointer is misaligned when a signal"
	    " arrives");
}
ATF_TC_BODY(misaligned_sp_and_signal, tc)
{
#if defined(HAVE_STACK_POINTER_H)
	/*
	 * Set up a handler for SIGALRM.
	 */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &on_alarm;
	ATF_REQUIRE(sigaction(SIGALRM, &sa, NULL) == 0);

	/*
	 * Set up an interval timer so that we receive SIGALRM after 50 ms.
	 */
	struct itimerval itv;
	memset(&itv, 0, sizeof(itv));
	itv.it_value.tv_usec = 1000 * 50;
	ATF_REQUIRE(setitimer(ITIMER_MONOTONIC, &itv, NULL) == 0);

	/*
	 * Now misalign the SP. Wait for the signal to arrive and see what
	 * happens. This should be fine as long as we don't use it to
	 * access memory.
	 */
	MISALIGN_SP;
	while (stack_pointer == NULL) {
		/*
		 * Make sure the compiler does not optimize this busy loop
		 * away.
		 */
		__asm__("" : : : "memory");
	}
	/*
	 * We could successfully return from a signal handler. Now we
	 * should fix the SP before calling any functions.
	 */
	FIX_SP;

	/*
	 * But was the stack pointer aligned when we were on the signal
	 * handler?
	 */
	ATF_CHECK_MSG(is_sp_aligned((uintptr_t)stack_pointer),
	    "signal handler was called with a misaligned sp: %p",
	    stack_pointer);
#else
	atf_tc_skip("Not implemented for this platform");
#endif
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, misaligned_sp_and_signal);
	return atf_no_error();
}
