/*	$NetBSD: db_interface.c,v 1.2 1998/02/23 03:23:05 mycroft Exp $ */
/*	$OpenBSD: db_interface.c,v 1.2 1996/12/28 06:21:50 rahnds Exp $	*/

#include <sys/param.h>
#include <sys/proc.h>

#include <machine/frame.h>
#include <machine/db_machdep.h>

void
Debugger()
{
	ddb_trap();
}

int
ddb_trap_glue(frame)
	struct trapframe *frame;
{
	int msr;

	if (!(frame->srr1 & PSL_PR)
	    && (frame->exc == EXC_TRC
		|| (frame->exc == EXC_PGM
		    && (frame->srr1 & 0x20000))
		|| frame->exc == EXC_BPT)) {

		bcopy(frame->fixreg, DDB_REGS->r, 32 * sizeof(u_int32_t));
		DDB_REGS->iar = frame->srr0;
		DDB_REGS->msr = frame->srr1;

		db_trap(T_BREAKPOINT, 0);

		bcopy(DDB_REGS->r, frame->fixreg, 32 * sizeof(u_int32_t));

		return 1;
	}
	return 0;
}
