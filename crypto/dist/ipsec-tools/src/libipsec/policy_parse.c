/*	$NetBSD: policy_parse.c,v 1.2.2.2 2005/11/21 21:12:30 tron Exp $	*/


/*  A Bison parser, made from policy_parse.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse __libipsecparse
#define yylex __libipseclex
#define yyerror __libipsecerror
#define yylval __libipseclval
#define yychar __libipsecchar
#define yydebug __libipsecdebug
#define yynerrs __libipsecnerrs
#define	DIR	257
#define	PRIORITY	258
#define	PLUS	259
#define	PRIO_BASE	260
#define	PRIO_OFFSET	261
#define	ACTION	262
#define	PROTOCOL	263
#define	MODE	264
#define	LEVEL	265
#define	LEVEL_SPECIFY	266
#define	IPADDRESS	267
#define	PORT	268
#define	ME	269
#define	ANY	270
#define	SLASH	271
#define	HYPHEN	272

#line 63 "policy_parse.y"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#ifdef HAVE_NETINET6_IPSEC
#  include <netinet6/ipsec.h>
#else
#  include <netinet/ipsec.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include <errno.h>

#include "config.h"

#include "ipsec_strerror.h"
#include "libpfkey.h"

#ifndef INT32_MAX
#define INT32_MAX	(0xffffffff)
#endif

#ifndef INT32_MIN
#define INT32_MIN	(-INT32_MAX-1)
#endif

#define ATOX(c) \
  (isdigit(c) ? (c - '0') : (isupper(c) ? (c - 'A' + 10) : (c - 'a' + 10) ))

static u_int8_t *pbuf = NULL;		/* sadb_x_policy buffer */
static int tlen = 0;			/* total length of pbuf */
static int offset = 0;			/* offset of pbuf */
static int p_dir, p_type, p_protocol, p_mode, p_level, p_reqid;
static u_int32_t p_priority = 0;
static long p_priority_offset = 0;
static struct sockaddr *p_src = NULL;
static struct sockaddr *p_dst = NULL;

struct _val;
extern void yyerror __P((char *msg));
static struct sockaddr *parse_sockaddr __P((struct _val *addrbuf,
    struct _val *portbuf));
static int rule_check __P((void));
static int init_x_policy __P((void));
static int set_x_request __P((struct sockaddr *, struct sockaddr *));
static int set_sockaddr __P((struct sockaddr *));
static void policy_parse_request_init __P((void));
static void *policy_parse __P((const char *, int));

extern void __policy__strbuffer__init__ __P((const char *));
extern void __policy__strbuffer__free__ __P((void));
extern int yyparse __P((void));
extern int yylex __P((void));

extern char *__libipsectext;	/*XXX*/


#line 131 "policy_parse.y"
typedef union {
	u_int num;
	u_int32_t num32;
	struct _val {
		int len;
		char *buf;
	} val;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		60
#define	YYFLAG		-32768
#define	YYNTBASE	19

#define YYTRANSLATE(x) ((unsigned)(x) <= 272 ? yytranslate[x] : 34)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     6,     7,    14,    15,    23,    24,    31,    32,
    41,    42,    51,    53,    54,    57,    65,    72,    78,    83,
    90,    94,    97,    99,   101,   103,   105,   107,   108,   113,
   114,   121,   125
};

static const short yyrhs[] = {    -1,
     3,     8,    20,    26,     0,     0,     3,     4,     7,     8,
    21,    26,     0,     0,     3,     4,    18,     7,     8,    22,
    26,     0,     0,     3,     4,     6,     8,    23,    26,     0,
     0,     3,     4,     6,     5,     7,     8,    24,    26,     0,
     0,     3,     4,     6,    18,     7,     8,    25,    26,     0,
     3,     0,     0,    26,    27,     0,    28,    17,    29,    17,
    31,    17,    30,     0,    28,    17,    29,    17,    31,    17,
     0,    28,    17,    29,    17,    31,     0,    28,    17,    29,
    17,     0,    28,    17,    29,    17,    17,    30,     0,    28,
    17,    29,     0,    28,    17,     0,    28,     0,     9,     0,
    10,     0,    11,     0,    12,     0,     0,    13,    32,    18,
    13,     0,     0,    13,    14,    33,    18,    13,    14,     0,
    15,    18,    16,     0,    16,    18,    15,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   152,   167,   167,   203,   203,   225,   225,   236,   236,   258,
   258,   280,   280,   293,   294,   306,   307,   308,   309,   310,
   311,   312,   316,   323,   327,   331,   335,   342,   348,   353,
   359,   364,   370
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","DIR","PRIORITY",
"PLUS","PRIO_BASE","PRIO_OFFSET","ACTION","PROTOCOL","MODE","LEVEL","LEVEL_SPECIFY",
"IPADDRESS","PORT","ME","ANY","SLASH","HYPHEN","policy_spec","@1","@2","@3",
"@4","@5","@6","rules","rule","protocol","mode","level","addresses","@7","@8", NULL
};
#endif

static const short yyr1[] = {     0,
    20,    19,    21,    19,    22,    19,    23,    19,    24,    19,
    25,    19,    19,    26,    26,    27,    27,    27,    27,    27,
    27,    27,    27,    28,    29,    30,    30,    32,    31,    33,
    31,    31,    31
};

static const short yyr2[] = {     0,
     0,     4,     0,     6,     0,     7,     0,     6,     0,     8,
     0,     8,     1,     0,     2,     7,     6,     5,     4,     6,
     3,     2,     1,     1,     1,     1,     1,     0,     4,     0,
     6,     3,     3
};

static const short yydefact[] = {     0,
    13,     0,     1,     0,     0,     0,    14,     0,     7,     0,
     3,     0,     2,     0,    14,     0,    14,     5,    24,    15,
    23,     9,     8,    11,     4,    14,    22,    14,    14,     6,
    25,    21,    10,    12,    19,    28,     0,     0,     0,    18,
    30,     0,     0,     0,    26,    27,    20,    17,     0,     0,
    32,    33,    16,     0,    29,     0,    31,     0,     0,     0
};

static const short yydefgoto[] = {    58,
     7,    17,    26,    15,    28,    29,    13,    20,    21,    32,
    47,    40,    42,    49
};

static const short yypact[] = {     6,
    -3,     1,-32768,    -2,    -4,     3,-32768,     8,-32768,    10,
-32768,     4,    16,    18,-32768,    19,-32768,-32768,-32768,-32768,
    11,-32768,    16,-32768,    16,-32768,    20,-32768,-32768,    16,
-32768,    14,    16,    16,     5,    15,    17,    21,    12,    23,
-32768,    24,    22,    26,-32768,-32768,-32768,    12,    25,    31,
-32768,-32768,-32768,    32,-32768,    33,-32768,    34,    36,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   -15,-32768,-32768,-32768,
   -16,-32768,-32768,-32768
};


#define	YYLAST		47


static const short yytable[] = {    23,
     2,    25,     8,    11,     3,     9,     4,     5,     1,    12,
    30,    18,    33,    34,    14,    10,    16,    36,     6,    37,
    38,    39,    45,    46,    19,    22,    24,    27,    41,    31,
    35,    53,     0,    59,    43,    60,     0,    51,    44,    48,
    52,    50,    54,    55,    56,     0,    57
};

static const short yycheck[] = {    15,
     4,    17,     5,     8,     8,     8,     6,     7,     3,     7,
    26,     8,    28,    29,     7,    18,     7,    13,    18,    15,
    16,    17,    11,    12,     9,     8,     8,    17,    14,    10,
    17,    48,    -1,     0,    18,     0,    -1,    16,    18,    17,
    15,    18,    18,    13,    13,    -1,    14
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 153 "policy_parse.y"
{
			p_dir = yyvsp[-1].num;
			p_type = yyvsp[0].num;

#ifdef HAVE_PFKEY_POLICY_PRIORITY
			p_priority = PRIORITY_DEFAULT;
#else
			p_priority = 0;
#endif

			if (init_x_policy())
				return -1;
		;
    break;}
case 3:
#line 168 "policy_parse.y"
{
			char *offset_buf;

			p_dir = yyvsp[-3].num;
			p_type = yyvsp[0].num;

			/* buffer big enough to hold a prepended negative sign */
			offset_buf = malloc(yyvsp[-1].val.len + 2);
			if (offset_buf == NULL) 
			{
				__ipsec_errcode = EIPSEC_NO_BUFS;
				return -1;
			}

			/* positive input value means higher priority, therefore lower
			   actual value so that is closer to the beginning of the list */
			sprintf (offset_buf, "-%s", yyvsp[-1].val.buf);

			errno = 0;
			p_priority_offset = atol(offset_buf);

			free(offset_buf);

			if (errno != 0 || p_priority_offset < INT32_MIN)
			{
				__ipsec_errcode = EIPSEC_INVAL_PRIORITY_OFFSET;
				return -1;
			}

			p_priority = PRIORITY_DEFAULT + (u_int32_t) p_priority_offset;

			if (init_x_policy())
				return -1;
		;
    break;}
case 5:
#line 204 "policy_parse.y"
{
			p_dir = yyvsp[-4].num;
			p_type = yyvsp[0].num;

			errno = 0;
			p_priority_offset = atol(yyvsp[-1].val.buf);

			if (errno != 0 || p_priority_offset > INT32_MAX)
			{
				__ipsec_errcode = EIPSEC_INVAL_PRIORITY_OFFSET;
				return -1;
			}

			/* negative input value means lower priority, therefore higher
			   actual value so that is closer to the end of the list */
			p_priority = PRIORITY_DEFAULT + (u_int32_t) p_priority_offset;

			if (init_x_policy())
				return -1;
		;
    break;}
case 7:
#line 226 "policy_parse.y"
{
			p_dir = yyvsp[-3].num;
			p_type = yyvsp[0].num;

			p_priority = yyvsp[-1].num32;

			if (init_x_policy())
				return -1;
		;
    break;}
case 9:
#line 237 "policy_parse.y"
{
			p_dir = yyvsp[-5].num;
			p_type = yyvsp[0].num;

			errno = 0;
			p_priority_offset = atol(yyvsp[-1].val.buf);

			if (errno != 0 || p_priority_offset > PRIORITY_OFFSET_NEGATIVE_MAX)
			{
				__ipsec_errcode = EIPSEC_INVAL_PRIORITY_BASE_OFFSET;
				return -1;
			}

			/* adding value means higher priority, therefore lower
			   actual value so that is closer to the beginning of the list */
			p_priority = yyvsp[-3].num32 - (u_int32_t) p_priority_offset;

			if (init_x_policy())
				return -1;
		;
    break;}
case 11:
#line 259 "policy_parse.y"
{
			p_dir = yyvsp[-5].num;
			p_type = yyvsp[0].num;

			errno = 0;
			p_priority_offset = atol(yyvsp[-1].val.buf);

			if (errno != 0 || p_priority_offset > PRIORITY_OFFSET_POSITIVE_MAX)
			{
				__ipsec_errcode = EIPSEC_INVAL_PRIORITY_BASE_OFFSET;
				return -1;
			}

			/* subtracting value means lower priority, therefore higher
			   actual value so that is closer to the end of the list */
			p_priority = yyvsp[-3].num32 + (u_int32_t) p_priority_offset;

			if (init_x_policy())
				return -1;
		;
    break;}
case 13:
#line 281 "policy_parse.y"
{
			p_dir = yyvsp[0].num;
			p_type = 0;	/* ignored it by kernel */

			p_priority = 0;

			if (init_x_policy())
				return -1;
		;
    break;}
case 15:
#line 294 "policy_parse.y"
{
			if (rule_check() < 0)
				return -1;

			if (set_x_request(p_src, p_dst) < 0)
				return -1;

			policy_parse_request_init();
		;
    break;}
case 22:
#line 312 "policy_parse.y"
{
			__ipsec_errcode = EIPSEC_FEW_ARGUMENTS;
			return -1;
		;
    break;}
case 23:
#line 316 "policy_parse.y"
{
			__ipsec_errcode = EIPSEC_FEW_ARGUMENTS;
			return -1;
		;
    break;}
case 24:
#line 323 "policy_parse.y"
{ p_protocol = yyvsp[0].num; ;
    break;}
case 25:
#line 327 "policy_parse.y"
{ p_mode = yyvsp[0].num; ;
    break;}
case 26:
#line 331 "policy_parse.y"
{
			p_level = yyvsp[0].num;
			p_reqid = 0;
		;
    break;}
case 27:
#line 335 "policy_parse.y"
{
			p_level = IPSEC_LEVEL_UNIQUE;
			p_reqid = atol(yyvsp[0].val.buf);	/* atol() is good. */
		;
    break;}
case 28:
#line 342 "policy_parse.y"
{
			p_src = parse_sockaddr(&yyvsp[0].val, NULL);
			if (p_src == NULL)
				return -1;
		;
    break;}
case 29:
#line 348 "policy_parse.y"
{
			p_dst = parse_sockaddr(&yyvsp[0].val, NULL);
			if (p_dst == NULL)
				return -1;
		;
    break;}
case 30:
#line 353 "policy_parse.y"
{
			p_src = parse_sockaddr(&yyvsp[-1].val, &yyvsp[0].val);
			if (p_src == NULL)
				return -1;
		;
    break;}
case 31:
#line 359 "policy_parse.y"
{
			p_dst = parse_sockaddr(&yyvsp[-1].val, &yyvsp[0].val);
			if (p_dst == NULL)
				return -1;
		;
    break;}
case 32:
#line 364 "policy_parse.y"
{
			if (p_dir != IPSEC_DIR_OUTBOUND) {
				__ipsec_errcode = EIPSEC_INVAL_DIR;
				return -1;
			}
		;
    break;}
case 33:
#line 370 "policy_parse.y"
{
			if (p_dir != IPSEC_DIR_INBOUND) {
				__ipsec_errcode = EIPSEC_INVAL_DIR;
				return -1;
			}
		;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 381 "policy_parse.y"


void
yyerror(msg)
	char *msg;
{
	fprintf(stderr, "libipsec: %s while parsing \"%s\"\n",
		msg, __libipsectext);

	return;
}

static struct sockaddr *
parse_sockaddr(addrbuf, portbuf)
	struct _val *addrbuf;
	struct _val *portbuf;
{
	struct addrinfo hints, *res;
	char *addr;
	char *serv = NULL;
	int error;
	struct sockaddr *newaddr = NULL;

	if ((addr = malloc(addrbuf->len + 1)) == NULL) {
		yyerror("malloc failed");
		__ipsec_set_strerror(strerror(errno));
		return NULL;
	}

	if (portbuf && ((serv = malloc(portbuf->len + 1)) == NULL)) {
		free(addr);
		yyerror("malloc failed");
		__ipsec_set_strerror(strerror(errno));
		return NULL;
	}

	strncpy(addr, addrbuf->buf, addrbuf->len);
	addr[addrbuf->len] = '\0';

	if (portbuf) {
		strncpy(serv, portbuf->buf, portbuf->len);
		serv[portbuf->len] = '\0';
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_socktype = SOCK_DGRAM;
	error = getaddrinfo(addr, serv, &hints, &res);
	free(addr);
	if (serv != NULL)
		free(serv);
	if (error != 0) {
		yyerror("invalid IP address");
		__ipsec_set_strerror(gai_strerror(error));
		return NULL;
	}

	if (res->ai_addr == NULL) {
		yyerror("invalid IP address");
		__ipsec_set_strerror(gai_strerror(error));
		return NULL;
	}

	newaddr = malloc(res->ai_addrlen);
	if (newaddr == NULL) {
		__ipsec_errcode = EIPSEC_NO_BUFS;
		freeaddrinfo(res);
		return NULL;
	}
	memcpy(newaddr, res->ai_addr, res->ai_addrlen);

	freeaddrinfo(res);

	__ipsec_errcode = EIPSEC_NO_ERROR;
	return newaddr;
}

static int
rule_check()
{
	if (p_type == IPSEC_POLICY_IPSEC) {
		if (p_protocol == IPPROTO_IP) {
			__ipsec_errcode = EIPSEC_NO_PROTO;
			return -1;
		}

		if (p_mode != IPSEC_MODE_TRANSPORT
		 && p_mode != IPSEC_MODE_TUNNEL) {
			__ipsec_errcode = EIPSEC_INVAL_MODE;
			return -1;
		}

		if (p_src == NULL && p_dst == NULL) {
			 if (p_mode != IPSEC_MODE_TRANSPORT) {
				__ipsec_errcode = EIPSEC_INVAL_ADDRESS;
				return -1;
			}
		}
		else if (p_src->sa_family != p_dst->sa_family) {
			__ipsec_errcode = EIPSEC_FAMILY_MISMATCH;
			return -1;
		}
	}

	__ipsec_errcode = EIPSEC_NO_ERROR;
	return 0;
}

static int
init_x_policy()
{
	struct sadb_x_policy *p;

	if (pbuf) {
		free(pbuf);
		tlen = 0;
	}
	pbuf = malloc(sizeof(struct sadb_x_policy));
	if (pbuf == NULL) {
		__ipsec_errcode = EIPSEC_NO_BUFS;
		return -1;
	}
	tlen = sizeof(struct sadb_x_policy);

	memset(pbuf, 0, tlen);
	p = (struct sadb_x_policy *)pbuf;
	p->sadb_x_policy_len = 0;	/* must update later */
	p->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	p->sadb_x_policy_type = p_type;
	p->sadb_x_policy_dir = p_dir;
	p->sadb_x_policy_id = 0;
#ifdef HAVE_PFKEY_POLICY_PRIORITY
	p->sadb_x_policy_priority = p_priority;
#else
    /* fail if given a priority and libipsec was not compiled with 
	   priority support */
	if (p_priority != 0)
	{
		__ipsec_errcode = EIPSEC_PRIORITY_NOT_COMPILED;
		return -1;
	}
#endif

	offset = tlen;

	__ipsec_errcode = EIPSEC_NO_ERROR;
	return 0;
}

static int
set_x_request(src, dst)
	struct sockaddr *src, *dst;
{
	struct sadb_x_ipsecrequest *p;
	int reqlen;
	u_int8_t *n;

	reqlen = sizeof(*p)
		+ (src ? sysdep_sa_len(src) : 0)
		+ (dst ? sysdep_sa_len(dst) : 0);
	tlen += reqlen;		/* increment to total length */

	n = realloc(pbuf, tlen);
	if (n == NULL) {
		__ipsec_errcode = EIPSEC_NO_BUFS;
		return -1;
	}
	pbuf = n;

	p = (struct sadb_x_ipsecrequest *)&pbuf[offset];
	p->sadb_x_ipsecrequest_len = reqlen;
	p->sadb_x_ipsecrequest_proto = p_protocol;
	p->sadb_x_ipsecrequest_mode = p_mode;
	p->sadb_x_ipsecrequest_level = p_level;
	p->sadb_x_ipsecrequest_reqid = p_reqid;
	offset += sizeof(*p);

	if (set_sockaddr(src) || set_sockaddr(dst))
		return -1;

	__ipsec_errcode = EIPSEC_NO_ERROR;
	return 0;
}

static int
set_sockaddr(addr)
	struct sockaddr *addr;
{
	if (addr == NULL) {
		__ipsec_errcode = EIPSEC_NO_ERROR;
		return 0;
	}

	/* tlen has already incremented */

	memcpy(&pbuf[offset], addr, sysdep_sa_len(addr));

	offset += sysdep_sa_len(addr);

	__ipsec_errcode = EIPSEC_NO_ERROR;
	return 0;
}

static void
policy_parse_request_init()
{
	p_protocol = IPPROTO_IP;
	p_mode = IPSEC_MODE_ANY;
	p_level = IPSEC_LEVEL_DEFAULT;
	p_reqid = 0;
	if (p_src != NULL) {
		free(p_src);
		p_src = NULL;
	}
	if (p_dst != NULL) {
		free(p_dst);
		p_dst = NULL;
	}

	return;
}

static void *
policy_parse(msg, msglen)
	const char *msg;
	int msglen;
{
	int error;

	pbuf = NULL;
	tlen = 0;

	/* initialize */
	p_dir = IPSEC_DIR_INVALID;
	p_type = IPSEC_POLICY_DISCARD;
	policy_parse_request_init();
	__policy__strbuffer__init__(msg);

	error = yyparse();	/* it must be set errcode. */
	__policy__strbuffer__free__();

	if (error) {
		if (pbuf != NULL)
			free(pbuf);
		return NULL;
	}

	/* update total length */
	((struct sadb_x_policy *)pbuf)->sadb_x_policy_len = PFKEY_UNIT64(tlen);

	__ipsec_errcode = EIPSEC_NO_ERROR;

	return pbuf;
}

ipsec_policy_t
ipsec_set_policy(msg, msglen)
	__ipsec_const char *msg;
	int msglen;
{
	caddr_t policy;

	policy = policy_parse(msg, msglen);
	if (policy == NULL) {
		if (__ipsec_errcode == EIPSEC_NO_ERROR)
			__ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
		return NULL;
	}

	__ipsec_errcode = EIPSEC_NO_ERROR;
	return policy;
}
