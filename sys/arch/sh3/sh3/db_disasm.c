/*	$NetBSD: db_disasm.c,v 1.19.4.1 2007/02/27 16:52:59 yamt Exp $	*/

/*
 * Copyright (c) 1998-2000 Internet Initiative Japan Inc.
 * All rights reserved.
 *
 * Author: Akinori Koketsu
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistribution with functional modification must include
 *    prominent notice stating how and when and by whom it is
 *    modified.
 * 3. Redistributions in binary form have to be along with the source
 *    code or documentation which include above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 4. All commercial advertising materials mentioning features or use
 *    of this software must display the following acknowledgement:
 *      This product includes software developed by Internet
 *      Initiative Japan Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_disasm.c,v 1.19.4.1 2007/02/27 16:52:59 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>

#include <ddb/db_command.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

static uint16_t	get_insn(const void *);

static void	get_opcode(const uint16_t *, char *);

static void	f_02(const uint16_t *, char *);
static void	f_03(const uint16_t *, char *);
static void	f_04(const uint16_t *, char *);
static void	f_08(const uint16_t *, char *);
static void	f_09(const uint16_t *, char *);
static void	f_0a(const uint16_t *, char *);
static void	f_0b(const uint16_t *, char *);
static void	f_0c(const uint16_t *, char *);
static void	f_10(const uint16_t *, char *);
static void	f_20(const uint16_t *, char *);
static void	f_24(const uint16_t *, char *);
static void	f_28(const uint16_t *, char *);
static void	f_2c(const uint16_t *, char *);
static void	f_30(const uint16_t *, char *);
static void	f_34(const uint16_t *, char *);
static void	f_38(const uint16_t *, char *);
static void	f_3c(const uint16_t *, char *);
static void	f_40(const uint16_t *, char *);
static void	f_41(const uint16_t *, char *);
static void	f_42(const uint16_t *, char *);
static void	f_43(const uint16_t *, char *);
static void	f_44(const uint16_t *, char *);
static void	f_45(const uint16_t *, char *);
static void	f_46(const uint16_t *, char *);
static void	f_47(const uint16_t *, char *);
static void	f_48(const uint16_t *, char *);
static void	f_49(const uint16_t *, char *);
static void	f_4a(const uint16_t *, char *);
static void	f_4b(const uint16_t *, char *);
static void	f_4c(const uint16_t *, char *);
static void	f_4d(const uint16_t *, char *);
static void	f_4e(const uint16_t *, char *);
static void	f_4f(const uint16_t *, char *);
static void	f_50(const uint16_t *, char *);
static void	f_60(const uint16_t *, char *);
static void	f_64(const uint16_t *, char *);
static void	f_68(const uint16_t *, char *);
static void	f_6c(const uint16_t *, char *);
static void	f_70(const uint16_t *, char *);
static void	f_80(const uint16_t *, char *);
static void	f_90(const uint16_t *, char *);
static void	f_a0(const uint16_t *, char *);
static void	f_b0(const uint16_t *, char *);
static void	f_c0(const uint16_t *, char *);
static void	f_d0(const uint16_t *, char *);
static void	f_e0(const uint16_t *, char *);
static void	f_f0(const uint16_t *, char *);
static void	f_f4(const uint16_t *, char *);
static void	f_f8(const uint16_t *, char *);
static void	f_fc(const uint16_t *, char *);
static void	f_fd(const uint16_t *, char *);
static void	f_fe(const uint16_t *, char *);

typedef	void (*rasm_t)(const uint16_t *, char *);
static	rasm_t	f[16][16] = {
	{ /* [0][0-7] */	NULL, NULL, f_02, f_03, f_04, f_04, f_04, f_04,
	  /* [0][8-f] */	f_08, f_09, f_0a, f_0b, f_0c, f_0c, f_0c, f_0c },
	{ /* [1][0-7] */	f_10, f_10, f_10, f_10, f_10, f_10, f_10, f_10,
	  /* [1][8-f] */	f_10, f_10, f_10, f_10, f_10, f_10, f_10, f_10 },
	{ /* [2][0-7] */	f_20, f_20, f_20, f_20, f_24, f_24, f_24, f_24,
	  /* [2][8-f] */	f_28, f_28, f_28, f_28, f_2c, f_2c, f_2c, f_2c },
	{ /* [3][0-7] */	f_30, f_30, f_30, f_30, f_34, f_34, f_34, f_34,
	  /* [3][8-f] */	f_38, f_38, f_38, f_38, f_3c, f_3c, f_3c, f_3c },
	{ /* [4][0-7] */	f_40, f_41, f_42, f_43, f_44, f_45, f_46, f_47,
	  /* [4][8-f] */	f_48, f_49, f_4a, f_4b, f_4c, f_4d, f_4e, f_4f },
	{ /* [5][0-7] */	f_50, f_50, f_50, f_50, f_50, f_50, f_50, f_50,
	  /* [5][8-f] */	f_50, f_50, f_50, f_50, f_50, f_50, f_50, f_50 },
	{ /* [6][0-7] */	f_60, f_60, f_60, f_60, f_64, f_64, f_64, f_64,
	  /* [6][8-f] */	f_68, f_68, f_68, f_68, f_6c, f_6c, f_6c, f_6c },
	{ /* [7][0-7] */	f_70, f_70, f_70, f_70, f_70, f_70, f_70, f_70,
	  /* [7][8-f] */	f_70, f_70, f_70, f_70, f_70, f_70, f_70, f_70 },
	{ /* [8][0-7] */	f_80, f_80, f_80, f_80, f_80, f_80, f_80, f_80,
	  /* [8][8-f] */	f_80, f_80, f_80, f_80, f_80, f_80, f_80, f_80 },
	{ /* [9][0-7] */	f_90, f_90, f_90, f_90, f_90, f_90, f_90, f_90,
	  /* [9][8-f] */	f_90, f_90, f_90, f_90, f_90, f_90, f_90, f_90 },
	{ /* [a][0-7] */	f_a0, f_a0, f_a0, f_a0, f_a0, f_a0, f_a0, f_a0,
	  /* [a][8-f] */	f_a0, f_a0, f_a0, f_a0, f_a0, f_a0, f_a0, f_a0 },
	{ /* [b][0-7] */	f_b0, f_b0, f_b0, f_b0, f_b0, f_b0, f_b0, f_b0,
	  /* [b][8-f] */	f_b0, f_b0, f_b0, f_b0, f_b0, f_b0, f_b0, f_b0 },
	{ /* [c][0-7] */	f_c0, f_c0, f_c0, f_c0, f_c0, f_c0, f_c0, f_c0,
	  /* [c][8-f] */	f_c0, f_c0, f_c0, f_c0, f_c0, f_c0, f_c0, f_c0 },
	{ /* [d][0-7] */	f_d0, f_d0, f_d0, f_d0, f_d0, f_d0, f_d0, f_d0,
	  /* [d][8-f] */	f_d0, f_d0, f_d0, f_d0, f_d0, f_d0, f_d0, f_d0 },
	{ /* [e][0-7] */	f_e0, f_e0, f_e0, f_e0, f_e0, f_e0, f_e0, f_e0,
	  /* [e][8-f] */	f_e0, f_e0, f_e0, f_e0, f_e0, f_e0, f_e0, f_e0 },
	{ /* [f][0-7] */	f_f0, f_f0, f_f0, f_f0, f_f4, f_f4, f_f4, f_f4,
	  /* [f][8-f] */	f_f8, f_f8, f_f8, f_f8, f_fc, f_fd, f_fe, NULL }
};

db_addr_t
db_disasm(db_addr_t loc, bool altfmt)
{
	const void *pc = (void *)loc;
	char line[40];

	get_opcode(pc, line);
	db_printf("%s\n", line);

	return (loc + 2);
}


static uint16_t
get_insn(const void *pc)
{
	vaddr_t addr = (uintptr_t)pc;
	uint16_t insn;
	int retval;

	if (addr & 1)
		db_error("Instruction address not aligned\n");

	if (addr >= SH3_P4SEG_BASE) /* p4: on-chip i/o registers */
		db_error("Instruction address in P4 area\n");

	if ((int)addr >= 0) {	/* p0: user-space */
		retval = fusword(pc);
		if (retval < 0)
			db_error("Instruction fetch fault (user)\n");
		insn = (uint16_t)retval;
	}
	else {			/* kernel p1/p2/p3 */
		retval = kcopy(pc, &insn, sizeof(insn));
		if (retval != 0)
			db_error("Instruction fetch fault (kernel)\n");
	}

	return insn;
}

static void
get_opcode(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int n0, n3;

	strcpy(buf, "????");

	n0 = (insn & 0xf000) >> 12;
	n3 = (insn & 0x000f);

	if (f[n0][n3] != NULL) {
		(*f[n0][n3])(pc, buf);
	}
}

static void
f_02(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, type, md;

	rn   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "stc     sr, r%d", rn);
			break;

		case 1:
			sprintf(buf, "stc     gbr, r%d", rn);
			break;

		case 2:
			sprintf(buf, "stc     vbr, r%d", rn);
			break;

		case 3:
			sprintf(buf, "stc     ssr, r%d", rn);
			break;

		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "stc     spc, r%d", rn);
			break;
		}
		break;

	case 2:
		sprintf(buf, "stc     r%d_bank, r%d", md, rn);
		break;

	case 3:
		sprintf(buf, "stc     r%d_bank, r%d", md+4, rn);
		break;
	} /* end of switch (type) */
}

static void
f_03(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, type, md;

	rn   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "bsrf    r%d", rn);
			break;

		case 2:
			sprintf(buf, "braf    r%d", rn);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
			sprintf(buf, "pref    @r%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}


static void
f_04(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "mov.b   r%d, @(r0, r%d)", rm, rn);
		break;

	case 1:
		sprintf(buf, "mov.w   r%d, @(r0, r%d)", rm, rn);
		break;

	case 2:
		sprintf(buf, "mov.l   r%d, @(r0, r%d)", rm, rn);
		break;

	case 3:
		sprintf(buf, "mul.l   r%d, r%d)", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_08(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int n1, type, md;

	n1   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	if (n1 != 0)
		return;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "clrt");
			break;

		case 1:
			sprintf(buf, "sett");
			break;

		case 2:
			sprintf(buf, "clrmac");
			break;

		case 3:
			sprintf(buf, "ldtlb");
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "clrs");
			break;

		case 1:
			sprintf(buf, "sets");
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_09(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn = (insn & 0x0f00) >> 8;
	fx = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		if (rn != 0)
			return;
		sprintf(buf, "nop");
		break;

	case 1:
		if (rn != 0)
			return;
		sprintf(buf, "div0u");
		break;

	case 2:
		sprintf(buf, "movt    r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_0a(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, type, md;

	rn   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "sts     mach, r%d", rn);
			break;

		case 1:
			sprintf(buf, "sts     macl, r%d", rn);
			break;

		case 2:
			sprintf(buf, "sts     pr, r%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			sprintf(buf, "sts     fpul, r%d", rn);
			break;

		case 2:
			sprintf(buf, "sts     fpscr, r%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_0b(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int n1, fx;

	n1 = (insn & 0x0f00) >> 8;
	if (n1 != 0)
		return;

	fx = (insn & 0x00f0) >> 4;
	switch (fx) {
	case 0:
		sprintf(buf, "rts");
		break;

	case 1:
		sprintf(buf, "sleep");
		break;

	case 2:
		sprintf(buf, "rte");
		break;
	} /* end of switch (fx) */
}

static void
f_0c(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "mov.b   @(r0, r%d), r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "mov.w   @(r0, r%d), r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "mov.l   @(r0, r%d), r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "mac.l   @r%d+, r%d+", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_10(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, disp;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	disp = (insn & 0x000f);
	disp *= 4;

	sprintf(buf, "mov.l   r%d, @(%d, r%d)", rm, disp, rn);
}

static void
f_20(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "mov.b   r%d, @r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "mov.w   r%d, @r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "mov.l   r%d, @r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_24(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "mov.b   r%d, @-r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "mov.w   r%d, @-r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "mov.l   r%d, @-r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "div0s   r%d, r%d)", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_28(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "tst     r%d, r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "and     r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "xor     r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "or      r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_2c(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "cmp/str r%d, r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "xtrct   r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "mulu.w  r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "muls.w  r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_30(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "cmp/eq  r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "cmp/hs  r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "cmp/ge  r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_34(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "div1    r%d, r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "dmulu.l r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "cmp/hi  r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "cmp/gt  r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_38(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "sub     r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "subc    r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "subv    r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_3c(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "add     r%d, r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "dmulu.l r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "addc    r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "addv    r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_40(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "shll    r%d", rn);
		break;

	case 1:
		sprintf(buf, "dt      r%d", rn);
		break;

	case 2:
		sprintf(buf, "shal    r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_41(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "shlr    r%d", rn);
		break;

	case 1:
		sprintf(buf, "cmp/pz  r%d", rn);
		break;

	case 2:
		sprintf(buf, "shar    r%d", rn);
		break;
	} /* end of switch (fx) */
}


static void
f_42(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, type, md;

	rn   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "sts.l   mach, @-r%d", rn);
			break;

		case 1:
			sprintf(buf, "sts.l   macl, @-r%d", rn);
			break;

		case 2:
			sprintf(buf, "sts.l   pr, @-r%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			sprintf(buf, "sts.l   fpul, @-r%d", rn);
			break;

		case 2:
			sprintf(buf, "sts.l   fpscr, @-r%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_43(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, type, md;

	rn   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "stc.l   sr, @-r%d", rn);
			break;

		case 1:
			sprintf(buf, "stc.l   gbr, @-r%d", rn);
			break;

		case 2:
			sprintf(buf, "stc.l   vbr, @-r%d", rn);
			break;

		case 3:
			sprintf(buf, "stc.l   ssr, @-r%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "stc.l   spc, @-r%d", rn);
			break;
		}
		break;

	case 2:
		sprintf(buf, "stc.l   r%d_bank, @-r%d", md, rn);
		break;

	case 3:
		sprintf(buf, "stc.l   r%d_bank, @-r%d", md+4, rn);
		break;
	} /* end of switch (type) */
}

static void
f_44(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "rotl    r%d", rn);
		break;

	case 2:
		sprintf(buf, "rotcl   r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_45(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "rotr    r%d", rn);
		break;

	case 1:
		sprintf(buf, "cmp/pl  r%d", rn);
		break;

	case 2:
		sprintf(buf, "rotcr   r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_46(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rm, type, md;

	rm   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "lds.l   @r%d+, mach", rm);
			break;

		case 1:
			sprintf(buf, "lds.l   @r%d+, macl", rm);
			break;

		case 2:
			sprintf(buf, "lds.l   @r%d+, pr", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			sprintf(buf, "lds.l   @r%d+, fpul", rm);
			break;

		case 2:
			sprintf(buf, "lds.l   @r%d+, fpscr", rm);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_47(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rm, type, md;

	rm   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "ldc.l   @r%d+, sr", rm);
			break;

		case 1:
			sprintf(buf, "ldc.l   @r%d+, gbr", rm);
			break;

		case 2:
			sprintf(buf, "ldc.l   @r%d+, vbr", rm);
			break;

		case 3:
			sprintf(buf, "ldc.l   @r%d+, ssr", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "ldc.l   @r%d+, spc", rm);
			break;
		}
		break;

	case 2:
		sprintf(buf, "ldc.l   @r%d+, r%d_bank", rm, md);
		break;

	case 3:
		sprintf(buf, "ldc.l   @r%d+, r%d_bank", rm, md+4);
		break;
	} /* end of switch (type) */
}

static void
f_48(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "shll2   r%d", rn);
		break;

	case 1:
		sprintf(buf, "shll8   r%d", rn);
		break;

	case 2:
		sprintf(buf, "shll16  r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_49(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "shlr2   r%d", rn);
		break;

	case 1:
		sprintf(buf, "shlr8   r%d", rn);
		break;

	case 2:
		sprintf(buf, "shlr16  r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_4a(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rm, type, md;

	rm   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "lds     r%d, mach", rm);
			break;

		case 1:
			sprintf(buf, "lds     r%d, macl", rm);
			break;

		case 2:
			sprintf(buf, "lds     r%d, pr", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			sprintf(buf, "lds     r%d, fpul", rm);
			break;

		case 2:
			sprintf(buf, "lds     r%d, fpscr", rm);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_4b(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rm, fx;

	rm   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "jsr     @r%d", rm);
		break;

	case 1:
		sprintf(buf, "tas.b   @r%d", rm);
		break;

	case 2:
		sprintf(buf, "jmp     @r%d", rm);
		break;
	} /* end of switch (fx) */
}

static void
f_4c(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	sprintf(buf, "shad    r%d, r%d", rm, rn);
}

static void
f_4d(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	sprintf(buf, "shld    r%d, r%d", rm, rn);
}

static void
f_4e(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rm, type, md;

	rm   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "ldc     r%d, sr", rm);
			break;

		case 1:
			sprintf(buf, "ldc     r%d, gbr", rm);
			break;

		case 2:
			sprintf(buf, "ldc     r%d, vbr", rm);
			break;

		case 3:
			sprintf(buf, "ldc     r%d, ssr", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "ldc     r%d, spc", rm);
			break;
		}
		break;

	case 2:
		sprintf(buf, "ldc     r%d, r%d_bank", rm, md);
		break;

	case 3:
		sprintf(buf, "ldc     r%d, r%d_bank", rm, md+4);
		break;
	} /* end of switch (type) */
}

static void
f_4f(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	sprintf(buf, "mac.w   @r%d+, @r%d+", rm, rn);
}

static void
f_50(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, disp;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	disp = (insn & 0x000f);
	disp *= 4;

	sprintf(buf, "mov.l   @(%d, r%d), r%d", disp, rm, rn);
}

static void
f_60(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "mov.b   @r%d, r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "mov.w   @r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "mov.l   @r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "mov     r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_64(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "mov.b   @r%d+, r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "mov.w   @r%d+, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "mov.l   @r%d+, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "not     r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_68(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "swap.b  r%d, r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "swap.w  r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "negc    r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "neg     r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_6c(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "extu.b  r%d, r%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "extu.w  r%d, r%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "exts.b  r%d, r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "exts.w  r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_70(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, imm;

	rn   = (insn & 0x0f00) >> 8;
	imm  = (int) ((char) (insn & 0x00ff));

	sprintf(buf, "add     #0x%x, r%d", imm, rn);
}

static void
f_80(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int type, md, rn, disp;

	type = (insn & 0x0c00) >> 10;
	md   = (insn & 0x0300) >> 8;

	switch (type) {
	case 0:
		rn   = (insn & 0x00f0) >> 4;
		disp = (insn & 0x000f);

		switch (md) {
		case 0:
			sprintf(buf, "mov.b   r0, @(%d, r%d)", disp, rn);
			break;

		case 1:
			disp *= 2;
			sprintf(buf, "mov.w   r0, @(%d, r%d)", disp, rn);
			break;
		}
		break;

	case 1:
		rn   = (insn & 0x00f0) >> 4;
		disp = (insn & 0x000f);

		switch (md) {
		case 0:
			sprintf(buf, "mov.b   @(%d, r%d), r0", disp, rn);
			break;

		case 1:
			disp *= 2;
			sprintf(buf, "mov.w   @(%d, r%d), r0", disp, rn);
			break;
		}
		break;

	case 2:
		disp = (insn & 0x00ff);

		switch (md) {
		case 0:
			sprintf(buf, "cmp/eq  #%d, r0", disp);
			break;

		case 1:
			disp = (int) ((char) disp);
			disp *= 2;
			sprintf(buf, "bt      0x%x", disp);
			break;

		case 3:
			disp = (int) ((char) disp);
			disp *= 2;
			sprintf(buf, "bf      0x%x", disp);
			break;
		}
		break;

	case 3:
		disp = (int) ((char) (insn & 0x00ff));
		disp *= 2;

		switch (md) {
		case 1:
			sprintf(buf, "bt/s    0x%x", disp);
			break;

		case 3:
			sprintf(buf, "bf/s    0x%x", disp);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_90(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, disp;

	rn   = (insn & 0x0f00) >> 8;
	disp = (insn & 0x00ff);
	disp *= 2;

	sprintf(buf, "mov.w   @(%d, pc), r%d", disp, rn);
}

static void
f_a0(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int disp;

	disp = (insn & 0x0fff);
	if (disp & 0x0800)	/* negative displacement? */
		disp |= 0xfffff000; /* sign extend */
	disp *= 2;

	sprintf(buf, "bra     %d(0x%x)", disp, disp);
}

static void
f_b0(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int disp;

	disp = (insn & 0x0fff);
	if (disp & 0x0800)	/* negative displacement? */
		disp |= 0xfffff000; /* sign extend */
	disp *= 2;

	sprintf(buf, "bsr     %d(0x%x)", disp, disp);
}

static void
f_c0(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int type, md, imm;

	type = (insn & 0x0c00) >> 10;
	md   = (insn & 0x0300) >> 8;
	imm  = (insn & 0x00ff);

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "mov.b   r0, @(%d, gbr)", imm);
			break;

		case 1:
			imm *= 2;
			sprintf(buf, "mov.w   r0, @(%d, gbr)", imm);
			break;

		case 2:
			imm *= 4;
			sprintf(buf, "mov.l   r0, @(%d, gbr)", imm);
			break;

		case 3:
			sprintf(buf, "trapa   #%d", imm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "mov.b   @(%d, gbr), r0", imm);
			break;

		case 1:
			imm *= 2;
			sprintf(buf, "mov.w   @(%d, gbr), r0", imm);
			break;

		case 2:
			imm *= 4;
			sprintf(buf, "mov.l   @(%d, gbr), r0", imm);
			break;

		case 3:
			imm *= 4;
			sprintf(buf, "mova    @(%d, pc), r0", imm);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
			sprintf(buf, "tst     #%d, r0", imm);
			break;

		case 1:
			sprintf(buf, "and     #%d, r0", imm);
			break;

		case 2:
			sprintf(buf, "xor     #%d, r0", imm);
			break;

		case 3:
			sprintf(buf, "or      #%d, r0", imm);
			break;
		}
		break;

	case 3:
		switch (md) {
		case 0:
			sprintf(buf, "tst.b   #%d, @(r0, gbr)", imm);
			break;

		case 1:
			sprintf(buf, "and.b   #%d, @(r0, gbr)", imm);
			break;

		case 2:
			sprintf(buf, "xor.b   #%d, @(r0, gbr)", imm);
			break;

		case 3:
			sprintf(buf, "or.b    #%d, @(r0, gbr)", imm);
			break;
		}
		break;
	} /* end of switch (type) */
}


static void
f_d0(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, disp;

	rn   = (insn & 0x0f00) >> 8;
	disp = (insn & 0x00ff);
	disp *= 4;

	sprintf(buf, "mov.l   @(%d, pc), r%d", disp, rn);
}

static void
f_e0(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, imm;

	rn   = (insn & 0x0f00) >> 8;
	imm  = (int) ((char) (insn & 0x00ff));

	sprintf(buf, "mov     #0x%x, r%d", imm, rn);
}

static void
f_f0(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "fadd    fr%d, fr%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "fsub    fr%d, fr%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "fmul    fr%d, fr%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "fdiv    fr%d, fr%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_f4(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "fcmp/eq fr%d, fr%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "fcmp/gt fr%d, fr%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "fmov.s  @(r0, r%d), fr%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "fmov.s  fr%d, @(r0, r%d)", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_f8(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "fmov.s  @r%d, fr%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "fmov.s  @r%d+, fr%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "fmov.s  fr%d, @r%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "fmov.s  fr%d, @-r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_fc(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;

	sprintf(buf, "fmov    fr%d, fr%d", rm, rn);
}

static void
f_fd(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, type, md;

	rn   = (insn & 0x0f00) >> 8;
	type = (insn & 0x00c0) >> 6;
	md   = (insn & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "fsts    fpul, fr%d", rn);
			break;

		case 1:
			sprintf(buf, "flds    fr%d, fpul", rn);
			break;

		case 2:
			sprintf(buf, "float   fpul, fr%d", rn);
			break;

		case 3:
			sprintf(buf, "ftrc    fr%d, fpul", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "fneg    fr%d", rn);
			break;

		case 1:
			sprintf(buf, "fabs    fr%d", rn);
			break;

		case 2:
			sprintf(buf, "fsqrt   fr%d", rn);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
		case 1:
			sprintf(buf, "fldi%d   fr%d", md, rn);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_fe(const uint16_t *pc, char *buf)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;

	sprintf(buf, "fmac    fr0, fr%d, fr%d", rm, rn);
}
