/*	$NetBSD: db_trap.c,v 1.17.4.2 2002/03/16 16:00:45 jdolecek Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 * Trap entry point to kernel debugger.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trap.c,v 1.17.4.2 2002/03/16 16:00:45 jdolecek Exp $");

#include <sys/param.h>
#include <sys/proc.h>

#include <machine/db_machdep.h>

#include <ddb/db_run.h>
#include <ddb/db_command.h>
#include <ddb/db_break.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

/*
 * db_trap_callback can be hooked by MD port code to handle special
 * cases such as disabling hardware watchdogs while in DDB.
 */
void (*db_trap_callback)(int);

void
db_trap(int type, int code)
{
	boolean_t	bkpt;
	boolean_t	watchpt;

	bkpt = IS_BREAKPOINT_TRAP(type, code);
	watchpt = IS_WATCHPOINT_TRAP(type, code);

	if (db_trap_callback) db_trap_callback(1);

	if (db_stop_at_pc(DDB_REGS, &bkpt)) {
		if (db_inst_count) {
			db_printf("After %d instructions "
			    "(%d loads, %d stores),\n",
			    db_inst_count, db_load_count, db_store_count);
		}
		if (curproc != NULL) {
			if (bkpt)
				db_printf("Breakpoint");
			else if (watchpt)
				db_printf("Watchpoint");
			else
				db_printf("Stopped");
			db_printf(" in pid %d (%s) at\t", curproc->p_pid,
				curproc->p_comm);
		} else if (bkpt)
			db_printf("Breakpoint at\t");
		else if (watchpt)
			db_printf("Watchpoint at\t");
		else
			db_printf("Stopped at\t");
		db_dot = PC_REGS(DDB_REGS);
		db_print_loc_and_inst(db_dot);

		db_command_loop();
	}

	db_restart_at_pc(DDB_REGS, watchpt);

	if (db_trap_callback)
		db_trap_callback(0);
}
