/* A Bison parser, made by GNU Bison 3.0.2.  */

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
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 39 "d-exp.y" /* yacc.c:339  */


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "d-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"

#define parse_type(ps) builtin_type (parse_gdbarch (ps))
#define parse_d_type(ps) builtin_d_type (parse_gdbarch (ps))

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in gdb.  Note that these are only the variables
   produced by yacc.  If other parser generators (bison, byacc, etc) produce
   additional global names that conflict at link time, then those parser
   generators need to be fixed instead of adding those names to this list.  */

#define	yymaxdepth d_maxdepth
#define	yyparse	d_parse_internal
#define	yylex	d_lex
#define	yyerror	d_error
#define	yylval	d_lval
#define	yychar	d_char
#define	yydebug	d_debug
#define	yypact	d_pact
#define	yyr1	d_r1
#define	yyr2	d_r2
#define	yydef	d_def
#define	yychk	d_chk
#define	yypgo	d_pgo
#define	yyact	d_act
#define	yyexca	d_exca
#define	yyerrflag d_errflag
#define	yynerrs	d_nerrs
#define	yyps	d_ps
#define	yypv	d_pv
#define	yys	d_s
#define	yy_yys	d_yys
#define	yystate	d_state
#define	yytmp	d_tmp
#define	yyv	d_v
#define	yy_yyv	d_yyv
#define	yyval	d_val
#define	yylloc	d_lloc
#define	yyreds	d_reds	/* With YYDEBUG defined */
#define	yytoks	d_toks	/* With YYDEBUG defined */
#define	yyname	d_name	/* With YYDEBUG defined */
#define	yyrule	d_rule	/* With YYDEBUG defined */
#define	yylhs	d_yylhs
#define	yylen	d_yylen
#define	yydefre	d_yydefred
#define	yydgoto	d_yydgoto
#define	yysindex d_yysindex
#define	yyrindex d_yyrindex
#define	yygindex d_yygindex
#define	yytable	d_yytable
#define	yycheck	d_yycheck
#define	yyss	d_yyss
#define	yysslim	d_yysslim
#define	yyssp	d_yyssp
#define	yystacksize d_yystacksize
#define	yyvs	d_yyvs
#define	yyvsp	d_yyvsp

#ifndef YYDEBUG
#define YYDEBUG 1	/* Default to yydebug support */
#endif

#define YYFPRINTF parser_fprintf

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

void yyerror (char *);


#line 158 "d-exp.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    IDENTIFIER = 258,
    TYPENAME = 259,
    COMPLETE = 260,
    NAME_OR_INT = 261,
    INTEGER_LITERAL = 262,
    FLOAT_LITERAL = 263,
    CHARACTER_LITERAL = 264,
    STRING_LITERAL = 265,
    ENTRY = 266,
    ERROR = 267,
    TRUE_KEYWORD = 268,
    FALSE_KEYWORD = 269,
    NULL_KEYWORD = 270,
    SUPER_KEYWORD = 271,
    CAST_KEYWORD = 272,
    SIZEOF_KEYWORD = 273,
    TYPEOF_KEYWORD = 274,
    TYPEID_KEYWORD = 275,
    INIT_KEYWORD = 276,
    IMMUTABLE_KEYWORD = 277,
    CONST_KEYWORD = 278,
    SHARED_KEYWORD = 279,
    STRUCT_KEYWORD = 280,
    UNION_KEYWORD = 281,
    CLASS_KEYWORD = 282,
    INTERFACE_KEYWORD = 283,
    ENUM_KEYWORD = 284,
    TEMPLATE_KEYWORD = 285,
    DELEGATE_KEYWORD = 286,
    FUNCTION_KEYWORD = 287,
    DOLLAR_VARIABLE = 288,
    ASSIGN_MODIFY = 289,
    OROR = 290,
    ANDAND = 291,
    EQUAL = 292,
    NOTEQUAL = 293,
    LEQ = 294,
    GEQ = 295,
    LSH = 296,
    RSH = 297,
    HATHAT = 298,
    IDENTITY = 299,
    NOTIDENTITY = 300,
    INCREMENT = 301,
    DECREMENT = 302,
    DOTDOT = 303
  };
#endif
/* Tokens.  */
#define IDENTIFIER 258
#define TYPENAME 259
#define COMPLETE 260
#define NAME_OR_INT 261
#define INTEGER_LITERAL 262
#define FLOAT_LITERAL 263
#define CHARACTER_LITERAL 264
#define STRING_LITERAL 265
#define ENTRY 266
#define ERROR 267
#define TRUE_KEYWORD 268
#define FALSE_KEYWORD 269
#define NULL_KEYWORD 270
#define SUPER_KEYWORD 271
#define CAST_KEYWORD 272
#define SIZEOF_KEYWORD 273
#define TYPEOF_KEYWORD 274
#define TYPEID_KEYWORD 275
#define INIT_KEYWORD 276
#define IMMUTABLE_KEYWORD 277
#define CONST_KEYWORD 278
#define SHARED_KEYWORD 279
#define STRUCT_KEYWORD 280
#define UNION_KEYWORD 281
#define CLASS_KEYWORD 282
#define INTERFACE_KEYWORD 283
#define ENUM_KEYWORD 284
#define TEMPLATE_KEYWORD 285
#define DELEGATE_KEYWORD 286
#define FUNCTION_KEYWORD 287
#define DOLLAR_VARIABLE 288
#define ASSIGN_MODIFY 289
#define OROR 290
#define ANDAND 291
#define EQUAL 292
#define NOTEQUAL 293
#define LEQ 294
#define GEQ 295
#define LSH 296
#define RSH 297
#define HATHAT 298
#define IDENTITY 299
#define NOTIDENTITY 300
#define INCREMENT 301
#define DECREMENT 302
#define DOTDOT 303

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 136 "d-exp.y" /* yacc.c:355  */

    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      DOUBLEST dval;
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct typed_stoken tsval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int ival;
    struct block *bval;
    enum exp_opcode opcode;
    struct stoken_vector svec;
  

#line 313 "d-exp.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */
#line 157 "d-exp.y" /* yacc.c:358  */

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *,
			 int, int, YYSTYPE *);

static void push_expression_name (struct parser_state *, struct stoken);

#line 335 "d-exp.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
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
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#   if ! defined xmalloc && ! defined EXIT_SUCCESS
void *xmalloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE xfree
#   if ! defined xfree && ! defined EXIT_SUCCESS
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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

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
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  79
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   236

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  111
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  175

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   303

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    67,     2,     2,     2,    55,    42,     2,
      63,    68,    53,    51,    35,    52,    61,    54,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    65,     2,
      45,    36,    46,    37,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    62,     2,    69,    41,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    40,     2,    66,     2,     2,     2,
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
      38,    39,    43,    44,    47,    48,    49,    50,    56,    57,
      58,    59,    60,    64
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   232,   232,   233,   239,   243,   244,   249,   250,   252,
     259,   260,   265,   266,   271,   272,   277,   278,   283,   284,
     289,   290,   295,   296,   297,   298,   302,   304,   309,   311,
     316,   318,   320,   322,   327,   328,   330,   335,   336,   338,
     340,   345,   346,   348,   350,   354,   356,   358,   360,   362,
     364,   366,   368,   370,   371,   375,   381,   388,   389,   394,
     395,   397,   399,   400,   401,   405,   407,   413,   414,   419,
     418,   427,   440,   442,   447,   449,   451,   460,   466,   468,
     476,   483,   487,   491,   496,   501,   506,   512,   520,   525,
     526,   543,   558,   576,   580,   589,   591,   593,   596,   602,
     604,   607,   610,   613,   616,   619,   622,   625,   628,   631,
     634,   637
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENTIFIER", "TYPENAME", "COMPLETE",
  "NAME_OR_INT", "INTEGER_LITERAL", "FLOAT_LITERAL", "CHARACTER_LITERAL",
  "STRING_LITERAL", "ENTRY", "ERROR", "TRUE_KEYWORD", "FALSE_KEYWORD",
  "NULL_KEYWORD", "SUPER_KEYWORD", "CAST_KEYWORD", "SIZEOF_KEYWORD",
  "TYPEOF_KEYWORD", "TYPEID_KEYWORD", "INIT_KEYWORD", "IMMUTABLE_KEYWORD",
  "CONST_KEYWORD", "SHARED_KEYWORD", "STRUCT_KEYWORD", "UNION_KEYWORD",
  "CLASS_KEYWORD", "INTERFACE_KEYWORD", "ENUM_KEYWORD", "TEMPLATE_KEYWORD",
  "DELEGATE_KEYWORD", "FUNCTION_KEYWORD", "DOLLAR_VARIABLE",
  "ASSIGN_MODIFY", "','", "'='", "'?'", "OROR", "ANDAND", "'|'", "'^'",
  "'&'", "EQUAL", "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "LSH", "RSH",
  "'+'", "'-'", "'*'", "'/'", "'%'", "HATHAT", "IDENTITY", "NOTIDENTITY",
  "INCREMENT", "DECREMENT", "'.'", "'['", "'('", "DOTDOT", "':'", "'~'",
  "'!'", "')'", "']'", "$accept", "start", "Expression", "CommaExpression",
  "AssignExpression", "ConditionalExpression", "OrOrExpression",
  "AndAndExpression", "OrExpression", "XorExpression", "AndExpression",
  "CmpExpression", "EqualExpression", "IdentityExpression",
  "RelExpression", "ShiftExpression", "AddExpression", "MulExpression",
  "UnaryExpression", "CastExpression", "PowExpression",
  "PostfixExpression", "ArgumentList", "ArgumentList_opt",
  "CallExpression", "$@1", "IndexExpression", "SliceExpression",
  "PrimaryExpression", "ArrayLiteral", "IdentifierExp", "StringExp",
  "TypeExp", "BasicType2", "BasicType", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,    44,    61,    63,   290,   291,
     124,    94,    38,   292,   293,    60,    62,   294,   295,   296,
     297,    43,    45,    42,    47,    37,   298,   299,   300,   301,
     302,    46,    91,    40,   303,    58,   126,    33,    41,    93
};
# endif

#define YYPACT_NINF -86

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-86)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     107,   -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,
     -86,   -24,    37,    40,    76,    86,   -86,   169,   169,   169,
     169,   169,   169,   169,   107,   169,   169,    72,   -86,   -86,
      16,    68,    20,    34,    38,    43,    48,   -86,   -86,   -86,
     -86,    51,   -22,    -6,   -86,   -86,   -86,    67,   -86,   -86,
     -86,   -86,   -86,    32,    93,   -86,   -25,     9,   -86,     1,
     -86,    15,   -86,    19,   -86,    21,   -86,   -86,   -86,   -86,
     -86,   -86,   -86,    71,    56,    44,    60,   -86,   -86,   -86,
     169,   169,   169,   169,   169,   169,   169,   169,   169,   169,
     169,   169,   169,   169,   169,   169,   169,   169,   169,   169,
     169,   169,   169,   169,   169,   169,   -86,   -86,     8,   -86,
     132,   -86,   -25,   124,   -86,    73,   -86,   136,   -86,   -86,
     -86,   169,   -86,   -86,   169,   -86,   -86,   -86,    78,    34,
      38,    43,    48,   -86,    14,    14,    14,    14,    14,    14,
     -22,   -22,    14,    14,    -6,    -6,    -6,   -86,   -86,   -86,
     -86,   -86,    80,   -23,   169,   140,   -86,   -86,    77,   169,
     -86,   -86,   -86,   169,   169,   -86,    79,   -86,   -25,   -86,
     -86,    81,   -86,   -86,   -86
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    89,    99,    79,    83,    84,    85,    91,    81,    82,
      80,     0,     0,     0,     0,     0,    78,     0,     0,     0,
       0,     0,     0,    67,     0,     0,     0,     0,     2,     4,
       5,     7,    10,    12,    14,    16,    18,    20,    23,    24,
      25,    22,    34,    37,    41,    53,    54,    57,    62,    63,
      64,    59,    87,    75,    86,     3,    93,     0,   104,   103,
     107,   106,   101,   100,   110,   109,    45,    50,    49,    48,
      46,    47,    65,    68,     0,     0,     0,    52,    51,     1,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    60,    61,     0,    69,
       0,    92,    95,     0,    94,     0,   105,     0,   108,   102,
     111,     0,    88,    74,     0,     6,     9,     8,     0,    13,
      15,    17,    19,    21,    26,    27,    30,    32,    31,    33,
      35,    36,    28,    29,    38,    39,    40,    42,    43,    44,
      58,    72,    65,     0,    67,    90,    76,    96,     0,     0,
      90,    66,    56,     0,     0,    71,     0,    77,    97,    55,
      11,     0,    70,    98,    73
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -86,   -86,   -14,    74,   -16,   -12,   -86,    69,    70,    66,
      75,    83,   -86,   -86,   -86,    98,    23,   -68,   -17,   -86,
     -86,   -86,    49,     2,   -86,   -86,   -86,   -86,   -86,   -86,
      41,   -86,    -5,   -85,   -86
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    73,    74,    48,   154,    49,    50,    51,    52,
      53,    54,    55,   114,    56
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      66,    67,    68,    69,    70,    71,   116,    72,    77,    78,
      75,     1,   121,     2,     3,     4,     5,     6,     7,    76,
     118,     8,     9,    10,   119,    11,   120,   157,   112,    99,
     100,   144,   145,   146,    12,    13,    14,   113,    15,    57,
       1,    16,    58,     1,   101,    60,   165,   102,   103,   104,
      17,    80,   115,    59,    61,    63,    65,    83,    84,    18,
      19,    20,   117,    95,    96,   126,   127,    21,    22,   128,
      23,    24,    79,    85,    25,    26,   117,   151,    86,     1,
     117,    62,   117,   173,    87,   147,   148,   149,   150,     1,
      88,    64,   152,   110,    89,    90,    91,    92,    93,    94,
      95,    96,    81,   111,    82,   161,   121,   162,    97,    98,
       1,     2,   123,     3,     4,     5,     6,     7,   140,   141,
       8,     9,    10,   105,    11,   122,   106,   107,   124,   108,
     109,   158,    12,    13,    14,   155,    15,   156,    72,   160,
      16,   159,   169,   163,   164,   167,   168,   172,   171,    17,
     174,   170,   131,   129,   125,   130,   166,   153,    18,    19,
      20,     0,   132,     0,     0,     0,    21,    22,     0,    23,
      24,   133,     1,    25,    26,     3,     4,     5,     6,     7,
       0,     0,     8,     9,    10,     0,    11,   134,   135,   136,
     137,   138,   139,     0,     0,   142,   143,     0,     0,     0,
       0,     0,    16,     0,     0,     0,     0,     0,     0,     0,
       0,    17,     0,     0,     0,     0,     0,     0,     0,     0,
      18,    19,    20,     0,     0,     0,     0,     0,    21,    22,
       0,    23,    24,     0,     0,    25,    26
};

static const yytype_int16 yycheck[] =
{
      17,    18,    19,    20,    21,    22,     5,    23,    25,    26,
      24,     3,    35,     4,     6,     7,     8,     9,    10,    24,
       5,    13,    14,    15,     5,    17,     5,   112,    53,    51,
      52,    99,   100,   101,    25,    26,    27,    62,    29,    63,
       3,    33,     5,     3,    66,     5,    69,    53,    54,    55,
      42,    35,    57,    12,    13,    14,    15,    37,    38,    51,
      52,    53,    61,    49,    50,    81,    82,    59,    60,    83,
      62,    63,     0,    39,    66,    67,    61,    69,    40,     3,
      61,     5,    61,   168,    41,   102,   103,   104,   105,     3,
      42,     5,   108,    61,    43,    44,    45,    46,    47,    48,
      49,    50,    34,    10,    36,   121,    35,   124,    57,    58,
       3,     4,    68,     6,     7,     8,     9,    10,    95,    96,
      13,    14,    15,    56,    17,    69,    59,    60,    68,    62,
      63,     7,    25,    26,    27,     3,    29,     5,   154,     3,
      33,    68,   159,    65,    64,     5,    69,    68,   164,    42,
      69,   163,    86,    84,    80,    85,   154,   108,    51,    52,
      53,    -1,    87,    -1,    -1,    -1,    59,    60,    -1,    62,
      63,    88,     3,    66,    67,     6,     7,     8,     9,    10,
      -1,    -1,    13,    14,    15,    -1,    17,    89,    90,    91,
      92,    93,    94,    -1,    -1,    97,    98,    -1,    -1,    -1,
      -1,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      51,    52,    53,    -1,    -1,    -1,    -1,    -1,    59,    60,
      -1,    62,    63,    -1,    -1,    66,    67
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     6,     7,     8,     9,    10,    13,    14,
      15,    17,    25,    26,    27,    29,    33,    42,    51,    52,
      53,    59,    60,    62,    63,    66,    67,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    94,    96,
      97,    98,    99,   100,   101,   102,   104,    63,     5,   100,
       5,   100,     5,   100,     5,   100,    88,    88,    88,    88,
      88,    88,    74,    92,    93,    72,   102,    88,    88,     0,
      35,    34,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    57,    58,    51,
      52,    66,    53,    54,    55,    56,    59,    60,    62,    63,
      61,    10,    53,    62,   103,   102,     5,    61,     5,     5,
       5,    35,    69,    68,    68,    73,    74,    74,    72,    77,
      78,    79,    80,    81,    85,    85,    85,    85,    85,    85,
      86,    86,    85,    85,    87,    87,    87,    88,    88,    88,
      88,    69,    74,    92,    95,     3,     5,   103,     7,    68,
       3,    74,    88,    65,    64,    69,    93,     5,    69,    88,
      75,    74,    68,   103,    69
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    70,    71,    71,    72,    73,    73,    74,    74,    74,
      75,    75,    76,    76,    77,    77,    78,    78,    79,    79,
      80,    80,    81,    81,    81,    81,    82,    82,    83,    83,
      84,    84,    84,    84,    85,    85,    85,    86,    86,    86,
      86,    87,    87,    87,    87,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    89,    89,    90,    90,    91,
      91,    91,    91,    91,    91,    92,    92,    93,    93,    95,
      94,    96,    97,    97,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    98,    99,   100,
     100,   101,   101,   102,   102,   103,   103,   103,   103,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     3,     1,     3,     3,
       1,     5,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     1,     1,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       3,     1,     3,     3,     3,     2,     2,     2,     2,     2,
       2,     2,     2,     1,     1,     5,     4,     1,     3,     1,
       2,     2,     1,     1,     1,     1,     3,     0,     1,     0,
       5,     4,     3,     6,     3,     1,     3,     4,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     1,
       3,     1,     2,     1,     2,     1,     2,     3,     4,     1,
       2,     2,     3,     2,     2,     3,     2,     2,     3,     2,
       2,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


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
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
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

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

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
      yychar = yylex ();
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
     '$$ = $1'.

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
#line 245 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_COMMA); }
#line 1567 "d-exp.c" /* yacc.c:1646  */
    break;

  case 8:
#line 251 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
#line 1573 "d-exp.c" /* yacc.c:1646  */
    break;

  case 9:
#line 253 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
		  write_exp_elt_opcode (pstate, (yyvsp[-1].opcode));
		  write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY); }
#line 1581 "d-exp.c" /* yacc.c:1646  */
    break;

  case 11:
#line 261 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_COND); }
#line 1587 "d-exp.c" /* yacc.c:1646  */
    break;

  case 13:
#line 267 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
#line 1593 "d-exp.c" /* yacc.c:1646  */
    break;

  case 15:
#line 273 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
#line 1599 "d-exp.c" /* yacc.c:1646  */
    break;

  case 17:
#line 279 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
#line 1605 "d-exp.c" /* yacc.c:1646  */
    break;

  case 19:
#line 285 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
#line 1611 "d-exp.c" /* yacc.c:1646  */
    break;

  case 21:
#line 291 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
#line 1617 "d-exp.c" /* yacc.c:1646  */
    break;

  case 26:
#line 303 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL); }
#line 1623 "d-exp.c" /* yacc.c:1646  */
    break;

  case 27:
#line 305 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
#line 1629 "d-exp.c" /* yacc.c:1646  */
    break;

  case 28:
#line 310 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL); }
#line 1635 "d-exp.c" /* yacc.c:1646  */
    break;

  case 29:
#line 312 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
#line 1641 "d-exp.c" /* yacc.c:1646  */
    break;

  case 30:
#line 317 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LESS); }
#line 1647 "d-exp.c" /* yacc.c:1646  */
    break;

  case 31:
#line 319 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LEQ); }
#line 1653 "d-exp.c" /* yacc.c:1646  */
    break;

  case 32:
#line 321 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GTR); }
#line 1659 "d-exp.c" /* yacc.c:1646  */
    break;

  case 33:
#line 323 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GEQ); }
#line 1665 "d-exp.c" /* yacc.c:1646  */
    break;

  case 35:
#line 329 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LSH); }
#line 1671 "d-exp.c" /* yacc.c:1646  */
    break;

  case 36:
#line 331 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_RSH); }
#line 1677 "d-exp.c" /* yacc.c:1646  */
    break;

  case 38:
#line 337 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ADD); }
#line 1683 "d-exp.c" /* yacc.c:1646  */
    break;

  case 39:
#line 339 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUB); }
#line 1689 "d-exp.c" /* yacc.c:1646  */
    break;

  case 40:
#line 341 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_CONCAT); }
#line 1695 "d-exp.c" /* yacc.c:1646  */
    break;

  case 42:
#line 347 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_MUL); }
#line 1701 "d-exp.c" /* yacc.c:1646  */
    break;

  case 43:
#line 349 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_DIV); }
#line 1707 "d-exp.c" /* yacc.c:1646  */
    break;

  case 44:
#line 351 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REM); }
#line 1713 "d-exp.c" /* yacc.c:1646  */
    break;

  case 45:
#line 355 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ADDR); }
#line 1719 "d-exp.c" /* yacc.c:1646  */
    break;

  case 46:
#line 357 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREINCREMENT); }
#line 1725 "d-exp.c" /* yacc.c:1646  */
    break;

  case 47:
#line 359 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREDECREMENT); }
#line 1731 "d-exp.c" /* yacc.c:1646  */
    break;

  case 48:
#line 361 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_IND); }
#line 1737 "d-exp.c" /* yacc.c:1646  */
    break;

  case 49:
#line 363 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_NEG); }
#line 1743 "d-exp.c" /* yacc.c:1646  */
    break;

  case 50:
#line 365 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PLUS); }
#line 1749 "d-exp.c" /* yacc.c:1646  */
    break;

  case 51:
#line 367 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
#line 1755 "d-exp.c" /* yacc.c:1646  */
    break;

  case 52:
#line 369 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_COMPLEMENT); }
#line 1761 "d-exp.c" /* yacc.c:1646  */
    break;

  case 55:
#line 376 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST);
		  write_exp_elt_type (pstate, (yyvsp[-2].tval));
		  write_exp_elt_opcode (pstate, UNOP_CAST); }
#line 1769 "d-exp.c" /* yacc.c:1646  */
    break;

  case 56:
#line 382 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST);
		  write_exp_elt_type (pstate, (yyvsp[-2].tval));
		  write_exp_elt_opcode (pstate, UNOP_CAST); }
#line 1777 "d-exp.c" /* yacc.c:1646  */
    break;

  case 58:
#line 390 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EXP); }
#line 1783 "d-exp.c" /* yacc.c:1646  */
    break;

  case 60:
#line 396 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTINCREMENT); }
#line 1789 "d-exp.c" /* yacc.c:1646  */
    break;

  case 61:
#line 398 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTDECREMENT); }
#line 1795 "d-exp.c" /* yacc.c:1646  */
    break;

  case 65:
#line 406 "d-exp.y" /* yacc.c:1646  */
    { arglist_len = 1; }
#line 1801 "d-exp.c" /* yacc.c:1646  */
    break;

  case 66:
#line 408 "d-exp.y" /* yacc.c:1646  */
    { arglist_len++; }
#line 1807 "d-exp.c" /* yacc.c:1646  */
    break;

  case 67:
#line 413 "d-exp.y" /* yacc.c:1646  */
    { arglist_len = 0; }
#line 1813 "d-exp.c" /* yacc.c:1646  */
    break;

  case 69:
#line 419 "d-exp.y" /* yacc.c:1646  */
    { start_arglist (); }
#line 1819 "d-exp.c" /* yacc.c:1646  */
    break;

  case 70:
#line 421 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_FUNCALL);
		  write_exp_elt_longcst (pstate, (LONGEST) end_arglist ());
		  write_exp_elt_opcode (pstate, OP_FUNCALL); }
#line 1827 "d-exp.c" /* yacc.c:1646  */
    break;

  case 71:
#line 428 "d-exp.y" /* yacc.c:1646  */
    { if (arglist_len > 0)
		    {
		      write_exp_elt_opcode (pstate, MULTI_SUBSCRIPT);
		      write_exp_elt_longcst (pstate, (LONGEST) arglist_len);
		      write_exp_elt_opcode (pstate, MULTI_SUBSCRIPT);
		    }
		  else
		    write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT);
		}
#line 1841 "d-exp.c" /* yacc.c:1646  */
    break;

  case 72:
#line 441 "d-exp.y" /* yacc.c:1646  */
    { /* Do nothing.  */ }
#line 1847 "d-exp.c" /* yacc.c:1646  */
    break;

  case 73:
#line 443 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_SLICE); }
#line 1853 "d-exp.c" /* yacc.c:1646  */
    break;

  case 74:
#line 448 "d-exp.y" /* yacc.c:1646  */
    { /* Do nothing.  */ }
#line 1859 "d-exp.c" /* yacc.c:1646  */
    break;

  case 75:
#line 450 "d-exp.y" /* yacc.c:1646  */
    { push_expression_name (pstate, (yyvsp[0].sval)); }
#line 1865 "d-exp.c" /* yacc.c:1646  */
    break;

  case 76:
#line 452 "d-exp.y" /* yacc.c:1646  */
    { struct stoken s;
		  s.ptr = "";
		  s.length = 0;
		  push_expression_name (pstate, (yyvsp[-2].sval));
		  mark_struct_expression (pstate);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
		  write_exp_string (pstate, s);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 1878 "d-exp.c" /* yacc.c:1646  */
    break;

  case 77:
#line 461 "d-exp.y" /* yacc.c:1646  */
    { push_expression_name (pstate, (yyvsp[-3].sval));
		  mark_struct_expression (pstate);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
		  write_exp_string (pstate, (yyvsp[-1].sval));
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 1888 "d-exp.c" /* yacc.c:1646  */
    break;

  case 78:
#line 467 "d-exp.y" /* yacc.c:1646  */
    { write_dollar_variable (pstate, (yyvsp[0].sval)); }
#line 1894 "d-exp.c" /* yacc.c:1646  */
    break;

  case 79:
#line 469 "d-exp.y" /* yacc.c:1646  */
    { YYSTYPE val;
                  parse_number (pstate, (yyvsp[0].sval).ptr, (yyvsp[0].sval).length, 0, &val);
		  write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, val.typed_val_int.type);
		  write_exp_elt_longcst (pstate,
					 (LONGEST) val.typed_val_int.val);
		  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1906 "d-exp.c" /* yacc.c:1646  */
    break;

  case 80:
#line 477 "d-exp.y" /* yacc.c:1646  */
    { struct type *type = parse_d_type (pstate)->builtin_void;
		  type = lookup_pointer_type (type);
		  write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, type);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1917 "d-exp.c" /* yacc.c:1646  */
    break;

  case 81:
#line 484 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_BOOL);
		  write_exp_elt_longcst (pstate, (LONGEST) 1);
		  write_exp_elt_opcode (pstate, OP_BOOL); }
#line 1925 "d-exp.c" /* yacc.c:1646  */
    break;

  case 82:
#line 488 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_BOOL);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_opcode (pstate, OP_BOOL); }
#line 1933 "d-exp.c" /* yacc.c:1646  */
    break;

  case 83:
#line 492 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, (yyvsp[0].typed_val_int).type);
		  write_exp_elt_longcst (pstate, (LONGEST)((yyvsp[0].typed_val_int).val));
		  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1942 "d-exp.c" /* yacc.c:1646  */
    break;

  case 84:
#line 497 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DOUBLE);
		  write_exp_elt_type (pstate, (yyvsp[0].typed_val_float).type);
		  write_exp_elt_dblcst (pstate, (yyvsp[0].typed_val_float).dval);
		  write_exp_elt_opcode (pstate, OP_DOUBLE); }
#line 1951 "d-exp.c" /* yacc.c:1646  */
    break;

  case 85:
#line 502 "d-exp.y" /* yacc.c:1646  */
    { struct stoken_vector vec;
		  vec.len = 1;
		  vec.tokens = &(yyvsp[0].tsval);
		  write_exp_string_vector (pstate, (yyvsp[0].tsval).type, &vec); }
#line 1960 "d-exp.c" /* yacc.c:1646  */
    break;

  case 86:
#line 507 "d-exp.y" /* yacc.c:1646  */
    { int i;
		  write_exp_string_vector (pstate, 0, &(yyvsp[0].svec));
		  for (i = 0; i < (yyvsp[0].svec).len; ++i)
		    xfree ((yyvsp[0].svec).tokens[i].ptr);
		  xfree ((yyvsp[0].svec).tokens); }
#line 1970 "d-exp.c" /* yacc.c:1646  */
    break;

  case 87:
#line 513 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ARRAY);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].ival) - 1);
		  write_exp_elt_opcode (pstate, OP_ARRAY); }
#line 1979 "d-exp.c" /* yacc.c:1646  */
    break;

  case 88:
#line 521 "d-exp.y" /* yacc.c:1646  */
    { (yyval.ival) = arglist_len; }
#line 1985 "d-exp.c" /* yacc.c:1646  */
    break;

  case 90:
#line 527 "d-exp.y" /* yacc.c:1646  */
    { (yyval.sval).length = (yyvsp[-2].sval).length + (yyvsp[0].sval).length + 1;
		  if ((yyvsp[-2].sval).ptr + (yyvsp[-2].sval).length + 1 == (yyvsp[0].sval).ptr
		      && (yyvsp[-2].sval).ptr[(yyvsp[-2].sval).length] == '.')
		    (yyval.sval).ptr = (yyvsp[-2].sval).ptr;  /* Optimization.  */
		  else
		    {
		      char *buf = xmalloc ((yyval.sval).length + 1);
		      make_cleanup (xfree, buf);
		      sprintf (buf, "%.*s.%.*s",
		               (yyvsp[-2].sval).length, (yyvsp[-2].sval).ptr, (yyvsp[0].sval).length, (yyvsp[0].sval).ptr);
		      (yyval.sval).ptr = buf;
		    }
		}
#line 2003 "d-exp.c" /* yacc.c:1646  */
    break;

  case 91:
#line 544 "d-exp.y" /* yacc.c:1646  */
    { /* We copy the string here, and not in the
		     lexer, to guarantee that we do not leak a
		     string.  Note that we follow the
		     NUL-termination convention of the
		     lexer.  */
		  struct typed_stoken *vec = XNEW (struct typed_stoken);
		  (yyval.svec).len = 1;
		  (yyval.svec).tokens = vec;

		  vec->type = (yyvsp[0].tsval).type;
		  vec->length = (yyvsp[0].tsval).length;
		  vec->ptr = xmalloc ((yyvsp[0].tsval).length + 1);
		  memcpy (vec->ptr, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);
		}
#line 2022 "d-exp.c" /* yacc.c:1646  */
    break;

  case 92:
#line 559 "d-exp.y" /* yacc.c:1646  */
    { /* Note that we NUL-terminate here, but just
		     for convenience.  */
		  char *p;
		  ++(yyval.svec).len;
		  (yyval.svec).tokens = xrealloc ((yyval.svec).tokens,
				       (yyval.svec).len * sizeof (struct typed_stoken));

		  p = xmalloc ((yyvsp[0].tsval).length + 1);
		  memcpy (p, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);

		  (yyval.svec).tokens[(yyval.svec).len - 1].type = (yyvsp[0].tsval).type;
		  (yyval.svec).tokens[(yyval.svec).len - 1].length = (yyvsp[0].tsval).length;
		  (yyval.svec).tokens[(yyval.svec).len - 1].ptr = p;
		}
#line 2041 "d-exp.c" /* yacc.c:1646  */
    break;

  case 93:
#line 577 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPE);
		  write_exp_elt_type (pstate, (yyvsp[0].tval));
		  write_exp_elt_opcode (pstate, OP_TYPE); }
#line 2049 "d-exp.c" /* yacc.c:1646  */
    break;

  case 94:
#line 581 "d-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = follow_types ((yyvsp[-1].tval));
		  write_exp_elt_opcode (pstate, OP_TYPE);
		  write_exp_elt_type (pstate, (yyval.tval));
		  write_exp_elt_opcode (pstate, OP_TYPE);
		}
#line 2059 "d-exp.c" /* yacc.c:1646  */
    break;

  case 95:
#line 590 "d-exp.y" /* yacc.c:1646  */
    { push_type (tp_pointer); }
#line 2065 "d-exp.c" /* yacc.c:1646  */
    break;

  case 96:
#line 592 "d-exp.y" /* yacc.c:1646  */
    { push_type (tp_pointer); }
#line 2071 "d-exp.c" /* yacc.c:1646  */
    break;

  case 97:
#line 594 "d-exp.y" /* yacc.c:1646  */
    { push_type_int ((yyvsp[-1].typed_val_int).val);
		  push_type (tp_array); }
#line 2078 "d-exp.c" /* yacc.c:1646  */
    break;

  case 98:
#line 597 "d-exp.y" /* yacc.c:1646  */
    { push_type_int ((yyvsp[-2].typed_val_int).val);
		  push_type (tp_array); }
#line 2085 "d-exp.c" /* yacc.c:1646  */
    break;

  case 99:
#line 603 "d-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 2091 "d-exp.c" /* yacc.c:1646  */
    break;

  case 100:
#line 605 "d-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
				      expression_context_block); }
#line 2098 "d-exp.c" /* yacc.c:1646  */
    break;

  case 101:
#line 608 "d-exp.y" /* yacc.c:1646  */
    { mark_completion_tag (TYPE_CODE_STRUCT, "", 0);
		  (yyval.tval) = NULL; }
#line 2105 "d-exp.c" /* yacc.c:1646  */
    break;

  case 102:
#line 611 "d-exp.y" /* yacc.c:1646  */
    { mark_completion_tag (TYPE_CODE_STRUCT, (yyvsp[-1].sval).ptr, (yyvsp[-1].sval).length);
		  (yyval.tval) = NULL; }
#line 2112 "d-exp.c" /* yacc.c:1646  */
    break;

  case 103:
#line 614 "d-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
				      expression_context_block); }
#line 2119 "d-exp.c" /* yacc.c:1646  */
    break;

  case 104:
#line 617 "d-exp.y" /* yacc.c:1646  */
    { mark_completion_tag (TYPE_CODE_STRUCT, "", 0);
		  (yyval.tval) = NULL; }
#line 2126 "d-exp.c" /* yacc.c:1646  */
    break;

  case 105:
#line 620 "d-exp.y" /* yacc.c:1646  */
    { mark_completion_tag (TYPE_CODE_STRUCT, (yyvsp[-1].sval).ptr, (yyvsp[-1].sval).length);
		  (yyval.tval) = NULL; }
#line 2133 "d-exp.c" /* yacc.c:1646  */
    break;

  case 106:
#line 623 "d-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_union (copy_name ((yyvsp[0].sval)),
				     expression_context_block); }
#line 2140 "d-exp.c" /* yacc.c:1646  */
    break;

  case 107:
#line 626 "d-exp.y" /* yacc.c:1646  */
    { mark_completion_tag (TYPE_CODE_UNION, "", 0);
		  (yyval.tval) = NULL; }
#line 2147 "d-exp.c" /* yacc.c:1646  */
    break;

  case 108:
#line 629 "d-exp.y" /* yacc.c:1646  */
    { mark_completion_tag (TYPE_CODE_UNION, (yyvsp[-1].sval).ptr, (yyvsp[-1].sval).length);
		  (yyval.tval) = NULL; }
#line 2154 "d-exp.c" /* yacc.c:1646  */
    break;

  case 109:
#line 632 "d-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_enum (copy_name ((yyvsp[0].sval)),
				    expression_context_block); }
#line 2161 "d-exp.c" /* yacc.c:1646  */
    break;

  case 110:
#line 635 "d-exp.y" /* yacc.c:1646  */
    { mark_completion_tag (TYPE_CODE_ENUM, "", 0);
		  (yyval.tval) = NULL; }
#line 2168 "d-exp.c" /* yacc.c:1646  */
    break;

  case 111:
#line 638 "d-exp.y" /* yacc.c:1646  */
    { mark_completion_tag (TYPE_CODE_ENUM, (yyvsp[-1].sval).ptr, (yyvsp[-1].sval).length);
		  (yyval.tval) = NULL; }
#line 2175 "d-exp.c" /* yacc.c:1646  */
    break;


#line 2179 "d-exp.c" /* yacc.c:1646  */
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

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

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
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
#line 642 "d-exp.y" /* yacc.c:1906  */


/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *ps, const char *p,
	      int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;
  ULONGEST un;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      const struct builtin_d_type *builtin_d_types;
      const char *suffix;
      int suffix_len;
      char *s, *sp;

      /* Strip out all embedded '_' before passing to parse_float.  */
      s = (char *) alloca (len + 1);
      sp = s;
      while (len-- > 0)
	{
	  if (*p != '_')
	    *sp++ = *p;
	  p++;
	}
      *sp = '\0';
      len = strlen (s);

      if (! parse_float (s, len, &putithere->typed_val_float.dval, &suffix))
	return ERROR;

      suffix_len = s + len - suffix;

      if (suffix_len == 0)
	{
	  putithere->typed_val_float.type
	    = parse_d_type (ps)->builtin_double;
	}
      else if (suffix_len == 1)
	{
	  /* Check suffix for `f', `l', or `i' (float, real, or idouble).  */
	  if (tolower (*suffix) == 'f')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_float;
	    }
	  else if (tolower (*suffix) == 'l')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_real;
	    }
	  else if (tolower (*suffix) == 'i')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_idouble;
	    }
	  else
	    return ERROR;
	}
      else if (suffix_len == 2)
	{
	  /* Check suffix for `fi' or `li' (ifloat or ireal).  */
	  if (tolower (suffix[0]) == 'f' && tolower (suffix[1] == 'i'))
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_ifloat;
	    }
	  else if (tolower (suffix[0]) == 'l' && tolower (suffix[1] == 'i'))
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_ireal;
	    }
	  else
	    return ERROR;
	}
      else
	return ERROR;

      return FLOAT_LITERAL;
    }

  /* Handle base-switching prefixes 0x, 0b, 0 */
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

      default:
	base = 8;
	break;
      }

  while (len-- > 0)
    {
      c = *p++;
      if (c == '_')
	continue;	/* Ignore embedded '_'.  */
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
	  else if (c == 'l' && long_p == 0)
	    {
	      long_p = 1;
	      found_suffix = 1;
	    }
	  else if (c == 'u' && unsigned_p == 0)
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base.  */
      /* Portably test for integer overflow.  */
      if (c != 'l' && c != 'u')
	{
	  ULONGEST n2 = prevn * base;
	  if ((n2 / base != prevn) || (n2 + i < prevn))
	    error (_("Numeric constant too large."));
	}
      prevn = n;
    }

  /* An integer constant is an int or a long.  An L suffix forces it to
     be long, and a U suffix forces it to be unsigned.  To figure out
     whether it fits, we shift it right and see whether anything remains.
     Note that we can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or
     more in one operation, because many compilers will warn about such a
     shift (which always produces a zero result).  To deal with the case
     where it is we just always shift the value more than once, with fewer
     bits each time.  */
  un = (ULONGEST) n >> 2;
  if (long_p == 0 && (un >> 30) == 0)
    {
      high_bit = ((ULONGEST) 1) << 31;
      signed_type = parse_d_type (ps)->builtin_int;
      /* For decimal notation, keep the sign of the worked out type.  */
      if (base == 10 && !unsigned_p)
	unsigned_type = parse_d_type (ps)->builtin_long;
      else
	unsigned_type = parse_d_type (ps)->builtin_uint;
    }
  else
    {
      int shift;
      if (sizeof (ULONGEST) * HOST_CHAR_BIT < 64)
	/* A long long does not fit in a LONGEST.  */
	shift = (sizeof (ULONGEST) * HOST_CHAR_BIT - 1);
      else
	shift = 63;
      high_bit = (ULONGEST) 1 << shift;
      signed_type = parse_d_type (ps)->builtin_long;
      unsigned_type = parse_d_type (ps)->builtin_ulong;
    }

  putithere->typed_val_int.val = n;

  /* If the high bit of the worked out type is set then this number
     has to be unsigned_type.  */
  if (unsigned_p || (n & high_bit))
    putithere->typed_val_int.type = unsigned_type;
  else
    putithere->typed_val_int.type = signed_type;

  return INTEGER_LITERAL;
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
parse_string_or_char (const char *tokptr, const char **outptr,
		      struct typed_stoken *value, int *host_chars)
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
      if (quote == '"' || quote == '`')
	error (_("Unterminated string in expression."));
      else
	error (_("Unmatched single quote."));
    }
  ++tokptr;

  /* FIXME: should instead use own language string_type enum
     and handle D-specific string suffixes here. */
  if (quote == '\'')
    value->type = C_CHAR;
  else
    value->type = C_STRING;

  value->ptr = obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '\'' ? CHARACTER_LITERAL : STRING_LITERAL;
}

struct token
{
  char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {"^^=", ASSIGN_MODIFY, BINOP_EXP},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH},
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
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
    {"&&", ANDAND, BINOP_END},
    {"||", OROR, BINOP_END},
    {"^^", HATHAT, BINOP_END},
    {"<<", LSH, BINOP_END},
    {">>", RSH, BINOP_END},
    {"==", EQUAL, BINOP_END},
    {"!=", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END},
    {"..", DOTDOT, BINOP_END},
  };

/* Identifier-like tokens.  */
static const struct token ident_tokens[] =
  {
    {"is", IDENTITY, BINOP_END},
    {"!is", NOTIDENTITY, BINOP_END},

    {"cast", CAST_KEYWORD, OP_NULL},
    {"const", CONST_KEYWORD, OP_NULL},
    {"immutable", IMMUTABLE_KEYWORD, OP_NULL},
    {"shared", SHARED_KEYWORD, OP_NULL},
    {"super", SUPER_KEYWORD, OP_NULL},

    {"null", NULL_KEYWORD, OP_NULL},
    {"true", TRUE_KEYWORD, OP_NULL},
    {"false", FALSE_KEYWORD, OP_NULL},

    {"init", INIT_KEYWORD, OP_NULL},
    {"sizeof", SIZEOF_KEYWORD, OP_NULL},
    {"typeof", TYPEOF_KEYWORD, OP_NULL},
    {"typeid", TYPEID_KEYWORD, OP_NULL},

    {"delegate", DELEGATE_KEYWORD, OP_NULL},
    {"function", FUNCTION_KEYWORD, OP_NULL},
    {"struct", STRUCT_KEYWORD, OP_NULL},
    {"union", UNION_KEYWORD, OP_NULL},
    {"class", CLASS_KEYWORD, OP_NULL},
    {"interface", INTERFACE_KEYWORD, OP_NULL},
    {"enum", ENUM_KEYWORD, OP_NULL},
    {"template", TEMPLATE_KEYWORD, OP_NULL},
  };

/* If NAME is a type name in this scope, return it.  */

static struct type *
d_type_from_name (struct stoken name)
{
  struct symbol *sym;
  char *copy = copy_name (name);

  sym = lookup_symbol (copy, expression_context_block,
		       STRUCT_DOMAIN, NULL);
  if (sym != NULL)
    return SYMBOL_TYPE (sym);

  return NULL;
}

/* If NAME is a module name in this scope, return it.  */

static struct type *
d_module_from_name (struct stoken name)
{
  struct symbol *sym;
  char *copy = copy_name (name);

  sym = lookup_symbol (copy, expression_context_block,
		       MODULE_DOMAIN, NULL);
  if (sym != NULL)
    return SYMBOL_TYPE (sym);

  return NULL;
}

/* If NAME is a valid variable name in this scope, push it and return 1.
   Otherwise, return 0.  */

static int
push_variable (struct parser_state *ps, struct stoken name)
{
  char *copy = copy_name (name);
  struct field_of_this_result is_a_field_of_this;
  struct symbol *sym;
  sym = lookup_symbol (copy, expression_context_block, VAR_DOMAIN,
                       &is_a_field_of_this);
  if (sym && SYMBOL_CLASS (sym) != LOC_TYPEDEF)
    {
      if (symbol_read_needs_frame (sym))
        {
          if (innermost_block == 0 ||
              contained_in (block_found, innermost_block))
            innermost_block = block_found;
        }

      write_exp_elt_opcode (ps, OP_VAR_VALUE);
      /* We want to use the selected frame, not another more inner frame
         which happens to be in the same block.  */
      write_exp_elt_block (ps, NULL);
      write_exp_elt_sym (ps, sym);
      write_exp_elt_opcode (ps, OP_VAR_VALUE);
      return 1;
    }
  if (is_a_field_of_this.type != NULL)
    {
      /* It hangs off of `this'.  Must not inadvertently convert from a
         method call to data ref.  */
      if (innermost_block == 0 ||
          contained_in (block_found, innermost_block))
        innermost_block = block_found;
      write_exp_elt_opcode (ps, OP_THIS);
      write_exp_elt_opcode (ps, OP_THIS);
      write_exp_elt_opcode (ps, STRUCTOP_PTR);
      write_exp_string (ps, name);
      write_exp_elt_opcode (ps, STRUCTOP_PTR);
      return 1;
    }
  return 0;
}

/* Assuming a reference expression has been pushed, emit the
   STRUCTOP_PTR ops to access the field named NAME.  If NAME is a
   qualified name (has '.'), generate a field access for each part.  */

static void
push_fieldnames (struct parser_state *ps, struct stoken name)
{
  int i;
  struct stoken token;
  token.ptr = name.ptr;
  for (i = 0;  ;  i++)
    {
      if (i == name.length || name.ptr[i] == '.')
        {
          /* token.ptr is start of current field name.  */
          token.length = &name.ptr[i] - token.ptr;
          write_exp_elt_opcode (ps, STRUCTOP_PTR);
          write_exp_string (ps, token);
          write_exp_elt_opcode (ps, STRUCTOP_PTR);
          token.ptr += token.length + 1;
        }
      if (i >= name.length)
        break;
    }
}

/* Helper routine for push_expression_name.  Handle a TYPE symbol,
   where DOT_INDEX is the index of the first '.' if NAME is part of
   a qualified type.  */

static void
push_type_name (struct parser_state *ps, struct type *type,
		struct stoken name, int dot_index)
{
  if (dot_index == name.length)
    {
      write_exp_elt_opcode (ps, OP_TYPE);
      write_exp_elt_type (ps, type);
      write_exp_elt_opcode (ps, OP_TYPE);
    }
  else
    {
      struct stoken token;

      token.ptr = name.ptr;
      token.length = dot_index;

      dot_index = 0;

      while (dot_index < name.length && name.ptr[dot_index] != '.')
	dot_index++;
      token.ptr = name.ptr;
      token.length = dot_index;

      write_exp_elt_opcode (ps, OP_SCOPE);
      write_exp_elt_type (ps, type);
      write_exp_string (ps, token);
      write_exp_elt_opcode (ps, OP_SCOPE);

      if (dot_index < name.length)
	{
	  dot_index++;
	  name.ptr += dot_index;
	  name.length -= dot_index;
	  push_fieldnames (ps, name);
	}
    }
}

/* Helper routine for push_expression_name.  Like push_type_name,
   but used when TYPE is a module.  Returns 1 on pushing the symbol.  */

static int
push_module_name (struct parser_state *ps, struct type *module,
		  struct stoken name, int dot_index)
{
  if (dot_index == name.length)
    {
      write_exp_elt_opcode (ps, OP_TYPE);
      write_exp_elt_type (ps, module);
      write_exp_elt_opcode (ps, OP_TYPE);
      return 1;
    }
  else
    {
      struct symbol *sym;
      char *copy;

      copy = copy_name (name);
      sym = lookup_symbol_in_static_block (copy, expression_context_block,
					   VAR_DOMAIN);
      if (sym != NULL)
	sym = lookup_global_symbol (copy, expression_context_block,
				    VAR_DOMAIN);

      if (sym != NULL)
	{
	  if (SYMBOL_CLASS (sym) != LOC_TYPEDEF)
	    {
	      write_exp_elt_opcode (ps, OP_VAR_VALUE);
	      write_exp_elt_block (ps, NULL);
	      write_exp_elt_sym (ps, sym);
	      write_exp_elt_opcode (ps, OP_VAR_VALUE);
	    }
	  else
	    {
	      write_exp_elt_opcode (ps, OP_TYPE);
	      write_exp_elt_type (ps, SYMBOL_TYPE (sym));
	      write_exp_elt_opcode (ps, OP_TYPE);
	    }
	  return 1;
	}
    }

  return 0;
}

/* Handle NAME in an expression (or LHS), which could be a
   variable, type, or module.  */

static void
push_expression_name (struct parser_state *ps, struct stoken name)
{
  struct stoken token;
  struct type *typ;
  struct bound_minimal_symbol msymbol;
  char *copy;
  int doti;

  /* Handle VAR, which could be local or global.  */
  if (push_variable (ps, name) != 0)
    return;

  /* Handle MODULE.  */
  typ = d_module_from_name (name);
  if (typ != NULL)
    {
      if (push_module_name (ps, typ, name, name.length) != 0)
	return;
    }

  /* Handle TYPE.  */
  typ = d_type_from_name (name);
  if (typ != NULL)
    {
      push_type_name (ps, typ, name, name.length);
      return;
    }

  /* Handle VAR.FIELD1..FIELDN.  */
  for (doti = 0;  doti < name.length;  doti++)
    {
      if (name.ptr[doti] == '.')
	{
	  token.ptr = name.ptr;
	  token.length = doti;

	  if (push_variable (ps, token) != 0)
	    {
	      token.ptr = name.ptr + doti + 1;
	      token.length = name.length - doti - 1;
	      push_fieldnames (ps, token);
	      return;
	    }
	  break;
	}
    }

  /* Continue looking if we found a '.' in the name.  */
  if (doti < name.length)
    {
      token.ptr = name.ptr;
      for (;;)
	{
	  token.length = doti;

	  /* Handle PACKAGE.MODULE.  */
	  typ = d_module_from_name (token);
	  if (typ != NULL)
	    {
	      if (push_module_name (ps, typ, name, doti) != 0)
		return;
	    }
	  /* Handle TYPE.FIELD1..FIELDN.  */
	  typ = d_type_from_name (token);
	  if (typ != NULL)
	    {
	      push_type_name (ps, typ, name, doti);
	      return;
	    }

	  if (doti >= name.length)
	    break;
	  doti++;   /* Skip '.'  */
	  while (doti < name.length && name.ptr[doti] != '.')
	    doti++;
	}
    }

  /* Lookup foreign name in global static symbols.  */
  copy  = copy_name (name);
  msymbol = lookup_bound_minimal_symbol (copy);
  if (msymbol.minsym != NULL)
    write_exp_msymbol (ps, msymbol);
  else if (!have_full_symbols () && !have_partial_symbols ())
    error (_("No symbol table is loaded.  Use the \"file\" command"));
  else
    error (_("No symbol \"%s\" in current context."), copy);
}

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure operator.
   This is used only when parsing to do field name completion.  */
static int last_was_structop;

/* Read one token, getting characters through lexptr.  */

static int
yylex (void)
{
  int c;
  int namelen;
  unsigned int i;
  const char *tokstart;
  int saw_structop = last_was_structop;
  char *copy;

  last_was_structop = 0;

 retry:

  prev_lexptr = lexptr;

  tokstart = lexptr;
  /* See if it is a special token of length 3.  */
  for (i = 0; i < sizeof tokentab3 / sizeof tokentab3[0]; i++)
    if (strncmp (tokstart, tokentab3[i].oper, 3) == 0)
      {
	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof tokentab2 / sizeof tokentab2[0]; i++)
    if (strncmp (tokstart, tokentab2[i].oper, 2) == 0)
      {
	lexptr += 2;
	yylval.opcode = tokentab2[i].opcode;
	return tokentab2[i].token;
      }

  switch (c = *tokstart)
    {
    case 0:
      /* If we're parsing for field name completion, and the previous
	 token allows such completion, return a COMPLETE token.
	 Otherwise, we were already scanning the original text, and
	 we're really done.  */
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
      if (comma_terminates && paren_depth == 0)
	return 0;
      lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (lexptr[1] < '0' || lexptr[1] > '9')
	{
	  if (parse_completion)
	    last_was_structop = 1;
	  goto symbol;		/* Nope, must be a symbol.  */
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

	for (;; ++p)
	  {
	    /* Hex exponents start with 'p', because 'e' is a valid hex
	       digit and thus does not indicate a floating point number
	       when the radix is hex.  */
	    if ((!hex && !got_e && tolower (p[0]) == 'e')
		|| (hex && !got_e && tolower (p[0] == 'p')))
	      got_dot = got_e = 1;
	    /* A '.' always indicates a decimal floating point number
	       regardless of the radix.  If we have a '..' then its the
	       end of the number and the beginning of a slice.  */
	    else if (!got_dot && (p[0] == '.' && p[1] != '.'))
		got_dot = 1;
	    /* This is the sign of the exponent, not the end of the number.  */
	    else if (got_e && (tolower (p[-1]) == 'e' || tolower (p[-1]) == 'p')
		     && (*p == '-' || *p == '+'))
	      continue;
	    /* We will take any letters or digits, ignoring any embedded '_'.
	       parse_number will complain if past the radix, or if L or U are
	       not final.  */
	    else if ((*p < '0' || *p > '9') && (*p != '_') &&
		     ((*p < 'a' || *p > 'z') && (*p < 'A' || *p > 'Z')))
	      break;
	  }

	toktype = parse_number (pstate, tokstart, p - tokstart,
				got_dot|got_e, &yylval);
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
	if (result == CHARACTER_LITERAL)
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
    error (_("Invalid character '%c' in expression"), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));)
    c = tokstart[++namelen];

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    return 0;

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.  "task" is similar.  Handle abbreviations of these,
     similarly to breakpoint.c:find_condition_and_thread.  */
  if (namelen >= 1
      && (strncmp (tokstart, "thread", namelen) == 0
	  || strncmp (tokstart, "task", namelen) == 0)
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t'))
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
    if (strcmp (copy, ident_tokens[i].oper) == 0)
      {
	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = ident_tokens[i].opcode;
	return ident_tokens[i].token;
      }

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  yylval.tsym.type
    = language_lookup_primitive_type (parse_language (pstate),
				      parse_gdbarch (pstate), copy);
  if (yylval.tsym.type != NULL)
    return TYPENAME;

  /* Input names that aren't symbols but ARE valid hex numbers,
     when the input radix permits them, can be names or numbers
     depending on the parse.  Note we support radixes > 16 here.  */
  if ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
      || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (pstate, tokstart, namelen, 0, &newlval);
      if (hextype == INTEGER_LITERAL)
	return NAME_OR_INT;
    }

  if (parse_completion && *lexptr == '\0')
    saw_name_at_eof = 1;

  return IDENTIFIER;
}

int
d_parse (struct parser_state *par_state)
{
  int result;
  struct cleanup *back_to;

  /* Setting up the parser state.  */
  gdb_assert (par_state != NULL);
  pstate = par_state;

  back_to = make_cleanup (null_cleanup, NULL);

  make_cleanup_restore_integer (&yydebug);
  make_cleanup_clear_parser_state (&pstate);
  yydebug = parser_debug;

  /* Initialize some state used by the lexer.  */
  last_was_structop = 0;
  saw_name_at_eof = 0;

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

