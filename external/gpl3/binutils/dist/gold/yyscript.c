/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 26 "yyscript.y" /* yacc.c:339  */


#include "config.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "script-c.h"


#line 79 "yyscript.c" /* yacc.c:339  */

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
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_YYSCRIPT_H_INCLUDED
# define YY_YY_YYSCRIPT_H_INCLUDED
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
    PLUSEQ = 258,
    MINUSEQ = 259,
    MULTEQ = 260,
    DIVEQ = 261,
    LSHIFTEQ = 262,
    RSHIFTEQ = 263,
    ANDEQ = 264,
    OREQ = 265,
    OROR = 266,
    ANDAND = 267,
    EQ = 268,
    NE = 269,
    LE = 270,
    GE = 271,
    LSHIFT = 272,
    RSHIFT = 273,
    UNARY = 274,
    STRING = 275,
    QUOTED_STRING = 276,
    INTEGER = 277,
    ABSOLUTE = 278,
    ADDR = 279,
    ALIGN_K = 280,
    ALIGNOF = 281,
    ASSERT_K = 282,
    AS_NEEDED = 283,
    AT = 284,
    BIND = 285,
    BLOCK = 286,
    BYTE = 287,
    CONSTANT = 288,
    CONSTRUCTORS = 289,
    COPY = 290,
    CREATE_OBJECT_SYMBOLS = 291,
    DATA_SEGMENT_ALIGN = 292,
    DATA_SEGMENT_END = 293,
    DATA_SEGMENT_RELRO_END = 294,
    DEFINED = 295,
    DSECT = 296,
    ENTRY = 297,
    EXCLUDE_FILE = 298,
    EXTERN = 299,
    FILL = 300,
    FLOAT = 301,
    FORCE_COMMON_ALLOCATION = 302,
    GLOBAL = 303,
    GROUP = 304,
    HIDDEN = 305,
    HLL = 306,
    INCLUDE = 307,
    INHIBIT_COMMON_ALLOCATION = 308,
    INFO = 309,
    INPUT = 310,
    KEEP = 311,
    LEN = 312,
    LENGTH = 313,
    LOADADDR = 314,
    LOCAL = 315,
    LONG = 316,
    MAP = 317,
    MAX_K = 318,
    MEMORY = 319,
    MIN_K = 320,
    NEXT = 321,
    NOCROSSREFS = 322,
    NOFLOAT = 323,
    NOLOAD = 324,
    ONLY_IF_RO = 325,
    ONLY_IF_RW = 326,
    ORG = 327,
    ORIGIN = 328,
    OUTPUT = 329,
    OUTPUT_ARCH = 330,
    OUTPUT_FORMAT = 331,
    OVERLAY = 332,
    PHDRS = 333,
    PROVIDE = 334,
    PROVIDE_HIDDEN = 335,
    QUAD = 336,
    SEARCH_DIR = 337,
    SECTIONS = 338,
    SEGMENT_START = 339,
    SHORT = 340,
    SIZEOF = 341,
    SIZEOF_HEADERS = 342,
    SORT_BY_ALIGNMENT = 343,
    SORT_BY_INIT_PRIORITY = 344,
    SORT_BY_NAME = 345,
    SPECIAL = 346,
    SQUAD = 347,
    STARTUP = 348,
    SUBALIGN = 349,
    SYSLIB = 350,
    TARGET_K = 351,
    TRUNCATE = 352,
    VERSIONK = 353,
    OPTION = 354,
    PARSING_LINKER_SCRIPT = 355,
    PARSING_VERSION_SCRIPT = 356,
    PARSING_DEFSYM = 357,
    PARSING_DYNAMIC_LIST = 358,
    PARSING_SECTIONS_BLOCK = 359,
    PARSING_SECTION_COMMANDS = 360,
    PARSING_MEMORY_DEF = 361
  };
#endif
/* Tokens.  */
#define PLUSEQ 258
#define MINUSEQ 259
#define MULTEQ 260
#define DIVEQ 261
#define LSHIFTEQ 262
#define RSHIFTEQ 263
#define ANDEQ 264
#define OREQ 265
#define OROR 266
#define ANDAND 267
#define EQ 268
#define NE 269
#define LE 270
#define GE 271
#define LSHIFT 272
#define RSHIFT 273
#define UNARY 274
#define STRING 275
#define QUOTED_STRING 276
#define INTEGER 277
#define ABSOLUTE 278
#define ADDR 279
#define ALIGN_K 280
#define ALIGNOF 281
#define ASSERT_K 282
#define AS_NEEDED 283
#define AT 284
#define BIND 285
#define BLOCK 286
#define BYTE 287
#define CONSTANT 288
#define CONSTRUCTORS 289
#define COPY 290
#define CREATE_OBJECT_SYMBOLS 291
#define DATA_SEGMENT_ALIGN 292
#define DATA_SEGMENT_END 293
#define DATA_SEGMENT_RELRO_END 294
#define DEFINED 295
#define DSECT 296
#define ENTRY 297
#define EXCLUDE_FILE 298
#define EXTERN 299
#define FILL 300
#define FLOAT 301
#define FORCE_COMMON_ALLOCATION 302
#define GLOBAL 303
#define GROUP 304
#define HIDDEN 305
#define HLL 306
#define INCLUDE 307
#define INHIBIT_COMMON_ALLOCATION 308
#define INFO 309
#define INPUT 310
#define KEEP 311
#define LEN 312
#define LENGTH 313
#define LOADADDR 314
#define LOCAL 315
#define LONG 316
#define MAP 317
#define MAX_K 318
#define MEMORY 319
#define MIN_K 320
#define NEXT 321
#define NOCROSSREFS 322
#define NOFLOAT 323
#define NOLOAD 324
#define ONLY_IF_RO 325
#define ONLY_IF_RW 326
#define ORG 327
#define ORIGIN 328
#define OUTPUT 329
#define OUTPUT_ARCH 330
#define OUTPUT_FORMAT 331
#define OVERLAY 332
#define PHDRS 333
#define PROVIDE 334
#define PROVIDE_HIDDEN 335
#define QUAD 336
#define SEARCH_DIR 337
#define SECTIONS 338
#define SEGMENT_START 339
#define SHORT 340
#define SIZEOF 341
#define SIZEOF_HEADERS 342
#define SORT_BY_ALIGNMENT 343
#define SORT_BY_INIT_PRIORITY 344
#define SORT_BY_NAME 345
#define SPECIAL 346
#define SQUAD 347
#define STARTUP 348
#define SUBALIGN 349
#define SYSLIB 350
#define TARGET_K 351
#define TRUNCATE 352
#define VERSIONK 353
#define OPTION 354
#define PARSING_LINKER_SCRIPT 355
#define PARSING_VERSION_SCRIPT 356
#define PARSING_DEFSYM 357
#define PARSING_DYNAMIC_LIST 358
#define PARSING_SECTIONS_BLOCK 359
#define PARSING_SECTION_COMMANDS 360
#define PARSING_MEMORY_DEF 361

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 53 "yyscript.y" /* yacc.c:355  */

  /* A string.  */
  struct Parser_string string;
  /* A number.  */
  uint64_t integer;
  /* An expression.  */
  Expression_ptr expr;
  /* An output section header.  */
  struct Parser_output_section_header output_section_header;
  /* An output section trailer.  */
  struct Parser_output_section_trailer output_section_trailer;
  /* A section constraint.  */
  enum Section_constraint constraint;
  /* A complete input section specification.  */
  struct Input_section_spec input_section_spec;
  /* A list of wildcard specifications, with exclusions.  */
  struct Wildcard_sections wildcard_sections;
  /* A single wildcard specification.  */
  struct Wildcard_section wildcard_section;
  /* A list of strings.  */
  String_list_ptr string_list;
  /* Information for a program header.  */
  struct Phdr_info phdr_info;
  /* Used for version scripts and within VERSION {}.  */
  struct Version_dependency_list* deplist;
  struct Version_expression_list* versyms;
  struct Version_tree* versnode;
  enum Script_section_type section_type;

#line 361 "yyscript.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (void* closure);

#endif /* !YY_YY_YYSCRIPT_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 377 "yyscript.c" /* yacc.c:358  */

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
#define YYFINAL  26
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1465

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  130
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  70
/* YYNRULES -- Number of rules.  */
#define YYNRULES  241
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  555

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   361

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   126,     2,     2,     2,    31,    18,     2,
     120,   121,    29,    27,   124,    28,     2,    30,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    13,   125,
      21,     7,    22,    12,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    17,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   128,     2,
       2,   127,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   122,    16,   123,   129,     2,     2,     2,
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
       5,     6,     8,     9,    10,    11,    14,    15,    19,    20,
      23,    24,    25,    26,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   236,   236,   237,   238,   239,   240,   241,   242,   247,
     248,   253,   254,   257,   256,   260,   262,   263,   264,   266,
     272,   279,   280,   283,   282,   286,   289,   288,   292,   294,
     295,   297,   300,   301,   309,   317,   317,   323,   325,   327,
     333,   334,   339,   341,   344,   343,   351,   352,   357,   359,
     360,   362,   366,   365,   374,   376,   374,   393,   398,   403,
     408,   413,   418,   427,   429,   434,   439,   444,   454,   455,
     462,   463,   470,   471,   478,   479,   481,   483,   489,   498,
     500,   505,   507,   512,   515,   521,   524,   529,   531,   537,
     538,   539,   541,   543,   545,   552,   553,   556,   562,   564,
     566,   568,   570,   577,   579,   585,   592,   601,   606,   615,
     620,   625,   630,   639,   644,   663,   682,   691,   693,   700,
     702,   707,   716,   717,   722,   725,   728,   733,   736,   739,
     743,   745,   747,   751,   753,   755,   760,   761,   766,   775,
     777,   784,   785,   793,   798,   809,   818,   820,   826,   832,
     838,   844,   850,   856,   862,   868,   870,   872,   878,   878,
     888,   890,   892,   894,   896,   898,   900,   902,   904,   906,
     908,   910,   912,   914,   916,   918,   920,   922,   924,   926,
     928,   930,   932,   934,   936,   938,   940,   942,   944,   946,
     948,   950,   952,   954,   956,   958,   960,   962,   964,   966,
     968,   970,   975,   980,   982,   990,   996,  1006,  1009,  1010,
    1014,  1020,  1024,  1025,  1029,  1033,  1038,  1045,  1049,  1057,
    1058,  1060,  1062,  1064,  1073,  1078,  1083,  1088,  1095,  1094,
    1105,  1104,  1111,  1116,  1126,  1128,  1135,  1136,  1141,  1142,
    1147,  1148
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "PLUSEQ", "MINUSEQ", "MULTEQ", "DIVEQ",
  "'='", "LSHIFTEQ", "RSHIFTEQ", "ANDEQ", "OREQ", "'?'", "':'", "OROR",
  "ANDAND", "'|'", "'^'", "'&'", "EQ", "NE", "'<'", "'>'", "LE", "GE",
  "LSHIFT", "RSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'", "UNARY", "STRING",
  "QUOTED_STRING", "INTEGER", "ABSOLUTE", "ADDR", "ALIGN_K", "ALIGNOF",
  "ASSERT_K", "AS_NEEDED", "AT", "BIND", "BLOCK", "BYTE", "CONSTANT",
  "CONSTRUCTORS", "COPY", "CREATE_OBJECT_SYMBOLS", "DATA_SEGMENT_ALIGN",
  "DATA_SEGMENT_END", "DATA_SEGMENT_RELRO_END", "DEFINED", "DSECT",
  "ENTRY", "EXCLUDE_FILE", "EXTERN", "FILL", "FLOAT",
  "FORCE_COMMON_ALLOCATION", "GLOBAL", "GROUP", "HIDDEN", "HLL", "INCLUDE",
  "INHIBIT_COMMON_ALLOCATION", "INFO", "INPUT", "KEEP", "LEN", "LENGTH",
  "LOADADDR", "LOCAL", "LONG", "MAP", "MAX_K", "MEMORY", "MIN_K", "NEXT",
  "NOCROSSREFS", "NOFLOAT", "NOLOAD", "ONLY_IF_RO", "ONLY_IF_RW", "ORG",
  "ORIGIN", "OUTPUT", "OUTPUT_ARCH", "OUTPUT_FORMAT", "OVERLAY", "PHDRS",
  "PROVIDE", "PROVIDE_HIDDEN", "QUAD", "SEARCH_DIR", "SECTIONS",
  "SEGMENT_START", "SHORT", "SIZEOF", "SIZEOF_HEADERS",
  "SORT_BY_ALIGNMENT", "SORT_BY_INIT_PRIORITY", "SORT_BY_NAME", "SPECIAL",
  "SQUAD", "STARTUP", "SUBALIGN", "SYSLIB", "TARGET_K", "TRUNCATE",
  "VERSIONK", "OPTION", "PARSING_LINKER_SCRIPT", "PARSING_VERSION_SCRIPT",
  "PARSING_DEFSYM", "PARSING_DYNAMIC_LIST", "PARSING_SECTIONS_BLOCK",
  "PARSING_SECTION_COMMANDS", "PARSING_MEMORY_DEF", "'('", "')'", "'{'",
  "'}'", "','", "';'", "'!'", "'o'", "'l'", "'~'", "$accept", "top",
  "linker_script", "file_cmd", "$@1", "$@2", "$@3", "ignore_cmd",
  "extern_name_list", "$@4", "extern_name_list_body", "input_list",
  "input_list_element", "$@5", "sections_block", "section_block_cmd",
  "$@6", "section_header", "$@7", "$@8", "opt_address_and_section_type",
  "section_type", "opt_at", "opt_align", "opt_subalign", "opt_constraint",
  "section_trailer", "opt_memspec", "opt_at_memspec", "opt_phdr",
  "opt_fill", "section_cmds", "section_cmd", "data_length",
  "input_section_spec", "input_section_no_keep", "wildcard_file",
  "wildcard_sections", "wildcard_section", "exclude_names",
  "wildcard_name", "memory_defs", "memory_def", "memory_attr",
  "memory_origin", "memory_length", "phdrs_defs", "phdr_def", "phdr_type",
  "phdr_info", "assignment", "parse_exp", "$@9", "exp", "defsym_expr",
  "dynamic_list_expr", "dynamic_list_nodes", "dynamic_list_node",
  "version_script", "vers_nodes", "vers_node", "verdep", "vers_tag",
  "vers_defns", "$@10", "$@11", "string", "end", "opt_semicolon",
  "opt_comma", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,    61,   262,   263,
     264,   265,    63,    58,   266,   267,   124,    94,    38,   268,
     269,    60,    62,   270,   271,   272,   273,    43,    45,    42,
      47,    37,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
      40,    41,   123,   125,    44,    59,    33,   111,   108,   126
};
# endif

#define YYPACT_NINF -423

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-423)))

#define YYTABLE_NINF -120

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     365,  -423,    81,   214,  -112,  -423,  -423,  -423,    14,  1300,
    -423,  -423,    33,  -423,    81,  -423,   -93,  -423,    52,   103,
    -423,  -112,  -423,   177,   568,     9,  -423,   -37,   -28,    -1,
    -423,  -423,    15,   214,  -423,    19,   -69,    60,    62,     1,
      64,    80,    82,    73,    84,    90,    86,  -423,  -423,  -423,
     136,   318,  -423,  -423,   214,   208,   217,   104,   108,  -423,
      33,  -423,   113,  -423,   132,   155,   214,  -423,   136,   318,
    -423,  -423,   157,  -423,  -423,   160,   214,   162,  -423,  -423,
    -423,   168,  -423,  -423,  -423,   169,  -423,  -423,   174,  -423,
     136,    38,  -423,    92,  -423,   214,  -423,   195,   214,  -423,
     231,  -423,   214,   214,  -423,   214,   214,   214,  -423,   214,
    -423,   214,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,  -423,  -423,   151,   103,   103,   192,   159,   156,
    -423,  1253,    21,  -423,   214,  -423,  -423,  -423,   137,  -423,
    -423,  -423,    39,   422,  -423,    46,  -423,   214,  -423,   199,
     207,   215,   220,   214,   231,   341,   316,  -423,   -40,  -423,
    -423,   144,   229,     3,    36,   345,   346,   264,  -423,   266,
      81,   267,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,   269,   270,  -423,  -423,  -423,   214,   -17,  1253,
    1253,  -423,   271,   272,   276,   279,   280,   305,   306,   309,
     310,   312,   322,   323,   324,   330,   332,   333,   337,   339,
    -423,  1253,  1253,  1253,  1434,  -423,   335,   265,   340,   343,
    -423,  1156,   420,  1151,   350,   349,   355,   364,   356,   374,
     375,  -423,   376,   397,   398,   399,   400,     7,  -423,  -423,
    -423,   -10,   508,   214,  -423,  -423,    -6,  -423,    70,  -423,
    -423,   402,  -423,   231,  -423,  -423,  -423,   214,  -423,  -423,
     300,  -423,  -423,  -423,   191,  -423,   401,  -423,   103,    96,
     159,   403,  -423,    -8,  -423,  -423,  -423,  1253,   214,  1253,
     214,  1253,  1253,   214,  1253,  1253,  1253,   214,   214,   214,
    1253,  1253,   214,   214,   214,   281,  -423,  -423,  1253,  1253,
    1253,  1253,  1253,  1253,  1253,  1253,  1253,  1253,  1253,  1253,
    1253,  1253,  1253,  1253,  1253,  1253,  1253,  -423,   214,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,   510,   406,   408,   491,
    -423,   153,   214,  -423,   216,  -423,  -423,  -423,  -423,   216,
      53,   216,    53,  -423,   412,   214,   410,   -67,   413,   214,
    -423,  -423,   416,   231,  -423,   415,  -423,    29,  -423,   421,
     426,  -423,  -423,   405,   535,  -423,  -423,  -423,   825,   430,
     342,   431,   392,   862,   432,   714,   882,   751,   433,   434,
     435,   771,   791,   455,   454,   458,  -423,  1414,   731,   841,
     948,   481,   562,   353,   353,   463,   463,   463,   463,   409,
     409,   315,   315,  -423,  -423,  -423,   474,   565,  -423,   583,
    1253,   480,   496,   591,   485,   486,    75,  -423,   488,   490,
     493,   497,  -423,   495,  -423,  -423,  -423,  -423,   611,  -423,
    -423,  -423,    98,   214,   499,    29,   500,    43,  -423,  -423,
     159,   498,   103,   103,  -423,  -423,  -423,  1253,  -423,   214,
    -423,  -423,  1253,  -423,  1253,  -423,  -423,  -423,  1253,  1253,
    -423,  1253,  -423,  1253,  -423,   598,  -423,   902,  1253,   502,
    -423,  -423,   614,  -423,  -423,   216,  -423,  -423,  -423,   216,
    -423,  -423,  -423,   503,  -423,  -423,  -423,   594,  -423,  -423,
     507,   405,   933,   514,   970,   990,  1010,  1041,  1078,  1434,
     214,  -423,   596,  -423,  1098,  1253,   114,  -423,  -423,   105,
     512,  -423,   519,   520,   159,   521,  -423,  -423,  -423,  -423,
    -423,  -423,  -423,  -423,   621,  -423,  -423,  1118,  -423,  -423,
    -423,  -423,  -423,    18,    29,    29,  -423,   214,   154,  -423,
    -423,  -423,  -423,   638,  -423,  -423,  -423,  -423,   214,   512,
    -423,  -423,  -423,  -423,  -423
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    10,     0,     0,     0,    47,    87,   123,     0,     2,
     234,   235,   219,     3,   211,   212,     0,     4,     0,     0,
       5,   207,   208,     6,     7,   241,     1,     0,     0,     0,
      12,    13,     0,     0,    15,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    33,     9,    32,
       0,     0,   224,   225,   232,     0,     0,     0,     0,   213,
     219,   158,     0,   209,     0,     0,     0,    46,     0,    54,
     121,   120,     0,   102,    94,     0,     0,     0,   100,    98,
     101,     0,    99,    97,    88,     0,    90,   103,     0,   107,
       0,   105,   240,   126,   158,     0,    35,     0,     0,    31,
       0,   123,     0,     0,   137,     0,     0,     0,    23,     0,
      26,     0,   237,   236,    29,   158,   158,   158,   158,   158,
     158,   158,   158,   158,     0,     0,     0,     0,   220,     0,
     206,     0,     0,   158,     0,    51,    49,    52,     0,   158,
     158,    96,     0,     0,   158,     0,    89,     0,   122,   129,
       0,     0,     0,     0,     0,     0,     0,    44,   241,    40,
      42,   241,     0,     0,     0,     0,     0,     0,    47,     0,
       0,     0,   147,   148,   149,   150,   146,   151,   152,   153,
     154,   228,     0,     0,   214,   226,   227,   233,     0,     0,
       0,   184,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     189,     0,     0,     0,   159,   185,     0,     0,     0,     0,
      57,     0,    68,     0,     0,     0,     0,     0,   105,     0,
       0,   119,     0,     0,     0,     0,     0,   241,   110,   113,
     125,     0,     0,     0,    28,    11,    36,    37,   241,   158,
      43,     0,    16,     0,    17,    34,    19,     0,    21,   136,
       0,   158,   158,    22,     0,    25,     0,    18,     0,   221,
     222,     0,   215,     0,   217,   164,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   162,   163,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   210,     0,    48,
      87,    65,    64,    66,    63,    67,     0,     0,     0,    70,
      59,     0,     0,    93,     0,   104,    95,   108,    91,     0,
       0,     0,     0,   106,     0,     0,     0,     0,     0,     0,
      38,    14,     0,     0,    41,     0,   140,   141,   139,     0,
       0,    24,    27,   239,     0,   230,   216,   218,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   160,     0,   182,   181,
     180,   179,   178,   172,   173,   176,   177,   174,   175,   170,
     171,   168,   169,   165,   166,   167,     0,     0,    58,     0,
       0,     0,    72,     0,     0,     0,   241,   118,     0,     0,
       0,     0,   109,     0,   127,   131,   130,   132,     0,    30,
      39,   155,   241,     0,     0,   141,     0,   141,   156,   157,
     238,     0,     0,     0,   197,   192,   198,     0,   190,     0,
     200,   196,     0,   203,     0,   188,   195,   193,     0,     0,
     194,     0,   191,     0,    50,    80,    61,     0,     0,     0,
      55,    60,     0,    92,   112,     0,   115,   116,   114,     0,
     128,   158,    45,     0,   158,   143,   138,     0,   142,   229,
       0,   239,     0,     0,     0,     0,     0,     0,     0,   183,
       0,    53,    82,    69,     0,     0,    74,    62,   117,   241,
     241,    20,     0,     0,   223,     0,   199,   205,   201,   202,
     186,   187,   204,    79,     0,    84,    71,     0,    75,    76,
      77,    56,   111,     0,   141,   141,   231,     0,    86,    73,
     134,   133,   135,     0,   145,   144,    81,   158,     0,   241,
     158,    85,    83,    78,   124
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -148,   393,  -423,   479,  -423,  -423,  -423,  -423,  -423,
    -423,   317,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,   329,  -423,  -423,  -423,   509,  -423,  -423,  -245,   171,
     -21,   551,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -422,
      -4,   -83,  -423,   259,  -423,  -423,  -423,   632,   484,  -423,
     641,  -423,   604,   -15,  -423,  -423,    -2,   -60,   165,   -23
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     8,     9,    48,    97,   168,   170,    49,   152,   153,
     246,   158,   159,   251,    23,    67,   219,   137,   138,   506,
     222,   327,   329,   412,   470,   531,   501,   502,   525,   538,
     549,    24,    84,    85,    86,    87,    88,   237,   238,   416,
     239,    25,   148,   242,   428,   543,   164,   259,   357,   436,
      68,   130,   131,   295,    17,    20,    21,    22,    13,    14,
      15,   273,    57,    58,   268,   443,   215,   114,   441,   253
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      16,    18,    93,    89,    62,    50,   248,    51,   136,    -8,
      19,   150,    16,   485,    26,   488,    10,    11,   425,   426,
      90,    69,    91,    10,    11,    10,    11,    10,    11,    60,
     146,    99,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     217,    70,   124,   101,   185,   186,   224,   225,    70,    61,
     427,   232,    10,    11,   135,    70,    52,    53,    71,    10,
      11,   434,    10,    11,   141,    71,    10,    11,   187,    10,
      11,   252,    71,    94,    92,   434,    10,    11,   540,   541,
      54,   149,    95,   151,    55,   418,   155,   420,   160,   422,
     162,   163,   233,   165,   166,   167,    56,   169,   272,   171,
     182,   183,   544,   545,    10,    11,   345,   366,   349,    96,
     435,    89,   230,   104,   256,    10,    11,   257,   343,   185,
     186,    92,   218,    92,   435,    98,    52,    53,    93,   100,
     228,   231,   226,   231,   216,   240,   542,   234,   235,   236,
     220,   247,   160,   187,   234,   235,   236,   147,  -119,   258,
      54,   547,   260,   487,   189,   190,   352,   548,    16,   364,
      10,    11,   191,   192,   193,   194,   195,   196,   359,   360,
     102,   197,   103,   198,   105,   271,   274,   199,   200,   201,
     202,   351,   185,   186,    92,   108,   474,   528,   529,    92,
     106,   321,   107,    12,   109,   432,   111,   322,   203,   204,
      10,    11,   110,   205,   344,   206,   187,    64,   530,   482,
     323,   125,    92,   207,    10,    11,   532,   127,    70,    92,
     126,    64,    65,   128,   208,   324,   209,   210,   132,   346,
      32,   348,    66,   325,   350,    71,    65,    10,    11,    10,
      11,   160,   133,   363,    32,   355,    66,   221,   358,   156,
     112,   113,    69,   212,    10,    11,   213,   254,    92,    40,
      41,   367,   157,   181,   413,   134,   369,   139,   371,   188,
     140,   374,   142,    40,    41,   378,   379,   380,   143,   144,
     383,   384,   385,   298,   145,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   230,   361,   154,   406,   184,   417,   241,
     419,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     415,   243,   231,    10,    11,   356,   244,   231,   231,   231,
     231,   245,   231,   423,   314,   315,   316,   430,   249,   250,
     255,   160,   261,   262,   298,   437,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   263,    89,   265,   267,   318,
     214,   277,   278,   475,   269,   270,   279,   223,   510,   280,
     281,   512,   386,    90,   298,    91,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,    70,   282,   283,   490,   491,   284,
     285,   483,   286,   437,    70,   437,   312,   313,   314,   315,
     316,    71,   287,   288,   289,    10,    11,   493,   275,   276,
     290,    71,   291,   292,   508,    10,    11,   293,   417,   294,
     317,   319,   328,   446,   551,   320,   447,   554,   421,   229,
     333,   296,   297,   231,   332,   334,  -119,   231,     1,     2,
       3,     4,     5,     6,     7,   335,   475,   533,   310,   311,
     312,   313,   314,   315,   316,   336,   337,   338,   523,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   234,   235,   236,   449,   339,   340,   341,
     342,   347,   353,   408,   362,   365,   553,   409,   410,   411,
     440,   424,   437,   437,   429,   546,   368,   431,   370,   433,
     372,   373,   438,   375,   376,   377,   552,   439,   442,   381,
     382,   445,   448,   451,   455,   456,   457,   387,   388,   389,
     390,   391,   392,   393,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   405,   460,    70,   461,   462,
      70,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,    71,   464,   466,    71,    10,    11,
     468,    10,    11,   469,   471,    72,   472,   473,    72,   476,
      73,   477,    74,    73,   478,    74,   480,   479,   481,   484,
     500,   489,   505,    75,   511,   486,    75,   507,    32,   513,
      76,    32,   514,    76,    77,   517,    92,    77,   524,    78,
     534,   535,    78,   537,   536,   550,   354,   264,   414,   407,
     509,   227,   161,    63,   266,    59,   515,    40,    41,    79,
      40,    41,    79,    80,   129,     0,    80,     0,    81,   467,
      82,    81,     0,    82,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   465,     0,
      83,     0,     0,    83,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   492,     0,     0,     0,
       0,   494,     0,   495,     0,     0,     0,   496,   497,     0,
     498,     0,   499,     0,     0,     0,   298,   504,   299,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   298,   527,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   298,     0,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   298,     0,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   298,   452,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,     0,   298,   454,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   298,   458,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   298,   459,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   298,   444,   299,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
       0,     0,   298,   450,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   298,   453,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   298,   503,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   298,   516,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     298,   518,   299,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
     298,   519,   299,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
     298,   520,   299,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   521,   298,   330,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   189,   190,     0,     0,     0,     0,    10,
      11,   191,   192,   193,   194,   195,   196,     0,     0,   522,
     197,     0,   198,     0,   321,     0,   199,   200,   201,   202,
     322,     0,     0,     0,     0,     0,     0,     0,     0,   526,
       0,     0,     0,   323,     0,     0,     0,   203,   204,     0,
       0,     0,   205,     0,   206,     0,     0,     0,   324,   539,
       0,     0,   207,     0,     0,     0,   325,     0,     0,     0,
       0,     0,     0,   208,     0,   209,   210,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   331,     0,     0,     0,     0,   211,   326,     0,     0,
     189,   190,   212,     0,     0,   213,    10,    11,   191,   192,
     193,   194,   195,   196,     0,     0,     0,   197,     0,   198,
       0,     0,     0,   199,   200,   201,   202,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   203,   204,     0,     0,     0,   205,
       0,   206,     0,    10,    11,     0,     0,     0,     0,   207,
      27,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     208,     0,   209,   210,     0,    28,     0,    29,     0,     0,
      30,     0,    31,    32,     0,    33,    34,     0,    35,     0,
       0,     0,     0,   211,     0,     0,     0,    36,     0,   212,
       0,     0,   213,     0,     0,     0,     0,     0,    37,    38,
       0,    39,    40,    41,     0,    42,    43,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    44,
       0,    45,    46,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    47,   298,   463,   299,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,   298,     0,   299,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316
};

static const yytype_int16 yycheck[] =
{
       2,     3,    25,    24,    19,     9,   154,     9,    68,     0,
     122,    94,    14,   435,     0,   437,    33,    34,    85,    86,
      24,    23,    24,    33,    34,    33,    34,    33,    34,   122,
      90,    33,   115,   116,   117,   118,   119,   120,   121,   122,
     123,     3,     4,     5,     6,     7,     8,     9,    10,    11,
     133,    12,    54,   122,    33,    34,   139,   140,    12,     7,
     127,   144,    33,    34,    66,    12,    33,    34,    29,    33,
      34,    42,    33,    34,    76,    29,    33,    34,    57,    33,
      34,   121,    29,   120,   124,    42,    33,    34,    70,    71,
      57,    93,   120,    95,    61,   340,    98,   342,   100,   344,
     102,   103,    56,   105,   106,   107,    73,   109,   125,   111,
     125,   126,   534,   535,    33,    34,   126,   125,   124,   120,
      91,   142,   143,   122,   121,    33,    34,   124,   121,    33,
      34,   124,   134,   124,    91,   120,    33,    34,   161,   120,
     142,   143,   103,   145,   123,   147,   128,   101,   102,   103,
      13,   153,   154,    57,   101,   102,   103,    65,   120,   123,
      57,     7,   164,   120,    27,    28,   249,    13,   170,    73,
      33,    34,    35,    36,    37,    38,    39,    40,   261,   262,
     120,    44,   120,    46,   120,   187,   188,    50,    51,    52,
      53,   121,    33,    34,   124,   122,   121,    83,    84,   124,
     120,    48,   120,   122,   120,   353,   120,    54,    71,    72,
      33,    34,   122,    76,   237,    78,    57,    40,   104,   121,
      67,    13,   124,    86,    33,    34,   121,   123,    12,   124,
      13,    40,    55,   125,    97,    82,    99,   100,   125,   241,
      63,   243,    65,    90,   246,    29,    55,    33,    34,    33,
      34,   253,   120,   268,    63,   257,    65,   120,   260,    28,
     124,   125,   264,   126,    33,    34,   129,   123,   124,    92,
      93,   273,    41,   122,   121,   120,   278,   120,   280,   123,
     120,   283,   120,    92,    93,   287,   288,   289,   120,   120,
     292,   293,   294,    12,   120,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,   334,   123,   120,   318,   125,   339,   120,
     341,     3,     4,     5,     6,     7,     8,     9,    10,    11,
     332,   124,   334,    33,    34,    35,   121,   339,   340,   341,
     342,   121,   344,   345,    29,    30,    31,   349,     7,    33,
     121,   353,     7,     7,    12,   357,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,   121,   407,   121,   121,   124,
     131,   120,   120,   416,   125,   125,   120,   138,   481,   120,
     120,   484,   121,   407,    12,   407,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    12,   120,   120,   442,   443,   120,
     120,   433,   120,   435,    12,   437,    27,    28,    29,    30,
      31,    29,   120,   120,   120,    33,    34,   449,   189,   190,
     120,    29,   120,   120,   475,    33,    34,   120,   479,   120,
     125,   121,    42,   121,   547,   122,   124,   550,    56,    47,
     121,   212,   213,   475,   124,   120,   120,   479,   113,   114,
     115,   116,   117,   118,   119,   121,   509,   510,    25,    26,
      27,    28,    29,    30,    31,   121,   121,   121,   500,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,   101,   102,   103,   124,   120,   120,   120,
     120,    13,   120,    13,   123,   122,   549,   121,   120,    38,
     125,   121,   534,   535,   121,   537,   277,   121,   279,   124,
     281,   282,   121,   284,   285,   286,   548,   121,    13,   290,
     291,   121,   121,   121,   121,   121,   121,   298,   299,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,   121,    12,   124,   121,
      12,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    29,   121,    13,    29,    33,    34,
     120,    33,    34,   107,    13,    40,   121,   121,    40,   121,
      45,   121,    47,    45,   121,    47,   121,   120,     7,   120,
      22,   123,   120,    58,   121,   125,    58,    13,    63,    35,
      65,    63,   125,    65,    69,   121,   124,    69,    42,    74,
     121,   121,    74,    22,   123,     7,   253,   168,   331,   320,
     479,   142,   101,    21,   170,    14,   491,    92,    93,    94,
      92,    93,    94,    98,    60,    -1,    98,    -1,   103,   410,
     105,   103,    -1,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   123,    -1,
     125,    -1,    -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   447,    -1,    -1,    -1,
      -1,   452,    -1,   454,    -1,    -1,    -1,   458,   459,    -1,
     461,    -1,   463,    -1,    -1,    -1,    12,   468,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    12,   505,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    12,    -1,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    12,    -1,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    12,   124,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    12,   124,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    12,   124,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    12,   124,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    12,   121,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    12,   121,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    12,   121,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    12,   121,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    12,   121,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      12,   121,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      12,   121,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      12,   121,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    27,    28,    -1,    -1,    -1,    -1,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,   121,
      44,    -1,    46,    -1,    48,    -1,    50,    51,    52,    53,
      54,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,
      -1,    -1,    -1,    67,    -1,    -1,    -1,    71,    72,    -1,
      -1,    -1,    76,    -1,    78,    -1,    -1,    -1,    82,   121,
      -1,    -1,    86,    -1,    -1,    -1,    90,    -1,    -1,    -1,
      -1,    -1,    -1,    97,    -1,    99,   100,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   120,    -1,    -1,    -1,    -1,   120,   121,    -1,    -1,
      27,    28,   126,    -1,    -1,   129,    33,    34,    35,    36,
      37,    38,    39,    40,    -1,    -1,    -1,    44,    -1,    46,
      -1,    -1,    -1,    50,    51,    52,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    72,    -1,    -1,    -1,    76,
      -1,    78,    -1,    33,    34,    -1,    -1,    -1,    -1,    86,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      97,    -1,    99,   100,    -1,    55,    -1,    57,    -1,    -1,
      60,    -1,    62,    63,    -1,    65,    66,    -1,    68,    -1,
      -1,    -1,    -1,   120,    -1,    -1,    -1,    77,    -1,   126,
      -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,    88,    89,
      -1,    91,    92,    93,    -1,    95,    96,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,
      -1,   111,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   125,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    12,    -1,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   113,   114,   115,   116,   117,   118,   119,   131,   132,
      33,    34,   122,   188,   189,   190,   196,   184,   196,   122,
     185,   186,   187,   144,   161,   171,     0,    40,    55,    57,
      60,    62,    63,    65,    66,    68,    77,    88,    89,    91,
      92,    93,    95,    96,   109,   111,   112,   125,   133,   137,
     180,   196,    33,    34,    57,    61,    73,   192,   193,   190,
     122,     7,   193,   187,    40,    55,    65,   145,   180,   196,
      12,    29,    40,    45,    47,    58,    65,    69,    74,    94,
      98,   103,   105,   125,   162,   163,   164,   165,   166,   170,
     180,   196,   124,   199,   120,   120,   120,   134,   120,   196,
     120,   122,   120,   120,   122,   120,   120,   120,   122,   120,
     122,   120,   124,   125,   197,     3,     4,     5,     6,     7,
       8,     9,    10,    11,   196,    13,    13,   123,   125,   192,
     181,   182,   125,   120,   120,   196,   197,   147,   148,   120,
     120,   196,   120,   120,   120,   120,   197,    65,   172,   196,
     181,   196,   138,   139,   120,   196,    28,    41,   141,   142,
     196,   171,   196,   196,   176,   196,   196,   196,   135,   196,
     136,   196,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   122,   193,   193,   125,    33,    34,    57,   123,    27,
      28,    35,    36,    37,    38,    39,    40,    44,    46,    50,
      51,    52,    53,    71,    72,    76,    78,    86,    97,    99,
     100,   120,   126,   129,   183,   196,   123,   181,   196,   146,
      13,   120,   150,   183,   181,   181,   103,   165,   196,    47,
     170,   196,   181,    56,   101,   102,   103,   167,   168,   170,
     196,   120,   173,   124,   121,   121,   140,   196,   141,     7,
      33,   143,   121,   199,   123,   121,   121,   124,   123,   177,
     196,     7,     7,   121,   144,   121,   188,   121,   194,   125,
     125,   196,   125,   191,   196,   183,   183,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   183,   183,   183,    12,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,   125,   124,   121,
     122,    48,    54,    67,    82,    90,   121,   151,    42,   152,
      13,   120,   124,   121,   120,   121,   121,   121,   121,   120,
     120,   120,   120,   121,   199,   126,   196,    13,   196,   124,
     196,   121,   181,   120,   142,   196,    35,   178,   196,   181,
     181,   123,   123,   193,    73,   122,   125,   196,   183,   196,
     183,   196,   183,   183,   196,   183,   183,   183,   196,   196,
     196,   183,   183,   196,   196,   196,   121,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   196,   161,    13,   121,
     120,    38,   153,   121,   151,   196,   169,   170,   168,   170,
     168,    56,   168,   196,   121,    85,    86,   127,   174,   121,
     196,   121,   141,   124,    42,    91,   179,   196,   121,   121,
     125,   198,    13,   195,   121,   121,   121,   124,   121,   124,
     121,   121,   124,   121,   124,   121,   121,   121,   124,   124,
     121,   124,   121,    13,   121,   123,    13,   183,   120,   107,
     154,    13,   121,   121,   121,   199,   121,   121,   121,   120,
     121,     7,   121,   196,   120,   179,   125,   120,   179,   123,
     193,   193,   183,   196,   183,   183,   183,   183,   183,   183,
      22,   156,   157,   121,   183,   120,   149,    13,   170,   169,
     181,   121,   181,    35,   125,   198,   121,   121,   121,   121,
     121,   121,   121,   196,    42,   158,   121,   183,    83,    84,
     104,   155,   121,   199,   121,   121,   123,    22,   159,   121,
      70,    71,   128,   175,   179,   179,   196,     7,    13,   160,
       7,   181,   196,   199,   181
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   130,   131,   131,   131,   131,   131,   131,   131,   132,
     132,   133,   133,   134,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   135,   133,   133,   136,   133,   133,   133,
     133,   133,   133,   133,   137,   139,   138,   140,   140,   140,
     141,   141,   142,   142,   143,   142,   144,   144,   145,   145,
     145,   145,   146,   145,   148,   149,   147,   150,   150,   150,
     150,   150,   150,   151,   151,   151,   151,   151,   152,   152,
     153,   153,   154,   154,   155,   155,   155,   155,   156,   157,
     157,   158,   158,   159,   159,   160,   160,   161,   161,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   163,   163,
     163,   163,   163,   164,   164,   165,   165,   166,   166,   167,
     167,   167,   167,   168,   168,   168,   168,   169,   169,   170,
     170,   170,   171,   171,   172,   172,   172,   173,   173,   173,
     174,   174,   174,   175,   175,   175,   176,   176,   177,   178,
     178,   179,   179,   179,   179,   179,   180,   180,   180,   180,
     180,   180,   180,   180,   180,   180,   180,   180,   182,   181,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   184,   185,   186,   186,
     187,   188,   189,   189,   190,   190,   190,   191,   191,   192,
     192,   192,   192,   192,   193,   193,   193,   193,   194,   193,
     195,   193,   193,   193,   196,   196,   197,   197,   198,   198,
     199,   199
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       0,     4,     1,     0,     5,     1,     4,     4,     4,     4,
       8,     4,     4,     0,     5,     4,     0,     5,     4,     2,
       6,     2,     1,     1,     4,     0,     2,     1,     2,     3,
       1,     3,     1,     2,     0,     5,     2,     0,     4,     2,
       6,     2,     0,     7,     0,     0,     7,     1,     3,     2,
       4,     4,     5,     1,     1,     1,     1,     1,     0,     4,
       0,     4,     0,     4,     0,     1,     1,     1,     5,     2,
       0,     3,     0,     3,     0,     2,     0,     0,     2,     2,
       1,     4,     6,     4,     1,     4,     2,     1,     1,     1,
       1,     1,     1,     1,     4,     1,     4,     1,     4,     3,
       1,     6,     4,     1,     4,     4,     4,     3,     1,     1,
       1,     1,     3,     0,    10,     2,     0,     3,     4,     0,
       1,     1,     1,     1,     1,     1,     2,     0,     4,     1,
       1,     0,     2,     2,     5,     5,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     6,     6,     6,     0,     2,
       3,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     1,     1,     6,     6,     4,     1,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     6,
       4,     6,     6,     4,     6,     6,     3,     1,     1,     2,
       5,     1,     1,     2,     4,     5,     6,     1,     2,     0,
       2,     4,     4,     8,     1,     1,     3,     3,     0,     7,
       0,     9,     1,     3,     1,     1,     1,     1,     1,     0,
       1,     0
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
      yyerror (closure, YY_("syntax error: cannot back up")); \
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
                  Type, Value, closure); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void* closure)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (closure);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void* closure)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, closure);
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, void* closure)
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
                                              , closure);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, closure); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void* closure)
{
  YYUSE (yyvaluep);
  YYUSE (closure);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void* closure)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

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
      yychar = yylex (&yylval, closure);
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
        case 12:
#line 255 "yyscript.y" /* yacc.c:1648  */
    { script_set_common_allocation(closure, 1); }
#line 2049 "yyscript.c" /* yacc.c:1648  */
    break;

  case 13:
#line 257 "yyscript.y" /* yacc.c:1648  */
    { script_start_group(closure); }
#line 2055 "yyscript.c" /* yacc.c:1648  */
    break;

  case 14:
#line 259 "yyscript.y" /* yacc.c:1648  */
    { script_end_group(closure); }
#line 2061 "yyscript.c" /* yacc.c:1648  */
    break;

  case 15:
#line 261 "yyscript.y" /* yacc.c:1648  */
    { script_set_common_allocation(closure, 0); }
#line 2067 "yyscript.c" /* yacc.c:1648  */
    break;

  case 18:
#line 265 "yyscript.y" /* yacc.c:1648  */
    { script_parse_option(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 2073 "yyscript.c" /* yacc.c:1648  */
    break;

  case 19:
#line 267 "yyscript.y" /* yacc.c:1648  */
    {
	      if (!script_check_output_format(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length,
					      NULL, 0, NULL, 0))
		YYABORT;
	    }
#line 2083 "yyscript.c" /* yacc.c:1648  */
    break;

  case 20:
#line 273 "yyscript.y" /* yacc.c:1648  */
    {
	      if (!script_check_output_format(closure, (yyvsp[-5].string).value, (yyvsp[-5].string).length,
					      (yyvsp[-3].string).value, (yyvsp[-3].string).length,
					      (yyvsp[-1].string).value, (yyvsp[-1].string).length))
		YYABORT;
	    }
#line 2094 "yyscript.c" /* yacc.c:1648  */
    break;

  case 22:
#line 281 "yyscript.y" /* yacc.c:1648  */
    { script_add_search_dir(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 2100 "yyscript.c" /* yacc.c:1648  */
    break;

  case 23:
#line 283 "yyscript.y" /* yacc.c:1648  */
    { script_start_sections(closure); }
#line 2106 "yyscript.c" /* yacc.c:1648  */
    break;

  case 24:
#line 285 "yyscript.y" /* yacc.c:1648  */
    { script_finish_sections(closure); }
#line 2112 "yyscript.c" /* yacc.c:1648  */
    break;

  case 25:
#line 287 "yyscript.y" /* yacc.c:1648  */
    { script_set_target(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 2118 "yyscript.c" /* yacc.c:1648  */
    break;

  case 26:
#line 289 "yyscript.y" /* yacc.c:1648  */
    { script_push_lex_into_version_mode(closure); }
#line 2124 "yyscript.c" /* yacc.c:1648  */
    break;

  case 27:
#line 291 "yyscript.y" /* yacc.c:1648  */
    { script_pop_lex_mode(closure); }
#line 2130 "yyscript.c" /* yacc.c:1648  */
    break;

  case 28:
#line 293 "yyscript.y" /* yacc.c:1648  */
    { script_set_entry(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 2136 "yyscript.c" /* yacc.c:1648  */
    break;

  case 30:
#line 296 "yyscript.y" /* yacc.c:1648  */
    { script_add_assertion(closure, (yyvsp[-3].expr), (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 2142 "yyscript.c" /* yacc.c:1648  */
    break;

  case 31:
#line 298 "yyscript.y" /* yacc.c:1648  */
    { script_include_directive(PARSING_LINKER_SCRIPT, closure,
				       (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2149 "yyscript.c" /* yacc.c:1648  */
    break;

  case 35:
#line 317 "yyscript.y" /* yacc.c:1648  */
    { script_push_lex_into_expression_mode(closure); }
#line 2155 "yyscript.c" /* yacc.c:1648  */
    break;

  case 36:
#line 319 "yyscript.y" /* yacc.c:1648  */
    { script_pop_lex_mode(closure); }
#line 2161 "yyscript.c" /* yacc.c:1648  */
    break;

  case 37:
#line 324 "yyscript.y" /* yacc.c:1648  */
    { script_add_extern(closure, (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2167 "yyscript.c" /* yacc.c:1648  */
    break;

  case 38:
#line 326 "yyscript.y" /* yacc.c:1648  */
    { script_add_extern(closure, (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2173 "yyscript.c" /* yacc.c:1648  */
    break;

  case 39:
#line 328 "yyscript.y" /* yacc.c:1648  */
    { script_add_extern(closure, (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2179 "yyscript.c" /* yacc.c:1648  */
    break;

  case 42:
#line 340 "yyscript.y" /* yacc.c:1648  */
    { script_add_file(closure, (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2185 "yyscript.c" /* yacc.c:1648  */
    break;

  case 43:
#line 342 "yyscript.y" /* yacc.c:1648  */
    { script_add_library(closure, (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2191 "yyscript.c" /* yacc.c:1648  */
    break;

  case 44:
#line 344 "yyscript.y" /* yacc.c:1648  */
    { script_start_as_needed(closure); }
#line 2197 "yyscript.c" /* yacc.c:1648  */
    break;

  case 45:
#line 346 "yyscript.y" /* yacc.c:1648  */
    { script_end_as_needed(closure); }
#line 2203 "yyscript.c" /* yacc.c:1648  */
    break;

  case 48:
#line 358 "yyscript.y" /* yacc.c:1648  */
    { script_set_entry(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 2209 "yyscript.c" /* yacc.c:1648  */
    break;

  case 50:
#line 361 "yyscript.y" /* yacc.c:1648  */
    { script_add_assertion(closure, (yyvsp[-3].expr), (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 2215 "yyscript.c" /* yacc.c:1648  */
    break;

  case 51:
#line 363 "yyscript.y" /* yacc.c:1648  */
    { script_include_directive(PARSING_SECTIONS_BLOCK, closure,
				       (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2222 "yyscript.c" /* yacc.c:1648  */
    break;

  case 52:
#line 366 "yyscript.y" /* yacc.c:1648  */
    { script_start_output_section(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length, &(yyvsp[0].output_section_header)); }
#line 2228 "yyscript.c" /* yacc.c:1648  */
    break;

  case 53:
#line 368 "yyscript.y" /* yacc.c:1648  */
    { script_finish_output_section(closure, &(yyvsp[0].output_section_trailer)); }
#line 2234 "yyscript.c" /* yacc.c:1648  */
    break;

  case 54:
#line 374 "yyscript.y" /* yacc.c:1648  */
    { script_push_lex_into_expression_mode(closure); }
#line 2240 "yyscript.c" /* yacc.c:1648  */
    break;

  case 55:
#line 376 "yyscript.y" /* yacc.c:1648  */
    { script_pop_lex_mode(closure); }
#line 2246 "yyscript.c" /* yacc.c:1648  */
    break;

  case 56:
#line 378 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.output_section_header).address = (yyvsp[-5].output_section_header).address;
	      (yyval.output_section_header).section_type = (yyvsp[-5].output_section_header).section_type;
	      (yyval.output_section_header).load_address = (yyvsp[-4].expr);
	      (yyval.output_section_header).align = (yyvsp[-3].expr);
	      (yyval.output_section_header).subalign = (yyvsp[-2].expr);
	      (yyval.output_section_header).constraint = (yyvsp[0].constraint);
	    }
#line 2259 "yyscript.c" /* yacc.c:1648  */
    break;

  case 57:
#line 394 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.output_section_header).address = NULL;
	      (yyval.output_section_header).section_type = SCRIPT_SECTION_TYPE_NONE;
	    }
#line 2268 "yyscript.c" /* yacc.c:1648  */
    break;

  case 58:
#line 399 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.output_section_header).address = NULL;
	      (yyval.output_section_header).section_type = SCRIPT_SECTION_TYPE_NONE;
	    }
#line 2277 "yyscript.c" /* yacc.c:1648  */
    break;

  case 59:
#line 404 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.output_section_header).address = (yyvsp[-1].expr);
	      (yyval.output_section_header).section_type = SCRIPT_SECTION_TYPE_NONE;
	    }
#line 2286 "yyscript.c" /* yacc.c:1648  */
    break;

  case 60:
#line 409 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.output_section_header).address = (yyvsp[-3].expr);
	      (yyval.output_section_header).section_type = SCRIPT_SECTION_TYPE_NONE;
	    }
#line 2295 "yyscript.c" /* yacc.c:1648  */
    break;

  case 61:
#line 414 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.output_section_header).address = NULL;
	      (yyval.output_section_header).section_type = (yyvsp[-2].section_type);
	    }
#line 2304 "yyscript.c" /* yacc.c:1648  */
    break;

  case 62:
#line 419 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.output_section_header).address = (yyvsp[-4].expr);
	      (yyval.output_section_header).section_type = (yyvsp[-2].section_type);
	    }
#line 2313 "yyscript.c" /* yacc.c:1648  */
    break;

  case 63:
#line 428 "yyscript.y" /* yacc.c:1648  */
    { (yyval.section_type) = SCRIPT_SECTION_TYPE_NOLOAD; }
#line 2319 "yyscript.c" /* yacc.c:1648  */
    break;

  case 64:
#line 430 "yyscript.y" /* yacc.c:1648  */
    {
	      yyerror(closure, "DSECT section type is unsupported");
	      (yyval.section_type) = SCRIPT_SECTION_TYPE_DSECT;
	    }
#line 2328 "yyscript.c" /* yacc.c:1648  */
    break;

  case 65:
#line 435 "yyscript.y" /* yacc.c:1648  */
    {
	      yyerror(closure, "COPY section type is unsupported");
	      (yyval.section_type) = SCRIPT_SECTION_TYPE_COPY;
	    }
#line 2337 "yyscript.c" /* yacc.c:1648  */
    break;

  case 66:
#line 440 "yyscript.y" /* yacc.c:1648  */
    {
	      yyerror(closure, "INFO section type is unsupported");
	      (yyval.section_type) = SCRIPT_SECTION_TYPE_INFO;
	    }
#line 2346 "yyscript.c" /* yacc.c:1648  */
    break;

  case 67:
#line 445 "yyscript.y" /* yacc.c:1648  */
    {
	      yyerror(closure, "OVERLAY section type is unsupported");
	      (yyval.section_type) = SCRIPT_SECTION_TYPE_OVERLAY;
	    }
#line 2355 "yyscript.c" /* yacc.c:1648  */
    break;

  case 68:
#line 454 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = NULL; }
#line 2361 "yyscript.c" /* yacc.c:1648  */
    break;

  case 69:
#line 456 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2367 "yyscript.c" /* yacc.c:1648  */
    break;

  case 70:
#line 462 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = NULL; }
#line 2373 "yyscript.c" /* yacc.c:1648  */
    break;

  case 71:
#line 464 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2379 "yyscript.c" /* yacc.c:1648  */
    break;

  case 72:
#line 470 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = NULL; }
#line 2385 "yyscript.c" /* yacc.c:1648  */
    break;

  case 73:
#line 472 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2391 "yyscript.c" /* yacc.c:1648  */
    break;

  case 74:
#line 478 "yyscript.y" /* yacc.c:1648  */
    { (yyval.constraint) = CONSTRAINT_NONE; }
#line 2397 "yyscript.c" /* yacc.c:1648  */
    break;

  case 75:
#line 480 "yyscript.y" /* yacc.c:1648  */
    { (yyval.constraint) = CONSTRAINT_ONLY_IF_RO; }
#line 2403 "yyscript.c" /* yacc.c:1648  */
    break;

  case 76:
#line 482 "yyscript.y" /* yacc.c:1648  */
    { (yyval.constraint) = CONSTRAINT_ONLY_IF_RW; }
#line 2409 "yyscript.c" /* yacc.c:1648  */
    break;

  case 77:
#line 484 "yyscript.y" /* yacc.c:1648  */
    { (yyval.constraint) = CONSTRAINT_SPECIAL; }
#line 2415 "yyscript.c" /* yacc.c:1648  */
    break;

  case 78:
#line 490 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.output_section_trailer).fill = (yyvsp[-1].expr);
	      (yyval.output_section_trailer).phdrs = (yyvsp[-2].string_list);
	    }
#line 2424 "yyscript.c" /* yacc.c:1648  */
    break;

  case 79:
#line 499 "yyscript.y" /* yacc.c:1648  */
    { script_set_section_region(closure, (yyvsp[0].string).value, (yyvsp[0].string).length, 1); }
#line 2430 "yyscript.c" /* yacc.c:1648  */
    break;

  case 81:
#line 506 "yyscript.y" /* yacc.c:1648  */
    { script_set_section_region(closure, (yyvsp[0].string).value, (yyvsp[0].string).length, 0); }
#line 2436 "yyscript.c" /* yacc.c:1648  */
    break;

  case 83:
#line 513 "yyscript.y" /* yacc.c:1648  */
    { (yyval.string_list) = script_string_list_push_back((yyvsp[-2].string_list), (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2442 "yyscript.c" /* yacc.c:1648  */
    break;

  case 84:
#line 515 "yyscript.y" /* yacc.c:1648  */
    { (yyval.string_list) = NULL; }
#line 2448 "yyscript.c" /* yacc.c:1648  */
    break;

  case 85:
#line 522 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2454 "yyscript.c" /* yacc.c:1648  */
    break;

  case 86:
#line 524 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = NULL; }
#line 2460 "yyscript.c" /* yacc.c:1648  */
    break;

  case 91:
#line 540 "yyscript.y" /* yacc.c:1648  */
    { script_add_data(closure, (yyvsp[-3].integer), (yyvsp[-1].expr)); }
#line 2466 "yyscript.c" /* yacc.c:1648  */
    break;

  case 92:
#line 542 "yyscript.y" /* yacc.c:1648  */
    { script_add_assertion(closure, (yyvsp[-3].expr), (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 2472 "yyscript.c" /* yacc.c:1648  */
    break;

  case 93:
#line 544 "yyscript.y" /* yacc.c:1648  */
    { script_add_fill(closure, (yyvsp[-1].expr)); }
#line 2478 "yyscript.c" /* yacc.c:1648  */
    break;

  case 94:
#line 546 "yyscript.y" /* yacc.c:1648  */
    {
	      /* The GNU linker uses CONSTRUCTORS for the a.out object
		 file format.  It does nothing when using ELF.  Since
		 some ELF linker scripts use it although it does
		 nothing, we accept it and ignore it.  */
	    }
#line 2489 "yyscript.c" /* yacc.c:1648  */
    break;

  case 96:
#line 554 "yyscript.y" /* yacc.c:1648  */
    { script_include_directive(PARSING_SECTION_COMMANDS, closure,
				       (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2496 "yyscript.c" /* yacc.c:1648  */
    break;

  case 98:
#line 563 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = QUAD; }
#line 2502 "yyscript.c" /* yacc.c:1648  */
    break;

  case 99:
#line 565 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = SQUAD; }
#line 2508 "yyscript.c" /* yacc.c:1648  */
    break;

  case 100:
#line 567 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = LONG; }
#line 2514 "yyscript.c" /* yacc.c:1648  */
    break;

  case 101:
#line 569 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = SHORT; }
#line 2520 "yyscript.c" /* yacc.c:1648  */
    break;

  case 102:
#line 571 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = BYTE; }
#line 2526 "yyscript.c" /* yacc.c:1648  */
    break;

  case 103:
#line 578 "yyscript.y" /* yacc.c:1648  */
    { script_add_input_section(closure, &(yyvsp[0].input_section_spec), 0); }
#line 2532 "yyscript.c" /* yacc.c:1648  */
    break;

  case 104:
#line 580 "yyscript.y" /* yacc.c:1648  */
    { script_add_input_section(closure, &(yyvsp[-1].input_section_spec), 1); }
#line 2538 "yyscript.c" /* yacc.c:1648  */
    break;

  case 105:
#line 586 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.input_section_spec).file.name = (yyvsp[0].string);
	      (yyval.input_section_spec).file.sort = SORT_WILDCARD_NONE;
	      (yyval.input_section_spec).input_sections.sections = NULL;
	      (yyval.input_section_spec).input_sections.exclude = NULL;
	    }
#line 2549 "yyscript.c" /* yacc.c:1648  */
    break;

  case 106:
#line 593 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.input_section_spec).file = (yyvsp[-3].wildcard_section);
	      (yyval.input_section_spec).input_sections = (yyvsp[-1].wildcard_sections);
	    }
#line 2558 "yyscript.c" /* yacc.c:1648  */
    break;

  case 107:
#line 602 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_section).name = (yyvsp[0].string);
	      (yyval.wildcard_section).sort = SORT_WILDCARD_NONE;
	    }
#line 2567 "yyscript.c" /* yacc.c:1648  */
    break;

  case 108:
#line 607 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_section).name = (yyvsp[-1].string);
	      (yyval.wildcard_section).sort = SORT_WILDCARD_BY_NAME;
	    }
#line 2576 "yyscript.c" /* yacc.c:1648  */
    break;

  case 109:
#line 616 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_sections).sections = script_string_sort_list_add((yyvsp[-2].wildcard_sections).sections, &(yyvsp[0].wildcard_section));
	      (yyval.wildcard_sections).exclude = (yyvsp[-2].wildcard_sections).exclude;
	    }
#line 2585 "yyscript.c" /* yacc.c:1648  */
    break;

  case 110:
#line 621 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_sections).sections = script_new_string_sort_list(&(yyvsp[0].wildcard_section));
	      (yyval.wildcard_sections).exclude = NULL;
	    }
#line 2594 "yyscript.c" /* yacc.c:1648  */
    break;

  case 111:
#line 626 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_sections).sections = (yyvsp[-5].wildcard_sections).sections;
	      (yyval.wildcard_sections).exclude = script_string_list_append((yyvsp[-5].wildcard_sections).exclude, (yyvsp[-1].string_list));
	    }
#line 2603 "yyscript.c" /* yacc.c:1648  */
    break;

  case 112:
#line 631 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_sections).sections = NULL;
	      (yyval.wildcard_sections).exclude = (yyvsp[-1].string_list);
	    }
#line 2612 "yyscript.c" /* yacc.c:1648  */
    break;

  case 113:
#line 640 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_section).name = (yyvsp[0].string);
	      (yyval.wildcard_section).sort = SORT_WILDCARD_NONE;
	    }
#line 2621 "yyscript.c" /* yacc.c:1648  */
    break;

  case 114:
#line 645 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_section).name = (yyvsp[-1].wildcard_section).name;
	      switch ((yyvsp[-1].wildcard_section).sort)
		{
		case SORT_WILDCARD_NONE:
		  (yyval.wildcard_section).sort = SORT_WILDCARD_BY_NAME;
		  break;
		case SORT_WILDCARD_BY_NAME:
		case SORT_WILDCARD_BY_NAME_BY_ALIGNMENT:
		  break;
		case SORT_WILDCARD_BY_ALIGNMENT:
		case SORT_WILDCARD_BY_ALIGNMENT_BY_NAME:
		  (yyval.wildcard_section).sort = SORT_WILDCARD_BY_NAME_BY_ALIGNMENT;
		  break;
		default:
		  abort();
		}
	    }
#line 2644 "yyscript.c" /* yacc.c:1648  */
    break;

  case 115:
#line 664 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_section).name = (yyvsp[-1].wildcard_section).name;
	      switch ((yyvsp[-1].wildcard_section).sort)
		{
		case SORT_WILDCARD_NONE:
		  (yyval.wildcard_section).sort = SORT_WILDCARD_BY_ALIGNMENT;
		  break;
		case SORT_WILDCARD_BY_ALIGNMENT:
		case SORT_WILDCARD_BY_ALIGNMENT_BY_NAME:
		  break;
		case SORT_WILDCARD_BY_NAME:
		case SORT_WILDCARD_BY_NAME_BY_ALIGNMENT:
		  (yyval.wildcard_section).sort = SORT_WILDCARD_BY_ALIGNMENT_BY_NAME;
		  break;
		default:
		  abort();
		}
	    }
#line 2667 "yyscript.c" /* yacc.c:1648  */
    break;

  case 116:
#line 683 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.wildcard_section).name = (yyvsp[-1].string);
	      (yyval.wildcard_section).sort = SORT_WILDCARD_BY_INIT_PRIORITY;
	    }
#line 2676 "yyscript.c" /* yacc.c:1648  */
    break;

  case 117:
#line 692 "yyscript.y" /* yacc.c:1648  */
    { (yyval.string_list) = script_string_list_push_back((yyvsp[-2].string_list), (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2682 "yyscript.c" /* yacc.c:1648  */
    break;

  case 118:
#line 694 "yyscript.y" /* yacc.c:1648  */
    { (yyval.string_list) = script_new_string_list((yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2688 "yyscript.c" /* yacc.c:1648  */
    break;

  case 119:
#line 701 "yyscript.y" /* yacc.c:1648  */
    { (yyval.string) = (yyvsp[0].string); }
#line 2694 "yyscript.c" /* yacc.c:1648  */
    break;

  case 120:
#line 703 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.string).value = "*";
	      (yyval.string).length = 1;
	    }
#line 2703 "yyscript.c" /* yacc.c:1648  */
    break;

  case 121:
#line 708 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.string).value = "?";
	      (yyval.string).length = 1;
	    }
#line 2712 "yyscript.c" /* yacc.c:1648  */
    break;

  case 124:
#line 723 "yyscript.y" /* yacc.c:1648  */
    { script_add_memory(closure, (yyvsp[-9].string).value, (yyvsp[-9].string).length, (yyvsp[-8].integer), (yyvsp[-4].expr), (yyvsp[0].expr)); }
#line 2718 "yyscript.c" /* yacc.c:1648  */
    break;

  case 125:
#line 726 "yyscript.y" /* yacc.c:1648  */
    { script_include_directive(PARSING_MEMORY_DEF, closure,
				     (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2725 "yyscript.c" /* yacc.c:1648  */
    break;

  case 127:
#line 734 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = script_parse_memory_attr(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length, 0); }
#line 2731 "yyscript.c" /* yacc.c:1648  */
    break;

  case 128:
#line 737 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = script_parse_memory_attr(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length, 1); }
#line 2737 "yyscript.c" /* yacc.c:1648  */
    break;

  case 129:
#line 739 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = 0; }
#line 2743 "yyscript.c" /* yacc.c:1648  */
    break;

  case 138:
#line 767 "yyscript.y" /* yacc.c:1648  */
    { script_add_phdr(closure, (yyvsp[-3].string).value, (yyvsp[-3].string).length, (yyvsp[-2].integer), &(yyvsp[-1].phdr_info)); }
#line 2749 "yyscript.c" /* yacc.c:1648  */
    break;

  case 139:
#line 776 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = script_phdr_string_to_type(closure, (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 2755 "yyscript.c" /* yacc.c:1648  */
    break;

  case 140:
#line 778 "yyscript.y" /* yacc.c:1648  */
    { (yyval.integer) = (yyvsp[0].integer); }
#line 2761 "yyscript.c" /* yacc.c:1648  */
    break;

  case 141:
#line 784 "yyscript.y" /* yacc.c:1648  */
    { memset(&(yyval.phdr_info), 0, sizeof(struct Phdr_info)); }
#line 2767 "yyscript.c" /* yacc.c:1648  */
    break;

  case 142:
#line 786 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.phdr_info) = (yyvsp[0].phdr_info);
	      if ((yyvsp[-1].string).length == 7 && strncmp((yyvsp[-1].string).value, "FILEHDR", 7) == 0)
		(yyval.phdr_info).includes_filehdr = 1;
	      else
		yyerror(closure, "PHDRS syntax error");
	    }
#line 2779 "yyscript.c" /* yacc.c:1648  */
    break;

  case 143:
#line 794 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.phdr_info) = (yyvsp[0].phdr_info);
	      (yyval.phdr_info).includes_phdrs = 1;
	    }
#line 2788 "yyscript.c" /* yacc.c:1648  */
    break;

  case 144:
#line 799 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.phdr_info) = (yyvsp[0].phdr_info);
	      if ((yyvsp[-4].string).length == 5 && strncmp((yyvsp[-4].string).value, "FLAGS", 5) == 0)
		{
		  (yyval.phdr_info).is_flags_valid = 1;
		  (yyval.phdr_info).flags = (yyvsp[-2].integer);
		}
	      else
		yyerror(closure, "PHDRS syntax error");
	    }
#line 2803 "yyscript.c" /* yacc.c:1648  */
    break;

  case 145:
#line 810 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.phdr_info) = (yyvsp[0].phdr_info);
	      (yyval.phdr_info).load_address = (yyvsp[-2].expr);
	    }
#line 2812 "yyscript.c" /* yacc.c:1648  */
    break;

  case 146:
#line 819 "yyscript.y" /* yacc.c:1648  */
    { script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, (yyvsp[0].expr), 0, 0); }
#line 2818 "yyscript.c" /* yacc.c:1648  */
    break;

  case 147:
#line 821 "yyscript.y" /* yacc.c:1648  */
    {
	      Expression_ptr s = script_exp_string((yyvsp[-2].string).value, (yyvsp[-2].string).length);
	      Expression_ptr e = script_exp_binary_add(s, (yyvsp[0].expr));
	      script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, e, 0, 0);
	    }
#line 2828 "yyscript.c" /* yacc.c:1648  */
    break;

  case 148:
#line 827 "yyscript.y" /* yacc.c:1648  */
    {
	      Expression_ptr s = script_exp_string((yyvsp[-2].string).value, (yyvsp[-2].string).length);
	      Expression_ptr e = script_exp_binary_sub(s, (yyvsp[0].expr));
	      script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, e, 0, 0);
	    }
#line 2838 "yyscript.c" /* yacc.c:1648  */
    break;

  case 149:
#line 833 "yyscript.y" /* yacc.c:1648  */
    {
	      Expression_ptr s = script_exp_string((yyvsp[-2].string).value, (yyvsp[-2].string).length);
	      Expression_ptr e = script_exp_binary_mult(s, (yyvsp[0].expr));
	      script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, e, 0, 0);
	    }
#line 2848 "yyscript.c" /* yacc.c:1648  */
    break;

  case 150:
#line 839 "yyscript.y" /* yacc.c:1648  */
    {
	      Expression_ptr s = script_exp_string((yyvsp[-2].string).value, (yyvsp[-2].string).length);
	      Expression_ptr e = script_exp_binary_div(s, (yyvsp[0].expr));
	      script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, e, 0, 0);
	    }
#line 2858 "yyscript.c" /* yacc.c:1648  */
    break;

  case 151:
#line 845 "yyscript.y" /* yacc.c:1648  */
    {
	      Expression_ptr s = script_exp_string((yyvsp[-2].string).value, (yyvsp[-2].string).length);
	      Expression_ptr e = script_exp_binary_lshift(s, (yyvsp[0].expr));
	      script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, e, 0, 0);
	    }
#line 2868 "yyscript.c" /* yacc.c:1648  */
    break;

  case 152:
#line 851 "yyscript.y" /* yacc.c:1648  */
    {
	      Expression_ptr s = script_exp_string((yyvsp[-2].string).value, (yyvsp[-2].string).length);
	      Expression_ptr e = script_exp_binary_rshift(s, (yyvsp[0].expr));
	      script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, e, 0, 0);
	    }
#line 2878 "yyscript.c" /* yacc.c:1648  */
    break;

  case 153:
#line 857 "yyscript.y" /* yacc.c:1648  */
    {
	      Expression_ptr s = script_exp_string((yyvsp[-2].string).value, (yyvsp[-2].string).length);
	      Expression_ptr e = script_exp_binary_bitwise_and(s, (yyvsp[0].expr));
	      script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, e, 0, 0);
	    }
#line 2888 "yyscript.c" /* yacc.c:1648  */
    break;

  case 154:
#line 863 "yyscript.y" /* yacc.c:1648  */
    {
	      Expression_ptr s = script_exp_string((yyvsp[-2].string).value, (yyvsp[-2].string).length);
	      Expression_ptr e = script_exp_binary_bitwise_or(s, (yyvsp[0].expr));
	      script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, e, 0, 0);
	    }
#line 2898 "yyscript.c" /* yacc.c:1648  */
    break;

  case 155:
#line 869 "yyscript.y" /* yacc.c:1648  */
    { script_set_symbol(closure, (yyvsp[-3].string).value, (yyvsp[-3].string).length, (yyvsp[-1].expr), 0, 1); }
#line 2904 "yyscript.c" /* yacc.c:1648  */
    break;

  case 156:
#line 871 "yyscript.y" /* yacc.c:1648  */
    { script_set_symbol(closure, (yyvsp[-3].string).value, (yyvsp[-3].string).length, (yyvsp[-1].expr), 1, 0); }
#line 2910 "yyscript.c" /* yacc.c:1648  */
    break;

  case 157:
#line 873 "yyscript.y" /* yacc.c:1648  */
    { script_set_symbol(closure, (yyvsp[-3].string).value, (yyvsp[-3].string).length, (yyvsp[-1].expr), 1, 1); }
#line 2916 "yyscript.c" /* yacc.c:1648  */
    break;

  case 158:
#line 878 "yyscript.y" /* yacc.c:1648  */
    { script_push_lex_into_expression_mode(closure); }
#line 2922 "yyscript.c" /* yacc.c:1648  */
    break;

  case 159:
#line 880 "yyscript.y" /* yacc.c:1648  */
    {
	      script_pop_lex_mode(closure);
	      (yyval.expr) = (yyvsp[0].expr);
	    }
#line 2931 "yyscript.c" /* yacc.c:1648  */
    break;

  case 160:
#line 889 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2937 "yyscript.c" /* yacc.c:1648  */
    break;

  case 161:
#line 891 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_unary_minus((yyvsp[0].expr)); }
#line 2943 "yyscript.c" /* yacc.c:1648  */
    break;

  case 162:
#line 893 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_unary_logical_not((yyvsp[0].expr)); }
#line 2949 "yyscript.c" /* yacc.c:1648  */
    break;

  case 163:
#line 895 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_unary_bitwise_not((yyvsp[0].expr)); }
#line 2955 "yyscript.c" /* yacc.c:1648  */
    break;

  case 164:
#line 897 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2961 "yyscript.c" /* yacc.c:1648  */
    break;

  case 165:
#line 899 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_mult((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2967 "yyscript.c" /* yacc.c:1648  */
    break;

  case 166:
#line 901 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_div((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2973 "yyscript.c" /* yacc.c:1648  */
    break;

  case 167:
#line 903 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_mod((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2979 "yyscript.c" /* yacc.c:1648  */
    break;

  case 168:
#line 905 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_add((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2985 "yyscript.c" /* yacc.c:1648  */
    break;

  case 169:
#line 907 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_sub((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2991 "yyscript.c" /* yacc.c:1648  */
    break;

  case 170:
#line 909 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_lshift((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2997 "yyscript.c" /* yacc.c:1648  */
    break;

  case 171:
#line 911 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_rshift((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3003 "yyscript.c" /* yacc.c:1648  */
    break;

  case 172:
#line 913 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_eq((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3009 "yyscript.c" /* yacc.c:1648  */
    break;

  case 173:
#line 915 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_ne((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3015 "yyscript.c" /* yacc.c:1648  */
    break;

  case 174:
#line 917 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_le((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3021 "yyscript.c" /* yacc.c:1648  */
    break;

  case 175:
#line 919 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_ge((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3027 "yyscript.c" /* yacc.c:1648  */
    break;

  case 176:
#line 921 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_lt((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3033 "yyscript.c" /* yacc.c:1648  */
    break;

  case 177:
#line 923 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_gt((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3039 "yyscript.c" /* yacc.c:1648  */
    break;

  case 178:
#line 925 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_bitwise_and((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3045 "yyscript.c" /* yacc.c:1648  */
    break;

  case 179:
#line 927 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_bitwise_xor((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3051 "yyscript.c" /* yacc.c:1648  */
    break;

  case 180:
#line 929 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_bitwise_or((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3057 "yyscript.c" /* yacc.c:1648  */
    break;

  case 181:
#line 931 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_logical_and((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3063 "yyscript.c" /* yacc.c:1648  */
    break;

  case 182:
#line 933 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_binary_logical_or((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3069 "yyscript.c" /* yacc.c:1648  */
    break;

  case 183:
#line 935 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_trinary_cond((yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3075 "yyscript.c" /* yacc.c:1648  */
    break;

  case 184:
#line 937 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_integer((yyvsp[0].integer)); }
#line 3081 "yyscript.c" /* yacc.c:1648  */
    break;

  case 185:
#line 939 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_symbol(closure, (yyvsp[0].string).value, (yyvsp[0].string).length); }
#line 3087 "yyscript.c" /* yacc.c:1648  */
    break;

  case 186:
#line 941 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_max((yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 3093 "yyscript.c" /* yacc.c:1648  */
    break;

  case 187:
#line 943 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_min((yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 3099 "yyscript.c" /* yacc.c:1648  */
    break;

  case 188:
#line 945 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_defined((yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3105 "yyscript.c" /* yacc.c:1648  */
    break;

  case 189:
#line 947 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_sizeof_headers(); }
#line 3111 "yyscript.c" /* yacc.c:1648  */
    break;

  case 190:
#line 949 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_alignof((yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3117 "yyscript.c" /* yacc.c:1648  */
    break;

  case 191:
#line 951 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_sizeof((yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3123 "yyscript.c" /* yacc.c:1648  */
    break;

  case 192:
#line 953 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_addr((yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3129 "yyscript.c" /* yacc.c:1648  */
    break;

  case 193:
#line 955 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_loadaddr((yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3135 "yyscript.c" /* yacc.c:1648  */
    break;

  case 194:
#line 957 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_origin(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3141 "yyscript.c" /* yacc.c:1648  */
    break;

  case 195:
#line 959 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_length(closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3147 "yyscript.c" /* yacc.c:1648  */
    break;

  case 196:
#line 961 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_constant((yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3153 "yyscript.c" /* yacc.c:1648  */
    break;

  case 197:
#line 963 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_absolute((yyvsp[-1].expr)); }
#line 3159 "yyscript.c" /* yacc.c:1648  */
    break;

  case 198:
#line 965 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_align(script_exp_string(".", 1), (yyvsp[-1].expr)); }
#line 3165 "yyscript.c" /* yacc.c:1648  */
    break;

  case 199:
#line 967 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_align((yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 3171 "yyscript.c" /* yacc.c:1648  */
    break;

  case 200:
#line 969 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_align(script_exp_string(".", 1), (yyvsp[-1].expr)); }
#line 3177 "yyscript.c" /* yacc.c:1648  */
    break;

  case 201:
#line 971 "yyscript.y" /* yacc.c:1648  */
    {
	      script_data_segment_align(closure);
	      (yyval.expr) = script_exp_function_data_segment_align((yyvsp[-3].expr), (yyvsp[-1].expr));
	    }
#line 3186 "yyscript.c" /* yacc.c:1648  */
    break;

  case 202:
#line 976 "yyscript.y" /* yacc.c:1648  */
    {
	      script_data_segment_relro_end(closure);
	      (yyval.expr) = script_exp_function_data_segment_relro_end((yyvsp[-3].expr), (yyvsp[-1].expr));
	    }
#line 3195 "yyscript.c" /* yacc.c:1648  */
    break;

  case 203:
#line 981 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_data_segment_end((yyvsp[-1].expr)); }
#line 3201 "yyscript.c" /* yacc.c:1648  */
    break;

  case 204:
#line 983 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.expr) = script_exp_function_segment_start((yyvsp[-3].string).value, (yyvsp[-3].string).length, (yyvsp[-1].expr));
	      /* We need to take note of any SEGMENT_START expressions
		 because they change the behaviour of -Ttext, -Tdata and
		 -Tbss options.  */
	      script_saw_segment_start_expression(closure);
	    }
#line 3213 "yyscript.c" /* yacc.c:1648  */
    break;

  case 205:
#line 991 "yyscript.y" /* yacc.c:1648  */
    { (yyval.expr) = script_exp_function_assert((yyvsp[-3].expr), (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3219 "yyscript.c" /* yacc.c:1648  */
    break;

  case 206:
#line 997 "yyscript.y" /* yacc.c:1648  */
    { script_set_symbol(closure, (yyvsp[-2].string).value, (yyvsp[-2].string).length, (yyvsp[0].expr), 0, 0); }
#line 3225 "yyscript.c" /* yacc.c:1648  */
    break;

  case 210:
#line 1015 "yyscript.y" /* yacc.c:1648  */
    { script_new_vers_node (closure, NULL, (yyvsp[-3].versyms)); }
#line 3231 "yyscript.c" /* yacc.c:1648  */
    break;

  case 214:
#line 1030 "yyscript.y" /* yacc.c:1648  */
    {
	      script_register_vers_node (closure, NULL, 0, (yyvsp[-2].versnode), NULL);
	    }
#line 3239 "yyscript.c" /* yacc.c:1648  */
    break;

  case 215:
#line 1034 "yyscript.y" /* yacc.c:1648  */
    {
	      script_register_vers_node (closure, (yyvsp[-4].string).value, (yyvsp[-4].string).length, (yyvsp[-2].versnode),
					 NULL);
	    }
#line 3248 "yyscript.c" /* yacc.c:1648  */
    break;

  case 216:
#line 1039 "yyscript.y" /* yacc.c:1648  */
    {
	      script_register_vers_node (closure, (yyvsp[-5].string).value, (yyvsp[-5].string).length, (yyvsp[-3].versnode), (yyvsp[-1].deplist));
	    }
#line 3256 "yyscript.c" /* yacc.c:1648  */
    break;

  case 217:
#line 1046 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.deplist) = script_add_vers_depend (closure, NULL, (yyvsp[0].string).value, (yyvsp[0].string).length);
	    }
#line 3264 "yyscript.c" /* yacc.c:1648  */
    break;

  case 218:
#line 1050 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.deplist) = script_add_vers_depend (closure, (yyvsp[-1].deplist), (yyvsp[0].string).value, (yyvsp[0].string).length);
	    }
#line 3272 "yyscript.c" /* yacc.c:1648  */
    break;

  case 219:
#line 1057 "yyscript.y" /* yacc.c:1648  */
    { (yyval.versnode) = script_new_vers_node (closure, NULL, NULL); }
#line 3278 "yyscript.c" /* yacc.c:1648  */
    break;

  case 220:
#line 1059 "yyscript.y" /* yacc.c:1648  */
    { (yyval.versnode) = script_new_vers_node (closure, (yyvsp[-1].versyms), NULL); }
#line 3284 "yyscript.c" /* yacc.c:1648  */
    break;

  case 221:
#line 1061 "yyscript.y" /* yacc.c:1648  */
    { (yyval.versnode) = script_new_vers_node (closure, (yyvsp[-1].versyms), NULL); }
#line 3290 "yyscript.c" /* yacc.c:1648  */
    break;

  case 222:
#line 1063 "yyscript.y" /* yacc.c:1648  */
    { (yyval.versnode) = script_new_vers_node (closure, NULL, (yyvsp[-1].versyms)); }
#line 3296 "yyscript.c" /* yacc.c:1648  */
    break;

  case 223:
#line 1065 "yyscript.y" /* yacc.c:1648  */
    { (yyval.versnode) = script_new_vers_node (closure, (yyvsp[-5].versyms), (yyvsp[-1].versyms)); }
#line 3302 "yyscript.c" /* yacc.c:1648  */
    break;

  case 224:
#line 1074 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.versyms) = script_new_vers_pattern (closure, NULL, (yyvsp[0].string).value,
					    (yyvsp[0].string).length, 0);
	    }
#line 3311 "yyscript.c" /* yacc.c:1648  */
    break;

  case 225:
#line 1079 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.versyms) = script_new_vers_pattern (closure, NULL, (yyvsp[0].string).value,
					    (yyvsp[0].string).length, 1);
	    }
#line 3320 "yyscript.c" /* yacc.c:1648  */
    break;

  case 226:
#line 1084 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.versyms) = script_new_vers_pattern (closure, (yyvsp[-2].versyms), (yyvsp[0].string).value,
                                            (yyvsp[0].string).length, 0);
	    }
#line 3329 "yyscript.c" /* yacc.c:1648  */
    break;

  case 227:
#line 1089 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.versyms) = script_new_vers_pattern (closure, (yyvsp[-2].versyms), (yyvsp[0].string).value,
                                            (yyvsp[0].string).length, 1);
	    }
#line 3338 "yyscript.c" /* yacc.c:1648  */
    break;

  case 228:
#line 1095 "yyscript.y" /* yacc.c:1648  */
    { version_script_push_lang (closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3344 "yyscript.c" /* yacc.c:1648  */
    break;

  case 229:
#line 1097 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.versyms) = (yyvsp[-2].versyms);
	      version_script_pop_lang(closure);
	    }
#line 3353 "yyscript.c" /* yacc.c:1648  */
    break;

  case 230:
#line 1105 "yyscript.y" /* yacc.c:1648  */
    { version_script_push_lang (closure, (yyvsp[-1].string).value, (yyvsp[-1].string).length); }
#line 3359 "yyscript.c" /* yacc.c:1648  */
    break;

  case 231:
#line 1107 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.versyms) = script_merge_expressions ((yyvsp[-8].versyms), (yyvsp[-2].versyms));
	      version_script_pop_lang(closure);
	    }
#line 3368 "yyscript.c" /* yacc.c:1648  */
    break;

  case 232:
#line 1112 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.versyms) = script_new_vers_pattern (closure, NULL, "extern",
					    sizeof("extern") - 1, 1);
	    }
#line 3377 "yyscript.c" /* yacc.c:1648  */
    break;

  case 233:
#line 1117 "yyscript.y" /* yacc.c:1648  */
    {
	      (yyval.versyms) = script_new_vers_pattern (closure, (yyvsp[-2].versyms), "extern",
					    sizeof("extern") - 1, 1);
	    }
#line 3386 "yyscript.c" /* yacc.c:1648  */
    break;

  case 234:
#line 1127 "yyscript.y" /* yacc.c:1648  */
    { (yyval.string) = (yyvsp[0].string); }
#line 3392 "yyscript.c" /* yacc.c:1648  */
    break;

  case 235:
#line 1129 "yyscript.y" /* yacc.c:1648  */
    { (yyval.string) = (yyvsp[0].string); }
#line 3398 "yyscript.c" /* yacc.c:1648  */
    break;


#line 3402 "yyscript.c" /* yacc.c:1648  */
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
      yyerror (closure, YY_("syntax error"));
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
        yyerror (closure, yymsgp);
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
                      yytoken, &yylval, closure);
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
                  yystos[yystate], yyvsp, closure);
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
  yyerror (closure, YY_("memory exhausted"));
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
                  yytoken, &yylval, closure);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, closure);
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
#line 1151 "yyscript.y" /* yacc.c:1907  */

