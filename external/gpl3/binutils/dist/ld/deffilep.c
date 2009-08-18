/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* Bison version.  */
#define YYBISON_VERSION "2.1"

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
     STACKSIZE = 261,
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
     READ = 275,
     WRITE = 276,
     EXECUTE = 277,
     SHARED = 278,
     NONAMEU = 279,
     NONAMEL = 280,
     DIRECTIVE = 281,
     ID = 282,
     NUMBER = 283
   };
#endif
/* Tokens.  */
#define NAME 258
#define LIBRARY 259
#define DESCRIPTION 260
#define STACKSIZE 261
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
#define READ 275
#define WRITE 276
#define EXECUTE 277
#define SHARED 278
#define NONAMEU 279
#define NONAMEL 280
#define DIRECTIVE 281
#define ID 282
#define NUMBER 283




/* Copy the first part of user declarations.  */
#line 1 "deffilep.y"
 /* deffilep.y - parser for .def files */

/*   Copyright 1995, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2007
     Free Software Foundation, Inc.

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

static void def_description (const char *);
static void def_exports (const char *, const char *, int, int);
static void def_heapsize (int, int);
static void def_import (const char *, const char *, const char *, const char *,
			int);
static void def_image_name (const char *, int, int);
static void def_section (const char *, int);
static void def_section_alt (const char *, const char *);
static void def_stacksize (int, int);
static void def_version (int, int);
static void def_directive (char *);
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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 102 "deffilep.y"
typedef union YYSTYPE {
  char *id;
  int number;
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 247 "deffilep.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 259 "deffilep.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  40
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   121

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  33
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  20
/* YYNRULES -- Number of rules. */
#define YYNRULES  63
/* YYNRULES -- Number of states. */
#define YYNSTATES  96

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   283

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    31,     2,    29,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    30,     2,     2,    32,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     6,     8,    12,    16,    19,    23,    27,
      30,    33,    36,    39,    42,    45,    50,    53,    54,    56,
      59,    65,    69,    70,    72,    74,    76,    78,    80,    82,
      84,    86,    89,    91,    99,   107,   113,   119,   125,   129,
     132,   134,   137,   140,   144,   146,   148,   149,   152,   153,
     155,   157,   159,   161,   163,   167,   168,   171,   172,   175,
     176,   180,   181,   183
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      34,     0,    -1,    34,    35,    -1,    35,    -1,     3,    48,
      51,    -1,     4,    48,    51,    -1,     5,    27,    -1,     6,
      28,    46,    -1,     7,    28,    46,    -1,     8,    44,    -1,
       9,    44,    -1,    11,    42,    -1,    12,    36,    -1,    13,
      40,    -1,    14,    28,    -1,    14,    28,    29,    28,    -1,
      26,    27,    -1,    -1,    37,    -1,    36,    37,    -1,    52,
      50,    49,    45,    38,    -1,    39,    45,    38,    -1,    -1,
      24,    -1,    25,    -1,    16,    -1,    17,    -1,     9,    -1,
      10,    -1,    18,    -1,    19,    -1,    40,    41,    -1,    41,
      -1,    27,    30,    27,    29,    27,    29,    27,    -1,    27,
      30,    27,    29,    27,    29,    28,    -1,    27,    30,    27,
      29,    27,    -1,    27,    30,    27,    29,    28,    -1,    27,
      29,    27,    29,    27,    -1,    27,    29,    27,    -1,    42,
      43,    -1,    43,    -1,    27,    44,    -1,    27,    27,    -1,
      44,    45,    47,    -1,    47,    -1,    31,    -1,    -1,    31,
      28,    -1,    -1,    20,    -1,    21,    -1,    22,    -1,    23,
      -1,    27,    -1,    27,    29,    27,    -1,    -1,    32,    28,
      -1,    -1,    30,    52,    -1,    -1,    15,    30,    28,    -1,
      -1,    27,    -1,    52,    29,    27,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,   119,   119,   120,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   140,   142,   143,
     150,   157,   158,   161,   162,   163,   164,   165,   166,   167,
     168,   171,   172,   176,   177,   178,   179,   180,   181,   185,
     186,   190,   191,   195,   196,   200,   201,   203,   204,   208,
     209,   210,   211,   214,   215,   221,   225,   226,   230,   231,
     234,   235,   238,   239
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NAME", "LIBRARY", "DESCRIPTION",
  "STACKSIZE", "HEAPSIZE", "CODE", "DATAU", "DATAL", "SECTIONS", "EXPORTS",
  "IMPORTS", "VERSIONK", "BASE", "CONSTANTU", "CONSTANTL", "PRIVATEU",
  "PRIVATEL", "READ", "WRITE", "EXECUTE", "SHARED", "NONAMEU", "NONAMEL",
  "DIRECTIVE", "ID", "NUMBER", "'.'", "'='", "','", "'@'", "$accept",
  "start", "command", "explist", "expline", "exp_opt_list", "exp_opt",
  "implist", "impline", "seclist", "secline", "attr_list", "opt_comma",
  "opt_number", "attr", "opt_name", "opt_ordinal", "opt_equal_name",
  "opt_base", "dot_name", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,    46,
      61,    44,    64
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    33,    34,    34,    35,    35,    35,    35,    35,    35,
      35,    35,    35,    35,    35,    35,    35,    36,    36,    36,
      37,    38,    38,    39,    39,    39,    39,    39,    39,    39,
      39,    40,    40,    41,    41,    41,    41,    41,    41,    42,
      42,    43,    43,    44,    44,    45,    45,    46,    46,    47,
      47,    47,    47,    48,    48,    48,    49,    49,    50,    50,
      51,    51,    52,    52
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     2,     1,     3,     3,     2,     3,     3,     2,
       2,     2,     2,     2,     2,     4,     2,     0,     1,     2,
       5,     3,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     1,     7,     7,     5,     5,     5,     3,     2,
       1,     2,     2,     3,     1,     1,     0,     2,     0,     1,
       1,     1,     1,     1,     3,     0,     2,     0,     2,     0,
       3,     0,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,    55,    55,     0,     0,     0,     0,     0,     0,    17,
       0,     0,     0,     0,     3,    53,    61,    61,     6,    48,
      48,    49,    50,    51,    52,     9,    44,    10,     0,    11,
      40,    62,    12,    18,    59,     0,    13,    32,    14,    16,
       1,     2,     0,     0,     4,     5,     0,     7,     8,    45,
       0,    42,    41,    39,    19,     0,     0,    57,     0,     0,
      31,     0,    54,     0,    47,    43,    63,    58,     0,    46,
      38,     0,    15,    60,    56,    22,     0,     0,    27,    28,
      25,    26,    29,    30,    23,    24,    20,    46,    37,    35,
      36,    22,     0,    21,    33,    34
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    13,    14,    32,    33,    86,    87,    36,    37,    29,
      30,    25,    50,    47,    26,    16,    69,    57,    44,    34
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -55
static const yysigned_char yypact[] =
{
      16,    -9,    -9,     5,    22,    29,    41,    41,    31,    32,
      38,    39,    42,     0,   -55,    37,    53,    53,   -55,    40,
      40,   -55,   -55,   -55,   -55,    18,   -55,    18,    33,    31,
     -55,   -55,    32,   -55,   -28,   -13,    38,   -55,    43,   -55,
     -55,   -55,    46,    44,   -55,   -55,    47,   -55,   -55,   -55,
      41,   -55,    18,   -55,   -55,    49,    32,    45,    51,    52,
     -55,    54,   -55,    55,   -55,   -55,   -55,    56,    58,    50,
      59,    60,   -55,   -55,   -55,    27,    57,     7,   -55,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,   -55,    50,   -55,    61,
     -55,    27,    20,   -55,   -55,   -55
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -55,   -55,    67,   -55,    62,   -21,   -55,   -55,    63,   -55,
      64,     3,   -54,    71,    48,    85,   -55,   -55,    75,    65
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -47
static const yysigned_char yytable[] =
{
      40,    55,    56,     1,     2,     3,     4,     5,     6,     7,
      27,     8,     9,    10,    11,    75,    58,    59,    15,     1,
       2,     3,     4,     5,     6,     7,    12,     8,     9,    10,
      11,    52,    18,    91,    89,    90,    78,    79,   -46,   -46,
     -46,   -46,    12,    80,    81,    82,    83,    94,    95,    49,
      19,    84,    85,    21,    22,    23,    24,    20,    28,    31,
      51,    21,    22,    23,    24,    35,    42,    38,    43,    39,
      93,    46,    61,    62,    63,    64,    66,    68,    70,    71,
      41,    49,    72,    73,    88,    55,    74,    17,    76,    77,
      92,    48,    45,    53,    54,     0,     0,     0,    65,    60,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    67
};

static const yysigned_char yycheck[] =
{
       0,    29,    30,     3,     4,     5,     6,     7,     8,     9,
       7,    11,    12,    13,    14,    69,    29,    30,    27,     3,
       4,     5,     6,     7,     8,     9,    26,    11,    12,    13,
      14,    28,    27,    87,    27,    28,     9,    10,    20,    21,
      22,    23,    26,    16,    17,    18,    19,    27,    28,    31,
      28,    24,    25,    20,    21,    22,    23,    28,    27,    27,
      27,    20,    21,    22,    23,    27,    29,    28,    15,    27,
      91,    31,    29,    27,    30,    28,    27,    32,    27,    27,
      13,    31,    28,    28,    27,    29,    28,     2,    29,    29,
      29,    20,    17,    29,    32,    -1,    -1,    -1,    50,    36,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    56
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    11,    12,
      13,    14,    26,    34,    35,    27,    48,    48,    27,    28,
      28,    20,    21,    22,    23,    44,    47,    44,    27,    42,
      43,    27,    36,    37,    52,    27,    40,    41,    28,    27,
       0,    35,    29,    15,    51,    51,    31,    46,    46,    31,
      45,    27,    44,    43,    37,    29,    30,    50,    29,    30,
      41,    29,    27,    30,    28,    47,    27,    52,    32,    49,
      27,    27,    28,    28,    28,    45,    29,    29,     9,    10,
      16,    17,    18,    19,    24,    25,    38,    39,    27,    27,
      28,    45,    29,    38,    27,    28
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
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
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
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
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
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
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
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
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
  const char *yys = yystr;

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
      size_t yyn = 0;
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

#endif /* YYERROR_VERBOSE */



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
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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
    ;
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

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



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
	short int *yyss1 = yyss;


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
	short int *yyss1 = yyss;
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

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

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

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

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
        case 4:
#line 124 "deffilep.y"
    { def_image_name ((yyvsp[-1].id), (yyvsp[0].number), 0); }
    break;

  case 5:
#line 125 "deffilep.y"
    { def_image_name ((yyvsp[-1].id), (yyvsp[0].number), 1); }
    break;

  case 6:
#line 126 "deffilep.y"
    { def_description ((yyvsp[0].id));}
    break;

  case 7:
#line 127 "deffilep.y"
    { def_stacksize ((yyvsp[-1].number), (yyvsp[0].number));}
    break;

  case 8:
#line 128 "deffilep.y"
    { def_heapsize ((yyvsp[-1].number), (yyvsp[0].number));}
    break;

  case 9:
#line 129 "deffilep.y"
    { def_section ("CODE", (yyvsp[0].number));}
    break;

  case 10:
#line 130 "deffilep.y"
    { def_section ("DATA", (yyvsp[0].number));}
    break;

  case 14:
#line 134 "deffilep.y"
    { def_version ((yyvsp[0].number), 0);}
    break;

  case 15:
#line 135 "deffilep.y"
    { def_version ((yyvsp[-2].number), (yyvsp[0].number));}
    break;

  case 16:
#line 136 "deffilep.y"
    { def_directive ((yyvsp[0].id));}
    break;

  case 20:
#line 151 "deffilep.y"
    { def_exports ((yyvsp[-4].id), (yyvsp[-3].id), (yyvsp[-2].number), (yyvsp[0].number)); }
    break;

  case 21:
#line 157 "deffilep.y"
    { (yyval.number) = (yyvsp[-2].number) | (yyvsp[0].number); }
    break;

  case 22:
#line 158 "deffilep.y"
    { (yyval.number) = 0; }
    break;

  case 23:
#line 161 "deffilep.y"
    { (yyval.number) = 1; }
    break;

  case 24:
#line 162 "deffilep.y"
    { (yyval.number) = 1; }
    break;

  case 25:
#line 163 "deffilep.y"
    { (yyval.number) = 2; }
    break;

  case 26:
#line 164 "deffilep.y"
    { (yyval.number) = 2; }
    break;

  case 27:
#line 165 "deffilep.y"
    { (yyval.number) = 4; }
    break;

  case 28:
#line 166 "deffilep.y"
    { (yyval.number) = 4; }
    break;

  case 29:
#line 167 "deffilep.y"
    { (yyval.number) = 8; }
    break;

  case 30:
#line 168 "deffilep.y"
    { (yyval.number) = 8; }
    break;

  case 33:
#line 176 "deffilep.y"
    { def_import ((yyvsp[-6].id), (yyvsp[-4].id), (yyvsp[-2].id), (yyvsp[0].id), -1); }
    break;

  case 34:
#line 177 "deffilep.y"
    { def_import ((yyvsp[-6].id), (yyvsp[-4].id), (yyvsp[-2].id),  0, (yyvsp[0].number)); }
    break;

  case 35:
#line 178 "deffilep.y"
    { def_import ((yyvsp[-4].id), (yyvsp[-2].id),  0, (yyvsp[0].id), -1); }
    break;

  case 36:
#line 179 "deffilep.y"
    { def_import ((yyvsp[-4].id), (yyvsp[-2].id),  0,  0, (yyvsp[0].number)); }
    break;

  case 37:
#line 180 "deffilep.y"
    { def_import ( 0, (yyvsp[-4].id), (yyvsp[-2].id), (yyvsp[0].id), -1); }
    break;

  case 38:
#line 181 "deffilep.y"
    { def_import ( 0, (yyvsp[-2].id),  0, (yyvsp[0].id), -1); }
    break;

  case 41:
#line 190 "deffilep.y"
    { def_section ((yyvsp[-1].id), (yyvsp[0].number));}
    break;

  case 42:
#line 191 "deffilep.y"
    { def_section_alt ((yyvsp[-1].id), (yyvsp[0].id));}
    break;

  case 43:
#line 195 "deffilep.y"
    { (yyval.number) = (yyvsp[-2].number) | (yyvsp[0].number); }
    break;

  case 44:
#line 196 "deffilep.y"
    { (yyval.number) = (yyvsp[0].number); }
    break;

  case 47:
#line 203 "deffilep.y"
    { (yyval.number)=(yyvsp[0].number);}
    break;

  case 48:
#line 204 "deffilep.y"
    { (yyval.number)=-1;}
    break;

  case 49:
#line 208 "deffilep.y"
    { (yyval.number) = 1;}
    break;

  case 50:
#line 209 "deffilep.y"
    { (yyval.number) = 2;}
    break;

  case 51:
#line 210 "deffilep.y"
    { (yyval.number)=4;}
    break;

  case 52:
#line 211 "deffilep.y"
    { (yyval.number)=8;}
    break;

  case 53:
#line 214 "deffilep.y"
    { (yyval.id) = (yyvsp[0].id); }
    break;

  case 54:
#line 216 "deffilep.y"
    { 
	    char *name = xmalloc (strlen ((yyvsp[-2].id)) + 1 + strlen ((yyvsp[0].id)) + 1);
	    sprintf (name, "%s.%s", (yyvsp[-2].id), (yyvsp[0].id));
	    (yyval.id) = name;
	  }
    break;

  case 55:
#line 221 "deffilep.y"
    { (yyval.id) = ""; }
    break;

  case 56:
#line 225 "deffilep.y"
    { (yyval.number) = (yyvsp[0].number);}
    break;

  case 57:
#line 226 "deffilep.y"
    { (yyval.number) = -1;}
    break;

  case 58:
#line 230 "deffilep.y"
    { (yyval.id) = (yyvsp[0].id); }
    break;

  case 59:
#line 231 "deffilep.y"
    { (yyval.id) =  0; }
    break;

  case 60:
#line 234 "deffilep.y"
    { (yyval.number) = (yyvsp[0].number);}
    break;

  case 61:
#line 235 "deffilep.y"
    { (yyval.number) = -1;}
    break;

  case 62:
#line 238 "deffilep.y"
    { (yyval.id) = (yyvsp[0].id); }
    break;

  case 63:
#line 240 "deffilep.y"
    { 
	    char *name = xmalloc (strlen ((yyvsp[-2].id)) + 1 + strlen ((yyvsp[0].id)) + 1);
	    sprintf (name, "%s.%s", (yyvsp[-2].id), (yyvsp[0].id));
	    (yyval.id) = name;
	  }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 1588 "deffilep.c"

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
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
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
	  int yychecklim = YYLAST - yyn;
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
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
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
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
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
	  yydestruct ("Error: discarding", yytoken, &yylval);
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
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
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
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 248 "deffilep.y"


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
      return 0;
    }

  fclose (the_file);

  for (d = directives; d; d = d->next)
    {
#if TRACE
      printf ("Adding directive %08x `%s'\n", d->name, d->name);
#endif
      def_file_add_directive (def, d->name, d->len);
    }

  return def;
}

void
def_file_free (def_file *def)
{
  int i;

  if (!def)
    return;
  if (def->name)
    free (def->name);
  if (def->description)
    free (def->description);

  if (def->section_defs)
    {
      for (i = 0; i < def->num_section_defs; i++)
	{
	  if (def->section_defs[i].name)
	    free (def->section_defs[i].name);
	  if (def->section_defs[i].class)
	    free (def->section_defs[i].class);
	}
      free (def->section_defs);
    }

  if (def->exports)
    {
      for (i = 0; i < def->num_exports; i++)
	{
	  if (def->exports[i].internal_name
	      && def->exports[i].internal_name != def->exports[i].name)
	    free (def->exports[i].internal_name);
	  if (def->exports[i].name)
	    free (def->exports[i].name);
	}
      free (def->exports);
    }

  if (def->imports)
    {
      for (i = 0; i < def->num_imports; i++)
	{
	  if (def->imports[i].internal_name
	      && def->imports[i].internal_name != def->imports[i].name)
	    free (def->imports[i].internal_name);
	  if (def->imports[i].name)
	    free (def->imports[i].name);
	}
      free (def->imports);
    }

  while (def->modules)
    {
      def_file_module *m = def->modules;
      def->modules = def->modules->next;
      free (m);
    }

  free (def);
}

#ifdef DEF_FILE_PRINT
void
def_file_print (FILE *file, def_file *def)
{
  int i;

  fprintf (file, ">>>> def_file at 0x%08x\n", def);
  if (def->name)
    fprintf (file, "  name: %s\n", def->name ? def->name : "(unspecified)");
  if (def->is_dll != -1)
    fprintf (file, "  is dll: %s\n", def->is_dll ? "yes" : "no");
  if (def->base_address != (bfd_vma) -1)
    fprintf (file, "  base address: 0x%08x\n", def->base_address);
  if (def->description)
    fprintf (file, "  description: `%s'\n", def->description);
  if (def->stack_reserve != -1)
    fprintf (file, "  stack reserve: 0x%08x\n", def->stack_reserve);
  if (def->stack_commit != -1)
    fprintf (file, "  stack commit: 0x%08x\n", def->stack_commit);
  if (def->heap_reserve != -1)
    fprintf (file, "  heap reserve: 0x%08x\n", def->heap_reserve);
  if (def->heap_commit != -1)
    fprintf (file, "  heap commit: 0x%08x\n", def->heap_commit);

  if (def->num_section_defs > 0)
    {
      fprintf (file, "  section defs:\n");

      for (i = 0; i < def->num_section_defs; i++)
	{
	  fprintf (file, "    name: `%s', class: `%s', flags:",
		   def->section_defs[i].name, def->section_defs[i].class);
	  if (def->section_defs[i].flag_read)
	    fprintf (file, " R");
	  if (def->section_defs[i].flag_write)
	    fprintf (file, " W");
	  if (def->section_defs[i].flag_execute)
	    fprintf (file, " X");
	  if (def->section_defs[i].flag_shared)
	    fprintf (file, " S");
	  fprintf (file, "\n");
	}
    }

  if (def->num_exports > 0)
    {
      fprintf (file, "  exports:\n");

      for (i = 0; i < def->num_exports; i++)
	{
	  fprintf (file, "    name: `%s', int: `%s', ordinal: %d, flags:",
		   def->exports[i].name, def->exports[i].internal_name,
		   def->exports[i].ordinal);
	  if (def->exports[i].flag_private)
	    fprintf (file, " P");
	  if (def->exports[i].flag_constant)
	    fprintf (file, " C");
	  if (def->exports[i].flag_noname)
	    fprintf (file, " N");
	  if (def->exports[i].flag_data)
	    fprintf (file, " D");
	  fprintf (file, "\n");
	}
    }

  if (def->num_imports > 0)
    {
      fprintf (file, "  imports:\n");

      for (i = 0; i < def->num_imports; i++)
	{
	  fprintf (file, "    int: %s, from: `%s', name: `%s', ordinal: %d\n",
		   def->imports[i].internal_name,
		   def->imports[i].module,
		   def->imports[i].name,
		   def->imports[i].ordinal);
	}
    }

  if (def->version_major != -1)
    fprintf (file, "  version: %d.%d\n", def->version_major, def->version_minor);

  fprintf (file, "<<<< def_file at 0x%08x\n", def);
}
#endif

def_file_export *
def_file_add_export (def_file *def,
		     const char *external_name,
		     const char *internal_name,
		     int ordinal)
{
  def_file_export *e;
  int max_exports = ROUND_UP(def->num_exports, 32);

  if (def->num_exports >= max_exports)
    {
      max_exports = ROUND_UP(def->num_exports + 1, 32);
      if (def->exports)
	def->exports = xrealloc (def->exports,
				 max_exports * sizeof (def_file_export));
      else
	def->exports = xmalloc (max_exports * sizeof (def_file_export));
    }
  e = def->exports + def->num_exports;
  memset (e, 0, sizeof (def_file_export));
  if (internal_name && !external_name)
    external_name = internal_name;
  if (external_name && !internal_name)
    internal_name = external_name;
  e->name = xstrdup (external_name);
  e->internal_name = xstrdup (internal_name);
  e->ordinal = ordinal;
  def->num_exports++;
  return e;
}

def_file_module *
def_get_module (def_file *def, const char *name)
{
  def_file_module *s;

  for (s = def->modules; s; s = s->next)
    if (strcmp (s->name, name) == 0)
      return s;

  return NULL;
}

static def_file_module *
def_stash_module (def_file *def, const char *name)
{
  def_file_module *s;

  if ((s = def_get_module (def, name)) != NULL)
      return s;
  s = xmalloc (sizeof (def_file_module) + strlen (name));
  s->next = def->modules;
  def->modules = s;
  s->user_data = 0;
  strcpy (s->name, name);
  return s;
}

def_file_import *
def_file_add_import (def_file *def,
		     const char *name,
		     const char *module,
		     int ordinal,
		     const char *internal_name)
{
  def_file_import *i;
  int max_imports = ROUND_UP (def->num_imports, 16);

  if (def->num_imports >= max_imports)
    {
      max_imports = ROUND_UP (def->num_imports+1, 16);

      if (def->imports)
	def->imports = xrealloc (def->imports,
				 max_imports * sizeof (def_file_import));
      else
	def->imports = xmalloc (max_imports * sizeof (def_file_import));
    }
  i = def->imports + def->num_imports;
  memset (i, 0, sizeof (def_file_import));
  if (name)
    i->name = xstrdup (name);
  if (module)
    i->module = def_stash_module (def, module);
  i->ordinal = ordinal;
  if (internal_name)
    i->internal_name = xstrdup (internal_name);
  else
    i->internal_name = i->name;
  def->num_imports++;

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
  { "-stack", STACKSIZE },
  { "-attr", SECTIONS },
  { "-export", EXPORTS },
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
	  int len = strlen (diropts[i].param);

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
	  char saved;

	  saved = * tend;
	  * tend = 0;
	  /* xgettext:c-format */
	  einfo (_("Warning: .drectve `%s' unrecognized\n"), param);
	  * tend = saved;
	}

      lex_parse_string = 0;
      param = tend;
    }

  def = save_def;
}

/* Parser Callbacks.  */

static void
def_image_name (const char *name, int base, int is_dll)
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
	     int flags)
{
  def_file_export *dfe;

  if (!internal_name && external_name)
    internal_name = external_name;
#if TRACE
  printf ("def_exports, ext=%s int=%s\n", external_name, internal_name);
#endif

  dfe = def_file_add_export (def, external_name, internal_name, ordinal);
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
	    int ordinal)
{
  char *buf = 0;
  const char *ext = dllext ? dllext : "dll";    
   
  buf = xmalloc (strlen (module) + strlen (ext) + 2);
  sprintf (buf, "%s.%s", module, ext);
  module = buf;

  def_file_add_import (def, name, module, ordinal, internal_name);
  if (buf)
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
  { "STACKSIZE", STACKSIZE },
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
      yylval.number = strtoul (buffer, 0, 0);
#if TRACE
      printf ("lex: `%s' returns NUMBER %d\n", buffer, yylval.number);
#endif
      return NUMBER;
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

      while (c != EOF && (ISALNUM (c) || strchr ("$:-_?/@", c)))
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
      yylval.id = xstrdup (buffer);
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
      yylval.id = xstrdup (buffer);
#if TRACE
      printf ("lex: `%s' returns ID\n", buffer);
#endif
      return ID;
    }

  if (c == '=' || c == '.' || c == ',')
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

