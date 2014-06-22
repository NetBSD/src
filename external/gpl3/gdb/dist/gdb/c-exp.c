/* A Bison parser, made by GNU Bison 2.7.12-4996.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.7.12-4996"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 36 "c-exp.y"


#include "defs.h"
#include <string.h>
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"
#include "cp-support.h"
#include "dfp.h"
#include "gdb_assert.h"
#include "macroscope.h"
#include "objc-lang.h"
#include "typeprint.h"
#include "cp-abi.h"

#define parse_type builtin_type (parse_gdbarch)

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in gdb.  Note that these are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list. */

#define	yymaxdepth c_maxdepth
#define	yyparse	c_parse_internal
#define	yylex	c_lex
#define	yyerror	c_error
#define	yylval	c_lval
#define	yychar	c_char
#define	yydebug	c_debug
#define	yypact	c_pact	
#define	yyr1	c_r1			
#define	yyr2	c_r2			
#define	yydef	c_def		
#define	yychk	c_chk		
#define	yypgo	c_pgo		
#define	yyact	c_act		
#define	yyexca	c_exca
#define yyerrflag c_errflag
#define yynerrs	c_nerrs
#define	yyps	c_ps
#define	yypv	c_pv
#define	yys	c_s
#define	yy_yys	c_yys
#define	yystate	c_state
#define	yytmp	c_tmp
#define	yyv	c_v
#define	yy_yyv	c_yyv
#define	yyval	c_val
#define	yylloc	c_lloc
#define yyreds	c_reds		/* With YYDEBUG defined */
#define yytoks	c_toks		/* With YYDEBUG defined */
#define yyname	c_name		/* With YYDEBUG defined */
#define yyrule	c_rule		/* With YYDEBUG defined */
#define yylhs	c_yylhs
#define yylen	c_yylen
#define yydefred c_yydefred
#define yydgoto	c_yydgoto
#define yysindex c_yysindex
#define yyrindex c_yyrindex
#define yygindex c_yygindex
#define yytable	 c_yytable
#define yycheck	 c_yycheck
#define yyss	c_yyss
#define yysslim	c_yysslim
#define yyssp	c_yyssp
#define yystacksize c_yystacksize
#define yyvs	c_yyvs
#define yyvsp	c_yyvsp

#ifndef YYDEBUG
#define	YYDEBUG 1		/* Default to yydebug support */
#endif

#define YYFPRINTF parser_fprintf

int yyparse (void);

static int yylex (void);

void yyerror (char *);


/* Line 371 of yacc.c  */
#line 161 "c-exp.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT = 258,
     FLOAT = 259,
     DECFLOAT = 260,
     STRING = 261,
     NSSTRING = 262,
     SELECTOR = 263,
     CHAR = 264,
     NAME = 265,
     UNKNOWN_CPP_NAME = 266,
     COMPLETE = 267,
     TYPENAME = 268,
     CLASSNAME = 269,
     OBJC_LBRAC = 270,
     NAME_OR_INT = 271,
     OPERATOR = 272,
     STRUCT = 273,
     CLASS = 274,
     UNION = 275,
     ENUM = 276,
     SIZEOF = 277,
     UNSIGNED = 278,
     COLONCOLON = 279,
     TEMPLATE = 280,
     ERROR = 281,
     NEW = 282,
     DELETE = 283,
     REINTERPRET_CAST = 284,
     DYNAMIC_CAST = 285,
     STATIC_CAST = 286,
     CONST_CAST = 287,
     ENTRY = 288,
     TYPEOF = 289,
     DECLTYPE = 290,
     TYPEID = 291,
     SIGNED_KEYWORD = 292,
     LONG = 293,
     SHORT = 294,
     INT_KEYWORD = 295,
     CONST_KEYWORD = 296,
     VOLATILE_KEYWORD = 297,
     DOUBLE_KEYWORD = 298,
     VARIABLE = 299,
     ASSIGN_MODIFY = 300,
     TRUEKEYWORD = 301,
     FALSEKEYWORD = 302,
     ABOVE_COMMA = 303,
     OROR = 304,
     ANDAND = 305,
     NOTEQUAL = 306,
     EQUAL = 307,
     GEQ = 308,
     LEQ = 309,
     RSH = 310,
     LSH = 311,
     DECREMENT = 312,
     INCREMENT = 313,
     UNARY = 314,
     DOT_STAR = 315,
     ARROW_STAR = 316,
     ARROW = 317,
     BLOCKNAME = 318,
     FILENAME = 319,
     DOTDOTDOT = 320
   };
#endif
/* Tokens.  */
#define INT 258
#define FLOAT 259
#define DECFLOAT 260
#define STRING 261
#define NSSTRING 262
#define SELECTOR 263
#define CHAR 264
#define NAME 265
#define UNKNOWN_CPP_NAME 266
#define COMPLETE 267
#define TYPENAME 268
#define CLASSNAME 269
#define OBJC_LBRAC 270
#define NAME_OR_INT 271
#define OPERATOR 272
#define STRUCT 273
#define CLASS 274
#define UNION 275
#define ENUM 276
#define SIZEOF 277
#define UNSIGNED 278
#define COLONCOLON 279
#define TEMPLATE 280
#define ERROR 281
#define NEW 282
#define DELETE 283
#define REINTERPRET_CAST 284
#define DYNAMIC_CAST 285
#define STATIC_CAST 286
#define CONST_CAST 287
#define ENTRY 288
#define TYPEOF 289
#define DECLTYPE 290
#define TYPEID 291
#define SIGNED_KEYWORD 292
#define LONG 293
#define SHORT 294
#define INT_KEYWORD 295
#define CONST_KEYWORD 296
#define VOLATILE_KEYWORD 297
#define DOUBLE_KEYWORD 298
#define VARIABLE 299
#define ASSIGN_MODIFY 300
#define TRUEKEYWORD 301
#define FALSEKEYWORD 302
#define ABOVE_COMMA 303
#define OROR 304
#define ANDAND 305
#define NOTEQUAL 306
#define EQUAL 307
#define GEQ 308
#define LEQ 309
#define RSH 310
#define LSH 311
#define DECREMENT 312
#define INCREMENT 313
#define UNARY 314
#define DOT_STAR 315
#define ARROW_STAR 316
#define ARROW 317
#define BLOCKNAME 318
#define FILENAME 319
#define DOTDOTDOT 320



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 134 "c-exp.y"

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      DOUBLEST dval;
      struct type *type;
    } typed_val_float;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_decfloat;
    struct type *tval;
    struct stoken sval;
    struct typed_stoken tsval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;

    struct stoken_vector svec;
    VEC (type_ptr) *tvec;

    struct type_stack *type_stack;

    struct objc_class_str class;
  

/* Line 387 of yacc.c  */
#line 363 "c-exp.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

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



/* Copy the second part of user declarations.  */
/* Line 390 of yacc.c  */
#line 165 "c-exp.y"

/* YYSTYPE gets defined by %union */
static int parse_number (const char *, int, int, YYSTYPE *);
static struct stoken operator_stoken (const char *);
static void check_parameter_typelist (VEC (type_ptr) *);
static void write_destructor_name (struct stoken);

static void c_print_token (FILE *file, int type, YYSTYPE value);
#define YYPRINT(FILE, TYPE, VALUE) c_print_token (FILE, TYPE, VALUE)

/* Line 390 of yacc.c  */
#line 402 "c-exp.c"

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
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif


/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined xmalloc) \
	     && (defined YYFREE || defined xfree)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC xmalloc
#   if ! defined xmalloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *xmalloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE xfree
#   if ! defined xfree && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
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

# define YYCOPY_NEEDED 1

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

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  167
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1734

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  90
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  46
/* YYNRULES -- Number of rules.  */
#define YYNRULES  260
/* YYNRULES -- Number of states.  */
#define YYNSTATES  413

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   320

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    84,     2,     2,     2,    70,    56,     2,
      76,    83,    68,    66,    48,    67,    74,    69,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    87,     2,
      59,    50,    60,    51,    65,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    75,     2,    86,    55,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    88,    54,    89,    85,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    49,    52,    53,    57,    58,    61,    62,
      63,    64,    71,    72,    73,    77,    78,    79,    80,    81,
      82
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    14,    19,    24,    26,
      30,    33,    36,    39,    42,    45,    48,    51,    54,    57,
      60,    65,    70,    73,    77,    82,    86,    91,    97,   101,
     105,   109,   114,   118,   123,   129,   133,   137,   142,   147,
     148,   154,   155,   161,   162,   168,   170,   172,   174,   177,
     181,   184,   187,   188,   194,   195,   201,   203,   204,   206,
     210,   216,   218,   222,   227,   232,   236,   240,   244,   248,
     252,   256,   260,   264,   268,   272,   276,   280,   284,   288,
     292,   296,   300,   304,   308,   312,   318,   322,   326,   328,
     330,   332,   334,   336,   338,   340,   345,   350,   358,   366,
     374,   382,   384,   387,   389,   391,   393,   395,   397,   399,
     403,   406,   410,   414,   419,   425,   427,   430,   432,   435,
     437,   438,   442,   444,   446,   448,   449,   450,   455,   456,
     460,   462,   465,   467,   470,   472,   474,   478,   481,   483,
     486,   488,   491,   494,   498,   502,   505,   509,   511,   513,
     515,   517,   519,   522,   526,   529,   533,   537,   541,   544,
     547,   551,   556,   560,   564,   569,   573,   578,   582,   587,
     590,   594,   597,   601,   604,   608,   610,   613,   616,   619,
     623,   626,   629,   633,   636,   639,   643,   646,   649,   653,
     656,   658,   661,   663,   669,   672,   675,   677,   679,   681,
     683,   685,   689,   691,   695,   697,   700,   703,   704,   707,
     710,   713,   715,   717,   719,   722,   725,   730,   735,   740,
     745,   748,   751,   754,   757,   760,   763,   766,   769,   772,
     775,   778,   781,   784,   787,   790,   793,   796,   799,   802,
     805,   808,   811,   814,   817,   820,   823,   826,   830,   834,
     838,   841,   843,   845,   847,   849,   851,   853,   855,   857,
     859
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      91,     0,    -1,    93,    -1,    92,    -1,   123,    -1,    34,
      76,    94,    83,    -1,    34,    76,   123,    83,    -1,    35,
      76,    94,    83,    -1,    94,    -1,    93,    48,    94,    -1,
      68,    94,    -1,    56,    94,    -1,    67,    94,    -1,    66,
      94,    -1,    84,    94,    -1,    85,    94,    -1,    72,    94,
      -1,    71,    94,    -1,    94,    72,    -1,    94,    71,    -1,
      36,    76,    94,    83,    -1,    36,    76,    92,    83,    -1,
      22,    94,    -1,    94,    79,   134,    -1,    94,    79,   134,
      12,    -1,    94,    79,    12,    -1,    94,    79,    85,   134,
      -1,    94,    79,    85,   134,    12,    -1,    94,    79,   109,
      -1,    94,    78,    94,    -1,    94,    74,   134,    -1,    94,
      74,   134,    12,    -1,    94,    74,    12,    -1,    94,    74,
      85,   134,    -1,    94,    74,    85,   134,    12,    -1,    94,
      74,   109,    -1,    94,    77,    94,    -1,    94,    75,    93,
      86,    -1,    94,    15,    93,    86,    -1,    -1,    15,    13,
      95,    98,    86,    -1,    -1,    15,    14,    96,    98,    86,
      -1,    -1,    15,    94,    97,    98,    86,    -1,   134,    -1,
      99,    -1,   100,    -1,    99,   100,    -1,   134,    87,    94,
      -1,    87,    94,    -1,    48,    94,    -1,    -1,    94,    76,
     101,   104,    83,    -1,    -1,    11,    76,   102,   104,    83,
      -1,    88,    -1,    -1,    94,    -1,   104,    48,    94,    -1,
      94,    76,   126,    83,   111,    -1,    89,    -1,   103,   104,
     105,    -1,   103,    92,   105,    94,    -1,    76,    92,    83,
      94,    -1,    76,    93,    83,    -1,    94,    65,    94,    -1,
      94,    68,    94,    -1,    94,    69,    94,    -1,    94,    70,
      94,    -1,    94,    66,    94,    -1,    94,    67,    94,    -1,
      94,    64,    94,    -1,    94,    63,    94,    -1,    94,    58,
      94,    -1,    94,    57,    94,    -1,    94,    62,    94,    -1,
      94,    61,    94,    -1,    94,    59,    94,    -1,    94,    60,
      94,    -1,    94,    56,    94,    -1,    94,    55,    94,    -1,
      94,    54,    94,    -1,    94,    53,    94,    -1,    94,    52,
      94,    -1,    94,    51,    94,    87,    94,    -1,    94,    50,
      94,    -1,    94,    45,    94,    -1,     3,    -1,     9,    -1,
      16,    -1,     4,    -1,     5,    -1,   108,    -1,    44,    -1,
       8,    76,   134,    83,    -1,    22,    76,   123,    83,    -1,
      29,    59,    92,    60,    76,    94,    83,    -1,    31,    59,
      92,    60,    76,    94,    83,    -1,    30,    59,    92,    60,
      76,    94,    83,    -1,    32,    59,    92,    60,    76,    94,
      83,    -1,     6,    -1,   106,     6,    -1,   106,    -1,     7,
      -1,    46,    -1,    47,    -1,    80,    -1,    81,    -1,   107,
      24,   134,    -1,   135,    33,    -1,   107,    24,   134,    -1,
      13,    24,   134,    -1,    13,    24,    85,   134,    -1,    13,
      24,   134,    24,   134,    -1,   109,    -1,    24,   135,    -1,
     135,    -1,    65,    10,    -1,   132,    -1,    -1,   111,   110,
     111,    -1,   112,    -1,   132,    -1,   113,    -1,    -1,    -1,
     115,    68,   116,   114,    -1,    -1,    68,   117,   114,    -1,
      56,    -1,    56,   115,    -1,   115,    -1,   118,   120,    -1,
     118,    -1,   120,    -1,    76,   119,    83,    -1,   120,   121,
      -1,   121,    -1,   120,   122,    -1,   122,    -1,    75,    86,
      -1,    15,    86,    -1,    75,     3,    86,    -1,    15,     3,
      86,    -1,    76,    83,    -1,    76,   126,    83,    -1,   128,
      -1,    13,    -1,    40,    -1,    38,    -1,    39,    -1,    38,
      40,    -1,    38,    37,    40,    -1,    38,    37,    -1,    37,
      38,    40,    -1,    23,    38,    40,    -1,    38,    23,    40,
      -1,    38,    23,    -1,    38,    38,    -1,    38,    38,    40,
      -1,    38,    38,    37,    40,    -1,    38,    38,    37,    -1,
      37,    38,    38,    -1,    37,    38,    38,    40,    -1,    23,
      38,    38,    -1,    23,    38,    38,    40,    -1,    38,    38,
      23,    -1,    38,    38,    23,    40,    -1,    39,    40,    -1,
      39,    37,    40,    -1,    39,    37,    -1,    23,    39,    40,
      -1,    39,    23,    -1,    39,    23,    40,    -1,    43,    -1,
      38,    43,    -1,    18,   134,    -1,    18,    12,    -1,    18,
     134,    12,    -1,    19,   134,    -1,    19,    12,    -1,    19,
     134,    12,    -1,    20,   134,    -1,    20,    12,    -1,    20,
     134,    12,    -1,    21,   134,    -1,    21,    12,    -1,    21,
     134,    12,    -1,    23,   125,    -1,    23,    -1,    37,   125,
      -1,    37,    -1,    25,   134,    59,   123,    60,    -1,   113,
     124,    -1,   124,   113,    -1,    13,    -1,    40,    -1,    38,
      -1,    39,    -1,   127,    -1,   127,    48,    82,    -1,   123,
      -1,   127,    48,   123,    -1,   124,    -1,   128,   119,    -1,
     124,   130,    -1,    -1,   115,   130,    -1,    41,    42,    -1,
      42,    41,    -1,   131,    -1,    41,    -1,    42,    -1,    17,
      27,    -1,    17,    28,    -1,    17,    27,    75,    86,    -1,
      17,    28,    75,    86,    -1,    17,    27,    15,    86,    -1,
      17,    28,    15,    86,    -1,    17,    66,    -1,    17,    67,
      -1,    17,    68,    -1,    17,    69,    -1,    17,    70,    -1,
      17,    55,    -1,    17,    56,    -1,    17,    54,    -1,    17,
      85,    -1,    17,    84,    -1,    17,    50,    -1,    17,    59,
      -1,    17,    60,    -1,    17,    45,    -1,    17,    64,    -1,
      17,    63,    -1,    17,    58,    -1,    17,    57,    -1,    17,
      62,    -1,    17,    61,    -1,    17,    53,    -1,    17,    52,
      -1,    17,    72,    -1,    17,    71,    -1,    17,    48,    -1,
      17,    78,    -1,    17,    79,    -1,    17,    76,    83,    -1,
      17,    75,    86,    -1,    17,    15,    86,    -1,    17,   129,
      -1,    10,    -1,    80,    -1,    13,    -1,    16,    -1,    11,
      -1,   133,    -1,    10,    -1,    80,    -1,   133,    -1,    11,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   279,   279,   280,   283,   287,   291,   297,   304,   305,
     310,   314,   318,   322,   326,   330,   334,   338,   342,   346,
     350,   354,   358,   362,   368,   375,   385,   391,   398,   406,
     410,   416,   423,   433,   439,   446,   454,   458,   462,   472,
     471,   494,   493,   509,   508,   517,   519,   522,   523,   526,
     528,   530,   537,   534,   545,   544,   567,   571,   574,   578,
     582,   600,   603,   610,   614,   618,   624,   628,   632,   636,
     640,   644,   648,   652,   656,   660,   664,   668,   672,   676,
     680,   684,   688,   692,   696,   700,   704,   708,   714,   721,
     730,   741,   748,   755,   758,   764,   771,   781,   785,   789,
     793,   800,   817,   835,   868,   877,   884,   893,   901,   907,
     917,   932,   954,   969,   995,  1004,  1005,  1033,  1087,  1091,
    1092,  1095,  1098,  1099,  1103,  1104,  1109,  1108,  1112,  1111,
    1114,  1116,  1120,  1129,  1131,  1132,  1135,  1137,  1144,  1151,
    1157,  1164,  1166,  1168,  1170,  1174,  1176,  1188,  1192,  1194,
    1198,  1202,  1206,  1210,  1214,  1218,  1222,  1226,  1230,  1234,
    1238,  1242,  1246,  1250,  1254,  1258,  1262,  1266,  1270,  1274,
    1278,  1282,  1286,  1290,  1294,  1298,  1302,  1306,  1309,  1314,
    1320,  1323,  1328,  1334,  1337,  1342,  1348,  1351,  1356,  1362,
    1366,  1370,  1374,  1381,  1385,  1387,  1391,  1392,  1400,  1408,
    1419,  1421,  1430,  1436,  1443,  1444,  1451,  1455,  1456,  1459,
    1460,  1463,  1467,  1469,  1473,  1475,  1477,  1479,  1481,  1483,
    1485,  1487,  1489,  1491,  1493,  1495,  1497,  1499,  1501,  1503,
    1505,  1507,  1509,  1511,  1551,  1553,  1555,  1557,  1559,  1561,
    1563,  1565,  1567,  1569,  1571,  1573,  1575,  1577,  1579,  1581,
    1583,  1599,  1600,  1601,  1602,  1603,  1604,  1607,  1608,  1616,
    1628
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "FLOAT", "DECFLOAT", "STRING",
  "NSSTRING", "SELECTOR", "CHAR", "NAME", "UNKNOWN_CPP_NAME", "COMPLETE",
  "TYPENAME", "CLASSNAME", "OBJC_LBRAC", "NAME_OR_INT", "OPERATOR",
  "STRUCT", "CLASS", "UNION", "ENUM", "SIZEOF", "UNSIGNED", "COLONCOLON",
  "TEMPLATE", "ERROR", "NEW", "DELETE", "REINTERPRET_CAST", "DYNAMIC_CAST",
  "STATIC_CAST", "CONST_CAST", "ENTRY", "TYPEOF", "DECLTYPE", "TYPEID",
  "SIGNED_KEYWORD", "LONG", "SHORT", "INT_KEYWORD", "CONST_KEYWORD",
  "VOLATILE_KEYWORD", "DOUBLE_KEYWORD", "VARIABLE", "ASSIGN_MODIFY",
  "TRUEKEYWORD", "FALSEKEYWORD", "','", "ABOVE_COMMA", "'='", "'?'",
  "OROR", "ANDAND", "'|'", "'^'", "'&'", "NOTEQUAL", "EQUAL", "'<'", "'>'",
  "GEQ", "LEQ", "RSH", "LSH", "'@'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "DECREMENT", "INCREMENT", "UNARY", "'.'", "'['", "'('", "DOT_STAR",
  "ARROW_STAR", "ARROW", "BLOCKNAME", "FILENAME", "DOTDOTDOT", "')'",
  "'!'", "'~'", "']'", "':'", "'{'", "'}'", "$accept", "start", "type_exp",
  "exp1", "exp", "$@1", "$@2", "$@3", "msglist", "msgarglist", "msgarg",
  "$@4", "$@5", "lcurly", "arglist", "rcurly", "string_exp", "block",
  "variable", "qualified_name", "space_identifier", "const_or_volatile",
  "cv_with_space_id", "const_or_volatile_or_space_identifier_noopt",
  "const_or_volatile_or_space_identifier", "ptr_operator", "$@6", "$@7",
  "ptr_operator_ts", "abs_decl", "direct_abs_decl", "array_mod",
  "func_mod", "type", "typebase", "typename", "parameter_typelist",
  "nonempty_typelist", "ptype", "conversion_type_id",
  "conversion_declarator", "const_and_volatile", "const_or_volatile_noopt",
  "operator", "name", "name_not_typename", YY_NULL
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,    44,   303,
      61,    63,   304,   305,   124,    94,    38,   306,   307,    60,
      62,   308,   309,   310,   311,    64,    43,    45,    42,    47,
      37,   312,   313,   314,    46,    91,    40,   315,   316,   317,
     318,   319,   320,    41,    33,   126,    93,    58,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    90,    91,    91,    92,    92,    92,    92,    93,    93,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    95,
      94,    96,    94,    97,    94,    98,    98,    99,    99,   100,
     100,   100,   101,    94,   102,    94,   103,   104,   104,   104,
      94,   105,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,   106,   106,    94,    94,    94,    94,   107,   107,   107,
     108,   108,   109,   109,   109,   108,   108,   108,   110,   111,
     111,   112,   113,   113,   114,   114,   116,   115,   117,   115,
     115,   115,   118,   119,   119,   119,   120,   120,   120,   120,
     120,   121,   121,   121,   121,   122,   122,   123,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   125,   125,   125,   125,
     126,   126,   127,   127,   128,   128,   129,   130,   130,   131,
     131,   132,   132,   132,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   134,   134,   134,   134,   134,   134,   135,   135,   135,
     135
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     4,     4,     4,     1,     3,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       4,     4,     2,     3,     4,     3,     4,     5,     3,     3,
       3,     4,     3,     4,     5,     3,     3,     4,     4,     0,
       5,     0,     5,     0,     5,     1,     1,     1,     2,     3,
       2,     2,     0,     5,     0,     5,     1,     0,     1,     3,
       5,     1,     3,     4,     4,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     5,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     4,     4,     7,     7,     7,
       7,     1,     2,     1,     1,     1,     1,     1,     1,     3,
       2,     3,     3,     4,     5,     1,     2,     1,     2,     1,
       0,     3,     1,     1,     1,     0,     0,     4,     0,     3,
       1,     2,     1,     2,     1,     1,     3,     2,     1,     2,
       1,     2,     2,     3,     3,     2,     3,     1,     1,     1,
       1,     1,     2,     3,     2,     3,     3,     3,     2,     2,
       3,     4,     3,     3,     4,     3,     4,     3,     4,     2,
       3,     2,     3,     2,     3,     1,     2,     2,     2,     3,
       2,     2,     3,     2,     2,     3,     2,     2,     3,     2,
       1,     2,     1,     5,     2,     2,     1,     1,     1,     1,
       1,     3,     1,     3,     1,     2,     2,     0,     2,     2,
       2,     1,     1,     1,     2,     2,     4,     4,     4,     4,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     3,     3,     3,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
     120,    88,    91,    92,   101,   104,     0,    89,   257,   260,
     148,     0,    90,   120,     0,     0,     0,     0,     0,   190,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   192,
     150,   151,   149,   212,   213,   175,    94,   105,   106,     0,
       0,     0,     0,     0,     0,   120,   258,   108,     0,     0,
      56,     0,     3,     2,     8,    57,   103,     0,    93,   115,
       0,   122,   120,     4,   204,   147,   211,   123,   259,   117,
       0,    54,     0,    39,    41,    43,   148,     0,   214,   215,
     233,   244,   230,   241,   240,   227,   225,   226,   237,   236,
     231,   232,   239,   238,   235,   234,   220,   221,   222,   223,
     224,   243,   242,     0,     0,   245,   246,   229,   228,   207,
     250,   251,   255,   178,   253,   254,   252,   256,   177,   181,
     180,   184,   183,   187,   186,     0,   120,    22,   196,   198,
     199,   197,   189,   260,   258,   116,     0,   120,   120,   120,
     120,   120,     0,   120,   198,   199,   191,   158,   154,   159,
     152,   176,   173,   171,   169,   209,   210,    11,    13,    12,
      10,    17,    16,     0,     0,    14,    15,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    19,    18,     0,     0,    52,     0,     0,     0,
       0,    58,     0,   102,     0,     0,   120,   194,   195,     0,
     130,   128,     0,   120,   132,   134,   205,   135,   138,   140,
     110,     0,    57,     0,   112,     0,     0,     0,   249,     0,
       0,     0,     0,   248,   247,   207,   206,   179,   182,   185,
     188,     0,   165,   156,   172,   120,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   163,   155,   157,   153,   167,
     162,   160,   174,   170,     0,    65,     9,     0,    87,    86,
       0,    84,    83,    82,    81,    80,    75,    74,    78,    79,
      77,    76,    73,    72,    66,    70,    71,    67,    68,    69,
      32,   253,     0,    35,    30,     0,    57,   202,     0,   200,
      36,    29,    25,     0,    28,    23,    61,     0,     0,    62,
     111,   118,   121,   119,     0,   142,   131,   125,     0,   141,
     145,     0,     0,   126,   133,   120,   137,   139,    95,     0,
     113,     0,     0,     0,     0,    46,    47,    45,     0,     0,
     218,   216,   219,   217,   126,   208,    96,   166,     0,     0,
       0,     0,     0,     5,     6,     7,    21,    20,   164,   168,
     161,    64,    38,     0,    33,    31,    37,     0,   120,   120,
      26,    24,    63,    59,   144,   124,   129,   143,   136,   146,
     125,    55,   114,    51,    50,    40,    48,     0,     0,    42,
      44,   193,     0,     0,     0,     0,    85,    34,    53,    60,
     201,   203,    27,   127,    49,     0,     0,     0,     0,    97,
      99,    98,   100
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    51,   163,   164,    54,   225,   226,   227,   334,   335,
     336,   296,   222,    55,   202,   307,    56,    57,    58,    59,
     206,    60,    61,    62,   376,   214,   380,   317,   215,   216,
     217,   218,   219,    63,    64,   132,   322,   299,    65,   110,
     236,    66,    67,    68,   337,    69
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -214
static const yytype_int16 yypact[] =
{
     468,  -214,  -214,  -214,  -214,  -214,   -24,  -214,  -214,    -2,
      58,   640,  -214,   886,   265,   292,   330,   347,   726,   110,
      25,   193,    33,    48,    63,    84,    50,    80,    82,   185,
     328,    22,  -214,   139,   149,  -214,  -214,  -214,  -214,   812,
     812,   812,   812,   812,   812,   468,   167,  -214,   812,   812,
    -214,   192,  -214,   145,  1399,   382,   189,   172,  -214,  -214,
     135,  -214,   746,  -214,    86,   215,  -214,   136,  -214,   186,
     193,  -214,    53,    58,  -214,  1399,  -214,   141,    19,    43,
    -214,  -214,  -214,  -214,  -214,  -214,  -214,  -214,  -214,  -214,
    -214,  -214,  -214,  -214,  -214,  -214,  -214,  -214,  -214,  -214,
    -214,  -214,  -214,   142,   146,  -214,  -214,  -214,  -214,   271,
    -214,  -214,  -214,  -214,  -214,  -214,  -214,  -214,   220,  -214,
     222,  -214,   223,  -214,   230,    58,   468,    24,  -214,    96,
     204,  -214,  -214,  -214,  -214,  -214,   203,   660,   660,   660,
     660,   554,   812,   468,    97,  -214,  -214,   211,   232,   288,
    -214,  -214,   239,   240,  -214,  -214,  -214,    24,    24,    24,
      24,    24,    24,   180,   -23,    24,    24,  -214,   812,   812,
     812,   812,   812,   812,   812,   812,   812,   812,   812,   812,
     812,   812,   812,   812,   812,   812,   812,   812,   812,   812,
     812,   812,  -214,  -214,    67,   812,  1669,   812,   812,   103,
     195,  1399,    -8,  -214,   193,   278,    30,    86,  -214,     5,
     221,  -214,    11,   218,   224,    12,  -214,    34,  -214,  -214,
    -214,   216,   812,   193,   276,    37,    37,    37,  -214,   228,
     229,   233,   234,  -214,  -214,   -13,  -214,  -214,  -214,  -214,
    -214,   235,   267,  -214,  -214,   746,   256,   263,   264,   269,
     993,   248,  1051,   249,  1109,   293,  -214,  -214,  -214,   295,
     297,  -214,  -214,  -214,   812,  -214,  1399,    20,  1399,  1399,
     922,  1457,  1486,  1515,  1544,  1573,  1602,  1602,  1072,  1072,
    1072,  1072,   365,   365,  1130,  1188,  1188,    24,    24,    24,
    -214,    58,   193,  -214,   332,    60,   812,  -214,   266,   300,
     138,   138,  -214,   193,  -214,   338,  -214,   812,   812,  -214,
     329,  -214,  -214,  -214,   268,  -214,   224,    86,   270,  -214,
    -214,   272,   279,  -214,    34,   227,  -214,  -214,  -214,   -22,
    -214,   193,   812,   812,   275,    37,  -214,   280,   283,   284,
    -214,  -214,  -214,  -214,  -214,  -214,  -214,  -214,   313,   298,
     299,   302,   303,  -214,  -214,  -214,  -214,  -214,  -214,  -214,
    -214,    24,  -214,   812,   364,  -214,  -214,    38,    30,   992,
     384,  -214,    24,  1399,  -214,  -214,  -214,  -214,  -214,  -214,
      86,  -214,  -214,  1399,  1399,  -214,  -214,   280,   812,  -214,
    -214,  -214,   812,   812,   812,   812,  1428,  -214,  -214,  -214,
    -214,  -214,  -214,  -214,  1399,  1167,  1225,  1283,  1341,  -214,
    -214,  -214,  -214
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -214,  -214,     2,    13,   -11,  -214,  -214,  -214,   -72,  -214,
      59,  -214,  -214,  -214,  -207,   206,  -214,  -214,  -214,  -143,
    -214,  -184,  -214,   -63,    29,  -106,  -214,  -214,  -214,   202,
     231,  -213,  -212,  -120,    28,   416,   255,  -214,  -214,  -214,
     217,  -214,  -183,    -5,     3,   435
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -121
static const yytype_int16 yytable[] =
{
      75,   208,    52,   235,   326,   327,   241,   127,   314,   117,
     117,   117,   117,    53,   318,   329,   117,   118,   120,   122,
     124,   251,   312,   313,   136,   168,   308,   209,   157,   158,
     159,   160,   161,   162,   229,     8,   133,   165,   166,   169,
     308,   109,    13,   210,   201,   152,   208,   111,   112,   209,
     114,   293,    70,   115,    13,   344,   304,   200,   231,   153,
     265,   381,   154,   111,   112,   117,   114,   117,   168,   115,
      13,    33,    34,   221,    71,   224,   297,   111,   112,   290,
     291,   306,    72,   115,    13,   332,   308,   212,   213,   367,
     207,   315,   137,   297,   230,   192,   193,   319,   194,   195,
     196,   197,   198,   199,   316,   134,   362,   138,   168,   212,
     325,   326,   327,   111,   112,   302,   291,   116,   232,   115,
      13,   398,   139,   128,   333,   348,   141,    33,    34,   235,
     250,   252,   254,   116,   242,   255,   243,   256,   223,   246,
     247,   248,   249,   140,   208,   253,   366,   116,   129,   130,
     131,  -120,   292,   169,   338,   339,   142,   266,   143,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   155,   267,   116,   399,   313,   300,   301,   303,   117,
     156,  -107,   167,   168,   117,   203,   204,   294,   128,   117,
     205,  -119,   305,   111,   112,   297,   114,   310,   295,   115,
      13,   201,   194,   195,   196,   197,   198,   199,   117,   220,
     117,   117,   117,   144,   145,   131,   330,   228,   233,   234,
     209,    76,   237,   209,   238,   239,    14,    15,    16,    17,
      76,    19,   240,    21,   244,    14,    15,    16,    17,   401,
      19,   257,    21,   361,   375,    29,    30,    31,    32,    33,
      34,    35,   245,   264,    29,    30,    31,    32,    33,    34,
      35,   210,   258,   116,   210,   111,   112,   113,   114,   262,
     263,   115,    13,   211,   306,   201,   211,   117,   311,   211,
     212,   213,   323,   212,   213,   364,   372,   373,   117,   328,
     331,   320,   111,   112,   119,   114,   370,   347,   115,    13,
     320,   259,    33,    34,   340,   341,   349,   375,   346,   342,
     343,   383,   384,   350,   351,   260,   117,   210,   261,   352,
     117,   354,   356,   358,   382,   359,  -120,   360,   387,   211,
     111,   112,   121,   114,   365,   116,   115,    13,   369,   368,
     371,   147,   396,  -109,   374,   378,   377,   111,   112,   123,
     114,   385,   379,   115,    13,   148,   149,   388,   150,   389,
     390,   151,   116,   391,   392,   393,   397,   404,   394,   395,
     169,   405,   406,   407,   408,     1,     2,     3,     4,     5,
       6,     7,     8,     9,   386,    10,   402,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,   309,   403,
     116,    22,    23,    24,    25,   321,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,   116,    37,    38,
     186,   187,   188,   189,   190,   191,   192,   193,    39,   194,
     195,   196,   197,   198,   199,   146,   324,  -120,    40,    41,
      42,   298,   345,    43,    44,   135,     0,     0,    45,     0,
       0,     0,    46,    47,     0,     0,    48,    49,     0,     0,
      50,     1,     2,     3,     4,     5,     6,     7,     8,     9,
       0,    10,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,     0,     0,     0,    22,    23,    24,
      25,     0,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,     0,    37,    38,     0,     0,     0,     0,
       0,     0,     0,     0,    39,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    40,    41,    42,     0,     0,    43,
      44,     0,     0,     0,    45,     0,     0,     0,    46,    47,
       0,     0,    48,    49,     0,     0,    50,     1,     2,     3,
       4,     5,     6,     7,     8,     9,     0,    10,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
       0,     0,     0,    22,    23,    24,    25,     0,     0,     0,
      28,    29,    30,    31,    32,    33,    34,    35,    36,     0,
      37,    38,     0,     0,     0,     0,     0,     0,     0,     0,
      39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      40,    41,    42,     0,     0,    43,    44,     0,     0,     0,
      45,     0,     0,     0,    46,    47,     0,     0,    48,    49,
       0,     0,    50,     1,     2,     3,     4,     5,     6,     7,
       8,     9,     0,    73,    74,    11,    12,    13,     0,     0,
       0,     0,    18,     0,    20,     0,     0,     0,     0,    22,
      23,    24,    25,    76,     0,     0,    28,     0,    14,    15,
      16,    17,     0,    19,    36,    21,    37,    38,     0,     0,
       0,     0,     0,     0,    26,    27,    39,    29,    30,    31,
      32,    33,    34,    35,     0,     0,    40,    41,    42,     0,
       0,    43,    44,     0,     0,     0,    45,     0,     0,     0,
      46,    47,     0,     0,    48,    49,     0,     0,    50,     1,
       2,     3,     4,     5,     6,     7,     8,     9,     0,   125,
       0,    11,    12,    13,     0,     0,     0,     0,    18,     0,
      20,     0,     0,     0,     0,    22,    23,    24,    25,    76,
       0,     0,    28,     0,    14,    15,    16,    17,     0,    19,
      36,    21,    37,    38,     0,     0,     0,     0,     0,     0,
       0,     0,    39,    29,    30,    31,    32,    33,    34,    35,
       0,     0,    40,    41,    42,     0,     0,    43,    44,     0,
       0,     0,   126,     0,     0,     0,    46,    47,     0,     0,
      48,    49,     0,     0,    50,     1,     2,     3,     4,     5,
       6,     7,     8,     9,     0,   125,     0,    11,    12,    13,
       0,     0,     0,     0,    18,     0,    20,     0,     0,     0,
       0,    22,    23,    24,    25,     0,     0,     0,    28,     0,
       0,     0,     0,     0,     0,     0,    36,     0,    37,    38,
       0,     0,     0,     0,     0,     0,     0,     0,    39,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    40,    41,
      42,     0,     0,    43,    44,     0,     0,     0,    45,     0,
       0,     0,    46,    47,     0,     0,    48,    49,     0,    76,
      50,    77,     0,     0,    14,    15,    16,    17,     0,    19,
       0,    21,     0,    78,    79,     0,     0,     0,     0,     0,
       0,     0,     0,    29,    30,    31,    32,    33,    34,    35,
       0,    80,     0,     0,    81,     0,    82,   169,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,     0,    96,    97,    98,    99,   100,   101,   102,     0,
       0,   103,   104,     0,   105,   106,     0,   170,     0,     0,
     107,   108,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,     0,   194,   195,   196,   197,
     198,   199,     0,     0,     0,    76,     0,     0,   169,   363,
      14,    15,    16,    17,     0,    19,     0,    21,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    29,
      30,    31,    32,    33,    34,    35,     0,     0,   170,     0,
       0,     0,     0,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   169,   194,   195,   196,
     197,   198,   199,     0,   400,     0,   353,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   169,     0,     0,
       0,     0,     0,     0,     0,     0,   170,     0,     0,     0,
       0,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   169,   194,   195,   196,   197,   198,
     199,     0,     0,     0,   355,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   169,   194,   195,   196,   197,
     198,   199,     0,     0,   170,     0,     0,     0,     0,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   169,   194,   195,   196,   197,   198,   199,     0,
       0,     0,   357,     0,     0,     0,   187,   188,   189,   190,
     191,   192,   193,   169,   194,   195,   196,   197,   198,   199,
       0,     0,   170,     0,     0,     0,     0,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     169,   194,   195,   196,   197,   198,   199,     0,     0,     0,
     409,     0,     0,     0,     0,     0,   189,   190,   191,   192,
     193,     0,   194,   195,   196,   197,   198,   199,     0,     0,
     170,     0,     0,     0,     0,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   169,   194,
     195,   196,   197,   198,   199,     0,     0,     0,   410,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   170,     0,
       0,     0,     0,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   169,   194,   195,   196,
     197,   198,   199,     0,     0,     0,   411,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   170,     0,     0,     0,
       0,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   169,   194,   195,   196,   197,   198,
     199,     0,     0,     0,   412,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   169,   170,     0,     0,     0,     0,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   169,   194,   195,   196,   197,   198,   199,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   192,
     193,   169,   194,   195,   196,   197,   198,   199,     0,     0,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     169,   194,   195,   196,   197,   198,   199,     0,     0,     0,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   169,
     194,   195,   196,   197,   198,   199,     0,     0,     0,     0,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   169,   194,
     195,   196,   197,   198,   199,     0,     0,     0,     0,     0,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   169,   194,   195,
     196,   197,   198,   199,     0,     0,     0,     0,     0,     0,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,     0,   194,   195,   196,
     197,   198,   199,     0,     0,     0,     0,     0,     0,     0,
       0,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,     0,   194,   195,   196,   197,
     198,   199,    76,     0,     0,     0,     0,    14,    15,    16,
      17,     0,    19,     0,    21,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    29,    30,    31,    32,
      33,    34,    35,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  -120
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-214)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      11,    64,     0,   109,   217,   217,   126,    18,     3,    14,
      15,    16,    17,     0,     3,   222,    21,    14,    15,    16,
      17,   141,   206,   206,    21,    48,    48,    15,    39,    40,
      41,    42,    43,    44,    15,    10,    11,    48,    49,    15,
      48,    13,    17,    56,    55,    23,   109,    10,    11,    15,
      13,   194,    76,    16,    17,    68,   199,    55,    15,    37,
      83,    83,    40,    10,    11,    70,    13,    72,    48,    16,
      17,    41,    42,    70,    76,    72,   196,    10,    11,    12,
      13,    89,    24,    16,    17,    48,    48,    75,    76,   296,
      62,    86,    59,   213,    75,    71,    72,    86,    74,    75,
      76,    77,    78,    79,   210,    80,    86,    59,    48,    75,
      76,   324,   324,    10,    11,    12,    13,    80,    75,    16,
      17,    83,    59,    13,    87,   245,    76,    41,    42,   235,
     141,   142,   143,    80,    38,    38,    40,    40,    85,   137,
     138,   139,   140,    59,   207,   143,    86,    80,    38,    39,
      40,    65,    85,    15,   226,   227,    76,   168,    76,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    42,   169,    80,   368,   368,   197,   198,    85,   194,
      41,    24,     0,    48,   199,     6,    24,   194,    13,   204,
      65,    65,   199,    10,    11,   325,    13,   204,   195,    16,
      17,   222,    74,    75,    76,    77,    78,    79,   223,    33,
     225,   226,   227,    38,    39,    40,   223,    86,    86,    83,
      15,    13,    12,    15,    12,    12,    18,    19,    20,    21,
      13,    23,    12,    25,    40,    18,    19,    20,    21,   369,
      23,    40,    25,   264,   317,    37,    38,    39,    40,    41,
      42,    43,    59,    83,    37,    38,    39,    40,    41,    42,
      43,    56,    40,    80,    56,    10,    11,    12,    13,    40,
      40,    16,    17,    68,    89,   296,    68,   292,    10,    68,
      75,    76,    68,    75,    76,   292,   307,   308,   303,    83,
      24,    83,    10,    11,    12,    13,   303,    40,    16,    17,
      83,    23,    41,    42,    86,    86,    60,   380,    83,    86,
      86,   332,   333,    60,    60,    37,   331,    56,    40,    60,
     335,    83,    83,    40,   331,    40,    65,    40,   335,    68,
      10,    11,    12,    13,    12,    80,    16,    17,    48,    83,
      12,    23,   363,    24,    86,    83,    86,    10,    11,    12,
      13,    86,    83,    16,    17,    37,    38,    87,    40,    86,
      86,    43,    80,    60,    76,    76,    12,   388,    76,    76,
      15,   392,   393,   394,   395,     3,     4,     5,     6,     7,
       8,     9,    10,    11,   335,    13,    12,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,   202,   380,
      80,    29,    30,    31,    32,   213,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    80,    46,    47,
      65,    66,    67,    68,    69,    70,    71,    72,    56,    74,
      75,    76,    77,    78,    79,    29,   215,    65,    66,    67,
      68,   196,   235,    71,    72,    20,    -1,    -1,    76,    -1,
      -1,    -1,    80,    81,    -1,    -1,    84,    85,    -1,    -1,
      88,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      -1,    13,    -1,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    -1,    -1,    -1,    29,    30,    31,
      32,    -1,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    -1,    -1,    71,
      72,    -1,    -1,    -1,    76,    -1,    -1,    -1,    80,    81,
      -1,    -1,    84,    85,    -1,    -1,    88,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    13,    -1,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      -1,    -1,    -1,    29,    30,    31,    32,    -1,    -1,    -1,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    -1,
      46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    -1,    71,    72,    -1,    -1,    -1,
      76,    -1,    -1,    -1,    80,    81,    -1,    -1,    84,    85,
      -1,    -1,    88,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    -1,    13,    14,    15,    16,    17,    -1,    -1,
      -1,    -1,    22,    -1,    24,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    13,    -1,    -1,    36,    -1,    18,    19,
      20,    21,    -1,    23,    44,    25,    46,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    35,    56,    37,    38,    39,
      40,    41,    42,    43,    -1,    -1,    66,    67,    68,    -1,
      -1,    71,    72,    -1,    -1,    -1,    76,    -1,    -1,    -1,
      80,    81,    -1,    -1,    84,    85,    -1,    -1,    88,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    13,
      -1,    15,    16,    17,    -1,    -1,    -1,    -1,    22,    -1,
      24,    -1,    -1,    -1,    -1,    29,    30,    31,    32,    13,
      -1,    -1,    36,    -1,    18,    19,    20,    21,    -1,    23,
      44,    25,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    37,    38,    39,    40,    41,    42,    43,
      -1,    -1,    66,    67,    68,    -1,    -1,    71,    72,    -1,
      -1,    -1,    76,    -1,    -1,    -1,    80,    81,    -1,    -1,
      84,    85,    -1,    -1,    88,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    13,    -1,    15,    16,    17,
      -1,    -1,    -1,    -1,    22,    -1,    24,    -1,    -1,    -1,
      -1,    29,    30,    31,    32,    -1,    -1,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,    46,    47,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    -1,    -1,    71,    72,    -1,    -1,    -1,    76,    -1,
      -1,    -1,    80,    81,    -1,    -1,    84,    85,    -1,    13,
      88,    15,    -1,    -1,    18,    19,    20,    21,    -1,    23,
      -1,    25,    -1,    27,    28,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    38,    39,    40,    41,    42,    43,
      -1,    45,    -1,    -1,    48,    -1,    50,    15,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    -1,    66,    67,    68,    69,    70,    71,    72,    -1,
      -1,    75,    76,    -1,    78,    79,    -1,    45,    -1,    -1,
      84,    85,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    74,    75,    76,    77,
      78,    79,    -1,    -1,    -1,    13,    -1,    -1,    15,    87,
      18,    19,    20,    21,    -1,    23,    -1,    25,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      38,    39,    40,    41,    42,    43,    -1,    -1,    45,    -1,
      -1,    -1,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    15,    74,    75,    76,
      77,    78,    79,    -1,    82,    -1,    83,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    15,    74,    75,    76,    77,    78,
      79,    -1,    -1,    -1,    83,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    15,    74,    75,    76,    77,
      78,    79,    -1,    -1,    45,    -1,    -1,    -1,    -1,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    15,    74,    75,    76,    77,    78,    79,    -1,
      -1,    -1,    83,    -1,    -1,    -1,    66,    67,    68,    69,
      70,    71,    72,    15,    74,    75,    76,    77,    78,    79,
      -1,    -1,    45,    -1,    -1,    -1,    -1,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      15,    74,    75,    76,    77,    78,    79,    -1,    -1,    -1,
      83,    -1,    -1,    -1,    -1,    -1,    68,    69,    70,    71,
      72,    -1,    74,    75,    76,    77,    78,    79,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    15,    74,
      75,    76,    77,    78,    79,    -1,    -1,    -1,    83,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    -1,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    15,    74,    75,    76,
      77,    78,    79,    -1,    -1,    -1,    83,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    15,    74,    75,    76,    77,    78,
      79,    -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    15,    45,    -1,    -1,    -1,    -1,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    15,    74,    75,    76,    77,    78,    79,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    15,    74,    75,    76,    77,    78,    79,    -1,    -1,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      15,    74,    75,    76,    77,    78,    79,    -1,    -1,    -1,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    15,
      74,    75,    76,    77,    78,    79,    -1,    -1,    -1,    -1,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    15,    74,
      75,    76,    77,    78,    79,    -1,    -1,    -1,    -1,    -1,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    15,    74,    75,
      76,    77,    78,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    74,    75,    76,
      77,    78,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    74,    75,    76,    77,
      78,    79,    13,    -1,    -1,    -1,    -1,    18,    19,    20,
      21,    -1,    23,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      13,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    29,    30,    31,    32,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    46,    47,    56,
      66,    67,    68,    71,    72,    76,    80,    81,    84,    85,
      88,    91,    92,    93,    94,   103,   106,   107,   108,   109,
     111,   112,   113,   123,   124,   128,   131,   132,   133,   135,
      76,    76,    24,    13,    14,    94,    13,    15,    27,    28,
      45,    48,    50,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    66,    67,    68,    69,
      70,    71,    72,    75,    76,    78,    79,    84,    85,   124,
     129,    10,    11,    12,    13,    16,    80,   133,   134,    12,
     134,    12,   134,    12,   134,    13,    76,    94,    13,    38,
      39,    40,   125,    11,    80,   135,   134,    59,    59,    59,
      59,    76,    76,    76,    38,    39,   125,    23,    37,    38,
      40,    43,    23,    37,    40,    42,    41,    94,    94,    94,
      94,    94,    94,    92,    93,    94,    94,     0,    48,    15,
      45,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    74,    75,    76,    77,    78,    79,
      92,    94,   104,     6,    24,    65,   110,   124,   113,    15,
      56,    68,    75,    76,   115,   118,   119,   120,   121,   122,
      33,   134,   102,    85,   134,    95,    96,    97,    86,    15,
      75,    15,    75,    86,    83,   115,   130,    12,    12,    12,
      12,   123,    38,    40,    40,    59,    92,    92,    92,    92,
      94,   123,    94,    92,    94,    38,    40,    40,    40,    23,
      37,    40,    40,    40,    83,    83,    94,    93,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      12,    13,    85,   109,   134,    93,   101,   123,   126,   127,
      94,    94,    12,    85,   109,   134,    89,   105,    48,   105,
     134,    10,   111,   132,     3,    86,   115,   117,     3,    86,
      83,   119,   126,    68,   120,    76,   121,   122,    83,   104,
     134,    24,    48,    87,    98,    99,   100,   134,    98,    98,
      86,    86,    86,    86,    68,   130,    83,    40,   123,    60,
      60,    60,    60,    83,    83,    83,    83,    83,    40,    40,
      40,    94,    86,    87,   134,    12,    86,   104,    83,    48,
     134,    12,    94,    94,    86,   113,   114,    86,    83,    83,
     116,    83,   134,    94,    94,    86,   100,   134,    87,    86,
      86,    60,    76,    76,    76,    76,    94,    12,    83,   111,
      82,   123,    12,   114,    94,    94,    94,    94,    94,    83,
      83,    83,    83
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
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
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
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  YYUSE (yytype);
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
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

  YYUSE (yytype);
}




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

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
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
  if (yypact_value_is_default (yyn))
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
      if (yytable_value_is_error (yyn))
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
/* Line 1787 of yacc.c  */
#line 284 "c-exp.y"
    { write_exp_elt_opcode(OP_TYPE);
			  write_exp_elt_type((yyvsp[(1) - (1)].tval));
			  write_exp_elt_opcode(OP_TYPE);}
    break;

  case 5:
/* Line 1787 of yacc.c  */
#line 288 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_TYPEOF);
			}
    break;

  case 6:
/* Line 1787 of yacc.c  */
#line 292 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_TYPE);
			  write_exp_elt_type ((yyvsp[(3) - (4)].tval));
			  write_exp_elt_opcode (OP_TYPE);
			}
    break;

  case 7:
/* Line 1787 of yacc.c  */
#line 298 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_DECLTYPE);
			}
    break;

  case 9:
/* Line 1787 of yacc.c  */
#line 306 "c-exp.y"
    { write_exp_elt_opcode (BINOP_COMMA); }
    break;

  case 10:
/* Line 1787 of yacc.c  */
#line 311 "c-exp.y"
    { write_exp_elt_opcode (UNOP_IND); }
    break;

  case 11:
/* Line 1787 of yacc.c  */
#line 315 "c-exp.y"
    { write_exp_elt_opcode (UNOP_ADDR); }
    break;

  case 12:
/* Line 1787 of yacc.c  */
#line 319 "c-exp.y"
    { write_exp_elt_opcode (UNOP_NEG); }
    break;

  case 13:
/* Line 1787 of yacc.c  */
#line 323 "c-exp.y"
    { write_exp_elt_opcode (UNOP_PLUS); }
    break;

  case 14:
/* Line 1787 of yacc.c  */
#line 327 "c-exp.y"
    { write_exp_elt_opcode (UNOP_LOGICAL_NOT); }
    break;

  case 15:
/* Line 1787 of yacc.c  */
#line 331 "c-exp.y"
    { write_exp_elt_opcode (UNOP_COMPLEMENT); }
    break;

  case 16:
/* Line 1787 of yacc.c  */
#line 335 "c-exp.y"
    { write_exp_elt_opcode (UNOP_PREINCREMENT); }
    break;

  case 17:
/* Line 1787 of yacc.c  */
#line 339 "c-exp.y"
    { write_exp_elt_opcode (UNOP_PREDECREMENT); }
    break;

  case 18:
/* Line 1787 of yacc.c  */
#line 343 "c-exp.y"
    { write_exp_elt_opcode (UNOP_POSTINCREMENT); }
    break;

  case 19:
/* Line 1787 of yacc.c  */
#line 347 "c-exp.y"
    { write_exp_elt_opcode (UNOP_POSTDECREMENT); }
    break;

  case 20:
/* Line 1787 of yacc.c  */
#line 351 "c-exp.y"
    { write_exp_elt_opcode (OP_TYPEID); }
    break;

  case 21:
/* Line 1787 of yacc.c  */
#line 355 "c-exp.y"
    { write_exp_elt_opcode (OP_TYPEID); }
    break;

  case 22:
/* Line 1787 of yacc.c  */
#line 359 "c-exp.y"
    { write_exp_elt_opcode (UNOP_SIZEOF); }
    break;

  case 23:
/* Line 1787 of yacc.c  */
#line 363 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string ((yyvsp[(3) - (3)].sval));
			  write_exp_elt_opcode (STRUCTOP_PTR); }
    break;

  case 24:
/* Line 1787 of yacc.c  */
#line 369 "c-exp.y"
    { mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string ((yyvsp[(3) - (4)].sval));
			  write_exp_elt_opcode (STRUCTOP_PTR); }
    break;

  case 25:
/* Line 1787 of yacc.c  */
#line 376 "c-exp.y"
    { struct stoken s;
			  mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_PTR);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (s);
			  write_exp_elt_opcode (STRUCTOP_PTR); }
    break;

  case 26:
/* Line 1787 of yacc.c  */
#line 386 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_PTR);
			  write_destructor_name ((yyvsp[(4) - (4)].sval));
			  write_exp_elt_opcode (STRUCTOP_PTR); }
    break;

  case 27:
/* Line 1787 of yacc.c  */
#line 392 "c-exp.y"
    { mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_PTR);
			  write_destructor_name ((yyvsp[(4) - (5)].sval));
			  write_exp_elt_opcode (STRUCTOP_PTR); }
    break;

  case 28:
/* Line 1787 of yacc.c  */
#line 399 "c-exp.y"
    { /* exp->type::name becomes exp->*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (STRUCTOP_MPTR); }
    break;

  case 29:
/* Line 1787 of yacc.c  */
#line 407 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_MPTR); }
    break;

  case 30:
/* Line 1787 of yacc.c  */
#line 411 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string ((yyvsp[(3) - (3)].sval));
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 31:
/* Line 1787 of yacc.c  */
#line 417 "c-exp.y"
    { mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string ((yyvsp[(3) - (4)].sval));
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 32:
/* Line 1787 of yacc.c  */
#line 424 "c-exp.y"
    { struct stoken s;
			  mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_STRUCT);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (s);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 33:
/* Line 1787 of yacc.c  */
#line 434 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_destructor_name ((yyvsp[(4) - (4)].sval));
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 34:
/* Line 1787 of yacc.c  */
#line 440 "c-exp.y"
    { mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_destructor_name ((yyvsp[(4) - (5)].sval));
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 35:
/* Line 1787 of yacc.c  */
#line 447 "c-exp.y"
    { /* exp.type::name becomes exp.*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (STRUCTOP_MEMBER); }
    break;

  case 36:
/* Line 1787 of yacc.c  */
#line 455 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_MEMBER); }
    break;

  case 37:
/* Line 1787 of yacc.c  */
#line 459 "c-exp.y"
    { write_exp_elt_opcode (BINOP_SUBSCRIPT); }
    break;

  case 38:
/* Line 1787 of yacc.c  */
#line 463 "c-exp.y"
    { write_exp_elt_opcode (BINOP_SUBSCRIPT); }
    break;

  case 39:
/* Line 1787 of yacc.c  */
#line 472 "c-exp.y"
    {
			  CORE_ADDR class;

			  class = lookup_objc_class (parse_gdbarch,
						     copy_name ((yyvsp[(2) - (2)].tsym).stoken));
			  if (class == 0)
			    error (_("%s is not an ObjC Class"),
				   copy_name ((yyvsp[(2) - (2)].tsym).stoken));
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (parse_type->builtin_int);
			  write_exp_elt_longcst ((LONGEST) class);
			  write_exp_elt_opcode (OP_LONG);
			  start_msglist();
			}
    break;

  case 40:
/* Line 1787 of yacc.c  */
#line 487 "c-exp.y"
    { write_exp_elt_opcode (OP_OBJC_MSGCALL);
			  end_msglist();
			  write_exp_elt_opcode (OP_OBJC_MSGCALL);
			}
    break;

  case 41:
/* Line 1787 of yacc.c  */
#line 494 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (parse_type->builtin_int);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[(2) - (2)].class).class);
			  write_exp_elt_opcode (OP_LONG);
			  start_msglist();
			}
    break;

  case 42:
/* Line 1787 of yacc.c  */
#line 502 "c-exp.y"
    { write_exp_elt_opcode (OP_OBJC_MSGCALL);
			  end_msglist();
			  write_exp_elt_opcode (OP_OBJC_MSGCALL);
			}
    break;

  case 43:
/* Line 1787 of yacc.c  */
#line 509 "c-exp.y"
    { start_msglist(); }
    break;

  case 44:
/* Line 1787 of yacc.c  */
#line 511 "c-exp.y"
    { write_exp_elt_opcode (OP_OBJC_MSGCALL);
			  end_msglist();
			  write_exp_elt_opcode (OP_OBJC_MSGCALL);
			}
    break;

  case 45:
/* Line 1787 of yacc.c  */
#line 518 "c-exp.y"
    { add_msglist(&(yyvsp[(1) - (1)].sval), 0); }
    break;

  case 49:
/* Line 1787 of yacc.c  */
#line 527 "c-exp.y"
    { add_msglist(&(yyvsp[(1) - (3)].sval), 1); }
    break;

  case 50:
/* Line 1787 of yacc.c  */
#line 529 "c-exp.y"
    { add_msglist(0, 1);   }
    break;

  case 51:
/* Line 1787 of yacc.c  */
#line 531 "c-exp.y"
    { add_msglist(0, 0);   }
    break;

  case 52:
/* Line 1787 of yacc.c  */
#line 537 "c-exp.y"
    { start_arglist (); }
    break;

  case 53:
/* Line 1787 of yacc.c  */
#line 539 "c-exp.y"
    { write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL); }
    break;

  case 54:
/* Line 1787 of yacc.c  */
#line 545 "c-exp.y"
    {
			  /* This could potentially be a an argument defined
			     lookup function (Koenig).  */
			  write_exp_elt_opcode (OP_ADL_FUNC);
			  write_exp_elt_block (expression_context_block);
			  write_exp_elt_sym (NULL); /* Placeholder.  */
			  write_exp_string ((yyvsp[(1) - (2)].ssym).stoken);
			  write_exp_elt_opcode (OP_ADL_FUNC);

			/* This is to save the value of arglist_len
			   being accumulated by an outer function call.  */

			  start_arglist ();
			}
    break;

  case 55:
/* Line 1787 of yacc.c  */
#line 560 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL);
			}
    break;

  case 56:
/* Line 1787 of yacc.c  */
#line 568 "c-exp.y"
    { start_arglist (); }
    break;

  case 58:
/* Line 1787 of yacc.c  */
#line 575 "c-exp.y"
    { arglist_len = 1; }
    break;

  case 59:
/* Line 1787 of yacc.c  */
#line 579 "c-exp.y"
    { arglist_len++; }
    break;

  case 60:
/* Line 1787 of yacc.c  */
#line 583 "c-exp.y"
    { int i;
			  VEC (type_ptr) *type_list = (yyvsp[(3) - (5)].tvec);
			  struct type *type_elt;
			  LONGEST len = VEC_length (type_ptr, type_list);

			  write_exp_elt_opcode (TYPE_INSTANCE);
			  write_exp_elt_longcst (len);
			  for (i = 0;
			       VEC_iterate (type_ptr, type_list, i, type_elt);
			       ++i)
			    write_exp_elt_type (type_elt);
			  write_exp_elt_longcst(len);
			  write_exp_elt_opcode (TYPE_INSTANCE);
			  VEC_free (type_ptr, type_list);
			}
    break;

  case 61:
/* Line 1787 of yacc.c  */
#line 601 "c-exp.y"
    { (yyval.lval) = end_arglist () - 1; }
    break;

  case 62:
/* Line 1787 of yacc.c  */
#line 604 "c-exp.y"
    { write_exp_elt_opcode (OP_ARRAY);
			  write_exp_elt_longcst ((LONGEST) 0);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[(3) - (3)].lval));
			  write_exp_elt_opcode (OP_ARRAY); }
    break;

  case 63:
/* Line 1787 of yacc.c  */
#line 611 "c-exp.y"
    { write_exp_elt_opcode (UNOP_MEMVAL_TYPE); }
    break;

  case 64:
/* Line 1787 of yacc.c  */
#line 615 "c-exp.y"
    { write_exp_elt_opcode (UNOP_CAST_TYPE); }
    break;

  case 65:
/* Line 1787 of yacc.c  */
#line 619 "c-exp.y"
    { }
    break;

  case 66:
/* Line 1787 of yacc.c  */
#line 625 "c-exp.y"
    { write_exp_elt_opcode (BINOP_REPEAT); }
    break;

  case 67:
/* Line 1787 of yacc.c  */
#line 629 "c-exp.y"
    { write_exp_elt_opcode (BINOP_MUL); }
    break;

  case 68:
/* Line 1787 of yacc.c  */
#line 633 "c-exp.y"
    { write_exp_elt_opcode (BINOP_DIV); }
    break;

  case 69:
/* Line 1787 of yacc.c  */
#line 637 "c-exp.y"
    { write_exp_elt_opcode (BINOP_REM); }
    break;

  case 70:
/* Line 1787 of yacc.c  */
#line 641 "c-exp.y"
    { write_exp_elt_opcode (BINOP_ADD); }
    break;

  case 71:
/* Line 1787 of yacc.c  */
#line 645 "c-exp.y"
    { write_exp_elt_opcode (BINOP_SUB); }
    break;

  case 72:
/* Line 1787 of yacc.c  */
#line 649 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LSH); }
    break;

  case 73:
/* Line 1787 of yacc.c  */
#line 653 "c-exp.y"
    { write_exp_elt_opcode (BINOP_RSH); }
    break;

  case 74:
/* Line 1787 of yacc.c  */
#line 657 "c-exp.y"
    { write_exp_elt_opcode (BINOP_EQUAL); }
    break;

  case 75:
/* Line 1787 of yacc.c  */
#line 661 "c-exp.y"
    { write_exp_elt_opcode (BINOP_NOTEQUAL); }
    break;

  case 76:
/* Line 1787 of yacc.c  */
#line 665 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LEQ); }
    break;

  case 77:
/* Line 1787 of yacc.c  */
#line 669 "c-exp.y"
    { write_exp_elt_opcode (BINOP_GEQ); }
    break;

  case 78:
/* Line 1787 of yacc.c  */
#line 673 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LESS); }
    break;

  case 79:
/* Line 1787 of yacc.c  */
#line 677 "c-exp.y"
    { write_exp_elt_opcode (BINOP_GTR); }
    break;

  case 80:
/* Line 1787 of yacc.c  */
#line 681 "c-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_AND); }
    break;

  case 81:
/* Line 1787 of yacc.c  */
#line 685 "c-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_XOR); }
    break;

  case 82:
/* Line 1787 of yacc.c  */
#line 689 "c-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_IOR); }
    break;

  case 83:
/* Line 1787 of yacc.c  */
#line 693 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LOGICAL_AND); }
    break;

  case 84:
/* Line 1787 of yacc.c  */
#line 697 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LOGICAL_OR); }
    break;

  case 85:
/* Line 1787 of yacc.c  */
#line 701 "c-exp.y"
    { write_exp_elt_opcode (TERNOP_COND); }
    break;

  case 86:
/* Line 1787 of yacc.c  */
#line 705 "c-exp.y"
    { write_exp_elt_opcode (BINOP_ASSIGN); }
    break;

  case 87:
/* Line 1787 of yacc.c  */
#line 709 "c-exp.y"
    { write_exp_elt_opcode (BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode ((yyvsp[(2) - (3)].opcode));
			  write_exp_elt_opcode (BINOP_ASSIGN_MODIFY); }
    break;

  case 88:
/* Line 1787 of yacc.c  */
#line 715 "c-exp.y"
    { write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type ((yyvsp[(1) - (1)].typed_val_int).type);
			  write_exp_elt_longcst ((LONGEST)((yyvsp[(1) - (1)].typed_val_int).val));
			  write_exp_elt_opcode (OP_LONG); }
    break;

  case 89:
/* Line 1787 of yacc.c  */
#line 722 "c-exp.y"
    {
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &(yyvsp[(1) - (1)].tsval);
			  write_exp_string_vector ((yyvsp[(1) - (1)].tsval).type, &vec);
			}
    break;

  case 90:
/* Line 1787 of yacc.c  */
#line 731 "c-exp.y"
    { YYSTYPE val;
			  parse_number ((yyvsp[(1) - (1)].ssym).stoken.ptr, (yyvsp[(1) - (1)].ssym).stoken.length, 0, &val);
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (val.typed_val_int.type);
			  write_exp_elt_longcst ((LONGEST)val.typed_val_int.val);
			  write_exp_elt_opcode (OP_LONG);
			}
    break;

  case 91:
/* Line 1787 of yacc.c  */
#line 742 "c-exp.y"
    { write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type ((yyvsp[(1) - (1)].typed_val_float).type);
			  write_exp_elt_dblcst ((yyvsp[(1) - (1)].typed_val_float).dval);
			  write_exp_elt_opcode (OP_DOUBLE); }
    break;

  case 92:
/* Line 1787 of yacc.c  */
#line 749 "c-exp.y"
    { write_exp_elt_opcode (OP_DECFLOAT);
			  write_exp_elt_type ((yyvsp[(1) - (1)].typed_val_decfloat).type);
			  write_exp_elt_decfloatcst ((yyvsp[(1) - (1)].typed_val_decfloat).val);
			  write_exp_elt_opcode (OP_DECFLOAT); }
    break;

  case 94:
/* Line 1787 of yacc.c  */
#line 759 "c-exp.y"
    {
			  write_dollar_variable ((yyvsp[(1) - (1)].sval));
			}
    break;

  case 95:
/* Line 1787 of yacc.c  */
#line 765 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_OBJC_SELECTOR);
			  write_exp_string ((yyvsp[(3) - (4)].sval));
			  write_exp_elt_opcode (OP_OBJC_SELECTOR); }
    break;

  case 96:
/* Line 1787 of yacc.c  */
#line 772 "c-exp.y"
    { write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (lookup_signed_typename
					      (parse_language, parse_gdbarch,
					       "int"));
			  CHECK_TYPEDEF ((yyvsp[(3) - (4)].tval));
			  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH ((yyvsp[(3) - (4)].tval)));
			  write_exp_elt_opcode (OP_LONG); }
    break;

  case 97:
/* Line 1787 of yacc.c  */
#line 782 "c-exp.y"
    { write_exp_elt_opcode (UNOP_REINTERPRET_CAST); }
    break;

  case 98:
/* Line 1787 of yacc.c  */
#line 786 "c-exp.y"
    { write_exp_elt_opcode (UNOP_CAST_TYPE); }
    break;

  case 99:
/* Line 1787 of yacc.c  */
#line 790 "c-exp.y"
    { write_exp_elt_opcode (UNOP_DYNAMIC_CAST); }
    break;

  case 100:
/* Line 1787 of yacc.c  */
#line 794 "c-exp.y"
    { /* We could do more error checking here, but
			     it doesn't seem worthwhile.  */
			  write_exp_elt_opcode (UNOP_CAST_TYPE); }
    break;

  case 101:
/* Line 1787 of yacc.c  */
#line 801 "c-exp.y"
    {
			  /* We copy the string here, and not in the
			     lexer, to guarantee that we do not leak a
			     string.  Note that we follow the
			     NUL-termination convention of the
			     lexer.  */
			  struct typed_stoken *vec = XNEW (struct typed_stoken);
			  (yyval.svec).len = 1;
			  (yyval.svec).tokens = vec;

			  vec->type = (yyvsp[(1) - (1)].tsval).type;
			  vec->length = (yyvsp[(1) - (1)].tsval).length;
			  vec->ptr = xmalloc ((yyvsp[(1) - (1)].tsval).length + 1);
			  memcpy (vec->ptr, (yyvsp[(1) - (1)].tsval).ptr, (yyvsp[(1) - (1)].tsval).length + 1);
			}
    break;

  case 102:
/* Line 1787 of yacc.c  */
#line 818 "c-exp.y"
    {
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  char *p;
			  ++(yyval.svec).len;
			  (yyval.svec).tokens = xrealloc ((yyval.svec).tokens,
					       (yyval.svec).len * sizeof (struct typed_stoken));

			  p = xmalloc ((yyvsp[(2) - (2)].tsval).length + 1);
			  memcpy (p, (yyvsp[(2) - (2)].tsval).ptr, (yyvsp[(2) - (2)].tsval).length + 1);

			  (yyval.svec).tokens[(yyval.svec).len - 1].type = (yyvsp[(2) - (2)].tsval).type;
			  (yyval.svec).tokens[(yyval.svec).len - 1].length = (yyvsp[(2) - (2)].tsval).length;
			  (yyval.svec).tokens[(yyval.svec).len - 1].ptr = p;
			}
    break;

  case 103:
/* Line 1787 of yacc.c  */
#line 836 "c-exp.y"
    {
			  int i;
			  enum c_string_type type = C_STRING;

			  for (i = 0; i < (yyvsp[(1) - (1)].svec).len; ++i)
			    {
			      switch ((yyvsp[(1) - (1)].svec).tokens[i].type)
				{
				case C_STRING:
				  break;
				case C_WIDE_STRING:
				case C_STRING_16:
				case C_STRING_32:
				  if (type != C_STRING
				      && type != (yyvsp[(1) - (1)].svec).tokens[i].type)
				    error (_("Undefined string concatenation."));
				  type = (yyvsp[(1) - (1)].svec).tokens[i].type;
				  break;
				default:
				  /* internal error */
				  internal_error (__FILE__, __LINE__,
						  "unrecognized type in string concatenation");
				}
			    }

			  write_exp_string_vector (type, &(yyvsp[(1) - (1)].svec));
			  for (i = 0; i < (yyvsp[(1) - (1)].svec).len; ++i)
			    xfree ((yyvsp[(1) - (1)].svec).tokens[i].ptr);
			  xfree ((yyvsp[(1) - (1)].svec).tokens);
			}
    break;

  case 104:
/* Line 1787 of yacc.c  */
#line 871 "c-exp.y"
    { write_exp_elt_opcode (OP_OBJC_NSSTRING);
			  write_exp_string ((yyvsp[(1) - (1)].sval));
			  write_exp_elt_opcode (OP_OBJC_NSSTRING); }
    break;

  case 105:
/* Line 1787 of yacc.c  */
#line 878 "c-exp.y"
    { write_exp_elt_opcode (OP_LONG);
                          write_exp_elt_type (parse_type->builtin_bool);
                          write_exp_elt_longcst ((LONGEST) 1);
                          write_exp_elt_opcode (OP_LONG); }
    break;

  case 106:
/* Line 1787 of yacc.c  */
#line 885 "c-exp.y"
    { write_exp_elt_opcode (OP_LONG);
                          write_exp_elt_type (parse_type->builtin_bool);
                          write_exp_elt_longcst ((LONGEST) 0);
                          write_exp_elt_opcode (OP_LONG); }
    break;

  case 107:
/* Line 1787 of yacc.c  */
#line 894 "c-exp.y"
    {
			  if ((yyvsp[(1) - (1)].ssym).sym)
			    (yyval.bval) = SYMBOL_BLOCK_VALUE ((yyvsp[(1) - (1)].ssym).sym);
			  else
			    error (_("No file or function \"%s\"."),
				   copy_name ((yyvsp[(1) - (1)].ssym).stoken));
			}
    break;

  case 108:
/* Line 1787 of yacc.c  */
#line 902 "c-exp.y"
    {
			  (yyval.bval) = (yyvsp[(1) - (1)].bval);
			}
    break;

  case 109:
/* Line 1787 of yacc.c  */
#line 908 "c-exp.y"
    { struct symbol *tem
			    = lookup_symbol (copy_name ((yyvsp[(3) - (3)].sval)), (yyvsp[(1) - (3)].bval),
					     VAR_DOMAIN, NULL);
			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy_name ((yyvsp[(3) - (3)].sval)));
			  (yyval.bval) = SYMBOL_BLOCK_VALUE (tem); }
    break;

  case 110:
/* Line 1787 of yacc.c  */
#line 918 "c-exp.y"
    { struct symbol *sym = (yyvsp[(1) - (2)].ssym).sym;

			  if (sym == NULL || !SYMBOL_IS_ARGUMENT (sym)
			      || !symbol_read_needs_frame (sym))
			    error (_("@entry can be used only for function "
				     "parameters, not for \"%s\""),
				   copy_name ((yyvsp[(1) - (2)].ssym).stoken));

			  write_exp_elt_opcode (OP_VAR_ENTRY_VALUE);
			  write_exp_elt_sym (sym);
			  write_exp_elt_opcode (OP_VAR_ENTRY_VALUE);
			}
    break;

  case 111:
/* Line 1787 of yacc.c  */
#line 933 "c-exp.y"
    { struct symbol *sym;
			  sym = lookup_symbol (copy_name ((yyvsp[(3) - (3)].sval)), (yyvsp[(1) - (3)].bval),
					       VAR_DOMAIN, NULL);
			  if (sym == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy_name ((yyvsp[(3) - (3)].sval)));
			  if (symbol_read_needs_frame (sym))
			    {
			      if (innermost_block == 0
				  || contained_in (block_found,
						   innermost_block))
				innermost_block = block_found;
			    }

			  write_exp_elt_opcode (OP_VAR_VALUE);
			  /* block_found is set by lookup_symbol.  */
			  write_exp_elt_block (block_found);
			  write_exp_elt_sym (sym);
			  write_exp_elt_opcode (OP_VAR_VALUE); }
    break;

  case 112:
/* Line 1787 of yacc.c  */
#line 955 "c-exp.y"
    {
			  struct type *type = (yyvsp[(1) - (3)].tsym).type;
			  CHECK_TYPEDEF (type);
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION
			      && TYPE_CODE (type) != TYPE_CODE_NAMESPACE)
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));

			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string ((yyvsp[(3) - (3)].sval));
			  write_exp_elt_opcode (OP_SCOPE);
			}
    break;

  case 113:
/* Line 1787 of yacc.c  */
#line 970 "c-exp.y"
    {
			  struct type *type = (yyvsp[(1) - (4)].tsym).type;
			  struct stoken tmp_token;
			  char *buf;

			  CHECK_TYPEDEF (type);
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION
			      && TYPE_CODE (type) != TYPE_CODE_NAMESPACE)
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));
			  buf = alloca ((yyvsp[(4) - (4)].sval).length + 2);
			  tmp_token.ptr = buf;
			  tmp_token.length = (yyvsp[(4) - (4)].sval).length + 1;
			  buf[0] = '~';
			  memcpy (buf+1, (yyvsp[(4) - (4)].sval).ptr, (yyvsp[(4) - (4)].sval).length);
			  buf[tmp_token.length] = 0;

			  /* Check for valid destructor name.  */
			  destructor_name_p (tmp_token.ptr, (yyvsp[(1) - (4)].tsym).type);
			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string (tmp_token);
			  write_exp_elt_opcode (OP_SCOPE);
			}
    break;

  case 114:
/* Line 1787 of yacc.c  */
#line 996 "c-exp.y"
    {
			  char *copy = copy_name ((yyvsp[(3) - (5)].sval));
			  error (_("No type \"%s\" within class "
				   "or namespace \"%s\"."),
				 copy, TYPE_SAFE_NAME ((yyvsp[(1) - (5)].tsym).type));
			}
    break;

  case 116:
/* Line 1787 of yacc.c  */
#line 1006 "c-exp.y"
    {
			  char *name = copy_name ((yyvsp[(2) - (2)].ssym).stoken);
			  struct symbol *sym;
			  struct bound_minimal_symbol msymbol;

			  sym =
			    lookup_symbol (name, (const struct block *) NULL,
					   VAR_DOMAIN, NULL);
			  if (sym)
			    {
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      write_exp_elt_block (NULL);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      break;
			    }

			  msymbol = lookup_bound_minimal_symbol (name);
			  if (msymbol.minsym != NULL)
			    write_exp_msymbol (msymbol);
			  else if (!have_full_symbols () && !have_partial_symbols ())
			    error (_("No symbol table is loaded.  Use the \"file\" command."));
			  else
			    error (_("No symbol \"%s\" in current context."), name);
			}
    break;

  case 117:
/* Line 1787 of yacc.c  */
#line 1034 "c-exp.y"
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
			      /* C++: it hangs off of `this'.  Must
			         not inadvertently convert from a method call
				 to data ref.  */
			      if (innermost_block == 0
				  || contained_in (block_found,
						   innermost_block))
				innermost_block = block_found;
			      write_exp_elt_opcode (OP_THIS);
			      write_exp_elt_opcode (OP_THIS);
			      write_exp_elt_opcode (STRUCTOP_PTR);
			      write_exp_string ((yyvsp[(1) - (1)].ssym).stoken);
			      write_exp_elt_opcode (STRUCTOP_PTR);
			    }
			  else
			    {
			      struct bound_minimal_symbol msymbol;
			      char *arg = copy_name ((yyvsp[(1) - (1)].ssym).stoken);

			      msymbol =
				lookup_bound_minimal_symbol (arg);
			      if (msymbol.minsym != NULL)
				write_exp_msymbol (msymbol);
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error (_("No symbol table is loaded.  Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       copy_name ((yyvsp[(1) - (1)].ssym).stoken));
			    }
			}
    break;

  case 118:
/* Line 1787 of yacc.c  */
#line 1088 "c-exp.y"
    { insert_type_address_space (copy_name ((yyvsp[(2) - (2)].ssym).stoken)); }
    break;

  case 126:
/* Line 1787 of yacc.c  */
#line 1109 "c-exp.y"
    { insert_type (tp_pointer); }
    break;

  case 128:
/* Line 1787 of yacc.c  */
#line 1112 "c-exp.y"
    { insert_type (tp_pointer); }
    break;

  case 130:
/* Line 1787 of yacc.c  */
#line 1115 "c-exp.y"
    { insert_type (tp_reference); }
    break;

  case 131:
/* Line 1787 of yacc.c  */
#line 1117 "c-exp.y"
    { insert_type (tp_reference); }
    break;

  case 132:
/* Line 1787 of yacc.c  */
#line 1121 "c-exp.y"
    {
			  (yyval.type_stack) = get_type_stack ();
			  /* This cleanup is eventually run by
			     c_parse.  */
			  make_cleanup (type_stack_cleanup, (yyval.type_stack));
			}
    break;

  case 133:
/* Line 1787 of yacc.c  */
#line 1130 "c-exp.y"
    { (yyval.type_stack) = append_type_stack ((yyvsp[(2) - (2)].type_stack), (yyvsp[(1) - (2)].type_stack)); }
    break;

  case 136:
/* Line 1787 of yacc.c  */
#line 1136 "c-exp.y"
    { (yyval.type_stack) = (yyvsp[(2) - (3)].type_stack); }
    break;

  case 137:
/* Line 1787 of yacc.c  */
#line 1138 "c-exp.y"
    {
			  push_type_stack ((yyvsp[(1) - (2)].type_stack));
			  push_type_int ((yyvsp[(2) - (2)].lval));
			  push_type (tp_array);
			  (yyval.type_stack) = get_type_stack ();
			}
    break;

  case 138:
/* Line 1787 of yacc.c  */
#line 1145 "c-exp.y"
    {
			  push_type_int ((yyvsp[(1) - (1)].lval));
			  push_type (tp_array);
			  (yyval.type_stack) = get_type_stack ();
			}
    break;

  case 139:
/* Line 1787 of yacc.c  */
#line 1152 "c-exp.y"
    {
			  push_type_stack ((yyvsp[(1) - (2)].type_stack));
			  push_typelist ((yyvsp[(2) - (2)].tvec));
			  (yyval.type_stack) = get_type_stack ();
			}
    break;

  case 140:
/* Line 1787 of yacc.c  */
#line 1158 "c-exp.y"
    {
			  push_typelist ((yyvsp[(1) - (1)].tvec));
			  (yyval.type_stack) = get_type_stack ();
			}
    break;

  case 141:
/* Line 1787 of yacc.c  */
#line 1165 "c-exp.y"
    { (yyval.lval) = -1; }
    break;

  case 142:
/* Line 1787 of yacc.c  */
#line 1167 "c-exp.y"
    { (yyval.lval) = -1; }
    break;

  case 143:
/* Line 1787 of yacc.c  */
#line 1169 "c-exp.y"
    { (yyval.lval) = (yyvsp[(2) - (3)].typed_val_int).val; }
    break;

  case 144:
/* Line 1787 of yacc.c  */
#line 1171 "c-exp.y"
    { (yyval.lval) = (yyvsp[(2) - (3)].typed_val_int).val; }
    break;

  case 145:
/* Line 1787 of yacc.c  */
#line 1175 "c-exp.y"
    { (yyval.tvec) = NULL; }
    break;

  case 146:
/* Line 1787 of yacc.c  */
#line 1177 "c-exp.y"
    { (yyval.tvec) = (yyvsp[(2) - (3)].tvec); }
    break;

  case 148:
/* Line 1787 of yacc.c  */
#line 1193 "c-exp.y"
    { (yyval.tval) = (yyvsp[(1) - (1)].tsym).type; }
    break;

  case 149:
/* Line 1787 of yacc.c  */
#line 1195 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "int"); }
    break;

  case 150:
/* Line 1787 of yacc.c  */
#line 1199 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 151:
/* Line 1787 of yacc.c  */
#line 1203 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "short"); }
    break;

  case 152:
/* Line 1787 of yacc.c  */
#line 1207 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 153:
/* Line 1787 of yacc.c  */
#line 1211 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 154:
/* Line 1787 of yacc.c  */
#line 1215 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 155:
/* Line 1787 of yacc.c  */
#line 1219 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 156:
/* Line 1787 of yacc.c  */
#line 1223 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long"); }
    break;

  case 157:
/* Line 1787 of yacc.c  */
#line 1227 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long"); }
    break;

  case 158:
/* Line 1787 of yacc.c  */
#line 1231 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long"); }
    break;

  case 159:
/* Line 1787 of yacc.c  */
#line 1235 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 160:
/* Line 1787 of yacc.c  */
#line 1239 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 161:
/* Line 1787 of yacc.c  */
#line 1243 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 162:
/* Line 1787 of yacc.c  */
#line 1247 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 163:
/* Line 1787 of yacc.c  */
#line 1251 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 164:
/* Line 1787 of yacc.c  */
#line 1255 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 165:
/* Line 1787 of yacc.c  */
#line 1259 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long long"); }
    break;

  case 166:
/* Line 1787 of yacc.c  */
#line 1263 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long long"); }
    break;

  case 167:
/* Line 1787 of yacc.c  */
#line 1267 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long long"); }
    break;

  case 168:
/* Line 1787 of yacc.c  */
#line 1271 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long long"); }
    break;

  case 169:
/* Line 1787 of yacc.c  */
#line 1275 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "short"); }
    break;

  case 170:
/* Line 1787 of yacc.c  */
#line 1279 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "short"); }
    break;

  case 171:
/* Line 1787 of yacc.c  */
#line 1283 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "short"); }
    break;

  case 172:
/* Line 1787 of yacc.c  */
#line 1287 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "short"); }
    break;

  case 173:
/* Line 1787 of yacc.c  */
#line 1291 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "short"); }
    break;

  case 174:
/* Line 1787 of yacc.c  */
#line 1295 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "short"); }
    break;

  case 175:
/* Line 1787 of yacc.c  */
#line 1299 "c-exp.y"
    { (yyval.tval) = lookup_typename (parse_language, parse_gdbarch,
						"double", (struct block *) NULL,
						0); }
    break;

  case 176:
/* Line 1787 of yacc.c  */
#line 1303 "c-exp.y"
    { (yyval.tval) = lookup_typename (parse_language, parse_gdbarch,
						"long double",
						(struct block *) NULL, 0); }
    break;

  case 177:
/* Line 1787 of yacc.c  */
#line 1307 "c-exp.y"
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[(2) - (2)].sval)),
					      expression_context_block); }
    break;

  case 178:
/* Line 1787 of yacc.c  */
#line 1310 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, "", 0);
			  (yyval.tval) = NULL;
			}
    break;

  case 179:
/* Line 1787 of yacc.c  */
#line 1315 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, (yyvsp[(2) - (3)].sval).ptr,
					       (yyvsp[(2) - (3)].sval).length);
			  (yyval.tval) = NULL;
			}
    break;

  case 180:
/* Line 1787 of yacc.c  */
#line 1321 "c-exp.y"
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[(2) - (2)].sval)),
					      expression_context_block); }
    break;

  case 181:
/* Line 1787 of yacc.c  */
#line 1324 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_CLASS, "", 0);
			  (yyval.tval) = NULL;
			}
    break;

  case 182:
/* Line 1787 of yacc.c  */
#line 1329 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_CLASS, (yyvsp[(2) - (3)].sval).ptr,
					       (yyvsp[(2) - (3)].sval).length);
			  (yyval.tval) = NULL;
			}
    break;

  case 183:
/* Line 1787 of yacc.c  */
#line 1335 "c-exp.y"
    { (yyval.tval) = lookup_union (copy_name ((yyvsp[(2) - (2)].sval)),
					     expression_context_block); }
    break;

  case 184:
/* Line 1787 of yacc.c  */
#line 1338 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_UNION, "", 0);
			  (yyval.tval) = NULL;
			}
    break;

  case 185:
/* Line 1787 of yacc.c  */
#line 1343 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_UNION, (yyvsp[(2) - (3)].sval).ptr,
					       (yyvsp[(2) - (3)].sval).length);
			  (yyval.tval) = NULL;
			}
    break;

  case 186:
/* Line 1787 of yacc.c  */
#line 1349 "c-exp.y"
    { (yyval.tval) = lookup_enum (copy_name ((yyvsp[(2) - (2)].sval)),
					    expression_context_block); }
    break;

  case 187:
/* Line 1787 of yacc.c  */
#line 1352 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_ENUM, "", 0);
			  (yyval.tval) = NULL;
			}
    break;

  case 188:
/* Line 1787 of yacc.c  */
#line 1357 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_ENUM, (yyvsp[(2) - (3)].sval).ptr,
					       (yyvsp[(2) - (3)].sval).length);
			  (yyval.tval) = NULL;
			}
    break;

  case 189:
/* Line 1787 of yacc.c  */
#line 1363 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 TYPE_NAME((yyvsp[(2) - (2)].tsym).type)); }
    break;

  case 190:
/* Line 1787 of yacc.c  */
#line 1367 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "int"); }
    break;

  case 191:
/* Line 1787 of yacc.c  */
#line 1371 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       TYPE_NAME((yyvsp[(2) - (2)].tsym).type)); }
    break;

  case 192:
/* Line 1787 of yacc.c  */
#line 1375 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "int"); }
    break;

  case 193:
/* Line 1787 of yacc.c  */
#line 1382 "c-exp.y"
    { (yyval.tval) = lookup_template_type(copy_name((yyvsp[(2) - (5)].sval)), (yyvsp[(4) - (5)].tval),
						    expression_context_block);
			}
    break;

  case 194:
/* Line 1787 of yacc.c  */
#line 1386 "c-exp.y"
    { (yyval.tval) = follow_types ((yyvsp[(2) - (2)].tval)); }
    break;

  case 195:
/* Line 1787 of yacc.c  */
#line 1388 "c-exp.y"
    { (yyval.tval) = follow_types ((yyvsp[(1) - (2)].tval)); }
    break;

  case 197:
/* Line 1787 of yacc.c  */
#line 1393 "c-exp.y"
    {
		  (yyval.tsym).stoken.ptr = "int";
		  (yyval.tsym).stoken.length = 3;
		  (yyval.tsym).type = lookup_signed_typename (parse_language,
						    parse_gdbarch,
						    "int");
		}
    break;

  case 198:
/* Line 1787 of yacc.c  */
#line 1401 "c-exp.y"
    {
		  (yyval.tsym).stoken.ptr = "long";
		  (yyval.tsym).stoken.length = 4;
		  (yyval.tsym).type = lookup_signed_typename (parse_language,
						    parse_gdbarch,
						    "long");
		}
    break;

  case 199:
/* Line 1787 of yacc.c  */
#line 1409 "c-exp.y"
    {
		  (yyval.tsym).stoken.ptr = "short";
		  (yyval.tsym).stoken.length = 5;
		  (yyval.tsym).type = lookup_signed_typename (parse_language,
						    parse_gdbarch,
						    "short");
		}
    break;

  case 200:
/* Line 1787 of yacc.c  */
#line 1420 "c-exp.y"
    { check_parameter_typelist ((yyvsp[(1) - (1)].tvec)); }
    break;

  case 201:
/* Line 1787 of yacc.c  */
#line 1422 "c-exp.y"
    {
			  VEC_safe_push (type_ptr, (yyvsp[(1) - (3)].tvec), NULL);
			  check_parameter_typelist ((yyvsp[(1) - (3)].tvec));
			  (yyval.tvec) = (yyvsp[(1) - (3)].tvec);
			}
    break;

  case 202:
/* Line 1787 of yacc.c  */
#line 1431 "c-exp.y"
    {
		  VEC (type_ptr) *typelist = NULL;
		  VEC_safe_push (type_ptr, typelist, (yyvsp[(1) - (1)].tval));
		  (yyval.tvec) = typelist;
		}
    break;

  case 203:
/* Line 1787 of yacc.c  */
#line 1437 "c-exp.y"
    {
		  VEC_safe_push (type_ptr, (yyvsp[(1) - (3)].tvec), (yyvsp[(3) - (3)].tval));
		  (yyval.tvec) = (yyvsp[(1) - (3)].tvec);
		}
    break;

  case 205:
/* Line 1787 of yacc.c  */
#line 1445 "c-exp.y"
    {
		  push_type_stack ((yyvsp[(2) - (2)].type_stack));
		  (yyval.tval) = follow_types ((yyvsp[(1) - (2)].tval));
		}
    break;

  case 206:
/* Line 1787 of yacc.c  */
#line 1452 "c-exp.y"
    { (yyval.tval) = follow_types ((yyvsp[(1) - (2)].tval)); }
    break;

  case 211:
/* Line 1787 of yacc.c  */
#line 1464 "c-exp.y"
    { insert_type (tp_const);
			  insert_type (tp_volatile); 
			}
    break;

  case 212:
/* Line 1787 of yacc.c  */
#line 1468 "c-exp.y"
    { insert_type (tp_const); }
    break;

  case 213:
/* Line 1787 of yacc.c  */
#line 1470 "c-exp.y"
    { insert_type (tp_volatile); }
    break;

  case 214:
/* Line 1787 of yacc.c  */
#line 1474 "c-exp.y"
    { (yyval.sval) = operator_stoken (" new"); }
    break;

  case 215:
/* Line 1787 of yacc.c  */
#line 1476 "c-exp.y"
    { (yyval.sval) = operator_stoken (" delete"); }
    break;

  case 216:
/* Line 1787 of yacc.c  */
#line 1478 "c-exp.y"
    { (yyval.sval) = operator_stoken (" new[]"); }
    break;

  case 217:
/* Line 1787 of yacc.c  */
#line 1480 "c-exp.y"
    { (yyval.sval) = operator_stoken (" delete[]"); }
    break;

  case 218:
/* Line 1787 of yacc.c  */
#line 1482 "c-exp.y"
    { (yyval.sval) = operator_stoken (" new[]"); }
    break;

  case 219:
/* Line 1787 of yacc.c  */
#line 1484 "c-exp.y"
    { (yyval.sval) = operator_stoken (" delete[]"); }
    break;

  case 220:
/* Line 1787 of yacc.c  */
#line 1486 "c-exp.y"
    { (yyval.sval) = operator_stoken ("+"); }
    break;

  case 221:
/* Line 1787 of yacc.c  */
#line 1488 "c-exp.y"
    { (yyval.sval) = operator_stoken ("-"); }
    break;

  case 222:
/* Line 1787 of yacc.c  */
#line 1490 "c-exp.y"
    { (yyval.sval) = operator_stoken ("*"); }
    break;

  case 223:
/* Line 1787 of yacc.c  */
#line 1492 "c-exp.y"
    { (yyval.sval) = operator_stoken ("/"); }
    break;

  case 224:
/* Line 1787 of yacc.c  */
#line 1494 "c-exp.y"
    { (yyval.sval) = operator_stoken ("%"); }
    break;

  case 225:
/* Line 1787 of yacc.c  */
#line 1496 "c-exp.y"
    { (yyval.sval) = operator_stoken ("^"); }
    break;

  case 226:
/* Line 1787 of yacc.c  */
#line 1498 "c-exp.y"
    { (yyval.sval) = operator_stoken ("&"); }
    break;

  case 227:
/* Line 1787 of yacc.c  */
#line 1500 "c-exp.y"
    { (yyval.sval) = operator_stoken ("|"); }
    break;

  case 228:
/* Line 1787 of yacc.c  */
#line 1502 "c-exp.y"
    { (yyval.sval) = operator_stoken ("~"); }
    break;

  case 229:
/* Line 1787 of yacc.c  */
#line 1504 "c-exp.y"
    { (yyval.sval) = operator_stoken ("!"); }
    break;

  case 230:
/* Line 1787 of yacc.c  */
#line 1506 "c-exp.y"
    { (yyval.sval) = operator_stoken ("="); }
    break;

  case 231:
/* Line 1787 of yacc.c  */
#line 1508 "c-exp.y"
    { (yyval.sval) = operator_stoken ("<"); }
    break;

  case 232:
/* Line 1787 of yacc.c  */
#line 1510 "c-exp.y"
    { (yyval.sval) = operator_stoken (">"); }
    break;

  case 233:
/* Line 1787 of yacc.c  */
#line 1512 "c-exp.y"
    { const char *op = "unknown";
			  switch ((yyvsp[(2) - (2)].opcode))
			    {
			    case BINOP_RSH:
			      op = ">>=";
			      break;
			    case BINOP_LSH:
			      op = "<<=";
			      break;
			    case BINOP_ADD:
			      op = "+=";
			      break;
			    case BINOP_SUB:
			      op = "-=";
			      break;
			    case BINOP_MUL:
			      op = "*=";
			      break;
			    case BINOP_DIV:
			      op = "/=";
			      break;
			    case BINOP_REM:
			      op = "%=";
			      break;
			    case BINOP_BITWISE_IOR:
			      op = "|=";
			      break;
			    case BINOP_BITWISE_AND:
			      op = "&=";
			      break;
			    case BINOP_BITWISE_XOR:
			      op = "^=";
			      break;
			    default:
			      break;
			    }

			  (yyval.sval) = operator_stoken (op);
			}
    break;

  case 234:
/* Line 1787 of yacc.c  */
#line 1552 "c-exp.y"
    { (yyval.sval) = operator_stoken ("<<"); }
    break;

  case 235:
/* Line 1787 of yacc.c  */
#line 1554 "c-exp.y"
    { (yyval.sval) = operator_stoken (">>"); }
    break;

  case 236:
/* Line 1787 of yacc.c  */
#line 1556 "c-exp.y"
    { (yyval.sval) = operator_stoken ("=="); }
    break;

  case 237:
/* Line 1787 of yacc.c  */
#line 1558 "c-exp.y"
    { (yyval.sval) = operator_stoken ("!="); }
    break;

  case 238:
/* Line 1787 of yacc.c  */
#line 1560 "c-exp.y"
    { (yyval.sval) = operator_stoken ("<="); }
    break;

  case 239:
/* Line 1787 of yacc.c  */
#line 1562 "c-exp.y"
    { (yyval.sval) = operator_stoken (">="); }
    break;

  case 240:
/* Line 1787 of yacc.c  */
#line 1564 "c-exp.y"
    { (yyval.sval) = operator_stoken ("&&"); }
    break;

  case 241:
/* Line 1787 of yacc.c  */
#line 1566 "c-exp.y"
    { (yyval.sval) = operator_stoken ("||"); }
    break;

  case 242:
/* Line 1787 of yacc.c  */
#line 1568 "c-exp.y"
    { (yyval.sval) = operator_stoken ("++"); }
    break;

  case 243:
/* Line 1787 of yacc.c  */
#line 1570 "c-exp.y"
    { (yyval.sval) = operator_stoken ("--"); }
    break;

  case 244:
/* Line 1787 of yacc.c  */
#line 1572 "c-exp.y"
    { (yyval.sval) = operator_stoken (","); }
    break;

  case 245:
/* Line 1787 of yacc.c  */
#line 1574 "c-exp.y"
    { (yyval.sval) = operator_stoken ("->*"); }
    break;

  case 246:
/* Line 1787 of yacc.c  */
#line 1576 "c-exp.y"
    { (yyval.sval) = operator_stoken ("->"); }
    break;

  case 247:
/* Line 1787 of yacc.c  */
#line 1578 "c-exp.y"
    { (yyval.sval) = operator_stoken ("()"); }
    break;

  case 248:
/* Line 1787 of yacc.c  */
#line 1580 "c-exp.y"
    { (yyval.sval) = operator_stoken ("[]"); }
    break;

  case 249:
/* Line 1787 of yacc.c  */
#line 1582 "c-exp.y"
    { (yyval.sval) = operator_stoken ("[]"); }
    break;

  case 250:
/* Line 1787 of yacc.c  */
#line 1584 "c-exp.y"
    { char *name;
			  long length;
			  struct ui_file *buf = mem_fileopen ();

			  c_print_type ((yyvsp[(2) - (2)].tval), NULL, buf, -1, 0,
					&type_print_raw_options);
			  name = ui_file_xstrdup (buf, &length);
			  ui_file_delete (buf);
			  (yyval.sval) = operator_stoken (name);
			  xfree (name);
			}
    break;

  case 251:
/* Line 1787 of yacc.c  */
#line 1599 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ssym).stoken; }
    break;

  case 252:
/* Line 1787 of yacc.c  */
#line 1600 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ssym).stoken; }
    break;

  case 253:
/* Line 1787 of yacc.c  */
#line 1601 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].tsym).stoken; }
    break;

  case 254:
/* Line 1787 of yacc.c  */
#line 1602 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ssym).stoken; }
    break;

  case 255:
/* Line 1787 of yacc.c  */
#line 1603 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ssym).stoken; }
    break;

  case 256:
/* Line 1787 of yacc.c  */
#line 1604 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].sval); }
    break;

  case 259:
/* Line 1787 of yacc.c  */
#line 1617 "c-exp.y"
    {
			  struct field_of_this_result is_a_field_of_this;

			  (yyval.ssym).stoken = (yyvsp[(1) - (1)].sval);
			  (yyval.ssym).sym = lookup_symbol ((yyvsp[(1) - (1)].sval).ptr,
						  expression_context_block,
						  VAR_DOMAIN,
						  &is_a_field_of_this);
			  (yyval.ssym).is_a_field_of_this
			    = is_a_field_of_this.type != NULL;
			}
    break;


/* Line 1787 of yacc.c  */
#line 4239 "c-exp.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
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
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
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


/* Line 2050 of yacc.c  */
#line 1631 "c-exp.y"


/* Like write_exp_string, but prepends a '~'.  */

static void
write_destructor_name (struct stoken token)
{
  char *copy = alloca (token.length + 1);

  copy[0] = '~';
  memcpy (&copy[1], token.ptr, token.length);

  token.ptr = copy;
  ++token.length;

  write_exp_string (token);
}

/* Returns a stoken of the operator name given by OP (which does not
   include the string "operator").  */ 
static struct stoken
operator_stoken (const char *op)
{
  static const char *operator_string = "operator";
  struct stoken st = { NULL, 0 };
  char *buf;

  st.length = strlen (operator_string) + strlen (op);
  buf = xmalloc (st.length + 1);
  strcpy (buf, operator_string);
  strcat (buf, op);
  st.ptr = buf;

  /* The toplevel (c_parse) will free the memory allocated here.  */
  make_cleanup (xfree, buf);
  return st;
};

/* Validate a parameter typelist.  */

static void
check_parameter_typelist (VEC (type_ptr) *params)
{
  struct type *type;
  int ix;

  for (ix = 0; VEC_iterate (type_ptr, params, ix, type); ++ix)
    {
      if (type != NULL && TYPE_CODE (check_typedef (type)) == TYPE_CODE_VOID)
	{
	  if (ix == 0)
	    {
	      if (VEC_length (type_ptr, params) == 1)
		{
		  /* Ok.  */
		  break;
		}
	      VEC_free (type_ptr, params);
	      error (_("parameter types following 'void'"));
	    }
	  else
	    {
	      VEC_free (type_ptr, params);
	      error (_("'void' invalid as parameter type"));
	    }
	}
    }
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (const char *buf, int len, int parsed_float, YYSTYPE *putithere)
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
  char *p;

  p = alloca (len);
  memcpy (p, buf, len);

  if (parsed_float)
    {
      /* If it ends at "df", "dd" or "dl", take it as type of decimal floating
         point.  Return DECFLOAT.  */

      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'f')
	{
	  p[len - 2] = '\0';
	  putithere->typed_val_decfloat.type
	    = parse_type->builtin_decfloat;
	  decimal_from_string (putithere->typed_val_decfloat.val, 4,
			       gdbarch_byte_order (parse_gdbarch), p);
	  p[len - 2] = 'd';
	  return DECFLOAT;
	}

      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'd')
	{
	  p[len - 2] = '\0';
	  putithere->typed_val_decfloat.type
	    = parse_type->builtin_decdouble;
	  decimal_from_string (putithere->typed_val_decfloat.val, 8,
			       gdbarch_byte_order (parse_gdbarch), p);
	  p[len - 2] = 'd';
	  return DECFLOAT;
	}

      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'l')
	{
	  p[len - 2] = '\0';
	  putithere->typed_val_decfloat.type
	    = parse_type->builtin_declong;
	  decimal_from_string (putithere->typed_val_decfloat.val, 16,
			       gdbarch_byte_order (parse_gdbarch), p);
	  p[len - 2] = 'd';
	  return DECFLOAT;
	}

      if (! parse_c_float (parse_gdbarch, p, len,
			   &putithere->typed_val_float.dval,
			   &putithere->typed_val_float.type))
	return ERROR;
      return FLOAT;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0 */
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
	return ERROR;		/* Invalid digit in this base */

      /* Portably test for overflow (only works for nonzero values, so make
	 a second check for zero).  FIXME: Can't we just make n and prevn
	 unsigned and avoid this?  */
      if (c != 'l' && c != 'u' && (prevn >= n) && n != 0)
	unsigned_p = 1;		/* Try something unsigned */

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
      has to be unsigned. */

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

/* Parse a C escape sequence.  The initial backslash of the sequence
   is at (*PTR)[-1].  *PTR will be updated to point to just after the
   last character of the sequence.  If OUTPUT is not NULL, the
   translated form of the escape sequence will be written there.  If
   OUTPUT is NULL, no output is written and the call will only affect
   *PTR.  If an escape sequence is expressed in target bytes, then the
   entire sequence will simply be copied to OUTPUT.  Return 1 if any
   character was emitted, 0 otherwise.  */

int
c_parse_escape (const char **ptr, struct obstack *output)
{
  const char *tokptr = *ptr;
  int result = 1;

  /* Some escape sequences undergo character set conversion.  Those we
     translate here.  */
  switch (*tokptr)
    {
      /* Hex escapes do not undergo character set conversion, so keep
	 the escape sequence for later.  */
    case 'x':
      if (output)
	obstack_grow_str (output, "\\x");
      ++tokptr;
      if (!isxdigit (*tokptr))
	error (_("\\x escape without a following hex digit"));
      while (isxdigit (*tokptr))
	{
	  if (output)
	    obstack_1grow (output, *tokptr);
	  ++tokptr;
	}
      break;

      /* Octal escapes do not undergo character set conversion, so
	 keep the escape sequence for later.  */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      {
	int i;
	if (output)
	  obstack_grow_str (output, "\\");
	for (i = 0;
	     i < 3 && isdigit (*tokptr) && *tokptr != '8' && *tokptr != '9';
	     ++i)
	  {
	    if (output)
	      obstack_1grow (output, *tokptr);
	    ++tokptr;
	  }
      }
      break;

      /* We handle UCNs later.  We could handle them here, but that
	 would mean a spurious error in the case where the UCN could
	 be converted to the target charset but not the host
	 charset.  */
    case 'u':
    case 'U':
      {
	char c = *tokptr;
	int i, len = c == 'U' ? 8 : 4;
	if (output)
	  {
	    obstack_1grow (output, '\\');
	    obstack_1grow (output, *tokptr);
	  }
	++tokptr;
	if (!isxdigit (*tokptr))
	  error (_("\\%c escape without a following hex digit"), c);
	for (i = 0; i < len && isxdigit (*tokptr); ++i)
	  {
	    if (output)
	      obstack_1grow (output, *tokptr);
	    ++tokptr;
	  }
      }
      break;

      /* We must pass backslash through so that it does not
	 cause quoting during the second expansion.  */
    case '\\':
      if (output)
	obstack_grow_str (output, "\\\\");
      ++tokptr;
      break;

      /* Escapes which undergo conversion.  */
    case 'a':
      if (output)
	obstack_1grow (output, '\a');
      ++tokptr;
      break;
    case 'b':
      if (output)
	obstack_1grow (output, '\b');
      ++tokptr;
      break;
    case 'f':
      if (output)
	obstack_1grow (output, '\f');
      ++tokptr;
      break;
    case 'n':
      if (output)
	obstack_1grow (output, '\n');
      ++tokptr;
      break;
    case 'r':
      if (output)
	obstack_1grow (output, '\r');
      ++tokptr;
      break;
    case 't':
      if (output)
	obstack_1grow (output, '\t');
      ++tokptr;
      break;
    case 'v':
      if (output)
	obstack_1grow (output, '\v');
      ++tokptr;
      break;

      /* GCC extension.  */
    case 'e':
      if (output)
	obstack_1grow (output, HOST_ESCAPE_CHAR);
      ++tokptr;
      break;

      /* Backslash-newline expands to nothing at all.  */
    case '\n':
      ++tokptr;
      result = 0;
      break;

      /* A few escapes just expand to the character itself.  */
    case '\'':
    case '\"':
    case '?':
      /* GCC extensions.  */
    case '(':
    case '{':
    case '[':
    case '%':
      /* Unrecognized escapes turn into the character itself.  */
    default:
      if (output)
	obstack_1grow (output, *tokptr);
      ++tokptr;
      break;
    }
  *ptr = tokptr;
  return result;
}

/* Parse a string or character literal from TOKPTR.  The string or
   character may be wide or unicode.  *OUTPTR is set to just after the
   end of the literal in the input string.  The resulting token is
   stored in VALUE.  This returns a token value, either STRING or
   CHAR, depending on what was parsed.  *HOST_CHARS is set to the
   number of host characters in the literal.  */
static int
parse_string_or_char (const char *tokptr, const char **outptr,
		      struct typed_stoken *value, int *host_chars)
{
  int quote;
  enum c_string_type type;
  int is_objc = 0;

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

  /* Record the string type.  */
  if (*tokptr == 'L')
    {
      type = C_WIDE_STRING;
      ++tokptr;
    }
  else if (*tokptr == 'u')
    {
      type = C_STRING_16;
      ++tokptr;
    }
  else if (*tokptr == 'U')
    {
      type = C_STRING_32;
      ++tokptr;
    }
  else if (*tokptr == '@')
    {
      /* An Objective C string.  */
      is_objc = 1;
      type = C_STRING;
      ++tokptr;
    }
  else
    type = C_STRING;

  /* Skip the quote.  */
  quote = *tokptr;
  if (quote == '\'')
    type |= C_CHAR;
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

  value->type = type;
  value->ptr = obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '"' ? (is_objc ? NSSTRING : STRING) : CHAR;
}

/* This is used to associate some attributes with a token.  */

enum token_flags
{
  /* If this bit is set, the token is C++-only.  */

  FLAG_CXX = 1,

  /* If this bit is set, the token is conditional: if there is a
     symbol of the same name, then the token is a symbol; otherwise,
     the token is a keyword.  */

  FLAG_SHADOW = 2
};

struct token
{
  char *operator;
  int token;
  enum exp_opcode opcode;
  enum token_flags flags;
};

static const struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH, 0},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH, 0},
    {"->*", ARROW_STAR, BINOP_END, FLAG_CXX},
    {"...", DOTDOTDOT, BINOP_END, 0}
  };

static const struct token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD, 0},
    {"-=", ASSIGN_MODIFY, BINOP_SUB, 0},
    {"*=", ASSIGN_MODIFY, BINOP_MUL, 0},
    {"/=", ASSIGN_MODIFY, BINOP_DIV, 0},
    {"%=", ASSIGN_MODIFY, BINOP_REM, 0},
    {"|=", ASSIGN_MODIFY, BINOP_BITWISE_IOR, 0},
    {"&=", ASSIGN_MODIFY, BINOP_BITWISE_AND, 0},
    {"^=", ASSIGN_MODIFY, BINOP_BITWISE_XOR, 0},
    {"++", INCREMENT, BINOP_END, 0},
    {"--", DECREMENT, BINOP_END, 0},
    {"->", ARROW, BINOP_END, 0},
    {"&&", ANDAND, BINOP_END, 0},
    {"||", OROR, BINOP_END, 0},
    /* "::" is *not* only C++: gdb overrides its meaning in several
       different ways, e.g., 'filename'::func, function::variable.  */
    {"::", COLONCOLON, BINOP_END, 0},
    {"<<", LSH, BINOP_END, 0},
    {">>", RSH, BINOP_END, 0},
    {"==", EQUAL, BINOP_END, 0},
    {"!=", NOTEQUAL, BINOP_END, 0},
    {"<=", LEQ, BINOP_END, 0},
    {">=", GEQ, BINOP_END, 0},
    {".*", DOT_STAR, BINOP_END, FLAG_CXX}
  };

/* Identifier-like tokens.  */
static const struct token ident_tokens[] =
  {
    {"unsigned", UNSIGNED, OP_NULL, 0},
    {"template", TEMPLATE, OP_NULL, FLAG_CXX},
    {"volatile", VOLATILE_KEYWORD, OP_NULL, 0},
    {"struct", STRUCT, OP_NULL, 0},
    {"signed", SIGNED_KEYWORD, OP_NULL, 0},
    {"sizeof", SIZEOF, OP_NULL, 0},
    {"double", DOUBLE_KEYWORD, OP_NULL, 0},
    {"false", FALSEKEYWORD, OP_NULL, FLAG_CXX},
    {"class", CLASS, OP_NULL, FLAG_CXX},
    {"union", UNION, OP_NULL, 0},
    {"short", SHORT, OP_NULL, 0},
    {"const", CONST_KEYWORD, OP_NULL, 0},
    {"enum", ENUM, OP_NULL, 0},
    {"long", LONG, OP_NULL, 0},
    {"true", TRUEKEYWORD, OP_NULL, FLAG_CXX},
    {"int", INT_KEYWORD, OP_NULL, 0},
    {"new", NEW, OP_NULL, FLAG_CXX},
    {"delete", DELETE, OP_NULL, FLAG_CXX},
    {"operator", OPERATOR, OP_NULL, FLAG_CXX},

    {"and", ANDAND, BINOP_END, FLAG_CXX},
    {"and_eq", ASSIGN_MODIFY, BINOP_BITWISE_AND, FLAG_CXX},
    {"bitand", '&', OP_NULL, FLAG_CXX},
    {"bitor", '|', OP_NULL, FLAG_CXX},
    {"compl", '~', OP_NULL, FLAG_CXX},
    {"not", '!', OP_NULL, FLAG_CXX},
    {"not_eq", NOTEQUAL, BINOP_END, FLAG_CXX},
    {"or", OROR, BINOP_END, FLAG_CXX},
    {"or_eq", ASSIGN_MODIFY, BINOP_BITWISE_IOR, FLAG_CXX},
    {"xor", '^', OP_NULL, FLAG_CXX},
    {"xor_eq", ASSIGN_MODIFY, BINOP_BITWISE_XOR, FLAG_CXX},

    {"const_cast", CONST_CAST, OP_NULL, FLAG_CXX },
    {"dynamic_cast", DYNAMIC_CAST, OP_NULL, FLAG_CXX },
    {"static_cast", STATIC_CAST, OP_NULL, FLAG_CXX },
    {"reinterpret_cast", REINTERPRET_CAST, OP_NULL, FLAG_CXX },

    {"__typeof__", TYPEOF, OP_TYPEOF, 0 },
    {"__typeof", TYPEOF, OP_TYPEOF, 0 },
    {"typeof", TYPEOF, OP_TYPEOF, FLAG_SHADOW },
    {"__decltype", DECLTYPE, OP_DECLTYPE, FLAG_CXX },
    {"decltype", DECLTYPE, OP_DECLTYPE, FLAG_CXX | FLAG_SHADOW },

    {"typeid", TYPEID, OP_TYPEID, FLAG_CXX}
  };

/* When we find that lexptr (the global var defined in parse.c) is
   pointing at a macro invocation, we expand the invocation, and call
   scan_macro_expansion to save the old lexptr here and point lexptr
   into the expanded text.  When we reach the end of that, we call
   end_macro_expansion to pop back to the value we saved here.  The
   macro expansion code promises to return only fully-expanded text,
   so we don't need to "push" more than one level.

   This is disgusting, of course.  It would be cleaner to do all macro
   expansion beforehand, and then hand that to lexptr.  But we don't
   really know where the expression ends.  Remember, in a command like

     (gdb) break *ADDRESS if CONDITION

   we evaluate ADDRESS in the scope of the current frame, but we
   evaluate CONDITION in the scope of the breakpoint's location.  So
   it's simply wrong to try to macro-expand the whole thing at once.  */
static const char *macro_original_text;

/* We save all intermediate macro expansions on this obstack for the
   duration of a single parse.  The expansion text may sometimes have
   to live past the end of the expansion, due to yacc lookahead.
   Rather than try to be clever about saving the data for a single
   token, we simply keep it all and delete it after parsing has
   completed.  */
static struct obstack expansion_obstack;

static void
scan_macro_expansion (char *expansion)
{
  char *copy;

  /* We'd better not be trying to push the stack twice.  */
  gdb_assert (! macro_original_text);

  /* Copy to the obstack, and then free the intermediate
     expansion.  */
  copy = obstack_copy0 (&expansion_obstack, expansion, strlen (expansion));
  xfree (expansion);

  /* Save the old lexptr value, so we can return to it when we're done
     parsing the expanded text.  */
  macro_original_text = lexptr;
  lexptr = copy;
}


static int
scanning_macro_expansion (void)
{
  return macro_original_text != 0;
}


static void 
finished_macro_expansion (void)
{
  /* There'd better be something to pop back to.  */
  gdb_assert (macro_original_text);

  /* Pop back to the original text.  */
  lexptr = macro_original_text;
  macro_original_text = 0;
}


static void
scan_macro_cleanup (void *dummy)
{
  if (macro_original_text)
    finished_macro_expansion ();

  obstack_free (&expansion_obstack, NULL);
}

/* Return true iff the token represents a C++ cast operator.  */

static int
is_cast_operator (const char *token, int len)
{
  return (! strncmp (token, "dynamic_cast", len)
	  || ! strncmp (token, "static_cast", len)
	  || ! strncmp (token, "reinterpret_cast", len)
	  || ! strncmp (token, "const_cast", len));
}

/* The scope used for macro expansion.  */
static struct macro_scope *expression_macro_scope;

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
lex_one_token (int *is_quoted_name)
{
  int c;
  int namelen;
  unsigned int i;
  const char *tokstart;
  int saw_structop = last_was_structop;
  char *copy;

  last_was_structop = 0;
  *is_quoted_name = 0;

 retry:

  /* Check if this is a macro invocation that we need to expand.  */
  if (! scanning_macro_expansion ())
    {
      char *expanded = macro_expand_next (&lexptr,
                                          standard_macro_lookup,
                                          expression_macro_scope);

      if (expanded)
        scan_macro_expansion (expanded);
    }

  prev_lexptr = lexptr;

  tokstart = lexptr;
  /* See if it is a special token of length 3.  */
  for (i = 0; i < sizeof tokentab3 / sizeof tokentab3[0]; i++)
    if (strncmp (tokstart, tokentab3[i].operator, 3) == 0)
      {
	if ((tokentab3[i].flags & FLAG_CXX) != 0
	    && parse_language->la_language != language_cplus)
	  break;

	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof tokentab2 / sizeof tokentab2[0]; i++)
    if (strncmp (tokstart, tokentab2[i].operator, 2) == 0)
      {
	if ((tokentab2[i].flags & FLAG_CXX) != 0
	    && parse_language->la_language != language_cplus)
	  break;

	lexptr += 2;
	yylval.opcode = tokentab2[i].opcode;
	if (parse_completion && tokentab2[i].token == ARROW)
	  last_was_structop = 1;
	return tokentab2[i].token;
      }

  switch (c = *tokstart)
    {
    case 0:
      /* If we were just scanning the result of a macro expansion,
         then we need to resume scanning the original text.
	 If we're parsing for field name completion, and the previous
	 token allows such completion, return a COMPLETE token.
         Otherwise, we were already scanning the original text, and
         we're really done.  */
      if (scanning_macro_expansion ())
        {
          finished_macro_expansion ();
          goto retry;
        }
      else if (saw_name_at_eof)
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
      if (parse_language->la_language == language_objc && c == '[')
	return OBJC_LBRAC;
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
          && paren_depth == 0
          && ! scanning_macro_expansion ())
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
	const char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }
	else if (c == '0' && (p[1]=='t' || p[1]=='T' || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
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
	const char *p = &tokstart[1];
	size_t len = strlen ("entry");

	if (parse_language->la_language == language_objc)
	  {
	    size_t len = strlen ("selector");

	    if (strncmp (p, "selector", len) == 0
		&& (p[len] == '\0' || isspace (p[len])))
	      {
		lexptr = p + len;
		return SELECTOR;
	      }
	    else if (*p == '"')
	      goto parse_string;
	  }

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

    case 'L':
    case 'u':
    case 'U':
      if (tokstart[1] != '"' && tokstart[1] != '\'')
	break;
      /* Fall through.  */
    case '\'':
    case '"':

    parse_string:
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
		*is_quoted_name = 1;

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
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '<');)
    {
      /* Template parameter lists are part of the name.
	 FIXME: This mishandles `print $a<4&&$a>3'.  */

      if (c == '<')
	{
	  if (! is_cast_operator (tokstart, namelen))
	    {
	      /* Scan ahead to get rest of the template specification.  Note
		 that we look ahead only when the '<' adjoins non-whitespace
		 characters; for comparison expressions, e.g. "a < b > c",
		 there must be spaces before the '<', etc. */
               
	      const char *p = find_template_name_end (tokstart + namelen);

	      if (p)
		namelen = p - tokstart;
	    }
	  break;
	}
      c = tokstart[++namelen];
    }

  /* The token "if" terminates the expression and is NOT removed from
     the input stream.  It doesn't count if it appears in the
     expansion of a macro.  */
  if (namelen == 2
      && tokstart[0] == 'i'
      && tokstart[1] == 'f'
      && ! scanning_macro_expansion ())
    {
      return 0;
    }

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.  "task" is similar.  Handle abbreviations of these,
     similarly to breakpoint.c:find_condition_and_thread.  */
  if (namelen >= 1
      && (strncmp (tokstart, "thread", namelen) == 0
	  || strncmp (tokstart, "task", namelen) == 0)
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t')
      && ! scanning_macro_expansion ())
    {
      const char *p = tokstart + namelen + 1;

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
  for (i = 0; i < sizeof ident_tokens / sizeof ident_tokens[0]; i++)
    if (strcmp (copy, ident_tokens[i].operator) == 0)
      {
	if ((ident_tokens[i].flags & FLAG_CXX) != 0
	    && parse_language->la_language != language_cplus)
	  break;

	if ((ident_tokens[i].flags & FLAG_SHADOW) != 0)
	  {
	    struct field_of_this_result is_a_field_of_this;

	    if (lookup_symbol (copy, expression_context_block,
			       VAR_DOMAIN,
			       (parse_language->la_language == language_cplus
				? &is_a_field_of_this
				: NULL))
		!= NULL)
	      {
		/* The keyword is shadowed.  */
		break;
	      }
	  }

	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = ident_tokens[i].opcode;
	return ident_tokens[i].token;
      }

  if (*tokstart == '$')
    return VARIABLE;

  if (parse_completion && *lexptr == '\0')
    saw_name_at_eof = 1;

  yylval.ssym.stoken = yylval.sval;
  yylval.ssym.sym = NULL;
  yylval.ssym.is_a_field_of_this = 0;
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

/* Temporary storage for c_lex; this holds symbol names as they are
   built up.  */
static struct obstack name_obstack;

/* Classify a NAME token.  The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.  BLOCK is the block
   in which lookups start; this can be NULL to mean the global scope.
   IS_QUOTED_NAME is non-zero if the name token was originally quoted
   in single quotes.  */
static int
classify_name (const struct block *block, int is_quoted_name)
{
  struct symbol *sym;
  char *copy;
  struct field_of_this_result is_a_field_of_this;

  copy = copy_name (yylval.sval);

  /* Initialize this in case we *don't* use it in this call; that way
     we can refer to it unconditionally below.  */
  memset (&is_a_field_of_this, 0, sizeof (is_a_field_of_this));

  sym = lookup_symbol (copy, block, VAR_DOMAIN, 
		       parse_language->la_name_of_this
		       ? &is_a_field_of_this : NULL);

  if (sym && SYMBOL_CLASS (sym) == LOC_BLOCK)
    {
      yylval.ssym.sym = sym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
      return BLOCKNAME;
    }
  else if (!sym)
    {
      /* If we found a field of 'this', we might have erroneously
	 found a constructor where we wanted a type name.  Handle this
	 case by noticing that we found a constructor and then look up
	 the type tag instead.  */
      if (is_a_field_of_this.type != NULL
	  && is_a_field_of_this.fn_field != NULL
	  && TYPE_FN_FIELD_CONSTRUCTOR (is_a_field_of_this.fn_field->fn_fields,
					0))
	{
	  struct field_of_this_result inner_is_a_field_of_this;

	  sym = lookup_symbol (copy, block, STRUCT_DOMAIN,
			       &inner_is_a_field_of_this);
	  if (sym != NULL)
	    {
	      yylval.tsym.type = SYMBOL_TYPE (sym);
	      return TYPENAME;
	    }
	}

      /* If we found a field, then we want to prefer it over a
	 filename.  However, if the name was quoted, then it is better
	 to check for a filename or a block, since this is the only
	 way the user has of requiring the extension to be used.  */
      if (is_a_field_of_this.type == NULL || is_quoted_name)
	{
	  /* See if it's a file name. */
	  struct symtab *symtab;

	  symtab = lookup_symtab (copy);
	  if (symtab)
	    {
	      yylval.bval = BLOCKVECTOR_BLOCK (BLOCKVECTOR (symtab),
					       STATIC_BLOCK);
	      return FILENAME;
	    }
	}
    }

  if (sym && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
    {
      yylval.tsym.type = SYMBOL_TYPE (sym);
      return TYPENAME;
    }

  yylval.tsym.type
    = language_lookup_primitive_type_by_name (parse_language,
					      parse_gdbarch, copy);
  if (yylval.tsym.type != NULL)
    return TYPENAME;

  /* See if it's an ObjC classname.  */
  if (parse_language->la_language == language_objc && !sym)
    {
      CORE_ADDR Class = lookup_objc_class (parse_gdbarch, copy);
      if (Class)
	{
	  yylval.class.class = Class;
	  sym = lookup_struct_typedef (copy, expression_context_block, 1);
	  if (sym)
	    yylval.class.type = SYMBOL_TYPE (sym);
	  return CLASSNAME;
	}
    }

  /* Input names that aren't symbols but ARE valid hex numbers, when
     the input radix permits them, can be names or numbers depending
     on the parse.  Note we support radixes > 16 here.  */
  if (!sym
      && ((copy[0] >= 'a' && copy[0] < 'a' + input_radix - 10)
	  || (copy[0] >= 'A' && copy[0] < 'A' + input_radix - 10)))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (copy, yylval.sval.length, 0, &newlval);
      if (hextype == INT)
	{
	  yylval.ssym.sym = sym;
	  yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	  return NAME_OR_INT;
	}
    }

  /* Any other kind of symbol */
  yylval.ssym.sym = sym;
  yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;

  if (sym == NULL
      && parse_language->la_language == language_cplus
      && is_a_field_of_this.type == NULL
      && !lookup_minimal_symbol (copy, NULL, NULL))
    return UNKNOWN_CPP_NAME;

  return NAME;
}

/* Like classify_name, but used by the inner loop of the lexer, when a
   name might have already been seen.  CONTEXT is the context type, or
   NULL if this is the first component of a name.  */

static int
classify_inner_name (const struct block *block, struct type *context)
{
  struct type *type;
  char *copy;

  if (context == NULL)
    return classify_name (block, 0);

  type = check_typedef (context);
  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
      && TYPE_CODE (type) != TYPE_CODE_UNION
      && TYPE_CODE (type) != TYPE_CODE_NAMESPACE)
    return ERROR;

  copy = copy_name (yylval.ssym.stoken);
  yylval.ssym.sym = cp_lookup_nested_symbol (type, copy, block);

  /* If no symbol was found, search for a matching base class named
     COPY.  This will allow users to enter qualified names of class members
     relative to the `this' pointer.  */
  if (yylval.ssym.sym == NULL)
    {
      struct type *base_type = find_type_baseclass_by_name (type, copy);

      if (base_type != NULL)
	{
	  yylval.tsym.type = base_type;
	  return TYPENAME;
	}

      return ERROR;
    }

  switch (SYMBOL_CLASS (yylval.ssym.sym))
    {
    case LOC_BLOCK:
    case LOC_LABEL:
      /* cp_lookup_nested_symbol might have accidentally found a constructor
	 named COPY when we really wanted a base class of the same name.
	 Double-check this case by looking for a base class.  */
      {
	struct type *base_type = find_type_baseclass_by_name (type, copy);

	if (base_type != NULL)
	  {
	    yylval.tsym.type = base_type;
	    return TYPENAME;
	  }
      }
      return ERROR;

    case LOC_TYPEDEF:
      yylval.tsym.type = SYMBOL_TYPE (yylval.ssym.sym);;
      return TYPENAME;

    default:
      return NAME;
    }
  internal_error (__FILE__, __LINE__, _("not reached"));
}

/* The outer level of a two-level lexer.  This calls the inner lexer
   to return tokens.  It then either returns these tokens, or
   aggregates them into a larger token.  This lets us work around a
   problem in our parsing approach, where the parser could not
   distinguish between qualified names and qualified types at the
   right point.
   
   This approach is still not ideal, because it mishandles template
   types.  See the comment in lex_one_token for an example.  However,
   this is still an improvement over the earlier approach, and will
   suffice until we move to better parsing technology.  */
static int
yylex (void)
{
  token_and_value current;
  int first_was_coloncolon, last_was_coloncolon;
  struct type *context_type = NULL;
  int last_to_examine, next_to_examine, checkpoint;
  const struct block *search_block;
  int is_quoted_name;

  if (popping && !VEC_empty (token_and_value, token_fifo))
    goto do_pop;
  popping = 0;

  /* Read the first token and decide what to do.  Most of the
     subsequent code is C++-only; but also depends on seeing a "::" or
     name-like token.  */
  current.token = lex_one_token (&is_quoted_name);
  if (current.token == NAME)
    current.token = classify_name (expression_context_block, is_quoted_name);
  if (parse_language->la_language != language_cplus
      || (current.token != TYPENAME && current.token != COLONCOLON
	  && current.token != FILENAME))
    return current.token;

  /* Read any sequence of alternating "::" and name-like tokens into
     the token FIFO.  */
  current.value = yylval;
  VEC_safe_push (token_and_value, token_fifo, &current);
  last_was_coloncolon = current.token == COLONCOLON;
  while (1)
    {
      int ignore;

      /* We ignore quoted names other than the very first one.
	 Subsequent ones do not have any special meaning.  */
      current.token = lex_one_token (&ignore);
      current.value = yylval;
      VEC_safe_push (token_and_value, token_fifo, &current);

      if ((last_was_coloncolon && current.token != NAME)
	  || (!last_was_coloncolon && current.token != COLONCOLON))
	break;
      last_was_coloncolon = !last_was_coloncolon;
    }
  popping = 1;

  /* We always read one extra token, so compute the number of tokens
     to examine accordingly.  */
  last_to_examine = VEC_length (token_and_value, token_fifo) - 2;
  next_to_examine = 0;

  current = *VEC_index (token_and_value, token_fifo, next_to_examine);
  ++next_to_examine;

  obstack_free (&name_obstack, obstack_base (&name_obstack));
  checkpoint = 0;
  if (current.token == FILENAME)
    search_block = current.value.bval;
  else if (current.token == COLONCOLON)
    search_block = NULL;
  else
    {
      gdb_assert (current.token == TYPENAME);
      search_block = expression_context_block;
      obstack_grow (&name_obstack, current.value.sval.ptr,
		    current.value.sval.length);
      context_type = current.value.tsym.type;
      checkpoint = 1;
    }

  first_was_coloncolon = current.token == COLONCOLON;
  last_was_coloncolon = first_was_coloncolon;

  while (next_to_examine <= last_to_examine)
    {
      token_and_value *next;

      next = VEC_index (token_and_value, token_fifo, next_to_examine);
      ++next_to_examine;

      if (next->token == NAME && last_was_coloncolon)
	{
	  int classification;

	  yylval = next->value;
	  classification = classify_inner_name (search_block, context_type);
	  /* We keep going until we either run out of names, or until
	     we have a qualified name which is not a type.  */
	  if (classification != TYPENAME && classification != NAME)
	    break;

	  /* Accept up to this token.  */
	  checkpoint = next_to_examine;

	  /* Update the partial name we are constructing.  */
	  if (context_type != NULL)
	    {
	      /* We don't want to put a leading "::" into the name.  */
	      obstack_grow_str (&name_obstack, "::");
	    }
	  obstack_grow (&name_obstack, next->value.sval.ptr,
			next->value.sval.length);

	  yylval.sval.ptr = obstack_base (&name_obstack);
	  yylval.sval.length = obstack_object_size (&name_obstack);
	  current.value = yylval;
	  current.token = classification;

	  last_was_coloncolon = 0;
	  
	  if (classification == NAME)
	    break;

	  context_type = yylval.tsym.type;
	}
      else if (next->token == COLONCOLON && !last_was_coloncolon)
	last_was_coloncolon = 1;
      else
	{
	  /* We've reached the end of the name.  */
	  break;
	}
    }

  /* If we have a replacement token, install it as the first token in
     the FIFO, and delete the other constituent tokens.  */
  if (checkpoint > 0)
    {
      current.value.sval.ptr = obstack_copy0 (&expansion_obstack,
					      current.value.sval.ptr,
					      current.value.sval.length);

      VEC_replace (token_and_value, token_fifo, 0, &current);
      if (checkpoint > 1)
	VEC_block_remove (token_and_value, token_fifo, 1, checkpoint - 1);
    }

 do_pop:
  current = *VEC_index (token_and_value, token_fifo, 0);
  VEC_ordered_remove (token_and_value, token_fifo, 0);
  yylval = current.value;
  return current.token;
}

int
c_parse (void)
{
  int result;
  struct cleanup *back_to = make_cleanup (free_current_contents,
					  &expression_macro_scope);

  /* Set up the scope for macro expansion.  */
  expression_macro_scope = NULL;

  if (expression_context_block)
    expression_macro_scope
      = sal_macro_scope (find_pc_line (expression_context_pc, 0));
  else
    expression_macro_scope = default_macro_scope ();
  if (! expression_macro_scope)
    expression_macro_scope = user_macro_scope ();

  /* Initialize macro expansion code.  */
  obstack_init (&expansion_obstack);
  gdb_assert (! macro_original_text);
  make_cleanup (scan_macro_cleanup, 0);

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

/* This is called via the YYPRINT macro when parser debugging is
   enabled.  It prints a token's value.  */

static void
c_print_token (FILE *file, int type, YYSTYPE value)
{
  switch (type)
    {
    case INT:
      fprintf (file, "typed_val_int<%s, %s>",
	       TYPE_SAFE_NAME (value.typed_val_int.type),
	       pulongest (value.typed_val_int.val));
      break;

    case CHAR:
    case STRING:
      {
	char *copy = alloca (value.tsval.length + 1);

	memcpy (copy, value.tsval.ptr, value.tsval.length);
	copy[value.tsval.length] = '\0';

	fprintf (file, "tsval<type=%d, %s>", value.tsval.type, copy);
      }
      break;

    case NSSTRING:
    case VARIABLE:
      fprintf (file, "sval<%s>", copy_name (value.sval));
      break;

    case TYPENAME:
      fprintf (file, "tsym<type=%s, name=%s>",
	       TYPE_SAFE_NAME (value.tsym.type),
	       copy_name (value.tsym.stoken));
      break;

    case NAME:
    case UNKNOWN_CPP_NAME:
    case NAME_OR_INT:
    case BLOCKNAME:
      fprintf (file, "ssym<name=%s, sym=%s, field_of_this=%d>",
	       copy_name (value.ssym.stoken),
	       (value.ssym.sym == NULL
		? "(null)" : SYMBOL_PRINT_NAME (value.ssym.sym)),
	       value.ssym.is_a_field_of_this);
      break;

    case FILENAME:
      fprintf (file, "bval<%s>", host_address_to_string (value.bval));
      break;
    }
}

void
yyerror (char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), (msg ? msg : "error"), lexptr);
}
