/*	$NetBSD: hpux_syscall.c,v 1.1.6.2 2002/09/06 08:36:47 jdolecek Exp $	*/

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
