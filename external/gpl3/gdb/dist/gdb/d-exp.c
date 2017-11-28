/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.4"

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

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX d_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

void yyerror (const char *);

static int type_aggregate_p (struct type *);


#line 105 "d-exp.c" /* yacc.c:339  */

# ifndef YY_NULLPTRPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTRPTR nullptr
#  else
#   define YY_NULLPTRPTR 0
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
    UNKNOWN_NAME = 259,
    TYPENAME = 260,
    COMPLETE = 261,
    NAME_OR_INT = 262,
    INTEGER_LITERAL = 263,
    FLOAT_LITERAL = 264,
    CHARACTER_LITERAL = 265,
    STRING_LITERAL = 266,
    ENTRY = 267,
    ERROR = 268,
    TRUE_KEYWORD = 269,
    FALSE_KEYWORD = 270,
    NULL_KEYWORD = 271,
    SUPER_KEYWORD = 272,
    CAST_KEYWORD = 273,
    SIZEOF_KEYWORD = 274,
    TYPEOF_KEYWORD = 275,
    TYPEID_KEYWORD = 276,
    INIT_KEYWORD = 277,
    IMMUTABLE_KEYWORD = 278,
    CONST_KEYWORD = 279,
    SHARED_KEYWORD = 280,
    STRUCT_KEYWORD = 281,
    UNION_KEYWORD = 282,
    CLASS_KEYWORD = 283,
    INTERFACE_KEYWORD = 284,
    ENUM_KEYWORD = 285,
    TEMPLATE_KEYWORD = 286,
    DELEGATE_KEYWORD = 287,
    FUNCTION_KEYWORD = 288,
    DOLLAR_VARIABLE = 289,
    ASSIGN_MODIFY = 290,
    OROR = 291,
    ANDAND = 292,
    EQUAL = 293,
    NOTEQUAL = 294,
    LEQ = 295,
    GEQ = 296,
    LSH = 297,
    RSH = 298,
    HATHAT = 299,
    IDENTITY = 300,
    NOTIDENTITY = 301,
    INCREMENT = 302,
    DECREMENT = 303,
    DOTDOT = 304
  };
#endif
/* Tokens.  */
#define IDENTIFIER 258
#define UNKNOWN_NAME 259
#define TYPENAME 260
#define COMPLETE 261
#define NAME_OR_INT 262
#define INTEGER_LITERAL 263
#define FLOAT_LITERAL 264
#define CHARACTER_LITERAL 265
#define STRING_LITERAL 266
#define ENTRY 267
#define ERROR 268
#define TRUE_KEYWORD 269
#define FALSE_KEYWORD 270
#define NULL_KEYWORD 271
#define SUPER_KEYWORD 272
#define CAST_KEYWORD 273
#define SIZEOF_KEYWORD 274
#define TYPEOF_KEYWORD 275
#define TYPEID_KEYWORD 276
#define INIT_KEYWORD 277
#define IMMUTABLE_KEYWORD 278
#define CONST_KEYWORD 279
#define SHARED_KEYWORD 280
#define STRUCT_KEYWORD 281
#define UNION_KEYWORD 282
#define CLASS_KEYWORD 283
#define INTERFACE_KEYWORD 284
#define ENUM_KEYWORD 285
#define TEMPLATE_KEYWORD 286
#define DELEGATE_KEYWORD 287
#define FUNCTION_KEYWORD 288
#define DOLLAR_VARIABLE 289
#define ASSIGN_MODIFY 290
#define OROR 291
#define ANDAND 292
#define EQUAL 293
#define NOTEQUAL 294
#define LEQ 295
#define GEQ 296
#define LSH 297
#define RSH 298
#define HATHAT 299
#define IDENTITY 300
#define NOTIDENTITY 301
#define INCREMENT 302
#define DECREMENT 303
#define DOTDOT 304

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 83 "d-exp.y" /* yacc.c:355  */

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
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;
    struct stoken_vector svec;
  

#line 263 "d-exp.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */
#line 105 "d-exp.y" /* yacc.c:358  */

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *,
			 int, int, YYSTYPE *);

#line 285 "d-exp.c" /* yacc.c:358  */

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
#define YYFINAL  70
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   200

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  71
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  104
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  169

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   304

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    68,     2,     2,     2,    56,    43,     2,
      64,    69,    54,    52,    36,    53,    62,    55,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,     2,
      46,    37,    47,    38,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    63,     2,    70,    42,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    41,     2,    67,     2,     2,     2,
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
      35,    39,    40,    44,    45,    48,    49,    50,    51,    57,
      58,    59,    60,    61,    65
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   178,   178,   179,   185,   189,   190,   195,   196,   198,
     205,   206,   211,   212,   217,   218,   223,   224,   229,   230,
     235,   236,   241,   242,   243,   244,   248,   250,   255,   257,
     262,   264,   266,   268,   273,   274,   276,   281,   282,   284,
     286,   291,   292,   294,   296,   300,   302,   304,   306,   308,
     310,   312,   314,   316,   318,   319,   323,   327,   333,   334,
     339,   340,   348,   352,   357,   359,   361,   363,   364,   365,
     369,   371,   377,   378,   383,   382,   391,   404,   406,   411,
     413,   461,   512,   514,   522,   529,   533,   537,   542,   547,
     552,   558,   563,   568,   573,   577,   592,   610,   612,   616,
     625,   627,   629,   632,   638
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENTIFIER", "UNKNOWN_NAME", "TYPENAME",
  "COMPLETE", "NAME_OR_INT", "INTEGER_LITERAL", "FLOAT_LITERAL",
  "CHARACTER_LITERAL", "STRING_LITERAL", "ENTRY", "ERROR", "TRUE_KEYWORD",
  "FALSE_KEYWORD", "NULL_KEYWORD", "SUPER_KEYWORD", "CAST_KEYWORD",
  "SIZEOF_KEYWORD", "TYPEOF_KEYWORD", "TYPEID_KEYWORD", "INIT_KEYWORD",
  "IMMUTABLE_KEYWORD", "CONST_KEYWORD", "SHARED_KEYWORD", "STRUCT_KEYWORD",
  "UNION_KEYWORD", "CLASS_KEYWORD", "INTERFACE_KEYWORD", "ENUM_KEYWORD",
  "TEMPLATE_KEYWORD", "DELEGATE_KEYWORD", "FUNCTION_KEYWORD",
  "DOLLAR_VARIABLE", "ASSIGN_MODIFY", "','", "'='", "'?'", "OROR",
  "ANDAND", "'|'", "'^'", "'&'", "EQUAL", "NOTEQUAL", "'<'", "'>'", "LEQ",
  "GEQ", "LSH", "RSH", "'+'", "'-'", "'*'", "'/'", "'%'", "HATHAT",
  "IDENTITY", "NOTIDENTITY", "INCREMENT", "DECREMENT", "'.'", "'['", "'('",
  "DOTDOT", "':'", "'~'", "'!'", "')'", "']'", "$accept", "start",
  "Expression", "CommaExpression", "AssignExpression",
  "ConditionalExpression", "OrOrExpression", "AndAndExpression",
  "OrExpression", "XorExpression", "AndExpression", "CmpExpression",
  "EqualExpression", "IdentityExpression", "RelExpression",
  "ShiftExpression", "AddExpression", "MulExpression", "UnaryExpression",
  "CastExpression", "PowExpression", "PostfixExpression", "ArgumentList",
  "ArgumentList_opt", "CallExpression", "$@1", "IndexExpression",
  "SliceExpression", "PrimaryExpression", "ArrayLiteral", "IdentifierExp",
  "StringExp", "TypeExp", "BasicType2", "BasicType", YY_NULLPTRPTR
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
     285,   286,   287,   288,   289,   290,    44,    61,    63,   291,
     292,   124,    94,    38,   293,   294,    60,    62,   295,   296,
     297,   298,    43,    45,    42,    47,    37,   299,   300,   301,
     302,   303,    46,    91,    40,   304,    58,   126,    33,    41,
      93
};
# endif

#define YYPACT_NINF -91

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-91)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     132,   -91,   -91,   -91,   -91,   -91,   -91,   -91,   -91,   -91,
     -91,   -43,   -27,   -91,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,    30,   -91,   -91,    17,    -8,     0,
      14,    21,    35,    12,   -91,   -91,   -91,   -91,    -1,   -34,
     -31,   -91,   -91,   -91,    43,   -91,   -91,   -91,   -91,   -91,
     -91,    72,    22,   -37,     6,   132,   -91,    22,   -91,   -91,
     -91,   -91,   -91,   -91,    49,    16,    19,   -49,   -91,   -91,
     -91,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   -91,   -91,     9,
      58,   -91,   -91,    13,   -37,    81,   -91,     6,    24,    25,
     132,   -91,   -91,   132,   -91,   -91,   -91,    29,    14,    21,
      35,    12,   -91,    -9,    -9,    -9,    -9,    -9,    -9,   -34,
     -34,    -9,    -9,   -31,   -31,   -31,   -91,   -91,   -91,   -91,
      84,   -91,   -91,   -91,    26,   -30,   132,   -91,   -91,   -91,
      28,    27,   132,   -91,   -91,   -91,   132,   -91,   132,   -91,
      33,   -37,   -91,   -91,   -91,    38,   -91,   -91,   -91
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    94,   104,    83,    87,    88,    89,    95,    85,    86,
      84,     0,     0,    82,     0,     0,     0,     0,     0,     0,
      72,     0,     0,     0,     0,     2,     4,     5,     7,    10,
      12,    14,    16,    18,    20,    23,    24,    25,    22,    34,
      37,    41,    54,    55,    58,    67,    68,    69,    60,    91,
      80,    90,     3,    98,     0,     0,    45,     0,    50,    49,
      48,    46,    47,    70,    73,     0,     0,     0,    52,    51,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    65,    66,     0,
       0,    74,    96,     0,   100,     0,    99,     0,     0,     0,
       0,    93,    79,    97,     6,     9,     8,     0,    13,    15,
      17,    19,    21,    26,    27,    30,    32,    31,    33,    35,
      36,    28,    29,    38,    39,    40,    42,    43,    44,    59,
      62,    61,    64,    77,    70,     0,    72,    53,    81,   101,
       0,     0,     0,    92,    71,    57,     0,    63,     0,    76,
       0,   102,    97,    56,    11,     0,    75,   103,    78
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -91,   -91,     1,    42,   -13,   -47,   -91,    39,    40,    46,
      37,    41,   -91,   -91,   -91,    73,   -35,   -56,   -14,   -91,
     -91,   -91,    31,   -22,   -91,   -91,   -91,   -91,   -91,   -91,
      48,   -91,    10,   -90,   -91
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    64,    65,    45,   146,    46,    47,    48,    49,
      50,    51,    57,   106,    53
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      56,    58,    59,    60,    61,    62,   110,    63,    68,    69,
      52,     2,   140,   103,   149,   141,     1,   104,    90,    91,
     113,    54,    66,    93,    94,    95,   105,    72,   142,    73,
      70,    67,   147,    92,   133,   134,   135,    55,    74,    75,
     159,    86,    87,    80,    81,    82,    83,    84,    85,    86,
      87,   129,   130,    71,    76,    79,   109,    88,    89,   115,
     116,     1,    77,     2,   108,     3,     4,     5,     6,     7,
     107,   167,     8,     9,    10,   117,    11,    78,    12,   136,
     137,   138,   139,   102,   103,   110,   111,   144,   112,   150,
     157,   158,    13,   152,   153,   156,   162,   154,   161,   155,
      96,    14,   166,    97,    98,    99,   100,   101,   168,   164,
      15,    16,    17,   114,   118,   121,   119,   151,    18,    19,
     122,    20,    21,   120,   160,    22,    23,     0,   143,     0,
       0,   145,     0,    63,     0,     1,     0,     2,   163,     3,
       4,     5,     6,     7,     0,   165,     8,     9,    10,     0,
      11,   148,    12,   123,   124,   125,   126,   127,   128,     0,
       0,   131,   132,     0,     0,     0,    13,     0,     0,     0,
       0,     0,     0,     0,     0,    14,     0,     0,     0,     0,
       0,     0,     0,     0,    15,    16,    17,     0,     0,     0,
       0,     0,    18,    19,     0,    20,    21,     0,     0,    22,
      23
};

static const yytype_int16 yycheck[] =
{
      14,    15,    16,    17,    18,    19,    36,    20,    22,    23,
       0,     5,     3,    62,   104,     6,     3,    54,    52,    53,
      69,    64,    21,    54,    55,    56,    63,    35,    19,    37,
       0,    21,    19,    67,    90,    91,    92,    64,    38,    39,
      70,    50,    51,    44,    45,    46,    47,    48,    49,    50,
      51,    86,    87,    36,    40,    43,    55,    58,    59,    72,
      73,     3,    41,     5,    54,     7,     8,     9,    10,    11,
      64,   161,    14,    15,    16,    74,    18,    42,    20,    93,
      94,    95,    96,    11,    62,    36,    70,   100,    69,     8,
       6,    65,    34,    69,    69,    66,    69,   110,    70,   113,
      57,    43,    69,    60,    61,    62,    63,    64,    70,   156,
      52,    53,    54,    71,    75,    78,    76,   107,    60,    61,
      79,    63,    64,    77,   146,    67,    68,    -1,    70,    -1,
      -1,   100,    -1,   146,    -1,     3,    -1,     5,   152,     7,
       8,     9,    10,    11,    -1,   158,    14,    15,    16,    -1,
      18,   103,    20,    80,    81,    82,    83,    84,    85,    -1,
      -1,    88,    89,    -1,    -1,    -1,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,    -1,
      -1,    -1,    60,    61,    -1,    63,    64,    -1,    -1,    67,
      68
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     7,     8,     9,    10,    11,    14,    15,
      16,    18,    20,    34,    43,    52,    53,    54,    60,    61,
      63,    64,    67,    68,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    95,    97,    98,    99,   100,
     101,   102,   103,   105,    64,    64,    89,   103,    89,    89,
      89,    89,    89,    75,    93,    94,    73,   103,    89,    89,
       0,    36,    35,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    58,    59,
      52,    53,    67,    54,    55,    56,    57,    60,    61,    62,
      63,    64,    11,    62,    54,    63,   104,    64,   103,    73,
      36,    70,    69,    69,    74,    75,    75,    73,    78,    79,
      80,    81,    82,    86,    86,    86,    86,    86,    86,    87,
      87,    86,    86,    88,    88,    88,    89,    89,    89,    89,
       3,     6,    19,    70,    75,    93,    96,    19,   101,   104,
       8,   103,    69,    69,    75,    89,    66,     6,    65,    70,
      94,    70,    69,    89,    76,    75,    69,   104,    70
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    71,    72,    72,    73,    74,    74,    75,    75,    75,
      76,    76,    77,    77,    78,    78,    79,    79,    80,    80,
      81,    81,    82,    82,    82,    82,    83,    83,    84,    84,
      85,    85,    85,    85,    86,    86,    86,    87,    87,    87,
      87,    88,    88,    88,    88,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    90,    90,    91,    91,
      92,    92,    92,    92,    92,    92,    92,    92,    92,    92,
      93,    93,    94,    94,    96,    95,    97,    98,    98,    99,
      99,    99,    99,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,   100,   101,   102,   102,   103,   103,   103,
     104,   104,   104,   104,   105
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     3,     1,     3,     3,
       1,     5,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     1,     1,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       3,     1,     3,     3,     3,     2,     2,     2,     2,     2,
       2,     2,     2,     3,     1,     1,     5,     4,     1,     3,
       1,     3,     3,     4,     3,     2,     2,     1,     1,     1,
       1,     3,     0,     1,     0,     5,     4,     3,     6,     3,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     4,     3,     1,     1,     2,     3,     1,     2,
       1,     2,     3,     4,     1
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTRPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTRPTR;
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTRPTR, yytname[yyx]);
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
#line 191 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_COMMA); }
#line 1506 "d-exp.c" /* yacc.c:1646  */
    break;

  case 8:
#line 197 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
#line 1512 "d-exp.c" /* yacc.c:1646  */
    break;

  case 9:
#line 199 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
		  write_exp_elt_opcode (pstate, (yyvsp[-1].opcode));
		  write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY); }
#line 1520 "d-exp.c" /* yacc.c:1646  */
    break;

  case 11:
#line 207 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_COND); }
#line 1526 "d-exp.c" /* yacc.c:1646  */
    break;

  case 13:
#line 213 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
#line 1532 "d-exp.c" /* yacc.c:1646  */
    break;

  case 15:
#line 219 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
#line 1538 "d-exp.c" /* yacc.c:1646  */
    break;

  case 17:
#line 225 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
#line 1544 "d-exp.c" /* yacc.c:1646  */
    break;

  case 19:
#line 231 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
#line 1550 "d-exp.c" /* yacc.c:1646  */
    break;

  case 21:
#line 237 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
#line 1556 "d-exp.c" /* yacc.c:1646  */
    break;

  case 26:
#line 249 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL); }
#line 1562 "d-exp.c" /* yacc.c:1646  */
    break;

  case 27:
#line 251 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
#line 1568 "d-exp.c" /* yacc.c:1646  */
    break;

  case 28:
#line 256 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL); }
#line 1574 "d-exp.c" /* yacc.c:1646  */
    break;

  case 29:
#line 258 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
#line 1580 "d-exp.c" /* yacc.c:1646  */
    break;

  case 30:
#line 263 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LESS); }
#line 1586 "d-exp.c" /* yacc.c:1646  */
    break;

  case 31:
#line 265 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LEQ); }
#line 1592 "d-exp.c" /* yacc.c:1646  */
    break;

  case 32:
#line 267 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GTR); }
#line 1598 "d-exp.c" /* yacc.c:1646  */
    break;

  case 33:
#line 269 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GEQ); }
#line 1604 "d-exp.c" /* yacc.c:1646  */
    break;

  case 35:
#line 275 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LSH); }
#line 1610 "d-exp.c" /* yacc.c:1646  */
    break;

  case 36:
#line 277 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_RSH); }
#line 1616 "d-exp.c" /* yacc.c:1646  */
    break;

  case 38:
#line 283 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ADD); }
#line 1622 "d-exp.c" /* yacc.c:1646  */
    break;

  case 39:
#line 285 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUB); }
#line 1628 "d-exp.c" /* yacc.c:1646  */
    break;

  case 40:
#line 287 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_CONCAT); }
#line 1634 "d-exp.c" /* yacc.c:1646  */
    break;

  case 42:
#line 293 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_MUL); }
#line 1640 "d-exp.c" /* yacc.c:1646  */
    break;

  case 43:
#line 295 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_DIV); }
#line 1646 "d-exp.c" /* yacc.c:1646  */
    break;

  case 44:
#line 297 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REM); }
#line 1652 "d-exp.c" /* yacc.c:1646  */
    break;

  case 45:
#line 301 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ADDR); }
#line 1658 "d-exp.c" /* yacc.c:1646  */
    break;

  case 46:
#line 303 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREINCREMENT); }
#line 1664 "d-exp.c" /* yacc.c:1646  */
    break;

  case 47:
#line 305 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREDECREMENT); }
#line 1670 "d-exp.c" /* yacc.c:1646  */
    break;

  case 48:
#line 307 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_IND); }
#line 1676 "d-exp.c" /* yacc.c:1646  */
    break;

  case 49:
#line 309 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_NEG); }
#line 1682 "d-exp.c" /* yacc.c:1646  */
    break;

  case 50:
#line 311 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PLUS); }
#line 1688 "d-exp.c" /* yacc.c:1646  */
    break;

  case 51:
#line 313 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
#line 1694 "d-exp.c" /* yacc.c:1646  */
    break;

  case 52:
#line 315 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_COMPLEMENT); }
#line 1700 "d-exp.c" /* yacc.c:1646  */
    break;

  case 53:
#line 317 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_SIZEOF); }
#line 1706 "d-exp.c" /* yacc.c:1646  */
    break;

  case 56:
#line 324 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST_TYPE); }
#line 1712 "d-exp.c" /* yacc.c:1646  */
    break;

  case 57:
#line 328 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST_TYPE); }
#line 1718 "d-exp.c" /* yacc.c:1646  */
    break;

  case 59:
#line 335 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EXP); }
#line 1724 "d-exp.c" /* yacc.c:1646  */
    break;

  case 61:
#line 341 "d-exp.y" /* yacc.c:1646  */
    { struct stoken s;
		  mark_struct_expression (pstate);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
		  s.ptr = "";
		  s.length = 0;
		  write_exp_string (pstate, s);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 1736 "d-exp.c" /* yacc.c:1646  */
    break;

  case 62:
#line 349 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
		  write_exp_string (pstate, (yyvsp[0].sval));
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 1744 "d-exp.c" /* yacc.c:1646  */
    break;

  case 63:
#line 353 "d-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
		  write_exp_string (pstate, (yyvsp[-1].sval));
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 1753 "d-exp.c" /* yacc.c:1646  */
    break;

  case 64:
#line 358 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_SIZEOF); }
#line 1759 "d-exp.c" /* yacc.c:1646  */
    break;

  case 65:
#line 360 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTINCREMENT); }
#line 1765 "d-exp.c" /* yacc.c:1646  */
    break;

  case 66:
#line 362 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTDECREMENT); }
#line 1771 "d-exp.c" /* yacc.c:1646  */
    break;

  case 70:
#line 370 "d-exp.y" /* yacc.c:1646  */
    { arglist_len = 1; }
#line 1777 "d-exp.c" /* yacc.c:1646  */
    break;

  case 71:
#line 372 "d-exp.y" /* yacc.c:1646  */
    { arglist_len++; }
#line 1783 "d-exp.c" /* yacc.c:1646  */
    break;

  case 72:
#line 377 "d-exp.y" /* yacc.c:1646  */
    { arglist_len = 0; }
#line 1789 "d-exp.c" /* yacc.c:1646  */
    break;

  case 74:
#line 383 "d-exp.y" /* yacc.c:1646  */
    { start_arglist (); }
#line 1795 "d-exp.c" /* yacc.c:1646  */
    break;

  case 75:
#line 385 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_FUNCALL);
		  write_exp_elt_longcst (pstate, (LONGEST) end_arglist ());
		  write_exp_elt_opcode (pstate, OP_FUNCALL); }
#line 1803 "d-exp.c" /* yacc.c:1646  */
    break;

  case 76:
#line 392 "d-exp.y" /* yacc.c:1646  */
    { if (arglist_len > 0)
		    {
		      write_exp_elt_opcode (pstate, MULTI_SUBSCRIPT);
		      write_exp_elt_longcst (pstate, (LONGEST) arglist_len);
		      write_exp_elt_opcode (pstate, MULTI_SUBSCRIPT);
		    }
		  else
		    write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT);
		}
#line 1817 "d-exp.c" /* yacc.c:1646  */
    break;

  case 77:
#line 405 "d-exp.y" /* yacc.c:1646  */
    { /* Do nothing.  */ }
#line 1823 "d-exp.c" /* yacc.c:1646  */
    break;

  case 78:
#line 407 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_SLICE); }
#line 1829 "d-exp.c" /* yacc.c:1646  */
    break;

  case 79:
#line 412 "d-exp.y" /* yacc.c:1646  */
    { /* Do nothing.  */ }
#line 1835 "d-exp.c" /* yacc.c:1646  */
    break;

  case 80:
#line 414 "d-exp.y" /* yacc.c:1646  */
    { struct bound_minimal_symbol msymbol;
		  char *copy = copy_name ((yyvsp[0].sval));
		  struct field_of_this_result is_a_field_of_this;
		  struct block_symbol sym;

		  /* Handle VAR, which could be local or global.  */
		  sym = lookup_symbol (copy, expression_context_block, VAR_DOMAIN,
				       &is_a_field_of_this);
		  if (sym.symbol && SYMBOL_CLASS (sym.symbol) != LOC_TYPEDEF)
		    {
		      if (symbol_read_needs_frame (sym.symbol))
			{
			  if (innermost_block == 0
			      || contained_in (sym.block, innermost_block))
			    innermost_block = sym.block;
			}

		      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
		      write_exp_elt_block (pstate, sym.block);
		      write_exp_elt_sym (pstate, sym.symbol);
		      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
		    }
		  else if (is_a_field_of_this.type != NULL)
		     {
		      /* It hangs off of `this'.  Must not inadvertently convert from a
			 method call to data ref.  */
		      if (innermost_block == 0
			  || contained_in (sym.block, innermost_block))
			innermost_block = sym.block;
		      write_exp_elt_opcode (pstate, OP_THIS);
		      write_exp_elt_opcode (pstate, OP_THIS);
		      write_exp_elt_opcode (pstate, STRUCTOP_PTR);
		      write_exp_string (pstate, (yyvsp[0].sval));
		      write_exp_elt_opcode (pstate, STRUCTOP_PTR);
		    }
		  else
		    {
		      /* Lookup foreign name in global static symbols.  */
		      msymbol = lookup_bound_minimal_symbol (copy);
		      if (msymbol.minsym != NULL)
			write_exp_msymbol (pstate, msymbol);
		      else if (!have_full_symbols () && !have_partial_symbols ())
			error (_("No symbol table is loaded.  Use the \"file\" command"));
		      else
			error (_("No symbol \"%s\" in current context."), copy);
		    }
		  }
#line 1887 "d-exp.c" /* yacc.c:1646  */
    break;

  case 81:
#line 462 "d-exp.y" /* yacc.c:1646  */
    { struct type *type = check_typedef ((yyvsp[-2].tval));

			  /* Check if the qualified name is in the global
			     context.  However if the symbol has not already
			     been resolved, it's not likely to be found.  */
			  if (TYPE_CODE (type) == TYPE_CODE_MODULE)
			    {
			      struct bound_minimal_symbol msymbol;
			      struct block_symbol sym;
			      const char *type_name = TYPE_SAFE_NAME (type);
			      int type_name_len = strlen (type_name);
			      char *name;

			      name = xstrprintf ("%.*s.%.*s",
						 type_name_len, type_name,
						 (yyvsp[0].sval).length, (yyvsp[0].sval).ptr);
			      make_cleanup (xfree, name);

			      sym =
				lookup_symbol (name, (const struct block *) NULL,
					       VAR_DOMAIN, NULL);
			      if (sym.symbol)
				{
				  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
				  write_exp_elt_block (pstate, sym.block);
				  write_exp_elt_sym (pstate, sym.symbol);
				  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
				  break;
				}

			      msymbol = lookup_bound_minimal_symbol (name);
			      if (msymbol.minsym != NULL)
				write_exp_msymbol (pstate, msymbol);
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error (_("No symbol table is loaded.  Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."), name);
			    }

			  /* Check if the qualified name resolves as a member
			     of an aggregate or an enum type.  */
			  if (!type_aggregate_p (type))
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));

			  write_exp_elt_opcode (pstate, OP_SCOPE);
			  write_exp_elt_type (pstate, type);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, OP_SCOPE);
			}
#line 1942 "d-exp.c" /* yacc.c:1646  */
    break;

  case 82:
#line 513 "d-exp.y" /* yacc.c:1646  */
    { write_dollar_variable (pstate, (yyvsp[0].sval)); }
#line 1948 "d-exp.c" /* yacc.c:1646  */
    break;

  case 83:
#line 515 "d-exp.y" /* yacc.c:1646  */
    { YYSTYPE val;
                  parse_number (pstate, (yyvsp[0].sval).ptr, (yyvsp[0].sval).length, 0, &val);
		  write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, val.typed_val_int.type);
		  write_exp_elt_longcst (pstate,
					 (LONGEST) val.typed_val_int.val);
		  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1960 "d-exp.c" /* yacc.c:1646  */
    break;

  case 84:
#line 523 "d-exp.y" /* yacc.c:1646  */
    { struct type *type = parse_d_type (pstate)->builtin_void;
		  type = lookup_pointer_type (type);
		  write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, type);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1971 "d-exp.c" /* yacc.c:1646  */
    break;

  case 85:
#line 530 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_BOOL);
		  write_exp_elt_longcst (pstate, (LONGEST) 1);
		  write_exp_elt_opcode (pstate, OP_BOOL); }
#line 1979 "d-exp.c" /* yacc.c:1646  */
    break;

  case 86:
#line 534 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_BOOL);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_opcode (pstate, OP_BOOL); }
#line 1987 "d-exp.c" /* yacc.c:1646  */
    break;

  case 87:
#line 538 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, (yyvsp[0].typed_val_int).type);
		  write_exp_elt_longcst (pstate, (LONGEST)((yyvsp[0].typed_val_int).val));
		  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1996 "d-exp.c" /* yacc.c:1646  */
    break;

  case 88:
#line 543 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DOUBLE);
		  write_exp_elt_type (pstate, (yyvsp[0].typed_val_float).type);
		  write_exp_elt_dblcst (pstate, (yyvsp[0].typed_val_float).dval);
		  write_exp_elt_opcode (pstate, OP_DOUBLE); }
#line 2005 "d-exp.c" /* yacc.c:1646  */
    break;

  case 89:
#line 548 "d-exp.y" /* yacc.c:1646  */
    { struct stoken_vector vec;
		  vec.len = 1;
		  vec.tokens = &(yyvsp[0].tsval);
		  write_exp_string_vector (pstate, (yyvsp[0].tsval).type, &vec); }
#line 2014 "d-exp.c" /* yacc.c:1646  */
    break;

  case 90:
#line 553 "d-exp.y" /* yacc.c:1646  */
    { int i;
		  write_exp_string_vector (pstate, 0, &(yyvsp[0].svec));
		  for (i = 0; i < (yyvsp[0].svec).len; ++i)
		    xfree ((yyvsp[0].svec).tokens[i].ptr);
		  xfree ((yyvsp[0].svec).tokens); }
#line 2024 "d-exp.c" /* yacc.c:1646  */
    break;

  case 91:
#line 559 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ARRAY);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].ival) - 1);
		  write_exp_elt_opcode (pstate, OP_ARRAY); }
#line 2033 "d-exp.c" /* yacc.c:1646  */
    break;

  case 92:
#line 564 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPEOF); }
#line 2039 "d-exp.c" /* yacc.c:1646  */
    break;

  case 93:
#line 569 "d-exp.y" /* yacc.c:1646  */
    { (yyval.ival) = arglist_len; }
#line 2045 "d-exp.c" /* yacc.c:1646  */
    break;

  case 95:
#line 578 "d-exp.y" /* yacc.c:1646  */
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
		  vec->ptr = (char *) xmalloc ((yyvsp[0].tsval).length + 1);
		  memcpy (vec->ptr, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);
		}
#line 2064 "d-exp.c" /* yacc.c:1646  */
    break;

  case 96:
#line 593 "d-exp.y" /* yacc.c:1646  */
    { /* Note that we NUL-terminate here, but just
		     for convenience.  */
		  char *p;
		  ++(yyval.svec).len;
		  (yyval.svec).tokens
		    = XRESIZEVEC (struct typed_stoken, (yyval.svec).tokens, (yyval.svec).len);

		  p = (char *) xmalloc ((yyvsp[0].tsval).length + 1);
		  memcpy (p, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);

		  (yyval.svec).tokens[(yyval.svec).len - 1].type = (yyvsp[0].tsval).type;
		  (yyval.svec).tokens[(yyval.svec).len - 1].length = (yyvsp[0].tsval).length;
		  (yyval.svec).tokens[(yyval.svec).len - 1].ptr = p;
		}
#line 2083 "d-exp.c" /* yacc.c:1646  */
    break;

  case 97:
#line 611 "d-exp.y" /* yacc.c:1646  */
    { /* Do nothing.  */ }
#line 2089 "d-exp.c" /* yacc.c:1646  */
    break;

  case 98:
#line 613 "d-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPE);
		  write_exp_elt_type (pstate, (yyvsp[0].tval));
		  write_exp_elt_opcode (pstate, OP_TYPE); }
#line 2097 "d-exp.c" /* yacc.c:1646  */
    break;

  case 99:
#line 617 "d-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = follow_types ((yyvsp[-1].tval));
		  write_exp_elt_opcode (pstate, OP_TYPE);
		  write_exp_elt_type (pstate, (yyval.tval));
		  write_exp_elt_opcode (pstate, OP_TYPE);
		}
#line 2107 "d-exp.c" /* yacc.c:1646  */
    break;

  case 100:
#line 626 "d-exp.y" /* yacc.c:1646  */
    { push_type (tp_pointer); }
#line 2113 "d-exp.c" /* yacc.c:1646  */
    break;

  case 101:
#line 628 "d-exp.y" /* yacc.c:1646  */
    { push_type (tp_pointer); }
#line 2119 "d-exp.c" /* yacc.c:1646  */
    break;

  case 102:
#line 630 "d-exp.y" /* yacc.c:1646  */
    { push_type_int ((yyvsp[-1].typed_val_int).val);
		  push_type (tp_array); }
#line 2126 "d-exp.c" /* yacc.c:1646  */
    break;

  case 103:
#line 633 "d-exp.y" /* yacc.c:1646  */
    { push_type_int ((yyvsp[-2].typed_val_int).val);
		  push_type (tp_array); }
#line 2133 "d-exp.c" /* yacc.c:1646  */
    break;

  case 104:
#line 639 "d-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 2139 "d-exp.c" /* yacc.c:1646  */
    break;


#line 2143 "d-exp.c" /* yacc.c:1646  */
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


/* Return true if the type is aggregate-like.  */

static int
type_aggregate_p (struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_STRUCT
	  || TYPE_CODE (type) == TYPE_CODE_UNION
	  || TYPE_CODE (type) == TYPE_CODE_MODULE
	  || (TYPE_CODE (type) == TYPE_CODE_ENUM
	      && TYPE_DECLARED_CLASS (type)));
}

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

  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '\'' ? CHARACTER_LITERAL : STRING_LITERAL;
}

struct token
{
  const char *oper;
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

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure operator.
   This is used only when parsing to do field name completion.  */
static int last_was_structop;

/* Read one token, getting characters through lexptr.  */

static int
lex_one_token (struct parser_state *par_state)
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
	    else if ((*p < '0' || *p > '9') && (*p != '_')
		     && ((*p < 'a' || *p > 'z') && (*p < 'A' || *p > 'Z')))
	      break;
	  }

	toktype = parse_number (par_state, tokstart, p - tokstart,
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
    = language_lookup_primitive_type (parse_language (par_state),
				      parse_gdbarch (par_state), copy);
  if (yylval.tsym.type != NULL)
    return TYPENAME;

  /* Input names that aren't symbols but ARE valid hex numbers,
     when the input radix permits them, can be names or numbers
     depending on the parse.  Note we support radixes > 16 here.  */
  if ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
      || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, tokstart, namelen, 0, &newlval);
      if (hextype == INTEGER_LITERAL)
	return NAME_OR_INT;
    }

  if (parse_completion && *lexptr == '\0')
    saw_name_at_eof = 1;

  return IDENTIFIER;
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

/* Classify an IDENTIFIER token.  The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.  BLOCK is the block
   in which lookups start; this can be NULL to mean the global scope.  */

static int
classify_name (struct parser_state *par_state, const struct block *block)
{
  struct block_symbol sym;
  char *copy;
  struct field_of_this_result is_a_field_of_this;

  copy = copy_name (yylval.sval);

  sym = lookup_symbol (copy, block, VAR_DOMAIN, &is_a_field_of_this);
  if (sym.symbol && SYMBOL_CLASS (sym.symbol) == LOC_TYPEDEF)
    {
      yylval.tsym.type = SYMBOL_TYPE (sym.symbol);
      return TYPENAME;
    }
  else if (sym.symbol == NULL)
    {
      /* Look-up first for a module name, then a type.  */
      sym = lookup_symbol (copy, block, MODULE_DOMAIN, NULL);
      if (sym.symbol == NULL)
	sym = lookup_symbol (copy, block, STRUCT_DOMAIN, NULL);

      if (sym.symbol != NULL)
	{
	  yylval.tsym.type = SYMBOL_TYPE (sym.symbol);
	  return TYPENAME;
	}

      return UNKNOWN_NAME;
    }

  return IDENTIFIER;
}

/* Like classify_name, but used by the inner loop of the lexer, when a
   name might have already been seen.  CONTEXT is the context type, or
   NULL if this is the first component of a name.  */

static int
classify_inner_name (struct parser_state *par_state,
		     const struct block *block, struct type *context)
{
  struct type *type;
  char *copy;

  if (context == NULL)
    return classify_name (par_state, block);

  type = check_typedef (context);
  if (!type_aggregate_p (type))
    return ERROR;

  copy = copy_name (yylval.ssym.stoken);
  yylval.ssym.sym = d_lookup_nested_symbol (type, copy, block);

  if (yylval.ssym.sym.symbol == NULL)
    return ERROR;

  if (SYMBOL_CLASS (yylval.ssym.sym.symbol) == LOC_TYPEDEF)
    {
      yylval.tsym.type = SYMBOL_TYPE (yylval.ssym.sym.symbol);
      return TYPENAME;
    }

  return IDENTIFIER;
}

/* The outer level of a two-level lexer.  This calls the inner lexer
   to return tokens.  It then either returns these tokens, or
   aggregates them into a larger token.  This lets us work around a
   problem in our parsing approach, where the parser could not
   distinguish between qualified names and qualified types at the
   right point.  */

static int
yylex (void)
{
  token_and_value current;
  int last_was_dot;
  struct type *context_type = NULL;
  int last_to_examine, next_to_examine, checkpoint;
  const struct block *search_block;

  if (popping && !VEC_empty (token_and_value, token_fifo))
    goto do_pop;
  popping = 0;

  /* Read the first token and decide what to do.  */
  current.token = lex_one_token (pstate);
  if (current.token != IDENTIFIER && current.token != '.')
    return current.token;

  /* Read any sequence of alternating "." and identifier tokens into
     the token FIFO.  */
  current.value = yylval;
  VEC_safe_push (token_and_value, token_fifo, &current);
  last_was_dot = current.token == '.';

  while (1)
    {
      current.token = lex_one_token (pstate);
      current.value = yylval;
      VEC_safe_push (token_and_value, token_fifo, &current);

      if ((last_was_dot && current.token != IDENTIFIER)
	  || (!last_was_dot && current.token != '.'))
	break;

      last_was_dot = !last_was_dot;
    }
  popping = 1;

  /* We always read one extra token, so compute the number of tokens
     to examine accordingly.  */
  last_to_examine = VEC_length (token_and_value, token_fifo) - 2;
  next_to_examine = 0;

  current = *VEC_index (token_and_value, token_fifo, next_to_examine);
  ++next_to_examine;

  /* If we are not dealing with a typename, now is the time to find out.  */
  if (current.token == IDENTIFIER)
    {
      yylval = current.value;
      current.token = classify_name (pstate, expression_context_block);
      current.value = yylval;
    }

  /* If the IDENTIFIER is not known, it could be a package symbol,
     first try building up a name until we find the qualified module.  */
  if (current.token == UNKNOWN_NAME)
    {
      obstack_free (&name_obstack, obstack_base (&name_obstack));
      obstack_grow (&name_obstack, current.value.sval.ptr,
		    current.value.sval.length);

      last_was_dot = 0;

      while (next_to_examine <= last_to_examine)
	{
	  token_and_value *next;

	  next = VEC_index (token_and_value, token_fifo, next_to_examine);
	  ++next_to_examine;

	  if (next->token == IDENTIFIER && last_was_dot)
	    {
	      /* Update the partial name we are constructing.  */
              obstack_grow_str (&name_obstack, ".");
	      obstack_grow (&name_obstack, next->value.sval.ptr,
			    next->value.sval.length);

	      yylval.sval.ptr = (char *) obstack_base (&name_obstack);
	      yylval.sval.length = obstack_object_size (&name_obstack);

	      current.token = classify_name (pstate, expression_context_block);
	      current.value = yylval;

	      /* We keep going until we find a TYPENAME.  */
	      if (current.token == TYPENAME)
		{
		  /* Install it as the first token in the FIFO.  */
		  VEC_replace (token_and_value, token_fifo, 0, &current);
		  VEC_block_remove (token_and_value, token_fifo, 1,
				    next_to_examine - 1);
		  break;
		}
	    }
	  else if (next->token == '.' && !last_was_dot)
	    last_was_dot = 1;
	  else
	    {
	      /* We've reached the end of the name.  */
	      break;
	    }
	}

      /* Reset our current token back to the start, if we found nothing
	 this means that we will just jump to do pop.  */
      current = *VEC_index (token_and_value, token_fifo, 0);
      next_to_examine = 1;
    }
  if (current.token != TYPENAME && current.token != '.')
    goto do_pop;

  obstack_free (&name_obstack, obstack_base (&name_obstack));
  checkpoint = 0;
  if (current.token == '.')
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

  last_was_dot = current.token == '.';

  while (next_to_examine <= last_to_examine)
    {
      token_and_value *next;

      next = VEC_index (token_and_value, token_fifo, next_to_examine);
      ++next_to_examine;

      if (next->token == IDENTIFIER && last_was_dot)
	{
	  int classification;

	  yylval = next->value;
	  classification = classify_inner_name (pstate, search_block,
						context_type);
	  /* We keep going until we either run out of names, or until
	     we have a qualified name which is not a type.  */
	  if (classification != TYPENAME && classification != IDENTIFIER)
	    break;

	  /* Accept up to this token.  */
	  checkpoint = next_to_examine;

	  /* Update the partial name we are constructing.  */
	  if (context_type != NULL)
	    {
	      /* We don't want to put a leading "." into the name.  */
              obstack_grow_str (&name_obstack, ".");
	    }
	  obstack_grow (&name_obstack, next->value.sval.ptr,
			next->value.sval.length);

	  yylval.sval.ptr = (char *) obstack_base (&name_obstack);
	  yylval.sval.length = obstack_object_size (&name_obstack);
	  current.value = yylval;
	  current.token = classification;

	  last_was_dot = 0;

	  if (classification == IDENTIFIER)
	    break;

	  context_type = yylval.tsym.type;
	}
      else if (next->token == '.' && !last_was_dot)
	last_was_dot = 1;
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
d_parse (struct parser_state *par_state)
{
  int result;
  struct cleanup *back_to;

  /* Setting up the parser state.  */
  gdb_assert (par_state != NULL);
  pstate = par_state;

  back_to = make_cleanup (null_cleanup, NULL);

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);
  make_cleanup_clear_parser_state (&pstate);

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
yyerror (const char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), (msg ? msg : "error"), lexptr);
}

