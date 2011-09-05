/* $NetBSD: urkelvisor.c,v 1.7 2011/09/05 20:54:48 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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
 * Usermode kernel supervisor
 */

#include <sys/cdefs.h>
#ifdef __NetBSD__
__RCSID("$NetBSD: urkelvisor.c,v 1.7 2011/09/05 20:54:48 jmcneill Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#ifdef __linux__
#include <sys/user.h>
#else
#include <machine/reg.h>
#endif

#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "../include/urkelvisor.h"

#if defined(__linux__)
#include <linux/ptrace.h>
#define PT_SYSCALLEMU PTRACE_SYSEMU
typedef intptr_t vaddr_t;
#endif

extern vaddr_t kmem_user_start, kmem_user_end;	/* usermode/pmap.c */

#if defined(__linux__)
# define reg_struct		user_regs_struct
# define R_SYSCALL(_regs)	((_regs)->orig_eax)
# define R_PC(_regs)		((_regs)->eip)
#else
# define reg_struct		reg
# if defined(__i386__)
#  define R_SYSCALL(_regs)	((_regs)->r_eax)
#  define R_PC(_regs)		((_regs)->r_eip)
# elif defined(__x86_64__)
#  define R_SYSCALL(_regs)	((_regs)->regs[_REG_RAX])
#  define R_PC(_regs)		((_regs)->regs[_REG_RIP])
# else
#  error port me
# endif
#endif

static int
wait_urkel(pid_t urkel_pid)
{
	pid_t pid;
	int status;

	pid = waitpid(urkel_pid, &status, 0);
	if (pid == -1)
		err(EXIT_FAILURE, "waitpid failed");

	if (WIFEXITED(status))
		exit(WEXITSTATUS(status));

	return status;
}

static void
ptrace_getregs(pid_t urkel_pid, struct reg_struct *puregs)
{
	errno = 0;
#ifdef __linux__
	ptrace(PT_GETREGS, urkel_pid, NULL, puregs);
#else
	ptrace(PT_GETREGS, urkel_pid, puregs, 0);
#endif
	if (errno)
		err(EXIT_FAILURE, "ptrace(PT_GETREGS, %d, &uregs, 0) failed",
		    urkel_pid);
}

static int
handle_syscall(struct reg_struct *puregs, pid_t urkel_pid)
{
	extern int cpu_lwp_inkernel;
	int sig = 0;
	int inkernel;

	errno = 0;
	inkernel = ptrace(PT_READ_D, urkel_pid, &cpu_lwp_inkernel, 0);
	if (errno)
		err(EXIT_FAILURE, "ptrace(PT_READ_D, %d, %p, 0) failed",
		    urkel_pid, &cpu_lwp_inkernel);

	//fprintf(stderr, "%s: pid=%d pc=%p inkernel=%d\n",
	//    __func__, urkel_pid, (void *)R_PC(puregs), inkernel);

	if (!inkernel) {
		fprintf(stderr, "caught syscall %d\n", (int)R_SYSCALL(puregs));
		errno = 0;
		ptrace(PT_SYSCALLEMU, urkel_pid, NULL, 0);
		if (errno)
			err(EXIT_FAILURE,
			    "ptrace(PT_SYSCALLEMU, %d, NULL, 0) failed",
			    urkel_pid);
		sig = SIGILL; 
	}

	return sig;
}

static int
urkelvisor(pid_t urkel_pid)
{
	struct reg_struct uregs;
	int status, insyscall, sig;

	insyscall = 0;
	sig = 0;

	status = wait_urkel(urkel_pid);

	for (;;) {
		errno = 0;
		//fprintf(stderr, "sig = %d\n", sig);
		ptrace(PT_SYSCALL, urkel_pid, (void *)1, sig);
		if (errno)
			err(EXIT_FAILURE, "ptrace(PT_SYSCALL, %d, 1, %d) failed",
			    urkel_pid, sig);
		sig = 0;
		status = wait_urkel(urkel_pid);

		//fprintf(stderr, "syscall insyscall=%d status=%x\n",
		//    insyscall, status);
		if (WIFSTOPPED(status)) {
			if (WSTOPSIG(status) == SIGTRAP) {
				insyscall = !insyscall;
				if (insyscall) {
					ptrace_getregs(urkel_pid, &uregs);
					sig = handle_syscall(&uregs, urkel_pid);
					if (sig)
						insyscall = !insyscall;
				}
			} else {
				sig = WSTOPSIG(status);
			}
		}
	}
}

void
urkelvisor_init(void)
{
	char *enable;
	pid_t child_pid;
	int status;

	/* env URKELVISOR=0 disables the urkelvisor */
	enable = getenv("URKELVISOR");
	if (enable && *enable == '0') {
		fprintf(stderr, "%s: disabled\n", __func__);
		return;
	}

	child_pid = fork();
	switch (child_pid) {
	case -1:
		err(EXIT_FAILURE, "fork() failed");
		/* NOTREACHED */
	case 0:
		errno = 0;
		ptrace(PT_TRACE_ME, 0, NULL, 0);
		if (errno)
			err(EXIT_FAILURE,
			    "ptrace(PT_TRACE_ME, 0, NULL, 0) failed");
		raise(SIGSTOP);
		break;
	default:
		status = urkelvisor(child_pid);
		exit(status);
	}
}

