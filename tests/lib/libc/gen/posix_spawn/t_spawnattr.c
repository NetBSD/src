/* $NetBSD: t_spawnattr.c,v 1.7 2025/03/16 15:35:36 riastradh Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles Zhang <charles@NetBSD.org> and
 * Martin Husemann <martin@NetBSD.org>.
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
__RCSID("$NetBSD: t_spawnattr.c,v 1.7 2025/03/16 15:35:36 riastradh Exp $");

#include <sys/param.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "h_macros.h"

static int
get_different_scheduler(void)
{
	int sched;

	/*
	 * We don't want to use SCHED_OTHER because it does not have
	 * different priorities.
	 */

	/* get current schedule policy */
	RL(sched = sched_getscheduler(0));
	switch (sched) {
	case SCHED_RR:
		return SCHED_FIFO;
	case SCHED_FIFO:
	case SCHED_OTHER:
		return SCHED_RR;
	default:
		abort();
	}
}

static int
get_different_priority(int scheduler)
{
	int min, max, new, priority;
	struct sched_param param;

	/* Get the priority range for the new scheduler */
	RL(max = sched_get_priority_max(scheduler));
	RL(min = sched_get_priority_min(scheduler));

	RL(sched_getparam(0, &param));
	priority = param.sched_priority;

	/* new schedule policy */
	for (new = min; new <= max; new++)
		if (priority != new)
			break;

	ATF_REQUIRE_MSG(priority != new, "could not find different priority");
	printf("min %d max %d for scheduler %d, returning %d\n",
	    min, max, scheduler, new);
	return new;
}

ATF_TC(t_spawnattr);
ATF_TC_HEAD(t_spawnattr, tc)
{
	atf_tc_set_md_var(tc, "require.user", "root");
	atf_tc_set_md_var(tc, "descr",
	    "Tests posix_spawn with scheduler attributes");
}
ATF_TC_BODY(t_spawnattr, tc)
{
	int pid, scheduler, child_scheduler, priority, status, pfd[2];
	char helper_arg[128];
	char * const args[] = { __UNCONST("h_spawnattr"), helper_arg, NULL };
	struct sched_param sp, child_sp;
	sigset_t sig;
	posix_spawnattr_t attr;
	char helper[FILENAME_MAX];

	/*
	 * create a pipe to control the child
	 */
	RL(pipe(pfd));
	snprintf(helper_arg, sizeof(helper_arg), "%d", pfd[0]);

	RZ(posix_spawnattr_init(&attr));

	scheduler = get_different_scheduler();
	priority = get_different_priority(scheduler);
	sp.sched_priority = priority;
	printf("using scheduler %d, priority %d\n", scheduler, priority);

	RL(sigemptyset(&sig));
	RL(sigaddset(&sig, SIGUSR1));

	RZ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSCHEDULER |
		POSIX_SPAWN_SETSCHEDPARAM | POSIX_SPAWN_SETPGROUP |
		POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF |
		POSIX_SPAWN_SETSIGDEF));
	RZ(posix_spawnattr_setpgroup(&attr, 0));
	RZ(posix_spawnattr_setschedparam(&attr, &sp));
	RZ(posix_spawnattr_setschedpolicy(&attr, scheduler));
	RZ(posix_spawnattr_setsigmask(&attr, &sig));
	RZ(posix_spawnattr_setsigdefault(&attr, &sig));

	snprintf(helper, sizeof(helper), "%s/h_spawnattr",
	    atf_tc_get_config_var(tc, "srcdir"));
	RZ(posix_spawn(&pid, helper, NULL, &attr, args, NULL));
	ATF_REQUIRE_MSG(pid > 0, "pid=%lld", (long long)pid);

	RL(child_scheduler = sched_getscheduler(pid));
	ATF_REQUIRE_MSG(scheduler == child_scheduler,
	    "scheduler = %d, child_scheduler = %d, pid %d, errno %d",
	    scheduler, child_scheduler, pid, errno);

	RL(sched_getparam(pid, &child_sp));
	ATF_REQUIRE_MSG(child_sp.sched_priority == sp.sched_priority,
	    "priority is: %d, but we requested: %d",
	    child_sp.sched_priority, sp.sched_priority);

	ATF_REQUIRE_MSG(pid == getpgid(pid), "child pid: %d, child pgid: %d",
	    pid, getpgid(pid));

	/* ready, let child go */
	RL(write(pfd[1], "q", 1));
	RL(close(pfd[0]));
	RL(close(pfd[1]));

	/* wait and check result from child */
	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);

	RZ(posix_spawnattr_destroy(&attr));
}

ATF_TC(t_spawn_resetids);

ATF_TC_HEAD(t_spawn_resetids, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn a child and with POSIX_SPAWN_RESETIDS flag");
}

ATF_TC_BODY(t_spawn_resetids, tc)
{
	char buf[FILENAME_MAX];
	char * const args[] = {
	     __UNCONST("h_spawn"), __UNCONST("--resetids"), NULL
	};
	posix_spawnattr_t attr;
	int status;
	pid_t pid;

	RZ(posix_spawnattr_init(&attr));
	RZ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_RESETIDS));

	snprintf(buf, sizeof(buf), "%s/h_spawn",
	    atf_tc_get_config_var(tc, "srcdir"));

	RZ(posix_spawn(&pid, buf, NULL, &attr, args, NULL));
	ATF_REQUIRE_MSG(pid > 0, "pid=%lld", (long long)pid);
	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG(WIFEXITED(status) && WEXITSTATUS(status) == 0,
	    "status=0x%x", status);

	RZ(posix_spawnattr_destroy(&attr));
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_spawnattr);
	ATF_TP_ADD_TC(tp, t_spawn_resetids);

	return atf_no_error();
}
