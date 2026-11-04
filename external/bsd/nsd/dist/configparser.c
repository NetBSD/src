/* A Bison parser, made by GNU Bison 3.8.2.  */

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
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

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
static int parse_catalog_role(const char *str, int *role);
static int parse_expire_expr(const char *str, long long *num, uint8_t *expr);
static int parse_number(const char *str, long long *num);
static int parse_range(const char *str, long long *low, long long *high);

struct component {
	struct component *next;
	char *str;
};


#line 116 "configparser.c"

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
  YYSYMBOL_VAR_TCP_LISTEN_QUEUE = 42,      /* VAR_TCP_LISTEN_QUEUE  */
  YYSYMBOL_VAR_IPV4_EDNS_SIZE = 43,        /* VAR_IPV4_EDNS_SIZE  */
  YYSYMBOL_VAR_IPV6_EDNS_SIZE = 44,        /* VAR_IPV6_EDNS_SIZE  */
  YYSYMBOL_VAR_STATISTICS = 45,            /* VAR_STATISTICS  */
  YYSYMBOL_VAR_XFRD_RELOAD_TIMEOUT = 46,   /* VAR_XFRD_RELOAD_TIMEOUT  */
  YYSYMBOL_VAR_LOG_TIME_ASCII = 47,        /* VAR_LOG_TIME_ASCII  */
  YYSYMBOL_VAR_LOG_TIME_ISO = 48,          /* VAR_LOG_TIME_ISO  */
  YYSYMBOL_VAR_ROUND_ROBIN = 49,           /* VAR_ROUND_ROBIN  */
  YYSYMBOL_VAR_MINIMAL_RESPONSES = 50,     /* VAR_MINIMAL_RESPONSES  */
  YYSYMBOL_VAR_CONFINE_TO_ZONE = 51,       /* VAR_CONFINE_TO_ZONE  */
  YYSYMBOL_VAR_REFUSE_ANY = 52,            /* VAR_REFUSE_ANY  */
  YYSYMBOL_VAR_RELOAD_CONFIG = 53,         /* VAR_RELOAD_CONFIG  */
  YYSYMBOL_VAR_ZONEFILES_CHECK = 54,       /* VAR_ZONEFILES_CHECK  */
  YYSYMBOL_VAR_ZONEFILES_WRITE = 55,       /* VAR_ZONEFILES_WRITE  */
  YYSYMBOL_VAR_RRL_SIZE = 56,              /* VAR_RRL_SIZE  */
  YYSYMBOL_VAR_RRL_RATELIMIT = 57,         /* VAR_RRL_RATELIMIT  */
  YYSYMBOL_VAR_RRL_SLIP = 58,              /* VAR_RRL_SLIP  */
  YYSYMBOL_VAR_RRL_IPV4_PREFIX_LENGTH = 59, /* VAR_RRL_IPV4_PREFIX_LENGTH  */
  YYSYMBOL_VAR_RRL_IPV6_PREFIX_LENGTH = 60, /* VAR_RRL_IPV6_PREFIX_LENGTH  */
  YYSYMBOL_VAR_RRL_WHITELIST_RATELIMIT = 61, /* VAR_RRL_WHITELIST_RATELIMIT  */
  YYSYMBOL_VAR_TLS_SERVICE_KEY = 62,       /* VAR_TLS_SERVICE_KEY  */
  YYSYMBOL_VAR_TLS_SERVICE_PEM = 63,       /* VAR_TLS_SERVICE_PEM  */
  YYSYMBOL_VAR_TLS_SERVICE_OCSP = 64,      /* VAR_TLS_SERVICE_OCSP  */
  YYSYMBOL_VAR_TLS_PORT = 65,              /* VAR_TLS_PORT  */
  YYSYMBOL_VAR_TLS_AUTH_PORT = 66,         /* VAR_TLS_AUTH_PORT  */
  YYSYMBOL_VAR_TLS_AUTH_XFR_ONLY = 67,     /* VAR_TLS_AUTH_XFR_ONLY  */
  YYSYMBOL_VAR_TLS_CERT_BUNDLE = 68,       /* VAR_TLS_CERT_BUNDLE  */
  YYSYMBOL_VAR_PROXY_PROTOCOL_PORT = 69,   /* VAR_PROXY_PROTOCOL_PORT  */
  YYSYMBOL_VAR_CPU_AFFINITY = 70,          /* VAR_CPU_AFFINITY  */
  YYSYMBOL_VAR_XFRD_CPU_AFFINITY = 71,     /* VAR_XFRD_CPU_AFFINITY  */
  YYSYMBOL_VAR_SERVER_CPU_AFFINITY = 72,   /* VAR_SERVER_CPU_AFFINITY  */
  YYSYMBOL_VAR_DROP_UPDATES = 73,          /* VAR_DROP_UPDATES  */
  YYSYMBOL_VAR_XFRD_TCP_MAX = 74,          /* VAR_XFRD_TCP_MAX  */
  YYSYMBOL_VAR_XFRD_TCP_PIPELINE = 75,     /* VAR_XFRD_TCP_PIPELINE  */
  YYSYMBOL_VAR_METRICS_ENABLE = 76,        /* VAR_METRICS_ENABLE  */
  YYSYMBOL_VAR_METRICS_INTERFACE = 77,     /* VAR_METRICS_INTERFACE  */
  YYSYMBOL_VAR_METRICS_PORT = 78,          /* VAR_METRICS_PORT  */
  YYSYMBOL_VAR_METRICS_PATH = 79,          /* VAR_METRICS_PATH  */
  YYSYMBOL_VAR_DNSTAP = 80,                /* VAR_DNSTAP  */
  YYSYMBOL_VAR_DNSTAP_ENABLE = 81,         /* VAR_DNSTAP_ENABLE  */
  YYSYMBOL_VAR_DNSTAP_SOCKET_PATH = 82,    /* VAR_DNSTAP_SOCKET_PATH  */
  YYSYMBOL_VAR_DNSTAP_IP = 83,             /* VAR_DNSTAP_IP  */
  YYSYMBOL_VAR_DNSTAP_TLS = 84,            /* VAR_DNSTAP_TLS  */
  YYSYMBOL_VAR_DNSTAP_TLS_SERVER_NAME = 85, /* VAR_DNSTAP_TLS_SERVER_NAME  */
  YYSYMBOL_VAR_DNSTAP_TLS_CERT_BUNDLE = 86, /* VAR_DNSTAP_TLS_CERT_BUNDLE  */
  YYSYMBOL_VAR_DNSTAP_TLS_CLIENT_KEY_FILE = 87, /* VAR_DNSTAP_TLS_CLIENT_KEY_FILE  */
  YYSYMBOL_VAR_DNSTAP_TLS_CLIENT_CERT_FILE = 88, /* VAR_DNSTAP_TLS_CLIENT_CERT_FILE  */
  YYSYMBOL_VAR_DNSTAP_SEND_IDENTITY = 89,  /* VAR_DNSTAP_SEND_IDENTITY  */
  YYSYMBOL_VAR_DNSTAP_SEND_VERSION = 90,   /* VAR_DNSTAP_SEND_VERSION  */
  YYSYMBOL_VAR_DNSTAP_IDENTITY = 91,       /* VAR_DNSTAP_IDENTITY  */
  YYSYMBOL_VAR_DNSTAP_VERSION = 92,        /* VAR_DNSTAP_VERSION  */
  YYSYMBOL_VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES = 93, /* VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES  */
  YYSYMBOL_VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES = 94, /* VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES  */
  YYSYMBOL_VAR_REMOTE_CONTROL = 95,        /* VAR_REMOTE_CONTROL  */
  YYSYMBOL_VAR_CONTROL_ENABLE = 96,        /* VAR_CONTROL_ENABLE  */
  YYSYMBOL_VAR_CONTROL_INTERFACE = 97,     /* VAR_CONTROL_INTERFACE  */
  YYSYMBOL_VAR_CONTROL_PORT = 98,          /* VAR_CONTROL_PORT  */
  YYSYMBOL_VAR_SERVER_KEY_FILE = 99,       /* VAR_SERVER_KEY_FILE  */
  YYSYMBOL_VAR_SERVER_CERT_FILE = 100,     /* VAR_SERVER_CERT_FILE  */
  YYSYMBOL_VAR_CONTROL_KEY_FILE = 101,     /* VAR_CONTROL_KEY_FILE  */
  YYSYMBOL_VAR_CONTROL_CERT_FILE = 102,    /* VAR_CONTROL_CERT_FILE  */
  YYSYMBOL_VAR_KEY = 103,                  /* VAR_KEY  */
  YYSYMBOL_VAR_ALGORITHM = 104,            /* VAR_ALGORITHM  */
  YYSYMBOL_VAR_SECRET = 105,               /* VAR_SECRET  */
  YYSYMBOL_VAR_TLS_AUTH = 106,             /* VAR_TLS_AUTH  */
  YYSYMBOL_VAR_TLS_AUTH_DOMAIN_NAME = 107, /* VAR_TLS_AUTH_DOMAIN_NAME  */
  YYSYMBOL_VAR_TLS_AUTH_CLIENT_CERT = 108, /* VAR_TLS_AUTH_CLIENT_CERT  */
  YYSYMBOL_VAR_TLS_AUTH_CLIENT_KEY = 109,  /* VAR_TLS_AUTH_CLIENT_KEY  */
  YYSYMBOL_VAR_TLS_AUTH_CLIENT_KEY_PW = 110, /* VAR_TLS_AUTH_CLIENT_KEY_PW  */
  YYSYMBOL_VAR_PATTERN = 111,              /* VAR_PATTERN  */
  YYSYMBOL_VAR_NAME = 112,                 /* VAR_NAME  */
  YYSYMBOL_VAR_ZONEFILE = 113,             /* VAR_ZONEFILE  */
  YYSYMBOL_VAR_NOTIFY = 114,               /* VAR_NOTIFY  */
  YYSYMBOL_VAR_PROVIDE_XFR = 115,          /* VAR_PROVIDE_XFR  */
  YYSYMBOL_VAR_ALLOW_QUERY = 116,          /* VAR_ALLOW_QUERY  */
  YYSYMBOL_VAR_AXFR = 117,                 /* VAR_AXFR  */
  YYSYMBOL_VAR_UDP = 118,                  /* VAR_UDP  */
  YYSYMBOL_VAR_NOTIFY_RETRY = 119,         /* VAR_NOTIFY_RETRY  */
  YYSYMBOL_VAR_ALLOW_NOTIFY = 120,         /* VAR_ALLOW_NOTIFY  */
  YYSYMBOL_VAR_REQUEST_XFR = 121,          /* VAR_REQUEST_XFR  */
  YYSYMBOL_VAR_ALLOW_AXFR_FALLBACK = 122,  /* VAR_ALLOW_AXFR_FALLBACK  */
  YYSYMBOL_VAR_OUTGOING_INTERFACE = 123,   /* VAR_OUTGOING_INTERFACE  */
  YYSYMBOL_VAR_ANSWER_COOKIE = 124,        /* VAR_ANSWER_COOKIE  */
  YYSYMBOL_VAR_COOKIE_SECRET = 125,        /* VAR_COOKIE_SECRET  */
  YYSYMBOL_VAR_COOKIE_SECRET_FILE = 126,   /* VAR_COOKIE_SECRET_FILE  */
  YYSYMBOL_VAR_COOKIE_STAGING_SECRET = 127, /* VAR_COOKIE_STAGING_SECRET  */
  YYSYMBOL_VAR_MAX_REFRESH_TIME = 128,     /* VAR_MAX_REFRESH_TIME  */
  YYSYMBOL_VAR_MIN_REFRESH_TIME = 129,     /* VAR_MIN_REFRESH_TIME  */
  YYSYMBOL_VAR_MAX_RETRY_TIME = 130,       /* VAR_MAX_RETRY_TIME  */
  YYSYMBOL_VAR_MIN_RETRY_TIME = 131,       /* VAR_MIN_RETRY_TIME  */
  YYSYMBOL_VAR_MIN_EXPIRE_TIME = 132,      /* VAR_MIN_EXPIRE_TIME  */
  YYSYMBOL_VAR_MULTI_PRIMARY_CHECK = 133,  /* VAR_MULTI_PRIMARY_CHECK  */
  YYSYMBOL_VAR_SIZE_LIMIT_XFR = 134,       /* VAR_SIZE_LIMIT_XFR  */
  YYSYMBOL_VAR_ZONESTATS = 135,            /* VAR_ZONESTATS  */
  YYSYMBOL_VAR_INCLUDE_PATTERN = 136,      /* VAR_INCLUDE_PATTERN  */
  YYSYMBOL_VAR_STORE_IXFR = 137,           /* VAR_STORE_IXFR  */
  YYSYMBOL_VAR_IXFR_SIZE = 138,            /* VAR_IXFR_SIZE  */
  YYSYMBOL_VAR_IXFR_NUMBER = 139,          /* VAR_IXFR_NUMBER  */
  YYSYMBOL_VAR_CREATE_IXFR = 140,          /* VAR_CREATE_IXFR  */
  YYSYMBOL_VAR_CATALOG = 141,              /* VAR_CATALOG  */
  YYSYMBOL_VAR_CATALOG_MEMBER_PATTERN = 142, /* VAR_CATALOG_MEMBER_PATTERN  */
  YYSYMBOL_VAR_CATALOG_PRODUCER_ZONE = 143, /* VAR_CATALOG_PRODUCER_ZONE  */
  YYSYMBOL_VAR_XDP_INTERFACE = 144,        /* VAR_XDP_INTERFACE  */
  YYSYMBOL_VAR_XDP_PROGRAM_PATH = 145,     /* VAR_XDP_PROGRAM_PATH  */
  YYSYMBOL_VAR_XDP_PROGRAM_LOAD = 146,     /* VAR_XDP_PROGRAM_LOAD  */
  YYSYMBOL_VAR_XDP_BPFFS_PATH = 147,       /* VAR_XDP_BPFFS_PATH  */
  YYSYMBOL_VAR_XDP_FORCE_COPY = 148,       /* VAR_XDP_FORCE_COPY  */
  YYSYMBOL_VAR_ZONE = 149,                 /* VAR_ZONE  */
  YYSYMBOL_VAR_RRL_WHITELIST = 150,        /* VAR_RRL_WHITELIST  */
  YYSYMBOL_VAR_SERVERS = 151,              /* VAR_SERVERS  */
  YYSYMBOL_VAR_BINDTODEVICE = 152,         /* VAR_BINDTODEVICE  */
  YYSYMBOL_VAR_SETFIB = 153,               /* VAR_SETFIB  */
  YYSYMBOL_VAR_VERIFY = 154,               /* VAR_VERIFY  */
  YYSYMBOL_VAR_ENABLE = 155,               /* VAR_ENABLE  */
  YYSYMBOL_VAR_VERIFY_ZONE = 156,          /* VAR_VERIFY_ZONE  */
  YYSYMBOL_VAR_VERIFY_ZONES = 157,         /* VAR_VERIFY_ZONES  */
  YYSYMBOL_VAR_VERIFIER = 158,             /* VAR_VERIFIER  */
  YYSYMBOL_VAR_VERIFIER_COUNT = 159,       /* VAR_VERIFIER_COUNT  */
  YYSYMBOL_VAR_VERIFIER_FEED_ZONE = 160,   /* VAR_VERIFIER_FEED_ZONE  */
  YYSYMBOL_VAR_VERIFIER_TIMEOUT = 161,     /* VAR_VERIFIER_TIMEOUT  */
  YYSYMBOL_YYACCEPT = 162,                 /* $accept  */
  YYSYMBOL_blocks = 163,                   /* blocks  */
  YYSYMBOL_block = 164,                    /* block  */
  YYSYMBOL_server = 165,                   /* server  */
  YYSYMBOL_server_block = 166,             /* server_block  */
  YYSYMBOL_server_option = 167,            /* server_option  */
  YYSYMBOL_168_1 = 168,                    /* $@1  */
  YYSYMBOL_socket_options = 169,           /* socket_options  */
  YYSYMBOL_socket_option = 170,            /* socket_option  */
  YYSYMBOL_cpus = 171,                     /* cpus  */
  YYSYMBOL_service_cpu_affinity = 172,     /* service_cpu_affinity  */
  YYSYMBOL_dnstap = 173,                   /* dnstap  */
  YYSYMBOL_dnstap_block = 174,             /* dnstap_block  */
  YYSYMBOL_dnstap_option = 175,            /* dnstap_option  */
  YYSYMBOL_remote_control = 176,           /* remote_control  */
  YYSYMBOL_remote_control_block = 177,     /* remote_control_block  */
  YYSYMBOL_remote_control_option = 178,    /* remote_control_option  */
  YYSYMBOL_tls_auth = 179,                 /* tls_auth  */
  YYSYMBOL_180_2 = 180,                    /* $@2  */
  YYSYMBOL_tls_auth_block = 181,           /* tls_auth_block  */
  YYSYMBOL_tls_auth_option = 182,          /* tls_auth_option  */
  YYSYMBOL_key = 183,                      /* key  */
  YYSYMBOL_184_3 = 184,                    /* $@3  */
  YYSYMBOL_key_block = 185,                /* key_block  */
  YYSYMBOL_key_option = 186,               /* key_option  */
  YYSYMBOL_zone = 187,                     /* zone  */
  YYSYMBOL_188_4 = 188,                    /* $@4  */
  YYSYMBOL_zone_block = 189,               /* zone_block  */
  YYSYMBOL_zone_option = 190,              /* zone_option  */
  YYSYMBOL_pattern = 191,                  /* pattern  */
  YYSYMBOL_192_5 = 192,                    /* $@5  */
  YYSYMBOL_pattern_block = 193,            /* pattern_block  */
  YYSYMBOL_pattern_option = 194,           /* pattern_option  */
  YYSYMBOL_pattern_or_zone_option = 195,   /* pattern_or_zone_option  */
  YYSYMBOL_196_6 = 196,                    /* $@6  */
  YYSYMBOL_197_7 = 197,                    /* $@7  */
  YYSYMBOL_198_8 = 198,                    /* $@8  */
  YYSYMBOL_verify = 199,                   /* verify  */
  YYSYMBOL_verify_block = 200,             /* verify_block  */
  YYSYMBOL_verify_option = 201,            /* verify_option  */
  YYSYMBOL_command = 202,                  /* command  */
  YYSYMBOL_arguments = 203,                /* arguments  */
  YYSYMBOL_ip_address = 204,               /* ip_address  */
  YYSYMBOL_number = 205,                   /* number  */
  YYSYMBOL_boolean = 206,                  /* boolean  */
  YYSYMBOL_request_xfr_tlsauth_option = 207, /* request_xfr_tlsauth_option  */
  YYSYMBOL_provide_xfr_tlsauth_option = 208, /* provide_xfr_tlsauth_option  */
  YYSYMBOL_catalog_role = 209              /* catalog_role  */
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

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
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
#define YYLAST   555

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  162
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  48
/* YYNRULES -- Number of rules.  */
#define YYNRULES  219
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  384

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   416


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
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   248,   248,   250,   253,   254,   255,   256,   257,   258,
     259,   260,   263,   266,   266,   270,   269,   286,   294,   296,
     298,   308,   318,   320,   322,   324,   326,   328,   330,   332,
     334,   336,   338,   340,   342,   372,   374,   376,   384,   386,
     388,   390,   392,   394,   397,   399,   401,   403,   410,   412,
     414,   416,   418,   420,   422,   424,   426,   428,   430,   432,
     442,   448,   454,   464,   474,   480,   482,   484,   486,   491,
     496,   501,   506,   508,   510,   512,   514,   516,   523,   530,
     538,   540,   548,   550,   561,   572,   584,   586,   588,   592,
     620,   626,   632,   638,   644,   650,   656,   668,   678,   686,
     687,   690,   717,   719,   724,   725,   759,   761,   772,   775,
     775,   778,   780,   782,   784,   786,   788,   790,   792,   794,
     796,   798,   800,   802,   804,   809,   812,   812,   815,   817,
     827,   835,   837,   839,   841,   847,   846,   868,   868,   871,
     882,   886,   890,   894,   902,   901,   926,   926,   929,   941,
     949,   968,   967,   992,   992,   995,  1008,  1012,  1011,  1028,
    1028,  1031,  1038,  1041,  1047,  1049,  1051,  1059,  1061,  1064,
    1063,  1077,  1076,  1088,  1098,  1103,  1113,  1112,  1119,  1124,
    1129,  1134,  1139,  1144,  1149,  1154,  1159,  1171,  1176,  1181,
    1186,  1191,  1193,  1195,  1197,  1199,  1206,  1210,  1226,  1229,
    1229,  1232,  1234,  1244,  1251,  1253,  1255,  1257,  1259,  1263,
    1283,  1284,  1302,  1312,  1321,  1329,  1330,  1334,  1335,  1340
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
  "VAR_OUTGOING_TCP_MSS", "VAR_TCP_LISTEN_QUEUE", "VAR_IPV4_EDNS_SIZE",
  "VAR_IPV6_EDNS_SIZE", "VAR_STATISTICS", "VAR_XFRD_RELOAD_TIMEOUT",
  "VAR_LOG_TIME_ASCII", "VAR_LOG_TIME_ISO", "VAR_ROUND_ROBIN",
  "VAR_MINIMAL_RESPONSES", "VAR_CONFINE_TO_ZONE", "VAR_REFUSE_ANY",
  "VAR_RELOAD_CONFIG", "VAR_ZONEFILES_CHECK", "VAR_ZONEFILES_WRITE",
  "VAR_RRL_SIZE", "VAR_RRL_RATELIMIT", "VAR_RRL_SLIP",
  "VAR_RRL_IPV4_PREFIX_LENGTH", "VAR_RRL_IPV6_PREFIX_LENGTH",
  "VAR_RRL_WHITELIST_RATELIMIT", "VAR_TLS_SERVICE_KEY",
  "VAR_TLS_SERVICE_PEM", "VAR_TLS_SERVICE_OCSP", "VAR_TLS_PORT",
  "VAR_TLS_AUTH_PORT", "VAR_TLS_AUTH_XFR_ONLY", "VAR_TLS_CERT_BUNDLE",
  "VAR_PROXY_PROTOCOL_PORT", "VAR_CPU_AFFINITY", "VAR_XFRD_CPU_AFFINITY",
  "VAR_SERVER_CPU_AFFINITY", "VAR_DROP_UPDATES", "VAR_XFRD_TCP_MAX",
  "VAR_XFRD_TCP_PIPELINE", "VAR_METRICS_ENABLE", "VAR_METRICS_INTERFACE",
  "VAR_METRICS_PORT", "VAR_METRICS_PATH", "VAR_DNSTAP",
  "VAR_DNSTAP_ENABLE", "VAR_DNSTAP_SOCKET_PATH", "VAR_DNSTAP_IP",
  "VAR_DNSTAP_TLS", "VAR_DNSTAP_TLS_SERVER_NAME",
  "VAR_DNSTAP_TLS_CERT_BUNDLE", "VAR_DNSTAP_TLS_CLIENT_KEY_FILE",
  "VAR_DNSTAP_TLS_CLIENT_CERT_FILE", "VAR_DNSTAP_SEND_IDENTITY",
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
  "VAR_COOKIE_SECRET_FILE", "VAR_COOKIE_STAGING_SECRET",
  "VAR_MAX_REFRESH_TIME", "VAR_MIN_REFRESH_TIME", "VAR_MAX_RETRY_TIME",
  "VAR_MIN_RETRY_TIME", "VAR_MIN_EXPIRE_TIME", "VAR_MULTI_PRIMARY_CHECK",
  "VAR_SIZE_LIMIT_XFR", "VAR_ZONESTATS", "VAR_INCLUDE_PATTERN",
  "VAR_STORE_IXFR", "VAR_IXFR_SIZE", "VAR_IXFR_NUMBER", "VAR_CREATE_IXFR",
  "VAR_CATALOG", "VAR_CATALOG_MEMBER_PATTERN", "VAR_CATALOG_PRODUCER_ZONE",
  "VAR_XDP_INTERFACE", "VAR_XDP_PROGRAM_PATH", "VAR_XDP_PROGRAM_LOAD",
  "VAR_XDP_BPFFS_PATH", "VAR_XDP_FORCE_COPY", "VAR_ZONE",
  "VAR_RRL_WHITELIST", "VAR_SERVERS", "VAR_BINDTODEVICE", "VAR_SETFIB",
  "VAR_VERIFY", "VAR_ENABLE", "VAR_VERIFY_ZONE", "VAR_VERIFY_ZONES",
  "VAR_VERIFIER", "VAR_VERIFIER_COUNT", "VAR_VERIFIER_FEED_ZONE",
  "VAR_VERIFIER_TIMEOUT", "$accept", "blocks", "block", "server",
  "server_block", "server_option", "$@1", "socket_options",
  "socket_option", "cpus", "service_cpu_affinity", "dnstap",
  "dnstap_block", "dnstap_option", "remote_control",
  "remote_control_block", "remote_control_option", "tls_auth", "$@2",
  "tls_auth_block", "tls_auth_option", "key", "$@3", "key_block",
  "key_option", "zone", "$@4", "zone_block", "zone_option", "pattern",
  "$@5", "pattern_block", "pattern_option", "pattern_or_zone_option",
  "$@6", "$@7", "$@8", "verify", "verify_block", "verify_option",
  "command", "arguments", "ip_address", "number", "boolean",
  "request_xfr_tlsauth_option", "provide_xfr_tlsauth_option",
  "catalog_role", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-205)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -205,   115,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
     407,   146,   -66,  -205,  -205,  -205,  -205,     3,    -1,    13,
      18,    18,    18,    -1,    -1,    18,    18,    18,    18,    18,
      -1,    18,    -1,    19,    22,    23,    24,    25,    34,    18,
      35,    49,    51,    52,    18,    18,    53,    54,    57,    -1,
      18,    -1,    -1,    -1,    -1,    58,    -1,    -1,    -1,    -1,
      18,    18,    18,    18,    18,    18,    18,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    60,    61,    -1,    -1,
      18,    63,    -1,  -205,  -205,  -205,    18,    -1,    -1,    18,
      13,    -1,    64,    18,    66,    67,    68,    70,    71,    18,
      74,    18,  -205,    -1,    18,    76,    78,    18,    82,    84,
      85,    86,    18,    18,    87,    90,    18,    18,  -205,    18,
      13,    -1,    91,    94,    96,    97,  -205,   -93,   -59,   228,
     379,    13,    -1,    18,    18,    99,    -1,    18,    -1,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,   104,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,   111,   124,   125,
    -205,   126,   129,   130,   132,   133,  -205,   134,   135,   138,
     139,   141,    -1,   142,     0,    18,   143,    -1,    -1,    -1,
      -1,   144,    18,    -1,   145,   147,    18,    -1,    -1,    18,
     148,   149,   150,   151,    18,    99,    18,    -1,  -205,  -205,
     153,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,   154,   156,   162,  -205,   163,   164,
     165,   166,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,   167,  -138,  -205,
    -205,  -205,  -205,  -205,   168,   169,  -205,   171,    18,    -1,
    -205,   172,   173,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,   173,  -205
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    14,   110,   127,   144,   135,   157,   151,
     200,     3,     4,     5,     6,     8,     7,    10,     9,    11,
      12,   108,   125,   147,   138,   160,   154,   198,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   104,   106,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    13,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   109,     0,
       0,     0,     0,     0,     0,     0,   126,   145,   136,   158,
     152,     0,     0,     0,     0,     0,     0,     0,     0,   199,
     213,    17,   212,    15,   214,    18,    19,    48,    20,    21,
      22,    27,    28,    29,    30,    47,    23,    58,    51,    50,
      52,    53,    31,    35,    36,    46,    54,    55,    56,    24,
      25,    33,    32,    34,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    49,    57,    68,    69,    70,    71,    72,
      73,    65,    66,    67,    59,    60,    61,    62,    63,    64,
      74,    76,    75,    77,    78,    79,    80,    81,    88,    26,
      86,    87,    95,    96,    97,    98,    82,    83,    85,    84,
      90,    91,    92,    93,    94,    89,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     128,   129,   130,   131,   132,   133,   134,     0,     0,     0,
     146,     0,     0,     0,     0,     0,   137,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   159,   162,
       0,   153,   156,   202,   203,   201,   204,   210,   205,   206,
     208,   207,    99,   105,   149,   150,   148,   140,   141,   142,
     143,   139,   161,   164,     0,     0,     0,   181,     0,     0,
       0,     0,   180,   179,   182,   183,   184,   185,   186,   167,
     166,   165,   168,   187,   188,   189,   190,   219,   195,   196,
     197,   163,   191,   192,   193,   194,   155,   209,    16,   175,
     176,   178,   174,   169,     0,     0,   211,     0,     0,     0,
     100,   217,   215,   171,   173,   101,   102,   103,   218,   177,
     216,   170,   215,   172
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,  -205,
    -205,  -205,  -205,     9,  -205,  -205,  -205,  -205,  -205,  -205,
    -118,  -205,   -83,    42,   -31,  -204,  -205,  -205
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    11,    12,    20,   112,   312,   358,   370,   218,
     113,    13,    21,   128,    14,    22,   136,    15,    24,   138,
     266,    16,    23,   137,   260,    17,    26,   140,   301,    18,
      25,   139,   298,   299,   372,   382,   371,    19,    27,   149,
     308,   357,   153,   151,   155,   381,   379,   348
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     156,   157,   150,   329,   160,   161,   162,   163,   164,   141,
     166,   257,   258,   367,   368,   369,   152,   223,   174,   259,
     142,   154,   168,   179,   180,   169,   170,   171,   172,   185,
     129,   130,   131,   132,   133,   134,   135,   173,   175,   195,
     196,   197,   198,   199,   200,   201,   202,   251,   261,   262,
     263,   264,   176,   265,   177,   178,   181,   182,   303,   215,
     183,   190,   210,   211,   212,   219,   216,   225,   222,   227,
     228,   229,   226,   230,   231,   158,   159,   233,   232,   237,
     234,   238,   165,   236,   167,   240,   239,   241,   242,   243,
     246,   244,   245,   247,   253,   248,   249,   254,   250,   255,
     256,   184,   307,   186,   187,   188,   189,   313,   191,   192,
     193,   194,   305,   306,   314,     2,   310,   330,   331,     3,
     203,   204,   205,   206,   207,   208,   209,   315,   316,   317,
     213,   214,   318,   319,   217,   320,   321,   322,   323,   220,
     221,   324,   325,   224,   326,   328,   333,   338,   341,   302,
     342,   347,   349,   350,   351,   235,   356,   359,   143,   360,
     144,   145,   146,   147,   148,   361,   362,   363,   364,   365,
     366,   373,   374,   252,   375,   378,   380,   353,   383,     0,
       0,     0,     0,     0,   304,     0,     0,     0,   309,     0,
     311,     0,     0,     0,     0,     4,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       5,     0,     0,     0,     0,     0,     0,     0,     6,     0,
       0,     7,     0,     0,     0,     0,     8,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,     0,     0,     0,   332,     0,     0,     0,     0,     0,
       0,   339,     0,     0,     0,   343,     0,     0,   346,     0,
       0,     0,     0,   352,     9,   354,     0,     0,     0,    10,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   327,     0,     0,     0,     0,   334,
     335,   336,   337,     0,     0,   340,     0,     0,     0,   344,
     345,     0,     0,     0,     0,     0,     0,   376,     0,   355,
     267,   268,   269,   270,   271,     0,     0,   272,   273,   274,
     275,   276,     0,     0,     0,     0,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,     0,     0,     0,     0,     0,     0,   293,     0,
       0,     0,     0,     0,   294,     0,   295,     0,   296,   297,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   377,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,     0,     0,     0,
       0,   300,   268,   269,   270,   271,     0,     0,   272,   273,
     274,   275,   276,     0,     0,     0,     0,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,     0,     0,     0,     0,     0,     0,   293,
       0,   103,   104,   105,   106,   294,     0,   295,     0,   296,
     297,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,   108,   109,   110,   111
};

static const yytype_int16 yycheck[] =
{
      31,    32,     3,     3,    35,    36,    37,    38,    39,     6,
      41,   104,   105,   151,   152,   153,     3,   100,    49,   112,
      17,     3,     3,    54,    55,     3,     3,     3,     3,    60,
      96,    97,    98,    99,   100,   101,   102,     3,     3,    70,
      71,    72,    73,    74,    75,    76,    77,   130,   107,   108,
     109,   110,     3,   112,     3,     3,     3,     3,   141,    90,
       3,     3,     3,     3,     3,    96,     3,     3,    99,     3,
       3,     3,   103,     3,     3,    33,    34,     3,   109,     3,
     111,     3,    40,   114,    42,     3,   117,     3,     3,     3,
       3,   122,   123,     3,     3,   126,   127,     3,   129,     3,
       3,    59,     3,    61,    62,    63,    64,     3,    66,    67,
      68,    69,   143,   144,     3,     0,   147,   117,   118,     4,
      78,    79,    80,    81,    82,    83,    84,     3,     3,     3,
      88,    89,     3,     3,    92,     3,     3,     3,     3,    97,
      98,     3,     3,   101,     3,     3,     3,     3,     3,   140,
       3,     3,     3,     3,     3,   113,     3,     3,   155,     3,
     157,   158,   159,   160,   161,     3,     3,     3,     3,     3,
       3,     3,     3,   131,     3,     3,     3,   295,   382,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,   146,    -1,
     148,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,    -1,
      -1,   106,    -1,    -1,    -1,    -1,   111,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    -1,    -1,    -1,   275,    -1,    -1,    -1,    -1,    -1,
      -1,   282,    -1,    -1,    -1,   286,    -1,    -1,   289,    -1,
      -1,    -1,    -1,   294,   149,   296,    -1,    -1,    -1,   154,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   272,    -1,    -1,    -1,    -1,   277,
     278,   279,   280,    -1,    -1,   283,    -1,    -1,    -1,   287,
     288,    -1,    -1,    -1,    -1,    -1,    -1,   368,    -1,   297,
     112,   113,   114,   115,   116,    -1,    -1,   119,   120,   121,
     122,   123,    -1,    -1,    -1,    -1,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,    -1,    -1,    -1,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,   156,    -1,   158,    -1,   160,   161,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   369,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    -1,    -1,    -1,
      -1,   112,   113,   114,   115,   116,    -1,    -1,   119,   120,
     121,   122,   123,    -1,    -1,    -1,    -1,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,    -1,    -1,    -1,    -1,    -1,    -1,   150,
      -1,   124,   125,   126,   127,   156,    -1,   158,    -1,   160,
     161,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   144,   145,   146,   147,   148
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   163,     0,     4,    80,    95,   103,   106,   111,   149,
     154,   164,   165,   173,   176,   179,   183,   187,   191,   199,
     166,   174,   177,   184,   180,   192,   188,   200,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,   124,   125,   126,   127,   144,   145,   146,
     147,   148,   167,   172,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,   175,    96,
      97,    98,    99,   100,   101,   102,   178,   185,   181,   193,
     189,     6,    17,   155,   157,   158,   159,   160,   161,   201,
       3,   205,     3,   204,     3,   206,   206,   206,   205,   205,
     206,   206,   206,   206,   206,   205,   206,   205,     3,     3,
       3,     3,     3,     3,   206,     3,     3,     3,     3,   206,
     206,     3,     3,     3,   205,   206,   205,   205,   205,   205,
       3,   205,   205,   205,   205,   206,   206,   206,   206,   206,
     206,   206,   206,   205,   205,   205,   205,   205,   205,   205,
       3,     3,     3,   205,   205,   206,     3,   205,   171,   206,
     205,   205,   206,   204,   205,     3,   206,     3,     3,     3,
       3,     3,   206,     3,   206,   205,   206,     3,     3,   206,
       3,     3,     3,     3,   206,   206,     3,     3,   206,   206,
     206,   204,   205,     3,     3,     3,     3,   104,   105,   112,
     186,   107,   108,   109,   110,   112,   182,   112,   113,   114,
     115,   116,   119,   120,   121,   122,   123,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   150,   156,   158,   160,   161,   194,   195,
     112,   190,   195,   204,   205,   206,   206,     3,   202,   205,
     206,   205,   168,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,   205,     3,     3,
     117,   118,   206,     3,   205,   205,   205,   205,     3,   206,
     205,     3,     3,   206,   205,   205,   206,     3,   209,     3,
       3,     3,   206,   202,   206,   205,     3,   203,   169,     3,
       3,     3,     3,     3,     3,     3,     3,   151,   152,   153,
     170,   198,   196,     3,     3,     3,   206,   205,     3,   208,
       3,   207,   197,   207
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   162,   163,   163,   164,   164,   164,   164,   164,   164,
     164,   164,   165,   166,   166,   168,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   169,
     169,   170,   170,   170,   171,   171,   172,   172,   173,   174,
     174,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   176,   177,   177,   178,   178,
     178,   178,   178,   178,   178,   180,   179,   181,   181,   182,
     182,   182,   182,   182,   184,   183,   185,   185,   186,   186,
     186,   188,   187,   189,   189,   190,   190,   192,   191,   193,
     193,   194,   194,   195,   195,   195,   195,   195,   195,   196,
     195,   197,   195,   195,   195,   195,   198,   195,   195,   195,
     195,   195,   195,   195,   195,   195,   195,   195,   195,   195,
     195,   195,   195,   195,   195,   195,   195,   195,   199,   200,
     200,   201,   201,   201,   201,   201,   201,   201,   201,   202,
     203,   203,   204,   205,   206,   207,   207,   208,   208,   209
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     0,
       2,     2,     2,     2,     0,     2,     1,     1,     2,     2,
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     0,     2,     2,
       2,     2,     2,     2,     2,     0,     3,     2,     0,     2,
       2,     2,     2,     2,     0,     3,     2,     0,     2,     2,
       2,     0,     3,     2,     0,     2,     1,     0,     3,     2,
       0,     2,     1,     2,     2,     2,     2,     2,     2,     0,
       5,     0,     6,     4,     3,     3,     0,     5,     3,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       0,     2,     1,     1,     1,     0,     1,     0,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


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
    YYNOMEM;
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
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
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
#line 270 "configparser.y"
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
#line 1682 "configparser.c"
    break;

  case 16: /* server_option: VAR_IP_ADDRESS ip_address $@1 socket_options  */
#line 283 "configparser.y"
    {
      cfg_parser->ip = NULL;
    }
#line 1690 "configparser.c"
    break;

  case 17: /* server_option: VAR_SERVER_COUNT number  */
#line 287 "configparser.y"
    {
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->server_count = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 1702 "configparser.c"
    break;

  case 18: /* server_option: VAR_IP_TRANSPARENT boolean  */
#line 295 "configparser.y"
    { cfg_parser->opt->ip_transparent = (yyvsp[0].bln); }
#line 1708 "configparser.c"
    break;

  case 19: /* server_option: VAR_IP_FREEBIND boolean  */
#line 297 "configparser.y"
    { cfg_parser->opt->ip_freebind = (yyvsp[0].bln); }
#line 1714 "configparser.c"
    break;

  case 20: /* server_option: VAR_SEND_BUFFER_SIZE number  */
#line 299 "configparser.y"
    {
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->send_buffer_size = (int)(yyvsp[0].llng);
      } else if ((yyvsp[0].llng) == 0) {
        /* do nothing and use the default value */
      } else {
        yyerror("expected a number equal to or greater than zero");
      }
    }
#line 1728 "configparser.c"
    break;

  case 21: /* server_option: VAR_RECEIVE_BUFFER_SIZE number  */
#line 309 "configparser.y"
    {
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->receive_buffer_size = (int)(yyvsp[0].llng);
      } else if ((yyvsp[0].llng) == 0) {
        /* do nothing and use the default value */
      } else {
        yyerror("expected a number equal to or greater than zero");
      }
    }
#line 1742 "configparser.c"
    break;

  case 22: /* server_option: VAR_DEBUG_MODE boolean  */
#line 319 "configparser.y"
    { cfg_parser->opt->debug_mode = (yyvsp[0].bln); }
#line 1748 "configparser.c"
    break;

  case 23: /* server_option: VAR_USE_SYSTEMD boolean  */
#line 321 "configparser.y"
    { /* ignored, obsolete */ }
#line 1754 "configparser.c"
    break;

  case 24: /* server_option: VAR_HIDE_VERSION boolean  */
#line 323 "configparser.y"
    { cfg_parser->opt->hide_version = (yyvsp[0].bln); }
#line 1760 "configparser.c"
    break;

  case 25: /* server_option: VAR_HIDE_IDENTITY boolean  */
#line 325 "configparser.y"
    { cfg_parser->opt->hide_identity = (yyvsp[0].bln); }
#line 1766 "configparser.c"
    break;

  case 26: /* server_option: VAR_DROP_UPDATES boolean  */
#line 327 "configparser.y"
    { cfg_parser->opt->drop_updates = (yyvsp[0].bln); }
#line 1772 "configparser.c"
    break;

  case 27: /* server_option: VAR_IP4_ONLY boolean  */
#line 329 "configparser.y"
    { if((yyvsp[0].bln)) { cfg_parser->opt->do_ip4 = 1; cfg_parser->opt->do_ip6 = 0; } }
#line 1778 "configparser.c"
    break;

  case 28: /* server_option: VAR_IP6_ONLY boolean  */
#line 331 "configparser.y"
    { if((yyvsp[0].bln)) { cfg_parser->opt->do_ip4 = 0; cfg_parser->opt->do_ip6 = 1; } }
#line 1784 "configparser.c"
    break;

  case 29: /* server_option: VAR_DO_IP4 boolean  */
#line 333 "configparser.y"
    { cfg_parser->opt->do_ip4 = (yyvsp[0].bln); }
#line 1790 "configparser.c"
    break;

  case 30: /* server_option: VAR_DO_IP6 boolean  */
#line 335 "configparser.y"
    { cfg_parser->opt->do_ip6 = (yyvsp[0].bln); }
#line 1796 "configparser.c"
    break;

  case 31: /* server_option: VAR_DATABASE STRING  */
#line 337 "configparser.y"
    { /* ignored, obsolete */ }
#line 1802 "configparser.c"
    break;

  case 32: /* server_option: VAR_IDENTITY STRING  */
#line 339 "configparser.y"
    { cfg_parser->opt->identity = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1808 "configparser.c"
    break;

  case 33: /* server_option: VAR_VERSION STRING  */
#line 341 "configparser.y"
    { cfg_parser->opt->version = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1814 "configparser.c"
    break;

  case 34: /* server_option: VAR_NSID STRING  */
#line 343 "configparser.y"
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
#line 1848 "configparser.c"
    break;

  case 35: /* server_option: VAR_LOGFILE STRING  */
#line 373 "configparser.y"
    { cfg_parser->opt->logfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1854 "configparser.c"
    break;

  case 36: /* server_option: VAR_LOG_ONLY_SYSLOG boolean  */
#line 375 "configparser.y"
    { cfg_parser->opt->log_only_syslog = (yyvsp[0].bln); }
#line 1860 "configparser.c"
    break;

  case 37: /* server_option: VAR_TCP_COUNT number  */
#line 377 "configparser.y"
    {
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->tcp_count = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 1872 "configparser.c"
    break;

  case 38: /* server_option: VAR_TCP_REJECT_OVERFLOW boolean  */
#line 385 "configparser.y"
    { cfg_parser->opt->tcp_reject_overflow = (yyvsp[0].bln); }
#line 1878 "configparser.c"
    break;

  case 39: /* server_option: VAR_TCP_QUERY_COUNT number  */
#line 387 "configparser.y"
    { cfg_parser->opt->tcp_query_count = (int)(yyvsp[0].llng); }
#line 1884 "configparser.c"
    break;

  case 40: /* server_option: VAR_TCP_TIMEOUT number  */
#line 389 "configparser.y"
    { cfg_parser->opt->tcp_timeout = (int)(yyvsp[0].llng); }
#line 1890 "configparser.c"
    break;

  case 41: /* server_option: VAR_TCP_MSS number  */
#line 391 "configparser.y"
    { cfg_parser->opt->tcp_mss = (int)(yyvsp[0].llng); }
#line 1896 "configparser.c"
    break;

  case 42: /* server_option: VAR_OUTGOING_TCP_MSS number  */
#line 393 "configparser.y"
    { cfg_parser->opt->outgoing_tcp_mss = (int)(yyvsp[0].llng); }
#line 1902 "configparser.c"
    break;

  case 43: /* server_option: VAR_TCP_LISTEN_QUEUE STRING  */
#line 395 "configparser.y"
    { /* With atoi is it allowed to be negative, for system chosen result. */
      cfg_parser->opt->tcp_listen_queue = atoi((yyvsp[0].str)); }
#line 1909 "configparser.c"
    break;

  case 44: /* server_option: VAR_IPV4_EDNS_SIZE number  */
#line 398 "configparser.y"
    { cfg_parser->opt->ipv4_edns_size = (size_t)(yyvsp[0].llng); }
#line 1915 "configparser.c"
    break;

  case 45: /* server_option: VAR_IPV6_EDNS_SIZE number  */
#line 400 "configparser.y"
    { cfg_parser->opt->ipv6_edns_size = (size_t)(yyvsp[0].llng); }
#line 1921 "configparser.c"
    break;

  case 46: /* server_option: VAR_PIDFILE STRING  */
#line 402 "configparser.y"
    { cfg_parser->opt->pidfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1927 "configparser.c"
    break;

  case 47: /* server_option: VAR_PORT number  */
#line 404 "configparser.y"
    {
      /* port number, stored as a string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 1938 "configparser.c"
    break;

  case 48: /* server_option: VAR_REUSEPORT boolean  */
#line 411 "configparser.y"
    { cfg_parser->opt->reuseport = (yyvsp[0].bln); }
#line 1944 "configparser.c"
    break;

  case 49: /* server_option: VAR_STATISTICS number  */
#line 413 "configparser.y"
    { cfg_parser->opt->statistics = (int)(yyvsp[0].llng); }
#line 1950 "configparser.c"
    break;

  case 50: /* server_option: VAR_CHROOT STRING  */
#line 415 "configparser.y"
    { cfg_parser->opt->chroot = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1956 "configparser.c"
    break;

  case 51: /* server_option: VAR_USERNAME STRING  */
#line 417 "configparser.y"
    { cfg_parser->opt->username = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1962 "configparser.c"
    break;

  case 52: /* server_option: VAR_ZONESDIR STRING  */
#line 419 "configparser.y"
    { cfg_parser->opt->zonesdir = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1968 "configparser.c"
    break;

  case 53: /* server_option: VAR_ZONELISTFILE STRING  */
#line 421 "configparser.y"
    { cfg_parser->opt->zonelistfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1974 "configparser.c"
    break;

  case 54: /* server_option: VAR_DIFFFILE STRING  */
#line 423 "configparser.y"
    { /* ignored, obsolete */ }
#line 1980 "configparser.c"
    break;

  case 55: /* server_option: VAR_XFRDFILE STRING  */
#line 425 "configparser.y"
    { cfg_parser->opt->xfrdfile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1986 "configparser.c"
    break;

  case 56: /* server_option: VAR_XFRDIR STRING  */
#line 427 "configparser.y"
    { cfg_parser->opt->xfrdir = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 1992 "configparser.c"
    break;

  case 57: /* server_option: VAR_XFRD_RELOAD_TIMEOUT number  */
#line 429 "configparser.y"
    { cfg_parser->opt->xfrd_reload_timeout = (int)(yyvsp[0].llng); }
#line 1998 "configparser.c"
    break;

  case 58: /* server_option: VAR_VERBOSITY number  */
#line 431 "configparser.y"
    { cfg_parser->opt->verbosity = (int)(yyvsp[0].llng); }
#line 2004 "configparser.c"
    break;

  case 59: /* server_option: VAR_RRL_SIZE number  */
#line 433 "configparser.y"
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 0) {
        cfg_parser->opt->rrl_size = (size_t)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
#endif
    }
#line 2018 "configparser.c"
    break;

  case 60: /* server_option: VAR_RRL_RATELIMIT number  */
#line 443 "configparser.y"
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_ratelimit = (size_t)(yyvsp[0].llng);
#endif
    }
#line 2028 "configparser.c"
    break;

  case 61: /* server_option: VAR_RRL_SLIP number  */
#line 449 "configparser.y"
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_slip = (size_t)(yyvsp[0].llng);
#endif
    }
#line 2038 "configparser.c"
    break;

  case 62: /* server_option: VAR_RRL_IPV4_PREFIX_LENGTH number  */
#line 455 "configparser.y"
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 32) {
        yyerror("invalid IPv4 prefix length");
      } else {
        cfg_parser->opt->rrl_ipv4_prefix_length = (size_t)(yyvsp[0].llng);
      }
#endif
    }
#line 2052 "configparser.c"
    break;

  case 63: /* server_option: VAR_RRL_IPV6_PREFIX_LENGTH number  */
#line 465 "configparser.y"
    {
#ifdef RATELIMIT
      if ((yyvsp[0].llng) > 64) {
        yyerror("invalid IPv6 prefix length");
      } else {
        cfg_parser->opt->rrl_ipv6_prefix_length = (size_t)(yyvsp[0].llng);
      }
#endif
    }
#line 2066 "configparser.c"
    break;

  case 64: /* server_option: VAR_RRL_WHITELIST_RATELIMIT number  */
#line 475 "configparser.y"
    {
#ifdef RATELIMIT
      cfg_parser->opt->rrl_whitelist_ratelimit = (size_t)(yyvsp[0].llng);
#endif
    }
#line 2076 "configparser.c"
    break;

  case 65: /* server_option: VAR_RELOAD_CONFIG boolean  */
#line 481 "configparser.y"
    { cfg_parser->opt->reload_config = (yyvsp[0].bln); }
#line 2082 "configparser.c"
    break;

  case 66: /* server_option: VAR_ZONEFILES_CHECK boolean  */
#line 483 "configparser.y"
    { cfg_parser->opt->zonefiles_check = (yyvsp[0].bln); }
#line 2088 "configparser.c"
    break;

  case 67: /* server_option: VAR_ZONEFILES_WRITE number  */
#line 485 "configparser.y"
    { cfg_parser->opt->zonefiles_write = (int)(yyvsp[0].llng); }
#line 2094 "configparser.c"
    break;

  case 68: /* server_option: VAR_LOG_TIME_ASCII boolean  */
#line 487 "configparser.y"
    {
      cfg_parser->opt->log_time_ascii = (yyvsp[0].bln);
      log_time_asc = cfg_parser->opt->log_time_ascii;
    }
#line 2103 "configparser.c"
    break;

  case 69: /* server_option: VAR_LOG_TIME_ISO boolean  */
#line 492 "configparser.y"
    {
      cfg_parser->opt->log_time_iso = (yyvsp[0].bln);
      log_time_iso = cfg_parser->opt->log_time_iso;
    }
#line 2112 "configparser.c"
    break;

  case 70: /* server_option: VAR_ROUND_ROBIN boolean  */
#line 497 "configparser.y"
    {
      cfg_parser->opt->round_robin = (yyvsp[0].bln);
      round_robin = cfg_parser->opt->round_robin;
    }
#line 2121 "configparser.c"
    break;

  case 71: /* server_option: VAR_MINIMAL_RESPONSES boolean  */
#line 502 "configparser.y"
    {
      cfg_parser->opt->minimal_responses = (yyvsp[0].bln);
      minimal_responses = cfg_parser->opt->minimal_responses;
    }
#line 2130 "configparser.c"
    break;

  case 72: /* server_option: VAR_CONFINE_TO_ZONE boolean  */
#line 507 "configparser.y"
    { cfg_parser->opt->confine_to_zone = (yyvsp[0].bln); }
#line 2136 "configparser.c"
    break;

  case 73: /* server_option: VAR_REFUSE_ANY boolean  */
#line 509 "configparser.y"
    { cfg_parser->opt->refuse_any = (yyvsp[0].bln); }
#line 2142 "configparser.c"
    break;

  case 74: /* server_option: VAR_TLS_SERVICE_KEY STRING  */
#line 511 "configparser.y"
    { cfg_parser->opt->tls_service_key = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2148 "configparser.c"
    break;

  case 75: /* server_option: VAR_TLS_SERVICE_OCSP STRING  */
#line 513 "configparser.y"
    { cfg_parser->opt->tls_service_ocsp = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2154 "configparser.c"
    break;

  case 76: /* server_option: VAR_TLS_SERVICE_PEM STRING  */
#line 515 "configparser.y"
    { cfg_parser->opt->tls_service_pem = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2160 "configparser.c"
    break;

  case 77: /* server_option: VAR_TLS_PORT number  */
#line 517 "configparser.y"
    {
      /* port number, stored as string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->tls_port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 2171 "configparser.c"
    break;

  case 78: /* server_option: VAR_TLS_AUTH_PORT number  */
#line 524 "configparser.y"
    {
      /* port number, stored as string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->tls_auth_port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 2182 "configparser.c"
    break;

  case 79: /* server_option: VAR_TLS_AUTH_XFR_ONLY boolean  */
#line 531 "configparser.y"
    {
      if (!cfg_parser->opt->tls_auth_port) {
        yyerror("tls-auth-xfr-only set without or before tls-auth-port");
        YYABORT;
      }
      cfg_parser->opt->tls_auth_xfr_only = (yyvsp[0].bln);
    }
#line 2194 "configparser.c"
    break;

  case 80: /* server_option: VAR_TLS_CERT_BUNDLE STRING  */
#line 539 "configparser.y"
    { cfg_parser->opt->tls_cert_bundle = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2200 "configparser.c"
    break;

  case 81: /* server_option: VAR_PROXY_PROTOCOL_PORT number  */
#line 541 "configparser.y"
    {
      struct proxy_protocol_port_list* elem = region_alloc_zero(
	cfg_parser->opt->region, sizeof(*elem));
      elem->port = (yyvsp[0].llng);
      elem->next = cfg_parser->opt->proxy_protocol_port;
      cfg_parser->opt->proxy_protocol_port = elem;
    }
#line 2212 "configparser.c"
    break;

  case 82: /* server_option: VAR_ANSWER_COOKIE boolean  */
#line 549 "configparser.y"
    { cfg_parser->opt->answer_cookie = (yyvsp[0].bln); }
#line 2218 "configparser.c"
    break;

  case 83: /* server_option: VAR_COOKIE_SECRET STRING  */
#line 551 "configparser.y"
    {
      uint8_t secret[32];
      ssize_t len = hex_pton((yyvsp[0].str), secret, NSD_COOKIE_SECRET_SIZE);

      if(len != NSD_COOKIE_SECRET_SIZE) {
        yyerror("expected a 128 bit hex string");
      } else {
        cfg_parser->opt->cookie_secret = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      }
    }
#line 2233 "configparser.c"
    break;

  case 84: /* server_option: VAR_COOKIE_STAGING_SECRET STRING  */
#line 562 "configparser.y"
    {
      uint8_t secret[32];
      ssize_t len = hex_pton((yyvsp[0].str), secret, NSD_COOKIE_SECRET_SIZE);

      if(len != NSD_COOKIE_SECRET_SIZE) {
        yyerror("expected a 128 bit hex string");
      } else {
        cfg_parser->opt->cookie_staging_secret = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      }
    }
#line 2248 "configparser.c"
    break;

  case 85: /* server_option: VAR_COOKIE_SECRET_FILE STRING  */
#line 573 "configparser.y"
    {
      /* Empty filename means explicitly disabled cookies from file, internally
       * represented as NULL.
       * Note that after parsing, if no value was configured, then
       * cookie_secret_file_is_default is still 1, then the default cookie
       * secret file value will be assigned to cookie_secret_file.
       */
      if(*(yyvsp[0].str)) cfg_parser->opt->cookie_secret_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      cfg_parser->opt->cookie_secret_file_is_default = 0;
    }
#line 2263 "configparser.c"
    break;

  case 86: /* server_option: VAR_XFRD_TCP_MAX number  */
#line 585 "configparser.y"
    { cfg_parser->opt->xfrd_tcp_max = (int)(yyvsp[0].llng); }
#line 2269 "configparser.c"
    break;

  case 87: /* server_option: VAR_XFRD_TCP_PIPELINE number  */
#line 587 "configparser.y"
    { cfg_parser->opt->xfrd_tcp_pipeline = (int)(yyvsp[0].llng); }
#line 2275 "configparser.c"
    break;

  case 88: /* server_option: VAR_CPU_AFFINITY cpus  */
#line 589 "configparser.y"
    {
      cfg_parser->opt->cpu_affinity = (yyvsp[0].cpu);
    }
#line 2283 "configparser.c"
    break;

  case 89: /* server_option: service_cpu_affinity number  */
#line 593 "configparser.y"
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
#line 2315 "configparser.c"
    break;

  case 90: /* server_option: VAR_XDP_INTERFACE STRING  */
#line 621 "configparser.y"
    {
#ifdef USE_XDP
      cfg_parser->opt->xdp_interface = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
#endif
    }
#line 2325 "configparser.c"
    break;

  case 91: /* server_option: VAR_XDP_PROGRAM_PATH STRING  */
#line 627 "configparser.y"
    {
#ifdef USE_XDP
      cfg_parser->opt->xdp_program_path = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
#endif
    }
#line 2335 "configparser.c"
    break;

  case 92: /* server_option: VAR_XDP_PROGRAM_LOAD boolean  */
#line 633 "configparser.y"
    {
#ifdef USE_XDP
      cfg_parser->opt->xdp_program_load = (yyvsp[0].bln);
#endif
    }
#line 2345 "configparser.c"
    break;

  case 93: /* server_option: VAR_XDP_BPFFS_PATH STRING  */
#line 639 "configparser.y"
    {
#ifdef USE_XDP
      cfg_parser->opt->xdp_bpffs_path = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
#endif
	}
#line 2355 "configparser.c"
    break;

  case 94: /* server_option: VAR_XDP_FORCE_COPY boolean  */
#line 645 "configparser.y"
    {
#ifdef USE_XDP
      cfg_parser->opt->xdp_force_copy = (yyvsp[0].bln);
#endif
    }
#line 2365 "configparser.c"
    break;

  case 95: /* server_option: VAR_METRICS_ENABLE boolean  */
#line 651 "configparser.y"
    {
#ifdef USE_METRICS
      cfg_parser->opt->metrics_enable = (yyvsp[0].bln);
#endif /* USE_METRICS */
    }
#line 2375 "configparser.c"
    break;

  case 96: /* server_option: VAR_METRICS_INTERFACE ip_address  */
#line 657 "configparser.y"
    {
#ifdef USE_METRICS
      struct ip_address_option *ip = cfg_parser->opt->metrics_interface;
      if(ip == NULL) {
        cfg_parser->opt->metrics_interface = (yyvsp[0].ip);
      } else {
        while(ip->next != NULL) { ip = ip->next; }
        ip->next = (yyvsp[0].ip);
      }
#endif /* USE_METRICS */
    }
#line 2391 "configparser.c"
    break;

  case 97: /* server_option: VAR_METRICS_PORT number  */
#line 669 "configparser.y"
    {
#ifdef USE_METRICS
      if((yyvsp[0].llng) == 0) {
        yyerror("metrics port number expected");
      } else {
        cfg_parser->opt->metrics_port = (int)(yyvsp[0].llng);
      }
#endif /* USE_METRICS */
    }
#line 2405 "configparser.c"
    break;

  case 98: /* server_option: VAR_METRICS_PATH STRING  */
#line 679 "configparser.y"
    {
#ifdef USE_METRICS
      cfg_parser->opt->metrics_path = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
#endif /* USE_METRICS */
    }
#line 2415 "configparser.c"
    break;

  case 101: /* socket_option: VAR_SERVERS STRING  */
#line 691 "configparser.y"
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
#line 2446 "configparser.c"
    break;

  case 102: /* socket_option: VAR_BINDTODEVICE boolean  */
#line 718 "configparser.y"
    { cfg_parser->ip->dev = (yyvsp[0].bln); }
#line 2452 "configparser.c"
    break;

  case 103: /* socket_option: VAR_SETFIB number  */
#line 720 "configparser.y"
    { cfg_parser->ip->fib = (yyvsp[0].llng); }
#line 2458 "configparser.c"
    break;

  case 104: /* cpus: %empty  */
#line 724 "configparser.y"
    { (yyval.cpu) = NULL; }
#line 2464 "configparser.c"
    break;

  case 105: /* cpus: cpus STRING  */
#line 726 "configparser.y"
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
#line 2499 "configparser.c"
    break;

  case 106: /* service_cpu_affinity: VAR_XFRD_CPU_AFFINITY  */
#line 760 "configparser.y"
    { (yyval.llng) = -1; }
#line 2505 "configparser.c"
    break;

  case 107: /* service_cpu_affinity: VAR_SERVER_CPU_AFFINITY  */
#line 762 "configparser.y"
    {
      if((yyvsp[0].llng) <= 0) {
        yyerror("invalid server identifier");
        YYABORT;
      }
      (yyval.llng) = (yyvsp[0].llng);
    }
#line 2517 "configparser.c"
    break;

  case 111: /* dnstap_option: VAR_DNSTAP_ENABLE boolean  */
#line 779 "configparser.y"
    { cfg_parser->opt->dnstap_enable = (yyvsp[0].bln); }
#line 2523 "configparser.c"
    break;

  case 112: /* dnstap_option: VAR_DNSTAP_SOCKET_PATH STRING  */
#line 781 "configparser.y"
    { cfg_parser->opt->dnstap_socket_path = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2529 "configparser.c"
    break;

  case 113: /* dnstap_option: VAR_DNSTAP_IP STRING  */
#line 783 "configparser.y"
    { cfg_parser->opt->dnstap_ip = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2535 "configparser.c"
    break;

  case 114: /* dnstap_option: VAR_DNSTAP_TLS boolean  */
#line 785 "configparser.y"
    { cfg_parser->opt->dnstap_tls = (yyvsp[0].bln); }
#line 2541 "configparser.c"
    break;

  case 115: /* dnstap_option: VAR_DNSTAP_TLS_SERVER_NAME STRING  */
#line 787 "configparser.y"
    { cfg_parser->opt->dnstap_tls_server_name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2547 "configparser.c"
    break;

  case 116: /* dnstap_option: VAR_DNSTAP_TLS_CERT_BUNDLE STRING  */
#line 789 "configparser.y"
    { cfg_parser->opt->dnstap_tls_cert_bundle = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2553 "configparser.c"
    break;

  case 117: /* dnstap_option: VAR_DNSTAP_TLS_CLIENT_KEY_FILE STRING  */
#line 791 "configparser.y"
    { cfg_parser->opt->dnstap_tls_client_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2559 "configparser.c"
    break;

  case 118: /* dnstap_option: VAR_DNSTAP_TLS_CLIENT_CERT_FILE STRING  */
#line 793 "configparser.y"
    { cfg_parser->opt->dnstap_tls_client_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2565 "configparser.c"
    break;

  case 119: /* dnstap_option: VAR_DNSTAP_SEND_IDENTITY boolean  */
#line 795 "configparser.y"
    { cfg_parser->opt->dnstap_send_identity = (yyvsp[0].bln); }
#line 2571 "configparser.c"
    break;

  case 120: /* dnstap_option: VAR_DNSTAP_SEND_VERSION boolean  */
#line 797 "configparser.y"
    { cfg_parser->opt->dnstap_send_version = (yyvsp[0].bln); }
#line 2577 "configparser.c"
    break;

  case 121: /* dnstap_option: VAR_DNSTAP_IDENTITY STRING  */
#line 799 "configparser.y"
    { cfg_parser->opt->dnstap_identity = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2583 "configparser.c"
    break;

  case 122: /* dnstap_option: VAR_DNSTAP_VERSION STRING  */
#line 801 "configparser.y"
    { cfg_parser->opt->dnstap_version = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2589 "configparser.c"
    break;

  case 123: /* dnstap_option: VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES boolean  */
#line 803 "configparser.y"
    { cfg_parser->opt->dnstap_log_auth_query_messages = (yyvsp[0].bln); }
#line 2595 "configparser.c"
    break;

  case 124: /* dnstap_option: VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES boolean  */
#line 805 "configparser.y"
    { cfg_parser->opt->dnstap_log_auth_response_messages = (yyvsp[0].bln); }
#line 2601 "configparser.c"
    break;

  case 128: /* remote_control_option: VAR_CONTROL_ENABLE boolean  */
#line 816 "configparser.y"
    { cfg_parser->opt->control_enable = (yyvsp[0].bln); }
#line 2607 "configparser.c"
    break;

  case 129: /* remote_control_option: VAR_CONTROL_INTERFACE ip_address  */
#line 818 "configparser.y"
    {
      struct ip_address_option *ip = cfg_parser->opt->control_interface;
      if(ip == NULL) {
        cfg_parser->opt->control_interface = (yyvsp[0].ip);
      } else {
        while(ip->next != NULL) { ip = ip->next; }
        ip->next = (yyvsp[0].ip);
      }
    }
#line 2621 "configparser.c"
    break;

  case 130: /* remote_control_option: VAR_CONTROL_PORT number  */
#line 828 "configparser.y"
    {
      if((yyvsp[0].llng) == 0) {
        yyerror("control port number expected");
      } else {
        cfg_parser->opt->control_port = (int)(yyvsp[0].llng);
      }
    }
#line 2633 "configparser.c"
    break;

  case 131: /* remote_control_option: VAR_SERVER_KEY_FILE STRING  */
#line 836 "configparser.y"
    { cfg_parser->opt->server_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2639 "configparser.c"
    break;

  case 132: /* remote_control_option: VAR_SERVER_CERT_FILE STRING  */
#line 838 "configparser.y"
    { cfg_parser->opt->server_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2645 "configparser.c"
    break;

  case 133: /* remote_control_option: VAR_CONTROL_KEY_FILE STRING  */
#line 840 "configparser.y"
    { cfg_parser->opt->control_key_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2651 "configparser.c"
    break;

  case 134: /* remote_control_option: VAR_CONTROL_CERT_FILE STRING  */
#line 842 "configparser.y"
    { cfg_parser->opt->control_cert_file = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2657 "configparser.c"
    break;

  case 135: /* $@2: %empty  */
#line 847 "configparser.y"
      {
        tls_auth_options_type *tls_auth = tls_auth_options_create(cfg_parser->opt->region);
        assert(cfg_parser->tls_auth == NULL);
        cfg_parser->tls_auth = tls_auth;
      }
#line 2667 "configparser.c"
    break;

  case 136: /* tls_auth: VAR_TLS_AUTH $@2 tls_auth_block  */
#line 853 "configparser.y"
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
#line 2685 "configparser.c"
    break;

  case 139: /* tls_auth_option: VAR_NAME STRING  */
#line 872 "configparser.y"
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
#line 2700 "configparser.c"
    break;

  case 140: /* tls_auth_option: VAR_TLS_AUTH_DOMAIN_NAME STRING  */
#line 883 "configparser.y"
    {
      cfg_parser->tls_auth->auth_domain_name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2708 "configparser.c"
    break;

  case 141: /* tls_auth_option: VAR_TLS_AUTH_CLIENT_CERT STRING  */
#line 887 "configparser.y"
    {
	    cfg_parser->tls_auth->client_cert = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2716 "configparser.c"
    break;

  case 142: /* tls_auth_option: VAR_TLS_AUTH_CLIENT_KEY STRING  */
#line 891 "configparser.y"
    {
	    cfg_parser->tls_auth->client_key = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2724 "configparser.c"
    break;

  case 143: /* tls_auth_option: VAR_TLS_AUTH_CLIENT_KEY_PW STRING  */
#line 895 "configparser.y"
    {
	    cfg_parser->tls_auth->client_key_pw = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2732 "configparser.c"
    break;

  case 144: /* $@3: %empty  */
#line 902 "configparser.y"
      {
        key_options_type *key = key_options_create(cfg_parser->opt->region);
        key->algorithm = region_strdup(cfg_parser->opt->region, "sha256");
        assert(cfg_parser->key == NULL);
        cfg_parser->key = key;
      }
#line 2743 "configparser.c"
    break;

  case 145: /* key: VAR_KEY $@3 key_block  */
#line 909 "configparser.y"
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
#line 2763 "configparser.c"
    break;

  case 148: /* key_option: VAR_NAME STRING  */
#line 930 "configparser.y"
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
#line 2779 "configparser.c"
    break;

  case 149: /* key_option: VAR_ALGORITHM STRING  */
#line 942 "configparser.y"
    {
      if(tsig_get_algorithm_by_name((yyvsp[0].str)) == NULL) {
        yyerror("bad tsig key algorithm %s", (yyvsp[0].str));
      } else {
        cfg_parser->key->algorithm = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      }
    }
#line 2791 "configparser.c"
    break;

  case 150: /* key_option: VAR_SECRET STRING  */
#line 950 "configparser.y"
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
#line 2810 "configparser.c"
    break;

  case 151: /* $@4: %empty  */
#line 968 "configparser.y"
      {
        assert(cfg_parser->pattern == NULL);
        assert(cfg_parser->zone == NULL);
        cfg_parser->zone = zone_options_create(cfg_parser->opt->region);
        cfg_parser->zone->part_of_config = 1;
        cfg_parser->zone->pattern = cfg_parser->pattern =
          pattern_options_create(cfg_parser->opt->region);
        cfg_parser->zone->pattern->implicit = 1;
      }
#line 2824 "configparser.c"
    break;

  case 152: /* zone: VAR_ZONE $@4 zone_block  */
#line 978 "configparser.y"
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
#line 2841 "configparser.c"
    break;

  case 155: /* zone_option: VAR_NAME STRING  */
#line 996 "configparser.y"
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
#line 2858 "configparser.c"
    break;

  case 157: /* $@5: %empty  */
#line 1012 "configparser.y"
      {
        assert(cfg_parser->pattern == NULL);
        cfg_parser->pattern = pattern_options_create(cfg_parser->opt->region);
      }
#line 2867 "configparser.c"
    break;

  case 158: /* pattern: VAR_PATTERN $@5 pattern_block  */
#line 1017 "configparser.y"
    {
      pattern_options_type *pattern = cfg_parser->pattern;
      if(pattern->pname == NULL) {
        yyerror("pattern has no name");
      } else if(!nsd_options_insert_pattern(cfg_parser->opt, pattern)) {
        yyerror("duplicate pattern %s", pattern->pname);
      }
      cfg_parser->pattern = NULL;
    }
#line 2881 "configparser.c"
    break;

  case 161: /* pattern_option: VAR_NAME STRING  */
#line 1032 "configparser.y"
    {
      if(strchr((yyvsp[0].str), ' ')) {
        yyerror("space is not allowed in pattern name: '%s'", (yyvsp[0].str));
      }
      cfg_parser->pattern->pname = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
    }
#line 2892 "configparser.c"
    break;

  case 163: /* pattern_or_zone_option: VAR_RRL_WHITELIST STRING  */
#line 1042 "configparser.y"
    {
#ifdef RATELIMIT
      cfg_parser->pattern->rrl_whitelist |= rrlstr2type((yyvsp[0].str));
#endif
    }
#line 2902 "configparser.c"
    break;

  case 164: /* pattern_or_zone_option: VAR_ZONEFILE STRING  */
#line 1048 "configparser.y"
    { cfg_parser->pattern->zonefile = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2908 "configparser.c"
    break;

  case 165: /* pattern_or_zone_option: VAR_ZONESTATS STRING  */
#line 1050 "configparser.y"
    { cfg_parser->pattern->zonestats = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); }
#line 2914 "configparser.c"
    break;

  case 166: /* pattern_or_zone_option: VAR_SIZE_LIMIT_XFR number  */
#line 1052 "configparser.y"
    {
      if((yyvsp[0].llng) > 0) {
        cfg_parser->pattern->size_limit_xfr = (int)(yyvsp[0].llng);
      } else {
        yyerror("expected a number greater than zero");
      }
    }
#line 2926 "configparser.c"
    break;

  case 167: /* pattern_or_zone_option: VAR_MULTI_PRIMARY_CHECK boolean  */
#line 1060 "configparser.y"
    { cfg_parser->pattern->multi_primary_check = (int)(yyvsp[0].bln); }
#line 2932 "configparser.c"
    break;

  case 168: /* pattern_or_zone_option: VAR_INCLUDE_PATTERN STRING  */
#line 1062 "configparser.y"
    { config_apply_pattern(cfg_parser->pattern, (yyvsp[0].str)); }
#line 2938 "configparser.c"
    break;

  case 169: /* $@6: %empty  */
#line 1064 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      if(cfg_parser->pattern->catalog_role == CATALOG_ROLE_PRODUCER)
        yyerror("catalog producer zones cannot be secondary zones");
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2953 "configparser.c"
    break;

  case 170: /* pattern_or_zone_option: VAR_REQUEST_XFR STRING STRING $@6 request_xfr_tlsauth_option  */
#line 1075 "configparser.y"
        { }
#line 2959 "configparser.c"
    break;

  case 171: /* $@7: %empty  */
#line 1077 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      acl->use_axfr_only = 1;
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2973 "configparser.c"
    break;

  case 172: /* pattern_or_zone_option: VAR_REQUEST_XFR VAR_AXFR STRING STRING $@7 request_xfr_tlsauth_option  */
#line 1087 "configparser.y"
        { }
#line 2979 "configparser.c"
    break;

  case 173: /* pattern_or_zone_option: VAR_REQUEST_XFR VAR_UDP STRING STRING  */
#line 1089 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      acl->allow_udp = 1;
      if(acl->blocked)
        yyerror("blocked address used for request-xfr");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for request-xfr");
      append_acl(&cfg_parser->pattern->request_xfr, acl);
    }
#line 2993 "configparser.c"
    break;

  case 174: /* pattern_or_zone_option: VAR_ALLOW_NOTIFY STRING STRING  */
#line 1099 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      append_acl(&cfg_parser->pattern->allow_notify, acl);
    }
#line 3002 "configparser.c"
    break;

  case 175: /* pattern_or_zone_option: VAR_NOTIFY STRING STRING  */
#line 1104 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      if(acl->blocked)
        yyerror("blocked address used for notify");
      if(acl->rangetype != acl_range_single)
        yyerror("address range used for notify");
      append_acl(&cfg_parser->pattern->notify, acl);
    }
#line 3015 "configparser.c"
    break;

  case 176: /* $@8: %empty  */
#line 1113 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      append_acl(&cfg_parser->pattern->provide_xfr, acl);
    }
#line 3024 "configparser.c"
    break;

  case 177: /* pattern_or_zone_option: VAR_PROVIDE_XFR STRING STRING $@8 provide_xfr_tlsauth_option  */
#line 1118 "configparser.y"
        { }
#line 3030 "configparser.c"
    break;

  case 178: /* pattern_or_zone_option: VAR_ALLOW_QUERY STRING STRING  */
#line 1120 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[-1].str), (yyvsp[0].str));
      append_acl(&cfg_parser->pattern->allow_query, acl);
    }
#line 3039 "configparser.c"
    break;

  case 179: /* pattern_or_zone_option: VAR_OUTGOING_INTERFACE STRING  */
#line 1125 "configparser.y"
    {
      acl_options_type *acl = parse_acl_info(cfg_parser->opt->region, (yyvsp[0].str), "NOKEY");
      append_acl(&cfg_parser->pattern->outgoing_interface, acl);
    }
#line 3048 "configparser.c"
    break;

  case 180: /* pattern_or_zone_option: VAR_ALLOW_AXFR_FALLBACK boolean  */
#line 1130 "configparser.y"
    {
      cfg_parser->pattern->allow_axfr_fallback = (yyvsp[0].bln);
      cfg_parser->pattern->allow_axfr_fallback_is_default = 0;
    }
#line 3057 "configparser.c"
    break;

  case 181: /* pattern_or_zone_option: VAR_NOTIFY_RETRY number  */
#line 1135 "configparser.y"
    {
      cfg_parser->pattern->notify_retry = (yyvsp[0].llng);
      cfg_parser->pattern->notify_retry_is_default = 0;
    }
#line 3066 "configparser.c"
    break;

  case 182: /* pattern_or_zone_option: VAR_MAX_REFRESH_TIME number  */
#line 1140 "configparser.y"
    {
      cfg_parser->pattern->max_refresh_time = (yyvsp[0].llng);
      cfg_parser->pattern->max_refresh_time_is_default = 0;
    }
#line 3075 "configparser.c"
    break;

  case 183: /* pattern_or_zone_option: VAR_MIN_REFRESH_TIME number  */
#line 1145 "configparser.y"
    {
      cfg_parser->pattern->min_refresh_time = (yyvsp[0].llng);
      cfg_parser->pattern->min_refresh_time_is_default = 0;
    }
#line 3084 "configparser.c"
    break;

  case 184: /* pattern_or_zone_option: VAR_MAX_RETRY_TIME number  */
#line 1150 "configparser.y"
    {
      cfg_parser->pattern->max_retry_time = (yyvsp[0].llng);
      cfg_parser->pattern->max_retry_time_is_default = 0;
    }
#line 3093 "configparser.c"
    break;

  case 185: /* pattern_or_zone_option: VAR_MIN_RETRY_TIME number  */
#line 1155 "configparser.y"
    {
      cfg_parser->pattern->min_retry_time = (yyvsp[0].llng);
      cfg_parser->pattern->min_retry_time_is_default = 0;
    }
#line 3102 "configparser.c"
    break;

  case 186: /* pattern_or_zone_option: VAR_MIN_EXPIRE_TIME STRING  */
#line 1160 "configparser.y"
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
#line 3118 "configparser.c"
    break;

  case 187: /* pattern_or_zone_option: VAR_STORE_IXFR boolean  */
#line 1172 "configparser.y"
    {
      cfg_parser->pattern->store_ixfr = (yyvsp[0].bln);
      cfg_parser->pattern->store_ixfr_is_default = 0;
    }
#line 3127 "configparser.c"
    break;

  case 188: /* pattern_or_zone_option: VAR_IXFR_SIZE number  */
#line 1177 "configparser.y"
    {
      cfg_parser->pattern->ixfr_size = (yyvsp[0].llng);
      cfg_parser->pattern->ixfr_size_is_default = 0;
    }
#line 3136 "configparser.c"
    break;

  case 189: /* pattern_or_zone_option: VAR_IXFR_NUMBER number  */
#line 1182 "configparser.y"
    {
      cfg_parser->pattern->ixfr_number = (yyvsp[0].llng);
      cfg_parser->pattern->ixfr_number_is_default = 0;
    }
#line 3145 "configparser.c"
    break;

  case 190: /* pattern_or_zone_option: VAR_CREATE_IXFR boolean  */
#line 1187 "configparser.y"
    {
      cfg_parser->pattern->create_ixfr = (yyvsp[0].bln);
      cfg_parser->pattern->create_ixfr_is_default = 0;
    }
#line 3154 "configparser.c"
    break;

  case 191: /* pattern_or_zone_option: VAR_VERIFY_ZONE boolean  */
#line 1192 "configparser.y"
    { cfg_parser->pattern->verify_zone = (yyvsp[0].bln); }
#line 3160 "configparser.c"
    break;

  case 192: /* pattern_or_zone_option: VAR_VERIFIER command  */
#line 1194 "configparser.y"
    { cfg_parser->pattern->verifier = (yyvsp[0].strv); }
#line 3166 "configparser.c"
    break;

  case 193: /* pattern_or_zone_option: VAR_VERIFIER_FEED_ZONE boolean  */
#line 1196 "configparser.y"
    { cfg_parser->pattern->verifier_feed_zone = (yyvsp[0].bln); }
#line 3172 "configparser.c"
    break;

  case 194: /* pattern_or_zone_option: VAR_VERIFIER_TIMEOUT number  */
#line 1198 "configparser.y"
    { cfg_parser->pattern->verifier_timeout = (yyvsp[0].llng); }
#line 3178 "configparser.c"
    break;

  case 195: /* pattern_or_zone_option: VAR_CATALOG catalog_role  */
#line 1200 "configparser.y"
    {
      if((yyvsp[0].role) == CATALOG_ROLE_PRODUCER && cfg_parser->pattern->request_xfr)
        yyerror("catalog producer zones cannot be secondary zones");
      cfg_parser->pattern->catalog_role = (yyvsp[0].role);
      cfg_parser->pattern->catalog_role_is_default = 0;
    }
#line 3189 "configparser.c"
    break;

  case 196: /* pattern_or_zone_option: VAR_CATALOG_MEMBER_PATTERN STRING  */
#line 1207 "configparser.y"
    { 
      cfg_parser->pattern->catalog_member_pattern = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); 
    }
#line 3197 "configparser.c"
    break;

  case 197: /* pattern_or_zone_option: VAR_CATALOG_PRODUCER_ZONE STRING  */
#line 1211 "configparser.y"
    {
      dname_type *dname;

      if(cfg_parser->zone) {
        yyerror("catalog-producer-zone option is for patterns only and cannot "
                "be used in a zone clause");
      } else if(!(dname = (dname_type *)dname_parse(cfg_parser->opt->region, (yyvsp[0].str)))) {
        yyerror("bad catalog producer name %s", (yyvsp[0].str));
      } else {
        region_recycle(cfg_parser->opt->region, dname, dname_total_size(dname));
        cfg_parser->pattern->catalog_producer_zone = region_strdup(cfg_parser->opt->region, (yyvsp[0].str)); 
      }
    }
#line 3215 "configparser.c"
    break;

  case 201: /* verify_option: VAR_ENABLE boolean  */
#line 1233 "configparser.y"
    { cfg_parser->opt->verify_enable = (yyvsp[0].bln); }
#line 3221 "configparser.c"
    break;

  case 202: /* verify_option: VAR_IP_ADDRESS ip_address  */
#line 1235 "configparser.y"
    {
      struct ip_address_option *ip = cfg_parser->opt->verify_ip_addresses;
      if(!ip) {
        cfg_parser->opt->verify_ip_addresses = (yyvsp[0].ip);
      } else {
        while(ip->next) { ip = ip->next; }
        ip->next = (yyvsp[0].ip);
      }
    }
#line 3235 "configparser.c"
    break;

  case 203: /* verify_option: VAR_PORT number  */
#line 1245 "configparser.y"
    {
      /* port number, stored as a string */
      char buf[16];
      (void)snprintf(buf, sizeof(buf), "%lld", (yyvsp[0].llng));
      cfg_parser->opt->verify_port = region_strdup(cfg_parser->opt->region, buf);
    }
#line 3246 "configparser.c"
    break;

  case 204: /* verify_option: VAR_VERIFY_ZONES boolean  */
#line 1252 "configparser.y"
    { cfg_parser->opt->verify_zones = (yyvsp[0].bln); }
#line 3252 "configparser.c"
    break;

  case 205: /* verify_option: VAR_VERIFIER command  */
#line 1254 "configparser.y"
    { cfg_parser->opt->verifier = (yyvsp[0].strv); }
#line 3258 "configparser.c"
    break;

  case 206: /* verify_option: VAR_VERIFIER_COUNT number  */
#line 1256 "configparser.y"
    { cfg_parser->opt->verifier_count = (int)(yyvsp[0].llng); }
#line 3264 "configparser.c"
    break;

  case 207: /* verify_option: VAR_VERIFIER_TIMEOUT number  */
#line 1258 "configparser.y"
    { cfg_parser->opt->verifier_timeout = (int)(yyvsp[0].llng); }
#line 3270 "configparser.c"
    break;

  case 208: /* verify_option: VAR_VERIFIER_FEED_ZONE boolean  */
#line 1260 "configparser.y"
    { cfg_parser->opt->verifier_feed_zone = (yyvsp[0].bln); }
#line 3276 "configparser.c"
    break;

  case 209: /* command: STRING arguments  */
#line 1264 "configparser.y"
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
#line 3298 "configparser.c"
    break;

  case 210: /* arguments: %empty  */
#line 1283 "configparser.y"
    { (yyval.comp) = NULL; }
#line 3304 "configparser.c"
    break;

  case 211: /* arguments: arguments STRING  */
#line 1285 "configparser.y"
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
#line 3324 "configparser.c"
    break;

  case 212: /* ip_address: STRING  */
#line 1303 "configparser.y"
    {
      struct ip_address_option *ip = region_alloc_zero(
        cfg_parser->opt->region, sizeof(*ip));
      ip->address = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
      ip->fib = -1;
      (yyval.ip) = ip;
    }
#line 3336 "configparser.c"
    break;

  case 213: /* number: STRING  */
#line 1313 "configparser.y"
    {
      if(!parse_number((yyvsp[0].str), &(yyval.llng))) {
        yyerror("expected a number");
        YYABORT; /* trigger a parser error */
      }
    }
#line 3347 "configparser.c"
    break;

  case 214: /* boolean: STRING  */
#line 1322 "configparser.y"
    {
      if(!parse_boolean((yyvsp[0].str), &(yyval.bln))) {
        yyerror("expected yes or no");
        YYABORT; /* trigger a parser error */
      }
    }
#line 3358 "configparser.c"
    break;

  case 216: /* request_xfr_tlsauth_option: STRING  */
#line 1331 "configparser.y"
        { char *tls_auth_name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	  add_to_last_acl(&cfg_parser->pattern->request_xfr, tls_auth_name);}
#line 3365 "configparser.c"
    break;

  case 218: /* provide_xfr_tlsauth_option: STRING  */
#line 1336 "configparser.y"
        { char *tls_auth_name = region_strdup(cfg_parser->opt->region, (yyvsp[0].str));
	  add_to_last_acl(&cfg_parser->pattern->provide_xfr, tls_auth_name);}
#line 3372 "configparser.c"
    break;

  case 219: /* catalog_role: STRING  */
#line 1341 "configparser.y"
    {
      if(!parse_catalog_role((yyvsp[0].str), &(yyval.role))) {
        yyerror("expected consumer or producer");
        YYABORT; /* trigger a parser error */
      }
    }
#line 3383 "configparser.c"
    break;


#line 3387 "configparser.c"

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
  ++yynerrs;

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
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
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

#line 1348 "configparser.y"


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

static int
parse_catalog_role(const char *str, int *role)
{
	if(strcasecmp(str, "consumer") == 0) {
		*role = CATALOG_ROLE_CONSUMER;
	} else if(strcmp(str, "producer") == 0) {
		*role = CATALOG_ROLE_PRODUCER;
	} else {
		return 0;
	}
	return 1;
}


