/*	$NetBSD: db_output.h,v 1.15 2000/08/09 19:51:46 tv Exp $	*/

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
 *	Date:	8/90
 */

/*
 * Printing routines for kernel debugger.
 */
void db_force_whitespace __P((void));
void db_putchar __P((int));
int db_print_position __P((void));
void db_printf __P((const char *, ...))
    __attribute__((__format__(__printf__,1,2)));
void db_format_radix __P((char *, size_t, quad_t, int));
void db_format_hex __P((char *, size_t, quad_t, int));
void db_end_line __P((void));

extern int	db_radix;
extern int	db_max_width;
extern int	db_tab_stop_width;
extern int	db_max_line;

#define	DB_NEXT_TAB(i) \
	((((i) + db_tab_stop_width) / db_tab_stop_width) * db_tab_stop_width)
