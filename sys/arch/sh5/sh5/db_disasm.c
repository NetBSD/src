/*	$NetBSD: db_disasm.c,v 1.14 2003/08/10 22:22:31 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_disasm.c,v 1.14 2003/08/10 22:22:31 scw Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>
#include <machine/frame.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
#include <ddb/ddbvar.h>

typedef const char *(*format_func_t)(opcode_t, db_addr_t,
    char *, char *, char *);

#define	SH5_OPCODE_FORMAT(op)	(((op) >> 26) & 0x3f)

/*
 * Opcode Major Formats
 */
static const char *sh5_fmt_mnd0(opcode_t, db_addr_t, char *, char *, char *);
static const char *sh5_fmt_msd6(opcode_t, db_addr_t, char *, char *, char *);
static const char *sh5_fmt_msd10(opcode_t, db_addr_t, char *, char *, char *);
static const char *sh5_fmt_xsd16(opcode_t, db_addr_t, char *, char *, char *);

static const format_func_t major_format_funcs[] = {
	/* Opcode bits 5, 4 and 3 == 000 */
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	NULL,			/* FPU-reserved */
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,

	/* Opcode bits 5, 4 and 3 == 001 */
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,

	/* Opcode bits 5, 4 and 3 == 010 */
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	NULL,			/* reserved */
	NULL,			/* reserved */
	NULL,			/* reserved */
	NULL,			/* reserved */

	/* Opcode bits 5, 4 and 3 == 011 */
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	sh5_fmt_mnd0,
	NULL,			/* reserved */
	NULL,			/* reserved */
	NULL,			/* reserved */
	NULL,			/* reserved */

	/* Opcode bits 5, 4 and 3 == 100 */
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,

	/* Opcode bits 5, 4 and 3 == 101 */
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,

	/* Opcode bits 5, 4 and 3 == 110 */
	sh5_fmt_msd6,
	sh5_fmt_msd6,
	sh5_fmt_xsd16,
	sh5_fmt_xsd16,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,
	sh5_fmt_msd10,

	/* Opcode bits 5, 4 and 3 == 111 */
	sh5_fmt_msd6,
	sh5_fmt_msd6,
	sh5_fmt_xsd16,
	sh5_fmt_xsd16,
	NULL,			/* reserved */
	NULL,			/* reserved */
	NULL,			/* reserved */
	NULL			/* reserved */
};


/*
 * Major Format MND0 is decoded using the following table
 */
struct format_mnd0 {
	const char *mnemonic;
	char op_s1;		/* Source operand 1 */
	char op_s2;		/* Source operand 2 */
	char op_d;		/* Destination operand */
};
static int sh5_fmt_mnd0_decode_op(int, int, char *);

#define	FMT_MND0_MAJ_INDEX(op)	(SH5_OPCODE_FORMAT(op))
#define	FMT_MND0_MIN_INDEX(op)	(((op) >> 16) & 0x0f)
#define	FMT_MND0_S1(op)		(((op) >> 20) & 0x3f)
#define	FMT_MND0_S2(op)		(((op) >> 10) & 0x3f)
#define	FMT_MND0_D(op)		(((op) >> 4) & 0x3f)

/* Possible values for the operands */
#define	FMT_MND0_OP_NONE	0	/* Unused, but must be encoded = 0x3f */
#define	FMT_MND0_OP_AS1		1	/* Unused, but must be encoded as s1 */
#define	FMT_MND0_OP_R		2	/* General purpose register */
#define	FMT_MND0_OP_F		3	/* Single-precision FP register */
#define	FMT_MND0_OP_D		4	/* Double-precision FP register */
#define	FMT_MND0_OP_V		5	/* Vector specification */
#define	FMT_MND0_OP_MTRX	6	/* Matrix specification */
#define	FMT_MND0_OP_CR		7	/* Control Register */
#define	FMT_MND0_OP_TR		8	/* Branch Target Register */
#define	FMT_MND0_OP_TRL		9	/* Branch Target Reg, w/ "likely" bit */

static const struct format_mnd0 format_mnd0[][16] = {
	/* Opcode 000000, ALU */
	{
	{NULL,		0},
	{"cmpeq",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"cmpgt",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"cmpgu",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"add.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"add",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"sub.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"sub",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"addz.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"nsb",		FMT_MND0_OP_R, FMT_MND0_OP_NONE, FMT_MND0_OP_R},
	{"mulu.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"byterev",	FMT_MND0_OP_R, FMT_MND0_OP_NONE, FMT_MND0_OP_R}
	},

	/* Opcode 000001, ALU */
	{
	{"shlld.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"shlld",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"shlrd.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"shlrd",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{"shard.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"shard",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"or",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"and",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"xor",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"muls.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"andc",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R}
	},

	/* Opcode 000010, MM */
	{
	{NULL,		0},
	{"madd.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"madd.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"madds.ub",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"madds.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"madds.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{"msub.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"msub.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"msubs.ub",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"msubs.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"msubs.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0}
	},

	/* Opcode 000011, MM */
	{
	{NULL,		0},
	{"mshlld.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshlld.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{"mshalds.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshalds.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{"mshard.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshard.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshards.q",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mshlrd.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshlrd.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0}
	},

	/* Opcode 000100, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 000101, FPU */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"fipr.s",	FMT_MND0_OP_V, FMT_MND0_OP_V, FMT_MND0_OP_F},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"ftrv.s",	FMT_MND0_OP_MTRX, FMT_MND0_OP_V, FMT_MND0_OP_V},
	{NULL,		0}
	},

	/* Opcode 000110, FPU */
	{
	{"fabs.s",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{"fabs.d",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{"fneg.s",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{"fneg.d",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"fsina.s",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{NULL,		0},
	{"fsrra.s",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{NULL,		0},
	{"fcosa.s",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 000111, FLOAD */
	{
	{"fmov.ls",	FMT_MND0_OP_R, FMT_MND0_OP_NONE, FMT_MND0_OP_F},
	{"fmov.qd",	FMT_MND0_OP_R, FMT_MND0_OP_NONE, FMT_MND0_OP_D},
	{"fgetscr",	FMT_MND0_OP_NONE, FMT_MND0_OP_NONE, FMT_MND0_OP_F},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"fldx.s",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_F},
	{"fldx.d",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_D},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"fldx.p",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_F},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 001000, RMW */
	{
	{NULL,		0},
	{"cmveq",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"swap.q",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"cmvne",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 001001, MISC */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"getcon",	FMT_MND0_OP_CR, FMT_MND0_OP_NONE, FMT_MND0_OP_R}
	},

	/* Opcode 001010, MM */
	{
	{"mcmpeq.b",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mcmpeq.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mcmpeq.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mcmpgt.b",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mcmpgt.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mcmpgt.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mextr1",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mabs.w",	FMT_MND0_OP_R, FMT_MND0_OP_NONE, FMT_MND0_OP_R},
	{"mabs.l",	FMT_MND0_OP_R, FMT_MND0_OP_NONE, FMT_MND0_OP_R},
	{"mextr2",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mperm.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mextr3",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R}
	},

	/* Opcode 001011, MM */
	{
	{"mshflo.b",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshflo.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshflo.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mextr4",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshfhi.b",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshfhi.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mshfhi.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mextr5",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"mextr6",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"mextr7",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R}
	},

	/* Opcode 001100, FPU */
	{
	{"fmov.sl",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_R},
	{"fmov.dq",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_R},
	{"fputscr",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_NONE},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"fcmpeq.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_R},
	{"fcmpeq.d",	FMT_MND0_OP_D, FMT_MND0_OP_D, FMT_MND0_OP_R},
	{"fcmpun.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_R},
	{"fcmpun.d",	FMT_MND0_OP_D, FMT_MND0_OP_D, FMT_MND0_OP_R},
	{"fcmpgt.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_R},
	{"fcmpgt.d",	FMT_MND0_OP_D, FMT_MND0_OP_D, FMT_MND0_OP_R},
	{"fcmpge.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_R},
	{"fcmpge.d",	FMT_MND0_OP_D, FMT_MND0_OP_D, FMT_MND0_OP_R}
	},

	/* Opcode 001101, FPU */
	{
	{"fadd.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_F},
	{"fadd.d",	FMT_MND0_OP_D, FMT_MND0_OP_D, FMT_MND0_OP_D},
	{"fsub.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_F},
	{"fsub.d",	FMT_MND0_OP_D, FMT_MND0_OP_D, FMT_MND0_OP_D},
	{"fdiv.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_F},
	{"fdiv.d",	FMT_MND0_OP_D, FMT_MND0_OP_D, FMT_MND0_OP_D},
	{"fdiv.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_F},
	{"fdiv.d",	FMT_MND0_OP_D, FMT_MND0_OP_D, FMT_MND0_OP_D},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"fmac.s",	FMT_MND0_OP_F, FMT_MND0_OP_F, FMT_MND0_OP_F},
	{NULL,		0}
	},

	/* Opcode 001110, FPU */
	{
	{"fmov.s",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{"fmov.d",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{NULL,		0},
	{NULL,		0},
	{"fsqrt.s",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{"fsqrt.d",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{"fcnv.sd",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{"fcnv.ds",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{"ftrc.sl",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{"ftrc.dq",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{"ftrc.sq",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{"ftrc.dl",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{"float.ls",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_F},
	{"float.qd",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{"float.ld",	FMT_MND0_OP_F, FMT_MND0_OP_AS1, FMT_MND0_OP_D},
	{"float.qs",	FMT_MND0_OP_D, FMT_MND0_OP_AS1, FMT_MND0_OP_F}
	},

	/* Opcode 001111, FSTORE */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"fstx.s",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_F},
	{"fstx.d",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_D},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"fstx.p",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_F},
	{NULL,		0},
	{NULL,		0},
	},

	/* Opcode 010000, LOAD */
	{
	{"ldx.b",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"ldx.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"ldx.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"ldx.q",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"ldx.ub",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"ldx.uw",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 010001, BRANCH */
	{
	{NULL,		0},
	{"blink",	FMT_MND0_OP_TR, FMT_MND0_OP_NONE, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"gettr",	FMT_MND0_OP_TR, FMT_MND0_OP_NONE, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 010010, MM */
	{
	{"msad.ubq",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mmacfx.wl",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mcmv",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mmacnfx.wl",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"mmulsum.wq",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 010011, MM */
	{
	{NULL,		0},
	{"mmul.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mmul.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{"mmulfx.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mmulfx.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mcnvs.wb",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mmulfxrp.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mmullo.wl",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{"mcnvs.wub",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mcnvs.lw",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"mmulhi.wl",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0}
	},

	/* Opcode 010100, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 010101, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 010110, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 010111, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 011000, STORE */
	{
	{"stx.b",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"stx.w",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"stx.l",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{"stx.q",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 011001, BRANCH */
	{
	{NULL,		0},
	{"beq",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_TRL},
	{NULL,		0},
	{"bge",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_TRL},
	{NULL,		0},
	{"bne",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_TRL},
	{NULL,		0},
	{"bgt",		FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_TRL},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"bgeu",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_TRL},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"bgtu",	FMT_MND0_OP_R, FMT_MND0_OP_R, FMT_MND0_OP_TRL}
	},

	/* Opcode 011010, PT */
	{
	{NULL,		0},
	{"ptabs",	FMT_MND0_OP_NONE, FMT_MND0_OP_R, FMT_MND0_OP_TRL},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"ptrel",	FMT_MND0_OP_NONE, FMT_MND0_OP_R, FMT_MND0_OP_TRL},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 011011, MISC */
	{
	{"nop",		FMT_MND0_OP_NONE, FMT_MND0_OP_NONE, FMT_MND0_OP_NONE},
	{"trapa",	FMT_MND0_OP_R, FMT_MND0_OP_NONE, FMT_MND0_OP_NONE},
	{"synci",	FMT_MND0_OP_NONE, FMT_MND0_OP_NONE, FMT_MND0_OP_NONE},
	{"rte",		FMT_MND0_OP_NONE, FMT_MND0_OP_NONE, FMT_MND0_OP_NONE},
	{"illegal",	FMT_MND0_OP_NONE, FMT_MND0_OP_NONE, FMT_MND0_OP_NONE},
	{"brk",		FMT_MND0_OP_NONE, FMT_MND0_OP_NONE, FMT_MND0_OP_NONE},
	{"synco",	FMT_MND0_OP_NONE, FMT_MND0_OP_NONE, FMT_MND0_OP_NONE},
	{"sleep",	FMT_MND0_OP_NONE, FMT_MND0_OP_NONE, FMT_MND0_OP_NONE},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"putcon",	FMT_MND0_OP_R, FMT_MND0_OP_NONE, FMT_MND0_OP_CR},
	}
};


/*
 * Major Format MSD6 is decoded using the following table
 */
struct format_msd6 {
	const char *mnemonic;
	char op_r;		/* Register operand */
	char op_imm;		/* Immediate operand */
	char op_sd;		/* Source/Destination operand */
};
static int sh5_fmt_msd6_decode_op(int, int, char *);

#define	FMT_MSD6_MAJ_INDEX(op)	(SH5_OPCODE_FORMAT(op) - 0x30)
#define	FMT_MSD6_MIN_INDEX(op)	(((op) >> 16) & 0x0f)
#define	FMT_MSD6_R(op)		(((op) >> 20) & 0x3f)
#define	FMT_MSD6_IMM(op)	(((op) >> 10) & 0x3f)
#define	FMT_MSD6_SD(op)		(((op) >> 4) & 0x3f)

/* Possible values for the operands */
#define	FMT_MSD6_OP_NONE	0	/* Unused, but must be encoded = 0x3f */
#define	FMT_MSD6_OP_R		1	/* General purpose register */
#define	FMT_MSD6_OP_SIMM	2	/* Signed Immediate */
#define	FMT_MSD6_OP_IMM		3	/* Unsigned Immediate */
#define	FMT_MSD6_OP_SIMM32	4	/* Signed Immediate, scaled by 32 */
#define	FMT_MSD6_OP_TRL		5	/* Branch Target Register, "likely" */

static const struct format_msd6 format_msd6[][16] = {
	/* Opcode 110000, LOAD */
	{
	{NULL,		0},
	{NULL,		0},
	{"ldlo.l",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{"ldlo.q",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{NULL,		0},
	{NULL,		0},
	{"ldhi.l",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{"ldhi.q",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"getcfg",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R}
	},

	/* Opcode 110001, ALU */
	{
	{"shlli.l",	FMT_MSD6_OP_R, FMT_MSD6_OP_IMM, FMT_MSD6_OP_R},
	{"shlli",	FMT_MSD6_OP_R, FMT_MSD6_OP_IMM, FMT_MSD6_OP_R},
	{"shlri.l",	FMT_MSD6_OP_R, FMT_MSD6_OP_IMM, FMT_MSD6_OP_R},
	{"shlri",	FMT_MSD6_OP_R, FMT_MSD6_OP_IMM, FMT_MSD6_OP_R},
	{NULL,		0},
	{NULL,		0},
	{"shari.l",	FMT_MSD6_OP_R, FMT_MSD6_OP_IMM, FMT_MSD6_OP_R},
	{"shari",	FMT_MSD6_OP_R, FMT_MSD6_OP_IMM, FMT_MSD6_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"xori",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 110010, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},


	/* Opcode 110011, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},


	/* Opcode 110100, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},


	/* Opcode 110101, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},


	/* Opcode 110110, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},


	/* Opcode 110111, <unused> */
	{
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	},

	/* Opcode 111000, STORE */
	{
	{NULL,		0},
	{"prefi",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM32, FMT_MSD6_OP_NONE},
	{"stlo.l",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{"stlo.q",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{"alloco",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM32, FMT_MSD6_OP_NONE},
	{"icbi",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM32, FMT_MSD6_OP_NONE},
	{"sthi.l",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{"sthi.q",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R},
	{"ocbp",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM32, FMT_MSD6_OP_NONE},
	{"ocbi",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM32, FMT_MSD6_OP_NONE},
	{NULL,		0},
	{NULL,		0},
	{"ocbwb",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM32, FMT_MSD6_OP_NONE},
	{NULL,		0},
	{NULL,		0},
	{"putcfg",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_R}
	},

	/* Opcode 111001, BRANCH */
	{
	{NULL,		0},
	{"beqi",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_TRL},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"bnei",	FMT_MSD6_OP_R, FMT_MSD6_OP_SIMM, FMT_MSD6_OP_TRL},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0}
	}
};


/*
 * Major Format MSD10 is decoded using the following table
 */
struct format_msd10 {
	const char *mnemonic;
	char op_imm;		/* Immediate operand */
	char op_sd;		/* Source/Destination operand */
};
static int sh5_fmt_msd10_decode_op(int, int, char *);

#define	FMT_MSD10_MAJ_INDEX(op)	(SH5_OPCODE_FORMAT(op) - 0x20)
#define	FMT_MSD10_R(op)		(((op) >> 20) & 0x3f)
#define	FMT_MSD10_IMM(op)	(((op) >> 10) & 0x3ff)
#define	FMT_MSD10_SD(op)	(((op) >> 4) & 0x3f)

/* Possible values for the operands */
#define	FMT_MSD10_OP_R		0	/* General purpose register */
#define	FMT_MSD10_OP_F		1	/* Single-precision FP register */
#define	FMT_MSD10_OP_D		2	/* Double-precision FP register */
#define	FMT_MSD10_OP_SIMM	3	/* Signed Immediate */
#define	FMT_MSD10_OP_SIMM2	4	/* Signed Immediate, scaled by 2 */
#define	FMT_MSD10_OP_SIMM4	5	/* Signed Immediate, scaled by 4 */
#define	FMT_MSD10_OP_SIMM8	6	/* Signed Immediate, scaled by 8 */

static const struct format_msd10 format_msd10[] = {
	{"ld.b",	FMT_MSD10_OP_SIMM, FMT_MSD10_OP_R},
	{"ld.w",	FMT_MSD10_OP_SIMM2, FMT_MSD10_OP_R},
	{"ld.l",	FMT_MSD10_OP_SIMM4, FMT_MSD10_OP_R},
	{"ld.q",	FMT_MSD10_OP_SIMM8, FMT_MSD10_OP_R},
	{"ld.ub",	FMT_MSD10_OP_SIMM, FMT_MSD10_OP_R},
	{"fld.s",	FMT_MSD10_OP_SIMM4, FMT_MSD10_OP_F},
	{"fld.p",	FMT_MSD10_OP_SIMM8, FMT_MSD10_OP_F},
	{"fld.d",	FMT_MSD10_OP_SIMM8, FMT_MSD10_OP_D},
	{"st.b",	FMT_MSD10_OP_SIMM, FMT_MSD10_OP_R},
	{"st.w",	FMT_MSD10_OP_SIMM2, FMT_MSD10_OP_R},
	{"st.l",	FMT_MSD10_OP_SIMM4, FMT_MSD10_OP_R},
	{"st.q",	FMT_MSD10_OP_SIMM8, FMT_MSD10_OP_R},
	{"ld.uw",	FMT_MSD10_OP_SIMM2, FMT_MSD10_OP_R},
	{"fst.s",	FMT_MSD10_OP_SIMM4, FMT_MSD10_OP_F},
	{"fst.p",	FMT_MSD10_OP_SIMM8, FMT_MSD10_OP_F},
	{"fst.d",	FMT_MSD10_OP_SIMM8, FMT_MSD10_OP_D},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"addi",	FMT_MSD10_OP_SIMM, FMT_MSD10_OP_R},
	{"addi.l",	FMT_MSD10_OP_SIMM, FMT_MSD10_OP_R},
	{"andi",	FMT_MSD10_OP_SIMM, FMT_MSD10_OP_R},
	{"ori",		FMT_MSD10_OP_SIMM, FMT_MSD10_OP_R}
};


/*
 * Major Format XSD16 is decoded using the following table
 */
struct format_xsd16 {
	const char *mnemonic;
	char op_imm;		/* Immediate operand */
	char op_d;		/* Destination operand */
};
static int sh5_fmt_xsd16_decode_op(int, int, int, db_addr_t, char *);

#define	FMT_XSD16_MAJ_INDEX(op)	(SH5_OPCODE_FORMAT(op) - 0x32)
#define	FMT_XSD16_IMM(op)	(((op) >> 10) & 0xffff)
#define	FMT_XSD16_D(op)		(((op) >> 4) & 0x3f)

/* Possible values for the operands */
#define	FMT_XSD16_OP_R		0	/* General purpose register */
#define	FMT_XSD16_OP_TRL	1	/* Branch Target Reg, w/ "likely" bit */
#define	FMT_XSD16_OP_SHORI	2	/* Unsigned Immediate */
#define	FMT_XSD16_OP_MOVI	3	/* Signed Immediate */
#define	FMT_XSD16_OP_LABEL	5	/* Branch label/offset */

static const struct format_xsd16 format_xsd16[] = {
	{"shori",	FMT_XSD16_OP_SHORI, FMT_XSD16_OP_R},
	{"movi",	FMT_XSD16_OP_MOVI, FMT_XSD16_OP_R},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{NULL,		0},
	{"pta",		FMT_XSD16_OP_LABEL, FMT_XSD16_OP_TRL},
	{"ptb",		FMT_XSD16_OP_LABEL, FMT_XSD16_OP_TRL}
};


static const char *sh5_conreg_names[64] = {
	"sr",       "ssr",      "pssr",     "undef_03",
	"intevt",   "expevt",   "pexpevt",  "tra",
	"spc",      "pspc",     "resvec",   "vbr",
	"undef_12", "tea",      "undef_14", "undef_15",
	"dcr",      "kcr0",     "kcr1",     "undef_19",
	"undef_20", "undef_21", "undef_22", "undef_23",
	"undef_24", "undef_25", "undef_26", "undef_27",
	"undef_28", "undef_29", "undef_30", "undef_31",
	"resvd_32", "resvd_33", "resvd_34", "resvd_35",
	"resvd_36", "resvd_37", "resvd_38", "resvd_39",
	"resvd_40", "resvd_41", "resvd_42", "resvd_43",
	"resvd_44", "resvd_45", "resvd_46", "resvd_47",
	"resvd_48", "resvd_49", "resvd_50", "resvd_51",
	"resvd_52", "resvd_53", "resvd_54", "resvd_55",
	"resvd_56", "resvd_57", "resvd_58", "resvd_59",
	"resvd_60", "resvd_61", "ctc",      "usr"
};

static int sh5_sign_extend(int, int);

static char oper1[128], oper2[128], oper3[128];
static char extra_info[256];

db_addr_t
db_disasm(db_addr_t loc, boolean_t dummy)
{
	format_func_t fp;
	opcode_t op;
	const char *mnemonic, *comma = "";

	/*
	 * Ditch the SHmedia bit
	 */
	loc &= ~3;

	if (loc < SH5_KSEG0_BASE) {
		op = fuword((void *)(intptr_t)loc);
		if (op == 0xffffffff) {
			db_printf("invalid address.\n");
			return (loc);
		}
	} else
		op = *((opcode_t *)(intptr_t)loc);

	extra_info[0] = '\0';

	/*
	 * The lowest 4 bits must be zero
	 */
	if ((op & 0xf) == 0 &&
	    (fp = major_format_funcs[SH5_OPCODE_FORMAT(op)]) != NULL)
		mnemonic = (fp)(op, loc, oper1, oper2, oper3);
	else
		mnemonic = NULL;

	if (mnemonic == NULL) {
		mnemonic = ".word";
		sprintf(oper1, "0x%08x", op);
		oper2[0] = oper3[0] = '\0';
	}

	db_printf("%-9s", mnemonic);

	if (oper1[0]) {
		db_printf("%s", oper1);
		comma = ", ";
	}

	if (oper2[0]) {
		db_printf("%s%s", comma, oper2);
		comma = ", ";
	}

	if (oper3[0])
		db_printf("%s%s", comma, oper3);

	if (extra_info[0] != '\0')
		db_printf("\t%s\n", extra_info);
	else
		db_printf("\n");

	return (loc + sizeof(opcode_t));
}

/*ARGSUSED*/
static const char *
sh5_fmt_mnd0(opcode_t op, db_addr_t loc, char *op1, char *op2, char *op3)
{
	const struct format_mnd0 *fp;
	static char trl[16];
	int s1, s2, d;

	fp = &format_mnd0[FMT_MND0_MAJ_INDEX(op)][FMT_MND0_MIN_INDEX(op)];

	s1 = FMT_MND0_S1(op);
	s2 = FMT_MND0_S2(op);
	d = FMT_MND0_D(op);

	if (fp->mnemonic == NULL)
		return (NULL);

	if (sh5_fmt_mnd0_decode_op(fp->op_s1, s1, op1) < 0)
		return (NULL);

	if (fp->op_s2 == FMT_MND0_OP_AS1) {
		if (s2 != s1)
			return (NULL);
		op2[0] = '\0';
	} else
	if (sh5_fmt_mnd0_decode_op(fp->op_s2, s2, op2) < 0)
		return (NULL);

	if (sh5_fmt_mnd0_decode_op(fp->op_d, d, op3) < 0)
		return (NULL);

	if (fp->op_d == FMT_MND0_OP_TRL) {
		sprintf(trl, "%s/%c", fp->mnemonic, (d & 0x20) ? 'l' : 'u');
		return (trl);
	}

	return (fp->mnemonic);
}

static int
sh5_fmt_mnd0_decode_op(int fmt, int op, char *ops)
{
	char opstr[16];

	switch (fmt) {
	case FMT_MND0_OP_NONE:
		if (op == 0x3f) {
			ops[0] = '\0';
			return (0);
		}
		/* FALLTHROUGH */

	case FMT_MND0_OP_AS1:
		return (-1);

	case FMT_MND0_OP_R:
		sprintf(opstr, "r%d", op);
		break;

	case FMT_MND0_OP_F:
		sprintf(opstr, "fr%d", op);
		break;

	case FMT_MND0_OP_D:
		sprintf(opstr, "dr%d", op);
		break;

	case FMT_MND0_OP_V:
		sprintf(opstr, "fv%d", op);
		break;

	case FMT_MND0_OP_MTRX:
		sprintf(opstr, "mtrx%d", op);
		break;

	case FMT_MND0_OP_CR:
		strcpy(opstr, sh5_conreg_names[op]);
		break;

	case FMT_MND0_OP_TR:
	case FMT_MND0_OP_TRL:
		if ((op & 0x18) != 0)
			return (-1);
		sprintf(opstr, "tr%d", op & 0x7);
		break;

	default:
		return (-1);
	}

	strcpy(ops, opstr);
	return (0);
}

/*ARGSUSED*/
static const char *
sh5_fmt_msd6(opcode_t op, db_addr_t loc, char *op1, char *op2, char *op3)
{
	const struct format_msd6 *fp;
	static char trl[16];
	int r, imm, sd;

	fp = &format_msd6[FMT_MSD6_MAJ_INDEX(op)][FMT_MSD6_MIN_INDEX(op)];

	r = FMT_MSD6_R(op);
	imm = FMT_MSD6_IMM(op);
	sd = FMT_MSD6_SD(op);

	if (fp->mnemonic == NULL)
		return (NULL);

	if (sh5_fmt_msd6_decode_op(fp->op_r, r, op1) < 0)
		return (NULL);

	if (sh5_fmt_msd6_decode_op(fp->op_imm, imm, op2) < 0)
		return (NULL);

	if (sh5_fmt_msd6_decode_op(fp->op_sd, sd, op3) < 0)
		return (NULL);

	if (fp->op_sd == FMT_MSD6_OP_TRL) {
		sprintf(trl, "%s/%c", fp->mnemonic, (sd & 0x20) ? 'l' : 'u');
		return (trl);
	}

	return (fp->mnemonic);
}

static int
sh5_fmt_msd6_decode_op(int fmt, int op, char *ops)
{
	char opstr[16];

	switch (fmt) {
	case FMT_MSD6_OP_NONE:
		if (op == 0x3f) {
			ops[0] = '\0';
			return (0);
		}
		return (-1);

	case FMT_MSD6_OP_R:
		sprintf(opstr, "r%d", op);
		break;

	case FMT_MSD6_OP_SIMM:
		op = sh5_sign_extend(op, 6);
		/* FALTHROUGH */

	case FMT_MSD6_OP_IMM:
		sprintf(opstr, "%d", op);
		break;

	case FMT_MSD6_OP_SIMM32:
		op = sh5_sign_extend(op, 6);
		sprintf(opstr, "%d", op * 32);
		break;

	case FMT_MSD6_OP_TRL:
		if ((op & 0x18) != 0)
			return (-1);
		sprintf(opstr, "tr%d", op & 0x7);
		break;

	default:
		return (-1);
	}

	strcpy(ops, opstr);
	return (0);
}

/*ARGSUSED*/
static const char *
sh5_fmt_msd10(opcode_t op, db_addr_t loc, char *op1, char *op2, char *op3)
{
	const struct format_msd10 *fp;
	int r, imm, sd;

	fp = &format_msd10[FMT_MSD10_MAJ_INDEX(op)];

	r = FMT_MSD10_R(op);
	imm = FMT_MSD10_IMM(op);
	sd = FMT_MSD10_SD(op);

	if (fp->mnemonic == NULL)
		return (NULL);

	(void) sh5_fmt_msd10_decode_op(FMT_MSD10_OP_R, r, op1);

	if (sh5_fmt_msd10_decode_op(fp->op_imm, imm, op2) < 0)
		return (NULL);

	if (sh5_fmt_msd10_decode_op(fp->op_sd, sd, op3) < 0)
		return (NULL);

	return (fp->mnemonic);
}

static int
sh5_fmt_msd10_decode_op(int fmt, int op, char *ops)
{
	char opstr[16];

	switch (fmt) {
	case FMT_MSD10_OP_R:
		sprintf(opstr, "r%d", op);
		break;

	case FMT_MSD10_OP_F:
		sprintf(opstr, "fr%d", op);
		break;

	case FMT_MSD10_OP_D:
		sprintf(opstr, "dr%d", op);
		break;

	case FMT_MSD10_OP_SIMM:
	case FMT_MSD10_OP_SIMM2:
	case FMT_MSD10_OP_SIMM4:
	case FMT_MSD10_OP_SIMM8:
		op = sh5_sign_extend(op, 10);
		op *= 1 << (fmt - FMT_MSD10_OP_SIMM);
		sprintf(opstr, "%d", op);
		break;

	default:
		return (-1);
	}

	strcpy(ops, opstr);
	return (0);
}

static const char *
sh5_fmt_xsd16(opcode_t op, db_addr_t loc, char *op1, char *op2, char *op3)
{
	const struct format_xsd16 *fp;
	static char trl[16];
	int imm, d;

	fp = &format_xsd16[FMT_XSD16_MAJ_INDEX(op)];

	imm = FMT_XSD16_IMM(op);
	d = FMT_XSD16_D(op);

	if (fp->mnemonic == NULL)
		return (NULL);

	if (sh5_fmt_xsd16_decode_op(fp->op_imm, imm, d, loc, op1) < 0)
		return (NULL);

	if (sh5_fmt_xsd16_decode_op(fp->op_d, d, 0, 0, op2) < 0)
		return (NULL);

	op3[0] = '\0';

	if (fp->op_d == FMT_XSD16_OP_TRL) {
		sprintf(trl, "%s/%c", fp->mnemonic, (d & 0x20) ? 'l' : 'u');
		return (trl);
	}

	return (fp->mnemonic);
}

static int
sh5_fmt_xsd16_decode_op(int fmt, int op, int d, db_addr_t loc, char *ops)
{
	char *symname;
	db_sym_t sym;
	db_expr_t diff;
	opcode_t nextop;
	char accmovi_str[32];
	static db_addr_t last_movi;
	static int last_d = -1;
	static int64_t accmovi;

	switch (fmt) {
	case FMT_XSD16_OP_R:
		sprintf(ops, "r%d", op);
		break;

	case FMT_XSD16_OP_TRL:
		if ((op & 0x18) != 0)
			return (-1);
		sprintf(ops, "tr%d", op & 0x7);
		break;

	case FMT_XSD16_OP_SHORI:
		sprintf(ops, "%d", op);
		if ((last_movi + 4) == loc && last_d == d) {
			accmovi <<= 16;
			accmovi |= op;

			if ((loc + 4) < SH5_KSEG0_BASE)
				nextop = fuword((void *)(intptr_t)(loc + 4));
			else
				nextop = *((opcode_t *)(intptr_t)(loc + 4));

			if ((nextop & 0xfc00000f) == 0xc8000000 &&
			    ((nextop >> 4) & 0x3f) == d) {
				last_movi = loc;
			} else {
				symname = NULL;
				sym = db_search_symbol((db_addr_t)accmovi,
				    DB_STGY_PROC, &diff);
				db_symbol_values(sym, &symname, NULL);

				if (symname == NULL || symname[0] == '/') {
					sym = db_search_symbol(
					    (db_addr_t)accmovi,
					    DB_STGY_XTRN, &diff);
					db_symbol_values(sym, &symname, NULL);
					if (symname && symname[0] == '/')
						symname = NULL;
				} else
					diff &= ~1;

				if ((u_int64_t)accmovi >= 0x100000000ULL) {
					sprintf(accmovi_str, "0x%08x%08x",
					    (u_int)(accmovi >> 32),
					    (u_int)accmovi);
				} else
					sprintf(accmovi_str, "0x%08x",
					    (u_int)accmovi);

				if (symname == NULL || diff >= 0x400000)
					strcpy(extra_info, accmovi_str);
				else {
					if (diff)
						sprintf(extra_info,
						    "%s <%s+0x%x>",
						    accmovi_str, symname,
						    (int)diff);
					else
						sprintf(extra_info, "%s <%s>",
						    accmovi_str, symname);
				}
			}
		}
		break;

	case FMT_XSD16_OP_MOVI:
		op = sh5_sign_extend(op, 16);
		sprintf(ops, "%d", op);
		last_movi = loc;
		last_d = d;
		accmovi = (int64_t)op;
		break;

	case FMT_XSD16_OP_LABEL:
		op = sh5_sign_extend(op, 16) * 4;
		loc = (long)loc + (long)op;
		symname = NULL;
		sym = db_search_symbol(loc, DB_STGY_PROC, &diff);
		db_symbol_values(sym, &symname, NULL);
		if (symname == NULL)
			sprintf(ops, "0x%lx", loc);
		else {
			if (diff)
				sprintf(ops, "%s+0x%x", symname, (int) diff);
			else
				strcpy(ops, symname);
		}
		break;

	default:
		return (-1);
	}

	return (0);
}

static int
sh5_sign_extend(int imm, int bits)
{
	if (imm & (1 << (bits - 1)))
		imm |= (-1 << bits);

	return (imm);
}
