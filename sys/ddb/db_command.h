/*	$NetBSD: db_command.h,v 1.20.2.2 2002/02/28 04:13:05 nathanw Exp $	*/

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

/*
 * Command loop declarations.
 */
struct db_command;

void	db_skip_to_eol(void);
void	db_command_loop(void);
void	db_error(char *);

extern db_addr_t db_dot;	/* current location */
extern db_addr_t db_last_addr;	/* last explicit address typed */
extern db_addr_t db_prev;	/* last address examined
				   or written */
extern db_addr_t db_next;	/* next address to be examined
				   or written */

/*
 * Command table
 */
struct db_command {
	const char	*name;		/* command name */
	/* function to call */
	void		(*fcn)(db_expr_t, int, db_expr_t, char *);
	int		flag;		/* extra info: */
#define	CS_OWN		0x1		/* non-standard syntax */
#define	CS_MORE		0x2		/* standard syntax, but may have other
					   words at end */
#define	CS_SET_DOT	0x100		/* set dot after command */
	const struct db_command *more;	/* another level of command */
};
