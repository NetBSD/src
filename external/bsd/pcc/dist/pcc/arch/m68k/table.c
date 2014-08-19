/*	Id: table.c,v 1.7 2014/04/08 19:51:31 ragge Exp 	*/	
/*	$NetBSD: table.c,v 1.1.1.1.6.2 2014/08/19 23:52:08 tls Exp $	*/
/*
 * Copyright (c) 2014 Anders Magnusson (ragge@ludd.ltu.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


# include "pass2.h"

#define TLL	TLONGLONG|TULONGLONG
#define TAREG	TINT|TSHORT|TCHAR|TUNSIGNED|TUSHORT|TUCHAR
#define SABREG	SAREG|SBREG
#define TFP	TFLOAT|TDOUBLE|TLDOUBLE

# define ANYSIGNED TINT|TSHORT|TCHAR
# define ANYUSIGNED TUNSIGNED|TUSHORT|TUCHAR
# define ANYFIXED ANYSIGNED|ANYUSIGNED
# define TUWORD TUNSIGNED
# define TSWORD TINT
# define TWORD	TUWORD|TSWORD
#define TANYINT TLL|ANYFIXED
#define	 SHINT	SAREG	/* Any integer */
#define	 ININT	INAREG
#define	 SHFL	SCREG	/* shape for long double */
#define	 INFL	INCREG	/* shape for long double */

struct optab table[] = {
/* First entry must be an empty entry */
{ -1, FOREFF, SANY, TANY, SANY, TANY, 0, 0, "", },

/* begin with all these casts */

/* pointer to pointer (same reg class) */
{ PCONV,	INBREG,
	SBREG,		TPOINT,
	SBREG,		TANY,
		0,	RLEFT,
		"", },

/* (u)int -> pointer */
{ PCONV,	INBREG,
	SAREG,		TWORD,
	SBREG,		TANY,
		NBREG,	RESC1,
		"	move.l AL,A1\n", },

/* pointer to int/unsigned */
{ SCONV,	INAREG,
	SBREG,		TPOINT,
	SAREG,		TWORD,
		NAREG,	RESC1,
		"	move.l AL,A1\n", },

/* (u)char -> (u)char */
{ SCONV,	INAREG,
	SAREG,		TCHAR|TUCHAR,
	SAREG,		TCHAR|TUCHAR,
		0,	RLEFT,
		"", },

/* char -> short/ushort */
{ SCONV,	INAREG|INBREG,
	SABREG,		TCHAR,
	SABREG,		TSHORT|TUSHORT,
		0,	RLEFT,
		"	ext.w AL\n", },

/* uchar -> short/ushort */
{ SCONV,	INAREG|INBREG,
	SABREG,		TUCHAR,
	SABREG,		TSHORT|TUSHORT,
		0,	RLEFT,
		"	and.l #255,AL\n", },

/* char -> (u)int */
{ SCONV,	INAREG,
	SAREG,		TCHAR,
	SAREG,		TINT|TUNSIGNED,
		0,	RLEFT,
		"	extb.l AL\n", },

/* uchar -> (u)int */
{ SCONV,	INAREG,
	SAREG,		TUCHAR,
	SAREG,		TINT|TUNSIGNED,
		0,	RLEFT,
		"	and.l #255,AL\n", },

/* char -> (u)longlong */
{ SCONV,	INCREG,
	SAREG|SNAME|SOREG,	TCHAR,
	SCREG,			TLL,
		NCREG,	RESC1,
		"	move.b AL,U1\n	extb.l U1\n"
		"	smi A1\n	extb.l A1\n", },

/* uchar -> (u)longlong */
{ SCONV,	INCREG,
	SAREG|SNAME|SOREG,	TUCHAR,
	SCREG,			TLL,
		NCREG,	RESC1,
		"	move.b AL,U1\n	and.l #255,U1\n	clr.l A1\n", },

/* char -> float/(l)double */
{ SCONV,	INDREG,
	SAREG,		TCHAR,
	SDREG,		TFP,
		NDREG,	RESC1,
		"	fmove.ZL AL,A1\n", },

/* (u)char -> float/(l)double */
{ SCONV,	INDREG,
	SAREG,		TUCHAR,
	SDREG,		TFP,
		NAREG|NDREG,	RESC2,
		"	clr.l A1\n	move.b AL,A1\n	fmove.w A1,A2\n", },

/* (u)short -> (u)char */
{ SCONV,	INAREG,
	SAREG,		TSHORT|TUSHORT,
	SAREG,		TCHAR|TUCHAR,
		0,	RLEFT,
		"", },

/* (u)short -> (u)short */
{ SCONV,	INAREG,
	SAREG,		TSHORT|TUSHORT,
	SAREG,		TSHORT|TUSHORT,
		0,	RLEFT,
		"", },

/* short -> (u)int */
{ SCONV,	INAREG|INBREG,
	SABREG,		TSHORT,
	SABREG,		TINT|TUNSIGNED,
		0,	RLEFT,
		"	ext.l AL\n", },

/* ushort -> (u)int */
{ SCONV,	INAREG|INBREG,
	SABREG,		TUSHORT,
	SABREG,		TINT|TUNSIGNED,
		0,	RLEFT,
		"	and.l #65535,AL\n", },

/* short -> (u)longlong */
{ SCONV,	INCREG,
	SAREG,		TSHORT,
	SCREG,		TLL,
		NCREG,	RESC1,
		"	move AL,U1\n	ext.l U1\n"
		"	smi A1\n	extb.l A1\n", },

/* ushort -> (u)longlong */
{ SCONV,	INCREG,
	SAREG|SNAME|SOREG,	TUSHORT,
	SCREG,			TLL,
		NCREG,	RESC1,
		"	move.l AL,U1\n	and.l #65535,U1\n	clr.l A1\n", },

/* short -> float/(l)double */
{ SCONV,	INDREG,
	SAREG|SNAME|SOREG,	TSHORT,
	SDREG,			TFP,
		NDREG|NDSL,	RESC1,
		"	fmove.w AL,A1\n", },

/* ushort -> float/(l)double */
{ SCONV,	INDREG,
	SAREG|SNAME|SOREG,	TUSHORT,
	SAREG|SDREG,		TFP,
		NAREG|NDREG|NDSL,	RESC2,
		"	move.w AL,A1\n	and.l #65535,A1\n"
		"	fmove.l A1,A2\n", },

/* (u)int -> (u)char */
{ SCONV,	INAREG,
	SAREG,		TWORD,
	SAREG,		TCHAR|TUCHAR,
		0,	RLEFT,
		"	and.l #255,AL\n", },

/* (u)int -> (u)short */
{ SCONV,	INAREG,
	SAREG,		TWORD,
	SAREG,		TSHORT|TUSHORT,
		0,	RLEFT,
		"	and.l #65535,AL\n", },

/* (u)int -> (u)int  - nothing */
{ SCONV,	INAREG,
	SAREG,		TWORD,
	SAREG,		TWORD,
		0,	RLEFT,
		"", },

/* int -> (u)longlong */
{ SCONV,	INCREG,
	SAREG|SOREG|SNAME,	TINT,
	SCREG,			TLL,
		NCREG,	RESC1,
		"	move.l AL,U1\n	smi A1\n	extb.l A1\n", },

/* (u)int -> (u)longlong */
{ SCONV,	INCREG,
	SAREG|SOREG|SNAME,	TUNSIGNED,
	SCREG,			TLL,
		NCREG,	RESC1,
		"	move.l AL,U1\n	clr.l A1\n", },

/* int -> float/(l)double */
{ SCONV,	INDREG,
	SAREG|SNAME|SOREG,	TINT,
	SDREG,			TFP,
		NDREG|NDSL,	RESC1,
		"	fmove.l AL,A1\n", },

/* uint -> double */
{ SCONV,	INDREG,
	SAREG,		TUNSIGNED,
	SDREG,		TFLOAT|TDOUBLE,
		NDREG|NDSL,	RESC1,
		"	fmove.l AL,A1\n"
		"	tst.l AL\n"
		"	jge 1f\n"
		"	fadd.d #0x41f0000000000000,A1\n"
		"1:\n", },

/* (u)longlong -> (u)char/(u)short/(u)int */
{ SCONV,	INAREG,
	SCREG|SOREG|SNAME,	TLL,
	SAREG,			TAREG,
		NAREG,	RESC1,
		"	movl UL,A1\n", },

/* (u)longlong to (u)longlong */
{ SCONV,	INCREG,
	SCREG,	TLL,
	SCREG,	TLL,
		0,	RLEFT,
		"", },

/* float/(l)double -> int/short/char (in reg) */
{ SCONV,	INAREG,
	SDREG,	TFP,
	SAREG,	TINT|TSHORT|TCHAR,
		NAREG,	RESC1,
		"	fmove.ZA AL,A1\n", },

/* float -> unsigned */
{ SCONV,	INAREG,
	SDREG,	TFLOAT,
	SAREG,	TUNSIGNED,
		NAREG|NDREG|NDSL,	RESC1,
		"Z2	fcmp.d #0x4f000000,A2\n"
		"	fjge 2f\n"
		"	fintrz.x A2,A2\n"
		"	fmove.l A2,A1\n"
		"	jra 3f\n"
		"2:	fsub.d #0x4f000000,A2\n"
		"	fintrz.x A2,A2\n"
		"	fmove.l A2,A1\n"
		"	add.l #-2147483648,A1\n3:\n", },

/* float -> (l)double */
{ SCONV,	INDREG,
	SDREG,	TFLOAT,
	SDREG,	TDOUBLE|TLDOUBLE,
		0,	RLEFT,
		"", }, /* in fp regs -> do nothing */

/* double -> ldouble */
{ SCONV,	INDREG,
	SDREG,	TDOUBLE,
	SDREG,	TLDOUBLE,
		0,	RLEFT,
		"", }, /* in fp regs -> do nothing */

/* double -> uchar */
{ SCONV,	INAREG,
	SDREG,	TDOUBLE,
	SAREG,	TUCHAR,
		NAREG|NDREG|NDSL,	RESC1,
		"	fintrz.x AL,A2\n	fmove.w A2,A1\n", },

/* double -> ushort */
{ SCONV,	INAREG,
	SDREG,	TDOUBLE,
	SAREG,	TUSHORT,
		NAREG|NDREG|NDSL,	RESC1,
		"	fintrz.x AL,A2\n	fmove.l A2,A1\n", },

/* double -> unsigned */
{ SCONV,	INAREG,
	SDREG,	TDOUBLE,
	SAREG,	TUNSIGNED,
		NAREG|NDREG|NDSL,	RESC1,
		"Z2	fcmp.d #0x41e0000000000000,A2\n"
		"	fjge 2f\n"
		"	fintrz.x A2,A2\n"
		"	fmove.l A2,A1\n"
		"	jra 3f\n"
		"2:	fsub.d #0x41e0000000000000,A2\n"
		"	fintrz.x A2,A2\n"
		"	fmove.l A2,A1\n"
		"	add.l #-2147483648,A1\n3:\n", },

/* (l)double -> float */
{ SCONV,	INDREG,
	SDREG,	TDOUBLE|TLDOUBLE,
	SDREG,	TFLOAT,
		NAREG,	RLEFT,
		"	fmove.s AL,A1\n	fmove.s A1,AL\n", },

/* ldouble -> double */
{ SCONV,	INDREG,
	SDREG,	TLDOUBLE,
	SDREG,	TDOUBLE,
		NTEMP*2,	RLEFT,
		"	fmove.d AL,A1\n	fmove.d A1,AL\n", },

/* assignment */
{ ASSIGN,	FOREFF,
	SCREG|SNAME|SOREG,	TLL,
	SCREG|SNAME|SOREG,	TLL,
		0,	0,
		"	move.l AR,AL\n"
		"	move.l UR,UL\n", },

{ ASSIGN,	INCREG,
	SCREG|SNAME|SOREG,	TLL,
	SCREG,			TLL,
		0,	RDEST,
		"	move.l AR,AL\n"
		"	move.l UR,UL\n", },

{ ASSIGN,	FOREFF,
	SAREG|SNAME|SOREG,	TAREG,
	SAREG|SNAME|SOREG,	TAREG,
		0,	0,
		"	move.ZA AR,AL\n", },

{ ASSIGN,	FOREFF,
	SBREG|SNAME|SOREG,	TPOINT,
	SBREG|SNAME|SOREG,	TPOINT,
		0,	0,
		"	move.l AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG|SNAME|SOREG,	TAREG,
	SAREG,			TAREG,
		0,	RDEST,
		"	move.ZA AR,AL\n", },

{ ASSIGN,	FOREFF|INBREG,
	SBREG|SNAME|SOREG,	TPOINT,
	SBREG,			TPOINT,
		0,	RDEST,
		"	move.l AR,AL\n", },

{ ASSIGN,	FOREFF|INDREG,
	SNAME|SOREG,	TFP,
	SDREG,		TFP,
		0,	RDEST,
		"	fmove.ZA AR,AL\n", },

{ ASSIGN,	FOREFF|INDREG,
	SDREG,		TFP,
	SNAME|SOREG,	TFP,
		0,	RDEST,
		"	fmove.ZA AR,AL\n", },

{ ASSIGN,	FOREFF|INDREG,
	SDREG,	TFP,
	SDREG,	TFP,
		0,	RDEST,
		"	fmove.x AR,AL\n", },

/* structure stuff */
{ STASG,	INBREG|FOREFF,
	SOREG|SNAME,	TANY,
	SBREG,		TPTRTO|TANY,
		NSPECIAL,	RDEST,
		"ZQ", },
/* 
 * Simple ops (add, sub, and, or, xor)
 */
/* Address registers may be added to (or subtracted from) */
{ PLUS, FOREFF,
	SBREG|SNAME|SOREG|SCON,		TWORD|TPOINT,
	SAREG,				TWORD,
		0,	0,
		"	add.l AR,AL\n", },

{ PLUS, FOREFF|INBREG,
	SBREG,			TPOINT,
	SAREG|SNAME|SOREG|SCON, TWORD,
		0,	RLEFT|RESCC,
		"	add.l AR,AL\n", },

{ PLUS, FOREFF|INDREG,
	SDREG,		TFP,
	SNAME|SOREG|SCON, TFP,
		0,	RLEFT|RESCC,
		"	fadd.ZA AR,AL\n", },

{ PLUS, FOREFF|INDREG,
	SDREG,	TFP,
	SDREG,	TFP,
		0,	RLEFT|RESCC,
		"	fadd.x AR,AL\n", },

{ PLUS, FOREFF|INCREG|RESCC,
	SCREG,	TLL,
	SCREG,	TLL,
		0,	RLEFT|RESCC,
		"	add.l UR,UL\n	addx.l AR,AL\n", },

{ MINUS, FOREFF,
	SBREG|SNAME|SOREG|SCON,		TWORD|TPOINT,
	SBREG,				TWORD,
		0,	0,
		"	sub.l AR,AL\n", },

{ MINUS, FOREFF|INAREG,
	SBREG,		TPOINT,
	SBREG,		TPOINT,
		NAREG,	RESC1,
		"	move.l AL,A1\n	sub.l AR,A1\n", },

{ MINUS, FOREFF|INBREG,
	SBREG,				TWORD|TPOINT,
	SABREG|SNAME|SOREG|SCON,	TWORD,
		0,	RLEFT|RESCC,
		"	sub.l AR,AL\n", },

{ MINUS, FOREFF|INDREG,
	SDREG,			TFP,
	SNAME|SOREG|SCON, TFP,
		0,	RLEFT|RESCC,
		"	fsub.ZA AR,AL\n", },

{ MINUS, FOREFF|INDREG,
	SDREG,	TFP,
	SDREG,	TFP,
		0,	RLEFT|RESCC,
		"	fsub.x AR,AL\n", },

{ MINUS, FOREFF|INCREG|RESCC,
	SCREG,	TLL,
	SCREG,	TLL,
		0,	RLEFT|RESCC,
		"	sub.l UR,UL\n	subx.l AR,AL\n", },

/* two pointers give a scalar */
{ MINUS,	INAREG|FORCC,
	SBREG|SNAME|SOREG|SCON, TPOINT,
	SBREG|SNAME|SOREG|SCON, TPOINT,
		NAREG,	RESC1|RESCC,
		"	move.l AL,A1\n	sub.l AR,A1\n", },

/* Hack to allow for opsimp later down */
/* Fortunately xor is not that common */
{ ER,	FOREFF|INAREG,
	SAREG,				TAREG,
	SNAME|SOREG|SCON,		TAREG,
		NAREG,	RLEFT|RESCC,
		"	move.ZA AR,A1\n	eor.ZA A1,AL\n", },

{ ER,	FOREFF|INCREG|FORCC,
	SCREG|SNAME|SOREG|SCON,		TLL,
	SCREG,				TLL,
		0,	RLEFT|RESCC,
		"	eor.l AR,AL\n	eor.l UR,UL\n", },

{ AND,	FOREFF|INCREG|FORCC,
	SCREG,				TLL,
	SCREG|SNAME|SOREG|SCON,		TLL,
		0,	RLEFT|RESCC,
		"	and.l AR,AL\n	and.l UR,UL\n", },

{ OR,	FOREFF|INCREG|FORCC,
	SCREG,				TLL,
	SCREG|SNAME|SOREG|SCON,		TLL,
		0,	RLEFT|RESCC,
		"	or.l AR,AL\n	or.l UR,UL\n", },

{ OPSIMP,	FOREFF|INAREG,
	SAREG,				TAREG,
	SAREG|SNAME|SOREG|SCON,		TAREG,
		0,	RLEFT|RESCC,
		"	Oz.ZA AR,AL\n", },

{ OPSIMP,	FOREFF,
	SAREG|SNAME|SOREG,	TAREG,
	SAREG,			TAREG,
		0,	RLEFT|RESCC,
		"	Oz.ZA AR,AL\n", },

/*
 * Negate a word.
 */

{ UMINUS,	FOREFF|INCREG|FORCC,
	SCREG,	TLL,
	SCREG,	TLL,
		0,	RLEFT|RESCC,
		"	neg.l UL\n	negx.l AL\n", },

{ UMINUS,	FOREFF|INAREG|FORCC,
	SAREG,	TAREG,
	SAREG,	TAREG,
		0,	RLEFT|RESCC,
		"	neg.ZA AL\n", },

{ UMINUS,	FOREFF|INDREG|FORCC,
	SDREG,	TFP,
	SDREG,	TFP,
		NDREG,	RESC1|RESCC,
		"	fmovecr #0xf,A1\n	fsub.x AL,A1\n", },

{ UMINUS,	FOREFF|INDREG|FORCC,
	SNAME|SOREG,	TFP,
	SDREG,		TFP,
		NDREG,	RESC1|RESCC,
		"	fmovecr #0xf,A1\n	fsub.ZA AL,A1\n", },

{ COMPL,	FOREFF|INAREG|FORCC,
	SAREG,	TAREG,
	SAREG,	TAREG,
		0,	RLEFT|RESCC,
		"	not.ZA AL\n", },

{ COMPL,	FOREFF|INCREG,
	SCREG,	TLL,
	SANY,	TANY,
		0,	RLEFT,
		"	not.l AL\n	not.l UL\n", },

/*
 * Shift operators.
 */
{ LS,	INAREG|FOREFF,
	SAREG,	TAREG,
	SAREG,	TAREG,
		0,	RLEFT,
		"	lsl.ZA AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG,	TUNSIGNED|TUSHORT|TUCHAR,
	SAREG,	TAREG,
		0,	RLEFT,
		"	lsr.ZA AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG,	TINT|TSHORT|TCHAR,
	SAREG,	TAREG,
		0,	RLEFT,
		"	asr.ZA AR,AL\n", },

/*
 * Leaf movements
 */
{ OPLTYPE,	INCREG,
	SANY,			TANY,
	SCREG|SCON|SOREG|SNAME,	TLL,
		NCREG|NCSL,	RESC1,
		"	move.l AL,A1\n	move.l UL,U1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SCON|SOREG|SNAME, TAREG,
		NAREG|NASL,	RESC1,
		"	move.ZA AL,A1\n", },

{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SBREG|SCON|SOREG|SNAME, TPOINT,
		NBREG|NBSL,	RESC1,
		"	move.l AL,A1\n", },

{ OPLTYPE,	INDREG,
	SANY,		TANY,
	SNAME|SOREG,	TFP,
		NDREG|NDSL,	RESC1,
		"	fmove.ZA AL,A1\n", },

{ OPLTYPE,	INDREG,
	SANY,	TANY,
	SDREG,	TFP,
		NDREG|NDSL,	RESC1,
		"	fmove.x AL,A1\n", },

/*
 * Indirection operators.
 */
{ UMUL, INCREG,
	SANY,	TPOINT,
	SOREG,	TLL,
		NCREG,	RESC1,
		"	move.l AL,A1\n	move.l UL,U1\n", },

{ UMUL, INAREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TPOINT|TWORD,
		NAREG|NASL,	RESC1,
		"	move.l AL,A1\n", },

{ UMUL, INBREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TPOINT|TWORD,
		NBREG|NBSL,	RESC1,
		"	move.l AL,A1\n", },

{ UMUL, INDREG,
	SANY,	TPOINT|TFP,
	SOREG,	TFP,
		NDREG|NDSL,	RESC1,
		"	fmove.ZA AL,A1\n", },

{ UMUL, INAREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TSHORT|TUSHORT,
		NAREG|NASL,	RESC1,
		"	move.w AL,A1\n", },

{ UMUL, INAREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"	move.b AL,A1\n", },


/*
 * DIV/MOD/MUL 
 */
{ DIV,	INAREG,
	SAREG,			TINT,
	SAREG|SNAME|SOREG,	TINT,
		0,	RLEFT,
		"	divs.l AR,AL\n", },

{ DIV,	INAREG,
	SAREG,			TUNSIGNED,
	SAREG|SNAME|SOREG,	TUNSIGNED,
		0,	RLEFT,
		"	divu.l AR,AL\n", },

{ DIV,	INDREG,
	SDREG,		TDOUBLE|TLDOUBLE,
	SNAME|SOREG,	TDOUBLE|TLDOUBLE,
		0,	RLEFT,
		"	fdiv.ZA AR,AL\n", },

{ DIV,	INDREG,
	SDREG,	TFP,
	SDREG,	TFP,
		0,	RLEFT,
		"	fdiv.x AR,AL\n", },

{ DIV,	INDREG,
	SDREG,		TFLOAT,
	SNAME|SOREG,	TFLOAT,
		0,	RLEFT,
		"	fsgldiv.ZA AR,AL\n", },

{ MOD,	INAREG,
	SAREG,			TINT,
	SAREG|SNAME|SOREG,	TINT,
		NAREG,	RESC1,
		"	divsl.l AR,A1:AL\n", },

{ MOD,	INAREG,
	SAREG,			TUNSIGNED,
	SAREG|SNAME|SOREG,	TUNSIGNED,
		NAREG,	RESC1,
		"	divul.l AR,A1:AL\n", },

{ MUL,	INAREG,
	SAREG,			TWORD,
	SAREG|SNAME|SOREG,	TWORD,
		0,	RLEFT,
		"	muls.l AR,AL\n", },

{ MUL,	INDREG,
	SDREG,		TDOUBLE|TLDOUBLE,
	SNAME|SOREG,	TDOUBLE|TLDOUBLE,
		0,	RLEFT,
		"	fmul.ZA AR,AL\n", },

{ MUL,	INDREG,
	SDREG,	TFP,
	SDREG,	TFP,
		0,	RLEFT,
		"	fmul.x AR,AL\n", },

{ MUL,	INDREG,
	SDREG,		TFLOAT,
	SNAME|SOREG,	TFLOAT,
		0,	RLEFT,
		"	fsglmul.s AR,AL\n", },

/*
 * Function call nodes.
 * Too many of them.
 */
/* FOREFF both direct and indirect */
{ UCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"	jsr CL\n", },

{ CALL,		FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"	jsr CL\nZB", },

{ UCALL,	FOREFF,
	SBREG,	TANY,
	SANY,	TANY,
		0,	0,
		"	jsr (AL)\n", },

{ CALL,		FOREFF,
	SBREG,	TANY,
	SANY,	TANY,
		0,	0,
		"	jsr (AL)\nZB", },

/* small scalar both direct and indirect */
{ UCALL,	INAREG,
	SCON,	TANY,
	SAREG,	TAREG,
		NAREG|NASL,	RESC1,
		"	jsr CL\n", },

{ CALL,		INAREG,
	SCON,	TANY,
	SAREG,	TAREG,
		NAREG|NASL,	RESC1,
		"	jsr CL\nZB", },

{ UCALL,	INAREG,
	SBREG,	TANY,
	SAREG,	TAREG,
		NAREG|NASL,	RESC1,
		"	jsr (AL)\n", },

{ CALL,		INAREG,
	SBREG,	TANY,
	SAREG,	TAREG,
		NAREG|NASL,	RESC1,
		"	jsr (AL)\nZB", },

/* long long both direct and indirect */
{ UCALL,	INCREG,
	SCON,	TANY,
	SCREG,	TLL,
		NCREG|NCSL,	RESC1,
		"	jsr CL\n", },

{ CALL,		INCREG,
	SCON,	TANY,
	SCREG,	TLL,
		NCREG|NCSL,	RESC1,
		"	jsr CL\nZB", },

{ UCALL,	INCREG,
	SBREG,	TANY,
	SCREG,	TLL,
		NCREG|NCSL,	RESC1,
		"	jsr (AL)\n", },

{ CALL,		INCREG,
	SBREG,	TANY,
	SCREG,	TLL,
		NCREG|NCSL,	RESC1,
		"	jsr (AL)\nZB", },

/* floats both direct and indirect */
{ UCALL,	INDREG,
	SCON,	TANY,
	SDREG,	TFP,
		NDREG|NDSL,	RESC1,
		"	jsr CL\n", },

{ CALL,		INDREG,
	SCON,	TANY,
	SDREG,	TFP,
		NDREG|NDSL,	RESC1,
		"	jsr CL\nZB", },

{ UCALL,	INDREG,
	SBREG,	TANY,
	SDREG,	TFP,
		NDREG|NDSL,	RESC1,
		"	jsr (AL)\n", },

{ CALL,		INDREG,
	SBREG,	TANY,
	SDREG,	TFP,
		NDREG|NDSL,	RESC1,
		"	jsr (AL)\nZB", },

/* pointers both direct and indirect */
{ UCALL,	INBREG,
	SCON,	TANY,
	SBREG,	TWORD|TPOINT,
		NBREG|NBSL,	RESC1,
		"	jsr CL\n", },

{ CALL,		INBREG,
	SCON,	TANY,
	SBREG,	TWORD|TPOINT,
		NBREG|NBSL,	RESC1,
		"	jsr CL\nZB", },

{ UCALL,	INBREG,
	SBREG,	TANY,
	SBREG,	TWORD|TPOINT,
		NBREG|NBSL,	RESC1,
		"	jsr (AL)\n", },

{ CALL,		INBREG,
	SBREG,	TANY,
	SBREG,	TWORD|TPOINT,
		NBREG|NBSL,	RESC1,
		"	jsr (AL)\nZB", },


/* struct return both direct and indirect */
{ USTCALL,	INBREG|FOREFF,
	SCON,	TANY,
	SBREG,	TWORD|TPOINT,
		NBREG|NBSL,	RESC1,
		"ZP	jsr CL\n", },

{ STCALL,	INBREG|FOREFF,
	SCON,	TANY,
	SBREG,	TWORD|TPOINT,
		NBREG|NBSL,	RESC1,
		"ZP	jsr CL\nZB", },

{ USTCALL,	INBREG|FOREFF,
	SBREG,	TANY,
	SBREG,	TWORD|TPOINT,
		NBREG|NBSL,	RESC1,
		"ZP	jsr (AL)\n", },

{ STCALL,		INBREG|FOREFF,
	SBREG,	TANY,
	SBREG,	TWORD|TPOINT,
		NBREG|NBSL,	RESC1,
		"ZP	jsr (AL)\nZB", },


/*
 * Arguments to functions.
 */
{ FUNARG,	FOREFF,
	SAREG|SOREG|SNAME|SCON, TINT|TUNSIGNED,
	SANY,			TANY,
		0,	RNULL,
		"	move.l AL,-(%sp)\n", },

{ FUNARG,	FOREFF,
	SCREG|SOREG|SNAME|SCON, TLL,
	SANY,			TANY,
		0,	RNULL,
		"	move.l UL,-(%sp)\n	move.l AL,-(%sp)\n", },

{ FUNARG,	FOREFF,
	SCON,	TPOINT,
	SANY,	TANY,
		0,	RNULL,
		"	pea CL\n", },

{ FUNARG,	FOREFF,
	SCON|SABREG|SNAME,	TWORD|TPOINT,
	SANY,			TWORD|TPOINT,
		0,	RNULL,
		"	move.l AL,-(%sp)\n", },

{ FUNARG,	FOREFF,
	SOREG,	TWORD|TPOINT,
	SANY,	TWORD|TPOINT,
		0,	RNULL,
		"	move.l AL,-(%sp)\n", },

{ FUNARG,	FOREFF,
	SDREG,	TFP,
	SANY,	TFP,
		0,	RNULL,
		"	fmove.ZA AL,-(%sp)\n", },

{ STARG,	FOREFF,
	SBREG,	TPTRTO|TSTRUCT,
	SANY,	TSTRUCT,
		NAREG|NBREG,	RNULL,
		"ZS", },

/*
 * Logical/branching operators
 */

/* Comparisions, take care of everything */
#if 0
{ OPLOG,	FORCC,
	SHLL|SOREG|SNAME,	TLL,
	SHLL,			TLL,
		0,	0,
		"ZD", },
#endif

{ OPLOG,	FORCC,
	SCREG,	TLL,
	SCREG,	TLL,
		0,	RESCC|RLEFT, /* trash left nodes */
		"	sub.l UR,UL\n	subx.l AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG,			TWORD,
	SCON|SAREG|SOREG|SNAME, TWORD,
		0,	RESCC,
		"	cmp.l AR,AL\n", },

{ OPLOG,	FORCC,
	SBREG,			TPOINT,
	SCON|SBREG|SOREG|SNAME, TPOINT,
		0,	RESCC,
		"	cmp.l AR,AL\n", },

/* jumps below emitted in zzzcode */
{ OPLOG,	FORCC,
	SDREG,			TFP,
	SCON|SOREG|SNAME,	TFP,
		0,	0,
		"	fcmp.ZL AR,AL\n	ZF", },

{ OPLOG,	FORCC,
	SDREG,	TFP,
	SDREG,	TFP,
		0,	0,
		"	fcmp.x AR,AL\n	ZF", },


/*
 * Jumps.
 */
{ GOTO,		FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp LL\n", },

# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

{ UMUL, DF( UMUL ), },

{ ASSIGN, DF(ASSIGN), },

{ STASG, DF(STASG), },

{ FLD, DF(FLD), },

{ OPLEAF, DF(NAME), },

/* { INIT, DF(INIT), }, */

{ OPUNARY, DF(UMINUS), },

{ OPANY, DF(BITYPE), },

{ FREE, FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" },
};

int tablesize = sizeof(table)/sizeof(table[0]);
