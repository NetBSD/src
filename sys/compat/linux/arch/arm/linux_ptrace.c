/*	$NetBSD: linux_ptrace.c,v 1.1 2002/01/14 23:14:39 bjh21 Exp $	*/

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: linux_ptrace.c,v 1.1 2002/01/14 23:14:39 bjh21 Exp $");

#include <sys/systm.h>

int
linux_sys_ptrace_arch(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	panic("linux_sys_ptrace_arch not implemented");
}
