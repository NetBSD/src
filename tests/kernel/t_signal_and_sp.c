/*	$NetBSD: t_signal_and_sp.c,v 1.22 2026/08/28 06:35:26 riastradh Exp $	*/

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

#define	__EXPOSE_STACK	/* <sys/param.h>: expose STACK_ALIGNBYTES */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_signal_and_sp.c,v 1.22 2026/08/28 06:35:26 riastradh Exp $");

#include <sys/param.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <machine/vmparam.h>

#include <atf-c.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#include "getstack.h"
#include "h_execsp.h"
#include "h_macros.h"

#ifdef HAVE_STACK_POINTER_H
#  include "stack_pointer.h"
#endif

#define PR_59327 "PR kern/59327: user stack pointer is not aligned properly"

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
#if defined(__sh__)
	atf_tc_expect_fail(PR_59327);
#endif
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
	pthread_t t;
	void *sp;
	char *stack;
	unsigned i;

	REQUIRE_LIBC(stack = malloc(SIGSTKSZ + STACK_ALIGNBYTES), NULL);
	fprintf(stderr, "stack @ [%p,%p)\n", stack,
	    stack + SIGSTKSZ + STACK_ALIGNBYTES);

	RZ(pthread_create(&t, NULL, &threadspfunc, NULL));

	alarm(1);
	RZ(pthread_join(t, &sp));
	alarm(0);

	ATF_CHECK(sp != NULL);
	ATF_CHECK_MSG(((uintptr_t)sp & STACK_ALIGNBYTES) == 0,
	    "thread called with misaligned sp: %p", sp);

	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		pthread_attr_t attr;

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
		ATF_CHECK_MSG(((uintptr_t)sp & STACK_ALIGNBYTES) == 0,
		    "[%u] thread called with misaligned sp: %p", i, sp);
	}

	for (i = 0; i <= STACK_ALIGNBYTES; i++) {
		pthread_attr_t attr;

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
		ATF_CHECK_MSG(((uintptr_t)sp & STACK_ALIGNBYTES) == 0,
		    "[%u] thread called with misaligned sp: %p", i, sp);
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
#if defined(__sh__)
	atf_tc_expect_fail(PR_59327);
#endif

	/*
	 * Set up a handler for SIGALRM.
	 */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &signalsphandler;
	RL(sigaction(SIGALRM, &sa, NULL));

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

ATF_TC(getstack_self);
ATF_TC_HEAD(getstack_self, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length");
}
ATF_TC_BODY(getstack_self, tc)
{
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_fork);
ATF_TC_HEAD(getstack_fork, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " in forked child");
}
ATF_TC_BODY(getstack_fork, tc)
{
	pid_t pid;
	int status;

	printf("child\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	RL(pid = fork());
	if (pid == 0)
		_exit(checkgetstack());
	RL(waitpid(pid, &status, 0));
	if (WIFSIGNALED(status)) {
		atf_tc_fail_nonfatal("child exited on signal %d (%s)",
		    WTERMSIG(status), strsignal(WTERMSIG(status)));
	} else if (!WIFEXITED(status)) {
		atf_tc_fail_nonfatal("child exited status=0x%x", status);
	} else {
		/* fork screws up child's stack base */
		atf_tc_expect_fail("PR kern/60653:"
		    " posix_spawn(3) causes incorrect stack base information");
		ATF_CHECK_MSG(WEXITSTATUS(status) == 0,
		    "child exited with code %d",
		    WEXITSTATUS(status));
		atf_tc_expect_pass();
	}

	printf("parent\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_vfork);
ATF_TC_HEAD(getstack_vfork, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " in vforked child");
}
ATF_TC_BODY(getstack_vfork, tc)
{
	pid_t pid;
	int status;

	printf("child\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	RL(pid = vfork());
	if (pid == 0)
		_exit(checkgetstack_singlethreaded());
	RL(waitpid(pid, &status, 0));
	if (WIFSIGNALED(status)) {
		atf_tc_fail_nonfatal("child exited on signal %d (%s)",
		    WTERMSIG(status), strsignal(WTERMSIG(status)));
	} else if (!WIFEXITED(status)) {
		atf_tc_fail_nonfatal("child exited status=0x%x", status);
	} else {
		/* vfork screws up child's stack base */
		atf_tc_expect_fail("PR kern/60653:"
		    " posix_spawn(3) causes incorrect stack base information");
		ATF_CHECK_MSG(WEXITSTATUS(status) == 0,
		    "child exited with code %d",
		    WEXITSTATUS(status));
		atf_tc_expect_pass();
	}

	printf("parent\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_forkexec);
ATF_TC_HEAD(getstack_forkexec, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " in forked and execed child");
}
ATF_TC_BODY(getstack_forkexec, tc)
{
	char h_getstack[PATH_MAX];
	pid_t pid;
	int status;

	RL(snprintf(h_getstack, sizeof(h_getstack), "%s/%s",
		atf_tc_get_config_var(tc, "srcdir"), "h_getstack"));

	printf("child\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	RL(pid = fork());
	if (pid == 0) {
		char *const argv[] = {h_getstack, NULL};

		if (closefrom(STDERR_FILENO + 1) == -1)
			_exit(1);
		if (execve(argv[0], argv, NULL) == -1)
			_exit(2);
		_exit(3);
	}

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

	printf("parent\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_vforkexec);
ATF_TC_HEAD(getstack_vforkexec, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " in vforked and execed child");
}
ATF_TC_BODY(getstack_vforkexec, tc)
{
	char h_getstack[PATH_MAX];
	pid_t pid;
	int status;

	RL(snprintf(h_getstack, sizeof(h_getstack), "%s/%s",
		atf_tc_get_config_var(tc, "srcdir"), "h_getstack"));

	printf("child\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	RL(pid = vfork());
	if (pid == 0) {
		char *const argv[] = {h_getstack, NULL};

		if (closefrom(STDERR_FILENO + 1) == -1)
			_exit(1);
		if (execve(argv[0], argv, NULL) == -1)
			_exit(2);
		_exit(3);
	}

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

	printf("parent\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_spawn);
ATF_TC_HEAD(getstack_spawn, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " in posix_spawned child");
}
ATF_TC_BODY(getstack_spawn, tc)
{
	char h_getstack[PATH_MAX];
	char *const argv[] = {h_getstack, NULL};
	pid_t pid;
	int status;

	RL(snprintf(h_getstack, sizeof(h_getstack), "%s/%s",
		atf_tc_get_config_var(tc, "srcdir"), "h_getstack"));

	printf("child\n");
	REQUIRE_LIBC(fflush(stdout), EOF);
	RZ(posix_spawn(&pid, argv[0], NULL, NULL, argv, NULL));

	RL(waitpid(pid, &status, 0));
	if (WIFSIGNALED(status)) {
		atf_tc_fail_nonfatal("child exited on signal %d (%s)",
		    WTERMSIG(status), strsignal(WTERMSIG(status)));
	} else if (!WIFEXITED(status)) {
		atf_tc_fail_nonfatal("child exited status=0x%x", status);
	} else {
		/* posix_spawn screws up child's stack base */
		atf_tc_expect_fail("PR kern/60653:"
		    " posix_spawn(3) causes incorrect stack base information");
		ATF_CHECK_MSG(WEXITSTATUS(status) == 0,
		    "child exited with code %d",
		    WEXITSTATUS(status));
		atf_tc_expect_pass();
	}

	printf("parent\n");
	REQUIRE_LIBC(fflush(stdout), EOF);

	/* and posix_spawn screws up parent's stack base */
	atf_tc_expect_fail("PR kern/60653:"
	    " posix_spawn(3) causes incorrect stack base information");
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedspawn_nonexistent);
ATF_TC_HEAD(getstack_failedspawn_nonexistent, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed posix_spawn due to nonexistent executable");
}
ATF_TC_BODY(getstack_failedspawn_nonexistent, tc)
{
	char *const argv[] = {__UNCONST("/nonexistent"), NULL};
	pid_t pid;

	ATF_CHECK_ERRNO(ENOENT,
	    (errno = posix_spawn(&pid, argv[0], NULL, NULL, argv, NULL)) != 0);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedspawn_rlimit_nproc);
ATF_TC_HEAD(getstack_failedspawn_rlimit_nproc, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed posix_spawn due to RLIMIT_NPROC");
	/* root can bypass RLIMIT_NPROC */
	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}
ATF_TC_BODY(getstack_failedspawn_rlimit_nproc, tc)
{
	char *const argv[] = {__UNCONST("/usr/bin/true"), NULL};
	struct rlimit rlim0, rlim;
	pid_t pid;

	ATF_CHECK(getuid() != 0);
	ATF_CHECK(geteuid() != 0);
	RL(getrlimit(RLIMIT_NPROC, &rlim0));
	rlim = rlim0;
	rlim.rlim_cur = 0;
	RL(setrlimit(RLIMIT_NPROC, &rlim));
	ATF_CHECK_ERRNO(EAGAIN,
	    (errno = posix_spawn(&pid, argv[0], NULL, NULL, argv, NULL)) != 0);
	RL(setrlimit(RLIMIT_NPROC, &rlim0));
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedspawn_rlimit_data);
ATF_TC_HEAD(getstack_failedspawn_rlimit_data, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed posix_spawn due to RLIMIT_DATA");
}
ATF_TC_BODY(getstack_failedspawn_rlimit_data, tc)
{
	char *const argv[] = {__UNCONST("/usr/bin/true"), NULL};
	struct rlimit rlim0, rlim;
	pid_t pid;

	RL(getrlimit(RLIMIT_DATA, &rlim0));
	rlim = rlim0;
	rlim.rlim_cur = 0;
	RL(setrlimit(RLIMIT_DATA, &rlim));
	ATF_CHECK_ERRNO(ENOMEM,
	    (errno = posix_spawn(&pid, argv[0], NULL, NULL, argv, NULL)) != 0);
	RL(setrlimit(RLIMIT_DATA, &rlim0));

	/* posix_spawn screws up parent's stack base */
	atf_tc_expect_fail("PR kern/60653:"
	    " posix_spawn(3) causes incorrect stack base information");
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedspawn_rlimit_stack);
ATF_TC_HEAD(getstack_failedspawn_rlimit_stack, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed posix_spawn due to RLIMIT_STACK");
}
ATF_TC_BODY(getstack_failedspawn_rlimit_stack, tc)
{
	char *argv[] = {__UNCONST("/usr/bin/true"), NULL, NULL};
	size_t bigsize;
	struct rlimit rlim0, rlim;
	pid_t pid;

	/*
	 * Get a size small enough to fit on the stack, but big enough
	 * for setrlimit(RLIMIT_STACK) to allow it temporarily (with a
	 * little extra stack depth for another subroutine call).
	 */
	(void)getstack_getcontext(&bigsize);
	bigsize = roundup(bigsize, getpagesize());
	bigsize += getpagesize();
	fprintf(stderr, "bigsize = %zx\n", bigsize);
	REQUIRE_LIBC(argv[1] = malloc(bigsize), NULL);
	memset(argv[1], 'c', bigsize - 1);
	argv[1][bigsize - 1] = '\0';

	RL(getrlimit(RLIMIT_STACK, &rlim0));
	rlim = rlim0;
	rlim.rlim_cur = bigsize;
	RL(setrlimit(RLIMIT_STACK, &rlim));
	ATF_CHECK_ERRNO(ENOMEM,
	    (errno = posix_spawn(&pid, argv[0], NULL, NULL, argv, NULL)) != 0);
	RL(setrlimit(RLIMIT_STACK, &rlim0));

	/* posix_spawn screws up parent's stack base */
	atf_tc_expect_fail("PR kern/60653:"
	    " posix_spawn(3) causes incorrect stack base information");
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedspawn_badfileaction);
ATF_TC_HEAD(getstack_failedspawn_badfileaction, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed posix_spawn due to bad file actions");
}
ATF_TC_BODY(getstack_failedspawn_badfileaction, tc)
{
	char *const argv[] = {__UNCONST("/usr/bin/true"), NULL};
	posix_spawn_file_actions_t fa;
	posix_spawnattr_t at;
	int fd;
	pid_t pid;

	/*
	 * Open a file descriptor number that we close in the child so
	 * that dup2ing it will fail.
	 */
	RL(fd = open("/dev/null", O_RDONLY));

	RZ(posix_spawn_file_actions_init(&fa));
	RZ(posix_spawn_file_actions_addclose(&fa, fd));
	RZ(posix_spawn_file_actions_adddup2(&fa, fd, STDERR_FILENO + 1));
	RZ(posix_spawnattr_init(&at));
	RZ(posix_spawnattr_setflags(&at, POSIX_SPAWN_RETURNERROR));
	ATF_CHECK_ERRNO(EBADF,
	    (errno = posix_spawn(&pid, argv[0], &fa, &at, argv, NULL)) != 0);
	RZ(posix_spawnattr_destroy(&at));
	RZ(posix_spawn_file_actions_destroy(&fa));

	RL(close(fd));

	/* some exec failure paths leave stack base screwy */
	atf_tc_expect_fail("PR kern/60653:"
	    " posix_spawn(3) causes incorrect stack base information");
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_nonexistent);
ATF_TC_HEAD(getstack_failedexec_nonexistent, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with nonexistent executable");
}
ATF_TC_BODY(getstack_failedexec_nonexistent, tc)
{
	char *const argv[] = {__UNCONST("/nonexistent"), NULL};

	ATF_CHECK_ERRNO(ENOENT, execve(argv[0], argv, NULL) == -1);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_argvnull);
ATF_TC_HEAD(getstack_failedexec_argvnull, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with argv=NULL");
}
ATF_TC_BODY(getstack_failedexec_argvnull, tc)
{
	char *const argv[] = {__UNCONST("/usr/bin/true"), NULL};

	ATF_CHECK_ERRNO(EINVAL, execve(argv[0], NULL, NULL) == -1);

	/* some exec failure paths leave stack base screwy */
	atf_tc_expect_fail("PR kern/60653:"
	    " posix_spawn(3) causes incorrect stack base information");
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_device);
ATF_TC_HEAD(getstack_failedexec_device, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with device");
}
ATF_TC_BODY(getstack_failedexec_device, tc)
{
	char *const argv[] = {__UNCONST("/dev/null"), NULL};

	ATF_CHECK_ERRNO(EACCES, execve(argv[0], argv, NULL) == -1);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_badinterp);
ATF_TC_HEAD(getstack_failedexec_badinterp, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with a bad #! interpreter");
}
ATF_TC_BODY(getstack_failedexec_badinterp, tc)
{
	char *const argv[] = {__UNCONST("./file"), NULL};
	FILE *fp;

	REQUIRE_LIBC(fp = fopen(argv[0], "w"), NULL);
	RL(fprintf(fp, "#!/nonexistent\n"));
	REQUIRE_LIBC(fclose(fp), EOF);

	RL(chmod(argv[0], 0755));
	ATF_CHECK_ERRNO(ENOENT, execve(argv[0], argv, NULL) == -1);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_nonexecutable);
ATF_TC_HEAD(getstack_failedexec_nonexecutable, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with a nonexecutable file");
}
ATF_TC_BODY(getstack_failedexec_nonexecutable, tc)
{
	char *const argv[] = {__UNCONST("./file"), NULL};
	FILE *fp;

	REQUIRE_LIBC(fp = fopen(argv[0], "w"), NULL);
	RL(fprintf(fp, "#!/bin/sh\n"));
	REQUIRE_LIBC(fclose(fp), EOF);

	RL(chmod(argv[0], 0644));
	ATF_CHECK_ERRNO(EACCES, execve(argv[0], argv, NULL) == -1);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_nonreadable);
ATF_TC_HEAD(getstack_failedexec_nonreadable, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with a nonreadable file");
}
ATF_TC_BODY(getstack_failedexec_nonreadable, tc)
{
	char *const argv[] = {__UNCONST("./file"), NULL};
	FILE *fp;

	REQUIRE_LIBC(fp = fopen(argv[0], "w"), NULL);
	RL(fprintf(fp, "#!bin/sh\n"));
	REQUIRE_LIBC(fclose(fp), EOF);

	RL(chmod(argv[0], 0000));
	ATF_CHECK_ERRNO(EACCES, execve(argv[0], argv, NULL) == -1);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_badformat);
ATF_TC_HEAD(getstack_failedexec_badformat, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with a file in a bad format");
}
ATF_TC_BODY(getstack_failedexec_badformat, tc)
{
	char *const argv[] = {__UNCONST("./file"), NULL};
	FILE *fp;

	REQUIRE_LIBC(fp = fopen(argv[0], "w"), NULL);
	RL(fprintf(fp, "not an executable\n"));
	REQUIRE_LIBC(fclose(fp), EOF);

	RL(chmod(argv[0], 0755));
	ATF_CHECK_ERRNO(ENOEXEC, execve(argv[0], argv, NULL) == -1);
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_rlimit_data);
ATF_TC_HEAD(getstack_failedexec_rlimit_data, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with RLIMIT_DATA too small");
}
ATF_TC_BODY(getstack_failedexec_rlimit_data, tc)
{
	char *const argv[] = {__UNCONST("/usr/bin/true"), NULL};
	struct rlimit rlim0, rlim;

	RL(getrlimit(RLIMIT_DATA, &rlim0));
	rlim = rlim0;
	rlim.rlim_cur = 0;
	RL(setrlimit(RLIMIT_DATA, &rlim));
	ATF_CHECK_ERRNO(ENOMEM, execve(argv[0], argv, NULL) == -1);
	RL(setrlimit(RLIMIT_DATA, &rlim0));

	/* some exec failure paths leave stack base screwy */
	atf_tc_expect_fail("PR kern/60653:"
	    " posix_spawn(3) causes incorrect stack base information");
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TC(getstack_failedexec_rlimit_stack);
ATF_TC_HEAD(getstack_failedexec_rlimit_stack, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify stack base and length"
	    " after failed exec with RLIMIT_STACK too small");
}
ATF_TC_BODY(getstack_failedexec_rlimit_stack, tc)
{
	char *argv[] = {__UNCONST("/usr/bin/true"), NULL, NULL};
	size_t bigsize;
	struct rlimit rlim0, rlim;

	/*
	 * Get a size small enough to fit on the stack, but big enough
	 * for setrlimit(RLIMIT_STACK) to allow it temporarily (with a
	 * little extra stack depth for another subroutine call).
	 */
	(void)getstack_getcontext(&bigsize);
	bigsize = roundup(bigsize, getpagesize());
	bigsize += getpagesize();
	fprintf(stderr, "bigsize = %zx\n", bigsize);
	REQUIRE_LIBC(argv[1] = malloc(bigsize), NULL);
	memset(argv[1], 'c', bigsize - 1);
	argv[1][bigsize - 1] = '\0';

	RL(getrlimit(RLIMIT_STACK, &rlim0));
	rlim = rlim0;
	rlim.rlim_cur = bigsize;
	RL(setrlimit(RLIMIT_STACK, &rlim));
	ATF_CHECK_ERRNO(ENOMEM, execve(argv[0], argv, NULL) == -1);
	RL(setrlimit(RLIMIT_STACK, &rlim0));

	/* some exec failure paths leave stack base screwy */
	atf_tc_expect_fail("PR kern/60653:"
	    " posix_spawn(3) causes incorrect stack base information");
	ATF_CHECK(checkgetstack() == 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, contextsp);
	ATF_TP_ADD_TC(tp, contextsplink);
	ATF_TP_ADD_TC(tp, execsp_dynamic);
	ATF_TP_ADD_TC(tp, execsp_static);
	ATF_TP_ADD_TC(tp, getstack_failedexec_argvnull);
	ATF_TP_ADD_TC(tp, getstack_failedexec_badformat);
	ATF_TP_ADD_TC(tp, getstack_failedexec_badinterp);
	ATF_TP_ADD_TC(tp, getstack_failedexec_device);
	ATF_TP_ADD_TC(tp, getstack_failedexec_nonexecutable);
	ATF_TP_ADD_TC(tp, getstack_failedexec_nonexistent);
	ATF_TP_ADD_TC(tp, getstack_failedexec_nonreadable);
	ATF_TP_ADD_TC(tp, getstack_failedexec_rlimit_data);
	ATF_TP_ADD_TC(tp, getstack_failedexec_rlimit_stack);
	ATF_TP_ADD_TC(tp, getstack_failedspawn_badfileaction);
	ATF_TP_ADD_TC(tp, getstack_failedspawn_nonexistent);
	ATF_TP_ADD_TC(tp, getstack_failedspawn_rlimit_data);
	ATF_TP_ADD_TC(tp, getstack_failedspawn_rlimit_nproc);
	ATF_TP_ADD_TC(tp, getstack_failedspawn_rlimit_stack);
	ATF_TP_ADD_TC(tp, getstack_fork);
	ATF_TP_ADD_TC(tp, getstack_forkexec);
	ATF_TP_ADD_TC(tp, getstack_self);
	ATF_TP_ADD_TC(tp, getstack_spawn);
	ATF_TP_ADD_TC(tp, getstack_vfork);
	ATF_TP_ADD_TC(tp, getstack_vforkexec);
	ATF_TP_ADD_TC(tp, misaligned_sp_and_signal);
	ATF_TP_ADD_TC(tp, signalsp);
	ATF_TP_ADD_TC(tp, signalsp_sigaltstack);
	ATF_TP_ADD_TC(tp, threadsp);
	return atf_no_error();
}
