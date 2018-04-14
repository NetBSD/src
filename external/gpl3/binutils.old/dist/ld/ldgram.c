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
     NOCROSSREFS_TO = 332,
     ORIGIN = 333,
     FILL = 334,
     LENGTH = 335,
     CREATE_OBJECT_SYMBOLS = 336,
     INPUT = 337,
     GROUP = 338,
     OUTPUT = 339,
     CONSTRUCTORS = 340,
     ALIGNMOD = 341,
     AT = 342,
     SUBALIGN = 343,
     HIDDEN = 344,
     PROVIDE = 345,
     PROVIDE_HIDDEN = 346,
     AS_NEEDED = 347,
     CHIP = 348,
     LIST = 349,
     SECT = 350,
     ABSOLUTE = 351,
     LOAD = 352,
     NEWLINE = 353,
     ENDWORD = 354,
     ORDER = 355,
     NAMEWORD = 356,
     ASSERT_K = 357,
     LOG2CEIL = 358,
     FORMAT = 359,
     PUBLIC = 360,
     DEFSYMEND = 361,
     BASE = 362,
     ALIAS = 363,
     TRUNCATE = 364,
     REL = 365,
     INPUT_SCRIPT = 366,
     INPUT_MRI_SCRIPT = 367,
     INPUT_DEFSYM = 368,
     CASE = 369,
     EXTERN = 370,
     START = 371,
     VERS_TAG = 372,
     VERS_IDENTIFIER = 373,
     GLOBAL = 374,
     LOCAL = 375,
     VERSIONK = 376,
     INPUT_VERSION_SCRIPT = 377,
     KEEP = 378,
     ONLY_IF_RO = 379,
     ONLY_IF_RW = 380,
     SPECIAL = 381,
     INPUT_SECTION_FLAGS = 382,
     ALIGN_WITH_INPUT = 383,
     EXCLUDE_FILE = 384,
     CONSTANT = 385,
     INPUT_DYNAMIC_LIST = 386
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
#define NOCROSSREFS_TO 332
#define ORIGIN 333
#define FILL 334
#define LENGTH 335
#define CREATE_OBJECT_SYMBOLS 336
#define INPUT 337
#define GROUP 338
#define OUTPUT 339
#define CONSTRUCTORS 340
#define ALIGNMOD 341
#define AT 342
#define SUBALIGN 343
#define HIDDEN 344
#define PROVIDE 345
#define PROVIDE_HIDDEN 346
#define AS_NEEDED 347
#define CHIP 348
#define LIST 349
#define SECT 350
#define ABSOLUTE 351
#define LOAD 352
#define NEWLINE 353
#define ENDWORD 354
#define ORDER 355
#define NAMEWORD 356
#define ASSERT_K 357
#define LOG2CEIL 358
#define FORMAT 359
#define PUBLIC 360
#define DEFSYMEND 361
#define BASE 362
#define ALIAS 363
#define TRUNCATE 364
#define REL 365
#define INPUT_SCRIPT 366
#define INPUT_MRI_SCRIPT 367
#define INPUT_DEFSYM 368
#define CASE 369
#define EXTERN 370
#define START 371
#define VERS_TAG 372
#define VERS_IDENTIFIER 373
#define GLOBAL 374
#define LOCAL 375
#define VERSIONK 376
#define INPUT_VERSION_SCRIPT 377
#define KEEP 378
#define ONLY_IF_RO 379
#define ONLY_IF_RW 380
#define SPECIAL 381
#define INPUT_SECTION_FLAGS 382
#define ALIGN_WITH_INPUT 383
#define EXCLUDE_FILE 384
#define CONSTANT 385
#define INPUT_DYNAMIC_LIST 386




/* Copy the first part of user declarations.  */
#line 22 "ldgram.y"

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
#line 60 "ldgram.y"
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
#line 428 "ldgram.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 441 "ldgram.c"

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
#define YYLAST   1973

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  155
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  131
/* YYNRULES -- Number of rules.  */
#define YYNRULES  372
/* YYNRULES -- Number of states.  */
#define YYNSTATES  809

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   386

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   153,     2,     2,     2,    34,    21,     2,
      37,   150,    32,    30,   148,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   149,
      24,     6,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   151,     2,   152,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,    19,    59,   154,     2,     2,     2,
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
     141,   142,   143,   144,   145,   146,   147
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
     290,   294,   298,   305,   310,   311,   314,   316,   320,   323,
     325,   329,   332,   333,   339,   340,   348,   349,   356,   361,
     364,   367,   368,   373,   376,   377,   385,   387,   389,   391,
     393,   399,   404,   409,   414,   422,   430,   438,   446,   455,
     460,   462,   466,   471,   474,   476,   480,   482,   484,   487,
     491,   496,   501,   507,   509,   510,   516,   519,   521,   523,
     525,   530,   532,   537,   542,   543,   552,   553,   559,   562,
     564,   565,   567,   569,   571,   573,   575,   577,   579,   582,
     583,   585,   587,   589,   591,   593,   595,   597,   599,   601,
     603,   607,   611,   618,   625,   632,   634,   635,   640,   642,
     643,   647,   649,   650,   658,   659,   665,   669,   673,   674,
     678,   680,   683,   685,   688,   693,   698,   702,   706,   708,
     713,   717,   718,   720,   722,   723,   726,   730,   731,   734,
     737,   741,   746,   749,   752,   755,   759,   763,   767,   771,
     775,   779,   783,   787,   791,   795,   799,   803,   807,   811,
     815,   819,   825,   829,   833,   838,   840,   842,   847,   852,
     857,   862,   867,   872,   877,   884,   891,   898,   903,   910,
     915,   917,   924,   931,   938,   943,   948,   953,   957,   958,
     963,   964,   969,   970,   972,   973,   978,   979,   981,   983,
     985,   986,   987,   988,   989,   990,   991,  1012,  1013,  1014,
    1015,  1016,  1017,  1036,  1037,  1038,  1046,  1047,  1053,  1055,
    1057,  1059,  1061,  1063,  1067,  1068,  1071,  1075,  1078,  1085,
    1096,  1099,  1101,  1102,  1104,  1107,  1108,  1109,  1113,  1114,
    1115,  1116,  1117,  1129,  1134,  1135,  1138,  1139,  1140,  1147,
    1149,  1150,  1154,  1160,  1161,  1165,  1166,  1169,  1171,  1174,
    1179,  1182,  1183,  1186,  1187,  1193,  1195,  1198,  1203,  1209,
    1216,  1218,  1221,  1222,  1225,  1230,  1235,  1244,  1246,  1248,
    1252,  1256,  1257,  1267,  1268,  1276,  1278,  1282,  1284,  1288,
    1290,  1294,  1295
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     156,     0,    -1,   127,   172,    -1,   128,   160,    -1,   138,
     274,    -1,   147,   269,    -1,   129,   158,    -1,     4,    -1,
      -1,   159,     4,     6,   229,    -1,    -1,   161,   162,    -1,
     162,   163,   114,    -1,    -1,   109,   229,    -1,   109,   229,
     148,   229,    -1,     4,    -1,   110,    -1,   116,   165,    -1,
     115,    -1,   121,     4,     6,   229,    -1,   121,     4,   148,
     229,    -1,   121,     4,   229,    -1,   120,     4,    -1,   111,
       4,   148,   229,    -1,   111,     4,   229,    -1,   111,     4,
       6,   229,    -1,    38,     4,     6,   229,    -1,    38,     4,
     148,   229,    -1,   102,     4,     6,   229,    -1,   102,     4,
     148,   229,    -1,   112,   167,    -1,   113,   166,    -1,   117,
       4,    -1,   124,     4,   148,     4,    -1,   124,     4,   148,
       3,    -1,   123,   229,    -1,   125,     3,    -1,   130,   168,
      -1,   131,   169,    -1,    -1,    66,   157,   164,   162,    36,
      -1,   132,     4,    -1,    -1,   165,   148,     4,    -1,   165,
       4,    -1,    -1,     4,    -1,   166,   148,     4,    -1,     4,
      -1,   167,   148,     4,    -1,    -1,     4,    -1,   168,   148,
       4,    -1,    -1,   170,   171,    -1,     4,    -1,   171,     4,
      -1,   171,   148,     4,    -1,    -1,   173,   174,    -1,   174,
     175,    -1,    -1,   209,    -1,   184,    -1,   261,    -1,   220,
      -1,   221,    -1,   223,    -1,   225,    -1,   186,    -1,   276,
      -1,   149,    -1,    76,    37,     4,   150,    -1,    77,    37,
     157,   150,    -1,   100,    37,   157,   150,    -1,    61,    37,
       4,   150,    -1,    61,    37,     4,   148,     4,   148,     4,
     150,    -1,    63,    37,     4,   150,    -1,    62,    -1,    64,
      -1,    98,    37,   178,   150,    -1,    -1,    99,   176,    37,
     178,   150,    -1,    78,    37,   157,   150,    -1,    -1,    66,
     157,   177,   174,    36,    -1,    92,    37,   226,   150,    -1,
      93,    37,   226,   150,    -1,   131,    37,   169,   150,    -1,
      48,    49,     4,    -1,    48,    50,     4,    -1,    68,    37,
       4,   148,     4,   150,    -1,    69,    37,     4,   150,    -1,
      -1,   179,   180,    -1,     4,    -1,   180,   148,     4,    -1,
     180,     4,    -1,     5,    -1,   180,   148,     5,    -1,   180,
       5,    -1,    -1,   108,    37,   181,   180,   150,    -1,    -1,
     180,   148,   108,    37,   182,   180,   150,    -1,    -1,   180,
     108,    37,   183,   180,   150,    -1,    46,    58,   185,    59,
      -1,   185,   236,    -1,   185,   186,    -1,    -1,    79,    37,
       4,   150,    -1,   207,   206,    -1,    -1,   118,   187,    37,
     229,   148,     4,   150,    -1,     4,    -1,    32,    -1,    15,
      -1,   188,    -1,   145,    37,   192,   150,   188,    -1,    54,
      37,   188,   150,    -1,    55,    37,   188,   150,    -1,    56,
      37,   188,   150,    -1,    54,    37,    55,    37,   188,   150,
     150,    -1,    54,    37,    54,    37,   188,   150,   150,    -1,
      55,    37,    54,    37,   188,   150,   150,    -1,    55,    37,
      55,    37,   188,   150,   150,    -1,    54,    37,   145,    37,
     192,   150,   188,   150,    -1,    57,    37,   188,   150,    -1,
       4,    -1,   190,    21,     4,    -1,   143,    37,   190,   150,
      -1,   192,   188,    -1,   188,    -1,   193,   208,   189,    -1,
     189,    -1,     4,    -1,   191,     4,    -1,   151,   193,   152,
      -1,   191,   151,   193,   152,    -1,   189,    37,   193,   150,
      -1,   191,   189,    37,   193,   150,    -1,   194,    -1,    -1,
     139,    37,   196,   194,   150,    -1,   207,   206,    -1,    97,
      -1,   149,    -1,   101,    -1,    54,    37,   101,   150,    -1,
     195,    -1,   202,    37,   227,   150,    -1,    95,    37,   203,
     150,    -1,    -1,   118,   198,    37,   229,   148,     4,   150,
     206,    -1,    -1,    66,   157,   199,   201,    36,    -1,   200,
     197,    -1,   197,    -1,    -1,   200,    -1,    41,    -1,    42,
      -1,    43,    -1,    44,    -1,    45,    -1,   227,    -1,     6,
     203,    -1,    -1,    14,    -1,    13,    -1,    12,    -1,    11,
      -1,    10,    -1,     9,    -1,     8,    -1,     7,    -1,   149,
      -1,   148,    -1,     4,     6,   227,    -1,     4,   205,   227,
      -1,   105,    37,     4,     6,   227,   150,    -1,   106,    37,
       4,     6,   227,   150,    -1,   107,    37,     4,     6,   227,
     150,    -1,   148,    -1,    -1,    67,    58,   210,    59,    -1,
     211,    -1,    -1,   211,   208,   212,    -1,   212,    -1,    -1,
       4,   213,   217,    16,   215,   208,   216,    -1,    -1,    66,
     157,   214,   210,    36,    -1,    94,     6,   227,    -1,    96,
       6,   227,    -1,    -1,    37,   218,   150,    -1,   219,    -1,
     218,   219,    -1,     4,    -1,   153,     4,    -1,    87,    37,
     157,   150,    -1,    88,    37,   222,   150,    -1,    88,    37,
     150,    -1,   222,   208,   157,    -1,   157,    -1,    89,    37,
     224,   150,    -1,   224,   208,   157,    -1,    -1,    90,    -1,
      91,    -1,    -1,     4,   226,    -1,     4,   148,   226,    -1,
      -1,   228,   229,    -1,    31,   229,    -1,    37,   229,   150,
      -1,    80,    37,   229,   150,    -1,   153,   229,    -1,    30,
     229,    -1,   154,   229,    -1,   229,    32,   229,    -1,   229,
      33,   229,    -1,   229,    34,   229,    -1,   229,    30,   229,
      -1,   229,    31,   229,    -1,   229,    29,   229,    -1,   229,
      28,   229,    -1,   229,    23,   229,    -1,   229,    22,   229,
      -1,   229,    27,   229,    -1,   229,    26,   229,    -1,   229,
      24,   229,    -1,   229,    25,   229,    -1,   229,    21,   229,
      -1,   229,    20,   229,    -1,   229,    19,   229,    -1,   229,
      15,   229,    16,   229,    -1,   229,    18,   229,    -1,   229,
      17,   229,    -1,    75,    37,     4,   150,    -1,     3,    -1,
      60,    -1,    82,    37,     4,   150,    -1,    81,    37,     4,
     150,    -1,    83,    37,     4,   150,    -1,    84,    37,     4,
     150,    -1,   146,    37,     4,   150,    -1,   112,    37,   229,
     150,    -1,    38,    37,   229,   150,    -1,    38,    37,   229,
     148,   229,   150,    -1,    51,    37,   229,   148,   229,   150,
      -1,    52,    37,   229,   148,   229,   150,    -1,    53,    37,
     229,   150,    -1,    65,    37,     4,   148,   229,   150,    -1,
      39,    37,   229,   150,    -1,     4,    -1,    85,    37,   229,
     148,   229,   150,    -1,    86,    37,   229,   148,   229,   150,
      -1,   118,    37,   229,   148,     4,   150,    -1,    94,    37,
       4,   150,    -1,    96,    37,     4,   150,    -1,   119,    37,
     229,   150,    -1,   103,    25,     4,    -1,    -1,   103,    37,
     229,   150,    -1,    -1,    38,    37,   229,   150,    -1,    -1,
     144,    -1,    -1,   104,    37,   229,   150,    -1,    -1,   140,
      -1,   141,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     4,   237,   252,   231,   232,   233,   234,   238,   235,
      58,   239,   201,    59,   240,   255,   230,   256,   204,   241,
     208,    -1,    -1,    -1,    -1,    -1,    -1,    74,   242,   253,
     254,   231,   234,   243,    58,   244,   257,    59,   245,   255,
     230,   256,   204,   246,   208,    -1,    -1,    -1,    99,   247,
     252,   248,    58,   185,    59,    -1,    -1,    66,   157,   249,
     185,    36,    -1,    70,    -1,    71,    -1,    72,    -1,    73,
      -1,    74,    -1,    37,   250,   150,    -1,    -1,    37,   150,
      -1,   229,   251,    16,    -1,   251,    16,    -1,    40,    37,
     229,   150,   251,    16,    -1,    40,    37,   229,   150,    39,
      37,   229,   150,   251,    16,    -1,   229,    16,    -1,    16,
      -1,    -1,    92,    -1,    25,     4,    -1,    -1,    -1,   256,
      16,     4,    -1,    -1,    -1,    -1,    -1,   257,     4,   258,
      58,   201,    59,   259,   256,   204,   260,   208,    -1,    47,
      58,   262,    59,    -1,    -1,   262,   263,    -1,    -1,    -1,
       4,   264,   266,   267,   265,   149,    -1,   229,    -1,    -1,
       4,   268,   267,    -1,   103,    37,   229,   150,   267,    -1,
      -1,    37,   229,   150,    -1,    -1,   270,   271,    -1,   272,
      -1,   271,   272,    -1,    58,   273,    59,   149,    -1,   282,
     149,    -1,    -1,   275,   278,    -1,    -1,   277,   137,    58,
     278,    59,    -1,   279,    -1,   278,   279,    -1,    58,   281,
      59,   149,    -1,   133,    58,   281,    59,   149,    -1,   133,
      58,   281,    59,   280,   149,    -1,   133,    -1,   280,   133,
      -1,    -1,   282,   149,    -1,   135,    16,   282,   149,    -1,
     136,    16,   282,   149,    -1,   135,    16,   282,   149,   136,
      16,   282,   149,    -1,   134,    -1,     4,    -1,   282,   149,
     134,    -1,   282,   149,     4,    -1,    -1,   282,   149,   131,
       4,    58,   283,   282,   285,    59,    -1,    -1,   131,     4,
      58,   284,   282,   285,    59,    -1,   135,    -1,   282,   149,
     135,    -1,   136,    -1,   282,   149,   136,    -1,   131,    -1,
     282,   149,   131,    -1,    -1,   149,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
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
     339,   341,   343,   342,   346,   349,   348,   352,   356,   360,
     361,   363,   365,   367,   372,   372,   377,   380,   383,   386,
     389,   392,   396,   395,   401,   400,   406,   405,   413,   417,
     418,   419,   423,   425,   426,   426,   434,   438,   442,   449,
     456,   463,   470,   477,   484,   491,   498,   505,   512,   519,
     528,   546,   567,   580,   589,   600,   609,   620,   629,   638,
     642,   651,   655,   663,   665,   664,   671,   672,   676,   677,
     682,   687,   688,   693,   697,   697,   701,   700,   707,   708,
     711,   713,   717,   719,   721,   723,   725,   730,   737,   739,
     743,   745,   747,   749,   751,   753,   755,   757,   762,   762,
     767,   771,   779,   783,   787,   795,   795,   799,   802,   802,
     805,   806,   811,   810,   816,   815,   822,   830,   838,   839,
     843,   844,   848,   850,   855,   860,   861,   866,   868,   874,
     876,   878,   882,   884,   890,   893,   902,   913,   913,   919,
     921,   923,   925,   927,   929,   932,   934,   936,   938,   940,
     942,   944,   946,   948,   950,   952,   954,   956,   958,   960,
     962,   964,   966,   968,   970,   972,   974,   977,   979,   981,
     983,   985,   987,   989,   991,   993,   995,   997,   999,  1008,
    1010,  1012,  1014,  1016,  1018,  1020,  1022,  1028,  1029,  1033,
    1034,  1038,  1039,  1043,  1044,  1048,  1049,  1053,  1054,  1055,
    1056,  1059,  1064,  1067,  1073,  1075,  1059,  1082,  1084,  1086,
    1091,  1093,  1081,  1103,  1105,  1103,  1111,  1110,  1117,  1118,
    1119,  1120,  1121,  1125,  1126,  1127,  1131,  1132,  1137,  1138,
    1143,  1144,  1149,  1150,  1155,  1157,  1162,  1165,  1178,  1182,
    1187,  1189,  1180,  1197,  1200,  1202,  1206,  1207,  1206,  1216,
    1261,  1264,  1277,  1286,  1289,  1296,  1296,  1308,  1309,  1313,
    1317,  1326,  1326,  1340,  1340,  1350,  1351,  1355,  1359,  1363,
    1370,  1374,  1382,  1385,  1389,  1393,  1397,  1404,  1408,  1412,
    1416,  1421,  1420,  1434,  1433,  1443,  1447,  1451,  1455,  1459,
    1463,  1469,  1471
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
  "NOFLOAT", "NOCROSSREFS", "NOCROSSREFS_TO", "ORIGIN", "FILL", "LENGTH",
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
  "$accept", "file", "filename", "defsym_expr", "@1", "mri_script_file",
  "@2", "mri_script_lines", "mri_script_command", "@3", "ordernamelist",
  "mri_load_name_list", "mri_abs_name_list", "casesymlist",
  "extern_name_list", "@4", "extern_name_list_body", "script_file", "@5",
  "ifile_list", "ifile_p1", "@6", "@7", "input_list", "@8", "input_list1",
  "@9", "@10", "@11", "sections", "sec_or_group_p1", "statement_anywhere",
  "@12", "wildcard_name", "wildcard_spec", "sect_flag_list", "sect_flags",
  "exclude_name_list", "file_NAME_list", "input_section_spec_no_keep",
  "input_section_spec", "@13", "statement", "@14", "@15", "statement_list",
  "statement_list_opt", "length", "fill_exp", "fill_opt", "assign_op",
  "end", "assignment", "opt_comma", "memory", "memory_spec_list_opt",
  "memory_spec_list", "memory_spec", "@16", "@17", "origin_spec",
  "length_spec", "attributes_opt", "attributes_list", "attributes_string",
  "startup", "high_level_library", "high_level_library_NAME_list",
  "low_level_library", "low_level_library_NAME_list",
  "floating_point_support", "nocrossref_list", "mustbe_exp", "@18", "exp",
  "memspec_at_opt", "opt_at", "opt_align", "opt_align_with_input",
  "opt_subalign", "sect_constraint", "section", "@19", "@20", "@21", "@22",
  "@23", "@24", "@25", "@26", "@27", "@28", "@29", "@30", "@31", "type",
  "atype", "opt_exp_with_type", "opt_exp_without_type", "opt_nocrossrefs",
  "memspec_opt", "phdr_opt", "overlay_section", "@32", "@33", "@34",
  "phdrs", "phdr_list", "phdr", "@35", "@36", "phdr_type",
  "phdr_qualifiers", "phdr_val", "dynamic_list_file", "@37",
  "dynamic_list_nodes", "dynamic_list_node", "dynamic_list_tag",
  "version_script_file", "@38", "version", "@39", "vers_nodes",
  "vers_node", "verdep", "vers_tag", "vers_defns", "@40", "@41",
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
     379,   380,   381,   382,   383,   384,   385,   386,    44,    59,
      41,    91,    93,    33,   126
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   155,   156,   156,   156,   156,   156,   157,   159,   158,
     161,   160,   162,   162,   163,   163,   163,   163,   163,   163,
     163,   163,   163,   163,   163,   163,   163,   163,   163,   163,
     163,   163,   163,   163,   163,   163,   163,   163,   163,   163,
     164,   163,   163,   163,   165,   165,   165,   166,   166,   167,
     167,   168,   168,   168,   170,   169,   171,   171,   171,   173,
     172,   174,   174,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   176,   175,   175,   177,   175,   175,   175,   175,
     175,   175,   175,   175,   179,   178,   180,   180,   180,   180,
     180,   180,   181,   180,   182,   180,   183,   180,   184,   185,
     185,   185,   186,   186,   187,   186,   188,   188,   188,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     190,   190,   191,   192,   192,   193,   193,   194,   194,   194,
     194,   194,   194,   195,   196,   195,   197,   197,   197,   197,
     197,   197,   197,   197,   198,   197,   199,   197,   200,   200,
     201,   201,   202,   202,   202,   202,   202,   203,   204,   204,
     205,   205,   205,   205,   205,   205,   205,   205,   206,   206,
     207,   207,   207,   207,   207,   208,   208,   209,   210,   210,
     211,   211,   213,   212,   214,   212,   215,   216,   217,   217,
     218,   218,   219,   219,   220,   221,   221,   222,   222,   223,
     224,   224,   225,   225,   226,   226,   226,   228,   227,   229,
     229,   229,   229,   229,   229,   229,   229,   229,   229,   229,
     229,   229,   229,   229,   229,   229,   229,   229,   229,   229,
     229,   229,   229,   229,   229,   229,   229,   229,   229,   229,
     229,   229,   229,   229,   229,   229,   229,   229,   229,   229,
     229,   229,   229,   229,   229,   229,   229,   230,   230,   231,
     231,   232,   232,   233,   233,   234,   234,   235,   235,   235,
     235,   237,   238,   239,   240,   241,   236,   242,   243,   244,
     245,   246,   236,   247,   248,   236,   249,   236,   250,   250,
     250,   250,   250,   251,   251,   251,   252,   252,   252,   252,
     253,   253,   254,   254,   255,   255,   256,   256,   257,   258,
     259,   260,   257,   261,   262,   262,   264,   265,   263,   266,
     267,   267,   267,   268,   268,   270,   269,   271,   271,   272,
     273,   275,   274,   277,   276,   278,   278,   279,   279,   279,
     280,   280,   281,   281,   281,   281,   281,   282,   282,   282,
     282,   283,   282,   284,   282,   282,   282,   282,   282,   282,
     282,   285,   285
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
       1,     4,     0,     5,     4,     0,     5,     4,     4,     4,
       3,     3,     6,     4,     0,     2,     1,     3,     2,     1,
       3,     2,     0,     5,     0,     7,     0,     6,     4,     2,
       2,     0,     4,     2,     0,     7,     1,     1,     1,     1,
       5,     4,     4,     4,     7,     7,     7,     7,     8,     4,
       1,     3,     4,     2,     1,     3,     1,     1,     2,     3,
       4,     4,     5,     1,     0,     5,     2,     1,     1,     1,
       4,     1,     4,     4,     0,     8,     0,     5,     2,     1,
       0,     1,     1,     1,     1,     1,     1,     1,     2,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     6,     6,     6,     1,     0,     4,     1,     0,
       3,     1,     0,     7,     0,     5,     3,     3,     0,     3,
       1,     2,     1,     2,     4,     4,     3,     3,     1,     4,
       3,     0,     1,     1,     0,     2,     3,     0,     2,     2,
       3,     4,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     5,     3,     3,     4,     1,     1,     4,     4,     4,
       4,     4,     4,     4,     6,     6,     6,     4,     6,     4,
       1,     6,     6,     6,     4,     4,     4,     3,     0,     4,
       0,     4,     0,     1,     0,     4,     0,     1,     1,     1,
       0,     0,     0,     0,     0,     0,    20,     0,     0,     0,
       0,     0,    18,     0,     0,     7,     0,     5,     1,     1,
       1,     1,     1,     3,     0,     2,     3,     2,     6,    10,
       2,     1,     0,     1,     2,     0,     0,     3,     0,     0,
       0,     0,    11,     4,     0,     2,     0,     0,     6,     1,
       0,     3,     5,     0,     3,     0,     2,     1,     2,     4,
       2,     0,     2,     0,     5,     1,     2,     4,     5,     6,
       1,     2,     0,     2,     4,     4,     8,     1,     1,     3,
       3,     0,     9,     0,     7,     1,     3,     1,     3,     1,
       3,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    59,    10,     8,   341,   335,     0,     2,    62,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    60,    11,
       0,   352,     0,   342,   345,     0,   336,   337,     0,     0,
       0,     0,     0,    79,     0,    80,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   212,   213,     0,
       0,     0,    82,     0,     0,     0,     0,   114,     0,    72,
      61,    64,    70,     0,    63,    66,    67,    68,    69,    65,
      71,     0,    16,     0,     0,     0,     0,    17,     0,     0,
       0,    19,    46,     0,     0,     0,     0,     0,     0,    51,
      54,     0,     0,     0,   358,   369,   357,   365,   367,     0,
       0,   352,   346,   365,   367,     0,     0,   338,   217,   177,
     176,   175,   174,   173,   172,   171,   170,   217,   111,   324,
       0,     0,     0,     0,     7,    85,   189,     0,     0,     0,
       0,     0,     0,     0,     0,   211,   214,   214,    94,     0,
       0,     0,     0,     0,     0,    54,   179,   178,   113,     0,
       0,    40,     0,   245,   260,     0,     0,     0,     0,     0,
       0,     0,     0,   246,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    14,     0,    49,    31,    47,    32,    18,    33,    23,
       0,    36,     0,    37,    52,    38,    39,     0,    42,    12,
       9,     0,     0,     0,     0,   353,     0,     0,   340,   180,
       0,   181,     0,     0,    90,    91,     0,     0,    62,   192,
       0,     0,   186,   191,     0,     0,     0,     0,     0,     0,
       0,   206,   208,   186,   186,   214,     0,     0,     0,     0,
      94,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      13,     0,     0,   223,   219,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   222,   224,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      25,     0,     0,    45,     0,     0,     0,    22,     0,     0,
      56,    55,   363,     0,     0,   347,   360,   370,   359,   366,
     368,     0,   339,   218,   281,   108,     0,   287,   293,   110,
     109,   326,   323,   325,     0,    76,    78,   343,   198,   194,
     187,   185,     0,     0,    93,    73,    74,    84,   112,   204,
     205,     0,   209,     0,   214,   215,    87,    88,    81,    96,
      99,     0,    95,     0,    75,   217,   217,   217,     0,    89,
       0,    27,    28,    43,    29,    30,   220,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   243,   242,
     240,   239,   238,   233,   232,   236,   237,   235,   234,   231,
     230,   228,   229,   225,   226,   227,    15,    26,    24,    50,
      48,    44,    20,    21,    35,    34,    53,    57,     0,     0,
     354,   355,     0,   350,   348,     0,   304,   296,     0,   304,
       0,     0,    86,     0,     0,   189,   190,     0,   207,   210,
     216,   102,    98,   101,     0,     0,    83,     0,     0,     0,
       0,   344,    41,     0,   253,   259,     0,     0,   257,     0,
     244,   221,   248,   247,   249,   250,     0,     0,   264,   265,
     252,     0,   266,   251,     0,    58,   371,   368,   361,   351,
     349,     0,     0,   304,     0,   270,   111,   311,     0,   312,
     294,   329,   330,     0,   202,     0,     0,   200,     0,     0,
      92,     0,   106,    97,   100,     0,   182,   183,   184,     0,
       0,     0,     0,     0,     0,     0,     0,   241,   372,     0,
       0,     0,   298,   299,   300,   301,   302,   305,     0,     0,
       0,     0,   307,     0,   272,     0,   310,   313,   270,     0,
     333,     0,   327,     0,   203,   199,   201,     0,   186,   195,
       0,     0,   104,   115,   254,   255,   256,   258,   261,   262,
     263,   364,     0,   371,   303,     0,   306,     0,     0,   274,
     297,   276,   111,     0,   330,     0,     0,    77,   217,     0,
     103,     0,     0,   356,     0,   304,     0,     0,   273,   276,
       0,   288,     0,     0,   331,     0,   328,   196,     0,   193,
     107,     0,   362,     0,     0,   269,     0,   282,     0,     0,
     295,   334,   330,   217,   105,     0,   308,   271,   280,     0,
     289,   332,   197,     0,   277,   278,   279,     0,   275,   318,
     304,   283,     0,     0,   160,   319,   290,   309,   137,   118,
     117,   162,   163,   164,   165,   166,     0,     0,     0,     0,
       0,     0,   147,   149,   154,     0,     0,     0,   148,     0,
     119,     0,     0,   143,   151,   159,   161,     0,     0,     0,
       0,   315,     0,     0,     0,     0,   156,   217,     0,   144,
       0,     0,   116,     0,   136,   186,     0,   138,     0,     0,
     158,   284,   217,   146,   160,     0,   268,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   160,     0,   167,
       0,     0,   130,     0,   134,     0,     0,   139,     0,   186,
     186,     0,   315,     0,     0,   314,     0,   316,     0,     0,
     150,     0,   121,     0,     0,   122,   123,   129,     0,   153,
       0,   116,     0,     0,   132,     0,   133,   135,   141,   140,
     186,   268,   152,   320,     0,   169,     0,     0,     0,     0,
       0,   157,     0,   145,   131,   120,   142,   316,   316,   267,
     217,     0,   291,     0,     0,     0,     0,     0,     0,   169,
     169,   168,   317,   186,   125,   124,     0,   126,   127,     0,
     285,   321,   292,   128,   155,   186,   186,   286,   322
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,   125,    11,    12,     9,    10,    19,    92,   250,
     187,   186,   184,   195,   196,   197,   311,     7,     8,    18,
      60,   139,   218,   238,   239,   362,   511,   592,   561,    61,
     212,   329,   144,   670,   671,   723,   672,   725,   695,   673,
     674,   721,   675,   688,   717,   676,   677,   678,   718,   782,
     117,   148,    63,   728,    64,   221,   222,   223,   338,   445,
     558,   609,   444,   506,   507,    65,    66,   233,    67,   234,
      68,   236,   719,   210,   255,   737,   544,   579,   599,   601,
     637,   330,   436,   628,   644,   732,   805,   438,   619,   639,
     681,   793,   439,   549,   496,   538,   494,   495,   499,   548,
     706,   765,   642,   680,   778,   806,    69,   213,   333,   440,
     586,   502,   552,   584,    15,    16,    26,    27,   105,    13,
      14,    70,    71,    23,    24,   435,    99,   100,   531,   429,
     529
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -664
static const yytype_int16 yypact[] =
{
     288,  -664,  -664,  -664,  -664,  -664,    32,  -664,  -664,  -664,
    -664,  -664,    59,  -664,   -12,  -664,    10,  -664,   931,  1764,
      92,   109,    58,   -12,  -664,   115,    10,  -664,   486,    65,
      74,    34,    55,  -664,    98,  -664,   149,   131,   164,   190,
     200,   205,   234,   248,   256,   260,   261,  -664,  -664,   275,
     277,   285,  -664,   287,   290,   293,   296,  -664,   298,  -664,
    -664,  -664,  -664,    85,  -664,  -664,  -664,  -664,  -664,  -664,
    -664,   144,  -664,   333,   149,   335,   765,  -664,   337,   338,
     340,  -664,  -664,   342,   344,   345,   765,   347,   356,   363,
    -664,   365,   254,   765,  -664,   366,  -664,   361,   367,   321,
     243,   109,  -664,  -664,  -664,   327,   255,  -664,  -664,  -664,
    -664,  -664,  -664,  -664,  -664,  -664,  -664,  -664,  -664,  -664,
     409,   410,   415,   416,  -664,  -664,    36,   417,   418,   423,
     149,   149,   424,   149,     8,  -664,   425,   425,  -664,   393,
     149,   427,   429,   430,   399,  -664,  -664,  -664,  -664,   390,
       9,  -664,    15,  -664,  -664,   765,   765,   765,   413,   414,
     422,   428,   448,  -664,   449,   450,   451,   452,   467,   469,
     471,   473,   475,   478,   479,   482,   485,   487,   490,   765,
     765,  1552,   372,  -664,   312,  -664,   313,    43,  -664,  -664,
     499,  1922,   314,  -664,  -664,   316,  -664,   463,  -664,  -664,
    1922,   465,   115,   115,   379,   148,   472,   383,   148,  -664,
     765,  -664,   266,    37,  -664,  -664,   -59,   384,  -664,  -664,
     149,   474,    52,  -664,   387,   389,   391,   394,   395,   396,
     398,  -664,  -664,   -19,    14,    46,   403,   421,   436,    25,
    -664,   438,   534,   537,   557,   765,   439,   -12,   765,   765,
    -664,   765,   765,  -664,  -664,   941,   765,   765,   765,   765,
     765,   563,   568,   765,   569,   588,   590,   593,   765,   765,
     594,   605,   765,   765,   765,   606,  -664,  -664,   765,   765,
     765,   765,   765,   765,   765,   765,   765,   765,   765,   765,
     765,   765,   765,   765,   765,   765,   765,   765,   765,   765,
    1922,   608,   609,  -664,   610,   765,   765,  1922,   244,   611,
    -664,    47,  -664,   470,   476,  -664,  -664,   612,  -664,  -664,
    -664,   -71,  -664,  1922,   486,  -664,   149,  -664,  -664,  -664,
    -664,  -664,  -664,  -664,   616,  -664,  -664,   997,   584,  -664,
    -664,  -664,    36,   618,  -664,  -664,  -664,  -664,  -664,  -664,
    -664,   149,  -664,   149,   425,  -664,  -664,  -664,  -664,  -664,
    -664,   586,    22,   480,  -664,  -664,  -664,  -664,  1572,  -664,
     -21,  1922,  1922,   445,  1922,  1922,  -664,  1130,  1150,  1592,
    1612,  1170,   481,   483,  1190,   488,   489,   491,   492,  1632,
    1684,   493,   496,  1210,  1704,  1230,   501,  1882,  1939,  1110,
     758,  1378,  1243,   446,   446,   574,   574,   574,   574,   364,
     364,   230,   230,  -664,  -664,  -664,  1922,  1922,  1922,  -664,
    -664,  -664,  1922,  1922,  -664,  -664,  -664,  -664,   620,   115,
     221,   148,   573,  -664,  -664,   -67,   597,  -664,   681,   597,
     765,   484,  -664,     2,   624,    36,  -664,   505,  -664,  -664,
    -664,  -664,  -664,  -664,   589,    38,  -664,   506,   508,   509,
     656,  -664,  -664,   765,  -664,  -664,   765,   765,  -664,   765,
    -664,  -664,  -664,  -664,  -664,  -664,   765,   765,  -664,  -664,
    -664,   659,  -664,  -664,   765,  -664,   515,   649,  -664,  -664,
    -664,   235,   629,  1766,   651,   565,  -664,  -664,  1902,   578,
    -664,  1922,    21,   667,  -664,   669,     1,  -664,   581,   640,
    -664,    25,  -664,  -664,  -664,   650,  -664,  -664,  -664,   536,
    1264,  1284,  1304,  1324,  1344,  1364,   538,  1922,   148,   630,
     115,   115,  -664,  -664,  -664,  -664,  -664,  -664,   540,   765,
     184,   676,  -664,   658,   660,   300,  -664,  -664,   565,   638,
     662,   666,  -664,   554,  -664,  -664,  -664,   699,   558,  -664,
      18,    25,  -664,  -664,  -664,  -664,  -664,  -664,  -664,  -664,
    -664,  -664,   559,   515,  -664,  1398,  -664,   765,   670,   566,
    -664,   613,  -664,   765,    21,   765,   572,  -664,  -664,   626,
    -664,    29,    25,   148,   655,   257,  1418,   765,  -664,   613,
     686,  -664,   595,  1438,  -664,  1458,  -664,  -664,   718,  -664,
    -664,    40,  -664,   688,   710,  -664,  1478,  -664,   765,   671,
    -664,  -664,    21,  -664,  -664,   765,  -664,  -664,   135,  1498,
    -664,  -664,  -664,  1532,  -664,  -664,  -664,   672,  -664,  -664,
     690,  -664,    50,   712,   837,  -664,  -664,  -664,   432,  -664,
    -664,  -664,  -664,  -664,  -664,  -664,   694,   698,   700,   701,
     149,   702,  -664,  -664,  -664,   703,   705,   707,  -664,    86,
    -664,   708,    20,  -664,  -664,  -664,   837,   677,   711,    85,
     689,   724,   237,    95,   124,   124,  -664,  -664,   715,  -664,
     749,   124,  -664,   717,  -664,    76,    86,   720,    86,   721,
    -664,  -664,  -664,  -664,   837,   751,   657,   722,   733,   621,
     735,   623,   737,   739,   644,   647,   648,   837,   661,  -664,
     765,    16,  -664,    28,  -664,    24,    90,  -664,    86,   153,
      84,    86,   724,   663,   742,  -664,   780,  -664,   124,   124,
    -664,   124,  -664,   124,   124,  -664,  -664,  -664,   770,  -664,
    1724,   664,   665,   803,  -664,   124,  -664,  -664,  -664,  -664,
     163,   657,  -664,  -664,   804,    53,   673,   674,    49,   678,
     679,  -664,   805,  -664,  -664,  -664,  -664,  -664,  -664,  -664,
    -664,   806,  -664,   682,   683,   124,   687,   692,   693,    53,
      53,  -664,  -664,   558,  -664,  -664,   704,  -664,  -664,    85,
    -664,  -664,  -664,  -664,  -664,   558,   558,  -664,  -664
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -664,  -664,   -73,  -664,  -664,  -664,  -664,   562,  -664,  -664,
    -664,  -664,  -664,  -664,   675,  -664,  -664,  -664,  -664,   601,
    -664,  -664,  -664,   582,  -664,  -475,  -664,  -664,  -664,  -664,
    -482,   -14,  -664,  1079,  -370,  -664,  -664,    80,  -611,   105,
    -664,  -664,   155,  -664,  -664,  -664,  -609,  -664,    56,  -522,
    -664,  -663,  -579,  -215,  -664,   408,  -664,   497,  -664,  -664,
    -664,  -664,  -664,  -664,   332,  -664,  -664,  -664,  -664,  -664,
    -664,  -128,  -106,  -664,   -76,    83,   307,  -664,  -664,   258,
    -664,  -664,  -664,  -664,  -664,  -664,  -664,  -664,  -664,  -664,
    -664,  -664,  -664,  -664,  -664,  -664,  -480,   419,  -664,  -664,
     128,  -488,  -664,  -664,  -664,  -664,  -664,  -664,  -664,  -664,
    -664,  -664,  -529,  -664,  -664,  -664,  -664,   830,  -664,  -664,
    -664,  -664,  -664,   615,   -20,  -664,   762,   -17,  -664,  -664,
     291
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -344
static const yytype_int16 yytable[] =
{
     181,   151,   209,   102,    62,   504,   504,   342,   106,   237,
     191,   211,   124,   541,   545,   248,   703,   200,   351,   353,
     751,   251,   452,   453,   697,   550,   452,   453,   692,   359,
     360,   649,    17,   452,   453,   649,   560,    21,   461,   649,
     219,   331,   513,   514,   452,   453,    21,   303,   650,   753,
     235,   427,   650,   692,   645,   604,   650,   227,   228,   780,
     230,   232,   433,    20,   649,   679,   489,   241,    25,   781,
     693,   657,   658,   659,   693,   657,   658,   659,   434,   253,
     254,   650,   490,   120,   121,   729,   591,   730,  -188,   334,
     692,   335,   122,   631,   692,   734,   332,   679,    93,   692,
     602,   649,   220,   276,   277,   649,   300,   355,   748,   646,
     649,  -188,    22,    94,   307,   614,   101,   611,   650,    94,
     760,    22,   650,   118,   551,   679,   454,   650,   692,   341,
     454,   350,   119,   361,   323,   123,   804,   454,   679,   649,
     693,   657,   658,   659,   707,   708,   515,   339,   454,   712,
     713,   555,   316,   124,   505,   505,   650,   249,   231,   666,
     643,   667,   341,   252,   352,   667,   455,   669,   590,   368,
     455,   698,   371,   372,   755,   374,   375,   455,   754,   610,
     377,   378,   379,   380,   381,   313,   314,   384,   455,   126,
     624,   304,   389,   390,   354,   428,   393,   394,   395,   785,
     341,   127,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   341,   316,   450,   128,   727,   422,
     423,   667,   341,   146,   147,   710,   759,   129,   153,   154,
      95,   692,   130,    96,    97,    98,    95,   424,   425,    96,
     103,   104,   649,   437,   532,   533,   534,   535,   536,   457,
     458,   459,   294,   295,   296,   155,   156,   800,   801,   650,
     324,   131,   157,   158,   159,   634,   635,   636,   448,   317,
     449,   149,   318,   319,   320,   132,   160,   161,   162,   789,
     790,   707,   708,   133,   540,   163,   613,   134,   135,   694,
     164,   341,   699,   758,   324,   532,   533,   534,   535,   536,
     165,   341,   136,   776,   137,   166,   167,   168,   169,   170,
     171,   172,   138,    62,   140,   325,   694,   141,   694,   173,
     142,   174,   326,   143,   537,   145,   580,   150,   709,   152,
     327,   182,   183,   589,   185,    43,   188,   175,   189,   190,
     102,   192,   317,   176,   177,   318,   319,   487,   757,   193,
     493,   694,   498,   493,   501,   328,   326,   194,   199,   198,
     201,    54,    55,    56,   327,   153,   154,   202,   298,    43,
     204,   178,   710,   203,    57,   537,   207,   520,   179,   180,
     521,   522,   205,   523,   292,   293,   294,   295,   296,   328,
     524,   525,   155,   156,   208,    54,    55,    56,   527,   157,
     158,   159,   486,   214,   215,     1,     2,     3,    57,   216,
     217,   224,   225,   160,   161,   162,     4,   226,   229,   235,
     240,   242,   163,   243,   244,     5,   245,   164,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   165,   247,    72,
     256,   257,   166,   167,   168,   169,   170,   171,   172,   258,
     301,   302,   308,   575,   309,   259,   173,   310,   174,  -116,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   462,   607,    73,   175,   260,   261,   262,   263,   264,
     176,   177,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   596,   153,   154,   265,   305,   266,   603,   267,   605,
     268,    74,   269,   572,   573,   270,   271,   632,   178,   272,
     299,   616,   273,   312,   274,   179,   180,   275,   315,   155,
     156,   321,   322,   340,   336,   343,   157,   158,   159,   344,
     365,   345,   629,   366,   346,   347,   348,    75,   349,   633,
     160,   161,   162,   356,    76,    77,    78,    79,    80,   163,
      81,    82,    83,   367,   164,    84,    85,   382,    86,    87,
      88,   357,   383,   385,   165,    89,    90,    91,   802,   166,
     167,   168,   169,   170,   171,   172,   358,   686,   364,   369,
     807,   808,   386,   173,   387,   174,   733,   388,   391,   324,
     153,   154,   290,   291,   292,   293,   294,   295,   296,   392,
     396,   175,   419,   420,   421,   426,   432,   176,   177,   430,
     441,   443,   447,   451,   485,   431,   512,   155,   156,   469,
     456,   488,   503,   470,   491,   158,   159,   492,   472,   473,
     508,   474,   475,   478,   750,   178,   479,   306,   160,   161,
     162,   483,   179,   180,   620,   510,   516,   163,   517,   518,
     519,   326,   164,   526,   528,   530,   539,   542,   543,   327,
     547,   553,   165,   554,    43,   557,   559,   166,   167,   168,
     169,   170,   171,   172,   153,   154,   563,   562,   570,   571,
     574,   173,   576,   174,   328,   577,   582,   497,   578,   583,
      54,    55,    56,   585,   587,   588,   341,   597,   593,   175,
     598,   155,   156,    57,   612,   176,   177,   600,   157,   158,
     159,   606,   608,   618,   623,   625,   626,   540,   647,   630,
     641,   682,   160,   161,   162,   683,   701,   684,   685,   687,
     689,   163,   690,   178,   691,   696,   164,   704,   702,   705,
     179,   180,   720,   722,   726,   735,   165,  -116,   731,   738,
     736,   166,   167,   168,   169,   170,   171,   172,   153,   154,
     739,   740,   741,   742,   743,   173,   744,   174,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   175,   745,   155,   156,   746,   747,   176,
     177,   763,   157,   158,   159,   764,   771,   774,   779,   788,
     792,   749,   373,   762,  -137,   773,   160,   161,   162,   337,
     246,   768,   363,   783,   784,   163,   752,   178,   786,   787,
     164,   700,   794,   795,   179,   180,   791,   797,   556,   446,
     165,   648,   798,   799,   777,   166,   167,   168,   169,   170,
     171,   172,   649,   509,   803,   581,   107,   617,   500,   173,
     761,   174,   370,   206,   594,     0,     0,     0,     0,   650,
       0,     0,     0,     0,     0,     0,     0,   175,   651,   652,
     653,   654,   655,   176,   177,     0,     0,     0,     0,     0,
       0,   656,   657,   658,   659,     0,     0,     0,     0,     0,
       0,     0,     0,   660,     0,     0,     0,     0,     0,     0,
       0,   178,     0,     0,     0,     0,     0,     0,   179,   180,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   661,     0,   662,    28,     0,     0,   663,     0,
       0,     0,    54,    55,    56,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   664,   278,     0,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   665,    29,    30,    31,
     666,     0,   667,     0,     0,     0,   668,     0,   669,     0,
       0,     0,    32,    33,    34,    35,     0,    36,    37,    38,
      39,    28,     0,     0,     0,     0,     0,    40,    41,    42,
      43,     0,     0,     0,     0,     0,     0,     0,    44,    45,
      46,    47,    48,    49,    50,     0,     0,     0,     0,    51,
      52,    53,     0,   442,     0,     0,    54,    55,    56,     0,
       0,     0,     0,    29,    30,    31,     0,     0,     0,    57,
       0,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,    35,    58,    36,    37,    38,    39,     0,  -343,     0,
       0,     0,     0,    40,    41,    42,    43,     0,     0,     0,
      59,     0,     0,     0,    44,    45,    46,    47,    48,    49,
      50,   376,     0,     0,     0,    51,    52,    53,     0,     0,
       0,     0,    54,    55,    56,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    58,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,    59,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,     0,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,     0,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,     0,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,     0,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,     0,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   463,   278,
     464,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   278,
     465,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   278,
     468,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   278,
     471,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   278,
     480,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   278,
     482,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   278,   564,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   278,   565,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   278,   566,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   278,   567,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   278,   568,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   278,   569,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   278,   595,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   278,   615,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   278,   621,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   278,   622,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   278,   627,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   278,   638,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   640,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   278,
     297,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   278,
     460,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   278,
     466,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,     0,
     467,   711,   714,   715,   716,     0,     0,     0,    72,     0,
     724,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     476,   278,     0,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,     0,    73,   540,   756,   711,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   766,   767,     0,
     724,     0,   769,   770,     0,     0,     0,     0,     0,     0,
      74,     0,   477,     0,   775,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   756,     0,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   796,     0,    75,     0,     0,     0,
       0,     0,   772,    76,    77,    78,    79,    80,   -43,    81,
      82,    83,     0,     0,    84,    85,     0,    86,    87,    88,
       0,     0,     0,     0,    89,    90,    91,   278,   484,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   278,   546,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   278,     0,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296
};

static const yytype_int16 yycheck[] =
{
      76,    74,   108,    23,    18,     4,     4,   222,    25,   137,
      86,   117,     4,   493,   496,     6,   679,    93,   233,   234,
       4,     6,     4,     5,     4,     4,     4,     5,     4,     4,
       5,    15,     0,     4,     5,    15,   511,    58,    59,    15,
       4,     4,     4,     5,     4,     5,    58,     4,    32,    21,
       4,     4,    32,     4,     4,   584,    32,   130,   131,     6,
     133,   134,   133,     4,    15,   644,   133,   140,    58,    16,
      54,    55,    56,    57,    54,    55,    56,    57,   149,   155,
     156,    32,   149,    49,    50,   696,   561,   698,    36,   148,
       4,   150,    37,   622,     4,   704,    59,   676,     6,     4,
     582,    15,    66,   179,   180,    15,   182,   235,   717,    59,
      15,    59,   133,     4,   190,   595,    58,   592,    32,     4,
     731,   133,    32,    58,   103,   704,   108,    32,     4,   148,
     108,   150,    58,   108,   210,    37,   799,   108,   717,    15,
      54,    55,    56,    57,    54,    55,   108,   220,   108,    54,
      55,   150,     4,     4,   153,   153,    32,   148,   150,   143,
     640,   145,   148,   148,   150,   145,   148,   151,   150,   245,
     148,   151,   248,   249,   150,   251,   252,   148,   150,   150,
     256,   257,   258,   259,   260,   202,   203,   263,   148,    58,
     150,   148,   268,   269,   148,   148,   272,   273,   274,   150,
     148,    37,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   148,     4,   354,    37,   152,   305,
     306,   145,   148,   148,   149,   145,   152,    37,     3,     4,
     131,     4,    37,   134,   135,   136,   131,     3,     4,   134,
     135,   136,    15,   326,    70,    71,    72,    73,    74,   365,
     366,   367,    32,    33,    34,    30,    31,   789,   790,    32,
       4,    37,    37,    38,    39,   140,   141,   142,   351,   131,
     353,   137,   134,   135,   136,    37,    51,    52,    53,   777,
     778,    54,    55,    37,    37,    60,    39,    37,    37,   669,
      65,   148,   672,   150,     4,    70,    71,    72,    73,    74,
      75,   148,    37,   150,    37,    80,    81,    82,    83,    84,
      85,    86,    37,   337,    37,    59,   696,    37,   698,    94,
      37,    96,    66,    37,   150,    37,    36,     4,   101,     4,
      74,     4,     4,   558,     4,    79,     4,   112,     4,     4,
     370,     4,   131,   118,   119,   134,   135,   136,   728,     3,
     436,   731,   438,   439,   440,    99,    66,     4,   114,     4,
       4,   105,   106,   107,    74,     3,     4,    16,     6,    79,
      59,   146,   145,    16,   118,   150,    59,   463,   153,   154,
     466,   467,   149,   469,    30,    31,    32,    33,    34,    99,
     476,   477,    30,    31,   149,   105,   106,   107,   484,    37,
      38,    39,   429,     4,     4,   127,   128,   129,   118,     4,
       4,     4,     4,    51,    52,    53,   138,     4,     4,     4,
      37,     4,    60,     4,     4,   147,    37,    65,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    75,    58,     4,
      37,    37,    80,    81,    82,    83,    84,    85,    86,    37,
     148,   148,   148,   539,   148,    37,    94,     4,    96,    37,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    36,   588,    38,   112,    37,    37,    37,    37,    37,
     118,   119,     6,     7,     8,     9,    10,    11,    12,    13,
      14,   577,     3,     4,    37,     6,    37,   583,    37,   585,
      37,    66,    37,   530,   531,    37,    37,   623,   146,    37,
     148,   597,    37,    58,    37,   153,   154,    37,   149,    30,
      31,    59,   149,    59,   150,   148,    37,    38,    39,   150,
       6,   150,   618,     6,   150,   150,   150,   102,   150,   625,
      51,    52,    53,   150,   109,   110,   111,   112,   113,    60,
     115,   116,   117,     6,    65,   120,   121,     4,   123,   124,
     125,   150,     4,     4,    75,   130,   131,   132,   793,    80,
      81,    82,    83,    84,    85,    86,   150,   660,   150,   150,
     805,   806,     4,    94,     4,    96,   702,     4,     4,     4,
       3,     4,    28,    29,    30,    31,    32,    33,    34,     4,
       4,   112,     4,     4,     4,     4,     4,   118,   119,   149,
       4,    37,     4,    37,     4,   149,    37,    30,    31,   148,
     150,    58,   148,   150,    37,    38,    39,    40,   150,   150,
      16,   150,   150,   150,   720,   146,   150,   148,    51,    52,
      53,   150,   153,   154,    59,   150,   150,    60,   150,   150,
       4,    66,    65,     4,   149,    16,    37,    16,   103,    74,
      92,     4,    75,     4,    79,    94,    36,    80,    81,    82,
      83,    84,    85,    86,     3,     4,   150,    37,   150,    59,
     150,    94,    16,    96,    99,    37,    58,    16,    38,    37,
     105,   106,   107,    37,   150,     6,   148,    37,   149,   112,
     144,    30,    31,   118,    59,   118,   119,   104,    37,    38,
      39,   149,    96,    37,     6,    37,    16,    37,    16,    58,
      58,    37,    51,    52,    53,    37,    59,    37,    37,    37,
      37,    60,    37,   146,    37,    37,    65,    58,    37,    25,
     153,   154,    37,     4,    37,     4,    75,    37,    37,    37,
     103,    80,    81,    82,    83,    84,    85,    86,     3,     4,
      37,   150,    37,   150,    37,    94,    37,    96,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,   112,   150,    30,    31,   150,   150,   118,
     119,    59,    37,    38,    39,    25,    36,     4,     4,     4,
       4,   150,   250,   150,   150,   150,    51,    52,    53,   218,
     145,   741,   240,   150,   150,    60,   721,   146,   150,   150,
      65,   676,   150,   150,   153,   154,   780,   150,   506,   342,
      75,     4,   150,   150,   761,    80,    81,    82,    83,    84,
      85,    86,    15,   445,   150,   548,    26,   599,   439,    94,
     732,    96,   247,   101,   573,    -1,    -1,    -1,    -1,    32,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,    41,    42,
      43,    44,    45,   118,   119,    -1,    -1,    -1,    -1,    -1,
      -1,    54,    55,    56,    57,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    66,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,   153,   154,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    95,    -1,    97,     4,    -1,    -1,   101,    -1,
      -1,    -1,   105,   106,   107,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,    15,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,   139,    46,    47,    48,
     143,    -1,   145,    -1,    -1,    -1,   149,    -1,   151,    -1,
      -1,    -1,    61,    62,    63,    64,    -1,    66,    67,    68,
      69,     4,    -1,    -1,    -1,    -1,    -1,    76,    77,    78,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,    88,
      89,    90,    91,    92,    93,    -1,    -1,    -1,    -1,    98,
      99,   100,    -1,    36,    -1,    -1,   105,   106,   107,    -1,
      -1,    -1,    -1,    46,    47,    48,    -1,    -1,    -1,   118,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    62,
      63,    64,   131,    66,    67,    68,    69,    -1,   137,    -1,
      -1,    -1,    -1,    76,    77,    78,    79,    -1,    -1,    -1,
     149,    -1,    -1,    -1,    87,    88,    89,    90,    91,    92,
      93,   150,    -1,    -1,    -1,    98,    99,   100,    -1,    -1,
      -1,    -1,   105,   106,   107,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   131,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   149,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,   148,    15,
     150,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     150,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     150,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     150,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     150,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     150,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   150,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   150,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   150,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   150,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   150,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   150,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,   150,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   150,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   150,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   150,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   150,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   150,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,
     148,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     148,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     148,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
     148,   682,   683,   684,   685,    -1,    -1,    -1,     4,    -1,
     691,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     148,    15,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    38,    37,   725,   726,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   738,   739,    -1,
     741,    -1,   743,   744,    -1,    -1,    -1,    -1,    -1,    -1,
      66,    -1,   148,    -1,   755,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   768,    -1,    -1,
      -1,    -1,   148,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   785,    -1,   102,    -1,    -1,    -1,
      -1,    -1,   148,   109,   110,   111,   112,   113,   114,   115,
     116,   117,    -1,    -1,   120,   121,    -1,   123,   124,   125,
      -1,    -1,    -1,    -1,   130,   131,   132,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   127,   128,   129,   138,   147,   156,   172,   173,   160,
     161,   158,   159,   274,   275,   269,   270,     0,   174,   162,
       4,    58,   133,   278,   279,    58,   271,   272,     4,    46,
      47,    48,    61,    62,    63,    64,    66,    67,    68,    69,
      76,    77,    78,    79,    87,    88,    89,    90,    91,    92,
      93,    98,    99,   100,   105,   106,   107,   118,   131,   149,
     175,   184,   186,   207,   209,   220,   221,   223,   225,   261,
     276,   277,     4,    38,    66,   102,   109,   110,   111,   112,
     113,   115,   116,   117,   120,   121,   123,   124,   125,   130,
     131,   132,   163,     6,     4,   131,   134,   135,   136,   281,
     282,    58,   279,   135,   136,   273,   282,   272,     6,     7,
       8,     9,    10,    11,    12,    13,    14,   205,    58,    58,
      49,    50,    37,    37,     4,   157,    58,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,   176,
      37,    37,    37,    37,   187,    37,   148,   149,   206,   137,
       4,   157,     4,     3,     4,    30,    31,    37,    38,    39,
      51,    52,    53,    60,    65,    75,    80,    81,    82,    83,
      84,    85,    86,    94,    96,   112,   118,   119,   146,   153,
     154,   229,     4,     4,   167,     4,   166,   165,     4,     4,
       4,   229,     4,     3,     4,   168,   169,   170,     4,   114,
     229,     4,    16,    16,    59,   149,   281,    59,   149,   227,
     228,   227,   185,   262,     4,     4,     4,     4,   177,     4,
      66,   210,   211,   212,     4,     4,     4,   157,   157,     4,
     157,   150,   157,   222,   224,     4,   226,   226,   178,   179,
      37,   157,     4,     4,     4,    37,   169,    58,     6,   148,
     164,     6,   148,   229,   229,   229,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,   229,   229,    15,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,   148,     6,   148,
     229,   148,   148,     4,   148,     6,   148,   229,   148,   148,
       4,   171,    58,   282,   282,   149,     4,   131,   134,   135,
     136,    59,   149,   229,     4,    59,    66,    74,    99,   186,
     236,     4,    59,   263,   148,   150,   150,   174,   213,   157,
      59,   148,   208,   148,   150,   150,   150,   150,   150,   150,
     150,   208,   150,   208,   148,   226,   150,   150,   150,     4,
       5,   108,   180,   178,   150,     6,     6,     6,   229,   150,
     278,   229,   229,   162,   229,   229,   150,   229,   229,   229,
     229,   229,     4,     4,   229,     4,     4,     4,     4,   229,
     229,     4,     4,   229,   229,   229,     4,   229,   229,   229,
     229,   229,   229,   229,   229,   229,   229,   229,   229,   229,
     229,   229,   229,   229,   229,   229,   229,   229,   229,     4,
       4,     4,   229,   229,     3,     4,     4,     4,   148,   284,
     149,   149,     4,   133,   149,   280,   237,   157,   242,   247,
     264,     4,    36,    37,   217,   214,   212,     4,   157,   157,
     226,    37,     4,     5,   108,   148,   150,   227,   227,   227,
     148,    59,    36,   148,   150,   150,   148,   148,   150,   148,
     150,   150,   150,   150,   150,   150,   148,   148,   150,   150,
     150,   148,   150,   150,    16,     4,   282,   136,    58,   133,
     149,    37,    40,   229,   251,   252,   249,    16,   229,   253,
     252,   229,   266,   148,     4,   153,   218,   219,    16,   210,
     150,   181,    37,     4,     5,   108,   150,   150,   150,     4,
     229,   229,   229,   229,   229,   229,     4,   229,   149,   285,
      16,   283,    70,    71,    72,    73,    74,   150,   250,    37,
      37,   251,    16,   103,   231,   185,    16,    92,   254,   248,
       4,   103,   267,     4,     4,   150,   219,    94,   215,    36,
     180,   183,    37,   150,   150,   150,   150,   150,   150,   150,
     150,    59,   282,   282,   150,   229,    16,    37,    38,   232,
      36,   231,    58,    37,   268,    37,   265,   150,     6,   208,
     150,   180,   182,   149,   285,   150,   229,    37,   144,   233,
     104,   234,   185,   229,   267,   229,   149,   227,    96,   216,
     150,   180,    59,    39,   251,   150,   229,   234,    37,   243,
      59,   150,   150,     6,   150,    37,    16,   150,   238,   229,
      58,   267,   227,   229,   140,   141,   142,   235,   150,   244,
     150,    58,   257,   251,   239,     4,    59,    16,     4,    15,
      32,    41,    42,    43,    44,    45,    54,    55,    56,    57,
      66,    95,    97,   101,   118,   139,   143,   145,   149,   151,
     188,   189,   191,   194,   195,   197,   200,   201,   202,   207,
     258,   245,    37,    37,    37,    37,   157,    37,   198,    37,
      37,    37,     4,    54,   189,   193,    37,     4,   151,   189,
     197,    59,    37,   206,    58,    25,   255,    54,    55,   101,
     145,   188,    54,    55,   188,   188,   188,   199,   203,   227,
      37,   196,     4,   190,   188,   192,    37,   152,   208,   193,
     193,    37,   240,   227,   201,     4,   103,   230,    37,    37,
     150,    37,   150,    37,    37,   150,   150,   150,   201,   150,
     229,     4,   194,    21,   150,   150,   188,   189,   150,   152,
     193,   255,   150,    59,    25,   256,   188,   188,   192,   188,
     188,    36,   148,   150,     4,   188,   150,   230,   259,     4,
       6,    16,   204,   150,   150,   150,   150,   150,     4,   256,
     256,   203,     4,   246,   150,   150,   188,   150,   150,   150,
     204,   204,   208,   150,   206,   241,   260,   208,   208
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
#line 178 "ldgram.y"
    { ldlex_defsym(); }
    break;

  case 9:
#line 180 "ldgram.y"
    {
		  ldlex_popstate();
		  lang_add_assignment (exp_defsym ((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)));
		}
    break;

  case 10:
#line 188 "ldgram.y"
    {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
    break;

  case 11:
#line 193 "ldgram.y"
    {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
    break;

  case 16:
#line 208 "ldgram.y"
    {
			einfo(_("%P%F: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[(1) - (1)].name));
			}
    break;

  case 17:
#line 211 "ldgram.y"
    {
			config.map_filename = "-";
			}
    break;

  case 20:
#line 217 "ldgram.y"
    { mri_public((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)); }
    break;

  case 21:
#line 219 "ldgram.y"
    { mri_public((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)); }
    break;

  case 22:
#line 221 "ldgram.y"
    { mri_public((yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].etree)); }
    break;

  case 23:
#line 223 "ldgram.y"
    { mri_format((yyvsp[(2) - (2)].name)); }
    break;

  case 24:
#line 225 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree));}
    break;

  case 25:
#line 227 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].etree));}
    break;

  case 26:
#line 229 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree));}
    break;

  case 27:
#line 231 "ldgram.y"
    { mri_align((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 28:
#line 233 "ldgram.y"
    { mri_align((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 29:
#line 235 "ldgram.y"
    { mri_alignmod((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 30:
#line 237 "ldgram.y"
    { mri_alignmod((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 33:
#line 241 "ldgram.y"
    { mri_name((yyvsp[(2) - (2)].name)); }
    break;

  case 34:
#line 243 "ldgram.y"
    { mri_alias((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].name),0);}
    break;

  case 35:
#line 245 "ldgram.y"
    { mri_alias ((yyvsp[(2) - (4)].name), 0, (int) (yyvsp[(4) - (4)].bigint).integer); }
    break;

  case 36:
#line 247 "ldgram.y"
    { mri_base((yyvsp[(2) - (2)].etree)); }
    break;

  case 37:
#line 249 "ldgram.y"
    { mri_truncate ((unsigned int) (yyvsp[(2) - (2)].bigint).integer); }
    break;

  case 40:
#line 253 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 41:
#line 255 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 42:
#line 257 "ldgram.y"
    { lang_add_entry ((yyvsp[(2) - (2)].name), FALSE); }
    break;

  case 44:
#line 262 "ldgram.y"
    { mri_order((yyvsp[(3) - (3)].name)); }
    break;

  case 45:
#line 263 "ldgram.y"
    { mri_order((yyvsp[(2) - (2)].name)); }
    break;

  case 47:
#line 269 "ldgram.y"
    { mri_load((yyvsp[(1) - (1)].name)); }
    break;

  case 48:
#line 270 "ldgram.y"
    { mri_load((yyvsp[(3) - (3)].name)); }
    break;

  case 49:
#line 275 "ldgram.y"
    { mri_only_load((yyvsp[(1) - (1)].name)); }
    break;

  case 50:
#line 277 "ldgram.y"
    { mri_only_load((yyvsp[(3) - (3)].name)); }
    break;

  case 51:
#line 281 "ldgram.y"
    { (yyval.name) = NULL; }
    break;

  case 54:
#line 288 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 55:
#line 290 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 56:
#line 294 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(1) - (1)].name), FALSE); }
    break;

  case 57:
#line 296 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(2) - (2)].name), FALSE); }
    break;

  case 58:
#line 298 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(3) - (3)].name), FALSE); }
    break;

  case 59:
#line 302 "ldgram.y"
    { ldlex_both(); }
    break;

  case 60:
#line 304 "ldgram.y"
    { ldlex_popstate(); }
    break;

  case 73:
#line 325 "ldgram.y"
    { lang_add_target((yyvsp[(3) - (4)].name)); }
    break;

  case 74:
#line 327 "ldgram.y"
    { ldfile_add_library_path ((yyvsp[(3) - (4)].name), FALSE); }
    break;

  case 75:
#line 329 "ldgram.y"
    { lang_add_output((yyvsp[(3) - (4)].name), 1); }
    break;

  case 76:
#line 331 "ldgram.y"
    { lang_add_output_format ((yyvsp[(3) - (4)].name), (char *) NULL,
					    (char *) NULL, 1); }
    break;

  case 77:
#line 334 "ldgram.y"
    { lang_add_output_format ((yyvsp[(3) - (8)].name), (yyvsp[(5) - (8)].name), (yyvsp[(7) - (8)].name), 1); }
    break;

  case 78:
#line 336 "ldgram.y"
    { ldfile_set_output_arch ((yyvsp[(3) - (4)].name), bfd_arch_unknown); }
    break;

  case 79:
#line 338 "ldgram.y"
    { command_line.force_common_definition = TRUE ; }
    break;

  case 80:
#line 340 "ldgram.y"
    { command_line.inhibit_common_definition = TRUE ; }
    break;

  case 82:
#line 343 "ldgram.y"
    { lang_enter_group (); }
    break;

  case 83:
#line 345 "ldgram.y"
    { lang_leave_group (); }
    break;

  case 84:
#line 347 "ldgram.y"
    { lang_add_map((yyvsp[(3) - (4)].name)); }
    break;

  case 85:
#line 349 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 86:
#line 351 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 87:
#line 353 "ldgram.y"
    {
		  lang_add_nocrossref ((yyvsp[(3) - (4)].nocrossref));
		}
    break;

  case 88:
#line 357 "ldgram.y"
    {
		  lang_add_nocrossref_to ((yyvsp[(3) - (4)].nocrossref));
		}
    break;

  case 90:
#line 362 "ldgram.y"
    { lang_add_insert ((yyvsp[(3) - (3)].name), 0); }
    break;

  case 91:
#line 364 "ldgram.y"
    { lang_add_insert ((yyvsp[(3) - (3)].name), 1); }
    break;

  case 92:
#line 366 "ldgram.y"
    { lang_memory_region_alias ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].name)); }
    break;

  case 93:
#line 368 "ldgram.y"
    { lang_ld_feature ((yyvsp[(3) - (4)].name)); }
    break;

  case 94:
#line 372 "ldgram.y"
    { ldlex_inputlist(); }
    break;

  case 95:
#line 374 "ldgram.y"
    { ldlex_popstate(); }
    break;

  case 96:
#line 378 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 97:
#line 381 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 98:
#line 384 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 99:
#line 387 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 100:
#line 390 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 101:
#line 393 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 102:
#line 396 "ldgram.y"
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 103:
#line 399 "ldgram.y"
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[(3) - (5)].integer); }
    break;

  case 104:
#line 401 "ldgram.y"
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 105:
#line 404 "ldgram.y"
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[(5) - (7)].integer); }
    break;

  case 106:
#line 406 "ldgram.y"
    { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 107:
#line 409 "ldgram.y"
    { input_flags.add_DT_NEEDED_for_regular = (yyvsp[(4) - (6)].integer); }
    break;

  case 112:
#line 424 "ldgram.y"
    { lang_add_entry ((yyvsp[(3) - (4)].name), FALSE); }
    break;

  case 114:
#line 426 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 115:
#line 427 "ldgram.y"
    { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[(4) - (7)].etree), (yyvsp[(6) - (7)].name))); }
    break;

  case 116:
#line 435 "ldgram.y"
    {
			  (yyval.cname) = (yyvsp[(1) - (1)].name);
			}
    break;

  case 117:
#line 439 "ldgram.y"
    {
			  (yyval.cname) = "*";
			}
    break;

  case 118:
#line 443 "ldgram.y"
    {
			  (yyval.cname) = "?";
			}
    break;

  case 119:
#line 450 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(1) - (1)].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 120:
#line 457 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (5)].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[(3) - (5)].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 121:
#line 464 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 122:
#line 471 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 123:
#line 478 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 124:
#line 485 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_name_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 125:
#line 492 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 126:
#line 499 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_alignment_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 127:
#line 506 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 128:
#line 513 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(7) - (8)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = (yyvsp[(5) - (8)].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 129:
#line 520 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_init_priority;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
    break;

  case 130:
#line 529 "ldgram.y"
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

  case 131:
#line 547 "ldgram.y"
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

  case 132:
#line 568 "ldgram.y"
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

  case 133:
#line 581 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[(2) - (2)].cname);
			  tmp->next = (yyvsp[(1) - (2)].name_list);
			  (yyval.name_list) = tmp;
			}
    break;

  case 134:
#line 590 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[(1) - (1)].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
    break;

  case 135:
#line 601 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[(1) - (3)].wildcard_list);
			  tmp->spec = (yyvsp[(3) - (3)].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 136:
#line 610 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[(1) - (1)].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 137:
#line 621 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[(1) - (1)].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = NULL;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
    break;

  case 138:
#line 630 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[(2) - (2)].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[(1) - (2)].flag_info);
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
    break;

  case 139:
#line 639 "ldgram.y"
    {
			  lang_add_wild (NULL, (yyvsp[(2) - (3)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 140:
#line 643 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = NULL;
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[(1) - (4)].flag_info);
			  lang_add_wild (&tmp, (yyvsp[(3) - (4)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 141:
#line 652 "ldgram.y"
    {
			  lang_add_wild (&(yyvsp[(1) - (4)].wildcard), (yyvsp[(3) - (4)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 142:
#line 656 "ldgram.y"
    {
			  (yyvsp[(2) - (5)].wildcard).section_flag_list = (yyvsp[(1) - (5)].flag_info);
			  lang_add_wild (&(yyvsp[(2) - (5)].wildcard), (yyvsp[(4) - (5)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 144:
#line 665 "ldgram.y"
    { ldgram_had_keep = TRUE; }
    break;

  case 145:
#line 667 "ldgram.y"
    { ldgram_had_keep = FALSE; }
    break;

  case 147:
#line 673 "ldgram.y"
    {
 		lang_add_attribute(lang_object_symbols_statement_enum);
	      	}
    break;

  case 149:
#line 678 "ldgram.y"
    {

		  lang_add_attribute(lang_constructors_statement_enum);
		}
    break;

  case 150:
#line 683 "ldgram.y"
    {
		  constructors_sorted = TRUE;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
    break;

  case 152:
#line 689 "ldgram.y"
    {
			  lang_add_data ((int) (yyvsp[(1) - (4)].integer), (yyvsp[(3) - (4)].etree));
			}
    break;

  case 153:
#line 694 "ldgram.y"
    {
			  lang_add_fill ((yyvsp[(3) - (4)].fill));
			}
    break;

  case 154:
#line 697 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 155:
#line 698 "ldgram.y"
    { ldlex_popstate ();
			  lang_add_assignment (exp_assert ((yyvsp[(4) - (8)].etree), (yyvsp[(6) - (8)].name))); }
    break;

  case 156:
#line 701 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 157:
#line 703 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 162:
#line 718 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 163:
#line 720 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 164:
#line 722 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 165:
#line 724 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 166:
#line 726 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 167:
#line 731 "ldgram.y"
    {
		  (yyval.fill) = exp_get_fill ((yyvsp[(1) - (1)].etree), 0, "fill value");
		}
    break;

  case 168:
#line 738 "ldgram.y"
    { (yyval.fill) = (yyvsp[(2) - (2)].fill); }
    break;

  case 169:
#line 739 "ldgram.y"
    { (yyval.fill) = (fill_type *) 0; }
    break;

  case 170:
#line 744 "ldgram.y"
    { (yyval.token) = '+'; }
    break;

  case 171:
#line 746 "ldgram.y"
    { (yyval.token) = '-'; }
    break;

  case 172:
#line 748 "ldgram.y"
    { (yyval.token) = '*'; }
    break;

  case 173:
#line 750 "ldgram.y"
    { (yyval.token) = '/'; }
    break;

  case 174:
#line 752 "ldgram.y"
    { (yyval.token) = LSHIFT; }
    break;

  case 175:
#line 754 "ldgram.y"
    { (yyval.token) = RSHIFT; }
    break;

  case 176:
#line 756 "ldgram.y"
    { (yyval.token) = '&'; }
    break;

  case 177:
#line 758 "ldgram.y"
    { (yyval.token) = '|'; }
    break;

  case 180:
#line 768 "ldgram.y"
    {
		  lang_add_assignment (exp_assign ((yyvsp[(1) - (3)].name), (yyvsp[(3) - (3)].etree), FALSE));
		}
    break;

  case 181:
#line 772 "ldgram.y"
    {
		  lang_add_assignment (exp_assign ((yyvsp[(1) - (3)].name),
						   exp_binop ((yyvsp[(2) - (3)].token),
							      exp_nameop (NAME,
									  (yyvsp[(1) - (3)].name)),
							      (yyvsp[(3) - (3)].etree)), FALSE));
		}
    break;

  case 182:
#line 780 "ldgram.y"
    {
		  lang_add_assignment (exp_assign ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), TRUE));
		}
    break;

  case 183:
#line 784 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), FALSE));
		}
    break;

  case 184:
#line 788 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), TRUE));
		}
    break;

  case 192:
#line 811 "ldgram.y"
    { region = lang_memory_region_lookup ((yyvsp[(1) - (1)].name), TRUE); }
    break;

  case 193:
#line 814 "ldgram.y"
    {}
    break;

  case 194:
#line 816 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 195:
#line 818 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 196:
#line 823 "ldgram.y"
    {
		  region->origin_exp = (yyvsp[(3) - (3)].etree);
		  region->current = region->origin;
		}
    break;

  case 197:
#line 831 "ldgram.y"
    {
		  region->length_exp = (yyvsp[(3) - (3)].etree);
		}
    break;

  case 198:
#line 838 "ldgram.y"
    { /* dummy action to avoid bison 1.25 error message */ }
    break;

  case 202:
#line 849 "ldgram.y"
    { lang_set_flags (region, (yyvsp[(1) - (1)].name), 0); }
    break;

  case 203:
#line 851 "ldgram.y"
    { lang_set_flags (region, (yyvsp[(2) - (2)].name), 1); }
    break;

  case 204:
#line 856 "ldgram.y"
    { lang_startup((yyvsp[(3) - (4)].name)); }
    break;

  case 206:
#line 862 "ldgram.y"
    { ldemul_hll((char *)NULL); }
    break;

  case 207:
#line 867 "ldgram.y"
    { ldemul_hll((yyvsp[(3) - (3)].name)); }
    break;

  case 208:
#line 869 "ldgram.y"
    { ldemul_hll((yyvsp[(1) - (1)].name)); }
    break;

  case 210:
#line 877 "ldgram.y"
    { ldemul_syslib((yyvsp[(3) - (3)].name)); }
    break;

  case 212:
#line 883 "ldgram.y"
    { lang_float(TRUE); }
    break;

  case 213:
#line 885 "ldgram.y"
    { lang_float(FALSE); }
    break;

  case 214:
#line 890 "ldgram.y"
    {
		  (yyval.nocrossref) = NULL;
		}
    break;

  case 215:
#line 894 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[(1) - (2)].name);
		  n->next = (yyvsp[(2) - (2)].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 216:
#line 903 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[(1) - (3)].name);
		  n->next = (yyvsp[(3) - (3)].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 217:
#line 913 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 218:
#line 915 "ldgram.y"
    { ldlex_popstate (); (yyval.etree)=(yyvsp[(2) - (2)].etree);}
    break;

  case 219:
#line 920 "ldgram.y"
    { (yyval.etree) = exp_unop ('-', (yyvsp[(2) - (2)].etree)); }
    break;

  case 220:
#line 922 "ldgram.y"
    { (yyval.etree) = (yyvsp[(2) - (3)].etree); }
    break;

  case 221:
#line 924 "ldgram.y"
    { (yyval.etree) = exp_unop ((int) (yyvsp[(1) - (4)].integer),(yyvsp[(3) - (4)].etree)); }
    break;

  case 222:
#line 926 "ldgram.y"
    { (yyval.etree) = exp_unop ('!', (yyvsp[(2) - (2)].etree)); }
    break;

  case 223:
#line 928 "ldgram.y"
    { (yyval.etree) = (yyvsp[(2) - (2)].etree); }
    break;

  case 224:
#line 930 "ldgram.y"
    { (yyval.etree) = exp_unop ('~', (yyvsp[(2) - (2)].etree));}
    break;

  case 225:
#line 933 "ldgram.y"
    { (yyval.etree) = exp_binop ('*', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 226:
#line 935 "ldgram.y"
    { (yyval.etree) = exp_binop ('/', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 227:
#line 937 "ldgram.y"
    { (yyval.etree) = exp_binop ('%', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 228:
#line 939 "ldgram.y"
    { (yyval.etree) = exp_binop ('+', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 229:
#line 941 "ldgram.y"
    { (yyval.etree) = exp_binop ('-' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 230:
#line 943 "ldgram.y"
    { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 231:
#line 945 "ldgram.y"
    { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 232:
#line 947 "ldgram.y"
    { (yyval.etree) = exp_binop (EQ , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 233:
#line 949 "ldgram.y"
    { (yyval.etree) = exp_binop (NE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 234:
#line 951 "ldgram.y"
    { (yyval.etree) = exp_binop (LE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 235:
#line 953 "ldgram.y"
    { (yyval.etree) = exp_binop (GE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 236:
#line 955 "ldgram.y"
    { (yyval.etree) = exp_binop ('<' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 237:
#line 957 "ldgram.y"
    { (yyval.etree) = exp_binop ('>' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 238:
#line 959 "ldgram.y"
    { (yyval.etree) = exp_binop ('&' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 239:
#line 961 "ldgram.y"
    { (yyval.etree) = exp_binop ('^' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 240:
#line 963 "ldgram.y"
    { (yyval.etree) = exp_binop ('|' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 241:
#line 965 "ldgram.y"
    { (yyval.etree) = exp_trinop ('?' , (yyvsp[(1) - (5)].etree), (yyvsp[(3) - (5)].etree), (yyvsp[(5) - (5)].etree)); }
    break;

  case 242:
#line 967 "ldgram.y"
    { (yyval.etree) = exp_binop (ANDAND , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 243:
#line 969 "ldgram.y"
    { (yyval.etree) = exp_binop (OROR , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 244:
#line 971 "ldgram.y"
    { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[(3) - (4)].name)); }
    break;

  case 245:
#line 973 "ldgram.y"
    { (yyval.etree) = exp_bigintop ((yyvsp[(1) - (1)].bigint).integer, (yyvsp[(1) - (1)].bigint).str); }
    break;

  case 246:
#line 975 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
    break;

  case 247:
#line 978 "ldgram.y"
    { (yyval.etree) = exp_nameop (ALIGNOF,(yyvsp[(3) - (4)].name)); }
    break;

  case 248:
#line 980 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF,(yyvsp[(3) - (4)].name)); }
    break;

  case 249:
#line 982 "ldgram.y"
    { (yyval.etree) = exp_nameop (ADDR,(yyvsp[(3) - (4)].name)); }
    break;

  case 250:
#line 984 "ldgram.y"
    { (yyval.etree) = exp_nameop (LOADADDR,(yyvsp[(3) - (4)].name)); }
    break;

  case 251:
#line 986 "ldgram.y"
    { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[(3) - (4)].name)); }
    break;

  case 252:
#line 988 "ldgram.y"
    { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[(3) - (4)].etree)); }
    break;

  case 253:
#line 990 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[(3) - (4)].etree)); }
    break;

  case 254:
#line 992 "ldgram.y"
    { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[(3) - (6)].etree),(yyvsp[(5) - (6)].etree)); }
    break;

  case 255:
#line 994 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree)); }
    break;

  case 256:
#line 996 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[(5) - (6)].etree), (yyvsp[(3) - (6)].etree)); }
    break;

  case 257:
#line 998 "ldgram.y"
    { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[(3) - (4)].etree)); }
    break;

  case 258:
#line 1000 "ldgram.y"
    { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[(5) - (6)].etree),
					  exp_nameop (NAME, (yyvsp[(3) - (6)].name))); }
    break;

  case 259:
#line 1009 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[(3) - (4)].etree)); }
    break;

  case 260:
#line 1011 "ldgram.y"
    { (yyval.etree) = exp_nameop (NAME,(yyvsp[(1) - (1)].name)); }
    break;

  case 261:
#line 1013 "ldgram.y"
    { (yyval.etree) = exp_binop (MAX_K, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree) ); }
    break;

  case 262:
#line 1015 "ldgram.y"
    { (yyval.etree) = exp_binop (MIN_K, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree) ); }
    break;

  case 263:
#line 1017 "ldgram.y"
    { (yyval.etree) = exp_assert ((yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].name)); }
    break;

  case 264:
#line 1019 "ldgram.y"
    { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[(3) - (4)].name)); }
    break;

  case 265:
#line 1021 "ldgram.y"
    { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[(3) - (4)].name)); }
    break;

  case 266:
#line 1023 "ldgram.y"
    { (yyval.etree) = exp_unop (LOG2CEIL, (yyvsp[(3) - (4)].etree)); }
    break;

  case 267:
#line 1028 "ldgram.y"
    { (yyval.name) = (yyvsp[(3) - (3)].name); }
    break;

  case 268:
#line 1029 "ldgram.y"
    { (yyval.name) = 0; }
    break;

  case 269:
#line 1033 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 270:
#line 1034 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 271:
#line 1038 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 272:
#line 1039 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 273:
#line 1043 "ldgram.y"
    { (yyval.token) = ALIGN_WITH_INPUT; }
    break;

  case 274:
#line 1044 "ldgram.y"
    { (yyval.token) = 0; }
    break;

  case 275:
#line 1048 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 276:
#line 1049 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 277:
#line 1053 "ldgram.y"
    { (yyval.token) = ONLY_IF_RO; }
    break;

  case 278:
#line 1054 "ldgram.y"
    { (yyval.token) = ONLY_IF_RW; }
    break;

  case 279:
#line 1055 "ldgram.y"
    { (yyval.token) = SPECIAL; }
    break;

  case 280:
#line 1056 "ldgram.y"
    { (yyval.token) = 0; }
    break;

  case 281:
#line 1059 "ldgram.y"
    { ldlex_expression(); }
    break;

  case 282:
#line 1064 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 283:
#line 1067 "ldgram.y"
    {
			  lang_enter_output_section_statement((yyvsp[(1) - (10)].name), (yyvsp[(3) - (10)].etree),
							      sectype,
							      (yyvsp[(5) - (10)].etree), (yyvsp[(7) - (10)].etree), (yyvsp[(4) - (10)].etree), (yyvsp[(9) - (10)].token), (yyvsp[(6) - (10)].token));
			}
    break;

  case 284:
#line 1073 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 285:
#line 1075 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_leave_output_section_statement ((yyvsp[(18) - (18)].fill), (yyvsp[(15) - (18)].name), (yyvsp[(17) - (18)].section_phdr), (yyvsp[(16) - (18)].name));
		}
    break;

  case 286:
#line 1080 "ldgram.y"
    {}
    break;

  case 287:
#line 1082 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 288:
#line 1084 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 289:
#line 1086 "ldgram.y"
    {
			  lang_enter_overlay ((yyvsp[(3) - (8)].etree), (yyvsp[(6) - (8)].etree));
			}
    break;

  case 290:
#line 1091 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 291:
#line 1093 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay ((yyvsp[(5) - (16)].etree), (int) (yyvsp[(4) - (16)].integer),
					      (yyvsp[(16) - (16)].fill), (yyvsp[(13) - (16)].name), (yyvsp[(15) - (16)].section_phdr), (yyvsp[(14) - (16)].name));
			}
    break;

  case 293:
#line 1103 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 294:
#line 1105 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assign (".", (yyvsp[(3) - (3)].etree), FALSE));
		}
    break;

  case 296:
#line 1111 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 297:
#line 1113 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 298:
#line 1117 "ldgram.y"
    { sectype = noload_section; }
    break;

  case 299:
#line 1118 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 300:
#line 1119 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 301:
#line 1120 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 302:
#line 1121 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 304:
#line 1126 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 305:
#line 1127 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 306:
#line 1131 "ldgram.y"
    { (yyval.etree) = (yyvsp[(1) - (3)].etree); }
    break;

  case 307:
#line 1132 "ldgram.y"
    { (yyval.etree) = (etree_type *)NULL;  }
    break;

  case 308:
#line 1137 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (6)].etree); }
    break;

  case 309:
#line 1139 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (10)].etree); }
    break;

  case 310:
#line 1143 "ldgram.y"
    { (yyval.etree) = (yyvsp[(1) - (2)].etree); }
    break;

  case 311:
#line 1144 "ldgram.y"
    { (yyval.etree) = (etree_type *) NULL;  }
    break;

  case 312:
#line 1149 "ldgram.y"
    { (yyval.integer) = 0; }
    break;

  case 313:
#line 1151 "ldgram.y"
    { (yyval.integer) = 1; }
    break;

  case 314:
#line 1156 "ldgram.y"
    { (yyval.name) = (yyvsp[(2) - (2)].name); }
    break;

  case 315:
#line 1157 "ldgram.y"
    { (yyval.name) = DEFAULT_MEMORY_REGION; }
    break;

  case 316:
#line 1162 "ldgram.y"
    {
		  (yyval.section_phdr) = NULL;
		}
    break;

  case 317:
#line 1166 "ldgram.y"
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

  case 319:
#line 1182 "ldgram.y"
    {
			  ldlex_script ();
			  lang_enter_overlay_section ((yyvsp[(2) - (2)].name));
			}
    break;

  case 320:
#line 1187 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 321:
#line 1189 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay_section ((yyvsp[(9) - (9)].fill), (yyvsp[(8) - (9)].section_phdr));
			}
    break;

  case 326:
#line 1206 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 327:
#line 1207 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 328:
#line 1209 "ldgram.y"
    {
		  lang_new_phdr ((yyvsp[(1) - (6)].name), (yyvsp[(3) - (6)].etree), (yyvsp[(4) - (6)].phdr).filehdr, (yyvsp[(4) - (6)].phdr).phdrs, (yyvsp[(4) - (6)].phdr).at,
				 (yyvsp[(4) - (6)].phdr).flags);
		}
    break;

  case 329:
#line 1217 "ldgram.y"
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

  case 330:
#line 1261 "ldgram.y"
    {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
    break;

  case 331:
#line 1265 "ldgram.y"
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

  case 332:
#line 1278 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[(5) - (5)].phdr);
		  (yyval.phdr).at = (yyvsp[(3) - (5)].etree);
		}
    break;

  case 333:
#line 1286 "ldgram.y"
    {
		  (yyval.etree) = NULL;
		}
    break;

  case 334:
#line 1290 "ldgram.y"
    {
		  (yyval.etree) = (yyvsp[(2) - (3)].etree);
		}
    break;

  case 335:
#line 1296 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
    break;

  case 336:
#line 1301 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 340:
#line 1318 "ldgram.y"
    {
		  lang_append_dynamic_list ((yyvsp[(1) - (2)].versyms));
		}
    break;

  case 341:
#line 1326 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
    break;

  case 342:
#line 1331 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 343:
#line 1340 "ldgram.y"
    {
		  ldlex_version_script ();
		}
    break;

  case 344:
#line 1344 "ldgram.y"
    {
		  ldlex_popstate ();
		}
    break;

  case 347:
#line 1356 "ldgram.y"
    {
		  lang_register_vers_node (NULL, (yyvsp[(2) - (4)].versnode), NULL);
		}
    break;

  case 348:
#line 1360 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[(1) - (5)].name), (yyvsp[(3) - (5)].versnode), NULL);
		}
    break;

  case 349:
#line 1364 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[(1) - (6)].name), (yyvsp[(3) - (6)].versnode), (yyvsp[(5) - (6)].deflist));
		}
    break;

  case 350:
#line 1371 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[(1) - (1)].name));
		}
    break;

  case 351:
#line 1375 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[(1) - (2)].deflist), (yyvsp[(2) - (2)].name));
		}
    break;

  case 352:
#line 1382 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
    break;

  case 353:
#line 1386 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(1) - (2)].versyms), NULL);
		}
    break;

  case 354:
#line 1390 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(3) - (4)].versyms), NULL);
		}
    break;

  case 355:
#line 1394 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[(3) - (4)].versyms));
		}
    break;

  case 356:
#line 1398 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(3) - (8)].versyms), (yyvsp[(7) - (8)].versyms));
		}
    break;

  case 357:
#line 1405 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[(1) - (1)].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 358:
#line 1409 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[(1) - (1)].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 359:
#line 1413 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), (yyvsp[(3) - (3)].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 360:
#line 1417 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), (yyvsp[(3) - (3)].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 361:
#line 1421 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[(4) - (5)].name);
			}
    break;

  case 362:
#line 1426 "ldgram.y"
    {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[(7) - (9)].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[(1) - (9)].versyms);
			  (yyval.versyms) = (yyvsp[(7) - (9)].versyms);
			  ldgram_vers_current_lang = (yyvsp[(6) - (9)].name);
			}
    break;

  case 363:
#line 1434 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[(2) - (3)].name);
			}
    break;

  case 364:
#line 1439 "ldgram.y"
    {
			  (yyval.versyms) = (yyvsp[(5) - (7)].versyms);
			  ldgram_vers_current_lang = (yyvsp[(4) - (7)].name);
			}
    break;

  case 365:
#line 1444 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 366:
#line 1448 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 367:
#line 1452 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 368:
#line 1456 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 369:
#line 1460 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 370:
#line 1464 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "extern", ldgram_vers_current_lang, FALSE);
		}
    break;


/* Line 1267 of yacc.c.  */
#line 4512 "ldgram.c"
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


#line 1474 "ldgram.y"

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

