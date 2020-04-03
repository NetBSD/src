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
    EXPR = 262,
    UNKNOWN_OPCODE = 263,
    IS_OPCODE = 264,
    DOT_S = 265,
    DOT_B = 266,
    DOT_W = 267,
    DOT_L = 268,
    DOT_A = 269,
    DOT_UB = 270,
    DOT_UW = 271,
    ABS = 272,
    ADC = 273,
    ADD = 274,
    AND_ = 275,
    BCLR = 276,
    BCND = 277,
    BMCND = 278,
    BNOT = 279,
    BRA = 280,
    BRK = 281,
    BSET = 282,
    BSR = 283,
    BTST = 284,
    CLRPSW = 285,
    CMP = 286,
    DBT = 287,
    DIV = 288,
    DIVU = 289,
    EDIV = 290,
    EDIVU = 291,
    EMACA = 292,
    EMSBA = 293,
    EMUL = 294,
    EMULA = 295,
    EMULU = 296,
    FADD = 297,
    FCMP = 298,
    FDIV = 299,
    FMUL = 300,
    FREIT = 301,
    FSUB = 302,
    FSQRT = 303,
    FTOI = 304,
    FTOU = 305,
    INT = 306,
    ITOF = 307,
    JMP = 308,
    JSR = 309,
    MACHI = 310,
    MACLH = 311,
    MACLO = 312,
    MAX = 313,
    MIN = 314,
    MOV = 315,
    MOVCO = 316,
    MOVLI = 317,
    MOVU = 318,
    MSBHI = 319,
    MSBLH = 320,
    MSBLO = 321,
    MUL = 322,
    MULHI = 323,
    MULLH = 324,
    MULLO = 325,
    MULU = 326,
    MVFACHI = 327,
    MVFACGU = 328,
    MVFACMI = 329,
    MVFACLO = 330,
    MVFC = 331,
    MVTACGU = 332,
    MVTACHI = 333,
    MVTACLO = 334,
    MVTC = 335,
    MVTIPL = 336,
    NEG = 337,
    NOP = 338,
    NOT = 339,
    OR = 340,
    POP = 341,
    POPC = 342,
    POPM = 343,
    PUSH = 344,
    PUSHA = 345,
    PUSHC = 346,
    PUSHM = 347,
    RACL = 348,
    RACW = 349,
    RDACL = 350,
    RDACW = 351,
    REIT = 352,
    REVL = 353,
    REVW = 354,
    RMPA = 355,
    ROLC = 356,
    RORC = 357,
    ROTL = 358,
    ROTR = 359,
    ROUND = 360,
    RTE = 361,
    RTFI = 362,
    RTS = 363,
    RTSD = 364,
    SAT = 365,
    SATR = 366,
    SBB = 367,
    SCCND = 368,
    SCMPU = 369,
    SETPSW = 370,
    SHAR = 371,
    SHLL = 372,
    SHLR = 373,
    SMOVB = 374,
    SMOVF = 375,
    SMOVU = 376,
    SSTR = 377,
    STNZ = 378,
    STOP = 379,
    STZ = 380,
    SUB = 381,
    SUNTIL = 382,
    SWHILE = 383,
    TST = 384,
    UTOF = 385,
    WAIT = 386,
    XCHG = 387,
    XOR = 388
  };
#endif
/* Tokens.  */
#define REG 258
#define FLAG 259
#define CREG 260
#define ACC 261
#define EXPR 262
#define UNKNOWN_OPCODE 263
#define IS_OPCODE 264
#define DOT_S 265
#define DOT_B 266
#define DOT_W 267
#define DOT_L 268
#define DOT_A 269
#define DOT_UB 270
#define DOT_UW 271
#define ABS 272
#define ADC 273
#define ADD 274
#define AND_ 275
#define BCLR 276
#define BCND 277
#define BMCND 278
#define BNOT 279
#define BRA 280
#define BRK 281
#define BSET 282
#define BSR 283
#define BTST 284
#define CLRPSW 285
#define CMP 286
#define DBT 287
#define DIV 288
#define DIVU 289
#define EDIV 290
#define EDIVU 291
#define EMACA 292
#define EMSBA 293
#define EMUL 294
#define EMULA 295
#define EMULU 296
#define FADD 297
#define FCMP 298
#define FDIV 299
#define FMUL 300
#define FREIT 301
#define FSUB 302
#define FSQRT 303
#define FTOI 304
#define FTOU 305
#define INT 306
#define ITOF 307
#define JMP 308
#define JSR 309
#define MACHI 310
#define MACLH 311
#define MACLO 312
#define MAX 313
#define MIN 314
#define MOV 315
#define MOVCO 316
#define MOVLI 317
#define MOVU 318
#define MSBHI 319
#define MSBLH 320
#define MSBLO 321
#define MUL 322
#define MULHI 323
#define MULLH 324
#define MULLO 325
#define MULU 326
#define MVFACHI 327
#define MVFACGU 328
#define MVFACMI 329
#define MVFACLO 330
#define MVFC 331
#define MVTACGU 332
#define MVTACHI 333
#define MVTACLO 334
#define MVTC 335
#define MVTIPL 336
#define NEG 337
#define NOP 338
#define NOT 339
#define OR 340
#define POP 341
#define POPC 342
#define POPM 343
#define PUSH 344
#define PUSHA 345
#define PUSHC 346
#define PUSHM 347
#define RACL 348
#define RACW 349
#define RDACL 350
#define RDACW 351
#define REIT 352
#define REVL 353
#define REVW 354
#define RMPA 355
#define ROLC 356
#define RORC 357
#define ROTL 358
#define ROTR 359
#define ROUND 360
#define RTE 361
#define RTFI 362
#define RTS 363
#define RTSD 364
#define SAT 365
#define SATR 366
#define SBB 367
#define SCCND 368
#define SCMPU 369
#define SETPSW 370
#define SHAR 371
#define SHLL 372
#define SHLR 373
#define SMOVB 374
#define SMOVF 375
#define SMOVU 376
#define SSTR 377
#define STNZ 378
#define STOP 379
#define STZ 380
#define SUB 381
#define SUNTIL 382
#define SWHILE 383
#define TST 384
#define UTOF 385
#define WAIT 386
#define XCHG 387
#define XOR 388

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 135 "./config/rx-parse.y" /* yacc.c:1909  */

  int regno;
  expressionS exp;

#line 325 "rx-parse.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE rx_lval;

int rx_parse (void);

#endif /* !YY_RX_RX_PARSE_H_INCLUDED  */
