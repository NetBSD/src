/* $NetBSD: t_status.c,v 1.2 2010/11/03 16:10:22 christos Exp $ */

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2010\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_status.c,v 1.2 2010/11/03 16:10:22 christos Exp $");

#include <sys/resource.h>
#include <sys/wait.h>

#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#define NUM 100

static void *
threadfunc(void *arg)
{

	sleep(1000);
	return 0;
}

static int
child(const int seed)
{
	pthread_t thr[NUM];
	int i, j, res, errors;
	char nam[20];
	struct rlimit sl;

	srandom(seed);
	getrlimit(RLIMIT_STACK, &sl);

	errors = 0;
	for (i = 0; i < NUM; i++) {
		res = pthread_create(&thr[i], 0, threadfunc, 0);
		if (res)
			errx(1, "pthread_create iter %d: %s", i, strerror(res));
		for (j = 0; j <= i; j++) {
			res = pthread_getname_np(thr[j], nam, sizeof(nam));
			if (res) {
				warnx("getname(%d/%d): %s\n", i, j,
				    strerror(res));
				errors++;
			}
		}
		if (errors)
			break;
		malloc((random() & 7) * sl.rlim_cur);
	}
	if (errors)
		printf("%d errors; seed was %d\n", errors, seed);
	return errors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

static void
do_test(const int seed)
{
	pid_t pid = fork();
	ATF_REQUIRE(pid != -1);
	if (pid == 0) {
		exit(child(seed));
	} else {
		int status;
		ATF_CHECK_EQ(pid, waitpid(pid, &status, 0));
		if (!(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS))
			atf_tc_fail_nonfatal("Check with seed %d failed", seed);
	}
}

ATF_TC(findthreads);
ATF_TC_HEAD(findthreads, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests libpthread's ability to remember "
	    "the tests it has created");
}
ATF_TC_BODY(findthreads, tc)
{
	int i;

	for (i = 0; i < 100; i++)
		do_test(i);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, findthreads);

	return atf_no_error();
}
