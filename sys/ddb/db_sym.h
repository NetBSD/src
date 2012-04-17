/*	$NetBSD: db_sym.h,v 1.24.4.1 2012/04/17 00:07:24 yamt Exp $	*/

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
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	8/90
 */
#include <sys/ksyms.h>

typedef vaddr_t db_sym_t;
#define	DB_SYM_NULL	((db_sym_t)0)
typedef int		db_strategy_t;	/* search strategy */

#define	DB_STGY_ANY	(KSYMS_ANY|KSYMS_CLOSEST)    /* anything goes */
#define DB_STGY_XTRN	(KSYMS_EXTERN|KSYMS_CLOSEST) /* only external symbols */
#define DB_STGY_PROC	(KSYMS_PROC|KSYMS_CLOSEST)   /* only procedures */

#ifndef _KERNEL
/*
 * These structures and functions are not used in the kernel, but only
 * in crash(8).
 */
typedef struct {
	const char	*name;		/* symtab name */
	char		*start;		/* symtab location */
	char		*end;
	char		*private;	/* optional machdep pointer */
} db_symtab_t;

/*
 * Internal db_forall function calling convention:
 *
 * (*db_forall_func)(stab, sym, name, suffix, prefix, arg);
 *
 * stab is the symbol table, symbol the (opaque) symbol pointer,
 * name the name of the symbol, suffix a string representing
 * the type, prefix an initial ignorable function prefix (e.g. "_"
 * in a.out), and arg an opaque argument to be passed in.
 */
typedef void (db_forall_func_t)
	(db_symtab_t *, db_sym_t, char *, char *, int, void *);

/*
 * A symbol table may be in one of many formats.  All symbol tables
 * must be of the same format as the master kernel symbol table.
 */
typedef struct {
	const char     *sym_format;
	bool		(*sym_init)(int, void *, void *, const char *);
	db_sym_t	(*sym_lookup)(db_symtab_t *, const char *);
	db_sym_t	(*sym_search)(db_symtab_t *, db_addr_t, db_strategy_t,
			    db_expr_t *);
	void		(*sym_value)(db_symtab_t *, db_sym_t, const char **,
			    db_expr_t *);
	bool		(*sym_line_at_pc)(db_symtab_t *, db_sym_t, char **,
			    int *, db_expr_t);
	bool		(*sym_numargs)(db_symtab_t *, db_sym_t, int *, char **);
	void		(*sym_forall)(db_symtab_t *,
			    db_forall_func_t *db_forall_func, void *);
} db_symformat_t;
#endif

extern unsigned int db_maxoff;		/* like gdb's "max-symbolic-offset" */

/*
 * Functions exported by the symtable module
 */
bool		db_eqname(const char *, const char *, int);
					/* strcmp, modulo leading char */

bool		db_value_of_name(const char *, db_expr_t *);
					/* find symbol value given name */

void		db_sifting(char *, int);
				/* print partially matching symbol names */

db_sym_t	db_search_symbol(db_addr_t, db_strategy_t, db_expr_t *);
					/* find symbol given value */

void		db_symbol_values(db_sym_t, const char **, db_expr_t *);
					/* return name and value of symbol */

#define db_find_sym_and_offset(val,namep,offp)	\
	db_symbol_values(db_search_symbol(val,DB_STGY_ANY,offp),namep,0)
					/* find name&value given approx val */

#define db_find_xtrn_sym_and_offset(val,namep,offp)	\
	db_symbol_values(db_search_symbol(val,DB_STGY_XTRN,offp),namep,0)
					/* ditto, but no locals */

void		db_symstr(char *, size_t, db_expr_t, db_strategy_t);
void		db_printsym(db_expr_t, db_strategy_t,
    void(*)(const char *, ...) __printflike(1, 2));
					/* print closest symbol to a value */
bool		db_sym_numargs(db_sym_t, int *, char **);
