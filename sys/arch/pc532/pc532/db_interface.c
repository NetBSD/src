/*	$NetBSD: db_interface.c,v 1.10 2002/05/13 20:30:09 matt Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1992 Helsinki University of Technology
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND HELSINKI UNIVERSITY OF TECHNOLOGY ALLOW FREE USE
 * OF THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * HELSINKI UNIVERSITY OF TECHNOLOGY DISCLAIM ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */
/*
 * 	File: ns532/db_interface.c
 *	Author: Tero Kivinen, Helsinki University of Technology 1992.
 *
 *	Interface to new kernel debugger.
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>


extern char *	trap_type[];
extern int	trap_types;

int	db_active = 0;
int	db_active_ipl;

static void	kdbprinttrap __P((int, int));

/*
 * Print trap reason.
 */
static void
kdbprinttrap(type, code)
	int	type, code;
{
	printf("kernel: ");
	if (type >= trap_types || type < 0)
		db_printf("type %d", type);
	else
		db_printf("%s", trap_type[type]);
	printf(" trap, code=%x\n", code);
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(type, code, regs)
	int	type, code;
	register db_regs_t *regs;
{
	switch (type) {
	case T_BPT:		/* breakpoint */
	case T_WATCHPOINT:	/* watchpoint */
	case T_TRC:		/* trace step */
	case T_DBG:		/* hardware debug trap */
	case -1:		/* keyboard interrupt */
		break;
		
	default:
		kdbprinttrap(type, code);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
		return(0);
	}
	
	ddb_regs = *regs;
	db_active++;
	cnpollc(TRUE);

	db_trap(type, code);
	if ((type = T_BPT) &&
	    (db_get_value(PC_REGS(DDB_REGS), BKPT_SIZE, FALSE) == BKPT_INST))
		PC_REGS(DDB_REGS) += BKPT_SIZE;
	
	cnpollc(FALSE);
	db_active--;
	*regs = ddb_regs;

	return (1);
}
