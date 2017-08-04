/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)
#define YYENOMEM       (-2)
#define YYEOF          0
#undef YYBTYACC
#define YYBTYACC 0
#define YYDEBUGSTR YYPREFIX "debug"
#define YYPREFIX "yy"

#define YYPURE 0

#line 2 "parser.y"
/*
 * Copyright 2010 Nexenta Systems, Inc.  All rights reserved.
 * Copyright 2015 John Marino <draco@marino.st>
 *
 * This source code is derived from the illumos localedef command, and
 * provided under BSD-style license terms by Nexenta Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: head/usr.bin/localedef/parser.y 306783 2016-10-06 19:51:30Z bapt $
 */

/*
 * POSIX localedef grammar.
 */

#include <wchar.h>
#include <stdio.h>
#include <limits.h>
#include "localedef.h"

#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#line 44 "parser.y"
typedef union {
	int		num;
	wchar_t		wc;
	char		*token;
	collsym_t	*collsym;
	collelem_t	*collelem;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 80 "parser.c"

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define T_CODE_SET 257
#define T_MB_CUR_MAX 258
#define T_MB_CUR_MIN 259
#define T_COM_CHAR 260
#define T_ESC_CHAR 261
#define T_LT 262
#define T_GT 263
#define T_NL 264
#define T_SEMI 265
#define T_COMMA 266
#define T_ELLIPSIS 267
#define T_RPAREN 268
#define T_LPAREN 269
#define T_QUOTE 270
#define T_NULL 271
#define T_WS 272
#define T_END 273
#define T_COPY 274
#define T_CHARMAP 275
#define T_WIDTH 276
#define T_CTYPE 277
#define T_VARIABLE 278
#define T_ISUPPER 279
#define T_ISLOWER 280
#define T_ISALPHA 281
#define T_ISDIGIT 282
#define T_ISPUNCT 283
#define T_ISXDIGIT 284
#define T_ISSPACE 285
#define T_ISPRINT 286
#define T_ISGRAPH 287
#define T_ISBLANK 288
#define T_ISCNTRL 289
#define T_ISALNUM 290
#define T_ISSPECIAL 291
#define T_ISPHONOGRAM 292
#define T_ISIDEOGRAM 293
#define T_ISENGLISH 294
#define T_ISNUMBER 295
#define T_TOUPPER 296
#define T_TOLOWER 297
#define T_COLLATE 298
#define T_COLLATING_SYMBOL 299
#define T_COLLATING_ELEMENT 300
#define T_ORDER_START 301
#define T_ORDER_END 302
#define T_FORWARD 303
#define T_BACKWARD 304
#define T_POSITION 305
#define T_FROM 306
#define T_UNDEFINED 307
#define T_IGNORE 308
#define T_MESSAGES 309
#define T_YESSTR 310
#define T_NOSTR 311
#define T_YESEXPR 312
#define T_NOEXPR 313
#define T_MONETARY 314
#define T_INT_CURR_SYMBOL 315
#define T_CURRENCY_SYMBOL 316
#define T_MON_DECIMAL_POINT 317
#define T_MON_THOUSANDS_SEP 318
#define T_POSITIVE_SIGN 319
#define T_NEGATIVE_SIGN 320
#define T_MON_GROUPING 321
#define T_INT_FRAC_DIGITS 322
#define T_FRAC_DIGITS 323
#define T_P_CS_PRECEDES 324
#define T_P_SEP_BY_SPACE 325
#define T_N_CS_PRECEDES 326
#define T_N_SEP_BY_SPACE 327
#define T_P_SIGN_POSN 328
#define T_N_SIGN_POSN 329
#define T_INT_P_CS_PRECEDES 330
#define T_INT_N_CS_PRECEDES 331
#define T_INT_P_SEP_BY_SPACE 332
#define T_INT_N_SEP_BY_SPACE 333
#define T_INT_P_SIGN_POSN 334
#define T_INT_N_SIGN_POSN 335
#define T_NUMERIC 336
#define T_DECIMAL_POINT 337
#define T_THOUSANDS_SEP 338
#define T_GROUPING 339
#define T_TIME 340
#define T_ABDAY 341
#define T_DAY 342
#define T_ABMON 343
#define T_MON 344
#define T_ERA 345
#define T_ERA_D_FMT 346
#define T_ERA_T_FMT 347
#define T_ERA_D_T_FMT 348
#define T_ALT_DIGITS 349
#define T_D_T_FMT 350
#define T_D_FMT 351
#define T_T_FMT 352
#define T_AM_PM 353
#define T_T_FMT_AMPM 354
#define T_DATE_FMT 355
#define T_CHAR 356
#define T_NAME 357
#define T_NUMBER 358
#define T_SYMBOL 359
#define T_COLLSYM 360
#define T_COLLELEM 361
#define YYERRCODE 256
typedef int YYINT;
static const YYINT yylhs[] = {                           -1,
    0,    0,    3,    3,    4,    4,    1,    1,    5,    5,
    5,    5,    5,    5,    6,    6,    2,    2,    7,    7,
    7,    7,    7,    7,    7,    8,    8,   15,   15,   17,
   17,   17,   16,   16,   18,   18,   18,   18,   18,   18,
   18,   11,   11,   19,   19,   20,   20,   20,   20,   20,
   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
   20,   20,   20,   20,   20,   21,   21,   21,   23,   24,
   24,   22,   22,   25,   25,   25,   25,   12,   12,   12,
   27,   27,   27,   27,   28,   29,   26,   26,   31,   31,
   32,   32,   33,   33,   33,   30,   30,   34,   34,   34,
   35,   35,   35,   35,   35,   36,   36,   36,   37,   37,
   37,   37,   37,   37,   37,   38,   38,   39,   39,   39,
   39,    9,    9,   40,   40,   42,   42,   42,   42,   41,
   10,   10,   43,   43,   45,   45,   45,   45,   45,   45,
   46,   46,   46,   46,   46,   46,   46,   46,   46,   46,
   46,   46,   46,   46,   44,   44,   44,   47,   47,   13,
   13,   48,   48,   49,   49,   50,   50,   51,   51,   14,
   14,   52,   52,   53,   53,   55,   55,   55,   55,   55,
   55,   55,   54,   54,   54,   54,   54,   54,   54,   54,
   56,   56,
};
static const YYINT yylen[] = {                            2,
    2,    1,    3,    2,    2,    1,    2,    1,    3,    3,
    3,    3,    3,    3,    3,    3,    2,    1,    1,    1,
    1,    1,    1,    1,    1,    6,    6,    2,    1,    2,
    4,    1,    2,    1,    3,    3,    5,    5,    5,    5,
    1,    6,    6,    2,    1,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    1,    3,    1,
    1,    3,    1,    5,    5,    5,    5,    6,    7,    6,
    2,    2,    1,    1,    3,    5,    5,    6,    3,    1,
    3,    1,    1,    1,    1,    2,    1,    2,    2,    3,
    1,    1,    1,    1,    1,    3,    2,    1,    1,    1,
    1,    1,    1,    1,    3,    2,    1,    1,    1,    1,
    1,    6,    6,    2,    1,    1,    1,    1,    1,    3,
    6,    6,    2,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    3,    3,    3,    1,    3,    6,
    6,    2,    1,    3,    3,    1,    1,    1,    3,    6,
    6,    2,    1,    3,    3,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    3,    1,
};
static const YYINT yydefred[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    8,   18,   19,   20,
   21,   22,   23,   24,   25,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    7,   17,    4,    6,    0,   14,   13,   11,   12,
    9,   10,   32,    0,    0,   29,   41,    0,    0,    0,
   34,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   45,    0,    0,    0,    0,    0,
    0,   83,   84,  126,  127,  128,  129,    0,    0,  125,
    0,  135,  136,  137,  138,  139,  140,    0,  141,  142,
  143,  144,  145,  146,  147,  148,  149,  150,  151,  152,
  153,  154,    0,    0,  134,    0,    0,  166,  167,    0,
    0,    0,  163,    0,  176,  177,  178,  179,  180,  185,
  184,  183,  181,  186,  187,  188,  182,  189,  190,    0,
    0,  173,    0,    0,    3,    5,    0,   30,    0,   28,
    0,    0,    0,    0,    0,   33,    0,    0,    0,   70,
   71,    0,   68,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   73,    0,    0,    0,   44,    0,    0,    0,   93,
   94,   95,    0,    0,   92,    0,    0,    0,   81,   82,
    0,    0,  124,    0,  158,    0,    0,    0,  133,    0,
    0,  168,    0,    0,    0,  162,    0,    0,    0,  172,
    0,  192,    0,    0,    0,    0,    0,   35,    0,    0,
   36,    0,   15,   16,   65,   46,    0,   47,   48,   49,
   50,   51,   52,   53,   54,   55,   56,   57,   58,   62,
   61,   59,   60,    0,    0,   63,    0,   64,    0,    0,
   85,    0,  102,  104,  101,  105,    0,  103,    0,   97,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  130,
  157,    0,    0,    0,  155,  156,  165,    0,    0,    0,
  164,    0,    0,  174,  175,    0,   31,   26,    0,    0,
    0,    0,   27,    0,   66,   67,    0,    0,   72,   43,
   42,    0,   98,    0,   96,   99,  112,    0,  113,  111,
  114,  110,  109,    0,  108,    0,    0,   91,   80,   78,
    0,  123,  122,  159,  132,  131,  169,  161,  160,  171,
  170,  191,   37,   39,   40,   38,    0,    0,    0,    0,
    0,   86,   87,  118,  121,  119,  120,    0,  117,  100,
    0,    0,   79,   69,   74,   77,   75,   76,  115,  116,
  106,   88,
};
#if defined(YYDESTRUCT_CALL) || defined(YYSTYPE_TOSTRING)
static const YYINT yystos[] = {                           0,
  257,  258,  259,  260,  261,  275,  276,  277,  298,  309,
  314,  336,  340,  363,  364,  365,  368,  370,  371,  372,
  373,  374,  375,  376,  377,  270,  357,  366,  358,  358,
  356,  356,  264,  264,  264,  264,  264,  264,  264,  264,
  365,  368,  370,  270,  356,  367,  264,  264,  264,  264,
  264,  264,  264,  359,  378,  380,  264,  356,  359,  379,
  381,  274,  278,  279,  280,  281,  282,  283,  284,  285,
  286,  287,  288,  289,  290,  291,  292,  293,  294,  295,
  296,  297,  369,  382,  383,  299,  300,  301,  369,  389,
  390,  391,  392,  310,  311,  312,  313,  369,  403,  404,
  405,  315,  316,  317,  318,  319,  320,  321,  322,  323,
  324,  325,  326,  327,  328,  329,  330,  331,  332,  333,
  334,  335,  369,  406,  407,  408,  409,  337,  338,  339,
  369,  411,  412,  413,  341,  342,  343,  344,  345,  346,
  347,  348,  349,  350,  351,  352,  353,  354,  355,  369,
  415,  416,  417,  418,  270,  356,  267,  356,  273,  380,
  267,  358,  267,  358,  273,  381,  357,  366,  366,  356,
  359,  384,  387,  384,  384,  384,  384,  384,  384,  384,
  384,  384,  384,  384,  384,  384,  384,  384,  384,  269,
  385,  388,  385,  273,  273,  383,  359,  359,  264,  303,
  304,  305,  394,  395,  396,  273,  273,  389,  391,  392,
  273,  273,  404,  366,  358,  410,  273,  273,  407,  366,
  358,  358,  414,  273,  273,  412,  366,  273,  273,  416,
  366,  366,  419,  359,  275,  356,  359,  264,  356,  359,
  264,  276,  264,  264,  264,  264,  265,  264,  264,  264,
  264,  264,  264,  264,  264,  264,  264,  264,  264,  264,
  264,  264,  264,  356,  359,  264,  265,  264,  277,  277,
  264,  306,  267,  307,  356,  359,  360,  361,  393,  397,
  398,  264,  265,  266,  298,  298,  273,  309,  309,  264,
  264,  265,  314,  314,  264,  264,  264,  265,  336,  336,
  264,  336,  340,  264,  264,  265,  356,  264,  358,  358,
  358,  358,  264,  267,  386,  387,  266,  266,  388,  264,
  264,  366,  264,  302,  397,  264,  267,  270,  308,  356,
  359,  360,  361,  399,  400,  393,  395,  396,  264,  264,
  298,  264,  264,  358,  264,  264,  358,  264,  264,  264,
  264,  366,  264,  264,  264,  264,  265,  356,  359,  356,
  359,  264,  264,  356,  359,  360,  361,  401,  402,  264,
  265,  302,  264,  356,  268,  268,  268,  268,  270,  402,
  400,  264,
};
#endif /* YYDESTRUCT_CALL || YYSTYPE_TOSTRING */
static const YYINT yydgoto[] = {                         14,
   15,   16,   28,   46,   17,   83,   18,   19,   20,   21,
   22,   23,   24,   25,   55,   60,   56,   61,   84,   85,
  172,  191,  315,  173,  192,   90,   91,   92,   93,  279,
  203,  204,  205,  280,  281,  334,  335,  368,  369,   99,
  100,  101,  124,  125,  126,  127,  216,  132,  133,  134,
  223,  151,  152,  153,  154,  233,
};
static const YYINT yysindex[] = {                      -118,
 -267, -351, -342, -335, -334, -230, -201, -161, -154, -135,
  -95,  -87,  -85,    0, -118, -128,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -247,  -57,  -55,  -53,  -47,
  -41,   -4, -252, -254,   54, -217, -146,  -33, -187,  -96,
 -128,    0,    0,    0,    0, -240,    0,    0,    0,    0,
    0,    0,    0, -261, -253,    0,    0, -243, -238, -255,
    0, -257, -244, -328, -328, -328, -328, -328, -328, -328,
 -328, -328, -328, -328, -328, -328, -328, -328, -328, -328,
 -228, -228, -105,   30,    0,  -30,  -29, -237,   58,   95,
 -155,    0,    0,    0,    0,    0,    0,   96, -236,    0,
 -244,    0,    0,    0,    0,    0,    0,   13,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   97,  -54,    0, -244,   14,    0,    0,   15,
  101, -258,    0, -244,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  102,
 -117,    0, -244, -244,    0,    0,   17,    0,  103,    0,
 -323,  113, -303,  115,  104,    0,  117,  118,  119,    0,
    0, -203,    0, -195, -142, -130, -104, -102,  -93,  -91,
  -89,  -82,  -80,  -77,  -75,  -72,  -70,  -67,  -65, -301,
  -63,    0,  -61,  107,  108,    0,  122,   81, -229,    0,
    0,    0,  -59,  123,    0,   90,   92,  120,    0,    0,
   82,   83,    0,  130,    0,  -51,   84,   85,    0,  131,
  132,    0,  -49,   61,   64,    0,  137,   66,   63,    0,
  140,    0,  -44,   49,  142,   50,   51,    0,   52,   53,
    0,  143,    0,    0,    0,    0, -263,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  146,  147,    0, -228,    0,  150,  151,
    0, -244,    0,    0,    0,    0,  152,    0, -248,    0,
 -262, -229, -150, -150,  153,  154,  121,  156,  157,    0,
    0,   65,  158,  160,    0,    0,    0,   67,  162,  163,
    0,  164,  165,    0,    0, -244,    0,    0,  166,  167,
  168,  169,    0,  170,    0,    0, -271, -270,    0,    0,
    0,  172,    0,  173,    0,    0,    0, -316,    0,    0,
    0,    0,    0,  -25,    0, -242,  123,    0,    0,    0,
  174,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   78,  171,  175,  176,
  177,    0,    0,    0,    0,    0,    0, -223,    0,    0,
 -235,  178,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
static const YYINT yyrindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  440,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  441,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -22,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   40,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   42,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
#if YYBTYACC
static const YYINT yycindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
#endif
static const YYINT yygindex[] = {                         0,
    0,  431,  -62,    0,  432,   12,    1,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  393,  389,    0,  366,
  287,  369,    0,  205,  186,  363,    0,  364,  365,  179,
    0,  180,  181, -265,    0,    0,   86,    0,   91,    0,
  359,    0,    0,  336,    0,    0,    0,    0,  330,    0,
    0,    0,  313,    0,    0,    0,
};
#define YYTABLESIZE 465
static const YYINT yytable[] = {                        168,
  169,  326,   26,  314,  327,  157,   29,  328,   57,   57,
   53,   53,   26,  325,  225,   30,   43,  165,  273,  159,
   31,   32,   44,  161,  273,   26,  199,  170,  163,  155,
  171,  327,  236,   33,  328,  237,  212,  273,  214,  364,
  190,   43,  365,  366,  367,  329,  379,   89,   98,  123,
  131,  150,  239,  324,  264,  240,   62,  265,  274,  372,
  246,  247,   34,  220,  274,  200,  201,  202,  248,  247,
  325,  227,  329,   94,   95,   96,   97,  274,  128,  129,
  130,   86,   87,   88,  358,  360,   62,  359,  361,   27,
  231,  232,  170,  330,  158,  171,  331,  332,  333,  167,
   58,   58,   35,   59,   59,   54,   54,  275,   45,   36,
  276,  277,  278,  275,  162,  156,  276,  277,  278,  164,
  330,  249,  247,  331,  332,  333,  275,   62,   37,  276,
  277,  278,  364,  250,  247,  365,  366,  367,    1,    2,
    3,    4,    5,   86,   87,   88,    6,    7,    8,  128,
  129,  130,  200,  201,  202,  229,    6,    7,    8,  251,
  247,  252,  247,   94,   95,   96,   97,  194,   38,    9,
  253,  247,  254,  247,  255,  247,   39,   62,   40,    9,
   10,  256,  247,  257,  247,   11,  258,  247,  259,  247,
   10,  260,  247,  261,  247,   11,  262,  247,  263,  247,
  266,  267,  268,  267,  282,  283,   47,   12,   48,  322,
   49,   13,  291,  292,  297,  298,   50,   12,  218,  305,
  306,   13,   51,  135,  136,  137,  138,  139,  140,  141,
  142,  143,  144,  145,  146,  147,  148,  149,  370,  371,
   62,   90,   90,  352,  135,  136,  137,  138,  139,  140,
  141,  142,  143,  144,  145,  146,  147,  148,  149,   52,
  102,  103,  104,  105,  106,  107,  108,  109,  110,  111,
  112,  113,  114,  115,  116,  117,  118,  119,  120,  121,
  122,  102,  103,  104,  105,  106,  107,  108,  109,  110,
  111,  112,  113,  114,  115,  116,  117,  118,  119,  120,
  121,  122,  195,   89,   89,  107,  107,   63,   64,   65,
   66,   67,   68,   69,   70,   71,   72,   73,   74,   75,
   76,   77,   78,   79,   80,   81,   82,   62,  197,  198,
  206,   63,   64,   65,   66,   67,   68,   69,   70,   71,
   72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
   82,  174,  175,  176,  177,  178,  179,  180,  181,  182,
  183,  184,  185,  186,  187,  188,  189,  207,  211,  217,
  215,  221,  222,  224,  228,  234,  238,  235,  241,  242,
  243,  244,  245,  269,  270,  271,  272,  285,  284,  286,
  288,  289,  287,  290,  295,  296,  299,  293,  294,  300,
  301,  302,  303,  304,  307,  308,  313,  309,  310,  311,
  312,  317,  318,  320,  321,  323,  339,  340,  341,  342,
  343,  345,  344,  346,  347,  348,  349,  350,  351,  353,
  354,  355,  356,  374,  357,  362,  363,  373,  375,    2,
    1,  382,  376,  377,  378,   41,   42,  160,  166,  196,
  193,  316,  319,  208,  209,  210,  381,  213,  380,  219,
  336,  226,  337,  230,  338,
};
static const YYINT yycheck[] = {                         62,
   63,  264,  270,  267,  267,  267,  358,  270,  264,  264,
  264,  264,  270,  279,  273,  358,   16,  273,  267,  273,
  356,  356,  270,  267,  267,  270,  264,  356,  267,  270,
  359,  267,  356,  264,  270,  359,  273,  267,  101,  356,
  269,   41,  359,  360,  361,  308,  270,   36,   37,   38,
   39,   40,  356,  302,  356,  359,  274,  359,  307,  302,
  264,  265,  264,  126,  307,  303,  304,  305,  264,  265,
  336,  134,  308,  310,  311,  312,  313,  307,  337,  338,
  339,  299,  300,  301,  356,  356,  274,  359,  359,  357,
  153,  154,  356,  356,  356,  359,  359,  360,  361,  357,
  356,  356,  264,  359,  359,  359,  359,  356,  356,  264,
  359,  360,  361,  356,  358,  356,  359,  360,  361,  358,
  356,  264,  265,  359,  360,  361,  356,  274,  264,  359,
  360,  361,  356,  264,  265,  359,  360,  361,  257,  258,
  259,  260,  261,  299,  300,  301,  275,  276,  277,  337,
  338,  339,  303,  304,  305,  273,  275,  276,  277,  264,
  265,  264,  265,  310,  311,  312,  313,  273,  264,  298,
  264,  265,  264,  265,  264,  265,  264,  274,  264,  298,
  309,  264,  265,  264,  265,  314,  264,  265,  264,  265,
  309,  264,  265,  264,  265,  314,  264,  265,  264,  265,
  264,  265,  264,  265,  264,  265,  264,  336,  264,  272,
  264,  340,  264,  265,  264,  265,  264,  336,  273,  264,
  265,  340,  264,  341,  342,  343,  344,  345,  346,  347,
  348,  349,  350,  351,  352,  353,  354,  355,  264,  265,
  274,  264,  265,  306,  341,  342,  343,  344,  345,  346,
  347,  348,  349,  350,  351,  352,  353,  354,  355,  264,
  315,  316,  317,  318,  319,  320,  321,  322,  323,  324,
  325,  326,  327,  328,  329,  330,  331,  332,  333,  334,
  335,  315,  316,  317,  318,  319,  320,  321,  322,  323,
  324,  325,  326,  327,  328,  329,  330,  331,  332,  333,
  334,  335,  273,  264,  265,  264,  265,  278,  279,  280,
  281,  282,  283,  284,  285,  286,  287,  288,  289,  290,
  291,  292,  293,  294,  295,  296,  297,  274,  359,  359,
  273,  278,  279,  280,  281,  282,  283,  284,  285,  286,
  287,  288,  289,  290,  291,  292,  293,  294,  295,  296,
  297,   65,   66,   67,   68,   69,   70,   71,   72,   73,
   74,   75,   76,   77,   78,   79,   80,  273,  273,  273,
  358,  358,  358,  273,  273,  359,  264,  275,  264,  276,
  264,  264,  264,  277,  277,  264,  306,  298,  266,  298,
  309,  309,  273,  264,  264,  264,  336,  314,  314,  336,
  264,  336,  340,  264,  356,  264,  264,  358,  358,  358,
  358,  266,  266,  264,  264,  264,  264,  264,  298,  264,
  264,  264,  358,  264,  358,  264,  264,  264,  264,  264,
  264,  264,  264,  356,  265,  264,  264,  264,  268,    0,
    0,  264,  268,  268,  268,   15,   15,   55,   60,   84,
   82,  247,  267,   91,   91,   91,  371,   99,  368,  124,
  282,  132,  283,  151,  284,
};
#if YYBTYACC
static const YYINT yyctable[] = {                        -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,
};
#endif
#define YYFINAL 14
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 361
#define YYUNDFTOKEN 420
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const yyname[] = {

"$end",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"error","T_CODE_SET","T_MB_CUR_MAX",
"T_MB_CUR_MIN","T_COM_CHAR","T_ESC_CHAR","T_LT","T_GT","T_NL","T_SEMI",
"T_COMMA","T_ELLIPSIS","T_RPAREN","T_LPAREN","T_QUOTE","T_NULL","T_WS","T_END",
"T_COPY","T_CHARMAP","T_WIDTH","T_CTYPE","T_VARIABLE","T_ISUPPER","T_ISLOWER",
"T_ISALPHA","T_ISDIGIT","T_ISPUNCT","T_ISXDIGIT","T_ISSPACE","T_ISPRINT",
"T_ISGRAPH","T_ISBLANK","T_ISCNTRL","T_ISALNUM","T_ISSPECIAL","T_ISPHONOGRAM",
"T_ISIDEOGRAM","T_ISENGLISH","T_ISNUMBER","T_TOUPPER","T_TOLOWER","T_COLLATE",
"T_COLLATING_SYMBOL","T_COLLATING_ELEMENT","T_ORDER_START","T_ORDER_END",
"T_FORWARD","T_BACKWARD","T_POSITION","T_FROM","T_UNDEFINED","T_IGNORE",
"T_MESSAGES","T_YESSTR","T_NOSTR","T_YESEXPR","T_NOEXPR","T_MONETARY",
"T_INT_CURR_SYMBOL","T_CURRENCY_SYMBOL","T_MON_DECIMAL_POINT",
"T_MON_THOUSANDS_SEP","T_POSITIVE_SIGN","T_NEGATIVE_SIGN","T_MON_GROUPING",
"T_INT_FRAC_DIGITS","T_FRAC_DIGITS","T_P_CS_PRECEDES","T_P_SEP_BY_SPACE",
"T_N_CS_PRECEDES","T_N_SEP_BY_SPACE","T_P_SIGN_POSN","T_N_SIGN_POSN",
"T_INT_P_CS_PRECEDES","T_INT_N_CS_PRECEDES","T_INT_P_SEP_BY_SPACE",
"T_INT_N_SEP_BY_SPACE","T_INT_P_SIGN_POSN","T_INT_N_SIGN_POSN","T_NUMERIC",
"T_DECIMAL_POINT","T_THOUSANDS_SEP","T_GROUPING","T_TIME","T_ABDAY","T_DAY",
"T_ABMON","T_MON","T_ERA","T_ERA_D_FMT","T_ERA_T_FMT","T_ERA_D_T_FMT",
"T_ALT_DIGITS","T_D_T_FMT","T_D_FMT","T_T_FMT","T_AM_PM","T_T_FMT_AMPM",
"T_DATE_FMT","T_CHAR","T_NAME","T_NUMBER","T_SYMBOL","T_COLLSYM","T_COLLELEM",
"$accept","localedef","setting_list","categories","string","charlist","setting",
"copycat","category","charmap","messages","monetary","ctype","collate",
"numeric","time","charmap_list","width_list","charmap_entry","width_entry",
"ctype_list","ctype_kw","cc_list","conv_list","cc_range_end","cc_char",
"conv_pair","coll_order","coll_optional","coll_symbols","coll_elements",
"order_list","order_args","order_arg","order_dir","order_item","order_itemkw",
"order_weights","order_weight","order_str","order_stritem","messages_list",
"messages_item","messages_kw","monetary_list","monetary_kw","monetary_strkw",
"monetary_numkw","mon_group_list","numeric_list","numeric_item","numeric_strkw",
"group_list","time_kwlist","time_kw","time_strkw","time_listkw","time_list",
"illegal-symbol",
};
static const char *const yyrule[] = {
"$accept : localedef",
"localedef : setting_list categories",
"localedef : categories",
"string : T_QUOTE charlist T_QUOTE",
"string : T_QUOTE T_QUOTE",
"charlist : charlist T_CHAR",
"charlist : T_CHAR",
"setting_list : setting_list setting",
"setting_list : setting",
"setting : T_COM_CHAR T_CHAR T_NL",
"setting : T_ESC_CHAR T_CHAR T_NL",
"setting : T_MB_CUR_MAX T_NUMBER T_NL",
"setting : T_MB_CUR_MIN T_NUMBER T_NL",
"setting : T_CODE_SET string T_NL",
"setting : T_CODE_SET T_NAME T_NL",
"copycat : T_COPY T_NAME T_NL",
"copycat : T_COPY string T_NL",
"categories : categories category",
"categories : category",
"category : charmap",
"category : messages",
"category : monetary",
"category : ctype",
"category : collate",
"category : numeric",
"category : time",
"charmap : T_CHARMAP T_NL charmap_list T_END T_CHARMAP T_NL",
"charmap : T_WIDTH T_NL width_list T_END T_WIDTH T_NL",
"charmap_list : charmap_list charmap_entry",
"charmap_list : charmap_entry",
"charmap_entry : T_SYMBOL T_CHAR",
"charmap_entry : T_SYMBOL T_ELLIPSIS T_SYMBOL T_CHAR",
"charmap_entry : T_NL",
"width_list : width_list width_entry",
"width_list : width_entry",
"width_entry : T_CHAR T_NUMBER T_NL",
"width_entry : T_SYMBOL T_NUMBER T_NL",
"width_entry : T_CHAR T_ELLIPSIS T_CHAR T_NUMBER T_NL",
"width_entry : T_SYMBOL T_ELLIPSIS T_SYMBOL T_NUMBER T_NL",
"width_entry : T_CHAR T_ELLIPSIS T_SYMBOL T_NUMBER T_NL",
"width_entry : T_SYMBOL T_ELLIPSIS T_CHAR T_NUMBER T_NL",
"width_entry : T_NL",
"ctype : T_CTYPE T_NL ctype_list T_END T_CTYPE T_NL",
"ctype : T_CTYPE T_NL copycat T_END T_CTYPE T_NL",
"ctype_list : ctype_list ctype_kw",
"ctype_list : ctype_kw",
"ctype_kw : T_ISUPPER cc_list T_NL",
"ctype_kw : T_ISLOWER cc_list T_NL",
"ctype_kw : T_ISALPHA cc_list T_NL",
"ctype_kw : T_ISDIGIT cc_list T_NL",
"ctype_kw : T_ISPUNCT cc_list T_NL",
"ctype_kw : T_ISXDIGIT cc_list T_NL",
"ctype_kw : T_ISSPACE cc_list T_NL",
"ctype_kw : T_ISPRINT cc_list T_NL",
"ctype_kw : T_ISGRAPH cc_list T_NL",
"ctype_kw : T_ISBLANK cc_list T_NL",
"ctype_kw : T_ISCNTRL cc_list T_NL",
"ctype_kw : T_ISALNUM cc_list T_NL",
"ctype_kw : T_ISSPECIAL cc_list T_NL",
"ctype_kw : T_ISENGLISH cc_list T_NL",
"ctype_kw : T_ISNUMBER cc_list T_NL",
"ctype_kw : T_ISIDEOGRAM cc_list T_NL",
"ctype_kw : T_ISPHONOGRAM cc_list T_NL",
"ctype_kw : T_TOUPPER conv_list T_NL",
"ctype_kw : T_TOLOWER conv_list T_NL",
"ctype_kw : T_VARIABLE string T_NL",
"cc_list : cc_list T_SEMI cc_range_end",
"cc_list : cc_list T_SEMI cc_char",
"cc_list : cc_char",
"cc_range_end : T_ELLIPSIS T_SEMI T_CHAR",
"cc_char : T_CHAR",
"cc_char : T_SYMBOL",
"conv_list : conv_list T_SEMI conv_pair",
"conv_list : conv_pair",
"conv_pair : T_LPAREN T_CHAR T_COMMA T_CHAR T_RPAREN",
"conv_pair : T_LPAREN T_SYMBOL T_COMMA T_CHAR T_RPAREN",
"conv_pair : T_LPAREN T_SYMBOL T_COMMA T_SYMBOL T_RPAREN",
"conv_pair : T_LPAREN T_CHAR T_COMMA T_SYMBOL T_RPAREN",
"collate : T_COLLATE T_NL coll_order T_END T_COLLATE T_NL",
"collate : T_COLLATE T_NL coll_optional coll_order T_END T_COLLATE T_NL",
"collate : T_COLLATE T_NL copycat T_END T_COLLATE T_NL",
"coll_optional : coll_optional coll_symbols",
"coll_optional : coll_optional coll_elements",
"coll_optional : coll_symbols",
"coll_optional : coll_elements",
"coll_symbols : T_COLLATING_SYMBOL T_SYMBOL T_NL",
"coll_elements : T_COLLATING_ELEMENT T_SYMBOL T_FROM string T_NL",
"coll_order : T_ORDER_START T_NL order_list T_ORDER_END T_NL",
"coll_order : T_ORDER_START order_args T_NL order_list T_ORDER_END T_NL",
"order_args : order_args T_SEMI order_arg",
"order_args : order_arg",
"order_arg : order_arg T_COMMA order_dir",
"order_arg : order_dir",
"order_dir : T_FORWARD",
"order_dir : T_BACKWARD",
"order_dir : T_POSITION",
"order_list : order_list order_item",
"order_list : order_item",
"order_item : T_COLLSYM T_NL",
"order_item : order_itemkw T_NL",
"order_item : order_itemkw order_weights T_NL",
"order_itemkw : T_CHAR",
"order_itemkw : T_ELLIPSIS",
"order_itemkw : T_COLLELEM",
"order_itemkw : T_UNDEFINED",
"order_itemkw : T_SYMBOL",
"order_weights : order_weights T_SEMI order_weight",
"order_weights : order_weights T_SEMI",
"order_weights : order_weight",
"order_weight : T_COLLELEM",
"order_weight : T_COLLSYM",
"order_weight : T_CHAR",
"order_weight : T_ELLIPSIS",
"order_weight : T_IGNORE",
"order_weight : T_SYMBOL",
"order_weight : T_QUOTE order_str T_QUOTE",
"order_str : order_str order_stritem",
"order_str : order_stritem",
"order_stritem : T_CHAR",
"order_stritem : T_COLLSYM",
"order_stritem : T_COLLELEM",
"order_stritem : T_SYMBOL",
"messages : T_MESSAGES T_NL messages_list T_END T_MESSAGES T_NL",
"messages : T_MESSAGES T_NL copycat T_END T_MESSAGES T_NL",
"messages_list : messages_list messages_item",
"messages_list : messages_item",
"messages_kw : T_YESSTR",
"messages_kw : T_NOSTR",
"messages_kw : T_YESEXPR",
"messages_kw : T_NOEXPR",
"messages_item : messages_kw string T_NL",
"monetary : T_MONETARY T_NL monetary_list T_END T_MONETARY T_NL",
"monetary : T_MONETARY T_NL copycat T_END T_MONETARY T_NL",
"monetary_list : monetary_list monetary_kw",
"monetary_list : monetary_kw",
"monetary_strkw : T_INT_CURR_SYMBOL",
"monetary_strkw : T_CURRENCY_SYMBOL",
"monetary_strkw : T_MON_DECIMAL_POINT",
"monetary_strkw : T_MON_THOUSANDS_SEP",
"monetary_strkw : T_POSITIVE_SIGN",
"monetary_strkw : T_NEGATIVE_SIGN",
"monetary_numkw : T_INT_FRAC_DIGITS",
"monetary_numkw : T_FRAC_DIGITS",
"monetary_numkw : T_P_CS_PRECEDES",
"monetary_numkw : T_P_SEP_BY_SPACE",
"monetary_numkw : T_N_CS_PRECEDES",
"monetary_numkw : T_N_SEP_BY_SPACE",
"monetary_numkw : T_P_SIGN_POSN",
"monetary_numkw : T_N_SIGN_POSN",
"monetary_numkw : T_INT_P_CS_PRECEDES",
"monetary_numkw : T_INT_N_CS_PRECEDES",
"monetary_numkw : T_INT_P_SEP_BY_SPACE",
"monetary_numkw : T_INT_N_SEP_BY_SPACE",
"monetary_numkw : T_INT_P_SIGN_POSN",
"monetary_numkw : T_INT_N_SIGN_POSN",
"monetary_kw : monetary_strkw string T_NL",
"monetary_kw : monetary_numkw T_NUMBER T_NL",
"monetary_kw : T_MON_GROUPING mon_group_list T_NL",
"mon_group_list : T_NUMBER",
"mon_group_list : mon_group_list T_SEMI T_NUMBER",
"numeric : T_NUMERIC T_NL numeric_list T_END T_NUMERIC T_NL",
"numeric : T_NUMERIC T_NL copycat T_END T_NUMERIC T_NL",
"numeric_list : numeric_list numeric_item",
"numeric_list : numeric_item",
"numeric_item : numeric_strkw string T_NL",
"numeric_item : T_GROUPING group_list T_NL",
"numeric_strkw : T_DECIMAL_POINT",
"numeric_strkw : T_THOUSANDS_SEP",
"group_list : T_NUMBER",
"group_list : group_list T_SEMI T_NUMBER",
"time : T_TIME T_NL time_kwlist T_END T_TIME T_NL",
"time : T_TIME T_NL copycat T_END T_NUMERIC T_NL",
"time_kwlist : time_kwlist time_kw",
"time_kwlist : time_kw",
"time_kw : time_strkw string T_NL",
"time_kw : time_listkw time_list T_NL",
"time_listkw : T_ABDAY",
"time_listkw : T_DAY",
"time_listkw : T_ABMON",
"time_listkw : T_MON",
"time_listkw : T_ERA",
"time_listkw : T_ALT_DIGITS",
"time_listkw : T_AM_PM",
"time_strkw : T_ERA_D_T_FMT",
"time_strkw : T_ERA_T_FMT",
"time_strkw : T_ERA_D_FMT",
"time_strkw : T_D_T_FMT",
"time_strkw : T_D_FMT",
"time_strkw : T_T_FMT",
"time_strkw : T_T_FMT_AMPM",
"time_strkw : T_DATE_FMT",
"time_list : time_list T_SEMI string",
"time_list : string",

};
#endif

int      yydebug;
int      yynerrs;

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
YYLTYPE  yyloc; /* position returned by actions */
YYLTYPE  yylloc; /* position from the lexer */
#endif

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
#ifndef YYLLOC_DEFAULT
#define YYLLOC_DEFAULT(loc, rhs, n) \
do \
{ \
    if (n == 0) \
    { \
        (loc).first_line   = ((rhs)[-1]).last_line; \
        (loc).first_column = ((rhs)[-1]).last_column; \
        (loc).last_line    = ((rhs)[-1]).last_line; \
        (loc).last_column  = ((rhs)[-1]).last_column; \
    } \
    else \
    { \
        (loc).first_line   = ((rhs)[ 0 ]).first_line; \
        (loc).first_column = ((rhs)[ 0 ]).first_column; \
        (loc).last_line    = ((rhs)[n-1]).last_line; \
        (loc).last_column  = ((rhs)[n-1]).last_column; \
    } \
} while (0)
#endif /* YYLLOC_DEFAULT */
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#if YYBTYACC

#ifndef YYLVQUEUEGROWTH
#define YYLVQUEUEGROWTH 32
#endif
#endif /* YYBTYACC */

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  10000
#endif
#endif

#ifndef YYINITSTACKSIZE
#define YYINITSTACKSIZE 200
#endif

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  *p_base;
    YYLTYPE  *p_mark;
#endif
} YYSTACKDATA;
#if YYBTYACC

struct YYParseState_s
{
    struct YYParseState_s *save;    /* Previously saved parser state */
    YYSTACKDATA            yystack; /* saved parser stack */
    int                    state;   /* saved parser state */
    int                    errflag; /* saved error recovery status */
    int                    lexeme;  /* saved index of the conflict lexeme in the lexical queue */
    YYINT                  ctry;    /* saved index in yyctable[] for this conflict */
};
typedef struct YYParseState_s YYParseState;
#endif /* YYBTYACC */
/* variables for the parser stack */
static YYSTACKDATA yystack;
#if YYBTYACC

/* Current parser state */
static YYParseState *yyps = 0;

/* yypath != NULL: do the full parse, starting at *yypath parser state. */
static YYParseState *yypath = 0;

/* Base of the lexical value queue */
static YYSTYPE *yylvals = 0;

/* Current position at lexical value queue */
static YYSTYPE *yylvp = 0;

/* End position of lexical value queue */
static YYSTYPE *yylve = 0;

/* The last allocated position at the lexical value queue */
static YYSTYPE *yylvlim = 0;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
/* Base of the lexical position queue */
static YYLTYPE *yylpsns = 0;

/* Current position at lexical position queue */
static YYLTYPE *yylpp = 0;

/* End position of lexical position queue */
static YYLTYPE *yylpe = 0;

/* The last allocated position at the lexical position queue */
static YYLTYPE *yylplim = 0;
#endif

/* Current position at lexical token queue */
static YYINT  *yylexp = 0;

static YYINT  *yylexemes = 0;
#endif /* YYBTYACC */

/* For use in generated program */
#define yydepth (int)(yystack.s_mark - yystack.s_base)
#if YYBTYACC
#define yytrial (yyps->save)
#endif /* YYBTYACC */

#if YYDEBUG
#include <stdio.h>	/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    YYINT *newss;
    YYSTYPE *newvs;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE *newps;
#endif

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return YYENOMEM;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (YYINT *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return YYENOMEM;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return YYENOMEM;

    data->l_base = newvs;
    data->l_mark = newvs + i;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    newps = (YYLTYPE *)realloc(data->p_base, newsize * sizeof(*newps));
    if (newps == 0)
        return YYENOMEM;

    data->p_base = newps;
    data->p_mark = newps + i;
#endif

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;

#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%sdebug: stack size increased to %d\n", YYPREFIX, newsize);
#endif
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    free(data->p_base);
#endif
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif /* YYPURE || defined(YY_NO_LEAKS) */
#if YYBTYACC

static YYParseState *
yyNewState(unsigned size)
{
    YYParseState *p = (YYParseState *) malloc(sizeof(YYParseState));
    if (p == NULL) return NULL;

    p->yystack.stacksize = size;
    if (size == 0)
    {
        p->yystack.s_base = NULL;
        p->yystack.l_base = NULL;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        p->yystack.p_base = NULL;
#endif
        return p;
    }
    p->yystack.s_base    = (YYINT *) malloc(size * sizeof(YYINT));
    if (p->yystack.s_base == NULL) return NULL;
    p->yystack.l_base    = (YYSTYPE *) malloc(size * sizeof(YYSTYPE));
    if (p->yystack.l_base == NULL) return NULL;
    memset(p->yystack.l_base, 0, size * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    p->yystack.p_base    = (YYLTYPE *) malloc(size * sizeof(YYLTYPE));
    if (p->yystack.p_base == NULL) return NULL;
    memset(p->yystack.p_base, 0, size * sizeof(YYLTYPE));
#endif

    return p;
}

static void
yyFreeState(YYParseState *p)
{
    yyfreestack(&p->yystack);
    free(p);
}
#endif /* YYBTYACC */

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab
#if YYBTYACC
#define YYVALID        do { if (yyps->save)            goto yyvalid; } while(0)
#define YYVALID_NESTED do { if (yyps->save && \
                                yyps->save->save == 0) goto yyvalid; } while(0)
#endif /* YYBTYACC */

int
YYPARSE_DECL()
{
    int yym, yyn, yystate, yyresult;
#if YYBTYACC
    int yynewerrflag;
    YYParseState *yyerrctx = NULL;
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  yyerror_loc_range[2]; /* position of error start & end */
#endif
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
    if (yydebug)
        fprintf(stderr, "%sdebug[<# of symbols on state stack>]\n", YYPREFIX);
#endif
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    memset(yyerror_loc_range, 0, sizeof(yyerror_loc_range));
#endif

#if YYBTYACC
    yyps = yyNewState(0); if (yyps == 0) goto yyenomem;
    yyps->save = 0;
#endif /* YYBTYACC */
    yym = 0;
    yyn = 0;
    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base;
#endif
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
#if YYBTYACC
        do {
        if (yylvp < yylve)
        {
            /* we're currently re-reading tokens */
            yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc = *yylpp++;
#endif
            yychar = *yylexp++;
            break;
        }
        if (yyps->save)
        {
            /* in trial mode; save scanner results for future parse attempts */
            if (yylvp == yylvlim)
            {   /* Enlarge lexical value queue */
                size_t p = (size_t) (yylvp - yylvals);
                size_t s = (size_t) (yylvlim - yylvals);

                s += YYLVQUEUEGROWTH;
                if ((yylexemes = realloc(yylexemes, s * sizeof(YYINT))) == NULL) goto yyenomem;
                if ((yylvals   = realloc(yylvals, s * sizeof(YYSTYPE))) == NULL) goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                if ((yylpsns   = realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL) goto yyenomem;
#endif
                yylvp   = yylve = yylvals + p;
                yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp   = yylpe = yylpsns + p;
                yylplim = yylpsns + s;
#endif
                yylexp  = yylexemes + p;
            }
            *yylexp = (YYINT) YYLEX;
            *yylvp++ = yylval;
            yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *yylpp++ = yylloc;
            yylpe++;
#endif
            yychar = *yylexp++;
            break;
        }
        /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
        yychar = YYLEX;
#if YYBTYACC
        } while (0);
#endif /* YYBTYACC */
        if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, " <%s>", YYSTYPE_TOSTRING(yychar, yylval));
#endif
            fputc('\n', stderr);
        }
#endif
    }
#if YYBTYACC

    /* Do we have a conflict? */
    if (((yyn = yycindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
        yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        YYINT ctry;

        if (yypath)
        {
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: CONFLICT in state %d: following successful trial parse\n",
                                YYDEBUGSTR, yydepth, yystate);
#endif
            /* Switch to the next conflict context */
            save = yypath;
            yypath = save->save;
            save->save = NULL;
            ctry = save->ctry;
            if (save->state != yystate) YYABORT;
            yyFreeState(save);

        }
        else
        {

            /* Unresolved conflict - start/continue trial parse */
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
            {
                fprintf(stderr, "%s[%d]: CONFLICT in state %d. ", YYDEBUGSTR, yydepth, yystate);
                if (yyps->save)
                    fputs("ALREADY in conflict, continuing trial parse.\n", stderr);
                else
                    fputs("Starting trial parse.\n", stderr);
            }
#endif
            save                  = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (save == NULL) goto yyenomem;
            save->save            = yyps->save;
            save->state           = yystate;
            save->errflag         = yyerrflag;
            save->yystack.s_mark  = save->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (save->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            save->yystack.l_mark  = save->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (save->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            save->yystack.p_mark  = save->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (save->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            ctry                  = yytable[yyn];
            if (yyctable[ctry] == -1)
            {
#if YYDEBUG
                if (yydebug && yychar >= YYEOF)
                    fprintf(stderr, "%s[%d]: backtracking 1 token\n", YYDEBUGSTR, yydepth);
#endif
                ctry++;
            }
            save->ctry = ctry;
            if (yyps->save == NULL)
            {
                /* If this is a first conflict in the stack, start saving lexemes */
                if (!yylexemes)
                {
                    yylexemes = malloc((YYLVQUEUEGROWTH) * sizeof(YYINT));
                    if (yylexemes == NULL) goto yyenomem;
                    yylvals   = (YYSTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYSTYPE));
                    if (yylvals == NULL) goto yyenomem;
                    yylvlim   = yylvals + YYLVQUEUEGROWTH;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpsns   = (YYLTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYLTYPE));
                    if (yylpsns == NULL) goto yyenomem;
                    yylplim   = yylpsns + YYLVQUEUEGROWTH;
#endif
                }
                if (yylvp == yylve)
                {
                    yylvp  = yylve = yylvals;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp  = yylpe = yylpsns;
#endif
                    yylexp = yylexemes;
                    if (yychar >= YYEOF)
                    {
                        *yylve++ = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                        *yylpe++ = yylloc;
#endif
                        *yylexp  = (YYINT) yychar;
                        yychar   = YYEMPTY;
                    }
                }
            }
            if (yychar >= YYEOF)
            {
                yylvp--;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp--;
#endif
                yylexp--;
                yychar = YYEMPTY;
            }
            save->lexeme = (int) (yylvp - yylvals);
            yyps->save   = save;
        }
        if (yytable[yyn] == ctry)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                                YYDEBUGSTR, yydepth, yystate, yyctable[ctry]);
#endif
            if (yychar < 0)
            {
                yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp++;
#endif
                yylexp++;
            }
            if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
                goto yyoverflow;
            yystate = yyctable[ctry];
            *++yystack.s_mark = (YYINT) yystate;
            *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *++yystack.p_mark = yylloc;
#endif
            yychar  = YYEMPTY;
            if (yyerrflag > 0) --yyerrflag;
            goto yyloop;
        }
        else
        {
            yyn = yyctable[ctry];
            goto yyreduce;
        }
    } /* End of code dealing with conflicts */
#endif /* YYBTYACC */
    if (((yyn = yysindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                            YYDEBUGSTR, yydepth, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yylloc;
#endif
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if (((yyn = yyrindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag != 0) goto yyinrecovery;
#if YYBTYACC

    yynewerrflag = 1;
    goto yyerrhandler;
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */

yyerrlab:
    /* explicit YYERROR from an action -- pop the rhs of the rule reduced
     * before looking for error recovery */
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif

    yynewerrflag = 0;
yyerrhandler:
    while (yyps->save)
    {
        int ctry;
        YYParseState *save = yyps->save;
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: ERROR in state %d, CONFLICT BACKTRACKING to state %d, %d tokens\n",
                            YYDEBUGSTR, yydepth, yystate, yyps->save->state,
                    (int)(yylvp - yylvals - yyps->save->lexeme));
#endif
        /* Memorize most forward-looking error state in case it's really an error. */
        if (yyerrctx == NULL || yyerrctx->lexeme < yylvp - yylvals)
        {
            /* Free old saved error context state */
            if (yyerrctx) yyFreeState(yyerrctx);
            /* Create and fill out new saved error context state */
            yyerrctx                 = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (yyerrctx == NULL) goto yyenomem;
            yyerrctx->save           = yyps->save;
            yyerrctx->state          = yystate;
            yyerrctx->errflag        = yyerrflag;
            yyerrctx->yystack.s_mark = yyerrctx->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (yyerrctx->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yyerrctx->yystack.l_mark = yyerrctx->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (yyerrctx->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yyerrctx->yystack.p_mark = yyerrctx->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (yyerrctx->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yyerrctx->lexeme         = (int) (yylvp - yylvals);
        }
        yylvp          = yylvals   + save->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yylpp          = yylpsns   + save->lexeme;
#endif
        yylexp         = yylexemes + save->lexeme;
        yychar         = YYEMPTY;
        yystack.s_mark = yystack.s_base + (save->yystack.s_mark - save->yystack.s_base);
        memcpy (yystack.s_base, save->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
        yystack.l_mark = yystack.l_base + (save->yystack.l_mark - save->yystack.l_base);
        memcpy (yystack.l_base, save->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yystack.p_mark = yystack.p_base + (save->yystack.p_mark - save->yystack.p_base);
        memcpy (yystack.p_base, save->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
        ctry           = ++save->ctry;
        yystate        = save->state;
        /* We tried shift, try reduce now */
        if ((yyn = yyctable[ctry]) >= 0) goto yyreduce;
        yyps->save     = save->save;
        save->save     = NULL;
        yyFreeState(save);

        /* Nothing left on the stack -- error */
        if (!yyps->save)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%sdebug[%d,trial]: trial parse FAILED, entering ERROR mode\n",
                                YYPREFIX, yydepth);
#endif
            /* Restore state as it was in the most forward-advanced error */
            yylvp          = yylvals   + yyerrctx->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylpp          = yylpsns   + yyerrctx->lexeme;
#endif
            yylexp         = yylexemes + yyerrctx->lexeme;
            yychar         = yylexp[-1];
            yylval         = yylvp[-1];
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc         = yylpp[-1];
#endif
            yystack.s_mark = yystack.s_base + (yyerrctx->yystack.s_mark - yyerrctx->yystack.s_base);
            memcpy (yystack.s_base, yyerrctx->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yystack.l_mark = yystack.l_base + (yyerrctx->yystack.l_mark - yyerrctx->yystack.l_base);
            memcpy (yystack.l_base, yyerrctx->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yystack.p_mark = yystack.p_base + (yyerrctx->yystack.p_mark - yyerrctx->yystack.p_base);
            memcpy (yystack.p_base, yyerrctx->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yystate        = yyerrctx->state;
            yyFreeState(yyerrctx);
            yyerrctx       = NULL;
        }
        yynewerrflag = 1;
    }
    if (yynewerrflag == 0) goto yyinrecovery;
#endif /* YYBTYACC */

    YYERROR_CALL("syntax error");
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yyerror_loc_range[0] = yylloc; /* lookahead position is error start position */
#endif

#if !YYBTYACC
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */
yyerrlab:
#endif
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if (((yyn = yysindex[*yystack.s_mark]) != 0) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: state %d, error recovery shifting to state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* lookahead position is error end position */
                yyerror_loc_range[1] = yylloc;
                YYLLOC_DEFAULT(yyloc, yyerror_loc_range, 2); /* position of error span */
                *++yystack.p_mark = yyloc;
#endif
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: error recovery discarding state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* the current TOS position is the error start position */
                yyerror_loc_range[0] = *yystack.p_mark;
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
                if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark, yystack.p_mark);
#else
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
                --yystack.s_mark;
                --yystack.l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                --yystack.p_mark;
#endif
            }
        }
    }
    else
    {
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, error recovery discarding token %d (%s)\n",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
        }
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval, &yylloc);
#else
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
    yym = yylen[yyn];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: state %d, reducing by rule %d (%s)",
                        YYDEBUGSTR, yydepth, yystate, yyn, yyrule[yyn]);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            if (yym > 0)
            {
                int i;
                fputc('<', stderr);
                for (i = yym; i > 0; i--)
                {
                    if (i != yym) fputs(", ", stderr);
                    fputs(YYSTYPE_TOSTRING(yystos[yystack.s_mark[1-i]],
                                           yystack.l_mark[1-i]), stderr);
                }
                fputc('>', stderr);
            }
#endif
        fputc('\n', stderr);
    }
#endif
    if (yym > 0)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)

    /* Perform position reduction */
    memset(&yyloc, 0, sizeof(yyloc));
#if YYBTYACC
    if (!yytrial)
#endif /* YYBTYACC */
    {
        YYLLOC_DEFAULT(yyloc, &yystack.p_mark[1-yym], yym);
        /* just in case YYERROR is invoked within the action, save
           the start of the rhs as the error start position */
        yyerror_loc_range[0] = yystack.p_mark[1-yym];
    }
#endif

    switch (yyn)
    {
case 5:
#line 169 "parser.y"
	{
			add_wcs(yystack.l_mark[0].wc);
		}
break;
case 6:
#line 173 "parser.y"
	{
			add_wcs(yystack.l_mark[0].wc);
		}
break;
case 9:
#line 184 "parser.y"
	{
			com_char = yystack.l_mark[-1].wc;
		}
break;
case 10:
#line 188 "parser.y"
	{
			esc_char = yystack.l_mark[-1].wc;
		}
break;
case 11:
#line 192 "parser.y"
	{
			mb_cur_max = yystack.l_mark[-1].num;
		}
break;
case 12:
#line 196 "parser.y"
	{
			mb_cur_min = yystack.l_mark[-1].num;
		}
break;
case 13:
#line 200 "parser.y"
	{
			wchar_t *w = get_wcs();
			set_wide_encoding(to_mb_string(w));
			free(w);
		}
break;
case 14:
#line 206 "parser.y"
	{
			set_wide_encoding(yystack.l_mark[-1].token);
		}
break;
case 15:
#line 212 "parser.y"
	{
			copy_category(yystack.l_mark[-1].token);
		}
break;
case 16:
#line 216 "parser.y"
	{
			wchar_t *w = get_wcs();
			copy_category(to_mb_string(w));
			free(w);
		}
break;
case 30:
#line 249 "parser.y"
	{
			add_charmap(yystack.l_mark[-1].token, yystack.l_mark[0].wc);
			scan_to_eol();
		}
break;
case 31:
#line 254 "parser.y"
	{
			add_charmap_range(yystack.l_mark[-3].token, yystack.l_mark[-1].token, yystack.l_mark[0].wc);
			scan_to_eol();
		}
break;
case 35:
#line 266 "parser.y"
	{
			add_width(yystack.l_mark[-2].wc, yystack.l_mark[-1].num);
		}
break;
case 36:
#line 270 "parser.y"
	{
			add_charmap_undefined(yystack.l_mark[-2].token);
		}
break;
case 37:
#line 274 "parser.y"
	{
			add_width_range(yystack.l_mark[-4].wc, yystack.l_mark[-2].wc, yystack.l_mark[-1].num);
		}
break;
case 38:
#line 278 "parser.y"
	{
			add_charmap_undefined(yystack.l_mark[-4].token);
			add_charmap_undefined(yystack.l_mark[-2].token);
		}
break;
case 39:
#line 283 "parser.y"
	{
			add_width(yystack.l_mark[-4].wc, yystack.l_mark[-1].num);
			add_charmap_undefined(yystack.l_mark[-2].token);
		}
break;
case 40:
#line 288 "parser.y"
	{
			add_width(yystack.l_mark[-2].wc, yystack.l_mark[-1].num);
			add_charmap_undefined(yystack.l_mark[-4].token);
		}
break;
case 42:
#line 296 "parser.y"
	{
			dump_ctype();
		}
break;
case 65:
#line 326 "parser.y"
	{
			wchar_t *w = get_wcs();
			set_variable(to_mb_string(w));
			free(w);
		}
break;
case 69:
#line 339 "parser.y"
	{
			add_ctype_range(yystack.l_mark[0].wc);
		}
break;
case 70:
#line 345 "parser.y"
	{
			add_ctype(yystack.l_mark[0].wc);
		}
break;
case 71:
#line 349 "parser.y"
	{
			add_charmap_undefined(yystack.l_mark[0].token);
		}
break;
case 74:
#line 360 "parser.y"
	{
			add_caseconv(yystack.l_mark[-3].wc, yystack.l_mark[-1].wc);
		}
break;
case 75:
#line 364 "parser.y"
	{
			add_charmap_undefined(yystack.l_mark[-3].token);
		}
break;
case 76:
#line 368 "parser.y"
	{
			add_charmap_undefined(yystack.l_mark[-3].token);
			add_charmap_undefined(yystack.l_mark[-1].token);
		}
break;
case 77:
#line 373 "parser.y"
	{
			add_charmap_undefined(yystack.l_mark[-1].token);
		}
break;
case 78:
#line 379 "parser.y"
	{
			dump_collate();
		}
break;
case 79:
#line 383 "parser.y"
	{
			dump_collate();
		}
break;
case 85:
#line 398 "parser.y"
	{
			define_collsym(yystack.l_mark[-1].token);
		}
break;
case 86:
#line 405 "parser.y"
	{
			define_collelem(yystack.l_mark[-3].token, get_wcs());
		}
break;
case 87:
#line 411 "parser.y"
	{
			/* If no order list supplied default to one forward */
			add_order_bit(T_FORWARD);
			add_order_directive();
		}
break;
case 89:
#line 421 "parser.y"
	{
			add_order_directive();
		}
break;
case 90:
#line 425 "parser.y"
	{
			add_order_directive();
		}
break;
case 93:
#line 435 "parser.y"
	{
			add_order_bit(T_FORWARD);
		}
break;
case 94:
#line 439 "parser.y"
	{
			add_order_bit(T_BACKWARD);
		}
break;
case 95:
#line 443 "parser.y"
	{
			add_order_bit(T_POSITION);
		}
break;
case 98:
#line 453 "parser.y"
	{
			end_order_collsym(yystack.l_mark[-1].collsym);
		}
break;
case 99:
#line 457 "parser.y"
	{
			end_order();
		}
break;
case 100:
#line 461 "parser.y"
	{
			end_order();
		}
break;
case 101:
#line 467 "parser.y"
	{
			start_order_char(yystack.l_mark[0].wc);
		}
break;
case 102:
#line 471 "parser.y"
	{
			start_order_ellipsis();
		}
break;
case 103:
#line 475 "parser.y"
	{
			start_order_collelem(yystack.l_mark[0].collelem);
		}
break;
case 104:
#line 479 "parser.y"
	{
			start_order_undefined();
		}
break;
case 105:
#line 483 "parser.y"
	{
			start_order_symbol(yystack.l_mark[0].token);
		}
break;
case 109:
#line 494 "parser.y"
	{
			add_order_collelem(yystack.l_mark[0].collelem);
		}
break;
case 110:
#line 498 "parser.y"
	{
			add_order_collsym(yystack.l_mark[0].collsym);
		}
break;
case 111:
#line 502 "parser.y"
	{
			add_order_char(yystack.l_mark[0].wc);
		}
break;
case 112:
#line 506 "parser.y"
	{
			add_order_ellipsis();
		}
break;
case 113:
#line 510 "parser.y"
	{
			add_order_ignore();
		}
break;
case 114:
#line 514 "parser.y"
	{
			add_order_symbol(yystack.l_mark[0].token);
		}
break;
case 115:
#line 518 "parser.y"
	{
			add_order_subst();
		}
break;
case 118:
#line 528 "parser.y"
	{
			add_subst_char(yystack.l_mark[0].wc);
		}
break;
case 119:
#line 532 "parser.y"
	{
			add_subst_collsym(yystack.l_mark[0].collsym);
		}
break;
case 120:
#line 536 "parser.y"
	{
			add_subst_collelem(yystack.l_mark[0].collelem);
		}
break;
case 121:
#line 540 "parser.y"
	{
			add_subst_symbol(yystack.l_mark[0].token);
		}
break;
case 122:
#line 546 "parser.y"
	{
			dump_messages();
		}
break;
case 130:
#line 563 "parser.y"
	{
			add_message(get_wcs());
		}
break;
case 131:
#line 569 "parser.y"
	{
			dump_monetary();
		}
break;
case 155:
#line 604 "parser.y"
	{
			add_monetary_str(get_wcs());
		}
break;
case 156:
#line 608 "parser.y"
	{
			add_monetary_num(yystack.l_mark[-1].num);
		}
break;
case 158:
#line 615 "parser.y"
	{
			reset_monetary_group();
			add_monetary_group(yystack.l_mark[0].num);
		}
break;
case 159:
#line 620 "parser.y"
	{
			add_monetary_group(yystack.l_mark[0].num);
		}
break;
case 160:
#line 627 "parser.y"
	{
			dump_numeric();
		}
break;
case 164:
#line 640 "parser.y"
	{
			add_numeric_str(get_wcs());
		}
break;
case 168:
#line 652 "parser.y"
	{
			reset_numeric_group();
			add_numeric_group(yystack.l_mark[0].num);
		}
break;
case 169:
#line 657 "parser.y"
	{
			add_numeric_group(yystack.l_mark[0].num);
		}
break;
case 170:
#line 664 "parser.y"
	{
			dump_time();
		}
break;
case 174:
#line 675 "parser.y"
	{
			add_time_str(get_wcs());
		}
break;
case 175:
#line 679 "parser.y"
	{
			check_time_list();
		}
break;
case 191:
#line 704 "parser.y"
	{
			add_time_list(get_wcs());
		}
break;
case 192:
#line 708 "parser.y"
	{
			reset_time_list();
			add_time_list(get_wcs());
		}
break;
#line 2127 "parser.c"
    default:
        break;
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
        {
            fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[YYFINAL], yyval));
#endif
            fprintf(stderr, "shifting from state 0 to final state %d\n", YYFINAL);
        }
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yyloc;
#endif
        if (yychar < 0)
        {
#if YYBTYACC
            do {
            if (yylvp < yylve)
            {
                /* we're currently re-reading tokens */
                yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylloc = *yylpp++;
#endif
                yychar = *yylexp++;
                break;
            }
            if (yyps->save)
            {
                /* in trial mode; save scanner results for future parse attempts */
                if (yylvp == yylvlim)
                {   /* Enlarge lexical value queue */
                    size_t p = (size_t) (yylvp - yylvals);
                    size_t s = (size_t) (yylvlim - yylvals);

                    s += YYLVQUEUEGROWTH;
                    if ((yylexemes = realloc(yylexemes, s * sizeof(YYINT))) == NULL)
                        goto yyenomem;
                    if ((yylvals   = realloc(yylvals, s * sizeof(YYSTYPE))) == NULL)
                        goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    if ((yylpsns   = realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL)
                        goto yyenomem;
#endif
                    yylvp   = yylve = yylvals + p;
                    yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp   = yylpe = yylpsns + p;
                    yylplim = yylpsns + s;
#endif
                    yylexp  = yylexemes + p;
                }
                *yylexp = (YYINT) YYLEX;
                *yylvp++ = yylval;
                yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                *yylpp++ = yylloc;
                yylpe++;
#endif
                yychar = *yylexp++;
                break;
            }
            /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
            yychar = YYLEX;
#if YYBTYACC
            } while (0);
#endif /* YYBTYACC */
            if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
                fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)\n",
                                YYDEBUGSTR, yydepth, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
        goto yyloop;
    }
    if (((yyn = yygindex[yym]) != 0) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[yystate], yyval));
#endif
        fprintf(stderr, "shifting from state %d to state %d\n", *yystack.s_mark, yystate);
    }
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    *++yystack.p_mark = yyloc;
#endif
    goto yyloop;
#if YYBTYACC

    /* Reduction declares that this path is valid. Set yypath and do a full parse */
yyvalid:
    if (yypath) YYABORT;
    while (yyps->save)
    {
        YYParseState *save = yyps->save;
        yyps->save = save->save;
        save->save = yypath;
        yypath = save;
    }
#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%s[%d]: state %d, CONFLICT trial successful, backtracking to state %d, %d tokens\n",
                        YYDEBUGSTR, yydepth, yystate, yypath->state, (int)(yylvp - yylvals - yypath->lexeme));
#endif
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    yylvp          = yylvals + yypath->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yylpp          = yylpsns + yypath->lexeme;
#endif
    yylexp         = yylexemes + yypath->lexeme;
    yychar         = YYEMPTY;
    yystack.s_mark = yystack.s_base + (yypath->yystack.s_mark - yypath->yystack.s_base);
    memcpy (yystack.s_base, yypath->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
    yystack.l_mark = yystack.l_base + (yypath->yystack.l_mark - yypath->yystack.l_base);
    memcpy (yystack.l_base, yypath->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base + (yypath->yystack.p_mark - yypath->yystack.p_base);
    memcpy (yystack.p_base, yypath->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
    yystate        = yypath->state;
    goto yyloop;
#endif /* YYBTYACC */

yyoverflow:
    YYERROR_CALL("yacc stack overflow");
#if YYBTYACC
    goto yyabort_nomem;
yyenomem:
    YYERROR_CALL("memory exhausted");
yyabort_nomem:
#endif /* YYBTYACC */
    yyresult = 2;
    goto yyreturn;

yyabort:
    yyresult = 1;
    goto yyreturn;

yyaccept:
#if YYBTYACC
    if (yyps->save) goto yyvalid;
#endif /* YYBTYACC */
    yyresult = 0;

yyreturn:
#if defined(YYDESTRUCT_CALL)
    if (yychar != YYEOF && yychar != YYEMPTY)
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval, &yylloc);
#else
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */

    {
        YYSTYPE *pv;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYLTYPE *pp;

        for (pv = yystack.l_base, pp = yystack.p_base; pv <= yystack.l_mark; ++pv, ++pp)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv, pp);
#else
        for (pv = yystack.l_base; pv <= yystack.l_mark; ++pv)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
    }
#endif /* defined(YYDESTRUCT_CALL) */

#if YYBTYACC
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    while (yyps)
    {
        YYParseState *save = yyps;
        yyps = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
    while (yypath)
    {
        YYParseState *save = yypath;
        yypath = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
#endif /* YYBTYACC */
    yyfreestack(&yystack);
    return (yyresult);
}
