/*	$NetBSD: svr4_syscall.c,v 1.1.2.2 2002/07/16 08:36:54 gehenna Exp $	*/

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
