/*	$NetBSD: process_machdep.c,v 1.1 2006/04/07 14:21:18 cherry Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.1 2006/04/07 14:21:18 cherry Exp $");

#include <sys/param.h>
#include <sys/ptrace.h>

#include <machine/reg.h>
int
process_read_regs(struct lwp *l, struct reg *regs)
{
	return 0;
}


int
process_write_regs(struct lwp *l, const struct reg *regs)
{
	return 0;
}


int
process_read_fpregs(struct lwp *l, struct fpreg *fpregs)
{
	return 0;
}


int
process_write_fpregs(struct lwp *l, const struct fpreg *fpregs)
{
	return 0;
}

/*
 * Set the process's program counter.
 */
int
process_set_pc(struct lwp *l, caddr_t addr)
{
	return 0;
}


int
process_sstep(struct lwp *l, int sstep)
{
	return 0;
}

