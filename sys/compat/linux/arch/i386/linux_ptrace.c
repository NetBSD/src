/*	$NetBSD: linux_ptrace.c,v 1.26.18.2 2017/12/03 11:36:54 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_ptrace.c,v 1.26.18.2 2017/12/03 11:36:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/systm.h>
#include <sys/syscall.h>
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

struct linux_reg {
	long ebx;
	long ecx;
	long edx;
	long esi;
	long edi;
	long ebp;
	long eax;
	long xds, xes;		/* unsigned short ds, __ds, es, __es; */
	long __fs, __gs;	/* unsigned short fs, __fs, gs, __gs; */
	long orig_eax;
	long eip;
	long xcs;		/* unsigned short cs, __cs; */
	long eflags;
	long esp;
	long xss;		/* unsigned short ss, __ss; */
};

/* structure used for storing floating point context */
struct linux_fpctx {
	long cwd;
	long swd;
	long twd;
	long fip;
	long fcs;
	long foo;
	long fos;
	long st_space[20];
};

/* user struct for linux process - this is used for Linux ptrace emulation */
/* most of it is junk only used by gdb */
struct linux_user {
	struct linux_reg regs;		/* registers */
	int u_fpvalid;		/* true if math co-processor being used. */
	struct linux_fpctx i387;	/* Math Co-processor registers. */
/* The rest of this junk is to help gdb figure out what goes where */
#define lusr_startgdb	u_tsize
	unsigned long int u_tsize;	/* Text segment size (pages). */
	unsigned long int u_dsize;	/* Data segment size (pages). */
	unsigned long int u_ssize;	/* Stack segment size (pages). */
	unsigned long start_code;	/* Starting virtual address of text. */
	unsigned long start_stack;	/* Starting virtual address of stack
					   area. This is actually the bottom of
					   the stack, the top of the stack is
					   always found in the esp register. */
	long int __signal;  		/* Signal that caused the core dump. */
	int __reserved;			/* unused */
	void *u_ar0;			/* Used by gdb to help find the values
					   for the registers. */
	struct linux_fpctx *u_fpstate;	/* Math Co-processor pointer. */
	unsigned long __magic;		/* To uniquely identify a core file */
	char u_comm[32];		/* User command that was responsible */
	int u_debugreg[8];
#define u_debugreg_end	u_debugreg[7]
};

#define LUSR_OFF(member)	offsetof(struct linux_user, member)
#define ISSET(t, f)		((t) & (f))

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
	struct reg *regs = NULL;
	struct fpreg *fpregs = NULL;
	struct linux_reg *linux_regs = NULL;
	struct linux_fpctx *linux_fpregs = NULL;
	struct linux_emuldata *led;
	int request, error, addr;
	size_t fp_size;

	if (linux_ptrace_disabled)
		return ENOSYS;

	error = 0;
	request = SCARG(uap, request);

	switch (request) {
	case LINUX_PTRACE_PEEKUSR:
	case LINUX_PTRACE_POKEUSR:
		break;
	case LINUX_PTRACE_GETREGS:
	case LINUX_PTRACE_SETREGS:
		regs = kmem_alloc(sizeof(struct reg), KM_SLEEP);
		linux_regs = kmem_alloc(sizeof(struct linux_reg), KM_SLEEP);
		if (request == LINUX_PTRACE_SETREGS) {
			error = copyin((void *)SCARG(uap, data), linux_regs,
			    sizeof(struct linux_reg));
			if (error) {
				goto out;
			}
		}
		break;
	case LINUX_PTRACE_GETFPREGS:
	case LINUX_PTRACE_SETFPREGS:
		fpregs = kmem_alloc(sizeof(struct fpreg), KM_SLEEP);
		linux_fpregs = kmem_alloc(sizeof(struct linux_fpctx), KM_SLEEP);
		if (request == LINUX_PTRACE_SETFPREGS) {
			error = copyin((void *)SCARG(uap, data), linux_fpregs,
			    sizeof(struct linux_fpctx));
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
	 * 2. It is not being traced by _you_, or
	 * 3. It is not currently stopped.
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
		linux_regs->ebx = regs->r_ebx;
		linux_regs->ecx = regs->r_ecx;
		linux_regs->edx = regs->r_edx;
		linux_regs->esi = regs->r_esi;
		linux_regs->edi = regs->r_edi;
		linux_regs->ebp = regs->r_ebp;
		linux_regs->eax = regs->r_eax;
		linux_regs->xds = regs->r_ds;
		linux_regs->xes = regs->r_es;
		linux_regs->orig_eax = regs->r_eax; /* XXX is this correct? */
		linux_regs->eip = regs->r_cs + regs->r_eip;
		linux_regs->xcs = regs->r_cs;
		linux_regs->eflags = regs->r_eflags;
		linux_regs->esp = regs->r_esp;
		linux_regs->xss = regs->r_ss;

		error = copyout(linux_regs, (void *)SCARG(uap, data),
		    sizeof(struct linux_reg));
		break;

	case LINUX_PTRACE_SETREGS:
		regs->r_ebx = linux_regs->ebx;
		regs->r_ecx = linux_regs->ecx;
		regs->r_edx = linux_regs->edx;
		regs->r_esi = linux_regs->esi;
		regs->r_edi = linux_regs->edi;
		regs->r_ebp = linux_regs->ebp;
		regs->r_eax = linux_regs->eax;
		regs->r_ds = linux_regs->xds;
		regs->r_es = linux_regs->xes;
		regs->r_eip = linux_regs->eip - linux_regs->xcs;
		regs->r_cs = linux_regs->xcs;
		regs->r_eflags = linux_regs->eflags;
		regs->r_esp = linux_regs->esp;
		regs->r_ss = linux_regs->xss;

		error = process_write_regs(lt, regs);
		mutex_exit(t->p_lock);
		break;

	case LINUX_PTRACE_GETFPREGS:
		fp_size = sizeof fpregs;
		error = process_read_fpregs(lt, fpregs, &fp_size);
		mutex_exit(t->p_lock);
		if (error) {
			break;
		}
		/* Zero the contents if NetBSD fpreg structure is smaller */
		if (fp_size < sizeof(struct linux_fpctx)) {
			memset(linux_fpregs, '\0', sizeof(struct linux_fpctx));
		}
		memcpy(linux_fpregs, fpregs,
		    min(sizeof(struct linux_fpctx), fp_size));
		error = copyout(linux_fpregs, (void *)SCARG(uap, data),
		    sizeof(struct linux_fpctx));
		break;

	case LINUX_PTRACE_SETFPREGS:
		memset(fpregs, '\0', sizeof(struct fpreg));
		memcpy(fpregs, linux_fpregs,
		    min(sizeof(struct linux_fpctx), sizeof(struct fpreg)));
		error = process_write_regs(lt, regs);
		mutex_exit(t->p_lock);
		break;

	case LINUX_PTRACE_PEEKUSR:
		/* XXX locking */
		addr = SCARG(uap, addr);

		error = 0;
		if (addr < LUSR_OFF(lusr_startgdb)) {
			/* XXX should provide appropriate register */
			error = ENOTSUP;
		} else if (addr == LUSR_OFF(u_tsize))
			*retval = p->p_vmspace->vm_tsize;
		else if (addr == LUSR_OFF(u_dsize))
			*retval = p->p_vmspace->vm_dsize;
		else if (addr == LUSR_OFF(u_ssize))
			*retval = p->p_vmspace->vm_ssize;
		else if (addr == LUSR_OFF(start_code))
			*retval = (register_t) p->p_vmspace->vm_taddr;
		else if (addr == LUSR_OFF(start_stack))
			*retval = (register_t) p->p_vmspace->vm_minsaddr;
		else if (addr == LUSR_OFF(u_ar0))
			*retval = LUSR_OFF(regs);
		else if (addr >= LUSR_OFF(u_debugreg)
			   && addr <= LUSR_OFF(u_debugreg_end)) {
			int off = (addr - LUSR_OFF(u_debugreg)) / sizeof(int);

			/* only do this for Linux processes */
			if (t->p_emul != &emul_linux) {
				mutex_exit(t->p_lock);
				return EINVAL;
			}

			led = lt->l_emuldata;
			*retval = led->led_debugreg[off];
		} else if (addr == LUSR_OFF(__signal)) {
			error = ENOTSUP;
		} else if (addr == LUSR_OFF(u_fpstate)) {
			error = ENOTSUP;
		} else if (addr == LUSR_OFF(__magic)) {
			error = ENOTSUP;
		} else if (addr == LUSR_OFF(u_comm)) {
			error = ENOTSUP;
		} else {
#ifdef DEBUG_LINUX
			printf("linux_ptrace: unsupported address: %d\n", addr);
#endif
			error = ENOTSUP;
		}
		mutex_exit(t->p_lock);
		break;

	case LINUX_PTRACE_POKEUSR:
		/* we only support setting debugregs for now */
		addr = SCARG(uap, addr);
		if (addr >= LUSR_OFF(u_debugreg) &&
		    addr <= LUSR_OFF(u_debugreg_end)) {
			int off = (addr - LUSR_OFF(u_debugreg)) / sizeof(int);
			int data = SCARG(uap, data);

			/* only do this for Linux processes */
			if (t->p_emul != &emul_linux) {
				mutex_exit(t->p_lock);
				return EINVAL;
			}
			led = lt->l_emuldata;
			led->led_debugreg[off] = data;
		}
		mutex_exit(t->p_lock);
		break;

	default:
		mutex_exit(t->p_lock);
		break;
	}
out:
	if (regs)
		kmem_free(regs, sizeof(*regs));
	if (linux_regs)
		kmem_free(linux_regs, sizeof(*linux_regs));
	if (fpregs)
		kmem_free(fpregs, sizeof(*fpregs));
	if (linux_fpregs)
		kmem_free(linux_fpregs, sizeof(*linux_fpregs));

	return error;
}
