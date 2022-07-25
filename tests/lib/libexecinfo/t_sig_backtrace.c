/*	$NetBSD: t_sig_backtrace.c,v 1.6 2022/07/25 22:43:01 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__RCSID("$NetBSD: t_sig_backtrace.c,v 1.6 2022/07/25 22:43:01 riastradh Exp $");

#include <sys/mman.h>
#include <execinfo.h>
#include <setjmp.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

stack_t sig_stack;

char *foo;
char *(*bar)(void);

static int the_loop_deref(int);
static int the_loop_jump(int);

#ifdef NOINLINE_HACK
volatile int noinline;
#endif

static int __noinline
func1(int i)
{
	if (i > 100) {
		return the_loop_deref(i);
	}
	return i + 1;
}

static int __noinline
func2(int i)
{
	return func1(i) << 1;
}

static int __noinline
func3(int i)
{
	if (func1(i) < 10) {
		return func2(i);
	} else {
		return func1(i);
	}
}

static int __noinline
the_loop_deref(int i0)
{
	volatile int i = i0;

	while (*foo != 0) {
		i = func3(i);
		i = func1(i);
		i = func2(i);
	}

#ifdef NOINLINE_HACK
	if (noinline)
		vfork();
#endif

	return i;
}

static int __noinline
the_loop_jump(int i0)
{
	volatile int i = i0;

	while ((*bar)() != 0) {
		i = func3(i);
		i = func1(i);
		i = func2(i);
	}

#ifdef NOINLINE_HACK
	if (noinline)
		vfork();
#endif

	return i;
}

jmp_buf env;

static void
handler(int s)
{
	printf("signal: %d\n", s);

	void *array[10];
	size_t size = backtrace(array, 10);
	ATF_REQUIRE(size != 0);

	printf("Backtrace %zd stack frames.\n", size);
	backtrace_symbols_fd(array, size, STDOUT_FILENO);

	char **strings = backtrace_symbols_fmt(array, size, "%n");
	bool found_handler = false;
	bool found_sigtramp = false;
	bool found_the_loop = false;
	bool found_main = false;
	size_t i;

	/*
	 * We must find the symbols in the following order:
	 *
	 * handler -> __sigtramp_siginfo_* -> the_loop -> main
	 */
	for (i = 0; i < size; i++) {
		if (!found_handler &&
		    strcmp(strings[i], "handler") == 0) {
			found_handler = true;
			continue;
		}
		if (found_handler && !found_sigtramp &&
		    strncmp(strings[i], "__sigtramp_siginfo_",
			    strlen("__sigtramp_siginfo_")) == 0) {
			found_sigtramp = true;
			continue;
		}
		if (found_sigtramp && !found_the_loop &&
		    strncmp(strings[i], "the_loop", strlen("the_loop")) == 0) {
			found_the_loop = true;
			continue;
		}
		if (found_the_loop && !found_main &&
		    strcmp(strings[i], "atf_tp_main") == 0) {
			found_main = true;
			break;
		}
	}

	ATF_REQUIRE(found_handler);
	ATF_REQUIRE(found_sigtramp);
	ATF_REQUIRE(found_the_loop);
	ATF_REQUIRE(found_main);

	longjmp(env, 1);
}

ATF_TC(sig_backtrace_deref);
ATF_TC_HEAD(sig_backtrace_deref, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test backtrace(3) across signal handlers, null pointer deref");
}

ATF_TC_BODY(sig_backtrace_deref, tc)
{
	sig_stack.ss_sp = mmap(NULL, SIGSTKSZ, PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_STACK, -1, 0);
	ATF_REQUIRE(sig_stack.ss_sp != MAP_FAILED);

	sig_stack.ss_size = SIGSTKSZ;
	sig_stack.ss_flags = 0;
	ATF_REQUIRE(sigaltstack(&sig_stack, NULL) == 0);

	struct sigaction sa = {
		.sa_handler = handler,
		.sa_flags = SA_ONSTACK,
	};
	ATF_REQUIRE(sigaction(SIGSEGV, &sa, NULL) == 0);

	if (setjmp(env) == 0) {
		printf("%d\n", the_loop_deref(0));
	}
}

ATF_TC(sig_backtrace_jump);
ATF_TC_HEAD(sig_backtrace_jump, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test backtrace(3) across signal handlers, null pointer jump");
}

ATF_TC_BODY(sig_backtrace_jump, tc)
{
	sig_stack.ss_sp = mmap(NULL, SIGSTKSZ, PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_STACK, -1, 0);
	ATF_REQUIRE(sig_stack.ss_sp != MAP_FAILED);

	sig_stack.ss_size = SIGSTKSZ;
	sig_stack.ss_flags = 0;
	ATF_REQUIRE(sigaltstack(&sig_stack, NULL) == 0);

	struct sigaction sa = {
		.sa_handler = handler,
		.sa_flags = SA_ONSTACK,
	};
	ATF_REQUIRE(sigaction(SIGSEGV, &sa, NULL) == 0);

	atf_tc_expect_fail("PR lib/56940");

	if (setjmp(env) == 0) {
		printf("%d\n", the_loop_jump(0));
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, sig_backtrace_deref);
	ATF_TP_ADD_TC(tp, sig_backtrace_jump);

	return atf_no_error();
}
