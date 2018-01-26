/*	$NetBSD: linux_ptrace.c,v 1.21 2018/01/26 09:29:15 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_ptrace.c,v 1.21 2018/01/26 09:29:15 christos Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>
#include <uvm/uvm_extern.h>

#include <machine/reg.h>
#include <machine/pcb.h>

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

int linux_ptrace_disabled = 1;	/* bitrotted */

int
linux_sys_ptrace_arch(struct lwp *l, const struct linux_sys_ptrace_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) request;
		syscallarg(int) pid;
		syscallarg(int) addr;
		syscallarg(int) data;
	} */
	struct proc *p = l->l_proc, *t;
	struct lwp *lt;
#ifdef _ARM_ARCH_6
	struct pcb *pcb;
	void *val;
#endif
	struct reg *regs = NULL;
	struct linux_reg *linux_regs = NULL;
	int request, error;

	if (linux_ptrace_disabled)
		return ENOSYS;

	error = 0;
	request = SCARG(uap, request);
	regs = kmem_alloc(sizeof(struct reg), KM_SLEEP);
	linux_regs = kmem_alloc(sizeof(struct linux_reg), KM_SLEEP);

	switch (request) {
	case LINUX_PTRACE_GETREGS:
		break;
	case LINUX_PTRACE_SETREGS:
		error = copyin((void *)SCARG(uap, data), linux_regs,
		    sizeof(struct linux_reg));
		if (error) {
			goto out;
		}
		break;
	default:
		error = EIO;
		goto out;
	}

	/* Find the process we are supposed to be operating on. */
	mutex_enter(proc_lock);
	if ((t = proc_find(SCARG(uap, pid))) == NULL) {
		mutex_exit(proc_lock);
		error = ESRCH;
		goto out;
	}
	mutex_enter(t->p_lock);

	/*
	 * You cannot do what you want to the process if:
	 * 1. It is not being traced at all,
	 */
	if (!ISSET(t->p_slflag, PSL_TRACED)) {
		mutex_exit(t->p_lock);
		mutex_exit(proc_lock);
		error = EPERM;
		goto out;
	}
	/*
	 * 2. It is being traced by procfs (which has different signal
	 *    delivery semantics),
	 * 3. It is not being traced by _you_, or
	 * 4. It is not currently stopped.
	 */
	if (t->p_pptr != p || t->p_stat != SSTOP || !t->p_waited) {
		mutex_exit(t->p_lock);
		mutex_exit(proc_lock);
		error = EBUSY;
		goto out;
	}
	mutex_exit(proc_lock);
	/* XXX: ptrace needs revamp for multi-threading support. */
	if (t->p_nlwps > 1) {
		mutex_exit(t->p_lock);
		error = ENOSYS;
		goto out;
	}
	lt = LIST_FIRST(&t->p_lwps);
	*retval = 0;

	switch (request) {
	case LINUX_PTRACE_GETREGS:
		error = process_read_regs(lt, regs);
		mutex_exit(t->p_lock);
		if (error) {
			break;
		}
		memcpy(linux_regs->uregs, regs->r, 13 * sizeof(register_t));
		linux_regs->uregs[LINUX_REG_SP] = regs->r_sp;
		linux_regs->uregs[LINUX_REG_LR] = regs->r_lr;
		linux_regs->uregs[LINUX_REG_PC] = regs->r_pc;
		linux_regs->uregs[LINUX_REG_CPSR] = regs->r_cpsr;
		linux_regs->uregs[LINUX_REG_ORIG_R0] = regs->r[0];

		error = copyout(linux_regs, (void *)SCARG(uap, data),
		    sizeof(struct linux_reg));
		break;

	case LINUX_PTRACE_SETREGS:
		memcpy(regs->r, linux_regs->uregs, 13 * sizeof(register_t));
		regs->r_sp = linux_regs->uregs[LINUX_REG_SP];
		regs->r_lr = linux_regs->uregs[LINUX_REG_LR];
		regs->r_pc = linux_regs->uregs[LINUX_REG_PC];
		regs->r_cpsr = linux_regs->uregs[LINUX_REG_CPSR];
		error = process_write_regs(lt, regs);
		mutex_exit(t->p_lock);
		break;

#ifdef _ARM_ARCH_6
#define LINUX_PTRACE_GET_THREAD_AREA	22
	case LINUX_PTRACE_GET_THREAD_AREA:
		mutex_exit(t->p_lock);
		pcb = lwp_getpcb(l);
		val = (void *)pcb->pcb_user_pid_ro;
		error = copyout(&val, (void *)SCARG(uap, data), sizeof(val));
		break;
#endif

	default:
		mutex_exit(t->p_lock);
	}
out:
	if (regs)
		kmem_free(regs, sizeof(*regs));
	if (linux_regs)
		kmem_free(linux_regs, sizeof(*linux_regs));
	return error;

}
