/*	$NetBSD: t_sigstack.c,v 1.12.2.2 2024/08/23 16:56:29 martin Exp $	*/

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
__RCSID("$NetBSD: t_sigstack.c,v 1.12.2.2 2024/08/23 16:56:29 martin Exp $");

#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <ucontext.h>

#include "h_macros.h"

struct sigaltstack ss[3];
jmp_buf jmp;
sigjmp_buf sigjmp;
unsigned nentries;
const char *bailname;
void (*bailfn)(void) __dead;

static void
on_sigusr1(int signo, siginfo_t *si, void *ctx)
{
	ucontext_t *uc = ctx;
	void *sp = (void *)(uintptr_t)_UC_MACHINE_SP(uc);
	void *fp = __builtin_frame_address(0);
	struct sigaltstack *ssp;

	/*
	 * Ensure we haven't re-entered the signal handler too many
	 * times.  We should enter only twice.
	 */
	ATF_REQUIRE_MSG(nentries < 2,
	    "%u recursive signal handler entries is too many in this test",
	    nentries + 1);

	/*
	 * Ensure that the signal handler was called in the alternate
	 * signal stack.
	 */
	ssp = &ss[nentries];
	ATF_REQUIRE_MSG((fp >= ssp->ss_sp &&
		fp < (void *)((char *)ssp->ss_sp + ssp->ss_size)),
	    "sigaltstack failed to take effect on entry %u --"
	    " signal handler's frame pointer %p doesn't lie in sigaltstack"
	    " [%p, %p), size 0x%zx",
	    nentries,
	    fp, ssp->ss_sp, (char *)ssp->ss_sp + ssp->ss_size, ssp->ss_size);

	/*
	 * Ensure that if we enter the signal handler, we are entering
	 * it from the original stack, not from any of the alternate
	 * signal stacks.
	 *
	 * On some architectures, this is broken.  Those that appear to
	 * get this right are:
	 *
	 *	aarch64
	 *	alpha
	 *	arm
	 *	i386
	 *	m68k
	 *	or1k
	 *	powerpc
	 *	powerpc64
	 *	riscv
	 *	vax
	 *	x86_64
	 */
#if defined __hppa__ || \
    defined __ia64__ || defined __mips__ || \
    defined __sparc__ || defined __sparc64__
	if (nentries > 0)
		atf_tc_expect_fail("PR lib/57946");
#endif
	for (ssp = &ss[0]; ssp < &ss[__arraycount(ss)]; ssp++) {
		ATF_REQUIRE_MSG((sp < ssp->ss_sp ||
			sp >= (void *)((char *)ssp->ss_sp + ssp->ss_size)),
		    "%s failed to restore stack"
		    " before allowing signal on entry %u --"
		    " interrupted stack pointer %p lies in sigaltstack %zd"
		    " [%p, %p), size 0x%zx",
		    bailname,
		    nentries,
		    sp, ssp - ss,
		    ssp->ss_sp, (char *)ssp->ss_sp + ssp->ss_size,
		    ssp->ss_size);
	}

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
	 * Set up the next sigaltstack.  We can't reuse the current one
	 * for the next signal handler re-entry until the system clears
	 * the SS_ONSTACK process state -- which normal return from
	 * signal handler does, but which longjmp does not.  So to keep
	 * it simple (ha), we just use another sigaltstack.
	 */
	RL(sigaltstack(&ss[nentries], NULL));

	/*
	 * Jump back to the original context.
	 */
	(*bailfn)();
}

static void
go(const char *name, void (*fn)(void) __dead)
{
	struct sigaction sa;
	unsigned i;

	bailname = name;
	bailfn = fn;

	/*
	 * Allocate a stack for the signal handler to run in, and
	 * configure the system to use the first one.
	 *
	 * XXX Should maybe use a guard page but this is simpler.
	 */
	for (i = 0; i < __arraycount(ss); i++) {
		ss[i].ss_size = SIGSTKSZ;
		REQUIRE_LIBC(ss[i].ss_sp = malloc(ss[i].ss_size), NULL);
	}
	RL(sigaltstack(&ss[0], NULL));

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
	 * Raise the signal to enter the signal handler the first time.
	 */
	RL(raise(SIGUSR1));

	/*
	 * If we ever reach this point, something went seriously wrong.
	 */
	atf_tc_fail("unreachable");
}

static void __dead
bail_longjmp(void)
{

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

	/*
	 * Set up a return point for the signal handler: when the
	 * signal handler does longjmp(jmp, 1), it comes flying out of
	 * here.
	 */
	if (setjmp(jmp) == 1)
		return;

	/*
	 * Run the test with longjmp.
	 */
	go("longjmp", &bail_longjmp);
}

static void __dead
bail_siglongjmp(void)
{

	siglongjmp(sigjmp, 1);
}

ATF_TC(sigsetjmp);
ATF_TC_HEAD(sigsetjmp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test siglongjmp restores stack first, then signal mask");
}
ATF_TC_BODY(sigsetjmp, tc)
{

	/*
	 * Set up a return point for the signal handler: when the
	 * signal handler does siglongjmp(sigjmp, 1), it comes flying
	 * out of here.
	 */
	if (sigsetjmp(sigjmp, /*savesigmask*/1) == 1)
		return;

	/*
	 * Run the test with siglongjmp.
	 */
	go("siglongjmp", &bail_siglongjmp);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, setjmp);
	ATF_TP_ADD_TC(tp, sigsetjmp);

	return atf_no_error();
}
