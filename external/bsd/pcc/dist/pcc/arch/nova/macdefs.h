/*	Id: macdefs.h,v 1.8 2014/06/03 20:19:50 ragge Exp 	*/	
/*	$NetBSD: macdefs.h,v 1.1.1.3.20.1 2014/08/10 07:10:06 tls Exp $	*/
/*
 * Copyright (c) 2006 Anders Magnusson (ragge@ludd.luth.se).
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

/*
 * Machine-dependent defines for Data General Nova.
 */

/*
 * Convert (multi-)character constant to integer.
 */
#define makecc(val,i)	lastcon = (lastcon<<8)|(val);

/*
 * Storage space requirements
 */
#define SZCHAR		8
#define SZBOOL		8
#define SZINT		16
#define SZFLOAT		32
#define SZDOUBLE	64
#define SZLDOUBLE	64
#define SZLONG		32
#define SZSHORT		16
#define SZLONGLONG	64
#define SZPOINT(t)	16	/* Actually 15 */

/*
 * Alignment constraints
 */
#define ALCHAR		8
#define ALBOOL		8
#define ALINT		16
#define ALFLOAT		16
#define ALDOUBLE	16
#define ALLDOUBLE	16
#define ALLONG		16
#define ALLONGLONG	16
#define ALSHORT		16
#define ALPOINT		16
#define ALSTRUCT	16
#define ALSTACK		16 

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
#define	MIN_LONG	0x80000000L
#define	MAX_LONG	0x7fffffffL
#define	MAX_ULONG	0xffffffffUL
#define	MIN_LONGLONG	0x8000000000000000LL
#define	MAX_LONGLONG	0x7fffffffffffffffLL
#define	MAX_ULONGLONG	0xffffffffffffffffULL

/* Default char is unsigned */
#define	CHAR_UNSIGNED
#define WORD_ADDRESSED
#define	BOOL_TYPE	UCHAR
#define	MYALIGN		/* provide private alignment function */

/*
 * Use large-enough types.
 */
typedef	long CONSZ;
typedef	unsigned long U_CONSZ;
typedef long OFFSZ;

#define CONFMT	"0%lo"		/* format for printing constants */
#define LABFMT	"L%d"		/* format for printing labels */
#define	STABLBL	"LL%d"		/* format for stab (debugging) labels */

#define BACKAUTO 		/* stack grows negatively for automatics */
#define BACKTEMP 		/* stack grows negatively for temporaries */
#define ARGINIT		0	/* first arg at 0 offset */
#define AUTOINIT	32	/* first var below 32-bit offset */


#undef	FIELDOPS	/* no bit-field instructions */
#define TARGET_ENDIAN	TARGET_BE

/* Definitions mostly used in pass2 */

#define BYTEOFF(x)	((x)&01)
#define wdal(k)		(BYTEOFF(k)==0)

#define	szty(t)	((t) == DOUBLE || (t) == LDOUBLE || \
	(t) == LONGLONG || (t) == ULONGLONG ? 4 : \
	((t) == LONG || (t) == ULONG || (t) == FLOAT) ? 2 : 1)

/*
 * The Nova has two register classes.  Note that the space used in 
 * zero page is considered stack.
 * Register 6 and 7 are FP and SP (in zero page).
 *
 * The classes used on Nova are:
 *	A - AC0-AC3 (as non-index registers)	: reg 0-3
 *	B - AC2-AC3 (as index registers)	: reg 4-5
 * FP/SP as 6/7.
 */
#define	MAXREGS	8	/* 0-29 */

#define	RSTATUS	\
	SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG,	\
	SBREG|TEMPREG, SBREG|TEMPREG, 0, 0

#define	ROVERLAP \
	{ -1 }, { -1 }, { 4, -1 }, { 5, -1 }, { 2, -1 }, { 3, -1 },	\
	{ -1 }, { -1 }

/* Return a register class based on the type of the node */
/* Used in tshape, avoid matching fp/sp as reg */
#define PCLASS(p) (p->n_op == REG && regno(p) > 5 ? 0 :	\
	ISPTR(p->n_type) ? SBREG : SAREG)

#define	NUMCLASS 	2	/* highest number of reg classes used */

int COLORMAP(int c, int *r);
#define	GCLASS(x) (x < 4 ? CLASSA : CLASSB)
#define DECRA(x,y)	(((x) >> (y*6)) & 63)	/* decode encoded regs */
#define	ENCRD(x)	(x)		/* Encode dest reg in n_reg */
#define ENCRA1(x)	((x) << 6)	/* A1 */
#define ENCRA2(x)	((x) << 12)	/* A2 */
#define ENCRA(x,y)	((x) << (6+y*6))	/* encode regs in int */
#define	RETREG(x)	(0) /* ? Sanity */

#define FPREG	6	/* frame pointer */
#define STKREG	7	/* stack pointer */

#define	MAXZP	030	/* number of locations used as stack */
#define	ZPOFF	050	/* offset of zero page regs */

#define	MYSTOREMOD
#define	MYLONGTEMP(p,w) {					\
	if (w->r_class == 0) {					\
		w->r_color = freetemp(szty(p->n_type));		\
		w->r_class = FPREG;				\
	}							\
	if (w->r_color < MAXZP*2) { /* color in bytes */	\
		p->n_op = NAME;					\
		p->n_lval = w->r_color/2 + ZPOFF;		\
		p->n_name = "";					\
		break;						\
	}							\
}

/*
 * special shapes for sp/fp.
 */
#define	SLDFPSP		(MAXSPECIAL+1)	/* load fp or sp */
