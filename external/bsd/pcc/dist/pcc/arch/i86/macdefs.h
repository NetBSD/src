/*	Id: macdefs.h,v 1.6 2015/11/24 17:35:11 ragge Exp 	*/	
/*	$NetBSD: macdefs.h,v 1.1.1.1 2016/02/09 20:28:39 plunky Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * Machine-dependent defines for both passes.
 */

/*
 * Convert (multi-)character constant to integer.
 */
#define makecc(val,i)	lastcon = (lastcon<<8)|((val<<24)>>24);

#define ARGINIT		64	/* # bits above fp where arguments start */
#define AUTOINIT	0	/* # bits below fp where automatics start */

/*
 * Storage space requirements
 */
#define SZCHAR		8
#define SZBOOL		8
#define SZINT		16
#define SZFLOAT		32
#define SZDOUBLE	64
#define SZLDOUBLE	96
#define SZLONG		32
#define SZSHORT		16
#define SZLONGLONG	64	/* Doesn't work usefully yet */
#define SZPOINT(t)	16	/* FIXME: 32 for large model work */

/*
 * Alignment constraints
 */
#define ALCHAR		8
#define ALBOOL		8
#define ALINT		16
#define ALFLOAT		32
#define ALDOUBLE	32
#define ALLDOUBLE	32
#define ALLONG		16
#define ALLONGLONG	16
#define ALSHORT		16
#define ALPOINT		16
#undef ALSTRUCT		/* Not defined if ELF ABI */
#define ALSTACK		16 
#define	ALMAX		128	/* not yet supported type */

/*
 * Min/max values.
 */
#define	MIN_CHAR	-128
#define	MAX_CHAR	127
#define	MAX_UCHAR	255
#define	MIN_SHORT	-32768
#define	MAX_SHORT	32767
#define	MAX_USHORT	65535
#define	MIN_INT		MIN_SHORT
#define	MAX_INT		MAX_SHORT
#define	MAX_UNSIGNED	MAX_USHORT
#define	MIN_LONG	(-0x7fffffff-1)
#define	MAX_LONG	0x7fffffff
#define	MAX_ULONG	0xffffffff
#define	MIN_LONGLONG	MIN_LONG
#define	MAX_LONGLONG	MAX_LONG
#define	MAX_ULONGLONG	MAX_ULONG

/* Default char is signed */
#undef	CHAR_UNSIGNED
#define	BOOL_TYPE	UCHAR	/* what used to store _Bool */
#undef UNALIGNED_ACCESS
/*
 * Use large-enough types.
 */
typedef	long long CONSZ;
typedef	unsigned long long U_CONSZ;
typedef long long OFFSZ;

#define CONFMT	"%lld"		/* format for printing constants */
#define LABFMT	"L%d"		/* format for printing labels */
#define	STABLBL	"LL%d"		/* format for stab (debugging) labels */
#ifdef LANG_F77
#define BLANKCOMMON "_BLNK_"
#define MSKIREG  (M(TYSHORT)|M(TYLONG))
#define TYIREG TYLONG
#define FSZLENG  FSZLONG
#define	AUTOREG	BP
#define	ARGREG	BP
#define ARGOFFSET 4
#endif

#define BACKAUTO 		/* stack grows negatively for automatics */
#define BACKTEMP 		/* stack grows negatively for temporaries */

#undef	FIELDOPS		/* no bit-field instructions */
#define TARGET_ENDIAN TARGET_LE

#define FINDMOPS	/* i86 has instructions that modifies memory */
#define	CC_DIV_0	/* division by zero is safe in the compiler */

/* Definitions mostly used in pass2 */

#define BYTEOFF(x)	((x)&03)
#define wdal(k)		(BYTEOFF(k)==0)

#define STOARG(p)
#define STOFARG(p)
#define STOSTARG(p)
#define genfcall(a,b)	gencall(a,b)

/* FIXME: float sizes wrong at this point ? */
#define	szty(t)	(((t) == DOUBLE || (t) == FLOAT || \
	(t) == LONG || t == ULONG || (t) == LONGLONG || (t) == ULONGLONG) ? 2 : (t) == LDOUBLE ? 3 : 1)

/*
 * The x86 has a bunch of register classes, most of them interfering
 * with each other.  All registers are given a sequential number to
 * identify it which must match rnames[] in local2.c.
 * Class membership and overlaps are defined in the macros RSTATUS
 * and ROVERLAP below.
 *
 * The classes used on x86 are:
 *	A - short and int regs
 *	B - char regs
 *	C - long and longlong regs
 *	D - floating point
 */
#define	AX	000	/* Scratch and return register */
#define	DX	001	/* Scratch and secondary return register */
#define	CX	002	/* Scratch (and shift count) register */
#define	BX	003	/* GDT pointer or callee-saved temporary register */
#define	SI	004	/* Callee-saved temporary register */
#define	DI	005	/* Callee-saved temporary register */
#define	BP	006	/* Frame pointer */
#define	SP	007	/* Stack pointer */

#define	AL	010
#define	AH	011
#define	DL	012
#define	DH	013
#define	CL	014
#define	CH	015
#define	BL	016
#define	BH	017

#define	AXDX	020
#define	AXCX	021
#define	AXBX	022
#define	AXSI	023
#define	AXDI	024
#define	DXCX	025
#define	DXBX	026
#define	DXSI	027
#define	DXDI	030
#define	CXBX	031
#define	CXSI	032
#define	CXDI	033
#define	BXSI	034
#define	BXDI	035
#define	SIDI	036

/* The 8 math registers in class D lacks names */

#define	MAXREGS	047	/* 39 registers */

#define	RSTATUS	\
	SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|PERMREG,	\
	SAREG|PERMREG, SAREG|PERMREG, 0, 0,	 			\
	SBREG, SBREG, SBREG, SBREG, SBREG, SBREG, SBREG, SBREG,		\
	SCREG, SCREG, SCREG, SCREG, SCREG, SCREG, SCREG, SCREG, 	\
	SCREG, SCREG, SCREG, SCREG, SCREG, SCREG, SCREG,		\
	SDREG, SDREG, SDREG, SDREG,  SDREG, SDREG, SDREG, SDREG,

#define	ROVERLAP \
	/* 8 basic registers */\
	{ AL, AH, AXDX, AXCX, AXBX, AXSI, AXDI, -1 },\
	{ DL, DH, AXDX, DXCX, DXBX, DXSI, DXDI, -1 },\
	{ CL, CH, AXCX, DXCX, CXBX, CXSI, CXDI, -1 },\
	{ BL, BH, AXBX, DXBX, CXBX, BXSI, BXDI, -1 },\
	{ AXSI, DXSI, CXSI, BXSI, SIDI, -1 },\
	{ AXDI, DXDI, CXDI, BXDI, SIDI, -1 },\
	{ -1 },\
	{ -1 },\
\
	/* 8 char registers */\
	{ AX, AXDX, AXCX, AXBX, AXSI, AXDI, -1 },\
	{ AX, AXDX, AXCX, AXBX, AXSI, AXDI, -1 },\
	{ DX, AXDX, DXCX, DXBX, DXSI, DXDI, -1 },\
	{ DX, AXDX, DXCX, DXBX, DXSI, DXDI, -1 },\
	{ CX, AXCX, DXCX, CXBX, CXSI, CXDI, -1 },\
	{ CX, AXCX, DXCX, CXBX, CXSI, CXDI, -1 },\
	{ BX, AXBX, DXBX, CXBX, BXSI, BXDI, -1 },\
	{ BX, AXBX, DXBX, CXBX, BXSI, BXDI, -1 },\
\
	/* 15 long emulating registers */\
	{ AX, AL, AH, DX, DL, DH, AXCX, AXBX, AXSI,	/* axdx */\
	  AXDI, DXCX, DXBX, DXSI, DXDI, -1, },\
	{ AX, AL, AH, CX, CL, CH, AXDX, AXBX, AXSI,	/* axcx */\
	  AXDI, DXCX, CXBX, CXSI, CXDI, -1 },\
	{ AX, AL, AH, BX, BL, BH, AXDX, AXCX, AXSI,	/* axbx */\
	  AXDI, DXBX, CXBX, BXSI, BXDI, -1 },\
	{ AX, AL, AH, SI, AXDX, AXCX, AXBX, AXDI,	/* axsi */\
	  DXSI, CXSI, BXSI, SIDI, -1 },\
	{ AX, AL, AH, DI, AXDX, AXCX, AXBX, AXSI,	/* axdi */\
	  DXDI, CXDI, BXDI, SIDI, -1 },\
	{ DX, DL, DH, CX, CL, CH, AXDX, AXCX, DXBX,	/* dxcx */\
	  DXSI, DXDI, CXBX, CXSI, CXDI, -1 },\
	{ DX, DL, DH, BX, BL, BH, AXDX, DXCX, DXSI,	/* dxbx */\
	  DXDI, AXBX, CXBX, BXSI, BXDI, -1 },\
	{ DX, DL, DH, SI, AXDX, DXCX, DXBX, DXDI,	/* dxsi */\
	  AXSI, CXSI, BXSI, SIDI, -1 },\
	{ DX, DL, DH, DI, AXDX, DXCX, DXBX, DXSI,	/* dxdi */\
	  AXDI, CXDI, BXDI, SIDI, -1 },\
	{ CX, CL, CH, BX, BL, BH, AXCX, DXCX, CXSI,	/* cxbx */\
	  CXDI, AXBX, DXBX, BXSI, BXDI, -1 },\
	{ CX, CL, CH, SI, AXCX, DXCX, CXBX, CXDI,	/* cxsi */\
	  AXSI, DXSI, BXSI, SIDI, -1 },\
	{ CX, CL, CH, DI, AXCX, DXCX, CXBX, CXSI,	/* cxdi */\
	  AXDI, DXDI, BXDI, SIDI, -1 },\
	{ BX, BL, BH, SI, AXBX, DXBX, CXBX, BXDI,	/* bxsi */\
	  AXSI, DXSI, CXSI, SIDI, -1 },\
	{ BX, BL, BH, DI, AXBX, DXBX, CXBX, BXSI,	/* bxdi */\
	  AXDI, DXDI, CXDI, SIDI, -1 },\
	{ SI, DI, AXSI, DXSI, CXSI, BXSI,		/* sidi */\
	  AXDI, DXDI, CXDI, BXDI, -1 },\
\
	/* The fp registers do not overlap with anything */\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },


/* Return a register class based on the type of the node */
/* FIXME: <= UCHAR includes long ?? check */
#define PCLASS(p) (p->n_type <= UCHAR ? SBREG : \
		  (p->n_type == LONG || p->n_type == ULONG || p->n_type == LONGLONG || p->n_type == ULONGLONG ? SCREG : \
		  (p->n_type >= FLOAT && p->n_type <= LDOUBLE ? SDREG : SAREG)))

#define	NUMCLASS 	4	/* highest number of reg classes used */

int COLORMAP(int c, int *r);
#define	GCLASS(x) (x < 8 ? CLASSA : x < 16 ? CLASSB : x < 31 ? CLASSC : CLASSD)
#define DECRA(x,y)	(((x) >> (y*6)) & 63)	/* decode encoded regs */
#define	ENCRD(x)	(x)		/* Encode dest reg in n_reg */
#define ENCRA1(x)	((x) << 6)	/* A1 */
#define ENCRA2(x)	((x) << 12)	/* A2 */
#define ENCRA(x,y)	((x) << (6+y*6))	/* encode regs in int */
/* XXX - return char in al? */
#define	RETREG(x)	(x == CHAR || x == UCHAR ? AL : \
			 x == LONG || x == ULONG || x == LONGLONG || x == ULONGLONG ? AXDX : \
			 x == FLOAT || x == DOUBLE || x == LDOUBLE ? 31 : AX)

#if 0
#define R2REGS	1	/* permit double indexing */
#endif

/* XXX - to die */
#define FPREG	BP	/* frame pointer */
#define STKREG	SP	/* stack pointer */

#define	SHSTR		(MAXSPECIAL+1)	/* short struct */
#define	SFUNCALL	(MAXSPECIAL+2)	/* struct assign after function call */
#define	SPCON		(MAXSPECIAL+3)	/* positive nonnamed constant */

/*
 * Specials that indicate the applicability of machine idioms.
 */
#define SMIXOR		(MAXSPECIAL+4)
#define SMILWXOR	(MAXSPECIAL+5)
#define SMIHWXOR	(MAXSPECIAL+6)
#define	STWO		(MAXSPECIAL+7)	/* exactly two */
#define	SMTWO		(MAXSPECIAL+8)	/* exactly minus two */

/*
 * i86-specific symbol table flags.
 */
#define	SSECTION	SLOCAL1
#define SSTDCALL	SLOCAL2	
#define SDLLINDIRECT	SLOCAL3

/*
 * i86-specific interpass stuff.
 */

#define TARGET_IPP_MEMBERS			\
	int ipp_argstacksize;

#define	HAVE_WEAKREF
#define	TARGET_FLT_EVAL_METHOD	2	/* all as long double */

/*
 * Extended assembler macros.
 */
void targarg(char *w, void *arg);
#define	XASM_TARGARG(w, ary)	\
	(w[1] == 'b' || w[1] == 'h' || w[1] == 'w' || w[1] == 'k' ? \
	w++, targarg(w, ary), 1 : 0)
int numconv(void *ip, void *p, void *q);
#define	XASM_NUMCONV(ip, p, q)	numconv(ip, p, q)
int xasmconstregs(char *);
#define	XASMCONSTREGS(x) xasmconstregs(x)
#define	MYSETXARG if (XASMVAL(cw) == 'q') {	\
	c = 'r'; addalledges(&ablock[SI]); addalledges(&ablock[DI]); }

/* target specific attributes */
#define	ATTR_MI_TARGET	ATTR_I86_FPPOP
#define NATIVE_FLOATING_POINT
