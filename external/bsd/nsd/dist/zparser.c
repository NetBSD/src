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
#line 1 "zparser.y" /* yacc.c:339  */

/*
 * zyparser.y -- yacc grammar for (DNS) zone files
 *
 * Copyright (c) 2001-2006, NLnet Labs. All rights reserved.
 *
 * See LICENSE for the license.
 *
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "dname.h"
#include "namedb.h"
#include "zonec.h"

/* these need to be global, otherwise they cannot be used inside yacc */
zparser_type *parser;

#ifdef __cplusplus
extern "C"
#endif /* __cplusplus */
int yywrap(void);

/* this hold the nxt bits */
static uint8_t nxtbits[16];
static int dlv_warn = 1;

/* 256 windows of 256 bits (32 bytes) */
/* still need to reset the bastard somewhere */
static uint8_t nsecbits[NSEC_WINDOW_COUNT][NSEC_WINDOW_BITS_SIZE];

/* hold the highest rcode seen in a NSEC rdata , BUG #106 */
uint16_t nsec_highest_rcode;

void yyerror(const char *message);

#ifdef NSEC3
/* parse nsec3 parameters and add the (first) rdata elements */
static void
nsec3_add_params(const char* hash_algo_str, const char* flag_str,
	const char* iter_str, const char* salt_str, int salt_len);
#endif /* NSEC3 */


#line 116 "zparser.c" /* yacc.c:339  */

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
   by #include "zparser.h".  */
#ifndef YY_YY_ZPARSER_H_INCLUDED
# define YY_YY_ZPARSER_H_INCLUDED
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
    T_A = 258,
    T_NS = 259,
    T_MX = 260,
    T_TXT = 261,
    T_CNAME = 262,
    T_AAAA = 263,
    T_PTR = 264,
    T_NXT = 265,
    T_KEY = 266,
    T_SOA = 267,
    T_SIG = 268,
    T_SRV = 269,
    T_CERT = 270,
    T_LOC = 271,
    T_MD = 272,
    T_MF = 273,
    T_MB = 274,
    T_MG = 275,
    T_MR = 276,
    T_NULL = 277,
    T_WKS = 278,
    T_HINFO = 279,
    T_MINFO = 280,
    T_RP = 281,
    T_AFSDB = 282,
    T_X25 = 283,
    T_ISDN = 284,
    T_RT = 285,
    T_NSAP = 286,
    T_NSAP_PTR = 287,
    T_PX = 288,
    T_GPOS = 289,
    T_EID = 290,
    T_NIMLOC = 291,
    T_ATMA = 292,
    T_NAPTR = 293,
    T_KX = 294,
    T_A6 = 295,
    T_DNAME = 296,
    T_SINK = 297,
    T_OPT = 298,
    T_APL = 299,
    T_UINFO = 300,
    T_UID = 301,
    T_GID = 302,
    T_UNSPEC = 303,
    T_TKEY = 304,
    T_TSIG = 305,
    T_IXFR = 306,
    T_AXFR = 307,
    T_MAILB = 308,
    T_MAILA = 309,
    T_DS = 310,
    T_DLV = 311,
    T_SSHFP = 312,
    T_RRSIG = 313,
    T_NSEC = 314,
    T_DNSKEY = 315,
    T_SPF = 316,
    T_NSEC3 = 317,
    T_IPSECKEY = 318,
    T_DHCID = 319,
    T_NSEC3PARAM = 320,
    T_TLSA = 321,
    T_URI = 322,
    T_NID = 323,
    T_L32 = 324,
    T_L64 = 325,
    T_LP = 326,
    T_EUI48 = 327,
    T_EUI64 = 328,
    T_CAA = 329,
    T_CDS = 330,
    T_CDNSKEY = 331,
    T_OPENPGPKEY = 332,
    T_CSYNC = 333,
    T_AVC = 334,
    DOLLAR_TTL = 335,
    DOLLAR_ORIGIN = 336,
    NL = 337,
    SP = 338,
    STR = 339,
    PREV = 340,
    BITLAB = 341,
    T_TTL = 342,
    T_RRCLASS = 343,
    URR = 344,
    T_UTYPE = 345
  };
#endif
/* Tokens.  */
#define T_A 258
#define T_NS 259
#define T_MX 260
#define T_TXT 261
#define T_CNAME 262
#define T_AAAA 263
#define T_PTR 264
#define T_NXT 265
#define T_KEY 266
#define T_SOA 267
#define T_SIG 268
#define T_SRV 269
#define T_CERT 270
#define T_LOC 271
#define T_MD 272
#define T_MF 273
#define T_MB 274
#define T_MG 275
#define T_MR 276
#define T_NULL 277
#define T_WKS 278
#define T_HINFO 279
#define T_MINFO 280
#define T_RP 281
#define T_AFSDB 282
#define T_X25 283
#define T_ISDN 284
#define T_RT 285
#define T_NSAP 286
#define T_NSAP_PTR 287
#define T_PX 288
#define T_GPOS 289
#define T_EID 290
#define T_NIMLOC 291
#define T_ATMA 292
#define T_NAPTR 293
#define T_KX 294
#define T_A6 295
#define T_DNAME 296
#define T_SINK 297
#define T_OPT 298
#define T_APL 299
#define T_UINFO 300
#define T_UID 301
#define T_GID 302
#define T_UNSPEC 303
#define T_TKEY 304
#define T_TSIG 305
#define T_IXFR 306
#define T_AXFR 307
#define T_MAILB 308
#define T_MAILA 309
#define T_DS 310
#define T_DLV 311
#define T_SSHFP 312
#define T_RRSIG 313
#define T_NSEC 314
#define T_DNSKEY 315
#define T_SPF 316
#define T_NSEC3 317
#define T_IPSECKEY 318
#define T_DHCID 319
#define T_NSEC3PARAM 320
#define T_TLSA 321
#define T_URI 322
#define T_NID 323
#define T_L32 324
#define T_L64 325
#define T_LP 326
#define T_EUI48 327
#define T_EUI64 328
#define T_CAA 329
#define T_CDS 330
#define T_CDNSKEY 331
#define T_OPENPGPKEY 332
#define T_CSYNC 333
#define T_AVC 334
#define DOLLAR_TTL 335
#define DOLLAR_ORIGIN 336
#define NL 337
#define SP 338
#define STR 339
#define PREV 340
#define BITLAB 341
#define T_TTL 342
#define T_RRCLASS 343
#define URR 344
#define T_UTYPE 345

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 50 "zparser.y" /* yacc.c:355  */

	domain_type	 *domain;
	const dname_type *dname;
	struct lex_data	  data;
	uint32_t	  ttl;
	uint16_t	  klass;
	uint16_t	  type;
	uint16_t	 *unknown;

#line 346 "zparser.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_ZPARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 363 "zparser.c" /* yacc.c:358  */

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
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   931

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  93
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  74
/* YYNRULES -- Number of rules.  */
#define YYNRULES  233
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  574

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   345

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
       2,     2,     2,     2,     2,     2,    91,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    92,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    92,    92,    93,    96,    97,    98,    99,   107,   115,
     135,   139,   140,   143,   144,   147,   157,   168,   174,   181,
     186,   193,   197,   202,   207,   212,   219,   220,   241,   245,
     249,   259,   273,   280,   281,   299,   300,   303,   311,   323,
     340,   341,   356,   360,   370,   371,   376,   385,   397,   406,
     417,   420,   423,   437,   438,   445,   446,   462,   463,   478,
     479,   484,   494,   512,   513,   514,   515,   516,   517,   522,
     523,   529,   530,   531,   532,   533,   534,   540,   541,   542,
     543,   545,   546,   547,   548,   549,   550,   551,   552,   553,
     554,   555,   556,   557,   558,   559,   560,   561,   562,   563,
     564,   565,   566,   567,   568,   569,   570,   571,   572,   573,
     574,   575,   576,   577,   578,   579,   580,   581,   582,   583,
     584,   585,   586,   587,   588,   589,   590,   591,   592,   593,
     594,   595,   596,   597,   598,   599,   600,   601,   602,   603,
     604,   605,   606,   607,   608,   609,   610,   611,   612,   613,
     614,   615,   616,   617,   618,   619,   620,   621,   622,   623,
     624,   625,   626,   627,   628,   629,   630,   631,   632,   633,
     634,   635,   636,   637,   638,   639,   640,   641,   642,   643,
     644,   645,   657,   663,   670,   683,   690,   697,   705,   712,
     719,   727,   735,   742,   746,   754,   762,   774,   782,   788,
     794,   802,   812,   824,   832,   842,   845,   849,   855,   864,
     873,   881,   887,   902,   912,   927,   937,   946,   955,   994,
     998,  1002,  1009,  1016,  1023,  1030,  1036,  1043,  1052,  1061,
    1068,  1078,  1084,  1088
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_A", "T_NS", "T_MX", "T_TXT",
  "T_CNAME", "T_AAAA", "T_PTR", "T_NXT", "T_KEY", "T_SOA", "T_SIG",
  "T_SRV", "T_CERT", "T_LOC", "T_MD", "T_MF", "T_MB", "T_MG", "T_MR",
  "T_NULL", "T_WKS", "T_HINFO", "T_MINFO", "T_RP", "T_AFSDB", "T_X25",
  "T_ISDN", "T_RT", "T_NSAP", "T_NSAP_PTR", "T_PX", "T_GPOS", "T_EID",
  "T_NIMLOC", "T_ATMA", "T_NAPTR", "T_KX", "T_A6", "T_DNAME", "T_SINK",
  "T_OPT", "T_APL", "T_UINFO", "T_UID", "T_GID", "T_UNSPEC", "T_TKEY",
  "T_TSIG", "T_IXFR", "T_AXFR", "T_MAILB", "T_MAILA", "T_DS", "T_DLV",
  "T_SSHFP", "T_RRSIG", "T_NSEC", "T_DNSKEY", "T_SPF", "T_NSEC3",
  "T_IPSECKEY", "T_DHCID", "T_NSEC3PARAM", "T_TLSA", "T_URI", "T_NID",
  "T_L32", "T_L64", "T_LP", "T_EUI48", "T_EUI64", "T_CAA", "T_CDS",
  "T_CDNSKEY", "T_OPENPGPKEY", "T_CSYNC", "T_AVC", "DOLLAR_TTL",
  "DOLLAR_ORIGIN", "NL", "SP", "STR", "PREV", "BITLAB", "T_TTL",
  "T_RRCLASS", "URR", "T_UTYPE", "'.'", "'@'", "$accept", "lines", "line",
  "sp", "trail", "ttl_directive", "origin_directive", "rr", "owner",
  "classttl", "dname", "abs_dname", "label", "rel_dname", "wire_dname",
  "wire_abs_dname", "wire_label", "wire_rel_dname", "str_seq",
  "concatenated_str_seq", "nxt_seq", "nsec_more", "nsec_seq", "str_sp_seq",
  "str_dot_seq", "dotted_str", "type_and_rdata", "rdata_a",
  "rdata_domain_name", "rdata_soa", "rdata_wks", "rdata_hinfo",
  "rdata_minfo", "rdata_mx", "rdata_txt", "rdata_rp", "rdata_afsdb",
  "rdata_x25", "rdata_isdn", "rdata_rt", "rdata_nsap", "rdata_px",
  "rdata_aaaa", "rdata_loc", "rdata_nxt", "rdata_srv", "rdata_naptr",
  "rdata_kx", "rdata_cert", "rdata_apl", "rdata_apl_seq", "rdata_ds",
  "rdata_dlv", "rdata_sshfp", "rdata_dhcid", "rdata_rrsig", "rdata_nsec",
  "rdata_nsec3", "rdata_nsec3_param", "rdata_tlsa", "rdata_dnskey",
  "rdata_ipsec_base", "rdata_ipseckey", "rdata_nid", "rdata_l32",
  "rdata_l64", "rdata_lp", "rdata_eui48", "rdata_eui64", "rdata_uri",
  "rdata_caa", "rdata_openpgpkey", "rdata_csync", "rdata_unknown", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,    46,    64
};
# endif

#define YYPACT_NINF -438

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-438)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -438,   107,  -438,   -70,   -68,   -68,  -438,  -438,  -438,    32,
    -438,  -438,  -438,  -438,   -48,  -438,  -438,  -438,    36,   -68,
    -438,  -438,    25,  -438,    98,    18,  -438,  -438,  -438,   -68,
     -68,   781,    60,    14,   102,   102,   -75,   -58,    16,   -68,
     -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,
     -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,
     -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,
     -68,   102,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,
     -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,
     -68,   -68,   -68,   -68,   -68,   -68,   -68,   145,   -68,  -438,
    -438,  -438,   159,  -438,  -438,  -438,   -68,   -68,    69,    42,
     -52,    69,    42,    69,    42,    42,    38,    42,   112,   121,
     124,    72,    42,    42,    42,    42,    42,    69,   128,    42,
      42,   132,   135,   139,   142,   151,   154,   167,   170,    42,
      56,  -438,   180,   182,   191,   112,    82,    38,    69,   194,
     201,   203,   205,   216,   219,   228,   231,   240,   243,   250,
     253,   260,   180,    38,   203,   262,    69,    77,   -56,  -438,
      60,    60,  -438,    13,  -438,    53,  -438,  -438,   102,  -438,
    -438,   -68,  -438,  -438,   102,    63,  -438,  -438,  -438,  -438,
      53,  -438,  -438,  -438,  -438,   -68,  -438,  -438,   -68,  -438,
    -438,   -68,  -438,  -438,   -68,  -438,  -438,   -68,  -438,  -438,
     -68,  -438,  -438,  -438,  -438,    59,  -438,  -438,  -438,  -438,
    -438,  -438,  -438,  -438,  -438,  -438,  -438,  -438,   -78,  -438,
    -438,   -68,  -438,  -438,   -68,  -438,  -438,   -68,  -438,  -438,
     -68,  -438,  -438,   102,  -438,  -438,   102,  -438,  -438,   -68,
    -438,  -438,  -438,    66,  -438,  -438,   -68,  -438,  -438,   -68,
    -438,  -438,   -68,  -438,  -438,  -438,  -438,    63,  -438,   102,
    -438,   -68,  -438,  -438,   -68,  -438,  -438,   -68,  -438,  -438,
    -438,  -438,  -438,  -438,   270,  -438,  -438,    71,  -438,  -438,
    -438,  -438,  -438,  -438,   -68,  -438,  -438,   -68,   102,  -438,
    -438,  -438,   102,  -438,  -438,   -68,  -438,  -438,   -68,  -438,
    -438,   -68,  -438,  -438,   -68,  -438,  -438,   -68,  -438,  -438,
     -68,  -438,  -438,   -68,  -438,  -438,   102,  -438,  -438,   102,
    -438,  -438,   -68,  -438,  -438,  -438,  -438,  -438,  -438,   102,
    -438,  -438,   -68,  -438,  -438,  -438,  -438,  -438,  -438,    92,
     275,   118,  -438,  -438,    18,    29,  -438,  -438,   284,   293,
      18,   301,   304,   310,   136,   273,  -438,   313,   315,    18,
      18,    18,  -438,   279,  -438,    18,   148,  -438,    18,   320,
      18,    29,  -438,   323,   328,   336,  -438,   282,  -438,   160,
     344,   353,   290,  -438,   299,  -438,   355,   365,   367,    84,
      84,    84,    18,  -438,  -438,   372,  -438,   375,  -438,   102,
    -438,   102,    63,  -438,   102,   -68,   -68,   -68,   -68,   -68,
    -438,  -438,   -68,   102,   102,   102,   102,   102,   102,  -438,
     -68,   -68,   102,    63,   -68,   -68,   -68,  -438,   282,   270,
    -438,  -438,   -68,   -68,   102,  -438,   -68,   -68,   -68,    53,
      53,    53,   102,   -68,   270,   290,  -438,  -438,   308,  -438,
     377,   379,   382,   385,   390,    86,  -438,  -438,  -438,  -438,
    -438,  -438,    18,   394,  -438,   399,   401,   403,  -438,  -438,
     405,   427,  -438,   433,   435,   437,  -438,  -438,  -438,  -438,
     439,  -438,   102,  -438,   -68,   -68,   -68,   -68,   -68,    59,
     102,   -68,   -68,   -68,   102,   -68,   -68,   -68,   -68,   102,
     102,  -438,   403,   441,   458,    18,   403,  -438,  -438,   460,
     403,   403,  -438,   462,    84,   471,   403,  -438,  -438,   102,
     -68,   -68,   102,   102,   -68,   102,   102,   -68,    63,   102,
     102,  -438,   473,   478,  -438,  -438,   480,  -438,  -438,   484,
    -438,  -438,   -68,   -68,   -68,   270,   489,   491,    18,  -438,
     -68,   -68,   102,   493,   495,  -438,   102,   -68,  -438,    95,
     -68,   403,   102,  -438
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     4,    11,    31,    20,
      32,    28,    29,     3,     0,     7,     8,     9,    21,     0,
      26,    33,    27,    10,     0,     0,     6,     5,    12,     0,
       0,     0,    19,    30,     0,     0,     0,    23,    22,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
      34,    13,     0,    15,    16,    17,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   135,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    14,
      24,    25,    59,     0,    60,     0,    63,    64,     0,    65,
      66,     0,    89,    90,     0,    42,    91,    92,    71,    72,
       0,   119,   120,    83,    84,     0,   123,   124,     0,   115,
     116,     0,    73,    74,     0,   113,   114,     0,   125,   126,
       0,   131,   132,    44,    45,     0,   121,   122,    67,    68,
      69,    70,    75,    76,    77,    78,    79,    80,     0,    81,
      82,     0,    85,    86,     0,    87,    88,     0,    97,    98,
       0,    99,   100,     0,   101,   102,     0,   103,   104,     0,
     109,   110,    57,     0,   111,   112,     0,   117,   118,     0,
     127,   128,     0,   129,   130,   133,   134,   206,   136,     0,
     137,     0,   138,   139,     0,   140,   141,     0,   142,   143,
     144,   145,    39,    37,     0,    35,    40,    36,   146,   147,
     152,   153,    93,    94,     0,   148,   149,     0,     0,   105,
     106,    55,     0,   107,   108,     0,   150,   151,     0,   154,
     155,     0,   178,   179,     0,   156,   157,     0,   158,   159,
       0,   160,   161,     0,   162,   163,     0,   164,   165,     0,
     166,   167,     0,   168,   169,   170,   171,   172,   173,     0,
     174,   175,     0,   176,   177,    95,    96,   181,   180,     0,
       0,    61,   182,   183,     0,     0,   189,   198,     0,     0,
       0,     0,     0,     0,     0,     0,   199,     0,     0,     0,
       0,     0,   192,     0,   193,     0,     0,   196,     0,     0,
       0,     0,   205,     0,     0,     0,    53,     0,   213,    38,
       0,     0,     0,   220,     0,   211,     0,     0,     0,     0,
       0,     0,     0,   225,   226,     0,   229,     0,   233,     0,
      62,     0,    43,    48,     0,     0,     0,     0,     0,     0,
      47,    46,     0,     0,     0,     0,     0,     0,     0,    58,
       0,     0,     0,   207,     0,     0,     0,    51,     0,     0,
      54,    41,     0,     0,     0,    56,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   232,   188,     0,   200,
       0,     0,     0,     0,     0,     0,   186,   187,   190,   191,
     194,   195,     0,     0,   203,     0,     0,     0,    50,    52,
       0,     0,   219,     0,     0,     0,   221,   222,   223,   224,
       0,   230,     0,    49,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   231,     0,     0,     0,     0,     0,   185,   197,     0,
       0,     0,   210,     0,     0,     0,     0,   227,   228,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   218,     0,
       0,   217,     0,     0,   201,   204,     0,   208,   209,     0,
     215,   216,     0,     0,     0,     0,     0,     0,     0,   214,
       0,     0,     0,     0,     0,   202,     0,     0,   184,     0,
       0,     0,     0,   212
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -438,  -438,  -438,    -1,   211,  -438,  -438,  -438,  -438,  -438,
       0,   204,   143,   224,  -317,  -438,  -132,  -438,  -438,  -205,
    -438,  -176,  -437,   -45,  -438,    24,  -438,  -438,  -103,  -438,
    -438,  -438,  -438,  -438,  -142,  -438,  -438,  -438,  -438,  -438,
    -438,  -438,  -438,  -438,  -438,  -438,  -438,  -438,  -438,  -438,
    -438,   105,  -438,  -438,  -438,   127,  -438,  -438,  -438,  -438,
    -137,  -438,  -438,  -438,  -438,  -438,  -438,  -438,  -438,  -438,
    -438,  -438,  -438,   763
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    13,   102,   103,    15,    16,    17,    18,    31,
     178,    20,    21,    22,   284,   285,   286,   287,   184,   215,
     414,   440,   388,   302,   253,   185,    99,   176,   179,   202,
     229,   232,   235,   182,   186,   238,   241,   244,   247,   250,
     254,   257,   191,   216,   196,   208,   260,   263,   211,   268,
     269,   272,   275,   278,   303,   205,   288,   295,   306,   309,
     199,   298,   299,   315,   318,   321,   324,   327,   330,   312,
     333,   340,   343,   177
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      14,    19,   479,    24,    25,     7,   292,   101,     7,   188,
     290,   193,    23,   351,   349,     7,    33,   491,    32,   218,
     220,   222,   224,   226,   345,    28,   337,    28,    37,    38,
     106,    28,   181,   173,    27,    28,   265,   173,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,     7,   168,     8,    28,
      10,    28,     8,   107,    10,   170,   171,     2,     3,    11,
      12,   169,    28,   172,    26,   195,    33,   201,   559,   339,
     174,    28,   198,    29,    30,    28,     8,   173,    10,   234,
     237,   173,   175,    11,    12,   101,     7,   190,   169,    28,
     172,   101,     7,    28,   351,   173,   167,   174,   101,     7,
     364,   228,    28,   172,   351,    28,   213,   376,   173,   347,
     174,   173,   389,   214,   267,    28,   282,    28,   172,    28,
     213,   173,   350,   283,   408,   174,   100,   214,    28,   282,
     354,    28,    34,   355,   101,     7,   283,     4,     5,     6,
       7,     8,     9,    10,   358,    28,   204,   359,    11,    12,
     360,   173,   410,   361,    28,   207,   362,    28,   210,   363,
     173,    28,   231,   173,   365,    28,   240,   173,    28,   243,
     420,   173,    28,   246,   173,    28,   249,   367,   173,    35,
     368,   173,   429,   369,    28,   252,   370,    28,   256,   371,
     173,   169,    28,   173,   282,   373,   104,   105,   375,    36,
      28,   259,   570,    28,   262,   378,   173,   441,   379,   173,
     499,   380,   478,    28,   271,    28,   274,   335,   381,   173,
     383,   173,   280,   384,    28,   277,   385,    28,   294,     0,
     173,     0,   141,   173,    28,   297,    28,   301,    28,   305,
     173,     0,   173,   390,   173,     0,   391,   392,     0,    28,
     308,   394,    28,   311,   396,   173,     0,   397,   173,     0,
     398,    28,   314,   399,    28,   317,   400,   173,     0,   401,
     173,     0,   402,    28,   320,     0,    28,   323,     0,   173,
       0,   405,   173,    28,   326,     0,    28,   329,   394,   173,
       0,   407,   173,    28,   332,    28,   342,   444,     0,   173,
       0,   173,   386,   387,   411,   169,    28,   421,    28,   409,
     416,   169,    28,   427,   437,   438,   439,    28,   413,   424,
     425,   426,   169,    28,   301,   428,    28,   415,   430,   412,
     432,   169,    28,   445,    28,   417,   352,    28,   418,   353,
     169,    28,   493,    28,   419,   356,    28,   422,    28,   423,
       0,   357,   452,    28,   431,   433,    28,   434,   455,     0,
     492,    28,   435,   458,   460,   461,   462,   463,   464,    28,
     436,   465,     0,   449,   450,   451,   366,    28,   442,   472,
     473,     0,   504,   475,   476,   477,    28,   443,    28,   446,
       0,   480,   481,   394,     0,   483,   484,   485,    28,   447,
      28,   448,   490,     0,   372,    28,   453,   374,    28,   454,
      28,   494,    28,   495,   377,    28,   496,   529,    28,   497,
       0,   533,   500,    28,   498,   535,   536,    28,   501,     0,
     382,   540,    28,   502,    28,   503,    28,   301,    28,   505,
       0,   394,     0,   512,   513,   514,   515,   516,   365,     0,
     519,   520,   521,   394,   523,   524,   525,   526,     0,   393,
      28,   506,     0,   395,     0,   532,    28,   507,    28,   508,
      28,   509,    28,   510,    28,   530,   572,     0,   394,   542,
     543,     0,   394,   546,   394,   394,   549,   403,     0,   394,
     404,    28,   531,    28,   534,    28,   537,     0,   538,     0,
     406,   556,   557,   558,    28,   539,    28,   552,   562,   563,
     564,    28,   553,    28,   554,     0,   569,    28,   555,   571,
       0,   394,    28,   560,    28,   561,    28,   566,    28,   567,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     456,     0,   457,     0,     0,   459,     0,     0,     0,     0,
       0,     0,     0,     0,   466,   467,   468,   469,   470,   471,
       0,     0,     0,   474,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   482,     0,     0,     0,     0,
     486,   487,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   511,     0,     0,     0,     0,     0,     0,
     517,   518,     0,     0,     0,   522,     0,     0,     0,     0,
     527,   528,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     541,     0,     0,   544,   545,     0,   547,   548,     0,     0,
     550,   551,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   565,     0,     0,     0,   568,     0,     0,
       0,     0,     0,   573,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,     0,    58,    59,    60,    61,    62,    63,
      64,    65,    66,     0,    67,     0,     0,     0,     0,    68,
      69,     0,    70,     0,     0,    71,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,     0,     0,     0,     0,    97,     0,     0,     0,     0,
       0,    98,   180,   183,   187,   189,   192,   194,   197,   200,
     203,   206,   209,   212,   217,   219,   221,   223,   225,   227,
     230,   233,   236,   239,   242,   245,   248,   251,   255,   258,
     261,   264,   266,   270,     0,   273,   276,   279,   281,   289,
     291,   293,   296,   300,   304,   307,   310,   313,   316,   319,
     322,   325,   328,   331,   334,   336,   338,   341,   344,   346,
       0,   348
};

static const yytype_int16 yycheck[] =
{
       1,     1,   439,     4,     5,    83,   148,    82,    83,   112,
     147,   114,    82,    91,     1,    83,    91,   454,    19,   122,
     123,   124,   125,   126,   166,    83,   163,    83,    29,    30,
      88,    83,    84,    89,    82,    83,   139,    89,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    83,    98,    84,    83,
      86,    83,    84,    87,    86,   106,   107,     0,     1,    91,
      92,    82,    83,    84,    82,   115,    91,   117,   555,   164,
      91,    83,    84,    87,    88,    83,    84,    89,    86,   129,
     130,    89,   108,    91,    92,    82,    83,   113,    82,    83,
      84,    82,    83,    83,    91,    89,     1,    91,    82,    83,
      91,   127,    83,    84,    91,    83,    84,    91,    89,    82,
      91,    89,    91,    91,   140,    83,    84,    83,    84,    83,
      84,    89,   173,    91,    82,    91,    33,    91,    83,    84,
     181,    83,    84,   184,    82,    83,    91,    80,    81,    82,
      83,    84,    85,    86,   195,    83,    84,   198,    91,    92,
     201,    89,    84,   204,    83,    84,   207,    83,    84,   210,
      89,    83,    84,    89,   215,    83,    84,    89,    83,    84,
      84,    89,    83,    84,    89,    83,    84,   228,    89,    25,
     231,    89,    84,   234,    83,    84,   237,    83,    84,   240,
      89,    82,    83,    89,    84,   246,    35,    36,   249,    25,
      83,    84,   569,    83,    84,   256,    89,   389,   259,    89,
     465,   262,   438,    83,    84,    83,    84,   162,   269,    89,
     271,    89,   145,   274,    83,    84,   277,    83,    84,    -1,
      89,    -1,    71,    89,    83,    84,    83,    84,    83,    84,
      89,    -1,    89,   294,    89,    -1,   297,   298,    -1,    83,
      84,   302,    83,    84,   305,    89,    -1,   308,    89,    -1,
     311,    83,    84,   314,    83,    84,   317,    89,    -1,   320,
      89,    -1,   323,    83,    84,    -1,    83,    84,    -1,    89,
      -1,   332,    89,    83,    84,    -1,    83,    84,   339,    89,
      -1,   342,    89,    83,    84,    83,    84,   392,    -1,    89,
      -1,    89,    82,    83,   354,    82,    83,    84,    83,    84,
     360,    82,    83,    84,    82,    83,    84,    83,    84,   369,
     370,   371,    82,    83,    84,   375,    83,    84,   378,   355,
     380,    82,    83,    84,    83,    84,   175,    83,    84,   178,
      82,    83,    84,    83,    84,   184,    83,    84,    83,    84,
      -1,   190,   402,    83,    84,   381,    83,    84,   409,    -1,
     455,    83,    84,   414,   415,   416,   417,   418,   419,    83,
      84,   422,    -1,   399,   400,   401,   215,    83,    84,   430,
     431,    -1,   477,   434,   435,   436,    83,    84,    83,    84,
      -1,   442,   443,   444,    -1,   446,   447,   448,    83,    84,
      83,    84,   453,    -1,   243,    83,    84,   246,    83,    84,
      83,    84,    83,    84,   253,    83,    84,   512,    83,    84,
      -1,   516,   472,    83,    84,   520,   521,    83,    84,    -1,
     269,   526,    83,    84,    83,    84,    83,    84,    83,    84,
      -1,   492,    -1,   494,   495,   496,   497,   498,   499,    -1,
     501,   502,   503,   504,   505,   506,   507,   508,    -1,   298,
      83,    84,    -1,   302,    -1,   515,    83,    84,    83,    84,
      83,    84,    83,    84,    83,    84,   571,    -1,   529,   530,
     531,    -1,   533,   534,   535,   536,   537,   326,    -1,   540,
     329,    83,    84,    83,    84,    83,    84,    -1,   524,    -1,
     339,   552,   553,   554,    83,    84,    83,    84,   558,   560,
     561,    83,    84,    83,    84,    -1,   567,    83,    84,   570,
      -1,   572,    83,    84,    83,    84,    83,    84,    83,    84,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     409,    -1,   411,    -1,    -1,   414,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   423,   424,   425,   426,   427,   428,
      -1,    -1,    -1,   432,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   444,    -1,    -1,    -1,    -1,
     449,   450,   451,   452,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   492,    -1,    -1,    -1,    -1,    -1,    -1,
     499,   500,    -1,    -1,    -1,   504,    -1,    -1,    -1,    -1,
     509,   510,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     529,    -1,    -1,   532,   533,    -1,   535,   536,    -1,    -1,
     539,   540,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   562,    -1,    -1,    -1,   566,    -1,    -1,
      -1,    -1,    -1,   572,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    33,    -1,    -1,    -1,    -1,    38,
      39,    -1,    41,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    -1,    -1,    -1,    -1,    84,    -1,    -1,    -1,    -1,
      -1,    90,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,    -1,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
      -1,   168
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    94,     0,     1,    80,    81,    82,    83,    84,    85,
      86,    91,    92,    95,    96,    98,    99,   100,   101,   103,
     104,   105,   106,    82,    96,    96,    82,    82,    83,    87,
      88,   102,    96,    91,    84,   104,   106,    96,    96,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    33,    38,    39,
      41,    44,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    84,    90,   119,
     105,    82,    96,    97,    97,    97,    88,    87,    96,    96,
      96,    96,    96,    96,    96,    96,    96,    96,    96,    96,
      96,    96,    96,    96,    96,    96,    96,    96,    96,    96,
      96,    96,    96,    96,    96,    96,    96,    96,    96,    96,
      96,    97,    96,    96,    96,    96,    96,    96,    96,    96,
      96,    96,    96,    96,    96,    96,    96,    96,    96,    96,
      96,    96,    96,    96,    96,    96,    96,     1,    96,    82,
      96,    96,    84,    89,    91,   118,   120,   166,   103,   121,
     166,    84,   126,   166,   111,   118,   127,   166,   121,   166,
     118,   135,   166,   121,   166,   103,   137,   166,    84,   153,
     166,   103,   122,   166,    84,   148,   166,    84,   138,   166,
      84,   141,   166,    84,    91,   112,   136,   166,   121,   166,
     121,   166,   121,   166,   121,   166,   121,   166,   118,   123,
     166,    84,   124,   166,   103,   125,   166,   103,   128,   166,
      84,   129,   166,    84,   130,   166,    84,   131,   166,    84,
     132,   166,    84,   117,   133,   166,    84,   134,   166,    84,
     139,   166,    84,   140,   166,   121,   166,   118,   142,   143,
     166,    84,   144,   166,    84,   145,   166,    84,   146,   166,
     148,   166,    84,    91,   107,   108,   109,   110,   149,   166,
     153,   166,   127,   166,    84,   150,   166,    84,   154,   155,
     166,    84,   116,   147,   166,    84,   151,   166,    84,   152,
     166,    84,   162,   166,    84,   156,   166,    84,   157,   166,
      84,   158,   166,    84,   159,   166,    84,   160,   166,    84,
     161,   166,    84,   163,   166,   144,   166,   153,   166,   116,
     164,   166,    84,   165,   166,   127,   166,    82,   166,     1,
      96,    91,    97,    97,    96,    96,    97,    97,    96,    96,
      96,    96,    96,    96,    91,    96,    97,    96,    96,    96,
      96,    96,    97,    96,    97,    96,    91,    97,    96,    96,
      96,    96,    97,    96,    96,    96,    82,    83,   115,    91,
      96,    96,    96,    97,    96,    97,    96,    96,    96,    96,
      96,    96,    96,    97,    97,    96,    97,    96,    82,    84,
      84,   103,   118,    84,   113,    84,   103,    84,    84,    84,
      84,    84,    84,    84,   103,   103,   103,    84,   103,    84,
     103,    84,   103,   118,    84,    84,    84,    82,    83,    84,
     114,   109,    84,    84,   116,    84,    84,    84,    84,   118,
     118,   118,   103,    84,    84,    96,    97,    97,    96,    97,
      96,    96,    96,    96,    96,    96,    97,    97,    97,    97,
      97,    97,    96,    96,    97,    96,    96,    96,   114,   115,
      96,    96,    97,    96,    96,    96,    97,    97,    97,    97,
      96,   115,   116,    84,    84,    84,    84,    84,    84,   112,
     103,    84,    84,    84,   116,    84,    84,    84,    84,    84,
      84,    97,    96,    96,    96,    96,    96,    97,    97,    96,
      96,    96,    97,    96,    96,    96,    96,    97,    97,   116,
      84,    84,   103,   116,    84,   116,   116,    84,   118,    84,
     116,    97,    96,    96,    97,    97,    96,    97,    97,    96,
      97,    97,    84,    84,    84,    84,    96,    96,    96,   115,
      84,    84,   103,    96,    96,    97,    84,    84,    97,    96,
     107,    96,   116,    97
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    93,    94,    94,    95,    95,    95,    95,    95,    95,
      95,    96,    96,    97,    97,    98,    99,    99,   100,   101,
     101,   102,   102,   102,   102,   102,   103,   103,   104,   104,
     104,   105,   105,   106,   106,   107,   107,   108,   108,   109,
     110,   110,   111,   111,   112,   112,   112,   112,   113,   113,
     114,   114,   114,   115,   115,   116,   116,   117,   117,   118,
     118,   118,   118,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   166,   166
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     2,     2,     1,     1,     1,
       2,     1,     2,     1,     2,     4,     4,     4,     3,     2,
       1,     0,     2,     2,     4,     4,     1,     1,     1,     1,
       2,     1,     1,     1,     3,     1,     1,     1,     2,     1,
       1,     3,     1,     3,     1,     1,     3,     3,     1,     3,
       2,     1,     2,     1,     2,     1,     3,     1,     3,     1,
       1,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,    14,     6,     4,     4,     4,     2,
       4,     4,     2,     2,     4,     4,     2,     6,     2,     2,
       4,     8,    12,     4,     8,     2,     1,     3,     8,     8,
       6,     2,    18,     2,    10,     8,     8,     8,     7,     4,
       2,     4,     4,     4,     4,     2,     2,     6,     6,     2,
       4,     6,     4,     3
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
        case 6:
#line 98 "zparser.y" /* yacc.c:1646  */
    {}
#line 1913 "zparser.c" /* yacc.c:1646  */
    break;

  case 7:
#line 100 "zparser.y" /* yacc.c:1646  */
    {
	    region_free_all(parser->rr_region);
	    parser->current_rr.type = 0;
	    parser->current_rr.rdata_count = 0;
	    parser->current_rr.rdatas = parser->temporary_rdatas;
	    parser->error_occurred = 0;
    }
#line 1925 "zparser.c" /* yacc.c:1646  */
    break;

  case 8:
#line 108 "zparser.y" /* yacc.c:1646  */
    {
	    region_free_all(parser->rr_region);
	    parser->current_rr.type = 0;
	    parser->current_rr.rdata_count = 0;
	    parser->current_rr.rdatas = parser->temporary_rdatas;
	    parser->error_occurred = 0;
    }
#line 1937 "zparser.c" /* yacc.c:1646  */
    break;

  case 9:
#line 116 "zparser.y" /* yacc.c:1646  */
    {	/* rr should be fully parsed */
	    if (!parser->error_occurred) {
			    parser->current_rr.rdatas
				    =(rdata_atom_type *)region_alloc_array_init(
					    parser->region,
					    parser->current_rr.rdatas,
					    parser->current_rr.rdata_count,
					    sizeof(rdata_atom_type));

			    process_rr();
	    }

	    region_free_all(parser->rr_region);

	    parser->current_rr.type = 0;
	    parser->current_rr.rdata_count = 0;
	    parser->current_rr.rdatas = parser->temporary_rdatas;
	    parser->error_occurred = 0;
    }
#line 1961 "zparser.c" /* yacc.c:1646  */
    break;

  case 15:
#line 148 "zparser.y" /* yacc.c:1646  */
    {
	    parser->default_ttl = zparser_ttl2int((yyvsp[-1].data).str, &(parser->error_occurred));
	    if (parser->error_occurred == 1) {
		    parser->default_ttl = DEFAULT_TTL;
			parser->error_occurred = 0;
	    }
    }
#line 1973 "zparser.c" /* yacc.c:1646  */
    break;

  case 16:
#line 158 "zparser.y" /* yacc.c:1646  */
    {
	    /* if previous origin is unused, remove it, do not leak it */
	    if(parser->origin != error_domain && parser->origin != (yyvsp[-1].domain)) {
		/* protect $3 from deletion, because deldomain walks up */
		(yyvsp[-1].domain)->usage ++;
	    	domain_table_deldomain(parser->db, parser->origin);
		(yyvsp[-1].domain)->usage --;
	    }
	    parser->origin = (yyvsp[-1].domain);
    }
#line 1988 "zparser.c" /* yacc.c:1646  */
    break;

  case 17:
#line 169 "zparser.y" /* yacc.c:1646  */
    {
	    zc_error_prev_line("$ORIGIN directive requires absolute domain name");
    }
#line 1996 "zparser.c" /* yacc.c:1646  */
    break;

  case 18:
#line 175 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.owner = (yyvsp[-2].domain);
	    parser->current_rr.type = (yyvsp[0].type);
    }
#line 2005 "zparser.c" /* yacc.c:1646  */
    break;

  case 19:
#line 182 "zparser.y" /* yacc.c:1646  */
    {
	    parser->prev_dname = (yyvsp[-1].domain);
	    (yyval.domain) = (yyvsp[-1].domain);
    }
#line 2014 "zparser.c" /* yacc.c:1646  */
    break;

  case 20:
#line 187 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.domain) = parser->prev_dname;
    }
#line 2022 "zparser.c" /* yacc.c:1646  */
    break;

  case 21:
#line 193 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = parser->default_ttl;
	    parser->current_rr.klass = parser->default_class;
    }
#line 2031 "zparser.c" /* yacc.c:1646  */
    break;

  case 22:
#line 198 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = parser->default_ttl;
	    parser->current_rr.klass = (yyvsp[-1].klass);
    }
#line 2040 "zparser.c" /* yacc.c:1646  */
    break;

  case 23:
#line 203 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = (yyvsp[-1].ttl);
	    parser->current_rr.klass = parser->default_class;
    }
#line 2049 "zparser.c" /* yacc.c:1646  */
    break;

  case 24:
#line 208 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = (yyvsp[-3].ttl);
	    parser->current_rr.klass = (yyvsp[-1].klass);
    }
#line 2058 "zparser.c" /* yacc.c:1646  */
    break;

  case 25:
#line 213 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = (yyvsp[-1].ttl);
	    parser->current_rr.klass = (yyvsp[-3].klass);
    }
#line 2067 "zparser.c" /* yacc.c:1646  */
    break;

  case 27:
#line 221 "zparser.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[0].dname) == error_dname) {
		    (yyval.domain) = error_domain;
	    } else if(parser->origin == error_domain) {
		    zc_error("cannot concatenate origin to domain name, because origin failed to parse");
		    (yyval.domain) = error_domain;
	    } else if ((yyvsp[0].dname)->name_size + domain_dname(parser->origin)->name_size - 1 > MAXDOMAINLEN) {
		    zc_error("domain name exceeds %d character limit", MAXDOMAINLEN);
		    (yyval.domain) = error_domain;
	    } else {
		    (yyval.domain) = domain_table_insert(
			    parser->db->domains,
			    dname_concatenate(
				    parser->rr_region,
				    (yyvsp[0].dname),
				    domain_dname(parser->origin)));
	    }
    }
#line 2090 "zparser.c" /* yacc.c:1646  */
    break;

  case 28:
#line 242 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.domain) = parser->db->domains->root;
    }
#line 2098 "zparser.c" /* yacc.c:1646  */
    break;

  case 29:
#line 246 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.domain) = parser->origin;
    }
#line 2106 "zparser.c" /* yacc.c:1646  */
    break;

  case 30:
#line 250 "zparser.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[-1].dname) != error_dname) {
		    (yyval.domain) = domain_table_insert(parser->db->domains, (yyvsp[-1].dname));
	    } else {
		    (yyval.domain) = error_domain;
	    }
    }
#line 2118 "zparser.c" /* yacc.c:1646  */
    break;

  case 31:
#line 260 "zparser.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[0].data).len > MAXLABELLEN) {
		    zc_error("label exceeds %d character limit", MAXLABELLEN);
		    (yyval.dname) = error_dname;
	    } else if ((yyvsp[0].data).len <= 0) {
		    zc_error("zero label length");
		    (yyval.dname) = error_dname;
	    } else {
		    (yyval.dname) = dname_make_from_label(parser->rr_region,
					       (uint8_t *) (yyvsp[0].data).str,
					       (yyvsp[0].data).len);
	    }
    }
#line 2136 "zparser.c" /* yacc.c:1646  */
    break;

  case 32:
#line 274 "zparser.y" /* yacc.c:1646  */
    {
	    zc_error("bitlabels are now deprecated. RFC2673 is obsoleted.");
	    (yyval.dname) = error_dname;
    }
#line 2145 "zparser.c" /* yacc.c:1646  */
    break;

  case 34:
#line 282 "zparser.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[-2].dname) == error_dname || (yyvsp[0].dname) == error_dname) {
		    (yyval.dname) = error_dname;
	    } else if ((yyvsp[-2].dname)->name_size + (yyvsp[0].dname)->name_size - 1 > MAXDOMAINLEN) {
		    zc_error("domain name exceeds %d character limit",
			     MAXDOMAINLEN);
		    (yyval.dname) = error_dname;
	    } else {
		    (yyval.dname) = dname_concatenate(parser->rr_region, (yyvsp[-2].dname), (yyvsp[0].dname));
	    }
    }
#line 2161 "zparser.c" /* yacc.c:1646  */
    break;

  case 37:
#line 304 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region, 2);
	    result[0] = 0;
	    result[1] = '\0';
	    (yyval.data).str = result;
	    (yyval.data).len = 1;
    }
#line 2173 "zparser.c" /* yacc.c:1646  */
    break;

  case 38:
#line 312 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-1].data).len + 2);
	    memcpy(result, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
	    result[(yyvsp[-1].data).len] = 0;
	    result[(yyvsp[-1].data).len+1] = '\0';
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-1].data).len + 1;
    }
#line 2187 "zparser.c" /* yacc.c:1646  */
    break;

  case 39:
#line 324 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[0].data).len + 1);

	    if ((yyvsp[0].data).len > MAXLABELLEN)
		    zc_error("label exceeds %d character limit", MAXLABELLEN);

	    /* make label anyway */
	    result[0] = (yyvsp[0].data).len;
	    memcpy(result+1, (yyvsp[0].data).str, (yyvsp[0].data).len);

	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[0].data).len + 1;
    }
#line 2206 "zparser.c" /* yacc.c:1646  */
    break;

  case 41:
#line 342 "zparser.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[-2].data).len + (yyvsp[0].data).len - 3 > MAXDOMAINLEN)
		    zc_error("domain name exceeds %d character limit",
			     MAXDOMAINLEN);

	    /* make dname anyway */
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len + 1);
	    memcpy((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2223 "zparser.c" /* yacc.c:1646  */
    break;

  case 42:
#line 357 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 1);
    }
#line 2231 "zparser.c" /* yacc.c:1646  */
    break;

  case 43:
#line 361 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 0);
    }
#line 2239 "zparser.c" /* yacc.c:1646  */
    break;

  case 45:
#line 372 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.data).len = 1;
	    (yyval.data).str = region_strdup(parser->rr_region, ".");
    }
#line 2248 "zparser.c" /* yacc.c:1646  */
    break;

  case 46:
#line 377 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len + 1;
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len + 1);
	    memcpy((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len, " ", 1);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len + 1, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2261 "zparser.c" /* yacc.c:1646  */
    break;

  case 47:
#line 386 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len + 1;
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len + 1);
	    memcpy((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len, ".", 1);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len + 1, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2274 "zparser.c" /* yacc.c:1646  */
    break;

  case 48:
#line 398 "zparser.y" /* yacc.c:1646  */
    {
	    uint16_t type = rrtype_from_string((yyvsp[0].data).str);
	    if (type != 0 && type < 128) {
		    set_bit(nxtbits, type);
	    } else {
		    zc_error("bad type %d in NXT record", (int) type);
	    }
    }
#line 2287 "zparser.c" /* yacc.c:1646  */
    break;

  case 49:
#line 407 "zparser.y" /* yacc.c:1646  */
    {
	    uint16_t type = rrtype_from_string((yyvsp[0].data).str);
	    if (type != 0 && type < 128) {
		    set_bit(nxtbits, type);
	    } else {
		    zc_error("bad type %d in NXT record", (int) type);
	    }
    }
#line 2300 "zparser.c" /* yacc.c:1646  */
    break;

  case 50:
#line 418 "zparser.y" /* yacc.c:1646  */
    {
    }
#line 2307 "zparser.c" /* yacc.c:1646  */
    break;

  case 51:
#line 421 "zparser.y" /* yacc.c:1646  */
    {
    }
#line 2314 "zparser.c" /* yacc.c:1646  */
    break;

  case 52:
#line 424 "zparser.y" /* yacc.c:1646  */
    {
	    uint16_t type = rrtype_from_string((yyvsp[-1].data).str);
	    if (type != 0) {
                    if (type > nsec_highest_rcode) {
                            nsec_highest_rcode = type;
                    }
		    set_bitnsec(nsecbits, type);
	    } else {
		    zc_error("bad type %d in NSEC record", (int) type);
	    }
    }
#line 2330 "zparser.c" /* yacc.c:1646  */
    break;

  case 56:
#line 447 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-2].data).len + (yyvsp[0].data).len + 1);
	    memcpy(result, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy(result + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2344 "zparser.c" /* yacc.c:1646  */
    break;

  case 58:
#line 464 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-2].data).len + (yyvsp[0].data).len + 1);
	    memcpy(result, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy(result + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2358 "zparser.c" /* yacc.c:1646  */
    break;

  case 60:
#line 480 "zparser.y" /* yacc.c:1646  */
    {
	(yyval.data).str = ".";
	(yyval.data).len = 1;
    }
#line 2367 "zparser.c" /* yacc.c:1646  */
    break;

  case 61:
#line 485 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-1].data).len + 2);
	    memcpy(result, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
	    result[(yyvsp[-1].data).len] = '.';
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-1].data).len + 1;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2381 "zparser.c" /* yacc.c:1646  */
    break;

  case 62:
#line 495 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-2].data).len + (yyvsp[0].data).len + 2);
	    memcpy(result, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    result[(yyvsp[-2].data).len] = '.';
	    memcpy(result + (yyvsp[-2].data).len + 1, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len + 1;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2396 "zparser.c" /* yacc.c:1646  */
    break;

  case 64:
#line 513 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2402 "zparser.c" /* yacc.c:1646  */
    break;

  case 66:
#line 515 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2408 "zparser.c" /* yacc.c:1646  */
    break;

  case 67:
#line 516 "zparser.y" /* yacc.c:1646  */
    { zc_warning_prev_line("MD is obsolete"); }
#line 2414 "zparser.c" /* yacc.c:1646  */
    break;

  case 68:
#line 518 "zparser.y" /* yacc.c:1646  */
    {
	    zc_warning_prev_line("MD is obsolete");
	    (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2423 "zparser.c" /* yacc.c:1646  */
    break;

  case 69:
#line 522 "zparser.y" /* yacc.c:1646  */
    { zc_warning_prev_line("MF is obsolete"); }
#line 2429 "zparser.c" /* yacc.c:1646  */
    break;

  case 70:
#line 524 "zparser.y" /* yacc.c:1646  */
    {
	    zc_warning_prev_line("MF is obsolete");
	    (yyval.type) = (yyvsp[-2].type);
	    parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2439 "zparser.c" /* yacc.c:1646  */
    break;

  case 72:
#line 530 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2445 "zparser.c" /* yacc.c:1646  */
    break;

  case 74:
#line 532 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2451 "zparser.c" /* yacc.c:1646  */
    break;

  case 75:
#line 533 "zparser.y" /* yacc.c:1646  */
    { zc_warning_prev_line("MB is obsolete"); }
#line 2457 "zparser.c" /* yacc.c:1646  */
    break;

  case 76:
#line 535 "zparser.y" /* yacc.c:1646  */
    {
	    zc_warning_prev_line("MB is obsolete");
	    (yyval.type) = (yyvsp[-2].type);
	    parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2467 "zparser.c" /* yacc.c:1646  */
    break;

  case 78:
#line 541 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2473 "zparser.c" /* yacc.c:1646  */
    break;

  case 80:
#line 543 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2479 "zparser.c" /* yacc.c:1646  */
    break;

  case 82:
#line 546 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2485 "zparser.c" /* yacc.c:1646  */
    break;

  case 84:
#line 548 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2491 "zparser.c" /* yacc.c:1646  */
    break;

  case 86:
#line 550 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2497 "zparser.c" /* yacc.c:1646  */
    break;

  case 88:
#line 552 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2503 "zparser.c" /* yacc.c:1646  */
    break;

  case 90:
#line 554 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2509 "zparser.c" /* yacc.c:1646  */
    break;

  case 92:
#line 556 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2515 "zparser.c" /* yacc.c:1646  */
    break;

  case 94:
#line 558 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2521 "zparser.c" /* yacc.c:1646  */
    break;

  case 96:
#line 560 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2527 "zparser.c" /* yacc.c:1646  */
    break;

  case 98:
#line 562 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2533 "zparser.c" /* yacc.c:1646  */
    break;

  case 100:
#line 564 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2539 "zparser.c" /* yacc.c:1646  */
    break;

  case 102:
#line 566 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2545 "zparser.c" /* yacc.c:1646  */
    break;

  case 104:
#line 568 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2551 "zparser.c" /* yacc.c:1646  */
    break;

  case 106:
#line 570 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2557 "zparser.c" /* yacc.c:1646  */
    break;

  case 108:
#line 572 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2563 "zparser.c" /* yacc.c:1646  */
    break;

  case 110:
#line 574 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2569 "zparser.c" /* yacc.c:1646  */
    break;

  case 112:
#line 576 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2575 "zparser.c" /* yacc.c:1646  */
    break;

  case 114:
#line 578 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2581 "zparser.c" /* yacc.c:1646  */
    break;

  case 116:
#line 580 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2587 "zparser.c" /* yacc.c:1646  */
    break;

  case 118:
#line 582 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2593 "zparser.c" /* yacc.c:1646  */
    break;

  case 120:
#line 584 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2599 "zparser.c" /* yacc.c:1646  */
    break;

  case 122:
#line 586 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2605 "zparser.c" /* yacc.c:1646  */
    break;

  case 124:
#line 588 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2611 "zparser.c" /* yacc.c:1646  */
    break;

  case 126:
#line 590 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2617 "zparser.c" /* yacc.c:1646  */
    break;

  case 128:
#line 592 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2623 "zparser.c" /* yacc.c:1646  */
    break;

  case 130:
#line 594 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2629 "zparser.c" /* yacc.c:1646  */
    break;

  case 132:
#line 596 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2635 "zparser.c" /* yacc.c:1646  */
    break;

  case 134:
#line 598 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2641 "zparser.c" /* yacc.c:1646  */
    break;

  case 137:
#line 601 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2647 "zparser.c" /* yacc.c:1646  */
    break;

  case 139:
#line 603 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2653 "zparser.c" /* yacc.c:1646  */
    break;

  case 140:
#line 604 "zparser.y" /* yacc.c:1646  */
    { if (dlv_warn) { dlv_warn = 0; zc_warning_prev_line("DLV is experimental"); } }
#line 2659 "zparser.c" /* yacc.c:1646  */
    break;

  case 141:
#line 605 "zparser.y" /* yacc.c:1646  */
    { if (dlv_warn) { dlv_warn = 0; zc_warning_prev_line("DLV is experimental"); } (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2665 "zparser.c" /* yacc.c:1646  */
    break;

  case 143:
#line 607 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2671 "zparser.c" /* yacc.c:1646  */
    break;

  case 145:
#line 609 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2677 "zparser.c" /* yacc.c:1646  */
    break;

  case 147:
#line 611 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2683 "zparser.c" /* yacc.c:1646  */
    break;

  case 149:
#line 613 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2689 "zparser.c" /* yacc.c:1646  */
    break;

  case 151:
#line 615 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2695 "zparser.c" /* yacc.c:1646  */
    break;

  case 153:
#line 617 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2701 "zparser.c" /* yacc.c:1646  */
    break;

  case 155:
#line 619 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2707 "zparser.c" /* yacc.c:1646  */
    break;

  case 157:
#line 621 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2713 "zparser.c" /* yacc.c:1646  */
    break;

  case 159:
#line 623 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2719 "zparser.c" /* yacc.c:1646  */
    break;

  case 161:
#line 625 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2725 "zparser.c" /* yacc.c:1646  */
    break;

  case 163:
#line 627 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2731 "zparser.c" /* yacc.c:1646  */
    break;

  case 165:
#line 629 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2737 "zparser.c" /* yacc.c:1646  */
    break;

  case 167:
#line 631 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2743 "zparser.c" /* yacc.c:1646  */
    break;

  case 169:
#line 633 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2749 "zparser.c" /* yacc.c:1646  */
    break;

  case 171:
#line 635 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2755 "zparser.c" /* yacc.c:1646  */
    break;

  case 173:
#line 637 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2761 "zparser.c" /* yacc.c:1646  */
    break;

  case 175:
#line 639 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2767 "zparser.c" /* yacc.c:1646  */
    break;

  case 177:
#line 641 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2773 "zparser.c" /* yacc.c:1646  */
    break;

  case 179:
#line 643 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2779 "zparser.c" /* yacc.c:1646  */
    break;

  case 180:
#line 644 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2785 "zparser.c" /* yacc.c:1646  */
    break;

  case 181:
#line 646 "zparser.y" /* yacc.c:1646  */
    {
	    zc_error_prev_line("unrecognized RR type '%s'", (yyvsp[-2].data).str);
    }
#line 2793 "zparser.c" /* yacc.c:1646  */
    break;

  case 182:
#line 658 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-1].data).str));
    }
#line 2801 "zparser.c" /* yacc.c:1646  */
    break;

  case 183:
#line 664 "zparser.y" /* yacc.c:1646  */
    {
	    /* convert a single dname record */
	    zadd_rdata_domain((yyvsp[-1].domain));
    }
#line 2810 "zparser.c" /* yacc.c:1646  */
    break;

  case 184:
#line 671 "zparser.y" /* yacc.c:1646  */
    {
	    /* convert the soa data */
	    zadd_rdata_domain((yyvsp[-13].domain));	/* prim. ns */
	    zadd_rdata_domain((yyvsp[-11].domain));	/* email */
	    zadd_rdata_wireformat(zparser_conv_serial(parser->region, (yyvsp[-9].data).str)); /* serial */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-7].data).str)); /* refresh */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-5].data).str)); /* retry */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-3].data).str)); /* expire */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-1].data).str)); /* minimum */
    }
#line 2825 "zparser.c" /* yacc.c:1646  */
    break;

  case 185:
#line 684 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-5].data).str)); /* address */
	    zadd_rdata_wireformat(zparser_conv_services(parser->region, (yyvsp[-3].data).str, (yyvsp[-1].data).str)); /* protocol and services */
    }
#line 2834 "zparser.c" /* yacc.c:1646  */
    break;

  case 186:
#line 691 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* CPU */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* OS*/
    }
#line 2843 "zparser.c" /* yacc.c:1646  */
    break;

  case 187:
#line 698 "zparser.y" /* yacc.c:1646  */
    {
	    /* convert a single dname record */
	    zadd_rdata_domain((yyvsp[-3].domain));
	    zadd_rdata_domain((yyvsp[-1].domain));
    }
#line 2853 "zparser.c" /* yacc.c:1646  */
    break;

  case 188:
#line 706 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* priority */
	    zadd_rdata_domain((yyvsp[-1].domain));	/* MX host */
    }
#line 2862 "zparser.c" /* yacc.c:1646  */
    break;

  case 189:
#line 713 "zparser.y" /* yacc.c:1646  */
    {
	zadd_rdata_txt_clean_wireformat();
    }
#line 2870 "zparser.c" /* yacc.c:1646  */
    break;

  case 190:
#line 720 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_domain((yyvsp[-3].domain)); /* mbox d-name */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* txt d-name */
    }
#line 2879 "zparser.c" /* yacc.c:1646  */
    break;

  case 191:
#line 728 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* subtype */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* domain name */
    }
#line 2888 "zparser.c" /* yacc.c:1646  */
    break;

  case 192:
#line 736 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* X.25 address. */
    }
#line 2896 "zparser.c" /* yacc.c:1646  */
    break;

  case 193:
#line 743 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* address */
    }
#line 2904 "zparser.c" /* yacc.c:1646  */
    break;

  case 194:
#line 747 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* address */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* sub-address */
    }
#line 2913 "zparser.c" /* yacc.c:1646  */
    break;

  case 195:
#line 755 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* intermediate host */
    }
#line 2922 "zparser.c" /* yacc.c:1646  */
    break;

  case 196:
#line 763 "zparser.y" /* yacc.c:1646  */
    {
	    /* String must start with "0x" or "0X".	 */
	    if (strncasecmp((yyvsp[-1].data).str, "0x", 2) != 0) {
		    zc_error_prev_line("NSAP rdata must start with '0x'");
	    } else {
		    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str + 2, (yyvsp[-1].data).len - 2)); /* NSAP */
	    }
    }
#line 2935 "zparser.c" /* yacc.c:1646  */
    break;

  case 197:
#line 775 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-3].domain)); /* MAP822 */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* MAPX400 */
    }
#line 2945 "zparser.c" /* yacc.c:1646  */
    break;

  case 198:
#line 783 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_aaaa(parser->region, (yyvsp[-1].data).str));  /* IPv6 address */
    }
#line 2953 "zparser.c" /* yacc.c:1646  */
    break;

  case 199:
#line 789 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_loc(parser->region, (yyvsp[-1].data).str)); /* Location */
    }
#line 2961 "zparser.c" /* yacc.c:1646  */
    break;

  case 200:
#line 795 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_domain((yyvsp[-3].domain)); /* nxt name */
	    zadd_rdata_wireformat(zparser_conv_nxt(parser->region, nxtbits)); /* nxt bitlist */
	    memset(nxtbits, 0, sizeof(nxtbits));
    }
#line 2971 "zparser.c" /* yacc.c:1646  */
    break;

  case 201:
#line 803 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* prio */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* weight */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* port */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* target name */
    }
#line 2982 "zparser.c" /* yacc.c:1646  */
    break;

  case 202:
#line 813 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-11].data).str)); /* order */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-9].data).str)); /* preference */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-7].data).str, (yyvsp[-7].data).len)); /* flags */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-5].data).str, (yyvsp[-5].data).len)); /* service */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* regexp */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* target name */
    }
#line 2995 "zparser.c" /* yacc.c:1646  */
    break;

  case 203:
#line 825 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* exchanger */
    }
#line 3004 "zparser.c" /* yacc.c:1646  */
    break;

  case 204:
#line 833 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_certificate_type(parser->region, (yyvsp[-7].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* key tag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-3].data).str)); /* algorithm */
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* certificate or CRL */
    }
#line 3015 "zparser.c" /* yacc.c:1646  */
    break;

  case 206:
#line 846 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_apl_rdata(parser->region, (yyvsp[0].data).str));
    }
#line 3023 "zparser.c" /* yacc.c:1646  */
    break;

  case 207:
#line 850 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_apl_rdata(parser->region, (yyvsp[0].data).str));
    }
#line 3031 "zparser.c" /* yacc.c:1646  */
    break;

  case 208:
#line 856 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* keytag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
    }
#line 3042 "zparser.c" /* yacc.c:1646  */
    break;

  case 209:
#line 865 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* keytag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
    }
#line 3053 "zparser.c" /* yacc.c:1646  */
    break;

  case 210:
#line 874 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* fp type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
    }
#line 3063 "zparser.c" /* yacc.c:1646  */
    break;

  case 211:
#line 882 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* data blob */
    }
#line 3071 "zparser.c" /* yacc.c:1646  */
    break;

  case 212:
#line 888 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_rrtype(parser->region, (yyvsp[-17].data).str)); /* rr covered */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-15].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-13].data).str)); /* # labels */
	    zadd_rdata_wireformat(zparser_conv_period(parser->region, (yyvsp[-11].data).str)); /* # orig TTL */
	    zadd_rdata_wireformat(zparser_conv_time(parser->region, (yyvsp[-9].data).str)); /* sig exp */
	    zadd_rdata_wireformat(zparser_conv_time(parser->region, (yyvsp[-7].data).str)); /* sig inc */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* key id */
	    zadd_rdata_wireformat(zparser_conv_dns_name(parser->region, 
				(const uint8_t*) (yyvsp[-3].data).str,(yyvsp[-3].data).len)); /* sig name */
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* sig data */
    }
#line 3088 "zparser.c" /* yacc.c:1646  */
    break;

  case 213:
#line 903 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_dns_name(parser->region, 
				(const uint8_t*) (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* nsec name */
	    zadd_rdata_wireformat(zparser_conv_nsec(parser->region, nsecbits)); /* nsec bitlist */
	    memset(nsecbits, 0, sizeof(nsecbits));
            nsec_highest_rcode = 0;
    }
#line 3100 "zparser.c" /* yacc.c:1646  */
    break;

  case 214:
#line 913 "zparser.y" /* yacc.c:1646  */
    {
#ifdef NSEC3
	    nsec3_add_params((yyvsp[-9].data).str, (yyvsp[-7].data).str, (yyvsp[-5].data).str, (yyvsp[-3].data).str, (yyvsp[-3].data).len);

	    zadd_rdata_wireformat(zparser_conv_b32(parser->region, (yyvsp[-1].data).str)); /* next hashed name */
	    zadd_rdata_wireformat(zparser_conv_nsec(parser->region, nsecbits)); /* nsec bitlist */
	    memset(nsecbits, 0, sizeof(nsecbits));
	    nsec_highest_rcode = 0;
#else
	    zc_error_prev_line("nsec3 not supported");
#endif /* NSEC3 */
    }
#line 3117 "zparser.c" /* yacc.c:1646  */
    break;

  case 215:
#line 928 "zparser.y" /* yacc.c:1646  */
    {
#ifdef NSEC3
	    nsec3_add_params((yyvsp[-7].data).str, (yyvsp[-5].data).str, (yyvsp[-3].data).str, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
#else
	    zc_error_prev_line("nsec3 not supported");
#endif /* NSEC3 */
    }
#line 3129 "zparser.c" /* yacc.c:1646  */
    break;

  case 216:
#line 938 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-7].data).str)); /* usage */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* selector */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* matching type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* ca data */
    }
#line 3140 "zparser.c" /* yacc.c:1646  */
    break;

  case 217:
#line 947 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* flags */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* proto */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-3].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* hash */
    }
#line 3151 "zparser.c" /* yacc.c:1646  */
    break;

  case 218:
#line 956 "zparser.y" /* yacc.c:1646  */
    {
	    const dname_type* name = 0;
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-6].data).str)); /* precedence */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-4].data).str)); /* gateway type */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-2].data).str)); /* algorithm */
	    switch(atoi((yyvsp[-4].data).str)) {
		case IPSECKEY_NOGATEWAY: 
			zadd_rdata_wireformat(alloc_rdata_init(parser->region, "", 0));
			break;
		case IPSECKEY_IP4:
			zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[0].data).str));
			break;
		case IPSECKEY_IP6:
			zadd_rdata_wireformat(zparser_conv_aaaa(parser->region, (yyvsp[0].data).str));
			break;
		case IPSECKEY_DNAME:
			/* convert and insert the dname */
			if(strlen((yyvsp[0].data).str) == 0)
				zc_error_prev_line("IPSECKEY must specify gateway name");
			if(!(name = dname_parse(parser->region, (yyvsp[0].data).str)))
				zc_error_prev_line("IPSECKEY bad gateway dname %s", (yyvsp[0].data).str);
			if((yyvsp[0].data).str[strlen((yyvsp[0].data).str)-1] != '.') {
				if(parser->origin == error_domain) {
		    			zc_error("cannot concatenate origin to domain name, because origin failed to parse");
					break;
				}
				name = dname_concatenate(parser->rr_region, name, 
					domain_dname(parser->origin));
			}
			zadd_rdata_wireformat(alloc_rdata_init(parser->region,
				dname_name(name), name->name_size));
			break;
		default:
			zc_error_prev_line("unknown IPSECKEY gateway type");
	    }
    }
#line 3192 "zparser.c" /* yacc.c:1646  */
    break;

  case 219:
#line 995 "zparser.y" /* yacc.c:1646  */
    {
	   zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* public key */
    }
#line 3200 "zparser.c" /* yacc.c:1646  */
    break;

  case 221:
#line 1003 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_ilnp64(parser->region, (yyvsp[-1].data).str));  /* NodeID */
    }
#line 3209 "zparser.c" /* yacc.c:1646  */
    break;

  case 222:
#line 1010 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-1].data).str));  /* Locator32 */
    }
#line 3218 "zparser.c" /* yacc.c:1646  */
    break;

  case 223:
#line 1017 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_ilnp64(parser->region, (yyvsp[-1].data).str));  /* Locator64 */
    }
#line 3227 "zparser.c" /* yacc.c:1646  */
    break;

  case 224:
#line 1024 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain));  /* FQDN */
    }
#line 3236 "zparser.c" /* yacc.c:1646  */
    break;

  case 225:
#line 1031 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_eui(parser->region, (yyvsp[-1].data).str, 48));
    }
#line 3244 "zparser.c" /* yacc.c:1646  */
    break;

  case 226:
#line 1037 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_eui(parser->region, (yyvsp[-1].data).str, 64));
    }
#line 3252 "zparser.c" /* yacc.c:1646  */
    break;

  case 227:
#line 1044 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* priority */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* weight */
	    zadd_rdata_wireformat(zparser_conv_long_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* target */
    }
#line 3262 "zparser.c" /* yacc.c:1646  */
    break;

  case 228:
#line 1053 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* Flags */
	    zadd_rdata_wireformat(zparser_conv_tag(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* Tag */
	    zadd_rdata_wireformat(zparser_conv_long_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* Value */
    }
#line 3272 "zparser.c" /* yacc.c:1646  */
    break;

  case 229:
#line 1062 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str));
    }
#line 3280 "zparser.c" /* yacc.c:1646  */
    break;

  case 230:
#line 1069 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_serial(parser->region, (yyvsp[-3].data).str));
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-1].data).str));
	    zadd_rdata_wireformat(zparser_conv_nsec(parser->region, nsecbits)); /* nsec bitlist */
	    memset(nsecbits, 0, sizeof(nsecbits));
            nsec_highest_rcode = 0;
    }
#line 3292 "zparser.c" /* yacc.c:1646  */
    break;

  case 231:
#line 1079 "zparser.y" /* yacc.c:1646  */
    {
	    /* $2 is the number of octets, currently ignored */
	    (yyval.unknown) = zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len);

    }
#line 3302 "zparser.c" /* yacc.c:1646  */
    break;

  case 232:
#line 1085 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.unknown) = zparser_conv_hex(parser->region, "", 0);
    }
#line 3310 "zparser.c" /* yacc.c:1646  */
    break;

  case 233:
#line 1089 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.unknown) = zparser_conv_hex(parser->region, "", 0);
    }
#line 3318 "zparser.c" /* yacc.c:1646  */
    break;


#line 3322 "zparser.c" /* yacc.c:1646  */
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
#line 1093 "zparser.y" /* yacc.c:1906  */


int
yywrap(void)
{
	return 1;
}

/*
 * Create the parser.
 */
zparser_type *
zparser_create(region_type *region, region_type *rr_region, namedb_type *db)
{
	zparser_type *result;

	result = (zparser_type *) region_alloc(region, sizeof(zparser_type));
	result->region = region;
	result->rr_region = rr_region;
	result->db = db;

	result->filename = NULL;
	result->current_zone = NULL;
	result->origin = NULL;
	result->prev_dname = NULL;
	result->default_apex = NULL;

	result->temporary_rdatas = (rdata_atom_type *) region_alloc_array(
		result->region, MAXRDATALEN, sizeof(rdata_atom_type));

	return result;
}

/*
 * Initialize the parser for a new zone file.
 */
void
zparser_init(const char *filename, uint32_t ttl, uint16_t klass,
	     const dname_type *origin)
{
	memset(nxtbits, 0, sizeof(nxtbits));
	memset(nsecbits, 0, sizeof(nsecbits));
        nsec_highest_rcode = 0;

	parser->default_ttl = ttl;
	parser->default_class = klass;
	parser->current_zone = NULL;
	parser->origin = domain_table_insert(parser->db->domains, origin);
	parser->prev_dname = parser->origin;
	parser->default_apex = parser->origin;
	parser->error_occurred = 0;
	parser->errors = 0;
	parser->line = 1;
	parser->filename = filename;
	parser->current_rr.rdata_count = 0;
	parser->current_rr.rdatas = parser->temporary_rdatas;
}

void
yyerror(const char *message)
{
	zc_error("%s", message);
}

static void
error_va_list(unsigned line, const char *fmt, va_list args)
{
	if (parser->filename) {
		char message[MAXSYSLOGMSGLEN];
		vsnprintf(message, sizeof(message), fmt, args);
		log_msg(LOG_ERR, "%s:%u: %s", parser->filename, line, message);
	}
	else log_vmsg(LOG_ERR, fmt, args);

	++parser->errors;
	parser->error_occurred = 1;
}

/* the line counting sux, to say the least
 * with this grose hack we try do give sane
 * numbers back */
void
zc_error_prev_line(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	error_va_list(parser->line - 1, fmt, args);
	va_end(args);
}

void
zc_error(const char *fmt, ...)
{
	/* send an error message to stderr */
	va_list args;
	va_start(args, fmt);
	error_va_list(parser->line, fmt, args);
	va_end(args);
}

static void
warning_va_list(unsigned line, const char *fmt, va_list args)
{
	if (parser->filename) {
		char m[MAXSYSLOGMSGLEN];
		vsnprintf(m, sizeof(m), fmt, args);
		log_msg(LOG_WARNING, "%s:%u: %s", parser->filename, line, m);
	}
	else log_vmsg(LOG_WARNING, fmt, args);
}

void
zc_warning_prev_line(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	warning_va_list(parser->line - 1, fmt, args);
	va_end(args);
}

void
zc_warning(const char *fmt, ... )
{
	va_list args;
	va_start(args, fmt);
	warning_va_list(parser->line, fmt, args);
	va_end(args);
}

#ifdef NSEC3
static void
nsec3_add_params(const char* hashalgo_str, const char* flag_str,
	const char* iter_str, const char* salt_str, int salt_len)
{
	zadd_rdata_wireformat(zparser_conv_byte(parser->region, hashalgo_str));
	zadd_rdata_wireformat(zparser_conv_byte(parser->region, flag_str));
	zadd_rdata_wireformat(zparser_conv_short(parser->region, iter_str));

	/* salt */
	if(strcmp(salt_str, "-") != 0) 
		zadd_rdata_wireformat(zparser_conv_hex_length(parser->region, 
			salt_str, salt_len)); 
	else 
		zadd_rdata_wireformat(alloc_rdata_init(parser->region, "", 1));
}
#endif /* NSEC3 */
