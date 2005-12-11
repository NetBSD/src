/*	$NetBSD: svr4_syscall.c,v 1.4 2005/12/11 12:17:59 christos Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_syscall.c,v 1.4 2005/12/11 12:17:59 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>

void	svr4_syscall_intern(struct proc *);

void
svr4_syscall_intern(struct proc *p)
{
	/*
	 * XXX: Just defer to the regular syscall_intern() for now.
	 */
	extern void syscall_intern(struct proc *);

	syscall_intern(p);
}
