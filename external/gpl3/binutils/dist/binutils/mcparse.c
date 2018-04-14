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
#line 1 "mcparse.y" /* yacc.c:339  */
 /* mcparse.y -- parser for Windows mc files
  Copyright (C) 2007-2018 Free Software Foundation, Inc.

  Parser for Windows mc files
  Written by Kai Tietz, Onevision.

  This file is part of GNU Binutils.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
  02110-1301, USA.  */

/* This is a parser for Windows rc files.  It is based on the parser
   by Gunther Ebert <gunther.ebert@ixos-leipzig.de>.  */

#include "sysdep.h"
#include "bfd.h"
#include "bucomm.h"
#include "libiberty.h"
#include "windmc.h"
#include "safe-ctype.h"

static rc_uint_type mc_last_id = 0;
static rc_uint_type mc_sefa_val = 0;
static unichar *mc_last_symbol = NULL;
static const mc_keyword *mc_cur_severity = NULL;
static const mc_keyword *mc_cur_facility = NULL;
static mc_node *cur_node = NULL;


#line 108 "mcparse.c" /* yacc.c:339  */

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

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_MCPARSE_H_INCLUDED
# define YY_YY_MCPARSE_H_INCLUDED
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
    NL = 258,
    MCIDENT = 259,
    MCFILENAME = 260,
    MCLINE = 261,
    MCCOMMENT = 262,
    MCTOKEN = 263,
    MCENDLINE = 264,
    MCLANGUAGENAMES = 265,
    MCFACILITYNAMES = 266,
    MCSEVERITYNAMES = 267,
    MCOUTPUTBASE = 268,
    MCMESSAGEIDTYPEDEF = 269,
    MCLANGUAGE = 270,
    MCMESSAGEID = 271,
    MCSEVERITY = 272,
    MCFACILITY = 273,
    MCSYMBOLICNAME = 274,
    MCNUMBER = 275
  };
#endif
/* Tokens.  */
#define NL 258
#define MCIDENT 259
#define MCFILENAME 260
#define MCLINE 261
#define MCCOMMENT 262
#define MCTOKEN 263
#define MCENDLINE 264
#define MCLANGUAGENAMES 265
#define MCFACILITYNAMES 266
#define MCSEVERITYNAMES 267
#define MCOUTPUTBASE 268
#define MCMESSAGEIDTYPEDEF 269
#define MCLANGUAGE 270
#define MCMESSAGEID 271
#define MCSEVERITY 272
#define MCFACILITY 273
#define MCSYMBOLICNAME 274
#define MCNUMBER 275

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 44 "mcparse.y" /* yacc.c:355  */

  rc_uint_type ival;
  unichar *ustr;
  const mc_keyword *tok;
  mc_node *nod;

#line 195 "mcparse.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_MCPARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 212 "mcparse.c" /* yacc.c:358  */

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

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
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
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
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
#define YYLAST   114

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  29
/* YYNRULES -- Number of rules.  */
#define YYNRULES  82
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  125

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   275

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
      22,    23,     2,    25,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    24,     2,
       2,    21,     2,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    66,    66,    69,    71,    73,    74,    75,    80,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,   102,   106,   110,   117,   118,   119,   123,   127,
     128,   132,   133,   134,   138,   142,   143,   147,   148,   149,
     153,   157,   158,   159,   160,   165,   168,   172,   177,   176,
     189,   190,   191,   195,   198,   202,   206,   211,   218,   224,
     230,   238,   246,   254,   261,   262,   266,   276,   280,   292,
     293,   296,   297,   311,   315,   320,   325,   330,   337,   338,
     342,   346,   350
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NL", "MCIDENT", "MCFILENAME", "MCLINE",
  "MCCOMMENT", "MCTOKEN", "MCENDLINE", "MCLANGUAGENAMES",
  "MCFACILITYNAMES", "MCSEVERITYNAMES", "MCOUTPUTBASE",
  "MCMESSAGEIDTYPEDEF", "MCLANGUAGE", "MCMESSAGEID", "MCSEVERITY",
  "MCFACILITY", "MCSYMBOLICNAME", "MCNUMBER", "'='", "'('", "')'", "':'",
  "'+'", "$accept", "input", "entities", "entity", "global_section",
  "severitymaps", "severitymap", "facilitymaps", "facilitymap", "langmaps",
  "langmap", "alias_name", "message", "$@1", "id", "vid", "sefasy_def",
  "severity", "facility", "symbol", "lang_entities", "lang_entity",
  "lines", "comments", "lang", "token", "lex_want_nl", "lex_want_line",
  "lex_want_filename", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,    61,    40,    41,    58,    43
};
# endif

#define YYPACT_NINF -34

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-34)))

#define YYTABLE_NINF -83

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
     -34,    62,    70,   -34,   -34,   -34,    15,    22,    30,   -15,
      34,    37,   -34,   -34,   -34,   -34,    56,   -34,    10,   -34,
      12,   -34,    20,    25,   -34,    52,   -34,     0,    80,   -34,
     -34,    71,   -34,    84,   -34,    86,   -34,   -34,   -34,   -34,
     -34,    45,   -34,     1,    68,    74,    76,   -34,   -34,   -34,
     -34,   -34,   -34,     4,   -34,    38,   -34,     6,   -34,    39,
     -34,    29,   -34,    40,   -34,   -34,    93,    94,    99,    43,
      76,   -34,   -34,   -34,   -34,   -34,   -34,    46,   -34,   -34,
     -34,   -34,    47,   -34,   -34,   -34,   -34,    49,   -34,   -34,
     -34,   -34,    83,   -34,     3,   -34,     2,   -34,    81,   -34,
      81,    92,   -34,   -34,    48,   -34,    82,    72,   -34,   -34,
     -34,   104,   105,   108,   -34,   -34,   -34,    73,   -34,   -34,
     -34,   -34,   -34,   -34,   -34
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     1,     8,    71,     0,     0,     0,     0,
       0,     0,     4,     5,     6,    57,     7,    16,     0,    20,
       0,    12,     0,     0,    24,     0,    52,     0,    48,    72,
      15,     0,    19,     0,    11,     0,    21,    23,    22,    51,
      54,     0,    50,     0,     0,     0,     0,    58,    59,    60,
      39,    78,    79,     0,    37,     0,    33,     0,    31,     0,
      27,     0,    25,     0,    56,    55,     0,     0,     0,     0,
      49,    64,    81,    14,    13,    38,    44,     0,    18,    17,
      32,    36,     0,    10,     9,    26,    30,     0,    61,    62,
      63,    77,     0,    65,     0,    43,     0,    35,    45,    29,
      45,     0,    69,    67,     0,    42,     0,     0,    34,    28,
      76,    78,    79,     0,    70,    68,    66,     0,    47,    46,
      74,    73,    75,    41,    40
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -34,   -34,   -34,   -34,   -34,   -34,    50,   -34,    53,   -34,
      59,    13,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,
     -34,    44,   -34,   -34,   -34,   -33,   -34,   -34,   -34
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,     2,    12,    13,    61,    62,    57,    58,    53,
      54,   108,    14,    46,    15,    42,    28,    47,    48,    49,
      70,    71,   104,    16,    72,    55,    92,    94,   106
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      59,    39,    63,   105,   102,    73,    23,    78,    51,   103,
      51,    30,    52,    32,    52,   -53,    17,   -53,   -53,   -53,
      40,    34,    66,    19,    59,    41,   -82,    74,    63,    79,
      83,    21,    31,    51,    33,    24,    18,    52,    26,    76,
      81,    86,    35,    20,    91,    36,    64,    95,    97,   114,
      99,    22,    84,    37,   115,    25,    38,   116,    27,    77,
      82,    87,     3,    29,   -80,    65,    96,    98,   113,   100,
      -2,     4,    50,   118,   123,    51,   119,     5,   124,    52,
       6,     7,     8,     9,    10,    56,    11,    60,    51,    67,
      51,    69,    52,   110,    52,    68,   111,    43,    44,    45,
     112,    88,    89,    90,   101,   107,   117,   120,   121,   122,
      80,    85,    75,   109,    93
};

static const yytype_uint8 yycheck[] =
{
      33,     1,    35,     1,     1,     1,    21,     1,     4,     6,
       4,     1,     8,     1,     8,    15,     1,    17,    18,    19,
      20,     1,    21,     1,    57,    25,    24,    23,    61,    23,
       1,     1,    22,     4,    22,     1,    21,     8,     1,     1,
       1,     1,    22,    21,     1,    20,     1,     1,     1,     1,
       1,    21,    23,     1,     6,    21,     4,     9,    21,    21,
      21,    21,     0,     7,    21,    20,    20,    20,   101,    20,
       0,     1,     1,     1,     1,     4,     4,     7,     5,     8,
      10,    11,    12,    13,    14,     1,    16,     1,     4,    21,
       4,    15,     8,     1,     8,    21,     4,    17,    18,    19,
       8,     8,     8,     4,    21,    24,    24,     3,     3,     1,
      57,    61,    53,   100,    70
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    27,    28,     0,     1,     7,    10,    11,    12,    13,
      14,    16,    29,    30,    38,    40,    49,     1,    21,     1,
      21,     1,    21,    21,     1,    21,     1,    21,    42,     7,
       1,    22,     1,    22,     1,    22,    20,     1,     4,     1,
      20,    25,    41,    17,    18,    19,    39,    43,    44,    45,
       1,     4,     8,    35,    36,    51,     1,    33,    34,    51,
       1,    31,    32,    51,     1,    20,    21,    21,    21,    15,
      46,    47,    50,     1,    23,    36,     1,    21,     1,    23,
      34,     1,    21,     1,    23,    32,     1,    21,     8,     8,
       4,     1,    52,    47,    53,     1,    20,     1,    20,     1,
      20,    21,     1,     6,    48,     1,    54,    24,    37,    37,
       1,     4,     8,    51,     1,     6,     9,    24,     1,     4,
       3,     3,     1,     1,     5
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    26,    27,    28,    28,    29,    29,    29,    29,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    31,    31,    31,    32,    32,
      32,    33,    33,    33,    34,    34,    34,    35,    35,    35,
      36,    36,    36,    36,    36,    37,    37,    37,    39,    38,
      40,    40,    40,    41,    41,    41,    41,    42,    42,    42,
      42,    43,    44,    45,    46,    46,    47,    48,    48,    48,
      48,    49,    49,    50,    50,    50,    50,    50,    51,    51,
      52,    53,    54
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     2,     1,     1,     1,     1,     5,
       5,     3,     2,     5,     5,     3,     2,     5,     5,     3,
       2,     3,     3,     3,     2,     1,     2,     1,     4,     3,
       2,     1,     2,     1,     4,     3,     2,     1,     2,     1,
       6,     6,     4,     3,     2,     0,     2,     2,     0,     4,
       3,     3,     2,     0,     1,     2,     2,     0,     2,     2,
       2,     3,     3,     3,     1,     2,     4,     1,     2,     1,
       2,     1,     2,     5,     5,     5,     4,     2,     1,     1,
       0,     0,     0
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
       to reallocate them elsewhere.  */

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
        /* Give user a chance to reallocate the stack.  Use copies of
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
        case 7:
#line 76 "mcparse.y" /* yacc.c:1646  */
    {
	    cur_node = mc_add_node ();
	    cur_node->user_text = (yyvsp[0].ustr);
	  }
#line 1379 "mcparse.c" /* yacc.c:1646  */
    break;

  case 8:
#line 80 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("syntax error"); }
#line 1385 "mcparse.c" /* yacc.c:1646  */
    break;

  case 10:
#line 85 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing ')' in SeverityNames"); }
#line 1391 "mcparse.c" /* yacc.c:1646  */
    break;

  case 11:
#line 86 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing '(' in SeverityNames"); }
#line 1397 "mcparse.c" /* yacc.c:1646  */
    break;

  case 12:
#line 87 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing '=' for SeverityNames"); }
#line 1403 "mcparse.c" /* yacc.c:1646  */
    break;

  case 14:
#line 89 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing ')' in LanguageNames"); }
#line 1409 "mcparse.c" /* yacc.c:1646  */
    break;

  case 15:
#line 90 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing '(' in LanguageNames"); }
#line 1415 "mcparse.c" /* yacc.c:1646  */
    break;

  case 16:
#line 91 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing '=' for LanguageNames"); }
#line 1421 "mcparse.c" /* yacc.c:1646  */
    break;

  case 18:
#line 93 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing ')' in FacilityNames"); }
#line 1427 "mcparse.c" /* yacc.c:1646  */
    break;

  case 19:
#line 94 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing '(' in FacilityNames"); }
#line 1433 "mcparse.c" /* yacc.c:1646  */
    break;

  case 20:
#line 95 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing '=' for FacilityNames"); }
#line 1439 "mcparse.c" /* yacc.c:1646  */
    break;

  case 21:
#line 97 "mcparse.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[0].ival) != 10 && (yyvsp[0].ival) != 16)
	      mc_fatal ("OutputBase allows 10 or 16 as value");
	    mcset_out_values_are_decimal = ((yyvsp[0].ival) == 10 ? 1 : 0);
	  }
#line 1449 "mcparse.c" /* yacc.c:1646  */
    break;

  case 22:
#line 103 "mcparse.y" /* yacc.c:1646  */
    {
	    mcset_msg_id_typedef = (yyvsp[0].ustr);
	  }
#line 1457 "mcparse.c" /* yacc.c:1646  */
    break;

  case 23:
#line 107 "mcparse.y" /* yacc.c:1646  */
    {
	    mc_fatal ("MessageIdTypedef expects an identifier");
	  }
#line 1465 "mcparse.c" /* yacc.c:1646  */
    break;

  case 24:
#line 111 "mcparse.y" /* yacc.c:1646  */
    {
	    mc_fatal ("missing '=' for MessageIdTypedef");
	  }
#line 1473 "mcparse.c" /* yacc.c:1646  */
    break;

  case 27:
#line 119 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("severity ident missing"); }
#line 1479 "mcparse.c" /* yacc.c:1646  */
    break;

  case 28:
#line 124 "mcparse.y" /* yacc.c:1646  */
    {
	    mc_add_keyword ((yyvsp[-3].ustr), MCTOKEN, "severity", (yyvsp[-1].ival), (yyvsp[0].ustr));
	  }
#line 1487 "mcparse.c" /* yacc.c:1646  */
    break;

  case 29:
#line 127 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("severity number missing"); }
#line 1493 "mcparse.c" /* yacc.c:1646  */
    break;

  case 30:
#line 128 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("severity missing '='"); }
#line 1499 "mcparse.c" /* yacc.c:1646  */
    break;

  case 33:
#line 134 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing ident in FacilityNames"); }
#line 1505 "mcparse.c" /* yacc.c:1646  */
    break;

  case 34:
#line 139 "mcparse.y" /* yacc.c:1646  */
    {
	    mc_add_keyword ((yyvsp[-3].ustr), MCTOKEN, "facility", (yyvsp[-1].ival), (yyvsp[0].ustr));
	  }
#line 1513 "mcparse.c" /* yacc.c:1646  */
    break;

  case 35:
#line 142 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("facility number missing"); }
#line 1519 "mcparse.c" /* yacc.c:1646  */
    break;

  case 36:
#line 143 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("facility missing '='"); }
#line 1525 "mcparse.c" /* yacc.c:1646  */
    break;

  case 39:
#line 149 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing ident in LanguageNames"); }
#line 1531 "mcparse.c" /* yacc.c:1646  */
    break;

  case 40:
#line 154 "mcparse.y" /* yacc.c:1646  */
    {
	    mc_add_keyword ((yyvsp[-5].ustr), MCTOKEN, "language", (yyvsp[-3].ival), (yyvsp[0].ustr));
	  }
#line 1539 "mcparse.c" /* yacc.c:1646  */
    break;

  case 41:
#line 157 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing filename in LanguageNames"); }
#line 1545 "mcparse.c" /* yacc.c:1646  */
    break;

  case 42:
#line 158 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing ':' in LanguageNames"); }
#line 1551 "mcparse.c" /* yacc.c:1646  */
    break;

  case 43:
#line 159 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing language code in LanguageNames"); }
#line 1557 "mcparse.c" /* yacc.c:1646  */
    break;

  case 44:
#line 160 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing '=' for LanguageNames"); }
#line 1563 "mcparse.c" /* yacc.c:1646  */
    break;

  case 45:
#line 165 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.ustr) = NULL;
	  }
#line 1571 "mcparse.c" /* yacc.c:1646  */
    break;

  case 46:
#line 169 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.ustr) = (yyvsp[0].ustr);
	  }
#line 1579 "mcparse.c" /* yacc.c:1646  */
    break;

  case 47:
#line 172 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("illegal token in identifier"); (yyval.ustr) = NULL; }
#line 1585 "mcparse.c" /* yacc.c:1646  */
    break;

  case 48:
#line 177 "mcparse.y" /* yacc.c:1646  */
    {
	    cur_node = mc_add_node ();
	    cur_node->symbol = mc_last_symbol;
	    cur_node->facility = mc_cur_facility;
	    cur_node->severity = mc_cur_severity;
	    cur_node->id = ((yyvsp[-1].ival) & 0xffffUL);
	    cur_node->vid = ((yyvsp[-1].ival) & 0xffffUL) | mc_sefa_val;
	    mc_last_id = (yyvsp[-1].ival);
	  }
#line 1599 "mcparse.c" /* yacc.c:1646  */
    break;

  case 50:
#line 189 "mcparse.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].ival); }
#line 1605 "mcparse.c" /* yacc.c:1646  */
    break;

  case 51:
#line 190 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing number in MessageId"); (yyval.ival) = 0; }
#line 1611 "mcparse.c" /* yacc.c:1646  */
    break;

  case 52:
#line 191 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing '=' for MessageId"); (yyval.ival) = 0; }
#line 1617 "mcparse.c" /* yacc.c:1646  */
    break;

  case 53:
#line 195 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.ival) = ++mc_last_id;
	  }
#line 1625 "mcparse.c" /* yacc.c:1646  */
    break;

  case 54:
#line 199 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.ival) = (yyvsp[0].ival);
	  }
#line 1633 "mcparse.c" /* yacc.c:1646  */
    break;

  case 55:
#line 203 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.ival) = mc_last_id + (yyvsp[0].ival);
	  }
#line 1641 "mcparse.c" /* yacc.c:1646  */
    break;

  case 56:
#line 206 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing number after MessageId '+'"); }
#line 1647 "mcparse.c" /* yacc.c:1646  */
    break;

  case 57:
#line 211 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.ival) = 0;
	    mc_sefa_val = (mcset_custom_bit ? 1 : 0) << 29;
	    mc_last_symbol = NULL;
	    mc_cur_severity = NULL;
	    mc_cur_facility = NULL;
	  }
#line 1659 "mcparse.c" /* yacc.c:1646  */
    break;

  case 58:
#line 219 "mcparse.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[-1].ival) & 1)
	      mc_warn (_("duplicate definition of Severity"));
	    (yyval.ival) = (yyvsp[-1].ival) | 1;
	  }
#line 1669 "mcparse.c" /* yacc.c:1646  */
    break;

  case 59:
#line 225 "mcparse.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[-1].ival) & 2)
	      mc_warn (_("duplicate definition of Facility"));
	    (yyval.ival) = (yyvsp[-1].ival) | 2;
	  }
#line 1679 "mcparse.c" /* yacc.c:1646  */
    break;

  case 60:
#line 231 "mcparse.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[-1].ival) & 4)
	      mc_warn (_("duplicate definition of SymbolicName"));
	    (yyval.ival) = (yyvsp[-1].ival) | 4;
	  }
#line 1689 "mcparse.c" /* yacc.c:1646  */
    break;

  case 61:
#line 239 "mcparse.y" /* yacc.c:1646  */
    {
	    mc_sefa_val &= ~ (0x3UL << 30);
	    mc_sefa_val |= (((yyvsp[0].tok)->nval & 0x3UL) << 30);
	    mc_cur_severity = (yyvsp[0].tok);
	  }
#line 1699 "mcparse.c" /* yacc.c:1646  */
    break;

  case 62:
#line 247 "mcparse.y" /* yacc.c:1646  */
    {
	    mc_sefa_val &= ~ (0xfffUL << 16);
	    mc_sefa_val |= (((yyvsp[0].tok)->nval & 0xfffUL) << 16);
	    mc_cur_facility = (yyvsp[0].tok);
	  }
#line 1709 "mcparse.c" /* yacc.c:1646  */
    break;

  case 63:
#line 255 "mcparse.y" /* yacc.c:1646  */
    {
	  mc_last_symbol = (yyvsp[0].ustr);
	}
#line 1717 "mcparse.c" /* yacc.c:1646  */
    break;

  case 66:
#line 267 "mcparse.y" /* yacc.c:1646  */
    {
	    mc_node_lang *h;
	    h = mc_add_node_lang (cur_node, (yyvsp[-3].tok), cur_node->vid);
	    h->message = (yyvsp[-1].ustr);
	    if (mcset_max_message_length != 0 && unichar_len (h->message) > mcset_max_message_length)
	      mc_warn ("message length to long");
	  }
#line 1729 "mcparse.c" /* yacc.c:1646  */
    break;

  case 67:
#line 277 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.ustr) = (yyvsp[0].ustr);
	  }
#line 1737 "mcparse.c" /* yacc.c:1646  */
    break;

  case 68:
#line 281 "mcparse.y" /* yacc.c:1646  */
    {
	    unichar *h;
	    rc_uint_type l1,l2;
	    l1 = unichar_len ((yyvsp[-1].ustr));
	    l2 = unichar_len ((yyvsp[0].ustr));
	    h = (unichar *) res_alloc ((l1 + l2 + 1) * sizeof (unichar));
	    if (l1) memcpy (h, (yyvsp[-1].ustr), l1 * sizeof (unichar));
	    if (l2) memcpy (&h[l1], (yyvsp[0].ustr), l2 * sizeof (unichar));
	    h[l1 + l2] = 0;
	    (yyval.ustr) = h;
	  }
#line 1753 "mcparse.c" /* yacc.c:1646  */
    break;

  case 69:
#line 292 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing end of message text"); (yyval.ustr) = NULL; }
#line 1759 "mcparse.c" /* yacc.c:1646  */
    break;

  case 70:
#line 293 "mcparse.y" /* yacc.c:1646  */
    { mc_fatal ("missing end of message text"); (yyval.ustr) = (yyvsp[-1].ustr); }
#line 1765 "mcparse.c" /* yacc.c:1646  */
    break;

  case 71:
#line 296 "mcparse.y" /* yacc.c:1646  */
    { (yyval.ustr) = (yyvsp[0].ustr); }
#line 1771 "mcparse.c" /* yacc.c:1646  */
    break;

  case 72:
#line 298 "mcparse.y" /* yacc.c:1646  */
    {
	    unichar *h;
	    rc_uint_type l1,l2;
	    l1 = unichar_len ((yyvsp[-1].ustr));
	    l2 = unichar_len ((yyvsp[0].ustr));
	    h = (unichar *) res_alloc ((l1 + l2 + 1) * sizeof (unichar));
	    if (l1) memcpy (h, (yyvsp[-1].ustr), l1 * sizeof (unichar));
	    if (l2) memcpy (&h[l1], (yyvsp[0].ustr), l2 * sizeof (unichar));
	    h[l1 + l2] = 0;
	    (yyval.ustr) = h;
	  }
#line 1787 "mcparse.c" /* yacc.c:1646  */
    break;

  case 73:
#line 312 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.tok) = (yyvsp[-1].tok);
	  }
#line 1795 "mcparse.c" /* yacc.c:1646  */
    break;

  case 74:
#line 316 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.tok) = NULL;
	    mc_fatal (_("undeclared language identifier"));
	  }
#line 1804 "mcparse.c" /* yacc.c:1646  */
    break;

  case 75:
#line 321 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.tok) = NULL;
	    mc_fatal ("missing newline after Language");
	  }
#line 1813 "mcparse.c" /* yacc.c:1646  */
    break;

  case 76:
#line 326 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.tok) = NULL;
	    mc_fatal ("missing ident for Language");
	  }
#line 1822 "mcparse.c" /* yacc.c:1646  */
    break;

  case 77:
#line 331 "mcparse.y" /* yacc.c:1646  */
    {
	    (yyval.tok) = NULL;
	    mc_fatal ("missing '=' for Language");
	  }
#line 1831 "mcparse.c" /* yacc.c:1646  */
    break;

  case 78:
#line 337 "mcparse.y" /* yacc.c:1646  */
    { (yyval.ustr) = (yyvsp[0].ustr); }
#line 1837 "mcparse.c" /* yacc.c:1646  */
    break;

  case 79:
#line 338 "mcparse.y" /* yacc.c:1646  */
    { (yyval.ustr) = (yyvsp[0].tok)->usz; }
#line 1843 "mcparse.c" /* yacc.c:1646  */
    break;

  case 80:
#line 342 "mcparse.y" /* yacc.c:1646  */
    { mclex_want_nl = 1; }
#line 1849 "mcparse.c" /* yacc.c:1646  */
    break;

  case 81:
#line 346 "mcparse.y" /* yacc.c:1646  */
    { mclex_want_line = 1; }
#line 1855 "mcparse.c" /* yacc.c:1646  */
    break;

  case 82:
#line 350 "mcparse.y" /* yacc.c:1646  */
    { mclex_want_filename = 1; }
#line 1861 "mcparse.c" /* yacc.c:1646  */
    break;


#line 1865 "mcparse.c" /* yacc.c:1646  */
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
#line 353 "mcparse.y" /* yacc.c:1906  */


/* Something else.  */
