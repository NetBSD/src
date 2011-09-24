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
     SIZEOF_HEADERS = 297,
     OUTPUT_FORMAT = 298,
     FORCE_COMMON_ALLOCATION = 299,
     OUTPUT_ARCH = 300,
     INHIBIT_COMMON_ALLOCATION = 301,
     SEGMENT_START = 302,
     INCLUDE = 303,
     MEMORY = 304,
     REGION_ALIAS = 305,
     LD_FEATURE = 306,
     NOLOAD = 307,
     DSECT = 308,
     COPY = 309,
     INFO = 310,
     OVERLAY = 311,
     DEFINED = 312,
     TARGET_K = 313,
     SEARCH_DIR = 314,
     MAP = 315,
     ENTRY = 316,
     NEXT = 317,
     SIZEOF = 318,
     ALIGNOF = 319,
     ADDR = 320,
     LOADADDR = 321,
     MAX_K = 322,
     MIN_K = 323,
     STARTUP = 324,
     HLL = 325,
     SYSLIB = 326,
     FLOAT = 327,
     NOFLOAT = 328,
     NOCROSSREFS = 329,
     ORIGIN = 330,
     FILL = 331,
     LENGTH = 332,
     CREATE_OBJECT_SYMBOLS = 333,
     INPUT = 334,
     GROUP = 335,
     OUTPUT = 336,
     CONSTRUCTORS = 337,
     ALIGNMOD = 338,
     AT = 339,
     SUBALIGN = 340,
     PROVIDE = 341,
     PROVIDE_HIDDEN = 342,
     AS_NEEDED = 343,
     CHIP = 344,
     LIST = 345,
     SECT = 346,
     ABSOLUTE = 347,
     LOAD = 348,
     NEWLINE = 349,
     ENDWORD = 350,
     ORDER = 351,
     NAMEWORD = 352,
     ASSERT_K = 353,
     FORMAT = 354,
     PUBLIC = 355,
     DEFSYMEND = 356,
     BASE = 357,
     ALIAS = 358,
     TRUNCATE = 359,
     REL = 360,
     INPUT_SCRIPT = 361,
     INPUT_MRI_SCRIPT = 362,
     INPUT_DEFSYM = 363,
     CASE = 364,
     EXTERN = 365,
     START = 366,
     VERS_TAG = 367,
     VERS_IDENTIFIER = 368,
     GLOBAL = 369,
     LOCAL = 370,
     VERSIONK = 371,
     INPUT_VERSION_SCRIPT = 372,
     KEEP = 373,
     ONLY_IF_RO = 374,
     ONLY_IF_RW = 375,
     SPECIAL = 376,
     EXCLUDE_FILE = 377,
     CONSTANT = 378,
     INPUT_DYNAMIC_LIST = 379
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
#define SIZEOF_HEADERS 297
#define OUTPUT_FORMAT 298
#define FORCE_COMMON_ALLOCATION 299
#define OUTPUT_ARCH 300
#define INHIBIT_COMMON_ALLOCATION 301
#define SEGMENT_START 302
#define INCLUDE 303
#define MEMORY 304
#define REGION_ALIAS 305
#define LD_FEATURE 306
#define NOLOAD 307
#define DSECT 308
#define COPY 309
#define INFO 310
#define OVERLAY 311
#define DEFINED 312
#define TARGET_K 313
#define SEARCH_DIR 314
#define MAP 315
#define ENTRY 316
#define NEXT 317
#define SIZEOF 318
#define ALIGNOF 319
#define ADDR 320
#define LOADADDR 321
#define MAX_K 322
#define MIN_K 323
#define STARTUP 324
#define HLL 325
#define SYSLIB 326
#define FLOAT 327
#define NOFLOAT 328
#define NOCROSSREFS 329
#define ORIGIN 330
#define FILL 331
#define LENGTH 332
#define CREATE_OBJECT_SYMBOLS 333
#define INPUT 334
#define GROUP 335
#define OUTPUT 336
#define CONSTRUCTORS 337
#define ALIGNMOD 338
#define AT 339
#define SUBALIGN 340
#define PROVIDE 341
#define PROVIDE_HIDDEN 342
#define AS_NEEDED 343
#define CHIP 344
#define LIST 345
#define SECT 346
#define ABSOLUTE 347
#define LOAD 348
#define NEWLINE 349
#define ENDWORD 350
#define ORDER 351
#define NAMEWORD 352
#define ASSERT_K 353
#define FORMAT 354
#define PUBLIC 355
#define DEFSYMEND 356
#define BASE 357
#define ALIAS 358
#define TRUNCATE 359
#define REL 360
#define INPUT_SCRIPT 361
#define INPUT_MRI_SCRIPT 362
#define INPUT_DEFSYM 363
#define CASE 364
#define EXTERN 365
#define START 366
#define VERS_TAG 367
#define VERS_IDENTIFIER 368
#define GLOBAL 369
#define LOCAL 370
#define VERSIONK 371
#define INPUT_VERSION_SCRIPT 372
#define KEEP 373
#define ONLY_IF_RO 374
#define ONLY_IF_RW 375
#define SPECIAL 376
#define EXCLUDE_FILE 377
#define CONSTANT 378
#define INPUT_DYNAMIC_LIST 379




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
#line 412 "ldgram.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 425 "ldgram.c"

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
#define YYLAST   1922

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  148
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  126
/* YYNRULES -- Number of rules.  */
#define YYNRULES  357
/* YYNRULES -- Number of states.  */
#define YYNSTATES  767

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   379

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   146,     2,     2,     2,    34,    21,     2,
      37,   143,    32,    30,   141,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   142,
      24,     6,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   144,     2,   145,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    56,    19,    57,   147,     2,     2,     2,
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
      49,    50,    51,    52,    53,    54,    55,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140
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
     400,   408,   416,   424,   432,   441,   444,   446,   450,   452,
     454,   458,   463,   465,   466,   472,   475,   477,   479,   481,
     486,   488,   493,   498,   499,   508,   509,   515,   518,   520,
     521,   523,   525,   527,   529,   531,   533,   535,   538,   539,
     541,   543,   545,   547,   549,   551,   553,   555,   557,   559,
     563,   567,   574,   581,   583,   584,   589,   591,   592,   596,
     598,   599,   607,   608,   614,   618,   622,   623,   627,   629,
     632,   634,   637,   642,   647,   651,   655,   657,   662,   666,
     667,   669,   671,   672,   675,   679,   680,   683,   686,   690,
     695,   698,   701,   704,   708,   712,   716,   720,   724,   728,
     732,   736,   740,   744,   748,   752,   756,   760,   764,   768,
     774,   778,   782,   787,   789,   791,   796,   801,   806,   811,
     816,   821,   826,   833,   840,   847,   852,   859,   864,   866,
     873,   880,   887,   892,   897,   901,   902,   907,   908,   913,
     914,   919,   920,   922,   924,   926,   927,   928,   929,   930,
     931,   932,   952,   953,   954,   955,   956,   957,   976,   977,
     978,   986,   987,   993,   995,   997,   999,  1001,  1003,  1007,
    1008,  1011,  1015,  1018,  1025,  1036,  1039,  1041,  1042,  1044,
    1047,  1048,  1049,  1053,  1054,  1055,  1056,  1057,  1069,  1074,
    1075,  1078,  1079,  1080,  1087,  1089,  1090,  1094,  1100,  1101,
    1105,  1106,  1109,  1111,  1114,  1119,  1122,  1123,  1126,  1127,
    1133,  1135,  1138,  1143,  1149,  1156,  1158,  1161,  1162,  1165,
    1170,  1175,  1184,  1186,  1188,  1192,  1196,  1197,  1207,  1208,
    1216,  1218,  1222,  1224,  1228,  1230,  1234,  1235
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     149,     0,    -1,   122,   165,    -1,   123,   153,    -1,   133,
     262,    -1,   140,   257,    -1,   124,   151,    -1,     4,    -1,
      -1,   152,     4,     6,   218,    -1,    -1,   154,   155,    -1,
     155,   156,   110,    -1,    -1,   105,   218,    -1,   105,   218,
     141,   218,    -1,     4,    -1,   106,    -1,   112,   158,    -1,
     111,    -1,   116,     4,     6,   218,    -1,   116,     4,   141,
     218,    -1,   116,     4,   218,    -1,   115,     4,    -1,   107,
       4,   141,   218,    -1,   107,     4,   218,    -1,   107,     4,
       6,   218,    -1,    38,     4,     6,   218,    -1,    38,     4,
     141,   218,    -1,    99,     4,     6,   218,    -1,    99,     4,
     141,   218,    -1,   108,   160,    -1,   109,   159,    -1,   113,
       4,    -1,   119,     4,   141,     4,    -1,   119,     4,   141,
       3,    -1,   118,   218,    -1,   120,     3,    -1,   125,   161,
      -1,   126,   162,    -1,    -1,    64,   150,   157,   155,    36,
      -1,   127,     4,    -1,    -1,   158,   141,     4,    -1,   158,
       4,    -1,    -1,     4,    -1,   159,   141,     4,    -1,     4,
      -1,   160,   141,     4,    -1,    -1,     4,    -1,   161,   141,
       4,    -1,    -1,   163,   164,    -1,     4,    -1,   164,     4,
      -1,   164,   141,     4,    -1,    -1,   166,   167,    -1,   167,
     168,    -1,    -1,   198,    -1,   175,    -1,   249,    -1,   209,
      -1,   210,    -1,   212,    -1,   214,    -1,   177,    -1,   264,
      -1,   142,    -1,    74,    37,     4,   143,    -1,    75,    37,
     150,   143,    -1,    97,    37,   150,   143,    -1,    59,    37,
       4,   143,    -1,    59,    37,     4,   141,     4,   141,     4,
     143,    -1,    61,    37,     4,   143,    -1,    60,    -1,    62,
      -1,    95,    37,   171,   143,    -1,    -1,    96,   169,    37,
     171,   143,    -1,    76,    37,   150,   143,    -1,    -1,    64,
     150,   170,   167,    36,    -1,    90,    37,   215,   143,    -1,
     126,    37,   162,   143,    -1,    48,    49,     4,    -1,    48,
      50,     4,    -1,    66,    37,     4,   141,     4,   143,    -1,
      67,    37,     4,   143,    -1,     4,    -1,   171,   141,     4,
      -1,   171,     4,    -1,     5,    -1,   171,   141,     5,    -1,
     171,     5,    -1,    -1,   104,    37,   172,   171,   143,    -1,
      -1,   171,   141,   104,    37,   173,   171,   143,    -1,    -1,
     171,   104,    37,   174,   171,   143,    -1,    46,    56,   176,
      57,    -1,   176,   224,    -1,   176,   177,    -1,    -1,    77,
      37,     4,   143,    -1,   196,   195,    -1,    -1,   114,   178,
      37,   218,   141,     4,   143,    -1,     4,    -1,    32,    -1,
      15,    -1,   179,    -1,   138,    37,   181,   143,   179,    -1,
      54,    37,   179,   143,    -1,    55,    37,   179,   143,    -1,
      54,    37,    55,    37,   179,   143,   143,    -1,    54,    37,
      54,    37,   179,   143,   143,    -1,    55,    37,    54,    37,
     179,   143,   143,    -1,    55,    37,    55,    37,   179,   143,
     143,    -1,    54,    37,   138,    37,   181,   143,   179,   143,
      -1,   181,   179,    -1,   179,    -1,   182,   197,   180,    -1,
     180,    -1,     4,    -1,   144,   182,   145,    -1,   180,    37,
     182,   143,    -1,   183,    -1,    -1,   134,    37,   185,   183,
     143,    -1,   196,   195,    -1,    94,    -1,   142,    -1,    98,
      -1,    54,    37,    98,   143,    -1,   184,    -1,   191,    37,
     216,   143,    -1,    92,    37,   192,   143,    -1,    -1,   114,
     187,    37,   218,   141,     4,   143,   195,    -1,    -1,    64,
     150,   188,   190,    36,    -1,   189,   186,    -1,   186,    -1,
      -1,   189,    -1,    41,    -1,    42,    -1,    43,    -1,    44,
      -1,    45,    -1,   216,    -1,     6,   192,    -1,    -1,    14,
      -1,    13,    -1,    12,    -1,    11,    -1,    10,    -1,     9,
      -1,     8,    -1,     7,    -1,   142,    -1,   141,    -1,     4,
       6,   216,    -1,     4,   194,   216,    -1,   102,    37,     4,
       6,   216,   143,    -1,   103,    37,     4,     6,   216,   143,
      -1,   141,    -1,    -1,    65,    56,   199,    57,    -1,   200,
      -1,    -1,   200,   197,   201,    -1,   201,    -1,    -1,     4,
     202,   206,    16,   204,   197,   205,    -1,    -1,    64,   150,
     203,   199,    36,    -1,    91,     6,   216,    -1,    93,     6,
     216,    -1,    -1,    37,   207,   143,    -1,   208,    -1,   207,
     208,    -1,     4,    -1,   146,     4,    -1,    85,    37,   150,
     143,    -1,    86,    37,   211,   143,    -1,    86,    37,   143,
      -1,   211,   197,   150,    -1,   150,    -1,    87,    37,   213,
     143,    -1,   213,   197,   150,    -1,    -1,    88,    -1,    89,
      -1,    -1,     4,   215,    -1,     4,   141,   215,    -1,    -1,
     217,   218,    -1,    31,   218,    -1,    37,   218,   143,    -1,
      78,    37,   218,   143,    -1,   146,   218,    -1,    30,   218,
      -1,   147,   218,    -1,   218,    32,   218,    -1,   218,    33,
     218,    -1,   218,    34,   218,    -1,   218,    30,   218,    -1,
     218,    31,   218,    -1,   218,    29,   218,    -1,   218,    28,
     218,    -1,   218,    23,   218,    -1,   218,    22,   218,    -1,
     218,    27,   218,    -1,   218,    26,   218,    -1,   218,    24,
     218,    -1,   218,    25,   218,    -1,   218,    21,   218,    -1,
     218,    20,   218,    -1,   218,    19,   218,    -1,   218,    15,
     218,    16,   218,    -1,   218,    18,   218,    -1,   218,    17,
     218,    -1,    73,    37,     4,   143,    -1,     3,    -1,    58,
      -1,    80,    37,     4,   143,    -1,    79,    37,     4,   143,
      -1,    81,    37,     4,   143,    -1,    82,    37,     4,   143,
      -1,   139,    37,     4,   143,    -1,   108,    37,   218,   143,
      -1,    38,    37,   218,   143,    -1,    38,    37,   218,   141,
     218,   143,    -1,    51,    37,   218,   141,   218,   143,    -1,
      52,    37,   218,   141,   218,   143,    -1,    53,    37,   218,
     143,    -1,    63,    37,     4,   141,   218,   143,    -1,    39,
      37,   218,   143,    -1,     4,    -1,    83,    37,   218,   141,
     218,   143,    -1,    84,    37,   218,   141,   218,   143,    -1,
     114,    37,   218,   141,     4,   143,    -1,    91,    37,     4,
     143,    -1,    93,    37,     4,   143,    -1,   100,    25,     4,
      -1,    -1,   100,    37,   218,   143,    -1,    -1,    38,    37,
     218,   143,    -1,    -1,   101,    37,   218,   143,    -1,    -1,
     135,    -1,   136,    -1,   137,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     4,   225,   240,   220,   221,   222,   226,   223,
      56,   227,   190,    57,   228,   243,   219,   244,   193,   229,
     197,    -1,    -1,    -1,    -1,    -1,    -1,    72,   230,   241,
     242,   220,   222,   231,    56,   232,   245,    57,   233,   243,
     219,   244,   193,   234,   197,    -1,    -1,    -1,    96,   235,
     240,   236,    56,   176,    57,    -1,    -1,    64,   150,   237,
     176,    36,    -1,    68,    -1,    69,    -1,    70,    -1,    71,
      -1,    72,    -1,    37,   238,   143,    -1,    -1,    37,   143,
      -1,   218,   239,    16,    -1,   239,    16,    -1,    40,    37,
     218,   143,   239,    16,    -1,    40,    37,   218,   143,    39,
      37,   218,   143,   239,    16,    -1,   218,    16,    -1,    16,
      -1,    -1,    90,    -1,    25,     4,    -1,    -1,    -1,   244,
      16,     4,    -1,    -1,    -1,    -1,    -1,   245,     4,   246,
      56,   190,    57,   247,   244,   193,   248,   197,    -1,    47,
      56,   250,    57,    -1,    -1,   250,   251,    -1,    -1,    -1,
       4,   252,   254,   255,   253,   142,    -1,   218,    -1,    -1,
       4,   256,   255,    -1,   100,    37,   218,   143,   255,    -1,
      -1,    37,   218,   143,    -1,    -1,   258,   259,    -1,   260,
      -1,   259,   260,    -1,    56,   261,    57,   142,    -1,   270,
     142,    -1,    -1,   263,   266,    -1,    -1,   265,   132,    56,
     266,    57,    -1,   267,    -1,   266,   267,    -1,    56,   269,
      57,   142,    -1,   128,    56,   269,    57,   142,    -1,   128,
      56,   269,    57,   268,   142,    -1,   128,    -1,   268,   128,
      -1,    -1,   270,   142,    -1,   130,    16,   270,   142,    -1,
     131,    16,   270,   142,    -1,   130,    16,   270,   142,   131,
      16,   270,   142,    -1,   129,    -1,     4,    -1,   270,   142,
     129,    -1,   270,   142,     4,    -1,    -1,   270,   142,   126,
       4,    56,   271,   270,   273,    57,    -1,    -1,   126,     4,
      56,   272,   270,   273,    57,    -1,   130,    -1,   270,   142,
     130,    -1,   131,    -1,   270,   142,   131,    -1,   126,    -1,
     270,   142,   126,    -1,    -1,   142,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   163,   163,   164,   165,   166,   167,   171,   175,   175,
     185,   185,   198,   199,   203,   204,   205,   208,   211,   212,
     213,   215,   217,   219,   221,   223,   225,   227,   229,   231,
     233,   235,   236,   237,   239,   241,   243,   245,   247,   248,
     250,   249,   253,   255,   259,   260,   261,   265,   267,   271,
     273,   278,   279,   280,   285,   285,   290,   292,   294,   299,
     299,   305,   306,   311,   312,   313,   314,   315,   316,   317,
     318,   319,   320,   321,   323,   325,   327,   330,   332,   334,
     336,   338,   340,   339,   343,   346,   345,   349,   353,   354,
     356,   358,   360,   365,   368,   371,   374,   377,   380,   384,
     383,   388,   387,   392,   391,   398,   402,   403,   404,   408,
     410,   411,   411,   419,   423,   427,   434,   440,   446,   452,
     458,   464,   470,   476,   482,   491,   500,   511,   520,   531,
     539,   543,   550,   552,   551,   558,   559,   563,   564,   569,
     574,   575,   580,   584,   584,   588,   587,   594,   595,   598,
     600,   604,   606,   608,   610,   612,   617,   624,   626,   630,
     632,   634,   636,   638,   640,   642,   644,   649,   649,   654,
     658,   666,   670,   678,   678,   682,   685,   685,   688,   689,
     694,   693,   699,   698,   705,   713,   721,   722,   726,   727,
     731,   733,   738,   743,   744,   749,   751,   757,   759,   761,
     765,   767,   773,   776,   785,   796,   796,   802,   804,   806,
     808,   810,   812,   815,   817,   819,   821,   823,   825,   827,
     829,   831,   833,   835,   837,   839,   841,   843,   845,   847,
     849,   851,   853,   855,   857,   860,   862,   864,   866,   868,
     870,   872,   874,   876,   878,   880,   882,   891,   893,   895,
     897,   899,   901,   903,   909,   910,   914,   915,   919,   920,
     924,   925,   929,   930,   931,   932,   935,   939,   942,   948,
     950,   935,   957,   959,   961,   966,   968,   956,   978,   980,
     978,   986,   985,   992,   993,   994,   995,   996,  1000,  1001,
    1002,  1006,  1007,  1012,  1013,  1018,  1019,  1024,  1025,  1030,
    1032,  1037,  1040,  1053,  1057,  1062,  1064,  1055,  1072,  1075,
    1077,  1081,  1082,  1081,  1091,  1136,  1139,  1151,  1160,  1163,
    1170,  1170,  1182,  1183,  1187,  1191,  1200,  1200,  1214,  1214,
    1224,  1225,  1229,  1233,  1237,  1244,  1248,  1256,  1259,  1263,
    1267,  1271,  1278,  1282,  1286,  1290,  1295,  1294,  1308,  1307,
    1317,  1321,  1325,  1329,  1333,  1337,  1343,  1345
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
  "DATA_SEGMENT_END", "SORT_BY_NAME", "SORT_BY_ALIGNMENT", "'{'", "'}'",
  "SIZEOF_HEADERS", "OUTPUT_FORMAT", "FORCE_COMMON_ALLOCATION",
  "OUTPUT_ARCH", "INHIBIT_COMMON_ALLOCATION", "SEGMENT_START", "INCLUDE",
  "MEMORY", "REGION_ALIAS", "LD_FEATURE", "NOLOAD", "DSECT", "COPY",
  "INFO", "OVERLAY", "DEFINED", "TARGET_K", "SEARCH_DIR", "MAP", "ENTRY",
  "NEXT", "SIZEOF", "ALIGNOF", "ADDR", "LOADADDR", "MAX_K", "MIN_K",
  "STARTUP", "HLL", "SYSLIB", "FLOAT", "NOFLOAT", "NOCROSSREFS", "ORIGIN",
  "FILL", "LENGTH", "CREATE_OBJECT_SYMBOLS", "INPUT", "GROUP", "OUTPUT",
  "CONSTRUCTORS", "ALIGNMOD", "AT", "SUBALIGN", "PROVIDE",
  "PROVIDE_HIDDEN", "AS_NEEDED", "CHIP", "LIST", "SECT", "ABSOLUTE",
  "LOAD", "NEWLINE", "ENDWORD", "ORDER", "NAMEWORD", "ASSERT_K", "FORMAT",
  "PUBLIC", "DEFSYMEND", "BASE", "ALIAS", "TRUNCATE", "REL",
  "INPUT_SCRIPT", "INPUT_MRI_SCRIPT", "INPUT_DEFSYM", "CASE", "EXTERN",
  "START", "VERS_TAG", "VERS_IDENTIFIER", "GLOBAL", "LOCAL", "VERSIONK",
  "INPUT_VERSION_SCRIPT", "KEEP", "ONLY_IF_RO", "ONLY_IF_RW", "SPECIAL",
  "EXCLUDE_FILE", "CONSTANT", "INPUT_DYNAMIC_LIST", "','", "';'", "')'",
  "'['", "']'", "'!'", "'~'", "$accept", "file", "filename", "defsym_expr",
  "@1", "mri_script_file", "@2", "mri_script_lines", "mri_script_command",
  "@3", "ordernamelist", "mri_load_name_list", "mri_abs_name_list",
  "casesymlist", "extern_name_list", "@4", "extern_name_list_body",
  "script_file", "@5", "ifile_list", "ifile_p1", "@6", "@7", "input_list",
  "@8", "@9", "@10", "sections", "sec_or_group_p1", "statement_anywhere",
  "@11", "wildcard_name", "wildcard_spec", "exclude_name_list",
  "file_NAME_list", "input_section_spec_no_keep", "input_section_spec",
  "@12", "statement", "@13", "@14", "statement_list", "statement_list_opt",
  "length", "fill_exp", "fill_opt", "assign_op", "end", "assignment",
  "opt_comma", "memory", "memory_spec_list_opt", "memory_spec_list",
  "memory_spec", "@15", "@16", "origin_spec", "length_spec",
  "attributes_opt", "attributes_list", "attributes_string", "startup",
  "high_level_library", "high_level_library_NAME_list",
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
     291,   292,   293,   294,   295,   296,   123,   125,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,    44,    59,    41,    91,    93,    33,   126
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   148,   149,   149,   149,   149,   149,   150,   152,   151,
     154,   153,   155,   155,   156,   156,   156,   156,   156,   156,
     156,   156,   156,   156,   156,   156,   156,   156,   156,   156,
     156,   156,   156,   156,   156,   156,   156,   156,   156,   156,
     157,   156,   156,   156,   158,   158,   158,   159,   159,   160,
     160,   161,   161,   161,   163,   162,   164,   164,   164,   166,
     165,   167,   167,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   169,   168,   168,   170,   168,   168,   168,   168,
     168,   168,   168,   171,   171,   171,   171,   171,   171,   172,
     171,   173,   171,   174,   171,   175,   176,   176,   176,   177,
     177,   178,   177,   179,   179,   179,   180,   180,   180,   180,
     180,   180,   180,   180,   180,   181,   181,   182,   182,   183,
     183,   183,   184,   185,   184,   186,   186,   186,   186,   186,
     186,   186,   186,   187,   186,   188,   186,   189,   189,   190,
     190,   191,   191,   191,   191,   191,   192,   193,   193,   194,
     194,   194,   194,   194,   194,   194,   194,   195,   195,   196,
     196,   196,   196,   197,   197,   198,   199,   199,   200,   200,
     202,   201,   203,   201,   204,   205,   206,   206,   207,   207,
     208,   208,   209,   210,   210,   211,   211,   212,   213,   213,
     214,   214,   215,   215,   215,   217,   216,   218,   218,   218,
     218,   218,   218,   218,   218,   218,   218,   218,   218,   218,
     218,   218,   218,   218,   218,   218,   218,   218,   218,   218,
     218,   218,   218,   218,   218,   218,   218,   218,   218,   218,
     218,   218,   218,   218,   218,   218,   218,   218,   218,   218,
     218,   218,   218,   218,   219,   219,   220,   220,   221,   221,
     222,   222,   223,   223,   223,   223,   225,   226,   227,   228,
     229,   224,   230,   231,   232,   233,   234,   224,   235,   236,
     224,   237,   224,   238,   238,   238,   238,   238,   239,   239,
     239,   240,   240,   240,   240,   241,   241,   242,   242,   243,
     243,   244,   244,   245,   246,   247,   248,   245,   249,   250,
     250,   252,   253,   251,   254,   255,   255,   255,   256,   256,
     258,   257,   259,   259,   260,   261,   263,   262,   265,   264,
     266,   266,   267,   267,   267,   268,   268,   269,   269,   269,
     269,   269,   270,   270,   270,   270,   271,   270,   272,   270,
     270,   270,   270,   270,   270,   270,   273,   273
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
       7,     7,     7,     7,     8,     2,     1,     3,     1,     1,
       3,     4,     1,     0,     5,     2,     1,     1,     1,     4,
       1,     4,     4,     0,     8,     0,     5,     2,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     6,     6,     1,     0,     4,     1,     0,     3,     1,
       0,     7,     0,     5,     3,     3,     0,     3,     1,     2,
       1,     2,     4,     4,     3,     3,     1,     4,     3,     0,
       1,     1,     0,     2,     3,     0,     2,     2,     3,     4,
       2,     2,     2,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     5,
       3,     3,     4,     1,     1,     4,     4,     4,     4,     4,
       4,     4,     6,     6,     6,     4,     6,     4,     1,     6,
       6,     6,     4,     4,     3,     0,     4,     0,     4,     0,
       4,     0,     1,     1,     1,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,    18,     0,     0,
       7,     0,     5,     1,     1,     1,     1,     1,     3,     0,
       2,     3,     2,     6,    10,     2,     1,     0,     1,     2,
       0,     0,     3,     0,     0,     0,     0,    11,     4,     0,
       2,     0,     0,     6,     1,     0,     3,     5,     0,     3,
       0,     2,     1,     2,     4,     2,     0,     2,     0,     5,
       1,     2,     4,     5,     6,     1,     2,     0,     2,     4,
       4,     8,     1,     1,     3,     3,     0,     9,     0,     7,
       1,     3,     1,     3,     1,     3,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    59,    10,     8,   326,   320,     0,     2,    62,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    60,    11,
       0,   337,     0,   327,   330,     0,   321,   322,     0,     0,
       0,     0,     0,    79,     0,    80,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   200,   201,     0,
       0,    82,     0,     0,     0,   111,     0,    72,    61,    64,
      70,     0,    63,    66,    67,    68,    69,    65,    71,     0,
      16,     0,     0,     0,     0,    17,     0,     0,     0,    19,
      46,     0,     0,     0,     0,     0,     0,    51,    54,     0,
       0,     0,   343,   354,   342,   350,   352,     0,     0,   337,
     331,   350,   352,     0,     0,   323,   205,   166,   165,   164,
     163,   162,   161,   160,   159,   205,   108,   309,     0,     0,
       0,     0,     7,    85,   177,     0,     0,     0,     0,     0,
       0,     0,     0,   199,   202,     0,     0,     0,     0,     0,
       0,    54,   168,   167,   110,     0,     0,    40,     0,   233,
     248,     0,     0,     0,     0,     0,     0,     0,     0,   234,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    14,     0,    49,    31,
      47,    32,    18,    33,    23,     0,    36,     0,    37,    52,
      38,    39,     0,    42,    12,     9,     0,     0,     0,     0,
     338,     0,     0,   325,   169,     0,   170,     0,     0,    89,
      90,     0,     0,    62,   180,     0,     0,   174,   179,     0,
       0,     0,     0,     0,     0,     0,   194,   196,   174,   174,
     202,     0,    93,    96,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    13,     0,     0,   211,   207,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     210,   212,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    25,     0,     0,    45,     0,     0,
       0,    22,     0,     0,    56,    55,   348,     0,     0,   332,
     345,   355,   344,   351,   353,     0,   324,   206,   266,   105,
       0,   272,   278,   107,   106,   311,   308,   310,     0,    76,
      78,   328,   186,   182,   175,   173,     0,     0,    92,    73,
      74,    84,   109,   192,   193,     0,   197,     0,   202,   203,
      87,    99,    95,    98,     0,     0,    81,     0,    75,   205,
     205,     0,    88,     0,    27,    28,    43,    29,    30,   208,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     231,   230,   228,   227,   226,   221,   220,   224,   225,   223,
     222,   219,   218,   216,   217,   213,   214,   215,    15,    26,
      24,    50,    48,    44,    20,    21,    35,    34,    53,    57,
       0,     0,   339,   340,     0,   335,   333,     0,   289,   281,
       0,   289,     0,     0,    86,     0,     0,   177,   178,     0,
     195,   198,   204,     0,   103,    94,    97,     0,    83,     0,
       0,     0,   329,    41,     0,   241,   247,     0,     0,   245,
       0,   232,   209,   236,   235,   237,   238,     0,     0,   252,
     253,   240,     0,   239,     0,    58,   356,   353,   346,   336,
     334,     0,     0,   289,     0,   257,   108,   296,     0,   297,
     279,   314,   315,     0,   190,     0,     0,   188,     0,     0,
      91,     0,     0,   101,   171,   172,     0,     0,     0,     0,
       0,     0,     0,     0,   229,   357,     0,     0,     0,   283,
     284,   285,   286,   287,   290,     0,     0,     0,     0,   292,
       0,   259,     0,   295,   298,   257,     0,   318,     0,   312,
       0,   191,   187,   189,     0,   174,   183,   100,     0,     0,
     112,   242,   243,   244,   246,   249,   250,   251,   349,     0,
     356,   288,     0,   291,     0,     0,   261,   282,   261,   108,
       0,   315,     0,     0,    77,   205,     0,   104,     0,   341,
       0,   289,     0,     0,     0,   267,   273,     0,     0,   316,
       0,   313,   184,     0,   181,   102,   347,     0,     0,   256,
       0,     0,   265,     0,   280,   319,   315,   205,     0,   293,
     258,     0,   262,   263,   264,     0,   274,   317,   185,     0,
     260,   268,   303,   289,   149,     0,     0,   129,   115,   114,
     151,   152,   153,   154,   155,     0,     0,     0,     0,   136,
     138,   143,     0,     0,   137,     0,   116,     0,   132,   140,
     148,   150,     0,     0,     0,   304,   275,   294,     0,     0,
     145,   205,     0,   133,     0,   113,     0,   128,   174,     0,
     147,   269,   205,   135,     0,   300,     0,     0,     0,     0,
       0,     0,     0,     0,   149,     0,   156,     0,     0,   126,
       0,     0,   130,     0,   174,   300,     0,   149,     0,   255,
       0,     0,   139,     0,   118,     0,     0,   119,     0,   142,
       0,   113,     0,     0,   125,   127,   131,   255,   141,     0,
     299,     0,   301,     0,     0,     0,     0,     0,   146,     0,
     134,   117,   301,   305,     0,   158,     0,     0,     0,     0,
       0,     0,   158,   301,   254,   205,     0,   276,   121,   120,
       0,   122,   123,     0,   270,   158,   157,   302,   174,   124,
     144,   174,   306,   277,   271,   174,   307
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,   123,    11,    12,     9,    10,    19,    90,   245,
     182,   181,   179,   190,   191,   192,   305,     7,     8,    18,
      58,   136,   213,   235,   443,   549,   502,    59,   207,   323,
     140,   646,   647,   690,   668,   648,   649,   688,   650,   662,
     684,   651,   652,   653,   685,   747,   115,   144,    61,   693,
      62,   216,   217,   218,   332,   437,   545,   594,   436,   496,
     497,    63,    64,   228,    65,   229,    66,   231,   686,   205,
     250,   722,   531,   566,   585,   615,   324,   428,   602,   624,
     695,   761,   430,   603,   622,   675,   758,   431,   536,   486,
     525,   484,   485,   489,   535,   699,   735,   625,   674,   743,
     765,    67,   208,   327,   432,   573,   492,   539,   571,    15,
      16,    26,    27,   103,    13,    14,    68,    69,    23,    24,
     427,    97,    98,   518,   421,   516
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -658
static const yytype_int16 yypact[] =
{
     130,  -658,  -658,  -658,  -658,  -658,    42,  -658,  -658,  -658,
    -658,  -658,    61,  -658,   -23,  -658,    12,  -658,   886,  1644,
      82,    66,    38,   -23,  -658,    98,    12,  -658,   529,    54,
      67,    91,    89,  -658,   119,  -658,   217,   174,   207,   214,
     220,   223,   227,   257,   258,   266,   267,  -658,  -658,   268,
     293,  -658,   295,   296,   297,  -658,   298,  -658,  -658,  -658,
    -658,     9,  -658,  -658,  -658,  -658,  -658,  -658,  -658,   151,
    -658,   316,   217,   333,   716,  -658,   334,   335,   336,  -658,
    -658,   337,   339,   341,   716,   343,   299,   344,  -658,   345,
     240,   716,  -658,   348,  -658,   346,   349,   302,   213,    66,
    -658,  -658,  -658,   303,   221,  -658,  -658,  -658,  -658,  -658,
    -658,  -658,  -658,  -658,  -658,  -658,  -658,  -658,   360,   365,
     366,   368,  -658,  -658,    31,   369,   372,   373,   217,   217,
     378,   217,    28,  -658,   383,   129,   351,   217,   385,   386,
     358,  -658,  -658,  -658,  -658,   340,    47,  -658,    50,  -658,
    -658,   716,   716,   716,   361,   362,   364,   370,   371,  -658,
     379,   380,   381,   384,   388,   392,   393,   394,   395,   399,
     400,   402,   403,   404,   716,   716,  1435,   375,  -658,   256,
    -658,   269,    23,  -658,  -658,   525,  1843,   274,  -658,  -658,
     279,  -658,   418,  -658,  -658,  1843,   367,    98,    98,   300,
     110,   387,   304,   110,  -658,   716,  -658,   289,    55,  -658,
    -658,    -5,   292,  -658,  -658,   217,   390,    -2,  -658,   310,
     317,   320,   321,   322,   324,   326,  -658,  -658,    19,    94,
      27,   329,  -658,  -658,   408,    16,   129,   330,   468,   470,
     716,   338,   -23,   716,   716,  -658,   716,   716,  -658,  -658,
     895,   716,   716,   716,   716,   716,   473,   474,   716,   475,
     476,   478,   480,   716,   716,   481,   482,   716,   716,   483,
    -658,  -658,   716,   716,   716,   716,   716,   716,   716,   716,
     716,   716,   716,   716,   716,   716,   716,   716,   716,   716,
     716,   716,   716,   716,  1843,   484,   487,  -658,   488,   716,
     716,  1843,   265,   489,  -658,    46,  -658,   353,   355,  -658,
    -658,   495,  -658,  -658,  -658,   -56,  -658,  1843,   529,  -658,
     217,  -658,  -658,  -658,  -658,  -658,  -658,  -658,   498,  -658,
    -658,   955,   467,  -658,  -658,  -658,    31,   503,  -658,  -658,
    -658,  -658,  -658,  -658,  -658,   217,  -658,   217,   383,  -658,
    -658,  -658,  -658,  -658,   471,   138,  -658,    20,  -658,  -658,
    -658,  1462,  -658,    25,  1843,  1843,  1667,  1843,  1843,  -658,
     849,  1081,  1482,  1502,  1101,   374,   376,  1121,   377,   382,
     389,   405,  1522,  1560,   406,   407,  1141,  1587,   409,  1803,
    1860,  1540,  1875,  1061,  1888,  1046,  1046,   294,   294,   294,
     294,   283,   283,   239,   239,  -658,  -658,  -658,  1843,  1843,
    1843,  -658,  -658,  -658,  1843,  1843,  -658,  -658,  -658,  -658,
     506,    98,   117,   110,   455,  -658,  -658,   -55,   597,  -658,
     679,   597,   716,   412,  -658,     3,   497,    31,  -658,   411,
    -658,  -658,  -658,   129,  -658,  -658,  -658,   486,  -658,   414,
     415,   513,  -658,  -658,   716,  -658,  -658,   716,   716,  -658,
     716,  -658,  -658,  -658,  -658,  -658,  -658,   716,   716,  -658,
    -658,  -658,   514,  -658,   716,  -658,   391,   508,  -658,  -658,
    -658,   228,   493,  1780,   510,   445,  -658,  -658,  1823,   456,
    -658,  1843,    18,   555,  -658,   556,     2,  -658,   479,   531,
    -658,    24,   129,  -658,  -658,  -658,   422,  1161,  1181,  1208,
    1228,  1248,  1268,   426,  1843,   110,   504,    98,    98,  -658,
    -658,  -658,  -658,  -658,  -658,   428,   716,   -24,   557,  -658,
     535,   536,   398,  -658,  -658,   445,   519,   542,   543,  -658,
     438,  -658,  -658,  -658,   576,   443,  -658,  -658,    33,   129,
    -658,  -658,  -658,  -658,  -658,  -658,  -658,  -658,  -658,   444,
     391,  -658,  1288,  -658,   716,   548,   501,  -658,   501,  -658,
     716,    18,   716,   457,  -658,  -658,   494,  -658,    79,   110,
     553,   238,  1308,   716,   574,  -658,  -658,   347,  1335,  -658,
    1355,  -658,  -658,   606,  -658,  -658,  -658,   577,   599,  -658,
    1375,   716,   152,   561,  -658,  -658,    18,  -658,   716,  -658,
    -658,  1395,  -658,  -658,  -658,   563,  -658,  -658,  -658,  1415,
    -658,  -658,  -658,   584,   797,    60,   607,   583,  -658,  -658,
    -658,  -658,  -658,  -658,  -658,   585,   587,   217,   588,  -658,
    -658,  -658,   589,   592,  -658,   100,  -658,   593,  -658,  -658,
    -658,   797,   575,   594,     9,  -658,  -658,  -658,    37,    75,
    -658,  -658,   601,  -658,   246,  -658,   603,  -658,   -52,   100,
    -658,  -658,  -658,  -658,   586,   616,   608,   609,   500,   610,
     509,   614,   617,   515,   797,   516,  -658,   716,     8,  -658,
       4,   230,  -658,   100,   149,   616,   518,   797,   640,   562,
     246,   246,  -658,   246,  -658,   246,   246,  -658,   620,  -658,
    1607,   520,   522,   246,  -658,  -658,  -658,   562,  -658,   596,
    -658,   632,  -658,   524,   526,    39,   530,   541,  -658,   664,
    -658,  -658,  -658,  -658,   670,    90,   544,   546,   246,   549,
     550,   551,    90,  -658,  -658,  -658,   681,  -658,  -658,  -658,
     554,  -658,  -658,     9,  -658,    90,  -658,  -658,   443,  -658,
    -658,   443,  -658,  -658,  -658,   443,  -658
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -658,  -658,   -71,  -658,  -658,  -658,  -658,   441,  -658,  -658,
    -658,  -658,  -658,  -658,   558,  -658,  -658,  -658,  -658,   485,
    -658,  -658,  -658,  -220,  -658,  -658,  -658,  -658,  -456,   -13,
    -658,   986,  -594,    -7,    22,    13,  -658,  -658,    49,  -658,
    -658,  -658,  -617,  -658,   -43,  -657,  -658,  -628,  -575,  -214,
    -658,   270,  -658,   397,  -658,  -658,  -658,  -658,  -658,  -658,
     208,  -658,  -658,  -658,  -658,  -658,  -658,  -221,  -104,  -658,
     -74,   -14,   171,  -658,   140,  -658,  -658,  -658,  -658,  -658,
    -658,  -658,  -658,  -658,  -658,  -658,  -658,  -658,  -658,  -658,
    -658,  -465,   281,  -658,  -658,    26,  -635,  -658,  -658,  -658,
    -658,  -658,  -658,  -658,  -658,  -658,  -658,  -532,  -658,  -658,
    -658,  -658,   687,  -658,  -658,  -658,  -658,  -658,   472,   -19,
    -658,   623,   -12,  -658,  -658,   155
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -329
static const yytype_int16 yytable[] =
{
     176,   147,   204,   336,   100,    60,   494,   494,   665,   349,
     186,   206,   711,   104,   345,   347,   357,   195,   528,   628,
     352,   353,   537,   628,   352,   353,   673,   297,   352,   353,
     532,   230,   122,    21,  -176,   214,   629,   352,   353,   589,
     629,   665,    17,   665,   519,   520,   521,   522,   523,   654,
     419,   667,   628,   243,   628,  -176,   246,   222,   223,   325,
     225,   227,   666,   636,   655,    20,   237,   708,    25,   629,
      92,   629,   425,   479,   617,   667,   654,   248,   249,   665,
     719,    21,   452,   352,   353,   754,   426,   480,    91,   335,
     628,   676,   677,   692,    99,   215,   745,   742,   762,   715,
     270,   271,    92,   294,   665,    22,   746,   629,   755,   654,
     116,   301,   326,   587,   310,   628,   598,   656,   538,   524,
     354,   310,   654,   117,   354,   760,   120,   442,   354,   681,
     682,   317,   629,   232,   233,   678,   328,   354,   329,   335,
     118,   119,   445,   446,   333,   542,   643,   713,   495,   495,
     142,   143,   645,    22,   666,   636,   121,   355,   626,   356,
     335,   355,   344,   448,   298,   355,   361,   547,   348,   364,
     365,   226,   367,   368,   355,   679,   577,   370,   371,   372,
     373,   374,   738,   354,   377,   307,   308,   420,   244,   382,
     383,   247,    93,   386,   387,    94,    95,    96,   389,   390,
     391,   392,   393,   394,   395,   396,   397,   398,   399,   400,
     401,   402,   403,   404,   405,   406,   407,   408,   409,   410,
     355,   122,   595,   501,    93,   414,   415,    94,   101,   102,
     124,   149,   150,   234,   665,   335,   311,   346,   643,   312,
     313,   314,   447,   311,   125,   628,   312,   313,   477,   429,
     665,   126,     1,     2,     3,   449,   450,   127,   151,   152,
     128,   628,   629,     4,   129,   153,   154,   155,   416,   417,
       5,   288,   289,   290,   440,   527,   441,   597,   629,   156,
     157,   158,   548,   145,   676,   677,   159,   612,   613,   614,
     335,   160,   716,   318,   130,   131,   519,   520,   521,   522,
     523,   161,   188,   132,   133,   134,   162,   163,   164,   165,
     166,   167,   168,   286,   287,   288,   289,   290,    60,   169,
     146,   170,   284,   285,   286,   287,   288,   289,   290,   578,
     135,   576,   137,   138,   139,   141,   171,   148,   177,   178,
     180,   183,   172,   184,   100,   185,   319,   187,   189,   193,
     194,   318,   196,   320,   483,   200,   488,   483,   491,   199,
     202,   321,   197,   203,   209,   198,    43,   173,   679,   210,
     211,   524,   212,   219,   174,   175,   220,   221,   149,   150,
     507,   292,   224,   508,   509,   322,   510,   230,   236,   238,
     239,    53,    54,   511,   512,   240,   242,   295,   251,   252,
     514,   253,   318,    55,   604,   151,   152,   254,   255,   476,
     296,   320,   153,   154,   155,   302,   256,   257,   258,   321,
     303,   259,   304,   306,    43,   260,   156,   157,   158,   261,
     262,   263,   264,   159,   567,   330,   265,   266,   160,   267,
     268,   269,   309,   322,   315,   351,   316,   334,   161,    53,
      54,   337,   562,   162,   163,   164,   165,   166,   167,   168,
     338,    55,   320,   339,   340,   341,   169,   342,   170,   343,
     321,   592,   350,   358,   359,    43,   360,   375,   376,   378,
     379,   362,   380,   171,   381,   384,   385,   388,   411,   172,
     582,   412,   413,   418,   322,   422,   588,   423,   590,   424,
      53,    54,   433,   618,   435,   559,   560,   439,   444,   600,
     475,   478,    55,   498,   173,   460,   293,   506,   513,   461,
     463,   174,   175,   503,   517,   464,   529,   611,   149,   150,
     526,   299,   465,   515,   619,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   763,   530,   534,   764,   466,   469,
     470,   766,   473,   493,   500,   151,   152,   504,   505,   540,
     541,   558,   153,   154,   155,   550,   660,   546,   696,   557,
     544,   561,   564,   563,   565,   569,   156,   157,   158,   570,
     572,   574,   575,   159,   335,   583,   579,   593,   160,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   161,   591,
     149,   150,   584,   162,   163,   164,   165,   166,   167,   168,
     596,   601,   607,   710,   608,   609,   169,   616,   170,   621,
    -113,   527,   658,   657,   659,   661,   663,   151,   152,   664,
     669,   672,   671,   171,   481,   154,   155,   482,   687,   172,
     691,   698,   697,   702,   720,   700,   701,   703,   156,   157,
     158,   705,   704,   733,   706,   159,   728,   734,   707,   709,
     160,   718,   721,  -129,   173,   730,   300,   736,   741,   737,
     161,   174,   175,   739,   744,   162,   163,   164,   165,   166,
     167,   168,   149,   150,   740,   757,   366,   748,   169,   749,
     170,   694,   751,   752,   753,   487,   725,   759,   331,   241,
     670,   712,   756,   732,   543,   171,   568,   499,   586,   151,
     152,   172,   490,   105,   363,   580,   153,   154,   155,   149,
     150,   717,   201,     0,     0,     0,     0,     0,     0,     0,
     156,   157,   158,   438,     0,     0,   173,   159,     0,     0,
       0,     0,   160,   174,   175,     0,   151,   152,     0,     0,
       0,     0,   161,   153,   154,   155,     0,   162,   163,   164,
     165,   166,   167,   168,     0,     0,     0,   156,   157,   158,
     169,     0,   170,     0,   159,     0,     0,     0,     0,   160,
       0,     0,     0,     0,     0,     0,     0,   171,     0,   161,
       0,     0,     0,   172,   162,   163,   164,   165,   166,   167,
     168,   627,     0,     0,     0,     0,     0,   169,     0,   170,
       0,     0,   628,     0,     0,     0,     0,     0,   173,     0,
       0,     0,     0,     0,   171,   174,   175,     0,     0,   629,
     172,     0,     0,     0,     0,     0,     0,     0,   630,   631,
     632,   633,   634,     0,     0,     0,     0,     0,     0,     0,
       0,   635,   636,     0,     0,   173,     0,     0,     0,     0,
       0,   637,   174,   175,   272,     0,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,     0,     0,     0,     0,     0,   638,
      28,   639,     0,     0,     0,   640,     0,     0,     0,    53,
      54,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     272,   641,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
       0,   642,    29,    30,    31,   643,     0,     0,     0,   644,
       0,   645,     0,     0,     0,    32,    33,    34,    35,     0,
      36,    37,    38,    39,     0,     0,     0,     0,     0,    28,
      40,    41,    42,    43,     0,     0,     0,     0,     0,     0,
       0,    44,    45,    46,    47,    48,    49,     0,     0,     0,
       0,    50,    51,    52,     0,     0,     0,     0,    53,    54,
     454,   434,   455,     0,     0,     0,     0,     0,     0,     0,
      55,    29,    30,    31,     0,     0,     0,     0,     0,     0,
       0,     0,    56,     0,    32,    33,    34,    35,  -328,    36,
      37,    38,    39,     0,     0,     0,     0,     0,    57,    40,
      41,    42,    43,     0,     0,     0,     0,     0,   369,     0,
      44,    45,    46,    47,    48,    49,     0,     0,     0,     0,
      50,    51,    52,     0,     0,     0,     0,    53,    54,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    55,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,    56,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   272,    57,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   272,     0,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   272,     0,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   272,     0,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   272,     0,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   272,     0,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,     0,     0,     0,     0,
       0,     0,     0,   272,   456,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,   459,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,   462,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,   471,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,   551,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,   552,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,     0,     0,     0,     0,     0,     0,     0,
     272,   553,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   554,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   555,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   556,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   581,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   599,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
       0,     0,     0,     0,     0,     0,     0,   272,   605,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   272,   606,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   272,   610,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   272,   620,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,     0,   623,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   272,   291,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,     0,     0,     0,     0,     0,
       0,     0,   272,   451,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   272,   457,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,     0,   458,   680,   683,     0,     0,    70,     0,
     689,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   467,     0,     0,     0,     0,     0,     0,
       0,    70,     0,     0,     0,     0,   714,   680,     0,     0,
       0,     0,    71,     0,     0,     0,   723,   724,     0,   689,
       0,   726,   727,     0,     0,     0,     0,     0,     0,   731,
       0,   468,     0,   453,     0,    71,     0,     0,    72,     0,
       0,   714,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   750,     0,     0,     0,   472,     0,
       0,    72,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    73,     0,     0,     0,     0,   729,    74,
      75,    76,    77,    78,   -43,    79,    80,    81,     0,    82,
      83,     0,    84,    85,    86,     0,    73,     0,     0,    87,
      88,    89,    74,    75,    76,    77,    78,     0,    79,    80,
      81,     0,    82,    83,     0,    84,    85,    86,     0,     0,
       0,     0,    87,    88,    89,   272,     0,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,     0,     0,   527,   272,   474,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   272,   533,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   272,     0,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290
};

static const yytype_int16 yycheck[] =
{
      74,    72,   106,   217,    23,    18,     4,     4,     4,   230,
      84,   115,     4,    25,   228,   229,   236,    91,   483,    15,
       4,     5,     4,    15,     4,     5,   654,     4,     4,     5,
     486,     4,     4,    56,    36,     4,    32,     4,     5,   571,
      32,     4,     0,     4,    68,    69,    70,    71,    72,   624,
       4,   645,    15,     6,    15,    57,     6,   128,   129,     4,
     131,   132,    54,    55,     4,     4,   137,   684,    56,    32,
       4,    32,   128,   128,   606,   669,   651,   151,   152,     4,
     697,    56,    57,     4,     5,   742,   142,   142,     6,   141,
      15,    54,    55,   145,    56,    64,     6,   732,   755,   693,
     174,   175,     4,   177,     4,   128,    16,    32,   743,   684,
      56,   185,    57,   569,     4,    15,   581,    57,   100,   143,
     104,     4,   697,    56,   104,   753,    37,   348,   104,    54,
      55,   205,    32,     4,     5,    98,   141,   104,   143,   141,
      49,    50,     4,     5,   215,   143,   138,   143,   146,   146,
     141,   142,   144,   128,    54,    55,    37,   141,   623,   143,
     141,   141,   143,   143,   141,   141,   240,   143,   141,   243,
     244,   143,   246,   247,   141,   138,   143,   251,   252,   253,
     254,   255,   143,   104,   258,   197,   198,   141,   141,   263,
     264,   141,   126,   267,   268,   129,   130,   131,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     141,     4,   143,   443,   126,   299,   300,   129,   130,   131,
      56,     3,     4,   104,     4,   141,   126,   143,   138,   129,
     130,   131,   104,   126,    37,    15,   129,   130,   131,   320,
       4,    37,   122,   123,   124,   359,   360,    37,    30,    31,
      37,    15,    32,   133,    37,    37,    38,    39,     3,     4,
     140,    32,    33,    34,   345,    37,   347,    39,    32,    51,
      52,    53,   502,   132,    54,    55,    58,   135,   136,   137,
     141,    63,   143,     4,    37,    37,    68,    69,    70,    71,
      72,    73,     3,    37,    37,    37,    78,    79,    80,    81,
      82,    83,    84,    30,    31,    32,    33,    34,   331,    91,
       4,    93,    28,    29,    30,    31,    32,    33,    34,   549,
      37,   545,    37,    37,    37,    37,   108,     4,     4,     4,
       4,     4,   114,     4,   363,     4,    57,     4,     4,     4,
     110,     4,     4,    64,   428,   142,   430,   431,   432,    57,
      57,    72,    16,   142,     4,    16,    77,   139,   138,     4,
       4,   143,     4,     4,   146,   147,     4,     4,     3,     4,
     454,     6,     4,   457,   458,    96,   460,     4,    37,     4,
       4,   102,   103,   467,   468,    37,    56,   141,    37,    37,
     474,    37,     4,   114,    57,    30,    31,    37,    37,   421,
     141,    64,    37,    38,    39,   141,    37,    37,    37,    72,
     141,    37,     4,    56,    77,    37,    51,    52,    53,    37,
      37,    37,    37,    58,    36,   143,    37,    37,    63,    37,
      37,    37,   142,    96,    57,    37,   142,    57,    73,   102,
     103,   141,   526,    78,    79,    80,    81,    82,    83,    84,
     143,   114,    64,   143,   143,   143,    91,   143,    93,   143,
      72,   575,   143,   143,     6,    77,     6,     4,     4,     4,
       4,   143,     4,   108,     4,     4,     4,     4,     4,   114,
     564,     4,     4,     4,    96,   142,   570,   142,   572,     4,
     102,   103,     4,   607,    37,   517,   518,     4,    37,   583,
       4,    56,   114,    16,   139,   141,   141,     4,     4,   143,
     143,   146,   147,    37,    16,   143,    16,   601,     3,     4,
      37,     6,   143,   142,   608,     6,     7,     8,     9,    10,
      11,    12,    13,    14,   758,   100,    90,   761,   143,   143,
     143,   765,   143,   141,   143,    30,    31,   143,   143,     4,
       4,    57,    37,    38,    39,   143,   637,    36,   672,   143,
      91,   143,    37,    16,    38,    56,    51,    52,    53,    37,
      37,   143,     6,    58,   141,    37,   142,    93,    63,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    73,   142,
       3,     4,   101,    78,    79,    80,    81,    82,    83,    84,
      57,    37,     6,   687,    37,    16,    91,    56,    93,    56,
      37,    37,    37,    16,    37,    37,    37,    30,    31,    37,
      37,    37,    57,   108,    37,    38,    39,    40,    37,   114,
      37,    25,    56,   143,     4,    37,    37,    37,    51,    52,
      53,    37,   143,    57,    37,    58,    36,    25,   143,   143,
      63,   143,   100,   143,   139,   143,   141,   143,     4,   143,
      73,   146,   147,   143,     4,    78,    79,    80,    81,    82,
      83,    84,     3,     4,   143,     4,   245,   143,    91,   143,
      93,   669,   143,   143,   143,    16,   703,   143,   213,   141,
     651,   688,   745,   717,   496,   108,   535,   437,   568,    30,
      31,   114,   431,    26,   242,   560,    37,    38,    39,     3,
       4,   695,    99,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      51,    52,    53,   336,    -1,    -1,   139,    58,    -1,    -1,
      -1,    -1,    63,   146,   147,    -1,    30,    31,    -1,    -1,
      -1,    -1,    73,    37,    38,    39,    -1,    78,    79,    80,
      81,    82,    83,    84,    -1,    -1,    -1,    51,    52,    53,
      91,    -1,    93,    -1,    58,    -1,    -1,    -1,    -1,    63,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,    -1,    73,
      -1,    -1,    -1,   114,    78,    79,    80,    81,    82,    83,
      84,     4,    -1,    -1,    -1,    -1,    -1,    91,    -1,    93,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,   108,   146,   147,    -1,    -1,    32,
     114,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    54,    55,    -1,    -1,   139,    -1,    -1,    -1,    -1,
      -1,    64,   146,   147,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    92,
       4,    94,    -1,    -1,    -1,    98,    -1,    -1,    -1,   102,
     103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      15,   114,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,   134,    46,    47,    48,   138,    -1,    -1,    -1,   142,
      -1,   144,    -1,    -1,    -1,    59,    60,    61,    62,    -1,
      64,    65,    66,    67,    -1,    -1,    -1,    -1,    -1,     4,
      74,    75,    76,    77,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    85,    86,    87,    88,    89,    90,    -1,    -1,    -1,
      -1,    95,    96,    97,    -1,    -1,    -1,    -1,   102,   103,
     141,    36,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     114,    46,    47,    48,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   126,    -1,    59,    60,    61,    62,   132,    64,
      65,    66,    67,    -1,    -1,    -1,    -1,    -1,   142,    74,
      75,    76,    77,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      85,    86,    87,    88,    89,    90,    -1,    -1,    -1,    -1,
      95,    96,    97,    -1,    -1,    -1,    -1,   102,   103,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,   126,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    15,   142,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    15,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    15,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    15,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    15,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    15,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    15,   143,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   143,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   143,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   143,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   143,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   143,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,   143,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    15,   141,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   141,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,   141,   658,   659,    -1,    -1,     4,    -1,
     664,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     4,    -1,    -1,    -1,    -1,   690,   691,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    -1,   700,   701,    -1,   703,
      -1,   705,   706,    -1,    -1,    -1,    -1,    -1,    -1,   713,
      -1,   141,    -1,    36,    -1,    38,    -1,    -1,    64,    -1,
      -1,   725,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   738,    -1,    -1,    -1,   141,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    99,    -1,    -1,    -1,    -1,   141,   105,
     106,   107,   108,   109,   110,   111,   112,   113,    -1,   115,
     116,    -1,   118,   119,   120,    -1,    99,    -1,    -1,   125,
     126,   127,   105,   106,   107,   108,   109,    -1,   111,   112,
     113,    -1,   115,   116,    -1,   118,   119,   120,    -1,    -1,
      -1,    -1,   125,   126,   127,    15,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    37,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   122,   123,   124,   133,   140,   149,   165,   166,   153,
     154,   151,   152,   262,   263,   257,   258,     0,   167,   155,
       4,    56,   128,   266,   267,    56,   259,   260,     4,    46,
      47,    48,    59,    60,    61,    62,    64,    65,    66,    67,
      74,    75,    76,    77,    85,    86,    87,    88,    89,    90,
      95,    96,    97,   102,   103,   114,   126,   142,   168,   175,
     177,   196,   198,   209,   210,   212,   214,   249,   264,   265,
       4,    38,    64,    99,   105,   106,   107,   108,   109,   111,
     112,   113,   115,   116,   118,   119,   120,   125,   126,   127,
     156,     6,     4,   126,   129,   130,   131,   269,   270,    56,
     267,   130,   131,   261,   270,   260,     6,     7,     8,     9,
      10,    11,    12,    13,    14,   194,    56,    56,    49,    50,
      37,    37,     4,   150,    56,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,   169,    37,    37,    37,
     178,    37,   141,   142,   195,   132,     4,   150,     4,     3,
       4,    30,    31,    37,    38,    39,    51,    52,    53,    58,
      63,    73,    78,    79,    80,    81,    82,    83,    84,    91,
      93,   108,   114,   139,   146,   147,   218,     4,     4,   160,
       4,   159,   158,     4,     4,     4,   218,     4,     3,     4,
     161,   162,   163,     4,   110,   218,     4,    16,    16,    57,
     142,   269,    57,   142,   216,   217,   216,   176,   250,     4,
       4,     4,     4,   170,     4,    64,   199,   200,   201,     4,
       4,     4,   150,   150,     4,   150,   143,   150,   211,   213,
       4,   215,     4,     5,   104,   171,    37,   150,     4,     4,
      37,   162,    56,     6,   141,   157,     6,   141,   218,   218,
     218,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
     218,   218,    15,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,   141,     6,   141,   218,   141,   141,     4,   141,     6,
     141,   218,   141,   141,     4,   164,    56,   270,   270,   142,
       4,   126,   129,   130,   131,    57,   142,   218,     4,    57,
      64,    72,    96,   177,   224,     4,    57,   251,   141,   143,
     143,   167,   202,   150,    57,   141,   197,   141,   143,   143,
     143,   143,   143,   143,   143,   197,   143,   197,   141,   215,
     143,    37,     4,     5,   104,   141,   143,   171,   143,     6,
       6,   218,   143,   266,   218,   218,   155,   218,   218,   143,
     218,   218,   218,   218,   218,     4,     4,   218,     4,     4,
       4,     4,   218,   218,     4,     4,   218,   218,     4,   218,
     218,   218,   218,   218,   218,   218,   218,   218,   218,   218,
     218,   218,   218,   218,   218,   218,   218,   218,   218,   218,
     218,     4,     4,     4,   218,   218,     3,     4,     4,     4,
     141,   272,   142,   142,     4,   128,   142,   268,   225,   150,
     230,   235,   252,     4,    36,    37,   206,   203,   201,     4,
     150,   150,   215,   172,    37,     4,     5,   104,   143,   216,
     216,   141,    57,    36,   141,   143,   143,   141,   141,   143,
     141,   143,   143,   143,   143,   143,   143,   141,   141,   143,
     143,   143,   141,   143,    16,     4,   270,   131,    56,   128,
     142,    37,    40,   218,   239,   240,   237,    16,   218,   241,
     240,   218,   254,   141,     4,   146,   207,   208,    16,   199,
     143,   171,   174,    37,   143,   143,     4,   218,   218,   218,
     218,   218,   218,     4,   218,   142,   273,    16,   271,    68,
      69,    70,    71,    72,   143,   238,    37,    37,   239,    16,
     100,   220,   176,    16,    90,   242,   236,     4,   100,   255,
       4,     4,   143,   208,    91,   204,    36,   143,   171,   173,
     143,   143,   143,   143,   143,   143,   143,   143,    57,   270,
     270,   143,   218,    16,    37,    38,   221,    36,   220,    56,
      37,   256,    37,   253,   143,     6,   197,   143,   171,   142,
     273,   143,   218,    37,   101,   222,   222,   176,   218,   255,
     218,   142,   216,    93,   205,   143,    57,    39,   239,   143,
     218,    37,   226,   231,    57,   143,   143,     6,    37,    16,
     143,   218,   135,   136,   137,   223,    56,   255,   216,   218,
     143,    56,   232,   143,   227,   245,   239,     4,    15,    32,
      41,    42,    43,    44,    45,    54,    55,    64,    92,    94,
      98,   114,   134,   138,   142,   144,   179,   180,   183,   184,
     186,   189,   190,   191,   196,     4,    57,    16,    37,    37,
     150,    37,   187,    37,    37,     4,    54,   180,   182,    37,
     186,    57,    37,   195,   246,   233,    54,    55,    98,   138,
     179,    54,    55,   179,   188,   192,   216,    37,   185,   179,
     181,    37,   145,   197,   182,   228,   216,    56,    25,   243,
      37,    37,   143,    37,   143,    37,    37,   143,   190,   143,
     218,     4,   183,   143,   179,   180,   143,   243,   143,   190,
       4,   100,   219,   179,   179,   181,   179,   179,    36,   141,
     143,   179,   219,    57,    25,   244,   143,   143,   143,   143,
     143,     4,   244,   247,     4,     6,    16,   193,   143,   143,
     179,   143,   143,   143,   193,   244,   192,     4,   234,   143,
     195,   229,   193,   197,   197,   248,   197
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
#line 175 "ldgram.y"
    { ldlex_defsym(); }
    break;

  case 9:
#line 177 "ldgram.y"
    {
		  ldlex_popstate();
		  lang_add_assignment (exp_defsym ((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)));
		}
    break;

  case 10:
#line 185 "ldgram.y"
    {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
    break;

  case 11:
#line 190 "ldgram.y"
    {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
    break;

  case 16:
#line 205 "ldgram.y"
    {
			einfo(_("%P%F: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[(1) - (1)].name));
			}
    break;

  case 17:
#line 208 "ldgram.y"
    {
			config.map_filename = "-";
			}
    break;

  case 20:
#line 214 "ldgram.y"
    { mri_public((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)); }
    break;

  case 21:
#line 216 "ldgram.y"
    { mri_public((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)); }
    break;

  case 22:
#line 218 "ldgram.y"
    { mri_public((yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].etree)); }
    break;

  case 23:
#line 220 "ldgram.y"
    { mri_format((yyvsp[(2) - (2)].name)); }
    break;

  case 24:
#line 222 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree));}
    break;

  case 25:
#line 224 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].etree));}
    break;

  case 26:
#line 226 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree));}
    break;

  case 27:
#line 228 "ldgram.y"
    { mri_align((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 28:
#line 230 "ldgram.y"
    { mri_align((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 29:
#line 232 "ldgram.y"
    { mri_alignmod((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 30:
#line 234 "ldgram.y"
    { mri_alignmod((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 33:
#line 238 "ldgram.y"
    { mri_name((yyvsp[(2) - (2)].name)); }
    break;

  case 34:
#line 240 "ldgram.y"
    { mri_alias((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].name),0);}
    break;

  case 35:
#line 242 "ldgram.y"
    { mri_alias ((yyvsp[(2) - (4)].name), 0, (int) (yyvsp[(4) - (4)].bigint).integer); }
    break;

  case 36:
#line 244 "ldgram.y"
    { mri_base((yyvsp[(2) - (2)].etree)); }
    break;

  case 37:
#line 246 "ldgram.y"
    { mri_truncate ((unsigned int) (yyvsp[(2) - (2)].bigint).integer); }
    break;

  case 40:
#line 250 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 41:
#line 252 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 42:
#line 254 "ldgram.y"
    { lang_add_entry ((yyvsp[(2) - (2)].name), FALSE); }
    break;

  case 44:
#line 259 "ldgram.y"
    { mri_order((yyvsp[(3) - (3)].name)); }
    break;

  case 45:
#line 260 "ldgram.y"
    { mri_order((yyvsp[(2) - (2)].name)); }
    break;

  case 47:
#line 266 "ldgram.y"
    { mri_load((yyvsp[(1) - (1)].name)); }
    break;

  case 48:
#line 267 "ldgram.y"
    { mri_load((yyvsp[(3) - (3)].name)); }
    break;

  case 49:
#line 272 "ldgram.y"
    { mri_only_load((yyvsp[(1) - (1)].name)); }
    break;

  case 50:
#line 274 "ldgram.y"
    { mri_only_load((yyvsp[(3) - (3)].name)); }
    break;

  case 51:
#line 278 "ldgram.y"
    { (yyval.name) = NULL; }
    break;

  case 54:
#line 285 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 55:
#line 287 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 56:
#line 291 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(1) - (1)].name), FALSE); }
    break;

  case 57:
#line 293 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(2) - (2)].name), FALSE); }
    break;

  case 58:
#line 295 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(3) - (3)].name), FALSE); }
    break;

  case 59:
#line 299 "ldgram.y"
    { ldlex_both(); }
    break;

  case 60:
#line 301 "ldgram.y"
    { ldlex_popstate(); }
    break;

  case 73:
#line 322 "ldgram.y"
    { lang_add_target((yyvsp[(3) - (4)].name)); }
    break;

  case 74:
#line 324 "ldgram.y"
    { ldfile_add_library_path ((yyvsp[(3) - (4)].name), FALSE); }
    break;

  case 75:
#line 326 "ldgram.y"
    { lang_add_output((yyvsp[(3) - (4)].name), 1); }
    break;

  case 76:
#line 328 "ldgram.y"
    { lang_add_output_format ((yyvsp[(3) - (4)].name), (char *) NULL,
					    (char *) NULL, 1); }
    break;

  case 77:
#line 331 "ldgram.y"
    { lang_add_output_format ((yyvsp[(3) - (8)].name), (yyvsp[(5) - (8)].name), (yyvsp[(7) - (8)].name), 1); }
    break;

  case 78:
#line 333 "ldgram.y"
    { ldfile_set_output_arch ((yyvsp[(3) - (4)].name), bfd_arch_unknown); }
    break;

  case 79:
#line 335 "ldgram.y"
    { command_line.force_common_definition = TRUE ; }
    break;

  case 80:
#line 337 "ldgram.y"
    { command_line.inhibit_common_definition = TRUE ; }
    break;

  case 82:
#line 340 "ldgram.y"
    { lang_enter_group (); }
    break;

  case 83:
#line 342 "ldgram.y"
    { lang_leave_group (); }
    break;

  case 84:
#line 344 "ldgram.y"
    { lang_add_map((yyvsp[(3) - (4)].name)); }
    break;

  case 85:
#line 346 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 86:
#line 348 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 87:
#line 350 "ldgram.y"
    {
		  lang_add_nocrossref ((yyvsp[(3) - (4)].nocrossref));
		}
    break;

  case 89:
#line 355 "ldgram.y"
    { lang_add_insert ((yyvsp[(3) - (3)].name), 0); }
    break;

  case 90:
#line 357 "ldgram.y"
    { lang_add_insert ((yyvsp[(3) - (3)].name), 1); }
    break;

  case 91:
#line 359 "ldgram.y"
    { lang_memory_region_alias ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].name)); }
    break;

  case 92:
#line 361 "ldgram.y"
    { lang_ld_feature ((yyvsp[(3) - (4)].name)); }
    break;

  case 93:
#line 366 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 94:
#line 369 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 95:
#line 372 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 96:
#line 375 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 97:
#line 378 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 98:
#line 381 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 99:
#line 384 "ldgram.y"
    { (yyval.integer) = add_DT_NEEDED_for_regular; add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 100:
#line 386 "ldgram.y"
    { add_DT_NEEDED_for_regular = (yyvsp[(3) - (5)].integer); }
    break;

  case 101:
#line 388 "ldgram.y"
    { (yyval.integer) = add_DT_NEEDED_for_regular; add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 102:
#line 390 "ldgram.y"
    { add_DT_NEEDED_for_regular = (yyvsp[(5) - (7)].integer); }
    break;

  case 103:
#line 392 "ldgram.y"
    { (yyval.integer) = add_DT_NEEDED_for_regular; add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 104:
#line 394 "ldgram.y"
    { add_DT_NEEDED_for_regular = (yyvsp[(4) - (6)].integer); }
    break;

  case 109:
#line 409 "ldgram.y"
    { lang_add_entry ((yyvsp[(3) - (4)].name), FALSE); }
    break;

  case 111:
#line 411 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 112:
#line 412 "ldgram.y"
    { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[(4) - (7)].etree), (yyvsp[(6) - (7)].name))); }
    break;

  case 113:
#line 420 "ldgram.y"
    {
			  (yyval.cname) = (yyvsp[(1) - (1)].name);
			}
    break;

  case 114:
#line 424 "ldgram.y"
    {
			  (yyval.cname) = "*";
			}
    break;

  case 115:
#line 428 "ldgram.y"
    {
			  (yyval.cname) = "?";
			}
    break;

  case 116:
#line 435 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(1) - (1)].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 117:
#line 441 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (5)].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[(3) - (5)].name_list);
			}
    break;

  case 118:
#line 447 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 119:
#line 453 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 120:
#line 459 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_name_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 121:
#line 465 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 122:
#line 471 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_alignment_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 123:
#line 477 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 124:
#line 483 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(7) - (8)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = (yyvsp[(5) - (8)].name_list);
			}
    break;

  case 125:
#line 492 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[(2) - (2)].cname);
			  tmp->next = (yyvsp[(1) - (2)].name_list);
			  (yyval.name_list) = tmp;
			}
    break;

  case 126:
#line 501 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[(1) - (1)].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
    break;

  case 127:
#line 512 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[(1) - (3)].wildcard_list);
			  tmp->spec = (yyvsp[(3) - (3)].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 128:
#line 521 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[(1) - (1)].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 129:
#line 532 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[(1) - (1)].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
    break;

  case 130:
#line 540 "ldgram.y"
    {
			  lang_add_wild (NULL, (yyvsp[(2) - (3)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 131:
#line 544 "ldgram.y"
    {
			  lang_add_wild (&(yyvsp[(1) - (4)].wildcard), (yyvsp[(3) - (4)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 133:
#line 552 "ldgram.y"
    { ldgram_had_keep = TRUE; }
    break;

  case 134:
#line 554 "ldgram.y"
    { ldgram_had_keep = FALSE; }
    break;

  case 136:
#line 560 "ldgram.y"
    {
 		lang_add_attribute(lang_object_symbols_statement_enum);
	      	}
    break;

  case 138:
#line 565 "ldgram.y"
    {

		  lang_add_attribute(lang_constructors_statement_enum);
		}
    break;

  case 139:
#line 570 "ldgram.y"
    {
		  constructors_sorted = TRUE;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
    break;

  case 141:
#line 576 "ldgram.y"
    {
			  lang_add_data ((int) (yyvsp[(1) - (4)].integer), (yyvsp[(3) - (4)].etree));
			}
    break;

  case 142:
#line 581 "ldgram.y"
    {
			  lang_add_fill ((yyvsp[(3) - (4)].fill));
			}
    break;

  case 143:
#line 584 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 144:
#line 585 "ldgram.y"
    { ldlex_popstate ();
			  lang_add_assignment (exp_assert ((yyvsp[(4) - (8)].etree), (yyvsp[(6) - (8)].name))); }
    break;

  case 145:
#line 588 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 146:
#line 590 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 151:
#line 605 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 152:
#line 607 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 153:
#line 609 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 154:
#line 611 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 155:
#line 613 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 156:
#line 618 "ldgram.y"
    {
		  (yyval.fill) = exp_get_fill ((yyvsp[(1) - (1)].etree), 0, "fill value");
		}
    break;

  case 157:
#line 625 "ldgram.y"
    { (yyval.fill) = (yyvsp[(2) - (2)].fill); }
    break;

  case 158:
#line 626 "ldgram.y"
    { (yyval.fill) = (fill_type *) 0; }
    break;

  case 159:
#line 631 "ldgram.y"
    { (yyval.token) = '+'; }
    break;

  case 160:
#line 633 "ldgram.y"
    { (yyval.token) = '-'; }
    break;

  case 161:
#line 635 "ldgram.y"
    { (yyval.token) = '*'; }
    break;

  case 162:
#line 637 "ldgram.y"
    { (yyval.token) = '/'; }
    break;

  case 163:
#line 639 "ldgram.y"
    { (yyval.token) = LSHIFT; }
    break;

  case 164:
#line 641 "ldgram.y"
    { (yyval.token) = RSHIFT; }
    break;

  case 165:
#line 643 "ldgram.y"
    { (yyval.token) = '&'; }
    break;

  case 166:
#line 645 "ldgram.y"
    { (yyval.token) = '|'; }
    break;

  case 169:
#line 655 "ldgram.y"
    {
		  lang_add_assignment (exp_assign ((yyvsp[(1) - (3)].name), (yyvsp[(3) - (3)].etree)));
		}
    break;

  case 170:
#line 659 "ldgram.y"
    {
		  lang_add_assignment (exp_assign ((yyvsp[(1) - (3)].name),
						   exp_binop ((yyvsp[(2) - (3)].token),
							      exp_nameop (NAME,
									  (yyvsp[(1) - (3)].name)),
							      (yyvsp[(3) - (3)].etree))));
		}
    break;

  case 171:
#line 667 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), FALSE));
		}
    break;

  case 172:
#line 671 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), TRUE));
		}
    break;

  case 180:
#line 694 "ldgram.y"
    { region = lang_memory_region_lookup ((yyvsp[(1) - (1)].name), TRUE); }
    break;

  case 181:
#line 697 "ldgram.y"
    {}
    break;

  case 182:
#line 699 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 183:
#line 701 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 184:
#line 706 "ldgram.y"
    {
		  region->origin = exp_get_vma ((yyvsp[(3) - (3)].etree), 0, "origin");
		  region->current = region->origin;
		}
    break;

  case 185:
#line 714 "ldgram.y"
    {
		  region->length = exp_get_vma ((yyvsp[(3) - (3)].etree), -1, "length");
		}
    break;

  case 186:
#line 721 "ldgram.y"
    { /* dummy action to avoid bison 1.25 error message */ }
    break;

  case 190:
#line 732 "ldgram.y"
    { lang_set_flags (region, (yyvsp[(1) - (1)].name), 0); }
    break;

  case 191:
#line 734 "ldgram.y"
    { lang_set_flags (region, (yyvsp[(2) - (2)].name), 1); }
    break;

  case 192:
#line 739 "ldgram.y"
    { lang_startup((yyvsp[(3) - (4)].name)); }
    break;

  case 194:
#line 745 "ldgram.y"
    { ldemul_hll((char *)NULL); }
    break;

  case 195:
#line 750 "ldgram.y"
    { ldemul_hll((yyvsp[(3) - (3)].name)); }
    break;

  case 196:
#line 752 "ldgram.y"
    { ldemul_hll((yyvsp[(1) - (1)].name)); }
    break;

  case 198:
#line 760 "ldgram.y"
    { ldemul_syslib((yyvsp[(3) - (3)].name)); }
    break;

  case 200:
#line 766 "ldgram.y"
    { lang_float(TRUE); }
    break;

  case 201:
#line 768 "ldgram.y"
    { lang_float(FALSE); }
    break;

  case 202:
#line 773 "ldgram.y"
    {
		  (yyval.nocrossref) = NULL;
		}
    break;

  case 203:
#line 777 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[(1) - (2)].name);
		  n->next = (yyvsp[(2) - (2)].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 204:
#line 786 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[(1) - (3)].name);
		  n->next = (yyvsp[(3) - (3)].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 205:
#line 796 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 206:
#line 798 "ldgram.y"
    { ldlex_popstate (); (yyval.etree)=(yyvsp[(2) - (2)].etree);}
    break;

  case 207:
#line 803 "ldgram.y"
    { (yyval.etree) = exp_unop ('-', (yyvsp[(2) - (2)].etree)); }
    break;

  case 208:
#line 805 "ldgram.y"
    { (yyval.etree) = (yyvsp[(2) - (3)].etree); }
    break;

  case 209:
#line 807 "ldgram.y"
    { (yyval.etree) = exp_unop ((int) (yyvsp[(1) - (4)].integer),(yyvsp[(3) - (4)].etree)); }
    break;

  case 210:
#line 809 "ldgram.y"
    { (yyval.etree) = exp_unop ('!', (yyvsp[(2) - (2)].etree)); }
    break;

  case 211:
#line 811 "ldgram.y"
    { (yyval.etree) = (yyvsp[(2) - (2)].etree); }
    break;

  case 212:
#line 813 "ldgram.y"
    { (yyval.etree) = exp_unop ('~', (yyvsp[(2) - (2)].etree));}
    break;

  case 213:
#line 816 "ldgram.y"
    { (yyval.etree) = exp_binop ('*', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 214:
#line 818 "ldgram.y"
    { (yyval.etree) = exp_binop ('/', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 215:
#line 820 "ldgram.y"
    { (yyval.etree) = exp_binop ('%', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 216:
#line 822 "ldgram.y"
    { (yyval.etree) = exp_binop ('+', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 217:
#line 824 "ldgram.y"
    { (yyval.etree) = exp_binop ('-' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 218:
#line 826 "ldgram.y"
    { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 219:
#line 828 "ldgram.y"
    { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 220:
#line 830 "ldgram.y"
    { (yyval.etree) = exp_binop (EQ , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 221:
#line 832 "ldgram.y"
    { (yyval.etree) = exp_binop (NE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 222:
#line 834 "ldgram.y"
    { (yyval.etree) = exp_binop (LE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 223:
#line 836 "ldgram.y"
    { (yyval.etree) = exp_binop (GE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 224:
#line 838 "ldgram.y"
    { (yyval.etree) = exp_binop ('<' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 225:
#line 840 "ldgram.y"
    { (yyval.etree) = exp_binop ('>' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 226:
#line 842 "ldgram.y"
    { (yyval.etree) = exp_binop ('&' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 227:
#line 844 "ldgram.y"
    { (yyval.etree) = exp_binop ('^' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 228:
#line 846 "ldgram.y"
    { (yyval.etree) = exp_binop ('|' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 229:
#line 848 "ldgram.y"
    { (yyval.etree) = exp_trinop ('?' , (yyvsp[(1) - (5)].etree), (yyvsp[(3) - (5)].etree), (yyvsp[(5) - (5)].etree)); }
    break;

  case 230:
#line 850 "ldgram.y"
    { (yyval.etree) = exp_binop (ANDAND , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 231:
#line 852 "ldgram.y"
    { (yyval.etree) = exp_binop (OROR , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 232:
#line 854 "ldgram.y"
    { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[(3) - (4)].name)); }
    break;

  case 233:
#line 856 "ldgram.y"
    { (yyval.etree) = exp_bigintop ((yyvsp[(1) - (1)].bigint).integer, (yyvsp[(1) - (1)].bigint).str); }
    break;

  case 234:
#line 858 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
    break;

  case 235:
#line 861 "ldgram.y"
    { (yyval.etree) = exp_nameop (ALIGNOF,(yyvsp[(3) - (4)].name)); }
    break;

  case 236:
#line 863 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF,(yyvsp[(3) - (4)].name)); }
    break;

  case 237:
#line 865 "ldgram.y"
    { (yyval.etree) = exp_nameop (ADDR,(yyvsp[(3) - (4)].name)); }
    break;

  case 238:
#line 867 "ldgram.y"
    { (yyval.etree) = exp_nameop (LOADADDR,(yyvsp[(3) - (4)].name)); }
    break;

  case 239:
#line 869 "ldgram.y"
    { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[(3) - (4)].name)); }
    break;

  case 240:
#line 871 "ldgram.y"
    { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[(3) - (4)].etree)); }
    break;

  case 241:
#line 873 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[(3) - (4)].etree)); }
    break;

  case 242:
#line 875 "ldgram.y"
    { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[(3) - (6)].etree),(yyvsp[(5) - (6)].etree)); }
    break;

  case 243:
#line 877 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree)); }
    break;

  case 244:
#line 879 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[(5) - (6)].etree), (yyvsp[(3) - (6)].etree)); }
    break;

  case 245:
#line 881 "ldgram.y"
    { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[(3) - (4)].etree)); }
    break;

  case 246:
#line 883 "ldgram.y"
    { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[(5) - (6)].etree),
					  exp_nameop (NAME, (yyvsp[(3) - (6)].name))); }
    break;

  case 247:
#line 892 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[(3) - (4)].etree)); }
    break;

  case 248:
#line 894 "ldgram.y"
    { (yyval.etree) = exp_nameop (NAME,(yyvsp[(1) - (1)].name)); }
    break;

  case 249:
#line 896 "ldgram.y"
    { (yyval.etree) = exp_binop (MAX_K, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree) ); }
    break;

  case 250:
#line 898 "ldgram.y"
    { (yyval.etree) = exp_binop (MIN_K, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree) ); }
    break;

  case 251:
#line 900 "ldgram.y"
    { (yyval.etree) = exp_assert ((yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].name)); }
    break;

  case 252:
#line 902 "ldgram.y"
    { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[(3) - (4)].name)); }
    break;

  case 253:
#line 904 "ldgram.y"
    { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[(3) - (4)].name)); }
    break;

  case 254:
#line 909 "ldgram.y"
    { (yyval.name) = (yyvsp[(3) - (3)].name); }
    break;

  case 255:
#line 910 "ldgram.y"
    { (yyval.name) = 0; }
    break;

  case 256:
#line 914 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 257:
#line 915 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 258:
#line 919 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 259:
#line 920 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 260:
#line 924 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 261:
#line 925 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 262:
#line 929 "ldgram.y"
    { (yyval.token) = ONLY_IF_RO; }
    break;

  case 263:
#line 930 "ldgram.y"
    { (yyval.token) = ONLY_IF_RW; }
    break;

  case 264:
#line 931 "ldgram.y"
    { (yyval.token) = SPECIAL; }
    break;

  case 265:
#line 932 "ldgram.y"
    { (yyval.token) = 0; }
    break;

  case 266:
#line 935 "ldgram.y"
    { ldlex_expression(); }
    break;

  case 267:
#line 939 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 268:
#line 942 "ldgram.y"
    {
			  lang_enter_output_section_statement((yyvsp[(1) - (9)].name), (yyvsp[(3) - (9)].etree),
							      sectype,
							      (yyvsp[(5) - (9)].etree), (yyvsp[(6) - (9)].etree), (yyvsp[(4) - (9)].etree), (yyvsp[(8) - (9)].token));
			}
    break;

  case 269:
#line 948 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 270:
#line 950 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_leave_output_section_statement ((yyvsp[(17) - (17)].fill), (yyvsp[(14) - (17)].name), (yyvsp[(16) - (17)].section_phdr), (yyvsp[(15) - (17)].name));
		}
    break;

  case 271:
#line 955 "ldgram.y"
    {}
    break;

  case 272:
#line 957 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 273:
#line 959 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 274:
#line 961 "ldgram.y"
    {
			  lang_enter_overlay ((yyvsp[(3) - (8)].etree), (yyvsp[(6) - (8)].etree));
			}
    break;

  case 275:
#line 966 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 276:
#line 968 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay ((yyvsp[(5) - (16)].etree), (int) (yyvsp[(4) - (16)].integer),
					      (yyvsp[(16) - (16)].fill), (yyvsp[(13) - (16)].name), (yyvsp[(15) - (16)].section_phdr), (yyvsp[(14) - (16)].name));
			}
    break;

  case 278:
#line 978 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 279:
#line 980 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assign (".", (yyvsp[(3) - (3)].etree)));
		}
    break;

  case 281:
#line 986 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 282:
#line 988 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 283:
#line 992 "ldgram.y"
    { sectype = noload_section; }
    break;

  case 284:
#line 993 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 285:
#line 994 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 286:
#line 995 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 287:
#line 996 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 289:
#line 1001 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 290:
#line 1002 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 291:
#line 1006 "ldgram.y"
    { (yyval.etree) = (yyvsp[(1) - (3)].etree); }
    break;

  case 292:
#line 1007 "ldgram.y"
    { (yyval.etree) = (etree_type *)NULL;  }
    break;

  case 293:
#line 1012 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (6)].etree); }
    break;

  case 294:
#line 1014 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (10)].etree); }
    break;

  case 295:
#line 1018 "ldgram.y"
    { (yyval.etree) = (yyvsp[(1) - (2)].etree); }
    break;

  case 296:
#line 1019 "ldgram.y"
    { (yyval.etree) = (etree_type *) NULL;  }
    break;

  case 297:
#line 1024 "ldgram.y"
    { (yyval.integer) = 0; }
    break;

  case 298:
#line 1026 "ldgram.y"
    { (yyval.integer) = 1; }
    break;

  case 299:
#line 1031 "ldgram.y"
    { (yyval.name) = (yyvsp[(2) - (2)].name); }
    break;

  case 300:
#line 1032 "ldgram.y"
    { (yyval.name) = DEFAULT_MEMORY_REGION; }
    break;

  case 301:
#line 1037 "ldgram.y"
    {
		  (yyval.section_phdr) = NULL;
		}
    break;

  case 302:
#line 1041 "ldgram.y"
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

  case 304:
#line 1057 "ldgram.y"
    {
			  ldlex_script ();
			  lang_enter_overlay_section ((yyvsp[(2) - (2)].name));
			}
    break;

  case 305:
#line 1062 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 306:
#line 1064 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay_section ((yyvsp[(9) - (9)].fill), (yyvsp[(8) - (9)].section_phdr));
			}
    break;

  case 311:
#line 1081 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 312:
#line 1082 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 313:
#line 1084 "ldgram.y"
    {
		  lang_new_phdr ((yyvsp[(1) - (6)].name), (yyvsp[(3) - (6)].etree), (yyvsp[(4) - (6)].phdr).filehdr, (yyvsp[(4) - (6)].phdr).phdrs, (yyvsp[(4) - (6)].phdr).at,
				 (yyvsp[(4) - (6)].phdr).flags);
		}
    break;

  case 314:
#line 1092 "ldgram.y"
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
				     s);
			      (yyval.etree) = exp_intop (0);
			    }
			}
		    }
		}
    break;

  case 315:
#line 1136 "ldgram.y"
    {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
    break;

  case 316:
#line 1140 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[(3) - (3)].phdr);
		  if (strcmp ((yyvsp[(1) - (3)].name), "FILEHDR") == 0 && (yyvsp[(2) - (3)].etree) == NULL)
		    (yyval.phdr).filehdr = TRUE;
		  else if (strcmp ((yyvsp[(1) - (3)].name), "PHDRS") == 0 && (yyvsp[(2) - (3)].etree) == NULL)
		    (yyval.phdr).phdrs = TRUE;
		  else if (strcmp ((yyvsp[(1) - (3)].name), "FLAGS") == 0 && (yyvsp[(2) - (3)].etree) != NULL)
		    (yyval.phdr).flags = (yyvsp[(2) - (3)].etree);
		  else
		    einfo (_("%X%P:%S: PHDRS syntax error at `%s'\n"), (yyvsp[(1) - (3)].name));
		}
    break;

  case 317:
#line 1152 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[(5) - (5)].phdr);
		  (yyval.phdr).at = (yyvsp[(3) - (5)].etree);
		}
    break;

  case 318:
#line 1160 "ldgram.y"
    {
		  (yyval.etree) = NULL;
		}
    break;

  case 319:
#line 1164 "ldgram.y"
    {
		  (yyval.etree) = (yyvsp[(2) - (3)].etree);
		}
    break;

  case 320:
#line 1170 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
    break;

  case 321:
#line 1175 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 325:
#line 1192 "ldgram.y"
    {
		  lang_append_dynamic_list ((yyvsp[(1) - (2)].versyms));
		}
    break;

  case 326:
#line 1200 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
    break;

  case 327:
#line 1205 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 328:
#line 1214 "ldgram.y"
    {
		  ldlex_version_script ();
		}
    break;

  case 329:
#line 1218 "ldgram.y"
    {
		  ldlex_popstate ();
		}
    break;

  case 332:
#line 1230 "ldgram.y"
    {
		  lang_register_vers_node (NULL, (yyvsp[(2) - (4)].versnode), NULL);
		}
    break;

  case 333:
#line 1234 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[(1) - (5)].name), (yyvsp[(3) - (5)].versnode), NULL);
		}
    break;

  case 334:
#line 1238 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[(1) - (6)].name), (yyvsp[(3) - (6)].versnode), (yyvsp[(5) - (6)].deflist));
		}
    break;

  case 335:
#line 1245 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[(1) - (1)].name));
		}
    break;

  case 336:
#line 1249 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[(1) - (2)].deflist), (yyvsp[(2) - (2)].name));
		}
    break;

  case 337:
#line 1256 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
    break;

  case 338:
#line 1260 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(1) - (2)].versyms), NULL);
		}
    break;

  case 339:
#line 1264 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(3) - (4)].versyms), NULL);
		}
    break;

  case 340:
#line 1268 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[(3) - (4)].versyms));
		}
    break;

  case 341:
#line 1272 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(3) - (8)].versyms), (yyvsp[(7) - (8)].versyms));
		}
    break;

  case 342:
#line 1279 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[(1) - (1)].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 343:
#line 1283 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[(1) - (1)].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 344:
#line 1287 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), (yyvsp[(3) - (3)].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 345:
#line 1291 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), (yyvsp[(3) - (3)].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 346:
#line 1295 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[(4) - (5)].name);
			}
    break;

  case 347:
#line 1300 "ldgram.y"
    {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[(7) - (9)].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[(1) - (9)].versyms);
			  (yyval.versyms) = (yyvsp[(7) - (9)].versyms);
			  ldgram_vers_current_lang = (yyvsp[(6) - (9)].name);
			}
    break;

  case 348:
#line 1308 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[(2) - (3)].name);
			}
    break;

  case 349:
#line 1313 "ldgram.y"
    {
			  (yyval.versyms) = (yyvsp[(5) - (7)].versyms);
			  ldgram_vers_current_lang = (yyvsp[(4) - (7)].name);
			}
    break;

  case 350:
#line 1318 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 351:
#line 1322 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 352:
#line 1326 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 353:
#line 1330 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 354:
#line 1334 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 355:
#line 1338 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "extern", ldgram_vers_current_lang, FALSE);
		}
    break;


/* Line 1267 of yacc.c.  */
#line 4294 "ldgram.c"
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


#line 1348 "ldgram.y"

void
yyerror(arg)
     const char *arg;
{
  if (ldfile_assumed_script)
    einfo (_("%P:%s: file format not recognized; treating as linker script\n"),
	   ldfile_input_filename);
  if (error_index > 0 && error_index < ERROR_NAME_MAX)
     einfo ("%P%F:%S: %s in %s\n", arg, error_names[error_index-1]);
  else
     einfo ("%P%F:%S: %s\n", arg);
}

