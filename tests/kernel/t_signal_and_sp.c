/*	$NetBSD: t_signal_and_sp.c,v 1.9 2025/04/21 03:48:07 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_signal_and_sp.c,v 1.9 2025/04/21 03:48:07 riastradh Exp $");

#include <sys/wait.h>

#include <machine/param.h>

#include <atf-c.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#include "h_execsp.h"
#include "h_macros.h"

#ifdef HAVE_STACK_POINTER_H
#  include "stack_pointer.h"
#endif

#ifdef HAVE_SIGNALSPHANDLER
void signalsphandler(int);	/* signalsphandler.S assembly routine */
#endif

void *volatile signalsp;

static void
test_execsp(const struct atf_tc *tc, const char *prog)
{
#ifdef STACK_ALIGNBYTES
	char h_execsp[PATH_MAX];
	struct execsp execsp;
	int fd[2];
	pid_t pid;
	struct pollfd pollfd;
	int nfds;
	ssize_t nread;
	int status;

	/*
	 * Determine the full path to the helper program.
	 */
	RL(snprintf(h_execsp, sizeof(h_execsp), "%s/%s",
		atf_tc_get_config_var(tc, "srcdir"), prog));

	/*
	 * Create a pipe to read a bundle of stack pointer samples from
	 * the child, and fork the child.
	 */
	RL(pipe(fd));
	RL(pid = vfork());
	if (pid == 0) {		/* child */
		char *const argv[] = {h_execsp, NULL};

		if (dup2(fd[1], STDOUT_FILENO) == -1)
			_exit(1);
		if (closefrom(STDERR_FILENO + 1) == -1)
			_exit(2);
		if (execve(argv[0], argv, NULL) == -1)
			_exit(3);
		_exit(4);
	}

	/*
	 * Close the writing end so, if something goes wrong in the
	 * child, we don't hang indefinitely waiting for output.
	 */
	RL(close(fd[1]));

	/*
	 * Wait up to 5sec for the child to return an answer.  Any more
	 * than that, and we kill it.  The child is mostly hand-written
	 * assembly routines where lots can go wrong, so don't bother
	 * waiting if it gets stuck in a loop.
	 */
	pollfd.fd = fd[0];
	pollfd.events = POLLIN;
	RL(nfds = poll(&pollfd, 1, 5*1000/*ms*/));
	if (nfds == 0) {
		fprintf(stderr, "child hung, killing\n");
		RL(kill(pid, SIGKILL));
	}

	/*
	 * Read a bundle of stack pointer samples from the child.
	 */
	RL(nread = read(fd[0], &execsp, sizeof(execsp)));
	ATF_CHECK_MSG((size_t)nread == sizeof(execsp),
	    "nread=%zu sizeof(execsp)=%zu",
	    (size_t)nread, sizeof(execsp));

	/*
	 * Wait for the child to terminate and report failure if it
	 * didn't exit cleanly.
	 */
	RL(waitpid(pid, &status, 0));
	if (WIFSIGNALED(status)) {
		atf_tc_fail_nonfatal("child exited on signal %d (%s)",
		    WTERMSIG(status), strsignal(WTERMSIG(status)));
	} else if (!WIFEXITED(status)) {
		atf_tc_fail_nonfatal("child exited status=0x%x", status);
	} else {
		ATF_CHECK_MSG(WEXITSTATUS(status) == 0,
		    "child exited with code %d",
		    WEXITSTATUS(status));
	}

	/*
	 * Now that we have reaped the child, stop here if the stack
	 * pointer samples are bogus; otherwise verify they are all
	 * aligned.
	 */
	if ((size_t)nread != sizeof(execsp))
		return;		/* failed already */

	printf("start sp @ %p\n", execsp.startsp);
	printf("ctor sp @ %p\n", execsp.ctorsp);
	printf("main sp @ %p\n", execsp.mainsp);
	printf("dtor sp @ %p\n", execsp.dtorsp);

	ATF_CHECK_MSG(((uintptr_t)execsp.startsp & STACK_ALIGNBYTES) == 0,
	    "elf entry point was called with misaligned sp: %p",
	    execsp.startsp);

	ATF_CHECK_MSG(((uintptr_t)execsp.ctorsp & STACK_ALIGNBYTES) == 0,
	    "elf constructor was called with misaligned sp: %p",
	    execsp.ctorsp);

	ATF_CHECK_MSG(((uintptr_t)execsp.mainsp & STACK_ALIGNBYTES) == 0,
	    "main function was called with misaligned sp: %p",
	    execsp.mainsp);

	ATF_CHECK_MSG(((uintptr_t)execsp.dtorsp & STACK_ALIGNBYTES) == 0,
	    "elf destructor was called with misaligned sp: %p",
	    execsp.dtorsp);

	/*
	 * Leave a reminder on architectures for which we haven't
	 * implemented execsp_start.S.
	 */
	if (execsp.startsp == NULL ||
	    execsp.ctorsp == NULL ||
	    execsp.mainsp == NULL ||
	    execsp.dtorsp == NULL)
		atf_tc_skip("Not fully supported on this architecture");
#else
	atf_tc_skip("Unknown STACK_ALIGNBYTES on this architecture");
#endif
}

ATF_TC(execsp_dynamic);
ATF_TC_HEAD(execsp_dynamic, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify stack pointer is aligned on dynamic program start");
}
ATF_TC_BODY(execsp_dynamic, tc)
{
	test_execsp(tc, "h_execsp_dynamic");
}

ATF_TC(execsp_static);
ATF_TC_HEAD(execsp_static, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify stack pointer is aligned on static program start");
}
ATF_TC_BODY(execsp_static, tc)
{
	test_execsp(tc, "h_execsp_static");
}

#if defined STACK_ALIGNBYTES && defined HAVE_CONTEXTSPFUNC
void *volatile contextsp;	/* set by contextspfunc.S */
static ucontext_t return_context;
static volatile bool test_context_done;

void contextspfunc(void);	/* contextspfunc.S assembly routine */

static void
contextnoop(void)
{

	fprintf(stderr, "contextnoop\n");
	/* control will return to contextspfunc via uc_link */
}

void contextdone(void);		/* called by contextspfunc.S */
void
contextdone(void)
{

	fprintf(stderr, "contextdone\n");
	ATF_REQUIRE(!test_context_done);
	test_context_done = true;
	RL(setcontext(&return_context));
	atf_tc_fail("setcontext returned");
}
#endif

ATF_TC(contextsp);
ATF_TC_HEAD(contextsp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify stack pointer is aligned on makecontext entry");
}
ATF_TC_BODY(contextsp, tc)
{
#if defined STACK_ALIGNBYTES && defined HAVE_CONTEXTSPFUNC
	ucontext_t uc;
	char *stack;
	unsigned i;

#ifdef __hppa__
	/*
	 * Not sure what the deal is but I probably wrote contextspfunc
	 * wrong.
	 */
	atf_tc_expect_signal(SIGILL, "PR kern/59327:"
	    " user stack pointer is not aligned properly");
#endif

	REQUIRE_LIBC(stack = malloc(SIGSTKSZ + STACK_ALIGNBYTES), NULL);
	fprintf(stderr, "stack @ [%p,%p)\n", stack,
	    stack + SIGSTKSZ + STACK_ALIGNBYTES);

	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		contextsp = NULL;
		test_context_done = false;

		RL(getcontext(&uc));
		uc.uc_stack.ss_sp = stack;
		uc.uc_stack.ss_size = SIGSTKSZ + i;
		makecontext(&uc, &contextspfunc, 0);

		fprintf(stderr, "[%u] swapcontext\n", i);
		RL(swapcontext(&return_context, &uc));

		ATF_CHECK(contextsp != NULL);
		ATF_CHECK_MSG((uintptr_t)stack <= (uintptr_t)contextsp &&
		    (uintptr_t)contextsp <= (uintptr_t)stack + SIGSTKSZ + i,
		    "contextsp=%p", contextsp);
		ATF_CHECK_MSG(((uintptr_t)contextsp & STACK_ALIGNBYTES) == 0,
		    "[%u] makecontext function called with misaligned sp %p",
		    i, contextsp);
	}

	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		contextsp = NULL;
		test_context_done = false;

		RL(getcontext(&uc));
		uc.uc_stack.ss_sp = stack + i;
		uc.uc_stack.ss_size = SIGSTKSZ;
		makecontext(&uc, &contextspfunc, 0);

		fprintf(stderr, "[%u] swapcontext\n", i);
		RL(swapcontext(&return_context, &uc));

		ATF_CHECK(contextsp != NULL);
		ATF_CHECK_MSG((uintptr_t)stack + i <= (uintptr_t)contextsp &&
		    (uintptr_t)contextsp <= (uintptr_t)stack + i + SIGSTKSZ,
		    "contextsp=%p", contextsp);
		ATF_CHECK_MSG(((uintptr_t)contextsp & STACK_ALIGNBYTES) == 0,
		    "[%u] makecontext function called with misaligned sp %p",
		    i, contextsp);
	}
#else
	atf_tc_skip("Not implemented on this platform");
#endif
}

ATF_TC(contextsplink);
ATF_TC_HEAD(contextsplink, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify stack pointer is aligned on makecontext link entry");
}
ATF_TC_BODY(contextsplink, tc)
{
#if defined STACK_ALIGNBYTES && defined HAVE_CONTEXTSPFUNC
	ucontext_t uc1, uc2;
	char *stack1, *stack2;
	unsigned i;

	REQUIRE_LIBC(stack1 = malloc(SIGSTKSZ), NULL);
	fprintf(stderr, "stack1 @ [%p,%p)\n", stack1, stack1 + SIGSTKSZ);
	REQUIRE_LIBC(stack2 = malloc(SIGSTKSZ + STACK_ALIGNBYTES), NULL);
	fprintf(stderr, "stack2 @ [%p,%p)\n",
	    stack2, stack2 + SIGSTKSZ + STACK_ALIGNBYTES);

#ifdef __hppa__
	/*
	 * Not sure what the deal is but I probably wrote contextspfunc
	 * wrong.
	 */
	atf_tc_expect_signal(SIGILL, "PR kern/59327:"
	    " user stack pointer is not aligned properly");
#endif
#ifdef __mips_n64
	atf_tc_expect_fail("PR kern/59327:"
	    " user stack pointer is not aligned properly");
#endif

	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		contextsp = NULL;
		test_context_done = false;

		RL(getcontext(&uc1));
		uc1.uc_stack.ss_sp = stack1;
		uc1.uc_stack.ss_size = SIGSTKSZ;
		uc1.uc_link = &uc2;
		makecontext(&uc1, &contextnoop, 0);

		RL(getcontext(&uc2));
		uc2.uc_stack.ss_sp = stack2;
		uc2.uc_stack.ss_size = SIGSTKSZ + i;
		makecontext(&uc2, &contextspfunc, 0);

		fprintf(stderr, "[%u] swapcontext\n", i);
		RL(swapcontext(&return_context, &uc1));

		ATF_CHECK(contextsp != NULL);
		ATF_CHECK_MSG((uintptr_t)stack2 <= (uintptr_t)contextsp &&
		    (uintptr_t)contextsp <= (uintptr_t)stack2 + SIGSTKSZ + i,
		    "contextsp=%p", contextsp);
		ATF_CHECK_MSG(((uintptr_t)contextsp & STACK_ALIGNBYTES) == 0,
		    "[%u] makecontext function called with misaligned sp %p",
		    i, contextsp);
	}

	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		contextsp = NULL;
		test_context_done = false;

		RL(getcontext(&uc1));
		uc1.uc_stack.ss_sp = stack1;
		uc1.uc_stack.ss_size = SIGSTKSZ;
		uc1.uc_link = &uc2;
		makecontext(&uc1, &contextnoop, 0);

		RL(getcontext(&uc2));
		uc2.uc_stack.ss_sp = stack2 + i;
		uc2.uc_stack.ss_size = SIGSTKSZ;
		makecontext(&uc2, &contextspfunc, 0);

		fprintf(stderr, "[%u] swapcontext\n", i);
		RL(swapcontext(&return_context, &uc1));

		ATF_CHECK(contextsp != NULL);
		ATF_CHECK_MSG((uintptr_t)stack2 + i <= (uintptr_t)contextsp &&
		    (uintptr_t)contextsp <= (uintptr_t)stack2 + SIGSTKSZ + i,
		    "contextsp=%p", contextsp);
		ATF_CHECK_MSG(((uintptr_t)contextsp & STACK_ALIGNBYTES) == 0,
		    "[%u] makecontext function called with misaligned sp %p",
		    i, contextsp);
	}
#else
	atf_tc_skip("Not implemented on this platform");
#endif
}

ATF_TC(signalsp);
ATF_TC_HEAD(signalsp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify stack pointer is aligned on entry to signal handler");
}
ATF_TC_BODY(signalsp, tc)
{
#if defined STACK_ALIGNBYTES && defined HAVE_SIGNALSPHANDLER
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &signalsphandler;
	RL(sigaction(SIGUSR1, &sa, NULL));
	RL(raise(SIGUSR1));

	ATF_CHECK_MSG(((uintptr_t)signalsp & STACK_ALIGNBYTES) == 0,
	    "signal handler was called with a misaligned sp: %p",
	    signalsp);
#else
	atf_tc_skip("Not implemented on this platform");
#endif
}

ATF_TC(signalsp_sigaltstack);
ATF_TC_HEAD(signalsp_sigaltstack, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify stack pointer is aligned on entry to signal handler"
	    " with maximally misaligned sigaltstack");
}
ATF_TC_BODY(signalsp_sigaltstack, tc)
{
#if defined STACK_ALIGNBYTES && HAVE_SIGNALSPHANDLER
	char *stack;
	struct sigaction sa;
	struct sigaltstack ss;
	unsigned i;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &signalsphandler;
	sa.sa_flags = SA_ONSTACK;
	RL(sigaction(SIGUSR1, &sa, NULL));

	/*
	 * Allocate a signal stack with enough slop to try all possible
	 * misalignments of the stack pointer.  Print it to stderr so
	 * it always appears in atf output before shenanigans happen.
	 */
	REQUIRE_LIBC(stack = malloc(SIGSTKSZ + STACK_ALIGNBYTES), NULL);
	fprintf(stderr, "stack @ [%p, %p)\n",
	    stack, stack + SIGSTKSZ + STACK_ALIGNBYTES);

#if defined __alpha__ || defined __i386__ || defined __mips__
	atf_tc_expect_fail("PR kern/59327:"
	    " user stack pointer is not aligned properly");
#endif

	/*
	 * Try with all alignments of high addresses.
	 */
	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		ss.ss_sp = stack;
		ss.ss_size = SIGSTKSZ + i;
		ss.ss_flags = 0;
		RL(sigaltstack(&ss, NULL));

		signalsp = NULL;
		RL(raise(SIGUSR1));
		ATF_CHECK(signalsp != NULL);
		ATF_CHECK_MSG((uintptr_t)stack <= (uintptr_t)signalsp &&
		    (uintptr_t)signalsp <= (uintptr_t)stack + SIGSTKSZ + i,
		    "signalsp=%p", signalsp);
		ATF_CHECK_MSG(((uintptr_t)signalsp & STACK_ALIGNBYTES) == 0,
		    "[%u] signal handler was called with a misaligned sp: %p",
		    i, signalsp);
	}

	/*
	 * Try with all alignments of low addresses.
	 */
	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		ss.ss_sp = stack + i;
		ss.ss_size = SIGSTKSZ;
		ss.ss_flags = 0;
		RL(sigaltstack(&ss, NULL));

		signalsp = NULL;
		RL(raise(SIGUSR1));
		ATF_CHECK(signalsp != NULL);
		ATF_CHECK_MSG((uintptr_t)stack + i <= (uintptr_t)signalsp &&
		    (uintptr_t)signalsp <= (uintptr_t)stack + i + SIGSTKSZ,
		    "signalsp=%p", signalsp);
		ATF_CHECK_MSG(((uintptr_t)signalsp & STACK_ALIGNBYTES) == 0,
		    "[%u] signal handler was called with a misaligned sp: %p",
		    i, signalsp);
	}
#else
	atf_tc_skip("Not implemented on this platform");
#endif
}

#if defined STACK_ALIGNBYTES && defined HAVE_THREADSPFUNC
void *threadspfunc(void *);	/* threadspfunc.S assembly routine */
#endif

ATF_TC(threadsp);
ATF_TC_HEAD(threadsp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify stack pointer is aligned on thread start");
}
ATF_TC_BODY(threadsp, tc)
{
#if defined STACK_ALIGNBYTES && defined HAVE_THREADSPFUNC
	char *stack;
	unsigned i;

	REQUIRE_LIBC(stack = malloc(SIGSTKSZ + STACK_ALIGNBYTES), NULL);
	fprintf(stderr, "stack @ [%p,%p)\n", stack,
	    stack + SIGSTKSZ + STACK_ALIGNBYTES);

#ifdef __hppa__
	/*
	 * Not sure what the deal is but I probably wrote threadspfunc
	 * wrong.
	 */
	atf_tc_expect_signal(SIGBUS, "PR kern/59327:"
	    " user stack pointer is not aligned properly");
#endif
#ifdef __riscv__
	atf_tc_expect_fail("sp inexplicably lies outside stack range");
#endif

	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		pthread_t t;
		pthread_attr_t attr;
		void *sp;

		RZ(pthread_attr_init(&attr));
		RZ(pthread_attr_setstack(&attr, stack, SIGSTKSZ + i));
		RZ(pthread_create(&t, &attr, &threadspfunc, NULL));
		RZ(pthread_attr_destroy(&attr));

		alarm(1);
		RZ(pthread_join(t, &sp));
		alarm(0);

		ATF_CHECK(sp != NULL);
		ATF_CHECK_MSG((uintptr_t)stack <= (uintptr_t)sp &&
		    (uintptr_t)sp <= (uintptr_t)stack + SIGSTKSZ + i,
		    "sp=%p", sp);
		ATF_CHECK_MSG(((uintptr_t)signalsp & STACK_ALIGNBYTES) == 0,
		    "[%u] thread called with misaligned sp: %p", i, signalsp);
	}

	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		pthread_t t;
		pthread_attr_t attr;
		void *sp;

		RZ(pthread_attr_init(&attr));
		RZ(pthread_attr_setstack(&attr, stack + i, SIGSTKSZ));
		RZ(pthread_create(&t, &attr, &threadspfunc, NULL));
		RZ(pthread_attr_destroy(&attr));

		alarm(1);
		RZ(pthread_join(t, &sp));
		alarm(0);

		ATF_CHECK(sp != NULL);
		ATF_CHECK_MSG((uintptr_t)stack + i <= (uintptr_t)sp &&
		    (uintptr_t)sp <= (uintptr_t)stack + i + SIGSTKSZ,
		    "sp=%p", sp);
		ATF_CHECK_MSG(((uintptr_t)signalsp & STACK_ALIGNBYTES) == 0,
		    "[%u] thread called with misaligned sp: %p", i, signalsp);
	}
#else
	atf_tc_skip("Not implemented on this platform");
#endif
}

ATF_TC(misaligned_sp_and_signal);
ATF_TC_HEAD(misaligned_sp_and_signal, tc)
{
	atf_tc_set_md_var(tc, "descr", "process can return from a signal"
	    " handler even if the stack pointer is misaligned when a signal"
	    " arrives");
}
ATF_TC_BODY(misaligned_sp_and_signal, tc)
{
#if defined STACK_ALIGNBYTES && defined HAVE_STACK_POINTER_H
	/*
	 * Set up a handler for SIGALRM.
	 */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &signalsphandler;
	RL(sigaction(SIGALRM, &sa, NULL));

#if defined __alpha__ || defined __i386__ || defined __mips__
	atf_tc_expect_fail("PR kern/58149:"
	    " Cannot return from a signal handler"
	    " if SP was misaligned when the signal arrived");
#endif

	/*
	 * Set up an interval timer so that we receive SIGALRM after 50 ms.
	 */
	struct itimerval itv;
	memset(&itv, 0, sizeof(itv));
	itv.it_value.tv_usec = 1000 * 50;
	RL(setitimer(ITIMER_MONOTONIC, &itv, NULL));

	/*
	 * Now misalign the SP. Wait for the signal to arrive and see what
	 * happens. This should be fine as long as we don't use it to
	 * access memory.
	 */
	MISALIGN_SP;
	while (signalsp == NULL) {
		/*
		 * Make sure the compiler does not optimize this busy loop
		 * away.
		 */
		__asm__("" ::: "memory");
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
	ATF_CHECK_MSG(((uintptr_t)signalsp & STACK_ALIGNBYTES) == 0,
	    "signal handler was called with a misaligned sp: %p",
	    signalsp);
#else
	atf_tc_skip("Not implemented for this platform");
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, contextsp);
	ATF_TP_ADD_TC(tp, contextsplink);
	ATF_TP_ADD_TC(tp, execsp_dynamic);
	ATF_TP_ADD_TC(tp, execsp_static);
	ATF_TP_ADD_TC(tp, misaligned_sp_and_signal);
	ATF_TP_ADD_TC(tp, signalsp);
	ATF_TP_ADD_TC(tp, signalsp_sigaltstack);
	ATF_TP_ADD_TC(tp, threadsp);
	return atf_no_error();
}
