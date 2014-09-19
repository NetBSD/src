/*	$NetBSD: db_run.c,v 1.33 2014/09/19 17:29:01 matt Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1993-1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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
 * 	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

/*
 * Commands to run process.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_run.c,v 1.33 2014/09/19 17:29:01 matt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>

#include <machine/db_machdep.h>

#include <ddb/db_run.h>
#include <ddb/db_access.h>
#include <ddb/db_break.h>

int	db_inst_count;
int	db_load_count;
int	db_store_count;

#ifdef	SOFTWARE_SSTEP
static void	db_set_temp_breakpoint(db_breakpoint_t, db_addr_t);
static void	db_delete_temp_breakpoint(db_breakpoint_t);
static struct	db_breakpoint	db_not_taken_bkpt;
static struct	db_breakpoint	db_taken_bkpt;
#endif

#if defined(DDB)
#include <ddb/db_lex.h>
#include <ddb/db_watch.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

static int	db_run_mode;
#define	STEP_NONE	0
#define	STEP_ONCE	1
#define	STEP_RETURN	2
#define	STEP_CALLT	3
#define	STEP_CONTINUE	4
#define STEP_INVISIBLE	5
#define	STEP_COUNT	6

static bool		db_sstep_print;
static int		db_loop_count;
static int		db_call_depth;

bool
db_stop_at_pc(db_regs_t *regs, bool *is_breakpoint)
{
	db_addr_t	pc;
	db_breakpoint_t bkpt;

	pc = PC_REGS(regs);

#ifdef	FIXUP_PC_AFTER_BREAK
	if (*is_breakpoint) {
		/*
		 * Breakpoint trap.  Regardless if we treat this as a
		 * real breakpoint (e.g. software single-step), fix up the PC.
		 */
		FIXUP_PC_AFTER_BREAK(regs);
		pc = PC_REGS(regs);
	}
#endif

#ifdef	SOFTWARE_SSTEP
	/*
	 * If we stopped at one of the single-step breakpoints, say it's not
	 * really a breakpoint so that we don't skip over the real instruction.
	 */
	if (db_taken_bkpt.address == pc || db_not_taken_bkpt.address == pc)
		*is_breakpoint = false;
#endif	/* SOFTWARE_SSTEP */

	db_clear_single_step(regs);
	db_clear_breakpoints();
	db_clear_watchpoints();

	/*
	 * Now check for a breakpoint at this address.
	 */
	bkpt = db_find_breakpoint_here(pc);
	if (bkpt) {
		if (--bkpt->count == 0) {
			bkpt->count = bkpt->init_count;
			*is_breakpoint = true;
			return (true);	/* stop here */
		}
	} else if (*is_breakpoint) {
#ifdef PC_ADVANCE
		PC_ADVANCE(regs);
#else
		PC_REGS(regs) += BKPT_SIZE;
#endif
	}

	*is_breakpoint = false;

	if (db_run_mode == STEP_INVISIBLE) {
		db_run_mode = STEP_CONTINUE;
		return (false);	/* continue */
	}
	if (db_run_mode == STEP_COUNT) {
		return (false); /* continue */
	}
	if (db_run_mode == STEP_ONCE) {
		if (--db_loop_count > 0) {
			if (db_sstep_print) {
				db_printf("\t\t");
				db_print_loc_and_inst(pc);
				db_printf("\n");
			}
			return (false);	/* continue */
		}
	}
	if (db_run_mode == STEP_RETURN) {
		db_expr_t ins = db_get_value(pc, sizeof(int), false);

		/* continue until matching return */

		if (!inst_trap_return(ins) &&
		    (!inst_return(ins) || --db_call_depth != 0)) {
			if (db_sstep_print) {
				if (inst_call(ins) || inst_return(ins)) {
					int i;

					db_printf("[after %6d]     ",
					    db_inst_count);
					for (i = db_call_depth; --i > 0; )
						db_printf("  ");
					db_print_loc_and_inst(pc);
					db_printf("\n");
				}
			}
			if (inst_call(ins))
				db_call_depth++;
			return (false);	/* continue */
		}
	}
	if (db_run_mode == STEP_CALLT) {
		db_expr_t ins = db_get_value(pc, sizeof(int), false);

		/* continue until call or return */

		if (!inst_call(ins) &&
		    !inst_return(ins) &&
		    !inst_trap_return(ins)) {
			return (false);	/* continue */
		}
	}
	db_run_mode = STEP_NONE;
	return (true);
}

void
db_restart_at_pc(db_regs_t *regs, bool watchpt)
{
	db_addr_t pc = PC_REGS(regs);
#ifdef SOFTWARE_SSTEP
	db_addr_t brpc;
#endif

	if ((db_run_mode == STEP_COUNT) ||
	    (db_run_mode == STEP_RETURN) ||
	    (db_run_mode == STEP_CALLT)) {
		db_expr_t		ins __unused;

		/*
		 * We are about to execute this instruction,
		 * so count it now.
		 */
		ins = db_get_value(pc, sizeof(int), false);
		db_inst_count++;
		db_load_count += inst_load(ins);
		db_store_count += inst_store(ins);

#ifdef SOFTWARE_SSTEP
		/*
		 * Account for instructions in delay slots.
		 */
		brpc = next_instr_address(pc, true);
		if ((brpc != pc) &&
		    (inst_branch(ins) || inst_call(ins) || inst_return(ins))) {
			ins = db_get_value(brpc, sizeof(int), false);
			db_inst_count++;
			db_load_count += inst_load(ins);
			db_store_count += inst_store(ins);
		}
#endif
	}

	if (db_run_mode == STEP_CONTINUE) {
		if (watchpt || db_find_breakpoint_here(pc)) {
			/*
			 * Step over breakpoint/watchpoint.
			 */
			db_run_mode = STEP_INVISIBLE;
			db_set_single_step(regs);
		} else {
			db_set_breakpoints();
			db_set_watchpoints();
		}
	} else {
		db_set_single_step(regs);
	}
}

void
db_single_step(db_regs_t *regs)
{

	if (db_run_mode == STEP_CONTINUE) {
		db_run_mode = STEP_INVISIBLE;
		db_set_single_step(regs);
	}
}

/* single-step */
/*ARGSUSED*/
void
db_single_step_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool print = false;

	if (count == -1)
		count = 1;

	if (modif[0] == 'p')
		print = true;

	db_run_mode = STEP_ONCE;
	db_loop_count = count;
	db_sstep_print = print;
	db_inst_count = 0;
	db_load_count = 0;
	db_store_count = 0;

	db_cmd_loop_done = true;
}

/* trace and print until call/return */
/*ARGSUSED*/
void
db_trace_until_call_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool print = false;

	if (modif[0] == 'p')
		print = true;

	db_run_mode = STEP_CALLT;
	db_sstep_print = print;
	db_inst_count = 0;
	db_load_count = 0;
	db_store_count = 0;

	db_cmd_loop_done = true;
}

/*ARGSUSED*/
void
db_trace_until_matching_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool print = false;

	if (modif[0] == 'p')
		print = true;

	db_run_mode = STEP_RETURN;
	db_call_depth = 1;
	db_sstep_print = print;
	db_inst_count = 0;
	db_load_count = 0;
	db_store_count = 0;

	db_cmd_loop_done = true;
}

/* continue */
/*ARGSUSED*/
void
db_continue_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

	if (modif[0] == 'c')
		db_run_mode = STEP_COUNT;
	else
		db_run_mode = STEP_CONTINUE;
	db_inst_count = 0;
	db_load_count = 0;
	db_store_count = 0;

	db_cmd_loop_done = true;
}
#endif /* DDB */

#ifdef SOFTWARE_SSTEP
/*
 *	Software implementation of single-stepping.
 *	If your machine does not have a trace mode
 *	similar to the vax or sun ones you can use
 *	this implementation, done for the mips.
 *	Just define the above conditional and provide
 *	the functions/macros defined below.
 *
 * bool inst_branch(int inst)
 * bool inst_call(int inst)
 *	returns true if the instruction might branch
 *
 * bool inst_return(int inst)
 *	returns true is the instruction will return to its caller
 *
 * bool inst_unconditional_flow_transfer(int inst)
 *	returns true if the instruction is an unconditional
 *	transter of flow (i.e. unconditional branch)
 *
 * db_addr_t branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
 *	returns the target address of the branch
 *
 * db_addr_t next_instr_address(db_addr_t pc, bool bd)
 *	returns the address of the first instruction following the
 *	one at "pc", which is either in the taken path of the branch
 *	(bd == true) or not.  This is for machines (e.g. mips) with
 *	branch delays.
 *
 *	A single-step may involve at most 2 breakpoints -
 *	one for branch-not-taken and one for branch taken.
 *	If one of these addresses does not already have a breakpoint,
 *	we allocate a breakpoint and save it here.
 *	These breakpoints are deleted on return.
 */

#if !defined(DDB)
/* XXX - don't check for existing breakpoints in KGDB-only case */
#define db_find_breakpoint_here(pc)	(0)
#endif

void
db_set_single_step(db_regs_t *regs)
{
	db_addr_t pc = PC_REGS(regs), brpc = pc;
	bool unconditional;
	unsigned int inst;

	/*
	 *	User was stopped at pc, e.g. the instruction
	 *	at pc was not executed.
	 */
	inst = db_get_value(pc, sizeof(int), false);
	if (inst_branch(inst) || inst_call(inst) || inst_return(inst)) {
		brpc = branch_taken(inst, pc, regs);
		if (brpc != pc) {	/* self-branches are hopeless */
			db_set_temp_breakpoint(&db_taken_bkpt, brpc);
		} else
			db_taken_bkpt.address = 0;
		pc = next_instr_address(pc, true);
	}

	/*
	 *	Check if this control flow instruction is an
	 *	unconditional transfer.
	 */
	unconditional = inst_unconditional_flow_transfer(inst);

	pc = next_instr_address(pc, false);

	/*
	 *	We only set the sequential breakpoint if previous
	 *	instruction was not an unconditional change of flow
	 *	control.  If the previous instruction is an
	 *	unconditional change of flow control, setting a
	 *	breakpoint in the next sequential location may set
	 *	a breakpoint in data or in another routine, which
	 *	could screw up in either the program or the debugger.
	 *	(Consider, for instance, that the next sequential
	 *	instruction is the start of a routine needed by the
	 *	debugger.)
	 *
	 *	Also, don't set both the taken and not-taken breakpoints
	 *	in the same place even if the MD code would otherwise
	 *	have us do so.
	 */
	if (unconditional == false &&
	    db_find_breakpoint_here(pc) == 0 &&
	    pc != brpc)
		db_set_temp_breakpoint(&db_not_taken_bkpt, pc);
	else
		db_not_taken_bkpt.address = 0;
}

void
db_clear_single_step(db_regs_t *regs)
{

	if (db_taken_bkpt.address != 0)
		db_delete_temp_breakpoint(&db_taken_bkpt);

	if (db_not_taken_bkpt.address != 0)
		db_delete_temp_breakpoint(&db_not_taken_bkpt);
}

void
db_set_temp_breakpoint(db_breakpoint_t bkpt, db_addr_t addr)
{

	bkpt->map = NULL;
	bkpt->address = addr;
	/* bkpt->flags = BKPT_TEMP;	- this is not used */
	bkpt->init_count = 1;
	bkpt->count = 1;

	bkpt->bkpt_inst = db_get_value(bkpt->address, BKPT_SIZE, false);
	db_put_value(bkpt->address, BKPT_SIZE,
		BKPT_SET(bkpt->bkpt_inst, bkpt->address));
}

void
db_delete_temp_breakpoint(db_breakpoint_t bkpt)
{

	db_put_value(bkpt->address, BKPT_SIZE, bkpt->bkpt_inst);
	bkpt->address = 0;
}
#endif /* SOFTWARE_SSTEP */
