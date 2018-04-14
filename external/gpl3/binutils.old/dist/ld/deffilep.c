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
     NAME = 258,
     LIBRARY = 259,
     DESCRIPTION = 260,
     STACKSIZE_K = 261,
     HEAPSIZE = 262,
     CODE = 263,
     DATAU = 264,
     DATAL = 265,
     SECTIONS = 266,
     EXPORTS = 267,
     IMPORTS = 268,
     VERSIONK = 269,
     BASE = 270,
     CONSTANTU = 271,
     CONSTANTL = 272,
     PRIVATEU = 273,
     PRIVATEL = 274,
     ALIGNCOMM = 275,
     READ = 276,
     WRITE = 277,
     EXECUTE = 278,
     SHARED = 279,
     NONAMEU = 280,
     NONAMEL = 281,
     DIRECTIVE = 282,
     EQUAL = 283,
     ID = 284,
     DIGITS = 285
   };
#endif
/* Tokens.  */
#define NAME 258
#define LIBRARY 259
#define DESCRIPTION 260
#define STACKSIZE_K 261
#define HEAPSIZE 262
#define CODE 263
#define DATAU 264
#define DATAL 265
#define SECTIONS 266
#define EXPORTS 267
#define IMPORTS 268
#define VERSIONK 269
#define BASE 270
#define CONSTANTU 271
#define CONSTANTL 272
#define PRIVATEU 273
#define PRIVATEL 274
#define ALIGNCOMM 275
#define READ 276
#define WRITE 277
#define EXECUTE 278
#define SHARED 279
#define NONAMEU 280
#define NONAMEL 281
#define DIRECTIVE 282
#define EQUAL 283
#define ID 284
#define DIGITS 285




/* Copy the first part of user declarations.  */
#line 1 "deffilep.y"
 /* deffilep.y - parser for .def files */

/*   Copyright (C) 1995-2016 Free Software Foundation, Inc.

     This file is part of GNU Binutils.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
     MA 02110-1301, USA.  */

#include "sysdep.h"
#include "libiberty.h"
#include "safe-ctype.h"
#include "bfd.h"
#include "ld.h"
#include "ldmisc.h"
#include "deffile.h"

#define TRACE 0

#define ROUND_UP(a, b) (((a)+((b)-1))&~((b)-1))

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in ld.  Note that these are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list.  */

#define	yymaxdepth def_maxdepth
#define	yyparse	def_parse
#define	yylex	def_lex
#define	yyerror	def_error
#define	yylval	def_lval
#define	yychar	def_char
#define	yydebug	def_debug
#define	yypact	def_pact
#define	yyr1	def_r1
#define	yyr2	def_r2
#define	yydef	def_def
#define	yychk	def_chk
#define	yypgo	def_pgo
#define	yyact	def_act
#define	yyexca	def_exca
#define yyerrflag def_errflag
#define yynerrs	def_nerrs
#define	yyps	def_ps
#define	yypv	def_pv
#define	yys	def_s
#define	yy_yys	def_yys
#define	yystate	def_state
#define	yytmp	def_tmp
#define	yyv	def_v
#define	yy_yyv	def_yyv
#define	yyval	def_val
#define	yylloc	def_lloc
#define yyreds	def_reds		/* With YYDEBUG defined.  */
#define yytoks	def_toks		/* With YYDEBUG defined.  */
#define yylhs	def_yylhs
#define yylen	def_yylen
#define yydefred def_yydefred
#define yydgoto	def_yydgoto
#define yysindex def_yysindex
#define yyrindex def_yyrindex
#define yygindex def_yygindex
#define yytable	 def_yytable
#define yycheck	 def_yycheck

typedef struct def_pool_str {
  struct def_pool_str *next;
  char data[1];
} def_pool_str;

static def_pool_str *pool_strs = NULL;

static char *def_pool_alloc (size_t sz);
static char *def_pool_strdup (const char *str);
static void def_pool_free (void);

static void def_description (const char *);
static void def_exports (const char *, const char *, int, int, const char *);
static void def_heapsize (int, int);
static void def_import (const char *, const char *, const char *, const char *,
			int, const char *);
static void def_image_name (const char *, bfd_vma, int);
static void def_section (const char *, int);
static void def_section_alt (const char *, const char *);
static void def_stacksize (int, int);
static void def_version (int, int);
static void def_directive (char *);
static void def_aligncomm (char *str, int align);
static int def_parse (void);
static int def_error (const char *);
static int def_lex (void);

static int lex_forced_token = 0;
static const char *lex_parse_string = 0;
static const char *lex_parse_string_end = 0;



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
#line 113 "deffilep.y"
{
  char *id;
  const char *id_const;
  int number;
  bfd_vma vma;
  char *digits;
}
/* Line 193 of yacc.c.  */
#line 276 "deffilep.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 289 "deffilep.c"

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
#define YYFINAL  69
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   149

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  35
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  99
/* YYNRULES -- Number of states.  */
#define YYNSTATES  146

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   285

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    32,     2,    31,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    33,     2,     2,    34,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,    12,    16,    19,    23,    27,
      30,    33,    36,    39,    42,    45,    50,    53,    58,    59,
      61,    64,    72,    76,    77,    79,    81,    83,    85,    87,
      89,    91,    93,    96,    98,   107,   116,   123,   130,   137,
     142,   145,   147,   150,   153,   157,   159,   161,   162,   165,
     166,   168,   170,   172,   174,   176,   178,   180,   182,   184,
     186,   188,   190,   192,   194,   196,   198,   200,   202,   204,
     206,   208,   210,   212,   214,   216,   218,   220,   223,   226,
     230,   234,   236,   237,   240,   241,   244,   245,   248,   249,
     253,   254,   256,   259,   264,   266,   267,   269,   270,   272
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      36,     0,    -1,    36,    37,    -1,    37,    -1,     3,    52,
      56,    -1,     4,    52,    56,    -1,     5,    29,    -1,     6,
      60,    48,    -1,     7,    60,    48,    -1,     8,    46,    -1,
       9,    46,    -1,    11,    44,    -1,    12,    38,    -1,    13,
      42,    -1,    14,    60,    -1,    14,    60,    31,    60,    -1,
      27,    29,    -1,    20,    57,    32,    60,    -1,    -1,    39,
      -1,    38,    39,    -1,    51,    55,    54,    47,    40,    47,
      53,    -1,    41,    47,    40,    -1,    -1,    25,    -1,    26,
      -1,    16,    -1,    17,    -1,     9,    -1,    10,    -1,    18,
      -1,    19,    -1,    42,    43,    -1,    43,    -1,    29,    33,
      29,    31,    29,    31,    29,    53,    -1,    29,    33,    29,
      31,    29,    31,    60,    53,    -1,    29,    33,    29,    31,
      29,    53,    -1,    29,    33,    29,    31,    60,    53,    -1,
      29,    31,    29,    31,    29,    53,    -1,    29,    31,    29,
      53,    -1,    44,    45,    -1,    45,    -1,    29,    46,    -1,
      29,    29,    -1,    46,    47,    49,    -1,    49,    -1,    32,
      -1,    -1,    32,    60,    -1,    -1,    21,    -1,    22,    -1,
      23,    -1,    24,    -1,    15,    -1,     8,    -1,    16,    -1,
      17,    -1,     9,    -1,    10,    -1,     5,    -1,    27,    -1,
      23,    -1,    12,    -1,     7,    -1,    13,    -1,     3,    -1,
      25,    -1,    26,    -1,    18,    -1,    19,    -1,    21,    -1,
      24,    -1,     6,    -1,    14,    -1,    22,    -1,    29,    -1,
      31,    50,    -1,    31,    51,    -1,    50,    31,    51,    -1,
      29,    31,    51,    -1,    51,    -1,    -1,    28,    29,    -1,
      -1,    34,    60,    -1,    -1,    33,    51,    -1,    -1,    15,
      33,    61,    -1,    -1,    29,    -1,    31,    29,    -1,    57,
      31,    58,    59,    -1,    30,    -1,    -1,    29,    -1,    -1,
      30,    -1,    30,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   138,   138,   139,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   160,   162,
     163,   170,   177,   178,   181,   182,   183,   184,   185,   186,
     187,   188,   191,   192,   196,   198,   200,   202,   204,   206,
     211,   212,   216,   217,   221,   222,   226,   227,   229,   230,
     234,   235,   236,   237,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   271,   272,   278,   284,
     290,   298,   299,   302,   303,   307,   308,   312,   313,   316,
     317,   320,   321,   327,   335,   336,   339,   340,   343,   345
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NAME", "LIBRARY", "DESCRIPTION",
  "STACKSIZE_K", "HEAPSIZE", "CODE", "DATAU", "DATAL", "SECTIONS",
  "EXPORTS", "IMPORTS", "VERSIONK", "BASE", "CONSTANTU", "CONSTANTL",
  "PRIVATEU", "PRIVATEL", "ALIGNCOMM", "READ", "WRITE", "EXECUTE",
  "SHARED", "NONAMEU", "NONAMEL", "DIRECTIVE", "EQUAL", "ID", "DIGITS",
  "'.'", "','", "'='", "'@'", "$accept", "start", "command", "explist",
  "expline", "exp_opt_list", "exp_opt", "implist", "impline", "seclist",
  "secline", "attr_list", "opt_comma", "opt_number", "attr",
  "keyword_as_name", "opt_name2", "opt_name", "opt_equalequal_name",
  "opt_ordinal", "opt_equal_name", "opt_base", "anylang_id", "opt_digits",
  "opt_id", "NUMBER", "VMA", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,    46,    44,    61,    64
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    35,    36,    36,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    38,    38,
      38,    39,    40,    40,    41,    41,    41,    41,    41,    41,
      41,    41,    42,    42,    43,    43,    43,    43,    43,    43,
      44,    44,    45,    45,    46,    46,    47,    47,    48,    48,
      49,    49,    49,    49,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    50,    51,    51,    51,    51,
      51,    52,    52,    53,    53,    54,    54,    55,    55,    56,
      56,    57,    57,    57,    58,    58,    59,    59,    60,    61
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     3,     3,     2,     3,     3,     2,
       2,     2,     2,     2,     2,     4,     2,     4,     0,     1,
       2,     7,     3,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     8,     8,     6,     6,     6,     4,
       2,     1,     2,     2,     3,     1,     1,     0,     2,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     3,
       3,     1,     0,     2,     0,     2,     0,     2,     0,     3,
       0,     1,     2,     4,     1,     0,     1,     0,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    82,    82,     0,     0,     0,     0,     0,     0,    18,
       0,     0,     0,     0,     0,     3,    66,    60,    73,    64,
      55,    58,    59,    63,    65,    74,    54,    56,    57,    69,
      70,    71,    75,    62,    72,    67,    68,    61,    76,     0,
       0,    81,    90,    90,     6,    98,    49,    49,    50,    51,
      52,    53,     9,    45,    10,     0,    11,    41,    12,    19,
      88,     0,    13,    33,    14,    91,     0,     0,    16,     1,
       2,     0,    77,    78,     0,     0,     4,     5,     0,     7,
       8,    46,     0,    43,    42,    40,    20,     0,    86,     0,
       0,    32,     0,    92,    95,     0,    80,    79,     0,    48,
      44,    87,     0,    47,    84,     0,    15,    94,    97,    17,
      99,    89,    85,    23,     0,     0,    39,     0,    96,    93,
      28,    29,    26,    27,    30,    31,    24,    25,    47,    47,
      83,    84,    84,    84,    84,    23,    38,     0,    36,    37,
      21,    22,    84,    84,    34,    35
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    14,    15,    58,    59,   128,   129,    62,    63,    56,
      57,    52,    82,    79,    53,    40,    41,    42,   116,   103,
      88,    76,    67,   108,   119,    46,   111
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -82
static const yytype_int8 yypact[] =
{
     122,    11,    11,   -25,     9,     9,    53,    53,   -17,    11,
      14,     9,   -18,    20,    95,   -82,   -82,   -82,   -82,   -82,
     -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,
     -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,    29,    11,
      47,   -82,    67,    67,   -82,   -82,    54,    54,   -82,   -82,
     -82,   -82,    48,   -82,    48,   -14,   -17,   -82,    11,   -82,
      58,    50,    14,   -82,    61,   -82,    64,    33,   -82,   -82,
     -82,    11,    47,   -82,    11,    63,   -82,   -82,     9,   -82,
     -82,   -82,    53,   -82,    48,   -82,   -82,    11,    60,    76,
      81,   -82,     9,   -82,    83,     9,   -82,   -82,    84,   -82,
     -82,   -82,     9,    79,   -26,    85,   -82,   -82,    88,   -82,
     -82,   -82,   -82,    36,    89,    90,   -82,    55,   -82,   -82,
     -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,    79,    79,
     -82,    92,    13,    92,    92,    36,   -82,    59,   -82,   -82,
     -82,   -82,    92,    92,   -82,   -82
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -82,   -82,   107,   -82,    65,   -11,   -82,   -82,    75,   -82,
      82,    -4,   -81,    93,    57,   102,    -8,   141,   -75,   -82,
     -82,   101,   -82,   -82,   -82,    -5,   -82
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -48
static const yytype_int16 yytable[] =
{
      47,    60,   114,    54,    44,   115,    64,    48,    49,    50,
      51,    65,    55,    66,    16,    83,    17,    18,    19,    20,
      21,    22,   113,    23,    24,    25,    26,    27,    28,    29,
      30,    73,    31,    32,    33,    34,    35,    36,    37,    45,
      38,   114,    39,    61,   137,   120,   121,   134,   135,    68,
      60,    84,   122,   123,   124,   125,   136,   138,   139,   140,
      71,   126,   127,    96,    94,    95,    97,   144,   145,   -47,
     -47,   -47,   -47,    99,    48,    49,    50,    51,    74,   101,
      81,    89,    75,    90,   132,    45,    78,   106,   142,    45,
     109,    87,    92,    93,   102,    69,    98,   112,     1,     2,
       3,     4,     5,     6,     7,   104,     8,     9,    10,    11,
     105,    81,   133,   107,   110,    12,   117,   118,   130,   131,
     114,    70,    13,    86,   141,     1,     2,     3,     4,     5,
       6,     7,   143,     8,     9,    10,    11,    91,    85,   100,
      80,    72,    12,    43,    77,     0,     0,     0,     0,    13
};

static const yytype_int16 yycheck[] =
{
       5,     9,    28,     7,    29,    31,    11,    21,    22,    23,
      24,    29,    29,    31,     3,    29,     5,     6,     7,     8,
       9,    10,   103,    12,    13,    14,    15,    16,    17,    18,
      19,    39,    21,    22,    23,    24,    25,    26,    27,    30,
      29,    28,    31,    29,    31,     9,    10,   128,   129,    29,
      58,    55,    16,    17,    18,    19,   131,   132,   133,   134,
      31,    25,    26,    71,    31,    32,    74,   142,   143,    21,
      22,    23,    24,    78,    21,    22,    23,    24,    31,    87,
      32,    31,    15,    33,    29,    30,    32,    92,    29,    30,
      95,    33,    31,    29,    34,     0,    33,   102,     3,     4,
       5,     6,     7,     8,     9,    29,    11,    12,    13,    14,
      29,    32,   117,    30,    30,    20,    31,    29,    29,    29,
      28,    14,    27,    58,   135,     3,     4,     5,     6,     7,
       8,     9,   137,    11,    12,    13,    14,    62,    56,    82,
      47,    39,    20,     2,    43,    -1,    -1,    -1,    -1,    27
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    11,    12,
      13,    14,    20,    27,    36,    37,     3,     5,     6,     7,
       8,     9,    10,    12,    13,    14,    15,    16,    17,    18,
      19,    21,    22,    23,    24,    25,    26,    27,    29,    31,
      50,    51,    52,    52,    29,    30,    60,    60,    21,    22,
      23,    24,    46,    49,    46,    29,    44,    45,    38,    39,
      51,    29,    42,    43,    60,    29,    31,    57,    29,     0,
      37,    31,    50,    51,    31,    15,    56,    56,    32,    48,
      48,    32,    47,    29,    46,    45,    39,    33,    55,    31,
      33,    43,    31,    29,    31,    32,    51,    51,    33,    60,
      49,    51,    34,    54,    29,    29,    60,    30,    58,    60,
      30,    61,    60,    47,    28,    31,    53,    31,    29,    59,
       9,    10,    16,    17,    18,    19,    25,    26,    40,    41,
      29,    29,    29,    60,    47,    47,    53,    31,    53,    53,
      53,    40,    29,    60,    53,    53
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
        case 4:
#line 143 "deffilep.y"
    { def_image_name ((yyvsp[(2) - (3)].id), (yyvsp[(3) - (3)].vma), 0); }
    break;

  case 5:
#line 144 "deffilep.y"
    { def_image_name ((yyvsp[(2) - (3)].id), (yyvsp[(3) - (3)].vma), 1); }
    break;

  case 6:
#line 145 "deffilep.y"
    { def_description ((yyvsp[(2) - (2)].id));}
    break;

  case 7:
#line 146 "deffilep.y"
    { def_stacksize ((yyvsp[(2) - (3)].number), (yyvsp[(3) - (3)].number));}
    break;

  case 8:
#line 147 "deffilep.y"
    { def_heapsize ((yyvsp[(2) - (3)].number), (yyvsp[(3) - (3)].number));}
    break;

  case 9:
#line 148 "deffilep.y"
    { def_section ("CODE", (yyvsp[(2) - (2)].number));}
    break;

  case 10:
#line 149 "deffilep.y"
    { def_section ("DATA", (yyvsp[(2) - (2)].number));}
    break;

  case 14:
#line 153 "deffilep.y"
    { def_version ((yyvsp[(2) - (2)].number), 0);}
    break;

  case 15:
#line 154 "deffilep.y"
    { def_version ((yyvsp[(2) - (4)].number), (yyvsp[(4) - (4)].number));}
    break;

  case 16:
#line 155 "deffilep.y"
    { def_directive ((yyvsp[(2) - (2)].id));}
    break;

  case 17:
#line 156 "deffilep.y"
    { def_aligncomm ((yyvsp[(2) - (4)].id), (yyvsp[(4) - (4)].number));}
    break;

  case 21:
#line 171 "deffilep.y"
    { def_exports ((yyvsp[(1) - (7)].id), (yyvsp[(2) - (7)].id), (yyvsp[(3) - (7)].number), (yyvsp[(5) - (7)].number), (yyvsp[(7) - (7)].id)); }
    break;

  case 22:
#line 177 "deffilep.y"
    { (yyval.number) = (yyvsp[(1) - (3)].number) | (yyvsp[(3) - (3)].number); }
    break;

  case 23:
#line 178 "deffilep.y"
    { (yyval.number) = 0; }
    break;

  case 24:
#line 181 "deffilep.y"
    { (yyval.number) = 1; }
    break;

  case 25:
#line 182 "deffilep.y"
    { (yyval.number) = 1; }
    break;

  case 26:
#line 183 "deffilep.y"
    { (yyval.number) = 2; }
    break;

  case 27:
#line 184 "deffilep.y"
    { (yyval.number) = 2; }
    break;

  case 28:
#line 185 "deffilep.y"
    { (yyval.number) = 4; }
    break;

  case 29:
#line 186 "deffilep.y"
    { (yyval.number) = 4; }
    break;

  case 30:
#line 187 "deffilep.y"
    { (yyval.number) = 8; }
    break;

  case 31:
#line 188 "deffilep.y"
    { (yyval.number) = 8; }
    break;

  case 34:
#line 197 "deffilep.y"
    { def_import ((yyvsp[(1) - (8)].id), (yyvsp[(3) - (8)].id), (yyvsp[(5) - (8)].id), (yyvsp[(7) - (8)].id), -1, (yyvsp[(8) - (8)].id)); }
    break;

  case 35:
#line 199 "deffilep.y"
    { def_import ((yyvsp[(1) - (8)].id), (yyvsp[(3) - (8)].id), (yyvsp[(5) - (8)].id),  0, (yyvsp[(7) - (8)].number), (yyvsp[(8) - (8)].id)); }
    break;

  case 36:
#line 201 "deffilep.y"
    { def_import ((yyvsp[(1) - (6)].id), (yyvsp[(3) - (6)].id),  0, (yyvsp[(5) - (6)].id), -1, (yyvsp[(6) - (6)].id)); }
    break;

  case 37:
#line 203 "deffilep.y"
    { def_import ((yyvsp[(1) - (6)].id), (yyvsp[(3) - (6)].id),  0,  0, (yyvsp[(5) - (6)].number), (yyvsp[(6) - (6)].id)); }
    break;

  case 38:
#line 205 "deffilep.y"
    { def_import( 0, (yyvsp[(1) - (6)].id), (yyvsp[(3) - (6)].id), (yyvsp[(5) - (6)].id), -1, (yyvsp[(6) - (6)].id)); }
    break;

  case 39:
#line 207 "deffilep.y"
    { def_import ( 0, (yyvsp[(1) - (4)].id),  0, (yyvsp[(3) - (4)].id), -1, (yyvsp[(4) - (4)].id)); }
    break;

  case 42:
#line 216 "deffilep.y"
    { def_section ((yyvsp[(1) - (2)].id), (yyvsp[(2) - (2)].number));}
    break;

  case 43:
#line 217 "deffilep.y"
    { def_section_alt ((yyvsp[(1) - (2)].id), (yyvsp[(2) - (2)].id));}
    break;

  case 44:
#line 221 "deffilep.y"
    { (yyval.number) = (yyvsp[(1) - (3)].number) | (yyvsp[(3) - (3)].number); }
    break;

  case 45:
#line 222 "deffilep.y"
    { (yyval.number) = (yyvsp[(1) - (1)].number); }
    break;

  case 48:
#line 229 "deffilep.y"
    { (yyval.number)=(yyvsp[(2) - (2)].number);}
    break;

  case 49:
#line 230 "deffilep.y"
    { (yyval.number)=-1;}
    break;

  case 50:
#line 234 "deffilep.y"
    { (yyval.number) = 1;}
    break;

  case 51:
#line 235 "deffilep.y"
    { (yyval.number) = 2;}
    break;

  case 52:
#line 236 "deffilep.y"
    { (yyval.number)=4;}
    break;

  case 53:
#line 237 "deffilep.y"
    { (yyval.number)=8;}
    break;

  case 54:
#line 241 "deffilep.y"
    { (yyval.id_const) = "BASE"; }
    break;

  case 55:
#line 242 "deffilep.y"
    { (yyval.id_const) = "CODE"; }
    break;

  case 56:
#line 243 "deffilep.y"
    { (yyval.id_const) = "CONSTANT"; }
    break;

  case 57:
#line 244 "deffilep.y"
    { (yyval.id_const) = "constant"; }
    break;

  case 58:
#line 245 "deffilep.y"
    { (yyval.id_const) = "DATA"; }
    break;

  case 59:
#line 246 "deffilep.y"
    { (yyval.id_const) = "data"; }
    break;

  case 60:
#line 247 "deffilep.y"
    { (yyval.id_const) = "DESCRIPTION"; }
    break;

  case 61:
#line 248 "deffilep.y"
    { (yyval.id_const) = "DIRECTIVE"; }
    break;

  case 62:
#line 249 "deffilep.y"
    { (yyval.id_const) = "EXECUTE"; }
    break;

  case 63:
#line 250 "deffilep.y"
    { (yyval.id_const) = "EXPORTS"; }
    break;

  case 64:
#line 251 "deffilep.y"
    { (yyval.id_const) = "HEAPSIZE"; }
    break;

  case 65:
#line 252 "deffilep.y"
    { (yyval.id_const) = "IMPORTS"; }
    break;

  case 66:
#line 259 "deffilep.y"
    { (yyval.id_const) = "NAME"; }
    break;

  case 67:
#line 260 "deffilep.y"
    { (yyval.id_const) = "NONAME"; }
    break;

  case 68:
#line 261 "deffilep.y"
    { (yyval.id_const) = "noname"; }
    break;

  case 69:
#line 262 "deffilep.y"
    { (yyval.id_const) = "PRIVATE"; }
    break;

  case 70:
#line 263 "deffilep.y"
    { (yyval.id_const) = "private"; }
    break;

  case 71:
#line 264 "deffilep.y"
    { (yyval.id_const) = "READ"; }
    break;

  case 72:
#line 265 "deffilep.y"
    { (yyval.id_const) = "SHARED"; }
    break;

  case 73:
#line 266 "deffilep.y"
    { (yyval.id_const) = "STACKSIZE"; }
    break;

  case 74:
#line 267 "deffilep.y"
    { (yyval.id_const) = "VERSION"; }
    break;

  case 75:
#line 268 "deffilep.y"
    { (yyval.id_const) = "WRITE"; }
    break;

  case 76:
#line 271 "deffilep.y"
    { (yyval.id) = (yyvsp[(1) - (1)].id); }
    break;

  case 77:
#line 273 "deffilep.y"
    {
	    char *name = xmalloc (strlen ((yyvsp[(2) - (2)].id_const)) + 2);
	    sprintf (name, ".%s", (yyvsp[(2) - (2)].id_const));
	    (yyval.id) = name;
	  }
    break;

  case 78:
#line 279 "deffilep.y"
    {
	    char *name = def_pool_alloc (strlen ((yyvsp[(2) - (2)].id)) + 2);
	    sprintf (name, ".%s", (yyvsp[(2) - (2)].id));
	    (yyval.id) = name;
	  }
    break;

  case 79:
#line 285 "deffilep.y"
    {
	    char *name = def_pool_alloc (strlen ((yyvsp[(1) - (3)].id_const)) + 1 + strlen ((yyvsp[(3) - (3)].id)) + 1);
	    sprintf (name, "%s.%s", (yyvsp[(1) - (3)].id_const), (yyvsp[(3) - (3)].id));
	    (yyval.id) = name;
	  }
    break;

  case 80:
#line 291 "deffilep.y"
    {
	    char *name = def_pool_alloc (strlen ((yyvsp[(1) - (3)].id)) + 1 + strlen ((yyvsp[(3) - (3)].id)) + 1);
	    sprintf (name, "%s.%s", (yyvsp[(1) - (3)].id), (yyvsp[(3) - (3)].id));
	    (yyval.id) = name;
	  }
    break;

  case 81:
#line 298 "deffilep.y"
    { (yyval.id) = (yyvsp[(1) - (1)].id); }
    break;

  case 82:
#line 299 "deffilep.y"
    { (yyval.id) = ""; }
    break;

  case 83:
#line 302 "deffilep.y"
    { (yyval.id) = (yyvsp[(2) - (2)].id); }
    break;

  case 84:
#line 303 "deffilep.y"
    { (yyval.id) = 0; }
    break;

  case 85:
#line 307 "deffilep.y"
    { (yyval.number) = (yyvsp[(2) - (2)].number);}
    break;

  case 86:
#line 308 "deffilep.y"
    { (yyval.number) = -1;}
    break;

  case 87:
#line 312 "deffilep.y"
    { (yyval.id) = (yyvsp[(2) - (2)].id); }
    break;

  case 88:
#line 313 "deffilep.y"
    { (yyval.id) =  0; }
    break;

  case 89:
#line 316 "deffilep.y"
    { (yyval.vma) = (yyvsp[(3) - (3)].vma);}
    break;

  case 90:
#line 317 "deffilep.y"
    { (yyval.vma) = (bfd_vma) -1;}
    break;

  case 91:
#line 320 "deffilep.y"
    { (yyval.id) = (yyvsp[(1) - (1)].id); }
    break;

  case 92:
#line 322 "deffilep.y"
    {
	    char *id = def_pool_alloc (strlen ((yyvsp[(2) - (2)].id)) + 2);
	    sprintf (id, ".%s", (yyvsp[(2) - (2)].id));
	    (yyval.id) = id;
	  }
    break;

  case 93:
#line 328 "deffilep.y"
    {
	    char *id = def_pool_alloc (strlen ((yyvsp[(1) - (4)].id)) + 1 + strlen ((yyvsp[(3) - (4)].digits)) + strlen ((yyvsp[(4) - (4)].id)) + 1);
	    sprintf (id, "%s.%s%s", (yyvsp[(1) - (4)].id), (yyvsp[(3) - (4)].digits), (yyvsp[(4) - (4)].id));
	    (yyval.id) = id;
	  }
    break;

  case 94:
#line 335 "deffilep.y"
    { (yyval.digits) = (yyvsp[(1) - (1)].digits); }
    break;

  case 95:
#line 336 "deffilep.y"
    { (yyval.digits) = ""; }
    break;

  case 96:
#line 339 "deffilep.y"
    { (yyval.id) = (yyvsp[(1) - (1)].id); }
    break;

  case 97:
#line 340 "deffilep.y"
    { (yyval.id) = ""; }
    break;

  case 98:
#line 343 "deffilep.y"
    { (yyval.number) = strtoul ((yyvsp[(1) - (1)].digits), 0, 0); }
    break;

  case 99:
#line 345 "deffilep.y"
    { (yyval.vma) = (bfd_vma) strtoull ((yyvsp[(1) - (1)].digits), 0, 0); }
    break;


/* Line 1267 of yacc.c.  */
#line 2064 "deffilep.c"
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


#line 347 "deffilep.y"


/*****************************************************************************
 API
 *****************************************************************************/

static FILE *the_file;
static const char *def_filename;
static int linenumber;
static def_file *def;
static int saw_newline;

struct directive
  {
    struct directive *next;
    char *name;
    int len;
  };

static struct directive *directives = 0;

def_file *
def_file_empty (void)
{
  def_file *rv = xmalloc (sizeof (def_file));
  memset (rv, 0, sizeof (def_file));
  rv->is_dll = -1;
  rv->base_address = (bfd_vma) -1;
  rv->stack_reserve = rv->stack_commit = -1;
  rv->heap_reserve = rv->heap_commit = -1;
  rv->version_major = rv->version_minor = -1;
  return rv;
}

def_file *
def_file_parse (const char *filename, def_file *add_to)
{
  struct directive *d;

  the_file = fopen (filename, "r");
  def_filename = filename;
  linenumber = 1;
  if (!the_file)
    {
      perror (filename);
      return 0;
    }
  if (add_to)
    {
      def = add_to;
    }
  else
    {
      def = def_file_empty ();
    }

  saw_newline = 1;
  if (def_parse ())
    {
      def_file_free (def);
      fclose (the_file);
      def_pool_free ();
      return 0;
    }

  fclose (the_file);

  while ((d = directives) != NULL)
    {
#if TRACE
      printf ("Adding directive %08x `%s'\n", d->name, d->name);
#endif
      def_file_add_directive (def, d->name, d->len);
      directives = d->next;
      free (d->name);
      free (d);
    }
  def_pool_free ();

  return def;
}

void
def_file_free (def_file *fdef)
{
  int i;

  if (!fdef)
    return;
  if (fdef->name)
    free (fdef->name);
  if (fdef->description)
    free (fdef->description);

  if (fdef->section_defs)
    {
      for (i = 0; i < fdef->num_section_defs; i++)
	{
	  if (fdef->section_defs[i].name)
	    free (fdef->section_defs[i].name);
	  if (fdef->section_defs[i].class)
	    free (fdef->section_defs[i].class);
	}
      free (fdef->section_defs);
    }

  if (fdef->exports)
    {
      for (i = 0; i < fdef->num_exports; i++)
	{
	  if (fdef->exports[i].internal_name
	      && fdef->exports[i].internal_name != fdef->exports[i].name)
	    free (fdef->exports[i].internal_name);
	  if (fdef->exports[i].name)
	    free (fdef->exports[i].name);
	  if (fdef->exports[i].its_name)
	    free (fdef->exports[i].its_name);
	}
      free (fdef->exports);
    }

  if (fdef->imports)
    {
      for (i = 0; i < fdef->num_imports; i++)
	{
	  if (fdef->imports[i].internal_name
	      && fdef->imports[i].internal_name != fdef->imports[i].name)
	    free (fdef->imports[i].internal_name);
	  if (fdef->imports[i].name)
	    free (fdef->imports[i].name);
	  if (fdef->imports[i].its_name)
	    free (fdef->imports[i].its_name);
	}
      free (fdef->imports);
    }

  while (fdef->modules)
    {
      def_file_module *m = fdef->modules;

      fdef->modules = fdef->modules->next;
      free (m);
    }

  while (fdef->aligncomms)
    {
      def_file_aligncomm *c = fdef->aligncomms;

      fdef->aligncomms = fdef->aligncomms->next;
      free (c->symbol_name);
      free (c);
    }

  free (fdef);
}

#ifdef DEF_FILE_PRINT
void
def_file_print (FILE *file, def_file *fdef)
{
  int i;

  fprintf (file, ">>>> def_file at 0x%08x\n", fdef);
  if (fdef->name)
    fprintf (file, "  name: %s\n", fdef->name ? fdef->name : "(unspecified)");
  if (fdef->is_dll != -1)
    fprintf (file, "  is dll: %s\n", fdef->is_dll ? "yes" : "no");
  if (fdef->base_address != (bfd_vma) -1)
    {
      fprintf (file, "  base address: 0x");
      fprintf_vma (file, fdef->base_address);
      fprintf (file, "\n");
    }
  if (fdef->description)
    fprintf (file, "  description: `%s'\n", fdef->description);
  if (fdef->stack_reserve != -1)
    fprintf (file, "  stack reserve: 0x%08x\n", fdef->stack_reserve);
  if (fdef->stack_commit != -1)
    fprintf (file, "  stack commit: 0x%08x\n", fdef->stack_commit);
  if (fdef->heap_reserve != -1)
    fprintf (file, "  heap reserve: 0x%08x\n", fdef->heap_reserve);
  if (fdef->heap_commit != -1)
    fprintf (file, "  heap commit: 0x%08x\n", fdef->heap_commit);

  if (fdef->num_section_defs > 0)
    {
      fprintf (file, "  section defs:\n");

      for (i = 0; i < fdef->num_section_defs; i++)
	{
	  fprintf (file, "    name: `%s', class: `%s', flags:",
		   fdef->section_defs[i].name, fdef->section_defs[i].class);
	  if (fdef->section_defs[i].flag_read)
	    fprintf (file, " R");
	  if (fdef->section_defs[i].flag_write)
	    fprintf (file, " W");
	  if (fdef->section_defs[i].flag_execute)
	    fprintf (file, " X");
	  if (fdef->section_defs[i].flag_shared)
	    fprintf (file, " S");
	  fprintf (file, "\n");
	}
    }

  if (fdef->num_exports > 0)
    {
      fprintf (file, "  exports:\n");

      for (i = 0; i < fdef->num_exports; i++)
	{
	  fprintf (file, "    name: `%s', int: `%s', ordinal: %d, flags:",
		   fdef->exports[i].name, fdef->exports[i].internal_name,
		   fdef->exports[i].ordinal);
	  if (fdef->exports[i].flag_private)
	    fprintf (file, " P");
	  if (fdef->exports[i].flag_constant)
	    fprintf (file, " C");
	  if (fdef->exports[i].flag_noname)
	    fprintf (file, " N");
	  if (fdef->exports[i].flag_data)
	    fprintf (file, " D");
	  fprintf (file, "\n");
	}
    }

  if (fdef->num_imports > 0)
    {
      fprintf (file, "  imports:\n");

      for (i = 0; i < fdef->num_imports; i++)
	{
	  fprintf (file, "    int: %s, from: `%s', name: `%s', ordinal: %d\n",
		   fdef->imports[i].internal_name,
		   fdef->imports[i].module,
		   fdef->imports[i].name,
		   fdef->imports[i].ordinal);
	}
    }

  if (fdef->version_major != -1)
    fprintf (file, "  version: %d.%d\n", fdef->version_major, fdef->version_minor);

  fprintf (file, "<<<< def_file at 0x%08x\n", fdef);
}
#endif

/* Helper routine to check for identity of string pointers,
   which might be NULL.  */

static int
are_names_equal (const char *s1, const char *s2)
{
  if (!s1 && !s2)
    return 0;
  if (!s1 || !s2)
    return (!s1 ? -1 : 1);
  return strcmp (s1, s2);
}

static int
cmp_export_elem (const def_file_export *e, const char *ex_name,
		 const char *in_name, const char *its_name,
		 int ord)
{
  int r;

  if ((r = are_names_equal (ex_name, e->name)) != 0)
    return r;
  if ((r = are_names_equal (in_name, e->internal_name)) != 0)
    return r;
  if ((r = are_names_equal (its_name, e->its_name)) != 0)
    return r;
  return (ord - e->ordinal);
}

/* Search the position of the identical element, or returns the position
   of the next higher element. If last valid element is smaller, then MAX
   is returned.  */

static int
find_export_in_list (def_file_export *b, int max,
		     const char *ex_name, const char *in_name,
		     const char *its_name, int ord, int *is_ident)
{
  int e, l, r, p;

  *is_ident = 0;
  if (!max)
    return 0;
  if ((e = cmp_export_elem (b, ex_name, in_name, its_name, ord)) <= 0)
    {
      if (!e)
        *is_ident = 1;
      return 0;
    }
  if (max == 1)
    return 1;
  if ((e = cmp_export_elem (b + (max - 1), ex_name, in_name, its_name, ord)) > 0)
    return max;
  else if (!e || max == 2)
    {
      if (!e)
	*is_ident = 1;
      return max - 1;
    }
  l = 0; r = max - 1;
  while (l < r)
    {
      p = (l + r) / 2;
      e = cmp_export_elem (b + p, ex_name, in_name, its_name, ord);
      if (!e)
        {
          *is_ident = 1;
          return p;
        }
      else if (e < 0)
        r = p - 1;
      else if (e > 0)
        l = p + 1;
    }
  if ((e = cmp_export_elem (b + l, ex_name, in_name, its_name, ord)) > 0)
    ++l;
  else if (!e)
    *is_ident = 1;
  return l;
}

def_file_export *
def_file_add_export (def_file *fdef,
		     const char *external_name,
		     const char *internal_name,
		     int ordinal,
		     const char *its_name,
		     int *is_dup)
{
  def_file_export *e;
  int pos;
  int max_exports = ROUND_UP(fdef->num_exports, 32);

  if (internal_name && !external_name)
    external_name = internal_name;
  if (external_name && !internal_name)
    internal_name = external_name;

  /* We need to avoid duplicates.  */
  *is_dup = 0;
  pos = find_export_in_list (fdef->exports, fdef->num_exports,
		     external_name, internal_name,
		     its_name, ordinal, is_dup);

  if (*is_dup != 0)
    return (fdef->exports + pos);

  if (fdef->num_exports >= max_exports)
    {
      max_exports = ROUND_UP(fdef->num_exports + 1, 32);
      if (fdef->exports)
	fdef->exports = xrealloc (fdef->exports,
				 max_exports * sizeof (def_file_export));
      else
	fdef->exports = xmalloc (max_exports * sizeof (def_file_export));
    }

  e = fdef->exports + pos;
  if (pos != fdef->num_exports)
    memmove (&e[1], e, (sizeof (def_file_export) * (fdef->num_exports - pos)));
  memset (e, 0, sizeof (def_file_export));
  e->name = xstrdup (external_name);
  e->internal_name = xstrdup (internal_name);
  e->its_name = (its_name ? xstrdup (its_name) : NULL);
  e->ordinal = ordinal;
  fdef->num_exports++;
  return e;
}

def_file_module *
def_get_module (def_file *fdef, const char *name)
{
  def_file_module *s;

  for (s = fdef->modules; s; s = s->next)
    if (strcmp (s->name, name) == 0)
      return s;

  return NULL;
}

static def_file_module *
def_stash_module (def_file *fdef, const char *name)
{
  def_file_module *s;

  if ((s = def_get_module (fdef, name)) != NULL)
      return s;
  s = xmalloc (sizeof (def_file_module) + strlen (name));
  s->next = fdef->modules;
  fdef->modules = s;
  s->user_data = 0;
  strcpy (s->name, name);
  return s;
}

static int
cmp_import_elem (const def_file_import *e, const char *ex_name,
		 const char *in_name, const char *module,
		 int ord)
{
  int r;

  if ((r = are_names_equal (module, (e->module ? e->module->name : NULL))))
    return r;
  if ((r = are_names_equal (ex_name, e->name)) != 0)
    return r;
  if ((r = are_names_equal (in_name, e->internal_name)) != 0)
    return r;
  if (ord != e->ordinal)
    return (ord < e->ordinal ? -1 : 1);
  return 0;
}

/* Search the position of the identical element, or returns the position
   of the next higher element. If last valid element is smaller, then MAX
   is returned.  */

static int
find_import_in_list (def_file_import *b, int max,
		     const char *ex_name, const char *in_name,
		     const char *module, int ord, int *is_ident)
{
  int e, l, r, p;

  *is_ident = 0;
  if (!max)
    return 0;
  if ((e = cmp_import_elem (b, ex_name, in_name, module, ord)) <= 0)
    {
      if (!e)
        *is_ident = 1;
      return 0;
    }
  if (max == 1)
    return 1;
  if ((e = cmp_import_elem (b + (max - 1), ex_name, in_name, module, ord)) > 0)
    return max;
  else if (!e || max == 2)
    {
      if (!e)
        *is_ident = 1;
      return max - 1;
    }
  l = 0; r = max - 1;
  while (l < r)
    {
      p = (l + r) / 2;
      e = cmp_import_elem (b + p, ex_name, in_name, module, ord);
      if (!e)
        {
          *is_ident = 1;
          return p;
        }
      else if (e < 0)
        r = p - 1;
      else if (e > 0)
        l = p + 1;
    }
  if ((e = cmp_import_elem (b + l, ex_name, in_name, module, ord)) > 0)
    ++l;
  else if (!e)
    *is_ident = 1;
  return l;
}

def_file_import *
def_file_add_import (def_file *fdef,
		     const char *name,
		     const char *module,
		     int ordinal,
		     const char *internal_name,
		     const char *its_name,
		     int *is_dup)
{
  def_file_import *i;
  int pos;
  int max_imports = ROUND_UP (fdef->num_imports, 16);

  /* We need to avoid here duplicates.  */
  *is_dup = 0;
  pos = find_import_in_list (fdef->imports, fdef->num_imports,
			     name,
			     (!internal_name ? name : internal_name),
			     module, ordinal, is_dup);
  if (*is_dup != 0)
    return fdef->imports + pos;

  if (fdef->num_imports >= max_imports)
    {
      max_imports = ROUND_UP (fdef->num_imports+1, 16);

      if (fdef->imports)
	fdef->imports = xrealloc (fdef->imports,
				 max_imports * sizeof (def_file_import));
      else
	fdef->imports = xmalloc (max_imports * sizeof (def_file_import));
    }
  i = fdef->imports + pos;
  if (pos != fdef->num_imports)
    memmove (&i[1], i, (sizeof (def_file_import) * (fdef->num_imports - pos)));
  memset (i, 0, sizeof (def_file_import));
  if (name)
    i->name = xstrdup (name);
  if (module)
    i->module = def_stash_module (fdef, module);
  i->ordinal = ordinal;
  if (internal_name)
    i->internal_name = xstrdup (internal_name);
  else
    i->internal_name = i->name;
  i->its_name = (its_name ? xstrdup (its_name) : NULL);
  fdef->num_imports++;

  return i;
}

struct
{
  char *param;
  int token;
}
diropts[] =
{
  { "-heap", HEAPSIZE },
  { "-stack", STACKSIZE_K },
  { "-attr", SECTIONS },
  { "-export", EXPORTS },
  { "-aligncomm", ALIGNCOMM },
  { 0, 0 }
};

void
def_file_add_directive (def_file *my_def, const char *param, int len)
{
  def_file *save_def = def;
  const char *pend = param + len;
  char * tend = (char *) param;
  int i;

  def = my_def;

  while (param < pend)
    {
      while (param < pend
	     && (ISSPACE (*param) || *param == '\n' || *param == 0))
	param++;

      if (param == pend)
	break;

      /* Scan forward until we encounter any of:
          - the end of the buffer
	  - the start of a new option
	  - a newline seperating options
          - a NUL seperating options.  */
      for (tend = (char *) (param + 1);
	   (tend < pend
	    && !(ISSPACE (tend[-1]) && *tend == '-')
	    && *tend != '\n' && *tend != 0);
	   tend++)
	;

      for (i = 0; diropts[i].param; i++)
	{
	  len = strlen (diropts[i].param);

	  if (tend - param >= len
	      && strncmp (param, diropts[i].param, len) == 0
	      && (param[len] == ':' || param[len] == ' '))
	    {
	      lex_parse_string_end = tend;
	      lex_parse_string = param + len + 1;
	      lex_forced_token = diropts[i].token;
	      saw_newline = 0;
	      if (def_parse ())
		continue;
	      break;
	    }
	}

      if (!diropts[i].param)
	{
	  if (tend < pend)
	    {
	      char saved;

	      saved = * tend;
	      * tend = 0;
	      /* xgettext:c-format */
	      einfo (_("Warning: .drectve `%s' unrecognized\n"), param);
	      * tend = saved;
	    }
	  else
	    {
	      einfo (_("Warning: corrupt .drectve at end of def file\n"));
	    }
	}

      lex_parse_string = 0;
      param = tend;
    }

  def = save_def;
  def_pool_free ();
}

/* Parser Callbacks.  */

static void
def_image_name (const char *name, bfd_vma base, int is_dll)
{
  /* If a LIBRARY or NAME statement is specified without a name, there is nothing
     to do here.  We retain the output filename specified on command line.  */
  if (*name)
    {
      const char* image_name = lbasename (name);

      if (image_name != name)
	einfo ("%s:%d: Warning: path components stripped from %s, '%s'\n",
	       def_filename, linenumber, is_dll ? "LIBRARY" : "NAME",
	       name);
      if (def->name)
	free (def->name);
      /* Append the default suffix, if none specified.  */
      if (strchr (image_name, '.') == 0)
	{
	  const char * suffix = is_dll ? ".dll" : ".exe";

	  def->name = xmalloc (strlen (image_name) + strlen (suffix) + 1);
	  sprintf (def->name, "%s%s", image_name, suffix);
        }
      else
	def->name = xstrdup (image_name);
    }

  /* Honor a BASE address statement, even if LIBRARY string is empty.  */
  def->base_address = base;
  def->is_dll = is_dll;
}

static void
def_description (const char *text)
{
  int len = def->description ? strlen (def->description) : 0;

  len += strlen (text) + 1;
  if (def->description)
    {
      def->description = xrealloc (def->description, len);
      strcat (def->description, text);
    }
  else
    {
      def->description = xmalloc (len);
      strcpy (def->description, text);
    }
}

static void
def_stacksize (int reserve, int commit)
{
  def->stack_reserve = reserve;
  def->stack_commit = commit;
}

static void
def_heapsize (int reserve, int commit)
{
  def->heap_reserve = reserve;
  def->heap_commit = commit;
}

static void
def_section (const char *name, int attr)
{
  def_file_section *s;
  int max_sections = ROUND_UP (def->num_section_defs, 4);

  if (def->num_section_defs >= max_sections)
    {
      max_sections = ROUND_UP (def->num_section_defs+1, 4);

      if (def->section_defs)
	def->section_defs = xrealloc (def->section_defs,
				      max_sections * sizeof (def_file_import));
      else
	def->section_defs = xmalloc (max_sections * sizeof (def_file_import));
    }
  s = def->section_defs + def->num_section_defs;
  memset (s, 0, sizeof (def_file_section));
  s->name = xstrdup (name);
  if (attr & 1)
    s->flag_read = 1;
  if (attr & 2)
    s->flag_write = 1;
  if (attr & 4)
    s->flag_execute = 1;
  if (attr & 8)
    s->flag_shared = 1;

  def->num_section_defs++;
}

static void
def_section_alt (const char *name, const char *attr)
{
  int aval = 0;

  for (; *attr; attr++)
    {
      switch (*attr)
	{
	case 'R':
	case 'r':
	  aval |= 1;
	  break;
	case 'W':
	case 'w':
	  aval |= 2;
	  break;
	case 'X':
	case 'x':
	  aval |= 4;
	  break;
	case 'S':
	case 's':
	  aval |= 8;
	  break;
	}
    }
  def_section (name, aval);
}

static void
def_exports (const char *external_name,
	     const char *internal_name,
	     int ordinal,
	     int flags,
	     const char *its_name)
{
  def_file_export *dfe;
  int is_dup = 0;

  if (!internal_name && external_name)
    internal_name = external_name;
#if TRACE
  printf ("def_exports, ext=%s int=%s\n", external_name, internal_name);
#endif

  dfe = def_file_add_export (def, external_name, internal_name, ordinal,
			     its_name, &is_dup);

  /* We might check here for flag redefinition and warn.  For now we
     ignore duplicates silently.  */
  if (is_dup)
    return;

  if (flags & 1)
    dfe->flag_noname = 1;
  if (flags & 2)
    dfe->flag_constant = 1;
  if (flags & 4)
    dfe->flag_data = 1;
  if (flags & 8)
    dfe->flag_private = 1;
}

static void
def_import (const char *internal_name,
	    const char *module,
	    const char *dllext,
	    const char *name,
	    int ordinal,
	    const char *its_name)
{
  char *buf = 0;
  const char *ext = dllext ? dllext : "dll";
  int is_dup = 0;

  buf = xmalloc (strlen (module) + strlen (ext) + 2);
  sprintf (buf, "%s.%s", module, ext);
  module = buf;

  def_file_add_import (def, name, module, ordinal, internal_name, its_name,
		       &is_dup);
  free (buf);
}

static void
def_version (int major, int minor)
{
  def->version_major = major;
  def->version_minor = minor;
}

static void
def_directive (char *str)
{
  struct directive *d = xmalloc (sizeof (struct directive));

  d->next = directives;
  directives = d;
  d->name = xstrdup (str);
  d->len = strlen (str);
}

static void
def_aligncomm (char *str, int align)
{
  def_file_aligncomm *c, *p;

  p = NULL;
  c = def->aligncomms;
  while (c != NULL)
    {
      int e = strcmp (c->symbol_name, str);
      if (!e)
	{
	  /* Not sure if we want to allow here duplicates with
	     different alignments, but for now we keep them.  */
	  e = (int) c->alignment - align;
	  if (!e)
	    return;
	}
      if (e > 0)
        break;
      c = (p = c)->next;
    }

  c = xmalloc (sizeof (def_file_aligncomm));
  c->symbol_name = xstrdup (str);
  c->alignment = (unsigned int) align;
  if (!p)
    {
      c->next = def->aligncomms;
      def->aligncomms = c;
    }
  else
    {
      c->next = p->next;
      p->next = c;
    }
}

static int
def_error (const char *err)
{
  einfo ("%P: %s:%d: %s\n",
	 def_filename ? def_filename : "<unknown-file>", linenumber, err);
  return 0;
}


/* Lexical Scanner.  */

#undef TRACE
#define TRACE 0

/* Never freed, but always reused as needed, so no real leak.  */
static char *buffer = 0;
static int buflen = 0;
static int bufptr = 0;

static void
put_buf (char c)
{
  if (bufptr == buflen)
    {
      buflen += 50;		/* overly reasonable, eh?  */
      if (buffer)
	buffer = xrealloc (buffer, buflen + 1);
      else
	buffer = xmalloc (buflen + 1);
    }
  buffer[bufptr++] = c;
  buffer[bufptr] = 0;		/* not optimal, but very convenient.  */
}

static struct
{
  char *name;
  int token;
}
tokens[] =
{
  { "BASE", BASE },
  { "CODE", CODE },
  { "CONSTANT", CONSTANTU },
  { "constant", CONSTANTL },
  { "DATA", DATAU },
  { "data", DATAL },
  { "DESCRIPTION", DESCRIPTION },
  { "DIRECTIVE", DIRECTIVE },
  { "EXECUTE", EXECUTE },
  { "EXPORTS", EXPORTS },
  { "HEAPSIZE", HEAPSIZE },
  { "IMPORTS", IMPORTS },
  { "LIBRARY", LIBRARY },
  { "NAME", NAME },
  { "NONAME", NONAMEU },
  { "noname", NONAMEL },
  { "PRIVATE", PRIVATEU },
  { "private", PRIVATEL },
  { "READ", READ },
  { "SECTIONS", SECTIONS },
  { "SEGMENTS", SECTIONS },
  { "SHARED", SHARED },
  { "STACKSIZE", STACKSIZE_K },
  { "VERSION", VERSIONK },
  { "WRITE", WRITE },
  { 0, 0 }
};

static int
def_getc (void)
{
  int rv;

  if (lex_parse_string)
    {
      if (lex_parse_string >= lex_parse_string_end)
	rv = EOF;
      else
	rv = *lex_parse_string++;
    }
  else
    {
      rv = fgetc (the_file);
    }
  if (rv == '\n')
    saw_newline = 1;
  return rv;
}

static int
def_ungetc (int c)
{
  if (lex_parse_string)
    {
      lex_parse_string--;
      return c;
    }
  else
    return ungetc (c, the_file);
}

static int
def_lex (void)
{
  int c, i, q;

  if (lex_forced_token)
    {
      i = lex_forced_token;
      lex_forced_token = 0;
#if TRACE
      printf ("lex: forcing token %d\n", i);
#endif
      return i;
    }

  c = def_getc ();

  /* Trim leading whitespace.  */
  while (c != EOF && (c == ' ' || c == '\t') && saw_newline)
    c = def_getc ();

  if (c == EOF)
    {
#if TRACE
      printf ("lex: EOF\n");
#endif
      return 0;
    }

  if (saw_newline && c == ';')
    {
      do
	{
	  c = def_getc ();
	}
      while (c != EOF && c != '\n');
      if (c == '\n')
	return def_lex ();
      return 0;
    }

  /* Must be something else.  */
  saw_newline = 0;

  if (ISDIGIT (c))
    {
      bufptr = 0;
      while (c != EOF && (ISXDIGIT (c) || (c == 'x')))
	{
	  put_buf (c);
	  c = def_getc ();
	}
      if (c != EOF)
	def_ungetc (c);
      yylval.digits = def_pool_strdup (buffer);
#if TRACE
      printf ("lex: `%s' returns DIGITS\n", buffer);
#endif
      return DIGITS;
    }

  if (ISALPHA (c) || strchr ("$:-_?@", c))
    {
      bufptr = 0;
      q = c;
      put_buf (c);
      c = def_getc ();

      if (q == '@')
	{
          if (ISBLANK (c) ) /* '@' followed by whitespace.  */
	    return (q);
          else if (ISDIGIT (c)) /* '@' followed by digit.  */
            {
	      def_ungetc (c);
              return (q);
	    }
#if TRACE
	  printf ("lex: @ returns itself\n");
#endif
	}

      while (c != EOF && (ISALNUM (c) || strchr ("$:-_?/@<>", c)))
	{
	  put_buf (c);
	  c = def_getc ();
	}
      if (c != EOF)
	def_ungetc (c);
      if (ISALPHA (q)) /* Check for tokens.  */
	{
          for (i = 0; tokens[i].name; i++)
	    if (strcmp (tokens[i].name, buffer) == 0)
	      {
#if TRACE
	        printf ("lex: `%s' is a string token\n", buffer);
#endif
	        return tokens[i].token;
	      }
	}
#if TRACE
      printf ("lex: `%s' returns ID\n", buffer);
#endif
      yylval.id = def_pool_strdup (buffer);
      return ID;
    }

  if (c == '\'' || c == '"')
    {
      q = c;
      c = def_getc ();
      bufptr = 0;

      while (c != EOF && c != q)
	{
	  put_buf (c);
	  c = def_getc ();
	}
      yylval.id = def_pool_strdup (buffer);
#if TRACE
      printf ("lex: `%s' returns ID\n", buffer);
#endif
      return ID;
    }

  if ( c == '=')
    {
      c = def_getc ();
      if (c == '=')
        {
#if TRACE
          printf ("lex: `==' returns EQUAL\n");
#endif
		  return EQUAL;
        }
      def_ungetc (c);
#if TRACE
      printf ("lex: `=' returns itself\n");
#endif
      return '=';
    }
  if (c == '.' || c == ',')
    {
#if TRACE
      printf ("lex: `%c' returns itself\n", c);
#endif
      return c;
    }

  if (c == '\n')
    {
      linenumber++;
      saw_newline = 1;
    }

  /*printf ("lex: 0x%02x ignored\n", c); */
  return def_lex ();
}

static char *
def_pool_alloc (size_t sz)
{
  def_pool_str *e;

  e = (def_pool_str *) xmalloc (sizeof (def_pool_str) + sz);
  e->next = pool_strs;
  pool_strs = e;
  return e->data;
}

static char *
def_pool_strdup (const char *str)
{
  char *s;
  size_t len;
  if (!str)
    return NULL;
  len = strlen (str) + 1;
  s = def_pool_alloc (len);
  memcpy (s, str, len);
  return s;
}

static void
def_pool_free (void)
{
  def_pool_str *p;
  while ((p = pool_strs) != NULL)
    {
      pool_strs = p->next;
      free (p);
    }
}

