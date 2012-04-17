/*	$NetBSD: db_machdep.c,v 1.1.4.2 2012/04/17 00:06:56 yamt Exp $ */

/*
 * Copyright (c) 1996-2002 Eduardo Horvath.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: db_machdep.c,v 1.1.4.2 2012/04/17 00:06:56 yamt Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#endif

#include <sys/param.h>
#include <sys/cpu.h>
#include <ddb/ddb.h>

static int
db_sparc_charop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	char *regaddr =
	    (char *)(((uint8_t *)DDB_REGS) + (size_t)vp->valuep);

	switch (opcode) {
	case DB_VAR_GET:
		*val = *regaddr;
		break;
	case DB_VAR_SET:
		*regaddr = *val;
		break;
#ifdef DIAGNOSTIC
	default:
		printf("db_sparc_charop: opcode %d\n", opcode);
		break;
#endif
	}

	return 0;
}

#ifdef not_used
static int
db_sparc_shortop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	short *regaddr =
	    (short *)(((uint8_t *)DDB_REGS) + (size_t)vp->valuep);

	switch (opcode) {
	case DB_VAR_GET:
		*val = *regaddr;
		break;
	case DB_VAR_SET:
		*regaddr = *val;
		break;
#ifdef DIAGNOSTIC
	default:
		printf("sparc_shortop: opcode %d\n", opcode);
		break;
#endif
	}

	return 0;
}
#endif

static int
db_sparc_intop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	int *regaddr =
	    (int *)(((uint8_t *)DDB_REGS) + (size_t)vp->valuep);

	switch (opcode) {
	case DB_VAR_GET:
		*val = *regaddr;
		break;
	case DB_VAR_SET:
		*regaddr = *val;
		break;
#ifdef DIAGNOSTIC
	default:
		printf("db_sparc_intop: opcode %d\n", opcode);
		break;
#endif
	}

	return 0;
}

static int
db_sparc_regop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	db_expr_t *regaddr =
	    (db_expr_t *)(((uint8_t *)DDB_REGS) + (size_t)vp->valuep);

	switch (opcode) {
	case DB_VAR_GET:
		*val = *regaddr;
		break;
	case DB_VAR_SET:
		*regaddr = *val;
		break;
#ifdef DIAGNOSTIC
	default:
		printf("db_sparc_regop: unknown op %d\n", opcode);
		break;
#endif
	}
	return 0;
}

/*
 * Machine register set.
 */
#define dbreg(xx) (long *)offsetof(db_regs_t, db_tf.tf_ ## xx)
#define dbregfr(xx) (long *)offsetof(db_regs_t, db_fr.fr_ ## xx)
#define dbregfp(xx) (long *)offsetof(db_regs_t, db_fpstate.fs_ ## xx)

static uint64_t nil;

const struct db_variable db_regs[] = {
	{ "tstate",	dbreg(tstate),		db_sparc_regop, 0 },
	{ "pc",		dbreg(pc),		db_sparc_regop, 0 },
	{ "npc",	dbreg(npc),		db_sparc_regop, 0 },
	{ "ipl",	dbreg(oldpil),		db_sparc_charop, 0 },
	{ "y",		dbreg(y),		db_sparc_intop, 0 },
	{ "g0",		(void *)&nil,		FCN_NULL, 0 },
	{ "g1",		dbreg(global[1]),	db_sparc_regop, 0 },
	{ "g2",		dbreg(global[2]),	db_sparc_regop, 0 },
	{ "g3",		dbreg(global[3]),	db_sparc_regop, 0 },
	{ "g4",		dbreg(global[4]),	db_sparc_regop, 0 },
	{ "g5",		dbreg(global[5]),	db_sparc_regop, 0 },
	{ "g6",		dbreg(global[6]),	db_sparc_regop, 0 },
	{ "g7",		dbreg(global[7]),	db_sparc_regop, 0 },
	{ "o0",		dbreg(out[0]),		db_sparc_regop, 0 },
	{ "o1",		dbreg(out[1]),		db_sparc_regop, 0 },
	{ "o2",		dbreg(out[2]),		db_sparc_regop, 0 },
	{ "o3",		dbreg(out[3]),		db_sparc_regop, 0 },
	{ "o4",		dbreg(out[4]),		db_sparc_regop, 0 },
	{ "o5",		dbreg(out[5]),		db_sparc_regop, 0 },
	{ "o6",		dbreg(out[6]),		db_sparc_regop, 0 },
	{ "o7",		dbreg(out[7]),		db_sparc_regop, 0 },
	{ "l0",		dbregfr(local[0]),	db_sparc_regop, 0 },
	{ "l1",		dbregfr(local[1]),	db_sparc_regop, 0 },
	{ "l2",		dbregfr(local[2]),	db_sparc_regop, 0 },
	{ "l3",		dbregfr(local[3]),	db_sparc_regop, 0 },
	{ "l4",		dbregfr(local[4]),	db_sparc_regop, 0 },
	{ "l5",		dbregfr(local[5]),	db_sparc_regop, 0 },
	{ "l6",		dbregfr(local[6]),	db_sparc_regop, 0 },
	{ "l7",		dbregfr(local[7]),	db_sparc_regop, 0 },
	{ "i0",		dbregfr(arg[0]),	db_sparc_regop, 0 },
	{ "i1",		dbregfr(arg[1]),	db_sparc_regop, 0 },
	{ "i2",		dbregfr(arg[2]),	db_sparc_regop, 0 },
	{ "i3",		dbregfr(arg[3]),	db_sparc_regop, 0 },
	{ "i4",		dbregfr(arg[4]),	db_sparc_regop, 0 },
	{ "i5",		dbregfr(arg[5]),	db_sparc_regop, 0 },
	{ "i6",		dbregfr(fp),		db_sparc_regop, 0 },
	{ "i7",		dbregfr(pc),		db_sparc_regop, 0 },
	{ "f0",		dbregfp(regs[0]),	db_sparc_regop, 0 },
	{ "f2",		dbregfp(regs[2]),	db_sparc_regop, 0 },
	{ "f4",		dbregfp(regs[4]),	db_sparc_regop, 0 },
	{ "f6",		dbregfp(regs[6]),	db_sparc_regop, 0 },
	{ "f8",		dbregfp(regs[8]),	db_sparc_regop, 0 },
	{ "f10",	dbregfp(regs[10]),	db_sparc_regop, 0 },
	{ "f12",	dbregfp(regs[12]),	db_sparc_regop, 0 },
	{ "f14",	dbregfp(regs[14]),	db_sparc_regop, 0 },
	{ "f16",	dbregfp(regs[16]),	db_sparc_regop, 0 },
	{ "f18",	dbregfp(regs[18]),	db_sparc_regop, 0 },
	{ "f20",	dbregfp(regs[20]),	db_sparc_regop, 0 },
	{ "f22",	dbregfp(regs[22]),	db_sparc_regop, 0 },
	{ "f24",	dbregfp(regs[24]),	db_sparc_regop, 0 },
	{ "f26",	dbregfp(regs[26]),	db_sparc_regop, 0 },
	{ "f28",	dbregfp(regs[28]),	db_sparc_regop, 0 },
	{ "f30",	dbregfp(regs[30]),	db_sparc_regop, 0 },
	{ "f32",	dbregfp(regs[32]),	db_sparc_regop, 0 },
	{ "f34",	dbregfp(regs[34]),	db_sparc_regop, 0 },
	{ "f36",	dbregfp(regs[36]),	db_sparc_regop, 0 },
	{ "f38",	dbregfp(regs[38]),	db_sparc_regop, 0 },
	{ "f40",	dbregfp(regs[40]),	db_sparc_regop, 0 },
	{ "f42",	dbregfp(regs[42]),	db_sparc_regop, 0 },
	{ "f44",	dbregfp(regs[44]),	db_sparc_regop, 0 },
	{ "f46",	dbregfp(regs[46]),	db_sparc_regop, 0 },
	{ "f48",	dbregfp(regs[48]),	db_sparc_regop, 0 },
	{ "f50",	dbregfp(regs[50]),	db_sparc_regop, 0 },
	{ "f52",	dbregfp(regs[52]),	db_sparc_regop, 0 },
	{ "f54",	dbregfp(regs[54]),	db_sparc_regop, 0 },
	{ "f56",	dbregfp(regs[56]),	db_sparc_regop, 0 },
	{ "f58",	dbregfp(regs[58]),	db_sparc_regop, 0 },
	{ "f60",	dbregfp(regs[60]),	db_sparc_regop, 0 },
	{ "f62",	dbregfp(regs[62]),	db_sparc_regop, 0 },
	{ "fsr",	dbregfp(fsr),		db_sparc_regop, 0 },
	{ "gsr",	dbregfp(gsr),		db_sparc_intop, 0 },
};
const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

#ifndef	DDB
const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD(NULL,     NULL,           0,	NULL,NULL,NULL) }
};
#endif	/* DDB */
