/*	$NetBSD: linux_ptrace.c,v 1.2 1999/12/12 01:30:49 tron Exp $	*/

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

int
linux_sys_ptrace(p, v, retval)
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
	struct sys_ptrace_args pta;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);

	SCARG(&pta, pid) = SCARG(uap, pid);
	SCARG(&pta, addr) = (caddr_t)SCARG(uap, addr);
	SCARG(&pta, data) = SCARG(uap, data);

	switch (SCARG(uap, request)) {
	case LINUX_PTRACE_TRACEME:
		SCARG(&pta, req) = PT_TRACE_ME;
		break;
	case LINUX_PTRACE_PEEKTEXT:
		SCARG(&pta, req) = PT_READ_I;
		break;
	case LINUX_PTRACE_PEEKDATA:
		SCARG(&pta, req) = PT_READ_D;
		break;
	case LINUX_PTRACE_POKETEXT:
		SCARG(&pta, req) = PT_WRITE_I;
		break;
	case LINUX_PTRACE_POKEDATA:
		SCARG(&pta, req) = PT_WRITE_D;
		break;
	case LINUX_PTRACE_CONT:
		SCARG(&pta, req) = PT_CONTINUE;
		break;
	case LINUX_PTRACE_KILL:
		SCARG(&pta, req) = PT_KILL;
		break;
	case LINUX_PTRACE_ATTACH:
		SCARG(&pta, req) = PT_ATTACH;
		break;
	case LINUX_PTRACE_DETACH:
		SCARG(&pta, req) = PT_DETACH;
		break;
	default:
		return EIO;
	}

	return sys_ptrace(p, &pta, retval);
}
