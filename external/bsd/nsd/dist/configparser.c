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
#line 10 "configparser.y" /* yacc.c:339  */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "options.h"
#include "util.h"
#include "dname.h"
#include "tsig.h"
#include "rrl.h"
#include "configyyrename.h"
int c_lex(void);
void c_error(const char *message);

#ifdef __cplusplus
extern "C"
#endif /* __cplusplus */

/* these need to be global, otherwise they cannot be used inside yacc */
extern config_parser_state_type* cfg_parser;

#if 0
#define OUTYY(s) printf s /* used ONLY when debugging */
#else
#define OUTYY(s)
#endif


#line 99 "configparser.c" /* yacc.c:339  */

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
   by #include "configparser.h".  */
#ifndef YY_YY_CONFIGPARSER_H_INCLUDED
# define YY_YY_CONFIGPARSER_H_INCLUDED
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
    SPACE = 258,
    LETTER = 259,
    NEWLINE = 260,
    COMMENT = 261,
    COLON = 262,
    ANY = 263,
    ZONESTR = 264,
    STRING = 265,
    VAR_SERVER = 266,
    VAR_NAME = 267,
    VAR_IP_ADDRESS = 268,
    VAR_IP_TRANSPARENT = 269,
    VAR_DEBUG_MODE = 270,
    VAR_IP4_ONLY = 271,
    VAR_IP6_ONLY = 272,
    VAR_DATABASE = 273,
    VAR_IDENTITY = 274,
    VAR_NSID = 275,
    VAR_LOGFILE = 276,
    VAR_SERVER_COUNT = 277,
    VAR_TCP_COUNT = 278,
    VAR_PIDFILE = 279,
    VAR_PORT = 280,
    VAR_STATISTICS = 281,
    VAR_CHROOT = 282,
    VAR_USERNAME = 283,
    VAR_ZONESDIR = 284,
    VAR_XFRDFILE = 285,
    VAR_DIFFFILE = 286,
    VAR_XFRD_RELOAD_TIMEOUT = 287,
    VAR_TCP_QUERY_COUNT = 288,
    VAR_TCP_TIMEOUT = 289,
    VAR_IPV4_EDNS_SIZE = 290,
    VAR_IPV6_EDNS_SIZE = 291,
    VAR_DO_IP4 = 292,
    VAR_DO_IP6 = 293,
    VAR_TCP_MSS = 294,
    VAR_OUTGOING_TCP_MSS = 295,
    VAR_IP_FREEBIND = 296,
    VAR_ZONEFILE = 297,
    VAR_ZONE = 298,
    VAR_ALLOW_NOTIFY = 299,
    VAR_REQUEST_XFR = 300,
    VAR_NOTIFY = 301,
    VAR_PROVIDE_XFR = 302,
    VAR_SIZE_LIMIT_XFR = 303,
    VAR_NOTIFY_RETRY = 304,
    VAR_OUTGOING_INTERFACE = 305,
    VAR_ALLOW_AXFR_FALLBACK = 306,
    VAR_KEY = 307,
    VAR_ALGORITHM = 308,
    VAR_SECRET = 309,
    VAR_AXFR = 310,
    VAR_UDP = 311,
    VAR_VERBOSITY = 312,
    VAR_HIDE_VERSION = 313,
    VAR_PATTERN = 314,
    VAR_INCLUDEPATTERN = 315,
    VAR_ZONELISTFILE = 316,
    VAR_REMOTE_CONTROL = 317,
    VAR_CONTROL_ENABLE = 318,
    VAR_CONTROL_INTERFACE = 319,
    VAR_CONTROL_PORT = 320,
    VAR_SERVER_KEY_FILE = 321,
    VAR_SERVER_CERT_FILE = 322,
    VAR_CONTROL_KEY_FILE = 323,
    VAR_CONTROL_CERT_FILE = 324,
    VAR_XFRDIR = 325,
    VAR_RRL_SIZE = 326,
    VAR_RRL_RATELIMIT = 327,
    VAR_RRL_SLIP = 328,
    VAR_RRL_IPV4_PREFIX_LENGTH = 329,
    VAR_RRL_IPV6_PREFIX_LENGTH = 330,
    VAR_RRL_WHITELIST_RATELIMIT = 331,
    VAR_RRL_WHITELIST = 332,
    VAR_ZONEFILES_CHECK = 333,
    VAR_ZONEFILES_WRITE = 334,
    VAR_LOG_TIME_ASCII = 335,
    VAR_ROUND_ROBIN = 336,
    VAR_ZONESTATS = 337,
    VAR_REUSEPORT = 338,
    VAR_VERSION = 339,
    VAR_MAX_REFRESH_TIME = 340,
    VAR_MIN_REFRESH_TIME = 341,
    VAR_MAX_RETRY_TIME = 342,
    VAR_MIN_RETRY_TIME = 343,
    VAR_MULTI_MASTER_CHECK = 344,
    VAR_MINIMAL_RESPONSES = 345,
    VAR_REFUSE_ANY = 346,
    VAR_USE_SYSTEMD = 347,
    VAR_DNSTAP = 348,
    VAR_DNSTAP_ENABLE = 349,
    VAR_DNSTAP_SOCKET_PATH = 350,
    VAR_DNSTAP_SEND_IDENTITY = 351,
    VAR_DNSTAP_SEND_VERSION = 352,
    VAR_DNSTAP_IDENTITY = 353,
    VAR_DNSTAP_VERSION = 354,
    VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES = 355,
    VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES = 356
  };
#endif
/* Tokens.  */
#define SPACE 258
#define LETTER 259
#define NEWLINE 260
#define COMMENT 261
#define COLON 262
#define ANY 263
#define ZONESTR 264
#define STRING 265
#define VAR_SERVER 266
#define VAR_NAME 267
#define VAR_IP_ADDRESS 268
#define VAR_IP_TRANSPARENT 269
#define VAR_DEBUG_MODE 270
#define VAR_IP4_ONLY 271
#define VAR_IP6_ONLY 272
#define VAR_DATABASE 273
#define VAR_IDENTITY 274
#define VAR_NSID 275
#define VAR_LOGFILE 276
#define VAR_SERVER_COUNT 277
#define VAR_TCP_COUNT 278
#define VAR_PIDFILE 279
#define VAR_PORT 280
#define VAR_STATISTICS 281
#define VAR_CHROOT 282
#define VAR_USERNAME 283
#define VAR_ZONESDIR 284
#define VAR_XFRDFILE 285
#define VAR_DIFFFILE 286
#define VAR_XFRD_RELOAD_TIMEOUT 287
#define VAR_TCP_QUERY_COUNT 288
#define VAR_TCP_TIMEOUT 289
#define VAR_IPV4_EDNS_SIZE 290
#define VAR_IPV6_EDNS_SIZE 291
#define VAR_DO_IP4 292
#define VAR_DO_IP6 293
#define VAR_TCP_MSS 294
#define VAR_OUTGOING_TCP_MSS 295
#define VAR_IP_FREEBIND 296
#define VAR_ZONEFILE 297
#define VAR_ZONE 298
#define VAR_ALLOW_NOTIFY 299
#define VAR_REQUEST_XFR 300
#define VAR_NOTIFY 301
#define VAR_PROVIDE_XFR 302
#define VAR_SIZE_LIMIT_XFR 303
#define VAR_NOTIFY_RETRY 304
#define VAR_OUTGOING_INTERFACE 305
#define VAR_ALLOW_AXFR_FALLBACK 306
#define VAR_KEY 307
#define VAR_ALGORITHM 308
#define VAR_SECRET 309
#define VAR_AXFR 310
#define VAR_UDP 311
#define VAR_VERBOSITY 312
#define VAR_HIDE_VERSION 313
#define VAR_PATTERN 314
#define VAR_INCLUDEPATTERN 315
#define VAR_ZONELISTFILE 316
#define VAR_REMOTE_CONTROL 317
#define VAR_CONTROL_ENABLE 318
#define VAR_CONTROL_INTERFACE 319
#define VAR_CONTROL_PORT 320
#define VAR_SERVER_KEY_FILE 321
#define VAR_SERVER_CERT_FILE 322
#define VAR_CONTROL_KEY_FILE 323
#define VAR_CONTROL_CERT_FILE 324
#define VAR_XFRDIR 325
#define VAR_RRL_SIZE 326
#define VAR_RRL_RATELIMIT 327
#define VAR_RRL_SLIP 328
#define VAR_RRL_IPV4_PREFIX_LENGTH 329
#define VAR_RRL_IPV6_PREFIX_LENGTH 330
#define VAR_RRL_WHITELIST_RATELIMIT 331
#define VAR_RRL_WHITELIST 332
#define VAR_ZONEFILES_CHECK 333
#define VAR_ZONEFILES_WRITE 334
#define VAR_LOG_TIME_ASCII 335
#define VAR_ROUND_ROBIN 336
#define VAR_ZONESTATS 337
#define VAR_REUSEPORT 338
#define VAR_VERSION 339
#define VAR_MAX_REFRESH_TIME 340
#define VAR_MIN_REFRESH_TIME 341
#define VAR_MAX_RETRY_TIME 342
#define VAR_MIN_RETRY_TIME 343
#define VAR_MULTI_MASTER_CHECK 344
#define VAR_MINIMAL_RESPONSES 345
#define VAR_REFUSE_ANY 346
#define VAR_USE_SYSTEMD 347
#define VAR_DNSTAP 348
#define VAR_DNSTAP_ENABLE 349
#define VAR_DNSTAP_SOCKET_PATH 350
#define VAR_DNSTAP_SEND_IDENTITY 351
#define VAR_DNSTAP_SEND_VERSION 352
#define VAR_DNSTAP_IDENTITY 353
#define VAR_DNSTAP_VERSION 354
#define VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES 355
#define VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES 356

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 42 "configparser.y" /* yacc.c:355  */

	char*	str;

#line 345 "configparser.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_CONFIGPARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 362 "configparser.c" /* yacc.c:358  */

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
#define YYLAST   208

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  102
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  108
/* YYNRULES -- Number of rules.  */
#define YYNRULES  202
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  299

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   356

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
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    81,    81,    81,    82,    82,    83,    83,    84,    84,
      87,    95,    95,    96,    96,    96,    96,    97,    97,    97,
      97,    97,    98,    98,    98,    98,    99,    99,    99,    99,
     100,   100,   100,   101,   101,   101,   102,   102,   102,   103,
     103,   104,   104,   105,   105,   105,   106,   106,   106,   107,
     107,   107,   108,   108,   108,   109,   109,   109,   110,   110,
     110,   111,   133,   141,   149,   157,   162,   170,   178,   190,
     202,   210,   218,   226,   235,   241,   247,   277,   283,   294,
     305,   316,   326,   334,   342,   348,   354,   362,   368,   374,
     380,   386,   392,   398,   404,   412,   420,   428,   436,   444,
     452,   460,   470,   478,   488,   498,   508,   516,   524,   533,
     538,   539,   540,   540,   540,   541,   541,   541,   542,   544,
     552,   560,   577,   583,   589,   595,   603,   608,   609,   610,
     610,   611,   611,   612,   612,   613,   614,   616,   624,   630,
     638,   646,   652,   658,   666,   676,   710,   710,   711,   711,
     712,   712,   712,   713,   713,   713,   714,   714,   714,   715,
     715,   715,   716,   716,   716,   717,   717,   718,   730,   741,
     778,   778,   779,   779,   780,   800,   809,   818,   829,   833,
     841,   853,   866,   880,   893,   904,   915,   927,   938,   946,
     956,   966,   976,   986,   995,  1008,  1008,  1009,  1009,  1009,
    1010,  1024,  1037
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SPACE", "LETTER", "NEWLINE", "COMMENT",
  "COLON", "ANY", "ZONESTR", "STRING", "VAR_SERVER", "VAR_NAME",
  "VAR_IP_ADDRESS", "VAR_IP_TRANSPARENT", "VAR_DEBUG_MODE", "VAR_IP4_ONLY",
  "VAR_IP6_ONLY", "VAR_DATABASE", "VAR_IDENTITY", "VAR_NSID",
  "VAR_LOGFILE", "VAR_SERVER_COUNT", "VAR_TCP_COUNT", "VAR_PIDFILE",
  "VAR_PORT", "VAR_STATISTICS", "VAR_CHROOT", "VAR_USERNAME",
  "VAR_ZONESDIR", "VAR_XFRDFILE", "VAR_DIFFFILE",
  "VAR_XFRD_RELOAD_TIMEOUT", "VAR_TCP_QUERY_COUNT", "VAR_TCP_TIMEOUT",
  "VAR_IPV4_EDNS_SIZE", "VAR_IPV6_EDNS_SIZE", "VAR_DO_IP4", "VAR_DO_IP6",
  "VAR_TCP_MSS", "VAR_OUTGOING_TCP_MSS", "VAR_IP_FREEBIND", "VAR_ZONEFILE",
  "VAR_ZONE", "VAR_ALLOW_NOTIFY", "VAR_REQUEST_XFR", "VAR_NOTIFY",
  "VAR_PROVIDE_XFR", "VAR_SIZE_LIMIT_XFR", "VAR_NOTIFY_RETRY",
  "VAR_OUTGOING_INTERFACE", "VAR_ALLOW_AXFR_FALLBACK", "VAR_KEY",
  "VAR_ALGORITHM", "VAR_SECRET", "VAR_AXFR", "VAR_UDP", "VAR_VERBOSITY",
  "VAR_HIDE_VERSION", "VAR_PATTERN", "VAR_INCLUDEPATTERN",
  "VAR_ZONELISTFILE", "VAR_REMOTE_CONTROL", "VAR_CONTROL_ENABLE",
  "VAR_CONTROL_INTERFACE", "VAR_CONTROL_PORT", "VAR_SERVER_KEY_FILE",
  "VAR_SERVER_CERT_FILE", "VAR_CONTROL_KEY_FILE", "VAR_CONTROL_CERT_FILE",
  "VAR_XFRDIR", "VAR_RRL_SIZE", "VAR_RRL_RATELIMIT", "VAR_RRL_SLIP",
  "VAR_RRL_IPV4_PREFIX_LENGTH", "VAR_RRL_IPV6_PREFIX_LENGTH",
  "VAR_RRL_WHITELIST_RATELIMIT", "VAR_RRL_WHITELIST",
  "VAR_ZONEFILES_CHECK", "VAR_ZONEFILES_WRITE", "VAR_LOG_TIME_ASCII",
  "VAR_ROUND_ROBIN", "VAR_ZONESTATS", "VAR_REUSEPORT", "VAR_VERSION",
  "VAR_MAX_REFRESH_TIME", "VAR_MIN_REFRESH_TIME", "VAR_MAX_RETRY_TIME",
  "VAR_MIN_RETRY_TIME", "VAR_MULTI_MASTER_CHECK", "VAR_MINIMAL_RESPONSES",
  "VAR_REFUSE_ANY", "VAR_USE_SYSTEMD", "VAR_DNSTAP", "VAR_DNSTAP_ENABLE",
  "VAR_DNSTAP_SOCKET_PATH", "VAR_DNSTAP_SEND_IDENTITY",
  "VAR_DNSTAP_SEND_VERSION", "VAR_DNSTAP_IDENTITY", "VAR_DNSTAP_VERSION",
  "VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES",
  "VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES", "$accept", "toplevelvars",
  "toplevelvar", "serverstart", "contents_server", "content_server",
  "server_ip_address", "server_ip_transparent", "server_ip_freebind",
  "server_debug_mode", "server_use_systemd", "server_verbosity",
  "server_hide_version", "server_ip4_only", "server_ip6_only",
  "server_do_ip4", "server_do_ip6", "server_reuseport", "server_database",
  "server_identity", "server_version", "server_nsid", "server_logfile",
  "server_log_time_ascii", "server_round_robin",
  "server_minimal_responses", "server_refuse_any", "server_server_count",
  "server_tcp_count", "server_pidfile", "server_port", "server_statistics",
  "server_chroot", "server_username", "server_zonesdir",
  "server_zonelistfile", "server_xfrdir", "server_difffile",
  "server_xfrdfile", "server_xfrd_reload_timeout",
  "server_tcp_query_count", "server_tcp_timeout", "server_tcp_mss",
  "server_outgoing_tcp_mss", "server_ipv4_edns_size",
  "server_ipv6_edns_size", "server_rrl_size", "server_rrl_ratelimit",
  "server_rrl_slip", "server_rrl_ipv4_prefix_length",
  "server_rrl_ipv6_prefix_length", "server_rrl_whitelist_ratelimit",
  "server_zonefiles_check", "server_zonefiles_write", "rcstart",
  "contents_rc", "content_rc", "rc_control_enable", "rc_control_port",
  "rc_control_interface", "rc_server_key_file", "rc_server_cert_file",
  "rc_control_key_file", "rc_control_cert_file", "dtstart", "contents_dt",
  "content_dt", "dt_dnstap_enable", "dt_dnstap_socket_path",
  "dt_dnstap_send_identity", "dt_dnstap_send_version",
  "dt_dnstap_identity", "dt_dnstap_version",
  "dt_dnstap_log_auth_query_messages",
  "dt_dnstap_log_auth_response_messages", "patternstart",
  "contents_pattern", "content_pattern", "zone_config_item",
  "pattern_name", "include_pattern", "zonestart", "contents_zone",
  "content_zone", "zone_name", "zone_zonefile", "zone_zonestats",
  "zone_allow_notify", "zone_request_xfr", "zone_size_limit_xfr",
  "zone_request_xfr_data", "zone_notify", "zone_notify_retry",
  "zone_provide_xfr", "zone_outgoing_interface",
  "zone_allow_axfr_fallback", "zone_rrl_whitelist",
  "zone_max_refresh_time", "zone_min_refresh_time", "zone_max_retry_time",
  "zone_min_retry_time", "zone_multi_master_check", "keystart",
  "contents_key", "content_key", "key_name", "key_algorithm", "key_secret", YY_NULLPTR
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
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356
};
# endif

#define YYPACT_NINF -57

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-57)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -57,     0,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,    52,    72,    -4,    -1,    79,    10,    -7,
      -6,    -5,    -9,    -3,    31,    32,    34,    35,    41,    43,
      44,    45,    48,    51,    53,    55,    56,    52,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,    57,   -57,
      72,   -57,   -57,    58,    66,    71,    -4,   -57,   -57,   -57,
     -57,    75,    76,    77,    78,    82,    85,   103,   105,   114,
     115,   116,   117,   118,   120,   121,   123,   125,   126,   140,
     141,   142,   143,   145,   146,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   176,   177,
     178,   179,   180,   181,   182,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   183,   184,   185,   186,   187,   188,   189,
     190,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   191,   192,   193,   194,   -57,   195,   196,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   197,   198,   -57,   -57,   -57,   -57
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    10,   169,   194,   145,   109,   126,     3,
      12,   111,   128,     0,     0,     0,     4,     8,     9,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     7,   147,   149,
     148,   158,   150,   160,   151,   152,   165,   153,   154,   155,
     156,   157,   159,   161,   162,   163,   164,   166,     0,   173,
       5,   171,   172,     0,     0,     0,     6,   196,   197,   198,
     199,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    11,
      13,    14,    57,    15,    60,    37,    38,    16,    17,    50,
      51,    55,    18,    19,    56,    20,    21,    53,    54,    58,
      59,    22,    23,    24,    25,    26,    27,    28,    29,    39,
      40,    30,    31,    32,    33,    34,    41,    42,    35,    36,
      43,    44,    45,    46,    47,    48,    49,    52,     0,     0,
       0,     0,     0,     0,     0,   110,   112,   114,   113,   115,
     116,   117,   118,     0,     0,     0,     0,     0,     0,     0,
       0,   127,   129,   130,   131,   132,   133,   134,   135,   136,
     167,   175,     0,     0,     0,     0,   178,     0,     0,   179,
     184,   186,   187,   168,   188,   176,   189,   190,   191,   192,
     193,   146,   174,   170,   200,   201,   202,   195,    61,    62,
      64,    68,    69,    73,    74,    76,    77,    82,    83,    84,
      85,    86,    87,    88,    89,    93,    92,    94,    95,    96,
      99,   100,    70,    71,    97,    98,    63,    66,    67,    90,
      91,   101,   102,   103,   104,   105,   106,   107,   108,    78,
      79,    72,    75,    80,    81,    65,   119,   121,   120,   122,
     123,   124,   125,   137,   138,   139,   140,   141,   142,   143,
     144,   177,   180,     0,     0,   183,   185,   181,   182
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -31,   -12,   -57,
     -57,   -57,   -57,   -51,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,   -57,
     -57,   -57,   -57,   -57,   -56,   -57,   -57,   -57
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     9,    10,    16,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,    11,    17,   175,   176,   177,   178,
     179,   180,   181,   182,    12,    18,   191,   192,   193,   194,
     195,   196,   197,   198,   199,    13,    37,    38,    39,    40,
      41,    14,    60,    61,    62,    42,    43,    44,    45,    46,
     206,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    15,    66,    67,    68,    69,    70
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
       2,   203,    59,   200,   201,   202,   221,   207,    63,   223,
     227,     3,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   208,   209,     4,   210,   211,   204,   205,    59,    64,
      65,   212,     5,   213,   214,   215,   100,   101,   216,     6,
     102,   217,     7,   218,    19,   219,   220,   222,   224,   103,
     104,   105,   106,   107,   108,   109,   225,   110,   111,   112,
     113,   226,   114,   115,    58,   228,   229,   230,   231,   116,
     117,   118,   232,     8,    20,   233,    21,    22,    23,    24,
      25,    26,    27,    28,   183,   184,   185,   186,   187,   188,
     189,   190,    29,   234,    20,   235,    21,    22,    23,    24,
      25,    26,    27,    28,   236,   237,   238,   239,   240,    30,
     241,   242,    29,   243,    31,   244,   245,    32,    33,    34,
      35,    36,   168,   169,   170,   171,   172,   173,   174,    30,
     246,   247,   248,   249,    31,   250,   251,    32,    33,    34,
      35,    36,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298
};

static const yytype_uint8 yycheck[] =
{
       0,    10,    14,    10,    10,    10,    37,    10,    12,    60,
      66,    11,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    10,    10,    43,    10,    10,    55,    56,    60,    53,
      54,    10,    52,    10,    10,    10,    57,    58,    10,    59,
      61,    10,    62,    10,    12,    10,    10,    10,    10,    70,
      71,    72,    73,    74,    75,    76,    10,    78,    79,    80,
      81,    10,    83,    84,    12,    10,    10,    10,    10,    90,
      91,    92,    10,    93,    42,    10,    44,    45,    46,    47,
      48,    49,    50,    51,    94,    95,    96,    97,    98,    99,
     100,   101,    60,    10,    42,    10,    44,    45,    46,    47,
      48,    49,    50,    51,    10,    10,    10,    10,    10,    77,
      10,    10,    60,    10,    82,    10,    10,    85,    86,    87,
      88,    89,    63,    64,    65,    66,    67,    68,    69,    77,
      10,    10,    10,    10,    82,    10,    10,    85,    86,    87,
      88,    89,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   103,     0,    11,    43,    52,    59,    62,    93,   104,
     105,   156,   166,   177,   183,   204,   106,   157,   167,    12,
      42,    44,    45,    46,    47,    48,    49,    50,    51,    60,
      77,    82,    85,    86,    87,    88,    89,   178,   179,   180,
     181,   182,   187,   188,   189,   190,   191,   193,   194,   195,
     196,   197,   198,   199,   200,   201,   202,   203,    12,   180,
     184,   185,   186,    12,    53,    54,   205,   206,   207,   208,
     209,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      57,    58,    61,    70,    71,    72,    73,    74,    75,    76,
      78,    79,    80,    81,    83,    84,    90,    91,    92,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,    63,    64,
      65,    66,    67,    68,    69,   158,   159,   160,   161,   162,
     163,   164,   165,    94,    95,    96,    97,    98,    99,   100,
     101,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      10,    10,    10,    10,    55,    56,   192,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,   179,    10,   185,    10,    10,    10,   206,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   102,   103,   103,   104,   104,   104,   104,   104,   104,
     105,   106,   106,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   157,   158,   158,   158,   158,   158,   158,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   167,   168,
     168,   168,   168,   168,   168,   168,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   178,   179,   179,
     180,   180,   180,   180,   180,   180,   180,   180,   180,   180,
     180,   180,   180,   180,   180,   180,   180,   181,   182,   183,
     184,   184,   185,   185,   186,   187,   188,   189,   190,   191,
     192,   192,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   205,   206,   206,   206,
     207,   208,   209
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       1,     2,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     1,
       2,     0,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     2,
       2,     2,     2,     2,     2,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     1,
       2,     1,     1,     1,     2,     2,     2,     3,     2,     2,
       2,     3,     3,     3,     2,     3,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     2,     1,     1,     1,     1,
       2,     2,     2
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
        case 10:
#line 88 "configparser.y" /* yacc.c:1646  */
    { OUTYY(("\nP(server:)\n")); 
		if(cfg_parser->server_settings_seen) {
			yyerror("duplicate server: element.");
		}
		cfg_parser->server_settings_seen = 1;
	}
#line 1723 "configparser.c" /* yacc.c:1646  */
    break;

  case 61:
#line 112 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_ip_address:%s)\n", (yyvsp[0].str))); 
		if(cfg_parser->current_ip_address_option) {
			cfg_parser->current_ip_address_option->next = 
				(ip_address_option_type*)region_alloc(
				cfg_parser->opt->region, sizeof(ip_address_option_type));
			cfg_parser->current_ip_address_option = 
				cfg_parser->current_ip_address_option->next;
			cfg_parser->current_ip_address_option->next=0;
		} else {
			cfg_parser->current_ip_address_option = 
				(ip_address_option_type*)region_alloc(
				cfg_parser->opt->region, sizeof(ip_address_option_type));
			cfg_parser->current_ip_address_option->next=0;
			cfg_parser->opt->ip_addresses = cfg_parser->current_ip_address_option;
		}

		cfg_parser->current_ip_address_option->address = 
			region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 1748 "configparser.c" /* yacc.c:1646  */
    break;

  case 62:
#line 134 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_ip_transparent:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->ip_transparent = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 1759 "configparser.c" /* yacc.c:1646  */
    break;

  case 63:
#line 142 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_ip_freebind:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->ip_freebind = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 1770 "configparser.c" /* yacc.c:1646  */
    break;

  case 64:
#line 150 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_debug_mode:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->debug_mode = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 1781 "configparser.c" /* yacc.c:1646  */
    break;

  case 65:
#line 158 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_use_systemd:%s)\n", (yyvsp[0].str))); 
	}
#line 1789 "configparser.c" /* yacc.c:1646  */
    break;

  case 66:
#line 163 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_verbosity:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->opt->verbosity = atoi((yyvsp[0].str));
	}
#line 1800 "configparser.c" /* yacc.c:1646  */
    break;

  case 67:
#line 171 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_hide_version:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->hide_version = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 1811 "configparser.c" /* yacc.c:1646  */
    break;

  case 68:
#line 179 "configparser.y" /* yacc.c:1646  */
    { 
		/* for backwards compatibility in config file with NSD3 */
		OUTYY(("P(server_ip4_only:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else if(strcmp((yyvsp[0].str), "yes")==0) {
			cfg_parser->opt->do_ip4 = 1;
			cfg_parser->opt->do_ip6 = 0;
		}
	}
#line 1826 "configparser.c" /* yacc.c:1646  */
    break;

  case 69:
#line 191 "configparser.y" /* yacc.c:1646  */
    { 
		/* for backwards compatibility in config file with NSD3 */
		OUTYY(("P(server_ip6_only:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else if(strcmp((yyvsp[0].str), "yes")==0) {
			cfg_parser->opt->do_ip6 = 1;
			cfg_parser->opt->do_ip4 = 0;
		}
	}
#line 1841 "configparser.c" /* yacc.c:1646  */
    break;

  case 70:
#line 203 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_do_ip4:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->do_ip4 = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 1852 "configparser.c" /* yacc.c:1646  */
    break;

  case 71:
#line 211 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_do_ip6:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->do_ip6 = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 1863 "configparser.c" /* yacc.c:1646  */
    break;

  case 72:
#line 219 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_reuseport:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->reuseport = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 1874 "configparser.c" /* yacc.c:1646  */
    break;

  case 73:
#line 227 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_database:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->database = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
		if(cfg_parser->opt->database[0] == 0 &&
			cfg_parser->opt->zonefiles_write == 0)
			cfg_parser->opt->zonefiles_write = ZONEFILES_WRITE_INTERVAL;
	}
#line 1886 "configparser.c" /* yacc.c:1646  */
    break;

  case 74:
#line 236 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_identity:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->identity = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 1895 "configparser.c" /* yacc.c:1646  */
    break;

  case 75:
#line 242 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_version:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->version = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 1904 "configparser.c" /* yacc.c:1646  */
    break;

  case 76:
#line 248 "configparser.y" /* yacc.c:1646  */
    { 
		unsigned char* nsid = 0;
		size_t nsid_len = 0;

		OUTYY(("P(server_nsid:%s)\n", (yyvsp[0].str)));

		if (strncasecmp((yyvsp[0].str), "ascii_", 6) == 0) {
			nsid_len = strlen((yyvsp[0].str)+6);
			if(nsid_len < 65535) {
				cfg_parser->opt->nsid = region_alloc(cfg_parser->opt->region, nsid_len*2+1);
				hex_ntop((uint8_t*)(yyvsp[0].str)+6, nsid_len, (char*)cfg_parser->opt->nsid, nsid_len*2+1);
			} else
				yyerror("NSID too long");
		} else if (strlen((yyvsp[0].str)) % 2 != 0) {
			yyerror("the NSID must be a hex string of an even length.");
		} else {
			nsid_len = strlen((yyvsp[0].str)) / 2;
			if(nsid_len < 65535) {
				nsid = xalloc(nsid_len);
				if (hex_pton((yyvsp[0].str), nsid, nsid_len) == -1)
					yyerror("hex string cannot be parsed in NSID.");
				else
					cfg_parser->opt->nsid = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
				free(nsid);
			} else
				yyerror("NSID too long");
		}
	}
#line 1937 "configparser.c" /* yacc.c:1646  */
    break;

  case 77:
#line 278 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_logfile:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->logfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 1946 "configparser.c" /* yacc.c:1646  */
    break;

  case 78:
#line 284 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_log_time_ascii:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else {
			cfg_parser->opt->log_time_ascii = (strcmp((yyvsp[0].str), "yes")==0);
			log_time_asc = cfg_parser->opt->log_time_ascii;
		}
	}
#line 1960 "configparser.c" /* yacc.c:1646  */
    break;

  case 79:
#line 295 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_round_robin:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else {
			cfg_parser->opt->round_robin = (strcmp((yyvsp[0].str), "yes")==0);
			round_robin = cfg_parser->opt->round_robin;
		}
	}
#line 1974 "configparser.c" /* yacc.c:1646  */
    break;

  case 80:
#line 306 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_minimal_responses:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else {
			cfg_parser->opt->minimal_responses = (strcmp((yyvsp[0].str), "yes")==0);
			minimal_responses = cfg_parser->opt->minimal_responses;
		}
	}
#line 1988 "configparser.c" /* yacc.c:1646  */
    break;

  case 81:
#line 317 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_refuse_any:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else {
			cfg_parser->opt->refuse_any = (strcmp((yyvsp[0].str), "yes")==0);
		}
	}
#line 2001 "configparser.c" /* yacc.c:1646  */
    break;

  case 82:
#line 327 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_server_count:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) <= 0)
			yyerror("number greater than zero expected");
		else cfg_parser->opt->server_count = atoi((yyvsp[0].str));
	}
#line 2012 "configparser.c" /* yacc.c:1646  */
    break;

  case 83:
#line 335 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_tcp_count:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) <= 0)
			yyerror("number greater than zero expected");
		else cfg_parser->opt->tcp_count = atoi((yyvsp[0].str));
	}
#line 2023 "configparser.c" /* yacc.c:1646  */
    break;

  case 84:
#line 343 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_pidfile:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->pidfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2032 "configparser.c" /* yacc.c:1646  */
    break;

  case 85:
#line 349 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_port:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->port = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2041 "configparser.c" /* yacc.c:1646  */
    break;

  case 86:
#line 355 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_statistics:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->opt->statistics = atoi((yyvsp[0].str));
	}
#line 2052 "configparser.c" /* yacc.c:1646  */
    break;

  case 87:
#line 363 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_chroot:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->chroot = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2061 "configparser.c" /* yacc.c:1646  */
    break;

  case 88:
#line 369 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_username:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->username = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2070 "configparser.c" /* yacc.c:1646  */
    break;

  case 89:
#line 375 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_zonesdir:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->zonesdir = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2079 "configparser.c" /* yacc.c:1646  */
    break;

  case 90:
#line 381 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_zonelistfile:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->zonelistfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2088 "configparser.c" /* yacc.c:1646  */
    break;

  case 91:
#line 387 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_xfrdir:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->xfrdir = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2097 "configparser.c" /* yacc.c:1646  */
    break;

  case 92:
#line 393 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_difffile:%s)\n", (yyvsp[0].str))); 
		/* ignore the value for backwards compatibility in config file*/
	}
#line 2106 "configparser.c" /* yacc.c:1646  */
    break;

  case 93:
#line 399 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_xfrdfile:%s)\n", (yyvsp[0].str))); 
		cfg_parser->opt->xfrdfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2115 "configparser.c" /* yacc.c:1646  */
    break;

  case 94:
#line 405 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_xfrd_reload_timeout:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		cfg_parser->opt->xfrd_reload_timeout = atoi((yyvsp[0].str));
	}
#line 2126 "configparser.c" /* yacc.c:1646  */
    break;

  case 95:
#line 413 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_tcp_query_count:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		cfg_parser->opt->tcp_query_count = atoi((yyvsp[0].str));
	}
#line 2137 "configparser.c" /* yacc.c:1646  */
    break;

  case 96:
#line 421 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_tcp_timeout:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		cfg_parser->opt->tcp_timeout = atoi((yyvsp[0].str));
	}
#line 2148 "configparser.c" /* yacc.c:1646  */
    break;

  case 97:
#line 429 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(server_tcp_mss:%s)\n", (yyvsp[0].str)));
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		cfg_parser->opt->tcp_mss = atoi((yyvsp[0].str));
	}
#line 2159 "configparser.c" /* yacc.c:1646  */
    break;

  case 98:
#line 437 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(server_outgoing_tcp_mss:%s)\n", (yyvsp[0].str)));
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		cfg_parser->opt->outgoing_tcp_mss = atoi((yyvsp[0].str));
	}
#line 2170 "configparser.c" /* yacc.c:1646  */
    break;

  case 99:
#line 445 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_ipv4_edns_size:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		cfg_parser->opt->ipv4_edns_size = atoi((yyvsp[0].str));
	}
#line 2181 "configparser.c" /* yacc.c:1646  */
    break;

  case 100:
#line 453 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_ipv6_edns_size:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		cfg_parser->opt->ipv6_edns_size = atoi((yyvsp[0].str));
	}
#line 2192 "configparser.c" /* yacc.c:1646  */
    break;

  case 101:
#line 461 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_rrl_size:%s)\n", (yyvsp[0].str))); 
#ifdef RATELIMIT
		if(atoi((yyvsp[0].str)) <= 0)
			yyerror("number greater than zero expected");
		cfg_parser->opt->rrl_size = atoi((yyvsp[0].str));
#endif
	}
#line 2205 "configparser.c" /* yacc.c:1646  */
    break;

  case 102:
#line 471 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_rrl_ratelimit:%s)\n", (yyvsp[0].str))); 
#ifdef RATELIMIT
		cfg_parser->opt->rrl_ratelimit = atoi((yyvsp[0].str));
#endif
	}
#line 2216 "configparser.c" /* yacc.c:1646  */
    break;

  case 103:
#line 479 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_rrl_slip:%s)\n", (yyvsp[0].str))); 
#ifdef RATELIMIT
		if(atoi((yyvsp[0].str)) < 0)
			yyerror("number equal or greater than zero expected");
		cfg_parser->opt->rrl_slip = atoi((yyvsp[0].str));
#endif
	}
#line 2229 "configparser.c" /* yacc.c:1646  */
    break;

  case 104:
#line 489 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(server_rrl_ipv4_prefix_length:%s)\n", (yyvsp[0].str))); 
#ifdef RATELIMIT
		if(atoi((yyvsp[0].str)) < 0 || atoi((yyvsp[0].str)) > 32)
			yyerror("invalid IPv4 prefix length");
		cfg_parser->opt->rrl_ipv4_prefix_length = atoi((yyvsp[0].str));
#endif
	}
#line 2242 "configparser.c" /* yacc.c:1646  */
    break;

  case 105:
#line 499 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(server_rrl_ipv6_prefix_length:%s)\n", (yyvsp[0].str))); 
#ifdef RATELIMIT
		if(atoi((yyvsp[0].str)) < 0 || atoi((yyvsp[0].str)) > 64)
			yyerror("invalid IPv6 prefix length");
		cfg_parser->opt->rrl_ipv6_prefix_length = atoi((yyvsp[0].str));
#endif
	}
#line 2255 "configparser.c" /* yacc.c:1646  */
    break;

  case 106:
#line 509 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_rrl_whitelist_ratelimit:%s)\n", (yyvsp[0].str))); 
#ifdef RATELIMIT
		cfg_parser->opt->rrl_whitelist_ratelimit = atoi((yyvsp[0].str));
#endif
	}
#line 2266 "configparser.c" /* yacc.c:1646  */
    break;

  case 107:
#line 517 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_zonefiles_check:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->zonefiles_check = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 2277 "configparser.c" /* yacc.c:1646  */
    break;

  case 108:
#line 525 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(server_zonefiles_write:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->opt->zonefiles_write = atoi((yyvsp[0].str));
	}
#line 2288 "configparser.c" /* yacc.c:1646  */
    break;

  case 109:
#line 534 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("\nP(remote-control:)\n"));
	}
#line 2296 "configparser.c" /* yacc.c:1646  */
    break;

  case 119:
#line 545 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(control_enable:%s)\n", (yyvsp[0].str)));
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->control_enable = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 2307 "configparser.c" /* yacc.c:1646  */
    break;

  case 120:
#line 553 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(control_port:%s)\n", (yyvsp[0].str)));
		if(atoi((yyvsp[0].str)) == 0)
			yyerror("control port number expected");
		else cfg_parser->opt->control_port = atoi((yyvsp[0].str));
	}
#line 2318 "configparser.c" /* yacc.c:1646  */
    break;

  case 121:
#line 561 "configparser.y" /* yacc.c:1646  */
    {
		ip_address_option_type* last = NULL;
		ip_address_option_type* o = (ip_address_option_type*)region_alloc(
			cfg_parser->opt->region, sizeof(ip_address_option_type));
		OUTYY(("P(control_interface:%s)\n", (yyvsp[0].str)));
		/* append at end */
		last = cfg_parser->opt->control_interface;
		while(last && last->next)
			last = last->next;
		if(last == NULL)
			cfg_parser->opt->control_interface = o;
		else	last->next = o;
		o->next = NULL;
		o->address = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2338 "configparser.c" /* yacc.c:1646  */
    break;

  case 122:
#line 578 "configparser.y" /* yacc.c:1646  */
    {
	OUTYY(("P(rc_server_key_file:%s)\n", (yyvsp[0].str)));
	cfg_parser->opt->server_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2347 "configparser.c" /* yacc.c:1646  */
    break;

  case 123:
#line 584 "configparser.y" /* yacc.c:1646  */
    {
	OUTYY(("P(rc_server_cert_file:%s)\n", (yyvsp[0].str)));
	cfg_parser->opt->server_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2356 "configparser.c" /* yacc.c:1646  */
    break;

  case 124:
#line 590 "configparser.y" /* yacc.c:1646  */
    {
	OUTYY(("P(rc_control_key_file:%s)\n", (yyvsp[0].str)));
	cfg_parser->opt->control_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2365 "configparser.c" /* yacc.c:1646  */
    break;

  case 125:
#line 596 "configparser.y" /* yacc.c:1646  */
    {
	OUTYY(("P(rc_control_cert_file:%s)\n", (yyvsp[0].str)));
	cfg_parser->opt->control_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2374 "configparser.c" /* yacc.c:1646  */
    break;

  case 126:
#line 604 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("\nP(dnstap:)\n"));
	}
#line 2382 "configparser.c" /* yacc.c:1646  */
    break;

  case 137:
#line 617 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(dt_dnstap_enable:%s)\n", (yyvsp[0].str)));
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->dnstap_enable = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 2393 "configparser.c" /* yacc.c:1646  */
    break;

  case 138:
#line 625 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(dt_dnstap_socket_path:%s)\n", (yyvsp[0].str)));
		cfg_parser->opt->dnstap_socket_path = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2402 "configparser.c" /* yacc.c:1646  */
    break;

  case 139:
#line 631 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(dt_dnstap_send_identity:%s)\n", (yyvsp[0].str)));
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->dnstap_send_identity = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 2413 "configparser.c" /* yacc.c:1646  */
    break;

  case 140:
#line 639 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(dt_dnstap_send_version:%s)\n", (yyvsp[0].str)));
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->dnstap_send_version = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 2424 "configparser.c" /* yacc.c:1646  */
    break;

  case 141:
#line 647 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(dt_dnstap_identity:%s)\n", (yyvsp[0].str)));
		cfg_parser->opt->dnstap_identity = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2433 "configparser.c" /* yacc.c:1646  */
    break;

  case 142:
#line 653 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(dt_dnstap_version:%s)\n", (yyvsp[0].str)));
		cfg_parser->opt->dnstap_version = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2442 "configparser.c" /* yacc.c:1646  */
    break;

  case 143:
#line 659 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(dt_dnstap_log_auth_query_messages:%s)\n", (yyvsp[0].str)));
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->dnstap_log_auth_query_messages = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 2453 "configparser.c" /* yacc.c:1646  */
    break;

  case 144:
#line 667 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(dt_dnstap_log_auth_response_messages:%s)\n", (yyvsp[0].str)));
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->opt->dnstap_log_auth_response_messages = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 2464 "configparser.c" /* yacc.c:1646  */
    break;

  case 145:
#line 677 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("\nP(pattern:)\n")); 
		if(cfg_parser->current_zone) {
			if(!cfg_parser->current_zone->name) 
				c_error("previous zone has no name");
			else {
				if(!nsd_options_insert_zone(cfg_parser->opt, 
					cfg_parser->current_zone))
					c_error("duplicate zone");
			}
			if(!cfg_parser->current_zone->pattern) 
				c_error("previous zone has no pattern");
			cfg_parser->current_zone = NULL;
		}
		if(cfg_parser->current_pattern) {
			if(!cfg_parser->current_pattern->pname) 
				c_error("previous pattern has no name");
			else {
				if(!nsd_options_insert_pattern(cfg_parser->opt, 
					cfg_parser->current_pattern))
					c_error_msg("duplicate pattern %s",
						cfg_parser->current_pattern->pname);
			}
		}
		cfg_parser->current_pattern = pattern_options_create(
			cfg_parser->opt->region);
		cfg_parser->current_allow_notify = 0;
		cfg_parser->current_request_xfr = 0;
		cfg_parser->current_notify = 0;
		cfg_parser->current_provide_xfr = 0;
		cfg_parser->current_outgoing_interface = 0;
	}
#line 2501 "configparser.c" /* yacc.c:1646  */
    break;

  case 167:
#line 719 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(pattern_name:%s)\n", (yyvsp[0].str))); 
#ifndef NDEBUG
		assert(cfg_parser->current_pattern);
#endif
		if(strchr((yyvsp[0].str), ' '))
			c_error_msg("space is not allowed in pattern name: "
				"'%s'", (yyvsp[0].str));
		cfg_parser->current_pattern->pname = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2516 "configparser.c" /* yacc.c:1646  */
    break;

  case 168:
#line 731 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(include-pattern:%s)\n", (yyvsp[0].str))); 
#ifndef NDEBUG
		assert(cfg_parser->current_pattern);
#endif
		config_apply_pattern((yyvsp[0].str));
	}
#line 2528 "configparser.c" /* yacc.c:1646  */
    break;

  case 169:
#line 742 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("\nP(zone:)\n")); 
		if(cfg_parser->current_zone) {
			if(!cfg_parser->current_zone->name) 
				c_error("previous zone has no name");
			else {
				if(!nsd_options_insert_zone(cfg_parser->opt, 
					cfg_parser->current_zone))
					c_error("duplicate zone");
			}
			if(!cfg_parser->current_zone->pattern) 
				c_error("previous zone has no pattern");
		}
		if(cfg_parser->current_pattern) {
			if(!cfg_parser->current_pattern->pname) 
				c_error("previous pattern has no name");
			else {
				if(!nsd_options_insert_pattern(cfg_parser->opt, 
					cfg_parser->current_pattern))
					c_error_msg("duplicate pattern %s",
						cfg_parser->current_pattern->pname);
			}
		}
		cfg_parser->current_zone = zone_options_create(cfg_parser->opt->region);
		cfg_parser->current_zone->part_of_config = 1;
		cfg_parser->current_pattern = pattern_options_create(
			cfg_parser->opt->region);
		cfg_parser->current_pattern->implicit = 1;
		cfg_parser->current_zone->pattern = cfg_parser->current_pattern;
		cfg_parser->current_allow_notify = 0;
		cfg_parser->current_request_xfr = 0;
		cfg_parser->current_notify = 0;
		cfg_parser->current_provide_xfr = 0;
		cfg_parser->current_outgoing_interface = 0;
	}
#line 2568 "configparser.c" /* yacc.c:1646  */
    break;

  case 174:
#line 781 "configparser.y" /* yacc.c:1646  */
    { 
		char* s;
		OUTYY(("P(zone_name:%s)\n", (yyvsp[0].str))); 
#ifndef NDEBUG
		assert(cfg_parser->current_zone);
		assert(cfg_parser->current_pattern);
#endif
		cfg_parser->current_zone->name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
		s = (char*)region_alloc(cfg_parser->opt->region,
			strlen((yyvsp[0].str))+strlen(PATTERN_IMPLICIT_MARKER)+1);
		memmove(s, PATTERN_IMPLICIT_MARKER,
			strlen(PATTERN_IMPLICIT_MARKER));
		memmove(s+strlen(PATTERN_IMPLICIT_MARKER), (yyvsp[0].str), strlen((yyvsp[0].str))+1);
		if(pattern_options_find(cfg_parser->opt, s))
			c_error_msg("zone %s cannot be created because "
				"implicit pattern %s already exists", (yyvsp[0].str), s);
		cfg_parser->current_pattern->pname = s;
	}
#line 2591 "configparser.c" /* yacc.c:1646  */
    break;

  case 175:
#line 801 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(zonefile:%s)\n", (yyvsp[0].str))); 
#ifndef NDEBUG
		assert(cfg_parser->current_pattern);
#endif
		cfg_parser->current_pattern->zonefile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2603 "configparser.c" /* yacc.c:1646  */
    break;

  case 176:
#line 810 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(zonestats:%s)\n", (yyvsp[0].str))); 
#ifndef NDEBUG
		assert(cfg_parser->current_pattern);
#endif
		cfg_parser->current_pattern->zonestats = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	}
#line 2615 "configparser.c" /* yacc.c:1646  */
    break;

  case 177:
#line 819 "configparser.y" /* yacc.c:1646  */
    { 
		acl_options_type* acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
		OUTYY(("P(allow_notify:%s %s)\n", (yyvsp[-1].str), (yyvsp[0].str))); 
		if(cfg_parser->current_allow_notify)
			cfg_parser->current_allow_notify->next = acl;
		else
			cfg_parser->current_pattern->allow_notify = acl;
		cfg_parser->current_allow_notify = acl;
	}
#line 2629 "configparser.c" /* yacc.c:1646  */
    break;

  case 178:
#line 830 "configparser.y" /* yacc.c:1646  */
    {
	}
#line 2636 "configparser.c" /* yacc.c:1646  */
    break;

  case 179:
#line 834 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(size_limit_xfr:%s)\n", (yyvsp[0].str))); 
		if(atoll((yyvsp[0].str)) < 0)
			yyerror("number >= 0 expected");
		else cfg_parser->current_pattern->size_limit_xfr = atoll((yyvsp[0].str));
	}
#line 2647 "configparser.c" /* yacc.c:1646  */
    break;

  case 180:
#line 842 "configparser.y" /* yacc.c:1646  */
    { 
		acl_options_type* acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
		OUTYY(("P(request_xfr:%s %s)\n", (yyvsp[-1].str), (yyvsp[0].str))); 
		if(acl->blocked) c_error("blocked address used for request-xfr");
		if(acl->rangetype!=acl_range_single) c_error("address range used for request-xfr");
		if(cfg_parser->current_request_xfr)
			cfg_parser->current_request_xfr->next = acl;
		else
			cfg_parser->current_pattern->request_xfr = acl;
		cfg_parser->current_request_xfr = acl;
	}
#line 2663 "configparser.c" /* yacc.c:1646  */
    break;

  case 181:
#line 854 "configparser.y" /* yacc.c:1646  */
    { 
		acl_options_type* acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
		acl->use_axfr_only = 1;
		OUTYY(("P(request_xfr:%s %s)\n", (yyvsp[-1].str), (yyvsp[0].str))); 
		if(acl->blocked) c_error("blocked address used for request-xfr");
		if(acl->rangetype!=acl_range_single) c_error("address range used for request-xfr");
		if(cfg_parser->current_request_xfr)
			cfg_parser->current_request_xfr->next = acl;
		else
			cfg_parser->current_pattern->request_xfr = acl;
		cfg_parser->current_request_xfr = acl;
	}
#line 2680 "configparser.c" /* yacc.c:1646  */
    break;

  case 182:
#line 867 "configparser.y" /* yacc.c:1646  */
    { 
		acl_options_type* acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
		acl->allow_udp = 1;
		OUTYY(("P(request_xfr:%s %s)\n", (yyvsp[-1].str), (yyvsp[0].str))); 
		if(acl->blocked) c_error("blocked address used for request-xfr");
		if(acl->rangetype!=acl_range_single) c_error("address range used for request-xfr");
		if(cfg_parser->current_request_xfr)
			cfg_parser->current_request_xfr->next = acl;
		else
			cfg_parser->current_pattern->request_xfr = acl;
		cfg_parser->current_request_xfr = acl;
	}
#line 2697 "configparser.c" /* yacc.c:1646  */
    break;

  case 183:
#line 881 "configparser.y" /* yacc.c:1646  */
    { 
		acl_options_type* acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
		OUTYY(("P(notify:%s %s)\n", (yyvsp[-1].str), (yyvsp[0].str))); 
		if(acl->blocked) c_error("blocked address used for notify");
		if(acl->rangetype!=acl_range_single) c_error("address range used for notify");
		if(cfg_parser->current_notify)
			cfg_parser->current_notify->next = acl;
		else
			cfg_parser->current_pattern->notify = acl;
		cfg_parser->current_notify = acl;
	}
#line 2713 "configparser.c" /* yacc.c:1646  */
    break;

  case 184:
#line 894 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(notify_retry:%s)\n", (yyvsp[0].str))); 
		if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
			yyerror("number expected");
		else {
			cfg_parser->current_pattern->notify_retry = atoi((yyvsp[0].str));
			cfg_parser->current_pattern->notify_retry_is_default=0;
		}
	}
#line 2727 "configparser.c" /* yacc.c:1646  */
    break;

  case 185:
#line 905 "configparser.y" /* yacc.c:1646  */
    { 
		acl_options_type* acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
		OUTYY(("P(provide_xfr:%s %s)\n", (yyvsp[-1].str), (yyvsp[0].str))); 
		if(cfg_parser->current_provide_xfr)
			cfg_parser->current_provide_xfr->next = acl;
		else
			cfg_parser->current_pattern->provide_xfr = acl;
		cfg_parser->current_provide_xfr = acl;
	}
#line 2741 "configparser.c" /* yacc.c:1646  */
    break;

  case 186:
#line 916 "configparser.y" /* yacc.c:1646  */
    { 
		acl_options_type* acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[0].str), "NOKEY");
		OUTYY(("P(outgoing_interface:%s)\n", (yyvsp[0].str))); 
		if(acl->rangetype!=acl_range_single) c_error("address range used for outgoing interface");
		if(cfg_parser->current_outgoing_interface)
			cfg_parser->current_outgoing_interface->next = acl;
		else
			cfg_parser->current_pattern->outgoing_interface = acl;
		cfg_parser->current_outgoing_interface = acl;
	}
#line 2756 "configparser.c" /* yacc.c:1646  */
    break;

  case 187:
#line 928 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(allow_axfr_fallback:%s)\n", (yyvsp[0].str))); 
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else {
			cfg_parser->current_pattern->allow_axfr_fallback = (strcmp((yyvsp[0].str), "yes")==0);
			cfg_parser->current_pattern->allow_axfr_fallback_is_default = 0;
		}
	}
#line 2770 "configparser.c" /* yacc.c:1646  */
    break;

  case 188:
#line 939 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(zone_rrl_whitelist:%s)\n", (yyvsp[0].str))); 
#ifdef RATELIMIT
		cfg_parser->current_pattern->rrl_whitelist |= rrlstr2type((yyvsp[0].str));
#endif
	}
#line 2781 "configparser.c" /* yacc.c:1646  */
    break;

  case 189:
#line 947 "configparser.y" /* yacc.c:1646  */
    {
	OUTYY(("P(zone_max_refresh_time:%s)\n", (yyvsp[0].str)));
	if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
		yyerror("number expected");
	else {
		cfg_parser->current_pattern->max_refresh_time = atoi((yyvsp[0].str));
		cfg_parser->current_pattern->max_refresh_time_is_default = 0;
	}
}
#line 2795 "configparser.c" /* yacc.c:1646  */
    break;

  case 190:
#line 957 "configparser.y" /* yacc.c:1646  */
    {
	OUTYY(("P(zone_min_refresh_time:%s)\n", (yyvsp[0].str)));
	if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
		yyerror("number expected");
	else {
		cfg_parser->current_pattern->min_refresh_time = atoi((yyvsp[0].str));
		cfg_parser->current_pattern->min_refresh_time_is_default = 0;
	}
}
#line 2809 "configparser.c" /* yacc.c:1646  */
    break;

  case 191:
#line 967 "configparser.y" /* yacc.c:1646  */
    {
	OUTYY(("P(zone_max_retry_time:%s)\n", (yyvsp[0].str)));
	if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
		yyerror("number expected");
	else {
		cfg_parser->current_pattern->max_retry_time = atoi((yyvsp[0].str));
		cfg_parser->current_pattern->max_retry_time_is_default = 0;
	}
}
#line 2823 "configparser.c" /* yacc.c:1646  */
    break;

  case 192:
#line 977 "configparser.y" /* yacc.c:1646  */
    {
	OUTYY(("P(zone_min_retry_time:%s)\n", (yyvsp[0].str)));
	if(atoi((yyvsp[0].str)) == 0 && strcmp((yyvsp[0].str), "0") != 0)
		yyerror("number expected");
	else {
		cfg_parser->current_pattern->min_retry_time = atoi((yyvsp[0].str));
		cfg_parser->current_pattern->min_retry_time_is_default = 0;
	}
}
#line 2837 "configparser.c" /* yacc.c:1646  */
    break;

  case 193:
#line 987 "configparser.y" /* yacc.c:1646  */
    {
		OUTYY(("P(zone_multi_master_check:%s)\n", (yyvsp[0].str)));
		if(strcmp((yyvsp[0].str), "yes") != 0 && strcmp((yyvsp[0].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->current_pattern->multi_master_check = (strcmp((yyvsp[0].str), "yes")==0);
	}
#line 2848 "configparser.c" /* yacc.c:1646  */
    break;

  case 194:
#line 996 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("\nP(key:)\n")); 
		if(cfg_parser->current_key) {
			if(!cfg_parser->current_key->name) c_error("previous key has no name");
			if(!cfg_parser->current_key->algorithm) c_error("previous key has no algorithm");
			if(!cfg_parser->current_key->secret) c_error("previous key has no secret blob");
			key_options_insert(cfg_parser->opt, cfg_parser->current_key);
		}
		cfg_parser->current_key = key_options_create(cfg_parser->opt->region);
		cfg_parser->current_key->algorithm = region_strdup(cfg_parser->opt->region, "sha256");
	}
#line 2864 "configparser.c" /* yacc.c:1646  */
    break;

  case 200:
#line 1011 "configparser.y" /* yacc.c:1646  */
    { 
		const dname_type* d;
		OUTYY(("P(key_name:%s)\n", (yyvsp[0].str))); 
#ifndef NDEBUG
		assert(cfg_parser->current_key);
#endif
		cfg_parser->current_key->name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
		d = dname_parse(cfg_parser->opt->region, (yyvsp[0].str));
		if(!d)	c_error_msg("Failed to parse tsig key name %s", (yyvsp[0].str));
		else	region_recycle(cfg_parser->opt->region, (void*)d,
				dname_total_size(d));
	}
#line 2881 "configparser.c" /* yacc.c:1646  */
    break;

  case 201:
#line 1025 "configparser.y" /* yacc.c:1646  */
    { 
		OUTYY(("P(key_algorithm:%s)\n", (yyvsp[0].str))); 
#ifndef NDEBUG
		assert(cfg_parser->current_key);
#endif
		if(cfg_parser->current_key->algorithm)
			region_recycle(cfg_parser->opt->region, cfg_parser->current_key->algorithm, strlen(cfg_parser->current_key->algorithm)+1);
		cfg_parser->current_key->algorithm = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
		if(tsig_get_algorithm_by_name((yyvsp[0].str)) == NULL)
			c_error_msg("Bad tsig algorithm %s", (yyvsp[0].str));
	}
#line 2897 "configparser.c" /* yacc.c:1646  */
    break;

  case 202:
#line 1038 "configparser.y" /* yacc.c:1646  */
    { 
		uint8_t data[16384];
		int size;
		OUTYY(("key_secret:%s)\n", (yyvsp[0].str))); 
#ifndef NDEBUG
		assert(cfg_parser->current_key);
#endif
		cfg_parser->current_key->secret = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
		size = b64_pton((yyvsp[0].str), data, sizeof(data));
		if(size == -1) {
			c_error_msg("Cannot base64 decode tsig secret %s",
				cfg_parser->current_key->name?
				cfg_parser->current_key->name:"");
		} else if(size != 0) {
			memset(data, 0xdd, size); /* wipe secret */
		}
	}
#line 2919 "configparser.c" /* yacc.c:1646  */
    break;


#line 2923 "configparser.c" /* yacc.c:1646  */
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
#line 1057 "configparser.y" /* yacc.c:1906  */


/* parse helper routines could be here */
