/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_C_CONFIGPARSER_H_INCLUDED
# define YY_C_CONFIGPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int c_debug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    STRING = 258,                  /* STRING  */
    VAR_SERVER = 259,              /* VAR_SERVER  */
    VAR_SERVER_COUNT = 260,        /* VAR_SERVER_COUNT  */
    VAR_IP_ADDRESS = 261,          /* VAR_IP_ADDRESS  */
    VAR_IP_TRANSPARENT = 262,      /* VAR_IP_TRANSPARENT  */
    VAR_IP_FREEBIND = 263,         /* VAR_IP_FREEBIND  */
    VAR_REUSEPORT = 264,           /* VAR_REUSEPORT  */
    VAR_SEND_BUFFER_SIZE = 265,    /* VAR_SEND_BUFFER_SIZE  */
    VAR_RECEIVE_BUFFER_SIZE = 266, /* VAR_RECEIVE_BUFFER_SIZE  */
    VAR_DEBUG_MODE = 267,          /* VAR_DEBUG_MODE  */
    VAR_IP4_ONLY = 268,            /* VAR_IP4_ONLY  */
    VAR_IP6_ONLY = 269,            /* VAR_IP6_ONLY  */
    VAR_DO_IP4 = 270,              /* VAR_DO_IP4  */
    VAR_DO_IP6 = 271,              /* VAR_DO_IP6  */
    VAR_PORT = 272,                /* VAR_PORT  */
    VAR_USE_SYSTEMD = 273,         /* VAR_USE_SYSTEMD  */
    VAR_VERBOSITY = 274,           /* VAR_VERBOSITY  */
    VAR_USERNAME = 275,            /* VAR_USERNAME  */
    VAR_CHROOT = 276,              /* VAR_CHROOT  */
    VAR_ZONESDIR = 277,            /* VAR_ZONESDIR  */
    VAR_ZONELISTFILE = 278,        /* VAR_ZONELISTFILE  */
    VAR_DATABASE = 279,            /* VAR_DATABASE  */
    VAR_LOGFILE = 280,             /* VAR_LOGFILE  */
    VAR_LOG_ONLY_SYSLOG = 281,     /* VAR_LOG_ONLY_SYSLOG  */
    VAR_PIDFILE = 282,             /* VAR_PIDFILE  */
    VAR_DIFFFILE = 283,            /* VAR_DIFFFILE  */
    VAR_XFRDFILE = 284,            /* VAR_XFRDFILE  */
    VAR_XFRDIR = 285,              /* VAR_XFRDIR  */
    VAR_HIDE_VERSION = 286,        /* VAR_HIDE_VERSION  */
    VAR_HIDE_IDENTITY = 287,       /* VAR_HIDE_IDENTITY  */
    VAR_VERSION = 288,             /* VAR_VERSION  */
    VAR_IDENTITY = 289,            /* VAR_IDENTITY  */
    VAR_NSID = 290,                /* VAR_NSID  */
    VAR_TCP_COUNT = 291,           /* VAR_TCP_COUNT  */
    VAR_TCP_REJECT_OVERFLOW = 292, /* VAR_TCP_REJECT_OVERFLOW  */
    VAR_TCP_QUERY_COUNT = 293,     /* VAR_TCP_QUERY_COUNT  */
    VAR_TCP_TIMEOUT = 294,         /* VAR_TCP_TIMEOUT  */
    VAR_TCP_MSS = 295,             /* VAR_TCP_MSS  */
    VAR_OUTGOING_TCP_MSS = 296,    /* VAR_OUTGOING_TCP_MSS  */
    VAR_TCP_LISTEN_QUEUE = 297,    /* VAR_TCP_LISTEN_QUEUE  */
    VAR_IPV4_EDNS_SIZE = 298,      /* VAR_IPV4_EDNS_SIZE  */
    VAR_IPV6_EDNS_SIZE = 299,      /* VAR_IPV6_EDNS_SIZE  */
    VAR_STATISTICS = 300,          /* VAR_STATISTICS  */
    VAR_XFRD_RELOAD_TIMEOUT = 301, /* VAR_XFRD_RELOAD_TIMEOUT  */
    VAR_LOG_TIME_ASCII = 302,      /* VAR_LOG_TIME_ASCII  */
    VAR_LOG_TIME_ISO = 303,        /* VAR_LOG_TIME_ISO  */
    VAR_ROUND_ROBIN = 304,         /* VAR_ROUND_ROBIN  */
    VAR_MINIMAL_RESPONSES = 305,   /* VAR_MINIMAL_RESPONSES  */
    VAR_CONFINE_TO_ZONE = 306,     /* VAR_CONFINE_TO_ZONE  */
    VAR_REFUSE_ANY = 307,          /* VAR_REFUSE_ANY  */
    VAR_RELOAD_CONFIG = 308,       /* VAR_RELOAD_CONFIG  */
    VAR_ZONEFILES_CHECK = 309,     /* VAR_ZONEFILES_CHECK  */
    VAR_ZONEFILES_WRITE = 310,     /* VAR_ZONEFILES_WRITE  */
    VAR_RRL_SIZE = 311,            /* VAR_RRL_SIZE  */
    VAR_RRL_RATELIMIT = 312,       /* VAR_RRL_RATELIMIT  */
    VAR_RRL_SLIP = 313,            /* VAR_RRL_SLIP  */
    VAR_RRL_IPV4_PREFIX_LENGTH = 314, /* VAR_RRL_IPV4_PREFIX_LENGTH  */
    VAR_RRL_IPV6_PREFIX_LENGTH = 315, /* VAR_RRL_IPV6_PREFIX_LENGTH  */
    VAR_RRL_WHITELIST_RATELIMIT = 316, /* VAR_RRL_WHITELIST_RATELIMIT  */
    VAR_TLS_SERVICE_KEY = 317,     /* VAR_TLS_SERVICE_KEY  */
    VAR_TLS_SERVICE_PEM = 318,     /* VAR_TLS_SERVICE_PEM  */
    VAR_TLS_SERVICE_OCSP = 319,    /* VAR_TLS_SERVICE_OCSP  */
    VAR_TLS_PORT = 320,            /* VAR_TLS_PORT  */
    VAR_TLS_AUTH_PORT = 321,       /* VAR_TLS_AUTH_PORT  */
    VAR_TLS_AUTH_XFR_ONLY = 322,   /* VAR_TLS_AUTH_XFR_ONLY  */
    VAR_TLS_CERT_BUNDLE = 323,     /* VAR_TLS_CERT_BUNDLE  */
    VAR_PROXY_PROTOCOL_PORT = 324, /* VAR_PROXY_PROTOCOL_PORT  */
    VAR_CPU_AFFINITY = 325,        /* VAR_CPU_AFFINITY  */
    VAR_XFRD_CPU_AFFINITY = 326,   /* VAR_XFRD_CPU_AFFINITY  */
    VAR_SERVER_CPU_AFFINITY = 327, /* VAR_SERVER_CPU_AFFINITY  */
    VAR_DROP_UPDATES = 328,        /* VAR_DROP_UPDATES  */
    VAR_XFRD_TCP_MAX = 329,        /* VAR_XFRD_TCP_MAX  */
    VAR_XFRD_TCP_PIPELINE = 330,   /* VAR_XFRD_TCP_PIPELINE  */
    VAR_METRICS_ENABLE = 331,      /* VAR_METRICS_ENABLE  */
    VAR_METRICS_INTERFACE = 332,   /* VAR_METRICS_INTERFACE  */
    VAR_METRICS_PORT = 333,        /* VAR_METRICS_PORT  */
    VAR_METRICS_PATH = 334,        /* VAR_METRICS_PATH  */
    VAR_DNSTAP = 335,              /* VAR_DNSTAP  */
    VAR_DNSTAP_ENABLE = 336,       /* VAR_DNSTAP_ENABLE  */
    VAR_DNSTAP_SOCKET_PATH = 337,  /* VAR_DNSTAP_SOCKET_PATH  */
    VAR_DNSTAP_IP = 338,           /* VAR_DNSTAP_IP  */
    VAR_DNSTAP_TLS = 339,          /* VAR_DNSTAP_TLS  */
    VAR_DNSTAP_TLS_SERVER_NAME = 340, /* VAR_DNSTAP_TLS_SERVER_NAME  */
    VAR_DNSTAP_TLS_CERT_BUNDLE = 341, /* VAR_DNSTAP_TLS_CERT_BUNDLE  */
    VAR_DNSTAP_TLS_CLIENT_KEY_FILE = 342, /* VAR_DNSTAP_TLS_CLIENT_KEY_FILE  */
    VAR_DNSTAP_TLS_CLIENT_CERT_FILE = 343, /* VAR_DNSTAP_TLS_CLIENT_CERT_FILE  */
    VAR_DNSTAP_SEND_IDENTITY = 344, /* VAR_DNSTAP_SEND_IDENTITY  */
    VAR_DNSTAP_SEND_VERSION = 345, /* VAR_DNSTAP_SEND_VERSION  */
    VAR_DNSTAP_IDENTITY = 346,     /* VAR_DNSTAP_IDENTITY  */
    VAR_DNSTAP_VERSION = 347,      /* VAR_DNSTAP_VERSION  */
    VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES = 348, /* VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES  */
    VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES = 349, /* VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES  */
    VAR_REMOTE_CONTROL = 350,      /* VAR_REMOTE_CONTROL  */
    VAR_CONTROL_ENABLE = 351,      /* VAR_CONTROL_ENABLE  */
    VAR_CONTROL_INTERFACE = 352,   /* VAR_CONTROL_INTERFACE  */
    VAR_CONTROL_PORT = 353,        /* VAR_CONTROL_PORT  */
    VAR_SERVER_KEY_FILE = 354,     /* VAR_SERVER_KEY_FILE  */
    VAR_SERVER_CERT_FILE = 355,    /* VAR_SERVER_CERT_FILE  */
    VAR_CONTROL_KEY_FILE = 356,    /* VAR_CONTROL_KEY_FILE  */
    VAR_CONTROL_CERT_FILE = 357,   /* VAR_CONTROL_CERT_FILE  */
    VAR_KEY = 358,                 /* VAR_KEY  */
    VAR_ALGORITHM = 359,           /* VAR_ALGORITHM  */
    VAR_SECRET = 360,              /* VAR_SECRET  */
    VAR_TLS_AUTH = 361,            /* VAR_TLS_AUTH  */
    VAR_TLS_AUTH_DOMAIN_NAME = 362, /* VAR_TLS_AUTH_DOMAIN_NAME  */
    VAR_TLS_AUTH_CLIENT_CERT = 363, /* VAR_TLS_AUTH_CLIENT_CERT  */
    VAR_TLS_AUTH_CLIENT_KEY = 364, /* VAR_TLS_AUTH_CLIENT_KEY  */
    VAR_TLS_AUTH_CLIENT_KEY_PW = 365, /* VAR_TLS_AUTH_CLIENT_KEY_PW  */
    VAR_PATTERN = 366,             /* VAR_PATTERN  */
    VAR_NAME = 367,                /* VAR_NAME  */
    VAR_ZONEFILE = 368,            /* VAR_ZONEFILE  */
    VAR_NOTIFY = 369,              /* VAR_NOTIFY  */
    VAR_PROVIDE_XFR = 370,         /* VAR_PROVIDE_XFR  */
    VAR_ALLOW_QUERY = 371,         /* VAR_ALLOW_QUERY  */
    VAR_AXFR = 372,                /* VAR_AXFR  */
    VAR_UDP = 373,                 /* VAR_UDP  */
    VAR_NOTIFY_RETRY = 374,        /* VAR_NOTIFY_RETRY  */
    VAR_ALLOW_NOTIFY = 375,        /* VAR_ALLOW_NOTIFY  */
    VAR_REQUEST_XFR = 376,         /* VAR_REQUEST_XFR  */
    VAR_ALLOW_AXFR_FALLBACK = 377, /* VAR_ALLOW_AXFR_FALLBACK  */
    VAR_OUTGOING_INTERFACE = 378,  /* VAR_OUTGOING_INTERFACE  */
    VAR_ANSWER_COOKIE = 379,       /* VAR_ANSWER_COOKIE  */
    VAR_COOKIE_SECRET = 380,       /* VAR_COOKIE_SECRET  */
    VAR_COOKIE_SECRET_FILE = 381,  /* VAR_COOKIE_SECRET_FILE  */
    VAR_COOKIE_STAGING_SECRET = 382, /* VAR_COOKIE_STAGING_SECRET  */
    VAR_MAX_REFRESH_TIME = 383,    /* VAR_MAX_REFRESH_TIME  */
    VAR_MIN_REFRESH_TIME = 384,    /* VAR_MIN_REFRESH_TIME  */
    VAR_MAX_RETRY_TIME = 385,      /* VAR_MAX_RETRY_TIME  */
    VAR_MIN_RETRY_TIME = 386,      /* VAR_MIN_RETRY_TIME  */
    VAR_MIN_EXPIRE_TIME = 387,     /* VAR_MIN_EXPIRE_TIME  */
    VAR_MULTI_PRIMARY_CHECK = 388, /* VAR_MULTI_PRIMARY_CHECK  */
    VAR_SIZE_LIMIT_XFR = 389,      /* VAR_SIZE_LIMIT_XFR  */
    VAR_ZONESTATS = 390,           /* VAR_ZONESTATS  */
    VAR_INCLUDE_PATTERN = 391,     /* VAR_INCLUDE_PATTERN  */
    VAR_STORE_IXFR = 392,          /* VAR_STORE_IXFR  */
    VAR_IXFR_SIZE = 393,           /* VAR_IXFR_SIZE  */
    VAR_IXFR_NUMBER = 394,         /* VAR_IXFR_NUMBER  */
    VAR_CREATE_IXFR = 395,         /* VAR_CREATE_IXFR  */
    VAR_CATALOG = 396,             /* VAR_CATALOG  */
    VAR_CATALOG_MEMBER_PATTERN = 397, /* VAR_CATALOG_MEMBER_PATTERN  */
    VAR_CATALOG_PRODUCER_ZONE = 398, /* VAR_CATALOG_PRODUCER_ZONE  */
    VAR_XDP_INTERFACE = 399,       /* VAR_XDP_INTERFACE  */
    VAR_XDP_PROGRAM_PATH = 400,    /* VAR_XDP_PROGRAM_PATH  */
    VAR_XDP_PROGRAM_LOAD = 401,    /* VAR_XDP_PROGRAM_LOAD  */
    VAR_XDP_BPFFS_PATH = 402,      /* VAR_XDP_BPFFS_PATH  */
    VAR_XDP_FORCE_COPY = 403,      /* VAR_XDP_FORCE_COPY  */
    VAR_ZONE = 404,                /* VAR_ZONE  */
    VAR_RRL_WHITELIST = 405,       /* VAR_RRL_WHITELIST  */
    VAR_SERVERS = 406,             /* VAR_SERVERS  */
    VAR_BINDTODEVICE = 407,        /* VAR_BINDTODEVICE  */
    VAR_SETFIB = 408,              /* VAR_SETFIB  */
    VAR_VERIFY = 409,              /* VAR_VERIFY  */
    VAR_ENABLE = 410,              /* VAR_ENABLE  */
    VAR_VERIFY_ZONE = 411,         /* VAR_VERIFY_ZONE  */
    VAR_VERIFY_ZONES = 412,        /* VAR_VERIFY_ZONES  */
    VAR_VERIFIER = 413,            /* VAR_VERIFIER  */
    VAR_VERIFIER_COUNT = 414,      /* VAR_VERIFIER_COUNT  */
    VAR_VERIFIER_FEED_ZONE = 415,  /* VAR_VERIFIER_FEED_ZONE  */
    VAR_VERIFIER_TIMEOUT = 416     /* VAR_VERIFIER_TIMEOUT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
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
#define VAR_LOG_ONLY_SYSLOG 281
#define VAR_PIDFILE 282
#define VAR_DIFFFILE 283
#define VAR_XFRDFILE 284
#define VAR_XFRDIR 285
#define VAR_HIDE_VERSION 286
#define VAR_HIDE_IDENTITY 287
#define VAR_VERSION 288
#define VAR_IDENTITY 289
#define VAR_NSID 290
#define VAR_TCP_COUNT 291
#define VAR_TCP_REJECT_OVERFLOW 292
#define VAR_TCP_QUERY_COUNT 293
#define VAR_TCP_TIMEOUT 294
#define VAR_TCP_MSS 295
#define VAR_OUTGOING_TCP_MSS 296
#define VAR_TCP_LISTEN_QUEUE 297
#define VAR_IPV4_EDNS_SIZE 298
#define VAR_IPV6_EDNS_SIZE 299
#define VAR_STATISTICS 300
#define VAR_XFRD_RELOAD_TIMEOUT 301
#define VAR_LOG_TIME_ASCII 302
#define VAR_LOG_TIME_ISO 303
#define VAR_ROUND_ROBIN 304
#define VAR_MINIMAL_RESPONSES 305
#define VAR_CONFINE_TO_ZONE 306
#define VAR_REFUSE_ANY 307
#define VAR_RELOAD_CONFIG 308
#define VAR_ZONEFILES_CHECK 309
#define VAR_ZONEFILES_WRITE 310
#define VAR_RRL_SIZE 311
#define VAR_RRL_RATELIMIT 312
#define VAR_RRL_SLIP 313
#define VAR_RRL_IPV4_PREFIX_LENGTH 314
#define VAR_RRL_IPV6_PREFIX_LENGTH 315
#define VAR_RRL_WHITELIST_RATELIMIT 316
#define VAR_TLS_SERVICE_KEY 317
#define VAR_TLS_SERVICE_PEM 318
#define VAR_TLS_SERVICE_OCSP 319
#define VAR_TLS_PORT 320
#define VAR_TLS_AUTH_PORT 321
#define VAR_TLS_AUTH_XFR_ONLY 322
#define VAR_TLS_CERT_BUNDLE 323
#define VAR_PROXY_PROTOCOL_PORT 324
#define VAR_CPU_AFFINITY 325
#define VAR_XFRD_CPU_AFFINITY 326
#define VAR_SERVER_CPU_AFFINITY 327
#define VAR_DROP_UPDATES 328
#define VAR_XFRD_TCP_MAX 329
#define VAR_XFRD_TCP_PIPELINE 330
#define VAR_METRICS_ENABLE 331
#define VAR_METRICS_INTERFACE 332
#define VAR_METRICS_PORT 333
#define VAR_METRICS_PATH 334
#define VAR_DNSTAP 335
#define VAR_DNSTAP_ENABLE 336
#define VAR_DNSTAP_SOCKET_PATH 337
#define VAR_DNSTAP_IP 338
#define VAR_DNSTAP_TLS 339
#define VAR_DNSTAP_TLS_SERVER_NAME 340
#define VAR_DNSTAP_TLS_CERT_BUNDLE 341
#define VAR_DNSTAP_TLS_CLIENT_KEY_FILE 342
#define VAR_DNSTAP_TLS_CLIENT_CERT_FILE 343
#define VAR_DNSTAP_SEND_IDENTITY 344
#define VAR_DNSTAP_SEND_VERSION 345
#define VAR_DNSTAP_IDENTITY 346
#define VAR_DNSTAP_VERSION 347
#define VAR_DNSTAP_LOG_AUTH_QUERY_MESSAGES 348
#define VAR_DNSTAP_LOG_AUTH_RESPONSE_MESSAGES 349
#define VAR_REMOTE_CONTROL 350
#define VAR_CONTROL_ENABLE 351
#define VAR_CONTROL_INTERFACE 352
#define VAR_CONTROL_PORT 353
#define VAR_SERVER_KEY_FILE 354
#define VAR_SERVER_CERT_FILE 355
#define VAR_CONTROL_KEY_FILE 356
#define VAR_CONTROL_CERT_FILE 357
#define VAR_KEY 358
#define VAR_ALGORITHM 359
#define VAR_SECRET 360
#define VAR_TLS_AUTH 361
#define VAR_TLS_AUTH_DOMAIN_NAME 362
#define VAR_TLS_AUTH_CLIENT_CERT 363
#define VAR_TLS_AUTH_CLIENT_KEY 364
#define VAR_TLS_AUTH_CLIENT_KEY_PW 365
#define VAR_PATTERN 366
#define VAR_NAME 367
#define VAR_ZONEFILE 368
#define VAR_NOTIFY 369
#define VAR_PROVIDE_XFR 370
#define VAR_ALLOW_QUERY 371
#define VAR_AXFR 372
#define VAR_UDP 373
#define VAR_NOTIFY_RETRY 374
#define VAR_ALLOW_NOTIFY 375
#define VAR_REQUEST_XFR 376
#define VAR_ALLOW_AXFR_FALLBACK 377
#define VAR_OUTGOING_INTERFACE 378
#define VAR_ANSWER_COOKIE 379
#define VAR_COOKIE_SECRET 380
#define VAR_COOKIE_SECRET_FILE 381
#define VAR_COOKIE_STAGING_SECRET 382
#define VAR_MAX_REFRESH_TIME 383
#define VAR_MIN_REFRESH_TIME 384
#define VAR_MAX_RETRY_TIME 385
#define VAR_MIN_RETRY_TIME 386
#define VAR_MIN_EXPIRE_TIME 387
#define VAR_MULTI_PRIMARY_CHECK 388
#define VAR_SIZE_LIMIT_XFR 389
#define VAR_ZONESTATS 390
#define VAR_INCLUDE_PATTERN 391
#define VAR_STORE_IXFR 392
#define VAR_IXFR_SIZE 393
#define VAR_IXFR_NUMBER 394
#define VAR_CREATE_IXFR 395
#define VAR_CATALOG 396
#define VAR_CATALOG_MEMBER_PATTERN 397
#define VAR_CATALOG_PRODUCER_ZONE 398
#define VAR_XDP_INTERFACE 399
#define VAR_XDP_PROGRAM_PATH 400
#define VAR_XDP_PROGRAM_LOAD 401
#define VAR_XDP_BPFFS_PATH 402
#define VAR_XDP_FORCE_COPY 403
#define VAR_ZONE 404
#define VAR_RRL_WHITELIST 405
#define VAR_SERVERS 406
#define VAR_BINDTODEVICE 407
#define VAR_SETFIB 408
#define VAR_VERIFY 409
#define VAR_ENABLE 410
#define VAR_VERIFY_ZONE 411
#define VAR_VERIFY_ZONES 412
#define VAR_VERIFIER 413
#define VAR_VERIFIER_COUNT 414
#define VAR_VERIFIER_FEED_ZONE 415
#define VAR_VERIFIER_TIMEOUT 416

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 48 "configparser.y"

  char *str;
  long long llng;
  int bln;
  struct ip_address_option *ip;
  struct range_option *range;
  struct cpu_option *cpu;
  char **strv;
  struct component *comp;
  int role;

#line 401 "configparser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE c_lval;


int c_parse (void);


#endif /* !YY_C_CONFIGPARSER_H_INCLUDED  */
