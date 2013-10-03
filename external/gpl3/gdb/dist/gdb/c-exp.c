
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
#line 36 "c-exp.y"


#include "defs.h"
#include "gdb_string.h"
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



/* Line 189 of yacc.c  */
#line 167 "c-exp.c"

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
     SIGNED_KEYWORD = 291,
     LONG = 292,
     SHORT = 293,
     INT_KEYWORD = 294,
     CONST_KEYWORD = 295,
     VOLATILE_KEYWORD = 296,
     DOUBLE_KEYWORD = 297,
     VARIABLE = 298,
     ASSIGN_MODIFY = 299,
     TRUEKEYWORD = 300,
     FALSEKEYWORD = 301,
     ABOVE_COMMA = 302,
     OROR = 303,
     ANDAND = 304,
     NOTEQUAL = 305,
     EQUAL = 306,
     GEQ = 307,
     LEQ = 308,
     RSH = 309,
     LSH = 310,
     DECREMENT = 311,
     INCREMENT = 312,
     UNARY = 313,
     DOT_STAR = 314,
     ARROW_STAR = 315,
     ARROW = 316,
     BLOCKNAME = 317,
     FILENAME = 318,
     DOTDOTDOT = 319
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
#define SIGNED_KEYWORD 291
#define LONG 292
#define SHORT 293
#define INT_KEYWORD 294
#define CONST_KEYWORD 295
#define VOLATILE_KEYWORD 296
#define DOUBLE_KEYWORD 297
#define VARIABLE 298
#define ASSIGN_MODIFY 299
#define TRUEKEYWORD 300
#define FALSEKEYWORD 301
#define ABOVE_COMMA 302
#define OROR 303
#define ANDAND 304
#define NOTEQUAL 305
#define EQUAL 306
#define GEQ 307
#define LEQ 308
#define RSH 309
#define LSH 310
#define DECREMENT 311
#define INCREMENT 312
#define UNARY 313
#define DOT_STAR 314
#define ARROW_STAR 315
#define ARROW 316
#define BLOCKNAME 317
#define FILENAME 318
#define DOTDOTDOT 319




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
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
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    struct typed_stoken tsval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct stoken_vector svec;
    VEC (type_ptr) *tvec;
    int *ivec;

    struct type_stack *type_stack;

    struct objc_class_str class;
  


/* Line 214 of yacc.c  */
#line 367 "c-exp.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */

/* Line 264 of yacc.c  */
#line 168 "c-exp.y"

/* YYSTYPE gets defined by %union */
static int parse_number (char *, int, int, YYSTYPE *);
static struct stoken operator_stoken (const char *);
static void check_parameter_typelist (VEC (type_ptr) *);


/* Line 264 of yacc.c  */
#line 387 "c-exp.c"

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
#define YYFINAL  165
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1636

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  89
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  46
/* YYNRULES -- Number of rules.  */
#define YYNRULES  254
/* YYNRULES -- Number of states.  */
#define YYNSTATES  401

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   319

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    83,     2,     2,     2,    69,    55,     2,
      75,    82,    67,    65,    47,    66,    73,    68,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    86,     2,
      58,    49,    59,    50,    64,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    74,     2,    85,    54,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    87,    53,    88,    84,     2,     2,     2,
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
      45,    46,    48,    51,    52,    56,    57,    60,    61,    62,
      63,    70,    71,    72,    76,    77,    78,    79,    80,    81
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    14,    19,    24,    26,
      30,    33,    36,    39,    42,    45,    48,    51,    54,    57,
      60,    63,    67,    72,    76,    80,    84,    88,    93,    97,
     101,   105,   110,   115,   116,   122,   123,   129,   130,   136,
     138,   140,   142,   145,   149,   152,   155,   156,   162,   163,
     169,   171,   172,   174,   178,   184,   186,   190,   195,   200,
     204,   208,   212,   216,   220,   224,   228,   232,   236,   240,
     244,   248,   252,   256,   260,   264,   268,   272,   276,   280,
     286,   290,   294,   296,   298,   300,   302,   304,   306,   308,
     313,   318,   326,   334,   342,   350,   352,   355,   357,   359,
     361,   363,   365,   367,   371,   374,   378,   382,   387,   393,
     395,   398,   400,   403,   405,   406,   410,   412,   414,   416,
     417,   418,   423,   424,   428,   430,   433,   435,   438,   440,
     442,   446,   449,   451,   454,   456,   459,   462,   466,   470,
     473,   477,   479,   481,   483,   485,   487,   490,   494,   497,
     501,   505,   509,   512,   515,   519,   524,   528,   532,   537,
     541,   546,   550,   555,   558,   562,   565,   569,   572,   576,
     578,   581,   584,   587,   591,   594,   597,   601,   604,   607,
     611,   614,   617,   621,   624,   626,   629,   631,   637,   640,
     643,   645,   647,   649,   651,   653,   657,   659,   663,   665,
     668,   671,   672,   675,   678,   681,   683,   685,   687,   690,
     693,   698,   703,   708,   713,   716,   719,   722,   725,   728,
     731,   734,   737,   740,   743,   746,   749,   752,   755,   758,
     761,   764,   767,   770,   773,   776,   779,   782,   785,   788,
     791,   794,   798,   802,   806,   809,   811,   813,   815,   817,
     819,   821,   823,   825,   827
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      90,     0,    -1,    92,    -1,    91,    -1,   122,    -1,    34,
      75,    93,    82,    -1,    34,    75,   122,    82,    -1,    35,
      75,    93,    82,    -1,    93,    -1,    92,    47,    93,    -1,
      67,    93,    -1,    55,    93,    -1,    66,    93,    -1,    65,
      93,    -1,    83,    93,    -1,    84,    93,    -1,    71,    93,
      -1,    70,    93,    -1,    93,    71,    -1,    93,    70,    -1,
      22,    93,    -1,    93,    78,   133,    -1,    93,    78,   133,
      12,    -1,    93,    78,    12,    -1,    93,    78,   108,    -1,
      93,    77,    93,    -1,    93,    73,   133,    -1,    93,    73,
     133,    12,    -1,    93,    73,    12,    -1,    93,    73,   108,
      -1,    93,    76,    93,    -1,    93,    74,    92,    85,    -1,
      93,    15,    92,    85,    -1,    -1,    15,    13,    94,    97,
      85,    -1,    -1,    15,    14,    95,    97,    85,    -1,    -1,
      15,    93,    96,    97,    85,    -1,   133,    -1,    98,    -1,
      99,    -1,    98,    99,    -1,   133,    86,    93,    -1,    86,
      93,    -1,    47,    93,    -1,    -1,    93,    75,   100,   103,
      82,    -1,    -1,    11,    75,   101,   103,    82,    -1,    87,
      -1,    -1,    93,    -1,   103,    47,    93,    -1,    93,    75,
     125,    82,   110,    -1,    88,    -1,   102,   103,   104,    -1,
     102,    91,   104,    93,    -1,    75,    91,    82,    93,    -1,
      75,    92,    82,    -1,    93,    64,    93,    -1,    93,    67,
      93,    -1,    93,    68,    93,    -1,    93,    69,    93,    -1,
      93,    65,    93,    -1,    93,    66,    93,    -1,    93,    63,
      93,    -1,    93,    62,    93,    -1,    93,    57,    93,    -1,
      93,    56,    93,    -1,    93,    61,    93,    -1,    93,    60,
      93,    -1,    93,    58,    93,    -1,    93,    59,    93,    -1,
      93,    55,    93,    -1,    93,    54,    93,    -1,    93,    53,
      93,    -1,    93,    52,    93,    -1,    93,    51,    93,    -1,
      93,    50,    93,    86,    93,    -1,    93,    49,    93,    -1,
      93,    44,    93,    -1,     3,    -1,     9,    -1,    16,    -1,
       4,    -1,     5,    -1,   107,    -1,    43,    -1,     8,    75,
     133,    82,    -1,    22,    75,   122,    82,    -1,    29,    58,
      91,    59,    75,    93,    82,    -1,    31,    58,    91,    59,
      75,    93,    82,    -1,    30,    58,    91,    59,    75,    93,
      82,    -1,    32,    58,    91,    59,    75,    93,    82,    -1,
       6,    -1,   105,     6,    -1,   105,    -1,     7,    -1,    45,
      -1,    46,    -1,    79,    -1,    80,    -1,   106,    24,   133,
      -1,   134,    33,    -1,   106,    24,   133,    -1,    13,    24,
     133,    -1,    13,    24,    84,   133,    -1,    13,    24,   133,
      24,   133,    -1,   108,    -1,    24,   134,    -1,   134,    -1,
      64,    10,    -1,   131,    -1,    -1,   110,   109,   110,    -1,
     111,    -1,   131,    -1,   112,    -1,    -1,    -1,   114,    67,
     115,   113,    -1,    -1,    67,   116,   113,    -1,    55,    -1,
      55,   114,    -1,   114,    -1,   117,   119,    -1,   117,    -1,
     119,    -1,    75,   118,    82,    -1,   119,   120,    -1,   120,
      -1,   119,   121,    -1,   121,    -1,    74,    85,    -1,    15,
      85,    -1,    74,     3,    85,    -1,    15,     3,    85,    -1,
      75,    82,    -1,    75,   125,    82,    -1,   127,    -1,    13,
      -1,    39,    -1,    37,    -1,    38,    -1,    37,    39,    -1,
      37,    36,    39,    -1,    37,    36,    -1,    36,    37,    39,
      -1,    23,    37,    39,    -1,    37,    23,    39,    -1,    37,
      23,    -1,    37,    37,    -1,    37,    37,    39,    -1,    37,
      37,    36,    39,    -1,    37,    37,    36,    -1,    36,    37,
      37,    -1,    36,    37,    37,    39,    -1,    23,    37,    37,
      -1,    23,    37,    37,    39,    -1,    37,    37,    23,    -1,
      37,    37,    23,    39,    -1,    38,    39,    -1,    38,    36,
      39,    -1,    38,    36,    -1,    23,    38,    39,    -1,    38,
      23,    -1,    38,    23,    39,    -1,    42,    -1,    37,    42,
      -1,    18,   133,    -1,    18,    12,    -1,    18,   133,    12,
      -1,    19,   133,    -1,    19,    12,    -1,    19,   133,    12,
      -1,    20,   133,    -1,    20,    12,    -1,    20,   133,    12,
      -1,    21,   133,    -1,    21,    12,    -1,    21,   133,    12,
      -1,    23,   124,    -1,    23,    -1,    36,   124,    -1,    36,
      -1,    25,   133,    58,   122,    59,    -1,   112,   123,    -1,
     123,   112,    -1,    13,    -1,    39,    -1,    37,    -1,    38,
      -1,   126,    -1,   126,    47,    81,    -1,   122,    -1,   126,
      47,   122,    -1,   123,    -1,   127,   118,    -1,   123,   129,
      -1,    -1,   114,   129,    -1,    40,    41,    -1,    41,    40,
      -1,   130,    -1,    40,    -1,    41,    -1,    17,    27,    -1,
      17,    28,    -1,    17,    27,    74,    85,    -1,    17,    28,
      74,    85,    -1,    17,    27,    15,    85,    -1,    17,    28,
      15,    85,    -1,    17,    65,    -1,    17,    66,    -1,    17,
      67,    -1,    17,    68,    -1,    17,    69,    -1,    17,    54,
      -1,    17,    55,    -1,    17,    53,    -1,    17,    84,    -1,
      17,    83,    -1,    17,    49,    -1,    17,    58,    -1,    17,
      59,    -1,    17,    44,    -1,    17,    63,    -1,    17,    62,
      -1,    17,    57,    -1,    17,    56,    -1,    17,    61,    -1,
      17,    60,    -1,    17,    52,    -1,    17,    51,    -1,    17,
      71,    -1,    17,    70,    -1,    17,    47,    -1,    17,    77,
      -1,    17,    78,    -1,    17,    75,    82,    -1,    17,    74,
      85,    -1,    17,    15,    85,    -1,    17,   128,    -1,    10,
      -1,    79,    -1,    13,    -1,    16,    -1,    11,    -1,   132,
      -1,    10,    -1,    79,    -1,   132,    -1,    11,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   277,   277,   278,   281,   285,   289,   295,   302,   303,
     308,   312,   316,   320,   324,   328,   332,   336,   340,   344,
     348,   352,   358,   365,   375,   383,   387,   393,   400,   410,
     418,   422,   426,   436,   435,   458,   457,   473,   472,   481,
     483,   486,   487,   490,   492,   494,   501,   498,   509,   508,
     531,   535,   538,   542,   546,   564,   567,   574,   578,   582,
     588,   592,   596,   600,   604,   608,   612,   616,   620,   624,
     628,   632,   636,   640,   644,   648,   652,   656,   660,   664,
     668,   672,   678,   685,   694,   705,   712,   719,   722,   728,
     735,   745,   749,   753,   757,   764,   781,   799,   832,   841,
     848,   857,   865,   871,   881,   896,   918,   933,   957,   966,
     967,   995,  1049,  1053,  1054,  1057,  1060,  1061,  1065,  1066,
    1071,  1070,  1074,  1073,  1076,  1078,  1082,  1091,  1093,  1094,
    1097,  1099,  1106,  1113,  1119,  1126,  1128,  1130,  1132,  1136,
    1138,  1150,  1154,  1156,  1160,  1164,  1168,  1172,  1176,  1180,
    1184,  1188,  1192,  1196,  1200,  1204,  1208,  1212,  1216,  1220,
    1224,  1228,  1232,  1236,  1240,  1244,  1248,  1252,  1256,  1260,
    1264,  1268,  1271,  1276,  1282,  1285,  1290,  1296,  1299,  1304,
    1310,  1313,  1318,  1324,  1328,  1332,  1336,  1343,  1347,  1349,
    1353,  1354,  1362,  1370,  1381,  1383,  1392,  1398,  1405,  1406,
    1413,  1417,  1418,  1421,  1422,  1425,  1429,  1431,  1435,  1437,
    1439,  1441,  1443,  1445,  1447,  1449,  1451,  1453,  1455,  1457,
    1459,  1461,  1463,  1465,  1467,  1469,  1471,  1473,  1513,  1515,
    1517,  1519,  1521,  1523,  1525,  1527,  1529,  1531,  1533,  1535,
    1537,  1539,  1541,  1543,  1545,  1561,  1562,  1563,  1564,  1565,
    1566,  1569,  1570,  1578,  1590
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "FLOAT", "DECFLOAT", "STRING",
  "NSSTRING", "SELECTOR", "CHAR", "NAME", "UNKNOWN_CPP_NAME", "COMPLETE",
  "TYPENAME", "CLASSNAME", "OBJC_LBRAC", "NAME_OR_INT", "OPERATOR",
  "STRUCT", "CLASS", "UNION", "ENUM", "SIZEOF", "UNSIGNED", "COLONCOLON",
  "TEMPLATE", "ERROR", "NEW", "DELETE", "REINTERPRET_CAST", "DYNAMIC_CAST",
  "STATIC_CAST", "CONST_CAST", "ENTRY", "TYPEOF", "DECLTYPE",
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
  "operator", "name", "name_not_typename", 0
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
     295,   296,   297,   298,   299,   300,   301,    44,   302,    61,
      63,   303,   304,   124,    94,    38,   305,   306,    60,    62,
     307,   308,   309,   310,    64,    43,    45,    42,    47,    37,
     311,   312,   313,    46,    91,    40,   314,   315,   316,   317,
     318,   319,    41,    33,   126,    93,    58,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    89,    90,    90,    91,    91,    91,    91,    92,    92,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    94,    93,    95,    93,    96,    93,    97,
      97,    98,    98,    99,    99,    99,   100,    93,   101,    93,
     102,   103,   103,   103,    93,   104,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,   105,   105,    93,    93,    93,
      93,   106,   106,   106,   107,   107,   108,   108,   108,   107,
     107,   107,   109,   110,   110,   111,   112,   112,   113,   113,
     115,   114,   116,   114,   114,   114,   117,   118,   118,   118,
     119,   119,   119,   119,   119,   120,   120,   120,   120,   121,
     121,   122,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     124,   124,   124,   124,   125,   125,   126,   126,   127,   127,
     128,   129,   129,   130,   130,   131,   131,   131,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   133,   133,   133,   133,   133,
     133,   134,   134,   134,   134
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     4,     4,     4,     1,     3,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     3,     4,     3,     3,     3,     3,     4,     3,     3,
       3,     4,     4,     0,     5,     0,     5,     0,     5,     1,
       1,     1,     2,     3,     2,     2,     0,     5,     0,     5,
       1,     0,     1,     3,     5,     1,     3,     4,     4,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     5,
       3,     3,     1,     1,     1,     1,     1,     1,     1,     4,
       4,     7,     7,     7,     7,     1,     2,     1,     1,     1,
       1,     1,     1,     3,     2,     3,     3,     4,     5,     1,
       2,     1,     2,     1,     0,     3,     1,     1,     1,     0,
       0,     4,     0,     3,     1,     2,     1,     2,     1,     1,
       3,     2,     1,     2,     1,     2,     2,     3,     3,     2,
       3,     1,     1,     1,     1,     1,     2,     3,     2,     3,
       3,     3,     2,     2,     3,     4,     3,     3,     4,     3,
       4,     3,     4,     2,     3,     2,     3,     2,     3,     1,
       2,     2,     2,     3,     2,     2,     3,     2,     2,     3,
       2,     2,     3,     2,     1,     2,     1,     5,     2,     2,
       1,     1,     1,     1,     1,     3,     1,     3,     1,     2,
       2,     0,     2,     2,     2,     1,     1,     1,     2,     2,
       4,     4,     4,     4,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     3,     3,     3,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
     114,    82,    85,    86,    95,    98,     0,    83,   251,   254,
     142,     0,    84,   114,     0,     0,     0,     0,     0,   184,
       0,     0,     0,     0,     0,     0,     0,     0,   186,   144,
     145,   143,   206,   207,   169,    88,    99,   100,     0,     0,
       0,     0,     0,     0,   114,   252,   102,     0,     0,    50,
       0,     3,     2,     8,    51,    97,     0,    87,   109,     0,
     116,   114,     4,   198,   141,   205,   117,   253,   111,     0,
      48,     0,    33,    35,    37,   142,     0,   208,   209,   227,
     238,   224,   235,   234,   221,   219,   220,   231,   230,   225,
     226,   233,   232,   229,   228,   214,   215,   216,   217,   218,
     237,   236,     0,     0,   239,   240,   223,   222,   201,   244,
     245,   249,   172,   247,   248,   246,   250,   171,   175,   174,
     178,   177,   181,   180,     0,   114,    20,   190,   192,   193,
     191,   183,   254,   252,   110,     0,   114,   114,   114,   114,
     114,     0,   192,   193,   185,   152,   148,   153,   146,   170,
     167,   165,   163,   203,   204,    11,    13,    12,    10,    17,
      16,     0,     0,    14,    15,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      19,    18,     0,     0,    46,     0,     0,     0,     0,    52,
       0,    96,     0,     0,   114,   188,   189,     0,   124,   122,
       0,   114,   126,   128,   199,   129,   132,   134,   104,     0,
      51,     0,   106,     0,     0,     0,   243,     0,     0,     0,
       0,   242,   241,   201,   200,   173,   176,   179,   182,     0,
     159,   150,   166,   114,     0,     0,     0,     0,     0,     0,
       0,   157,   149,   151,   147,   161,   156,   154,   168,   164,
       0,    59,     9,     0,    81,    80,     0,    78,    77,    76,
      75,    74,    69,    68,    72,    73,    71,    70,    67,    66,
      60,    64,    65,    61,    62,    63,    28,   247,    29,    26,
       0,    51,   196,     0,   194,    30,    25,    23,    24,    21,
      55,     0,     0,    56,   105,   112,   115,   113,     0,   136,
     125,   119,     0,   135,   139,     0,     0,   120,   127,   114,
     131,   133,    89,     0,   107,     0,     0,     0,     0,    40,
      41,    39,     0,     0,   212,   210,   213,   211,   120,   202,
      90,   160,     0,     0,     0,     0,     0,     5,     6,     7,
     158,   162,   155,    58,    32,     0,    27,    31,     0,   114,
     114,    22,    57,    53,   138,   118,   123,   137,   130,   140,
     119,    49,   108,    45,    44,    34,    42,     0,     0,    36,
      38,   187,     0,     0,     0,     0,    79,    47,    54,   195,
     197,   121,    43,     0,     0,     0,     0,    91,    93,    92,
      94
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    50,   161,   162,    53,   223,   224,   225,   328,   329,
     330,   291,   220,    54,   200,   301,    55,    56,    57,    58,
     204,    59,    60,    61,   366,   212,   370,   311,   213,   214,
     215,   216,   217,    62,    63,   131,   316,   294,    64,   109,
     234,    65,    66,    67,   331,    68
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -212
static const yytype_int16 yypact[] =
{
     457,  -212,  -212,  -212,  -212,  -212,   -51,  -212,  -212,   -20,
      34,   255,  -212,   785,    67,   137,   214,   223,   627,    96,
      24,   197,    37,    54,    57,    61,    68,   113,   216,   553,
     159,  -212,   148,   152,  -212,  -212,  -212,  -212,   712,   712,
     712,   712,   712,   712,   457,   169,  -212,   712,   712,  -212,
     201,  -212,   150,  1287,   372,   196,   187,  -212,  -212,   157,
    -212,  1594,  -212,    64,    51,  -212,   164,  -212,   179,   197,
    -212,    43,    34,  -212,  1287,  -212,   147,    27,    46,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,   153,   155,  -212,  -212,  -212,  -212,   241,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,   230,  -212,   231,
    -212,   232,  -212,   244,    34,   457,   443,  -212,   144,   218,
    -212,  -212,  -212,  -212,  -212,   209,  1563,  1563,  1563,  1563,
     542,   712,   183,  -212,  -212,   234,   236,   167,  -212,  -212,
     239,   249,  -212,  -212,  -212,   443,   443,   443,   443,   443,
     443,   192,    63,   443,   443,  -212,   712,   712,   712,   712,
     712,   712,   712,   712,   712,   712,   712,   712,   712,   712,
     712,   712,   712,   712,   712,   712,   712,   712,   712,   712,
    -212,  -212,   235,   712,  1554,   712,   712,   337,   195,  1287,
      29,  -212,   197,   279,     9,    64,  -212,    11,   225,  -212,
      17,   903,   227,    23,  -212,    25,  -212,  -212,  -212,   213,
     712,   197,   273,    35,    35,    35,  -212,   219,   221,   222,
     226,  -212,  -212,    56,  -212,  -212,  -212,  -212,  -212,   217,
     274,  -212,  -212,  1594,   253,   258,   259,   265,   945,   246,
    1002,   290,  -212,  -212,  -212,   293,   294,  -212,  -212,  -212,
     712,  -212,  1287,   -21,  1287,  1287,   834,  1344,  1374,  1401,
    1431,  1458,  1488,  1488,   966,   966,   966,   966,   355,   355,
     898,   288,   288,   443,   443,   443,  -212,    34,  -212,   324,
      47,   712,  -212,   261,   298,   431,   431,  -212,  -212,   325,
    -212,   712,   712,  -212,   316,  -212,  -212,  -212,   256,  -212,
     227,    64,   266,  -212,  -212,   264,   270,  -212,    25,    49,
    -212,  -212,  -212,    69,  -212,   197,   712,   712,   275,    35,
    -212,   282,   284,   299,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,   327,   323,   330,   359,   360,  -212,  -212,  -212,
    -212,  -212,  -212,   443,  -212,   712,  -212,  -212,   105,     9,
     912,  -212,   443,  1287,  -212,  -212,  -212,  -212,  -212,  -212,
      64,  -212,  -212,  1287,  1287,  -212,  -212,   282,   712,  -212,
    -212,  -212,   712,   712,   712,   712,  1317,  -212,  -212,  -212,
    -212,  -212,  1287,  1059,  1116,  1173,  1230,  -212,  -212,  -212,
    -212
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -212,  -212,     3,    12,   -11,  -212,  -212,  -212,   -88,  -212,
      70,  -212,  -212,  -212,  -198,   200,  -212,  -212,  -212,  -153,
    -212,  -203,  -212,   -61,    71,   -95,  -212,  -212,  -212,   229,
     237,  -211,  -210,  -119,    20,   416,   251,  -212,  -212,  -212,
     215,  -212,  -179,    -6,     2,   429
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -115
static const yytype_int16 yytable[] =
{
      74,   306,   206,    51,   320,   321,   239,   126,   116,   116,
     116,   116,    52,   233,   308,   116,   117,   119,   121,   123,
     312,   249,   323,   135,    69,   307,   166,   155,   156,   157,
     158,   159,   160,   108,     8,   132,   163,   164,   207,   288,
     207,    13,   227,   199,   298,   110,   111,   206,   113,    32,
      33,   114,    13,   110,   111,    70,   113,   198,    71,   114,
      13,   229,    75,   116,   354,   116,   207,    14,    15,    16,
      17,   219,    19,   222,    21,   292,   302,   110,   111,   112,
     113,   205,   326,   114,    13,    28,    29,    30,    31,    32,
      33,    34,   292,   358,   166,   136,   309,   210,   211,   210,
     319,   228,   313,   133,    32,    33,   208,   320,   321,   127,
     166,   208,   137,   310,   115,   138,   302,   300,   209,   139,
     230,   327,   115,   338,   342,   210,   211,   221,  -114,   248,
     250,   314,   357,   128,   129,   130,   332,   333,   233,   244,
     245,   246,   247,   140,   206,   261,   115,   110,   111,   118,
     113,   371,   302,   114,    13,   262,   388,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   263,
     307,   240,   150,   241,   295,   296,   116,   387,   141,   153,
     255,   116,   154,  -101,   289,   151,   116,   166,   152,   299,
     292,   165,   201,   256,   304,   290,   257,   110,   111,   199,
     113,   202,   218,   114,    13,   116,   115,   116,   116,   116,
     251,   203,   252,   324,   110,   111,   120,   113,  -113,   127,
     114,    13,   226,   110,   111,   122,   113,   232,   231,   114,
      13,   390,   235,   236,   237,   110,   111,   286,   287,   353,
     365,   114,    13,   142,   143,   130,   238,   242,     1,     2,
       3,     4,     5,     6,     7,     8,     9,   243,    72,    73,
      11,    12,    13,   253,   260,   254,   115,    18,   258,    20,
     199,    32,    33,   300,    22,    23,    24,    25,   259,   305,
     362,   363,   209,   115,   317,   322,   208,   325,    35,   340,
      36,    37,   115,   167,   334,  -114,   335,   336,   209,   365,
      38,   337,   343,   341,   115,   373,   374,   344,   345,   116,
      39,    40,    41,   116,   346,    42,    43,   372,   348,   350,
      44,   377,   351,   352,    45,    46,   356,   361,    47,    48,
    -103,   364,    49,   359,   386,   360,   368,   110,   111,   297,
     287,   367,   369,   114,    13,   187,   188,   189,   190,   191,
     375,   192,   193,   194,   195,   196,   197,   392,   378,   379,
     167,   393,   394,   395,   396,     1,     2,     3,     4,     5,
       6,     7,     8,     9,   380,    10,   381,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,   382,   376,
     303,    22,    23,    24,    25,   383,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,   115,    36,    37,   184,
     185,   186,   187,   188,   189,   190,   191,    38,   192,   193,
     194,   195,   196,   197,   384,   385,  -114,    39,    40,    41,
     315,   391,    42,    43,   144,   293,   167,    44,   339,   134,
     318,    45,    46,     0,     0,    47,    48,     0,   167,    49,
       1,     2,     3,     4,     5,     6,     7,     8,     9,     0,
      10,     0,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,     0,     0,     0,    22,    23,    24,    25,
       0,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,     0,    36,    37,   192,   193,   194,   195,   196,   197,
       0,     0,    38,   190,   191,     0,   192,   193,   194,   195,
     196,   197,    39,    40,    41,     0,     0,    42,    43,     0,
       0,     0,    44,     0,     0,     0,    45,    46,     0,     0,
      47,    48,     0,     0,    49,     1,     2,     3,     4,     5,
       6,     7,     8,     9,     0,    10,     0,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,     0,     0,
       0,    22,    23,    24,    25,     0,   145,     0,    28,    29,
      30,    31,    32,    33,    34,    35,     0,    36,    37,   146,
     147,     0,   148,     0,     0,   149,     0,    38,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    39,    40,    41,
       0,     0,    42,    43,     0,     0,     0,    44,     0,     0,
       0,    45,    46,     0,     0,    47,    48,     0,     0,    49,
       1,     2,     3,     4,     5,     6,     7,     8,     9,     0,
     124,     0,    11,    12,    13,     0,     0,     0,     0,    18,
       0,    20,     0,     0,     0,     0,    22,    23,    24,    25,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      35,     0,    36,    37,     0,     0,     0,     0,     0,     0,
       0,     0,    38,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    39,    40,    41,     0,     0,    42,    43,     0,
       0,     0,   125,     0,     0,     0,    45,    46,     0,     0,
      47,    48,     0,     0,    49,     1,     2,     3,     4,     5,
       6,     7,     8,     9,     0,   124,     0,    11,    12,    13,
       0,     0,     0,     0,    18,     0,    20,     0,     0,     0,
       0,    22,    23,    24,    25,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    35,     0,    36,    37,     0,
       0,     0,     0,     0,     0,     0,     0,    38,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    39,    40,    41,
       0,     0,    42,    43,     0,     0,     0,    44,     0,     0,
       0,    45,    46,     0,     0,    47,    48,     0,    75,    49,
      76,     0,     0,    14,    15,    16,    17,     0,    19,     0,
      21,     0,    77,    78,     0,     0,     0,     0,     0,     0,
       0,    28,    29,    30,    31,    32,    33,    34,     0,    79,
       0,     0,    80,     0,    81,     0,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,   167,
      95,    96,    97,    98,    99,   100,   101,     0,     0,   102,
     103,     0,   104,   105,     0,     0,     0,     0,   106,   107,
       0,     0,     0,     0,     0,     0,     0,     0,   168,     0,
       0,     0,     0,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,     0,   192,   193,   194,
     195,   196,   197,   167,     0,     0,    75,     0,   207,     0,
     355,    14,    15,    16,    17,    75,    19,     0,    21,     0,
      14,    15,    16,    17,     0,    19,     0,    21,     0,    28,
      29,    30,    31,    32,    33,    34,     0,     0,    28,    29,
      30,    31,    32,    33,    34,     0,     0,     0,   208,     0,
     167,     0,     0,   185,   186,   187,   188,   189,   190,   191,
     209,   192,   193,   194,   195,   196,   197,   210,   211,     0,
       0,   167,     0,     0,     0,   314,     0,     0,     0,   168,
       0,     0,     0,   389,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   167,   192,   193,
     194,   195,   196,   197,     0,     0,     0,   347,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,     0,   192,
     193,   194,   195,   196,   197,     0,   168,     0,     0,     0,
       0,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   167,   192,   193,   194,   195,   196,
     197,     0,     0,     0,   349,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   168,     0,     0,     0,     0,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   167,   192,   193,   194,   195,   196,   197,     0,     0,
       0,   397,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     168,     0,     0,     0,     0,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   167,   192,
     193,   194,   195,   196,   197,     0,     0,     0,   398,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   168,     0,     0,
       0,     0,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   167,   192,   193,   194,   195,
     196,   197,     0,     0,     0,   399,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   168,     0,     0,     0,     0,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   167,   192,   193,   194,   195,   196,   197,     0,
       0,     0,   400,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   168,   167,     0,     0,     0,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   167,
     192,   193,   194,   195,   196,   197,     0,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   167,
     192,   193,   194,   195,   196,   197,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   167,   192,   193,   194,
     195,   196,   197,     0,     0,     0,     0,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   167,   192,   193,   194,
     195,   196,   197,     0,     0,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   167,   192,   193,   194,   195,   196,   197,
       0,     0,     0,     0,     0,     0,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   167,   192,   193,   194,   195,   196,   197,
       0,     0,     0,     0,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
       0,   192,   193,   194,   195,   196,   197,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
       0,   192,   193,   194,   195,   196,   197,    75,     0,     0,
       0,     0,    14,    15,    16,    17,    75,    19,     0,    21,
       0,    14,    15,    16,    17,     0,    19,     0,    21,     0,
      28,    29,    30,    31,    32,    33,    34,    26,    27,    28,
      29,    30,    31,    32,    33,    34,     0,    75,     0,     0,
       0,     0,    14,    15,    16,    17,     0,    19,  -114,    21,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    29,    30,    31,    32,    33,    34
};

static const yytype_int16 yycheck[] =
{
      11,   204,    63,     0,   215,   215,   125,    18,    14,    15,
      16,    17,     0,   108,     3,    21,    14,    15,    16,    17,
       3,   140,   220,    21,    75,   204,    47,    38,    39,    40,
      41,    42,    43,    13,    10,    11,    47,    48,    15,   192,
      15,    17,    15,    54,   197,    10,    11,   108,    13,    40,
      41,    16,    17,    10,    11,    75,    13,    54,    24,    16,
      17,    15,    13,    69,    85,    71,    15,    18,    19,    20,
      21,    69,    23,    71,    25,   194,    47,    10,    11,    12,
      13,    61,    47,    16,    17,    36,    37,    38,    39,    40,
      41,    42,   211,   291,    47,    58,    85,    74,    75,    74,
      75,    74,    85,    79,    40,    41,    55,   318,   318,    13,
      47,    55,    58,   208,    79,    58,    47,    88,    67,    58,
      74,    86,    79,    67,   243,    74,    75,    84,    64,   140,
     141,    82,    85,    37,    38,    39,   224,   225,   233,   136,
     137,   138,   139,    75,   205,    82,    79,    10,    11,    12,
      13,    82,    47,    16,    17,   166,   359,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   167,
     359,    37,    23,    39,   195,   196,   192,    82,    75,    41,
      23,   197,    40,    24,   192,    36,   202,    47,    39,   197,
     319,     0,     6,    36,   202,   193,    39,    10,    11,   220,
      13,    24,    33,    16,    17,   221,    79,   223,   224,   225,
      37,    64,    39,   221,    10,    11,    12,    13,    64,    13,
      16,    17,    85,    10,    11,    12,    13,    82,    85,    16,
      17,   360,    12,    12,    12,    10,    11,    12,    13,   260,
     311,    16,    17,    37,    38,    39,    12,    39,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    58,    13,    14,
      15,    16,    17,    39,    82,    39,    79,    22,    39,    24,
     291,    40,    41,    88,    29,    30,    31,    32,    39,    10,
     301,   302,    67,    79,    67,    82,    55,    24,    43,    82,
      45,    46,    79,    15,    85,    64,    85,    85,    67,   370,
      55,    85,    59,    39,    79,   326,   327,    59,    59,   325,
      65,    66,    67,   329,    59,    70,    71,   325,    82,    39,
      75,   329,    39,    39,    79,    80,    12,    12,    83,    84,
      24,    85,    87,    82,   355,    47,    82,    10,    11,    12,
      13,    85,    82,    16,    17,    67,    68,    69,    70,    71,
      85,    73,    74,    75,    76,    77,    78,   378,    86,    85,
      15,   382,   383,   384,   385,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    85,    13,    59,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    75,   329,
     200,    29,    30,    31,    32,    75,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    79,    45,    46,    64,
      65,    66,    67,    68,    69,    70,    71,    55,    73,    74,
      75,    76,    77,    78,    75,    75,    64,    65,    66,    67,
     211,   370,    70,    71,    28,   194,    15,    75,   233,    20,
     213,    79,    80,    -1,    -1,    83,    84,    -1,    15,    87,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      13,    -1,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    -1,    -1,    -1,    29,    30,    31,    32,
      -1,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    -1,    45,    46,    73,    74,    75,    76,    77,    78,
      -1,    -1,    55,    70,    71,    -1,    73,    74,    75,    76,
      77,    78,    65,    66,    67,    -1,    -1,    70,    71,    -1,
      -1,    -1,    75,    -1,    -1,    -1,    79,    80,    -1,    -1,
      83,    84,    -1,    -1,    87,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    13,    -1,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    -1,    -1,
      -1,    29,    30,    31,    32,    -1,    23,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    -1,    45,    46,    36,
      37,    -1,    39,    -1,    -1,    42,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    66,    67,
      -1,    -1,    70,    71,    -1,    -1,    -1,    75,    -1,    -1,
      -1,    79,    80,    -1,    -1,    83,    84,    -1,    -1,    87,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      13,    -1,    15,    16,    17,    -1,    -1,    -1,    -1,    22,
      -1,    24,    -1,    -1,    -1,    -1,    29,    30,    31,    32,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      43,    -1,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    66,    67,    -1,    -1,    70,    71,    -1,
      -1,    -1,    75,    -1,    -1,    -1,    79,    80,    -1,    -1,
      83,    84,    -1,    -1,    87,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    13,    -1,    15,    16,    17,
      -1,    -1,    -1,    -1,    22,    -1,    24,    -1,    -1,    -1,
      -1,    29,    30,    31,    32,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    43,    -1,    45,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    66,    67,
      -1,    -1,    70,    71,    -1,    -1,    -1,    75,    -1,    -1,
      -1,    79,    80,    -1,    -1,    83,    84,    -1,    13,    87,
      15,    -1,    -1,    18,    19,    20,    21,    -1,    23,    -1,
      25,    -1,    27,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    37,    38,    39,    40,    41,    42,    -1,    44,
      -1,    -1,    47,    -1,    49,    -1,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    15,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    74,
      75,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    84,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    -1,    73,    74,    75,
      76,    77,    78,    15,    -1,    -1,    13,    -1,    15,    -1,
      86,    18,    19,    20,    21,    13,    23,    -1,    25,    -1,
      18,    19,    20,    21,    -1,    23,    -1,    25,    -1,    36,
      37,    38,    39,    40,    41,    42,    -1,    -1,    36,    37,
      38,    39,    40,    41,    42,    -1,    -1,    -1,    55,    -1,
      15,    -1,    -1,    65,    66,    67,    68,    69,    70,    71,
      67,    73,    74,    75,    76,    77,    78,    74,    75,    -1,
      -1,    15,    -1,    -1,    -1,    82,    -1,    -1,    -1,    44,
      -1,    -1,    -1,    81,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    15,    73,    74,
      75,    76,    77,    78,    -1,    -1,    -1,    82,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    -1,    73,
      74,    75,    76,    77,    78,    -1,    44,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    15,    73,    74,    75,    76,    77,
      78,    -1,    -1,    -1,    82,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    15,    73,    74,    75,    76,    77,    78,    -1,    -1,
      -1,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      44,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    15,    73,
      74,    75,    76,    77,    78,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    15,    73,    74,    75,    76,
      77,    78,    -1,    -1,    -1,    82,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    15,    73,    74,    75,    76,    77,    78,    -1,
      -1,    -1,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    44,    15,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    15,
      73,    74,    75,    76,    77,    78,    -1,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    15,
      73,    74,    75,    76,    77,    78,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    15,    73,    74,    75,
      76,    77,    78,    -1,    -1,    -1,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    15,    73,    74,    75,
      76,    77,    78,    -1,    -1,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    15,    73,    74,    75,    76,    77,    78,
      -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    15,    73,    74,    75,    76,    77,    78,
      -1,    -1,    -1,    -1,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      -1,    73,    74,    75,    76,    77,    78,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      -1,    73,    74,    75,    76,    77,    78,    13,    -1,    -1,
      -1,    -1,    18,    19,    20,    21,    13,    23,    -1,    25,
      -1,    18,    19,    20,    21,    -1,    23,    -1,    25,    -1,
      36,    37,    38,    39,    40,    41,    42,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    -1,    13,    -1,    -1,
      -1,    -1,    18,    19,    20,    21,    -1,    23,    64,    25,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      36,    37,    38,    39,    40,    41,    42
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      13,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    29,    30,    31,    32,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    45,    46,    55,    65,
      66,    67,    70,    71,    75,    79,    80,    83,    84,    87,
      90,    91,    92,    93,   102,   105,   106,   107,   108,   110,
     111,   112,   122,   123,   127,   130,   131,   132,   134,    75,
      75,    24,    13,    14,    93,    13,    15,    27,    28,    44,
      47,    49,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    65,    66,    67,    68,    69,
      70,    71,    74,    75,    77,    78,    83,    84,   123,   128,
      10,    11,    12,    13,    16,    79,   132,   133,    12,   133,
      12,   133,    12,   133,    13,    75,    93,    13,    37,    38,
      39,   124,    11,    79,   134,   133,    58,    58,    58,    58,
      75,    75,    37,    38,   124,    23,    36,    37,    39,    42,
      23,    36,    39,    41,    40,    93,    93,    93,    93,    93,
      93,    91,    92,    93,    93,     0,    47,    15,    44,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    73,    74,    75,    76,    77,    78,    91,    93,
     103,     6,    24,    64,   109,   123,   112,    15,    55,    67,
      74,    75,   114,   117,   118,   119,   120,   121,    33,   133,
     101,    84,   133,    94,    95,    96,    85,    15,    74,    15,
      74,    85,    82,   114,   129,    12,    12,    12,    12,   122,
      37,    39,    39,    58,    91,    91,    91,    91,    93,   122,
      93,    37,    39,    39,    39,    23,    36,    39,    39,    39,
      82,    82,    93,    92,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    93,    12,    13,   108,   133,
      92,   100,   122,   125,   126,    93,    93,    12,   108,   133,
      88,   104,    47,   104,   133,    10,   110,   131,     3,    85,
     114,   116,     3,    85,    82,   118,   125,    67,   119,    75,
     120,   121,    82,   103,   133,    24,    47,    86,    97,    98,
      99,   133,    97,    97,    85,    85,    85,    85,    67,   129,
      82,    39,   122,    59,    59,    59,    59,    82,    82,    82,
      39,    39,    39,    93,    85,    86,    12,    85,   103,    82,
      47,    12,    93,    93,    85,   112,   113,    85,    82,    82,
     115,    82,   133,    93,    93,    85,    99,   133,    86,    85,
      85,    59,    75,    75,    75,    75,    93,    82,   110,    81,
     122,   113,    93,    93,    93,    93,    93,    82,    82,    82,
      82
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
#line 282 "c-exp.y"
    { write_exp_elt_opcode(OP_TYPE);
			  write_exp_elt_type((yyvsp[(1) - (1)].tval));
			  write_exp_elt_opcode(OP_TYPE);}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 286 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_TYPEOF);
			}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 290 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_TYPE);
			  write_exp_elt_type ((yyvsp[(3) - (4)].tval));
			  write_exp_elt_opcode (OP_TYPE);
			}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 296 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_DECLTYPE);
			}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 304 "c-exp.y"
    { write_exp_elt_opcode (BINOP_COMMA); }
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 309 "c-exp.y"
    { write_exp_elt_opcode (UNOP_IND); }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 313 "c-exp.y"
    { write_exp_elt_opcode (UNOP_ADDR); }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 317 "c-exp.y"
    { write_exp_elt_opcode (UNOP_NEG); }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 321 "c-exp.y"
    { write_exp_elt_opcode (UNOP_PLUS); }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 325 "c-exp.y"
    { write_exp_elt_opcode (UNOP_LOGICAL_NOT); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 329 "c-exp.y"
    { write_exp_elt_opcode (UNOP_COMPLEMENT); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 333 "c-exp.y"
    { write_exp_elt_opcode (UNOP_PREINCREMENT); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 337 "c-exp.y"
    { write_exp_elt_opcode (UNOP_PREDECREMENT); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 341 "c-exp.y"
    { write_exp_elt_opcode (UNOP_POSTINCREMENT); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 345 "c-exp.y"
    { write_exp_elt_opcode (UNOP_POSTDECREMENT); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 349 "c-exp.y"
    { write_exp_elt_opcode (UNOP_SIZEOF); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 353 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string ((yyvsp[(3) - (3)].sval));
			  write_exp_elt_opcode (STRUCTOP_PTR); }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 359 "c-exp.y"
    { mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string ((yyvsp[(3) - (4)].sval));
			  write_exp_elt_opcode (STRUCTOP_PTR); }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 366 "c-exp.y"
    { struct stoken s;
			  mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_PTR);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (s);
			  write_exp_elt_opcode (STRUCTOP_PTR); }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 376 "c-exp.y"
    { /* exp->type::name becomes exp->*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (STRUCTOP_MPTR); }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 384 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_MPTR); }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 388 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string ((yyvsp[(3) - (3)].sval));
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 394 "c-exp.y"
    { mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string ((yyvsp[(3) - (4)].sval));
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 401 "c-exp.y"
    { struct stoken s;
			  mark_struct_expression ();
			  write_exp_elt_opcode (STRUCTOP_STRUCT);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (s);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 411 "c-exp.y"
    { /* exp.type::name becomes exp.*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (STRUCTOP_MEMBER); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 419 "c-exp.y"
    { write_exp_elt_opcode (STRUCTOP_MEMBER); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 423 "c-exp.y"
    { write_exp_elt_opcode (BINOP_SUBSCRIPT); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 427 "c-exp.y"
    { write_exp_elt_opcode (BINOP_SUBSCRIPT); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 436 "c-exp.y"
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

  case 34:

/* Line 1455 of yacc.c  */
#line 451 "c-exp.y"
    { write_exp_elt_opcode (OP_OBJC_MSGCALL);
			  end_msglist();
			  write_exp_elt_opcode (OP_OBJC_MSGCALL);
			}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 458 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (parse_type->builtin_int);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[(2) - (2)].class).class);
			  write_exp_elt_opcode (OP_LONG);
			  start_msglist();
			}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 466 "c-exp.y"
    { write_exp_elt_opcode (OP_OBJC_MSGCALL);
			  end_msglist();
			  write_exp_elt_opcode (OP_OBJC_MSGCALL);
			}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 473 "c-exp.y"
    { start_msglist(); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 475 "c-exp.y"
    { write_exp_elt_opcode (OP_OBJC_MSGCALL);
			  end_msglist();
			  write_exp_elt_opcode (OP_OBJC_MSGCALL);
			}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 482 "c-exp.y"
    { add_msglist(&(yyvsp[(1) - (1)].sval), 0); }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 491 "c-exp.y"
    { add_msglist(&(yyvsp[(1) - (3)].sval), 1); }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 493 "c-exp.y"
    { add_msglist(0, 1);   }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 495 "c-exp.y"
    { add_msglist(0, 0);   }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 501 "c-exp.y"
    { start_arglist (); }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 503 "c-exp.y"
    { write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL); }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 509 "c-exp.y"
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

  case 49:

/* Line 1455 of yacc.c  */
#line 524 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL);
			}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 532 "c-exp.y"
    { start_arglist (); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 539 "c-exp.y"
    { arglist_len = 1; }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 543 "c-exp.y"
    { arglist_len++; }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 547 "c-exp.y"
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

  case 55:

/* Line 1455 of yacc.c  */
#line 565 "c-exp.y"
    { (yyval.lval) = end_arglist () - 1; }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 568 "c-exp.y"
    { write_exp_elt_opcode (OP_ARRAY);
			  write_exp_elt_longcst ((LONGEST) 0);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[(3) - (3)].lval));
			  write_exp_elt_opcode (OP_ARRAY); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 575 "c-exp.y"
    { write_exp_elt_opcode (UNOP_MEMVAL_TYPE); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 579 "c-exp.y"
    { write_exp_elt_opcode (UNOP_CAST_TYPE); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 583 "c-exp.y"
    { }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 589 "c-exp.y"
    { write_exp_elt_opcode (BINOP_REPEAT); }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 593 "c-exp.y"
    { write_exp_elt_opcode (BINOP_MUL); }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 597 "c-exp.y"
    { write_exp_elt_opcode (BINOP_DIV); }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 601 "c-exp.y"
    { write_exp_elt_opcode (BINOP_REM); }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 605 "c-exp.y"
    { write_exp_elt_opcode (BINOP_ADD); }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 609 "c-exp.y"
    { write_exp_elt_opcode (BINOP_SUB); }
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 613 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LSH); }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 617 "c-exp.y"
    { write_exp_elt_opcode (BINOP_RSH); }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 621 "c-exp.y"
    { write_exp_elt_opcode (BINOP_EQUAL); }
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 625 "c-exp.y"
    { write_exp_elt_opcode (BINOP_NOTEQUAL); }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 629 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LEQ); }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 633 "c-exp.y"
    { write_exp_elt_opcode (BINOP_GEQ); }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 637 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LESS); }
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 641 "c-exp.y"
    { write_exp_elt_opcode (BINOP_GTR); }
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 645 "c-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_AND); }
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 649 "c-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_XOR); }
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 653 "c-exp.y"
    { write_exp_elt_opcode (BINOP_BITWISE_IOR); }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 657 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LOGICAL_AND); }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 661 "c-exp.y"
    { write_exp_elt_opcode (BINOP_LOGICAL_OR); }
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 665 "c-exp.y"
    { write_exp_elt_opcode (TERNOP_COND); }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 669 "c-exp.y"
    { write_exp_elt_opcode (BINOP_ASSIGN); }
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 673 "c-exp.y"
    { write_exp_elt_opcode (BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode ((yyvsp[(2) - (3)].opcode));
			  write_exp_elt_opcode (BINOP_ASSIGN_MODIFY); }
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 679 "c-exp.y"
    { write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type ((yyvsp[(1) - (1)].typed_val_int).type);
			  write_exp_elt_longcst ((LONGEST)((yyvsp[(1) - (1)].typed_val_int).val));
			  write_exp_elt_opcode (OP_LONG); }
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 686 "c-exp.y"
    {
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &(yyvsp[(1) - (1)].tsval);
			  write_exp_string_vector ((yyvsp[(1) - (1)].tsval).type, &vec);
			}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 695 "c-exp.y"
    { YYSTYPE val;
			  parse_number ((yyvsp[(1) - (1)].ssym).stoken.ptr, (yyvsp[(1) - (1)].ssym).stoken.length, 0, &val);
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (val.typed_val_int.type);
			  write_exp_elt_longcst ((LONGEST)val.typed_val_int.val);
			  write_exp_elt_opcode (OP_LONG);
			}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 706 "c-exp.y"
    { write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type ((yyvsp[(1) - (1)].typed_val_float).type);
			  write_exp_elt_dblcst ((yyvsp[(1) - (1)].typed_val_float).dval);
			  write_exp_elt_opcode (OP_DOUBLE); }
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 713 "c-exp.y"
    { write_exp_elt_opcode (OP_DECFLOAT);
			  write_exp_elt_type ((yyvsp[(1) - (1)].typed_val_decfloat).type);
			  write_exp_elt_decfloatcst ((yyvsp[(1) - (1)].typed_val_decfloat).val);
			  write_exp_elt_opcode (OP_DECFLOAT); }
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 723 "c-exp.y"
    {
			  write_dollar_variable ((yyvsp[(1) - (1)].sval));
			}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 729 "c-exp.y"
    {
			  write_exp_elt_opcode (OP_OBJC_SELECTOR);
			  write_exp_string ((yyvsp[(3) - (4)].sval));
			  write_exp_elt_opcode (OP_OBJC_SELECTOR); }
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 736 "c-exp.y"
    { write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (lookup_signed_typename
					      (parse_language, parse_gdbarch,
					       "int"));
			  CHECK_TYPEDEF ((yyvsp[(3) - (4)].tval));
			  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH ((yyvsp[(3) - (4)].tval)));
			  write_exp_elt_opcode (OP_LONG); }
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 746 "c-exp.y"
    { write_exp_elt_opcode (UNOP_REINTERPRET_CAST); }
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 750 "c-exp.y"
    { write_exp_elt_opcode (UNOP_CAST_TYPE); }
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 754 "c-exp.y"
    { write_exp_elt_opcode (UNOP_DYNAMIC_CAST); }
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 758 "c-exp.y"
    { /* We could do more error checking here, but
			     it doesn't seem worthwhile.  */
			  write_exp_elt_opcode (UNOP_CAST_TYPE); }
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 765 "c-exp.y"
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

  case 96:

/* Line 1455 of yacc.c  */
#line 782 "c-exp.y"
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

  case 97:

/* Line 1455 of yacc.c  */
#line 800 "c-exp.y"
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

  case 98:

/* Line 1455 of yacc.c  */
#line 835 "c-exp.y"
    { write_exp_elt_opcode (OP_OBJC_NSSTRING);
			  write_exp_string ((yyvsp[(1) - (1)].sval));
			  write_exp_elt_opcode (OP_OBJC_NSSTRING); }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 842 "c-exp.y"
    { write_exp_elt_opcode (OP_LONG);
                          write_exp_elt_type (parse_type->builtin_bool);
                          write_exp_elt_longcst ((LONGEST) 1);
                          write_exp_elt_opcode (OP_LONG); }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 849 "c-exp.y"
    { write_exp_elt_opcode (OP_LONG);
                          write_exp_elt_type (parse_type->builtin_bool);
                          write_exp_elt_longcst ((LONGEST) 0);
                          write_exp_elt_opcode (OP_LONG); }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 858 "c-exp.y"
    {
			  if ((yyvsp[(1) - (1)].ssym).sym)
			    (yyval.bval) = SYMBOL_BLOCK_VALUE ((yyvsp[(1) - (1)].ssym).sym);
			  else
			    error (_("No file or function \"%s\"."),
				   copy_name ((yyvsp[(1) - (1)].ssym).stoken));
			}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 866 "c-exp.y"
    {
			  (yyval.bval) = (yyvsp[(1) - (1)].bval);
			}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 872 "c-exp.y"
    { struct symbol *tem
			    = lookup_symbol (copy_name ((yyvsp[(3) - (3)].sval)), (yyvsp[(1) - (3)].bval),
					     VAR_DOMAIN, NULL);
			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy_name ((yyvsp[(3) - (3)].sval)));
			  (yyval.bval) = SYMBOL_BLOCK_VALUE (tem); }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 882 "c-exp.y"
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

  case 105:

/* Line 1455 of yacc.c  */
#line 897 "c-exp.y"
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

  case 106:

/* Line 1455 of yacc.c  */
#line 919 "c-exp.y"
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

  case 107:

/* Line 1455 of yacc.c  */
#line 934 "c-exp.y"
    {
			  struct type *type = (yyvsp[(1) - (4)].tsym).type;
			  struct stoken tmp_token;
			  CHECK_TYPEDEF (type);
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION
			      && TYPE_CODE (type) != TYPE_CODE_NAMESPACE)
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));

			  tmp_token.ptr = (char*) alloca ((yyvsp[(4) - (4)].sval).length + 2);
			  tmp_token.length = (yyvsp[(4) - (4)].sval).length + 1;
			  tmp_token.ptr[0] = '~';
			  memcpy (tmp_token.ptr+1, (yyvsp[(4) - (4)].sval).ptr, (yyvsp[(4) - (4)].sval).length);
			  tmp_token.ptr[tmp_token.length] = 0;

			  /* Check for valid destructor name.  */
			  destructor_name_p (tmp_token.ptr, (yyvsp[(1) - (4)].tsym).type);
			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string (tmp_token);
			  write_exp_elt_opcode (OP_SCOPE);
			}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 958 "c-exp.y"
    {
			  char *copy = copy_name ((yyvsp[(3) - (5)].sval));
			  error (_("No type \"%s\" within class "
				   "or namespace \"%s\"."),
				 copy, TYPE_SAFE_NAME ((yyvsp[(1) - (5)].tsym).type));
			}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 968 "c-exp.y"
    {
			  char *name = copy_name ((yyvsp[(2) - (2)].ssym).stoken);
			  struct symbol *sym;
			  struct minimal_symbol *msymbol;

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

			  msymbol = lookup_minimal_symbol (name, NULL, NULL);
			  if (msymbol != NULL)
			    write_exp_msymbol (msymbol);
			  else if (!have_full_symbols () && !have_partial_symbols ())
			    error (_("No symbol table is loaded.  Use the \"file\" command."));
			  else
			    error (_("No symbol \"%s\" in current context."), name);
			}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 996 "c-exp.y"
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
			      struct minimal_symbol *msymbol;
			      char *arg = copy_name ((yyvsp[(1) - (1)].ssym).stoken);

			      msymbol =
				lookup_minimal_symbol (arg, NULL, NULL);
			      if (msymbol != NULL)
				write_exp_msymbol (msymbol);
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error (_("No symbol table is loaded.  Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       copy_name ((yyvsp[(1) - (1)].ssym).stoken));
			    }
			}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1050 "c-exp.y"
    { insert_type_address_space (copy_name ((yyvsp[(2) - (2)].ssym).stoken)); }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1071 "c-exp.y"
    { insert_type (tp_pointer); }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1074 "c-exp.y"
    { insert_type (tp_pointer); }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1077 "c-exp.y"
    { insert_type (tp_reference); }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1079 "c-exp.y"
    { insert_type (tp_reference); }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1083 "c-exp.y"
    {
			  (yyval.type_stack) = get_type_stack ();
			  /* This cleanup is eventually run by
			     c_parse.  */
			  make_cleanup (type_stack_cleanup, (yyval.type_stack));
			}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1092 "c-exp.y"
    { (yyval.type_stack) = append_type_stack ((yyvsp[(2) - (2)].type_stack), (yyvsp[(1) - (2)].type_stack)); }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1098 "c-exp.y"
    { (yyval.type_stack) = (yyvsp[(2) - (3)].type_stack); }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1100 "c-exp.y"
    {
			  push_type_stack ((yyvsp[(1) - (2)].type_stack));
			  push_type_int ((yyvsp[(2) - (2)].lval));
			  push_type (tp_array);
			  (yyval.type_stack) = get_type_stack ();
			}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1107 "c-exp.y"
    {
			  push_type_int ((yyvsp[(1) - (1)].lval));
			  push_type (tp_array);
			  (yyval.type_stack) = get_type_stack ();
			}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1114 "c-exp.y"
    {
			  push_type_stack ((yyvsp[(1) - (2)].type_stack));
			  push_typelist ((yyvsp[(2) - (2)].tvec));
			  (yyval.type_stack) = get_type_stack ();
			}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1120 "c-exp.y"
    {
			  push_typelist ((yyvsp[(1) - (1)].tvec));
			  (yyval.type_stack) = get_type_stack ();
			}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1127 "c-exp.y"
    { (yyval.lval) = -1; }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1129 "c-exp.y"
    { (yyval.lval) = -1; }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1131 "c-exp.y"
    { (yyval.lval) = (yyvsp[(2) - (3)].typed_val_int).val; }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1133 "c-exp.y"
    { (yyval.lval) = (yyvsp[(2) - (3)].typed_val_int).val; }
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1137 "c-exp.y"
    { (yyval.tvec) = NULL; }
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1139 "c-exp.y"
    { (yyval.tvec) = (yyvsp[(2) - (3)].tvec); }
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1155 "c-exp.y"
    { (yyval.tval) = (yyvsp[(1) - (1)].tsym).type; }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1157 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "int"); }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1161 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1165 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "short"); }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1169 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1173 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1177 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1181 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long"); }
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1185 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long"); }
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1189 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long"); }
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1193 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long"); }
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1197 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1201 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1205 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1209 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1213 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1217 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "long long"); }
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1221 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long long"); }
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1225 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long long"); }
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1229 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long long"); }
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1233 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "long long"); }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1237 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "short"); }
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1241 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "short"); }
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1245 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "short"); }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1249 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "short"); }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1253 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "short"); }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1257 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "short"); }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1261 "c-exp.y"
    { (yyval.tval) = lookup_typename (parse_language, parse_gdbarch,
						"double", (struct block *) NULL,
						0); }
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1265 "c-exp.y"
    { (yyval.tval) = lookup_typename (parse_language, parse_gdbarch,
						"long double",
						(struct block *) NULL, 0); }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1269 "c-exp.y"
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[(2) - (2)].sval)),
					      expression_context_block); }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1272 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, "", 0);
			  (yyval.tval) = NULL;
			}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1277 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, (yyvsp[(2) - (3)].sval).ptr,
					       (yyvsp[(2) - (3)].sval).length);
			  (yyval.tval) = NULL;
			}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1283 "c-exp.y"
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[(2) - (2)].sval)),
					      expression_context_block); }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1286 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_CLASS, "", 0);
			  (yyval.tval) = NULL;
			}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1291 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_CLASS, (yyvsp[(2) - (3)].sval).ptr,
					       (yyvsp[(2) - (3)].sval).length);
			  (yyval.tval) = NULL;
			}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1297 "c-exp.y"
    { (yyval.tval) = lookup_union (copy_name ((yyvsp[(2) - (2)].sval)),
					     expression_context_block); }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1300 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_UNION, "", 0);
			  (yyval.tval) = NULL;
			}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1305 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_UNION, (yyvsp[(2) - (3)].sval).ptr,
					       (yyvsp[(2) - (3)].sval).length);
			  (yyval.tval) = NULL;
			}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1311 "c-exp.y"
    { (yyval.tval) = lookup_enum (copy_name ((yyvsp[(2) - (2)].sval)),
					    expression_context_block); }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1314 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_ENUM, "", 0);
			  (yyval.tval) = NULL;
			}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1319 "c-exp.y"
    {
			  mark_completion_tag (TYPE_CODE_ENUM, (yyvsp[(2) - (3)].sval).ptr,
					       (yyvsp[(2) - (3)].sval).length);
			  (yyval.tval) = NULL;
			}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1325 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 TYPE_NAME((yyvsp[(2) - (2)].tsym).type)); }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1329 "c-exp.y"
    { (yyval.tval) = lookup_unsigned_typename (parse_language,
							 parse_gdbarch,
							 "int"); }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1333 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       TYPE_NAME((yyvsp[(2) - (2)].tsym).type)); }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1337 "c-exp.y"
    { (yyval.tval) = lookup_signed_typename (parse_language,
						       parse_gdbarch,
						       "int"); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1344 "c-exp.y"
    { (yyval.tval) = lookup_template_type(copy_name((yyvsp[(2) - (5)].sval)), (yyvsp[(4) - (5)].tval),
						    expression_context_block);
			}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1348 "c-exp.y"
    { (yyval.tval) = follow_types ((yyvsp[(2) - (2)].tval)); }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1350 "c-exp.y"
    { (yyval.tval) = follow_types ((yyvsp[(1) - (2)].tval)); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1355 "c-exp.y"
    {
		  (yyval.tsym).stoken.ptr = "int";
		  (yyval.tsym).stoken.length = 3;
		  (yyval.tsym).type = lookup_signed_typename (parse_language,
						    parse_gdbarch,
						    "int");
		}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1363 "c-exp.y"
    {
		  (yyval.tsym).stoken.ptr = "long";
		  (yyval.tsym).stoken.length = 4;
		  (yyval.tsym).type = lookup_signed_typename (parse_language,
						    parse_gdbarch,
						    "long");
		}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1371 "c-exp.y"
    {
		  (yyval.tsym).stoken.ptr = "short";
		  (yyval.tsym).stoken.length = 5;
		  (yyval.tsym).type = lookup_signed_typename (parse_language,
						    parse_gdbarch,
						    "short");
		}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1382 "c-exp.y"
    { check_parameter_typelist ((yyvsp[(1) - (1)].tvec)); }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1384 "c-exp.y"
    {
			  VEC_safe_push (type_ptr, (yyvsp[(1) - (3)].tvec), NULL);
			  check_parameter_typelist ((yyvsp[(1) - (3)].tvec));
			  (yyval.tvec) = (yyvsp[(1) - (3)].tvec);
			}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1393 "c-exp.y"
    {
		  VEC (type_ptr) *typelist = NULL;
		  VEC_safe_push (type_ptr, typelist, (yyvsp[(1) - (1)].tval));
		  (yyval.tvec) = typelist;
		}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1399 "c-exp.y"
    {
		  VEC_safe_push (type_ptr, (yyvsp[(1) - (3)].tvec), (yyvsp[(3) - (3)].tval));
		  (yyval.tvec) = (yyvsp[(1) - (3)].tvec);
		}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1407 "c-exp.y"
    {
		  push_type_stack ((yyvsp[(2) - (2)].type_stack));
		  (yyval.tval) = follow_types ((yyvsp[(1) - (2)].tval));
		}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1414 "c-exp.y"
    { (yyval.tval) = follow_types ((yyvsp[(1) - (2)].tval)); }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1426 "c-exp.y"
    { insert_type (tp_const);
			  insert_type (tp_volatile); 
			}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1430 "c-exp.y"
    { insert_type (tp_const); }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1432 "c-exp.y"
    { insert_type (tp_volatile); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1436 "c-exp.y"
    { (yyval.sval) = operator_stoken (" new"); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1438 "c-exp.y"
    { (yyval.sval) = operator_stoken (" delete"); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1440 "c-exp.y"
    { (yyval.sval) = operator_stoken (" new[]"); }
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1442 "c-exp.y"
    { (yyval.sval) = operator_stoken (" delete[]"); }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1444 "c-exp.y"
    { (yyval.sval) = operator_stoken (" new[]"); }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1446 "c-exp.y"
    { (yyval.sval) = operator_stoken (" delete[]"); }
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1448 "c-exp.y"
    { (yyval.sval) = operator_stoken ("+"); }
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1450 "c-exp.y"
    { (yyval.sval) = operator_stoken ("-"); }
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1452 "c-exp.y"
    { (yyval.sval) = operator_stoken ("*"); }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1454 "c-exp.y"
    { (yyval.sval) = operator_stoken ("/"); }
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1456 "c-exp.y"
    { (yyval.sval) = operator_stoken ("%"); }
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1458 "c-exp.y"
    { (yyval.sval) = operator_stoken ("^"); }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1460 "c-exp.y"
    { (yyval.sval) = operator_stoken ("&"); }
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1462 "c-exp.y"
    { (yyval.sval) = operator_stoken ("|"); }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1464 "c-exp.y"
    { (yyval.sval) = operator_stoken ("~"); }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1466 "c-exp.y"
    { (yyval.sval) = operator_stoken ("!"); }
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1468 "c-exp.y"
    { (yyval.sval) = operator_stoken ("="); }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1470 "c-exp.y"
    { (yyval.sval) = operator_stoken ("<"); }
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1472 "c-exp.y"
    { (yyval.sval) = operator_stoken (">"); }
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1474 "c-exp.y"
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

  case 228:

/* Line 1455 of yacc.c  */
#line 1514 "c-exp.y"
    { (yyval.sval) = operator_stoken ("<<"); }
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1516 "c-exp.y"
    { (yyval.sval) = operator_stoken (">>"); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1518 "c-exp.y"
    { (yyval.sval) = operator_stoken ("=="); }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1520 "c-exp.y"
    { (yyval.sval) = operator_stoken ("!="); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1522 "c-exp.y"
    { (yyval.sval) = operator_stoken ("<="); }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1524 "c-exp.y"
    { (yyval.sval) = operator_stoken (">="); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1526 "c-exp.y"
    { (yyval.sval) = operator_stoken ("&&"); }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1528 "c-exp.y"
    { (yyval.sval) = operator_stoken ("||"); }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1530 "c-exp.y"
    { (yyval.sval) = operator_stoken ("++"); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1532 "c-exp.y"
    { (yyval.sval) = operator_stoken ("--"); }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1534 "c-exp.y"
    { (yyval.sval) = operator_stoken (","); }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1536 "c-exp.y"
    { (yyval.sval) = operator_stoken ("->*"); }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1538 "c-exp.y"
    { (yyval.sval) = operator_stoken ("->"); }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1540 "c-exp.y"
    { (yyval.sval) = operator_stoken ("()"); }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1542 "c-exp.y"
    { (yyval.sval) = operator_stoken ("[]"); }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1544 "c-exp.y"
    { (yyval.sval) = operator_stoken ("[]"); }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1546 "c-exp.y"
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

  case 245:

/* Line 1455 of yacc.c  */
#line 1561 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ssym).stoken; }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1562 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ssym).stoken; }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1563 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].tsym).stoken; }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1564 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ssym).stoken; }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1565 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ssym).stoken; }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1566 "c-exp.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].sval); }
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1579 "c-exp.y"
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



/* Line 1455 of yacc.c  */
#line 4369 "c-exp.c"
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
#line 1593 "c-exp.y"


/* Returns a stoken of the operator name given by OP (which does not
   include the string "operator").  */ 
static struct stoken
operator_stoken (const char *op)
{
  static const char *operator_string = "operator";
  struct stoken st = { NULL, 0 };
  st.length = strlen (operator_string) + strlen (op);
  st.ptr = xmalloc (st.length + 1);
  strcpy (st.ptr, operator_string);
  strcat (st.ptr, op);

  /* The toplevel (c_parse) will free the memory allocated here.  */
  make_cleanup (xfree, st.ptr);
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
c_parse_escape (char **ptr, struct obstack *output)
{
  char *tokptr = *ptr;
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
parse_string_or_char (char *tokptr, char **outptr, struct typed_stoken *value,
		      int *host_chars)
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
    {"decltype", DECLTYPE, OP_DECLTYPE, FLAG_CXX | FLAG_SHADOW }
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
static char *macro_original_text;

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
	char *p = tokstart;
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
	char *p = &tokstart[1];
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
               
	      char * p = find_template_name_end (tokstart + namelen);
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
   in which lookups start; this can be NULL to mean the global
   scope.  */
static int
classify_name (const struct block *block)
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
      /* See if it's a file name. */
      struct symtab *symtab;

      symtab = lookup_symtab (copy);
      if (symtab)
	{
	  yylval.bval = BLOCKVECTOR_BLOCK (BLOCKVECTOR (symtab), STATIC_BLOCK);
	  return FILENAME;
	}

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
    return classify_name (block);

  type = check_typedef (context);
  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
      && TYPE_CODE (type) != TYPE_CODE_UNION
      && TYPE_CODE (type) != TYPE_CODE_NAMESPACE)
    return ERROR;

  copy = copy_name (yylval.ssym.stoken);
  yylval.ssym.sym = cp_lookup_nested_symbol (type, copy, block);
  if (yylval.ssym.sym == NULL)
    return ERROR;

  switch (SYMBOL_CLASS (yylval.ssym.sym))
    {
    case LOC_BLOCK:
    case LOC_LABEL:
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
  int first_was_coloncolon, last_was_coloncolon, first_iter;
  struct type *context_type = NULL;

  if (popping && !VEC_empty (token_and_value, token_fifo))
    {
      token_and_value tv = *VEC_index (token_and_value, token_fifo, 0);
      VEC_ordered_remove (token_and_value, token_fifo, 0);
      yylval = tv.value;
      return tv.token;
    }
  popping = 0;

  current.token = lex_one_token ();
  if (current.token == NAME)
    current.token = classify_name (expression_context_block);
  if (parse_language->la_language != language_cplus
      || (current.token != TYPENAME && current.token != COLONCOLON))
    return current.token;

  first_was_coloncolon = current.token == COLONCOLON;
  last_was_coloncolon = first_was_coloncolon;
  obstack_free (&name_obstack, obstack_base (&name_obstack));
  if (!last_was_coloncolon)
    {
      obstack_grow (&name_obstack, yylval.sval.ptr, yylval.sval.length);
      context_type = yylval.tsym.type;
    }
  current.value = yylval;
  first_iter = 1;
  while (1)
    {
      token_and_value next;

      next.token = lex_one_token ();
      next.value = yylval;

      if (next.token == NAME && last_was_coloncolon)
	{
	  int classification;

	  classification = classify_inner_name (first_was_coloncolon
						? NULL
						: expression_context_block,
						context_type);
	  /* We keep going until we either run out of names, or until
	     we have a qualified name which is not a type.  */
	  if (classification != TYPENAME && classification != NAME)
	    {
	      /* Push the final component and leave the loop.  */
	      VEC_safe_push (token_and_value, token_fifo, &next);
	      break;
	    }

	  /* Update the partial name we are constructing.  */
	  if (context_type != NULL)
	    {
	      /* We don't want to put a leading "::" into the name.  */
	      obstack_grow_str (&name_obstack, "::");
	    }
	  obstack_grow (&name_obstack, next.value.sval.ptr,
			next.value.sval.length);

	  yylval.sval.ptr = obstack_base (&name_obstack);
	  yylval.sval.length = obstack_object_size (&name_obstack);
	  current.value = yylval;
	  current.token = classification;

	  last_was_coloncolon = 0;
	  
	  if (classification == NAME)
	    break;

	  context_type = yylval.tsym.type;
	}
      else if (next.token == COLONCOLON && !last_was_coloncolon)
	last_was_coloncolon = 1;
      else
	{
	  /* We've reached the end of the name.  */
	  VEC_safe_push (token_and_value, token_fifo, &next);
	  break;
	}

      first_iter = 0;
    }

  popping = 1;

  /* If we ended with a "::", insert it too.  */
  if (last_was_coloncolon)
    {
      token_and_value cc;
      memset (&cc, 0, sizeof (token_and_value));
      if (first_was_coloncolon && first_iter)
	{
	  yylval = cc.value;
	  return COLONCOLON;
	}
      cc.token = COLONCOLON;
      VEC_safe_insert (token_and_value, token_fifo, 0, &cc);
    }

  yylval = current.value;
  yylval.sval.ptr = obstack_copy0 (&expansion_obstack,
				   yylval.sval.ptr,
				   yylval.sval.length);
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


void
yyerror (char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), (msg ? msg : "error"), lexptr);
}

