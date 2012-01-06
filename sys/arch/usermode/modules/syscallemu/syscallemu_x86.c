/* $NetBSD: syscallemu_x86.c,v 1.1 2012/01/06 13:16:20 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 Reinoud Zandijk <reinoud@NetBSD.org>
 * Copyright (c) 2012 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: syscallemu_x86.c,v 1.1 2012/01/06 13:16:20 jmcneill Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/userret.h>

#include "syscallemu.h"

/*
 * If syscallemu specific data is present for the process, verify that the
 * caller is allowed to execute system calls. If not, deliver a SIGILL to
 * the process. When syscallemu specific data is not present, simply defer
 * to the original syscall handler.
 */
static void
x86_syscall_emu(struct trapframe *frame)
{
	void (*md_syscall)(struct trapframe *) = NULL;
	struct syscallemu_data *sce;
	register_t rip_call;
	struct proc *p;
	ksiginfo_t ksi;
	lwp_t *l;

	l = curlwp;
	p = l->l_proc;

	rip_call = X86_TF_RIP(frame) - frame->tf_err;

	/* Determine if we need to emulate the system call */
	sce = syscallemu_getsce(p);
	if (sce) {
		if ((rip_call >= sce->sce_user_start &&
		     rip_call < sce->sce_user_end) ||
		    (rip_call + frame->tf_err >= sce->sce_user_start &&
		     rip_call + frame->tf_err < sce->sce_user_end)) {
			md_syscall = NULL;
		} else {
			md_syscall = sce->sce_md_syscall;
		}
	} else {
		md_syscall = p->p_md.md_syscall;
	}

	if (md_syscall == NULL) {
		/* If emulating, deliver SIGILL to process */
		X86_TF_RIP(frame) = rip_call;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLTRP;
		ksi.ksi_addr = (void *)X86_TF_RIP(frame);
		ksi.ksi_trap = 0;
		trapsignal(l, &ksi);
		userret(l);
	} else {
		/* Not emulating, so treat as a normal syscall */
		KASSERT(md_syscall != NULL);
		md_syscall(frame);
	}
}

/*
 * Set p_md.md_syscall to our syscall filter, and return a pointer to the
 * original syscall handler.
 */
void *
md_syscallemu(struct proc *p)
{
	void *osyscall;

	osyscall = p->p_md.md_syscall;
	p->p_md.md_syscall = x86_syscall_emu;

	return osyscall;
}
