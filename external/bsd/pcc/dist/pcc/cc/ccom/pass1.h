/*	Id: pass1.h,v 1.291 2016/02/09 17:57:35 ragge Exp 	*/	
/*	$NetBSD: pass1.h,v 1.5 2016/02/09 20:37:32 plunky Exp $	*/
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

#include "config.h"

#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdlib.h>

#ifndef MKEXT
#include "external.h"
#else
typedef unsigned int bittype; /* XXX - for basicblock */
#endif
#include "manifest.h"
#include "softfloat.h"

/*
 * Storage classes
 */
#define SNULL		0
#define AUTO		1
#define EXTERN		2
#define STATIC		3
#define REGISTER	4
#define EXTDEF		5
#define THLOCAL		6
#define KEYWORD		7
#define MOS		8
#define PARAM		9
#define STNAME		10
#define MOU		11
#define UNAME		12
#define TYPEDEF		13
/* #define FORTRAN		14 */
#define ENAME		15
#define MOE		16
/* #define UFORTRAN 	17 */
#define USTATIC		18

	/* field size is ORed in */
#define FIELD		0200
#define FLDSIZ		0177
extern	char *scnames(int);

/*
 * Symbol table flags
 */
#define	SNORMAL		0
#define	STAGNAME	01
#define	SLBLNAME	02
#define	SMOSNAME	03
#define	SSTRING		04
#define	NSTYPES		05
#define	SMASK		07

#define	STLS		00010	/* Thread Local Support variable */
#define SINSYS		00020	/* Declared in system header */
#define	SSTMT		SINSYS	/* Allocate symtab on statement stack */
#define SNOCREAT	00040	/* don't create a symbol in lookup() */
#define STEMP		00100	/* Allocate symtab from temp or perm mem */
#define	SDYNARRAY	00200	/* symbol is dynamic array on stack */
#define	SINLINE		00400	/* function is of type inline */
#define	SBLK		SINLINE	/* Allocate symtab from blk mem */
#define	STNODE		01000	/* symbol shall be a temporary node */
#define	SBUILTIN	02000	/* this is a builtin function */
#define	SASG		04000	/* symbol is assigned to already */
#define	SLOCAL1		010000
#define	SLOCAL2		020000
#define	SLOCAL3		040000

	/* alignment of initialized quantities */
#ifndef AL_INIT
#define	AL_INIT ALINT
#endif

struct rstack;
struct symtab;
union arglist;
#ifdef GCC_COMPAT
struct gcc_attr_pack;
#endif

/*
 * Dimension/prototype information.
 * 	ddim > 0 holds the dimension of an array.
 *	ddim < 0 is a dynamic array and refers to a tempnode.
 *	...unless:
 *		ddim == NOOFFSET, an array without dimenston, "[]"
 *		ddim == -1, dynamic array while building before defid.
 */
union dimfun {
	int	ddim;		/* Dimension of an array */
	union arglist *dfun;	/* Prototype index */
};

/*
 * Argument list member info when storing prototypes.
 */
union arglist {
	TWORD type;
	union dimfun *df;
	struct attr *sap;
};
#define TNULL		INCREF(FARG) /* pointer to FARG -- impossible type */
#define TELLIPSIS 	INCREF(INCREF(FARG))

/*
 * Symbol table definition.
 */
struct	symtab {
	struct	symtab *snext;	/* link to other symbols in the same scope */
	int	soffset;	/* offset or value */
	char	sclass;		/* storage class */
	char	slevel;		/* scope level */
	short	sflags;		/* flags, see below */
	char	*sname;		/* Symbol name */
	TWORD	stype;		/* type word */
	TWORD	squal;		/* qualifier word */
	union	dimfun *sdf;	/* ptr to the dimension/prototype array */
	struct	attr *sap;	/* the base type attribute list */
};

#define	ISSOU(ty)   ((ty) == STRTY || (ty) == UNIONTY)

/*
 * External definitions
 */
struct swents {			/* switch table */
	struct swents *next;	/* Next struct in linked list */
	CONSZ	sval;		/* case value */
	int	slab;		/* associated label */
};
int mygenswitch(int, TWORD, struct swents **, int);

extern	int blevel;
extern	int oldstyle;

extern	int lineno, nerrors, issyshdr;

extern	char *ftitle;
extern	struct symtab *cftnsp;
extern	int autooff, maxautooff, argoff;

extern	OFFSZ inoff;

extern	int reached;
extern	int isinlining;
extern	int xinline, xgnu89, xgnu99;
extern	int bdebug, ddebug, edebug, idebug, ndebug;
extern	int odebug, pdebug, sdebug, tdebug, xdebug;

/* various labels */
extern	int brklab;
extern	int contlab;
extern	int flostat;
extern	int retlab;
extern	int doing_init, statinit;
extern	short sztable[];
extern	char *astypnames[];

/* pragma globals */
extern int pragma_allpacked, pragma_packed, pragma_aligned;

/*
 * Flags used in the (elementary) flow analysis ...
 */
#define FBRK		02
#define FCONT		04
#define FDEF		010
#define FLOOP		020

/*
 * Location counters
 */
#define NOSEG		-1
#define PROG		0		/* (ro) program segment */
#define DATA		1		/* (rw) data segment */
#define RDATA		2		/* (ro) data segment */
#define LDATA		3		/* (rw) local data */
#define UDATA		4		/* (rw) uninitialized data */
#define STRNG		5		/* (ro) string segment */
#define PICDATA		6		/* (rw) relocatable data segment */
#define PICRDATA	7		/* (ro) relocatable data segment */
#define PICLDATA	8		/* (rw) local relocatable data */
#define TLSDATA		9		/* (rw) TLS data segment */
#define TLSUDATA	10		/* (rw) TLS uninitialized segment */
#define CTORS		11		/* constructor */
#define DTORS		12		/* destructor */
#define	NMSEG		13		/* other (named) segment */

extern int lastloc;
void locctr(int type, struct symtab *sp);
void setseg(int type, char *name);
void defalign(int al);
void symdirec(struct symtab *sp);

/*
 * Tree struct for pass1.  
 */
typedef struct p1node {
	int	n_op;
	TWORD	n_type;
	TWORD	n_qual;
	union {
		char *	_name;
		union	dimfun *_df;
	} n_5;
	struct attr *n_ap;
	union {
		struct {
			union {
				struct p1node *_left;
				CONSZ _val;
			} n_l;
			union {
				struct p1node *_right;
				int _rval;
				struct symtab *_sp;
			} n_r;
		} n_u;
		struct {
			union flt *_dcon;
			union flt *_ccon;
		};
	} n_f;
} P1ND;

#define glval(p)	((p)->n_f.n_u.n_l._val)
#define slval(p,v)	((p)->n_f.n_u.n_l._val = (v))
#define	n_ccon		n_f._ccon


/*	mark an offset which is undefined */

#define NOOFFSET	(-10201)

/* declarations of various functions */
extern	P1ND
	*buildtree(int, P1ND *, P1ND *r),
	*mkty(unsigned, union dimfun *, struct attr *),
	*rstruct(char *, int),
	*dclstruct(struct rstack *),
	*strend(char *, TWORD),
	*tymerge(P1ND *, P1ND *),
	*stref(P1ND *),
#ifdef WORD_ADDRESSED
	*offcon(OFFSZ, TWORD, union dimfun *, struct attr *),
#endif
	*bcon(int),
	*xbcon(CONSZ, struct symtab *, TWORD),
	*bpsize(P1ND *),
	*convert(P1ND *, int),
	*pconvert(P1ND *),
	*oconvert(P1ND *),
	*ptmatch(P1ND *),
	*makety(P1ND *, TWORD, TWORD, union dimfun *, struct attr *),
	*block(int, P1ND *, P1ND *, TWORD, union dimfun *, struct attr *),
	*doszof(P1ND *),
	*p1alloc(void),
	*optim(P1ND *),
	*clocal(P1ND *),
	*tempnode(int, TWORD, union dimfun *, struct attr *),
	*eve(P1ND *),
	*doacall(struct symtab *, P1ND *, P1ND *);
P1ND	*intprom(P1ND *);
OFFSZ	tsize(TWORD, union dimfun *, struct attr *),
	psize(P1ND *);
P1ND *	typenode(P1ND *new);
void	spalloc(P1ND *, P1ND *, OFFSZ);
char	*exname(char *);
union flt *floatcon(char *);
union flt *fhexcon(char *);
P1ND	*bdty(int op, ...);
extern struct rstack *rpole;

int oalloc(struct symtab *, int *);
void deflabel(char *, P1ND *);
void gotolabel(char *);
unsigned int esccon(char **);
void inline_start(struct symtab *, int class);
void inline_end(void);
void inline_addarg(struct interpass *);
void inline_ref(struct symtab *);
void inline_prtout(void);
void inline_args(struct symtab **, int);
P1ND *inlinetree(struct symtab *, P1ND *, P1ND *);
void ftnarg(P1ND *);
struct rstack *bstruct(char *, int, P1ND *);
void moedef(char *);
void beginit(struct symtab *);
void simpleinit(struct symtab *, P1ND *);
struct symtab *lookup(char *, int);
struct symtab *getsymtab(char *, int);
char *addstring(char *);
char *addname(char *);
void symclear(int);
struct symtab *hide(struct symtab *);
void soumemb(P1ND *, char *, int);
int talign(unsigned int, struct attr *);
void bfcode(struct symtab **, int);
int chkftn(union arglist *, union arglist *);
void branch(int);
void cbranch(P1ND *, P1ND *);
void extdec(struct symtab *);
void defzero(struct symtab *);
int falloc(struct symtab *, int, P1ND *);
TWORD ctype(TWORD);  
void inval(CONSZ, int, P1ND *);
int ninval(CONSZ, int, P1ND *);
void infld(CONSZ, int, CONSZ);
void zbits(CONSZ, int);
void instring(struct symtab *);
void inwstring(struct symtab *);
void plabel(int);
void bjobcode(void);
void ejobcode(int);
void calldec(P1ND *, P1ND *);
int cisreg(TWORD);
void asginit(P1ND *);
void desinit(P1ND *);
void endinit(int);
void endictx(void);
void sspinit(void);
void sspstart(void);
void sspend(void);
void ilbrace(void);
void irbrace(void);
CONSZ scalinit(P1ND *);
void p1print(char *, ...);
char *copst(int);
int cdope(int);
void myp2tree(P1ND *);
void lcommprint(void), strprint(void);
void lcommdel(struct symtab *);
P1ND *funcode(P1ND *);
struct symtab *enumhd(char *);
P1ND *enumdcl(struct symtab *);
P1ND *enumref(char *);
CONSZ icons(P1ND *);
CONSZ valcast(CONSZ v, TWORD t);
int mypragma(char *);
char *pragtok(char *);
int eat(int);
void fixdef(struct symtab *);
int cqual(TWORD, TWORD);
void defloc(struct symtab *);
int fldchk(int);
int nncon(P1ND *);
void cunput(char);
P1ND *nametree(struct symtab *sp);
void pass1_lastchance(struct interpass *);
void fldty(struct symtab *p);
struct suedef *sueget(struct suedef *p);
void complinit(void);
void kwinit(void);
P1ND *structref(P1ND *p, int f, char *name);
P1ND *cxop(int op, P1ND *l, P1ND *r);
P1ND *imop(int op, P1ND *l, P1ND *r);
P1ND *cxelem(int op, P1ND *p);
P1ND *cxconj(P1ND *p);
P1ND *cxcast(P1ND *p1, P1ND *p2);
P1ND *cxret(P1ND *p, P1ND *q);
P1ND *imret(P1ND *p, P1ND *q);
P1ND *cast(P1ND *p, TWORD t, TWORD q);
P1ND *ccast(P1ND *p, TWORD t, TWORD u, union dimfun *df, struct attr *sue);
int andable(P1ND *);
int conval(P1ND *, int, P1ND *);
int ispow2(CONSZ);
void defid(P1ND *q, int class);
void defid2(P1ND *q, int class, char *astr);
void efcode(void);
void ecomp(P1ND *p);
int upoff(int size, int alignment, int *poff);
void nidcl(P1ND *p, int class);
void nidcl2(P1ND *p, int class, char *astr);
void eprint(P1ND *, int, int *, int *);
int uclass(int class);
int notlval(P1ND *);
void ecode(P1ND *p);
void ftnend(void);
void dclargs(void);
int suemeq(struct attr *s1, struct attr *s2);
struct symtab *strmemb(struct attr *ap);
int yylex(void);
void yyerror(char *);
int pragmas_gcc(char *t);
int concast(P1ND *p, TWORD t);
char *stradd(char *old, char *new);
#ifdef WORD_ADDRESSED
#define rmpconv(p) (p)
#else
P1ND *rmpconv(P1ND *);
#endif
P1ND *optloop(P1ND *);
P1ND *nlabel(int label);
TWORD styp(void);
void *stmtalloc(size_t);
void *blkalloc(size_t);
void stmtfree(void);
void blkfree(void);
char *getexname(struct symtab *sp);
void putjops(P1ND *p, void *arg);

void p1walkf(P1ND *, void (*f)(P1ND *, void *), void *);
void p1fwalk(P1ND *t, void (*f)(P1ND *, int, int *, int *), int down);
void p1listf(P1ND *p, void (*f)(P1ND *));
void p1flist(P1ND *p, void (*f)(P1ND *, void *), void *);
P1ND *p1nfree(P1ND *);
void p1tfree(P1ND *);
P1ND *p1tcopy(P1ND *);

enum {	ATTR_FIRST = ATTR_MI_MAX + 1,

	/* PCC used attributes */
	ATTR_COMPLEX,	/* Internal definition of complex */
	xxxATTR_BASETYP,	/* Internal; see below */
	ATTR_QUALTYP,	/* Internal; const/volatile, see below */
	ATTR_ALIGNED,	/* Internal; also used as gcc type attribute */
	ATTR_NORETURN,	/* Function does not return */
	ATTR_STRUCT,	/* Internal; element list */
#define ATTR_MAX ATTR_STRUCT

	ATTR_P1LABELS,	/* used to store stuff while parsing */
	ATTR_SONAME,	/* output name of symbol */

#ifdef GCC_COMPAT
	/* type attributes */
	GCC_ATYP_PACKED,
	GCC_ATYP_SECTION,
	GCC_ATYP_TRANSP_UNION,
	GCC_ATYP_UNUSED,
	GCC_ATYP_DEPRECATED,
	GCC_ATYP_MAYALIAS,

	/* variable attributes */
	GCC_ATYP_MODE,

	/* function attributes */
	GCC_ATYP_FORMAT,
	GCC_ATYP_NONNULL,
	GCC_ATYP_SENTINEL,
	GCC_ATYP_WEAK,
	GCC_ATYP_FORMATARG,
	GCC_ATYP_GNU_INLINE,
	GCC_ATYP_MALLOC,
	GCC_ATYP_NOTHROW,
	GCC_ATYP_CONST,
	GCC_ATYP_PURE,
	GCC_ATYP_CONSTRUCTOR,
	GCC_ATYP_DESTRUCTOR,
	GCC_ATYP_VISIBILITY,
	GCC_ATYP_WARN_UNUSED_RESULT,
	GCC_ATYP_USED,
	GCC_ATYP_NO_INSTR_FUN,
	GCC_ATYP_NOINLINE,
	GCC_ATYP_ALIAS,
	GCC_ATYP_WEAKREF,
	GCC_ATYP_ALLOCSZ,
	GCC_ATYP_ALW_INL,
	GCC_ATYP_TLSMODEL,
	GCC_ATYP_ALIASWEAK,
	GCC_ATYP_RETURNS_TWICE,
	GCC_ATYP_WARNING,
	GCC_ATYP_NOCLONE,
	GCC_ATYP_REGPARM,
	GCC_ATYP_FASTCALL,

	/* other stuff */
	GCC_ATYP_BOUNDED,	/* OpenBSD extra boundary checks */

	/* OSX toolchain */
	GCC_ATYP_WEAKIMPORT,

	GCC_ATYP_MAX,
#endif
#ifdef ATTR_P1_TARGET
	ATTR_P1_TARGET,
#endif
	ATTR_P1_MAX
};

/*
#ifdef notdef
 * ATTR_BASETYP has the following layout:
 * aa[0].iarg has size
 * aa[1].iarg has alignment
#endif
 * ATTR_QUALTYP has the following layout:
 * aa[0].iarg has CON/VOL + FUN/ARY/PTR
 * Not defined yet...
 * aa[3].iarg is dimension for arrays (XXX future)
 * aa[3].varg is function defs for functions.
 */
#ifdef notdef
#define	atypsz	aa[0].iarg
#define	aalign	aa[1].iarg
#endif

/*
 * ATTR_STRUCT member list.
 */
#define amlist  aa[0].varg
#define amsize  aa[1].iarg
#define	strattr(x)	(attr_find(x, ATTR_STRUCT))

void gcc_init(void);
int gcc_keyword(char *);
struct attr *gcc_attr_parse(P1ND *);
void gcc_tcattrfix(P1ND *);
struct gcc_attrib *gcc_get_attr(struct suedef *, int);
void dump_attr(struct attr *gap);
void gcc_modefix(P1ND *);
P1ND *gcc_eval_timode(int op, P1ND *, P1ND *);
P1ND *gcc_eval_ticast(int op, P1ND *, P1ND *);
P1ND *gcc_eval_tiuni(int op, P1ND *);
struct attr *isti(P1ND *p);

#ifndef NO_C_BUILTINS
struct bitable {
	char *name;
	P1ND *(*fun)(const struct bitable *, P1ND *a);
	short flags;
#define	BTNOPROTO	001
#define	BTNORVAL	002
#define	BTNOEVE		004
#define	BTGNUONLY	010
	short narg;
	TWORD *tp;
	TWORD rt;
};

P1ND *builtin_check(struct symtab *, P1ND *a);
void builtin_init(void);

/* Some builtins targets need to implement */
P1ND *builtin_frame_address(const struct bitable *bt, P1ND *a);
P1ND *builtin_return_address(const struct bitable *bt, P1ND *a);
P1ND *builtin_cfa(const struct bitable *bt, P1ND *a);
#endif


#ifdef STABS
void stabs_init(void);
void stabs_file(char *);
void stabs_efile(char *);
void stabs_line(int);
void stabs_rbrac(int);
void stabs_lbrac(int);
void stabs_func(struct symtab *);
void stabs_newsym(struct symtab *);
void stabs_chgsym(struct symtab *);
void stabs_struct(struct symtab *, struct attr *);
#endif

#ifndef CHARCAST
/* to make character constants into character connstants */
/* this is a macro to defend against cross-compilers, etc. */
#define CHARCAST(x) (char)(x)
#endif

/* sometimes int is smaller than pointers */
#if SZPOINT(CHAR) <= SZINT
#define INTPTR  INT
#elif SZPOINT(CHAR) <= SZLONG
#define INTPTR  LONG
#elif SZPOINT(CHAR) <= SZLONGLONG
#define INTPTR  LONGLONG
#else
#error int size unknown
#endif

/* Generate a bitmask from a given type size */
#define SZMASK(y) ((((1LL << ((y)-1))-1) << 1) | 1)

/*
 * finction specifiers.
 */
#define	INLINE		1
#define	NORETURN	2

/*
 * C compiler first pass extra defines.
 */
#define	QUALIFIER	(MAXOP+1)
#define	CLASS		(MAXOP+2)
#define	RB		(MAXOP+3)
#define	DOT		(MAXOP+4)
#define	ELLIPSIS	(MAXOP+5)
#define	TYPE		(MAXOP+6)
#define	LB		(MAXOP+7)
#define	COMOP		(MAXOP+8)
#define	QUEST		(MAXOP+9)
#define	COLON		(MAXOP+10)
#define	ANDAND		(MAXOP+11)
#define	OROR		(MAXOP+12)
#define	NOT		(MAXOP+13)
#define	CAST		(MAXOP+14)
#define	STRING		(MAXOP+15)

/* The following must be in the same order as their NOASG counterparts */
#define	PLUSEQ		(MAXOP+16)
#define	MINUSEQ		(MAXOP+17)
#define	DIVEQ		(MAXOP+18)
#define	MODEQ		(MAXOP+19)
#define	MULEQ		(MAXOP+20)
#define	ANDEQ		(MAXOP+21)
#define	OREQ		(MAXOP+22)
#define	EREQ		(MAXOP+23)
#define	LSEQ		(MAXOP+24)
#define	RSEQ		(MAXOP+25)

#define	UNASG		(-(PLUSEQ-PLUS))+

#define INCR		(MAXOP+26)
#define DECR		(MAXOP+27)
#define SZOF		(MAXOP+28)
#define CLOP		(MAXOP+29)
#define ATTRIB		(MAXOP+30)
#define XREAL		(MAXOP+31)
#define XIMAG		(MAXOP+32)
#define TYMERGE		(MAXOP+33)
#define LABEL		(MAXOP+34)
#define BIQUEST		(MAXOP+35)
#define UPLUS		(MAXOP+36)
#define ALIGN		(MAXOP+37)
#define FUNSPEC		(MAXOP+38)
#define STREF		(MAXOP+39)

/*
 * The following types are only used in pass1.
 */
#define SIGNED		(MAXTYPES+1)
#define FARG		(MAXTYPES+2)
#define	FIMAG		(MAXTYPES+3)
#define	IMAG		(MAXTYPES+4)
#define	LIMAG		(MAXTYPES+5)
#define	FCOMPLEX	(MAXTYPES+6)
#define	COMPLEX		(MAXTYPES+7)
#define	LCOMPLEX	(MAXTYPES+8)
#define	ENUMTY		(MAXTYPES+9)

#define	ISFTY(x)	((x) >= FLOAT && (x) <= LDOUBLE)
#define	ISCTY(x)	((x) >= FCOMPLEX && (x) <= LCOMPLEX)
#define	ISITY(x)	((x) >= FIMAG && (x) <= LIMAG)
#define ANYCX(p) (p->n_type == STRTY && attr_find(p->n_ap, ATTR_COMPLEX))

#define coptype(o)	(cdope(o)&TYFLG)
#define clogop(o)	(cdope(o)&LOGFLG)
#define casgop(o)	(cdope(o)&ASGFLG)

#ifdef TWOPASS
#define	PRTPREF	"* "
#else
#define	PRTPREF ""
#endif

/*
 * Allocation routines.
 */
#if defined(__PCC__) || defined(__GNUC__)
#define	FUNALLO(x)	__builtin_alloca(x)
#define	FUNFREE(x)
#elif defined(HAVE_ALLOCA)
#define FUNALLO(x)      alloca(x)
#define FUNFREE(x)
#else
#define FUNALLO(x)	malloc(x)
#define FUNFREE(x)	free(x)
#endif
