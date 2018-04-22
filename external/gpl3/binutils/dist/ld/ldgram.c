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

static bfd_boolean ldgram_had_keep = FALSE;
static char *ldgram_vers_current_lang = NULL;

#define ERROR_NAME_MAX 20
static char *error_names[ERROR_NAME_MAX];
static int error_index;
#define PUSH_ERROR(x) if (error_index < ERROR_NAME_MAX) error_names[error_index] = x; error_index++;
#define POP_ERROR()   error_index--;

#line 105 "ldgram.c" /* yacc.c:339  */

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
    FORCE_GROUP_ALLOCATION = 304,
    SEGMENT_START = 305,
    INCLUDE = 306,
    MEMORY = 307,
    REGION_ALIAS = 308,
    LD_FEATURE = 309,
    NOLOAD = 310,
    DSECT = 311,
    COPY = 312,
    INFO = 313,
    OVERLAY = 314,
    DEFINED = 315,
    TARGET_K = 316,
    SEARCH_DIR = 317,
    MAP = 318,
    ENTRY = 319,
    NEXT = 320,
    SIZEOF = 321,
    ALIGNOF = 322,
    ADDR = 323,
    LOADADDR = 324,
    MAX_K = 325,
    MIN_K = 326,
    STARTUP = 327,
    HLL = 328,
    SYSLIB = 329,
    FLOAT = 330,
    NOFLOAT = 331,
    NOCROSSREFS = 332,
    NOCROSSREFS_TO = 333,
    ORIGIN = 334,
    FILL = 335,
    LENGTH = 336,
    CREATE_OBJECT_SYMBOLS = 337,
    INPUT = 338,
    GROUP = 339,
    OUTPUT = 340,
    CONSTRUCTORS = 341,
    ALIGNMOD = 342,
    AT = 343,
    SUBALIGN = 344,
    HIDDEN = 345,
    PROVIDE = 346,
    PROVIDE_HIDDEN = 347,
    AS_NEEDED = 348,
    CHIP = 349,
    LIST = 350,
    SECT = 351,
    ABSOLUTE = 352,
    LOAD = 353,
    NEWLINE = 354,
    ENDWORD = 355,
    ORDER = 356,
    NAMEWORD = 357,
    ASSERT_K = 358,
    LOG2CEIL = 359,
    FORMAT = 360,
    PUBLIC = 361,
    DEFSYMEND = 362,
    BASE = 363,
    ALIAS = 364,
    TRUNCATE = 365,
    REL = 366,
    INPUT_SCRIPT = 367,
    INPUT_MRI_SCRIPT = 368,
    INPUT_DEFSYM = 369,
    CASE = 370,
    EXTERN = 371,
    START = 372,
    VERS_TAG = 373,
    VERS_IDENTIFIER = 374,
    GLOBAL = 375,
    LOCAL = 376,
    VERSIONK = 377,
    INPUT_VERSION_SCRIPT = 378,
    KEEP = 379,
    ONLY_IF_RO = 380,
    ONLY_IF_RW = 381,
    SPECIAL = 382,
    INPUT_SECTION_FLAGS = 383,
    ALIGN_WITH_INPUT = 384,
    EXCLUDE_FILE = 385,
    CONSTANT = 386,
    INPUT_DYNAMIC_LIST = 387
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
#define FORCE_GROUP_ALLOCATION 304
#define SEGMENT_START 305
#define INCLUDE 306
#define MEMORY 307
#define REGION_ALIAS 308
#define LD_FEATURE 309
#define NOLOAD 310
#define DSECT 311
#define COPY 312
#define INFO 313
#define OVERLAY 314
#define DEFINED 315
#define TARGET_K 316
#define SEARCH_DIR 317
#define MAP 318
#define ENTRY 319
#define NEXT 320
#define SIZEOF 321
#define ALIGNOF 322
#define ADDR 323
#define LOADADDR 324
#define MAX_K 325
#define MIN_K 326
#define STARTUP 327
#define HLL 328
#define SYSLIB 329
#define FLOAT 330
#define NOFLOAT 331
#define NOCROSSREFS 332
#define NOCROSSREFS_TO 333
#define ORIGIN 334
#define FILL 335
#define LENGTH 336
#define CREATE_OBJECT_SYMBOLS 337
#define INPUT 338
#define GROUP 339
#define OUTPUT 340
#define CONSTRUCTORS 341
#define ALIGNMOD 342
#define AT 343
#define SUBALIGN 344
#define HIDDEN 345
#define PROVIDE 346
#define PROVIDE_HIDDEN 347
#define AS_NEEDED 348
#define CHIP 349
#define LIST 350
#define SECT 351
#define ABSOLUTE 352
#define LOAD 353
#define NEWLINE 354
#define ENDWORD 355
#define ORDER 356
#define NAMEWORD 357
#define ASSERT_K 358
#define LOG2CEIL 359
#define FORMAT 360
#define PUBLIC 361
#define DEFSYMEND 362
#define BASE 363
#define ALIAS 364
#define TRUNCATE 365
#define REL 366
#define INPUT_SCRIPT 367
#define INPUT_MRI_SCRIPT 368
#define INPUT_DEFSYM 369
#define CASE 370
#define EXTERN 371
#define START 372
#define VERS_TAG 373
#define VERS_IDENTIFIER 374
#define GLOBAL 375
#define LOCAL 376
#define VERSIONK 377
#define INPUT_VERSION_SCRIPT 378
#define KEEP 379
#define ONLY_IF_RO 380
#define ONLY_IF_RW 381
#define SPECIAL 382
#define INPUT_SECTION_FLAGS 383
#define ALIGN_WITH_INPUT 384
#define EXCLUDE_FILE 385
#define CONSTANT 386
#define INPUT_DYNAMIC_LIST 387

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

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

#line 440 "ldgram.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_LDGRAM_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 457 "ldgram.c" /* yacc.c:358  */

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
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1960

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  156
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  133
/* YYNRULES -- Number of rules.  */
#define YYNRULES  376
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  814

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   387

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   154,     2,     2,     2,    34,    21,     2,
      37,   151,    32,    30,   149,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   150,
      24,    10,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   152,     2,   153,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,    19,    59,   155,     2,     2,     2,
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
     141,   142,   143,   144,   145,   146,   147,   148
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
     339,   341,   343,   345,   344,   348,   351,   350,   354,   358,
     362,   363,   365,   367,   369,   374,   374,   379,   382,   385,
     388,   391,   394,   398,   397,   403,   402,   408,   407,   415,
     419,   420,   421,   425,   427,   428,   428,   436,   440,   444,
     451,   458,   468,   469,   474,   482,   483,   488,   493,   498,
     503,   508,   513,   518,   525,   543,   564,   577,   586,   597,
     606,   617,   626,   635,   639,   648,   652,   660,   662,   661,
     668,   669,   673,   674,   679,   684,   685,   690,   694,   694,
     698,   697,   704,   705,   708,   710,   714,   716,   718,   720,
     722,   727,   734,   736,   740,   742,   744,   746,   748,   750,
     752,   754,   759,   759,   764,   768,   776,   780,   784,   792,
     792,   796,   799,   799,   802,   803,   808,   807,   813,   812,
     819,   827,   835,   836,   840,   841,   845,   847,   852,   857,
     858,   863,   865,   871,   873,   875,   879,   881,   887,   890,
     899,   910,   910,   916,   918,   920,   922,   924,   926,   929,
     931,   933,   935,   937,   939,   941,   943,   945,   947,   949,
     951,   953,   955,   957,   959,   961,   963,   965,   967,   969,
     971,   974,   976,   978,   980,   982,   984,   986,   988,   990,
     992,   994,   996,  1005,  1007,  1009,  1011,  1013,  1015,  1017,
    1019,  1025,  1026,  1030,  1031,  1035,  1036,  1040,  1041,  1045,
    1046,  1050,  1051,  1052,  1053,  1056,  1061,  1064,  1070,  1072,
    1056,  1079,  1081,  1083,  1088,  1090,  1078,  1100,  1102,  1100,
    1108,  1107,  1114,  1115,  1116,  1117,  1118,  1122,  1123,  1124,
    1128,  1129,  1134,  1135,  1140,  1141,  1146,  1147,  1152,  1154,
    1159,  1162,  1175,  1179,  1184,  1186,  1177,  1194,  1197,  1199,
    1203,  1204,  1203,  1213,  1258,  1261,  1274,  1283,  1286,  1293,
    1293,  1305,  1306,  1310,  1314,  1323,  1323,  1337,  1337,  1347,
    1348,  1352,  1356,  1360,  1367,  1371,  1379,  1382,  1386,  1390,
    1394,  1401,  1405,  1409,  1413,  1418,  1417,  1431,  1430,  1440,
    1444,  1448,  1452,  1456,  1460,  1466,  1468
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
  "FORCE_GROUP_ALLOCATION", "SEGMENT_START", "INCLUDE", "MEMORY",
  "REGION_ALIAS", "LD_FEATURE", "NOLOAD", "DSECT", "COPY", "INFO",
  "OVERLAY", "DEFINED", "TARGET_K", "SEARCH_DIR", "MAP", "ENTRY", "NEXT",
  "SIZEOF", "ALIGNOF", "ADDR", "LOADADDR", "MAX_K", "MIN_K", "STARTUP",
  "HLL", "SYSLIB", "FLOAT", "NOFLOAT", "NOCROSSREFS", "NOCROSSREFS_TO",
  "ORIGIN", "FILL", "LENGTH", "CREATE_OBJECT_SYMBOLS", "INPUT", "GROUP",
  "OUTPUT", "CONSTRUCTORS", "ALIGNMOD", "AT", "SUBALIGN", "HIDDEN",
  "PROVIDE", "PROVIDE_HIDDEN", "AS_NEEDED", "CHIP", "LIST", "SECT",
  "ABSOLUTE", "LOAD", "NEWLINE", "ENDWORD", "ORDER", "NAMEWORD",
  "ASSERT_K", "LOG2CEIL", "FORMAT", "PUBLIC", "DEFSYMEND", "BASE", "ALIAS",
  "TRUNCATE", "REL", "INPUT_SCRIPT", "INPUT_MRI_SCRIPT", "INPUT_DEFSYM",
  "CASE", "EXTERN", "START", "VERS_TAG", "VERS_IDENTIFIER", "GLOBAL",
  "LOCAL", "VERSIONK", "INPUT_VERSION_SCRIPT", "KEEP", "ONLY_IF_RO",
  "ONLY_IF_RW", "SPECIAL", "INPUT_SECTION_FLAGS", "ALIGN_WITH_INPUT",
  "EXCLUDE_FILE", "CONSTANT", "INPUT_DYNAMIC_LIST", "','", "';'", "')'",
  "'['", "']'", "'!'", "'~'", "$accept", "file", "filename", "defsym_expr",
  "$@1", "mri_script_file", "$@2", "mri_script_lines",
  "mri_script_command", "$@3", "ordernamelist", "mri_load_name_list",
  "mri_abs_name_list", "casesymlist", "extern_name_list", "$@4",
  "extern_name_list_body", "script_file", "$@5", "ifile_list", "ifile_p1",
  "$@6", "$@7", "input_list", "$@8", "input_list1", "@9", "@10", "@11",
  "sections", "sec_or_group_p1", "statement_anywhere", "$@12",
  "wildcard_name", "wildcard_maybe_exclude", "filename_spec",
  "section_name_spec", "sect_flag_list", "sect_flags", "exclude_name_list",
  "section_name_list", "input_section_spec_no_keep", "input_section_spec",
  "$@13", "statement", "$@14", "$@15", "statement_list",
  "statement_list_opt", "length", "fill_exp", "fill_opt", "assign_op",
  "end", "assignment", "opt_comma", "memory", "memory_spec_list_opt",
  "memory_spec_list", "memory_spec", "$@16", "$@17", "origin_spec",
  "length_spec", "attributes_opt", "attributes_list", "attributes_string",
  "startup", "high_level_library", "high_level_library_NAME_list",
  "low_level_library", "low_level_library_NAME_list",
  "floating_point_support", "nocrossref_list", "mustbe_exp", "$@18", "exp",
  "memspec_at_opt", "opt_at", "opt_align", "opt_align_with_input",
  "opt_subalign", "sect_constraint", "section", "$@19", "$@20", "$@21",
  "$@22", "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "$@30",
  "$@31", "type", "atype", "opt_exp_with_type", "opt_exp_without_type",
  "opt_nocrossrefs", "memspec_opt", "phdr_opt", "overlay_section", "$@32",
  "$@33", "$@34", "phdrs", "phdr_list", "phdr", "$@35", "$@36",
  "phdr_type", "phdr_qualifiers", "phdr_val", "dynamic_list_file", "$@37",
  "dynamic_list_nodes", "dynamic_list_node", "dynamic_list_tag",
  "version_script_file", "$@38", "version", "$@39", "vers_nodes",
  "vers_node", "verdep", "vers_tag", "vers_defns", "@40", "@41",
  "opt_semicolon", YY_NULLPTR
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
     379,   380,   381,   382,   383,   384,   385,   386,   387,    44,
      59,    41,    91,    93,    33,   126
};
# endif

#define YYPACT_NINF -655

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-655)))

#define YYTABLE_NINF -348

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     218,  -655,  -655,  -655,  -655,  -655,    76,  -655,  -655,  -655,
    -655,  -655,    84,  -655,   -10,  -655,    65,  -655,   931,  1688,
     120,   101,    98,   -10,  -655,   112,    65,  -655,   557,   106,
     170,   151,   198,  -655,   205,  -655,  -655,   249,   202,   227,
     270,   271,   275,   276,   277,   279,   280,   282,  -655,  -655,
     284,   285,   298,  -655,   299,   300,   312,   313,  -655,   315,
    -655,  -655,  -655,  -655,    96,  -655,  -655,  -655,  -655,  -655,
    -655,  -655,   182,  -655,   350,   249,   352,   776,  -655,   354,
     355,   357,  -655,  -655,   361,   363,   364,   776,   365,   370,
     371,  -655,   372,   259,   776,  -655,   382,  -655,   373,   376,
     329,   244,   101,  -655,  -655,  -655,   336,   246,  -655,  -655,
    -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,
    -655,   394,   398,   399,   400,  -655,  -655,    31,   402,   403,
     410,   249,   249,   411,   249,    23,  -655,   424,   424,  -655,
     392,   249,   426,   427,   429,   397,  -655,  -655,  -655,  -655,
     378,    29,  -655,    42,  -655,  -655,   776,   776,   776,   404,
     405,   418,   419,   425,  -655,   432,   433,   434,   435,   441,
     442,   443,   444,   445,   447,   448,   449,   459,   460,   461,
     776,   776,   887,   407,  -655,   290,  -655,   291,     8,  -655,
    -655,   521,  1904,   308,  -655,  -655,   316,  -655,   457,  -655,
    -655,  1904,   389,   112,   112,   349,   167,   446,   356,   167,
    -655,   776,  -655,   368,    32,  -655,  -655,   -92,   359,  -655,
    -655,   249,   452,    -5,  -655,   317,   362,   379,   381,   386,
     387,   388,  -655,  -655,   -52,    -4,    36,   390,   393,   395,
      40,  -655,   406,   490,   504,   505,   776,   431,   -10,   776,
     776,  -655,   776,   776,  -655,  -655,   939,   776,   776,   776,
     776,   776,   512,   515,   776,   518,   525,   529,   536,   776,
     776,   539,   541,   776,   776,   776,   544,  -655,  -655,   776,
     776,   776,   776,   776,   776,   776,   776,   776,   776,   776,
     776,   776,   776,   776,   776,   776,   776,   776,   776,   776,
     776,  1904,   546,   549,  -655,   551,   776,   776,  1904,   254,
     572,  -655,    45,  -655,   430,   438,  -655,  -655,   573,  -655,
    -655,  -655,   -76,  -655,  1904,   557,  -655,   249,  -655,  -655,
    -655,  -655,  -655,  -655,  -655,   575,  -655,  -655,  1009,   547,
    -655,  -655,  -655,    31,   579,  -655,  -655,  -655,  -655,  -655,
    -655,  -655,   249,  -655,   249,   424,  -655,  -655,  -655,  -655,
    -655,  -655,   548,    16,   450,  -655,  -655,  -655,  -655,  1548,
    -655,    -3,  1904,  1904,  1715,  1904,  1904,  -655,   248,  1143,
    1568,  1588,  1163,   451,   462,  1183,   463,   464,   466,   469,
    1608,  1628,   470,   471,  1203,  1648,  1223,   472,  1854,  1926,
    1123,   769,  1257,  1391,   684,   684,   351,   351,   351,   351,
     420,   420,   207,   207,  -655,  -655,  -655,  1904,  1904,  1904,
    -655,  -655,  -655,  1904,  1904,  -655,  -655,  -655,  -655,   588,
     112,   191,   167,   537,  -655,  -655,   -73,   606,  -655,   691,
     606,   776,   475,  -655,     4,   580,    31,  -655,   477,  -655,
    -655,  -655,  -655,  -655,  -655,   561,    46,  -655,   478,   479,
     480,   607,  -655,  -655,   776,  -655,  -655,   776,   776,  -655,
     776,  -655,  -655,  -655,  -655,  -655,  -655,   776,   776,  -655,
    -655,  -655,   608,  -655,  -655,   776,  -655,   482,   617,  -655,
    -655,  -655,   258,   598,  1741,   626,   543,  -655,  -655,  1884,
     555,  -655,  1904,    34,   645,  -655,   646,     0,  -655,   556,
     616,  -655,    40,  -655,  -655,  -655,   618,  -655,  -655,  -655,
     502,  1243,  1278,  1298,  1318,  1338,  1358,   503,  1904,   167,
     597,   112,   112,  -655,  -655,  -655,  -655,  -655,  -655,   509,
     776,   219,   647,  -655,   624,   627,   428,  -655,  -655,   543,
     604,   630,   632,  -655,   513,  -655,  -655,  -655,   661,   524,
    -655,    19,    40,  -655,  -655,  -655,  -655,  -655,  -655,  -655,
    -655,  -655,  -655,   527,   482,  -655,  1378,  -655,   776,   637,
     533,  -655,   574,  -655,   776,    34,   776,   530,  -655,  -655,
     584,  -655,    28,    40,   167,   625,   111,  1413,   776,  -655,
     574,   648,  -655,   519,  1433,  -655,  1453,  -655,  -655,   673,
    -655,  -655,    37,  -655,   649,   680,  -655,  1473,  -655,   776,
     639,  -655,  -655,    34,  -655,  -655,   776,  -655,  -655,   109,
    1493,  -655,  -655,  -655,  1513,  -655,  -655,  -655,   640,  -655,
    -655,   662,  -655,    68,   686,   836,  -655,  -655,  -655,   412,
    -655,  -655,  -655,  -655,  -655,  -655,  -655,   663,   667,   249,
     668,  -655,  -655,  -655,   669,   683,   687,  -655,    85,  -655,
    -655,   690,    14,  -655,  -655,  -655,   836,   664,   694,    96,
     674,   708,    97,    79,  -655,  -655,   697,  -655,   731,   283,
    -655,   699,   700,   701,   702,  -655,  -655,   -63,    85,   703,
     704,    85,   709,  -655,  -655,  -655,  -655,   836,   741,   643,
     599,   601,   603,   836,   605,  -655,   776,    15,  -655,     1,
    -655,    11,    78,    81,    79,    79,  -655,    85,    83,    79,
     -27,    85,   708,   611,   689,  -655,   724,  -655,  -655,  -655,
    -655,   719,  -655,  1681,   612,   613,   754,  -655,   283,  -655,
     722,   728,   615,   732,   733,   620,   631,   633,  -655,  -655,
    -655,   134,   643,  -655,  -655,   764,    71,  -655,   777,  -655,
    -655,  -655,    79,    79,  -655,    79,    79,  -655,  -655,  -655,
    -655,  -655,  -655,  -655,  -655,   779,  -655,   634,   636,   654,
     657,   658,    71,    71,  -655,  -655,   524,    96,   665,   666,
     670,   671,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,
     524,   524,  -655,  -655
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    59,    10,     8,   345,   339,     0,     2,    62,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    60,    11,
       0,   356,     0,   346,   349,     0,   340,   341,     0,     0,
       0,     0,     0,    79,     0,    81,    80,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   216,   217,
       0,     0,     0,    83,     0,     0,     0,     0,   115,     0,
      72,    61,    64,    70,     0,    63,    66,    67,    68,    69,
      65,    71,     0,    16,     0,     0,     0,     0,    17,     0,
       0,     0,    19,    46,     0,     0,     0,     0,     0,     0,
      51,    54,     0,     0,     0,   362,   373,   361,   369,   371,
       0,     0,   356,   350,   369,   371,     0,     0,   342,   174,
     175,   176,   177,   221,   178,   179,   180,   181,   221,   112,
     328,     0,     0,     0,     0,     7,    86,   193,     0,     0,
       0,     0,     0,     0,     0,     0,   215,   218,   218,    95,
       0,     0,     0,     0,     0,     0,    54,   183,   182,   114,
       0,     0,    40,     0,   249,   264,     0,     0,     0,     0,
       0,     0,     0,     0,   250,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    14,     0,    49,    31,    47,    32,    18,    33,
      23,     0,    36,     0,    37,    52,    38,    39,     0,    42,
      12,     9,     0,     0,     0,     0,   357,     0,     0,   344,
     184,     0,   185,     0,     0,    91,    92,     0,     0,    62,
     196,     0,     0,   190,   195,     0,     0,     0,     0,     0,
       0,     0,   210,   212,   190,   190,   218,     0,     0,     0,
       0,    95,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    13,     0,     0,   227,   223,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   226,   228,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    25,     0,     0,    45,     0,     0,     0,    22,     0,
       0,    56,    55,   367,     0,     0,   351,   364,   374,   363,
     370,   372,     0,   343,   222,   285,   109,     0,   291,   297,
     111,   110,   330,   327,   329,     0,    76,    78,   347,   202,
     198,   191,   189,     0,     0,    94,    73,    74,    85,   113,
     208,   209,     0,   213,     0,   218,   219,    88,    89,    82,
      97,   100,     0,    96,     0,    75,   221,   221,   221,     0,
      90,     0,    27,    28,    43,    29,    30,   224,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   247,
     246,   244,   243,   242,   236,   237,   240,   241,   238,   239,
     234,   235,   232,   233,   229,   230,   231,    15,    26,    24,
      50,    48,    44,    20,    21,    35,    34,    53,    57,     0,
       0,   358,   359,     0,   354,   352,     0,   308,   300,     0,
     308,     0,     0,    87,     0,     0,   193,   194,     0,   211,
     214,   220,   103,    99,   102,     0,     0,    84,     0,     0,
       0,     0,   348,    41,     0,   257,   263,     0,     0,   261,
       0,   248,   225,   252,   251,   253,   254,     0,     0,   268,
     269,   256,     0,   270,   255,     0,    58,   375,   372,   365,
     355,   353,     0,     0,   308,     0,   274,   112,   315,     0,
     316,   298,   333,   334,     0,   206,     0,     0,   204,     0,
       0,    93,     0,   107,    98,   101,     0,   186,   187,   188,
       0,     0,     0,     0,     0,     0,     0,     0,   245,   376,
       0,     0,     0,   302,   303,   304,   305,   306,   309,     0,
       0,     0,     0,   311,     0,   276,     0,   314,   317,   274,
       0,   337,     0,   331,     0,   207,   203,   205,     0,   190,
     199,     0,     0,   105,   116,   258,   259,   260,   262,   265,
     266,   267,   368,     0,   375,   307,     0,   310,     0,     0,
     278,   301,   280,   112,     0,   334,     0,     0,    77,   221,
       0,   104,     0,     0,   360,     0,   308,     0,     0,   277,
     280,     0,   292,     0,     0,   335,     0,   332,   200,     0,
     197,   108,     0,   366,     0,     0,   273,     0,   286,     0,
       0,   299,   338,   334,   221,   106,     0,   312,   275,   284,
       0,   293,   336,   201,     0,   281,   282,   283,     0,   279,
     322,   308,   287,     0,     0,   164,   323,   294,   313,   141,
     119,   118,   166,   167,   168,   169,   170,     0,     0,     0,
       0,   151,   153,   158,     0,     0,     0,   152,     0,   120,
     122,     0,     0,   147,   155,   163,   165,     0,     0,     0,
       0,   319,     0,     0,   160,   221,     0,   148,     0,     0,
     117,     0,     0,     0,     0,   125,   140,   190,     0,   142,
       0,     0,     0,   162,   288,   221,   150,   164,     0,   272,
       0,     0,     0,   164,     0,   171,     0,     0,   134,     0,
     138,     0,     0,     0,     0,     0,   143,     0,   190,     0,
     190,     0,   319,     0,     0,   318,     0,   320,   154,   123,
     124,     0,   157,     0,   117,     0,     0,   136,     0,   137,
       0,     0,     0,     0,     0,     0,     0,     0,   139,   145,
     144,   190,   272,   156,   324,     0,   173,   161,     0,   149,
     135,   121,     0,     0,   126,     0,     0,   127,   128,   133,
     146,   320,   320,   271,   221,     0,   295,     0,     0,     0,
       0,     0,   173,   173,   172,   321,   190,     0,     0,     0,
       0,     0,   289,   325,   296,   159,   130,   129,   131,   132,
     190,   190,   290,   326
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -655,  -655,   -68,  -655,  -655,  -655,  -655,   567,  -655,  -655,
    -655,  -655,  -655,  -655,   677,  -655,  -655,  -655,  -655,   593,
    -655,  -655,  -655,   578,  -655,  -484,  -655,  -655,  -655,  -655,
    -463,   -13,  -655,  -629,  1167,   148,    99,  -655,  -655,  -655,
    -636,   107,  -655,  -655,   149,  -655,  -655,  -655,  -605,  -655,
      47,  -492,  -655,  -654,  -592,  -221,  -655,   384,  -655,   489,
    -655,  -655,  -655,  -655,  -655,  -655,   326,  -655,  -655,  -655,
    -655,  -655,  -655,  -129,  -112,  -655,   -77,    72,   286,  -655,
    -655,   237,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,
    -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -478,   401,
    -655,  -655,   115,  -476,  -655,  -655,  -655,  -655,  -655,  -655,
    -655,  -655,  -655,  -655,  -548,  -655,  -655,  -655,  -655,   813,
    -655,  -655,  -655,  -655,  -655,   595,   -20,  -655,   742,   -14,
    -655,  -655,   274
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,   126,    11,    12,     9,    10,    19,    93,   251,
     188,   187,   185,   196,   197,   198,   312,     7,     8,    18,
      61,   140,   219,   239,   240,   363,   512,   593,   562,    62,
     213,   330,   145,   669,   670,   671,   696,   719,   672,   721,
     697,   673,   674,   717,   675,   686,   713,   676,   677,   678,
     714,   786,   118,   149,    64,   727,    65,   222,   223,   224,
     339,   446,   559,   610,   445,   507,   508,    66,    67,   234,
      68,   235,    69,   237,   715,   211,   256,   737,   545,   580,
     600,   602,   638,   331,   437,   629,   645,   732,   810,   439,
     620,   640,   681,   796,   440,   550,   497,   539,   495,   496,
     500,   549,   709,   766,   643,   680,   782,   811,    70,   214,
     334,   441,   587,   503,   553,   585,    15,    16,    26,    27,
     106,    13,    14,    71,    72,    23,    24,   436,   100,   101,
     532,   430,   530
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     182,   210,   343,   103,   505,    63,   212,   152,   505,   238,
     192,   107,   304,   352,   354,   690,   542,   201,   699,   744,
     453,   454,   746,   453,   454,   706,   650,   125,   561,   650,
     650,  -192,   453,   454,   546,   220,   332,   605,   551,   249,
     236,   453,   454,   651,   360,   361,   651,   651,    21,   428,
     514,   515,   252,   679,  -192,    21,   462,   335,   434,   336,
     720,   490,   728,   228,   229,   730,   231,   233,   700,   700,
     658,   658,   646,   242,   435,   632,    17,   491,   592,   254,
     255,   784,   690,   690,   679,   690,   342,   785,    20,   690,
     726,   333,   749,   650,   650,   761,   650,   342,   221,   351,
     650,   690,   734,   277,   278,    95,   301,   356,   741,   612,
     651,   651,   650,   651,   308,   679,    95,   651,   615,   771,
     603,   679,   342,    25,    22,   455,   760,   647,   455,   651,
      94,    22,   750,   751,   324,   753,   754,   455,   552,   691,
     692,   693,   694,   805,   342,   342,   455,   353,   541,   362,
     614,   556,   747,   340,   506,   516,   102,   305,   506,   665,
     666,   666,   748,   644,   119,   456,   701,   668,   456,   369,
     591,   317,   372,   373,   232,   375,   376,   456,   250,   611,
     378,   379,   380,   381,   382,   355,   456,   385,   625,   314,
     315,   253,   390,   391,   429,   317,   394,   395,   396,   710,
     121,   122,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   414,   415,
     416,   417,   418,   419,   666,   666,   451,   666,   120,   423,
     424,   666,   342,    96,   759,   123,    97,    98,    99,   295,
     296,   297,   124,   666,    96,   147,   148,    97,   104,   105,
     635,   636,   637,   125,   458,   459,   460,   425,   426,   438,
     127,   154,   155,   279,   128,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   342,   449,   780,   450,   690,   156,   157,
     533,   534,   535,   536,   537,   158,   159,   160,   650,   318,
     802,   803,   319,   320,   321,   792,   793,   129,   130,   161,
     162,   163,   131,   132,   133,   651,   134,   135,   164,   136,
     150,   137,   138,   318,   165,    63,   319,   320,   488,   533,
     534,   535,   536,   537,   166,   139,   141,   142,   590,   167,
     168,   169,   170,   171,   172,   173,     1,     2,     3,   143,
     144,   103,   146,   174,   151,   175,   153,     4,   183,   184,
     494,   186,   499,   494,   502,   189,     5,   190,   191,   193,
     538,   176,   325,   194,   200,   195,   199,   177,   178,   291,
     292,   293,   294,   295,   296,   297,   202,   521,   205,   203,
     522,   523,   204,   524,   206,   208,   209,   464,   215,   465,
     525,   526,   216,   217,   218,   179,   225,   226,   528,   538,
     154,   155,   180,   181,   227,   230,   487,   299,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   326,   236,   241,
     243,   244,   325,   245,   246,   327,   248,   156,   157,   302,
     303,   257,   258,   328,   158,   159,   160,   313,    44,  -117,
     293,   294,   295,   296,   297,   259,   260,   309,   161,   162,
     163,   311,   261,   576,   581,   310,   344,   164,   329,   262,
     263,   264,   265,   165,    55,    56,    57,   608,   266,   267,
     268,   269,   270,   166,   271,   272,   273,    58,   167,   168,
     169,   170,   171,   172,   173,   327,   274,   275,   276,   316,
     366,   597,   174,   328,   175,   322,   323,   604,    44,   606,
     337,   341,   633,   345,   367,   368,   383,   573,   574,   384,
     176,   617,   386,   325,   154,   155,   177,   178,   329,   387,
     346,   306,   347,   388,    55,    56,    57,   348,   349,   350,
     389,   357,   630,   392,   358,   393,   359,    58,   397,   634,
     420,   156,   157,   421,   179,   422,   300,   365,   158,   159,
     160,   180,   181,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   161,   162,   163,   804,   427,   433,   621,   442,
     431,   164,   370,   448,   444,   452,   327,   165,   432,   812,
     813,   684,   486,   733,   328,   489,   509,   166,   513,    44,
     470,   457,   167,   168,   169,   170,   171,   172,   173,   154,
     155,   520,   527,   471,   473,   474,   174,   475,   175,   329,
     476,   479,   480,   484,   504,    55,    56,    57,   511,   517,
     518,   519,   529,   531,   176,   540,   156,   157,    58,   743,
     177,   178,   543,   492,   159,   160,   493,   544,   548,   554,
     555,   558,   560,   564,   571,   563,   572,   161,   162,   163,
     575,   578,   583,   577,   588,   579,   164,   584,   179,   586,
     307,   589,   165,   342,   598,   180,   181,   594,   599,   601,
     607,   609,   166,   624,   613,   619,   626,   167,   168,   169,
     170,   171,   172,   173,   154,   155,   627,   631,   642,   541,
     682,   174,   648,   175,   683,   685,   687,   498,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   176,
     688,   156,   157,   704,   689,   177,   178,   698,   158,   159,
     160,   705,   707,   708,   716,   718,   722,   723,   724,   725,
    -117,   729,   161,   162,   163,   735,   731,   736,   764,   765,
     738,   164,   739,   179,   740,   767,   742,   165,   770,   772,
     180,   181,   763,  -141,   769,   773,   774,   166,   783,   775,
     776,   777,   167,   168,   169,   170,   171,   172,   173,   154,
     155,   787,   778,   795,   779,   797,   174,   798,   175,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,   176,   799,   156,   157,   800,   801,
     177,   178,   338,   158,   159,   160,   806,   807,   374,   364,
     702,   808,   809,   247,   745,   703,   758,   161,   162,   163,
     510,   794,   447,   557,   781,   582,   164,   618,   179,   108,
     649,   501,   165,   371,   207,   180,   181,   762,   595,     0,
       0,   650,   166,     0,     0,     0,     0,   167,   168,   169,
     170,   171,   172,   173,     0,     0,     0,     0,   651,     0,
       0,   174,     0,   175,     0,     0,     0,   652,   653,   654,
     655,   656,     0,     0,     0,     0,     0,     0,     0,   176,
     657,     0,   658,     0,     0,   177,   178,     0,     0,     0,
       0,     0,   279,   659,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,     0,   179,     0,     0,     0,     0,     0,     0,
     180,   181,   660,     0,   661,    28,     0,     0,   662,     0,
       0,     0,    55,    56,    57,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   279,   663,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,     0,     0,   664,    29,    30,    31,
     665,     0,   666,     0,     0,     0,   667,     0,   668,     0,
       0,     0,    32,    33,    34,    35,    36,     0,    37,    38,
      39,    40,     0,     0,     0,     0,     0,     0,    41,    42,
      43,    44,     0,    28,     0,     0,     0,     0,     0,    45,
      46,    47,    48,    49,    50,    51,     0,     0,     0,     0,
      52,    53,    54,     0,     0,     0,   298,    55,    56,    57,
       0,     0,     0,     0,     0,   443,     0,     0,     0,     0,
      58,     0,     0,     0,     0,    29,    30,    31,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,     0,  -347,
      32,    33,    34,    35,    36,     0,    37,    38,    39,    40,
       0,    60,     0,     0,     0,     0,    41,    42,    43,    44,
     377,     0,     0,     0,     0,     0,     0,    45,    46,    47,
      48,    49,    50,    51,     0,     0,     0,     0,    52,    53,
      54,     0,     0,     0,     0,    55,    56,    57,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,    60,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,     0,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,     0,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,     0,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,     0,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,     0,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,     0,   279,   466,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   469,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   472,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   481,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   483,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   565,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   297,     0,     0,   279,   566,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,   567,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,   568,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,   569,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,   570,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   279,   596,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   279,   616,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   622,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   623,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   628,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   639,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   641,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    73,     0,     0,     0,   279,   461,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   297,     0,   467,     0,    73,
       0,     0,     0,     0,     0,     0,    74,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   468,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   463,     0,    74,     0,    75,   279,   477,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   297,     0,   478,   541,     0,
       0,     0,    75,     0,     0,     0,     0,     0,     0,     0,
       0,    76,     0,     0,     0,     0,     0,   482,    77,    78,
      79,    80,    81,   -43,    82,    83,    84,     0,     0,    85,
      86,     0,    87,    88,    89,     0,     0,     0,    76,    90,
      91,    92,     0,     0,     0,    77,    78,    79,    80,    81,
     768,    82,    83,    84,     0,   695,    85,    86,     0,    87,
      88,    89,     0,     0,     0,     0,    90,    91,    92,   711,
     712,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   695,     0,     0,   695,   279,
     485,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   752,
     755,   756,   757,     0,   695,     0,   711,     0,   695,   279,
     547,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   279,
       0,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   788,
     789,     0,   790,   791,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297
};

static const yytype_int16 yycheck[] =
{
      77,   113,   223,    23,     4,    18,   118,    75,     4,   138,
      87,    25,     4,   234,   235,     4,   494,    94,     4,     4,
       4,     5,    21,     4,     5,   679,    15,     4,   512,    15,
      15,    36,     4,     5,   497,     4,     4,   585,     4,    10,
       4,     4,     5,    32,     4,     5,    32,    32,    58,     4,
       4,     5,    10,   645,    59,    58,    59,   149,   134,   151,
     689,   134,   698,   131,   132,   701,   134,   135,    54,    54,
      56,    56,     4,   141,   150,   623,     0,   150,   562,   156,
     157,    10,     4,     4,   676,     4,   149,    16,     4,     4,
     153,    59,   721,    15,    15,   731,    15,   149,    67,   151,
      15,     4,   707,   180,   181,     4,   183,   236,   713,   593,
      32,    32,    15,    32,   191,   707,     4,    32,   596,   748,
     583,   713,   149,    58,   134,   109,   153,    59,   109,    32,
      10,   134,    54,    55,   211,    54,    55,   109,   104,    54,
      55,    56,    57,   797,   149,   149,   109,   151,    37,   109,
      39,   151,   151,   221,   154,   109,    58,   149,   154,   144,
     146,   146,   151,   641,    58,   149,   152,   152,   149,   246,
     151,     4,   249,   250,   151,   252,   253,   149,   149,   151,
     257,   258,   259,   260,   261,   149,   149,   264,   151,   203,
     204,   149,   269,   270,   149,     4,   273,   274,   275,   102,
      49,    50,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   146,   146,   355,   146,    58,   306,
     307,   146,   149,   132,   151,    37,   135,   136,   137,    32,
      33,    34,    37,   146,   132,   149,   150,   135,   136,   137,
     141,   142,   143,     4,   366,   367,   368,     3,     4,   327,
      58,     3,     4,    15,    37,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,   149,   352,   151,   354,     4,    30,    31,
      71,    72,    73,    74,    75,    37,    38,    39,    15,   132,
     792,   793,   135,   136,   137,   781,   782,    37,    37,    51,
      52,    53,    37,    37,    37,    32,    37,    37,    60,    37,
     138,    37,    37,   132,    66,   338,   135,   136,   137,    71,
      72,    73,    74,    75,    76,    37,    37,    37,   559,    81,
      82,    83,    84,    85,    86,    87,   128,   129,   130,    37,
      37,   371,    37,    95,     4,    97,     4,   139,     4,     4,
     437,     4,   439,   440,   441,     4,   148,     4,     4,     4,
     151,   113,     4,     3,   115,     4,     4,   119,   120,    28,
      29,    30,    31,    32,    33,    34,     4,   464,    59,    16,
     467,   468,    16,   470,   150,    59,   150,   149,     4,   151,
     477,   478,     4,     4,     4,   147,     4,     4,   485,   151,
       3,     4,   154,   155,     4,     4,   430,    10,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    59,     4,    37,
       4,     4,     4,     4,    37,    67,    58,    30,    31,   149,
     149,    37,    37,    75,    37,    38,    39,    58,    80,    37,
      30,    31,    32,    33,    34,    37,    37,   149,    51,    52,
      53,     4,    37,   540,    36,   149,   149,    60,   100,    37,
      37,    37,    37,    66,   106,   107,   108,   589,    37,    37,
      37,    37,    37,    76,    37,    37,    37,   119,    81,    82,
      83,    84,    85,    86,    87,    67,    37,    37,    37,   150,
      10,   578,    95,    75,    97,    59,   150,   584,    80,   586,
     151,    59,   624,   151,    10,    10,     4,   531,   532,     4,
     113,   598,     4,     4,     3,     4,   119,   120,   100,     4,
     151,    10,   151,     4,   106,   107,   108,   151,   151,   151,
       4,   151,   619,     4,   151,     4,   151,   119,     4,   626,
       4,    30,    31,     4,   147,     4,   149,   151,    37,    38,
      39,   154,   155,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    51,    52,    53,   796,     4,     4,    59,     4,
     150,    60,   151,     4,    37,    37,    67,    66,   150,   810,
     811,   659,     4,   705,    75,    58,    16,    76,    37,    80,
     149,   151,    81,    82,    83,    84,    85,    86,    87,     3,
       4,     4,     4,   151,   151,   151,    95,   151,    97,   100,
     151,   151,   151,   151,   149,   106,   107,   108,   151,   151,
     151,   151,   150,    16,   113,    37,    30,    31,   119,   716,
     119,   120,    16,    37,    38,    39,    40,   104,    93,     4,
       4,    95,    36,   151,   151,    37,    59,    51,    52,    53,
     151,    37,    58,    16,   151,    38,    60,    37,   147,    37,
     149,    10,    66,   149,    37,   154,   155,   150,   145,   105,
     150,    97,    76,    10,    59,    37,    37,    81,    82,    83,
      84,    85,    86,    87,     3,     4,    16,    58,    58,    37,
      37,    95,    16,    97,    37,    37,    37,    16,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,   113,
      37,    30,    31,    59,    37,   119,   120,    37,    37,    38,
      39,    37,    58,    25,    37,     4,    37,    37,    37,    37,
      37,    37,    51,    52,    53,     4,    37,   104,    59,    25,
     151,    60,   151,   147,   151,    36,   151,    66,     4,    37,
     154,   155,   151,   151,   151,    37,   151,    76,     4,    37,
      37,   151,    81,    82,    83,    84,    85,    86,    87,     3,
       4,     4,   151,     4,   151,   151,    95,   151,    97,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,   113,   151,    30,    31,   151,   151,
     119,   120,   219,    37,    38,    39,   151,   151,   251,   241,
     672,   151,   151,   146,   717,   676,   727,    51,    52,    53,
     446,   784,   343,   507,   762,   549,    60,   600,   147,    26,
       4,   440,    66,   248,   102,   154,   155,   732,   574,    -1,
      -1,    15,    76,    -1,    -1,    -1,    -1,    81,    82,    83,
      84,    85,    86,    87,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    95,    -1,    97,    -1,    -1,    -1,    41,    42,    43,
      44,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,
      54,    -1,    56,    -1,    -1,   119,   120,    -1,    -1,    -1,
      -1,    -1,    15,    67,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,   147,    -1,    -1,    -1,    -1,    -1,    -1,
     154,   155,    96,    -1,    98,     4,    -1,    -1,   102,    -1,
      -1,    -1,   106,   107,   108,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    15,   119,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,   140,    46,    47,    48,
     144,    -1,   146,    -1,    -1,    -1,   150,    -1,   152,    -1,
      -1,    -1,    61,    62,    63,    64,    65,    -1,    67,    68,
      69,    70,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      79,    80,    -1,     4,    -1,    -1,    -1,    -1,    -1,    88,
      89,    90,    91,    92,    93,    94,    -1,    -1,    -1,    -1,
      99,   100,   101,    -1,    -1,    -1,   149,   106,   107,   108,
      -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,
     119,    -1,    -1,    -1,    -1,    46,    47,    48,    -1,    -1,
      -1,    -1,    -1,   132,    -1,    -1,    -1,    -1,    -1,   138,
      61,    62,    63,    64,    65,    -1,    67,    68,    69,    70,
      -1,   150,    -1,    -1,    -1,    -1,    77,    78,    79,    80,
     151,    -1,    -1,    -1,    -1,    -1,    -1,    88,    89,    90,
      91,    92,    93,    94,    -1,    -1,    -1,    -1,    99,   100,
     101,    -1,    -1,    -1,    -1,   106,   107,   108,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   132,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   150,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    15,   151,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   151,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   151,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   151,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   151,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   151,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   151,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     4,    -1,    -1,    -1,    15,   149,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,   149,    -1,     4,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   149,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    38,    -1,    67,    15,   149,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,   149,    37,    -1,
      -1,    -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   103,    -1,    -1,    -1,    -1,    -1,   149,   110,   111,
     112,   113,   114,   115,   116,   117,   118,    -1,    -1,   121,
     122,    -1,   124,   125,   126,    -1,    -1,    -1,   103,   131,
     132,   133,    -1,    -1,    -1,   110,   111,   112,   113,   114,
     149,   116,   117,   118,    -1,   668,   121,   122,    -1,   124,
     125,   126,    -1,    -1,    -1,    -1,   131,   132,   133,   682,
     683,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   698,    -1,    -1,   701,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,   722,
     723,   724,   725,    -1,   727,    -1,   729,    -1,   731,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,   772,
     773,    -1,   775,   776,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   128,   129,   130,   139,   148,   157,   173,   174,   161,
     162,   159,   160,   277,   278,   272,   273,     0,   175,   163,
       4,    58,   134,   281,   282,    58,   274,   275,     4,    46,
      47,    48,    61,    62,    63,    64,    65,    67,    68,    69,
      70,    77,    78,    79,    80,    88,    89,    90,    91,    92,
      93,    94,    99,   100,   101,   106,   107,   108,   119,   132,
     150,   176,   185,   187,   210,   212,   223,   224,   226,   228,
     264,   279,   280,     4,    38,    67,   103,   110,   111,   112,
     113,   114,   116,   117,   118,   121,   122,   124,   125,   126,
     131,   132,   133,   164,    10,     4,   132,   135,   136,   137,
     284,   285,    58,   282,   136,   137,   276,   285,   275,     6,
       7,     8,     9,    10,    11,    12,    13,    14,   208,    58,
      58,    49,    50,    37,    37,     4,   158,    58,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
     177,    37,    37,    37,    37,   188,    37,   149,   150,   209,
     138,     4,   158,     4,     3,     4,    30,    31,    37,    38,
      39,    51,    52,    53,    60,    66,    76,    81,    82,    83,
      84,    85,    86,    87,    95,    97,   113,   119,   120,   147,
     154,   155,   232,     4,     4,   168,     4,   167,   166,     4,
       4,     4,   232,     4,     3,     4,   169,   170,   171,     4,
     115,   232,     4,    16,    16,    59,   150,   284,    59,   150,
     230,   231,   230,   186,   265,     4,     4,     4,     4,   178,
       4,    67,   213,   214,   215,     4,     4,     4,   158,   158,
       4,   158,   151,   158,   225,   227,     4,   229,   229,   179,
     180,    37,   158,     4,     4,     4,    37,   170,    58,    10,
     149,   165,    10,   149,   232,   232,   232,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,   232,   232,    15,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,   149,    10,
     149,   232,   149,   149,     4,   149,    10,   149,   232,   149,
     149,     4,   172,    58,   285,   285,   150,     4,   132,   135,
     136,   137,    59,   150,   232,     4,    59,    67,    75,   100,
     187,   239,     4,    59,   266,   149,   151,   151,   175,   216,
     158,    59,   149,   211,   149,   151,   151,   151,   151,   151,
     151,   151,   211,   151,   211,   149,   229,   151,   151,   151,
       4,     5,   109,   181,   179,   151,    10,    10,    10,   232,
     151,   281,   232,   232,   163,   232,   232,   151,   232,   232,
     232,   232,   232,     4,     4,   232,     4,     4,     4,     4,
     232,   232,     4,     4,   232,   232,   232,     4,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
       4,     4,     4,   232,   232,     3,     4,     4,     4,   149,
     287,   150,   150,     4,   134,   150,   283,   240,   158,   245,
     250,   267,     4,    36,    37,   220,   217,   215,     4,   158,
     158,   229,    37,     4,     5,   109,   149,   151,   230,   230,
     230,   149,    59,    36,   149,   151,   151,   149,   149,   151,
     149,   151,   151,   151,   151,   151,   151,   149,   149,   151,
     151,   151,   149,   151,   151,    16,     4,   285,   137,    58,
     134,   150,    37,    40,   232,   254,   255,   252,    16,   232,
     256,   255,   232,   269,   149,     4,   154,   221,   222,    16,
     213,   151,   182,    37,     4,     5,   109,   151,   151,   151,
       4,   232,   232,   232,   232,   232,   232,     4,   232,   150,
     288,    16,   286,    71,    72,    73,    74,    75,   151,   253,
      37,    37,   254,    16,   104,   234,   186,    16,    93,   257,
     251,     4,   104,   270,     4,     4,   151,   222,    95,   218,
      36,   181,   184,    37,   151,   151,   151,   151,   151,   151,
     151,   151,    59,   285,   285,   151,   232,    16,    37,    38,
     235,    36,   234,    58,    37,   271,    37,   268,   151,    10,
     211,   151,   181,   183,   150,   288,   151,   232,    37,   145,
     236,   105,   237,   186,   232,   270,   232,   150,   230,    97,
     219,   151,   181,    59,    39,   254,   151,   232,   237,    37,
     246,    59,   151,   151,    10,   151,    37,    16,   151,   241,
     232,    58,   270,   230,   232,   141,   142,   143,   238,   151,
     247,   151,    58,   260,   254,   242,     4,    59,    16,     4,
      15,    32,    41,    42,    43,    44,    45,    54,    56,    67,
      96,    98,   102,   119,   140,   144,   146,   150,   152,   189,
     190,   191,   194,   197,   198,   200,   203,   204,   205,   210,
     261,   248,    37,    37,   158,    37,   201,    37,    37,    37,
       4,    54,    55,    56,    57,   190,   192,   196,    37,     4,
      54,   152,   191,   200,    59,    37,   209,    58,    25,   258,
     102,   190,   190,   202,   206,   230,    37,   199,     4,   193,
     189,   195,    37,    37,    37,    37,   153,   211,   196,    37,
     196,    37,   243,   230,   204,     4,   104,   233,   151,   151,
     151,   204,   151,   232,     4,   197,    21,   151,   151,   189,
      54,    55,   190,    54,    55,   190,   190,   190,   192,   151,
     153,   196,   258,   151,    59,    25,   259,    36,   149,   151,
       4,   189,    37,    37,   151,    37,    37,   151,   151,   151,
     151,   233,   262,     4,    10,    16,   207,     4,   190,   190,
     190,   190,   259,   259,   206,     4,   249,   151,   151,   151,
     151,   151,   207,   207,   211,   209,   151,   151,   151,   151,
     244,   263,   211,   211
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   156,   157,   157,   157,   157,   157,   158,   160,   159,
     162,   161,   163,   163,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     165,   164,   164,   164,   166,   166,   166,   167,   167,   168,
     168,   169,   169,   169,   171,   170,   172,   172,   172,   174,
     173,   175,   175,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   177,   176,   176,   178,   176,   176,   176,
     176,   176,   176,   176,   176,   180,   179,   181,   181,   181,
     181,   181,   181,   182,   181,   183,   181,   184,   181,   185,
     186,   186,   186,   187,   187,   188,   187,   189,   189,   189,
     190,   190,   191,   191,   191,   192,   192,   192,   192,   192,
     192,   192,   192,   192,   193,   193,   194,   195,   195,   196,
     196,   197,   197,   197,   197,   197,   197,   198,   199,   198,
     200,   200,   200,   200,   200,   200,   200,   200,   201,   200,
     202,   200,   203,   203,   204,   204,   205,   205,   205,   205,
     205,   206,   207,   207,   208,   208,   208,   208,   208,   208,
     208,   208,   209,   209,   210,   210,   210,   210,   210,   211,
     211,   212,   213,   213,   214,   214,   216,   215,   217,   215,
     218,   219,   220,   220,   221,   221,   222,   222,   223,   224,
     224,   225,   225,   226,   227,   227,   228,   228,   229,   229,
     229,   231,   230,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   233,   233,   234,   234,   235,   235,   236,   236,   237,
     237,   238,   238,   238,   238,   240,   241,   242,   243,   244,
     239,   245,   246,   247,   248,   249,   239,   250,   251,   239,
     252,   239,   253,   253,   253,   253,   253,   254,   254,   254,
     255,   255,   255,   255,   256,   256,   257,   257,   258,   258,
     259,   259,   260,   261,   262,   263,   260,   264,   265,   265,
     267,   268,   266,   269,   270,   270,   270,   271,   271,   273,
     272,   274,   274,   275,   276,   278,   277,   280,   279,   281,
     281,   282,   282,   282,   283,   283,   284,   284,   284,   284,
     284,   285,   285,   285,   285,   286,   285,   287,   285,   285,
     285,   285,   285,   285,   285,   288,   288
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
       1,     1,     4,     0,     5,     4,     0,     5,     4,     4,
       4,     3,     3,     6,     4,     0,     2,     1,     3,     2,
       1,     3,     2,     0,     5,     0,     7,     0,     6,     4,
       2,     2,     0,     4,     2,     0,     7,     1,     1,     1,
       1,     5,     1,     4,     4,     1,     4,     4,     4,     7,
       7,     7,     7,     4,     1,     3,     4,     2,     1,     3,
       1,     1,     2,     3,     4,     4,     5,     1,     0,     5,
       2,     1,     1,     1,     4,     1,     4,     4,     0,     8,
       0,     5,     2,     1,     0,     1,     1,     1,     1,     1,
       1,     1,     2,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     6,     6,     6,     1,
       0,     4,     1,     0,     3,     1,     0,     7,     0,     5,
       3,     3,     0,     3,     1,     2,     1,     2,     4,     4,
       3,     3,     1,     4,     3,     0,     1,     1,     0,     2,
       3,     0,     2,     2,     3,     4,     2,     2,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     5,     3,     3,     4,     1,
       1,     4,     4,     4,     4,     4,     4,     4,     6,     6,
       6,     4,     6,     4,     1,     6,     6,     6,     4,     4,
       4,     3,     0,     4,     0,     4,     0,     1,     0,     4,
       0,     1,     1,     1,     0,     0,     0,     0,     0,     0,
      20,     0,     0,     0,     0,     0,    18,     0,     0,     7,
       0,     5,     1,     1,     1,     1,     1,     3,     0,     2,
       3,     2,     6,    10,     2,     1,     0,     1,     2,     0,
       0,     3,     0,     0,     0,     0,    11,     4,     0,     2,
       0,     0,     6,     1,     0,     3,     5,     0,     3,     0,
       2,     1,     2,     4,     2,     0,     2,     0,     5,     1,
       2,     4,     5,     6,     1,     2,     0,     2,     4,     4,
       8,     1,     1,     3,     3,     0,     9,     0,     7,     1,
       3,     1,     3,     1,     3,     0,     1
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
        case 8:
#line 178 "ldgram.y" /* yacc.c:1646  */
    { ldlex_defsym(); }
#line 2374 "ldgram.c" /* yacc.c:1646  */
    break;

  case 9:
#line 180 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate();
		  lang_add_assignment (exp_defsym ((yyvsp[-2].name), (yyvsp[0].etree)));
		}
#line 2383 "ldgram.c" /* yacc.c:1646  */
    break;

  case 10:
#line 188 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
#line 2392 "ldgram.c" /* yacc.c:1646  */
    break;

  case 11:
#line 193 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
#line 2402 "ldgram.c" /* yacc.c:1646  */
    break;

  case 16:
#line 208 "ldgram.y" /* yacc.c:1646  */
    {
			einfo(_("%P%F: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[0].name));
			}
#line 2410 "ldgram.c" /* yacc.c:1646  */
    break;

  case 17:
#line 211 "ldgram.y" /* yacc.c:1646  */
    {
			config.map_filename = "-";
			}
#line 2418 "ldgram.c" /* yacc.c:1646  */
    break;

  case 20:
#line 217 "ldgram.y" /* yacc.c:1646  */
    { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2424 "ldgram.c" /* yacc.c:1646  */
    break;

  case 21:
#line 219 "ldgram.y" /* yacc.c:1646  */
    { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2430 "ldgram.c" /* yacc.c:1646  */
    break;

  case 22:
#line 221 "ldgram.y" /* yacc.c:1646  */
    { mri_public((yyvsp[-1].name), (yyvsp[0].etree)); }
#line 2436 "ldgram.c" /* yacc.c:1646  */
    break;

  case 23:
#line 223 "ldgram.y" /* yacc.c:1646  */
    { mri_format((yyvsp[0].name)); }
#line 2442 "ldgram.c" /* yacc.c:1646  */
    break;

  case 24:
#line 225 "ldgram.y" /* yacc.c:1646  */
    { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2448 "ldgram.c" /* yacc.c:1646  */
    break;

  case 25:
#line 227 "ldgram.y" /* yacc.c:1646  */
    { mri_output_section((yyvsp[-1].name), (yyvsp[0].etree));}
#line 2454 "ldgram.c" /* yacc.c:1646  */
    break;

  case 26:
#line 229 "ldgram.y" /* yacc.c:1646  */
    { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2460 "ldgram.c" /* yacc.c:1646  */
    break;

  case 27:
#line 231 "ldgram.y" /* yacc.c:1646  */
    { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2466 "ldgram.c" /* yacc.c:1646  */
    break;

  case 28:
#line 233 "ldgram.y" /* yacc.c:1646  */
    { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2472 "ldgram.c" /* yacc.c:1646  */
    break;

  case 29:
#line 235 "ldgram.y" /* yacc.c:1646  */
    { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2478 "ldgram.c" /* yacc.c:1646  */
    break;

  case 30:
#line 237 "ldgram.y" /* yacc.c:1646  */
    { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2484 "ldgram.c" /* yacc.c:1646  */
    break;

  case 33:
#line 241 "ldgram.y" /* yacc.c:1646  */
    { mri_name((yyvsp[0].name)); }
#line 2490 "ldgram.c" /* yacc.c:1646  */
    break;

  case 34:
#line 243 "ldgram.y" /* yacc.c:1646  */
    { mri_alias((yyvsp[-2].name),(yyvsp[0].name),0);}
#line 2496 "ldgram.c" /* yacc.c:1646  */
    break;

  case 35:
#line 245 "ldgram.y" /* yacc.c:1646  */
    { mri_alias ((yyvsp[-2].name), 0, (int) (yyvsp[0].bigint).integer); }
#line 2502 "ldgram.c" /* yacc.c:1646  */
    break;

  case 36:
#line 247 "ldgram.y" /* yacc.c:1646  */
    { mri_base((yyvsp[0].etree)); }
#line 2508 "ldgram.c" /* yacc.c:1646  */
    break;

  case 37:
#line 249 "ldgram.y" /* yacc.c:1646  */
    { mri_truncate ((unsigned int) (yyvsp[0].bigint).integer); }
#line 2514 "ldgram.c" /* yacc.c:1646  */
    break;

  case 40:
#line 253 "ldgram.y" /* yacc.c:1646  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 2520 "ldgram.c" /* yacc.c:1646  */
    break;

  case 41:
#line 255 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 2526 "ldgram.c" /* yacc.c:1646  */
    break;

  case 42:
#line 257 "ldgram.y" /* yacc.c:1646  */
    { lang_add_entry ((yyvsp[0].name), FALSE); }
#line 2532 "ldgram.c" /* yacc.c:1646  */
    break;

  case 44:
#line 262 "ldgram.y" /* yacc.c:1646  */
    { mri_order((yyvsp[0].name)); }
#line 2538 "ldgram.c" /* yacc.c:1646  */
    break;

  case 45:
#line 263 "ldgram.y" /* yacc.c:1646  */
    { mri_order((yyvsp[0].name)); }
#line 2544 "ldgram.c" /* yacc.c:1646  */
    break;

  case 47:
#line 269 "ldgram.y" /* yacc.c:1646  */
    { mri_load((yyvsp[0].name)); }
#line 2550 "ldgram.c" /* yacc.c:1646  */
    break;

  case 48:
#line 270 "ldgram.y" /* yacc.c:1646  */
    { mri_load((yyvsp[0].name)); }
#line 2556 "ldgram.c" /* yacc.c:1646  */
    break;

  case 49:
#line 275 "ldgram.y" /* yacc.c:1646  */
    { mri_only_load((yyvsp[0].name)); }
#line 2562 "ldgram.c" /* yacc.c:1646  */
    break;

  case 50:
#line 277 "ldgram.y" /* yacc.c:1646  */
    { mri_only_load((yyvsp[0].name)); }
#line 2568 "ldgram.c" /* yacc.c:1646  */
    break;

  case 51:
#line 281 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = NULL; }
#line 2574 "ldgram.c" /* yacc.c:1646  */
    break;

  case 54:
#line 288 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 2580 "ldgram.c" /* yacc.c:1646  */
    break;

  case 55:
#line 290 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 2586 "ldgram.c" /* yacc.c:1646  */
    break;

  case 56:
#line 294 "ldgram.y" /* yacc.c:1646  */
    { ldlang_add_undef ((yyvsp[0].name), FALSE); }
#line 2592 "ldgram.c" /* yacc.c:1646  */
    break;

  case 57:
#line 296 "ldgram.y" /* yacc.c:1646  */
    { ldlang_add_undef ((yyvsp[0].name), FALSE); }
#line 2598 "ldgram.c" /* yacc.c:1646  */
    break;

  case 58:
#line 298 "ldgram.y" /* yacc.c:1646  */
    { ldlang_add_undef ((yyvsp[0].name), FALSE); }
#line 2604 "ldgram.c" /* yacc.c:1646  */
    break;

  case 59:
#line 302 "ldgram.y" /* yacc.c:1646  */
    { ldlex_both(); }
#line 2610 "ldgram.c" /* yacc.c:1646  */
    break;

  case 60:
#line 304 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate(); }
#line 2616 "ldgram.c" /* yacc.c:1646  */
    break;

  case 73:
#line 325 "ldgram.y" /* yacc.c:1646  */
    { lang_add_target((yyvsp[-1].name)); }
#line 2622 "ldgram.c" /* yacc.c:1646  */
    break;

  case 74:
#line 327 "ldgram.y" /* yacc.c:1646  */
    { ldfile_add_library_path ((yyvsp[-1].name), FALSE); }
#line 2628 "ldgram.c" /* yacc.c:1646  */
    break;

  case 75:
#line 329 "ldgram.y" /* yacc.c:1646  */
    { lang_add_output((yyvsp[-1].name), 1); }
#line 2634 "ldgram.c" /* yacc.c:1646  */
    break;

  case 76:
#line 331 "ldgram.y" /* yacc.c:1646  */
    { lang_add_output_format ((yyvsp[-1].name), (char *) NULL,
					    (char *) NULL, 1); }
#line 2641 "ldgram.c" /* yacc.c:1646  */
    break;

  case 77:
#line 334 "ldgram.y" /* yacc.c:1646  */
    { lang_add_output_format ((yyvsp[-5].name), (yyvsp[-3].name), (yyvsp[-1].name), 1); }
#line 2647 "ldgram.c" /* yacc.c:1646  */
    break;

  case 78:
#line 336 "ldgram.y" /* yacc.c:1646  */
    { ldfile_set_output_arch ((yyvsp[-1].name), bfd_arch_unknown); }
#line 2653 "ldgram.c" /* yacc.c:1646  */
    break;

  case 79:
#line 338 "ldgram.y" /* yacc.c:1646  */
    { command_line.force_common_definition = TRUE ; }
#line 2659 "ldgram.c" /* yacc.c:1646  */
    break;

  case 80:
#line 340 "ldgram.y" /* yacc.c:1646  */
    { command_line.force_group_allocation = TRUE ; }
#line 2665 "ldgram.c" /* yacc.c:1646  */
    break;

  case 81:
#line 342 "ldgram.y" /* yacc.c:1646  */
    { link_info.inhibit_common_definition = TRUE ; }
#line 2671 "ldgram.c" /* yacc.c:1646  */
    break;

  case 83:
#line 345 "ldgram.y" /* yacc.c:1646  */
    { lang_enter_group (); }
#line 2677 "ldgram.c" /* yacc.c:1646  */
    break;

  case 84:
#line 347 "ldgram.y" /* yacc.c:1646  */
    { lang_leave_group (); }
#line 2683 "ldgram.c" /* yacc.c:1646  */
    break;

  case 85:
#line 349 "ldgram.y" /* yacc.c:1646  */
    { lang_add_map((yyvsp[-1].name)); }
#line 2689 "ldgram.c" /* yacc.c:1646  */
    break;

  case 86:
#line 351 "ldgram.y" /* yacc.c:1646  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 2695 "ldgram.c" /* yacc.c:1646  */
    break;

  case 87:
#line 353 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 2701 "ldgram.c" /* yacc.c:1646  */
    break;

  case 88:
#line 355 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_nocrossref ((yyvsp[-1].nocrossref));
		}
#line 2709 "ldgram.c" /* yacc.c:1646  */
    break;

  case 89:
#line 359 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_nocrossref_to ((yyvsp[-1].nocrossref));
		}
#line 2717 "ldgram.c" /* yacc.c:1646  */
    break;

  case 91:
#line 364 "ldgram.y" /* yacc.c:1646  */
    { lang_add_insert ((yyvsp[0].name), 0); }
#line 2723 "ldgram.c" /* yacc.c:1646  */
    break;

  case 92:
#line 366 "ldgram.y" /* yacc.c:1646  */
    { lang_add_insert ((yyvsp[0].name), 1); }
#line 2729 "ldgram.c" /* yacc.c:1646  */
    break;

  case 93:
#line 368 "ldgram.y" /* yacc.c:1646  */
    { lang_memory_region_alias ((yyvsp[-3].name), (yyvsp[-1].name)); }
#line 2735 "ldgram.c" /* yacc.c:1646  */
    break;

  case 94:
#line 370 "ldgram.y" /* yacc.c:1646  */
    { lang_ld_feature ((yyvsp[-1].name)); }
#line 2741 "ldgram.c" /* yacc.c:1646  */
    break;

  case 95:
#line 374 "ldgram.y" /* yacc.c:1646  */
    { ldlex_inputlist(); }
#line 2747 "ldgram.c" /* yacc.c:1646  */
    break;

  case 96:
#line 376 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate(); }
#line 2753 "ldgram.c" /* yacc.c:1646  */
    break;

  case 97:
#line 380 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2760 "ldgram.c" /* yacc.c:1646  */
    break;

  case 98:
#line 383 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2767 "ldgram.c" /* yacc.c:1646  */
    break;

  case 99:
#line 386 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2774 "ldgram.c" /* yacc.c:1646  */
    break;

  case 100:
#line 389 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2781 "ldgram.c" /* yacc.c:1646  */
    break;

  case 101:
#line 392 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2788 "ldgram.c" /* yacc.c:1646  */
    break;

  case 102:
#line 395 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2795 "ldgram.c" /* yacc.c:1646  */
    break;

  case 103:
#line 398 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
#line 2802 "ldgram.c" /* yacc.c:1646  */
    break;

  case 104:
#line 401 "ldgram.y" /* yacc.c:1646  */
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 2808 "ldgram.c" /* yacc.c:1646  */
    break;

  case 105:
#line 403 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
#line 2815 "ldgram.c" /* yacc.c:1646  */
    break;

  case 106:
#line 406 "ldgram.y" /* yacc.c:1646  */
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 2821 "ldgram.c" /* yacc.c:1646  */
    break;

  case 107:
#line 408 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
#line 2828 "ldgram.c" /* yacc.c:1646  */
    break;

  case 108:
#line 411 "ldgram.y" /* yacc.c:1646  */
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 2834 "ldgram.c" /* yacc.c:1646  */
    break;

  case 113:
#line 426 "ldgram.y" /* yacc.c:1646  */
    { lang_add_entry ((yyvsp[-1].name), FALSE); }
#line 2840 "ldgram.c" /* yacc.c:1646  */
    break;

  case 115:
#line 428 "ldgram.y" /* yacc.c:1646  */
    {ldlex_expression ();}
#line 2846 "ldgram.c" /* yacc.c:1646  */
    break;

  case 116:
#line 429 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name))); }
#line 2853 "ldgram.c" /* yacc.c:1646  */
    break;

  case 117:
#line 437 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.cname) = (yyvsp[0].name);
			}
#line 2861 "ldgram.c" /* yacc.c:1646  */
    break;

  case 118:
#line 441 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.cname) = "*";
			}
#line 2869 "ldgram.c" /* yacc.c:1646  */
    break;

  case 119:
#line 445 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.cname) = "?";
			}
#line 2877 "ldgram.c" /* yacc.c:1646  */
    break;

  case 120:
#line 452 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2888 "ldgram.c" /* yacc.c:1646  */
    break;

  case 121:
#line 459 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-2].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 2899 "ldgram.c" /* yacc.c:1646  */
    break;

  case 123:
#line 470 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 2908 "ldgram.c" /* yacc.c:1646  */
    break;

  case 124:
#line 475 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_none;
			}
#line 2917 "ldgram.c" /* yacc.c:1646  */
    break;

  case 126:
#line 484 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 2926 "ldgram.c" /* yacc.c:1646  */
    break;

  case 127:
#line 489 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_alignment;
			}
#line 2935 "ldgram.c" /* yacc.c:1646  */
    break;

  case 128:
#line 494 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_none;
			}
#line 2944 "ldgram.c" /* yacc.c:1646  */
    break;

  case 129:
#line 499 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name_alignment;
			}
#line 2953 "ldgram.c" /* yacc.c:1646  */
    break;

  case 130:
#line 504 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 2962 "ldgram.c" /* yacc.c:1646  */
    break;

  case 131:
#line 509 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_alignment_name;
			}
#line 2971 "ldgram.c" /* yacc.c:1646  */
    break;

  case 132:
#line 514 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_alignment;
			}
#line 2980 "ldgram.c" /* yacc.c:1646  */
    break;

  case 133:
#line 519 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_init_priority;
			}
#line 2989 "ldgram.c" /* yacc.c:1646  */
    break;

  case 134:
#line 526 "ldgram.y" /* yacc.c:1646  */
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
#line 3011 "ldgram.c" /* yacc.c:1646  */
    break;

  case 135:
#line 544 "ldgram.y" /* yacc.c:1646  */
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
#line 3033 "ldgram.c" /* yacc.c:1646  */
    break;

  case 136:
#line 565 "ldgram.y" /* yacc.c:1646  */
    {
			  struct flag_info *n;
			  n = ((struct flag_info *) xmalloc (sizeof *n));
			  n->flag_list = (yyvsp[-1].flag_info_list);
			  n->flags_initialized = FALSE;
			  n->not_with_flags = 0;
			  n->only_with_flags = 0;
			  (yyval.flag_info) = n;
			}
#line 3047 "ldgram.c" /* yacc.c:1646  */
    break;

  case 137:
#line 578 "ldgram.y" /* yacc.c:1646  */
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = (yyvsp[-1].name_list);
			  (yyval.name_list) = tmp;
			}
#line 3059 "ldgram.c" /* yacc.c:1646  */
    break;

  case 138:
#line 587 "ldgram.y" /* yacc.c:1646  */
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
#line 3071 "ldgram.c" /* yacc.c:1646  */
    break;

  case 139:
#line 598 "ldgram.y" /* yacc.c:1646  */
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[-2].wildcard_list);
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3083 "ldgram.c" /* yacc.c:1646  */
    break;

  case 140:
#line 607 "ldgram.y" /* yacc.c:1646  */
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3095 "ldgram.c" /* yacc.c:1646  */
    break;

  case 141:
#line 618 "ldgram.y" /* yacc.c:1646  */
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = NULL;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3108 "ldgram.c" /* yacc.c:1646  */
    break;

  case 142:
#line 627 "ldgram.y" /* yacc.c:1646  */
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-1].flag_info);
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3121 "ldgram.c" /* yacc.c:1646  */
    break;

  case 143:
#line 636 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_add_wild (NULL, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3129 "ldgram.c" /* yacc.c:1646  */
    break;

  case 144:
#line 640 "ldgram.y" /* yacc.c:1646  */
    {
			  struct wildcard_spec tmp;
			  tmp.name = NULL;
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-3].flag_info);
			  lang_add_wild (&tmp, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3142 "ldgram.c" /* yacc.c:1646  */
    break;

  case 145:
#line 649 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3150 "ldgram.c" /* yacc.c:1646  */
    break;

  case 146:
#line 653 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyvsp[-3].wildcard).section_flag_list = (yyvsp[-4].flag_info);
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3159 "ldgram.c" /* yacc.c:1646  */
    break;

  case 148:
#line 662 "ldgram.y" /* yacc.c:1646  */
    { ldgram_had_keep = TRUE; }
#line 3165 "ldgram.c" /* yacc.c:1646  */
    break;

  case 149:
#line 664 "ldgram.y" /* yacc.c:1646  */
    { ldgram_had_keep = FALSE; }
#line 3171 "ldgram.c" /* yacc.c:1646  */
    break;

  case 151:
#line 670 "ldgram.y" /* yacc.c:1646  */
    {
		lang_add_attribute(lang_object_symbols_statement_enum);
		}
#line 3179 "ldgram.c" /* yacc.c:1646  */
    break;

  case 153:
#line 675 "ldgram.y" /* yacc.c:1646  */
    {

		  lang_add_attribute(lang_constructors_statement_enum);
		}
#line 3188 "ldgram.c" /* yacc.c:1646  */
    break;

  case 154:
#line 680 "ldgram.y" /* yacc.c:1646  */
    {
		  constructors_sorted = TRUE;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 3197 "ldgram.c" /* yacc.c:1646  */
    break;

  case 156:
#line 686 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_data ((int) (yyvsp[-3].integer), (yyvsp[-1].etree));
		}
#line 3205 "ldgram.c" /* yacc.c:1646  */
    break;

  case 157:
#line 691 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_fill ((yyvsp[-1].fill));
		}
#line 3213 "ldgram.c" /* yacc.c:1646  */
    break;

  case 158:
#line 694 "ldgram.y" /* yacc.c:1646  */
    {ldlex_expression ();}
#line 3219 "ldgram.c" /* yacc.c:1646  */
    break;

  case 159:
#line 695 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate ();
			  lang_add_assignment (exp_assert ((yyvsp[-4].etree), (yyvsp[-2].name))); }
#line 3226 "ldgram.c" /* yacc.c:1646  */
    break;

  case 160:
#line 698 "ldgram.y" /* yacc.c:1646  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 3232 "ldgram.c" /* yacc.c:1646  */
    break;

  case 161:
#line 700 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 3238 "ldgram.c" /* yacc.c:1646  */
    break;

  case 166:
#line 715 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3244 "ldgram.c" /* yacc.c:1646  */
    break;

  case 167:
#line 717 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3250 "ldgram.c" /* yacc.c:1646  */
    break;

  case 168:
#line 719 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3256 "ldgram.c" /* yacc.c:1646  */
    break;

  case 169:
#line 721 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3262 "ldgram.c" /* yacc.c:1646  */
    break;

  case 170:
#line 723 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 3268 "ldgram.c" /* yacc.c:1646  */
    break;

  case 171:
#line 728 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.fill) = exp_get_fill ((yyvsp[0].etree), 0, "fill value");
		}
#line 3276 "ldgram.c" /* yacc.c:1646  */
    break;

  case 172:
#line 735 "ldgram.y" /* yacc.c:1646  */
    { (yyval.fill) = (yyvsp[0].fill); }
#line 3282 "ldgram.c" /* yacc.c:1646  */
    break;

  case 173:
#line 736 "ldgram.y" /* yacc.c:1646  */
    { (yyval.fill) = (fill_type *) 0; }
#line 3288 "ldgram.c" /* yacc.c:1646  */
    break;

  case 174:
#line 741 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '+'; }
#line 3294 "ldgram.c" /* yacc.c:1646  */
    break;

  case 175:
#line 743 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '-'; }
#line 3300 "ldgram.c" /* yacc.c:1646  */
    break;

  case 176:
#line 745 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '*'; }
#line 3306 "ldgram.c" /* yacc.c:1646  */
    break;

  case 177:
#line 747 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '/'; }
#line 3312 "ldgram.c" /* yacc.c:1646  */
    break;

  case 178:
#line 749 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = LSHIFT; }
#line 3318 "ldgram.c" /* yacc.c:1646  */
    break;

  case 179:
#line 751 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = RSHIFT; }
#line 3324 "ldgram.c" /* yacc.c:1646  */
    break;

  case 180:
#line 753 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '&'; }
#line 3330 "ldgram.c" /* yacc.c:1646  */
    break;

  case 181:
#line 755 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '|'; }
#line 3336 "ldgram.c" /* yacc.c:1646  */
    break;

  case 184:
#line 765 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name), (yyvsp[0].etree), FALSE));
		}
#line 3344 "ldgram.c" /* yacc.c:1646  */
    break;

  case 185:
#line 769 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name),
						   exp_binop ((yyvsp[-1].token),
							      exp_nameop (NAME,
									  (yyvsp[-2].name)),
							      (yyvsp[0].etree)), FALSE));
		}
#line 3356 "ldgram.c" /* yacc.c:1646  */
    break;

  case 186:
#line 777 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_assign ((yyvsp[-3].name), (yyvsp[-1].etree), TRUE));
		}
#line 3364 "ldgram.c" /* yacc.c:1646  */
    break;

  case 187:
#line 781 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), FALSE));
		}
#line 3372 "ldgram.c" /* yacc.c:1646  */
    break;

  case 188:
#line 785 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), TRUE));
		}
#line 3380 "ldgram.c" /* yacc.c:1646  */
    break;

  case 196:
#line 808 "ldgram.y" /* yacc.c:1646  */
    { region = lang_memory_region_lookup ((yyvsp[0].name), TRUE); }
#line 3386 "ldgram.c" /* yacc.c:1646  */
    break;

  case 197:
#line 811 "ldgram.y" /* yacc.c:1646  */
    {}
#line 3392 "ldgram.c" /* yacc.c:1646  */
    break;

  case 198:
#line 813 "ldgram.y" /* yacc.c:1646  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 3398 "ldgram.c" /* yacc.c:1646  */
    break;

  case 199:
#line 815 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 3404 "ldgram.c" /* yacc.c:1646  */
    break;

  case 200:
#line 820 "ldgram.y" /* yacc.c:1646  */
    {
		  region->origin_exp = (yyvsp[0].etree);
		  region->current = region->origin;
		}
#line 3413 "ldgram.c" /* yacc.c:1646  */
    break;

  case 201:
#line 828 "ldgram.y" /* yacc.c:1646  */
    {
		  region->length_exp = (yyvsp[0].etree);
		}
#line 3421 "ldgram.c" /* yacc.c:1646  */
    break;

  case 202:
#line 835 "ldgram.y" /* yacc.c:1646  */
    { /* dummy action to avoid bison 1.25 error message */ }
#line 3427 "ldgram.c" /* yacc.c:1646  */
    break;

  case 206:
#line 846 "ldgram.y" /* yacc.c:1646  */
    { lang_set_flags (region, (yyvsp[0].name), 0); }
#line 3433 "ldgram.c" /* yacc.c:1646  */
    break;

  case 207:
#line 848 "ldgram.y" /* yacc.c:1646  */
    { lang_set_flags (region, (yyvsp[0].name), 1); }
#line 3439 "ldgram.c" /* yacc.c:1646  */
    break;

  case 208:
#line 853 "ldgram.y" /* yacc.c:1646  */
    { lang_startup((yyvsp[-1].name)); }
#line 3445 "ldgram.c" /* yacc.c:1646  */
    break;

  case 210:
#line 859 "ldgram.y" /* yacc.c:1646  */
    { ldemul_hll((char *)NULL); }
#line 3451 "ldgram.c" /* yacc.c:1646  */
    break;

  case 211:
#line 864 "ldgram.y" /* yacc.c:1646  */
    { ldemul_hll((yyvsp[0].name)); }
#line 3457 "ldgram.c" /* yacc.c:1646  */
    break;

  case 212:
#line 866 "ldgram.y" /* yacc.c:1646  */
    { ldemul_hll((yyvsp[0].name)); }
#line 3463 "ldgram.c" /* yacc.c:1646  */
    break;

  case 214:
#line 874 "ldgram.y" /* yacc.c:1646  */
    { ldemul_syslib((yyvsp[0].name)); }
#line 3469 "ldgram.c" /* yacc.c:1646  */
    break;

  case 216:
#line 880 "ldgram.y" /* yacc.c:1646  */
    { lang_float(TRUE); }
#line 3475 "ldgram.c" /* yacc.c:1646  */
    break;

  case 217:
#line 882 "ldgram.y" /* yacc.c:1646  */
    { lang_float(FALSE); }
#line 3481 "ldgram.c" /* yacc.c:1646  */
    break;

  case 218:
#line 887 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.nocrossref) = NULL;
		}
#line 3489 "ldgram.c" /* yacc.c:1646  */
    break;

  case 219:
#line 891 "ldgram.y" /* yacc.c:1646  */
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-1].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3502 "ldgram.c" /* yacc.c:1646  */
    break;

  case 220:
#line 900 "ldgram.y" /* yacc.c:1646  */
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-2].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3515 "ldgram.c" /* yacc.c:1646  */
    break;

  case 221:
#line 910 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 3521 "ldgram.c" /* yacc.c:1646  */
    break;

  case 222:
#line 912 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); (yyval.etree)=(yyvsp[0].etree);}
#line 3527 "ldgram.c" /* yacc.c:1646  */
    break;

  case 223:
#line 917 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop ('-', (yyvsp[0].etree)); }
#line 3533 "ldgram.c" /* yacc.c:1646  */
    break;

  case 224:
#line 919 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3539 "ldgram.c" /* yacc.c:1646  */
    break;

  case 225:
#line 921 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop ((int) (yyvsp[-3].integer),(yyvsp[-1].etree)); }
#line 3545 "ldgram.c" /* yacc.c:1646  */
    break;

  case 226:
#line 923 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop ('!', (yyvsp[0].etree)); }
#line 3551 "ldgram.c" /* yacc.c:1646  */
    break;

  case 227:
#line 925 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[0].etree); }
#line 3557 "ldgram.c" /* yacc.c:1646  */
    break;

  case 228:
#line 927 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop ('~', (yyvsp[0].etree));}
#line 3563 "ldgram.c" /* yacc.c:1646  */
    break;

  case 229:
#line 930 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('*', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3569 "ldgram.c" /* yacc.c:1646  */
    break;

  case 230:
#line 932 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('/', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3575 "ldgram.c" /* yacc.c:1646  */
    break;

  case 231:
#line 934 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('%', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3581 "ldgram.c" /* yacc.c:1646  */
    break;

  case 232:
#line 936 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('+', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3587 "ldgram.c" /* yacc.c:1646  */
    break;

  case 233:
#line 938 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('-' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3593 "ldgram.c" /* yacc.c:1646  */
    break;

  case 234:
#line 940 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3599 "ldgram.c" /* yacc.c:1646  */
    break;

  case 235:
#line 942 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3605 "ldgram.c" /* yacc.c:1646  */
    break;

  case 236:
#line 944 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (EQ , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3611 "ldgram.c" /* yacc.c:1646  */
    break;

  case 237:
#line 946 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (NE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3617 "ldgram.c" /* yacc.c:1646  */
    break;

  case 238:
#line 948 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (LE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3623 "ldgram.c" /* yacc.c:1646  */
    break;

  case 239:
#line 950 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (GE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3629 "ldgram.c" /* yacc.c:1646  */
    break;

  case 240:
#line 952 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('<' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3635 "ldgram.c" /* yacc.c:1646  */
    break;

  case 241:
#line 954 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('>' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3641 "ldgram.c" /* yacc.c:1646  */
    break;

  case 242:
#line 956 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('&' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3647 "ldgram.c" /* yacc.c:1646  */
    break;

  case 243:
#line 958 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('^' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3653 "ldgram.c" /* yacc.c:1646  */
    break;

  case 244:
#line 960 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('|' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3659 "ldgram.c" /* yacc.c:1646  */
    break;

  case 245:
#line 962 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_trinop ('?' , (yyvsp[-4].etree), (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3665 "ldgram.c" /* yacc.c:1646  */
    break;

  case 246:
#line 964 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (ANDAND , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3671 "ldgram.c" /* yacc.c:1646  */
    break;

  case 247:
#line 966 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (OROR , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3677 "ldgram.c" /* yacc.c:1646  */
    break;

  case 248:
#line 968 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[-1].name)); }
#line 3683 "ldgram.c" /* yacc.c:1646  */
    break;

  case 249:
#line 970 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_bigintop ((yyvsp[0].bigint).integer, (yyvsp[0].bigint).str); }
#line 3689 "ldgram.c" /* yacc.c:1646  */
    break;

  case 250:
#line 972 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
#line 3695 "ldgram.c" /* yacc.c:1646  */
    break;

  case 251:
#line 975 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (ALIGNOF,(yyvsp[-1].name)); }
#line 3701 "ldgram.c" /* yacc.c:1646  */
    break;

  case 252:
#line 977 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (SIZEOF,(yyvsp[-1].name)); }
#line 3707 "ldgram.c" /* yacc.c:1646  */
    break;

  case 253:
#line 979 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (ADDR,(yyvsp[-1].name)); }
#line 3713 "ldgram.c" /* yacc.c:1646  */
    break;

  case 254:
#line 981 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (LOADADDR,(yyvsp[-1].name)); }
#line 3719 "ldgram.c" /* yacc.c:1646  */
    break;

  case 255:
#line 983 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[-1].name)); }
#line 3725 "ldgram.c" /* yacc.c:1646  */
    break;

  case 256:
#line 985 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[-1].etree)); }
#line 3731 "ldgram.c" /* yacc.c:1646  */
    break;

  case 257:
#line 987 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3737 "ldgram.c" /* yacc.c:1646  */
    break;

  case 258:
#line 989 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[-3].etree),(yyvsp[-1].etree)); }
#line 3743 "ldgram.c" /* yacc.c:1646  */
    break;

  case 259:
#line 991 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[-3].etree), (yyvsp[-1].etree)); }
#line 3749 "ldgram.c" /* yacc.c:1646  */
    break;

  case 260:
#line 993 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[-1].etree), (yyvsp[-3].etree)); }
#line 3755 "ldgram.c" /* yacc.c:1646  */
    break;

  case 261:
#line 995 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[-1].etree)); }
#line 3761 "ldgram.c" /* yacc.c:1646  */
    break;

  case 262:
#line 997 "ldgram.y" /* yacc.c:1646  */
    { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[-1].etree),
					  exp_nameop (NAME, (yyvsp[-3].name))); }
#line 3774 "ldgram.c" /* yacc.c:1646  */
    break;

  case 263:
#line 1006 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3780 "ldgram.c" /* yacc.c:1646  */
    break;

  case 264:
#line 1008 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (NAME,(yyvsp[0].name)); }
#line 3786 "ldgram.c" /* yacc.c:1646  */
    break;

  case 265:
#line 1010 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (MAX_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 3792 "ldgram.c" /* yacc.c:1646  */
    break;

  case 266:
#line 1012 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (MIN_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 3798 "ldgram.c" /* yacc.c:1646  */
    break;

  case 267:
#line 1014 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name)); }
#line 3804 "ldgram.c" /* yacc.c:1646  */
    break;

  case 268:
#line 1016 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[-1].name)); }
#line 3810 "ldgram.c" /* yacc.c:1646  */
    break;

  case 269:
#line 1018 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[-1].name)); }
#line 3816 "ldgram.c" /* yacc.c:1646  */
    break;

  case 270:
#line 1020 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (LOG2CEIL, (yyvsp[-1].etree)); }
#line 3822 "ldgram.c" /* yacc.c:1646  */
    break;

  case 271:
#line 1025 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = (yyvsp[0].name); }
#line 3828 "ldgram.c" /* yacc.c:1646  */
    break;

  case 272:
#line 1026 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = 0; }
#line 3834 "ldgram.c" /* yacc.c:1646  */
    break;

  case 273:
#line 1030 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3840 "ldgram.c" /* yacc.c:1646  */
    break;

  case 274:
#line 1031 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = 0; }
#line 3846 "ldgram.c" /* yacc.c:1646  */
    break;

  case 275:
#line 1035 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3852 "ldgram.c" /* yacc.c:1646  */
    break;

  case 276:
#line 1036 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = 0; }
#line 3858 "ldgram.c" /* yacc.c:1646  */
    break;

  case 277:
#line 1040 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = ALIGN_WITH_INPUT; }
#line 3864 "ldgram.c" /* yacc.c:1646  */
    break;

  case 278:
#line 1041 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = 0; }
#line 3870 "ldgram.c" /* yacc.c:1646  */
    break;

  case 279:
#line 1045 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3876 "ldgram.c" /* yacc.c:1646  */
    break;

  case 280:
#line 1046 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = 0; }
#line 3882 "ldgram.c" /* yacc.c:1646  */
    break;

  case 281:
#line 1050 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = ONLY_IF_RO; }
#line 3888 "ldgram.c" /* yacc.c:1646  */
    break;

  case 282:
#line 1051 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = ONLY_IF_RW; }
#line 3894 "ldgram.c" /* yacc.c:1646  */
    break;

  case 283:
#line 1052 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = SPECIAL; }
#line 3900 "ldgram.c" /* yacc.c:1646  */
    break;

  case 284:
#line 1053 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = 0; }
#line 3906 "ldgram.c" /* yacc.c:1646  */
    break;

  case 285:
#line 1056 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression(); }
#line 3912 "ldgram.c" /* yacc.c:1646  */
    break;

  case 286:
#line 1061 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_script (); }
#line 3918 "ldgram.c" /* yacc.c:1646  */
    break;

  case 287:
#line 1064 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_enter_output_section_statement((yyvsp[-9].name), (yyvsp[-7].etree),
							      sectype,
							      (yyvsp[-5].etree), (yyvsp[-3].etree), (yyvsp[-6].etree), (yyvsp[-1].token), (yyvsp[-4].token));
			}
#line 3928 "ldgram.c" /* yacc.c:1646  */
    break;

  case 288:
#line 1070 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_expression (); }
#line 3934 "ldgram.c" /* yacc.c:1646  */
    break;

  case 289:
#line 1072 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  lang_leave_output_section_statement ((yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
		}
#line 3943 "ldgram.c" /* yacc.c:1646  */
    break;

  case 290:
#line 1077 "ldgram.y" /* yacc.c:1646  */
    {}
#line 3949 "ldgram.c" /* yacc.c:1646  */
    break;

  case 291:
#line 1079 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 3955 "ldgram.c" /* yacc.c:1646  */
    break;

  case 292:
#line 1081 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_script (); }
#line 3961 "ldgram.c" /* yacc.c:1646  */
    break;

  case 293:
#line 1083 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_enter_overlay ((yyvsp[-5].etree), (yyvsp[-2].etree));
			}
#line 3969 "ldgram.c" /* yacc.c:1646  */
    break;

  case 294:
#line 1088 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_expression (); }
#line 3975 "ldgram.c" /* yacc.c:1646  */
    break;

  case 295:
#line 1090 "ldgram.y" /* yacc.c:1646  */
    {
			  ldlex_popstate ();
			  lang_leave_overlay ((yyvsp[-11].etree), (int) (yyvsp[-12].integer),
					      (yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
			}
#line 3985 "ldgram.c" /* yacc.c:1646  */
    break;

  case 297:
#line 1100 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 3991 "ldgram.c" /* yacc.c:1646  */
    break;

  case 298:
#line 1102 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assign (".", (yyvsp[0].etree), FALSE));
		}
#line 4000 "ldgram.c" /* yacc.c:1646  */
    break;

  case 300:
#line 1108 "ldgram.y" /* yacc.c:1646  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 4006 "ldgram.c" /* yacc.c:1646  */
    break;

  case 301:
#line 1110 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 4012 "ldgram.c" /* yacc.c:1646  */
    break;

  case 302:
#line 1114 "ldgram.y" /* yacc.c:1646  */
    { sectype = noload_section; }
#line 4018 "ldgram.c" /* yacc.c:1646  */
    break;

  case 303:
#line 1115 "ldgram.y" /* yacc.c:1646  */
    { sectype = noalloc_section; }
#line 4024 "ldgram.c" /* yacc.c:1646  */
    break;

  case 304:
#line 1116 "ldgram.y" /* yacc.c:1646  */
    { sectype = noalloc_section; }
#line 4030 "ldgram.c" /* yacc.c:1646  */
    break;

  case 305:
#line 1117 "ldgram.y" /* yacc.c:1646  */
    { sectype = noalloc_section; }
#line 4036 "ldgram.c" /* yacc.c:1646  */
    break;

  case 306:
#line 1118 "ldgram.y" /* yacc.c:1646  */
    { sectype = noalloc_section; }
#line 4042 "ldgram.c" /* yacc.c:1646  */
    break;

  case 308:
#line 1123 "ldgram.y" /* yacc.c:1646  */
    { sectype = normal_section; }
#line 4048 "ldgram.c" /* yacc.c:1646  */
    break;

  case 309:
#line 1124 "ldgram.y" /* yacc.c:1646  */
    { sectype = normal_section; }
#line 4054 "ldgram.c" /* yacc.c:1646  */
    break;

  case 310:
#line 1128 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-2].etree); }
#line 4060 "ldgram.c" /* yacc.c:1646  */
    break;

  case 311:
#line 1129 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (etree_type *)NULL;  }
#line 4066 "ldgram.c" /* yacc.c:1646  */
    break;

  case 312:
#line 1134 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-3].etree); }
#line 4072 "ldgram.c" /* yacc.c:1646  */
    break;

  case 313:
#line 1136 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-7].etree); }
#line 4078 "ldgram.c" /* yacc.c:1646  */
    break;

  case 314:
#line 1140 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 4084 "ldgram.c" /* yacc.c:1646  */
    break;

  case 315:
#line 1141 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (etree_type *) NULL;  }
#line 4090 "ldgram.c" /* yacc.c:1646  */
    break;

  case 316:
#line 1146 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = 0; }
#line 4096 "ldgram.c" /* yacc.c:1646  */
    break;

  case 317:
#line 1148 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = 1; }
#line 4102 "ldgram.c" /* yacc.c:1646  */
    break;

  case 318:
#line 1153 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = (yyvsp[0].name); }
#line 4108 "ldgram.c" /* yacc.c:1646  */
    break;

  case 319:
#line 1154 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = DEFAULT_MEMORY_REGION; }
#line 4114 "ldgram.c" /* yacc.c:1646  */
    break;

  case 320:
#line 1159 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.section_phdr) = NULL;
		}
#line 4122 "ldgram.c" /* yacc.c:1646  */
    break;

  case 321:
#line 1163 "ldgram.y" /* yacc.c:1646  */
    {
		  struct lang_output_section_phdr_list *n;

		  n = ((struct lang_output_section_phdr_list *)
		       xmalloc (sizeof *n));
		  n->name = (yyvsp[0].name);
		  n->used = FALSE;
		  n->next = (yyvsp[-2].section_phdr);
		  (yyval.section_phdr) = n;
		}
#line 4137 "ldgram.c" /* yacc.c:1646  */
    break;

  case 323:
#line 1179 "ldgram.y" /* yacc.c:1646  */
    {
			  ldlex_script ();
			  lang_enter_overlay_section ((yyvsp[0].name));
			}
#line 4146 "ldgram.c" /* yacc.c:1646  */
    break;

  case 324:
#line 1184 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_expression (); }
#line 4152 "ldgram.c" /* yacc.c:1646  */
    break;

  case 325:
#line 1186 "ldgram.y" /* yacc.c:1646  */
    {
			  ldlex_popstate ();
			  lang_leave_overlay_section ((yyvsp[0].fill), (yyvsp[-1].section_phdr));
			}
#line 4161 "ldgram.c" /* yacc.c:1646  */
    break;

  case 330:
#line 1203 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 4167 "ldgram.c" /* yacc.c:1646  */
    break;

  case 331:
#line 1204 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 4173 "ldgram.c" /* yacc.c:1646  */
    break;

  case 332:
#line 1206 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_new_phdr ((yyvsp[-5].name), (yyvsp[-3].etree), (yyvsp[-2].phdr).filehdr, (yyvsp[-2].phdr).phdrs, (yyvsp[-2].phdr).at,
				 (yyvsp[-2].phdr).flags);
		}
#line 4182 "ldgram.c" /* yacc.c:1646  */
    break;

  case 333:
#line 1214 "ldgram.y" /* yacc.c:1646  */
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
#line 4227 "ldgram.c" /* yacc.c:1646  */
    break;

  case 334:
#line 1258 "ldgram.y" /* yacc.c:1646  */
    {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
#line 4235 "ldgram.c" /* yacc.c:1646  */
    break;

  case 335:
#line 1262 "ldgram.y" /* yacc.c:1646  */
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
#line 4252 "ldgram.c" /* yacc.c:1646  */
    break;

  case 336:
#line 1275 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  (yyval.phdr).at = (yyvsp[-2].etree);
		}
#line 4261 "ldgram.c" /* yacc.c:1646  */
    break;

  case 337:
#line 1283 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.etree) = NULL;
		}
#line 4269 "ldgram.c" /* yacc.c:1646  */
    break;

  case 338:
#line 1287 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.etree) = (yyvsp[-1].etree);
		}
#line 4277 "ldgram.c" /* yacc.c:1646  */
    break;

  case 339:
#line 1293 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
#line 4286 "ldgram.c" /* yacc.c:1646  */
    break;

  case 340:
#line 1298 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4295 "ldgram.c" /* yacc.c:1646  */
    break;

  case 344:
#line 1315 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_append_dynamic_list ((yyvsp[-1].versyms));
		}
#line 4303 "ldgram.c" /* yacc.c:1646  */
    break;

  case 345:
#line 1323 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
#line 4312 "ldgram.c" /* yacc.c:1646  */
    break;

  case 346:
#line 1328 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4321 "ldgram.c" /* yacc.c:1646  */
    break;

  case 347:
#line 1337 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_version_script ();
		}
#line 4329 "ldgram.c" /* yacc.c:1646  */
    break;

  case 348:
#line 1341 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		}
#line 4337 "ldgram.c" /* yacc.c:1646  */
    break;

  case 351:
#line 1353 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_register_vers_node (NULL, (yyvsp[-2].versnode), NULL);
		}
#line 4345 "ldgram.c" /* yacc.c:1646  */
    break;

  case 352:
#line 1357 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_register_vers_node ((yyvsp[-4].name), (yyvsp[-2].versnode), NULL);
		}
#line 4353 "ldgram.c" /* yacc.c:1646  */
    break;

  case 353:
#line 1361 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_register_vers_node ((yyvsp[-5].name), (yyvsp[-3].versnode), (yyvsp[-1].deflist));
		}
#line 4361 "ldgram.c" /* yacc.c:1646  */
    break;

  case 354:
#line 1368 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[0].name));
		}
#line 4369 "ldgram.c" /* yacc.c:1646  */
    break;

  case 355:
#line 1372 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[-1].deflist), (yyvsp[0].name));
		}
#line 4377 "ldgram.c" /* yacc.c:1646  */
    break;

  case 356:
#line 1379 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
#line 4385 "ldgram.c" /* yacc.c:1646  */
    break;

  case 357:
#line 1383 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4393 "ldgram.c" /* yacc.c:1646  */
    break;

  case 358:
#line 1387 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4401 "ldgram.c" /* yacc.c:1646  */
    break;

  case 359:
#line 1391 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[-1].versyms));
		}
#line 4409 "ldgram.c" /* yacc.c:1646  */
    break;

  case 360:
#line 1395 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-5].versyms), (yyvsp[-1].versyms));
		}
#line 4417 "ldgram.c" /* yacc.c:1646  */
    break;

  case 361:
#line 1402 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, FALSE);
		}
#line 4425 "ldgram.c" /* yacc.c:1646  */
    break;

  case 362:
#line 1406 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, TRUE);
		}
#line 4433 "ldgram.c" /* yacc.c:1646  */
    break;

  case 363:
#line 1410 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, FALSE);
		}
#line 4441 "ldgram.c" /* yacc.c:1646  */
    break;

  case 364:
#line 1414 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, TRUE);
		}
#line 4449 "ldgram.c" /* yacc.c:1646  */
    break;

  case 365:
#line 1418 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4458 "ldgram.c" /* yacc.c:1646  */
    break;

  case 366:
#line 1423 "ldgram.y" /* yacc.c:1646  */
    {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[-2].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[-8].versyms);
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4470 "ldgram.c" /* yacc.c:1646  */
    break;

  case 367:
#line 1431 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4479 "ldgram.c" /* yacc.c:1646  */
    break;

  case 368:
#line 1436 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4488 "ldgram.c" /* yacc.c:1646  */
    break;

  case 369:
#line 1441 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, FALSE);
		}
#line 4496 "ldgram.c" /* yacc.c:1646  */
    break;

  case 370:
#line 1445 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "global", ldgram_vers_current_lang, FALSE);
		}
#line 4504 "ldgram.c" /* yacc.c:1646  */
    break;

  case 371:
#line 1449 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, FALSE);
		}
#line 4512 "ldgram.c" /* yacc.c:1646  */
    break;

  case 372:
#line 1453 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "local", ldgram_vers_current_lang, FALSE);
		}
#line 4520 "ldgram.c" /* yacc.c:1646  */
    break;

  case 373:
#line 1457 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, FALSE);
		}
#line 4528 "ldgram.c" /* yacc.c:1646  */
    break;

  case 374:
#line 1461 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "extern", ldgram_vers_current_lang, FALSE);
		}
#line 4536 "ldgram.c" /* yacc.c:1646  */
    break;


#line 4540 "ldgram.c" /* yacc.c:1646  */
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
#line 1471 "ldgram.y" /* yacc.c:1906  */

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
