/*	$NetBSD: db_output.c,v 1.29.90.1 2009/05/13 17:19:04 jym Exp $	*/

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
 */

/*
 * Printf and character output for debugger.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_output.c,v 1.29.90.1 2009/05/13 17:19:04 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/stdarg.h>

#include <dev/cons.h>

#include <ddb/ddb.h>

/*
 *	Character output - tracks position in line.
 *	To do this correctly, we should know how wide
 *	the output device is - then we could zero
 *	the line position when the output device wraps
 *	around to the start of the next line.
 *
 *	Instead, we count the number of spaces printed
 *	since the last printing character so that we
 *	don't print trailing spaces.  This avoids most
 *	of the wraparounds.
 */

#ifndef	DB_MAX_LINE
#define	DB_MAX_LINE		24	/* maximum line */
#endif	/* DB_MAX_LINE */
#ifndef	DB_MAX_WIDTH
#define DB_MAX_WIDTH		80	/* maximum width */
#endif	/* DB_MAX_WIDTH */

#define DB_MIN_MAX_WIDTH	20	/* minimum max width */
#define DB_MIN_MAX_LINE		3	/* minimum max line */
#define CTRL(c)			((c) & 0xff)

int	db_output_line = 0;			/* output line number */
int	db_tab_stop_width = 8;			/* how wide are tab stops? */
int	db_max_line = DB_MAX_LINE;		/* output max lines */
int	db_max_width = DB_MAX_WIDTH;		/* output line width */

static int	db_output_position = 0;		/* output column */
static int	db_last_non_space = 0;		/* last non-space character */

static void db_more(void);

/*
 * Force pending whitespace.
 */
void
db_force_whitespace(void)
{
	int last_print, next_tab;

	last_print = db_last_non_space;
	while (last_print < db_output_position) {
		next_tab = DB_NEXT_TAB(last_print);
		if (next_tab <= db_output_position) {
			while (last_print < next_tab) { /* DON'T send a tab!! */
				cnputc(' ');
				last_print++;
			}
		} else {
			cnputc(' ');
			last_print++;
		}
	}
	db_last_non_space = db_output_position;
}

static void
db_more(void)
{
	const char *p;
	int quit_output = 0;

	for (p = "--db_more--"; *p; p++)
		cnputc(*p);
	switch(cngetc()) {
	case ' ':
		db_output_line = 0;
		break;
	case 'q':
	case CTRL('c'):
		db_output_line = 0;
		quit_output = 1;
		break;
	default:
		db_output_line--;
		break;
	}
	p = "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b";
	while (*p)
		cnputc(*p++);
	if (quit_output) {
		db_error(0);
		/* NOTREACHED */
	}
}

/*
 * Output character.  Buffer whitespace.
 */
void
db_putchar(int c)
{
	if (db_max_line >= DB_MIN_MAX_LINE && db_output_line >= db_max_line-1)
		db_more();
	if (c > ' ' && c <= '~') {
		/*
		 * Printing character.
		 * If we have spaces to print, print them first.
		 * Use tabs if possible.
		 */
		db_force_whitespace();
		cnputc(c);
		db_output_position++;
		if (db_max_width >= DB_MIN_MAX_WIDTH
		    && db_output_position >= db_max_width) {
			/* auto new line */
			cnputc('\n');
			db_output_position = 0;
			db_last_non_space = 0;
			db_output_line++;
		}
		db_last_non_space = db_output_position;
	} else if (c == '\n') {
		/* Return */
		cnputc(c);
		db_output_position = 0;
		db_last_non_space = 0;
		db_output_line++;
		db_check_interrupt();
	} else if (c == '\t') {
		/* assume tabs every 8 positions */
		db_output_position = DB_NEXT_TAB(db_output_position);
	} else if (c == ' ') {
		/* space */
		db_output_position++;
	} else if (c == '\007') {
		/* bell */
		cnputc(c);
	}
	/* other characters are assumed non-printing */
}

/*
 * Return output position
 */
int
db_print_position(void)
{

	return (db_output_position);
}

/*
 * End line if too long.
 */
void
db_end_line(void)
{

	if (db_output_position >= db_max_width)
		db_printf("\n");
}

/*
 * Replacement for old '%r' kprintf format.
 */
void
db_format_radix(char *buf, size_t bufsiz, quad_t val, int altflag)
{
	const char *fmt;

	if (db_radix == 16) {
		db_format_hex(buf, bufsiz, val, altflag);
		return;
	}

	if (db_radix == 8)
		fmt = altflag ? "-%#qo" : "-%qo";
	else
		fmt = altflag ? "-%#qu" : "-%qu";

	if (val < 0)
		val = -val;
	else
		++fmt;

	snprintf(buf, bufsiz, fmt, val);
}

/*
 * Replacement for old '%z' kprintf format.
 */
void
db_format_hex(char *buf, size_t bufsiz, quad_t val, int altflag)
{
	/* Only use alternate form if val is nonzero. */
	const char *fmt = (altflag && val) ? "-%#qx" : "-%qx";

	if (val < 0)
		val = -val;
	else
		++fmt;

	snprintf(buf, bufsiz, fmt, val);
}
