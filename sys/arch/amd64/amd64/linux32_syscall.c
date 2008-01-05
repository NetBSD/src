/*	$NetBSD: linux32_syscall.c,v 1.19 2008/01/05 12:53:55 dsl Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux32_syscall.c,v 1.19 2008/01/05 12:53:55 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

#include <compat/linux32/arch/amd64/linux32_errno.h>

void linux32_syscall_intern(struct proc *);
void linux32_syscall(struct trapframe *);

void
linux32_syscall_intern(struct proc *p)
{

	p->p_trace_enabled = trace_is_enabled(p);
	p->p_md.md_syscall = linux32_syscall;
}

void
linux32_syscall(frame)
	struct trapframe *frame;
{
	const struct sysent *callp;
	struct proc *p;
	struct lwp *l;
	int error;
	size_t argsize;
	register32_t code, args[6];
	register_t rval[2];
	int i;
	register_t args64[6];

	l = curlwp;
	p = l->l_proc;

	code = frame->tf_rax;
	uvmexp.syscalls++;

	LWP_CACHE_CREDS(l, p);

	callp = p->p_emul->e_sysent;

	code &= (SYS_NSYSENT - 1);
	callp += code;

	/*
	 * Linux passes the args in ebx, ecx, edx, esi, edi, ebp, in
	 * increasing order.
	 */
	args[0] = frame->tf_rbx & 0xffffffff;
	args[1] = frame->tf_rcx & 0xffffffff;
	args[2] = frame->tf_rdx & 0xffffffff;
	args[3] = frame->tf_rsi & 0xffffffff;
	args[4] = frame->tf_rdi & 0xffffffff;
	args[5] = frame->tf_rbp & 0xffffffff;

	KERNEL_LOCK(1, l);

	if (__predict_false(p->p_trace_enabled)) {
		argsize = callp->sy_argsize;
		if (__predict_false(argsize > sizeof args))
			panic("impossible syscall argsize, code %d, size %zd",
			    code, argsize);
		for (i = 0; i < (argsize >> 2); i++)
			args64[i] = args[i] & 0xffffffff;
		if ((error = trace_enter(code, code, NULL, args64)) != 0)
			goto out;
	}

	rval[0] = 0;
	rval[1] = 0;

	error = (*callp->sy_call)(l, args, rval);
out:
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
		error = native_to_linux32_errno[error];
		frame->tf_rax = error;
		frame->tf_rflags |= PSL_C;	/* carry bit */
		break;
	}

	if (__predict_false(p->p_trace_enabled))
		trace_exit(code, args64, rval, error);
	userret(l);
}
