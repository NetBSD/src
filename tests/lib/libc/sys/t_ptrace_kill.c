/*	$NetBSD: t_ptrace_kill.c,v 1.2 2025/05/02 02:24:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: t_ptrace_kill.c,v 1.2 2025/05/02 02:24:32 riastradh Exp $");

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <atf-c.h>

#define SYSCALL(a, b) ATF_REQUIRE_EQ_MSG(a, b, "%s got %s", #a, strerror(errno))

ATF_TC(pt_kill);
ATF_TC_HEAD(pt_kill, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test PT_KILL of a PT_STOP'ed process");
}

static void
child(int *fdto, int *fdfrom)
{
	char p = '2', q;
	printf("%d: born\n", getpid());
	write(fdfrom[1], &p, 1);
	read(fdto[0], &q, 1);
	printf("%d: seppuku %c\n", getpid(), q);
	write(fdfrom[1], &p, 1);
	read(fdto[0], &q, 1);
//	*(int *)1 = 0;
//	kill(getpid(), SIGSEGV);
//	kill(getpid(), SIGSTOP);
	for (;;)
		sleep(1);

}

static void *
waitthread(void *pidp)
{
	int status = 0;
	pid_t rpid, pid;

	pid = *(pid_t *)pidp;
	printf("waiting for %d\n", pid);
	while ((rpid = waitpid(pid, &status, 0)) != pid) {
		printf("waitpid %d = %d status = %#x", pid, rpid, status);
	}
	printf("done waitpid %d = %d status = %#x", pid, rpid, status);
	return NULL;
}

ATF_TC_BODY(pt_kill, tc)
{
	pid_t pid;
	int fdto[2], fdfrom[2];
	char p = '1', q;
	int status;
	pthread_t thread;

	SYSCALL(pipe(fdto), 0);
	SYSCALL(pipe(fdfrom), 0);
	switch (pid = fork()) {
	case 0:
		child(fdto, fdfrom);
		break;
	case -1:
		err(EXIT_FAILURE, "fork failed");
	default:
		SYSCALL(pthread_create(&thread, NULL, waitthread, &pid), 0);
		sleep(1); // XXX: too lazy to sync properly
		SYSCALL(read(fdfrom[0], &q, 1), 1);
		printf("%d: read %c\n", pid, q);
		SYSCALL(ptrace(PT_ATTACH, pid, NULL, 0), 0);
		printf("%d: attached\n", pid);
		SYSCALL(write(fdto[1], &p, 1), 1);
		waitpid(pid, NULL, WNOHANG);
		printf("%d: sent\n", pid);
		SYSCALL(ptrace(PT_CONTINUE, pid, (void *)1, 0), 0);
		SYSCALL(read(fdfrom[0], &p, 1), 1);
		printf("%d: received\n", pid);
		SYSCALL(ptrace(PT_STOP, pid, NULL, 0), 0);
		SYSCALL(ptrace(PT_KILL, pid, NULL, 0), 0);
		SYSCALL(waitpid(pid, &status, 0), pid);
		ATF_REQUIRE(status == 9);
		break;
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, pt_kill);

	return atf_no_error();
}
