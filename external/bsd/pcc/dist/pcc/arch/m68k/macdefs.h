/*
 * Copyright (c) 2011 Janne Johansson <jj@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Machine-dependent defines for both passes.
 */

/*
 * Convert (multi-)character constant to integer.
 */
#define makecc(val,i)  lastcon = i ? (val<<8)|lastcon : val

#define ARGINIT		64	/* # bits above fp where arguments start */
#define AUTOINIT	0	/* # bits below fp where automatics start */

/*
 * Storage space requirements
 */
#define SZCHAR		8
#define SZBOOL		8
#define SZSHORT		16
#define SZINT		32
#define SZLONG		32
#define SZPOINT(t)	32
#define SZLONGLONG	64
#define SZFLOAT		32
#define SZDOUBLE	64
#define SZLDOUBLE	96	/* actually 80 */

/*
 * Alignment constraints
 */
#define ALCHAR		8
#define ALBOOL		8
#define ALSHORT		16
#define ALINT		16
#define ALLONG		16
#define ALPOINT		16
#define ALLONGLONG	16
#define ALFLOAT		16
#define ALDOUBLE	16
#define ALLDOUBLE	16
/* #undef ALSTRUCT	m68k struct alignment is member defined */
#define ALSTACK		16
#define ALMAX		16 

/*
 * Min/max values.
 */
#define MIN_CHAR	-128
#define MAX_CHAR	127
#define MAX_UCHAR	255
#define MIN_SHORT	-32768
#define MAX_SHORT	32767
#define MAX_USHORT	65535
#define MIN_INT		(-0x7fffffff-1)
#define MAX_INT		0x7fffffff
#define MAX_UNSIGNED	0xffffffffU
#define MIN_LONG	0x8000000000000000LL
#define MAX_LONG	0x7fffffffffffffffLL
#define MAX_ULONG	0xffffffffffffffffULL
#define MIN_LONGLONG	0x8000000000000000LL
#define MAX_LONGLONG	0x7fffffffffffffffLL
#define MAX_ULONGLONG	0xffffffffffffffffULL

/* Default char is signed */
#undef	CHAR_UNSIGNED
#define BOOL_TYPE	UCHAR	/* what used to store _Bool */

/*
 * Use large-enough types.
 */
typedef long long CONSZ;
typedef unsigned long long U_CONSZ;
typedef long long OFFSZ;

#define CONFMT	"%lld"		/* format for printing constants */
#define LABFMT	".L%d"		/* format for printing labels */
#define STABLBL ".LL%d"		/* format for stab (debugging) labels */
#ifdef LANG_F77
#define BLANKCOMMON "_BLNK_"
#define MSKIREG	 (M(TYSHORT)|M(TYLONG))
#define TYIREG TYLONG
#define FSZLENG	 FSZLONG
#define AUTOREG EBP
#define ARGREG	EBP
#define ARGOFFSET 8
#endif

#define BACKAUTO		/* stack grows negatively for automatics */
#define BACKTEMP		/* stack grows negatively for temporaries */

#undef	FIELDOPS		/* no bit-field instructions */
#define TARGET_ENDIAN TARGET_BE /* big-endian */

#undef FINDMOPS /* XXX FIXME */

#define CC_DIV_0	/* division by zero is safe in the compiler */

/* Definitions mostly used in pass2 */

#define BYTEOFF(x)	((x)&03)
#define wdal(k)		(BYTEOFF(k)==0)

#define STOARG(p)
#define STOFARG(p)
#define STOSTARG(p)
#define genfcall(a,b)	gencall(a,b)

/* How many integer registers are needed? (used for stack allocation) */
#define szty(t) ((t) == LDOUBLE ? 3 : \
	(t) == DOUBLE || DEUNSIGN(t) == LONGLONG ? 2 : 1)

/*
 * All registers are given a sequential number to
 * identify it which must match rnames[] in local2.c.
 *
 * The classes used on m68k are:
 *	A - 32-bit data registers
 *	B - 32-bit address registers
 *	C - 64-bit combined registers
 *	D - 80-bit floating point registers
 */
#define D0	0
#define D1	1
#define D2	2
#define D3	3
#define D4	4
#define D5	5
#define D6	6
#define D7	7

/* no support yet for using A registers for data calculations */
#define A0	8
#define A1	9
#define A2	10
#define A3	11
#define A4	12
#define A5	13
#define A6	14 /* frame pointer?  Isnt it A4 on amigaos.. */
#define A7	15  /* Stack pointer */

#define D0D1	16
#define D1D2	17
#define D2D3	18
#define D3D4	19
#define D4D5	20
#define D5D6	21
#define D6D7	22

#define	FP0	23
#define	FP1	24
#define	FP2	25
#define	FP3	26
#define	FP4	27
#define	FP5	28
#define	FP6	29
#define	FP7	30

#define MAXREGS 31	/* 31 registers */

#define RSTATUS \
	SAREG|TEMPREG, SAREG|TEMPREG, SAREG|PERMREG, SAREG|PERMREG, \
	SAREG|PERMREG, SAREG|PERMREG, SAREG|PERMREG, SAREG|PERMREG, \
	SBREG|TEMPREG, SBREG|TEMPREG, SBREG|PERMREG, SBREG|PERMREG, \
	SBREG|PERMREG, SBREG|PERMREG, 0, 0, /* fp and sp are ignored here */ \
	SCREG, SCREG, SCREG, SCREG, SCREG, SCREG, SCREG, \
	SDREG|TEMPREG, SDREG|TEMPREG, SDREG|PERMREG, SDREG|PERMREG, \
	SDREG|PERMREG, SDREG|PERMREG, SDREG|PERMREG, SDREG|PERMREG,


/* no overlapping registers at all */
#define ROVERLAP \
	/* 8 data registers */	 \
	{ D0D1, -1},\
	{ D1D2, D0D1, -1},\
	{ D2D3, D1D2,-1},\
	{ D3D4, D2D3,-1},\
	{ D4D5, D3D4,-1},\
	{ D5D6, D4D5,-1},\
	{ D6D7, D5D6,-1},\
	{ D6D7, -1},\
	/* 8 adress registers */  \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ D0, D1, D1D2, -1},	\
	{ D1, D2, D0D1, D2D3, -1},	\
	{ D2, D3, D1D2, D3D4, -1},	\
	{ D3, D4, D2D3, D4D5, -1},	\
	{ D4, D5, D3D4, D5D6, -1},	\
	{ D5, D6, D4D5, D6D7, -1},	\
	{ D6, D7, D5D6, -1},  \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1}, \
	{ -1},


/* Return a register class based on the type of the node */
#define PCLASS(p) (p->n_type == FLOAT || p->n_type == DOUBLE || \
	p->n_type == LDOUBLE ? SDREG : p->n_type == LONGLONG || \
	p->n_type == ULONGLONG ? SCREG : p->n_type > BTMASK ? SBREG : SAREG)

#define NUMCLASS	4	/* highest number of reg classes used */

int COLORMAP(int c, int *r);
#define GCLASS(x) (x < 8 ? CLASSA : x < 16 ? CLASSB : x < 23 ? CLASSC : CLASSD)
#define DECRA(x,y)	(((x) >> (y*6)) & 63)	/* decode encoded regs */
#define ENCRD(x)	(x)		/* Encode dest reg in n_reg */
#define ENCRA1(x)	((x) << 12)	/* A1 */
#define ENCRA2(x)	((x) << 18)	/* A2 */
#define ENCRA(x,y)	((x) << (6+y*6))	/* encode regs in int */

#define RETREG(x)	((x) == FLOAT || (x) == DOUBLE || (x) == LDOUBLE ? FP0 : \
	(x) == LONGLONG || (x) == ULONGLONG ? D0D1 : (x) > BTMASK ? A0 : D0)

#define FPREG	A6	/* frame pointer */
#define STKREG	A7	/* stack pointer */

#define HAVE_WEAKREF
#define TARGET_FLT_EVAL_METHOD	2	/* all as long double */
