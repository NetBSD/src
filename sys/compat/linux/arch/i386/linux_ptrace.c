/*	$NetBSD: linux_ptrace.c,v 1.3.2.1 1999/12/27 18:34:25 wrstuden Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Scheler.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>

#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ptrace.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>

#define LINUX_PTRACE_GETREGS		12
#define LINUX_PTRACE_SETREGS		13
#define LINUX_PTRACE_GETFPREGS		14
#define LINUX_PTRACE_SETFPREGS		15

struct linux_reg {
	long ebx;
	long ecx;
	long edx;
	long esi;
	long edi;
	long ebp;
	long eax;
	int  xds;
	int  xes;
	long orig_eax;
	long eip;
	int  xcs;
	long eflags;
	long esp;
	int  xss;
};

#define ISSET(t, f)	((t) & (f))

int
linux_sys_ptrace_arch(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_ptrace_args /* {
		syscallarg(int) request;
		syscallarg(int) pid;
		syscallarg(int) addr;
		syscallarg(int) data;
	} */ *uap = v;
	int request, error;
	struct proc *t;				/* target process */
	struct reg regs;
	struct linux_reg linux_regs;

	request = SCARG(uap, request);

	if ((request != LINUX_PTRACE_GETREGS) &&
	    (request != LINUX_PTRACE_SETREGS) &&
	    (request != LINUX_PTRACE_GETFPREGS) &&
	    (request != LINUX_PTRACE_SETFPREGS))
		return EIO;

	/* Find the process we're supposed to be operating on. */
	if ((t = pfind(SCARG(uap, pid))) == NULL)
		return ESRCH;

	/*
	 * You can't do what you want to the process if:
	 *	(1) It's not being traced at all,
	 */
	if (!ISSET(t->p_flag, P_TRACED))
		return EPERM;

	/*
	 *	(2) it's being traced by procfs (which has
	 *	    different signal delivery semantics),
	 */
	if (ISSET(t->p_flag, P_FSTRACE))
		return EBUSY;

	/*
	 *	(3) it's not being traced by _you_, or
	 */
	if (t->p_pptr != p)
		return EBUSY;

	/*
	 *	(4) it's not currently stopped.
	 */
	if (t->p_stat != SSTOP || !ISSET(t->p_flag, P_WAITED))
		return EBUSY;

	*retval = 0;

	switch (request) {
	case  LINUX_PTRACE_GETREGS:
		error = process_read_regs(t, &regs);
		if (error != 0)
			return error;

		linux_regs.ebx = regs.r_ebx;
		linux_regs.ecx = regs.r_ecx;
		linux_regs.edx = regs.r_edx;
		linux_regs.esi = regs.r_esi;
		linux_regs.edi = regs.r_edi;
		linux_regs.ebp = regs.r_ebp;
		linux_regs.eax = regs.r_eax;
		linux_regs.xds = regs.r_ds;
		linux_regs.xes = regs.r_es;
		linux_regs.orig_eax = regs.r_eax; /* XXX is this correct? */
		linux_regs.eip = regs.r_eip;
		linux_regs.xcs = regs.r_cs;
		linux_regs.eflags = regs.r_eflags;
		linux_regs.esp = regs.r_esp;
		linux_regs.xss = regs.r_ss;

		return copyout(&linux_regs, (caddr_t)SCARG(uap, data),
		    sizeof(struct linux_reg));
	case  LINUX_PTRACE_SETREGS:
		error = copyin((caddr_t)SCARG(uap, data), &linux_regs,
		    sizeof(struct linux_reg));
		if (error != 0)
			return error;

		regs.r_ebx = linux_regs.ebx;
		regs.r_ecx = linux_regs.ecx;
		regs.r_edx = linux_regs.edx;
		regs.r_esi = linux_regs.esi;
		regs.r_edi = linux_regs.edi;
		regs.r_ebp = linux_regs.ebp;
		regs.r_eax = linux_regs.eax;
		regs.r_ds = linux_regs.xds;
		regs.r_es = linux_regs.xes;
		regs.r_eip = linux_regs.eip;
		regs.r_cs = linux_regs.xcs;
		regs.r_eflags = linux_regs.eflags;
		regs.r_esp = linux_regs.esp;
		regs.r_ss = linux_regs.xss;

		return process_write_regs(t, &regs);
	}

	return EIO;
}
