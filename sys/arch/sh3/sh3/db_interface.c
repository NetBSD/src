/*	$NetBSD: db_interface.c,v 1.2 2000/06/29 07:44:04 mrg Exp $	*/

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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS
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
 * Interface to new debugger.
 */
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/cpufunc.h>
#include <machine/db_machdep.h>
#include <sh3/sh_opcode.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>


extern label_t	*db_recover;
extern char *trap_type[];
extern int trap_types;

int	db_active = 0;

void kdbprinttrap __P((int, int));

/*
 * Print trap reason.
 */
void
kdbprinttrap(type, code)
	int type, code;
{
	db_printf("kernel: ");
	if (type >= trap_types || type < 0)
		db_printf("type %d", type);
	else
		db_printf("%s", trap_type[type]);
	db_printf(" trap, code=%x\n", code);
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(type, code, regs)
	int type, code;
	db_regs_t *regs;
{
	int s;

#if 0
	if ((boothowto&RB_KDB) == 0)
		return(0);
#endif

	switch (type) {
	case T_NMI:	/* NMI interrupt */
	case T_USERBREAK:	/* breakpoint */
	case -1:	/* keyboard interrupt */
		break;
	default:
		kdbprinttrap(type, code);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* XXX Should switch to kdb`s own stack here. */

	ddb_regs = *regs;

	s = splhigh();
	db_active++;
	cnpollc(TRUE);
	db_trap(type, code);
	cnpollc(FALSE);
	db_active--;
	splx(s);

	/*  write ddb_regs back to trapframe */
	*regs = ddb_regs;

	return (1);
}

void
Debugger()
{
	breakpoint();
}

boolean_t
inst_branch(ins)
	int ins;
{
	InstFmt i;
	int delay = 0;

	i.word = (unsigned short)(ins & 0x0000ffff);
	switch (i.oType.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}

	switch (i.nType.op1) {
		case OP1_BRAF: /* or OP1_BSRF */
		switch (i.nType.op2) {
		case OP2_BRAF:
		case OP2_BSRF:
			delay = 1;
			return delay;
		}
	}

#if 0
	switch (i.mType.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}

	switch (i.nmType.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}
#endif

	switch (i.mdType.op) {
		case OP_BF:
		case OP_BT:
		delay = 0;
		return delay;
		case OP_BFS:
		case OP_BTS:
		delay = 1;
		return delay;
	}

#if 0
	switch (i.nd4Type.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}
#endif

	switch (i.nmdType.op) {
		case OP_BRA:
		delay = 1;
		return delay;
	}

#if 0
	switch (i.dType.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}

	switch (i.d12Type.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}

	switch (i.nd8Type.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}

	switch (i.iType.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}

	switch (i.niType.op) {
		case OP_RTS:
		delay = 1;
		return delay;
	}
#endif

	return delay;
}
