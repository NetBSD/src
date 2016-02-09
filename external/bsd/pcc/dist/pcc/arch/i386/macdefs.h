/*	Id: macdefs.h,v 1.93 2015/11/24 17:35:11 ragge Exp 	*/	
/*	$NetBSD: macdefs.h,v 1.1.1.6 2016/02/09 20:28:16 plunky Exp $	*/
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
#define SZINT		32
#define SZFLOAT		32
#define SZDOUBLE	64
#ifdef MACHOABI
#define SZLDOUBLE	128
#else
#define SZLDOUBLE	96
#endif
#define SZLONG		32
#define SZSHORT		16
#define SZLONGLONG	64
#define SZPOINT(t)	32

/*
 * Alignment constraints
 */
#define ALCHAR		8
#define ALBOOL		8
#define ALINT		32
#define ALFLOAT		32
#define ALDOUBLE	32
#ifdef MACHOABI
#define ALLDOUBLE	128
#else
#define ALLDOUBLE	32
#endif
#define ALLONG		32
#define ALLONGLONG	32
#define ALSHORT		16
#define ALPOINT		32
#undef ALSTRUCT		/* Not defined if ELF ABI */
#define ALSTACK		32 
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
#define	MIN_INT		(-0x7fffffff-1)
#define	MAX_INT		0x7fffffff
#define	MAX_UNSIGNED	0xffffffff
#define	MIN_LONG	MIN_INT
#define	MAX_LONG	MAX_INT
#define	MAX_ULONG	MAX_UNSIGNED
#define	MIN_LONGLONG	0x8000000000000000LL
#define	MAX_LONGLONG	0x7fffffffffffffffLL
#define	MAX_ULONGLONG	0xffffffffffffffffULL

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
#if defined(ELFABI)
#define LABFMT	".L%d"		/* format for printing labels */
#define	STABLBL	".LL%d"		/* format for stab (debugging) labels */
#else
#define LABFMT	"L%d"		/* format for printing labels */
#define	STABLBL	"LL%d"		/* format for stab (debugging) labels */
#endif
#ifdef LANG_F77
#define BLANKCOMMON "_BLNK_"
#define MSKIREG  (M(TYSHORT)|M(TYLONG))
#define TYIREG TYLONG
#define FSZLENG  FSZLONG
#define	AUTOREG	EBP
#define	ARGREG	EBP
#define ARGOFFSET 8
#endif

#ifdef MACHOABI
#define STAB_LINE_ABSOLUTE	/* S_LINE fields use absolute addresses */
#define	MYALIGN			/* user power-of-2 alignment */
#endif

#define BACKAUTO 		/* stack grows negatively for automatics */
#define BACKTEMP 		/* stack grows negatively for temporaries */

#undef	FIELDOPS		/* no bit-field instructions */
#define TARGET_ENDIAN TARGET_LE

#define FINDMOPS	/* i386 has instructions that modifies memory */
#define	CC_DIV_0	/* division by zero is safe in the compiler */

/* Definitions mostly used in pass2 */

#define BYTEOFF(x)	((x)&03)
#define wdal(k)		(BYTEOFF(k)==0)

#define STOARG(p)
#define STOFARG(p)
#define STOSTARG(p)
#define genfcall(a,b)	gencall(a,b)

#define	szty(t)	(((t) == DOUBLE || (t) == FLOAT || \
	(t) == LONGLONG || (t) == ULONGLONG) ? 2 : (t) == LDOUBLE ? 3 : 1)

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
 *	C - long long regs
 *	D - floating point
 */
#define	EAX	000	/* Scratch and return register */
#define	EDX	001	/* Scratch and secondary return register */
#define	ECX	002	/* Scratch (and shift count) register */
#define	EBX	003	/* GDT pointer or callee-saved temporary register */
#define	ESI	004	/* Callee-saved temporary register */
#define	EDI	005	/* Callee-saved temporary register */
#define	EBP	006	/* Frame pointer */
#define	ESP	007	/* Stack pointer */

#define	AL	010
#define	AH	011
#define	DL	012
#define	DH	013
#define	CL	014
#define	CH	015
#define	BL	016
#define	BH	017

#define	EAXEDX	020
#define	EAXECX	021
#define	EAXEBX	022
#define	EAXESI	023
#define	EAXEDI	024
#define	EDXECX	025
#define	EDXEBX	026
#define	EDXESI	027
#define	EDXEDI	030
#define	ECXEBX	031
#define	ECXESI	032
#define	ECXEDI	033
#define	EBXESI	034
#define	EBXEDI	035
#define	ESIEDI	036

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
	{ AL, AH, EAXEDX, EAXECX, EAXEBX, EAXESI, EAXEDI, -1 },\
	{ DL, DH, EAXEDX, EDXECX, EDXEBX, EDXESI, EDXEDI, -1 },\
	{ CL, CH, EAXECX, EDXECX, ECXEBX, ECXESI, ECXEDI, -1 },\
	{ BL, BH, EAXEBX, EDXEBX, ECXEBX, EBXESI, EBXEDI, -1 },\
	{ EAXESI, EDXESI, ECXESI, EBXESI, ESIEDI, -1 },\
	{ EAXEDI, EDXEDI, ECXEDI, EBXEDI, ESIEDI, -1 },\
	{ -1 },\
	{ -1 },\
\
	/* 8 char registers */\
	{ EAX, EAXEDX, EAXECX, EAXEBX, EAXESI, EAXEDI, -1 },\
	{ EAX, EAXEDX, EAXECX, EAXEBX, EAXESI, EAXEDI, -1 },\
	{ EDX, EAXEDX, EDXECX, EDXEBX, EDXESI, EDXEDI, -1 },\
	{ EDX, EAXEDX, EDXECX, EDXEBX, EDXESI, EDXEDI, -1 },\
	{ ECX, EAXECX, EDXECX, ECXEBX, ECXESI, ECXEDI, -1 },\
	{ ECX, EAXECX, EDXECX, ECXEBX, ECXESI, ECXEDI, -1 },\
	{ EBX, EAXEBX, EDXEBX, ECXEBX, EBXESI, EBXEDI, -1 },\
	{ EBX, EAXEBX, EDXEBX, ECXEBX, EBXESI, EBXEDI, -1 },\
\
	/* 15 long-long-emulating registers */\
	{ EAX, AL, AH, EDX, DL, DH, EAXECX, EAXEBX, EAXESI,	/* eaxedx */\
	  EAXEDI, EDXECX, EDXEBX, EDXESI, EDXEDI, -1, },\
	{ EAX, AL, AH, ECX, CL, CH, EAXEDX, EAXEBX, EAXESI,	/* eaxecx */\
	  EAXEDI, EDXECX, ECXEBX, ECXESI, ECXEDI, -1 },\
	{ EAX, AL, AH, EBX, BL, BH, EAXEDX, EAXECX, EAXESI,	/* eaxebx */\
	  EAXEDI, EDXEBX, ECXEBX, EBXESI, EBXEDI, -1 },\
	{ EAX, AL, AH, ESI, EAXEDX, EAXECX, EAXEBX, EAXEDI,	/* eaxesi */\
	  EDXESI, ECXESI, EBXESI, ESIEDI, -1 },\
	{ EAX, AL, AH, EDI, EAXEDX, EAXECX, EAXEBX, EAXESI,	/* eaxedi */\
	  EDXEDI, ECXEDI, EBXEDI, ESIEDI, -1 },\
	{ EDX, DL, DH, ECX, CL, CH, EAXEDX, EAXECX, EDXEBX,	/* edxecx */\
	  EDXESI, EDXEDI, ECXEBX, ECXESI, ECXEDI, -1 },\
	{ EDX, DL, DH, EBX, BL, BH, EAXEDX, EDXECX, EDXESI,	/* edxebx */\
	  EDXEDI, EAXEBX, ECXEBX, EBXESI, EBXEDI, -1 },\
	{ EDX, DL, DH, ESI, EAXEDX, EDXECX, EDXEBX, EDXEDI,	/* edxesi */\
	  EAXESI, ECXESI, EBXESI, ESIEDI, -1 },\
	{ EDX, DL, DH, EDI, EAXEDX, EDXECX, EDXEBX, EDXESI,	/* edxedi */\
	  EAXEDI, ECXEDI, EBXEDI, ESIEDI, -1 },\
	{ ECX, CL, CH, EBX, BL, BH, EAXECX, EDXECX, ECXESI,	/* ecxebx */\
	  ECXEDI, EAXEBX, EDXEBX, EBXESI, EBXEDI, -1 },\
	{ ECX, CL, CH, ESI, EAXECX, EDXECX, ECXEBX, ECXEDI,	/* ecxesi */\
	  EAXESI, EDXESI, EBXESI, ESIEDI, -1 },\
	{ ECX, CL, CH, EDI, EAXECX, EDXECX, ECXEBX, ECXESI,	/* ecxedi */\
	  EAXEDI, EDXEDI, EBXEDI, ESIEDI, -1 },\
	{ EBX, BL, BH, ESI, EAXEBX, EDXEBX, ECXEBX, EBXEDI,	/* ebxesi */\
	  EAXESI, EDXESI, ECXESI, ESIEDI, -1 },\
	{ EBX, BL, BH, EDI, EAXEBX, EDXEBX, ECXEBX, EBXESI,	/* ebxedi */\
	  EAXEDI, EDXEDI, ECXEDI, ESIEDI, -1 },\
	{ ESI, EDI, EAXESI, EDXESI, ECXESI, EBXESI,		/* esiedi */\
	  EAXEDI, EDXEDI, ECXEDI, EBXEDI, -1 },\
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
#define PCLASS(p) (p->n_type <= UCHAR ? SBREG : \
		  (p->n_type == LONGLONG || p->n_type == ULONGLONG ? SCREG : \
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
			 x == LONGLONG || x == ULONGLONG ? EAXEDX : \
			 x == FLOAT || x == DOUBLE || x == LDOUBLE ? 31 : EAX)

#if 0
#define R2REGS	1	/* permit double indexing */
#endif

/* XXX - to die */
#define FPREG	EBP	/* frame pointer */
#define STKREG	ESP	/* stack pointer */

#define	SHSTR		(MAXSPECIAL+1)	/* short struct */
#define	SFUNCALL	(MAXSPECIAL+2)	/* struct assign after function call */
#define	SPCON		(MAXSPECIAL+3)	/* positive nonnamed constant */

/*
 * Specials that indicate the applicability of machine idioms.
 */
#define SMIXOR		(MAXSPECIAL+4)
#define SMILWXOR	(MAXSPECIAL+5)
#define SMIHWXOR	(MAXSPECIAL+6)

/*
 * i386-specific symbol table flags.
 */
#define	SSECTION	SLOCAL1
#define SSTDCALL	SLOCAL2	
#define SDLLINDIRECT	SLOCAL3

/*
 * i386-specific interpass stuff.
 */

#define TARGET_IPP_MEMBERS			\
	int ipp_argstacksize;

#define	target_members_print_prolog(ipp) printf("%d", ipp->ipp_argstacksize)
#define	target_members_print_epilog(ipp) printf("%d", ipp->ipp_argstacksize)
#define target_members_read_prolog(ipp) ipp->ipp_argstacksize = rdint(&p)
#define target_members_read_epilog(ipp) ipp->ipp_argstacksize = rdint(&p)

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
	c = 'r'; addalledges(&ablock[ESI]); addalledges(&ablock[EDI]); }

#if defined(MACHOABI)
struct stub {
	struct { struct stub *q_forw, *q_back; } link;
	char *name;
};    
extern struct stub stublist;
extern struct stub nlplist;
void addstub(struct stub *list, char *name);
#endif

/* -m flags */
extern int msettings;
#define	MI386	0x001
#define	MI486	0x002
#define	MI586	0x004
#define	MI686	0x008
#define	MCPUMSK	0x00f

/* target specific attributes */
#define	ATTR_MI_TARGET	ATTR_I386_FCMPLRET, ATTR_I386_FPPOP
#define NATIVE_FLOATING_POINT
