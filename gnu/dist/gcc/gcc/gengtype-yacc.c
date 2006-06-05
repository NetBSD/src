/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ENT_TYPEDEF_STRUCT = 258,
     ENT_STRUCT = 259,
     ENT_EXTERNSTATIC = 260,
     ENT_YACCUNION = 261,
     GTY_TOKEN = 262,
     UNION = 263,
     STRUCT = 264,
     ENUM = 265,
     ALIAS = 266,
     PARAM_IS = 267,
     NUM = 268,
     PERCENTPERCENT = 269,
     SCALAR = 270,
     ID = 271,
     STRING = 272,
     ARRAY = 273,
     PERCENT_ID = 274,
     CHAR = 275
   };
#endif
#define ENT_TYPEDEF_STRUCT 258
#define ENT_STRUCT 259
#define ENT_EXTERNSTATIC 260
#define ENT_YACCUNION 261
#define GTY_TOKEN 262
#define UNION 263
#define STRUCT 264
#define ENUM 265
#define ALIAS 266
#define PARAM_IS 267
#define NUM 268
#define PERCENTPERCENT 269
#define SCALAR 270
#define ID 271
#define STRING 272
#define ARRAY 273
#define PERCENT_ID 274
#define CHAR 275




/* Copy the first part of user declarations.  */
#line 22 "gengtype-yacc.y"

#include "hconfig.h"
#include "system.h"
#include "gengtype.h"
#define YYERROR_VERBOSE


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 29 "gengtype-yacc.y"
typedef union YYSTYPE {
  type_p t;
  pair_p p;
  options_p o;
  const char *s;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 129 "gengtype-yacc.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 141 "gengtype-yacc.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

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
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  14
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   103

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  32
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  21
/* YYNRULES -- Number of rules. */
#define YYNRULES  53
/* YYNRULES -- Number of states. */
#define YYNSTATES  112

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   275

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    13,    14,    23,    24,
      32,    38,    45,    53,    55,    57,    59,    66,    67,    71,
      78,    79,    82,    85,    86,    93,   100,   108,   109,   112,
     115,   117,   119,   122,   128,   131,   137,   140,   143,   149,
     150,   156,   160,   163,   164,   166,   173,   175,   177,   182,
     187,   189,   193,   194
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      33,     0,    -1,    -1,    34,    33,    -1,    37,    33,    -1,
      40,    33,    -1,    -1,     3,    48,    21,    43,    22,    16,
      35,    23,    -1,    -1,     4,    48,    21,    43,    22,    36,
      23,    -1,     5,    48,    38,    16,    39,    -1,     5,    48,
      38,    16,    18,    39,    -1,     5,    48,    38,    16,    18,
      18,    39,    -1,    45,    -1,    23,    -1,    24,    -1,     6,
      48,    43,    22,    41,    14,    -1,    -1,    41,    19,    42,
      -1,    41,    19,    25,    16,    26,    42,    -1,    -1,    42,
      16,    -1,    42,    20,    -1,    -1,    45,    47,    16,    44,
      23,    43,    -1,    45,    47,    16,    18,    23,    43,    -1,
      45,    47,    16,    18,    18,    23,    43,    -1,    -1,    27,
      13,    -1,    27,    16,    -1,    15,    -1,    16,    -1,    45,
      28,    -1,     9,    16,    21,    43,    22,    -1,     9,    16,
      -1,     8,    16,    21,    43,    22,    -1,     8,    16,    -1,
      10,    16,    -1,    10,    16,    21,    46,    22,    -1,    -1,
      16,    24,    13,    29,    46,    -1,    16,    29,    46,    -1,
      16,    46,    -1,    -1,    48,    -1,     7,    30,    30,    52,
      31,    31,    -1,    11,    -1,    12,    -1,    49,    30,    45,
      31,    -1,    16,    30,    17,    31,    -1,    50,    -1,    51,
      29,    50,    -1,    -1,    51,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,    62,    62,    63,    64,    65,    69,    68,    78,    77,
      87,    92,    97,   105,   112,   113,   116,   124,   125,   138,
     157,   158,   169,   182,   183,   193,   203,   215,   216,   217,
     220,   222,   224,   226,   231,   233,   238,   240,   242,   246,
     247,   249,   251,   255,   256,   259,   263,   265,   269,   276,
     285,   290,   297,   298
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ENT_TYPEDEF_STRUCT", "ENT_STRUCT", 
  "ENT_EXTERNSTATIC", "ENT_YACCUNION", "GTY_TOKEN", "UNION", "STRUCT", 
  "ENUM", "ALIAS", "PARAM_IS", "NUM", "\"%%\"", "SCALAR", "ID", "STRING", 
  "ARRAY", "PERCENT_ID", "CHAR", "'{'", "'}'", "';'", "'='", "'<'", "'>'", 
  "':'", "'*'", "','", "'('", "')'", "$accept", "start", "typedef_struct", 
  "@1", "@2", "externstatic", "lasttype", "semiequal", "yacc_union", 
  "yacc_typematch", "yacc_ids", "struct_fields", "bitfieldopt", "type", 
  "enum_items", "optionsopt", "options", "type_option", "option", 
  "optionseq", "optionseqopt", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   123,   125,    59,    61,    60,    62,    58,    42,    44,
      40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    32,    33,    33,    33,    33,    35,    34,    36,    34,
      37,    37,    37,    38,    39,    39,    40,    41,    41,    41,
      42,    42,    42,    43,    43,    43,    43,    44,    44,    44,
      45,    45,    45,    45,    45,    45,    45,    45,    45,    46,
      46,    46,    46,    47,    47,    48,    49,    49,    50,    50,
      51,    51,    52,    52
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     2,     2,     0,     8,     0,     7,
       5,     6,     7,     1,     1,     1,     6,     0,     3,     6,
       0,     2,     2,     0,     6,     6,     7,     0,     2,     2,
       1,     1,     2,     5,     2,     5,     2,     2,     5,     0,
       5,     3,     2,     0,     1,     6,     1,     1,     4,     4,
       1,     3,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     0,     0,     0,     0,     2,     2,     2,     0,
       0,     0,     0,    23,     1,     3,     4,     5,     0,    23,
      23,     0,     0,     0,    30,    31,     0,    13,     0,    43,
      52,     0,     0,    36,    34,    37,     0,    32,    17,     0,
      44,    46,    47,     0,     0,    50,    53,     0,     0,     8,
      23,    23,    39,     0,    14,    15,    10,     0,    27,     0,
       0,     0,     0,     6,     0,     0,     0,    39,     0,     0,
      11,    16,    20,     0,     0,     0,     0,     0,    51,    45,
       0,     9,    35,    33,     0,    39,    42,    38,    12,     0,
      18,     0,    23,    28,    29,    23,    49,    48,     7,     0,
      41,     0,    21,    22,    23,    25,    24,    39,    20,    26,
      40,    19
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     5,     6,    80,    64,     7,    26,    56,     8,    57,
      90,    28,    75,    29,    68,    39,    10,    44,    45,    46,
      47
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -65
static const yysigned_char yypact[] =
{
      48,     5,     5,     5,     5,    19,    48,    48,    48,    -2,
      39,    40,     7,     7,   -65,   -65,   -65,   -65,    15,     7,
       7,    11,    46,    47,   -65,   -65,    50,    36,    43,    -3,
      25,    45,    52,    51,    54,    56,     6,   -65,   -65,    53,
     -65,   -65,   -65,    38,    41,   -65,    55,    57,    62,   -65,
       7,     7,    63,    16,   -65,   -65,   -65,    -5,    -7,    64,
       7,    25,    58,   -65,    59,    61,    65,   -11,    68,    35,
     -65,   -65,    66,    24,    22,    69,    67,    18,   -65,   -65,
      70,   -65,   -65,   -65,    73,    63,   -65,   -65,   -65,    78,
      28,    72,     7,   -65,   -65,     7,   -65,   -65,   -65,    71,
     -65,    44,   -65,   -65,     7,   -65,   -65,    63,   -65,   -65,
     -65,    28
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -65,    49,   -65,   -65,   -65,   -65,   -65,   -43,   -65,   -65,
     -28,   -19,   -65,   -10,   -64,   -65,     4,   -65,    42,   -65,
     -65
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      31,    32,    27,    86,     9,    67,    11,    12,    13,    71,
      70,    73,     9,    84,    72,    21,    22,    23,    85,    14,
      74,   100,    24,    25,    53,    37,    88,    33,    18,    54,
      55,    65,    66,    40,    69,    93,    41,    42,    94,    54,
      55,    43,    91,   110,   102,    30,    37,    92,   103,    97,
      77,     1,     2,     3,     4,    15,    16,    17,    54,    55,
      19,    20,    34,    35,    37,    38,    36,    48,    59,    58,
     108,    60,    50,   105,    49,    51,   106,    52,    63,    67,
     111,    76,    81,    82,    61,   109,    99,    83,    62,    79,
      87,    89,    95,    98,   101,   104,     0,     0,    96,     0,
     107,     0,     0,    78
};

static const yysigned_char yycheck[] =
{
      19,    20,    12,    67,     7,    16,     2,     3,     4,    14,
      53,    18,     7,    24,    19,     8,     9,    10,    29,     0,
      27,    85,    15,    16,    18,    28,    69,    16,    30,    23,
      24,    50,    51,    29,    18,    13,    11,    12,    16,    23,
      24,    16,    18,   107,    16,    30,    28,    23,    20,    31,
      60,     3,     4,     5,     6,     6,     7,     8,    23,    24,
      21,    21,    16,    16,    28,    22,    16,    22,    30,    16,
      26,    30,    21,    92,    22,    21,    95,    21,    16,    16,
     108,    17,    23,    22,    29,   104,    13,    22,    31,    31,
      22,    25,    23,    23,    16,    23,    -1,    -1,    31,    -1,
      29,    -1,    -1,    61
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     5,     6,    33,    34,    37,    40,     7,
      48,    48,    48,    48,     0,    33,    33,    33,    30,    21,
      21,     8,     9,    10,    15,    16,    38,    45,    43,    45,
      30,    43,    43,    16,    16,    16,    16,    28,    22,    47,
      48,    11,    12,    16,    49,    50,    51,    52,    22,    22,
      21,    21,    21,    18,    23,    24,    39,    41,    16,    30,
      30,    29,    31,    16,    36,    43,    43,    16,    46,    18,
      39,    14,    19,    18,    27,    44,    17,    45,    50,    31,
      35,    23,    22,    22,    24,    29,    46,    22,    39,    25,
      42,    18,    23,    13,    16,    23,    31,    31,    23,    13,
      46,    16,    16,    20,    23,    43,    43,    29,    26,    43,
      46,    42
};

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
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
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
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

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

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
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



#if YYERROR_VERBOSE

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

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
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

  if (yyss + yystacksize - 1 <= yyssp)
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
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


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

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 6:
#line 69 "gengtype-yacc.y"
    {
		     new_structure (yyvsp[-5].t->u.s.tag, UNION_P (yyvsp[-5].t), &lexer_line,
				    yyvsp[-2].p, yyvsp[-4].o);
		     do_typedef (yyvsp[0].s, yyvsp[-5].t, &lexer_line);
		     lexer_toplevel_done = 1;
		   ;}
    break;

  case 7:
#line 76 "gengtype-yacc.y"
    {;}
    break;

  case 8:
#line 78 "gengtype-yacc.y"
    {
		     new_structure (yyvsp[-4].t->u.s.tag, UNION_P (yyvsp[-4].t), &lexer_line,
				    yyvsp[-1].p, yyvsp[-3].o);
		     lexer_toplevel_done = 1;
		   ;}
    break;

  case 9:
#line 84 "gengtype-yacc.y"
    {;}
    break;

  case 10:
#line 88 "gengtype-yacc.y"
    {
	           note_variable (yyvsp[-1].s, adjust_field_type (yyvsp[-2].t, yyvsp[-3].o), yyvsp[-3].o,
				  &lexer_line);
	         ;}
    break;

  case 11:
#line 93 "gengtype-yacc.y"
    {
	           note_variable (yyvsp[-2].s, create_array (yyvsp[-3].t, yyvsp[-1].s),
	      		    yyvsp[-4].o, &lexer_line);
	         ;}
    break;

  case 12:
#line 98 "gengtype-yacc.y"
    {
	           note_variable (yyvsp[-3].s, create_array (create_array (yyvsp[-4].t, yyvsp[-1].s),
	      				      yyvsp[-2].s),
	      		    yyvsp[-5].o, &lexer_line);
	         ;}
    break;

  case 13:
#line 106 "gengtype-yacc.y"
    {
	      lexer_toplevel_done = 1;
	      yyval.t = yyvsp[0].t;
	    ;}
    break;

  case 16:
#line 118 "gengtype-yacc.y"
    {
	        note_yacc_type (yyvsp[-4].o, yyvsp[-3].p, yyvsp[-1].p, &lexer_line);
	      ;}
    break;

  case 17:
#line 124 "gengtype-yacc.y"
    { yyval.p = NULL; ;}
    break;

  case 18:
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
		   ;}
    break;

  case 19:
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
		   ;}
    break;

  case 20:
#line 157 "gengtype-yacc.y"
    { yyval.p = NULL; ;}
    break;

  case 21:
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
	;}
    break;

  case 22:
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
	;}
    break;

  case 23:
#line 182 "gengtype-yacc.y"
    { yyval.p = NULL; ;}
    break;

  case 24:
#line 184 "gengtype-yacc.y"
    {
	            pair_p p = xmalloc (sizeof (*p));
		    p->type = adjust_field_type (yyvsp[-5].t, yyvsp[-4].o);
		    p->opt = yyvsp[-4].o;
		    p->name = yyvsp[-3].s;
		    p->next = yyvsp[0].p;
		    p->line = lexer_line;
		    yyval.p = p;
		  ;}
    break;

  case 25:
#line 194 "gengtype-yacc.y"
    {
	            pair_p p = xmalloc (sizeof (*p));
		    p->type = adjust_field_type (create_array (yyvsp[-5].t, yyvsp[-2].s), yyvsp[-4].o);
		    p->opt = yyvsp[-4].o;
		    p->name = yyvsp[-3].s;
		    p->next = yyvsp[0].p;
		    p->line = lexer_line;
		    yyval.p = p;
		  ;}
    break;

  case 26:
#line 204 "gengtype-yacc.y"
    {
	            pair_p p = xmalloc (sizeof (*p));
		    p->type = create_array (create_array (yyvsp[-6].t, yyvsp[-2].s), yyvsp[-3].s);
		    p->opt = yyvsp[-5].o;
		    p->name = yyvsp[-4].s;
		    p->next = yyvsp[0].p;
		    p->line = lexer_line;
		    yyval.p = p;
		  ;}
    break;

  case 30:
#line 221 "gengtype-yacc.y"
    { yyval.t = yyvsp[0].t; ;}
    break;

  case 31:
#line 223 "gengtype-yacc.y"
    { yyval.t = resolve_typedef (yyvsp[0].s, &lexer_line); ;}
    break;

  case 32:
#line 225 "gengtype-yacc.y"
    { yyval.t = create_pointer (yyvsp[-1].t); ;}
    break;

  case 33:
#line 227 "gengtype-yacc.y"
    {
	   new_structure (yyvsp[-3].s, 0, &lexer_line, yyvsp[-1].p, NULL);
           yyval.t = find_structure (yyvsp[-3].s, 0);
	 ;}
    break;

  case 34:
#line 232 "gengtype-yacc.y"
    { yyval.t = find_structure (yyvsp[0].s, 0); ;}
    break;

  case 35:
#line 234 "gengtype-yacc.y"
    {
	   new_structure (yyvsp[-3].s, 1, &lexer_line, yyvsp[-1].p, NULL);
           yyval.t = find_structure (yyvsp[-3].s, 1);
	 ;}
    break;

  case 36:
#line 239 "gengtype-yacc.y"
    { yyval.t = find_structure (yyvsp[0].s, 1); ;}
    break;

  case 37:
#line 241 "gengtype-yacc.y"
    { yyval.t = create_scalar_type (yyvsp[0].s, strlen (yyvsp[0].s)); ;}
    break;

  case 38:
#line 243 "gengtype-yacc.y"
    { yyval.t = create_scalar_type (yyvsp[-3].s, strlen (yyvsp[-3].s)); ;}
    break;

  case 40:
#line 248 "gengtype-yacc.y"
    { ;}
    break;

  case 41:
#line 250 "gengtype-yacc.y"
    { ;}
    break;

  case 42:
#line 252 "gengtype-yacc.y"
    { ;}
    break;

  case 43:
#line 255 "gengtype-yacc.y"
    { yyval.o = NULL; ;}
    break;

  case 44:
#line 256 "gengtype-yacc.y"
    { yyval.o = yyvsp[0].o; ;}
    break;

  case 45:
#line 260 "gengtype-yacc.y"
    { yyval.o = yyvsp[-2].o; ;}
    break;

  case 46:
#line 264 "gengtype-yacc.y"
    { yyval.s = "ptr_alias"; ;}
    break;

  case 47:
#line 266 "gengtype-yacc.y"
    { yyval.s = yyvsp[0].s; ;}
    break;

  case 48:
#line 270 "gengtype-yacc.y"
    {
	     options_p o = xmalloc (sizeof (*o));
	     o->name = yyvsp[-3].s;
	     o->info = adjust_field_type (yyvsp[-1].t, NULL);
	     yyval.o = o;
	   ;}
    break;

  case 49:
#line 277 "gengtype-yacc.y"
    {
	     options_p o = xmalloc (sizeof (*o));
	     o->name = yyvsp[-3].s;
	     o->info = (void *)yyvsp[-1].s;
	     yyval.o = o;
	   ;}
    break;

  case 50:
#line 286 "gengtype-yacc.y"
    {
	        yyvsp[0].o->next = NULL;
		yyval.o = yyvsp[0].o;
	      ;}
    break;

  case 51:
#line 291 "gengtype-yacc.y"
    {
	        yyvsp[0].o->next = yyvsp[-2].o;
		yyval.o = yyvsp[0].o;
	      ;}
    break;

  case 52:
#line 297 "gengtype-yacc.y"
    { yyval.o = NULL; ;}
    break;

  case 53:
#line 298 "gengtype-yacc.y"
    { yyval.o = yyvsp[0].o; ;}
    break;


    }

/* Line 991 of yacc.c.  */
#line 1432 "gengtype-yacc.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
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
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__) \
    && !defined __cplusplus
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 300 "gengtype-yacc.y"


