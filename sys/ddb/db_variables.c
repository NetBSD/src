/*	$NetBSD: db_variables.c,v 1.16 1999/04/12 20:38:21 pk Exp $	*/

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

#include "opt_ddb.h"				/* for sysctl.h */
#include "opt_ddbparam.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <vm/vm.h>
#include <sys/sysctl.h>

#include <machine/db_machdep.h>

#include <ddb/ddbvar.h>

#include <ddb/db_lex.h>
#include <ddb/db_variables.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>


/*
 * If this is non-zero, the DDB will be entered when the system
 * panics.  Initialize it so that it's patchable.
 */
#ifndef DDB_ONPANIC
#define DDB_ONPANIC	1
#endif
int		db_onpanic = DDB_ONPANIC;

/*
 * Can  DDB can be entered from the console?
 */
#ifndef	DDB_FROMCONSOLE
#define	DDB_FROMCONSOLE 1
#endif
int		db_fromconsole = DDB_FROMCONSOLE;


extern int	db_radix;
extern int	db_max_width;
extern int	db_tab_stop_width;
extern int	db_max_line;

static int	db_rw_internal_variable __P((struct db_variable *, db_expr_t *,
		    int));

/* XXX must all be ints for sysctl. */
struct db_variable db_vars[] = {
	{ "radix",	(long *)&db_radix,	db_rw_internal_variable },
	{ "maxoff",	(long *)&db_maxoff,	db_rw_internal_variable },
	{ "maxwidth",	(long *)&db_max_width,	db_rw_internal_variable },
	{ "tabstops",	(long *)&db_tab_stop_width, db_rw_internal_variable },
	{ "lines",	(long *)&db_max_line,	db_rw_internal_variable },
	{ "onpanic",	(long *)&db_onpanic,	db_rw_internal_variable },
	{ "fromconsole", (long *)&db_onpanic,	db_rw_internal_variable },
};
struct db_variable *db_evars = db_vars + sizeof(db_vars)/sizeof(db_vars[0]);

/*
 * ddb command line access to the DDB variables defined above.
 */
static int
db_rw_internal_variable(vp, valp, rw)
	struct db_variable *vp;
	db_expr_t *valp;
	int rw;
{

	if (rw == DB_VAR_GET) {
		*valp = *(int *)vp->valuep;
	} else {
		*(int *)vp->valuep = *valp;
	}
	return (0);
}

/*
 * sysctl(3) access to the DDB variables defined above.
 */
int
ddb_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int	*name;
	u_int	namelen;
	void	*oldp;
	size_t	*oldlenp;
	void	*newp;
	size_t	newlen;
	struct proc *p;
{

	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return (ENOTDIR);

	switch (name[0]) {
	case DDBCTL_RADIX:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &db_radix));

	case DDBCTL_MAXOFF:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &db_maxoff));

	case DDBCTL_MAXWIDTH:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &db_max_width));

	case DDBCTL_TABSTOPS:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &db_tab_stop_width));

	case DDBCTL_LINES:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &db_max_line));

	case DDBCTL_ONPANIC:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &db_onpanic));
	case DDBCTL_FROMCONSOLE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &db_fromconsole));
	}

	return (EOPNOTSUPP);
}

int
db_find_variable(varp)
	struct db_variable	**varp;
{
	int	t;
	struct db_variable *vp;

	t = db_read_token();
	if (t == tIDENT) {
	    for (vp = db_vars; vp < db_evars; vp++) {
		if (!strcmp(db_tok_string, vp->name)) {
		    *varp = vp;
		    return (1);
		}
	    }
	    for (vp = db_regs; vp < db_eregs; vp++) {
		if (!strcmp(db_tok_string, vp->name)) {
		    *varp = vp;
		    return (1);
		}
	    }
	}
	db_error("Unknown variable\n");
	/*NOTREACHED*/
	return 0;
}

int
db_get_variable(valuep)
	db_expr_t	*valuep;
{
	struct db_variable *vp;

	if (!db_find_variable(&vp))
	    return (0);

	db_read_variable(vp, valuep);

	return (1);
}

int
db_set_variable(value)
	db_expr_t	value;
{
	struct db_variable *vp;

	if (!db_find_variable(&vp))
	    return (0);

	db_write_variable(vp, &value);

	return (1);
}


void
db_read_variable(vp, valuep)
	struct db_variable *vp;
	db_expr_t	*valuep;
{
	int	(*func) __P((struct db_variable *, db_expr_t *, int)) = vp->fcn;

	if (func == FCN_NULL)
	    *valuep = *(vp->valuep);
	else
	    (*func)(vp, valuep, DB_VAR_GET);
}

void
db_write_variable(vp, valuep)
	struct db_variable *vp;
	db_expr_t	*valuep;
{
	int	(*func) __P((struct db_variable *, db_expr_t *, int)) = vp->fcn;

	if (func == FCN_NULL)
	    *(vp->valuep) = *valuep;
	else
	    (*func)(vp, valuep, DB_VAR_SET);
}

/*ARGSUSED*/
void
db_set_cmd(addr, have_addr, count, modif)
	db_expr_t	addr;
	int		have_addr;
	db_expr_t	count;
	char *		modif;
{
	db_expr_t	value;
	struct db_variable *vp;
	int	t;

	t = db_read_token();
	if (t != tDOLLAR) {
	    db_error("Unknown variable\n");
	    /*NOTREACHED*/
	}
	if (!db_find_variable(&vp)) {
	    db_error("Unknown variable\n");
	    /*NOTREACHED*/
	}

	t = db_read_token();
	if (t != tEQ)
	    db_unread_token(t);

	if (!db_expression(&value)) {
	    db_error("No value\n");
	    /*NOTREACHED*/
	}
	if (db_read_token() != tEOL) {
	    db_error("?\n");
	    /*NOTREACHED*/
	}

	db_write_variable(vp, &value);
}
