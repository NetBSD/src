/*	Id: manifest.h,v 1.97 2011/08/31 18:02:24 plunky Exp 	*/	
/*	$NetBSD: manifest.h,v 1.1.1.4.2.1 2012/04/17 00:04:06 yamt Exp $	*/
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

#ifndef MANIFEST
#define	MANIFEST

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "macdefs.h"
#include "node.h"
#include "compat.h"

/*
 * Node types
 */
#define LTYPE	02		/* leaf */
#define UTYPE	04		/* unary */
#define BITYPE	010		/* binary */

/*
 * DSIZE is the size of the dope array
 */
#define DSIZE	(MAXOP+1)

/*
 * Type names, used in symbol table building.
 * The order of the integer types are important.
 * Signed types must have bit 0 unset, unsigned types set (used below).
 */
#define	UNDEF		0	/* free symbol table entry */
#define	BOOL		1 	/* function argument */
#define	CHAR		2
#define	UCHAR		3
#define	SHORT		4
#define	USHORT		5
#define	INT		6
#define	UNSIGNED	7
#define	LONG		8
#define	ULONG		9      
#define	LONGLONG	10
#define	ULONGLONG	11
#define	FLOAT		12
#define	DOUBLE		13
#define	LDOUBLE		14
#define	STRTY		15
#define	UNIONTY		16
#define	XTYPE		17	/* Extended target-specific type */
/* #define	MOETY		18 */	/* member of enum */
#define	VOID		19

#define	MAXTYPES	19	/* highest type+1 to be used by lang code */
/*
 * Various flags
 */
#define NOLAB	(-1)

/* 
 * Type modifiers.
 */
#define	PTR		0x20
#define	FTN		0x40
#define	ARY		0x60
#define	CON		0x20
#define	VOL		0x40

/*
 * Type packing constants
 */
#define TMASK	0x060
#define TMASK1	0x180
#define TMASK2	0x1e0
#define BTMASK	0x1f
#define BTSHIFT	5
#define TSHIFT	2

/*
 * Macros
 */
#define MODTYPE(x,y)	x = ((x)&(~BTMASK))|(y)	/* set basic type of x to y */
#define BTYPE(x)	((x)&BTMASK)		/* basic type of x */
#define	ISLONGLONG(x)	((x) == LONGLONG || (x) == ULONGLONG)
#define ISUNSIGNED(x)	(((x) <= ULONGLONG) && (((x) & 1) == (UNSIGNED & 1)))
#define UNSIGNABLE(x)	(((x)<=ULONGLONG&&(x)>=CHAR) && !ISUNSIGNED(x))
#define ENUNSIGN(x)	enunsign(x)
#define DEUNSIGN(x)	deunsign(x)
#define ISINTEGER(x)	((x) >= BOOL && (x) <= ULONGLONG)
#define ISPTR(x)	(((x)&TMASK)==PTR)
#define ISFTN(x)	(((x)&TMASK)==FTN)	/* is x a function type? */
#define ISARY(x)	(((x)&TMASK)==ARY)	/* is x an array type? */
#define	ISCON(x)	(((x)&CON)==CON)	/* is x const? */
#define	ISVOL(x)	(((x)&VOL)==VOL)	/* is x volatile? */
#define INCREF(x)	((((x)&~BTMASK)<<TSHIFT)|PTR|((x)&BTMASK))
#define INCQAL(x)	((((x)&~BTMASK)<<TSHIFT)|((x)&BTMASK))
#define DECREF(x)	((((x)>>TSHIFT)&~BTMASK)|((x)&BTMASK))
#define DECQAL(x)	((((x)>>TSHIFT)&~BTMASK)|((x)&BTMASK))
#define SETOFF(x,y)	{ if ((x)%(y) != 0) (x) = (((x)/(y) + 1) * (y)); }
		/* advance x to a multiple of y */
#define NOFIT(x,y,z)	(((x)%(z) + (y)) > (z))
		/* can y bits be added to x without overflowing z */

/* Endianness.	Target is expected to TARGET_ENDIAN to one of these  */
#define TARGET_LE	1
#define TARGET_BE	2
#define TARGET_PDP	3
#define TARGET_ANY	4

#ifndef SPECIAL_INTEGERS
#define	ASGLVAL(lval, val)
#endif

/*
 * Pack and unpack field descriptors (size and offset)
 */
#define PKFIELD(s,o)	(((o)<<7)| (s))
#define UPKFSZ(v)	((v)&0177)
#define UPKFOFF(v)	((v)>>7)

/*
 * Operator information
 */
#define TYFLG	016
#define ASGFLG	01
#define LOGFLG	020

#define SIMPFLG	040
#define COMMFLG	0100
#define DIVFLG	0200
#define FLOFLG	0400
#define LTYFLG	01000
#define CALLFLG	02000
#define MULFLG	04000
#define SHFFLG	010000
#define ASGOPFLG 020000

#define SPFLG	040000

#define	regno(p)	((p)->n_rval)	/* register number */

/*
 * 
 */
extern int gflag, kflag, pflag;
extern int sspflag;
extern int xssa, xtailcall, xtemps, xdeljumps, xdce;
extern int xuchar;

int yyparse(void);
void yyaccpt(void);

/*
 * List handling macros, similar to those in 4.4BSD.
 * The double-linked list is insque-style.
 */
/* Double-linked list macros */
#define	DLIST_INIT(h,f)		{ (h)->f.q_forw = (h); (h)->f.q_back = (h); }
#define	DLIST_ENTRY(t)		struct { struct t *q_forw, *q_back; }
#define	DLIST_NEXT(h,f)		(h)->f.q_forw
#define	DLIST_PREV(h,f)		(h)->f.q_back
#define DLIST_ISEMPTY(h,f)	((h)->f.q_forw == (h))
#define DLIST_ENDMARK(h)	(h)
#define	DLIST_FOREACH(v,h,f) \
	for ((v) = (h)->f.q_forw; (v) != (h); (v) = (v)->f.q_forw)
#define	DLIST_FOREACH_REVERSE(v,h,f) \
	for ((v) = (h)->f.q_back; (v) != (h); (v) = (v)->f.q_back)
#define	DLIST_INSERT_BEFORE(h,e,f) {	\
	(e)->f.q_forw = (h);		\
	(e)->f.q_back = (h)->f.q_back;	\
	(e)->f.q_back->f.q_forw = (e);	\
	(h)->f.q_back = (e);		\
}
#define	DLIST_INSERT_AFTER(h,e,f) {	\
	(e)->f.q_forw = (h)->f.q_forw;	\
	(e)->f.q_back = (h);		\
	(e)->f.q_forw->f.q_back = (e);	\
	(h)->f.q_forw = (e);		\
}
#define DLIST_REMOVE(e,f) {			 \
	(e)->f.q_forw->f.q_back = (e)->f.q_back; \
	(e)->f.q_back->f.q_forw = (e)->f.q_forw; \
}

/* Single-linked list */
#define	SLIST_INIT(h)	\
	{ (h)->q_forw = NULL; (h)->q_last = &(h)->q_forw; }
#define	SLIST_SETUP(h) { NULL, &(h)->q_forw }
#define	SLIST_ENTRY(t)	struct { struct t *q_forw; }
#define	SLIST_HEAD(n,t) struct n { struct t *q_forw, **q_last; }
#define	SLIST_ISEMPTY(h) ((h)->q_last == &(h)->q_forw)
#define	SLIST_FIRST(h)	((h)->q_forw)
#define	SLIST_FOREACH(v,h,f) \
	for ((v) = (h)->q_forw; (v) != NULL; (v) = (v)->f.q_forw)
#define	SLIST_INSERT_FIRST(h,e,f) {		\
	if ((h)->q_last == &(h)->q_forw)	\
		(h)->q_last = &(e)->f.q_forw;	\
	(e)->f.q_forw = (h)->q_forw;		\
	(h)->q_forw = (e);			\
}
#define	SLIST_INSERT_LAST(h,e,f) {	\
	(e)->f.q_forw = NULL;		\
	*(h)->q_last = (e);		\
	(h)->q_last = &(e)->f.q_forw;	\
}

#ifndef	MKEXT
/*
 * Functions for inter-pass communication.
 *
 */
struct interpass {
	DLIST_ENTRY(interpass) qelem;
	int type;
	int lineno;
	union {
		NODE *_p;
		int _locctr;
		int _label;
		int _curoff;
		char *_name;
	} _un;
};

/*
 * Special struct for prologue/epilogue.
 * - ip_lblnum contains the lowest/highest+1 label used
 * - ip_lbl is set before/after all code and after/before the prolog/epilog.
 */
struct interpass_prolog {
	struct interpass ipp_ip;
	char *ipp_name;		/* Function name */
	int ipp_vis;		/* Function visibility */
	TWORD ipp_type;		/* Function type */
#define	NIPPREGS	BIT2BYTE(MAXREGS)/sizeof(bittype)
	bittype ipp_regs[NIPPREGS];
				/* Bitmask of registers to save */
	int ipp_autos;		/* Size on stack needed */
	int ip_tmpnum;		/* # allocated temp nodes so far */
	int ip_lblnum;		/* # used labels so far */
#ifdef TARGET_IPP_MEMBERS
	TARGET_IPP_MEMBERS
#endif
};
#else
struct interpass { int dummy; };
struct interpass_prolog;
#endif /* !MKEXT */

/*
 * Epilog/prolog takes following arguments (in order):
 * - type
 * - regs
 * - autos
 * - name
 * - type
 * - retlab
 */

#define	ip_node	_un._p
#define	ip_locc	_un._locctr
#define	ip_lbl	_un._label
#define	ip_name	_un._name
#define	ip_asm	_un._name
#define	ip_off	_un._curoff

/* Types of inter-pass structs */
#define	IP_NODE		1
#define	IP_PROLOG	2
#define	IP_EPILOG	4
#define	IP_DEFLAB	5
#define	IP_DEFNAM	6
#define	IP_ASM		7
#define	MAXIP		7

void send_passt(int type, ...);
/*
 * External declarations, typedefs and the like
 */

/* used for memory allocation */
typedef struct mark {
	void *tmsav;
	void *tasav;
	int elem;
} MARK;

/* memory management stuff */
void *permalloc(int size);
void *tmpcalloc(int size);
void *tmpalloc(int size);
void tmpfree(void);
char *newstring(char *, int len);
char *tmpstrdup(char *str);
void markset(struct mark *m);
void markfree(struct mark *m);

/* command-line processing */
void mflags(char *);

void tprint(FILE *, TWORD, TWORD);

/* pass t communication subroutines */
void topt_compile(struct interpass *);

/* pass 2 communication subroutines */
void pass2_compile(struct interpass *);

/* node routines */
NODE *nfree(NODE *);
void tfree(NODE *);
NODE *tcopy(NODE *);
void walkf(NODE *, void (*f)(NODE *, void *), void *);
void fwalk(NODE *t, void (*f)(NODE *, int, int *, int *), int down);
void flist(NODE *p, void (*f)(NODE *, void *), void *);
void listf(NODE *p, void (*f)(NODE *));
NODE *listarg(NODE *p, int n, int *cnt);
void cerror(char *s, ...);
void werror(char *s, ...);
void uerror(char *s, ...);
void mkdope(void);
void tcheck(void);

extern	int nerrors;		/* number of errors seen so far */
extern	int warniserr;		/* treat warnings as errors */

/* gcc warning stuff */
#define	Wtruncate			0
#define	Wstrict_prototypes		1
#define	Wmissing_prototypes		2
#define	Wimplicit_int			3
#define	Wimplicit_function_declaration	4
#define	Wshadow				5
#define	Wpointer_sign			6
#define	Wsign_compare			7
#define	Wunknown_pragmas		8
#define	Wunreachable_code		9
#define	NUMW				10

void warner(int type, ...);
void Wflags(char *str);
TWORD deunsign(TWORD t);
TWORD enunsign(TWORD t);
#endif
