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
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 10 "configparser.y" /* yacc.c:339  */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "options.h"
#include "util.h"
#include "dname.h"
#include "tsig.h"
#include "rrl.h"
#include "configyyrename.h"

int yylex(void);

#ifdef __cplusplus
extern "C"
#endif

/* these need to be global, otherwise they cannot be used inside yacc */
extern config_parser_state_type *cfg_parser;

static void append_acl(struct acl_options **list, struct acl_options *acl);

#line 93 "configparser.c" /* yacc.c:339  */

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
    STRING = 258,
    VAR_SERVER = 259,
    VAR_SERVER_COUNT = 260,
    VAR_IP_ADDRESS = 261,
    VAR_IP_TRANSPARENT = 262,
    VAR_IP_FREEBIND = 263,
    VAR_REUSEPORT = 264,
    VAR_SEND_BUFFER_SIZE = 265,
    VAR_RECEIVE_BUFFER_SIZE = 266,
    VAR_DEBUG_MODE = 267,
    VAR_IP4_ONLY = 268,
    VAR_IP6_ONLY = 269,
    VAR_DO_IP4 = 270,
    VAR_DO_IP6 = 271,
    VAR_PORT = 272,
    VAR_USE_SYSTEMD = 273,
    VAR_VERBOSITY = 274,
    VAR_USERNAME = 275,
    VAR_CHROOT = 276,
    VAR_ZONESDIR = 277,
    VAR_ZONELISTFILE = 278,
    VAR_DATABASE = 279,
    VAR_LOGFILE = 280,
    VAR_PIDFILE = 281,
    VAR_DIFFFILE = 282,
    VAR_XFRDFILE = 283,
    VAR_XFRDIR = 284,
    VAR_HIDE_VERSION = 285,
    VAR_HIDE_IDENTITY = 286,
    VAR_VERSION = 287,
    VAR_IDENTITY = 288,
    VAR_NSID = 289,
    VAR_TCP_COUNT = 290,
    VAR_TCP_REJECT_OVERFLOW = 291,
    VAR_TCP_QUERY_COUNT = 292,
    VAR_TCP_TIMEOUT = 293,
    VAR_TCP_MSS = 294,
    VAR_OUTGOING_TCP_MSS = 295,
    VAR_IPV4_EDNS_SIZE = 296,
    VAR_IPV6_EDNS_SIZE = 297,
    VAR_STATISTICS = 298,
    VAR_XFRD_RELOAD_TIMEOUT = 299,
    VAR_LOG_TIME_ASCII = 300,
    VAR_ROUND_ROBIN = 301,
    VAR_MINIMAL_RESPONSES = 302,
    VAR_CONFINE_TO_ZONE = 303,
    VAR_REFUSE_ANY = 304,
    VAR_ZONEFILES_CHECK = 305,
    VAR_ZONEFILES_WRITE = 306,
    VAR_RRL_SIZE = 307,
    VAR_RRL_RATELIMIT = 308,
    VAR_RRL_SLIP = 309,
    VAR_RRL_IPV4_PREFIX_LENGTH = 310,
    VAR_RRL_IPV6_PREFIX_LENGTH = 311,
    VAR_RRL_WHITELIST_RATELIMIT = 312,
    VAR_TLS_SERVICE_KEY = 313,
    VAR_TLS_SERVICE_PEM = 314,
    VAR_TLS_SERVICE_OCSP = 315,
    VAR_TLS_PORT = 316,
    VAR_DNSTAP = 317,
    VAR_DNSTAP_ENABLE = 318,
    VAR_DNSTAP_SOCKET_PATH = 319,
    VAR_DNSTAP_SEND_IDENTITY = 320,
    VAR_DNSTAP_SEND_VERSION = 321,
    VAR_DNSTAP_IDENTITY = 322,
    VAR_DNSTAP_VERSION = 323,
    VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES = 324,
    VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES = 325,
    VAR_REMOTE_CONTROL = 326,
    VAR_CONTROL_ENABLE = 327,
    VAR_CONTROL_INTERFACE = 328,
    VAR_CONTROL_PORT = 329,
    VAR_SERVER_KEY_FILE = 330,
    VAR_SERVER_CERT_FILE = 331,
    VAR_CONTROL_KEY_FILE = 332,
    VAR_CONTROL_CERT_FILE = 333,
    VAR_KEY = 334,
    VAR_ALGORITHM = 335,
    VAR_SECRET = 336,
    VAR_PATTERN = 337,
    VAR_NAME = 338,
    VAR_ZONEFILE = 339,
    VAR_NOTIFY = 340,
    VAR_PROVIDE_XFR = 341,
    VAR_AXFR = 342,
    VAR_UDP = 343,
    VAR_NOTIFY_RETRY = 344,
    VAR_ALLOW_NOTIFY = 345,
    VAR_REQUEST_XFR = 346,
    VAR_ALLOW_AXFR_FALLBACK = 347,
    VAR_OUTGOING_INTERFACE = 348,
    VAR_MAX_REFRESH_TIME = 349,
    VAR_MIN_REFRESH_TIME = 350,
    VAR_MAX_RETRY_TIME = 351,
    VAR_MIN_RETRY_TIME = 352,
    VAR_MULTI_MASTER_CHECK = 353,
    VAR_SIZE_LIMIT_XFR = 354,
    VAR_ZONESTATS = 355,
    VAR_INCLUDE_PATTERN = 356,
    VAR_ZONE = 357,
    VAR_RRL_WHITELIST = 358
  };
#endif
/* Tokens.  */
#define STRING 258
#define VAR_SERVER 259
#define VAR_SERVER_COUNT 260
#define VAR_IP_ADDRESS 261
#define VAR_IP_TRANSPARENT 262
#define VAR_IP_FREEBIND 263
#define VAR_REUSEPORT 264
#define VAR_SEND_BUFFER_SIZE 265
#define VAR_RECEIVE_BUFFER_SIZE 266
#define VAR_DEBUG_MODE 267
#define VAR_IP4_ONLY 268
#define VAR_IP6_ONLY 269
#define VAR_DO_IP4 270
#define VAR_DO_IP6 271
#define VAR_PORT 272
#define VAR_USE_SYSTEMD 273
#define VAR_VERBOSITY 274
#define VAR_USERNAME 275
#define VAR_CHROOT 276
#define VAR_ZONESDIR 277
#define VAR_ZONELISTFILE 278
#define VAR_DATABASE 279
#define VAR_LOGFILE 280
#define VAR_PIDFILE 281
#define VAR_DIFFFILE 282
#define VAR_XFRDFILE 283
#define VAR_XFRDIR 284
#define VAR_HIDE_VERSION 285
#define VAR_HIDE_IDENTITY 286
#define VAR_VERSION 287
#define VAR_IDENTITY 288
#define VAR_NSID 289
#define VAR_TCP_COUNT 290
#define VAR_TCP_REJECT_OVERFLOW 291
#define VAR_TCP_QUERY_COUNT 292
#define VAR_TCP_TIMEOUT 293
#define VAR_TCP_MSS 294
#define VAR_OUTGOING_TCP_MSS 295
#define VAR_IPV4_EDNS_SIZE 296
#define VAR_IPV6_EDNS_SIZE 297
#define VAR_STATISTICS 298
#define VAR_XFRD_RELOAD_TIMEOUT 299
#define VAR_LOG_TIME_ASCII 300
#define VAR_ROUND_ROBIN 301
#define VAR_MINIMAL_RESPONSES 302
#define VAR_CONFINE_TO_ZONE 303
#define VAR_REFUSE_ANY 304
#define VAR_ZONEFILES_CHECK 305
#define VAR_ZONEFILES_WRITE 306
#define VAR_RRL_SIZE 307
#define VAR_RRL_RATELIMIT 308
#define VAR_RRL_SLIP 309
#define VAR_RRL_IPV4_PREFIX_LENGTH 310
#define VAR_RRL_IPV6_PREFIX_LENGTH 311
#define VAR_RRL_WHITELIST_RATELIMIT 312
#define VAR_TLS_SERVICE_KEY 313
#define VAR_TLS_SERVICE_PEM 314
#define VAR_TLS_SERVICE_OCSP 315
#define VAR_TLS_PORT 316
#define VAR_DNSTAP 317
#define VAR_DNSTAP_ENABLE 318
#define VAR_DNSTAP_SOCKET_PATH 319
#define VAR_DNSTAP_SEND_IDENTITY 320
#define VAR_DNSTAP_SEND_VERSION 321
#define VAR_DNSTAP_IDENTITY 322
#define VAR_DNSTAP_VERSION 323
#define VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES 324
#define VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES 325
#define VAR_REMOTE_CONTROL 326
#define VAR_CONTROL_ENABLE 327
#define VAR_CONTROL_INTERFACE 328
#define VAR_CONTROL_PORT 329
#define VAR_SERVER_KEY_FILE 330
#define VAR_SERVER_CERT_FILE 331
#define VAR_CONTROL_KEY_FILE 332
#define VAR_CONTROL_CERT_FILE 333
#define VAR_KEY 334
#define VAR_ALGORITHM 335
#define VAR_SECRET 336
#define VAR_PATTERN 337
#define VAR_NAME 338
#define VAR_ZONEFILE 339
#define VAR_NOTIFY 340
#define VAR_PROVIDE_XFR 341
#define VAR_AXFR 342
#define VAR_UDP 343
#define VAR_NOTIFY_RETRY 344
#define VAR_ALLOW_NOTIFY 345
#define VAR_REQUEST_XFR 346
#define VAR_ALLOW_AXFR_FALLBACK 347
#define VAR_OUTGOING_INTERFACE 348
#define VAR_MAX_REFRESH_TIME 349
#define VAR_MIN_REFRESH_TIME 350
#define VAR_MAX_RETRY_TIME 351
#define VAR_MIN_RETRY_TIME 352
#define VAR_MULTI_MASTER_CHECK 353
#define VAR_SIZE_LIMIT_XFR 354
#define VAR_ZONESTATS 355
#define VAR_INCLUDE_PATTERN 356
#define VAR_ZONE 357
#define VAR_RRL_WHITELIST 358

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 37 "configparser.y" /* yacc.c:355  */

  char *str;
  long long llng;
  int bln;
  struct ip_address_option *ip;

#line 346 "configparser.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_CONFIGPARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 363 "configparser.c" /* yacc.c:358  */

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
#define YYLAST   266

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  104
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  28
/* YYNRULES -- Number of rules.  */
#define YYNRULES  131
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  234

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   358

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
      95,    96,    97,    98,    99,   100,   101,   102,   103
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   163,   163,   165,   168,   169,   170,   171,   172,   173,
     176,   179,   179,   182,   192,   200,   202,   204,   206,   208,
     210,   212,   214,   216,   218,   220,   222,   224,   233,   235,
     237,   267,   269,   277,   279,   281,   283,   285,   287,   289,
     291,   293,   300,   302,   304,   306,   308,   310,   312,   314,
     316,   318,   320,   322,   332,   338,   344,   354,   364,   370,
     372,   374,   379,   384,   389,   391,   393,   395,   397,   399,
     409,   412,   412,   415,   417,   419,   421,   423,   425,   427,
     429,   434,   437,   437,   440,   442,   452,   460,   462,   464,
     466,   472,   471,   496,   496,   499,   511,   519,   538,   537,
     562,   562,   565,   578,   582,   581,   598,   598,   601,   608,
     611,   617,   619,   621,   629,   631,   633,   642,   652,   662,
     667,   676,   681,   686,   691,   696,   701,   706,   711,   718,
     727,   749
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "STRING", "VAR_SERVER",
  "VAR_SERVER_COUNT", "VAR_IP_ADDRESS", "VAR_IP_TRANSPARENT",
  "VAR_IP_FREEBIND", "VAR_REUSEPORT", "VAR_SEND_BUFFER_SIZE",
  "VAR_RECEIVE_BUFFER_SIZE", "VAR_DEBUG_MODE", "VAR_IP4_ONLY",
  "VAR_IP6_ONLY", "VAR_DO_IP4", "VAR_DO_IP6", "VAR_PORT",
  "VAR_USE_SYSTEMD", "VAR_VERBOSITY", "VAR_USERNAME", "VAR_CHROOT",
  "VAR_ZONESDIR", "VAR_ZONELISTFILE", "VAR_DATABASE", "VAR_LOGFILE",
  "VAR_PIDFILE", "VAR_DIFFFILE", "VAR_XFRDFILE", "VAR_XFRDIR",
  "VAR_HIDE_VERSION", "VAR_HIDE_IDENTITY", "VAR_VERSION", "VAR_IDENTITY",
  "VAR_NSID", "VAR_TCP_COUNT", "VAR_TCP_REJECT_OVERFLOW",
  "VAR_TCP_QUERY_COUNT", "VAR_TCP_TIMEOUT", "VAR_TCP_MSS",
  "VAR_OUTGOING_TCP_MSS", "VAR_IPV4_EDNS_SIZE", "VAR_IPV6_EDNS_SIZE",
  "VAR_STATISTICS", "VAR_XFRD_RELOAD_TIMEOUT", "VAR_LOG_TIME_ASCII",
  "VAR_ROUND_ROBIN", "VAR_MINIMAL_RESPONSES", "VAR_CONFINE_TO_ZONE",
  "VAR_REFUSE_ANY", "VAR_ZONEFILES_CHECK", "VAR_ZONEFILES_WRITE",
  "VAR_RRL_SIZE", "VAR_RRL_RATELIMIT", "VAR_RRL_SLIP",
  "VAR_RRL_IPV4_PREFIX_LENGTH", "VAR_RRL_IPV6_PREFIX_LENGTH",
  "VAR_RRL_WHITELIST_RATELIMIT", "VAR_TLS_SERVICE_KEY",
  "VAR_TLS_SERVICE_PEM", "VAR_TLS_SERVICE_OCSP", "VAR_TLS_PORT",
  "VAR_DNSTAP", "VAR_DNSTAP_ENABLE", "VAR_DNSTAP_SOCKET_PATH",
  "VAR_DNSTAP_SEND_IDENTITY", "VAR_DNSTAP_SEND_VERSION",
  "VAR_DNSTAP_IDENTITY", "VAR_DNSTAP_VERSION",
  "VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES",
  "VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES", "VAR_REMOTE_CONTROL",
  "VAR_CONTROL_ENABLE", "VAR_CONTROL_INTERFACE", "VAR_CONTROL_PORT",
  "VAR_SERVER_KEY_FILE", "VAR_SERVER_CERT_FILE", "VAR_CONTROL_KEY_FILE",
  "VAR_CONTROL_CERT_FILE", "VAR_KEY", "VAR_ALGORITHM", "VAR_SECRET",
  "VAR_PATTERN", "VAR_NAME", "VAR_ZONEFILE", "VAR_NOTIFY",
  "VAR_PROVIDE_XFR", "VAR_AXFR", "VAR_UDP", "VAR_NOTIFY_RETRY",
  "VAR_ALLOW_NOTIFY", "VAR_REQUEST_XFR", "VAR_ALLOW_AXFR_FALLBACK",
  "VAR_OUTGOING_INTERFACE", "VAR_MAX_REFRESH_TIME", "VAR_MIN_REFRESH_TIME",
  "VAR_MAX_RETRY_TIME", "VAR_MIN_RETRY_TIME", "VAR_MULTI_MASTER_CHECK",
  "VAR_SIZE_LIMIT_XFR", "VAR_ZONESTATS", "VAR_INCLUDE_PATTERN", "VAR_ZONE",
  "VAR_RRL_WHITELIST", "$accept", "blocks", "block", "server",
  "server_block", "server_option", "dnstap", "dnstap_block",
  "dnstap_option", "remote_control", "remote_control_block",
  "remote_control_option", "key", "$@1", "key_block", "key_option", "zone",
  "$@2", "zone_block", "zone_option", "pattern", "$@3", "pattern_block",
  "pattern_option", "pattern_or_zone_option", "ip_address", "number",
  "boolean", YY_NULLPTR
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
     355,   356,   357,   358
};
# endif

#define YYPACT_NINF -52

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-52)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -52,    20,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   205,   -51,   -43,   -52,
     -52,   -52,    -1,     0,     6,     6,     6,    -1,    -1,     6,
       6,     6,     6,     6,    -1,     6,    -1,    18,    22,    23,
      24,    33,    44,    46,    48,    50,    51,     6,     6,    53,
      56,    57,    -1,     6,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     6,     6,     6,     6,     6,     6,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    60,    62,    63,    -1,   -52,
       6,    64,     6,     6,    66,    75,     6,     6,   -52,     6,
       0,    -1,    76,    77,    78,    80,   -52,   -35,    34,    55,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,    89,    90,    94,   -52,    95,
      97,    98,   100,    -1,   101,     8,     6,   102,    -1,    -1,
      -1,    -1,     6,    -1,   103,   105,   106,   -52,   -52,   107,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   108,   109,   -52,
     110,   111,   112,   113,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     118,   133,   -52,   -52
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    12,    72,    83,    91,   104,    98,     3,
       4,     5,     6,     7,     9,     8,    10,    70,    81,    94,
     107,   101,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    11,
       0,     0,     0,     0,     0,     0,     0,     0,    71,     0,
       0,     0,     0,     0,     0,     0,    82,    92,   105,    99,
     130,    14,   129,    13,   131,    15,    16,    42,    17,    18,
      19,    23,    24,    25,    26,    41,    20,    52,    45,    44,
      46,    47,    27,    31,    40,    48,    49,    50,    21,    22,
      29,    28,    30,    32,    33,    34,    35,    36,    37,    38,
      39,    43,    51,    61,    62,    63,    64,    65,    59,    60,
      53,    54,    55,    56,    57,    58,    66,    68,    67,    69,
      73,    74,    75,    76,    77,    78,    79,    80,    84,    85,
      86,    87,    88,    89,    90,     0,     0,     0,    93,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   106,   109,     0,
     100,   103,    96,    97,    95,   108,   111,     0,     0,   124,
       0,     0,     0,     0,   123,   122,   125,   126,   127,   128,
     114,   113,   112,   115,   110,   102,   120,   121,   119,   116,
       0,     0,   117,   118
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,   -52,
     -52,   -52,   -52,   -52,    43,    67,    16,   -25
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     9,    10,    16,    79,    11,    17,    88,    12,
      18,    96,    13,    19,    97,   178,    14,    21,    99,   200,
      15,    20,    98,   197,   198,   103,   101,   105
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
     106,   107,   100,   102,   110,   111,   112,   113,   114,   104,
     116,   211,    80,    81,    82,    83,    84,    85,    86,    87,
       2,   118,   128,   129,     3,   119,   120,   121,   134,    89,
      90,    91,    92,    93,    94,    95,   122,   143,   144,   145,
     146,   147,   148,   108,   109,   175,   176,   123,   177,   124,
     115,   125,   117,   126,   127,   160,   130,   162,   163,   131,
     132,   166,   167,   156,   168,   157,   158,   161,   133,   164,
     135,   136,   137,   138,   139,   140,   141,   142,   165,   171,
     172,   173,     4,   174,   149,   150,   151,   152,   153,   154,
     155,     5,   202,   203,   159,   212,   213,   204,   205,     6,
     206,   207,     7,   208,   210,   215,   222,   170,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   179,   180,   181,
     182,   232,     8,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   233,   196,   199,   180,
     181,   182,   201,     0,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   169,   196,     0,
       0,   214,     0,     0,     0,     0,     0,   220,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   209,
       0,     0,     0,     0,   216,   217,   218,   219,     0,   221,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78
};

static const yytype_int16 yycheck[] =
{
      25,    26,     3,     3,    29,    30,    31,    32,    33,     3,
      35,     3,    63,    64,    65,    66,    67,    68,    69,    70,
       0,     3,    47,    48,     4,     3,     3,     3,    53,    72,
      73,    74,    75,    76,    77,    78,     3,    62,    63,    64,
      65,    66,    67,    27,    28,    80,    81,     3,    83,     3,
      34,     3,    36,     3,     3,    80,     3,    82,    83,     3,
       3,    86,    87,     3,    89,     3,     3,     3,    52,     3,
      54,    55,    56,    57,    58,    59,    60,    61,     3,     3,
       3,     3,    62,     3,    68,    69,    70,    71,    72,    73,
      74,    71,     3,     3,    78,    87,    88,     3,     3,    79,
       3,     3,    82,     3,     3,     3,     3,    91,     3,     3,
       3,     3,     3,     3,     3,     3,     3,    83,    84,    85,
      86,     3,   102,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,     3,   103,    83,    84,
      85,    86,    99,    -1,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,    90,   103,    -1,
      -1,   186,    -1,    -1,    -1,    -1,    -1,   192,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,    -1,    -1,    -1,   188,   189,   190,   191,    -1,   193,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   105,     0,     4,    62,    71,    79,    82,   102,   106,
     107,   110,   113,   116,   120,   124,   108,   111,   114,   117,
     125,   121,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,   109,
      63,    64,    65,    66,    67,    68,    69,    70,   112,    72,
      73,    74,    75,    76,    77,    78,   115,   118,   126,   122,
       3,   130,     3,   129,     3,   131,   131,   131,   130,   130,
     131,   131,   131,   131,   131,   130,   131,   130,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,   131,   131,
       3,     3,     3,   130,   131,   130,   130,   130,   130,   130,
     130,   130,   130,   131,   131,   131,   131,   131,   131,   130,
     130,   130,   130,   130,   130,   130,     3,     3,     3,   130,
     131,     3,   131,   131,     3,     3,   131,   131,   131,   129,
     130,     3,     3,     3,     3,    80,    81,    83,   119,    83,
      84,    85,    86,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   103,   127,   128,    83,
     123,   128,     3,     3,     3,     3,     3,     3,     3,   130,
       3,     3,    87,    88,   131,     3,   130,   130,   130,   130,
     131,   130,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   104,   105,   105,   106,   106,   106,   106,   106,   106,
     107,   108,   108,   109,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     110,   111,   111,   112,   112,   112,   112,   112,   112,   112,
     112,   113,   114,   114,   115,   115,   115,   115,   115,   115,
     115,   117,   116,   118,   118,   119,   119,   119,   121,   120,
     122,   122,   123,   123,   125,   124,   126,   126,   127,   127,
     128,   128,   128,   128,   128,   128,   128,   128,   128,   128,
     128,   128,   128,   128,   128,   128,   128,   128,   128,   129,
     130,   131
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       2,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     0,     2,     2,     2,     2,     2,     2,
       2,     0,     3,     2,     0,     2,     2,     2,     0,     3,
       2,     0,     2,     1,     0,     3,     2,     0,     2,     1,
       2,     2,     2,     2,     2,     2,     3,     4,     4,     3,
       3,     3,     2,     2,     2,     2,     2,     2,     2,     1,
       1,     1
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
        case 13:
#line 183 "configparser.y" /* yacc.c:1648  */
    {
      struct ip_address_option *ip = cfg_parser->opt->ip_addresses;
      if(ip == NULL) {
        cfg_parser->opt->ip_addresses = (yyvsp[0].ip);
      } else {
        while(ip->next) { ip = ip->next; }
        ip->next = (yyvsp[0].ip);
      }
    }
#line 1659 "configparser.c" /* yacc.c:1648  */
    break;

  case 14:
#line 193 "configparser.y" /* yacc.c:1648  */
    {
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->server_count = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 1671 "configparser.c" /* yacc.c:1648  */
    break;

  case 15:
#line 201 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->ip_transparent = (int)(yyvsp[0].bln); }
#line 1677 "configparser.c" /* yacc.c:1648  */
    break;

  case 16:
#line 203 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->ip_freebind = (yyvsp[0].bln); }
#line 1683 "configparser.c" /* yacc.c:1648  */
    break;

  case 17:
#line 205 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->send_buffer_size = (int)(yyvsp[0].llng); }
#line 1689 "configparser.c" /* yacc.c:1648  */
    break;

  case 18:
#line 207 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->receive_buffer_size = (int)(yyvsp[0].llng); }
#line 1695 "configparser.c" /* yacc.c:1648  */
    break;

  case 19:
#line 209 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->debug_mode = (yyvsp[0].bln); }
#line 1701 "configparser.c" /* yacc.c:1648  */
    break;

  case 20:
#line 211 "configparser.y" /* yacc.c:1648  */
    { /* ignored, deprecated */ }
#line 1707 "configparser.c" /* yacc.c:1648  */
    break;

  case 21:
#line 213 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->hide_version = (yyvsp[0].bln); }
#line 1713 "configparser.c" /* yacc.c:1648  */
    break;

  case 22:
#line 215 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->hide_identity = (yyvsp[0].bln); }
#line 1719 "configparser.c" /* yacc.c:1648  */
    break;

  case 23:
#line 217 "configparser.y" /* yacc.c:1648  */
    { if((yyvsp[0].bln)) { cfg_parser->opt->do_ip4 = 1; cfg_parser->opt->do_ip6 = 0; } }
#line 1725 "configparser.c" /* yacc.c:1648  */
    break;

  case 24:
#line 219 "configparser.y" /* yacc.c:1648  */
    { if((yyvsp[0].bln)) { cfg_parser->opt->do_ip4 = 0; cfg_parser->opt->do_ip6 = 1; } }
#line 1731 "configparser.c" /* yacc.c:1648  */
    break;

  case 25:
#line 221 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->do_ip4 = (yyvsp[0].bln); }
#line 1737 "configparser.c" /* yacc.c:1648  */
    break;

  case 26:
#line 223 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->do_ip6 = (yyvsp[0].bln); }
#line 1743 "configparser.c" /* yacc.c:1648  */
    break;

  case 27:
#line 225 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->opt->database = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      if(cfg_parser->opt->database[0] == 0 &&
         cfg_parser->opt->zonefiles_write == 0)
      {
        cfg_parser->opt->zonefiles_write = ZONEFILES_WRITE_INTERVAL;
      }
    }
#line 1756 "configparser.c" /* yacc.c:1648  */
    break;

  case 28:
#line 234 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->identity = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1762 "configparser.c" /* yacc.c:1648  */
    break;

  case 29:
#line 236 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->version = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1768 "configparser.c" /* yacc.c:1648  */
    break;

  case 30:
#line 238 "configparser.y" /* yacc.c:1648  */
    {
      unsigned char* nsid = 0;
      size_t nsid_len = strlen((yyvsp[0].str));

      if (strncasecmp((yyvsp[0].str), "ascii_", 6) == 0) {
        nsid_len -= 6; /* discard "ascii_" */
        if(nsid_len < 65535) {
          cfg_parser->opt->nsid = region_alloc(cfg_parser->opt->region, nsid_len*2+1);
          hex_ntop((uint8_t*)(yyvsp[0].str)+6, nsid_len, (char*)cfg_parser->opt->nsid, nsid_len*2+1);
        } else {
          yyerror("NSID too long");
        }
      } else if (nsid_len % 2 != 0) {
        yyerror("the NSID must be a hex string of an even length.");
      } else {
        nsid_len = nsid_len / 2;
        if(nsid_len < 65535) {
          nsid = xalloc(nsid_len);
          if (hex_pton((yyvsp[0].str), nsid, nsid_len) == -1) {
            yyerror("hex string cannot be parsed in NSID.");
          } else {
            cfg_parser->opt->nsid = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
          }
          free(nsid);
        } else {
          yyerror("NSID too long");
        }
      }
    }
#line 1802 "configparser.c" /* yacc.c:1648  */
    break;

  case 31:
#line 268 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->logfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1808 "configparser.c" /* yacc.c:1648  */
    break;

  case 32:
#line 270 "configparser.y" /* yacc.c:1648  */
    {
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->tcp_count = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 1820 "configparser.c" /* yacc.c:1648  */
    break;

  case 33:
#line 278 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->tcp_reject_overflow = (yyvsp[0].bln); }
#line 1826 "configparser.c" /* yacc.c:1648  */
    break;

  case 34:
#line 280 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->tcp_query_count = (int)(yyvsp[0].llng); }
#line 1832 "configparser.c" /* yacc.c:1648  */
    break;

  case 35:
#line 282 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->tcp_timeout = (int)(yyvsp[0].llng); }
#line 1838 "configparser.c" /* yacc.c:1648  */
    break;

  case 36:
#line 284 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->tcp_mss = (int)(yyvsp[0].llng); }
#line 1844 "configparser.c" /* yacc.c:1648  */
    break;

  case 37:
#line 286 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->outgoing_tcp_mss = (int)(yyvsp[0].llng); }
#line 1850 "configparser.c" /* yacc.c:1648  */
    break;

  case 38:
#line 288 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->ipv4_edns_size = (size_t)(yyvsp[0].llng); }
#line 1856 "configparser.c" /* yacc.c:1648  */
    break;

  case 39:
#line 290 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->ipv6_edns_size = (size_t)(yyvsp[0].llng); }
#line 1862 "configparser.c" /* yacc.c:1648  */
    break;

  case 40:
#line 292 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->pidfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1868 "configparser.c" /* yacc.c:1648  */
    break;

  case 41:
#line 294 "configparser.y" /* yacc.c:1648  */
    {
      /* port number, stored as a string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 1879 "configparser.c" /* yacc.c:1648  */
    break;

  case 42:
#line 301 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->reuseport = (yyvsp[0].bln); }
#line 1885 "configparser.c" /* yacc.c:1648  */
    break;

  case 43:
#line 303 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->statistics = (int)(yyvsp[0].llng); }
#line 1891 "configparser.c" /* yacc.c:1648  */
    break;

  case 44:
#line 305 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->chroot = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1897 "configparser.c" /* yacc.c:1648  */
    break;

  case 45:
#line 307 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->username = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1903 "configparser.c" /* yacc.c:1648  */
    break;

  case 46:
#line 309 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->zonesdir = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1909 "configparser.c" /* yacc.c:1648  */
    break;

  case 47:
#line 311 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->zonelistfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1915 "configparser.c" /* yacc.c:1648  */
    break;

  case 48:
#line 313 "configparser.y" /* yacc.c:1648  */
    { /* ignored, deprecated */ }
#line 1921 "configparser.c" /* yacc.c:1648  */
    break;

  case 49:
#line 315 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->xfrdfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1927 "configparser.c" /* yacc.c:1648  */
    break;

  case 50:
#line 317 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->xfrdir = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1933 "configparser.c" /* yacc.c:1648  */
    break;

  case 51:
#line 319 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->xfrd_reload_timeout = (int)(yyvsp[0].llng); }
#line 1939 "configparser.c" /* yacc.c:1648  */
    break;

  case 52:
#line 321 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->verbosity = (int)(yyvsp[0].llng); }
#line 1945 "configparser.c" /* yacc.c:1648  */
    break;

  case 53:
#line 323 "configparser.y" /* yacc.c:1648  */
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->rrl_size = (size_t)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
#endif
    }
#line 1959 "configparser.c" /* yacc.c:1648  */
    break;

  case 54:
#line 333 "configparser.y" /* yacc.c:1648  */
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_ratelimit = (size_t)(yyvsp[0].llng);
#endif
    }
#line 1969 "configparser.c" /* yacc.c:1648  */
    break;

  case 55:
#line 339 "configparser.y" /* yacc.c:1648  */
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_slip = (size_t)(yyvsp[0].llng);
#endif
    }
#line 1979 "configparser.c" /* yacc.c:1648  */
    break;

  case 56:
#line 345 "configparser.y" /* yacc.c:1648  */
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 32) {
        yyerror("invalid IPv4 prefix length");
      } else {
        cfg_parser->opt->rrl_ipv4_prefix_length = (size_t)(yyvsp[0].llng);
      }
#endif
    }
#line 1993 "configparser.c" /* yacc.c:1648  */
    break;

  case 57:
#line 355 "configparser.y" /* yacc.c:1648  */
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 64) {
        yyerror("invalid IPv6 prefix length");
      } else {
        cfg_parser->opt->rrl_ipv6_prefix_length = (size_t)(yyvsp[0].llng);
      }
#endif
    }
#line 2007 "configparser.c" /* yacc.c:1648  */
    break;

  case 58:
#line 365 "configparser.y" /* yacc.c:1648  */
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_whitelist_ratelimit = (size_t)(yyvsp[0].llng);
#endif
    }
#line 2017 "configparser.c" /* yacc.c:1648  */
    break;

  case 59:
#line 371 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->zonefiles_check = (yyvsp[0].bln); }
#line 2023 "configparser.c" /* yacc.c:1648  */
    break;

  case 60:
#line 373 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->zonefiles_write = (int)(yyvsp[0].llng); }
#line 2029 "configparser.c" /* yacc.c:1648  */
    break;

  case 61:
#line 375 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->opt->log_time_ascii = (yyvsp[0].bln);
      log_time_asc = cfg_parser->opt->log_time_ascii;
    }
#line 2038 "configparser.c" /* yacc.c:1648  */
    break;

  case 62:
#line 380 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->opt->round_robin = (yyvsp[0].bln);
      round_robin = cfg_parser->opt->round_robin;
    }
#line 2047 "configparser.c" /* yacc.c:1648  */
    break;

  case 63:
#line 385 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->opt->minimal_responses = (yyvsp[0].bln);
      minimal_responses = cfg_parser->opt->minimal_responses;
    }
#line 2056 "configparser.c" /* yacc.c:1648  */
    break;

  case 64:
#line 390 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->confine_to_zone = (yyvsp[0].bln); }
#line 2062 "configparser.c" /* yacc.c:1648  */
    break;

  case 65:
#line 392 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->refuse_any = (yyvsp[0].bln); }
#line 2068 "configparser.c" /* yacc.c:1648  */
    break;

  case 66:
#line 394 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->tls_service_key = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2074 "configparser.c" /* yacc.c:1648  */
    break;

  case 67:
#line 396 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->tls_service_ocsp = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2080 "configparser.c" /* yacc.c:1648  */
    break;

  case 68:
#line 398 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->tls_service_pem = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2086 "configparser.c" /* yacc.c:1648  */
    break;

  case 69:
#line 400 "configparser.y" /* yacc.c:1648  */
    {
      /* port number, stored as string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->tls_port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 2097 "configparser.c" /* yacc.c:1648  */
    break;

  case 73:
#line 416 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->dnstap_enable = (yyvsp[0].bln); }
#line 2103 "configparser.c" /* yacc.c:1648  */
    break;

  case 74:
#line 418 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->dnstap_socket_path = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2109 "configparser.c" /* yacc.c:1648  */
    break;

  case 75:
#line 420 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->dnstap_send_identity = (yyvsp[0].bln); }
#line 2115 "configparser.c" /* yacc.c:1648  */
    break;

  case 76:
#line 422 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->dnstap_send_version = (yyvsp[0].bln); }
#line 2121 "configparser.c" /* yacc.c:1648  */
    break;

  case 77:
#line 424 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->dnstap_identity = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2127 "configparser.c" /* yacc.c:1648  */
    break;

  case 78:
#line 426 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->dnstap_version = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2133 "configparser.c" /* yacc.c:1648  */
    break;

  case 79:
#line 428 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->dnstap_log_auth_query_messages = (yyvsp[0].bln); }
#line 2139 "configparser.c" /* yacc.c:1648  */
    break;

  case 80:
#line 430 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->dnstap_log_auth_response_messages = (yyvsp[0].bln); }
#line 2145 "configparser.c" /* yacc.c:1648  */
    break;

  case 84:
#line 441 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->control_enable = (yyvsp[0].bln); }
#line 2151 "configparser.c" /* yacc.c:1648  */
    break;

  case 85:
#line 443 "configparser.y" /* yacc.c:1648  */
    {
      struct ip_address_option *ip = cfg_parser->opt->control_interface;
      if(ip == NULL) {
        cfg_parser->opt->control_interface = (yyvsp[0].ip);
      } else {
        while(ip->next != NULL) { ip = ip->next; }
        ip->next = (yyvsp[0].ip);
      }
    }
#line 2165 "configparser.c" /* yacc.c:1648  */
    break;

  case 86:
#line 453 "configparser.y" /* yacc.c:1648  */
    {
      if((yyvsp[0].llng) == 0) {
        yyerror("control port number expected");
      } else {
        cfg_parser->opt->control_port = (int)(yyvsp[0].llng);
      }
    }
#line 2177 "configparser.c" /* yacc.c:1648  */
    break;

  case 87:
#line 461 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->server_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2183 "configparser.c" /* yacc.c:1648  */
    break;

  case 88:
#line 463 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->server_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2189 "configparser.c" /* yacc.c:1648  */
    break;

  case 89:
#line 465 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->control_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2195 "configparser.c" /* yacc.c:1648  */
    break;

  case 90:
#line 467 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->opt->control_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2201 "configparser.c" /* yacc.c:1648  */
    break;

  case 91:
#line 472 "configparser.y" /* yacc.c:1648  */
    {
        key_options_type *key = key_options_create(cfg_parser->opt->region);
        key->algorithm = region_strdup(cfg_parser->opt->region, "sha256");
        assert(cfg_parser->key == NULL);
        cfg_parser->key = key;
      }
#line 2212 "configparser.c" /* yacc.c:1648  */
    break;

  case 92:
#line 479 "configparser.y" /* yacc.c:1648  */
    {
      struct key_options *key = cfg_parser->key;
      if(key->name == NULL) {
        yyerror("tsig key has no name");
      } else if(key->algorithm == NULL) {
        yyerror("tsig key %s has no algorithm", key->name);
      } else if(key->secret == NULL) {
        yyerror("tsig key %s has no secret blob", key->name);
      } else if(key_options_find(cfg_parser->opt, key->name)) {
        yyerror("duplicate tsig key %s", key->name);
      } else {
        key_options_insert(cfg_parser->opt, key);
        cfg_parser->key = NULL;
      }
    }
#line 2232 "configparser.c" /* yacc.c:1648  */
    break;

  case 95:
#line 500 "configparser.y" /* yacc.c:1648  */
    {
      dname_type *dname;

      dname = (dname_type *)dname_parse(cfg_parser->opt->region, (yyvsp[0].str));
      cfg_parser->key->name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      if(dname == NULL) {
        yyerror("bad tsig key name %s", (yyvsp[0].str));
      } else {
        region_recycle(cfg_parser->opt->region, dname, dname_total_size(dname));
      }
    }
#line 2248 "configparser.c" /* yacc.c:1648  */
    break;

  case 96:
#line 512 "configparser.y" /* yacc.c:1648  */
    {
      if(tsig_get_algorithm_by_name((yyvsp[0].str)) == NULL) {
        yyerror("bad tsig key algorithm %s", (yyvsp[0].str));
      } else {
        cfg_parser->key->algorithm = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      }
    }
#line 2260 "configparser.c" /* yacc.c:1648  */
    break;

  case 97:
#line 520 "configparser.y" /* yacc.c:1648  */
    {
      uint8_t data[16384];
      int size;

      cfg_parser->key->secret = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      size = b64_pton((yyvsp[0].str), data, sizeof(data));
      if(size == -1) {
        yyerror("cannot base64 decode tsig secret %s",
          cfg_parser->key->name?
          cfg_parser->key->name:"");
      } else if(size != 0) {
        memset(data, 0xdd, size); /* wipe secret */
      }
    }
#line 2279 "configparser.c" /* yacc.c:1648  */
    break;

  case 98:
#line 538 "configparser.y" /* yacc.c:1648  */
    {
        assert(cfg_parser->pattern == NULL);
        assert(cfg_parser->zone == NULL);
        cfg_parser->zone = zone_options_create(cfg_parser->opt->region);
        cfg_parser->zone->part_of_config = 1;
        cfg_parser->zone->pattern = cfg_parser->pattern =
          pattern_options_create(cfg_parser->opt->region);
        cfg_parser->zone->pattern->implicit = 1;
      }
#line 2293 "configparser.c" /* yacc.c:1648  */
    break;

  case 99:
#line 548 "configparser.y" /* yacc.c:1648  */
    {
      assert(cfg_parser->zone != NULL);
      if(cfg_parser->zone->name == NULL) {
        yyerror("zone has no name");
      } else if(!nsd_options_insert_zone(cfg_parser->opt, cfg_parser->zone)) {
        yyerror("duplicate zone %s", cfg_parser->zone->name);
      } else if(!nsd_options_insert_pattern(cfg_parser->opt, cfg_parser->zone->pattern)) {
        yyerror("duplicate pattern %s", cfg_parser->zone->pattern->pname);
      }
      cfg_parser->pattern = NULL;
      cfg_parser->zone = NULL;
    }
#line 2310 "configparser.c" /* yacc.c:1648  */
    break;

  case 102:
#line 566 "configparser.y" /* yacc.c:1648  */
    {
      const char *marker = PATTERN_IMPLICIT_MARKER;
      char *pname = region_alloc(cfg_parser->opt->region, strlen((yyvsp[0].str)) + strlen(marker) + 1);
      memmove(pname, marker, strlen(marker));
      memmove(pname + strlen(marker), (yyvsp[0].str), strlen((yyvsp[0].str)) + 1);
      cfg_parser->zone->pattern->pname = pname;
      cfg_parser->zone->name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      if(pattern_options_find(cfg_parser->opt, pname)) {
        yyerror("zone %s cannot be created because implicit pattern %s "
                    "already exists", (yyvsp[0].str), pname);
      }
    }
#line 2327 "configparser.c" /* yacc.c:1648  */
    break;

  case 104:
#line 582 "configparser.y" /* yacc.c:1648  */
    {
        assert(cfg_parser->pattern == NULL);
        cfg_parser->pattern = pattern_options_create(cfg_parser->opt->region);
      }
#line 2336 "configparser.c" /* yacc.c:1648  */
    break;

  case 105:
#line 587 "configparser.y" /* yacc.c:1648  */
    {
      pattern_options_type *pattern = cfg_parser->pattern;
      if(pattern->pname == NULL) {
        yyerror("pattern has no name");
      } else if(!nsd_options_insert_pattern(cfg_parser->opt, pattern)) {
        yyerror("duplicate pattern %s", pattern->pname);
      }
      cfg_parser->pattern = NULL;
    }
#line 2350 "configparser.c" /* yacc.c:1648  */
    break;

  case 108:
#line 602 "configparser.y" /* yacc.c:1648  */
    {
      if(strchr((yyvsp[0].str), ' ')) {
        yyerror("space is not allowed in pattern name: '%s'", (yyvsp[0].str));
      }
      cfg_parser->pattern->pname = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2361 "configparser.c" /* yacc.c:1648  */
    break;

  case 110:
#line 612 "configparser.y" /* yacc.c:1648  */
    {
#ifdef RATELIMIT
      cfg_parser->pattern->rrl_whitelist |= rrlstr2type((yyvsp[0].str));
#endif
    }
#line 2371 "configparser.c" /* yacc.c:1648  */
    break;

  case 111:
#line 618 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->pattern->zonefile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2377 "configparser.c" /* yacc.c:1648  */
    break;

  case 112:
#line 620 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->pattern->zonestats = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2383 "configparser.c" /* yacc.c:1648  */
    break;

  case 113:
#line 622 "configparser.y" /* yacc.c:1648  */
    {
      if((yyvsp[0].llng) > 0) {
        cfg_parser->pattern->size_limit_xfr = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 2395 "configparser.c" /* yacc.c:1648  */
    break;

  case 114:
#line 630 "configparser.y" /* yacc.c:1648  */
    { cfg_parser->pattern->multi_master_check = (int)(yyvsp[0].bln); }
#line 2401 "configparser.c" /* yacc.c:1648  */
    break;

  case 115:
#line 632 "configparser.y" /* yacc.c:1648  */
    { config_apply_pattern(cfg_parser->pattern, (yyvsp[0].str)); }
#line 2407 "configparser.c" /* yacc.c:1648  */
    break;

  case 116:
#line 634 "configparser.y" /* yacc.c:1648  */
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2420 "configparser.c" /* yacc.c:1648  */
    break;

  case 117:
#line 643 "configparser.y" /* yacc.c:1648  */
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      acl->use_axfr_only = 1;
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2434 "configparser.c" /* yacc.c:1648  */
    break;

  case 118:
#line 653 "configparser.y" /* yacc.c:1648  */
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      acl->allow_udp = 1;
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2448 "configparser.c" /* yacc.c:1648  */
    break;

  case 119:
#line 663 "configparser.y" /* yacc.c:1648  */
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      append_acl(&cfg_parser->pattern->allow_notify, acl);
    }
#line 2457 "configparser.c" /* yacc.c:1648  */
    break;

  case 120:
#line 668 "configparser.y" /* yacc.c:1648  */
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      if(acl->blocked)
        yyerror("blocked address used for notify");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for notify");
      append_acl(&cfg_parser->pattern->notify, acl);
    }
#line 2470 "configparser.c" /* yacc.c:1648  */
    break;

  case 121:
#line 677 "configparser.y" /* yacc.c:1648  */
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      append_acl(&cfg_parser->pattern->provide_xfr, acl);
    }
#line 2479 "configparser.c" /* yacc.c:1648  */
    break;

  case 122:
#line 682 "configparser.y" /* yacc.c:1648  */
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[0].str), "NOKEY");
      append_acl(&cfg_parser->pattern->outgoing_interface, acl);
    }
#line 2488 "configparser.c" /* yacc.c:1648  */
    break;

  case 123:
#line 687 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->pattern->allow_axfr_fallback = (yyvsp[0].bln);
      cfg_parser->pattern->allow_axfr_fallback_is_default = 0;
    }
#line 2497 "configparser.c" /* yacc.c:1648  */
    break;

  case 124:
#line 692 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->pattern->notify_retry = (yyvsp[0].llng);
      cfg_parser->pattern->notify_retry_is_default = 0;
    }
#line 2506 "configparser.c" /* yacc.c:1648  */
    break;

  case 125:
#line 697 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->pattern->max_refresh_time = (yyvsp[0].llng);
      cfg_parser->pattern->max_refresh_time_is_default = 0;
    }
#line 2515 "configparser.c" /* yacc.c:1648  */
    break;

  case 126:
#line 702 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->pattern->min_refresh_time = (yyvsp[0].llng);
      cfg_parser->pattern->min_refresh_time_is_default = 0;
    }
#line 2524 "configparser.c" /* yacc.c:1648  */
    break;

  case 127:
#line 707 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->pattern->max_retry_time = (yyvsp[0].llng);
      cfg_parser->pattern->max_retry_time_is_default = 0;
    }
#line 2533 "configparser.c" /* yacc.c:1648  */
    break;

  case 128:
#line 712 "configparser.y" /* yacc.c:1648  */
    {
      cfg_parser->pattern->min_retry_time = (yyvsp[0].llng);
      cfg_parser->pattern->min_retry_time_is_default = 0;
    }
#line 2542 "configparser.c" /* yacc.c:1648  */
    break;

  case 129:
#line 719 "configparser.y" /* yacc.c:1648  */
    {
      struct ip_address_option *ip = region_alloc_zero(
        cfg_parser->opt->region, sizeof(*ip));
      ip->address = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      (yyval.ip) = ip;
    }
#line 2553 "configparser.c" /* yacc.c:1648  */
    break;

  case 130:
#line 728 "configparser.y" /* yacc.c:1648  */
    {
      /* ensure string consists entirely of digits */
      const char *str = (yyvsp[0].str);
      size_t pos = 0;
      while(str[pos] >= '0' && str[pos] <= '9') {
        pos++;
      }

      (yyval.llng) = 0;
      if(pos > 0 && str[pos] == '\0') {
        int err = errno;
        errno = 0;
        (yyval.llng) = strtoll(str, NULL, 10);
        errno = err;
      } else {
        yyerror("expected a number");
        YYABORT; /* trigger a parser error */
      }
    }
#line 2577 "configparser.c" /* yacc.c:1648  */
    break;

  case 131:
#line 750 "configparser.y" /* yacc.c:1648  */
    {
      (yyval.bln) = 0;
      if(strcmp((yyvsp[0].str), "yes") == 0) {
        (yyval.bln) = 1;
      } else if(strcmp((yyvsp[0].str), "no") == 0) {
        (yyval.bln) = 0;
      } else {
        yyerror("expected yes or no");
        YYABORT; /* trigger a parser error */
      }
    }
#line 2593 "configparser.c" /* yacc.c:1648  */
    break;


#line 2597 "configparser.c" /* yacc.c:1648  */
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
#line 762 "configparser.y" /* yacc.c:1907  */


static void
append_acl(struct acl_options **list, struct acl_options *acl)
{
	assert(list != NULL);

	if(*list == NULL) {
		*list = acl;
	} else {
		struct acl_options *tail = *list;
		while(tail->next != NULL)
			tail = tail->next;
		tail->next = acl;
	}
}

