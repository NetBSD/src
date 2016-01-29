/* A Bison parser, made by GNU Bison 3.0.  */

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
#define YYBISON_VERSION "3.0"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 22 "ldgram.y" /* yacc.c:339  */

/*

 */

#define DONTDECLARE_MALLOC

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"
#include "ld.h"
#include "ldexp.h"
#include "ldver.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldemul.h"
#include "ldmisc.h"
#include "ldmain.h"
#include "mri.h"
#include "ldctor.h"
#include "ldlex.h"

#ifndef YYDEBUG
#define YYDEBUG 1
#endif

static enum section_type sectype;
static lang_memory_region_type *region;

bfd_boolean ldgram_had_keep = FALSE;
char *ldgram_vers_current_lang = NULL;

#define ERROR_NAME_MAX 20
static char *error_names[ERROR_NAME_MAX];
static int error_index;
#define PUSH_ERROR(x) if (error_index < ERROR_NAME_MAX) error_names[error_index] = x; error_index++;
#define POP_ERROR()   error_index--;

#line 105 "ldgram.c" /* yacc.c:339  */

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
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
#ifndef YY_YY_LDGRAM_H_INCLUDED
# define YY_YY_LDGRAM_H_INCLUDED
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
    NAME = 259,
    LNAME = 260,
    PLUSEQ = 261,
    MINUSEQ = 262,
    MULTEQ = 263,
    DIVEQ = 264,
    LSHIFTEQ = 265,
    RSHIFTEQ = 266,
    ANDEQ = 267,
    OREQ = 268,
    OROR = 269,
    ANDAND = 270,
    EQ = 271,
    NE = 272,
    LE = 273,
    GE = 274,
    LSHIFT = 275,
    RSHIFT = 276,
    UNARY = 277,
    END = 278,
    ALIGN_K = 279,
    BLOCK = 280,
    BIND = 281,
    QUAD = 282,
    SQUAD = 283,
    LONG = 284,
    SHORT = 285,
    BYTE = 286,
    SECTIONS = 287,
    PHDRS = 288,
    INSERT_K = 289,
    AFTER = 290,
    BEFORE = 291,
    DATA_SEGMENT_ALIGN = 292,
    DATA_SEGMENT_RELRO_END = 293,
    DATA_SEGMENT_END = 294,
    SORT_BY_NAME = 295,
    SORT_BY_ALIGNMENT = 296,
    SORT_NONE = 297,
    SORT_BY_INIT_PRIORITY = 298,
    SIZEOF_HEADERS = 299,
    OUTPUT_FORMAT = 300,
    FORCE_COMMON_ALLOCATION = 301,
    OUTPUT_ARCH = 302,
    INHIBIT_COMMON_ALLOCATION = 303,
    SEGMENT_START = 304,
    INCLUDE = 305,
    MEMORY = 306,
    REGION_ALIAS = 307,
    LD_FEATURE = 308,
    NOLOAD = 309,
    DSECT = 310,
    COPY = 311,
    INFO = 312,
    OVERLAY = 313,
    DEFINED = 314,
    TARGET_K = 315,
    SEARCH_DIR = 316,
    MAP = 317,
    ENTRY = 318,
    NEXT = 319,
    SIZEOF = 320,
    ALIGNOF = 321,
    ADDR = 322,
    LOADADDR = 323,
    MAX_K = 324,
    MIN_K = 325,
    STARTUP = 326,
    HLL = 327,
    SYSLIB = 328,
    FLOAT = 329,
    NOFLOAT = 330,
    NOCROSSREFS = 331,
    ORIGIN = 332,
    FILL = 333,
    LENGTH = 334,
    CREATE_OBJECT_SYMBOLS = 335,
    INPUT = 336,
    GROUP = 337,
    OUTPUT = 338,
    CONSTRUCTORS = 339,
    ALIGNMOD = 340,
    AT = 341,
    SUBALIGN = 342,
    HIDDEN = 343,
    PROVIDE = 344,
    PROVIDE_HIDDEN = 345,
    AS_NEEDED = 346,
    CHIP = 347,
    LIST = 348,
    SECT = 349,
    ABSOLUTE = 350,
    LOAD = 351,
    NEWLINE = 352,
    ENDWORD = 353,
    ORDER = 354,
    NAMEWORD = 355,
    ASSERT_K = 356,
    LOG2CEIL = 357,
    FORMAT = 358,
    PUBLIC = 359,
    DEFSYMEND = 360,
    BASE = 361,
    ALIAS = 362,
    TRUNCATE = 363,
    REL = 364,
    INPUT_SCRIPT = 365,
    INPUT_MRI_SCRIPT = 366,
    INPUT_DEFSYM = 367,
    CASE = 368,
    EXTERN = 369,
    START = 370,
    VERS_TAG = 371,
    VERS_IDENTIFIER = 372,
    GLOBAL = 373,
    LOCAL = 374,
    VERSIONK = 375,
    INPUT_VERSION_SCRIPT = 376,
    KEEP = 377,
    ONLY_IF_RO = 378,
    ONLY_IF_RW = 379,
    SPECIAL = 380,
    INPUT_SECTION_FLAGS = 381,
    ALIGN_WITH_INPUT = 382,
    EXCLUDE_FILE = 383,
    CONSTANT = 384,
    INPUT_DYNAMIC_LIST = 385
  };
#endif
/* Tokens.  */
#define INT 258
#define NAME 259
#define LNAME 260
#define PLUSEQ 261
#define MINUSEQ 262
#define MULTEQ 263
#define DIVEQ 264
#define LSHIFTEQ 265
#define RSHIFTEQ 266
#define ANDEQ 267
#define OREQ 268
#define OROR 269
#define ANDAND 270
#define EQ 271
#define NE 272
#define LE 273
#define GE 274
#define LSHIFT 275
#define RSHIFT 276
#define UNARY 277
#define END 278
#define ALIGN_K 279
#define BLOCK 280
#define BIND 281
#define QUAD 282
#define SQUAD 283
#define LONG 284
#define SHORT 285
#define BYTE 286
#define SECTIONS 287
#define PHDRS 288
#define INSERT_K 289
#define AFTER 290
#define BEFORE 291
#define DATA_SEGMENT_ALIGN 292
#define DATA_SEGMENT_RELRO_END 293
#define DATA_SEGMENT_END 294
#define SORT_BY_NAME 295
#define SORT_BY_ALIGNMENT 296
#define SORT_NONE 297
#define SORT_BY_INIT_PRIORITY 298
#define SIZEOF_HEADERS 299
#define OUTPUT_FORMAT 300
#define FORCE_COMMON_ALLOCATION 301
#define OUTPUT_ARCH 302
#define INHIBIT_COMMON_ALLOCATION 303
#define SEGMENT_START 304
#define INCLUDE 305
#define MEMORY 306
#define REGION_ALIAS 307
#define LD_FEATURE 308
#define NOLOAD 309
#define DSECT 310
#define COPY 311
#define INFO 312
#define OVERLAY 313
#define DEFINED 314
#define TARGET_K 315
#define SEARCH_DIR 316
#define MAP 317
#define ENTRY 318
#define NEXT 319
#define SIZEOF 320
#define ALIGNOF 321
#define ADDR 322
#define LOADADDR 323
#define MAX_K 324
#define MIN_K 325
#define STARTUP 326
#define HLL 327
#define SYSLIB 328
#define FLOAT 329
#define NOFLOAT 330
#define NOCROSSREFS 331
#define ORIGIN 332
#define FILL 333
#define LENGTH 334
#define CREATE_OBJECT_SYMBOLS 335
#define INPUT 336
#define GROUP 337
#define OUTPUT 338
#define CONSTRUCTORS 339
#define ALIGNMOD 340
#define AT 341
#define SUBALIGN 342
#define HIDDEN 343
#define PROVIDE 344
#define PROVIDE_HIDDEN 345
#define AS_NEEDED 346
#define CHIP 347
#define LIST 348
#define SECT 349
#define ABSOLUTE 350
#define LOAD 351
#define NEWLINE 352
#define ENDWORD 353
#define ORDER 354
#define NAMEWORD 355
#define ASSERT_K 356
#define LOG2CEIL 357
#define FORMAT 358
#define PUBLIC 359
#define DEFSYMEND 360
#define BASE 361
#define ALIAS 362
#define TRUNCATE 363
#define REL 364
#define INPUT_SCRIPT 365
#define INPUT_MRI_SCRIPT 366
#define INPUT_DEFSYM 367
#define CASE 368
#define EXTERN 369
#define START 370
#define VERS_TAG 371
#define VERS_IDENTIFIER 372
#define GLOBAL 373
#define LOCAL 374
#define VERSIONK 375
#define INPUT_VERSION_SCRIPT 376
#define KEEP 377
#define ONLY_IF_RO 378
#define ONLY_IF_RW 379
#define SPECIAL 380
#define INPUT_SECTION_FLAGS 381
#define ALIGN_WITH_INPUT 382
#define EXCLUDE_FILE 383
#define CONSTANT 384
#define INPUT_DYNAMIC_LIST 385

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 60 "ldgram.y" /* yacc.c:355  */

  bfd_vma integer;
  struct big_int
    {
      bfd_vma integer;
      char *str;
    } bigint;
  fill_type *fill;
  char *name;
  const char *cname;
  struct wildcard_spec wildcard;
  struct wildcard_list *wildcard_list;
  struct name_list *name_list;
  struct flag_info_list *flag_info_list;
  struct flag_info *flag_info;
  int token;
  union etree_union *etree;
  struct phdr_info
    {
      bfd_boolean filehdr;
      bfd_boolean phdrs;
      union etree_union *at;
      union etree_union *flags;
    } phdr;
  struct lang_nocrossref *nocrossref;
  struct lang_output_section_phdr_list *section_phdr;
  struct bfd_elf_version_deps *deflist;
  struct bfd_elf_version_expr *versyms;
  struct bfd_elf_version_tree *versnode;

#line 436 "ldgram.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_LDGRAM_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 451 "ldgram.c" /* yacc.c:358  */

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

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
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
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1915

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  154
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  131
/* YYNRULES -- Number of rules.  */
#define YYNRULES  371
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  805

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   385

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   152,     2,     2,     2,    34,    21,     2,
      37,   149,    32,    30,   147,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   148,
      24,    10,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   150,     2,   151,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,    19,    59,   153,     2,     2,     2,
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
       5,     6,     7,     8,     9,    11,    12,    13,    14,    17,
      18,    22,    23,    26,    27,    28,    29,    35,    36,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   166,   166,   167,   168,   169,   170,   174,   178,   178,
     188,   188,   201,   202,   206,   207,   208,   211,   214,   215,
     216,   218,   220,   222,   224,   226,   228,   230,   232,   234,
     236,   238,   239,   240,   242,   244,   246,   248,   250,   251,
     253,   252,   256,   258,   262,   263,   264,   268,   270,   274,
     276,   281,   282,   283,   288,   288,   293,   295,   297,   302,
     302,   308,   309,   314,   315,   316,   317,   318,   319,   320,
     321,   322,   323,   324,   326,   328,   330,   333,   335,   337,
     339,   341,   343,   342,   346,   349,   348,   352,   356,   357,
     359,   361,   363,   368,   368,   373,   376,   379,   382,   385,
     388,   392,   391,   397,   396,   402,   401,   409,   413,   414,
     415,   419,   421,   422,   422,   430,   434,   438,   445,   452,
     459,   466,   473,   480,   487,   494,   501,   508,   515,   524,
     542,   563,   576,   585,   596,   605,   616,   625,   634,   638,
     647,   651,   659,   661,   660,   667,   668,   672,   673,   678,
     683,   684,   689,   693,   693,   697,   696,   703,   704,   707,
     709,   713,   715,   717,   719,   721,   726,   733,   735,   739,
     741,   743,   745,   747,   749,   751,   753,   758,   758,   763,
     767,   775,   779,   783,   791,   791,   795,   798,   798,   801,
     802,   807,   806,   812,   811,   818,   826,   834,   835,   839,
     840,   844,   846,   851,   856,   857,   862,   864,   870,   872,
     874,   878,   880,   886,   889,   898,   909,   909,   915,   917,
     919,   921,   923,   925,   928,   930,   932,   934,   936,   938,
     940,   942,   944,   946,   948,   950,   952,   954,   956,   958,
     960,   962,   964,   966,   968,   970,   973,   975,   977,   979,
     981,   983,   985,   987,   989,   991,   993,   995,  1004,  1006,
    1008,  1010,  1012,  1014,  1016,  1018,  1024,  1025,  1029,  1030,
    1034,  1035,  1039,  1040,  1044,  1045,  1049,  1050,  1051,  1052,
    1055,  1060,  1063,  1069,  1071,  1055,  1078,  1080,  1082,  1087,
    1089,  1077,  1099,  1101,  1099,  1107,  1106,  1113,  1114,  1115,
    1116,  1117,  1121,  1122,  1123,  1127,  1128,  1133,  1134,  1139,
    1140,  1145,  1146,  1151,  1153,  1158,  1161,  1174,  1178,  1183,
    1185,  1176,  1193,  1196,  1198,  1202,  1203,  1202,  1212,  1257,
    1260,  1273,  1282,  1285,  1292,  1292,  1304,  1305,  1309,  1313,
    1322,  1322,  1336,  1336,  1346,  1347,  1351,  1355,  1359,  1366,
    1370,  1378,  1381,  1385,  1389,  1393,  1400,  1404,  1408,  1412,
    1417,  1416,  1430,  1429,  1439,  1443,  1447,  1451,  1455,  1459,
    1465,  1467
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "NAME", "LNAME", "PLUSEQ",
  "MINUSEQ", "MULTEQ", "DIVEQ", "'='", "LSHIFTEQ", "RSHIFTEQ", "ANDEQ",
  "OREQ", "'?'", "':'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQ", "NE",
  "'<'", "'>'", "LE", "GE", "LSHIFT", "RSHIFT", "'+'", "'-'", "'*'", "'/'",
  "'%'", "UNARY", "END", "'('", "ALIGN_K", "BLOCK", "BIND", "QUAD",
  "SQUAD", "LONG", "SHORT", "BYTE", "SECTIONS", "PHDRS", "INSERT_K",
  "AFTER", "BEFORE", "DATA_SEGMENT_ALIGN", "DATA_SEGMENT_RELRO_END",
  "DATA_SEGMENT_END", "SORT_BY_NAME", "SORT_BY_ALIGNMENT", "SORT_NONE",
  "SORT_BY_INIT_PRIORITY", "'{'", "'}'", "SIZEOF_HEADERS", "OUTPUT_FORMAT",
  "FORCE_COMMON_ALLOCATION", "OUTPUT_ARCH", "INHIBIT_COMMON_ALLOCATION",
  "SEGMENT_START", "INCLUDE", "MEMORY", "REGION_ALIAS", "LD_FEATURE",
  "NOLOAD", "DSECT", "COPY", "INFO", "OVERLAY", "DEFINED", "TARGET_K",
  "SEARCH_DIR", "MAP", "ENTRY", "NEXT", "SIZEOF", "ALIGNOF", "ADDR",
  "LOADADDR", "MAX_K", "MIN_K", "STARTUP", "HLL", "SYSLIB", "FLOAT",
  "NOFLOAT", "NOCROSSREFS", "ORIGIN", "FILL", "LENGTH",
  "CREATE_OBJECT_SYMBOLS", "INPUT", "GROUP", "OUTPUT", "CONSTRUCTORS",
  "ALIGNMOD", "AT", "SUBALIGN", "HIDDEN", "PROVIDE", "PROVIDE_HIDDEN",
  "AS_NEEDED", "CHIP", "LIST", "SECT", "ABSOLUTE", "LOAD", "NEWLINE",
  "ENDWORD", "ORDER", "NAMEWORD", "ASSERT_K", "LOG2CEIL", "FORMAT",
  "PUBLIC", "DEFSYMEND", "BASE", "ALIAS", "TRUNCATE", "REL",
  "INPUT_SCRIPT", "INPUT_MRI_SCRIPT", "INPUT_DEFSYM", "CASE", "EXTERN",
  "START", "VERS_TAG", "VERS_IDENTIFIER", "GLOBAL", "LOCAL", "VERSIONK",
  "INPUT_VERSION_SCRIPT", "KEEP", "ONLY_IF_RO", "ONLY_IF_RW", "SPECIAL",
  "INPUT_SECTION_FLAGS", "ALIGN_WITH_INPUT", "EXCLUDE_FILE", "CONSTANT",
  "INPUT_DYNAMIC_LIST", "','", "';'", "')'", "'['", "']'", "'!'", "'~'",
  "$accept", "file", "filename", "defsym_expr", "$@1", "mri_script_file",
  "$@2", "mri_script_lines", "mri_script_command", "$@3", "ordernamelist",
  "mri_load_name_list", "mri_abs_name_list", "casesymlist",
  "extern_name_list", "$@4", "extern_name_list_body", "script_file", "$@5",
  "ifile_list", "ifile_p1", "$@6", "$@7", "input_list", "$@8",
  "input_list1", "@9", "@10", "@11", "sections", "sec_or_group_p1",
  "statement_anywhere", "$@12", "wildcard_name", "wildcard_spec",
  "sect_flag_list", "sect_flags", "exclude_name_list", "file_NAME_list",
  "input_section_spec_no_keep", "input_section_spec", "$@13", "statement",
  "$@14", "$@15", "statement_list", "statement_list_opt", "length",
  "fill_exp", "fill_opt", "assign_op", "end", "assignment", "opt_comma",
  "memory", "memory_spec_list_opt", "memory_spec_list", "memory_spec",
  "$@16", "$@17", "origin_spec", "length_spec", "attributes_opt",
  "attributes_list", "attributes_string", "startup", "high_level_library",
  "high_level_library_NAME_list", "low_level_library",
  "low_level_library_NAME_list", "floating_point_support",
  "nocrossref_list", "mustbe_exp", "$@18", "exp", "memspec_at_opt",
  "opt_at", "opt_align", "opt_align_with_input", "opt_subalign",
  "sect_constraint", "section", "$@19", "$@20", "$@21", "$@22", "$@23",
  "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "$@30", "$@31", "type",
  "atype", "opt_exp_with_type", "opt_exp_without_type", "opt_nocrossrefs",
  "memspec_opt", "phdr_opt", "overlay_section", "$@32", "$@33", "$@34",
  "phdrs", "phdr_list", "phdr", "$@35", "$@36", "phdr_type",
  "phdr_qualifiers", "phdr_val", "dynamic_list_file", "$@37",
  "dynamic_list_nodes", "dynamic_list_node", "dynamic_list_tag",
  "version_script_file", "$@38", "version", "$@39", "vers_nodes",
  "vers_node", "verdep", "vers_tag", "vers_defns", "@40", "@41",
  "opt_semicolon", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
      61,   265,   266,   267,   268,    63,    58,   269,   270,   124,
      94,    38,   271,   272,    60,    62,   273,   274,   275,   276,
      43,    45,    42,    47,    37,   277,   278,    40,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   123,   125,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,   384,   385,    44,    59,    41,
      91,    93,    33,   126
};
# endif

#define YYPACT_NINF -648

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-648)))

#define YYTABLE_NINF -343

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     310,  -648,  -648,  -648,  -648,  -648,    59,  -648,  -648,  -648,
    -648,  -648,    62,  -648,   -11,  -648,    23,  -648,   856,  1654,
      66,   219,    43,   -11,  -648,   234,    23,  -648,   556,    60,
      85,    76,   101,  -648,   116,  -648,   168,   134,   147,   188,
     194,   211,   225,   229,   243,   253,   255,  -648,  -648,   272,
     275,  -648,   283,   285,   287,   288,  -648,   290,  -648,  -648,
    -648,  -648,    -1,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
     193,  -648,   327,   168,   328,   680,  -648,   331,   333,   334,
    -648,  -648,   335,   336,   338,   680,   339,   344,   341,  -648,
     351,   249,   680,  -648,   352,  -648,   332,   347,   306,   222,
     219,  -648,  -648,  -648,   307,   244,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,   373,
     379,   389,   397,  -648,  -648,    41,   398,   399,   400,   168,
     168,   405,   168,    17,  -648,   406,  -648,   381,   168,   415,
     420,   421,   390,  -648,  -648,  -648,  -648,   372,     9,  -648,
      29,  -648,  -648,   680,   680,   680,   394,   395,   396,   411,
     422,  -648,   424,   426,   428,   429,   432,   434,   435,   436,
     438,   439,   440,   441,   442,   444,   445,   680,   680,  1463,
       5,  -648,   299,  -648,   311,    28,  -648,  -648,   369,  1835,
     337,  -648,  -648,   345,  -648,   453,  -648,  -648,  1835,   425,
     234,   234,   340,   241,   430,   346,   241,  -648,   680,  -648,
     431,    45,  -648,  -648,   -39,   350,  -648,  -648,   168,   437,
      -6,  -648,   348,   353,   354,   358,   359,   362,   366,  -648,
    -648,    93,    97,    30,   368,   370,    33,  -648,   371,   475,
     483,   490,   680,   377,   -11,   680,   680,  -648,   680,   680,
    -648,  -648,   864,   680,   680,   680,   680,   680,   497,   523,
     680,   524,   526,   527,   528,   680,   680,   529,   530,   680,
     680,   680,   534,  -648,  -648,   680,   680,   680,   680,   680,
     680,   680,   680,   680,   680,   680,   680,   680,   680,   680,
     680,   680,   680,   680,   680,   680,   680,  1835,   536,   537,
    -648,   538,   680,   680,  1835,   185,   539,  -648,    36,  -648,
     401,   402,  -648,  -648,   540,  -648,  -648,  -648,   -65,  -648,
    1835,   556,  -648,   168,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,   541,  -648,  -648,   932,   510,  -648,  -648,  -648,    41,
     549,  -648,  -648,  -648,  -648,  -648,  -648,  -648,   168,  -648,
     168,   406,  -648,  -648,  -648,  -648,  -648,   517,    22,   407,
    -648,  -648,  -648,  -648,  1483,  -648,    -4,  1835,  1835,  1678,
    1835,  1835,  -648,   820,  1064,  1503,  1523,  1084,   410,   427,
    1104,   433,   443,   446,   448,  1543,  1563,   459,   460,  1124,
    1594,  1144,   461,  1795,  1852,  1044,  1867,  1881,  1177,   755,
     755,   664,   664,   664,   664,   409,   409,   235,   235,  -648,
    -648,  -648,  1835,  1835,  1835,  -648,  -648,  -648,  1835,  1835,
    -648,  -648,  -648,  -648,   571,   234,   261,   241,   520,  -648,
    -648,   -55,   521,  -648,   596,   521,   680,   447,  -648,     3,
     561,    41,  -648,   462,  -648,  -648,  -648,  -648,  -648,  -648,
     542,    47,  -648,   464,   466,   469,   576,  -648,  -648,   680,
    -648,  -648,   680,   680,  -648,   680,  -648,  -648,  -648,  -648,
    -648,  -648,   680,   680,  -648,  -648,  -648,   585,  -648,  -648,
     680,  -648,   471,   574,  -648,  -648,  -648,   233,   554,  1706,
     604,   535,  -648,  -648,  1815,   548,  -648,  1835,    37,   617,
    -648,   618,    12,  -648,   557,   592,  -648,    33,  -648,  -648,
    -648,   593,  -648,  -648,  -648,   480,  1164,  1197,  1217,  1237,
    1257,  1277,   482,  1835,   241,   583,   234,   234,  -648,  -648,
    -648,  -648,  -648,  -648,   502,   680,   342,   637,  -648,   620,
     616,   519,  -648,  -648,   535,   597,   621,   622,  -648,   511,
    -648,  -648,  -648,   652,   516,  -648,    92,    33,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,   522,   471,
    -648,  1297,  -648,   680,   627,   525,  -648,   562,  -648,   680,
      37,   680,   553,  -648,  -648,   572,  -648,   126,    33,   241,
     610,   218,  1330,   680,  -648,   562,   635,  -648,  1595,  1350,
    -648,  1370,  -648,  -648,   665,  -648,  -648,   140,  -648,   648,
     672,  -648,  1390,  -648,   680,   632,  -648,  -648,    37,  -648,
    -648,   680,  -648,  -648,   156,  1410,  -648,  -648,  -648,  1430,
    -648,  -648,  -648,   645,  -648,  -648,   669,  -648,    58,   692,
     763,  -648,  -648,  -648,   714,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,   675,   678,   693,   697,   168,   698,  -648,  -648,
    -648,   699,   700,   701,  -648,   245,  -648,   702,    80,  -648,
    -648,  -648,   763,   650,   705,    -1,   685,   719,   326,   413,
      67,    67,  -648,  -648,   710,  -648,   746,    67,  -648,   715,
    -648,   -27,   245,   716,   245,   717,  -648,  -648,  -648,  -648,
     763,   752,   655,   721,   722,   619,   732,   623,   733,   734,
     625,   628,   641,   763,   643,  -648,   680,    18,  -648,     2,
    -648,    14,   279,  -648,   245,   112,    75,   245,   719,   644,
     735,  -648,   751,  -648,    67,    67,  -648,    67,  -648,    67,
      67,  -648,  -648,  -648,   760,  -648,  1614,   651,   653,   795,
    -648,    67,  -648,  -648,  -648,  -648,   129,   655,  -648,  -648,
     797,    99,   654,   660,    16,   661,   662,  -648,   808,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,   809,  -648,   666,
     667,    67,   673,   674,   677,    99,    99,  -648,  -648,   516,
    -648,  -648,   679,  -648,  -648,    -1,  -648,  -648,  -648,  -648,
    -648,   516,   516,  -648,  -648
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    59,    10,     8,   340,   334,     0,     2,    62,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    60,    11,
       0,   351,     0,   341,   344,     0,   335,   336,     0,     0,
       0,     0,     0,    79,     0,    80,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   211,   212,     0,
       0,    82,     0,     0,     0,     0,   113,     0,    72,    61,
      64,    70,     0,    63,    66,    67,    68,    69,    65,    71,
       0,    16,     0,     0,     0,     0,    17,     0,     0,     0,
      19,    46,     0,     0,     0,     0,     0,     0,    51,    54,
       0,     0,     0,   357,   368,   356,   364,   366,     0,     0,
     351,   345,   364,   366,     0,     0,   337,   169,   170,   171,
     172,   216,   173,   174,   175,   176,   216,   110,   323,     0,
       0,     0,     0,     7,    85,   188,     0,     0,     0,     0,
       0,     0,     0,     0,   210,   213,    93,     0,     0,     0,
       0,     0,     0,    54,   178,   177,   112,     0,     0,    40,
       0,   244,   259,     0,     0,     0,     0,     0,     0,     0,
       0,   245,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    14,
       0,    49,    31,    47,    32,    18,    33,    23,     0,    36,
       0,    37,    52,    38,    39,     0,    42,    12,     9,     0,
       0,     0,     0,   352,     0,     0,   339,   179,     0,   180,
       0,     0,    89,    90,     0,     0,    62,   191,     0,     0,
     185,   190,     0,     0,     0,     0,     0,     0,     0,   205,
     207,   185,   185,   213,     0,     0,     0,    93,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    13,     0,     0,
     222,   218,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   221,   223,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    25,     0,     0,
      45,     0,     0,     0,    22,     0,     0,    56,    55,   362,
       0,     0,   346,   359,   369,   358,   365,   367,     0,   338,
     217,   280,   107,     0,   286,   292,   109,   108,   325,   322,
     324,     0,    76,    78,   342,   197,   193,   186,   184,     0,
       0,    92,    73,    74,    84,   111,   203,   204,     0,   208,
       0,   213,   214,    87,    81,    95,    98,     0,    94,     0,
      75,   216,   216,   216,     0,    88,     0,    27,    28,    43,
      29,    30,   219,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   242,   241,   239,   238,   237,   231,
     232,   235,   236,   233,   234,   229,   230,   227,   228,   224,
     225,   226,    15,    26,    24,    50,    48,    44,    20,    21,
      35,    34,    53,    57,     0,     0,   353,   354,     0,   349,
     347,     0,   303,   295,     0,   303,     0,     0,    86,     0,
       0,   188,   189,     0,   206,   209,   215,   101,    97,   100,
       0,     0,    83,     0,     0,     0,     0,   343,    41,     0,
     252,   258,     0,     0,   256,     0,   243,   220,   247,   246,
     248,   249,     0,     0,   263,   264,   251,     0,   265,   250,
       0,    58,   370,   367,   360,   350,   348,     0,     0,   303,
       0,   269,   110,   310,     0,   311,   293,   328,   329,     0,
     201,     0,     0,   199,     0,     0,    91,     0,   105,    96,
      99,     0,   181,   182,   183,     0,     0,     0,     0,     0,
       0,     0,     0,   240,   371,     0,     0,     0,   297,   298,
     299,   300,   301,   304,     0,     0,     0,     0,   306,     0,
     271,     0,   309,   312,   269,     0,   332,     0,   326,     0,
     202,   198,   200,     0,   185,   194,     0,     0,   103,   114,
     253,   254,   255,   257,   260,   261,   262,   363,     0,   370,
     302,     0,   305,     0,     0,   273,   296,   275,   110,     0,
     329,     0,     0,    77,   216,     0,   102,     0,     0,   355,
       0,   303,     0,     0,   272,   275,     0,   287,     0,     0,
     330,     0,   327,   195,     0,   192,   106,     0,   361,     0,
       0,   268,     0,   281,     0,     0,   294,   333,   329,   216,
     104,     0,   307,   270,   279,     0,   288,   331,   196,     0,
     276,   277,   278,     0,   274,   317,   303,   282,     0,     0,
     159,   318,   289,   308,   136,   117,   116,   161,   162,   163,
     164,   165,     0,     0,     0,     0,     0,     0,   146,   148,
     153,     0,     0,     0,   147,     0,   118,     0,     0,   142,
     150,   158,   160,     0,     0,     0,     0,   314,     0,     0,
       0,     0,   155,   216,     0,   143,     0,     0,   115,     0,
     135,   185,     0,   137,     0,     0,   157,   283,   216,   145,
     159,     0,   267,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   159,     0,   166,     0,     0,   129,     0,
     133,     0,     0,   138,     0,   185,   185,     0,   314,     0,
       0,   313,     0,   315,     0,     0,   149,     0,   120,     0,
       0,   121,   122,   128,     0,   152,     0,   115,     0,     0,
     131,     0,   132,   134,   140,   139,   185,   267,   151,   319,
       0,   168,     0,     0,     0,     0,     0,   156,     0,   144,
     130,   119,   141,   315,   315,   266,   216,     0,   290,     0,
       0,     0,     0,     0,     0,   168,   168,   167,   316,   185,
     124,   123,     0,   125,   126,     0,   284,   320,   291,   127,
     154,   185,   185,   285,   321
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -648,  -648,   -69,  -648,  -648,  -648,  -648,   567,  -648,  -648,
    -648,  -648,  -648,  -648,   681,  -648,  -648,  -648,  -648,   605,
    -648,  -648,  -648,   590,  -648,  -314,  -648,  -648,  -648,  -648,
    -467,   -13,  -648,   -35,  -436,  -648,  -648,    94,  -600,   113,
    -648,  -648,   162,  -648,  -648,  -648,  -581,  -648,    79,  -588,
    -648,  -647,  -390,  -218,  -648,   417,  -648,   531,  -648,  -648,
    -648,  -648,  -648,  -648,   360,  -648,  -648,  -648,  -648,  -648,
    -648,  -209,  -110,  -648,   -75,   104,   292,  -648,  -648,   269,
    -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -648,  -648,  -648,  -648,  -477,   465,  -648,  -648,
     128,  -539,  -648,  -648,  -648,  -648,  -648,  -648,  -648,  -648,
    -648,  -648,  -512,  -648,  -648,  -648,  -648,   839,  -648,  -648,
    -648,  -648,  -648,   629,   -20,  -648,   766,   -14,  -648,  -648,
     302
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,   124,    11,    12,     9,    10,    19,    91,   247,
     185,   184,   182,   193,   194,   195,   308,     7,     8,    18,
      59,   137,   216,   235,   236,   358,   507,   588,   557,    60,
     210,   326,   142,   666,   667,   719,   668,   721,   691,   669,
     670,   717,   671,   684,   713,   672,   673,   674,   714,   778,
     116,   146,    62,   724,    63,   219,   220,   221,   335,   441,
     554,   605,   440,   502,   503,    64,    65,   231,    66,   232,
      67,   234,   715,   208,   252,   733,   540,   575,   595,   597,
     633,   327,   432,   624,   640,   728,   801,   434,   615,   635,
     677,   789,   435,   545,   492,   534,   490,   491,   495,   544,
     702,   761,   638,   676,   774,   802,    68,   211,   330,   436,
     582,   498,   548,   580,    15,    16,    26,    27,   104,    13,
      14,    69,    70,    23,    24,   431,    98,    99,   527,   425,
     525
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     179,   207,   339,   101,   149,    61,   209,   500,   151,   152,
     189,   105,   537,   348,   350,   295,   500,   198,   688,   245,
     688,   123,   747,   749,   352,   541,   448,   449,   699,   645,
    -187,   645,   300,   645,   233,   153,   154,   355,   356,   248,
     423,   546,   155,   156,   157,   217,   646,    21,   646,   328,
     646,   509,   510,  -187,    21,   457,   158,   159,   160,    17,
     225,   226,   641,   228,   230,   161,    20,   429,   600,   238,
     162,   688,   689,   653,   654,   655,    92,   485,   250,   251,
     163,    25,   645,   430,   693,   164,   165,   166,   167,   168,
     169,   170,   725,   486,   726,   645,   448,   449,   171,   646,
     172,   100,   273,   274,   329,   297,   627,   218,   331,   776,
     332,   598,   646,   304,   610,   777,   173,   642,   117,   730,
     338,    22,   174,   175,   723,   119,   120,   756,    22,   450,
     448,   449,   744,   320,   689,   653,   654,   655,   121,   547,
     357,   338,   446,   118,   448,   449,   144,   145,   800,   336,
     176,   750,   296,   122,   511,   501,   246,   177,   178,   639,
     662,   551,   663,   751,   501,   781,   229,   364,   665,   451,
     367,   368,   123,   370,   371,   301,   249,   351,   373,   374,
     375,   376,   377,   424,   126,   380,   310,   311,   420,   421,
     385,   386,   125,   556,   389,   390,   391,   796,   797,   450,
     393,   394,   395,   396,   397,   398,   399,   400,   401,   402,
     403,   404,   405,   406,   407,   408,   409,   410,   411,   412,
     413,   414,   338,    93,   663,   127,   755,   418,   419,   690,
     694,   128,   695,   450,   785,   786,   151,   152,    93,   451,
     338,   586,   347,   587,   338,   313,   349,   450,   129,   688,
     675,   453,   454,   455,   433,   536,   690,   609,   690,   338,
     645,   754,   130,   153,   154,   313,   131,   291,   292,   293,
     155,   156,   157,   451,   607,   606,   338,   646,   772,   444,
     132,   445,   675,   688,   158,   159,   160,   451,   753,   620,
     133,   690,   134,   161,   645,   630,   631,   632,   162,   689,
     653,   654,   655,   528,   529,   530,   531,   532,   163,   135,
     675,   646,   136,   164,   165,   166,   167,   168,   169,   170,
     138,    61,   139,   675,   140,   141,   171,   143,   172,   147,
     688,   148,   150,   703,   704,   180,   585,   181,   183,   186,
     187,   645,   188,   190,   173,   192,   101,   191,   200,    94,
     174,   175,    95,    96,    97,   196,   199,   489,   646,   494,
     489,   497,   197,   201,    94,   202,   205,    95,   102,   103,
     203,   314,   151,   152,   315,   316,   317,   212,   176,   302,
     703,   704,   533,   213,   516,   177,   178,   517,   518,   663,
     519,   314,   206,   214,   315,   316,   483,   520,   521,   153,
     154,   215,   222,   223,   224,   523,   155,   156,   157,   227,
     233,   482,   528,   529,   530,   531,   532,   688,   237,   239,
     158,   159,   160,   706,   240,   241,   705,   242,   645,   161,
     244,   253,   254,   255,   162,   321,     1,     2,     3,   289,
     290,   291,   292,   293,   163,   646,   298,     4,   256,   164,
     165,   166,   167,   168,   169,   170,     5,   307,   299,   257,
     571,   258,   171,   259,   172,   260,   261,   708,   709,   262,
     706,   263,   264,   265,   603,   266,   267,   268,   269,   270,
     173,   271,   272,   309,   305,   361,   174,   175,   312,   318,
     322,   533,   306,   362,   319,   340,   337,   323,   592,   333,
     363,   378,   341,   342,   599,   324,   601,   343,   344,   628,
      43,   345,   568,   569,   176,   346,   303,   353,   612,   354,
     360,   177,   178,   321,   151,   152,   365,   379,   381,   325,
     382,   383,   384,   387,   388,    53,    54,    55,   392,   625,
     415,   416,   417,   422,   428,   437,   629,   439,    56,   426,
     427,   153,   154,   443,   447,   576,   452,   465,   487,   156,
     157,   488,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   798,   158,   159,   160,   481,   466,   504,   484,   508,
     515,   161,   468,   803,   804,   323,   162,   682,   729,   522,
     526,   535,   469,   324,   499,   470,   163,   471,    43,   151,
     152,   164,   165,   166,   167,   168,   169,   170,   474,   475,
     479,   506,   493,   512,   171,   513,   172,   325,   514,   524,
     538,   549,   550,    53,    54,    55,   153,   154,   555,   559,
     558,   566,   173,   155,   156,   157,    56,   539,   174,   175,
     543,   746,   567,   707,   710,   711,   712,   158,   159,   160,
     553,   570,   720,   572,   574,   578,   161,   573,   579,   581,
     583,   162,   584,   338,   593,   596,   176,   604,   594,   608,
     589,   163,   614,   177,   178,   619,   164,   165,   166,   167,
     168,   169,   170,   151,   152,   621,   752,   707,   622,   171,
     626,   172,   287,   288,   289,   290,   291,   292,   293,   762,
     763,   602,   720,   637,   765,   766,   536,   173,   643,   697,
     153,   154,   678,   174,   175,   679,   771,   155,   156,   157,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   752,
     680,   158,   159,   160,   681,   683,   685,   686,   687,   692,
     161,   176,   698,   700,   701,   162,   792,   716,   177,   178,
     718,  -115,   722,  -115,   727,   163,   731,   732,   734,   735,
     164,   165,   166,   167,   168,   169,   170,   644,   736,   737,
     739,   740,   738,   171,   741,   172,   760,   742,   645,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     743,   173,   745,   758,   759,   646,   767,   174,   175,   770,
    -136,   775,   769,   779,   647,   648,   649,   650,   651,   780,
     782,   783,   784,   788,   369,   790,   791,   652,   653,   654,
     655,   334,   793,   794,   243,   176,   795,   359,   799,   656,
     748,   764,   177,   178,   696,   275,   577,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   787,   757,   657,   505,   658,
      28,   773,   552,   659,   613,   106,   204,    53,    54,    55,
     442,   590,     0,   366,     0,     0,     0,     0,     0,   275,
     660,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,     0,
     496,   661,    29,    30,    31,   662,     0,   663,     0,     0,
       0,   664,     0,   665,     0,     0,     0,    32,    33,    34,
      35,     0,    36,    37,    38,    39,     0,     0,     0,     0,
       0,     0,    40,    41,    42,    43,    28,     0,     0,     0,
       0,     0,     0,    44,    45,    46,    47,    48,    49,     0,
       0,     0,     0,    50,    51,    52,     0,     0,     0,     0,
      53,    54,    55,     0,     0,     0,     0,   459,   438,   460,
       0,     0,     0,    56,     0,     0,     0,     0,    29,    30,
      31,     0,     0,     0,     0,     0,    57,     0,     0,     0,
       0,     0,  -342,    32,    33,    34,    35,     0,    36,    37,
      38,    39,     0,     0,    58,     0,     0,     0,    40,    41,
      42,    43,     0,   372,     0,     0,     0,     0,     0,    44,
      45,    46,    47,    48,    49,     0,     0,     0,     0,    50,
      51,    52,     0,     0,     0,     0,    53,    54,    55,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    56,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    57,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
      58,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
       0,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
       0,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
       0,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
       0,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
       0,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   275,   461,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   275,   464,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   275,   467,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   275,   476,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   275,   478,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   275,   560,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   275,   561,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   275,   562,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   275,   563,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   275,   564,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   275,   565,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   275,   591,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   275,   611,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,   617,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,   618,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,   623,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,   634,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,   636,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,     0,   321,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   275,
     294,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
     456,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,     0,
     462,     0,     0,     0,   616,     0,     0,     0,    71,     0,
       0,   323,     0,     0,     0,     0,     0,     0,     0,   324,
     463,     0,     0,     0,    43,     0,     0,     0,     0,     0,
       0,     0,    71,     0,     0,     0,     0,     0,     0,     0,
     472,     0,    72,   325,     0,     0,     0,     0,     0,    53,
      54,    55,     0,     0,     0,     0,     0,     0,     0,     0,
     473,     0,    56,     0,   458,     0,    72,     0,     0,     0,
      73,   275,     0,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   477,     0,   536,    73,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    74,     0,     0,     0,     0,
       0,   768,    75,    76,    77,    78,    79,   -43,    80,    81,
      82,     0,     0,    83,    84,     0,    85,    86,    87,    74,
       0,     0,     0,    88,    89,    90,    75,    76,    77,    78,
      79,     0,    80,    81,    82,     0,     0,    83,    84,     0,
      85,    86,    87,     0,     0,     0,     0,    88,    89,    90,
     275,   480,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,   542,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,     0,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293
};

static const yytype_int16 yycheck[] =
{
      75,   111,   220,    23,    73,    18,   116,     4,     3,     4,
      85,    25,   489,   231,   232,    10,     4,    92,     4,    10,
       4,     4,     4,    21,   233,   492,     4,     5,   675,    15,
      36,    15,     4,    15,     4,    30,    31,     4,     5,    10,
       4,     4,    37,    38,    39,     4,    32,    58,    32,     4,
      32,     4,     5,    59,    58,    59,    51,    52,    53,     0,
     129,   130,     4,   132,   133,    60,     4,   132,   580,   138,
      65,     4,    54,    55,    56,    57,    10,   132,   153,   154,
      75,    58,    15,   148,     4,    80,    81,    82,    83,    84,
      85,    86,   692,   148,   694,    15,     4,     5,    93,    32,
      95,    58,   177,   178,    59,   180,   618,    66,   147,    10,
     149,   578,    32,   188,   591,    16,   111,    59,    58,   700,
     147,   132,   117,   118,   151,    49,    50,   727,   132,   107,
       4,     5,   713,   208,    54,    55,    56,    57,    37,   102,
     107,   147,   351,    58,     4,     5,   147,   148,   795,   218,
     145,   149,   147,    37,   107,   152,   147,   152,   153,   636,
     142,   149,   144,   149,   152,   149,   149,   242,   150,   147,
     245,   246,     4,   248,   249,   147,   147,   147,   253,   254,
     255,   256,   257,   147,    37,   260,   200,   201,     3,     4,
     265,   266,    58,   507,   269,   270,   271,   785,   786,   107,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   147,     4,   144,    37,   151,   302,   303,   665,
     150,    37,   668,   107,   773,   774,     3,     4,     4,   147,
     147,   149,   149,   557,   147,     4,   149,   107,    37,     4,
     640,   361,   362,   363,   323,    37,   692,    39,   694,   147,
      15,   149,    37,    30,    31,     4,    37,    32,    33,    34,
      37,    38,    39,   147,   588,   149,   147,    32,   149,   348,
      37,   350,   672,     4,    51,    52,    53,   147,   724,   149,
      37,   727,    37,    60,    15,   139,   140,   141,    65,    54,
      55,    56,    57,    70,    71,    72,    73,    74,    75,    37,
     700,    32,    37,    80,    81,    82,    83,    84,    85,    86,
      37,   334,    37,   713,    37,    37,    93,    37,    95,   136,
       4,     4,     4,    54,    55,     4,   554,     4,     4,     4,
       4,    15,     4,     4,   111,     4,   366,     3,    16,   130,
     117,   118,   133,   134,   135,     4,     4,   432,    32,   434,
     435,   436,   113,    16,   130,    59,    59,   133,   134,   135,
     148,   130,     3,     4,   133,   134,   135,     4,   145,    10,
      54,    55,   149,     4,   459,   152,   153,   462,   463,   144,
     465,   130,   148,     4,   133,   134,   135,   472,   473,    30,
      31,     4,     4,     4,     4,   480,    37,    38,    39,     4,
       4,   425,    70,    71,    72,    73,    74,     4,    37,     4,
      51,    52,    53,   144,     4,     4,   100,    37,    15,    60,
      58,    37,    37,    37,    65,     4,   126,   127,   128,    30,
      31,    32,    33,    34,    75,    32,   147,   137,    37,    80,
      81,    82,    83,    84,    85,    86,   146,     4,   147,    37,
     535,    37,    93,    37,    95,    37,    37,    54,    55,    37,
     144,    37,    37,    37,   584,    37,    37,    37,    37,    37,
     111,    37,    37,    58,   147,    10,   117,   118,   148,    59,
      59,   149,   147,    10,   148,   147,    59,    66,   573,   149,
      10,     4,   149,   149,   579,    74,   581,   149,   149,   619,
      79,   149,   526,   527,   145,   149,   147,   149,   593,   149,
     149,   152,   153,     4,     3,     4,   149,     4,     4,    98,
       4,     4,     4,     4,     4,   104,   105,   106,     4,   614,
       4,     4,     4,     4,     4,     4,   621,    37,   117,   148,
     148,    30,    31,     4,    37,    36,   149,   147,    37,    38,
      39,    40,     6,     7,     8,     9,    10,    11,    12,    13,
      14,   789,    51,    52,    53,     4,   149,    16,    58,    37,
       4,    60,   149,   801,   802,    66,    65,   656,   698,     4,
      16,    37,   149,    74,   147,   149,    75,   149,    79,     3,
       4,    80,    81,    82,    83,    84,    85,    86,   149,   149,
     149,   149,    16,   149,    93,   149,    95,    98,   149,   148,
      16,     4,     4,   104,   105,   106,    30,    31,    36,   149,
      37,   149,   111,    37,    38,    39,   117,   102,   117,   118,
      92,   716,    59,   678,   679,   680,   681,    51,    52,    53,
      93,   149,   687,    16,    38,    58,    60,    37,    37,    37,
     149,    65,    10,   147,    37,   103,   145,    95,   143,    59,
     148,    75,    37,   152,   153,    10,    80,    81,    82,    83,
      84,    85,    86,     3,     4,    37,   721,   722,    16,    93,
      58,    95,    28,    29,    30,    31,    32,    33,    34,   734,
     735,   148,   737,    58,   739,   740,    37,   111,    16,    59,
      30,    31,    37,   117,   118,    37,   751,    37,    38,    39,
       6,     7,     8,     9,    10,    11,    12,    13,    14,   764,
      37,    51,    52,    53,    37,    37,    37,    37,    37,    37,
      60,   145,    37,    58,    25,    65,   781,    37,   152,   153,
       4,    37,    37,    37,    37,    75,     4,   102,    37,    37,
      80,    81,    82,    83,    84,    85,    86,     4,   149,    37,
      37,    37,   149,    93,   149,    95,    25,   149,    15,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
     149,   111,   149,   149,    59,    32,    36,   117,   118,     4,
     149,     4,   149,   149,    41,    42,    43,    44,    45,   149,
     149,   149,     4,     4,   247,   149,   149,    54,    55,    56,
      57,   216,   149,   149,   143,   145,   149,   237,   149,    66,
     717,   737,   152,   153,   672,    15,   544,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,   776,   728,    94,   441,    96,
       4,   757,   502,   100,   595,    26,   100,   104,   105,   106,
     339,   569,    -1,   244,    -1,    -1,    -1,    -1,    -1,    15,
     117,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
     435,   138,    46,    47,    48,   142,    -1,   144,    -1,    -1,
      -1,   148,    -1,   150,    -1,    -1,    -1,    61,    62,    63,
      64,    -1,    66,    67,    68,    69,    -1,    -1,    -1,    -1,
      -1,    -1,    76,    77,    78,    79,     4,    -1,    -1,    -1,
      -1,    -1,    -1,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    -1,    97,    98,    99,    -1,    -1,    -1,    -1,
     104,   105,   106,    -1,    -1,    -1,    -1,   147,    36,   149,
      -1,    -1,    -1,   117,    -1,    -1,    -1,    -1,    46,    47,
      48,    -1,    -1,    -1,    -1,    -1,   130,    -1,    -1,    -1,
      -1,    -1,   136,    61,    62,    63,    64,    -1,    66,    67,
      68,    69,    -1,    -1,   148,    -1,    -1,    -1,    76,    77,
      78,    79,    -1,   149,    -1,    -1,    -1,    -1,    -1,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    97,
      98,    99,    -1,    -1,    -1,    -1,   104,   105,   106,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   130,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     148,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   149,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   149,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   149,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   149,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   149,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   149,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    15,   149,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   149,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   149,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   149,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   149,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   149,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,   149,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   149,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   149,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   149,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   149,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   149,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,     4,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,
     147,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     147,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
     147,    -1,    -1,    -1,    59,    -1,    -1,    -1,     4,    -1,
      -1,    66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    74,
     147,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     147,    -1,    38,    98,    -1,    -1,    -1,    -1,    -1,   104,
     105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     147,    -1,   117,    -1,    36,    -1,    38,    -1,    -1,    -1,
      66,    15,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,   147,    -1,    37,    66,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   101,    -1,    -1,    -1,    -1,
      -1,   147,   108,   109,   110,   111,   112,   113,   114,   115,
     116,    -1,    -1,   119,   120,    -1,   122,   123,   124,   101,
      -1,    -1,    -1,   129,   130,   131,   108,   109,   110,   111,
     112,    -1,   114,   115,   116,    -1,    -1,   119,   120,    -1,
     122,   123,   124,    -1,    -1,    -1,    -1,   129,   130,   131,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   126,   127,   128,   137,   146,   155,   171,   172,   159,
     160,   157,   158,   273,   274,   268,   269,     0,   173,   161,
       4,    58,   132,   277,   278,    58,   270,   271,     4,    46,
      47,    48,    61,    62,    63,    64,    66,    67,    68,    69,
      76,    77,    78,    79,    87,    88,    89,    90,    91,    92,
      97,    98,    99,   104,   105,   106,   117,   130,   148,   174,
     183,   185,   206,   208,   219,   220,   222,   224,   260,   275,
     276,     4,    38,    66,   101,   108,   109,   110,   111,   112,
     114,   115,   116,   119,   120,   122,   123,   124,   129,   130,
     131,   162,    10,     4,   130,   133,   134,   135,   280,   281,
      58,   278,   134,   135,   272,   281,   271,     6,     7,     8,
       9,    10,    11,    12,    13,    14,   204,    58,    58,    49,
      50,    37,    37,     4,   156,    58,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,   175,    37,    37,
      37,    37,   186,    37,   147,   148,   205,   136,     4,   156,
       4,     3,     4,    30,    31,    37,    38,    39,    51,    52,
      53,    60,    65,    75,    80,    81,    82,    83,    84,    85,
      86,    93,    95,   111,   117,   118,   145,   152,   153,   228,
       4,     4,   166,     4,   165,   164,     4,     4,     4,   228,
       4,     3,     4,   167,   168,   169,     4,   113,   228,     4,
      16,    16,    59,   148,   280,    59,   148,   226,   227,   226,
     184,   261,     4,     4,     4,     4,   176,     4,    66,   209,
     210,   211,     4,     4,     4,   156,   156,     4,   156,   149,
     156,   221,   223,     4,   225,   177,   178,    37,   156,     4,
       4,     4,    37,   168,    58,    10,   147,   163,    10,   147,
     228,   228,   228,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,   228,   228,    15,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,   147,    10,   147,   228,   147,   147,
       4,   147,    10,   147,   228,   147,   147,     4,   170,    58,
     281,   281,   148,     4,   130,   133,   134,   135,    59,   148,
     228,     4,    59,    66,    74,    98,   185,   235,     4,    59,
     262,   147,   149,   149,   173,   212,   156,    59,   147,   207,
     147,   149,   149,   149,   149,   149,   149,   149,   207,   149,
     207,   147,   225,   149,   149,     4,     5,   107,   179,   177,
     149,    10,    10,    10,   228,   149,   277,   228,   228,   161,
     228,   228,   149,   228,   228,   228,   228,   228,     4,     4,
     228,     4,     4,     4,     4,   228,   228,     4,     4,   228,
     228,   228,     4,   228,   228,   228,   228,   228,   228,   228,
     228,   228,   228,   228,   228,   228,   228,   228,   228,   228,
     228,   228,   228,   228,   228,     4,     4,     4,   228,   228,
       3,     4,     4,     4,   147,   283,   148,   148,     4,   132,
     148,   279,   236,   156,   241,   246,   263,     4,    36,    37,
     216,   213,   211,     4,   156,   156,   225,    37,     4,     5,
     107,   147,   149,   226,   226,   226,   147,    59,    36,   147,
     149,   149,   147,   147,   149,   147,   149,   149,   149,   149,
     149,   149,   147,   147,   149,   149,   149,   147,   149,   149,
      16,     4,   281,   135,    58,   132,   148,    37,    40,   228,
     250,   251,   248,    16,   228,   252,   251,   228,   265,   147,
       4,   152,   217,   218,    16,   209,   149,   180,    37,     4,
       5,   107,   149,   149,   149,     4,   228,   228,   228,   228,
     228,   228,     4,   228,   148,   284,    16,   282,    70,    71,
      72,    73,    74,   149,   249,    37,    37,   250,    16,   102,
     230,   184,    16,    92,   253,   247,     4,   102,   266,     4,
       4,   149,   218,    93,   214,    36,   179,   182,    37,   149,
     149,   149,   149,   149,   149,   149,   149,    59,   281,   281,
     149,   228,    16,    37,    38,   231,    36,   230,    58,    37,
     267,    37,   264,   149,    10,   207,   149,   179,   181,   148,
     284,   149,   228,    37,   143,   232,   103,   233,   184,   228,
     266,   228,   148,   226,    95,   215,   149,   179,    59,    39,
     250,   149,   228,   233,    37,   242,    59,   149,   149,    10,
     149,    37,    16,   149,   237,   228,    58,   266,   226,   228,
     139,   140,   141,   234,   149,   243,   149,    58,   256,   250,
     238,     4,    59,    16,     4,    15,    32,    41,    42,    43,
      44,    45,    54,    55,    56,    57,    66,    94,    96,   100,
     117,   138,   142,   144,   148,   150,   187,   188,   190,   193,
     194,   196,   199,   200,   201,   206,   257,   244,    37,    37,
      37,    37,   156,    37,   197,    37,    37,    37,     4,    54,
     188,   192,    37,     4,   150,   188,   196,    59,    37,   205,
      58,    25,   254,    54,    55,   100,   144,   187,    54,    55,
     187,   187,   187,   198,   202,   226,    37,   195,     4,   189,
     187,   191,    37,   151,   207,   192,   192,    37,   239,   226,
     200,     4,   102,   229,    37,    37,   149,    37,   149,    37,
      37,   149,   149,   149,   200,   149,   228,     4,   193,    21,
     149,   149,   187,   188,   149,   151,   192,   254,   149,    59,
      25,   255,   187,   187,   191,   187,   187,    36,   147,   149,
       4,   187,   149,   229,   258,     4,    10,    16,   203,   149,
     149,   149,   149,   149,     4,   255,   255,   202,     4,   245,
     149,   149,   187,   149,   149,   149,   203,   203,   207,   149,
     205,   240,   259,   207,   207
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   154,   155,   155,   155,   155,   155,   156,   158,   157,
     160,   159,   161,   161,   162,   162,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     163,   162,   162,   162,   164,   164,   164,   165,   165,   166,
     166,   167,   167,   167,   169,   168,   170,   170,   170,   172,
     171,   173,   173,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   175,   174,   174,   176,   174,   174,   174,   174,
     174,   174,   174,   178,   177,   179,   179,   179,   179,   179,
     179,   180,   179,   181,   179,   182,   179,   183,   184,   184,
     184,   185,   185,   186,   185,   187,   187,   187,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   189,
     189,   190,   191,   191,   192,   192,   193,   193,   193,   193,
     193,   193,   194,   195,   194,   196,   196,   196,   196,   196,
     196,   196,   196,   197,   196,   198,   196,   199,   199,   200,
     200,   201,   201,   201,   201,   201,   202,   203,   203,   204,
     204,   204,   204,   204,   204,   204,   204,   205,   205,   206,
     206,   206,   206,   206,   207,   207,   208,   209,   209,   210,
     210,   212,   211,   213,   211,   214,   215,   216,   216,   217,
     217,   218,   218,   219,   220,   220,   221,   221,   222,   223,
     223,   224,   224,   225,   225,   225,   227,   226,   228,   228,
     228,   228,   228,   228,   228,   228,   228,   228,   228,   228,
     228,   228,   228,   228,   228,   228,   228,   228,   228,   228,
     228,   228,   228,   228,   228,   228,   228,   228,   228,   228,
     228,   228,   228,   228,   228,   228,   228,   228,   228,   228,
     228,   228,   228,   228,   228,   228,   229,   229,   230,   230,
     231,   231,   232,   232,   233,   233,   234,   234,   234,   234,
     236,   237,   238,   239,   240,   235,   241,   242,   243,   244,
     245,   235,   246,   247,   235,   248,   235,   249,   249,   249,
     249,   249,   250,   250,   250,   251,   251,   251,   251,   252,
     252,   253,   253,   254,   254,   255,   255,   256,   257,   258,
     259,   256,   260,   261,   261,   263,   264,   262,   265,   266,
     266,   266,   267,   267,   269,   268,   270,   270,   271,   272,
     274,   273,   276,   275,   277,   277,   278,   278,   278,   279,
     279,   280,   280,   280,   280,   280,   281,   281,   281,   281,
     282,   281,   283,   281,   281,   281,   281,   281,   281,   281,
     284,   284
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     2,     2,     2,     2,     1,     0,     4,
       0,     2,     3,     0,     2,     4,     1,     1,     2,     1,
       4,     4,     3,     2,     4,     3,     4,     4,     4,     4,
       4,     2,     2,     2,     4,     4,     2,     2,     2,     2,
       0,     5,     2,     0,     3,     2,     0,     1,     3,     1,
       3,     0,     1,     3,     0,     2,     1,     2,     3,     0,
       2,     2,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     4,     4,     8,     4,     1,
       1,     4,     0,     5,     4,     0,     5,     4,     4,     3,
       3,     6,     4,     0,     2,     1,     3,     2,     1,     3,
       2,     0,     5,     0,     7,     0,     6,     4,     2,     2,
       0,     4,     2,     0,     7,     1,     1,     1,     1,     5,
       4,     4,     4,     7,     7,     7,     7,     8,     4,     1,
       3,     4,     2,     1,     3,     1,     1,     2,     3,     4,
       4,     5,     1,     0,     5,     2,     1,     1,     1,     4,
       1,     4,     4,     0,     8,     0,     5,     2,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     6,     6,     6,     1,     0,     4,     1,     0,     3,
       1,     0,     7,     0,     5,     3,     3,     0,     3,     1,
       2,     1,     2,     4,     4,     3,     3,     1,     4,     3,
       0,     1,     1,     0,     2,     3,     0,     2,     2,     3,
       4,     2,     2,     2,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       5,     3,     3,     4,     1,     1,     4,     4,     4,     4,
       4,     4,     4,     6,     6,     6,     4,     6,     4,     1,
       6,     6,     6,     4,     4,     4,     3,     0,     4,     0,
       4,     0,     1,     0,     4,     0,     1,     1,     1,     0,
       0,     0,     0,     0,     0,    20,     0,     0,     0,     0,
       0,    18,     0,     0,     7,     0,     5,     1,     1,     1,
       1,     1,     3,     0,     2,     3,     2,     6,    10,     2,
       1,     0,     1,     2,     0,     0,     3,     0,     0,     0,
       0,    11,     4,     0,     2,     0,     0,     6,     1,     0,
       3,     5,     0,     3,     0,     2,     1,     2,     4,     2,
       0,     2,     0,     5,     1,     2,     4,     5,     6,     1,
       2,     0,     2,     4,     4,     8,     1,     1,     3,     3,
       0,     9,     0,     7,     1,     3,     1,     3,     1,     3,
       0,     1
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
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
        case 8:
#line 178 "ldgram.y" /* yacc.c:1661  */
    { ldlex_defsym(); }
#line 2335 "ldgram.c" /* yacc.c:1661  */
    break;

  case 9:
#line 180 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_popstate();
		  lang_add_assignment (exp_defsym ((yyvsp[-2].name), (yyvsp[0].etree)));
		}
#line 2344 "ldgram.c" /* yacc.c:1661  */
    break;

  case 10:
#line 188 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
#line 2353 "ldgram.c" /* yacc.c:1661  */
    break;

  case 11:
#line 193 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
#line 2363 "ldgram.c" /* yacc.c:1661  */
    break;

  case 16:
#line 208 "ldgram.y" /* yacc.c:1661  */
    {
			einfo(_("%P%F: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[0].name));
			}
#line 2371 "ldgram.c" /* yacc.c:1661  */
    break;

  case 17:
#line 211 "ldgram.y" /* yacc.c:1661  */
    {
			config.map_filename = "-";
			}
#line 2379 "ldgram.c" /* yacc.c:1661  */
    break;

  case 20:
#line 217 "ldgram.y" /* yacc.c:1661  */
    { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2385 "ldgram.c" /* yacc.c:1661  */
    break;

  case 21:
#line 219 "ldgram.y" /* yacc.c:1661  */
    { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2391 "ldgram.c" /* yacc.c:1661  */
    break;

  case 22:
#line 221 "ldgram.y" /* yacc.c:1661  */
    { mri_public((yyvsp[-1].name), (yyvsp[0].etree)); }
#line 2397 "ldgram.c" /* yacc.c:1661  */
    break;

  case 23:
#line 223 "ldgram.y" /* yacc.c:1661  */
    { mri_format((yyvsp[0].name)); }
#line 2403 "ldgram.c" /* yacc.c:1661  */
    break;

  case 24:
#line 225 "ldgram.y" /* yacc.c:1661  */
    { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2409 "ldgram.c" /* yacc.c:1661  */
    break;

  case 25:
#line 227 "ldgram.y" /* yacc.c:1661  */
    { mri_output_section((yyvsp[-1].name), (yyvsp[0].etree));}
#line 2415 "ldgram.c" /* yacc.c:1661  */
    break;

  case 26:
#line 229 "ldgram.y" /* yacc.c:1661  */
    { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2421 "ldgram.c" /* yacc.c:1661  */
    break;

  case 27:
#line 231 "ldgram.y" /* yacc.c:1661  */
    { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2427 "ldgram.c" /* yacc.c:1661  */
    break;

  case 28:
#line 233 "ldgram.y" /* yacc.c:1661  */
    { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2433 "ldgram.c" /* yacc.c:1661  */
    break;

  case 29:
#line 235 "ldgram.y" /* yacc.c:1661  */
    { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2439 "ldgram.c" /* yacc.c:1661  */
    break;

  case 30:
#line 237 "ldgram.y" /* yacc.c:1661  */
    { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2445 "ldgram.c" /* yacc.c:1661  */
    break;

  case 33:
#line 241 "ldgram.y" /* yacc.c:1661  */
    { mri_name((yyvsp[0].name)); }
#line 2451 "ldgram.c" /* yacc.c:1661  */
    break;

  case 34:
#line 243 "ldgram.y" /* yacc.c:1661  */
    { mri_alias((yyvsp[-2].name),(yyvsp[0].name),0);}
#line 2457 "ldgram.c" /* yacc.c:1661  */
    break;

  case 35:
#line 245 "ldgram.y" /* yacc.c:1661  */
    { mri_alias ((yyvsp[-2].name), 0, (int) (yyvsp[0].bigint).integer); }
#line 2463 "ldgram.c" /* yacc.c:1661  */
    break;

  case 36:
#line 247 "ldgram.y" /* yacc.c:1661  */
    { mri_base((yyvsp[0].etree)); }
#line 2469 "ldgram.c" /* yacc.c:1661  */
    break;

  case 37:
#line 249 "ldgram.y" /* yacc.c:1661  */
    { mri_truncate ((unsigned int) (yyvsp[0].bigint).integer); }
#line 2475 "ldgram.c" /* yacc.c:1661  */
    break;

  case 40:
#line 253 "ldgram.y" /* yacc.c:1661  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 2481 "ldgram.c" /* yacc.c:1661  */
    break;

  case 41:
#line 255 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); }
#line 2487 "ldgram.c" /* yacc.c:1661  */
    break;

  case 42:
#line 257 "ldgram.y" /* yacc.c:1661  */
    { lang_add_entry ((yyvsp[0].name), FALSE); }
#line 2493 "ldgram.c" /* yacc.c:1661  */
    break;

  case 44:
#line 262 "ldgram.y" /* yacc.c:1661  */
    { mri_order((yyvsp[0].name)); }
#line 2499 "ldgram.c" /* yacc.c:1661  */
    break;

  case 45:
#line 263 "ldgram.y" /* yacc.c:1661  */
    { mri_order((yyvsp[0].name)); }
#line 2505 "ldgram.c" /* yacc.c:1661  */
    break;

  case 47:
#line 269 "ldgram.y" /* yacc.c:1661  */
    { mri_load((yyvsp[0].name)); }
#line 2511 "ldgram.c" /* yacc.c:1661  */
    break;

  case 48:
#line 270 "ldgram.y" /* yacc.c:1661  */
    { mri_load((yyvsp[0].name)); }
#line 2517 "ldgram.c" /* yacc.c:1661  */
    break;

  case 49:
#line 275 "ldgram.y" /* yacc.c:1661  */
    { mri_only_load((yyvsp[0].name)); }
#line 2523 "ldgram.c" /* yacc.c:1661  */
    break;

  case 50:
#line 277 "ldgram.y" /* yacc.c:1661  */
    { mri_only_load((yyvsp[0].name)); }
#line 2529 "ldgram.c" /* yacc.c:1661  */
    break;

  case 51:
#line 281 "ldgram.y" /* yacc.c:1661  */
    { (yyval.name) = NULL; }
#line 2535 "ldgram.c" /* yacc.c:1661  */
    break;

  case 54:
#line 288 "ldgram.y" /* yacc.c:1661  */
    { ldlex_expression (); }
#line 2541 "ldgram.c" /* yacc.c:1661  */
    break;

  case 55:
#line 290 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); }
#line 2547 "ldgram.c" /* yacc.c:1661  */
    break;

  case 56:
#line 294 "ldgram.y" /* yacc.c:1661  */
    { ldlang_add_undef ((yyvsp[0].name), FALSE); }
#line 2553 "ldgram.c" /* yacc.c:1661  */
    break;

  case 57:
#line 296 "ldgram.y" /* yacc.c:1661  */
    { ldlang_add_undef ((yyvsp[0].name), FALSE); }
#line 2559 "ldgram.c" /* yacc.c:1661  */
    break;

  case 58:
#line 298 "ldgram.y" /* yacc.c:1661  */
    { ldlang_add_undef ((yyvsp[0].name), FALSE); }
#line 2565 "ldgram.c" /* yacc.c:1661  */
    break;

  case 59:
#line 302 "ldgram.y" /* yacc.c:1661  */
    { ldlex_both(); }
#line 2571 "ldgram.c" /* yacc.c:1661  */
    break;

  case 60:
#line 304 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate(); }
#line 2577 "ldgram.c" /* yacc.c:1661  */
    break;

  case 73:
#line 325 "ldgram.y" /* yacc.c:1661  */
    { lang_add_target((yyvsp[-1].name)); }
#line 2583 "ldgram.c" /* yacc.c:1661  */
    break;

  case 74:
#line 327 "ldgram.y" /* yacc.c:1661  */
    { ldfile_add_library_path ((yyvsp[-1].name), FALSE); }
#line 2589 "ldgram.c" /* yacc.c:1661  */
    break;

  case 75:
#line 329 "ldgram.y" /* yacc.c:1661  */
    { lang_add_output((yyvsp[-1].name), 1); }
#line 2595 "ldgram.c" /* yacc.c:1661  */
    break;

  case 76:
#line 331 "ldgram.y" /* yacc.c:1661  */
    { lang_add_output_format ((yyvsp[-1].name), (char *) NULL,
					    (char *) NULL, 1); }
#line 2602 "ldgram.c" /* yacc.c:1661  */
    break;

  case 77:
#line 334 "ldgram.y" /* yacc.c:1661  */
    { lang_add_output_format ((yyvsp[-5].name), (yyvsp[-3].name), (yyvsp[-1].name), 1); }
#line 2608 "ldgram.c" /* yacc.c:1661  */
    break;

  case 78:
#line 336 "ldgram.y" /* yacc.c:1661  */
    { ldfile_set_output_arch ((yyvsp[-1].name), bfd_arch_unknown); }
#line 2614 "ldgram.c" /* yacc.c:1661  */
    break;

  case 79:
#line 338 "ldgram.y" /* yacc.c:1661  */
    { command_line.force_common_definition = TRUE ; }
#line 2620 "ldgram.c" /* yacc.c:1661  */
    break;

  case 80:
#line 340 "ldgram.y" /* yacc.c:1661  */
    { command_line.inhibit_common_definition = TRUE ; }
#line 2626 "ldgram.c" /* yacc.c:1661  */
    break;

  case 82:
#line 343 "ldgram.y" /* yacc.c:1661  */
    { lang_enter_group (); }
#line 2632 "ldgram.c" /* yacc.c:1661  */
    break;

  case 83:
#line 345 "ldgram.y" /* yacc.c:1661  */
    { lang_leave_group (); }
#line 2638 "ldgram.c" /* yacc.c:1661  */
    break;

  case 84:
#line 347 "ldgram.y" /* yacc.c:1661  */
    { lang_add_map((yyvsp[-1].name)); }
#line 2644 "ldgram.c" /* yacc.c:1661  */
    break;

  case 85:
#line 349 "ldgram.y" /* yacc.c:1661  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 2650 "ldgram.c" /* yacc.c:1661  */
    break;

  case 86:
#line 351 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); }
#line 2656 "ldgram.c" /* yacc.c:1661  */
    break;

  case 87:
#line 353 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_add_nocrossref ((yyvsp[-1].nocrossref));
		}
#line 2664 "ldgram.c" /* yacc.c:1661  */
    break;

  case 89:
#line 358 "ldgram.y" /* yacc.c:1661  */
    { lang_add_insert ((yyvsp[0].name), 0); }
#line 2670 "ldgram.c" /* yacc.c:1661  */
    break;

  case 90:
#line 360 "ldgram.y" /* yacc.c:1661  */
    { lang_add_insert ((yyvsp[0].name), 1); }
#line 2676 "ldgram.c" /* yacc.c:1661  */
    break;

  case 91:
#line 362 "ldgram.y" /* yacc.c:1661  */
    { lang_memory_region_alias ((yyvsp[-3].name), (yyvsp[-1].name)); }
#line 2682 "ldgram.c" /* yacc.c:1661  */
    break;

  case 92:
#line 364 "ldgram.y" /* yacc.c:1661  */
    { lang_ld_feature ((yyvsp[-1].name)); }
#line 2688 "ldgram.c" /* yacc.c:1661  */
    break;

  case 93:
#line 368 "ldgram.y" /* yacc.c:1661  */
    { ldlex_inputlist(); }
#line 2694 "ldgram.c" /* yacc.c:1661  */
    break;

  case 94:
#line 370 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate(); }
#line 2700 "ldgram.c" /* yacc.c:1661  */
    break;

  case 95:
#line 374 "ldgram.y" /* yacc.c:1661  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2707 "ldgram.c" /* yacc.c:1661  */
    break;

  case 96:
#line 377 "ldgram.y" /* yacc.c:1661  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2714 "ldgram.c" /* yacc.c:1661  */
    break;

  case 97:
#line 380 "ldgram.y" /* yacc.c:1661  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2721 "ldgram.c" /* yacc.c:1661  */
    break;

  case 98:
#line 383 "ldgram.y" /* yacc.c:1661  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2728 "ldgram.c" /* yacc.c:1661  */
    break;

  case 99:
#line 386 "ldgram.y" /* yacc.c:1661  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2735 "ldgram.c" /* yacc.c:1661  */
    break;

  case 100:
#line 389 "ldgram.y" /* yacc.c:1661  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2742 "ldgram.c" /* yacc.c:1661  */
    break;

  case 101:
#line 392 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
#line 2749 "ldgram.c" /* yacc.c:1661  */
    break;

  case 102:
#line 395 "ldgram.y" /* yacc.c:1661  */
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 2755 "ldgram.c" /* yacc.c:1661  */
    break;

  case 103:
#line 397 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
#line 2762 "ldgram.c" /* yacc.c:1661  */
    break;

  case 104:
#line 400 "ldgram.y" /* yacc.c:1661  */
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 2768 "ldgram.c" /* yacc.c:1661  */
    break;

  case 105:
#line 402 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
#line 2775 "ldgram.c" /* yacc.c:1661  */
    break;

  case 106:
#line 405 "ldgram.y" /* yacc.c:1661  */
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 2781 "ldgram.c" /* yacc.c:1661  */
    break;

  case 111:
#line 420 "ldgram.y" /* yacc.c:1661  */
    { lang_add_entry ((yyvsp[-1].name), FALSE); }
#line 2787 "ldgram.c" /* yacc.c:1661  */
    break;

  case 113:
#line 422 "ldgram.y" /* yacc.c:1661  */
    {ldlex_expression ();}
#line 2793 "ldgram.c" /* yacc.c:1661  */
    break;

  case 114:
#line 423 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name))); }
#line 2800 "ldgram.c" /* yacc.c:1661  */
    break;

  case 115:
#line 431 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.cname) = (yyvsp[0].name);
			}
#line 2808 "ldgram.c" /* yacc.c:1661  */
    break;

  case 116:
#line 435 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.cname) = "*";
			}
#line 2816 "ldgram.c" /* yacc.c:1661  */
    break;

  case 117:
#line 439 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.cname) = "?";
			}
#line 2824 "ldgram.c" /* yacc.c:1661  */
    break;

  case 118:
#line 446 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2835 "ldgram.c" /* yacc.c:1661  */
    break;

  case 119:
#line 453 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-2].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2846 "ldgram.c" /* yacc.c:1661  */
    break;

  case 120:
#line 460 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2857 "ldgram.c" /* yacc.c:1661  */
    break;

  case 121:
#line 467 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2868 "ldgram.c" /* yacc.c:1661  */
    break;

  case 122:
#line 474 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2879 "ldgram.c" /* yacc.c:1661  */
    break;

  case 123:
#line 481 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_name_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2890 "ldgram.c" /* yacc.c:1661  */
    break;

  case 124:
#line 488 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2901 "ldgram.c" /* yacc.c:1661  */
    break;

  case 125:
#line 495 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_alignment_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2912 "ldgram.c" /* yacc.c:1661  */
    break;

  case 126:
#line 502 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2923 "ldgram.c" /* yacc.c:1661  */
    break;

  case 127:
#line 509 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-3].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2934 "ldgram.c" /* yacc.c:1661  */
    break;

  case 128:
#line 516 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_init_priority;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2945 "ldgram.c" /* yacc.c:1661  */
    break;

  case 129:
#line 525 "ldgram.y" /* yacc.c:1661  */
    {
			  struct flag_info_list *n;
			  n = ((struct flag_info_list *) xmalloc (sizeof *n));
			  if ((yyvsp[0].name)[0] == '!')
			    {
			      n->with = without_flags;
			      n->name = &(yyvsp[0].name)[1];
			    }
			  else
			    {
			      n->with = with_flags;
			      n->name = (yyvsp[0].name);
			    }
			  n->valid = FALSE;
			  n->next = NULL;
			  (yyval.flag_info_list) = n;
			}
#line 2967 "ldgram.c" /* yacc.c:1661  */
    break;

  case 130:
#line 543 "ldgram.y" /* yacc.c:1661  */
    {
			  struct flag_info_list *n;
			  n = ((struct flag_info_list *) xmalloc (sizeof *n));
			  if ((yyvsp[0].name)[0] == '!')
			    {
			      n->with = without_flags;
			      n->name = &(yyvsp[0].name)[1];
			    }
			  else
			    {
			      n->with = with_flags;
			      n->name = (yyvsp[0].name);
			    }
			  n->valid = FALSE;
			  n->next = (yyvsp[-2].flag_info_list);
			  (yyval.flag_info_list) = n;
			}
#line 2989 "ldgram.c" /* yacc.c:1661  */
    break;

  case 131:
#line 564 "ldgram.y" /* yacc.c:1661  */
    {
			  struct flag_info *n;
			  n = ((struct flag_info *) xmalloc (sizeof *n));
			  n->flag_list = (yyvsp[-1].flag_info_list);
			  n->flags_initialized = FALSE;
			  n->not_with_flags = 0;
			  n->only_with_flags = 0;
			  (yyval.flag_info) = n;
			}
#line 3003 "ldgram.c" /* yacc.c:1661  */
    break;

  case 132:
#line 577 "ldgram.y" /* yacc.c:1661  */
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = (yyvsp[-1].name_list);
			  (yyval.name_list) = tmp;
			}
#line 3015 "ldgram.c" /* yacc.c:1661  */
    break;

  case 133:
#line 586 "ldgram.y" /* yacc.c:1661  */
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
#line 3027 "ldgram.c" /* yacc.c:1661  */
    break;

  case 134:
#line 597 "ldgram.y" /* yacc.c:1661  */
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[-2].wildcard_list);
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3039 "ldgram.c" /* yacc.c:1661  */
    break;

  case 135:
#line 606 "ldgram.y" /* yacc.c:1661  */
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3051 "ldgram.c" /* yacc.c:1661  */
    break;

  case 136:
#line 617 "ldgram.y" /* yacc.c:1661  */
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = NULL;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3064 "ldgram.c" /* yacc.c:1661  */
    break;

  case 137:
#line 626 "ldgram.y" /* yacc.c:1661  */
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-1].flag_info);
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3077 "ldgram.c" /* yacc.c:1661  */
    break;

  case 138:
#line 635 "ldgram.y" /* yacc.c:1661  */
    {
			  lang_add_wild (NULL, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3085 "ldgram.c" /* yacc.c:1661  */
    break;

  case 139:
#line 639 "ldgram.y" /* yacc.c:1661  */
    {
			  struct wildcard_spec tmp;
			  tmp.name = NULL;
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-3].flag_info);
			  lang_add_wild (&tmp, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3098 "ldgram.c" /* yacc.c:1661  */
    break;

  case 140:
#line 648 "ldgram.y" /* yacc.c:1661  */
    {
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3106 "ldgram.c" /* yacc.c:1661  */
    break;

  case 141:
#line 652 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyvsp[-3].wildcard).section_flag_list = (yyvsp[-4].flag_info);
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3115 "ldgram.c" /* yacc.c:1661  */
    break;

  case 143:
#line 661 "ldgram.y" /* yacc.c:1661  */
    { ldgram_had_keep = TRUE; }
#line 3121 "ldgram.c" /* yacc.c:1661  */
    break;

  case 144:
#line 663 "ldgram.y" /* yacc.c:1661  */
    { ldgram_had_keep = FALSE; }
#line 3127 "ldgram.c" /* yacc.c:1661  */
    break;

  case 146:
#line 669 "ldgram.y" /* yacc.c:1661  */
    {
 		lang_add_attribute(lang_object_symbols_statement_enum);
	      	}
#line 3135 "ldgram.c" /* yacc.c:1661  */
    break;

  case 148:
#line 674 "ldgram.y" /* yacc.c:1661  */
    {

		  lang_add_attribute(lang_constructors_statement_enum);
		}
#line 3144 "ldgram.c" /* yacc.c:1661  */
    break;

  case 149:
#line 679 "ldgram.y" /* yacc.c:1661  */
    {
		  constructors_sorted = TRUE;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 3153 "ldgram.c" /* yacc.c:1661  */
    break;

  case 151:
#line 685 "ldgram.y" /* yacc.c:1661  */
    {
			  lang_add_data ((int) (yyvsp[-3].integer), (yyvsp[-1].etree));
			}
#line 3161 "ldgram.c" /* yacc.c:1661  */
    break;

  case 152:
#line 690 "ldgram.y" /* yacc.c:1661  */
    {
			  lang_add_fill ((yyvsp[-1].fill));
			}
#line 3169 "ldgram.c" /* yacc.c:1661  */
    break;

  case 153:
#line 693 "ldgram.y" /* yacc.c:1661  */
    {ldlex_expression ();}
#line 3175 "ldgram.c" /* yacc.c:1661  */
    break;

  case 154:
#line 694 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate ();
			  lang_add_assignment (exp_assert ((yyvsp[-4].etree), (yyvsp[-2].name))); }
#line 3182 "ldgram.c" /* yacc.c:1661  */
    break;

  case 155:
#line 697 "ldgram.y" /* yacc.c:1661  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 3188 "ldgram.c" /* yacc.c:1661  */
    break;

  case 156:
#line 699 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); }
#line 3194 "ldgram.c" /* yacc.c:1661  */
    break;

  case 161:
#line 714 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3200 "ldgram.c" /* yacc.c:1661  */
    break;

  case 162:
#line 716 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3206 "ldgram.c" /* yacc.c:1661  */
    break;

  case 163:
#line 718 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3212 "ldgram.c" /* yacc.c:1661  */
    break;

  case 164:
#line 720 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3218 "ldgram.c" /* yacc.c:1661  */
    break;

  case 165:
#line 722 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3224 "ldgram.c" /* yacc.c:1661  */
    break;

  case 166:
#line 727 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.fill) = exp_get_fill ((yyvsp[0].etree), 0, "fill value");
		}
#line 3232 "ldgram.c" /* yacc.c:1661  */
    break;

  case 167:
#line 734 "ldgram.y" /* yacc.c:1661  */
    { (yyval.fill) = (yyvsp[0].fill); }
#line 3238 "ldgram.c" /* yacc.c:1661  */
    break;

  case 168:
#line 735 "ldgram.y" /* yacc.c:1661  */
    { (yyval.fill) = (fill_type *) 0; }
#line 3244 "ldgram.c" /* yacc.c:1661  */
    break;

  case 169:
#line 740 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = '+'; }
#line 3250 "ldgram.c" /* yacc.c:1661  */
    break;

  case 170:
#line 742 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = '-'; }
#line 3256 "ldgram.c" /* yacc.c:1661  */
    break;

  case 171:
#line 744 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = '*'; }
#line 3262 "ldgram.c" /* yacc.c:1661  */
    break;

  case 172:
#line 746 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = '/'; }
#line 3268 "ldgram.c" /* yacc.c:1661  */
    break;

  case 173:
#line 748 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = LSHIFT; }
#line 3274 "ldgram.c" /* yacc.c:1661  */
    break;

  case 174:
#line 750 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = RSHIFT; }
#line 3280 "ldgram.c" /* yacc.c:1661  */
    break;

  case 175:
#line 752 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = '&'; }
#line 3286 "ldgram.c" /* yacc.c:1661  */
    break;

  case 176:
#line 754 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = '|'; }
#line 3292 "ldgram.c" /* yacc.c:1661  */
    break;

  case 179:
#line 764 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name), (yyvsp[0].etree), FALSE));
		}
#line 3300 "ldgram.c" /* yacc.c:1661  */
    break;

  case 180:
#line 768 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name),
						   exp_binop ((yyvsp[-1].token),
							      exp_nameop (NAME,
									  (yyvsp[-2].name)),
							      (yyvsp[0].etree)), FALSE));
		}
#line 3312 "ldgram.c" /* yacc.c:1661  */
    break;

  case 181:
#line 776 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_add_assignment (exp_assign ((yyvsp[-3].name), (yyvsp[-1].etree), TRUE));
		}
#line 3320 "ldgram.c" /* yacc.c:1661  */
    break;

  case 182:
#line 780 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), FALSE));
		}
#line 3328 "ldgram.c" /* yacc.c:1661  */
    break;

  case 183:
#line 784 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), TRUE));
		}
#line 3336 "ldgram.c" /* yacc.c:1661  */
    break;

  case 191:
#line 807 "ldgram.y" /* yacc.c:1661  */
    { region = lang_memory_region_lookup ((yyvsp[0].name), TRUE); }
#line 3342 "ldgram.c" /* yacc.c:1661  */
    break;

  case 192:
#line 810 "ldgram.y" /* yacc.c:1661  */
    {}
#line 3348 "ldgram.c" /* yacc.c:1661  */
    break;

  case 193:
#line 812 "ldgram.y" /* yacc.c:1661  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 3354 "ldgram.c" /* yacc.c:1661  */
    break;

  case 194:
#line 814 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); }
#line 3360 "ldgram.c" /* yacc.c:1661  */
    break;

  case 195:
#line 819 "ldgram.y" /* yacc.c:1661  */
    {
		  region->origin_exp = (yyvsp[0].etree);
		  region->current = region->origin;
		}
#line 3369 "ldgram.c" /* yacc.c:1661  */
    break;

  case 196:
#line 827 "ldgram.y" /* yacc.c:1661  */
    {
		  region->length_exp = (yyvsp[0].etree);
		}
#line 3377 "ldgram.c" /* yacc.c:1661  */
    break;

  case 197:
#line 834 "ldgram.y" /* yacc.c:1661  */
    { /* dummy action to avoid bison 1.25 error message */ }
#line 3383 "ldgram.c" /* yacc.c:1661  */
    break;

  case 201:
#line 845 "ldgram.y" /* yacc.c:1661  */
    { lang_set_flags (region, (yyvsp[0].name), 0); }
#line 3389 "ldgram.c" /* yacc.c:1661  */
    break;

  case 202:
#line 847 "ldgram.y" /* yacc.c:1661  */
    { lang_set_flags (region, (yyvsp[0].name), 1); }
#line 3395 "ldgram.c" /* yacc.c:1661  */
    break;

  case 203:
#line 852 "ldgram.y" /* yacc.c:1661  */
    { lang_startup((yyvsp[-1].name)); }
#line 3401 "ldgram.c" /* yacc.c:1661  */
    break;

  case 205:
#line 858 "ldgram.y" /* yacc.c:1661  */
    { ldemul_hll((char *)NULL); }
#line 3407 "ldgram.c" /* yacc.c:1661  */
    break;

  case 206:
#line 863 "ldgram.y" /* yacc.c:1661  */
    { ldemul_hll((yyvsp[0].name)); }
#line 3413 "ldgram.c" /* yacc.c:1661  */
    break;

  case 207:
#line 865 "ldgram.y" /* yacc.c:1661  */
    { ldemul_hll((yyvsp[0].name)); }
#line 3419 "ldgram.c" /* yacc.c:1661  */
    break;

  case 209:
#line 873 "ldgram.y" /* yacc.c:1661  */
    { ldemul_syslib((yyvsp[0].name)); }
#line 3425 "ldgram.c" /* yacc.c:1661  */
    break;

  case 211:
#line 879 "ldgram.y" /* yacc.c:1661  */
    { lang_float(TRUE); }
#line 3431 "ldgram.c" /* yacc.c:1661  */
    break;

  case 212:
#line 881 "ldgram.y" /* yacc.c:1661  */
    { lang_float(FALSE); }
#line 3437 "ldgram.c" /* yacc.c:1661  */
    break;

  case 213:
#line 886 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.nocrossref) = NULL;
		}
#line 3445 "ldgram.c" /* yacc.c:1661  */
    break;

  case 214:
#line 890 "ldgram.y" /* yacc.c:1661  */
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-1].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3458 "ldgram.c" /* yacc.c:1661  */
    break;

  case 215:
#line 899 "ldgram.y" /* yacc.c:1661  */
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-2].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3471 "ldgram.c" /* yacc.c:1661  */
    break;

  case 216:
#line 909 "ldgram.y" /* yacc.c:1661  */
    { ldlex_expression (); }
#line 3477 "ldgram.c" /* yacc.c:1661  */
    break;

  case 217:
#line 911 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); (yyval.etree)=(yyvsp[0].etree);}
#line 3483 "ldgram.c" /* yacc.c:1661  */
    break;

  case 218:
#line 916 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop ('-', (yyvsp[0].etree)); }
#line 3489 "ldgram.c" /* yacc.c:1661  */
    break;

  case 219:
#line 918 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3495 "ldgram.c" /* yacc.c:1661  */
    break;

  case 220:
#line 920 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop ((int) (yyvsp[-3].integer),(yyvsp[-1].etree)); }
#line 3501 "ldgram.c" /* yacc.c:1661  */
    break;

  case 221:
#line 922 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop ('!', (yyvsp[0].etree)); }
#line 3507 "ldgram.c" /* yacc.c:1661  */
    break;

  case 222:
#line 924 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[0].etree); }
#line 3513 "ldgram.c" /* yacc.c:1661  */
    break;

  case 223:
#line 926 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop ('~', (yyvsp[0].etree));}
#line 3519 "ldgram.c" /* yacc.c:1661  */
    break;

  case 224:
#line 929 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('*', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3525 "ldgram.c" /* yacc.c:1661  */
    break;

  case 225:
#line 931 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('/', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3531 "ldgram.c" /* yacc.c:1661  */
    break;

  case 226:
#line 933 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('%', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3537 "ldgram.c" /* yacc.c:1661  */
    break;

  case 227:
#line 935 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('+', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3543 "ldgram.c" /* yacc.c:1661  */
    break;

  case 228:
#line 937 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('-' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3549 "ldgram.c" /* yacc.c:1661  */
    break;

  case 229:
#line 939 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3555 "ldgram.c" /* yacc.c:1661  */
    break;

  case 230:
#line 941 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3561 "ldgram.c" /* yacc.c:1661  */
    break;

  case 231:
#line 943 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (EQ , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3567 "ldgram.c" /* yacc.c:1661  */
    break;

  case 232:
#line 945 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (NE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3573 "ldgram.c" /* yacc.c:1661  */
    break;

  case 233:
#line 947 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (LE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3579 "ldgram.c" /* yacc.c:1661  */
    break;

  case 234:
#line 949 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (GE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3585 "ldgram.c" /* yacc.c:1661  */
    break;

  case 235:
#line 951 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('<' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3591 "ldgram.c" /* yacc.c:1661  */
    break;

  case 236:
#line 953 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('>' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3597 "ldgram.c" /* yacc.c:1661  */
    break;

  case 237:
#line 955 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('&' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3603 "ldgram.c" /* yacc.c:1661  */
    break;

  case 238:
#line 957 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('^' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3609 "ldgram.c" /* yacc.c:1661  */
    break;

  case 239:
#line 959 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop ('|' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3615 "ldgram.c" /* yacc.c:1661  */
    break;

  case 240:
#line 961 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_trinop ('?' , (yyvsp[-4].etree), (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3621 "ldgram.c" /* yacc.c:1661  */
    break;

  case 241:
#line 963 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (ANDAND , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3627 "ldgram.c" /* yacc.c:1661  */
    break;

  case 242:
#line 965 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (OROR , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3633 "ldgram.c" /* yacc.c:1661  */
    break;

  case 243:
#line 967 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[-1].name)); }
#line 3639 "ldgram.c" /* yacc.c:1661  */
    break;

  case 244:
#line 969 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_bigintop ((yyvsp[0].bigint).integer, (yyvsp[0].bigint).str); }
#line 3645 "ldgram.c" /* yacc.c:1661  */
    break;

  case 245:
#line 971 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
#line 3651 "ldgram.c" /* yacc.c:1661  */
    break;

  case 246:
#line 974 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (ALIGNOF,(yyvsp[-1].name)); }
#line 3657 "ldgram.c" /* yacc.c:1661  */
    break;

  case 247:
#line 976 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (SIZEOF,(yyvsp[-1].name)); }
#line 3663 "ldgram.c" /* yacc.c:1661  */
    break;

  case 248:
#line 978 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (ADDR,(yyvsp[-1].name)); }
#line 3669 "ldgram.c" /* yacc.c:1661  */
    break;

  case 249:
#line 980 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (LOADADDR,(yyvsp[-1].name)); }
#line 3675 "ldgram.c" /* yacc.c:1661  */
    break;

  case 250:
#line 982 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[-1].name)); }
#line 3681 "ldgram.c" /* yacc.c:1661  */
    break;

  case 251:
#line 984 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[-1].etree)); }
#line 3687 "ldgram.c" /* yacc.c:1661  */
    break;

  case 252:
#line 986 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3693 "ldgram.c" /* yacc.c:1661  */
    break;

  case 253:
#line 988 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[-3].etree),(yyvsp[-1].etree)); }
#line 3699 "ldgram.c" /* yacc.c:1661  */
    break;

  case 254:
#line 990 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[-3].etree), (yyvsp[-1].etree)); }
#line 3705 "ldgram.c" /* yacc.c:1661  */
    break;

  case 255:
#line 992 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[-1].etree), (yyvsp[-3].etree)); }
#line 3711 "ldgram.c" /* yacc.c:1661  */
    break;

  case 256:
#line 994 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[-1].etree)); }
#line 3717 "ldgram.c" /* yacc.c:1661  */
    break;

  case 257:
#line 996 "ldgram.y" /* yacc.c:1661  */
    { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[-1].etree),
					  exp_nameop (NAME, (yyvsp[-3].name))); }
#line 3730 "ldgram.c" /* yacc.c:1661  */
    break;

  case 258:
#line 1005 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3736 "ldgram.c" /* yacc.c:1661  */
    break;

  case 259:
#line 1007 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (NAME,(yyvsp[0].name)); }
#line 3742 "ldgram.c" /* yacc.c:1661  */
    break;

  case 260:
#line 1009 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (MAX_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 3748 "ldgram.c" /* yacc.c:1661  */
    break;

  case 261:
#line 1011 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_binop (MIN_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 3754 "ldgram.c" /* yacc.c:1661  */
    break;

  case 262:
#line 1013 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name)); }
#line 3760 "ldgram.c" /* yacc.c:1661  */
    break;

  case 263:
#line 1015 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[-1].name)); }
#line 3766 "ldgram.c" /* yacc.c:1661  */
    break;

  case 264:
#line 1017 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[-1].name)); }
#line 3772 "ldgram.c" /* yacc.c:1661  */
    break;

  case 265:
#line 1019 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = exp_unop (LOG2CEIL, (yyvsp[-1].etree)); }
#line 3778 "ldgram.c" /* yacc.c:1661  */
    break;

  case 266:
#line 1024 "ldgram.y" /* yacc.c:1661  */
    { (yyval.name) = (yyvsp[0].name); }
#line 3784 "ldgram.c" /* yacc.c:1661  */
    break;

  case 267:
#line 1025 "ldgram.y" /* yacc.c:1661  */
    { (yyval.name) = 0; }
#line 3790 "ldgram.c" /* yacc.c:1661  */
    break;

  case 268:
#line 1029 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3796 "ldgram.c" /* yacc.c:1661  */
    break;

  case 269:
#line 1030 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = 0; }
#line 3802 "ldgram.c" /* yacc.c:1661  */
    break;

  case 270:
#line 1034 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3808 "ldgram.c" /* yacc.c:1661  */
    break;

  case 271:
#line 1035 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = 0; }
#line 3814 "ldgram.c" /* yacc.c:1661  */
    break;

  case 272:
#line 1039 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = ALIGN_WITH_INPUT; }
#line 3820 "ldgram.c" /* yacc.c:1661  */
    break;

  case 273:
#line 1040 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = 0; }
#line 3826 "ldgram.c" /* yacc.c:1661  */
    break;

  case 274:
#line 1044 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3832 "ldgram.c" /* yacc.c:1661  */
    break;

  case 275:
#line 1045 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = 0; }
#line 3838 "ldgram.c" /* yacc.c:1661  */
    break;

  case 276:
#line 1049 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = ONLY_IF_RO; }
#line 3844 "ldgram.c" /* yacc.c:1661  */
    break;

  case 277:
#line 1050 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = ONLY_IF_RW; }
#line 3850 "ldgram.c" /* yacc.c:1661  */
    break;

  case 278:
#line 1051 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = SPECIAL; }
#line 3856 "ldgram.c" /* yacc.c:1661  */
    break;

  case 279:
#line 1052 "ldgram.y" /* yacc.c:1661  */
    { (yyval.token) = 0; }
#line 3862 "ldgram.c" /* yacc.c:1661  */
    break;

  case 280:
#line 1055 "ldgram.y" /* yacc.c:1661  */
    { ldlex_expression(); }
#line 3868 "ldgram.c" /* yacc.c:1661  */
    break;

  case 281:
#line 1060 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); ldlex_script (); }
#line 3874 "ldgram.c" /* yacc.c:1661  */
    break;

  case 282:
#line 1063 "ldgram.y" /* yacc.c:1661  */
    {
			  lang_enter_output_section_statement((yyvsp[-9].name), (yyvsp[-7].etree),
							      sectype,
							      (yyvsp[-5].etree), (yyvsp[-3].etree), (yyvsp[-6].etree), (yyvsp[-1].token), (yyvsp[-4].token));
			}
#line 3884 "ldgram.c" /* yacc.c:1661  */
    break;

  case 283:
#line 1069 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); ldlex_expression (); }
#line 3890 "ldgram.c" /* yacc.c:1661  */
    break;

  case 284:
#line 1071 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_popstate ();
		  lang_leave_output_section_statement ((yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
		}
#line 3899 "ldgram.c" /* yacc.c:1661  */
    break;

  case 285:
#line 1076 "ldgram.y" /* yacc.c:1661  */
    {}
#line 3905 "ldgram.c" /* yacc.c:1661  */
    break;

  case 286:
#line 1078 "ldgram.y" /* yacc.c:1661  */
    { ldlex_expression (); }
#line 3911 "ldgram.c" /* yacc.c:1661  */
    break;

  case 287:
#line 1080 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); ldlex_script (); }
#line 3917 "ldgram.c" /* yacc.c:1661  */
    break;

  case 288:
#line 1082 "ldgram.y" /* yacc.c:1661  */
    {
			  lang_enter_overlay ((yyvsp[-5].etree), (yyvsp[-2].etree));
			}
#line 3925 "ldgram.c" /* yacc.c:1661  */
    break;

  case 289:
#line 1087 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); ldlex_expression (); }
#line 3931 "ldgram.c" /* yacc.c:1661  */
    break;

  case 290:
#line 1089 "ldgram.y" /* yacc.c:1661  */
    {
			  ldlex_popstate ();
			  lang_leave_overlay ((yyvsp[-11].etree), (int) (yyvsp[-12].integer),
					      (yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
			}
#line 3941 "ldgram.c" /* yacc.c:1661  */
    break;

  case 292:
#line 1099 "ldgram.y" /* yacc.c:1661  */
    { ldlex_expression (); }
#line 3947 "ldgram.c" /* yacc.c:1661  */
    break;

  case 293:
#line 1101 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assign (".", (yyvsp[0].etree), FALSE));
		}
#line 3956 "ldgram.c" /* yacc.c:1661  */
    break;

  case 295:
#line 1107 "ldgram.y" /* yacc.c:1661  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 3962 "ldgram.c" /* yacc.c:1661  */
    break;

  case 296:
#line 1109 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); }
#line 3968 "ldgram.c" /* yacc.c:1661  */
    break;

  case 297:
#line 1113 "ldgram.y" /* yacc.c:1661  */
    { sectype = noload_section; }
#line 3974 "ldgram.c" /* yacc.c:1661  */
    break;

  case 298:
#line 1114 "ldgram.y" /* yacc.c:1661  */
    { sectype = noalloc_section; }
#line 3980 "ldgram.c" /* yacc.c:1661  */
    break;

  case 299:
#line 1115 "ldgram.y" /* yacc.c:1661  */
    { sectype = noalloc_section; }
#line 3986 "ldgram.c" /* yacc.c:1661  */
    break;

  case 300:
#line 1116 "ldgram.y" /* yacc.c:1661  */
    { sectype = noalloc_section; }
#line 3992 "ldgram.c" /* yacc.c:1661  */
    break;

  case 301:
#line 1117 "ldgram.y" /* yacc.c:1661  */
    { sectype = noalloc_section; }
#line 3998 "ldgram.c" /* yacc.c:1661  */
    break;

  case 303:
#line 1122 "ldgram.y" /* yacc.c:1661  */
    { sectype = normal_section; }
#line 4004 "ldgram.c" /* yacc.c:1661  */
    break;

  case 304:
#line 1123 "ldgram.y" /* yacc.c:1661  */
    { sectype = normal_section; }
#line 4010 "ldgram.c" /* yacc.c:1661  */
    break;

  case 305:
#line 1127 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[-2].etree); }
#line 4016 "ldgram.c" /* yacc.c:1661  */
    break;

  case 306:
#line 1128 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (etree_type *)NULL;  }
#line 4022 "ldgram.c" /* yacc.c:1661  */
    break;

  case 307:
#line 1133 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[-3].etree); }
#line 4028 "ldgram.c" /* yacc.c:1661  */
    break;

  case 308:
#line 1135 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[-7].etree); }
#line 4034 "ldgram.c" /* yacc.c:1661  */
    break;

  case 309:
#line 1139 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 4040 "ldgram.c" /* yacc.c:1661  */
    break;

  case 310:
#line 1140 "ldgram.y" /* yacc.c:1661  */
    { (yyval.etree) = (etree_type *) NULL;  }
#line 4046 "ldgram.c" /* yacc.c:1661  */
    break;

  case 311:
#line 1145 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = 0; }
#line 4052 "ldgram.c" /* yacc.c:1661  */
    break;

  case 312:
#line 1147 "ldgram.y" /* yacc.c:1661  */
    { (yyval.integer) = 1; }
#line 4058 "ldgram.c" /* yacc.c:1661  */
    break;

  case 313:
#line 1152 "ldgram.y" /* yacc.c:1661  */
    { (yyval.name) = (yyvsp[0].name); }
#line 4064 "ldgram.c" /* yacc.c:1661  */
    break;

  case 314:
#line 1153 "ldgram.y" /* yacc.c:1661  */
    { (yyval.name) = DEFAULT_MEMORY_REGION; }
#line 4070 "ldgram.c" /* yacc.c:1661  */
    break;

  case 315:
#line 1158 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.section_phdr) = NULL;
		}
#line 4078 "ldgram.c" /* yacc.c:1661  */
    break;

  case 316:
#line 1162 "ldgram.y" /* yacc.c:1661  */
    {
		  struct lang_output_section_phdr_list *n;

		  n = ((struct lang_output_section_phdr_list *)
		       xmalloc (sizeof *n));
		  n->name = (yyvsp[0].name);
		  n->used = FALSE;
		  n->next = (yyvsp[-2].section_phdr);
		  (yyval.section_phdr) = n;
		}
#line 4093 "ldgram.c" /* yacc.c:1661  */
    break;

  case 318:
#line 1178 "ldgram.y" /* yacc.c:1661  */
    {
			  ldlex_script ();
			  lang_enter_overlay_section ((yyvsp[0].name));
			}
#line 4102 "ldgram.c" /* yacc.c:1661  */
    break;

  case 319:
#line 1183 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); ldlex_expression (); }
#line 4108 "ldgram.c" /* yacc.c:1661  */
    break;

  case 320:
#line 1185 "ldgram.y" /* yacc.c:1661  */
    {
			  ldlex_popstate ();
			  lang_leave_overlay_section ((yyvsp[0].fill), (yyvsp[-1].section_phdr));
			}
#line 4117 "ldgram.c" /* yacc.c:1661  */
    break;

  case 325:
#line 1202 "ldgram.y" /* yacc.c:1661  */
    { ldlex_expression (); }
#line 4123 "ldgram.c" /* yacc.c:1661  */
    break;

  case 326:
#line 1203 "ldgram.y" /* yacc.c:1661  */
    { ldlex_popstate (); }
#line 4129 "ldgram.c" /* yacc.c:1661  */
    break;

  case 327:
#line 1205 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_new_phdr ((yyvsp[-5].name), (yyvsp[-3].etree), (yyvsp[-2].phdr).filehdr, (yyvsp[-2].phdr).phdrs, (yyvsp[-2].phdr).at,
				 (yyvsp[-2].phdr).flags);
		}
#line 4138 "ldgram.c" /* yacc.c:1661  */
    break;

  case 328:
#line 1213 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.etree) = (yyvsp[0].etree);

		  if ((yyvsp[0].etree)->type.node_class == etree_name
		      && (yyvsp[0].etree)->type.node_code == NAME)
		    {
		      const char *s;
		      unsigned int i;
		      static const char * const phdr_types[] =
			{
			  "PT_NULL", "PT_LOAD", "PT_DYNAMIC",
			  "PT_INTERP", "PT_NOTE", "PT_SHLIB",
			  "PT_PHDR", "PT_TLS"
			};

		      s = (yyvsp[0].etree)->name.name;
		      for (i = 0;
			   i < sizeof phdr_types / sizeof phdr_types[0];
			   i++)
			if (strcmp (s, phdr_types[i]) == 0)
			  {
			    (yyval.etree) = exp_intop (i);
			    break;
			  }
		      if (i == sizeof phdr_types / sizeof phdr_types[0])
			{
			  if (strcmp (s, "PT_GNU_EH_FRAME") == 0)
			    (yyval.etree) = exp_intop (0x6474e550);
			  else if (strcmp (s, "PT_GNU_STACK") == 0)
			    (yyval.etree) = exp_intop (0x6474e551);
			  else
			    {
			      einfo (_("\
%X%P:%S: unknown phdr type `%s' (try integer literal)\n"),
				     NULL, s);
			      (yyval.etree) = exp_intop (0);
			    }
			}
		    }
		}
#line 4183 "ldgram.c" /* yacc.c:1661  */
    break;

  case 329:
#line 1257 "ldgram.y" /* yacc.c:1661  */
    {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
#line 4191 "ldgram.c" /* yacc.c:1661  */
    break;

  case 330:
#line 1261 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  if (strcmp ((yyvsp[-2].name), "FILEHDR") == 0 && (yyvsp[-1].etree) == NULL)
		    (yyval.phdr).filehdr = TRUE;
		  else if (strcmp ((yyvsp[-2].name), "PHDRS") == 0 && (yyvsp[-1].etree) == NULL)
		    (yyval.phdr).phdrs = TRUE;
		  else if (strcmp ((yyvsp[-2].name), "FLAGS") == 0 && (yyvsp[-1].etree) != NULL)
		    (yyval.phdr).flags = (yyvsp[-1].etree);
		  else
		    einfo (_("%X%P:%S: PHDRS syntax error at `%s'\n"),
			   NULL, (yyvsp[-2].name));
		}
#line 4208 "ldgram.c" /* yacc.c:1661  */
    break;

  case 331:
#line 1274 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  (yyval.phdr).at = (yyvsp[-2].etree);
		}
#line 4217 "ldgram.c" /* yacc.c:1661  */
    break;

  case 332:
#line 1282 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.etree) = NULL;
		}
#line 4225 "ldgram.c" /* yacc.c:1661  */
    break;

  case 333:
#line 1286 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.etree) = (yyvsp[-1].etree);
		}
#line 4233 "ldgram.c" /* yacc.c:1661  */
    break;

  case 334:
#line 1292 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
#line 4242 "ldgram.c" /* yacc.c:1661  */
    break;

  case 335:
#line 1297 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4251 "ldgram.c" /* yacc.c:1661  */
    break;

  case 339:
#line 1314 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_append_dynamic_list ((yyvsp[-1].versyms));
		}
#line 4259 "ldgram.c" /* yacc.c:1661  */
    break;

  case 340:
#line 1322 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
#line 4268 "ldgram.c" /* yacc.c:1661  */
    break;

  case 341:
#line 1327 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4277 "ldgram.c" /* yacc.c:1661  */
    break;

  case 342:
#line 1336 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_version_script ();
		}
#line 4285 "ldgram.c" /* yacc.c:1661  */
    break;

  case 343:
#line 1340 "ldgram.y" /* yacc.c:1661  */
    {
		  ldlex_popstate ();
		}
#line 4293 "ldgram.c" /* yacc.c:1661  */
    break;

  case 346:
#line 1352 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_register_vers_node (NULL, (yyvsp[-2].versnode), NULL);
		}
#line 4301 "ldgram.c" /* yacc.c:1661  */
    break;

  case 347:
#line 1356 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_register_vers_node ((yyvsp[-4].name), (yyvsp[-2].versnode), NULL);
		}
#line 4309 "ldgram.c" /* yacc.c:1661  */
    break;

  case 348:
#line 1360 "ldgram.y" /* yacc.c:1661  */
    {
		  lang_register_vers_node ((yyvsp[-5].name), (yyvsp[-3].versnode), (yyvsp[-1].deflist));
		}
#line 4317 "ldgram.c" /* yacc.c:1661  */
    break;

  case 349:
#line 1367 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[0].name));
		}
#line 4325 "ldgram.c" /* yacc.c:1661  */
    break;

  case 350:
#line 1371 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[-1].deflist), (yyvsp[0].name));
		}
#line 4333 "ldgram.c" /* yacc.c:1661  */
    break;

  case 351:
#line 1378 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
#line 4341 "ldgram.c" /* yacc.c:1661  */
    break;

  case 352:
#line 1382 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4349 "ldgram.c" /* yacc.c:1661  */
    break;

  case 353:
#line 1386 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4357 "ldgram.c" /* yacc.c:1661  */
    break;

  case 354:
#line 1390 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[-1].versyms));
		}
#line 4365 "ldgram.c" /* yacc.c:1661  */
    break;

  case 355:
#line 1394 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-5].versyms), (yyvsp[-1].versyms));
		}
#line 4373 "ldgram.c" /* yacc.c:1661  */
    break;

  case 356:
#line 1401 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, FALSE);
		}
#line 4381 "ldgram.c" /* yacc.c:1661  */
    break;

  case 357:
#line 1405 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, TRUE);
		}
#line 4389 "ldgram.c" /* yacc.c:1661  */
    break;

  case 358:
#line 1409 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, FALSE);
		}
#line 4397 "ldgram.c" /* yacc.c:1661  */
    break;

  case 359:
#line 1413 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, TRUE);
		}
#line 4405 "ldgram.c" /* yacc.c:1661  */
    break;

  case 360:
#line 1417 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4414 "ldgram.c" /* yacc.c:1661  */
    break;

  case 361:
#line 1422 "ldgram.y" /* yacc.c:1661  */
    {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[-2].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[-8].versyms);
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4426 "ldgram.c" /* yacc.c:1661  */
    break;

  case 362:
#line 1430 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4435 "ldgram.c" /* yacc.c:1661  */
    break;

  case 363:
#line 1435 "ldgram.y" /* yacc.c:1661  */
    {
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4444 "ldgram.c" /* yacc.c:1661  */
    break;

  case 364:
#line 1440 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, FALSE);
		}
#line 4452 "ldgram.c" /* yacc.c:1661  */
    break;

  case 365:
#line 1444 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "global", ldgram_vers_current_lang, FALSE);
		}
#line 4460 "ldgram.c" /* yacc.c:1661  */
    break;

  case 366:
#line 1448 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, FALSE);
		}
#line 4468 "ldgram.c" /* yacc.c:1661  */
    break;

  case 367:
#line 1452 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "local", ldgram_vers_current_lang, FALSE);
		}
#line 4476 "ldgram.c" /* yacc.c:1661  */
    break;

  case 368:
#line 1456 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, FALSE);
		}
#line 4484 "ldgram.c" /* yacc.c:1661  */
    break;

  case 369:
#line 1460 "ldgram.y" /* yacc.c:1661  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "extern", ldgram_vers_current_lang, FALSE);
		}
#line 4492 "ldgram.c" /* yacc.c:1661  */
    break;


#line 4496 "ldgram.c" /* yacc.c:1661  */
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
#line 1470 "ldgram.y" /* yacc.c:1906  */

void
yyerror(arg)
     const char *arg;
{
  if (ldfile_assumed_script)
    einfo (_("%P:%s: file format not recognized; treating as linker script\n"),
	   ldlex_filename ());
  if (error_index > 0 && error_index < ERROR_NAME_MAX)
    einfo ("%P%F:%S: %s in %s\n", NULL, arg, error_names[error_index - 1]);
  else
    einfo ("%P%F:%S: %s\n", NULL, arg);
}
