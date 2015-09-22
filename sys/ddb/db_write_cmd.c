/*	$NetBSD: db_write_cmd.c,v 1.26.40.1 2015/09/22 12:05:56 skrll Exp $	*/

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
 *	Author: David B. Golub,  Carnegie Mellon University
 *	Date:	7/90
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_write_cmd.c,v 1.26.40.1 2015/09/22 12:05:56 skrll Exp $");

#include <sys/param.h>
#include <sys/proc.h>

#include <ddb/ddb.h>

/*
 * Write to file.
 */
/*ARGSUSED*/
void
db_write_cmd(db_expr_t address, bool have_addr,
    db_expr_t count, const char *modif)
{
	db_addr_t	addr;
	db_expr_t	old_value;
	db_expr_t	new_value;
	int		size;
	bool		wrote_one;
	bool		show_old_val;

	addr = (db_addr_t) address;
	wrote_one = false;
	show_old_val = islower((unsigned char)modif[0]);

	switch (tolower((unsigned char)modif[0])) {
	case 'b':
		size = 1;
		break;
	case 'h':
		size = 2;
		break;
	case 'l':
	case '\0':
		size = 4;
		break;
	case 'q':
		if (sizeof(db_expr_t) != sizeof(uint64_t)) {
			size = -1;
			db_error("q not supported\n");
			/*NOTREACHED*/
		}
	case 'L':
		size = sizeof(db_expr_t);
		break;
	default:
		size = -1;
		db_error("Unknown size\n");
		/*NOTREACHED*/
	}

	while (db_expression(&new_value)) {
		db_printsym(addr, DB_STGY_ANY, db_printf);
		if (show_old_val) {
			old_value = db_get_value(addr, size, false);
			db_printf("\t\t%s = ", db_num_to_str(old_value));
			db_printf("%s\n", db_num_to_str(new_value));
		}
		else
			db_printf("\t\t= %s\n", db_num_to_str(new_value));
		db_put_value(addr, size, new_value);
		addr += size;

		wrote_one = true;
	}

	if (!wrote_one) {
		db_error("Nothing written.\n");
		/*NOTREACHED*/
	}

	db_next = addr;
	db_prev = addr - size;

	db_skip_to_eol();
}
