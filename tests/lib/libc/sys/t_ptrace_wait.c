/*	$NetBSD: t_ptrace_wait.c,v 1.188 2020/05/05 00:50:39 kamil Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_ptrace_wait.c,v 1.188 2020/05/05 00:50:39 kamil Exp $");

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

static void
ptrace_siginfo(bool faked, void (*sah)(int a, siginfo_t *b, void *c), int *signal_caught)
{
	const int exitval = 5;
	const int sigval = SIGINT;
	const int sigfaked = SIGTRAP;
	const int sicodefaked = TRAP_BRKPT;
	pid_t child, wpid;
	struct sigaction sa;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	struct ptrace_siginfo info;
	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		sa.sa_sigaction = sah;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		FORKEE_ASSERT(sigaction(faked ? sigfaked : sigval, &sa, NULL)
		    != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		FORKEE_ASSERT_EQ(*signal_caught, 1);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_GET_SIGINFO, child, &info, sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x si_errno=%#x\n",
	    info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
	    info.psi_siginfo.si_errno);

	if (faked) {
		DPRINTF("Before setting new faked signal to signo=%d "
		    "si_code=%d\n", sigfaked, sicodefaked);
		info.psi_siginfo.si_signo = sigfaked;
		info.psi_siginfo.si_code = sicodefaked;
	}

	DPRINTF("Before calling ptrace(2) with PT_SET_SIGINFO for child\n");
	SYSCALL_REQUIRE(
	    ptrace(PT_SET_SIGINFO, child, &info, sizeof(info)) != -1);

	if (faked) {
		DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
		    "child\n");
		SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info,
		    sizeof(info)) != -1);

		DPRINTF("Before checking siginfo_t\n");
		ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigfaked);
		ATF_REQUIRE_EQ(info.psi_siginfo.si_code, sicodefaked);
	}

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1,
	    faked ? sigfaked : sigval) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define PTRACE_SIGINFO(test, faked)					\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify basic PT_GET_SIGINFO and PT_SET_SIGINFO calls"	\
	    "with%s setting signal to new value", faked ? "" : "out");	\
}									\
									\
static int test##_caught = 0;						\
									\
static void								\
test##_sighandler(int sig, siginfo_t *info, void *ctx)			\
{									\
	if (faked) {							\
		FORKEE_ASSERT_EQ(sig, SIGTRAP);				\
		FORKEE_ASSERT_EQ(info->si_signo, SIGTRAP);		\
		FORKEE_ASSERT_EQ(info->si_code, TRAP_BRKPT);		\
	} else {							\
		FORKEE_ASSERT_EQ(sig, SIGINT);				\
		FORKEE_ASSERT_EQ(info->si_signo, SIGINT);		\
		FORKEE_ASSERT_EQ(info->si_code, SI_LWP);		\
	}								\
									\
	++ test##_caught;						\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	ptrace_siginfo(faked, test##_sighandler, & test##_caught); 	\
}

PTRACE_SIGINFO(siginfo_set_unmodified, false)
PTRACE_SIGINFO(siginfo_set_faked, true)

/// ----------------------------------------------------------------------------

static void
user_va0_disable(int operation)
{
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	const int sigval = SIGSTOP;
	int rv;

	struct ptrace_siginfo info;

	if (get_user_va0_disable() == 0)
		atf_tc_skip("vm.user_va0_disable is set to 0");

	memset(&info, 0, sizeof(info));

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		/* NOTREACHED */
		FORKEE_ASSERTX(0 && "This shall not be reached");
		__unreachable();
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for "
		"child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info,
		sizeof(info)) != -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		"si_errno=%#x\n",
		info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	DPRINTF("Before resuming the child process in PC=0x0 "
	    "and without signal to be sent\n");
	errno = 0;
	rv = ptrace(operation, child, (void *)0, 0);
	ATF_REQUIRE_EQ(errno, EINVAL);
	ATF_REQUIRE_EQ(rv, -1);

	SYSCALL_REQUIRE(ptrace(PT_KILL, child, NULL, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);
	validate_status_signaled(status, SIGKILL, 0);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define USER_VA0_DISABLE(test, operation)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify behavior of " #operation " with PC set to 0x0");	\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	user_va0_disable(operation);					\
}

USER_VA0_DISABLE(user_va0_disable_pt_continue, PT_CONTINUE)
USER_VA0_DISABLE(user_va0_disable_pt_syscall, PT_SYSCALL)
USER_VA0_DISABLE(user_va0_disable_pt_detach, PT_DETACH)

/// ----------------------------------------------------------------------------

/*
 * Parse the core file and find the requested note.  If the reading or parsing
 * fails, the test is failed.  If the note is found, it is read onto buf, up to
 * buf_len.  The actual length of the note is returned (which can be greater
 * than buf_len, indicating that it has been truncated).  If the note is not
 * found, -1 is returned.
 *
 * If the note_name ends in '*', then we find the first note that matches
 * the note_name prefix up to the '*' character, e.g.:
 *
 *	NetBSD-CORE@*
 *
 * finds the first note whose name prefix matches "NetBSD-CORE@".
 */
static ssize_t core_find_note(const char *core_path,
    const char *note_name, uint64_t note_type, void *buf, size_t buf_len)
{
	int core_fd;
	Elf *core_elf;
	size_t core_numhdr, i;
	ssize_t ret = -1;
	size_t name_len = strlen(note_name);
	bool prefix_match = false;

	if (note_name[name_len - 1] == '*') {
		prefix_match = true;
		name_len--;
	} else {
		/* note: we assume note name will be null-terminated */
		name_len++;
	}

	SYSCALL_REQUIRE((core_fd = open(core_path, O_RDONLY)) != -1);
	SYSCALL_REQUIRE(elf_version(EV_CURRENT) != EV_NONE);
	SYSCALL_REQUIRE((core_elf = elf_begin(core_fd, ELF_C_READ, NULL)));

	SYSCALL_REQUIRE(elf_getphnum(core_elf, &core_numhdr) != 0);
	for (i = 0; i < core_numhdr && ret == -1; i++) {
		GElf_Phdr core_hdr;
		size_t offset;
		SYSCALL_REQUIRE(gelf_getphdr(core_elf, i, &core_hdr));
		if (core_hdr.p_type != PT_NOTE)
		    continue;

		for (offset = core_hdr.p_offset;
		    offset < core_hdr.p_offset + core_hdr.p_filesz;) {
			Elf64_Nhdr note_hdr;
			char name_buf[64];

			switch (gelf_getclass(core_elf)) {
			case ELFCLASS64:
				SYSCALL_REQUIRE(pread(core_fd, &note_hdr,
				    sizeof(note_hdr), offset)
				    == sizeof(note_hdr));
				offset += sizeof(note_hdr);
				break;
			case ELFCLASS32:
				{
				Elf32_Nhdr tmp_hdr;
				SYSCALL_REQUIRE(pread(core_fd, &tmp_hdr,
				    sizeof(tmp_hdr), offset)
				    == sizeof(tmp_hdr));
				offset += sizeof(tmp_hdr);
				note_hdr.n_namesz = tmp_hdr.n_namesz;
				note_hdr.n_descsz = tmp_hdr.n_descsz;
				note_hdr.n_type = tmp_hdr.n_type;
				}
				break;
			}

			/* indicates end of notes */
			if (note_hdr.n_namesz == 0 || note_hdr.n_descsz == 0)
				break;
			if (((prefix_match &&
			      note_hdr.n_namesz > name_len) ||
			     (!prefix_match &&
			      note_hdr.n_namesz == name_len)) &&
			    note_hdr.n_namesz <= sizeof(name_buf)) {
				SYSCALL_REQUIRE(pread(core_fd, name_buf,
				    note_hdr.n_namesz, offset)
				    == (ssize_t)(size_t)note_hdr.n_namesz);

				if (!strncmp(note_name, name_buf, name_len) &&
				    note_hdr.n_type == note_type)
					ret = note_hdr.n_descsz;
			}

			offset += note_hdr.n_namesz;
			/* fix to alignment */
			offset = roundup(offset, core_hdr.p_align);

			/* if name & type matched above */
			if (ret != -1) {
				ssize_t read_len = MIN(buf_len,
				    note_hdr.n_descsz);
				SYSCALL_REQUIRE(pread(core_fd, buf,
				    read_len, offset) == read_len);
				break;
			}

			offset += note_hdr.n_descsz;
			/* fix to alignment */
			offset = roundup(offset, core_hdr.p_align);
		}
	}

	elf_end(core_elf);
	close(core_fd);

	return ret;
}

ATF_TC(core_dump_procinfo);
ATF_TC_HEAD(core_dump_procinfo, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Trigger a core dump and verify its contents.");
}

ATF_TC_BODY(core_dump_procinfo, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	char core_path[] = "/tmp/core.XXXXXX";
	int core_fd;
	struct netbsd_elfcore_procinfo procinfo;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before triggering SIGTRAP\n");
		trigger_trap();

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	SYSCALL_REQUIRE((core_fd = mkstemp(core_path)) != -1);
	close(core_fd);

	DPRINTF("Call DUMPCORE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_DUMPCORE, child, core_path, strlen(core_path))
	    != -1);

	DPRINTF("Read core file\n");
	ATF_REQUIRE_EQ(core_find_note(core_path, "NetBSD-CORE",
	    ELF_NOTE_NETBSD_CORE_PROCINFO, &procinfo, sizeof(procinfo)),
	    sizeof(procinfo));

	ATF_CHECK_EQ(procinfo.cpi_version, 1);
	ATF_CHECK_EQ(procinfo.cpi_cpisize, sizeof(procinfo));
	ATF_CHECK_EQ(procinfo.cpi_signo, SIGTRAP);
	ATF_CHECK_EQ(procinfo.cpi_pid, child);
	ATF_CHECK_EQ(procinfo.cpi_ppid, getpid());
	ATF_CHECK_EQ(procinfo.cpi_pgrp, getpgid(child));
	ATF_CHECK_EQ(procinfo.cpi_sid, getsid(child));
	ATF_CHECK_EQ(procinfo.cpi_ruid, getuid());
	ATF_CHECK_EQ(procinfo.cpi_euid, geteuid());
	ATF_CHECK_EQ(procinfo.cpi_rgid, getgid());
	ATF_CHECK_EQ(procinfo.cpi_egid, getegid());
	ATF_CHECK_EQ(procinfo.cpi_nlwps, 1);
	ATF_CHECK(procinfo.cpi_siglwp > 0);

	unlink(core_path);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

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
	ATF_TP_ADD_TC(tp, siginfo_set_unmodified);
	ATF_TP_ADD_TC(tp, siginfo_set_faked);

	ATF_TP_ADD_TC(tp, user_va0_disable_pt_continue);
	ATF_TP_ADD_TC(tp, user_va0_disable_pt_syscall);
	ATF_TP_ADD_TC(tp, user_va0_disable_pt_detach);

	ATF_TP_ADD_TC(tp, core_dump_procinfo);

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

	ATF_TP_ADD_TCS_PTRACE_WAIT_AMD64();
	ATF_TP_ADD_TCS_PTRACE_WAIT_I386();
	ATF_TP_ADD_TCS_PTRACE_WAIT_X86();

#else
	ATF_TP_ADD_TC(tp, dummy);
#endif

	return atf_no_error();
}
