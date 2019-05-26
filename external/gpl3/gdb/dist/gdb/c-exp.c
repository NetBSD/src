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
#include "c-support.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"
#include "cp-support.h"
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

/* Data that must be held for the duration of a parse.  */

struct c_parse_state
{
  /* These are used to hold type lists and type stacks that are
     allocated during the parse.  */
  std::vector<std::unique_ptr<std::vector<struct type *>>> type_lists;
  std::vector<std::unique_ptr<struct type_stack>> type_stacks;

  /* Storage for some strings allocated during the parse.  */
  std::vector<gdb::unique_xmalloc_ptr<char>> strings;

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
  const char *macro_original_text = nullptr;

  /* We save all intermediate macro expansions on this obstack for the
     duration of a single parse.  The expansion text may sometimes have
     to live past the end of the expansion, due to yacc lookahead.
     Rather than try to be clever about saving the data for a single
     token, we simply keep it all and delete it after parsing has
     completed.  */
  auto_obstack expansion_obstack;
};

/* This is set and cleared in c_parse.  */

static struct c_parse_state *cpstate;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static int type_aggregate_p (struct type *);


#line 153 "c-exp.c.tmp" /* yacc.c:339  */

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
    NSSTRING = 261,
    SELECTOR = 262,
    CHAR = 263,
    NAME = 264,
    UNKNOWN_CPP_NAME = 265,
    COMPLETE = 266,
    TYPENAME = 267,
    CLASSNAME = 268,
    OBJC_LBRAC = 269,
    NAME_OR_INT = 270,
    OPERATOR = 271,
    STRUCT = 272,
    CLASS = 273,
    UNION = 274,
    ENUM = 275,
    SIZEOF = 276,
    ALIGNOF = 277,
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
    DOLLAR_VARIABLE = 299,
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
#define STRING 260
#define NSSTRING 261
#define SELECTOR 262
#define CHAR 263
#define NAME 264
#define UNKNOWN_CPP_NAME 265
#define COMPLETE 266
#define TYPENAME 267
#define CLASSNAME 268
#define OBJC_LBRAC 269
#define NAME_OR_INT 270
#define OPERATOR 271
#define STRUCT 272
#define CLASS 273
#define UNION 274
#define ENUM 275
#define SIZEOF 276
#define ALIGNOF 277
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
#define DOLLAR_VARIABLE 299
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
#line 128 "c-exp.y" /* yacc.c:355  */

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    struct typed_stoken tsval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    const struct block *bval;
    enum exp_opcode opcode;

    struct stoken_vector svec;
    std::vector<struct type *> *tvec;

    struct type_stack *type_stack;

    struct objc_class_str theclass;
  

#line 348 "c-exp.c.tmp" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */
#line 155 "c-exp.y" /* yacc.c:358  */

/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *par_state,
			 const char *, int, int, YYSTYPE *);
static struct stoken operator_stoken (const char *);
static struct stoken typename_stoken (const char *);
static void check_parameter_typelist (std::vector<struct type *> *);
static void write_destructor_name (struct parser_state *par_state,
				   struct stoken);

#ifdef YYBISON
static void c_print_token (FILE *file, int type, YYSTYPE value);
#define YYPRINT(FILE, TYPE, VALUE) c_print_token (FILE, TYPE, VALUE)
#endif

#line 380 "c-exp.c.tmp" /* yacc.c:358  */

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
#define YYFINAL  171
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1732

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  90
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  50
/* YYNRULES -- Number of rules.  */
#define YYNRULES  275
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  432

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
       0,   273,   273,   274,   277,   281,   285,   291,   298,   299,
     304,   308,   312,   316,   320,   324,   328,   332,   336,   340,
     344,   348,   352,   356,   360,   366,   373,   383,   389,   396,
     404,   408,   414,   421,   431,   437,   444,   452,   456,   460,
     470,   469,   493,   492,   509,   508,   517,   519,   522,   523,
     526,   528,   530,   537,   534,   548,   558,   557,   583,   587,
     590,   594,   598,   617,   628,   635,   636,   639,   647,   650,
     657,   661,   665,   671,   675,   679,   683,   687,   691,   695,
     699,   703,   707,   711,   715,   719,   723,   727,   731,   735,
     739,   743,   747,   751,   755,   762,   769,   778,   791,   798,
     801,   807,   814,   834,   839,   843,   847,   854,   871,   889,
     922,   931,   939,   949,   957,   963,   974,   989,  1007,  1020,
    1044,  1053,  1054,  1082,  1160,  1164,  1165,  1168,  1171,  1172,
    1176,  1177,  1182,  1181,  1185,  1184,  1187,  1189,  1191,  1193,
    1197,  1204,  1206,  1207,  1210,  1212,  1220,  1228,  1235,  1243,
    1245,  1247,  1249,  1253,  1258,  1270,  1284,  1286,  1290,  1294,
    1298,  1302,  1306,  1310,  1314,  1318,  1322,  1326,  1330,  1334,
    1338,  1342,  1346,  1350,  1354,  1358,  1362,  1366,  1370,  1374,
    1378,  1382,  1386,  1390,  1396,  1402,  1405,  1410,  1416,  1419,
    1424,  1430,  1433,  1438,  1444,  1447,  1452,  1458,  1462,  1466,
    1470,  1477,  1481,  1483,  1487,  1488,  1496,  1504,  1515,  1517,
    1526,  1535,  1542,  1543,  1550,  1554,  1555,  1558,  1559,  1562,
    1566,  1568,  1572,  1574,  1576,  1578,  1580,  1582,  1584,  1586,
    1588,  1590,  1592,  1594,  1596,  1598,  1600,  1602,  1604,  1606,
    1608,  1610,  1650,  1652,  1654,  1656,  1658,  1660,  1662,  1664,
    1666,  1668,  1670,  1672,  1674,  1676,  1678,  1680,  1682,  1705,
    1706,  1707,  1708,  1709,  1710,  1711,  1714,  1715,  1716,  1717,
    1718,  1719,  1722,  1723,  1731,  1743
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "FLOAT", "STRING", "NSSTRING",
  "SELECTOR", "CHAR", "NAME", "UNKNOWN_CPP_NAME", "COMPLETE", "TYPENAME",
  "CLASSNAME", "OBJC_LBRAC", "NAME_OR_INT", "OPERATOR", "STRUCT", "CLASS",
  "UNION", "ENUM", "SIZEOF", "ALIGNOF", "UNSIGNED", "COLONCOLON",
  "TEMPLATE", "ERROR", "NEW", "DELETE", "REINTERPRET_CAST", "DYNAMIC_CAST",
  "STATIC_CAST", "CONST_CAST", "ENTRY", "TYPEOF", "DECLTYPE", "TYPEID",
  "SIGNED_KEYWORD", "LONG", "SHORT", "INT_KEYWORD", "CONST_KEYWORD",
  "VOLATILE_KEYWORD", "DOUBLE_KEYWORD", "DOLLAR_VARIABLE", "ASSIGN_MODIFY",
  "TRUEKEYWORD", "FALSEKEYWORD", "','", "ABOVE_COMMA", "'='", "'?'",
  "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQUAL", "NOTEQUAL", "'<'", "'>'",
  "LEQ", "GEQ", "LSH", "RSH", "'@'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "UNARY", "INCREMENT", "DECREMENT", "ARROW", "ARROW_STAR", "'.'",
  "DOT_STAR", "'['", "'('", "BLOCKNAME", "FILENAME", "DOTDOTDOT", "')'",
  "'!'", "'~'", "']'", "':'", "'{'", "'}'", "$accept", "start", "type_exp",
  "exp1", "exp", "$@1", "$@2", "$@3", "msglist", "msgarglist", "msgarg",
  "$@4", "$@5", "lcurly", "arglist", "function_method",
  "function_method_void", "function_method_void_or_typelist", "rcurly",
  "string_exp", "block", "variable", "qualified_name", "space_identifier",
  "const_or_volatile", "cv_with_space_id",
  "const_or_volatile_or_space_identifier_noopt",
  "const_or_volatile_or_space_identifier", "ptr_operator", "$@6", "$@7",
  "ptr_operator_ts", "abs_decl", "direct_abs_decl", "array_mod",
  "func_mod", "type", "typebase", "type_name", "parameter_typelist",
  "nonempty_typelist", "ptype", "conversion_type_id",
  "conversion_declarator", "const_and_volatile", "const_or_volatile_noopt",
  "oper", "field_name", "name", "name_not_typename", YY_NULLPTRPTR
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

#define YYPACT_NINF -226

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-226)))

#define YYTABLE_NINF -127

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     488,  -226,  -226,  -226,  -226,   -64,  -226,  -226,   -54,     4,
     660,  -226,   907,   139,   243,   254,   271,   746,   -27,   182,
      27,    81,    -1,    15,    26,    35,    -8,    24,    34,   263,
     288,   202,  -226,    78,    89,  -226,  -226,  -226,  -226,   832,
     832,   832,   832,   832,   832,   488,   114,  -226,   832,   832,
    -226,   145,  -226,   105,  1308,   402,   172,  -226,   176,   158,
     177,  -226,  -226,   141,  -226,  1689,  -226,   146,   282,  -226,
     151,  -226,   184,    81,  -226,    47,     4,  -226,  1308,  -226,
     137,    21,    30,  -226,  -226,  -226,  -226,  -226,  -226,  -226,
    -226,  -226,  -226,  -226,  -226,  -226,  -226,  -226,  -226,  -226,
    -226,  -226,  -226,  -226,  -226,  -226,  -226,  -226,   143,   148,
    -226,  -226,   276,  -226,  -226,  -226,  -226,  -226,  -226,  -226,
    -226,   222,  -226,   225,  -226,   227,  -226,   229,     4,   488,
     216,  1657,  -226,    11,   201,  -226,  -226,  -226,  -226,  -226,
     189,  1657,  1657,  1657,  1657,   574,   832,   488,    86,  -226,
    -226,   211,   220,   239,  -226,  -226,   238,   244,  -226,  -226,
    -226,   216,   216,   216,   216,   216,   216,   173,    -7,   216,
     216,  -226,   832,   832,   832,   832,   832,   832,   832,   832,
     832,   832,   832,   832,   832,   832,   832,   832,   832,   832,
     832,   832,   832,   832,   832,   832,  -226,  -226,    72,   832,
     234,   832,   832,  1024,   178,  1308,    12,    81,  -226,    81,
     289,   118,   146,  -226,     3,    48,   231,  -226,    14,  1015,
     236,    50,  -226,    55,  -226,  -226,  -226,   214,   832,    81,
     281,    38,    38,    38,  -226,   223,   235,   241,   250,  -226,
    -226,   179,  -226,  -226,  -226,  -226,  -226,   232,   237,   268,
    -226,  -226,  1689,   262,   264,   270,   277,  1063,   259,  1098,
     265,  1133,   305,  -226,  -226,  -226,   306,   309,  -226,  -226,
    -226,   832,  -226,  1308,   -25,  1308,  1308,   944,  1374,  1406,
    1431,  1463,  1488,  1520,  1520,  1545,  1545,  1545,  1545,  1560,
    1560,  1577,  1589,  1589,   216,   216,   216,  -226,     4,  -226,
    -226,  -226,  -226,  -226,  -226,    81,  -226,   341,  -226,    65,
    -226,    81,  -226,   343,    65,   -21,    31,   832,  -226,   272,
     308,  -226,   832,   832,  -226,  -226,   334,  -226,  -226,  -226,
     283,  -226,   236,   236,   146,   287,  -226,  -226,   279,   280,
    -226,    55,   347,  -226,  -226,  -226,    -6,  -226,    81,   832,
     832,   290,    38,  -226,   291,   293,   294,  -226,  -226,  -226,
    -226,  -226,  -226,  -226,  -226,  -226,   314,   296,   298,   302,
     303,  -226,  -226,  -226,  -226,  -226,  -226,  -226,  -226,   216,
    -226,   832,   357,  -226,   372,  -226,  -226,  -226,    19,   118,
    1062,   216,  1308,  -226,  -226,  -226,  -226,  -226,  -226,   146,
    -226,  -226,  1308,  1308,  -226,  -226,   291,   832,  -226,  -226,
    -226,   832,   832,   832,   832,  1340,  -226,  -226,  -226,  -226,
    -226,  -226,  -226,  1308,  1168,  1203,  1238,  1273,  -226,  -226,
    -226,  -226
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
     126,    95,    98,   107,   110,     0,    96,   272,   275,   156,
       0,    97,   126,     0,     0,     0,     0,     0,     0,   198,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   200,
     158,   159,   157,   220,   221,   183,   100,   111,   112,     0,
       0,     0,     0,     0,     0,   126,   273,   114,     0,     0,
      58,     0,     3,     2,     8,    59,    64,    66,     0,   109,
       0,    99,   121,     0,   128,   126,     4,   212,   155,   219,
     129,   274,   123,     0,    56,     0,    40,    42,    44,   156,
       0,   222,   223,   241,   252,   238,   249,   248,   235,   233,
     234,   244,   245,   239,   240,   246,   247,   242,   243,   228,
     229,   230,   231,   232,   250,   251,   254,   253,     0,     0,
     237,   236,   215,   258,   266,   270,   186,   268,   269,   267,
     271,   185,   189,   188,   192,   191,   195,   194,     0,   126,
      22,   126,   204,   206,   207,   205,   197,   275,   273,   122,
       0,   126,   126,   126,   126,   126,     0,   126,   206,   207,
     199,   166,   162,   167,   160,   184,   181,   179,   177,   217,
     218,    11,    13,    12,    10,    16,    17,     0,     0,    14,
      15,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    18,    19,     0,     0,
       0,     0,     0,    53,     0,    60,     0,     0,   108,     0,
       0,   126,   202,   203,     0,   138,   136,   134,     0,   126,
     140,   142,   213,   143,   146,   148,   116,     0,    59,     0,
     118,     0,     0,     0,   257,     0,     0,     0,     0,   256,
     255,   215,   214,   187,   190,   193,   196,     0,     0,   173,
     164,   180,   126,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   171,   163,   165,   161,   175,   170,   168,   182,
     178,     0,    72,     9,     0,    94,    93,     0,    91,    90,
      89,    88,    87,    81,    82,    85,    86,    83,    84,    79,
      80,    73,    77,    78,    74,    75,    76,    26,   268,   265,
     264,   262,   263,   261,   260,     0,    29,    24,   259,    30,
      33,     0,    36,    31,    37,     0,    55,    59,   210,     0,
     208,    68,     0,     0,    69,    67,   117,   124,   127,   125,
       0,   150,   139,   137,   131,     0,   149,   153,     0,     0,
     132,   141,   126,   145,   147,   101,     0,   119,     0,     0,
       0,     0,    47,    48,    46,     0,     0,   226,   224,   227,
     225,   132,   216,   102,    23,   174,     0,     0,     0,     0,
       0,     5,     6,     7,    21,    20,   172,   176,   169,    71,
      39,     0,    27,    25,    34,    32,    38,    63,     0,   126,
     126,    70,    61,   152,   130,   135,   151,   144,   154,   131,
      57,   120,    52,    51,    41,    49,     0,     0,    43,    45,
     201,     0,     0,     0,     0,    92,    28,    35,    54,    62,
     209,   211,   133,    50,     0,     0,     0,     0,   103,   105,
     104,   106
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -226,  -226,    66,    13,   -10,  -226,  -226,  -226,   -34,  -226,
      39,  -226,  -226,  -226,  -225,  -226,  -226,  -226,   186,  -226,
    -226,  -226,   -42,  -226,  -199,  -226,   -66,    -4,  -110,  -226,
    -226,  -226,   174,   175,  -219,  -218,  -105,    10,   365,   195,
    -226,  -226,  -226,   159,  -226,  -197,    -5,   199,     5,   393
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    51,   167,   168,    54,   231,   232,   233,   351,   352,
     353,   317,   228,    55,   206,    56,    57,    58,   322,    59,
      60,    61,    62,   211,    63,    64,    65,   395,   220,   399,
     334,   221,   222,   223,   224,   225,    66,    67,   136,   339,
     320,    68,   113,   242,    69,    70,    71,   307,   354,    72
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      78,   213,   241,   346,   343,   344,   330,   130,   120,   120,
     120,   120,   328,    53,   329,    73,   120,   335,   121,   123,
     125,   127,   112,   172,   247,    74,   140,   172,    75,   161,
     162,   163,   164,   165,   166,   235,     7,   137,   169,   170,
     258,   172,   323,    12,   237,   205,   213,   114,   115,   249,
     117,   250,   131,   118,    12,  -126,   114,   115,   141,   117,
     323,   380,   118,    12,   214,   386,    52,   323,   120,   214,
     120,   145,    33,    34,   142,   212,   272,   400,   227,   173,
     230,   114,   115,   297,   298,   143,   349,   118,    12,   331,
     114,   115,   388,   117,   144,   299,   118,    12,   318,   236,
     336,   321,   418,   146,   216,   332,   333,   138,   238,   300,
     301,   302,   303,   147,   318,   304,   217,   387,   119,   329,
     159,   204,   343,   344,   262,   350,   263,   119,   218,   219,
     160,   241,   229,   218,   342,   257,   259,   261,  -113,   198,
     199,   200,   201,   202,   203,   171,   213,   366,   114,   115,
     116,   117,   119,   172,   118,    12,   306,   305,   312,    33,
      34,   119,   273,   208,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   274,    33,    34,   309,
     419,   314,   329,   120,   132,   120,   -65,   248,   355,   356,
     207,   209,   120,   308,   120,   308,   210,   253,   254,   255,
     256,  -126,   325,   260,   326,   315,  -125,   226,   205,   119,
     133,   134,   135,   234,   120,   156,   120,   120,   120,   239,
     173,   240,   215,   243,   347,   216,   244,   318,   245,   157,
     246,   251,   158,   114,   115,   310,   298,   361,   252,   118,
      12,   264,   114,   115,   122,   117,   271,   299,   118,    12,
     265,   379,   266,   114,   115,   124,   117,   321,   394,   118,
      12,   300,   301,   302,   303,   132,   267,   304,   269,   268,
     114,   115,   126,   117,   270,   421,   118,    12,   196,   197,
     198,   199,   200,   201,   202,   203,   214,   345,   327,   217,
     120,   148,   149,   135,   340,   348,   120,   205,   365,   357,
     382,   151,   391,   392,   119,   363,   384,    33,    34,   311,
     364,   358,   367,   119,   368,   152,   153,   359,   154,   215,
     369,   155,   216,   394,   119,   215,   360,   370,   216,   402,
     403,  -126,   372,   120,   217,   376,   377,   120,   374,   378,
     217,   119,   383,   401,   385,   389,   390,   406,  -115,    79,
     218,   219,   397,   398,    13,    14,    15,    16,   416,   393,
      19,   415,    21,   396,   410,   411,   404,   412,   407,   408,
     409,   413,   414,   417,    29,    30,    31,    32,    33,    34,
      35,   405,   324,   338,   150,   422,   341,   423,   319,   313,
     362,   424,   425,   426,   427,     1,     2,     3,     4,     5,
       6,     7,     8,   139,     9,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,     0,     0,
     337,    22,    23,    24,    25,     0,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,     0,    37,    38,
       0,     0,     0,     0,     0,     0,     0,     0,    39,     0,
       0,     0,     0,     0,     0,     0,     0,  -126,    40,    41,
      42,     0,     0,     0,    43,    44,     0,     0,     0,     0,
       0,    45,    46,    47,     0,     0,    48,    49,     0,     0,
      50,     1,     2,     3,     4,     5,     6,     7,     8,     0,
       9,     0,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,     0,     0,     0,    22,    23,    24,
      25,     0,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,     0,    37,    38,     0,     0,     0,     0,
       0,     0,     0,     0,    39,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    40,    41,    42,     0,     0,     0,
      43,    44,     0,     0,     0,     0,     0,    45,    46,    47,
       0,     0,    48,    49,     0,     0,    50,     1,     2,     3,
       4,     5,     6,     7,     8,     0,     9,     0,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
       0,     0,     0,    22,    23,    24,    25,     0,     0,     0,
      28,    29,    30,    31,    32,    33,    34,    35,    36,     0,
      37,    38,     0,     0,     0,     0,     0,     0,     0,     0,
      39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      40,    41,    42,     0,     0,     0,    43,    44,     0,     0,
       0,     0,     0,    45,    46,    47,     0,     0,    48,    49,
       0,     0,    50,     1,     2,     3,     4,     5,     6,     7,
       8,     0,    76,    77,    10,    11,    12,     0,     0,     0,
       0,    17,    18,     0,    20,     0,     0,     0,     0,    22,
      23,    24,    25,     0,     0,     0,    28,     0,     0,     0,
       0,     0,     0,     0,    36,     0,    37,    38,     0,     0,
       0,     0,     0,     0,     0,     0,    39,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    40,    41,    42,     0,
       0,     0,    43,    44,     0,     0,     0,     0,     0,    45,
      46,    47,     0,     0,    48,    49,     0,     0,    50,     1,
       2,     3,     4,     5,     6,     7,     8,     0,   128,     0,
      10,    11,    12,     0,     0,     0,     0,    17,    18,     0,
      20,     0,     0,     0,     0,    22,    23,    24,    25,     0,
       0,     0,    28,     0,     0,     0,     0,     0,     0,     0,
      36,     0,    37,    38,     0,     0,     0,     0,     0,     0,
       0,     0,    39,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    40,    41,    42,     0,     0,     0,    43,    44,
       0,     0,     0,     0,     0,   129,    46,    47,     0,     0,
      48,    49,     0,     0,    50,     1,     2,     3,     4,     5,
       6,     7,     8,     0,   128,     0,    10,    11,    12,     0,
       0,     0,     0,    17,    18,     0,    20,     0,     0,     0,
       0,    22,    23,    24,    25,     0,     0,     0,    28,     0,
       0,     0,     0,     0,     0,     0,    36,     0,    37,    38,
       0,     0,     0,     0,     0,     0,     0,     0,    39,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    40,    41,
      42,     0,     0,     0,    43,    44,     0,     0,     0,     0,
       0,    45,    46,    47,     0,     0,    48,    49,     0,    79,
      50,    80,     0,     0,    13,    14,    15,    16,     0,     0,
      19,     0,    21,     0,    81,    82,     0,     0,     0,     0,
       0,     0,     0,     0,    29,    30,    31,    32,    33,    34,
      35,     0,    83,     0,     0,    84,     0,    85,   173,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,     0,    99,   100,   101,   102,   103,     0,   104,
     105,   106,   107,     0,     0,   108,   109,     0,     0,   174,
       0,   110,   111,     0,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,     0,   196,   197,   198,   199,
     200,   201,   202,   203,     0,     0,     0,    79,     0,   214,
       0,   381,    13,    14,    15,    16,    79,     0,    19,     0,
      21,    13,    14,    15,    16,     0,     0,    19,     0,    21,
       0,     0,    29,    30,    31,    32,    33,    34,    35,     0,
       0,    29,    30,    31,    32,    33,    34,    35,   215,     0,
       0,   216,     0,     0,    79,     0,     0,   173,     0,    13,
      14,    15,    16,   217,     0,    19,     0,    21,     0,  -126,
       0,     0,     0,   218,   219,     0,     0,     0,   337,    29,
      30,    31,    32,    33,    34,    35,     0,   316,   174,     0,
       0,     0,   173,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,     0,   196,   197,   198,   199,   200,
     201,   202,   203,   174,   420,     0,   371,   173,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,     0,
     196,   197,   198,   199,   200,   201,   202,   203,   174,     0,
       0,   373,   173,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,     0,   196,   197,   198,   199,   200,
     201,   202,   203,   174,     0,     0,   375,   173,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,     0,
     196,   197,   198,   199,   200,   201,   202,   203,   174,     0,
       0,   428,   173,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,     0,   196,   197,   198,   199,   200,
     201,   202,   203,   174,     0,     0,   429,   173,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,     0,
     196,   197,   198,   199,   200,   201,   202,   203,   174,     0,
       0,   430,   173,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,     0,   196,   197,   198,   199,   200,
     201,   202,   203,   174,   173,     0,   431,     0,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,     0,
     196,   197,   198,   199,   200,   201,   202,   203,   173,     0,
       0,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,     0,   196,   197,   198,   199,   200,   201,   202,   203,
     173,     0,     0,     0,     0,     0,     0,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   173,   196,   197,   198,   199,
     200,   201,   202,   203,     0,     0,     0,     0,     0,     0,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   173,   196,   197,
     198,   199,   200,   201,   202,   203,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   173,   196,   197,   198,   199,   200,   201,   202,
     203,     0,     0,     0,     0,     0,     0,     0,     0,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   173,   196,   197,   198,   199,   200,
     201,   202,   203,     0,     0,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   173,
     196,   197,   198,   199,   200,   201,   202,   203,     0,     0,
       0,     0,     0,     0,   173,     0,     0,     0,     0,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   173,   196,   197,   198,   199,   200,   201,   202,   203,
       0,     0,     0,   173,     0,     0,     0,     0,   188,   189,
     190,   191,   192,   193,   194,   195,     0,   196,   197,   198,
     199,   200,   201,   202,   203,   190,   191,   192,   193,   194,
     195,     0,   196,   197,   198,   199,   200,   201,   202,   203,
       0,     0,     0,   191,   192,   193,   194,   195,     0,   196,
     197,   198,   199,   200,   201,   202,   203,   193,   194,   195,
       0,   196,   197,   198,   199,   200,   201,   202,   203,    79,
       0,     0,     0,     0,    13,    14,    15,    16,     0,     0,
      19,     0,    21,     0,     0,     0,     0,     0,     0,     0,
       0,    26,    27,     0,    29,    30,    31,    32,    33,    34,
      35,    79,     0,     0,     0,     0,    13,    14,    15,    16,
       0,     0,    19,     0,    21,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    29,    30,    31,    32,
      33,    34,    35
};

static const yytype_int16 yycheck[] =
{
      10,    67,   112,   228,   223,   223,     3,    17,    13,    14,
      15,    16,   211,     0,   211,    79,    21,     3,    13,    14,
      15,    16,    12,    48,   129,    79,    21,    48,    24,    39,
      40,    41,    42,    43,    44,    14,     9,    10,    48,    49,
     145,    48,    48,    16,    14,    55,   112,     9,    10,    38,
      12,    40,    79,    15,    16,    24,     9,    10,    59,    12,
      48,    86,    15,    16,    14,    86,     0,    48,    73,    14,
      75,    79,    41,    42,    59,    65,    83,    83,    73,    14,
      75,     9,    10,    11,    12,    59,    48,    15,    16,    86,
       9,    10,   317,    12,    59,    23,    15,    16,   203,    78,
      86,    89,    83,    79,    56,   215,   216,    80,    78,    37,
      38,    39,    40,    79,   219,    43,    68,   316,    80,   316,
      42,    55,   341,   341,    38,    87,    40,    80,    78,    79,
      41,   241,    85,    78,    79,   145,   146,   147,    24,    74,
      75,    76,    77,    78,    79,     0,   212,   252,     9,    10,
      11,    12,    80,    48,    15,    16,   198,    85,   200,    41,
      42,    80,   172,     5,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   173,    41,    42,   199,
     389,   201,   389,   198,    12,   200,    24,   131,   232,   233,
      24,    24,   207,   198,   209,   200,    65,   141,   142,   143,
     144,    65,   207,   147,   209,   202,    65,    33,   228,    80,
      38,    39,    40,    86,   229,    23,   231,   232,   233,    86,
      14,    83,    53,    11,   229,    56,    11,   342,    11,    37,
      11,    40,    40,     9,    10,    11,    12,    68,    59,    15,
      16,    40,     9,    10,    11,    12,    83,    23,    15,    16,
      40,   271,    23,     9,    10,    11,    12,    89,   334,    15,
      16,    37,    38,    39,    40,    12,    37,    43,    40,    40,
       9,    10,    11,    12,    40,   390,    15,    16,    72,    73,
      74,    75,    76,    77,    78,    79,    14,    83,     9,    68,
     305,    38,    39,    40,    68,    24,   311,   317,    40,    86,
     305,    23,   322,   323,    80,    83,   311,    41,    42,    85,
      83,    86,    60,    80,    60,    37,    38,    86,    40,    53,
      60,    43,    56,   399,    80,    53,    86,    60,    56,   349,
     350,    65,    83,   348,    68,    40,    40,   352,    83,    40,
      68,    80,    11,   348,    11,    83,    48,   352,    24,    12,
      78,    79,    83,    83,    17,    18,    19,    20,    11,    86,
      23,   381,    25,    86,    60,    79,    86,    79,    87,    86,
      86,    79,    79,    11,    37,    38,    39,    40,    41,    42,
      43,   352,   206,   219,    29,   399,   221,   407,   203,   200,
     241,   411,   412,   413,   414,     3,     4,     5,     6,     7,
       8,     9,    10,    20,    12,    -1,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    -1,    -1,
      83,    29,    30,    31,    32,    -1,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    -1,    46,    47,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    66,    67,
      68,    -1,    -1,    -1,    72,    73,    -1,    -1,    -1,    -1,
      -1,    79,    80,    81,    -1,    -1,    84,    85,    -1,    -1,
      88,     3,     4,     5,     6,     7,     8,     9,    10,    -1,
      12,    -1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    -1,    -1,    -1,    29,    30,    31,
      32,    -1,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    -1,    -1,    -1,
      72,    73,    -1,    -1,    -1,    -1,    -1,    79,    80,    81,
      -1,    -1,    84,    85,    -1,    -1,    88,     3,     4,     5,
       6,     7,     8,     9,    10,    -1,    12,    -1,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      -1,    -1,    -1,    29,    30,    31,    32,    -1,    -1,    -1,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    -1,
      46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    -1,    -1,    72,    73,    -1,    -1,
      -1,    -1,    -1,    79,    80,    81,    -1,    -1,    84,    85,
      -1,    -1,    88,     3,     4,     5,     6,     7,     8,     9,
      10,    -1,    12,    13,    14,    15,    16,    -1,    -1,    -1,
      -1,    21,    22,    -1,    24,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    -1,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    44,    -1,    46,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,    -1,
      -1,    -1,    72,    73,    -1,    -1,    -1,    -1,    -1,    79,
      80,    81,    -1,    -1,    84,    85,    -1,    -1,    88,     3,
       4,     5,     6,     7,     8,     9,    10,    -1,    12,    -1,
      14,    15,    16,    -1,    -1,    -1,    -1,    21,    22,    -1,
      24,    -1,    -1,    -1,    -1,    29,    30,    31,    32,    -1,
      -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      44,    -1,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    66,    67,    68,    -1,    -1,    -1,    72,    73,
      -1,    -1,    -1,    -1,    -1,    79,    80,    81,    -1,    -1,
      84,    85,    -1,    -1,    88,     3,     4,     5,     6,     7,
       8,     9,    10,    -1,    12,    -1,    14,    15,    16,    -1,
      -1,    -1,    -1,    21,    22,    -1,    24,    -1,    -1,    -1,
      -1,    29,    30,    31,    32,    -1,    -1,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,    46,    47,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    -1,    -1,    -1,    72,    73,    -1,    -1,    -1,    -1,
      -1,    79,    80,    81,    -1,    -1,    84,    85,    -1,    12,
      88,    14,    -1,    -1,    17,    18,    19,    20,    -1,    -1,
      23,    -1,    25,    -1,    27,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,    42,
      43,    -1,    45,    -1,    -1,    48,    -1,    50,    14,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    -1,    66,    67,    68,    69,    70,    -1,    72,
      73,    74,    75,    -1,    -1,    78,    79,    -1,    -1,    45,
      -1,    84,    85,    -1,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    -1,    72,    73,    74,    75,
      76,    77,    78,    79,    -1,    -1,    -1,    12,    -1,    14,
      -1,    87,    17,    18,    19,    20,    12,    -1,    23,    -1,
      25,    17,    18,    19,    20,    -1,    -1,    23,    -1,    25,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    -1,
      -1,    37,    38,    39,    40,    41,    42,    43,    53,    -1,
      -1,    56,    -1,    -1,    12,    -1,    -1,    14,    -1,    17,
      18,    19,    20,    68,    -1,    23,    -1,    25,    -1,    65,
      -1,    -1,    -1,    78,    79,    -1,    -1,    -1,    83,    37,
      38,    39,    40,    41,    42,    43,    -1,    83,    45,    -1,
      -1,    -1,    14,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    75,    76,
      77,    78,    79,    45,    82,    -1,    83,    14,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    76,    77,    78,    79,    45,    -1,
      -1,    83,    14,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    75,    76,
      77,    78,    79,    45,    -1,    -1,    83,    14,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    76,    77,    78,    79,    45,    -1,
      -1,    83,    14,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    75,    76,
      77,    78,    79,    45,    -1,    -1,    83,    14,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    76,    77,    78,    79,    45,    -1,
      -1,    83,    14,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    75,    76,
      77,    78,    79,    45,    14,    -1,    83,    -1,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    76,    77,    78,    79,    14,    -1,
      -1,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    72,    73,    74,    75,    76,    77,    78,    79,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    14,    72,    73,    74,    75,
      76,    77,    78,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    14,    72,    73,
      74,    75,    76,    77,    78,    79,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    14,    72,    73,    74,    75,    76,    77,    78,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    14,    72,    73,    74,    75,    76,
      77,    78,    79,    -1,    -1,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    14,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    -1,
      -1,    -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    14,    72,    73,    74,    75,    76,    77,    78,    79,
      -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    -1,    72,    73,    74,
      75,    76,    77,    78,    79,    65,    66,    67,    68,    69,
      70,    -1,    72,    73,    74,    75,    76,    77,    78,    79,
      -1,    -1,    -1,    66,    67,    68,    69,    70,    -1,    72,
      73,    74,    75,    76,    77,    78,    79,    68,    69,    70,
      -1,    72,    73,    74,    75,    76,    77,    78,    79,    12,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    -1,    -1,
      23,    -1,    25,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    35,    -1,    37,    38,    39,    40,    41,    42,
      43,    12,    -1,    -1,    -1,    -1,    17,    18,    19,    20,
      -1,    -1,    23,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    12,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    29,    30,    31,    32,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    46,    47,    56,
      66,    67,    68,    72,    73,    79,    80,    81,    84,    85,
      88,    91,    92,    93,    94,   103,   105,   106,   107,   109,
     110,   111,   112,   114,   115,   116,   126,   127,   131,   134,
     135,   136,   139,    79,    79,    24,    12,    13,    94,    12,
      14,    27,    28,    45,    48,    50,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    66,
      67,    68,    69,    70,    72,    73,    74,    75,    78,    79,
      84,    85,   127,   132,     9,    10,    11,    12,    15,    80,
     136,   138,    11,   138,    11,   138,    11,   138,    12,    79,
      94,    79,    12,    38,    39,    40,   128,    10,    80,   139,
     138,    59,    59,    59,    59,    79,    79,    79,    38,    39,
     128,    23,    37,    38,    40,    43,    23,    37,    40,    42,
      41,    94,    94,    94,    94,    94,    94,    92,    93,    94,
      94,     0,    48,    14,    45,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    72,    73,    74,    75,
      76,    77,    78,    79,    92,    94,   104,    24,     5,    24,
      65,   113,   127,   116,    14,    53,    56,    68,    78,    79,
     118,   121,   122,   123,   124,   125,    33,   138,   102,    85,
     138,    95,    96,    97,    86,    14,    78,    14,    78,    86,
      83,   118,   133,    11,    11,    11,    11,   126,    92,    38,
      40,    40,    59,    92,    92,    92,    92,    94,   126,    94,
      92,    94,    38,    40,    40,    40,    23,    37,    40,    40,
      40,    83,    83,    94,    93,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    11,    12,    23,
      37,    38,    39,    40,    43,    85,   112,   137,   138,    94,
      11,    85,   112,   137,    94,    93,    83,   101,   126,   129,
     130,    89,   108,    48,   108,   138,   138,     9,   114,   135,
       3,    86,   118,   118,   120,     3,    86,    83,   122,   129,
      68,   123,    79,   124,   125,    83,   104,   138,    24,    48,
      87,    98,    99,   100,   138,    98,    98,    86,    86,    86,
      86,    68,   133,    83,    83,    40,   126,    60,    60,    60,
      60,    83,    83,    83,    83,    83,    40,    40,    40,    94,
      86,    87,   138,    11,   138,    11,    86,   114,   104,    83,
      48,    94,    94,    86,   116,   117,    86,    83,    83,   119,
      83,   138,    94,    94,    86,   100,   138,    87,    86,    86,
      60,    79,    79,    79,    79,    94,    11,    11,    83,   114,
      82,   126,   117,    94,    94,    94,    94,    94,    83,    83,
      83,    83
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    90,    91,    91,    92,    92,    92,    92,    93,    93,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      95,    94,    96,    94,    97,    94,    98,    98,    99,    99,
     100,   100,   100,   101,    94,    94,   102,    94,   103,   104,
     104,   104,   105,   106,    94,   107,   107,    94,   108,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,   109,   109,    94,
      94,    94,    94,   110,   110,   110,   111,   111,   112,   112,
     112,   111,   111,   111,   113,   114,   114,   115,   116,   116,
     117,   117,   119,   118,   120,   118,   118,   118,   118,   118,
     121,   122,   122,   122,   123,   123,   123,   123,   123,   124,
     124,   124,   124,   125,   125,   126,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   128,   128,   128,   128,   129,   129,
     130,   130,   131,   131,   132,   133,   133,   134,   134,   135,
     135,   135,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   137,
     137,   137,   137,   137,   137,   137,   138,   138,   138,   138,
     138,   138,   139,   139,   139,   139
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     4,     4,     4,     1,     3,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       4,     4,     2,     4,     3,     4,     3,     4,     5,     3,
       3,     3,     4,     3,     4,     5,     3,     3,     4,     4,
       0,     5,     0,     5,     0,     5,     1,     1,     1,     2,
       3,     2,     2,     0,     5,     3,     0,     5,     1,     0,
       1,     3,     5,     4,     1,     1,     1,     3,     1,     3,
       4,     4,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     5,     3,     3,     1,     1,     1,     1,     1,
       1,     4,     4,     7,     7,     7,     7,     1,     2,     1,
       1,     1,     1,     1,     1,     3,     2,     3,     3,     4,
       5,     1,     2,     1,     2,     1,     0,     3,     1,     1,
       1,     0,     0,     4,     0,     3,     1,     2,     1,     2,
       1,     2,     1,     1,     3,     2,     1,     2,     1,     2,
       2,     3,     3,     2,     3,     1,     1,     1,     1,     1,
       2,     3,     2,     3,     3,     3,     2,     2,     3,     4,
       3,     3,     4,     3,     4,     3,     4,     2,     3,     2,
       3,     2,     3,     1,     2,     2,     2,     3,     2,     2,
       3,     2,     2,     3,     2,     2,     3,     2,     1,     2,
       1,     5,     2,     2,     1,     1,     1,     1,     1,     3,
       1,     3,     1,     2,     2,     0,     2,     2,     2,     1,
       1,     1,     2,     2,     4,     4,     4,     4,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     3,     3,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1
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
#line 278 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode(pstate, OP_TYPE);
			  write_exp_elt_type(pstate, (yyvsp[0].tval));
			  write_exp_elt_opcode(pstate, OP_TYPE);}
#line 2050 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 5:
#line 282 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_TYPEOF);
			}
#line 2058 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 6:
#line 286 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_TYPE);
			  write_exp_elt_type (pstate, (yyvsp[-1].tval));
			  write_exp_elt_opcode (pstate, OP_TYPE);
			}
#line 2068 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 7:
#line 292 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_DECLTYPE);
			}
#line 2076 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 9:
#line 300 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_COMMA); }
#line 2082 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 10:
#line 305 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_IND); }
#line 2088 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 11:
#line 309 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ADDR); }
#line 2094 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 12:
#line 313 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_NEG); }
#line 2100 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 13:
#line 317 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PLUS); }
#line 2106 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 14:
#line 321 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
#line 2112 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 15:
#line 325 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_COMPLEMENT); }
#line 2118 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 16:
#line 329 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREINCREMENT); }
#line 2124 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 17:
#line 333 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_PREDECREMENT); }
#line 2130 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 18:
#line 337 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTINCREMENT); }
#line 2136 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 19:
#line 341 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_POSTDECREMENT); }
#line 2142 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 20:
#line 345 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPEID); }
#line 2148 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 21:
#line 349 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_TYPEID); }
#line 2154 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 22:
#line 353 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_SIZEOF); }
#line 2160 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 23:
#line 357 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_ALIGNOF); }
#line 2166 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 24:
#line 361 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2174 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 25:
#line 367 "c-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  write_exp_string (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2183 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 26:
#line 374 "c-exp.y" /* yacc.c:1646  */
    { struct stoken s;
			  mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (pstate, s);
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2195 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 27:
#line 384 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  write_destructor_name (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2203 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 28:
#line 390 "c-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			  write_destructor_name (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_PTR); }
#line 2212 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 29:
#line 397 "c-exp.y" /* yacc.c:1646  */
    { /* exp->type::name becomes exp->*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (pstate, UNOP_ADDR);
			  write_exp_elt_opcode (pstate, STRUCTOP_MPTR); }
#line 2222 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 30:
#line 405 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_MPTR); }
#line 2228 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 31:
#line 409 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2236 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 32:
#line 415 "c-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2245 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 33:
#line 422 "c-exp.y" /* yacc.c:1646  */
    { struct stoken s;
			  mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (pstate, s);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2257 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 34:
#line 432 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_destructor_name (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2265 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 35:
#line 438 "c-exp.y" /* yacc.c:1646  */
    { mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_destructor_name (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
#line 2274 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 36:
#line 445 "c-exp.y" /* yacc.c:1646  */
    { /* exp.type::name becomes exp.*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (pstate, UNOP_ADDR);
			  write_exp_elt_opcode (pstate, STRUCTOP_MEMBER); }
#line 2284 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 37:
#line 453 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, STRUCTOP_MEMBER); }
#line 2290 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 38:
#line 457 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT); }
#line 2296 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 39:
#line 461 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT); }
#line 2302 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 40:
#line 470 "c-exp.y" /* yacc.c:1646  */
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
#line 2322 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 41:
#line 486 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			  end_msglist (pstate);
			  write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			}
#line 2331 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 42:
#line 493 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate,
					    parse_type (pstate)->builtin_int);
			  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].theclass).theclass);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  start_msglist();
			}
#line 2344 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 43:
#line 502 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			  end_msglist (pstate);
			  write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			}
#line 2353 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 44:
#line 509 "c-exp.y" /* yacc.c:1646  */
    { start_msglist(); }
#line 2359 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 45:
#line 511 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			  end_msglist (pstate);
			  write_exp_elt_opcode (pstate, OP_OBJC_MSGCALL);
			}
#line 2368 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 46:
#line 518 "c-exp.y" /* yacc.c:1646  */
    { add_msglist(&(yyvsp[0].sval), 0); }
#line 2374 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 50:
#line 527 "c-exp.y" /* yacc.c:1646  */
    { add_msglist(&(yyvsp[-2].sval), 1); }
#line 2380 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 51:
#line 529 "c-exp.y" /* yacc.c:1646  */
    { add_msglist(0, 1);   }
#line 2386 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 52:
#line 531 "c-exp.y" /* yacc.c:1646  */
    { add_msglist(0, 0);   }
#line 2392 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 53:
#line 537 "c-exp.y" /* yacc.c:1646  */
    { start_arglist (); }
#line 2398 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 54:
#line 539 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_FUNCALL);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) end_arglist ());
			  write_exp_elt_opcode (pstate, OP_FUNCALL); }
#line 2407 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 55:
#line 549 "c-exp.y" /* yacc.c:1646  */
    { start_arglist ();
			  write_exp_elt_opcode (pstate, OP_FUNCALL);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) end_arglist ());
			  write_exp_elt_opcode (pstate, OP_FUNCALL); }
#line 2417 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 56:
#line 558 "c-exp.y" /* yacc.c:1646  */
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
#line 2438 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 57:
#line 575 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_FUNCALL);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) end_arglist ());
			  write_exp_elt_opcode (pstate, OP_FUNCALL);
			}
#line 2449 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 58:
#line 584 "c-exp.y" /* yacc.c:1646  */
    { start_arglist (); }
#line 2455 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 60:
#line 591 "c-exp.y" /* yacc.c:1646  */
    { arglist_len = 1; }
#line 2461 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 61:
#line 595 "c-exp.y" /* yacc.c:1646  */
    { arglist_len++; }
#line 2467 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 62:
#line 599 "c-exp.y" /* yacc.c:1646  */
    {
			  std::vector<struct type *> *type_list = (yyvsp[-2].tvec);
			  LONGEST len = type_list->size ();

			  write_exp_elt_opcode (pstate, TYPE_INSTANCE);
			  /* Save the const/volatile qualifiers as
			     recorded by the const_or_volatile
			     production's actions.  */
			  write_exp_elt_longcst (pstate,
						 follow_type_instance_flags ());
			  write_exp_elt_longcst (pstate, len);
			  for (type *type_elt : *type_list)
			    write_exp_elt_type (pstate, type_elt);
			  write_exp_elt_longcst(pstate, len);
			  write_exp_elt_opcode (pstate, TYPE_INSTANCE);
			}
#line 2488 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 63:
#line 618 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TYPE_INSTANCE);
			 /* See above.  */
			 write_exp_elt_longcst (pstate,
						follow_type_instance_flags ());
			 write_exp_elt_longcst (pstate, 0);
			 write_exp_elt_longcst (pstate, 0);
			 write_exp_elt_opcode (pstate, TYPE_INSTANCE);
		       }
#line 2501 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 67:
#line 640 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_FUNC_STATIC_VAR);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, OP_FUNC_STATIC_VAR);
			}
#line 2511 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 68:
#line 648 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = end_arglist () - 1; }
#line 2517 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 69:
#line 651 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_ARRAY);
			  write_exp_elt_longcst (pstate, (LONGEST) 0);
			  write_exp_elt_longcst (pstate, (LONGEST) (yyvsp[0].lval));
			  write_exp_elt_opcode (pstate, OP_ARRAY); }
#line 2526 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 70:
#line 658 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_MEMVAL_TYPE); }
#line 2532 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 71:
#line 662 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST_TYPE); }
#line 2538 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 72:
#line 666 "c-exp.y" /* yacc.c:1646  */
    { }
#line 2544 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 73:
#line 672 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REPEAT); }
#line 2550 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 74:
#line 676 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_MUL); }
#line 2556 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 75:
#line 680 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_DIV); }
#line 2562 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 76:
#line 684 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_REM); }
#line 2568 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 77:
#line 688 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ADD); }
#line 2574 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 78:
#line 692 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_SUB); }
#line 2580 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 79:
#line 696 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LSH); }
#line 2586 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 80:
#line 700 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_RSH); }
#line 2592 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 81:
#line 704 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_EQUAL); }
#line 2598 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 82:
#line 708 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
#line 2604 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 83:
#line 712 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LEQ); }
#line 2610 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 84:
#line 716 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GEQ); }
#line 2616 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 85:
#line 720 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LESS); }
#line 2622 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 86:
#line 724 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_GTR); }
#line 2628 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 87:
#line 728 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
#line 2634 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 88:
#line 732 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
#line 2640 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 89:
#line 736 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
#line 2646 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 90:
#line 740 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
#line 2652 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 91:
#line 744 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
#line 2658 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 92:
#line 748 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, TERNOP_COND); }
#line 2664 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 93:
#line 752 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
#line 2670 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 94:
#line 756 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode (pstate, (yyvsp[-1].opcode));
			  write_exp_elt_opcode (pstate,
						BINOP_ASSIGN_MODIFY); }
#line 2679 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 95:
#line 763 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, (yyvsp[0].typed_val_int).type);
			  write_exp_elt_longcst (pstate, (LONGEST) ((yyvsp[0].typed_val_int).val));
			  write_exp_elt_opcode (pstate, OP_LONG); }
#line 2688 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 96:
#line 770 "c-exp.y" /* yacc.c:1646  */
    {
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &(yyvsp[0].tsval);
			  write_exp_string_vector (pstate, (yyvsp[0].tsval).type, &vec);
			}
#line 2699 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 97:
#line 779 "c-exp.y" /* yacc.c:1646  */
    { YYSTYPE val;
			  parse_number (pstate, (yyvsp[0].ssym).stoken.ptr,
					(yyvsp[0].ssym).stoken.length, 0, &val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, val.typed_val_int.type);
			  write_exp_elt_longcst (pstate,
					    (LONGEST) val.typed_val_int.val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			}
#line 2713 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 98:
#line 792 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_FLOAT);
			  write_exp_elt_type (pstate, (yyvsp[0].typed_val_float).type);
			  write_exp_elt_floatcst (pstate, (yyvsp[0].typed_val_float).val);
			  write_exp_elt_opcode (pstate, OP_FLOAT); }
#line 2722 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 100:
#line 802 "c-exp.y" /* yacc.c:1646  */
    {
			  write_dollar_variable (pstate, (yyvsp[0].sval));
			}
#line 2730 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 101:
#line 808 "c-exp.y" /* yacc.c:1646  */
    {
			  write_exp_elt_opcode (pstate, OP_OBJC_SELECTOR);
			  write_exp_string (pstate, (yyvsp[-1].sval));
			  write_exp_elt_opcode (pstate, OP_OBJC_SELECTOR); }
#line 2739 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 102:
#line 815 "c-exp.y" /* yacc.c:1646  */
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
#line 2761 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 103:
#line 835 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate,
						UNOP_REINTERPRET_CAST); }
#line 2768 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 104:
#line 840 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_CAST_TYPE); }
#line 2774 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 105:
#line 844 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, UNOP_DYNAMIC_CAST); }
#line 2780 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 106:
#line 848 "c-exp.y" /* yacc.c:1646  */
    { /* We could do more error checking here, but
			     it doesn't seem worthwhile.  */
			  write_exp_elt_opcode (pstate, UNOP_CAST_TYPE); }
#line 2788 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 107:
#line 855 "c-exp.y" /* yacc.c:1646  */
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
#line 2808 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 108:
#line 872 "c-exp.y" /* yacc.c:1646  */
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
#line 2828 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 109:
#line 890 "c-exp.y" /* yacc.c:1646  */
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
#line 2863 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 110:
#line 925 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_OBJC_NSSTRING);
			  write_exp_string (pstate, (yyvsp[0].sval));
			  write_exp_elt_opcode (pstate, OP_OBJC_NSSTRING); }
#line 2871 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 111:
#line 932 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
                          write_exp_elt_type (pstate,
					  parse_type (pstate)->builtin_bool);
                          write_exp_elt_longcst (pstate, (LONGEST) 1);
                          write_exp_elt_opcode (pstate, OP_LONG); }
#line 2881 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 112:
#line 940 "c-exp.y" /* yacc.c:1646  */
    { write_exp_elt_opcode (pstate, OP_LONG);
                          write_exp_elt_type (pstate,
					  parse_type (pstate)->builtin_bool);
                          write_exp_elt_longcst (pstate, (LONGEST) 0);
                          write_exp_elt_opcode (pstate, OP_LONG); }
#line 2891 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 113:
#line 950 "c-exp.y" /* yacc.c:1646  */
    {
			  if ((yyvsp[0].ssym).sym.symbol)
			    (yyval.bval) = SYMBOL_BLOCK_VALUE ((yyvsp[0].ssym).sym.symbol);
			  else
			    error (_("No file or function \"%s\"."),
				   copy_name ((yyvsp[0].ssym).stoken));
			}
#line 2903 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 114:
#line 958 "c-exp.y" /* yacc.c:1646  */
    {
			  (yyval.bval) = (yyvsp[0].bval);
			}
#line 2911 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 115:
#line 964 "c-exp.y" /* yacc.c:1646  */
    { struct symbol *tem
			    = lookup_symbol (copy_name ((yyvsp[0].sval)), (yyvsp[-2].bval),
					     VAR_DOMAIN, NULL).symbol;

			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy_name ((yyvsp[0].sval)));
			  (yyval.bval) = SYMBOL_BLOCK_VALUE (tem); }
#line 2924 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 116:
#line 975 "c-exp.y" /* yacc.c:1646  */
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
#line 2941 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 117:
#line 990 "c-exp.y" /* yacc.c:1646  */
    { struct block_symbol sym
			    = lookup_symbol (copy_name ((yyvsp[0].sval)), (yyvsp[-2].bval),
					     VAR_DOMAIN, NULL);

			  if (sym.symbol == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy_name ((yyvsp[0].sval)));
			  if (symbol_read_needs_frame (sym.symbol))

			    innermost_block.update (sym);

			  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			  write_exp_elt_block (pstate, sym.block);
			  write_exp_elt_sym (pstate, sym.symbol);
			  write_exp_elt_opcode (pstate, OP_VAR_VALUE); }
#line 2961 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 118:
#line 1008 "c-exp.y" /* yacc.c:1646  */
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
#line 2978 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 119:
#line 1021 "c-exp.y" /* yacc.c:1646  */
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
#line 3006 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 120:
#line 1045 "c-exp.y" /* yacc.c:1646  */
    {
			  char *copy = copy_name ((yyvsp[-2].sval));
			  error (_("No type \"%s\" within class "
				   "or namespace \"%s\"."),
				 copy, TYPE_SAFE_NAME ((yyvsp[-4].tsym).type));
			}
#line 3017 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 122:
#line 1055 "c-exp.y" /* yacc.c:1646  */
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
#line 3047 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 123:
#line 1083 "c-exp.y" /* yacc.c:1646  */
    { struct block_symbol sym = (yyvsp[0].ssym).sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				innermost_block.update (sym);

			      /* If we found a function, see if it's
				 an ifunc resolver that has the same
				 address as the ifunc symbol itself.
				 If so, prefer the ifunc symbol.  */

			      bound_minimal_symbol resolver
				= find_gnu_ifunc (sym.symbol);
			      if (resolver.minsym != NULL)
				write_exp_msymbol (pstate, resolver);
			      else
				{
				  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
				  write_exp_elt_block (pstate, sym.block);
				  write_exp_elt_sym (pstate, sym.symbol);
				  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
				}
			    }
			  else if ((yyvsp[0].ssym).is_a_field_of_this)
			    {
			      /* C++: it hangs off of `this'.  Must
			         not inadvertently convert from a method call
				 to data ref.  */
			      innermost_block.update (sym);
			      write_exp_elt_opcode (pstate, OP_THIS);
			      write_exp_elt_opcode (pstate, OP_THIS);
			      write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			      write_exp_string (pstate, (yyvsp[0].ssym).stoken);
			      write_exp_elt_opcode (pstate, STRUCTOP_PTR);
			    }
			  else
			    {
			      char *arg = copy_name ((yyvsp[0].ssym).stoken);

			      bound_minimal_symbol msymbol
				= lookup_bound_minimal_symbol (arg);
			      if (msymbol.minsym == NULL)
				{
				  if (!have_full_symbols () && !have_partial_symbols ())
				    error (_("No symbol table is loaded.  Use the \"file\" command."));
				  else
				    error (_("No symbol \"%s\" in current context."),
					   copy_name ((yyvsp[0].ssym).stoken));
				}

			      /* This minsym might be an alias for
				 another function.  See if we can find
				 the debug symbol for the target, and
				 if so, use it instead, since it has
				 return type / prototype info.  This
				 is important for example for "p
				 *__errno_location()".  */
			      symbol *alias_target
				= ((msymbol.minsym->type != mst_text_gnu_ifunc
				    && msymbol.minsym->type != mst_data_gnu_ifunc)
				   ? find_function_alias_target (msymbol)
				   : NULL);
			      if (alias_target != NULL)
				{
				  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
				  write_exp_elt_block
				    (pstate, SYMBOL_BLOCK_VALUE (alias_target));
				  write_exp_elt_sym (pstate, alias_target);
				  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
				}
			      else
				write_exp_msymbol (pstate, msymbol);
			    }
			}
#line 3127 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 124:
#line 1161 "c-exp.y" /* yacc.c:1646  */
    { insert_type_address_space (pstate, copy_name ((yyvsp[0].ssym).stoken)); }
#line 3133 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 132:
#line 1182 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_pointer); }
#line 3139 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 134:
#line 1185 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_pointer); }
#line 3145 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 136:
#line 1188 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_reference); }
#line 3151 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 137:
#line 1190 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_reference); }
#line 3157 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 138:
#line 1192 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_rvalue_reference); }
#line 3163 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 139:
#line 1194 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_rvalue_reference); }
#line 3169 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 140:
#line 1198 "c-exp.y" /* yacc.c:1646  */
    {
			  (yyval.type_stack) = get_type_stack ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3178 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 141:
#line 1205 "c-exp.y" /* yacc.c:1646  */
    { (yyval.type_stack) = append_type_stack ((yyvsp[0].type_stack), (yyvsp[-1].type_stack)); }
#line 3184 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 144:
#line 1211 "c-exp.y" /* yacc.c:1646  */
    { (yyval.type_stack) = (yyvsp[-1].type_stack); }
#line 3190 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 145:
#line 1213 "c-exp.y" /* yacc.c:1646  */
    {
			  push_type_stack ((yyvsp[-1].type_stack));
			  push_type_int ((yyvsp[0].lval));
			  push_type (tp_array);
			  (yyval.type_stack) = get_type_stack ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3202 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 146:
#line 1221 "c-exp.y" /* yacc.c:1646  */
    {
			  push_type_int ((yyvsp[0].lval));
			  push_type (tp_array);
			  (yyval.type_stack) = get_type_stack ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3213 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 147:
#line 1229 "c-exp.y" /* yacc.c:1646  */
    {
			  push_type_stack ((yyvsp[-1].type_stack));
			  push_typelist ((yyvsp[0].tvec));
			  (yyval.type_stack) = get_type_stack ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3224 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 148:
#line 1236 "c-exp.y" /* yacc.c:1646  */
    {
			  push_typelist ((yyvsp[0].tvec));
			  (yyval.type_stack) = get_type_stack ();
			  cpstate->type_stacks.emplace_back ((yyval.type_stack));
			}
#line 3234 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 149:
#line 1244 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = -1; }
#line 3240 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 150:
#line 1246 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = -1; }
#line 3246 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 151:
#line 1248 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-1].typed_val_int).val; }
#line 3252 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 152:
#line 1250 "c-exp.y" /* yacc.c:1646  */
    { (yyval.lval) = (yyvsp[-1].typed_val_int).val; }
#line 3258 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 153:
#line 1254 "c-exp.y" /* yacc.c:1646  */
    {
			  (yyval.tvec) = new std::vector<struct type *>;
			  cpstate->type_lists.emplace_back ((yyval.tvec));
			}
#line 3267 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 154:
#line 1259 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tvec) = (yyvsp[-1].tvec); }
#line 3273 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 156:
#line 1285 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = (yyvsp[0].tsym).type; }
#line 3279 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 157:
#line 1287 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "int"); }
#line 3287 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 158:
#line 1291 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3295 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 159:
#line 1295 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "short"); }
#line 3303 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 160:
#line 1299 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3311 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 161:
#line 1303 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3319 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 162:
#line 1307 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3327 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 163:
#line 1311 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long"); }
#line 3335 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 164:
#line 1315 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long"); }
#line 3343 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 165:
#line 1319 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long"); }
#line 3351 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 166:
#line 1323 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long"); }
#line 3359 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 167:
#line 1327 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3367 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 168:
#line 1331 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3375 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 169:
#line 1335 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3383 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 170:
#line 1339 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3391 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 171:
#line 1343 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3399 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 172:
#line 1347 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "long long"); }
#line 3407 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 173:
#line 1351 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long long"); }
#line 3415 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 174:
#line 1355 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long long"); }
#line 3423 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 175:
#line 1359 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long long"); }
#line 3431 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 176:
#line 1363 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "long long"); }
#line 3439 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 177:
#line 1367 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "short"); }
#line 3447 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 178:
#line 1371 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "short"); }
#line 3455 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 179:
#line 1375 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "short"); }
#line 3463 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 180:
#line 1379 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "short"); }
#line 3471 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 181:
#line 1383 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "short"); }
#line 3479 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 182:
#line 1387 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "short"); }
#line 3487 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 183:
#line 1391 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_typename (parse_language (pstate),
						parse_gdbarch (pstate),
						"double",
						(struct block *) NULL,
						0); }
#line 3497 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 184:
#line 1397 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_typename (parse_language (pstate),
						parse_gdbarch (pstate),
						"long double",
						(struct block *) NULL,
						0); }
#line 3507 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 185:
#line 1403 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
					      expression_context_block); }
#line 3514 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 186:
#line 1406 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3523 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 187:
#line 1411 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, (yyvsp[-1].sval).ptr,
					       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3533 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 188:
#line 1417 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
					      expression_context_block); }
#line 3540 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 189:
#line 1420 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3549 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 190:
#line 1425 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_STRUCT, (yyvsp[-1].sval).ptr,
					       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3559 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 191:
#line 1431 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_union (copy_name ((yyvsp[0].sval)),
					     expression_context_block); }
#line 3566 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 192:
#line 1434 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_UNION, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3575 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 193:
#line 1439 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_UNION, (yyvsp[-1].sval).ptr,
					       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3585 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 194:
#line 1445 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_enum (copy_name ((yyvsp[0].sval)),
					    expression_context_block); }
#line 3592 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 195:
#line 1448 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_ENUM, "", 0);
			  (yyval.tval) = NULL;
			}
#line 3601 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 196:
#line 1453 "c-exp.y" /* yacc.c:1646  */
    {
			  mark_completion_tag (TYPE_CODE_ENUM, (yyvsp[-1].sval).ptr,
					       (yyvsp[-1].sval).length);
			  (yyval.tval) = NULL;
			}
#line 3611 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 197:
#line 1459 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 TYPE_NAME((yyvsp[0].tsym).type)); }
#line 3619 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 198:
#line 1463 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_unsigned_typename (parse_language (pstate),
							 parse_gdbarch (pstate),
							 "int"); }
#line 3627 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 199:
#line 1467 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       TYPE_NAME((yyvsp[0].tsym).type)); }
#line 3635 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 200:
#line 1471 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_signed_typename (parse_language (pstate),
						       parse_gdbarch (pstate),
						       "int"); }
#line 3643 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 201:
#line 1478 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = lookup_template_type(copy_name((yyvsp[-3].sval)), (yyvsp[-1].tval),
						    expression_context_block);
			}
#line 3651 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 202:
#line 1482 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = follow_types ((yyvsp[0].tval)); }
#line 3657 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 203:
#line 1484 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = follow_types ((yyvsp[-1].tval)); }
#line 3663 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 205:
#line 1489 "c-exp.y" /* yacc.c:1646  */
    {
		  (yyval.tsym).stoken.ptr = "int";
		  (yyval.tsym).stoken.length = 3;
		  (yyval.tsym).type = lookup_signed_typename (parse_language (pstate),
						    parse_gdbarch (pstate),
						    "int");
		}
#line 3675 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 206:
#line 1497 "c-exp.y" /* yacc.c:1646  */
    {
		  (yyval.tsym).stoken.ptr = "long";
		  (yyval.tsym).stoken.length = 4;
		  (yyval.tsym).type = lookup_signed_typename (parse_language (pstate),
						    parse_gdbarch (pstate),
						    "long");
		}
#line 3687 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 207:
#line 1505 "c-exp.y" /* yacc.c:1646  */
    {
		  (yyval.tsym).stoken.ptr = "short";
		  (yyval.tsym).stoken.length = 5;
		  (yyval.tsym).type = lookup_signed_typename (parse_language (pstate),
						    parse_gdbarch (pstate),
						    "short");
		}
#line 3699 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 208:
#line 1516 "c-exp.y" /* yacc.c:1646  */
    { check_parameter_typelist ((yyvsp[0].tvec)); }
#line 3705 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 209:
#line 1518 "c-exp.y" /* yacc.c:1646  */
    {
			  (yyvsp[-2].tvec)->push_back (NULL);
			  check_parameter_typelist ((yyvsp[-2].tvec));
			  (yyval.tvec) = (yyvsp[-2].tvec);
			}
#line 3715 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 210:
#line 1527 "c-exp.y" /* yacc.c:1646  */
    {
		  std::vector<struct type *> *typelist
		    = new std::vector<struct type *>;
		  cpstate->type_lists.emplace_back (typelist);

		  typelist->push_back ((yyvsp[0].tval));
		  (yyval.tvec) = typelist;
		}
#line 3728 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 211:
#line 1536 "c-exp.y" /* yacc.c:1646  */
    {
		  (yyvsp[-2].tvec)->push_back ((yyvsp[0].tval));
		  (yyval.tvec) = (yyvsp[-2].tvec);
		}
#line 3737 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 213:
#line 1544 "c-exp.y" /* yacc.c:1646  */
    {
		  push_type_stack ((yyvsp[0].type_stack));
		  (yyval.tval) = follow_types ((yyvsp[-1].tval));
		}
#line 3746 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 214:
#line 1551 "c-exp.y" /* yacc.c:1646  */
    { (yyval.tval) = follow_types ((yyvsp[-1].tval)); }
#line 3752 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 219:
#line 1563 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_const);
			  insert_type (tp_volatile);
			}
#line 3760 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 220:
#line 1567 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_const); }
#line 3766 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 221:
#line 1569 "c-exp.y" /* yacc.c:1646  */
    { insert_type (tp_volatile); }
#line 3772 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 222:
#line 1573 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" new"); }
#line 3778 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 223:
#line 1575 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" delete"); }
#line 3784 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 224:
#line 1577 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" new[]"); }
#line 3790 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 225:
#line 1579 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" delete[]"); }
#line 3796 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 226:
#line 1581 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" new[]"); }
#line 3802 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 227:
#line 1583 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (" delete[]"); }
#line 3808 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 228:
#line 1585 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("+"); }
#line 3814 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 229:
#line 1587 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("-"); }
#line 3820 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 230:
#line 1589 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("*"); }
#line 3826 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 231:
#line 1591 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("/"); }
#line 3832 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 232:
#line 1593 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("%"); }
#line 3838 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 233:
#line 1595 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("^"); }
#line 3844 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 234:
#line 1597 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("&"); }
#line 3850 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 235:
#line 1599 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("|"); }
#line 3856 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 236:
#line 1601 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("~"); }
#line 3862 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 237:
#line 1603 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("!"); }
#line 3868 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 238:
#line 1605 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("="); }
#line 3874 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 239:
#line 1607 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("<"); }
#line 3880 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 240:
#line 1609 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (">"); }
#line 3886 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 241:
#line 1611 "c-exp.y" /* yacc.c:1646  */
    { const char *op = " unknown";
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
#line 3930 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 242:
#line 1651 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("<<"); }
#line 3936 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 243:
#line 1653 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (">>"); }
#line 3942 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 244:
#line 1655 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("=="); }
#line 3948 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 245:
#line 1657 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("!="); }
#line 3954 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 246:
#line 1659 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("<="); }
#line 3960 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 247:
#line 1661 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (">="); }
#line 3966 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 248:
#line 1663 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("&&"); }
#line 3972 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 249:
#line 1665 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("||"); }
#line 3978 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 250:
#line 1667 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("++"); }
#line 3984 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 251:
#line 1669 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("--"); }
#line 3990 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 252:
#line 1671 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken (","); }
#line 3996 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 253:
#line 1673 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("->*"); }
#line 4002 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 254:
#line 1675 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("->"); }
#line 4008 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 255:
#line 1677 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("()"); }
#line 4014 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 256:
#line 1679 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("[]"); }
#line 4020 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 257:
#line 1681 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = operator_stoken ("[]"); }
#line 4026 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 258:
#line 1683 "c-exp.y" /* yacc.c:1646  */
    { string_file buf;

			  c_print_type ((yyvsp[0].tval), NULL, &buf, -1, 0,
					&type_print_raw_options);

			  /* This also needs canonicalization.  */
			  std::string canon
			    = cp_canonicalize_string (buf.c_str ());
			  if (canon.empty ())
			    canon = std::move (buf.string ());
			  (yyval.sval) = operator_stoken ((" " + canon).c_str ());
			}
#line 4043 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 260:
#line 1706 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = typename_stoken ("double"); }
#line 4049 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 261:
#line 1707 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = typename_stoken ("int"); }
#line 4055 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 262:
#line 1708 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = typename_stoken ("long"); }
#line 4061 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 263:
#line 1709 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = typename_stoken ("short"); }
#line 4067 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 264:
#line 1710 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = typename_stoken ("signed"); }
#line 4073 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 265:
#line 1711 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = typename_stoken ("unsigned"); }
#line 4079 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 266:
#line 1714 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 4085 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 267:
#line 1715 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 4091 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 268:
#line 1716 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].tsym).stoken; }
#line 4097 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 269:
#line 1717 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 4103 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 270:
#line 1718 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].ssym).stoken; }
#line 4109 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 271:
#line 1719 "c-exp.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].sval); }
#line 4115 "c-exp.c.tmp" /* yacc.c:1646  */
    break;

  case 274:
#line 1732 "c-exp.y" /* yacc.c:1646  */
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
#line 4131 "c-exp.c.tmp" /* yacc.c:1646  */
    break;


#line 4135 "c-exp.c.tmp" /* yacc.c:1646  */
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
#line 1746 "c-exp.y" /* yacc.c:1906  */


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
  struct stoken st = { NULL, 0 };
  char *buf;

  st.length = CP_OPERATOR_LEN + strlen (op);
  buf = (char *) xmalloc (st.length + 1);
  strcpy (buf, CP_OPERATOR_STR);
  strcat (buf, op);
  st.ptr = buf;

  /* The toplevel (c_parse) will free the memory allocated here.  */
  cpstate->strings.emplace_back (buf);
  return st;
};

/* Returns a stoken of the type named TYPE.  */

static struct stoken
typename_stoken (const char *type)
{
  struct stoken st = { type, 0 };
  st.length = strlen (type);
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
check_parameter_typelist (std::vector<struct type *> *params)
{
  struct type *type;
  int ix;

  for (ix = 0; ix < params->size (); ++ix)
    {
      type = (*params)[ix];
      if (type != NULL && TYPE_CODE (check_typedef (type)) == TYPE_CODE_VOID)
	{
	  if (ix == 0)
	    {
	      if (params->size () == 1)
		{
		  /* Ok.  */
		  break;
		}
	      error (_("parameter types following 'void'"));
	    }
	  else
	    error (_("'void' invalid as parameter type"));
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
  ULONGEST n = 0;
  ULONGEST prevn = 0;
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
      /* Handle suffixes for decimal floating-point: "df", "dd" or "dl".  */
      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_decfloat;
	  len -= 2;
	}
      else if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'd')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_decdouble;
	  len -= 2;
	}
      else if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_declong;
	  len -= 2;
	}
      /* Handle suffixes: 'f' for float, 'l' for long double.  */
      else if (len >= 1 && TOLOWER (p[len - 1]) == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_float;
	  len -= 1;
	}
      else if (len >= 1 && TOLOWER (p[len - 1]) == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_long_double;
	  len -= 1;
	}
      /* Default type for floating-point literals is double.  */
      else
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_double;
	}

      if (!parse_float (p, len,
			putithere->typed_val_float.type,
			putithere->typed_val_float.val))
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
	  if (unsigned_p && prevn >= n)
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

  un = n >> 2;
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
      if (!ISXDIGIT (*tokptr))
	error (_("\\x escape without a following hex digit"));
      while (ISXDIGIT (*tokptr))
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
	     i < 3 && ISDIGIT (*tokptr) && *tokptr != '8' && *tokptr != '9';
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
	if (!ISXDIGIT (*tokptr))
	  error (_("\\%c escape without a following hex digit"), c);
	for (i = 0; i < len && ISXDIGIT (*tokptr); ++i)
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

/* Identifier-like tokens.  Only type-specifiers than can appear in
   multi-word type names (for example 'double' can appear in 'long
   double') need to be listed here.  type-specifiers that are only ever
   single word (like 'float') are handled by the classify_name function.  */
static const struct token ident_tokens[] =
  {
    {"unsigned", UNSIGNED, OP_NULL, 0},
    {"template", TEMPLATE, OP_NULL, FLAG_CXX},
    {"volatile", VOLATILE_KEYWORD, OP_NULL, 0},
    {"struct", STRUCT, OP_NULL, 0},
    {"signed", SIGNED_KEYWORD, OP_NULL, 0},
    {"sizeof", SIZEOF, OP_NULL, 0},
    {"_Alignof", ALIGNOF, OP_NULL, 0},
    {"alignof", ALIGNOF, OP_NULL, FLAG_CXX},
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


static void
scan_macro_expansion (char *expansion)
{
  char *copy;

  /* We'd better not be trying to push the stack twice.  */
  gdb_assert (! cpstate->macro_original_text);

  /* Copy to the obstack, and then free the intermediate
     expansion.  */
  copy = (char *) obstack_copy0 (&cpstate->expansion_obstack, expansion,
				 strlen (expansion));
  xfree (expansion);

  /* Save the old lexptr value, so we can return to it when we're done
     parsing the expanded text.  */
  cpstate->macro_original_text = lexptr;
  lexptr = copy;
}

static int
scanning_macro_expansion (void)
{
  return cpstate->macro_original_text != 0;
}

static void
finished_macro_expansion (void)
{
  /* There'd better be something to pop back to.  */
  gdb_assert (cpstate->macro_original_text);

  /* Pop back to the original text.  */
  lexptr = cpstate->macro_original_text;
  cpstate->macro_original_text = 0;
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
   operator -- either '.' or ARROW.  */
static bool last_was_structop;

/* Read one token, getting characters through lexptr.  */

static int
lex_one_token (struct parser_state *par_state, bool *is_quoted_name)
{
  int c;
  int namelen;
  unsigned int i;
  const char *tokstart;
  bool saw_structop = last_was_structop;
  char *copy;

  last_was_structop = false;
  *is_quoted_name = false;

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
	if (tokentab2[i].token == ARROW)
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
      else if (parse_completion && saw_structop)
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
	  last_was_structop = true;
	  goto symbol;		/* Nope, must be a symbol. */
	}
      /* FALL THRU.  */

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

	if (parse_language (par_state)->la_language == language_objc)
	  {
	    size_t len = strlen ("selector");

	    if (strncmp (p, "selector", len) == 0
		&& (p[len] == '\0' || ISSPACE (p[len])))
	      {
		lexptr = p + len;
		return SELECTOR;
	      }
	    else if (*p == '"')
	      goto parse_string;
	  }

	while (ISSPACE (*p))
	  p++;
	size_t len = strlen ("entry");
	if (strncmp (p, "entry", len) == 0 && !c_ident_is_alnum (p[len])
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
		*is_quoted_name = true;

		goto tryname;
	      }
	    else if (host_len > 1)
	      error (_("Invalid character constant."));
	  }
	return result;
      }
    }

  if (!(c == '_' || c == '$' || c_ident_is_alpha (c)))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || c_ident_is_alnum (c) || c == '<');)
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
    return DOLLAR_VARIABLE;

  if (parse_completion && *lexptr == '\0')
    saw_name_at_eof = 1;

  yylval.ssym.stoken = yylval.sval;
  yylval.ssym.sym.symbol = NULL;
  yylval.ssym.sym.block = NULL;
  yylval.ssym.is_a_field_of_this = 0;
  return NAME;
}

/* An object of this type is pushed on a FIFO by the "outer" lexer.  */
struct token_and_value
{
  int token;
  YYSTYPE value;
};

/* A FIFO of tokens that have been read but not yet returned to the
   parser.  */
static std::vector<token_and_value> token_fifo;

/* Non-zero if the lexer should return tokens from the FIFO.  */
static int popping;

/* Temporary storage for c_lex; this holds symbol names as they are
   built up.  */
auto_obstack name_obstack;

/* Classify a NAME token.  The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.  BLOCK is the block
   in which lookups start; this can be NULL to mean the global scope.
   IS_QUOTED_NAME is non-zero if the name token was originally quoted
   in single quotes.  IS_AFTER_STRUCTOP is true if this name follows
   a structure operator -- either '.' or ARROW  */

static int
classify_name (struct parser_state *par_state, const struct block *block,
	       bool is_quoted_name, bool is_after_structop)
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

      /* If we found a field on the "this" object, or we are looking
	 up a field on a struct, then we want to prefer it over a
	 filename.  However, if the name was quoted, then it is better
	 to check for a filename or a block, since this is the only
	 way the user has of requiring the extension to be used.  */
      if ((is_a_field_of_this.type == NULL && !is_after_structop) 
	  || is_quoted_name)
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
    return classify_name (par_state, block, false, false);

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
  bool is_quoted_name, last_lex_was_structop;

  if (popping && !token_fifo.empty ())
    goto do_pop;
  popping = 0;

  last_lex_was_structop = last_was_structop;

  /* Read the first token and decide what to do.  Most of the
     subsequent code is C++-only; but also depends on seeing a "::" or
     name-like token.  */
  current.token = lex_one_token (pstate, &is_quoted_name);
  if (current.token == NAME)
    current.token = classify_name (pstate, expression_context_block,
				   is_quoted_name, last_lex_was_structop);
  if (parse_language (pstate)->la_language != language_cplus
      || (current.token != TYPENAME && current.token != COLONCOLON
	  && current.token != FILENAME))
    return current.token;

  /* Read any sequence of alternating "::" and name-like tokens into
     the token FIFO.  */
  current.value = yylval;
  token_fifo.push_back (current);
  last_was_coloncolon = current.token == COLONCOLON;
  while (1)
    {
      bool ignore;

      /* We ignore quoted names other than the very first one.
	 Subsequent ones do not have any special meaning.  */
      current.token = lex_one_token (pstate, &ignore);
      current.value = yylval;
      token_fifo.push_back (current);

      if ((last_was_coloncolon && current.token != NAME)
	  || (!last_was_coloncolon && current.token != COLONCOLON))
	break;
      last_was_coloncolon = !last_was_coloncolon;
    }
  popping = 1;

  /* We always read one extra token, so compute the number of tokens
     to examine accordingly.  */
  last_to_examine = token_fifo.size () - 2;
  next_to_examine = 0;

  current = token_fifo[next_to_examine];
  ++next_to_examine;

  name_obstack.clear ();
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
      token_and_value next;

      next = token_fifo[next_to_examine];
      ++next_to_examine;

      if (next.token == NAME && last_was_coloncolon)
	{
	  int classification;

	  yylval = next.value;
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
	  obstack_grow (&name_obstack, next.value.sval.ptr,
			next.value.sval.length);

	  yylval.sval.ptr = (const char *) obstack_base (&name_obstack);
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
	  break;
	}
    }

  /* If we have a replacement token, install it as the first token in
     the FIFO, and delete the other constituent tokens.  */
  if (checkpoint > 0)
    {
      current.value.sval.ptr
	= (const char *) obstack_copy0 (&cpstate->expansion_obstack,
					current.value.sval.ptr,
					current.value.sval.length);

      token_fifo[0] = current;
      if (checkpoint > 1)
	token_fifo.erase (token_fifo.begin () + 1,
			  token_fifo.begin () + checkpoint);
    }

 do_pop:
  current = token_fifo[0];
  token_fifo.erase (token_fifo.begin ());
  yylval = current.value;
  return current.token;
}

int
c_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;

  c_parse_state cstate;
  scoped_restore cstate_restore = make_scoped_restore (&cpstate, &cstate);

  gdb::unique_xmalloc_ptr<struct macro_scope> macro_scope;

  if (expression_context_block)
    macro_scope = sal_macro_scope (find_pc_line (expression_context_pc, 0));
  else
    macro_scope = default_macro_scope ();
  if (! macro_scope)
    macro_scope = user_macro_scope ();

  scoped_restore restore_macro_scope
    = make_scoped_restore (&expression_macro_scope, macro_scope.get ());

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);

  /* Initialize some state used by the lexer.  */
  last_was_structop = false;
  saw_name_at_eof = 0;

  token_fifo.clear ();
  popping = 0;
  name_obstack.clear ();

  return yyparse ();
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
    case DOLLAR_VARIABLE:
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

static void
yyerror (const char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), msg, lexptr);
}
