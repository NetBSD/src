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

#ifndef YY_RL78_RL_PARSE_H_INCLUDED
# define YY_RL78_RL_PARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int rl78_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    A = 258,
    X = 259,
    B = 260,
    C = 261,
    D = 262,
    E = 263,
    H = 264,
    L = 265,
    AX = 266,
    BC = 267,
    DE = 268,
    HL = 269,
    SPL = 270,
    SPH = 271,
    PSW = 272,
    CS = 273,
    ES = 274,
    PMC = 275,
    MEM = 276,
    FLAG = 277,
    SP = 278,
    CY = 279,
    RB0 = 280,
    RB1 = 281,
    RB2 = 282,
    RB3 = 283,
    EXPR = 284,
    UNKNOWN_OPCODE = 285,
    IS_OPCODE = 286,
    DOT_S = 287,
    DOT_B = 288,
    DOT_W = 289,
    DOT_L = 290,
    DOT_A = 291,
    DOT_UB = 292,
    DOT_UW = 293,
    ADD = 294,
    ADDC = 295,
    ADDW = 296,
    AND_ = 297,
    AND1 = 298,
    BF = 299,
    BH = 300,
    BNC = 301,
    BNH = 302,
    BNZ = 303,
    BR = 304,
    BRK = 305,
    BRK1 = 306,
    BT = 307,
    BTCLR = 308,
    BZ = 309,
    CALL = 310,
    CALLT = 311,
    CLR1 = 312,
    CLRB = 313,
    CLRW = 314,
    CMP = 315,
    CMP0 = 316,
    CMPS = 317,
    CMPW = 318,
    DEC = 319,
    DECW = 320,
    DI = 321,
    DIVHU = 322,
    DIVWU = 323,
    EI = 324,
    HALT = 325,
    INC = 326,
    INCW = 327,
    MACH = 328,
    MACHU = 329,
    MOV = 330,
    MOV1 = 331,
    MOVS = 332,
    MOVW = 333,
    MULH = 334,
    MULHU = 335,
    MULU = 336,
    NOP = 337,
    NOT1 = 338,
    ONEB = 339,
    ONEW = 340,
    OR = 341,
    OR1 = 342,
    POP = 343,
    PUSH = 344,
    RET = 345,
    RETI = 346,
    RETB = 347,
    ROL = 348,
    ROLC = 349,
    ROLWC = 350,
    ROR = 351,
    RORC = 352,
    SAR = 353,
    SARW = 354,
    SEL = 355,
    SET1 = 356,
    SHL = 357,
    SHLW = 358,
    SHR = 359,
    SHRW = 360,
    SKC = 361,
    SKH = 362,
    SKNC = 363,
    SKNH = 364,
    SKNZ = 365,
    SKZ = 366,
    STOP = 367,
    SUB = 368,
    SUBC = 369,
    SUBW = 370,
    XCH = 371,
    XCHW = 372,
    XOR = 373,
    XOR1 = 374
  };
#endif
/* Tokens.  */
#define A 258
#define X 259
#define B 260
#define C 261
#define D 262
#define E 263
#define H 264
#define L 265
#define AX 266
#define BC 267
#define DE 268
#define HL 269
#define SPL 270
#define SPH 271
#define PSW 272
#define CS 273
#define ES 274
#define PMC 275
#define MEM 276
#define FLAG 277
#define SP 278
#define CY 279
#define RB0 280
#define RB1 281
#define RB2 282
#define RB3 283
#define EXPR 284
#define UNKNOWN_OPCODE 285
#define IS_OPCODE 286
#define DOT_S 287
#define DOT_B 288
#define DOT_W 289
#define DOT_L 290
#define DOT_A 291
#define DOT_UB 292
#define DOT_UW 293
#define ADD 294
#define ADDC 295
#define ADDW 296
#define AND_ 297
#define AND1 298
#define BF 299
#define BH 300
#define BNC 301
#define BNH 302
#define BNZ 303
#define BR 304
#define BRK 305
#define BRK1 306
#define BT 307
#define BTCLR 308
#define BZ 309
#define CALL 310
#define CALLT 311
#define CLR1 312
#define CLRB 313
#define CLRW 314
#define CMP 315
#define CMP0 316
#define CMPS 317
#define CMPW 318
#define DEC 319
#define DECW 320
#define DI 321
#define DIVHU 322
#define DIVWU 323
#define EI 324
#define HALT 325
#define INC 326
#define INCW 327
#define MACH 328
#define MACHU 329
#define MOV 330
#define MOV1 331
#define MOVS 332
#define MOVW 333
#define MULH 334
#define MULHU 335
#define MULU 336
#define NOP 337
#define NOT1 338
#define ONEB 339
#define ONEW 340
#define OR 341
#define OR1 342
#define POP 343
#define PUSH 344
#define RET 345
#define RETI 346
#define RETB 347
#define ROL 348
#define ROLC 349
#define ROLWC 350
#define ROR 351
#define RORC 352
#define SAR 353
#define SARW 354
#define SEL 355
#define SET1 356
#define SHL 357
#define SHLW 358
#define SHR 359
#define SHRW 360
#define SKC 361
#define SKH 362
#define SKNC 363
#define SKNH 364
#define SKNZ 365
#define SKZ 366
#define STOP 367
#define SUB 368
#define SUBC 369
#define SUBW 370
#define XCH 371
#define XCHW 372
#define XOR 373
#define XOR1 374

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 144 "./config/rl78-parse.y" /* yacc.c:1910  */

  int regno;
  expressionS exp;

#line 297 "rl78-parse.h" /* yacc.c:1910  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE rl78_lval;

int rl78_parse (void);

#endif /* !YY_RL78_RL_PARSE_H_INCLUDED  */
