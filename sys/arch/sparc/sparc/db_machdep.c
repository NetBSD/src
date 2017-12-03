/*	$NetBSD: db_machdep.c,v 1.1.6.2 2017/12/03 11:36:43 jdolecek Exp $ */

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
 *	From: db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Interface to new debugger.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_machdep.c,v 1.1.6.2 2017/12/03 11:36:43 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#endif

#include <sys/param.h>
#include <machine/db_machdep.h>
#include <ddb/db_user.h>
#include <ddb/db_command.h>
#include <ddb/db_variables.h>

/*
 * Machine register set.
 */
#define dbreg(xx) (long *)offsetof(db_regs_t, db_tf.tf_ ## xx)
#define dbregfr(xx) (long *)offsetof(db_regs_t, db_fr.fr_ ## xx)

static int db_sparc_regop(const struct db_variable *, db_expr_t *, int);

db_regs_t *ddb_regp;
static long nil;

const struct db_variable db_regs[] = {
	{ "psr",	dbreg(psr),		db_sparc_regop, NULL, },
	{ "pc",		dbreg(pc),		db_sparc_regop, NULL, },
	{ "npc",	dbreg(npc),		db_sparc_regop, NULL, },
	{ "y",		dbreg(y),		db_sparc_regop, NULL, },
	{ "wim",	dbreg(global[0]),	db_sparc_regop, NULL, }, /* see reg.h */
	{ "g0",		&nil,			FCN_NULL, 	NULL, },
	{ "g1",		dbreg(global[1]),	db_sparc_regop, NULL, },
	{ "g2",		dbreg(global[2]),	db_sparc_regop, NULL, },
	{ "g3",		dbreg(global[3]),	db_sparc_regop, NULL, },
	{ "g4",		dbreg(global[4]),	db_sparc_regop, NULL, },
	{ "g5",		dbreg(global[5]),	db_sparc_regop, NULL, },
	{ "g6",		dbreg(global[6]),	db_sparc_regop, NULL, },
	{ "g7",		dbreg(global[7]),	db_sparc_regop, NULL, },
	{ "o0",		dbreg(out[0]),		db_sparc_regop, NULL, },
	{ "o1",		dbreg(out[1]),		db_sparc_regop, NULL, },
	{ "o2",		dbreg(out[2]),		db_sparc_regop, NULL, },
	{ "o3",		dbreg(out[3]),		db_sparc_regop, NULL, },
	{ "o4",		dbreg(out[4]),		db_sparc_regop, NULL, },
	{ "o5",		dbreg(out[5]),		db_sparc_regop, NULL, },
	{ "o6",		dbreg(out[6]),		db_sparc_regop, NULL, },
	{ "o7",		dbreg(out[7]),		db_sparc_regop, NULL, },
	{ "l0",		dbregfr(local[0]),	db_sparc_regop, NULL, },
	{ "l1",		dbregfr(local[1]),	db_sparc_regop, NULL, },
	{ "l2",		dbregfr(local[2]),	db_sparc_regop, NULL, },
	{ "l3",		dbregfr(local[3]),	db_sparc_regop, NULL, },
	{ "l4",		dbregfr(local[4]),	db_sparc_regop, NULL, },
	{ "l5",		dbregfr(local[5]),	db_sparc_regop, NULL, },
	{ "l6",		dbregfr(local[6]),	db_sparc_regop, NULL, },
	{ "l7",		dbregfr(local[7]),	db_sparc_regop, NULL, },
	{ "i0",		dbregfr(arg[0]),	db_sparc_regop, NULL, },
	{ "i1",		dbregfr(arg[1]),	db_sparc_regop, NULL, },
	{ "i2",		dbregfr(arg[2]),	db_sparc_regop, NULL, },
	{ "i3",		dbregfr(arg[3]),	db_sparc_regop, NULL, },
	{ "i4",		dbregfr(arg[4]),	db_sparc_regop, NULL, },
	{ "i5",		dbregfr(arg[5]),	db_sparc_regop, NULL, },
	{ "i6",		dbregfr(fp),		db_sparc_regop, NULL, },
	{ "i7",		dbregfr(pc),		db_sparc_regop, NULL, },
};
const struct db_variable * const db_eregs =
    db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

static int
db_sparc_regop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	db_expr_t *regaddr =
	    (db_expr_t *)(((uint8_t *)DDB_REGS) + ((size_t)vp->valuep));

	switch (opcode) {
	case DB_VAR_GET:
		*val = *regaddr;
		break;
	case DB_VAR_SET:
		*regaddr = *val;
		break;
	default:
#ifdef _KERNEL
		panic("db_sparc_regop: unknown op %d", opcode);
#else
		printf("db_sparc_regop: unknown op %d\n", opcode);
#endif
	}
	return 0;
}

#ifndef DDB
const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD(NULL,     NULL,           0,	NULL,NULL,NULL) }
};
#endif /* DDB */
