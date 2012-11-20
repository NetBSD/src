/* A Bison parser, made from defparse.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	NAME	257
# define	LIBRARY	258
# define	DESCRIPTION	259
# define	STACKSIZE	260
# define	HEAPSIZE	261
# define	CODE	262
# define	DATA	263
# define	SECTIONS	264
# define	EXPORTS	265
# define	IMPORTS	266
# define	VERSIONK	267
# define	BASE	268
# define	CONSTANT	269
# define	READ	270
# define	WRITE	271
# define	EXECUTE	272
# define	SHARED	273
# define	NONSHARED	274
# define	NONAME	275
# define	PRIVATE	276
# define	SINGLE	277
# define	MULTIPLE	278
# define	INITINSTANCE	279
# define	INITGLOBAL	280
# define	TERMINSTANCE	281
# define	TERMGLOBAL	282
# define	ID	283
# define	NUMBER	284

#line 1 "defparse.y"
 /* defparse.y - parser for .def files */

/*  Copyright 1995, 1997, 1998, 1999, 2001, 2004
    Free Software Foundation, Inc.

    This file is part of GNU Binutils.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"
#include "bucomm.h"
#include "dlltool.h"

#line 27 "defparse.y"
#ifndef YYSTYPE
typedef union {
  char *id;
  int number;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		98
#define	YYFLAG		-32768
#define	YYNTBASE	35

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 284 ? yytranslate[x] : 57)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    33,     2,    31,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    32,     2,     2,    34,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     3,     5,     9,    14,    17,    20,    24,    28,
      31,    34,    37,    40,    43,    48,    49,    52,    60,    63,
      65,    73,    81,    87,    93,    99,   105,   109,   113,   116,
     118,   121,   125,   127,   129,   130,   133,   134,   136,   138,
     140,   142,   144,   146,   148,   150,   151,   153,   154,   156,
     157,   159,   160,   162,   166,   167,   170,   171,   174,   179,
     180,   184,   185,   186,   190,   192,   194,   196
};
static const short yyrhs[] =
{
      35,    36,     0,    36,     0,     3,    51,    54,     0,     4,
      51,    54,    55,     0,    11,    37,     0,     5,    29,     0,
       6,    30,    45,     0,     7,    30,    45,     0,     8,    43,
       0,     9,    43,     0,    10,    41,     0,    12,    39,     0,
      13,    30,     0,    13,    30,    31,    30,     0,     0,    37,
      38,     0,    29,    53,    52,    48,    47,    49,    50,     0,
      39,    40,     0,    40,     0,    29,    32,    29,    31,    29,
      31,    29,     0,    29,    32,    29,    31,    29,    31,    30,
       0,    29,    32,    29,    31,    29,     0,    29,    32,    29,
      31,    30,     0,    29,    31,    29,    31,    29,     0,    29,
      31,    29,    31,    30,     0,    29,    31,    29,     0,    29,
      31,    30,     0,    41,    42,     0,    42,     0,    29,    43,
       0,    43,    44,    46,     0,    46,     0,    33,     0,     0,
      33,    30,     0,     0,    16,     0,    17,     0,    18,     0,
      19,     0,    20,     0,    23,     0,    24,     0,    15,     0,
       0,    21,     0,     0,     9,     0,     0,    22,     0,     0,
      29,     0,    29,    31,    29,     0,     0,    34,    30,     0,
       0,    32,    29,     0,    32,    29,    31,    29,     0,     0,
      14,    32,    30,     0,     0,     0,    55,    44,    56,     0,
      25,     0,    26,     0,    27,     0,    28,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,    44,    45,    48,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    64,    66,    69,    73,    75,
      78,    80,    81,    82,    83,    84,    85,    86,    89,    91,
      94,    98,   100,   103,   105,   107,   108,   111,   113,   114,
     115,   116,   117,   118,   121,   123,   126,   128,   131,   133,
     136,   138,   141,   142,   148,   151,   153,   156,   158,   164,
     167,   168,   171,   173,   176,   178,   179,   180
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "NAME", "LIBRARY", "DESCRIPTION", 
  "STACKSIZE", "HEAPSIZE", "CODE", "DATA", "SECTIONS", "EXPORTS", 
  "IMPORTS", "VERSIONK", "BASE", "CONSTANT", "READ", "WRITE", "EXECUTE", 
  "SHARED", "NONSHARED", "NONAME", "PRIVATE", "SINGLE", "MULTIPLE", 
  "INITINSTANCE", "INITGLOBAL", "TERMINSTANCE", "TERMGLOBAL", "ID", 
  "NUMBER", "'.'", "'='", "','", "'@'", "start", "command", "explist", 
  "expline", "implist", "impline", "seclist", "secline", "attr_list", 
  "opt_comma", "opt_number", "attr", "opt_CONSTANT", "opt_NONAME", 
  "opt_DATA", "opt_PRIVATE", "opt_name", "opt_ordinal", "opt_equal_name", 
  "opt_base", "option_list", "option", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    35,    35,    36,    36,    36,    36,    36,    36,    36,
      36,    36,    36,    36,    36,    37,    37,    38,    39,    39,
      40,    40,    40,    40,    40,    40,    40,    40,    41,    41,
      42,    43,    43,    44,    44,    45,    45,    46,    46,    46,
      46,    46,    46,    46,    47,    47,    48,    48,    49,    49,
      50,    50,    51,    51,    51,    52,    52,    53,    53,    53,
      54,    54,    55,    55,    56,    56,    56,    56
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     2,     1,     3,     4,     2,     2,     3,     3,     2,
       2,     2,     2,     2,     4,     0,     2,     7,     2,     1,
       7,     7,     5,     5,     5,     5,     3,     3,     2,     1,
       2,     3,     1,     1,     0,     2,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     1,     0,     1,     0,
       1,     0,     1,     3,     0,     2,     0,     2,     4,     0,
       3,     0,     0,     3,     1,     1,     1,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       0,    54,    54,     0,     0,     0,     0,     0,     0,    15,
       0,     0,     0,     2,    52,    61,    61,     6,    36,    36,
      37,    38,    39,    40,    41,    42,    43,     9,    32,    10,
       0,    11,    29,     5,     0,    12,    19,    13,     1,     0,
       0,     3,    62,     0,     7,     8,    33,     0,    30,    28,
      59,    16,     0,     0,    18,     0,    53,     0,     4,    35,
      31,     0,    56,    26,    27,     0,    14,    60,     0,    57,
       0,    47,     0,     0,    64,    65,    66,    67,    63,     0,
      55,    46,    45,    24,    25,    22,    23,    58,    44,    49,
       0,    48,    51,    20,    21,    50,    17,     0,     0
};

static const short yydefgoto[] =
{
      12,    13,    33,    51,    35,    36,    31,    32,    27,    47,
      44,    28,    89,    82,    92,    96,    15,    71,    62,    41,
      58,    78
};

static const short yypact[] =
{
      32,   -12,   -12,    17,    -8,    33,    -4,    -4,    35,-32768,
      36,    37,    21,-32768,    38,    48,    48,-32768,    39,    39,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   -15,-32768,   -15,
      -4,    35,-32768,    41,   -25,    36,-32768,    40,-32768,    44,
      34,-32768,-32768,    45,-32768,-32768,-32768,    -4,   -15,-32768,
      42,-32768,   -19,    47,-32768,    49,-32768,    50,    22,-32768,
  -32768,    52,    43,    51,-32768,    53,-32768,-32768,    26,    54,
      56,    57,    27,    29,-32768,-32768,-32768,-32768,-32768,    58,
  -32768,-32768,    68,-32768,-32768,    59,-32768,-32768,-32768,    79,
      31,-32768,    46,-32768,-32768,-32768,-32768,    89,-32768
};

static const short yypgoto[] =
{
  -32768,    80,-32768,-32768,-32768,    60,-32768,    62,    -7,    55,
      72,    61,-32768,-32768,-32768,-32768,    92,-32768,-32768,    81,
  -32768,-32768
};


#define	YYLAST		113


static const short yytable[] =
{
      29,   -34,   -34,   -34,   -34,   -34,    52,    53,   -34,   -34,
      63,    64,    20,    21,    22,    23,    24,    14,    46,    25,
      26,    97,    18,    48,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    17,   -34,   -34,   -34,
     -34,    74,    75,    76,    77,    46,    83,    84,    85,    86,
      93,    94,    40,    19,    30,    34,    57,    37,    95,    39,
      50,    55,    43,    56,    61,    59,    65,    70,    81,    66,
      67,    69,    72,    88,    73,    79,    80,    87,    91,    98,
      90,    45,    38,    49,    16,    54,     0,    42,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    60,     0,
       0,     0,     0,    68
};

static const short yycheck[] =
{
       7,    16,    17,    18,    19,    20,    31,    32,    23,    24,
      29,    30,    16,    17,    18,    19,    20,    29,    33,    23,
      24,     0,    30,    30,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    29,    25,    26,    27,
      28,    25,    26,    27,    28,    33,    29,    30,    29,    30,
      29,    30,    14,    30,    29,    29,    32,    30,    22,    31,
      29,    31,    33,    29,    32,    30,    29,    34,    21,    30,
      30,    29,    31,    15,    31,    31,    30,    29,     9,     0,
      31,    19,    12,    31,     2,    35,    -1,    16,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    58
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

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

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   include <string.h>
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif

#line 316 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
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
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
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
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 3:
#line 49 "defparse.y"
{ def_name (yyvsp[-1].id, yyvsp[0].number); }
    break;
case 4:
#line 50 "defparse.y"
{ def_library (yyvsp[-2].id, yyvsp[-1].number); }
    break;
case 6:
#line 52 "defparse.y"
{ def_description (yyvsp[0].id);}
    break;
case 7:
#line 53 "defparse.y"
{ def_stacksize (yyvsp[-1].number, yyvsp[0].number);}
    break;
case 8:
#line 54 "defparse.y"
{ def_heapsize (yyvsp[-1].number, yyvsp[0].number);}
    break;
case 9:
#line 55 "defparse.y"
{ def_code (yyvsp[0].number);}
    break;
case 10:
#line 56 "defparse.y"
{ def_data (yyvsp[0].number);}
    break;
case 13:
#line 59 "defparse.y"
{ def_version (yyvsp[0].number,0);}
    break;
case 14:
#line 60 "defparse.y"
{ def_version (yyvsp[-2].number,yyvsp[0].number);}
    break;
case 17:
#line 71 "defparse.y"
{ def_exports (yyvsp[-6].id, yyvsp[-5].id, yyvsp[-4].number, yyvsp[-3].number, yyvsp[-2].number, yyvsp[-1].number, yyvsp[0].number);}
    break;
case 20:
#line 79 "defparse.y"
{ def_import (yyvsp[-6].id,yyvsp[-4].id,yyvsp[-2].id,yyvsp[0].id, 0); }
    break;
case 21:
#line 80 "defparse.y"
{ def_import (yyvsp[-6].id,yyvsp[-4].id,yyvsp[-2].id, 0,yyvsp[0].number); }
    break;
case 22:
#line 81 "defparse.y"
{ def_import (yyvsp[-4].id,yyvsp[-2].id, 0,yyvsp[0].id, 0); }
    break;
case 23:
#line 82 "defparse.y"
{ def_import (yyvsp[-4].id,yyvsp[-2].id, 0, 0,yyvsp[0].number); }
    break;
case 24:
#line 83 "defparse.y"
{ def_import ( 0,yyvsp[-4].id,yyvsp[-2].id,yyvsp[0].id, 0); }
    break;
case 25:
#line 84 "defparse.y"
{ def_import ( 0,yyvsp[-4].id,yyvsp[-2].id, 0,yyvsp[0].number); }
    break;
case 26:
#line 85 "defparse.y"
{ def_import ( 0,yyvsp[-2].id, 0,yyvsp[0].id, 0); }
    break;
case 27:
#line 86 "defparse.y"
{ def_import ( 0,yyvsp[-2].id, 0, 0,yyvsp[0].number); }
    break;
case 30:
#line 95 "defparse.y"
{ def_section (yyvsp[-1].id,yyvsp[0].number);}
    break;
case 35:
#line 107 "defparse.y"
{ yyval.number=yyvsp[0].number;}
    break;
case 36:
#line 108 "defparse.y"
{ yyval.number=-1;}
    break;
case 37:
#line 112 "defparse.y"
{ yyval.number = 1; }
    break;
case 38:
#line 113 "defparse.y"
{ yyval.number = 2; }
    break;
case 39:
#line 114 "defparse.y"
{ yyval.number = 4; }
    break;
case 40:
#line 115 "defparse.y"
{ yyval.number = 8; }
    break;
case 41:
#line 116 "defparse.y"
{ yyval.number = 0; }
    break;
case 42:
#line 117 "defparse.y"
{ yyval.number = 0; }
    break;
case 43:
#line 118 "defparse.y"
{ yyval.number = 0; }
    break;
case 44:
#line 122 "defparse.y"
{yyval.number=1;}
    break;
case 45:
#line 123 "defparse.y"
{yyval.number=0;}
    break;
case 46:
#line 127 "defparse.y"
{yyval.number=1;}
    break;
case 47:
#line 128 "defparse.y"
{yyval.number=0;}
    break;
case 48:
#line 132 "defparse.y"
{ yyval.number = 1; }
    break;
case 49:
#line 133 "defparse.y"
{ yyval.number = 0; }
    break;
case 50:
#line 137 "defparse.y"
{ yyval.number = 1; }
    break;
case 51:
#line 138 "defparse.y"
{ yyval.number = 0; }
    break;
case 52:
#line 141 "defparse.y"
{ yyval.id =yyvsp[0].id; }
    break;
case 53:
#line 143 "defparse.y"
{ 
	    char *name = xmalloc (strlen (yyvsp[-2].id) + 1 + strlen (yyvsp[0].id) + 1);
	    sprintf (name, "%s.%s", yyvsp[-2].id, yyvsp[0].id);
	    yyval.id = name;
	  }
    break;
case 54:
#line 148 "defparse.y"
{ yyval.id=""; }
    break;
case 55:
#line 152 "defparse.y"
{ yyval.number=yyvsp[0].number;}
    break;
case 56:
#line 153 "defparse.y"
{ yyval.number=-1;}
    break;
case 57:
#line 157 "defparse.y"
{ yyval.id = yyvsp[0].id; }
    break;
case 58:
#line 159 "defparse.y"
{ 
	    char *name = xmalloc (strlen (yyvsp[-2].id) + 1 + strlen (yyvsp[0].id) + 1);
	    sprintf (name, "%s.%s", yyvsp[-2].id, yyvsp[0].id);
	    yyval.id = name;
	  }
    break;
case 59:
#line 164 "defparse.y"
{ yyval.id =  0; }
    break;
case 60:
#line 167 "defparse.y"
{ yyval.number= yyvsp[0].number;}
    break;
case 61:
#line 168 "defparse.y"
{ yyval.number=-1;}
    break;
}

#line 706 "/usr/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
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

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 182 "defparse.y"
