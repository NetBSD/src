/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

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
#line 50 "zparser.y" /* yacc.c:1909  */

	domain_type	 *domain;
	const dname_type *dname;
	struct lex_data	  data;
	uint32_t	  ttl;
	uint16_t	  klass;
	uint16_t	  type;
	uint16_t	 *unknown;

#line 246 "zparser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_ZPARSER_H_INCLUDED  */
