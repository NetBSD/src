/*	$NetBSD: t_zombie.c,v 1.1.2.3 2018/05/21 04:36:17 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2018\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_zombie.c,v 1.1.2.3 2018/05/21 04:36:17 pgoyette Exp $");

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <err.h>

#include <atf-c.h>

static int debug = 0;

#define DPRINTF(a, ...)							\
do {									\
        if (debug) printf(a,  ##__VA_ARGS__);				\
} while (/*CONSTCOND*/0)

/*
 * A child process cannot call atf functions and expect them to magically
 * work like in the parent.
 * The printf(3) messaging from a child will not work out of the box as well
 * without estabilishing a communication protocol with its parent. To not
 * overcomplicate the tests - do not log from a child and use err(3)/errx(3)
 * wrapped with ASSERT_EQ()/ASSERT_NEQ() as that is guaranteed to work.
 */
#define ASSERT_EQ(x, y)								\
do {										\
	uintmax_t vx = (x);							\
	uintmax_t vy = (y);							\
	int ret = vx == vy;							\
	if (!ret)								\
		errx(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: "		\
		    "%s(%ju) == %s(%ju)", __FILE__, __LINE__, __func__,		\
		    #x, vx, #y, vy);						\
} while (/*CONSTCOND*/0)

#define ASSERT_NEQ(x, y)							\
do {										\
	uintmax_t vx = (x);							\
	uintmax_t vy = (y);							\
	int ret = vx != vy;							\
	if (!ret)								\
		errx(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: "		\
		    "%s(%ju) != %s(%ju)", __FILE__, __LINE__, __func__,		\
		    #x, vx, #y, vy);						\
} while (/*CONSTCOND*/0)

#define ASSERT(x)								\
do {										\
	int ret = (x);								\
	if (!ret)								\
		errx(EXIT_FAILURE, "%s:%d %s(): Assertion failed for: %s",	\
		     __FILE__, __LINE__, __func__, #x);				\
} while (/*CONSTCOND*/0)

static bool
check_zombie(pid_t process)
{
	struct kinfo_proc2 p;
	size_t len = sizeof(p);

	const int name[] = {
		[0] = CTL_KERN,
		[1] = KERN_PROC2,
		[2] = KERN_PROC_PID,
		[3] = process,
		[4] = sizeof(p),
		[5] = 1
	};

	const size_t namelen = __arraycount(name);

	ASSERT_EQ(sysctl(name, namelen, &p, &len, NULL, 0), 0);

	return (p.p_stat == LSZOMB);
}

static void __used
await_zombie(pid_t process)
{

	/* Await the process becoming a zombie */
	while (!check_zombie(process)) {
		ASSERT_EQ(usleep(100), 0);
	}
}

static void
signal_raw(int sig)
{
	int status;
	pid_t child1, child2, pid;

	child1 = atf_utils_fork();
	ATF_REQUIRE(child1 != -1);
	if (child1 == 0) {
		/* Just die and turn into a zombie */
		_exit(0);
	}

	child2 = atf_utils_fork();
	ATF_REQUIRE(child2 != -1);
	if (child2 == 0) {
		await_zombie(child1);

		/*
		 * zombie does not process signals
		 * POSIX requires that zombie does not set errno ESRCH
		 * return value of kill() for a zombie is not specified
		 *
		 * Try to emit a signal towards it from an unrelated process.
		 */
		errno = 0;
		kill(child1, sig);
		ASSERT_NEQ(errno, ESRCH);

		/* A zombie is still a zombie waiting for collecting */
		ASSERT(check_zombie(child1));

		_exit(0);
	}

	pid = waitpid(child2, &status, WEXITED);
	ATF_REQUIRE_EQ(pid, child2);
	ATF_REQUIRE(WIFEXITED(status));
	ATF_REQUIRE(!WIFCONTINUED(status));
	ATF_REQUIRE(!WIFSIGNALED(status));
	ATF_REQUIRE(!WIFSTOPPED(status));
	ATF_REQUIRE_EQ(WEXITSTATUS(status), 0);

	/* Assert that child1 is still a zombie after collecting child2 */
	ATF_REQUIRE(check_zombie(child1));

	/*
	 * zombie does not process signals
	 * POSIX requires that zombie does not set errno ESRCH
	 * return value of kill() for a zombie is not specified
	 *
	 * Try to emit a signal towards it from the parent.
	 */
	errno = 0;
	kill(child1, sig);
	// ATF_CHECK_NEQ not available
	ASSERT_NEQ(errno, ESRCH);

	/* Assert that child1 is still a zombie after emitting a signal */
	ATF_REQUIRE(check_zombie(child1));

	pid = waitpid(child1, &status, WEXITED);
	ATF_REQUIRE_EQ(pid, child1);
	ATF_REQUIRE(WIFEXITED(status));
	ATF_REQUIRE(!WIFCONTINUED(status));
	ATF_REQUIRE(!WIFSIGNALED(status));
	ATF_REQUIRE(!WIFSTOPPED(status));
	ATF_REQUIRE_EQ(WEXITSTATUS(status), 0);
}

#define KILLABLE(test, sig)							\
ATF_TC(test);									\
ATF_TC_HEAD(test, tc)								\
{										\
										\
	atf_tc_set_md_var(tc, "descr",						\
	    "process is not killable with " #sig);				\
}										\
										\
ATF_TC_BODY(test, tc)								\
{										\
										\
	signal_raw(sig);							\
}

KILLABLE(signal1, SIGKILL) /* non-maskable */
KILLABLE(signal2, SIGSTOP) /* non-maskable */
KILLABLE(signal3, SIGABRT) /* regular abort trap */
KILLABLE(signal4, SIGHUP)  /* hangup */
KILLABLE(signal5, SIGCONT) /* continued? */

ATF_TC(race1);
ATF_TC_HEAD(race1, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "check if there are any races with sending signals, killing and "
	    "lookup of a zombie");
}

ATF_TC_BODY(race1, tc)
{
        time_t start, end;
        double diff;
        unsigned long N = 0;
	int sig;

	/*
	 * Assert that a dying process can be correctly looked up
	 * with sysctl(3) kern.proc and operation KERN_PROC_PID.
	 *
	 * This test has been inspired by a bug fixed in
	 * sys/kern/kern_proc.c 1.211
	 * "Make sysctl_doeproc() more predictable"
	 */

        start = time(NULL);
        while (true) {
		/*
		 * A signal number does not matter, but it does not harm to
		 * randomize it.
		 *
		 * Skip signal 0 as sending to it to a zombie is not
		 * defined in POSIX, and explicitly discouraged.
		 */
		sig = 1 + arc4random_uniform(NSIG - 2);

                DPRINTF("Step: %lu (signal: %s)\n", N, signalname(sig));

		signal_raw(sig);
                end = time(NULL);
                diff = difftime(end, start);
                if (diff >= 5.0)
                        break;
                ++N;
        }
        DPRINTF("Iterations: %lu\n", N);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, signal1);
	ATF_TP_ADD_TC(tp, signal2);
	ATF_TP_ADD_TC(tp, signal3);
	ATF_TP_ADD_TC(tp, signal4);
	ATF_TP_ADD_TC(tp, signal5);

	ATF_TP_ADD_TC(tp, race1);

	return atf_no_error();
}
