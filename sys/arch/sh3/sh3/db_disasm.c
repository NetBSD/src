/*	$NetBSD: db_disasm.c,v 1.21.42.1 2014/08/20 00:03:23 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: db_disasm.c,v 1.21.42.1 2014/08/20 00:03:23 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>

#include <ddb/db_command.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

static uint16_t	get_insn(const void *);

static void	get_opcode(const uint16_t *, char *, size_t);

static void	f_02(const uint16_t *, char *, size_t);
static void	f_03(const uint16_t *, char *, size_t);
static void	f_04(const uint16_t *, char *, size_t);
static void	f_08(const uint16_t *, char *, size_t);
static void	f_09(const uint16_t *, char *, size_t);
static void	f_0a(const uint16_t *, char *, size_t);
static void	f_0b(const uint16_t *, char *, size_t);
static void	f_0c(const uint16_t *, char *, size_t);
static void	f_10(const uint16_t *, char *, size_t);
static void	f_20(const uint16_t *, char *, size_t);
static void	f_24(const uint16_t *, char *, size_t);
static void	f_28(const uint16_t *, char *, size_t);
static void	f_2c(const uint16_t *, char *, size_t);
static void	f_30(const uint16_t *, char *, size_t);
static void	f_34(const uint16_t *, char *, size_t);
static void	f_38(const uint16_t *, char *, size_t);
static void	f_3c(const uint16_t *, char *, size_t);
static void	f_40(const uint16_t *, char *, size_t);
static void	f_41(const uint16_t *, char *, size_t);
static void	f_42(const uint16_t *, char *, size_t);
static void	f_43(const uint16_t *, char *, size_t);
static void	f_44(const uint16_t *, char *, size_t);
static void	f_45(const uint16_t *, char *, size_t);
static void	f_46(const uint16_t *, char *, size_t);
static void	f_47(const uint16_t *, char *, size_t);
static void	f_48(const uint16_t *, char *, size_t);
static void	f_49(const uint16_t *, char *, size_t);
static void	f_4a(const uint16_t *, char *, size_t);
static void	f_4b(const uint16_t *, char *, size_t);
static void	f_4c(const uint16_t *, char *, size_t);
static void	f_4d(const uint16_t *, char *, size_t);
static void	f_4e(const uint16_t *, char *, size_t);
static void	f_4f(const uint16_t *, char *, size_t);
static void	f_50(const uint16_t *, char *, size_t);
static void	f_60(const uint16_t *, char *, size_t);
static void	f_64(const uint16_t *, char *, size_t);
static void	f_68(const uint16_t *, char *, size_t);
static void	f_6c(const uint16_t *, char *, size_t);
static void	f_70(const uint16_t *, char *, size_t);
static void	f_80(const uint16_t *, char *, size_t);
static void	f_90(const uint16_t *, char *, size_t);
static void	f_a0(const uint16_t *, char *, size_t);
static void	f_b0(const uint16_t *, char *, size_t);
static void	f_c0(const uint16_t *, char *, size_t);
static void	f_d0(const uint16_t *, char *, size_t);
static void	f_e0(const uint16_t *, char *, size_t);
static void	f_f0(const uint16_t *, char *, size_t);
static void	f_f4(const uint16_t *, char *, size_t);
static void	f_f8(const uint16_t *, char *, size_t);
static void	f_fc(const uint16_t *, char *, size_t);
static void	f_fd(const uint16_t *, char *, size_t);
static void	f_fe(const uint16_t *, char *, size_t);

typedef	void (*rasm_t)(const uint16_t *, char *, size_t);
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

	get_opcode(pc, line, sizeof(line));
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
get_opcode(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int n0, n3;

	strlcpy(buf, "????", len);

	n0 = (insn & 0xf000) >> 12;
	n3 = (insn & 0x000f);

	if (f[n0][n3] != NULL) {
		(*f[n0][n3])(pc, buf, len);
	}
}

static void
f_02(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "stc     sr, r%d", rn);
			break;

		case 1:
			snprintf(buf, len, "stc     gbr, r%d", rn);
			break;

		case 2:
			snprintf(buf, len, "stc     vbr, r%d", rn);
			break;

		case 3:
			snprintf(buf, len, "stc     ssr, r%d", rn);
			break;

		}
		break;

	case 1:
		switch (md) {
		case 0:
			snprintf(buf, len, "stc     spc, r%d", rn);
			break;
		}
		break;

	case 2:
		snprintf(buf, len, "stc     r%d_bank, r%d", md, rn);
		break;

	case 3:
		snprintf(buf, len, "stc     r%d_bank, r%d", md+4, rn);
		break;
	} /* end of switch (type) */
}

static void
f_03(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "bsrf    r%d", rn);
			break;

		case 2:
			snprintf(buf, len, "braf    r%d", rn);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
			snprintf(buf, len, "pref    @r%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}


static void
f_04(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "mov.b   r%d, @(r0, r%d)", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "mov.w   r%d, @(r0, r%d)", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "mov.l   r%d, @(r0, r%d)", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "mul.l   r%d, r%d)", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_08(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "clrt");
			break;

		case 1:
			snprintf(buf, len, "sett");
			break;

		case 2:
			snprintf(buf, len, "clrmac");
			break;

		case 3:
			snprintf(buf, len, "ldtlb");
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			snprintf(buf, len, "clrs");
			break;

		case 1:
			snprintf(buf, len, "sets");
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_09(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn = (insn & 0x0f00) >> 8;
	fx = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		if (rn != 0)
			return;
		snprintf(buf, len, "nop");
		break;

	case 1:
		if (rn != 0)
			return;
		snprintf(buf, len, "div0u");
		break;

	case 2:
		snprintf(buf, len, "movt    r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_0a(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "sts     mach, r%d", rn);
			break;

		case 1:
			snprintf(buf, len, "sts     macl, r%d", rn);
			break;

		case 2:
			snprintf(buf, len, "sts     pr, r%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			snprintf(buf, len, "sts     fpul, r%d", rn);
			break;

		case 2:
			snprintf(buf, len, "sts     fpscr, r%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_0b(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int n1, fx;

	n1 = (insn & 0x0f00) >> 8;
	if (n1 != 0)
		return;

	fx = (insn & 0x00f0) >> 4;
	switch (fx) {
	case 0:
		snprintf(buf, len, "rts");
		break;

	case 1:
		snprintf(buf, len, "sleep");
		break;

	case 2:
		snprintf(buf, len, "rte");
		break;
	} /* end of switch (fx) */
}

static void
f_0c(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "mov.b   @(r0, r%d), r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "mov.w   @(r0, r%d), r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "mov.l   @(r0, r%d), r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "mac.l   @r%d+, r%d+", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_10(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, disp;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	disp = (insn & 0x000f);
	disp *= 4;

	snprintf(buf, len, "mov.l   r%d, @(%d, r%d)", rm, disp, rn);
}

static void
f_20(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "mov.b   r%d, @r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "mov.w   r%d, @r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "mov.l   r%d, @r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_24(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "mov.b   r%d, @-r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "mov.w   r%d, @-r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "mov.l   r%d, @-r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "div0s   r%d, r%d)", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_28(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "tst     r%d, r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "and     r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "xor     r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "or      r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_2c(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "cmp/str r%d, r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "xtrct   r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "mulu.w  r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "muls.w  r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_30(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "cmp/eq  r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "cmp/hs  r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "cmp/ge  r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_34(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "div1    r%d, r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "dmulu.l r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "cmp/hi  r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "cmp/gt  r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_38(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "sub     r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "subc    r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "subv    r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_3c(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "add     r%d, r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "dmulu.l r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "addc    r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "addv    r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static void
f_40(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		snprintf(buf, len, "shll    r%d", rn);
		break;

	case 1:
		snprintf(buf, len, "dt      r%d", rn);
		break;

	case 2:
		snprintf(buf, len, "shal    r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_41(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		snprintf(buf, len, "shlr    r%d", rn);
		break;

	case 1:
		snprintf(buf, len, "cmp/pz  r%d", rn);
		break;

	case 2:
		snprintf(buf, len, "shar    r%d", rn);
		break;
	} /* end of switch (fx) */
}


static void
f_42(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "sts.l   mach, @-r%d", rn);
			break;

		case 1:
			snprintf(buf, len, "sts.l   macl, @-r%d", rn);
			break;

		case 2:
			snprintf(buf, len, "sts.l   pr, @-r%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			snprintf(buf, len, "sts.l   fpul, @-r%d", rn);
			break;

		case 2:
			snprintf(buf, len, "sts.l   fpscr, @-r%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_43(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "stc.l   sr, @-r%d", rn);
			break;

		case 1:
			snprintf(buf, len, "stc.l   gbr, @-r%d", rn);
			break;

		case 2:
			snprintf(buf, len, "stc.l   vbr, @-r%d", rn);
			break;

		case 3:
			snprintf(buf, len, "stc.l   ssr, @-r%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			snprintf(buf, len, "stc.l   spc, @-r%d", rn);
			break;
		}
		break;

	case 2:
		snprintf(buf, len, "stc.l   r%d_bank, @-r%d", md, rn);
		break;

	case 3:
		snprintf(buf, len, "stc.l   r%d_bank, @-r%d", md+4, rn);
		break;
	} /* end of switch (type) */
}

static void
f_44(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		snprintf(buf, len, "rotl    r%d", rn);
		break;

	case 2:
		snprintf(buf, len, "rotcl   r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_45(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		snprintf(buf, len, "rotr    r%d", rn);
		break;

	case 1:
		snprintf(buf, len, "cmp/pl  r%d", rn);
		break;

	case 2:
		snprintf(buf, len, "rotcr   r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_46(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "lds.l   @r%d+, mach", rm);
			break;

		case 1:
			snprintf(buf, len, "lds.l   @r%d+, macl", rm);
			break;

		case 2:
			snprintf(buf, len, "lds.l   @r%d+, pr", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			snprintf(buf, len, "lds.l   @r%d+, fpul", rm);
			break;

		case 2:
			snprintf(buf, len, "lds.l   @r%d+, fpscr", rm);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_47(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "ldc.l   @r%d+, sr", rm);
			break;

		case 1:
			snprintf(buf, len, "ldc.l   @r%d+, gbr", rm);
			break;

		case 2:
			snprintf(buf, len, "ldc.l   @r%d+, vbr", rm);
			break;

		case 3:
			snprintf(buf, len, "ldc.l   @r%d+, ssr", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			snprintf(buf, len, "ldc.l   @r%d+, spc", rm);
			break;
		}
		break;

	case 2:
		snprintf(buf, len, "ldc.l   @r%d+, r%d_bank", rm, md);
		break;

	case 3:
		snprintf(buf, len, "ldc.l   @r%d+, r%d_bank", rm, md+4);
		break;
	} /* end of switch (type) */
}

static void
f_48(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		snprintf(buf, len, "shll2   r%d", rn);
		break;

	case 1:
		snprintf(buf, len, "shll8   r%d", rn);
		break;

	case 2:
		snprintf(buf, len, "shll16  r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_49(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, fx;

	rn   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		snprintf(buf, len, "shlr2   r%d", rn);
		break;

	case 1:
		snprintf(buf, len, "shlr8   r%d", rn);
		break;

	case 2:
		snprintf(buf, len, "shlr16  r%d", rn);
		break;
	} /* end of switch (fx) */
}

static void
f_4a(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "lds     r%d, mach", rm);
			break;

		case 1:
			snprintf(buf, len, "lds     r%d, macl", rm);
			break;

		case 2:
			snprintf(buf, len, "lds     r%d, pr", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			snprintf(buf, len, "lds     r%d, fpul", rm);
			break;

		case 2:
			snprintf(buf, len, "lds     r%d, fpscr", rm);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_4b(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rm, fx;

	rm   = (insn & 0x0f00) >> 8;
	fx   = (insn & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		snprintf(buf, len, "jsr     @r%d", rm);
		break;

	case 1:
		snprintf(buf, len, "tas.b   @r%d", rm);
		break;

	case 2:
		snprintf(buf, len, "jmp     @r%d", rm);
		break;
	} /* end of switch (fx) */
}

static void
f_4c(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	snprintf(buf, len, "shad    r%d, r%d", rm, rn);
}

static void
f_4d(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	snprintf(buf, len, "shld    r%d, r%d", rm, rn);
}

static void
f_4e(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "ldc     r%d, sr", rm);
			break;

		case 1:
			snprintf(buf, len, "ldc     r%d, gbr", rm);
			break;

		case 2:
			snprintf(buf, len, "ldc     r%d, vbr", rm);
			break;

		case 3:
			snprintf(buf, len, "ldc     r%d, ssr", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			snprintf(buf, len, "ldc     r%d, spc", rm);
			break;
		}
		break;

	case 2:
		snprintf(buf, len, "ldc     r%d, r%d_bank", rm, md);
		break;

	case 3:
		snprintf(buf, len, "ldc     r%d, r%d_bank", rm, md+4);
		break;
	} /* end of switch (type) */
}

static void
f_4f(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	snprintf(buf, len, "mac.w   @r%d+, @r%d+", rm, rn);
}

static void
f_50(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, disp;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	disp = (insn & 0x000f);
	disp *= 4;

	snprintf(buf, len, "mov.l   @(%d, r%d), r%d", disp, rm, rn);
}

static void
f_60(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "mov.b   @r%d, r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "mov.w   @r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "mov.l   @r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "mov     r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_64(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "mov.b   @r%d+, r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "mov.w   @r%d+, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "mov.l   @r%d+, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "not     r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_68(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "swap.b  r%d, r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "swap.w  r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "negc    r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "neg     r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_6c(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "extu.b  r%d, r%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "extu.w  r%d, r%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "exts.b  r%d, r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "exts.w  r%d, r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_70(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, imm;

	rn   = (insn & 0x0f00) >> 8;
	imm  = (int) ((char) (insn & 0x00ff));

	snprintf(buf, len, "add     #0x%x, r%d", imm, rn);
}

static void
f_80(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "mov.b   r0, @(%d, r%d)", disp, rn);
			break;

		case 1:
			disp *= 2;
			snprintf(buf, len, "mov.w   r0, @(%d, r%d)", disp, rn);
			break;
		}
		break;

	case 1:
		rn   = (insn & 0x00f0) >> 4;
		disp = (insn & 0x000f);

		switch (md) {
		case 0:
			snprintf(buf, len, "mov.b   @(%d, r%d), r0", disp, rn);
			break;

		case 1:
			disp *= 2;
			snprintf(buf, len, "mov.w   @(%d, r%d), r0", disp, rn);
			break;
		}
		break;

	case 2:
		disp = (insn & 0x00ff);

		switch (md) {
		case 0:
			snprintf(buf, len, "cmp/eq  #%d, r0", disp);
			break;

		case 1:
			disp = (int) ((char) disp);
			disp *= 2;
			snprintf(buf, len, "bt      0x%x", disp);
			break;

		case 3:
			disp = (int) ((char) disp);
			disp *= 2;
			snprintf(buf, len, "bf      0x%x", disp);
			break;
		}
		break;

	case 3:
		disp = (int) ((char) (insn & 0x00ff));
		disp *= 2;

		switch (md) {
		case 1:
			snprintf(buf, len, "bt/s    0x%x", disp);
			break;

		case 3:
			snprintf(buf, len, "bf/s    0x%x", disp);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_90(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, disp;

	rn   = (insn & 0x0f00) >> 8;
	disp = (insn & 0x00ff);
	disp *= 2;

	snprintf(buf, len, "mov.w   @(%d, pc), r%d", disp, rn);
}

static void
f_a0(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int disp;

	disp = (insn & 0x0fff);
	if (disp & 0x0800)	/* negative displacement? */
		disp |= 0xfffff000; /* sign extend */
	disp *= 2;

	snprintf(buf, len, "bra     %d(0x%x)", disp, disp);
}

static void
f_b0(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int disp;

	disp = (insn & 0x0fff);
	if (disp & 0x0800)	/* negative displacement? */
		disp |= 0xfffff000; /* sign extend */
	disp *= 2;

	snprintf(buf, len, "bsr     %d(0x%x)", disp, disp);
}

static void
f_c0(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "mov.b   r0, @(%d, gbr)", imm);
			break;

		case 1:
			imm *= 2;
			snprintf(buf, len, "mov.w   r0, @(%d, gbr)", imm);
			break;

		case 2:
			imm *= 4;
			snprintf(buf, len, "mov.l   r0, @(%d, gbr)", imm);
			break;

		case 3:
			snprintf(buf, len, "trapa   #0x%x", imm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			snprintf(buf, len, "mov.b   @(%d, gbr), r0", imm);
			break;

		case 1:
			imm *= 2;
			snprintf(buf, len, "mov.w   @(%d, gbr), r0", imm);
			break;

		case 2:
			imm *= 4;
			snprintf(buf, len, "mov.l   @(%d, gbr), r0", imm);
			break;

		case 3:
			imm *= 4;
			snprintf(buf, len, "mova    @(%d, pc), r0", imm);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
			snprintf(buf, len, "tst     #%d, r0", imm);
			break;

		case 1:
			snprintf(buf, len, "and     #%d, r0", imm);
			break;

		case 2:
			snprintf(buf, len, "xor     #%d, r0", imm);
			break;

		case 3:
			snprintf(buf, len, "or      #%d, r0", imm);
			break;
		}
		break;

	case 3:
		switch (md) {
		case 0:
			snprintf(buf, len, "tst.b   #%d, @(r0, gbr)", imm);
			break;

		case 1:
			snprintf(buf, len, "and.b   #%d, @(r0, gbr)", imm);
			break;

		case 2:
			snprintf(buf, len, "xor.b   #%d, @(r0, gbr)", imm);
			break;

		case 3:
			snprintf(buf, len, "or.b    #%d, @(r0, gbr)", imm);
			break;
		}
		break;
	} /* end of switch (type) */
}


static void
f_d0(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, disp;

	rn   = (insn & 0x0f00) >> 8;
	disp = (insn & 0x00ff);
	disp *= 4;

	snprintf(buf, len, "mov.l   @(%d, pc), r%d", disp, rn);
}

static void
f_e0(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, imm;

	rn   = (insn & 0x0f00) >> 8;
	imm  = (int) ((char) (insn & 0x00ff));

	snprintf(buf, len, "mov     #0x%x, r%d", imm, rn);
}

static void
f_f0(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "fadd    fr%d, fr%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "fsub    fr%d, fr%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "fmul    fr%d, fr%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "fdiv    fr%d, fr%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_f4(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "fcmp/eq fr%d, fr%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "fcmp/gt fr%d, fr%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "fmov.s  @(r0, r%d), fr%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "fmov.s  fr%d, @(r0, r%d)", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_f8(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm, md;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;
	md   = (insn & 0x0003);

	switch (md) {
	case 0:
		snprintf(buf, len, "fmov.s  @r%d, fr%d", rm, rn);
		break;

	case 1:
		snprintf(buf, len, "fmov.s  @r%d+, fr%d", rm, rn);
		break;

	case 2:
		snprintf(buf, len, "fmov.s  fr%d, @r%d", rm, rn);
		break;

	case 3:
		snprintf(buf, len, "fmov.s  fr%d, @-r%d", rm, rn);
		break;
	} /* end of switch (md) */
}

static void
f_fc(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;

	snprintf(buf, len, "fmov    fr%d, fr%d", rm, rn);
}

static void
f_fd(const uint16_t *pc, char *buf, size_t len)
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
			snprintf(buf, len, "fsts    fpul, fr%d", rn);
			break;

		case 1:
			snprintf(buf, len, "flds    fr%d, fpul", rn);
			break;

		case 2:
			snprintf(buf, len, "float   fpul, fr%d", rn);
			break;

		case 3:
			snprintf(buf, len, "ftrc    fr%d, fpul", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			snprintf(buf, len, "fneg    fr%d", rn);
			break;

		case 1:
			snprintf(buf, len, "fabs    fr%d", rn);
			break;

		case 2:
			snprintf(buf, len, "fsqrt   fr%d", rn);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
		case 1:
			snprintf(buf, len, "fldi%d   fr%d", md, rn);
			break;
		}
		break;
	} /* end of switch (type) */
}

static void
f_fe(const uint16_t *pc, char *buf, size_t len)
{
	uint16_t insn = get_insn(pc);
	int rn, rm;

	rn   = (insn & 0x0f00) >> 8;
	rm   = (insn & 0x00f0) >> 4;

	snprintf(buf, len, "fmac    fr0, fr%d, fr%d", rm, rn);
}
