/* $NetBSD: syscall.c,v 1.1 2006/04/07 14:21:18 cherry Exp $ */

/* XXX: based on alpha/syscall.c */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.1 2006/04/07 14:21:18 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/frame.h>

void	syscall_intern(struct proc *);
void	syscall_plain(struct lwp *, u_int64_t, struct trapframe *);
void	syscall_fancy(struct lwp *, u_int64_t, struct trapframe *);

void
syscall_intern(struct proc *p)
{
	return;
}

/*
 * Process a system call.
 */
void
syscall_plain(struct lwp *l, u_int64_t code, struct trapframe *framep)
{
	return;
}

void
syscall_fancy(struct lwp *l, u_int64_t code, struct trapframe *framep)
{
	return;
}

/*
 * Process the tail end of a fork() for the child.
 */
void
child_return(void *arg)
{
	return;
}
