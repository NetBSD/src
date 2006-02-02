/* A Bison parser, made from /nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	DR	257
# define	AR	258
# define	FPR	259
# define	FPCR	260
# define	LPC	261
# define	ZAR	262
# define	ZDR	263
# define	LZPC	264
# define	CREG	265
# define	INDEXREG	266
# define	EXPR	267

#line 28 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"


#include "as.h"
#include "tc-m68k.h"
#include "m68k-parse.h"
#include "safe-ctype.h"

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc), as well as gratuitously global symbol names If other parser
   generators (bison, byacc, etc) produce additional global names that
   conflict at link time, then those parser generators need to be
   fixed instead of adding those names to this list.  */

#define	yymaxdepth m68k_maxdepth
#define	yyparse	m68k_parse
#define	yylex	m68k_lex
#define	yyerror	m68k_error
#define	yylval	m68k_lval
#define	yychar	m68k_char
#define	yydebug	m68k_debug
#define	yypact	m68k_pact	
#define	yyr1	m68k_r1			
#define	yyr2	m68k_r2			
#define	yydef	m68k_def		
#define	yychk	m68k_chk		
#define	yypgo	m68k_pgo		
#define	yyact	m68k_act		
#define	yyexca	m68k_exca
#define yyerrflag m68k_errflag
#define yynerrs	m68k_nerrs
#define	yyps	m68k_ps
#define	yypv	m68k_pv
#define	yys	m68k_s
#define	yy_yys	m68k_yys
#define	yystate	m68k_state
#define	yytmp	m68k_tmp
#define	yyv	m68k_v
#define	yy_yyv	m68k_yyv
#define	yyval	m68k_val
#define	yylloc	m68k_lloc
#define yyreds	m68k_reds		/* With YYDEBUG defined */
#define yytoks	m68k_toks		/* With YYDEBUG defined */
#define yylhs	m68k_yylhs
#define yylen	m68k_yylen
#define yydefred m68k_yydefred
#define yydgoto	m68k_yydgoto
#define yysindex m68k_yysindex
#define yyrindex m68k_yyrindex
#define yygindex m68k_yygindex
#define yytable	 m68k_yytable
#define yycheck	 m68k_yycheck

#ifndef YYDEBUG
#define YYDEBUG 1
#endif

/* Internal functions.  */

static enum m68k_register m68k_reg_parse PARAMS ((char **));
static int yylex PARAMS ((void));
static void yyerror PARAMS ((const char *));

/* The parser sets fields pointed to by this global variable.  */
static struct m68k_op *op;


#line 95 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
#ifndef YYSTYPE
typedef union
{
  struct m68k_indexreg indexreg;
  enum m68k_register reg;
  struct m68k_exp exp;
  unsigned long mask;
  int onereg;
  int trailing_ampersand;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		180
#define	YYFLAG		-32768
#define	YYNTBASE	27

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 267 ? yytranslate[x] : 47)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    17,     2,     2,    14,     2,
      18,    19,     2,    20,    22,    21,     2,    26,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      15,     2,    16,     2,    25,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    23,     2,    24,     2,     2,     2,     2,     2,     2,
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
       6,     7,     8,     9,    10,    11,    12,    13
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     2,     5,     8,     9,    11,    14,    17,    19,
      21,    23,    25,    27,    29,    32,    35,    37,    41,    46,
      51,    57,    63,    68,    72,    76,    80,    88,    96,   103,
     109,   116,   122,   129,   135,   141,   146,   156,   164,   173,
     180,   191,   200,   211,   220,   229,   232,   236,   240,   246,
     253,   264,   274,   285,   287,   289,   291,   293,   295,   297,
     299,   301,   303,   305,   307,   309,   311,   313,   314,   316,
     318,   320,   321,   324,   325,   328,   329,   332,   334,   338,
     342,   344,   346,   350,   354,   358,   360,   362,   364
};
static const short yyrhs[] =
{
      29,     0,    30,    28,     0,    31,    28,     0,     0,    14,
       0,    15,    15,     0,    16,    16,     0,     3,     0,     4,
       0,     5,     0,     6,     0,    11,     0,    13,     0,    17,
      13,     0,    14,    13,     0,    43,     0,    18,     4,    19,
       0,    18,     4,    19,    20,     0,    21,    18,     4,    19,
       0,    18,    13,    22,    37,    19,     0,    18,    37,    22,
      13,    19,     0,    13,    18,    37,    19,     0,    18,     7,
      19,     0,    18,     8,    19,     0,    18,    10,    19,     0,
      18,    13,    22,    37,    22,    32,    19,     0,    18,    13,
      22,    37,    22,    39,    19,     0,    18,    13,    22,    33,
      40,    19,     0,    18,    33,    22,    13,    19,     0,    13,
      18,    37,    22,    32,    19,     0,    18,    37,    22,    32,
      19,     0,    13,    18,    37,    22,    39,    19,     0,    18,
      37,    22,    39,    19,     0,    13,    18,    33,    40,    19,
       0,    18,    33,    40,    19,     0,    18,    23,    13,    40,
      24,    22,    32,    41,    19,     0,    18,    23,    13,    40,
      24,    41,    19,     0,    18,    23,    37,    24,    22,    32,
      41,    19,     0,    18,    23,    37,    24,    41,    19,     0,
      18,    23,    13,    22,    37,    22,    32,    24,    41,    19,
       0,    18,    23,    37,    22,    32,    24,    41,    19,     0,
      18,    23,    13,    22,    37,    22,    39,    24,    41,    19,
       0,    18,    23,    37,    22,    39,    24,    41,    19,     0,
      18,    23,    42,    33,    40,    24,    41,    19,     0,    38,
      25,     0,    38,    25,    20,     0,    38,    25,    21,     0,
      38,    25,    18,    13,    19,     0,    38,    25,    18,    42,
      32,    19,     0,    38,    25,    18,    13,    19,    25,    18,
      42,    32,    19,     0,    38,    25,    18,    13,    19,    25,
      18,    13,    19,     0,    38,    25,    18,    42,    32,    19,
      25,    18,    13,    19,     0,    12,     0,    34,     0,    12,
       0,    35,     0,    35,     0,     4,     0,     8,     0,     3,
       0,     9,     0,     4,     0,     7,     0,    36,     0,    10,
       0,     8,     0,     0,    37,     0,     7,     0,    10,     0,
       0,    22,    37,     0,     0,    22,    13,     0,     0,    13,
      22,     0,    45,     0,    45,    26,    44,     0,    46,    26,
      44,     0,    46,     0,    45,     0,    45,    26,    44,     0,
      46,    26,    44,     0,    46,    21,    46,     0,     3,     0,
       4,     0,     5,     0,     6,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   120,   122,   126,   133,   136,   142,   148,   153,   158,
     163,   168,   173,   178,   183,   188,   193,   205,   211,   216,
     221,   231,   241,   251,   256,   261,   266,   273,   284,   291,
     297,   304,   310,   321,   331,   338,   344,   352,   359,   366,
     372,   380,   387,   399,   410,   422,   431,   439,   447,   457,
     464,   472,   479,   492,   494,   506,   508,   519,   521,   522,
     527,   529,   534,   536,   542,   544,   545,   550,   555,   560,
     562,   567,   572,   580,   586,   594,   600,   608,   610,   614,
     625,   630,   631,   635,   641,   651,   656,   660,   664
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "DR", "AR", "FPR", "FPCR", "LPC", "ZAR", 
  "ZDR", "LZPC", "CREG", "INDEXREG", "EXPR", "'&'", "'<'", "'>'", "'#'", 
  "'('", "')'", "'+'", "'-'", "','", "'['", "']'", "'@'", "'/'", 
  "operand", "optional_ampersand", "generic_operand", "motorola_operand", 
  "mit_operand", "zireg", "zdireg", "zadr", "zdr", "apc", "zapc", 
  "optzapc", "zpc", "optczapc", "optcexpr", "optexprc", "reglist", 
  "ireglist", "reglistpair", "reglistreg", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    27,    27,    27,    28,    28,    29,    29,    29,    29,
      29,    29,    29,    29,    29,    29,    29,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    31,    31,    31,    31,    31,
      31,    31,    31,    32,    32,    33,    33,    34,    34,    34,
      35,    35,    36,    36,    37,    37,    37,    38,    38,    39,
      39,    40,    40,    41,    41,    42,    42,    43,    43,    43,
      44,    44,    44,    44,    45,    46,    46,    46,    46
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     1,     2,     2,     0,     1,     2,     2,     1,     1,
       1,     1,     1,     1,     2,     2,     1,     3,     4,     4,
       5,     5,     4,     3,     3,     3,     7,     7,     6,     5,
       6,     5,     6,     5,     5,     4,     9,     7,     8,     6,
      10,     8,    10,     8,     8,     2,     3,     3,     5,     6,
      10,     9,    10,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     1,     1,
       1,     0,     2,     0,     2,     0,     2,     1,     3,     3,
       1,     1,     3,     3,     3,     1,     1,     1,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
      67,    85,    86,    87,    88,    63,    66,    65,    12,    13,
       0,     0,     0,     0,     0,     0,     1,     4,     4,    64,
      68,     0,    16,    77,     0,     0,    15,     6,     7,    14,
      60,    62,    63,    66,    61,    65,    55,     0,    75,    71,
      56,     0,     0,     5,     2,     3,    45,     0,     0,     0,
      62,    71,     0,    17,    23,    24,    25,     0,    71,     0,
       0,     0,     0,     0,     0,    75,    46,    47,    85,    86,
      87,    88,    78,    81,    80,    84,    79,     0,     0,    22,
       0,    18,    71,     0,    76,     0,     0,    73,    71,     0,
      72,    35,    58,    69,    59,    70,    53,     0,     0,    54,
      57,     0,    19,     0,     0,     0,     0,    34,     0,     0,
       0,    20,     0,    72,    73,     0,     0,     0,     0,     0,
      29,    21,    31,    33,    48,    76,     0,    82,    83,    30,
      32,    28,     0,     0,     0,     0,     0,    73,    73,    74,
      73,    39,    73,     0,    49,    26,    27,     0,     0,    73,
      37,     0,     0,     0,     0,     0,    75,     0,    73,    73,
       0,    41,    43,    38,    44,     0,     0,     0,     0,     0,
      36,    51,     0,     0,    40,    42,    50,    52,     0,     0,
       0
};

static const short yydefgoto[] =
{
     178,    44,    16,    17,    18,    98,    39,    99,   100,    19,
      90,    21,   101,    62,   118,    60,    22,    72,    73,    74
};

static const short yypact[] =
{
      89,     8,    11,    18,    22,-32768,-32768,-32768,-32768,    12,
      24,    16,    23,    28,    67,    40,-32768,    32,    32,-32768,
  -32768,    37,-32768,    62,   -11,   123,-32768,-32768,-32768,-32768,
  -32768,    54,    70,    82,-32768,    92,-32768,    76,   140,    94,
  -32768,    99,   118,-32768,-32768,-32768,    47,    79,    79,    79,
  -32768,   101,    29,   114,-32768,-32768,-32768,   123,   117,   -10,
      69,   147,   119,   105,   146,   139,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,   141,     0,-32768,-32768,   154,   149,-32768,
     133,-32768,   101,   127,   154,   145,   133,   148,   101,   152,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   153,   155,-32768,
  -32768,   156,-32768,   137,    20,    79,    79,-32768,   157,   158,
     159,-32768,   133,   151,   160,   161,   162,   116,   164,   163,
  -32768,-32768,-32768,-32768,   165,-32768,   169,-32768,-32768,-32768,
  -32768,-32768,   170,   172,   133,   116,   173,   171,   171,-32768,
     171,-32768,   171,   166,   174,-32768,-32768,   176,   177,   171,
  -32768,   167,   175,   178,   179,   183,   168,   185,   171,   171,
     186,-32768,-32768,-32768,-32768,   144,    20,   182,   187,   188,
  -32768,-32768,   189,   190,-32768,-32768,-32768,-32768,   196,   204,
  -32768
};

static const short yypgoto[] =
{
  -32768,   192,-32768,-32768,-32768,   -79,     9,-32768,    -8,-32768,
       2,-32768,   -77,   -38,   -95,   -65,-32768,   -45,   211,     5
};


#define	YYLAST		211


static const short yytable[] =
{
     104,   108,    20,   109,    76,    24,    40,   115,    -8,   116,
      48,    -9,    86,    78,    87,    49,    41,    40,   -10,   136,
      85,    48,   -11,    30,    92,   126,   106,    52,    94,    34,
      25,    27,    96,   132,    51,   133,   -62,    26,   140,    28,
      59,    29,   152,   153,   110,   154,    43,   155,    79,    40,
     119,    80,    40,    75,   160,   147,   149,   148,    42,    83,
     127,   128,    46,   168,   169,    65,    82,    66,    67,    88,
      30,    31,    30,    53,    32,    33,    34,    35,    34,    36,
      37,    36,    68,    69,    70,    71,   113,   172,    47,    54,
      38,   166,     1,     2,     3,     4,     5,     6,    57,     7,
       8,    55,     9,    10,    11,    12,    13,    14,    30,    92,
      15,    56,    93,    94,    34,    95,    61,    96,    97,    30,
      92,    63,    64,    77,    94,    34,    30,    50,    96,   139,
       5,     6,    34,     7,    81,    36,    30,    92,    91,    84,
      93,    94,    34,    95,    50,    96,   111,     5,     6,   112,
       7,    50,   103,    58,     5,     6,   124,     7,    50,   125,
      89,     5,     6,   171,     7,   102,   125,   105,   107,   114,
     117,   120,   121,   134,   122,   123,   129,   130,   131,     0,
     139,   165,   135,   141,   156,   137,   138,   142,   144,   145,
     143,   146,   150,   151,   161,   173,   179,   162,   163,   157,
     158,   159,   164,   167,   180,   170,   174,   175,   176,   177,
      45,    23
};

static const short yycheck[] =
{
      65,    80,     0,    80,    49,     0,    14,    86,     0,    86,
      21,     0,    22,    51,    24,    26,    14,    25,     0,   114,
      58,    21,     0,     3,     4,   104,    26,    25,     8,     9,
      18,    15,    12,   112,    25,   112,    25,    13,   117,    16,
      38,    13,   137,   138,    82,   140,    14,   142,    19,    57,
      88,    22,    60,    48,   149,   134,   135,   134,    18,    57,
     105,   106,    25,   158,   159,    18,    57,    20,    21,    60,
       3,     4,     3,    19,     7,     8,     9,    10,     9,    12,
      13,    12,     3,     4,     5,     6,    84,   166,    26,    19,
      23,   156,     3,     4,     5,     6,     7,     8,    22,    10,
      11,    19,    13,    14,    15,    16,    17,    18,     3,     4,
      21,    19,     7,     8,     9,    10,    22,    12,    13,     3,
       4,    22,     4,    22,     8,     9,     3,     4,    12,    13,
       7,     8,     9,    10,    20,    12,     3,     4,    19,    22,
       7,     8,     9,    10,     4,    12,    19,     7,     8,    22,
      10,     4,    13,    13,     7,     8,    19,    10,     4,    22,
      13,     7,     8,    19,    10,    19,    22,    26,    19,    24,
      22,    19,    19,    22,    19,    19,    19,    19,    19,    -1,
      13,    13,    22,    19,    18,    24,    24,    24,    19,    19,
      25,    19,    19,    22,    19,    13,     0,    19,    19,    25,
      24,    24,    19,    18,     0,    19,    19,    19,    19,    19,
      18,     0
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

case 2:
#line 123 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->trailing_ampersand = yyvsp[0].trailing_ampersand;
		}
    break;
case 3:
#line 127 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->trailing_ampersand = yyvsp[0].trailing_ampersand;
		}
    break;
case 4:
#line 135 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{ yyval.trailing_ampersand = 0; }
    break;
case 5:
#line 137 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{ yyval.trailing_ampersand = 1; }
    break;
case 6:
#line 144 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = LSH;
		}
    break;
case 7:
#line 149 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = RSH;
		}
    break;
case 8:
#line 154 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = DREG;
		  op->reg = yyvsp[0].reg;
		}
    break;
case 9:
#line 159 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = AREG;
		  op->reg = yyvsp[0].reg;
		}
    break;
case 10:
#line 164 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = FPREG;
		  op->reg = yyvsp[0].reg;
		}
    break;
case 11:
#line 169 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = CONTROL;
		  op->reg = yyvsp[0].reg;
		}
    break;
case 12:
#line 174 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = CONTROL;
		  op->reg = yyvsp[0].reg;
		}
    break;
case 13:
#line 179 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = ABSL;
		  op->disp = yyvsp[0].exp;
		}
    break;
case 14:
#line 184 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = IMMED;
		  op->disp = yyvsp[0].exp;
		}
    break;
case 15:
#line 189 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = IMMED;
		  op->disp = yyvsp[0].exp;
		}
    break;
case 16:
#line 194 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = REGLST;
		  op->mask = yyvsp[0].mask;
		}
    break;
case 17:
#line 207 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = AINDR;
		  op->reg = yyvsp[-1].reg;
		}
    break;
case 18:
#line 212 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = AINC;
		  op->reg = yyvsp[-2].reg;
		}
    break;
case 19:
#line 217 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = ADEC;
		  op->reg = yyvsp[-1].reg;
		}
    break;
case 20:
#line 222 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->reg = yyvsp[-1].reg;
		  op->disp = yyvsp[-3].exp;
		  if ((yyvsp[-1].reg >= ZADDR0 && yyvsp[-1].reg <= ZADDR7)
		      || yyvsp[-1].reg == ZPC)
		    op->mode = BASE;
		  else
		    op->mode = DISP;
		}
    break;
case 21:
#line 232 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->reg = yyvsp[-3].reg;
		  op->disp = yyvsp[-1].exp;
		  if ((yyvsp[-3].reg >= ZADDR0 && yyvsp[-3].reg <= ZADDR7)
		      || yyvsp[-3].reg == ZPC)
		    op->mode = BASE;
		  else
		    op->mode = DISP;
		}
    break;
case 22:
#line 242 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->reg = yyvsp[-1].reg;
		  op->disp = yyvsp[-3].exp;
		  if ((yyvsp[-1].reg >= ZADDR0 && yyvsp[-1].reg <= ZADDR7)
		      || yyvsp[-1].reg == ZPC)
		    op->mode = BASE;
		  else
		    op->mode = DISP;
		}
    break;
case 23:
#line 252 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = DISP;
		  op->reg = yyvsp[-1].reg;
		}
    break;
case 24:
#line 257 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-1].reg;
		}
    break;
case 25:
#line 262 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-1].reg;
		}
    break;
case 26:
#line 267 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-3].reg;
		  op->disp = yyvsp[-5].exp;
		  op->index = yyvsp[-1].indexreg;
		}
    break;
case 27:
#line 274 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  if (yyvsp[-3].reg == PC || yyvsp[-3].reg == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = BASE;
		  op->reg = yyvsp[-1].reg;
		  op->disp = yyvsp[-5].exp;
		  op->index.reg = yyvsp[-3].reg;
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		}
    break;
case 28:
#line 285 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-1].reg;
		  op->disp = yyvsp[-4].exp;
		  op->index = yyvsp[-2].indexreg;
		}
    break;
case 29:
#line 292 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->disp = yyvsp[-1].exp;
		  op->index = yyvsp[-3].indexreg;
		}
    break;
case 30:
#line 298 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-3].reg;
		  op->disp = yyvsp[-5].exp;
		  op->index = yyvsp[-1].indexreg;
		}
    break;
case 31:
#line 305 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-3].reg;
		  op->index = yyvsp[-1].indexreg;
		}
    break;
case 32:
#line 311 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  if (yyvsp[-3].reg == PC || yyvsp[-3].reg == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = BASE;
		  op->reg = yyvsp[-1].reg;
		  op->disp = yyvsp[-5].exp;
		  op->index.reg = yyvsp[-3].reg;
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		}
    break;
case 33:
#line 322 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  if (yyvsp[-3].reg == PC || yyvsp[-3].reg == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = BASE;
		  op->reg = yyvsp[-1].reg;
		  op->index.reg = yyvsp[-3].reg;
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		}
    break;
case 34:
#line 332 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-1].reg;
		  op->disp = yyvsp[-4].exp;
		  op->index = yyvsp[-2].indexreg;
		}
    break;
case 35:
#line 339 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-1].reg;
		  op->index = yyvsp[-2].indexreg;
		}
    break;
case 36:
#line 345 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = POST;
		  op->reg = yyvsp[-5].reg;
		  op->disp = yyvsp[-6].exp;
		  op->index = yyvsp[-2].indexreg;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 37:
#line 353 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = POST;
		  op->reg = yyvsp[-3].reg;
		  op->disp = yyvsp[-4].exp;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 38:
#line 360 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = POST;
		  op->reg = yyvsp[-5].reg;
		  op->index = yyvsp[-2].indexreg;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 39:
#line 367 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = POST;
		  op->reg = yyvsp[-3].reg;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 40:
#line 373 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = PRE;
		  op->reg = yyvsp[-5].reg;
		  op->disp = yyvsp[-7].exp;
		  op->index = yyvsp[-3].indexreg;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 41:
#line 381 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = PRE;
		  op->reg = yyvsp[-5].reg;
		  op->index = yyvsp[-3].indexreg;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 42:
#line 388 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  if (yyvsp[-5].reg == PC || yyvsp[-5].reg == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = PRE;
		  op->reg = yyvsp[-3].reg;
		  op->disp = yyvsp[-7].exp;
		  op->index.reg = yyvsp[-5].reg;
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 43:
#line 400 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  if (yyvsp[-5].reg == PC || yyvsp[-5].reg == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = PRE;
		  op->reg = yyvsp[-3].reg;
		  op->index.reg = yyvsp[-5].reg;
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 44:
#line 411 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = PRE;
		  op->reg = yyvsp[-3].reg;
		  op->disp = yyvsp[-5].exp;
		  op->index = yyvsp[-4].indexreg;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 45:
#line 424 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  /* We use optzapc to avoid a shift/reduce conflict.  */
		  if (yyvsp[-1].reg < ADDR0 || yyvsp[-1].reg > ADDR7)
		    yyerror (_("syntax error"));
		  op->mode = AINDR;
		  op->reg = yyvsp[-1].reg;
		}
    break;
case 46:
#line 432 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  /* We use optzapc to avoid a shift/reduce conflict.  */
		  if (yyvsp[-2].reg < ADDR0 || yyvsp[-2].reg > ADDR7)
		    yyerror (_("syntax error"));
		  op->mode = AINC;
		  op->reg = yyvsp[-2].reg;
		}
    break;
case 47:
#line 440 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  /* We use optzapc to avoid a shift/reduce conflict.  */
		  if (yyvsp[-2].reg < ADDR0 || yyvsp[-2].reg > ADDR7)
		    yyerror (_("syntax error"));
		  op->mode = ADEC;
		  op->reg = yyvsp[-2].reg;
		}
    break;
case 48:
#line 448 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->reg = yyvsp[-4].reg;
		  op->disp = yyvsp[-1].exp;
		  if ((yyvsp[-4].reg >= ZADDR0 && yyvsp[-4].reg <= ZADDR7)
		      || yyvsp[-4].reg == ZPC)
		    op->mode = BASE;
		  else
		    op->mode = DISP;
		}
    break;
case 49:
#line 458 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = BASE;
		  op->reg = yyvsp[-5].reg;
		  op->disp = yyvsp[-2].exp;
		  op->index = yyvsp[-1].indexreg;
		}
    break;
case 50:
#line 465 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = POST;
		  op->reg = yyvsp[-9].reg;
		  op->disp = yyvsp[-6].exp;
		  op->index = yyvsp[-1].indexreg;
		  op->odisp = yyvsp[-2].exp;
		}
    break;
case 51:
#line 473 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = POST;
		  op->reg = yyvsp[-8].reg;
		  op->disp = yyvsp[-5].exp;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 52:
#line 480 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  op->mode = PRE;
		  op->reg = yyvsp[-9].reg;
		  op->disp = yyvsp[-6].exp;
		  op->index = yyvsp[-5].indexreg;
		  op->odisp = yyvsp[-1].exp;
		}
    break;
case 54:
#line 495 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.indexreg.reg = yyvsp[0].reg;
		  yyval.indexreg.size = SIZE_UNSPEC;
		  yyval.indexreg.scale = 1;
		}
    break;
case 56:
#line 509 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.indexreg.reg = yyvsp[0].reg;
		  yyval.indexreg.size = SIZE_UNSPEC;
		  yyval.indexreg.scale = 1;
		}
    break;
case 67:
#line 552 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.reg = ZADDR0;
		}
    break;
case 71:
#line 569 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.reg = ZADDR0;
		}
    break;
case 72:
#line 573 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.reg = yyvsp[0].reg;
		}
    break;
case 73:
#line 582 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.exp.exp.X_op = O_absent;
		  yyval.exp.size = SIZE_UNSPEC;
		}
    break;
case 74:
#line 587 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.exp = yyvsp[0].exp;
		}
    break;
case 75:
#line 596 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.exp.exp.X_op = O_absent;
		  yyval.exp.size = SIZE_UNSPEC;
		}
    break;
case 76:
#line 601 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.exp = yyvsp[-1].exp;
		}
    break;
case 78:
#line 611 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.mask = yyvsp[-2].mask | yyvsp[0].mask;
		}
    break;
case 79:
#line 615 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.mask = (1 << yyvsp[-2].onereg) | yyvsp[0].mask;
		}
    break;
case 80:
#line 627 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.mask = 1 << yyvsp[0].onereg;
		}
    break;
case 82:
#line 632 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.mask = yyvsp[-2].mask | yyvsp[0].mask;
		}
    break;
case 83:
#line 636 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.mask = (1 << yyvsp[-2].onereg) | yyvsp[0].mask;
		}
    break;
case 84:
#line 643 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  if (yyvsp[-2].onereg <= yyvsp[0].onereg)
		    yyval.mask = (1 << (yyvsp[0].onereg + 1)) - 1 - ((1 << yyvsp[-2].onereg) - 1);
		  else
		    yyval.mask = (1 << (yyvsp[-2].onereg + 1)) - 1 - ((1 << yyvsp[0].onereg) - 1);
		}
    break;
case 85:
#line 653 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.onereg = yyvsp[0].reg - DATA0;
		}
    break;
case 86:
#line 657 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.onereg = yyvsp[0].reg - ADDR0 + 8;
		}
    break;
case 87:
#line 661 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  yyval.onereg = yyvsp[0].reg - FP0 + 16;
		}
    break;
case 88:
#line 665 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"
{
		  if (yyvsp[0].reg == FPI)
		    yyval.onereg = 24;
		  else if (yyvsp[0].reg == FPS)
		    yyval.onereg = 25;
		  else
		    yyval.onereg = 26;
		}
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
#line 675 "/nevyn/big/src/binutils/binutils-2.16/src.snap/gas/.././gas/config/m68k-parse.y"


/* The string to parse is stored here, and modified by yylex.  */

static char *str;

/* The original string pointer.  */

static char *strorig;

/* If *CCP could be a register, return the register number and advance
   *CCP.  Otherwise don't change *CCP, and return 0.  */

static enum m68k_register
m68k_reg_parse (ccp)
     register char **ccp;
{
  char *start = *ccp;
  char c;
  char *p;
  symbolS *symbolp;

  if (flag_reg_prefix_optional)
    {
      if (*start == REGISTER_PREFIX)
	start++;
      p = start;
    }
  else
    {
      if (*start != REGISTER_PREFIX)
	return 0;
      p = start + 1;
    }

  if (! is_name_beginner (*p))
    return 0;

  p++;
  while (is_part_of_name (*p) && *p != '.' && *p != ':' && *p != '*')
    p++;

  c = *p;
  *p = 0;
  symbolp = symbol_find (start);
  *p = c;

  if (symbolp != NULL && S_GET_SEGMENT (symbolp) == reg_section)
    {
      *ccp = p;
      return S_GET_VALUE (symbolp);
    }

  /* In MRI mode, something like foo.bar can be equated to a register
     name.  */
  while (flag_mri && c == '.')
    {
      ++p;
      while (is_part_of_name (*p) && *p != '.' && *p != ':' && *p != '*')
	p++;
      c = *p;
      *p = '\0';
      symbolp = symbol_find (start);
      *p = c;
      if (symbolp != NULL && S_GET_SEGMENT (symbolp) == reg_section)
	{
	  *ccp = p;
	  return S_GET_VALUE (symbolp);
	}
    }

  return 0;
}

/* The lexer.  */

static int
yylex ()
{
  enum m68k_register reg;
  char *s;
  int parens;
  int c = 0;
  int tail = 0;
  char *hold;

  if (*str == ' ')
    ++str;

  if (*str == '\0')
    return 0;

  /* Various special characters are just returned directly.  */
  switch (*str)
    {
    case '@':
      /* In MRI mode, this can be the start of an octal number.  */
      if (flag_mri)
	{
	  if (ISDIGIT (str[1])
	      || ((str[1] == '+' || str[1] == '-')
		  && ISDIGIT (str[2])))
	    break;
	}
      /* Fall through.  */
    case '#':
    case '&':
    case ',':
    case ')':
    case '/':
    case '[':
    case ']':
    case '<':
    case '>':
      return *str++;
    case '+':
      /* It so happens that a '+' can only appear at the end of an
	 operand, or if it is trailed by an '&'(see mac load insn).
	 If it appears anywhere else, it must be a unary.  */
      if (str[1] == '\0' || (str[1] == '&' && str[2] == '\0'))
	return *str++;
      break;
    case '-':
      /* A '-' can only appear in -(ar), rn-rn, or ar@-.  If it
         appears anywhere else, it must be a unary minus on an
         expression, unless it it trailed by a '&'(see mac load insn).  */
      if (str[1] == '\0' || (str[1] == '&' && str[2] == '\0'))
	return *str++;
      s = str + 1;
      if (*s == '(')
	++s;
      if (m68k_reg_parse (&s) != 0)
	return *str++;
      break;
    case '(':
      /* A '(' can only appear in `(reg)', `(expr,...', `([', `@(', or
         `)('.  If it appears anywhere else, it must be starting an
         expression.  */
      if (str[1] == '['
	  || (str > strorig
	      && (str[-1] == '@'
		  || str[-1] == ')')))
	return *str++;
      s = str + 1;
      if (m68k_reg_parse (&s) != 0)
	return *str++;
      /* Check for the case of '(expr,...' by scanning ahead.  If we
         find a comma outside of balanced parentheses, we return '('.
         If we find an unbalanced right parenthesis, then presumably
         the '(' really starts an expression.  */
      parens = 0;
      for (s = str + 1; *s != '\0'; s++)
	{
	  if (*s == '(')
	    ++parens;
	  else if (*s == ')')
	    {
	      if (parens == 0)
		break;
	      --parens;
	    }
	  else if (*s == ',' && parens == 0)
	    {
	      /* A comma can not normally appear in an expression, so
		 this is a case of '(expr,...'.  */
	      return *str++;
	    }
	}
    }

  /* See if it's a register.  */

  reg = m68k_reg_parse (&str);
  if (reg != 0)
    {
      int ret;

      yylval.reg = reg;

      if (reg >= DATA0 && reg <= DATA7)
	ret = DR;
      else if (reg >= ADDR0 && reg <= ADDR7)
	ret = AR;
      else if (reg >= FP0 && reg <= FP7)
	return FPR;
      else if (reg == FPI
	       || reg == FPS
	       || reg == FPC)
	return FPCR;
      else if (reg == PC)
	return LPC;
      else if (reg >= ZDATA0 && reg <= ZDATA7)
	ret = ZDR;
      else if (reg >= ZADDR0 && reg <= ZADDR7)
	ret = ZAR;
      else if (reg == ZPC)
	return LZPC;
      else
	return CREG;

      /* If we get here, we have a data or address register.  We
	 must check for a size or scale; if we find one, we must
	 return INDEXREG.  */

      s = str;

      if (*s != '.' && *s != ':' && *s != '*')
	return ret;

      yylval.indexreg.reg = reg;

      if (*s != '.' && *s != ':')
	yylval.indexreg.size = SIZE_UNSPEC;
      else
	{
	  ++s;
	  switch (*s)
	    {
	    case 'w':
	    case 'W':
	      yylval.indexreg.size = SIZE_WORD;
	      ++s;
	      break;
	    case 'l':
	    case 'L':
	      yylval.indexreg.size = SIZE_LONG;
	      ++s;
	      break;
	    default:
	      yyerror (_("illegal size specification"));
	      yylval.indexreg.size = SIZE_UNSPEC;
	      break;
	    }
	}

      yylval.indexreg.scale = 1;

      if (*s == '*' || *s == ':')
	{
	  expressionS scale;

	  ++s;

	  hold = input_line_pointer;
	  input_line_pointer = s;
	  expression (&scale);
	  s = input_line_pointer;
	  input_line_pointer = hold;

	  if (scale.X_op != O_constant)
	    yyerror (_("scale specification must resolve to a number"));
	  else
	    {
	      switch (scale.X_add_number)
		{
		case 1:
		case 2:
		case 4:
		case 8:
		  yylval.indexreg.scale = scale.X_add_number;
		  break;
		default:
		  yyerror (_("invalid scale value"));
		  break;
		}
	    }
	}

      str = s;

      return INDEXREG;
    }

  /* It must be an expression.  Before we call expression, we need to
     look ahead to see if there is a size specification.  We must do
     that first, because otherwise foo.l will be treated as the symbol
     foo.l, rather than as the symbol foo with a long size
     specification.  The grammar requires that all expressions end at
     the end of the operand, or with ',', '(', ']', ')'.  */

  parens = 0;
  for (s = str; *s != '\0'; s++)
    {
      if (*s == '(')
	{
	  if (parens == 0
	      && s > str
	      && (s[-1] == ')' || ISALNUM (s[-1])))
	    break;
	  ++parens;
	}
      else if (*s == ')')
	{
	  if (parens == 0)
	    break;
	  --parens;
	}
      else if (parens == 0
	       && (*s == ',' || *s == ']'))
	break;
    }

  yylval.exp.size = SIZE_UNSPEC;
  if (s <= str + 2
      || (s[-2] != '.' && s[-2] != ':'))
    tail = 0;
  else
    {
      switch (s[-1])
	{
	case 's':
	case 'S':
	case 'b':
	case 'B':
	  yylval.exp.size = SIZE_BYTE;
	  break;
	case 'w':
	case 'W':
	  yylval.exp.size = SIZE_WORD;
	  break;
	case 'l':
	case 'L':
	  yylval.exp.size = SIZE_LONG;
	  break;
	default:
	  break;
	}
      if (yylval.exp.size != SIZE_UNSPEC)
	tail = 2;
    }

#ifdef OBJ_ELF
  {
    /* Look for @PLTPC, etc.  */
    char *cp;

    yylval.exp.pic_reloc = pic_none;
    cp = s - tail;
    if (cp - 6 > str && cp[-6] == '@')
      {
	if (strncmp (cp - 6, "@PLTPC", 6) == 0)
	  {
	    yylval.exp.pic_reloc = pic_plt_pcrel;
	    tail += 6;
	  }
	else if (strncmp (cp - 6, "@GOTPC", 6) == 0)
	  {
	    yylval.exp.pic_reloc = pic_got_pcrel;
	    tail += 6;
	  }
      }
    else if (cp - 4 > str && cp[-4] == '@')
      {
	if (strncmp (cp - 4, "@PLT", 4) == 0)
	  {
	    yylval.exp.pic_reloc = pic_plt_off;
	    tail += 4;
	  }
	else if (strncmp (cp - 4, "@GOT", 4) == 0)
	  {
	    yylval.exp.pic_reloc = pic_got_off;
	    tail += 4;
	  }
      }
  }
#endif

  if (tail != 0)
    {
      c = s[-tail];
      s[-tail] = 0;
    }

  hold = input_line_pointer;
  input_line_pointer = str;
  expression (&yylval.exp.exp);
  str = input_line_pointer;
  input_line_pointer = hold;

  if (tail != 0)
    {
      s[-tail] = c;
      str = s;
    }

  return EXPR;
}

/* Parse an m68k operand.  This is the only function which is called
   from outside this file.  */

int
m68k_ip_op (s, oparg)
     char *s;
     struct m68k_op *oparg;
{
  memset (oparg, 0, sizeof *oparg);
  oparg->error = NULL;
  oparg->index.reg = ZDATA0;
  oparg->index.scale = 1;
  oparg->disp.exp.X_op = O_absent;
  oparg->odisp.exp.X_op = O_absent;

  str = strorig = s;
  op = oparg;

  return yyparse ();
}

/* The error handler.  */

static void
yyerror (s)
     const char *s;
{
  op->error = s;
}
