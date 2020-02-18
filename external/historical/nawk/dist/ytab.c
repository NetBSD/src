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

#line 86 "awkgram.tab.c"

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
    SUB = 301,
    GSUB = 302,
    IF = 303,
    INDEX = 304,
    LSUBSTR = 305,
    MATCHFCN = 306,
    NEXT = 307,
    NEXTFILE = 308,
    ADD = 309,
    MINUS = 310,
    MULT = 311,
    DIVIDE = 312,
    MOD = 313,
    ASSIGN = 314,
    ASGNOP = 315,
    ADDEQ = 316,
    SUBEQ = 317,
    MULTEQ = 318,
    DIVEQ = 319,
    MODEQ = 320,
    POWEQ = 321,
    PRINT = 322,
    PRINTF = 323,
    SPRINTF = 324,
    ELSE = 325,
    INTEST = 326,
    CONDEXPR = 327,
    POSTINCR = 328,
    PREINCR = 329,
    POSTDECR = 330,
    PREDECR = 331,
    VAR = 332,
    IVAR = 333,
    VARNF = 334,
    CALL = 335,
    NUMBER = 336,
    STRING = 337,
    REGEXPR = 338,
    GETLINE = 339,
    RETURN = 340,
    SPLIT = 341,
    SUBSTR = 342,
    WHILE = 343,
    CAT = 344,
    NOT = 345,
    UMINUS = 346,
    UPLUS = 347,
    POWER = 348,
    DECR = 349,
    INCR = 350,
    INDIRECT = 351,
    LASTTOKEN = 352
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 41 "awkgram.y"

	Node	*p;
	Cell	*cp;
	int	i;
	char	*s;

#line 234 "awkgram.tab.c"

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
#define YYLAST   4608

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  114
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  50
/* YYNRULES -- Number of rules.  */
#define YYNRULES  187
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  370

#define YYUNDEFTOK  2
#define YYMAXUTOK   352

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
       2,     2,     2,     2,     2,     2,     2,   105,     2,     2,
      12,    16,   104,   102,    10,   103,     2,    15,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    95,    14,
       2,     2,     2,    94,     2,     2,     2,     2,     2,     2,
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
      85,    86,    87,    88,    89,    90,    91,    92,    93,    96,
      97,    98,    99,   100,   101,   106,   107,   108,   109,   110,
     111,   112,   113
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    99,    99,   101,   105,   105,   109,   109,   113,   113,
     117,   117,   121,   121,   125,   125,   127,   127,   129,   129,
     134,   135,   139,   143,   143,   147,   147,   151,   152,   156,
     157,   162,   163,   167,   168,   172,   176,   177,   178,   179,
     180,   181,   183,   185,   185,   190,   191,   195,   196,   200,
     201,   203,   205,   207,   208,   213,   214,   215,   216,   217,
     221,   222,   224,   226,   228,   229,   230,   231,   232,   233,
     234,   235,   240,   241,   242,   245,   248,   249,   250,   254,
     255,   259,   260,   264,   265,   266,   270,   270,   274,   274,
     274,   274,   278,   278,   282,   284,   288,   288,   292,   292,
     296,   299,   302,   305,   306,   307,   308,   309,   313,   314,
     318,   320,   322,   322,   322,   324,   325,   326,   327,   328,
     329,   330,   333,   336,   337,   338,   339,   339,   340,   344,
     345,   349,   349,   353,   354,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,   374,   375,   376,   377,   378,   379,   380,   381,   382,
     384,   387,   388,   390,   395,   396,   398,   400,   402,   403,
     404,   406,   411,   413,   418,   420,   422,   426,   427,   428,
     429,   433,   434,   435,   441,   442,   443,   448
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
  "CONTINUE", "DELETE", "DO", "EXIT", "FOR", "FUNC", "SUB", "GSUB", "IF",
  "INDEX", "LSUBSTR", "MATCHFCN", "NEXT", "NEXTFILE", "ADD", "MINUS",
  "MULT", "DIVIDE", "MOD", "ASSIGN", "ASGNOP", "ADDEQ", "SUBEQ", "MULTEQ",
  "DIVEQ", "MODEQ", "POWEQ", "PRINT", "PRINTF", "SPRINTF", "ELSE",
  "INTEST", "CONDEXPR", "POSTINCR", "PREINCR", "POSTDECR", "PREDECR",
  "VAR", "IVAR", "VARNF", "CALL", "NUMBER", "STRING", "REGEXPR", "'?'",
  "':'", "GETLINE", "RETURN", "SPLIT", "SUBSTR", "WHILE", "CAT", "'+'",
  "'-'", "'*'", "'%'", "NOT", "UMINUS", "UPLUS", "POWER", "DECR", "INCR",
  "INDIRECT", "LASTTOKEN", "$accept", "program", "and", "bor", "comma",
  "do", "else", "for", "$@1", "$@2", "$@3", "funcname", "if", "lbrace",
  "nl", "opt_nl", "opt_pst", "opt_simple_stmt", "pas", "pa_pat", "pa_stat",
  "$@4", "pa_stats", "patlist", "ppattern", "pattern", "plist", "pplist",
  "prarg", "print", "pst", "rbrace", "re", "reg_expr", "$@5", "rparen",
  "simple_stmt", "st", "stmt", "$@6", "$@7", "$@8", "stmtlist", "subop",
  "string", "term", "var", "varlist", "varname", "while", YY_NULLPTR
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
     335,   336,   337,   338,    63,    58,   339,   340,   341,   342,
     343,   344,    43,    45,    42,    37,   345,   346,   347,   348,
     349,   350,   351,   352
};
# endif

#define YYPACT_NINF -316

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-316)))

#define YYTABLE_NINF -32

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-32)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     635,  -316,  -316,  -316,    10,  1580,  -316,   151,  -316,     3,
       3,  -316,  4178,  -316,  -316,    29,  4496,    18,  -316,  -316,
      40,    44,    56,  -316,  -316,  -316,    71,  -316,  -316,    81,
      95,   104,  4496,  4496,  4226,   261,   261,  4496,   763,    76,
    -316,   157,  3511,  -316,  -316,   106,   -62,    -3,   -34,   117,
    -316,  -316,   763,   763,  2184,    39,    53,  4014,  4178,  4496,
      -3,    32,  -316,  -316,   113,  4178,  4178,  4178,  4072,  4496,
     115,  4178,  4178,    65,    65,  -316,    65,  -316,  -316,  -316,
    -316,  -316,   166,   158,   158,   -14,  -316,  1733,   164,   178,
     158,   158,  -316,  -316,  1733,   186,   190,  -316,  1386,   763,
    3511,  4284,   158,  -316,   832,  -316,   166,   763,  1580,   108,
    4178,  -316,  -316,  4178,  4178,  4178,  4178,  4178,  4178,   -14,
    4178,  1791,  1849,    -3,  4178,  -316,  4332,  4496,  4496,  4496,
    4496,  4496,  4178,  -316,  -316,  4178,   901,   970,  -316,  -316,
    1907,   155,  1907,   192,  -316,    62,  3511,  2680,   116,  2588,
    2588,    80,  -316,    87,    -3,  4496,  2588,  2588,  -316,   196,
    -316,   166,   196,  -316,  -316,   191,  1675,  -316,  1454,  4178,
    -316,  -316,  1675,  -316,  4178,  -316,  1386,   130,  1039,  4178,
    3894,   201,    57,  -316,    -3,   -30,  -316,  -316,  -316,  1386,
    4178,  1108,  -316,   261,  3765,  -316,  3765,  3765,  3765,  3765,
    3765,  3765,  -316,  2772,  -316,  3684,  -316,  3603,  2588,   201,
    4496,    65,    43,    43,    65,    65,    65,  3511,    27,  -316,
    -316,  -316,  3511,   -14,  3511,  -316,  -316,  1907,  -316,   107,
    1907,  1907,  -316,  -316,    -3,     2,  1907,  -316,  -316,  4178,
    -316,   203,  -316,   -11,  2864,  -316,  2864,  -316,  -316,  1179,
    -316,   206,   128,  4400,   -14,  4400,  1965,  2023,    -3,  2081,
    4496,  4496,  4496,  4400,  -316,     3,  -316,  -316,  4178,  1907,
    1907,    -3,  -316,  -316,  3511,  -316,     6,   210,  2956,   204,
    3048,   213,   143,  2287,    47,   188,   -14,   210,   210,   132,
    -316,  -316,  -316,   193,  4178,  4448,  -316,  -316,  3813,  4120,
    3966,  3894,    -3,    -3,    -3,  3894,   763,  3511,  2390,  2493,
    -316,  -316,     3,  -316,  -316,  -316,  -316,  -316,  1907,  -316,
    1907,  -316,   166,  4178,   217,   223,   -14,   147,  4400,  1248,
    -316,    33,  -316,    33,   763,  3140,   220,  3232,  1522,  3327,
     210,  4178,  -316,   193,  3894,  -316,   226,   232,  1317,  -316,
    -316,  -316,   217,   166,  1386,  3419,  -316,  -316,  -316,   210,
    1522,  -316,   158,  1386,   217,  -316,  -316,   210,  1386,  -316
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     3,    88,    89,     0,    33,     2,    30,     1,     0,
       0,    23,     0,    96,   185,   147,     0,     0,   131,   132,
       0,     0,     0,   184,   179,   186,     0,   164,   133,   158,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    36,
      45,    29,    35,    77,    94,     0,   169,    78,   176,   177,
      90,    91,     0,     0,     0,     0,     0,     0,     0,     0,
     150,   176,    20,    21,     0,     0,     0,     0,     0,     0,
     157,     0,     0,   143,   142,    95,   144,   151,   152,   180,
     107,    24,    27,     0,     0,     0,    10,     0,     0,     0,
       0,     0,    86,    87,     0,     0,   112,   117,     0,     0,
     106,    83,     0,   129,     0,   126,    27,     0,    34,     0,
       0,     4,     6,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    76,     0,   134,     0,     0,     0,     0,
       0,     0,     0,   153,   154,     0,     0,     0,     8,   161,
       0,     0,     0,     0,   145,     0,    47,     0,   181,     0,
       0,     0,   148,     0,   156,     0,     0,     0,    25,    28,
     128,    27,   108,   110,   111,   105,     0,   116,     0,     0,
     121,   122,     0,   124,     0,    11,     0,   119,     0,     0,
      81,    84,   103,    58,    59,   176,   125,    40,   130,     0,
       0,     0,    46,    75,    71,    70,    64,    65,    66,    67,
      68,    69,    72,     0,     5,    63,     7,    62,     0,    94,
       0,   139,   136,   137,   138,   140,   141,    60,     0,    41,
      42,     9,    79,     0,    80,    97,   146,     0,   182,     0,
       0,     0,   168,   149,   155,     0,     0,    26,   109,     0,
     115,     0,    32,   177,     0,   123,     0,   113,    12,     0,
      92,   120,     0,     0,     0,     0,     0,     0,    57,     0,
       0,     0,     0,     0,   127,    38,    37,    74,     0,     0,
       0,   135,   178,    73,    48,    98,     0,    43,     0,    94,
       0,    94,     0,     0,     0,    27,     0,    22,   187,     0,
      13,   118,    93,    85,     0,    54,    53,    55,     0,    52,
      51,    82,   100,   101,   102,    49,     0,    61,     0,     0,
     183,    99,     0,   159,   160,   163,   162,   167,     0,   175,
       0,   104,    27,     0,     0,     0,     0,     0,     0,     0,
     171,     0,   170,     0,     0,     0,    94,     0,     0,     0,
      18,     0,    56,     0,    50,    39,     0,     0,     0,   165,
     166,   174,     0,    27,     0,     0,   173,   172,    44,    16,
       0,    19,     0,     0,     0,   114,    17,    14,     0,    15
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -316,  -316,    -1,    46,     5,  -316,  -316,  -316,  -316,  -316,
    -316,  -316,  -316,    -4,   -73,   -67,   209,  -315,  -316,    61,
     145,  -316,  -316,   -43,  -192,   482,  -175,  -316,  -316,  -316,
    -316,  -316,   -32,  -102,  -316,  -215,  -165,   -40,   381,  -316,
    -316,  -316,   -25,  -316,  -316,   236,   -16,  -316,   103,  -316
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     4,   121,   122,   227,    96,   249,    97,   368,   363,
     354,    64,    98,    99,   162,   160,     5,   241,     6,    39,
      40,   312,    41,   145,   180,   100,    55,   181,   182,   101,
       7,   251,    43,    44,    56,   277,   102,   163,   103,   176,
     289,   189,   104,    45,    46,    47,    48,   229,    49,   105
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      61,    38,    75,   242,   252,    52,    53,   135,   195,   159,
       8,   221,   126,    70,    11,   221,    61,    61,    61,    77,
      78,    61,   209,   352,   151,   153,    61,   136,   137,   287,
     125,   288,    14,   159,   286,   107,   132,   138,    61,   190,
     263,    57,   221,    61,   164,   364,   272,   167,    14,   138,
     170,   171,    65,    61,   173,   141,    66,   138,   126,   140,
     142,   295,   186,   298,   299,   300,   321,   301,    67,   183,
     260,   305,   138,    23,   178,    25,   133,   134,   226,    14,
     133,   134,   191,    68,    61,   185,   106,    11,   159,    23,
     138,    25,   218,   310,   238,   261,   232,   138,   262,   127,
     128,   129,   130,   233,    38,    62,   131,    71,    63,   340,
      61,    61,    61,    61,    61,    61,    72,   138,   124,   327,
      23,    24,    25,   275,    69,   148,   240,    14,   279,   281,
      61,    61,   245,    61,    61,   135,   344,   359,   138,    61,
      61,    61,   133,   134,   293,    37,   143,   129,   130,   367,
      61,   296,   131,   138,   230,   231,    61,   138,   155,   317,
      50,   235,   236,   343,    61,    51,     2,   158,    23,    24,
      25,     3,   161,   242,   131,   158,   168,   267,    61,   256,
      61,    61,    61,    61,    61,    61,   259,    61,   165,    61,
     169,    61,    61,    37,    61,   242,   284,   158,   174,   175,
     223,    61,   322,   228,   193,   237,    61,   225,    61,   239,
     248,   138,   159,   269,   270,   292,   336,   285,   323,   311,
     314,   183,   202,   183,   183,   183,   257,   183,    61,   316,
      61,   183,   325,   275,   276,   341,   350,   185,   326,   185,
     185,   185,   356,   185,    61,    61,    61,   185,   357,   159,
     108,   265,    60,   192,     0,   338,     0,   142,    61,     0,
       0,   306,    61,     0,    61,     0,     0,    61,    73,    74,
      76,   243,     0,    79,     0,     0,     0,     0,   123,    61,
     159,   329,    61,    61,    61,    61,   360,   318,   320,    61,
     123,    61,    61,    61,   256,    76,   183,   256,   256,   256,
     256,     0,     0,     0,   256,   154,     0,    14,   334,   348,
       0,     0,   185,   331,   333,   346,     0,   347,     0,    61,
       0,    61,   365,    61,     0,     0,   273,     0,    61,     0,
       0,     0,   142,     0,     0,     0,   123,   184,   282,    61,
       0,   257,     0,   256,   257,   257,   257,   257,    23,    24,
      25,   257,     0,     0,     0,     0,     0,   297,     0,     0,
       0,     0,   211,   212,   213,   214,   215,   216,     0,     0,
       0,     0,     0,    37,     0,     0,     0,     0,     0,     0,
       0,     0,   123,   123,     0,   123,   123,     0,     0,   324,
     257,   234,   123,   123,     0,     0,     0,     0,     0,     0,
       0,     0,   123,     0,     0,     0,     0,     0,   123,     0,
       0,     0,     0,     0,     0,     0,   258,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   342,
     123,     0,   123,   123,   123,   123,   123,   123,     0,   123,
       0,   123,     0,   123,   123,     0,   271,     0,     0,     0,
       0,     0,     0,   123,     0,     0,     0,     0,   123,     0,
     123,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   177,
     123,     0,   123,     0,     0,   188,     0,    42,     0,   184,
       0,   184,   184,   184,    54,   184,   302,   303,   304,   184,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     123,     0,     0,     0,   123,     0,   123,   188,   188,   123,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   258,     0,     0,   258,   258,   258,   258,     0,   146,
     147,   258,     0,   123,   123,   123,     0,   149,   150,   146,
     146,     0,     0,   156,   157,     0,     0,   247,     0,   188,
       0,     0,     0,     0,   184,     0,     0,     0,     0,   166,
     264,   123,   188,   123,     0,   123,   172,     0,     0,     0,
     258,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      42,   123,   194,     0,     0,   196,   197,   198,   199,   200,
     201,     0,   203,   205,   207,     0,   208,     0,     0,     0,
       0,     0,     0,     0,   217,     0,     0,   146,     0,     0,
       0,     0,   222,     0,   224,     0,     0,     0,     0,     0,
     291,     0,     0,     0,     0,   -29,     1,     0,     0,     0,
       0,     0,   -29,   -29,     2,     0,   -29,   -29,     0,     3,
     -29,   244,     0,     0,     0,     0,   246,     0,     0,     0,
       0,    54,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    42,     0,     0,     0,     0,     0,     0,     0,
       0,   -29,   -29,     0,   -29,     0,     0,     0,     0,     0,
     -29,   -29,   -29,     0,   -29,     0,   -29,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   274,
     188,     0,   278,   280,   -29,     0,     0,     0,   283,     0,
       0,   146,   -29,   -29,   -29,   -29,   -29,   -29,     0,   188,
       0,   -29,     0,   -29,   -29,   361,     0,   -29,   -29,     0,
       0,   -29,     0,     0,   366,   -29,   -29,   -29,     0,   369,
     307,   308,   309,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    80,     0,     0,     0,     0,     0,
       0,     0,    81,     0,    11,    12,    54,    82,    13,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     335,     0,   337,     0,     0,   339,     0,     0,     0,    14,
      15,    83,    16,    84,    85,    86,    87,    88,     0,    18,
      19,    89,    20,   355,    21,    90,    91,     0,     0,     0,
       0,     0,     0,    80,     0,     0,     0,     0,     0,     0,
      92,    93,    22,    11,    12,     0,    82,    13,     0,   187,
      23,    24,    25,    26,    27,    28,     0,     0,     0,    29,
      94,    30,    31,    95,     0,    32,    33,     0,     0,    34,
       0,     0,     0,    35,    36,    37,     0,     0,    14,    15,
      83,    16,    84,    85,    86,    87,    88,     0,    18,    19,
      89,    20,     0,    21,    90,    91,     0,     0,     0,     0,
       0,     0,    80,     0,     0,     0,     0,     0,     0,    92,
      93,    22,    11,    12,     0,    82,    13,     0,   219,    23,
      24,    25,    26,    27,    28,     0,     0,     0,    29,    94,
      30,    31,    95,     0,    32,    33,     0,     0,    34,     0,
       0,     0,    35,    36,    37,     0,     0,    14,    15,    83,
      16,    84,    85,    86,    87,    88,     0,    18,    19,    89,
      20,     0,    21,    90,    91,     0,     0,     0,     0,     0,
       0,    80,     0,     0,     0,     0,     0,     0,    92,    93,
      22,    11,    12,     0,    82,    13,     0,   220,    23,    24,
      25,    26,    27,    28,     0,     0,     0,    29,    94,    30,
      31,    95,     0,    32,    33,     0,     0,    34,     0,     0,
       0,    35,    36,    37,     0,     0,    14,    15,    83,    16,
      84,    85,    86,    87,    88,     0,    18,    19,    89,    20,
       0,    21,    90,    91,     0,     0,     0,     0,     0,     0,
      80,     0,     0,     0,     0,     0,     0,    92,    93,    22,
      11,    12,     0,    82,    13,     0,   250,    23,    24,    25,
      26,    27,    28,     0,     0,     0,    29,    94,    30,    31,
      95,     0,    32,    33,     0,     0,    34,     0,     0,     0,
      35,    36,    37,     0,     0,    14,    15,    83,    16,    84,
      85,    86,    87,    88,     0,    18,    19,    89,    20,     0,
      21,    90,    91,     0,     0,     0,     0,     0,     0,    80,
       0,     0,     0,     0,     0,     0,    92,    93,    22,    11,
      12,     0,    82,    13,     0,   266,    23,    24,    25,    26,
      27,    28,     0,     0,     0,    29,    94,    30,    31,    95,
       0,    32,    33,     0,     0,    34,     0,     0,     0,    35,
      36,    37,     0,     0,    14,    15,    83,    16,    84,    85,
      86,    87,    88,     0,    18,    19,    89,    20,     0,    21,
      90,    91,     0,     0,     0,     0,     0,     0,     0,     0,
      80,     0,     0,     0,     0,    92,    93,    22,   290,     0,
      11,    12,     0,    82,    13,    23,    24,    25,    26,    27,
      28,     0,     0,     0,    29,    94,    30,    31,    95,     0,
      32,    33,     0,     0,    34,     0,     0,     0,    35,    36,
      37,     0,     0,     0,     0,    14,    15,    83,    16,    84,
      85,    86,    87,    88,     0,    18,    19,    89,    20,     0,
      21,    90,    91,     0,     0,     0,     0,     0,     0,    80,
       0,     0,     0,     0,     0,     0,    92,    93,    22,    11,
      12,     0,    82,    13,     0,   345,    23,    24,    25,    26,
      27,    28,     0,     0,     0,    29,    94,    30,    31,    95,
       0,    32,    33,     0,     0,    34,     0,     0,     0,    35,
      36,    37,     0,     0,    14,    15,    83,    16,    84,    85,
      86,    87,    88,     0,    18,    19,    89,    20,     0,    21,
      90,    91,     0,     0,     0,     0,     0,     0,    80,     0,
       0,     0,     0,     0,     0,    92,    93,    22,    11,    12,
       0,    82,    13,     0,   358,    23,    24,    25,    26,    27,
      28,     0,     0,     0,    29,    94,    30,    31,    95,     0,
      32,    33,     0,     0,    34,     0,     0,     0,    35,    36,
      37,     0,     0,    14,    15,    83,    16,    84,    85,    86,
      87,    88,     0,    18,    19,    89,    20,     0,    21,    90,
      91,     0,     0,     0,     0,     0,     0,    80,     0,     0,
       0,     0,     0,     0,    92,    93,    22,    11,    12,     0,
      82,    13,     0,     0,    23,    24,    25,    26,    27,    28,
       0,     0,     0,    29,    94,    30,    31,    95,     0,    32,
      33,     0,     0,    34,     0,     0,     0,    35,    36,    37,
       0,     0,    14,    15,    83,    16,    84,    85,    86,    87,
      88,     0,    18,    19,    89,    20,     0,    21,    90,    91,
       0,     0,     0,     0,     0,    80,     0,     0,     0,     0,
       0,     0,     0,    92,    93,    22,    12,     0,   -31,    13,
       0,     0,     0,    23,    24,    25,    26,    27,    28,     0,
       0,     0,    29,    94,    30,    31,    95,     0,    32,    33,
       0,     0,    34,     0,     0,     0,    35,    36,    37,     0,
      14,    15,     0,    16,     0,    85,     0,     0,     0,     0,
      18,    19,     0,    20,     0,    21,     0,     0,     0,     0,
       0,     0,     0,    80,     0,     0,     0,     0,     0,     0,
       0,    92,    93,    22,    12,     0,     0,    13,   -31,     0,
       0,    23,    24,    25,    26,    27,    28,     0,     0,     0,
      29,     0,    30,    31,     0,     0,    32,    33,     0,     0,
      34,     0,     0,     0,    35,    36,    37,     0,    14,    15,
       0,    16,     0,    85,     0,     0,     0,     0,    18,    19,
       0,    20,     0,    21,     0,     0,     0,     9,    10,     0,
       0,    11,    12,     0,     0,    13,     0,     0,     0,    92,
      93,    22,     0,     0,     0,     0,     0,     0,     0,    23,
      24,    25,    26,    27,    28,     0,     0,     0,    29,     0,
      30,    31,     0,     0,    32,    33,    14,    15,    34,    16,
       0,     0,    35,    36,    37,    17,    18,    19,     0,    20,
       0,    21,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    22,
       0,     0,     0,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,     0,     0,     0,    29,     0,    30,    31,
       0,     0,    32,    33,   158,     0,    34,    58,   109,   161,
      35,    36,    37,     0,     0,     0,     0,     0,   110,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   111,   112,     0,   113,   114,   115,   116,   117,   118,
     119,    14,    15,     0,    16,     0,     0,     0,     0,     0,
       0,    18,    19,     0,    20,     0,    21,     0,     0,     0,
       0,     0,   158,     0,     0,    12,     0,   161,    13,     0,
       0,     0,     0,     0,    22,     0,     0,     0,     0,     0,
       0,     0,    23,    24,    25,    26,    27,    28,     0,   120,
       0,    29,     0,    30,    31,     0,     0,    32,    33,    14,
      15,    59,    16,     0,     0,    35,    36,    37,     0,    18,
      19,     0,    20,     0,    21,     0,     0,     0,     0,     0,
     204,     0,     0,    12,     0,     0,    13,     0,     0,     0,
       0,     0,    22,     0,     0,     0,     0,     0,     0,     0,
      23,    24,    25,    26,    27,    28,     0,     0,     0,    29,
       0,    30,    31,     0,     0,    32,    33,    14,    15,    34,
      16,     0,     0,    35,    36,    37,     0,    18,    19,     0,
      20,     0,    21,     0,     0,     0,     0,     0,   206,     0,
       0,    12,     0,     0,    13,     0,     0,     0,     0,     0,
      22,     0,     0,     0,     0,     0,     0,     0,    23,    24,
      25,    26,    27,    28,     0,     0,     0,    29,     0,    30,
      31,     0,     0,    32,    33,    14,    15,    34,    16,     0,
       0,    35,    36,    37,     0,    18,    19,     0,    20,     0,
      21,     0,     0,     0,     0,     0,   221,     0,     0,    12,
       0,     0,    13,     0,     0,     0,     0,     0,    22,     0,
       0,     0,     0,     0,     0,     0,    23,    24,    25,    26,
      27,    28,     0,     0,     0,    29,     0,    30,    31,     0,
       0,    32,    33,    14,    15,    34,    16,     0,     0,    35,
      36,    37,     0,    18,    19,     0,    20,     0,    21,     0,
       0,     0,     0,     0,   204,     0,     0,   294,     0,     0,
      13,     0,     0,     0,     0,     0,    22,     0,     0,     0,
       0,     0,     0,     0,    23,    24,    25,    26,    27,    28,
       0,     0,     0,    29,     0,    30,    31,     0,     0,    32,
      33,    14,    15,    34,    16,     0,     0,    35,    36,    37,
       0,    18,    19,     0,    20,     0,    21,     0,     0,     0,
       0,     0,   206,     0,     0,   294,     0,     0,    13,     0,
       0,     0,     0,     0,    22,     0,     0,     0,     0,     0,
       0,     0,    23,    24,    25,    26,    27,    28,     0,     0,
       0,    29,     0,    30,    31,     0,     0,    32,    33,    14,
      15,    34,    16,     0,     0,    35,    36,    37,     0,    18,
      19,     0,    20,     0,    21,     0,     0,     0,     0,     0,
     221,     0,     0,   294,     0,     0,    13,     0,     0,     0,
       0,     0,    22,     0,     0,     0,     0,     0,     0,     0,
      23,    24,    25,    26,    27,    28,     0,     0,     0,    29,
       0,    30,    31,     0,     0,    32,    33,    14,    15,    34,
      16,     0,     0,    35,    36,    37,     0,    18,    19,     0,
      20,     0,    21,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      22,     0,     0,     0,     0,     0,     0,     0,    23,    24,
      25,    26,    27,    28,     0,     0,     0,    29,     0,    30,
      31,     0,     0,    32,    33,     0,     0,    34,     0,     0,
       0,    35,    36,    37,   138,     0,    58,   109,     0,     0,
     139,     0,     0,     0,     0,     0,     0,   110,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     111,   112,     0,   113,   114,   115,   116,   117,   118,   119,
      14,    15,     0,    16,     0,     0,     0,     0,     0,     0,
      18,    19,     0,    20,     0,    21,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    22,     0,     0,     0,     0,     0,     0,
       0,    23,    24,    25,    26,    27,    28,     0,   120,     0,
      29,     0,    30,    31,     0,     0,    32,    33,     0,     0,
      59,     0,     0,     0,    35,    36,    37,   138,     0,    58,
     109,     0,     0,   319,     0,     0,     0,     0,     0,     0,
     110,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   111,   112,     0,   113,   114,   115,   116,
     117,   118,   119,    14,    15,     0,    16,     0,     0,     0,
       0,     0,     0,    18,    19,     0,    20,     0,    21,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    22,     0,     0,     0,
       0,     0,     0,     0,    23,    24,    25,    26,    27,    28,
       0,   120,     0,    29,     0,    30,    31,     0,     0,    32,
      33,     0,     0,    59,     0,     0,     0,    35,    36,    37,
     138,     0,    58,   109,     0,     0,   330,     0,     0,     0,
       0,     0,     0,   110,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   111,   112,     0,   113,
     114,   115,   116,   117,   118,   119,    14,    15,     0,    16,
       0,     0,     0,     0,     0,     0,    18,    19,     0,    20,
       0,    21,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    22,
       0,     0,     0,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,     0,   120,     0,    29,     0,    30,    31,
       0,     0,    32,    33,     0,     0,    59,     0,     0,     0,
      35,    36,    37,   138,     0,    58,   109,     0,     0,   332,
       0,     0,     0,     0,     0,     0,   110,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   111,
     112,     0,   113,   114,   115,   116,   117,   118,   119,    14,
      15,     0,    16,     0,     0,     0,     0,     0,     0,    18,
      19,     0,    20,     0,    21,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    22,     0,     0,     0,     0,     0,     0,     0,
      23,    24,    25,    26,    27,    28,     0,   120,     0,    29,
       0,    30,    31,     0,     0,    32,    33,     0,   138,    59,
      58,   109,     0,    35,    36,    37,     0,     0,     0,     0,
       0,   110,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   111,   112,     0,   113,   114,   115,
     116,   117,   118,   119,    14,    15,     0,    16,     0,     0,
       0,     0,     0,     0,    18,    19,     0,    20,     0,    21,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    22,     0,     0,
       0,     0,     0,     0,     0,    23,    24,    25,    26,    27,
      28,     0,   120,     0,    29,     0,    30,    31,     0,     0,
      32,    33,    58,   109,    59,     0,   139,     0,    35,    36,
      37,     0,     0,   110,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   111,   112,     0,   113,
     114,   115,   116,   117,   118,   119,    14,    15,     0,    16,
       0,     0,     0,     0,     0,     0,    18,    19,     0,    20,
       0,    21,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    22,
       0,     0,     0,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,     0,   120,     0,    29,     0,    30,    31,
       0,     0,    32,    33,    58,   109,    59,     0,     0,     0,
      35,    36,    37,     0,     0,   110,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   111,   112,
       0,   113,   114,   115,   116,   117,   118,   119,    14,    15,
       0,    16,     0,     0,     0,     0,     0,     0,    18,    19,
       0,    20,     0,    21,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    22,     0,     0,     0,     0,     0,     0,     0,    23,
      24,    25,    26,    27,    28,     0,   120,   268,    29,     0,
      30,    31,     0,     0,    32,    33,    58,   109,    59,     0,
     275,     0,    35,    36,    37,     0,     0,   110,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     111,   112,     0,   113,   114,   115,   116,   117,   118,   119,
      14,    15,     0,    16,     0,     0,     0,     0,     0,     0,
      18,    19,     0,    20,     0,    21,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    22,     0,     0,     0,     0,     0,     0,
       0,    23,    24,    25,    26,    27,    28,     0,   120,     0,
      29,     0,    30,    31,     0,     0,    32,    33,    58,   109,
      59,     0,   313,     0,    35,    36,    37,     0,     0,   110,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   111,   112,     0,   113,   114,   115,   116,   117,
     118,   119,    14,    15,     0,    16,     0,     0,     0,     0,
       0,     0,    18,    19,     0,    20,     0,    21,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    22,     0,     0,     0,     0,
       0,     0,     0,    23,    24,    25,    26,    27,    28,     0,
     120,     0,    29,     0,    30,    31,     0,     0,    32,    33,
      58,   109,    59,     0,   315,     0,    35,    36,    37,     0,
       0,   110,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   111,   112,     0,   113,   114,   115,
     116,   117,   118,   119,    14,    15,     0,    16,     0,     0,
       0,     0,     0,     0,    18,    19,     0,    20,     0,    21,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    22,     0,     0,
       0,     0,     0,     0,     0,    23,    24,    25,    26,    27,
      28,     0,   120,     0,    29,     0,    30,    31,     0,     0,
      32,    33,    58,   109,    59,     0,   349,     0,    35,    36,
      37,     0,     0,   110,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   111,   112,     0,   113,
     114,   115,   116,   117,   118,   119,    14,    15,     0,    16,
       0,     0,     0,     0,     0,     0,    18,    19,     0,    20,
       0,    21,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    22,
       0,     0,     0,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,     0,   120,     0,    29,     0,    30,    31,
       0,     0,    32,    33,    58,   109,    59,     0,   351,     0,
      35,    36,    37,     0,     0,   110,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   111,   112,
       0,   113,   114,   115,   116,   117,   118,   119,    14,    15,
       0,    16,     0,     0,     0,     0,     0,     0,    18,    19,
       0,    20,     0,    21,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    22,     0,     0,     0,     0,     0,     0,     0,    23,
      24,    25,    26,    27,    28,     0,   120,     0,    29,     0,
      30,    31,     0,     0,    32,    33,     0,     0,    59,    58,
     109,   353,    35,    36,    37,     0,     0,     0,     0,     0,
     110,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   111,   112,     0,   113,   114,   115,   116,
     117,   118,   119,    14,    15,     0,    16,     0,     0,     0,
       0,     0,     0,    18,    19,     0,    20,     0,    21,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    22,     0,     0,     0,
       0,     0,     0,     0,    23,    24,    25,    26,    27,    28,
       0,   120,     0,    29,     0,    30,    31,     0,     0,    32,
      33,    58,   109,    59,     0,   362,     0,    35,    36,    37,
       0,     0,   110,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   111,   112,     0,   113,   114,
     115,   116,   117,   118,   119,    14,    15,     0,    16,     0,
       0,     0,     0,     0,     0,    18,    19,     0,    20,     0,
      21,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    22,     0,
       0,     0,     0,     0,     0,     0,    23,    24,    25,    26,
      27,    28,     0,   120,     0,    29,     0,    30,    31,     0,
       0,    32,    33,    58,   109,    59,     0,     0,     0,    35,
      36,    37,     0,     0,   110,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   111,   112,     0,
     113,   114,   115,   116,   117,   118,   119,    14,    15,     0,
      16,     0,     0,     0,     0,     0,     0,    18,    19,     0,
      20,     0,    21,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      22,     0,     0,     0,     0,     0,     0,     0,    23,    24,
      25,    26,    27,    28,     0,   120,     0,    29,     0,    30,
      31,     0,     0,    32,    33,    58,   109,    59,     0,     0,
       0,    35,    36,    37,     0,     0,   110,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   111,
       0,     0,   113,   114,   115,   116,   117,   118,   119,    14,
      15,     0,    16,     0,     0,     0,     0,     0,     0,    18,
      19,     0,    20,     0,    21,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    22,     0,     0,     0,     0,     0,     0,     0,
      23,    24,    25,    26,    27,    28,    58,   109,     0,    29,
       0,    30,    31,     0,     0,    32,    33,   110,     0,    59,
       0,     0,     0,    35,    36,    37,     0,     0,     0,     0,
       0,     0,     0,   113,   114,   115,   116,   117,   118,   119,
      14,    15,     0,    16,     0,     0,     0,     0,     0,     0,
      18,    19,     0,    20,     0,    21,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    22,     0,     0,     0,     0,     0,     0,
       0,    23,    24,    25,    26,    27,    28,    58,   -32,     0,
      29,     0,    30,    31,     0,     0,    32,    33,   -32,     0,
      59,     0,     0,     0,    35,    36,    37,     0,     0,     0,
       0,     0,     0,     0,   -32,   -32,   -32,   -32,   -32,   -32,
     -32,    14,    15,     0,    16,     0,     0,     0,     0,     0,
       0,    18,    19,     0,    20,    58,    21,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   253,     0,     0,     0,
       0,     0,     0,     0,    22,     0,     0,     0,     0,   111,
     112,     0,    23,    24,    25,    26,    27,    28,   254,    14,
      15,     0,    16,    30,    31,     0,     0,    32,    33,    18,
      19,    59,    20,     0,    21,    35,    36,    37,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    22,     0,     0,     0,     0,     0,     0,     0,
      23,    24,    25,    26,    27,    28,    58,   255,   328,    29,
       0,    30,    31,     0,     0,    32,    33,   253,     0,    59,
       0,     0,     0,    35,    36,    37,     0,     0,     0,     0,
     111,   112,     0,     0,     0,     0,     0,     0,     0,   254,
      14,    15,     0,    16,     0,     0,     0,     0,     0,     0,
      18,    19,     0,    20,     0,    21,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    22,     0,     0,     0,     0,    58,     0,
       0,    23,    24,    25,    26,    27,    28,     0,   255,   253,
      29,     0,    30,    31,     0,     0,    32,    33,     0,     0,
      59,     0,   111,     0,    35,    36,    37,     0,     0,     0,
       0,   254,    14,    15,     0,    16,     0,     0,     0,     0,
       0,     0,    18,    19,     0,    20,    12,    21,     0,    13,
     144,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    22,     0,     0,     0,     0,
       0,     0,     0,    23,    24,    25,    26,    27,    28,     0,
      14,    15,    29,    16,    30,    31,     0,     0,    32,    33,
      18,    19,    59,    20,     0,    21,    35,    36,    37,     0,
       0,     0,     0,     0,    12,     0,     0,    13,   152,     0,
       0,     0,     0,    22,     0,     0,     0,     0,     0,     0,
       0,    23,    24,    25,    26,    27,    28,     0,     0,     0,
      29,     0,    30,    31,     0,     0,    32,    33,    14,    15,
      34,    16,     0,     0,    35,    36,    37,     0,    18,    19,
       0,    20,    58,    21,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   253,     0,     0,     0,     0,     0,     0,
       0,    22,     0,     0,     0,     0,     0,     0,     0,    23,
      24,    25,    26,    27,    28,   254,    14,    15,    29,    16,
      30,    31,     0,     0,    32,    33,    18,    19,    34,    20,
       0,    21,    35,    36,    37,     0,     0,     0,     0,     0,
      12,     0,     0,    13,     0,     0,     0,     0,     0,    22,
       0,     0,     0,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,     0,     0,     0,    29,     0,    30,    31,
       0,     0,    32,    33,    14,    15,    59,    16,     0,     0,
      35,    36,    37,     0,    18,    19,     0,    20,    58,    21,
       0,    13,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    22,     0,     0,
       0,     0,     0,     0,     0,    23,    24,    25,    26,    27,
      28,     0,    14,    15,    29,    16,    30,    31,     0,     0,
      32,    33,    18,    19,    34,    20,     0,    21,    35,    36,
      37,     0,     0,     0,     0,     0,   179,     0,     0,    13,
       0,     0,     0,     0,     0,    22,     0,     0,     0,     0,
       0,     0,     0,    23,    24,    25,    26,    27,    28,     0,
       0,     0,    29,     0,    30,    31,     0,     0,    32,    33,
      14,    15,    34,    16,     0,     0,    35,    36,    37,     0,
      18,    19,     0,    20,    58,    21,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    22,     0,     0,     0,     0,     0,     0,
       0,    23,    24,    25,    26,    27,    28,     0,    14,    15,
      29,    16,    30,    31,     0,     0,    32,    33,    18,    19,
      34,    20,     0,    21,    35,    36,    37,     0,     0,     0,
       0,     0,   210,     0,     0,     0,     0,     0,     0,     0,
       0,    22,   294,     0,     0,    13,     0,     0,     0,    23,
      24,    25,    26,    27,    28,     0,     0,     0,    29,     0,
      30,    31,     0,     0,    32,    33,     0,     0,    59,     0,
       0,     0,    35,    36,    37,     0,    14,    15,     0,    16,
       0,     0,     0,     0,     0,     0,    18,    19,     0,    20,
      58,    21,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   -32,     0,     0,     0,     0,     0,     0,     0,    22,
       0,     0,     0,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,   -32,    14,    15,    29,    16,    30,    31,
       0,     0,    32,    33,    18,    19,    34,    20,    58,    21,
      35,    36,    37,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    22,     0,     0,
       0,     0,     0,     0,     0,    23,    24,    25,    26,    27,
      28,     0,    14,    15,     0,    16,    30,    31,     0,     0,
      32,    33,    18,    19,    59,    20,     0,    21,    35,    36,
      37,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    22,     0,     0,     0,     0,
       0,     0,     0,    23,    24,    25,    26,    27,    28,     0,
       0,     0,    29,     0,    30,    31,     0,     0,    32,    33,
       0,     0,    59,     0,     0,     0,    35,    36,    37
};

static const yytype_int16 yycheck[] =
{
      16,     5,    34,   168,   179,     9,    10,    18,   110,    82,
       0,     9,    15,    29,    11,     9,    32,    33,    34,    35,
      36,    37,   124,   338,    67,    68,    42,    52,    53,   244,
      92,   246,    46,   106,    45,    39,    70,    10,    54,   106,
      70,    12,     9,    59,    84,   360,    19,    87,    46,    10,
      90,    91,    12,    69,    94,    16,    12,    10,    15,    54,
      55,   253,   102,   255,   256,   257,    19,   259,    12,   101,
      13,   263,    10,    87,    99,    89,   110,   111,    16,    46,
     110,   111,   107,    12,   100,   101,    10,    11,   161,    87,
      10,    89,   135,    87,   161,    38,    16,    10,    41,   102,
     103,   104,   105,    16,   108,    87,   109,    12,    90,   324,
     126,   127,   128,   129,   130,   131,    12,    10,    12,   294,
      87,    88,    89,    16,    43,    12,   166,    46,   230,   231,
     146,   147,   172,   149,   150,    18,   328,   352,    10,   155,
     156,   157,   110,   111,    16,   112,    93,   104,   105,   364,
     166,   253,   109,    10,   149,   150,   172,    10,    43,    16,
       9,   156,   157,    16,   180,    14,     9,     9,    87,    88,
      89,    14,    14,   338,   109,     9,    12,   193,   194,   180,
     196,   197,   198,   199,   200,   201,   181,   203,    85,   205,
      12,   207,   208,   112,   210,   360,   239,     9,    12,     9,
      45,   217,    14,    87,    96,     9,   222,    15,   224,    18,
      80,    10,   285,   208,   209,     9,   318,    14,   285,     9,
      16,   253,   119,   255,   256,   257,   180,   259,   244,    16,
     246,   263,   100,    16,   229,    12,    16,   253,    45,   255,
     256,   257,    16,   259,   260,   261,   262,   263,    16,   322,
      41,   190,    16,   108,    -1,   322,    -1,   252,   274,    -1,
      -1,   265,   278,    -1,   280,    -1,    -1,   283,    32,    33,
      34,   168,    -1,    37,    -1,    -1,    -1,    -1,    42,   295,
     353,   306,   298,   299,   300,   301,   353,   282,   283,   305,
      54,   307,   308,   309,   295,    59,   328,   298,   299,   300,
     301,    -1,    -1,    -1,   305,    69,    -1,    46,   312,   334,
      -1,    -1,   328,   308,   309,   331,    -1,   333,    -1,   335,
      -1,   337,   362,   339,    -1,    -1,   223,    -1,   344,    -1,
      -1,    -1,   327,    -1,    -1,    -1,   100,   101,   235,   355,
      -1,   295,    -1,   344,   298,   299,   300,   301,    87,    88,
      89,   305,    -1,    -1,    -1,    -1,    -1,   254,    -1,    -1,
      -1,    -1,   126,   127,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   146,   147,    -1,   149,   150,    -1,    -1,   286,
     344,   155,   156,   157,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,   172,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   326,
     194,    -1,   196,   197,   198,   199,   200,   201,    -1,   203,
      -1,   205,    -1,   207,   208,    -1,   210,    -1,    -1,    -1,
      -1,    -1,    -1,   217,    -1,    -1,    -1,    -1,   222,    -1,
     224,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
     244,    -1,   246,    -1,    -1,   104,    -1,     5,    -1,   253,
      -1,   255,   256,   257,    12,   259,   260,   261,   262,   263,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     274,    -1,    -1,    -1,   278,    -1,   280,   136,   137,   283,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   295,    -1,    -1,   298,   299,   300,   301,    -1,    57,
      58,   305,    -1,   307,   308,   309,    -1,    65,    66,    67,
      68,    -1,    -1,    71,    72,    -1,    -1,   176,    -1,   178,
      -1,    -1,    -1,    -1,   328,    -1,    -1,    -1,    -1,    87,
     189,   335,   191,   337,    -1,   339,    94,    -1,    -1,    -1,
     344,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   355,   110,    -1,    -1,   113,   114,   115,   116,   117,
     118,    -1,   120,   121,   122,    -1,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   132,    -1,    -1,   135,    -1,    -1,
      -1,    -1,   140,    -1,   142,    -1,    -1,    -1,    -1,    -1,
     249,    -1,    -1,    -1,    -1,     0,     1,    -1,    -1,    -1,
      -1,    -1,     7,     8,     9,    -1,    11,    12,    -1,    14,
      15,   169,    -1,    -1,    -1,    -1,   174,    -1,    -1,    -1,
      -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   190,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      55,    56,    57,    -1,    59,    -1,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   227,
     329,    -1,   230,   231,    79,    -1,    -1,    -1,   236,    -1,
      -1,   239,    87,    88,    89,    90,    91,    92,    -1,   348,
      -1,    96,    -1,    98,    99,   354,    -1,   102,   103,    -1,
      -1,   106,    -1,    -1,   363,   110,   111,   112,    -1,   368,
     268,   269,   270,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     9,    -1,    11,    12,   294,    14,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     318,    -1,   320,    -1,    -1,   323,    -1,    -1,    -1,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    58,    59,   341,    61,    62,    63,    -1,    -1,    -1,
      -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    79,    11,    12,    -1,    14,    15,    -1,    17,
      87,    88,    89,    90,    91,    92,    -1,    -1,    -1,    96,
      97,    98,    99,   100,    -1,   102,   103,    -1,    -1,   106,
      -1,    -1,    -1,   110,   111,   112,    -1,    -1,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    -1,    56,    57,
      58,    59,    -1,    61,    62,    63,    -1,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    77,
      78,    79,    11,    12,    -1,    14,    15,    -1,    17,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    96,    97,
      98,    99,   100,    -1,   102,   103,    -1,    -1,   106,    -1,
      -1,    -1,   110,   111,   112,    -1,    -1,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    -1,    56,    57,    58,
      59,    -1,    61,    62,    63,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      79,    11,    12,    -1,    14,    15,    -1,    17,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    96,    97,    98,
      99,   100,    -1,   102,   103,    -1,    -1,   106,    -1,    -1,
      -1,   110,   111,   112,    -1,    -1,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    -1,    56,    57,    58,    59,
      -1,    61,    62,    63,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    79,
      11,    12,    -1,    14,    15,    -1,    17,    87,    88,    89,
      90,    91,    92,    -1,    -1,    -1,    96,    97,    98,    99,
     100,    -1,   102,   103,    -1,    -1,   106,    -1,    -1,    -1,
     110,   111,   112,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    58,    59,    -1,
      61,    62,    63,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    79,    11,
      12,    -1,    14,    15,    -1,    17,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    96,    97,    98,    99,   100,
      -1,   102,   103,    -1,    -1,   106,    -1,    -1,    -1,   110,
     111,   112,    -1,    -1,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    -1,    56,    57,    58,    59,    -1,    61,
      62,    63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    77,    78,    79,     9,    -1,
      11,    12,    -1,    14,    15,    87,    88,    89,    90,    91,
      92,    -1,    -1,    -1,    96,    97,    98,    99,   100,    -1,
     102,   103,    -1,    -1,   106,    -1,    -1,    -1,   110,   111,
     112,    -1,    -1,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    58,    59,    -1,
      61,    62,    63,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    79,    11,
      12,    -1,    14,    15,    -1,    17,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    96,    97,    98,    99,   100,
      -1,   102,   103,    -1,    -1,   106,    -1,    -1,    -1,   110,
     111,   112,    -1,    -1,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    -1,    56,    57,    58,    59,    -1,    61,
      62,    63,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    77,    78,    79,    11,    12,
      -1,    14,    15,    -1,    17,    87,    88,    89,    90,    91,
      92,    -1,    -1,    -1,    96,    97,    98,    99,   100,    -1,
     102,   103,    -1,    -1,   106,    -1,    -1,    -1,   110,   111,
     112,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    58,    59,    -1,    61,    62,
      63,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    79,    11,    12,    -1,
      14,    15,    -1,    -1,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    96,    97,    98,    99,   100,    -1,   102,
     103,    -1,    -1,   106,    -1,    -1,    -1,   110,   111,   112,
      -1,    -1,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    -1,    56,    57,    58,    59,    -1,    61,    62,    63,
      -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    77,    78,    79,    12,    -1,    14,    15,
      -1,    -1,    -1,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    96,    97,    98,    99,   100,    -1,   102,   103,
      -1,    -1,   106,    -1,    -1,    -1,   110,   111,   112,    -1,
      46,    47,    -1,    49,    -1,    51,    -1,    -1,    -1,    -1,
      56,    57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    78,    79,    12,    -1,    -1,    15,    16,    -1,
      -1,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      96,    -1,    98,    99,    -1,    -1,   102,   103,    -1,    -1,
     106,    -1,    -1,    -1,   110,   111,   112,    -1,    46,    47,
      -1,    49,    -1,    51,    -1,    -1,    -1,    -1,    56,    57,
      -1,    59,    -1,    61,    -1,    -1,    -1,     7,     8,    -1,
      -1,    11,    12,    -1,    -1,    15,    -1,    -1,    -1,    77,
      78,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    96,    -1,
      98,    99,    -1,    -1,   102,   103,    46,    47,   106,    49,
      -1,    -1,   110,   111,   112,    55,    56,    57,    -1,    59,
      -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,
      90,    91,    92,    -1,    -1,    -1,    96,    -1,    98,    99,
      -1,    -1,   102,   103,     9,    -1,   106,    12,    13,    14,
     110,   111,   112,    -1,    -1,    -1,    -1,    -1,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    37,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,    -1,    59,    -1,    61,    -1,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    12,    -1,    14,    15,    -1,
      -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    87,    88,    89,    90,    91,    92,    -1,    94,
      -1,    96,    -1,    98,    99,    -1,    -1,   102,   103,    46,
      47,   106,    49,    -1,    -1,   110,   111,   112,    -1,    56,
      57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,    -1,
       9,    -1,    -1,    12,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      87,    88,    89,    90,    91,    92,    -1,    -1,    -1,    96,
      -1,    98,    99,    -1,    -1,   102,   103,    46,    47,   106,
      49,    -1,    -1,   110,   111,   112,    -1,    56,    57,    -1,
      59,    -1,    61,    -1,    -1,    -1,    -1,    -1,     9,    -1,
      -1,    12,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    96,    -1,    98,
      99,    -1,    -1,   102,   103,    46,    47,   106,    49,    -1,
      -1,   110,   111,   112,    -1,    56,    57,    -1,    59,    -1,
      61,    -1,    -1,    -1,    -1,    -1,     9,    -1,    -1,    12,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    96,    -1,    98,    99,    -1,
      -1,   102,   103,    46,    47,   106,    49,    -1,    -1,   110,
     111,   112,    -1,    56,    57,    -1,    59,    -1,    61,    -1,
      -1,    -1,    -1,    -1,     9,    -1,    -1,    12,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    96,    -1,    98,    99,    -1,    -1,   102,
     103,    46,    47,   106,    49,    -1,    -1,   110,   111,   112,
      -1,    56,    57,    -1,    59,    -1,    61,    -1,    -1,    -1,
      -1,    -1,     9,    -1,    -1,    12,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    87,    88,    89,    90,    91,    92,    -1,    -1,
      -1,    96,    -1,    98,    99,    -1,    -1,   102,   103,    46,
      47,   106,    49,    -1,    -1,   110,   111,   112,    -1,    56,
      57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,    -1,
       9,    -1,    -1,    12,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      87,    88,    89,    90,    91,    92,    -1,    -1,    -1,    96,
      -1,    98,    99,    -1,    -1,   102,   103,    46,    47,   106,
      49,    -1,    -1,   110,   111,   112,    -1,    56,    57,    -1,
      59,    -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    96,    -1,    98,
      99,    -1,    -1,   102,   103,    -1,    -1,   106,    -1,    -1,
      -1,   110,   111,   112,    10,    -1,    12,    13,    -1,    -1,
      16,    -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      36,    37,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    87,    88,    89,    90,    91,    92,    -1,    94,    -1,
      96,    -1,    98,    99,    -1,    -1,   102,   103,    -1,    -1,
     106,    -1,    -1,    -1,   110,   111,   112,    10,    -1,    12,
      13,    -1,    -1,    16,    -1,    -1,    -1,    -1,    -1,    -1,
      23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    37,    -1,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    56,    57,    -1,    59,    -1,    61,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,    92,
      -1,    94,    -1,    96,    -1,    98,    99,    -1,    -1,   102,
     103,    -1,    -1,   106,    -1,    -1,    -1,   110,   111,   112,
      10,    -1,    12,    13,    -1,    -1,    16,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,    59,
      -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,
      90,    91,    92,    -1,    94,    -1,    96,    -1,    98,    99,
      -1,    -1,   102,   103,    -1,    -1,   106,    -1,    -1,    -1,
     110,   111,   112,    10,    -1,    12,    13,    -1,    -1,    16,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      37,    -1,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      87,    88,    89,    90,    91,    92,    -1,    94,    -1,    96,
      -1,    98,    99,    -1,    -1,   102,   103,    -1,    10,   106,
      12,    13,    -1,   110,   111,   112,    -1,    -1,    -1,    -1,
      -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    37,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    57,    -1,    59,    -1,    61,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,
      92,    -1,    94,    -1,    96,    -1,    98,    99,    -1,    -1,
     102,   103,    12,    13,   106,    -1,    16,    -1,   110,   111,
     112,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,    59,
      -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,
      90,    91,    92,    -1,    94,    -1,    96,    -1,    98,    99,
      -1,    -1,   102,   103,    12,    13,   106,    -1,    -1,    -1,
     110,   111,   112,    -1,    -1,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,
      -1,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,
      -1,    59,    -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,
      88,    89,    90,    91,    92,    -1,    94,    95,    96,    -1,
      98,    99,    -1,    -1,   102,   103,    12,    13,   106,    -1,
      16,    -1,   110,   111,   112,    -1,    -1,    23,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      36,    37,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    87,    88,    89,    90,    91,    92,    -1,    94,    -1,
      96,    -1,    98,    99,    -1,    -1,   102,   103,    12,    13,
     106,    -1,    16,    -1,   110,   111,   112,    -1,    -1,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    36,    37,    -1,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    57,    -1,    59,    -1,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    87,    88,    89,    90,    91,    92,    -1,
      94,    -1,    96,    -1,    98,    99,    -1,    -1,   102,   103,
      12,    13,   106,    -1,    16,    -1,   110,   111,   112,    -1,
      -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    37,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    56,    57,    -1,    59,    -1,    61,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,
      92,    -1,    94,    -1,    96,    -1,    98,    99,    -1,    -1,
     102,   103,    12,    13,   106,    -1,    16,    -1,   110,   111,
     112,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,    59,
      -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,
      90,    91,    92,    -1,    94,    -1,    96,    -1,    98,    99,
      -1,    -1,   102,   103,    12,    13,   106,    -1,    16,    -1,
     110,   111,   112,    -1,    -1,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,
      -1,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,
      -1,    59,    -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,
      88,    89,    90,    91,    92,    -1,    94,    -1,    96,    -1,
      98,    99,    -1,    -1,   102,   103,    -1,    -1,   106,    12,
      13,    14,   110,   111,   112,    -1,    -1,    -1,    -1,    -1,
      23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    37,    -1,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    56,    57,    -1,    59,    -1,    61,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,    92,
      -1,    94,    -1,    96,    -1,    98,    99,    -1,    -1,   102,
     103,    12,    13,   106,    -1,    16,    -1,   110,   111,   112,
      -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    57,    -1,    59,    -1,
      61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,    90,
      91,    92,    -1,    94,    -1,    96,    -1,    98,    99,    -1,
      -1,   102,   103,    12,    13,   106,    -1,    -1,    -1,   110,
     111,   112,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,
      59,    -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,
      89,    90,    91,    92,    -1,    94,    -1,    96,    -1,    98,
      99,    -1,    -1,   102,   103,    12,    13,   106,    -1,    -1,
      -1,   110,   111,   112,    -1,    -1,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      -1,    -1,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    56,
      57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      87,    88,    89,    90,    91,    92,    12,    13,    -1,    96,
      -1,    98,    99,    -1,    -1,   102,   103,    23,    -1,   106,
      -1,    -1,    -1,   110,   111,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    87,    88,    89,    90,    91,    92,    12,    13,    -1,
      96,    -1,    98,    99,    -1,    -1,   102,   103,    23,    -1,
     106,    -1,    -1,    -1,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,    -1,    59,    12,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    36,
      37,    -1,    87,    88,    89,    90,    91,    92,    45,    46,
      47,    -1,    49,    98,    99,    -1,    -1,   102,   103,    56,
      57,   106,    59,    -1,    61,   110,   111,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      87,    88,    89,    90,    91,    92,    12,    94,    95,    96,
      -1,    98,    99,    -1,    -1,   102,   103,    23,    -1,   106,
      -1,    -1,    -1,   110,   111,   112,    -1,    -1,    -1,    -1,
      36,    37,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,
      46,    47,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    57,    -1,    59,    -1,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    12,    -1,
      -1,    87,    88,    89,    90,    91,    92,    -1,    94,    23,
      96,    -1,    98,    99,    -1,    -1,   102,   103,    -1,    -1,
     106,    -1,    36,    -1,   110,   111,   112,    -1,    -1,    -1,
      -1,    45,    46,    47,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    57,    -1,    59,    12,    61,    -1,    15,
      16,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    87,    88,    89,    90,    91,    92,    -1,
      46,    47,    96,    49,    98,    99,    -1,    -1,   102,   103,
      56,    57,   106,    59,    -1,    61,   110,   111,   112,    -1,
      -1,    -1,    -1,    -1,    12,    -1,    -1,    15,    16,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      96,    -1,    98,    99,    -1,    -1,   102,   103,    46,    47,
     106,    49,    -1,    -1,   110,   111,   112,    -1,    56,    57,
      -1,    59,    12,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,
      88,    89,    90,    91,    92,    45,    46,    47,    96,    49,
      98,    99,    -1,    -1,   102,   103,    56,    57,   106,    59,
      -1,    61,   110,   111,   112,    -1,    -1,    -1,    -1,    -1,
      12,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,
      90,    91,    92,    -1,    -1,    -1,    96,    -1,    98,    99,
      -1,    -1,   102,   103,    46,    47,   106,    49,    -1,    -1,
     110,   111,   112,    -1,    56,    57,    -1,    59,    12,    61,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,
      92,    -1,    46,    47,    96,    49,    98,    99,    -1,    -1,
     102,   103,    56,    57,   106,    59,    -1,    61,   110,   111,
     112,    -1,    -1,    -1,    -1,    -1,    12,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    96,    -1,    98,    99,    -1,    -1,   102,   103,
      46,    47,   106,    49,    -1,    -1,   110,   111,   112,    -1,
      56,    57,    -1,    59,    12,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    87,    88,    89,    90,    91,    92,    -1,    46,    47,
      96,    49,    98,    99,    -1,    -1,   102,   103,    56,    57,
     106,    59,    -1,    61,   110,   111,   112,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    79,    12,    -1,    -1,    15,    -1,    -1,    -1,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    96,    -1,
      98,    99,    -1,    -1,   102,   103,    -1,    -1,   106,    -1,
      -1,    -1,   110,   111,   112,    -1,    46,    47,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,    59,
      12,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,    89,
      90,    91,    92,    45,    46,    47,    96,    49,    98,    99,
      -1,    -1,   102,   103,    56,    57,   106,    59,    12,    61,
     110,   111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,
      92,    -1,    46,    47,    -1,    49,    98,    99,    -1,    -1,
     102,   103,    56,    57,   106,    59,    -1,    61,   110,   111,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    96,    -1,    98,    99,    -1,    -1,   102,   103,
      -1,    -1,   106,    -1,    -1,    -1,   110,   111,   112
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     9,    14,   115,   130,   132,   144,     0,     7,
       8,    11,    12,    15,    46,    47,    49,    55,    56,    57,
      59,    61,    79,    87,    88,    89,    90,    91,    92,    96,
      98,    99,   102,   103,   106,   110,   111,   112,   127,   133,
     134,   136,   139,   146,   147,   157,   158,   159,   160,   162,
       9,    14,   127,   127,   139,   140,   148,    12,    12,   106,
     159,   160,    87,    90,   125,    12,    12,    12,    12,    43,
     160,    12,    12,   159,   159,   146,   159,   160,   160,   159,
       1,     9,    14,    48,    50,    51,    52,    53,    54,    58,
      62,    63,    77,    78,    97,   100,   119,   121,   126,   127,
     139,   143,   150,   152,   156,   163,    10,   127,   130,    13,
      23,    36,    37,    39,    40,    41,    42,    43,    44,    45,
      94,   116,   117,   159,    12,    92,    15,   102,   103,   104,
     105,   109,    70,   110,   111,    18,   156,   156,    10,    16,
     118,    16,   118,    93,    16,   137,   139,   139,    12,   139,
     139,   137,    16,   137,   159,    43,   139,   139,     9,   128,
     129,    14,   128,   151,   151,   162,   139,   151,    12,    12,
     151,   151,   139,   151,    12,     9,   153,   152,   156,    12,
     138,   141,   142,   146,   159,   160,   151,    17,   152,   155,
     129,   156,   134,    96,   139,   147,   139,   139,   139,   139,
     139,   139,   162,   139,     9,   139,     9,   139,   139,   147,
      70,   159,   159,   159,   159,   159,   159,   139,   137,    17,
      17,     9,   139,    45,   139,    15,    16,   118,    87,   161,
     118,   118,    16,    16,   159,   118,   118,     9,   129,    18,
     151,   131,   150,   162,   139,   151,   139,   152,    80,   120,
      17,   145,   140,    23,    45,    94,   116,   117,   159,   118,
      13,    38,    41,    70,   152,   133,    17,   160,    95,   118,
     118,   159,    19,   162,   139,    16,   118,   149,   139,   147,
     139,   147,   162,   139,   137,    14,    45,   149,   149,   154,
       9,   152,     9,    16,    12,   138,   147,   162,   138,   138,
     138,   138,   159,   159,   159,   138,   127,   139,   139,   139,
      87,     9,   135,    16,    16,    16,    16,    16,   118,    16,
     118,    19,    14,   129,   162,   100,    45,   140,    95,   156,
      16,   118,    16,   118,   127,   139,   147,   139,   129,   139,
     149,    12,   162,    16,   138,    17,   160,   160,   156,    16,
      16,    16,   131,    14,   124,   139,    16,    16,    17,   149,
     129,   152,    16,   123,   131,   151,   152,   149,   122,   152
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   114,   115,   115,   116,   116,   117,   117,   118,   118,
     119,   119,   120,   120,   122,   121,   123,   121,   124,   121,
     125,   125,   126,   127,   127,   128,   128,   129,   129,   130,
     130,   131,   131,   132,   132,   133,   134,   134,   134,   134,
     134,   134,   134,   135,   134,   136,   136,   137,   137,   138,
     138,   138,   138,   138,   138,   138,   138,   138,   138,   138,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   140,
     140,   141,   141,   142,   142,   142,   143,   143,   144,   144,
     144,   144,   145,   145,   146,   146,   148,   147,   149,   149,
     150,   150,   150,   150,   150,   150,   150,   150,   151,   151,
     152,   152,   153,   154,   152,   152,   152,   152,   152,   152,
     152,   152,   152,   152,   152,   152,   155,   152,   152,   156,
     156,   157,   157,   158,   158,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   159,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   159,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   159,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   159,   159,   159,   160,   160,   160,
     160,   161,   161,   161,   162,   162,   162,   163
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
       2,     2,     2,     2,     2,     4,     3,     2,     1,     6,
       6,     3,     6,     6,     1,     8,     8,     6,     4,     1,
       6,     6,     8,     8,     8,     6,     1,     1,     4,     1,
       2,     0,     1,     3,     1,     1,     1,     4
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
#line 99 "awkgram.y"
    { if (errorflag==0)
			winner = (Node *)stat3(PROGRAM, beginloc, (yyvsp[0].p), endloc); }
#line 2448 "awkgram.tab.c"
    break;

  case 3:
#line 101 "awkgram.y"
    { yyclearin; bracecheck(); SYNTAX("bailing out"); }
#line 2454 "awkgram.tab.c"
    break;

  case 14:
#line 125 "awkgram.y"
    {inloop++;}
#line 2460 "awkgram.tab.c"
    break;

  case 15:
#line 126 "awkgram.y"
    { --inloop; (yyval.p) = stat4(FOR, (yyvsp[-9].p), notnull((yyvsp[-6].p)), (yyvsp[-3].p), (yyvsp[0].p)); }
#line 2466 "awkgram.tab.c"
    break;

  case 16:
#line 127 "awkgram.y"
    {inloop++;}
#line 2472 "awkgram.tab.c"
    break;

  case 17:
#line 128 "awkgram.y"
    { --inloop; (yyval.p) = stat4(FOR, (yyvsp[-7].p), NIL, (yyvsp[-3].p), (yyvsp[0].p)); }
#line 2478 "awkgram.tab.c"
    break;

  case 18:
#line 129 "awkgram.y"
    {inloop++;}
#line 2484 "awkgram.tab.c"
    break;

  case 19:
#line 130 "awkgram.y"
    { --inloop; (yyval.p) = stat3(IN, (yyvsp[-5].p), makearr((yyvsp[-3].p)), (yyvsp[0].p)); }
#line 2490 "awkgram.tab.c"
    break;

  case 20:
#line 134 "awkgram.y"
    { setfname((yyvsp[0].cp)); }
#line 2496 "awkgram.tab.c"
    break;

  case 21:
#line 135 "awkgram.y"
    { setfname((yyvsp[0].cp)); }
#line 2502 "awkgram.tab.c"
    break;

  case 22:
#line 139 "awkgram.y"
    { (yyval.p) = notnull((yyvsp[-1].p)); }
#line 2508 "awkgram.tab.c"
    break;

  case 27:
#line 151 "awkgram.y"
    { (yyval.i) = 0; }
#line 2514 "awkgram.tab.c"
    break;

  case 29:
#line 156 "awkgram.y"
    { (yyval.i) = 0; }
#line 2520 "awkgram.tab.c"
    break;

  case 31:
#line 162 "awkgram.y"
    { (yyval.p) = 0; }
#line 2526 "awkgram.tab.c"
    break;

  case 33:
#line 167 "awkgram.y"
    { (yyval.p) = 0; }
#line 2532 "awkgram.tab.c"
    break;

  case 34:
#line 168 "awkgram.y"
    { (yyval.p) = (yyvsp[-1].p); }
#line 2538 "awkgram.tab.c"
    break;

  case 35:
#line 172 "awkgram.y"
    { (yyval.p) = notnull((yyvsp[0].p)); }
#line 2544 "awkgram.tab.c"
    break;

  case 36:
#line 176 "awkgram.y"
    { (yyval.p) = stat2(PASTAT, (yyvsp[0].p), stat2(PRINT, rectonode(), NIL)); }
#line 2550 "awkgram.tab.c"
    break;

  case 37:
#line 177 "awkgram.y"
    { (yyval.p) = stat2(PASTAT, (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 2556 "awkgram.tab.c"
    break;

  case 38:
#line 178 "awkgram.y"
    { (yyval.p) = pa2stat((yyvsp[-3].p), (yyvsp[0].p), stat2(PRINT, rectonode(), NIL)); }
#line 2562 "awkgram.tab.c"
    break;

  case 39:
#line 179 "awkgram.y"
    { (yyval.p) = pa2stat((yyvsp[-6].p), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 2568 "awkgram.tab.c"
    break;

  case 40:
#line 180 "awkgram.y"
    { (yyval.p) = stat2(PASTAT, NIL, (yyvsp[-1].p)); }
#line 2574 "awkgram.tab.c"
    break;

  case 41:
#line 182 "awkgram.y"
    { beginloc = linkum(beginloc, (yyvsp[-1].p)); (yyval.p) = 0; }
#line 2580 "awkgram.tab.c"
    break;

  case 42:
#line 184 "awkgram.y"
    { endloc = linkum(endloc, (yyvsp[-1].p)); (yyval.p) = 0; }
#line 2586 "awkgram.tab.c"
    break;

  case 43:
#line 185 "awkgram.y"
    {infunc = true;}
#line 2592 "awkgram.tab.c"
    break;

  case 44:
#line 186 "awkgram.y"
    { infunc = false; curfname=0; defn((Cell *)(yyvsp[-7].p), (yyvsp[-5].p), (yyvsp[-1].p)); (yyval.p) = 0; }
#line 2598 "awkgram.tab.c"
    break;

  case 46:
#line 191 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2604 "awkgram.tab.c"
    break;

  case 48:
#line 196 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2610 "awkgram.tab.c"
    break;

  case 49:
#line 200 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2616 "awkgram.tab.c"
    break;

  case 50:
#line 202 "awkgram.y"
    { (yyval.p) = op3(CONDEXPR, notnull((yyvsp[-4].p)), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2622 "awkgram.tab.c"
    break;

  case 51:
#line 204 "awkgram.y"
    { (yyval.p) = op2(BOR, notnull((yyvsp[-2].p)), notnull((yyvsp[0].p))); }
#line 2628 "awkgram.tab.c"
    break;

  case 52:
#line 206 "awkgram.y"
    { (yyval.p) = op2(AND, notnull((yyvsp[-2].p)), notnull((yyvsp[0].p))); }
#line 2634 "awkgram.tab.c"
    break;

  case 53:
#line 207 "awkgram.y"
    { (yyval.p) = op3((yyvsp[-1].i), NIL, (yyvsp[-2].p), (Node*)makedfa((yyvsp[0].s), 0)); }
#line 2640 "awkgram.tab.c"
    break;

  case 54:
#line 209 "awkgram.y"
    { if (constnode((yyvsp[0].p)))
			(yyval.p) = op3((yyvsp[-1].i), NIL, (yyvsp[-2].p), (Node*)makedfa(strnode((yyvsp[0].p)), 0));
		  else
			(yyval.p) = op3((yyvsp[-1].i), (Node *)1, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2649 "awkgram.tab.c"
    break;

  case 55:
#line 213 "awkgram.y"
    { (yyval.p) = op2(INTEST, (yyvsp[-2].p), makearr((yyvsp[0].p))); }
#line 2655 "awkgram.tab.c"
    break;

  case 56:
#line 214 "awkgram.y"
    { (yyval.p) = op2(INTEST, (yyvsp[-3].p), makearr((yyvsp[0].p))); }
#line 2661 "awkgram.tab.c"
    break;

  case 57:
#line 215 "awkgram.y"
    { (yyval.p) = op2(CAT, (yyvsp[-1].p), (yyvsp[0].p)); }
#line 2667 "awkgram.tab.c"
    break;

  case 60:
#line 221 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2673 "awkgram.tab.c"
    break;

  case 61:
#line 223 "awkgram.y"
    { (yyval.p) = op3(CONDEXPR, notnull((yyvsp[-4].p)), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2679 "awkgram.tab.c"
    break;

  case 62:
#line 225 "awkgram.y"
    { (yyval.p) = op2(BOR, notnull((yyvsp[-2].p)), notnull((yyvsp[0].p))); }
#line 2685 "awkgram.tab.c"
    break;

  case 63:
#line 227 "awkgram.y"
    { (yyval.p) = op2(AND, notnull((yyvsp[-2].p)), notnull((yyvsp[0].p))); }
#line 2691 "awkgram.tab.c"
    break;

  case 64:
#line 228 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2697 "awkgram.tab.c"
    break;

  case 65:
#line 229 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2703 "awkgram.tab.c"
    break;

  case 66:
#line 230 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2709 "awkgram.tab.c"
    break;

  case 67:
#line 231 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2715 "awkgram.tab.c"
    break;

  case 68:
#line 232 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2721 "awkgram.tab.c"
    break;

  case 69:
#line 233 "awkgram.y"
    { (yyval.p) = op2((yyvsp[-1].i), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2727 "awkgram.tab.c"
    break;

  case 70:
#line 234 "awkgram.y"
    { (yyval.p) = op3((yyvsp[-1].i), NIL, (yyvsp[-2].p), (Node*)makedfa((yyvsp[0].s), 0)); }
#line 2733 "awkgram.tab.c"
    break;

  case 71:
#line 236 "awkgram.y"
    { if (constnode((yyvsp[0].p)))
			(yyval.p) = op3((yyvsp[-1].i), NIL, (yyvsp[-2].p), (Node*)makedfa(strnode((yyvsp[0].p)), 0));
		  else
			(yyval.p) = op3((yyvsp[-1].i), (Node *)1, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2742 "awkgram.tab.c"
    break;

  case 72:
#line 240 "awkgram.y"
    { (yyval.p) = op2(INTEST, (yyvsp[-2].p), makearr((yyvsp[0].p))); }
#line 2748 "awkgram.tab.c"
    break;

  case 73:
#line 241 "awkgram.y"
    { (yyval.p) = op2(INTEST, (yyvsp[-3].p), makearr((yyvsp[0].p))); }
#line 2754 "awkgram.tab.c"
    break;

  case 74:
#line 242 "awkgram.y"
    {
			if (safe) SYNTAX("cmd | getline is unsafe");
			else (yyval.p) = op3(GETLINE, (yyvsp[0].p), itonp((yyvsp[-2].i)), (yyvsp[-3].p)); }
#line 2762 "awkgram.tab.c"
    break;

  case 75:
#line 245 "awkgram.y"
    {
			if (safe) SYNTAX("cmd | getline is unsafe");
			else (yyval.p) = op3(GETLINE, (Node*)0, itonp((yyvsp[-1].i)), (yyvsp[-2].p)); }
#line 2770 "awkgram.tab.c"
    break;

  case 76:
#line 248 "awkgram.y"
    { (yyval.p) = op2(CAT, (yyvsp[-1].p), (yyvsp[0].p)); }
#line 2776 "awkgram.tab.c"
    break;

  case 79:
#line 254 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2782 "awkgram.tab.c"
    break;

  case 80:
#line 255 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2788 "awkgram.tab.c"
    break;

  case 82:
#line 260 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-2].p), (yyvsp[0].p)); }
#line 2794 "awkgram.tab.c"
    break;

  case 83:
#line 264 "awkgram.y"
    { (yyval.p) = rectonode(); }
#line 2800 "awkgram.tab.c"
    break;

  case 85:
#line 266 "awkgram.y"
    { (yyval.p) = (yyvsp[-1].p); }
#line 2806 "awkgram.tab.c"
    break;

  case 94:
#line 283 "awkgram.y"
    { (yyval.p) = op3(MATCH, NIL, rectonode(), (Node*)makedfa((yyvsp[0].s), 0)); }
#line 2812 "awkgram.tab.c"
    break;

  case 95:
#line 284 "awkgram.y"
    { (yyval.p) = op1(NOT, notnull((yyvsp[0].p))); }
#line 2818 "awkgram.tab.c"
    break;

  case 96:
#line 288 "awkgram.y"
    {startreg();}
#line 2824 "awkgram.tab.c"
    break;

  case 97:
#line 288 "awkgram.y"
    { (yyval.s) = (yyvsp[-1].s); }
#line 2830 "awkgram.tab.c"
    break;

  case 100:
#line 296 "awkgram.y"
    {
			if (safe) SYNTAX("print | is unsafe");
			else (yyval.p) = stat3((yyvsp[-3].i), (yyvsp[-2].p), itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 2838 "awkgram.tab.c"
    break;

  case 101:
#line 299 "awkgram.y"
    {
			if (safe) SYNTAX("print >> is unsafe");
			else (yyval.p) = stat3((yyvsp[-3].i), (yyvsp[-2].p), itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 2846 "awkgram.tab.c"
    break;

  case 102:
#line 302 "awkgram.y"
    {
			if (safe) SYNTAX("print > is unsafe");
			else (yyval.p) = stat3((yyvsp[-3].i), (yyvsp[-2].p), itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 2854 "awkgram.tab.c"
    break;

  case 103:
#line 305 "awkgram.y"
    { (yyval.p) = stat3((yyvsp[-1].i), (yyvsp[0].p), NIL, NIL); }
#line 2860 "awkgram.tab.c"
    break;

  case 104:
#line 306 "awkgram.y"
    { (yyval.p) = stat2(DELETE, makearr((yyvsp[-3].p)), (yyvsp[-1].p)); }
#line 2866 "awkgram.tab.c"
    break;

  case 105:
#line 307 "awkgram.y"
    { (yyval.p) = stat2(DELETE, makearr((yyvsp[0].p)), 0); }
#line 2872 "awkgram.tab.c"
    break;

  case 106:
#line 308 "awkgram.y"
    { (yyval.p) = exptostat((yyvsp[0].p)); }
#line 2878 "awkgram.tab.c"
    break;

  case 107:
#line 309 "awkgram.y"
    { yyclearin; SYNTAX("illegal statement"); }
#line 2884 "awkgram.tab.c"
    break;

  case 110:
#line 318 "awkgram.y"
    { if (!inloop) SYNTAX("break illegal outside of loops");
				  (yyval.p) = stat1(BREAK, NIL); }
#line 2891 "awkgram.tab.c"
    break;

  case 111:
#line 320 "awkgram.y"
    {  if (!inloop) SYNTAX("continue illegal outside of loops");
				  (yyval.p) = stat1(CONTINUE, NIL); }
#line 2898 "awkgram.tab.c"
    break;

  case 112:
#line 322 "awkgram.y"
    {inloop++;}
#line 2904 "awkgram.tab.c"
    break;

  case 113:
#line 322 "awkgram.y"
    {--inloop;}
#line 2910 "awkgram.tab.c"
    break;

  case 114:
#line 323 "awkgram.y"
    { (yyval.p) = stat2(DO, (yyvsp[-6].p), notnull((yyvsp[-2].p))); }
#line 2916 "awkgram.tab.c"
    break;

  case 115:
#line 324 "awkgram.y"
    { (yyval.p) = stat1(EXIT, (yyvsp[-1].p)); }
#line 2922 "awkgram.tab.c"
    break;

  case 116:
#line 325 "awkgram.y"
    { (yyval.p) = stat1(EXIT, NIL); }
#line 2928 "awkgram.tab.c"
    break;

  case 118:
#line 327 "awkgram.y"
    { (yyval.p) = stat3(IF, (yyvsp[-3].p), (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2934 "awkgram.tab.c"
    break;

  case 119:
#line 328 "awkgram.y"
    { (yyval.p) = stat3(IF, (yyvsp[-1].p), (yyvsp[0].p), NIL); }
#line 2940 "awkgram.tab.c"
    break;

  case 120:
#line 329 "awkgram.y"
    { (yyval.p) = (yyvsp[-1].p); }
#line 2946 "awkgram.tab.c"
    break;

  case 121:
#line 330 "awkgram.y"
    { if (infunc)
				SYNTAX("next is illegal inside a function");
			  (yyval.p) = stat1(NEXT, NIL); }
#line 2954 "awkgram.tab.c"
    break;

  case 122:
#line 333 "awkgram.y"
    { if (infunc)
				SYNTAX("nextfile is illegal inside a function");
			  (yyval.p) = stat1(NEXTFILE, NIL); }
#line 2962 "awkgram.tab.c"
    break;

  case 123:
#line 336 "awkgram.y"
    { (yyval.p) = stat1(RETURN, (yyvsp[-1].p)); }
#line 2968 "awkgram.tab.c"
    break;

  case 124:
#line 337 "awkgram.y"
    { (yyval.p) = stat1(RETURN, NIL); }
#line 2974 "awkgram.tab.c"
    break;

  case 126:
#line 339 "awkgram.y"
    {inloop++;}
#line 2980 "awkgram.tab.c"
    break;

  case 127:
#line 339 "awkgram.y"
    { --inloop; (yyval.p) = stat2(WHILE, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 2986 "awkgram.tab.c"
    break;

  case 128:
#line 340 "awkgram.y"
    { (yyval.p) = 0; }
#line 2992 "awkgram.tab.c"
    break;

  case 130:
#line 345 "awkgram.y"
    { (yyval.p) = linkum((yyvsp[-1].p), (yyvsp[0].p)); }
#line 2998 "awkgram.tab.c"
    break;

  case 134:
#line 354 "awkgram.y"
    { (yyval.cp) = catstr((yyvsp[-1].cp), (yyvsp[0].cp)); }
#line 3004 "awkgram.tab.c"
    break;

  case 135:
#line 358 "awkgram.y"
    { (yyval.p) = op2(DIVEQ, (yyvsp[-3].p), (yyvsp[0].p)); }
#line 3010 "awkgram.tab.c"
    break;

  case 136:
#line 359 "awkgram.y"
    { (yyval.p) = op2(ADD, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3016 "awkgram.tab.c"
    break;

  case 137:
#line 360 "awkgram.y"
    { (yyval.p) = op2(MINUS, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3022 "awkgram.tab.c"
    break;

  case 138:
#line 361 "awkgram.y"
    { (yyval.p) = op2(MULT, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3028 "awkgram.tab.c"
    break;

  case 139:
#line 362 "awkgram.y"
    { (yyval.p) = op2(DIVIDE, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3034 "awkgram.tab.c"
    break;

  case 140:
#line 363 "awkgram.y"
    { (yyval.p) = op2(MOD, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3040 "awkgram.tab.c"
    break;

  case 141:
#line 364 "awkgram.y"
    { (yyval.p) = op2(POWER, (yyvsp[-2].p), (yyvsp[0].p)); }
#line 3046 "awkgram.tab.c"
    break;

  case 142:
#line 365 "awkgram.y"
    { (yyval.p) = op1(UMINUS, (yyvsp[0].p)); }
#line 3052 "awkgram.tab.c"
    break;

  case 143:
#line 366 "awkgram.y"
    { (yyval.p) = op1(UPLUS, (yyvsp[0].p)); }
#line 3058 "awkgram.tab.c"
    break;

  case 144:
#line 367 "awkgram.y"
    { (yyval.p) = op1(NOT, notnull((yyvsp[0].p))); }
#line 3064 "awkgram.tab.c"
    break;

  case 145:
#line 368 "awkgram.y"
    { (yyval.p) = op2(BLTIN, itonp((yyvsp[-2].i)), rectonode()); }
#line 3070 "awkgram.tab.c"
    break;

  case 146:
#line 369 "awkgram.y"
    { (yyval.p) = op2(BLTIN, itonp((yyvsp[-3].i)), (yyvsp[-1].p)); }
#line 3076 "awkgram.tab.c"
    break;

  case 147:
#line 370 "awkgram.y"
    { (yyval.p) = op2(BLTIN, itonp((yyvsp[0].i)), rectonode()); }
#line 3082 "awkgram.tab.c"
    break;

  case 148:
#line 371 "awkgram.y"
    { (yyval.p) = op2(CALL, celltonode((yyvsp[-2].cp),CVAR), NIL); }
#line 3088 "awkgram.tab.c"
    break;

  case 149:
#line 372 "awkgram.y"
    { (yyval.p) = op2(CALL, celltonode((yyvsp[-3].cp),CVAR), (yyvsp[-1].p)); }
#line 3094 "awkgram.tab.c"
    break;

  case 150:
#line 373 "awkgram.y"
    { (yyval.p) = op1(CLOSE, (yyvsp[0].p)); }
#line 3100 "awkgram.tab.c"
    break;

  case 151:
#line 374 "awkgram.y"
    { (yyval.p) = op1(PREDECR, (yyvsp[0].p)); }
#line 3106 "awkgram.tab.c"
    break;

  case 152:
#line 375 "awkgram.y"
    { (yyval.p) = op1(PREINCR, (yyvsp[0].p)); }
#line 3112 "awkgram.tab.c"
    break;

  case 153:
#line 376 "awkgram.y"
    { (yyval.p) = op1(POSTDECR, (yyvsp[-1].p)); }
#line 3118 "awkgram.tab.c"
    break;

  case 154:
#line 377 "awkgram.y"
    { (yyval.p) = op1(POSTINCR, (yyvsp[-1].p)); }
#line 3124 "awkgram.tab.c"
    break;

  case 155:
#line 378 "awkgram.y"
    { (yyval.p) = op3(GETLINE, (yyvsp[-2].p), itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 3130 "awkgram.tab.c"
    break;

  case 156:
#line 379 "awkgram.y"
    { (yyval.p) = op3(GETLINE, NIL, itonp((yyvsp[-1].i)), (yyvsp[0].p)); }
#line 3136 "awkgram.tab.c"
    break;

  case 157:
#line 380 "awkgram.y"
    { (yyval.p) = op3(GETLINE, (yyvsp[0].p), NIL, NIL); }
#line 3142 "awkgram.tab.c"
    break;

  case 158:
#line 381 "awkgram.y"
    { (yyval.p) = op3(GETLINE, NIL, NIL, NIL); }
#line 3148 "awkgram.tab.c"
    break;

  case 159:
#line 383 "awkgram.y"
    { (yyval.p) = op2(INDEX, (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3154 "awkgram.tab.c"
    break;

  case 160:
#line 385 "awkgram.y"
    { SYNTAX("index() doesn't permit regular expressions");
		  (yyval.p) = op2(INDEX, (yyvsp[-3].p), (Node*)(yyvsp[-1].s)); }
#line 3161 "awkgram.tab.c"
    break;

  case 161:
#line 387 "awkgram.y"
    { (yyval.p) = (yyvsp[-1].p); }
#line 3167 "awkgram.tab.c"
    break;

  case 162:
#line 389 "awkgram.y"
    { (yyval.p) = op3(MATCHFCN, NIL, (yyvsp[-3].p), (Node*)makedfa((yyvsp[-1].s), 1)); }
#line 3173 "awkgram.tab.c"
    break;

  case 163:
#line 391 "awkgram.y"
    { if (constnode((yyvsp[-1].p)))
			(yyval.p) = op3(MATCHFCN, NIL, (yyvsp[-3].p), (Node*)makedfa(strnode((yyvsp[-1].p)), 1));
		  else
			(yyval.p) = op3(MATCHFCN, (Node *)1, (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3182 "awkgram.tab.c"
    break;

  case 164:
#line 395 "awkgram.y"
    { (yyval.p) = celltonode((yyvsp[0].cp), CCON); }
#line 3188 "awkgram.tab.c"
    break;

  case 165:
#line 397 "awkgram.y"
    { (yyval.p) = op4(SPLIT, (yyvsp[-5].p), makearr((yyvsp[-3].p)), (yyvsp[-1].p), (Node*)STRING); }
#line 3194 "awkgram.tab.c"
    break;

  case 166:
#line 399 "awkgram.y"
    { (yyval.p) = op4(SPLIT, (yyvsp[-5].p), makearr((yyvsp[-3].p)), (Node*)makedfa((yyvsp[-1].s), 1), (Node *)REGEXPR); }
#line 3200 "awkgram.tab.c"
    break;

  case 167:
#line 401 "awkgram.y"
    { (yyval.p) = op4(SPLIT, (yyvsp[-3].p), makearr((yyvsp[-1].p)), NIL, (Node*)STRING); }
#line 3206 "awkgram.tab.c"
    break;

  case 168:
#line 402 "awkgram.y"
    { (yyval.p) = op1((yyvsp[-3].i), (yyvsp[-1].p)); }
#line 3212 "awkgram.tab.c"
    break;

  case 169:
#line 403 "awkgram.y"
    { (yyval.p) = celltonode((yyvsp[0].cp), CCON); }
#line 3218 "awkgram.tab.c"
    break;

  case 170:
#line 405 "awkgram.y"
    { (yyval.p) = op4((yyvsp[-5].i), NIL, (Node*)makedfa((yyvsp[-3].s), 1), (yyvsp[-1].p), rectonode()); }
#line 3224 "awkgram.tab.c"
    break;

  case 171:
#line 407 "awkgram.y"
    { if (constnode((yyvsp[-3].p)))
			(yyval.p) = op4((yyvsp[-5].i), NIL, (Node*)makedfa(strnode((yyvsp[-3].p)), 1), (yyvsp[-1].p), rectonode());
		  else
			(yyval.p) = op4((yyvsp[-5].i), (Node *)1, (yyvsp[-3].p), (yyvsp[-1].p), rectonode()); }
#line 3233 "awkgram.tab.c"
    break;

  case 172:
#line 412 "awkgram.y"
    { (yyval.p) = op4((yyvsp[-7].i), NIL, (Node*)makedfa((yyvsp[-5].s), 1), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3239 "awkgram.tab.c"
    break;

  case 173:
#line 414 "awkgram.y"
    { if (constnode((yyvsp[-5].p)))
			(yyval.p) = op4((yyvsp[-7].i), NIL, (Node*)makedfa(strnode((yyvsp[-5].p)), 1), (yyvsp[-3].p), (yyvsp[-1].p));
		  else
			(yyval.p) = op4((yyvsp[-7].i), (Node *)1, (yyvsp[-5].p), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3248 "awkgram.tab.c"
    break;

  case 174:
#line 419 "awkgram.y"
    { (yyval.p) = op3(SUBSTR, (yyvsp[-5].p), (yyvsp[-3].p), (yyvsp[-1].p)); }
#line 3254 "awkgram.tab.c"
    break;

  case 175:
#line 421 "awkgram.y"
    { (yyval.p) = op3(SUBSTR, (yyvsp[-3].p), (yyvsp[-1].p), NIL); }
#line 3260 "awkgram.tab.c"
    break;

  case 178:
#line 427 "awkgram.y"
    { (yyval.p) = op2(ARRAY, makearr((yyvsp[-3].p)), (yyvsp[-1].p)); }
#line 3266 "awkgram.tab.c"
    break;

  case 179:
#line 428 "awkgram.y"
    { (yyval.p) = op1(INDIRECT, celltonode((yyvsp[0].cp), CVAR)); }
#line 3272 "awkgram.tab.c"
    break;

  case 180:
#line 429 "awkgram.y"
    { (yyval.p) = op1(INDIRECT, (yyvsp[0].p)); }
#line 3278 "awkgram.tab.c"
    break;

  case 181:
#line 433 "awkgram.y"
    { arglist = (yyval.p) = 0; }
#line 3284 "awkgram.tab.c"
    break;

  case 182:
#line 434 "awkgram.y"
    { arglist = (yyval.p) = celltonode((yyvsp[0].cp),CVAR); }
#line 3290 "awkgram.tab.c"
    break;

  case 183:
#line 435 "awkgram.y"
    {
			checkdup((yyvsp[-2].p), (yyvsp[0].cp));
			arglist = (yyval.p) = linkum((yyvsp[-2].p),celltonode((yyvsp[0].cp),CVAR)); }
#line 3298 "awkgram.tab.c"
    break;

  case 184:
#line 441 "awkgram.y"
    { (yyval.p) = celltonode((yyvsp[0].cp), CVAR); }
#line 3304 "awkgram.tab.c"
    break;

  case 185:
#line 442 "awkgram.y"
    { (yyval.p) = op1(ARG, itonp((yyvsp[0].i))); }
#line 3310 "awkgram.tab.c"
    break;

  case 186:
#line 443 "awkgram.y"
    { (yyval.p) = op1(VARNF, (Node *) (yyvsp[0].cp)); }
#line 3316 "awkgram.tab.c"
    break;

  case 187:
#line 448 "awkgram.y"
    { (yyval.p) = notnull((yyvsp[-1].p)); }
#line 3322 "awkgram.tab.c"
    break;


#line 3326 "awkgram.tab.c"

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
#line 451 "awkgram.y"


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
