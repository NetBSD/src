/*	$NetBSD: db_interface.c,v 1.6 2000/06/09 14:09:45 kleink Exp $ */
/*	$OpenBSD: db_interface.c,v 1.2 1996/12/28 06:21:50 rahnds Exp $	*/

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <ddb/ddbvar.h>

#include <machine/frame.h>
#include <machine/db_machdep.h>

void
cpu_Debugger()
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
