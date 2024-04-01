/*	$NetBSD: t_ptrace_wait.c,v 1.192 2024/04/01 18:33:23 riastradh Exp $	*/

/*-
 * Copyright (c) 2016, 2017, 2018, 2019, 2020 The NetBSD Foundation, Inc.
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

/*
 * XXX Hack: Force the use of sys/exec_elf.h, not elfdefinitions.h.
 * Why?
 *
 * - libelf.h and gelf.h are needed for parsing core files in
 *   t_ptrace_core_wait.h.
 *
 * - sys/exec_elf.h is needed for struct netbsd_elfcore_procinfo also
 *   in t_ptrace_core_wait.h.
 *
 * libelf.h and gelf.h pull in elfdefinitions.h, but that conflicts
 * with sys/exec_elf.h on most basic ELF definitions.
 */
#define	_SYS_ELFDEFINITIONS_H_

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.192 2024/04/01 18:33:23 riastradh Exp $");

#define __LEGACY_PT_LWPINFO

#include <sys/param.h>
#include <sys/types.h>
#include <sys/exec_elf.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <machine/reg.h>
#include <assert.h>
#include <elf.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <lwp.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#if defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#include <x86/cpu_extended_state.h>
#include <x86/specialreg.h>
#endif

#include <libelf.h>
#include <gelf.h>

#include <atf-c.h>

#ifdef ENABLE_TESTS

/* Assumptions in the kernel code that must be kept. */
__CTASSERT(sizeof(((struct ptrace_state *)0)->pe_report_event) ==
    sizeof(((siginfo_t *)0)->si_pe_report_event));
__CTASSERT(sizeof(((struct ptrace_state *)0)->pe_other_pid) ==
    sizeof(((siginfo_t *)0)->si_pe_other_pid));
__CTASSERT(sizeof(((struct ptrace_state *)0)->pe_lwp) ==
    sizeof(((siginfo_t *)0)->si_pe_lwp));
__CTASSERT(sizeof(((struct ptrace_state *)0)->pe_other_pid) ==
    sizeof(((struct ptrace_state *)0)->pe_lwp));

#include "h_macros.h"

#include "t_ptrace_wait.h"
#include "msg.h"

#define SYSCALL_REQUIRE(expr) ATF_REQUIRE_MSG(expr, "%s: %s", # expr, \
    strerror(errno))
#define SYSCALL_REQUIRE_ERRNO(res, exp) ATF_REQUIRE_MSG(res == exp, \
    "%d(%s) != %d", res, strerror(res), exp)

static int debug = 0;

#define DPRINTF(a, ...)	do  \
	if (debug) \
	printf("%s() %d.%d %s:%d " a, \
	__func__, getpid(), _lwp_self(), __FILE__, __LINE__,  ##__VA_ARGS__); \
    while (/*CONSTCOND*/0)

/// ----------------------------------------------------------------------------

#include "t_ptrace_register_wait.h"
#include "t_ptrace_syscall_wait.h"
#include "t_ptrace_step_wait.h"
#include "t_ptrace_kill_wait.h"
#include "t_ptrace_bytetransfer_wait.h"
#include "t_ptrace_clone_wait.h"
#include "t_ptrace_fork_wait.h"
#include "t_ptrace_signal_wait.h"
#include "t_ptrace_eventmask_wait.h"
#include "t_ptrace_lwp_wait.h"
#include "t_ptrace_exec_wait.h"
#include "t_ptrace_topology_wait.h"
#include "t_ptrace_threads_wait.h"
#include "t_ptrace_siginfo_wait.h"
#include "t_ptrace_core_wait.h"
#include "t_ptrace_misc_wait.h"

/// ----------------------------------------------------------------------------

#include "t_ptrace_amd64_wait.h"
#include "t_ptrace_i386_wait.h"
#include "t_ptrace_x86_wait.h"

/// ----------------------------------------------------------------------------

#else
ATF_TC(dummy);
ATF_TC_HEAD(dummy, tc)
{
	atf_tc_set_md_var(tc, "descr", "A dummy test");
}

ATF_TC_BODY(dummy, tc)
{

	// Dummy, skipped
	// The ATF framework requires at least a single defined test.
}
#endif

ATF_TP_ADD_TCS(tp)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

#ifdef ENABLE_TESTS
	ATF_TP_ADD_TCS_PTRACE_WAIT_REGISTER();
	ATF_TP_ADD_TCS_PTRACE_WAIT_SYSCALL();
	ATF_TP_ADD_TCS_PTRACE_WAIT_STEP();
	ATF_TP_ADD_TCS_PTRACE_WAIT_KILL();
	ATF_TP_ADD_TCS_PTRACE_WAIT_BYTETRANSFER();
	ATF_TP_ADD_TCS_PTRACE_WAIT_CLONE();
	ATF_TP_ADD_TCS_PTRACE_WAIT_FORK();
	ATF_TP_ADD_TCS_PTRACE_WAIT_SIGNAL();
	ATF_TP_ADD_TCS_PTRACE_WAIT_EVENTMASK();
	ATF_TP_ADD_TCS_PTRACE_WAIT_LWP();
	ATF_TP_ADD_TCS_PTRACE_WAIT_EXEC();
	ATF_TP_ADD_TCS_PTRACE_WAIT_TOPOLOGY();
	ATF_TP_ADD_TCS_PTRACE_WAIT_THREADS();
	ATF_TP_ADD_TCS_PTRACE_WAIT_SIGINFO();
	ATF_TP_ADD_TCS_PTRACE_WAIT_CORE();
	ATF_TP_ADD_TCS_PTRACE_WAIT_MISC();

	ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64();
	ATF_TP_ADD_TCS_PTRACE_WAIT_I386();
	ATF_TP_ADD_TCS_PTRACE_WAIT_X86();

#else
	ATF_TP_ADD_TC(tp, dummy);
#endif

	return atf_no_error();
}
