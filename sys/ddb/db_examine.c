/*	$NetBSD: db_examine.c,v 1.21.4.2 2002/03/16 16:00:43 jdolecek Exp $	*/

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
 *	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_examine.c,v 1.21.4.2 2002/03/16 16:00:43 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/db_machdep.h>		/* type definitions */

#include <ddb/db_lex.h>
#include <ddb/db_output.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_access.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>

static char	db_examine_format[TOK_STRING_SIZE] = "x";

static void	db_examine(db_addr_t, char *, int);
static void	db_search(db_addr_t, int, db_expr_t, db_expr_t, unsigned int);

/*
 * Examine (print) data.  Syntax is:
 *		x/[bhl][cdiorsuxz]*
 * For example, the command:
 *  	x/bxxxx
 * should print:
 *  	address:  01  23  45  67
 */
/*ARGSUSED*/
void
db_examine_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	if (modif[0] != '\0')
		db_strcpy(db_examine_format, modif);

	if (count == -1)
		count = 1;

	db_examine((db_addr_t) addr, db_examine_format, count);
}

static void
db_examine(db_addr_t addr, char *fmt, int count)
{
	int		i, c;
	db_expr_t	value;
	int		size;
	int		width;
	int		bytes;
	char *		fp;
	char		tbuf[24];

	while (--count >= 0) {
		fp = fmt;
		size = 4;
		width = 12;
		while ((c = *fp++) != 0) {
			if (db_print_position() == 0) {
				/* Always print the address. */
				db_printsym(addr, DB_STGY_ANY, db_printf);
				db_printf(":\t");
				db_prev = addr;
			}
			switch (c) {
			case 'b':	/* byte */
				size = 1;
				width = 4;
				break;
			case 'h':	/* half-word */
				size = 2;
				width = 8;
				break;
			case 'l':	/* long-word */
				size = 4;
				width = 12;
				break;
			case 'L':	/* implementation maximum */
				size = sizeof value;
				width = 12 * (sizeof value / 4);
				break;
			case 'a':	/* address */
				db_printf("= 0x%lx\n", addr);
				break;
			case 'r':	/* signed, current radix */
				value = db_get_value(addr, size, TRUE);
				addr += size;
				db_format_radix(tbuf, 24, value, FALSE);
				db_printf("%-*s", width, tbuf);
				break;
			case 'x':	/* unsigned hex */
				value = db_get_value(addr, size, FALSE);
				addr += size;
				db_printf("%-*lx", width, value);
				break;
			case 'm':	/* hex dump */
				/*
				 * Print off in chunks of size. Try to print 16
				 * bytes at a time into 4 columns. This
				 * loops modify's count extra times in order
				 * to get the nicely formatted lines.
				 */

				bytes = 0;
				do {
					for (i = 0; i < size; i++) {
						value =
 						    db_get_value(addr+bytes, 1,
							FALSE);
						db_printf("%02lx", value);
						bytes++;
						if (!(bytes % 4))
							db_printf(" ");
					}
				} while ((bytes != 16) && count--);
				/* True up the columns before continuing */
				for (i = 4; i >= (bytes / 4); i--)
					db_printf ("\t");
				/* Print chars,  use . for non-printable's. */
				while (bytes--) {
					value = db_get_value(addr, 1, FALSE);
					addr += 1;
					if (value >= ' ' && value <= '~')
						db_printf("%c", (char)value);
					else
						db_printf(".");
				}
				db_printf("\n");
				break;
			case 'z':	/* signed hex */
				value = db_get_value(addr, size, TRUE);
				addr += size;
				db_format_hex(tbuf, 24, value, FALSE);
				db_printf("%-*s", width, tbuf);
				break;
			case 'd':	/* signed decimal */
				value = db_get_value(addr, size, TRUE);
				addr += size;
				db_printf("%-*ld", width, value);
				break;
			case 'u':	/* unsigned decimal */
				value = db_get_value(addr, size, FALSE);
				addr += size;
				db_printf("%-*lu", width, value);
				break;
			case 'o':	/* unsigned octal */
				value = db_get_value(addr, size, FALSE);
				addr += size;
				db_printf("%-*lo", width, value);
				break;
			case 'c':	/* character */
				value = db_get_value(addr, 1, FALSE);
				addr += 1;
				if (value >= ' ' && value <= '~')
					db_printf("%c", (char)value);
				else
					db_printf("\\%03lo", value);
				break;
			case 's':	/* null-terminated string */
				for (;;) {
					value = db_get_value(addr, 1, FALSE);
					addr += 1;
					if (value == 0)
						break;
					if (value >= ' ' && value <= '~')
						db_printf("%c", (char)value);
					else
						db_printf("\\%03lo", value);
				}
				break;
			case 'i':	/* instruction */
				addr = db_disasm(addr, FALSE);
				break;
			case 'I':	/* instruction, alternate form */
				addr = db_disasm(addr, TRUE);
				break;
			default:
				break;
			}
			if (db_print_position() != 0)
				db_end_line();
		}
	}
	db_next = addr;
}

/*
 * Print value.
 */
static char	db_print_format = 'x';

/*ARGSUSED*/
void
db_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	db_expr_t	value;

	if (modif[0] != '\0')
		db_print_format = modif[0];

	switch (db_print_format) {
	case 'a':
		db_printsym((db_addr_t)addr, DB_STGY_ANY, db_printf);
		break;
	case 'r':
		{
			char tbuf[24];

			db_format_radix(tbuf, 24, addr, FALSE);
			db_printf("%11s", tbuf);
			break;
		}
	case 'x':
		db_printf("%8lx", addr);
		break;
	case 'z':
		{
			char tbuf[24];

			db_format_hex(tbuf, 24, addr, FALSE);
			db_printf("%8s", tbuf);
			break;
		}
	case 'd':
		db_printf("%11ld", addr);
		break;
	case 'u':
		db_printf("%11lu", addr);
		break;
	case 'o':
		db_printf("%16lo", addr);
		break;
	case 'c':
		value = addr & 0xFF;
		if (value >= ' ' && value <= '~')
			db_printf("%c", (char)value);
		else
			db_printf("\\%03lo", value);
		break;
	}
	db_printf("\n");
}

void
db_print_loc_and_inst(db_addr_t loc)
{

	db_printsym(loc, DB_STGY_PROC, db_printf);
	db_printf(":\t");
	(void) db_disasm(loc, FALSE);
}

void
db_strcpy(char *dst, char *src)
{

	while ((*dst++ = *src++) != '\0')
		;
}

/*
 * Search for a value in memory.
 * Syntax: search [/bhl] addr value [mask] [,count]
 */
/*ARGSUSED*/
void
db_search_cmd(db_expr_t daddr, int have_addr, db_expr_t dcount, char *modif)
{
	int		t;
	db_addr_t	addr;
	int		size;
	db_expr_t	value;
	db_expr_t	mask;
	db_expr_t	count;

	t = db_read_token();
	if (t == tSLASH) {
		t = db_read_token();
		if (t != tIDENT) {
			bad_modifier:
			db_printf("Bad modifier\n");
			db_flush_lex();
			return;
		}

		if (!strcmp(db_tok_string, "b"))
			size = 1;
		else if (!strcmp(db_tok_string, "h"))
			size = 2;
		else if (!strcmp(db_tok_string, "l"))
			size = 4;
		else
			goto bad_modifier;
	} else {
		db_unread_token(t);
		size = 4;
	}

	if (!db_expression(&value)) {
		db_printf("Address missing\n");
		db_flush_lex();
		return;
	}
	addr = (db_addr_t) value;

	if (!db_expression(&value)) {
		db_printf("Value missing\n");
		db_flush_lex();
		return;
	}

	if (!db_expression(&mask))
		mask = (int) ~0;

	t = db_read_token();
	if (t == tCOMMA) {
		if (!db_expression(&count)) {
			db_printf("Count missing\n");
			db_flush_lex();
			return;
		}
	} else {
		db_unread_token(t);
		count = -1;		/* effectively forever */
	}
	db_skip_to_eol();

	db_search(addr, size, value, mask, count);
}

static void
db_search(db_addr_t addr, int size, db_expr_t value, db_expr_t mask,
    unsigned int count)
{
	while (count-- != 0) {
		db_prev = addr;
		if ((db_get_value(addr, size, FALSE) & mask) == value)
			break;
		addr += size;
	}
	db_next = addr;
}
