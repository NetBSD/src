/*	Id: table.c,v 1.26 2012/09/25 19:17:50 ragge Exp 	*/	
/*	$NetBSD: table.c,v 1.1.1.3.8.1 2014/08/19 23:52:09 tls Exp $	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

# include "pass2.h"

# define WPTR TPTRTO|TINT|TFLOAT|TDOUBLE|TPOINT|TUNSIGNED
# define SAWM SNAME|SOREG|STARNM|STARREG
# define AWD SAWM|SCON
/* tbl */
# define TANYSIGNED TINT|TSHORT|TCHAR
# define TANYUSIGNED TPOINT|TUNSIGNED|TUSHORT|TUCHAR
# define TANYFIXED TANYSIGNED|TANYUSIGNED
# define TWORD TINT|TUNSIGNED|TPOINT
/* tbl */
# define TLL TLONGLONG|TULONGLONG
# define TBREG TLONGLONG|TULONGLONG|TDOUBLE
# define TAREG TANYFIXED|TFLOAT

struct optab  table[] = {
/* First entry must be an empty entry */
{ -1, FOREFF, SANY, TANY, SANY, TANY, 0, 0, "", },

/* PCONVs are usually not necessary */
{ PCONV,	INAREG,
	SAREG,	TWORD,
	SAREG,	TWORD,
		0,	RLEFT,
		"", },

{ PCONV,	INAREG|INAREG,
	SAREG|AWD,	TCHAR|TSHORT,
	SANY,	TPOINT,
		NAREG|NASL,	RESC1,
		"	cvtZLl	AL,A1\n", },

{ PCONV,	INAREG|INAREG,
	SAREG|AWD,	TUCHAR|TUSHORT,
	SANY,	TPOINT,
		NAREG|NASL,	RESC1,
		"	movzZLl	AL,A1\n", },

/* Handle conversions in C code */
{ SCONV,	INAREG,
	SAREG|AWD,	TAREG,
	SANY,		TANY,
		NAREG|NASL,	RESC1|RESCC,
		"ZG", },

{ SCONV,	INAREG,
	SBREG|AWD,	TBREG,
	SANY,		TANY,
		NAREG|NASL,	RESC1|RESCC,
		"ZG", },

{ SCONV,	INBREG,
	SBREG|AWD,	TBREG,
	SANY,		TANY,
		NBREG|NBSL,	RESC1|RESCC,
		"ZG", },

{ SCONV,	INBREG,
	SAREG|AWD,	TAREG,
	SANY,		TANY,
		NBREG|NBSL,	RESC1|RESCC,
		"ZG", },

{ GOTO,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	ZJ\n", },

{ GOTO,	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp	(AL)\n", },

{ STARG,	FOREFF,
	SCON|SAREG,	TANY,
	SANY,	TANY,
		NSPECIAL,	RNOP,
		"ZS", },

{ ADDROF,	INAREG,
	SNAME,	TANY,
	SAREG,	TANY,
		NAREG,	RESC1,
		"	movab	AL,A1\n", },

{ STASG,	FOREFF,
	SNAME|SOREG,	TANY,
	SCON|SAREG,	TANY,
		NSPECIAL,	RNOP,
		"ZS", },

{ STASG,	INAREG,
	SNAME|SOREG,	TANY,
	SCON,	TANY,
		NSPECIAL|NAREG,	RDEST,
		"ZS	movl	AR,A1\n", },

{ STASG,	INAREG,
	SNAME|SOREG,	TANY,
	SAREG,	TANY,
		NSPECIAL,	RDEST,
		"	pushl	AR\nZS	movl	(%sp)+,AR\n", },

{ FLD,	INAREG|INAREG,
	SANY,	TANY,
	SFLD,	TANYSIGNED,
		NAREG|NASR,	RESC1,
		"	extv	H,S,AR,A1\n", },

{ FLD,	INAREG|INAREG,
	SANY,	TANY,
	SFLD,	TANYUSIGNED,
		NAREG|NASR,	RESC1,
		"	extzv	H,S,AR,A1\n", },

#if 0
{ FLD,	FORARG,
	SANY,	TANY,
	SFLD,	ANYSIGNED,
		0,	RNULL,
		"	extv	H,S,AR,-(%sp)\n", },

{ FLD,	FORARG,
	SANY,	TANY,
	SFLD,	ANYUSIGNED,
		0,	RNULL,
		"	extzv	H,S,AR,-(%sp)\n", },
#endif

{ OPLOG,	FORCC,
	SBREG|AWD,	TLONGLONG|TULONGLONG,
	SBREG|AWD,	TLONGLONG|TULONGLONG,
		0,	0,
		"ZB", },

{ OPLOG,	FORCC,
	SAREG|AWD,	TWORD,
	SAREG|AWD,	TWORD,
		0,	RESCC,
		"	cmpl	AL,AR\n", },

{ OPLOG,	FORCC,
	SAREG|AWD,	TSHORT|TUSHORT,
	SAREG|AWD,	TSHORT|TUSHORT,
		0,	RESCC,
		"	cmpw	AL,AR\n", },

{ OPLOG,	FORCC,
	SAREG|AWD,	TCHAR|TUCHAR,
	SAREG|AWD,	TCHAR|TUCHAR,
		0,	RESCC,
		"	cmpb	AL,AR\n", },

{ OPLOG,	FORCC,
	SAREG|AWD,	TSHORT|TUSHORT,
	SSCON,	TANY,
		0,	RESCC,
		"	cmpw	AL,AR\n", },

{ OPLOG,	FORCC,
	SAREG|AWD,	TCHAR|TUCHAR,
	SCCON,	TANY,
		0,	RESCC,
		"	cmpb	AL,AR\n", },

{ OPLOG,	FORCC,
	SBREG|AWD,	TDOUBLE,
	SBREG|AWD,	TDOUBLE,
		0,	RESCC,
		"	cmpd	AL,AR\n", },

{ OPLOG,	FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TFLOAT,
		0,	RESCC,
		"	cmpf	AL,AR\n", },

{ CCODES,	INAREG|INAREG,
	SANY,	TANY,
	SANY,	TANY,
		NAREG,	RESC1,
		"	movl	$1,A1\nZN", },

/*
 * Subroutine calls.
 */

{ CALL,		FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"	calls	ZC,CL\n", },

{ UCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"	calls	$0,CL\n", },

{ CALL,		INAREG,
	SCON,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	RESC1, /* should be register 0 */
		"	calls	ZC,CL # 1\n", },

{ UCALL,	INAREG,
	SCON,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	RESC1, /* should be register 0 */
		"	calls	$0,CL\n", },

{ CALL,		INBREG,
	SCON,	TANY,
	SANY,	TBREG,
		NBREG|NBSL,	RESC1, /* should be register 0 */
		"	calls	ZC,CL # 2\n", },

{ UCALL,	INBREG,
	SCON,	TANY,
	SANY,	TBREG,
		NBREG|NASL,	RESC1, /* should be register 0 */
		"	calls	$0,CL\n", },

{ CALL,		INBREG,
	SAREG,	TANY,
	SANY,	TBREG,
		NBREG|NBSL,	RESC1,	/* should be 0 */
		"	calls	ZC,(AL)\n", },

{ UCALL,	INBREG,
	SAREG,	TANY,
	SANY,	TBREG,
		NBREG|NBSL,	RESC1,	/* should be 0 */
		"	calls	ZC,(AL)\n", },

{ CALL,		FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,	/* should be 0 */
		"	calls	ZC,(AL)\n", },

{ CALL,		INAREG,
	SAREG,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	calls	ZC,(AL)\n", },

{ UCALL,	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,	/* should be 0 */
		"	calls	ZC,(AL)\n", },

{ UCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	calls	ZC,(AL)\n", },

#if 0
{ UCALL,	FOREFF,
	SNAME,	TANY,
	SANY,	TANY,
		0,	0,	/* really reg 0 */
		"	calls	ZC,*AL\n", },

{ UCALL,	INAREG,
	SNAME,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	RESC1,	/* really reg 0 */
		"	calls	ZC,*AL\n", },

{ UCALL,	FOREFF,
	SSOREG,	TANY,
	SANY,	TANY,
		0,	0,	/* really reg 0 */
		"	calls	ZC,*AL\n", },

{ UCALL,	INAREG,
	SSOREG,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	RESC1,	/* really reg 0 */
		"	calls	ZC,*AL\n", },
#endif

{ STCALL,	INAREG,
	SCON,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	RESC1, /* should be register 0 */
		"	calls	ZC,CL\n", },

{ STCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	0, /* should be register 0 */
		"	calls	ZC,CL\n", },

{ STCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	calls	ZC,(AL)\n", },

{ STCALL,	FOREFF,
	SAREG,	TANY,
	SANY,	TAREG,
		NAREG|NASL,	0,	/* should be 0 */
		"	calls	ZC,(AL)\n", },

/*
 * Function arguments
 */
{ FUNARG,	FOREFF,
	SCON|SAREG|SNAME|SOREG,	TANY,
	SANY,	TWORD|TPOINT|TFLOAT,
		0,	RNULL,
		"	pushl	AL\n" },

{ FUNARG,	FOREFF,
	SCON|SBREG|SNAME|SOREG,	TLL|TDOUBLE,
	SANY,	TANY,
		0,	RNULL,
		"	movq	AL,-(%sp)\n" },

/* RS for signed <= int converted to negative LS */
#if 0
/* RS ulonglong converted to function call */
/* RS longlong converted to negative LS */
{ RS,	INBREG|FORCC,
	SBREG|AWD,		TLONGLONG,
	SAREG|SBREG|AWD,	TANY,
		NBREG|NBSL|NBSR,	RESC1|RESCC,
		"	ashq	AR,AL,A1\n", },
#endif

{ RS,	INAREG|FORCC,
	SAREG,		TUCHAR,
	SAREG|SAWM,	TANYFIXED,
		NAREG,	RLEFT|RESCC,
		"	subl3	AR,$8,A1\n	extzv	AR,A1,AL,AL\n", },

{ RS,	INAREG|FORCC,
	SAREG,		TUSHORT,
	SAREG|SAWM,	TANYFIXED,
		NAREG,	RLEFT|RESCC,
		"	subl3	AR,$16,A1\n	extzv	AR,A1,AL,AL\n", },

{ RS,	INAREG|FORCC,
	SAREG,	TUNSIGNED,
	SAREG|SAWM,	TANYFIXED,
		NAREG,	RLEFT|RESCC,
		"	subl3	AR,$32,A1\n	extzv	AR,A1,AL,AL\n", },

{ RS,	INAREG|FORCC,
	SAREG,	TUNSIGNED|TUSHORT|TUCHAR,
	SCON,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	extzv	AR,ZU,AL,A1\n", },

/* extv only for short and char, rest uses ashl/q */
{ RS,	INAREG|FORCC,
	SAREG,	TSHORT|TCHAR,
	SCON,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	extv	AR,ZU,AL,A1\n", },

{ LS,	INBREG|FORCC,
	SBREG|AWD,	TLL,
	SAREG|SBREG|AWD,	TANY,
		NBREG|NBSL|NBSR,	RESC1|RESCC,
		"	ashq	AR,AL,A1\n", },

{ LS,	INAREG|INAREG|FORCC,
	SAREG|AWD,	TANYFIXED,
	SAREG|AWD,	TANYFIXED,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	ashl	AR,AL,A1\n", },

#if 0
{ INCR,	FOREFF,
	SAREG|AWD,	TANY,
	SANY,	TANY,
		0,	RLEFT,
		"	ZE\n", },

{ DECR,	FOREFF,
	SAREG|AWD,	TANY,
	SCON,	TANY,
		0,	RLEFT,
		"	ZE\n", },

{ INCR,	INAREG|INAREG,
	SAREG|AWD,	TANY,
	SCON,	TANY,
		NAREG,	RESC1,
		"	ZD\n", },

{ DECR,	INAREG|INAREG,
	SAREG|AWD,	TANY,
	SCON,	TANY,
		NAREG,	RESC1,
		"	ZD\n", },
#endif

/* Assign to 64-bit register, three entries */
/* Have FOREFF first to catch mem-mem moves */
{ ASSIGN,	FOREFF,
	SBREG|AWD,	TBREG,
	SCON,		TBREG,
		0,	0,
		"ZA", },

{ ASSIGN,	FOREFF,
	SBREG|AWD,	TBREG,
	SBREG|AWD,	TBREG,
		0,	0,
		"	movq	AR,AL\n", },

{ ASSIGN,	INBREG,
	SBREG,	TBREG,
	SBREG|AWD,	TBREG,
		0,	RDEST,
		"	movq	AR,AL\n", },

{ ASSIGN,	INBREG,
	SBREG|AWD,	TBREG,
	SBREG,	TBREG,
		0,	RDEST,
		"	movq	AR,AL\n", },

/* Assign to 32-bit register, three entries */
{ ASSIGN,	FOREFF|FORCC,
	SAREG|AWD,	TAREG,
	SCON,		TAREG,
		0,	RESCC,
		"ZA", },

{ ASSIGN,	FOREFF|FORCC,
	SAREG|AWD,	TAREG,
	SAREG|AWD,	TAREG,
		0,	RESCC,
		"	movZL	AR,AL\n", },

{ ASSIGN,	INAREG|FORCC,
	SAREG,		TAREG,
	SAREG|AWD,	TAREG,
		0,	RDEST|RESCC,
		"	movZL	AR,AL\n", },

{ ASSIGN,	INAREG|FORCC,
	SAREG|AWD,	TAREG,
	SAREG,		TAREG,
		0,	RDEST|RESCC,
		"	movZL	AR,AL\n", },

/* Bitfields, not yet */
{ ASSIGN,	INAREG|FOREFF|FORCC,
	SFLD,	TANY,
	SAREG|AWD,	TWORD,
		0,	RDEST|RESCC,
		"	insv	AR,H,S,AL\n", },

{ ASSIGN,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TWORD,
	SFLD,	TANYSIGNED,
		0,	RDEST|RESCC,
		"	extv	H,S,AR,AL\n", },

{ ASSIGN,	INAREG|FOREFF|FORCC,
	SAREG|AWD,	TWORD,
	SFLD,	TANYUSIGNED,
		0,	RDEST|RESCC,
		"	extzv	H,S,AR,AL\n", },

/* dummy UNARY MUL entry to get U* to possibly match OPLTYPE */
{ UMUL,	FOREFF,
	SCC,	TANY,
	SCC,	TANY,
		0,	RNULL,
		"	HELP HELP HELP\n", },

{ UMUL, INBREG,
	SANY,	TPOINT,
	SOREG,	TBREG,
		NBREG|NBSL,	RESC1,
		"	movq AL,A1\n", },

{ UMUL, INAREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TPOINT|TWORD,
		NAREG|NASL,	RESC1,
		"	movl AL,A1\n", },

{ UMUL, INAREG,
	SANY,	TPOINT|TSHORT|TUSHORT,
	SOREG,	TPOINT|TSHORT|TUSHORT,
		NAREG|NASL,	RESC1,
		"	movw AL,A1\n", },

{ UMUL, INAREG,
	SANY,	TPOINT|TCHAR|TUCHAR,
	SOREG,	TPOINT|TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"	movb AL,A1\n", },

#if 0
{ REG,	FORARG,
	SANY,	TANY,
	SAREG,	TDOUBLE|TFLOAT,
		0,	RNULL,
		"	movZR	AR,-(%sp)\n", },

{ REG,	INTEMP,
	SANY,	TANY,
	SAREG,	TDOUBLE,
		2*NTEMP,	RESC1,
		"	movd	AR,A1\n", },

{ REG,	INTEMP,
	SANY,	TANY,
	SAREG,	TANY,
		NTEMP,	RESC1,
		"	movZF	AR,A1\n", },
#endif

{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SCON|SOREG|SNAME,	TLONGLONG|TULONGLONG,
		NBREG,	RESC1,
		"	movq AL,A1\n", },

{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SANY,	TBREG,
		NBREG|NBSR,	RESC1,
		"	movZR AR,A1\n", },

{ OPLTYPE,	INAREG|INAREG,
	SANY,	TANY,
	SANY,	TAREG,
		NAREG|NASR,	RESC1,
		"	movZR AR,A1\n", },

{ OPLTYPE,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		0,	RESCC,
		"	tstZR	AR\n", },

#if 0
{ OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TWORD,
		0,	RNULL,
		"	pushl	AR\n", },

{ OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TCHAR|TSHORT,
		0,	RNULL,
		"	cvtZRl	AR,-(%sp)\n", },

{ OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TUCHAR|TUSHORT,
		0,	RNULL,
		"	movzZRl	AR,-(%sp)\n", },

{ OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TDOUBLE,
		0,	RNULL,
		"	movd	AR,-(%sp)\n", },

{ OPLTYPE,	FORARG,
	SANY,	TANY,
	SANY,	TFLOAT,
		0,	RNULL,
		"	cvtfd	AR,-(%sp)\n", },
#endif

{ UMINUS,	INBREG,
	SBREG|AWD,	TLL,
	SANY,	TLL,
		NBREG|NBSL,	RESC1|RESCC,
		"	mnegl	UL,U1\n	mnegl	AL,A1\n	sbwc	$0,U1\n", },

{ UMINUS,	INAREG|FORCC,
	SAREG|AWD,	TAREG|TDOUBLE,
	SANY,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	mnegZL	AL,A1\n", },

{ UMINUS,	INBREG|FORCC,
	SBREG|AWD,	TDOUBLE,
	SANY,	TANY,
		NBREG|NASL,	RESC1|RESCC,
		"	mnegZL	AL,A1\n", },

{ COMPL,	INBREG,
	SBREG|AWD,	TLL,
	SANY,		TLL,
		NBREG|NBSL,	RESC1|RESCC,
		"	mcoml	AL,A1\n	mcoml	UL,U1\n", },

{ COMPL,	INAREG|FORCC,
	SAREG|AWD,	TINT|TUNSIGNED,
	SANY,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	mcomZL	AL,A1\n", },

{ COMPL,	INAREG|FORCC,
	SAREG|AWD,	TANYSIGNED|TANYUSIGNED,
	SANY,	TANY,
		NAREG|NASL,	RESC1|RESCC,
		"	cvtZLl	AL,A1\n	mcoml	A1,A1\n", },

{ AND,	FORCC,
	SAREG|AWD,	TWORD,
	SCON,	TWORD,
		0,	RESCC,
		"	bitl	ZZ,AL\n", },

{ AND,	FORCC,
	SAREG|AWD,	TSHORT|TUSHORT,
	SSCON,	TWORD,
		0,	RESCC,
		"	bitw	ZZ,AL\n", },

{ AND,	FORCC,
	SAREG|AWD,	TCHAR|TUCHAR,
	SCCON,	TWORD,
		0,	RESCC,
		"	bitb	ZZ,AL\n", },

{ MUL,	INAREG|FORCC,
	SAREG|AWD,		TANYFIXED,
	SAREG|AWD,		TANYFIXED,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	mulZL3	AR,AL,A1\n", },

{ OPMUL,	INAREG|INAREG|FORCC,
	SAREG,	TINT|TUNSIGNED|TLONG|TULONG,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
		0,	RLEFT|RESCC,
		"	OL2	AR,AL\n", },

{ OPMUL,	INAREG|INAREG|FORCC,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
	SAREG|AWD,	TINT|TUNSIGNED|TLONG|TULONG,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OL3	AR,AL,A1\n", },

{ MOD,	INAREG|INAREG,
	SAREG|AWD,	TINT,
	SAREG|AWD,	TINT,
		NAREG,	RESC1,
		"	divl3	AR,AL,A1\n	mull2	AR,A1\n	subl3	A1,AL,A1\n", },

{ PLUS,	INBREG|FORCC,
	SBREG,		TLL,
	SBREG|AWD,	TLL,
		0,	RLEFT,
		"	addl2	AR,AL\n"
		"	adwc	UR,UL\n", },

{ PLUS,		INAREG|FORCC,
	SAREG,	TANYFIXED,
	SONE,	TANY,
		0,	RLEFT|RESCC,
		"	incZL	AL\n", },

{ MINUS,	INAREG|FORCC,
	SAREG,	TANYFIXED,
	SONE,	TANY,
		0,	RLEFT|RESCC,
		"	decZL	AL\n", },

{ MINUS,	INBREG|FORCC,
	SBREG,		TLL,
	SBREG|AWD,	TLL,
		0,	RLEFT,
		"	subl2	AR,AL\n"
		"	sbwc	UR,UL\n", },
{ DIV,	INBREG,
	SBREG|AWD, 	TLL,
	SBREG|AWD, 	TLL,
		NSPECIAL|NBREG|NBSL|NBSR,	RESC1,
		"ZO", },

{ MOD,	INBREG,
	SBREG|AWD, 	TLL,
	SBREG|AWD, 	TLL,
		NSPECIAL|NBREG|NBSL|NBSR,	RESC1,
		"ZO", },

{ MUL,	INBREG,
	SBREG|AWD, 	TLL,
	SBREG|AWD, 	TLL,
		NSPECIAL|NBREG|NBSL|NBSR,	RESC1,
		"ZO", },

{ OR,	INBREG,
	SBREG,		TLL,
	SBREG|AWD,	TLL,
		0,	RLEFT,
		"	bisl2	AR,AL\n	bisl2	UR,UL\n", },

{ OR,	INBREG,
	SBREG|AWD,	TLL,
	SBREG|AWD,	TLL,
		NBREG,	RESC1,
		"	bisl3	AR,AL,A1\n	bisl3	UR,UL,U1\n", },

{ ER,	INBREG,
	SBREG,		TLL,
	SBREG|AWD,	TLL,
		0,	RLEFT,
		"	xorl2	AR,AL\n	xorl2	UR,UL\n", },

{ ER,	INBREG,
	SBREG|AWD,	TLL,
	SBREG|AWD,	TLL,
		NBREG,	RESC1,
		"	xorl3	AR,AL,A1\n	xorl3	UR,UL,U1\n", },

{ AND,	INBREG,
	SBREG,		TLL,
	SBREG|AWD,	TLL,
		0,	RLEFT,
		"	bicl2	AR,AL\n	bicl2	UR,UL\n", },

{ AND,	INBREG,
	SBREG|AWD,	TLL,
	SBREG|AWD,	TLL,
		NBREG,	RESC1,
		"	bicl3	AR,AL,A1\n	bicl3	UR,UL,U1\n", },

{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,		TWORD,
	SAREG|AWD,	TWORD,
		0,	RLEFT|RESCC,
		"	OL2	AR,AL\n", },

{ OPSIMP,	INAREG|FORCC,
	SAREG|AWD,	TWORD,
	SAREG|AWD,	TWORD,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OL3	AR,AL,A1\n", },

{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,		TSHORT|TUSHORT,
	SAREG|AWD,	TSHORT|TUSHORT,
		0,	RLEFT|RESCC,
		"	OW2	AR,AL\n", },

{ OPSIMP,	INAREG|FORCC,
	SAREG|AWD,	TSHORT|TUSHORT,
	SAREG|AWD,	TSHORT|TUSHORT,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OW3	AR,AL,A1\n", },

{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,		TCHAR|TUCHAR,
	SAREG|AWD,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"	OB2	AR,AL\n", },

{ OPSIMP,	INAREG|FORCC,
	SAREG|AWD,	TCHAR|TUCHAR,
	SAREG|AWD,	TCHAR|TUCHAR,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OB3	AR,AL,A1\n", },

{ OPFLOAT,	INAREG|FORCC,
	SAREG,		TFLOAT,
	SAREG|AWD,	TFLOAT,
		0,	RLEFT|RESCC,
		"	OF2	AR,AL\n", },

{ OPFLOAT,	INAREG|FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TFLOAT,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OF3	AR,AL,A1\n", },

{ OPFLOAT,	INBREG|FORCC,
	SBREG,		TDOUBLE,
	SBREG|AWD,	TDOUBLE,
		0,	RLEFT|RESCC,
		"	OD2	AR,AL\n", },

{ OPFLOAT,	INBREG|FORCC,
	SBREG|AWD,	TDOUBLE,
	SBREG|AWD,	TDOUBLE,
		NBREG|NBSL|NBSR,	RESC1|RESCC,
		"	OD3	AR,AL,A1\n", },

#if 0 /* XXX probably wrong */
{ OPFLOAT,	INAREG|INAREG|FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TDOUBLE,
		NAREG|NASL,	RESC1|RESCC,
		"	cvtfd	AL,A1\n	OD2	AR,A1\n", },

{ OPFLOAT,	INAREG|INAREG|FORCC,
	SAREG|AWD,	TDOUBLE,
	SAREG|AWD,	TFLOAT,
		NAREG|NASR,	RESC1|RESCC,
		"	cvtfd	AR,A1\n	OD3	A1,AL,A1\n", },

{ OPFLOAT,	INAREG|INAREG|FORCC,
	SAREG|AWD,	TFLOAT,
	SAREG|AWD,	TFLOAT,
		NAREG|NASL|NASR,	RESC1|RESCC,
		"	OF3	AR,AL,A1\n	cvtfd	A1,A1\n", },
#endif

	/* Default actions for hard trees ... */

# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

{ UMUL, DF( UMUL ), },

{ ASSIGN, DF(ASSIGN), },

{ STASG, DF(STASG), },

{ OPLEAF, DF(NAME), },

{ OPLOG,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		REWRITE,	BITYPE,
		"", },

{ OPUNARY, DF(UMINUS), },

{ OPANY, DF(BITYPE), },

{ FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" }
};
