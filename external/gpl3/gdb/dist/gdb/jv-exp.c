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
#line 36 "jv-exp.y" /* yacc.c:339  */


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "jv-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "block.h"
#include "completer.h"

#define parse_type(ps) builtin_type (parse_gdbarch (ps))
#define parse_java_type(ps) builtin_java_type (parse_gdbarch (ps))

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX java_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

void yyerror (char *);

static struct type *java_type_from_name (struct stoken);
static void push_expression_name (struct parser_state *, struct stoken);
static void push_fieldnames (struct parser_state *, struct stoken);

static struct expression *copy_exp (struct expression *, int);
static void insert_exp (struct parser_state *, int, struct expression *);


#line 109 "jv-exp.c" /* yacc.c:339  */

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
    INTEGER_LITERAL = 258,
    FLOATING_POINT_LITERAL = 259,
    IDENTIFIER = 260,
    STRING_LITERAL = 261,
    BOOLEAN_LITERAL = 262,
    TYPENAME = 263,
    NAME_OR_INT = 264,
    ERROR = 265,
    LONG = 266,
    SHORT = 267,
    BYTE = 268,
    INT = 269,
    CHAR = 270,
    BOOLEAN = 271,
    DOUBLE = 272,
    FLOAT = 273,
    VARIABLE = 274,
    ASSIGN_MODIFY = 275,
    SUPER = 276,
    NEW = 277,
    OROR = 278,
    ANDAND = 279,
    EQUAL = 280,
    NOTEQUAL = 281,
    LEQ = 282,
    GEQ = 283,
    LSH = 284,
    RSH = 285,
    INCREMENT = 286,
    DECREMENT = 287
  };
#endif
/* Tokens.  */
#define INTEGER_LITERAL 258
#define FLOATING_POINT_LITERAL 259
#define IDENTIFIER 260
#define STRING_LITERAL 261
#define BOOLEAN_LITERAL 262
#define TYPENAME 263
#define NAME_OR_INT 264
#define ERROR 265
#define LONG 266
#define SHORT 267
#define BYTE 268
#define INT 269
#define CHAR 270
#define BOOLEAN 271
#define DOUBLE 272
#define FLOAT 273
#define VARIABLE 274
#define ASSIGN_MODIFY 275
#define SUPER 276
#define NEW 277
#define OROR 278
#define ANDAND 279
#define EQUAL 280
#define NOTEQUAL 281
#define LEQ 282
#define GEQ 283
#define LSH 284
#define RSH 285
#define INCREMENT 286
#define DECREMENT 287

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 84 "jv-exp.y" /* yacc.c:355  */

    LONGEST lval;
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
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;
    int *ivec;
  

#line 232 "jv-exp.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */
#line 105 "jv-exp.y" /* yacc.c:358  */

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *, int,
			 int, YYSTYPE *);

#line 252 "jv-exp.c" /* yacc.c:358  */

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
#define YYFINAL  98
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   373

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  56
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  58
/* YYNRULES -- Number of rules.  */
#define YYNRULES  132
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  209

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   287

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    54,     2,     2,     2,    43,    30,     2,
      48,    49,    41,    39,    23,    40,    46,    42,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    55,     2,
      33,    24,    34,    25,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    47,     2,    52,    29,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    50,    28,    51,    53,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    26,    27,
      31,    32,    35,    36,    37,    38,    44,    45
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   162,   162,   163,   166,   175,   176,   180,   189,   194,
     203,   208,   214,   225,   226,   231,   232,   236,   238,   240,
     242,   244,   249,   251,   263,   268,   272,   274,   279,   280,
     284,   285,   289,   290,   294,   320,   321,   326,   327,   331,
     332,   333,   334,   335,   336,   337,   345,   350,   355,   361,
     363,   369,   370,   374,   377,   383,   384,   388,   392,   394,
     399,   401,   405,   407,   413,   419,   418,   424,   426,   431,
     451,   453,   458,   459,   461,   463,   464,   468,   473,   478,
     479,   480,   481,   483,   486,   490,   495,   500,   501,   503,
     505,   509,   513,   537,   547,   548,   550,   552,   557,   558,
     560,   565,   566,   568,   574,   575,   577,   579,   581,   587,
     588,   590,   595,   596,   601,   602,   606,   607,   612,   613,
     618,   619,   624,   625,   630,   631,   635,   637,   644,   646,
     648,   649,   654
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INTEGER_LITERAL",
  "FLOATING_POINT_LITERAL", "IDENTIFIER", "STRING_LITERAL",
  "BOOLEAN_LITERAL", "TYPENAME", "NAME_OR_INT", "ERROR", "LONG", "SHORT",
  "BYTE", "INT", "CHAR", "BOOLEAN", "DOUBLE", "FLOAT", "VARIABLE",
  "ASSIGN_MODIFY", "SUPER", "NEW", "','", "'='", "'?'", "OROR", "ANDAND",
  "'|'", "'^'", "'&'", "EQUAL", "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ",
  "LSH", "RSH", "'+'", "'-'", "'*'", "'/'", "'%'", "INCREMENT",
  "DECREMENT", "'.'", "'['", "'('", "')'", "'{'", "'}'", "']'", "'~'",
  "'!'", "':'", "$accept", "start", "type_exp", "PrimitiveOrArrayType",
  "StringLiteral", "Literal", "PrimitiveType", "NumericType",
  "IntegralType", "FloatingPointType", "ClassOrInterfaceType", "ClassType",
  "ArrayType", "Name", "ForcedName", "SimpleName", "QualifiedName", "exp1",
  "Primary", "PrimaryNoNewArray", "lcurly", "rcurly",
  "ClassInstanceCreationExpression", "ArgumentList", "ArgumentList_opt",
  "ArrayCreationExpression", "DimExprs", "DimExpr", "Dims", "Dims_opt",
  "FieldAccess", "FuncStart", "MethodInvocation", "$@1", "ArrayAccess",
  "PostfixExpression", "PostIncrementExpression",
  "PostDecrementExpression", "UnaryExpression", "PreIncrementExpression",
  "PreDecrementExpression", "UnaryExpressionNotPlusMinus",
  "CastExpression", "MultiplicativeExpression", "AdditiveExpression",
  "ShiftExpression", "RelationalExpression", "EqualityExpression",
  "AndExpression", "ExclusiveOrExpression", "InclusiveOrExpression",
  "ConditionalAndExpression", "ConditionalOrExpression",
  "ConditionalExpression", "AssignmentExpression", "Assignment",
  "LeftHandSide", "Expression", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,    44,    61,    63,   278,   279,   124,    94,
      38,   280,   281,    60,    62,   282,   283,   284,   285,    43,
      45,    42,    47,    37,   286,   287,    46,    91,    40,    41,
     123,   125,    93,   126,    33,    58
};
# endif

#define YYPACT_NINF -145

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-145)))

#define YYTABLE_NINF -132

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     215,  -145,  -145,    -5,  -145,  -145,     1,  -145,  -145,  -145,
    -145,  -145,  -145,  -145,  -145,    -7,   -19,   279,    50,    50,
      50,    50,    50,   215,  -145,    50,    50,    46,  -145,  -145,
    -145,  -145,    -9,  -145,  -145,  -145,  -145,    87,  -145,  -145,
      12,    44,     5,    16,   319,  -145,  -145,    28,  -145,  -145,
      38,    29,  -145,  -145,  -145,  -145,  -145,  -145,  -145,    99,
      53,    85,    52,    94,    66,    41,    71,    74,   122,  -145,
    -145,  -145,    40,  -145,    26,   319,    26,  -145,    59,    59,
      67,    82,  -145,  -145,   111,   107,  -145,  -145,  -145,  -145,
    -145,  -145,  -145,    -9,    87,    68,  -145,  -145,  -145,    79,
      91,    26,   267,  -145,    91,   319,    26,   319,   -18,  -145,
     319,  -145,  -145,    50,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
     319,    50,    50,    50,  -145,  -145,  -145,   112,   126,   319,
     128,  -145,   128,   319,   319,    91,   127,   -31,   193,  -145,
     125,  -145,   129,  -145,   131,   130,   319,  -145,  -145,   157,
     135,  -145,  -145,  -145,    99,    99,    53,    53,    85,    85,
      85,    85,    52,    52,    94,    66,    41,    71,   132,    74,
    -145,  -145,  -145,   319,   134,   267,  -145,  -145,  -145,   139,
      50,   193,  -145,  -145,  -145,   319,  -145,  -145,  -145,    50,
     141,  -145,  -145,  -145,  -145,   144,  -145,  -145,  -145
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     8,    10,    28,     7,    11,     9,    20,    18,    17,
      19,    21,    14,    23,    22,    74,     0,     0,     0,     0,
       0,     0,     0,     0,    46,     0,     0,     0,     3,     4,
      12,    39,     5,    13,    15,    16,     6,    73,   128,    30,
      29,     2,    72,    37,     0,    41,    38,    42,    65,    43,
      44,    87,    75,    76,    94,    79,    80,    84,    90,    98,
     101,   104,   109,   112,   114,   116,   118,   120,   122,   124,
     132,   125,     0,    35,     0,     0,     0,    28,     0,    25,
       0,    24,    29,     9,    74,    73,    42,    44,    81,    82,
      83,    85,    86,    61,    73,     0,    88,    89,     1,     0,
      26,     0,     0,    64,    27,     0,     0,     0,     0,    49,
      51,    77,    78,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    63,     0,     0,     0,
      61,    55,    61,    51,     0,    60,     0,     0,    40,    58,
       0,    34,     0,    36,    62,     0,     0,    47,    45,    52,
       0,    95,    96,    97,    99,   100,   102,   103,   105,   106,
     107,   108,   110,   111,   113,   115,   117,   119,     0,   121,
     127,   126,    70,    51,     0,     0,    56,    53,    54,     0,
       0,     0,    92,    59,    69,    51,    71,    50,    66,     0,
       0,    57,    48,    91,    93,     0,   123,    68,    67
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -145,  -145,  -145,  -145,  -145,  -145,    -3,  -145,  -145,  -145,
    -145,  -145,  -145,    11,  -145,   -64,     0,  -145,  -145,  -145,
    -145,  -145,  -145,   150,  -134,  -145,   124,  -116,   -29,   -99,
       6,  -145,  -145,  -145,    22,  -145,  -145,  -145,    58,  -145,
    -145,  -144,  -145,    43,    49,    -2,    45,    78,    81,    83,
      77,    92,  -145,  -131,  -145,  -145,  -145,     7
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      79,    80,    36,    85,    38,    39,    82,    41,    42,    43,
      44,   158,    45,   159,   160,    46,   140,   141,   145,   146,
      86,    48,    49,   110,    87,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,   109
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      40,   180,   181,   100,   192,   156,    47,    73,   104,   189,
     136,    37,   138,  -129,    78,   -32,   150,  -129,   191,   -32,
      93,   -33,    50,    40,   186,   -33,   186,    76,    81,    47,
      95,   134,   -31,   157,    94,   135,   -31,   151,    99,    74,
      75,   187,   154,   188,    40,    50,    98,   204,  -130,   200,
      47,   106,  -130,     1,     2,    77,     4,     5,  -131,    83,
     132,   205,  -131,   107,   133,   147,    50,   105,   206,    84,
     127,    16,    17,   111,   112,    40,    88,    89,    90,    91,
      92,    47,   137,    96,    97,   120,   121,   122,   123,    18,
      19,    20,   116,   117,    21,    22,   126,    50,    23,   128,
      24,   129,    40,    25,    26,    40,   139,    40,    47,   152,
      40,    47,   153,    47,   155,   143,    47,   148,   168,   169,
     170,   171,   118,   119,    50,   124,   125,    50,   101,    50,
      40,   149,    50,   101,   102,   103,    47,   178,   150,    40,
     113,   114,   115,    40,    40,    47,   184,   130,   131,    47,
      47,   152,    50,   101,   144,   103,    40,    74,    75,   164,
     165,    50,    47,   197,   182,    50,    50,   166,   167,   172,
     173,   161,   162,   163,   183,   185,   190,   193,    50,   195,
     156,   194,   196,    40,   198,    40,   201,   199,   202,    47,
     207,    47,   184,   208,   108,    40,     1,     2,    77,     4,
       5,    47,    83,   142,   174,    50,   177,    50,   175,     0,
       0,   176,    84,     0,    16,    17,     0,    50,     1,     2,
       3,     4,     5,   179,     6,     0,     7,     8,     9,    10,
      11,    12,    13,    14,    15,     0,    16,    17,     0,     0,
       0,    23,     0,    24,     0,     0,    25,    26,   203,     0,
       0,     0,     0,     0,    18,    19,    20,     0,     0,    21,
      22,     0,     0,    23,     0,    24,     0,     0,    25,    26,
       1,     2,     3,     4,     5,     0,     6,     0,     0,     0,
       0,     0,     0,     0,    77,     0,    15,     0,    16,    17,
       7,     8,     9,    10,    11,    12,    13,    14,     0,     0,
       0,     0,     0,     0,     0,     0,    18,    19,    20,     0,
       0,    21,    22,     0,     0,    23,     0,    24,     0,   149,
      25,    26,     1,     2,     3,     4,     5,     0,     6,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    15,     0,
      16,    17,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    18,    19,
      20,     0,     0,    21,    22,     0,     0,    23,     0,    24,
       0,     0,    25,    26
};

static const yytype_int16 yycheck[] =
{
       0,   132,   133,    32,   148,    23,     0,     0,    37,   143,
      74,     0,    76,    20,    17,    20,    47,    24,    49,    24,
      23,    20,     0,    23,   140,    24,   142,    46,    17,    23,
      23,     5,    20,    51,    23,     9,    24,   101,    47,    46,
      47,   140,   106,   142,    44,    23,     0,   191,    20,   183,
      44,    46,    24,     3,     4,     5,     6,     7,    20,     9,
      20,   195,    24,    47,    24,    94,    44,    23,   199,    19,
      29,    21,    22,    44,    45,    75,    18,    19,    20,    21,
      22,    75,    75,    25,    26,    33,    34,    35,    36,    39,
      40,    41,    39,    40,    44,    45,    30,    75,    48,    28,
      50,    27,   102,    53,    54,   105,    47,   107,   102,   102,
     110,   105,   105,   107,   107,    48,   110,    49,   120,   121,
     122,   123,    37,    38,   102,    31,    32,   105,    46,   107,
     130,    52,   110,    46,    47,    48,   130,   130,    47,   139,
      41,    42,    43,   143,   144,   139,   139,    25,    26,   143,
     144,   144,   130,    46,    47,    48,   156,    46,    47,   116,
     117,   139,   156,   156,    52,   143,   144,   118,   119,   124,
     125,   113,   114,   115,    48,    47,    49,    52,   156,    48,
      23,    52,    52,   183,    49,   185,    52,    55,    49,   183,
      49,   185,   185,    49,    44,   195,     3,     4,     5,     6,
       7,   195,     9,    79,   126,   183,   129,   185,   127,    -1,
      -1,   128,    19,    -1,    21,    22,    -1,   195,     3,     4,
       5,     6,     7,   131,     9,    -1,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    -1,    21,    22,    -1,    -1,
      -1,    48,    -1,    50,    -1,    -1,    53,    54,   190,    -1,
      -1,    -1,    -1,    -1,    39,    40,    41,    -1,    -1,    44,
      45,    -1,    -1,    48,    -1,    50,    -1,    -1,    53,    54,
       3,     4,     5,     6,     7,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,    -1,    19,    -1,    21,    22,
      11,    12,    13,    14,    15,    16,    17,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    40,    41,    -1,
      -1,    44,    45,    -1,    -1,    48,    -1,    50,    -1,    52,
      53,    54,     3,     4,     5,     6,     7,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    40,
      41,    -1,    -1,    44,    45,    -1,    -1,    48,    -1,    50,
      -1,    -1,    53,    54
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     9,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    21,    22,    39,    40,
      41,    44,    45,    48,    50,    53,    54,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    78,    81,    86,    87,    88,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,    46,    47,    46,     5,    62,    66,
      67,    69,    72,     9,    19,    69,    86,    90,    94,    94,
      94,    94,    94,    62,    69,   113,    94,    94,     0,    47,
      84,    46,    47,    48,    84,    23,    46,    47,    79,   113,
      89,    44,    45,    41,    42,    43,    39,    40,    37,    38,
      33,    34,    35,    36,    31,    32,    30,    29,    28,    27,
      25,    26,    20,    24,     5,     9,    71,   113,    71,    47,
      82,    83,    82,    48,    47,    84,    85,    84,    49,    52,
      47,    71,   113,   113,    71,   113,    23,    51,    77,    79,
      80,    94,    94,    94,    99,    99,   100,   100,   101,   101,
     101,   101,   102,   102,   103,   104,   105,   106,   113,   107,
     109,   109,    52,    48,   113,    47,    83,    85,    85,    80,
      49,    49,    97,    52,    52,    48,    52,   113,    49,    55,
      80,    52,    49,    94,    97,    80,   109,    49,    49
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    56,    57,    57,    58,    59,    59,    60,    61,    61,
      61,    61,    61,    62,    62,    63,    63,    64,    64,    64,
      64,    64,    65,    65,    66,    67,    68,    68,    69,    69,
      70,    70,    71,    71,    72,    73,    73,    74,    74,    75,
      75,    75,    75,    75,    75,    75,    76,    77,    78,    79,
      79,    80,    80,    81,    81,    82,    82,    83,    84,    84,
      85,    85,    86,    86,    87,    89,    88,    88,    88,    90,
      90,    90,    91,    91,    91,    91,    91,    92,    93,    94,
      94,    94,    94,    94,    94,    95,    96,    97,    97,    97,
      97,    98,    98,    98,    99,    99,    99,    99,   100,   100,
     100,   101,   101,   101,   102,   102,   102,   102,   102,   103,
     103,   103,   104,   104,   105,   105,   106,   106,   107,   107,
     108,   108,   109,   109,   110,   110,   111,   111,   112,   112,
     112,   112,   113
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     1,     1,
       1,     1,     1,     1,     3,     1,     3,     1,     1,     1,
       3,     1,     1,     1,     1,     3,     1,     1,     5,     1,
       3,     0,     1,     4,     4,     1,     2,     3,     2,     3,
       1,     0,     3,     3,     2,     0,     4,     6,     6,     4,
       4,     4,     1,     1,     1,     1,     1,     2,     2,     1,
       1,     2,     2,     2,     1,     2,     2,     1,     2,     2,
       1,     5,     4,     5,     1,     3,     3,     3,     1,     3,
       3,     1,     3,     3,     1,     3,     3,     3,     3,     1,
       3,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     5,     1,     1,     3,     3,     1,     1,
       1,     1,     1
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
        case 4:
#line 167 "jv-exp.y" /* yacc.c:1646  */
    {
		  write_exp_elt_opcode (pstate, OP_TYPE);
		  write_exp_elt_type (pstate, (yyvsp[0].tval));
		  write_exp_elt_opcode (pstate, OP_TYPE);
		}
#line 1535 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 7:
#line 181 "jv-exp.y" /* yacc.c:1646  */
    {
		  write_exp_elt_opcode (pstate, OP_STRING);
		  write_exp_string (pstate, (yyvsp[0].sval));
		  write_exp_elt_opcode (pstate, OP_STRING);
		}
#line 1545 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 8:
#line 190 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, (yyvsp[0].typed_val_int).type);
		  write_exp_elt_longcst (pstate, (LONGEST)((yyvsp[0].typed_val_int).val));
		  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1554 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 9:
#line 195 "jv-exp.y" /* yacc.c:1646  */
    { YYSTYPE val;
		  parse_number (pstate, (yyvsp[0].sval).ptr, (yyvsp[0].sval).length, 0, &val);
		  write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, val.typed_val_int.type);
		  write_exp_elt_longcst (pstate,
					 (LONGEST) val.typed_val_int.val);
		  write_exp_elt_opcode (pstate, OP_LONG);
		}
#line 1567 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 10:
#line 204 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DOUBLE);
		  write_exp_elt_type (pstate, (yyvsp[0].typed_val_float).type);
		  write_exp_elt_dblcst (pstate, (yyvsp[0].typed_val_float).dval);
		  write_exp_elt_opcode (pstate, OP_DOUBLE); }
#line 1576 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 11:
#line 209 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate,
				  parse_java_type (pstate)->builtin_boolean);
		  write_exp_elt_longcst (pstate, (LONGEST)(yyvsp[0].lval));
		  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1586 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 14:
#line 227 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = parse_java_type (pstate)->builtin_boolean; }
#line 1592 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 17:
#line 237 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = parse_java_type (pstate)->builtin_byte; }
#line 1598 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 18:
#line 239 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = parse_java_type (pstate)->builtin_short; }
#line 1604 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 19:
#line 241 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = parse_java_type (pstate)->builtin_int; }
#line 1610 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 20:
#line 243 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = parse_java_type (pstate)->builtin_long; }
#line 1616 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 21:
#line 245 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = parse_java_type (pstate)->builtin_char; }
#line 1622 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 22:
#line 250 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = parse_java_type (pstate)->builtin_float; }
#line 1628 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 23:
#line 252 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = parse_java_type (pstate)->builtin_double; }
#line 1634 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 24:
#line 264 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = java_type_from_name ((yyvsp[0].sval)); }
#line 1640 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 26:
#line 273 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = java_array_type ((yyvsp[-1].tval), (yyvsp[0].lval)); }
#line 1646 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 27:
#line 275 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = java_array_type (java_type_from_name ((yyvsp[-1].sval)), (yyvsp[0].lval)); }
#line 1652 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 34:
#line 295 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.sval).length = (yyvsp[-2].sval).length + (yyvsp[0].sval).length + 1;
		  if ((yyvsp[-2].sval).ptr + (yyvsp[-2].sval).length + 1 == (yyvsp[0].sval).ptr
		      && (yyvsp[-2].sval).ptr[(yyvsp[-2].sval).length] == '.')
		    (yyval.sval).ptr = (yyvsp[-2].sval).ptr;  /* Optimization.  */
		  else
		    {
		      char *buf;

		      buf = (char *) xmalloc ((yyval.sval).length + 1);
		      make_cleanup (xfree, buf);
		      sprintf (buf, "%.*s.%.*s",
			       (yyvsp[-2].sval).length, (yyvsp[-2].sval).ptr, (yyvsp[0].sval).length, (yyvsp[0].sval).ptr);
		      (yyval.sval).ptr = buf;
		} }
#line 1671 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 36:
#line 322 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_COMMA); }
#line 1677 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 45:
#line 338 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ARRAY);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].lval));
		  write_exp_elt_opcode (pstate, OP_ARRAY); }
#line 1686 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 46:
#line 346 "jv-exp.y" /* yacc.c:1646  */
    { start_arglist (); }
#line 1692 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 47:
#line 351 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = end_arglist () - 1; }
#line 1698 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 48:
#line 356 "jv-exp.y" /* yacc.c:1646  */
    { internal_error (__FILE__, __LINE__,
				  _("FIXME - ClassInstanceCreationExpression")); }
#line 1705 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 49:
#line 362 "jv-exp.y" /* yacc.c:1646  */
    { arglist_len = 1; }
#line 1711 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 50:
#line 364 "jv-exp.y" /* yacc.c:1646  */
    { arglist_len++; }
#line 1717 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 51:
#line 369 "jv-exp.y" /* yacc.c:1646  */
    { arglist_len = 0; }
#line 1723 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 53:
#line 375 "jv-exp.y" /* yacc.c:1646  */
    { internal_error (__FILE__, __LINE__,
				  _("FIXME - ArrayCreationExpression")); }
#line 1730 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 54:
#line 378 "jv-exp.y" /* yacc.c:1646  */
    { internal_error (__FILE__, __LINE__,
				  _("FIXME - ArrayCreationExpression")); }
#line 1737 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 58:
#line 393 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 1743 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 59:
#line 395 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-2].lval) + 1; }
#line 1749 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 61:
#line 401 "jv-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 0; }
#line 1755 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 62:
#line 406 "jv-exp.y" /* yacc.c:1646  */
    { push_fieldnames (pstate, (yyvsp[0].sval)); }
#line 1761 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 63:
#line 408 "jv-exp.y" /* yacc.c:1646  */
    { push_fieldnames (pstate, (yyvsp[0].sval)); }
#line 1767 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 64:
#line 414 "jv-exp.y" /* yacc.c:1646  */
    { push_expression_name (pstate, (yyvsp[-1].sval)); }
#line 1773 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 65:
#line 419 "jv-exp.y" /* yacc.c:1646  */
    { start_arglist(); }
#line 1779 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 66:
#line 421 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_FUNCALL);
		  write_exp_elt_longcst (pstate, (LONGEST) end_arglist ());
		  write_exp_elt_opcode (pstate, OP_FUNCALL); }
#line 1787 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 67:
#line 425 "jv-exp.y" /* yacc.c:1646  */
    { error (_("Form of method invocation not implemented")); }
#line 1793 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 68:
#line 427 "jv-exp.y" /* yacc.c:1646  */
    { error (_("Form of method invocation not implemented")); }
#line 1799 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 69:
#line 432 "jv-exp.y" /* yacc.c:1646  */
    {
                  /* Emit code for the Name now, then exchange it in the
		     expout array with the Expression's code.  We could
		     introduce a OP_SWAP code or a reversed version of
		     BINOP_SUBSCRIPT, but that makes the rest of GDB pay
		     for our parsing kludges.  */
		  struct expression *name_expr;

		  push_expression_name (pstate, (yyvsp[-3].sval));
		  name_expr = copy_exp (pstate->expout, pstate->expout_ptr);
		  pstate->expout_ptr -= name_expr->nelts;
		  insert_exp (pstate,
			      pstate->expout_ptr
			      - length_of_subexp (pstate->expout,
						  pstate->expout_ptr),
			      name_expr);
		  xfree (name_expr);
		  write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT);
		}
#line 1823 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 70:
#line 452 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT); }
#line 1829 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 71:
#line 454 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT); }
#line 1835 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 73:
#line 460 "jv-exp.y" /* yacc.c:1646  */
    { push_expression_name (pstate, (yyvsp[0].sval)); }
#line 1841 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 77:
#line 469 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTINCREMENT); }
#line 1847 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 78:
#line 474 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTDECREMENT); }
#line 1853 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 82:
#line 482 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_NEG); }
#line 1859 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 83:
#line 484 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate,
					UNOP_IND); }
#line 1866 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 85:
#line 491 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREINCREMENT); }
#line 1872 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 86:
#line 496 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREDECREMENT); }
#line 1878 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 88:
#line 502 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_COMPLEMENT); }
#line 1884 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 89:
#line 504 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
#line 1890 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 91:
#line 510 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST);
		  write_exp_elt_type (pstate, java_array_type ((yyvsp[-3].tval), (yyvsp[-2].lval)));
		  write_exp_elt_opcode (pstate, UNOP_CAST); }
#line 1898 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 92:
#line 514 "jv-exp.y" /* yacc.c:1646  */
    {
		  int last_exp_size = length_of_subexp (pstate->expout,
							pstate->expout_ptr);
		  struct type *type;
		  int i;
		  int base = pstate->expout_ptr - last_exp_size - 3;

		  if (base < 0
		      || pstate->expout->elts[base+2].opcode != OP_TYPE)
		    error (_("Invalid cast expression"));
		  type = pstate->expout->elts[base+1].type;
		  /* Remove the 'Expression' and slide the
		     UnaryExpressionNotPlusMinus down to replace it.  */
		  for (i = 0;  i < last_exp_size;  i++)
		    pstate->expout->elts[base + i]
		      = pstate->expout->elts[base + i + 3];
		  pstate->expout_ptr -= 3;
		  if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
		    type = lookup_pointer_type (type);
		  write_exp_elt_opcode (pstate, UNOP_CAST);
		  write_exp_elt_type (pstate, type);
		  write_exp_elt_opcode (pstate, UNOP_CAST);
		}
#line 1926 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 93:
#line 538 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST);
		  write_exp_elt_type (pstate,
				      java_array_type (java_type_from_name
						       ((yyvsp[-3].sval)), (yyvsp[-2].lval)));
		  write_exp_elt_opcode (pstate, UNOP_CAST); }
#line 1936 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 95:
#line 549 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_MUL); }
#line 1942 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 96:
#line 551 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_DIV); }
#line 1948 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 97:
#line 553 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REM); }
#line 1954 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 99:
#line 559 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ADD); }
#line 1960 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 100:
#line 561 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUB); }
#line 1966 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 102:
#line 567 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LSH); }
#line 1972 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 103:
#line 569 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_RSH); }
#line 1978 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 105:
#line 576 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LESS); }
#line 1984 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 106:
#line 578 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GTR); }
#line 1990 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 107:
#line 580 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LEQ); }
#line 1996 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 108:
#line 582 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GEQ); }
#line 2002 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 110:
#line 589 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL); }
#line 2008 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 111:
#line 591 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
#line 2014 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 113:
#line 597 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
#line 2020 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 115:
#line 603 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
#line 2026 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 117:
#line 608 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
#line 2032 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 119:
#line 614 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
#line 2038 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 121:
#line 620 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
#line 2044 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 123:
#line 626 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_COND); }
#line 2050 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 126:
#line 636 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
#line 2056 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 127:
#line 638 "jv-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
		  write_exp_elt_opcode (pstate, (yyvsp[-1].opcode));
		  write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY); }
#line 2064 "jv-exp.c" /* yacc.c:1646  */
    break;

  case 128:
#line 645 "jv-exp.y" /* yacc.c:1646  */
    { push_expression_name (pstate, (yyvsp[0].sval)); }
#line 2070 "jv-exp.c" /* yacc.c:1646  */
    break;


#line 2074 "jv-exp.c" /* yacc.c:1646  */
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
#line 657 "jv-exp.y" /* yacc.c:1906  */

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *par_state,
	      const char *p, int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST limit, limit_div_base;

  int c;
  int base = input_radix;

  struct type *type;

  if (parsed_float)
    {
      const char *suffix;
      int suffix_len;

      if (! parse_float (p, len, &putithere->typed_val_float.dval, &suffix))
	return ERROR;

      suffix_len = p + len - suffix;

      if (suffix_len == 0)
	putithere->typed_val_float.type
	  = parse_type (par_state)->builtin_double;
      else if (suffix_len == 1)
	{
	  /* See if it has `f' or `d' suffix (float or double).  */
	  if (tolower (*suffix) == 'f')
	    putithere->typed_val_float.type =
	      parse_type (par_state)->builtin_float;
	  else if (tolower (*suffix) == 'd')
	    putithere->typed_val_float.type =
	      parse_type (par_state)->builtin_double;
	  else
	    return ERROR;
	}
      else
	return ERROR;

      return FLOATING_POINT_LITERAL;
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

  c = p[len-1];
  /* A paranoid calculation of (1<<64)-1.  */
  limit = (ULONGEST)0xffffffff;
  limit = ((limit << 16) << 16) | limit;
  if (c == 'l' || c == 'L')
    {
      type = parse_java_type (par_state)->builtin_long;
      len--;
    }
  else
    {
      type = parse_java_type (par_state)->builtin_int;
    }
  limit_div_base = limit / (ULONGEST) base;

  while (--len >= 0)
    {
      c = *p++;
      if (c >= '0' && c <= '9')
	c -= '0';
      else if (c >= 'A' && c <= 'Z')
	c -= 'A' - 10;
      else if (c >= 'a' && c <= 'z')
	c -= 'a' - 10;
      else
	return ERROR;	/* Char not a digit */
      if (c >= base)
	return ERROR;
      if (n > limit_div_base
	  || (n *= base) > limit - c)
	error (_("Numeric constant too large"));
      n += c;
	}

  /* If the type is bigger than a 32-bit signed integer can be, implicitly
     promote to long.  Java does not do this, so mark it as
     parse_type (par_state)->builtin_uint64 rather than
     parse_java_type (par_state)->builtin_long.
     0x80000000 will become -0x80000000 instead of 0x80000000L, because we
     don't know the sign at this point.  */
  if (type == parse_java_type (par_state)->builtin_int
      && n > (ULONGEST)0x80000000)
    type = parse_type (par_state)->builtin_uint64;

  putithere->typed_val_int.val = n;
  putithere->typed_val_int.type = type;

  return INTEGER_LITERAL;
}

struct token
{
  char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH}
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
    {"<<", LSH, BINOP_END},
    {">>", RSH, BINOP_END},
    {"==", EQUAL, BINOP_END},
    {"!=", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END}
  };

/* Read one token, getting characters through lexptr.  */

static int
yylex (void)
{
  int c;
  int namelen;
  unsigned int i;
  const char *tokstart;
  const char *tokptr;
  int tempbufindex;
  static char *tempbuf;
  static int tempbufsize;
  
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
      return 0;

    case ' ':
    case '\t':
    case '\n':
      lexptr++;
      goto retry;

    case '\'':
      /* We either have a character constant ('0' or '\177' for example)
	 or we have a quoted symbol reference ('foo(int,int)' in C++
	 for example).  */
      lexptr++;
      c = *lexptr++;
      if (c == '\\')
	c = parse_escape (parse_gdbarch (pstate), &lexptr);
      else if (c == '\'')
	error (_("Empty character constant"));

      yylval.typed_val_int.val = c;
      yylval.typed_val_int.type = parse_java_type (pstate)->builtin_char;

      c = *lexptr++;
      if (c != '\'')
	{
	  namelen = skip_quoted (tokstart) - tokstart;
	  if (namelen > 2)
	    {
	      lexptr = tokstart + namelen;
	      if (lexptr[-1] != '\'')
		error (_("Unmatched single quote"));
	      namelen -= 2;
	      tokstart++;
	      goto tryname;
	    }
	  error (_("Invalid character constant"));
	}
      return INTEGER_LITERAL;

    case '(':
      paren_depth++;
      lexptr++;
      return c;

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
	goto symbol;		/* Nope, must be a symbol.  */
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
	toktype = parse_number (pstate, tokstart, p - tokstart,
				got_dot|got_e, &yylval);
        if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error (_("Invalid number \"%s\""), err_copy);
	  }
	lexptr = p;
	return toktype;
      }

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
    case '[':
    case ']':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      lexptr++;
      return c;

    case '"':

      /* Build the gdb internal form of the input string in tempbuf,
	 translating any standard C escape forms seen.  Note that the
	 buffer is null byte terminated *only* for the convenience of
	 debugging gdb itself and printing the buffer contents when
	 the buffer contains no embedded nulls.  Gdb does not depend
	 upon the buffer being null byte terminated, it uses the length
	 string instead.  This allows gdb to handle C strings (as well
	 as strings in other languages) with embedded null bytes */

      tokptr = ++tokstart;
      tempbufindex = 0;

      do {
	/* Grow the static temp buffer if necessary, including allocating
	   the first one on demand.  */
	if (tempbufindex + 1 >= tempbufsize)
	  {
	    tempbuf = (char *) xrealloc (tempbuf, tempbufsize += 64);
	  }
	switch (*tokptr)
	  {
	  case '\0':
	  case '"':
	    /* Do nothing, loop will terminate.  */
	    break;
	  case '\\':
	    tokptr++;
	    c = parse_escape (parse_gdbarch (pstate), &tokptr);
	    if (c == -1)
	      {
		continue;
	      }
	    tempbuf[tempbufindex++] = c;
	    break;
	  default:
	    tempbuf[tempbufindex++] = *tokptr++;
	    break;
	  }
      } while ((*tokptr != '"') && (*tokptr != '\0'));
      if (*tokptr++ != '"')
	{
	  error (_("Unterminated string in expression"));
	}
      tempbuf[tempbufindex] = '\0';	/* See note above */
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      lexptr = tokptr;
      return (STRING_LITERAL);
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression"), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_'
	|| c == '$'
	|| (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z')
	|| (c >= 'A' && c <= 'Z')
	|| c == '<');
       )
    {
      if (c == '<')
	{
	  int i = namelen;
	  while (tokstart[++i] && tokstart[i] != '>');
	  if (tokstart[i] == '>')
	    namelen = i;
	}
       c = tokstart[++namelen];
     }

  /* The token "if" terminates the expression and is NOT 
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    {
      return 0;
    }

  lexptr += namelen;

  tryname:

  /* Catch specific keywords.  Should be done with a data structure.  */
  switch (namelen)
    {
    case 7:
      if (strncmp (tokstart, "boolean", 7) == 0)
	return BOOLEAN;
      break;
    case 6:
      if (strncmp (tokstart, "double", 6) == 0)      
	return DOUBLE;
      break;
    case 5:
      if (strncmp (tokstart, "short", 5) == 0)
	return SHORT;
      if (strncmp (tokstart, "false", 5) == 0)
	{
	  yylval.lval = 0;
	  return BOOLEAN_LITERAL;
	}
      if (strncmp (tokstart, "super", 5) == 0)
	return SUPER;
      if (strncmp (tokstart, "float", 5) == 0)
	return FLOAT;
      break;
    case 4:
      if (strncmp (tokstart, "long", 4) == 0)
	return LONG;
      if (strncmp (tokstart, "byte", 4) == 0)
	return BYTE;
      if (strncmp (tokstart, "char", 4) == 0)
	return CHAR;
      if (strncmp (tokstart, "true", 4) == 0)
	{
	  yylval.lval = 1;
	  return BOOLEAN_LITERAL;
	}
      break;
    case 3:
      if (strncmp (tokstart, "int", 3) == 0)
	return INT;
      if (strncmp (tokstart, "new", 3) == 0)
	return NEW;
      break;
    default:
      break;
    }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  if (*tokstart == '$')
    {
      write_dollar_variable (pstate, yylval.sval);
      return VARIABLE;
    }

  /* Input names that aren't symbols but ARE valid hex numbers,
     when the input radix permits them, can be names or numbers
     depending on the parse.  Note we support radixes > 16 here.  */
  if (((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10) ||
       (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (pstate, tokstart, namelen, 0, &newlval);
      if (hextype == INTEGER_LITERAL)
	return NAME_OR_INT;
    }
  return IDENTIFIER;
}

int
java_parse (struct parser_state *par_state)
{
  int result;
  struct cleanup *c = make_cleanup_clear_parser_state (&pstate);

  /* Setting up the parser state.  */
  gdb_assert (par_state != NULL);
  pstate = par_state;

  result = yyparse ();
  do_cleanups (c);

  return result;
}

void
yyerror (char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  if (msg)
    error (_("%s: near `%s'"), msg, lexptr);
  else
    error (_("error in expression, near `%s'"), lexptr);
}

static struct type *
java_type_from_name (struct stoken name)
{
  char *tmp = copy_name (name);
  struct type *typ = java_lookup_class (tmp);
  if (typ == NULL || TYPE_CODE (typ) != TYPE_CODE_STRUCT)
    error (_("No class named `%s'"), tmp);
  return typ;
}

/* If NAME is a valid variable name in this scope, push it and return 1.
   Otherwise, return 0.  */

static int
push_variable (struct parser_state *par_state, struct stoken name)
{
  char *tmp = copy_name (name);
  struct field_of_this_result is_a_field_of_this;
  struct block_symbol sym;

  sym = lookup_symbol (tmp, expression_context_block, VAR_DOMAIN,
		       &is_a_field_of_this);
  if (sym.symbol && SYMBOL_CLASS (sym.symbol) != LOC_TYPEDEF)
    {
      if (symbol_read_needs_frame (sym.symbol))
	{
	  if (innermost_block == 0 ||
	      contained_in (sym.block, innermost_block))
	    innermost_block = sym.block;
	}

      write_exp_elt_opcode (par_state, OP_VAR_VALUE);
      write_exp_elt_block (par_state, sym.block);
      write_exp_elt_sym (par_state, sym.symbol);
      write_exp_elt_opcode (par_state, OP_VAR_VALUE);
      return 1;
    }
  if (is_a_field_of_this.type != NULL)
    {
      /* it hangs off of `this'.  Must not inadvertently convert from a
	 method call to data ref.  */
      if (innermost_block == 0 || 
	  contained_in (sym.block, innermost_block))
	innermost_block = sym.block;
      write_exp_elt_opcode (par_state, OP_THIS);
      write_exp_elt_opcode (par_state, OP_THIS);
      write_exp_elt_opcode (par_state, STRUCTOP_PTR);
      write_exp_string (par_state, name);
      write_exp_elt_opcode (par_state, STRUCTOP_PTR);
      return 1;
    }
  return 0;
}

/* Assuming a reference expression has been pushed, emit the
   STRUCTOP_PTR ops to access the field named NAME.  If NAME is a
   qualified name (has '.'), generate a field access for each part.  */

static void
push_fieldnames (struct parser_state *par_state, struct stoken name)
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
	  write_exp_elt_opcode (par_state, STRUCTOP_PTR);
	  write_exp_string (par_state, token);
	  write_exp_elt_opcode (par_state, STRUCTOP_PTR);
	  token.ptr += token.length + 1;
	}
      if (i >= name.length)
	break;
    }
}

/* Helper routine for push_expression_name.
   Handle a qualified name, where DOT_INDEX is the index of the first '.' */

static void
push_qualified_expression_name (struct parser_state *par_state,
				struct stoken name, int dot_index)
{
  struct stoken token;
  char *tmp;
  struct type *typ;

  token.ptr = name.ptr;
  token.length = dot_index;

  if (push_variable (par_state, token))
    {
      token.ptr = name.ptr + dot_index + 1;
      token.length = name.length - dot_index - 1;
      push_fieldnames (par_state, token);
      return;
    }

  token.ptr = name.ptr;
  for (;;)
    {
      token.length = dot_index;
      tmp = copy_name (token);
      typ = java_lookup_class (tmp);
      if (typ != NULL)
	{
	  if (dot_index == name.length)
	    {
	      write_exp_elt_opcode (par_state, OP_TYPE);
	      write_exp_elt_type (par_state, typ);
	      write_exp_elt_opcode (par_state, OP_TYPE);
	      return;
	    }
	  dot_index++;  /* Skip '.' */
	  name.ptr += dot_index;
	  name.length -= dot_index;
	  dot_index = 0;
	  while (dot_index < name.length && name.ptr[dot_index] != '.') 
	    dot_index++;
	  token.ptr = name.ptr;
	  token.length = dot_index;
	  write_exp_elt_opcode (par_state, OP_SCOPE);
	  write_exp_elt_type (par_state, typ);
	  write_exp_string (par_state, token);
	  write_exp_elt_opcode (par_state, OP_SCOPE); 
	  if (dot_index < name.length)
	    {
	      dot_index++;
	      name.ptr += dot_index;
	      name.length -= dot_index;
	      push_fieldnames (par_state, name);
	    }
	  return;
	}
      else if (dot_index >= name.length)
	break;
      dot_index++;  /* Skip '.' */
      while (dot_index < name.length && name.ptr[dot_index] != '.')
	dot_index++;
    }
  error (_("unknown type `%.*s'"), name.length, name.ptr);
}

/* Handle Name in an expression (or LHS).
   Handle VAR, TYPE, TYPE.FIELD1....FIELDN and VAR.FIELD1....FIELDN.  */

static void
push_expression_name (struct parser_state *par_state, struct stoken name)
{
  char *tmp;
  struct type *typ;
  int i;

  for (i = 0;  i < name.length;  i++)
    {
      if (name.ptr[i] == '.')
	{
	  /* It's a Qualified Expression Name.  */
	  push_qualified_expression_name (par_state, name, i);
	  return;
	}
    }

  /* It's a Simple Expression Name.  */
  
  if (push_variable (par_state, name))
    return;
  tmp = copy_name (name);
  typ = java_lookup_class (tmp);
  if (typ != NULL)
    {
      write_exp_elt_opcode (par_state, OP_TYPE);
      write_exp_elt_type (par_state, typ);
      write_exp_elt_opcode (par_state, OP_TYPE);
    }
  else
    {
      struct bound_minimal_symbol msymbol;

      msymbol = lookup_bound_minimal_symbol (tmp);
      if (msymbol.minsym != NULL)
	write_exp_msymbol (par_state, msymbol);
      else if (!have_full_symbols () && !have_partial_symbols ())
	error (_("No symbol table is loaded.  Use the \"file\" command"));
      else
	error (_("No symbol \"%s\" in current context."), tmp);
    }

}


/* The following two routines, copy_exp and insert_exp, aren't specific to
   Java, so they could go in parse.c, but their only purpose is to support
   the parsing kludges we use in this file, so maybe it's best to isolate
   them here.  */

/* Copy the expression whose last element is at index ENDPOS - 1 in EXPR
   into a freshly xmalloc'ed struct expression.  Its language_defn is set
   to null.  */
static struct expression *
copy_exp (struct expression *expr, int endpos)
{
  int len = length_of_subexp (expr, endpos);
  struct expression *newobj
    = (struct expression *) xmalloc (sizeof (*newobj) + EXP_ELEM_TO_BYTES (len));

  newobj->nelts = len;
  memcpy (newobj->elts, expr->elts + endpos - len, EXP_ELEM_TO_BYTES (len));
  newobj->language_defn = 0;

  return newobj;
}

/* Insert the expression NEW into the current expression (expout) at POS.  */
static void
insert_exp (struct parser_state *par_state, int pos, struct expression *newobj)
{
  int newlen = newobj->nelts;
  int i;

  /* Grow expout if necessary.  In this function's only use at present,
     this should never be necessary.  */
  increase_expout_size (par_state, newlen);

  for (i = par_state->expout_ptr - 1; i >= pos; i--)
    par_state->expout->elts[i + newlen] = par_state->expout->elts[i];
  
  memcpy (par_state->expout->elts + pos, newobj->elts,
	  EXP_ELEM_TO_BYTES (newlen));
  par_state->expout_ptr += newlen;
}
