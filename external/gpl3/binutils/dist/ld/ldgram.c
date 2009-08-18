/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

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
     NOLOAD = 305,
     DSECT = 306,
     COPY = 307,
     INFO = 308,
     OVERLAY = 309,
     DEFINED = 310,
     TARGET_K = 311,
     SEARCH_DIR = 312,
     MAP = 313,
     ENTRY = 314,
     NEXT = 315,
     SIZEOF = 316,
     ALIGNOF = 317,
     ADDR = 318,
     LOADADDR = 319,
     MAX_K = 320,
     MIN_K = 321,
     STARTUP = 322,
     HLL = 323,
     SYSLIB = 324,
     FLOAT = 325,
     NOFLOAT = 326,
     NOCROSSREFS = 327,
     ORIGIN = 328,
     FILL = 329,
     LENGTH = 330,
     CREATE_OBJECT_SYMBOLS = 331,
     INPUT = 332,
     GROUP = 333,
     OUTPUT = 334,
     CONSTRUCTORS = 335,
     ALIGNMOD = 336,
     AT = 337,
     SUBALIGN = 338,
     PROVIDE = 339,
     PROVIDE_HIDDEN = 340,
     AS_NEEDED = 341,
     CHIP = 342,
     LIST = 343,
     SECT = 344,
     ABSOLUTE = 345,
     LOAD = 346,
     NEWLINE = 347,
     ENDWORD = 348,
     ORDER = 349,
     NAMEWORD = 350,
     ASSERT_K = 351,
     FORMAT = 352,
     PUBLIC = 353,
     DEFSYMEND = 354,
     BASE = 355,
     ALIAS = 356,
     TRUNCATE = 357,
     REL = 358,
     INPUT_SCRIPT = 359,
     INPUT_MRI_SCRIPT = 360,
     INPUT_DEFSYM = 361,
     CASE = 362,
     EXTERN = 363,
     START = 364,
     VERS_TAG = 365,
     VERS_IDENTIFIER = 366,
     GLOBAL = 367,
     LOCAL = 368,
     VERSIONK = 369,
     INPUT_VERSION_SCRIPT = 370,
     KEEP = 371,
     ONLY_IF_RO = 372,
     ONLY_IF_RW = 373,
     SPECIAL = 374,
     EXCLUDE_FILE = 375,
     CONSTANT = 376,
     INPUT_DYNAMIC_LIST = 377
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
#define NOLOAD 305
#define DSECT 306
#define COPY 307
#define INFO 308
#define OVERLAY 309
#define DEFINED 310
#define TARGET_K 311
#define SEARCH_DIR 312
#define MAP 313
#define ENTRY 314
#define NEXT 315
#define SIZEOF 316
#define ALIGNOF 317
#define ADDR 318
#define LOADADDR 319
#define MAX_K 320
#define MIN_K 321
#define STARTUP 322
#define HLL 323
#define SYSLIB 324
#define FLOAT 325
#define NOFLOAT 326
#define NOCROSSREFS 327
#define ORIGIN 328
#define FILL 329
#define LENGTH 330
#define CREATE_OBJECT_SYMBOLS 331
#define INPUT 332
#define GROUP 333
#define OUTPUT 334
#define CONSTRUCTORS 335
#define ALIGNMOD 336
#define AT 337
#define SUBALIGN 338
#define PROVIDE 339
#define PROVIDE_HIDDEN 340
#define AS_NEEDED 341
#define CHIP 342
#define LIST 343
#define SECT 344
#define ABSOLUTE 345
#define LOAD 346
#define NEWLINE 347
#define ENDWORD 348
#define ORDER 349
#define NAMEWORD 350
#define ASSERT_K 351
#define FORMAT 352
#define PUBLIC 353
#define DEFSYMEND 354
#define BASE 355
#define ALIAS 356
#define TRUNCATE 357
#define REL 358
#define INPUT_SCRIPT 359
#define INPUT_MRI_SCRIPT 360
#define INPUT_DEFSYM 361
#define CASE 362
#define EXTERN 363
#define START 364
#define VERS_TAG 365
#define VERS_IDENTIFIER 366
#define GLOBAL 367
#define LOCAL 368
#define VERSIONK 369
#define INPUT_VERSION_SCRIPT 370
#define KEEP 371
#define ONLY_IF_RO 372
#define ONLY_IF_RW 373
#define SPECIAL 374
#define EXCLUDE_FILE 375
#define CONSTANT 376
#define INPUT_DYNAMIC_LIST 377




/* Copy the first part of user declarations.  */
#line 23 "ldgram.y"

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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 61 "ldgram.y"
typedef union YYSTYPE {
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
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 397 "ldgram.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 409 "ldgram.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
      while (0)
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
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1766

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  146
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  126
/* YYNRULES -- Number of rules. */
#define YYNRULES  355
/* YYNRULES -- Number of states. */
#define YYNSTATES  757

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   377

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   144,     2,     2,     2,    34,    21,     2,
      37,   141,    32,    30,   139,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   140,
      24,     6,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   142,     2,   143,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    56,    19,    57,   145,     2,     2,     2,
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
     131,   132,   133,   134,   135,   136,   137,   138
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
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
     289,   293,   295,   299,   302,   304,   308,   311,   312,   318,
     319,   327,   328,   335,   340,   343,   346,   347,   352,   355,
     356,   364,   366,   368,   370,   372,   378,   383,   388,   396,
     404,   412,   420,   429,   432,   434,   438,   440,   442,   446,
     451,   453,   454,   460,   463,   465,   467,   469,   474,   476,
     481,   486,   487,   496,   497,   503,   506,   508,   509,   511,
     513,   515,   517,   519,   521,   523,   526,   527,   529,   531,
     533,   535,   537,   539,   541,   543,   545,   547,   551,   555,
     562,   569,   571,   572,   577,   579,   580,   584,   586,   587,
     595,   596,   602,   606,   610,   611,   615,   617,   620,   622,
     625,   630,   635,   639,   643,   645,   650,   654,   655,   657,
     659,   660,   663,   667,   668,   671,   674,   678,   683,   686,
     689,   692,   696,   700,   704,   708,   712,   716,   720,   724,
     728,   732,   736,   740,   744,   748,   752,   756,   762,   766,
     770,   775,   777,   779,   784,   789,   794,   799,   804,   809,
     814,   821,   828,   835,   840,   847,   852,   854,   861,   868,
     875,   880,   885,   889,   890,   895,   896,   901,   902,   907,
     908,   910,   912,   914,   915,   916,   917,   918,   919,   920,
     940,   941,   942,   943,   944,   945,   964,   965,   966,   974,
     975,   981,   983,   985,   987,   989,   991,   995,   996,   999,
    1003,  1006,  1013,  1024,  1027,  1029,  1030,  1032,  1035,  1036,
    1037,  1041,  1042,  1043,  1044,  1045,  1057,  1062,  1063,  1066,
    1067,  1068,  1075,  1077,  1078,  1082,  1088,  1089,  1093,  1094,
    1097,  1099,  1102,  1107,  1110,  1111,  1114,  1115,  1121,  1123,
    1126,  1131,  1137,  1144,  1146,  1149,  1150,  1153,  1158,  1163,
    1172,  1174,  1176,  1180,  1184,  1185,  1195,  1196,  1204,  1206,
    1210,  1212,  1216,  1218,  1222,  1223
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     147,     0,    -1,   120,   163,    -1,   121,   151,    -1,   131,
     260,    -1,   138,   255,    -1,   122,   149,    -1,     4,    -1,
      -1,   150,     4,     6,   216,    -1,    -1,   152,   153,    -1,
     153,   154,   108,    -1,    -1,   103,   216,    -1,   103,   216,
     139,   216,    -1,     4,    -1,   104,    -1,   110,   156,    -1,
     109,    -1,   114,     4,     6,   216,    -1,   114,     4,   139,
     216,    -1,   114,     4,   216,    -1,   113,     4,    -1,   105,
       4,   139,   216,    -1,   105,     4,   216,    -1,   105,     4,
       6,   216,    -1,    38,     4,     6,   216,    -1,    38,     4,
     139,   216,    -1,    97,     4,     6,   216,    -1,    97,     4,
     139,   216,    -1,   106,   158,    -1,   107,   157,    -1,   111,
       4,    -1,   117,     4,   139,     4,    -1,   117,     4,   139,
       3,    -1,   116,   216,    -1,   118,     3,    -1,   123,   159,
      -1,   124,   160,    -1,    -1,    64,   148,   155,   153,    36,
      -1,   125,     4,    -1,    -1,   156,   139,     4,    -1,   156,
       4,    -1,    -1,     4,    -1,   157,   139,     4,    -1,     4,
      -1,   158,   139,     4,    -1,    -1,     4,    -1,   159,   139,
       4,    -1,    -1,   161,   162,    -1,     4,    -1,   162,     4,
      -1,   162,   139,     4,    -1,    -1,   164,   165,    -1,   165,
     166,    -1,    -1,   196,    -1,   173,    -1,   247,    -1,   207,
      -1,   208,    -1,   210,    -1,   212,    -1,   175,    -1,   262,
      -1,   140,    -1,    72,    37,     4,   141,    -1,    73,    37,
     148,   141,    -1,    95,    37,   148,   141,    -1,    59,    37,
       4,   141,    -1,    59,    37,     4,   139,     4,   139,     4,
     141,    -1,    61,    37,     4,   141,    -1,    60,    -1,    62,
      -1,    93,    37,   169,   141,    -1,    -1,    94,   167,    37,
     169,   141,    -1,    74,    37,   148,   141,    -1,    -1,    64,
     148,   168,   165,    36,    -1,    88,    37,   213,   141,    -1,
     124,    37,   160,   141,    -1,    48,    49,     4,    -1,    48,
      50,     4,    -1,     4,    -1,   169,   139,     4,    -1,   169,
       4,    -1,     5,    -1,   169,   139,     5,    -1,   169,     5,
      -1,    -1,   102,    37,   170,   169,   141,    -1,    -1,   169,
     139,   102,    37,   171,   169,   141,    -1,    -1,   169,   102,
      37,   172,   169,   141,    -1,    46,    56,   174,    57,    -1,
     174,   222,    -1,   174,   175,    -1,    -1,    75,    37,     4,
     141,    -1,   194,   193,    -1,    -1,   112,   176,    37,   216,
     139,     4,   141,    -1,     4,    -1,    32,    -1,    15,    -1,
     177,    -1,   136,    37,   179,   141,   177,    -1,    54,    37,
     177,   141,    -1,    55,    37,   177,   141,    -1,    54,    37,
      55,    37,   177,   141,   141,    -1,    54,    37,    54,    37,
     177,   141,   141,    -1,    55,    37,    54,    37,   177,   141,
     141,    -1,    55,    37,    55,    37,   177,   141,   141,    -1,
      54,    37,   136,    37,   179,   141,   177,   141,    -1,   179,
     177,    -1,   177,    -1,   180,   195,   178,    -1,   178,    -1,
       4,    -1,   142,   180,   143,    -1,   178,    37,   180,   141,
      -1,   181,    -1,    -1,   132,    37,   183,   181,   141,    -1,
     194,   193,    -1,    92,    -1,   140,    -1,    96,    -1,    54,
      37,    96,   141,    -1,   182,    -1,   189,    37,   214,   141,
      -1,    90,    37,   190,   141,    -1,    -1,   112,   185,    37,
     216,   139,     4,   141,   193,    -1,    -1,    64,   148,   186,
     188,    36,    -1,   187,   184,    -1,   184,    -1,    -1,   187,
      -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,
      -1,   214,    -1,     6,   190,    -1,    -1,    14,    -1,    13,
      -1,    12,    -1,    11,    -1,    10,    -1,     9,    -1,     8,
      -1,     7,    -1,   140,    -1,   139,    -1,     4,     6,   214,
      -1,     4,   192,   214,    -1,   100,    37,     4,     6,   214,
     141,    -1,   101,    37,     4,     6,   214,   141,    -1,   139,
      -1,    -1,    65,    56,   197,    57,    -1,   198,    -1,    -1,
     198,   195,   199,    -1,   199,    -1,    -1,     4,   200,   204,
      16,   202,   195,   203,    -1,    -1,    64,   148,   201,   197,
      36,    -1,    89,     6,   214,    -1,    91,     6,   214,    -1,
      -1,    37,   205,   141,    -1,   206,    -1,   205,   206,    -1,
       4,    -1,   144,     4,    -1,    83,    37,   148,   141,    -1,
      84,    37,   209,   141,    -1,    84,    37,   141,    -1,   209,
     195,   148,    -1,   148,    -1,    85,    37,   211,   141,    -1,
     211,   195,   148,    -1,    -1,    86,    -1,    87,    -1,    -1,
       4,   213,    -1,     4,   139,   213,    -1,    -1,   215,   216,
      -1,    31,   216,    -1,    37,   216,   141,    -1,    76,    37,
     216,   141,    -1,   144,   216,    -1,    30,   216,    -1,   145,
     216,    -1,   216,    32,   216,    -1,   216,    33,   216,    -1,
     216,    34,   216,    -1,   216,    30,   216,    -1,   216,    31,
     216,    -1,   216,    29,   216,    -1,   216,    28,   216,    -1,
     216,    23,   216,    -1,   216,    22,   216,    -1,   216,    27,
     216,    -1,   216,    26,   216,    -1,   216,    24,   216,    -1,
     216,    25,   216,    -1,   216,    21,   216,    -1,   216,    20,
     216,    -1,   216,    19,   216,    -1,   216,    15,   216,    16,
     216,    -1,   216,    18,   216,    -1,   216,    17,   216,    -1,
      71,    37,     4,   141,    -1,     3,    -1,    58,    -1,    78,
      37,     4,   141,    -1,    77,    37,     4,   141,    -1,    79,
      37,     4,   141,    -1,    80,    37,     4,   141,    -1,   137,
      37,     4,   141,    -1,   106,    37,   216,   141,    -1,    38,
      37,   216,   141,    -1,    38,    37,   216,   139,   216,   141,
      -1,    51,    37,   216,   139,   216,   141,    -1,    52,    37,
     216,   139,   216,   141,    -1,    53,    37,   216,   141,    -1,
      63,    37,     4,   139,   216,   141,    -1,    39,    37,   216,
     141,    -1,     4,    -1,    81,    37,   216,   139,   216,   141,
      -1,    82,    37,   216,   139,   216,   141,    -1,   112,    37,
     216,   139,     4,   141,    -1,    89,    37,     4,   141,    -1,
      91,    37,     4,   141,    -1,    98,    25,     4,    -1,    -1,
      98,    37,   216,   141,    -1,    -1,    38,    37,   216,   141,
      -1,    -1,    99,    37,   216,   141,    -1,    -1,   133,    -1,
     134,    -1,   135,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       4,   223,   238,   218,   219,   220,   224,   221,    56,   225,
     188,    57,   226,   241,   217,   242,   191,   227,   195,    -1,
      -1,    -1,    -1,    -1,    -1,    70,   228,   239,   240,   218,
     220,   229,    56,   230,   243,    57,   231,   241,   217,   242,
     191,   232,   195,    -1,    -1,    -1,    94,   233,   238,   234,
      56,   174,    57,    -1,    -1,    64,   148,   235,   174,    36,
      -1,    66,    -1,    67,    -1,    68,    -1,    69,    -1,    70,
      -1,    37,   236,   141,    -1,    -1,    37,   141,    -1,   216,
     237,    16,    -1,   237,    16,    -1,    40,    37,   216,   141,
     237,    16,    -1,    40,    37,   216,   141,    39,    37,   216,
     141,   237,    16,    -1,   216,    16,    -1,    16,    -1,    -1,
      88,    -1,    25,     4,    -1,    -1,    -1,   242,    16,     4,
      -1,    -1,    -1,    -1,    -1,   243,     4,   244,    56,   188,
      57,   245,   242,   191,   246,   195,    -1,    47,    56,   248,
      57,    -1,    -1,   248,   249,    -1,    -1,    -1,     4,   250,
     252,   253,   251,   140,    -1,   216,    -1,    -1,     4,   254,
     253,    -1,    98,    37,   216,   141,   253,    -1,    -1,    37,
     216,   141,    -1,    -1,   256,   257,    -1,   258,    -1,   257,
     258,    -1,    56,   259,    57,   140,    -1,   268,   140,    -1,
      -1,   261,   264,    -1,    -1,   263,   130,    56,   264,    57,
      -1,   265,    -1,   264,   265,    -1,    56,   267,    57,   140,
      -1,   126,    56,   267,    57,   140,    -1,   126,    56,   267,
      57,   266,   140,    -1,   126,    -1,   266,   126,    -1,    -1,
     268,   140,    -1,   128,    16,   268,   140,    -1,   129,    16,
     268,   140,    -1,   128,    16,   268,   140,   129,    16,   268,
     140,    -1,   127,    -1,     4,    -1,   268,   140,   127,    -1,
     268,   140,     4,    -1,    -1,   268,   140,   124,     4,    56,
     269,   268,   271,    57,    -1,    -1,   124,     4,    56,   270,
     268,   271,    57,    -1,   128,    -1,   268,   140,   128,    -1,
     129,    -1,   268,   140,   129,    -1,   124,    -1,   268,   140,
     124,    -1,    -1,   140,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   160,   160,   161,   162,   163,   164,   168,   172,   172,
     182,   182,   195,   196,   200,   201,   202,   205,   208,   209,
     210,   212,   214,   216,   218,   220,   222,   224,   226,   228,
     230,   232,   233,   234,   236,   238,   240,   242,   244,   245,
     247,   246,   250,   252,   256,   257,   258,   262,   264,   268,
     270,   275,   276,   277,   282,   282,   287,   289,   291,   296,
     296,   302,   303,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   320,   322,   324,   327,   329,   331,
     333,   335,   337,   336,   340,   343,   342,   346,   350,   351,
     353,   358,   361,   364,   367,   370,   373,   377,   376,   381,
     380,   385,   384,   391,   395,   396,   397,   401,   403,   404,
     404,   412,   416,   420,   427,   433,   439,   445,   451,   457,
     463,   469,   475,   484,   493,   504,   513,   524,   532,   536,
     543,   545,   544,   551,   552,   556,   557,   562,   567,   568,
     573,   577,   577,   581,   580,   587,   588,   591,   593,   597,
     599,   601,   603,   605,   610,   617,   619,   623,   625,   627,
     629,   631,   633,   635,   637,   642,   642,   647,   651,   659,
     663,   671,   671,   675,   678,   678,   681,   682,   687,   686,
     692,   691,   698,   706,   714,   715,   719,   720,   724,   726,
     731,   736,   737,   742,   744,   750,   752,   754,   758,   760,
     766,   769,   778,   789,   789,   795,   797,   799,   801,   803,
     805,   808,   810,   812,   814,   816,   818,   820,   822,   824,
     826,   828,   830,   832,   834,   836,   838,   840,   842,   844,
     846,   848,   850,   853,   855,   857,   859,   861,   863,   865,
     867,   869,   871,   873,   875,   884,   886,   888,   890,   892,
     894,   896,   902,   903,   907,   908,   912,   913,   917,   918,
     922,   923,   924,   925,   928,   932,   935,   941,   943,   928,
     950,   952,   954,   959,   961,   949,   971,   973,   971,   979,
     978,   985,   986,   987,   988,   989,   993,   994,   995,   999,
    1000,  1005,  1006,  1011,  1012,  1017,  1018,  1023,  1025,  1030,
    1033,  1046,  1050,  1055,  1057,  1048,  1065,  1068,  1070,  1074,
    1075,  1074,  1084,  1129,  1132,  1144,  1153,  1156,  1163,  1163,
    1175,  1176,  1180,  1184,  1193,  1193,  1207,  1207,  1217,  1218,
    1222,  1226,  1230,  1237,  1241,  1249,  1252,  1256,  1260,  1264,
    1271,  1275,  1279,  1283,  1288,  1287,  1301,  1300,  1310,  1314,
    1318,  1322,  1326,  1330,  1336,  1338
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
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
  "MEMORY", "NOLOAD", "DSECT", "COPY", "INFO", "OVERLAY", "DEFINED",
  "TARGET_K", "SEARCH_DIR", "MAP", "ENTRY", "NEXT", "SIZEOF", "ALIGNOF",
  "ADDR", "LOADADDR", "MAX_K", "MIN_K", "STARTUP", "HLL", "SYSLIB",
  "FLOAT", "NOFLOAT", "NOCROSSREFS", "ORIGIN", "FILL", "LENGTH",
  "CREATE_OBJECT_SYMBOLS", "INPUT", "GROUP", "OUTPUT", "CONSTRUCTORS",
  "ALIGNMOD", "AT", "SUBALIGN", "PROVIDE", "PROVIDE_HIDDEN", "AS_NEEDED",
  "CHIP", "LIST", "SECT", "ABSOLUTE", "LOAD", "NEWLINE", "ENDWORD",
  "ORDER", "NAMEWORD", "ASSERT_K", "FORMAT", "PUBLIC", "DEFSYMEND", "BASE",
  "ALIAS", "TRUNCATE", "REL", "INPUT_SCRIPT", "INPUT_MRI_SCRIPT",
  "INPUT_DEFSYM", "CASE", "EXTERN", "START", "VERS_TAG", "VERS_IDENTIFIER",
  "GLOBAL", "LOCAL", "VERSIONK", "INPUT_VERSION_SCRIPT", "KEEP",
  "ONLY_IF_RO", "ONLY_IF_RW", "SPECIAL", "EXCLUDE_FILE", "CONSTANT",
  "INPUT_DYNAMIC_LIST", "','", "';'", "')'", "'['", "']'", "'!'", "'~'",
  "$accept", "file", "filename", "defsym_expr", "@1", "mri_script_file",
  "@2", "mri_script_lines", "mri_script_command", "@3", "ordernamelist",
  "mri_load_name_list", "mri_abs_name_list", "casesymlist",
  "extern_name_list", "@4", "extern_name_list_body", "script_file", "@5",
  "ifile_list", "ifile_p1", "@6", "@7", "input_list", "@8", "@9", "@10",
  "sections", "sec_or_group_p1", "statement_anywhere", "@11",
  "wildcard_name", "wildcard_spec", "exclude_name_list", "file_NAME_list",
  "input_section_spec_no_keep", "input_section_spec", "@12", "statement",
  "@13", "@14", "statement_list", "statement_list_opt", "length",
  "fill_exp", "fill_opt", "assign_op", "end", "assignment", "opt_comma",
  "memory", "memory_spec_list_opt", "memory_spec_list", "memory_spec",
  "@15", "@16", "origin_spec", "length_spec", "attributes_opt",
  "attributes_list", "attributes_string", "startup", "high_level_library",
  "high_level_library_NAME_list", "low_level_library",
  "low_level_library_NAME_list", "floating_point_support",
  "nocrossref_list", "mustbe_exp", "@17", "exp", "memspec_at_opt",
  "opt_at", "opt_align", "opt_subalign", "sect_constraint", "section",
  "@18", "@19", "@20", "@21", "@22", "@23", "@24", "@25", "@26", "@27",
  "@28", "@29", "@30", "type", "atype", "opt_exp_with_type",
  "opt_exp_without_type", "opt_nocrossrefs", "memspec_opt", "phdr_opt",
  "overlay_section", "@31", "@32", "@33", "phdrs", "phdr_list", "phdr",
  "@34", "@35", "phdr_type", "phdr_qualifiers", "phdr_val",
  "dynamic_list_file", "@36", "dynamic_list_nodes", "dynamic_list_node",
  "dynamic_list_tag", "version_script_file", "@37", "version", "@38",
  "vers_nodes", "vers_node", "verdep", "vers_tag", "vers_defns", "@39",
  "@40", "opt_semicolon", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
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
     369,   370,   371,   372,   373,   374,   375,   376,   377,    44,
      59,    41,    91,    93,    33,   126
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short int yyr1[] =
{
       0,   146,   147,   147,   147,   147,   147,   148,   150,   149,
     152,   151,   153,   153,   154,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   154,   154,   154,   154,   154,
     155,   154,   154,   154,   156,   156,   156,   157,   157,   158,
     158,   159,   159,   159,   161,   160,   162,   162,   162,   164,
     163,   165,   165,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   167,   166,   166,   168,   166,   166,   166,   166,
     166,   169,   169,   169,   169,   169,   169,   170,   169,   171,
     169,   172,   169,   173,   174,   174,   174,   175,   175,   176,
     175,   177,   177,   177,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   179,   179,   180,   180,   181,   181,   181,
     182,   183,   182,   184,   184,   184,   184,   184,   184,   184,
     184,   185,   184,   186,   184,   187,   187,   188,   188,   189,
     189,   189,   189,   189,   190,   191,   191,   192,   192,   192,
     192,   192,   192,   192,   192,   193,   193,   194,   194,   194,
     194,   195,   195,   196,   197,   197,   198,   198,   200,   199,
     201,   199,   202,   203,   204,   204,   205,   205,   206,   206,
     207,   208,   208,   209,   209,   210,   211,   211,   212,   212,
     213,   213,   213,   215,   214,   216,   216,   216,   216,   216,
     216,   216,   216,   216,   216,   216,   216,   216,   216,   216,
     216,   216,   216,   216,   216,   216,   216,   216,   216,   216,
     216,   216,   216,   216,   216,   216,   216,   216,   216,   216,
     216,   216,   216,   216,   216,   216,   216,   216,   216,   216,
     216,   216,   217,   217,   218,   218,   219,   219,   220,   220,
     221,   221,   221,   221,   223,   224,   225,   226,   227,   222,
     228,   229,   230,   231,   232,   222,   233,   234,   222,   235,
     222,   236,   236,   236,   236,   236,   237,   237,   237,   238,
     238,   238,   238,   239,   239,   240,   240,   241,   241,   242,
     242,   243,   244,   245,   246,   243,   247,   248,   248,   250,
     251,   249,   252,   253,   253,   253,   254,   254,   256,   255,
     257,   257,   258,   259,   261,   260,   263,   262,   264,   264,
     265,   265,   265,   266,   266,   267,   267,   267,   267,   267,
     268,   268,   268,   268,   269,   268,   270,   268,   268,   268,
     268,   268,   268,   268,   271,   271
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
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
       3,     1,     3,     2,     1,     3,     2,     0,     5,     0,
       7,     0,     6,     4,     2,     2,     0,     4,     2,     0,
       7,     1,     1,     1,     1,     5,     4,     4,     7,     7,
       7,     7,     8,     2,     1,     3,     1,     1,     3,     4,
       1,     0,     5,     2,     1,     1,     1,     4,     1,     4,
       4,     0,     8,     0,     5,     2,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     2,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     6,
       6,     1,     0,     4,     1,     0,     3,     1,     0,     7,
       0,     5,     3,     3,     0,     3,     1,     2,     1,     2,
       4,     4,     3,     3,     1,     4,     3,     0,     1,     1,
       0,     2,     3,     0,     2,     2,     3,     4,     2,     2,
       2,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     5,     3,     3,
       4,     1,     1,     4,     4,     4,     4,     4,     4,     4,
       6,     6,     6,     4,     6,     4,     1,     6,     6,     6,
       4,     4,     3,     0,     4,     0,     4,     0,     4,     0,
       1,     1,     1,     0,     0,     0,     0,     0,     0,    19,
       0,     0,     0,     0,     0,    18,     0,     0,     7,     0,
       5,     1,     1,     1,     1,     1,     3,     0,     2,     3,
       2,     6,    10,     2,     1,     0,     1,     2,     0,     0,
       3,     0,     0,     0,     0,    11,     4,     0,     2,     0,
       0,     6,     1,     0,     3,     5,     0,     3,     0,     2,
       1,     2,     4,     2,     0,     2,     0,     5,     1,     2,
       4,     5,     6,     1,     2,     0,     2,     4,     4,     8,
       1,     1,     3,     3,     0,     9,     0,     7,     1,     3,
       1,     3,     1,     3,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       0,    59,    10,     8,   324,   318,     0,     2,    62,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    60,    11,
       0,   335,     0,   325,   328,     0,   319,   320,     0,     0,
       0,     0,     0,    79,     0,    80,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   198,   199,     0,     0,    82,
       0,     0,     0,   109,     0,    72,    61,    64,    70,     0,
      63,    66,    67,    68,    69,    65,    71,     0,    16,     0,
       0,     0,     0,    17,     0,     0,     0,    19,    46,     0,
       0,     0,     0,     0,     0,    51,    54,     0,     0,     0,
     341,   352,   340,   348,   350,     0,     0,   335,   329,   348,
     350,     0,     0,   321,   203,   164,   163,   162,   161,   160,
     159,   158,   157,   203,   106,   307,     0,     0,     0,     0,
       7,    85,   175,     0,     0,     0,     0,     0,     0,   197,
     200,     0,     0,     0,     0,     0,     0,    54,   166,   165,
     108,     0,     0,    40,     0,   231,   246,     0,     0,     0,
       0,     0,     0,     0,     0,   232,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    14,     0,    49,    31,    47,    32,    18,    33,
      23,     0,    36,     0,    37,    52,    38,    39,     0,    42,
      12,     9,     0,     0,     0,     0,   336,     0,     0,   323,
     167,     0,   168,     0,     0,    89,    90,     0,     0,    62,
     178,     0,     0,   172,   177,     0,     0,     0,     0,     0,
     192,   194,   172,   172,   200,     0,    91,    94,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    13,
       0,     0,   209,   205,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   208,   210,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    25,     0,
       0,    45,     0,     0,     0,    22,     0,     0,    56,    55,
     346,     0,     0,   330,   343,   353,   342,   349,   351,     0,
     322,   204,   264,   103,     0,   270,   276,   105,   104,   309,
     306,   308,     0,    76,    78,   326,   184,   180,   173,   171,
       0,    73,    74,    84,   107,   190,   191,     0,   195,     0,
     200,   201,    87,    97,    93,    96,     0,     0,    81,     0,
      75,   203,   203,     0,    88,     0,    27,    28,    43,    29,
      30,   206,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   229,   228,   226,   225,   224,   219,   218,   222,
     223,   221,   220,   217,   216,   214,   215,   211,   212,   213,
      15,    26,    24,    50,    48,    44,    20,    21,    35,    34,
      53,    57,     0,     0,   337,   338,     0,   333,   331,     0,
     287,   279,     0,   287,     0,     0,    86,     0,     0,   175,
     176,   193,   196,   202,     0,   101,    92,    95,     0,    83,
       0,     0,     0,   327,    41,     0,   239,   245,     0,     0,
     243,     0,   230,   207,   234,   233,   235,   236,     0,     0,
     250,   251,   238,     0,   237,     0,    58,   354,   351,   344,
     334,   332,     0,     0,   287,     0,   255,   106,   294,     0,
     295,   277,   312,   313,     0,   188,     0,     0,   186,     0,
       0,     0,     0,    99,   169,   170,     0,     0,     0,     0,
       0,     0,     0,     0,   227,   355,     0,     0,     0,   281,
     282,   283,   284,   285,   288,     0,     0,     0,     0,   290,
       0,   257,     0,   293,   296,   255,     0,   316,     0,   310,
       0,   189,   185,   187,     0,   172,   181,    98,     0,     0,
     110,   240,   241,   242,   244,   247,   248,   249,   347,     0,
     354,   286,     0,   289,     0,     0,   259,   280,   259,   106,
       0,   313,     0,     0,    77,   203,     0,   102,     0,   339,
       0,   287,     0,     0,     0,   265,   271,     0,     0,   314,
       0,   311,   182,     0,   179,   100,   345,     0,     0,   254,
       0,     0,   263,     0,   278,   317,   313,   203,     0,   291,
     256,     0,   260,   261,   262,     0,   272,   315,   183,     0,
     258,   266,   301,   287,   147,     0,     0,   127,   113,   112,
     149,   150,   151,   152,   153,     0,     0,     0,     0,   134,
     136,   141,     0,     0,   135,     0,   114,     0,   130,   138,
     146,   148,     0,     0,     0,   302,   273,   292,     0,     0,
     143,   203,     0,   131,     0,   111,     0,   126,   172,     0,
     145,   267,   203,   133,     0,   298,     0,     0,     0,     0,
       0,     0,     0,     0,   147,     0,   154,     0,     0,   124,
       0,     0,   128,     0,   172,   298,     0,   147,     0,   253,
       0,     0,   137,     0,   116,     0,     0,   117,     0,   140,
       0,   111,     0,     0,   123,   125,   129,   253,   139,     0,
     297,     0,   299,     0,     0,     0,     0,     0,   144,     0,
     132,   115,   299,   303,     0,   156,     0,     0,     0,     0,
       0,     0,   156,   299,   252,   203,     0,   274,   119,   118,
       0,   120,   121,     0,   268,   156,   155,   300,   172,   122,
     142,   172,   304,   275,   269,   172,   305
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     6,   121,    11,    12,     9,    10,    19,    88,   239,
     178,   177,   175,   186,   187,   188,   299,     7,     8,    18,
      56,   132,   209,   229,   434,   539,   492,    57,   203,   317,
     136,   636,   637,   680,   658,   638,   639,   678,   640,   652,
     674,   641,   642,   643,   675,   737,   113,   140,    59,   683,
      60,   212,   213,   214,   326,   429,   535,   584,   428,   487,
     488,    61,    62,   222,    63,   223,    64,   225,   676,   201,
     244,   712,   521,   556,   575,   605,   318,   420,   592,   614,
     685,   751,   422,   593,   612,   665,   748,   423,   526,   477,
     515,   475,   476,   480,   525,   689,   725,   615,   664,   733,
     755,    65,   204,   321,   424,   563,   483,   529,   561,    15,
      16,    26,    27,   101,    13,    14,    66,    67,    23,    24,
     419,    95,    96,   508,   413,   506
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -701
static const short int yypact[] =
{
     209,  -701,  -701,  -701,  -701,  -701,    41,  -701,  -701,  -701,
    -701,  -701,    61,  -701,   -26,  -701,    21,  -701,   789,  1540,
      65,   106,    58,   -26,  -701,   112,    21,  -701,   558,    74,
      92,   247,    50,  -701,   114,  -701,   177,   136,   179,   190,
     194,   221,   239,   255,   278,  -701,  -701,   283,   296,  -701,
     298,   302,   309,  -701,   312,  -701,  -701,  -701,  -701,   143,
    -701,  -701,  -701,  -701,  -701,  -701,  -701,   121,  -701,   323,
     177,   353,   623,  -701,   354,   356,   360,  -701,  -701,   361,
     364,   366,   623,   367,   369,   371,  -701,   374,   274,   623,
    -701,   380,  -701,   372,   373,   333,   251,   106,  -701,  -701,
    -701,   337,   258,  -701,  -701,  -701,  -701,  -701,  -701,  -701,
    -701,  -701,  -701,  -701,  -701,  -701,   396,   397,   399,   400,
    -701,  -701,    43,   401,   177,   177,   402,   177,    22,  -701,
     405,    20,   375,   177,   406,   407,   381,  -701,  -701,  -701,
    -701,   365,    32,  -701,    45,  -701,  -701,   623,   623,   623,
     383,   385,   386,   388,   389,  -701,   390,   394,   395,   404,
     408,   409,   410,   414,   415,   423,   425,   427,   428,   430,
     623,   623,  1335,   222,  -701,   294,  -701,   295,    39,  -701,
    -701,   377,  1562,   297,  -701,  -701,   299,  -701,   413,  -701,
    -701,  1562,   418,   112,   112,   338,   214,   382,   339,   214,
    -701,   623,  -701,   262,    33,  -701,  -701,     0,   301,  -701,
    -701,   177,   420,    81,  -701,   340,   343,   344,   345,   346,
    -701,  -701,   105,   116,    40,   350,  -701,  -701,   443,    31,
      20,   352,   431,   486,   623,   357,   -26,   623,   623,  -701,
     623,   623,  -701,  -701,   798,   623,   623,   623,   623,   623,
     490,   495,   623,   496,   500,   501,   502,   623,   623,   503,
     504,   623,   623,   509,  -701,  -701,   623,   623,   623,   623,
     623,   623,   623,   623,   623,   623,   623,   623,   623,   623,
     623,   623,   623,   623,   623,   623,   623,   623,  1562,   511,
     513,  -701,   514,   623,   623,  1562,   351,   516,  -701,    84,
    -701,   387,   391,  -701,  -701,   524,  -701,  -701,  -701,    98,
    -701,  1562,   558,  -701,   177,  -701,  -701,  -701,  -701,  -701,
    -701,  -701,   525,  -701,  -701,   861,   497,  -701,  -701,  -701,
      43,  -701,  -701,  -701,  -701,  -701,  -701,   177,  -701,   177,
     405,  -701,  -701,  -701,  -701,  -701,   499,    23,  -701,   115,
    -701,  -701,  -701,  1360,  -701,     6,  1562,  1562,  1563,  1562,
    1562,  -701,   754,   985,  1380,  1400,  1005,   393,   392,  1025,
     398,   416,   421,   435,  1420,  1458,   436,   438,  1045,  1483,
     442,  1697,  1587,  1438,  1732,   965,  1523,   950,   950,   557,
     557,   557,   557,   276,   276,   230,   230,  -701,  -701,  -701,
    1562,  1562,  1562,  -701,  -701,  -701,  1562,  1562,  -701,  -701,
    -701,  -701,   533,   112,   268,   214,   484,  -701,  -701,   103,
     472,  -701,   543,   472,   623,   417,  -701,     3,   526,    43,
    -701,  -701,  -701,  -701,    20,  -701,  -701,  -701,   518,  -701,
     451,   452,   595,  -701,  -701,   623,  -701,  -701,   623,   623,
    -701,   623,  -701,  -701,  -701,  -701,  -701,  -701,   623,   623,
    -701,  -701,  -701,   596,  -701,   623,  -701,   462,   588,  -701,
    -701,  -701,    15,   570,  1674,   592,   512,  -701,  -701,  1717,
     523,  -701,  1562,    30,   608,  -701,   609,     2,  -701,   529,
     579,   130,    20,  -701,  -701,  -701,   492,  1065,  1085,  1110,
    1130,  1150,  1170,   506,  1562,   214,   574,   112,   112,  -701,
    -701,  -701,  -701,  -701,  -701,   507,   623,   403,   620,  -701,
     600,   605,   349,  -701,  -701,   512,   582,   613,   614,  -701,
     515,  -701,  -701,  -701,   651,   519,  -701,  -701,   140,    20,
    -701,  -701,  -701,  -701,  -701,  -701,  -701,  -701,  -701,   527,
     462,  -701,  1190,  -701,   623,   622,   564,  -701,   564,  -701,
     623,    30,   623,   528,  -701,  -701,   575,  -701,   145,   214,
     612,   277,  1210,   623,   633,  -701,  -701,   571,  1235,  -701,
    1255,  -701,  -701,   667,  -701,  -701,  -701,   641,   663,  -701,
    1275,   623,   156,   626,  -701,  -701,    30,  -701,   623,  -701,
    -701,  1295,  -701,  -701,  -701,   628,  -701,  -701,  -701,  1315,
    -701,  -701,  -701,   648,   702,    46,   673,   712,  -701,  -701,
    -701,  -701,  -701,  -701,  -701,   653,   654,   177,   655,  -701,
    -701,  -701,   656,   658,  -701,   233,  -701,   659,  -701,  -701,
    -701,   702,   640,   661,   143,  -701,  -701,  -701,    57,   290,
    -701,  -701,   670,  -701,   122,  -701,   671,  -701,    48,   233,
    -701,  -701,  -701,  -701,   657,   684,   674,   678,   569,   679,
     586,   691,   693,   590,   702,   591,  -701,   623,    25,  -701,
      16,   263,  -701,   233,   182,   684,   597,   702,   729,   638,
     122,   122,  -701,   122,  -701,   122,   122,  -701,   701,  -701,
    1503,   598,   599,   122,  -701,  -701,  -701,   638,  -701,   685,
    -701,   716,  -701,   607,   610,    17,   611,   617,  -701,   746,
    -701,  -701,  -701,  -701,   749,    54,   618,   621,   122,   624,
     629,   649,    54,  -701,  -701,  -701,   750,  -701,  -701,  -701,
     650,  -701,  -701,   143,  -701,    54,  -701,  -701,   519,  -701,
    -701,   519,  -701,  -701,  -701,   519,  -701
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -701,  -701,   -69,  -701,  -701,  -701,  -701,   522,  -701,  -701,
    -701,  -701,  -701,  -701,   627,  -701,  -701,  -701,  -701,   546,
    -701,  -701,  -701,  -215,  -701,  -701,  -701,  -701,  -454,   -13,
    -701,   -51,  -495,    70,   137,   111,  -701,  -701,   154,  -701,
    -701,  -701,  -556,  -701,    62,  -609,  -701,  -635,  -572,  -210,
    -701,   370,  -701,   470,  -701,  -701,  -701,  -701,  -701,  -701,
     314,  -701,  -701,  -701,  -701,  -701,  -701,  -216,  -102,  -701,
     -72,    97,   280,  -701,   248,  -701,  -701,  -701,  -701,  -701,
    -701,  -701,  -701,  -701,  -701,  -701,  -701,  -701,  -701,  -701,
    -701,  -458,   384,  -701,  -701,   123,  -700,  -701,  -701,  -701,
    -701,  -701,  -701,  -701,  -701,  -701,  -701,  -522,  -701,  -701,
    -701,  -701,   783,  -701,  -701,  -701,  -701,  -701,   576,   -19,
    -701,   713,   -11,  -701,  -701,   261
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -327
static const short int yytable[] =
{
     172,   143,   200,   330,    98,    58,   485,   485,   341,   663,
     182,   202,   337,   339,   102,   349,   518,   191,   145,   146,
     655,   655,   732,   522,   226,   227,   120,   436,   437,   701,
      21,   618,   618,   745,   527,   344,   345,   319,   237,   579,
     618,    17,   644,   291,   224,   147,   148,   210,   619,   619,
     645,   240,   149,   150,   151,   216,   217,   619,   219,   221,
     735,   655,    21,   443,   231,    20,   152,   153,   154,   644,
     736,    89,   618,   155,   607,   242,   243,    25,   156,   656,
     626,   509,   510,   511,   512,   513,   157,   118,   411,   619,
     320,   158,   159,   160,   161,   162,   163,   164,   264,   265,
      22,   288,   644,   646,   165,   577,   166,   211,   750,   295,
      90,   666,   667,   588,    97,   644,    90,  -174,   698,   344,
     345,   167,   228,   744,   433,   438,   655,   168,   528,   311,
     114,   709,    22,   346,   344,   345,   752,   618,  -174,   322,
     657,   323,   327,   532,   344,   345,   486,   486,   115,   344,
     345,   119,   169,   668,   619,   616,   514,   703,   728,   170,
     171,   633,   353,   220,   657,   356,   357,   635,   359,   360,
     347,   238,   348,   362,   363,   364,   365,   366,   292,   340,
     369,   120,   301,   302,   241,   374,   375,   329,   705,   378,
     379,   682,   122,   669,   381,   382,   383,   384,   385,   386,
     387,   388,   389,   390,   391,   392,   393,   394,   395,   396,
     397,   398,   399,   400,   401,   402,   123,   346,   304,   491,
     329,   406,   407,   412,   417,   145,   146,   124,   286,   470,
      91,   125,   346,    92,    93,    94,    91,   655,   418,    92,
      99,   100,   346,   471,   329,   421,   336,   346,   618,   440,
     441,   141,   147,   148,   347,   329,   439,   338,   126,   149,
     150,   151,   282,   283,   284,   619,   312,   655,   431,   347,
     432,   537,   304,   152,   153,   154,   127,   538,   618,   347,
     155,   567,   138,   139,   347,   156,   585,   656,   626,   602,
     603,   604,   128,   157,   655,   619,   116,   117,   158,   159,
     160,   161,   162,   163,   164,   618,   280,   281,   282,   283,
     284,   165,    58,   166,   517,   129,   587,   666,   667,   313,
     130,   329,   619,   706,   568,   566,   314,   142,   167,     1,
       2,     3,   315,   131,   168,   133,    98,    41,   305,   134,
       4,   306,   307,   308,   671,   672,   135,     5,   474,   137,
     479,   474,   482,   312,   408,   409,   316,   144,   173,   169,
     174,   287,    51,    52,   176,   179,   170,   171,   180,   633,
     181,   183,   184,   497,    53,   185,   498,   499,   189,   500,
     145,   146,   190,   293,   192,   557,   501,   502,   193,   194,
     195,   196,   305,   504,   198,   306,   307,   468,   199,   669,
     205,   206,   467,   207,   208,   215,   218,   147,   148,   224,
     232,   233,   230,   314,   149,   150,   151,   298,   234,   315,
     245,   236,   246,   247,    41,   248,   249,   250,   152,   153,
     154,   251,   252,   289,   290,   155,   296,   351,   297,   309,
     156,   253,   324,   316,   552,   254,   255,   256,   157,    51,
      52,   257,   258,   158,   159,   160,   161,   162,   163,   164,
     259,    53,   260,   582,   261,   262,   165,   263,   166,   509,
     510,   511,   512,   513,   300,   145,   146,   328,   303,   310,
     343,   331,   572,   167,   332,   333,   334,   335,   578,   168,
     580,   342,   352,   350,   367,   608,   549,   550,   354,   368,
     370,   590,   147,   148,   371,   372,   373,   376,   377,   472,
     150,   151,   473,   380,   169,   403,   294,   404,   405,   601,
     410,   170,   171,   152,   153,   154,   609,   414,   416,   425,
     155,   415,   451,   452,   427,   156,   435,   466,   753,   454,
     469,   754,   489,   157,   514,   756,   145,   146,   158,   159,
     160,   161,   162,   163,   164,   493,   484,   455,   650,   478,
     686,   165,   456,   166,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   147,   148,   312,   457,   460,   167,   461,
     149,   150,   151,   464,   168,   278,   279,   280,   281,   282,
     283,   284,   494,   495,   152,   153,   154,   670,   673,   496,
     503,   155,   505,   679,   507,   700,   156,   516,   519,   169,
     520,   524,   530,   531,   157,   536,   170,   171,   534,   158,
     159,   160,   161,   162,   163,   164,   145,   146,   594,   704,
     670,   548,   165,   540,   166,   314,   553,   554,   559,   713,
     714,   315,   679,   555,   716,   717,    41,   547,   551,   167,
     560,   562,   721,   147,   148,   168,   564,   565,   329,   573,
     149,   150,   151,   574,   704,   316,   583,   569,   581,   586,
     591,    51,    52,   597,   152,   153,   154,   740,   598,   599,
     169,   155,   606,    53,   611,   517,   156,   170,   171,   647,
     648,   649,   651,   653,   157,   654,   659,   661,   662,   158,
     159,   160,   161,   162,   163,   164,   617,   677,   681,   688,
     692,   690,   165,   687,   166,   691,   693,   618,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   694,   695,   167,
     696,   697,   699,   710,   619,   168,   711,   718,   708,  -127,
     720,   724,   723,   620,   621,   622,   623,   624,   726,  -111,
     731,   727,   729,   734,   747,   325,   625,   626,   730,   738,
     169,   358,   739,   715,   235,   741,   627,   170,   171,   266,
     742,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   702,
     743,   749,   628,    28,   629,   660,   684,   746,   630,   490,
     430,   533,    51,    52,   722,   558,   576,   481,   707,   103,
     197,   570,   355,   266,   631,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,     0,   632,    29,    30,    31,   633,     0,
       0,     0,   634,     0,   635,     0,     0,     0,    32,    33,
      34,    35,     0,    36,    37,     0,     0,     0,     0,     0,
       0,    38,    39,    40,    41,    28,     0,     0,     0,     0,
       0,     0,    42,    43,    44,    45,    46,    47,     0,     0,
       0,     0,    48,    49,    50,     0,     0,     0,     0,    51,
      52,     0,     0,   445,     0,   446,     0,   426,     0,     0,
       0,    53,     0,     0,     0,     0,     0,    29,    30,    31,
       0,     0,     0,    54,     0,     0,     0,     0,     0,  -326,
      32,    33,    34,    35,     0,    36,    37,     0,     0,    55,
       0,     0,     0,    38,    39,    40,    41,     0,     0,   361,
       0,     0,     0,     0,    42,    43,    44,    45,    46,    47,
       0,     0,     0,     0,    48,    49,    50,     0,     0,     0,
       0,    51,    52,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    53,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,    54,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,    55,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,     0,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,     0,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,     0,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,     0,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,     0,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
       0,     0,     0,     0,     0,   266,   447,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   266,   450,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   266,   453,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   266,   462,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   266,   541,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   266,   542,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,     0,     0,     0,     0,     0,
     266,   543,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,   544,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,   545,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,   546,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,   571,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     266,   589,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
       0,     0,     0,     0,     0,   266,   595,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   266,   596,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   266,   600,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   266,   610,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,     0,   613,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   266,   285,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,     0,     0,     0,     0,     0,   266,   442,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   266,   448,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,     0,   449,
       0,     0,     0,     0,    68,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,     0,   458,
       0,     0,     0,     0,     0,     0,     0,    68,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   266,    69,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   459,     0,   444,
       0,    69,     0,     0,    70,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   463,     0,     0,     0,     0,    70,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    71,     0,     0,
       0,     0,   719,    72,    73,    74,    75,    76,   -43,    77,
      78,    79,     0,    80,    81,     0,    82,    83,    84,     0,
      71,     0,     0,    85,    86,    87,    72,    73,    74,    75,
      76,     0,    77,    78,    79,     0,    80,    81,     0,    82,
      83,    84,     0,     0,     0,     0,    85,    86,    87,   266,
       0,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,     0,
       0,   517,   266,   465,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   266,   523,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284
};

static const short int yycheck[] =
{
      72,    70,   104,   213,    23,    18,     4,     4,   224,   644,
      82,   113,   222,   223,    25,   230,   474,    89,     3,     4,
       4,     4,   722,   477,     4,     5,     4,     4,     5,     4,
      56,    15,    15,   733,     4,     4,     5,     4,     6,   561,
      15,     0,   614,     4,     4,    30,    31,     4,    32,    32,
       4,     6,    37,    38,    39,   124,   125,    32,   127,   128,
       6,     4,    56,    57,   133,     4,    51,    52,    53,   641,
      16,     6,    15,    58,   596,   147,   148,    56,    63,    54,
      55,    66,    67,    68,    69,    70,    71,    37,     4,    32,
      57,    76,    77,    78,    79,    80,    81,    82,   170,   171,
     126,   173,   674,    57,    89,   559,    91,    64,   743,   181,
       4,    54,    55,   571,    56,   687,     4,    36,   674,     4,
       5,   106,   102,   732,   340,   102,     4,   112,    98,   201,
      56,   687,   126,   102,     4,     5,   745,    15,    57,   139,
     635,   141,   211,   141,     4,     5,   144,   144,    56,     4,
       5,    37,   137,    96,    32,   613,   141,   141,   141,   144,
     145,   136,   234,   141,   659,   237,   238,   142,   240,   241,
     139,   139,   141,   245,   246,   247,   248,   249,   139,   139,
     252,     4,   193,   194,   139,   257,   258,   139,   683,   261,
     262,   143,    56,   136,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,    37,   102,     4,   434,
     139,   293,   294,   139,   126,     3,     4,    37,     6,   126,
     124,    37,   102,   127,   128,   129,   124,     4,   140,   127,
     128,   129,   102,   140,   139,   314,   141,   102,    15,   351,
     352,   130,    30,    31,   139,   139,   141,   141,    37,    37,
      38,    39,    32,    33,    34,    32,     4,     4,   337,   139,
     339,   141,     4,    51,    52,    53,    37,   492,    15,   139,
      58,   141,   139,   140,   139,    63,   141,    54,    55,   133,
     134,   135,    37,    71,     4,    32,    49,    50,    76,    77,
      78,    79,    80,    81,    82,    15,    30,    31,    32,    33,
      34,    89,   325,    91,    37,    37,    39,    54,    55,    57,
      37,   139,    32,   141,   539,   535,    64,     4,   106,   120,
     121,   122,    70,    37,   112,    37,   355,    75,   124,    37,
     131,   127,   128,   129,    54,    55,    37,   138,   420,    37,
     422,   423,   424,     4,     3,     4,    94,     4,     4,   137,
       4,   139,   100,   101,     4,     4,   144,   145,     4,   136,
       4,     4,     3,   445,   112,     4,   448,   449,     4,   451,
       3,     4,   108,     6,     4,    36,   458,   459,    16,    16,
      57,   140,   124,   465,    57,   127,   128,   129,   140,   136,
       4,     4,   413,     4,     4,     4,     4,    30,    31,     4,
       4,     4,    37,    64,    37,    38,    39,     4,    37,    70,
      37,    56,    37,    37,    75,    37,    37,    37,    51,    52,
      53,    37,    37,   139,   139,    58,   139,     6,   139,    57,
      63,    37,   141,    94,   516,    37,    37,    37,    71,   100,
     101,    37,    37,    76,    77,    78,    79,    80,    81,    82,
      37,   112,    37,   565,    37,    37,    89,    37,    91,    66,
      67,    68,    69,    70,    56,     3,     4,    57,   140,   140,
      37,   141,   554,   106,   141,   141,   141,   141,   560,   112,
     562,   141,     6,   141,     4,   597,   507,   508,   141,     4,
       4,   573,    30,    31,     4,     4,     4,     4,     4,    37,
      38,    39,    40,     4,   137,     4,   139,     4,     4,   591,
       4,   144,   145,    51,    52,    53,   598,   140,     4,     4,
      58,   140,   139,   141,    37,    63,    37,     4,   748,   141,
      56,   751,    16,    71,   141,   755,     3,     4,    76,    77,
      78,    79,    80,    81,    82,    37,   139,   141,   627,    16,
     662,    89,   141,    91,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    30,    31,     4,   141,   141,   106,   141,
      37,    38,    39,   141,   112,    28,    29,    30,    31,    32,
      33,    34,   141,   141,    51,    52,    53,   648,   649,     4,
       4,    58,   140,   654,    16,   677,    63,    37,    16,   137,
      98,    88,     4,     4,    71,    36,   144,   145,    89,    76,
      77,    78,    79,    80,    81,    82,     3,     4,    57,   680,
     681,    57,    89,   141,    91,    64,    16,    37,    56,   690,
     691,    70,   693,    38,   695,   696,    75,   141,   141,   106,
      37,    37,   703,    30,    31,   112,   141,     6,   139,    37,
      37,    38,    39,    99,   715,    94,    91,   140,   140,    57,
      37,   100,   101,     6,    51,    52,    53,   728,    37,    16,
     137,    58,    56,   112,    56,    37,    63,   144,   145,    16,
      37,    37,    37,    37,    71,    37,    37,    57,    37,    76,
      77,    78,    79,    80,    81,    82,     4,    37,    37,    25,
     141,    37,    89,    56,    91,    37,    37,    15,     6,     7,
       8,     9,    10,    11,    12,    13,    14,   141,    37,   106,
      37,   141,   141,     4,    32,   112,    98,    36,   141,   141,
     141,    25,    57,    41,    42,    43,    44,    45,   141,    37,
       4,   141,   141,     4,     4,   209,    54,    55,   141,   141,
     137,   239,   141,   693,   137,   141,    64,   144,   145,    15,
     141,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,   678,
     141,   141,    90,     4,    92,   641,   659,   735,    96,   429,
     330,   487,   100,   101,   707,   525,   558,   423,   685,    26,
      97,   550,   236,    15,   112,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,   132,    46,    47,    48,   136,    -1,
      -1,    -1,   140,    -1,   142,    -1,    -1,    -1,    59,    60,
      61,    62,    -1,    64,    65,    -1,    -1,    -1,    -1,    -1,
      -1,    72,    73,    74,    75,     4,    -1,    -1,    -1,    -1,
      -1,    -1,    83,    84,    85,    86,    87,    88,    -1,    -1,
      -1,    -1,    93,    94,    95,    -1,    -1,    -1,    -1,   100,
     101,    -1,    -1,   139,    -1,   141,    -1,    36,    -1,    -1,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    46,    47,    48,
      -1,    -1,    -1,   124,    -1,    -1,    -1,    -1,    -1,   130,
      59,    60,    61,    62,    -1,    64,    65,    -1,    -1,   140,
      -1,    -1,    -1,    72,    73,    74,    75,    -1,    -1,   141,
      -1,    -1,    -1,    -1,    83,    84,    85,    86,    87,    88,
      -1,    -1,    -1,    -1,    93,    94,    95,    -1,    -1,    -1,
      -1,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   112,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,   124,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   140,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,    -1,
      15,   141,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   141,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   141,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   141,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   141,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   141,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   141,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,   141,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   139,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    -1,    15,   139,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   139,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,   139,
      -1,    -1,    -1,    -1,     4,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,   139,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,    38,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,   139,    -1,    36,
      -1,    38,    -1,    -1,    64,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,   139,    -1,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    97,    -1,    -1,
      -1,    -1,   139,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    -1,   113,   114,    -1,   116,   117,   118,    -1,
      97,    -1,    -1,   123,   124,   125,   103,   104,   105,   106,
     107,    -1,   109,   110,   111,    -1,   113,   114,    -1,   116,
     117,   118,    -1,    -1,    -1,    -1,   123,   124,   125,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      -1,    37,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short int yystos[] =
{
       0,   120,   121,   122,   131,   138,   147,   163,   164,   151,
     152,   149,   150,   260,   261,   255,   256,     0,   165,   153,
       4,    56,   126,   264,   265,    56,   257,   258,     4,    46,
      47,    48,    59,    60,    61,    62,    64,    65,    72,    73,
      74,    75,    83,    84,    85,    86,    87,    88,    93,    94,
      95,   100,   101,   112,   124,   140,   166,   173,   175,   194,
     196,   207,   208,   210,   212,   247,   262,   263,     4,    38,
      64,    97,   103,   104,   105,   106,   107,   109,   110,   111,
     113,   114,   116,   117,   118,   123,   124,   125,   154,     6,
       4,   124,   127,   128,   129,   267,   268,    56,   265,   128,
     129,   259,   268,   258,     6,     7,     8,     9,    10,    11,
      12,    13,    14,   192,    56,    56,    49,    50,    37,    37,
       4,   148,    56,    37,    37,    37,    37,    37,    37,    37,
      37,    37,   167,    37,    37,    37,   176,    37,   139,   140,
     193,   130,     4,   148,     4,     3,     4,    30,    31,    37,
      38,    39,    51,    52,    53,    58,    63,    71,    76,    77,
      78,    79,    80,    81,    82,    89,    91,   106,   112,   137,
     144,   145,   216,     4,     4,   158,     4,   157,   156,     4,
       4,     4,   216,     4,     3,     4,   159,   160,   161,     4,
     108,   216,     4,    16,    16,    57,   140,   267,    57,   140,
     214,   215,   214,   174,   248,     4,     4,     4,     4,   168,
       4,    64,   197,   198,   199,     4,   148,   148,     4,   148,
     141,   148,   209,   211,     4,   213,     4,     5,   102,   169,
      37,   148,     4,     4,    37,   160,    56,     6,   139,   155,
       6,   139,   216,   216,   216,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,   216,   216,    15,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,   139,     6,   139,   216,   139,
     139,     4,   139,     6,   139,   216,   139,   139,     4,   162,
      56,   268,   268,   140,     4,   124,   127,   128,   129,    57,
     140,   216,     4,    57,    64,    70,    94,   175,   222,     4,
      57,   249,   139,   141,   141,   165,   200,   148,    57,   139,
     195,   141,   141,   141,   141,   141,   141,   195,   141,   195,
     139,   213,   141,    37,     4,     5,   102,   139,   141,   169,
     141,     6,     6,   216,   141,   264,   216,   216,   153,   216,
     216,   141,   216,   216,   216,   216,   216,     4,     4,   216,
       4,     4,     4,     4,   216,   216,     4,     4,   216,   216,
       4,   216,   216,   216,   216,   216,   216,   216,   216,   216,
     216,   216,   216,   216,   216,   216,   216,   216,   216,   216,
     216,   216,   216,     4,     4,     4,   216,   216,     3,     4,
       4,     4,   139,   270,   140,   140,     4,   126,   140,   266,
     223,   148,   228,   233,   250,     4,    36,    37,   204,   201,
     199,   148,   148,   213,   170,    37,     4,     5,   102,   141,
     214,   214,   139,    57,    36,   139,   141,   141,   139,   139,
     141,   139,   141,   141,   141,   141,   141,   141,   139,   139,
     141,   141,   141,   139,   141,    16,     4,   268,   129,    56,
     126,   140,    37,    40,   216,   237,   238,   235,    16,   216,
     239,   238,   216,   252,   139,     4,   144,   205,   206,    16,
     197,   169,   172,    37,   141,   141,     4,   216,   216,   216,
     216,   216,   216,     4,   216,   140,   271,    16,   269,    66,
      67,    68,    69,    70,   141,   236,    37,    37,   237,    16,
      98,   218,   174,    16,    88,   240,   234,     4,    98,   253,
       4,     4,   141,   206,    89,   202,    36,   141,   169,   171,
     141,   141,   141,   141,   141,   141,   141,   141,    57,   268,
     268,   141,   216,    16,    37,    38,   219,    36,   218,    56,
      37,   254,    37,   251,   141,     6,   195,   141,   169,   140,
     271,   141,   216,    37,    99,   220,   220,   174,   216,   253,
     216,   140,   214,    91,   203,   141,    57,    39,   237,   141,
     216,    37,   224,   229,    57,   141,   141,     6,    37,    16,
     141,   216,   133,   134,   135,   221,    56,   253,   214,   216,
     141,    56,   230,   141,   225,   243,   237,     4,    15,    32,
      41,    42,    43,    44,    45,    54,    55,    64,    90,    92,
      96,   112,   132,   136,   140,   142,   177,   178,   181,   182,
     184,   187,   188,   189,   194,     4,    57,    16,    37,    37,
     148,    37,   185,    37,    37,     4,    54,   178,   180,    37,
     184,    57,    37,   193,   244,   231,    54,    55,    96,   136,
     177,    54,    55,   177,   186,   190,   214,    37,   183,   177,
     179,    37,   143,   195,   180,   226,   214,    56,    25,   241,
      37,    37,   141,    37,   141,    37,    37,   141,   188,   141,
     216,     4,   181,   141,   177,   178,   141,   241,   141,   188,
       4,    98,   217,   177,   177,   179,   177,   177,    36,   139,
     141,   177,   217,    57,    25,   242,   141,   141,   141,   141,
     141,     4,   242,   245,     4,     6,    16,   191,   141,   141,
     177,   141,   141,   141,   191,   242,   190,     4,   232,   141,
     193,   227,   191,   195,   195,   246,   195
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
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
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
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
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
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
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
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
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
      size_t yyn = 0;
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

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
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
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
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
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
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

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

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
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


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
	short int *yyss1 = yyss;
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

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

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

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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
#line 172 "ldgram.y"
    { ldlex_defsym(); }
    break;

  case 9:
#line 174 "ldgram.y"
    {
		  ldlex_popstate();
		  lang_add_assignment(exp_assop((yyvsp[-1].token),(yyvsp[-2].name),(yyvsp[0].etree)));
		}
    break;

  case 10:
#line 182 "ldgram.y"
    {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
    break;

  case 11:
#line 187 "ldgram.y"
    {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
    break;

  case 16:
#line 202 "ldgram.y"
    {
			einfo(_("%P%F: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[0].name));
			}
    break;

  case 17:
#line 205 "ldgram.y"
    {
			config.map_filename = "-";
			}
    break;

  case 20:
#line 211 "ldgram.y"
    { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
    break;

  case 21:
#line 213 "ldgram.y"
    { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
    break;

  case 22:
#line 215 "ldgram.y"
    { mri_public((yyvsp[-1].name), (yyvsp[0].etree)); }
    break;

  case 23:
#line 217 "ldgram.y"
    { mri_format((yyvsp[0].name)); }
    break;

  case 24:
#line 219 "ldgram.y"
    { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
    break;

  case 25:
#line 221 "ldgram.y"
    { mri_output_section((yyvsp[-1].name), (yyvsp[0].etree));}
    break;

  case 26:
#line 223 "ldgram.y"
    { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
    break;

  case 27:
#line 225 "ldgram.y"
    { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
    break;

  case 28:
#line 227 "ldgram.y"
    { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
    break;

  case 29:
#line 229 "ldgram.y"
    { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
    break;

  case 30:
#line 231 "ldgram.y"
    { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
    break;

  case 33:
#line 235 "ldgram.y"
    { mri_name((yyvsp[0].name)); }
    break;

  case 34:
#line 237 "ldgram.y"
    { mri_alias((yyvsp[-2].name),(yyvsp[0].name),0);}
    break;

  case 35:
#line 239 "ldgram.y"
    { mri_alias ((yyvsp[-2].name), 0, (int) (yyvsp[0].bigint).integer); }
    break;

  case 36:
#line 241 "ldgram.y"
    { mri_base((yyvsp[0].etree)); }
    break;

  case 37:
#line 243 "ldgram.y"
    { mri_truncate ((unsigned int) (yyvsp[0].bigint).integer); }
    break;

  case 40:
#line 247 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
    break;

  case 41:
#line 249 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 42:
#line 251 "ldgram.y"
    { lang_add_entry ((yyvsp[0].name), FALSE); }
    break;

  case 44:
#line 256 "ldgram.y"
    { mri_order((yyvsp[0].name)); }
    break;

  case 45:
#line 257 "ldgram.y"
    { mri_order((yyvsp[0].name)); }
    break;

  case 47:
#line 263 "ldgram.y"
    { mri_load((yyvsp[0].name)); }
    break;

  case 48:
#line 264 "ldgram.y"
    { mri_load((yyvsp[0].name)); }
    break;

  case 49:
#line 269 "ldgram.y"
    { mri_only_load((yyvsp[0].name)); }
    break;

  case 50:
#line 271 "ldgram.y"
    { mri_only_load((yyvsp[0].name)); }
    break;

  case 51:
#line 275 "ldgram.y"
    { (yyval.name) = NULL; }
    break;

  case 54:
#line 282 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 55:
#line 284 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 56:
#line 288 "ldgram.y"
    { ldlang_add_undef ((yyvsp[0].name)); }
    break;

  case 57:
#line 290 "ldgram.y"
    { ldlang_add_undef ((yyvsp[0].name)); }
    break;

  case 58:
#line 292 "ldgram.y"
    { ldlang_add_undef ((yyvsp[0].name)); }
    break;

  case 59:
#line 296 "ldgram.y"
    { ldlex_both(); }
    break;

  case 60:
#line 298 "ldgram.y"
    { ldlex_popstate(); }
    break;

  case 73:
#line 319 "ldgram.y"
    { lang_add_target((yyvsp[-1].name)); }
    break;

  case 74:
#line 321 "ldgram.y"
    { ldfile_add_library_path ((yyvsp[-1].name), FALSE); }
    break;

  case 75:
#line 323 "ldgram.y"
    { lang_add_output((yyvsp[-1].name), 1); }
    break;

  case 76:
#line 325 "ldgram.y"
    { lang_add_output_format ((yyvsp[-1].name), (char *) NULL,
					    (char *) NULL, 1); }
    break;

  case 77:
#line 328 "ldgram.y"
    { lang_add_output_format ((yyvsp[-5].name), (yyvsp[-3].name), (yyvsp[-1].name), 1); }
    break;

  case 78:
#line 330 "ldgram.y"
    { ldfile_set_output_arch ((yyvsp[-1].name), bfd_arch_unknown); }
    break;

  case 79:
#line 332 "ldgram.y"
    { command_line.force_common_definition = TRUE ; }
    break;

  case 80:
#line 334 "ldgram.y"
    { command_line.inhibit_common_definition = TRUE ; }
    break;

  case 82:
#line 337 "ldgram.y"
    { lang_enter_group (); }
    break;

  case 83:
#line 339 "ldgram.y"
    { lang_leave_group (); }
    break;

  case 84:
#line 341 "ldgram.y"
    { lang_add_map((yyvsp[-1].name)); }
    break;

  case 85:
#line 343 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
    break;

  case 86:
#line 345 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 87:
#line 347 "ldgram.y"
    {
		  lang_add_nocrossref ((yyvsp[-1].nocrossref));
		}
    break;

  case 89:
#line 352 "ldgram.y"
    { lang_add_insert ((yyvsp[0].name), 0); }
    break;

  case 90:
#line 354 "ldgram.y"
    { lang_add_insert ((yyvsp[0].name), 1); }
    break;

  case 91:
#line 359 "ldgram.y"
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 92:
#line 362 "ldgram.y"
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 93:
#line 365 "ldgram.y"
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 94:
#line 368 "ldgram.y"
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 95:
#line 371 "ldgram.y"
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 96:
#line 374 "ldgram.y"
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 97:
#line 377 "ldgram.y"
    { (yyval.integer) = as_needed; as_needed = TRUE; }
    break;

  case 98:
#line 379 "ldgram.y"
    { as_needed = (yyvsp[-2].integer); }
    break;

  case 99:
#line 381 "ldgram.y"
    { (yyval.integer) = as_needed; as_needed = TRUE; }
    break;

  case 100:
#line 383 "ldgram.y"
    { as_needed = (yyvsp[-2].integer); }
    break;

  case 101:
#line 385 "ldgram.y"
    { (yyval.integer) = as_needed; as_needed = TRUE; }
    break;

  case 102:
#line 387 "ldgram.y"
    { as_needed = (yyvsp[-2].integer); }
    break;

  case 107:
#line 402 "ldgram.y"
    { lang_add_entry ((yyvsp[-1].name), FALSE); }
    break;

  case 109:
#line 404 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 110:
#line 405 "ldgram.y"
    { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name))); }
    break;

  case 111:
#line 413 "ldgram.y"
    {
			  (yyval.cname) = (yyvsp[0].name);
			}
    break;

  case 112:
#line 417 "ldgram.y"
    {
			  (yyval.cname) = "*";
			}
    break;

  case 113:
#line 421 "ldgram.y"
    {
			  (yyval.cname) = "?";
			}
    break;

  case 114:
#line 428 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 115:
#line 434 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-2].name_list);
			}
    break;

  case 116:
#line 440 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 117:
#line 446 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 118:
#line 452 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_name_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 119:
#line 458 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 120:
#line 464 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_alignment_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 121:
#line 470 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 122:
#line 476 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-3].name_list);
			}
    break;

  case 123:
#line 485 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = (yyvsp[-1].name_list);
			  (yyval.name_list) = tmp;
			}
    break;

  case 124:
#line 494 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
    break;

  case 125:
#line 505 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[-2].wildcard_list);
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 126:
#line 514 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 127:
#line 525 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
    break;

  case 128:
#line 533 "ldgram.y"
    {
			  lang_add_wild (NULL, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
    break;

  case 129:
#line 537 "ldgram.y"
    {
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
    break;

  case 131:
#line 545 "ldgram.y"
    { ldgram_had_keep = TRUE; }
    break;

  case 132:
#line 547 "ldgram.y"
    { ldgram_had_keep = FALSE; }
    break;

  case 134:
#line 553 "ldgram.y"
    {
 		lang_add_attribute(lang_object_symbols_statement_enum);
	      	}
    break;

  case 136:
#line 558 "ldgram.y"
    {

		  lang_add_attribute(lang_constructors_statement_enum);
		}
    break;

  case 137:
#line 563 "ldgram.y"
    {
		  constructors_sorted = TRUE;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
    break;

  case 139:
#line 569 "ldgram.y"
    {
			  lang_add_data ((int) (yyvsp[-3].integer), (yyvsp[-1].etree));
			}
    break;

  case 140:
#line 574 "ldgram.y"
    {
			  lang_add_fill ((yyvsp[-1].fill));
			}
    break;

  case 141:
#line 577 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 142:
#line 578 "ldgram.y"
    { ldlex_popstate ();
			  lang_add_assignment (exp_assert ((yyvsp[-4].etree), (yyvsp[-2].name))); }
    break;

  case 143:
#line 581 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
    break;

  case 144:
#line 583 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 149:
#line 598 "ldgram.y"
    { (yyval.integer) = (yyvsp[0].token); }
    break;

  case 150:
#line 600 "ldgram.y"
    { (yyval.integer) = (yyvsp[0].token); }
    break;

  case 151:
#line 602 "ldgram.y"
    { (yyval.integer) = (yyvsp[0].token); }
    break;

  case 152:
#line 604 "ldgram.y"
    { (yyval.integer) = (yyvsp[0].token); }
    break;

  case 153:
#line 606 "ldgram.y"
    { (yyval.integer) = (yyvsp[0].token); }
    break;

  case 154:
#line 611 "ldgram.y"
    {
		  (yyval.fill) = exp_get_fill ((yyvsp[0].etree), 0, "fill value");
		}
    break;

  case 155:
#line 618 "ldgram.y"
    { (yyval.fill) = (yyvsp[0].fill); }
    break;

  case 156:
#line 619 "ldgram.y"
    { (yyval.fill) = (fill_type *) 0; }
    break;

  case 157:
#line 624 "ldgram.y"
    { (yyval.token) = '+'; }
    break;

  case 158:
#line 626 "ldgram.y"
    { (yyval.token) = '-'; }
    break;

  case 159:
#line 628 "ldgram.y"
    { (yyval.token) = '*'; }
    break;

  case 160:
#line 630 "ldgram.y"
    { (yyval.token) = '/'; }
    break;

  case 161:
#line 632 "ldgram.y"
    { (yyval.token) = LSHIFT; }
    break;

  case 162:
#line 634 "ldgram.y"
    { (yyval.token) = RSHIFT; }
    break;

  case 163:
#line 636 "ldgram.y"
    { (yyval.token) = '&'; }
    break;

  case 164:
#line 638 "ldgram.y"
    { (yyval.token) = '|'; }
    break;

  case 167:
#line 648 "ldgram.y"
    {
		  lang_add_assignment (exp_assop ((yyvsp[-1].token), (yyvsp[-2].name), (yyvsp[0].etree)));
		}
    break;

  case 168:
#line 652 "ldgram.y"
    {
		  lang_add_assignment (exp_assop ('=', (yyvsp[-2].name),
						  exp_binop ((yyvsp[-1].token),
							     exp_nameop (NAME,
									 (yyvsp[-2].name)),
							     (yyvsp[0].etree))));
		}
    break;

  case 169:
#line 660 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), FALSE));
		}
    break;

  case 170:
#line 664 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), TRUE));
		}
    break;

  case 178:
#line 687 "ldgram.y"
    { region = lang_memory_region_lookup ((yyvsp[0].name), TRUE); }
    break;

  case 179:
#line 690 "ldgram.y"
    {}
    break;

  case 180:
#line 692 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
    break;

  case 181:
#line 694 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 182:
#line 699 "ldgram.y"
    {
		  region->origin = exp_get_vma ((yyvsp[0].etree), 0, "origin");
		  region->current = region->origin;
		}
    break;

  case 183:
#line 707 "ldgram.y"
    {
		  region->length = exp_get_vma ((yyvsp[0].etree), -1, "length");
		}
    break;

  case 184:
#line 714 "ldgram.y"
    { /* dummy action to avoid bison 1.25 error message */ }
    break;

  case 188:
#line 725 "ldgram.y"
    { lang_set_flags (region, (yyvsp[0].name), 0); }
    break;

  case 189:
#line 727 "ldgram.y"
    { lang_set_flags (region, (yyvsp[0].name), 1); }
    break;

  case 190:
#line 732 "ldgram.y"
    { lang_startup((yyvsp[-1].name)); }
    break;

  case 192:
#line 738 "ldgram.y"
    { ldemul_hll((char *)NULL); }
    break;

  case 193:
#line 743 "ldgram.y"
    { ldemul_hll((yyvsp[0].name)); }
    break;

  case 194:
#line 745 "ldgram.y"
    { ldemul_hll((yyvsp[0].name)); }
    break;

  case 196:
#line 753 "ldgram.y"
    { ldemul_syslib((yyvsp[0].name)); }
    break;

  case 198:
#line 759 "ldgram.y"
    { lang_float(TRUE); }
    break;

  case 199:
#line 761 "ldgram.y"
    { lang_float(FALSE); }
    break;

  case 200:
#line 766 "ldgram.y"
    {
		  (yyval.nocrossref) = NULL;
		}
    break;

  case 201:
#line 770 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-1].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 202:
#line 779 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-2].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 203:
#line 789 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 204:
#line 791 "ldgram.y"
    { ldlex_popstate (); (yyval.etree)=(yyvsp[0].etree);}
    break;

  case 205:
#line 796 "ldgram.y"
    { (yyval.etree) = exp_unop ('-', (yyvsp[0].etree)); }
    break;

  case 206:
#line 798 "ldgram.y"
    { (yyval.etree) = (yyvsp[-1].etree); }
    break;

  case 207:
#line 800 "ldgram.y"
    { (yyval.etree) = exp_unop ((int) (yyvsp[-3].integer),(yyvsp[-1].etree)); }
    break;

  case 208:
#line 802 "ldgram.y"
    { (yyval.etree) = exp_unop ('!', (yyvsp[0].etree)); }
    break;

  case 209:
#line 804 "ldgram.y"
    { (yyval.etree) = (yyvsp[0].etree); }
    break;

  case 210:
#line 806 "ldgram.y"
    { (yyval.etree) = exp_unop ('~', (yyvsp[0].etree));}
    break;

  case 211:
#line 809 "ldgram.y"
    { (yyval.etree) = exp_binop ('*', (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 212:
#line 811 "ldgram.y"
    { (yyval.etree) = exp_binop ('/', (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 213:
#line 813 "ldgram.y"
    { (yyval.etree) = exp_binop ('%', (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 214:
#line 815 "ldgram.y"
    { (yyval.etree) = exp_binop ('+', (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 215:
#line 817 "ldgram.y"
    { (yyval.etree) = exp_binop ('-' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 216:
#line 819 "ldgram.y"
    { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 217:
#line 821 "ldgram.y"
    { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 218:
#line 823 "ldgram.y"
    { (yyval.etree) = exp_binop (EQ , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 219:
#line 825 "ldgram.y"
    { (yyval.etree) = exp_binop (NE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 220:
#line 827 "ldgram.y"
    { (yyval.etree) = exp_binop (LE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 221:
#line 829 "ldgram.y"
    { (yyval.etree) = exp_binop (GE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 222:
#line 831 "ldgram.y"
    { (yyval.etree) = exp_binop ('<' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 223:
#line 833 "ldgram.y"
    { (yyval.etree) = exp_binop ('>' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 224:
#line 835 "ldgram.y"
    { (yyval.etree) = exp_binop ('&' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 225:
#line 837 "ldgram.y"
    { (yyval.etree) = exp_binop ('^' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 226:
#line 839 "ldgram.y"
    { (yyval.etree) = exp_binop ('|' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 227:
#line 841 "ldgram.y"
    { (yyval.etree) = exp_trinop ('?' , (yyvsp[-4].etree), (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 228:
#line 843 "ldgram.y"
    { (yyval.etree) = exp_binop (ANDAND , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 229:
#line 845 "ldgram.y"
    { (yyval.etree) = exp_binop (OROR , (yyvsp[-2].etree), (yyvsp[0].etree)); }
    break;

  case 230:
#line 847 "ldgram.y"
    { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[-1].name)); }
    break;

  case 231:
#line 849 "ldgram.y"
    { (yyval.etree) = exp_bigintop ((yyvsp[0].bigint).integer, (yyvsp[0].bigint).str); }
    break;

  case 232:
#line 851 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
    break;

  case 233:
#line 854 "ldgram.y"
    { (yyval.etree) = exp_nameop (ALIGNOF,(yyvsp[-1].name)); }
    break;

  case 234:
#line 856 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF,(yyvsp[-1].name)); }
    break;

  case 235:
#line 858 "ldgram.y"
    { (yyval.etree) = exp_nameop (ADDR,(yyvsp[-1].name)); }
    break;

  case 236:
#line 860 "ldgram.y"
    { (yyval.etree) = exp_nameop (LOADADDR,(yyvsp[-1].name)); }
    break;

  case 237:
#line 862 "ldgram.y"
    { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[-1].name)); }
    break;

  case 238:
#line 864 "ldgram.y"
    { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[-1].etree)); }
    break;

  case 239:
#line 866 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
    break;

  case 240:
#line 868 "ldgram.y"
    { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[-3].etree),(yyvsp[-1].etree)); }
    break;

  case 241:
#line 870 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[-3].etree), (yyvsp[-1].etree)); }
    break;

  case 242:
#line 872 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[-1].etree), (yyvsp[-3].etree)); }
    break;

  case 243:
#line 874 "ldgram.y"
    { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[-1].etree)); }
    break;

  case 244:
#line 876 "ldgram.y"
    { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[-1].etree),
					  exp_nameop (NAME, (yyvsp[-3].name))); }
    break;

  case 245:
#line 885 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
    break;

  case 246:
#line 887 "ldgram.y"
    { (yyval.etree) = exp_nameop (NAME,(yyvsp[0].name)); }
    break;

  case 247:
#line 889 "ldgram.y"
    { (yyval.etree) = exp_binop (MAX_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
    break;

  case 248:
#line 891 "ldgram.y"
    { (yyval.etree) = exp_binop (MIN_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
    break;

  case 249:
#line 893 "ldgram.y"
    { (yyval.etree) = exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name)); }
    break;

  case 250:
#line 895 "ldgram.y"
    { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[-1].name)); }
    break;

  case 251:
#line 897 "ldgram.y"
    { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[-1].name)); }
    break;

  case 252:
#line 902 "ldgram.y"
    { (yyval.name) = (yyvsp[0].name); }
    break;

  case 253:
#line 903 "ldgram.y"
    { (yyval.name) = 0; }
    break;

  case 254:
#line 907 "ldgram.y"
    { (yyval.etree) = (yyvsp[-1].etree); }
    break;

  case 255:
#line 908 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 256:
#line 912 "ldgram.y"
    { (yyval.etree) = (yyvsp[-1].etree); }
    break;

  case 257:
#line 913 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 258:
#line 917 "ldgram.y"
    { (yyval.etree) = (yyvsp[-1].etree); }
    break;

  case 259:
#line 918 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 260:
#line 922 "ldgram.y"
    { (yyval.token) = ONLY_IF_RO; }
    break;

  case 261:
#line 923 "ldgram.y"
    { (yyval.token) = ONLY_IF_RW; }
    break;

  case 262:
#line 924 "ldgram.y"
    { (yyval.token) = SPECIAL; }
    break;

  case 263:
#line 925 "ldgram.y"
    { (yyval.token) = 0; }
    break;

  case 264:
#line 928 "ldgram.y"
    { ldlex_expression(); }
    break;

  case 265:
#line 932 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 266:
#line 935 "ldgram.y"
    {
			  lang_enter_output_section_statement((yyvsp[-8].name), (yyvsp[-6].etree),
							      sectype,
							      (yyvsp[-4].etree), (yyvsp[-3].etree), (yyvsp[-5].etree), (yyvsp[-1].token));
			}
    break;

  case 267:
#line 941 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 268:
#line 943 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_leave_output_section_statement ((yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
		}
    break;

  case 269:
#line 948 "ldgram.y"
    {}
    break;

  case 270:
#line 950 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 271:
#line 952 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 272:
#line 954 "ldgram.y"
    {
			  lang_enter_overlay ((yyvsp[-5].etree), (yyvsp[-2].etree));
			}
    break;

  case 273:
#line 959 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 274:
#line 961 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay ((yyvsp[-11].etree), (int) (yyvsp[-12].integer),
					      (yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
			}
    break;

  case 276:
#line 971 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 277:
#line 973 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assop ('=', ".", (yyvsp[0].etree)));
		}
    break;

  case 279:
#line 979 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
    break;

  case 280:
#line 981 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 281:
#line 985 "ldgram.y"
    { sectype = noload_section; }
    break;

  case 282:
#line 986 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 283:
#line 987 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 284:
#line 988 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 285:
#line 989 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 287:
#line 994 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 288:
#line 995 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 289:
#line 999 "ldgram.y"
    { (yyval.etree) = (yyvsp[-2].etree); }
    break;

  case 290:
#line 1000 "ldgram.y"
    { (yyval.etree) = (etree_type *)NULL;  }
    break;

  case 291:
#line 1005 "ldgram.y"
    { (yyval.etree) = (yyvsp[-3].etree); }
    break;

  case 292:
#line 1007 "ldgram.y"
    { (yyval.etree) = (yyvsp[-7].etree); }
    break;

  case 293:
#line 1011 "ldgram.y"
    { (yyval.etree) = (yyvsp[-1].etree); }
    break;

  case 294:
#line 1012 "ldgram.y"
    { (yyval.etree) = (etree_type *) NULL;  }
    break;

  case 295:
#line 1017 "ldgram.y"
    { (yyval.integer) = 0; }
    break;

  case 296:
#line 1019 "ldgram.y"
    { (yyval.integer) = 1; }
    break;

  case 297:
#line 1024 "ldgram.y"
    { (yyval.name) = (yyvsp[0].name); }
    break;

  case 298:
#line 1025 "ldgram.y"
    { (yyval.name) = DEFAULT_MEMORY_REGION; }
    break;

  case 299:
#line 1030 "ldgram.y"
    {
		  (yyval.section_phdr) = NULL;
		}
    break;

  case 300:
#line 1034 "ldgram.y"
    {
		  struct lang_output_section_phdr_list *n;

		  n = ((struct lang_output_section_phdr_list *)
		       xmalloc (sizeof *n));
		  n->name = (yyvsp[0].name);
		  n->used = FALSE;
		  n->next = (yyvsp[-2].section_phdr);
		  (yyval.section_phdr) = n;
		}
    break;

  case 302:
#line 1050 "ldgram.y"
    {
			  ldlex_script ();
			  lang_enter_overlay_section ((yyvsp[0].name));
			}
    break;

  case 303:
#line 1055 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 304:
#line 1057 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay_section ((yyvsp[0].fill), (yyvsp[-1].section_phdr));
			}
    break;

  case 309:
#line 1074 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 310:
#line 1075 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 311:
#line 1077 "ldgram.y"
    {
		  lang_new_phdr ((yyvsp[-5].name), (yyvsp[-3].etree), (yyvsp[-2].phdr).filehdr, (yyvsp[-2].phdr).phdrs, (yyvsp[-2].phdr).at,
				 (yyvsp[-2].phdr).flags);
		}
    break;

  case 312:
#line 1085 "ldgram.y"
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
				     s);
			      (yyval.etree) = exp_intop (0);
			    }
			}
		    }
		}
    break;

  case 313:
#line 1129 "ldgram.y"
    {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
    break;

  case 314:
#line 1133 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  if (strcmp ((yyvsp[-2].name), "FILEHDR") == 0 && (yyvsp[-1].etree) == NULL)
		    (yyval.phdr).filehdr = TRUE;
		  else if (strcmp ((yyvsp[-2].name), "PHDRS") == 0 && (yyvsp[-1].etree) == NULL)
		    (yyval.phdr).phdrs = TRUE;
		  else if (strcmp ((yyvsp[-2].name), "FLAGS") == 0 && (yyvsp[-1].etree) != NULL)
		    (yyval.phdr).flags = (yyvsp[-1].etree);
		  else
		    einfo (_("%X%P:%S: PHDRS syntax error at `%s'\n"), (yyvsp[-2].name));
		}
    break;

  case 315:
#line 1145 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  (yyval.phdr).at = (yyvsp[-2].etree);
		}
    break;

  case 316:
#line 1153 "ldgram.y"
    {
		  (yyval.etree) = NULL;
		}
    break;

  case 317:
#line 1157 "ldgram.y"
    {
		  (yyval.etree) = (yyvsp[-1].etree);
		}
    break;

  case 318:
#line 1163 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
    break;

  case 319:
#line 1168 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 323:
#line 1185 "ldgram.y"
    {
		  lang_append_dynamic_list ((yyvsp[-1].versyms));
		}
    break;

  case 324:
#line 1193 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
    break;

  case 325:
#line 1198 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 326:
#line 1207 "ldgram.y"
    {
		  ldlex_version_script ();
		}
    break;

  case 327:
#line 1211 "ldgram.y"
    {
		  ldlex_popstate ();
		}
    break;

  case 330:
#line 1223 "ldgram.y"
    {
		  lang_register_vers_node (NULL, (yyvsp[-2].versnode), NULL);
		}
    break;

  case 331:
#line 1227 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[-4].name), (yyvsp[-2].versnode), NULL);
		}
    break;

  case 332:
#line 1231 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[-5].name), (yyvsp[-3].versnode), (yyvsp[-1].deflist));
		}
    break;

  case 333:
#line 1238 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[0].name));
		}
    break;

  case 334:
#line 1242 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[-1].deflist), (yyvsp[0].name));
		}
    break;

  case 335:
#line 1249 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
    break;

  case 336:
#line 1253 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
    break;

  case 337:
#line 1257 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
    break;

  case 338:
#line 1261 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[-1].versyms));
		}
    break;

  case 339:
#line 1265 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-5].versyms), (yyvsp[-1].versyms));
		}
    break;

  case 340:
#line 1272 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 341:
#line 1276 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 342:
#line 1280 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 343:
#line 1284 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 344:
#line 1288 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
    break;

  case 345:
#line 1293 "ldgram.y"
    {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[-2].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[-8].versyms);
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
    break;

  case 346:
#line 1301 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
    break;

  case 347:
#line 1306 "ldgram.y"
    {
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
    break;

  case 348:
#line 1311 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 349:
#line 1315 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 350:
#line 1319 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 351:
#line 1323 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 352:
#line 1327 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 353:
#line 1331 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "extern", ldgram_vers_current_lang, FALSE);
		}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 4025 "ldgram.c"

  yyvsp -= yylen;
  yyssp -= yylen;


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
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
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
	  int yychecklim = YYLAST - yyn;
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
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
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
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
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
	  yydestruct ("Error: discarding", yytoken, &yylval);
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
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
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
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 1341 "ldgram.y"

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

