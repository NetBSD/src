#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif
/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 2
#define YYMINOR 0

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)
#define YYENOMEM       (-2)
#define YYEOF          0
#undef YYBTYACC
#define YYBTYACC 0
#define YYDEBUGSTR YYPREFIX "debug"
#define YYPREFIX "yy"

#define YYPURE 0

#line 2 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
/*	$NetBSD: gram.y,v 1.56 2020/07/26 22:40:52 uwe Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)gram.y	8.1 (Berkeley) 6/6/93
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: gram.y,v 1.56 2020/07/26 22:40:52 uwe Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "defs.h"
#include "sem.h"

#define	FORMAT(n) (((n).fmt == 8 && (n).val != 0) ? "0%llo" : \
    ((n).fmt == 16) ? "0x%llx" : "%lld")

#define	stop(s)	cfgerror(s), exit(1)

static	struct	config conf;	/* at most one active at a time */
static	int	nowarn;		/* if warning suppression is on */


/*
 * Allocation wrapper functions
 */
static void wrap_alloc(void *ptr, unsigned code);
static void wrap_continue(void);
static void wrap_cleanup(void);

/*
 * Allocation wrapper type codes
 */
#define WRAP_CODE_nvlist	1
#define WRAP_CODE_defoptlist	2
#define WRAP_CODE_loclist	3
#define WRAP_CODE_attrlist	4
#define WRAP_CODE_condexpr	5

/*
 * The allocation wrappers themselves
 */
#define DECL_ALLOCWRAP(t)	static struct t *wrap_mk_##t(struct t *arg)

DECL_ALLOCWRAP(nvlist);
DECL_ALLOCWRAP(defoptlist);
DECL_ALLOCWRAP(loclist);
DECL_ALLOCWRAP(attrlist);
DECL_ALLOCWRAP(condexpr);

/* allow shorter names */
#define wrap_mk_loc(p) wrap_mk_loclist(p)
#define wrap_mk_cx(p) wrap_mk_condexpr(p)

/*
 * Macros for allocating new objects
 */

/* old-style for struct nvlist */
#define	new0(n,s,p,i,x)	wrap_mk_nvlist(newnv(n, s, p, i, x))
#define	new_n(n)	new0(n, NULL, NULL, 0, NULL)
#define	new_nx(n, x)	new0(n, NULL, NULL, 0, x)
#define	new_ns(n, s)	new0(n, s, NULL, 0, NULL)
#define	new_si(s, i)	new0(NULL, s, NULL, i, NULL)
#define	new_spi(s, p, i)	new0(NULL, s, p, i, NULL)
#define	new_nsi(n,s,i)	new0(n, s, NULL, i, NULL)
#define	new_np(n, p)	new0(n, NULL, p, 0, NULL)
#define	new_s(s)	new0(NULL, s, NULL, 0, NULL)
#define	new_p(p)	new0(NULL, NULL, p, 0, NULL)
#define	new_px(p, x)	new0(NULL, NULL, p, 0, x)
#define	new_sx(s, x)	new0(NULL, s, NULL, 0, x)
#define	new_nsx(n,s,x)	new0(n, s, NULL, 0, x)
#define	new_i(i)	new0(NULL, NULL, NULL, i, NULL)

/* new style, type-polymorphic; ordinary and for types with multiple flavors */
#define MK0(t)		wrap_mk_##t(mk_##t())
#define MK1(t, a0)	wrap_mk_##t(mk_##t(a0))
#define MK2(t, a0, a1)	wrap_mk_##t(mk_##t(a0, a1))
#define MK3(t, a0, a1, a2)	wrap_mk_##t(mk_##t(a0, a1, a2))

#define MKF0(t, f)		wrap_mk_##t(mk_##t##_##f())
#define MKF1(t, f, a0)		wrap_mk_##t(mk_##t##_##f(a0))
#define MKF2(t, f, a0, a1)	wrap_mk_##t(mk_##t##_##f(a0, a1))

/*
 * Data constructors
 */

static struct defoptlist *mk_defoptlist(const char *, const char *,
					const char *);
static struct loclist *mk_loc(const char *, const char *, long long);
static struct loclist *mk_loc_val(const char *, struct loclist *);
static struct attrlist *mk_attrlist(struct attrlist *, struct attr *);
static struct condexpr *mk_cx_atom(const char *);
static struct condexpr *mk_cx_not(struct condexpr *);
static struct condexpr *mk_cx_and(struct condexpr *, struct condexpr *);
static struct condexpr *mk_cx_or(struct condexpr *, struct condexpr *);

/*
 * Other private functions
 */

static	void	setmachine(const char *, const char *, struct nvlist *, int);
static	void	check_maxpart(void);

static struct loclist *present_loclist(struct loclist *ll);
static void app(struct loclist *, struct loclist *);
static struct loclist *locarray(const char *, int, struct loclist *, int);
static struct loclist *namelocvals(const char *, struct loclist *);

#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#line 155 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
typedef union {
	struct	attr *attr;
	struct	devbase *devb;
	struct	deva *deva;
	struct	nvlist *list;
	struct defoptlist *defoptlist;
	struct loclist *loclist;
	struct attrlist *attrlist;
	struct condexpr *condexpr;
	const char *str;
	struct	numconst num;
	int64_t	val;
	u_char	flag;
	devmajor_t devmajor;
	int32_t i32;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 199 "gram.c"

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

#if !(defined(yylex) || defined(YYSTATE))
int YYLEX_DECL();
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define AND 257
#define AT 258
#define ATTACH 259
#define BLOCK 260
#define BUILD 261
#define CHAR 262
#define COLONEQ 263
#define COMPILE_WITH 264
#define CONFIG 265
#define DEFFS 266
#define DEFINE 267
#define DEFOPT 268
#define DEFPARAM 269
#define DEFFLAG 270
#define DEFPSEUDO 271
#define DEFPSEUDODEV 272
#define DEVICE 273
#define DEVCLASS 274
#define DUMPS 275
#define DEVICE_MAJOR 276
#define ENDFILE 277
#define XFILE 278
#define FILE_SYSTEM 279
#define FLAGS 280
#define IDENT 281
#define IOCONF 282
#define LINKZERO 283
#define XMACHINE 284
#define MAJOR 285
#define MAKEOPTIONS 286
#define MAXUSERS 287
#define MAXPARTITIONS 288
#define MINOR 289
#define NEEDS_COUNT 290
#define NEEDS_FLAG 291
#define NO 292
#define CNO 293
#define XOBJECT 294
#define OBSOLETE 295
#define ON 296
#define OPTIONS 297
#define PACKAGE 298
#define PLUSEQ 299
#define PREFIX 300
#define BUILDPREFIX 301
#define PSEUDO_DEVICE 302
#define PSEUDO_ROOT 303
#define ROOT 304
#define SELECT 305
#define SINGLE 306
#define SOURCE 307
#define TYPE 308
#define VECTOR 309
#define VERSION 310
#define WITH 311
#define NUMBER 312
#define PATHNAME 313
#define QSTRING 314
#define WORD 315
#define EMPTYSTRING 316
#define ENDDEFS 317
#define YYERRCODE 256
typedef int YYINT;
static const YYINT yylhs[] = {                           -1,
    0,   58,   58,   62,   62,   62,   59,   59,   59,   59,
   59,   47,   47,   38,   38,   60,   63,   63,   63,   63,
   63,   64,   64,   64,   64,   64,   64,   64,   64,   64,
   64,   64,   64,   64,   64,   64,   64,   64,   64,   64,
   64,   64,   64,   65,   66,   67,   68,   68,   69,   69,
   69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
   79,   80,   81,   82,   83,   84,   85,   86,    1,    1,
    9,    9,   10,   10,   13,   13,   11,   11,   12,   53,
   53,   52,   52,   54,   54,   54,   55,   55,   57,   57,
   56,   40,   40,   39,   18,   18,   18,   20,   20,   21,
   21,   21,   21,   21,   21,   50,   50,   22,   24,   25,
   25,   26,   26,   14,   45,   45,   44,   44,   43,   17,
   17,   19,   19,   42,   42,   41,   41,   41,   41,   87,
   87,   89,   15,   16,   16,   88,   88,   90,   35,   61,
   91,   91,   91,   91,   92,   92,   92,   92,   92,   92,
   92,   92,   92,   92,   92,   92,   92,   92,   92,   92,
   92,   92,   92,   92,   92,   93,   94,  114,   95,   96,
  117,   97,   98,  120,   99,  100,  101,  102,  103,  104,
  105,  106,  107,  108,  109,  110,  111,  112,  115,  115,
  125,  113,  113,  126,  118,  118,  127,  127,  116,  116,
  128,  121,  121,  129,  129,  119,  119,  130,  122,  123,
  123,   29,   29,   29,   29,   33,    8,    8,  124,  124,
  132,   36,   36,   30,   30,   31,   31,   31,   27,   27,
   28,   28,   37,   37,    2,    4,    4,    5,    5,    6,
    7,    7,    7,    3,   51,   51,   46,   46,   48,   48,
   32,   32,   32,   32,   49,   49,   23,   23,   34,   34,
  131,  131,
};
static const YYINT yylen[] = {                            2,
    4,    0,    2,    1,    3,    3,    3,    4,    5,    3,
    1,    1,    2,    1,    1,    2,    0,    2,    3,    3,
    2,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    5,    4,    6,    2,    1,    2,    2,
    1,    2,    3,    4,    4,    4,    4,    4,    4,    4,
    6,    2,    4,    2,    4,    4,    4,    2,    0,    1,
    0,    2,    1,    1,    0,    2,    0,    2,    1,    0,
    2,    0,    2,    0,    3,    1,    1,    3,    1,    3,
    1,    1,    2,    1,    0,    2,    3,    1,    3,    2,
    1,    4,    4,    5,    7,    1,    1,    2,    4,    0,
    2,    1,    3,    1,    0,    2,    1,    3,    1,    1,
    3,    1,    1,    1,    2,    1,    3,    3,    5,    1,
    3,    4,    1,    0,    2,    1,    3,    3,    1,    1,
    0,    2,    3,    3,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    2,    3,    0,    4,    2,
    0,    4,    2,    0,    4,    2,    2,    2,    2,    4,
    3,    3,    3,    2,    4,    4,    2,    5,    1,    3,
    1,    1,    3,    1,    1,    3,    3,    3,    1,    3,
    1,    1,    3,    1,    3,    1,    3,    1,    1,    3,
    4,    1,    1,    1,    1,    4,    2,    2,    0,    2,
    3,    0,    1,    1,    2,    1,    1,    2,    0,    2,
    2,    2,    0,    2,    1,    1,    3,    1,    3,    1,
    1,    2,    3,    1,    1,    1,    0,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    3,    1,    2,
    0,    1,
};
static const YYINT yydefred[] = {                         2,
    0,    0,   11,    0,    0,    0,    0,    4,   17,    3,
  250,  249,    0,    0,    0,    0,  141,    0,    6,   10,
    0,    7,    5,    1,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   21,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   16,   18,
    0,   22,   23,   24,   25,   26,   27,   28,   29,   30,
   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
   41,   42,   43,   12,    8,    0,    0,    0,    0,    0,
    0,    0,   14,   15,    0,    0,    0,    0,    0,  142,
    0,    0,  145,    0,  146,  147,  148,  149,  150,  151,
  152,  153,  154,  155,  156,  157,  158,  159,  160,  161,
  162,  163,  164,  165,   20,  133,    0,   94,   92,    0,
    0,    0,  248,    0,    0,    0,    0,    0,   52,    0,
    0,    0,  244,    0,    0,    0,  241,    0,    0,  238,
  240,    0,  130,  139,    0,   62,    0,    0,    0,   47,
   50,   49,   68,   19,   13,    9,  144,  209,    0,  191,
    0,  189,  255,  256,  178,  245,    0,    0,    0,  195,
    0,    0,    0,  202,    0,  184,  166,  225,    0,    0,
    0,  168,  179,  171,  174,    0,    0,    0,  143,    0,
    0,   93,   53,    0,    0,    0,  124,    0,    0,    0,
    0,    0,    0,    0,    0,   71,   70,    0,    0,  136,
  242,    0,  246,    0,    0,    0,    0,    0,   77,    0,
    0,    0,  219,    0,    0,    0,    0,    0,    0,  223,
  183,  226,    0,  229,  181,    0,    0,    0,    0,  182,
  167,    0,  123,  122,    0,  120,  119,  117,    0,  107,
  106,   96,    0,    0,    0,    0,    0,   54,    0,    0,
  125,   55,   58,   56,   65,   66,   60,   81,    0,    0,
    0,    0,   67,    0,  243,    0,    0,  239,  131,   63,
    0,    0,    0,  262,    0,    0,  190,  259,  251,  252,
  253,    0,  198,  254,  197,  196,  205,  203,  228,    0,
  186,  194,    0,  192,  201,    0,  199,  208,    0,  206,
  185,    0,    0,    0,    0,    0,   97,    0,    0,    0,
  100,  114,  112,    0,  128,    0,   83,    0,    0,   73,
   74,   72,   44,  138,  137,  132,   79,   78,    0,  213,
  214,  212,    0,  215,    0,  220,  260,    0,    0,  230,
  188,    0,    0,    0,  135,  121,   61,  118,    0,    0,
   99,  108,    0,    0,    0,   87,    0,   46,    0,   76,
    0,    0,  211,    0,  234,  231,  232,    0,  193,  200,
  207,    0,  102,    0,  113,  129,    0,    0,    0,  218,
  217,  221,    0,    0,    0,  104,    0,   88,   91,   85,
  216,  258,    0,    0,    0,  105,    0,   90,  109,
};
#if defined(YYDESTRUCT_CALL) || defined(YYSTYPE_TOSTRING)
static const YYINT yystos[] = {                           0,
  319,  377,  256,  261,  282,  284,  307,   10,  378,  381,
  313,  314,  367,  315,  315,  367,  379,  382,   10,   10,
  315,   10,   10,  380,  410,  256,  259,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  276,  277,  278,  285,
  286,  287,  288,  294,  295,  300,  301,  310,  317,   10,
  383,  384,  385,  386,  387,  388,  389,  390,  391,  392,
  393,  394,  395,  396,  397,  398,  399,  400,  401,  402,
  403,  404,  405,  315,   10,  366,  256,  265,  279,  281,
  286,  287,  292,  293,  297,  302,  303,  305,  315,   10,
  349,  357,  383,  411,  412,  413,  414,  415,  416,  417,
  418,  419,  420,  421,  422,  423,  424,  425,  426,  427,
  428,  429,  430,  431,   10,  315,  334,  315,  358,  359,
  315,  365,  367,  365,  365,  334,  334,  334,  315,  315,
  367,  123,  315,   33,   40,  321,  322,  323,  324,  325,
  326,  406,  408,  312,  354,  354,  367,  269,  270,  367,
  315,  367,  354,   10,  315,   10,   10,  315,  441,  315,
  434,  444,  314,  315,  368,  314,  315,  370,  437,  446,
  354,  315,  440,  448,  315,  349,  315,   42,  258,  265,
  273,  279,  281,  286,  297,  302,  305,  349,   10,  258,
   58,  358,  364,  123,  337,  315,  360,  361,  361,  361,
  337,  337,  337,  262,  372,  320,  321,  334,  407,  409,
  322,  321,  315,  370,  124,   38,   44,  354,  320,  365,
  365,  304,  442,   44,  299,   61,   44,   61,   44,  354,
  355,  304,  315,  350,  315,  258,  433,  436,  439,  315,
  315,  258,  304,  315,  336,  338,  315,  362,  363,  314,
  315,  125,   91,  339,  340,  369,   58,  344,  263,   61,
  360,  364,  364,  364,  344,  344,  344,  354,  260,  371,
  328,   61,  125,   44,   41,  299,  324,  325,  408,  354,
  330,  361,  361,  296,  450,  443,  444,  312,  314,  315,
  316,   45,  351,  353,  351,  446,  351,  448,   63,  346,
  350,  315,  432,  445,  315,  435,  447,  315,  438,  449,
  350,  311,   44,  335,   44,  369,  125,   44,   61,   91,
  341,  315,  333,  345,  351,  351,  354,  320,  264,  290,
  291,  329,  332,  354,  409,  351,  291,  331,  285,  314,
  315,   63,  348,  352,  275,  451,  312,  280,  315,  347,
  356,   44,   44,   44,  315,  338,  344,  362,   91,  341,
  339,  351,  354,   44,  263,  306,  309,  373,  374,  368,
  312,  308,  327,  450,  354,   63,  342,  351,  445,  447,
  449,  354,   93,   93,  333,  351,   61,   44,  289,  315,
   63,  348,   44,   93,   61,  343,  312,  376,  283,  375,
  312,  342,  343,  123,   58,   93,  342,  312,  125,
};
#endif /* YYDESTRUCT_CALL || YYSTYPE_TOSTRING */
static const YYINT yydgoto[] = {                          1,
  206,  136,  137,  138,  139,  140,  141,  373,  271,  332,
  281,  338,  333,  323,  208,  314,  245,  195,  246,  254,
  255,  321,  377,  396,  258,  324,  300,  350,  343,   91,
  234,  378,  344,  294,  218,  231,  351,   92,  119,  120,
  197,  198,  248,  249,  193,  122,   76,  123,  165,  256,
  168,  270,  205,  368,  369,  400,  398,    2,    9,   17,
   24,   10,   18,   51,   52,   53,   54,   55,   56,   57,
   58,   59,   60,   61,   62,   63,   64,   65,   66,   67,
   68,   69,   70,   71,   72,   73,  142,  209,  143,  210,
   25,   94,   95,   96,   97,   98,   99,  100,  101,  102,
  103,  104,  105,  106,  107,  108,  109,  110,  111,  112,
  113,  114,  303,  237,  161,  306,  238,  169,  309,  239,
  173,  159,  223,  286,  162,  304,  170,  307,  174,  310,
  285,  346,
};
static const YYINT yysindex[] = {                         0,
    0,  118,    0, -217, -286, -271, -217,    0,    0,    0,
    0,    0,   36,   52,    1,   70,    0,   -6,    0,    0,
   15,    0,    0,    0,   53,   74, -229, -212, -202, -217,
 -217, -217, -229, -229, -229, -192, -171,    0, -217,   30,
  -19, -162, -162, -217, -143, -217, -141, -162,    0,    0,
  122,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   21,  147, -155, -154, -180,
  -23, -162,    0,    0, -152, -151, -146, -145,  133,    0,
  -78, -191,    0,  171,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  -72,    0,    0,  -39,
   64, -127,    0, -127, -127,   64,   64,   64,    0,  -71,
  -19, -229,    0, -126,  -19, -173,    0,   66,  154,    0,
    0,  150,    0,    0, -162,    0,  -19, -217, -217,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -109,    0,
  152,    0,    0,    0,    0,    0,    0,  -48,  153,    0,
 -162,  137,  155,    0, -162,    0,    0,    0, -244, -115,
  -57,    0,    0,    0,    0, -113, -112,  -54,    0, -243,
 -110,    0,    0,  -76,  148,  -41,    0,  -30,  -30,  -30,
  148,  148,  148, -162,  -53,    0,    0,  149,  -20,    0,
    0,  167,    0,  -90,  -19,  -19,  -19, -162,    0, -127,
 -127,  -85,    0, -154,   95,   95, -173,   95, -152,    0,
    0,    0,  151,    0,    0, -244, -103, -102,  -98,    0,
    0, -244,    0,    0,  -37,    0,    0,    0,  172,    0,
    0,    0, -168,   93,  175,    8,  -95,    0,   95,   95,
    0,    0,    0,    0,    0,    0,    0,    0, -162,  -19,
 -189, -162,    0, -229,    0,   95,  154,    0,    0,    0,
  -70, -127, -127,    0,   67,  -52,    0,    0,    0,    0,
    0,  -88,    0,    0,    0,    0,    0,    0,    0, -228,
    0,    0,  181,    0,    0,  182,    0,    0,  184,    0,
    0,  -86, -243,  148, -110,    9,    0,  -73,   95, -162,
    0,    0,    0,  187,    0,  -31,    0, -188, -180,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -79,    0,
    0,    0,  -74,    0,  -85,    0,    0, -162,   80,    0,
    0, -103, -102,  -98,    0,    0,    0,    0, -162,  142,
    0,    0,  143,  -95,   95,    0,  176,    0,  196,    0,
  -46,  -28,    0,   67,    0,    0,    0,  200,    0,    0,
    0,  156,    0,  185,    0,    0,  -67,  -36,  -64,    0,
    0,    0,   95,  185,  129,    0,  197,    0,    0,    0,
    0,    0,  161,   95,  -56,    0,  134,    0,    0,
};
static const YYINT yyrindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  258,    0,    0,    0,    0, -110,
 -110, -110,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  259,  263,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   28,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   74,
   20,    0,    0,    0,    0,   20,   20,   20,    0,   82,
   79,    0,    0,    0,    0,    0,    0,   71,   -7,    0,
    0,  280,    0,    0,    0,    0,    6, -110, -110,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  290,    0,    0,    0,    0,    0,   91,    0,  293,    0,
  295,   23,  305,    0,  327,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  334,    0,    0,
    0,    0,    0,    0,  339,   -2,    0,   74,   74,   74,
  339,  339,  339,    0,   58,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  102,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   13,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   27,    0,    0,    0,  341,    0,
    0,    0,    0,    0,  232,  -18,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   -8,
  355,    0,    0,    0,    0,    0,   69,    0,    0,    0,
  356,  361,  362,    0,    0,  366,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  379,
    0,    0,  383,    0,    0,  388,    0,    0,  389,    0,
    0,    0,    0,  339,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  391,    0,   -1,    0,  393,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    2,    0,  102,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  394,    0,
    0,    0,    0,    0,    0,    0,    0,   -5,    0,    0,
    0,    0,    0,  -17,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   29,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
#if YYBTYACC
static const YYINT yycindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
#endif
static const YYINT yygindex[] = {                         0,
 -125,  -99,  274,    0,  198,  199,    0,    0,    0,    0,
    0,    0,    0,   48,  104,    0,    0,   51,  101,  100,
    0,  103, -310,   26, -148,    0,    0,    0,   47,   17,
 -195, -183,    0,    0,  -42,    0,    0,    0,  302,    0,
 -134,  -66,  108,    0,  -15,   19,    0,  112,   97,  174,
  288,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  403,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  212,  157,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  206,   81,  205,   83,  208,   84,
   89,    0,
};
#define YYTABLESIZE 438
static const YYINT yytable[] = {                        145,
  146,   69,  236,   50,  257,  153,  313,  126,  127,  134,
   22,  210,  226,  134,  253,   69,  135,  253,  191,  260,
  135,  219,  227,  274,   75,  101,  103,  191,   14,   95,
  156,  207,  204,  236,  391,  212,  134,  224,   89,  171,
  301,  293,  295,   15,  297,   19,  311,  207,  252,  124,
  125,  348,  265,  266,  267,  126,  127,  199,  200,  232,
  243,   20,   90,  261,  261,  261,  204,   82,  319,  319,
  233,  244,   89,  180,  329,  325,  326,   95,  237,   23,
  235,  181,  402,  115,  134,  116,  349,  182,   69,  183,
   82,   80,  336,  407,  184,   11,   12,   82,  320,  359,
  330,  331,  118,  176,  273,  185,  101,  103,  188,  237,
  186,  235,  121,  187,   80,   13,  236,  366,   16,  257,
  367,   80,  129,   89,  292,  148,  149,    8,  244,  342,
  117,  154,  230,  163,  164,  362,  126,  127,  128,  292,
  166,  213,  376,  130,  328,  250,  251,  261,  261,  144,
  131,  246,  132,  282,  283,  147,  157,  150,  152,  158,
  160,  268,  172,  175,  261,  357,  220,  221,   89,  177,
  207,   11,   12,  151,  178,  280,  201,  202,  203,  179,
  189,  386,  262,  263,  264,  190,  194,  196,  133,  215,
  204,  216,  237,  217,  222,  224,  227,  228,  229,  235,
  236,  240,  241,  242,  247,  257,  269,  275,  276,  272,
  284,  302,  305,  299,  244,  315,  308,  317,  318,  322,
  337,  259,  345,  347,  352,  353,  327,  354,  355,  334,
  364,  365,  371,  372,  383,  384,  387,  250,  251,  388,
  250,  251,  389,  393,  397,  395,  399,  401,  394,   26,
  225,  404,   27,  406,  405,  408,  236,  140,  409,   28,
   29,   30,   31,   32,   33,   34,   35,   36,   48,   37,
   38,   39,   51,  312,  257,  118,  210,  363,   40,   41,
   42,   43,  236,  236,  196,  224,  390,   44,   45,   64,
  166,  167,  227,   46,   47,  133,   69,   69,  236,  170,
   69,  236,  173,   48,  177,  375,  236,  236,   77,  257,
   49,   27,  126,  127,  176,   21,  382,   78,   28,   29,
   30,   31,   32,   33,   34,   35,   36,  227,   37,   74,
   39,   79,  237,   80,  235,  155,  222,   40,   81,   82,
   43,   80,   69,  187,   83,   84,   44,   45,  110,   85,
  116,  339,   46,   47,   86,   87,   98,   88,  237,  237,
  235,  235,   48,   82,   75,   45,   82,   89,   69,   69,
   59,   57,   82,    3,  237,  180,  235,  237,    4,  235,
  340,  341,  237,  237,  235,  235,  261,   80,  233,  246,
   80,  288,  169,  289,  290,  291,   80,  172,  175,    5,
  111,    6,   84,   86,  244,  244,  288,  211,  289,  290,
  291,  385,  277,  356,  278,  261,  261,  361,  360,  403,
  392,  192,  358,  214,    7,  370,  316,   93,  279,  287,
  335,  296,  379,  374,    0,  380,  298,  381,
};
static const YYINT yycheck[] = {                         42,
   43,   10,   10,   10,   10,   48,   44,   10,   10,   33,
   10,   10,   61,   33,   91,   10,   40,   91,   58,   61,
   40,  147,   10,   44,   10,   44,   44,   58,  315,   10,
   10,  131,   10,   41,   63,  135,   10,   10,   10,   82,
  236,  225,  226,  315,  228,   10,  242,  147,  125,   31,
   32,  280,  201,  202,  203,   58,   58,  124,  125,  304,
  304,   10,   10,  198,  199,  200,   44,   10,   61,   61,
  315,  315,   44,  265,  264,  259,  260,   58,   10,   10,
   10,  273,  393,   10,   58,  315,  315,  279,   10,  281,
   33,   10,  276,  404,  286,  313,  314,   40,   91,   91,
  290,  291,  315,   87,  125,  297,  125,  125,   92,   41,
  302,   41,  315,  305,   33,    4,  124,  306,    7,  125,
  309,   40,  315,  315,   45,  269,  270,   10,   38,   63,
   27,   10,  175,  314,  315,  319,   33,   34,   35,   45,
  314,  315,   63,  315,  270,  314,  315,  282,  283,  312,
   39,   61,  123,  220,  221,   44,   10,   46,   47,  315,
  315,  204,  315,  315,   63,  314,  148,  149,  315,  315,
  270,  313,  314,  315,   42,  218,  126,  127,  128,  258,
   10,  365,  198,  199,  200,  258,  123,  315,  315,  124,
  262,   38,  124,   44,  304,   44,   44,   61,   44,  315,
  258,  315,  315,  258,  315,   58,  260,   41,  299,   61,
  296,  315,  315,   63,  124,   44,  315,  125,   44,  315,
  291,  263,  275,  312,   44,   44,  269,   44,  315,  272,
   44,  263,  312,  308,   93,   93,   61,  314,  315,   44,
  314,  315,  289,   44,  312,   61,  283,  312,   93,  256,
  299,  123,  259,   93,   58,  312,  264,    0,  125,  266,
  267,  268,  269,  270,  271,  272,  273,  274,   10,  276,
  277,  278,   10,  311,  280,  315,  275,  320,  285,  286,
  287,  288,  290,  291,  315,  258,  315,  294,  295,   10,
  314,  315,  280,  300,  301,  315,  291,  306,  306,   10,
  309,  309,   10,  310,   10,  348,  314,  315,  256,  315,
  317,  259,  315,  315,   10,  315,  359,  265,  266,  267,
  268,  269,  270,  271,  272,  273,  274,  315,  276,  315,
  278,  279,  264,  281,  264,  315,   10,  285,  286,  287,
  288,  260,  264,   10,  292,  293,  294,  295,   10,  297,
   10,  285,  300,  301,  302,  303,  125,  305,  290,  291,
  290,  291,  310,  306,   10,   10,  309,  315,  290,  291,
   10,   10,  315,  256,  306,   10,  306,  309,  261,  309,
  314,  315,  314,  315,  314,  315,  285,  306,   10,  299,
  309,  312,   10,  314,  315,  316,  315,   10,   10,  282,
   10,  284,   10,   10,  314,  315,  312,  134,  314,  315,
  316,  364,  215,  313,  216,  314,  315,  318,  316,  394,
  374,  120,  315,  136,  307,  329,  253,   25,  217,  224,
  274,  227,  352,  345,   -1,  353,  229,  354,
};
#if YYBTYACC
static const YYINT yyctable[] = {                        -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,
};
#endif
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 317
#define YYUNDFTOKEN 452
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const yyname[] = {

"$end",0,0,0,0,0,0,0,0,0,"'\\n'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'!'",0,0,0,0,"'&'",0,"'('","')'","'*'",0,"','","'-'",0,0,0,0,0,0,0,0,0,0,0,0,
"':'",0,0,"'='",0,"'?'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'['",0,"']'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",
"'|'","'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"error","AND","AT","ATTACH","BLOCK","BUILD",
"CHAR","COLONEQ","COMPILE_WITH","CONFIG","DEFFS","DEFINE","DEFOPT","DEFPARAM",
"DEFFLAG","DEFPSEUDO","DEFPSEUDODEV","DEVICE","DEVCLASS","DUMPS","DEVICE_MAJOR",
"ENDFILE","XFILE","FILE_SYSTEM","FLAGS","IDENT","IOCONF","LINKZERO","XMACHINE",
"MAJOR","MAKEOPTIONS","MAXUSERS","MAXPARTITIONS","MINOR","NEEDS_COUNT",
"NEEDS_FLAG","NO","CNO","XOBJECT","OBSOLETE","ON","OPTIONS","PACKAGE","PLUSEQ",
"PREFIX","BUILDPREFIX","PSEUDO_DEVICE","PSEUDO_ROOT","ROOT","SELECT","SINGLE",
"SOURCE","TYPE","VECTOR","VERSION","WITH","NUMBER","PATHNAME","QSTRING","WORD",
"EMPTYSTRING","ENDDEFS","$accept","configuration","fopts","condexpr","condatom",
"cond_or_expr","cond_and_expr","cond_prefix_expr","cond_base_expr","fs_spec",
"fflags","fflag","oflags","oflag","rule","depend","devbase","devattach_opt",
"atlist","interface_opt","atname","loclist","locdef","locdefault","values",
"locdefaults","depend_list","depends","locators","locator","dev_spec",
"device_instance","attachment","value","major_minor","signed_number","int32",
"npseudo","device_flags","no","deffs","deffses","defopt","defopts","optdepend",
"optdepends","optdepend_list","optfile_opt","subarches","filename",
"stringvalue","locname","mkvarname","device_major_block","device_major_char",
"devnodes","devnodetype","devnodeflags","devnode_dims","topthings",
"machine_spec","definition_part","selection_part","topthing","definitions",
"definition","define_file","define_object","define_device_major",
"define_prefix","define_buildprefix","define_devclass","define_filesystems",
"define_attribute","define_option","define_flag","define_obsolete_flag",
"define_param","define_obsolete_param","define_device",
"define_device_attachment","define_maxpartitions","define_maxusers",
"define_makeoptions","define_pseudo","define_pseudodev","define_major",
"define_version","condmkopt_list","majorlist","condmkoption","majordef",
"selections","selection","select_attr","select_no_attr","select_no_filesystems",
"select_filesystems","select_no_makeoptions","select_makeoptions",
"select_no_options","select_options","select_maxusers","select_ident",
"select_no_ident","select_config","select_no_config","select_no_pseudodev",
"select_pseudodev","select_pseudoroot","select_no_device_instance_attachment",
"select_no_device_attachment","select_no_device_instance",
"select_device_instance","no_fs_list","$$1","fs_list","no_mkopt_list","$$2",
"mkopt_list","no_opt_list","$$3","opt_list","conf","root_spec","sysparam_list",
"fsoption","no_fsoption","mkoption","no_mkoption","option","no_option","on_opt",
"sysparam","illegal-symbol",
};
static const char *const yyrule[] = {
"$accept : configuration",
"configuration : topthings machine_spec definition_part selection_part",
"topthings :",
"topthings : topthings topthing",
"topthing : '\\n'",
"topthing : SOURCE filename '\\n'",
"topthing : BUILD filename '\\n'",
"machine_spec : XMACHINE WORD '\\n'",
"machine_spec : XMACHINE WORD WORD '\\n'",
"machine_spec : XMACHINE WORD WORD subarches '\\n'",
"machine_spec : IOCONF WORD '\\n'",
"machine_spec : error",
"subarches : WORD",
"subarches : subarches WORD",
"no : NO",
"no : CNO",
"definition_part : definitions ENDDEFS",
"definitions :",
"definitions : definitions '\\n'",
"definitions : definitions definition '\\n'",
"definitions : definitions error '\\n'",
"definitions : definitions ENDFILE",
"definition : define_file",
"definition : define_object",
"definition : define_device_major",
"definition : define_prefix",
"definition : define_buildprefix",
"definition : define_devclass",
"definition : define_filesystems",
"definition : define_attribute",
"definition : define_option",
"definition : define_flag",
"definition : define_obsolete_flag",
"definition : define_param",
"definition : define_obsolete_param",
"definition : define_device",
"definition : define_device_attachment",
"definition : define_maxpartitions",
"definition : define_maxusers",
"definition : define_makeoptions",
"definition : define_pseudo",
"definition : define_pseudodev",
"definition : define_major",
"definition : define_version",
"define_file : XFILE filename fopts fflags rule",
"define_object : XOBJECT filename fopts oflags",
"define_device_major : DEVICE_MAJOR WORD device_major_char device_major_block fopts devnodes",
"define_prefix : PREFIX filename",
"define_prefix : PREFIX",
"define_buildprefix : BUILDPREFIX filename",
"define_buildprefix : BUILDPREFIX WORD",
"define_buildprefix : BUILDPREFIX",
"define_devclass : DEVCLASS WORD",
"define_filesystems : DEFFS deffses optdepend_list",
"define_attribute : DEFINE WORD interface_opt depend_list",
"define_option : DEFOPT optfile_opt defopts optdepend_list",
"define_flag : DEFFLAG optfile_opt defopts optdepend_list",
"define_obsolete_flag : OBSOLETE DEFFLAG optfile_opt defopts",
"define_param : DEFPARAM optfile_opt defopts optdepend_list",
"define_obsolete_param : OBSOLETE DEFPARAM optfile_opt defopts",
"define_device : DEVICE devbase interface_opt depend_list",
"define_device_attachment : ATTACH devbase AT atlist devattach_opt depend_list",
"define_maxpartitions : MAXPARTITIONS int32",
"define_maxusers : MAXUSERS int32 int32 int32",
"define_makeoptions : MAKEOPTIONS condmkopt_list",
"define_pseudo : DEFPSEUDO devbase interface_opt depend_list",
"define_pseudodev : DEFPSEUDODEV devbase interface_opt depend_list",
"define_major : MAJOR '{' majorlist '}'",
"define_version : VERSION int32",
"fopts :",
"fopts : condexpr",
"fflags :",
"fflags : fflags fflag",
"fflag : NEEDS_COUNT",
"fflag : NEEDS_FLAG",
"rule :",
"rule : COMPILE_WITH stringvalue",
"oflags :",
"oflags : oflags oflag",
"oflag : NEEDS_FLAG",
"device_major_char :",
"device_major_char : CHAR int32",
"device_major_block :",
"device_major_block : BLOCK int32",
"devnodes :",
"devnodes : devnodetype ',' devnodeflags",
"devnodes : devnodetype",
"devnodetype : SINGLE",
"devnodetype : VECTOR '=' devnode_dims",
"devnode_dims : NUMBER",
"devnode_dims : NUMBER ':' NUMBER",
"devnodeflags : LINKZERO",
"deffses : deffs",
"deffses : deffses deffs",
"deffs : WORD",
"interface_opt :",
"interface_opt : '{' '}'",
"interface_opt : '{' loclist '}'",
"loclist : locdef",
"loclist : locdef ',' loclist",
"locdef : locname locdefault",
"locdef : locname",
"locdef : '[' locname locdefault ']'",
"locdef : locname '[' int32 ']'",
"locdef : locname '[' int32 ']' locdefaults",
"locdef : '[' locname '[' int32 ']' locdefaults ']'",
"locname : WORD",
"locname : QSTRING",
"locdefault : '=' value",
"locdefaults : '=' '{' values '}'",
"depend_list :",
"depend_list : ':' depends",
"depends : depend",
"depends : depends ',' depend",
"depend : WORD",
"optdepend_list :",
"optdepend_list : ':' optdepends",
"optdepends : optdepend",
"optdepends : optdepends ',' optdepend",
"optdepend : WORD",
"atlist : atname",
"atlist : atlist ',' atname",
"atname : WORD",
"atname : ROOT",
"defopts : defopt",
"defopts : defopts defopt",
"defopt : WORD",
"defopt : WORD '=' value",
"defopt : WORD COLONEQ value",
"defopt : WORD '=' value COLONEQ value",
"condmkopt_list : condmkoption",
"condmkopt_list : condmkopt_list ',' condmkoption",
"condmkoption : condexpr mkvarname PLUSEQ value",
"devbase : WORD",
"devattach_opt :",
"devattach_opt : WITH WORD",
"majorlist : majordef",
"majorlist : majorlist ',' majordef",
"majordef : devbase '=' int32",
"int32 : NUMBER",
"selection_part : selections",
"selections :",
"selections : selections '\\n'",
"selections : selections selection '\\n'",
"selections : selections error '\\n'",
"selection : definition",
"selection : select_attr",
"selection : select_no_attr",
"selection : select_no_filesystems",
"selection : select_filesystems",
"selection : select_no_makeoptions",
"selection : select_makeoptions",
"selection : select_no_options",
"selection : select_options",
"selection : select_maxusers",
"selection : select_ident",
"selection : select_no_ident",
"selection : select_config",
"selection : select_no_config",
"selection : select_no_pseudodev",
"selection : select_pseudodev",
"selection : select_pseudoroot",
"selection : select_no_device_instance_attachment",
"selection : select_no_device_attachment",
"selection : select_no_device_instance",
"selection : select_device_instance",
"select_attr : SELECT WORD",
"select_no_attr : no SELECT WORD",
"$$1 :",
"select_no_filesystems : no FILE_SYSTEM $$1 no_fs_list",
"select_filesystems : FILE_SYSTEM fs_list",
"$$2 :",
"select_no_makeoptions : no MAKEOPTIONS $$2 no_mkopt_list",
"select_makeoptions : MAKEOPTIONS mkopt_list",
"$$3 :",
"select_no_options : no OPTIONS $$3 no_opt_list",
"select_options : OPTIONS opt_list",
"select_maxusers : MAXUSERS int32",
"select_ident : IDENT stringvalue",
"select_no_ident : no IDENT",
"select_config : CONFIG conf root_spec sysparam_list",
"select_no_config : no CONFIG WORD",
"select_no_pseudodev : no PSEUDO_DEVICE WORD",
"select_pseudodev : PSEUDO_DEVICE WORD npseudo",
"select_pseudoroot : PSEUDO_ROOT device_instance",
"select_no_device_instance_attachment : no device_instance AT attachment",
"select_no_device_attachment : no DEVICE AT attachment",
"select_no_device_instance : no device_instance",
"select_device_instance : device_instance AT attachment locators device_flags",
"fs_list : fsoption",
"fs_list : fs_list ',' fsoption",
"fsoption : WORD",
"no_fs_list : no_fsoption",
"no_fs_list : no_fs_list ',' no_fsoption",
"no_fsoption : WORD",
"mkopt_list : mkoption",
"mkopt_list : mkopt_list ',' mkoption",
"mkoption : mkvarname '=' value",
"mkoption : mkvarname PLUSEQ value",
"no_mkopt_list : no_mkoption",
"no_mkopt_list : no_mkopt_list ',' no_mkoption",
"no_mkoption : WORD",
"opt_list : option",
"opt_list : opt_list ',' option",
"option : WORD",
"option : WORD '=' value",
"no_opt_list : no_option",
"no_opt_list : no_opt_list ',' no_option",
"no_option : WORD",
"conf : WORD",
"root_spec : ROOT on_opt dev_spec",
"root_spec : ROOT on_opt dev_spec fs_spec",
"dev_spec : '?'",
"dev_spec : QSTRING",
"dev_spec : WORD",
"dev_spec : major_minor",
"major_minor : MAJOR NUMBER MINOR NUMBER",
"fs_spec : TYPE '?'",
"fs_spec : TYPE WORD",
"sysparam_list :",
"sysparam_list : sysparam_list sysparam",
"sysparam : DUMPS on_opt dev_spec",
"npseudo :",
"npseudo : int32",
"device_instance : WORD",
"device_instance : WORD '*'",
"attachment : ROOT",
"attachment : WORD",
"attachment : WORD '?'",
"locators :",
"locators : locators locator",
"locator : WORD '?'",
"locator : WORD values",
"device_flags :",
"device_flags : FLAGS int32",
"condexpr : cond_or_expr",
"cond_or_expr : cond_and_expr",
"cond_or_expr : cond_or_expr '|' cond_and_expr",
"cond_and_expr : cond_prefix_expr",
"cond_and_expr : cond_and_expr '&' cond_prefix_expr",
"cond_prefix_expr : cond_base_expr",
"cond_base_expr : condatom",
"cond_base_expr : '!' condatom",
"cond_base_expr : '(' condexpr ')'",
"condatom : WORD",
"mkvarname : QSTRING",
"mkvarname : WORD",
"optfile_opt :",
"optfile_opt : filename",
"filename : QSTRING",
"filename : PATHNAME",
"value : QSTRING",
"value : WORD",
"value : EMPTYSTRING",
"value : signed_number",
"stringvalue : QSTRING",
"stringvalue : WORD",
"values : value",
"values : value ',' values",
"signed_number : NUMBER",
"signed_number : '-' NUMBER",
"on_opt :",
"on_opt : ON",

};
#endif

#if YYDEBUG
int      yydebug;
#endif

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;
int      yynerrs;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
YYLTYPE  yyloc; /* position returned by actions */
YYLTYPE  yylloc; /* position from the lexer */
#endif

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
#ifndef YYLLOC_DEFAULT
#define YYLLOC_DEFAULT(loc, rhs, n) \
do \
{ \
    if (n == 0) \
    { \
        (loc).first_line   = YYRHSLOC(rhs, 0).last_line; \
        (loc).first_column = YYRHSLOC(rhs, 0).last_column; \
        (loc).last_line    = YYRHSLOC(rhs, 0).last_line; \
        (loc).last_column  = YYRHSLOC(rhs, 0).last_column; \
    } \
    else \
    { \
        (loc).first_line   = YYRHSLOC(rhs, 1).first_line; \
        (loc).first_column = YYRHSLOC(rhs, 1).first_column; \
        (loc).last_line    = YYRHSLOC(rhs, n).last_line; \
        (loc).last_column  = YYRHSLOC(rhs, n).last_column; \
    } \
} while (0)
#endif /* YYLLOC_DEFAULT */
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#if YYBTYACC

#ifndef YYLVQUEUEGROWTH
#define YYLVQUEUEGROWTH 32
#endif
#endif /* YYBTYACC */

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  10000
#endif
#endif

#ifndef YYINITSTACKSIZE
#define YYINITSTACKSIZE 200
#endif

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  *p_base;
    YYLTYPE  *p_mark;
#endif
} YYSTACKDATA;
#if YYBTYACC

struct YYParseState_s
{
    struct YYParseState_s *save;    /* Previously saved parser state */
    YYSTACKDATA            yystack; /* saved parser stack */
    int                    state;   /* saved parser state */
    int                    errflag; /* saved error recovery status */
    int                    lexeme;  /* saved index of the conflict lexeme in the lexical queue */
    YYINT                  ctry;    /* saved index in yyctable[] for this conflict */
};
typedef struct YYParseState_s YYParseState;
#endif /* YYBTYACC */
/* variables for the parser stack */
static YYSTACKDATA yystack;
#if YYBTYACC

/* Current parser state */
static YYParseState *yyps = 0;

/* yypath != NULL: do the full parse, starting at *yypath parser state. */
static YYParseState *yypath = 0;

/* Base of the lexical value queue */
static YYSTYPE *yylvals = 0;

/* Current position at lexical value queue */
static YYSTYPE *yylvp = 0;

/* End position of lexical value queue */
static YYSTYPE *yylve = 0;

/* The last allocated position at the lexical value queue */
static YYSTYPE *yylvlim = 0;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
/* Base of the lexical position queue */
static YYLTYPE *yylpsns = 0;

/* Current position at lexical position queue */
static YYLTYPE *yylpp = 0;

/* End position of lexical position queue */
static YYLTYPE *yylpe = 0;

/* The last allocated position at the lexical position queue */
static YYLTYPE *yylplim = 0;
#endif

/* Current position at lexical token queue */
static YYINT  *yylexp = 0;

static YYINT  *yylexemes = 0;
#endif /* YYBTYACC */
#line 1099 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"

void
yyerror(const char *s)
{

	cfgerror("%s", s);
}

/************************************************************/

/*
 * Wrap allocations that live on the parser stack so that we can free
 * them again on error instead of leaking.
 */

#define MAX_WRAP 1000

struct wrap_entry {
	void *ptr;
	unsigned typecode;
};

static struct wrap_entry wrapstack[MAX_WRAP];
static unsigned wrap_depth;

/*
 * Remember pointer PTR with type-code CODE.
 */
static void
wrap_alloc(void *ptr, unsigned code)
{
	unsigned pos;

	if (wrap_depth >= MAX_WRAP) {
		panic("allocation wrapper stack overflow");
	}
	pos = wrap_depth++;
	wrapstack[pos].ptr = ptr;
	wrapstack[pos].typecode = code;
}

/*
 * We succeeded; commit to keeping everything that's been allocated so
 * far and clear the stack.
 */
static void
wrap_continue(void)
{
	wrap_depth = 0;
}

/*
 * We failed; destroy all the objects allocated.
 */
static void
wrap_cleanup(void)
{
	unsigned i;

	/*
	 * Destroy each item. Note that because everything allocated
	 * is entered on the list separately, lists and trees need to
	 * have their links blanked before being destroyed. Also note
	 * that strings are interned elsewhere and not handled by this
	 * mechanism.
	 */

	for (i=0; i<wrap_depth; i++) {
		switch (wrapstack[i].typecode) {
		    case WRAP_CODE_nvlist:
			nvfree(wrapstack[i].ptr);
			break;
		    case WRAP_CODE_defoptlist:
			{
				struct defoptlist *dl = wrapstack[i].ptr;

				dl->dl_next = NULL;
				defoptlist_destroy(dl);
			}
			break;
		    case WRAP_CODE_loclist:
			{
				struct loclist *ll = wrapstack[i].ptr;

				ll->ll_next = NULL;
				loclist_destroy(ll);
			}
			break;
		    case WRAP_CODE_attrlist:
			{
				struct attrlist *al = wrapstack[i].ptr;

				al->al_next = NULL;
				al->al_this = NULL;
				attrlist_destroy(al);
			}
			break;
		    case WRAP_CODE_condexpr:
			{
				struct condexpr *cx = wrapstack[i].ptr;

				cx->cx_type = CX_ATOM;
				cx->cx_atom = NULL;
				condexpr_destroy(cx);
			}
			break;
		    default:
			panic("invalid code %u on allocation wrapper stack",
			      wrapstack[i].typecode);
		}
	}

	wrap_depth = 0;
}

/*
 * Instantiate the wrapper functions.
 *
 * Each one calls wrap_alloc to save the pointer and then returns the
 * pointer again; these need to be generated with the preprocessor in
 * order to be typesafe.
 */
#define DEF_ALLOCWRAP(t) \
	static struct t *				\
	wrap_mk_##t(struct t *arg)			\
	{						\
		wrap_alloc(arg, WRAP_CODE_##t);		\
		return arg;				\
	}

DEF_ALLOCWRAP(nvlist);
DEF_ALLOCWRAP(defoptlist);
DEF_ALLOCWRAP(loclist);
DEF_ALLOCWRAP(attrlist);
DEF_ALLOCWRAP(condexpr);

/************************************************************/

/*
 * Data constructors
 *
 * (These are *beneath* the allocation wrappers.)
 */

static struct defoptlist *
mk_defoptlist(const char *name, const char *val, const char *lintval)
{
	return defoptlist_create(name, val, lintval);
}

static struct loclist *
mk_loc(const char *name, const char *str, long long num)
{
	return loclist_create(name, str, num);
}

static struct loclist *
mk_loc_val(const char *str, struct loclist *next)
{
	struct loclist *ll;

	ll = mk_loc(NULL, str, 0);
	ll->ll_next = next;
	return ll;
}

static struct attrlist *
mk_attrlist(struct attrlist *next, struct attr *a)
{
	return attrlist_cons(next, a);
}

static struct condexpr *
mk_cx_atom(const char *s)
{
	struct condexpr *cx;

	cx = condexpr_create(CX_ATOM);
	cx->cx_atom = s;
	return cx;
}

static struct condexpr *
mk_cx_not(struct condexpr *sub)
{
	struct condexpr *cx;

	cx = condexpr_create(CX_NOT);
	cx->cx_not = sub;
	return cx;
}

static struct condexpr *
mk_cx_and(struct condexpr *left, struct condexpr *right)
{
	struct condexpr *cx;

	cx = condexpr_create(CX_AND);
	cx->cx_and.left = left;
	cx->cx_and.right = right;
	return cx;
}

static struct condexpr *
mk_cx_or(struct condexpr *left, struct condexpr *right)
{
	struct condexpr *cx;

	cx = condexpr_create(CX_OR);
	cx->cx_or.left = left;
	cx->cx_or.right = right;
	return cx;
}

/************************************************************/

static void
setmachine(const char *mch, const char *mcharch, struct nvlist *mchsubarches,
	int isioconf)
{
	char buf[MAXPATHLEN];
	struct nvlist *nv;

	if (isioconf) {
		if (include(_PATH_DEVNULL, ENDDEFS, 0, 0) != 0)
			exit(1);
		ioconfname = mch;
		return;
	}

	machine = mch;
	machinearch = mcharch;
	machinesubarches = mchsubarches;

	/*
	 * Define attributes for all the given names
	 */
	if (defattr(machine, NULL, NULL, 0) != 0 ||
	    (machinearch != NULL &&
	     defattr(machinearch, NULL, NULL, 0) != 0))
		exit(1);
	for (nv = machinesubarches; nv != NULL; nv = nv->nv_next) {
		if (defattr(nv->nv_name, NULL, NULL, 0) != 0)
			exit(1);
	}

	/*
	 * Set up the file inclusion stack.  This empty include tells
	 * the parser there are no more device definitions coming.
	 */
	if (include(_PATH_DEVNULL, ENDDEFS, 0, 0) != 0)
		exit(1);

	/* Include arch/${MACHINE}/conf/files.${MACHINE} */
	(void)snprintf(buf, sizeof(buf), "arch/%s/conf/files.%s",
	    machine, machine);
	if (include(buf, ENDFILE, 0, 0) != 0)
		exit(1);

	/* Include any arch/${MACHINE_SUBARCH}/conf/files.${MACHINE_SUBARCH} */
	for (nv = machinesubarches; nv != NULL; nv = nv->nv_next) {
		(void)snprintf(buf, sizeof(buf), "arch/%s/conf/files.%s",
		    nv->nv_name, nv->nv_name);
		if (include(buf, ENDFILE, 0, 0) != 0)
			exit(1);
	}

	/* Include any arch/${MACHINE_ARCH}/conf/files.${MACHINE_ARCH} */
	if (machinearch != NULL)
		(void)snprintf(buf, sizeof(buf), "arch/%s/conf/files.%s",
		    machinearch, machinearch);
	else
		strlcpy(buf, _PATH_DEVNULL, sizeof(buf));
	if (include(buf, ENDFILE, 0, 0) != 0)
		exit(1);

	/*
	 * Include the global conf/files.  As the last thing
	 * pushed on the stack, it will be processed first.
	 */
	if (include("conf/files", ENDFILE, 0, 0) != 0)
		exit(1);

	oktopackage = 1;
}

static void
check_maxpart(void)
{

	if (maxpartitions <= 0 && ioconfname == NULL) {
		stop("cannot proceed without maxpartitions specifier");
	}
}

static void
check_version(void)
{
	/*
	 * In essence, version is 0 and is not supported anymore
	 */
	if (version < CONFIG_MINVERSION)
		stop("your sources are out of date -- please update.");
}

/*
 * Prepend a blank entry to the locator definitions so the code in
 * sem.c can distinguish "empty locator list" from "no locator list".
 * XXX gross.
 */
static struct loclist *
present_loclist(struct loclist *ll)
{
	struct loclist *ret;

	ret = MK3(loc, "", NULL, 0);
	ret->ll_next = ll;
	return ret;
}

static void
app(struct loclist *p, struct loclist *q)
{
	while (p->ll_next)
		p = p->ll_next;
	p->ll_next = q;
}

static struct loclist *
locarray(const char *name, int count, struct loclist *adefs, int opt)
{
	struct loclist *defs = adefs;
	struct loclist **p;
	char buf[200];
	int i;

	if (count <= 0) {
		fprintf(stderr, "config: array with <= 0 size: %s\n", name);
		exit(1);
	}
	p = &defs;
	for(i = 0; i < count; i++) {
		if (*p == NULL)
			*p = MK3(loc, NULL, "0", 0);
		snprintf(buf, sizeof(buf), "%s%c%d", name, ARRCHR, i);
		(*p)->ll_name = i == 0 ? name : intern(buf);
		(*p)->ll_num = i > 0 || opt;
		p = &(*p)->ll_next;
	}
	*p = 0;
	return defs;
}


static struct loclist *
namelocvals(const char *name, struct loclist *vals)
{
	struct loclist *p;
	char buf[200];
	int i;

	for (i = 0, p = vals; p; i++, p = p->ll_next) {
		snprintf(buf, sizeof(buf), "%s%c%d", name, ARRCHR, i);
		p->ll_name = i == 0 ? name : intern(buf);
	}
	return vals;
}

#line 1567 "gram.c"

/* For use in generated program */
#define yydepth (int)(yystack.s_mark - yystack.s_base)
#if YYBTYACC
#define yytrial (yyps->save)
#endif /* YYBTYACC */

#if YYDEBUG
#include <stdio.h>	/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    YYINT *newss;
    YYSTYPE *newvs;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE *newps;
#endif

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return YYENOMEM;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (YYINT *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return YYENOMEM;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return YYENOMEM;

    data->l_base = newvs;
    data->l_mark = newvs + i;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    newps = (YYLTYPE *)realloc(data->p_base, newsize * sizeof(*newps));
    if (newps == 0)
        return YYENOMEM;

    data->p_base = newps;
    data->p_mark = newps + i;
#endif

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;

#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%sdebug: stack size increased to %d\n", YYPREFIX, newsize);
#endif
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    free(data->p_base);
#endif
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif /* YYPURE || defined(YY_NO_LEAKS) */
#if YYBTYACC

static YYParseState *
yyNewState(unsigned size)
{
    YYParseState *p = (YYParseState *) malloc(sizeof(YYParseState));
    if (p == NULL) return NULL;

    p->yystack.stacksize = size;
    if (size == 0)
    {
        p->yystack.s_base = NULL;
        p->yystack.l_base = NULL;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        p->yystack.p_base = NULL;
#endif
        return p;
    }
    p->yystack.s_base    = (YYINT *) malloc(size * sizeof(YYINT));
    if (p->yystack.s_base == NULL) return NULL;
    p->yystack.l_base    = (YYSTYPE *) malloc(size * sizeof(YYSTYPE));
    if (p->yystack.l_base == NULL) return NULL;
    memset(p->yystack.l_base, 0, size * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    p->yystack.p_base    = (YYLTYPE *) malloc(size * sizeof(YYLTYPE));
    if (p->yystack.p_base == NULL) return NULL;
    memset(p->yystack.p_base, 0, size * sizeof(YYLTYPE));
#endif

    return p;
}

static void
yyFreeState(YYParseState *p)
{
    yyfreestack(&p->yystack);
    free(p);
}
#endif /* YYBTYACC */

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab
#if YYBTYACC
#define YYVALID        do { if (yyps->save)            goto yyvalid; } while(0)
#define YYVALID_NESTED do { if (yyps->save && \
                                yyps->save->save == 0) goto yyvalid; } while(0)
#endif /* YYBTYACC */

int
YYPARSE_DECL()
{
    int yym, yyn, yystate, yyresult;
#if YYBTYACC
    int yynewerrflag;
    YYParseState *yyerrctx = NULL;
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  yyerror_loc_range[3]; /* position of error start/end (0 unused) */
#endif
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
    if (yydebug)
        fprintf(stderr, "%sdebug[<# of symbols on state stack>]\n", YYPREFIX);
#endif
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    memset(yyerror_loc_range, 0, sizeof(yyerror_loc_range));
#endif

#if YYBTYACC
    yyps = yyNewState(0); if (yyps == 0) goto yyenomem;
    yyps->save = 0;
#endif /* YYBTYACC */
    yym = 0;
    yyn = 0;
    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base;
#endif
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
#if YYBTYACC
        do {
        if (yylvp < yylve)
        {
            /* we're currently re-reading tokens */
            yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc = *yylpp++;
#endif
            yychar = *yylexp++;
            break;
        }
        if (yyps->save)
        {
            /* in trial mode; save scanner results for future parse attempts */
            if (yylvp == yylvlim)
            {   /* Enlarge lexical value queue */
                size_t p = (size_t) (yylvp - yylvals);
                size_t s = (size_t) (yylvlim - yylvals);

                s += YYLVQUEUEGROWTH;
                if ((yylexemes = (YYINT *)realloc(yylexemes, s * sizeof(YYINT))) == NULL) goto yyenomem;
                if ((yylvals   = (YYSTYPE *)realloc(yylvals, s * sizeof(YYSTYPE))) == NULL) goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                if ((yylpsns   = (YYLTYPE *)realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL) goto yyenomem;
#endif
                yylvp   = yylve = yylvals + p;
                yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp   = yylpe = yylpsns + p;
                yylplim = yylpsns + s;
#endif
                yylexp  = yylexemes + p;
            }
            *yylexp = (YYINT) YYLEX;
            *yylvp++ = yylval;
            yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *yylpp++ = yylloc;
            yylpe++;
#endif
            yychar = *yylexp++;
            break;
        }
        /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
        yychar = YYLEX;
#if YYBTYACC
        } while (0);
#endif /* YYBTYACC */
        if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, " <%s>", YYSTYPE_TOSTRING(yychar, yylval));
#endif
            fputc('\n', stderr);
        }
#endif
    }
#if YYBTYACC

    /* Do we have a conflict? */
    if (((yyn = yycindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
        yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        YYINT ctry;

        if (yypath)
        {
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: CONFLICT in state %d: following successful trial parse\n",
                                YYDEBUGSTR, yydepth, yystate);
#endif
            /* Switch to the next conflict context */
            save = yypath;
            yypath = save->save;
            save->save = NULL;
            ctry = save->ctry;
            if (save->state != yystate) YYABORT;
            yyFreeState(save);

        }
        else
        {

            /* Unresolved conflict - start/continue trial parse */
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
            {
                fprintf(stderr, "%s[%d]: CONFLICT in state %d. ", YYDEBUGSTR, yydepth, yystate);
                if (yyps->save)
                    fputs("ALREADY in conflict, continuing trial parse.\n", stderr);
                else
                    fputs("Starting trial parse.\n", stderr);
            }
#endif
            save                  = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (save == NULL) goto yyenomem;
            save->save            = yyps->save;
            save->state           = yystate;
            save->errflag         = yyerrflag;
            save->yystack.s_mark  = save->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (save->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            save->yystack.l_mark  = save->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (save->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            save->yystack.p_mark  = save->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (save->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            ctry                  = yytable[yyn];
            if (yyctable[ctry] == -1)
            {
#if YYDEBUG
                if (yydebug && yychar >= YYEOF)
                    fprintf(stderr, "%s[%d]: backtracking 1 token\n", YYDEBUGSTR, yydepth);
#endif
                ctry++;
            }
            save->ctry = ctry;
            if (yyps->save == NULL)
            {
                /* If this is a first conflict in the stack, start saving lexemes */
                if (!yylexemes)
                {
                    yylexemes = (YYINT *) malloc((YYLVQUEUEGROWTH) * sizeof(YYINT));
                    if (yylexemes == NULL) goto yyenomem;
                    yylvals   = (YYSTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYSTYPE));
                    if (yylvals == NULL) goto yyenomem;
                    yylvlim   = yylvals + YYLVQUEUEGROWTH;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpsns   = (YYLTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYLTYPE));
                    if (yylpsns == NULL) goto yyenomem;
                    yylplim   = yylpsns + YYLVQUEUEGROWTH;
#endif
                }
                if (yylvp == yylve)
                {
                    yylvp  = yylve = yylvals;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp  = yylpe = yylpsns;
#endif
                    yylexp = yylexemes;
                    if (yychar >= YYEOF)
                    {
                        *yylve++ = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                        *yylpe++ = yylloc;
#endif
                        *yylexp  = (YYINT) yychar;
                        yychar   = YYEMPTY;
                    }
                }
            }
            if (yychar >= YYEOF)
            {
                yylvp--;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp--;
#endif
                yylexp--;
                yychar = YYEMPTY;
            }
            save->lexeme = (int) (yylvp - yylvals);
            yyps->save   = save;
        }
        if (yytable[yyn] == ctry)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                                YYDEBUGSTR, yydepth, yystate, yyctable[ctry]);
#endif
            if (yychar < 0)
            {
                yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp++;
#endif
                yylexp++;
            }
            if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
                goto yyoverflow;
            yystate = yyctable[ctry];
            *++yystack.s_mark = (YYINT) yystate;
            *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *++yystack.p_mark = yylloc;
#endif
            yychar  = YYEMPTY;
            if (yyerrflag > 0) --yyerrflag;
            goto yyloop;
        }
        else
        {
            yyn = yyctable[ctry];
            goto yyreduce;
        }
    } /* End of code dealing with conflicts */
#endif /* YYBTYACC */
    if (((yyn = yysindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                            YYDEBUGSTR, yydepth, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yylloc;
#endif
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if (((yyn = yyrindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag != 0) goto yyinrecovery;
#if YYBTYACC

    yynewerrflag = 1;
    goto yyerrhandler;
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */

yyerrlab:
    /* explicit YYERROR from an action -- pop the rhs of the rule reduced
     * before looking for error recovery */
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif

    yynewerrflag = 0;
yyerrhandler:
    while (yyps->save)
    {
        int ctry;
        YYParseState *save = yyps->save;
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: ERROR in state %d, CONFLICT BACKTRACKING to state %d, %d tokens\n",
                            YYDEBUGSTR, yydepth, yystate, yyps->save->state,
                    (int)(yylvp - yylvals - yyps->save->lexeme));
#endif
        /* Memorize most forward-looking error state in case it's really an error. */
        if (yyerrctx == NULL || yyerrctx->lexeme < yylvp - yylvals)
        {
            /* Free old saved error context state */
            if (yyerrctx) yyFreeState(yyerrctx);
            /* Create and fill out new saved error context state */
            yyerrctx                 = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (yyerrctx == NULL) goto yyenomem;
            yyerrctx->save           = yyps->save;
            yyerrctx->state          = yystate;
            yyerrctx->errflag        = yyerrflag;
            yyerrctx->yystack.s_mark = yyerrctx->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (yyerrctx->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yyerrctx->yystack.l_mark = yyerrctx->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (yyerrctx->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yyerrctx->yystack.p_mark = yyerrctx->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (yyerrctx->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yyerrctx->lexeme         = (int) (yylvp - yylvals);
        }
        yylvp          = yylvals   + save->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yylpp          = yylpsns   + save->lexeme;
#endif
        yylexp         = yylexemes + save->lexeme;
        yychar         = YYEMPTY;
        yystack.s_mark = yystack.s_base + (save->yystack.s_mark - save->yystack.s_base);
        memcpy (yystack.s_base, save->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
        yystack.l_mark = yystack.l_base + (save->yystack.l_mark - save->yystack.l_base);
        memcpy (yystack.l_base, save->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yystack.p_mark = yystack.p_base + (save->yystack.p_mark - save->yystack.p_base);
        memcpy (yystack.p_base, save->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
        ctry           = ++save->ctry;
        yystate        = save->state;
        /* We tried shift, try reduce now */
        if ((yyn = yyctable[ctry]) >= 0) goto yyreduce;
        yyps->save     = save->save;
        save->save     = NULL;
        yyFreeState(save);

        /* Nothing left on the stack -- error */
        if (!yyps->save)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%sdebug[%d,trial]: trial parse FAILED, entering ERROR mode\n",
                                YYPREFIX, yydepth);
#endif
            /* Restore state as it was in the most forward-advanced error */
            yylvp          = yylvals   + yyerrctx->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylpp          = yylpsns   + yyerrctx->lexeme;
#endif
            yylexp         = yylexemes + yyerrctx->lexeme;
            yychar         = yylexp[-1];
            yylval         = yylvp[-1];
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc         = yylpp[-1];
#endif
            yystack.s_mark = yystack.s_base + (yyerrctx->yystack.s_mark - yyerrctx->yystack.s_base);
            memcpy (yystack.s_base, yyerrctx->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yystack.l_mark = yystack.l_base + (yyerrctx->yystack.l_mark - yyerrctx->yystack.l_base);
            memcpy (yystack.l_base, yyerrctx->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yystack.p_mark = yystack.p_base + (yyerrctx->yystack.p_mark - yyerrctx->yystack.p_base);
            memcpy (yystack.p_base, yyerrctx->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yystate        = yyerrctx->state;
            yyFreeState(yyerrctx);
            yyerrctx       = NULL;
        }
        yynewerrflag = 1;
    }
    if (yynewerrflag == 0) goto yyinrecovery;
#endif /* YYBTYACC */

    YYERROR_CALL("syntax error");
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yyerror_loc_range[1] = yylloc; /* lookahead position is error start position */
#endif

#if !YYBTYACC
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */
yyerrlab:
#endif
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if (((yyn = yysindex[*yystack.s_mark]) != 0) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: state %d, error recovery shifting to state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* lookahead position is error end position */
                yyerror_loc_range[2] = yylloc;
                YYLLOC_DEFAULT(yyloc, yyerror_loc_range, 2); /* position of error span */
                *++yystack.p_mark = yyloc;
#endif
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: error recovery discarding state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* the current TOS position is the error start position */
                yyerror_loc_range[1] = *yystack.p_mark;
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
                if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark, yystack.p_mark);
#else
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
                --yystack.s_mark;
                --yystack.l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                --yystack.p_mark;
#endif
            }
        }
    }
    else
    {
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, error recovery discarding token %d (%s)\n",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
        }
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval, &yylloc);
#else
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
    yym = yylen[yyn];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: state %d, reducing by rule %d (%s)",
                        YYDEBUGSTR, yydepth, yystate, yyn, yyrule[yyn]);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            if (yym > 0)
            {
                int i;
                fputc('<', stderr);
                for (i = yym; i > 0; i--)
                {
                    if (i != yym) fputs(", ", stderr);
                    fputs(YYSTYPE_TOSTRING(yystos[yystack.s_mark[1-i]],
                                           yystack.l_mark[1-i]), stderr);
                }
                fputc('>', stderr);
            }
#endif
        fputc('\n', stderr);
    }
#endif
    if (yym > 0)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)

    /* Perform position reduction */
    memset(&yyloc, 0, sizeof(yyloc));
#if YYBTYACC
    if (!yytrial)
#endif /* YYBTYACC */
    {
        YYLLOC_DEFAULT(yyloc, &yystack.p_mark[-yym], yym);
        /* just in case YYERROR is invoked within the action, save
           the start of the rhs as the error start position */
        yyerror_loc_range[1] = yystack.p_mark[1-yym];
    }
#endif

    switch (yyn)
    {
case 5:
#line 277 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ if (!srcdir) srcdir = yystack.l_mark[-1].str; }
#line 1 ""
break;
case 6:
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ if (!builddir) builddir = yystack.l_mark[-1].str; }
#line 1 ""
break;
case 7:
#line 283 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setmachine(yystack.l_mark[-1].str,NULL,NULL,0); }
#line 1 ""
break;
case 8:
#line 284 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setmachine(yystack.l_mark[-2].str,yystack.l_mark[-1].str,NULL,0); }
#line 1 ""
break;
case 9:
#line 285 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setmachine(yystack.l_mark[-3].str,yystack.l_mark[-2].str,yystack.l_mark[-1].list,0); }
#line 1 ""
break;
case 10:
#line 286 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setmachine(yystack.l_mark[-1].str,NULL,NULL,1); }
#line 1 ""
break;
case 11:
#line 287 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ stop("cannot proceed without machine or ioconf specifier"); }
#line 1 ""
break;
case 12:
#line 292 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_n(yystack.l_mark[0].str); }
#line 1 ""
break;
case 13:
#line 293 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_nx(yystack.l_mark[0].str, yystack.l_mark[-1].list); }
#line 1 ""
break;
case 14:
#line 297 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.i32 = 0; }
#line 1 ""
break;
case 15:
#line 298 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.i32 = 1; }
#line 1 ""
break;
case 16:
#line 309 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{
		CFGDBG(1, "ENDDEFS");
		check_maxpart();
		check_version();
	}
#line 1 ""
break;
case 19:
#line 320 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ wrap_continue(); }
#line 1 ""
break;
case 20:
#line 321 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ wrap_cleanup(); }
#line 1 ""
break;
case 21:
#line 322 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ enddefs(); checkfiles(); }
#line 1 ""
break;
case 44:
#line 353 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addfile(yystack.l_mark[-3].str, yystack.l_mark[-2].condexpr, yystack.l_mark[-1].flag, yystack.l_mark[0].str); }
#line 1 ""
break;
case 45:
#line 358 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addfile(yystack.l_mark[-2].str, yystack.l_mark[-1].condexpr, yystack.l_mark[0].flag, NULL); }
#line 1 ""
break;
case 46:
#line 364 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{
		adddevm(yystack.l_mark[-4].str, yystack.l_mark[-3].devmajor, yystack.l_mark[-2].devmajor, yystack.l_mark[-1].condexpr, yystack.l_mark[0].list);
		do_devsw = 1;
	}
#line 1 ""
break;
case 47:
#line 372 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ prefix_push(yystack.l_mark[0].str); }
#line 1 ""
break;
case 48:
#line 373 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ prefix_pop(); }
#line 1 ""
break;
case 49:
#line 377 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ buildprefix_push(yystack.l_mark[0].str); }
#line 1 ""
break;
case 50:
#line 378 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ buildprefix_push(yystack.l_mark[0].str); }
#line 1 ""
break;
case 51:
#line 379 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ buildprefix_pop(); }
#line 1 ""
break;
case 52:
#line 383 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ (void)defdevclass(yystack.l_mark[0].str, NULL, NULL, 1); }
#line 1 ""
break;
case 53:
#line 387 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ deffilesystem(yystack.l_mark[-1].list, yystack.l_mark[0].list); }
#line 1 ""
break;
case 54:
#line 392 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ (void)defattr0(yystack.l_mark[-2].str, yystack.l_mark[-1].loclist, yystack.l_mark[0].attrlist, 0); }
#line 1 ""
break;
case 55:
#line 397 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defoption(yystack.l_mark[-2].str, yystack.l_mark[-1].defoptlist, yystack.l_mark[0].list); }
#line 1 ""
break;
case 56:
#line 402 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defflag(yystack.l_mark[-2].str, yystack.l_mark[-1].defoptlist, yystack.l_mark[0].list, 0); }
#line 1 ""
break;
case 57:
#line 407 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defflag(yystack.l_mark[-1].str, yystack.l_mark[0].defoptlist, NULL, 1); }
#line 1 ""
break;
case 58:
#line 412 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defparam(yystack.l_mark[-2].str, yystack.l_mark[-1].defoptlist, yystack.l_mark[0].list, 0); }
#line 1 ""
break;
case 59:
#line 417 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defparam(yystack.l_mark[-1].str, yystack.l_mark[0].defoptlist, NULL, 1); }
#line 1 ""
break;
case 60:
#line 422 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defdev(yystack.l_mark[-2].devb, yystack.l_mark[-1].loclist, yystack.l_mark[0].attrlist, 0); }
#line 1 ""
break;
case 61:
#line 427 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defdevattach(yystack.l_mark[-1].deva, yystack.l_mark[-4].devb, yystack.l_mark[-2].list, yystack.l_mark[0].attrlist); }
#line 1 ""
break;
case 62:
#line 431 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ maxpartitions = yystack.l_mark[0].i32; }
#line 1 ""
break;
case 63:
#line 436 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setdefmaxusers(yystack.l_mark[-2].i32, yystack.l_mark[-1].i32, yystack.l_mark[0].i32); }
#line 1 ""
break;
case 65:
#line 446 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defdev(yystack.l_mark[-2].devb, yystack.l_mark[-1].loclist, yystack.l_mark[0].attrlist, 1); }
#line 1 ""
break;
case 66:
#line 451 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ defdev(yystack.l_mark[-2].devb, yystack.l_mark[-1].loclist, yystack.l_mark[0].attrlist, 2); }
#line 1 ""
break;
case 68:
#line 459 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setversion(yystack.l_mark[0].i32); }
#line 1 ""
break;
case 69:
#line 464 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.condexpr = NULL; }
#line 1 ""
break;
case 70:
#line 465 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.condexpr = yystack.l_mark[0].condexpr; }
#line 1 ""
break;
case 71:
#line 470 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.flag = 0; }
#line 1 ""
break;
case 72:
#line 471 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.flag = yystack.l_mark[-1].flag | yystack.l_mark[0].flag; }
#line 1 ""
break;
case 73:
#line 476 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.flag = FI_NEEDSCOUNT; }
#line 1 ""
break;
case 74:
#line 477 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.flag = FI_NEEDSFLAG; }
#line 1 ""
break;
case 75:
#line 482 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = NULL; }
#line 1 ""
break;
case 76:
#line 483 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 77:
#line 488 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.flag = 0; }
#line 1 ""
break;
case 78:
#line 489 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.flag = yystack.l_mark[-1].flag | yystack.l_mark[0].flag; }
#line 1 ""
break;
case 79:
#line 494 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.flag = FI_NEEDSFLAG; }
#line 1 ""
break;
case 80:
#line 499 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.devmajor = -1; }
#line 1 ""
break;
case 81:
#line 500 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.devmajor = yystack.l_mark[0].i32; }
#line 1 ""
break;
case 82:
#line 505 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.devmajor = -1; }
#line 1 ""
break;
case 83:
#line 506 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.devmajor = yystack.l_mark[0].i32; }
#line 1 ""
break;
case 84:
#line 511 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_s("DEVNODE_DONTBOTHER"); }
#line 1 ""
break;
case 85:
#line 512 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = nvcat(yystack.l_mark[-2].list, yystack.l_mark[0].list); }
#line 1 ""
break;
case 86:
#line 513 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = yystack.l_mark[0].list; }
#line 1 ""
break;
case 87:
#line 518 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_s("DEVNODE_SINGLE"); }
#line 1 ""
break;
case 88:
#line 519 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = nvcat(new_s("DEVNODE_VECTOR"), yystack.l_mark[0].list); }
#line 1 ""
break;
case 89:
#line 524 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_i(yystack.l_mark[0].num.val); }
#line 1 ""
break;
case 90:
#line 525 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{
		struct nvlist *__nv1, *__nv2;

		__nv1 = new_i(yystack.l_mark[-2].num.val);
		__nv2 = new_i(yystack.l_mark[0].num.val);
		yyval.list = nvcat(__nv1, __nv2);
	  }
#line 1 ""
break;
case 91:
#line 536 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_s("DEVNODE_FLAG_LINKZERO");}
#line 1 ""
break;
case 92:
#line 541 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_n(yystack.l_mark[0].str); }
#line 1 ""
break;
case 93:
#line 542 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_nx(yystack.l_mark[0].str, yystack.l_mark[-1].list); }
#line 1 ""
break;
case 94:
#line 547 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 95:
#line 552 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = NULL; }
#line 1 ""
break;
case 96:
#line 553 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = present_loclist(NULL); }
#line 1 ""
break;
case 97:
#line 554 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = present_loclist(yystack.l_mark[-1].loclist); }
#line 1 ""
break;
case 98:
#line 564 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = yystack.l_mark[0].loclist; }
#line 1 ""
break;
case 99:
#line 565 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = yystack.l_mark[-2].loclist; app(yystack.l_mark[-2].loclist, yystack.l_mark[0].loclist); }
#line 1 ""
break;
case 100:
#line 574 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = MK3(loc, yystack.l_mark[-1].str, yystack.l_mark[0].str, 0); }
#line 1 ""
break;
case 101:
#line 575 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = MK3(loc, yystack.l_mark[0].str, NULL, 0); }
#line 1 ""
break;
case 102:
#line 576 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = MK3(loc, yystack.l_mark[-2].str, yystack.l_mark[-1].str, 1); }
#line 1 ""
break;
case 103:
#line 577 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = locarray(yystack.l_mark[-3].str, yystack.l_mark[-1].i32, NULL, 0); }
#line 1 ""
break;
case 104:
#line 579 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = locarray(yystack.l_mark[-4].str, yystack.l_mark[-2].i32, yystack.l_mark[0].loclist, 0); }
#line 1 ""
break;
case 105:
#line 581 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = locarray(yystack.l_mark[-5].str, yystack.l_mark[-3].i32, yystack.l_mark[-1].loclist, 1); }
#line 1 ""
break;
case 106:
#line 586 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 107:
#line 587 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 108:
#line 592 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 109:
#line 597 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = yystack.l_mark[-1].loclist; }
#line 1 ""
break;
case 110:
#line 602 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.attrlist = NULL; }
#line 1 ""
break;
case 111:
#line 603 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.attrlist = yystack.l_mark[0].attrlist; }
#line 1 ""
break;
case 112:
#line 608 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.attrlist = MK2(attrlist, NULL, yystack.l_mark[0].attr); }
#line 1 ""
break;
case 113:
#line 609 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.attrlist = MK2(attrlist, yystack.l_mark[-2].attrlist, yystack.l_mark[0].attr); }
#line 1 ""
break;
case 114:
#line 614 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.attr = refattr(yystack.l_mark[0].str); }
#line 1 ""
break;
case 115:
#line 619 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = NULL; }
#line 1 ""
break;
case 116:
#line 620 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = yystack.l_mark[0].list; }
#line 1 ""
break;
case 117:
#line 625 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_n(yystack.l_mark[0].str); }
#line 1 ""
break;
case 118:
#line 626 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_nx(yystack.l_mark[0].str, yystack.l_mark[-2].list); }
#line 1 ""
break;
case 119:
#line 631 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 120:
#line 637 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_n(yystack.l_mark[0].str); }
#line 1 ""
break;
case 121:
#line 638 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_nx(yystack.l_mark[0].str, yystack.l_mark[-2].list); }
#line 1 ""
break;
case 122:
#line 643 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 123:
#line 644 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = NULL; }
#line 1 ""
break;
case 124:
#line 649 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.defoptlist = yystack.l_mark[0].defoptlist; }
#line 1 ""
break;
case 125:
#line 650 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.defoptlist = defoptlist_append(yystack.l_mark[0].defoptlist, yystack.l_mark[-1].defoptlist); }
#line 1 ""
break;
case 126:
#line 655 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.defoptlist = MK3(defoptlist, yystack.l_mark[0].str, NULL, NULL); }
#line 1 ""
break;
case 127:
#line 656 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.defoptlist = MK3(defoptlist, yystack.l_mark[-2].str, yystack.l_mark[0].str, NULL); }
#line 1 ""
break;
case 128:
#line 657 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.defoptlist = MK3(defoptlist, yystack.l_mark[-2].str, NULL, yystack.l_mark[0].str); }
#line 1 ""
break;
case 129:
#line 658 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.defoptlist = MK3(defoptlist, yystack.l_mark[-4].str, yystack.l_mark[-2].str, yystack.l_mark[0].str); }
#line 1 ""
break;
case 132:
#line 669 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ appendcondmkoption(yystack.l_mark[-3].condexpr, yystack.l_mark[-2].str, yystack.l_mark[0].str); }
#line 1 ""
break;
case 133:
#line 674 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.devb = getdevbase(yystack.l_mark[0].str); }
#line 1 ""
break;
case 134:
#line 679 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.deva = NULL; }
#line 1 ""
break;
case 135:
#line 680 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.deva = getdevattach(yystack.l_mark[0].str); }
#line 1 ""
break;
case 138:
#line 692 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setmajor(yystack.l_mark[-2].devb, yystack.l_mark[0].i32); }
#line 1 ""
break;
case 139:
#line 696 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{
		if (yystack.l_mark[0].num.val > INT_MAX || yystack.l_mark[0].num.val < INT_MIN)
			cfgerror("overflow %" PRId64, yystack.l_mark[0].num.val);
		else
			yyval.i32 = (int32_t)yystack.l_mark[0].num.val;
	}
#line 1 ""
break;
case 143:
#line 719 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ wrap_continue(); }
#line 1 ""
break;
case 144:
#line 720 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ wrap_cleanup(); }
#line 1 ""
break;
case 166:
#line 749 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addattr(yystack.l_mark[0].str); }
#line 1 ""
break;
case 167:
#line 753 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ delattr(yystack.l_mark[0].str, yystack.l_mark[-2].i32); }
#line 1 ""
break;
case 168:
#line 757 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ nowarn = yystack.l_mark[-1].i32; }
#line 1 ""
break;
case 169:
#line 757 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ nowarn = 0; }
#line 1 ""
break;
case 171:
#line 765 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ nowarn = yystack.l_mark[-1].i32; }
#line 1 ""
break;
case 172:
#line 765 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ nowarn = 0; }
#line 1 ""
break;
case 174:
#line 773 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ nowarn = yystack.l_mark[-1].i32; }
#line 1 ""
break;
case 175:
#line 773 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ nowarn = 0; }
#line 1 ""
break;
case 177:
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setmaxusers(yystack.l_mark[0].i32); }
#line 1 ""
break;
case 178:
#line 785 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setident(yystack.l_mark[0].str); }
#line 1 ""
break;
case 179:
#line 789 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setident(NULL); }
#line 1 ""
break;
case 180:
#line 794 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addconf(&conf); }
#line 1 ""
break;
case 181:
#line 798 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ delconf(yystack.l_mark[0].str, yystack.l_mark[-2].i32); }
#line 1 ""
break;
case 182:
#line 802 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ delpseudo(yystack.l_mark[0].str, yystack.l_mark[-2].i32); }
#line 1 ""
break;
case 183:
#line 806 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addpseudo(yystack.l_mark[-1].str, yystack.l_mark[0].i32); }
#line 1 ""
break;
case 184:
#line 810 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addpseudoroot(yystack.l_mark[0].str); }
#line 1 ""
break;
case 185:
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ deldevi(yystack.l_mark[-2].str, yystack.l_mark[0].str, yystack.l_mark[-3].i32); }
#line 1 ""
break;
case 186:
#line 819 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ deldeva(yystack.l_mark[0].str, yystack.l_mark[-3].i32); }
#line 1 ""
break;
case 187:
#line 823 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ deldev(yystack.l_mark[0].str, yystack.l_mark[-1].i32); }
#line 1 ""
break;
case 188:
#line 828 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ adddev(yystack.l_mark[-4].str, yystack.l_mark[-2].str, yystack.l_mark[-1].loclist, yystack.l_mark[0].i32); }
#line 1 ""
break;
case 191:
#line 839 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addfsoption(yystack.l_mark[0].str); }
#line 1 ""
break;
case 194:
#line 850 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ delfsoption(yystack.l_mark[0].str, nowarn); }
#line 1 ""
break;
case 197:
#line 862 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addmkoption(yystack.l_mark[-2].str, yystack.l_mark[0].str); }
#line 1 ""
break;
case 198:
#line 863 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ appendmkoption(yystack.l_mark[-2].str, yystack.l_mark[0].str); }
#line 1 ""
break;
case 201:
#line 875 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ delmkoption(yystack.l_mark[0].str, nowarn); }
#line 1 ""
break;
case 204:
#line 886 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addoption(yystack.l_mark[0].str, NULL); }
#line 1 ""
break;
case 205:
#line 887 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ addoption(yystack.l_mark[-2].str, yystack.l_mark[0].str); }
#line 1 ""
break;
case 208:
#line 898 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ deloption(yystack.l_mark[0].str, nowarn); }
#line 1 ""
break;
case 209:
#line 903 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{
		conf.cf_name = yystack.l_mark[0].str;
		conf.cf_where.w_srcline = currentline();
		conf.cf_fstype = NULL;
		conf.cf_root = NULL;
		conf.cf_dump = NULL;
	}
#line 1 ""
break;
case 210:
#line 914 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setconf(&conf.cf_root, "root", yystack.l_mark[0].list); }
#line 1 ""
break;
case 211:
#line 915 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setconf(&conf.cf_root, "root", yystack.l_mark[-1].list); }
#line 1 ""
break;
case 212:
#line 920 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_spi(intern("?"),
					    NULL,
					    (long long)NODEV); }
#line 1 ""
break;
case 213:
#line 923 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_spi(yystack.l_mark[0].str,
					    __UNCONST("spec"),
					    (long long)NODEV); }
#line 1 ""
break;
case 214:
#line 926 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_spi(yystack.l_mark[0].str,
					    NULL,
					    (long long)NODEV); }
#line 1 ""
break;
case 215:
#line 929 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.list = new_si(NULL, yystack.l_mark[0].val); }
#line 1 ""
break;
case 216:
#line 934 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.val = (int64_t)makedev(yystack.l_mark[-2].num.val, yystack.l_mark[0].num.val); }
#line 1 ""
break;
case 217:
#line 939 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setfstype(&conf.cf_fstype, intern("?")); }
#line 1 ""
break;
case 218:
#line 940 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setfstype(&conf.cf_fstype, yystack.l_mark[0].str); }
#line 1 ""
break;
case 221:
#line 951 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ setconf(&conf.cf_dump, "dumps", yystack.l_mark[0].list); }
#line 1 ""
break;
case 222:
#line 956 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.i32 = 1; }
#line 1 ""
break;
case 223:
#line 957 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.i32 = yystack.l_mark[0].i32; }
#line 1 ""
break;
case 224:
#line 962 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 225:
#line 963 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = starref(yystack.l_mark[-1].str); }
#line 1 ""
break;
case 226:
#line 968 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = NULL; }
#line 1 ""
break;
case 227:
#line 969 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 228:
#line 970 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = wildref(yystack.l_mark[-1].str); }
#line 1 ""
break;
case 229:
#line 975 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = NULL; }
#line 1 ""
break;
case 230:
#line 976 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = yystack.l_mark[0].loclist; app(yystack.l_mark[0].loclist, yystack.l_mark[-1].loclist); }
#line 1 ""
break;
case 231:
#line 981 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = MK3(loc, yystack.l_mark[-1].str, NULL, 0); }
#line 1 ""
break;
case 232:
#line 982 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = namelocvals(yystack.l_mark[-1].str, yystack.l_mark[0].loclist); }
#line 1 ""
break;
case 233:
#line 987 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.i32 = 0; }
#line 1 ""
break;
case 234:
#line 988 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.i32 = yystack.l_mark[0].i32; }
#line 1 ""
break;
case 237:
#line 1011 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.condexpr = MKF2(cx, or, yystack.l_mark[-2].condexpr, yystack.l_mark[0].condexpr); }
#line 1 ""
break;
case 239:
#line 1016 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.condexpr = MKF2(cx, and, yystack.l_mark[-2].condexpr, yystack.l_mark[0].condexpr); }
#line 1 ""
break;
case 241:
#line 1026 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.condexpr = yystack.l_mark[0].condexpr; }
#line 1 ""
break;
case 242:
#line 1027 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.condexpr = MKF1(cx, not, yystack.l_mark[0].condexpr); }
#line 1 ""
break;
case 243:
#line 1028 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.condexpr = yystack.l_mark[-1].condexpr; }
#line 1 ""
break;
case 244:
#line 1033 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.condexpr = MKF1(cx, atom, yystack.l_mark[0].str); }
#line 1 ""
break;
case 245:
#line 1044 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 246:
#line 1045 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 247:
#line 1050 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = NULL; }
#line 1 ""
break;
case 248:
#line 1051 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 249:
#line 1056 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 250:
#line 1057 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 251:
#line 1062 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 252:
#line 1063 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 253:
#line 1064 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 254:
#line 1065 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{
		char bf[40];

		(void)snprintf(bf, sizeof(bf), FORMAT(yystack.l_mark[0].num), (long long)yystack.l_mark[0].num.val);
		yyval.str = intern(bf);
	  }
#line 1 ""
break;
case 255:
#line 1075 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 256:
#line 1076 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.str = yystack.l_mark[0].str; }
#line 1 ""
break;
case 257:
#line 1082 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = MKF2(loc, val, yystack.l_mark[0].str, NULL); }
#line 1 ""
break;
case 258:
#line 1083 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.loclist = MKF2(loc, val, yystack.l_mark[-2].str, yystack.l_mark[0].loclist); }
#line 1 ""
break;
case 259:
#line 1088 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.num = yystack.l_mark[0].num; }
#line 1 ""
break;
case 260:
#line 1089 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/config/../../usr.bin/config/gram.y"
	{ yyval.num.fmt = yystack.l_mark[0].num.fmt; yyval.num.val = -yystack.l_mark[0].num.val; }
#line 1 ""
break;
#line 3177 "gram.c"
    default:
        break;
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
        {
            fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[YYFINAL], yyval));
#endif
            fprintf(stderr, "shifting from state 0 to final state %d\n", YYFINAL);
        }
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yyloc;
#endif
        if (yychar < 0)
        {
#if YYBTYACC
            do {
            if (yylvp < yylve)
            {
                /* we're currently re-reading tokens */
                yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylloc = *yylpp++;
#endif
                yychar = *yylexp++;
                break;
            }
            if (yyps->save)
            {
                /* in trial mode; save scanner results for future parse attempts */
                if (yylvp == yylvlim)
                {   /* Enlarge lexical value queue */
                    size_t p = (size_t) (yylvp - yylvals);
                    size_t s = (size_t) (yylvlim - yylvals);

                    s += YYLVQUEUEGROWTH;
                    if ((yylexemes = (YYINT *)realloc(yylexemes, s * sizeof(YYINT))) == NULL)
                        goto yyenomem;
                    if ((yylvals   = (YYSTYPE *)realloc(yylvals, s * sizeof(YYSTYPE))) == NULL)
                        goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    if ((yylpsns   = (YYLTYPE *)realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL)
                        goto yyenomem;
#endif
                    yylvp   = yylve = yylvals + p;
                    yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp   = yylpe = yylpsns + p;
                    yylplim = yylpsns + s;
#endif
                    yylexp  = yylexemes + p;
                }
                *yylexp = (YYINT) YYLEX;
                *yylvp++ = yylval;
                yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                *yylpp++ = yylloc;
                yylpe++;
#endif
                yychar = *yylexp++;
                break;
            }
            /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
            yychar = YYLEX;
#if YYBTYACC
            } while (0);
#endif /* YYBTYACC */
            if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
                fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)\n",
                                YYDEBUGSTR, yydepth, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
        goto yyloop;
    }
    if (((yyn = yygindex[yym]) != 0) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[yystate], yyval));
#endif
        fprintf(stderr, "shifting from state %d to state %d\n", *yystack.s_mark, yystate);
    }
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    *++yystack.p_mark = yyloc;
#endif
    goto yyloop;
#if YYBTYACC

    /* Reduction declares that this path is valid. Set yypath and do a full parse */
yyvalid:
    if (yypath) YYABORT;
    while (yyps->save)
    {
        YYParseState *save = yyps->save;
        yyps->save = save->save;
        save->save = yypath;
        yypath = save;
    }
#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%s[%d]: state %d, CONFLICT trial successful, backtracking to state %d, %d tokens\n",
                        YYDEBUGSTR, yydepth, yystate, yypath->state, (int)(yylvp - yylvals - yypath->lexeme));
#endif
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    yylvp          = yylvals + yypath->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yylpp          = yylpsns + yypath->lexeme;
#endif
    yylexp         = yylexemes + yypath->lexeme;
    yychar         = YYEMPTY;
    yystack.s_mark = yystack.s_base + (yypath->yystack.s_mark - yypath->yystack.s_base);
    memcpy (yystack.s_base, yypath->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
    yystack.l_mark = yystack.l_base + (yypath->yystack.l_mark - yypath->yystack.l_base);
    memcpy (yystack.l_base, yypath->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base + (yypath->yystack.p_mark - yypath->yystack.p_base);
    memcpy (yystack.p_base, yypath->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
    yystate        = yypath->state;
    goto yyloop;
#endif /* YYBTYACC */

yyoverflow:
    YYERROR_CALL("yacc stack overflow");
#if YYBTYACC
    goto yyabort_nomem;
yyenomem:
    YYERROR_CALL("memory exhausted");
yyabort_nomem:
#endif /* YYBTYACC */
    yyresult = 2;
    goto yyreturn;

yyabort:
    yyresult = 1;
    goto yyreturn;

yyaccept:
#if YYBTYACC
    if (yyps->save) goto yyvalid;
#endif /* YYBTYACC */
    yyresult = 0;

yyreturn:
#if defined(YYDESTRUCT_CALL)
    if (yychar != YYEOF && yychar != YYEMPTY)
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval, &yylloc);
#else
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */

    {
        YYSTYPE *pv;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYLTYPE *pp;

        for (pv = yystack.l_base, pp = yystack.p_base; pv <= yystack.l_mark; ++pv, ++pp)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv, pp);
#else
        for (pv = yystack.l_base; pv <= yystack.l_mark; ++pv)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
    }
#endif /* defined(YYDESTRUCT_CALL) */

#if YYBTYACC
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    while (yyps)
    {
        YYParseState *save = yyps;
        yyps = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
    while (yypath)
    {
        YYParseState *save = yypath;
        yypath = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
#endif /* YYBTYACC */
    yyfreestack(&yystack);
    return (yyresult);
}
