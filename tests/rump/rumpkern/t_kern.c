/*	$NetBSD: t_kern.c,v 1.5 2017/05/03 12:09:41 pgoyette Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include <rump/rump.h>

#include <atf-c.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

#include "h_macros.h"
#include "../kernspace/kernspace.h"

#define LOCKFUN(_name_, _descr_,_needld_, _expect_)			\
	ATF_TC(lockme_##_name_);					\
	ATF_TC_HEAD(lockme_##_name_, tc) {				\
		atf_tc_set_md_var(tc, "descr", _descr_);		\
	}								\
	ATF_TC_BODY(lockme_##_name_, tc) {				\
		locktest(tc, LOCKME_##_name_, _needld_, _expect_);	\
	}

static void
locktest(const atf_tc_t *tc, enum locktest lt, int needld, const char *expect)
{
	extern const int rump_lockdebug;
	int pipetti[2];
	int status;
	ssize_t len;
	regex_t preg;

	if (needld && !rump_lockdebug)
		atf_tc_skip("test requires LOCKDEBUG kernel");
	RL(pipe(pipetti));

	switch (fork()) {
	case 0:
		RL(dup2(pipetti[1], STDOUT_FILENO));
		RL(dup2(pipetti[1], STDOUT_FILENO));
		rump_init();
		rump_schedule();
		rumptest_lockme(lt);
		rump_unschedule();
		break;
	default:
		RL(wait(&status));
		ATF_REQUIRE(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
		if (rump_lockdebug) {
			char buf[8192];

			len = read(pipetti[0], buf, sizeof(buf) - 1);
			ATF_REQUIRE(len > 0);
			buf[len] = '\0';
			/*
			 * We use regex matching here, since the rump
			 * kernel messages include routine names and line
			 * numbers which may not remain constant.
			 */
			if ((status = regcomp(&preg, expect, REG_BASIC)) != 0) {
				regerror(status, &preg, buf, sizeof(buf));
				printf("regcomp error: %s\n", buf);
				atf_tc_fail("regcomp failed");
			}
			if ((status = regexec(&preg, buf, 0, NULL, 0)) != 0) {
				printf("expected: \"%s\"\n", expect);
				printf("received: \"%s\"\n", buf);
				regerror(status, &preg, buf, sizeof(buf));
				printf("regexec error: %s\n", buf);
				atf_tc_fail("unexpected output");
			}
			regfree(&preg);
		}
		break;
	case -1:
		atf_tc_fail("fork");
	}
}

LOCKFUN(DESTROYHELD, "destroy lock while held", 0,
    "mutex error: mutex_destroy,.*: is locked or in use");
LOCKFUN(DOUBLEFREE, "free lock twice", 0,
    "panic: mutex_destroy,.*: uninitialized lock");
LOCKFUN(DOUBLEINIT, "init lock twice", 1,
    "mutex error: mutex_init,.*: already initialized");
LOCKFUN(MEMFREE, "free memory active lock is in", 1,
    "mutex error: kmem_intr_free,.*: allocation contains active lock");
LOCKFUN(MTX, "locking-against-self mutex", 0,
    "mutex error: mutex_enter,.*: locking against myself");
LOCKFUN(RWDOUBLEX, "locking-against-self exclusive rwlock", 0,
    "rwlock error: rw_enter,.*: locking against myself");
LOCKFUN(RWRX, "rw: first shared, then exclusive", 1,
    "rwlock error: rw_enter,.*: locking against myself");
LOCKFUN(RWXR, "rw: first execusive, then shared", 0,
    "rwlock error: rw_enter,.*: locking against myself");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, lockme_MTX);
	ATF_TP_ADD_TC(tp, lockme_RWDOUBLEX);
	ATF_TP_ADD_TC(tp, lockme_RWRX);
	ATF_TP_ADD_TC(tp, lockme_RWXR);
	ATF_TP_ADD_TC(tp, lockme_DOUBLEINIT);
	ATF_TP_ADD_TC(tp, lockme_DOUBLEFREE);
	ATF_TP_ADD_TC(tp, lockme_DESTROYHELD);
	ATF_TP_ADD_TC(tp, lockme_MEMFREE);

	return atf_no_error();
}
