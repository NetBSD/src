/* $NetBSD: sh3disasm.c,v 1.2 2000/09/04 23:02:43 tsubai Exp $ */

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

/*
 * sh3disasm.c	: SH-3/SH-3E disassembler
 *
 * $Author: tsubai $
 * $Date: 2000/09/04 23:02:43 $
 * $Revision: 1.2 $
 */

#include <sys/param.h>
#ifdef SH3IPL
#include "util.h"
#else
#include <sys/systm.h>
#endif

#ifndef	NULL
#define	NULL	((void *) 0L)
#endif

#define	RASM_LINES	24

extern	void *roundadrs(u_char *, int);

/*
 * function prototypes
 */
#ifdef DDB
void sh3_disasm(void *, char *, char *);
#else
extern	void	show_opcode(u_short *, int);
#endif


/*
 * typedef
 */
typedef	void (*rasm_t)(u_short *, u_char *);


/*
 * local function prototypes
 */
static	void	get_opcode(u_short *, char *);
static	void	get_ascii(u_char *, u_char *);
static	void	f_02(u_short *, u_char *);
static	void	f_03(u_short *, u_char *);
static	void	f_04(u_short *, u_char *);
static	void	f_08(u_short *, u_char *);
static	void	f_09(u_short *, u_char *);
static	void	f_0a(u_short *, u_char *);
static	void	f_0b(u_short *, u_char *);
static	void	f_0c(u_short *, u_char *);
static	void	f_10(u_short *, u_char *);
static	void	f_20(u_short *, u_char *);
static	void	f_24(u_short *, u_char *);
static	void	f_28(u_short *, u_char *);
static	void	f_2c(u_short *, u_char *);
static	void	f_30(u_short *, u_char *);
static	void	f_34(u_short *, u_char *);
static	void	f_38(u_short *, u_char *);
static	void	f_3c(u_short *, u_char *);
static	void	f_40(u_short *, u_char *);
static	void	f_41(u_short *, u_char *);
static	void	f_42(u_short *, u_char *);
static	void	f_43(u_short *, u_char *);
static	void	f_44(u_short *, u_char *);
static	void	f_45(u_short *, u_char *);
static	void	f_46(u_short *, u_char *);
static	void	f_47(u_short *, u_char *);
static	void	f_48(u_short *, u_char *);
static	void	f_49(u_short *, u_char *);
static	void	f_4a(u_short *, u_char *);
static	void	f_4b(u_short *, u_char *);
static	void	f_4c(u_short *, u_char *);
static	void	f_4d(u_short *, u_char *);
static	void	f_4e(u_short *, u_char *);
static	void	f_4f(u_short *, u_char *);
static	void	f_50(u_short *, u_char *);
static	void	f_60(u_short *, u_char *);
static	void	f_64(u_short *, u_char *);
static	void	f_68(u_short *, u_char *);
static	void	f_6c(u_short *, u_char *);
static	void	f_70(u_short *, u_char *);
static	void	f_80(u_short *, u_char *);
static	void	f_90(u_short *, u_char *);
static	void	f_a0(u_short *, u_char *);
static	void	f_b0(u_short *, u_char *);
static	void	f_c0(u_short *, u_char *);
static	void	f_d0(u_short *, u_char *);
static	void	f_e0(u_short *, u_char *);
static	void	f_f0(u_short *, u_char *);
static	void	f_f4(u_short *, u_char *);
static	void	f_f8(u_short *, u_char *);
static	void	f_fc(u_short *, u_char *);
static	void	f_fd(u_short *, u_char *);
static	void	f_fe(u_short *, u_char *);

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


#ifdef DDB
void
sh3_disasm(void *pc, char *line, char *ascii)
{
	get_opcode(pc, line);
	get_ascii(pc, ascii);
}
#else
extern	void
show_opcode(u_short *sp, int lines)
{
	int	i;
	char	opcode[50];
	char	ascii[4];

	for (i = 0; i < lines; i++, sp++) {
		memset(opcode, 0, sizeof(opcode));
		get_opcode(sp, opcode);
		get_ascii((u_char *) sp, ascii);
		printf("0x%p  %04X  %-50s  %s\n", sp, *sp, opcode, ascii);
	}
}
#endif


static	void
get_ascii(u_char *cp, u_char *str)
{
	*str++ = (0x20 <= *cp && *cp <= 0x7f) ? *cp : '.';
	cp++;
	*str++ = (0x20 <= *cp && *cp <= 0x7f) ? *cp : '.';
	*str = '\0';
}


static	void
get_opcode(u_short *sp, char *buf)
{
	int	n0, n3;

	strcpy(buf, "????");

	n0 = (*sp & 0xf000) >> 12;
	n3 = (*sp & 0x000f);

	if (f[n0][n3] != NULL) {
		(*f[n0][n3])(sp, buf);
	}
}


static	void
f_02(u_short *code, u_char *buf)
{
	int	rn, type, md;

	rn   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "STC     SR, R%d", rn);
			break;

		case 1:
			sprintf(buf, "STC     GBR, R%d", rn);
			break;

		case 2:
			sprintf(buf, "STC     VBR, R%d", rn);
			break;

		case 3:
			sprintf(buf, "STC     SSR, R%d", rn);
			break;

		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "STC     SPC, R%d", rn);
			break;
		}
		break;

	case 2:
		sprintf(buf, "STC     R%d_BANK, R%d", md, rn);
		break;

	case 3:
		sprintf(buf, "STC     R%d_BANK, R%d", md+4, rn);
		break;
	} /* end of switch (type) */
}


static	void
f_03(u_short *code, u_char *buf)
{
	int	rn, type, md;

	rn   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "BSRF    R%d", rn);
			break;

		case 2:
			sprintf(buf, "BRAF    R%d", rn);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
			sprintf(buf, "PREF    @R%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_04(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "MOV.B   R%d, @(R0, R%d)", rm, rn);
		break;

	case 1:
		sprintf(buf, "MOV.W   R%d, @(R0, R%d)", rm, rn);
		break;

	case 2:
		sprintf(buf, "MOV.L   R%d, @(R0, R%d)", rm, rn);
		break;

	case 3:
		sprintf(buf, "MUL.L   R%d, R%d)", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_08(u_short *code, u_char *buf)
{
	int	n1, type, md;

	n1   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	if (n1 != 0)	return;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "CLRT");
			break;

		case 1:
			sprintf(buf, "SETT");
			break;

		case 2:
			sprintf(buf, "CLRMAC");
			break;

		case 3:
			sprintf(buf, "LDTLB");
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "CLRS");
			break;

		case 1:
			sprintf(buf, "SETS");
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_09(u_short *code, u_char *buf)
{
	int	rn, fx;

	rn = (*code & 0x0f00) >> 8;
	fx = (*code & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		if (rn != 0)	return;
		sprintf(buf, "NOP");
		break;

	case 1:
		if (rn != 0)	return;
		sprintf(buf, "DIV0U");
		break;

	case 2:
		sprintf(buf, "MOVT    R%d", rn);
		break;
	} /* end of switch (fx) */
}


static	void
f_0a(u_short *code, u_char *buf)
{
	int	rn, type, md;

	rn   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "STS     MACH, R%d", rn);
			break;

		case 1:
			sprintf(buf, "STS     MACL, R%d", rn);
			break;

		case 2:
			sprintf(buf, "STS     PR, R%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			sprintf(buf, "STS     FPUL, R%d", rn);
			break;

		case 2:
			sprintf(buf, "STS     FPSCR, R%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_0b(u_short *code, u_char *buf)
{
	int	n1, fx;

	n1 = (*code & 0x0f00) >> 8;
	if (n1 != 0)	return;

	fx = (*code & 0x00f0) >> 4;
	switch (fx) {
	case 0:
		sprintf(buf, "RTS");
		break;

	case 1:
		sprintf(buf, "SLEEP");
		break;

	case 2:
		sprintf(buf, "RTE");
		break;
	} /* end of switch (fx) */
}


static	void
f_0c(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "MOV.B   @(R0, R%d), R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "MOV.W   @(R0, R%d), R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "MOV.L   @(R0, R%d), R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "MAC.L   @R%d+, R%d+", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_10(u_short *code, u_char *buf)
{
	int	rn, rm, disp;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	disp = (*code & 0x000f);
	disp *= 4;

	sprintf(buf, "MOV.L   R%d, @(%d, R%d)", rm, disp, rn);
}


static	void
f_20(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "MOV.B   R%d, @R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "MOV.W   R%d, @R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "MOV.L   R%d, @R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_24(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "MOV.B   R%d, @-R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "MOV.W   R%d, @-R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "MOV.L   R%d, @-R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "DIV0S   R%d, R%d)", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_28(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "TST     R%d, R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "AND     R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "XOR     R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "OR      R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_2c(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "CMP/STR R%d, R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "XTRCT   R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "MULU.W  R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "MULS.W  R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_30(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "CMP/EQ  R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "CMP/HS  R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "CMP/GE  R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_34(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "DIV1    R%d, R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "DMULU.L R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "CMP/HI  R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "CMP/GT  R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_38(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "SUB     R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "SUBC    R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "SUBV    R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_3c(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "ADD     R%d, R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "DMULU.L R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "ADDC    R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "ADDV    R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_40(u_short *code, u_char *buf)
{
	int	rn, fx;

	rn   = (*code & 0x0f00) >> 8;
	fx   = (*code & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "SHLL    R%d", rn);
		break;

	case 1:
		sprintf(buf, "DT      R%d", rn);
		break;

	case 2:
		sprintf(buf, "SHAL    R%d", rn);
		break;
	} /* end of switch (fx) */
}


static	void
f_41(u_short *code, u_char *buf)
{
	int	rn, fx;

	rn   = (*code & 0x0f00) >> 8;
	fx   = (*code & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "SHLR    R%d", rn);
		break;

	case 1:
		sprintf(buf, "CMP/PZ  R%d", rn);
		break;

	case 2:
		sprintf(buf, "SHAR    R%d", rn);
		break;
	} /* end of switch (fx) */
}


static	void
f_42(u_short *code, u_char *buf)
{
	int	rn, type, md;

	rn   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "STS.L   MACH, @-R%d", rn);
			break;

		case 1:
			sprintf(buf, "STS.L   MACL, @-R%d", rn);
			break;

		case 2:
			sprintf(buf, "STS.L   PR, @-R%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			sprintf(buf, "STS.L   FPUL, @-R%d", rn);
			break;

		case 2:
			sprintf(buf, "STS.L   FPSCR, @-R%d", rn);
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_43(u_short *code, u_char *buf)
{
	int	rn, type, md;

	rn   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "STC.L   SR, @-R%d", rn);
			break;

		case 1:
			sprintf(buf, "STC.L   GBR, @-R%d", rn);
			break;

		case 2:
			sprintf(buf, "STC.L   VBR, @-R%d", rn);
			break;

		case 3:
			sprintf(buf, "STC.L   SSR, @-R%d", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "STC.L   SPC, @-R%d", rn);
			break;
		}
		break;

	case 2:
		sprintf(buf, "STC.L   R%d_BANK, @-R%d", md, rn);
		break;

	case 3:
		sprintf(buf, "STC.L   R%d_BANK, @-R%d", md+4, rn);
		break;
	} /* end of switch (type) */
}


static	void
f_44(u_short *code, u_char *buf)
{
	int	rn, fx;

	rn   = (*code & 0x0f00) >> 8;
	fx   = (*code & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "ROTL    R%d", rn);
		break;

	case 2:
		sprintf(buf, "ROTCL   R%d", rn);
		break;
	} /* end of switch (fx) */
}


static	void
f_45(u_short *code, u_char *buf)
{
	int	rn, fx;

	rn   = (*code & 0x0f00) >> 8;
	fx   = (*code & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "ROTR    R%d", rn);
		break;

	case 1:
		sprintf(buf, "CMP/PL  R%d", rn);
		break;

	case 2:
		sprintf(buf, "ROTCR   R%d", rn);
		break;
	} /* end of switch (fx) */
}


static	void
f_46(u_short *code, u_char *buf)
{
	int	rm, type, md;

	rm   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "LDS.L   @R%d+, MACH", rm);
			break;

		case 1:
			sprintf(buf, "LDS.L   @R%d+, MACL", rm);
			break;

		case 2:
			sprintf(buf, "LDS.L   @R%d+, PR", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			sprintf(buf, "LDS.L   @R%d+, FPUL", rm);
			break;

		case 2:
			sprintf(buf, "LDS.L   @R%d+, FPSCR", rm);
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_47(u_short *code, u_char *buf)
{
	int	rm, type, md;

	rm   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "LDC.L   @R%d+, SR", rm);
			break;

		case 1:
			sprintf(buf, "LDC.L   @R%d+, GBR", rm);
			break;

		case 2:
			sprintf(buf, "LDC.L   @R%d+, VBR", rm);
			break;

		case 3:
			sprintf(buf, "LDC.L   @R%d+, SSR", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "LDC.L   @R%d+, SPC", rm);
			break;
		}
		break;

	case 2:
		sprintf(buf, "LDC.L   @R%d+, R%d_BANK", rm, md);
		break;

	case 3:
		sprintf(buf, "LDC.L   @R%d+, R%d_BANK", rm, md+4);
		break;
	} /* end of switch (type) */
}


static	void
f_48(u_short *code, u_char *buf)
{
	int	rn, fx;

	rn   = (*code & 0x0f00) >> 8;
	fx   = (*code & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "SHLL2   R%d", rn);
		break;

	case 1:
		sprintf(buf, "SHLL8   R%d", rn);
		break;

	case 2:
		sprintf(buf, "SHLL16  R%d", rn);
		break;
	} /* end of switch (fx) */
}


static	void
f_49(u_short *code, u_char *buf)
{
	int	rn, fx;

	rn   = (*code & 0x0f00) >> 8;
	fx   = (*code & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "SHLR2   R%d", rn);
		break;

	case 1:
		sprintf(buf, "SHLR8   R%d", rn);
		break;

	case 2:
		sprintf(buf, "SHLR16  R%d", rn);
		break;
	} /* end of switch (fx) */
}


static	void
f_4a(u_short *code, u_char *buf)
{
	int	rm, type, md;

	rm   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "LDS     R%d, MACH", rm);
			break;

		case 1:
			sprintf(buf, "LDS     R%d, MACL", rm);
			break;

		case 2:
			sprintf(buf, "LDS     R%d, PR", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 1:
			sprintf(buf, "LDS     R%d, FPUL", rm);
			break;

		case 2:
			sprintf(buf, "LDS     R%d, FPSCR", rm);
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_4b(u_short *code, u_char *buf)
{
	int	rm, fx;

	rm   = (*code & 0x0f00) >> 8;
	fx   = (*code & 0x00f0) >> 4;

	switch (fx) {
	case 0:
		sprintf(buf, "JSR     @R%d", rm);
		break;

	case 1:
		sprintf(buf, "TAS.B   @R%d", rm);
		break;

	case 2:
		sprintf(buf, "JMP     @R%d", rm);
		break;
	} /* end of switch (fx) */
}


static	void
f_4c(u_short *code, u_char *buf)
{
	int	rn, rm;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	sprintf(buf, "SHAD    R%d, R%d", rm, rn);
}


static	void
f_4d(u_short *code, u_char *buf)
{
	int	rn, rm;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	sprintf(buf, "SHLD    R%d, R%d", rm, rn);
}


static	void
f_4e(u_short *code, u_char *buf)
{
	int	rm, type, md;

	rm   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "LDC     R%d, SR", rm);
			break;

		case 1:
			sprintf(buf, "LDC     R%d, GBR", rm);
			break;

		case 2:
			sprintf(buf, "LDC     R%d, VBR", rm);
			break;

		case 3:
			sprintf(buf, "LDC     R%d, SSR", rm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "LDC     R%d, SPC", rm);
			break;
		}
		break;

	case 2:
		sprintf(buf, "LDC     R%d, R%d_BANK", rm, md);
		break;

	case 3:
		sprintf(buf, "LDC     R%d, R%d_BANK", rm, md+4);
		break;
	} /* end of switch (type) */
}


static	void
f_4f(u_short *code, u_char *buf)
{
	int	rn, rm;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	sprintf(buf, "MAC.W   @R%d+, @R%d+", rm, rn);
}


static	void
f_50(u_short *code, u_char *buf)
{
	int	rn, rm, disp;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	disp = (*code & 0x000f);
	disp *= 4;

	sprintf(buf, "MOV.L   @(%d, R%d), R%d", disp, rm, rn);
}


static	void
f_60(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "MOV.B   @R%d, R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "MOV.W   @R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "MOV.L   @R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "MOV     R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_64(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "MOV.B   @R%d+, R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "MOV.W   @R%d+, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "MOV.L   @R%d+, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "NOT     R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_68(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "SWAP.B  R%d, R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "SWAP.W  R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "NEGC    R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "NEG     R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_6c(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "EXTU.B  R%d, R%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "EXTU.W  R%d, R%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "EXTS.B  R%d, R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "EXTS.W  R%d, R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_70(u_short *code, u_char *buf)
{
	int	rn, imm;

	rn   = (*code & 0x0f00) >> 8;
	imm  = (int) ((char) (*code & 0x00ff));

	sprintf(buf, "ADD     #0x%X, R%d", imm, rn);
}


static	void
f_80(u_short *code, u_char *buf)
{
	int	type, md, rn, disp;

	type = (*code & 0x0c00) >> 10;
	md   = (*code & 0x0300) >> 8;

	switch (type) {
	case 0:
		rn   = (*code & 0x00f0) >> 4;
		disp = (*code & 0x000f);

		switch (md) {
		case 0:
			sprintf(buf, "MOV.B   R0, @(%d, R%d)", disp, rn);
			break;

		case 1:
			disp *= 2;
			sprintf(buf, "MOV.W   R0, @(%d, R%d)", disp, rn);
			break;
		}
		break;

	case 1:
		rn   = (*code & 0x00f0) >> 4;
		disp = (*code & 0x000f);

		switch (md) {
		case 0:
			sprintf(buf, "MOV.B   @(%d, R%d), R0", disp, rn);
			break;

		case 1:
			disp *= 2;
			sprintf(buf, "MOV.W   @(%d, R%d), R0", disp, rn);
			break;
		}
		break;

	case 2:
		disp = (*code & 0x00ff);

		switch (md) {
		case 0:
			sprintf(buf, "CMP/EQ  #%d, R0", disp);
			break;

		case 1:
			disp = (int) ((char) disp);
			disp *= 2;
			sprintf(buf, "BT      0x%X", disp);
			break;

		case 3:
			disp = (int) ((char) disp);
			disp *= 2;
			sprintf(buf, "BF      0x%X", disp);
			break;
		}
		break;

	case 3:
		disp = (int) ((char) (*code & 0x00ff));
		disp *= 2;

		switch (md) {
		case 1:
			sprintf(buf, "BT/S    0x%X", disp);
			break;

		case 3:
			sprintf(buf, "BF/S    0x%X", disp);
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_90(u_short *code, u_char *buf)
{
	int	rn, disp;

	rn   = (*code & 0x0f00) >> 8;
	disp = (*code & 0x00ff);
	disp *= 2;

	sprintf(buf, "MOV.W   @(%d, PC), R%d", disp, rn);
}


static	void
f_a0(u_short *code, u_char *buf)
{
	int	disp;

	if (*code & 0x0800) {	/* sign = '-' */
		disp = 0xfffff000;
		disp |= (*code & 0x0fff);
	}
	else {			/* sign = '+' */
		disp = (*code & 0x0fff);
	}
	disp *= 2;

	sprintf(buf, "BRA     %d(0x%X)", disp, disp);
}


static	void
f_b0(u_short *code, u_char *buf)
{
	int	disp;

	if (*code & 0x0800) {	/* sign = '-' */
		disp = 0xfffff000;
		disp |= (*code & 0x0fff);
	}
	else {			/* sign = '+' */
		disp = (*code & 0x0fff);
	}
	disp *= 2;

	sprintf(buf, "BSR     %d(0x%X)", disp, disp);
}


static	void
f_c0(u_short *code, u_char *buf)
{
	int	type, md, imm;

	type = (*code & 0x0c00) >> 10;
	md   = (*code & 0x0300) >> 8;
	imm  = (*code & 0x00ff);

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "MOV.B   R0, @(%d, GBR)", imm);
			break;

		case 1:
			imm *= 2;
			sprintf(buf, "MOV.W   R0, @(%d, GBR)", imm);
			break;

		case 2:
			imm *= 4;
			sprintf(buf, "MOV.L   R0, @(%d, GBR)", imm);
			break;

		case 3:
			sprintf(buf, "TRAPA   #%d", imm);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "MOV.B   @(%d, GBR), R0", imm);
			break;

		case 1:
			imm *= 2;
			sprintf(buf, "MOV.W   @(%d, GBR), R0", imm);
			break;

		case 2:
			imm *= 4;
			sprintf(buf, "MOV.L   @(%d, GBR), R0", imm);
			break;

		case 3:
			imm *= 4;
			sprintf(buf, "MOVA    @(%d, PC), R0", imm);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
			sprintf(buf, "TST     #%d, R0", imm);
			break;

		case 1:
			sprintf(buf, "AND     #%d, R0", imm);
			break;

		case 2:
			sprintf(buf, "XOR     #%d, R0", imm);
			break;

		case 3:
			sprintf(buf, "OR      #%d, R0", imm);
			break;
		}
		break;

	case 3:
		switch (md) {
		case 0:
			sprintf(buf, "TST.B   #%d, @(R0, GBR)", imm);
			break;

		case 1:
			sprintf(buf, "AND.B   #%d, @(R0, GBR)", imm);
			break;

		case 2:
			sprintf(buf, "XOR.B   #%d, @(R0, GBR)", imm);
			break;

		case 3:
			sprintf(buf, "OR.B    #%d, @(R0, GBR)", imm);
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_d0(u_short *code, u_char *buf)
{
	int	rn, disp;

	rn   = (*code & 0x0f00) >> 8;
	disp = (*code & 0x00ff);
	disp *= 4;

	sprintf(buf, "MOV.L   @(%d, PC), R%d", disp, rn);
}


static	void
f_e0(u_short *code, u_char *buf)
{
	int	rn, imm;

	rn   = (*code & 0x0f00) >> 8;
	imm  = (int) ((char) (*code & 0x00ff));

	sprintf(buf, "MOV     #0x%X, R%d", imm, rn);
}


static	void
f_f0(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "FADD    FR%d, FR%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "FSUB    FR%d, FR%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "FMUL    FR%d, FR%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "FDIV    FR%d, FR%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_f4(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "FCMP/EQ FR%d, FR%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "FCMP/GT FR%d, FR%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "FMOV.S  @(R0, R%d), FR%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "FMOV.S  FR%d, @(R0, R%d)", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_f8(u_short *code, u_char *buf)
{
	int	rn, rm, md;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;
	md   = (*code & 0x0003);

	switch (md) {
	case 0:
		sprintf(buf, "FMOV.S  @R%d, FR%d", rm, rn);
		break;

	case 1:
		sprintf(buf, "FMOV.S  @R%d+, FR%d", rm, rn);
		break;

	case 2:
		sprintf(buf, "FMOV.S  FR%d, @R%d", rm, rn);
		break;

	case 3:
		sprintf(buf, "FMOV.S  FR%d, @-R%d", rm, rn);
		break;
	} /* end of switch (md) */
}


static	void
f_fc(u_short *code, u_char *buf)
{
	int	rn, rm;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;

	sprintf(buf, "FMOV    FR%d, FR%d", rm, rn);
}


static	void
f_fd(u_short *code, u_char *buf)
{
	int	rn, type, md;

	rn   = (*code & 0x0f00) >> 8;
	type = (*code & 0x00c0) >> 6;
	md   = (*code & 0x0030) >> 4;

	switch (type) {
	case 0:
		switch (md) {
		case 0:
			sprintf(buf, "FSTS    FPUL, FR%d", rn);
			break;

		case 1:
			sprintf(buf, "FLDS    FR%d, FPUL", rn);
			break;

		case 2:
			sprintf(buf, "FLOAT   FPUL, FR%d", rn);
			break;

		case 3:
			sprintf(buf, "FTRC    FR%d, FPUL", rn);
			break;
		}
		break;

	case 1:
		switch (md) {
		case 0:
			sprintf(buf, "FNEG    FR%d", rn);
			break;

		case 1:
			sprintf(buf, "FABS    FR%d", rn);
			break;

		case 2:
			sprintf(buf, "FSQRT   FR%d", rn);
			break;
		}
		break;

	case 2:
		switch (md) {
		case 0:
		case 1:
			sprintf(buf, "FLDI%d   FR%d", md, rn);
			break;
		}
		break;
	} /* end of switch (type) */
}


static	void
f_fe(u_short *code, u_char *buf)
{
	int	rn, rm;

	rn   = (*code & 0x0f00) >> 8;
	rm   = (*code & 0x00f0) >> 4;

	sprintf(buf, "FMAC    FR0, FR%d, FR%d", rm, rn);
}
