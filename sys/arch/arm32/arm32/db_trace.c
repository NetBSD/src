/* $NetBSD: db_trace.c,v 1.1 1996/02/15 22:37:25 mark Exp $ */

/* 
 * Copyright (c) 1996 Scott K. Stevens
 *
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
 * $Id: db_trace.c,v 1.1 1996/02/15 22:37:25 mark Exp $
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
 
#define INKERNEL(va)	(((vm_offset_t)(va)) >= USRSTACK)

void
db_stack_trace_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{
	struct frame	*frame;
	boolean_t	kernel_only = TRUE;

	{
		register char c, *cp = modif;
		while ((c = *cp++) != 0)
			if (c == 'u')
				kernel_only = FALSE;
	}

	if (count == -1)
		count = 65535;

	return;
#if 0

	if (!have_addr)
		frame = (struct frame *)DDB_TF->tf_r11;
	else
		frame = (struct frame *)addr;

	while (count--) {
		int		i;
		db_expr_t	offset;
		db_sym_t	sym;
		char		*name;
		db_addr_t	pc;

		pc = frame->fr_pc;
		if (!INKERNEL(pc))
			break;

		db_find_sym_and_offset(pc, &name, &offset);
		if (name == NULL)
			name = "?";

		db_printf("%s(", name);

		/*
		 * Switch to next frame up
		 */
		frame = frame->fr_fp;

		db_printsym(pc, DB_STGY_PROC);
		db_printf("\n");

	}
#endif
}
