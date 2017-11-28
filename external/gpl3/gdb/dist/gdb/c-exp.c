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
#line 36 "c-exp.y" /* yacc.c:339  */


#include "defs.h"
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
#include "macroscope.h"
#include "objc-lang.h"
#include "typeprint.h"
#include "cp-abi.h"

#define parse_type(ps) builtin_type (parse_gdbarch (ps))

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX c_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

void yyerror (const char *);

static int type_aggregate_p (struct type *);


#line 109 "c-exp.c" /* yacc.c:339  */

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
    EQUAL = 306,
    NOTEQUAL = 307,
    LEQ = 308,
    GEQ = 309,
    LSH = 310,
    RSH = 311,
    UNARY = 312,
    INCREMENT = 313,
    DECREMENT = 314,
    ARROW = 315,
    ARROW_STAR = 316,
    DOT_STAR = 317,
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
#define EQUAL 306
#define NOTEQUAL 307
#define LEQ 308
#define GEQ 309
#define LSH 310
#define RSH 311
#define UNARY 312
#define INCREMENT 313
#define DECREMENT 314
#define ARROW 315
#define ARROW_STAR 316
#define DOT_STAR 317
#define BLOCKNAME 318
#define FILENAME 319
#define DOTDOTDOT 320

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 84 "c-exp.y" /* yacc.c:355  */

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
    const struct block *bval;
    enum exp_opcode opcode;

    struct stoken_vector svec;
    VEC (type_ptr) *tvec;

    struct type_stack *type_stack;

    struct objc_class_str theclass;
  

#line 308 "c-exp.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */
#line 115 "c-exp.y" /* yacc.c:358  */

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *par_state,
			 const char *, int, int, YYSTYPE *);
static struct stoken operator_stoken (const char *);
static void check_parameter_typelist (VEC (type_ptr) *);
static void write_destructor_name (struct parser_state *par_state,
				   struct stoken);

#ifdef YYBISON
static void c_print_token (FILE *file, int type, YYSTYPE value);
#define YYPRINT(FILE, TYPE, VALUE) c_print_token (FILE, TYPE, VALUE)
#endif

#line 339 "c-exp.c" /* yacc.c:358  */

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
#define YYFINAL  167
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1692

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  90
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  46
/* YYNRULES -- Number of rules.  */
#define YYNRULES  262
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  415

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   320

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    84,     2,     2,     2,    70,    56,     2,
      79,    83,    68,    66,    48,    67,    76,    69,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    87,     2,
      59,    50,    60,    51,    65,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    78,     2,    86,    55,     2,     2,     2,     2,     2,
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
      63,    64,    71,    72,    73,    74,    75,    77,    80,    81,
      82
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   233,   233,   234,   237,   241,   245,   251,   258,   259,
     264,   268,   272,   276,   280,   284,   288,   292,   296,   300,
     304,   308,   312,   316,   322,   329,   339,   345,   352,   360,
     364,   370,   377,   387,   393,   400,   408,   412,   416,   426,
     425,   449,   448,   465,   464,   473,   475,   478,   479,   482,
     484,   486,   493,   490,   502,   501,   527,   531,   534,   538,
     542,   560,   563,   570,   574,   578,   584,   588,   592,   596,
     600,   604,   608,   612,   616,   620,   624,   628,   632,   636,
     640,   644,   648,   652,   656,   660,   664,   668,   675,   682,
     691,   704,   711,   718,   721,   727,   734,   754,   759,   763,
     767,   774,   791,   809,   842,   851,   859,   869,   877,   883,
     894,   909,   931,   944,   968,   977,   978,  1006,  1057,  1061,
    1062,  1065,  1068,  1069,  1073,  1074,  1079,  1078,  1082,  1081,
    1084,  1086,  1088,  1090,  1094,  1103,  1105,  1106,  1109,  1111,
    1118,  1125,  1131,  1138,  1140,  1142,  1144,  1148,  1150,  1162,
    1166,  1168,  1172,  1176,  1180,  1184,  1188,  1192,  1196,  1200,
    1204,  1208,  1212,  1216,  1220,  1224,  1228,  1232,  1236,  1240,
    1244,  1248,  1252,  1256,  1260,  1264,  1268,  1272,  1278,  1284,
    1287,  1292,  1298,  1301,  1306,  1312,  1315,  1320,  1326,  1329,
    1334,  1340,  1344,  1348,  1352,  1359,  1363,  1365,  1369,  1370,
    1378,  1386,  1397,  1399,  1408,  1414,  1421,  1422,  1429,  1433,
    1434,  1437,  1438,  1441,  1445,  1447,  1451,  1453,  1455,  1457,
    1459,  1461,  1463,  1465,  1467,  1469,  1471,  1473,  1475,  1477,
    1479,  1481,  1483,  1485,  1487,  1489,  1529,  1531,  1533,  1535,
    1537,  1539,  1541,  1543,  1545,  1547,  1549,  1551,  1553,  1555,
    1557,  1559,  1561,  1572,  1573,  1574,  1575,  1576,  1577,  1580,
    1581,  1589,  1601
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
  "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQUAL", "NOTEQUAL", "'<'", "'>'",
  "LEQ", "GEQ", "LSH", "RSH", "'@'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "UNARY", "INCREMENT", "DECREMENT", "ARROW", "ARROW_STAR", "'.'",
  "DOT_STAR", "'['", "'('", "BLOCKNAME", "FILENAME", "DOTDOTDOT", "')'",
  "'!'", "'~'", "']'", "':'", "'{'", "'}'", "$accept", "start", "type_exp",
  "exp1", "exp", "$@1", "$@2", "$@3", "msglist", "msgarglist", "msgarg",
  "$@4", "$@5", "lcurly", "arglist", "rcurly", "string_exp", "block",
  "variable", "qualified_name", "space_identifier", "const_or_volatile",
  "cv_with_space_id", "const_or_volatile_or_space_identifier_noopt",
  "const_or_volatile_or_space_identifier", "ptr_operator", "$@6", "$@7",
  "ptr_operator_ts", "abs_decl", "direct_abs_decl", "array_mod",
  "func_mod", "type", "typebase", "type_name", "parameter_typelist",
  "nonempty_typelist", "ptype", "conversion_type_id",
  "conversion_declarator", "const_and_volatile", "const_or_volatile_noopt",
  "oper", "name", "name_not_typename", YY_NULLPTRPTR
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,    44,   303,
      61,    63,   304,   305,   124,    94,    38,   306,   307,    60,
      62,   308,   309,   310,   311,    64,    43,    45,    42,    47,
      37,   312,   313,   314,   315,   316,    46,   317,    91,    40,
     318,   319,   320,    41,    33,   126,    93,    58,   123,   125
};
# endif

#define YYPACT_NINF -215

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-215)))

#define YYTABLE_NINF -121

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     470,  -215,  -215,  -215,  -215,  -215,   -40,  -215,  -215,    -6,
      86,   228,  -215,   802,   349,   357,   365,   511,   642,    32,
      24,   331,    59,    89,    92,    93,    50,    68,    74,    96,
     225,    79,  -215,   112,   115,  -215,  -215,  -215,  -215,   728,
     728,   728,   728,   728,   728,   470,   134,  -215,   728,   728,
    -215,   183,  -215,   139,  1342,   384,   182,   168,  -215,  -215,
     128,  -215,  1649,  -215,    14,    21,  -215,   129,  -215,   163,
     331,  -215,   110,    86,  -215,  1342,  -215,   111,    10,    25,
    -215,  -215,  -215,  -215,  -215,  -215,  -215,  -215,  -215,  -215,
    -215,  -215,  -215,  -215,  -215,  -215,  -215,  -215,  -215,  -215,
    -215,  -215,  -215,  -215,  -215,   131,   132,  -215,  -215,   249,
    -215,  -215,  -215,  -215,  -215,  -215,  -215,  -215,   202,  -215,
     204,  -215,   206,  -215,   208,    86,   470,   704,  -215,    19,
     184,  -215,  -215,  -215,  -215,  -215,   166,  1617,  1617,  1617,
    1617,   556,   728,   470,    23,  -215,  -215,   186,   187,    88,
    -215,  -215,   188,   189,  -215,  -215,  -215,   704,   704,   704,
     704,   704,   704,   157,     1,   704,   704,  -215,   728,   728,
     728,   728,   728,   728,   728,   728,   728,   728,   728,   728,
     728,   728,   728,   728,   728,   728,   728,   728,   728,   728,
     728,   728,  -215,  -215,    70,   728,   266,   728,   728,  1606,
     158,  1342,     4,  -215,   331,   236,    97,    14,  -215,     5,
      81,   185,  -215,    11,   908,   193,    27,  -215,    36,  -215,
    -215,  -215,   172,   728,   331,   227,    37,    37,    37,  -215,
     180,   181,   194,   195,  -215,  -215,   145,  -215,  -215,  -215,
    -215,  -215,   190,   229,  -215,  -215,  1649,   210,   211,   226,
     243,   950,   223,  1006,   232,  1062,   245,  -215,  -215,  -215,
     247,   270,  -215,  -215,  -215,   728,  -215,  1342,   -24,  1342,
    1342,   838,  1398,  1427,  1454,  1483,  1510,  1539,  1539,   541,
     541,   541,   541,   627,   627,   905,   970,   970,   704,   704,
     704,  -215,    86,   331,  -215,   277,   130,  -215,   331,  -215,
     299,   130,   -22,   728,  -215,   235,   272,  -215,   728,   728,
    -215,   297,  -215,  -215,  -215,   239,  -215,   193,   193,    14,
     240,  -215,  -215,   244,   246,  -215,    36,   748,  -215,  -215,
    -215,    12,  -215,   331,   728,   728,   248,    37,  -215,   250,
     252,   254,  -215,  -215,  -215,  -215,  -215,  -215,  -215,  -215,
     262,   251,   256,   257,   271,  -215,  -215,  -215,  -215,  -215,
    -215,  -215,  -215,   704,  -215,   728,   319,  -215,   321,  -215,
    -215,    18,    97,   917,   704,  1342,  -215,  -215,  -215,  -215,
    -215,  -215,    14,  -215,  -215,  1342,  1342,  -215,  -215,   250,
     728,  -215,  -215,  -215,   728,   728,   728,   728,  1371,  -215,
    -215,  -215,  -215,  -215,  -215,  -215,  1342,  1118,  1174,  1230,
    1286,  -215,  -215,  -215,  -215
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
     120,    88,    91,    92,   101,   104,     0,    89,   259,   262,
     150,     0,    90,   120,     0,     0,     0,     0,     0,   192,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   194,
     152,   153,   151,   214,   215,   177,    94,   105,   106,     0,
       0,     0,     0,     0,     0,   120,   260,   108,     0,     0,
      56,     0,     3,     2,     8,    57,   103,     0,    93,   115,
       0,   122,   120,     4,   206,   149,   213,   123,   261,   117,
       0,    54,     0,    39,    41,    43,   150,     0,   216,   217,
     235,   246,   232,   243,   242,   229,   227,   228,   238,   239,
     233,   234,   240,   241,   236,   237,   222,   223,   224,   225,
     226,   244,   245,   248,   247,     0,     0,   231,   230,   209,
     252,   253,   257,   180,   255,   256,   254,   258,   179,   183,
     182,   186,   185,   189,   188,     0,   120,    22,   198,   200,
     201,   199,   191,   262,   260,   116,     0,   120,   120,   120,
     120,   120,     0,   120,   200,   201,   193,   160,   156,   161,
     154,   178,   175,   173,   171,   211,   212,    11,    13,    12,
      10,    16,    17,     0,     0,    14,    15,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    18,    19,     0,     0,     0,     0,     0,    52,
       0,    58,     0,   102,     0,     0,   120,   196,   197,     0,
     132,   130,   128,     0,   120,   134,   136,   207,   137,   140,
     142,   110,     0,    57,     0,   112,     0,     0,     0,   251,
       0,     0,     0,     0,   250,   249,   209,   208,   181,   184,
     187,   190,     0,   167,   158,   174,   120,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   165,   157,   159,   155,
     169,   164,   162,   176,   172,     0,    65,     9,     0,    87,
      86,     0,    84,    83,    82,    81,    80,    74,    75,    78,
      79,    76,    77,    72,    73,    66,    70,    71,    67,    68,
      69,    25,   255,     0,    28,    23,    29,    32,     0,    35,
      30,    36,     0,    57,   204,     0,   202,    61,     0,     0,
      62,   111,   118,   121,   119,     0,   144,   133,   131,   125,
       0,   143,   147,     0,     0,   126,   135,   120,   139,   141,
      95,     0,   113,     0,     0,     0,     0,    46,    47,    45,
       0,     0,   220,   218,   221,   219,   126,   210,    96,   168,
       0,     0,     0,     0,     0,     5,     6,     7,    21,    20,
     166,   170,   163,    64,    38,     0,    26,    24,    33,    31,
      37,     0,   120,   120,    63,    59,   146,   124,   129,   145,
     138,   148,   125,    55,   114,    51,    50,    40,    48,     0,
       0,    42,    44,   195,     0,     0,     0,     0,    85,    27,
      34,    53,    60,   203,   205,   127,    49,     0,     0,     0,
       0,    97,    99,    98,   100
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -215,  -215,     3,    13,   -11,  -215,  -215,  -215,  -159,  -215,
       8,  -215,  -215,  -215,  -205,   147,  -215,  -215,  -215,  -100,
    -215,  -191,  -215,   -63,   -30,  -103,  -215,  -215,  -215,   141,
     137,  -214,  -213,  -124,    30,   327,   159,  -215,  -215,  -215,
     121,  -215,  -187,    -5,     6,   343
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    51,   163,   164,    54,   226,   227,   228,   336,   337,
     338,   303,   223,    55,   202,   308,    56,    57,    58,    59,
     206,    60,    61,    62,   378,   215,   382,   319,   216,   217,
     218,   219,   220,    63,    64,   132,   324,   306,    65,   110,
     237,    66,    67,    68,   339,    69
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      75,   208,   242,    52,   328,   329,   236,   127,   315,   117,
     117,   117,   117,    53,   320,   313,   117,   252,   331,   314,
     118,   120,   122,   124,   168,   230,   168,   136,   157,   158,
     159,   160,   161,   162,     8,   133,   209,   165,   166,    70,
     232,    13,   209,   109,   201,   128,   208,   111,   112,   168,
     114,   209,   309,   115,    13,    33,    34,   243,   200,   244,
     309,   256,   364,   257,   370,   117,   309,   117,   340,   341,
     129,   130,   131,    71,   210,   304,   222,   211,   225,  -120,
     111,   112,   291,   292,   266,   334,   115,    13,   231,   212,
     304,   316,   207,   307,   294,   383,   299,   321,   371,   213,
     214,   401,   152,   233,   134,   213,   214,   317,   318,   128,
      72,   260,   328,   329,   213,   327,   153,   116,   137,   154,
     111,   112,   350,   114,   335,   261,   115,    13,   262,   141,
     251,   253,   255,   236,   144,   145,   131,   211,    33,    34,
     247,   248,   249,   250,   208,   169,   254,   142,   138,   212,
     116,   139,   140,   143,   155,   293,   156,   267,  -107,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   402,   268,   167,   296,   314,   301,   168,   203,   117,
     116,   117,   204,   205,  -119,   224,   221,   229,   210,   117,
     295,   211,   300,   304,   194,   195,   196,   197,   198,   199,
     311,   302,   201,   346,   238,   235,   239,   234,   240,   117,
     241,   117,   117,   117,   245,   246,   258,   259,   263,   264,
     332,     1,     2,     3,     4,     5,     6,     7,     8,     9,
     265,    73,    74,    11,    12,    13,   312,   307,   147,   404,
      18,   333,    20,   212,   363,   330,   377,    22,    23,    24,
      25,   325,   148,   149,    28,   150,   342,   343,   151,   349,
     351,   352,    36,   348,    37,    38,   111,   112,   297,   292,
     344,   345,   115,    13,    39,   360,   353,   361,   117,   367,
      33,    34,   201,   117,    40,    41,    42,   374,   375,   366,
      43,    44,   210,   354,   368,   211,   356,    45,    46,    47,
     362,   369,    48,    49,  -120,   358,    50,   212,   372,   377,
     373,  -109,   393,   385,   386,   376,   379,   380,   117,   381,
     394,   399,   117,   400,   387,   395,   396,   390,   391,   384,
     392,   111,   112,   389,   114,   388,   116,   115,    13,   310,
     397,   298,   405,   326,   398,   323,   146,   347,   305,   111,
     112,   113,   114,   135,     0,   115,    13,   111,   112,   119,
     114,     0,     0,   115,    13,   111,   112,   121,   114,   406,
       0,   115,    13,   407,   408,   409,   410,     1,     2,     3,
       4,     5,     6,     7,     8,     9,     0,    10,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
       0,   116,     0,    22,    23,    24,    25,     0,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,   116,
      37,    38,     0,     0,     0,     0,     0,   116,     0,     0,
      39,     0,     0,     0,     0,   116,     0,     0,     0,  -120,
      40,    41,    42,     0,     0,     0,    43,    44,     0,     0,
       0,     0,     0,    45,    46,    47,     0,     0,    48,    49,
       0,     0,    50,     1,     2,     3,     4,     5,     6,     7,
       8,     9,     0,    10,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,     0,     0,     0,    22,
      23,    24,    25,     0,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,     0,    37,    38,     0,     0,
       0,   111,   112,   123,   114,     0,    39,   115,    13,     0,
       0,     0,     0,     0,     0,     0,    40,    41,    42,     0,
       0,     0,    43,    44,     0,     0,     0,     0,     0,    45,
      46,    47,     0,     0,    48,    49,   169,     0,    50,     1,
       2,     3,     4,     5,     6,     7,     8,     9,     0,    10,
       0,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,     0,     0,     0,    22,    23,    24,    25,     0,
       0,   116,    28,    29,    30,    31,    32,    33,    34,    35,
      36,     0,    37,    38,   184,   185,   186,   187,   188,   189,
     190,   191,    39,   192,   193,   194,   195,   196,   197,   198,
     199,     0,    40,    41,    42,     0,     0,     0,    43,    44,
       0,     0,     0,     0,     0,    45,    46,    47,     0,     0,
      48,    49,   169,     0,    50,     1,     2,     3,     4,     5,
       6,     7,     8,     9,     0,   125,     0,    11,    12,    13,
       0,     0,     0,     0,    18,     0,    20,     0,     0,     0,
       0,    22,    23,    24,    25,     0,     0,     0,    28,     0,
       0,     0,     0,     0,     0,     0,    36,     0,    37,    38,
       0,     0,   186,   187,   188,   189,   190,   191,    39,   192,
     193,   194,   195,   196,   197,   198,   199,     0,    40,    41,
      42,     0,     0,     0,    43,    44,     0,     0,     0,   169,
       0,   126,    46,    47,     0,     0,    48,    49,     0,     0,
      50,     1,     2,     3,     4,     5,     6,     7,     8,     9,
       0,   125,     0,    11,    12,    13,     0,     0,     0,     0,
      18,     0,    20,     0,     0,     0,     0,    22,    23,    24,
      25,    76,     0,     0,    28,     0,    14,    15,    16,    17,
       0,    19,    36,    21,    37,    38,   192,   193,   194,   195,
     196,   197,   198,   199,    39,    29,    30,    31,    32,    33,
      34,    35,     0,     0,    40,    41,    42,     0,     0,     0,
      43,    44,     0,     0,     0,     0,     0,    45,    46,    47,
       0,     0,    48,    49,     0,    76,    50,    77,     0,     0,
      14,    15,    16,    17,     0,    19,     0,    21,     0,    78,
      79,   322,     0,     0,     0,     0,     0,     0,     0,    29,
      30,    31,    32,    33,    34,    35,     0,    80,     0,     0,
      81,     0,    82,   169,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,     0,    96,    97,
      98,    99,   100,     0,   101,   102,   103,   104,     0,     0,
     105,   106,     0,   170,     0,     0,   107,   108,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,     0,
     192,   193,   194,   195,   196,   197,   198,   199,     0,     0,
     169,    76,     0,   209,     0,   365,    14,    15,    16,    17,
      76,    19,     0,    21,     0,    14,    15,    16,    17,     0,
      19,     0,    21,     0,     0,    29,    30,    31,    32,    33,
      34,    35,     0,     0,    29,    30,    31,    32,    33,    34,
      35,   210,     0,     0,   211,   169,     0,     0,     0,     0,
       0,   187,   188,   189,   190,   191,   212,   192,   193,   194,
     195,   196,   197,   198,   199,   169,   213,   214,     0,     0,
       0,   322,     0,     0,     0,   170,     0,     0,     0,   403,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   169,   192,   193,   194,   195,   196,   197,   198,   199,
       0,     0,     0,   355,     0,     0,     0,     0,   189,   190,
     191,     0,   192,   193,   194,   195,   196,   197,   198,   199,
       0,   170,     0,     0,     0,     0,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   169,   192,   193,
     194,   195,   196,   197,   198,   199,     0,     0,     0,   357,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   170,     0,     0,
       0,     0,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   169,   192,   193,   194,   195,   196,   197,
     198,   199,     0,     0,     0,   359,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   170,     0,     0,     0,     0,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   169,
     192,   193,   194,   195,   196,   197,   198,   199,     0,     0,
       0,   411,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   170,
       0,     0,     0,     0,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   169,   192,   193,   194,   195,
     196,   197,   198,   199,     0,     0,     0,   412,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   170,     0,     0,     0,     0,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   169,   192,   193,   194,   195,   196,   197,   198,   199,
       0,     0,     0,   413,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   170,     0,     0,     0,     0,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   169,   192,   193,
     194,   195,   196,   197,   198,   199,     0,     0,     0,   414,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   169,   170,     0,     0,
       0,     0,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   169,   192,   193,   194,   195,   196,   197,
     198,   199,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   169,   192,   193,   194,   195,   196,   197,   198,
     199,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   169,
     192,   193,   194,   195,   196,   197,   198,   199,     0,     0,
       0,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   169,   192,
     193,   194,   195,   196,   197,   198,   199,     0,     0,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   169,   192,   193,   194,   195,
     196,   197,   198,   199,     0,     0,     0,     0,     0,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   169,   192,   193,   194,   195,   196,
     197,   198,   199,     0,     0,     0,     0,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,     0,   192,   193,   194,   195,   196,   197,   198,   199,
       0,     0,     0,     0,     0,     0,     0,     0,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
       0,   192,   193,   194,   195,   196,   197,   198,   199,    76,
       0,     0,     0,     0,    14,    15,    16,    17,     0,    19,
      76,    21,     0,     0,     0,    14,    15,    16,    17,     0,
      19,     0,    21,    29,    30,    31,    32,    33,    34,    35,
       0,    26,    27,     0,    29,    30,    31,    32,    33,    34,
      35,     0,    76,     0,     0,     0,     0,    14,    15,    16,
      17,  -120,    19,     0,    21,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    29,    30,    31,    32,
      33,    34,    35
};

static const yytype_int16 yycheck[] =
{
      11,    64,   126,     0,   218,   218,   109,    18,     3,    14,
      15,    16,    17,     0,     3,   206,    21,   141,   223,   206,
      14,    15,    16,    17,    48,    15,    48,    21,    39,    40,
      41,    42,    43,    44,    10,    11,    15,    48,    49,    79,
      15,    17,    15,    13,    55,    13,   109,    10,    11,    48,
      13,    15,    48,    16,    17,    41,    42,    38,    55,    40,
      48,    38,    86,    40,    86,    70,    48,    72,   227,   228,
      38,    39,    40,    79,    53,   199,    70,    56,    72,    65,
      10,    11,    12,    13,    83,    48,    16,    17,    78,    68,
     214,    86,    62,    89,   194,    83,   196,    86,   303,    78,
      79,    83,    23,    78,    80,    78,    79,   210,   211,    13,
      24,    23,   326,   326,    78,    79,    37,    80,    59,    40,
      10,    11,   246,    13,    87,    37,    16,    17,    40,    79,
     141,   142,   143,   236,    38,    39,    40,    56,    41,    42,
     137,   138,   139,   140,   207,    15,   143,    79,    59,    68,
      80,    59,    59,    79,    42,    85,    41,   168,    24,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   372,   169,     0,   195,   372,   197,    48,     6,   194,
      80,   196,    24,    65,    65,    85,    33,    86,    53,   204,
     194,    56,   196,   327,    74,    75,    76,    77,    78,    79,
     204,   198,   223,    68,    12,    83,    12,    86,    12,   224,
      12,   226,   227,   228,    40,    59,    40,    40,    40,    40,
     224,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      83,    13,    14,    15,    16,    17,    10,    89,    23,   373,
      22,    24,    24,    68,   265,    83,   319,    29,    30,    31,
      32,    68,    37,    38,    36,    40,    86,    86,    43,    40,
      60,    60,    44,    83,    46,    47,    10,    11,    12,    13,
      86,    86,    16,    17,    56,    40,    60,    40,   293,    12,
      41,    42,   303,   298,    66,    67,    68,   308,   309,   293,
      72,    73,    53,    60,   298,    56,    83,    79,    80,    81,
      40,    12,    84,    85,    65,    83,    88,    68,    83,   382,
      48,    24,    60,   334,   335,    86,    86,    83,   333,    83,
      79,    12,   337,    12,    86,    79,    79,    87,    86,   333,
      86,    10,    11,   337,    13,   337,    80,    16,    17,   202,
      79,    85,   382,   216,   365,   214,    29,   236,   199,    10,
      11,    12,    13,    20,    -1,    16,    17,    10,    11,    12,
      13,    -1,    -1,    16,    17,    10,    11,    12,    13,   390,
      -1,    16,    17,   394,   395,   396,   397,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    13,    -1,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      -1,    80,    -1,    29,    30,    31,    32,    -1,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    80,
      46,    47,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      56,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    65,
      66,    67,    68,    -1,    -1,    -1,    72,    73,    -1,    -1,
      -1,    -1,    -1,    79,    80,    81,    -1,    -1,    84,    85,
      -1,    -1,    88,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    -1,    13,    -1,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    -1,    -1,    -1,    29,
      30,    31,    32,    -1,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    -1,    46,    47,    -1,    -1,
      -1,    10,    11,    12,    13,    -1,    56,    16,    17,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,    -1,
      -1,    -1,    72,    73,    -1,    -1,    -1,    -1,    -1,    79,
      80,    81,    -1,    -1,    84,    85,    15,    -1,    88,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    13,
      -1,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    -1,    -1,    -1,    29,    30,    31,    32,    -1,
      -1,    80,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    -1,    46,    47,    63,    64,    65,    66,    67,    68,
      69,    70,    56,    72,    73,    74,    75,    76,    77,    78,
      79,    -1,    66,    67,    68,    -1,    -1,    -1,    72,    73,
      -1,    -1,    -1,    -1,    -1,    79,    80,    81,    -1,    -1,
      84,    85,    15,    -1,    88,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    13,    -1,    15,    16,    17,
      -1,    -1,    -1,    -1,    22,    -1,    24,    -1,    -1,    -1,
      -1,    29,    30,    31,    32,    -1,    -1,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,    46,    47,
      -1,    -1,    65,    66,    67,    68,    69,    70,    56,    72,
      73,    74,    75,    76,    77,    78,    79,    -1,    66,    67,
      68,    -1,    -1,    -1,    72,    73,    -1,    -1,    -1,    15,
      -1,    79,    80,    81,    -1,    -1,    84,    85,    -1,    -1,
      88,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      -1,    13,    -1,    15,    16,    17,    -1,    -1,    -1,    -1,
      22,    -1,    24,    -1,    -1,    -1,    -1,    29,    30,    31,
      32,    13,    -1,    -1,    36,    -1,    18,    19,    20,    21,
      -1,    23,    44,    25,    46,    47,    72,    73,    74,    75,
      76,    77,    78,    79,    56,    37,    38,    39,    40,    41,
      42,    43,    -1,    -1,    66,    67,    68,    -1,    -1,    -1,
      72,    73,    -1,    -1,    -1,    -1,    -1,    79,    80,    81,
      -1,    -1,    84,    85,    -1,    13,    88,    15,    -1,    -1,
      18,    19,    20,    21,    -1,    23,    -1,    25,    -1,    27,
      28,    83,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      38,    39,    40,    41,    42,    43,    -1,    45,    -1,    -1,
      48,    -1,    50,    15,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    66,    67,
      68,    69,    70,    -1,    72,    73,    74,    75,    -1,    -1,
      78,    79,    -1,    45,    -1,    -1,    84,    85,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    -1,
      15,    13,    -1,    15,    -1,    87,    18,    19,    20,    21,
      13,    23,    -1,    25,    -1,    18,    19,    20,    21,    -1,
      23,    -1,    25,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    -1,    -1,    37,    38,    39,    40,    41,    42,
      43,    53,    -1,    -1,    56,    15,    -1,    -1,    -1,    -1,
      -1,    66,    67,    68,    69,    70,    68,    72,    73,    74,
      75,    76,    77,    78,    79,    15,    78,    79,    -1,    -1,
      -1,    83,    -1,    -1,    -1,    45,    -1,    -1,    -1,    82,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    15,    72,    73,    74,    75,    76,    77,    78,    79,
      -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,    68,    69,
      70,    -1,    72,    73,    74,    75,    76,    77,    78,    79,
      -1,    45,    -1,    -1,    -1,    -1,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    15,    72,    73,
      74,    75,    76,    77,    78,    79,    -1,    -1,    -1,    83,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      -1,    -1,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    15,    72,    73,    74,    75,    76,    77,
      78,    79,    -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    15,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    -1,
      -1,    83,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    -1,    -1,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    15,    72,    73,    74,    75,
      76,    77,    78,    79,    -1,    -1,    -1,    83,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    15,    72,    73,    74,    75,    76,    77,    78,    79,
      -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    -1,    -1,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    15,    72,    73,
      74,    75,    76,    77,    78,    79,    -1,    -1,    -1,    83,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    15,    45,    -1,    -1,
      -1,    -1,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    15,    72,    73,    74,    75,    76,    77,
      78,    79,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    15,    72,    73,    74,    75,    76,    77,    78,
      79,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    15,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    -1,
      -1,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    15,    72,
      73,    74,    75,    76,    77,    78,    79,    -1,    -1,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    15,    72,    73,    74,    75,
      76,    77,    78,    79,    -1,    -1,    -1,    -1,    -1,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    15,    72,    73,    74,    75,    76,
      77,    78,    79,    -1,    -1,    -1,    -1,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    72,    73,    74,    75,    76,    77,    78,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    72,    73,    74,    75,    76,    77,    78,    79,    13,
      -1,    -1,    -1,    -1,    18,    19,    20,    21,    -1,    23,
      13,    25,    -1,    -1,    -1,    18,    19,    20,    21,    -1,
      23,    -1,    25,    37,    38,    39,    40,    41,    42,    43,
      -1,    34,    35,    -1,    37,    38,    39,    40,    41,    42,
      43,    -1,    13,    -1,    -1,    -1,    -1,    18,    19,    20,
      21,    65,    23,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      13,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    29,    30,    31,    32,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    46,    47,    56,
      66,    67,    68,    72,    73,    79,    80,    81,    84,    85,
      88,    91,    92,    93,    94,   103,   106,   107,   108,   109,
     111,   112,   113,   123,   124,   128,   131,   132,   133,   135,
      79,    79,    24,    13,    14,    94,    13,    15,    27,    28,
      45,    48,    50,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    66,    67,    68,    69,
      70,    72,    73,    74,    75,    78,    79,    84,    85,   124,
     129,    10,    11,    12,    13,    16,    80,   133,   134,    12,
     134,    12,   134,    12,   134,    13,    79,    94,    13,    38,
      39,    40,   125,    11,    80,   135,   134,    59,    59,    59,
      59,    79,    79,    79,    38,    39,   125,    23,    37,    38,
      40,    43,    23,    37,    40,    42,    41,    94,    94,    94,
      94,    94,    94,    92,    93,    94,    94,     0,    48,    15,
      45,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    72,    73,    74,    75,    76,    77,    78,    79,
      92,    94,   104,     6,    24,    65,   110,   124,   113,    15,
      53,    56,    68,    78,    79,   115,   118,   119,   120,   121,
     122,    33,   134,   102,    85,   134,    95,    96,    97,    86,
      15,    78,    15,    78,    86,    83,   115,   130,    12,    12,
      12,    12,   123,    38,    40,    40,    59,    92,    92,    92,
      92,    94,   123,    94,    92,    94,    38,    40,    40,    40,
      23,    37,    40,    40,    40,    83,    83,    94,    93,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    12,    13,    85,   109,   134,    94,    12,    85,   109,
     134,    94,    93,   101,   123,   126,   127,    89,   105,    48,
     105,   134,    10,   111,   132,     3,    86,   115,   115,   117,
       3,    86,    83,   119,   126,    68,   120,    79,   121,   122,
      83,   104,   134,    24,    48,    87,    98,    99,   100,   134,
      98,    98,    86,    86,    86,    86,    68,   130,    83,    40,
     123,    60,    60,    60,    60,    83,    83,    83,    83,    83,
      40,    40,    40,    94,    86,    87,   134,    12,   134,    12,
      86,   104,    83,    48,    94,    94,    86,   113,   114,    86,
      83,    83,   116,    83,   134,    94,    94,    86,   100,   134,
      87,    86,    86,    60,    79,    79,    79,    79,    94,    12,
      12,    83,   111,    82,   123,   114,    94,    94,    94,    94,
      94,    83,    83,    83,    83
};

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
     115,   115,   115,   115,   118,   119,   119,   119,   120,   120,
     120,   120,   120,   121,   121,   121,   121,   122,   122,   123,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   125,   125,
     125,   125,   126,   126,   127,   127,   128,   128,   129,   130,
     130,   131,   131,   132,   132,   132,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   134,   134,   134,   134,   134,   134,   135,
     135,   135,   135
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
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
       1,     2,     1,     2,     1,     2,     1,     1,     3,     2,
       1,     2,     1,     2,     2,     3,     3,     2,     3,     1,
       1,     1,     1,     1,     2,     3,     2,     3,     3,     3,
       2,     2,     3,     4,     3,     3,     4,     3,     4,     3,
       4,     2,     3,     2,     3,     2,     3,     1,     2,     2,
       2,     3,     2,     2,     3,     2,     2,     3,     2,     2,
       3,     2,     1,     2,     1,     5,     2,     2,     1,     1,
       1,     1,     1,     3,     1,     3,     1,     2,     2,     0,
       2,     2,     2,     1,     1,     1,     2,     2,     4,     4,
       4,     4,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     3,
       3,     3,     2,     1,     1,     1,     1,     1,     1,     1,
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
#line 238 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode(pstate, OP_TYPE);
			  write_exp_elt_type(pstate, (yyvsp[0].tval));
			  write_exp_elt_opcode(pstate, OP_TYPE);}
#line 1990 "c-exp.c" /* yacc.c:1646  */
    break;

  case 5:
#line 242 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_TYPEOF);
			}
#line 1998 "c-exp.c" /* yacc.c:1646  */
    break;

  case 6:
#line 246 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_TYPE);
			  write_exp_elt_type (pstate, (yyvsp[-1].tval));
			  write_exp_elt_opcode (pstate, OP_TYPE);
			}
#line 2008 "c-exp.c" /* yacc.c:1646  */
    break;

  case 7:
#line 252 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_DECLTYPE);
			}
#line 2016 "c-exp.c" /* yacc.c:1646  */
    break;

  case 9:
#line 260 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_COMMA); }
#line 2022 "c-exp.c" /* yacc.c:1646  */
    break;

  case 10:
#line 265 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_IND); }
#line 2028 "c-exp.c" /* yacc.c:1646  */
    break;

  case 11:
#line 269 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ADDR); }
#line 2034 "c-exp.c" /* yacc.c:1646  */
    break;

  case 12:
#line 273 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_NEG); }
#line 2040 "c-exp.c" /* yacc.c:1646  */
    break;

  case 13:
#line 277 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PLUS); }
#line 2046 "c-exp.c" /* yacc.c:1646  */
    break;

  case 14:
#line 281 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
#line 2052 "c-exp.c" /* yacc.c:1646  */
    break;

  case 15:
#line 285 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_COMPLEMENT); }
#line 2058 "c-exp.c" /* yacc.c:1646  */
    break;

  case 16:
#line 289 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREINCREMENT); }
#line 2064 "c-exp.c" /* yacc.c:1646  */
    break;

  case 17:
#line 293 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREDECREMENT); }
#line 2070 "c-exp.c" /* yacc.c:1646  */
    break;

  case 18:
#line 297 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTINCREMENT); }
#line 2076 "c-exp.c" /* yacc.c:1646  */
    break;

  case 19:
#line 301 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTDECREMENT); }
#line 2082 "c-exp.c" /* yacc.c:1646  */
    break;

  case 20:
#line 305 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPEID); }
#line 2088 "c-exp.c" /* yacc.c:1646  */
    break;

  case 21:
#line 309 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPEID); }
#line 2094 "c-exp.c" /* yacc.c:1646  */
    break;

  case 22:
#line 313 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_SIZEOF); }
#line 2100 "c-exp.c" /* yacc.c:1646  */
    break;

  case 23:
#line 317 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2108 "c-exp.c" /* yacc.c:1646  */
    break;

  case 24:
#line 323 "c-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  write_exp_string (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2117 "c-exp.c" /* yacc.c:1646  */
    break;

  case 25:
#line 330 "c-exp.y" /* yacc.c:1646  */
    { struct stoken s;
			  mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (pstate, s);
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2129 "c-exp.c" /* yacc.c:1646  */
    break;

  case 26:
#line 340 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  write_destructor_name (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2137 "c-exp.c" /* yacc.c:1646  */
    break;

  case 27:
#line 346 "c-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  write_destructor_name (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2146 "c-exp.c" /* yacc.c:1646  */
    break;

  case 28:
#line 353 "c-exp.y" /* yacc.c:1646  */
    { /* exp->type::name becomes exp->*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (pstate, UNOP_ADDR);
			  write_exp_elt_opcode (pstate, STRUCTOP_MPTR); }
#line 2156 "c-exp.c" /* yacc.c:1646  */
    break;

  case 29:
#line 361 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_MPTR); }
#line 2162 "c-exp.c" /* yacc.c:1646  */
    break;

  case 30:
#line 365 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2170 "c-exp.c" /* yacc.c:1646  */
    break;

  case 31:
#line 371 "c-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2179 "c-exp.c" /* yacc.c:1646  */
    break;

  case 32:
#line 378 "c-exp.y" /* yacc.c:1646  */
    { struct stoken s;
			  mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (pstate, s);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2191 "c-exp.c" /* yacc.c:1646  */
    break;

  case 33:
#line 388 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_destructor_name (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2199 "c-exp.c" /* yacc.c:1646  */
    break;

  case 34:
#line 394 "c-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_destructor_name (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2208 "c-exp.c" /* yacc.c:1646  */
    break;

  case 35:
#line 401 "c-exp.y" /* yacc.c:1646  */
    { /* exp.type::name becomes exp.*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (pstate, UNOP_ADDR);
			  write_exp_elt_opcode (pstate, STRUCTOP_MEMBER); }
#line 2218 "c-exp.c" /* yacc.c:1646  */
    break;

  case 36:
#line 409 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_MEMBER); }
#line 2224 "c-exp.c" /* yacc.c:1646  */
    break;

  case 37:
#line 413 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT); }
#line 2230 "c-exp.c" /* yacc.c:1646  */
    break;

  case 38:
#line 417 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT); }
#line 2236 "c-exp.c" /* yacc.c:1646  */
    break;

  case 39:
#line 426 "c-exp.y" /* yacc.c:1646  */
    {
			  CORE_ADDR theclass;

			  theclass = lookup_objc_class (parse_gdbarch (pstate),
						     copy_name ((yyvsp[0].tsym).stoken));
			  if (theclass == 0)
			    error (_("%s is not an ObjC Class"),
				   copy_name ((yyvsp[0].tsym).stoken));
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate,
					    parse_type (pstate)->builtin_int);
			  write_exp_elt_longcst (pstate, (LONGEST) theclass);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  start_msglist();
			}
#line 2256 "c-exp.c" /* yacc.c:1646  */
    break;

  case 40:
#line 442 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			  end_msglist (pstate);
			  write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			}
#line 2265 "c-exp.c" /* yacc.c:1646  */
    break;

  case 41:
#line 449 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate,
					    parse_type (pstate)->builtin_int);
			  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].theclass).theclass);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  start_msglist();
			}
#line 2278 "c-exp.c" /* yacc.c:1646  */
    break;

  case 42:
#line 458 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			  end_msglist (pstate);
			  write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			}
#line 2287 "c-exp.c" /* yacc.c:1646  */
    break;

  case 43:
#line 465 "c-exp.y" /* yacc.c:1646  */
    { start_msglist(); }
#line 2293 "c-exp.c" /* yacc.c:1646  */
    break;

  case 44:
#line 467 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			  end_msglist (pstate);
			  write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			}
#line 2302 "c-exp.c" /* yacc.c:1646  */
    break;

  case 45:
#line 474 "c-exp.y" /* yacc.c:1646  */
    { add_msglist(&(yyvsp[0].sval), 0); }
#line 2308 "c-exp.c" /* yacc.c:1646  */
    break;

  case 49:
#line 483 "c-exp.y" /* yacc.c:1646  */
    { add_msglist(&(yyvsp[-2].sval), 1); }
#line 2314 "c-exp.c" /* yacc.c:1646  */
    break;

  case 50:
#line 485 "c-exp.y" /* yacc.c:1646  */
    { add_msglist(0, 1);   }
#line 2320 "c-exp.c" /* yacc.c:1646  */
    break;

  case 51:
#line 487 "c-exp.y" /* yacc.c:1646  */
    { add_msglist(0, 0);   }
#line 2326 "c-exp.c" /* yacc.c:1646  */
    break;

  case 52:
#line 493 "c-exp.y" /* yacc.c:1646  */
    { start_arglist (); }
#line 2332 "c-exp.c" /* yacc.c:1646  */
    break;

  case 53:
#line 495 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_FUNCALL);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) end_arglist ());
			  write_exp_elt_opcode (pstate, OP_FUNCALL); }
#line 2341 "c-exp.c" /* yacc.c:1646  */
    break;

  case 54:
#line 502 "c-exp.y" /* yacc.c:1646  */
    {
			  /* This could potentially be a an argument defined
			     lookup function (Koenig).  */
			  write_exp_elt_opcode (pstate, OP_ADL_FUNC);
			  write_exp_elt_block (pstate,
					       expression_context_block);
			  write_exp_elt_sym (pstate,
					     NULL); /* Placeholder.  */
			  write_exp_string (pstate, (yyvsp[-1].ssym).stoken);
			  write_exp_elt_opcode (pstate, OP_ADL_FUNC);

			/* This is to save the value of arglist_len
			   being accumulated by an outer function call.  */

			  start_arglist ();
			}
#line 2362 "c-exp.c" /* yacc.c:1646  */
    break;

  case 55:
#line 519 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_FUNCALL);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) end_arglist ());
			  write_exp_elt_opcode (pstate, OP_FUNCALL);
			}
#line 2373 "c-exp.c" /* yacc.c:1646  */
    break;

  case 56:
#line 528 "c-exp.y" /* yacc.c:1646  */
    { start_arglist (); }
#line 2379 "c-exp.c" /* yacc.c:1646  */
    break;

  case 58:
#line 535 "c-exp.y" /* yacc.c:1646  */
    { arglist_len = 1; }
#line 2385 "c-exp.c" /* yacc.c:1646  */
    break;

  case 59:
#line 539 "c-exp.y" /* yacc.c:1646  */
    { arglist_len++; }
#line 2391 "c-exp.c" /* yacc.c:1646  */
    break;

  case 60:
#line 543 "c-exp.y" /* yacc.c:1646  */
    { int i;
			  VEC (type_ptr) *type_list = (yyvsp[-2].tvec);
			  struct type *type_elt;
			  LONGEST len = VEC_length (type_ptr, type_list);

			  write_exp_elt_opcode (pstate, TYPE_INSTANCE);
			  write_exp_elt_longcst (pstate, len);
			  for (i = 0;
			       VEC_iterate (type_ptr, type_list, i, type_elt);
			       ++i)
			    write_exp_elt_type (pstate, type_elt);
			  write_exp_elt_longcst(pstate, len);
			  write_exp_elt_opcode (pstate, TYPE_INSTANCE);
			  VEC_free (type_ptr, type_list);
			}
#line 2411 "c-exp.c" /* yacc.c:1646  */
    break;

  case 61:
#line 561 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = end_arglist () - 1; }
#line 2417 "c-exp.c" /* yacc.c:1646  */
    break;

  case 62:
#line 564 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ARRAY);
			  write_exp_elt_longcst (pstate, (LONGEST) 0);
			  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].lval));
			  write_exp_elt_opcode (pstate, OP_ARRAY); }
#line 2426 "c-exp.c" /* yacc.c:1646  */
    break;

  case 63:
#line 571 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_MEMVAL_TYPE); }
#line 2432 "c-exp.c" /* yacc.c:1646  */
    break;

  case 64:
#line 575 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST_TYPE); }
#line 2438 "c-exp.c" /* yacc.c:1646  */
    break;

  case 65:
#line 579 "c-exp.y" /* yacc.c:1646  */
    { }
#line 2444 "c-exp.c" /* yacc.c:1646  */
    break;

  case 66:
#line 585 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REPEAT); }
#line 2450 "c-exp.c" /* yacc.c:1646  */
    break;

  case 67:
#line 589 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_MUL); }
#line 2456 "c-exp.c" /* yacc.c:1646  */
    break;

  case 68:
#line 593 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_DIV); }
#line 2462 "c-exp.c" /* yacc.c:1646  */
    break;

  case 69:
#line 597 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REM); }
#line 2468 "c-exp.c" /* yacc.c:1646  */
    break;

  case 70:
#line 601 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ADD); }
#line 2474 "c-exp.c" /* yacc.c:1646  */
    break;

  case 71:
#line 605 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUB); }
#line 2480 "c-exp.c" /* yacc.c:1646  */
    break;

  case 72:
#line 609 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LSH); }
#line 2486 "c-exp.c" /* yacc.c:1646  */
    break;

  case 73:
#line 613 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_RSH); }
#line 2492 "c-exp.c" /* yacc.c:1646  */
    break;

  case 74:
#line 617 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL); }
#line 2498 "c-exp.c" /* yacc.c:1646  */
    break;

  case 75:
#line 621 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
#line 2504 "c-exp.c" /* yacc.c:1646  */
    break;

  case 76:
#line 625 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LEQ); }
#line 2510 "c-exp.c" /* yacc.c:1646  */
    break;

  case 77:
#line 629 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GEQ); }
#line 2516 "c-exp.c" /* yacc.c:1646  */
    break;

  case 78:
#line 633 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LESS); }
#line 2522 "c-exp.c" /* yacc.c:1646  */
    break;

  case 79:
#line 637 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GTR); }
#line 2528 "c-exp.c" /* yacc.c:1646  */
    break;

  case 80:
#line 641 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
#line 2534 "c-exp.c" /* yacc.c:1646  */
    break;

  case 81:
#line 645 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
#line 2540 "c-exp.c" /* yacc.c:1646  */
    break;

  case 82:
#line 649 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
#line 2546 "c-exp.c" /* yacc.c:1646  */
    break;

  case 83:
#line 653 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
#line 2552 "c-exp.c" /* yacc.c:1646  */
    break;

  case 84:
#line 657 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
#line 2558 "c-exp.c" /* yacc.c:1646  */
    break;

  case 85:
#line 661 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_COND); }
#line 2564 "c-exp.c" /* yacc.c:1646  */
    break;

  case 86:
#line 665 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
#line 2570 "c-exp.c" /* yacc.c:1646  */
    break;

  case 87:
#line 669 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode (pstate, (yyvsp[-1].opcode));
			  write_exp_elt_opcode (pstate,
						BINOP_ASSIGN_MODIFY); }
#line 2579 "c-exp.c" /* yacc.c:1646  */
    break;

  case 88:
#line 676 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, (yyvsp[0].typed_val_int).type);
			  write_exp_elt_longcst (pstate, (LONGEST) ((yyvsp[0].typed_val_int).val));
			  write_exp_elt_opcode (pstate, OP_LONG); }
#line 2588 "c-exp.c" /* yacc.c:1646  */
    break;

  case 89:
#line 683 "c-exp.y" /* yacc.c:1646  */
    {
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &(yyvsp[0].tsval);
			  write_exp_string_vector (pstate, (yyvsp[0].tsval).type, &vec);
			}
#line 2599 "c-exp.c" /* yacc.c:1646  */
    break;

  case 90:
#line 692 "c-exp.y" /* yacc.c:1646  */
    { YYSTYPE val;
			  parse_number (pstate, (yyvsp[0].ssym).stoken.ptr,
					(yyvsp[0].ssym).stoken.length, 0, &val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, val.typed_val_int.type);
			  write_exp_elt_longcst (pstate,
					    (LONGEST) val.typed_val_int.val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			}
#line 2613 "c-exp.c" /* yacc.c:1646  */
    break;

  case 91:
#line 705 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DOUBLE);
			  write_exp_elt_type (pstate, (yyvsp[0].typed_val_float).type);
			  write_exp_elt_dblcst (pstate, (yyvsp[0].typed_val_float).dval);
			  write_exp_elt_opcode (pstate, OP_DOUBLE); }
#line 2622 "c-exp.c" /* yacc.c:1646  */
    break;

  case 92:
#line 712 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_DECFLOAT);
			  write_exp_elt_type (pstate, (yyvsp[0].typed_val_decfloat).type);
			  write_exp_elt_decfloatcst (pstate, (yyvsp[0].typed_val_decfloat).val);
			  write_exp_elt_opcode (pstate, OP_DECFLOAT); }
#line 2631 "c-exp.c" /* yacc.c:1646  */
    break;

  case 94:
#line 722 "c-exp.y" /* yacc.c:1646  */
    {
			  write_dollar_variable (pstate, (yyvsp[0].sval));
			}
#line 2639 "c-exp.c" /* yacc.c:1646  */
    break;

  case 95:
#line 728 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_OBJC_SELECTOR);
			  write_exp_string (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, OP_OBJC_SELECTOR); }
#line 2648 "c-exp.c" /* yacc.c:1646  */
    break;

  case 96:
#line 735 "c-exp.y" /* yacc.c:1646  */
    { struct type *type = (yyvsp[-1].tval);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, lookup_signed_typename
					      (parse_language (pstate),
					       parse_gdbarch (pstate),
					       "int"));
			  type = check_typedef (type);

			    /* $5.3.3/2 of the C++ Standard (n3290 draft)
			       says of sizeof:  "When applied to a reference
			       or a reference type, the result is the size of
			       the referenced type."  */
			  if (TYPE_IS_REFERENCE (type))
			    type = check_typedef (TYPE_TARGET_TYPE (type));
			  write_exp_elt_longcst (pstate,
						 (LONGEST) TYPE_LENGTH (type));
			  write_exp_elt_opcode (pstate, OP_LONG); }
#line 2670 "c-exp.c" /* yacc.c:1646  */
    break;

  case 97:
#line 755 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate,
						UNOP_REINTERPRET_CAST); }
#line 2677 "c-exp.c" /* yacc.c:1646  */
    break;

  case 98:
#line 760 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST_TYPE); }
#line 2683 "c-exp.c" /* yacc.c:1646  */
    break;

  case 99:
#line 764 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_DYNAMIC_CAST); }
#line 2689 "c-exp.c" /* yacc.c:1646  */
    break;

  case 100:
#line 768 "c-exp.y" /* yacc.c:1646  */
    { /* We could do more error checking here, but
			     it doesn't seem worthwhile.  */
			  write_exp_elt_opcode (pstate, UNOP_CAST_TYPE); }
#line 2697 "c-exp.c" /* yacc.c:1646  */
    break;

  case 101:
#line 775 "c-exp.y" /* yacc.c:1646  */
    {
			  /* We copy the string here, and not in the
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
#line 2717 "c-exp.c" /* yacc.c:1646  */
    break;

  case 102:
#line 792 "c-exp.y" /* yacc.c:1646  */
    {
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  char *p;
			  ++(yyval.svec).len;
			  (yyval.svec).tokens = XRESIZEVEC (struct typed_stoken,
						  (yyval.svec).tokens, (yyval.svec).len);

			  p = (char *) xmalloc ((yyvsp[0].tsval).length + 1);
			  memcpy (p, (yyvsp[0].tsval).ptr, (yyvsp[0].tsval).length + 1);

			  (yyval.svec).tokens[(yyval.svec).len - 1].type = (yyvsp[0].tsval).type;
			  (yyval.svec).tokens[(yyval.svec).len - 1].length = (yyvsp[0].tsval).length;
			  (yyval.svec).tokens[(yyval.svec).len - 1].ptr = p;
			}
#line 2737 "c-exp.c" /* yacc.c:1646  */
    break;

  case 103:
#line 810 "c-exp.y" /* yacc.c:1646  */
    {
			  int i;
			  c_string_type type = C_STRING;

			  for (i = 0; i < (yyvsp[0].svec).len; ++i)
			    {
			      switch ((yyvsp[0].svec).tokens[i].type)
				{
				case C_STRING:
				  break;
				case C_WIDE_STRING:
				case C_STRING_16:
				case C_STRING_32:
				  if (type != C_STRING
				      && type != (yyvsp[0].svec).tokens[i].type)
				    error (_("Undefined string concatenation."));
				  type = (enum c_string_type_values) (yyvsp[0].svec).tokens[i].type;
				  break;
				default:
				  /* internal error */
				  internal_error (__FILE__, __LINE__,
						  "unrecognized type in string concatenation");
				}
			    }

			  write_exp_string_vector (pstate, type, &(yyvsp[0].svec));
			  for (i = 0; i < (yyvsp[0].svec).len; ++i)
			    xfree ((yyvsp[0].svec).tokens[i].ptr);
			  xfree ((yyvsp[0].svec).tokens);
			}
#line 2772 "c-exp.c" /* yacc.c:1646  */
    break;

  case 104:
#line 845 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OBJC_NSSTRING);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, OP_OBJC_NSSTRING); }
#line 2780 "c-exp.c" /* yacc.c:1646  */
    break;

  case 105:
#line 852 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
                          write_exp_elt_type (pstate,
					  parse_type (pstate)->builtin_bool);
                          write_exp_elt_longcst (pstate, (LONGEST) 1);
                          write_exp_elt_opcode (pstate, OP_LONG); }
#line 2790 "c-exp.c" /* yacc.c:1646  */
    break;

  case 106:
#line 860 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
                          write_exp_elt_type (pstate,
					  parse_type (pstate)->builtin_bool);
                          write_exp_elt_longcst (pstate, (LONGEST) 0);
                          write_exp_elt_opcode (pstate, OP_LONG); }
#line 2800 "c-exp.c" /* yacc.c:1646  */
    break;

  case 107:
#line 870 "c-exp.y" /* yacc.c:1646  */
    {
			  if ((yyvsp[0].ssym).sym.symbol)
			    (yyval.bval) = SYMBOL_BLOCK_VALUE ((yyvsp[0].ssym).sym.symbol);
			  else
			    error (_("No file or function \"%s\"."),
				   copy_name ((yyvsp[0].ssym).stoken));
			}
#line 2812 "c-exp.c" /* yacc.c:1646  */
    break;

  case 108:
#line 878 "c-exp.y" /* yacc.c:1646  */
    {
			  (yyval.bval) = (yyvsp[0].bval);
			}
#line 2820 "c-exp.c" /* yacc.c:1646  */
    break;

  case 109:
#line 884 "c-exp.y" /* yacc.c:1646  */
    { struct symbol *tem
			    = lookup_symbol (copy_name ((yyvsp[0].sval)), (yyvsp[-2].bval),
					     VAR_DOMAIN, NULL).symbol;

			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy_name ((yyvsp[0].sval)));
			  (yyval.bval) = SYMBOL_BLOCK_VALUE (tem); }
#line 2833 "c-exp.c" /* yacc.c:1646  */
    break;

  case 110:
#line 895 "c-exp.y" /* yacc.c:1646  */
    { struct symbol *sym = (yyvsp[-1].ssym).sym.symbol;

			  if (sym == NULL || !SYMBOL_IS_ARGUMENT (sym)
			      || !symbol_read_needs_frame (sym))
			    error (_("@entry can be used only for function "
				     "parameters, not for \"%s\""),
				   copy_name ((yyvsp[-1].ssym).stoken));

			  write_exp_elt_opcode (pstate, OP_VAR_ENTRY_VALUE);
			  write_exp_elt_sym (pstate, sym);
			  write_exp_elt_opcode (pstate, OP_VAR_ENTRY_VALUE);
			}
#line 2850 "c-exp.c" /* yacc.c:1646  */
    break;

  case 111:
#line 910 "c-exp.y" /* yacc.c:1646  */
    { struct block_symbol sym
			    = lookup_symbol (copy_name ((yyvsp[0].sval)), (yyvsp[-2].bval),
					     VAR_DOMAIN, NULL);

			  if (sym.symbol == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy_name ((yyvsp[0].sval)));
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
			  write_exp_elt_opcode (pstate, OP_VAR_VALUE); }
#line 2874 "c-exp.c" /* yacc.c:1646  */
    break;

  case 112:
#line 932 "c-exp.y" /* yacc.c:1646  */
    {
			  struct type *type = (yyvsp[-2].tsym).type;
			  type = check_typedef (type);
			  if (!type_aggregate_p (type))
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));

			  write_exp_elt_opcode (pstate, OP_SCOPE);
			  write_exp_elt_type (pstate, type);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, OP_SCOPE);
			}
#line 2891 "c-exp.c" /* yacc.c:1646  */
    break;

  case 113:
#line 945 "c-exp.y" /* yacc.c:1646  */
    {
			  struct type *type = (yyvsp[-3].tsym).type;
			  struct stoken tmp_token;
			  char *buf;

			  type = check_typedef (type);
			  if (!type_aggregate_p (type))
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));
			  buf = (char *) alloca ((yyvsp[0].sval).length + 2);
			  tmp_token.ptr = buf;
			  tmp_token.length = (yyvsp[0].sval).length + 1;
			  buf[0] = '~';
			  memcpy (buf+1, (yyvsp[0].sval).ptr, (yyvsp[0].sval).length);
			  buf[tmp_token.length] = 0;

			  /* Check for valid destructor name.  */
			  destructor_name_p (tmp_token.ptr, (yyvsp[-3].tsym).type);
			  write_exp_elt_opcode (pstate, OP_SCOPE);
			  write_exp_elt_type (pstate, type);
			  write_exp_string (pstate, tmp_token);
			  write_exp_elt_opcode (pstate, OP_SCOPE);
			}
#line 2919 "c-exp.c" /* yacc.c:1646  */
    break;

  case 114:
#line 969 "c-exp.y" /* yacc.c:1646  */
    {
			  char *copy = copy_name ((yyvsp[-2].sval));
			  error (_("No type \"%s\" within class "
				   "or namespace \"%s\"."),
				 copy, TYPE_SAFE_NAME ((yyvsp[-4].tsym).type));
			}
#line 2930 "c-exp.c" /* yacc.c:1646  */
    break;

  case 116:
#line 979 "c-exp.y" /* yacc.c:1646  */
    {
			  char *name = copy_name ((yyvsp[0].ssym).stoken);
			  struct symbol *sym;
			  struct bound_minimal_symbol msymbol;

			  sym
			    = lookup_symbol (name, (const struct block *) NULL,
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
			  else if (!have_full_symbols () && !have_partial_symbols ())
			    error (_("No symbol table is loaded.  Use the \"file\" command."));
			  else
			    error (_("No symbol \"%s\" in current context."), name);
			}
#line 2960 "c-exp.c" /* yacc.c:1646  */
    break;

  case 117:
#line 1007 "c-exp.y" /* yacc.c:1646  */
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
			    }
			  else if ((yyvsp[0].ssym).is_a_field_of_this)
			    {
			      /* C++: it hangs off of `this'.  Must
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
			    }
			  else
			    {
			      struct bound_minimal_symbol msymbol;
			      char *arg = copy_name ((yyvsp[0].ssym).stoken);

			      msymbol =
				lookup_bound_minimal_symbol (arg);
			      if (msymbol.minsym != NULL)
				write_exp_msymbol (pstate, msymbol);
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error (_("No symbol table is loaded.  Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       copy_name ((yyvsp[0].ssym).stoken));
			    }
			}
#line 3013 "c-exp.c" /* yacc.c:1646  */
    break;

  case 118:
#line 1058 "c-exp.y" /* yacc.c:1646  */
    { insert_type_address_space (pstate, copy_name ((yyvsp[0].ssym).stoken)); }
#line 3019 "c-exp.c" /* yacc.c:1646  */
    break;

  case 126:
#line 1079 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_pointer); }
#line 3025 "c-exp.c" /* yacc.c:1646  */
    break;

  case 128:
#line 1082 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_pointer); }
#line 3031 "c-exp.c" /* yacc.c:1646  */
    break;

  case 130:
#line 1085 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_reference); }
#line 3037 "c-exp.c" /* yacc.c:1646  */
    break;

  case 131:
#line 1087 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_reference); }
#line 3043 "c-exp.c" /* yacc.c:1646  */
    break;

  case 132:
#line 1089 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_rvalue_reference); }
#line 3049 "c-exp.c" /* yacc.c:1646  */
    break;

  case 133:
#line 1091 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_rvalue_reference); }
#line 3055 "c-exp.c" /* yacc.c:1646  */
    break;

  case 134:
#line 1095 "c-exp.y" /* yacc.c:1646  */
    {
			  (yyval.type_stack) = get_type_stack ();
			  /* This cleanup is eventually run by
			     c_parse.  */
			  make_cleanup (type_stack_cleanup, (yyval.type_stack));
			}
#line 3066 "c-exp.c" /* yacc.c:1646  */
    break;

  case 135:
#line 1104 "c-exp.y" /* yacc.c:1646  */
    { (yyval.type_stack) = append_type_stack ((yyvsp[0].type_stack), (yyvsp[-1].type_stack)); }
#line 3072 "c-exp.c" /* yacc.c:1646  */
    break;

  case 138:
#line 1110 "c-exp.y" /* yacc.c:1646  */
    { (yyval.type_stack) = (yyvsp[-1].type_stack); }
#line 3078 "c-exp.c" /* yacc.c:1646  */
    break;

  case 139:
#line 1112 "c-exp.y" /* yacc.c:1646  */
    {
			  push_type_stack ((yyvsp[-1].type_stack));
			  push_type_int ((yyvsp[0].lval));
			  push_type (tp_array);
			  (yyval.type_stack) = get_type_stack ();
			}
#line 3089 "c-exp.c" /* yacc.c:1646  */
    break;

  case 140:
#line 1119 "c-exp.y" /* yacc.c:1646  */
    {
			  push_type_int ((yyvsp[0].lval));
			  push_type (tp_array);
			  (yyval.type_stack) = get_type_stack ();
			}
#line 3099 "c-exp.c" /* yacc.c:1646  */
    break;

  case 141:
#line 1126 "c-exp.y" /* yacc.c:1646  */
    {
			  push_type_stack ((yyvsp[-1].type_stack));
			  push_typelist ((yyvsp[0].tvec));
			  (yyval.type_stack) = get_type_stack ();
			}
#line 3109 "c-exp.c" /* yacc.c:1646  */
    break;

  case 142:
#line 1132 "c-exp.y" /* yacc.c:1646  */
    {
			  push_typelist ((yyvsp[0].tvec));
			  (yyval.type_stack) = get_type_stack ();
			}
#line 3118 "c-exp.c" /* yacc.c:1646  */
    break;

  case 143:
#line 1139 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = -1; }
#line 3124 "c-exp.c" /* yacc.c:1646  */
    break;

  case 144:
#line 1141 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = -1; }
#line 3130 "c-exp.c" /* yacc.c:1646  */
    break;

  case 145:
#line 1143 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-1].typed_val_int).val; }
#line 3136 "c-exp.c" /* yacc.c:1646  */
    break;

  case 146:
#line 1145 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-1].typed_val_int).val; }
#line 3142 "c-exp.c" /* yacc.c:1646  */
    break;

  case 147:
#line 1149 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tvec) = NULL; }
#line 3148 "c-exp.c" /* yacc.c:1646  */
    break;

  case 148:
#line 1151 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tvec) = (yyvsp[-1].tvec); }
#line 3154 "c-exp.c" /* yacc.c:1646  */
    break;

  case 150:
#line 1167 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 3160 "c-exp.c" /* yacc.c:1646  */
    break;

  case 151:
#line 1169 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "int"); }
#line 3168 "c-exp.c" /* yacc.c:1646  */
    break;

  case 152:
#line 1173 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3176 "c-exp.c" /* yacc.c:1646  */
    break;

  case 153:
#line 1177 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "short"); }
#line 3184 "c-exp.c" /* yacc.c:1646  */
    break;

  case 154:
#line 1181 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3192 "c-exp.c" /* yacc.c:1646  */
    break;

  case 155:
#line 1185 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3200 "c-exp.c" /* yacc.c:1646  */
    break;

  case 156:
#line 1189 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3208 "c-exp.c" /* yacc.c:1646  */
    break;

  case 157:
#line 1193 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3216 "c-exp.c" /* yacc.c:1646  */
    break;

  case 158:
#line 1197 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long"); }
#line 3224 "c-exp.c" /* yacc.c:1646  */
    break;

  case 159:
#line 1201 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long"); }
#line 3232 "c-exp.c" /* yacc.c:1646  */
    break;

  case 160:
#line 1205 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long"); }
#line 3240 "c-exp.c" /* yacc.c:1646  */
    break;

  case 161:
#line 1209 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3248 "c-exp.c" /* yacc.c:1646  */
    break;

  case 162:
#line 1213 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3256 "c-exp.c" /* yacc.c:1646  */
    break;

  case 163:
#line 1217 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3264 "c-exp.c" /* yacc.c:1646  */
    break;

  case 164:
#line 1221 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3272 "c-exp.c" /* yacc.c:1646  */
    break;

  case 165:
#line 1225 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3280 "c-exp.c" /* yacc.c:1646  */
    break;

  case 166:
#line 1229 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3288 "c-exp.c" /* yacc.c:1646  */
    break;

  case 167:
#line 1233 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long long"); }
#line 3296 "c-exp.c" /* yacc.c:1646  */
    break;

  case 168:
#line 1237 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long long"); }
#line 3304 "c-exp.c" /* yacc.c:1646  */
    break;

  case 169:
#line 1241 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long long"); }
#line 3312 "c-exp.c" /* yacc.c:1646  */
    break;

  case 170:
#line 1245 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long long"); }
#line 3320 "c-exp.c" /* yacc.c:1646  */
    break;

  case 171:
#line 1249 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "short"); }
#line 3328 "c-exp.c" /* yacc.c:1646  */
    break;

  case 172:
#line 1253 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "short"); }
#line 3336 "c-exp.c" /* yacc.c:1646  */
    break;

  case 173:
#line 1257 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "short"); }
#line 3344 "c-exp.c" /* yacc.c:1646  */
    break;

  case 174:
#line 1261 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "short"); }
#line 3352 "c-exp.c" /* yacc.c:1646  */
    break;

  case 175:
#line 1265 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "short"); }
#line 3360 "c-exp.c" /* yacc.c:1646  */
    break;

  case 176:
#line 1269 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "short"); }
#line 3368 "c-exp.c" /* yacc.c:1646  */
    break;

  case 177:
#line 1273 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_typename (parse_language (pstate),
						parse_gdbarch (pstate),
						"double",
						(struct block *) NULL,
						0); }
#line 3378 "c-exp.c" /* yacc.c:1646  */
    break;

  case 178:
#line 1279 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_typename (parse_language (pstate),
						parse_gdbarch (pstate),
						"long double",
						(struct block *) NULL,
						0); }
#line 3388 "c-exp.c" /* yacc.c:1646  */
    break;

  case 179:
#line 1285 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
					      expression_context_block); }
#line 3395 "c-exp.c" /* yacc.c:1646  */
    break;

  case 180:
#line 1288 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3404 "c-exp.c" /* yacc.c:1646  */
    break;

  case 181:
#line 1293 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, (yyvsp[-1].sval).ptr,
					       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3414 "c-exp.c" /* yacc.c:1646  */
    break;

  case 182:
#line 1299 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
					      expression_context_block); }
#line 3421 "c-exp.c" /* yacc.c:1646  */
    break;

  case 183:
#line 1302 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3430 "c-exp.c" /* yacc.c:1646  */
    break;

  case 184:
#line 1307 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, (yyvsp[-1].sval).ptr,
					       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3440 "c-exp.c" /* yacc.c:1646  */
    break;

  case 185:
#line 1313 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_union (copy_name ((yyvsp[0].sval)),
					     expression_context_block); }
#line 3447 "c-exp.c" /* yacc.c:1646  */
    break;

  case 186:
#line 1316 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_UNION, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3456 "c-exp.c" /* yacc.c:1646  */
    break;

  case 187:
#line 1321 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_UNION, (yyvsp[-1].sval).ptr,
					       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3466 "c-exp.c" /* yacc.c:1646  */
    break;

  case 188:
#line 1327 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_enum (copy_name ((yyvsp[0].sval)),
					    expression_context_block); }
#line 3473 "c-exp.c" /* yacc.c:1646  */
    break;

  case 189:
#line 1330 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_ENUM, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3482 "c-exp.c" /* yacc.c:1646  */
    break;

  case 190:
#line 1335 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_ENUM, (yyvsp[-1].sval).ptr,
					       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3492 "c-exp.c" /* yacc.c:1646  */
    break;

  case 191:
#line 1341 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 TYPE_NAME((yyvsp[0].tsym).type)); }
#line 3500 "c-exp.c" /* yacc.c:1646  */
    break;

  case 192:
#line 1345 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "int"); }
#line 3508 "c-exp.c" /* yacc.c:1646  */
    break;

  case 193:
#line 1349 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       TYPE_NAME((yyvsp[0].tsym).type)); }
#line 3516 "c-exp.c" /* yacc.c:1646  */
    break;

  case 194:
#line 1353 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "int"); }
#line 3524 "c-exp.c" /* yacc.c:1646  */
    break;

  case 195:
#line 1360 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_template_type(copy_name((yyvsp[-3].sval)), (yyvsp[-1].tval),
						    expression_context_block);
			}
#line 3532 "c-exp.c" /* yacc.c:1646  */
    break;

  case 196:
#line 1364 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = follow_types ((yyvsp[0].tval)); }
#line 3538 "c-exp.c" /* yacc.c:1646  */
    break;

  case 197:
#line 1366 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = follow_types ((yyvsp[-1].tval)); }
#line 3544 "c-exp.c" /* yacc.c:1646  */
    break;

  case 199:
#line 1371 "c-exp.y" /* yacc.c:1646  */
    {
		  (yyval.tsym).stoken.ptr = "int";
		  (yyval.tsym).stoken.length = 3;
		  (yyval.tsym).type = lookup_signed_typename (parse_language (pstate),
						    parse_gdbarch (pstate),
						    "int");
		}
#line 3556 "c-exp.c" /* yacc.c:1646  */
    break;

  case 200:
#line 1379 "c-exp.y" /* yacc.c:1646  */
    {
		  (yyval.tsym).stoken.ptr = "long";
		  (yyval.tsym).stoken.length = 4;
		  (yyval.tsym).type = lookup_signed_typename (parse_language (pstate),
						    parse_gdbarch (pstate),
						    "long");
		}
#line 3568 "c-exp.c" /* yacc.c:1646  */
    break;

  case 201:
#line 1387 "c-exp.y" /* yacc.c:1646  */
    {
		  (yyval.tsym).stoken.ptr = "short";
		  (yyval.tsym).stoken.length = 5;
		  (yyval.tsym).type = lookup_signed_typename (parse_language (pstate),
						    parse_gdbarch (pstate),
						    "short");
		}
#line 3580 "c-exp.c" /* yacc.c:1646  */
    break;

  case 202:
#line 1398 "c-exp.y" /* yacc.c:1646  */
    { check_parameter_typelist ((yyvsp[0].tvec)); }
#line 3586 "c-exp.c" /* yacc.c:1646  */
    break;

  case 203:
#line 1400 "c-exp.y" /* yacc.c:1646  */
    {
			  VEC_safe_push (type_ptr, (yyvsp[-2].tvec), NULL);
			  check_parameter_typelist ((yyvsp[-2].tvec));
			  (yyval.tvec) = (yyvsp[-2].tvec);
			}
#line 3596 "c-exp.c" /* yacc.c:1646  */
    break;

  case 204:
#line 1409 "c-exp.y" /* yacc.c:1646  */
    {
		  VEC (type_ptr) *typelist = NULL;
		  VEC_safe_push (type_ptr, typelist, (yyvsp[0].tval));
		  (yyval.tvec) = typelist;
		}
#line 3606 "c-exp.c" /* yacc.c:1646  */
    break;

  case 205:
#line 1415 "c-exp.y" /* yacc.c:1646  */
    {
		  VEC_safe_push (type_ptr, (yyvsp[-2].tvec), (yyvsp[0].tval));
		  (yyval.tvec) = (yyvsp[-2].tvec);
		}
#line 3615 "c-exp.c" /* yacc.c:1646  */
    break;

  case 207:
#line 1423 "c-exp.y" /* yacc.c:1646  */
    {
		  push_type_stack ((yyvsp[0].type_stack));
		  (yyval.tval) = follow_types ((yyvsp[-1].tval));
		}
#line 3624 "c-exp.c" /* yacc.c:1646  */
    break;

  case 208:
#line 1430 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = follow_types ((yyvsp[-1].tval)); }
#line 3630 "c-exp.c" /* yacc.c:1646  */
    break;

  case 213:
#line 1442 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_const);
			  insert_type (tp_volatile);
			}
#line 3638 "c-exp.c" /* yacc.c:1646  */
    break;

  case 214:
#line 1446 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_const); }
#line 3644 "c-exp.c" /* yacc.c:1646  */
    break;

  case 215:
#line 1448 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_volatile); }
#line 3650 "c-exp.c" /* yacc.c:1646  */
    break;

  case 216:
#line 1452 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" new"); }
#line 3656 "c-exp.c" /* yacc.c:1646  */
    break;

  case 217:
#line 1454 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" delete"); }
#line 3662 "c-exp.c" /* yacc.c:1646  */
    break;

  case 218:
#line 1456 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" new[]"); }
#line 3668 "c-exp.c" /* yacc.c:1646  */
    break;

  case 219:
#line 1458 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" delete[]"); }
#line 3674 "c-exp.c" /* yacc.c:1646  */
    break;

  case 220:
#line 1460 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" new[]"); }
#line 3680 "c-exp.c" /* yacc.c:1646  */
    break;

  case 221:
#line 1462 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" delete[]"); }
#line 3686 "c-exp.c" /* yacc.c:1646  */
    break;

  case 222:
#line 1464 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("+"); }
#line 3692 "c-exp.c" /* yacc.c:1646  */
    break;

  case 223:
#line 1466 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("-"); }
#line 3698 "c-exp.c" /* yacc.c:1646  */
    break;

  case 224:
#line 1468 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("*"); }
#line 3704 "c-exp.c" /* yacc.c:1646  */
    break;

  case 225:
#line 1470 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("/"); }
#line 3710 "c-exp.c" /* yacc.c:1646  */
    break;

  case 226:
#line 1472 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("%"); }
#line 3716 "c-exp.c" /* yacc.c:1646  */
    break;

  case 227:
#line 1474 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("^"); }
#line 3722 "c-exp.c" /* yacc.c:1646  */
    break;

  case 228:
#line 1476 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("&"); }
#line 3728 "c-exp.c" /* yacc.c:1646  */
    break;

  case 229:
#line 1478 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("|"); }
#line 3734 "c-exp.c" /* yacc.c:1646  */
    break;

  case 230:
#line 1480 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("~"); }
#line 3740 "c-exp.c" /* yacc.c:1646  */
    break;

  case 231:
#line 1482 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("!"); }
#line 3746 "c-exp.c" /* yacc.c:1646  */
    break;

  case 232:
#line 1484 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("="); }
#line 3752 "c-exp.c" /* yacc.c:1646  */
    break;

  case 233:
#line 1486 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("<"); }
#line 3758 "c-exp.c" /* yacc.c:1646  */
    break;

  case 234:
#line 1488 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (">"); }
#line 3764 "c-exp.c" /* yacc.c:1646  */
    break;

  case 235:
#line 1490 "c-exp.y" /* yacc.c:1646  */
    { const char *op = "unknown";
			  switch ((yyvsp[0].opcode))
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
#line 3808 "c-exp.c" /* yacc.c:1646  */
    break;

  case 236:
#line 1530 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("<<"); }
#line 3814 "c-exp.c" /* yacc.c:1646  */
    break;

  case 237:
#line 1532 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (">>"); }
#line 3820 "c-exp.c" /* yacc.c:1646  */
    break;

  case 238:
#line 1534 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("=="); }
#line 3826 "c-exp.c" /* yacc.c:1646  */
    break;

  case 239:
#line 1536 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("!="); }
#line 3832 "c-exp.c" /* yacc.c:1646  */
    break;

  case 240:
#line 1538 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("<="); }
#line 3838 "c-exp.c" /* yacc.c:1646  */
    break;

  case 241:
#line 1540 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (">="); }
#line 3844 "c-exp.c" /* yacc.c:1646  */
    break;

  case 242:
#line 1542 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("&&"); }
#line 3850 "c-exp.c" /* yacc.c:1646  */
    break;

  case 243:
#line 1544 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("||"); }
#line 3856 "c-exp.c" /* yacc.c:1646  */
    break;

  case 244:
#line 1546 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("++"); }
#line 3862 "c-exp.c" /* yacc.c:1646  */
    break;

  case 245:
#line 1548 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("--"); }
#line 3868 "c-exp.c" /* yacc.c:1646  */
    break;

  case 246:
#line 1550 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (","); }
#line 3874 "c-exp.c" /* yacc.c:1646  */
    break;

  case 247:
#line 1552 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("->*"); }
#line 3880 "c-exp.c" /* yacc.c:1646  */
    break;

  case 248:
#line 1554 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("->"); }
#line 3886 "c-exp.c" /* yacc.c:1646  */
    break;

  case 249:
#line 1556 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("()"); }
#line 3892 "c-exp.c" /* yacc.c:1646  */
    break;

  case 250:
#line 1558 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("[]"); }
#line 3898 "c-exp.c" /* yacc.c:1646  */
    break;

  case 251:
#line 1560 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("[]"); }
#line 3904 "c-exp.c" /* yacc.c:1646  */
    break;

  case 252:
#line 1562 "c-exp.y" /* yacc.c:1646  */
    { string_file buf;

			  c_print_type ((yyvsp[0].tval), NULL, &buf, -1, 0,
					&type_print_raw_options);
			  (yyval.sval) = operator_stoken (buf.c_str ());
			}
#line 3915 "c-exp.c" /* yacc.c:1646  */
    break;

  case 253:
#line 1572 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 3921 "c-exp.c" /* yacc.c:1646  */
    break;

  case 254:
#line 1573 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 3927 "c-exp.c" /* yacc.c:1646  */
    break;

  case 255:
#line 1574 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].tsym).stoken; }
#line 3933 "c-exp.c" /* yacc.c:1646  */
    break;

  case 256:
#line 1575 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 3939 "c-exp.c" /* yacc.c:1646  */
    break;

  case 257:
#line 1576 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 3945 "c-exp.c" /* yacc.c:1646  */
    break;

  case 258:
#line 1577 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].sval); }
#line 3951 "c-exp.c" /* yacc.c:1646  */
    break;

  case 261:
#line 1590 "c-exp.y" /* yacc.c:1646  */
    {
			  struct field_of_this_result is_a_field_of_this;

			  (yyval.ssym).stoken = (yyvsp[0].sval);
			  (yyval.ssym).sym = lookup_symbol ((yyvsp[0].sval).ptr,
						  expression_context_block,
						  VAR_DOMAIN,
						  &is_a_field_of_this);
			  (yyval.ssym).is_a_field_of_this
			    = is_a_field_of_this.type != NULL;
			}
#line 3967 "c-exp.c" /* yacc.c:1646  */
    break;


#line 3971 "c-exp.c" /* yacc.c:1646  */
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
#line 1604 "c-exp.y" /* yacc.c:1906  */


/* Like write_exp_string, but prepends a '~'.  */

static void
write_destructor_name (struct parser_state *par_state, struct stoken token)
{
  char *copy = (char *) alloca (token.length + 1);

  copy[0] = '~';
  memcpy (&copy[1], token.ptr, token.length);

  token.ptr = copy;
  ++token.length;

  write_exp_string (par_state, token);
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
  buf = (char *) xmalloc (st.length + 1);
  strcpy (buf, operator_string);
  strcat (buf, op);
  st.ptr = buf;

  /* The toplevel (c_parse) will free the memory allocated here.  */
  make_cleanup (xfree, buf);
  return st;
};

/* Return true if the type is aggregate-like.  */

static int
type_aggregate_p (struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_STRUCT
	  || TYPE_CODE (type) == TYPE_CODE_UNION
	  || TYPE_CODE (type) == TYPE_CODE_NAMESPACE
	  || (TYPE_CODE (type) == TYPE_CODE_ENUM
	      && TYPE_DECLARED_CLASS (type)));
}

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
parse_number (struct parser_state *par_state,
	      const char *buf, int len, int parsed_float, YYSTYPE *putithere)
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

  p = (char *) alloca (len);
  memcpy (p, buf, len);

  if (parsed_float)
    {
      /* If it ends at "df", "dd" or "dl", take it as type of decimal floating
         point.  Return DECFLOAT.  */

      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'f')
	{
	  p[len - 2] = '\0';
	  putithere->typed_val_decfloat.type
	    = parse_type (par_state)->builtin_decfloat;
	  decimal_from_string (putithere->typed_val_decfloat.val, 4,
			       gdbarch_byte_order (parse_gdbarch (par_state)),
			       p);
	  p[len - 2] = 'd';
	  return DECFLOAT;
	}

      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'd')
	{
	  p[len - 2] = '\0';
	  putithere->typed_val_decfloat.type
	    = parse_type (par_state)->builtin_decdouble;
	  decimal_from_string (putithere->typed_val_decfloat.val, 8,
			       gdbarch_byte_order (parse_gdbarch (par_state)),
			       p);
	  p[len - 2] = 'd';
	  return DECFLOAT;
	}

      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'l')
	{
	  p[len - 2] = '\0';
	  putithere->typed_val_decfloat.type
	    = parse_type (par_state)->builtin_declong;
	  decimal_from_string (putithere->typed_val_decfloat.val, 16,
			       gdbarch_byte_order (parse_gdbarch (par_state)),
			       p);
	  p[len - 2] = 'd';
	  return DECFLOAT;
	}

      if (! parse_c_float (parse_gdbarch (par_state), p, len,
			   &putithere->typed_val_float.dval,
			   &putithere->typed_val_float.type))
	return ERROR;
      return FLOAT;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0 */
  if (p[0] == '0' && len > 1)
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
  c_string_type type;
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
  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '"' ? (is_objc ? NSSTRING : STRING) : CHAR;
}

/* This is used to associate some attributes with a token.  */

enum token_flag
{
  /* If this bit is set, the token is C++-only.  */

  FLAG_CXX = 1,

  /* If this bit is set, the token is conditional: if there is a
     symbol of the same name, then the token is a symbol; otherwise,
     the token is a keyword.  */

  FLAG_SHADOW = 2
};
DEF_ENUM_FLAGS_TYPE (enum token_flag, token_flags);

struct token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
  token_flags flags;
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
  copy = (char *) obstack_copy0 (&expansion_obstack, expansion,
				 strlen (expansion));
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
lex_one_token (struct parser_state *par_state, int *is_quoted_name)
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
    if (strncmp (tokstart, tokentab3[i].oper, 3) == 0)
      {
	if ((tokentab3[i].flags & FLAG_CXX) != 0
	    && parse_language (par_state)->la_language != language_cplus)
	  break;

	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof tokentab2 / sizeof tokentab2[0]; i++)
    if (strncmp (tokstart, tokentab2[i].oper, 2) == 0)
      {
	if ((tokentab2[i].flags & FLAG_CXX) != 0
	    && parse_language (par_state)->la_language != language_cplus)
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
      if (parse_language (par_state)->la_language == language_objc
	  && c == '[')
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

	if (parse_language (par_state)->la_language == language_objc)
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
    if (strcmp (copy, ident_tokens[i].oper) == 0)
      {
	if ((ident_tokens[i].flags & FLAG_CXX) != 0
	    && parse_language (par_state)->la_language != language_cplus)
	  break;

	if ((ident_tokens[i].flags & FLAG_SHADOW) != 0)
	  {
	    struct field_of_this_result is_a_field_of_this;

	    if (lookup_symbol (copy, expression_context_block,
			       VAR_DOMAIN,
			       (parse_language (par_state)->la_language
			        == language_cplus ? &is_a_field_of_this
				: NULL)).symbol
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
  yylval.ssym.sym.symbol = NULL;
  yylval.ssym.sym.block = NULL;
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
classify_name (struct parser_state *par_state, const struct block *block,
	       int is_quoted_name)
{
  struct block_symbol bsym;
  char *copy;
  struct field_of_this_result is_a_field_of_this;

  copy = copy_name (yylval.sval);

  /* Initialize this in case we *don't* use it in this call; that way
     we can refer to it unconditionally below.  */
  memset (&is_a_field_of_this, 0, sizeof (is_a_field_of_this));

  bsym = lookup_symbol (copy, block, VAR_DOMAIN,
			parse_language (par_state)->la_name_of_this
			? &is_a_field_of_this : NULL);

  if (bsym.symbol && SYMBOL_CLASS (bsym.symbol) == LOC_BLOCK)
    {
      yylval.ssym.sym = bsym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
      return BLOCKNAME;
    }
  else if (!bsym.symbol)
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

	  bsym = lookup_symbol (copy, block, STRUCT_DOMAIN,
				&inner_is_a_field_of_this);
	  if (bsym.symbol != NULL)
	    {
	      yylval.tsym.type = SYMBOL_TYPE (bsym.symbol);
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
	      yylval.bval = BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (symtab),
					       STATIC_BLOCK);
	      return FILENAME;
	    }
	}
    }

  if (bsym.symbol && SYMBOL_CLASS (bsym.symbol) == LOC_TYPEDEF)
    {
      yylval.tsym.type = SYMBOL_TYPE (bsym.symbol);
      return TYPENAME;
    }

  /* See if it's an ObjC classname.  */
  if (parse_language (par_state)->la_language == language_objc && !bsym.symbol)
    {
      CORE_ADDR Class = lookup_objc_class (parse_gdbarch (par_state), copy);
      if (Class)
	{
	  struct symbol *sym;

	  yylval.theclass.theclass = Class;
	  sym = lookup_struct_typedef (copy, expression_context_block, 1);
	  if (sym)
	    yylval.theclass.type = SYMBOL_TYPE (sym);
	  return CLASSNAME;
	}
    }

  /* Input names that aren't symbols but ARE valid hex numbers, when
     the input radix permits them, can be names or numbers depending
     on the parse.  Note we support radixes > 16 here.  */
  if (!bsym.symbol
      && ((copy[0] >= 'a' && copy[0] < 'a' + input_radix - 10)
	  || (copy[0] >= 'A' && copy[0] < 'A' + input_radix - 10)))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, copy, yylval.sval.length,
				  0, &newlval);

      if (hextype == INT)
	{
	  yylval.ssym.sym = bsym;
	  yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	  return NAME_OR_INT;
	}
    }

  /* Any other kind of symbol */
  yylval.ssym.sym = bsym;
  yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;

  if (bsym.symbol == NULL
      && parse_language (par_state)->la_language == language_cplus
      && is_a_field_of_this.type == NULL
      && lookup_minimal_symbol (copy, NULL, NULL).minsym == NULL)
    return UNKNOWN_CPP_NAME;

  return NAME;
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
    return classify_name (par_state, block, 0);

  type = check_typedef (context);
  if (!type_aggregate_p (type))
    return ERROR;

  copy = copy_name (yylval.ssym.stoken);
  /* N.B. We assume the symbol can only be in VAR_DOMAIN.  */
  yylval.ssym.sym = cp_lookup_nested_symbol (type, copy, block, VAR_DOMAIN);

  /* If no symbol was found, search for a matching base class named
     COPY.  This will allow users to enter qualified names of class members
     relative to the `this' pointer.  */
  if (yylval.ssym.sym.symbol == NULL)
    {
      struct type *base_type = cp_find_type_baseclass_by_name (type, copy);

      if (base_type != NULL)
	{
	  yylval.tsym.type = base_type;
	  return TYPENAME;
	}

      return ERROR;
    }

  switch (SYMBOL_CLASS (yylval.ssym.sym.symbol))
    {
    case LOC_BLOCK:
    case LOC_LABEL:
      /* cp_lookup_nested_symbol might have accidentally found a constructor
	 named COPY when we really wanted a base class of the same name.
	 Double-check this case by looking for a base class.  */
      {
	struct type *base_type = cp_find_type_baseclass_by_name (type, copy);

	if (base_type != NULL)
	  {
	    yylval.tsym.type = base_type;
	    return TYPENAME;
	  }
      }
      return ERROR;

    case LOC_TYPEDEF:
      yylval.tsym.type = SYMBOL_TYPE (yylval.ssym.sym.symbol);
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
  current.token = lex_one_token (pstate, &is_quoted_name);
  if (current.token == NAME)
    current.token = classify_name (pstate, expression_context_block,
				   is_quoted_name);
  if (parse_language (pstate)->la_language != language_cplus
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
      current.token = lex_one_token (pstate, &ignore);
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
	  classification = classify_inner_name (pstate, search_block,
						context_type);
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

	  yylval.sval.ptr = (const char *) obstack_base (&name_obstack);
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
      current.value.sval.ptr
	= (const char *) obstack_copy0 (&expansion_obstack,
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
c_parse (struct parser_state *par_state)
{
  int result;
  struct cleanup *back_to;

  /* Setting up the parser state.  */
  gdb_assert (par_state != NULL);
  pstate = par_state;

  back_to = make_cleanup (free_current_contents, &expression_macro_scope);
  make_cleanup_clear_parser_state (&pstate);

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

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);

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

#ifdef YYBISON

/* This is called via the YYPRINT macro when parser debugging is
   enabled.  It prints a token's value.  */

static void
c_print_token (FILE *file, int type, YYSTYPE value)
{
  switch (type)
    {
    case INT:
      parser_fprintf (file, "typed_val_int<%s, %s>",
		      TYPE_SAFE_NAME (value.typed_val_int.type),
		      pulongest (value.typed_val_int.val));
      break;

    case CHAR:
    case STRING:
      {
	char *copy = (char *) alloca (value.tsval.length + 1);

	memcpy (copy, value.tsval.ptr, value.tsval.length);
	copy[value.tsval.length] = '\0';

	parser_fprintf (file, "tsval<type=%d, %s>", value.tsval.type, copy);
      }
      break;

    case NSSTRING:
    case VARIABLE:
      parser_fprintf (file, "sval<%s>", copy_name (value.sval));
      break;

    case TYPENAME:
      parser_fprintf (file, "tsym<type=%s, name=%s>",
		      TYPE_SAFE_NAME (value.tsym.type),
		      copy_name (value.tsym.stoken));
      break;

    case NAME:
    case UNKNOWN_CPP_NAME:
    case NAME_OR_INT:
    case BLOCKNAME:
      parser_fprintf (file, "ssym<name=%s, sym=%s, field_of_this=%d>",
		       copy_name (value.ssym.stoken),
		       (value.ssym.sym.symbol == NULL
			? "(null)" : SYMBOL_PRINT_NAME (value.ssym.sym.symbol)),
		       value.ssym.is_a_field_of_this);
      break;

    case FILENAME:
      parser_fprintf (file, "bval<%s>", host_address_to_string (value.bval));
      break;
    }
}

#endif

void
yyerror (const char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), (msg ? msg : "error"), lexptr);
}
