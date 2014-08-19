/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT = 258,
     NAME = 259,
     LNAME = 260,
     OREQ = 261,
     ANDEQ = 262,
     RSHIFTEQ = 263,
     LSHIFTEQ = 264,
     DIVEQ = 265,
     MULTEQ = 266,
     MINUSEQ = 267,
     PLUSEQ = 268,
     OROR = 269,
     ANDAND = 270,
     NE = 271,
     EQ = 272,
     GE = 273,
     LE = 274,
     RSHIFT = 275,
     LSHIFT = 276,
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
     FORMAT = 357,
     PUBLIC = 358,
     DEFSYMEND = 359,
     BASE = 360,
     ALIAS = 361,
     TRUNCATE = 362,
     REL = 363,
     INPUT_SCRIPT = 364,
     INPUT_MRI_SCRIPT = 365,
     INPUT_DEFSYM = 366,
     CASE = 367,
     EXTERN = 368,
     START = 369,
     VERS_TAG = 370,
     VERS_IDENTIFIER = 371,
     GLOBAL = 372,
     LOCAL = 373,
     VERSIONK = 374,
     INPUT_VERSION_SCRIPT = 375,
     KEEP = 376,
     ONLY_IF_RO = 377,
     ONLY_IF_RW = 378,
     SPECIAL = 379,
     INPUT_SECTION_FLAGS = 380,
     EXCLUDE_FILE = 381,
     CONSTANT = 382,
     INPUT_DYNAMIC_LIST = 383
   };
#endif
/* Tokens.  */
#define INT 258
#define NAME 259
#define LNAME 260
#define OREQ 261
#define ANDEQ 262
#define RSHIFTEQ 263
#define LSHIFTEQ 264
#define DIVEQ 265
#define MULTEQ 266
#define MINUSEQ 267
#define PLUSEQ 268
#define OROR 269
#define ANDAND 270
#define NE 271
#define EQ 272
#define GE 273
#define LE 274
#define RSHIFT 275
#define LSHIFT 276
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
#define FORMAT 357
#define PUBLIC 358
#define DEFSYMEND 359
#define BASE 360
#define ALIAS 361
#define TRUNCATE 362
#define REL 363
#define INPUT_SCRIPT 364
#define INPUT_MRI_SCRIPT 365
#define INPUT_DEFSYM 366
#define CASE 367
#define EXTERN 368
#define START 369
#define VERS_TAG 370
#define VERS_IDENTIFIER 371
#define GLOBAL 372
#define LOCAL 373
#define VERSIONK 374
#define INPUT_VERSION_SCRIPT 375
#define KEEP 376
#define ONLY_IF_RO 377
#define ONLY_IF_RW 378
#define SPECIAL 379
#define INPUT_SECTION_FLAGS 380
#define EXCLUDE_FILE 381
#define CONSTANT 382
#define INPUT_DYNAMIC_LIST 383




/* Copy the first part of user declarations.  */
#line 24 "ldgram.y"

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


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 62 "ldgram.y"
{
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
}
/* Line 193 of yacc.c.  */
#line 422 "ldgram.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 435 "ldgram.c"

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
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
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
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1883

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  152
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  128
/* YYNRULES -- Number of rules.  */
#define YYNRULES  366
/* YYNRULES -- Number of states.  */
#define YYNSTATES  797

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   383

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   150,     2,     2,     2,    34,    21,     2,
      37,   147,    32,    30,   145,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   146,
      24,     6,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   148,     2,   149,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,    19,    59,   151,     2,     2,     2,
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
       5,     7,     8,     9,    10,    11,    12,    13,    14,    17,
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
     141,   142,   143,   144
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     9,    12,    15,    18,    20,    21,
      26,    27,    30,    34,    35,    38,    43,    45,    47,    50,
      52,    57,    62,    66,    69,    74,    78,    83,    88,    93,
      98,   103,   106,   109,   112,   117,   122,   125,   128,   131,
     134,   135,   141,   144,   145,   149,   152,   153,   155,   159,
     161,   165,   166,   168,   172,   173,   176,   178,   181,   185,
     186,   189,   192,   193,   195,   197,   199,   201,   203,   205,
     207,   209,   211,   213,   218,   223,   228,   233,   242,   247,
     249,   251,   256,   257,   263,   268,   269,   275,   280,   285,
     289,   293,   300,   305,   307,   311,   314,   316,   320,   323,
     324,   330,   331,   339,   340,   347,   352,   355,   358,   359,
     364,   367,   368,   376,   378,   380,   382,   384,   390,   395,
     400,   405,   413,   421,   429,   437,   446,   451,   453,   457,
     462,   465,   467,   471,   473,   475,   478,   482,   487,   492,
     498,   500,   501,   507,   510,   512,   514,   516,   521,   523,
     528,   533,   534,   543,   544,   550,   553,   555,   556,   558,
     560,   562,   564,   566,   568,   570,   573,   574,   576,   578,
     580,   582,   584,   586,   588,   590,   592,   594,   598,   602,
     609,   616,   623,   625,   626,   631,   633,   634,   638,   640,
     641,   649,   650,   656,   660,   664,   665,   669,   671,   674,
     676,   679,   684,   689,   693,   697,   699,   704,   708,   709,
     711,   713,   714,   717,   721,   722,   725,   728,   732,   737,
     740,   743,   746,   750,   754,   758,   762,   766,   770,   774,
     778,   782,   786,   790,   794,   798,   802,   806,   810,   816,
     820,   824,   829,   831,   833,   838,   843,   848,   853,   858,
     863,   868,   875,   882,   889,   894,   901,   906,   908,   915,
     922,   929,   934,   939,   943,   944,   949,   950,   955,   956,
     961,   962,   964,   966,   968,   969,   970,   971,   972,   973,
     974,   994,   995,   996,   997,   998,   999,  1018,  1019,  1020,
    1028,  1029,  1035,  1037,  1039,  1041,  1043,  1045,  1049,  1050,
    1053,  1057,  1060,  1067,  1078,  1081,  1083,  1084,  1086,  1089,
    1090,  1091,  1095,  1096,  1097,  1098,  1099,  1111,  1116,  1117,
    1120,  1121,  1122,  1129,  1131,  1132,  1136,  1142,  1143,  1147,
    1148,  1151,  1153,  1156,  1161,  1164,  1165,  1168,  1169,  1175,
    1177,  1180,  1185,  1191,  1198,  1200,  1203,  1204,  1207,  1212,
    1217,  1226,  1228,  1230,  1234,  1238,  1239,  1249,  1250,  1258,
    1260,  1264,  1266,  1270,  1272,  1276,  1277
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     153,     0,    -1,   125,   169,    -1,   126,   157,    -1,   136,
     268,    -1,   144,   263,    -1,   127,   155,    -1,     4,    -1,
      -1,   156,     4,     6,   224,    -1,    -1,   158,   159,    -1,
     159,   160,   113,    -1,    -1,   108,   224,    -1,   108,   224,
     145,   224,    -1,     4,    -1,   109,    -1,   115,   162,    -1,
     114,    -1,   119,     4,     6,   224,    -1,   119,     4,   145,
     224,    -1,   119,     4,   224,    -1,   118,     4,    -1,   110,
       4,   145,   224,    -1,   110,     4,   224,    -1,   110,     4,
       6,   224,    -1,    38,     4,     6,   224,    -1,    38,     4,
     145,   224,    -1,   101,     4,     6,   224,    -1,   101,     4,
     145,   224,    -1,   111,   164,    -1,   112,   163,    -1,   116,
       4,    -1,   122,     4,   145,     4,    -1,   122,     4,   145,
       3,    -1,   121,   224,    -1,   123,     3,    -1,   128,   165,
      -1,   129,   166,    -1,    -1,    66,   154,   161,   159,    36,
      -1,   130,     4,    -1,    -1,   162,   145,     4,    -1,   162,
       4,    -1,    -1,     4,    -1,   163,   145,     4,    -1,     4,
      -1,   164,   145,     4,    -1,    -1,     4,    -1,   165,   145,
       4,    -1,    -1,   167,   168,    -1,     4,    -1,   168,     4,
      -1,   168,   145,     4,    -1,    -1,   170,   171,    -1,   171,
     172,    -1,    -1,   204,    -1,   179,    -1,   255,    -1,   215,
      -1,   216,    -1,   218,    -1,   220,    -1,   181,    -1,   270,
      -1,   146,    -1,    76,    37,     4,   147,    -1,    77,    37,
     154,   147,    -1,    99,    37,   154,   147,    -1,    61,    37,
       4,   147,    -1,    61,    37,     4,   145,     4,   145,     4,
     147,    -1,    63,    37,     4,   147,    -1,    62,    -1,    64,
      -1,    97,    37,   175,   147,    -1,    -1,    98,   173,    37,
     175,   147,    -1,    78,    37,   154,   147,    -1,    -1,    66,
     154,   174,   171,    36,    -1,    92,    37,   221,   147,    -1,
     129,    37,   166,   147,    -1,    48,    49,     4,    -1,    48,
      50,     4,    -1,    68,    37,     4,   145,     4,   147,    -1,
      69,    37,     4,   147,    -1,     4,    -1,   175,   145,     4,
      -1,   175,     4,    -1,     5,    -1,   175,   145,     5,    -1,
     175,     5,    -1,    -1,   107,    37,   176,   175,   147,    -1,
      -1,   175,   145,   107,    37,   177,   175,   147,    -1,    -1,
     175,   107,    37,   178,   175,   147,    -1,    46,    58,   180,
      59,    -1,   180,   230,    -1,   180,   181,    -1,    -1,    79,
      37,     4,   147,    -1,   202,   201,    -1,    -1,   117,   182,
      37,   224,   145,     4,   147,    -1,     4,    -1,    32,    -1,
      15,    -1,   183,    -1,   142,    37,   187,   147,   183,    -1,
      54,    37,   183,   147,    -1,    55,    37,   183,   147,    -1,
      56,    37,   183,   147,    -1,    54,    37,    55,    37,   183,
     147,   147,    -1,    54,    37,    54,    37,   183,   147,   147,
      -1,    55,    37,    54,    37,   183,   147,   147,    -1,    55,
      37,    55,    37,   183,   147,   147,    -1,    54,    37,   142,
      37,   187,   147,   183,   147,    -1,    57,    37,   183,   147,
      -1,     4,    -1,   185,    21,     4,    -1,   141,    37,   185,
     147,    -1,   187,   183,    -1,   183,    -1,   188,   203,   184,
      -1,   184,    -1,     4,    -1,   186,     4,    -1,   148,   188,
     149,    -1,   186,   148,   188,   149,    -1,   184,    37,   188,
     147,    -1,   186,   184,    37,   188,   147,    -1,   189,    -1,
      -1,   137,    37,   191,   189,   147,    -1,   202,   201,    -1,
      96,    -1,   146,    -1,   100,    -1,    54,    37,   100,   147,
      -1,   190,    -1,   197,    37,   222,   147,    -1,    94,    37,
     198,   147,    -1,    -1,   117,   193,    37,   224,   145,     4,
     147,   201,    -1,    -1,    66,   154,   194,   196,    36,    -1,
     195,   192,    -1,   192,    -1,    -1,   195,    -1,    41,    -1,
      42,    -1,    43,    -1,    44,    -1,    45,    -1,   222,    -1,
       6,   198,    -1,    -1,    14,    -1,    13,    -1,    12,    -1,
      11,    -1,    10,    -1,     9,    -1,     8,    -1,     7,    -1,
     146,    -1,   145,    -1,     4,     6,   222,    -1,     4,   200,
     222,    -1,   104,    37,     4,     6,   222,   147,    -1,   105,
      37,     4,     6,   222,   147,    -1,   106,    37,     4,     6,
     222,   147,    -1,   145,    -1,    -1,    67,    58,   205,    59,
      -1,   206,    -1,    -1,   206,   203,   207,    -1,   207,    -1,
      -1,     4,   208,   212,    16,   210,   203,   211,    -1,    -1,
      66,   154,   209,   205,    36,    -1,    93,     6,   222,    -1,
      95,     6,   222,    -1,    -1,    37,   213,   147,    -1,   214,
      -1,   213,   214,    -1,     4,    -1,   150,     4,    -1,    87,
      37,   154,   147,    -1,    88,    37,   217,   147,    -1,    88,
      37,   147,    -1,   217,   203,   154,    -1,   154,    -1,    89,
      37,   219,   147,    -1,   219,   203,   154,    -1,    -1,    90,
      -1,    91,    -1,    -1,     4,   221,    -1,     4,   145,   221,
      -1,    -1,   223,   224,    -1,    31,   224,    -1,    37,   224,
     147,    -1,    80,    37,   224,   147,    -1,   150,   224,    -1,
      30,   224,    -1,   151,   224,    -1,   224,    32,   224,    -1,
     224,    33,   224,    -1,   224,    34,   224,    -1,   224,    30,
     224,    -1,   224,    31,   224,    -1,   224,    29,   224,    -1,
     224,    28,   224,    -1,   224,    23,   224,    -1,   224,    22,
     224,    -1,   224,    27,   224,    -1,   224,    26,   224,    -1,
     224,    24,   224,    -1,   224,    25,   224,    -1,   224,    21,
     224,    -1,   224,    20,   224,    -1,   224,    19,   224,    -1,
     224,    15,   224,    16,   224,    -1,   224,    18,   224,    -1,
     224,    17,   224,    -1,    75,    37,     4,   147,    -1,     3,
      -1,    60,    -1,    82,    37,     4,   147,    -1,    81,    37,
       4,   147,    -1,    83,    37,     4,   147,    -1,    84,    37,
       4,   147,    -1,   143,    37,     4,   147,    -1,   111,    37,
     224,   147,    -1,    38,    37,   224,   147,    -1,    38,    37,
     224,   145,   224,   147,    -1,    51,    37,   224,   145,   224,
     147,    -1,    52,    37,   224,   145,   224,   147,    -1,    53,
      37,   224,   147,    -1,    65,    37,     4,   145,   224,   147,
      -1,    39,    37,   224,   147,    -1,     4,    -1,    85,    37,
     224,   145,   224,   147,    -1,    86,    37,   224,   145,   224,
     147,    -1,   117,    37,   224,   145,     4,   147,    -1,    93,
      37,     4,   147,    -1,    95,    37,     4,   147,    -1,   102,
      25,     4,    -1,    -1,   102,    37,   224,   147,    -1,    -1,
      38,    37,   224,   147,    -1,    -1,   103,    37,   224,   147,
      -1,    -1,   138,    -1,   139,    -1,   140,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,   231,   246,   226,   227,   228,
     232,   229,    58,   233,   196,    59,   234,   249,   225,   250,
     199,   235,   203,    -1,    -1,    -1,    -1,    -1,    -1,    74,
     236,   247,   248,   226,   228,   237,    58,   238,   251,    59,
     239,   249,   225,   250,   199,   240,   203,    -1,    -1,    -1,
      98,   241,   246,   242,    58,   180,    59,    -1,    -1,    66,
     154,   243,   180,    36,    -1,    70,    -1,    71,    -1,    72,
      -1,    73,    -1,    74,    -1,    37,   244,   147,    -1,    -1,
      37,   147,    -1,   224,   245,    16,    -1,   245,    16,    -1,
      40,    37,   224,   147,   245,    16,    -1,    40,    37,   224,
     147,    39,    37,   224,   147,   245,    16,    -1,   224,    16,
      -1,    16,    -1,    -1,    92,    -1,    25,     4,    -1,    -1,
      -1,   250,    16,     4,    -1,    -1,    -1,    -1,    -1,   251,
       4,   252,    58,   196,    59,   253,   250,   199,   254,   203,
      -1,    47,    58,   256,    59,    -1,    -1,   256,   257,    -1,
      -1,    -1,     4,   258,   260,   261,   259,   146,    -1,   224,
      -1,    -1,     4,   262,   261,    -1,   102,    37,   224,   147,
     261,    -1,    -1,    37,   224,   147,    -1,    -1,   264,   265,
      -1,   266,    -1,   265,   266,    -1,    58,   267,    59,   146,
      -1,   276,   146,    -1,    -1,   269,   272,    -1,    -1,   271,
     135,    58,   272,    59,    -1,   273,    -1,   272,   273,    -1,
      58,   275,    59,   146,    -1,   131,    58,   275,    59,   146,
      -1,   131,    58,   275,    59,   274,   146,    -1,   131,    -1,
     274,   131,    -1,    -1,   276,   146,    -1,   133,    16,   276,
     146,    -1,   134,    16,   276,   146,    -1,   133,    16,   276,
     146,   134,    16,   276,   146,    -1,   132,    -1,     4,    -1,
     276,   146,   132,    -1,   276,   146,     4,    -1,    -1,   276,
     146,   129,     4,    58,   277,   276,   279,    59,    -1,    -1,
     129,     4,    58,   278,   276,   279,    59,    -1,   133,    -1,
     276,   146,   133,    -1,   134,    -1,   276,   146,   134,    -1,
     129,    -1,   276,   146,   129,    -1,    -1,   146,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   168,   168,   169,   170,   171,   172,   176,   180,   180,
     190,   190,   203,   204,   208,   209,   210,   213,   216,   217,
     218,   220,   222,   224,   226,   228,   230,   232,   234,   236,
     238,   240,   241,   242,   244,   246,   248,   250,   252,   253,
     255,   254,   258,   260,   264,   265,   266,   270,   272,   276,
     278,   283,   284,   285,   290,   290,   295,   297,   299,   304,
     304,   310,   311,   316,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,   328,   330,   332,   335,   337,   339,
     341,   343,   345,   344,   348,   351,   350,   354,   358,   359,
     361,   363,   365,   370,   373,   376,   379,   382,   385,   389,
     388,   394,   393,   399,   398,   406,   410,   411,   412,   416,
     418,   419,   419,   427,   431,   435,   442,   449,   456,   463,
     470,   477,   484,   491,   498,   505,   512,   521,   539,   560,
     573,   582,   593,   602,   613,   622,   631,   635,   644,   648,
     656,   658,   657,   664,   665,   669,   670,   675,   680,   681,
     686,   690,   690,   694,   693,   700,   701,   704,   706,   710,
     712,   714,   716,   718,   723,   730,   732,   736,   738,   740,
     742,   744,   746,   748,   750,   755,   755,   760,   764,   772,
     776,   780,   788,   788,   792,   795,   795,   798,   799,   804,
     803,   809,   808,   815,   823,   831,   832,   836,   837,   841,
     843,   848,   853,   854,   859,   861,   867,   869,   871,   875,
     877,   883,   886,   895,   906,   906,   912,   914,   916,   918,
     920,   922,   925,   927,   929,   931,   933,   935,   937,   939,
     941,   943,   945,   947,   949,   951,   953,   955,   957,   959,
     961,   963,   965,   967,   970,   972,   974,   976,   978,   980,
     982,   984,   986,   988,   990,   992,  1001,  1003,  1005,  1007,
    1009,  1011,  1013,  1019,  1020,  1024,  1025,  1029,  1030,  1034,
    1035,  1039,  1040,  1041,  1042,  1045,  1049,  1052,  1058,  1060,
    1045,  1067,  1069,  1071,  1076,  1078,  1066,  1088,  1090,  1088,
    1096,  1095,  1102,  1103,  1104,  1105,  1106,  1110,  1111,  1112,
    1116,  1117,  1122,  1123,  1128,  1129,  1134,  1135,  1140,  1142,
    1147,  1150,  1163,  1167,  1172,  1174,  1165,  1182,  1185,  1187,
    1191,  1192,  1191,  1201,  1246,  1249,  1262,  1271,  1274,  1281,
    1281,  1293,  1294,  1298,  1302,  1311,  1311,  1325,  1325,  1335,
    1336,  1340,  1344,  1348,  1355,  1359,  1367,  1370,  1374,  1378,
    1382,  1389,  1393,  1397,  1401,  1406,  1405,  1419,  1418,  1428,
    1432,  1436,  1440,  1444,  1448,  1454,  1456
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "NAME", "LNAME", "'='", "OREQ",
  "ANDEQ", "RSHIFTEQ", "LSHIFTEQ", "DIVEQ", "MULTEQ", "MINUSEQ", "PLUSEQ",
  "'?'", "':'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "NE", "EQ", "'<'",
  "'>'", "GE", "LE", "RSHIFT", "LSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'",
  "UNARY", "END", "'('", "ALIGN_K", "BLOCK", "BIND", "QUAD", "SQUAD",
  "LONG", "SHORT", "BYTE", "SECTIONS", "PHDRS", "INSERT_K", "AFTER",
  "BEFORE", "DATA_SEGMENT_ALIGN", "DATA_SEGMENT_RELRO_END",
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
  "ENDWORD", "ORDER", "NAMEWORD", "ASSERT_K", "FORMAT", "PUBLIC",
  "DEFSYMEND", "BASE", "ALIAS", "TRUNCATE", "REL", "INPUT_SCRIPT",
  "INPUT_MRI_SCRIPT", "INPUT_DEFSYM", "CASE", "EXTERN", "START",
  "VERS_TAG", "VERS_IDENTIFIER", "GLOBAL", "LOCAL", "VERSIONK",
  "INPUT_VERSION_SCRIPT", "KEEP", "ONLY_IF_RO", "ONLY_IF_RW", "SPECIAL",
  "INPUT_SECTION_FLAGS", "EXCLUDE_FILE", "CONSTANT", "INPUT_DYNAMIC_LIST",
  "','", "';'", "')'", "'['", "']'", "'!'", "'~'", "$accept", "file",
  "filename", "defsym_expr", "@1", "mri_script_file", "@2",
  "mri_script_lines", "mri_script_command", "@3", "ordernamelist",
  "mri_load_name_list", "mri_abs_name_list", "casesymlist",
  "extern_name_list", "@4", "extern_name_list_body", "script_file", "@5",
  "ifile_list", "ifile_p1", "@6", "@7", "input_list", "@8", "@9", "@10",
  "sections", "sec_or_group_p1", "statement_anywhere", "@11",
  "wildcard_name", "wildcard_spec", "sect_flag_list", "sect_flags",
  "exclude_name_list", "file_NAME_list", "input_section_spec_no_keep",
  "input_section_spec", "@12", "statement", "@13", "@14", "statement_list",
  "statement_list_opt", "length", "fill_exp", "fill_opt", "assign_op",
  "end", "assignment", "opt_comma", "memory", "memory_spec_list_opt",
  "memory_spec_list", "memory_spec", "@15", "@16", "origin_spec",
  "length_spec", "attributes_opt", "attributes_list", "attributes_string",
  "startup", "high_level_library", "high_level_library_NAME_list",
  "low_level_library", "low_level_library_NAME_list",
  "floating_point_support", "nocrossref_list", "mustbe_exp", "@17", "exp",
  "memspec_at_opt", "opt_at", "opt_align", "opt_subalign",
  "sect_constraint", "section", "@18", "@19", "@20", "@21", "@22", "@23",
  "@24", "@25", "@26", "@27", "@28", "@29", "@30", "type", "atype",
  "opt_exp_with_type", "opt_exp_without_type", "opt_nocrossrefs",
  "memspec_opt", "phdr_opt", "overlay_section", "@31", "@32", "@33",
  "phdrs", "phdr_list", "phdr", "@34", "@35", "phdr_type",
  "phdr_qualifiers", "phdr_val", "dynamic_list_file", "@36",
  "dynamic_list_nodes", "dynamic_list_node", "dynamic_list_tag",
  "version_script_file", "@37", "version", "@38", "vers_nodes",
  "vers_node", "verdep", "vers_tag", "vers_defns", "@39", "@40",
  "opt_semicolon", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,    61,   261,   262,   263,
     264,   265,   266,   267,   268,    63,    58,   269,   270,   124,
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
     379,   380,   381,   382,   383,    44,    59,    41,    91,    93,
      33,   126
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   152,   153,   153,   153,   153,   153,   154,   156,   155,
     158,   157,   159,   159,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     161,   160,   160,   160,   162,   162,   162,   163,   163,   164,
     164,   165,   165,   165,   167,   166,   168,   168,   168,   170,
     169,   171,   171,   172,   172,   172,   172,   172,   172,   172,
     172,   172,   172,   172,   172,   172,   172,   172,   172,   172,
     172,   172,   173,   172,   172,   174,   172,   172,   172,   172,
     172,   172,   172,   175,   175,   175,   175,   175,   175,   176,
     175,   177,   175,   178,   175,   179,   180,   180,   180,   181,
     181,   182,   181,   183,   183,   183,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   185,   185,   186,
     187,   187,   188,   188,   189,   189,   189,   189,   189,   189,
     190,   191,   190,   192,   192,   192,   192,   192,   192,   192,
     192,   193,   192,   194,   192,   195,   195,   196,   196,   197,
     197,   197,   197,   197,   198,   199,   199,   200,   200,   200,
     200,   200,   200,   200,   200,   201,   201,   202,   202,   202,
     202,   202,   203,   203,   204,   205,   205,   206,   206,   208,
     207,   209,   207,   210,   211,   212,   212,   213,   213,   214,
     214,   215,   216,   216,   217,   217,   218,   219,   219,   220,
     220,   221,   221,   221,   223,   222,   224,   224,   224,   224,
     224,   224,   224,   224,   224,   224,   224,   224,   224,   224,
     224,   224,   224,   224,   224,   224,   224,   224,   224,   224,
     224,   224,   224,   224,   224,   224,   224,   224,   224,   224,
     224,   224,   224,   224,   224,   224,   224,   224,   224,   224,
     224,   224,   224,   225,   225,   226,   226,   227,   227,   228,
     228,   229,   229,   229,   229,   231,   232,   233,   234,   235,
     230,   236,   237,   238,   239,   240,   230,   241,   242,   230,
     243,   230,   244,   244,   244,   244,   244,   245,   245,   245,
     246,   246,   246,   246,   247,   247,   248,   248,   249,   249,
     250,   250,   251,   252,   253,   254,   251,   255,   256,   256,
     258,   259,   257,   260,   261,   261,   261,   262,   262,   264,
     263,   265,   265,   266,   267,   269,   268,   271,   270,   272,
     272,   273,   273,   273,   274,   274,   275,   275,   275,   275,
     275,   276,   276,   276,   276,   277,   276,   278,   276,   276,
     276,   276,   276,   276,   276,   279,   279
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
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
       3,     6,     4,     1,     3,     2,     1,     3,     2,     0,
       5,     0,     7,     0,     6,     4,     2,     2,     0,     4,
       2,     0,     7,     1,     1,     1,     1,     5,     4,     4,
       4,     7,     7,     7,     7,     8,     4,     1,     3,     4,
       2,     1,     3,     1,     1,     2,     3,     4,     4,     5,
       1,     0,     5,     2,     1,     1,     1,     4,     1,     4,
       4,     0,     8,     0,     5,     2,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     2,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     6,
       6,     6,     1,     0,     4,     1,     0,     3,     1,     0,
       7,     0,     5,     3,     3,     0,     3,     1,     2,     1,
       2,     4,     4,     3,     3,     1,     4,     3,     0,     1,
       1,     0,     2,     3,     0,     2,     2,     3,     4,     2,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     5,     3,
       3,     4,     1,     1,     4,     4,     4,     4,     4,     4,
       4,     6,     6,     6,     4,     6,     4,     1,     6,     6,
       6,     4,     4,     3,     0,     4,     0,     4,     0,     4,
       0,     1,     1,     1,     0,     0,     0,     0,     0,     0,
      19,     0,     0,     0,     0,     0,    18,     0,     0,     7,
       0,     5,     1,     1,     1,     1,     1,     3,     0,     2,
       3,     2,     6,    10,     2,     1,     0,     1,     2,     0,
       0,     3,     0,     0,     0,     0,    11,     4,     0,     2,
       0,     0,     6,     1,     0,     3,     5,     0,     3,     0,
       2,     1,     2,     4,     2,     0,     2,     0,     5,     1,
       2,     4,     5,     6,     1,     2,     0,     2,     4,     4,
       8,     1,     1,     3,     3,     0,     9,     0,     7,     1,
       3,     1,     3,     1,     3,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    59,    10,     8,   335,   329,     0,     2,    62,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    60,    11,
       0,   346,     0,   336,   339,     0,   330,   331,     0,     0,
       0,     0,     0,    79,     0,    80,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   209,   210,     0,
       0,    82,     0,     0,     0,     0,   111,     0,    72,    61,
      64,    70,     0,    63,    66,    67,    68,    69,    65,    71,
       0,    16,     0,     0,     0,     0,    17,     0,     0,     0,
      19,    46,     0,     0,     0,     0,     0,     0,    51,    54,
       0,     0,     0,   352,   363,   351,   359,   361,     0,     0,
     346,   340,   359,   361,     0,     0,   332,   214,   174,   173,
     172,   171,   170,   169,   168,   167,   214,   108,   318,     0,
       0,     0,     0,     7,    85,   186,     0,     0,     0,     0,
       0,     0,     0,     0,   208,   211,     0,     0,     0,     0,
       0,     0,     0,    54,   176,   175,   110,     0,     0,    40,
       0,   242,   257,     0,     0,     0,     0,     0,     0,     0,
       0,   243,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    14,     0,
      49,    31,    47,    32,    18,    33,    23,     0,    36,     0,
      37,    52,    38,    39,     0,    42,    12,     9,     0,     0,
       0,     0,   347,     0,     0,   334,   177,     0,   178,     0,
       0,    89,    90,     0,     0,    62,   189,     0,     0,   183,
     188,     0,     0,     0,     0,     0,     0,     0,   203,   205,
     183,   183,   211,     0,    93,    96,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    13,     0,
       0,   220,   216,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   219,   221,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    25,     0,     0,
      45,     0,     0,     0,    22,     0,     0,    56,    55,   357,
       0,     0,   341,   354,   364,   353,   360,   362,     0,   333,
     215,   275,   105,     0,   281,   287,   107,   106,   320,   317,
     319,     0,    76,    78,   337,   195,   191,   184,   182,     0,
       0,    92,    73,    74,    84,   109,   201,   202,     0,   206,
       0,   211,   212,    87,    99,    95,    98,     0,     0,    81,
       0,    75,   214,   214,   214,     0,    88,     0,    27,    28,
      43,    29,    30,   217,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   240,   239,   237,   236,   235,   230,
     229,   233,   234,   232,   231,   228,   227,   225,   226,   222,
     223,   224,    15,    26,    24,    50,    48,    44,    20,    21,
      35,    34,    53,    57,     0,     0,   348,   349,     0,   344,
     342,     0,   298,   290,     0,   298,     0,     0,    86,     0,
       0,   186,   187,     0,   204,   207,   213,     0,   103,    94,
      97,     0,    83,     0,     0,     0,     0,   338,    41,     0,
     250,   256,     0,     0,   254,     0,   241,   218,   245,   244,
     246,   247,     0,     0,   261,   262,   249,     0,   248,     0,
      58,   365,   362,   355,   345,   343,     0,     0,   298,     0,
     266,   108,   305,     0,   306,   288,   323,   324,     0,   199,
       0,     0,   197,     0,     0,    91,     0,     0,   101,   179,
     180,   181,     0,     0,     0,     0,     0,     0,     0,     0,
     238,   366,     0,     0,     0,   292,   293,   294,   295,   296,
     299,     0,     0,     0,     0,   301,     0,   268,     0,   304,
     307,   266,     0,   327,     0,   321,     0,   200,   196,   198,
       0,   183,   192,   100,     0,     0,   112,   251,   252,   253,
     255,   258,   259,   260,   358,     0,   365,   297,     0,   300,
       0,     0,   270,   291,   270,   108,     0,   324,     0,     0,
      77,   214,     0,   104,     0,   350,     0,   298,     0,     0,
       0,   276,   282,     0,     0,   325,     0,   322,   193,     0,
     190,   102,   356,     0,     0,   265,     0,     0,   274,     0,
     289,   328,   324,   214,     0,   302,   267,     0,   271,   272,
     273,     0,   283,   326,   194,     0,   269,   277,   312,   298,
     157,     0,     0,   134,   115,   114,   159,   160,   161,   162,
     163,     0,     0,     0,     0,     0,     0,   144,   146,   151,
       0,     0,     0,   145,     0,   116,     0,     0,   140,   148,
     156,   158,     0,     0,     0,   313,   284,   303,     0,     0,
       0,     0,   153,   214,     0,   141,     0,     0,   113,     0,
     133,   183,     0,   135,     0,     0,   155,   278,   214,   143,
       0,   309,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   157,     0,   164,     0,     0,   127,     0,   131,
       0,     0,   136,     0,   183,   183,     0,   309,     0,   157,
       0,   264,     0,     0,   147,     0,   118,     0,     0,   119,
     120,   126,     0,   150,     0,   113,     0,     0,   129,     0,
     130,   132,   138,   137,   183,   264,   149,     0,   308,     0,
     310,     0,     0,     0,     0,     0,   154,     0,   142,   128,
     117,   139,   310,   314,     0,   166,     0,     0,     0,     0,
       0,     0,   166,   310,   263,   214,     0,   285,   122,   121,
       0,   123,   124,     0,   279,   166,   165,   311,   183,   125,
     152,   183,   315,   286,   280,   183,   316
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,   124,    11,    12,     9,    10,    19,    91,   248,
     184,   183,   181,   192,   193,   194,   308,     7,     8,    18,
      59,   137,   215,   237,   447,   555,   507,    60,   209,   326,
     142,   655,   656,   708,   657,   710,   681,   658,   659,   706,
     660,   674,   702,   661,   662,   663,   703,   777,   116,   146,
      62,   713,    63,   218,   219,   220,   335,   441,   551,   600,
     440,   501,   502,    64,    65,   230,    66,   231,    67,   233,
     704,   207,   253,   750,   537,   572,   591,   621,   327,   432,
     608,   630,   717,   791,   434,   609,   628,   691,   788,   435,
     542,   491,   531,   489,   490,   494,   541,   721,   765,   631,
     690,   773,   795,    68,   210,   330,   436,   579,   497,   545,
     577,    15,    16,    26,    27,   104,    13,    14,    69,    70,
      23,    24,   431,    98,    99,   524,   425,   522
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -709
static const yytype_int16 yypact[] =
{
     309,  -709,  -709,  -709,  -709,  -709,    40,  -709,  -709,  -709,
    -709,  -709,    80,  -709,   -10,  -709,    53,  -709,   837,  1623,
     108,   109,    70,   -10,  -709,   115,    53,  -709,   475,    92,
     140,   248,   150,  -709,   195,  -709,   235,   187,   219,   225,
     228,   240,   252,   257,   272,   282,   285,  -709,  -709,   290,
     294,  -709,   306,   307,   312,   314,  -709,   315,  -709,  -709,
    -709,  -709,   184,  -709,  -709,  -709,  -709,  -709,  -709,  -709,
     211,  -709,   281,   235,   346,   682,  -709,   350,   352,   365,
    -709,  -709,   368,   375,   382,   682,   387,   391,   398,  -709,
     399,   292,   682,  -709,   403,  -709,   392,   396,   356,   249,
     109,  -709,  -709,  -709,   378,   287,  -709,  -709,  -709,  -709,
    -709,  -709,  -709,  -709,  -709,  -709,  -709,  -709,  -709,   434,
     436,   437,   438,  -709,  -709,    72,   439,   442,   443,   235,
     235,   444,   235,    16,  -709,   445,   172,   414,   235,   448,
     452,   454,   429,  -709,  -709,  -709,  -709,   409,    25,  -709,
      28,  -709,  -709,   682,   682,   682,   431,   432,   433,   440,
     441,  -709,   455,   456,   457,   460,   461,   462,   463,   465,
     467,   468,   469,   470,   472,   474,   682,   682,  1405,   379,
    -709,   326,  -709,   328,    33,  -709,  -709,   514,  1805,   330,
    -709,  -709,   334,  -709,   487,  -709,  -709,  1805,   458,   115,
     115,   369,   242,   464,   373,   242,  -709,   682,  -709,   889,
      50,  -709,  -709,    78,   380,  -709,  -709,   235,   466,    -8,
    -709,   376,   381,   384,   386,   388,   389,   390,  -709,  -709,
     107,   125,    49,   393,  -709,  -709,   489,    19,   172,   394,
     528,   532,   537,   682,   400,   -10,   682,   682,  -709,   682,
     682,  -709,  -709,   790,   682,   682,   682,   682,   682,   542,
     544,   682,   545,   546,   564,   565,   682,   682,   567,   572,
     682,   682,   574,  -709,  -709,   682,   682,   682,   682,   682,
     682,   682,   682,   682,   682,   682,   682,   682,   682,   682,
     682,   682,   682,   682,   682,   682,   682,  1805,   577,   578,
    -709,   580,   682,   682,  1805,   336,   582,  -709,    77,  -709,
     446,   447,  -709,  -709,   583,  -709,  -709,  -709,    -6,  -709,
    1805,   475,  -709,   235,  -709,  -709,  -709,  -709,  -709,  -709,
    -709,   584,  -709,  -709,   913,   553,  -709,  -709,  -709,    72,
     599,  -709,  -709,  -709,  -709,  -709,  -709,  -709,   235,  -709,
     235,   445,  -709,  -709,  -709,  -709,  -709,   554,   180,  -709,
      22,  -709,  -709,  -709,  -709,  1436,  -709,    59,  1805,  1805,
    1646,  1805,  1805,  -709,   841,  1043,  1456,  1476,  1063,   477,
     459,  1083,   479,   480,   485,   486,  1496,  1534,   491,   492,
    1103,  1565,   493,  1675,  1822,  1514,  1023,  1836,  1849,   530,
     530,   587,   587,   587,   587,   332,   332,   259,   259,  -709,
    -709,  -709,  1805,  1805,  1805,  -709,  -709,  -709,  1805,  1805,
    -709,  -709,  -709,  -709,   601,   115,   267,   242,   550,  -709,
    -709,     8,     5,  -709,   598,     5,   682,   496,  -709,     3,
     597,    72,  -709,   495,  -709,  -709,  -709,   172,  -709,  -709,
    -709,   606,  -709,   497,   505,   508,   620,  -709,  -709,   682,
    -709,  -709,   682,   682,  -709,   682,  -709,  -709,  -709,  -709,
    -709,  -709,   682,   682,  -709,  -709,  -709,   630,  -709,   682,
    -709,   507,   640,  -709,  -709,  -709,   230,   623,  1762,   645,
     560,  -709,  -709,  1785,   575,  -709,  1805,    29,   662,  -709,
     664,     2,  -709,   576,   634,  -709,    90,   172,  -709,  -709,
    -709,  -709,   524,  1123,  1143,  1174,  1194,  1214,  1234,   525,
    1805,   242,   615,   115,   115,  -709,  -709,  -709,  -709,  -709,
    -709,   529,   682,   264,   659,  -709,   652,   639,   506,  -709,
    -709,   560,   632,   655,   657,  -709,   548,  -709,  -709,  -709,
     690,   552,  -709,  -709,   119,   172,  -709,  -709,  -709,  -709,
    -709,  -709,  -709,  -709,  -709,   555,   507,  -709,  1254,  -709,
     682,   661,   600,  -709,   600,  -709,   682,    29,   682,   561,
    -709,  -709,   611,  -709,   129,   242,   649,   236,  1274,   682,
     673,  -709,  -709,  1566,  1305,  -709,  1325,  -709,  -709,   705,
    -709,  -709,  -709,   677,   701,  -709,  1345,   682,   168,   660,
    -709,  -709,    29,  -709,   682,  -709,  -709,  1365,  -709,  -709,
    -709,   665,  -709,  -709,  -709,  1385,  -709,  -709,  -709,   685,
     740,    71,   708,   413,  -709,  -709,  -709,  -709,  -709,  -709,
    -709,   688,   689,   691,   692,   235,   694,  -709,  -709,  -709,
     695,   699,   700,  -709,    88,  -709,   702,    51,  -709,  -709,
    -709,   740,   668,   703,   184,  -709,  -709,  -709,   313,   374,
      35,    35,  -709,  -709,   706,  -709,   734,    35,  -709,   709,
    -709,   -52,    88,   713,    88,   714,  -709,  -709,  -709,  -709,
     696,   727,   716,   719,   612,   721,   613,   724,   732,   624,
     626,   627,   740,   629,  -709,   682,    17,  -709,     4,  -709,
      14,   338,  -709,    88,   141,   106,    88,   727,   631,   740,
     766,   678,    35,    35,  -709,    35,  -709,    35,    35,  -709,
    -709,  -709,   743,  -709,  1585,   641,   642,   782,  -709,    35,
    -709,  -709,  -709,  -709,   173,   678,  -709,   728,  -709,   765,
    -709,   644,   651,    15,   653,   654,  -709,   788,  -709,  -709,
    -709,  -709,  -709,  -709,   798,   234,   656,   679,    35,   680,
     681,   683,   234,  -709,  -709,  -709,   800,  -709,  -709,  -709,
     684,  -709,  -709,   184,  -709,   234,  -709,  -709,   552,  -709,
    -709,   552,  -709,  -709,  -709,   552,  -709
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -709,  -709,   -70,  -709,  -709,  -709,  -709,   581,  -709,  -709,
    -709,  -709,  -709,  -709,   704,  -709,  -709,  -709,  -709,   622,
    -709,  -709,  -709,  -223,  -709,  -709,  -709,  -709,  -439,   -13,
    -709,   -23,  -358,  -709,  -709,   110,  -453,   132,  -709,  -709,
     178,  -709,  -709,  -709,  -620,  -709,    67,  -708,  -709,  -642,
    -592,  -218,  -709,   402,  -709,   509,  -709,  -709,  -709,  -709,
    -709,  -709,   348,  -709,  -709,  -709,  -709,  -709,  -709,  -191,
    -105,  -709,   -75,   105,   310,  -709,   278,  -709,  -709,  -709,
    -709,  -709,  -709,  -709,  -709,  -709,  -709,  -709,  -709,  -709,
    -709,  -709,  -709,  -472,   418,  -709,  -709,   137,  -627,  -709,
    -709,  -709,  -709,  -709,  -709,  -709,  -709,  -709,  -709,  -516,
    -709,  -709,  -709,  -709,   829,  -709,  -709,  -709,  -709,  -709,
     633,   -19,  -709,   776,   -11,  -709,  -709,   321
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -338
static const yytype_int16 yytable[] =
{
     178,   339,   206,   149,   101,    61,   499,   499,   151,   152,
     188,   208,   348,   350,   105,   360,   534,   197,   678,   678,
     123,   735,   689,   355,   356,   737,   355,   356,  -185,   634,
     634,   246,   634,   543,   249,   153,   154,   300,   664,   678,
      17,   352,   486,   156,   157,   487,   635,   635,    21,   635,
     634,  -185,   538,   232,   328,   683,   158,   159,   160,   224,
     225,   595,   227,   229,   784,   161,   634,   635,   239,   664,
     162,   679,   642,   643,   644,   665,   216,   792,   251,   252,
     163,   423,   732,   635,    20,   164,   165,   166,   167,   168,
     169,   170,   678,   338,   355,   356,   623,   712,   171,   747,
     172,   273,   274,   634,   297,   679,   642,   643,   644,   329,
     664,    25,   304,    93,    92,   604,   173,    21,   457,    93,
     635,    22,   174,   355,   356,   429,   357,   664,   100,   357,
     666,   544,   320,   355,   356,   772,   593,   338,   217,   484,
     430,   790,   679,   642,   643,   644,   785,   336,   175,   548,
     117,   738,   500,   500,   485,   176,   177,   632,   651,   652,
     446,   739,   768,   228,   358,   654,   359,   358,   365,   452,
     247,   368,   369,   250,   371,   372,   234,   235,   301,   374,
     375,   376,   377,   378,   449,   450,   381,   121,   310,   311,
      22,   386,   387,   652,   351,   390,   391,   357,   118,   684,
     393,   394,   395,   396,   397,   398,   399,   400,   401,   402,
     403,   404,   405,   406,   407,   408,   409,   410,   411,   412,
     413,   414,   424,   331,   506,   332,   357,   418,   419,   714,
     652,   715,   122,   151,   152,   358,   357,   553,    94,   123,
     775,    95,    96,    97,    94,   125,   313,    95,   102,   103,
     776,   338,   338,   433,   347,   743,   126,   453,   454,   455,
     153,   154,   127,   744,   358,   128,   583,   155,   156,   157,
     338,   313,   349,   533,   358,   603,   601,   129,   444,   236,
     445,   158,   159,   160,   554,   148,   338,   451,   742,   130,
     161,   291,   292,   293,   131,   162,   680,   119,   120,   685,
     525,   526,   527,   528,   529,   163,   618,   619,   620,   132,
     164,   165,   166,   167,   168,   169,   170,   678,   338,   133,
     761,    61,   134,   171,   680,   172,   680,   135,   634,   144,
     145,   136,   584,   582,   525,   526,   527,   528,   529,   420,
     421,   173,   678,   138,   139,   635,   147,   174,   101,   140,
     150,   141,   143,   634,   179,   741,   180,   488,   680,   493,
     488,   496,   289,   290,   291,   292,   293,   692,   693,   182,
     635,   314,   185,   175,   315,   316,   317,   530,   678,   186,
     176,   177,   151,   152,   513,   295,   187,   514,   515,   634,
     516,   189,   692,   693,   190,   202,   314,   517,   518,   315,
     316,   482,   191,   195,   520,   196,   635,   198,   199,   153,
     154,   530,   200,   694,   481,   201,   155,   156,   157,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   697,   698,
     158,   159,   160,   205,     1,     2,     3,   204,   211,   161,
     212,   213,   214,   221,   162,     4,   222,   223,   226,   232,
    -113,   238,   240,     5,   163,   695,   241,   568,   242,   164,
     165,   166,   167,   168,   169,   170,   243,   245,   254,   255,
     256,   298,   171,   299,   172,   305,   598,   257,   258,   306,
     695,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     173,   307,   259,   260,   261,   588,   174,   262,   263,   264,
     265,   594,   266,   596,   267,   268,   269,   270,   624,   271,
     321,   272,   565,   566,   606,   312,   309,   151,   152,   319,
     302,   340,   175,   318,   296,   337,   354,   333,   341,   176,
     177,   342,   617,   343,   362,   344,   345,   346,   363,   625,
     353,   361,   573,   364,   153,   154,   379,   366,   380,   382,
     383,   155,   156,   157,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   158,   159,   160,   384,   385,
     793,   388,   323,   794,   161,   672,   389,   796,   392,   162,
     324,   415,   416,   718,   417,    43,   422,   428,   437,   163,
     439,   448,   426,   427,   164,   165,   166,   167,   168,   169,
     170,   151,   152,   443,   325,   480,   466,   171,   483,   172,
      53,    54,    55,   503,   492,   287,   288,   289,   290,   291,
     292,   293,   465,    56,   512,   173,   468,   469,   153,   154,
     734,   174,   470,   471,   519,   155,   156,   157,   474,   475,
     478,   498,   505,   508,   509,   696,   699,   700,   701,   158,
     159,   160,   510,   521,   709,   511,   523,   175,   161,   303,
     532,   535,   536,   162,   176,   177,   546,   540,   547,   550,
     552,   556,   563,   163,   564,   569,   567,   571,   164,   165,
     166,   167,   168,   169,   170,   151,   152,   740,   696,   570,
     575,   171,   576,   172,   578,   580,   581,   338,   589,   751,
     752,   585,   709,   590,   754,   755,   599,   597,   602,   173,
     607,   613,   153,   154,   614,   174,   760,   615,   622,   155,
     156,   157,   533,   627,   667,   668,   669,   687,   670,   671,
     740,   673,   675,   158,   159,   160,   676,   677,   707,   682,
     688,   175,   161,   705,   633,   780,   711,   162,   176,   177,
    -113,   716,   720,   722,   719,   634,   723,   163,   725,   724,
     726,   727,   164,   165,   166,   167,   168,   169,   170,   728,
     748,   729,   635,   730,   731,   171,   733,   172,   746,   756,
     749,   636,   637,   638,   639,   640,   759,   763,  -134,   758,
     764,   766,   771,   173,   641,   642,   643,   644,   767,   174,
     769,   770,   774,   778,   787,   275,   645,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   175,   779,   781,   782,   370,
     783,   789,   176,   177,   646,   753,   647,   334,   736,   686,
     648,    28,   786,   504,    53,    54,    55,   244,   442,   549,
     762,   574,   592,   495,   745,   106,   275,   649,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   203,   650,   367,     0,
       0,   651,   652,    29,    30,    31,   653,   586,   654,     0,
       0,     0,     0,   321,     0,     0,     0,     0,    32,    33,
      34,    35,     0,    36,    37,    38,    39,     0,     0,     0,
       0,     0,     0,    40,    41,    42,    43,    28,     0,     0,
       0,     0,     0,     0,    44,    45,    46,    47,    48,    49,
       0,     0,     0,     0,    50,    51,    52,   373,     0,     0,
       0,    53,    54,    55,     0,     0,     0,     0,   322,   438,
       0,     0,     0,     0,    56,   323,     0,     0,     0,    29,
      30,    31,     0,   324,     0,     0,    57,     0,    43,     0,
       0,     0,  -337,     0,    32,    33,    34,    35,     0,    36,
      37,    38,    39,    58,     0,     0,   459,   325,   460,    40,
      41,    42,    43,    53,    54,    55,     0,     0,     0,     0,
      44,    45,    46,    47,    48,    49,    56,     0,     0,     0,
      50,    51,    52,     0,     0,     0,     0,    53,    54,    55,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      56,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    57,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,    58,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,     0,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,     0,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,     0,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,     0,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   275,     0,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   275,
     461,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
     464,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
     467,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
     476,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
     557,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
     558,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     275,   559,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,   560,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,   561,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,   562,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,   587,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,   605,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   275,   611,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   275,   612,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   275,   616,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   275,   626,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,     0,   629,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   275,
     294,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,     0,
     321,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     275,   456,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,   462,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
       0,   463,     0,     0,     0,   610,     0,    71,     0,     0,
       0,     0,   323,     0,     0,     0,     0,     0,     0,     0,
     324,   472,     0,     0,     0,    43,     0,     0,     0,     0,
      71,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    72,     0,     0,   325,     0,     0,     0,     0,     0,
      53,    54,    55,     0,     0,     0,     0,     0,     0,   473,
       0,     0,   458,    56,    72,     0,     0,     0,     0,    73,
     275,   479,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     477,     0,    73,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    74,     0,     0,     0,     0,     0,
     757,    75,    76,    77,    78,    79,   -43,    80,    81,    82,
       0,    83,    84,     0,    85,    86,    87,    74,     0,     0,
       0,    88,    89,    90,    75,    76,    77,    78,    79,     0,
      80,    81,    82,     0,    83,    84,     0,    85,    86,    87,
       0,     0,     0,     0,    88,    89,    90,   275,     0,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,     0,     0,   533,
     275,   539,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     275,     0,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293
};

static const yytype_int16 yycheck[] =
{
      75,   219,   107,    73,    23,    18,     4,     4,     3,     4,
      85,   116,   230,   231,    25,   238,   488,    92,     4,     4,
       4,     4,   664,     4,     5,    21,     4,     5,    36,    15,
      15,     6,    15,     4,     6,    30,    31,     4,   630,     4,
       0,   232,    37,    38,    39,    40,    32,    32,    58,    32,
      15,    59,   491,     4,     4,     4,    51,    52,    53,   129,
     130,   577,   132,   133,   772,    60,    15,    32,   138,   661,
      65,    54,    55,    56,    57,     4,     4,   785,   153,   154,
      75,     4,   702,    32,     4,    80,    81,    82,    83,    84,
      85,    86,     4,   145,     4,     5,   612,   149,    93,   719,
      95,   176,   177,    15,   179,    54,    55,    56,    57,    59,
     702,    58,   187,     4,     6,   587,   111,    58,    59,     4,
      32,   131,   117,     4,     5,   131,   107,   719,    58,   107,
      59,   102,   207,     4,     5,   762,   575,   145,    66,   131,
     146,   783,    54,    55,    56,    57,   773,   217,   143,   147,
      58,   147,   150,   150,   146,   150,   151,   629,   141,   142,
     351,   147,   147,   147,   145,   148,   147,   145,   243,   147,
     145,   246,   247,   145,   249,   250,     4,     5,   145,   254,
     255,   256,   257,   258,     4,     5,   261,    37,   199,   200,
     131,   266,   267,   142,   145,   270,   271,   107,    58,   148,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   145,   145,   447,   147,   107,   302,   303,   682,
     142,   684,    37,     3,     4,   145,   107,   147,   129,     4,
       6,   132,   133,   134,   129,    58,     4,   132,   133,   134,
      16,   145,   145,   323,   147,   149,    37,   362,   363,   364,
      30,    31,    37,   716,   145,    37,   147,    37,    38,    39,
     145,     4,   147,    37,   145,    39,   147,    37,   348,   107,
     350,    51,    52,    53,   507,     4,   145,   107,   147,    37,
      60,    32,    33,    34,    37,    65,   654,    49,    50,   657,
      70,    71,    72,    73,    74,    75,   138,   139,   140,    37,
      80,    81,    82,    83,    84,    85,    86,     4,   145,    37,
     147,   334,    37,    93,   682,    95,   684,    37,    15,   145,
     146,    37,   555,   551,    70,    71,    72,    73,    74,     3,
       4,   111,     4,    37,    37,    32,   135,   117,   367,    37,
       4,    37,    37,    15,     4,   713,     4,   432,   716,   434,
     435,   436,    30,    31,    32,    33,    34,    54,    55,     4,
      32,   129,     4,   143,   132,   133,   134,   147,     4,     4,
     150,   151,     3,     4,   459,     6,     4,   462,   463,    15,
     465,     4,    54,    55,     3,   146,   129,   472,   473,   132,
     133,   134,     4,     4,   479,   113,    32,     4,    16,    30,
      31,   147,    16,   100,   425,    59,    37,    38,    39,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    54,    55,
      51,    52,    53,   146,   125,   126,   127,    59,     4,    60,
       4,     4,     4,     4,    65,   136,     4,     4,     4,     4,
      37,    37,     4,   144,    75,   142,     4,   532,     4,    80,
      81,    82,    83,    84,    85,    86,    37,    58,    37,    37,
      37,   145,    93,   145,    95,   145,   581,    37,    37,   145,
     142,     6,     7,     8,     9,    10,    11,    12,    13,    14,
     111,     4,    37,    37,    37,   570,   117,    37,    37,    37,
      37,   576,    37,   578,    37,    37,    37,    37,   613,    37,
       4,    37,   523,   524,   589,   146,    58,     3,     4,   146,
       6,   145,   143,    59,   145,    59,    37,   147,   147,   150,
     151,   147,   607,   147,     6,   147,   147,   147,     6,   614,
     147,   147,    36,     6,    30,    31,     4,   147,     4,     4,
       4,    37,    38,    39,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    51,    52,    53,     4,     4,
     788,     4,    66,   791,    60,   645,     4,   795,     4,    65,
      74,     4,     4,   688,     4,    79,     4,     4,     4,    75,
      37,    37,   146,   146,    80,    81,    82,    83,    84,    85,
      86,     3,     4,     4,    98,     4,   147,    93,    58,    95,
     104,   105,   106,    16,    16,    28,    29,    30,    31,    32,
      33,    34,   145,   117,     4,   111,   147,   147,    30,    31,
     705,   117,   147,   147,     4,    37,    38,    39,   147,   147,
     147,   145,   147,    37,   147,   668,   669,   670,   671,    51,
      52,    53,   147,   146,   677,   147,    16,   143,    60,   145,
      37,    16,   102,    65,   150,   151,     4,    92,     4,    93,
      36,   147,   147,    75,    59,    16,   147,    38,    80,    81,
      82,    83,    84,    85,    86,     3,     4,   710,   711,    37,
      58,    93,    37,    95,    37,   147,     6,   145,    37,   722,
     723,   146,   725,   103,   727,   728,    95,   146,    59,   111,
      37,     6,    30,    31,    37,   117,   739,    16,    58,    37,
      38,    39,    37,    58,    16,    37,    37,    59,    37,    37,
     753,    37,    37,    51,    52,    53,    37,    37,     4,    37,
      37,   143,    60,    37,     4,   768,    37,    65,   150,   151,
      37,    37,    25,    37,    58,    15,    37,    75,    37,   147,
     147,    37,    80,    81,    82,    83,    84,    85,    86,    37,
       4,   147,    32,   147,   147,    93,   147,    95,   147,    36,
     102,    41,    42,    43,    44,    45,     4,    59,   147,   147,
      25,   147,     4,   111,    54,    55,    56,    57,   147,   117,
     147,   147,     4,   147,     4,    15,    66,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,   143,   147,   147,   147,   248,
     147,   147,   150,   151,    94,   725,    96,   215,   706,   661,
     100,     4,   775,   441,   104,   105,   106,   143,   339,   501,
     745,   541,   574,   435,   717,    26,    15,   117,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,   100,   137,   245,    -1,
      -1,   141,   142,    46,    47,    48,   146,   566,   148,    -1,
      -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,    61,    62,
      63,    64,    -1,    66,    67,    68,    69,    -1,    -1,    -1,
      -1,    -1,    -1,    76,    77,    78,    79,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    97,    98,    99,   147,    -1,    -1,
      -1,   104,   105,   106,    -1,    -1,    -1,    -1,    59,    36,
      -1,    -1,    -1,    -1,   117,    66,    -1,    -1,    -1,    46,
      47,    48,    -1,    74,    -1,    -1,   129,    -1,    79,    -1,
      -1,    -1,   135,    -1,    61,    62,    63,    64,    -1,    66,
      67,    68,    69,   146,    -1,    -1,   145,    98,   147,    76,
      77,    78,    79,   104,   105,   106,    -1,    -1,    -1,    -1,
      87,    88,    89,    90,    91,    92,   117,    -1,    -1,    -1,
      97,    98,    99,    -1,    -1,    -1,    -1,   104,   105,   106,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     117,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   129,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   146,
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
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,
     147,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     147,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     147,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     147,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     147,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     147,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      15,   147,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   147,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   147,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   147,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   147,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   147,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    15,   147,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   147,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   147,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   147,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,   147,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     145,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
       4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      15,   145,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   145,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,   145,    -1,    -1,    -1,    59,    -1,     4,    -1,    -1,
      -1,    -1,    66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      74,   145,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,
       4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    98,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,    -1,    -1,    -1,    -1,    -1,    -1,   145,
      -1,    -1,    36,   117,    38,    -1,    -1,    -1,    -1,    66,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
     145,    -1,    66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   101,    -1,    -1,    -1,    -1,    -1,
     145,   108,   109,   110,   111,   112,   113,   114,   115,   116,
      -1,   118,   119,    -1,   121,   122,   123,   101,    -1,    -1,
      -1,   128,   129,   130,   108,   109,   110,   111,   112,    -1,
     114,   115,   116,    -1,   118,   119,    -1,   121,   122,   123,
      -1,    -1,    -1,    -1,   128,   129,   130,    15,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    37,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   125,   126,   127,   136,   144,   153,   169,   170,   157,
     158,   155,   156,   268,   269,   263,   264,     0,   171,   159,
       4,    58,   131,   272,   273,    58,   265,   266,     4,    46,
      47,    48,    61,    62,    63,    64,    66,    67,    68,    69,
      76,    77,    78,    79,    87,    88,    89,    90,    91,    92,
      97,    98,    99,   104,   105,   106,   117,   129,   146,   172,
     179,   181,   202,   204,   215,   216,   218,   220,   255,   270,
     271,     4,    38,    66,   101,   108,   109,   110,   111,   112,
     114,   115,   116,   118,   119,   121,   122,   123,   128,   129,
     130,   160,     6,     4,   129,   132,   133,   134,   275,   276,
      58,   273,   133,   134,   267,   276,   266,     6,     7,     8,
       9,    10,    11,    12,    13,    14,   200,    58,    58,    49,
      50,    37,    37,     4,   154,    58,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,   173,    37,    37,
      37,    37,   182,    37,   145,   146,   201,   135,     4,   154,
       4,     3,     4,    30,    31,    37,    38,    39,    51,    52,
      53,    60,    65,    75,    80,    81,    82,    83,    84,    85,
      86,    93,    95,   111,   117,   143,   150,   151,   224,     4,
       4,   164,     4,   163,   162,     4,     4,     4,   224,     4,
       3,     4,   165,   166,   167,     4,   113,   224,     4,    16,
      16,    59,   146,   275,    59,   146,   222,   223,   222,   180,
     256,     4,     4,     4,     4,   174,     4,    66,   205,   206,
     207,     4,     4,     4,   154,   154,     4,   154,   147,   154,
     217,   219,     4,   221,     4,     5,   107,   175,    37,   154,
       4,     4,     4,    37,   166,    58,     6,   145,   161,     6,
     145,   224,   224,   224,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,   224,   224,    15,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,   145,     6,   145,   224,   145,   145,
       4,   145,     6,   145,   224,   145,   145,     4,   168,    58,
     276,   276,   146,     4,   129,   132,   133,   134,    59,   146,
     224,     4,    59,    66,    74,    98,   181,   230,     4,    59,
     257,   145,   147,   147,   171,   208,   154,    59,   145,   203,
     145,   147,   147,   147,   147,   147,   147,   147,   203,   147,
     203,   145,   221,   147,    37,     4,     5,   107,   145,   147,
     175,   147,     6,     6,     6,   224,   147,   272,   224,   224,
     159,   224,   224,   147,   224,   224,   224,   224,   224,     4,
       4,   224,     4,     4,     4,     4,   224,   224,     4,     4,
     224,   224,     4,   224,   224,   224,   224,   224,   224,   224,
     224,   224,   224,   224,   224,   224,   224,   224,   224,   224,
     224,   224,   224,   224,   224,     4,     4,     4,   224,   224,
       3,     4,     4,     4,   145,   278,   146,   146,     4,   131,
     146,   274,   231,   154,   236,   241,   258,     4,    36,    37,
     212,   209,   207,     4,   154,   154,   221,   176,    37,     4,
       5,   107,   147,   222,   222,   222,   145,    59,    36,   145,
     147,   147,   145,   145,   147,   145,   147,   147,   147,   147,
     147,   147,   145,   145,   147,   147,   147,   145,   147,    16,
       4,   276,   134,    58,   131,   146,    37,    40,   224,   245,
     246,   243,    16,   224,   247,   246,   224,   260,   145,     4,
     150,   213,   214,    16,   205,   147,   175,   178,    37,   147,
     147,   147,     4,   224,   224,   224,   224,   224,   224,     4,
     224,   146,   279,    16,   277,    70,    71,    72,    73,    74,
     147,   244,    37,    37,   245,    16,   102,   226,   180,    16,
      92,   248,   242,     4,   102,   261,     4,     4,   147,   214,
      93,   210,    36,   147,   175,   177,   147,   147,   147,   147,
     147,   147,   147,   147,    59,   276,   276,   147,   224,    16,
      37,    38,   227,    36,   226,    58,    37,   262,    37,   259,
     147,     6,   203,   147,   175,   146,   279,   147,   224,    37,
     103,   228,   228,   180,   224,   261,   224,   146,   222,    95,
     211,   147,    59,    39,   245,   147,   224,    37,   232,   237,
      59,   147,   147,     6,    37,    16,   147,   224,   138,   139,
     140,   229,    58,   261,   222,   224,   147,    58,   238,   147,
     233,   251,   245,     4,    15,    32,    41,    42,    43,    44,
      45,    54,    55,    56,    57,    66,    94,    96,   100,   117,
     137,   141,   142,   146,   148,   183,   184,   186,   189,   190,
     192,   195,   196,   197,   202,     4,    59,    16,    37,    37,
      37,    37,   154,    37,   193,    37,    37,    37,     4,    54,
     184,   188,    37,     4,   148,   184,   192,    59,    37,   201,
     252,   239,    54,    55,   100,   142,   183,    54,    55,   183,
     183,   183,   194,   198,   222,    37,   191,     4,   185,   183,
     187,    37,   149,   203,   188,   188,    37,   234,   222,    58,
      25,   249,    37,    37,   147,    37,   147,    37,    37,   147,
     147,   147,   196,   147,   224,     4,   189,    21,   147,   147,
     183,   184,   147,   149,   188,   249,   147,   196,     4,   102,
     225,   183,   183,   187,   183,   183,    36,   145,   147,     4,
     183,   147,   225,    59,    25,   250,   147,   147,   147,   147,
     147,     4,   250,   253,     4,     6,    16,   199,   147,   147,
     183,   147,   147,   147,   199,   250,   198,     4,   240,   147,
     201,   235,   199,   203,   203,   254,   203
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

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
#ifndef	YYINITDEPTH
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
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

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

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
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

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

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
     `$$ = $1'.

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
#line 180 "ldgram.y"
    { ldlex_defsym(); }
    break;

  case 9:
#line 182 "ldgram.y"
    {
		  ldlex_popstate();
		  lang_add_assignment (exp_defsym ((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)));
		}
    break;

  case 10:
#line 190 "ldgram.y"
    {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
    break;

  case 11:
#line 195 "ldgram.y"
    {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
    break;

  case 16:
#line 210 "ldgram.y"
    {
			einfo(_("%P%F: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[(1) - (1)].name));
			}
    break;

  case 17:
#line 213 "ldgram.y"
    {
			config.map_filename = "-";
			}
    break;

  case 20:
#line 219 "ldgram.y"
    { mri_public((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)); }
    break;

  case 21:
#line 221 "ldgram.y"
    { mri_public((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)); }
    break;

  case 22:
#line 223 "ldgram.y"
    { mri_public((yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].etree)); }
    break;

  case 23:
#line 225 "ldgram.y"
    { mri_format((yyvsp[(2) - (2)].name)); }
    break;

  case 24:
#line 227 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree));}
    break;

  case 25:
#line 229 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].etree));}
    break;

  case 26:
#line 231 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree));}
    break;

  case 27:
#line 233 "ldgram.y"
    { mri_align((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 28:
#line 235 "ldgram.y"
    { mri_align((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 29:
#line 237 "ldgram.y"
    { mri_alignmod((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 30:
#line 239 "ldgram.y"
    { mri_alignmod((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 33:
#line 243 "ldgram.y"
    { mri_name((yyvsp[(2) - (2)].name)); }
    break;

  case 34:
#line 245 "ldgram.y"
    { mri_alias((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].name),0);}
    break;

  case 35:
#line 247 "ldgram.y"
    { mri_alias ((yyvsp[(2) - (4)].name), 0, (int) (yyvsp[(4) - (4)].bigint).integer); }
    break;

  case 36:
#line 249 "ldgram.y"
    { mri_base((yyvsp[(2) - (2)].etree)); }
    break;

  case 37:
#line 251 "ldgram.y"
    { mri_truncate ((unsigned int) (yyvsp[(2) - (2)].bigint).integer); }
    break;

  case 40:
#line 255 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 41:
#line 257 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 42:
#line 259 "ldgram.y"
    { lang_add_entry ((yyvsp[(2) - (2)].name), FALSE); }
    break;

  case 44:
#line 264 "ldgram.y"
    { mri_order((yyvsp[(3) - (3)].name)); }
    break;

  case 45:
#line 265 "ldgram.y"
    { mri_order((yyvsp[(2) - (2)].name)); }
    break;

  case 47:
#line 271 "ldgram.y"
    { mri_load((yyvsp[(1) - (1)].name)); }
    break;

  case 48:
#line 272 "ldgram.y"
    { mri_load((yyvsp[(3) - (3)].name)); }
    break;

  case 49:
#line 277 "ldgram.y"
    { mri_only_load((yyvsp[(1) - (1)].name)); }
    break;

  case 50:
#line 279 "ldgram.y"
    { mri_only_load((yyvsp[(3) - (3)].name)); }
    break;

  case 51:
#line 283 "ldgram.y"
    { (yyval.name) = NULL; }
    break;

  case 54:
#line 290 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 55:
#line 292 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 56:
#line 296 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(1) - (1)].name), FALSE); }
    break;

  case 57:
#line 298 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(2) - (2)].name), FALSE); }
    break;

  case 58:
#line 300 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(3) - (3)].name), FALSE); }
    break;

  case 59:
#line 304 "ldgram.y"
    { ldlex_both(); }
    break;

  case 60:
#line 306 "ldgram.y"
    { ldlex_popstate(); }
    break;

  case 73:
#line 327 "ldgram.y"
    { lang_add_target((yyvsp[(3) - (4)].name)); }
    break;

  case 74:
#line 329 "ldgram.y"
    { ldfile_add_library_path ((yyvsp[(3) - (4)].name), FALSE); }
    break;

  case 75:
#line 331 "ldgram.y"
    { lang_add_output((yyvsp[(3) - (4)].name), 1); }
    break;

  case 76:
#line 333 "ldgram.y"
    { lang_add_output_format ((yyvsp[(3) - (4)].name), (char *) NULL,
					    (char *) NULL, 1); }
    break;

  case 77:
#line 336 "ldgram.y"
    { lang_add_output_format ((yyvsp[(3) - (8)].name), (yyvsp[(5) - (8)].name), (yyvsp[(7) - (8)].name), 1); }
    break;

  case 78:
#line 338 "ldgram.y"
    { ldfile_set_output_arch ((yyvsp[(3) - (4)].name), bfd_arch_unknown); }
    break;

  case 79:
#line 340 "ldgram.y"
    { command_line.force_common_definition = TRUE ; }
    break;

  case 80:
#line 342 "ldgram.y"
    { command_line.inhibit_common_definition = TRUE ; }
    break;

  case 82:
#line 345 "ldgram.y"
    { lang_enter_group (); }
    break;

  case 83:
#line 347 "ldgram.y"
    { lang_leave_group (); }
    break;

  case 84:
#line 349 "ldgram.y"
    { lang_add_map((yyvsp[(3) - (4)].name)); }
    break;

  case 85:
#line 351 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 86:
#line 353 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 87:
#line 355 "ldgram.y"
    {
		  lang_add_nocrossref ((yyvsp[(3) - (4)].nocrossref));
		}
    break;

  case 89:
#line 360 "ldgram.y"
    { lang_add_insert ((yyvsp[(3) - (3)].name), 0); }
    break;

  case 90:
#line 362 "ldgram.y"
    { lang_add_insert ((yyvsp[(3) - (3)].name), 1); }
    break;

  case 91:
#line 364 "ldgram.y"
    { lang_memory_region_alias ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].name)); }
    break;

  case 92:
#line 366 "ldgram.y"
    { lang_ld_feature ((yyvsp[(3) - (4)].name)); }
    break;

  case 93:
#line 371 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 94:
#line 374 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 95:
#line 377 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 96:
#line 380 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 97:
#line 383 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 98:
#line 386 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 99:
#line 389 "ldgram.y"
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 100:
#line 392 "ldgram.y"
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[(3) - (5)].integer); }
    break;

  case 101:
#line 394 "ldgram.y"
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 102:
#line 397 "ldgram.y"
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[(5) - (7)].integer); }
    break;

  case 103:
#line 399 "ldgram.y"
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 104:
#line 402 "ldgram.y"
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[(4) - (6)].integer); }
    break;

  case 109:
#line 417 "ldgram.y"
    { lang_add_entry ((yyvsp[(3) - (4)].name), FALSE); }
    break;

  case 111:
#line 419 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 112:
#line 420 "ldgram.y"
    { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[(4) - (7)].etree), (yyvsp[(6) - (7)].name))); }
    break;

  case 113:
#line 428 "ldgram.y"
    {
			  (yyval.cname) = (yyvsp[(1) - (1)].name);
			}
    break;

  case 114:
#line 432 "ldgram.y"
    {
			  (yyval.cname) = "*";
			}
    break;

  case 115:
#line 436 "ldgram.y"
    {
			  (yyval.cname) = "?";
			}
    break;

  case 116:
#line 443 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(1) - (1)].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 117:
#line 450 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (5)].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[(3) - (5)].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 118:
#line 457 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 119:
#line 464 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 120:
#line 471 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 121:
#line 478 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_name_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 122:
#line 485 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 123:
#line 492 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_alignment_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 124:
#line 499 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 125:
#line 506 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(7) - (8)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = (yyvsp[(5) - (8)].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 126:
#line 513 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_init_priority;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 127:
#line 522 "ldgram.y"
    {
			  struct flag_info_list *n;
			  n = ((struct flag_info_list *) xmalloc (sizeof *n));
			  if ((yyvsp[(1) - (1)].name)[0] == '!')
			    {
			      n->with = without_flags;
			      n->name = &(yyvsp[(1) - (1)].name)[1];
			    }
			  else
			    {
			      n->with = with_flags;
			      n->name = (yyvsp[(1) - (1)].name);
			    }
			  n->valid = FALSE;
			  n->next = NULL;
			  (yyval.flag_info_list) = n;
			}
    break;

  case 128:
#line 540 "ldgram.y"
    {
			  struct flag_info_list *n;
			  n = ((struct flag_info_list *) xmalloc (sizeof *n));
			  if ((yyvsp[(3) - (3)].name)[0] == '!')
			    {
			      n->with = without_flags;
			      n->name = &(yyvsp[(3) - (3)].name)[1];
			    }
			  else
			    {
			      n->with = with_flags;
			      n->name = (yyvsp[(3) - (3)].name);
			    }
			  n->valid = FALSE;
			  n->next = (yyvsp[(1) - (3)].flag_info_list);
			  (yyval.flag_info_list) = n;
			}
    break;

  case 129:
#line 561 "ldgram.y"
    {
			  struct flag_info *n;
			  n = ((struct flag_info *) xmalloc (sizeof *n));
			  n->flag_list = (yyvsp[(3) - (4)].flag_info_list);
			  n->flags_initialized = FALSE;
			  n->not_with_flags = 0;
			  n->only_with_flags = 0;
			  (yyval.flag_info) = n;
			}
    break;

  case 130:
#line 574 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[(2) - (2)].cname);
			  tmp->next = (yyvsp[(1) - (2)].name_list);
			  (yyval.name_list) = tmp;
			}
    break;

  case 131:
#line 583 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[(1) - (1)].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
    break;

  case 132:
#line 594 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[(1) - (3)].wildcard_list);
			  tmp->spec = (yyvsp[(3) - (3)].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 133:
#line 603 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[(1) - (1)].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 134:
#line 614 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[(1) - (1)].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = NULL;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
    break;

  case 135:
#line 623 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[(2) - (2)].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[(1) - (2)].flag_info);
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
    break;

  case 136:
#line 632 "ldgram.y"
    {
			  lang_add_wild (NULL, (yyvsp[(2) - (3)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 137:
#line 636 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = NULL;
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[(1) - (4)].flag_info);
			  lang_add_wild (&tmp, (yyvsp[(3) - (4)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 138:
#line 645 "ldgram.y"
    {
			  lang_add_wild (&(yyvsp[(1) - (4)].wildcard), (yyvsp[(3) - (4)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 139:
#line 649 "ldgram.y"
    {
			  (yyvsp[(2) - (5)].wildcard).section_flag_list = (yyvsp[(1) - (5)].flag_info);
			  lang_add_wild (&(yyvsp[(2) - (5)].wildcard), (yyvsp[(4) - (5)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 141:
#line 658 "ldgram.y"
    { ldgram_had_keep = TRUE; }
    break;

  case 142:
#line 660 "ldgram.y"
    { ldgram_had_keep = FALSE; }
    break;

  case 144:
#line 666 "ldgram.y"
    {
 		lang_add_attribute(lang_object_symbols_statement_enum);
	      	}
    break;

  case 146:
#line 671 "ldgram.y"
    {

		  lang_add_attribute(lang_constructors_statement_enum);
		}
    break;

  case 147:
#line 676 "ldgram.y"
    {
		  constructors_sorted = TRUE;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
    break;

  case 149:
#line 682 "ldgram.y"
    {
			  lang_add_data ((int) (yyvsp[(1) - (4)].integer), (yyvsp[(3) - (4)].etree));
			}
    break;

  case 150:
#line 687 "ldgram.y"
    {
			  lang_add_fill ((yyvsp[(3) - (4)].fill));
			}
    break;

  case 151:
#line 690 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 152:
#line 691 "ldgram.y"
    { ldlex_popstate ();
			  lang_add_assignment (exp_assert ((yyvsp[(4) - (8)].etree), (yyvsp[(6) - (8)].name))); }
    break;

  case 153:
#line 694 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 154:
#line 696 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 159:
#line 711 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 160:
#line 713 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 161:
#line 715 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 162:
#line 717 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 163:
#line 719 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 164:
#line 724 "ldgram.y"
    {
		  (yyval.fill) = exp_get_fill ((yyvsp[(1) - (1)].etree), 0, "fill value");
		}
    break;

  case 165:
#line 731 "ldgram.y"
    { (yyval.fill) = (yyvsp[(2) - (2)].fill); }
    break;

  case 166:
#line 732 "ldgram.y"
    { (yyval.fill) = (fill_type *) 0; }
    break;

  case 167:
#line 737 "ldgram.y"
    { (yyval.token) = '+'; }
    break;

  case 168:
#line 739 "ldgram.y"
    { (yyval.token) = '-'; }
    break;

  case 169:
#line 741 "ldgram.y"
    { (yyval.token) = '*'; }
    break;

  case 170:
#line 743 "ldgram.y"
    { (yyval.token) = '/'; }
    break;

  case 171:
#line 745 "ldgram.y"
    { (yyval.token) = LSHIFT; }
    break;

  case 172:
#line 747 "ldgram.y"
    { (yyval.token) = RSHIFT; }
    break;

  case 173:
#line 749 "ldgram.y"
    { (yyval.token) = '&'; }
    break;

  case 174:
#line 751 "ldgram.y"
    { (yyval.token) = '|'; }
    break;

  case 177:
#line 761 "ldgram.y"
    {
		  lang_add_assignment (exp_assign ((yyvsp[(1) - (3)].name), (yyvsp[(3) - (3)].etree), FALSE));
		}
    break;

  case 178:
#line 765 "ldgram.y"
    {
		  lang_add_assignment (exp_assign ((yyvsp[(1) - (3)].name),
						   exp_binop ((yyvsp[(2) - (3)].token),
							      exp_nameop (NAME,
									  (yyvsp[(1) - (3)].name)),
							      (yyvsp[(3) - (3)].etree)), FALSE));
		}
    break;

  case 179:
#line 773 "ldgram.y"
    {
		  lang_add_assignment (exp_assign ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), TRUE));
		}
    break;

  case 180:
#line 777 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), FALSE));
		}
    break;

  case 181:
#line 781 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), TRUE));
		}
    break;

  case 189:
#line 804 "ldgram.y"
    { region = lang_memory_region_lookup ((yyvsp[(1) - (1)].name), TRUE); }
    break;

  case 190:
#line 807 "ldgram.y"
    {}
    break;

  case 191:
#line 809 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 192:
#line 811 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 193:
#line 816 "ldgram.y"
    {
		  region->origin = exp_get_vma ((yyvsp[(3) - (3)].etree), 0, "origin");
		  region->current = region->origin;
		}
    break;

  case 194:
#line 824 "ldgram.y"
    {
		  region->length = exp_get_vma ((yyvsp[(3) - (3)].etree), -1, "length");
		}
    break;

  case 195:
#line 831 "ldgram.y"
    { /* dummy action to avoid bison 1.25 error message */ }
    break;

  case 199:
#line 842 "ldgram.y"
    { lang_set_flags (region, (yyvsp[(1) - (1)].name), 0); }
    break;

  case 200:
#line 844 "ldgram.y"
    { lang_set_flags (region, (yyvsp[(2) - (2)].name), 1); }
    break;

  case 201:
#line 849 "ldgram.y"
    { lang_startup((yyvsp[(3) - (4)].name)); }
    break;

  case 203:
#line 855 "ldgram.y"
    { ldemul_hll((char *)NULL); }
    break;

  case 204:
#line 860 "ldgram.y"
    { ldemul_hll((yyvsp[(3) - (3)].name)); }
    break;

  case 205:
#line 862 "ldgram.y"
    { ldemul_hll((yyvsp[(1) - (1)].name)); }
    break;

  case 207:
#line 870 "ldgram.y"
    { ldemul_syslib((yyvsp[(3) - (3)].name)); }
    break;

  case 209:
#line 876 "ldgram.y"
    { lang_float(TRUE); }
    break;

  case 210:
#line 878 "ldgram.y"
    { lang_float(FALSE); }
    break;

  case 211:
#line 883 "ldgram.y"
    {
		  (yyval.nocrossref) = NULL;
		}
    break;

  case 212:
#line 887 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[(1) - (2)].name);
		  n->next = (yyvsp[(2) - (2)].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 213:
#line 896 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[(1) - (3)].name);
		  n->next = (yyvsp[(3) - (3)].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 214:
#line 906 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 215:
#line 908 "ldgram.y"
    { ldlex_popstate (); (yyval.etree)=(yyvsp[(2) - (2)].etree);}
    break;

  case 216:
#line 913 "ldgram.y"
    { (yyval.etree) = exp_unop ('-', (yyvsp[(2) - (2)].etree)); }
    break;

  case 217:
#line 915 "ldgram.y"
    { (yyval.etree) = (yyvsp[(2) - (3)].etree); }
    break;

  case 218:
#line 917 "ldgram.y"
    { (yyval.etree) = exp_unop ((int) (yyvsp[(1) - (4)].integer),(yyvsp[(3) - (4)].etree)); }
    break;

  case 219:
#line 919 "ldgram.y"
    { (yyval.etree) = exp_unop ('!', (yyvsp[(2) - (2)].etree)); }
    break;

  case 220:
#line 921 "ldgram.y"
    { (yyval.etree) = (yyvsp[(2) - (2)].etree); }
    break;

  case 221:
#line 923 "ldgram.y"
    { (yyval.etree) = exp_unop ('~', (yyvsp[(2) - (2)].etree));}
    break;

  case 222:
#line 926 "ldgram.y"
    { (yyval.etree) = exp_binop ('*', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 223:
#line 928 "ldgram.y"
    { (yyval.etree) = exp_binop ('/', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 224:
#line 930 "ldgram.y"
    { (yyval.etree) = exp_binop ('%', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 225:
#line 932 "ldgram.y"
    { (yyval.etree) = exp_binop ('+', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 226:
#line 934 "ldgram.y"
    { (yyval.etree) = exp_binop ('-' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 227:
#line 936 "ldgram.y"
    { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 228:
#line 938 "ldgram.y"
    { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 229:
#line 940 "ldgram.y"
    { (yyval.etree) = exp_binop (EQ , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 230:
#line 942 "ldgram.y"
    { (yyval.etree) = exp_binop (NE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 231:
#line 944 "ldgram.y"
    { (yyval.etree) = exp_binop (LE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 232:
#line 946 "ldgram.y"
    { (yyval.etree) = exp_binop (GE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 233:
#line 948 "ldgram.y"
    { (yyval.etree) = exp_binop ('<' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 234:
#line 950 "ldgram.y"
    { (yyval.etree) = exp_binop ('>' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 235:
#line 952 "ldgram.y"
    { (yyval.etree) = exp_binop ('&' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 236:
#line 954 "ldgram.y"
    { (yyval.etree) = exp_binop ('^' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 237:
#line 956 "ldgram.y"
    { (yyval.etree) = exp_binop ('|' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 238:
#line 958 "ldgram.y"
    { (yyval.etree) = exp_trinop ('?' , (yyvsp[(1) - (5)].etree), (yyvsp[(3) - (5)].etree), (yyvsp[(5) - (5)].etree)); }
    break;

  case 239:
#line 960 "ldgram.y"
    { (yyval.etree) = exp_binop (ANDAND , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 240:
#line 962 "ldgram.y"
    { (yyval.etree) = exp_binop (OROR , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 241:
#line 964 "ldgram.y"
    { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[(3) - (4)].name)); }
    break;

  case 242:
#line 966 "ldgram.y"
    { (yyval.etree) = exp_bigintop ((yyvsp[(1) - (1)].bigint).integer, (yyvsp[(1) - (1)].bigint).str); }
    break;

  case 243:
#line 968 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
    break;

  case 244:
#line 971 "ldgram.y"
    { (yyval.etree) = exp_nameop (ALIGNOF,(yyvsp[(3) - (4)].name)); }
    break;

  case 245:
#line 973 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF,(yyvsp[(3) - (4)].name)); }
    break;

  case 246:
#line 975 "ldgram.y"
    { (yyval.etree) = exp_nameop (ADDR,(yyvsp[(3) - (4)].name)); }
    break;

  case 247:
#line 977 "ldgram.y"
    { (yyval.etree) = exp_nameop (LOADADDR,(yyvsp[(3) - (4)].name)); }
    break;

  case 248:
#line 979 "ldgram.y"
    { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[(3) - (4)].name)); }
    break;

  case 249:
#line 981 "ldgram.y"
    { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[(3) - (4)].etree)); }
    break;

  case 250:
#line 983 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[(3) - (4)].etree)); }
    break;

  case 251:
#line 985 "ldgram.y"
    { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[(3) - (6)].etree),(yyvsp[(5) - (6)].etree)); }
    break;

  case 252:
#line 987 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree)); }
    break;

  case 253:
#line 989 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[(5) - (6)].etree), (yyvsp[(3) - (6)].etree)); }
    break;

  case 254:
#line 991 "ldgram.y"
    { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[(3) - (4)].etree)); }
    break;

  case 255:
#line 993 "ldgram.y"
    { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[(5) - (6)].etree),
					  exp_nameop (NAME, (yyvsp[(3) - (6)].name))); }
    break;

  case 256:
#line 1002 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[(3) - (4)].etree)); }
    break;

  case 257:
#line 1004 "ldgram.y"
    { (yyval.etree) = exp_nameop (NAME,(yyvsp[(1) - (1)].name)); }
    break;

  case 258:
#line 1006 "ldgram.y"
    { (yyval.etree) = exp_binop (MAX_K, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree) ); }
    break;

  case 259:
#line 1008 "ldgram.y"
    { (yyval.etree) = exp_binop (MIN_K, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree) ); }
    break;

  case 260:
#line 1010 "ldgram.y"
    { (yyval.etree) = exp_assert ((yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].name)); }
    break;

  case 261:
#line 1012 "ldgram.y"
    { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[(3) - (4)].name)); }
    break;

  case 262:
#line 1014 "ldgram.y"
    { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[(3) - (4)].name)); }
    break;

  case 263:
#line 1019 "ldgram.y"
    { (yyval.name) = (yyvsp[(3) - (3)].name); }
    break;

  case 264:
#line 1020 "ldgram.y"
    { (yyval.name) = 0; }
    break;

  case 265:
#line 1024 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 266:
#line 1025 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 267:
#line 1029 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 268:
#line 1030 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 269:
#line 1034 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 270:
#line 1035 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 271:
#line 1039 "ldgram.y"
    { (yyval.token) = ONLY_IF_RO; }
    break;

  case 272:
#line 1040 "ldgram.y"
    { (yyval.token) = ONLY_IF_RW; }
    break;

  case 273:
#line 1041 "ldgram.y"
    { (yyval.token) = SPECIAL; }
    break;

  case 274:
#line 1042 "ldgram.y"
    { (yyval.token) = 0; }
    break;

  case 275:
#line 1045 "ldgram.y"
    { ldlex_expression(); }
    break;

  case 276:
#line 1049 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 277:
#line 1052 "ldgram.y"
    {
			  lang_enter_output_section_statement((yyvsp[(1) - (9)].name), (yyvsp[(3) - (9)].etree),
							      sectype,
							      (yyvsp[(5) - (9)].etree), (yyvsp[(6) - (9)].etree), (yyvsp[(4) - (9)].etree), (yyvsp[(8) - (9)].token));
			}
    break;

  case 278:
#line 1058 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 279:
#line 1060 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_leave_output_section_statement ((yyvsp[(17) - (17)].fill), (yyvsp[(14) - (17)].name), (yyvsp[(16) - (17)].section_phdr), (yyvsp[(15) - (17)].name));
		}
    break;

  case 280:
#line 1065 "ldgram.y"
    {}
    break;

  case 281:
#line 1067 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 282:
#line 1069 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 283:
#line 1071 "ldgram.y"
    {
			  lang_enter_overlay ((yyvsp[(3) - (8)].etree), (yyvsp[(6) - (8)].etree));
			}
    break;

  case 284:
#line 1076 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 285:
#line 1078 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay ((yyvsp[(5) - (16)].etree), (int) (yyvsp[(4) - (16)].integer),
					      (yyvsp[(16) - (16)].fill), (yyvsp[(13) - (16)].name), (yyvsp[(15) - (16)].section_phdr), (yyvsp[(14) - (16)].name));
			}
    break;

  case 287:
#line 1088 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 288:
#line 1090 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assign (".", (yyvsp[(3) - (3)].etree), FALSE));
		}
    break;

  case 290:
#line 1096 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 291:
#line 1098 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 292:
#line 1102 "ldgram.y"
    { sectype = noload_section; }
    break;

  case 293:
#line 1103 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 294:
#line 1104 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 295:
#line 1105 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 296:
#line 1106 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 298:
#line 1111 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 299:
#line 1112 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 300:
#line 1116 "ldgram.y"
    { (yyval.etree) = (yyvsp[(1) - (3)].etree); }
    break;

  case 301:
#line 1117 "ldgram.y"
    { (yyval.etree) = (etree_type *)NULL;  }
    break;

  case 302:
#line 1122 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (6)].etree); }
    break;

  case 303:
#line 1124 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (10)].etree); }
    break;

  case 304:
#line 1128 "ldgram.y"
    { (yyval.etree) = (yyvsp[(1) - (2)].etree); }
    break;

  case 305:
#line 1129 "ldgram.y"
    { (yyval.etree) = (etree_type *) NULL;  }
    break;

  case 306:
#line 1134 "ldgram.y"
    { (yyval.integer) = 0; }
    break;

  case 307:
#line 1136 "ldgram.y"
    { (yyval.integer) = 1; }
    break;

  case 308:
#line 1141 "ldgram.y"
    { (yyval.name) = (yyvsp[(2) - (2)].name); }
    break;

  case 309:
#line 1142 "ldgram.y"
    { (yyval.name) = DEFAULT_MEMORY_REGION; }
    break;

  case 310:
#line 1147 "ldgram.y"
    {
		  (yyval.section_phdr) = NULL;
		}
    break;

  case 311:
#line 1151 "ldgram.y"
    {
		  struct lang_output_section_phdr_list *n;

		  n = ((struct lang_output_section_phdr_list *)
		       xmalloc (sizeof *n));
		  n->name = (yyvsp[(3) - (3)].name);
		  n->used = FALSE;
		  n->next = (yyvsp[(1) - (3)].section_phdr);
		  (yyval.section_phdr) = n;
		}
    break;

  case 313:
#line 1167 "ldgram.y"
    {
			  ldlex_script ();
			  lang_enter_overlay_section ((yyvsp[(2) - (2)].name));
			}
    break;

  case 314:
#line 1172 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 315:
#line 1174 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay_section ((yyvsp[(9) - (9)].fill), (yyvsp[(8) - (9)].section_phdr));
			}
    break;

  case 320:
#line 1191 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 321:
#line 1192 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 322:
#line 1194 "ldgram.y"
    {
		  lang_new_phdr ((yyvsp[(1) - (6)].name), (yyvsp[(3) - (6)].etree), (yyvsp[(4) - (6)].phdr).filehdr, (yyvsp[(4) - (6)].phdr).phdrs, (yyvsp[(4) - (6)].phdr).at,
				 (yyvsp[(4) - (6)].phdr).flags);
		}
    break;

  case 323:
#line 1202 "ldgram.y"
    {
		  (yyval.etree) = (yyvsp[(1) - (1)].etree);

		  if ((yyvsp[(1) - (1)].etree)->type.node_class == etree_name
		      && (yyvsp[(1) - (1)].etree)->type.node_code == NAME)
		    {
		      const char *s;
		      unsigned int i;
		      static const char * const phdr_types[] =
			{
			  "PT_NULL", "PT_LOAD", "PT_DYNAMIC",
			  "PT_INTERP", "PT_NOTE", "PT_SHLIB",
			  "PT_PHDR", "PT_TLS"
			};

		      s = (yyvsp[(1) - (1)].etree)->name.name;
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
    break;

  case 324:
#line 1246 "ldgram.y"
    {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
    break;

  case 325:
#line 1250 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[(3) - (3)].phdr);
		  if (strcmp ((yyvsp[(1) - (3)].name), "FILEHDR") == 0 && (yyvsp[(2) - (3)].etree) == NULL)
		    (yyval.phdr).filehdr = TRUE;
		  else if (strcmp ((yyvsp[(1) - (3)].name), "PHDRS") == 0 && (yyvsp[(2) - (3)].etree) == NULL)
		    (yyval.phdr).phdrs = TRUE;
		  else if (strcmp ((yyvsp[(1) - (3)].name), "FLAGS") == 0 && (yyvsp[(2) - (3)].etree) != NULL)
		    (yyval.phdr).flags = (yyvsp[(2) - (3)].etree);
		  else
		    einfo (_("%X%P:%S: PHDRS syntax error at `%s'\n"),
			   NULL, (yyvsp[(1) - (3)].name));
		}
    break;

  case 326:
#line 1263 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[(5) - (5)].phdr);
		  (yyval.phdr).at = (yyvsp[(3) - (5)].etree);
		}
    break;

  case 327:
#line 1271 "ldgram.y"
    {
		  (yyval.etree) = NULL;
		}
    break;

  case 328:
#line 1275 "ldgram.y"
    {
		  (yyval.etree) = (yyvsp[(2) - (3)].etree);
		}
    break;

  case 329:
#line 1281 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
    break;

  case 330:
#line 1286 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 334:
#line 1303 "ldgram.y"
    {
		  lang_append_dynamic_list ((yyvsp[(1) - (2)].versyms));
		}
    break;

  case 335:
#line 1311 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
    break;

  case 336:
#line 1316 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 337:
#line 1325 "ldgram.y"
    {
		  ldlex_version_script ();
		}
    break;

  case 338:
#line 1329 "ldgram.y"
    {
		  ldlex_popstate ();
		}
    break;

  case 341:
#line 1341 "ldgram.y"
    {
		  lang_register_vers_node (NULL, (yyvsp[(2) - (4)].versnode), NULL);
		}
    break;

  case 342:
#line 1345 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[(1) - (5)].name), (yyvsp[(3) - (5)].versnode), NULL);
		}
    break;

  case 343:
#line 1349 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[(1) - (6)].name), (yyvsp[(3) - (6)].versnode), (yyvsp[(5) - (6)].deflist));
		}
    break;

  case 344:
#line 1356 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[(1) - (1)].name));
		}
    break;

  case 345:
#line 1360 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[(1) - (2)].deflist), (yyvsp[(2) - (2)].name));
		}
    break;

  case 346:
#line 1367 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
    break;

  case 347:
#line 1371 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(1) - (2)].versyms), NULL);
		}
    break;

  case 348:
#line 1375 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(3) - (4)].versyms), NULL);
		}
    break;

  case 349:
#line 1379 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[(3) - (4)].versyms));
		}
    break;

  case 350:
#line 1383 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(3) - (8)].versyms), (yyvsp[(7) - (8)].versyms));
		}
    break;

  case 351:
#line 1390 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[(1) - (1)].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 352:
#line 1394 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[(1) - (1)].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 353:
#line 1398 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), (yyvsp[(3) - (3)].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 354:
#line 1402 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), (yyvsp[(3) - (3)].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 355:
#line 1406 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[(4) - (5)].name);
			}
    break;

  case 356:
#line 1411 "ldgram.y"
    {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[(7) - (9)].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[(1) - (9)].versyms);
			  (yyval.versyms) = (yyvsp[(7) - (9)].versyms);
			  ldgram_vers_current_lang = (yyvsp[(6) - (9)].name);
			}
    break;

  case 357:
#line 1419 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[(2) - (3)].name);
			}
    break;

  case 358:
#line 1424 "ldgram.y"
    {
			  (yyval.versyms) = (yyvsp[(5) - (7)].versyms);
			  ldgram_vers_current_lang = (yyvsp[(4) - (7)].name);
			}
    break;

  case 359:
#line 1429 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 360:
#line 1433 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 361:
#line 1437 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 362:
#line 1441 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 363:
#line 1445 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 364:
#line 1449 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "extern", ldgram_vers_current_lang, FALSE);
		}
    break;


/* Line 1267 of yacc.c.  */
#line 4445 "ldgram.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
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

  /* Else will try to reuse look-ahead token after shifting the error
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

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
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

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


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

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
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
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 1459 "ldgram.y"

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

