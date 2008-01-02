/*	$NetBSD: linux_ptrace.c,v 1.10.4.1 2008/01/02 21:52:17 bouyer Exp $	*/

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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_ptrace.c,v 1.10.4.1 2008/01/02 21:52:17 bouyer Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>
#include <uvm/uvm_extern.h>

#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ptrace.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_emuldata.h>
#include <compat/linux/common/linux_exec.h>	/* for emul_linux */

#include <compat/linux/linux_syscallargs.h>

#include <lib/libkern/libkern.h>	/* for offsetof() */

/*
 * On ARMv2, uregs contains R0--R15, orig_R0.
 * On ARMv3 and later, it's R0--R15, CPSR, orig_R0.
 * As far as I can see, Linux doesn't initialise orig_R0 on ARMv2, so we
 * just produce the ARMv3 version.
 */

struct linux_reg {
	long uregs[18];
};

#define LINUX_REG_R0	0
#define LINUX_REG_R1	1
#define LINUX_REG_R2	2
#define LINUX_REG_R3	3
#define LINUX_REG_R4	4
#define LINUX_REG_R5	5
#define LINUX_REG_R6	6
#define LINUX_REG_R7	7
#define LINUX_REG_R8	8
#define LINUX_REG_R9	9
#define LINUX_REG_R10	10
#define LINUX_REG_FP	11
#define LINUX_REG_IP	12
#define LINUX_REG_SP	13
#define LINUX_REG_LR	14
#define LINUX_REG_PC	15
#define LINUX_REG_CPSR	16
#define LINUX_REG_ORIG_R0 17

int
linux_sys_ptrace_arch(struct lwp *l, const struct linux_sys_ptrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) request;
		syscallarg(int) pid;
		syscallarg(int) addr;
		syscallarg(int) data;
	} */
	struct proc *p = l->l_proc;
	int request, error;
	struct proc *t;				/* target process */
	struct lwp *lt;
	struct reg *regs = NULL;
	struct fpreg *fpregs = NULL;
	struct linux_reg *linux_regs = NULL;
	struct linux_fpreg *linux_fpregs = NULL;

	request = SCARG(uap, request);

	if ((request != LINUX_PTRACE_GETREGS) &&
	    (request != LINUX_PTRACE_SETREGS))
		return EIO;

	/* Find the process we're supposed to be operating on. */
	if ((t = pfind(SCARG(uap, pid))) == NULL)
		return ESRCH;

	/*
	 * You can't do what you want to the process if:
	 *	(1) It's not being traced at all,
	 */
	if (!ISSET(t->p_slflag, PSL_TRACED))
		return EPERM;

	/*
	 *	(2) it's being traced by procfs (which has
	 *	    different signal delivery semantics),
	 */
	if (ISSET(t->p_slflag, PSL_FSTRACE))
		return EBUSY;

	/*
	 *	(3) it's not being traced by _you_, or
	 */
	if (t->p_pptr != p)
		return EBUSY;

	/*
	 *	(4) it's not currently stopped.
	 */
	if (t->p_stat != SSTOP || !t->p_waited)
		return EBUSY;

	/* XXX NJWLWP
	 * The entire ptrace interface needs work to be useful to
	 * a process with multiple LWPs. For the moment, we'll
	 * just kluge this and fail on others.
	 */

	if (p->p_nlwps > 1)
		return (ENOSYS);

	lt = LIST_FIRST(&t->p_lwps);

	*retval = 0;

	switch (request) {
	case  LINUX_PTRACE_GETREGS:
		MALLOC(regs, struct reg*, sizeof(struct reg), M_TEMP, M_WAITOK);
		MALLOC(linux_regs, struct linux_reg*, sizeof(struct linux_reg),
			M_TEMP, M_WAITOK);

		error = process_read_regs(lt, regs);
		if (error != 0)
			goto out;

		memcpy(linux_regs->uregs, regs->r, 13 * sizeof(register_t));
		linux_regs->uregs[LINUX_REG_SP] = regs->r_sp;
		linux_regs->uregs[LINUX_REG_LR] = regs->r_lr;
		linux_regs->uregs[LINUX_REG_PC] = regs->r_pc;
		linux_regs->uregs[LINUX_REG_CPSR] = regs->r_cpsr;
		linux_regs->uregs[LINUX_REG_ORIG_R0] = regs->r[0];

		error = copyout(linux_regs, (void *)SCARG(uap, data),
		    sizeof(struct linux_reg));
		goto out;

	case  LINUX_PTRACE_SETREGS:
		MALLOC(regs, struct reg*, sizeof(struct reg), M_TEMP, M_WAITOK);
		MALLOC(linux_regs, struct linux_reg *, sizeof(struct linux_reg),
			M_TEMP, M_WAITOK);

		error = copyin((void *)SCARG(uap, data), linux_regs,
		    sizeof(struct linux_reg));
		if (error != 0)
			goto out;

		memcpy(regs->r, linux_regs->uregs, 13 * sizeof(register_t));
		regs->r_sp = linux_regs->uregs[LINUX_REG_SP];
		regs->r_lr = linux_regs->uregs[LINUX_REG_LR];
		regs->r_pc = linux_regs->uregs[LINUX_REG_PC];
		regs->r_cpsr = linux_regs->uregs[LINUX_REG_CPSR];

		error = process_write_regs(lt, regs);
		goto out;

	default:
		/* never reached */
		break;
	}

	return EIO;

    out:
	if (regs)
		FREE(regs, M_TEMP);
	if (fpregs)
		FREE(fpregs, M_TEMP);
	if (linux_regs)
		FREE(linux_regs, M_TEMP);
	if (linux_fpregs)
		FREE(linux_fpregs, M_TEMP);
	return (error);

}
