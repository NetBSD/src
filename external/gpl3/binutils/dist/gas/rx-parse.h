/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     REG = 258,
     FLAG = 259,
     CREG = 260,
     EXPR = 261,
     UNKNOWN_OPCODE = 262,
     IS_OPCODE = 263,
     DOT_S = 264,
     DOT_B = 265,
     DOT_W = 266,
     DOT_L = 267,
     DOT_A = 268,
     DOT_UB = 269,
     DOT_UW = 270,
     ABS = 271,
     ADC = 272,
     ADD = 273,
     AND_ = 274,
     BCLR = 275,
     BCND = 276,
     BMCND = 277,
     BNOT = 278,
     BRA = 279,
     BRK = 280,
     BSET = 281,
     BSR = 282,
     BTST = 283,
     CLRPSW = 284,
     CMP = 285,
     DBT = 286,
     DIV = 287,
     DIVU = 288,
     EDIV = 289,
     EDIVU = 290,
     EMUL = 291,
     EMULU = 292,
     FADD = 293,
     FCMP = 294,
     FDIV = 295,
     FMUL = 296,
     FREIT = 297,
     FSUB = 298,
     FTOI = 299,
     INT = 300,
     ITOF = 301,
     JMP = 302,
     JSR = 303,
     MACHI = 304,
     MACLO = 305,
     MAX = 306,
     MIN = 307,
     MOV = 308,
     MOVU = 309,
     MUL = 310,
     MULHI = 311,
     MULLO = 312,
     MULU = 313,
     MVFACHI = 314,
     MVFACMI = 315,
     MVFACLO = 316,
     MVFC = 317,
     MVTACHI = 318,
     MVTACLO = 319,
     MVTC = 320,
     MVTIPL = 321,
     NEG = 322,
     NOP = 323,
     NOT = 324,
     OR = 325,
     POP = 326,
     POPC = 327,
     POPM = 328,
     PUSH = 329,
     PUSHA = 330,
     PUSHC = 331,
     PUSHM = 332,
     RACW = 333,
     REIT = 334,
     REVL = 335,
     REVW = 336,
     RMPA = 337,
     ROLC = 338,
     RORC = 339,
     ROTL = 340,
     ROTR = 341,
     ROUND = 342,
     RTE = 343,
     RTFI = 344,
     RTS = 345,
     RTSD = 346,
     SAT = 347,
     SATR = 348,
     SBB = 349,
     SCCND = 350,
     SCMPU = 351,
     SETPSW = 352,
     SHAR = 353,
     SHLL = 354,
     SHLR = 355,
     SMOVB = 356,
     SMOVF = 357,
     SMOVU = 358,
     SSTR = 359,
     STNZ = 360,
     STOP = 361,
     STZ = 362,
     SUB = 363,
     SUNTIL = 364,
     SWHILE = 365,
     TST = 366,
     WAIT = 367,
     XCHG = 368,
     XOR = 369
   };
#endif
/* Tokens.  */
#define REG 258
#define FLAG 259
#define CREG 260
#define EXPR 261
#define UNKNOWN_OPCODE 262
#define IS_OPCODE 263
#define DOT_S 264
#define DOT_B 265
#define DOT_W 266
#define DOT_L 267
#define DOT_A 268
#define DOT_UB 269
#define DOT_UW 270
#define ABS 271
#define ADC 272
#define ADD 273
#define AND_ 274
#define BCLR 275
#define BCND 276
#define BMCND 277
#define BNOT 278
#define BRA 279
#define BRK 280
#define BSET 281
#define BSR 282
#define BTST 283
#define CLRPSW 284
#define CMP 285
#define DBT 286
#define DIV 287
#define DIVU 288
#define EDIV 289
#define EDIVU 290
#define EMUL 291
#define EMULU 292
#define FADD 293
#define FCMP 294
#define FDIV 295
#define FMUL 296
#define FREIT 297
#define FSUB 298
#define FTOI 299
#define INT 300
#define ITOF 301
#define JMP 302
#define JSR 303
#define MACHI 304
#define MACLO 305
#define MAX 306
#define MIN 307
#define MOV 308
#define MOVU 309
#define MUL 310
#define MULHI 311
#define MULLO 312
#define MULU 313
#define MVFACHI 314
#define MVFACMI 315
#define MVFACLO 316
#define MVFC 317
#define MVTACHI 318
#define MVTACLO 319
#define MVTC 320
#define MVTIPL 321
#define NEG 322
#define NOP 323
#define NOT 324
#define OR 325
#define POP 326
#define POPC 327
#define POPM 328
#define PUSH 329
#define PUSHA 330
#define PUSHC 331
#define PUSHM 332
#define RACW 333
#define REIT 334
#define REVL 335
#define REVW 336
#define RMPA 337
#define ROLC 338
#define RORC 339
#define ROTL 340
#define ROTR 341
#define ROUND 342
#define RTE 343
#define RTFI 344
#define RTS 345
#define RTSD 346
#define SAT 347
#define SATR 348
#define SBB 349
#define SCCND 350
#define SCMPU 351
#define SETPSW 352
#define SHAR 353
#define SHLL 354
#define SHLR 355
#define SMOVB 356
#define SMOVF 357
#define SMOVU 358
#define SSTR 359
#define STNZ 360
#define STOP 361
#define STZ 362
#define SUB 363
#define SUNTIL 364
#define SWHILE 365
#define TST 366
#define WAIT 367
#define XCHG 368
#define XOR 369




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 134 "rx-parse.y"
{
  int regno;
  expressionS exp;
}
/* Line 1529 of yacc.c.  */
#line 282 "rx-parse.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE rx_lval;

