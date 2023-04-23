/*	$NetBSD: t_open_pr_57260.c,v 1.2 2023/04/23 00:46:46 gutteridge Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell and David H. Gutteridge.
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
__RCSID("$NetBSD: t_open_pr_57260.c,v 1.2 2023/04/23 00:46:46 gutteridge Exp $");

#include <sys/stat.h>

#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#include <atf-c.h>
#include "h_macros.h"

static jmp_buf env;
static sig_atomic_t alarmed = 0;

static void
on_alarm(int sig)
{

	if (!alarmed) {
		alarmed = 1;
		alarm(1);
	} else {
		longjmp(env, 1);
	}
}

ATF_TC(openrestartsignal);
ATF_TC_HEAD(openrestartsignal, tc)
{

	atf_tc_set_md_var(tc, "descr", "open(2) should restart on signal "
	    "(PR kern/57260)");
}

ATF_TC_BODY(openrestartsignal, tc)
{
	struct sigaction sa;
	int fd;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_alarm;
	(void)sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sa, NULL) == -1)
		atf_tc_fail_errno("sigaction");

	if (mkfifo("fifo", 0600) == -1)
		atf_tc_fail_errno("mkfifo");
	alarm(1);
	if (setjmp(env))
		/* second signal handler longjmped out */
		return;
	fd = open("fifo", O_RDONLY);
	if (fd == -1)
		atf_tc_fail_errno("open");
	atf_tc_fail("open returned %d", fd);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, openrestartsignal);

	return atf_no_error();
}
