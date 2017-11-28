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
#line 36 "ada-exp.y" /* yacc.c:339  */


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "ada-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "frame.h"
#include "block.h"

#define parse_type(ps) builtin_type (parse_gdbarch (ps))

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX ada_
#include "yy-remap.h"

struct name_info {
  struct symbol *sym;
  struct minimal_symbol *msym;
  const struct block *block;
  struct stoken stoken;
};

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

static struct stoken empty_stoken = { "", 0 };

/* If expression is in the context of TYPE'(...), then TYPE, else
 * NULL.  */
static struct type *type_qualifier;

int yyparse (void);

static int yylex (void);

void yyerror (const char *);

static void write_int (struct parser_state *, LONGEST, struct type *);

static void write_object_renaming (struct parser_state *,
				   const struct block *, const char *, int,
				   const char *, int);

static struct type* write_var_or_type (struct parser_state *,
				       const struct block *, struct stoken);

static void write_name_assoc (struct parser_state *, struct stoken);

static void write_exp_op_with_string (struct parser_state *, enum exp_opcode,
				      struct stoken);

static const struct block *block_lookup (const struct block *, const char *);

static LONGEST convert_char_literal (struct type *, LONGEST);

static void write_ambiguous_var (struct parser_state *,
				 const struct block *, char *, int);

static struct type *type_int (struct parser_state *);

static struct type *type_long (struct parser_state *);

static struct type *type_long_long (struct parser_state *);

static struct type *type_float (struct parser_state *);

static struct type *type_double (struct parser_state *);

static struct type *type_long_double (struct parser_state *);

static struct type *type_char (struct parser_state *);

static struct type *type_boolean (struct parser_state *);

static struct type *type_system_address (struct parser_state *);


#line 153 "ada-exp.c" /* yacc.c:339  */

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
    NULL_PTR = 259,
    CHARLIT = 260,
    FLOAT = 261,
    TRUEKEYWORD = 262,
    FALSEKEYWORD = 263,
    COLONCOLON = 264,
    STRING = 265,
    NAME = 266,
    DOT_ID = 267,
    DOT_ALL = 268,
    SPECIAL_VARIABLE = 269,
    ASSIGN = 270,
    _AND_ = 271,
    OR = 272,
    XOR = 273,
    THEN = 274,
    ELSE = 275,
    NOTEQUAL = 276,
    LEQ = 277,
    GEQ = 278,
    IN = 279,
    DOTDOT = 280,
    UNARY = 281,
    MOD = 282,
    REM = 283,
    STARSTAR = 284,
    ABS = 285,
    NOT = 286,
    VAR = 287,
    ARROW = 288,
    TICK_ACCESS = 289,
    TICK_ADDRESS = 290,
    TICK_FIRST = 291,
    TICK_LAST = 292,
    TICK_LENGTH = 293,
    TICK_MAX = 294,
    TICK_MIN = 295,
    TICK_MODULUS = 296,
    TICK_POS = 297,
    TICK_RANGE = 298,
    TICK_SIZE = 299,
    TICK_TAG = 300,
    TICK_VAL = 301,
    NEW = 302,
    OTHERS = 303
  };
#endif
/* Tokens.  */
#define INT 258
#define NULL_PTR 259
#define CHARLIT 260
#define FLOAT 261
#define TRUEKEYWORD 262
#define FALSEKEYWORD 263
#define COLONCOLON 264
#define STRING 265
#define NAME 266
#define DOT_ID 267
#define DOT_ALL 268
#define SPECIAL_VARIABLE 269
#define ASSIGN 270
#define _AND_ 271
#define OR 272
#define XOR 273
#define THEN 274
#define ELSE 275
#define NOTEQUAL 276
#define LEQ 277
#define GEQ 278
#define IN 279
#define DOTDOT 280
#define UNARY 281
#define MOD 282
#define REM 283
#define STARSTAR 284
#define ABS 285
#define NOT 286
#define VAR 287
#define ARROW 288
#define TICK_ACCESS 289
#define TICK_ADDRESS 290
#define TICK_FIRST 291
#define TICK_LAST 292
#define TICK_LENGTH 293
#define TICK_MAX 294
#define TICK_MIN 295
#define TICK_MODULUS 296
#define TICK_POS 297
#define TICK_RANGE 298
#define TICK_SIZE 299
#define TICK_TAG 300
#define TICK_VAL 301
#define NEW 302
#define OTHERS 303

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 124 "ada-exp.y" /* yacc.c:355  */

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val;
    struct {
      DOUBLEST dval;
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    const struct block *bval;
    struct internalvar *ivar;
  

#line 303 "ada-exp.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */

#line 320 "ada-exp.c" /* yacc.c:358  */

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
#define YYFINAL  57
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   746

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  69
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  31
/* YYNRULES -- Number of rules.  */
#define YYNRULES  122
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  233

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
       2,     2,     2,     2,     2,     2,     2,     2,    32,    64,
      58,    63,    34,    30,    65,    31,    57,    35,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    62,
      23,    21,    24,     2,    29,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    59,     2,    68,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    66,    43,    67,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    22,    25,    26,    27,
      28,    33,    36,    37,    38,    39,    40,    41,    42,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    60,    61
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   189,   189,   193,   194,   196,   201,   205,   210,   216,
     235,   235,   247,   251,   253,   261,   272,   282,   286,   289,
     292,   296,   300,   304,   308,   311,   313,   315,   317,   321,
     334,   338,   342,   346,   350,   354,   358,   362,   366,   370,
     373,   377,   381,   385,   387,   392,   400,   404,   410,   421,
     425,   429,   433,   434,   435,   436,   437,   438,   442,   444,
     449,   451,   456,   458,   463,   465,   469,   471,   483,   485,
     492,   495,   498,   501,   503,   505,   507,   509,   511,   513,
     517,   519,   524,   534,   536,   543,   547,   555,   563,   567,
     573,   575,   579,   583,   585,   587,   595,   606,   608,   613,
     622,   623,   629,   634,   640,   649,   650,   651,   655,   660,
     675,   674,   677,   680,   679,   686,   685,   688,   691,   690,
     698,   700,   702
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "NULL_PTR", "CHARLIT", "FLOAT",
  "TRUEKEYWORD", "FALSEKEYWORD", "COLONCOLON", "STRING", "NAME", "DOT_ID",
  "DOT_ALL", "SPECIAL_VARIABLE", "ASSIGN", "_AND_", "OR", "XOR", "THEN",
  "ELSE", "'='", "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "IN", "DOTDOT",
  "'@'", "'+'", "'-'", "'&'", "UNARY", "'*'", "'/'", "MOD", "REM",
  "STARSTAR", "ABS", "NOT", "VAR", "ARROW", "'|'", "TICK_ACCESS",
  "TICK_ADDRESS", "TICK_FIRST", "TICK_LAST", "TICK_LENGTH", "TICK_MAX",
  "TICK_MIN", "TICK_MODULUS", "TICK_POS", "TICK_RANGE", "TICK_SIZE",
  "TICK_TAG", "TICK_VAL", "'.'", "'('", "'['", "NEW", "OTHERS", "';'",
  "')'", "'\\''", "','", "'{'", "'}'", "']'", "$accept", "start", "exp1",
  "primary", "$@1", "save_qualifier", "simple_exp", "arglist", "relation",
  "exp", "and_exp", "and_then_exp", "or_exp", "or_else_exp", "xor_exp",
  "tick_arglist", "type_prefix", "opt_type_prefix", "var_or_type", "block",
  "aggregate", "aggregate_component_list", "positional_list",
  "component_groups", "others", "component_group",
  "component_associations", "$@2", "$@3", "$@4", "$@5", YY_NULLPTRPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,    61,   276,    60,    62,   277,   278,   279,   280,    64,
      43,    45,    38,   281,    42,    47,   282,   283,   284,   285,
     286,   287,   288,   124,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,    46,    40,    91,
     302,   303,    59,    41,    39,    44,   123,   125,    93
};
# endif

#define YYPACT_NINF -104

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-104)))

#define YYTABLE_NINF -83

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     430,  -104,  -104,  -104,  -104,  -104,  -104,  -104,     3,  -104,
     430,   430,   544,   544,   430,   430,   289,    44,    49,    26,
     -53,    -2,   679,    86,  -104,    50,    55,    48,    57,    74,
     -28,    -1,   275,    61,  -104,  -104,  -104,   577,   219,   219,
       9,     9,   219,   219,    54,    58,   -31,   616,    42,    45,
     289,  -104,  -104,    46,  -104,  -104,    43,  -104,   430,  -104,
    -104,   430,  -104,  -104,    59,    59,    59,  -104,  -104,   277,
     430,   430,   430,   430,   430,   430,   430,   430,   430,   430,
     430,   430,   430,   430,   430,   430,   430,    89,   353,   392,
     430,   430,    94,   430,   100,   430,  -104,    63,    64,    65,
      67,   277,  -104,    15,  -104,  -104,   430,  -104,   430,   430,
     468,  -104,  -104,    62,  -104,   289,   544,  -104,  -104,   125,
    -104,  -104,  -104,    -4,   639,   -57,  -104,    70,   117,   117,
     117,   117,   117,   117,   149,   563,   174,   549,   219,   219,
     219,    91,    91,    91,    91,    91,   430,   430,  -104,   430,
    -104,  -104,  -104,   430,  -104,   430,  -104,   430,   430,   430,
     430,   659,   -38,  -104,  -104,  -104,   430,   468,  -104,   339,
    -104,   692,  -104,  -104,  -104,     9,    69,   430,   430,  -104,
     506,  -104,    59,   430,   561,   708,   211,  -104,  -104,  -104,
    -104,    81,    99,   102,    93,   430,  -104,    72,  -104,  -104,
    -104,  -104,  -104,  -104,   105,    82,  -104,  -104,   117,    59,
     430,  -104,   430,   430,  -104,   214,   430,   430,   468,  -104,
     430,  -104,   117,   106,   107,  -104,   109,  -104,  -104,  -104,
    -104,  -104,  -104
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      84,    85,    88,    86,    87,    90,    91,    89,    93,    17,
      84,    84,    84,    84,    84,    84,    84,     0,     0,     0,
       2,    19,    39,    52,     3,    53,    54,    55,    56,    57,
      83,     0,    16,     0,    18,    97,    95,    19,    21,    20,
     121,   120,    23,    22,    93,     0,     0,    39,     3,     0,
      84,   100,   105,   106,   109,    92,     0,     1,    84,     7,
       6,    84,    68,    69,    80,    80,    80,    73,    74,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,    84,     0,    84,    84,
      84,    84,     0,    84,     0,    84,    79,     0,     0,     0,
       0,    84,    12,    94,   110,   115,    84,    15,    84,    84,
      84,   103,    99,   101,   102,    84,    84,     4,     5,     0,
      70,    71,    72,    93,    39,     0,    25,     0,    40,    41,
      50,    51,    42,    49,    19,     0,    16,    35,    36,    38,
      37,    31,    32,    34,    33,    30,    84,    84,    58,    84,
      62,    66,    59,    84,    63,    84,    67,    84,    84,    84,
      84,    39,     0,    10,    98,    96,    84,    84,   108,     0,
     112,     0,   117,   104,   107,    29,     0,    84,    84,     8,
      84,   122,    80,    84,    19,     0,    16,    60,    64,    61,
      65,     0,     0,     0,     0,    84,     9,     0,   111,   116,
     113,   118,    81,    26,     0,    93,    27,    44,    43,    80,
      84,    78,    84,    84,    77,     0,    84,    84,    84,    13,
      84,    47,    46,     0,     0,    14,     0,   114,   119,    28,
      76,    75,    11
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -104,  -104,   157,    17,  -104,  -104,     4,    75,   -54,     0,
    -104,  -104,  -104,  -104,  -104,   -64,  -104,  -104,   -15,  -104,
    -104,  -104,  -104,   -46,  -104,  -104,  -103,  -104,  -104,  -104,
    -104
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    19,    20,    37,   197,   163,    22,   125,    23,   126,
      25,    26,    27,    28,    29,   120,    30,    31,    32,    33,
      34,    49,    50,    51,    52,    53,    54,   166,   217,   167,
     218
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      24,   121,   122,    56,   114,    35,   179,   172,   180,    58,
      59,    60,    35,    61,    38,    39,    48,    21,    42,    43,
      47,    59,    60,    96,   164,   196,    57,   180,    97,    40,
      41,    58,   107,    21,   148,   150,   151,   152,   177,   154,
      36,   156,    62,    63,    64,    65,    66,    36,    98,    99,
     113,   100,    67,    68,    47,    55,    69,    70,   117,   165,
       8,   118,   136,    35,   199,    93,    91,    69,    70,   174,
     127,    92,   103,   124,    94,   128,   129,   130,   131,   132,
     133,   135,   137,   138,   139,   140,   141,   142,   143,   144,
     145,    35,    95,   187,   134,   188,   104,   105,    36,   189,
     106,   190,    88,    89,    90,   161,   168,   111,   112,   170,
     116,   115,   169,   153,   171,   228,   146,   119,   207,   171,
     155,   157,   158,   159,   220,   160,    36,   173,   176,    86,
     216,   186,   202,   175,    78,    79,    80,    81,   181,    82,
      83,    84,    85,    86,   211,   221,    78,    79,    80,    81,
     185,    82,    83,    84,    85,    86,   214,   191,   192,   193,
     194,    59,    60,   184,   212,     0,   198,   213,   219,   230,
     231,   171,   232,    46,   -45,     0,   162,   203,     0,     0,
     206,     0,   204,     0,     0,     0,     0,   208,     0,     0,
     -45,   -45,   -45,    62,    63,    64,    65,    66,     0,   215,
       0,     0,   182,    67,    68,     0,     0,    69,    70,     0,
       0,   -48,   223,   224,   222,     0,   226,   227,     0,     0,
     229,     0,   171,   -82,   -82,   -82,   -82,   -48,   -48,   -48,
     -82,     0,   101,     0,     0,     0,   -45,   -45,   102,   -45,
       0,     0,   -45,    78,    79,    80,    81,     0,    82,    83,
      84,    85,    86,    82,    83,    84,    85,    86,     0,     0,
     -82,   -82,   -82,   -82,     0,     0,     0,   -82,     0,   101,
       0,     0,     0,   -48,   -48,   102,   -48,   225,     0,   -48,
       1,     2,     3,     4,     5,     6,     0,     7,   123,     0,
       0,     9,     1,     2,     3,     4,     5,     6,     0,     7,
      44,     0,     0,     9,     0,     0,     0,    10,    11,    12,
       0,    13,     0,     0,     0,     0,    14,    15,     0,    10,
      11,    12,     0,    13,   -82,   -82,   -82,   -82,    14,    15,
       0,   -82,     0,   101,     0,    16,     0,    17,     0,   102,
     -24,     0,   -24,    18,     0,     0,     0,    16,     0,    17,
      45,     0,     0,     0,     0,    18,     1,     2,     3,     4,
       5,     6,     0,     7,     8,     0,     0,     9,    78,    79,
      80,    81,   147,    82,    83,    84,    85,    86,     0,     0,
       0,   200,   201,    10,    11,    12,     0,    13,     0,     0,
       0,     0,    14,    15,     0,     1,     2,     3,     4,     5,
       6,     0,     7,     8,     0,     0,     9,     0,     0,     0,
       0,    16,   149,    17,     0,     0,     0,     0,     0,    18,
       0,     0,    10,    11,    12,     0,    13,     0,     0,     0,
       0,    14,    15,     1,     2,     3,     4,     5,     6,     0,
       7,     8,     0,     0,     9,     0,     0,     0,     0,     0,
      16,     0,    17,     0,     0,     0,     0,     0,    18,     0,
      10,    11,    12,     0,    13,     0,     0,     0,     0,    14,
      15,     1,     2,     3,     4,     5,     6,     0,     7,    44,
       0,     0,     9,     0,     0,     0,     0,     0,    16,     0,
      17,     0,     0,     0,     0,     0,    18,     0,    10,    11,
      12,     0,    13,     0,     0,     0,     0,    14,    15,     1,
       2,     3,     4,     5,     6,     0,     7,   205,     0,     0,
       9,     0,     0,     0,     0,     0,    16,     0,    17,     0,
       0,     0,     0,     0,    18,     0,    10,    11,    12,     0,
      13,     0,     0,     0,     0,    14,    15,     1,     2,     3,
       4,     5,     6,     0,     7,     8,     0,     0,     9,     0,
       0,     0,     0,     0,    16,     0,    17,     0,     0,     0,
       0,     0,    18,    59,    60,     0,    12,     0,    13,    79,
      80,    81,     0,    82,    83,    84,    85,    86,     0,    59,
      60,   183,    78,    79,    80,    81,     0,    82,    83,    84,
      85,    86,    16,     0,    17,    62,    63,    64,    65,    66,
      18,     0,     0,     0,   209,    67,    68,     0,     0,    69,
      70,    62,    63,    64,    65,    66,     0,     0,     0,     0,
       0,    67,    68,     0,     0,    69,    70,    71,    72,    73,
      74,    75,    76,    77,   108,    78,    79,    80,    81,     0,
      82,    83,    84,    85,    86,     0,    87,     0,   109,   110,
      71,    72,    73,    74,    75,    76,    77,   178,    78,    79,
      80,    81,     0,    82,    83,    84,    85,    86,     0,    87,
      71,    72,    73,    74,    75,    76,    77,   195,    78,    79,
      80,    81,     0,    82,    83,    84,    85,    86,     0,    87,
      71,    72,    73,    74,    75,    76,    77,     0,    78,    79,
      80,    81,     0,    82,    83,    84,    85,    86,     0,    87,
     108,    78,    79,    80,    81,     0,    82,    83,    84,    85,
      86,     0,     0,     0,   109,   110,   210,    78,    79,    80,
      81,     0,    82,    83,    84,    85,    86
};

static const yytype_int16 yycheck[] =
{
       0,    65,    66,    18,    50,     9,    63,   110,    65,    62,
      12,    13,     9,    15,    10,    11,    16,     0,    14,    15,
      16,    12,    13,    51,     9,    63,     0,    65,    56,    12,
      13,    62,    63,    16,    88,    89,    90,    91,    42,    93,
      44,    95,    44,    45,    46,    47,    48,    44,    49,    50,
      50,    52,    54,    55,    50,    11,    58,    59,    58,    44,
      11,    61,    77,     9,   167,    17,    16,    58,    59,   115,
      70,    16,    11,    69,    17,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,     9,    18,   147,    77,   149,    42,    43,    44,   153,
      42,   155,    16,    17,    18,   101,   106,    65,    63,   109,
      67,    65,   108,    19,   110,   218,    27,    58,   182,   115,
      20,    58,    58,    58,    42,    58,    44,    65,     3,    38,
      58,   146,    63,   116,    29,    30,    31,    32,    68,    34,
      35,    36,    37,    38,    63,   209,    29,    30,    31,    32,
     146,    34,    35,    36,    37,    38,    63,   157,   158,   159,
     160,    12,    13,   146,    65,    -1,   166,    65,    63,    63,
      63,   167,    63,    16,     0,    -1,   101,   177,    -1,    -1,
     180,    -1,   178,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      16,    17,    18,    44,    45,    46,    47,    48,    -1,   195,
      -1,    -1,    53,    54,    55,    -1,    -1,    58,    59,    -1,
      -1,     0,   212,   213,   210,    -1,   216,   217,    -1,    -1,
     220,    -1,   218,    49,    50,    51,    52,    16,    17,    18,
      56,    -1,    58,    -1,    -1,    -1,    62,    63,    64,    65,
      -1,    -1,    68,    29,    30,    31,    32,    -1,    34,    35,
      36,    37,    38,    34,    35,    36,    37,    38,    -1,    -1,
      49,    50,    51,    52,    -1,    -1,    -1,    56,    -1,    58,
      -1,    -1,    -1,    62,    63,    64,    65,    63,    -1,    68,
       3,     4,     5,     6,     7,     8,    -1,    10,    11,    -1,
      -1,    14,     3,     4,     5,     6,     7,     8,    -1,    10,
      11,    -1,    -1,    14,    -1,    -1,    -1,    30,    31,    32,
      -1,    34,    -1,    -1,    -1,    -1,    39,    40,    -1,    30,
      31,    32,    -1,    34,    49,    50,    51,    52,    39,    40,
      -1,    56,    -1,    58,    -1,    58,    -1,    60,    -1,    64,
      63,    -1,    65,    66,    -1,    -1,    -1,    58,    -1,    60,
      61,    -1,    -1,    -1,    -1,    66,     3,     4,     5,     6,
       7,     8,    -1,    10,    11,    -1,    -1,    14,    29,    30,
      31,    32,    19,    34,    35,    36,    37,    38,    -1,    -1,
      -1,    42,    43,    30,    31,    32,    -1,    34,    -1,    -1,
      -1,    -1,    39,    40,    -1,     3,     4,     5,     6,     7,
       8,    -1,    10,    11,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    58,    20,    60,    -1,    -1,    -1,    -1,    -1,    66,
      -1,    -1,    30,    31,    32,    -1,    34,    -1,    -1,    -1,
      -1,    39,    40,     3,     4,     5,     6,     7,     8,    -1,
      10,    11,    -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,
      58,    -1,    60,    -1,    -1,    -1,    -1,    -1,    66,    -1,
      30,    31,    32,    -1,    34,    -1,    -1,    -1,    -1,    39,
      40,     3,     4,     5,     6,     7,     8,    -1,    10,    11,
      -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,    58,    -1,
      60,    -1,    -1,    -1,    -1,    -1,    66,    -1,    30,    31,
      32,    -1,    34,    -1,    -1,    -1,    -1,    39,    40,     3,
       4,     5,     6,     7,     8,    -1,    10,    11,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    58,    -1,    60,    -1,
      -1,    -1,    -1,    -1,    66,    -1,    30,    31,    32,    -1,
      34,    -1,    -1,    -1,    -1,    39,    40,     3,     4,     5,
       6,     7,     8,    -1,    10,    11,    -1,    -1,    14,    -1,
      -1,    -1,    -1,    -1,    58,    -1,    60,    -1,    -1,    -1,
      -1,    -1,    66,    12,    13,    -1,    32,    -1,    34,    30,
      31,    32,    -1,    34,    35,    36,    37,    38,    -1,    12,
      13,    28,    29,    30,    31,    32,    -1,    34,    35,    36,
      37,    38,    58,    -1,    60,    44,    45,    46,    47,    48,
      66,    -1,    -1,    -1,    53,    54,    55,    -1,    -1,    58,
      59,    44,    45,    46,    47,    48,    -1,    -1,    -1,    -1,
      -1,    54,    55,    -1,    -1,    58,    59,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      34,    35,    36,    37,    38,    -1,    40,    -1,    42,    43,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    34,    35,    36,    37,    38,    -1,    40,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    34,    35,    36,    37,    38,    -1,    40,
      21,    22,    23,    24,    25,    26,    27,    -1,    29,    30,
      31,    32,    -1,    34,    35,    36,    37,    38,    -1,    40,
      28,    29,    30,    31,    32,    -1,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    42,    43,    28,    29,    30,    31,
      32,    -1,    34,    35,    36,    37,    38
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    10,    11,    14,
      30,    31,    32,    34,    39,    40,    58,    60,    66,    70,
      71,    72,    75,    77,    78,    79,    80,    81,    82,    83,
      85,    86,    87,    88,    89,     9,    44,    72,    75,    75,
      72,    72,    75,    75,    11,    61,    71,    75,    78,    90,
      91,    92,    93,    94,    95,    11,    87,     0,    62,    12,
      13,    15,    44,    45,    46,    47,    48,    54,    55,    58,
      59,    21,    22,    23,    24,    25,    26,    27,    29,    30,
      31,    32,    34,    35,    36,    37,    38,    40,    16,    17,
      18,    16,    16,    17,    17,    18,    51,    56,    49,    50,
      52,    58,    64,    11,    42,    43,    42,    63,    28,    42,
      43,    65,    63,    78,    92,    65,    67,    78,    78,    58,
      84,    84,    84,    11,    75,    76,    78,    78,    75,    75,
      75,    75,    75,    75,    72,    75,    87,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    27,    19,    77,    20,
      77,    77,    77,    19,    77,    20,    77,    58,    58,    58,
      58,    75,    76,    74,     9,    44,    96,    98,    78,    75,
      78,    75,    95,    65,    92,    72,     3,    42,    28,    63,
      65,    68,    53,    28,    72,    75,    87,    77,    77,    77,
      77,    78,    78,    78,    78,    28,    63,    73,    78,    95,
      42,    43,    63,    78,    75,    11,    78,    84,    75,    53,
      28,    63,    65,    65,    63,    75,    58,    97,    99,    63,
      42,    84,    75,    78,    78,    63,    78,    78,    95,    78,
      63,    63,    63
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    69,    70,    71,    71,    71,    72,    72,    72,    72,
      73,    72,    74,    72,    72,    72,    72,    72,    72,    75,
      75,    75,    75,    75,    76,    76,    76,    76,    76,    72,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    78,    78,    78,    78,    78,    78,    79,    79,
      80,    80,    81,    81,    82,    82,    83,    83,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      84,    84,    85,    86,    86,    72,    72,    72,    72,    72,
      72,    72,    72,    87,    87,    87,    87,    88,    88,    89,
      90,    90,    90,    91,    91,    92,    92,    92,    93,    94,
      96,    95,    95,    97,    95,    98,    95,    95,    99,    95,
      72,    72,    72
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     3,     3,     2,     2,     4,     4,
       0,     7,     0,     6,     6,     3,     1,     1,     1,     1,
       2,     2,     2,     2,     0,     1,     3,     3,     5,     4,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     1,
       3,     3,     3,     5,     5,     3,     6,     6,     4,     3,
       3,     3,     1,     1,     1,     1,     1,     1,     3,     3,
       4,     4,     3,     3,     4,     4,     3,     3,     2,     2,
       3,     3,     3,     2,     2,     7,     7,     5,     5,     2,
       0,     3,     1,     1,     0,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     2,     2,     3,     2,     3,     3,
       1,     2,     2,     2,     3,     1,     1,     3,     3,     1,
       0,     4,     3,     0,     6,     0,     4,     3,     0,     6,
       2,     2,     4
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
        case 4:
#line 195 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_COMMA); }
#line 1669 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 5:
#line 197 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
#line 1675 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 6:
#line 202 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_IND); }
#line 1681 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 7:
#line 206 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_op_with_string (pstate, STRUCTOP_STRUCT,
						    (yyvsp[0].sval)); }
#line 1688 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 8:
#line 211 "ada-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_FUNCALL);
			  write_exp_elt_longcst (pstate, (yyvsp[-1].lval));
			  write_exp_elt_opcode (pstate, OP_FUNCALL);
		        }
#line 1698 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 9:
#line 217 "ada-exp.y" /* yacc.c:1646  */
    {
			  if ((yyvsp[-3].tval) != NULL)
			    {
			      if ((yyvsp[-1].lval) != 1)
				error (_("Invalid conversion"));
			      write_exp_elt_opcode (pstate, UNOP_CAST);
			      write_exp_elt_type (pstate, (yyvsp[-3].tval));
			      write_exp_elt_opcode (pstate, UNOP_CAST);
			    }
			  else
			    {
			      write_exp_elt_opcode (pstate, OP_FUNCALL);
			      write_exp_elt_longcst (pstate, (yyvsp[-1].lval));
			      write_exp_elt_opcode (pstate, OP_FUNCALL);
			    }
			}
#line 1719 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 10:
#line 235 "ada-exp.y" /* yacc.c:1646  */
    { type_qualifier = (yyvsp[-2].tval); }
#line 1725 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 11:
#line 237 "ada-exp.y" /* yacc.c:1646  */
    {
			  if ((yyvsp[-6].tval) == NULL)
			    error (_("Type required for qualification"));
			  write_exp_elt_opcode (pstate, UNOP_QUAL);
			  write_exp_elt_type (pstate, (yyvsp[-6].tval));
			  write_exp_elt_opcode (pstate, UNOP_QUAL);
			  type_qualifier = (yyvsp[-4].tval);
			}
#line 1738 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 12:
#line 247 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = type_qualifier; }
#line 1744 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 13:
#line 252 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_SLICE); }
#line 1750 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 14:
#line 254 "ada-exp.y" /* yacc.c:1646  */
    { if ((yyvsp[-5].tval) == NULL) 
                            write_exp_elt_opcode (pstate, TERNOP_SLICE);
			  else
			    error (_("Cannot slice a type"));
			}
#line 1760 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 15:
#line 261 "ada-exp.y" /* yacc.c:1646  */
    { }
#line 1766 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 16:
#line 273 "ada-exp.y" /* yacc.c:1646  */
    { if ((yyvsp[0].tval) != NULL)
			    {
			      write_exp_elt_opcode (pstate, OP_TYPE);
			      write_exp_elt_type (pstate, (yyvsp[0].tval));
			      write_exp_elt_opcode (pstate, OP_TYPE);
			    }
			}
#line 1778 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 17:
#line 283 "ada-exp.y" /* yacc.c:1646  */
    { write_dollar_variable (pstate, (yyvsp[0].sval)); }
#line 1784 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 20:
#line 293 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_NEG); }
#line 1790 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 21:
#line 297 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PLUS); }
#line 1796 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 22:
#line 301 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
#line 1802 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 23:
#line 305 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ABS); }
#line 1808 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 24:
#line 308 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 0; }
#line 1814 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 25:
#line 312 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 1820 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 26:
#line 314 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 1826 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 27:
#line 316 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-2].lval) + 1; }
#line 1832 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 28:
#line 318 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-4].lval) + 1; }
#line 1838 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 29:
#line 323 "ada-exp.y" /* yacc.c:1646  */
    { 
			  if ((yyvsp[-2].tval) == NULL)
			    error (_("Type required within braces in coercion"));
			  write_exp_elt_opcode (pstate, UNOP_MEMVAL);
			  write_exp_elt_type (pstate, (yyvsp[-2].tval));
			  write_exp_elt_opcode (pstate, UNOP_MEMVAL);
			}
#line 1850 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 30:
#line 335 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EXP); }
#line 1856 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 31:
#line 339 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_MUL); }
#line 1862 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 32:
#line 343 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_DIV); }
#line 1868 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 33:
#line 347 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REM); }
#line 1874 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 34:
#line 351 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_MOD); }
#line 1880 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 35:
#line 355 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REPEAT); }
#line 1886 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 36:
#line 359 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ADD); }
#line 1892 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 37:
#line 363 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_CONCAT); }
#line 1898 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 38:
#line 367 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUB); }
#line 1904 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 40:
#line 374 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL); }
#line 1910 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 41:
#line 378 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
#line 1916 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 42:
#line 382 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LEQ); }
#line 1922 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 43:
#line 386 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_IN_RANGE); }
#line 1928 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 44:
#line 388 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_IN_BOUNDS);
			  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].lval));
			  write_exp_elt_opcode (pstate, BINOP_IN_BOUNDS);
			}
#line 1937 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 45:
#line 393 "ada-exp.y" /* yacc.c:1646  */
    { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  write_exp_elt_opcode (pstate, UNOP_IN_RANGE);
		          write_exp_elt_type (pstate, (yyvsp[0].tval));
		          write_exp_elt_opcode (pstate, UNOP_IN_RANGE);
			}
#line 1949 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 46:
#line 401 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_IN_RANGE);
		          write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT);
			}
#line 1957 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 47:
#line 405 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_IN_BOUNDS);
			  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].lval));
			  write_exp_elt_opcode (pstate, BINOP_IN_BOUNDS);
		          write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT);
			}
#line 1967 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 48:
#line 411 "ada-exp.y" /* yacc.c:1646  */
    { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  write_exp_elt_opcode (pstate, UNOP_IN_RANGE);
		          write_exp_elt_type (pstate, (yyvsp[0].tval));
		          write_exp_elt_opcode (pstate, UNOP_IN_RANGE);
		          write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT);
			}
#line 1980 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 49:
#line 422 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GEQ); }
#line 1986 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 50:
#line 426 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LESS); }
#line 1992 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 51:
#line 430 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GTR); }
#line 1998 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 58:
#line 443 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
#line 2004 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 59:
#line 445 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
#line 2010 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 60:
#line 450 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
#line 2016 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 61:
#line 452 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
#line 2022 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 62:
#line 457 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
#line 2028 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 63:
#line 459 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
#line 2034 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 64:
#line 464 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
#line 2040 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 65:
#line 466 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
#line 2046 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 66:
#line 470 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
#line 2052 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 67:
#line 472 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
#line 2058 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 68:
#line 484 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ADDR); }
#line 2064 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 69:
#line 486 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ADDR);
			  write_exp_elt_opcode (pstate, UNOP_CAST);
			  write_exp_elt_type (pstate,
					      type_system_address (pstate));
			  write_exp_elt_opcode (pstate, UNOP_CAST);
			}
#line 2075 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 70:
#line 493 "ada-exp.y" /* yacc.c:1646  */
    { write_int (pstate, (yyvsp[0].lval), type_int (pstate));
			  write_exp_elt_opcode (pstate, OP_ATR_FIRST); }
#line 2082 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 71:
#line 496 "ada-exp.y" /* yacc.c:1646  */
    { write_int (pstate, (yyvsp[0].lval), type_int (pstate));
			  write_exp_elt_opcode (pstate, OP_ATR_LAST); }
#line 2089 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 72:
#line 499 "ada-exp.y" /* yacc.c:1646  */
    { write_int (pstate, (yyvsp[0].lval), type_int (pstate));
			  write_exp_elt_opcode (pstate, OP_ATR_LENGTH); }
#line 2096 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 73:
#line 502 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ATR_SIZE); }
#line 2102 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 74:
#line 504 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ATR_TAG); }
#line 2108 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 75:
#line 506 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ATR_MIN); }
#line 2114 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 76:
#line 508 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ATR_MAX); }
#line 2120 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 77:
#line 510 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ATR_POS); }
#line 2126 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 78:
#line 512 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ATR_VAL); }
#line 2132 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 79:
#line 514 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ATR_MODULUS); }
#line 2138 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 80:
#line 518 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 2144 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 81:
#line 520 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-1].typed_val).val; }
#line 2150 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 82:
#line 525 "ada-exp.y" /* yacc.c:1646  */
    { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Prefix must be type"));
			  write_exp_elt_opcode (pstate, OP_TYPE);
			  write_exp_elt_type (pstate, (yyvsp[0].tval));
			  write_exp_elt_opcode (pstate, OP_TYPE); }
#line 2161 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 84:
#line 536 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPE);
			  write_exp_elt_type (pstate,
					  parse_type (pstate)->builtin_void);
			  write_exp_elt_opcode (pstate, OP_TYPE); }
#line 2170 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 85:
#line 544 "ada-exp.y" /* yacc.c:1646  */
    { write_int (pstate, (LONGEST) (yyvsp[0].typed_val).val, (yyvsp[0].typed_val).type); }
#line 2176 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 86:
#line 548 "ada-exp.y" /* yacc.c:1646  */
    { write_int (pstate,
			       convert_char_literal (type_qualifier, (yyvsp[0].typed_val).val),
			       (type_qualifier == NULL) 
			       ? (yyvsp[0].typed_val).type : type_qualifier);
		  }
#line 2186 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 87:
#line 556 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DOUBLE);
			  write_exp_elt_type (pstate, (yyvsp[0].typed_val_float).type);
			  write_exp_elt_dblcst (pstate, (yyvsp[0].typed_val_float).dval);
			  write_exp_elt_opcode (pstate, OP_DOUBLE);
			}
#line 2196 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 88:
#line 564 "ada-exp.y" /* yacc.c:1646  */
    { write_int (pstate, 0, type_int (pstate)); }
#line 2202 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 89:
#line 568 "ada-exp.y" /* yacc.c:1646  */
    { 
			  write_exp_op_with_string (pstate, OP_STRING, (yyvsp[0].sval));
			}
#line 2210 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 90:
#line 574 "ada-exp.y" /* yacc.c:1646  */
    { write_int (pstate, 1, type_boolean (pstate)); }
#line 2216 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 91:
#line 576 "ada-exp.y" /* yacc.c:1646  */
    { write_int (pstate, 0, type_boolean (pstate)); }
#line 2222 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 92:
#line 580 "ada-exp.y" /* yacc.c:1646  */
    { error (_("NEW not implemented.")); }
#line 2228 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 93:
#line 584 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[0].sval)); }
#line 2234 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 94:
#line 586 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = write_var_or_type (pstate, (yyvsp[-1].bval), (yyvsp[0].sval)); }
#line 2240 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 95:
#line 588 "ada-exp.y" /* yacc.c:1646  */
    { 
			  (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    write_exp_elt_opcode (pstate, UNOP_ADDR);
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2252 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 96:
#line 596 "ada-exp.y" /* yacc.c:1646  */
    { 
			  (yyval.tval) = write_var_or_type (pstate, (yyvsp[-2].bval), (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    write_exp_elt_opcode (pstate, UNOP_ADDR);
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2264 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 97:
#line 607 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.bval) = block_lookup (NULL, (yyvsp[-1].sval).ptr); }
#line 2270 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 98:
#line 609 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.bval) = block_lookup ((yyvsp[-2].bval), (yyvsp[-1].sval).ptr); }
#line 2276 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 99:
#line 614 "ada-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_AGGREGATE);
			  write_exp_elt_longcst (pstate, (yyvsp[-1].lval));
			  write_exp_elt_opcode (pstate, OP_AGGREGATE);
		        }
#line 2286 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 100:
#line 622 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[0].lval); }
#line 2292 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 101:
#line 624 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_POSITIONAL);
			  write_exp_elt_longcst (pstate, (yyvsp[-1].lval));
			  write_exp_elt_opcode (pstate, OP_POSITIONAL);
			  (yyval.lval) = (yyvsp[-1].lval) + 1;
			}
#line 2302 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 102:
#line 630 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-1].lval) + (yyvsp[0].lval); }
#line 2308 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 103:
#line 635 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_POSITIONAL);
			  write_exp_elt_longcst (pstate, 0);
			  write_exp_elt_opcode (pstate, OP_POSITIONAL);
			  (yyval.lval) = 1;
			}
#line 2318 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 104:
#line 641 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_POSITIONAL);
			  write_exp_elt_longcst (pstate, (yyvsp[-2].lval));
			  write_exp_elt_opcode (pstate, OP_POSITIONAL);
			  (yyval.lval) = (yyvsp[-2].lval) + 1; 
			}
#line 2328 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 105:
#line 649 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 2334 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 106:
#line 650 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 2340 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 107:
#line 652 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[0].lval) + 1; }
#line 2346 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 108:
#line 656 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OTHERS); }
#line 2352 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 109:
#line 661 "ada-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_CHOICES);
			  write_exp_elt_longcst (pstate, (yyvsp[0].lval));
			  write_exp_elt_opcode (pstate, OP_CHOICES);
		        }
#line 2362 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 110:
#line 675 "ada-exp.y" /* yacc.c:1646  */
    { write_name_assoc (pstate, (yyvsp[-1].sval)); }
#line 2368 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 111:
#line 676 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 2374 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 112:
#line 678 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 2380 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 113:
#line 680 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DISCRETE_RANGE);
			  write_exp_op_with_string (pstate, OP_NAME,
						    empty_stoken);
			}
#line 2389 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 114:
#line 684 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = 1; }
#line 2395 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 115:
#line 686 "ada-exp.y" /* yacc.c:1646  */
    { write_name_assoc (pstate, (yyvsp[-1].sval)); }
#line 2401 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 116:
#line 687 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[0].lval) + 1; }
#line 2407 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 117:
#line 689 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[0].lval) + 1; }
#line 2413 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 118:
#line 691 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DISCRETE_RANGE); }
#line 2419 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 119:
#line 692 "ada-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[0].lval) + 1; }
#line 2425 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 120:
#line 699 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_IND); }
#line 2431 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 121:
#line 701 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ADDR); }
#line 2437 "ada-exp.c" /* yacc.c:1646  */
    break;

  case 122:
#line 703 "ada-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT); }
#line 2443 "ada-exp.c" /* yacc.c:1646  */
    break;


#line 2447 "ada-exp.c" /* yacc.c:1646  */
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
#line 706 "ada-exp.y" /* yacc.c:1906  */


/* yylex defined in ada-lex.c: Reads one token, getting characters */
/* through lexptr.  */

/* Remap normal flex interface names (yylex) as well as gratuitiously */
/* global symbol names, so we can have multiple flex-generated parsers */
/* in gdb.  */

/* (See note above on previous definitions for YACC.) */

#define yy_create_buffer ada_yy_create_buffer
#define yy_delete_buffer ada_yy_delete_buffer
#define yy_init_buffer ada_yy_init_buffer
#define yy_load_buffer_state ada_yy_load_buffer_state
#define yy_switch_to_buffer ada_yy_switch_to_buffer
#define yyrestart ada_yyrestart
#define yytext ada_yytext
#define yywrap ada_yywrap

static struct obstack temp_parse_space;

/* The following kludge was found necessary to prevent conflicts between */
/* defs.h and non-standard stdlib.h files.  */
#define qsort __qsort__dummy
#include "ada-lex.c"

int
ada_parse (struct parser_state *par_state)
{
  int result;
  struct cleanup *c = make_cleanup_clear_parser_state (&pstate);

  /* Setting up the parser state.  */
  gdb_assert (par_state != NULL);
  pstate = par_state;

  lexer_init (yyin);		/* (Re-)initialize lexer.  */
  type_qualifier = NULL;
  obstack_free (&temp_parse_space, NULL);
  obstack_init (&temp_parse_space);

  result = yyparse ();
  do_cleanups (c);
  return result;
}

void
yyerror (const char *msg)
{
  error (_("Error in expression, near `%s'."), lexptr);
}

/* Emit expression to access an instance of SYM, in block BLOCK (if
 * non-NULL), and with :: qualification ORIG_LEFT_CONTEXT.  */
static void
write_var_from_sym (struct parser_state *par_state,
		    const struct block *orig_left_context,
		    const struct block *block,
		    struct symbol *sym)
{
  if (orig_left_context == NULL && symbol_read_needs_frame (sym))
    {
      if (innermost_block == 0
	  || contained_in (block, innermost_block))
	innermost_block = block;
    }

  write_exp_elt_opcode (par_state, OP_VAR_VALUE);
  write_exp_elt_block (par_state, block);
  write_exp_elt_sym (par_state, sym);
  write_exp_elt_opcode (par_state, OP_VAR_VALUE);
}

/* Write integer or boolean constant ARG of type TYPE.  */

static void
write_int (struct parser_state *par_state, LONGEST arg, struct type *type)
{
  write_exp_elt_opcode (par_state, OP_LONG);
  write_exp_elt_type (par_state, type);
  write_exp_elt_longcst (par_state, arg);
  write_exp_elt_opcode (par_state, OP_LONG);
}

/* Write an OPCODE, string, OPCODE sequence to the current expression.  */
static void
write_exp_op_with_string (struct parser_state *par_state,
			  enum exp_opcode opcode, struct stoken token)
{
  write_exp_elt_opcode (par_state, opcode);
  write_exp_string (par_state, token);
  write_exp_elt_opcode (par_state, opcode);
}
  
/* Emit expression corresponding to the renamed object named 
 * designated by RENAMED_ENTITY[0 .. RENAMED_ENTITY_LEN-1] in the
 * context of ORIG_LEFT_CONTEXT, to which is applied the operations
 * encoded by RENAMING_EXPR.  MAX_DEPTH is the maximum number of
 * cascaded renamings to allow.  If ORIG_LEFT_CONTEXT is null, it
 * defaults to the currently selected block. ORIG_SYMBOL is the 
 * symbol that originally encoded the renaming.  It is needed only
 * because its prefix also qualifies any index variables used to index
 * or slice an array.  It should not be necessary once we go to the
 * new encoding entirely (FIXME pnh 7/20/2007).  */

static void
write_object_renaming (struct parser_state *par_state,
		       const struct block *orig_left_context,
		       const char *renamed_entity, int renamed_entity_len,
		       const char *renaming_expr, int max_depth)
{
  char *name;
  enum { SIMPLE_INDEX, LOWER_BOUND, UPPER_BOUND } slice_state;
  struct block_symbol sym_info;

  if (max_depth <= 0)
    error (_("Could not find renamed symbol"));

  if (orig_left_context == NULL)
    orig_left_context = get_selected_block (NULL);

  name = (char *) obstack_copy0 (&temp_parse_space, renamed_entity,
				 renamed_entity_len);
  ada_lookup_encoded_symbol (name, orig_left_context, VAR_DOMAIN, &sym_info);
  if (sym_info.symbol == NULL)
    error (_("Could not find renamed variable: %s"), ada_decode (name));
  else if (SYMBOL_CLASS (sym_info.symbol) == LOC_TYPEDEF)
    /* We have a renaming of an old-style renaming symbol.  Don't
       trust the block information.  */
    sym_info.block = orig_left_context;

  {
    const char *inner_renamed_entity;
    int inner_renamed_entity_len;
    const char *inner_renaming_expr;

    switch (ada_parse_renaming (sym_info.symbol, &inner_renamed_entity,
				&inner_renamed_entity_len,
				&inner_renaming_expr))
      {
      case ADA_NOT_RENAMING:
	write_var_from_sym (par_state, orig_left_context, sym_info.block,
			    sym_info.symbol);
	break;
      case ADA_OBJECT_RENAMING:
	write_object_renaming (par_state, sym_info.block,
			       inner_renamed_entity, inner_renamed_entity_len,
			       inner_renaming_expr, max_depth - 1);
	break;
      default:
	goto BadEncoding;
      }
  }

  slice_state = SIMPLE_INDEX;
  while (*renaming_expr == 'X')
    {
      renaming_expr += 1;

      switch (*renaming_expr) {
      case 'A':
        renaming_expr += 1;
        write_exp_elt_opcode (par_state, UNOP_IND);
        break;
      case 'L':
	slice_state = LOWER_BOUND;
	/* FALLTHROUGH */
      case 'S':
	renaming_expr += 1;
	if (isdigit (*renaming_expr))
	  {
	    char *next;
	    long val = strtol (renaming_expr, &next, 10);
	    if (next == renaming_expr)
	      goto BadEncoding;
	    renaming_expr = next;
	    write_exp_elt_opcode (par_state, OP_LONG);
	    write_exp_elt_type (par_state, type_int (par_state));
	    write_exp_elt_longcst (par_state, (LONGEST) val);
	    write_exp_elt_opcode (par_state, OP_LONG);
	  }
	else
	  {
	    const char *end;
	    char *index_name;
	    struct block_symbol index_sym_info;

	    end = strchr (renaming_expr, 'X');
	    if (end == NULL)
	      end = renaming_expr + strlen (renaming_expr);

	    index_name
	      = (char *) obstack_copy0 (&temp_parse_space, renaming_expr,
					end - renaming_expr);
	    renaming_expr = end;

	    ada_lookup_encoded_symbol (index_name, NULL, VAR_DOMAIN,
				       &index_sym_info);
	    if (index_sym_info.symbol == NULL)
	      error (_("Could not find %s"), index_name);
	    else if (SYMBOL_CLASS (index_sym_info.symbol) == LOC_TYPEDEF)
	      /* Index is an old-style renaming symbol.  */
	      index_sym_info.block = orig_left_context;
	    write_var_from_sym (par_state, NULL, index_sym_info.block,
				index_sym_info.symbol);
	  }
	if (slice_state == SIMPLE_INDEX)
	  {
	    write_exp_elt_opcode (par_state, OP_FUNCALL);
	    write_exp_elt_longcst (par_state, (LONGEST) 1);
	    write_exp_elt_opcode (par_state, OP_FUNCALL);
	  }
	else if (slice_state == LOWER_BOUND)
	  slice_state = UPPER_BOUND;
	else if (slice_state == UPPER_BOUND)
	  {
	    write_exp_elt_opcode (par_state, TERNOP_SLICE);
	    slice_state = SIMPLE_INDEX;
	  }
	break;

      case 'R':
	{
	  struct stoken field_name;
	  const char *end;
	  char *buf;

	  renaming_expr += 1;

	  if (slice_state != SIMPLE_INDEX)
	    goto BadEncoding;
	  end = strchr (renaming_expr, 'X');
	  if (end == NULL)
	    end = renaming_expr + strlen (renaming_expr);
	  field_name.length = end - renaming_expr;
	  buf = (char *) xmalloc (end - renaming_expr + 1);
	  field_name.ptr = buf;
	  strncpy (buf, renaming_expr, end - renaming_expr);
	  buf[end - renaming_expr] = '\000';
	  renaming_expr = end;
	  write_exp_op_with_string (par_state, STRUCTOP_STRUCT, field_name);
	  break;
	}

      default:
	goto BadEncoding;
      }
    }
  if (slice_state == SIMPLE_INDEX)
    return;

 BadEncoding:
  error (_("Internal error in encoding of renaming declaration"));
}

static const struct block*
block_lookup (const struct block *context, const char *raw_name)
{
  const char *name;
  struct block_symbol *syms;
  int nsyms;
  struct symtab *symtab;

  if (raw_name[0] == '\'')
    {
      raw_name += 1;
      name = raw_name;
    }
  else
    name = ada_encode (raw_name);

  nsyms = ada_lookup_symbol_list (name, context, VAR_DOMAIN, &syms);
  if (context == NULL
      && (nsyms == 0 || SYMBOL_CLASS (syms[0].symbol) != LOC_BLOCK))
    symtab = lookup_symtab (name);
  else
    symtab = NULL;

  if (symtab != NULL)
    return BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (symtab), STATIC_BLOCK);
  else if (nsyms == 0 || SYMBOL_CLASS (syms[0].symbol) != LOC_BLOCK)
    {
      if (context == NULL)
	error (_("No file or function \"%s\"."), raw_name);
      else
	error (_("No function \"%s\" in specified context."), raw_name);
    }
  else
    {
      if (nsyms > 1)
	warning (_("Function name \"%s\" ambiguous here"), raw_name);
      return SYMBOL_BLOCK_VALUE (syms[0].symbol);
    }
}

static struct symbol*
select_possible_type_sym (struct block_symbol *syms, int nsyms)
{
  int i;
  int preferred_index;
  struct type *preferred_type;
	  
  preferred_index = -1; preferred_type = NULL;
  for (i = 0; i < nsyms; i += 1)
    switch (SYMBOL_CLASS (syms[i].symbol))
      {
      case LOC_TYPEDEF:
	if (ada_prefer_type (SYMBOL_TYPE (syms[i].symbol), preferred_type))
	  {
	    preferred_index = i;
	    preferred_type = SYMBOL_TYPE (syms[i].symbol);
	  }
	break;
      case LOC_REGISTER:
      case LOC_ARG:
      case LOC_REF_ARG:
      case LOC_REGPARM_ADDR:
      case LOC_LOCAL:
      case LOC_COMPUTED:
	return NULL;
      default:
	break;
      }
  if (preferred_type == NULL)
    return NULL;
  return syms[preferred_index].symbol;
}

static struct type*
find_primitive_type (struct parser_state *par_state, char *name)
{
  struct type *type;
  type = language_lookup_primitive_type (parse_language (par_state),
					 parse_gdbarch (par_state),
					 name);
  if (type == NULL && strcmp ("system__address", name) == 0)
    type = type_system_address (par_state);

  if (type != NULL)
    {
      /* Check to see if we have a regular definition of this
	 type that just didn't happen to have been read yet.  */
      struct symbol *sym;
      char *expanded_name = 
	(char *) alloca (strlen (name) + sizeof ("standard__"));
      strcpy (expanded_name, "standard__");
      strcat (expanded_name, name);
      sym = ada_lookup_symbol (expanded_name, NULL, VAR_DOMAIN, NULL).symbol;
      if (sym != NULL && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
	type = SYMBOL_TYPE (sym);
    }

  return type;
}

static int
chop_selector (char *name, int end)
{
  int i;
  for (i = end - 1; i > 0; i -= 1)
    if (name[i] == '.' || (name[i] == '_' && name[i+1] == '_'))
      return i;
  return -1;
}

/* If NAME is a string beginning with a separator (either '__', or
   '.'), chop this separator and return the result; else, return
   NAME.  */

static char *
chop_separator (char *name)
{
  if (*name == '.')
   return name + 1;

  if (name[0] == '_' && name[1] == '_')
    return name + 2;

  return name;
}

/* Given that SELS is a string of the form (<sep><identifier>)*, where
   <sep> is '__' or '.', write the indicated sequence of
   STRUCTOP_STRUCT expression operators. */
static void
write_selectors (struct parser_state *par_state, char *sels)
{
  while (*sels != '\0')
    {
      struct stoken field_name;
      char *p = chop_separator (sels);
      sels = p;
      while (*sels != '\0' && *sels != '.' 
	     && (sels[0] != '_' || sels[1] != '_'))
	sels += 1;
      field_name.length = sels - p;
      field_name.ptr = p;
      write_exp_op_with_string (par_state, STRUCTOP_STRUCT, field_name);
    }
}

/* Write a variable access (OP_VAR_VALUE) to ambiguous encoded name
   NAME[0..LEN-1], in block context BLOCK, to be resolved later.  Writes
   a temporary symbol that is valid until the next call to ada_parse.
   */
static void
write_ambiguous_var (struct parser_state *par_state,
		     const struct block *block, char *name, int len)
{
  struct symbol *sym = XOBNEW (&temp_parse_space, struct symbol);

  memset (sym, 0, sizeof (struct symbol));
  SYMBOL_DOMAIN (sym) = UNDEF_DOMAIN;
  SYMBOL_LINKAGE_NAME (sym)
    = (const char *) obstack_copy0 (&temp_parse_space, name, len);
  SYMBOL_LANGUAGE (sym) = language_ada;

  write_exp_elt_opcode (par_state, OP_VAR_VALUE);
  write_exp_elt_block (par_state, block);
  write_exp_elt_sym (par_state, sym);
  write_exp_elt_opcode (par_state, OP_VAR_VALUE);
}

/* A convenient wrapper around ada_get_field_index that takes
   a non NUL-terminated FIELD_NAME0 and a FIELD_NAME_LEN instead
   of a NUL-terminated field name.  */

static int
ada_nget_field_index (const struct type *type, const char *field_name0,
                      int field_name_len, int maybe_missing)
{
  char *field_name = (char *) alloca ((field_name_len + 1) * sizeof (char));

  strncpy (field_name, field_name0, field_name_len);
  field_name[field_name_len] = '\0';
  return ada_get_field_index (type, field_name, maybe_missing);
}

/* If encoded_field_name is the name of a field inside symbol SYM,
   then return the type of that field.  Otherwise, return NULL.

   This function is actually recursive, so if ENCODED_FIELD_NAME
   doesn't match one of the fields of our symbol, then try to see
   if ENCODED_FIELD_NAME could not be a succession of field names
   (in other words, the user entered an expression of the form
   TYPE_NAME.FIELD1.FIELD2.FIELD3), in which case we evaluate
   each field name sequentially to obtain the desired field type.
   In case of failure, we return NULL.  */

static struct type *
get_symbol_field_type (struct symbol *sym, char *encoded_field_name)
{
  char *field_name = encoded_field_name;
  char *subfield_name;
  struct type *type = SYMBOL_TYPE (sym);
  int fieldno;

  if (type == NULL || field_name == NULL)
    return NULL;
  type = check_typedef (type);

  while (field_name[0] != '\0')
    {
      field_name = chop_separator (field_name);

      fieldno = ada_get_field_index (type, field_name, 1);
      if (fieldno >= 0)
        return TYPE_FIELD_TYPE (type, fieldno);

      subfield_name = field_name;
      while (*subfield_name != '\0' && *subfield_name != '.' 
	     && (subfield_name[0] != '_' || subfield_name[1] != '_'))
	subfield_name += 1;

      if (subfield_name[0] == '\0')
        return NULL;

      fieldno = ada_nget_field_index (type, field_name,
                                      subfield_name - field_name, 1);
      if (fieldno < 0)
        return NULL;

      type = TYPE_FIELD_TYPE (type, fieldno);
      field_name = subfield_name;
    }

  return NULL;
}

/* Look up NAME0 (an unencoded identifier or dotted name) in BLOCK (or 
   expression_block_context if NULL).  If it denotes a type, return
   that type.  Otherwise, write expression code to evaluate it as an
   object and return NULL. In this second case, NAME0 will, in general,
   have the form <name>(.<selector_name>)*, where <name> is an object
   or renaming encoded in the debugging data.  Calls error if no
   prefix <name> matches a name in the debugging data (i.e., matches
   either a complete name or, as a wild-card match, the final 
   identifier).  */

static struct type*
write_var_or_type (struct parser_state *par_state,
		   const struct block *block, struct stoken name0)
{
  int depth;
  char *encoded_name;
  int name_len;

  if (block == NULL)
    block = expression_context_block;

  encoded_name = ada_encode (name0.ptr);
  name_len = strlen (encoded_name);
  encoded_name
    = (char *) obstack_copy0 (&temp_parse_space, encoded_name, name_len);
  for (depth = 0; depth < MAX_RENAMING_CHAIN_LENGTH; depth += 1)
    {
      int tail_index;
      
      tail_index = name_len;
      while (tail_index > 0)
	{
	  int nsyms;
	  struct block_symbol *syms;
	  struct symbol *type_sym;
	  struct symbol *renaming_sym;
	  const char* renaming;
	  int renaming_len;
	  const char* renaming_expr;
	  int terminator = encoded_name[tail_index];

	  encoded_name[tail_index] = '\0';
	  nsyms = ada_lookup_symbol_list (encoded_name, block,
					  VAR_DOMAIN, &syms);
	  encoded_name[tail_index] = terminator;

	  /* A single symbol may rename a package or object. */

	  /* This should go away when we move entirely to new version.
	     FIXME pnh 7/20/2007. */
	  if (nsyms == 1)
	    {
	      struct symbol *ren_sym =
		ada_find_renaming_symbol (syms[0].symbol, syms[0].block);

	      if (ren_sym != NULL)
		syms[0].symbol = ren_sym;
	    }

	  type_sym = select_possible_type_sym (syms, nsyms);

	  if (type_sym != NULL)
	    renaming_sym = type_sym;
	  else if (nsyms == 1)
	    renaming_sym = syms[0].symbol;
	  else 
	    renaming_sym = NULL;

	  switch (ada_parse_renaming (renaming_sym, &renaming,
				      &renaming_len, &renaming_expr))
	    {
	    case ADA_NOT_RENAMING:
	      break;
	    case ADA_PACKAGE_RENAMING:
	    case ADA_EXCEPTION_RENAMING:
	    case ADA_SUBPROGRAM_RENAMING:
	      {
	        int alloc_len = renaming_len + name_len - tail_index + 1;
		char *new_name
		  = (char *) obstack_alloc (&temp_parse_space, alloc_len);
		strncpy (new_name, renaming, renaming_len);
		strcpy (new_name + renaming_len, encoded_name + tail_index);
		encoded_name = new_name;
		name_len = renaming_len + name_len - tail_index;
		goto TryAfterRenaming;
	      }	
	    case ADA_OBJECT_RENAMING:
	      write_object_renaming (par_state, block, renaming, renaming_len,
				     renaming_expr, MAX_RENAMING_CHAIN_LENGTH);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    default:
	      internal_error (__FILE__, __LINE__,
			      _("impossible value from ada_parse_renaming"));
	    }

	  if (type_sym != NULL)
	    {
              struct type *field_type;
              
              if (tail_index == name_len)
                return SYMBOL_TYPE (type_sym);

              /* We have some extraneous characters after the type name.
                 If this is an expression "TYPE_NAME.FIELD0.[...].FIELDN",
                 then try to get the type of FIELDN.  */
              field_type
                = get_symbol_field_type (type_sym, encoded_name + tail_index);
              if (field_type != NULL)
                return field_type;
	      else 
		error (_("Invalid attempt to select from type: \"%s\"."),
                       name0.ptr);
	    }
	  else if (tail_index == name_len && nsyms == 0)
	    {
	      struct type *type = find_primitive_type (par_state,
						       encoded_name);

	      if (type != NULL)
		return type;
	    }

	  if (nsyms == 1)
	    {
	      write_var_from_sym (par_state, block, syms[0].block,
				  syms[0].symbol);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    }
	  else if (nsyms == 0) 
	    {
	      struct bound_minimal_symbol msym
		= ada_lookup_simple_minsym (encoded_name);
	      if (msym.minsym != NULL)
		{
		  write_exp_msymbol (par_state, msym);
		  /* Maybe cause error here rather than later? FIXME? */
		  write_selectors (par_state, encoded_name + tail_index);
		  return NULL;
		}

	      if (tail_index == name_len
		  && strncmp (encoded_name, "standard__", 
			      sizeof ("standard__") - 1) == 0)
		error (_("No definition of \"%s\" found."), name0.ptr);

	      tail_index = chop_selector (encoded_name, tail_index);
	    } 
	  else
	    {
	      write_ambiguous_var (par_state, block, encoded_name,
				   tail_index);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    }
	}

      if (!have_full_symbols () && !have_partial_symbols () && block == NULL)
	error (_("No symbol table is loaded.  Use the \"file\" command."));
      if (block == expression_context_block)
	error (_("No definition of \"%s\" in current context."), name0.ptr);
      else
	error (_("No definition of \"%s\" in specified context."), name0.ptr);
      
    TryAfterRenaming: ;
    }

  error (_("Could not find renamed symbol \"%s\""), name0.ptr);

}

/* Write a left side of a component association (e.g., NAME in NAME =>
   exp).  If NAME has the form of a selected component, write it as an
   ordinary expression.  If it is a simple variable that unambiguously
   corresponds to exactly one symbol that does not denote a type or an
   object renaming, also write it normally as an OP_VAR_VALUE.
   Otherwise, write it as an OP_NAME.

   Unfortunately, we don't know at this point whether NAME is supposed
   to denote a record component name or the value of an array index.
   Therefore, it is not appropriate to disambiguate an ambiguous name
   as we normally would, nor to replace a renaming with its referent.
   As a result, in the (one hopes) rare case that one writes an
   aggregate such as (R => 42) where R renames an object or is an
   ambiguous name, one must write instead ((R) => 42). */
   
static void
write_name_assoc (struct parser_state *par_state, struct stoken name)
{
  if (strchr (name.ptr, '.') == NULL)
    {
      struct block_symbol *syms;
      int nsyms = ada_lookup_symbol_list (name.ptr, expression_context_block,
					  VAR_DOMAIN, &syms);

      if (nsyms != 1 || SYMBOL_CLASS (syms[0].symbol) == LOC_TYPEDEF)
	write_exp_op_with_string (par_state, OP_NAME, name);
      else
	write_var_from_sym (par_state, NULL, syms[0].block, syms[0].symbol);
    }
  else
    if (write_var_or_type (par_state, NULL, name) != NULL)
      error (_("Invalid use of type."));
}

/* Convert the character literal whose ASCII value would be VAL to the
   appropriate value of type TYPE, if there is a translation.
   Otherwise return VAL.  Hence, in an enumeration type ('A', 'B'),
   the literal 'A' (VAL == 65), returns 0.  */

static LONGEST
convert_char_literal (struct type *type, LONGEST val)
{
  char name[7];
  int f;

  if (type == NULL)
    return val;
  type = check_typedef (type);
  if (TYPE_CODE (type) != TYPE_CODE_ENUM)
    return val;

  xsnprintf (name, sizeof (name), "QU%02x", (int) val);
  for (f = 0; f < TYPE_NFIELDS (type); f += 1)
    {
      if (strcmp (name, TYPE_FIELD_NAME (type, f)) == 0)
	return TYPE_FIELD_ENUMVAL (type, f);
    }
  return val;
}

static struct type *
type_int (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_int;
}

static struct type *
type_long (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long;
}

static struct type *
type_long_long (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long_long;
}

static struct type *
type_float (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_float;
}

static struct type *
type_double (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_double;
}

static struct type *
type_long_double (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long_double;
}

static struct type *
type_char (struct parser_state *par_state)
{
  return language_string_char_type (parse_language (par_state),
				    parse_gdbarch (par_state));
}

static struct type *
type_boolean (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_bool;
}

static struct type *
type_system_address (struct parser_state *par_state)
{
  struct type *type 
    = language_lookup_primitive_type (parse_language (par_state),
				      parse_gdbarch (par_state),
				      "system__address");
  return  type != NULL ? type : parse_type (par_state)->builtin_data_ptr;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_ada_exp;

void
_initialize_ada_exp (void)
{
  obstack_init (&temp_parse_space);
}
