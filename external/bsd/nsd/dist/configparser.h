/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison interface for Yacc-like parsers in C

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
#line 37 "configparser.y" /* yacc.c:1910  */

  char *str;
  long long llng;
  int bln;
  struct ip_address_option *ip;

#line 267 "configparser.h" /* yacc.c:1910  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_CONFIGPARSER_H_INCLUDED  */
