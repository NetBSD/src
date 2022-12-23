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

#ifndef YY_RX_RX_PARSE_H_INCLUDED
# define YY_RX_RX_PARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int rx_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    REG = 258,
    FLAG = 259,
    CREG = 260,
    ACC = 261,
    DREG = 262,
    DREGH = 263,
    DREGL = 264,
    DCREG = 265,
    EXPR = 266,
    UNKNOWN_OPCODE = 267,
    IS_OPCODE = 268,
    DOT_S = 269,
    DOT_B = 270,
    DOT_W = 271,
    DOT_L = 272,
    DOT_A = 273,
    DOT_UB = 274,
    DOT_UW = 275,
    DOT_D = 276,
    ABS = 277,
    ADC = 278,
    ADD = 279,
    AND_ = 280,
    BCLR = 281,
    BCND = 282,
    BFMOV = 283,
    BFMOVZ = 284,
    BMCND = 285,
    BNOT = 286,
    BRA = 287,
    BRK = 288,
    BSET = 289,
    BSR = 290,
    BTST = 291,
    CLRPSW = 292,
    CMP = 293,
    DABS = 294,
    DADD = 295,
    DBT = 296,
    DCMP = 297,
    DDIV = 298,
    DIV = 299,
    DIVU = 300,
    DMOV = 301,
    DMUL = 302,
    DNEG = 303,
    DPOPM = 304,
    DPUSHM = 305,
    DROUND = 306,
    DSQRT = 307,
    DSUB = 308,
    DTOF = 309,
    DTOI = 310,
    DTOU = 311,
    EDIV = 312,
    EDIVU = 313,
    EMACA = 314,
    EMSBA = 315,
    EMUL = 316,
    EMULA = 317,
    EMULU = 318,
    FADD = 319,
    FCMP = 320,
    FDIV = 321,
    FMUL = 322,
    FREIT = 323,
    FSUB = 324,
    FSQRT = 325,
    FTOD = 326,
    FTOI = 327,
    FTOU = 328,
    INT = 329,
    ITOD = 330,
    ITOF = 331,
    JMP = 332,
    JSR = 333,
    MACHI = 334,
    MACLH = 335,
    MACLO = 336,
    MAX = 337,
    MIN = 338,
    MOV = 339,
    MOVCO = 340,
    MOVLI = 341,
    MOVU = 342,
    MSBHI = 343,
    MSBLH = 344,
    MSBLO = 345,
    MUL = 346,
    MULHI = 347,
    MULLH = 348,
    MULLO = 349,
    MULU = 350,
    MVFACHI = 351,
    MVFACGU = 352,
    MVFACMI = 353,
    MVFACLO = 354,
    MVFC = 355,
    MVFDC = 356,
    MVFDR = 357,
    MVTACGU = 358,
    MVTACHI = 359,
    MVTACLO = 360,
    MVTC = 361,
    MVTDC = 362,
    MVTIPL = 363,
    NEG = 364,
    NOP = 365,
    NOT = 366,
    OR = 367,
    POP = 368,
    POPC = 369,
    POPM = 370,
    PUSH = 371,
    PUSHA = 372,
    PUSHC = 373,
    PUSHM = 374,
    RACL = 375,
    RACW = 376,
    RDACL = 377,
    RDACW = 378,
    REIT = 379,
    REVL = 380,
    REVW = 381,
    RMPA = 382,
    ROLC = 383,
    RORC = 384,
    ROTL = 385,
    ROTR = 386,
    ROUND = 387,
    RSTR = 388,
    RTE = 389,
    RTFI = 390,
    RTS = 391,
    RTSD = 392,
    SAT = 393,
    SATR = 394,
    SAVE = 395,
    SBB = 396,
    SCCND = 397,
    SCMPU = 398,
    SETPSW = 399,
    SHAR = 400,
    SHLL = 401,
    SHLR = 402,
    SMOVB = 403,
    SMOVF = 404,
    SMOVU = 405,
    SSTR = 406,
    STNZ = 407,
    STOP = 408,
    STZ = 409,
    SUB = 410,
    SUNTIL = 411,
    SWHILE = 412,
    TST = 413,
    UTOD = 414,
    UTOF = 415,
    WAIT = 416,
    XCHG = 417,
    XOR = 418
  };
#endif
/* Tokens.  */
#define REG 258
#define FLAG 259
#define CREG 260
#define ACC 261
#define DREG 262
#define DREGH 263
#define DREGL 264
#define DCREG 265
#define EXPR 266
#define UNKNOWN_OPCODE 267
#define IS_OPCODE 268
#define DOT_S 269
#define DOT_B 270
#define DOT_W 271
#define DOT_L 272
#define DOT_A 273
#define DOT_UB 274
#define DOT_UW 275
#define DOT_D 276
#define ABS 277
#define ADC 278
#define ADD 279
#define AND_ 280
#define BCLR 281
#define BCND 282
#define BFMOV 283
#define BFMOVZ 284
#define BMCND 285
#define BNOT 286
#define BRA 287
#define BRK 288
#define BSET 289
#define BSR 290
#define BTST 291
#define CLRPSW 292
#define CMP 293
#define DABS 294
#define DADD 295
#define DBT 296
#define DCMP 297
#define DDIV 298
#define DIV 299
#define DIVU 300
#define DMOV 301
#define DMUL 302
#define DNEG 303
#define DPOPM 304
#define DPUSHM 305
#define DROUND 306
#define DSQRT 307
#define DSUB 308
#define DTOF 309
#define DTOI 310
#define DTOU 311
#define EDIV 312
#define EDIVU 313
#define EMACA 314
#define EMSBA 315
#define EMUL 316
#define EMULA 317
#define EMULU 318
#define FADD 319
#define FCMP 320
#define FDIV 321
#define FMUL 322
#define FREIT 323
#define FSUB 324
#define FSQRT 325
#define FTOD 326
#define FTOI 327
#define FTOU 328
#define INT 329
#define ITOD 330
#define ITOF 331
#define JMP 332
#define JSR 333
#define MACHI 334
#define MACLH 335
#define MACLO 336
#define MAX 337
#define MIN 338
#define MOV 339
#define MOVCO 340
#define MOVLI 341
#define MOVU 342
#define MSBHI 343
#define MSBLH 344
#define MSBLO 345
#define MUL 346
#define MULHI 347
#define MULLH 348
#define MULLO 349
#define MULU 350
#define MVFACHI 351
#define MVFACGU 352
#define MVFACMI 353
#define MVFACLO 354
#define MVFC 355
#define MVFDC 356
#define MVFDR 357
#define MVTACGU 358
#define MVTACHI 359
#define MVTACLO 360
#define MVTC 361
#define MVTDC 362
#define MVTIPL 363
#define NEG 364
#define NOP 365
#define NOT 366
#define OR 367
#define POP 368
#define POPC 369
#define POPM 370
#define PUSH 371
#define PUSHA 372
#define PUSHC 373
#define PUSHM 374
#define RACL 375
#define RACW 376
#define RDACL 377
#define RDACW 378
#define REIT 379
#define REVL 380
#define REVW 381
#define RMPA 382
#define ROLC 383
#define RORC 384
#define ROTL 385
#define ROTR 386
#define ROUND 387
#define RSTR 388
#define RTE 389
#define RTFI 390
#define RTS 391
#define RTSD 392
#define SAT 393
#define SATR 394
#define SAVE 395
#define SBB 396
#define SCCND 397
#define SCMPU 398
#define SETPSW 399
#define SHAR 400
#define SHLL 401
#define SHLR 402
#define SMOVB 403
#define SMOVF 404
#define SMOVU 405
#define SSTR 406
#define STNZ 407
#define STOP 408
#define STZ 409
#define SUB 410
#define SUNTIL 411
#define SWHILE 412
#define TST 413
#define UTOD 414
#define UTOF 415
#define WAIT 416
#define XCHG 417
#define XOR 418

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 140 "./config/rx-parse.y" /* yacc.c:1910  */

  int regno;
  expressionS exp;

#line 385 "rx-parse.h" /* yacc.c:1910  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE rx_lval;

int rx_parse (void);

#endif /* !YY_RX_RX_PARSE_H_INCLUDED  */
