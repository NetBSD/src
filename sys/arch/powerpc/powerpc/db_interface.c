/*	$NetBSD: db_interface.c,v 1.5.2.2 2000/12/08 09:30:17 bouyer Exp $ */
/*	$OpenBSD: db_interface.c,v 1.2 1996/12/28 06:21:50 rahnds Exp $	*/

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>
#include <machine/frame.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <ddb/ddbvar.h>

extern label_t *db_recover;

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

int
kdb_trap(type, v)
	int type;
	void *v;
{
	struct trapframe *frame = v;

	switch (type) {
	case T_BREAKPOINT:
	case -1:
		break;
	default:
		if (!db_onpanic && db_recover == 0)
			return 0;
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* XXX Should switch to kdb's own stack here. */

	bcopy(frame->fixreg, DDB_REGS->r, 32 * sizeof(u_int32_t));
	DDB_REGS->iar = frame->srr0;
	DDB_REGS->msr = frame->srr1;

	db_trap(T_BREAKPOINT, 0);

	bcopy(DDB_REGS->r, frame->fixreg, 32 * sizeof(u_int32_t));
	frame->srr0 = DDB_REGS->iar;
	frame->srr1 = DDB_REGS->msr;

	return 1;
}
