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
#line 42 "configparser.y" /* yacc.c:1910  */

	char*	str;

#line 260 "configparser.h" /* yacc.c:1910  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_CONFIGPARSER_H_INCLUDED  */
