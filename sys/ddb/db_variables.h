/*	$NetBSD: db_variables.h,v 1.11.2.1 2002/03/16 16:00:45 jdolecek Exp $	*/

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

#ifndef	_DB_VARIABLES_H_
#define	_DB_VARIABLES_H_

/*
 * Debugger variables.
 */
struct db_variable {
	const char *name;	/* Name of variable */
	long	*valuep;	/* value of variable */
				/* function to call when reading/writing */
	int	(*fcn)(const struct db_variable *, db_expr_t *, int);
	char	*modif;
#define DB_VAR_GET	0
#define DB_VAR_SET	1
};
#define	FCN_NULL ((int (*)(const struct db_variable *, db_expr_t *, int))0)

extern const struct db_variable	db_vars[];	/* debugger variables */
extern const struct db_variable	* const db_evars;
extern const struct db_variable	db_regs[];	/* machine registers */
extern const struct db_variable	* const db_eregs;

int	db_get_variable(db_expr_t *);
int	db_set_variable(db_expr_t);
void	db_read_variable(const struct db_variable *, db_expr_t *);
void	db_write_variable(const struct db_variable *, db_expr_t *);
void	db_set_cmd(db_expr_t, int, db_expr_t, char *);

#endif	/* _DB_VARIABLES_H_ */
