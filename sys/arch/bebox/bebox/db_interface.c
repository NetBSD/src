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

		bzero(DDB_REGS, sizeof(db_regs_t));
		bcopy(frame->fixreg, DDB_REGS, 32 * sizeof(u_int32_t));
		PC_REGS(DDB_REGS) = frame->srr0;

		db_trap(T_BREAKPOINT, 0);

#if 0
		if ((db_addr_t)DDB_REGS->msr | SR_SINGLESTEP) {
			asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
				: "=r"(msr) : "K"((u_short)SR_SINGLESTEP));
		} else {
			asm volatile ("mfmsr %0" : "=r"(msr));
			msr &= ~SR_SINGLESTEP;
			asm volatile ("mtmsr %0" :: "r"(msr));
		}
#endif

		return 1;
	}
	return 0;
}
