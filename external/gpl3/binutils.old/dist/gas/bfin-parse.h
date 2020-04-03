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

#ifndef YY_YY_BFIN_PARSE_H_INCLUDED
# define YY_YY_BFIN_PARSE_H_INCLUDED
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
    BYTEOP16P = 258,
    BYTEOP16M = 259,
    BYTEOP1P = 260,
    BYTEOP2P = 261,
    BYTEOP3P = 262,
    BYTEUNPACK = 263,
    BYTEPACK = 264,
    PACK = 265,
    SAA = 266,
    ALIGN8 = 267,
    ALIGN16 = 268,
    ALIGN24 = 269,
    VIT_MAX = 270,
    EXTRACT = 271,
    DEPOSIT = 272,
    EXPADJ = 273,
    SEARCH = 274,
    ONES = 275,
    SIGN = 276,
    SIGNBITS = 277,
    LINK = 278,
    UNLINK = 279,
    REG = 280,
    PC = 281,
    CCREG = 282,
    BYTE_DREG = 283,
    REG_A_DOUBLE_ZERO = 284,
    REG_A_DOUBLE_ONE = 285,
    A_ZERO_DOT_L = 286,
    A_ZERO_DOT_H = 287,
    A_ONE_DOT_L = 288,
    A_ONE_DOT_H = 289,
    HALF_REG = 290,
    NOP = 291,
    RTI = 292,
    RTS = 293,
    RTX = 294,
    RTN = 295,
    RTE = 296,
    HLT = 297,
    IDLE = 298,
    STI = 299,
    CLI = 300,
    CSYNC = 301,
    SSYNC = 302,
    EMUEXCPT = 303,
    RAISE = 304,
    EXCPT = 305,
    LSETUP = 306,
    LOOP = 307,
    LOOP_BEGIN = 308,
    LOOP_END = 309,
    DISALGNEXCPT = 310,
    JUMP = 311,
    JUMP_DOT_S = 312,
    JUMP_DOT_L = 313,
    CALL = 314,
    ABORT = 315,
    NOT = 316,
    TILDA = 317,
    BANG = 318,
    AMPERSAND = 319,
    BAR = 320,
    PERCENT = 321,
    CARET = 322,
    BXOR = 323,
    MINUS = 324,
    PLUS = 325,
    STAR = 326,
    SLASH = 327,
    NEG = 328,
    MIN = 329,
    MAX = 330,
    ABS = 331,
    DOUBLE_BAR = 332,
    _PLUS_BAR_PLUS = 333,
    _PLUS_BAR_MINUS = 334,
    _MINUS_BAR_PLUS = 335,
    _MINUS_BAR_MINUS = 336,
    _MINUS_MINUS = 337,
    _PLUS_PLUS = 338,
    SHIFT = 339,
    LSHIFT = 340,
    ASHIFT = 341,
    BXORSHIFT = 342,
    _GREATER_GREATER_GREATER_THAN_ASSIGN = 343,
    ROT = 344,
    LESS_LESS = 345,
    GREATER_GREATER = 346,
    _GREATER_GREATER_GREATER = 347,
    _LESS_LESS_ASSIGN = 348,
    _GREATER_GREATER_ASSIGN = 349,
    DIVS = 350,
    DIVQ = 351,
    ASSIGN = 352,
    _STAR_ASSIGN = 353,
    _BAR_ASSIGN = 354,
    _CARET_ASSIGN = 355,
    _AMPERSAND_ASSIGN = 356,
    _MINUS_ASSIGN = 357,
    _PLUS_ASSIGN = 358,
    _ASSIGN_BANG = 359,
    _LESS_THAN_ASSIGN = 360,
    _ASSIGN_ASSIGN = 361,
    GE = 362,
    LT = 363,
    LE = 364,
    GT = 365,
    LESS_THAN = 366,
    FLUSHINV = 367,
    FLUSH = 368,
    IFLUSH = 369,
    PREFETCH = 370,
    PRNT = 371,
    OUTC = 372,
    WHATREG = 373,
    TESTSET = 374,
    ASL = 375,
    ASR = 376,
    B = 377,
    W = 378,
    NS = 379,
    S = 380,
    CO = 381,
    SCO = 382,
    TH = 383,
    TL = 384,
    BP = 385,
    BREV = 386,
    X = 387,
    Z = 388,
    M = 389,
    MMOD = 390,
    R = 391,
    RND = 392,
    RNDL = 393,
    RNDH = 394,
    RND12 = 395,
    RND20 = 396,
    V = 397,
    LO = 398,
    HI = 399,
    BITTGL = 400,
    BITCLR = 401,
    BITSET = 402,
    BITTST = 403,
    BITMUX = 404,
    DBGAL = 405,
    DBGAH = 406,
    DBGHALT = 407,
    DBG = 408,
    DBGA = 409,
    DBGCMPLX = 410,
    IF = 411,
    COMMA = 412,
    BY = 413,
    COLON = 414,
    SEMICOLON = 415,
    RPAREN = 416,
    LPAREN = 417,
    LBRACK = 418,
    RBRACK = 419,
    STATUS_REG = 420,
    MNOP = 421,
    SYMBOL = 422,
    NUMBER = 423,
    GOT = 424,
    GOT17M4 = 425,
    FUNCDESC_GOT17M4 = 426,
    AT = 427,
    PLTPC = 428
  };
#endif
/* Tokens.  */
#define BYTEOP16P 258
#define BYTEOP16M 259
#define BYTEOP1P 260
#define BYTEOP2P 261
#define BYTEOP3P 262
#define BYTEUNPACK 263
#define BYTEPACK 264
#define PACK 265
#define SAA 266
#define ALIGN8 267
#define ALIGN16 268
#define ALIGN24 269
#define VIT_MAX 270
#define EXTRACT 271
#define DEPOSIT 272
#define EXPADJ 273
#define SEARCH 274
#define ONES 275
#define SIGN 276
#define SIGNBITS 277
#define LINK 278
#define UNLINK 279
#define REG 280
#define PC 281
#define CCREG 282
#define BYTE_DREG 283
#define REG_A_DOUBLE_ZERO 284
#define REG_A_DOUBLE_ONE 285
#define A_ZERO_DOT_L 286
#define A_ZERO_DOT_H 287
#define A_ONE_DOT_L 288
#define A_ONE_DOT_H 289
#define HALF_REG 290
#define NOP 291
#define RTI 292
#define RTS 293
#define RTX 294
#define RTN 295
#define RTE 296
#define HLT 297
#define IDLE 298
#define STI 299
#define CLI 300
#define CSYNC 301
#define SSYNC 302
#define EMUEXCPT 303
#define RAISE 304
#define EXCPT 305
#define LSETUP 306
#define LOOP 307
#define LOOP_BEGIN 308
#define LOOP_END 309
#define DISALGNEXCPT 310
#define JUMP 311
#define JUMP_DOT_S 312
#define JUMP_DOT_L 313
#define CALL 314
#define ABORT 315
#define NOT 316
#define TILDA 317
#define BANG 318
#define AMPERSAND 319
#define BAR 320
#define PERCENT 321
#define CARET 322
#define BXOR 323
#define MINUS 324
#define PLUS 325
#define STAR 326
#define SLASH 327
#define NEG 328
#define MIN 329
#define MAX 330
#define ABS 331
#define DOUBLE_BAR 332
#define _PLUS_BAR_PLUS 333
#define _PLUS_BAR_MINUS 334
#define _MINUS_BAR_PLUS 335
#define _MINUS_BAR_MINUS 336
#define _MINUS_MINUS 337
#define _PLUS_PLUS 338
#define SHIFT 339
#define LSHIFT 340
#define ASHIFT 341
#define BXORSHIFT 342
#define _GREATER_GREATER_GREATER_THAN_ASSIGN 343
#define ROT 344
#define LESS_LESS 345
#define GREATER_GREATER 346
#define _GREATER_GREATER_GREATER 347
#define _LESS_LESS_ASSIGN 348
#define _GREATER_GREATER_ASSIGN 349
#define DIVS 350
#define DIVQ 351
#define ASSIGN 352
#define _STAR_ASSIGN 353
#define _BAR_ASSIGN 354
#define _CARET_ASSIGN 355
#define _AMPERSAND_ASSIGN 356
#define _MINUS_ASSIGN 357
#define _PLUS_ASSIGN 358
#define _ASSIGN_BANG 359
#define _LESS_THAN_ASSIGN 360
#define _ASSIGN_ASSIGN 361
#define GE 362
#define LT 363
#define LE 364
#define GT 365
#define LESS_THAN 366
#define FLUSHINV 367
#define FLUSH 368
#define IFLUSH 369
#define PREFETCH 370
#define PRNT 371
#define OUTC 372
#define WHATREG 373
#define TESTSET 374
#define ASL 375
#define ASR 376
#define B 377
#define W 378
#define NS 379
#define S 380
#define CO 381
#define SCO 382
#define TH 383
#define TL 384
#define BP 385
#define BREV 386
#define X 387
#define Z 388
#define M 389
#define MMOD 390
#define R 391
#define RND 392
#define RNDL 393
#define RNDH 394
#define RND12 395
#define RND20 396
#define V 397
#define LO 398
#define HI 399
#define BITTGL 400
#define BITCLR 401
#define BITSET 402
#define BITTST 403
#define BITMUX 404
#define DBGAL 405
#define DBGAH 406
#define DBGHALT 407
#define DBG 408
#define DBGA 409
#define DBGCMPLX 410
#define IF 411
#define COMMA 412
#define BY 413
#define COLON 414
#define SEMICOLON 415
#define RPAREN 416
#define LPAREN 417
#define LBRACK 418
#define RBRACK 419
#define STATUS_REG 420
#define MNOP 421
#define SYMBOL 422
#define NUMBER 423
#define GOT 424
#define GOT17M4 425
#define FUNCDESC_GOT17M4 426
#define AT 427
#define PLTPC 428

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 444 "./config/bfin-parse.y" /* yacc.c:1909  */

  INSTR_T instr;
  Expr_Node *expr;
  SYMBOL_T symbol;
  long value;
  Register reg;
  Macfunc macfunc;
  struct { int r0; int s0; int x0; int aop; } modcodes;
  struct { int r0; } r0;
  Opt_mode mod;

#line 412 "bfin-parse.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_BFIN_PARSE_H_INCLUDED  */
