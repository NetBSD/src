/* A Bison parser, made by GNU Bison 3.7.6.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30706

/* Bison version string.  */
#define YYBISON_VERSION "3.7.6"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         c_parse
#define yylex           c_lex
#define yyerror         c_error
#define yydebug         c_debug
#define yynerrs         c_nerrs
#define yylval          c_lval
#define yychar          c_char

/* First part of user prologue.  */
#line 10 "configparser.y"

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

int yylex(void);

#ifdef __cplusplus
extern "C"
#endif

/* these need to be global, otherwise they cannot be used inside yacc */
extern config_parser_state_type *cfg_parser;

static void append_acl(struct acl_options **list, struct acl_options *acl);
static void add_to_last_acl(struct acl_options **list, char *ac);
static int parse_boolean(const char *str, int *bln);
static int parse_expire_expr(const char *str, long long *num, uint8_t *expr);
static int parse_number(const char *str, long long *num);
static int parse_range(const char *str, long long *low, long long *high);

struct component {
	struct component *next;
	char *str;
};


#line 115 "configparser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "configparser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_STRING = 3,                     /* STRING  */
  YYSYMBOL_VAR_SERVER = 4,                 /* VAR_SERVER  */
  YYSYMBOL_VAR_SERVER_COUNT = 5,           /* VAR_SERVER_COUNT  */
  YYSYMBOL_VAR_IP_ADDRESS = 6,             /* VAR_IP_ADDRESS  */
  YYSYMBOL_VAR_IP_TRANSPARENT = 7,         /* VAR_IP_TRANSPARENT  */
  YYSYMBOL_VAR_IP_FREEBIND = 8,            /* VAR_IP_FREEBIND  */
  YYSYMBOL_VAR_REUSEPORT = 9,              /* VAR_REUSEPORT  */
  YYSYMBOL_VAR_SEND_BUFFER_SIZE = 10,      /* VAR_SEND_BUFFER_SIZE  */
  YYSYMBOL_VAR_RECEIVE_BUFFER_SIZE = 11,   /* VAR_RECEIVE_BUFFER_SIZE  */
  YYSYMBOL_VAR_DEBUG_MODE = 12,            /* VAR_DEBUG_MODE  */
  YYSYMBOL_VAR_IP4_ONLY = 13,              /* VAR_IP4_ONLY  */
  YYSYMBOL_VAR_IP6_ONLY = 14,              /* VAR_IP6_ONLY  */
  YYSYMBOL_VAR_DO_IP4 = 15,                /* VAR_DO_IP4  */
  YYSYMBOL_VAR_DO_IP6 = 16,                /* VAR_DO_IP6  */
  YYSYMBOL_VAR_PORT = 17,                  /* VAR_PORT  */
  YYSYMBOL_VAR_USE_SYSTEMD = 18,           /* VAR_USE_SYSTEMD  */
  YYSYMBOL_VAR_VERBOSITY = 19,             /* VAR_VERBOSITY  */
  YYSYMBOL_VAR_USERNAME = 20,              /* VAR_USERNAME  */
  YYSYMBOL_VAR_CHROOT = 21,                /* VAR_CHROOT  */
  YYSYMBOL_VAR_ZONESDIR = 22,              /* VAR_ZONESDIR  */
  YYSYMBOL_VAR_ZONELISTFILE = 23,          /* VAR_ZONELISTFILE  */
  YYSYMBOL_VAR_DATABASE = 24,              /* VAR_DATABASE  */
  YYSYMBOL_VAR_LOGFILE = 25,               /* VAR_LOGFILE  */
  YYSYMBOL_VAR_LOG_ONLY_SYSLOG = 26,       /* VAR_LOG_ONLY_SYSLOG  */
  YYSYMBOL_VAR_PIDFILE = 27,               /* VAR_PIDFILE  */
  YYSYMBOL_VAR_DIFFFILE = 28,              /* VAR_DIFFFILE  */
  YYSYMBOL_VAR_XFRDFILE = 29,              /* VAR_XFRDFILE  */
  YYSYMBOL_VAR_XFRDIR = 30,                /* VAR_XFRDIR  */
  YYSYMBOL_VAR_HIDE_VERSION = 31,          /* VAR_HIDE_VERSION  */
  YYSYMBOL_VAR_HIDE_IDENTITY = 32,         /* VAR_HIDE_IDENTITY  */
  YYSYMBOL_VAR_VERSION = 33,               /* VAR_VERSION  */
  YYSYMBOL_VAR_IDENTITY = 34,              /* VAR_IDENTITY  */
  YYSYMBOL_VAR_NSID = 35,                  /* VAR_NSID  */
  YYSYMBOL_VAR_TCP_COUNT = 36,             /* VAR_TCP_COUNT  */
  YYSYMBOL_VAR_TCP_REJECT_OVERFLOW = 37,   /* VAR_TCP_REJECT_OVERFLOW  */
  YYSYMBOL_VAR_TCP_QUERY_COUNT = 38,       /* VAR_TCP_QUERY_COUNT  */
  YYSYMBOL_VAR_TCP_TIMEOUT = 39,           /* VAR_TCP_TIMEOUT  */
  YYSYMBOL_VAR_TCP_MSS = 40,               /* VAR_TCP_MSS  */
  YYSYMBOL_VAR_OUTGOING_TCP_MSS = 41,      /* VAR_OUTGOING_TCP_MSS  */
  YYSYMBOL_VAR_IPV4_EDNS_SIZE = 42,        /* VAR_IPV4_EDNS_SIZE  */
  YYSYMBOL_VAR_IPV6_EDNS_SIZE = 43,        /* VAR_IPV6_EDNS_SIZE  */
  YYSYMBOL_VAR_STATISTICS = 44,            /* VAR_STATISTICS  */
  YYSYMBOL_VAR_XFRD_RELOAD_TIMEOUT = 45,   /* VAR_XFRD_RELOAD_TIMEOUT  */
  YYSYMBOL_VAR_LOG_TIME_ASCII = 46,        /* VAR_LOG_TIME_ASCII  */
  YYSYMBOL_VAR_ROUND_ROBIN = 47,           /* VAR_ROUND_ROBIN  */
  YYSYMBOL_VAR_MINIMAL_RESPONSES = 48,     /* VAR_MINIMAL_RESPONSES  */
  YYSYMBOL_VAR_CONFINE_TO_ZONE = 49,       /* VAR_CONFINE_TO_ZONE  */
  YYSYMBOL_VAR_REFUSE_ANY = 50,            /* VAR_REFUSE_ANY  */
  YYSYMBOL_VAR_ZONEFILES_CHECK = 51,       /* VAR_ZONEFILES_CHECK  */
  YYSYMBOL_VAR_ZONEFILES_WRITE = 52,       /* VAR_ZONEFILES_WRITE  */
  YYSYMBOL_VAR_RRL_SIZE = 53,              /* VAR_RRL_SIZE  */
  YYSYMBOL_VAR_RRL_RATELIMIT = 54,         /* VAR_RRL_RATELIMIT  */
  YYSYMBOL_VAR_RRL_SLIP = 55,              /* VAR_RRL_SLIP  */
  YYSYMBOL_VAR_RRL_IPV4_PREFIX_LENGTH = 56, /* VAR_RRL_IPV4_PREFIX_LENGTH  */
  YYSYMBOL_VAR_RRL_IPV6_PREFIX_LENGTH = 57, /* VAR_RRL_IPV6_PREFIX_LENGTH  */
  YYSYMBOL_VAR_RRL_WHITELIST_RATELIMIT = 58, /* VAR_RRL_WHITELIST_RATELIMIT  */
  YYSYMBOL_VAR_TLS_SERVICE_KEY = 59,       /* VAR_TLS_SERVICE_KEY  */
  YYSYMBOL_VAR_TLS_SERVICE_PEM = 60,       /* VAR_TLS_SERVICE_PEM  */
  YYSYMBOL_VAR_TLS_SERVICE_OCSP = 61,      /* VAR_TLS_SERVICE_OCSP  */
  YYSYMBOL_VAR_TLS_PORT = 62,              /* VAR_TLS_PORT  */
  YYSYMBOL_VAR_TLS_CERT_BUNDLE = 63,       /* VAR_TLS_CERT_BUNDLE  */
  YYSYMBOL_VAR_CPU_AFFINITY = 64,          /* VAR_CPU_AFFINITY  */
  YYSYMBOL_VAR_XFRD_CPU_AFFINITY = 65,     /* VAR_XFRD_CPU_AFFINITY  */
  YYSYMBOL_VAR_SERVER_CPU_AFFINITY = 66,   /* VAR_SERVER_CPU_AFFINITY  */
  YYSYMBOL_VAR_DROP_UPDATES = 67,          /* VAR_DROP_UPDATES  */
  YYSYMBOL_VAR_XFRD_TCP_MAX = 68,          /* VAR_XFRD_TCP_MAX  */
  YYSYMBOL_VAR_XFRD_TCP_PIPELINE = 69,     /* VAR_XFRD_TCP_PIPELINE  */
  YYSYMBOL_VAR_DNSTAP = 70,                /* VAR_DNSTAP  */
  YYSYMBOL_VAR_DNSTAP_ENABLE = 71,         /* VAR_DNSTAP_ENABLE  */
  YYSYMBOL_VAR_DNSTAP_SOCKET_PATH = 72,    /* VAR_DNSTAP_SOCKET_PATH  */
  YYSYMBOL_VAR_DNSTAP_SEND_IDENTITY = 73,  /* VAR_DNSTAP_SEND_IDENTITY  */
  YYSYMBOL_VAR_DNSTAP_SEND_VERSION = 74,   /* VAR_DNSTAP_SEND_VERSION  */
  YYSYMBOL_VAR_DNSTAP_IDENTITY = 75,       /* VAR_DNSTAP_IDENTITY  */
  YYSYMBOL_VAR_DNSTAP_VERSION = 76,        /* VAR_DNSTAP_VERSION  */
  YYSYMBOL_VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES = 77, /* VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES  */
  YYSYMBOL_VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES = 78, /* VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES  */
  YYSYMBOL_VAR_REMOTE_CONTROL = 79,        /* VAR_REMOTE_CONTROL  */
  YYSYMBOL_VAR_CONTROL_ENABLE = 80,        /* VAR_CONTROL_ENABLE  */
  YYSYMBOL_VAR_CONTROL_INTERFACE = 81,     /* VAR_CONTROL_INTERFACE  */
  YYSYMBOL_VAR_CONTROL_PORT = 82,          /* VAR_CONTROL_PORT  */
  YYSYMBOL_VAR_SERVER_KEY_FILE = 83,       /* VAR_SERVER_KEY_FILE  */
  YYSYMBOL_VAR_SERVER_CERT_FILE = 84,      /* VAR_SERVER_CERT_FILE  */
  YYSYMBOL_VAR_CONTROL_KEY_FILE = 85,      /* VAR_CONTROL_KEY_FILE  */
  YYSYMBOL_VAR_CONTROL_CERT_FILE = 86,     /* VAR_CONTROL_CERT_FILE  */
  YYSYMBOL_VAR_KEY = 87,                   /* VAR_KEY  */
  YYSYMBOL_VAR_ALGORITHM = 88,             /* VAR_ALGORITHM  */
  YYSYMBOL_VAR_SECRET = 89,                /* VAR_SECRET  */
  YYSYMBOL_VAR_TLS_AUTH = 90,              /* VAR_TLS_AUTH  */
  YYSYMBOL_VAR_TLS_AUTH_DOMAIN_NAME = 91,  /* VAR_TLS_AUTH_DOMAIN_NAME  */
  YYSYMBOL_VAR_TLS_AUTH_CLIENT_CERT = 92,  /* VAR_TLS_AUTH_CLIENT_CERT  */
  YYSYMBOL_VAR_TLS_AUTH_CLIENT_KEY = 93,   /* VAR_TLS_AUTH_CLIENT_KEY  */
  YYSYMBOL_VAR_TLS_AUTH_CLIENT_KEY_PW = 94, /* VAR_TLS_AUTH_CLIENT_KEY_PW  */
  YYSYMBOL_VAR_PATTERN = 95,               /* VAR_PATTERN  */
  YYSYMBOL_VAR_NAME = 96,                  /* VAR_NAME  */
  YYSYMBOL_VAR_ZONEFILE = 97,              /* VAR_ZONEFILE  */
  YYSYMBOL_VAR_NOTIFY = 98,                /* VAR_NOTIFY  */
  YYSYMBOL_VAR_PROVIDE_XFR = 99,           /* VAR_PROVIDE_XFR  */
  YYSYMBOL_VAR_ALLOW_QUERY = 100,          /* VAR_ALLOW_QUERY  */
  YYSYMBOL_VAR_AXFR = 101,                 /* VAR_AXFR  */
  YYSYMBOL_VAR_UDP = 102,                  /* VAR_UDP  */
  YYSYMBOL_VAR_NOTIFY_RETRY = 103,         /* VAR_NOTIFY_RETRY  */
  YYSYMBOL_VAR_ALLOW_NOTIFY = 104,         /* VAR_ALLOW_NOTIFY  */
  YYSYMBOL_VAR_REQUEST_XFR = 105,          /* VAR_REQUEST_XFR  */
  YYSYMBOL_VAR_ALLOW_AXFR_FALLBACK = 106,  /* VAR_ALLOW_AXFR_FALLBACK  */
  YYSYMBOL_VAR_OUTGOING_INTERFACE = 107,   /* VAR_OUTGOING_INTERFACE  */
  YYSYMBOL_VAR_ANSWER_COOKIE = 108,        /* VAR_ANSWER_COOKIE  */
  YYSYMBOL_VAR_COOKIE_SECRET = 109,        /* VAR_COOKIE_SECRET  */
  YYSYMBOL_VAR_COOKIE_SECRET_FILE = 110,   /* VAR_COOKIE_SECRET_FILE  */
  YYSYMBOL_VAR_MAX_REFRESH_TIME = 111,     /* VAR_MAX_REFRESH_TIME  */
  YYSYMBOL_VAR_MIN_REFRESH_TIME = 112,     /* VAR_MIN_REFRESH_TIME  */
  YYSYMBOL_VAR_MAX_RETRY_TIME = 113,       /* VAR_MAX_RETRY_TIME  */
  YYSYMBOL_VAR_MIN_RETRY_TIME = 114,       /* VAR_MIN_RETRY_TIME  */
  YYSYMBOL_VAR_MIN_EXPIRE_TIME = 115,      /* VAR_MIN_EXPIRE_TIME  */
  YYSYMBOL_VAR_MULTI_MASTER_CHECK = 116,   /* VAR_MULTI_MASTER_CHECK  */
  YYSYMBOL_VAR_SIZE_LIMIT_XFR = 117,       /* VAR_SIZE_LIMIT_XFR  */
  YYSYMBOL_VAR_ZONESTATS = 118,            /* VAR_ZONESTATS  */
  YYSYMBOL_VAR_INCLUDE_PATTERN = 119,      /* VAR_INCLUDE_PATTERN  */
  YYSYMBOL_VAR_STORE_IXFR = 120,           /* VAR_STORE_IXFR  */
  YYSYMBOL_VAR_IXFR_SIZE = 121,            /* VAR_IXFR_SIZE  */
  YYSYMBOL_VAR_IXFR_NUMBER = 122,          /* VAR_IXFR_NUMBER  */
  YYSYMBOL_VAR_CREATE_IXFR = 123,          /* VAR_CREATE_IXFR  */
  YYSYMBOL_VAR_ZONE = 124,                 /* VAR_ZONE  */
  YYSYMBOL_VAR_RRL_WHITELIST = 125,        /* VAR_RRL_WHITELIST  */
  YYSYMBOL_VAR_SERVERS = 126,              /* VAR_SERVERS  */
  YYSYMBOL_VAR_BINDTODEVICE = 127,         /* VAR_BINDTODEVICE  */
  YYSYMBOL_VAR_SETFIB = 128,               /* VAR_SETFIB  */
  YYSYMBOL_VAR_VERIFY = 129,               /* VAR_VERIFY  */
  YYSYMBOL_VAR_ENABLE = 130,               /* VAR_ENABLE  */
  YYSYMBOL_VAR_VERIFY_ZONE = 131,          /* VAR_VERIFY_ZONE  */
  YYSYMBOL_VAR_VERIFY_ZONES = 132,         /* VAR_VERIFY_ZONES  */
  YYSYMBOL_VAR_VERIFIER = 133,             /* VAR_VERIFIER  */
  YYSYMBOL_VAR_VERIFIER_COUNT = 134,       /* VAR_VERIFIER_COUNT  */
  YYSYMBOL_VAR_VERIFIER_FEED_ZONE = 135,   /* VAR_VERIFIER_FEED_ZONE  */
  YYSYMBOL_VAR_VERIFIER_TIMEOUT = 136,     /* VAR_VERIFIER_TIMEOUT  */
  YYSYMBOL_YYACCEPT = 137,                 /* $accept  */
  YYSYMBOL_blocks = 138,                   /* blocks  */
  YYSYMBOL_block = 139,                    /* block  */
  YYSYMBOL_server = 140,                   /* server  */
  YYSYMBOL_server_block = 141,             /* server_block  */
  YYSYMBOL_server_option = 142,            /* server_option  */
  YYSYMBOL_143_1 = 143,                    /* $@1  */
  YYSYMBOL_socket_options = 144,           /* socket_options  */
  YYSYMBOL_socket_option = 145,            /* socket_option  */
  YYSYMBOL_cpus = 146,                     /* cpus  */
  YYSYMBOL_service_cpu_affinity = 147,     /* service_cpu_affinity  */
  YYSYMBOL_dnstap = 148,                   /* dnstap  */
  YYSYMBOL_dnstap_block = 149,             /* dnstap_block  */
  YYSYMBOL_dnstap_option = 150,            /* dnstap_option  */
  YYSYMBOL_remote_control = 151,           /* remote_control  */
  YYSYMBOL_remote_control_block = 152,     /* remote_control_block  */
  YYSYMBOL_remote_control_option = 153,    /* remote_control_option  */
  YYSYMBOL_tls_auth = 154,                 /* tls_auth  */
  YYSYMBOL_155_2 = 155,                    /* $@2  */
  YYSYMBOL_tls_auth_block = 156,           /* tls_auth_block  */
  YYSYMBOL_tls_auth_option = 157,          /* tls_auth_option  */
  YYSYMBOL_key = 158,                      /* key  */
  YYSYMBOL_159_3 = 159,                    /* $@3  */
  YYSYMBOL_key_block = 160,                /* key_block  */
  YYSYMBOL_key_option = 161,               /* key_option  */
  YYSYMBOL_zone = 162,                     /* zone  */
  YYSYMBOL_163_4 = 163,                    /* $@4  */
  YYSYMBOL_zone_block = 164,               /* zone_block  */
  YYSYMBOL_zone_option = 165,              /* zone_option  */
  YYSYMBOL_pattern = 166,                  /* pattern  */
  YYSYMBOL_167_5 = 167,                    /* $@5  */
  YYSYMBOL_pattern_block = 168,            /* pattern_block  */
  YYSYMBOL_pattern_option = 169,           /* pattern_option  */
  YYSYMBOL_pattern_or_zone_option = 170,   /* pattern_or_zone_option  */
  YYSYMBOL_171_6 = 171,                    /* $@6  */
  YYSYMBOL_172_7 = 172,                    /* $@7  */
  YYSYMBOL_verify = 173,                   /* verify  */
  YYSYMBOL_verify_block = 174,             /* verify_block  */
  YYSYMBOL_verify_option = 175,            /* verify_option  */
  YYSYMBOL_command = 176,                  /* command  */
  YYSYMBOL_arguments = 177,                /* arguments  */
  YYSYMBOL_ip_address = 178,               /* ip_address  */
  YYSYMBOL_number = 179,                   /* number  */
  YYSYMBOL_boolean = 180,                  /* boolean  */
  YYSYMBOL_tlsauth_option = 181            /* tlsauth_option  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
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

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
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
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
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
#define YYLAST   451

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  137
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  45
/* YYNRULES -- Number of rules.  */
#define YYNRULES  190
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  330

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   391


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
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
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   220,   220,   222,   225,   226,   227,   228,   229,   230,
     231,   232,   235,   238,   238,   242,   241,   258,   266,   268,
     270,   272,   274,   276,   278,   280,   282,   284,   286,   288,
     290,   292,   301,   303,   305,   335,   337,   339,   347,   349,
     351,   353,   355,   357,   359,   361,   363,   370,   372,   374,
     376,   378,   380,   382,   384,   386,   388,   390,   392,   402,
     408,   414,   424,   434,   440,   442,   444,   449,   454,   459,
     461,   463,   465,   467,   469,   476,   478,   480,   482,   484,
     486,   488,   492,   522,   523,   526,   553,   555,   560,   561,
     595,   597,   608,   611,   611,   614,   616,   618,   620,   622,
     624,   626,   628,   633,   636,   636,   639,   641,   651,   659,
     661,   663,   665,   671,   670,   692,   692,   695,   706,   710,
     714,   718,   726,   725,   750,   750,   753,   765,   773,   792,
     791,   816,   816,   819,   832,   836,   835,   852,   852,   855,
     862,   865,   871,   873,   875,   883,   885,   888,   887,   899,
     898,   910,   920,   925,   934,   939,   944,   949,   954,   959,
     964,   969,   974,   979,   991,   996,  1001,  1006,  1011,  1013,
    1015,  1017,  1021,  1024,  1024,  1027,  1029,  1039,  1046,  1048,
    1050,  1052,  1054,  1058,  1078,  1079,  1097,  1107,  1116,  1124,
    1125
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "STRING", "VAR_SERVER",
  "VAR_SERVER_COUNT", "VAR_IP_ADDRESS", "VAR_IP_TRANSPARENT",
  "VAR_IP_FREEBIND", "VAR_REUSEPORT", "VAR_SEND_BUFFER_SIZE",
  "VAR_RECEIVE_BUFFER_SIZE", "VAR_DEBUG_MODE", "VAR_IP4_ONLY",
  "VAR_IP6_ONLY", "VAR_DO_IP4", "VAR_DO_IP6", "VAR_PORT",
  "VAR_USE_SYSTEMD", "VAR_VERBOSITY", "VAR_USERNAME", "VAR_CHROOT",
  "VAR_ZONESDIR", "VAR_ZONELISTFILE", "VAR_DATABASE", "VAR_LOGFILE",
  "VAR_LOG_ONLY_SYSLOG", "VAR_PIDFILE", "VAR_DIFFFILE", "VAR_XFRDFILE",
  "VAR_XFRDIR", "VAR_HIDE_VERSION", "VAR_HIDE_IDENTITY", "VAR_VERSION",
  "VAR_IDENTITY", "VAR_NSID", "VAR_TCP_COUNT", "VAR_TCP_REJECT_OVERFLOW",
  "VAR_TCP_QUERY_COUNT", "VAR_TCP_TIMEOUT", "VAR_TCP_MSS",
  "VAR_OUTGOING_TCP_MSS", "VAR_IPV4_EDNS_SIZE", "VAR_IPV6_EDNS_SIZE",
  "VAR_STATISTICS", "VAR_XFRD_RELOAD_TIMEOUT", "VAR_LOG_TIME_ASCII",
  "VAR_ROUND_ROBIN", "VAR_MINIMAL_RESPONSES", "VAR_CONFINE_TO_ZONE",
  "VAR_REFUSE_ANY", "VAR_ZONEFILES_CHECK", "VAR_ZONEFILES_WRITE",
  "VAR_RRL_SIZE", "VAR_RRL_RATELIMIT", "VAR_RRL_SLIP",
  "VAR_RRL_IPV4_PREFIX_LENGTH", "VAR_RRL_IPV6_PREFIX_LENGTH",
  "VAR_RRL_WHITELIST_RATELIMIT", "VAR_TLS_SERVICE_KEY",
  "VAR_TLS_SERVICE_PEM", "VAR_TLS_SERVICE_OCSP", "VAR_TLS_PORT",
  "VAR_TLS_CERT_BUNDLE", "VAR_CPU_AFFINITY", "VAR_XFRD_CPU_AFFINITY",
  "VAR_SERVER_CPU_AFFINITY", "VAR_DROP_UPDATES", "VAR_XFRD_TCP_MAX",
  "VAR_XFRD_TCP_PIPELINE", "VAR_DNSTAP", "VAR_DNSTAP_ENABLE",
  "VAR_DNSTAP_SOCKET_PATH", "VAR_DNSTAP_SEND_IDENTITY",
  "VAR_DNSTAP_SEND_VERSION", "VAR_DNSTAP_IDENTITY", "VAR_DNSTAP_VERSION",
  "VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES",
  "VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES", "VAR_REMOTE_CONTROL",
  "VAR_CONTROL_ENABLE", "VAR_CONTROL_INTERFACE", "VAR_CONTROL_PORT",
  "VAR_SERVER_KEY_FILE", "VAR_SERVER_CERT_FILE", "VAR_CONTROL_KEY_FILE",
  "VAR_CONTROL_CERT_FILE", "VAR_KEY", "VAR_ALGORITHM", "VAR_SECRET",
  "VAR_TLS_AUTH", "VAR_TLS_AUTH_DOMAIN_NAME", "VAR_TLS_AUTH_CLIENT_CERT",
  "VAR_TLS_AUTH_CLIENT_KEY", "VAR_TLS_AUTH_CLIENT_KEY_PW", "VAR_PATTERN",
  "VAR_NAME", "VAR_ZONEFILE", "VAR_NOTIFY", "VAR_PROVIDE_XFR",
  "VAR_ALLOW_QUERY", "VAR_AXFR", "VAR_UDP", "VAR_NOTIFY_RETRY",
  "VAR_ALLOW_NOTIFY", "VAR_REQUEST_XFR", "VAR_ALLOW_AXFR_FALLBACK",
  "VAR_OUTGOING_INTERFACE", "VAR_ANSWER_COOKIE", "VAR_COOKIE_SECRET",
  "VAR_COOKIE_SECRET_FILE", "VAR_MAX_REFRESH_TIME", "VAR_MIN_REFRESH_TIME",
  "VAR_MAX_RETRY_TIME", "VAR_MIN_RETRY_TIME", "VAR_MIN_EXPIRE_TIME",
  "VAR_MULTI_MASTER_CHECK", "VAR_SIZE_LIMIT_XFR", "VAR_ZONESTATS",
  "VAR_INCLUDE_PATTERN", "VAR_STORE_IXFR", "VAR_IXFR_SIZE",
  "VAR_IXFR_NUMBER", "VAR_CREATE_IXFR", "VAR_ZONE", "VAR_RRL_WHITELIST",
  "VAR_SERVERS", "VAR_BINDTODEVICE", "VAR_SETFIB", "VAR_VERIFY",
  "VAR_ENABLE", "VAR_VERIFY_ZONE", "VAR_VERIFY_ZONES", "VAR_VERIFIER",
  "VAR_VERIFIER_COUNT", "VAR_VERIFIER_FEED_ZONE", "VAR_VERIFIER_TIMEOUT",
  "$accept", "blocks", "block", "server", "server_block", "server_option",
  "$@1", "socket_options", "socket_option", "cpus", "service_cpu_affinity",
  "dnstap", "dnstap_block", "dnstap_option", "remote_control",
  "remote_control_block", "remote_control_option", "tls_auth", "$@2",
  "tls_auth_block", "tls_auth_option", "key", "$@3", "key_block",
  "key_option", "zone", "$@4", "zone_block", "zone_option", "pattern",
  "$@5", "pattern_block", "pattern_option", "pattern_or_zone_option",
  "$@6", "$@7", "verify", "verify_block", "verify_option", "command",
  "arguments", "ip_address", "number", "boolean", "tlsauth_option", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
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
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391
};
#endif

#define YYPACT_NINF (-161)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -161,    53,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
     335,   -41,   -36,  -161,  -161,  -161,  -161,     3,     0,     9,
      13,    13,    13,     0,     0,    13,    13,    13,    13,    13,
       0,    13,     0,    14,    16,    18,    25,    48,    49,    13,
      55,    57,    61,    62,    13,    13,    63,    65,    68,     0,
      13,     0,     0,     0,     0,     0,     0,     0,     0,    13,
      13,    13,    13,    13,    13,     0,     0,     0,     0,     0,
       0,     0,    69,    72,    74,     0,    75,  -161,  -161,  -161,
      13,     0,     0,    13,    76,    78,  -161,     0,    13,    89,
      13,    13,    90,    92,    13,    13,  -161,    13,     9,     0,
     100,   101,   102,   104,  -161,   -74,    23,   176,   315,     9,
       0,    13,    13,   105,     0,    13,     0,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
     108,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,   117,   118,   119,  -161,   121,   122,   123,
     124,   125,  -161,   126,   128,   131,   139,   141,     0,   143,
       8,    13,   146,     0,     0,     0,     0,   147,    13,     0,
     148,   149,    13,     0,     0,    13,   150,    13,   105,    13,
       0,  -161,  -161,   151,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,   152,   153,   154,
    -161,   155,   156,   157,   158,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,   159,  -101,  -161,  -161,
    -161,  -161,  -161,   160,   161,  -161,   162,    13,     0,  -161,
     163,  -161,  -161,  -161,  -161,  -161,  -161,  -161,   163,  -161
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    14,    94,   105,   122,   113,   135,   129,
     174,     3,     4,     5,     6,     8,     7,    10,     9,    11,
      12,    92,   103,   125,   116,   138,   132,   172,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    88,    90,    91,
       0,     0,     0,     0,     0,     0,    13,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    93,     0,     0,     0,
       0,     0,     0,     0,   104,   123,   114,   136,   130,     0,
       0,     0,     0,     0,     0,     0,     0,   173,   187,    17,
     186,    15,   188,    18,    19,    47,    20,    21,    22,    27,
      28,    29,    30,    46,    23,    57,    50,    49,    51,    52,
      31,    35,    36,    45,    53,    54,    55,    24,    25,    33,
      32,    34,    37,    38,    39,    40,    41,    42,    43,    44,
      48,    56,    66,    67,    68,    69,    70,    64,    65,    58,
      59,    60,    61,    62,    63,    71,    73,    72,    74,    75,
      81,    26,    79,    80,    76,    77,    78,    82,    95,    96,
      97,    98,    99,   100,   101,   102,   106,   107,   108,   109,
     110,   111,   112,     0,     0,     0,   124,     0,     0,     0,
       0,     0,   115,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   137,   140,     0,   131,   134,   176,   177,   175,   178,
     184,   179,   180,   182,   181,    83,    89,   127,   128,   126,
     118,   119,   120,   121,   117,   139,   142,     0,     0,     0,
     158,     0,     0,     0,     0,   157,   156,   159,   160,   161,
     162,   163,   145,   144,   143,   146,   164,   165,   166,   167,
     141,   168,   169,   170,   171,   133,   183,    16,   153,   154,
     155,   152,   147,     0,     0,   185,     0,     0,     0,    84,
     189,   149,   151,    85,    86,    87,   190,   148,   189,   150
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,   -62,  -161,  -161,  -161,  -161,  -161,   -81,
    -161,  -106,    21,   -31,  -160
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    11,    12,    20,    96,   265,   307,   319,   190,
      97,    13,    21,   106,    14,    22,   114,    15,    24,   116,
     222,    16,    23,   115,   216,    17,    26,   118,   254,    18,
      25,   117,   251,   252,   320,   328,    19,    27,   127,   261,
     306,   131,   129,   133,   327
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     134,   135,   207,   128,   138,   139,   140,   141,   142,   119,
     144,   282,   130,   256,   213,   214,   132,   146,   152,   147,
     120,   148,   215,   157,   158,   316,   317,   318,   149,   163,
      98,    99,   100,   101,   102,   103,   104,   105,   172,   173,
     174,   175,   176,   177,   107,   108,   109,   110,   111,   112,
     113,   150,   151,     2,   136,   137,   255,     3,   153,   191,
     154,   143,   194,   145,   155,   156,   159,   198,   160,   200,
     201,   161,   185,   204,   205,   186,   206,   187,   189,   195,
     162,   196,   164,   165,   166,   167,   168,   169,   170,   171,
     258,   259,   199,   202,   263,   203,   178,   179,   180,   181,
     182,   183,   184,   209,   210,   211,   188,   212,   260,   283,
     284,   266,   192,   193,   217,   218,   219,   220,   197,   221,
     267,   268,   269,     4,   270,   271,   272,   273,   274,   275,
     208,   276,     5,   121,   277,   122,   123,   124,   125,   126,
       6,   257,   278,     7,   279,   262,   281,   264,     8,   286,
     291,   294,   295,   300,   305,   308,   309,   310,   311,   312,
     313,   314,   315,   321,   322,   323,   326,   302,   329,     0,
       0,     0,     0,     0,     0,     0,     0,     9,     0,     0,
       0,     0,    10,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     285,     0,     0,     0,     0,     0,     0,   292,     0,     0,
       0,   296,     0,     0,   299,     0,   301,     0,   303,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   280,
       0,     0,     0,     0,   287,   288,   289,   290,     0,     0,
     293,     0,     0,     0,   297,   298,     0,     0,     0,     0,
       0,   304,   223,   224,   225,   226,   227,     0,     0,   228,
     229,   230,   231,   232,     0,     0,   324,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
       0,   246,     0,     0,     0,     0,     0,   247,     0,   248,
       0,   249,   250,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   325,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,     0,     0,     0,     0,     0,
       0,   253,   224,   225,   226,   227,     0,     0,   228,   229,
     230,   231,   232,     0,     0,     0,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,     0,
     246,     0,     0,    93,    94,    95,   247,     0,   248,     0,
     249,   250
};

static const yytype_int16 yycheck[] =
{
      31,    32,   108,     3,    35,    36,    37,    38,    39,     6,
      41,     3,     3,   119,    88,    89,     3,     3,    49,     3,
      17,     3,    96,    54,    55,   126,   127,   128,     3,    60,
      71,    72,    73,    74,    75,    76,    77,    78,    69,    70,
      71,    72,    73,    74,    80,    81,    82,    83,    84,    85,
      86,     3,     3,     0,    33,    34,   118,     4,     3,    90,
       3,    40,    93,    42,     3,     3,     3,    98,     3,   100,
     101,     3,     3,   104,   105,     3,   107,     3,     3,     3,
      59,     3,    61,    62,    63,    64,    65,    66,    67,    68,
     121,   122,     3,     3,   125,     3,    75,    76,    77,    78,
      79,    80,    81,     3,     3,     3,    85,     3,     3,   101,
     102,     3,    91,    92,    91,    92,    93,    94,    97,    96,
       3,     3,     3,    70,     3,     3,     3,     3,     3,     3,
     109,     3,    79,   130,     3,   132,   133,   134,   135,   136,
      87,   120,     3,    90,     3,   124,     3,   126,    95,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,   248,   328,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   124,    -1,    -1,
      -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     231,    -1,    -1,    -1,    -1,    -1,    -1,   238,    -1,    -1,
      -1,   242,    -1,    -1,   245,    -1,   247,    -1,   249,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   228,
      -1,    -1,    -1,    -1,   233,   234,   235,   236,    -1,    -1,
     239,    -1,    -1,    -1,   243,   244,    -1,    -1,    -1,    -1,
      -1,   250,    96,    97,    98,    99,   100,    -1,    -1,   103,
     104,   105,   106,   107,    -1,    -1,   317,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
      -1,   125,    -1,    -1,    -1,    -1,    -1,   131,    -1,   133,
      -1,   135,   136,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   318,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    -1,    -1,    -1,    -1,    -1,
      -1,    96,    97,    98,    99,   100,    -1,    -1,   103,   104,
     105,   106,   107,    -1,    -1,    -1,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,    -1,
     125,    -1,    -1,   108,   109,   110,   131,    -1,   133,    -1,
     135,   136
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   138,     0,     4,    70,    79,    87,    90,    95,   124,
     129,   139,   140,   148,   151,   154,   158,   162,   166,   173,
     141,   149,   152,   159,   155,   167,   163,   174,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,   108,   109,   110,   142,   147,    71,    72,
      73,    74,    75,    76,    77,    78,   150,    80,    81,    82,
      83,    84,    85,    86,   153,   160,   156,   168,   164,     6,
      17,   130,   132,   133,   134,   135,   136,   175,     3,   179,
       3,   178,     3,   180,   180,   180,   179,   179,   180,   180,
     180,   180,   180,   179,   180,   179,     3,     3,     3,     3,
       3,     3,   180,     3,     3,     3,     3,   180,   180,     3,
       3,     3,   179,   180,   179,   179,   179,   179,   179,   179,
     179,   179,   180,   180,   180,   180,   180,   180,   179,   179,
     179,   179,   179,   179,   179,     3,     3,     3,   179,     3,
     146,   180,   179,   179,   180,     3,     3,   179,   180,     3,
     180,   180,     3,     3,   180,   180,   180,   178,   179,     3,
       3,     3,     3,    88,    89,    96,   161,    91,    92,    93,
      94,    96,   157,    96,    97,    98,    99,   100,   103,   104,
     105,   106,   107,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   125,   131,   133,   135,
     136,   169,   170,    96,   165,   170,   178,   179,   180,   180,
       3,   176,   179,   180,   179,   143,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     179,     3,     3,   101,   102,   180,     3,   179,   179,   179,
     179,     3,   180,   179,     3,     3,   180,   179,   179,   180,
       3,   180,   176,   180,   179,     3,   177,   144,     3,     3,
       3,     3,     3,     3,     3,     3,   126,   127,   128,   145,
     171,     3,     3,     3,   180,   179,     3,   181,   172,   181
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   137,   138,   138,   139,   139,   139,   139,   139,   139,
     139,   139,   140,   141,   141,   143,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   144,   144,   145,   145,   145,   146,   146,
     147,   147,   148,   149,   149,   150,   150,   150,   150,   150,
     150,   150,   150,   151,   152,   152,   153,   153,   153,   153,
     153,   153,   153,   155,   154,   156,   156,   157,   157,   157,
     157,   157,   159,   158,   160,   160,   161,   161,   161,   163,
     162,   164,   164,   165,   165,   167,   166,   168,   168,   169,
     169,   170,   170,   170,   170,   170,   170,   171,   170,   172,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   173,   174,   174,   175,   175,   175,   175,   175,
     175,   175,   175,   176,   177,   177,   178,   179,   180,   181,
     181
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     0,     0,     4,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     0,     2,     2,     2,     2,     0,     2,
       1,     1,     2,     2,     0,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     0,     2,     2,     2,     2,
       2,     2,     2,     0,     3,     2,     0,     2,     2,     2,
       2,     2,     0,     3,     2,     0,     2,     2,     2,     0,
       3,     2,     0,     2,     1,     0,     3,     2,     0,     2,
       1,     2,     2,     2,     2,     2,     2,     0,     5,     0,
       6,     4,     3,     3,     3,     3,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     0,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     0,     2,     1,     1,     1,     0,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
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
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
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
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
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
  case 15: /* $@1: %empty  */
#line 242 "configparser.y"
      {
        struct ip_address_option *ip = cfg_parser->opt->ip_addresses;

        if(ip == NULL) {
          cfg_parser->opt->ip_addresses = (yyvsp[0].ip);
        } else {
          while(ip->next) { ip = ip->next; }
          ip->next = (yyvsp[0].ip);
        }

        cfg_parser->ip = (yyvsp[0].ip);
      }
#line 1615 "configparser.c"
    break;

  case 16: /* server_option: VAR_IP_ADDRESS ip_address $@1 socket_options  */
#line 255 "configparser.y"
    {
      cfg_parser->ip = NULL;
    }
#line 1623 "configparser.c"
    break;

  case 17: /* server_option: VAR_SERVER_COUNT number  */
#line 259 "configparser.y"
    {
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->server_count = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 1635 "configparser.c"
    break;

  case 18: /* server_option: VAR_IP_TRANSPARENT boolean  */
#line 267 "configparser.y"
    { cfg_parser->opt->ip_transparent = (yyvsp[0].bln); }
#line 1641 "configparser.c"
    break;

  case 19: /* server_option: VAR_IP_FREEBIND boolean  */
#line 269 "configparser.y"
    { cfg_parser->opt->ip_freebind = (yyvsp[0].bln); }
#line 1647 "configparser.c"
    break;

  case 20: /* server_option: VAR_SEND_BUFFER_SIZE number  */
#line 271 "configparser.y"
    { cfg_parser->opt->send_buffer_size = (int)(yyvsp[0].llng); }
#line 1653 "configparser.c"
    break;

  case 21: /* server_option: VAR_RECEIVE_BUFFER_SIZE number  */
#line 273 "configparser.y"
    { cfg_parser->opt->receive_buffer_size = (int)(yyvsp[0].llng); }
#line 1659 "configparser.c"
    break;

  case 22: /* server_option: VAR_DEBUG_MODE boolean  */
#line 275 "configparser.y"
    { cfg_parser->opt->debug_mode = (yyvsp[0].bln); }
#line 1665 "configparser.c"
    break;

  case 23: /* server_option: VAR_USE_SYSTEMD boolean  */
#line 277 "configparser.y"
    { /* ignored, deprecated */ }
#line 1671 "configparser.c"
    break;

  case 24: /* server_option: VAR_HIDE_VERSION boolean  */
#line 279 "configparser.y"
    { cfg_parser->opt->hide_version = (yyvsp[0].bln); }
#line 1677 "configparser.c"
    break;

  case 25: /* server_option: VAR_HIDE_IDENTITY boolean  */
#line 281 "configparser.y"
    { cfg_parser->opt->hide_identity = (yyvsp[0].bln); }
#line 1683 "configparser.c"
    break;

  case 26: /* server_option: VAR_DROP_UPDATES boolean  */
#line 283 "configparser.y"
    { cfg_parser->opt->drop_updates = (yyvsp[0].bln); }
#line 1689 "configparser.c"
    break;

  case 27: /* server_option: VAR_IP4_ONLY boolean  */
#line 285 "configparser.y"
    { if((yyvsp[0].bln)) { cfg_parser->opt->do_ip4 = 1; cfg_parser->opt->do_ip6 = 0; } }
#line 1695 "configparser.c"
    break;

  case 28: /* server_option: VAR_IP6_ONLY boolean  */
#line 287 "configparser.y"
    { if((yyvsp[0].bln)) { cfg_parser->opt->do_ip4 = 0; cfg_parser->opt->do_ip6 = 1; } }
#line 1701 "configparser.c"
    break;

  case 29: /* server_option: VAR_DO_IP4 boolean  */
#line 289 "configparser.y"
    { cfg_parser->opt->do_ip4 = (yyvsp[0].bln); }
#line 1707 "configparser.c"
    break;

  case 30: /* server_option: VAR_DO_IP6 boolean  */
#line 291 "configparser.y"
    { cfg_parser->opt->do_ip6 = (yyvsp[0].bln); }
#line 1713 "configparser.c"
    break;

  case 31: /* server_option: VAR_DATABASE STRING  */
#line 293 "configparser.y"
    {
      cfg_parser->opt->database = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      if(cfg_parser->opt->database[0] == 0 &&
         cfg_parser->opt->zonefiles_write == 0)
      {
        cfg_parser->opt->zonefiles_write = ZONEFILES_WRITE_INTERVAL;
      }
    }
#line 1726 "configparser.c"
    break;

  case 32: /* server_option: VAR_IDENTITY STRING  */
#line 302 "configparser.y"
    { cfg_parser->opt->identity = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1732 "configparser.c"
    break;

  case 33: /* server_option: VAR_VERSION STRING  */
#line 304 "configparser.y"
    { cfg_parser->opt->version = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1738 "configparser.c"
    break;

  case 34: /* server_option: VAR_NSID STRING  */
#line 306 "configparser.y"
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
#line 1772 "configparser.c"
    break;

  case 35: /* server_option: VAR_LOGFILE STRING  */
#line 336 "configparser.y"
    { cfg_parser->opt->logfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1778 "configparser.c"
    break;

  case 36: /* server_option: VAR_LOG_ONLY_SYSLOG boolean  */
#line 338 "configparser.y"
    { cfg_parser->opt->log_only_syslog = (yyvsp[0].bln); }
#line 1784 "configparser.c"
    break;

  case 37: /* server_option: VAR_TCP_COUNT number  */
#line 340 "configparser.y"
    {
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->tcp_count = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 1796 "configparser.c"
    break;

  case 38: /* server_option: VAR_TCP_REJECT_OVERFLOW boolean  */
#line 348 "configparser.y"
    { cfg_parser->opt->tcp_reject_overflow = (yyvsp[0].bln); }
#line 1802 "configparser.c"
    break;

  case 39: /* server_option: VAR_TCP_QUERY_COUNT number  */
#line 350 "configparser.y"
    { cfg_parser->opt->tcp_query_count = (int)(yyvsp[0].llng); }
#line 1808 "configparser.c"
    break;

  case 40: /* server_option: VAR_TCP_TIMEOUT number  */
#line 352 "configparser.y"
    { cfg_parser->opt->tcp_timeout = (int)(yyvsp[0].llng); }
#line 1814 "configparser.c"
    break;

  case 41: /* server_option: VAR_TCP_MSS number  */
#line 354 "configparser.y"
    { cfg_parser->opt->tcp_mss = (int)(yyvsp[0].llng); }
#line 1820 "configparser.c"
    break;

  case 42: /* server_option: VAR_OUTGOING_TCP_MSS number  */
#line 356 "configparser.y"
    { cfg_parser->opt->outgoing_tcp_mss = (int)(yyvsp[0].llng); }
#line 1826 "configparser.c"
    break;

  case 43: /* server_option: VAR_IPV4_EDNS_SIZE number  */
#line 358 "configparser.y"
    { cfg_parser->opt->ipv4_edns_size = (size_t)(yyvsp[0].llng); }
#line 1832 "configparser.c"
    break;

  case 44: /* server_option: VAR_IPV6_EDNS_SIZE number  */
#line 360 "configparser.y"
    { cfg_parser->opt->ipv6_edns_size = (size_t)(yyvsp[0].llng); }
#line 1838 "configparser.c"
    break;

  case 45: /* server_option: VAR_PIDFILE STRING  */
#line 362 "configparser.y"
    { cfg_parser->opt->pidfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1844 "configparser.c"
    break;

  case 46: /* server_option: VAR_PORT number  */
#line 364 "configparser.y"
    {
      /* port number, stored as a string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 1855 "configparser.c"
    break;

  case 47: /* server_option: VAR_REUSEPORT boolean  */
#line 371 "configparser.y"
    { cfg_parser->opt->reuseport = (yyvsp[0].bln); }
#line 1861 "configparser.c"
    break;

  case 48: /* server_option: VAR_STATISTICS number  */
#line 373 "configparser.y"
    { cfg_parser->opt->statistics = (int)(yyvsp[0].llng); }
#line 1867 "configparser.c"
    break;

  case 49: /* server_option: VAR_CHROOT STRING  */
#line 375 "configparser.y"
    { cfg_parser->opt->chroot = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1873 "configparser.c"
    break;

  case 50: /* server_option: VAR_USERNAME STRING  */
#line 377 "configparser.y"
    { cfg_parser->opt->username = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1879 "configparser.c"
    break;

  case 51: /* server_option: VAR_ZONESDIR STRING  */
#line 379 "configparser.y"
    { cfg_parser->opt->zonesdir = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1885 "configparser.c"
    break;

  case 52: /* server_option: VAR_ZONELISTFILE STRING  */
#line 381 "configparser.y"
    { cfg_parser->opt->zonelistfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1891 "configparser.c"
    break;

  case 53: /* server_option: VAR_DIFFFILE STRING  */
#line 383 "configparser.y"
    { /* ignored, deprecated */ }
#line 1897 "configparser.c"
    break;

  case 54: /* server_option: VAR_XFRDFILE STRING  */
#line 385 "configparser.y"
    { cfg_parser->opt->xfrdfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1903 "configparser.c"
    break;

  case 55: /* server_option: VAR_XFRDIR STRING  */
#line 387 "configparser.y"
    { cfg_parser->opt->xfrdir = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1909 "configparser.c"
    break;

  case 56: /* server_option: VAR_XFRD_RELOAD_TIMEOUT number  */
#line 389 "configparser.y"
    { cfg_parser->opt->xfrd_reload_timeout = (int)(yyvsp[0].llng); }
#line 1915 "configparser.c"
    break;

  case 57: /* server_option: VAR_VERBOSITY number  */
#line 391 "configparser.y"
    { cfg_parser->opt->verbosity = (int)(yyvsp[0].llng); }
#line 1921 "configparser.c"
    break;

  case 58: /* server_option: VAR_RRL_SIZE number  */
#line 393 "configparser.y"
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->rrl_size = (size_t)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
#endif
    }
#line 1935 "configparser.c"
    break;

  case 59: /* server_option: VAR_RRL_RATELIMIT number  */
#line 403 "configparser.y"
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_ratelimit = (size_t)(yyvsp[0].llng);
#endif
    }
#line 1945 "configparser.c"
    break;

  case 60: /* server_option: VAR_RRL_SLIP number  */
#line 409 "configparser.y"
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_slip = (size_t)(yyvsp[0].llng);
#endif
    }
#line 1955 "configparser.c"
    break;

  case 61: /* server_option: VAR_RRL_IPV4_PREFIX_LENGTH number  */
#line 415 "configparser.y"
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 32) {
        yyerror("invalid IPv4 prefix length");
      } else {
        cfg_parser->opt->rrl_ipv4_prefix_length = (size_t)(yyvsp[0].llng);
      }
#endif
    }
#line 1969 "configparser.c"
    break;

  case 62: /* server_option: VAR_RRL_IPV6_PREFIX_LENGTH number  */
#line 425 "configparser.y"
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 64) {
        yyerror("invalid IPv6 prefix length");
      } else {
        cfg_parser->opt->rrl_ipv6_prefix_length = (size_t)(yyvsp[0].llng);
      }
#endif
    }
#line 1983 "configparser.c"
    break;

  case 63: /* server_option: VAR_RRL_WHITELIST_RATELIMIT number  */
#line 435 "configparser.y"
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_whitelist_ratelimit = (size_t)(yyvsp[0].llng);
#endif
    }
#line 1993 "configparser.c"
    break;

  case 64: /* server_option: VAR_ZONEFILES_CHECK boolean  */
#line 441 "configparser.y"
    { cfg_parser->opt->zonefiles_check = (yyvsp[0].bln); }
#line 1999 "configparser.c"
    break;

  case 65: /* server_option: VAR_ZONEFILES_WRITE number  */
#line 443 "configparser.y"
    { cfg_parser->opt->zonefiles_write = (int)(yyvsp[0].llng); }
#line 2005 "configparser.c"
    break;

  case 66: /* server_option: VAR_LOG_TIME_ASCII boolean  */
#line 445 "configparser.y"
    {
      cfg_parser->opt->log_time_ascii = (yyvsp[0].bln);
      log_time_asc = cfg_parser->opt->log_time_ascii;
    }
#line 2014 "configparser.c"
    break;

  case 67: /* server_option: VAR_ROUND_ROBIN boolean  */
#line 450 "configparser.y"
    {
      cfg_parser->opt->round_robin = (yyvsp[0].bln);
      round_robin = cfg_parser->opt->round_robin;
    }
#line 2023 "configparser.c"
    break;

  case 68: /* server_option: VAR_MINIMAL_RESPONSES boolean  */
#line 455 "configparser.y"
    {
      cfg_parser->opt->minimal_responses = (yyvsp[0].bln);
      minimal_responses = cfg_parser->opt->minimal_responses;
    }
#line 2032 "configparser.c"
    break;

  case 69: /* server_option: VAR_CONFINE_TO_ZONE boolean  */
#line 460 "configparser.y"
    { cfg_parser->opt->confine_to_zone = (yyvsp[0].bln); }
#line 2038 "configparser.c"
    break;

  case 70: /* server_option: VAR_REFUSE_ANY boolean  */
#line 462 "configparser.y"
    { cfg_parser->opt->refuse_any = (yyvsp[0].bln); }
#line 2044 "configparser.c"
    break;

  case 71: /* server_option: VAR_TLS_SERVICE_KEY STRING  */
#line 464 "configparser.y"
    { cfg_parser->opt->tls_service_key = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2050 "configparser.c"
    break;

  case 72: /* server_option: VAR_TLS_SERVICE_OCSP STRING  */
#line 466 "configparser.y"
    { cfg_parser->opt->tls_service_ocsp = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2056 "configparser.c"
    break;

  case 73: /* server_option: VAR_TLS_SERVICE_PEM STRING  */
#line 468 "configparser.y"
    { cfg_parser->opt->tls_service_pem = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2062 "configparser.c"
    break;

  case 74: /* server_option: VAR_TLS_PORT number  */
#line 470 "configparser.y"
    {
      /* port number, stored as string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->tls_port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 2073 "configparser.c"
    break;

  case 75: /* server_option: VAR_TLS_CERT_BUNDLE STRING  */
#line 477 "configparser.y"
    { cfg_parser->opt->tls_cert_bundle = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2079 "configparser.c"
    break;

  case 76: /* server_option: VAR_ANSWER_COOKIE boolean  */
#line 479 "configparser.y"
    { cfg_parser->opt->answer_cookie = (yyvsp[0].bln); }
#line 2085 "configparser.c"
    break;

  case 77: /* server_option: VAR_COOKIE_SECRET STRING  */
#line 481 "configparser.y"
    { cfg_parser->opt->cookie_secret = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2091 "configparser.c"
    break;

  case 78: /* server_option: VAR_COOKIE_SECRET_FILE STRING  */
#line 483 "configparser.y"
    { cfg_parser->opt->cookie_secret_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2097 "configparser.c"
    break;

  case 79: /* server_option: VAR_XFRD_TCP_MAX number  */
#line 485 "configparser.y"
    { cfg_parser->opt->xfrd_tcp_max = (int)(yyvsp[0].llng); }
#line 2103 "configparser.c"
    break;

  case 80: /* server_option: VAR_XFRD_TCP_PIPELINE number  */
#line 487 "configparser.y"
    { cfg_parser->opt->xfrd_tcp_pipeline = (int)(yyvsp[0].llng); }
#line 2109 "configparser.c"
    break;

  case 81: /* server_option: VAR_CPU_AFFINITY cpus  */
#line 489 "configparser.y"
    {
      cfg_parser->opt->cpu_affinity = (yyvsp[0].cpu);
    }
#line 2117 "configparser.c"
    break;

  case 82: /* server_option: service_cpu_affinity number  */
#line 493 "configparser.y"
    {
      if((yyvsp[0].llng) < 0) {
        yyerror("expected a non-negative number");
        YYABORT;
      } else {
        struct cpu_map_option *opt, *tail;

        opt = cfg_parser->opt->service_cpu_affinity;
        while(opt && opt->service != (yyvsp[-1].llng)) { opt = opt->next; }

        if(opt) {
          opt->cpu = (yyvsp[0].llng);
        } else {
          opt = region_alloc_zero(cfg_parser->opt->region, sizeof(*opt));
          opt->service = (int)(yyvsp[-1].llng);
          opt->cpu = (int)(yyvsp[0].llng);

          tail = cfg_parser->opt->service_cpu_affinity;
          if(tail) {
            while(tail->next) { tail = tail->next; }
            tail->next = opt;
          } else {
            cfg_parser->opt->service_cpu_affinity = opt;
          }
        }
      }
    }
#line 2149 "configparser.c"
    break;

  case 85: /* socket_option: VAR_SERVERS STRING  */
#line 527 "configparser.y"
    {
      char *tok, *ptr, *str;
      struct range_option *servers = NULL;
      long long first, last;

      /* user may specify "0 1", "0" "1", 0 1 or a combination thereof */
      for(str = (yyvsp[0].str); (tok = strtok_r(str, " \t", &ptr)); str = NULL) {
        struct range_option *opt =
          region_alloc(cfg_parser->opt->region, sizeof(*opt));
        first = last = 0;
        if(!parse_range(tok, &first, &last)) {
          yyerror("invalid server range '%s'", tok);
          YYABORT;
        }
        assert(first >= 0);
        assert(last >= 0);
        opt->next = NULL;
        opt->first = (int)first;
        opt->last = (int)last;
        if(servers) {
          servers = servers->next = opt;
        } else {
          servers = cfg_parser->ip->servers = opt;
        }
      }
    }
#line 2180 "configparser.c"
    break;

  case 86: /* socket_option: VAR_BINDTODEVICE boolean  */
#line 554 "configparser.y"
    { cfg_parser->ip->dev = (yyvsp[0].bln); }
#line 2186 "configparser.c"
    break;

  case 87: /* socket_option: VAR_SETFIB number  */
#line 556 "configparser.y"
    { cfg_parser->ip->fib = (yyvsp[0].llng); }
#line 2192 "configparser.c"
    break;

  case 88: /* cpus: %empty  */
#line 560 "configparser.y"
    { (yyval.cpu) = NULL; }
#line 2198 "configparser.c"
    break;

  case 89: /* cpus: cpus STRING  */
#line 562 "configparser.y"
    {
      char *tok, *ptr, *str;
      struct cpu_option *tail;
      long long cpu;

      str = (yyvsp[0].str);
      (yyval.cpu) = tail = (yyvsp[-1].cpu);
      if(tail) {
        while(tail->next) { tail = tail->next; }
      }

      /* Users may specify "0 1", "0" "1", 0 1 or a combination thereof. */
      for(str = (yyvsp[0].str); (tok = strtok_r(str, " \t", &ptr)); str = NULL) {
        struct cpu_option *opt =
          region_alloc_zero(cfg_parser->opt->region, sizeof(*opt));
        cpu = 0;
        if(!parse_number(tok, &cpu) || cpu < 0) {
          yyerror("expected a positive number");
          YYABORT;
        }
        assert(cpu >=0);
        opt->cpu = (int)cpu;
        if(tail) {
          tail->next = opt;
          tail = opt;
        } else {
          (yyval.cpu) = tail = opt;
        }
      }
    }
#line 2233 "configparser.c"
    break;

  case 90: /* service_cpu_affinity: VAR_XFRD_CPU_AFFINITY  */
#line 596 "configparser.y"
    { (yyval.llng) = -1; }
#line 2239 "configparser.c"
    break;

  case 91: /* service_cpu_affinity: VAR_SERVER_CPU_AFFINITY  */
#line 598 "configparser.y"
    {
      if((yyvsp[0].llng) <= 0) {
        yyerror("invalid server identifier");
        YYABORT;
      }
      (yyval.llng) = (yyvsp[0].llng);
    }
#line 2251 "configparser.c"
    break;

  case 95: /* dnstap_option: VAR_DNSTAP_ENABLE boolean  */
#line 615 "configparser.y"
    { cfg_parser->opt->dnstap_enable = (yyvsp[0].bln); }
#line 2257 "configparser.c"
    break;

  case 96: /* dnstap_option: VAR_DNSTAP_SOCKET_PATH STRING  */
#line 617 "configparser.y"
    { cfg_parser->opt->dnstap_socket_path = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2263 "configparser.c"
    break;

  case 97: /* dnstap_option: VAR_DNSTAP_SEND_IDENTITY boolean  */
#line 619 "configparser.y"
    { cfg_parser->opt->dnstap_send_identity = (yyvsp[0].bln); }
#line 2269 "configparser.c"
    break;

  case 98: /* dnstap_option: VAR_DNSTAP_SEND_VERSION boolean  */
#line 621 "configparser.y"
    { cfg_parser->opt->dnstap_send_version = (yyvsp[0].bln); }
#line 2275 "configparser.c"
    break;

  case 99: /* dnstap_option: VAR_DNSTAP_IDENTITY STRING  */
#line 623 "configparser.y"
    { cfg_parser->opt->dnstap_identity = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2281 "configparser.c"
    break;

  case 100: /* dnstap_option: VAR_DNSTAP_VERSION STRING  */
#line 625 "configparser.y"
    { cfg_parser->opt->dnstap_version = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2287 "configparser.c"
    break;

  case 101: /* dnstap_option: VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES boolean  */
#line 627 "configparser.y"
    { cfg_parser->opt->dnstap_log_auth_query_messages = (yyvsp[0].bln); }
#line 2293 "configparser.c"
    break;

  case 102: /* dnstap_option: VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES boolean  */
#line 629 "configparser.y"
    { cfg_parser->opt->dnstap_log_auth_response_messages = (yyvsp[0].bln); }
#line 2299 "configparser.c"
    break;

  case 106: /* remote_control_option: VAR_CONTROL_ENABLE boolean  */
#line 640 "configparser.y"
    { cfg_parser->opt->control_enable = (yyvsp[0].bln); }
#line 2305 "configparser.c"
    break;

  case 107: /* remote_control_option: VAR_CONTROL_INTERFACE ip_address  */
#line 642 "configparser.y"
    {
      struct ip_address_option *ip = cfg_parser->opt->control_interface;
      if(ip == NULL) {
        cfg_parser->opt->control_interface = (yyvsp[0].ip);
      } else {
        while(ip->next != NULL) { ip = ip->next; }
        ip->next = (yyvsp[0].ip);
      }
    }
#line 2319 "configparser.c"
    break;

  case 108: /* remote_control_option: VAR_CONTROL_PORT number  */
#line 652 "configparser.y"
    {
      if((yyvsp[0].llng) == 0) {
        yyerror("control port number expected");
      } else {
        cfg_parser->opt->control_port = (int)(yyvsp[0].llng);
      }
    }
#line 2331 "configparser.c"
    break;

  case 109: /* remote_control_option: VAR_SERVER_KEY_FILE STRING  */
#line 660 "configparser.y"
    { cfg_parser->opt->server_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2337 "configparser.c"
    break;

  case 110: /* remote_control_option: VAR_SERVER_CERT_FILE STRING  */
#line 662 "configparser.y"
    { cfg_parser->opt->server_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2343 "configparser.c"
    break;

  case 111: /* remote_control_option: VAR_CONTROL_KEY_FILE STRING  */
#line 664 "configparser.y"
    { cfg_parser->opt->control_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2349 "configparser.c"
    break;

  case 112: /* remote_control_option: VAR_CONTROL_CERT_FILE STRING  */
#line 666 "configparser.y"
    { cfg_parser->opt->control_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2355 "configparser.c"
    break;

  case 113: /* $@2: %empty  */
#line 671 "configparser.y"
      {
        tls_auth_options_type *tls_auth = tls_auth_options_create(cfg_parser->opt->region);
        assert(cfg_parser->tls_auth == NULL);
        cfg_parser->tls_auth = tls_auth;
      }
#line 2365 "configparser.c"
    break;

  case 114: /* tls_auth: VAR_TLS_AUTH $@2 tls_auth_block  */
#line 677 "configparser.y"
    {
      struct tls_auth_options *tls_auth = cfg_parser->tls_auth;
      if(tls_auth->name == NULL) {
        yyerror("tls-auth has no name");
      } else if(tls_auth->auth_domain_name == NULL) {
        yyerror("tls-auth %s has no auth-domain-name", tls_auth->name);
      } else if(tls_auth_options_find(cfg_parser->opt, tls_auth->name)) {
        yyerror("duplicate tls-auth %s", tls_auth->name);
      } else {
      	tls_auth_options_insert(cfg_parser->opt, tls_auth);
        cfg_parser->tls_auth = NULL;
      }
    }
#line 2383 "configparser.c"
    break;

  case 117: /* tls_auth_option: VAR_NAME STRING  */
#line 696 "configparser.y"
    {
      dname_type *dname;
      dname = (dname_type *)dname_parse(cfg_parser->opt->region, (yyvsp[0].str));
      cfg_parser->tls_auth->name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      if(dname == NULL) {
        yyerror("bad tls-auth name %s", (yyvsp[0].str));
      } else {
        region_recycle(cfg_parser->opt->region, dname, dname_total_size(dname));
      }
    }
#line 2398 "configparser.c"
    break;

  case 118: /* tls_auth_option: VAR_TLS_AUTH_DOMAIN_NAME STRING  */
#line 707 "configparser.y"
    {
      cfg_parser->tls_auth->auth_domain_name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2406 "configparser.c"
    break;

  case 119: /* tls_auth_option: VAR_TLS_AUTH_CLIENT_CERT STRING  */
#line 711 "configparser.y"
    {
	    cfg_parser->tls_auth->client_cert = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2414 "configparser.c"
    break;

  case 120: /* tls_auth_option: VAR_TLS_AUTH_CLIENT_KEY STRING  */
#line 715 "configparser.y"
    {
	    cfg_parser->tls_auth->client_key = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2422 "configparser.c"
    break;

  case 121: /* tls_auth_option: VAR_TLS_AUTH_CLIENT_KEY_PW STRING  */
#line 719 "configparser.y"
    {
	    cfg_parser->tls_auth->client_key_pw = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2430 "configparser.c"
    break;

  case 122: /* $@3: %empty  */
#line 726 "configparser.y"
      {
        key_options_type *key = key_options_create(cfg_parser->opt->region);
        key->algorithm = region_strdup(cfg_parser->opt->region, "sha256");
        assert(cfg_parser->key == NULL);
        cfg_parser->key = key;
      }
#line 2441 "configparser.c"
    break;

  case 123: /* key: VAR_KEY $@3 key_block  */
#line 733 "configparser.y"
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
#line 2461 "configparser.c"
    break;

  case 126: /* key_option: VAR_NAME STRING  */
#line 754 "configparser.y"
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
#line 2477 "configparser.c"
    break;

  case 127: /* key_option: VAR_ALGORITHM STRING  */
#line 766 "configparser.y"
    {
      if(tsig_get_algorithm_by_name((yyvsp[0].str)) == NULL) {
        yyerror("bad tsig key algorithm %s", (yyvsp[0].str));
      } else {
        cfg_parser->key->algorithm = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      }
    }
#line 2489 "configparser.c"
    break;

  case 128: /* key_option: VAR_SECRET STRING  */
#line 774 "configparser.y"
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
#line 2508 "configparser.c"
    break;

  case 129: /* $@4: %empty  */
#line 792 "configparser.y"
      {
        assert(cfg_parser->pattern == NULL);
        assert(cfg_parser->zone == NULL);
        cfg_parser->zone = zone_options_create(cfg_parser->opt->region);
        cfg_parser->zone->part_of_config = 1;
        cfg_parser->zone->pattern = cfg_parser->pattern =
          pattern_options_create(cfg_parser->opt->region);
        cfg_parser->zone->pattern->implicit = 1;
      }
#line 2522 "configparser.c"
    break;

  case 130: /* zone: VAR_ZONE $@4 zone_block  */
#line 802 "configparser.y"
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
#line 2539 "configparser.c"
    break;

  case 133: /* zone_option: VAR_NAME STRING  */
#line 820 "configparser.y"
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
#line 2556 "configparser.c"
    break;

  case 135: /* $@5: %empty  */
#line 836 "configparser.y"
      {
        assert(cfg_parser->pattern == NULL);
        cfg_parser->pattern = pattern_options_create(cfg_parser->opt->region);
      }
#line 2565 "configparser.c"
    break;

  case 136: /* pattern: VAR_PATTERN $@5 pattern_block  */
#line 841 "configparser.y"
    {
      pattern_options_type *pattern = cfg_parser->pattern;
      if(pattern->pname == NULL) {
        yyerror("pattern has no name");
      } else if(!nsd_options_insert_pattern(cfg_parser->opt, pattern)) {
        yyerror("duplicate pattern %s", pattern->pname);
      }
      cfg_parser->pattern = NULL;
    }
#line 2579 "configparser.c"
    break;

  case 139: /* pattern_option: VAR_NAME STRING  */
#line 856 "configparser.y"
    {
      if(strchr((yyvsp[0].str), ' ')) {
        yyerror("space is not allowed in pattern name: '%s'", (yyvsp[0].str));
      }
      cfg_parser->pattern->pname = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2590 "configparser.c"
    break;

  case 141: /* pattern_or_zone_option: VAR_RRL_WHITELIST STRING  */
#line 866 "configparser.y"
    {
#ifdef RATELIMIT
      cfg_parser->pattern->rrl_whitelist |= rrlstr2type((yyvsp[0].str));
#endif
    }
#line 2600 "configparser.c"
    break;

  case 142: /* pattern_or_zone_option: VAR_ZONEFILE STRING  */
#line 872 "configparser.y"
    { cfg_parser->pattern->zonefile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2606 "configparser.c"
    break;

  case 143: /* pattern_or_zone_option: VAR_ZONESTATS STRING  */
#line 874 "configparser.y"
    { cfg_parser->pattern->zonestats = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2612 "configparser.c"
    break;

  case 144: /* pattern_or_zone_option: VAR_SIZE_LIMIT_XFR number  */
#line 876 "configparser.y"
    {
      if((yyvsp[0].llng) > 0) {
        cfg_parser->pattern->size_limit_xfr = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 2624 "configparser.c"
    break;

  case 145: /* pattern_or_zone_option: VAR_MULTI_MASTER_CHECK boolean  */
#line 884 "configparser.y"
    { cfg_parser->pattern->multi_master_check = (int)(yyvsp[0].bln); }
#line 2630 "configparser.c"
    break;

  case 146: /* pattern_or_zone_option: VAR_INCLUDE_PATTERN STRING  */
#line 886 "configparser.y"
    { config_apply_pattern(cfg_parser->pattern, (yyvsp[0].str)); }
#line 2636 "configparser.c"
    break;

  case 147: /* $@6: %empty  */
#line 888 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2649 "configparser.c"
    break;

  case 148: /* pattern_or_zone_option: VAR_REQUEST_XFR STRING STRING $@6 tlsauth_option  */
#line 897 "configparser.y"
        { }
#line 2655 "configparser.c"
    break;

  case 149: /* $@7: %empty  */
#line 899 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      acl->use_axfr_only = 1;
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2669 "configparser.c"
    break;

  case 150: /* pattern_or_zone_option: VAR_REQUEST_XFR VAR_AXFR STRING STRING $@7 tlsauth_option  */
#line 909 "configparser.y"
        { }
#line 2675 "configparser.c"
    break;

  case 151: /* pattern_or_zone_option: VAR_REQUEST_XFR VAR_UDP STRING STRING  */
#line 911 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      acl->allow_udp = 1;
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2689 "configparser.c"
    break;

  case 152: /* pattern_or_zone_option: VAR_ALLOW_NOTIFY STRING STRING  */
#line 921 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      append_acl(&cfg_parser->pattern->allow_notify, acl);
    }
#line 2698 "configparser.c"
    break;

  case 153: /* pattern_or_zone_option: VAR_NOTIFY STRING STRING  */
#line 926 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      if(acl->blocked)
        yyerror("blocked address used for notify");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for notify");
      append_acl(&cfg_parser->pattern->notify, acl);
    }
#line 2711 "configparser.c"
    break;

  case 154: /* pattern_or_zone_option: VAR_PROVIDE_XFR STRING STRING  */
#line 935 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      append_acl(&cfg_parser->pattern->provide_xfr, acl);
    }
#line 2720 "configparser.c"
    break;

  case 155: /* pattern_or_zone_option: VAR_ALLOW_QUERY STRING STRING  */
#line 940 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      append_acl(&cfg_parser->pattern->allow_query, acl);
    }
#line 2729 "configparser.c"
    break;

  case 156: /* pattern_or_zone_option: VAR_OUTGOING_INTERFACE STRING  */
#line 945 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[0].str), "NOKEY");
      append_acl(&cfg_parser->pattern->outgoing_interface, acl);
    }
#line 2738 "configparser.c"
    break;

  case 157: /* pattern_or_zone_option: VAR_ALLOW_AXFR_FALLBACK boolean  */
#line 950 "configparser.y"
    {
      cfg_parser->pattern->allow_axfr_fallback = (yyvsp[0].bln);
      cfg_parser->pattern->allow_axfr_fallback_is_default = 0;
    }
#line 2747 "configparser.c"
    break;

  case 158: /* pattern_or_zone_option: VAR_NOTIFY_RETRY number  */
#line 955 "configparser.y"
    {
      cfg_parser->pattern->notify_retry = (yyvsp[0].llng);
      cfg_parser->pattern->notify_retry_is_default = 0;
    }
#line 2756 "configparser.c"
    break;

  case 159: /* pattern_or_zone_option: VAR_MAX_REFRESH_TIME number  */
#line 960 "configparser.y"
    {
      cfg_parser->pattern->max_refresh_time = (yyvsp[0].llng);
      cfg_parser->pattern->max_refresh_time_is_default = 0;
    }
#line 2765 "configparser.c"
    break;

  case 160: /* pattern_or_zone_option: VAR_MIN_REFRESH_TIME number  */
#line 965 "configparser.y"
    {
      cfg_parser->pattern->min_refresh_time = (yyvsp[0].llng);
      cfg_parser->pattern->min_refresh_time_is_default = 0;
    }
#line 2774 "configparser.c"
    break;

  case 161: /* pattern_or_zone_option: VAR_MAX_RETRY_TIME number  */
#line 970 "configparser.y"
    {
      cfg_parser->pattern->max_retry_time = (yyvsp[0].llng);
      cfg_parser->pattern->max_retry_time_is_default = 0;
    }
#line 2783 "configparser.c"
    break;

  case 162: /* pattern_or_zone_option: VAR_MIN_RETRY_TIME number  */
#line 975 "configparser.y"
    {
      cfg_parser->pattern->min_retry_time = (yyvsp[0].llng);
      cfg_parser->pattern->min_retry_time_is_default = 0;
    }
#line 2792 "configparser.c"
    break;

  case 163: /* pattern_or_zone_option: VAR_MIN_EXPIRE_TIME STRING  */
#line 980 "configparser.y"
    {
      long long num;
      uint8_t expr;

      if (!parse_expire_expr((yyvsp[0].str), &num, &expr)) {
        yyerror("expected an expire time in seconds or \"refresh+retry+1\"");
        YYABORT; /* trigger a parser error */
      }
      cfg_parser->pattern->min_expire_time = num;
      cfg_parser->pattern->min_expire_time_expr = expr;
    }
#line 2808 "configparser.c"
    break;

  case 164: /* pattern_or_zone_option: VAR_STORE_IXFR boolean  */
#line 992 "configparser.y"
    {
      cfg_parser->pattern->store_ixfr = (yyvsp[0].bln);
      cfg_parser->pattern->store_ixfr_is_default = 0;
    }
#line 2817 "configparser.c"
    break;

  case 165: /* pattern_or_zone_option: VAR_IXFR_SIZE number  */
#line 997 "configparser.y"
    {
      cfg_parser->pattern->ixfr_size = (yyvsp[0].llng);
      cfg_parser->pattern->ixfr_size_is_default = 0;
    }
#line 2826 "configparser.c"
    break;

  case 166: /* pattern_or_zone_option: VAR_IXFR_NUMBER number  */
#line 1002 "configparser.y"
    {
      cfg_parser->pattern->ixfr_number = (yyvsp[0].llng);
      cfg_parser->pattern->ixfr_number_is_default = 0;
    }
#line 2835 "configparser.c"
    break;

  case 167: /* pattern_or_zone_option: VAR_CREATE_IXFR boolean  */
#line 1007 "configparser.y"
    {
      cfg_parser->pattern->create_ixfr = (yyvsp[0].bln);
      cfg_parser->pattern->create_ixfr_is_default = 0;
    }
#line 2844 "configparser.c"
    break;

  case 168: /* pattern_or_zone_option: VAR_VERIFY_ZONE boolean  */
#line 1012 "configparser.y"
    { cfg_parser->pattern->verify_zone = (yyvsp[0].bln); }
#line 2850 "configparser.c"
    break;

  case 169: /* pattern_or_zone_option: VAR_VERIFIER command  */
#line 1014 "configparser.y"
    { cfg_parser->pattern->verifier = (yyvsp[0].strv); }
#line 2856 "configparser.c"
    break;

  case 170: /* pattern_or_zone_option: VAR_VERIFIER_FEED_ZONE boolean  */
#line 1016 "configparser.y"
    { cfg_parser->pattern->verifier_feed_zone = (yyvsp[0].bln); }
#line 2862 "configparser.c"
    break;

  case 171: /* pattern_or_zone_option: VAR_VERIFIER_TIMEOUT number  */
#line 1018 "configparser.y"
    { cfg_parser->pattern->verifier_timeout = (yyvsp[0].llng); }
#line 2868 "configparser.c"
    break;

  case 175: /* verify_option: VAR_ENABLE boolean  */
#line 1028 "configparser.y"
    { cfg_parser->opt->verify_enable = (yyvsp[0].bln); }
#line 2874 "configparser.c"
    break;

  case 176: /* verify_option: VAR_IP_ADDRESS ip_address  */
#line 1030 "configparser.y"
    {
      struct ip_address_option *ip = cfg_parser->opt->verify_ip_addresses;
      if(!ip) {
        cfg_parser->opt->verify_ip_addresses = (yyvsp[0].ip);
      } else {
        while(ip->next) { ip = ip->next; }
        ip->next = (yyvsp[0].ip);
      }
    }
#line 2888 "configparser.c"
    break;

  case 177: /* verify_option: VAR_PORT number  */
#line 1040 "configparser.y"
    {
      /* port number, stored as a string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->verify_port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 2899 "configparser.c"
    break;

  case 178: /* verify_option: VAR_VERIFY_ZONES boolean  */
#line 1047 "configparser.y"
    { cfg_parser->opt->verify_zones = (yyvsp[0].bln); }
#line 2905 "configparser.c"
    break;

  case 179: /* verify_option: VAR_VERIFIER command  */
#line 1049 "configparser.y"
    { cfg_parser->opt->verifier = (yyvsp[0].strv); }
#line 2911 "configparser.c"
    break;

  case 180: /* verify_option: VAR_VERIFIER_COUNT number  */
#line 1051 "configparser.y"
    { cfg_parser->opt->verifier_count = (int)(yyvsp[0].llng); }
#line 2917 "configparser.c"
    break;

  case 181: /* verify_option: VAR_VERIFIER_TIMEOUT number  */
#line 1053 "configparser.y"
    { cfg_parser->opt->verifier_timeout = (int)(yyvsp[0].llng); }
#line 2923 "configparser.c"
    break;

  case 182: /* verify_option: VAR_VERIFIER_FEED_ZONE boolean  */
#line 1055 "configparser.y"
    { cfg_parser->opt->verifier_feed_zone = (yyvsp[0].bln); }
#line 2929 "configparser.c"
    break;

  case 183: /* command: STRING arguments  */
#line 1059 "configparser.y"
    {
      char **argv;
      size_t argc = 1;
      for(struct component *i = (yyvsp[0].comp); i; i = i->next) {
        argc++;
      }
      argv = region_alloc_zero(
        cfg_parser->opt->region, (argc + 1) * sizeof(char *));
      argc = 0;
      argv[argc++] = (yyvsp[-1].str);
      for(struct component *j, *i = (yyvsp[0].comp); i; i = j) {
        j = i->next;
        argv[argc++] = i->str;
        region_recycle(cfg_parser->opt->region, i, sizeof(*i));
      }
      (yyval.strv) = argv;
    }
#line 2951 "configparser.c"
    break;

  case 184: /* arguments: %empty  */
#line 1078 "configparser.y"
    { (yyval.comp) = NULL; }
#line 2957 "configparser.c"
    break;

  case 185: /* arguments: arguments STRING  */
#line 1080 "configparser.y"
    {
      struct component *comp = region_alloc_zero(
        cfg_parser->opt->region, sizeof(*comp));
      comp->str = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      if((yyvsp[-1].comp)) {
        struct component *tail = (yyvsp[-1].comp);
        while(tail->next) {
         tail = tail->next;
        }
        tail->next = comp;
        (yyval.comp) = (yyvsp[-1].comp);
      } else {
        (yyval.comp) = comp;
      }
    }
#line 2977 "configparser.c"
    break;

  case 186: /* ip_address: STRING  */
#line 1098 "configparser.y"
    {
      struct ip_address_option *ip = region_alloc_zero(
        cfg_parser->opt->region, sizeof(*ip));
      ip->address = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      ip->fib = -1;
      (yyval.ip) = ip;
    }
#line 2989 "configparser.c"
    break;

  case 187: /* number: STRING  */
#line 1108 "configparser.y"
    {
      if(!parse_number((yyvsp[0].str), &(yyval.llng))) {
        yyerror("expected a number");
        YYABORT; /* trigger a parser error */
      }
    }
#line 3000 "configparser.c"
    break;

  case 188: /* boolean: STRING  */
#line 1117 "configparser.y"
    {
      if(!parse_boolean((yyvsp[0].str), &(yyval.bln))) {
        yyerror("expected yes or no");
        YYABORT; /* trigger a parser error */
      }
    }
#line 3011 "configparser.c"
    break;

  case 190: /* tlsauth_option: STRING  */
#line 1126 "configparser.y"
        { char *tls_auth_name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	  add_to_last_acl(&cfg_parser->pattern->request_xfr, tls_auth_name);}
#line 3018 "configparser.c"
    break;


#line 3022 "configparser.c"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

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


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1129 "configparser.y"


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

static void
add_to_last_acl(struct acl_options **list, char *tls_auth_name)
{
	struct acl_options *tail = *list;
	assert(list != NULL);
	assert(*list != NULL);
	while(tail->next != NULL)
		tail = tail->next;
	tail->tls_auth_name = tls_auth_name;
}

static int
parse_boolean(const char *str, int *bln)
{
	if(strcmp(str, "yes") == 0) {
		*bln = 1;
	} else if(strcmp(str, "no") == 0) {
		*bln = 0;
	} else {
		return 0;
	}

	return 1;
}

static int
parse_expire_expr(const char *str, long long *num, uint8_t *expr)
{
	if(parse_number(str, num)) {
		*expr = EXPIRE_TIME_HAS_VALUE;
		return 1;
	}
	if(strcmp(str, REFRESHPLUSRETRYPLUS1_STR) == 0) {
		*num = 0;
		*expr = REFRESHPLUSRETRYPLUS1;
		return 1;
	}
	return 0;
}

static int
parse_number(const char *str, long long *num)
{
	/* ensure string consists entirely of digits */
	size_t pos = 0;
	while(str[pos] >= '0' && str[pos] <= '9') {
		pos++;
	}

	if(pos != 0 && str[pos] == '\0') {
		*num = strtoll(str, NULL, 10);
		return 1;
	}

	return 0;
}

static int
parse_range(const char *str, long long *low, long long *high)
{
	const char *ptr = str;
	long long num[2];

	/* require range to begin with a number */
	if(*ptr < '0' || *ptr > '9') {
		return 0;
	}

	num[0] = strtoll(ptr, (char **)&ptr, 10);

	/* require number to be followed by nothing at all or a dash */
	if(*ptr == '\0') {
		*low = num[0];
		*high = num[0];
		return 1;
	} else if(*ptr != '-') {
		return 0;
	}

	++ptr;
	/* require dash to be followed by a number */
	if(*ptr < '0' || *ptr > '9') {
		return 0;
	}

	num[1] = strtoll(ptr, (char **)&ptr, 10);

	/* require number to be followed by nothing at all */
	if(*ptr == '\0') {
		if(num[0] < num[1]) {
			*low = num[0];
			*high = num[1];
		} else {
			*low = num[1];
			*high = num[0];
		}
		return 1;
	}

	return 0;
}
