
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 52 "go-exp.y"


#include "defs.h"
#include "gdb_string.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "go-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"

#define parse_type builtin_type (parse_gdbarch)

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in gdb.  Note that these are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list.  */

#define	yymaxdepth go_maxdepth
#define	yyparse	go_parse_internal
#define	yylex	go_lex
#define	yyerror	go_error
#define	yylval	go_lval
#define	yychar	go_char
#define	yydebug	go_debug
#define	yypact	go_pact
#define	yyr1	go_r1
#define	yyr2	go_r2
#define	yydef	go_def
#define	yychk	go_chk
#define	yypgo	go_pgo
#define	yyact	go_act
#define	yyexca	go_exca
#define yyerrflag go_errflag
#define yynerrs	go_nerrs
#define	yyps	go_ps
#define	yypv	go_pv
#define	yys	go_s
#define	yy_yys	go_yys
#define	yystate	go_state
#define	yytmp	go_tmp
#define	yyv	go_v
#define	yy_yyv	go_yyv
#define	yyval	go_val
#define	yylloc	go_lloc
#define yyreds	go_reds		/* With YYDEBUG defined */
#define yytoks	go_toks		/* With YYDEBUG defined */
#define yyname	go_name		/* With YYDEBUG defined */
#define yyrule	go_rule		/* With YYDEBUG defined */
#define yylhs	go_yylhs
#define yylen	go_yylen
#define yydefred go_yydefred
#define yydgoto	go_yydgoto
#define yysindex go_yysindex
#define yyrindex go_yyrindex
#define yygindex go_yygindex
#define yytable	 go_yytable
#define yycheck	 go_yycheck

#ifndef YYDEBUG
#define	YYDEBUG 1		/* Default to yydebug support */
#endif

#define YYFPRINTF parser_fprintf

int yyparse (void);

static int yylex (void);

void yyerror (char *);



/* Line 189 of yacc.c  */
#line 155 "go-exp.c"

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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT = 258,
     FLOAT = 259,
     RAW_STRING = 260,
     STRING = 261,
     CHAR = 262,
     NAME = 263,
     TYPENAME = 264,
     COMPLETE = 265,
     NAME_OR_INT = 266,
     TRUE_KEYWORD = 267,
     FALSE_KEYWORD = 268,
     STRUCT_KEYWORD = 269,
     INTERFACE_KEYWORD = 270,
     TYPE_KEYWORD = 271,
     CHAN_KEYWORD = 272,
     SIZEOF_KEYWORD = 273,
     LEN_KEYWORD = 274,
     CAP_KEYWORD = 275,
     NEW_KEYWORD = 276,
     IOTA_KEYWORD = 277,
     NIL_KEYWORD = 278,
     CONST_KEYWORD = 279,
     DOTDOTDOT = 280,
     ENTRY = 281,
     ERROR = 282,
     BYTE_KEYWORD = 283,
     DOLLAR_VARIABLE = 284,
     ASSIGN_MODIFY = 285,
     ABOVE_COMMA = 286,
     OROR = 287,
     ANDAND = 288,
     ANDNOT = 289,
     NOTEQUAL = 290,
     EQUAL = 291,
     GEQ = 292,
     LEQ = 293,
     RSH = 294,
     LSH = 295,
     DECREMENT = 296,
     INCREMENT = 297,
     UNARY = 298,
     LEFT_ARROW = 299
   };
#endif
/* Tokens.  */
#define INT 258
#define FLOAT 259
#define RAW_STRING 260
#define STRING 261
#define CHAR 262
#define NAME 263
#define TYPENAME 264
#define COMPLETE 265
#define NAME_OR_INT 266
#define TRUE_KEYWORD 267
#define FALSE_KEYWORD 268
#define STRUCT_KEYWORD 269
#define INTERFACE_KEYWORD 270
#define TYPE_KEYWORD 271
#define CHAN_KEYWORD 272
#define SIZEOF_KEYWORD 273
#define LEN_KEYWORD 274
#define CAP_KEYWORD 275
#define NEW_KEYWORD 276
#define IOTA_KEYWORD 277
#define NIL_KEYWORD 278
#define CONST_KEYWORD 279
#define DOTDOTDOT 280
#define ENTRY 281
#define ERROR 282
#define BYTE_KEYWORD 283
#define DOLLAR_VARIABLE 284
#define ASSIGN_MODIFY 285
#define ABOVE_COMMA 286
#define OROR 287
#define ANDAND 288
#define ANDNOT 289
#define NOTEQUAL 290
#define EQUAL 291
#define GEQ 292
#define LEQ 293
#define RSH 294
#define LSH 295
#define DECREMENT 296
#define INCREMENT 297
#define UNARY 298
#define LEFT_ARROW 299




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 138 "go-exp.y"

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      DOUBLEST dval;
      struct type *type;
    } typed_val_float;
    struct stoken sval;
    struct symtoken ssym;
    struct type *tval;
    struct typed_stoken tsval;
    struct ttype tsym;
    int voidval;
    enum exp_opcode opcode;
    struct internalvar *ivar;
    struct stoken_vector svec;
  


/* Line 214 of yacc.c  */
#line 302 "go-exp.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */

/* Line 264 of yacc.c  */
#line 159 "go-exp.y"

/* YYSTYPE gets defined by %union.  */
static int parse_number (char *, int, int, YYSTYPE *);
static int parse_go_float (struct gdbarch *gdbarch, const char *p, int len,
			   DOUBLEST *d, struct type **t);


/* Line 264 of yacc.c  */
#line 322 "go-exp.c"

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
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or xmalloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
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
       && ! ((defined YYMALLOC || defined xmalloc) \
	     && (defined YYFREE || defined xfree)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC xmalloc
#   if ! defined xmalloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *xmalloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE xfree
#   if ! defined xfree && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void xfree (void *); /* INFRINGES ON USER NAME SPACE */
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  40
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   443

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  13
/* YYNRULES -- Number of rules.  */
#define YYNRULES  69
/* YYNRULES -- Number of states.  */
#define YYNSTATES  122

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   299

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    62,     2,     2,     2,    54,    39,     2,
      60,    64,    52,    50,    31,    51,    58,    53,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    67,     2,
      43,    33,    44,    34,    49,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    59,     2,    63,    38,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    65,    37,    66,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    32,    35,    36,    40,
      41,    42,    45,    46,    47,    48,    55,    56,    57,    61
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    15,    18,    21,
      24,    27,    30,    33,    36,    39,    43,    48,    52,    57,
      58,    64,    66,    67,    69,    73,    75,    80,    85,    89,
      93,    97,   101,   105,   109,   113,   117,   121,   125,   129,
     133,   137,   141,   145,   149,   153,   157,   161,   165,   171,
     175,   179,   181,   183,   185,   187,   189,   191,   196,   201,
     203,   207,   209,   211,   213,   216,   218,   221,   223,   225
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      69,     0,    -1,    71,    -1,    70,    -1,    79,    -1,    72,
      -1,    71,    31,    72,    -1,    52,    72,    -1,    39,    72,
      -1,    51,    72,    -1,    50,    72,    -1,    62,    72,    -1,
      38,    72,    -1,    72,    56,    -1,    72,    55,    -1,    72,
      58,    80,    -1,    72,    58,    80,    10,    -1,    72,    58,
      10,    -1,    72,    59,    71,    63,    -1,    -1,    72,    60,
      73,    75,    64,    -1,    65,    -1,    -1,    72,    -1,    75,
      31,    72,    -1,    66,    -1,    74,    79,    76,    72,    -1,
      79,    60,    72,    64,    -1,    60,    71,    64,    -1,    72,
      49,    72,    -1,    72,    52,    72,    -1,    72,    53,    72,
      -1,    72,    54,    72,    -1,    72,    50,    72,    -1,    72,
      51,    72,    -1,    72,    48,    72,    -1,    72,    47,    72,
      -1,    72,    42,    72,    -1,    72,    41,    72,    -1,    72,
      46,    72,    -1,    72,    45,    72,    -1,    72,    43,    72,
      -1,    72,    44,    72,    -1,    72,    39,    72,    -1,    72,
      38,    72,    -1,    72,    37,    72,    -1,    72,    36,    72,
      -1,    72,    35,    72,    -1,    72,    34,    72,    67,    72,
      -1,    72,    33,    72,    -1,    72,    30,    72,    -1,     3,
      -1,     7,    -1,    11,    -1,     4,    -1,    78,    -1,    29,
      -1,    18,    60,    79,    64,    -1,    18,    60,    72,    64,
      -1,     6,    -1,    77,    50,     6,    -1,    77,    -1,    12,
      -1,    13,    -1,    80,    26,    -1,    80,    -1,    52,    79,
      -1,     9,    -1,    28,    -1,     8,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   237,   237,   238,   241,   248,   249,   254,   258,   262,
     266,   270,   274,   278,   282,   288,   294,   301,   311,   318,
     315,   325,   329,   332,   336,   340,   344,   350,   356,   362,
     366,   370,   374,   378,   382,   386,   390,   394,   398,   402,
     406,   410,   414,   418,   422,   426,   430,   434,   438,   442,
     446,   452,   459,   468,   481,   488,   491,   497,   509,   516,
     533,   551,   562,   568,   574,   590,   645,   647,   654,   667
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "FLOAT", "RAW_STRING", "STRING",
  "CHAR", "NAME", "TYPENAME", "COMPLETE", "NAME_OR_INT", "TRUE_KEYWORD",
  "FALSE_KEYWORD", "STRUCT_KEYWORD", "INTERFACE_KEYWORD", "TYPE_KEYWORD",
  "CHAN_KEYWORD", "SIZEOF_KEYWORD", "LEN_KEYWORD", "CAP_KEYWORD",
  "NEW_KEYWORD", "IOTA_KEYWORD", "NIL_KEYWORD", "CONST_KEYWORD",
  "DOTDOTDOT", "ENTRY", "ERROR", "BYTE_KEYWORD", "DOLLAR_VARIABLE",
  "ASSIGN_MODIFY", "','", "ABOVE_COMMA", "'='", "'?'", "OROR", "ANDAND",
  "'|'", "'^'", "'&'", "ANDNOT", "NOTEQUAL", "EQUAL", "'<'", "'>'", "GEQ",
  "LEQ", "RSH", "LSH", "'@'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "DECREMENT", "INCREMENT", "UNARY", "'.'", "'['", "'('", "LEFT_ARROW",
  "'!'", "']'", "')'", "'{'", "'}'", "':'", "$accept", "start", "type_exp",
  "exp1", "exp", "$@1", "lcurly", "arglist", "rcurly", "string_exp",
  "variable", "type", "name_not_typename", 0
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
     285,    44,   286,    61,    63,   287,   288,   124,    94,    38,
     289,   290,   291,    60,    62,   292,   293,   294,   295,    64,
      43,    45,    42,    47,    37,   296,   297,   298,    46,    91,
      40,   299,    33,    93,    41,   123,   125,    58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    68,    69,    69,    70,    71,    71,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    73,
      72,    74,    75,    75,    75,    76,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    77,
      77,    72,    72,    72,    78,    78,    79,    79,    79,    80
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     3,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     4,     3,     4,     0,
       5,     1,     0,     1,     3,     1,     4,     4,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     5,     3,
       3,     1,     1,     1,     1,     1,     1,     4,     4,     1,
       3,     1,     1,     1,     2,     1,     2,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    51,    54,    59,    52,    69,    67,    53,    62,    63,
       0,    68,    56,     0,     0,     0,     0,     0,     0,     0,
      21,     0,     3,     2,     5,     0,    61,    55,     4,    65,
       0,    12,     0,     8,    10,     9,     7,    66,     0,    11,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    14,    13,     0,     0,    19,     0,
       0,     0,     0,    64,     0,     0,    28,     6,    50,    49,
       0,    47,    46,    45,    44,    43,    38,    37,    41,    42,
      40,    39,    36,    35,    29,    33,    34,    30,    31,    32,
      17,    15,     0,    22,    66,    25,     0,    60,     0,    58,
      57,     0,    16,    18,    23,     0,    26,    27,    48,     0,
      20,    24
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    21,    22,    23,    24,   103,    25,   115,   106,    26,
      27,    32,    29
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -56
static const yytype_int16 yypact[] =
{
      49,   -56,   -56,   -56,   -56,   -56,   -56,   -56,   -56,   -56,
     -44,   -56,   -56,    49,    49,    49,    49,    49,    49,    49,
     -56,    23,   -56,    34,   187,    45,   -23,   -56,     6,    46,
      49,   -48,     6,   -48,   -48,   -48,   -48,     6,    38,   -48,
     -56,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,   -56,   -56,     5,    49,   -56,    45,
       8,    65,    49,   -56,   123,   -55,   -56,   187,   187,   187,
      88,   239,   263,   286,   308,   328,   346,   346,   360,   360,
     360,   360,   372,   372,   383,   -34,   -34,   -48,   -48,   -48,
     -56,    69,    33,    49,   -56,   -56,    49,   -56,   155,   -56,
     -56,    49,   -56,   -56,   187,    39,   -48,   -56,   214,    49,
     -56,   187
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -56,   -56,   -56,    -4,   -13,   -56,   -56,   -56,   -56,   -56,
     -56,    51,     9
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      31,    33,    34,    35,    36,    72,    39,    64,    65,   110,
      66,    67,    68,     5,    38,   100,    30,    74,    61,    62,
      63,    64,    65,    40,    66,    67,    68,    71,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,    28,     1,     2,     6,     3,     4,     5,     6,   108,
       7,     8,     9,   102,    41,    41,    72,    10,    37,    41,
     119,   107,    73,    11,   105,   101,    70,    11,    12,   112,
       0,    75,     0,     0,     0,     0,     0,    13,    14,     0,
     114,     0,     0,   116,     0,     0,   113,    69,   118,    15,
      16,    17,    76,   120,     0,     0,   121,     0,     0,    18,
       0,    19,     0,     0,    20,     0,     0,     0,    42,     0,
     104,    43,    44,    45,    46,    47,    48,    49,     0,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,    66,    67,    68,     0,
       0,     0,     0,    42,     0,   111,    43,    44,    45,    46,
      47,    48,    49,     0,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,    66,    67,    68,     0,    42,     0,   109,    43,    44,
      45,    46,    47,    48,    49,     0,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,     0,    66,    67,    68,     0,    42,     0,   117,
      43,    44,    45,    46,    47,    48,    49,     0,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,     0,    66,    67,    68,    44,    45,
      46,    47,    48,    49,     0,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,     0,    66,    67,    68,    46,    47,    48,    49,     0,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,     0,    66,    67,    68,
      47,    48,    49,     0,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,    66,    67,    68,    48,    49,     0,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,     0,    66,    67,    68,    49,     0,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,    66,    67,    68,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,    66,    67,    68,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,     0,    66,    67,    68,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,     0,    66,    67,
      68,    58,    59,    60,    61,    62,    63,    64,    65,     0,
      66,    67,    68,    59,    60,    61,    62,    63,    64,    65,
       0,    66,    67,    68
};

static const yytype_int8 yycheck[] =
{
      13,    14,    15,    16,    17,    60,    19,    55,    56,    64,
      58,    59,    60,     8,    18,    10,    60,    30,    52,    53,
      54,    55,    56,     0,    58,    59,    60,    50,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,     0,     3,     4,     9,     6,     7,     8,     9,    72,
      11,    12,    13,    67,    31,    31,    60,    18,    17,    31,
      31,     6,    26,    28,    66,    66,    25,    28,    29,    10,
      -1,    30,    -1,    -1,    -1,    -1,    -1,    38,    39,    -1,
     103,    -1,    -1,   106,    -1,    -1,    63,    52,   111,    50,
      51,    52,    64,    64,    -1,    -1,   119,    -1,    -1,    60,
      -1,    62,    -1,    -1,    65,    -1,    -1,    -1,    30,    -1,
      69,    33,    34,    35,    36,    37,    38,    39,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    -1,    58,    59,    60,    -1,
      -1,    -1,    -1,    30,    -1,    67,    33,    34,    35,    36,
      37,    38,    39,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      -1,    58,    59,    60,    -1,    30,    -1,    64,    33,    34,
      35,    36,    37,    38,    39,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    -1,    58,    59,    60,    -1,    30,    -1,    64,
      33,    34,    35,    36,    37,    38,    39,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    -1,    58,    59,    60,    34,    35,
      36,    37,    38,    39,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    -1,    58,    59,    60,    36,    37,    38,    39,    -1,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    -1,    58,    59,    60,
      37,    38,    39,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      -1,    58,    59,    60,    38,    39,    -1,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    -1,    58,    59,    60,    39,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    -1,    58,    59,    60,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    -1,    58,    59,    60,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    -1,    58,    59,    60,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    -1,    58,    59,
      60,    49,    50,    51,    52,    53,    54,    55,    56,    -1,
      58,    59,    60,    50,    51,    52,    53,    54,    55,    56,
      -1,    58,    59,    60
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     6,     7,     8,     9,    11,    12,    13,
      18,    28,    29,    38,    39,    50,    51,    52,    60,    62,
      65,    69,    70,    71,    72,    74,    77,    78,    79,    80,
      60,    72,    79,    72,    72,    72,    72,    79,    71,    72,
       0,    31,    30,    33,    34,    35,    36,    37,    38,    39,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    58,    59,    60,    52,
      79,    50,    60,    26,    72,    79,    64,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      10,    80,    71,    73,    79,    66,    76,     6,    72,    64,
      64,    67,    10,    63,    72,    75,    72,    64,    72,    31,
      64,    72
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
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
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
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
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


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

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
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to xreallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
	/* Give user a chance to xreallocate the stack.  Use copies of
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
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
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

/* Line 1455 of yacc.c  */
#line 242 "go-exp.y"
    { write_exp_elt_opcode(OP_TYPE);
			  write_exp_elt_type((yyvsp[(1) - (1)].tval));
			  write_exp_elt_opcode(OP_TYPE); }
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 250 "go-exp.y"
    { write_exp_elt_opcode (BINOP_COMMA); }
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 255 "go-exp.y"
    { write_exp_elt_opcode (UNOP_IND); }
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 259 "go-exp.y"
    { write_exp_elt_opcode (UNOP_ADDR); }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 263 "go-exp.y"
    { write_exp_elt_opcode (UNOP_NEG); }
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 267 "go-exp.y"
    { write_exp_elt_opcode (UNOP_PLUS); }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 271 "go-exp.y"
    { write_exp_elt_opcode (UNOP_LOGICAL_NOT); }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 275 "go-exp.y"
    { write_exp_elt_opcode (UNOP_COMPLEMENT); }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 279 "go-exp.y"
    { write_exp_elt_opcode (UNOP_POSTINCREMENT); }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 283 "go-exp.y"
    { write_exp_elt_opcode (UNOP_POSTDECREMENT); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 289 "go-exp.y"
    { write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string ((yyvsp[(3) - (3)].ssym).stoken);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 295 "go-exp.y"
    { mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string ((yyvsp[(3) - (4)].ssym).stoken);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 302 "go-exp.y"
    { struct stoken s;
			  mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_STRUCT);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (s);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 312 "go-exp.y"
    { write_exp_elt_opcode (BINOP_SUBSCRIPT); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 318 "go-exp.y"
    { start_arglist (); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 320 "go-exp.y"
    { write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 326 "go-exp.y"
    { start_arglist (); }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 333 "go-exp.y"
    { arglist_len = 1; }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 337 "go-exp.y"
    { arglist_len++; }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 341 "go-exp.y"
    { (yyval.lval) = end_arglist () - 1; }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 345 "go-exp.y"
    { write_exp_elt_opcode (UNOP_MEMVAL);
			  write_exp_elt_type ((yyvsp[(2) - (4)].tval));
			  write_exp_elt_opcode (UNOP_MEMVAL); }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 351 "go-exp.y"
    { write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type ((yyvsp[(1) - (4)].tval));
			  write_exp_elt_opcode (UNOP_CAST); }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 357 "go-exp.y"
    { }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 363 "go-exp.y"
    { write_exp_elt_opcode (BINOP_REPEAT); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 367 "go-exp.y"
    { write_exp_elt_opcode (BINOP_MUL); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 371 "go-exp.y"
    { write_exp_elt_opcode (BINOP_DIV); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 375 "go-exp.y"
    { write_exp_elt_opcode (BINOP_REM); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 379 "go-exp.y"
    { write_exp_elt_opcode (BINOP_ADD); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 383 "go-exp.y"
    { write_exp_elt_opcode (BINOP_SUB); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 387 "go-exp.y"
    { write_exp_elt_opcode (BINOP_LSH); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 391 "go-exp.y"
    { write_exp_elt_opcode (BINOP_RSH); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 395 "go-exp.y"
    { write_exp_elt_opcode (BINOP_EQUAL); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 399 "go-exp.y"
    { write_exp_elt_opcode (BINOP_NOTEQUAL); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 403 "go-exp.y"
    { write_exp_elt_opcode (BINOP_LEQ); }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 407 "go-exp.y"
    { write_exp_elt_opcode (BINOP_GEQ); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 411 "go-exp.y"
    { write_exp_elt_opcode (BINOP_LESS); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 415 "go-exp.y"
    { write_exp_elt_opcode (BINOP_GTR); }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 419 "go-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_AND); }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 423 "go-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_XOR); }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 427 "go-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_IOR); }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 431 "go-exp.y"
    { write_exp_elt_opcode (BINOP_LOGICAL_AND); }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 435 "go-exp.y"
    { write_exp_elt_opcode (BINOP_LOGICAL_OR); }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 439 "go-exp.y"
    { write_exp_elt_opcode (TERNOP_COND); }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 443 "go-exp.y"
    { write_exp_elt_opcode (BINOP_ASSIGN); }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 447 "go-exp.y"
    { write_exp_elt_opcode (BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode ((yyvsp[(2) - (3)].opcode));
			  write_exp_elt_opcode (BINOP_ASSIGN_MODIFY); }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 453 "go-exp.y"
    { write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type ((yyvsp[(1) - (1)].typed_val_int).type);
			  write_exp_elt_longcst ((LONGEST)((yyvsp[(1) - (1)].typed_val_int).val));
			  write_exp_elt_opcode (OP_LONG); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 460 "go-exp.y"
    {
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &(yyvsp[(1) - (1)].tsval);
			  write_exp_string_vector ((yyvsp[(1) - (1)].tsval).type, &vec);
			}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 469 "go-exp.y"
    { YYSTYPE val;
			  parse_number ((yyvsp[(1) - (1)].ssym).stoken.ptr, (yyvsp[(1) - (1)].ssym).stoken.length,
					0, &val);
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (val.typed_val_int.type);
			  write_exp_elt_longcst ((LONGEST)
						 val.typed_val_int.val);
			  write_exp_elt_opcode (OP_LONG);
			}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 482 "go-exp.y"
    { write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type ((yyvsp[(1) - (1)].typed_val_float).type);
			  write_exp_elt_dblcst ((yyvsp[(1) - (1)].typed_val_float).dval);
			  write_exp_elt_opcode (OP_DOUBLE); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 492 "go-exp.y"
    {
			  write_dollar_variable ((yyvsp[(1) - (1)].sval));
			}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 498 "go-exp.y"
    {
			  /* TODO(dje): Go objects in structs.  */
			  write_exp_elt_opcode (OP_LONG);
			  /* TODO(dje): What's the right type here?  */
			  write_exp_elt_type (parse_type->builtin_unsigned_int);
			  CHECK_TYPEDEF ((yyvsp[(3) - (4)].tval));
			  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH ((yyvsp[(3) - (4)].tval)));
			  write_exp_elt_opcode (OP_LONG);
			}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 510 "go-exp.y"
    {
			  /* TODO(dje): Go objects in structs.  */
			  write_exp_elt_opcode (UNOP_SIZEOF);
			}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 517 "go-exp.y"
    {
			  /* We copy the string here, and not in the
			     lexer, to guarantee that we do not leak a
			     string.  */
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  struct typed_stoken *vec = XNEW (struct typed_stoken);
			  (yyval.svec).len = 1;
			  (yyval.svec).tokens = vec;

			  vec->type = (yyvsp[(1) - (1)].tsval).type;
			  vec->length = (yyvsp[(1) - (1)].tsval).length;
			  vec->ptr = xmalloc ((yyvsp[(1) - (1)].tsval).length + 1);
			  memcpy (vec->ptr, (yyvsp[(1) - (1)].tsval).ptr, (yyvsp[(1) - (1)].tsval).length + 1);
			}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 534 "go-exp.y"
    {
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  char *p;
			  ++(yyval.svec).len;
			  (yyval.svec).tokens = xrealloc ((yyval.svec).tokens,
					       (yyval.svec).len * sizeof (struct typed_stoken));

			  p = xmalloc ((yyvsp[(3) - (3)].tsval).length + 1);
			  memcpy (p, (yyvsp[(3) - (3)].tsval).ptr, (yyvsp[(3) - (3)].tsval).length + 1);

			  (yyval.svec).tokens[(yyval.svec).len - 1].type = (yyvsp[(3) - (3)].tsval).type;
			  (yyval.svec).tokens[(yyval.svec).len - 1].length = (yyvsp[(3) - (3)].tsval).length;
			  (yyval.svec).tokens[(yyval.svec).len - 1].ptr = p;
			}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 552 "go-exp.y"
    {
			  int i;

			  write_exp_string_vector (0 /*always utf8*/, &(yyvsp[(1) - (1)].svec));
			  for (i = 0; i < (yyvsp[(1) - (1)].svec).len; ++i)
			    xfree ((yyvsp[(1) - (1)].svec).tokens[i].ptr);
			  xfree ((yyvsp[(1) - (1)].svec).tokens);
			}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 563 "go-exp.y"
    { write_exp_elt_opcode (OP_BOOL);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[(1) - (1)].lval));
			  write_exp_elt_opcode (OP_BOOL); }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 569 "go-exp.y"
    { write_exp_elt_opcode (OP_BOOL);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[(1) - (1)].lval));
			  write_exp_elt_opcode (OP_BOOL); }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 575 "go-exp.y"
    { struct symbol *sym = (yyvsp[(1) - (2)].ssym).sym;

			  if (sym == NULL
			      || !SYMBOL_IS_ARGUMENT (sym)
			      || !symbol_read_needs_frame (sym))
			    error (_("@entry can be used only for function "
				     "parameters, not for \"%s\""),
				   copy_name ((yyvsp[(1) - (2)].ssym).stoken));

			  write_exp_elt_opcode (OP_VAR_ENTRY_VALUE);
			  write_exp_elt_sym (sym);
			  write_exp_elt_opcode (OP_VAR_ENTRY_VALUE);
			}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 591 "go-exp.y"
    { struct symbol *sym = (yyvsp[(1) - (1)].ssym).sym;

			  if (sym)
			    {
			      if (symbol_read_needs_frame (sym))
				{
				  if (innermost_block == 0
				      || contained_in (block_found,
						       innermost_block))
				    innermost_block = block_found;
				}

			      write_exp_elt_opcode (OP_VAR_VALUE);
			      /* We want to use the selected frame, not
				 another more inner frame which happens to
				 be in the same block.  */
			      write_exp_elt_block (NULL);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			    }
			  else if ((yyvsp[(1) - (1)].ssym).is_a_field_of_this)
			    {
			      /* TODO(dje): Can we get here?
				 E.g., via a mix of c++ and go?  */
			      gdb_assert_not_reached ("go with `this' field");
			    }
			  else
			    {
			      struct minimal_symbol *msymbol;
			      char *arg = copy_name ((yyvsp[(1) - (1)].ssym).stoken);

			      msymbol =
				lookup_minimal_symbol (arg, NULL, NULL);
			      if (msymbol != NULL)
				write_exp_msymbol (msymbol);
			      else if (!have_full_symbols ()
				       && !have_partial_symbols ())
				error (_("No symbol table is loaded.  "
				       "Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       copy_name ((yyvsp[(1) - (1)].ssym).stoken));
			    }
			}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 646 "go-exp.y"
    { (yyval.tval) = lookup_pointer_type ((yyvsp[(2) - (2)].tval)); }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 648 "go-exp.y"
    { (yyval.tval) = (yyvsp[(1) - (1)].tsym).type; }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 655 "go-exp.y"
    { (yyval.tval) = builtin_go_type (parse_gdbarch)
			    ->builtin_uint8; }
    break;



/* Line 1455 of yacc.c  */
#line 2274 "go-exp.c"
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
      /* If just tried and failed to reuse lookahead token after an
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

  /* Else will try to reuse lookahead token after shifting the error
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
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



/* Line 1675 of yacc.c  */
#line 677 "go-exp.y"


/* Wrapper on parse_c_float to get the type right for Go.  */

static int
parse_go_float (struct gdbarch *gdbarch, const char *p, int len,
		DOUBLEST *d, struct type **t)
{
  int result = parse_c_float (gdbarch, p, len, d, t);
  const struct builtin_type *builtin_types = builtin_type (gdbarch);
  const struct builtin_go_type *builtin_go_types = builtin_go_type (gdbarch);

  if (*t == builtin_types->builtin_float)
    *t = builtin_go_types->builtin_float32;
  else if (*t == builtin_types->builtin_double)
    *t = builtin_go_types->builtin_float64;

  return result;
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/* FIXME: Needs some error checking for the float case.  */
/* FIXME(dje): IWBN to use c-exp.y's parse_number if we could.
   That will require moving the guts into a function that we both call
   as our YYSTYPE is different than c-exp.y's  */

static int
parse_number (char *p, int len, int parsed_float, YYSTYPE *putithere)
{
  /* FIXME: Shouldn't these be unsigned?  We don't deal with negative values
     here, and we do kind of silly things like cast to unsigned.  */
  LONGEST n = 0;
  LONGEST prevn = 0;
  ULONGEST un;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      if (! parse_go_float (parse_gdbarch, p, len,
			    &putithere->typed_val_float.dval,
			    &putithere->typed_val_float.type))
	return ERROR;
      return FLOAT;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0.  */
  if (p[0] == '0')
    switch (p[1])
      {
      case 'x':
      case 'X':
	if (len >= 3)
	  {
	    p += 2;
	    base = 16;
	    len -= 2;
	  }
	break;

      case 'b':
      case 'B':
	if (len >= 3)
	  {
	    p += 2;
	    base = 2;
	    len -= 2;
	  }
	break;

      case 't':
      case 'T':
      case 'd':
      case 'D':
	if (len >= 3)
	  {
	    p += 2;
	    base = 10;
	    len -= 2;
	  }
	break;

      default:
	base = 8;
	break;
      }

  while (len-- > 0)
    {
      c = *p++;
      if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
      if (c != 'l' && c != 'u')
	n *= base;
      if (c >= '0' && c <= '9')
	{
	  if (found_suffix)
	    return ERROR;
	  n += i = c - '0';
	}
      else
	{
	  if (base > 10 && c >= 'a' && c <= 'f')
	    {
	      if (found_suffix)
		return ERROR;
	      n += i = c - 'a' + 10;
	    }
	  else if (c == 'l')
	    {
	      ++long_p;
	      found_suffix = 1;
	    }
	  else if (c == 'u')
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base.  */

      /* Portably test for overflow (only works for nonzero values, so make
	 a second check for zero).  FIXME: Can't we just make n and prevn
	 unsigned and avoid this?  */
      if (c != 'l' && c != 'u' && (prevn >= n) && n != 0)
	unsigned_p = 1;		/* Try something unsigned.  */

      /* Portably test for unsigned overflow.
	 FIXME: This check is wrong; for example it doesn't find overflow
	 on 0x123456789 when LONGEST is 32 bits.  */
      if (c != 'l' && c != 'u' && n != 0)
	{
	  if ((unsigned_p && (ULONGEST) prevn >= (ULONGEST) n))
	    error (_("Numeric constant too large."));
	}
      prevn = n;
    }

  /* An integer constant is an int, a long, or a long long.  An L
     suffix forces it to be long; an LL suffix forces it to be long
     long.  If not forced to a larger size, it gets the first type of
     the above that it fits in.  To figure out whether it fits, we
     shift it right and see whether anything remains.  Note that we
     can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or more in one
     operation, because many compilers will warn about such a shift
     (which always produces a zero result).  Sometimes gdbarch_int_bit
     or gdbarch_long_bit will be that big, sometimes not.  To deal with
     the case where it is we just always shift the value more than
     once, with fewer bits each time.  */

  un = (ULONGEST)n >> 2;
  if (long_p == 0
      && (un >> (gdbarch_int_bit (parse_gdbarch) - 2)) == 0)
    {
      high_bit = ((ULONGEST)1) << (gdbarch_int_bit (parse_gdbarch) - 1);

      /* A large decimal (not hex or octal) constant (between INT_MAX
	 and UINT_MAX) is a long or unsigned long, according to ANSI,
	 never an unsigned int, but this code treats it as unsigned
	 int.  This probably should be fixed.  GCC gives a warning on
	 such constants.  */

      unsigned_type = parse_type->builtin_unsigned_int;
      signed_type = parse_type->builtin_int;
    }
  else if (long_p <= 1
	   && (un >> (gdbarch_long_bit (parse_gdbarch) - 2)) == 0)
    {
      high_bit = ((ULONGEST)1) << (gdbarch_long_bit (parse_gdbarch) - 1);
      unsigned_type = parse_type->builtin_unsigned_long;
      signed_type = parse_type->builtin_long;
    }
  else
    {
      int shift;
      if (sizeof (ULONGEST) * HOST_CHAR_BIT
	  < gdbarch_long_long_bit (parse_gdbarch))
	/* A long long does not fit in a LONGEST.  */
	shift = (sizeof (ULONGEST) * HOST_CHAR_BIT - 1);
      else
	shift = (gdbarch_long_long_bit (parse_gdbarch) - 1);
      high_bit = (ULONGEST) 1 << shift;
      unsigned_type = parse_type->builtin_unsigned_long_long;
      signed_type = parse_type->builtin_long_long;
    }

   putithere->typed_val_int.val = n;

   /* If the high bit of the worked out type is set then this number
      has to be unsigned.  */

   if (unsigned_p || (n & high_bit))
     {
       putithere->typed_val_int.type = unsigned_type;
     }
   else
     {
       putithere->typed_val_int.type = signed_type;
     }

   return INT;
}

/* Temporary obstack used for holding strings.  */
static struct obstack tempbuf;
static int tempbuf_init;

/* Parse a string or character literal from TOKPTR.  The string or
   character may be wide or unicode.  *OUTPTR is set to just after the
   end of the literal in the input string.  The resulting token is
   stored in VALUE.  This returns a token value, either STRING or
   CHAR, depending on what was parsed.  *HOST_CHARS is set to the
   number of host characters in the literal.  */

static int
parse_string_or_char (char *tokptr, char **outptr, struct typed_stoken *value,
		      int *host_chars)
{
  int quote;

  /* Build the gdb internal form of the input string in tempbuf.  Note
     that the buffer is null byte terminated *only* for the
     convenience of debugging gdb itself and printing the buffer
     contents when the buffer contains no embedded nulls.  Gdb does
     not depend upon the buffer being null byte terminated, it uses
     the length string instead.  This allows gdb to handle C strings
     (as well as strings in other languages) with embedded null
     bytes */

  if (!tempbuf_init)
    tempbuf_init = 1;
  else
    obstack_free (&tempbuf, NULL);
  obstack_init (&tempbuf);

  /* Skip the quote.  */
  quote = *tokptr;
  ++tokptr;

  *host_chars = 0;

  while (*tokptr)
    {
      char c = *tokptr;
      if (c == '\\')
	{
	  ++tokptr;
	  *host_chars += c_parse_escape (&tokptr, &tempbuf);
	}
      else if (c == quote)
	break;
      else
	{
	  obstack_1grow (&tempbuf, c);
	  ++tokptr;
	  /* FIXME: this does the wrong thing with multi-byte host
	     characters.  We could use mbrlen here, but that would
	     make "set host-charset" a bit less useful.  */
	  ++*host_chars;
	}
    }

  if (*tokptr != quote)
    {
      if (quote == '"')
	error (_("Unterminated string in expression."));
      else
	error (_("Unmatched single quote."));
    }
  ++tokptr;

  value->type = C_STRING | (quote == '\'' ? C_CHAR : 0); /*FIXME*/
  value->ptr = obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '\'' ? CHAR : STRING;
}

struct token
{
  char *operator;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH},
    /*{"&^=", ASSIGN_MODIFY, BINOP_BITWISE_ANDNOT}, TODO */
    {"...", DOTDOTDOT, OP_NULL},
  };

static const struct token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD},
    {"-=", ASSIGN_MODIFY, BINOP_SUB},
    {"*=", ASSIGN_MODIFY, BINOP_MUL},
    {"/=", ASSIGN_MODIFY, BINOP_DIV},
    {"%=", ASSIGN_MODIFY, BINOP_REM},
    {"|=", ASSIGN_MODIFY, BINOP_BITWISE_IOR},
    {"&=", ASSIGN_MODIFY, BINOP_BITWISE_AND},
    {"^=", ASSIGN_MODIFY, BINOP_BITWISE_XOR},
    {"++", INCREMENT, BINOP_END},
    {"--", DECREMENT, BINOP_END},
    /*{"->", RIGHT_ARROW, BINOP_END}, Doesn't exist in Go.  */
    {"<-", LEFT_ARROW, BINOP_END},
    {"&&", ANDAND, BINOP_END},
    {"||", OROR, BINOP_END},
    {"<<", LSH, BINOP_END},
    {">>", RSH, BINOP_END},
    {"==", EQUAL, BINOP_END},
    {"!=", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END},
    /*{"&^", ANDNOT, BINOP_END}, TODO */
  };

/* Identifier-like tokens.  */
static const struct token ident_tokens[] =
  {
    {"true", TRUE_KEYWORD, OP_NULL},
    {"false", FALSE_KEYWORD, OP_NULL},
    {"nil", NIL_KEYWORD, OP_NULL},
    {"const", CONST_KEYWORD, OP_NULL},
    {"struct", STRUCT_KEYWORD, OP_NULL},
    {"type", TYPE_KEYWORD, OP_NULL},
    {"interface", INTERFACE_KEYWORD, OP_NULL},
    {"chan", CHAN_KEYWORD, OP_NULL},
    {"byte", BYTE_KEYWORD, OP_NULL}, /* An alias of uint8.  */
    {"len", LEN_KEYWORD, OP_NULL},
    {"cap", CAP_KEYWORD, OP_NULL},
    {"new", NEW_KEYWORD, OP_NULL},
    {"iota", IOTA_KEYWORD, OP_NULL},
  };

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure
   operator -- either '.' or ARROW.  This is used only when parsing to
   do field name completion.  */
static int last_was_structop;

/* Read one token, getting characters through lexptr.  */

static int
lex_one_token (void)
{
  int c;
  int namelen;
  unsigned int i;
  char *tokstart;
  int saw_structop = last_was_structop;
  char *copy;

  last_was_structop = 0;

 retry:

  prev_lexptr = lexptr;

  tokstart = lexptr;
  /* See if it is a special token of length 3.  */
  for (i = 0; i < sizeof (tokentab3) / sizeof (tokentab3[0]); i++)
    if (strncmp (tokstart, tokentab3[i].operator, 3) == 0)
      {
	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof (tokentab2) / sizeof (tokentab2[0]); i++)
    if (strncmp (tokstart, tokentab2[i].operator, 2) == 0)
      {
	lexptr += 2;
	yylval.opcode = tokentab2[i].opcode;
	/* NOTE: -> doesn't exist in Go, so we don't need to watch for
	   setting last_was_structop here.  */
	return tokentab2[i].token;
      }

  switch (c = *tokstart)
    {
    case 0:
      if (saw_name_at_eof)
	{
	  saw_name_at_eof = 0;
	  return COMPLETE;
	}
      else if (saw_structop)
	return COMPLETE;
      else
        return 0;

    case ' ':
    case '\t':
    case '\n':
      lexptr++;
      goto retry;

    case '[':
    case '(':
      paren_depth++;
      lexptr++;
      return c;

    case ']':
    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      lexptr++;
      return c;

    case ',':
      if (comma_terminates
          && paren_depth == 0)
	return 0;
      lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (lexptr[1] < '0' || lexptr[1] > '9')
	{
	  if (parse_completion)
	    last_was_structop = 1;
	  goto symbol;		/* Nope, must be a symbol. */
	}
      /* FALL THRU into number case.  */

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
	/* It's a number.  */
	int got_dot = 0, got_e = 0, toktype;
	char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }

	for (;; ++p)
	  {
	    /* This test includes !hex because 'e' is a valid hex digit
	       and thus does not indicate a floating point number when
	       the radix is hex.  */
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		     && (*p == '-' || *p == '+'))
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if ((*p < '0' || *p > '9')
		     && ((*p < 'a' || *p > 'z')
				  && (*p < 'A' || *p > 'Z')))
	      break;
	  }
	toktype = parse_number (tokstart, p - tokstart, got_dot|got_e, &yylval);
        if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error (_("Invalid number \"%s\"."), err_copy);
	  }
	lexptr = p;
	return toktype;
      }

    case '@':
      {
	char *p = &tokstart[1];
	size_t len = strlen ("entry");

	while (isspace (*p))
	  p++;
	if (strncmp (p, "entry", len) == 0 && !isalnum (p[len])
	    && p[len] != '_')
	  {
	    lexptr = &p[len];
	    return ENTRY;
	  }
      }
      /* FALLTHRU */
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '<':
    case '>':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      lexptr++;
      return c;

    case '\'':
    case '"':
    case '`':
      {
	int host_len;
	int result = parse_string_or_char (tokstart, &lexptr, &yylval.tsval,
					   &host_len);
	if (result == CHAR)
	  {
	    if (host_len == 0)
	      error (_("Empty character constant."));
	    else if (host_len > 2 && c == '\'')
	      {
		++tokstart;
		namelen = lexptr - tokstart - 1;
		goto tryname;
	      }
	    else if (host_len > 1)
	      error (_("Invalid character constant."));
	  }
	return result;
      }
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));)
    {
      c = tokstart[++namelen];
    }

  /* The token "if" terminates the expression and is NOT removed from
     the input stream.  It doesn't count if it appears in the
     expansion of a macro.  */
  if (namelen == 2
      && tokstart[0] == 'i'
      && tokstart[1] == 'f')
    {
      return 0;
    }

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.
     Handle abbreviations of these, similarly to
     breakpoint.c:find_condition_and_thread.
     TODO: Watch for "goroutine" here?  */
  if (namelen >= 1
      && strncmp (tokstart, "thread", namelen) == 0
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t'))
    {
      char *p = tokstart + namelen + 1;
      while (*p == ' ' || *p == '\t')
	p++;
      if (*p >= '0' && *p <= '9')
	return 0;
    }

  lexptr += namelen;

  tryname:

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  /* Catch specific keywords.  */
  copy = copy_name (yylval.sval);
  for (i = 0; i < sizeof (ident_tokens) / sizeof (ident_tokens[0]); i++)
    if (strcmp (copy, ident_tokens[i].operator) == 0)
      {
	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = ident_tokens[i].opcode;
	return ident_tokens[i].token;
      }

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  if (parse_completion && *lexptr == '\0')
    saw_name_at_eof = 1;
  return NAME;
}

/* An object of this type is pushed on a FIFO by the "outer" lexer.  */
typedef struct
{
  int token;
  YYSTYPE value;
} token_and_value;

DEF_VEC_O (token_and_value);

/* A FIFO of tokens that have been read but not yet returned to the
   parser.  */
static VEC (token_and_value) *token_fifo;

/* Non-zero if the lexer should return tokens from the FIFO.  */
static int popping;

/* Temporary storage for yylex; this holds symbol names as they are
   built up.  */
static struct obstack name_obstack;

/* Build "package.name" in name_obstack.
   For convenience of the caller, the name is NUL-terminated,
   but the NUL is not included in the recorded length.  */

static struct stoken
build_packaged_name (const char *package, int package_len,
		     const char *name, int name_len)
{
  struct stoken result;

  obstack_free (&name_obstack, obstack_base (&name_obstack));
  obstack_grow (&name_obstack, package, package_len);
  obstack_grow_str (&name_obstack, ".");
  obstack_grow (&name_obstack, name, name_len);
  obstack_grow (&name_obstack, "", 1);
  result.ptr = obstack_base (&name_obstack);
  result.length = obstack_object_size (&name_obstack) - 1;

  return result;
}

/* Return non-zero if NAME is a package name.
   BLOCK is the scope in which to interpret NAME; this can be NULL
   to mean the global scope.  */

static int
package_name_p (const char *name, const struct block *block)
{
  struct symbol *sym;
  struct field_of_this_result is_a_field_of_this;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, &is_a_field_of_this);

  if (sym
      && SYMBOL_CLASS (sym) == LOC_TYPEDEF
      && TYPE_CODE (SYMBOL_TYPE (sym)) == TYPE_CODE_MODULE)
    return 1;

  return 0;
}

/* Classify a (potential) function in the "unsafe" package.
   We fold these into "keywords" to keep things simple, at least until
   something more complex is warranted.  */

static int
classify_unsafe_function (struct stoken function_name)
{
  char *copy = copy_name (function_name);

  if (strcmp (copy, "Sizeof") == 0)
    {
      yylval.sval = function_name;
      return SIZEOF_KEYWORD;
    }

  error (_("Unknown function in `unsafe' package: %s"), copy);
}

/* Classify token(s) "name1.name2" where name1 is known to be a package.
   The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.

   The result is one of NAME, NAME_OR_INT, or TYPENAME.  */

static int
classify_packaged_name (const struct block *block)
{
  char *copy;
  struct symbol *sym;
  struct field_of_this_result is_a_field_of_this;

  copy = copy_name (yylval.sval);

  sym = lookup_symbol (copy, block, VAR_DOMAIN, &is_a_field_of_this);

  if (sym)
    {
      yylval.ssym.sym = sym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
    }

  return NAME;
}

/* Classify a NAME token.
   The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.
   BLOCK is the block in which lookups start; this can be NULL
   to mean the global scope.

   The result is one of NAME, NAME_OR_INT, or TYPENAME.  */

static int
classify_name (const struct block *block)
{
  struct type *type;
  struct symbol *sym;
  char *copy;
  struct field_of_this_result is_a_field_of_this;

  copy = copy_name (yylval.sval);

  /* Try primitive types first so they win over bad/weird debug info.  */
  type = language_lookup_primitive_type_by_name (parse_language,
						 parse_gdbarch, copy);
  if (type != NULL)
    {
      /* NOTE: We take advantage of the fact that yylval coming in was a
	 NAME, and that struct ttype is a compatible extension of struct
	 stoken, so yylval.tsym.stoken is already filled in.  */
      yylval.tsym.type = type;
      return TYPENAME;
    }

  /* TODO: What about other types?  */

  sym = lookup_symbol (copy, block, VAR_DOMAIN, &is_a_field_of_this);

  if (sym)
    {
      yylval.ssym.sym = sym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
      return NAME;
    }

  /* If we didn't find a symbol, look again in the current package.
     This is to, e.g., make "p global_var" work without having to specify
     the package name.  We intentionally only looks for objects in the
     current package.  */

  {
    char *current_package_name = go_block_package_name (block);

    if (current_package_name != NULL)
      {
	struct stoken sval =
	  build_packaged_name (current_package_name,
			       strlen (current_package_name),
			       copy, strlen (copy));

	xfree (current_package_name);
	sym = lookup_symbol (sval.ptr, block, VAR_DOMAIN,
			     &is_a_field_of_this);
	if (sym)
	  {
	    yylval.ssym.stoken = sval;
	    yylval.ssym.sym = sym;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	    return NAME;
	  }
      }
  }

  /* Input names that aren't symbols but ARE valid hex numbers, when
     the input radix permits them, can be names or numbers depending
     on the parse.  Note we support radixes > 16 here.  */
  if ((copy[0] >= 'a' && copy[0] < 'a' + input_radix - 10)
      || (copy[0] >= 'A' && copy[0] < 'A' + input_radix - 10))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (copy, yylval.sval.length, 0, &newlval);
      if (hextype == INT)
	{
	  yylval.ssym.sym = NULL;
	  yylval.ssym.is_a_field_of_this = 0;
	  return NAME_OR_INT;
	}
    }

  yylval.ssym.sym = NULL;
  yylval.ssym.is_a_field_of_this = 0;
  return NAME;
}

/* This is taken from c-exp.y mostly to get something working.
   The basic structure has been kept because we may yet need some of it.  */

static int
yylex (void)
{
  token_and_value current, next;

  if (popping && !VEC_empty (token_and_value, token_fifo))
    {
      token_and_value tv = *VEC_index (token_and_value, token_fifo, 0);
      VEC_ordered_remove (token_and_value, token_fifo, 0);
      yylval = tv.value;
      /* There's no need to fall through to handle package.name
	 as that can never happen here.  In theory.  */
      return tv.token;
    }
  popping = 0;

  current.token = lex_one_token ();

  /* TODO: Need a way to force specifying name1 as a package.
     .name1.name2 ?  */

  if (current.token != NAME)
    return current.token;

  /* See if we have "name1 . name2".  */

  current.value = yylval;
  next.token = lex_one_token ();
  next.value = yylval;

  if (next.token == '.')
    {
      token_and_value name2;

      name2.token = lex_one_token ();
      name2.value = yylval;

      if (name2.token == NAME)
	{
	  /* Ok, we have "name1 . name2".  */
	  char *copy;

	  copy = copy_name (current.value.sval);

	  if (strcmp (copy, "unsafe") == 0)
	    {
	      popping = 1;
	      return classify_unsafe_function (name2.value.sval);
	    }

	  if (package_name_p (copy, expression_context_block))
	    {
	      popping = 1;
	      yylval.sval = build_packaged_name (current.value.sval.ptr,
						 current.value.sval.length,
						 name2.value.sval.ptr,
						 name2.value.sval.length);
	      return classify_packaged_name (expression_context_block);
	    }
	}

      VEC_safe_push (token_and_value, token_fifo, &next);
      VEC_safe_push (token_and_value, token_fifo, &name2);
    }
  else
    {
      VEC_safe_push (token_and_value, token_fifo, &next);
    }

  /* If we arrive here we don't have a package-qualified name.  */

  popping = 1;
  yylval = current.value;
  return classify_name (expression_context_block);
}

int
go_parse (void)
{
  int result;
  struct cleanup *back_to = make_cleanup (null_cleanup, NULL);

  make_cleanup_restore_integer (&yydebug);
  yydebug = parser_debug;

  /* Initialize some state used by the lexer.  */
  last_was_structop = 0;
  saw_name_at_eof = 0;

  VEC_free (token_and_value, token_fifo);
  popping = 0;
  obstack_init (&name_obstack);
  make_cleanup_obstack_free (&name_obstack);

  result = yyparse ();
  do_cleanups (back_to);
  return result;
}

void
yyerror (char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), (msg ? msg : "error"), lexptr);
}

