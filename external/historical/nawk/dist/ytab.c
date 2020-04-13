/* A Bison parser, made by GNU Bison 3.4.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.4.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 25 "awkgram.y"

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <stdio.h>
#include <string.h>
#include "awk.h"

void checkdup(Node *list, Cell *item);
int yywrap(void) { return(1); }

Node	*beginloc = 0;
Node	*endloc = 0;
bool	infunc	= false;	/* = true if in arglist or body of func */
int	inloop	= 0;	/* >= 1 if in while, for, do; can't be bool, since loops can next */
char	*curfname = 0;	/* current function name */
Node	*arglist = 0;	/* list of args for current function */

#line 90 "awkgram.tab.c"

# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_AWKGRAM_TAB_H_INCLUDED
# define YY_YY_AWKGRAM_TAB_H_INCLUDED
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
    FIRSTTOKEN = 258,
    PROGRAM = 259,
    PASTAT = 260,
    PASTAT2 = 261,
    XBEGIN = 262,
    XEND = 263,
    NL = 264,
    ARRAY = 265,
    MATCH = 266,
    NOTMATCH = 267,
    MATCHOP = 268,
    FINAL = 269,
    DOT = 270,
    ALL = 271,
    CCL = 272,
    NCCL = 273,
    CHAR = 274,
    OR = 275,
    STAR = 276,
    QUEST = 277,
    PLUS = 278,
    EMPTYRE = 279,
    ZERO = 280,
    AND = 281,
    BOR = 282,
    APPEND = 283,
    EQ = 284,
    GE = 285,
    GT = 286,
    LE = 287,
    LT = 288,
    NE = 289,
    IN = 290,
    ARG = 291,
    BLTIN = 292,
    BREAK = 293,
    CLOSE = 294,
    CONTINUE = 295,
    DELETE = 296,
    DO = 297,
    EXIT = 298,
    FOR = 299,
    FUNC = 300,
    GENSUB = 301,
    SUB = 302,
    GSUB = 303,
    IF = 304,
    INDEX = 305,
    LSUBSTR = 306,
    MATCHFCN = 307,
    NEXT = 308,
    NEXTFILE = 309,
    ADD = 310,
    MINUS = 311,
    MULT = 312,
    DIVIDE = 313,
    MOD = 314,
    ASSIGN = 315,
    ASGNOP = 316,
    ADDEQ = 317,
    SUBEQ = 318,
    MULTEQ = 319,
    DIVEQ = 320,
    MODEQ = 321,
    POWEQ = 322,
    PRINT = 323,
    PRINTF = 324,
    SPRINTF = 325,
    ELSE = 326,
    INTEST = 327,
    CONDEXPR = 328,
    POSTINCR = 329,
    PREINCR = 330,
    POSTDECR = 331,
    PREDECR = 332,
    VAR = 333,
    IVAR = 334,
    VARNF = 335,
    CALL = 336,
    NUMBER = 337,
    STRING = 338,
    REGEXPR = 339,
    GETLINE = 340,
    RETURN = 341,
    SPLIT = 342,
    SUBSTR = 343,
    WHILE = 344,
    CAT = 345,
    NOT = 346,
    UMINUS = 347,
    UPLUS = 348,
    POWER = 349,
    DECR = 350,
    INCR = 351,
    INDIRECT = 352,
    LASTTOKEN = 353
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 45 "awkgram.y"

	Node	*p;
	Cell	*cp;
	int	i;
	char	*s;

#line 239 "awkgram.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_AWKGRAM_TAB_H_INCLUDED  */



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
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
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
#  define YYSIZE_T unsigned
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

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
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


#define YY_ASSERT(E) ((void) (0 && (E)))

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
#define YYFINAL  8
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   5325

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  115
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  50
/* YYNRULES -- Number of rules.  */
#define YYNRULES  191
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  390

#define YYUNDEFTOK  2
#define YYMAXUTOK   353

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,   106,     2,     2,
      12,    16,   105,   103,    10,   104,     2,    15,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    96,    14,
       2,     2,     2,    95,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    18,     2,    19,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    11,    13,    17,     2,     2,     2,     2,
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
       5,     6,     7,     8,     9,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      97,    98,    99,   100,   101,   102,   107,   108,   109,   110,
     111,   112,   113,   114
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   103,   103,   105,   109,   109,   113,   113,   117,   117,
     121,   121,   125,   125,   129,   129,   131,   131,   133,   133,
     138,   139,   143,   147,   147,   151,   151,   155,   156,   160,
     161,   166,   167,   171,   172,   176,   180,   181,   182,   183,
     184,   185,   187,   189,   189,   194,   195,   199,   200,   204,
     205,   207,   209,   211,   212,   217,   218,   219,   220,   221,
     225,   226,   228,   230,   232,   233,   234,   235,   236,   237,
     238,   239,   244,   245,   246,   249,   252,   253,   254,   258,
     259,   263,   264,   268,   269,   270,   274,   274,   278,   278,
     278,   278,   282,   282,   286,   288,   292,   292,   296,   296,
     300,   303,   306,   309,   310,   311,   312,   313,   317,   318,
     322,   324,   326,   326,   326,   328,   329,   330,   331,   332,
     333,   334,   337,   340,   341,   342,   343,   343,   344,   348,
     349,   353,   353,   357,   358,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   384,   390,   392,   398,
     399,   400,   401,   402,   404,   407,   408,   410,   415,   416,
     418,   420,   422,   423,   424,   426,   431,   433,   438,   440,
     442,   446,   447,   448,   449,   453,   454,   455,   461,   462,
     463,   468
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "FIRSTTOKEN", "PROGRAM", "PASTAT",
  "PASTAT2", "XBEGIN", "XEND", "NL", "','", "'{'", "'('", "'|'", "';'",
  "'/'", "')'", "'}'", "'['", "']'", "ARRAY", "MATCH", "NOTMATCH",
  "MATCHOP", "FINAL", "DOT", "ALL", "CCL", "NCCL", "CHAR", "OR", "STAR",
  "QUEST", "PLUS", "EMPTYRE", "ZERO", "AND", "BOR", "APPEND", "EQ", "GE",
  "GT", "LE", "LT", "NE", "IN", "ARG", "BLTIN", "BREAK", "CLOSE",
  "CONTINUE", "DELETE", "DO", "EXIT", "FOR", "FUNC", "GENSUB", "SUB",
  "GSUB", "IF", "INDEX", "LSUBSTR", "MATCHFCN", "NEXT", "NEXTFILE", "ADD",
  "MINUS", "MULT", "DIVIDE", "MOD", "ASSIGN", "ASGNOP", "ADDEQ", "SUBEQ",
  "MULTEQ", "DIVEQ", "MODEQ", "POWEQ", "PRINT", "PRINTF", "SPRINTF",
  "ELSE", "INTEST", "CONDEXPR", "POSTINCR", "PREINCR", "POSTDECR",
  "PREDECR", "VAR", "IVAR", "VARNF", "CALL", "NUMBER", "STRING", "REGEXPR",
  "'?'", "':'", "GETLINE", "RETURN", "SPLIT", "SUBSTR", "WHILE", "CAT",
  "'+'", "'-'", "'*'", "'%'", "NOT", "UMINUS", "UPLUS", "POWER", "DECR",
  "INCR", "INDIRECT", "LASTTOKEN", "$accept", "program", "and", "bor",
  "comma", "do", "else", "for", "$@1", "$@2", "$@3", "funcname", "if",
  "lbrace", "nl", "opt_nl", "opt_pst", "opt_simple_stmt", "pas", "pa_pat",
  "pa_stat", "$@4", "pa_stats", "patlist", "ppattern", "pattern", "plist",
  "pplist", "prarg", "print", "pst", "rbrace", "re", "reg_expr", "$@5",
  "rparen", "simple_stmt", "st", "stmt", "$@6", "$@7", "$@8", "stmtlist",
  "subop", "string", "term", "var", "varlist", "varname", "while", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
      44,   123,    40,   124,    59,    47,    41,   125,    91,    93,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,    63,    58,   340,   341,   342,
     343,   344,   345,    43,    45,    42,    37,   346,   347,   348,
     349,   350,   351,   352,   353
};
# endif

#define YYPACT_NINF -341

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-341)))

#define YYTABLE_NINF -32

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-32)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     822,  -341,  -341,  -341,    49,  1756,  -341,     2,  -341,    40,
      40,  -341,  4836,  -341,  -341,    41,  5212,   -57,    70,  -341,
    -341,    85,    99,   106,  -341,  -341,  -341,   114,  -341,  -341,
     -20,   117,   129,  5212,  5212,  4895,    88,    88,  5212,   927,
      74,  -341,    95,  4104,  -341,  -341,   134,    65,    -3,   -34,
     144,  -341,  -341,   927,   927,  2369,    38,    76,  4647,  4836,
    5212,    -3,    16,  -341,  -341,   160,  4836,  4836,  4836,  4836,
    4706,  5212,   147,  4836,  4836,    82,    82,  -341,    82,  -341,
    -341,  -341,  -341,  -341,   195,   165,   165,   -14,  -341,  1911,
     190,   194,   165,   165,  -341,  -341,  1911,   208,   198,  -341,
    1559,   927,  4104,  4954,   165,  -341,   997,  -341,   195,   927,
    1756,   124,  4836,  -341,  -341,  4836,  4836,  4836,  4836,  4836,
    4836,   -14,  4836,  1970,  2029,    -3,  4836,  -341,  5013,  5212,
    5212,  5212,  5212,  5212,  4836,  -341,  -341,  4836,  1067,  1137,
    -341,  -341,  2088,   177,  2088,   217,  -341,    73,  4104,  3078,
     135,  2985,   214,  2985,  2985,   132,  -341,   139,    -3,  5212,
    2985,  2985,  -341,   226,  -341,   195,   226,  -341,  -341,   221,
    1852,  -341,  1628,  4836,  -341,  -341,  1852,  -341,  4836,  -341,
    1559,   163,  1207,  4836,  4515,   214,   118,  -341,    -3,    28,
    -341,  -341,  -341,  1559,  4836,  1277,  -341,    88,  4361,  -341,
    4361,  4361,  4361,  4361,  4361,  4361,  -341,  3171,  -341,  4279,
    -341,  4197,  2985,   214,  5212,    82,    15,    15,    82,    82,
      82,  4104,    46,  -341,  -341,  -341,  4104,   -14,  4104,  -341,
    -341,  2088,  -341,   159,  2088,  2088,  2088,  2088,  -341,  -341,
      -3,    20,  2088,  -341,  -341,  4836,  -341,   227,  -341,    17,
    3264,  -341,  3264,  -341,  -341,  1349,  -341,   231,   193,  5082,
     -14,  5082,  2147,  2206,    -3,  2265,  5212,  5212,  5212,  5082,
    -341,    40,  -341,  -341,  4836,  2088,  2088,    -3,  -341,  -341,
    4104,  -341,     4,   239,  2985,  2985,  3357,   238,  3450,   240,
     201,  2473,    54,   185,   -14,   239,   239,   154,  -341,  -341,
    -341,   213,  4836,  5153,  -341,  -341,  4433,  4777,  4588,  4515,
      -3,    -3,    -3,  4515,   927,  4104,  2577,  2681,  -341,  -341,
      40,  2088,  2088,  -341,  -341,  -341,  -341,  -341,  2088,  -341,
    2088,  -341,   195,  4836,   244,   250,   -14,   209,  5082,  1419,
    -341,     6,  -341,     6,   927,  2785,  2889,  3543,   247,  3636,
    1697,  3732,   239,  4836,  -341,   213,  4515,  -341,   255,   257,
    1489,  -341,  2088,  -341,  2088,  -341,  -341,  -341,   244,   195,
    1559,  3825,  -341,  -341,  -341,  3918,  4011,   239,  1697,  -341,
     165,  -341,  -341,  1559,   244,  -341,  -341,   239,  1559,  -341
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     3,    88,    89,     0,    33,     2,    30,     1,     0,
       0,    23,     0,    96,   189,   147,     0,     0,     0,   131,
     132,     0,     0,     0,   188,   183,   190,     0,   168,   133,
     162,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      36,    45,    29,    35,    77,    94,     0,   173,    78,   180,
     181,    90,    91,     0,     0,     0,     0,     0,     0,     0,
       0,   150,   180,    20,    21,     0,     0,     0,     0,     0,
       0,     0,   161,     0,     0,   143,   142,    95,   144,   151,
     152,   184,   107,    24,    27,     0,     0,     0,    10,     0,
       0,     0,     0,     0,    86,    87,     0,     0,   112,   117,
       0,     0,   106,    83,     0,   129,     0,   126,    27,     0,
      34,     0,     0,     4,     6,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    76,     0,   134,     0,     0,
       0,     0,     0,     0,     0,   153,   154,     0,     0,     0,
       8,   165,     0,     0,     0,     0,   145,     0,    47,     0,
     185,     0,    94,     0,     0,     0,   148,     0,   160,     0,
       0,     0,    25,    28,   128,    27,   108,   110,   111,   105,
       0,   116,     0,     0,   121,   122,     0,   124,     0,    11,
       0,   119,     0,     0,    81,    84,   103,    58,    59,   180,
     125,    40,   130,     0,     0,     0,    46,    75,    71,    70,
      64,    65,    66,    67,    68,    69,    72,     0,     5,    63,
       7,    62,     0,    94,     0,   139,   136,   137,   138,   140,
     141,    60,     0,    41,    42,     9,    79,     0,    80,    97,
     146,     0,   186,     0,     0,     0,     0,     0,   172,   149,
     159,     0,     0,    26,   109,     0,   115,     0,    32,   181,
       0,   123,     0,   113,    12,     0,    92,   120,     0,     0,
       0,     0,     0,     0,    57,     0,     0,     0,     0,     0,
     127,    38,    37,    74,     0,     0,     0,   135,   182,    73,
      48,    98,     0,    43,     0,     0,     0,    94,     0,    94,
       0,     0,     0,    27,     0,    22,   191,     0,    13,   118,
      93,    85,     0,    54,    53,    55,     0,    52,    51,    82,
     100,   101,   102,    49,     0,    61,     0,     0,   187,    99,
       0,     0,     0,   163,   164,   167,   166,   171,     0,   179,
       0,   104,    27,     0,     0,     0,     0,     0,     0,     0,
     175,     0,   174,     0,     0,     0,     0,     0,    94,     0,
       0,     0,    18,     0,    56,     0,    50,    39,     0,     0,
       0,   156,     0,   155,     0,   169,   170,   178,     0,    27,
       0,     0,   177,   176,    44,     0,     0,    16,     0,    19,
       0,   158,   157,     0,     0,   114,    17,    14,     0,    15
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -341,  -341,  -156,  -142,   411,  -341,  -341,  -341,  -341,  -341,
    -341,  -341,  -341,    -4,   -75,   -67,   224,  -340,  -341,    80,
     169,  -341,  -341,   -62,  -202,   542,  -179,  -341,  -341,  -341,
    -341,  -341,   -32,   -21,  -341,  -171,  -170,   -46,   375,  -341,
    -341,  -341,   -29,  -341,  -341,   243,   -16,  -341,     1,  -341
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     4,   123,   124,   231,    98,   255,    99,   388,   383,
     370,    65,   100,   101,   166,   164,     5,   247,     6,    40,
      41,   320,    42,   147,   184,   102,    56,   185,   186,   103,
       7,   257,    44,    45,    57,   283,   104,   167,   105,   180,
     297,   193,   106,    46,    47,    48,    49,   233,    50,   107
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      62,    39,   248,    77,   258,    53,    54,   155,   157,   163,
     368,    51,   128,   225,    72,   225,    52,    62,    62,    62,
      79,    80,    62,    71,   138,   139,    14,    62,   262,   225,
     128,    63,    14,   163,    64,   137,   109,   134,   384,    62,
     168,   194,   263,   171,    62,   152,   174,   175,   140,     8,
     177,    11,    14,    58,   143,    62,   140,   303,   190,   306,
     307,   308,   294,   309,   140,   278,    14,   313,    24,    25,
      26,   187,   182,   331,    24,   222,    26,   135,   136,   295,
     195,   296,    66,   140,   108,    11,    62,   189,   169,   230,
     163,   199,   318,    38,    24,    25,    26,    67,   244,   269,
     129,   130,   131,   132,     2,   213,    39,   133,    24,     3,
      26,    68,    62,    62,    62,    62,    62,    62,    69,    38,
     131,   132,   206,   337,   246,   133,    70,   135,   136,    73,
     251,   266,    62,    62,    14,    62,   356,    62,    62,   135,
     136,    74,   140,    62,    62,    62,   126,   262,   238,   140,
     262,   262,   262,   262,    62,   239,   267,   262,   127,   268,
      62,   263,   137,   352,   263,   263,   263,   263,    62,   140,
     145,   263,   150,   249,   162,   281,    24,    25,    26,   165,
     248,   273,    62,   292,    62,    62,    62,    62,    62,    62,
     159,    62,   133,    62,   162,    62,    62,   377,    62,   332,
     262,    38,   172,   140,   162,    62,   173,   179,   248,   301,
      62,   140,    62,   387,   263,   287,   289,   327,   163,   140,
     178,   197,   227,   232,   140,   355,   333,   187,   279,   187,
     187,   187,   229,   187,    62,   243,    62,   187,   304,   245,
     300,   293,   290,   189,   254,   189,   189,   189,   319,   189,
      62,    62,    62,   189,   324,   335,   326,   163,   336,    61,
     281,   305,   353,   366,    62,   350,   110,   314,    62,    62,
      62,   372,    62,   373,   271,    62,    75,    76,    78,   196,
       0,    81,     0,     0,     0,   339,   125,    62,     0,     0,
      62,    62,    62,    62,   163,   334,     0,    62,   125,    62,
      62,    62,   378,    78,     0,     0,   187,   348,     0,     0,
       0,     0,     0,     0,   158,   360,   344,     0,     0,     0,
       0,     0,   189,     0,     0,   358,     0,   359,     0,    62,
      62,    62,     0,    62,   385,    62,     0,   354,     0,     0,
      62,     0,     0,     0,     0,   125,   188,     0,     0,     0,
       0,     0,     0,     0,     0,    62,     0,     0,     0,    62,
      62,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   215,   216,   217,   218,   219,   220,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   125,   125,     0,   125,     0,   125,   125,     0,     0,
       0,     0,   240,   125,   125,     0,     0,     0,     0,     0,
       0,     0,     0,   125,     0,     0,     0,     0,     0,   125,
       0,     0,     0,     0,     0,     0,     0,   264,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   125,     0,   125,   125,   125,   125,   125,   125,     0,
     125,     0,   125,     0,   125,   125,     0,   277,     0,     0,
       0,     0,     0,     0,   125,     0,   142,   144,     0,   125,
       0,   125,     0,     0,     0,   181,     0,     0,     0,     0,
       0,   192,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   125,     0,   125,     0,     0,     0,     0,
       0,     0,   188,     0,   188,   188,   188,     0,   188,   310,
     311,   312,   188,   192,   192,     0,     0,     0,     0,     0,
       0,     0,     0,   125,     0,     0,     0,   125,   125,   125,
       0,   125,     0,     0,   125,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   264,    43,     0,   264,
     264,   264,   264,     0,    55,   253,   264,   192,   125,   125,
     125,     0,   234,   235,   236,   237,     0,     0,   270,     0,
     192,   241,   242,     0,     0,     0,     0,     0,     0,     0,
       0,   188,     0,     0,     0,     0,     0,     0,   125,   125,
     125,     0,   125,     0,   125,     0,   265,     0,     0,   264,
     148,   149,     0,     0,     0,     0,     0,     0,   151,   153,
     154,   148,   148,     0,   125,   160,   161,     0,   125,   125,
       0,     0,     0,   275,   276,     0,     0,     0,     0,     0,
     299,   170,     0,     0,     0,     0,     0,     0,   176,     0,
       0,     0,     0,     0,   282,     0,     0,     0,     0,     0,
       0,     0,    43,     0,   198,     0,     0,   200,   201,   202,
     203,   204,   205,     0,   207,   209,   211,     0,   212,   144,
       0,     0,     0,     0,     0,     0,   221,     0,     0,   148,
       0,     0,     0,     0,   226,     0,   228,     0,     0,     0,
       0,     0,     0,     0,     0,   321,   322,     0,     0,     0,
       0,   328,   330,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   192,   250,     0,     0,     0,     0,
     252,     0,     0,     0,     0,    55,     0,   341,   343,     0,
       0,     0,     0,     0,     0,   192,    43,     0,     0,     0,
       0,     0,     0,     0,     0,   379,     0,     0,   144,     0,
       0,     0,     0,     0,     0,     0,   362,   364,   386,     0,
       0,     0,     0,   389,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   280,     0,     0,   284,   285,   286,   288,
       0,     0,     0,     0,   291,     0,     0,   148,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   315,   316,   317,     0,
       0,     0,   -29,     1,     0,     0,     0,     0,     0,   -29,
     -29,     2,     0,   -29,   -29,     0,     3,   -29,     0,     0,
       0,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   345,   346,     0,     0,     0,   -29,   -29,
     347,   -29,   349,     0,     0,   351,     0,   -29,   -29,   -29,
     -29,     0,   -29,     0,   -29,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   371,     0,     0,     0,     0,
       0,     0,   -29,     0,   375,     0,   376,     0,     0,     0,
     -29,   -29,   -29,   -29,   -29,   -29,     0,     0,     0,   -29,
       0,   -29,   -29,     0,     0,   -29,   -29,     0,    82,   -29,
       0,     0,     0,   -29,   -29,   -29,    83,     0,    11,    12,
       0,    84,    13,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    14,    15,    85,    16,    86,    87,    88,
      89,    90,     0,    18,    19,    20,    91,    21,     0,    22,
      92,    93,     0,     0,     0,     0,     0,     0,    82,     0,
       0,     0,     0,     0,     0,    94,    95,    23,    11,    12,
       0,    84,    13,     0,   191,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,    96,    31,    32,    97,     0,
      33,    34,     0,     0,    35,     0,     0,     0,    36,    37,
      38,     0,     0,    14,    15,    85,    16,    86,    87,    88,
      89,    90,     0,    18,    19,    20,    91,    21,     0,    22,
      92,    93,     0,     0,     0,     0,     0,     0,    82,     0,
       0,     0,     0,     0,     0,    94,    95,    23,    11,    12,
       0,    84,    13,     0,   223,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,    96,    31,    32,    97,     0,
      33,    34,     0,     0,    35,     0,     0,     0,    36,    37,
      38,     0,     0,    14,    15,    85,    16,    86,    87,    88,
      89,    90,     0,    18,    19,    20,    91,    21,     0,    22,
      92,    93,     0,     0,     0,     0,     0,     0,    82,     0,
       0,     0,     0,     0,     0,    94,    95,    23,    11,    12,
       0,    84,    13,     0,   224,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,    96,    31,    32,    97,     0,
      33,    34,     0,     0,    35,     0,     0,     0,    36,    37,
      38,     0,     0,    14,    15,    85,    16,    86,    87,    88,
      89,    90,     0,    18,    19,    20,    91,    21,     0,    22,
      92,    93,     0,     0,     0,     0,     0,     0,    82,     0,
       0,     0,     0,     0,     0,    94,    95,    23,    11,    12,
       0,    84,    13,     0,   256,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,    96,    31,    32,    97,     0,
      33,    34,     0,     0,    35,     0,     0,     0,    36,    37,
      38,     0,     0,    14,    15,    85,    16,    86,    87,    88,
      89,    90,     0,    18,    19,    20,    91,    21,     0,    22,
      92,    93,     0,     0,     0,     0,     0,     0,    82,     0,
       0,     0,     0,     0,     0,    94,    95,    23,    11,    12,
       0,    84,    13,     0,   272,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,    96,    31,    32,    97,     0,
      33,    34,     0,     0,    35,     0,     0,     0,    36,    37,
      38,     0,     0,    14,    15,    85,    16,    86,    87,    88,
      89,    90,     0,    18,    19,    20,    91,    21,     0,    22,
      92,    93,     0,     0,     0,     0,     0,     0,     0,     0,
      82,     0,     0,     0,     0,    94,    95,    23,   298,     0,
      11,    12,     0,    84,    13,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,    96,    31,    32,    97,     0,
      33,    34,     0,     0,    35,     0,     0,     0,    36,    37,
      38,     0,     0,     0,     0,    14,    15,    85,    16,    86,
      87,    88,    89,    90,     0,    18,    19,    20,    91,    21,
       0,    22,    92,    93,     0,     0,     0,     0,     0,     0,
      82,     0,     0,     0,     0,     0,     0,    94,    95,    23,
      11,    12,     0,    84,    13,     0,   357,    24,    25,    26,
      27,    28,    29,     0,     0,     0,    30,    96,    31,    32,
      97,     0,    33,    34,     0,     0,    35,     0,     0,     0,
      36,    37,    38,     0,     0,    14,    15,    85,    16,    86,
      87,    88,    89,    90,     0,    18,    19,    20,    91,    21,
       0,    22,    92,    93,     0,     0,     0,     0,     0,     0,
      82,     0,     0,     0,     0,     0,     0,    94,    95,    23,
      11,    12,     0,    84,    13,     0,   374,    24,    25,    26,
      27,    28,    29,     0,     0,     0,    30,    96,    31,    32,
      97,     0,    33,    34,     0,     0,    35,     0,     0,     0,
      36,    37,    38,     0,     0,    14,    15,    85,    16,    86,
      87,    88,    89,    90,     0,    18,    19,    20,    91,    21,
       0,    22,    92,    93,     0,     0,     0,     0,     0,     0,
      82,     0,     0,     0,     0,     0,     0,    94,    95,    23,
      11,    12,     0,    84,    13,     0,     0,    24,    25,    26,
      27,    28,    29,     0,     0,     0,    30,    96,    31,    32,
      97,     0,    33,    34,     0,     0,    35,     0,     0,     0,
      36,    37,    38,     0,     0,    14,    15,    85,    16,    86,
      87,    88,    89,    90,     0,    18,    19,    20,    91,    21,
       0,    22,    92,    93,     0,     0,     0,     0,     0,    82,
       0,     0,     0,     0,     0,     0,     0,    94,    95,    23,
      12,     0,   -31,    13,     0,     0,     0,    24,    25,    26,
      27,    28,    29,     0,     0,     0,    30,    96,    31,    32,
      97,     0,    33,    34,     0,     0,    35,     0,     0,     0,
      36,    37,    38,     0,    14,    15,     0,    16,     0,    87,
       0,     0,     0,     0,    18,    19,    20,     0,    21,     0,
      22,     0,     0,     0,     0,     0,     0,     0,    82,     0,
       0,     0,     0,     0,     0,     0,    94,    95,    23,    12,
       0,     0,    13,   -31,     0,     0,    24,    25,    26,    27,
      28,    29,     0,     0,     0,    30,     0,    31,    32,     0,
       0,    33,    34,     0,     0,    35,     0,     0,     0,    36,
      37,    38,     0,    14,    15,     0,    16,     0,    87,     0,
       0,     0,     0,    18,    19,    20,     0,    21,     0,    22,
       0,     0,     0,     9,    10,     0,     0,    11,    12,     0,
       0,    13,     0,     0,     0,    94,    95,    23,     0,     0,
       0,     0,     0,     0,     0,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,     0,    31,    32,     0,     0,
      33,    34,    14,    15,    35,    16,     0,     0,    36,    37,
      38,    17,    18,    19,    20,     0,    21,     0,    22,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,     0,     0,     0,
       0,     0,     0,     0,    24,    25,    26,    27,    28,    29,
       0,     0,     0,    30,     0,    31,    32,     0,     0,    33,
      34,   162,     0,    35,    59,   111,   165,    36,    37,    38,
       0,     0,     0,     0,     0,   112,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   113,   114,
       0,   115,   116,   117,   118,   119,   120,   121,    14,    15,
       0,    16,     0,     0,     0,     0,     0,     0,    18,    19,
      20,     0,    21,     0,    22,     0,     0,     0,     0,     0,
     162,     0,     0,    12,     0,   165,    13,     0,     0,     0,
       0,     0,    23,     0,     0,     0,     0,     0,     0,     0,
      24,    25,    26,    27,    28,    29,     0,   122,     0,    30,
       0,    31,    32,     0,     0,    33,    34,    14,    15,    60,
      16,     0,     0,    36,    37,    38,     0,    18,    19,    20,
       0,    21,     0,    22,     0,     0,     0,     0,     0,   208,
       0,     0,    12,     0,     0,    13,     0,     0,     0,     0,
       0,    23,     0,     0,     0,     0,     0,     0,     0,    24,
      25,    26,    27,    28,    29,     0,     0,     0,    30,     0,
      31,    32,     0,     0,    33,    34,    14,    15,    35,    16,
       0,     0,    36,    37,    38,     0,    18,    19,    20,     0,
      21,     0,    22,     0,     0,     0,     0,     0,   210,     0,
       0,    12,     0,     0,    13,     0,     0,     0,     0,     0,
      23,     0,     0,     0,     0,     0,     0,     0,    24,    25,
      26,    27,    28,    29,     0,     0,     0,    30,     0,    31,
      32,     0,     0,    33,    34,    14,    15,    35,    16,     0,
       0,    36,    37,    38,     0,    18,    19,    20,     0,    21,
       0,    22,     0,     0,     0,     0,     0,   225,     0,     0,
      12,     0,     0,    13,     0,     0,     0,     0,     0,    23,
       0,     0,     0,     0,     0,     0,     0,    24,    25,    26,
      27,    28,    29,     0,     0,     0,    30,     0,    31,    32,
       0,     0,    33,    34,    14,    15,    35,    16,     0,     0,
      36,    37,    38,     0,    18,    19,    20,     0,    21,     0,
      22,     0,     0,     0,     0,     0,   208,     0,     0,   302,
       0,     0,    13,     0,     0,     0,     0,     0,    23,     0,
       0,     0,     0,     0,     0,     0,    24,    25,    26,    27,
      28,    29,     0,     0,     0,    30,     0,    31,    32,     0,
       0,    33,    34,    14,    15,    35,    16,     0,     0,    36,
      37,    38,     0,    18,    19,    20,     0,    21,     0,    22,
       0,     0,     0,     0,     0,   210,     0,     0,   302,     0,
       0,    13,     0,     0,     0,     0,     0,    23,     0,     0,
       0,     0,     0,     0,     0,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,     0,    31,    32,     0,     0,
      33,    34,    14,    15,    35,    16,     0,     0,    36,    37,
      38,     0,    18,    19,    20,     0,    21,     0,    22,     0,
       0,     0,     0,     0,   225,     0,     0,   302,     0,     0,
      13,     0,     0,     0,     0,     0,    23,     0,     0,     0,
       0,     0,     0,     0,    24,    25,    26,    27,    28,    29,
       0,     0,     0,    30,     0,    31,    32,     0,     0,    33,
      34,    14,    15,    35,    16,     0,     0,    36,    37,    38,
       0,    18,    19,    20,     0,    21,     0,    22,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    23,     0,     0,     0,     0,
       0,     0,     0,    24,    25,    26,    27,    28,    29,     0,
       0,     0,    30,     0,    31,    32,     0,     0,    33,    34,
       0,     0,    35,     0,     0,     0,    36,    37,    38,   140,
       0,    59,   111,     0,     0,   141,     0,     0,     0,     0,
       0,     0,   112,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   113,   114,     0,   115,   116,
     117,   118,   119,   120,   121,    14,    15,     0,    16,     0,
       0,     0,     0,     0,     0,    18,    19,    20,     0,    21,
       0,    22,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    23,
       0,     0,     0,     0,     0,     0,     0,    24,    25,    26,
      27,    28,    29,     0,   122,     0,    30,     0,    31,    32,
       0,     0,    33,    34,     0,     0,    60,     0,     0,     0,
      36,    37,    38,   140,     0,    59,   111,     0,     0,   329,
       0,     0,     0,     0,     0,     0,   112,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   113,
     114,     0,   115,   116,   117,   118,   119,   120,   121,    14,
      15,     0,    16,     0,     0,     0,     0,     0,     0,    18,
      19,    20,     0,    21,     0,    22,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    23,     0,     0,     0,     0,     0,     0,
       0,    24,    25,    26,    27,    28,    29,     0,   122,     0,
      30,     0,    31,    32,     0,     0,    33,    34,     0,     0,
      60,     0,     0,     0,    36,    37,    38,   140,     0,    59,
     111,     0,     0,   340,     0,     0,     0,     0,     0,     0,
     112,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   113,   114,     0,   115,   116,   117,   118,
     119,   120,   121,    14,    15,     0,    16,     0,     0,     0,
       0,     0,     0,    18,    19,    20,     0,    21,     0,    22,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    23,     0,     0,
       0,     0,     0,     0,     0,    24,    25,    26,    27,    28,
      29,     0,   122,     0,    30,     0,    31,    32,     0,     0,
      33,    34,     0,     0,    60,     0,     0,     0,    36,    37,
      38,   140,     0,    59,   111,     0,     0,   342,     0,     0,
       0,     0,     0,     0,   112,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   113,   114,     0,
     115,   116,   117,   118,   119,   120,   121,    14,    15,     0,
      16,     0,     0,     0,     0,     0,     0,    18,    19,    20,
       0,    21,     0,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    23,     0,     0,     0,     0,     0,     0,     0,    24,
      25,    26,    27,    28,    29,     0,   122,     0,    30,     0,
      31,    32,     0,     0,    33,    34,     0,     0,    60,     0,
       0,     0,    36,    37,    38,   140,     0,    59,   111,     0,
       0,   361,     0,     0,     0,     0,     0,     0,   112,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   113,   114,     0,   115,   116,   117,   118,   119,   120,
     121,    14,    15,     0,    16,     0,     0,     0,     0,     0,
       0,    18,    19,    20,     0,    21,     0,    22,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    23,     0,     0,     0,     0,
       0,     0,     0,    24,    25,    26,    27,    28,    29,     0,
     122,     0,    30,     0,    31,    32,     0,     0,    33,    34,
       0,     0,    60,     0,     0,     0,    36,    37,    38,   140,
       0,    59,   111,     0,     0,   363,     0,     0,     0,     0,
       0,     0,   112,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   113,   114,     0,   115,   116,
     117,   118,   119,   120,   121,    14,    15,     0,    16,     0,
       0,     0,     0,     0,     0,    18,    19,    20,     0,    21,
       0,    22,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    23,
       0,     0,     0,     0,     0,     0,     0,    24,    25,    26,
      27,    28,    29,     0,   122,     0,    30,     0,    31,    32,
       0,     0,    33,    34,     0,   140,    60,    59,   111,     0,
      36,    37,    38,     0,     0,     0,     0,     0,   112,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   113,   114,     0,   115,   116,   117,   118,   119,   120,
     121,    14,    15,     0,    16,     0,     0,     0,     0,     0,
       0,    18,    19,    20,     0,    21,     0,    22,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    23,     0,     0,     0,     0,
       0,     0,     0,    24,    25,    26,    27,    28,    29,     0,
     122,     0,    30,     0,    31,    32,     0,     0,    33,    34,
      59,   111,    60,     0,   141,     0,    36,    37,    38,     0,
       0,   112,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   113,   114,     0,   115,   116,   117,
     118,   119,   120,   121,    14,    15,     0,    16,     0,     0,
       0,     0,     0,     0,    18,    19,    20,     0,    21,     0,
      22,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    23,     0,
       0,     0,     0,     0,     0,     0,    24,    25,    26,    27,
      28,    29,     0,   122,     0,    30,     0,    31,    32,     0,
       0,    33,    34,    59,   111,    60,     0,     0,     0,    36,
      37,    38,     0,     0,   112,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   113,   114,     0,
     115,   116,   117,   118,   119,   120,   121,    14,    15,     0,
      16,     0,     0,     0,     0,     0,     0,    18,    19,    20,
       0,    21,     0,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    23,     0,     0,     0,     0,     0,     0,     0,    24,
      25,    26,    27,    28,    29,     0,   122,   274,    30,     0,
      31,    32,     0,     0,    33,    34,    59,   111,    60,     0,
     281,     0,    36,    37,    38,     0,     0,   112,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     113,   114,     0,   115,   116,   117,   118,   119,   120,   121,
      14,    15,     0,    16,     0,     0,     0,     0,     0,     0,
      18,    19,    20,     0,    21,     0,    22,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    23,     0,     0,     0,     0,     0,
       0,     0,    24,    25,    26,    27,    28,    29,     0,   122,
       0,    30,     0,    31,    32,     0,     0,    33,    34,    59,
     111,    60,     0,   323,     0,    36,    37,    38,     0,     0,
     112,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   113,   114,     0,   115,   116,   117,   118,
     119,   120,   121,    14,    15,     0,    16,     0,     0,     0,
       0,     0,     0,    18,    19,    20,     0,    21,     0,    22,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    23,     0,     0,
       0,     0,     0,     0,     0,    24,    25,    26,    27,    28,
      29,     0,   122,     0,    30,     0,    31,    32,     0,     0,
      33,    34,    59,   111,    60,     0,   325,     0,    36,    37,
      38,     0,     0,   112,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   113,   114,     0,   115,
     116,   117,   118,   119,   120,   121,    14,    15,     0,    16,
       0,     0,     0,     0,     0,     0,    18,    19,    20,     0,
      21,     0,    22,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      23,     0,     0,     0,     0,     0,     0,     0,    24,    25,
      26,    27,    28,    29,     0,   122,     0,    30,     0,    31,
      32,     0,     0,    33,    34,    59,   111,    60,     0,   365,
       0,    36,    37,    38,     0,     0,   112,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   113,
     114,     0,   115,   116,   117,   118,   119,   120,   121,    14,
      15,     0,    16,     0,     0,     0,     0,     0,     0,    18,
      19,    20,     0,    21,     0,    22,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    23,     0,     0,     0,     0,     0,     0,
       0,    24,    25,    26,    27,    28,    29,     0,   122,     0,
      30,     0,    31,    32,     0,     0,    33,    34,    59,   111,
      60,     0,   367,     0,    36,    37,    38,     0,     0,   112,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   113,   114,     0,   115,   116,   117,   118,   119,
     120,   121,    14,    15,     0,    16,     0,     0,     0,     0,
       0,     0,    18,    19,    20,     0,    21,     0,    22,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,     0,     0,     0,
       0,     0,     0,     0,    24,    25,    26,    27,    28,    29,
       0,   122,     0,    30,     0,    31,    32,     0,     0,    33,
      34,     0,     0,    60,    59,   111,   369,    36,    37,    38,
       0,     0,     0,     0,     0,   112,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   113,   114,
       0,   115,   116,   117,   118,   119,   120,   121,    14,    15,
       0,    16,     0,     0,     0,     0,     0,     0,    18,    19,
      20,     0,    21,     0,    22,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    23,     0,     0,     0,     0,     0,     0,     0,
      24,    25,    26,    27,    28,    29,     0,   122,     0,    30,
       0,    31,    32,     0,     0,    33,    34,    59,   111,    60,
       0,   380,     0,    36,    37,    38,     0,     0,   112,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   113,   114,     0,   115,   116,   117,   118,   119,   120,
     121,    14,    15,     0,    16,     0,     0,     0,     0,     0,
       0,    18,    19,    20,     0,    21,     0,    22,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    23,     0,     0,     0,     0,
       0,     0,     0,    24,    25,    26,    27,    28,    29,     0,
     122,     0,    30,     0,    31,    32,     0,     0,    33,    34,
      59,   111,    60,     0,   381,     0,    36,    37,    38,     0,
       0,   112,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   113,   114,     0,   115,   116,   117,
     118,   119,   120,   121,    14,    15,     0,    16,     0,     0,
       0,     0,     0,     0,    18,    19,    20,     0,    21,     0,
      22,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    23,     0,
       0,     0,     0,     0,     0,     0,    24,    25,    26,    27,
      28,    29,     0,   122,     0,    30,     0,    31,    32,     0,
       0,    33,    34,    59,   111,    60,     0,   382,     0,    36,
      37,    38,     0,     0,   112,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   113,   114,     0,
     115,   116,   117,   118,   119,   120,   121,    14,    15,     0,
      16,     0,     0,     0,     0,     0,     0,    18,    19,    20,
       0,    21,     0,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    23,     0,     0,     0,     0,     0,     0,     0,    24,
      25,    26,    27,    28,    29,     0,   122,     0,    30,     0,
      31,    32,     0,     0,    33,    34,    59,   111,    60,     0,
       0,     0,    36,    37,    38,     0,     0,   112,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     113,   114,     0,   115,   116,   117,   118,   119,   120,   121,
      14,    15,     0,    16,     0,     0,     0,     0,     0,     0,
      18,    19,    20,     0,    21,     0,    22,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    23,     0,     0,     0,     0,     0,
       0,     0,    24,    25,    26,    27,    28,    29,     0,   122,
       0,    30,     0,    31,    32,     0,     0,    33,    34,    59,
     111,    60,     0,     0,     0,    36,    37,    38,     0,     0,
     112,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   113,     0,     0,   115,   116,   117,   118,
     119,   120,   121,    14,    15,     0,    16,     0,     0,     0,
       0,     0,     0,    18,    19,    20,     0,    21,     0,    22,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    23,     0,     0,
       0,     0,     0,     0,     0,    24,    25,    26,    27,    28,
      29,    59,   111,     0,    30,     0,    31,    32,     0,     0,
      33,    34,   112,     0,    60,     0,     0,     0,    36,    37,
      38,     0,     0,     0,     0,     0,     0,     0,   115,   116,
     117,   118,   119,   120,   121,    14,    15,     0,    16,     0,
       0,     0,     0,     0,     0,    18,    19,    20,     0,    21,
       0,    22,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    23,
       0,     0,     0,     0,     0,     0,     0,    24,    25,    26,
      27,    28,    29,    59,   -32,     0,    30,     0,    31,    32,
       0,     0,    33,    34,   -32,     0,    60,     0,     0,     0,
      36,    37,    38,     0,     0,     0,     0,     0,     0,     0,
     -32,   -32,   -32,   -32,   -32,   -32,   -32,    14,    15,     0,
      16,     0,     0,     0,     0,     0,     0,    18,    19,    20,
       0,    21,     0,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    23,     0,     0,     0,    59,     0,     0,     0,    24,
      25,    26,    27,    28,    29,     0,   259,     0,     0,     0,
      31,    32,     0,     0,    33,    34,     0,     0,    60,   113,
     114,     0,    36,    37,    38,     0,     0,     0,   260,    14,
      15,     0,    16,     0,     0,     0,     0,     0,     0,    18,
      19,    20,     0,    21,     0,    22,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    23,     0,     0,     0,     0,     0,     0,
       0,    24,    25,    26,    27,    28,    29,    59,   261,   338,
      30,     0,    31,    32,     0,     0,    33,    34,   259,     0,
      60,     0,     0,     0,    36,    37,    38,     0,     0,     0,
       0,   113,   114,     0,     0,     0,     0,     0,     0,     0,
     260,    14,    15,     0,    16,     0,     0,     0,     0,     0,
       0,    18,    19,    20,     0,    21,     0,    22,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    23,     0,     0,     0,     0,
      59,     0,     0,    24,    25,    26,    27,    28,    29,     0,
     261,   259,    30,     0,    31,    32,     0,     0,    33,    34,
       0,     0,    60,     0,   113,     0,    36,    37,    38,     0,
       0,     0,     0,   260,    14,    15,     0,    16,     0,     0,
       0,     0,     0,     0,    18,    19,    20,     0,    21,     0,
      22,     0,     0,     0,     0,     0,     0,     0,     0,    12,
       0,     0,    13,   146,     0,     0,     0,     0,    23,     0,
       0,     0,     0,     0,     0,     0,    24,    25,    26,    27,
      28,    29,     0,     0,     0,    30,     0,    31,    32,     0,
       0,    33,    34,    14,    15,    60,    16,     0,     0,    36,
      37,    38,     0,    18,    19,    20,     0,    21,     0,    22,
       0,     0,     0,     0,     0,     0,     0,     0,    12,     0,
       0,    13,   156,     0,     0,     0,     0,    23,     0,     0,
       0,     0,     0,     0,     0,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,     0,    31,    32,     0,     0,
      33,    34,    14,    15,    35,    16,     0,     0,    36,    37,
      38,     0,    18,    19,    20,     0,    21,     0,    22,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,     0,     0,    59,
       0,     0,     0,     0,    24,    25,    26,    27,    28,    29,
     259,     0,     0,    30,     0,    31,    32,     0,     0,    33,
      34,     0,     0,    35,     0,     0,     0,    36,    37,    38,
       0,     0,   260,    14,    15,     0,    16,     0,     0,     0,
       0,     0,     0,    18,    19,    20,     0,    21,     0,    22,
       0,     0,     0,     0,     0,     0,     0,     0,    12,     0,
       0,    13,     0,     0,     0,     0,     0,    23,     0,     0,
       0,     0,     0,     0,     0,    24,    25,    26,    27,    28,
      29,     0,     0,     0,    30,     0,    31,    32,     0,     0,
      33,    34,    14,    15,    60,    16,     0,     0,    36,    37,
      38,     0,    18,    19,    20,     0,    21,     0,    22,     0,
       0,     0,     0,     0,     0,     0,     0,    59,     0,     0,
      13,     0,     0,     0,     0,     0,    23,     0,     0,     0,
       0,     0,     0,     0,    24,    25,    26,    27,    28,    29,
       0,     0,     0,    30,     0,    31,    32,     0,     0,    33,
      34,    14,    15,    35,    16,     0,     0,    36,    37,    38,
       0,    18,    19,    20,     0,    21,     0,    22,     0,     0,
       0,     0,     0,     0,     0,     0,   183,     0,     0,    13,
       0,     0,     0,     0,     0,    23,     0,     0,     0,     0,
       0,     0,     0,    24,    25,    26,    27,    28,    29,     0,
       0,     0,    30,     0,    31,    32,     0,     0,    33,    34,
      14,    15,    35,    16,     0,     0,    36,    37,    38,     0,
      18,    19,    20,     0,    21,     0,    22,     0,     0,     0,
       0,     0,     0,     0,     0,    59,     0,     0,     0,     0,
       0,     0,     0,     0,    23,     0,     0,     0,     0,     0,
       0,     0,    24,    25,    26,    27,    28,    29,     0,     0,
       0,    30,     0,    31,    32,     0,     0,    33,    34,    14,
      15,    35,    16,     0,     0,    36,    37,    38,     0,    18,
      19,    20,     0,    21,     0,    22,     0,     0,     0,     0,
       0,     0,     0,     0,   214,     0,     0,     0,     0,     0,
       0,     0,     0,    23,   302,     0,     0,    13,     0,     0,
       0,    24,    25,    26,    27,    28,    29,     0,     0,     0,
      30,     0,    31,    32,     0,     0,    33,    34,     0,     0,
      60,     0,     0,     0,    36,    37,    38,     0,    14,    15,
       0,    16,     0,     0,     0,     0,     0,     0,    18,    19,
      20,     0,    21,     0,    22,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    23,     0,     0,    59,     0,     0,     0,     0,
      24,    25,    26,    27,    28,    29,   -32,     0,     0,    30,
       0,    31,    32,     0,     0,    33,    34,     0,     0,    35,
       0,     0,     0,    36,    37,    38,     0,     0,   -32,    14,
      15,     0,    16,     0,     0,     0,     0,     0,     0,    18,
      19,    20,     0,    21,     0,    22,     0,     0,     0,     0,
       0,     0,     0,     0,    59,     0,     0,     0,     0,     0,
       0,     0,     0,    23,     0,     0,     0,     0,     0,     0,
       0,    24,    25,    26,    27,    28,    29,     0,     0,     0,
       0,     0,    31,    32,     0,     0,    33,    34,    14,    15,
      60,    16,     0,     0,    36,    37,    38,     0,    18,    19,
      20,     0,    21,     0,    22,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    23,     0,     0,     0,     0,     0,     0,     0,
      24,    25,    26,    27,    28,    29,     0,     0,     0,    30,
       0,    31,    32,     0,     0,    33,    34,     0,     0,    60,
       0,     0,     0,    36,    37,    38
};

static const yytype_int16 yycheck[] =
{
      16,     5,   172,    35,   183,     9,    10,    69,    70,    84,
     350,     9,    15,     9,    30,     9,    14,    33,    34,    35,
      36,    37,    38,    43,    53,    54,    46,    43,   184,     9,
      15,    88,    46,   108,    91,    18,    40,    71,   378,    55,
      86,   108,   184,    89,    60,    66,    92,    93,    10,     0,
      96,    11,    46,    12,    16,    71,    10,   259,   104,   261,
     262,   263,    45,   265,    10,    19,    46,   269,    88,    89,
      90,   103,   101,    19,    88,   137,    90,   111,   112,   250,
     109,   252,    12,    10,    10,    11,   102,   103,    87,    16,
     165,   112,    88,   113,    88,    89,    90,    12,   165,    71,
     103,   104,   105,   106,     9,   126,   110,   110,    88,    14,
      90,    12,   128,   129,   130,   131,   132,   133,    12,   113,
     105,   106,   121,   302,   170,   110,    12,   111,   112,    12,
     176,    13,   148,   149,    46,   151,   338,   153,   154,   111,
     112,    12,    10,   159,   160,   161,    12,   303,    16,    10,
     306,   307,   308,   309,   170,    16,    38,   313,    93,    41,
     176,   303,    18,   334,   306,   307,   308,   309,   184,    10,
      94,   313,    12,   172,     9,    16,    88,    89,    90,    14,
     350,   197,   198,   245,   200,   201,   202,   203,   204,   205,
      43,   207,   110,   209,     9,   211,   212,   368,   214,    14,
     356,   113,    12,    10,     9,   221,    12,     9,   378,    16,
     226,    10,   228,   384,   356,   236,   237,    16,   293,    10,
      12,    97,    45,    88,    10,    16,   293,   259,   227,   261,
     262,   263,    15,   265,   250,     9,   252,   269,   259,    18,
       9,    14,   241,   259,    81,   261,   262,   263,     9,   265,
     266,   267,   268,   269,    16,   101,    16,   332,    45,    16,
      16,   260,    12,    16,   280,   332,    42,   271,   284,   285,
     286,    16,   288,    16,   194,   291,    33,    34,    35,   110,
      -1,    38,    -1,    -1,    -1,   314,    43,   303,    -1,    -1,
     306,   307,   308,   309,   369,   294,    -1,   313,    55,   315,
     316,   317,   369,    60,    -1,    -1,   338,   328,    -1,    -1,
      -1,    -1,    -1,    -1,    71,   344,   320,    -1,    -1,    -1,
      -1,    -1,   338,    -1,    -1,   341,    -1,   343,    -1,   345,
     346,   347,    -1,   349,   380,   351,    -1,   336,    -1,    -1,
     356,    -1,    -1,    -1,    -1,   102,   103,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   371,    -1,    -1,    -1,   375,
     376,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   128,   129,   130,   131,   132,   133,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   148,   149,    -1,   151,    -1,   153,   154,    -1,    -1,
      -1,    -1,   159,   160,   161,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,   176,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   184,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,   200,   201,   202,   203,   204,   205,    -1,
     207,    -1,   209,    -1,   211,   212,    -1,   214,    -1,    -1,
      -1,    -1,    -1,    -1,   221,    -1,    55,    56,    -1,   226,
      -1,   228,    -1,    -1,    -1,   100,    -1,    -1,    -1,    -1,
      -1,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   250,    -1,   252,    -1,    -1,    -1,    -1,
      -1,    -1,   259,    -1,   261,   262,   263,    -1,   265,   266,
     267,   268,   269,   138,   139,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   280,    -1,    -1,    -1,   284,   285,   286,
      -1,   288,    -1,    -1,   291,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   303,     5,    -1,   306,
     307,   308,   309,    -1,    12,   180,   313,   182,   315,   316,
     317,    -1,   151,   152,   153,   154,    -1,    -1,   193,    -1,
     195,   160,   161,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   338,    -1,    -1,    -1,    -1,    -1,    -1,   345,   346,
     347,    -1,   349,    -1,   351,    -1,   185,    -1,    -1,   356,
      58,    59,    -1,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    69,    70,    -1,   371,    73,    74,    -1,   375,   376,
      -1,    -1,    -1,   212,   213,    -1,    -1,    -1,    -1,    -1,
     255,    89,    -1,    -1,    -1,    -1,    -1,    -1,    96,    -1,
      -1,    -1,    -1,    -1,   233,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   110,    -1,   112,    -1,    -1,   115,   116,   117,
     118,   119,   120,    -1,   122,   123,   124,    -1,   126,   258,
      -1,    -1,    -1,    -1,    -1,    -1,   134,    -1,    -1,   137,
      -1,    -1,    -1,    -1,   142,    -1,   144,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   284,   285,    -1,    -1,    -1,
      -1,   290,   291,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   339,   173,    -1,    -1,    -1,    -1,
     178,    -1,    -1,    -1,    -1,   183,    -1,   316,   317,    -1,
      -1,    -1,    -1,    -1,    -1,   360,   194,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   370,    -1,    -1,   337,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   345,   346,   383,    -1,
      -1,    -1,    -1,   388,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   231,    -1,    -1,   234,   235,   236,   237,
      -1,    -1,    -1,    -1,   242,    -1,    -1,   245,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   274,   275,   276,    -1,
      -1,    -1,     0,     1,    -1,    -1,    -1,    -1,    -1,     7,
       8,     9,    -1,    11,    12,    -1,    14,    15,    -1,    -1,
      -1,    -1,    -1,    -1,   302,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   321,   322,    -1,    -1,    -1,    46,    47,
     328,    49,   330,    -1,    -1,   333,    -1,    55,    56,    57,
      58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   353,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,   362,    -1,   364,    -1,    -1,    -1,
      88,    89,    90,    91,    92,    93,    -1,    -1,    -1,    97,
      -1,    99,   100,    -1,    -1,   103,   104,    -1,     1,   107,
      -1,    -1,    -1,   111,   112,   113,     9,    -1,    11,    12,
      -1,    14,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    78,    79,    80,    11,    12,
      -1,    14,    15,    -1,    17,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    98,    99,   100,   101,    -1,
     103,   104,    -1,    -1,   107,    -1,    -1,    -1,   111,   112,
     113,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    78,    79,    80,    11,    12,
      -1,    14,    15,    -1,    17,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    98,    99,   100,   101,    -1,
     103,   104,    -1,    -1,   107,    -1,    -1,    -1,   111,   112,
     113,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    78,    79,    80,    11,    12,
      -1,    14,    15,    -1,    17,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    98,    99,   100,   101,    -1,
     103,   104,    -1,    -1,   107,    -1,    -1,    -1,   111,   112,
     113,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    78,    79,    80,    11,    12,
      -1,    14,    15,    -1,    17,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    98,    99,   100,   101,    -1,
     103,   104,    -1,    -1,   107,    -1,    -1,    -1,   111,   112,
     113,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    78,    79,    80,    11,    12,
      -1,    14,    15,    -1,    17,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    98,    99,   100,   101,    -1,
     103,   104,    -1,    -1,   107,    -1,    -1,    -1,   111,   112,
     113,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    78,    79,    80,     9,    -1,
      11,    12,    -1,    14,    15,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    98,    99,   100,   101,    -1,
     103,   104,    -1,    -1,   107,    -1,    -1,    -1,   111,   112,
     113,    -1,    -1,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    58,    59,    60,
      -1,    62,    63,    64,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    78,    79,    80,
      11,    12,    -1,    14,    15,    -1,    17,    88,    89,    90,
      91,    92,    93,    -1,    -1,    -1,    97,    98,    99,   100,
     101,    -1,   103,   104,    -1,    -1,   107,    -1,    -1,    -1,
     111,   112,   113,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    58,    59,    60,
      -1,    62,    63,    64,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    78,    79,    80,
      11,    12,    -1,    14,    15,    -1,    17,    88,    89,    90,
      91,    92,    93,    -1,    -1,    -1,    97,    98,    99,   100,
     101,    -1,   103,   104,    -1,    -1,   107,    -1,    -1,    -1,
     111,   112,   113,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    58,    59,    60,
      -1,    62,    63,    64,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    78,    79,    80,
      11,    12,    -1,    14,    15,    -1,    -1,    88,    89,    90,
      91,    92,    93,    -1,    -1,    -1,    97,    98,    99,   100,
     101,    -1,   103,   104,    -1,    -1,   107,    -1,    -1,    -1,
     111,   112,   113,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    58,    59,    60,
      -1,    62,    63,    64,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    78,    79,    80,
      12,    -1,    14,    15,    -1,    -1,    -1,    88,    89,    90,
      91,    92,    93,    -1,    -1,    -1,    97,    98,    99,   100,
     101,    -1,   103,   104,    -1,    -1,   107,    -1,    -1,    -1,
     111,   112,   113,    -1,    46,    47,    -1,    49,    -1,    51,
      -1,    -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    78,    79,    80,    12,
      -1,    -1,    15,    16,    -1,    -1,    88,    89,    90,    91,
      92,    93,    -1,    -1,    -1,    97,    -1,    99,   100,    -1,
      -1,   103,   104,    -1,    -1,   107,    -1,    -1,    -1,   111,
     112,   113,    -1,    46,    47,    -1,    49,    -1,    51,    -1,
      -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,    62,
      -1,    -1,    -1,     7,     8,    -1,    -1,    11,    12,    -1,
      -1,    15,    -1,    -1,    -1,    78,    79,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,
     103,   104,    46,    47,   107,    49,    -1,    -1,   111,   112,
     113,    55,    56,    57,    58,    -1,    60,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,    93,
      -1,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,   103,
     104,     9,    -1,   107,    12,    13,    14,   111,   112,   113,
      -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,
      -1,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,
      58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,
       9,    -1,    -1,    12,    -1,    14,    15,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      88,    89,    90,    91,    92,    93,    -1,    95,    -1,    97,
      -1,    99,   100,    -1,    -1,   103,   104,    46,    47,   107,
      49,    -1,    -1,   111,   112,   113,    -1,    56,    57,    58,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    12,    -1,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,
      89,    90,    91,    92,    93,    -1,    -1,    -1,    97,    -1,
      99,   100,    -1,    -1,   103,   104,    46,    47,   107,    49,
      -1,    -1,   111,   112,   113,    -1,    56,    57,    58,    -1,
      60,    -1,    62,    -1,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    12,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,    89,
      90,    91,    92,    93,    -1,    -1,    -1,    97,    -1,    99,
     100,    -1,    -1,   103,   104,    46,    47,   107,    49,    -1,
      -1,   111,   112,   113,    -1,    56,    57,    58,    -1,    60,
      -1,    62,    -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,
      12,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,
      91,    92,    93,    -1,    -1,    -1,    97,    -1,    99,   100,
      -1,    -1,   103,   104,    46,    47,   107,    49,    -1,    -1,
     111,   112,   113,    -1,    56,    57,    58,    -1,    60,    -1,
      62,    -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,    12,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,
      92,    93,    -1,    -1,    -1,    97,    -1,    99,   100,    -1,
      -1,   103,   104,    46,    47,   107,    49,    -1,    -1,   111,
     112,   113,    -1,    56,    57,    58,    -1,    60,    -1,    62,
      -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,    12,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,
     103,   104,    46,    47,   107,    49,    -1,    -1,   111,   112,
     113,    -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,
      -1,    -1,    -1,    -1,     9,    -1,    -1,    12,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,    93,
      -1,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,   103,
     104,    46,    47,   107,    49,    -1,    -1,   111,   112,   113,
      -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    88,    89,    90,    91,    92,    93,    -1,
      -1,    -1,    97,    -1,    99,   100,    -1,    -1,   103,   104,
      -1,    -1,   107,    -1,    -1,    -1,   111,   112,   113,    10,
      -1,    12,    13,    -1,    -1,    16,    -1,    -1,    -1,    -1,
      -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    57,    58,    -1,    60,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,
      91,    92,    93,    -1,    95,    -1,    97,    -1,    99,   100,
      -1,    -1,   103,   104,    -1,    -1,   107,    -1,    -1,    -1,
     111,   112,   113,    10,    -1,    12,    13,    -1,    -1,    16,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      37,    -1,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      57,    58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    88,    89,    90,    91,    92,    93,    -1,    95,    -1,
      97,    -1,    99,   100,    -1,    -1,   103,   104,    -1,    -1,
     107,    -1,    -1,    -1,   111,   112,   113,    10,    -1,    12,
      13,    -1,    -1,    16,    -1,    -1,    -1,    -1,    -1,    -1,
      23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    37,    -1,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      93,    -1,    95,    -1,    97,    -1,    99,   100,    -1,    -1,
     103,   104,    -1,    -1,   107,    -1,    -1,    -1,   111,   112,
     113,    10,    -1,    12,    13,    -1,    -1,    16,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    58,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,
      89,    90,    91,    92,    93,    -1,    95,    -1,    97,    -1,
      99,   100,    -1,    -1,   103,   104,    -1,    -1,   107,    -1,
      -1,    -1,   111,   112,   113,    10,    -1,    12,    13,    -1,
      -1,    16,    -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    37,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    88,    89,    90,    91,    92,    93,    -1,
      95,    -1,    97,    -1,    99,   100,    -1,    -1,   103,   104,
      -1,    -1,   107,    -1,    -1,    -1,   111,   112,   113,    10,
      -1,    12,    13,    -1,    -1,    16,    -1,    -1,    -1,    -1,
      -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    57,    58,    -1,    60,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,
      91,    92,    93,    -1,    95,    -1,    97,    -1,    99,   100,
      -1,    -1,   103,   104,    -1,    10,   107,    12,    13,    -1,
     111,   112,   113,    -1,    -1,    -1,    -1,    -1,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    37,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    88,    89,    90,    91,    92,    93,    -1,
      95,    -1,    97,    -1,    99,   100,    -1,    -1,   103,   104,
      12,    13,   107,    -1,    16,    -1,   111,   112,   113,    -1,
      -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    37,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,
      92,    93,    -1,    95,    -1,    97,    -1,    99,   100,    -1,
      -1,   103,   104,    12,    13,   107,    -1,    -1,    -1,   111,
     112,   113,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    58,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,
      89,    90,    91,    92,    93,    -1,    95,    96,    97,    -1,
      99,   100,    -1,    -1,   103,   104,    12,    13,   107,    -1,
      16,    -1,   111,   112,   113,    -1,    -1,    23,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      36,    37,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    88,    89,    90,    91,    92,    93,    -1,    95,
      -1,    97,    -1,    99,   100,    -1,    -1,   103,   104,    12,
      13,   107,    -1,    16,    -1,   111,   112,   113,    -1,    -1,
      23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    37,    -1,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      93,    -1,    95,    -1,    97,    -1,    99,   100,    -1,    -1,
     103,   104,    12,    13,   107,    -1,    16,    -1,   111,   112,
     113,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    58,    -1,
      60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,    89,
      90,    91,    92,    93,    -1,    95,    -1,    97,    -1,    99,
     100,    -1,    -1,   103,   104,    12,    13,   107,    -1,    16,
      -1,   111,   112,   113,    -1,    -1,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      37,    -1,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      57,    58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    88,    89,    90,    91,    92,    93,    -1,    95,    -1,
      97,    -1,    99,   100,    -1,    -1,   103,   104,    12,    13,
     107,    -1,    16,    -1,   111,   112,   113,    -1,    -1,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    36,    37,    -1,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,    93,
      -1,    95,    -1,    97,    -1,    99,   100,    -1,    -1,   103,
     104,    -1,    -1,   107,    12,    13,    14,   111,   112,   113,
      -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,
      -1,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,
      58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      88,    89,    90,    91,    92,    93,    -1,    95,    -1,    97,
      -1,    99,   100,    -1,    -1,   103,   104,    12,    13,   107,
      -1,    16,    -1,   111,   112,   113,    -1,    -1,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    37,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    88,    89,    90,    91,    92,    93,    -1,
      95,    -1,    97,    -1,    99,   100,    -1,    -1,   103,   104,
      12,    13,   107,    -1,    16,    -1,   111,   112,   113,    -1,
      -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    37,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,
      92,    93,    -1,    95,    -1,    97,    -1,    99,   100,    -1,
      -1,   103,   104,    12,    13,   107,    -1,    16,    -1,   111,
     112,   113,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    58,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,
      89,    90,    91,    92,    93,    -1,    95,    -1,    97,    -1,
      99,   100,    -1,    -1,   103,   104,    12,    13,   107,    -1,
      -1,    -1,   111,   112,   113,    -1,    -1,    23,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      36,    37,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    88,    89,    90,    91,    92,    93,    -1,    95,
      -1,    97,    -1,    99,   100,    -1,    -1,   103,   104,    12,
      13,   107,    -1,    -1,    -1,   111,   112,   113,    -1,    -1,
      23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      93,    12,    13,    -1,    97,    -1,    99,   100,    -1,    -1,
     103,   104,    23,    -1,   107,    -1,    -1,    -1,   111,   112,
     113,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    57,    58,    -1,    60,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,
      91,    92,    93,    12,    13,    -1,    97,    -1,    99,   100,
      -1,    -1,   103,   104,    23,    -1,   107,    -1,    -1,    -1,
     111,   112,   113,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    58,
      -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    12,    -1,    -1,    -1,    88,
      89,    90,    91,    92,    93,    -1,    23,    -1,    -1,    -1,
      99,   100,    -1,    -1,   103,   104,    -1,    -1,   107,    36,
      37,    -1,   111,   112,   113,    -1,    -1,    -1,    45,    46,
      47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      57,    58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    88,    89,    90,    91,    92,    93,    12,    95,    96,
      97,    -1,    99,   100,    -1,    -1,   103,   104,    23,    -1,
     107,    -1,    -1,    -1,   111,   112,   113,    -1,    -1,    -1,
      -1,    36,    37,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      12,    -1,    -1,    88,    89,    90,    91,    92,    93,    -1,
      95,    23,    97,    -1,    99,   100,    -1,    -1,   103,   104,
      -1,    -1,   107,    -1,    36,    -1,   111,   112,   113,    -1,
      -1,    -1,    -1,    45,    46,    47,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    12,
      -1,    -1,    15,    16,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,
      92,    93,    -1,    -1,    -1,    97,    -1,    99,   100,    -1,
      -1,   103,   104,    46,    47,   107,    49,    -1,    -1,   111,
     112,   113,    -1,    56,    57,    58,    -1,    60,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    12,    -1,
      -1,    15,    16,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,
     103,   104,    46,    47,   107,    49,    -1,    -1,   111,   112,
     113,    -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    12,
      -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,    93,
      23,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,   103,
     104,    -1,    -1,   107,    -1,    -1,    -1,   111,   112,   113,
      -1,    -1,    45,    46,    47,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    56,    57,    58,    -1,    60,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    12,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      93,    -1,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,
     103,   104,    46,    47,   107,    49,    -1,    -1,   111,   112,
     113,    -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    12,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,    93,
      -1,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,   103,
     104,    46,    47,   107,    49,    -1,    -1,   111,   112,   113,
      -1,    56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    12,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    88,    89,    90,    91,    92,    93,    -1,
      -1,    -1,    97,    -1,    99,   100,    -1,    -1,   103,   104,
      46,    47,   107,    49,    -1,    -1,   111,   112,   113,    -1,
      56,    57,    58,    -1,    60,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    88,    89,    90,    91,    92,    93,    -1,    -1,
      -1,    97,    -1,    99,   100,    -1,    -1,   103,   104,    46,
      47,   107,    49,    -1,    -1,   111,   112,   113,    -1,    56,
      57,    58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    12,    -1,    -1,    15,    -1,    -1,
      -1,    88,    89,    90,    91,    92,    93,    -1,    -1,    -1,
      97,    -1,    99,   100,    -1,    -1,   103,   104,    -1,    -1,
     107,    -1,    -1,    -1,   111,   112,   113,    -1,    46,    47,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,
      58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    12,    -1,    -1,    -1,    -1,
      88,    89,    90,    91,    92,    93,    23,    -1,    -1,    97,
      -1,    99,   100,    -1,    -1,   103,   104,    -1,    -1,   107,
      -1,    -1,    -1,   111,   112,   113,    -1,    -1,    45,    46,
      47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      57,    58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    88,    89,    90,    91,    92,    93,    -1,    -1,    -1,
      -1,    -1,    99,   100,    -1,    -1,   103,   104,    46,    47,
     107,    49,    -1,    -1,   111,   112,   113,    -1,    56,    57,
      58,    -1,    60,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      88,    89,    90,    91,    92,    93,    -1,    -1,    -1,    97,
      -1,    99,   100,    -1,    -1,   103,   104,    -1,    -1,   107,
      -1,    -1,    -1,   111,   112,   113
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     9,    14,   116,   131,   133,   145,     0,     7,
       8,    11,    12,    15,    46,    47,    49,    55,    56,    57,
      58,    60,    62,    80,    88,    89,    90,    91,    92,    93,
      97,    99,   100,   103,   104,   107,   111,   112,   113,   128,
     134,   135,   137,   140,   147,   148,   158,   159,   160,   161,
     163,     9,    14,   128,   128,   140,   141,   149,    12,    12,
     107,   160,   161,    88,    91,   126,    12,    12,    12,    12,
      12,    43,   161,    12,    12,   160,   160,   147,   160,   161,
     161,   160,     1,     9,    14,    48,    50,    51,    52,    53,
      54,    59,    63,    64,    78,    79,    98,   101,   120,   122,
     127,   128,   140,   144,   151,   153,   157,   164,    10,   128,
     131,    13,    23,    36,    37,    39,    40,    41,    42,    43,
      44,    45,    95,   117,   118,   160,    12,    93,    15,   103,
     104,   105,   106,   110,    71,   111,   112,    18,   157,   157,
      10,    16,   119,    16,   119,    94,    16,   138,   140,   140,
      12,   140,   148,   140,   140,   138,    16,   138,   160,    43,
     140,   140,     9,   129,   130,    14,   129,   152,   152,   163,
     140,   152,    12,    12,   152,   152,   140,   152,    12,     9,
     154,   153,   157,    12,   139,   142,   143,   147,   160,   161,
     152,    17,   153,   156,   130,   157,   135,    97,   140,   148,
     140,   140,   140,   140,   140,   140,   163,   140,     9,   140,
       9,   140,   140,   148,    71,   160,   160,   160,   160,   160,
     160,   140,   138,    17,    17,     9,   140,    45,   140,    15,
      16,   119,    88,   162,   119,   119,   119,   119,    16,    16,
     160,   119,   119,     9,   130,    18,   152,   132,   151,   163,
     140,   152,   140,   153,    81,   121,    17,   146,   141,    23,
      45,    95,   117,   118,   160,   119,    13,    38,    41,    71,
     153,   134,    17,   161,    96,   119,   119,   160,    19,   163,
     140,    16,   119,   150,   140,   140,   140,   148,   140,   148,
     163,   140,   138,    14,    45,   150,   150,   155,     9,   153,
       9,    16,    12,   139,   148,   163,   139,   139,   139,   139,
     160,   160,   160,   139,   128,   140,   140,   140,    88,     9,
     136,   119,   119,    16,    16,    16,    16,    16,   119,    16,
     119,    19,    14,   130,   163,   101,    45,   141,    96,   157,
      16,   119,    16,   119,   128,   140,   140,   140,   148,   140,
     130,   140,   150,    12,   163,    16,   139,    17,   161,   161,
     157,    16,   119,    16,   119,    16,    16,    16,   132,    14,
     125,   140,    16,    16,    17,   140,   140,   150,   130,   153,
      16,    16,    16,   124,   132,   152,   153,   150,   123,   153
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   115,   116,   116,   117,   117,   118,   118,   119,   119,
     120,   120,   121,   121,   123,   122,   124,   122,   125,   122,
     126,   126,   127,   128,   128,   129,   129,   130,   130,   131,
     131,   132,   132,   133,   133,   134,   135,   135,   135,   135,
     135,   135,   135,   136,   135,   137,   137,   138,   138,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     140,   140,   140,   140,   140,   140,   140,   140,   140,   140,
     140,   140,   140,   140,   140,   140,   140,   140,   140,   141,
     141,   142,   142,   143,   143,   143,   144,   144,   145,   145,
     145,   145,   146,   146,   147,   147,   149,   148,   150,   150,
     151,   151,   151,   151,   151,   151,   151,   151,   152,   152,
     153,   153,   154,   155,   153,   153,   153,   153,   153,   153,
     153,   153,   153,   153,   153,   153,   156,   153,   153,   157,
     157,   158,   158,   159,   159,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   161,   161,   161,   161,   162,   162,   162,   163,   163,
     163,   164
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     1,     2,     1,     2,
       1,     2,     1,     2,     0,    12,     0,    10,     0,     8,
       1,     1,     4,     1,     2,     1,     2,     0,     1,     0,
       1,     0,     1,     1,     3,     1,     1,     4,     4,     7,
       3,     4,     4,     0,     9,     1,     3,     1,     3,     3,
       5,     3,     3,     3,     3,     3,     5,     2,     1,     1,
       3,     5,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     4,     3,     2,     1,     1,     3,
       3,     1,     3,     0,     1,     3,     1,     1,     1,     1,
       2,     2,     1,     2,     1,     2,     0,     4,     1,     2,
       4,     4,     4,     2,     5,     2,     1,     1,     1,     2,
       2,     2,     0,     0,     9,     3,     2,     1,     4,     2,
       3,     2,     2,     3,     2,     2,     0,     3,     2,     1,
       2,     1,     1,     1,     2,     4,     3,     3,     3,     3,
       3,     3,     2,     2,     2,     3,     4,     1,     3,     4,
       2,     2,     2,     2,     2,     8,     8,    10,    10,     4,
       3,     2,     1,     6,     6,     3,     6,     6,     1,     8,
       8,     6,     4,     1,     6,     6,     8,     8,     8,     6,
       1,     1,     4,     1,     2,     0,     1,     3,     1,     1,
       1,     4
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
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
  unsigned long yylno = yyrline[yyrule];
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
                       &yyvsp[(yyi + 1) - (yynrhs)]
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
            else
              goto append;

          append:
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

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
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
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
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
    default: /* Avoid compiler warnings. */
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
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
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
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
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
# else /* defined YYSTACK_RELOCATE */
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
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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
| yyreduce -- do a reduction.  |
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
#line 103 "awkgram.y"
    { if (errorflag==0)
			winner = (Node *)stat3(PROGRAM, beginloc, (yyvsp[0].p), endloc); }
#line 2606 "awkgram.tab.c"
    break;

  case 3:
#line 105 "awkgram.y"
    { yyclearin; bracecheck(); SYNTAX("bailing out"); }
#line 2612 "awkgram.tab.c"
    break;

  case 14:
#line 129 "awkgram.y"
    {inloop++;}
#line 2618 "awkgram.tab.c"
    break;

  case 15:
#line 130 "awkgram.y"
    { --inloop; (yyval.p) = stat4(FOR, (yyvsp[-9].p), notnull((yyvsp[-6].p)), (yyvsp[-3].p), (yyvsp[0].p)); }
#line 2624 "awkgram.tab.c"
    break;

  case 16:
#line 131 "awkgram.y"
    {inloop++;}
#line 2630 "awkgram.tab.c"
    break;

  case 17:
#line 132 "awkgram.y"
    { --inloop; (yyval.p) = stat4(FOR, (yyvsp[-7].p), NIL, (yyvsp[-3].p), (yyvsp[0].p)); }
#line 2636 "awkgram.tab.c"
    break;

  case 18:
#line 133 "awkgram.y"
    {inloop++;}
#line 2642 "awkgram.tab.c"
    break;

  case 19:
#line 134 "awkgram.y"
    { --inloop; (yyval.p) = stat3(IN, (yyvsp[-5].p), makearr((yyvsp[-3].p)), (yyvsp[0].p)); }
#line 2648 "awkgram.tab.c"
    break;

  case 20:
#line 138 "awkgram.y"
    { setfname((yyvsp[0].cp)); }
#line 2654 "awkgram.tab.c"
    break;

  case 21:
#line 139 "awkgram.y"
    { setfname((yyvsp[0].cp)); }
#line 2660 "awkgram.tab.c"
    break;

  case 22:
#line 143 "awkgram.y"
    { (yyval.p) = notnull((yyvsp[-1].p)); }
#line 2666 "awkgram.tab.c"
    break;

  case 27:
#line 155 "awkgram.y"
    { (yyval.i) = 0; }
#line 2672 "awkgram.tab.c"
    break;

  case 29:
#line 160 "awkgram.y"
    { (yyval.i) = 0; }
#line 2678 "awkgram.tab.c"
    break;

  case 31:
#line 166 "awkgram.y"
    { (yyval.p) = 0; }
#line 2684 "awkgram.tab.c"
    break;

  case 33:
#line 171 "awkgram.y"
    { (yyval.p) = 0; }
#line 2690 "awkgram.tab.c"
    break;

  case 34:
#line 172 "awkgram.y"
    { (yyval.p) = (yyvsp[-1].p); }
#line 2696 "awkgram.tab.c"
    break;

  case 35:
#line 176 "awkgram.y"
    { (yyval.p) = notnull((yyvsp[0].p)); }
#line 2702 "awkgram.tab.c"
    break;

  case 36:
#line 180 "awkgram.y"
    { (yyval.p) = stat2(PASTAT, (yyvsp[0].p), stat2(PRINT, rectonode(), NIL)); }
#line 2708 "awkgram.tab.c"
    break;

  case 37:
#line 181 "awkgram.y"
    { (yyval.p) = stat2(PASTAT, (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 2714 "awkgram.tab.c"
    break;

  case 38:
#line 182 "awkgram.y"
    { (yyval.p) = pa2stat((yyvsp[-3].p), (yyvsp[0].p), stat2(PRINT, rectonode(), NIL)); }
#line 2720 "awkgram.tab.c"
    break;

  case 39:
#line 183 "awkgram.y"
    { (yyval.p) = pa2stat((yyvsp[-6].p), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 2726 "awkgram.tab.c"
    break;

  case 40:
#line 184 "awkgram.y"
    { (yyval.p) = stat2(PASTAT, NIL, (yyvsp[-1].p)); }
#line 2732 "awkgram.tab.c"
    break;

  case 41:
#line 186 "awkgram.y"
    { beginloc = linkum(beginloc, (yyvsp[-1].p)); (yyval.p) = 0; }
#line 2738 "awkgram.tab.c"
    break;

  case 42:
#line 188 "awkgram.y"
    { endloc = linkum(endloc, (yyvsp[-1].p)); (yyval.p) = 0; }
#line 2744 "awkgram.tab.c"
    break;

  case 43:
#line 189 "awkgram.y"
    {infunc = true;}
#line 2750 "awkgram.tab.c"
    break;

  case 44:
#line 190 "awkgram.y"
    { infunc = false; curfname=0; defn((Cell *)(yyvsp[-7].p), (yyvsp[-5].p), (yyvsp[-1].p)); (yyval.p) = 0; }
#line 2756 "awkgram.tab.c"
    break;

  case 46:
#line 195 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2762 "awkgram.tab.c"
    break;

  case 48:
#line 200 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2768 "awkgram.tab.c"
    break;

  case 49:
#line 204 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2774 "awkgram.tab.c"
    break;

  case 50:
#line 206 "awkgram.y"
    { (yyval.p) = op3(CONDEXPR, notnull((yyvsp[-4].p)), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2780 "awkgram.tab.c"
    break;

  case 51:
#line 208 "awkgram.y"
    { (yyval.p) = op2(BOR, notnull((yyvsp[-2].p)), notnull((yyvsp[0].p))); }
#line 2786 "awkgram.tab.c"
    break;

  case 52:
#line 210 "awkgram.y"
    { (yyval.p) = op2(AND, notnull((yyvsp[-2].p)), notnull((yyvsp[0].p))); }
#line 2792 "awkgram.tab.c"
    break;

  case 53:
#line 211 "awkgram.y"
    { (yyval.p) = op3((yyvsp[-1].i), NIL, (yyvsp[-2].p), (Node*)makedfa((yyvsp[0].s), 0)); }
#line 2798 "awkgram.tab.c"
    break;

  case 54:
#line 213 "awkgram.y"
    { if (constnode((yyvsp[0].p)))
			(yyval.p) = op3((yyvsp[-1].i), NIL, (yyvsp[-2].p), (Node*)makedfa(strnode((yyvsp[0].p)), 0));
		  else
			(yyval.p) = op3((yyvsp[-1].i), (Node *)1, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2807 "awkgram.tab.c"
    break;

  case 55:
#line 217 "awkgram.y"
    { (yyval.p) = op2(INTEST, (yyvsp[-2].p), makearr((yyvsp[0].p))); }
#line 2813 "awkgram.tab.c"
    break;

  case 56:
#line 218 "awkgram.y"
    { (yyval.p) = op2(INTEST, (yyvsp[-3].p), makearr((yyvsp[0].p))); }
#line 2819 "awkgram.tab.c"
    break;

  case 57:
#line 219 "awkgram.y"
    { (yyval.p) = op2(CAT, (yyvsp[-1].p), (yyvsp[0].p)); }
#line 2825 "awkgram.tab.c"
    break;

  case 60:
#line 225 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2831 "awkgram.tab.c"
    break;

  case 61:
#line 227 "awkgram.y"
    { (yyval.p) = op3(CONDEXPR, notnull((yyvsp[-4].p)), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2837 "awkgram.tab.c"
    break;

  case 62:
#line 229 "awkgram.y"
    { (yyval.p) = op2(BOR, notnull((yyvsp[-2].p)), notnull((yyvsp[0].p))); }
#line 2843 "awkgram.tab.c"
    break;

  case 63:
#line 231 "awkgram.y"
    { (yyval.p) = op2(AND, notnull((yyvsp[-2].p)), notnull((yyvsp[0].p))); }
#line 2849 "awkgram.tab.c"
    break;

  case 64:
#line 232 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2855 "awkgram.tab.c"
    break;

  case 65:
#line 233 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2861 "awkgram.tab.c"
    break;

  case 66:
#line 234 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2867 "awkgram.tab.c"
    break;

  case 67:
#line 235 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2873 "awkgram.tab.c"
    break;

  case 68:
#line 236 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2879 "awkgram.tab.c"
    break;

  case 69:
#line 237 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2885 "awkgram.tab.c"
    break;

  case 70:
#line 238 "awkgram.y"
    { (yyval.p) = op3((yyvsp[-1].i), NIL, (yyvsp[-2].p), (Node*)makedfa((yyvsp[0].s), 0)); }
#line 2891 "awkgram.tab.c"
    break;

  case 71:
#line 240 "awkgram.y"
    { if (constnode((yyvsp[0].p)))
			(yyval.p) = op3((yyvsp[-1].i), NIL, (yyvsp[-2].p), (Node*)makedfa(strnode((yyvsp[0].p)), 0));
		  else
			(yyval.p) = op3((yyvsp[-1].i), (Node *)1, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2900 "awkgram.tab.c"
    break;

  case 72:
#line 244 "awkgram.y"
    { (yyval.p) = op2(INTEST, (yyvsp[-2].p), makearr((yyvsp[0].p))); }
#line 2906 "awkgram.tab.c"
    break;

  case 73:
#line 245 "awkgram.y"
    { (yyval.p) = op2(INTEST, (yyvsp[-3].p), makearr((yyvsp[0].p))); }
#line 2912 "awkgram.tab.c"
    break;

  case 74:
#line 246 "awkgram.y"
    {
			if (safe) SYNTAX("cmd | getline is unsafe");
			else (yyval.p) = op3(GETLINE, (yyvsp[0].p), itonp((yyvsp[-2].i)), (yyvsp[-3].p)); }
#line 2920 "awkgram.tab.c"
    break;

  case 75:
#line 249 "awkgram.y"
    {
			if (safe) SYNTAX("cmd | getline is unsafe");
			else (yyval.p) = op3(GETLINE, (Node*)0, itonp((yyvsp[-1].i)), (yyvsp[-2].p)); }
#line 2928 "awkgram.tab.c"
    break;

  case 76:
#line 252 "awkgram.y"
    { (yyval.p) = op2(CAT, (yyvsp[-1].p), (yyvsp[0].p)); }
#line 2934 "awkgram.tab.c"
    break;

  case 79:
#line 258 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2940 "awkgram.tab.c"
    break;

  case 80:
#line 259 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2946 "awkgram.tab.c"
    break;

  case 82:
#line 264 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2952 "awkgram.tab.c"
    break;

  case 83:
#line 268 "awkgram.y"
    { (yyval.p) = rectonode(); }
#line 2958 "awkgram.tab.c"
    break;

  case 85:
#line 270 "awkgram.y"
    { (yyval.p) = (yyvsp[-1].p); }
#line 2964 "awkgram.tab.c"
    break;

  case 94:
#line 287 "awkgram.y"
    { (yyval.p) = op3(MATCH, NIL, rectonode(), (Node*)makedfa((yyvsp[0].s), 0)); }
#line 2970 "awkgram.tab.c"
    break;

  case 95:
#line 288 "awkgram.y"
    { (yyval.p) = op1(NOT, notnull((yyvsp[0].p))); }
#line 2976 "awkgram.tab.c"
    break;

  case 96:
#line 292 "awkgram.y"
    {startreg();}
#line 2982 "awkgram.tab.c"
    break;

  case 97:
#line 292 "awkgram.y"
    { (yyval.s) = (yyvsp[-1].s); }
#line 2988 "awkgram.tab.c"
    break;

  case 100:
#line 300 "awkgram.y"
    {
			if (safe) SYNTAX("print | is unsafe");
			else (yyval.p) = stat3((yyvsp[-3].i), (yyvsp[-2].p), itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 2996 "awkgram.tab.c"
    break;

  case 101:
#line 303 "awkgram.y"
    {
			if (safe) SYNTAX("print >> is unsafe");
			else (yyval.p) = stat3((yyvsp[-3].i), (yyvsp[-2].p), itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 3004 "awkgram.tab.c"
    break;

  case 102:
#line 306 "awkgram.y"
    {
			if (safe) SYNTAX("print > is unsafe");
			else (yyval.p) = stat3((yyvsp[-3].i), (yyvsp[-2].p), itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 3012 "awkgram.tab.c"
    break;

  case 103:
#line 309 "awkgram.y"
    { (yyval.p) = stat3((yyvsp[-1].i), (yyvsp[0].p), NIL, NIL); }
#line 3018 "awkgram.tab.c"
    break;

  case 104:
#line 310 "awkgram.y"
    { (yyval.p) = stat2(DELETE, makearr((yyvsp[-3].p)), (yyvsp[-1].p)); }
#line 3024 "awkgram.tab.c"
    break;

  case 105:
#line 311 "awkgram.y"
    { (yyval.p) = stat2(DELETE, makearr((yyvsp[0].p)), 0); }
#line 3030 "awkgram.tab.c"
    break;

  case 106:
#line 312 "awkgram.y"
    { (yyval.p) = exptostat((yyvsp[0].p)); }
#line 3036 "awkgram.tab.c"
    break;

  case 107:
#line 313 "awkgram.y"
    { yyclearin; SYNTAX("illegal statement"); }
#line 3042 "awkgram.tab.c"
    break;

  case 110:
#line 322 "awkgram.y"
    { if (!inloop) SYNTAX("break illegal outside of loops");
				  (yyval.p) = stat1(BREAK, NIL); }
#line 3049 "awkgram.tab.c"
    break;

  case 111:
#line 324 "awkgram.y"
    {  if (!inloop) SYNTAX("continue illegal outside of loops");
				  (yyval.p) = stat1(CONTINUE, NIL); }
#line 3056 "awkgram.tab.c"
    break;

  case 112:
#line 326 "awkgram.y"
    {inloop++;}
#line 3062 "awkgram.tab.c"
    break;

  case 113:
#line 326 "awkgram.y"
    {--inloop;}
#line 3068 "awkgram.tab.c"
    break;

  case 114:
#line 327 "awkgram.y"
    { (yyval.p) = stat2(DO, (yyvsp[-6].p), notnull((yyvsp[-2].p))); }
#line 3074 "awkgram.tab.c"
    break;

  case 115:
#line 328 "awkgram.y"
    { (yyval.p) = stat1(EXIT, (yyvsp[-1].p)); }
#line 3080 "awkgram.tab.c"
    break;

  case 116:
#line 329 "awkgram.y"
    { (yyval.p) = stat1(EXIT, NIL); }
#line 3086 "awkgram.tab.c"
    break;

  case 118:
#line 331 "awkgram.y"
    { (yyval.p) = stat3(IF, (yyvsp[-3].p), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3092 "awkgram.tab.c"
    break;

  case 119:
#line 332 "awkgram.y"
    { (yyval.p) = stat3(IF, (yyvsp[-1].p), (yyvsp[0].p), NIL); }
#line 3098 "awkgram.tab.c"
    break;

  case 120:
#line 333 "awkgram.y"
    { (yyval.p) = (yyvsp[-1].p); }
#line 3104 "awkgram.tab.c"
    break;

  case 121:
#line 334 "awkgram.y"
    { if (infunc)
				SYNTAX("next is illegal inside a function");
			  (yyval.p) = stat1(NEXT, NIL); }
#line 3112 "awkgram.tab.c"
    break;

  case 122:
#line 337 "awkgram.y"
    { if (infunc)
				SYNTAX("nextfile is illegal inside a function");
			  (yyval.p) = stat1(NEXTFILE, NIL); }
#line 3120 "awkgram.tab.c"
    break;

  case 123:
#line 340 "awkgram.y"
    { (yyval.p) = stat1(RETURN, (yyvsp[-1].p)); }
#line 3126 "awkgram.tab.c"
    break;

  case 124:
#line 341 "awkgram.y"
    { (yyval.p) = stat1(RETURN, NIL); }
#line 3132 "awkgram.tab.c"
    break;

  case 126:
#line 343 "awkgram.y"
    {inloop++;}
#line 3138 "awkgram.tab.c"
    break;

  case 127:
#line 343 "awkgram.y"
    { --inloop; (yyval.p) = stat2(WHILE, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3144 "awkgram.tab.c"
    break;

  case 128:
#line 344 "awkgram.y"
    { (yyval.p) = 0; }
#line 3150 "awkgram.tab.c"
    break;

  case 130:
#line 349 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-1].p), (yyvsp[0].p)); }
#line 3156 "awkgram.tab.c"
    break;

  case 134:
#line 358 "awkgram.y"
    { (yyval.cp) = catstr((yyvsp[-1].cp), (yyvsp[0].cp)); }
#line 3162 "awkgram.tab.c"
    break;

  case 135:
#line 362 "awkgram.y"
    { (yyval.p) = op2(DIVEQ, (yyvsp[-3].p), (yyvsp[0].p)); }
#line 3168 "awkgram.tab.c"
    break;

  case 136:
#line 363 "awkgram.y"
    { (yyval.p) = op2(ADD, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3174 "awkgram.tab.c"
    break;

  case 137:
#line 364 "awkgram.y"
    { (yyval.p) = op2(MINUS, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3180 "awkgram.tab.c"
    break;

  case 138:
#line 365 "awkgram.y"
    { (yyval.p) = op2(MULT, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3186 "awkgram.tab.c"
    break;

  case 139:
#line 366 "awkgram.y"
    { (yyval.p) = op2(DIVIDE, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3192 "awkgram.tab.c"
    break;

  case 140:
#line 367 "awkgram.y"
    { (yyval.p) = op2(MOD, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3198 "awkgram.tab.c"
    break;

  case 141:
#line 368 "awkgram.y"
    { (yyval.p) = op2(POWER, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3204 "awkgram.tab.c"
    break;

  case 142:
#line 369 "awkgram.y"
    { (yyval.p) = op1(UMINUS, (yyvsp[0].p)); }
#line 3210 "awkgram.tab.c"
    break;

  case 143:
#line 370 "awkgram.y"
    { (yyval.p) = op1(UPLUS, (yyvsp[0].p)); }
#line 3216 "awkgram.tab.c"
    break;

  case 144:
#line 371 "awkgram.y"
    { (yyval.p) = op1(NOT, notnull((yyvsp[0].p))); }
#line 3222 "awkgram.tab.c"
    break;

  case 145:
#line 372 "awkgram.y"
    { (yyval.p) = op2(BLTIN, itonp((yyvsp[-2].i)), rectonode()); }
#line 3228 "awkgram.tab.c"
    break;

  case 146:
#line 373 "awkgram.y"
    { (yyval.p) = op2(BLTIN, itonp((yyvsp[-3].i)), (yyvsp[-1].p)); }
#line 3234 "awkgram.tab.c"
    break;

  case 147:
#line 374 "awkgram.y"
    { (yyval.p) = op2(BLTIN, itonp((yyvsp[0].i)), rectonode()); }
#line 3240 "awkgram.tab.c"
    break;

  case 148:
#line 375 "awkgram.y"
    { (yyval.p) = op2(CALL, celltonode((yyvsp[-2].cp),CVAR), NIL); }
#line 3246 "awkgram.tab.c"
    break;

  case 149:
#line 376 "awkgram.y"
    { (yyval.p) = op2(CALL, celltonode((yyvsp[-3].cp),CVAR), (yyvsp[-1].p)); }
#line 3252 "awkgram.tab.c"
    break;

  case 150:
#line 377 "awkgram.y"
    { (yyval.p) = op1(CLOSE, (yyvsp[0].p)); }
#line 3258 "awkgram.tab.c"
    break;

  case 151:
#line 378 "awkgram.y"
    { (yyval.p) = op1(PREDECR, (yyvsp[0].p)); }
#line 3264 "awkgram.tab.c"
    break;

  case 152:
#line 379 "awkgram.y"
    { (yyval.p) = op1(PREINCR, (yyvsp[0].p)); }
#line 3270 "awkgram.tab.c"
    break;

  case 153:
#line 380 "awkgram.y"
    { (yyval.p) = op1(POSTDECR, (yyvsp[-1].p)); }
#line 3276 "awkgram.tab.c"
    break;

  case 154:
#line 381 "awkgram.y"
    { (yyval.p) = op1(POSTINCR, (yyvsp[-1].p)); }
#line 3282 "awkgram.tab.c"
    break;

  case 155:
#line 383 "awkgram.y"
    { (yyval.p) = op5(GENSUB, NIL, (Node*)makedfa((yyvsp[-5].s), 1), (yyvsp[-3].p), (yyvsp[-1].p), rectonode()); }
#line 3288 "awkgram.tab.c"
    break;

  case 156:
#line 385 "awkgram.y"
    { if (constnode((yyvsp[-5].p)))
			(yyval.p) = op5(GENSUB, NIL, (Node *)makedfa(strnode((yyvsp[-5].p)), 1), (yyvsp[-3].p), (yyvsp[-1].p), rectonode());
		  else
			(yyval.p) = op5(GENSUB, (Node *)1, (yyvsp[-5].p), (yyvsp[-3].p), (yyvsp[-1].p), rectonode());
		}
#line 3298 "awkgram.tab.c"
    break;

  case 157:
#line 391 "awkgram.y"
    { (yyval.p) = op5(GENSUB, NIL, (Node*)makedfa((yyvsp[-7].s), 1), (yyvsp[-5].p), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3304 "awkgram.tab.c"
    break;

  case 158:
#line 393 "awkgram.y"
    { if (constnode((yyvsp[-7].p)))
			(yyval.p) = op5(GENSUB, NIL, (Node *)makedfa(strnode((yyvsp[-7].p)),1), (yyvsp[-5].p),(yyvsp[-3].p),(yyvsp[-1].p));
		  else
			(yyval.p) = op5(GENSUB, (Node *)1, (yyvsp[-7].p), (yyvsp[-5].p), (yyvsp[-3].p), (yyvsp[-1].p));
		}
#line 3314 "awkgram.tab.c"
    break;

  case 159:
#line 398 "awkgram.y"
    { (yyval.p) = op3(GETLINE, (yyvsp[-2].p), itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 3320 "awkgram.tab.c"
    break;

  case 160:
#line 399 "awkgram.y"
    { (yyval.p) = op3(GETLINE, NIL, itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 3326 "awkgram.tab.c"
    break;

  case 161:
#line 400 "awkgram.y"
    { (yyval.p) = op3(GETLINE, (yyvsp[0].p), NIL, NIL); }
#line 3332 "awkgram.tab.c"
    break;

  case 162:
#line 401 "awkgram.y"
    { (yyval.p) = op3(GETLINE, NIL, NIL, NIL); }
#line 3338 "awkgram.tab.c"
    break;

  case 163:
#line 403 "awkgram.y"
    { (yyval.p) = op2(INDEX, (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3344 "awkgram.tab.c"
    break;

  case 164:
#line 405 "awkgram.y"
    { SYNTAX("index() doesn't permit regular expressions");
		  (yyval.p) = op2(INDEX, (yyvsp[-3].p), (Node*)(yyvsp[-1].s)); }
#line 3351 "awkgram.tab.c"
    break;

  case 165:
#line 407 "awkgram.y"
    { (yyval.p) = (yyvsp[-1].p); }
#line 3357 "awkgram.tab.c"
    break;

  case 166:
#line 409 "awkgram.y"
    { (yyval.p) = op3(MATCHFCN, NIL, (yyvsp[-3].p), (Node*)makedfa((yyvsp[-1].s), 1)); }
#line 3363 "awkgram.tab.c"
    break;

  case 167:
#line 411 "awkgram.y"
    { if (constnode((yyvsp[-1].p)))
			(yyval.p) = op3(MATCHFCN, NIL, (yyvsp[-3].p), (Node*)makedfa(strnode((yyvsp[-1].p)), 1));
		  else
			(yyval.p) = op3(MATCHFCN, (Node *)1, (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3372 "awkgram.tab.c"
    break;

  case 168:
#line 415 "awkgram.y"
    { (yyval.p) = celltonode((yyvsp[0].cp), CCON); }
#line 3378 "awkgram.tab.c"
    break;

  case 169:
#line 417 "awkgram.y"
    { (yyval.p) = op4(SPLIT, (yyvsp[-5].p), makearr((yyvsp[-3].p)), (yyvsp[-1].p), (Node*)STRING); }
#line 3384 "awkgram.tab.c"
    break;

  case 170:
#line 419 "awkgram.y"
    { (yyval.p) = op4(SPLIT, (yyvsp[-5].p), makearr((yyvsp[-3].p)), (Node*)makedfa((yyvsp[-1].s), 1), (Node *)REGEXPR); }
#line 3390 "awkgram.tab.c"
    break;

  case 171:
#line 421 "awkgram.y"
    { (yyval.p) = op4(SPLIT, (yyvsp[-3].p), makearr((yyvsp[-1].p)), NIL, (Node*)STRING); }
#line 3396 "awkgram.tab.c"
    break;

  case 172:
#line 422 "awkgram.y"
    { (yyval.p) = op1((yyvsp[-3].i), (yyvsp[-1].p)); }
#line 3402 "awkgram.tab.c"
    break;

  case 173:
#line 423 "awkgram.y"
    { (yyval.p) = celltonode((yyvsp[0].cp), CCON); }
#line 3408 "awkgram.tab.c"
    break;

  case 174:
#line 425 "awkgram.y"
    { (yyval.p) = op4((yyvsp[-5].i), NIL, (Node*)makedfa((yyvsp[-3].s), 1), (yyvsp[-1].p), rectonode()); }
#line 3414 "awkgram.tab.c"
    break;

  case 175:
#line 427 "awkgram.y"
    { if (constnode((yyvsp[-3].p)))
			(yyval.p) = op4((yyvsp[-5].i), NIL, (Node*)makedfa(strnode((yyvsp[-3].p)), 1), (yyvsp[-1].p), rectonode());
		  else
			(yyval.p) = op4((yyvsp[-5].i), (Node *)1, (yyvsp[-3].p), (yyvsp[-1].p), rectonode()); }
#line 3423 "awkgram.tab.c"
    break;

  case 176:
#line 432 "awkgram.y"
    { (yyval.p) = op4((yyvsp[-7].i), NIL, (Node*)makedfa((yyvsp[-5].s), 1), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3429 "awkgram.tab.c"
    break;

  case 177:
#line 434 "awkgram.y"
    { if (constnode((yyvsp[-5].p)))
			(yyval.p) = op4((yyvsp[-7].i), NIL, (Node*)makedfa(strnode((yyvsp[-5].p)), 1), (yyvsp[-3].p), (yyvsp[-1].p));
		  else
			(yyval.p) = op4((yyvsp[-7].i), (Node *)1, (yyvsp[-5].p), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3438 "awkgram.tab.c"
    break;

  case 178:
#line 439 "awkgram.y"
    { (yyval.p) = op3(SUBSTR, (yyvsp[-5].p), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3444 "awkgram.tab.c"
    break;

  case 179:
#line 441 "awkgram.y"
    { (yyval.p) = op3(SUBSTR, (yyvsp[-3].p), (yyvsp[-1].p), NIL); }
#line 3450 "awkgram.tab.c"
    break;

  case 182:
#line 447 "awkgram.y"
    { (yyval.p) = op2(ARRAY, makearr((yyvsp[-3].p)), (yyvsp[-1].p)); }
#line 3456 "awkgram.tab.c"
    break;

  case 183:
#line 448 "awkgram.y"
    { (yyval.p) = op1(INDIRECT, celltonode((yyvsp[0].cp), CVAR)); }
#line 3462 "awkgram.tab.c"
    break;

  case 184:
#line 449 "awkgram.y"
    { (yyval.p) = op1(INDIRECT, (yyvsp[0].p)); }
#line 3468 "awkgram.tab.c"
    break;

  case 185:
#line 453 "awkgram.y"
    { arglist = (yyval.p) = 0; }
#line 3474 "awkgram.tab.c"
    break;

  case 186:
#line 454 "awkgram.y"
    { arglist = (yyval.p) = celltonode((yyvsp[0].cp),CVAR); }
#line 3480 "awkgram.tab.c"
    break;

  case 187:
#line 455 "awkgram.y"
    {
			checkdup((yyvsp[-2].p), (yyvsp[0].cp));
			arglist = (yyval.p) = linkum((yyvsp[-2].p),celltonode((yyvsp[0].cp),CVAR)); }
#line 3488 "awkgram.tab.c"
    break;

  case 188:
#line 461 "awkgram.y"
    { (yyval.p) = celltonode((yyvsp[0].cp), CVAR); }
#line 3494 "awkgram.tab.c"
    break;

  case 189:
#line 462 "awkgram.y"
    { (yyval.p) = op1(ARG, itonp((yyvsp[0].i))); }
#line 3500 "awkgram.tab.c"
    break;

  case 190:
#line 463 "awkgram.y"
    { (yyval.p) = op1(VARNF, (Node *) (yyvsp[0].cp)); }
#line 3506 "awkgram.tab.c"
    break;

  case 191:
#line 468 "awkgram.y"
    { (yyval.p) = notnull((yyvsp[-1].p)); }
#line 3512 "awkgram.tab.c"
    break;


#line 3516 "awkgram.tab.c"

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
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

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


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
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
#line 471 "awkgram.y"


void setfname(Cell *p)
{
	if (isarr(p))
		SYNTAX("%s is an array, not a function", p->nval);
	else if (isfcn(p))
		SYNTAX("you can't define function %s more than once", p->nval);
	curfname = p->nval;
}

int constnode(Node *p)
{
	return isvalue(p) && ((Cell *) (p->narg[0]))->csub == CCON;
}

char *strnode(Node *p)
{
	return ((Cell *)(p->narg[0]))->sval;
}

Node *notnull(Node *n)
{
	switch (n->nobj) {
	case LE: case LT: case EQ: case NE: case GT: case GE:
	case BOR: case AND: case NOT:
		return n;
	default:
		return op2(NE, n, nullnode);
	}
}

void checkdup(Node *vl, Cell *cp)	/* check if name already in list */
{
	char *s = cp->nval;
	for ( ; vl; vl = vl->nnext) {
		if (strcmp(s, ((Cell *)(vl->narg[0]))->nval) == 0) {
			SYNTAX("duplicate argument %s", s);
			break;
		}
	}
}
