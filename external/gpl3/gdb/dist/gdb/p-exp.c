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
#line 44 "p-exp.y" /* yacc.c:339  */


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "p-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols.  */
#include "block.h"
#include "completer.h"

#define parse_type(ps) builtin_type (parse_gdbarch (ps))

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX pascal_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

void yyerror (const char *);

static char *uptok (const char *, int);

#line 102 "p-exp.c" /* yacc.c:339  */

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
    INT = 258,
    FLOAT = 259,
    STRING = 260,
    FIELDNAME = 261,
    COMPLETE = 262,
    NAME = 263,
    TYPENAME = 264,
    NAME_OR_INT = 265,
    STRUCT = 266,
    CLASS = 267,
    SIZEOF = 268,
    COLONCOLON = 269,
    ERROR = 270,
    VARIABLE = 271,
    THIS = 272,
    TRUEKEYWORD = 273,
    FALSEKEYWORD = 274,
    ABOVE_COMMA = 275,
    ASSIGN = 276,
    NOT = 277,
    OR = 278,
    XOR = 279,
    ANDAND = 280,
    NOTEQUAL = 281,
    LEQ = 282,
    GEQ = 283,
    LSH = 284,
    RSH = 285,
    DIV = 286,
    MOD = 287,
    UNARY = 288,
    INCREMENT = 289,
    DECREMENT = 290,
    ARROW = 291,
    BLOCKNAME = 292
  };
#endif
/* Tokens.  */
#define INT 258
#define FLOAT 259
#define STRING 260
#define FIELDNAME 261
#define COMPLETE 262
#define NAME 263
#define TYPENAME 264
#define NAME_OR_INT 265
#define STRUCT 266
#define CLASS 267
#define SIZEOF 268
#define COLONCOLON 269
#define ERROR 270
#define VARIABLE 271
#define THIS 272
#define TRUEKEYWORD 273
#define FALSEKEYWORD 274
#define ABOVE_COMMA 275
#define ASSIGN 276
#define NOT 277
#define OR 278
#define XOR 279
#define ANDAND 280
#define NOTEQUAL 281
#define LEQ 282
#define GEQ 283
#define LSH 284
#define RSH 285
#define DIV 286
#define MOD 287
#define UNARY 288
#define INCREMENT 289
#define DECREMENT 290
#define ARROW 291
#define BLOCKNAME 292

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 85 "p-exp.y" /* yacc.c:355  */

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
    int voidval;
    const struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  

#line 238 "p-exp.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */
#line 109 "p-exp.y" /* yacc.c:358  */

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *,
			 const char *, int, int, YYSTYPE *);

static struct type *current_type;
static struct internalvar *intvar;
static int leftdiv_is_integer;
static void push_current_type (void);
static void pop_current_type (void);
static int search_field;

#line 267 "p-exp.c" /* yacc.c:358  */

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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   377

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  53
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  77
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  126

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   292

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      48,    51,    40,    38,    20,    39,    46,    41,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      29,    27,    30,     2,    37,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    47,     2,    52,    49,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    21,    22,    23,    24,    25,
      26,    28,    31,    32,    33,    34,    35,    36,    42,    43,
      44,    45,    50
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   191,   191,   191,   200,   201,   204,   211,   212,   217,
     223,   229,   233,   237,   241,   246,   250,   268,   284,   290,
     302,   300,   333,   330,   346,   347,   349,   353,   368,   374,
     378,   378,   401,   405,   409,   413,   417,   421,   425,   431,
     437,   443,   449,   455,   461,   465,   469,   473,   477,   484,
     491,   499,   513,   521,   524,   539,   550,   554,   586,   614,
     632,   643,   658,   674,   675,   706,   775,   786,   790,   792,
     794,   797,   805,   806,   807,   808,   811,   812
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "FLOAT", "STRING", "FIELDNAME",
  "COMPLETE", "NAME", "TYPENAME", "NAME_OR_INT", "STRUCT", "CLASS",
  "SIZEOF", "COLONCOLON", "ERROR", "VARIABLE", "THIS", "TRUEKEYWORD",
  "FALSEKEYWORD", "','", "ABOVE_COMMA", "ASSIGN", "NOT", "OR", "XOR",
  "ANDAND", "'='", "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "LSH", "RSH",
  "DIV", "MOD", "'@'", "'+'", "'-'", "'*'", "'/'", "UNARY", "INCREMENT",
  "DECREMENT", "ARROW", "'.'", "'['", "'('", "'^'", "BLOCKNAME", "')'",
  "']'", "$accept", "start", "$@1", "normal_start", "type_exp", "exp1",
  "exp", "field_exp", "$@2", "$@3", "arglist", "$@4", "block", "variable",
  "qualified_name", "ptype", "type", "typebase", "name",
  "name_not_typename", YY_NULLPTRPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
      44,   275,   276,   277,   278,   279,   280,    61,   281,    60,
      62,   282,   283,   284,   285,   286,   287,    64,    43,    45,
      42,    47,   288,   289,   290,   291,    46,    91,    40,    94,
     292,    41,    93
};
# endif

#define YYPACT_NINF -44

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-44)))

#define YYTABLE_NINF -61

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -44,    20,    90,   -44,   -44,   -44,   -44,   -44,   -44,   -44,
       7,     7,   -41,     7,   -44,   -44,   -44,   -44,    90,    90,
      90,   -39,   -24,    90,    10,    13,   -44,   -44,     8,   231,
       4,    21,   -44,   -44,   -44,   -14,    41,   -44,   -44,   -44,
     -44,   -44,   -44,   -44,    90,   -44,    35,   -14,    35,    35,
      90,    90,     5,   -44,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    90,    90,    90,    90,    90,    90,    90,
      90,    90,   -44,   -44,   -44,   -44,   -44,   -44,   -44,    23,
       7,    90,     7,   119,   -43,   147,   175,   -44,   231,   231,
     256,   280,   303,   324,   324,    31,    31,    31,    31,    76,
      76,    76,    76,   328,   328,    35,    90,    90,    90,   -44,
      44,   203,   -44,   -44,   -44,   -44,   -44,    35,     9,   231,
      11,   -44,   -44,    90,   -44,   231
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     0,     1,    50,    52,    57,    76,    69,    51,
       0,     0,     0,     0,    54,    58,    48,    49,     0,     0,
       0,     0,     0,     0,     0,    77,     3,     5,     4,     7,
       0,     0,    53,    63,    67,     6,    66,    65,    72,    74,
      75,    73,    70,    71,     0,    64,    12,     0,    10,    11,
       0,     0,     0,    68,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    30,    15,    20,    22,     9,    16,    19,    17,
       0,     0,     0,     0,     0,     0,     0,    28,     8,    47,
      46,    45,    44,    38,    39,    42,    43,    40,    41,    36,
      37,    32,    33,    34,    35,    29,     0,     0,    24,    18,
      61,     0,    62,    56,    55,    13,    14,    31,     0,    25,
       0,    27,    21,     0,    23,    26
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -44,   -44,   -44,   -44,   -44,   -20,   -18,   -44,   -44,   -44,
     -44,   -44,   -44,   -44,   -44,   -44,    16,    50,    -7,   -44
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,     2,    26,    27,    28,    29,    30,   107,   108,
     120,   106,    31,    32,    33,    34,    47,    36,    42,    37
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      46,    48,    49,    52,    43,    81,    45,    44,   114,    50,
      77,    78,    38,    39,    40,    38,    39,    40,    35,     8,
       3,    10,    11,    79,    51,    54,    83,   -59,    54,    54,
     109,   123,    85,    86,    81,    80,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    41,    82,    87,    41,   -60,    24,
      84,   122,   124,   111,    65,    66,    67,    68,     0,    69,
      70,    71,    72,   110,    53,   112,     0,    73,    74,    75,
      76,    73,    74,    75,    76,     0,     0,   118,   117,     0,
     119,     0,     0,     4,     5,     6,     0,     0,     7,     8,
       9,    10,    11,    12,    13,   125,    14,    15,    16,    17,
       0,     0,     0,    18,    69,    70,    71,    72,     0,     0,
       0,     0,    73,    74,    75,    76,     0,    19,     0,    20,
       0,     0,     0,    21,    22,     0,     0,     0,    23,    24,
      25,    55,     0,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,     0,    69,    70,    71,
      72,     0,     0,     0,     0,    73,    74,    75,    76,    55,
     113,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,     0,    69,    70,    71,    72,     0,
       0,     0,     0,    73,    74,    75,    76,    55,   115,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,     0,    69,    70,    71,    72,     0,     0,     0,
       0,    73,    74,    75,    76,    55,   116,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
       0,    69,    70,    71,    72,     0,     0,     0,     0,    73,
      74,    75,    76,    55,   121,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,     0,    69,
      70,    71,    72,     0,     0,     0,     0,    73,    74,    75,
      76,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,     0,    69,    70,    71,    72,     0,     0,
       0,     0,    73,    74,    75,    76,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,     0,    69,    70,
      71,    72,     0,     0,     0,     0,    73,    74,    75,    76,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
       0,    69,    70,    71,    72,     0,     0,     0,     0,    73,
      74,    75,    76,    61,    62,    63,    64,    65,    66,    67,
      68,     0,    69,    70,    71,    72,     0,     0,    71,    72,
      73,    74,    75,    76,    73,    74,    75,    76
};

static const yytype_int8 yycheck[] =
{
      18,    19,    20,    23,    11,    48,    13,    48,    51,    48,
       6,     7,     8,     9,    10,     8,     9,    10,     2,     9,
       0,    11,    12,    30,    48,    20,    44,    14,    20,    20,
       7,    20,    50,    51,    48,    14,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    50,    14,    51,    50,    14,    49,
      44,    52,    51,    81,    33,    34,    35,    36,    -1,    38,
      39,    40,    41,    80,    24,    82,    -1,    46,    47,    48,
      49,    46,    47,    48,    49,    -1,    -1,   107,   106,    -1,
     108,    -1,    -1,     3,     4,     5,    -1,    -1,     8,     9,
      10,    11,    12,    13,    14,   123,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    38,    39,    40,    41,    -1,    -1,
      -1,    -1,    46,    47,    48,    49,    -1,    37,    -1,    39,
      -1,    -1,    -1,    43,    44,    -1,    -1,    -1,    48,    49,
      50,    22,    -1,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    -1,    38,    39,    40,
      41,    -1,    -1,    -1,    -1,    46,    47,    48,    49,    22,
      51,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    -1,    38,    39,    40,    41,    -1,
      -1,    -1,    -1,    46,    47,    48,    49,    22,    51,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    -1,    38,    39,    40,    41,    -1,    -1,    -1,
      -1,    46,    47,    48,    49,    22,    51,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      -1,    38,    39,    40,    41,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    22,    51,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    -1,    38,
      39,    40,    41,    -1,    -1,    -1,    -1,    46,    47,    48,
      49,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    -1,    38,    39,    40,    41,    -1,    -1,
      -1,    -1,    46,    47,    48,    49,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    -1,    38,    39,
      40,    41,    -1,    -1,    -1,    -1,    46,    47,    48,    49,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      -1,    38,    39,    40,    41,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    38,    39,    40,    41,    -1,    -1,    40,    41,
      46,    47,    48,    49,    46,    47,    48,    49
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    54,    55,     0,     3,     4,     5,     8,     9,    10,
      11,    12,    13,    14,    16,    17,    18,    19,    23,    37,
      39,    43,    44,    48,    49,    50,    56,    57,    58,    59,
      60,    65,    66,    67,    68,    69,    70,    72,     8,     9,
      10,    50,    71,    71,    48,    71,    59,    69,    59,    59,
      48,    48,    58,    70,    20,    22,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    38,
      39,    40,    41,    46,    47,    48,    49,     6,     7,    71,
      14,    48,    14,    59,    69,    59,    59,    51,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    64,    61,    62,     7,
      71,    59,    71,    51,    51,    51,    51,    59,    58,    59,
      63,    51,    52,    20,    51,    59
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    53,    55,    54,    56,    56,    57,    58,    58,    59,
      59,    59,    59,    59,    59,    60,    59,    59,    59,    59,
      61,    59,    62,    59,    63,    63,    63,    59,    59,    59,
      64,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    65,
      65,    66,    67,    66,    66,    66,    68,    69,    70,    70,
      70,    70,    71,    71,    71,    71,    72,    72
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     3,     2,
       2,     2,     2,     4,     4,     2,     2,     2,     3,     2,
       0,     5,     0,     5,     0,     1,     3,     4,     3,     3,
       0,     4,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     4,     4,     1,     1,     1,
       3,     3,     3,     1,     2,     1,     1,     1,     2,     1,
       2,     2,     1,     1,     1,     1,     1,     1
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
        case 2:
#line 191 "p-exp.y" /* yacc.c:1646  */
    { current_type = NULL;
		  intvar = NULL;
		  search_field = 0;
		  leftdiv_is_integer = 0;
		}
#line 1487 "p-exp.c" /* yacc.c:1646  */
    break;

  case 3:
#line 196 "p-exp.y" /* yacc.c:1646  */
    {}
#line 1493 "p-exp.c" /* yacc.c:1646  */
    break;

  case 6:
#line 205 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPE);
			  write_exp_elt_type (pstate, (yyvsp[0].tval));
			  write_exp_elt_opcode (pstate, OP_TYPE);
			  current_type = (yyvsp[0].tval); }
#line 1502 "p-exp.c" /* yacc.c:1646  */
    break;

  case 8:
#line 213 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_COMMA); }
#line 1508 "p-exp.c" /* yacc.c:1646  */
    break;

  case 9:
#line 218 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_IND);
			  if (current_type)
			    current_type = TYPE_TARGET_TYPE (current_type); }
#line 1516 "p-exp.c" /* yacc.c:1646  */
    break;

  case 10:
#line 224 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ADDR);
			  if (current_type)
			    current_type = TYPE_POINTER_TYPE (current_type); }
#line 1524 "p-exp.c" /* yacc.c:1646  */
    break;

  case 11:
#line 230 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_NEG); }
#line 1530 "p-exp.c" /* yacc.c:1646  */
    break;

  case 12:
#line 234 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
#line 1536 "p-exp.c" /* yacc.c:1646  */
    break;

  case 13:
#line 238 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREINCREMENT); }
#line 1542 "p-exp.c" /* yacc.c:1646  */
    break;

  case 14:
#line 242 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREDECREMENT); }
#line 1548 "p-exp.c" /* yacc.c:1646  */
    break;

  case 15:
#line 247 "p-exp.y" /* yacc.c:1646  */
    { search_field = 1; }
#line 1554 "p-exp.c" /* yacc.c:1646  */
    break;

  case 16:
#line 251 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  search_field = 0;
			  if (current_type)
			    {
			      while (TYPE_CODE (current_type)
				     == TYPE_CODE_PTR)
				current_type =
				  TYPE_TARGET_TYPE (current_type);
			      current_type = lookup_struct_elt_type (
				current_type, (yyvsp[0].sval).ptr, 0);
			    }
			 }
#line 1573 "p-exp.c" /* yacc.c:1646  */
    break;

  case 17:
#line 269 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  search_field = 0;
			  if (current_type)
			    {
			      while (TYPE_CODE (current_type)
				     == TYPE_CODE_PTR)
				current_type =
				  TYPE_TARGET_TYPE (current_type);
			      current_type = lookup_struct_elt_type (
				current_type, (yyvsp[0].sval).ptr, 0);
			    }
			}
#line 1592 "p-exp.c" /* yacc.c:1646  */
    break;

  case 18:
#line 285 "p-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 1601 "p-exp.c" /* yacc.c:1646  */
    break;

  case 19:
#line 291 "p-exp.y" /* yacc.c:1646  */
    { struct stoken s;
			  mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (pstate, s);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 1613 "p-exp.c" /* yacc.c:1646  */
    break;

  case 20:
#line 302 "p-exp.y" /* yacc.c:1646  */
    { const char *arrayname;
			  int arrayfieldindex;
			  arrayfieldindex = is_pascal_string_type (
				current_type, NULL, NULL,
				NULL, NULL, &arrayname);
			  if (arrayfieldindex)
			    {
			      struct stoken stringsval;
			      char *buf;

			      buf = (char *) alloca (strlen (arrayname) + 1);
			      stringsval.ptr = buf;
			      stringsval.length = strlen (arrayname);
			      strcpy (buf, arrayname);
			      current_type = TYPE_FIELD_TYPE (current_type,
				arrayfieldindex - 1);
			      write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			      write_exp_string (pstate, stringsval);
			      write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			    }
			  push_current_type ();  }
#line 1639 "p-exp.c" /* yacc.c:1646  */
    break;

  case 21:
#line 324 "p-exp.y" /* yacc.c:1646  */
    { pop_current_type ();
			  write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT);
			  if (current_type)
			    current_type = TYPE_TARGET_TYPE (current_type); }
#line 1648 "p-exp.c" /* yacc.c:1646  */
    break;

  case 22:
#line 333 "p-exp.y" /* yacc.c:1646  */
    { push_current_type ();
			  start_arglist (); }
#line 1655 "p-exp.c" /* yacc.c:1646  */
    break;

  case 23:
#line 336 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_FUNCALL);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) end_arglist ());
			  write_exp_elt_opcode (pstate, OP_FUNCALL);
			  pop_current_type ();
			  if (current_type)
 	  		    current_type = TYPE_TARGET_TYPE (current_type);
			}
#line 1668 "p-exp.c" /* yacc.c:1646  */
    break;

  case 25:
#line 348 "p-exp.y" /* yacc.c:1646  */
    { arglist_len = 1; }
#line 1674 "p-exp.c" /* yacc.c:1646  */
    break;

  case 26:
#line 350 "p-exp.y" /* yacc.c:1646  */
    { arglist_len++; }
#line 1680 "p-exp.c" /* yacc.c:1646  */
    break;

  case 27:
#line 354 "p-exp.y" /* yacc.c:1646  */
    { if (current_type)
			    {
			      /* Allow automatic dereference of classes.  */
			      if ((TYPE_CODE (current_type) == TYPE_CODE_PTR)
				  && (TYPE_CODE (TYPE_TARGET_TYPE (current_type)) == TYPE_CODE_STRUCT)
				  && (TYPE_CODE ((yyvsp[-3].tval)) == TYPE_CODE_STRUCT))
				write_exp_elt_opcode (pstate, UNOP_IND);
			    }
			  write_exp_elt_opcode (pstate, UNOP_CAST);
			  write_exp_elt_type (pstate, (yyvsp[-3].tval));
			  write_exp_elt_opcode (pstate, UNOP_CAST);
			  current_type = (yyvsp[-3].tval); }
#line 1697 "p-exp.c" /* yacc.c:1646  */
    break;

  case 28:
#line 369 "p-exp.y" /* yacc.c:1646  */
    { }
#line 1703 "p-exp.c" /* yacc.c:1646  */
    break;

  case 29:
#line 375 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_MUL); }
#line 1709 "p-exp.c" /* yacc.c:1646  */
    break;

  case 30:
#line 378 "p-exp.y" /* yacc.c:1646  */
    {
			  if (current_type && is_integral_type (current_type))
			    leftdiv_is_integer = 1;
			}
#line 1718 "p-exp.c" /* yacc.c:1646  */
    break;

  case 31:
#line 383 "p-exp.y" /* yacc.c:1646  */
    {
			  if (leftdiv_is_integer && current_type
			      && is_integral_type (current_type))
			    {
			      write_exp_elt_opcode (pstate, UNOP_CAST);
			      write_exp_elt_type (pstate,
						  parse_type (pstate)
						  ->builtin_long_double);
			      current_type
				= parse_type (pstate)->builtin_long_double;
			      write_exp_elt_opcode (pstate, UNOP_CAST);
			      leftdiv_is_integer = 0;
			    }

			  write_exp_elt_opcode (pstate, BINOP_DIV);
			}
#line 1739 "p-exp.c" /* yacc.c:1646  */
    break;

  case 32:
#line 402 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_INTDIV); }
#line 1745 "p-exp.c" /* yacc.c:1646  */
    break;

  case 33:
#line 406 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REM); }
#line 1751 "p-exp.c" /* yacc.c:1646  */
    break;

  case 34:
#line 410 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ADD); }
#line 1757 "p-exp.c" /* yacc.c:1646  */
    break;

  case 35:
#line 414 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUB); }
#line 1763 "p-exp.c" /* yacc.c:1646  */
    break;

  case 36:
#line 418 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LSH); }
#line 1769 "p-exp.c" /* yacc.c:1646  */
    break;

  case 37:
#line 422 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_RSH); }
#line 1775 "p-exp.c" /* yacc.c:1646  */
    break;

  case 38:
#line 426 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL);
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1783 "p-exp.c" /* yacc.c:1646  */
    break;

  case 39:
#line 432 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL);
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1791 "p-exp.c" /* yacc.c:1646  */
    break;

  case 40:
#line 438 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LEQ);
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1799 "p-exp.c" /* yacc.c:1646  */
    break;

  case 41:
#line 444 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GEQ);
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1807 "p-exp.c" /* yacc.c:1646  */
    break;

  case 42:
#line 450 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LESS);
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1815 "p-exp.c" /* yacc.c:1646  */
    break;

  case 43:
#line 456 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GTR);
			  current_type = parse_type (pstate)->builtin_bool;
			}
#line 1823 "p-exp.c" /* yacc.c:1646  */
    break;

  case 44:
#line 462 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
#line 1829 "p-exp.c" /* yacc.c:1646  */
    break;

  case 45:
#line 466 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
#line 1835 "p-exp.c" /* yacc.c:1646  */
    break;

  case 46:
#line 470 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
#line 1841 "p-exp.c" /* yacc.c:1646  */
    break;

  case 47:
#line 474 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
#line 1847 "p-exp.c" /* yacc.c:1646  */
    break;

  case 48:
#line 478 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_BOOL);
			  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].lval));
			  current_type = parse_type (pstate)->builtin_bool;
			  write_exp_elt_opcode (pstate, OP_BOOL); }
#line 1856 "p-exp.c" /* yacc.c:1646  */
    break;

  case 49:
#line 485 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_BOOL);
			  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].lval));
			  current_type = parse_type (pstate)->builtin_bool;
			  write_exp_elt_opcode (pstate, OP_BOOL); }
#line 1865 "p-exp.c" /* yacc.c:1646  */
    break;

  case 50:
#line 492 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, (yyvsp[0].typed_val_int).type);
			  current_type = (yyvsp[0].typed_val_int).type;
			  write_exp_elt_longcst (pstate, (LONGEST)((yyvsp[0].typed_val_int).val));
			  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1875 "p-exp.c" /* yacc.c:1646  */
    break;

  case 51:
#line 500 "p-exp.y" /* yacc.c:1646  */
    { YYSTYPE val;
			  parse_number (pstate, (yyvsp[0].ssym).stoken.ptr,
					(yyvsp[0].ssym).stoken.length, 0, &val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, val.typed_val_int.type);
			  current_type = val.typed_val_int.type;
			  write_exp_elt_longcst (pstate, (LONGEST)
						 val.typed_val_int.val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			}
#line 1890 "p-exp.c" /* yacc.c:1646  */
    break;

  case 52:
#line 514 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DOUBLE);
			  write_exp_elt_type (pstate, (yyvsp[0].typed_val_float).type);
			  current_type = (yyvsp[0].typed_val_float).type;
			  write_exp_elt_dblcst (pstate, (yyvsp[0].typed_val_float).dval);
			  write_exp_elt_opcode (pstate, OP_DOUBLE); }
#line 1900 "p-exp.c" /* yacc.c:1646  */
    break;

  case 54:
#line 527 "p-exp.y" /* yacc.c:1646  */
    {  if (intvar) {
 			     struct value * val, * mark;

			     mark = value_mark ();
 			     val = value_of_internalvar (parse_gdbarch (pstate),
 							 intvar);
 			     current_type = value_type (val);
			     value_release_to_mark (mark);
 			   }
 			}
#line 1915 "p-exp.c" /* yacc.c:1646  */
    break;

  case 55:
#line 540 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate,
					    parse_type (pstate)->builtin_int);
			  current_type = parse_type (pstate)->builtin_int;
			  (yyvsp[-1].tval) = check_typedef ((yyvsp[-1].tval));
			  write_exp_elt_longcst (pstate,
						 (LONGEST) TYPE_LENGTH ((yyvsp[-1].tval)));
			  write_exp_elt_opcode (pstate, OP_LONG); }
#line 1928 "p-exp.c" /* yacc.c:1646  */
    break;

  case 56:
#line 551 "p-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_SIZEOF);
			  current_type = parse_type (pstate)->builtin_int; }
#line 1935 "p-exp.c" /* yacc.c:1646  */
    break;

  case 57:
#line 555 "p-exp.y" /* yacc.c:1646  */
    { /* C strings are converted into array constants with
			     an explicit null byte added at the end.  Thus
			     the array upper bound is the string length.
			     There is no such thing in C as a completely empty
			     string.  */
			  const char *sp = (yyvsp[0].sval).ptr; int count = (yyvsp[0].sval).length;

			  while (count-- > 0)
			    {
			      write_exp_elt_opcode (pstate, OP_LONG);
			      write_exp_elt_type (pstate,
						  parse_type (pstate)
						  ->builtin_char);
			      write_exp_elt_longcst (pstate,
						     (LONGEST) (*sp++));
			      write_exp_elt_opcode (pstate, OP_LONG);
			    }
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate,
					      parse_type (pstate)
					      ->builtin_char);
			  write_exp_elt_longcst (pstate, (LONGEST)'\0');
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_opcode (pstate, OP_ARRAY);
			  write_exp_elt_longcst (pstate, (LONGEST) 0);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) ((yyvsp[0].sval).length));
			  write_exp_elt_opcode (pstate, OP_ARRAY); }
#line 1968 "p-exp.c" /* yacc.c:1646  */
    break;

  case 58:
#line 587 "p-exp.y" /* yacc.c:1646  */
    {
			  struct value * this_val;
			  struct type * this_type;
			  write_exp_elt_opcode (pstate, OP_THIS);
			  write_exp_elt_opcode (pstate, OP_THIS);
			  /* We need type of this.  */
			  this_val
			    = value_of_this_silent (parse_language (pstate));
			  if (this_val)
			    this_type = value_type (this_val);
			  else
			    this_type = NULL;
			  if (this_type)
			    {
			      if (TYPE_CODE (this_type) == TYPE_CODE_PTR)
				{
				  this_type = TYPE_TARGET_TYPE (this_type);
				  write_exp_elt_opcode (pstate, UNOP_IND);
				}
			    }

			  current_type = this_type;
			}
#line 1996 "p-exp.c" /* yacc.c:1646  */
    break;

  case 59:
#line 615 "p-exp.y" /* yacc.c:1646  */
    {
			  if ((yyvsp[0].ssym).sym.symbol != 0)
			      (yyval.bval) = SYMBOL_BLOCK_VALUE ((yyvsp[0].ssym).sym.symbol);
			  else
			    {
			      struct symtab *tem =
				  lookup_symtab (copy_name ((yyvsp[0].ssym).stoken));
			      if (tem)
				(yyval.bval) = BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (tem),
							STATIC_BLOCK);
			      else
				error (_("No file or function \"%s\"."),
				       copy_name ((yyvsp[0].ssym).stoken));
			    }
			}
#line 2016 "p-exp.c" /* yacc.c:1646  */
    break;

  case 60:
#line 633 "p-exp.y" /* yacc.c:1646  */
    { struct symbol *tem
			    = lookup_symbol (copy_name ((yyvsp[0].sval)), (yyvsp[-2].bval),
					     VAR_DOMAIN, NULL).symbol;

			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy_name ((yyvsp[0].sval)));
			  (yyval.bval) = SYMBOL_BLOCK_VALUE (tem); }
#line 2029 "p-exp.c" /* yacc.c:1646  */
    break;

  case 61:
#line 644 "p-exp.y" /* yacc.c:1646  */
    { struct block_symbol sym;

			  sym = lookup_symbol (copy_name ((yyvsp[0].sval)), (yyvsp[-2].bval),
					       VAR_DOMAIN, NULL);
			  if (sym.symbol == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy_name ((yyvsp[0].sval)));

			  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			  write_exp_elt_block (pstate, sym.block);
			  write_exp_elt_sym (pstate, sym.symbol);
			  write_exp_elt_opcode (pstate, OP_VAR_VALUE); }
#line 2046 "p-exp.c" /* yacc.c:1646  */
    break;

  case 62:
#line 659 "p-exp.y" /* yacc.c:1646  */
    {
			  struct type *type = (yyvsp[-2].tval);

			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION)
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_NAME (type));

			  write_exp_elt_opcode (pstate, OP_SCOPE);
			  write_exp_elt_type (pstate, type);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, OP_SCOPE);
			}
#line 2064 "p-exp.c" /* yacc.c:1646  */
    break;

  case 64:
#line 676 "p-exp.y" /* yacc.c:1646  */
    {
			  char *name = copy_name ((yyvsp[0].sval));
			  struct symbol *sym;
			  struct bound_minimal_symbol msymbol;

			  sym =
			    lookup_symbol (name, (const struct block *) NULL,
					   VAR_DOMAIN, NULL).symbol;
			  if (sym)
			    {
			      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			      write_exp_elt_block (pstate, NULL);
			      write_exp_elt_sym (pstate, sym);
			      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			      break;
			    }

			  msymbol = lookup_bound_minimal_symbol (name);
			  if (msymbol.minsym != NULL)
			    write_exp_msymbol (pstate, msymbol);
			  else if (!have_full_symbols ()
				   && !have_partial_symbols ())
			    error (_("No symbol table is loaded.  "
				   "Use the \"file\" command."));
			  else
			    error (_("No symbol \"%s\" in current context."),
				   name);
			}
#line 2097 "p-exp.c" /* yacc.c:1646  */
    break;

  case 65:
#line 707 "p-exp.y" /* yacc.c:1646  */
    { struct block_symbol sym = (yyvsp[0].ssym).sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				{
				  if (innermost_block == 0
				      || contained_in (sym.block,
						       innermost_block))
				    innermost_block = sym.block;
				}

			      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			      write_exp_elt_block (pstate, sym.block);
			      write_exp_elt_sym (pstate, sym.symbol);
			      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			      current_type = sym.symbol->type; }
			  else if ((yyvsp[0].ssym).is_a_field_of_this)
			    {
			      struct value * this_val;
			      struct type * this_type;
			      /* Object pascal: it hangs off of `this'.  Must
			         not inadvertently convert from a method call
				 to data ref.  */
			      if (innermost_block == 0
				  || contained_in (sym.block,
						   innermost_block))
				innermost_block = sym.block;
			      write_exp_elt_opcode (pstate, OP_THIS);
			      write_exp_elt_opcode (pstate, OP_THIS);
			      write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			      write_exp_string (pstate, (yyvsp[0].ssym).stoken);
			      write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			      /* We need type of this.  */
			      this_val
				= value_of_this_silent (parse_language (pstate));
			      if (this_val)
				this_type = value_type (this_val);
			      else
				this_type = NULL;
			      if (this_type)
				current_type = lookup_struct_elt_type (
				  this_type,
				  copy_name ((yyvsp[0].ssym).stoken), 0);
			      else
				current_type = NULL;
			    }
			  else
			    {
			      struct bound_minimal_symbol msymbol;
			      char *arg = copy_name ((yyvsp[0].ssym).stoken);

			      msymbol =
				lookup_bound_minimal_symbol (arg);
			      if (msymbol.minsym != NULL)
				write_exp_msymbol (pstate, msymbol);
			      else if (!have_full_symbols ()
				       && !have_partial_symbols ())
				error (_("No symbol table is loaded.  "
				       "Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       copy_name ((yyvsp[0].ssym).stoken));
			    }
			}
#line 2167 "p-exp.c" /* yacc.c:1646  */
    break;

  case 68:
#line 791 "p-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_pointer_type ((yyvsp[0].tval)); }
#line 2173 "p-exp.c" /* yacc.c:1646  */
    break;

  case 69:
#line 793 "p-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 2179 "p-exp.c" /* yacc.c:1646  */
    break;

  case 70:
#line 795 "p-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
					      expression_context_block); }
#line 2186 "p-exp.c" /* yacc.c:1646  */
    break;

  case 71:
#line 798 "p-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
					      expression_context_block); }
#line 2193 "p-exp.c" /* yacc.c:1646  */
    break;

  case 72:
#line 805 "p-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 2199 "p-exp.c" /* yacc.c:1646  */
    break;

  case 73:
#line 806 "p-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 2205 "p-exp.c" /* yacc.c:1646  */
    break;

  case 74:
#line 807 "p-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].tsym).stoken; }
#line 2211 "p-exp.c" /* yacc.c:1646  */
    break;

  case 75:
#line 808 "p-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 2217 "p-exp.c" /* yacc.c:1646  */
    break;


#line 2221 "p-exp.c" /* yacc.c:1646  */
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
#line 822 "p-exp.y" /* yacc.c:1906  */


/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *par_state,
	      const char *p, int len, int parsed_float, YYSTYPE *putithere)
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
      if (! parse_c_float (parse_gdbarch (par_state), p, len,
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
      && (un >> (gdbarch_int_bit (parse_gdbarch (par_state)) - 2)) == 0)
    {
      high_bit
	= ((ULONGEST)1) << (gdbarch_int_bit (parse_gdbarch (par_state)) - 1);

      /* A large decimal (not hex or octal) constant (between INT_MAX
	 and UINT_MAX) is a long or unsigned long, according to ANSI,
	 never an unsigned int, but this code treats it as unsigned
	 int.  This probably should be fixed.  GCC gives a warning on
	 such constants.  */

      unsigned_type = parse_type (par_state)->builtin_unsigned_int;
      signed_type = parse_type (par_state)->builtin_int;
    }
  else if (long_p <= 1
	   && (un >> (gdbarch_long_bit (parse_gdbarch (par_state)) - 2)) == 0)
    {
      high_bit
	= ((ULONGEST)1) << (gdbarch_long_bit (parse_gdbarch (par_state)) - 1);
      unsigned_type = parse_type (par_state)->builtin_unsigned_long;
      signed_type = parse_type (par_state)->builtin_long;
    }
  else
    {
      int shift;
      if (sizeof (ULONGEST) * HOST_CHAR_BIT
	  < gdbarch_long_long_bit (parse_gdbarch (par_state)))
	/* A long long does not fit in a LONGEST.  */
	shift = (sizeof (ULONGEST) * HOST_CHAR_BIT - 1);
      else
	shift = (gdbarch_long_long_bit (parse_gdbarch (par_state)) - 1);
      high_bit = (ULONGEST) 1 << shift;
      unsigned_type = parse_type (par_state)->builtin_unsigned_long_long;
      signed_type = parse_type (par_state)->builtin_long_long;
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


struct type_push
{
  struct type *stored;
  struct type_push *next;
};

static struct type_push *tp_top = NULL;

static void
push_current_type (void)
{
  struct type_push *tpnew;
  tpnew = (struct type_push *) xmalloc (sizeof (struct type_push));
  tpnew->next = tp_top;
  tpnew->stored = current_type;
  current_type = NULL;
  tp_top = tpnew;
}

static void
pop_current_type (void)
{
  struct type_push *tp = tp_top;
  if (tp)
    {
      current_type = tp->stored;
      tp_top = tp->next;
      xfree (tp);
    }
}

struct token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {"shr", RSH, BINOP_END},
    {"shl", LSH, BINOP_END},
    {"and", ANDAND, BINOP_END},
    {"div", DIV, BINOP_END},
    {"not", NOT, BINOP_END},
    {"mod", MOD, BINOP_END},
    {"inc", INCREMENT, BINOP_END},
    {"dec", DECREMENT, BINOP_END},
    {"xor", XOR, BINOP_END}
  };

static const struct token tokentab2[] =
  {
    {"or", OR, BINOP_END},
    {"<>", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END},
    {":=", ASSIGN, BINOP_END},
    {"::", COLONCOLON, BINOP_END} };

/* Allocate uppercased var: */
/* make an uppercased copy of tokstart.  */
static char *
uptok (const char *tokstart, int namelen)
{
  int i;
  char *uptokstart = (char *)xmalloc(namelen+1);
  for (i = 0;i <= namelen;i++)
    {
      if ((tokstart[i]>='a' && tokstart[i]<='z'))
        uptokstart[i] = tokstart[i]-('a'-'A');
      else
        uptokstart[i] = tokstart[i];
    }
  uptokstart[namelen]='\0';
  return uptokstart;
}

/* Read one token, getting characters through lexptr.  */

static int
yylex (void)
{
  int c;
  int namelen;
  unsigned int i;
  const char *tokstart;
  char *uptokstart;
  const char *tokptr;
  int explen, tempbufindex;
  static char *tempbuf;
  static int tempbufsize;

 retry:

  prev_lexptr = lexptr;

  tokstart = lexptr;
  explen = strlen (lexptr);

  /* See if it is a special token of length 3.  */
  if (explen > 2)
    for (i = 0; i < sizeof (tokentab3) / sizeof (tokentab3[0]); i++)
      if (strncasecmp (tokstart, tokentab3[i].oper, 3) == 0
          && (!isalpha (tokentab3[i].oper[0]) || explen == 3
              || (!isalpha (tokstart[3])
		  && !isdigit (tokstart[3]) && tokstart[3] != '_')))
        {
          lexptr += 3;
          yylval.opcode = tokentab3[i].opcode;
          return tokentab3[i].token;
        }

  /* See if it is a special token of length 2.  */
  if (explen > 1)
  for (i = 0; i < sizeof (tokentab2) / sizeof (tokentab2[0]); i++)
      if (strncasecmp (tokstart, tokentab2[i].oper, 2) == 0
          && (!isalpha (tokentab2[i].oper[0]) || explen == 2
              || (!isalpha (tokstart[2])
		  && !isdigit (tokstart[2]) && tokstart[2] != '_')))
        {
          lexptr += 2;
          yylval.opcode = tokentab2[i].opcode;
          return tokentab2[i].token;
        }

  switch (c = *tokstart)
    {
    case 0:
      if (search_field && parse_completion)
	return COMPLETE;
      else
       return 0;

    case ' ':
    case '\t':
    case '\n':
      lexptr++;
      goto retry;

    case '\'':
      /* We either have a character constant ('0' or '\177' for example)
	 or we have a quoted symbol reference ('foo(int,int)' in object pascal
	 for example).  */
      lexptr++;
      c = *lexptr++;
      if (c == '\\')
	c = parse_escape (parse_gdbarch (pstate), &lexptr);
      else if (c == '\'')
	error (_("Empty character constant."));

      yylval.typed_val_int.val = c;
      yylval.typed_val_int.type = parse_type (pstate)->builtin_char;

      c = *lexptr++;
      if (c != '\'')
	{
	  namelen = skip_quoted (tokstart) - tokstart;
	  if (namelen > 2)
	    {
	      lexptr = tokstart + namelen;
	      if (lexptr[-1] != '\'')
		error (_("Unmatched single quote."));
	      namelen -= 2;
              tokstart++;
              uptokstart = uptok(tokstart,namelen);
	      goto tryname;
	    }
	  error (_("Invalid character constant."));
	}
      return INT;

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
	{
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
	else if (c == '0' && (p[1]=='t' || p[1]=='T'
			      || p[1]=='d' || p[1]=='D'))
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
	toktype = parse_number (pstate, tokstart,
				p - tokstart, got_dot | got_e, &yylval);
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

    case '+':
    case '-':
    case '*':
    case '/':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '@':
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
	 as strings in other languages) with embedded null bytes.  */

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
	    ++tokptr;
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
	  error (_("Unterminated string in expression."));
	}
      tempbuf[tempbufindex] = '\0';	/* See note above.  */
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      lexptr = tokptr;
      return (STRING);
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
	  int i = namelen;
	  int nesting_level = 1;
	  while (tokstart[++i])
	    {
	      if (tokstart[i] == '<')
		nesting_level++;
	      else if (tokstart[i] == '>')
		{
		  if (--nesting_level == 0)
		    break;
		}
	    }
	  if (tokstart[i] == '>')
	    namelen = i;
	  else
	    break;
	}

      /* do NOT uppercase internals because of registers !!!  */
      c = tokstart[++namelen];
    }

  uptokstart = uptok(tokstart,namelen);

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && uptokstart[0] == 'I' && uptokstart[1] == 'F')
    {
      xfree (uptokstart);
      return 0;
    }

  lexptr += namelen;

  tryname:

  /* Catch specific keywords.  Should be done with a data structure.  */
  switch (namelen)
    {
    case 6:
      if (strcmp (uptokstart, "OBJECT") == 0)
	{
	  xfree (uptokstart);
	  return CLASS;
	}
      if (strcmp (uptokstart, "RECORD") == 0)
	{
	  xfree (uptokstart);
	  return STRUCT;
	}
      if (strcmp (uptokstart, "SIZEOF") == 0)
	{
	  xfree (uptokstart);
	  return SIZEOF;
	}
      break;
    case 5:
      if (strcmp (uptokstart, "CLASS") == 0)
	{
	  xfree (uptokstart);
	  return CLASS;
	}
      if (strcmp (uptokstart, "FALSE") == 0)
	{
          yylval.lval = 0;
	  xfree (uptokstart);
          return FALSEKEYWORD;
        }
      break;
    case 4:
      if (strcmp (uptokstart, "TRUE") == 0)
	{
          yylval.lval = 1;
	  xfree (uptokstart);
  	  return TRUEKEYWORD;
        }
      if (strcmp (uptokstart, "SELF") == 0)
        {
          /* Here we search for 'this' like
             inserted in FPC stabs debug info.  */
	  static const char this_name[] = "this";

	  if (lookup_symbol (this_name, expression_context_block,
			     VAR_DOMAIN, NULL).symbol)
	    {
	      xfree (uptokstart);
	      return THIS;
	    }
	}
      break;
    default:
      break;
    }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  if (*tokstart == '$')
    {
      char *tmp;

      /* $ is the normal prefix for pascal hexadecimal values
        but this conflicts with the GDB use for debugger variables
        so in expression to enter hexadecimal values
        we still need to use C syntax with 0xff  */
      write_dollar_variable (pstate, yylval.sval);
      tmp = (char *) alloca (namelen + 1);
      memcpy (tmp, tokstart, namelen);
      tmp[namelen] = '\0';
      intvar = lookup_only_internalvar (tmp + 1);
      xfree (uptokstart);
      return VARIABLE;
    }

  /* Use token-type BLOCKNAME for symbols that happen to be defined as
     functions or symtabs.  If this is not so, then ...
     Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    char *tmp = copy_name (yylval.sval);
    struct symbol *sym;
    struct field_of_this_result is_a_field_of_this;
    int is_a_field = 0;
    int hextype;

    is_a_field_of_this.type = NULL;
    if (search_field && current_type)
      is_a_field = (lookup_struct_elt_type (current_type, tmp, 1) != NULL);
    if (is_a_field)
      sym = NULL;
    else
      sym = lookup_symbol (tmp, expression_context_block,
			   VAR_DOMAIN, &is_a_field_of_this).symbol;
    /* second chance uppercased (as Free Pascal does).  */
    if (!sym && is_a_field_of_this.type == NULL && !is_a_field)
      {
       for (i = 0; i <= namelen; i++)
         {
           if ((tmp[i] >= 'a' && tmp[i] <= 'z'))
             tmp[i] -= ('a'-'A');
         }
       if (search_field && current_type)
	 is_a_field = (lookup_struct_elt_type (current_type, tmp, 1) != NULL);
       if (is_a_field)
	 sym = NULL;
       else
	 sym = lookup_symbol (tmp, expression_context_block,
			      VAR_DOMAIN, &is_a_field_of_this).symbol;
      }
    /* Third chance Capitalized (as GPC does).  */
    if (!sym && is_a_field_of_this.type == NULL && !is_a_field)
      {
       for (i = 0; i <= namelen; i++)
         {
           if (i == 0)
             {
              if ((tmp[i] >= 'a' && tmp[i] <= 'z'))
                tmp[i] -= ('a'-'A');
             }
           else
           if ((tmp[i] >= 'A' && tmp[i] <= 'Z'))
             tmp[i] -= ('A'-'a');
          }
       if (search_field && current_type)
	 is_a_field = (lookup_struct_elt_type (current_type, tmp, 1) != NULL);
       if (is_a_field)
	 sym = NULL;
       else
	 sym = lookup_symbol (tmp, expression_context_block,
			      VAR_DOMAIN, &is_a_field_of_this).symbol;
      }

    if (is_a_field || (is_a_field_of_this.type != NULL))
      {
	tempbuf = (char *) xrealloc (tempbuf, namelen + 1);
	strncpy (tempbuf, tmp, namelen);
	tempbuf [namelen] = 0;
	yylval.sval.ptr = tempbuf;
	yylval.sval.length = namelen;
	yylval.ssym.sym.symbol = NULL;
	yylval.ssym.sym.block = NULL;
	xfree (uptokstart);
        yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	if (is_a_field)
	  return FIELDNAME;
	else
	  return NAME;
      }
    /* Call lookup_symtab, not lookup_partial_symtab, in case there are
       no psymtabs (coff, xcoff, or some future change to blow away the
       psymtabs once once symbols are read).  */
    if ((sym && SYMBOL_CLASS (sym) == LOC_BLOCK)
        || lookup_symtab (tmp))
      {
	yylval.ssym.sym.symbol = sym;
	yylval.ssym.sym.block = NULL;
	yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	xfree (uptokstart);
	return BLOCKNAME;
      }
    if (sym && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
        {
#if 1
	  /* Despite the following flaw, we need to keep this code enabled.
	     Because we can get called from check_stub_method, if we don't
	     handle nested types then it screws many operations in any
	     program which uses nested types.  */
	  /* In "A::x", if x is a member function of A and there happens
	     to be a type (nested or not, since the stabs don't make that
	     distinction) named x, then this code incorrectly thinks we
	     are dealing with nested types rather than a member function.  */

	  const char *p;
	  const char *namestart;
	  struct symbol *best_sym;

	  /* Look ahead to detect nested types.  This probably should be
	     done in the grammar, but trying seemed to introduce a lot
	     of shift/reduce and reduce/reduce conflicts.  It's possible
	     that it could be done, though.  Or perhaps a non-grammar, but
	     less ad hoc, approach would work well.  */

	  /* Since we do not currently have any way of distinguishing
	     a nested type from a non-nested one (the stabs don't tell
	     us whether a type is nested), we just ignore the
	     containing type.  */

	  p = lexptr;
	  best_sym = sym;
	  while (1)
	    {
	      /* Skip whitespace.  */
	      while (*p == ' ' || *p == '\t' || *p == '\n')
		++p;
	      if (*p == ':' && p[1] == ':')
		{
		  /* Skip the `::'.  */
		  p += 2;
		  /* Skip whitespace.  */
		  while (*p == ' ' || *p == '\t' || *p == '\n')
		    ++p;
		  namestart = p;
		  while (*p == '_' || *p == '$' || (*p >= '0' && *p <= '9')
			 || (*p >= 'a' && *p <= 'z')
			 || (*p >= 'A' && *p <= 'Z'))
		    ++p;
		  if (p != namestart)
		    {
		      struct symbol *cur_sym;
		      /* As big as the whole rest of the expression, which is
			 at least big enough.  */
		      char *ncopy
			= (char *) alloca (strlen (tmp) + strlen (namestart)
					   + 3);
		      char *tmp1;

		      tmp1 = ncopy;
		      memcpy (tmp1, tmp, strlen (tmp));
		      tmp1 += strlen (tmp);
		      memcpy (tmp1, "::", 2);
		      tmp1 += 2;
		      memcpy (tmp1, namestart, p - namestart);
		      tmp1[p - namestart] = '\0';
		      cur_sym = lookup_symbol (ncopy, expression_context_block,
					       VAR_DOMAIN, NULL).symbol;
		      if (cur_sym)
			{
			  if (SYMBOL_CLASS (cur_sym) == LOC_TYPEDEF)
			    {
			      best_sym = cur_sym;
			      lexptr = p;
			    }
			  else
			    break;
			}
		      else
			break;
		    }
		  else
		    break;
		}
	      else
		break;
	    }

	  yylval.tsym.type = SYMBOL_TYPE (best_sym);
#else /* not 0 */
	  yylval.tsym.type = SYMBOL_TYPE (sym);
#endif /* not 0 */
	  xfree (uptokstart);
	  return TYPENAME;
        }
    yylval.tsym.type
      = language_lookup_primitive_type (parse_language (pstate),
					parse_gdbarch (pstate), tmp);
    if (yylval.tsym.type != NULL)
      {
	xfree (uptokstart);
	return TYPENAME;
      }

    /* Input names that aren't symbols but ARE valid hex numbers,
       when the input radix permits them, can be names or numbers
       depending on the parse.  Note we support radixes > 16 here.  */
    if (!sym
        && ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
            || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
      {
 	YYSTYPE newlval;	/* Its value is ignored.  */
	hextype = parse_number (pstate, tokstart, namelen, 0, &newlval);
	if (hextype == INT)
	  {
	    yylval.ssym.sym.symbol = sym;
	    yylval.ssym.sym.block = NULL;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	    xfree (uptokstart);
	    return NAME_OR_INT;
	  }
      }

    xfree(uptokstart);
    /* Any other kind of symbol.  */
    yylval.ssym.sym.symbol = sym;
    yylval.ssym.sym.block = NULL;
    return NAME;
  }
}

int
pascal_parse (struct parser_state *par_state)
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
yyerror (const char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), (msg ? msg : "error"), lexptr);
}
