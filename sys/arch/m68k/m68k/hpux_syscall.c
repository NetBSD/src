/*	$NetBSD: hpux_syscall.c,v 1.3 2005/12/11 12:17:59 christos Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_syscall.c,v 1.3 2005/12/11 12:17:59 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>

void	hpux_syscall_intern(struct proc *);

void
hpux_syscall_intern(struct proc *p)
{
	/*
	 * XXX: Just defer to the regular syscall_intern() for now.
	 */
	extern void syscall_intern(struct proc *);

	syscall_intern(p);
}
