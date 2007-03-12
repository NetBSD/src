/*	$NetBSD: linux32_syscall.c,v 1.8.2.2 2007/03/12 05:46:16 rmind Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux32_syscall.c,v 1.8.2.2 2007/03/12 05:46:16 rmind Exp $");

#include "opt_ktrace.h"
#include "opt_systrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

#include <compat/linux32/arch/amd64/linux32_errno.h>

void linux32_syscall_intern(struct proc *);
void linux32_syscall_plain(struct trapframe *);
void linux32_syscall_fancy(struct trapframe *);

void
linux32_syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = linux32_syscall_fancy;
	else
		p->p_md.md_syscall = linux32_syscall_plain;
}

void
linux32_syscall_plain(frame)
	struct trapframe *frame;
{
	char *params;
	const struct sysent *callp;
	struct proc *p;
	struct lwp *l;
	int error;
	size_t argsize;
	register32_t code, args[8];
	register_t rval[2];

	uvmexp.syscalls++;
	l = curlwp;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);

	code = frame->tf_rax;
	callp = p->p_emul->e_sysent;
	params = (char *)frame->tf_rsp + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		break;
	default:
		break;
	}

	code &= (SYS_NSYSENT - 1);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		/*
		 * Linux passes the args in ebx, ecx, edx, esi, edi, ebp, in
		 * increasing order.
		 */
		switch (argsize >> 2) {
		case 6:
			args[5] = frame->tf_rbp & 0xffffffff;
		case 5:
			args[4] = frame->tf_rdi & 0xffffffff;
		case 4:
			args[3] = frame->tf_rsi & 0xffffffff;
		case 3:
			args[2] = frame->tf_rdx & 0xffffffff;
		case 2:
			args[1] = frame->tf_rcx & 0xffffffff;
		case 1:
			args[0] = frame->tf_rbx & 0xffffffff;
			break;
		default:
			printf("linux syscall %d bogus argument size %ld",
			    code, argsize);
			error = ENOSYS;
			goto out;
			break;
		}
	}

	rval[0] = 0;
	rval[1] = 0;
#if 0
	printf("linux32: syscall %d (%x %x %x %x %x %x, %x)\n", code,
	    args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
#endif
	KERNEL_LOCK(1, l);
	error = (*callp->sy_call)(l, args, rval);
	KERNEL_UNLOCK_LAST(l);

out:
	switch (error) {
	case 0:
		frame->tf_rax = rval[0];
		frame->tf_rflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame->tf_rip -= frame->tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
		frame->tf_rax = native_to_linux32_errno[error];
		frame->tf_rflags |= PSL_C;	/* carry bit */
		break;
	}

	userret(l);
}

void
linux32_syscall_fancy(frame)
	struct trapframe *frame;
{
	char *params;
	const struct sysent *callp;
	struct proc *p;
	struct lwp *l;
	int error;
	size_t argsize;
	register32_t code, args[8];
	register_t rval[2];
#if defined(KTRACE) || defined(SYSTRACE)
	int i;
	register_t args64[8];
#endif

	uvmexp.syscalls++;
	l = curlwp;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);

	code = frame->tf_rax;
	callp = p->p_emul->e_sysent;
	params = (char *)frame->tf_rsp + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		break;
	default:
		break;
	}

	code &= (SYS_NSYSENT - 1);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		/*
		 * Linux passes the args in ebx, ecx, edx, esi, edi, ebp, in
		 * increasing order.
		 */
		switch (argsize >> 2) { 
		case 6:
			args[5] = frame->tf_rbp & 0xffffffff;
		case 5:
			args[4] = frame->tf_rdi & 0xffffffff;
		case 4:
			args[3] = frame->tf_rsi & 0xffffffff;
		case 3:
			args[2] = frame->tf_rdx & 0xffffffff;
		case 2:
			args[1] = frame->tf_rcx & 0xffffffff;
		case 1:
			args[0] = frame->tf_rbx & 0xffffffff;
			break;
		default:
			printf("linux syscall %d bogus argument size %ld",
			    code, argsize);
			error = ENOSYS;
#if defined(KTRACE) || defined(SYSTRACE)
			goto out;
#endif
			break;
		}
	}

#if 0
	printf("linux32: syscall %d (%x, %x, %x, %x, %x, %x, %x) [%ld]\n", code,
	    args[0], args[1], args[2], args[3], args[4], args[5], args[6],
	    (argsize >> 2));
#endif
	KERNEL_LOCK(1, l);

#if defined(KTRACE) || defined(SYSTRACE)
	if (
#ifdef KTRACE
	    KTRPOINT(p, KTR_SYSCALL) ||
#endif
#ifdef SYSTRACE
	    ISSET(p->p_flag, PK_SYSTRACE)
#else
	0
#endif
	) {
		for (i = 0; i < (argsize >> 2); i++)
			args64[i] = args[i] & 0xffffffff;
		/* XXX we need to pass argsize << 1 here? */
		if ((error = trace_enter(l, code, code, NULL, args64)) != 0)
			goto out;
	}
#endif

	rval[0] = 0;
	rval[1] = 0;

	error = (*callp->sy_call)(l, args, rval);
#if defined(KTRACE) || defined(SYSTRACE)
out:
#endif
	KERNEL_UNLOCK_LAST(l);
	switch (error) {
	case 0:
		frame->tf_rax = rval[0];
		frame->tf_rflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame->tf_rip -= frame->tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
		frame->tf_rax = native_to_linux32_errno[error];
		frame->tf_rflags |= PSL_C;	/* carry bit */
		break;
	}

#if defined(KTRACE) || defined(SYSTRACE)
	trace_exit(l, code, args64, rval, error);
#endif

	userret(l);
}
