/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

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
     DR = 258,
     AR = 259,
     FPR = 260,
     FPCR = 261,
     LPC = 262,
     ZAR = 263,
     ZDR = 264,
     LZPC = 265,
     CREG = 266,
     INDEXREG = 267,
     EXPR = 268
   };
#endif
/* Tokens.  */
#define DR 258
#define AR 259
#define FPR 260
#define FPCR 261
#define LPC 262
#define ZAR 263
#define ZDR 264
#define LZPC 265
#define CREG 266
#define INDEXREG 267
#define EXPR 268




/* Copy the first part of user declarations.  */
#line 27 "m68k-parse.y"


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

static enum m68k_register m68k_reg_parse (char **);
static int yylex (void);
static void yyerror (const char *);

/* The parser sets fields pointed to by this global variable.  */
static struct m68k_op *op;



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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 95 "m68k-parse.y"
{
  struct m68k_indexreg indexreg;
  enum m68k_register reg;
  struct m68k_exp exp;
  unsigned long mask;
  int onereg;
  int trailing_ampersand;
}
/* Line 193 of yacc.c.  */
#line 198 "m68k-parse.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 211 "m68k-parse.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
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
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  44
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   215

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  27
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  21
/* YYNRULES -- Number of rules.  */
#define YYNRULES  89
/* YYNRULES -- Number of states.  */
#define YYNSTATES  180

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   268

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     8,    11,    12,    14,    17,    20,
      22,    24,    26,    28,    30,    32,    35,    38,    40,    44,
      49,    54,    60,    66,    71,    75,    79,    83,    91,    99,
     106,   112,   119,   125,   132,   138,   144,   149,   159,   167,
     176,   183,   194,   203,   214,   223,   232,   235,   239,   243,
     249,   256,   267,   277,   288,   290,   292,   294,   296,   298,
     300,   302,   304,   306,   308,   310,   312,   314,   316,   317,
     319,   321,   323,   324,   327,   328,   331,   332,   335,   337,
     341,   345,   347,   349,   353,   357,   361,   363,   365,   367
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      28,     0,    -1,    30,    -1,    31,    29,    -1,    32,    29,
      -1,    -1,    14,    -1,    15,    15,    -1,    16,    16,    -1,
       3,    -1,     4,    -1,     5,    -1,     6,    -1,    11,    -1,
      13,    -1,    17,    13,    -1,    14,    13,    -1,    44,    -1,
      18,     4,    19,    -1,    18,     4,    19,    20,    -1,    21,
      18,     4,    19,    -1,    18,    13,    22,    38,    19,    -1,
      18,    38,    22,    13,    19,    -1,    13,    18,    38,    19,
      -1,    18,     7,    19,    -1,    18,     8,    19,    -1,    18,
      10,    19,    -1,    18,    13,    22,    38,    22,    33,    19,
      -1,    18,    13,    22,    38,    22,    40,    19,    -1,    18,
      13,    22,    34,    41,    19,    -1,    18,    34,    22,    13,
      19,    -1,    13,    18,    38,    22,    33,    19,    -1,    18,
      38,    22,    33,    19,    -1,    13,    18,    38,    22,    40,
      19,    -1,    18,    38,    22,    40,    19,    -1,    13,    18,
      34,    41,    19,    -1,    18,    34,    41,    19,    -1,    18,
      23,    13,    41,    24,    22,    33,    42,    19,    -1,    18,
      23,    13,    41,    24,    42,    19,    -1,    18,    23,    38,
      24,    22,    33,    42,    19,    -1,    18,    23,    38,    24,
      42,    19,    -1,    18,    23,    13,    22,    38,    22,    33,
      24,    42,    19,    -1,    18,    23,    38,    22,    33,    24,
      42,    19,    -1,    18,    23,    13,    22,    38,    22,    40,
      24,    42,    19,    -1,    18,    23,    38,    22,    40,    24,
      42,    19,    -1,    18,    23,    43,    34,    41,    24,    42,
      19,    -1,    39,    25,    -1,    39,    25,    20,    -1,    39,
      25,    21,    -1,    39,    25,    18,    13,    19,    -1,    39,
      25,    18,    43,    33,    19,    -1,    39,    25,    18,    13,
      19,    25,    18,    43,    33,    19,    -1,    39,    25,    18,
      13,    19,    25,    18,    13,    19,    -1,    39,    25,    18,
      43,    33,    19,    25,    18,    13,    19,    -1,    12,    -1,
      35,    -1,    12,    -1,    36,    -1,    36,    -1,     4,    -1,
       8,    -1,     3,    -1,     9,    -1,     4,    -1,     7,    -1,
      37,    -1,    10,    -1,     8,    -1,    -1,    38,    -1,     7,
      -1,    10,    -1,    -1,    22,    38,    -1,    -1,    22,    13,
      -1,    -1,    13,    22,    -1,    46,    -1,    46,    26,    45,
      -1,    47,    26,    45,    -1,    47,    -1,    46,    -1,    46,
      26,    45,    -1,    47,    26,    45,    -1,    47,    21,    47,
      -1,     3,    -1,     4,    -1,     5,    -1,     6,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   120,   120,   121,   125,   134,   135,   142,   147,   152,
     157,   162,   167,   172,   177,   182,   187,   192,   205,   210,
     215,   220,   230,   240,   250,   255,   260,   265,   272,   283,
     290,   296,   303,   309,   320,   330,   337,   343,   351,   358,
     365,   371,   379,   386,   398,   409,   422,   430,   438,   446,
     456,   463,   471,   478,   492,   493,   506,   507,   519,   520,
     521,   527,   528,   534,   535,   542,   543,   544,   551,   554,
     560,   561,   568,   571,   581,   585,   595,   599,   608,   609,
     613,   625,   629,   630,   634,   641,   651,   655,   659,   663
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DR", "AR", "FPR", "FPCR", "LPC", "ZAR",
  "ZDR", "LZPC", "CREG", "INDEXREG", "EXPR", "'&'", "'<'", "'>'", "'#'",
  "'('", "')'", "'+'", "'-'", "','", "'['", "']'", "'@'", "'/'", "$accept",
  "operand", "optional_ampersand", "generic_operand", "motorola_operand",
  "mit_operand", "zireg", "zdireg", "zadr", "zdr", "apc", "zapc",
  "optzapc", "zpc", "optczapc", "optcexpr", "optexprc", "reglist",
  "ireglist", "reglistpair", "reglistreg", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,    38,    60,    62,    35,    40,    41,
      43,    45,    44,    91,    93,    64,    47
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    27,    28,    28,    28,    29,    29,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    31,    31,
      31,    31,    31,    31,    31,    31,    31,    31,    31,    31,
      31,    31,    31,    31,    31,    31,    31,    31,    31,    31,
      31,    31,    31,    31,    31,    31,    32,    32,    32,    32,
      32,    32,    32,    32,    33,    33,    34,    34,    35,    35,
      35,    36,    36,    37,    37,    38,    38,    38,    39,    39,
      40,    40,    41,    41,    42,    42,    43,    43,    44,    44,
      44,    45,    45,    45,    45,    46,    47,    47,    47,    47
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     0,     1,     2,     2,     1,
       1,     1,     1,     1,     1,     2,     2,     1,     3,     4,
       4,     5,     5,     4,     3,     3,     3,     7,     7,     6,
       5,     6,     5,     6,     5,     5,     4,     9,     7,     8,
       6,    10,     8,    10,     8,     8,     2,     3,     3,     5,
       6,    10,     9,    10,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       1,     1,     0,     2,     0,     2,     0,     2,     1,     3,
       3,     1,     1,     3,     3,     3,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      68,    86,    87,    88,    89,    64,    67,    66,    13,    14,
       0,     0,     0,     0,     0,     0,     0,     2,     5,     5,
      65,    69,     0,    17,    78,     0,     0,    16,     7,     8,
      15,    61,    63,    64,    67,    62,    66,    56,     0,    76,
      72,    57,     0,     0,     1,     6,     3,     4,    46,     0,
       0,     0,    63,    72,     0,    18,    24,    25,    26,     0,
      72,     0,     0,     0,     0,     0,     0,    76,    47,    48,
      86,    87,    88,    89,    79,    82,    81,    85,    80,     0,
       0,    23,     0,    19,    72,     0,    77,     0,     0,    74,
      72,     0,    73,    36,    59,    70,    60,    71,    54,     0,
       0,    55,    58,     0,    20,     0,     0,     0,     0,    35,
       0,     0,     0,    21,     0,    73,    74,     0,     0,     0,
       0,     0,    30,    22,    32,    34,    49,    77,     0,    83,
      84,    31,    33,    29,     0,     0,     0,     0,     0,    74,
      74,    75,    74,    40,    74,     0,    50,    27,    28,     0,
       0,    74,    38,     0,     0,     0,     0,     0,    76,     0,
      74,    74,     0,    42,    44,    39,    45,     0,     0,     0,
       0,     0,    37,    52,     0,     0,    41,    43,    51,    53
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    16,    46,    17,    18,    19,   100,    40,   101,   102,
      20,    92,    22,   103,    64,   120,    62,    23,    74,    75,
      76
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -98
static const yytype_int16 yypact[] =
{
      89,    14,     9,    31,    35,   -98,   -98,   -98,   -98,     0,
      36,    42,    28,    56,    63,    67,    90,   -98,    75,    75,
     -98,   -98,    86,   -98,    96,   -15,   123,   -98,   -98,   -98,
     -98,   -98,    97,   115,   119,   -98,   120,   -98,   122,    16,
     126,   -98,   127,   157,   -98,   -98,   -98,   -98,    19,   154,
     154,   154,   -98,   140,    29,   144,   -98,   -98,   -98,   123,
     141,    99,    18,    70,   147,   105,   148,   152,   -98,   -98,
     -98,   -98,   -98,   -98,   -98,   142,   -13,   -98,   -98,   146,
     150,   -98,   133,   -98,   140,    60,   146,   149,   133,   153,
     140,   151,   -98,   -98,   -98,   -98,   -98,   -98,   -98,   155,
     158,   -98,   -98,   159,   -98,    62,   143,   154,   154,   -98,
     160,   161,   162,   -98,   133,   163,   164,   165,   166,   116,
     168,   167,   -98,   -98,   -98,   -98,   169,   -98,   173,   -98,
     -98,   -98,   -98,   -98,   174,   176,   133,   116,   177,   175,
     175,   -98,   175,   -98,   175,   170,   178,   -98,   -98,   180,
     181,   175,   -98,   171,   179,   182,   183,   187,   186,   189,
     175,   175,   190,   -98,   -98,   -98,   -98,    79,   143,   195,
     191,   192,   -98,   -98,   193,   194,   -98,   -98,   -98,   -98
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -98,   -98,   196,   -98,   -98,   -98,   -81,     6,   -98,    -9,
     -98,     2,   -98,   -78,   -38,   -97,   -67,   -98,   -48,   172,
      12
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -64
static const yytype_int16 yytable[] =
{
     106,   110,    21,    78,   111,    41,    50,   117,    50,   -10,
     118,    51,    25,   108,    -9,    80,    42,    41,    26,   138,
      52,    31,    87,     5,     6,   128,     7,    35,    54,    60,
      37,   -11,    53,   134,   -63,   -12,   135,    67,   142,    68,
      69,    61,   154,   155,    29,   156,   112,   157,    81,    27,
      41,    82,   121,    41,   162,   149,   151,    28,   150,   129,
     130,    85,    77,   170,   171,    84,    31,    32,    90,    30,
      33,    34,    35,    36,    52,    37,    38,     5,     6,   113,
       7,   126,   114,    91,   127,    43,    39,   174,   115,    45,
      44,   168,     1,     2,     3,     4,     5,     6,   173,     7,
       8,   127,     9,    10,    11,    12,    13,    14,    31,    94,
      15,    48,    95,    96,    35,    97,    55,    98,    99,    31,
      94,    88,    49,    89,    96,    35,    31,    52,    98,   141,
       5,     6,    35,     7,    56,    37,    31,    94,    57,    58,
      95,    96,    35,    97,    59,    98,    31,    94,    63,    65,
      52,    96,    35,     5,     6,    98,     7,    70,    71,    72,
      73,    66,    79,    86,    83,   105,    93,   104,   107,   109,
     122,     0,    24,   116,   123,   119,     0,   124,   125,   131,
     132,   133,     0,     0,   141,   136,   137,   143,   158,   139,
     140,   144,   146,   147,   145,   148,   152,   153,   163,   167,
       0,   164,   165,   159,   160,   161,   166,   169,   175,   172,
     176,   177,   178,   179,     0,    47
};

static const yytype_int16 yycheck[] =
{
      67,    82,     0,    51,    82,    14,    21,    88,    21,     0,
      88,    26,     0,    26,     0,    53,    14,    26,    18,   116,
       4,     3,    60,     7,     8,   106,    10,     9,    26,    13,
      12,     0,    26,   114,    25,     0,   114,    18,   119,    20,
      21,    39,   139,   140,    16,   142,    84,   144,    19,    13,
      59,    22,    90,    62,   151,   136,   137,    15,   136,   107,
     108,    59,    50,   160,   161,    59,     3,     4,    62,    13,
       7,     8,     9,    10,     4,    12,    13,     7,     8,    19,
      10,    19,    22,    13,    22,    18,    23,   168,    86,    14,
       0,   158,     3,     4,     5,     6,     7,     8,    19,    10,
      11,    22,    13,    14,    15,    16,    17,    18,     3,     4,
      21,    25,     7,     8,     9,    10,    19,    12,    13,     3,
       4,    22,    26,    24,     8,     9,     3,     4,    12,    13,
       7,     8,     9,    10,    19,    12,     3,     4,    19,    19,
       7,     8,     9,    10,    22,    12,     3,     4,    22,    22,
       4,     8,     9,     7,     8,    12,    10,     3,     4,     5,
       6,     4,    22,    22,    20,    13,    19,    19,    26,    19,
      19,    -1,     0,    24,    19,    22,    -1,    19,    19,    19,
      19,    19,    -1,    -1,    13,    22,    22,    19,    18,    24,
      24,    24,    19,    19,    25,    19,    19,    22,    19,    13,
      -1,    19,    19,    25,    24,    24,    19,    18,    13,    19,
      19,    19,    19,    19,    -1,    19
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    10,    11,    13,
      14,    15,    16,    17,    18,    21,    28,    30,    31,    32,
      37,    38,    39,    44,    46,    47,    18,    13,    15,    16,
      13,     3,     4,     7,     8,     9,    10,    12,    13,    23,
      34,    36,    38,    18,     0,    14,    29,    29,    25,    26,
      21,    26,     4,    34,    38,    19,    19,    19,    19,    22,
      13,    38,    43,    22,    41,    22,     4,    18,    20,    21,
       3,     4,     5,     6,    45,    46,    47,    47,    45,    22,
      41,    19,    22,    20,    34,    38,    22,    41,    22,    24,
      34,    13,    38,    19,     4,     7,     8,    10,    12,    13,
      33,    35,    36,    40,    19,    13,    43,    26,    26,    19,
      33,    40,    41,    19,    22,    38,    24,    33,    40,    22,
      42,    41,    19,    19,    19,    19,    19,    22,    33,    45,
      45,    19,    19,    19,    33,    40,    22,    22,    42,    24,
      24,    13,    33,    19,    24,    25,    19,    19,    19,    33,
      40,    33,    19,    22,    42,    42,    42,    42,    18,    25,
      24,    24,    42,    19,    19,    19,    19,    13,    43,    18,
      42,    42,    19,    19,    33,    13,    19,    19,    19,    19
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
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

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
        case 3:
#line 122 "m68k-parse.y"
    {
		  op->trailing_ampersand = (yyvsp[(2) - (2)].trailing_ampersand);
		}
    break;

  case 4:
#line 126 "m68k-parse.y"
    {
		  op->trailing_ampersand = (yyvsp[(2) - (2)].trailing_ampersand);
		}
    break;

  case 5:
#line 134 "m68k-parse.y"
    { (yyval.trailing_ampersand) = 0; }
    break;

  case 6:
#line 136 "m68k-parse.y"
    { (yyval.trailing_ampersand) = 1; }
    break;

  case 7:
#line 143 "m68k-parse.y"
    {
		  op->mode = LSH;
		}
    break;

  case 8:
#line 148 "m68k-parse.y"
    {
		  op->mode = RSH;
		}
    break;

  case 9:
#line 153 "m68k-parse.y"
    {
		  op->mode = DREG;
		  op->reg = (yyvsp[(1) - (1)].reg);
		}
    break;

  case 10:
#line 158 "m68k-parse.y"
    {
		  op->mode = AREG;
		  op->reg = (yyvsp[(1) - (1)].reg);
		}
    break;

  case 11:
#line 163 "m68k-parse.y"
    {
		  op->mode = FPREG;
		  op->reg = (yyvsp[(1) - (1)].reg);
		}
    break;

  case 12:
#line 168 "m68k-parse.y"
    {
		  op->mode = CONTROL;
		  op->reg = (yyvsp[(1) - (1)].reg);
		}
    break;

  case 13:
#line 173 "m68k-parse.y"
    {
		  op->mode = CONTROL;
		  op->reg = (yyvsp[(1) - (1)].reg);
		}
    break;

  case 14:
#line 178 "m68k-parse.y"
    {
		  op->mode = ABSL;
		  op->disp = (yyvsp[(1) - (1)].exp);
		}
    break;

  case 15:
#line 183 "m68k-parse.y"
    {
		  op->mode = IMMED;
		  op->disp = (yyvsp[(2) - (2)].exp);
		}
    break;

  case 16:
#line 188 "m68k-parse.y"
    {
		  op->mode = IMMED;
		  op->disp = (yyvsp[(2) - (2)].exp);
		}
    break;

  case 17:
#line 193 "m68k-parse.y"
    {
		  op->mode = REGLST;
		  op->mask = (yyvsp[(1) - (1)].mask);
		}
    break;

  case 18:
#line 206 "m68k-parse.y"
    {
		  op->mode = AINDR;
		  op->reg = (yyvsp[(2) - (3)].reg);
		}
    break;

  case 19:
#line 211 "m68k-parse.y"
    {
		  op->mode = AINC;
		  op->reg = (yyvsp[(2) - (4)].reg);
		}
    break;

  case 20:
#line 216 "m68k-parse.y"
    {
		  op->mode = ADEC;
		  op->reg = (yyvsp[(3) - (4)].reg);
		}
    break;

  case 21:
#line 221 "m68k-parse.y"
    {
		  op->reg = (yyvsp[(4) - (5)].reg);
		  op->disp = (yyvsp[(2) - (5)].exp);
		  if (((yyvsp[(4) - (5)].reg) >= ZADDR0 && (yyvsp[(4) - (5)].reg) <= ZADDR7)
		      || (yyvsp[(4) - (5)].reg) == ZPC)
		    op->mode = BASE;
		  else
		    op->mode = DISP;
		}
    break;

  case 22:
#line 231 "m68k-parse.y"
    {
		  op->reg = (yyvsp[(2) - (5)].reg);
		  op->disp = (yyvsp[(4) - (5)].exp);
		  if (((yyvsp[(2) - (5)].reg) >= ZADDR0 && (yyvsp[(2) - (5)].reg) <= ZADDR7)
		      || (yyvsp[(2) - (5)].reg) == ZPC)
		    op->mode = BASE;
		  else
		    op->mode = DISP;
		}
    break;

  case 23:
#line 241 "m68k-parse.y"
    {
		  op->reg = (yyvsp[(3) - (4)].reg);
		  op->disp = (yyvsp[(1) - (4)].exp);
		  if (((yyvsp[(3) - (4)].reg) >= ZADDR0 && (yyvsp[(3) - (4)].reg) <= ZADDR7)
		      || (yyvsp[(3) - (4)].reg) == ZPC)
		    op->mode = BASE;
		  else
		    op->mode = DISP;
		}
    break;

  case 24:
#line 251 "m68k-parse.y"
    {
		  op->mode = DISP;
		  op->reg = (yyvsp[(2) - (3)].reg);
		}
    break;

  case 25:
#line 256 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(2) - (3)].reg);
		}
    break;

  case 26:
#line 261 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(2) - (3)].reg);
		}
    break;

  case 27:
#line 266 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(4) - (7)].reg);
		  op->disp = (yyvsp[(2) - (7)].exp);
		  op->index = (yyvsp[(6) - (7)].indexreg);
		}
    break;

  case 28:
#line 273 "m68k-parse.y"
    {
		  if ((yyvsp[(4) - (7)].reg) == PC || (yyvsp[(4) - (7)].reg) == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = BASE;
		  op->reg = (yyvsp[(6) - (7)].reg);
		  op->disp = (yyvsp[(2) - (7)].exp);
		  op->index.reg = (yyvsp[(4) - (7)].reg);
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		}
    break;

  case 29:
#line 284 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(5) - (6)].reg);
		  op->disp = (yyvsp[(2) - (6)].exp);
		  op->index = (yyvsp[(4) - (6)].indexreg);
		}
    break;

  case 30:
#line 291 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->disp = (yyvsp[(4) - (5)].exp);
		  op->index = (yyvsp[(2) - (5)].indexreg);
		}
    break;

  case 31:
#line 297 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(3) - (6)].reg);
		  op->disp = (yyvsp[(1) - (6)].exp);
		  op->index = (yyvsp[(5) - (6)].indexreg);
		}
    break;

  case 32:
#line 304 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(2) - (5)].reg);
		  op->index = (yyvsp[(4) - (5)].indexreg);
		}
    break;

  case 33:
#line 310 "m68k-parse.y"
    {
		  if ((yyvsp[(3) - (6)].reg) == PC || (yyvsp[(3) - (6)].reg) == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = BASE;
		  op->reg = (yyvsp[(5) - (6)].reg);
		  op->disp = (yyvsp[(1) - (6)].exp);
		  op->index.reg = (yyvsp[(3) - (6)].reg);
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		}
    break;

  case 34:
#line 321 "m68k-parse.y"
    {
		  if ((yyvsp[(2) - (5)].reg) == PC || (yyvsp[(2) - (5)].reg) == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = BASE;
		  op->reg = (yyvsp[(4) - (5)].reg);
		  op->index.reg = (yyvsp[(2) - (5)].reg);
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		}
    break;

  case 35:
#line 331 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(4) - (5)].reg);
		  op->disp = (yyvsp[(1) - (5)].exp);
		  op->index = (yyvsp[(3) - (5)].indexreg);
		}
    break;

  case 36:
#line 338 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(3) - (4)].reg);
		  op->index = (yyvsp[(2) - (4)].indexreg);
		}
    break;

  case 37:
#line 344 "m68k-parse.y"
    {
		  op->mode = POST;
		  op->reg = (yyvsp[(4) - (9)].reg);
		  op->disp = (yyvsp[(3) - (9)].exp);
		  op->index = (yyvsp[(7) - (9)].indexreg);
		  op->odisp = (yyvsp[(8) - (9)].exp);
		}
    break;

  case 38:
#line 352 "m68k-parse.y"
    {
		  op->mode = POST;
		  op->reg = (yyvsp[(4) - (7)].reg);
		  op->disp = (yyvsp[(3) - (7)].exp);
		  op->odisp = (yyvsp[(6) - (7)].exp);
		}
    break;

  case 39:
#line 359 "m68k-parse.y"
    {
		  op->mode = POST;
		  op->reg = (yyvsp[(3) - (8)].reg);
		  op->index = (yyvsp[(6) - (8)].indexreg);
		  op->odisp = (yyvsp[(7) - (8)].exp);
		}
    break;

  case 40:
#line 366 "m68k-parse.y"
    {
		  op->mode = POST;
		  op->reg = (yyvsp[(3) - (6)].reg);
		  op->odisp = (yyvsp[(5) - (6)].exp);
		}
    break;

  case 41:
#line 372 "m68k-parse.y"
    {
		  op->mode = PRE;
		  op->reg = (yyvsp[(5) - (10)].reg);
		  op->disp = (yyvsp[(3) - (10)].exp);
		  op->index = (yyvsp[(7) - (10)].indexreg);
		  op->odisp = (yyvsp[(9) - (10)].exp);
		}
    break;

  case 42:
#line 380 "m68k-parse.y"
    {
		  op->mode = PRE;
		  op->reg = (yyvsp[(3) - (8)].reg);
		  op->index = (yyvsp[(5) - (8)].indexreg);
		  op->odisp = (yyvsp[(7) - (8)].exp);
		}
    break;

  case 43:
#line 387 "m68k-parse.y"
    {
		  if ((yyvsp[(5) - (10)].reg) == PC || (yyvsp[(5) - (10)].reg) == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = PRE;
		  op->reg = (yyvsp[(7) - (10)].reg);
		  op->disp = (yyvsp[(3) - (10)].exp);
		  op->index.reg = (yyvsp[(5) - (10)].reg);
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		  op->odisp = (yyvsp[(9) - (10)].exp);
		}
    break;

  case 44:
#line 399 "m68k-parse.y"
    {
		  if ((yyvsp[(3) - (8)].reg) == PC || (yyvsp[(3) - (8)].reg) == ZPC)
		    yyerror (_("syntax error"));
		  op->mode = PRE;
		  op->reg = (yyvsp[(5) - (8)].reg);
		  op->index.reg = (yyvsp[(3) - (8)].reg);
		  op->index.size = SIZE_UNSPEC;
		  op->index.scale = 1;
		  op->odisp = (yyvsp[(7) - (8)].exp);
		}
    break;

  case 45:
#line 410 "m68k-parse.y"
    {
		  op->mode = PRE;
		  op->reg = (yyvsp[(5) - (8)].reg);
		  op->disp = (yyvsp[(3) - (8)].exp);
		  op->index = (yyvsp[(4) - (8)].indexreg);
		  op->odisp = (yyvsp[(7) - (8)].exp);
		}
    break;

  case 46:
#line 423 "m68k-parse.y"
    {
		  /* We use optzapc to avoid a shift/reduce conflict.  */
		  if ((yyvsp[(1) - (2)].reg) < ADDR0 || (yyvsp[(1) - (2)].reg) > ADDR7)
		    yyerror (_("syntax error"));
		  op->mode = AINDR;
		  op->reg = (yyvsp[(1) - (2)].reg);
		}
    break;

  case 47:
#line 431 "m68k-parse.y"
    {
		  /* We use optzapc to avoid a shift/reduce conflict.  */
		  if ((yyvsp[(1) - (3)].reg) < ADDR0 || (yyvsp[(1) - (3)].reg) > ADDR7)
		    yyerror (_("syntax error"));
		  op->mode = AINC;
		  op->reg = (yyvsp[(1) - (3)].reg);
		}
    break;

  case 48:
#line 439 "m68k-parse.y"
    {
		  /* We use optzapc to avoid a shift/reduce conflict.  */
		  if ((yyvsp[(1) - (3)].reg) < ADDR0 || (yyvsp[(1) - (3)].reg) > ADDR7)
		    yyerror (_("syntax error"));
		  op->mode = ADEC;
		  op->reg = (yyvsp[(1) - (3)].reg);
		}
    break;

  case 49:
#line 447 "m68k-parse.y"
    {
		  op->reg = (yyvsp[(1) - (5)].reg);
		  op->disp = (yyvsp[(4) - (5)].exp);
		  if (((yyvsp[(1) - (5)].reg) >= ZADDR0 && (yyvsp[(1) - (5)].reg) <= ZADDR7)
		      || (yyvsp[(1) - (5)].reg) == ZPC)
		    op->mode = BASE;
		  else
		    op->mode = DISP;
		}
    break;

  case 50:
#line 457 "m68k-parse.y"
    {
		  op->mode = BASE;
		  op->reg = (yyvsp[(1) - (6)].reg);
		  op->disp = (yyvsp[(4) - (6)].exp);
		  op->index = (yyvsp[(5) - (6)].indexreg);
		}
    break;

  case 51:
#line 464 "m68k-parse.y"
    {
		  op->mode = POST;
		  op->reg = (yyvsp[(1) - (10)].reg);
		  op->disp = (yyvsp[(4) - (10)].exp);
		  op->index = (yyvsp[(9) - (10)].indexreg);
		  op->odisp = (yyvsp[(8) - (10)].exp);
		}
    break;

  case 52:
#line 472 "m68k-parse.y"
    {
		  op->mode = POST;
		  op->reg = (yyvsp[(1) - (9)].reg);
		  op->disp = (yyvsp[(4) - (9)].exp);
		  op->odisp = (yyvsp[(8) - (9)].exp);
		}
    break;

  case 53:
#line 479 "m68k-parse.y"
    {
		  op->mode = PRE;
		  op->reg = (yyvsp[(1) - (10)].reg);
		  op->disp = (yyvsp[(4) - (10)].exp);
		  op->index = (yyvsp[(5) - (10)].indexreg);
		  op->odisp = (yyvsp[(9) - (10)].exp);
		}
    break;

  case 55:
#line 494 "m68k-parse.y"
    {
		  (yyval.indexreg).reg = (yyvsp[(1) - (1)].reg);
		  (yyval.indexreg).size = SIZE_UNSPEC;
		  (yyval.indexreg).scale = 1;
		}
    break;

  case 57:
#line 508 "m68k-parse.y"
    {
		  (yyval.indexreg).reg = (yyvsp[(1) - (1)].reg);
		  (yyval.indexreg).size = SIZE_UNSPEC;
		  (yyval.indexreg).scale = 1;
		}
    break;

  case 68:
#line 551 "m68k-parse.y"
    {
		  (yyval.reg) = ZADDR0;
		}
    break;

  case 72:
#line 568 "m68k-parse.y"
    {
		  (yyval.reg) = ZADDR0;
		}
    break;

  case 73:
#line 572 "m68k-parse.y"
    {
		  (yyval.reg) = (yyvsp[(2) - (2)].reg);
		}
    break;

  case 74:
#line 581 "m68k-parse.y"
    {
		  (yyval.exp).exp.X_op = O_absent;
		  (yyval.exp).size = SIZE_UNSPEC;
		}
    break;

  case 75:
#line 586 "m68k-parse.y"
    {
		  (yyval.exp) = (yyvsp[(2) - (2)].exp);
		}
    break;

  case 76:
#line 595 "m68k-parse.y"
    {
		  (yyval.exp).exp.X_op = O_absent;
		  (yyval.exp).size = SIZE_UNSPEC;
		}
    break;

  case 77:
#line 600 "m68k-parse.y"
    {
		  (yyval.exp) = (yyvsp[(1) - (2)].exp);
		}
    break;

  case 79:
#line 610 "m68k-parse.y"
    {
		  (yyval.mask) = (yyvsp[(1) - (3)].mask) | (yyvsp[(3) - (3)].mask);
		}
    break;

  case 80:
#line 614 "m68k-parse.y"
    {
		  (yyval.mask) = (1 << (yyvsp[(1) - (3)].onereg)) | (yyvsp[(3) - (3)].mask);
		}
    break;

  case 81:
#line 626 "m68k-parse.y"
    {
		  (yyval.mask) = 1 << (yyvsp[(1) - (1)].onereg);
		}
    break;

  case 83:
#line 631 "m68k-parse.y"
    {
		  (yyval.mask) = (yyvsp[(1) - (3)].mask) | (yyvsp[(3) - (3)].mask);
		}
    break;

  case 84:
#line 635 "m68k-parse.y"
    {
		  (yyval.mask) = (1 << (yyvsp[(1) - (3)].onereg)) | (yyvsp[(3) - (3)].mask);
		}
    break;

  case 85:
#line 642 "m68k-parse.y"
    {
		  if ((yyvsp[(1) - (3)].onereg) <= (yyvsp[(3) - (3)].onereg))
		    (yyval.mask) = (1 << ((yyvsp[(3) - (3)].onereg) + 1)) - 1 - ((1 << (yyvsp[(1) - (3)].onereg)) - 1);
		  else
		    (yyval.mask) = (1 << ((yyvsp[(1) - (3)].onereg) + 1)) - 1 - ((1 << (yyvsp[(3) - (3)].onereg)) - 1);
		}
    break;

  case 86:
#line 652 "m68k-parse.y"
    {
		  (yyval.onereg) = (yyvsp[(1) - (1)].reg) - DATA0;
		}
    break;

  case 87:
#line 656 "m68k-parse.y"
    {
		  (yyval.onereg) = (yyvsp[(1) - (1)].reg) - ADDR0 + 8;
		}
    break;

  case 88:
#line 660 "m68k-parse.y"
    {
		  (yyval.onereg) = (yyvsp[(1) - (1)].reg) - FP0 + 16;
		}
    break;

  case 89:
#line 664 "m68k-parse.y"
    {
		  if ((yyvsp[(1) - (1)].reg) == FPI)
		    (yyval.onereg) = 24;
		  else if ((yyvsp[(1) - (1)].reg) == FPS)
		    (yyval.onereg) = 25;
		  else
		    (yyval.onereg) = 26;
		}
    break;


/* Line 1267 of yacc.c.  */
#line 2204 "m68k-parse.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 674 "m68k-parse.y"


/* The string to parse is stored here, and modified by yylex.  */

static char *str;

/* The original string pointer.  */

static char *strorig;

/* If *CCP could be a register, return the register number and advance
   *CCP.  Otherwise don't change *CCP, and return 0.  */

static enum m68k_register
m68k_reg_parse (char **ccp)
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
yylex (void)
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
    if (cp - 7 > str && cp[-7] == '@')
      {
	if (strncmp (cp - 7, "@TLSLDM", 7) == 0)
	  {
	    yylval.exp.pic_reloc = pic_tls_ldm;
	    tail += 7;
	  }
	else if (strncmp (cp - 7, "@TLSLDO", 7) == 0)
	  {
	    yylval.exp.pic_reloc = pic_tls_ldo;
	    tail += 7;
	  }
      }
    else if (cp - 6 > str && cp[-6] == '@')
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
	else if (strncmp (cp - 6, "@TLSGD", 6) == 0)
	  {
	    yylval.exp.pic_reloc = pic_tls_gd;
	    tail += 6;
	  }
	else if (strncmp (cp - 6, "@TLSIE", 6) == 0)
	  {
	    yylval.exp.pic_reloc = pic_tls_ie;
	    tail += 6;
	  }
	else if (strncmp (cp - 6, "@TLSLE", 6) == 0)
	  {
	    yylval.exp.pic_reloc = pic_tls_le;
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
m68k_ip_op (char *s, struct m68k_op *oparg)
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
yyerror (const char *s)
{
  op->error = s;
}

