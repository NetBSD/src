/*	$NetBSD: t_execregs.c,v 1.3 2025/02/28 16:08:42 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_execregs.c,v 1.3 2025/02/28 16:08:42 riastradh Exp $");

#include <sys/wait.h>

#include <atf-c.h>
#include <err.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_EXECREGS_TEST

#include "execregs.h"
#include "isqemu.h"
#include "h_macros.h"

static void
readregs(int rfd, register_t regs[static NEXECREGS])
{
	uint8_t *p;
	size_t n;
	ssize_t nread;

	p = (void *)regs;
	n = NEXECREGS*sizeof(regs[0]);
	while (n) {
		RL(nread = read(rfd, p, n));
		ATF_CHECK_MSG((size_t)nread <= n,
		    "overlong read: %zu > %zu", (size_t)nread, n);
		if (nread == 0)
			break;
		p += (size_t)nread;
		n -= (size_t)nread;
	}
	ATF_CHECK_EQ_MSG(n, 0,
	    "truncated read, missing %zu of %zu bytes",
	    n, NEXECREGS*sizeof(regs[0]));
}

static void
checkregs(const register_t regs[static NEXECREGS])
{
	unsigned i;

#ifdef __hppa__
	if (isQEMU()) {
		atf_tc_expect_fail("PR port-hppa/59114: hppa:"
		    " eager fpu switching for qemu and/or spectre mitigation");
	}
#endif

#if defined(__hppa__) || \
    defined(__ia64__) || \
    defined(__vax__) || \
    defined(__x86_64__)
	atf_tc_expect_fail("PR kern/59084: exec/spawn leaks register content");
#endif

	for (i = 0; i < NEXECREGS; i++) {
		if (regs[i] != 0) {
			for (i = 0; i < NEXECREGS; i++) {
				fprintf(stderr, "[%s] %"PRIxREGISTER"\n",
				    regname[i], regs[i]);
			}
			fprintf(stderr, "\n");
			atf_tc_fail("registers not zeroed");
		}
	}
}

static void
testregs(int child, const int pipefd[static 2],
    register_t regs[static NEXECREGS])
{
	int status;

	RL(close(pipefd[1]));

	readregs(pipefd[0], regs);

	RL(waitpid(child, &status, 0));
	ATF_CHECK_EQ_MSG(status, 0, "status=0x%x", status);

	checkregs(regs);
}

#endif

ATF_TC(execregszero);
ATF_TC_HEAD(execregszero, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test execve(2) zeroes registers");
}
ATF_TC_BODY(execregszero, tc)
{
#ifdef HAVE_EXECREGS_TEST
	char h_execregs[PATH_MAX];
	int pipefd[2];
	register_t regs[NEXECREGS];
	pid_t child;

	RL(snprintf(h_execregs, sizeof(h_execregs), "%s/h_execregs",
		atf_tc_get_config_var(tc, "srcdir")));

	RL(pipe(pipefd));
	memset(regs, 0x5a, sizeof(regs));

	RL(child = fork());
	if (child == 0) {
		if (dup2(pipefd[1], STDOUT_FILENO) == -1)
			err(1, "dup2");
		if (closefrom(STDERR_FILENO + 1) == -1)
			err(1, "closefrom");
		if (execregschild(h_execregs) == -1)
			err(1, "execve");
		_exit(2);
	}

	testregs(child, pipefd, regs);
#else
	atf_tc_skip("missing test for PR kern/59084:"
	    " exec/spawn leaks register content");
#endif
}

ATF_TC(spawnregszero);
ATF_TC_HEAD(spawnregszero, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test posix_spawn(2) zeroes registers");
}
ATF_TC_BODY(spawnregszero, tc)
{
#ifdef HAVE_EXECREGS_TEST
	char h_execregs[PATH_MAX];
	int pipefd[2];
	register_t regs[NEXECREGS];
	pid_t child;

	RL(snprintf(h_execregs, sizeof(h_execregs), "%s/h_execregs",
		atf_tc_get_config_var(tc, "srcdir")));

	RL(pipe(pipefd));
	memset(regs, 0x5a, sizeof(regs));

	RL(child = spawnregschild(h_execregs, pipefd[1]));

	testregs(child, pipefd, regs);
#else
	atf_tc_skip("missing test for PR kern/59084:"
	    " exec/spawn leaks register content");
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, execregszero);
	ATF_TP_ADD_TC(tp, spawnregszero);

	return atf_no_error();
}
