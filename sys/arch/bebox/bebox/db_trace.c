/*	$NetBSD: db_trace.c,v 1.3 1997/12/18 09:09:01 sakamoto Exp $	*/
/*	$OpenBSD: db_trace.c,v 1.3 1997/03/21 02:10:48 niklas Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
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
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/proc.h>

#include <machine/db_machdep.h>
#include <machine/pmap.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>

struct db_variable db_regs[] = {
	{ "r1",  (long *)&ddb_regs.r1,	FCN_NULL },
	{ "r2",  (long *)&ddb_regs.r2,	FCN_NULL },
	{ "r3",  (long *)&ddb_regs.r3,	FCN_NULL },
	{ "r4",  (long *)&ddb_regs.r4,	FCN_NULL },
	{ "r5",  (long *)&ddb_regs.r5,	FCN_NULL },
	{ "r6",  (long *)&ddb_regs.r6,	FCN_NULL },
	{ "r7",  (long *)&ddb_regs.r7,	FCN_NULL },
	{ "r8",  (long *)&ddb_regs.r8,	FCN_NULL },
	{ "r9",  (long *)&ddb_regs.r9,	FCN_NULL },
	{ "r10", (long *)&ddb_regs.r10,	FCN_NULL },
	{ "r11", (long *)&ddb_regs.r11,	FCN_NULL },
	{ "r12", (long *)&ddb_regs.r12,	FCN_NULL },
	{ "r13", (long *)&ddb_regs.r13,	FCN_NULL },
	{ "r14", (long *)&ddb_regs.r13,	FCN_NULL },
	{ "r15", (long *)&ddb_regs.r13,	FCN_NULL },
	{ "r16", (long *)&ddb_regs.r13,	FCN_NULL },
	{ "r17", (long *)&ddb_regs.r17,	FCN_NULL },
	{ "r18", (long *)&ddb_regs.r18,	FCN_NULL },
	{ "r19", (long *)&ddb_regs.r19,	FCN_NULL },
	{ "r20", (long *)&ddb_regs.r20,	FCN_NULL },
	{ "r21", (long *)&ddb_regs.r21,	FCN_NULL },
	{ "r22", (long *)&ddb_regs.r22,	FCN_NULL },
	{ "r23", (long *)&ddb_regs.r23,	FCN_NULL },
	{ "r24", (long *)&ddb_regs.r24,	FCN_NULL },
	{ "r25", (long *)&ddb_regs.r25,	FCN_NULL },
	{ "r26", (long *)&ddb_regs.r26,	FCN_NULL },
	{ "r27", (long *)&ddb_regs.r27,	FCN_NULL },
	{ "r28", (long *)&ddb_regs.r28,	FCN_NULL },
	{ "r29", (long *)&ddb_regs.r29,	FCN_NULL },
	{ "r30", (long *)&ddb_regs.r30,	FCN_NULL },
	{ "r31", (long *)&ddb_regs.r31,	FCN_NULL },
	{ "r32", (long *)&ddb_regs.r32,	FCN_NULL },
	{ "iar", (long *)&ddb_regs.iar,	FCN_NULL },
};
struct db_variable *db_eregs = db_regs + sizeof (db_regs)/sizeof (db_regs[0]);

extern label_t	*db_recover;

/*
 *	Frame tracing.
 */
void
db_stack_trace_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	db_addr_t frame, lr, caller;
	db_expr_t diff;
	db_sym_t sym;
	char *symname;
	boolean_t kernel_only = TRUE;
	boolean_t trace_thread = FALSE;

	{
		register char *cp = modif;
		register char c;

		while ((c = *cp++) != 0) {
			if (c == 't')
				trace_thread = TRUE;
			if (c == 'u')
				kernel_only = FALSE;
		}
	}

	if (count == -1)
		count = 65535;

	frame = (db_addr_t)ddb_regs.r1;
	while ((frame = *(db_addr_t *)frame) && count--) {
		lr = *(db_addr_t *)(frame + 4) - 4;
		if (lr & 3) {
			db_printf("saved LR(0x%x) is invalid.", lr);
			break;
		}
		if ((caller = (db_addr_t)vtophys(lr)) == 0)
			caller = lr;

		diff = 0;
		symname = NULL;
		sym = db_search_symbol(caller, DB_STGY_ANY, &diff);
		db_symbol_values(sym, &symname, 0);
		if (symname == NULL)
			symname = "?";
		db_printf("at %s+%x\n", symname, diff);
	}
}
