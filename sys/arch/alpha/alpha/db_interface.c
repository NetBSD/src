/* $NetBSD: db_interface.c,v 1.27.34.1 2009/05/13 17:16:04 jym Exp $ */

/* 
 * Mach Operating System
 * Copyright (c) 1992,1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS ``AS IS''
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Parts of this file are derived from Mach 3:
 *
 *	File: alpha_instruction.c
 *	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	6/92
 */

/*
 * Interface to DDB.
 *
 * Modified for NetBSD/alpha by:
 *
 *	Christopher G. Demetriou, Carnegie Mellon University
 *
 *	Jason R. Thorpe, Numerical Aerospace Simulation Facility,
 *	NASA Ames Research Center
 */

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.27.34.1 2009/05/13 17:16:04 jym Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/alpha.h>
#include <machine/db_machdep.h>
#include <machine/pal.h>
#include <machine/prom.h>

#include <alpha/alpha/db_instruction.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_interface.h>


#if 0
extern char *trap_type[];
extern int trap_types;
#endif

int	db_active = 0;

db_regs_t *ddb_regp;

#if defined(MULTIPROCESSOR)
void	db_mach_cpu(db_expr_t, bool, db_expr_t, const char *);
#endif

const struct db_command db_machine_command_table[] = {
#if defined(MULTIPROCESSOR)
	{ DDB_ADD_CMD("cpu",	db_mach_cpu,	0,NULL,NULL,NULL) },
#endif
	{ DDB_ADD_CMD(NULL,     NULL,           0,NULL,NULL,NULL) },
};

static int db_alpha_regop(const struct db_variable *, db_expr_t *, int);

#define	dbreg(xx)	((long *)(xx))

const struct db_variable db_regs[] = {
	{	"v0",	dbreg(FRAME_V0),	db_alpha_regop	},
	{	"t0",	dbreg(FRAME_T0),	db_alpha_regop	},
	{	"t1",	dbreg(FRAME_T1),	db_alpha_regop	},
	{	"t2",	dbreg(FRAME_T2),	db_alpha_regop	},
	{	"t3",	dbreg(FRAME_T3),	db_alpha_regop	},
	{	"t4",	dbreg(FRAME_T4),	db_alpha_regop	},
	{	"t5",	dbreg(FRAME_T5),	db_alpha_regop	},
	{	"t6",	dbreg(FRAME_T6),	db_alpha_regop	},
	{	"t7",	dbreg(FRAME_T7),	db_alpha_regop	},
	{	"s0",	dbreg(FRAME_S0),	db_alpha_regop	},
	{	"s1",	dbreg(FRAME_S1),	db_alpha_regop	},
	{	"s2",	dbreg(FRAME_S2),	db_alpha_regop	},
	{	"s3",	dbreg(FRAME_S3),	db_alpha_regop	},
	{	"s4",	dbreg(FRAME_S4),	db_alpha_regop	},
	{	"s5",	dbreg(FRAME_S5),	db_alpha_regop	},
	{	"s6",	dbreg(FRAME_S6),	db_alpha_regop	},
	{	"a0",	dbreg(FRAME_A0),	db_alpha_regop	},
	{	"a1",	dbreg(FRAME_A1),	db_alpha_regop	},
	{	"a2",	dbreg(FRAME_A2),	db_alpha_regop	},
	{	"a3",	dbreg(FRAME_A3),	db_alpha_regop	},
	{	"a4",	dbreg(FRAME_A4),	db_alpha_regop	},
	{	"a5",	dbreg(FRAME_A5),	db_alpha_regop	},
	{	"t8",	dbreg(FRAME_T8),	db_alpha_regop	},
	{	"t9",	dbreg(FRAME_T9),	db_alpha_regop	},
	{	"t10",	dbreg(FRAME_T10),	db_alpha_regop	},
	{	"t11",	dbreg(FRAME_T11),	db_alpha_regop	},
	{	"ra",	dbreg(FRAME_RA),	db_alpha_regop	},
	{	"t12",	dbreg(FRAME_T12),	db_alpha_regop	},
	{	"at",	dbreg(FRAME_AT),	db_alpha_regop	},
	{	"gp",	dbreg(FRAME_GP),	db_alpha_regop	},
	{	"sp",	dbreg(FRAME_SP),	db_alpha_regop	},
	{	"pc",	dbreg(FRAME_PC),	db_alpha_regop	},
	{	"ps",	dbreg(FRAME_PS),	db_alpha_regop	},
	{	"ai",	dbreg(FRAME_T11),	db_alpha_regop	},
	{	"pv",	dbreg(FRAME_T12),	db_alpha_regop	},
};
const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

static int
db_alpha_regop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	unsigned long *tfaddr;
	unsigned long zeroval = 0;
	struct trapframe *f = NULL;

	if (vp->modif != NULL && *vp->modif == 'u') {
		if (curlwp != NULL)
			f = curlwp->l_md.md_tf;
	} else	f = DDB_REGS;
	tfaddr = f == NULL ? &zeroval : &f->tf_regs[(u_long)vp->valuep];
	switch (opcode) {
	case DB_VAR_GET:
		*val = *tfaddr;
		break;

	case DB_VAR_SET:
		*tfaddr = *val;
		break;

	default:
		panic("db_alpha_regop: unknown op %d", opcode);
	}

	return (0);
}

/*
 * ddb_trap - field a kernel trap
 */
int
ddb_trap(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long entry, db_regs_t *regs)
{
	struct cpu_info *ci = curcpu();
	int s;

	if (entry != ALPHA_KENTRY_IF ||
	    (a0 != ALPHA_IF_CODE_BPT && a0 != ALPHA_IF_CODE_BUGCHK)) {
		if (db_recover != 0) {
			/* This will longjmp back into db_command_loop() */
			db_error("Caught exception in ddb.\n");
			/* NOTREACHED */
		}

		/*
		 * Tell caller "We did NOT handle the trap."
		 * Caller should panic, or whatever.
		 */
		return (0);
	}

	/*
	 * alpha_debug() switches us to the debugger stack.
	 */

	/* Our register state is simply the trapframe. */
	ddb_regp = ci->ci_db_regs = regs;

	s = splhigh();

	db_active++;
	cnpollc(true);		/* Set polling mode, unblank video */

	db_trap(entry, a0);	/* Where the work happens */

	cnpollc(false);		/* Resume interrupt mode */
	db_active--;

	splx(s);

	ddb_regp = ci->ci_db_regs = NULL;

	/*
	 * Tell caller "We HAVE handled the trap."
	 */
	return (1);
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, register size_t size, register char *data)
{
	register char	*src;

	src = (char *)addr;
	while (size-- > 0)
		*data++ = *src++;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, register size_t size, register const char *data)
{
	register char	*dst;

	dst = (char *)addr;
	while (size-- > 0)
		*dst++ = *data++;
	alpha_pal_imb();
}

void
cpu_Debugger(void)
{

	__asm volatile("call_pal 0x81");		/* bugchk */
}

/*
 * Alpha-specific ddb commands:
 *
 *	cpu		tell DDB to use register state from the
 *			CPU specified (MULTIPROCESSOR)
 */

#if defined(MULTIPROCESSOR)
void
db_mach_cpu(db_expr_t addr, bool have_addr, db_expr_t count, const char * modif)
{
	struct cpu_info *ci;

	if (!have_addr) {
		cpu_debug_dump();
		return;
	}

	if (addr < 0 || addr >= ALPHA_MAXPROCS) {
		db_printf("CPU %ld out of range\n", addr);
		return;
	}

	ci = cpu_info[addr];
	if (ci == NULL) {
		db_printf("CPU %ld is not configured\n", addr);
		return;
	}

	if (ci != curcpu()) {
		if ((ci->ci_flags & CPUF_PAUSED) == 0) {
			db_printf("CPU %ld not paused\n", addr);
			return;
		}
	}

	if (ci->ci_db_regs == NULL) {
		db_printf("CPU %ld has no register state\n", addr);
		return;
	}

	db_printf("Using CPU %ld\n", addr);
	ddb_regp = ci->ci_db_regs;
}
#endif /* MULTIPROCESSOR */

/*
 * Map Alpha register numbers to trapframe/db_regs_t offsets.
 */
static int reg_to_frame[32] = {
	FRAME_V0,
	FRAME_T0,
	FRAME_T1,
	FRAME_T2,
	FRAME_T3,
	FRAME_T4,
	FRAME_T5,
	FRAME_T6,
	FRAME_T7,

	FRAME_S0,
	FRAME_S1,
	FRAME_S2,
	FRAME_S3,
	FRAME_S4,
	FRAME_S5,
	FRAME_S6,

	FRAME_A0,
	FRAME_A1,
	FRAME_A2,
	FRAME_A3,
	FRAME_A4,
	FRAME_A5,

	FRAME_T8,
	FRAME_T9,
	FRAME_T10,
	FRAME_T11,
	FRAME_RA,
	FRAME_T12,
	FRAME_AT,
	FRAME_GP,
	FRAME_SP,
	-1,		/* zero */
};

u_long
db_register_value(db_regs_t *regs, int regno)
{

	if (regno > 31 || regno < 0) {
		db_printf(" **** STRANGE REGISTER NUMBER %d **** ", regno);
		return (0);
	}

	if (regno == 31)
		return (0);

	return (regs->tf_regs[reg_to_frame[regno]]);
}

/*
 * Support functions for software single-step.
 */

bool
db_inst_call(int ins)
{
	alpha_instruction insn;

	insn.bits = ins;
	return ((insn.branch_format.opcode == op_bsr) ||
	    ((insn.jump_format.opcode == op_j) &&
	     (insn.jump_format.action & 1)));
}

bool
db_inst_return(int ins)
{
	alpha_instruction insn;

	insn.bits = ins;
	return ((insn.jump_format.opcode == op_j) &&
	    (insn.jump_format.action == op_ret));
}

bool
db_inst_trap_return(int ins)
{
	alpha_instruction insn;

	insn.bits = ins;
	return ((insn.pal_format.opcode == op_pal) &&
	    (insn.pal_format.function == PAL_OSF1_rti));
}

bool
db_inst_branch(int ins)
{
	alpha_instruction insn;

	insn.bits = ins;
	switch (insn.branch_format.opcode) {
	case op_j:
	case op_br:
	case op_fbeq:
	case op_fblt:
	case op_fble:
	case op_fbne:
	case op_fbge:
	case op_fbgt:
	case op_blbc:
	case op_beq:
	case op_blt:
	case op_ble:
	case op_blbs:
	case op_bne:
	case op_bge:
	case op_bgt:
		return (true);
	}

	return (false);
}

bool
db_inst_unconditional_flow_transfer(int ins)
{
	alpha_instruction insn;

	insn.bits = ins;
	switch (insn.branch_format.opcode) {
	case op_j:
	case op_br:
		return (true);

	case op_pal:
		switch (insn.pal_format.function) {
		case PAL_OSF1_retsys:
		case PAL_OSF1_rti:
		case PAL_OSF1_callsys:
			return (true);
		}
	}

	return (false);
}

#if 0
bool
db_inst_spill(int ins, int regn)
{
	alpha_instruction insn;

	insn.bits = ins;
	return ((insn.mem_format.opcode == op_stq) &&
	    (insn.mem_format.rd == regn));
}
#endif

bool
db_inst_load(int ins)
{
	alpha_instruction insn;

	insn.bits = ins;
	
	/* Loads. */
	if (insn.mem_format.opcode == op_ldbu ||
	    insn.mem_format.opcode == op_ldq_u ||
	    insn.mem_format.opcode == op_ldwu)
		return (true);
	if ((insn.mem_format.opcode >= op_ldf) &&
	    (insn.mem_format.opcode <= op_ldt))
		return (true);
	if ((insn.mem_format.opcode >= op_ldl) &&
	    (insn.mem_format.opcode <= op_ldq_l))
		return (true);

	/* Prefetches. */
	if (insn.mem_format.opcode == op_special) {
		/* Note: MB is treated as a store. */
		if ((insn.mem_format.displacement == (short)op_fetch) ||
		    (insn.mem_format.displacement == (short)op_fetch_m))
			return (true);
	}

	return (false);
}

bool
db_inst_store(int ins)
{
	alpha_instruction insn;

	insn.bits = ins;

	/* Stores. */
	if (insn.mem_format.opcode == op_stw ||
	    insn.mem_format.opcode == op_stb ||
	    insn.mem_format.opcode == op_stq_u)
		return (true);
	if ((insn.mem_format.opcode >= op_stf) &&
	    (insn.mem_format.opcode <= op_stt))
		return (true);
	if ((insn.mem_format.opcode >= op_stl) &&
	    (insn.mem_format.opcode <= op_stq_c))
		return (true);

	/* Barriers. */
	if (insn.mem_format.opcode == op_special) {
		if (insn.mem_format.displacement == op_mb)
			return (true);
	}

	return (false);
}

db_addr_t
db_branch_taken(int ins, db_addr_t pc, db_regs_t *regs)
{
	long signed_immediate;
	alpha_instruction insn;
	db_addr_t newpc;

	insn.bits = ins;
	switch (insn.branch_format.opcode) {
	/*
	 * Jump format: target PC is (contents of instruction's "RB") & ~3.
	 */
	case op_j:
		newpc = db_register_value(regs, insn.jump_format.rb) & ~3;
		break;

	/*
	 * Branch format: target PC is
	 *	(new PC) + (4 * sign-ext(displacement)).
	 */
	case op_br:
	case op_fbeq:
	case op_fblt:
	case op_fble:
	case op_bsr:
	case op_fbne:
	case op_fbge:
	case op_fbgt:
	case op_blbc:
	case op_beq:
	case op_blt:
	case op_ble:
	case op_blbs:
	case op_bne:
	case op_bge:
	case op_bgt:
		signed_immediate = insn.branch_format.displacement;
		newpc = (pc + 4) + (signed_immediate << 2);
		break;

	default:
		printf("DDB: db_inst_branch_taken on non-branch!\n");
		newpc = pc;	/* XXX */
	}

	return (newpc);
}
