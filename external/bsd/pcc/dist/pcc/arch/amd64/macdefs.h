/*	$Id: macdefs.h,v 1.1.1.1 2009/09/04 00:27:30 gmcgarry Exp $	*/
/*
 * Copyright (c) 2008 Michael Shalayeff
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

#define ARGINIT		128	/* # bits above fp where arguments start */
#define AUTOINIT	0	/* # bits below fp where automatics start */

/*
 * Storage space requirements
 */
#define SZCHAR		8
#define SZBOOL		8
#define SZSHORT		16
#define SZINT		32
#define SZLONG		64
#define SZPOINT(t)	64
#define SZLONGLONG	64
#define SZFLOAT		32
#define SZDOUBLE	64
#define SZLDOUBLE	128

/*
 * Alignment constraints
 */
#define ALCHAR		8
#define ALBOOL		8
#define ALSHORT		16
#define ALINT		32
#define ALLONG		64
#define ALPOINT		64
#define ALLONGLONG	64
#define ALFLOAT		32
#define ALDOUBLE	64
#define ALLDOUBLE	128
/* #undef ALSTRUCT	amd64 struct alignment is member defined */
#define ALSTACK		64
#define ALMAX		128 

/*
 * Min/max values.
 */
#define	MIN_CHAR	-128
#define	MAX_CHAR	127
#define	MAX_UCHAR	255
#define	MIN_SHORT	-32768
#define	MAX_SHORT	32767
#define	MAX_USHORT	65535
#define	MIN_INT		(-0x7fffffff-1)
#define	MAX_INT		0x7fffffff
#define	MAX_UNSIGNED	0xffffffff
#define	MIN_LONG	MIN_LONGLONG
#define	MAX_LONG	MAX_LONGLONG
#define	MAX_ULONG	MAX_ULONGLONG
#define	MIN_LONGLONG	0x8000000000000000LL
#define	MAX_LONGLONG	0x7fffffffffffffffLL
#define	MAX_ULONGLONG	0xffffffffffffffffULL

/* Default char is signed */
#undef	CHAR_UNSIGNED
#define	BOOL_TYPE	CHAR	/* what used to store _Bool */

/*
 * Use large-enough types.
 */
typedef	long long CONSZ;
typedef	unsigned long long U_CONSZ;
typedef long long OFFSZ;

#define CONFMT	"%lld"		/* format for printing constants */
#define LABFMT	".L%d"		/* format for printing labels */
#define	STABLBL	".LL%d"		/* format for stab (debugging) labels */
#ifdef LANG_F77
#define BLANKCOMMON "_BLNK_"
#define MSKIREG  (M(TYSHORT)|M(TYLONG))
#define TYIREG TYLONG
#define FSZLENG  FSZLONG
#define	AUTOREG	EBP
#define	ARGREG	EBP
#define ARGOFFSET 8
#endif

#define BACKAUTO 		/* stack grows negatively for automatics */
#define BACKTEMP 		/* stack grows negatively for temporaries */

#undef	FIELDOPS		/* no bit-field instructions */
#define	RTOLBYTES		/* bytes are numbered right to left */

#define ENUMSIZE(high,low) INT	/* enums are always stored in full int */

#define FINDMOPS	/* i386 has instructions that modifies memory */

/* Definitions mostly used in pass2 */

#define BYTEOFF(x)	((x)&07)
#define wdal(k)		(BYTEOFF(k)==0)
#define BITOOR(x)	(x)	/* bit offset to oreg offset XXX die! */

#define STOARG(p)
#define STOFARG(p)
#define STOSTARG(p)
#define genfcall(a,b)	gencall(a,b)

/* How many integer registers are needed? (used for stack allocation) */
#define	szty(t)	(t < LONG || t == FLOAT ? 1 : t == LDOUBLE ? 4 : 2)

/*
 * The amd64 architecture has a much cleaner interface to its registers
 * than the x86, even though a part of the register block comes from 
 * the x86 architecture.  Therefore currently only two non-overlapping 
 * register classes are used; integer and xmm registers.
 *
 * All registers are given a sequential number to
 * identify it which must match rnames[] in local2.c.
 *
 * The classes used on amd64 are:
 *	A - integer registers
 *	B - xmm registers
 */
#define	RAX	000
#define	RDX	001
#define	RCX	002
#define	RBX	003
#define	RSI	004
#define	RDI	005
#define	RBP	006
#define	RSP	007
#define	R08	010
#define	R09	011
#define	R10	012
#define	R11	013
#define	R12	014
#define	R13	015
#define	R14	016
#define	R15	017

#define	XMM0	020
#define	XMM1	021
#define	XMM2	022
#define	XMM3	023
#define	XMM4	024
#define	XMM5	025
#define	XMM6	026
#define	XMM7	027
#define	XMM8	030
#define	XMM9	031
#define	XMM10	032
#define	XMM11	033
#define	XMM12	034
#define	XMM13	035
#define	XMM14	036
#define	XMM15	037

#define	MAXREGS	040	/* 32 registers */

#define	RSTATUS	\
	SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|PERMREG,	\
	SAREG|TEMPREG, SAREG|TEMPREG, 0, 0,	 			\
	SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG,	\
	SAREG|PERMREG, SAREG|PERMREG, SAREG|PERMREG, SAREG|PERMREG, 	\
	SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG,	\
	SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG,	\
	SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG,	\
	SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG,

/* no overlapping registers at all */
#define	ROVERLAP \
	{ -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, \
	{ -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, \
	{ -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, \
	{ -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 }, { -1 },


/* Return a register class based on the type of the node */
#define PCLASS(p) (p->n_type >= FLOAT && p->n_type <= LDOUBLE ? SBREG : SAREG)

#define	NUMCLASS 	2	/* highest number of reg classes used */

int COLORMAP(int c, int *r);
#define	GCLASS(x) (x < 16 ? CLASSA : CLASSB)
#define DECRA(x,y)	(((x) >> (y*8)) & 255)	/* decode encoded regs */
#define	ENCRD(x)	(x)		/* Encode dest reg in n_reg */
#define ENCRA1(x)	((x) << 8)	/* A1 */
#define ENCRA2(x)	((x) << 16)	/* A2 */
#define ENCRA(x,y)	((x) << (8+y*8))	/* encode regs in int */

#define	RETREG(x)	(x == FLOAT || x == DOUBLE || x == LDOUBLE ? XMM0 : RAX)

/* XXX - to die */
#define FPREG	RBP	/* frame pointer */
#define STKREG	RSP	/* stack pointer */

#define	SHSTR		(MAXSPECIAL+1)	/* short struct */
#define	SFUNCALL	(MAXSPECIAL+2)	/* struct assign after function call */
#define	SPCON		(MAXSPECIAL+3)	/* positive nonnamed constant */

/*
 * Specials that indicate the applicability of machine idioms.
 */
#define SMIXOR		(MAXSPECIAL+4)
#define SMILWXOR	(MAXSPECIAL+5)
#define SMIHWXOR	(MAXSPECIAL+6)
#define SCON32		(MAXSPECIAL+7)	/* 32-bit constant */

/*
 * i386-specific symbol table flags.
 */
#define	SSECTION	SLOCAL1
#define	STLS		SLOCAL2
#define	SNOUNDERSCORE	SLOCAL3
#define SSTDCALL	SLOCAL2	
#define SDLLINDIRECT	SLOCAL3

/*
 * i386-specific node flags.
 */
#define FSTDCALL	0x01

/*
 * i386-specific interpass stuff.
 */

#define TARGET_IPP_MEMBERS			\
	int ipp_argstacksize;

/*
 * Extended assembler macros.
 */
void targarg(char *w, void *arg);
#define	XASM_TARGARG(w, ary)	\
	(w[1] == 'b' || w[1] == 'h' || w[1] == 'w' || w[1] == 'k' ? \
	w++, targarg(w, ary), 1 : 0)
int numconv(void *ip, void *p, void *q);
#define	XASM_NUMCONV(ip, p, q)	numconv(ip, p, q)

/*
 * builtins.
 */
#define TARGET_VALIST
#define TARGET_STDARGS
#define TARGET_BUILTINS							\
	{ "__builtin_stdarg_start", amd64_builtin_stdarg_start },	\
	{ "__builtin_va_start", amd64_builtin_stdarg_start },	\
	{ "__builtin_va_arg", amd64_builtin_va_arg },			\
	{ "__builtin_va_end", amd64_builtin_va_end },			\
	{ "__builtin_va_copy", amd64_builtin_va_copy },			\
	{ "__builtin_frame_address", i386_builtin_frame_address },	\
	{ "__builtin_return_address", i386_builtin_return_address },

#define NODE struct node
struct node;
NODE *amd64_builtin_stdarg_start(NODE *f, NODE *a);
NODE *amd64_builtin_va_arg(NODE *f, NODE *a);
NODE *amd64_builtin_va_end(NODE *f, NODE *a);
NODE *amd64_builtin_va_copy(NODE *f, NODE *a);
NODE *i386_builtin_frame_address(NODE *f, NODE *a);
NODE *i386_builtin_return_address(NODE *f, NODE *a);
#undef NODE
