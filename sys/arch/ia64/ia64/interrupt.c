/*	$NetBSD: interrupt.c,v 1.1 2006/04/07 14:21:18 cherry Exp $	*/

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.1 2006/04/07 14:21:18 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/md_var.h>


int
interrupt(u_int64_t vector, struct trapframe *tf)
{
	return 0;
}
