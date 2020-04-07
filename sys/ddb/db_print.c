/*	$NetBSD: db_print.c,v 1.28 2011/04/12 08:42:12 mrg Exp $	*/

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
 * Miscellaneous printing.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_print.c,v 1.28 2011/04/12 08:42:12 mrg Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <ddb/ddb.h>

/*ARGSUSED*/
void
db_show_regs(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	const struct db_variable *regp;
	struct db_variable vs;
	db_expr_t value, offset;
	const char *name;

	for (regp = db_regs; regp < db_eregs; regp++) {
		vs = *regp;
		vs.modif = modif;
		db_read_variable(&vs, &value);
		db_printf("%-12s%s", vs.name, db_num_to_str(value));
		db_find_xtrn_sym_and_offset((db_addr_t)value, &name, &offset);
		if (name != NULL &&
		    (unsigned int) offset <= db_maxoff && offset != value) {
			db_printf("\t%s", name);
			if (offset != 0) {
				char tbuf[24];

				db_format_radix(tbuf, 24, offset, true);
				db_printf("+%s", tbuf);
			}
		}
		db_printf("\n");
	}
	db_print_loc_and_inst(PC_REGS(DDB_REGS));
}
