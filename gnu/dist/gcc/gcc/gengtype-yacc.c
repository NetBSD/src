/* A Bison parser, made from gengtype-yacc.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	ENT_TYPEDEF_STRUCT	257
# define	ENT_STRUCT	258
# define	ENT_EXTERNSTATIC	259
# define	ENT_YACCUNION	260
# define	GTY_TOKEN	261
# define	UNION	262
# define	STRUCT	263
# define	ENUM	264
# define	ALIAS	265
# define	PARAM_IS	266
# define	NUM	267
# define	PERCENTPERCENT	268
# define	SCALAR	269
# define	ID	270
# define	STRING	271
# define	ARRAY	272
# define	PERCENT_ID	273
# define	CHAR	274

#line 22 "gengtype-yacc.y"

#include "hconfig.h"
#include "system.h"
#include "gengtype.h"
#define YYERROR_VERBOSE

#line 29 "gengtype-yacc.y"
#ifndef YYSTYPE
typedef union {
  type_p t;
  pair_p p;
  options_p o;
  const char *s;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		112
#define	YYFLAG		-32768
#define	YYNTBASE	32

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 274 ? yytranslate[x] : 52)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      30,    31,    28,     2,    29,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    27,    23,
      25,    24,    26,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    21,     2,    22,     2,     2,     2,     2,
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
      16,    17,    18,    19,    20
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     4,     7,    10,    11,    20,    21,    29,
      35,    42,    50,    52,    54,    56,    63,    64,    68,    75,
      76,    79,    82,    83,    90,    97,   105,   106,   109,   112,
     114,   116,   119,   125,   128,   134,   137,   140,   146,   147,
     153,   157,   160,   161,   163,   170,   172,   174,   179,   184,
     186,   190,   191
};
static const short yyrhs[] =
{
      -1,    33,    32,     0,    36,    32,     0,    39,    32,     0,
       0,     3,    47,    21,    42,    22,    16,    34,    23,     0,
       0,     4,    47,    21,    42,    22,    35,    23,     0,     5,
      47,    37,    16,    38,     0,     5,    47,    37,    16,    18,
      38,     0,     5,    47,    37,    16,    18,    18,    38,     0,
      44,     0,    23,     0,    24,     0,     6,    47,    42,    22,
      40,    14,     0,     0,    40,    19,    41,     0,    40,    19,
      25,    16,    26,    41,     0,     0,    41,    16,     0,    41,
      20,     0,     0,    44,    46,    16,    43,    23,    42,     0,
      44,    46,    16,    18,    23,    42,     0,    44,    46,    16,
      18,    18,    23,    42,     0,     0,    27,    13,     0,    27,
      16,     0,    15,     0,    16,     0,    44,    28,     0,     9,
      16,    21,    42,    22,     0,     9,    16,     0,     8,    16,
      21,    42,    22,     0,     8,    16,     0,    10,    16,     0,
      10,    16,    21,    45,    22,     0,     0,    16,    24,    13,
      29,    45,     0,    16,    29,    45,     0,    16,    45,     0,
       0,    47,     0,     7,    30,    30,    51,    31,    31,     0,
      11,     0,    12,     0,    48,    30,    44,    31,     0,    16,
      30,    17,    31,     0,    49,     0,    50,    29,    49,     0,
       0,    50,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,    62,    63,    64,    65,    68,    68,    77,    77,    87,
      92,    97,   105,   112,   113,   116,   123,   125,   138,   156,
     158,   169,   182,   183,   193,   203,   215,   216,   217,   220,
     222,   224,   226,   231,   233,   238,   240,   242,   246,   247,
     249,   251,   255,   256,   259,   263,   265,   269,   276,   285,
     290,   297,   298
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "ENT_TYPEDEF_STRUCT", "ENT_STRUCT", 
  "ENT_EXTERNSTATIC", "ENT_YACCUNION", "GTY_TOKEN", "UNION", "STRUCT", 
  "ENUM", "ALIAS", "PARAM_IS", "NUM", "\"%%\"", "SCALAR", "ID", "STRING", 
  "ARRAY", "PERCENT_ID", "CHAR", "'{'", "'}'", "';'", "'='", "'<'", "'>'", 
  "':'", "'*'", "','", "'('", "')'", "start", "typedef_struct", "@1", 
  "@2", "externstatic", "lasttype", "semiequal", "yacc_union", 
  "yacc_typematch", "yacc_ids", "struct_fields", "bitfieldopt", "type", 
  "enum_items", "optionsopt", "options", "type_option", "option", 
  "optionseq", "optionseqopt", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    32,    32,    32,    32,    34,    33,    35,    33,    36,
      36,    36,    37,    38,    38,    39,    40,    40,    40,    41,
      41,    41,    42,    42,    42,    42,    43,    43,    43,    44,
      44,    44,    44,    44,    44,    44,    44,    44,    45,    45,
      45,    45,    46,    46,    47,    48,    48,    49,    49,    50,
      50,    51,    51
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     2,     2,     2,     0,     8,     0,     7,     5,
       6,     7,     1,     1,     1,     6,     0,     3,     6,     0,
       2,     2,     0,     6,     6,     7,     0,     2,     2,     1,
       1,     2,     5,     2,     5,     2,     2,     5,     0,     5,
       3,     2,     0,     1,     6,     1,     1,     4,     4,     1,
       3,     0,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       1,     0,     0,     0,     0,     1,     1,     1,     0,     0,
       0,     0,    22,     2,     3,     4,     0,    22,    22,     0,
       0,     0,    29,    30,     0,    12,     0,    42,    51,     0,
       0,    35,    33,    36,     0,    31,    16,     0,    43,    45,
      46,     0,     0,    49,    52,     0,     0,     7,    22,    22,
      38,     0,    13,    14,     9,     0,    26,     0,     0,     0,
       0,     5,     0,     0,     0,    38,     0,     0,    10,    15,
      19,     0,     0,     0,     0,     0,    50,    44,     0,     8,
      34,    32,     0,    38,    41,    37,    11,     0,    17,     0,
      22,    27,    28,    22,    48,    47,     6,     0,    40,     0,
      20,    21,    22,    24,    23,    38,    19,    25,    39,    18,
       0,     0,     0
};

static const short yydefgoto[] =
{
      13,     5,    78,    62,     6,    24,    54,     7,    55,    88,
      26,    73,    27,    66,    37,     9,    42,    43,    44,    45
};

static const short yypact[] =
{
      54,     7,     7,     7,     7,    54,    54,    54,     8,     4,
       9,    25,    25,-32768,-32768,-32768,    16,    25,    25,    37,
      38,    46,-32768,-32768,    47,    40,     1,     0,    39,    43,
      44,    48,    49,    50,    21,-32768,-32768,    51,-32768,-32768,
  -32768,    42,    45,-32768,    52,    53,    58,-32768,    25,    25,
      61,    24,-32768,-32768,-32768,    -3,    -8,    62,    25,    39,
      55,-32768,    57,    56,    60,    -7,    65,    13,-32768,-32768,
      63,    -5,    -1,    66,    59,    33,-32768,-32768,    68,-32768,
  -32768,-32768,    70,    61,-32768,-32768,-32768,    76,    36,    71,
      25,-32768,-32768,    25,-32768,-32768,-32768,    64,-32768,    69,
  -32768,-32768,    25,-32768,-32768,    61,-32768,-32768,-32768,    36,
      96,    97,-32768
};

static const short yypgoto[] =
{
      20,-32768,-32768,-32768,-32768,-32768,   -43,-32768,-32768,    -6,
     -17,-32768,    -9,   -62,-32768,     2,-32768,    67,-32768,-32768
};


#define	YYLAST		126


static const short yytable[] =
{
      29,    30,    25,    84,    10,    11,    12,     8,    68,    65,
      71,    69,    91,    89,     8,    92,    70,    82,    90,    72,
     110,    98,    83,    36,    86,    17,    14,    15,    35,    38,
      18,    63,    64,    19,    20,    21,    52,    53,    16,    51,
      22,    23,    67,   108,    52,    53,    28,    52,    53,    75,
      39,    40,   100,    31,    32,    41,   101,     1,     2,     3,
       4,    35,    33,    34,    95,    46,    47,    56,    35,    48,
      49,    50,    57,   103,    61,    58,   104,    65,    80,    74,
      79,    59,    81,    97,    60,   107,    77,    85,    87,    93,
      94,    96,    99,   105,   102,   106,   111,   112,     0,     0,
     109,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76
};

static const short yycheck[] =
{
      17,    18,    11,    65,     2,     3,     4,     7,    51,    16,
      18,    14,    13,    18,     7,    16,    19,    24,    23,    27,
       0,    83,    29,    22,    67,    21,     6,     7,    28,    27,
      21,    48,    49,     8,     9,    10,    23,    24,    30,    18,
      15,    16,    18,   105,    23,    24,    30,    23,    24,    58,
      11,    12,    16,    16,    16,    16,    20,     3,     4,     5,
       6,    28,    16,    16,    31,    22,    22,    16,    28,    21,
      21,    21,    30,    90,    16,    30,    93,    16,    22,    17,
      23,    29,    22,    13,    31,   102,    31,    22,    25,    23,
      31,    23,    16,    29,    23,    26,     0,     0,    -1,    -1,
     106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    59
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

#line 315 "/usr/share/bison/bison.simple"


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

case 5:
#line 69 "gengtype-yacc.y"
{
		     new_structure (yyvsp[-5].t->u.s.tag, UNION_P (yyvsp[-5].t), &lexer_line,
				    yyvsp[-2].p, yyvsp[-4].o);
		     do_typedef (yyvsp[0].s, yyvsp[-5].t, &lexer_line);
		     lexer_toplevel_done = 1;
		   ;
    break;}
case 6:
#line 76 "gengtype-yacc.y"
{;
    break;}
case 7:
#line 78 "gengtype-yacc.y"
{
		     new_structure (yyvsp[-4].t->u.s.tag, UNION_P (yyvsp[-4].t), &lexer_line,
				    yyvsp[-1].p, yyvsp[-3].o);
		     lexer_toplevel_done = 1;
		   ;
    break;}
case 8:
#line 84 "gengtype-yacc.y"
{;
    break;}
case 9:
#line 88 "gengtype-yacc.y"
{
	           note_variable (yyvsp[-1].s, adjust_field_type (yyvsp[-2].t, yyvsp[-3].o), yyvsp[-3].o,
				  &lexer_line);
	         ;
    break;}
case 10:
#line 93 "gengtype-yacc.y"
{
	           note_variable (yyvsp[-2].s, create_array (yyvsp[-3].t, yyvsp[-1].s),
	      		    yyvsp[-4].o, &lexer_line);
	         ;
    break;}
case 11:
#line 98 "gengtype-yacc.y"
{
	           note_variable (yyvsp[-3].s, create_array (create_array (yyvsp[-4].t, yyvsp[-1].s),
	      				      yyvsp[-2].s),
	      		    yyvsp[-5].o, &lexer_line);
	         ;
    break;}
case 12:
#line 106 "gengtype-yacc.y"
{
	      lexer_toplevel_done = 1;
	      yyval.t = yyvsp[0].t;
	    ;
    break;}
case 15:
#line 118 "gengtype-yacc.y"
{
	        note_yacc_type (yyvsp[-4].o, yyvsp[-3].p, yyvsp[-1].p, &lexer_line);
	      ;
    break;}
case 16:
#line 124 "gengtype-yacc.y"
{ yyval.p = NULL; ;
    break;}
case 17:
#line 126 "gengtype-yacc.y"
{
		     pair_p p;
		     for (p = yyvsp[0].p; p->next != NULL; p = p->next)
		       {
		         p->name = NULL;
			 p->type = NULL;
		       }
		     p->name = NULL;
		     p->type = NULL;
		     p->next = yyvsp[-2].p;
		     yyval.p = yyvsp[0].p;
		   ;
    break;}
case 18:
#line 139 "gengtype-yacc.y"
{
		     pair_p p;
		     type_p newtype = NULL;
		     if (strcmp (yyvsp[-4].s, "type") == 0)
		       newtype = (type_p) 1;
		     for (p = yyvsp[0].p; p->next != NULL; p = p->next)
		       {
		         p->name = yyvsp[-2].s;
		         p->type = newtype;
		       }
		     p->name = yyvsp[-2].s;
		     p->next = yyvsp[-5].p;
		     p->type = newtype;
		     yyval.p = yyvsp[0].p;
		   ;
    break;}
case 19:
#line 157 "gengtype-yacc.y"
{ yyval.p = NULL; ;
    break;}
case 20:
#line 159 "gengtype-yacc.y"
{
	  pair_p p = xcalloc (1, sizeof (*p));
	  p->next = yyvsp[-1].p;
	  p->line = lexer_line;
	  p->opt = xmalloc (sizeof (*(p->opt)));
	  p->opt->name = "tag";
	  p->opt->next = NULL;
	  p->opt->info = (char *)yyvsp[0].s;
	  yyval.p = p;
	;
    break;}
case 21:
#line 170 "gengtype-yacc.y"
{
	  pair_p p = xcalloc (1, sizeof (*p));
	  p->next = yyvsp[-1].p;
	  p->line = lexer_line;
	  p->opt = xmalloc (sizeof (*(p->opt)));
	  p->opt->name = "tag";
	  p->opt->next = NULL;
	  p->opt->info = xasprintf ("'%s'", yyvsp[0].s);
	  yyval.p = p;
	;
    break;}
case 22:
#line 182 "gengtype-yacc.y"
{ yyval.p = NULL; ;
    break;}
case 23:
#line 184 "gengtype-yacc.y"
{
	            pair_p p = xmalloc (sizeof (*p));
		    p->type = adjust_field_type (yyvsp[-5].t, yyvsp[-4].o);
		    p->opt = yyvsp[-4].o;
		    p->name = yyvsp[-3].s;
		    p->next = yyvsp[0].p;
		    p->line = lexer_line;
		    yyval.p = p;
		  ;
    break;}
case 24:
#line 194 "gengtype-yacc.y"
{
	            pair_p p = xmalloc (sizeof (*p));
		    p->type = adjust_field_type (create_array (yyvsp[-5].t, yyvsp[-2].s), yyvsp[-4].o);
		    p->opt = yyvsp[-4].o;
		    p->name = yyvsp[-3].s;
		    p->next = yyvsp[0].p;
		    p->line = lexer_line;
		    yyval.p = p;
		  ;
    break;}
case 25:
#line 204 "gengtype-yacc.y"
{
	            pair_p p = xmalloc (sizeof (*p));
		    p->type = create_array (create_array (yyvsp[-6].t, yyvsp[-2].s), yyvsp[-3].s);
		    p->opt = yyvsp[-5].o;
		    p->name = yyvsp[-4].s;
		    p->next = yyvsp[0].p;
		    p->line = lexer_line;
		    yyval.p = p;
		  ;
    break;}
case 29:
#line 221 "gengtype-yacc.y"
{ yyval.t = yyvsp[0].t; ;
    break;}
case 30:
#line 223 "gengtype-yacc.y"
{ yyval.t = resolve_typedef (yyvsp[0].s, &lexer_line); ;
    break;}
case 31:
#line 225 "gengtype-yacc.y"
{ yyval.t = create_pointer (yyvsp[-1].t); ;
    break;}
case 32:
#line 227 "gengtype-yacc.y"
{
	   new_structure (yyvsp[-3].s, 0, &lexer_line, yyvsp[-1].p, NULL);
           yyval.t = find_structure (yyvsp[-3].s, 0);
	 ;
    break;}
case 33:
#line 232 "gengtype-yacc.y"
{ yyval.t = find_structure (yyvsp[0].s, 0); ;
    break;}
case 34:
#line 234 "gengtype-yacc.y"
{
	   new_structure (yyvsp[-3].s, 1, &lexer_line, yyvsp[-1].p, NULL);
           yyval.t = find_structure (yyvsp[-3].s, 1);
	 ;
    break;}
case 35:
#line 239 "gengtype-yacc.y"
{ yyval.t = find_structure (yyvsp[0].s, 1); ;
    break;}
case 36:
#line 241 "gengtype-yacc.y"
{ yyval.t = create_scalar_type (yyvsp[0].s, strlen (yyvsp[0].s)); ;
    break;}
case 37:
#line 243 "gengtype-yacc.y"
{ yyval.t = create_scalar_type (yyvsp[-3].s, strlen (yyvsp[-3].s)); ;
    break;}
case 39:
#line 248 "gengtype-yacc.y"
{ ;
    break;}
case 40:
#line 250 "gengtype-yacc.y"
{ ;
    break;}
case 41:
#line 252 "gengtype-yacc.y"
{ ;
    break;}
case 42:
#line 255 "gengtype-yacc.y"
{ yyval.o = NULL; ;
    break;}
case 43:
#line 256 "gengtype-yacc.y"
{ yyval.o = yyvsp[0].o; ;
    break;}
case 44:
#line 260 "gengtype-yacc.y"
{ yyval.o = yyvsp[-2].o; ;
    break;}
case 45:
#line 264 "gengtype-yacc.y"
{ yyval.s = "ptr_alias"; ;
    break;}
case 46:
#line 266 "gengtype-yacc.y"
{ yyval.s = yyvsp[0].s; ;
    break;}
case 47:
#line 270 "gengtype-yacc.y"
{
	     options_p o = xmalloc (sizeof (*o));
	     o->name = yyvsp[-3].s;
	     o->info = adjust_field_type (yyvsp[-1].t, NULL);
	     yyval.o = o;
	   ;
    break;}
case 48:
#line 277 "gengtype-yacc.y"
{
	     options_p o = xmalloc (sizeof (*o));
	     o->name = yyvsp[-3].s;
	     o->info = (void *)yyvsp[-1].s;
	     yyval.o = o;
	   ;
    break;}
case 49:
#line 286 "gengtype-yacc.y"
{
	        yyvsp[0].o->next = NULL;
		yyval.o = yyvsp[0].o;
	      ;
    break;}
case 50:
#line 291 "gengtype-yacc.y"
{
	        yyvsp[0].o->next = yyvsp[-2].o;
		yyval.o = yyvsp[0].o;
	      ;
    break;}
case 51:
#line 297 "gengtype-yacc.y"
{ yyval.o = NULL; ;
    break;}
case 52:
#line 298 "gengtype-yacc.y"
{ yyval.o = yyvsp[0].o; ;
    break;}
}

#line 705 "/usr/share/bison/bison.simple"


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
#line 300 "gengtype-yacc.y"

