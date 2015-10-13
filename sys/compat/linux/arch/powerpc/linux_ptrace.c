/*	$NetBSD: linux_ptrace.c,v 1.29 2015/10/13 08:24:35 pgoyette Exp $ */

/*-
 * Copyright (c) 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Scheler and Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: linux_ptrace.c,v 1.29 2015/10/13 08:24:35 pgoyette Exp $");

#include <sys/param.h>
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
 * From Linux's include/asm-ppc/ptrace.h.
 * structure used for storing reg context:  defined in linux_machdep.h
 * structure used for storing floating point context:
 * Linux just uses a double fpr[32], no struct
 */

/*
 * user struct for linux process - this is used for Linux ptrace emulation
 * most of it is junk only used by gdb
 *
 * From Linux's include/asm-ppc/user.h
 *
 * XXX u_ar0 was a struct reg in Linux/powerpc
 * Can't find out what a struct regs is for Linux/powerpc,
 * so we use a struct pt_regs instead. don't know if this is right.
 */

struct linux_user {
	struct linux_pt_regs regs;
#define lusr_startgdb regs
	size_t u_tsize;
	size_t u_dsize;
	size_t u_ssize;
	unsigned long start_code;
	unsigned long start_data;
	unsigned long start_stack;
	long int signal;
	struct linux_pt_regs *u_ar0;		/* help gdb find registers */
	unsigned long magic;
	char u_comm[32];
#define lu_comm_end u_comm[31]
};

#define LUSR_OFF(member) offsetof(struct linux_user, member)
#define LUSR_REG_OFF(member) offsetof(struct linux_pt_regs, member)
#define ISSET(t, f) ((t) & (f))

int linux_ptrace_disabled = 1;	/* bitrotted */

/* XXX Check me! (From NetBSD/i386) */
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
	struct reg *regs = NULL;
	struct fpreg *fpregs = NULL;
	struct linux_pt_regs *linux_regs = NULL;
	double *linux_fpreg = NULL;  /* it's an array, not a struct */
	int request, error, addr, i;

	if (linux_ptrace_disabled)
		return ENOSYS;

	error = 0;
	request = SCARG(uap, request);

	switch (request) {
	case LINUX_PTRACE_PEEKUSR:
	case LINUX_PTRACE_POKEUSR:
		regs = kmem_alloc(sizeof(struct reg), KM_SLEEP);
		break;
	case LINUX_PTRACE_GETREGS:
	case LINUX_PTRACE_SETREGS:
		regs = kmem_alloc(sizeof(struct reg), KM_SLEEP);
		linux_regs = kmem_alloc(sizeof(*linux_regs), KM_SLEEP);
		if (request == LINUX_PTRACE_SETREGS) {
			error = copyin((void *)SCARG(uap, data), linux_regs,
			    sizeof(struct linux_pt_regs));
			if (error) {
				goto out;
			}
		}
		break;
	case LINUX_PTRACE_GETFPREGS:
	case LINUX_PTRACE_SETFPREGS:
		fpregs = kmem_alloc(sizeof(struct fpreg), KM_SLEEP);
		linux_fpreg = kmem_alloc(32 * sizeof(double), KM_SLEEP);
		if (request == LINUX_PTRACE_SETFPREGS) {
			error = copyin((void *)SCARG(uap, data), linux_fpreg,
			    32 * sizeof(double));
			if (error) {
				goto out;
			}
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
	if (ISSET(t->p_slflag, PSL_FSTRACE) || t->p_pptr != p ||
	    t->p_stat != SSTOP || !t->p_waited) {
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
		for (i = 0; i <= 31; i++) {
			linux_regs->lgpr[i] = regs->fixreg[i];
		}
		linux_regs->lnip = regs->pc;
		linux_regs->lmsr = 0;
		/* XXX Is that right? */
		linux_regs->lorig_gpr3 = regs->fixreg[3];
		linux_regs->lctr = regs->ctr;
		linux_regs->llink = regs->lr;
		linux_regs->lxer = regs->xer;
		linux_regs->lccr = regs->cr;
		linux_regs->lmq = 0;
		linux_regs->ltrap = 0;
		linux_regs->ldar = 0;
		linux_regs->ldsisr = 0;
		linux_regs->lresult = 0;

		error = copyout(linux_regs, (void *)SCARG(uap, data),
		    sizeof(struct linux_pt_regs));
		break;

	case LINUX_PTRACE_SETREGS:
		for (i = 0; i <= 31; i++) {
			regs->fixreg[i] = linux_regs->lgpr[i];
		}
		regs->lr = linux_regs->llink;
		regs->cr = linux_regs->lccr;
		regs->xer = linux_regs->lxer;
		regs->ctr = linux_regs->lctr;
		regs->pc = linux_regs->lnip; /* XXX */

		error = process_write_regs(lt, regs);
		mutex_exit(t->p_lock);
		break;

	case LINUX_PTRACE_GETFPREGS:
		error = process_read_fpregs(lt, fpregs, NULL);
		mutex_exit(t->p_lock);
		if (error) {
			break;
		}
		/* Zero the contents if NetBSD fpreg structure is smaller */
		if (sizeof(struct fpreg) < (32 * sizeof(double))) {
			memset(linux_fpreg, '\0', (32 * sizeof(double)));
		}
		memcpy(linux_fpreg, fpregs,
		    min(32 * sizeof(double), sizeof(struct fpreg)));
		error = copyout(linux_fpreg, (void *)SCARG(uap, data),
		    32 * sizeof(double));
		break;

	case LINUX_PTRACE_SETFPREGS:
		memset(fpregs, '\0', sizeof(struct fpreg));
		memcpy(fpregs, linux_fpreg,
		    min(32 * sizeof(double), sizeof(struct fpreg)));
		error = process_write_fpregs(lt, fpregs, sizeof fpregs);
		mutex_exit(t->p_lock);
		break;

	case LINUX_PTRACE_PEEKUSR:
		addr = SCARG(uap, addr);
		error = process_read_regs(lt, regs);
		mutex_exit(t->p_lock);
		if (error) {
			break;
		}
		error = 0;
		if ((addr < LUSR_OFF(lusr_startgdb)) ||
		    (addr > LUSR_OFF(lu_comm_end)))
		 	error = 1;
		else if (addr == LUSR_OFF(u_tsize))
			*retval = p->p_vmspace->vm_tsize;
		else if (addr == LUSR_OFF(u_dsize))
			*retval = p->p_vmspace->vm_dsize;
		else if (addr == LUSR_OFF(u_ssize))
			*retval = p->p_vmspace->vm_ssize;
		else if (addr == LUSR_OFF(start_code))
			*retval = (register_t) p->p_vmspace->vm_taddr;
		else if (addr == LUSR_OFF(start_stack))
			*retval = (register_t) p->p_vmspace->vm_minsaddr;
		else if ((addr >= LUSR_REG_OFF(lpt_regs_fixreg_begin)) &&
		    (addr <= LUSR_REG_OFF(lpt_regs_fixreg_end)))
			*retval = regs->fixreg[addr / sizeof (register_t)];
		else if (addr == LUSR_REG_OFF(lnip))
			*retval = regs->pc;
		else if (addr == LUSR_REG_OFF(lctr))
			*retval = regs->ctr;
		else if (addr == LUSR_REG_OFF(llink))
			*retval = regs->lr;
		else if (addr == LUSR_REG_OFF(lxer))
			*retval = regs->xer;
		else if (addr == LUSR_REG_OFF(lccr))
			*retval = regs->cr;
		else if (addr == LUSR_OFF(signal))
			error = 1;
		else if (addr == LUSR_OFF(magic))
			error = 1;
		else if (addr == LUSR_OFF(u_comm))
			error = 1;
		else {
#ifdef DEBUG_LINUX
			printf("linux_ptrace: unsupported address: %d\n", addr);
#endif
			error = 1;
		}
		if (error) {
			break;
		}
		error = copyout (retval, (void *)SCARG(uap, data),
		    sizeof(retval));
		*retval = SCARG(uap, data);
		break;

	case  LINUX_PTRACE_POKEUSR: /* XXX Not tested */
		addr = SCARG(uap, addr);
		error = process_read_regs(lt, regs);
		if (error) {
			mutex_exit(t->p_lock);
			break;
		}
		error = 0;
		if ((addr < LUSR_OFF(lusr_startgdb)) ||
		    (addr > LUSR_OFF(lu_comm_end)))
		 	error = 1;
		else if (addr == LUSR_OFF(u_tsize))
			error = 1;
		else if (addr == LUSR_OFF(u_dsize))
			error = 1;
		else if (addr == LUSR_OFF(u_ssize))
			error = 1;
		else if (addr == LUSR_OFF(start_code))
			error = 1;
		else if (addr == LUSR_OFF(start_stack))
			error = 1;
		else if ((addr >= LUSR_REG_OFF(lpt_regs_fixreg_begin)) &&
		    (addr <= LUSR_REG_OFF(lpt_regs_fixreg_end)))
			regs->fixreg[addr / sizeof (register_t)] =
			    (register_t)SCARG(uap, data);
		else if (addr == LUSR_REG_OFF(lnip))
			regs->pc = (register_t)SCARG(uap, data);
		else if (addr == LUSR_REG_OFF(lctr))
			regs->ctr = (register_t)SCARG(uap, data);
		else if (addr == LUSR_REG_OFF(llink))
			regs->lr = (register_t)SCARG(uap, data);
		else if (addr == LUSR_REG_OFF(lxer))
			regs->xer = (register_t)SCARG(uap, data);
		else if (addr == LUSR_REG_OFF(lccr))
			regs->cr = (register_t)SCARG(uap, data);
		else if (addr == LUSR_OFF(signal))
			error = 1;
		else if (addr == LUSR_OFF(magic))
			error = 1;
		else if (addr == LUSR_OFF(u_comm))
			error = 1;
		else {
#ifdef DEBUG_LINUX
			printf("linux_ptrace: unsupported address: %d\n", addr);
#endif
			error = 1;
		}
		error = process_write_regs(lt,regs);
		mutex_exit(t->p_lock);
		break;
	default:
		mutex_exit(t->p_lock);
		break;
	}
out:
	if (regs)
		kmem_free(regs, sizeof(*regs));
	if (fpregs)
		kmem_free(fpregs, sizeof(*fpregs));
	if (linux_regs)
		kmem_free(linux_regs, sizeof(*linux_regs));
	if (linux_fpreg)
		kmem_free(linux_fpreg, sizeof(*linux_fpreg));
	return error;
}
