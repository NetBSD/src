/*	$NetBSD: hpux_syscall.c,v 1.1.12.2 2004/09/18 14:36:38 skrll Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_syscall.c,v 1.1.12.2 2004/09/18 14:36:38 skrll Exp $");

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
