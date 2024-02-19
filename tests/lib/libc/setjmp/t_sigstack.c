/*	$NetBSD: t_sigstack.c,v 1.2 2024/02/19 04:33:21 riastradh Exp $	*/

/*-
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_sigstack.c,v 1.2 2024/02/19 04:33:21 riastradh Exp $");

#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <ucontext.h>

#include "h_macros.h"

struct sigaltstack ss;
jmp_buf jmp;
unsigned nentries;

static void
on_sigusr1(int signo, siginfo_t *si, void *ctx)
{
	ucontext_t *uc = ctx;
	void *sp = (void *)(uintptr_t)_UC_MACHINE_SP(uc);
	void *fp = __builtin_frame_address(0);

	/*
	 * Ensure that the signal handler was called in the alternate
	 * signal stack.
	 */
	ATF_REQUIRE_MSG(fp >= ss.ss_sp,
	    "sigaltstack failed to take effect --"
	    " signal handler's frame pointer %p doesn't lie in sigaltstack"
	    " [%p, %p), size 0x%zx",
	    fp, ss.ss_sp, (char *)ss.ss_sp + ss.ss_size, ss.ss_size);
	ATF_REQUIRE_MSG(fp < (void *)((char *)ss.ss_sp + ss.ss_size),
	    "sigaltstack failed to take effect --"
	    " signal handler's frame pointer %p doesn't lie in sigaltstack"
	    " [%p, %p), size 0x%zx",
	    fp, ss.ss_sp, (char *)ss.ss_sp + ss.ss_size, ss.ss_size);

	/*
	 * Ensure that if we enter the signal handler, we are entering
	 * it from the original stack, not from the alternate signal
	 * stack.
	 *
	 * On some architectures, this is broken.  Those that appear to
	 * get this right are:
	 *
	 *	alpha, m68k, or1k, powerpc, powerpc64, riscv, vax
	 */
#if defined __arm__ || defined __hppa__ || defined __i386__ || \
    defined __ia64__ || defined __mips__ || defined __sh3__ || \
    defined __sparc__ || defined __sparc64__ || defined __x86_64__
	if (nentries > 0)
		atf_tc_expect_fail("PR lib/57946");
#endif
	ATF_REQUIRE_MSG((sp < ss.ss_sp ||
		sp > (void *)((char *)ss.ss_sp + ss.ss_size)),
	    "longjmp failed to restore stack before allowing signal --"
	    " interrupted stack pointer %p lies in sigaltstack"
	    " [%p, %p), size 0x%zx",
	    sp, ss.ss_sp, (char *)ss.ss_sp + ss.ss_size, ss.ss_size);

	/*
	 * First time through, we want to test whether longjmp restores
	 * the signal mask first, or restores the stack pointer first.
	 * The signal should be blocked at this point, so we re-raise
	 * the signal to queue it up for delivery as soon as it is
	 * unmasked -- which should wait until the stack pointer has
	 * been restored in longjmp.
	 */
	if (nentries++ == 0)
		RL(raise(SIGUSR1));

	/*
	 * Jump back to the original context.
	 */
	longjmp(jmp, 1);
}

ATF_TC(setjmp);
ATF_TC_HEAD(setjmp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test longjmp restores stack first, then signal mask");
}
ATF_TC_BODY(setjmp, tc)
{
	struct sigaction sa;

	/*
	 * Allocate a stack for the signal handler to run in, and
	 * configure the system to use it.
	 *
	 * XXX Should maybe use a guard page but this is simpler.
	 */
	ss.ss_size = SIGSTKSZ;
	REQUIRE_LIBC(ss.ss_sp = malloc(ss.ss_size), NULL);
	RL(sigaltstack(&ss, NULL));

	/*
	 * Set up a test signal handler for SIGUSR1.  Allow all
	 * signals, except SIGUSR1 (which is masked by default) -- that
	 * way we don't inadvertently obscure weird crashes in the
	 * signal handler.
	 *
	 * Set SA_SIGINFO so the system will pass siginfo -- and, more
	 * to the point, ucontext, so the signal handler can determine
	 * the stack pointer of the logic it interrupted.
	 *
	 * Set SA_ONSTACK so the system will use the alternate signal
	 * stack to call the signal handler -- that way, it can tell
	 * whether the stack was restored before the second time
	 * around.
	 */
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = &on_sigusr1;
	RL(sigemptyset(&sa.sa_mask));
	sa.sa_flags = SA_SIGINFO|SA_ONSTACK;
	RL(sigaction(SIGUSR1, &sa, NULL));

	/*
	 * Set up a return point for the signal handler: when the
	 * signal handler does longjmp(jmp, 1), it comes flying out of
	 * here.
	 */
	if (setjmp(jmp) == 1)
		return;

	/*
	 * Raise the signal to enter the signal handler the first time.
	 */
	RL(raise(SIGUSR1));

	/*
	 * If we ever reach this point, something went seriously wrong.
	 */
	atf_tc_fail("unreachable");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, setjmp);
//	ATF_TP_ADD_TC(tp, sigsetjmp);

	return atf_no_error();
}
