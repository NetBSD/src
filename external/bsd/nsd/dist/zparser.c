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
    T_SMIMEA = 335,
    DOLLAR_TTL = 336,
    DOLLAR_ORIGIN = 337,
    NL = 338,
    SP = 339,
    STR = 340,
    PREV = 341,
    BITLAB = 342,
    T_TTL = 343,
    T_RRCLASS = 344,
    URR = 345,
    T_UTYPE = 346
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
#define T_SMIMEA 335
#define DOLLAR_TTL 336
#define DOLLAR_ORIGIN 337
#define NL 338
#define SP 339
#define STR 340
#define PREV 341
#define BITLAB 342
#define T_TTL 343
#define T_RRCLASS 344
#define URR 345
#define T_UTYPE 346

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

#line 348 "zparser.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_ZPARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 365 "zparser.c" /* yacc.c:358  */

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
#define YYLAST   961

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  94
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  75
/* YYNRULES -- Number of rules.  */
#define YYNRULES  237
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  587

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   346

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
       2,     2,     2,     2,     2,     2,    92,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    93,     2,     2,     2,     2,     2,
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
      85,    86,    87,    88,    89,    90,    91
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    92,    92,    93,    96,    97,    98,    99,   107,   115,
     135,   139,   140,   143,   144,   147,   157,   168,   174,   181,
     186,   193,   197,   202,   207,   212,   219,   220,   241,   245,
     249,   259,   273,   280,   281,   299,   300,   324,   331,   343,
     355,   372,   373,   385,   389,   399,   400,   405,   414,   426,
     435,   446,   449,   452,   466,   467,   474,   475,   491,   492,
     507,   508,   513,   523,   541,   542,   543,   544,   545,   546,
     551,   552,   558,   559,   560,   561,   562,   563,   569,   570,
     571,   572,   574,   575,   576,   577,   578,   579,   580,   581,
     582,   583,   584,   585,   586,   587,   588,   589,   590,   591,
     592,   593,   594,   595,   596,   597,   598,   599,   600,   601,
     602,   603,   604,   605,   606,   607,   608,   609,   610,   611,
     612,   613,   614,   615,   616,   617,   618,   619,   620,   621,
     622,   623,   624,   625,   626,   627,   628,   629,   630,   631,
     632,   633,   634,   635,   636,   637,   638,   639,   640,   641,
     642,   643,   644,   645,   646,   647,   648,   649,   650,   651,
     652,   653,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,   664,   665,   666,   667,   668,   669,   670,   671,
     672,   673,   674,   675,   676,   688,   694,   701,   714,   721,
     728,   736,   743,   750,   758,   766,   773,   777,   785,   793,
     805,   813,   819,   825,   833,   843,   855,   863,   873,   876,
     880,   886,   895,   904,   912,   918,   933,   943,   958,   968,
     977,   986,   995,  1036,  1040,  1044,  1051,  1058,  1065,  1072,
    1078,  1085,  1094,  1103,  1110,  1120,  1126,  1130
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
  "T_CDNSKEY", "T_OPENPGPKEY", "T_CSYNC", "T_AVC", "T_SMIMEA",
  "DOLLAR_TTL", "DOLLAR_ORIGIN", "NL", "SP", "STR", "PREV", "BITLAB",
  "T_TTL", "T_RRCLASS", "URR", "T_UTYPE", "'.'", "'@'", "$accept", "lines",
  "line", "sp", "trail", "ttl_directive", "origin_directive", "rr",
  "owner", "classttl", "dname", "abs_dname", "label", "rel_dname",
  "wire_dname", "wire_abs_dname", "wire_label", "wire_rel_dname",
  "str_seq", "concatenated_str_seq", "nxt_seq", "nsec_more", "nsec_seq",
  "str_sp_seq", "str_dot_seq", "dotted_str", "type_and_rdata", "rdata_a",
  "rdata_domain_name", "rdata_soa", "rdata_wks", "rdata_hinfo",
  "rdata_minfo", "rdata_mx", "rdata_txt", "rdata_rp", "rdata_afsdb",
  "rdata_x25", "rdata_isdn", "rdata_rt", "rdata_nsap", "rdata_px",
  "rdata_aaaa", "rdata_loc", "rdata_nxt", "rdata_srv", "rdata_naptr",
  "rdata_kx", "rdata_cert", "rdata_apl", "rdata_apl_seq", "rdata_ds",
  "rdata_dlv", "rdata_sshfp", "rdata_dhcid", "rdata_rrsig", "rdata_nsec",
  "rdata_nsec3", "rdata_nsec3_param", "rdata_tlsa", "rdata_smimea",
  "rdata_dnskey", "rdata_ipsec_base", "rdata_ipseckey", "rdata_nid",
  "rdata_l32", "rdata_l64", "rdata_lp", "rdata_eui48", "rdata_eui64",
  "rdata_uri", "rdata_caa", "rdata_openpgpkey", "rdata_csync",
  "rdata_unknown", YY_NULLPTR
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
     345,   346,    46,    64
};
# endif

#define YYPACT_NINF -445

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-445)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -445,   138,  -445,   -78,   -72,   -72,  -445,  -445,  -445,   -56,
    -445,  -445,  -445,  -445,    17,  -445,  -445,  -445,    89,   -72,
    -445,  -445,   -58,  -445,   130,    28,  -445,  -445,  -445,   -72,
     -72,   809,    30,    32,   207,   207,    45,   -54,    85,   -72,
     -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,
     -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,
     -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,
     -72,   207,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,
     -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,
     -72,   -72,   -72,   -72,   -72,   -72,   -72,   -72,   121,   -72,
    -445,  -445,  -445,   267,  -445,  -445,  -445,   -72,   -72,    75,
      18,   -53,    75,    18,    75,    18,    18,    96,    18,   103,
     105,   107,    78,    18,    18,    18,    18,    18,    75,   116,
      18,    18,   119,   128,   143,   152,   155,   159,   168,   181,
      18,    42,  -445,   183,   192,   195,   103,   -77,    96,    75,
     199,   202,   218,   221,   230,   233,   243,   245,   252,   255,
     262,   269,   273,   183,    96,   218,   277,    75,   280,    50,
      71,  -445,    30,    30,  -445,    13,  -445,    62,  -445,  -445,
     207,  -445,  -445,   -72,  -445,  -445,   207,    43,  -445,  -445,
    -445,  -445,    62,  -445,  -445,  -445,  -445,   -72,  -445,  -445,
     -72,  -445,  -445,   -72,  -445,  -445,   -72,  -445,  -445,   -72,
    -445,  -445,   -72,  -445,  -445,  -445,  -445,    64,  -445,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,
     -59,  -445,  -445,   -72,  -445,  -445,   -72,  -445,  -445,   -72,
    -445,  -445,   -72,  -445,  -445,   207,  -445,  -445,   207,  -445,
    -445,   -72,  -445,  -445,  -445,    74,  -445,  -445,   -72,  -445,
    -445,   -72,  -445,  -445,   -72,  -445,  -445,  -445,  -445,    43,
    -445,   207,  -445,   -72,  -445,  -445,   -72,  -445,  -445,   -72,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,   285,  -445,  -445,
      58,  -445,  -445,  -445,  -445,  -445,  -445,   -72,  -445,  -445,
     -72,   207,  -445,  -445,  -445,   207,  -445,  -445,   -72,  -445,
    -445,   -72,  -445,  -445,   -72,  -445,  -445,   -72,  -445,  -445,
     -72,  -445,  -445,   -72,  -445,  -445,   -72,  -445,  -445,   207,
    -445,  -445,   207,  -445,  -445,   -72,  -445,  -445,  -445,  -445,
    -445,  -445,   207,  -445,  -445,   -72,  -445,  -445,  -445,  -445,
     -72,  -445,  -445,  -445,  -445,    70,   309,    79,  -445,  -445,
      28,    57,  -445,  -445,   311,   314,    28,   320,   325,   329,
      99,   171,  -445,   332,   342,    28,    28,    28,  -445,   210,
    -445,    28,   109,  -445,    28,   345,    28,    57,  -445,   347,
     349,   354,  -445,   288,  -445,   113,   360,   362,   295,  -445,
     304,  -445,   373,   378,   382,    87,    87,    87,    28,  -445,
    -445,   384,  -445,   391,   393,  -445,   207,  -445,   207,    43,
    -445,   207,   -72,   -72,   -72,   -72,   -72,  -445,  -445,   -72,
     207,   207,   207,   207,   207,   207,  -445,   -72,   -72,   207,
      43,   -72,   -72,   -72,  -445,   288,   285,  -445,  -445,   -72,
     -72,   207,  -445,   -72,   -72,   -72,    62,    62,    62,   207,
     -72,   285,   -72,   295,  -445,  -445,   307,  -445,   398,   400,
     402,   405,   407,    91,  -445,  -445,  -445,  -445,  -445,  -445,
      28,   410,  -445,   414,   436,   438,  -445,  -445,   448,   450,
    -445,   452,   464,    87,  -445,  -445,  -445,  -445,    87,  -445,
     468,   207,  -445,   -72,   -72,   -72,   -72,   -72,    64,   207,
     -72,   -72,   -72,   207,   -72,   -72,   -72,   -72,    62,    62,
     -72,  -445,   438,   472,   475,    28,   438,  -445,  -445,   477,
     438,   438,  -445,   485,    87,   490,   438,  -445,  -445,   438,
     207,   -72,   -72,   207,   207,   -72,   207,   207,   -72,    43,
     207,   207,   207,  -445,   492,   496,  -445,  -445,   501,  -445,
    -445,   503,  -445,  -445,  -445,   -72,   -72,   -72,   285,   505,
     507,    28,  -445,   -72,   -72,   207,   509,   511,  -445,   207,
     -72,  -445,    59,   -72,   438,   207,  -445
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
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      18,    34,    13,     0,    15,    16,    17,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   136,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    14,    24,    25,    60,     0,    61,     0,    64,    65,
       0,    66,    67,     0,    90,    91,     0,    43,    92,    93,
      72,    73,     0,   120,   121,    84,    85,     0,   124,   125,
       0,   116,   117,     0,    74,    75,     0,   114,   115,     0,
     126,   127,     0,   132,   133,    45,    46,     0,   122,   123,
      68,    69,    70,    71,    76,    77,    78,    79,    80,    81,
       0,    82,    83,     0,    86,    87,     0,    88,    89,     0,
      98,    99,     0,   100,   101,     0,   102,   103,     0,   104,
     105,     0,   110,   111,    58,     0,   112,   113,     0,   118,
     119,     0,   128,   129,     0,   130,   131,   134,   135,   209,
     137,     0,   138,     0,   139,   140,     0,   141,   142,     0,
     143,   144,   145,   146,    40,    37,    38,     0,    35,    41,
      36,   147,   148,   153,   154,    94,    95,     0,   149,   150,
       0,     0,   106,   107,    56,     0,   108,   109,     0,   151,
     152,     0,   155,   156,     0,   181,   182,     0,   159,   160,
       0,   161,   162,     0,   163,   164,     0,   165,   166,     0,
     167,   168,     0,   169,   170,     0,   171,   172,   173,   174,
     175,   176,     0,   177,   178,     0,   179,   180,    96,    97,
       0,   157,   158,   184,   183,     0,     0,    62,   185,   186,
       0,     0,   192,   201,     0,     0,     0,     0,     0,     0,
       0,     0,   202,     0,     0,     0,     0,     0,   195,     0,
     196,     0,     0,   199,     0,     0,     0,     0,   208,     0,
       0,     0,    54,     0,   216,    39,     0,     0,     0,   224,
       0,   214,     0,     0,     0,     0,     0,     0,     0,   229,
     230,     0,   233,     0,     0,   237,     0,    63,     0,    44,
      49,     0,     0,     0,     0,     0,     0,    48,    47,     0,
       0,     0,     0,     0,     0,     0,    59,     0,     0,     0,
     210,     0,     0,     0,    52,     0,     0,    55,    42,     0,
       0,     0,    57,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   236,   191,     0,   203,     0,     0,
       0,     0,     0,     0,   189,   190,   193,   194,   197,   198,
       0,     0,   206,     0,     0,     0,    51,    53,     0,     0,
     223,     0,     0,     0,   225,   226,   227,   228,     0,   234,
       0,     0,    50,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   235,     0,     0,     0,     0,     0,   188,   200,     0,
       0,     0,   213,     0,     0,     0,     0,   231,   232,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   222,
       0,     0,     0,   221,     0,     0,   204,   207,     0,   211,
     212,     0,   218,   219,   220,     0,     0,     0,     0,     0,
       0,     0,   217,     0,     0,     0,     0,     0,   205,     0,
       0,   187,     0,     0,     0,     0,   215
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -445,  -445,  -445,    -1,   226,  -445,  -445,  -445,  -445,  -445,
       0,   182,   177,   201,  -365,  -445,  -161,  -445,  -445,  -227,
    -445,  -197,  -444,   -66,  -445,    -5,  -445,  -445,  -104,  -445,
    -445,  -445,  -445,  -445,  -143,  -445,  -445,  -445,  -445,  -445,
    -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,
    -445,    88,  -445,  -445,  -445,   118,  -445,  -445,  -445,  -445,
    -445,  -138,  -445,  -445,  -445,  -445,  -445,  -445,  -445,  -445,
    -445,  -445,  -445,  -445,   791
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    13,   103,   104,    15,    16,    17,    18,    31,
     180,    20,    21,    22,   287,   288,   289,   290,   186,   217,
     421,   447,   394,   305,   255,   187,   100,   178,   181,   204,
     231,   234,   237,   184,   188,   240,   243,   246,   249,   252,
     256,   259,   193,   218,   198,   210,   262,   265,   213,   270,
     271,   274,   277,   280,   306,   207,   291,   298,   309,   312,
     351,   201,   301,   302,   318,   321,   324,   327,   330,   333,
     315,   336,   343,   346,   179
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
      14,    19,   487,    24,    25,    23,   295,    28,   284,   190,
     293,   195,     7,   175,   355,   285,   286,   499,    32,   220,
     222,   224,   226,   228,   348,     7,   340,    26,    37,    38,
      28,    28,   183,   357,    33,   107,   267,   175,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,     7,   170,   342,
      27,    28,    28,     8,   177,    10,   172,   173,   175,   192,
      11,    12,    28,     8,    28,    10,   197,     8,   203,    10,
      11,    12,   169,   230,   572,   171,    28,   174,   102,     7,
     236,   239,   175,   353,   176,   357,   269,    33,     2,     3,
     171,    28,   174,    28,   284,   102,     7,   102,     7,   176,
     395,   285,   286,   415,   357,    28,   370,   102,     7,    28,
     174,   175,    28,   215,   417,   175,   382,   176,   175,    28,
     216,    28,   174,   108,   356,    28,   215,    29,    30,   176,
      28,   200,   360,   216,   427,   361,   175,    28,   206,    28,
     209,    28,   212,   175,   436,   175,   364,   175,   284,   365,
      28,   233,   366,    28,   242,   367,   175,    35,   368,   175,
     101,   369,    28,   245,    28,    34,   371,   583,   175,     4,
       5,     6,     7,     8,     9,    10,    36,    28,   248,   373,
      11,    12,   374,   175,   448,   375,    28,   251,   376,    28,
     254,   377,   175,    28,   258,   175,   508,   379,   486,   175,
     381,   338,    28,   261,   171,    28,   428,   384,   175,     0,
     385,   105,   106,   386,   282,    28,   264,    28,   273,     0,
     387,   175,   389,   175,     0,   390,    28,   276,   391,    28,
     279,     0,   175,    28,   297,   175,    28,   300,     0,   175,
     102,     7,   175,   171,    28,   434,   396,   142,     0,   397,
     398,     0,    28,   304,   400,    28,   308,   402,   175,     0,
     403,   175,     0,   404,    28,   311,   405,    28,   314,   406,
     175,     0,   407,   175,     0,   408,     0,    28,   317,    28,
     320,     0,   451,   175,   411,   175,    28,   323,     0,    28,
     326,   400,   175,     0,   413,   175,    28,   329,     0,   414,
     171,    28,   175,    28,   332,     0,   419,    28,   335,   175,
     418,    28,   345,   175,    28,   350,   423,   175,   392,   393,
     175,   444,   445,   446,     0,   431,   432,   433,   171,    28,
     304,   435,   440,     0,   437,     0,   439,   171,    28,   452,
     171,    28,   502,    28,   416,    28,   420,   501,    28,   422,
     456,   457,   458,   358,    28,   424,   359,     0,   459,    28,
     425,     0,   362,    28,   426,   463,    28,   429,   363,   513,
     466,   468,   469,   470,   471,   472,    28,   430,   473,    28,
     438,    28,   441,    28,   442,     0,   480,   481,    28,   443,
     483,   484,   485,   372,    28,   449,    28,   450,   488,   489,
     400,     0,   491,   492,   493,     0,   540,    28,   453,   498,
     544,   500,    28,   454,   546,   547,    28,   455,    28,   460,
     551,   378,     0,   552,   380,    28,   461,    28,   462,     0,
     509,   383,    28,   503,    28,   504,    28,   505,   518,    28,
     506,    28,   507,   519,    28,   510,     0,   388,    28,   511,
     400,     0,   522,   523,   524,   525,   526,   371,     0,   529,
     530,   531,   400,   533,   534,   535,   536,     0,   585,   539,
      28,   512,    28,   304,     0,   543,     0,   399,     0,   549,
       0,   401,    28,   514,    28,   515,    28,   516,     0,   400,
     554,   555,     0,   400,   558,   400,   400,   561,    28,   517,
     400,   400,    28,   520,     0,   409,    28,   541,   410,    28,
     542,    28,   545,     0,   569,   570,   571,     0,   412,    28,
     548,   575,   576,   577,    28,   550,    28,   565,     0,   582,
      28,   566,   584,     0,   400,    28,   567,    28,   568,    28,
     573,    28,   574,    28,   579,    28,   580,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   464,     0,   465,     0,     0,   467,     0,     0,
       0,     0,     0,     0,     0,     0,   474,   475,   476,   477,
     478,   479,     0,     0,     0,   482,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   490,     0,     0,
       0,     0,   494,   495,   496,   497,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   521,     0,     0,
       0,     0,     0,     0,   527,   528,     0,     0,     0,   532,
       0,     0,     0,     0,   537,   538,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   553,     0,     0,   556,
     557,     0,   559,   560,     0,     0,   562,   563,   564,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   578,     0,     0,     0,   581,     0,     0,     0,     0,
       0,   586,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,     0,    58,    59,    60,    61,    62,    63,    64,    65,
      66,     0,    67,     0,     0,     0,     0,    68,    69,     0,
      70,     0,     0,    71,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       0,     0,     0,     0,    98,     0,     0,     0,     0,     0,
      99,   182,   185,   189,   191,   194,   196,   199,   202,   205,
     208,   211,   214,   219,   221,   223,   225,   227,   229,   232,
     235,   238,   241,   244,   247,   250,   253,   257,   260,   263,
     266,   268,   272,     0,   275,   278,   281,   283,   292,   294,
     296,   299,   303,   307,   310,   313,   316,   319,   322,   325,
     328,   331,   334,   337,   339,   341,   344,   347,   349,   352,
       0,   354
};

static const yytype_int16 yycheck[] =
{
       1,     1,   446,     4,     5,    83,   149,    84,    85,   113,
     148,   115,    84,    90,     1,    92,    93,   461,    19,   123,
     124,   125,   126,   127,   167,    84,   164,    83,    29,    30,
      84,    84,    85,    92,    92,    89,   140,    90,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    84,    99,   165,
      83,    84,    84,    85,   109,    87,   107,   108,    90,   114,
      92,    93,    84,    85,    84,    87,   116,    85,   118,    87,
      92,    93,     1,   128,   568,    83,    84,    85,    83,    84,
     130,   131,    90,    83,    92,    92,   141,    92,     0,     1,
      83,    84,    85,    84,    85,    83,    84,    83,    84,    92,
      92,    92,    93,    83,    92,    84,    92,    83,    84,    84,
      85,    90,    84,    85,    85,    90,    92,    92,    90,    84,
      92,    84,    85,    88,   175,    84,    85,    88,    89,    92,
      84,    85,   183,    92,    85,   186,    90,    84,    85,    84,
      85,    84,    85,    90,    85,    90,   197,    90,    85,   200,
      84,    85,   203,    84,    85,   206,    90,    25,   209,    90,
      33,   212,    84,    85,    84,    85,   217,   582,    90,    81,
      82,    83,    84,    85,    86,    87,    25,    84,    85,   230,
      92,    93,   233,    90,   395,   236,    84,    85,   239,    84,
      85,   242,    90,    84,    85,    90,   473,   248,   445,    90,
     251,   163,    84,    85,    83,    84,    85,   258,    90,    -1,
     261,    35,    36,   264,   146,    84,    85,    84,    85,    -1,
     271,    90,   273,    90,    -1,   276,    84,    85,   279,    84,
      85,    -1,    90,    84,    85,    90,    84,    85,    -1,    90,
      83,    84,    90,    83,    84,    85,   297,    71,    -1,   300,
     301,    -1,    84,    85,   305,    84,    85,   308,    90,    -1,
     311,    90,    -1,   314,    84,    85,   317,    84,    85,   320,
      90,    -1,   323,    90,    -1,   326,    -1,    84,    85,    84,
      85,    -1,   398,    90,   335,    90,    84,    85,    -1,    84,
      85,   342,    90,    -1,   345,    90,    84,    85,    -1,   350,
      83,    84,    90,    84,    85,    -1,   361,    84,    85,    90,
     360,    84,    85,    90,    84,    85,   366,    90,    83,    84,
      90,    83,    84,    85,    -1,   375,   376,   377,    83,    84,
      85,   381,   387,    -1,   384,    -1,   386,    83,    84,    85,
      83,    84,    85,    84,    85,    84,    85,   463,    84,    85,
     405,   406,   407,   177,    84,    85,   180,    -1,   408,    84,
      85,    -1,   186,    84,    85,   416,    84,    85,   192,   485,
     421,   422,   423,   424,   425,   426,    84,    85,   429,    84,
      85,    84,    85,    84,    85,    -1,   437,   438,    84,    85,
     441,   442,   443,   217,    84,    85,    84,    85,   449,   450,
     451,    -1,   453,   454,   455,    -1,   522,    84,    85,   460,
     526,   462,    84,    85,   530,   531,    84,    85,    84,    85,
     536,   245,    -1,   539,   248,    84,    85,    84,    85,    -1,
     480,   255,    84,    85,    84,    85,    84,    85,   493,    84,
      85,    84,    85,   498,    84,    85,    -1,   271,    84,    85,
     501,    -1,   503,   504,   505,   506,   507,   508,    -1,   510,
     511,   512,   513,   514,   515,   516,   517,    -1,   584,   520,
      84,    85,    84,    85,    -1,   525,    -1,   301,    -1,   534,
      -1,   305,    84,    85,    84,    85,    84,    85,    -1,   540,
     541,   542,    -1,   544,   545,   546,   547,   548,    84,    85,
     551,   552,    84,    85,    -1,   329,    84,    85,   332,    84,
      85,    84,    85,    -1,   565,   566,   567,    -1,   342,    84,
      85,   571,   573,   574,    84,    85,    84,    85,    -1,   580,
      84,    85,   583,    -1,   585,    84,    85,    84,    85,    84,
      85,    84,    85,    84,    85,    84,    85,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   416,    -1,   418,    -1,    -1,   421,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   430,   431,   432,   433,
     434,   435,    -1,    -1,    -1,   439,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   451,    -1,    -1,
      -1,    -1,   456,   457,   458,   459,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   501,    -1,    -1,
      -1,    -1,    -1,    -1,   508,   509,    -1,    -1,    -1,   513,
      -1,    -1,    -1,    -1,   518,   519,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   540,    -1,    -1,   543,
     544,    -1,   546,   547,    -1,    -1,   550,   551,   552,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   575,    -1,    -1,    -1,   579,    -1,    -1,    -1,    -1,
      -1,   585,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    -1,    33,    -1,    -1,    -1,    -1,    38,    39,    -1,
      41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      -1,    -1,    -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,
      91,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,    -1,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
      -1,   170
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    95,     0,     1,    81,    82,    83,    84,    85,    86,
      87,    92,    93,    96,    97,    99,   100,   101,   102,   104,
     105,   106,   107,    83,    97,    97,    83,    83,    84,    88,
      89,   103,    97,    92,    85,   105,   107,    97,    97,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    33,    38,    39,
      41,    44,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    85,    91,
     120,   106,    83,    97,    98,    98,    98,    89,    88,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    98,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,     1,
      97,    83,    97,    97,    85,    90,    92,   119,   121,   168,
     104,   122,   168,    85,   127,   168,   112,   119,   128,   168,
     122,   168,   119,   136,   168,   122,   168,   104,   138,   168,
      85,   155,   168,   104,   123,   168,    85,   149,   168,    85,
     139,   168,    85,   142,   168,    85,    92,   113,   137,   168,
     122,   168,   122,   168,   122,   168,   122,   168,   122,   168,
     119,   124,   168,    85,   125,   168,   104,   126,   168,   104,
     129,   168,    85,   130,   168,    85,   131,   168,    85,   132,
     168,    85,   133,   168,    85,   118,   134,   168,    85,   135,
     168,    85,   140,   168,    85,   141,   168,   122,   168,   119,
     143,   144,   168,    85,   145,   168,    85,   146,   168,    85,
     147,   168,   149,   168,    85,    92,    93,   108,   109,   110,
     111,   150,   168,   155,   168,   128,   168,    85,   151,   168,
      85,   156,   157,   168,    85,   117,   148,   168,    85,   152,
     168,    85,   153,   168,    85,   164,   168,    85,   158,   168,
      85,   159,   168,    85,   160,   168,    85,   161,   168,    85,
     162,   168,    85,   163,   168,    85,   165,   168,   145,   168,
     155,   168,   117,   166,   168,    85,   167,   168,   128,   168,
      85,   154,   168,    83,   168,     1,    97,    92,    98,    98,
      97,    97,    98,    98,    97,    97,    97,    97,    97,    97,
      92,    97,    98,    97,    97,    97,    97,    97,    98,    97,
      98,    97,    92,    98,    97,    97,    97,    97,    98,    97,
      97,    97,    83,    84,   116,    92,    97,    97,    97,    98,
      97,    98,    97,    97,    97,    97,    97,    97,    97,    98,
      98,    97,    98,    97,    97,    83,    85,    85,   104,   119,
      85,   114,    85,   104,    85,    85,    85,    85,    85,    85,
      85,   104,   104,   104,    85,   104,    85,   104,    85,   104,
     119,    85,    85,    85,    83,    84,    85,   115,   110,    85,
      85,   117,    85,    85,    85,    85,   119,   119,   119,   104,
      85,    85,    85,    97,    98,    98,    97,    98,    97,    97,
      97,    97,    97,    97,    98,    98,    98,    98,    98,    98,
      97,    97,    98,    97,    97,    97,   115,   116,    97,    97,
      98,    97,    97,    97,    98,    98,    98,    98,    97,   116,
      97,   117,    85,    85,    85,    85,    85,    85,   113,   104,
      85,    85,    85,   117,    85,    85,    85,    85,   119,   119,
      85,    98,    97,    97,    97,    97,    97,    98,    98,    97,
      97,    97,    98,    97,    97,    97,    97,    98,    98,    97,
     117,    85,    85,   104,   117,    85,   117,   117,    85,   119,
      85,   117,   117,    98,    97,    97,    98,    98,    97,    98,
      98,    97,    98,    98,    98,    85,    85,    85,    85,    97,
      97,    97,   116,    85,    85,   104,    97,    97,    98,    85,
      85,    98,    97,   108,    97,   117,    98
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    94,    95,    95,    96,    96,    96,    96,    96,    96,
      96,    97,    97,    98,    98,    99,   100,   100,   101,   102,
     102,   103,   103,   103,   103,   103,   104,   104,   105,   105,
     105,   106,   106,   107,   107,   108,   108,   109,   109,   109,
     110,   111,   111,   112,   112,   113,   113,   113,   113,   114,
     114,   115,   115,   115,   116,   116,   117,   117,   118,   118,
     119,   119,   119,   119,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   157,   158,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   168,   168
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     2,     2,     1,     1,     1,
       2,     1,     2,     1,     2,     4,     4,     4,     3,     2,
       1,     0,     2,     2,     4,     4,     1,     1,     1,     1,
       2,     1,     1,     1,     3,     1,     1,     1,     1,     2,
       1,     1,     3,     1,     3,     1,     1,     3,     3,     1,
       3,     2,     1,     2,     1,     2,     1,     3,     1,     3,
       1,     1,     2,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     2,    14,     6,     4,
       4,     4,     2,     4,     4,     2,     2,     4,     4,     2,
       6,     2,     2,     4,     8,    12,     4,     8,     2,     1,
       3,     8,     8,     6,     2,    18,     2,    10,     8,     8,
       8,     8,     7,     4,     2,     4,     4,     4,     4,     2,
       2,     6,     6,     2,     4,     6,     4,     3
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
#line 1925 "zparser.c" /* yacc.c:1646  */
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
#line 1937 "zparser.c" /* yacc.c:1646  */
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
#line 1949 "zparser.c" /* yacc.c:1646  */
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
#line 1973 "zparser.c" /* yacc.c:1646  */
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
#line 1985 "zparser.c" /* yacc.c:1646  */
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
#line 2000 "zparser.c" /* yacc.c:1646  */
    break;

  case 17:
#line 169 "zparser.y" /* yacc.c:1646  */
    {
	    zc_error_prev_line("$ORIGIN directive requires absolute domain name");
    }
#line 2008 "zparser.c" /* yacc.c:1646  */
    break;

  case 18:
#line 175 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.owner = (yyvsp[-2].domain);
	    parser->current_rr.type = (yyvsp[0].type);
    }
#line 2017 "zparser.c" /* yacc.c:1646  */
    break;

  case 19:
#line 182 "zparser.y" /* yacc.c:1646  */
    {
	    parser->prev_dname = (yyvsp[-1].domain);
	    (yyval.domain) = (yyvsp[-1].domain);
    }
#line 2026 "zparser.c" /* yacc.c:1646  */
    break;

  case 20:
#line 187 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.domain) = parser->prev_dname;
    }
#line 2034 "zparser.c" /* yacc.c:1646  */
    break;

  case 21:
#line 193 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = parser->default_ttl;
	    parser->current_rr.klass = parser->default_class;
    }
#line 2043 "zparser.c" /* yacc.c:1646  */
    break;

  case 22:
#line 198 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = parser->default_ttl;
	    parser->current_rr.klass = (yyvsp[-1].klass);
    }
#line 2052 "zparser.c" /* yacc.c:1646  */
    break;

  case 23:
#line 203 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = (yyvsp[-1].ttl);
	    parser->current_rr.klass = parser->default_class;
    }
#line 2061 "zparser.c" /* yacc.c:1646  */
    break;

  case 24:
#line 208 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = (yyvsp[-3].ttl);
	    parser->current_rr.klass = (yyvsp[-1].klass);
    }
#line 2070 "zparser.c" /* yacc.c:1646  */
    break;

  case 25:
#line 213 "zparser.y" /* yacc.c:1646  */
    {
	    parser->current_rr.ttl = (yyvsp[-1].ttl);
	    parser->current_rr.klass = (yyvsp[-3].klass);
    }
#line 2079 "zparser.c" /* yacc.c:1646  */
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
#line 2102 "zparser.c" /* yacc.c:1646  */
    break;

  case 28:
#line 242 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.domain) = parser->db->domains->root;
    }
#line 2110 "zparser.c" /* yacc.c:1646  */
    break;

  case 29:
#line 246 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.domain) = parser->origin;
    }
#line 2118 "zparser.c" /* yacc.c:1646  */
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
#line 2130 "zparser.c" /* yacc.c:1646  */
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
#line 2148 "zparser.c" /* yacc.c:1646  */
    break;

  case 32:
#line 274 "zparser.y" /* yacc.c:1646  */
    {
	    zc_error("bitlabels are now deprecated. RFC2673 is obsoleted.");
	    (yyval.dname) = error_dname;
    }
#line 2157 "zparser.c" /* yacc.c:1646  */
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
#line 2173 "zparser.c" /* yacc.c:1646  */
    break;

  case 36:
#line 301 "zparser.y" /* yacc.c:1646  */
    {
	    /* terminate in root label and copy the origin in there */
	    if(parser->origin && domain_dname(parser->origin)) {
		    (yyval.data).len = (yyvsp[0].data).len + domain_dname(parser->origin)->name_size;
		    if ((yyval.data).len > MAXDOMAINLEN)
			    zc_error("domain name exceeds %d character limit",
				     MAXDOMAINLEN);
		    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
		    memmove((yyval.data).str, (yyvsp[0].data).str, (yyvsp[0].data).len);
		    memmove((yyval.data).str + (yyvsp[0].data).len, dname_name(domain_dname(parser->origin)),
			domain_dname(parser->origin)->name_size);
	    } else {
		    (yyval.data).len = (yyvsp[0].data).len + 1;
		    if ((yyval.data).len > MAXDOMAINLEN)
			    zc_error("domain name exceeds %d character limit",
				     MAXDOMAINLEN);
		    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
		    memmove((yyval.data).str, (yyvsp[0].data).str, (yyvsp[0].data).len);
		    (yyval.data).str[ (yyvsp[0].data).len ] = 0;
	    }
    }
#line 2199 "zparser.c" /* yacc.c:1646  */
    break;

  case 37:
#line 325 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region, 1);
	    result[0] = 0;
	    (yyval.data).str = result;
	    (yyval.data).len = 1;
    }
#line 2210 "zparser.c" /* yacc.c:1646  */
    break;

  case 38:
#line 332 "zparser.y" /* yacc.c:1646  */
    {
	    if(parser->origin && domain_dname(parser->origin)) {
		    (yyval.data).len = domain_dname(parser->origin)->name_size;
		    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
		    memmove((yyval.data).str, dname_name(domain_dname(parser->origin)), (yyval.data).len);
	    } else {
		    (yyval.data).len = 1;
		    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
		    (yyval.data).str[0] = 0;
	    }
    }
#line 2226 "zparser.c" /* yacc.c:1646  */
    break;

  case 39:
#line 344 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.data).len = (yyvsp[-1].data).len + 1;
	    if ((yyval.data).len > MAXDOMAINLEN)
		    zc_error("domain name exceeds %d character limit",
			     MAXDOMAINLEN);
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
	    memcpy((yyval.data).str, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
	    (yyval.data).str[(yyvsp[-1].data).len] = 0;
    }
#line 2240 "zparser.c" /* yacc.c:1646  */
    break;

  case 40:
#line 356 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[0].data).len + 1);

	    if ((yyvsp[0].data).len > MAXLABELLEN)
		    zc_error("label exceeds %d character limit", MAXLABELLEN);

	    /* make label anyway */
	    result[0] = (yyvsp[0].data).len;
	    memmove(result+1, (yyvsp[0].data).str, (yyvsp[0].data).len);

	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[0].data).len + 1;
    }
#line 2259 "zparser.c" /* yacc.c:1646  */
    break;

  case 42:
#line 374 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    if ((yyval.data).len > MAXDOMAINLEN)
		    zc_error("domain name exceeds %d character limit",
			     MAXDOMAINLEN);
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len);
	    memmove((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memmove((yyval.data).str + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
    }
#line 2273 "zparser.c" /* yacc.c:1646  */
    break;

  case 43:
#line 386 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 1);
    }
#line 2281 "zparser.c" /* yacc.c:1646  */
    break;

  case 44:
#line 390 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_txt_wireformat(zparser_conv_text(parser->rr_region, (yyvsp[0].data).str, (yyvsp[0].data).len), 0);
    }
#line 2289 "zparser.c" /* yacc.c:1646  */
    break;

  case 46:
#line 401 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.data).len = 1;
	    (yyval.data).str = region_strdup(parser->rr_region, ".");
    }
#line 2298 "zparser.c" /* yacc.c:1646  */
    break;

  case 47:
#line 406 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len + 1;
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len + 1);
	    memcpy((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len, " ", 1);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len + 1, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2311 "zparser.c" /* yacc.c:1646  */
    break;

  case 48:
#line 415 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len + 1;
	    (yyval.data).str = (char *) region_alloc(parser->rr_region, (yyval.data).len + 1);
	    memcpy((yyval.data).str, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len, ".", 1);
	    memcpy((yyval.data).str + (yyvsp[-2].data).len + 1, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2324 "zparser.c" /* yacc.c:1646  */
    break;

  case 49:
#line 427 "zparser.y" /* yacc.c:1646  */
    {
	    uint16_t type = rrtype_from_string((yyvsp[0].data).str);
	    if (type != 0 && type < 128) {
		    set_bit(nxtbits, type);
	    } else {
		    zc_error("bad type %d in NXT record", (int) type);
	    }
    }
#line 2337 "zparser.c" /* yacc.c:1646  */
    break;

  case 50:
#line 436 "zparser.y" /* yacc.c:1646  */
    {
	    uint16_t type = rrtype_from_string((yyvsp[0].data).str);
	    if (type != 0 && type < 128) {
		    set_bit(nxtbits, type);
	    } else {
		    zc_error("bad type %d in NXT record", (int) type);
	    }
    }
#line 2350 "zparser.c" /* yacc.c:1646  */
    break;

  case 51:
#line 447 "zparser.y" /* yacc.c:1646  */
    {
    }
#line 2357 "zparser.c" /* yacc.c:1646  */
    break;

  case 52:
#line 450 "zparser.y" /* yacc.c:1646  */
    {
    }
#line 2364 "zparser.c" /* yacc.c:1646  */
    break;

  case 53:
#line 453 "zparser.y" /* yacc.c:1646  */
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
#line 2380 "zparser.c" /* yacc.c:1646  */
    break;

  case 57:
#line 476 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-2].data).len + (yyvsp[0].data).len + 1);
	    memcpy(result, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy(result + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2394 "zparser.c" /* yacc.c:1646  */
    break;

  case 59:
#line 493 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-2].data).len + (yyvsp[0].data).len + 1);
	    memcpy(result, (yyvsp[-2].data).str, (yyvsp[-2].data).len);
	    memcpy(result + (yyvsp[-2].data).len, (yyvsp[0].data).str, (yyvsp[0].data).len);
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-2].data).len + (yyvsp[0].data).len;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2408 "zparser.c" /* yacc.c:1646  */
    break;

  case 61:
#line 509 "zparser.y" /* yacc.c:1646  */
    {
	(yyval.data).str = ".";
	(yyval.data).len = 1;
    }
#line 2417 "zparser.c" /* yacc.c:1646  */
    break;

  case 62:
#line 514 "zparser.y" /* yacc.c:1646  */
    {
	    char *result = (char *) region_alloc(parser->rr_region,
						 (yyvsp[-1].data).len + 2);
	    memcpy(result, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
	    result[(yyvsp[-1].data).len] = '.';
	    (yyval.data).str = result;
	    (yyval.data).len = (yyvsp[-1].data).len + 1;
	    (yyval.data).str[(yyval.data).len] = '\0';
    }
#line 2431 "zparser.c" /* yacc.c:1646  */
    break;

  case 63:
#line 524 "zparser.y" /* yacc.c:1646  */
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
#line 2446 "zparser.c" /* yacc.c:1646  */
    break;

  case 65:
#line 542 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2452 "zparser.c" /* yacc.c:1646  */
    break;

  case 67:
#line 544 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2458 "zparser.c" /* yacc.c:1646  */
    break;

  case 68:
#line 545 "zparser.y" /* yacc.c:1646  */
    { zc_warning_prev_line("MD is obsolete"); }
#line 2464 "zparser.c" /* yacc.c:1646  */
    break;

  case 69:
#line 547 "zparser.y" /* yacc.c:1646  */
    {
	    zc_warning_prev_line("MD is obsolete");
	    (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2473 "zparser.c" /* yacc.c:1646  */
    break;

  case 70:
#line 551 "zparser.y" /* yacc.c:1646  */
    { zc_warning_prev_line("MF is obsolete"); }
#line 2479 "zparser.c" /* yacc.c:1646  */
    break;

  case 71:
#line 553 "zparser.y" /* yacc.c:1646  */
    {
	    zc_warning_prev_line("MF is obsolete");
	    (yyval.type) = (yyvsp[-2].type);
	    parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2489 "zparser.c" /* yacc.c:1646  */
    break;

  case 73:
#line 559 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2495 "zparser.c" /* yacc.c:1646  */
    break;

  case 75:
#line 561 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2501 "zparser.c" /* yacc.c:1646  */
    break;

  case 76:
#line 562 "zparser.y" /* yacc.c:1646  */
    { zc_warning_prev_line("MB is obsolete"); }
#line 2507 "zparser.c" /* yacc.c:1646  */
    break;

  case 77:
#line 564 "zparser.y" /* yacc.c:1646  */
    {
	    zc_warning_prev_line("MB is obsolete");
	    (yyval.type) = (yyvsp[-2].type);
	    parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown));
    }
#line 2517 "zparser.c" /* yacc.c:1646  */
    break;

  case 79:
#line 570 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2523 "zparser.c" /* yacc.c:1646  */
    break;

  case 81:
#line 572 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2529 "zparser.c" /* yacc.c:1646  */
    break;

  case 83:
#line 575 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2535 "zparser.c" /* yacc.c:1646  */
    break;

  case 85:
#line 577 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2541 "zparser.c" /* yacc.c:1646  */
    break;

  case 87:
#line 579 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2547 "zparser.c" /* yacc.c:1646  */
    break;

  case 89:
#line 581 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2553 "zparser.c" /* yacc.c:1646  */
    break;

  case 91:
#line 583 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2559 "zparser.c" /* yacc.c:1646  */
    break;

  case 93:
#line 585 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2565 "zparser.c" /* yacc.c:1646  */
    break;

  case 95:
#line 587 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2571 "zparser.c" /* yacc.c:1646  */
    break;

  case 97:
#line 589 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2577 "zparser.c" /* yacc.c:1646  */
    break;

  case 99:
#line 591 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2583 "zparser.c" /* yacc.c:1646  */
    break;

  case 101:
#line 593 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2589 "zparser.c" /* yacc.c:1646  */
    break;

  case 103:
#line 595 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2595 "zparser.c" /* yacc.c:1646  */
    break;

  case 105:
#line 597 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2601 "zparser.c" /* yacc.c:1646  */
    break;

  case 107:
#line 599 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2607 "zparser.c" /* yacc.c:1646  */
    break;

  case 109:
#line 601 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2613 "zparser.c" /* yacc.c:1646  */
    break;

  case 111:
#line 603 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2619 "zparser.c" /* yacc.c:1646  */
    break;

  case 113:
#line 605 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2625 "zparser.c" /* yacc.c:1646  */
    break;

  case 115:
#line 607 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2631 "zparser.c" /* yacc.c:1646  */
    break;

  case 117:
#line 609 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2637 "zparser.c" /* yacc.c:1646  */
    break;

  case 119:
#line 611 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2643 "zparser.c" /* yacc.c:1646  */
    break;

  case 121:
#line 613 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2649 "zparser.c" /* yacc.c:1646  */
    break;

  case 123:
#line 615 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2655 "zparser.c" /* yacc.c:1646  */
    break;

  case 125:
#line 617 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2661 "zparser.c" /* yacc.c:1646  */
    break;

  case 127:
#line 619 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2667 "zparser.c" /* yacc.c:1646  */
    break;

  case 129:
#line 621 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2673 "zparser.c" /* yacc.c:1646  */
    break;

  case 131:
#line 623 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2679 "zparser.c" /* yacc.c:1646  */
    break;

  case 133:
#line 625 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2685 "zparser.c" /* yacc.c:1646  */
    break;

  case 135:
#line 627 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2691 "zparser.c" /* yacc.c:1646  */
    break;

  case 138:
#line 630 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2697 "zparser.c" /* yacc.c:1646  */
    break;

  case 140:
#line 632 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2703 "zparser.c" /* yacc.c:1646  */
    break;

  case 141:
#line 633 "zparser.y" /* yacc.c:1646  */
    { if (dlv_warn) { dlv_warn = 0; zc_warning_prev_line("DLV is experimental"); } }
#line 2709 "zparser.c" /* yacc.c:1646  */
    break;

  case 142:
#line 634 "zparser.y" /* yacc.c:1646  */
    { if (dlv_warn) { dlv_warn = 0; zc_warning_prev_line("DLV is experimental"); } (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2715 "zparser.c" /* yacc.c:1646  */
    break;

  case 144:
#line 636 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2721 "zparser.c" /* yacc.c:1646  */
    break;

  case 146:
#line 638 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2727 "zparser.c" /* yacc.c:1646  */
    break;

  case 148:
#line 640 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2733 "zparser.c" /* yacc.c:1646  */
    break;

  case 150:
#line 642 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2739 "zparser.c" /* yacc.c:1646  */
    break;

  case 152:
#line 644 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2745 "zparser.c" /* yacc.c:1646  */
    break;

  case 154:
#line 646 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2751 "zparser.c" /* yacc.c:1646  */
    break;

  case 156:
#line 648 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2757 "zparser.c" /* yacc.c:1646  */
    break;

  case 158:
#line 650 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2763 "zparser.c" /* yacc.c:1646  */
    break;

  case 160:
#line 652 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2769 "zparser.c" /* yacc.c:1646  */
    break;

  case 162:
#line 654 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2775 "zparser.c" /* yacc.c:1646  */
    break;

  case 164:
#line 656 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2781 "zparser.c" /* yacc.c:1646  */
    break;

  case 166:
#line 658 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2787 "zparser.c" /* yacc.c:1646  */
    break;

  case 168:
#line 660 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2793 "zparser.c" /* yacc.c:1646  */
    break;

  case 170:
#line 662 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2799 "zparser.c" /* yacc.c:1646  */
    break;

  case 172:
#line 664 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2805 "zparser.c" /* yacc.c:1646  */
    break;

  case 174:
#line 666 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2811 "zparser.c" /* yacc.c:1646  */
    break;

  case 176:
#line 668 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2817 "zparser.c" /* yacc.c:1646  */
    break;

  case 178:
#line 670 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2823 "zparser.c" /* yacc.c:1646  */
    break;

  case 180:
#line 672 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2829 "zparser.c" /* yacc.c:1646  */
    break;

  case 182:
#line 674 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2835 "zparser.c" /* yacc.c:1646  */
    break;

  case 183:
#line 675 "zparser.y" /* yacc.c:1646  */
    { (yyval.type) = (yyvsp[-2].type); parse_unknown_rdata((yyvsp[-2].type), (yyvsp[0].unknown)); }
#line 2841 "zparser.c" /* yacc.c:1646  */
    break;

  case 184:
#line 677 "zparser.y" /* yacc.c:1646  */
    {
	    zc_error_prev_line("unrecognized RR type '%s'", (yyvsp[-2].data).str);
    }
#line 2849 "zparser.c" /* yacc.c:1646  */
    break;

  case 185:
#line 689 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-1].data).str));
    }
#line 2857 "zparser.c" /* yacc.c:1646  */
    break;

  case 186:
#line 695 "zparser.y" /* yacc.c:1646  */
    {
	    /* convert a single dname record */
	    zadd_rdata_domain((yyvsp[-1].domain));
    }
#line 2866 "zparser.c" /* yacc.c:1646  */
    break;

  case 187:
#line 702 "zparser.y" /* yacc.c:1646  */
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
#line 2881 "zparser.c" /* yacc.c:1646  */
    break;

  case 188:
#line 715 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-5].data).str)); /* address */
	    zadd_rdata_wireformat(zparser_conv_services(parser->region, (yyvsp[-3].data).str, (yyvsp[-1].data).str)); /* protocol and services */
    }
#line 2890 "zparser.c" /* yacc.c:1646  */
    break;

  case 189:
#line 722 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* CPU */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* OS*/
    }
#line 2899 "zparser.c" /* yacc.c:1646  */
    break;

  case 190:
#line 729 "zparser.y" /* yacc.c:1646  */
    {
	    /* convert a single dname record */
	    zadd_rdata_domain((yyvsp[-3].domain));
	    zadd_rdata_domain((yyvsp[-1].domain));
    }
#line 2909 "zparser.c" /* yacc.c:1646  */
    break;

  case 191:
#line 737 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* priority */
	    zadd_rdata_domain((yyvsp[-1].domain));	/* MX host */
    }
#line 2918 "zparser.c" /* yacc.c:1646  */
    break;

  case 192:
#line 744 "zparser.y" /* yacc.c:1646  */
    {
	zadd_rdata_txt_clean_wireformat();
    }
#line 2926 "zparser.c" /* yacc.c:1646  */
    break;

  case 193:
#line 751 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_domain((yyvsp[-3].domain)); /* mbox d-name */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* txt d-name */
    }
#line 2935 "zparser.c" /* yacc.c:1646  */
    break;

  case 194:
#line 759 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* subtype */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* domain name */
    }
#line 2944 "zparser.c" /* yacc.c:1646  */
    break;

  case 195:
#line 767 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* X.25 address. */
    }
#line 2952 "zparser.c" /* yacc.c:1646  */
    break;

  case 196:
#line 774 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* address */
    }
#line 2960 "zparser.c" /* yacc.c:1646  */
    break;

  case 197:
#line 778 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* address */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* sub-address */
    }
#line 2969 "zparser.c" /* yacc.c:1646  */
    break;

  case 198:
#line 786 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* intermediate host */
    }
#line 2978 "zparser.c" /* yacc.c:1646  */
    break;

  case 199:
#line 794 "zparser.y" /* yacc.c:1646  */
    {
	    /* String must start with "0x" or "0X".	 */
	    if (strncasecmp((yyvsp[-1].data).str, "0x", 2) != 0) {
		    zc_error_prev_line("NSAP rdata must start with '0x'");
	    } else {
		    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str + 2, (yyvsp[-1].data).len - 2)); /* NSAP */
	    }
    }
#line 2991 "zparser.c" /* yacc.c:1646  */
    break;

  case 200:
#line 806 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-3].domain)); /* MAP822 */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* MAPX400 */
    }
#line 3001 "zparser.c" /* yacc.c:1646  */
    break;

  case 201:
#line 814 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_aaaa(parser->region, (yyvsp[-1].data).str));  /* IPv6 address */
    }
#line 3009 "zparser.c" /* yacc.c:1646  */
    break;

  case 202:
#line 820 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_loc(parser->region, (yyvsp[-1].data).str)); /* Location */
    }
#line 3017 "zparser.c" /* yacc.c:1646  */
    break;

  case 203:
#line 826 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_domain((yyvsp[-3].domain)); /* nxt name */
	    zadd_rdata_wireformat(zparser_conv_nxt(parser->region, nxtbits)); /* nxt bitlist */
	    memset(nxtbits, 0, sizeof(nxtbits));
    }
#line 3027 "zparser.c" /* yacc.c:1646  */
    break;

  case 204:
#line 834 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* prio */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* weight */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* port */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* target name */
    }
#line 3038 "zparser.c" /* yacc.c:1646  */
    break;

  case 205:
#line 844 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-11].data).str)); /* order */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-9].data).str)); /* preference */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-7].data).str, (yyvsp[-7].data).len)); /* flags */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-5].data).str, (yyvsp[-5].data).len)); /* service */
	    zadd_rdata_wireformat(zparser_conv_text(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* regexp */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* target name */
    }
#line 3051 "zparser.c" /* yacc.c:1646  */
    break;

  case 206:
#line 856 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain)); /* exchanger */
    }
#line 3060 "zparser.c" /* yacc.c:1646  */
    break;

  case 207:
#line 864 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_certificate_type(parser->region, (yyvsp[-7].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* key tag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-3].data).str)); /* algorithm */
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* certificate or CRL */
    }
#line 3071 "zparser.c" /* yacc.c:1646  */
    break;

  case 209:
#line 877 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_apl_rdata(parser->region, (yyvsp[0].data).str));
    }
#line 3079 "zparser.c" /* yacc.c:1646  */
    break;

  case 210:
#line 881 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_apl_rdata(parser->region, (yyvsp[0].data).str));
    }
#line 3087 "zparser.c" /* yacc.c:1646  */
    break;

  case 211:
#line 887 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* keytag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
    }
#line 3098 "zparser.c" /* yacc.c:1646  */
    break;

  case 212:
#line 896 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* keytag */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
    }
#line 3109 "zparser.c" /* yacc.c:1646  */
    break;

  case 213:
#line 905 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* fp type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* hash */
    }
#line 3119 "zparser.c" /* yacc.c:1646  */
    break;

  case 214:
#line 913 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* data blob */
    }
#line 3127 "zparser.c" /* yacc.c:1646  */
    break;

  case 215:
#line 919 "zparser.y" /* yacc.c:1646  */
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
#line 3144 "zparser.c" /* yacc.c:1646  */
    break;

  case 216:
#line 934 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_dns_name(parser->region, 
				(const uint8_t*) (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* nsec name */
	    zadd_rdata_wireformat(zparser_conv_nsec(parser->region, nsecbits)); /* nsec bitlist */
	    memset(nsecbits, 0, sizeof(nsecbits));
            nsec_highest_rcode = 0;
    }
#line 3156 "zparser.c" /* yacc.c:1646  */
    break;

  case 217:
#line 944 "zparser.y" /* yacc.c:1646  */
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
#line 3173 "zparser.c" /* yacc.c:1646  */
    break;

  case 218:
#line 959 "zparser.y" /* yacc.c:1646  */
    {
#ifdef NSEC3
	    nsec3_add_params((yyvsp[-7].data).str, (yyvsp[-5].data).str, (yyvsp[-3].data).str, (yyvsp[-1].data).str, (yyvsp[-1].data).len);
#else
	    zc_error_prev_line("nsec3 not supported");
#endif /* NSEC3 */
    }
#line 3185 "zparser.c" /* yacc.c:1646  */
    break;

  case 219:
#line 969 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-7].data).str)); /* usage */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* selector */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* matching type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* ca data */
    }
#line 3196 "zparser.c" /* yacc.c:1646  */
    break;

  case 220:
#line 978 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-7].data).str)); /* usage */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* selector */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-3].data).str)); /* matching type */
	    zadd_rdata_wireformat(zparser_conv_hex(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* ca data */
    }
#line 3207 "zparser.c" /* yacc.c:1646  */
    break;

  case 221:
#line 987 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-7].data).str)); /* flags */
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* proto */
	    zadd_rdata_wireformat(zparser_conv_algorithm(parser->region, (yyvsp[-3].data).str)); /* alg */
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* hash */
    }
#line 3218 "zparser.c" /* yacc.c:1646  */
    break;

  case 222:
#line 996 "zparser.y" /* yacc.c:1646  */
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
			if(!(name = dname_parse(parser->region, (yyvsp[0].data).str))) {
				zc_error_prev_line("IPSECKEY bad gateway dname %s", (yyvsp[0].data).str);
				break;
			}
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
#line 3261 "zparser.c" /* yacc.c:1646  */
    break;

  case 223:
#line 1037 "zparser.y" /* yacc.c:1646  */
    {
	   zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str)); /* public key */
    }
#line 3269 "zparser.c" /* yacc.c:1646  */
    break;

  case 225:
#line 1045 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_ilnp64(parser->region, (yyvsp[-1].data).str));  /* NodeID */
    }
#line 3278 "zparser.c" /* yacc.c:1646  */
    break;

  case 226:
#line 1052 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_a(parser->region, (yyvsp[-1].data).str));  /* Locator32 */
    }
#line 3287 "zparser.c" /* yacc.c:1646  */
    break;

  case 227:
#line 1059 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_wireformat(zparser_conv_ilnp64(parser->region, (yyvsp[-1].data).str));  /* Locator64 */
    }
#line 3296 "zparser.c" /* yacc.c:1646  */
    break;

  case 228:
#line 1066 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str));  /* preference */
	    zadd_rdata_domain((yyvsp[-1].domain));  /* FQDN */
    }
#line 3305 "zparser.c" /* yacc.c:1646  */
    break;

  case 229:
#line 1073 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_eui(parser->region, (yyvsp[-1].data).str, 48));
    }
#line 3313 "zparser.c" /* yacc.c:1646  */
    break;

  case 230:
#line 1079 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_eui(parser->region, (yyvsp[-1].data).str, 64));
    }
#line 3321 "zparser.c" /* yacc.c:1646  */
    break;

  case 231:
#line 1086 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-5].data).str)); /* priority */
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-3].data).str)); /* weight */
	    zadd_rdata_wireformat(zparser_conv_long_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* target */
    }
#line 3331 "zparser.c" /* yacc.c:1646  */
    break;

  case 232:
#line 1095 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_byte(parser->region, (yyvsp[-5].data).str)); /* Flags */
	    zadd_rdata_wireformat(zparser_conv_tag(parser->region, (yyvsp[-3].data).str, (yyvsp[-3].data).len)); /* Tag */
	    zadd_rdata_wireformat(zparser_conv_long_text(parser->region, (yyvsp[-1].data).str, (yyvsp[-1].data).len)); /* Value */
    }
#line 3341 "zparser.c" /* yacc.c:1646  */
    break;

  case 233:
#line 1104 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_b64(parser->region, (yyvsp[-1].data).str));
    }
#line 3349 "zparser.c" /* yacc.c:1646  */
    break;

  case 234:
#line 1111 "zparser.y" /* yacc.c:1646  */
    {
	    zadd_rdata_wireformat(zparser_conv_serial(parser->region, (yyvsp[-3].data).str));
	    zadd_rdata_wireformat(zparser_conv_short(parser->region, (yyvsp[-1].data).str));
	    zadd_rdata_wireformat(zparser_conv_nsec(parser->region, nsecbits)); /* nsec bitlist */
	    memset(nsecbits, 0, sizeof(nsecbits));
            nsec_highest_rcode = 0;
    }
#line 3361 "zparser.c" /* yacc.c:1646  */
    break;

  case 235:
#line 1121 "zparser.y" /* yacc.c:1646  */
    {
	    /* $2 is the number of octets, currently ignored */
	    (yyval.unknown) = zparser_conv_hex(parser->rr_region, (yyvsp[-1].data).str, (yyvsp[-1].data).len);

    }
#line 3371 "zparser.c" /* yacc.c:1646  */
    break;

  case 236:
#line 1127 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.unknown) = zparser_conv_hex(parser->rr_region, "", 0);
    }
#line 3379 "zparser.c" /* yacc.c:1646  */
    break;

  case 237:
#line 1131 "zparser.y" /* yacc.c:1646  */
    {
	    (yyval.unknown) = zparser_conv_hex(parser->rr_region, "", 0);
    }
#line 3387 "zparser.c" /* yacc.c:1646  */
    break;


#line 3391 "zparser.c" /* yacc.c:1646  */
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
#line 1135 "zparser.y" /* yacc.c:1906  */


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
