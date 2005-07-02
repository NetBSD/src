/*	$NetBSD: linux_syscall.c,v 1.13 2005/07/02 23:08:21 he Exp $	*/

/*-
 * Copyright (c) 2000, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ARMLinux emulation: syscall entry handling
 */

#include "opt_ktrace.h"
#include "opt_systrace.h"
#include "opt_syscall_debug.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: linux_syscall.c,v 1.13 2005/07/02 23:08:21 he Exp $");

#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/user.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <arm/swi.h>

#include <compat/linux/common/linux_errno.h>
#include <compat/linux/linux_syscall.h>

/* ARMLinux has some system calls of its very own. */
#define LINUX_ARM_NR_BASE	0x9f0000
#define LINUX_SYS_ARMBASE	0x000180 /* Must agree with syscalls.master */

void linux_syscall_intern(struct proc *);
void linux_syscall_plain(struct trapframe *, struct lwp *, u_int32_t);
void linux_syscall_fancy(struct trapframe *, struct lwp *, u_int32_t);

void
linux_syscall_intern(struct proc *p)
{
#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET)) {
		p->p_md.md_syscall = linux_syscall_fancy;
		return;
	}
#endif
#ifdef SYSTRACE
	if (p->p_flag & P_SYSTRACE) {
		p->p_md.md_syscall = linux_syscall_fancy;
		return;
	}
#endif
	p->p_md.md_syscall = linux_syscall_plain;
}

void
linux_syscall_plain(trapframe_t *frame, struct lwp *l, u_int32_t insn)
{
	const struct sysent *callp;
	struct proc *p = l->l_proc;
	int code, error;
	u_int nargs;
	register_t *args, rval[2];

	code = insn & 0x00ffffff;
	/* Remap ARM-specific syscalls onto the end of the standard range. */
	if (code > LINUX_ARM_NR_BASE)
		code = code - LINUX_ARM_NR_BASE + LINUX_SYS_ARMBASE;
	code &= LINUX_SYS_NSYSENT - 1;

	/* Linux passes all arguments in order in registers, which is nice. */
	args = &frame->tf_r0;
	callp = p->p_emul->e_sysent + code;
	nargs = callp->sy_argsize / sizeof(register_t);

	rval[0] = 0;
	rval[1] = 0;
	error = (*callp->sy_call)(l, args, rval);

	switch (error) {
	case 0:
		frame->tf_r0 = rval[0];
		break;

	case ERESTART:
		/* Reconstruct the pc to point at the swi.  */
 		frame->tf_pc -= INSN_SIZE;
		break;

	case EJUSTRETURN:
		/* nothing to do */
		break;

	default:
		error = native_to_linux_errno[error];
		frame->tf_r0 = error;
		break;
	}

	userret(l);
}

void
linux_syscall_fancy(trapframe_t *frame, struct lwp *l, u_int32_t insn)
{
	const struct sysent *callp;
	struct proc *p = l->l_proc;
	int code, error;
	u_int nargs;
	register_t *args, rval[2];

	code = insn & 0x00ffffff;
	/* Remap ARM-specific syscalls onto the end of the standard range. */
	if (code > LINUX_ARM_NR_BASE)
		code = code - LINUX_ARM_NR_BASE + LINUX_SYS_ARMBASE;
	code &= LINUX_SYS_NSYSENT - 1;

	/* Linux passes all arguments in order in registers, which is nice. */
	args = &frame->tf_r0;
	callp = p->p_emul->e_sysent + code;
	nargs = callp->sy_argsize / sizeof(register_t);

	if ((error = trace_enter(l, code, code, NULL, args)) != 0)
		goto out;

	rval[0] = 0;
	rval[1] = 0;
	error = (*callp->sy_call)(l, args, rval);
out:
	switch (error) {
	case 0:
		frame->tf_r0 = rval[0];
		break;

	case ERESTART:
		/* Reconstruct the pc to point at the swi.  */
 		frame->tf_pc -= INSN_SIZE;
		break;

	case EJUSTRETURN:
		/* nothing to do */
		break;

	default:
		error = native_to_linux_errno[error];
		frame->tf_r0 = error;
		break;
	}

	trace_exit(l, code, args, rval, error);

	userret(l);
}
