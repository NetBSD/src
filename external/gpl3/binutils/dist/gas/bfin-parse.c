/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     BYTEOP16P = 258,
     BYTEOP16M = 259,
     BYTEOP1P = 260,
     BYTEOP2P = 261,
     BYTEOP2M = 262,
     BYTEOP3P = 263,
     BYTEUNPACK = 264,
     BYTEPACK = 265,
     PACK = 266,
     SAA = 267,
     ALIGN8 = 268,
     ALIGN16 = 269,
     ALIGN24 = 270,
     VIT_MAX = 271,
     EXTRACT = 272,
     DEPOSIT = 273,
     EXPADJ = 274,
     SEARCH = 275,
     ONES = 276,
     SIGN = 277,
     SIGNBITS = 278,
     LINK = 279,
     UNLINK = 280,
     REG = 281,
     PC = 282,
     CCREG = 283,
     BYTE_DREG = 284,
     REG_A_DOUBLE_ZERO = 285,
     REG_A_DOUBLE_ONE = 286,
     A_ZERO_DOT_L = 287,
     A_ZERO_DOT_H = 288,
     A_ONE_DOT_L = 289,
     A_ONE_DOT_H = 290,
     HALF_REG = 291,
     NOP = 292,
     RTI = 293,
     RTS = 294,
     RTX = 295,
     RTN = 296,
     RTE = 297,
     HLT = 298,
     IDLE = 299,
     STI = 300,
     CLI = 301,
     CSYNC = 302,
     SSYNC = 303,
     EMUEXCPT = 304,
     RAISE = 305,
     EXCPT = 306,
     LSETUP = 307,
     LOOP = 308,
     LOOP_BEGIN = 309,
     LOOP_END = 310,
     DISALGNEXCPT = 311,
     JUMP = 312,
     JUMP_DOT_S = 313,
     JUMP_DOT_L = 314,
     CALL = 315,
     ABORT = 316,
     NOT = 317,
     TILDA = 318,
     BANG = 319,
     AMPERSAND = 320,
     BAR = 321,
     PERCENT = 322,
     CARET = 323,
     BXOR = 324,
     MINUS = 325,
     PLUS = 326,
     STAR = 327,
     SLASH = 328,
     NEG = 329,
     MIN = 330,
     MAX = 331,
     ABS = 332,
     DOUBLE_BAR = 333,
     _PLUS_BAR_PLUS = 334,
     _PLUS_BAR_MINUS = 335,
     _MINUS_BAR_PLUS = 336,
     _MINUS_BAR_MINUS = 337,
     _MINUS_MINUS = 338,
     _PLUS_PLUS = 339,
     SHIFT = 340,
     LSHIFT = 341,
     ASHIFT = 342,
     BXORSHIFT = 343,
     _GREATER_GREATER_GREATER_THAN_ASSIGN = 344,
     ROT = 345,
     LESS_LESS = 346,
     GREATER_GREATER = 347,
     _GREATER_GREATER_GREATER = 348,
     _LESS_LESS_ASSIGN = 349,
     _GREATER_GREATER_ASSIGN = 350,
     DIVS = 351,
     DIVQ = 352,
     ASSIGN = 353,
     _STAR_ASSIGN = 354,
     _BAR_ASSIGN = 355,
     _CARET_ASSIGN = 356,
     _AMPERSAND_ASSIGN = 357,
     _MINUS_ASSIGN = 358,
     _PLUS_ASSIGN = 359,
     _ASSIGN_BANG = 360,
     _LESS_THAN_ASSIGN = 361,
     _ASSIGN_ASSIGN = 362,
     GE = 363,
     LT = 364,
     LE = 365,
     GT = 366,
     LESS_THAN = 367,
     FLUSHINV = 368,
     FLUSH = 369,
     IFLUSH = 370,
     PREFETCH = 371,
     PRNT = 372,
     OUTC = 373,
     WHATREG = 374,
     TESTSET = 375,
     ASL = 376,
     ASR = 377,
     B = 378,
     W = 379,
     NS = 380,
     S = 381,
     CO = 382,
     SCO = 383,
     TH = 384,
     TL = 385,
     BP = 386,
     BREV = 387,
     X = 388,
     Z = 389,
     M = 390,
     MMOD = 391,
     R = 392,
     RND = 393,
     RNDL = 394,
     RNDH = 395,
     RND12 = 396,
     RND20 = 397,
     V = 398,
     LO = 399,
     HI = 400,
     BITTGL = 401,
     BITCLR = 402,
     BITSET = 403,
     BITTST = 404,
     BITMUX = 405,
     DBGAL = 406,
     DBGAH = 407,
     DBGHALT = 408,
     DBG = 409,
     DBGA = 410,
     DBGCMPLX = 411,
     IF = 412,
     COMMA = 413,
     BY = 414,
     COLON = 415,
     SEMICOLON = 416,
     RPAREN = 417,
     LPAREN = 418,
     LBRACK = 419,
     RBRACK = 420,
     STATUS_REG = 421,
     MNOP = 422,
     SYMBOL = 423,
     NUMBER = 424,
     GOT = 425,
     GOT17M4 = 426,
     FUNCDESC_GOT17M4 = 427,
     AT = 428,
     PLTPC = 429
   };
#endif
/* Tokens.  */
#define BYTEOP16P 258
#define BYTEOP16M 259
#define BYTEOP1P 260
#define BYTEOP2P 261
#define BYTEOP2M 262
#define BYTEOP3P 263
#define BYTEUNPACK 264
#define BYTEPACK 265
#define PACK 266
#define SAA 267
#define ALIGN8 268
#define ALIGN16 269
#define ALIGN24 270
#define VIT_MAX 271
#define EXTRACT 272
#define DEPOSIT 273
#define EXPADJ 274
#define SEARCH 275
#define ONES 276
#define SIGN 277
#define SIGNBITS 278
#define LINK 279
#define UNLINK 280
#define REG 281
#define PC 282
#define CCREG 283
#define BYTE_DREG 284
#define REG_A_DOUBLE_ZERO 285
#define REG_A_DOUBLE_ONE 286
#define A_ZERO_DOT_L 287
#define A_ZERO_DOT_H 288
#define A_ONE_DOT_L 289
#define A_ONE_DOT_H 290
#define HALF_REG 291
#define NOP 292
#define RTI 293
#define RTS 294
#define RTX 295
#define RTN 296
#define RTE 297
#define HLT 298
#define IDLE 299
#define STI 300
#define CLI 301
#define CSYNC 302
#define SSYNC 303
#define EMUEXCPT 304
#define RAISE 305
#define EXCPT 306
#define LSETUP 307
#define LOOP 308
#define LOOP_BEGIN 309
#define LOOP_END 310
#define DISALGNEXCPT 311
#define JUMP 312
#define JUMP_DOT_S 313
#define JUMP_DOT_L 314
#define CALL 315
#define ABORT 316
#define NOT 317
#define TILDA 318
#define BANG 319
#define AMPERSAND 320
#define BAR 321
#define PERCENT 322
#define CARET 323
#define BXOR 324
#define MINUS 325
#define PLUS 326
#define STAR 327
#define SLASH 328
#define NEG 329
#define MIN 330
#define MAX 331
#define ABS 332
#define DOUBLE_BAR 333
#define _PLUS_BAR_PLUS 334
#define _PLUS_BAR_MINUS 335
#define _MINUS_BAR_PLUS 336
#define _MINUS_BAR_MINUS 337
#define _MINUS_MINUS 338
#define _PLUS_PLUS 339
#define SHIFT 340
#define LSHIFT 341
#define ASHIFT 342
#define BXORSHIFT 343
#define _GREATER_GREATER_GREATER_THAN_ASSIGN 344
#define ROT 345
#define LESS_LESS 346
#define GREATER_GREATER 347
#define _GREATER_GREATER_GREATER 348
#define _LESS_LESS_ASSIGN 349
#define _GREATER_GREATER_ASSIGN 350
#define DIVS 351
#define DIVQ 352
#define ASSIGN 353
#define _STAR_ASSIGN 354
#define _BAR_ASSIGN 355
#define _CARET_ASSIGN 356
#define _AMPERSAND_ASSIGN 357
#define _MINUS_ASSIGN 358
#define _PLUS_ASSIGN 359
#define _ASSIGN_BANG 360
#define _LESS_THAN_ASSIGN 361
#define _ASSIGN_ASSIGN 362
#define GE 363
#define LT 364
#define LE 365
#define GT 366
#define LESS_THAN 367
#define FLUSHINV 368
#define FLUSH 369
#define IFLUSH 370
#define PREFETCH 371
#define PRNT 372
#define OUTC 373
#define WHATREG 374
#define TESTSET 375
#define ASL 376
#define ASR 377
#define B 378
#define W 379
#define NS 380
#define S 381
#define CO 382
#define SCO 383
#define TH 384
#define TL 385
#define BP 386
#define BREV 387
#define X 388
#define Z 389
#define M 390
#define MMOD 391
#define R 392
#define RND 393
#define RNDL 394
#define RNDH 395
#define RND12 396
#define RND20 397
#define V 398
#define LO 399
#define HI 400
#define BITTGL 401
#define BITCLR 402
#define BITSET 403
#define BITTST 404
#define BITMUX 405
#define DBGAL 406
#define DBGAH 407
#define DBGHALT 408
#define DBG 409
#define DBGA 410
#define DBGCMPLX 411
#define IF 412
#define COMMA 413
#define BY 414
#define COLON 415
#define SEMICOLON 416
#define RPAREN 417
#define LPAREN 418
#define LBRACK 419
#define RBRACK 420
#define STATUS_REG 421
#define MNOP 422
#define SYMBOL 423
#define NUMBER 424
#define GOT 425
#define GOT17M4 426
#define FUNCDESC_GOT17M4 427
#define AT 428
#define PLTPC 429




/* Copy the first part of user declarations.  */
#line 21 "bfin-parse.y"


#include "as.h"
#include <obstack.h>

#include "bfin-aux.h"  // opcode generating auxiliaries
#include "libbfd.h"
#include "elf/common.h"
#include "elf/bfin.h"

#define DSP32ALU(aopcde, HL, dst1, dst0, src0, src1, s, x, aop) \
	bfin_gen_dsp32alu (HL, aopcde, aop, s, x, dst0, dst1, src0, src1)

#define DSP32MAC(op1, MM, mmod, w1, P, h01, h11, h00, h10, dst, op0, src0, src1, w0) \
	bfin_gen_dsp32mac (op1, MM, mmod, w1, P, h01, h11, h00, h10, op0, \
	                   dst, src0, src1, w0)

#define DSP32MULT(op1, MM, mmod, w1, P, h01, h11, h00, h10, dst, op0, src0, src1, w0) \
	bfin_gen_dsp32mult (op1, MM, mmod, w1, P, h01, h11, h00, h10, op0, \
	                    dst, src0, src1, w0)

#define DSP32SHIFT(sopcde, dst0, src0, src1, sop, hls)  \
	bfin_gen_dsp32shift (sopcde, dst0, src0, src1, sop, hls)

#define DSP32SHIFTIMM(sopcde, dst0, immag, src1, sop, hls)  \
	bfin_gen_dsp32shiftimm (sopcde, dst0, immag, src1, sop, hls)

#define LDIMMHALF_R(reg, h, s, z, hword) \
	bfin_gen_ldimmhalf (reg, h, s, z, hword, 1)

#define LDIMMHALF_R5(reg, h, s, z, hword) \
        bfin_gen_ldimmhalf (reg, h, s, z, hword, 2)

#define LDSTIDXI(ptr, reg, w, sz, z, offset)  \
	bfin_gen_ldstidxi (ptr, reg, w, sz, z, offset)

#define LDST(ptr, reg, aop, sz, z, w)  \
	bfin_gen_ldst (ptr, reg, aop, sz, z, w)

#define LDSTII(ptr, reg, offset, w, op)  \
	bfin_gen_ldstii (ptr, reg, offset, w, op)

#define DSPLDST(i, m, reg, aop, w) \
	bfin_gen_dspldst (i, reg, aop, w, m)

#define LDSTPMOD(ptr, reg, idx, aop, w) \
	bfin_gen_ldstpmod (ptr, reg, aop, w, idx)

#define LDSTIIFP(offset, reg, w)  \
	bfin_gen_ldstiifp (reg, offset, w)

#define LOGI2OP(dst, src, opc) \
	bfin_gen_logi2op (opc, src, dst.regno & CODE_MASK)

#define ALU2OP(dst, src, opc)  \
	bfin_gen_alu2op (dst, src, opc)

#define BRCC(t, b, offset) \
	bfin_gen_brcc (t, b, offset)

#define UJUMP(offset) \
	bfin_gen_ujump (offset)

#define PROGCTRL(prgfunc, poprnd) \
	bfin_gen_progctrl (prgfunc, poprnd)

#define PUSHPOPMULTIPLE(dr, pr, d, p, w) \
	bfin_gen_pushpopmultiple (dr, pr, d, p, w)

#define PUSHPOPREG(reg, w) \
	bfin_gen_pushpopreg (reg, w)

#define CALLA(addr, s)  \
	bfin_gen_calla (addr, s)

#define LINKAGE(r, framesize) \
	bfin_gen_linkage (r, framesize)

#define COMPI2OPD(dst, src, op)  \
	bfin_gen_compi2opd (dst, src, op)

#define COMPI2OPP(dst, src, op)  \
	bfin_gen_compi2opp (dst, src, op)

#define DAGMODIK(i, op)  \
	bfin_gen_dagmodik (i, op)

#define DAGMODIM(i, m, op, br)  \
	bfin_gen_dagmodim (i, m, op, br)

#define COMP3OP(dst, src0, src1, opc)   \
	bfin_gen_comp3op (src0, src1, dst, opc)

#define PTR2OP(dst, src, opc)   \
	bfin_gen_ptr2op (dst, src, opc)

#define CCFLAG(x, y, opc, i, g)  \
	bfin_gen_ccflag (x, y, opc, i, g)

#define CCMV(src, dst, t) \
	bfin_gen_ccmv (src, dst, t)

#define CACTRL(reg, a, op) \
	bfin_gen_cactrl (reg, a, op)

#define LOOPSETUP(soffset, c, rop, eoffset, reg) \
	bfin_gen_loopsetup (soffset, c, rop, eoffset, reg)

#define HL2(r1, r0)  (IS_H (r1) << 1 | IS_H (r0))
#define IS_RANGE(bits, expr, sign, mul)    \
	value_match(expr, bits, sign, mul, 1)
#define IS_URANGE(bits, expr, sign, mul)    \
	value_match(expr, bits, sign, mul, 0)
#define IS_CONST(expr) (expr->type == Expr_Node_Constant)
#define IS_RELOC(expr) (expr->type != Expr_Node_Constant)
#define IS_IMM(expr, bits)  value_match (expr, bits, 0, 1, 1)
#define IS_UIMM(expr, bits)  value_match (expr, bits, 0, 1, 0)

#define IS_PCREL4(expr) \
	(value_match (expr, 4, 0, 2, 0))

#define IS_LPPCREL10(expr) \
	(value_match (expr, 10, 0, 2, 0))

#define IS_PCREL10(expr) \
	(value_match (expr, 10, 0, 2, 1))

#define IS_PCREL12(expr) \
	(value_match (expr, 12, 0, 2, 1))

#define IS_PCREL24(expr) \
	(value_match (expr, 24, 0, 2, 1))


static int value_match (Expr_Node *expr, int sz, int sign, int mul, int issigned);

extern FILE *errorf;
extern INSTR_T insn;

static Expr_Node *binary (Expr_Op_Type, Expr_Node *, Expr_Node *);
static Expr_Node *unary  (Expr_Op_Type, Expr_Node *);

static void notethat (char *format, ...);

char *current_inputline;
extern char *yytext;
int yyerror (char *msg);

void error (char *format, ...)
{
    va_list ap;
    char buffer[2000];
    
    va_start (ap, format);
    vsprintf (buffer, format, ap);
    va_end (ap);

    as_bad (buffer);
}

int
yyerror (char *msg)
{
  if (msg[0] == '\0')
    error ("%s", msg);

  else if (yytext[0] != ';')
    error ("%s. Input text was %s.", msg, yytext);
  else
    error ("%s.", msg);

  return -1;
}

static int
in_range_p (Expr_Node *expr, int from, int to, unsigned int mask)
{
  int val = EXPR_VALUE (expr);
  if (expr->type != Expr_Node_Constant)
    return 0;
  if (val < from || val > to)
    return 0;
  return (val & mask) == 0;
}

extern int yylex (void);

#define imm3(x) EXPR_VALUE (x)
#define imm4(x) EXPR_VALUE (x)
#define uimm4(x) EXPR_VALUE (x)
#define imm5(x) EXPR_VALUE (x)
#define uimm5(x) EXPR_VALUE (x)
#define imm6(x) EXPR_VALUE (x)
#define imm7(x) EXPR_VALUE (x)
#define imm16(x) EXPR_VALUE (x)
#define uimm16s4(x) ((EXPR_VALUE (x)) >> 2)
#define uimm16(x) EXPR_VALUE (x)

/* Return true if a value is inside a range.  */
#define IN_RANGE(x, low, high) \
  (((EXPR_VALUE(x)) >= (low)) && (EXPR_VALUE(x)) <= ((high)))

/* Auxiliary functions.  */

static void
neg_value (Expr_Node *expr)
{
  expr->value.i_value = -expr->value.i_value;
}

static int
valid_dreg_pair (Register *reg1, Expr_Node *reg2)
{
  if (!IS_DREG (*reg1))
    {
      yyerror ("Dregs expected");
      return 0;
    }

  if (reg1->regno != 1 && reg1->regno != 3)
    {
      yyerror ("Bad register pair");
      return 0;
    }

  if (imm7 (reg2) != reg1->regno - 1)
    {
      yyerror ("Bad register pair");
      return 0;
    }

  reg1->regno--;
  return 1;
}

static int
check_multiply_halfregs (Macfunc *aa, Macfunc *ab)
{
  if ((!REG_EQUAL (aa->s0, ab->s0) && !REG_EQUAL (aa->s0, ab->s1))
      || (!REG_EQUAL (aa->s1, ab->s1) && !REG_EQUAL (aa->s1, ab->s0)))
    return yyerror ("Source multiplication register mismatch");

  return 0;
}


/* Check mac option.  */

static int
check_macfunc_option (Macfunc *a, Opt_mode *opt)
{
  /* Default option is always valid.  */
  if (opt->mod == 0)
    return 0;

  if ((a->w == 1 && a->P == 1
       && opt->mod != M_FU && opt->mod != M_IS && opt->mod != M_IU
       && opt->mod != M_S2RND && opt->mod != M_ISS2)
      || (a->w == 1 && a->P == 0
	  && opt->mod != M_FU && opt->mod != M_IS && opt->mod != M_IU
	  && opt->mod != M_T && opt->mod != M_TFU && opt->mod != M_S2RND
	  && opt->mod != M_ISS2 && opt->mod != M_IH)
      || (a->w == 0 && a->P == 0
	  && opt->mod != M_FU && opt->mod != M_IS && opt->mod != M_W32))
    return -1;

  return 0;
}

/* Check (vector) mac funcs and ops.  */

static int
check_macfuncs (Macfunc *aa, Opt_mode *opa,
		Macfunc *ab, Opt_mode *opb)
{
  /* Variables for swapping.  */
  Macfunc mtmp;
  Opt_mode otmp;

  /* The option mode should be put at the end of the second instruction
     of the vector except M, which should follow MAC1 instruction.  */
  if (opa->mod != 0)
    return yyerror ("Bad opt mode");

  /* If a0macfunc comes before a1macfunc, swap them.  */
	
  if (aa->n == 0)
    {
      /*  (M) is not allowed here.  */
      if (opa->MM != 0)
	return yyerror ("(M) not allowed with A0MAC");
      if (ab->n != 1)
	return yyerror ("Vector AxMACs can't be same");

      mtmp = *aa; *aa = *ab; *ab = mtmp;
      otmp = *opa; *opa = *opb; *opb = otmp;
    }
  else
    {
      if (opb->MM != 0)
	return yyerror ("(M) not allowed with A0MAC");
      if (ab->n != 0)
	return yyerror ("Vector AxMACs can't be same");
    }

  /*  If both ops are one of 0, 1, or 2, we have multiply_halfregs in both
  assignment_or_macfuncs.  */
  if ((aa->op == 0 || aa->op == 1 || aa->op == 2)
      && (ab->op == 0 || ab->op == 1 || ab->op == 2))
    {
      if (check_multiply_halfregs (aa, ab) < 0)
	return -1;
    }
  else
    {
      /*  Only one of the assign_macfuncs has a half reg multiply
      Evil trick: Just 'OR' their source register codes:
      We can do that, because we know they were initialized to 0
      in the rules that don't use multiply_halfregs.  */
      aa->s0.regno |= (ab->s0.regno & CODE_MASK);
      aa->s1.regno |= (ab->s1.regno & CODE_MASK);
    }

  if (aa->w == ab->w  && aa->P != ab->P)
    {
      return yyerror ("macfuncs must differ");
      if (aa->w && (aa->dst.regno - ab->dst.regno != 1))
	return yyerror ("Destination Dregs must differ by one");
    }

  /* Make sure mod flags get ORed, too.  */
  opb->mod |= opa->mod;

  /* Check option.  */
  if (check_macfunc_option (aa, opb) < 0
      && check_macfunc_option (ab, opb) < 0)
    return yyerror ("bad option");

  /* Make sure first macfunc has got both P flags ORed.  */
  aa->P |= ab->P;

  return 0;	
}


static int
is_group1 (INSTR_T x)
{
  /* Group1 is dpsLDST, LDSTpmod, LDST, LDSTiiFP, LDSTii.  */
  if ((x->value & 0xc000) == 0x8000 || (x->value == 0x0000))
    return 1;

  return 0;
}

static int
is_group2 (INSTR_T x)
{
  if ((((x->value & 0xfc00) == 0x9c00)  /* dspLDST.  */
       && !((x->value & 0xfde0) == 0x9c60)  /* dagMODim.  */
       && !((x->value & 0xfde0) == 0x9ce0)  /* dagMODim with bit rev.  */
       && !((x->value & 0xfde0) == 0x9d60)) /* pick dagMODik.  */
      || (x->value == 0x0000))
    return 1;
  return 0;
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 390 "bfin-parse.y"
typedef union YYSTYPE {
  INSTR_T instr;
  Expr_Node *expr;
  SYMBOL_T symbol;
  long value;
  Register reg;
  Macfunc macfunc;
  struct { int r0; int s0; int x0; int aop; } modcodes;
  struct { int r0; } r0;
  Opt_mode mod;
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 814 "bfin-parse.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 826 "bfin-parse.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  145
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1278

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  175
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  47
/* YYNRULES -- Number of rules. */
#define YYNRULES  348
/* YYNRULES -- Number of states. */
#define YYNSTATES  1021

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   429

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     4,     6,     9,    16,    21,    23,    25,
      28,    34,    36,    43,    50,    54,    58,    76,    94,   106,
     118,   130,   143,   156,   169,   175,   179,   183,   187,   196,
     210,   223,   237,   251,   265,   274,   292,   299,   309,   313,
     320,   324,   330,   337,   346,   355,   358,   361,   366,   370,
     373,   378,   382,   389,   394,   402,   410,   414,   418,   425,
     429,   434,   438,   442,   446,   458,   470,   480,   486,   492,
     502,   508,   514,   521,   528,   534,   540,   546,   553,   560,
     566,   568,   572,   576,   580,   584,   589,   594,   604,   614,
     620,   628,   633,   640,   646,   653,   661,   671,   680,   689,
     701,   711,   716,   722,   729,   737,   744,   749,   756,   762,
     769,   776,   781,   790,   801,   812,   825,   831,   838,   844,
     851,   856,   861,   866,   874,   884,   894,   904,   911,   918,
     925,   934,   943,   950,   956,   962,   971,   976,   984,   986,
     988,   990,   992,   994,   996,   998,  1000,  1002,  1004,  1007,
    1010,  1015,  1020,  1027,  1034,  1037,  1040,  1045,  1048,  1051,
    1054,  1057,  1060,  1063,  1070,  1077,  1083,  1088,  1092,  1096,
    1100,  1104,  1108,  1112,  1117,  1120,  1125,  1128,  1133,  1136,
    1141,  1144,  1152,  1161,  1170,  1178,  1186,  1194,  1204,  1212,
    1221,  1231,  1240,  1247,  1255,  1264,  1274,  1283,  1291,  1299,
    1306,  1318,  1326,  1338,  1346,  1350,  1353,  1355,  1363,  1373,
    1385,  1389,  1395,  1403,  1405,  1408,  1411,  1416,  1418,  1425,
    1432,  1439,  1441,  1443,  1444,  1450,  1456,  1460,  1464,  1468,
    1472,  1473,  1475,  1477,  1479,  1481,  1483,  1484,  1488,  1489,
    1493,  1497,  1498,  1502,  1506,  1512,  1518,  1519,  1523,  1527,
    1528,  1532,  1536,  1537,  1541,  1545,  1549,  1555,  1561,  1562,
    1566,  1567,  1571,  1573,  1575,  1577,  1579,  1580,  1584,  1588,
    1592,  1598,  1604,  1606,  1608,  1610,  1611,  1615,  1616,  1620,
    1625,  1630,  1632,  1634,  1636,  1638,  1640,  1642,  1644,  1646,
    1650,  1654,  1658,  1662,  1668,  1674,  1680,  1686,  1690,  1694,
    1700,  1706,  1707,  1709,  1711,  1714,  1717,  1720,  1724,  1726,
    1732,  1738,  1742,  1745,  1748,  1751,  1755,  1757,  1759,  1761,
    1763,  1767,  1771,  1775,  1779,  1781,  1783,  1785,  1787,  1791,
    1793,  1795,  1799,  1801,  1803,  1807,  1810,  1813,  1815,  1819,
    1823,  1827,  1831,  1835,  1839,  1843,  1847,  1851,  1855
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     176,     0,    -1,    -1,   177,    -1,   178,   161,    -1,   178,
      78,   178,    78,   178,   161,    -1,   178,    78,   178,   161,
      -1,     1,    -1,   167,    -1,   209,   180,    -1,   209,   180,
     158,   209,   180,    -1,    56,    -1,    26,    98,   163,   208,
     179,   162,    -1,    36,    98,   163,   208,   179,   162,    -1,
      33,    98,    36,    -1,    35,    98,    36,    -1,   163,    26,
     158,    26,   162,    98,     3,   163,    26,   160,   220,   158,
      26,   160,   220,   162,   193,    -1,   163,    26,   158,    26,
     162,    98,     4,   163,    26,   160,   220,   158,    26,   160,
     220,   162,   193,    -1,   163,    26,   158,    26,   162,    98,
       9,    26,   160,   220,   193,    -1,   163,    26,   158,    26,
     162,    98,    20,    26,   163,   192,   162,    -1,    26,    98,
      34,    71,    35,   158,    26,    98,    32,    71,    33,    -1,
      26,    98,   179,    71,   179,   158,    26,    98,   179,    70,
     179,   185,    -1,    26,    98,    26,   202,    26,   158,    26,
      98,    26,   202,    26,   185,    -1,    26,    98,    26,   201,
      26,   158,    26,    98,    26,   201,    26,   186,    -1,    26,
      98,    77,    26,   190,    -1,   206,    77,   179,    -1,    32,
      98,    36,    -1,    34,    98,    36,    -1,    26,    98,   195,
     163,    26,   158,    26,   162,    -1,    26,    98,     5,   163,
      26,   160,   220,   158,    26,   160,   220,   162,   194,    -1,
      26,    98,     5,   163,    26,   160,   220,   158,    26,   160,
     220,   162,    -1,    26,    98,     6,   163,    26,   160,   220,
     158,    26,   160,   220,   162,   203,    -1,    26,    98,     7,
     163,    26,   160,   220,   158,    26,   160,   220,   162,   203,
      -1,    26,    98,     8,   163,    26,   160,   220,   158,    26,
     160,   220,   162,   204,    -1,    26,    98,    10,   163,    26,
     158,    26,   162,    -1,    36,    98,    36,    98,    22,   163,
      36,   162,    72,    36,    71,    22,   163,    36,   162,    72,
      36,    -1,    26,    98,    26,   202,    26,   185,    -1,    26,
      98,   200,   163,    26,   158,    26,   162,   190,    -1,   206,
      70,   179,    -1,    36,    98,    36,   202,    36,   185,    -1,
     206,   206,   220,    -1,   206,   179,   163,   126,   162,    -1,
      36,    98,    26,   163,   138,   162,    -1,    36,    98,    26,
     202,    26,   163,   141,   162,    -1,    36,    98,    26,   202,
      26,   163,   142,   162,    -1,   206,   179,    -1,   206,    26,
      -1,    26,    98,    36,   187,    -1,    36,    98,   220,    -1,
     206,   220,    -1,    26,    98,   220,   188,    -1,    36,    98,
      26,    -1,    26,    98,    26,   201,    26,   184,    -1,    26,
      98,    29,   187,    -1,   206,    77,   179,   158,   206,    77,
     179,    -1,   206,    70,   179,   158,   206,    70,   179,    -1,
     207,   179,   196,    -1,    26,   103,   220,    -1,    26,   104,
      26,   163,   132,   162,    -1,    26,   103,    26,    -1,   179,
     104,   179,   196,    -1,    26,   104,    26,    -1,    26,   104,
     220,    -1,    26,    99,    26,    -1,    12,   163,    26,   160,
     220,   158,    26,   160,   220,   162,   193,    -1,   206,   179,
     163,   126,   162,   158,   206,   179,   163,   126,   162,    -1,
      26,    98,   163,    26,    71,    26,   162,    91,   220,    -1,
      26,    98,    26,    66,    26,    -1,    26,    98,    26,    68,
      26,    -1,    26,    98,    26,    71,   163,    26,    91,   220,
     162,    -1,    28,    98,   179,   107,   179,    -1,    28,    98,
     179,   112,   179,    -1,    28,    98,    26,   112,    26,   197,
      -1,    28,    98,    26,   112,   220,   197,    -1,    28,    98,
      26,   107,    26,    -1,    28,    98,    26,   107,   220,    -1,
      28,    98,   179,   106,   179,    -1,    28,    98,    26,   106,
      26,   197,    -1,    28,    98,    26,   106,   220,   197,    -1,
      26,    98,    26,    65,    26,    -1,   213,    -1,    26,    98,
      26,    -1,    28,    98,    26,    -1,    26,    98,    28,    -1,
      28,   105,    28,    -1,    36,    98,   211,   180,    -1,    26,
      98,   211,   180,    -1,    36,    98,   211,   180,   158,    36,
      98,   211,   180,    -1,    26,    98,   211,   180,   158,    26,
      98,   211,   180,    -1,   206,    87,   179,   159,    36,    -1,
      36,    98,    87,    36,   159,    36,   191,    -1,   206,   179,
      91,   220,    -1,    26,    98,    26,    91,   220,   189,    -1,
      36,    98,    36,    91,   220,    -1,    36,    98,    36,    91,
     220,   191,    -1,    26,    98,    87,    26,   159,    36,   189,
      -1,    36,    98,    19,   163,    26,   158,    36,   162,   190,
      -1,    36,    98,    19,   163,    36,   158,    36,   162,    -1,
      26,    98,    18,   163,    26,   158,    26,   162,    -1,    26,
      98,    18,   163,    26,   158,    26,   162,   163,   133,   162,
      -1,    26,    98,    17,   163,    26,   158,    36,   162,   187,
      -1,   206,   179,    93,   220,    -1,   206,    86,   179,   159,
      36,    -1,    36,    98,    86,    36,   159,    36,    -1,    26,
      98,    86,    26,   159,    36,   190,    -1,    26,    98,    85,
      26,   159,    36,    -1,   206,   179,    92,   220,    -1,    26,
      98,    26,    92,   220,   190,    -1,    36,    98,    36,    92,
     220,    -1,    36,    98,    36,    93,   220,   191,    -1,    26,
      98,    26,    93,   220,   189,    -1,    36,    98,    21,    26,
      -1,    26,    98,    11,   163,    36,   158,    36,   162,    -1,
      36,    98,    28,    98,    88,   163,   179,   158,    26,   162,
      -1,    36,    98,    28,    98,    69,   163,   179,   158,    26,
     162,    -1,    36,    98,    28,    98,    69,   163,   179,   158,
     179,   158,    28,   162,    -1,   206,    90,   179,   159,    36,
      -1,    26,    98,    90,    26,   159,    36,    -1,   206,    90,
     179,   159,   220,    -1,    26,    98,    90,    26,   159,   220,
      -1,    36,    98,    23,   179,    -1,    36,    98,    23,    26,
      -1,    36,    98,    23,    36,    -1,    36,    98,    16,   163,
      26,   162,   181,    -1,    26,    98,    16,   163,    26,   158,
      26,   162,   181,    -1,   150,   163,    26,   158,    26,   158,
     179,   162,   181,    -1,   206,    88,   163,   179,   158,   179,
     158,    28,   162,    -1,   147,   163,    26,   158,   220,   162,
      -1,   148,   163,    26,   158,   220,   162,    -1,   146,   163,
      26,   158,   220,   162,    -1,    28,   105,   149,   163,    26,
     158,   220,   162,    -1,    28,    98,   149,   163,    26,   158,
     220,   162,    -1,   157,    64,    28,    26,    98,    26,    -1,
     157,    28,    26,    98,    26,    -1,   157,    64,    28,    57,
     220,    -1,   157,    64,    28,    57,   220,   163,   131,   162,
      -1,   157,    28,    57,   220,    -1,   157,    28,    57,   220,
     163,   131,   162,    -1,    37,    -1,    39,    -1,    38,    -1,
      40,    -1,    41,    -1,    42,    -1,    44,    -1,    47,    -1,
      48,    -1,    49,    -1,    46,    26,    -1,    45,    26,    -1,
      57,   163,    26,   162,    -1,    60,   163,    26,   162,    -1,
      60,   163,    27,    71,    26,   162,    -1,    57,   163,    27,
      71,    26,   162,    -1,    50,   220,    -1,    51,   220,    -1,
     120,   163,    26,   162,    -1,    57,   220,    -1,    58,   220,
      -1,    59,   220,    -1,    59,   218,    -1,    60,   220,    -1,
      60,   218,    -1,    97,   163,    26,   158,    26,   162,    -1,
      96,   163,    26,   158,    26,   162,    -1,    26,    98,    70,
      26,   189,    -1,    26,    98,    63,    26,    -1,    26,    95,
      26,    -1,    26,    95,   220,    -1,    26,    89,    26,    -1,
      26,    94,    26,    -1,    26,    94,   220,    -1,    26,    89,
     220,    -1,   114,   164,    26,   165,    -1,   114,   199,    -1,
     113,   164,    26,   165,    -1,   113,   199,    -1,   115,   164,
      26,   165,    -1,   115,   199,    -1,   116,   164,    26,   165,
      -1,   116,   199,    -1,   123,   164,    26,   205,   165,    98,
      26,    -1,   123,   164,    26,   202,   220,   165,    98,    26,
      -1,   124,   164,    26,   202,   220,   165,    98,    26,    -1,
     124,   164,    26,   205,   165,    98,    26,    -1,   124,   164,
      26,   205,   165,    98,    36,    -1,   164,    26,   202,   220,
     165,    98,    26,    -1,    26,    98,   124,   164,    26,   202,
     220,   165,   187,    -1,    36,    98,   124,   164,    26,   205,
     165,    -1,    26,    98,   124,   164,    26,   205,   165,   187,
      -1,    26,    98,   124,   164,    26,    84,    26,   165,   187,
      -1,    36,    98,   124,   164,    26,    84,    26,   165,    -1,
     164,    26,   205,   165,    98,    26,    -1,   164,    26,    84,
      26,   165,    98,    26,    -1,   124,   164,    26,    84,    26,
     165,    98,    36,    -1,    26,    98,   123,   164,    26,   202,
     220,   165,   187,    -1,    26,    98,   123,   164,    26,   205,
     165,   187,    -1,    26,    98,   164,    26,    84,    26,   165,
      -1,    26,    98,   164,    26,   202,   217,   165,    -1,    26,
      98,   164,    26,   205,   165,    -1,   198,    98,   163,    26,
     160,   220,   158,    26,   160,   220,   162,    -1,   198,    98,
     163,    26,   160,   220,   162,    -1,   163,    26,   160,   220,
     158,    26,   160,   220,   162,    98,   199,    -1,   163,    26,
     160,   220,   162,    98,   199,    -1,   198,    98,    26,    -1,
      24,   220,    -1,    25,    -1,    52,   163,   220,   158,   220,
     162,    26,    -1,    52,   163,   220,   158,   220,   162,    26,
      98,    26,    -1,    52,   163,   220,   158,   220,   162,    26,
      98,    26,    92,   220,    -1,    53,   220,    26,    -1,    53,
     220,    26,    98,    26,    -1,    53,   220,    26,    98,    26,
      92,   220,    -1,   154,    -1,   154,   179,    -1,   154,    26,
      -1,   156,   163,    26,   162,    -1,   153,    -1,   155,   163,
      36,   158,   220,   162,    -1,   152,   163,    26,   158,   220,
     162,    -1,   151,   163,    26,   158,   220,   162,    -1,    30,
      -1,    31,    -1,    -1,   163,   135,   158,   136,   162,    -1,
     163,   136,   158,   135,   162,    -1,   163,   136,   162,    -1,
     163,   135,   162,    -1,   163,   121,   162,    -1,   163,   122,
     162,    -1,    -1,   126,    -1,   127,    -1,   128,    -1,   121,
      -1,   122,    -1,    -1,   163,   182,   162,    -1,    -1,   163,
     125,   162,    -1,   163,   126,   162,    -1,    -1,   163,   183,
     162,    -1,   163,   182,   162,    -1,   163,   183,   158,   182,
     162,    -1,   163,   182,   158,   183,   162,    -1,    -1,   163,
     134,   162,    -1,   163,   133,   162,    -1,    -1,   163,   133,
     162,    -1,   163,   134,   162,    -1,    -1,   163,   125,   162,
      -1,   163,   126,   162,    -1,   163,   143,   162,    -1,   163,
     143,   158,   126,   162,    -1,   163,   126,   158,   143,   162,
      -1,    -1,   163,   143,   162,    -1,    -1,   163,   126,   162,
      -1,   108,    -1,   111,    -1,   110,    -1,   109,    -1,    -1,
     163,   137,   162,    -1,   163,   137,   162,    -1,   163,   136,
     162,    -1,   163,   136,   158,   137,   162,    -1,   163,   137,
     158,   136,   162,    -1,    13,    -1,    14,    -1,    15,    -1,
      -1,   163,   136,   162,    -1,    -1,   163,   136,   162,    -1,
     164,    83,    26,   165,    -1,   164,    26,    84,   165,    -1,
      75,    -1,    76,    -1,    79,    -1,    80,    -1,    81,    -1,
      82,    -1,    71,    -1,    70,    -1,   163,   140,   162,    -1,
     163,   129,   162,    -1,   163,   139,   162,    -1,   163,   130,
     162,    -1,   163,   140,   158,   137,   162,    -1,   163,   129,
     158,   137,   162,    -1,   163,   139,   158,   137,   162,    -1,
     163,   130,   158,   137,   162,    -1,   163,   144,   162,    -1,
     163,   145,   162,    -1,   163,   144,   158,   137,   162,    -1,
     163,   145,   158,   137,   162,    -1,    -1,    84,    -1,    83,
      -1,   179,    98,    -1,   179,   103,    -1,   179,   104,    -1,
      26,    98,   179,    -1,   210,    -1,    26,    98,   163,   210,
     162,    -1,    36,    98,   163,   210,   162,    -1,    36,    98,
     179,    -1,   206,   211,    -1,   208,   211,    -1,   207,   211,
      -1,    36,    72,    36,    -1,    98,    -1,   100,    -1,   102,
      -1,   101,    -1,    28,   212,   166,    -1,    28,   212,   143,
      -1,   166,   212,    28,    -1,   143,   212,    28,    -1,   168,
      -1,   170,    -1,   171,    -1,   172,    -1,   214,   173,   215,
      -1,   216,    -1,   220,    -1,   214,   173,   174,    -1,   169,
      -1,   214,    -1,   163,   221,   162,    -1,    63,   221,    -1,
      70,   221,    -1,   221,    -1,   221,    72,   221,    -1,   221,
      73,   221,    -1,   221,    67,   221,    -1,   221,    71,   221,
      -1,   221,    70,   221,    -1,   221,    91,   221,    -1,   221,
      92,   221,    -1,   221,    65,   221,    -1,   221,    68,   221,
      -1,   221,    66,   221,    -1,   219,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   591,   591,   592,   604,   606,   639,   666,   677,   681,
     719,   739,   744,   754,   764,   769,   774,   790,   806,   818,
     828,   841,   860,   878,   901,   923,   928,   938,   949,   960,
     974,   989,  1005,  1021,  1037,  1048,  1062,  1088,  1106,  1111,
    1117,  1129,  1140,  1151,  1162,  1173,  1184,  1195,  1221,  1235,
    1245,  1290,  1309,  1320,  1331,  1342,  1353,  1364,  1380,  1397,
    1413,  1424,  1435,  1468,  1479,  1492,  1503,  1542,  1552,  1562,
    1582,  1592,  1602,  1612,  1623,  1633,  1643,  1653,  1664,  1688,
    1699,  1705,  1716,  1727,  1738,  1746,  1772,  1802,  1831,  1862,
    1876,  1887,  1901,  1935,  1945,  1955,  1980,  1992,  2010,  2021,
    2032,  2043,  2056,  2067,  2078,  2089,  2100,  2111,  2144,  2154,
    2167,  2187,  2198,  2209,  2222,  2235,  2246,  2257,  2268,  2279,
    2289,  2300,  2311,  2323,  2334,  2345,  2356,  2369,  2381,  2393,
    2404,  2415,  2426,  2438,  2450,  2461,  2472,  2483,  2493,  2499,
    2505,  2511,  2517,  2523,  2529,  2535,  2541,  2547,  2553,  2564,
    2575,  2586,  2597,  2608,  2619,  2630,  2636,  2647,  2658,  2669,
    2680,  2691,  2701,  2714,  2722,  2730,  2754,  2765,  2776,  2787,
    2798,  2809,  2821,  2834,  2843,  2854,  2865,  2877,  2888,  2899,
    2910,  2924,  2936,  2951,  2970,  2981,  2999,  3033,  3051,  3068,
    3079,  3090,  3101,  3122,  3141,  3154,  3168,  3180,  3196,  3241,
    3272,  3288,  3307,  3321,  3340,  3356,  3364,  3373,  3384,  3396,
    3410,  3418,  3428,  3440,  3445,  3450,  3456,  3464,  3470,  3476,
    3482,  3495,  3499,  3509,  3513,  3518,  3523,  3528,  3535,  3539,
    3546,  3550,  3555,  3560,  3568,  3572,  3579,  3583,  3591,  3596,
    3602,  3611,  3616,  3622,  3628,  3634,  3643,  3646,  3650,  3657,
    3660,  3664,  3671,  3676,  3682,  3688,  3694,  3699,  3707,  3710,
    3717,  3720,  3727,  3731,  3735,  3739,  3746,  3749,  3756,  3761,
    3768,  3775,  3787,  3791,  3795,  3802,  3805,  3815,  3818,  3827,
    3833,  3842,  3846,  3853,  3857,  3861,  3865,  3872,  3876,  3883,
    3891,  3899,  3907,  3915,  3922,  3929,  3937,  3947,  3952,  3957,
    3962,  3970,  3973,  3977,  3986,  3993,  4000,  4007,  4022,  4028,
    4041,  4054,  4072,  4079,  4086,  4096,  4109,  4113,  4117,  4121,
    4128,  4134,  4140,  4146,  4156,  4165,  4167,  4169,  4173,  4181,
    4185,  4192,  4198,  4204,  4208,  4212,  4216,  4222,  4228,  4232,
    4236,  4240,  4244,  4248,  4252,  4256,  4260,  4264,  4268
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "BYTEOP16P", "BYTEOP16M", "BYTEOP1P",
  "BYTEOP2P", "BYTEOP2M", "BYTEOP3P", "BYTEUNPACK", "BYTEPACK", "PACK",
  "SAA", "ALIGN8", "ALIGN16", "ALIGN24", "VIT_MAX", "EXTRACT", "DEPOSIT",
  "EXPADJ", "SEARCH", "ONES", "SIGN", "SIGNBITS", "LINK", "UNLINK", "REG",
  "PC", "CCREG", "BYTE_DREG", "REG_A_DOUBLE_ZERO", "REG_A_DOUBLE_ONE",
  "A_ZERO_DOT_L", "A_ZERO_DOT_H", "A_ONE_DOT_L", "A_ONE_DOT_H", "HALF_REG",
  "NOP", "RTI", "RTS", "RTX", "RTN", "RTE", "HLT", "IDLE", "STI", "CLI",
  "CSYNC", "SSYNC", "EMUEXCPT", "RAISE", "EXCPT", "LSETUP", "LOOP",
  "LOOP_BEGIN", "LOOP_END", "DISALGNEXCPT", "JUMP", "JUMP_DOT_S",
  "JUMP_DOT_L", "CALL", "ABORT", "NOT", "TILDA", "BANG", "AMPERSAND",
  "BAR", "PERCENT", "CARET", "BXOR", "MINUS", "PLUS", "STAR", "SLASH",
  "NEG", "MIN", "MAX", "ABS", "DOUBLE_BAR", "_PLUS_BAR_PLUS",
  "_PLUS_BAR_MINUS", "_MINUS_BAR_PLUS", "_MINUS_BAR_MINUS", "_MINUS_MINUS",
  "_PLUS_PLUS", "SHIFT", "LSHIFT", "ASHIFT", "BXORSHIFT",
  "_GREATER_GREATER_GREATER_THAN_ASSIGN", "ROT", "LESS_LESS",
  "GREATER_GREATER", "_GREATER_GREATER_GREATER", "_LESS_LESS_ASSIGN",
  "_GREATER_GREATER_ASSIGN", "DIVS", "DIVQ", "ASSIGN", "_STAR_ASSIGN",
  "_BAR_ASSIGN", "_CARET_ASSIGN", "_AMPERSAND_ASSIGN", "_MINUS_ASSIGN",
  "_PLUS_ASSIGN", "_ASSIGN_BANG", "_LESS_THAN_ASSIGN", "_ASSIGN_ASSIGN",
  "GE", "LT", "LE", "GT", "LESS_THAN", "FLUSHINV", "FLUSH", "IFLUSH",
  "PREFETCH", "PRNT", "OUTC", "WHATREG", "TESTSET", "ASL", "ASR", "B", "W",
  "NS", "S", "CO", "SCO", "TH", "TL", "BP", "BREV", "X", "Z", "M", "MMOD",
  "R", "RND", "RNDL", "RNDH", "RND12", "RND20", "V", "LO", "HI", "BITTGL",
  "BITCLR", "BITSET", "BITTST", "BITMUX", "DBGAL", "DBGAH", "DBGHALT",
  "DBG", "DBGA", "DBGCMPLX", "IF", "COMMA", "BY", "COLON", "SEMICOLON",
  "RPAREN", "LPAREN", "LBRACK", "RBRACK", "STATUS_REG", "MNOP", "SYMBOL",
  "NUMBER", "GOT", "GOT17M4", "FUNCDESC_GOT17M4", "AT", "PLTPC", "$accept",
  "statement", "asm", "asm_1", "REG_A", "opt_mode", "asr_asl", "sco",
  "asr_asl_0", "amod0", "amod1", "amod2", "xpmod", "xpmod1", "vsmod",
  "vmod", "smod", "searchmod", "aligndir", "byteop_mod", "c_align",
  "w32_or_nothing", "iu_or_nothing", "reg_with_predec", "reg_with_postinc",
  "min_max", "op_bar_op", "plus_minus", "rnd_op", "b3_op", "post_op",
  "a_assign", "a_minusassign", "a_plusassign", "assign_macfunc",
  "a_macfunc", "multiply_halfregs", "cc_op", "ccstat", "symbol",
  "any_gotrel", "got", "got_or_expr", "pltpc", "eterm", "expr", "expr_1", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   175,   176,   176,   177,   177,   177,   177,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   179,   179,   180,   180,   180,   180,   180,   181,   181,
     182,   182,   182,   182,   183,   183,   184,   184,   185,   185,
     185,   186,   186,   186,   186,   186,   187,   187,   187,   188,
     188,   188,   189,   189,   189,   189,   189,   189,   190,   190,
     191,   191,   192,   192,   192,   192,   193,   193,   194,   194,
     194,   194,   195,   195,   195,   196,   196,   197,   197,   198,
     199,   200,   200,   201,   201,   201,   201,   202,   202,   203,
     203,   203,   203,   203,   203,   203,   203,   204,   204,   204,
     204,   205,   205,   205,   206,   207,   208,   209,   209,   209,
     209,   209,   210,   210,   210,   211,   212,   212,   212,   212,
     213,   213,   213,   213,   214,   215,   215,   215,   216,   217,
     217,   218,   219,   219,   219,   219,   219,   220,   221,   221,
     221,   221,   221,   221,   221,   221,   221,   221,   221
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     1,     2,     6,     4,     1,     1,     2,
       5,     1,     6,     6,     3,     3,    17,    17,    11,    11,
      11,    12,    12,    12,     5,     3,     3,     3,     8,    13,
      12,    13,    13,    13,     8,    17,     6,     9,     3,     6,
       3,     5,     6,     8,     8,     2,     2,     4,     3,     2,
       4,     3,     6,     4,     7,     7,     3,     3,     6,     3,
       4,     3,     3,     3,    11,    11,     9,     5,     5,     9,
       5,     5,     6,     6,     5,     5,     5,     6,     6,     5,
       1,     3,     3,     3,     3,     4,     4,     9,     9,     5,
       7,     4,     6,     5,     6,     7,     9,     8,     8,    11,
       9,     4,     5,     6,     7,     6,     4,     6,     5,     6,
       6,     4,     8,    10,    10,    12,     5,     6,     5,     6,
       4,     4,     4,     7,     9,     9,     9,     6,     6,     6,
       8,     8,     6,     5,     5,     8,     4,     7,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       4,     4,     6,     6,     2,     2,     4,     2,     2,     2,
       2,     2,     2,     6,     6,     5,     4,     3,     3,     3,
       3,     3,     3,     4,     2,     4,     2,     4,     2,     4,
       2,     7,     8,     8,     7,     7,     7,     9,     7,     8,
       9,     8,     6,     7,     8,     9,     8,     7,     7,     6,
      11,     7,    11,     7,     3,     2,     1,     7,     9,    11,
       3,     5,     7,     1,     2,     2,     4,     1,     6,     6,
       6,     1,     1,     0,     5,     5,     3,     3,     3,     3,
       0,     1,     1,     1,     1,     1,     0,     3,     0,     3,
       3,     0,     3,     3,     5,     5,     0,     3,     3,     0,
       3,     3,     0,     3,     3,     3,     5,     5,     0,     3,
       0,     3,     1,     1,     1,     1,     0,     3,     3,     3,
       5,     5,     1,     1,     1,     0,     3,     0,     3,     4,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     5,     5,     5,     5,     3,     3,     5,
       5,     0,     1,     1,     2,     2,     2,     3,     1,     5,
       5,     3,     2,     2,     2,     3,     1,     1,     1,     1,
       3,     3,     3,     3,     1,     1,     1,     1,     3,     1,
       1,     3,     1,     1,     3,     2,     2,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       0,     7,     0,     0,   206,     0,     0,   221,   222,     0,
       0,     0,     0,     0,   138,   140,   139,   141,   142,   143,
     144,     0,     0,   145,   146,   147,     0,     0,     0,     0,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   217,   213,     0,     0,     0,     0,     0,     0,     8,
       0,     3,     0,     0,     0,     0,     0,     0,   223,   308,
      80,     0,     0,     0,     0,   324,   332,   333,   348,   205,
     337,     0,     0,     0,     0,     0,     0,     0,   316,   317,
     319,   318,     0,     0,     0,     0,     0,     0,     0,   149,
     148,   154,   155,     0,     0,     0,   157,   158,   333,   160,
     159,     0,   162,   161,     0,     0,     0,   176,     0,   174,
       0,   178,     0,   180,     0,     0,     0,   316,     0,     0,
       0,     0,     0,     0,     0,   215,   214,     0,     0,     0,
       0,     0,   301,     0,     0,     1,     0,     4,   304,   305,
     306,     0,    46,     0,     0,     0,     0,     0,     0,     0,
      45,     0,   312,    49,   275,   314,   313,     0,     9,     0,
     335,   336,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   169,   172,   170,   171,   167,   168,     0,
       0,     0,     0,     0,     0,   272,   273,   274,     0,     0,
       0,    81,    83,   246,     0,   246,     0,     0,   281,   282,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   307,
       0,     0,   223,   249,    63,    59,    57,    61,    62,    82,
       0,     0,    84,     0,   321,   320,    26,    14,    27,    15,
       0,     0,     0,     0,    51,     0,     0,     0,     0,     0,
       0,   311,   223,    48,     0,   210,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   301,   301,
     323,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   288,   287,   303,   302,     0,     0,
       0,   322,     0,   275,   204,     0,     0,    38,    25,     0,
       0,     0,     0,     0,     0,     0,     0,    40,     0,    56,
       0,     0,     0,     0,   334,   345,   347,   340,   346,   342,
     341,   338,   339,   343,   344,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   287,   283,   284,
     285,   286,     0,     0,     0,     0,     0,     0,    53,     0,
      47,   166,   252,   258,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   301,     0,     0,     0,
      86,     0,    50,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   111,   121,   122,   120,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    85,     0,     0,   150,     0,   331,   151,     0,     0,
       0,     0,   175,   173,   177,   179,   156,   302,     0,     0,
     302,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     216,     0,   136,     0,     0,     0,     0,     0,     0,     0,
     279,     0,     6,    60,     0,   315,     0,     0,     0,     0,
       0,     0,    91,   106,   101,     0,     0,     0,   227,     0,
     226,     0,     0,   223,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    79,    67,    68,     0,   252,   258,
     252,   236,   238,     0,     0,     0,     0,   165,     0,    24,
       0,     0,     0,     0,   301,   301,     0,   306,     0,   309,
     302,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     277,   277,    74,    75,   277,   277,     0,    76,    70,    71,
       0,     0,     0,     0,     0,     0,     0,     0,    93,   108,
     260,     0,   238,     0,     0,   301,     0,   310,     0,     0,
     211,     0,     0,     0,     0,   280,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   133,     0,
       0,   134,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   102,    89,     0,   116,   118,    41,   276,
       0,     0,     0,     0,    10,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    92,   107,   110,     0,
     230,    52,     0,     0,    36,   248,   247,     0,     0,     0,
       0,     0,   105,   258,   252,   117,   119,     0,     0,   302,
       0,     0,     0,    12,     0,   333,   329,     0,   330,   199,
       0,     0,     0,     0,   250,   251,    58,     0,    77,    78,
      72,    73,     0,     0,     0,     0,     0,    42,     0,     0,
       0,     0,    94,   109,     0,    39,   103,   260,   302,     0,
      13,     0,     0,     0,   153,   152,   164,   163,     0,     0,
       0,     0,     0,   129,   127,   128,     0,   220,   219,   218,
       0,   132,     0,     0,     0,     0,     0,     0,   192,     5,
       0,     0,     0,     0,     0,   224,   225,     0,   307,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   231,   232,   233,     0,     0,     0,     0,     0,
     253,     0,   254,     0,   255,   259,   104,    95,     0,   246,
       0,     0,   246,     0,   197,     0,   198,     0,     0,     0,
       0,     0,     0,     0,     0,   123,     0,     0,     0,     0,
       0,     0,     0,     0,    90,     0,   188,     0,   207,   212,
       0,   181,     0,     0,   184,   185,     0,   137,     0,     0,
       0,     0,     0,     0,     0,   203,   193,   186,     0,   201,
      55,    54,     0,     0,     0,     0,     0,     0,     0,    34,
     112,     0,   246,    98,     0,     0,   237,     0,   239,   240,
       0,     0,     0,   246,   196,   246,   246,   189,     0,   325,
     326,   327,   328,     0,    28,   258,   223,   278,   131,   130,
       0,     0,   258,    97,    43,    44,     0,     0,   261,     0,
     191,   223,     0,   182,   194,   183,     0,   135,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   124,   100,     0,    69,     0,     0,     0,   257,
     256,   195,   190,   187,    66,     0,    37,    88,   228,   229,
      96,     0,     0,     0,     0,    87,   208,   125,     0,     0,
       0,     0,     0,     0,   126,     0,   266,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   114,     0,   113,     0,
       0,     0,     0,   266,   262,   265,   264,   263,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,    99,   241,
     238,    20,   238,     0,     0,   209,     0,     0,    18,    19,
     202,   200,    65,     0,    30,     0,     0,     0,   230,    23,
      22,    21,   115,     0,     0,     0,   267,     0,    29,     0,
      31,    32,     0,    33,   234,   235,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     243,   230,   242,     0,     0,     0,     0,   269,     0,   268,
       0,   290,     0,   292,     0,   291,     0,   289,     0,   297,
       0,   298,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   245,   244,     0,   266,   266,
     270,   271,   294,   296,   295,   293,   299,   300,    35,    16,
      17
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    60,    61,    62,   361,   168,   745,   715,   957,   601,
     604,   939,   348,   372,   487,   489,   652,   908,   913,   948,
     220,   309,   638,    64,   117,   221,   345,   288,   950,   953,
     289,   362,   363,    67,    68,    69,   166,    93,    70,    77,
     812,   626,   627,   109,    78,    79,    80
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -907
static const short int yypact[] =
{
     878,  -907,  -120,   291,  -907,   614,   439,  -907,  -907,   -35,
     -11,    31,    61,    81,  -907,  -907,  -907,  -907,  -907,  -907,
    -907,   183,   202,  -907,  -907,  -907,   291,   291,   -56,   291,
    -907,   351,   291,   291,   354,    96,   111,    86,   160,   162,
     168,   122,   170,   172,   318,   134,   146,   196,   200,   217,
     228,  -907,   230,   233,   252,    46,   207,    26,   318,  -907,
     412,  -907,   -48,   195,   324,   270,   274,   394,   271,  -907,
    -907,   418,   291,   291,   291,  -907,  -907,  -907,  -907,  -907,
     561,    55,    69,    71,   482,   421,    80,   107,    70,  -907,
    -907,  -907,     5,  -107,   414,   419,   422,   440,   121,  -907,
    -907,  -907,  -907,   291,   465,    42,  -907,  -907,   321,  -907,
    -907,    52,  -907,  -907,   479,   489,   495,  -907,   502,  -907,
     520,  -907,   558,  -907,   560,   569,   593,  -907,   594,   599,
     604,   609,   630,   640,   643,  -907,  -907,   638,   650,    27,
     657,   -22,   237,   663,   667,  -907,  1024,  -907,  -907,  -907,
     186,     9,  -907,   568,   272,   186,   186,   186,   547,   186,
     174,   291,  -907,  -907,   548,  -907,  -907,   147,   556,   555,
    -907,  -907,   459,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,  -907,  -907,  -907,  -907,  -907,  -907,   562,
     563,   564,   565,   567,   570,  -907,  -907,  -907,   572,   573,
     574,   508,  -907,   575,   645,   -52,   159,   201,  -907,  -907,
     697,   698,   703,   705,   706,   576,   577,    20,   708,   668,
     579,   580,   271,   581,  -907,  -907,  -907,   582,  -907,   239,
     583,   320,  -907,   585,  -907,  -907,  -907,  -907,  -907,  -907,
     586,   587,   725,   145,    15,   654,   411,   717,   718,   591,
     272,  -907,   271,  -907,   598,   659,   597,   689,   588,   601,
     690,   606,   607,   -46,   -39,   -37,    -4,   605,   301,   323,
    -907,   610,   613,   615,   616,   617,   618,   619,   621,   671,
     291,    47,   740,   291,  -907,  -907,  -907,   744,   291,   620,
     622,  -907,   -47,   548,  -907,   746,   742,   623,   626,   627,
     629,   186,   631,   291,   291,   291,   653,  -907,   644,  -907,
     -42,   -31,   374,   291,  -907,   608,   571,  -907,   600,   397,
     397,  -907,  -907,   510,   510,   763,   765,   766,   768,   769,
     760,   771,   772,   773,   774,   775,   776,   641,  -907,  -907,
    -907,  -907,   291,   291,   291,   777,   779,   219,  -907,   778,
    -907,  -907,   646,   647,   648,   649,   652,   655,   780,   786,
     745,   403,   394,   394,   274,   656,   382,   186,   789,   791,
     661,   249,  -907,   688,   218,   260,   268,   795,   186,   186,
     186,   796,   798,   154,  -907,  -907,  -907,  -907,   687,   800,
      33,   291,   291,   291,   805,   792,   670,   672,   804,   274,
     673,   674,   291,   807,  -907,   808,  -907,  -907,   811,   812,
     813,   675,  -907,  -907,  -907,  -907,  -907,  -907,   291,   676,
     816,   291,   678,   291,   291,   291,   818,   291,   291,   291,
    -907,   819,   683,   749,   291,   686,     0,   684,   685,   753,
    -907,  1024,  -907,  -907,   692,  -907,   186,   186,   817,   820,
     700,   210,  -907,  -907,  -907,   693,   704,   723,  -907,   730,
    -907,   762,   770,   271,   709,   710,   711,   712,   713,   719,
     722,   726,   727,   728,  -907,  -907,  -907,   843,   646,   647,
     646,    24,   105,   721,   729,   731,    87,  -907,   738,  -907,
     851,   852,   856,   305,   301,   464,   867,  -907,   732,  -907,
     869,   291,   733,   739,   741,   743,   870,   759,   781,   782,
     737,   737,  -907,  -907,   737,   737,   783,  -907,  -907,  -907,
     784,   785,   787,   788,   790,   793,   794,   797,   799,  -907,
     799,   801,   802,   871,   896,   481,   806,  -907,   897,   809,
     847,   810,   814,   815,   821,  -907,   822,   842,   823,   824,
     853,   828,   833,   834,   803,   835,   837,   838,  -907,   827,
     924,   840,   855,   928,   857,   861,   865,   940,   825,   291,
     872,   899,   890,  -907,  -907,   186,  -907,  -907,   826,  -907,
     844,   845,    11,    40,  -907,   947,   291,   291,   291,   291,
     952,   943,   954,   945,   956,   894,  -907,  -907,  -907,   978,
     428,  -907,   979,   487,  -907,  -907,  -907,   982,   848,    44,
      73,   849,  -907,   647,   646,  -907,  -907,   291,   850,   983,
     291,   854,   858,  -907,   862,   839,  -907,   873,  -907,  -907,
     987,   988,   990,   919,  -907,  -907,  -907,   882,  -907,  -907,
    -907,  -907,   291,   291,   859,  1001,  1003,  -907,   513,   186,
     186,   914,  -907,  -907,  1007,  -907,  -907,   799,   997,   881,
    -907,   949,  1025,   291,  -907,  -907,  -907,  -907,   955,  1041,
     980,   981,   178,  -907,  -907,  -907,   186,  -907,  -907,  -907,
     923,  -907,   957,   238,   926,   925,  1061,  1064,  -907,  -907,
     133,   186,   186,   933,   186,  -907,  -907,   186,  -907,   186,
     932,   935,   936,   937,   938,   939,   941,   942,   944,   946,
     291,   999,  -907,  -907,  -907,   948,  1000,   950,   951,  1002,
    -907,   959,  -907,   973,  -907,  -907,  -907,  -907,   953,   575,
     958,   960,   575,  1014,  -907,   477,  -907,  1009,   962,   964,
     394,   965,   966,   967,   566,  -907,   968,   969,   970,   971,
     961,   976,   974,   984,  -907,   977,  -907,   394,  1011,  -907,
    1085,  -907,  1078,  1089,  -907,  -907,   989,  -907,   991,   972,
     986,  1090,  1091,   291,  1096,  -907,  -907,  -907,  1115,  -907,
    -907,  -907,  1117,   186,   291,  1124,  1126,  1128,  1129,  -907,
    -907,   859,   575,   993,   995,  1132,  -907,  1133,  -907,  -907,
    1111,   998,  1004,   575,  -907,   575,   575,  -907,   291,  -907,
    -907,  -907,  -907,   186,  -907,   647,   271,  -907,  -907,  -907,
    1006,  1020,   647,  -907,  -907,  -907,   313,  1135,  -907,  1092,
    -907,   271,  1136,  -907,  -907,  -907,   859,  -907,  1137,  1139,
    1013,  1021,  1023,  1099,  1026,  1027,  1029,  1031,  1034,  1035,
    1036,  1037,  -907,  -907,  1065,  -907,   398,   612,  1098,  -907,
    -907,  -907,  -907,  -907,  -907,  1130,  -907,  -907,  -907,  -907,
    -907,  1039,  1044,  1042,  1163,  -907,  1113,  -907,  1043,  1046,
     291,   611,  1109,   291,  -907,  1082,  1047,   291,   291,   291,
     291,  1049,  1183,  1186,  1180,   186,  -907,  1187,  -907,  1143,
     291,   291,   291,  1047,  -907,  -907,  -907,  -907,  1054,   925,
    1055,  1056,  1083,  -907,  1057,  1059,  1060,  1062,  -907,  1063,
     802,  -907,   802,  1066,  1201,  -907,  1067,  1069,  -907,  -907,
    -907,  -907,  -907,  1068,  1070,  1071,  1071,  1072,   476,  -907,
    -907,  -907,  -907,  1073,  1203,  1205,  -907,   557,  -907,   332,
    -907,  -907,   553,  -907,  -907,  -907,   167,   380,  1196,  1077,
    1079,   391,   408,   413,   453,   456,   462,   499,   506,   584,
    -907,   428,  -907,  1076,   291,   291,  1103,  -907,  1105,  -907,
    1106,  -907,  1107,  -907,  1108,  -907,  1110,  -907,  1112,  -907,
    1114,  -907,  1080,  1084,  1176,  1088,  1093,  1094,  1095,  1097,
    1100,  1101,  1102,  1104,  1116,  -907,  -907,  1216,  1047,  1047,
    -907,  -907,  -907,  -907,  -907,  -907,  -907,  -907,  -907,  -907,
    -907
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -907,  -907,  -907,  -122,    10,  -208,  -737,  -906,   284,  -907,
    -509,  -907,  -196,  -907,  -451,  -460,  -502,  -907,  -863,  -907,
    -907,   975,   -69,  -907,   -27,  -907,   402,  -180,   325,  -907,
    -243,     2,    22,  -168,   963,  -213,   -50,    59,  -907,   -16,
    -907,  -907,  -907,  1220,  -907,   -26,    19
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -3
static const short int yytable[] =
{
     101,   102,    65,   104,   365,   106,   107,   110,   113,   350,
      63,   119,   121,   123,   370,   162,   165,   108,   108,   597,
     296,   346,    66,   655,   292,   419,   422,   596,   653,   598,
     146,   441,   956,   232,   222,   294,   234,   400,   411,   163,
     928,     7,     8,    71,   401,   411,   360,   411,   252,   364,
       7,     8,   142,   279,   852,   184,   186,   188,   223,   235,
     226,   228,   136,    94,   389,   993,   395,   161,   256,   257,
       7,     8,   253,   433,   139,   160,   164,   254,   259,   260,
     411,   183,   399,    72,   280,   284,   285,    95,   418,   421,
      73,   170,   171,   172,   219,   185,   229,   187,   231,   877,
       7,     8,   526,   128,   434,    72,   225,   103,   251,   143,
     140,   347,    73,   147,   442,    72,   457,   144,    72,   412,
     458,   527,    73,   502,   172,    73,   413,   459,   414,    96,
     172,   460,    72,   227,    72,   307,   282,   240,   283,    73,
     241,    73,   242,    72,   243,  1019,  1020,   244,    65,   245,
      73,     7,     8,   726,   233,   754,    63,   246,   563,    97,
     293,   415,   564,   727,   297,   298,   299,   300,    66,   302,
      72,   385,   295,   171,   697,     7,     8,    73,   388,    98,
     522,   386,   599,    74,    72,   351,   501,   600,    75,    76,
     523,    73,   315,   316,   317,   318,   319,   320,   321,   322,
     323,   324,   721,   699,   764,    74,   722,   247,   248,    99,
      75,    76,   608,   609,   765,    74,     7,     8,    74,   230,
      75,    76,    72,    75,    76,   170,   171,   352,   100,    73,
     610,   723,    74,   141,    74,   724,   172,    75,    76,    75,
      76,   769,   770,    74,   510,   249,   576,   771,    75,    76,
     116,   618,   621,   387,   432,   584,   135,   436,   772,   114,
       7,     8,   438,   602,    72,   303,   304,   305,   603,   172,
      74,    73,   148,    72,   115,    75,    76,   452,   453,   454,
      73,    72,   310,   311,   250,   124,   512,   464,    73,    75,
      76,   778,   659,   148,   514,   779,   152,   129,   149,   150,
       7,     8,     7,     8,     7,     8,   153,   284,   285,   130,
     153,   450,   162,   165,   617,   620,   478,   479,   480,   568,
     286,   287,    74,    72,   118,   969,   120,    75,    76,   970,
      73,    72,   122,    72,   125,    72,   126,   306,    73,   871,
     154,   615,    73,     7,     8,   374,   375,   155,   511,   513,
     515,   376,   483,   484,    72,   866,   156,   157,   158,   131,
     159,    73,   870,   132,    74,   528,   529,   530,    72,    75,
      76,   284,   285,    74,   498,    73,   539,   503,    75,    76,
     133,    74,   507,   508,   286,   417,    75,    76,   517,   518,
     519,   134,   546,   284,   285,   549,   137,   551,   552,   553,
     461,   555,   556,   557,     7,     8,   286,   420,   561,   536,
     462,   940,   145,   941,    72,   138,   127,    72,    89,    90,
      91,    73,   151,    74,    73,   577,   378,   379,    75,    76,
     153,    74,   380,    74,   167,    74,    75,    76,    75,    76,
      75,    76,   639,    65,   169,   640,   641,   224,   571,   572,
     236,    63,   284,   285,    74,   237,   570,   570,   238,    75,
      76,   963,   964,    66,   175,   286,   500,   616,    74,   179,
     180,   965,   966,    75,    76,   628,   239,   338,   339,   340,
     341,   284,   285,   296,   365,   625,   400,   189,   190,   191,
     192,   255,   193,   194,   258,   195,   196,   197,   198,   199,
     200,   148,   391,   392,   393,   261,   149,   497,   201,   394,
     202,   203,     7,     8,   105,   262,   204,   111,   205,    75,
      76,   263,    75,    76,   173,   174,   175,   176,   264,   177,
     178,   179,   180,   804,   284,   285,   807,    88,   971,    89,
      90,    91,   972,   690,    92,   206,   265,   286,   619,   976,
     181,   182,   207,   977,   712,   713,   714,   208,   209,   210,
     701,   702,   703,   704,   286,   658,   978,   211,   212,   213,
     979,   980,   214,   334,   335,   981,   336,   175,   284,   337,
     177,   178,   179,   180,   266,   693,   267,   338,   339,   340,
     341,   728,   698,   251,   731,   268,   853,   954,   955,   342,
     343,   344,   712,   713,   714,   215,   216,   861,   867,   862,
     863,   982,   717,   718,   984,   983,   742,   743,   985,   269,
     986,   314,   270,   875,   987,   271,   173,   174,   175,   176,
     272,   177,   178,   179,   180,   273,   173,   759,   175,   176,
     296,   177,   178,   179,   180,   217,   218,   809,   810,   811,
      75,    76,   181,   182,   748,   749,   274,   988,   775,   750,
     751,   989,   181,   182,   990,   173,   275,   175,   991,   276,
     177,   178,   179,   180,   277,   175,   278,   893,   177,   178,
     179,   180,   284,   285,   794,   281,   766,   820,   821,   290,
     816,   181,   182,   961,   962,   291,   783,   967,   968,   181,
     182,   780,   781,    81,   570,   954,   955,   831,    82,    83,
     301,   308,    84,    85,   312,   313,   349,    86,    87,   904,
     905,   906,   907,   353,   354,   325,   326,   327,   328,   355,
     329,   356,   357,   330,   366,   331,   332,   333,   347,   367,
     358,   359,   368,   369,   371,   373,   377,   842,   381,   382,
     383,   384,   390,   396,   397,   398,   402,   403,   847,   404,
     405,   408,   406,   407,   409,   410,   435,   416,   423,   431,
     437,   424,   444,   425,   426,   427,   428,   429,   445,   455,
     456,   446,   864,   430,   447,   439,   448,   440,   449,   465,
     451,   466,   467,   846,   468,   469,   470,   471,   472,   473,
     474,   475,   476,   481,   477,   482,   494,   490,   491,   486,
     488,   492,   495,   485,   493,   504,   496,   505,   499,   506,
     509,   516,   520,   865,   521,   524,   525,   531,   532,   533,
     535,   534,   538,   540,   541,   537,   872,   542,   543,   544,
     545,   547,   548,   550,   554,   558,   559,   560,   562,   565,
     566,   567,   569,   573,   903,   578,   574,   910,   575,   580,
     582,   914,   915,   916,   917,   581,   579,   585,   583,   595,
     586,   587,   588,   589,   925,   926,   927,   590,    -2,     1,
     591,   611,   930,   605,   592,   593,   594,   612,   613,   607,
       2,   606,   614,   622,   623,   624,   633,   630,   629,   631,
     637,   632,     3,     4,     5,   922,     6,   656,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,   634,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,   657,   661,    30,    31,    32,    33,    34,   663,
     669,   642,   643,   635,   636,   645,   646,   644,   995,   996,
     681,   672,   647,   683,   684,   685,   648,   649,   680,   686,
     650,   676,   651,   687,   654,   603,   688,   692,   660,   691,
     148,   662,   664,   700,    35,    36,   665,   666,   705,   706,
     707,   708,   709,   667,   694,   710,   689,   668,   670,   671,
     673,    37,    38,    39,    40,   674,   675,   677,    41,   678,
     679,    42,    43,   682,   711,   716,   695,   696,   719,   730,
     720,   725,   735,   737,   738,   729,   739,   740,   741,   732,
     733,    44,   744,   755,    45,    46,    47,   734,    48,    49,
      50,    51,    52,    53,    54,    55,     2,   746,   736,   747,
     752,    56,    57,   753,    58,    59,   756,   757,     3,     4,
       5,   758,     6,   760,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,   761,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,   762,   763,
      30,    31,    32,    33,    34,   767,   773,   776,   768,   774,
     777,   782,   784,   785,   786,   787,   788,   795,   797,   802,
     800,   789,   801,   790,   791,   808,   792,   813,   793,   832,
     796,   833,   798,   799,   834,   835,   840,   841,   803,   826,
      35,    36,   843,   805,   814,   806,   815,   817,   818,   819,
     822,   823,   824,   825,   827,   838,   828,    37,    38,    39,
      40,   844,   830,   858,    41,   845,   829,    42,    43,   839,
     848,   836,   849,   837,   850,   851,   854,   855,   856,   857,
     859,   873,   876,   878,   874,   879,   860,    44,   868,   894,
      45,    46,    47,   880,    48,    49,    50,    51,    52,    53,
      54,    55,   869,   411,   881,   882,   883,    56,    57,   884,
      58,    59,   885,   886,   887,   888,   889,   890,   891,   899,
     895,   896,   897,   901,   898,   900,   902,   909,   911,   919,
     912,   918,   920,   921,   924,   923,   929,   931,   932,   934,
     933,   935,   936,   943,   937,   944,   938,   945,   942,   959,
     946,   960,   973,   947,   949,   952,   958,   974,   994,   975,
     997,   998,  1005,   999,  1000,  1001,  1006,  1002,  1007,  1003,
    1008,  1004,  1018,   992,   112,  1009,  1010,  1011,   892,  1012,
       0,   951,  1013,  1014,  1015,     0,  1016,     0,   443,     0,
       0,     0,     0,     0,     0,   463,     0,     0,  1017
};

static const short int yycheck[] =
{
      26,    27,     0,    29,   217,    31,    32,    33,    34,   205,
       0,    38,    39,    40,   222,    65,    66,    33,    34,   479,
      72,   201,     0,   532,   146,   268,   269,   478,   530,   480,
      78,    78,   938,    28,    84,    26,   143,   250,    84,    65,
     903,    30,    31,   163,   252,    84,    26,    84,    98,   217,
      30,    31,    26,    26,   791,    81,    82,    83,    84,   166,
      86,    87,    52,    98,   244,   971,   246,    65,    26,    27,
      30,    31,    98,    26,    28,    65,    66,   103,    26,    27,
      84,    26,   250,    63,    57,    70,    71,    98,   268,   269,
      70,    72,    73,    74,    84,    26,    26,    26,    88,   836,
      30,    31,    69,    44,    57,    63,    26,   163,    98,    83,
      64,   163,    70,   161,   161,    63,   158,    58,    63,   165,
     162,    88,    70,   366,   105,    70,   165,   158,   165,    98,
     111,   162,    63,    26,    63,   161,   158,    16,   160,    70,
      19,    70,    21,    63,    23,  1008,  1009,    26,   146,    28,
      70,    30,    31,   613,   149,   657,   146,    36,   158,    98,
     150,   165,   162,   614,   154,   155,   156,   157,   146,   159,
      63,    26,   163,   154,   163,    30,    31,    70,   163,    98,
      26,    36,   158,   163,    63,    26,   366,   163,   168,   169,
      36,    70,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   158,   163,    26,   163,   162,    86,    87,    26,
     168,   169,   125,   126,    36,   163,    30,    31,   163,   149,
     168,   169,    63,   168,   169,   206,   207,    26,    26,    70,
     143,   158,   163,    26,   163,   162,   217,   168,   169,   168,
     169,     3,     4,   163,    26,   124,    36,     9,   168,   169,
     164,   494,   495,   243,   280,   463,    26,   283,    20,   163,
      30,    31,   288,   158,    63,    91,    92,    93,   163,   250,
     163,    70,    98,    63,   163,   168,   169,   303,   304,   305,
      70,    63,   135,   136,   163,   163,    26,   313,    70,   168,
     169,   158,   535,    98,    26,   162,    26,   163,   103,   104,
      30,    31,    30,    31,    30,    31,    36,    70,    71,   163,
      36,   301,   362,   363,   494,   495,   342,   343,   344,   441,
      83,    84,   163,    63,   164,   158,   164,   168,   169,   162,
      70,    63,   164,    63,   164,    63,   164,   163,    70,    26,
      70,    36,    70,    30,    31,   106,   107,    77,   374,   375,
     376,   112,   133,   134,    63,   815,    86,    87,    88,   163,
      90,    70,   822,   163,   163,   391,   392,   393,    63,   168,
     169,    70,    71,   163,   364,    70,   402,   367,   168,   169,
     163,   163,   133,   134,    83,    84,   168,   169,   378,   379,
     380,   163,   418,    70,    71,   421,   163,   423,   424,   425,
      26,   427,   428,   429,    30,    31,    83,    84,   434,   399,
      36,   920,     0,   922,    63,   163,    98,    63,   100,   101,
     102,    70,    98,   163,    70,   451,   106,   107,   168,   169,
      36,   163,   112,   163,   163,   163,   168,   169,   168,   169,
     168,   169,   511,   441,    26,   514,   515,    26,   446,   447,
      36,   441,    70,    71,   163,    36,   446,   447,    36,   168,
     169,   129,   130,   441,    67,    83,    84,   493,   163,    72,
      73,   139,   140,   168,   169,   501,    36,    79,    80,    81,
      82,    70,    71,    72,   697,   501,   699,     5,     6,     7,
       8,    26,    10,    11,   173,    13,    14,    15,    16,    17,
      18,    98,    91,    92,    93,    26,   103,   104,    26,    98,
      28,    29,    30,    31,   163,    26,    34,   163,    36,   168,
     169,    26,   168,   169,    65,    66,    67,    68,    26,    70,
      71,    72,    73,   729,    70,    71,   732,    98,   158,   100,
     101,   102,   162,   569,   105,    63,    26,    83,    84,   158,
      91,    92,    70,   162,   126,   127,   128,    75,    76,    77,
     586,   587,   588,   589,    83,    84,   158,    85,    86,    87,
     162,   158,    90,    65,    66,   162,    68,    67,    70,    71,
      70,    71,    72,    73,    26,   575,    26,    79,    80,    81,
      82,   617,   582,   583,   620,    26,   792,   121,   122,    91,
      92,    93,   126,   127,   128,   123,   124,   803,   816,   805,
     806,   158,   125,   126,   158,   162,   642,   643,   162,    26,
     158,   162,    28,   831,   162,    26,    65,    66,    67,    68,
      26,    70,    71,    72,    73,    26,    65,   663,    67,    68,
      72,    70,    71,    72,    73,   163,   164,   170,   171,   172,
     168,   169,    91,    92,   141,   142,    26,   158,   685,   649,
     650,   162,    91,    92,   158,    65,    26,    67,   162,    26,
      70,    71,    72,    73,    36,    67,    26,   857,    70,    71,
      72,    73,    70,    71,   710,    28,   676,   121,   122,    26,
     740,    91,    92,   136,   137,    28,   694,   144,   145,    91,
      92,   691,   692,    89,   694,   121,   122,   757,    94,    95,
     163,   163,    98,    99,   158,   160,    71,   103,   104,   108,
     109,   110,   111,    26,    26,   163,   163,   163,   163,    26,
     163,    26,    26,   163,    26,   163,   163,   163,   163,    71,
     164,   164,   163,   163,   163,   163,   163,   773,   163,   163,
     163,    26,    98,    36,    36,   164,   158,    98,   784,   162,
      71,    71,   174,   162,   158,   158,    26,   162,   158,    98,
      26,   158,    26,   158,   158,   158,   158,   158,    36,   126,
     136,   158,   808,   162,   158,   165,   159,   165,   159,    26,
     159,    26,    26,   783,    26,    26,    36,    26,    26,    26,
      26,    26,    26,    26,   163,    26,    26,   159,   159,   163,
     163,   159,    26,    35,   159,    26,    71,    26,   162,   158,
     132,    26,    26,   813,    26,   138,    26,    22,    36,   159,
      26,   159,   158,    26,    26,   162,   826,    26,    26,    26,
     165,   165,    26,   165,    26,    26,   163,    98,   162,   165,
     165,    98,   160,    36,   880,   162,    36,   883,   158,   136,
      98,   887,   888,   889,   890,   135,   162,   158,    98,    26,
     160,   160,   160,   160,   900,   901,   902,   158,     0,     1,
     158,   143,   909,   162,   158,   158,   158,    36,    36,   158,
      12,   162,    36,    26,   162,    26,    26,   158,   165,   158,
     163,   158,    24,    25,    26,   895,    28,    36,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,   162,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    36,    36,    56,    57,    58,    59,    60,    92,
      98,   158,   158,   162,   162,   158,   158,   162,   974,   975,
      26,    98,   162,    98,    26,    98,   163,   163,   131,    98,
     163,   158,   163,    98,   163,   163,    26,    77,   162,    70,
      98,   162,   162,    26,    96,    97,   162,   162,    26,    36,
      26,    36,    26,   162,   158,    91,   161,   165,   165,   165,
     162,   113,   114,   115,   116,   162,   162,   162,   120,   162,
     162,   123,   124,   163,    26,    26,   162,   162,    26,    26,
     162,   162,   173,    26,    26,   165,    26,    98,   136,   165,
     162,   143,   163,    26,   146,   147,   148,   165,   150,   151,
     152,   153,   154,   155,   156,   157,    12,    36,   165,    36,
     126,   163,   164,    36,   166,   167,   165,    98,    24,    25,
      26,    26,    28,    98,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    26,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    98,    98,
      56,    57,    58,    59,    60,   162,   160,    26,   131,   164,
      26,   158,   160,   158,   158,   158,   158,    98,    98,   126,
      98,   162,   143,   162,   162,    91,   162,    98,   162,    98,
     162,    26,   162,   162,    36,    26,    26,    26,   165,   158,
      96,    97,    26,   165,   162,   165,   162,   162,   162,   162,
     162,   162,   162,   162,   158,   163,   162,   113,   114,   115,
     116,    26,   165,    32,   120,    28,   162,   123,   124,   163,
      26,   162,    26,   162,    26,    26,   163,   162,    26,    26,
     162,    26,    26,    26,    72,    26,   162,   143,   162,    71,
     146,   147,   148,   160,   150,   151,   152,   153,   154,   155,
     156,   157,   162,    84,   163,   162,   160,   163,   164,   162,
     166,   167,   163,   162,   160,   160,   160,   160,   133,    36,
      70,   162,   158,   160,   162,    92,   160,    98,   126,    26,
     163,   162,    26,    33,    71,    28,   162,   162,   162,   162,
     137,   162,   162,    22,   162,   158,   163,   158,   162,    26,
     162,    26,    36,   163,   163,   163,   163,   160,   162,   160,
     137,   136,   162,   137,   137,   137,   162,   137,    72,   137,
     162,   137,    36,   969,    34,   162,   162,   162,   856,   162,
      -1,   936,   162,   162,   162,    -1,   162,    -1,   293,    -1,
      -1,    -1,    -1,    -1,    -1,   312,    -1,    -1,   162
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     1,    12,    24,    25,    26,    28,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      56,    57,    58,    59,    60,    96,    97,   113,   114,   115,
     116,   120,   123,   124,   143,   146,   147,   148,   150,   151,
     152,   153,   154,   155,   156,   157,   163,   164,   166,   167,
     176,   177,   178,   179,   198,   206,   207,   208,   209,   210,
     213,   163,    63,    70,   163,   168,   169,   214,   219,   220,
     221,    89,    94,    95,    98,    99,   103,   104,    98,   100,
     101,   102,   105,   212,    98,    98,    98,    98,    98,    26,
      26,   220,   220,   163,   220,   163,   220,   220,   214,   218,
     220,   163,   218,   220,   163,   163,   164,   199,   164,   199,
     164,   199,   164,   199,   163,   164,   164,    98,   212,   163,
     163,   163,   163,   163,   163,    26,   179,   163,   163,    28,
      64,    26,    26,    83,   212,     0,    78,   161,    98,   103,
     104,    98,    26,    36,    70,    77,    86,    87,    88,    90,
     179,   206,   211,   220,   179,   211,   211,   163,   180,    26,
     221,   221,   221,    65,    66,    67,    68,    70,    71,    72,
      73,    91,    92,    26,   220,    26,   220,    26,   220,     5,
       6,     7,     8,    10,    11,    13,    14,    15,    16,    17,
      18,    26,    28,    29,    34,    36,    63,    70,    75,    76,
      77,    85,    86,    87,    90,   123,   124,   163,   164,   179,
     195,   200,   211,   220,    26,    26,   220,    26,   220,    26,
     149,   179,    28,   149,   143,   166,    36,    36,    36,    36,
      16,    19,    21,    23,    26,    28,    36,    86,    87,   124,
     163,   179,   211,   220,   220,    26,    26,    27,   173,    26,
      27,    26,    26,    26,    26,    26,    26,    26,    26,    26,
      28,    26,    26,    26,    26,    26,    26,    36,    26,    26,
      57,    28,   158,   160,    70,    71,    83,    84,   202,   205,
      26,    28,   178,   179,    26,   163,    72,   179,   179,   179,
     179,   163,   179,    91,    92,    93,   163,   220,   163,   196,
     135,   136,   158,   160,   162,   221,   221,   221,   221,   221,
     221,   221,   221,   221,   221,   163,   163,   163,   163,   163,
     163,   163,   163,   163,    65,    66,    68,    71,    79,    80,
      81,    82,    91,    92,    93,   201,   202,   163,   187,    71,
     187,    26,    26,    26,    26,    26,    26,    26,   164,   164,
      26,   179,   206,   207,   208,   210,    26,    71,   163,   163,
     180,   163,   188,   163,   106,   107,   112,   163,   106,   107,
     112,   163,   163,   163,    26,    26,    36,   179,   163,   202,
      98,    91,    92,    93,    98,   202,    36,    36,   164,   208,
     210,   180,   158,    98,   162,    71,   174,   162,    71,   158,
     158,    84,   165,   165,   165,   165,   162,    84,   202,   205,
      84,   202,   205,   158,   158,   158,   158,   158,   158,   158,
     162,    98,   220,    26,    57,    26,   220,    26,   220,   165,
     165,    78,   161,   196,    26,    36,   158,   158,   159,   159,
     179,   159,   220,   220,   220,   126,   136,   158,   162,   158,
     162,    26,    36,   209,   220,    26,    26,    26,    26,    26,
      36,    26,    26,    26,    26,    26,    26,   163,   220,   220,
     220,    26,    26,   133,   134,    35,   163,   189,   163,   190,
     159,   159,   159,   159,    26,    26,    71,   104,   179,   162,
      84,   202,   205,   179,    26,    26,   158,   133,   134,   132,
      26,   220,    26,   220,    26,   220,    26,   179,   179,   179,
      26,    26,    26,    36,   138,    26,    69,    88,   220,   220,
     220,    22,    36,   159,   159,    26,   179,   162,   158,   220,
      26,    26,    26,    26,    26,   165,   220,   165,    26,   220,
     165,   220,   220,   220,    26,   220,   220,   220,    26,   163,
      98,   220,   162,   158,   162,   165,   165,    98,   178,   160,
     179,   206,   206,    36,    36,   158,    36,   220,   162,   162,
     136,   135,    98,    98,   180,   158,   160,   160,   160,   160,
     158,   158,   158,   158,   158,    26,   189,   190,   189,   158,
     163,   184,   158,   163,   185,   162,   162,   158,   125,   126,
     143,   143,    36,    36,    36,    36,   220,   202,   205,    84,
     202,   205,    26,   162,    26,   214,   216,   217,   220,   165,
     158,   158,   158,    26,   162,   162,   162,   163,   197,   197,
     197,   197,   158,   158,   162,   158,   158,   162,   163,   163,
     163,   163,   191,   191,   163,   185,    36,    36,    84,   205,
     162,    36,   162,    92,   162,   162,   162,   162,   165,    98,
     165,   165,    98,   162,   162,   162,   158,   162,   162,   162,
     131,    26,   163,    98,    26,    98,    98,    98,    26,   161,
     220,    70,    77,   179,   158,   162,   162,   163,   179,   163,
      26,   220,   220,   220,   220,    26,    36,    26,    36,    26,
      91,    26,   126,   127,   128,   182,    26,   125,   126,    26,
     162,   158,   162,   158,   162,   162,   190,   189,   220,   165,
      26,   220,   165,   162,   165,   173,   165,    26,    26,    26,
      98,   136,   220,   220,   163,   181,    36,    36,   141,   142,
     179,   179,   126,    36,   191,    26,   165,    98,    26,   220,
      98,    26,    98,    98,    26,    36,   179,   162,   131,     3,
       4,     9,    20,   160,   164,   199,    26,    26,   158,   162,
     179,   179,   158,   206,   160,   158,   158,   158,   158,   162,
     162,   162,   162,   162,   220,    98,   162,    98,   162,   162,
      98,   143,   126,   165,   187,   165,   165,   187,    91,   170,
     171,   172,   215,    98,   162,   162,   211,   162,   162,   162,
     121,   122,   162,   162,   162,   162,   158,   158,   162,   162,
     165,   211,    98,    26,    36,    26,   162,   162,   163,   163,
      26,    26,   220,    26,    26,    28,   179,   220,    26,    26,
      26,    26,   181,   187,   163,   162,    26,    26,    32,   162,
     162,   187,   187,   187,   220,   179,   190,   180,   162,   162,
     190,    26,   179,    26,    72,   180,    26,   181,    26,    26,
     160,   163,   162,   160,   162,   163,   162,   160,   160,   160,
     160,   133,   201,   202,    71,    70,   162,   158,   162,    36,
      92,   160,   160,   220,   108,   109,   110,   111,   192,    98,
     220,   126,   163,   193,   220,   220,   220,   220,   162,    26,
      26,    33,   179,    28,    71,   220,   220,   220,   193,   162,
     199,   162,   162,   137,   162,   162,   162,   162,   163,   186,
     185,   185,   162,    22,   158,   158,   162,   163,   194,   163,
     203,   203,   163,   204,   121,   122,   182,   183,   163,    26,
      26,   136,   137,   129,   130,   139,   140,   144,   145,   158,
     162,   158,   162,    36,   160,   160,   158,   162,   158,   162,
     158,   162,   158,   162,   158,   162,   158,   162,   158,   162,
     158,   162,   183,   182,   162,   220,   220,   137,   136,   137,
     137,   137,   137,   137,   137,   162,   162,    72,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,    36,   193,
     193
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 593 "bfin-parse.y"
    {
	  insn = (yyvsp[0].instr);
	  if (insn == (INSTR_T) 0)
	    return NO_INSN_GENERATED;
	  else if (insn == (INSTR_T) - 1)
	    return SEMANTIC_ERROR;
	  else
	    return INSN_GENERATED;
	}
    break;

  case 5:
#line 607 "bfin-parse.y"
    {
	  if (((yyvsp[-5].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-3].instr)) && is_group2 ((yyvsp[-1].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-5].instr), (yyvsp[-3].instr), (yyvsp[-1].instr));
	      else if (is_group2 ((yyvsp[-3].instr)) && is_group1 ((yyvsp[-1].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-5].instr), (yyvsp[-1].instr), (yyvsp[-3].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 2 and slot 3 must be 16-bit instrution group");
	    }
	  else if (((yyvsp[-3].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-5].instr)) && is_group2 ((yyvsp[-1].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-3].instr), (yyvsp[-5].instr), (yyvsp[-1].instr));
	      else if (is_group2 ((yyvsp[-5].instr)) && is_group1 ((yyvsp[-1].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-3].instr), (yyvsp[-1].instr), (yyvsp[-5].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 1 and slot 3 must be 16-bit instrution group");
	    }
	  else if (((yyvsp[-1].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-5].instr)) && is_group2 ((yyvsp[-3].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-1].instr), (yyvsp[-5].instr), (yyvsp[-3].instr));
	      else if (is_group2 ((yyvsp[-5].instr)) && is_group1 ((yyvsp[-3].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-1].instr), (yyvsp[-3].instr), (yyvsp[-5].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 1 and slot 2 must be 16-bit instrution group");
	    }
	  else
	    error ("\nIllegal Multi Issue Construct, at least any one of the slot must be DSP32 instruction group\n");
	}
    break;

  case 6:
#line 640 "bfin-parse.y"
    {
	  if (((yyvsp[-3].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-1].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-3].instr), (yyvsp[-1].instr), 0);
	      else if (is_group2 ((yyvsp[-1].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-3].instr), 0, (yyvsp[-1].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 2 must be the 16-bit instruction group");
	    }
	  else if (((yyvsp[-1].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-3].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-1].instr), (yyvsp[-3].instr), 0);
	      else if (is_group2 ((yyvsp[-3].instr)))
		(yyval.instr) = bfin_gen_multi_instr ((yyvsp[-1].instr), 0, (yyvsp[-3].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 1 must be the 16-bit instruction group");
	    }
	  else if (is_group1 ((yyvsp[-3].instr)) && is_group2 ((yyvsp[-1].instr)))
	      (yyval.instr) = bfin_gen_multi_instr (0, (yyvsp[-3].instr), (yyvsp[-1].instr));
	  else if (is_group2 ((yyvsp[-3].instr)) && is_group1 ((yyvsp[-1].instr)))
	    (yyval.instr) = bfin_gen_multi_instr (0, (yyvsp[-1].instr), (yyvsp[-3].instr));
	  else
	    return yyerror ("Wrong 16 bit instructions groups, slot 1 and slot 2 must be the 16-bit instruction group");
	}
    break;

  case 7:
#line 667 "bfin-parse.y"
    {
	(yyval.instr) = 0;
	yyerror ("");
	yyerrok;
	}
    break;

  case 8:
#line 678 "bfin-parse.y"
    {
	  (yyval.instr) = DSP32MAC (3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0);
	}
    break;

  case 9:
#line 682 "bfin-parse.y"
    {
	  int op0, op1;
	  int w0 = 0, w1 = 0;
	  int h00, h10, h01, h11;

	  if (check_macfunc_option (&(yyvsp[-1].macfunc), &(yyvsp[0].mod)) < 0)
	    return yyerror ("bad option");

	  if ((yyvsp[-1].macfunc).n == 0)
	    {
	      if ((yyvsp[0].mod).MM) 
		return yyerror ("(m) not allowed with a0 unit");
	      op1 = 3;
	      op0 = (yyvsp[-1].macfunc).op;
	      w1 = 0;
              w0 = (yyvsp[-1].macfunc).w;
	      h00 = IS_H ((yyvsp[-1].macfunc).s0);
              h10 = IS_H ((yyvsp[-1].macfunc).s1);
	      h01 = h11 = 0;
	    }
	  else
	    {
	      op1 = (yyvsp[-1].macfunc).op;
	      op0 = 3;
	      w1 = (yyvsp[-1].macfunc).w;
              w0 = 0;
	      h00 = h10 = 0;
	      h01 = IS_H ((yyvsp[-1].macfunc).s0);
              h11 = IS_H ((yyvsp[-1].macfunc).s1);
	    }
	  (yyval.instr) = DSP32MAC (op1, (yyvsp[0].mod).MM, (yyvsp[0].mod).mod, w1, (yyvsp[-1].macfunc).P, h01, h11, h00, h10,
			 &(yyvsp[-1].macfunc).dst, op0, &(yyvsp[-1].macfunc).s0, &(yyvsp[-1].macfunc).s1, w0);
	}
    break;

  case 10:
#line 720 "bfin-parse.y"
    {
	  Register *dst;

	  if (check_macfuncs (&(yyvsp[-4].macfunc), &(yyvsp[-3].mod), &(yyvsp[-1].macfunc), &(yyvsp[0].mod)) < 0) 
	    return -1;
	  notethat ("assign_macfunc (.), assign_macfunc (.)\n");

	  if ((yyvsp[-4].macfunc).w)
	    dst = &(yyvsp[-4].macfunc).dst;
	  else
	    dst = &(yyvsp[-1].macfunc).dst;

	  (yyval.instr) = DSP32MAC ((yyvsp[-4].macfunc).op, (yyvsp[-3].mod).MM, (yyvsp[0].mod).mod, (yyvsp[-4].macfunc).w, (yyvsp[-4].macfunc).P,
			 IS_H ((yyvsp[-4].macfunc).s0),  IS_H ((yyvsp[-4].macfunc).s1), IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1),
			 dst, (yyvsp[-1].macfunc).op, &(yyvsp[-4].macfunc).s0, &(yyvsp[-4].macfunc).s1, (yyvsp[-1].macfunc).w);
	}
    break;

  case 11:
#line 740 "bfin-parse.y"
    {
	  notethat ("dsp32alu: DISALGNEXCPT\n");
	  (yyval.instr) = DSP32ALU (18, 0, 0, 0, 0, 0, 0, 0, 3);
	}
    break;

  case 12:
#line 745 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && !IS_A1 ((yyvsp[-2].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = ( A0 += A1 )\n");
	      (yyval.instr) = DSP32ALU (11, 0, 0, &(yyvsp[-5].reg), 0, 0, 0, 0, 0);
	    }
	  else 
	    return yyerror ("Register mismatch");
	}
    break;

  case 13:
#line 755 "bfin-parse.y"
    {
	  if (!IS_A1 ((yyvsp[-2].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs_half = ( A0 += A1 )\n");
	      (yyval.instr) = DSP32ALU (11, IS_H ((yyvsp[-5].reg)), 0, &(yyvsp[-5].reg), 0, 0, 0, 0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 14:
#line 765 "bfin-parse.y"
    {
	  notethat ("dsp32alu: A_ZERO_DOT_H = dregs_hi\n");
	  (yyval.instr) = DSP32ALU (9, IS_H ((yyvsp[0].reg)), 0, 0, &(yyvsp[0].reg), 0, 0, 0, 0);
	}
    break;

  case 15:
#line 770 "bfin-parse.y"
    {
	  notethat ("dsp32alu: A_ZERO_DOT_H = dregs_hi\n");
	  (yyval.instr) = DSP32ALU (9, IS_H ((yyvsp[0].reg)), 0, 0, &(yyvsp[0].reg), 0, 0, 0, 2);
	}
    break;

  case 16:
#line 776 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-15].reg)) || !IS_DREG ((yyvsp[-13].reg)))
	    return yyerror ("Dregs expected");
	  else if (!valid_dreg_pair (&(yyvsp[-8].reg), (yyvsp[-6].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-4].reg), (yyvsp[-2].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: (dregs , dregs ) = BYTEOP16P (dregs_pair , dregs_pair ) (half)\n");
	      (yyval.instr) = DSP32ALU (21, 0, &(yyvsp[-15].reg), &(yyvsp[-13].reg), &(yyvsp[-8].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0, 0);
	    }
	}
    break;

  case 17:
#line 792 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-15].reg)) || !IS_DREG((yyvsp[-13].reg)))
	    return yyerror ("Dregs expected");
	  else if (!valid_dreg_pair (&(yyvsp[-8].reg), (yyvsp[-6].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-4].reg), (yyvsp[-2].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: (dregs , dregs ) = BYTEOP16M (dregs_pair , dregs_pair ) (aligndir)\n");
	      (yyval.instr) = DSP32ALU (21, 0, &(yyvsp[-15].reg), &(yyvsp[-13].reg), &(yyvsp[-8].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0, 1);
	    }
	}
    break;

  case 18:
#line 807 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-9].reg)) || !IS_DREG ((yyvsp[-7].reg)))
	    return yyerror ("Dregs expected");
	  else if (!valid_dreg_pair (&(yyvsp[-3].reg), (yyvsp[-1].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: (dregs , dregs ) = BYTEUNPACK dregs_pair (aligndir)\n");
	      (yyval.instr) = DSP32ALU (24, 0, &(yyvsp[-9].reg), &(yyvsp[-7].reg), &(yyvsp[-3].reg), 0, (yyvsp[0].r0).r0, 0, 1);
	    }
	}
    break;

  case 19:
#line 819 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-9].reg)) && IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: (dregs , dregs ) = SEARCH dregs (searchmod)\n");
	      (yyval.instr) = DSP32ALU (13, 0, &(yyvsp[-9].reg), &(yyvsp[-7].reg), &(yyvsp[-3].reg), 0, 0, 0, (yyvsp[-1].r0).r0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 20:
#line 830 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-10].reg)) && IS_DREG ((yyvsp[-4].reg)))
	    {
	      notethat ("dsp32alu: dregs = A1.l + A1.h, dregs = A0.l + A0.h  \n");
	      (yyval.instr) = DSP32ALU (12, 0, &(yyvsp[-10].reg), &(yyvsp[-4].reg), 0, 0, 0, 0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 21:
#line 842 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-11].reg)) && IS_DREG ((yyvsp[-5].reg)) && !REG_SAME ((yyvsp[-9].reg), (yyvsp[-7].reg))
	      && IS_A1 ((yyvsp[-3].reg)) && !IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = A1 + A0 , dregs = A1 - A0 (amod1)\n");
	      (yyval.instr) = DSP32ALU (17, 0, &(yyvsp[-11].reg), &(yyvsp[-5].reg), 0, 0, (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, 0);
	      
	    }
	  else if (IS_DREG ((yyvsp[-11].reg)) && IS_DREG ((yyvsp[-5].reg)) && !REG_SAME ((yyvsp[-9].reg), (yyvsp[-7].reg))
		   && !IS_A1 ((yyvsp[-3].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = A0 + A1 , dregs = A0 - A1 (amod1)\n");
	      (yyval.instr) = DSP32ALU (17, 0, &(yyvsp[-11].reg), &(yyvsp[-5].reg), 0, 0, (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 22:
#line 861 "bfin-parse.y"
    {
	  if ((yyvsp[-8].r0).r0 == (yyvsp[-2].r0).r0) 
	    return yyerror ("Operators must differ");

	  if (IS_DREG ((yyvsp[-11].reg)) && IS_DREG ((yyvsp[-9].reg)) && IS_DREG ((yyvsp[-7].reg))
	      && REG_SAME ((yyvsp[-9].reg), (yyvsp[-3].reg)) && REG_SAME ((yyvsp[-7].reg), (yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = dregs + dregs,"
		       "dregs = dregs - dregs (amod1)\n");
	      (yyval.instr) = DSP32ALU (4, 0, &(yyvsp[-11].reg), &(yyvsp[-5].reg), &(yyvsp[-9].reg), &(yyvsp[-7].reg), (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, 2);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 23:
#line 879 "bfin-parse.y"
    {
	  if (!REG_SAME ((yyvsp[-9].reg), (yyvsp[-3].reg)) || !REG_SAME ((yyvsp[-7].reg), (yyvsp[-1].reg)))
	    return yyerror ("Differing source registers");

	  if (!IS_DREG ((yyvsp[-11].reg)) || !IS_DREG ((yyvsp[-9].reg)) || !IS_DREG ((yyvsp[-7].reg)) || !IS_DREG ((yyvsp[-5].reg))) 
	    return yyerror ("Dregs expected");

	
	  if ((yyvsp[-8].r0).r0 == 1 && (yyvsp[-2].r0).r0 == 2)
	    {
	      notethat ("dsp32alu:  dregs = dregs .|. dregs , dregs = dregs .|. dregs (amod2)\n");
	      (yyval.instr) = DSP32ALU (1, 1, &(yyvsp[-11].reg), &(yyvsp[-5].reg), &(yyvsp[-9].reg), &(yyvsp[-7].reg), (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, (yyvsp[0].modcodes).r0);
	    }
	  else if ((yyvsp[-8].r0).r0 == 0 && (yyvsp[-2].r0).r0 == 3)
	    {
	      notethat ("dsp32alu:  dregs = dregs .|. dregs , dregs = dregs .|. dregs (amod2)\n");
	      (yyval.instr) = DSP32ALU (1, 0, &(yyvsp[-11].reg), &(yyvsp[-5].reg), &(yyvsp[-9].reg), &(yyvsp[-7].reg), (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, (yyvsp[0].modcodes).r0);
	    }
	  else
	    return yyerror ("Bar operand mismatch");
	}
    break;

  case 24:
#line 902 "bfin-parse.y"
    {
	  int op;

	  if (IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      if ((yyvsp[0].r0).r0)
		{
		  notethat ("dsp32alu: dregs = ABS dregs (v)\n");
		  op = 6;
		}
	      else
		{
		  /* Vector version of ABS.  */
		  notethat ("dsp32alu: dregs = ABS dregs\n");
		  op = 7;
		}
	      (yyval.instr) = DSP32ALU (op, 0, 0, &(yyvsp[-4].reg), &(yyvsp[-1].reg), 0, 0, 0, 2);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 25:
#line 924 "bfin-parse.y"
    {
	  notethat ("dsp32alu: Ax = ABS Ax\n");
	  (yyval.instr) = DSP32ALU (16, IS_A1 ((yyvsp[-2].reg)), 0, 0, 0, 0, 0, 0, IS_A1 ((yyvsp[0].reg)));
	}
    break;

  case 26:
#line 929 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu: A0.l = reg_half\n");
	      (yyval.instr) = DSP32ALU (9, IS_H ((yyvsp[0].reg)), 0, 0, &(yyvsp[0].reg), 0, 0, 0, 0);
	    }
	  else
	    return yyerror ("A0.l = Rx.l expected");
	}
    break;

  case 27:
#line 939 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu: A1.l = reg_half\n");
	      (yyval.instr) = DSP32ALU (9, IS_H ((yyvsp[0].reg)), 0, 0, &(yyvsp[0].reg), 0, 0, 0, 2);
	    }
	  else
	    return yyerror ("A1.l = Rx.l expected");
	}
    break;

  case 28:
#line 950 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs = ALIGN8 (dregs , dregs )\n");
	      (yyval.instr) = DSP32SHIFT (13, &(yyvsp[-7].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), (yyvsp[-5].r0).r0, 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 29:
#line 961 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-12].reg)))
	    return yyerror ("Dregs expected");
	  else if (!valid_dreg_pair (&(yyvsp[-8].reg), (yyvsp[-6].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-4].reg), (yyvsp[-2].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: dregs = BYTEOP1P (dregs_pair , dregs_pair ) (T)\n");
	      (yyval.instr) = DSP32ALU (20, 0, 0, &(yyvsp[-12].reg), &(yyvsp[-8].reg), &(yyvsp[-4].reg), (yyvsp[0].modcodes).s0, 0, (yyvsp[0].modcodes).r0);
	    }
	}
    break;

  case 30:
#line 975 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-11].reg)))
	    return yyerror ("Dregs expected");
	  else if (!valid_dreg_pair (&(yyvsp[-7].reg), (yyvsp[-5].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-3].reg), (yyvsp[-1].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: dregs = BYTEOP1P (dregs_pair , dregs_pair ) (T)\n");
	      (yyval.instr) = DSP32ALU (20, 0, 0, &(yyvsp[-11].reg), &(yyvsp[-7].reg), &(yyvsp[-3].reg), 0, 0, 0);
	    }
	}
    break;

  case 31:
#line 991 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-12].reg)))
	    return yyerror ("Dregs expected");
	  else if (!valid_dreg_pair (&(yyvsp[-8].reg), (yyvsp[-6].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-4].reg), (yyvsp[-2].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: dregs = BYTEOP2P (dregs_pair , dregs_pair ) (rnd_op)\n");
	      (yyval.instr) = DSP32ALU (22, (yyvsp[0].modcodes).r0, 0, &(yyvsp[-12].reg), &(yyvsp[-8].reg), &(yyvsp[-4].reg), (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, (yyvsp[0].modcodes).aop);
	    }
	}
    break;

  case 32:
#line 1007 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-12].reg)))
	    return yyerror ("Dregs expected");
	  else if (!valid_dreg_pair (&(yyvsp[-8].reg), (yyvsp[-6].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-4].reg), (yyvsp[-2].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: dregs = BYTEOP2P (dregs_pair , dregs_pair ) (rnd_op)\n");
	      (yyval.instr) = DSP32ALU (22, (yyvsp[0].modcodes).r0, 0, &(yyvsp[-12].reg), &(yyvsp[-8].reg), &(yyvsp[-4].reg), (yyvsp[0].modcodes).s0, 0, (yyvsp[0].modcodes).x0);
	    }
	}
    break;

  case 33:
#line 1023 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-12].reg)))
	    return yyerror ("Dregs expected");
	  else if (!valid_dreg_pair (&(yyvsp[-8].reg), (yyvsp[-6].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-4].reg), (yyvsp[-2].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: dregs = BYTEOP3P (dregs_pair , dregs_pair ) (b3_op)\n");
	      (yyval.instr) = DSP32ALU (23, (yyvsp[0].modcodes).x0, 0, &(yyvsp[-12].reg), &(yyvsp[-8].reg), &(yyvsp[-4].reg), (yyvsp[0].modcodes).s0, 0, 0);
	    }
	}
    break;

  case 34:
#line 1038 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = BYTEPACK (dregs , dregs )\n");
	      (yyval.instr) = DSP32ALU (24, 0, 0, &(yyvsp[-7].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg), 0, 0, 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 35:
#line 1050 "bfin-parse.y"
    {
	  if (IS_HCOMPL ((yyvsp[-16].reg), (yyvsp[-14].reg)) && IS_HCOMPL ((yyvsp[-10].reg), (yyvsp[-3].reg)) && IS_HCOMPL ((yyvsp[-7].reg), (yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu:	dregs_hi = dregs_lo ="
		       "SIGN (dregs_hi) * dregs_hi + "
		       "SIGN (dregs_lo) * dregs_lo \n");

		(yyval.instr) = DSP32ALU (12, 0, 0, &(yyvsp[-16].reg), &(yyvsp[-10].reg), &(yyvsp[-7].reg), 0, 0, 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 36:
#line 1063 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      if ((yyvsp[0].modcodes).aop == 0)
		{
	          /* No saturation flag specified, generate the 16 bit variant.  */
		  notethat ("COMP3op: dregs = dregs +- dregs\n");
		  (yyval.instr) = COMP3OP (&(yyvsp[-5].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg), (yyvsp[-2].r0).r0);
		}
	      else
		{
		 /* Saturation flag specified, generate the 32 bit variant.  */
                 notethat ("dsp32alu: dregs = dregs +- dregs (amod1)\n");
                 (yyval.instr) = DSP32ALU (4, 0, 0, &(yyvsp[-5].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg), (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, (yyvsp[-2].r0).r0);
		}
	    }
	  else
	    if (IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg)) && IS_PREG ((yyvsp[-1].reg)) && (yyvsp[-2].r0).r0 == 0)
	      {
		notethat ("COMP3op: pregs = pregs + pregs\n");
		(yyval.instr) = COMP3OP (&(yyvsp[-5].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg), 5);
	      }
	    else
	      return yyerror ("Dregs expected");
	}
    break;

  case 37:
#line 1089 "bfin-parse.y"
    {
	  int op;

	  if (IS_DREG ((yyvsp[-8].reg)) && IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)))
	    {
	      if ((yyvsp[0].r0).r0)
		op = 6;
	      else
		op = 7;

	      notethat ("dsp32alu: dregs = {MIN|MAX} (dregs, dregs)\n");
	      (yyval.instr) = DSP32ALU (op, 0, 0, &(yyvsp[-8].reg), &(yyvsp[-4].reg), &(yyvsp[-2].reg), 0, 0, (yyvsp[-6].r0).r0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 38:
#line 1107 "bfin-parse.y"
    {
	  notethat ("dsp32alu: Ax = - Ax\n");
	  (yyval.instr) = DSP32ALU (14, IS_A1 ((yyvsp[-2].reg)), 0, 0, 0, 0, 0, 0, IS_A1 ((yyvsp[0].reg)));
	}
    break;

  case 39:
#line 1112 "bfin-parse.y"
    {
	  notethat ("dsp32alu: dregs_lo = dregs_lo +- dregs_lo (amod1)\n");
	  (yyval.instr) = DSP32ALU (2 | (yyvsp[-2].r0).r0, IS_H ((yyvsp[-5].reg)), 0, &(yyvsp[-5].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg),
			 (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, HL2 ((yyvsp[-3].reg), (yyvsp[-1].reg)));
	}
    break;

  case 40:
#line 1118 "bfin-parse.y"
    {
	  if (EXPR_VALUE ((yyvsp[0].expr)) == 0 && !REG_SAME ((yyvsp[-2].reg), (yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: A1 = A0 = 0\n");
	      (yyval.instr) = DSP32ALU (8, 0, 0, 0, 0, 0, 0, 0, 2);
	    }
	  else
	    return yyerror ("Bad value, 0 expected");
	}
    break;

  case 41:
#line 1130 "bfin-parse.y"
    {
	  if (REG_SAME ((yyvsp[-4].reg), (yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: Ax = Ax (S)\n");
	      (yyval.instr) = DSP32ALU (8, 0, 0, 0, 0, 0, 1, 0, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Registers must be equal");
	}
    break;

  case 42:
#line 1141 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: dregs_half = dregs (RND)\n");
	      (yyval.instr) = DSP32ALU (12, IS_H ((yyvsp[-5].reg)), 0, &(yyvsp[-5].reg), &(yyvsp[-3].reg), 0, 0, 0, 3);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 43:
#line 1152 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: dregs_half = dregs (+-) dregs (RND12)\n");
	      (yyval.instr) = DSP32ALU (5, IS_H ((yyvsp[-7].reg)), 0, &(yyvsp[-7].reg), &(yyvsp[-5].reg), &(yyvsp[-3].reg), 0, 0, (yyvsp[-4].r0).r0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 44:
#line 1163 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: dregs_half = dregs -+ dregs (RND20)\n");
	      (yyval.instr) = DSP32ALU (5, IS_H ((yyvsp[-7].reg)), 0, &(yyvsp[-7].reg), &(yyvsp[-5].reg), &(yyvsp[-3].reg), 0, 1, (yyvsp[-4].r0).r0 | 2);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 45:
#line 1174 "bfin-parse.y"
    {
	  if (!REG_SAME ((yyvsp[-1].reg), (yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu: An = Am\n");
	      (yyval.instr) = DSP32ALU (8, 0, 0, 0, 0, 0, IS_A1 ((yyvsp[-1].reg)), 0, 3);
	    }
	  else
	    return yyerror ("Accu reg arguments must differ");
	}
    break;

  case 46:
#line 1185 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu: An = dregs\n");
	      (yyval.instr) = DSP32ALU (9, 0, 0, 0, &(yyvsp[0].reg), 0, 1, 0, IS_A1 ((yyvsp[-1].reg)) << 1);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 47:
#line 1196 "bfin-parse.y"
    {
	  if (!IS_H ((yyvsp[-1].reg)))
	    {
	      if ((yyvsp[-3].reg).regno == REG_A0x && IS_DREG ((yyvsp[-1].reg)))
		{
		  notethat ("dsp32alu: A0.x = dregs_lo\n");
		  (yyval.instr) = DSP32ALU (9, 0, 0, 0, &(yyvsp[-1].reg), 0, 0, 0, 1);
		}
	      else if ((yyvsp[-3].reg).regno == REG_A1x && IS_DREG ((yyvsp[-1].reg)))
		{
		  notethat ("dsp32alu: A1.x = dregs_lo\n");
		  (yyval.instr) = DSP32ALU (9, 0, 0, 0, &(yyvsp[-1].reg), 0, 0, 0, 3);
		}
	      else if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
		{
		  notethat ("ALU2op: dregs = dregs_lo\n");
		  (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[-1].reg), 10 | ((yyvsp[0].r0).r0 ? 0: 1));
		}
	      else
	        return yyerror ("Register mismatch");
	    }
	  else
	    return yyerror ("Low reg expected");
	}
    break;

  case 48:
#line 1222 "bfin-parse.y"
    {
	  notethat ("LDIMMhalf: pregs_half = imm16\n");

	  if (!IS_DREG ((yyvsp[-2].reg)) && !IS_PREG ((yyvsp[-2].reg)) && !IS_IREG ((yyvsp[-2].reg))
	      && !IS_MREG ((yyvsp[-2].reg)) && !IS_BREG ((yyvsp[-2].reg)) && !IS_LREG ((yyvsp[-2].reg)))
	    return yyerror ("Wrong register for load immediate");

	  if (!IS_IMM ((yyvsp[0].expr), 16) && !IS_UIMM ((yyvsp[0].expr), 16))
	    return yyerror ("Constant out of range");

	  (yyval.instr) = LDIMMHALF_R (&(yyvsp[-2].reg), IS_H ((yyvsp[-2].reg)), 0, 0, (yyvsp[0].expr));
	}
    break;

  case 49:
#line 1236 "bfin-parse.y"
    {
	  notethat ("dsp32alu: An = 0\n");

	  if (imm7 ((yyvsp[0].expr)) != 0)
	    return yyerror ("0 expected");

	  (yyval.instr) = DSP32ALU (8, 0, 0, 0, 0, 0, 0, 0, IS_A1 ((yyvsp[-1].reg)));
	}
    break;

  case 50:
#line 1246 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-3].reg)) && !IS_PREG ((yyvsp[-3].reg)) && !IS_IREG ((yyvsp[-3].reg))
	      && !IS_MREG ((yyvsp[-3].reg)) && !IS_BREG ((yyvsp[-3].reg)) && !IS_LREG ((yyvsp[-3].reg)))
	    return yyerror ("Wrong register for load immediate");

	  if ((yyvsp[0].r0).r0 == 0)
	    {
	      /* 7 bit immediate value if possible.
		 We will check for that constant value for efficiency
		 If it goes to reloc, it will be 16 bit.  */
	      if (IS_CONST ((yyvsp[-1].expr)) && IS_IMM ((yyvsp[-1].expr), 7) && IS_DREG ((yyvsp[-3].reg)))
		{
		  notethat ("COMPI2opD: dregs = imm7 (x) \n");
		  (yyval.instr) = COMPI2OPD (&(yyvsp[-3].reg), imm7 ((yyvsp[-1].expr)), 0);
		}
	      else if (IS_CONST ((yyvsp[-1].expr)) && IS_IMM ((yyvsp[-1].expr), 7) && IS_PREG ((yyvsp[-3].reg)))
		{
		  notethat ("COMPI2opP: pregs = imm7 (x)\n");
		  (yyval.instr) = COMPI2OPP (&(yyvsp[-3].reg), imm7 ((yyvsp[-1].expr)), 0);
		}
	      else
		{
		  if (IS_CONST ((yyvsp[-1].expr)) && !IS_IMM ((yyvsp[-1].expr), 16))
		    return yyerror ("Immediate value out of range");

		  notethat ("LDIMMhalf: regs = luimm16 (x)\n");
		  /* reg, H, S, Z.   */
		  (yyval.instr) = LDIMMHALF_R5 (&(yyvsp[-3].reg), 0, 1, 0, (yyvsp[-1].expr));
		} 
	    }
	  else
	    {
	      /* (z) There is no 7 bit zero extended instruction.
	      If the expr is a relocation, generate it.   */

	      if (IS_CONST ((yyvsp[-1].expr)) && !IS_UIMM ((yyvsp[-1].expr), 16))
		return yyerror ("Immediate value out of range");

	      notethat ("LDIMMhalf: regs = luimm16 (x)\n");
	      /* reg, H, S, Z.  */
	      (yyval.instr) = LDIMMHALF_R5 (&(yyvsp[-3].reg), 0, 0, 1, (yyvsp[-1].expr));
	    }
	}
    break;

  case 51:
#line 1291 "bfin-parse.y"
    {
	  if (IS_H ((yyvsp[-2].reg)))
	    return yyerror ("Low reg expected");

	  if (IS_DREG ((yyvsp[-2].reg)) && (yyvsp[0].reg).regno == REG_A0x)
	    {
	      notethat ("dsp32alu: dregs_lo = A0.x\n");
	      (yyval.instr) = DSP32ALU (10, 0, 0, &(yyvsp[-2].reg), 0, 0, 0, 0, 0);
	    }
	  else if (IS_DREG ((yyvsp[-2].reg)) && (yyvsp[0].reg).regno == REG_A1x)
	    {
	      notethat ("dsp32alu: dregs_lo = A1.x\n");
	      (yyval.instr) = DSP32ALU (10, 0, 0, &(yyvsp[-2].reg), 0, 0, 0, 0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 52:
#line 1310 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = dregs .|. dregs (amod0)\n");
	      (yyval.instr) = DSP32ALU (0, 0, 0, &(yyvsp[-5].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg), (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, (yyvsp[-2].r0).r0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 53:
#line 1321 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ALU2op: dregs = dregs_byte\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[-1].reg), 12 | ((yyvsp[0].r0).r0 ? 0: 1));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 54:
#line 1332 "bfin-parse.y"
    {
	  if (REG_SAME ((yyvsp[-6].reg), (yyvsp[-4].reg)) && REG_SAME ((yyvsp[-2].reg), (yyvsp[0].reg)) && !REG_SAME ((yyvsp[-6].reg), (yyvsp[-2].reg)))
	    {
	      notethat ("dsp32alu: A1 = ABS A1 , A0 = ABS A0\n");
	      (yyval.instr) = DSP32ALU (16, 0, 0, 0, 0, 0, 0, 0, 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 55:
#line 1343 "bfin-parse.y"
    {
	  if (REG_SAME ((yyvsp[-6].reg), (yyvsp[-4].reg)) && REG_SAME ((yyvsp[-2].reg), (yyvsp[0].reg)) && !REG_SAME ((yyvsp[-6].reg), (yyvsp[-2].reg)))
	    {
	      notethat ("dsp32alu: A1 = - A1 , A0 = - A0\n");
	      (yyval.instr) = DSP32ALU (14, 0, 0, 0, 0, 0, 0, 0, 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 56:
#line 1354 "bfin-parse.y"
    {
	  if (!IS_A1 ((yyvsp[-2].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: A0 -= A1\n");
	      (yyval.instr) = DSP32ALU (11, 0, 0, 0, 0, 0, (yyvsp[0].r0).r0, 0, 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 57:
#line 1365 "bfin-parse.y"
    {
	  if (IS_IREG ((yyvsp[-2].reg)) && EXPR_VALUE ((yyvsp[0].expr)) == 4)
	    {
	      notethat ("dagMODik: iregs -= 4\n");
	      (yyval.instr) = DAGMODIK (&(yyvsp[-2].reg), 3);
	    }
	  else if (IS_IREG ((yyvsp[-2].reg)) && EXPR_VALUE ((yyvsp[0].expr)) == 2)
	    {
	      notethat ("dagMODik: iregs -= 2\n");
	      (yyval.instr) = DAGMODIK (&(yyvsp[-2].reg), 1);
	    }
	  else
	    return yyerror ("Register or value mismatch");
	}
    break;

  case 58:
#line 1381 "bfin-parse.y"
    {
	  if (IS_IREG ((yyvsp[-5].reg)) && IS_MREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dagMODim: iregs += mregs (opt_brev)\n");
	      /* i, m, op, br.  */
	      (yyval.instr) = DAGMODIM (&(yyvsp[-5].reg), &(yyvsp[-3].reg), 0, 1);
	    }
	  else if (IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg)))
	    {
	      notethat ("PTR2op: pregs += pregs (BREV )\n");
	      (yyval.instr) = PTR2OP (&(yyvsp[-5].reg), &(yyvsp[-3].reg), 5);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 59:
#line 1398 "bfin-parse.y"
    {
	  if (IS_IREG ((yyvsp[-2].reg)) && IS_MREG ((yyvsp[0].reg)))
	    {
	      notethat ("dagMODim: iregs -= mregs\n");
	      (yyval.instr) = DAGMODIM (&(yyvsp[-2].reg), &(yyvsp[0].reg), 1, 0);
	    }
	  else if (IS_PREG ((yyvsp[-2].reg)) && IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("PTR2op: pregs -= pregs\n");
	      (yyval.instr) = PTR2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 60:
#line 1414 "bfin-parse.y"
    {
	  if (!IS_A1 ((yyvsp[-3].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: A0 += A1 (W32)\n");
	      (yyval.instr) = DSP32ALU (11, 0, 0, 0, 0, 0, (yyvsp[0].r0).r0, 0, 2);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 61:
#line 1425 "bfin-parse.y"
    {
	  if (IS_IREG ((yyvsp[-2].reg)) && IS_MREG ((yyvsp[0].reg)))
	    {
	      notethat ("dagMODim: iregs += mregs\n");
	      (yyval.instr) = DAGMODIM (&(yyvsp[-2].reg), &(yyvsp[0].reg), 0, 0);
	    }
	  else
	    return yyerror ("iregs += mregs expected");
	}
    break;

  case 62:
#line 1436 "bfin-parse.y"
    {
	  if (IS_IREG ((yyvsp[-2].reg)))
	    {
	      if (EXPR_VALUE ((yyvsp[0].expr)) == 4)
		{
		  notethat ("dagMODik: iregs += 4\n");
		  (yyval.instr) = DAGMODIK (&(yyvsp[-2].reg), 2);
		}
	      else if (EXPR_VALUE ((yyvsp[0].expr)) == 2)
		{
		  notethat ("dagMODik: iregs += 2\n");
		  (yyval.instr) = DAGMODIK (&(yyvsp[-2].reg), 0);
		}
	      else
		return yyerror ("iregs += [ 2 | 4 ");
	    }
	  else if (IS_PREG ((yyvsp[-2].reg)) && IS_IMM ((yyvsp[0].expr), 7))
	    {
	      notethat ("COMPI2opP: pregs += imm7\n");
	      (yyval.instr) = COMPI2OPP (&(yyvsp[-2].reg), imm7 ((yyvsp[0].expr)), 1);
	    }
	  else if (IS_DREG ((yyvsp[-2].reg)) && IS_IMM ((yyvsp[0].expr), 7))
	    {
	      notethat ("COMPI2opD: dregs += imm7\n");
	      (yyval.instr) = COMPI2OPD (&(yyvsp[-2].reg), imm7 ((yyvsp[0].expr)), 1);
	    }
	  else if ((IS_DREG ((yyvsp[-2].reg)) || IS_PREG ((yyvsp[-2].reg))) && IS_CONST ((yyvsp[0].expr)))
	    return yyerror ("Immediate value out of range");
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 63:
#line 1469 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs *= dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 64:
#line 1480 "bfin-parse.y"
    {
	  if (!valid_dreg_pair (&(yyvsp[-8].reg), (yyvsp[-6].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-4].reg), (yyvsp[-2].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: SAA (dregs_pair , dregs_pair ) (aligndir)\n");
	      (yyval.instr) = DSP32ALU (18, 0, 0, 0, &(yyvsp[-8].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0, 0);
	    }
	}
    break;

  case 65:
#line 1493 "bfin-parse.y"
    {
	  if (REG_SAME ((yyvsp[-10].reg), (yyvsp[-9].reg)) && REG_SAME ((yyvsp[-4].reg), (yyvsp[-3].reg)) && !REG_SAME ((yyvsp[-10].reg), (yyvsp[-4].reg)))
	    {
	      notethat ("dsp32alu: A1 = A1 (S) , A0 = A0 (S)\n");
	      (yyval.instr) = DSP32ALU (8, 0, 0, 0, 0, 0, 1, 0, 2);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 66:
#line 1504 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-8].reg)) && IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg))
	      && REG_SAME ((yyvsp[-8].reg), (yyvsp[-5].reg)))
	    {
	      if (EXPR_VALUE ((yyvsp[0].expr)) == 1)
		{
		  notethat ("ALU2op: dregs = (dregs + dregs) << 1\n");
		  (yyval.instr) = ALU2OP (&(yyvsp[-8].reg), &(yyvsp[-3].reg), 4);
		}
	      else if (EXPR_VALUE ((yyvsp[0].expr)) == 2)
		{
		  notethat ("ALU2op: dregs = (dregs + dregs) << 2\n");
		  (yyval.instr) = ALU2OP (&(yyvsp[-8].reg), &(yyvsp[-3].reg), 5);
		}
	      else
		return yyerror ("Bad shift value");
	    }
	  else if (IS_PREG ((yyvsp[-8].reg)) && IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg))
		   && REG_SAME ((yyvsp[-8].reg), (yyvsp[-5].reg)))
	    {
	      if (EXPR_VALUE ((yyvsp[0].expr)) == 1)
		{
		  notethat ("PTR2op: pregs = (pregs + pregs) << 1\n");
		  (yyval.instr) = PTR2OP (&(yyvsp[-8].reg), &(yyvsp[-3].reg), 6);
		}
	      else if (EXPR_VALUE ((yyvsp[0].expr)) == 2)
		{
		  notethat ("PTR2op: pregs = (pregs + pregs) << 2\n");
		  (yyval.instr) = PTR2OP (&(yyvsp[-8].reg), &(yyvsp[-3].reg), 7);
		}
	      else
		return yyerror ("Bad shift value");
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 67:
#line 1543 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("COMP3op: dregs = dregs | dregs\n");
	      (yyval.instr) = COMP3OP (&(yyvsp[-4].reg), &(yyvsp[-2].reg), &(yyvsp[0].reg), 3);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 68:
#line 1553 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("COMP3op: dregs = dregs ^ dregs\n");
	      (yyval.instr) = COMP3OP (&(yyvsp[-4].reg), &(yyvsp[-2].reg), &(yyvsp[0].reg), 4);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 69:
#line 1563 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-8].reg)) && IS_PREG ((yyvsp[-6].reg)) && IS_PREG ((yyvsp[-3].reg)))
	    {
	      if (EXPR_VALUE ((yyvsp[-1].expr)) == 1)
		{
		  notethat ("COMP3op: pregs = pregs + (pregs << 1)\n");
		  (yyval.instr) = COMP3OP (&(yyvsp[-8].reg), &(yyvsp[-6].reg), &(yyvsp[-3].reg), 6);
		}
	      else if (EXPR_VALUE ((yyvsp[-1].expr)) == 2)
		{
		  notethat ("COMP3op: pregs = pregs + (pregs << 2)\n");
		  (yyval.instr) = COMP3OP (&(yyvsp[-8].reg), &(yyvsp[-6].reg), &(yyvsp[-3].reg), 7);
		}
	      else
		  return yyerror ("Bad shift value");
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 70:
#line 1583 "bfin-parse.y"
    {
	  if ((yyvsp[-2].reg).regno == REG_A0 && (yyvsp[0].reg).regno == REG_A1)
	    {
	      notethat ("CCflag: CC = A0 == A1\n");
	      (yyval.instr) = CCFLAG (0, 0, 5, 0, 0);
	    }
	  else
	    return yyerror ("AREGs are in bad order or same");
	}
    break;

  case 71:
#line 1593 "bfin-parse.y"
    {
	  if ((yyvsp[-2].reg).regno == REG_A0 && (yyvsp[0].reg).regno == REG_A1)
	    {
	      notethat ("CCflag: CC = A0 < A1\n");
	      (yyval.instr) = CCFLAG (0, 0, 6, 0, 0);
	    }
	  else
	    return yyerror ("AREGs are in bad order or same");
	}
    break;

  case 72:
#line 1603 "bfin-parse.y"
    {
	  if (REG_CLASS((yyvsp[-3].reg)) == REG_CLASS((yyvsp[-1].reg)))
	    {
	      notethat ("CCflag: CC = dpregs < dpregs\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), (yyvsp[-1].reg).regno & CODE_MASK, (yyvsp[0].r0).r0, 0, IS_PREG ((yyvsp[-3].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Compare only of same register class");
	}
    break;

  case 73:
#line 1613 "bfin-parse.y"
    {
	  if (((yyvsp[0].r0).r0 == 1 && IS_IMM ((yyvsp[-1].expr), 3))
	      || ((yyvsp[0].r0).r0 == 3 && IS_UIMM ((yyvsp[-1].expr), 3)))
	    {
	      notethat ("CCflag: CC = dpregs < (u)imm3\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), imm3 ((yyvsp[-1].expr)), (yyvsp[0].r0).r0, 1, IS_PREG ((yyvsp[-3].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Bad constant value");
	}
    break;

  case 74:
#line 1624 "bfin-parse.y"
    {
	  if (REG_CLASS((yyvsp[-2].reg)) == REG_CLASS((yyvsp[0].reg)))
	    {
	      notethat ("CCflag: CC = dpregs == dpregs\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-2].reg), (yyvsp[0].reg).regno & CODE_MASK, 0, 0, IS_PREG ((yyvsp[-2].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Compare only of same register class");
	}
    break;

  case 75:
#line 1634 "bfin-parse.y"
    {
	  if (IS_IMM ((yyvsp[0].expr), 3))
	    {
	      notethat ("CCflag: CC = dpregs == imm3\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-2].reg), imm3 ((yyvsp[0].expr)), 0, 1, IS_PREG ((yyvsp[-2].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Bad constant range");
	}
    break;

  case 76:
#line 1644 "bfin-parse.y"
    {
	  if ((yyvsp[-2].reg).regno == REG_A0 && (yyvsp[0].reg).regno == REG_A1)
	    {
	      notethat ("CCflag: CC = A0 <= A1\n");
	      (yyval.instr) = CCFLAG (0, 0, 7, 0, 0);
	    }
	  else
	    return yyerror ("AREGs are in bad order or same");
	}
    break;

  case 77:
#line 1654 "bfin-parse.y"
    {
	  if (REG_CLASS((yyvsp[-3].reg)) == REG_CLASS((yyvsp[-1].reg)))
	    {
	      notethat ("CCflag: CC = pregs <= pregs (..)\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), (yyvsp[-1].reg).regno & CODE_MASK,
			   1 + (yyvsp[0].r0).r0, 0, IS_PREG ((yyvsp[-3].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Compare only of same register class");
	}
    break;

  case 78:
#line 1665 "bfin-parse.y"
    {
	  if (((yyvsp[0].r0).r0 == 1 && IS_IMM ((yyvsp[-1].expr), 3))
	      || ((yyvsp[0].r0).r0 == 3 && IS_UIMM ((yyvsp[-1].expr), 3)))
	    {
	      if (IS_DREG ((yyvsp[-3].reg)))
		{
		  notethat ("CCflag: CC = dregs <= (u)imm3\n");
		  /*    x       y     opc     I     G   */
		  (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), imm3 ((yyvsp[-1].expr)), 1 + (yyvsp[0].r0).r0, 1, 0);
		}
	      else if (IS_PREG ((yyvsp[-3].reg)))
		{
		  notethat ("CCflag: CC = pregs <= (u)imm3\n");
		  /*    x       y     opc     I     G   */
		  (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), imm3 ((yyvsp[-1].expr)), 1 + (yyvsp[0].r0).r0, 1, 1);
		}
	      else
		return yyerror ("Dreg or Preg expected");
	    }
	  else
	    return yyerror ("Bad constant value");
	}
    break;

  case 79:
#line 1689 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("COMP3op: dregs = dregs & dregs\n");
	      (yyval.instr) = COMP3OP (&(yyvsp[-4].reg), &(yyvsp[-2].reg), &(yyvsp[0].reg), 2);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 80:
#line 1700 "bfin-parse.y"
    {
	  notethat ("CC2stat operation\n");
	  (yyval.instr) = bfin_gen_cc2stat ((yyvsp[0].modcodes).r0, (yyvsp[0].modcodes).x0, (yyvsp[0].modcodes).s0);
	}
    break;

  case 81:
#line 1706 "bfin-parse.y"
    {
	  if (IS_ALLREG ((yyvsp[-2].reg)) && IS_ALLREG ((yyvsp[0].reg)))
	    {
	      notethat ("REGMV: allregs = allregs\n");
	      (yyval.instr) = bfin_gen_regmv (&(yyvsp[0].reg), &(yyvsp[-2].reg));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 82:
#line 1717 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("CC2dreg: CC = dregs\n");
	      (yyval.instr) = bfin_gen_cc2dreg (1, &(yyvsp[0].reg));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 83:
#line 1728 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)))
	    {
	      notethat ("CC2dreg: dregs = CC\n");
	      (yyval.instr) = bfin_gen_cc2dreg (0, &(yyvsp[-2].reg));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 84:
#line 1739 "bfin-parse.y"
    {
	  notethat ("CC2dreg: CC =! CC\n");
	  (yyval.instr) = bfin_gen_cc2dreg (3, 0);
	}
    break;

  case 85:
#line 1747 "bfin-parse.y"
    {
	  notethat ("dsp32mult: dregs_half = multiply_halfregs (opt_mode)\n");

	  if (!IS_H ((yyvsp[-3].reg)) && (yyvsp[0].mod).MM)
	    return yyerror ("(M) not allowed with MAC0");

	  if ((yyvsp[0].mod).mod != 0 && (yyvsp[0].mod).mod != M_FU && (yyvsp[0].mod).mod != M_IS
	      && (yyvsp[0].mod).mod != M_IU && (yyvsp[0].mod).mod != M_T && (yyvsp[0].mod).mod != M_TFU
	      && (yyvsp[0].mod).mod != M_S2RND && (yyvsp[0].mod).mod != M_ISS2 && (yyvsp[0].mod).mod != M_IH)
	    return yyerror ("bad option.");

	  if (IS_H ((yyvsp[-3].reg)))
	    {
	      (yyval.instr) = DSP32MULT (0, (yyvsp[0].mod).MM, (yyvsp[0].mod).mod, 1, 0,
			      IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1), 0, 0,
			      &(yyvsp[-3].reg), 0, &(yyvsp[-1].macfunc).s0, &(yyvsp[-1].macfunc).s1, 0);
	    }
	  else
	    {
	      (yyval.instr) = DSP32MULT (0, 0, (yyvsp[0].mod).mod, 0, 0,
			      0, 0, IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1), 
			      &(yyvsp[-3].reg), 0, &(yyvsp[-1].macfunc).s0, &(yyvsp[-1].macfunc).s1, 1);
	    }
	}
    break;

  case 86:
#line 1773 "bfin-parse.y"
    {
	  /* Odd registers can use (M).  */
	  if (!IS_DREG ((yyvsp[-3].reg)))
	    return yyerror ("Dreg expected");

	  if (IS_EVEN ((yyvsp[-3].reg)) && (yyvsp[0].mod).MM)
	    return yyerror ("(M) not allowed with MAC0");

	  if ((yyvsp[0].mod).mod != 0 && (yyvsp[0].mod).mod != M_FU && (yyvsp[0].mod).mod != M_IS
	      && (yyvsp[0].mod).mod != M_S2RND && (yyvsp[0].mod).mod != M_ISS2)
	    return yyerror ("bad option");

	  if (!IS_EVEN ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32mult: dregs = multiply_halfregs (opt_mode)\n");

	      (yyval.instr) = DSP32MULT (0, (yyvsp[0].mod).MM, (yyvsp[0].mod).mod, 1, 1,
			      IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1), 0, 0,
			      &(yyvsp[-3].reg), 0, &(yyvsp[-1].macfunc).s0, &(yyvsp[-1].macfunc).s1, 0);
	    }
	  else
	    {
	      notethat ("dsp32mult: dregs = multiply_halfregs opt_mode\n");
	      (yyval.instr) = DSP32MULT (0, 0, (yyvsp[0].mod).mod, 0, 1,
			      0, 0, IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1), 
			      &(yyvsp[-3].reg),  0, &(yyvsp[-1].macfunc).s0, &(yyvsp[-1].macfunc).s1, 1);
	    }
	}
    break;

  case 87:
#line 1804 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-8].reg)) || !IS_DREG ((yyvsp[-3].reg))) 
	    return yyerror ("Dregs expected");

	  if (!IS_HCOMPL((yyvsp[-8].reg), (yyvsp[-3].reg)))
	    return yyerror ("Dest registers mismatch");

	  if (check_multiply_halfregs (&(yyvsp[-6].macfunc), &(yyvsp[-1].macfunc)) < 0)
	    return -1;

	  if ((!IS_H ((yyvsp[-8].reg)) && (yyvsp[-5].mod).MM)
	      || (!IS_H ((yyvsp[-3].reg)) && (yyvsp[0].mod).MM))
	    return yyerror ("(M) not allowed with MAC0");

	  notethat ("dsp32mult: dregs_hi = multiply_halfregs mxd_mod, "
		    "dregs_lo = multiply_halfregs opt_mode\n");

	  if (IS_H ((yyvsp[-8].reg)))
	    (yyval.instr) = DSP32MULT (0, (yyvsp[-5].mod).MM, (yyvsp[0].mod).mod, 1, 0,
			    IS_H ((yyvsp[-6].macfunc).s0), IS_H ((yyvsp[-6].macfunc).s1), IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1),
			    &(yyvsp[-8].reg), 0, &(yyvsp[-6].macfunc).s0, &(yyvsp[-6].macfunc).s1, 1);
	  else
	    (yyval.instr) = DSP32MULT (0, (yyvsp[0].mod).MM, (yyvsp[0].mod).mod, 1, 0,
			    IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1), IS_H ((yyvsp[-6].macfunc).s0), IS_H ((yyvsp[-6].macfunc).s1),
			    &(yyvsp[-8].reg), 0, &(yyvsp[-6].macfunc).s0, &(yyvsp[-6].macfunc).s1, 1);
	}
    break;

  case 88:
#line 1832 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-8].reg)) || !IS_DREG ((yyvsp[-3].reg))) 
	    return yyerror ("Dregs expected");

	  if ((IS_EVEN ((yyvsp[-8].reg)) && (yyvsp[-3].reg).regno - (yyvsp[-8].reg).regno != 1)
	      || (IS_EVEN ((yyvsp[-3].reg)) && (yyvsp[-8].reg).regno - (yyvsp[-3].reg).regno != 1))
	    return yyerror ("Dest registers mismatch");

	  if (check_multiply_halfregs (&(yyvsp[-6].macfunc), &(yyvsp[-1].macfunc)) < 0)
	    return -1;

	  if ((IS_EVEN ((yyvsp[-8].reg)) && (yyvsp[-5].mod).MM)
	      || (IS_EVEN ((yyvsp[-3].reg)) && (yyvsp[0].mod).MM))
	    return yyerror ("(M) not allowed with MAC0");

	  notethat ("dsp32mult: dregs = multiply_halfregs mxd_mod, "
		   "dregs = multiply_halfregs opt_mode\n");

	  if (IS_EVEN ((yyvsp[-8].reg)))
	    (yyval.instr) = DSP32MULT (0, (yyvsp[0].mod).MM, (yyvsp[0].mod).mod, 1, 1,
			    IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1), IS_H ((yyvsp[-6].macfunc).s0), IS_H ((yyvsp[-6].macfunc).s1),
			    &(yyvsp[-8].reg), 0, &(yyvsp[-6].macfunc).s0, &(yyvsp[-6].macfunc).s1, 1);
	  else
	    (yyval.instr) = DSP32MULT (0, (yyvsp[-5].mod).MM, (yyvsp[0].mod).mod, 1, 1,
			    IS_H ((yyvsp[-6].macfunc).s0), IS_H ((yyvsp[-6].macfunc).s1), IS_H ((yyvsp[-1].macfunc).s0), IS_H ((yyvsp[-1].macfunc).s1),
			    &(yyvsp[-8].reg), 0, &(yyvsp[-6].macfunc).s0, &(yyvsp[-6].macfunc).s1, 1);
	}
    break;

  case 89:
#line 1863 "bfin-parse.y"
    {
	  if (!REG_SAME ((yyvsp[-4].reg), (yyvsp[-2].reg)))
	    return yyerror ("Aregs must be same");

	  if (IS_DREG ((yyvsp[0].reg)) && !IS_H ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: A0 = ASHIFT A0 BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (3, 0, &(yyvsp[0].reg), 0, 0, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 90:
#line 1877 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-1].reg)) && !IS_H ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs_half = ASHIFT dregs_half BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (0, &(yyvsp[-6].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0, HL2 ((yyvsp[-6].reg), (yyvsp[-3].reg)));
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 91:
#line 1888 "bfin-parse.y"
    {
	  if (!REG_SAME ((yyvsp[-3].reg), (yyvsp[-2].reg)))
	    return yyerror ("Aregs must be same");

	  if (IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("dsp32shiftimm: A0 = A0 << uimm5\n");
	      (yyval.instr) = DSP32SHIFTIMM (3, 0, imm5 ((yyvsp[0].expr)), 0, 0, IS_A1 ((yyvsp[-3].reg)));
	    }
	  else
	    return yyerror ("Bad shift value");
	}
    break;

  case 92:
#line 1902 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      if ((yyvsp[0].modcodes).r0)
		{
		  /*  Vector?  */
		  notethat ("dsp32shiftimm: dregs = dregs << expr (V, .)\n");
		  (yyval.instr) = DSP32SHIFTIMM (1, &(yyvsp[-5].reg), imm4 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0 ? 1 : 2, 0);
		}
	      else
		{
		  notethat ("dsp32shiftimm: dregs =  dregs << uimm5 (.)\n");
		  (yyval.instr) = DSP32SHIFTIMM (2, &(yyvsp[-5].reg), imm6 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0 ? 1 : 2, 0);
		}
	    }
	  else if ((yyvsp[0].modcodes).s0 == 0 && IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg)))
	    {
	      if (EXPR_VALUE ((yyvsp[-1].expr)) == 2)
		{
		  notethat ("PTR2op: pregs = pregs << 2\n");
		  (yyval.instr) = PTR2OP (&(yyvsp[-5].reg), &(yyvsp[-3].reg), 1);
		}
	      else if (EXPR_VALUE ((yyvsp[-1].expr)) == 1)
		{
		  notethat ("COMP3op: pregs = pregs << 1\n");
		  (yyval.instr) = COMP3OP (&(yyvsp[-5].reg), &(yyvsp[-3].reg), &(yyvsp[-3].reg), 5);
		}
	      else
		return yyerror ("Bad shift value");
	    }
	  else
	    return yyerror ("Bad shift value or register");
	}
    break;

  case 93:
#line 1936 "bfin-parse.y"
    {
	  if (IS_UIMM ((yyvsp[0].expr), 4))
	    {
	      notethat ("dsp32shiftimm: dregs_half = dregs_half << uimm4\n");
	      (yyval.instr) = DSP32SHIFTIMM (0x0, &(yyvsp[-4].reg), imm5 ((yyvsp[0].expr)), &(yyvsp[-2].reg), 2, HL2 ((yyvsp[-4].reg), (yyvsp[-2].reg)));
	    }
	  else
	    return yyerror ("Bad shift value");
	}
    break;

  case 94:
#line 1946 "bfin-parse.y"
    {
	  if (IS_UIMM ((yyvsp[-1].expr), 4))
	    {
	      notethat ("dsp32shiftimm: dregs_half = dregs_half << uimm4\n");
	      (yyval.instr) = DSP32SHIFTIMM (0x0, &(yyvsp[-5].reg), imm5 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0, HL2 ((yyvsp[-5].reg), (yyvsp[-3].reg)));
	    }
	  else
	    return yyerror ("Bad shift value");
	}
    break;

  case 95:
#line 1956 "bfin-parse.y"
    {
	  int op;

	  if (IS_DREG ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)) && !IS_H ((yyvsp[-1].reg)))
	    {
	      if ((yyvsp[0].modcodes).r0)
		{
		  op = 1;
		  notethat ("dsp32shift: dregs = ASHIFT dregs BY "
			   "dregs_lo (V, .)\n");
		}
	      else
		{
		  
		  op = 2;
		  notethat ("dsp32shift: dregs = ASHIFT dregs BY dregs_lo (.)\n");
		}
	      (yyval.instr) = DSP32SHIFT (op, &(yyvsp[-6].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0, 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 96:
#line 1981 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[-8].reg)) && IS_DREG_L ((yyvsp[-4].reg)) && IS_DREG_L ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = EXPADJ (dregs , dregs_lo )\n");
	      (yyval.instr) = DSP32SHIFT (7, &(yyvsp[-8].reg), &(yyvsp[-2].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0);
	    }
	  else
	    return yyerror ("Bad shift value or register");
	}
    break;

  case 97:
#line 1993 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[-7].reg)) && IS_DREG_L ((yyvsp[-3].reg)) && IS_DREG_L ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = EXPADJ (dregs_lo, dregs_lo)\n");
	      (yyval.instr) = DSP32SHIFT (7, &(yyvsp[-7].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), 2, 0);
	    }
	  else if (IS_DREG_L ((yyvsp[-7].reg)) && IS_DREG_H ((yyvsp[-3].reg)) && IS_DREG_L ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = EXPADJ (dregs_hi, dregs_lo)\n");
	      (yyval.instr) = DSP32SHIFT (7, &(yyvsp[-7].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), 3, 0);
	    }
	  else
	    return yyerror ("Bad shift value or register");
	}
    break;

  case 98:
#line 2011 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs = DEPOSIT (dregs , dregs )\n");
	      (yyval.instr) = DSP32SHIFT (10, &(yyvsp[-7].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), 2, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 99:
#line 2022 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-10].reg)) && IS_DREG ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-4].reg)))
	    {
	      notethat ("dsp32shift: dregs = DEPOSIT (dregs , dregs ) (X)\n");
	      (yyval.instr) = DSP32SHIFT (10, &(yyvsp[-10].reg), &(yyvsp[-4].reg), &(yyvsp[-6].reg), 3, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 100:
#line 2033 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-8].reg)) && IS_DREG ((yyvsp[-4].reg)) && IS_DREG_L ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: dregs = EXTRACT (dregs, dregs_lo ) (.)\n");
	      (yyval.instr) = DSP32SHIFT (10, &(yyvsp[-8].reg), &(yyvsp[-2].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 101:
#line 2044 "bfin-parse.y"
    {
	  if (!REG_SAME ((yyvsp[-3].reg), (yyvsp[-2].reg)))
	    return yyerror ("Aregs must be same");

	  if (IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("dsp32shiftimm: Ax = Ax >>> uimm5\n");
	      (yyval.instr) = DSP32SHIFTIMM (3, 0, -imm6 ((yyvsp[0].expr)), 0, 0, IS_A1 ((yyvsp[-3].reg)));
	    }
	  else
	    return yyerror ("Shift value range error");
	}
    break;

  case 102:
#line 2057 "bfin-parse.y"
    {
	  if (REG_SAME ((yyvsp[-4].reg), (yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: Ax = LSHIFT Ax BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (3, 0, &(yyvsp[0].reg), 0, 1, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 103:
#line 2068 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = LSHIFT dregs_hi BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (0, &(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-2].reg), 2, HL2 ((yyvsp[-5].reg), (yyvsp[-2].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 104:
#line 2079 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG_L ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs = LSHIFT dregs BY dregs_lo (V )\n");
	      (yyval.instr) = DSP32SHIFT ((yyvsp[0].r0).r0 ? 1: 2, &(yyvsp[-6].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), 2, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 105:
#line 2090 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs = SHIFT dregs BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (2, &(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-2].reg), 2, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 106:
#line 2101 "bfin-parse.y"
    {
	  if (REG_SAME ((yyvsp[-3].reg), (yyvsp[-2].reg)) && IS_IMM ((yyvsp[0].expr), 6) >= 0)
	    {
	      notethat ("dsp32shiftimm: Ax = Ax >> imm6\n");
	      (yyval.instr) = DSP32SHIFTIMM (3, 0, -imm6 ((yyvsp[0].expr)), 0, 1, IS_A1 ((yyvsp[-3].reg)));
	    }
	  else
	    return yyerror ("Accu register expected");
	}
    break;

  case 107:
#line 2112 "bfin-parse.y"
    {
	  if ((yyvsp[0].r0).r0 == 1)
	    {
	      if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
		{
		  notethat ("dsp32shiftimm: dregs = dregs >> uimm5 (V)\n");
		  (yyval.instr) = DSP32SHIFTIMM (1, &(yyvsp[-5].reg), -uimm5 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), 2, 0);
		}
	      else
	        return yyerror ("Register mismatch");
	    }
	  else
	    {
	      if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
		{
		  notethat ("dsp32shiftimm: dregs = dregs >> uimm5\n");
		  (yyval.instr) = DSP32SHIFTIMM (2, &(yyvsp[-5].reg), -imm6 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), 2, 0);
		}
	      else if (IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg)) && EXPR_VALUE ((yyvsp[-1].expr)) == 2)
		{
		  notethat ("PTR2op: pregs = pregs >> 2\n");
		  (yyval.instr) = PTR2OP (&(yyvsp[-5].reg), &(yyvsp[-3].reg), 3);
		}
	      else if (IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg)) && EXPR_VALUE ((yyvsp[-1].expr)) == 1)
		{
		  notethat ("PTR2op: pregs = pregs >> 1\n");
		  (yyval.instr) = PTR2OP (&(yyvsp[-5].reg), &(yyvsp[-3].reg), 4);
		}
	      else
	        return yyerror ("Register mismatch");
	    }
	}
    break;

  case 108:
#line 2145 "bfin-parse.y"
    {
	  if (IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("dsp32shiftimm:  dregs_half =  dregs_half >> uimm5\n");
	      (yyval.instr) = DSP32SHIFTIMM (0, &(yyvsp[-4].reg), -uimm5 ((yyvsp[0].expr)), &(yyvsp[-2].reg), 2, HL2 ((yyvsp[-4].reg), (yyvsp[-2].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 109:
#line 2155 "bfin-parse.y"
    {
	  if (IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("dsp32shiftimm: dregs_half = dregs_half >>> uimm5\n");
	      (yyval.instr) = DSP32SHIFTIMM (0, &(yyvsp[-5].reg), -uimm5 ((yyvsp[-1].expr)), &(yyvsp[-3].reg),
				  (yyvsp[0].modcodes).s0, HL2 ((yyvsp[-5].reg), (yyvsp[-3].reg)));
	    }
	  else
	    return yyerror ("Register or modifier mismatch");
	}
    break;

  case 110:
#line 2168 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      if ((yyvsp[0].modcodes).r0)
		{
		  /* Vector?  */
		  notethat ("dsp32shiftimm: dregs  =  dregs >>> uimm5 (V, .)\n");
		  (yyval.instr) = DSP32SHIFTIMM (1, &(yyvsp[-5].reg), -uimm5 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0, 0);
		}
	      else
		{
		  notethat ("dsp32shiftimm: dregs  =  dregs >>> uimm5 (.)\n");
		  (yyval.instr) = DSP32SHIFTIMM (2, &(yyvsp[-5].reg), -uimm5 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0, 0);
		}
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 111:
#line 2188 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = ONES dregs\n");
	      (yyval.instr) = DSP32SHIFT (6, &(yyvsp[-3].reg), 0, &(yyvsp[0].reg), 3, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 112:
#line 2199 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs = PACK (dregs_hi , dregs_hi )\n");
	      (yyval.instr) = DSP32SHIFT (4, &(yyvsp[-7].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), HL2 ((yyvsp[-3].reg), (yyvsp[-1].reg)), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 113:
#line 2210 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-9].reg))
	      && (yyvsp[-3].reg).regno == REG_A0
	      && IS_DREG ((yyvsp[-1].reg)) && !IS_H ((yyvsp[-9].reg)) && !IS_A1 ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = CC = BXORSHIFT (A0 , dregs )\n");
	      (yyval.instr) = DSP32SHIFT (11, &(yyvsp[-9].reg), &(yyvsp[-1].reg), 0, 0, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 114:
#line 2223 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-9].reg))
	      && (yyvsp[-3].reg).regno == REG_A0
	      && IS_DREG ((yyvsp[-1].reg)) && !IS_H ((yyvsp[-9].reg)) && !IS_A1 ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = CC = BXOR (A0 , dregs)\n");
	      (yyval.instr) = DSP32SHIFT (11, &(yyvsp[-9].reg), &(yyvsp[-1].reg), 0, 1, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 115:
#line 2236 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-11].reg)) && !IS_H ((yyvsp[-11].reg)) && !REG_SAME ((yyvsp[-5].reg), (yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = CC = BXOR (A0 , A1 , CC)\n");
	      (yyval.instr) = DSP32SHIFT (12, &(yyvsp[-11].reg), 0, 0, 1, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 116:
#line 2247 "bfin-parse.y"
    {
	  if (REG_SAME ((yyvsp[-4].reg), (yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: Ax = ROT Ax BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (3, 0, &(yyvsp[0].reg), 0, 2, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 117:
#line 2258 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs = ROT dregs BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (2, &(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-2].reg), 3, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 118:
#line 2269 "bfin-parse.y"
    {
	  if (IS_IMM ((yyvsp[0].expr), 6))
	    {
	      notethat ("dsp32shiftimm: An = ROT An BY imm6\n");
	      (yyval.instr) = DSP32SHIFTIMM (3, 0, imm6 ((yyvsp[0].expr)), 0, 2, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 119:
#line 2280 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_IMM ((yyvsp[0].expr), 6))
	    {
	      (yyval.instr) = DSP32SHIFTIMM (2, &(yyvsp[-5].reg), imm6 ((yyvsp[0].expr)), &(yyvsp[-2].reg), 3, IS_A1 ((yyvsp[-5].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 120:
#line 2290 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = SIGNBITS An\n");
	      (yyval.instr) = DSP32SHIFT (6, &(yyvsp[-3].reg), 0, 0, IS_A1 ((yyvsp[0].reg)), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 121:
#line 2301 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = SIGNBITS dregs\n");
	      (yyval.instr) = DSP32SHIFT (5, &(yyvsp[-3].reg), 0, &(yyvsp[0].reg), 0, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 122:
#line 2312 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = SIGNBITS dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (5, &(yyvsp[-3].reg), 0, &(yyvsp[0].reg), 1 + IS_H ((yyvsp[0].reg)), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 123:
#line 2324 "bfin-parse.y"
    {
	  if (IS_DREG_L ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = VIT_MAX (dregs) (..)\n");
	      (yyval.instr) = DSP32SHIFT (9, &(yyvsp[-6].reg), 0, &(yyvsp[-2].reg), ((yyvsp[0].r0).r0 ? 0 : 1), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 124:
#line 2335 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-8].reg)) && IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: dregs = VIT_MAX (dregs, dregs) (ASR)\n");
	      (yyval.instr) = DSP32SHIFT (9, &(yyvsp[-8].reg), &(yyvsp[-2].reg), &(yyvsp[-4].reg), 2 | ((yyvsp[0].r0).r0 ? 0 : 1), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 125:
#line 2346 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-4].reg)) && !IS_A1 ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: BITMUX (dregs , dregs , A0) (ASR)\n");
	      (yyval.instr) = DSP32SHIFT (8, 0, &(yyvsp[-6].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 126:
#line 2357 "bfin-parse.y"
    {
	  if (!IS_A1 ((yyvsp[-8].reg)) && !IS_A1 ((yyvsp[-5].reg)) && IS_A1 ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: A0 = BXORSHIFT (A0 , A1 , CC )\n");
	      (yyval.instr) = DSP32SHIFT (12, 0, 0, 0, 0, 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 127:
#line 2370 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: BITCLR (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 4);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 128:
#line 2382 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: BITCLR (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 2);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 129:
#line 2394 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: BITCLR (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 130:
#line 2405 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: CC =! BITTST (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 0);
	    }
	  else
	    return yyerror ("Register mismatch or value error");
	}
    break;

  case 131:
#line 2416 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: CC = BITTST (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 1);
	    }
	  else
	    return yyerror ("Register mismatch or value error");
	}
    break;

  case 132:
#line 2427 "bfin-parse.y"
    {
	  if ((IS_DREG ((yyvsp[-2].reg)) || IS_PREG ((yyvsp[-2].reg)))
	      && (IS_DREG ((yyvsp[0].reg)) || IS_PREG ((yyvsp[0].reg))))
	    {
	      notethat ("ccMV: IF ! CC gregs = gregs\n");
	      (yyval.instr) = CCMV (&(yyvsp[0].reg), &(yyvsp[-2].reg), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 133:
#line 2439 "bfin-parse.y"
    {
	  if ((IS_DREG ((yyvsp[0].reg)) || IS_PREG ((yyvsp[0].reg)))
	      && (IS_DREG ((yyvsp[-2].reg)) || IS_PREG ((yyvsp[-2].reg))))
	    {
	      notethat ("ccMV: IF CC gregs = gregs\n");
	      (yyval.instr) = CCMV (&(yyvsp[0].reg), &(yyvsp[-2].reg), 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 134:
#line 2451 "bfin-parse.y"
    {
	  if (IS_PCREL10 ((yyvsp[0].expr)))
	    {
	      notethat ("BRCC: IF !CC JUMP  pcrel11m2\n");
	      (yyval.instr) = BRCC (0, 0, (yyvsp[0].expr));
	    }
	  else
	    return yyerror ("Bad jump offset");
	}
    break;

  case 135:
#line 2462 "bfin-parse.y"
    {
	  if (IS_PCREL10 ((yyvsp[-3].expr)))
	    {
	      notethat ("BRCC: IF !CC JUMP  pcrel11m2\n");
	      (yyval.instr) = BRCC (0, 1, (yyvsp[-3].expr));
	    }
	  else
	    return yyerror ("Bad jump offset");
	}
    break;

  case 136:
#line 2473 "bfin-parse.y"
    {
	  if (IS_PCREL10 ((yyvsp[0].expr)))
	    {
	      notethat ("BRCC: IF CC JUMP  pcrel11m2\n");
	      (yyval.instr) = BRCC (1, 0, (yyvsp[0].expr));
	    }
	  else
	    return yyerror ("Bad jump offset");
	}
    break;

  case 137:
#line 2484 "bfin-parse.y"
    {
	  if (IS_PCREL10 ((yyvsp[-3].expr)))
	    {
	      notethat ("BRCC: IF !CC JUMP  pcrel11m2\n");
	      (yyval.instr) = BRCC (1, 1, (yyvsp[-3].expr));
	    }
	  else
	    return yyerror ("Bad jump offset");
	}
    break;

  case 138:
#line 2494 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: NOP\n");
	  (yyval.instr) = PROGCTRL (0, 0);
	}
    break;

  case 139:
#line 2500 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: RTS\n");
	  (yyval.instr) = PROGCTRL (1, 0);
	}
    break;

  case 140:
#line 2506 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: RTI\n");
	  (yyval.instr) = PROGCTRL (1, 1);
	}
    break;

  case 141:
#line 2512 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: RTX\n");
	  (yyval.instr) = PROGCTRL (1, 2);
	}
    break;

  case 142:
#line 2518 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: RTN\n");
	  (yyval.instr) = PROGCTRL (1, 3);
	}
    break;

  case 143:
#line 2524 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: RTE\n");
	  (yyval.instr) = PROGCTRL (1, 4);
	}
    break;

  case 144:
#line 2530 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: IDLE\n");
	  (yyval.instr) = PROGCTRL (2, 0);
	}
    break;

  case 145:
#line 2536 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: CSYNC\n");
	  (yyval.instr) = PROGCTRL (2, 3);
	}
    break;

  case 146:
#line 2542 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: SSYNC\n");
	  (yyval.instr) = PROGCTRL (2, 4);
	}
    break;

  case 147:
#line 2548 "bfin-parse.y"
    {
	  notethat ("ProgCtrl: EMUEXCPT\n");
	  (yyval.instr) = PROGCTRL (2, 5);
	}
    break;

  case 148:
#line 2554 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ProgCtrl: CLI dregs\n");
	      (yyval.instr) = PROGCTRL (3, (yyvsp[0].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Dreg expected for CLI");
	}
    break;

  case 149:
#line 2565 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ProgCtrl: STI dregs\n");
	      (yyval.instr) = PROGCTRL (4, (yyvsp[0].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Dreg expected for STI");
	}
    break;

  case 150:
#line 2576 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: JUMP (pregs )\n");
	      (yyval.instr) = PROGCTRL (5, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Bad register for indirect jump");
	}
    break;

  case 151:
#line 2587 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: CALL (pregs )\n");
	      (yyval.instr) = PROGCTRL (6, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Bad register for indirect call");
	}
    break;

  case 152:
#line 2598 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: CALL (PC + pregs )\n");
	      (yyval.instr) = PROGCTRL (7, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Bad register for indirect call");
	}
    break;

  case 153:
#line 2609 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: JUMP (PC + pregs )\n");
	      (yyval.instr) = PROGCTRL (8, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Bad register for indirect jump");
	}
    break;

  case 154:
#line 2620 "bfin-parse.y"
    {
	  if (IS_UIMM ((yyvsp[0].expr), 4))
	    {
	      notethat ("ProgCtrl: RAISE uimm4\n");
	      (yyval.instr) = PROGCTRL (9, uimm4 ((yyvsp[0].expr)));
	    }
	  else
	    return yyerror ("Bad value for RAISE");
	}
    break;

  case 155:
#line 2631 "bfin-parse.y"
    {
		notethat ("ProgCtrl: EMUEXCPT\n");
		(yyval.instr) = PROGCTRL (10, uimm4 ((yyvsp[0].expr)));
	}
    break;

  case 156:
#line 2637 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: TESTSET (pregs )\n");
	      (yyval.instr) = PROGCTRL (11, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Preg expected");
	}
    break;

  case 157:
#line 2648 "bfin-parse.y"
    {
	  if (IS_PCREL12 ((yyvsp[0].expr)))
	    {
	      notethat ("UJUMP: JUMP pcrel12\n");
	      (yyval.instr) = UJUMP ((yyvsp[0].expr));
	    }
	  else
	    return yyerror ("Bad value for relative jump");
	}
    break;

  case 158:
#line 2659 "bfin-parse.y"
    {
	  if (IS_PCREL12 ((yyvsp[0].expr)))
	    {
	      notethat ("UJUMP: JUMP_DOT_S pcrel12\n");
	      (yyval.instr) = UJUMP((yyvsp[0].expr));
	    }
	  else
	    return yyerror ("Bad value for relative jump");
	}
    break;

  case 159:
#line 2670 "bfin-parse.y"
    {
	  if (IS_PCREL24 ((yyvsp[0].expr)))
	    {
	      notethat ("CALLa: jump.l pcrel24\n");
	      (yyval.instr) = CALLA ((yyvsp[0].expr), 0);
	    }
	  else
	    return yyerror ("Bad value for long jump");
	}
    break;

  case 160:
#line 2681 "bfin-parse.y"
    {
	  if (IS_PCREL24 ((yyvsp[0].expr)))
	    {
	      notethat ("CALLa: jump.l pcrel24\n");
	      (yyval.instr) = CALLA ((yyvsp[0].expr), 2);
	    }
	  else
	    return yyerror ("Bad value for long jump");
	}
    break;

  case 161:
#line 2692 "bfin-parse.y"
    {
	  if (IS_PCREL24 ((yyvsp[0].expr)))
	    {
	      notethat ("CALLa: CALL pcrel25m2\n");
	      (yyval.instr) = CALLA ((yyvsp[0].expr), 1);
	    }
	  else
	    return yyerror ("Bad call address");
	}
    break;

  case 162:
#line 2702 "bfin-parse.y"
    {
	  if (IS_PCREL24 ((yyvsp[0].expr)))
	    {
	      notethat ("CALLa: CALL pcrel25m2\n");
	      (yyval.instr) = CALLA ((yyvsp[0].expr), 2);
	    }
	  else
	    return yyerror ("Bad call address");
	}
    break;

  case 163:
#line 2715 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[-1].reg), 8);
	  else
	    return yyerror ("Bad registers for DIVQ");
	}
    break;

  case 164:
#line 2723 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[-1].reg), 9);
	  else
	    return yyerror ("Bad registers for DIVS");
	}
    break;

  case 165:
#line 2731 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      if ((yyvsp[0].modcodes).r0 == 0 && (yyvsp[0].modcodes).s0 == 0 && (yyvsp[0].modcodes).aop == 0)
		{
		  notethat ("ALU2op: dregs = - dregs\n");
		  (yyval.instr) = ALU2OP (&(yyvsp[-4].reg), &(yyvsp[-1].reg), 14);
		}
	      else if ((yyvsp[0].modcodes).r0 == 1 && (yyvsp[0].modcodes).s0 == 0 && (yyvsp[0].modcodes).aop == 3)
		{
		  notethat ("dsp32alu: dregs = - dregs (.)\n");
		  (yyval.instr) = DSP32ALU (15, 0, 0, &(yyvsp[-4].reg), &(yyvsp[-1].reg), 0, (yyvsp[0].modcodes).s0, 0, 3);
		}
	      else
		{
		  notethat ("dsp32alu: dregs = - dregs (.)\n");
		  (yyval.instr) = DSP32ALU (7, 0, 0, &(yyvsp[-4].reg), &(yyvsp[-1].reg), 0, (yyvsp[0].modcodes).s0, 0, 3);
		}
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 166:
#line 2755 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs = ~dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[0].reg), 15);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 167:
#line 2766 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs >>= dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 1);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 168:
#line 2777 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("LOGI2op: dregs >>= uimm5\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-2].reg), uimm5 ((yyvsp[0].expr)), 6);
	    }
	  else
	    return yyerror ("Dregs expected or value error");
	}
    break;

  case 169:
#line 2788 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs >>>= dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 170:
#line 2799 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs <<= dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 2);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 171:
#line 2810 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("LOGI2op: dregs <<= uimm5\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-2].reg), uimm5 ((yyvsp[0].expr)), 7);
	    }
	  else
	    return yyerror ("Dregs expected or const value error");
	}
    break;

  case 172:
#line 2822 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("LOGI2op: dregs >>>= uimm5\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-2].reg), uimm5 ((yyvsp[0].expr)), 5);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 173:
#line 2835 "bfin-parse.y"
    {
	  notethat ("CaCTRL: FLUSH [ pregs ]\n");
	  if (IS_PREG ((yyvsp[-1].reg)))
	    (yyval.instr) = CACTRL (&(yyvsp[-1].reg), 0, 2);
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
    break;

  case 174:
#line 2844 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("CaCTRL: FLUSH [ pregs ++ ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[0].reg), 1, 2);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
    break;

  case 175:
#line 2855 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("CaCTRL: FLUSHINV [ pregs ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[-1].reg), 0, 1);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
    break;

  case 176:
#line 2866 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("CaCTRL: FLUSHINV [ pregs ++ ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[0].reg), 1, 1);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
    break;

  case 177:
#line 2878 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("CaCTRL: IFLUSH [ pregs ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[-1].reg), 0, 3);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
    break;

  case 178:
#line 2889 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("CaCTRL: IFLUSH [ pregs ++ ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[0].reg), 1, 3);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
    break;

  case 179:
#line 2900 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("CaCTRL: PREFETCH [ pregs ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[-1].reg), 0, 0);
	    }
	  else
	    return yyerror ("Bad register(s) for PREFETCH");
	}
    break;

  case 180:
#line 2911 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("CaCTRL: PREFETCH [ pregs ++ ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[0].reg), 1, 0);
	    }
	  else
	    return yyerror ("Bad register(s) for PREFETCH");
	}
    break;

  case 181:
#line 2925 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDST: B [ pregs <post_op> ] = dregs\n");
	      (yyval.instr) = LDST (&(yyvsp[-4].reg), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 2, 0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
    break;

  case 182:
#line 2937 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-5].reg)) && IS_RANGE(16, (yyvsp[-3].expr), (yyvsp[-4].r0).r0, 1) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDST: B [ pregs + imm16 ] = dregs\n");
	      if ((yyvsp[-4].r0).r0)
		neg_value ((yyvsp[-3].expr));
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-5].reg), &(yyvsp[0].reg), 1, 2, 0, (yyvsp[-3].expr));
	    }
	  else
	    return yyerror ("Register mismatch or const size wrong");
	}
    break;

  case 183:
#line 2952 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-5].reg)) && IS_URANGE (4, (yyvsp[-3].expr), (yyvsp[-4].r0).r0, 2) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDSTii: W [ pregs +- uimm5m2 ] = dregs\n");
	      (yyval.instr) = LDSTII (&(yyvsp[-5].reg), &(yyvsp[0].reg), (yyvsp[-3].expr), 1, 1);
	    }
	  else if (IS_PREG ((yyvsp[-5].reg)) && IS_RANGE(16, (yyvsp[-3].expr), (yyvsp[-4].r0).r0, 2) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDSTidxI: W [ pregs + imm17m2 ] = dregs\n");
	      if ((yyvsp[-4].r0).r0)
		neg_value ((yyvsp[-3].expr));
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-5].reg), &(yyvsp[0].reg), 1, 1, 0, (yyvsp[-3].expr));
	    }
	  else
	    return yyerror ("Bad register(s) or wrong constant size");
	}
    break;

  case 184:
#line 2971 "bfin-parse.y"
    {
	  if (IS_PREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDST: W [ pregs <post_op> ] = dregs\n");
	      (yyval.instr) = LDST (&(yyvsp[-4].reg), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 1, 0, 1);
	    }
	  else
	    return yyerror ("Bad register(s) for STORE");
	}
    break;

  case 185:
#line 2982 "bfin-parse.y"
    {
	  if (IS_IREG ((yyvsp[-4].reg)))
	    {
	      notethat ("dspLDST: W [ iregs <post_op> ] = dregs_half\n");
	      (yyval.instr) = DSPLDST (&(yyvsp[-4].reg), 1 + IS_H ((yyvsp[0].reg)), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 1);
	    }
	  else if ((yyvsp[-3].modcodes).x0 == 2 && IS_PREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDSTpmod: W [ pregs <post_op>] = dregs_half\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-4].reg), &(yyvsp[0].reg), &(yyvsp[-4].reg), 1 + IS_H ((yyvsp[0].reg)), 1);
	      
	    }
	  else
	    return yyerror ("Bad register(s) for STORE");
	}
    break;

  case 186:
#line 3000 "bfin-parse.y"
    {
	  Expr_Node *tmp = (yyvsp[-3].expr);
	  int ispreg = IS_PREG ((yyvsp[0].reg));

	  if (!IS_PREG ((yyvsp[-5].reg)))
	    return yyerror ("Preg expected for indirect");

	  if (!IS_DREG ((yyvsp[0].reg)) && !ispreg)
	    return yyerror ("Bad source register for STORE");

	  if ((yyvsp[-4].r0).r0)
	    tmp = unary (Expr_Op_Type_NEG, tmp);

	  if (in_range_p (tmp, 0, 63, 3))
	    {
	      notethat ("LDSTii: dpregs = [ pregs + uimm6m4 ]\n");
	      (yyval.instr) = LDSTII (&(yyvsp[-5].reg), &(yyvsp[0].reg), tmp, 1, ispreg ? 3 : 0);
	    }
	  else if ((yyvsp[-5].reg).regno == REG_FP && in_range_p (tmp, -128, 0, 3))
	    {
	      notethat ("LDSTiiFP: dpregs = [ FP - uimm7m4 ]\n");
	      tmp = unary (Expr_Op_Type_NEG, tmp);
	      (yyval.instr) = LDSTIIFP (tmp, &(yyvsp[0].reg), 1);
	    }
	  else if (in_range_p (tmp, -131072, 131071, 3))
	    {
	      notethat ("LDSTidxI: [ pregs + imm18m4 ] = dpregs\n");
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-5].reg), &(yyvsp[0].reg), 1, 0, ispreg ? 1: 0, tmp);
	    }
	  else
	    return yyerror ("Displacement out of range for store");
	}
    break;

  case 187:
#line 3034 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-8].reg)) && IS_PREG ((yyvsp[-4].reg)) && IS_URANGE (4, (yyvsp[-2].expr), (yyvsp[-3].r0).r0, 2))
	    {
	      notethat ("LDSTii: dregs = W [ pregs + uimm4s2 ] (.)\n");
	      (yyval.instr) = LDSTII (&(yyvsp[-4].reg), &(yyvsp[-8].reg), (yyvsp[-2].expr), 0, 1 << (yyvsp[0].r0).r0);
	    }
	  else if (IS_DREG ((yyvsp[-8].reg)) && IS_PREG ((yyvsp[-4].reg)) && IS_RANGE(16, (yyvsp[-2].expr), (yyvsp[-3].r0).r0, 2))
	    {
	      notethat ("LDSTidxI: dregs = W [ pregs + imm17m2 ] (.)\n");
	      if ((yyvsp[-3].r0).r0)
		neg_value ((yyvsp[-2].expr));
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-4].reg), &(yyvsp[-8].reg), 0, 1, (yyvsp[0].r0).r0, (yyvsp[-2].expr));
	    }
	  else
	    return yyerror ("Bad register or constant for LOAD");
	}
    break;

  case 188:
#line 3052 "bfin-parse.y"
    {
	  if (IS_IREG ((yyvsp[-2].reg)))
	    {
	      notethat ("dspLDST: dregs_half = W [ iregs ]\n");
	      (yyval.instr) = DSPLDST(&(yyvsp[-2].reg), 1 + IS_H ((yyvsp[-6].reg)), &(yyvsp[-6].reg), (yyvsp[-1].modcodes).x0, 0);
	    }
	  else if ((yyvsp[-1].modcodes).x0 == 2 && IS_DREG ((yyvsp[-6].reg)) && IS_PREG ((yyvsp[-2].reg)))
	    {
	      notethat ("LDSTpmod: dregs_half = W [ pregs ]\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-2].reg), &(yyvsp[-6].reg), &(yyvsp[-2].reg), 1 + IS_H ((yyvsp[-6].reg)), 0);
	    }
	  else
	    return yyerror ("Bad register or post_op for LOAD");
	}
    break;

  case 189:
#line 3069 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_PREG ((yyvsp[-3].reg)))
	    {
	      notethat ("LDST: dregs = W [ pregs <post_op> ] (.)\n");
	      (yyval.instr) = LDST (&(yyvsp[-3].reg), &(yyvsp[-7].reg), (yyvsp[-2].modcodes).x0, 1, (yyvsp[0].r0).r0, 0);
	    }
	  else
	    return yyerror ("Bad register for LOAD");
	}
    break;

  case 190:
#line 3080 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-8].reg)) && IS_PREG ((yyvsp[-4].reg)) && IS_PREG ((yyvsp[-2].reg)))
	    {
	      notethat ("LDSTpmod: dregs = W [ pregs ++ pregs ] (.)\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-4].reg), &(yyvsp[-8].reg), &(yyvsp[-2].reg), 3, (yyvsp[0].r0).r0);
	    }
	  else
	    return yyerror ("Bad register for LOAD");
	}
    break;

  case 191:
#line 3091 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_PREG ((yyvsp[-3].reg)) && IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("LDSTpmod: dregs_half = W [ pregs ++ pregs ]\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-3].reg), &(yyvsp[-7].reg), &(yyvsp[-1].reg), 1 + IS_H ((yyvsp[-7].reg)), 0);
	    }
	  else
	    return yyerror ("Bad register for LOAD");
	}
    break;

  case 192:
#line 3102 "bfin-parse.y"
    {
	  if (IS_IREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("dspLDST: [ iregs <post_op> ] = dregs\n");
	      (yyval.instr) = DSPLDST(&(yyvsp[-4].reg), 0, &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 1);
	    }
	  else if (IS_PREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDST: [ pregs <post_op> ] = dregs\n");
	      (yyval.instr) = LDST (&(yyvsp[-4].reg), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 0, 0, 1);
	    }
	  else if (IS_PREG ((yyvsp[-4].reg)) && IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDST: [ pregs <post_op> ] = pregs\n");
	      (yyval.instr) = LDST (&(yyvsp[-4].reg), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 0, 1, 1);
	    }
	  else
	    return yyerror ("Bad register for STORE");
	}
    break;

  case 193:
#line 3123 "bfin-parse.y"
    {
	  if (! IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Expected Dreg for last argument");

	  if (IS_IREG ((yyvsp[-5].reg)) && IS_MREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dspLDST: [ iregs ++ mregs ] = dregs\n");
	      (yyval.instr) = DSPLDST(&(yyvsp[-5].reg), (yyvsp[-3].reg).regno & CODE_MASK, &(yyvsp[0].reg), 3, 1);
	    }
	  else if (IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg)))
	    {
	      notethat ("LDSTpmod: [ pregs ++ pregs ] = dregs\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-3].reg), 0, 1);
	    }
	  else
	    return yyerror ("Bad register for STORE");
	}
    break;

  case 194:
#line 3142 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Expect Dreg as last argument");
	  if (IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg)))
	    {
	      notethat ("LDSTpmod: W [ pregs ++ pregs ] = dregs_half\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-3].reg), 1 + IS_H ((yyvsp[0].reg)), 1);
	    }
	  else
	    return yyerror ("Bad register for STORE");
	}
    break;

  case 195:
#line 3155 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-8].reg)) && IS_PREG ((yyvsp[-4].reg)) && IS_RANGE(16, (yyvsp[-2].expr), (yyvsp[-3].r0).r0, 1))
	    {
	      notethat ("LDSTidxI: dregs = B [ pregs + imm16 ] (%c)\n",
		       (yyvsp[0].r0).r0 ? 'X' : 'Z');
	      if ((yyvsp[-3].r0).r0)
		neg_value ((yyvsp[-2].expr));
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-4].reg), &(yyvsp[-8].reg), 0, 2, (yyvsp[0].r0).r0, (yyvsp[-2].expr));
	    }
	  else
	    return yyerror ("Bad register or value for LOAD");
	}
    break;

  case 196:
#line 3169 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_PREG ((yyvsp[-3].reg)))
	    {
	      notethat ("LDST: dregs = B [ pregs <post_op> ] (%c)\n",
		       (yyvsp[0].r0).r0 ? 'X' : 'Z');
	      (yyval.instr) = LDST (&(yyvsp[-3].reg), &(yyvsp[-7].reg), (yyvsp[-2].modcodes).x0, 2, (yyvsp[0].r0).r0, 0);
	    }
	  else
	    return yyerror ("Bad register for LOAD");
	}
    break;

  case 197:
#line 3181 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-6].reg)) && IS_IREG ((yyvsp[-3].reg)) && IS_MREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dspLDST: dregs = [ iregs ++ mregs ]\n");
	      (yyval.instr) = DSPLDST(&(yyvsp[-3].reg), (yyvsp[-1].reg).regno & CODE_MASK, &(yyvsp[-6].reg), 3, 0);
	    }
	  else if (IS_DREG ((yyvsp[-6].reg)) && IS_PREG ((yyvsp[-3].reg)) && IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("LDSTpmod: dregs = [ pregs ++ pregs ]\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-3].reg), &(yyvsp[-6].reg), &(yyvsp[-1].reg), 0, 0);
	    }
	  else
	    return yyerror ("Bad register for LOAD");
	}
    break;

  case 198:
#line 3197 "bfin-parse.y"
    {
	  Expr_Node *tmp = (yyvsp[-1].expr);
	  int ispreg = IS_PREG ((yyvsp[-6].reg));
	  int isgot = IS_RELOC((yyvsp[-1].expr));

	  if (!IS_PREG ((yyvsp[-3].reg)))
	    return yyerror ("Preg expected for indirect");

	  if (!IS_DREG ((yyvsp[-6].reg)) && !ispreg)
	    return yyerror ("Bad destination register for LOAD");

	  if (tmp->type == Expr_Node_Reloc
	      && strcmp (tmp->value.s_value,
			 "_current_shared_library_p5_offset_") != 0)
	    return yyerror ("Plain symbol used as offset");

	  if ((yyvsp[-2].r0).r0)
	    tmp = unary (Expr_Op_Type_NEG, tmp);

	  if(isgot){
	      notethat ("LDSTidxI: dpregs = [ pregs + sym@got ]\n");
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-3].reg), &(yyvsp[-6].reg), 0, 0, ispreg ? 1: 0, tmp);
	  }
	  else if (in_range_p (tmp, 0, 63, 3))
	    {
	      notethat ("LDSTii: dpregs = [ pregs + uimm7m4 ]\n");
	      (yyval.instr) = LDSTII (&(yyvsp[-3].reg), &(yyvsp[-6].reg), tmp, 0, ispreg ? 3 : 0);
	    }
	  else if ((yyvsp[-3].reg).regno == REG_FP && in_range_p (tmp, -128, 0, 3))
	    {
	      notethat ("LDSTiiFP: dpregs = [ FP - uimm7m4 ]\n");
	      tmp = unary (Expr_Op_Type_NEG, tmp);
	      (yyval.instr) = LDSTIIFP (tmp, &(yyvsp[-6].reg), 0);
	    }
	  else if (in_range_p (tmp, -131072, 131071, 3))
	    {
	      notethat ("LDSTidxI: dpregs = [ pregs + imm18m4 ]\n");
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-3].reg), &(yyvsp[-6].reg), 0, 0, ispreg ? 1: 0, tmp);
	      
	    }
	  else
	    return yyerror ("Displacement out of range for load");
	}
    break;

  case 199:
#line 3242 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_IREG ((yyvsp[-2].reg)))
	    {
	      notethat ("dspLDST: dregs = [ iregs <post_op> ]\n");
	      (yyval.instr) = DSPLDST (&(yyvsp[-2].reg), 0, &(yyvsp[-5].reg), (yyvsp[-1].modcodes).x0, 0);
	    }
	  else if (IS_DREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-2].reg)))
	    {
	      notethat ("LDST: dregs = [ pregs <post_op> ]\n");
	      (yyval.instr) = LDST (&(yyvsp[-2].reg), &(yyvsp[-5].reg), (yyvsp[-1].modcodes).x0, 0, 0, 0);
	    }
	  else if (IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-2].reg)))
	    {
	      if (REG_SAME ((yyvsp[-5].reg), (yyvsp[-2].reg)) && (yyvsp[-1].modcodes).x0 != 2)
		return yyerror ("Pregs can't be same");

	      notethat ("LDST: pregs = [ pregs <post_op> ]\n");
	      (yyval.instr) = LDST (&(yyvsp[-2].reg), &(yyvsp[-5].reg), (yyvsp[-1].modcodes).x0, 0, 1, 0);
	    }
	  else if ((yyvsp[-2].reg).regno == REG_SP && IS_ALLREG ((yyvsp[-5].reg)) && (yyvsp[-1].modcodes).x0 == 0)
	    {
	      notethat ("PushPopReg: allregs = [ SP ++ ]\n");
	      (yyval.instr) = PUSHPOPREG (&(yyvsp[-5].reg), 0);
	    }
	  else
	    return yyerror ("Bad register or value");
	}
    break;

  case 200:
#line 3273 "bfin-parse.y"
    {
	  if ((yyvsp[-10].reg).regno != REG_SP)
	    yyerror ("Stack Pointer expected");
	  if ((yyvsp[-7].reg).regno == REG_R7
	      && IN_RANGE ((yyvsp[-5].expr), 0, 7)
	      && (yyvsp[-3].reg).regno == REG_P5
	      && IN_RANGE ((yyvsp[-1].expr), 0, 5))
	    {
	      notethat ("PushPopMultiple: [ -- SP ] = (R7 : reglim , P5 : reglim )\n");
	      (yyval.instr) = PUSHPOPMULTIPLE (imm5 ((yyvsp[-5].expr)), imm5 ((yyvsp[-1].expr)), 1, 1, 1);
	    }
	  else
	    return yyerror ("Bad register for PushPopMultiple");
	}
    break;

  case 201:
#line 3289 "bfin-parse.y"
    {
	  if ((yyvsp[-6].reg).regno != REG_SP)
	    yyerror ("Stack Pointer expected");

	  if ((yyvsp[-3].reg).regno == REG_R7 && IN_RANGE ((yyvsp[-1].expr), 0, 7))
	    {
	      notethat ("PushPopMultiple: [ -- SP ] = (R7 : reglim )\n");
	      (yyval.instr) = PUSHPOPMULTIPLE (imm5 ((yyvsp[-1].expr)), 0, 1, 0, 1);
	    }
	  else if ((yyvsp[-3].reg).regno == REG_P5 && IN_RANGE ((yyvsp[-1].expr), 0, 6))
	    {
	      notethat ("PushPopMultiple: [ -- SP ] = (P5 : reglim )\n");
	      (yyval.instr) = PUSHPOPMULTIPLE (0, imm5 ((yyvsp[-1].expr)), 0, 1, 1);
	    }
	  else
	    return yyerror ("Bad register for PushPopMultiple");
	}
    break;

  case 202:
#line 3308 "bfin-parse.y"
    {
	  if ((yyvsp[0].reg).regno != REG_SP)
	    yyerror ("Stack Pointer expected");
	  if ((yyvsp[-9].reg).regno == REG_R7 && (IN_RANGE ((yyvsp[-7].expr), 0, 7))
	      && (yyvsp[-5].reg).regno == REG_P5 && (IN_RANGE ((yyvsp[-3].expr), 0, 6)))
	    {
	      notethat ("PushPopMultiple: (R7 : reglim , P5 : reglim ) = [ SP ++ ]\n");
	      (yyval.instr) = PUSHPOPMULTIPLE (imm5 ((yyvsp[-7].expr)), imm5 ((yyvsp[-3].expr)), 1, 1, 0);
	    }
	  else
	    return yyerror ("Bad register range for PushPopMultiple");
	}
    break;

  case 203:
#line 3322 "bfin-parse.y"
    {
	  if ((yyvsp[0].reg).regno != REG_SP)
	    yyerror ("Stack Pointer expected");

	  if ((yyvsp[-5].reg).regno == REG_R7 && IN_RANGE ((yyvsp[-3].expr), 0, 7))
	    {
	      notethat ("PushPopMultiple: (R7 : reglim ) = [ SP ++ ]\n");
	      (yyval.instr) = PUSHPOPMULTIPLE (imm5 ((yyvsp[-3].expr)), 0, 1, 0, 0);
	    }
	  else if ((yyvsp[-5].reg).regno == REG_P5 && IN_RANGE ((yyvsp[-3].expr), 0, 6))
	    {
	      notethat ("PushPopMultiple: (P5 : reglim ) = [ SP ++ ]\n");
	      (yyval.instr) = PUSHPOPMULTIPLE (0, imm5 ((yyvsp[-3].expr)), 0, 1, 0);
	    }
	  else
	    return yyerror ("Bad register range for PushPopMultiple");
	}
    break;

  case 204:
#line 3341 "bfin-parse.y"
    {
	  if ((yyvsp[-2].reg).regno != REG_SP)
	    yyerror ("Stack Pointer expected");

	  if (IS_ALLREG ((yyvsp[0].reg)))
	    {
	      notethat ("PushPopReg: [ -- SP ] = allregs\n");
	      (yyval.instr) = PUSHPOPREG (&(yyvsp[0].reg), 1);
	    }
	  else
	    return yyerror ("Bad register for PushPopReg");
	}
    break;

  case 205:
#line 3357 "bfin-parse.y"
    {
	  if (IS_URANGE (16, (yyvsp[0].expr), 0, 4))
	    (yyval.instr) = LINKAGE (0, uimm16s4 ((yyvsp[0].expr)));
	  else
	    return yyerror ("Bad constant for LINK");
	}
    break;

  case 206:
#line 3365 "bfin-parse.y"
    {
		notethat ("linkage: UNLINK\n");
		(yyval.instr) = LINKAGE (1, 0);
	}
    break;

  case 207:
#line 3374 "bfin-parse.y"
    {
	  if (IS_PCREL4 ((yyvsp[-4].expr)) && IS_LPPCREL10 ((yyvsp[-2].expr)) && IS_CREG ((yyvsp[0].reg)))
	    {
	      notethat ("LoopSetup: LSETUP (pcrel4 , lppcrel10 ) counters\n");
	      (yyval.instr) = LOOPSETUP ((yyvsp[-4].expr), &(yyvsp[0].reg), 0, (yyvsp[-2].expr), 0);
	    }
	  else
	    return yyerror ("Bad register or values for LSETUP");
	  
	}
    break;

  case 208:
#line 3385 "bfin-parse.y"
    {
	  if (IS_PCREL4 ((yyvsp[-6].expr)) && IS_LPPCREL10 ((yyvsp[-4].expr))
	      && IS_PREG ((yyvsp[0].reg)) && IS_CREG ((yyvsp[-2].reg)))
	    {
	      notethat ("LoopSetup: LSETUP (pcrel4 , lppcrel10 ) counters = pregs\n");
	      (yyval.instr) = LOOPSETUP ((yyvsp[-6].expr), &(yyvsp[-2].reg), 1, (yyvsp[-4].expr), &(yyvsp[0].reg));
	    }
	  else
	    return yyerror ("Bad register or values for LSETUP");
	}
    break;

  case 209:
#line 3397 "bfin-parse.y"
    {
	  if (IS_PCREL4 ((yyvsp[-8].expr)) && IS_LPPCREL10 ((yyvsp[-6].expr))
	      && IS_PREG ((yyvsp[-2].reg)) && IS_CREG ((yyvsp[-4].reg)) 
	      && EXPR_VALUE ((yyvsp[0].expr)) == 1)
	    {
	      notethat ("LoopSetup: LSETUP (pcrel4 , lppcrel10 ) counters = pregs >> 1\n");
	      (yyval.instr) = LOOPSETUP ((yyvsp[-8].expr), &(yyvsp[-4].reg), 3, (yyvsp[-6].expr), &(yyvsp[-2].reg));
	    }
	  else
	    return yyerror ("Bad register or values for LSETUP");
	}
    break;

  case 210:
#line 3411 "bfin-parse.y"
    {
	  if (!IS_RELOC ((yyvsp[-1].expr)))
	    return yyerror ("Invalid expression in loop statement");
	  if (!IS_CREG ((yyvsp[0].reg)))
            return yyerror ("Invalid loop counter register");
	(yyval.instr) = bfin_gen_loop ((yyvsp[-1].expr), &(yyvsp[0].reg), 0, 0);
	}
    break;

  case 211:
#line 3419 "bfin-parse.y"
    {
	  if (IS_RELOC ((yyvsp[-3].expr)) && IS_PREG ((yyvsp[0].reg)) && IS_CREG ((yyvsp[-2].reg)))
	    {
	      notethat ("Loop: LOOP expr counters = pregs\n");
	      (yyval.instr) = bfin_gen_loop ((yyvsp[-3].expr), &(yyvsp[-2].reg), 1, &(yyvsp[0].reg));
	    }
	  else
	    return yyerror ("Bad register or values for LOOP");
	}
    break;

  case 212:
#line 3429 "bfin-parse.y"
    {
	  if (IS_RELOC ((yyvsp[-5].expr)) && IS_PREG ((yyvsp[-2].reg)) && IS_CREG ((yyvsp[-4].reg)) && EXPR_VALUE ((yyvsp[0].expr)) == 1)
	    {
	      notethat ("Loop: LOOP expr counters = pregs >> 1\n");
	      (yyval.instr) = bfin_gen_loop ((yyvsp[-5].expr), &(yyvsp[-4].reg), 3, &(yyvsp[-2].reg));
	    }
	  else
	    return yyerror ("Bad register or values for LOOP");
	}
    break;

  case 213:
#line 3441 "bfin-parse.y"
    {
	  notethat ("pseudoDEBUG: DBG\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, 7, 0);
	}
    break;

  case 214:
#line 3446 "bfin-parse.y"
    {
	  notethat ("pseudoDEBUG: DBG REG_A\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, IS_A1 ((yyvsp[0].reg)), 0);
	}
    break;

  case 215:
#line 3451 "bfin-parse.y"
    {
	  notethat ("pseudoDEBUG: DBG allregs\n");
	  (yyval.instr) = bfin_gen_pseudodbg (0, (yyvsp[0].reg).regno & CODE_MASK, (yyvsp[0].reg).regno & CLASS_MASK);
	}
    break;

  case 216:
#line 3457 "bfin-parse.y"
    {
	  if (!IS_DREG ((yyvsp[-1].reg)))
	    return yyerror ("Dregs expected");
	  notethat ("pseudoDEBUG: DBGCMPLX (dregs )\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, 6, (yyvsp[-1].reg).regno & CODE_MASK);
	}
    break;

  case 217:
#line 3465 "bfin-parse.y"
    {
	  notethat ("psedoDEBUG: DBGHALT\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, 5, 0);
	}
    break;

  case 218:
#line 3471 "bfin-parse.y"
    {
	  notethat ("pseudodbg_assert: DBGA (dregs_lo , uimm16 )\n");
	  (yyval.instr) = bfin_gen_pseudodbg_assert (IS_H ((yyvsp[-3].reg)), &(yyvsp[-3].reg), uimm16 ((yyvsp[-1].expr)));
	}
    break;

  case 219:
#line 3477 "bfin-parse.y"
    {
	  notethat ("pseudodbg_assert: DBGAH (dregs , uimm16 )\n");
	  (yyval.instr) = bfin_gen_pseudodbg_assert (3, &(yyvsp[-3].reg), uimm16 ((yyvsp[-1].expr)));
	}
    break;

  case 220:
#line 3483 "bfin-parse.y"
    {
	  notethat ("psedodbg_assert: DBGAL (dregs , uimm16 )\n");
	  (yyval.instr) = bfin_gen_pseudodbg_assert (2, &(yyvsp[-3].reg), uimm16 ((yyvsp[-1].expr)));
	}
    break;

  case 221:
#line 3496 "bfin-parse.y"
    {
	(yyval.reg) = (yyvsp[0].reg);
	}
    break;

  case 222:
#line 3500 "bfin-parse.y"
    {
	(yyval.reg) = (yyvsp[0].reg);
	}
    break;

  case 223:
#line 3509 "bfin-parse.y"
    {
	(yyval.mod).MM = 0;
	(yyval.mod).mod = 0;
	}
    break;

  case 224:
#line 3514 "bfin-parse.y"
    {
	(yyval.mod).MM = 1;
	(yyval.mod).mod = (yyvsp[-1].value);
	}
    break;

  case 225:
#line 3519 "bfin-parse.y"
    {
	(yyval.mod).MM = 1;
	(yyval.mod).mod = (yyvsp[-3].value);
	}
    break;

  case 226:
#line 3524 "bfin-parse.y"
    {
	(yyval.mod).MM = 0;
	(yyval.mod).mod = (yyvsp[-1].value);
	}
    break;

  case 227:
#line 3529 "bfin-parse.y"
    {
	(yyval.mod).MM = 1;
	(yyval.mod).mod = 0;
	}
    break;

  case 228:
#line 3536 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 229:
#line 3540 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 230:
#line 3546 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	}
    break;

  case 231:
#line 3551 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 1;
	(yyval.modcodes).x0 = 0;
	}
    break;

  case 232:
#line 3556 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 1;
	}
    break;

  case 233:
#line 3561 "bfin-parse.y"
    {	
	(yyval.modcodes).s0 = 1;
	(yyval.modcodes).x0 = 1;
	}
    break;

  case 234:
#line 3569 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 235:
#line 3573 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 236:
#line 3579 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	}
    break;

  case 237:
#line 3584 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = (yyvsp[-1].modcodes).s0;
	(yyval.modcodes).x0 = (yyvsp[-1].modcodes).x0;
	}
    break;

  case 238:
#line 3591 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	(yyval.modcodes).aop = 0;
	}
    break;

  case 239:
#line 3597 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	(yyval.modcodes).aop = 1;
	}
    break;

  case 240:
#line 3603 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 1;
	(yyval.modcodes).x0 = 0;
	(yyval.modcodes).aop = 1;
	}
    break;

  case 241:
#line 3611 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	}
    break;

  case 242:
#line 3617 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 2 + (yyvsp[-1].r0).r0;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	}
    break;

  case 243:
#line 3623 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = (yyvsp[-1].modcodes).s0;
	(yyval.modcodes).x0 = (yyvsp[-1].modcodes).x0;
	}
    break;

  case 244:
#line 3629 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 2 + (yyvsp[-3].r0).r0;
	(yyval.modcodes).s0 = (yyvsp[-1].modcodes).s0;
	(yyval.modcodes).x0 = (yyvsp[-1].modcodes).x0;
	}
    break;

  case 245:
#line 3635 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 2 + (yyvsp[-1].r0).r0;
	(yyval.modcodes).s0 = (yyvsp[-3].modcodes).s0;
	(yyval.modcodes).x0 = (yyvsp[-3].modcodes).x0;
	}
    break;

  case 246:
#line 3643 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 247:
#line 3647 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 248:
#line 3651 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 249:
#line 3657 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 250:
#line 3661 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 251:
#line 3665 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 252:
#line 3671 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).aop = 0;
	}
    break;

  case 253:
#line 3677 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).aop = 3;
	}
    break;

  case 254:
#line 3683 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 1;
	(yyval.modcodes).aop = 3;
	}
    break;

  case 255:
#line 3689 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).aop = 3;
	}
    break;

  case 256:
#line 3695 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 1;
	}
    break;

  case 257:
#line 3700 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 1;
	}
    break;

  case 258:
#line 3707 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 259:
#line 3711 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 260:
#line 3717 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 0;
	}
    break;

  case 261:
#line 3721 "bfin-parse.y"
    {
	(yyval.modcodes).s0 = 1;
	}
    break;

  case 262:
#line 3728 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 263:
#line 3732 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 264:
#line 3736 "bfin-parse.y"
    {
	(yyval.r0).r0 = 3;
	}
    break;

  case 265:
#line 3740 "bfin-parse.y"
    {
	(yyval.r0).r0 = 2;
	}
    break;

  case 266:
#line 3746 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 267:
#line 3750 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 268:
#line 3757 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 1;
	}
    break;

  case 269:
#line 3762 "bfin-parse.y"
    {
	if ((yyvsp[-1].value) != M_T)
	  return yyerror ("Bad modifier");
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 0;
	}
    break;

  case 270:
#line 3769 "bfin-parse.y"
    {
	if ((yyvsp[-3].value) != M_T)
	  return yyerror ("Bad modifier");
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 1;
	}
    break;

  case 271:
#line 3776 "bfin-parse.y"
    {
	if ((yyvsp[-1].value) != M_T)
	  return yyerror ("Bad modifier");
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 1;
	}
    break;

  case 272:
#line 3788 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 273:
#line 3792 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 274:
#line 3796 "bfin-parse.y"
    {
	(yyval.r0).r0 = 2;
	}
    break;

  case 275:
#line 3802 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 276:
#line 3806 "bfin-parse.y"
    {
	  if ((yyvsp[-1].value) == M_W32)
	    (yyval.r0).r0 = 1;
	  else
	    return yyerror ("Only (W32) allowed");
	}
    break;

  case 277:
#line 3815 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 278:
#line 3819 "bfin-parse.y"
    {
	  if ((yyvsp[-1].value) == M_IU)
	    (yyval.r0).r0 = 3;
	  else
	    return yyerror ("(IU) expected");
	}
    break;

  case 279:
#line 3828 "bfin-parse.y"
    {
	(yyval.reg) = (yyvsp[-1].reg);
	}
    break;

  case 280:
#line 3834 "bfin-parse.y"
    {
	(yyval.reg) = (yyvsp[-2].reg);
	}
    break;

  case 281:
#line 3843 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 282:
#line 3847 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 283:
#line 3854 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 284:
#line 3858 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 285:
#line 3862 "bfin-parse.y"
    {
	(yyval.r0).r0 = 2;
	}
    break;

  case 286:
#line 3866 "bfin-parse.y"
    {
	(yyval.r0).r0 = 3;
	}
    break;

  case 287:
#line 3873 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 288:
#line 3877 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 289:
#line 3884 "bfin-parse.y"
    {
	  (yyval.modcodes).r0 = 1;	/* HL.  */
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 0;	/* aop.  */
	}
    break;

  case 290:
#line 3892 "bfin-parse.y"
    {
	  (yyval.modcodes).r0 = 1;	/* HL.  */
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 1;	/* aop.  */
	}
    break;

  case 291:
#line 3900 "bfin-parse.y"
    {
	  (yyval.modcodes).r0 = 0;	/* HL.  */
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 0;	/* aop.  */
	}
    break;

  case 292:
#line 3908 "bfin-parse.y"
    {
	  (yyval.modcodes).r0 = 0;	/* HL.  */
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 1;
	}
    break;

  case 293:
#line 3916 "bfin-parse.y"
    {
	  (yyval.modcodes).r0 = 1;	/* HL.  */
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 0;	/* aop.  */
	}
    break;

  case 294:
#line 3923 "bfin-parse.y"
    {
	  (yyval.modcodes).r0 = 1;	/* HL.  */
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 1;	/* aop.  */
	}
    break;

  case 295:
#line 3930 "bfin-parse.y"
    {
	  (yyval.modcodes).r0 = 0;	/* HL.  */
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 0;	/* aop.  */
	}
    break;

  case 296:
#line 3938 "bfin-parse.y"
    {
	  (yyval.modcodes).r0 = 0;	/* HL.  */
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 1;	/* aop.  */
	}
    break;

  case 297:
#line 3948 "bfin-parse.y"
    {
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* HL.  */
	}
    break;

  case 298:
#line 3953 "bfin-parse.y"
    {
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 1;	/* HL.  */
	}
    break;

  case 299:
#line 3958 "bfin-parse.y"
    {
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* HL.  */
	}
    break;

  case 300:
#line 3963 "bfin-parse.y"
    {
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 1;	/* HL.  */
	}
    break;

  case 301:
#line 3970 "bfin-parse.y"
    {
	(yyval.modcodes).x0 = 2;
	}
    break;

  case 302:
#line 3974 "bfin-parse.y"
    {
	(yyval.modcodes).x0 = 0;
	}
    break;

  case 303:
#line 3978 "bfin-parse.y"
    {
	(yyval.modcodes).x0 = 1;
	}
    break;

  case 304:
#line 3987 "bfin-parse.y"
    {
	(yyval.reg) = (yyvsp[-1].reg);
	}
    break;

  case 305:
#line 3994 "bfin-parse.y"
    {
	(yyval.reg) = (yyvsp[-1].reg);
	}
    break;

  case 306:
#line 4001 "bfin-parse.y"
    {
	(yyval.reg) = (yyvsp[-1].reg);
	}
    break;

  case 307:
#line 4008 "bfin-parse.y"
    {
	  if (IS_A1 ((yyvsp[0].reg)) && IS_EVEN ((yyvsp[-2].reg)))
	    return yyerror ("Cannot move A1 to even register");
	  else if (!IS_A1 ((yyvsp[0].reg)) && !IS_EVEN ((yyvsp[-2].reg)))
	    return yyerror ("Cannot move A0 to odd register");

	  (yyval.macfunc).w = 1;
          (yyval.macfunc).P = 1;
          (yyval.macfunc).n = IS_A1 ((yyvsp[0].reg));
	  (yyval.macfunc).op = 3;
          (yyval.macfunc).dst = (yyvsp[-2].reg);
	  (yyval.macfunc).s0.regno = 0;
          (yyval.macfunc).s1.regno = 0;
	}
    break;

  case 308:
#line 4023 "bfin-parse.y"
    {
	  (yyval.macfunc) = (yyvsp[0].macfunc);
	  (yyval.macfunc).w = 0; (yyval.macfunc).P = 0;
	  (yyval.macfunc).dst.regno = 0;
	}
    break;

  case 309:
#line 4029 "bfin-parse.y"
    {
	  if ((yyvsp[-1].macfunc).n && IS_EVEN ((yyvsp[-4].reg)))
	    return yyerror ("Cannot move A1 to even register");
	  else if (!(yyvsp[-1].macfunc).n && !IS_EVEN ((yyvsp[-4].reg)))
	    return yyerror ("Cannot move A0 to odd register");

	  (yyval.macfunc) = (yyvsp[-1].macfunc);
	  (yyval.macfunc).w = 1;
          (yyval.macfunc).P = 1;
          (yyval.macfunc).dst = (yyvsp[-4].reg);
	}
    break;

  case 310:
#line 4042 "bfin-parse.y"
    {
	  if ((yyvsp[-1].macfunc).n && !IS_H ((yyvsp[-4].reg)))
	    return yyerror ("Cannot move A1 to low half of register");
	  else if (!(yyvsp[-1].macfunc).n && IS_H ((yyvsp[-4].reg)))
	    return yyerror ("Cannot move A0 to high half of register");

	  (yyval.macfunc) = (yyvsp[-1].macfunc);
	  (yyval.macfunc).w = 1;
	  (yyval.macfunc).P = 0;
          (yyval.macfunc).dst = (yyvsp[-4].reg);
	}
    break;

  case 311:
#line 4055 "bfin-parse.y"
    {
	  if (IS_A1 ((yyvsp[0].reg)) && !IS_H ((yyvsp[-2].reg)))
	    return yyerror ("Cannot move A1 to low half of register");
	  else if (!IS_A1 ((yyvsp[0].reg)) && IS_H ((yyvsp[-2].reg)))
	    return yyerror ("Cannot move A0 to high half of register");

	  (yyval.macfunc).w = 1;
	  (yyval.macfunc).P = 0;
	  (yyval.macfunc).n = IS_A1 ((yyvsp[0].reg));
	  (yyval.macfunc).op = 3;
          (yyval.macfunc).dst = (yyvsp[-2].reg);
	  (yyval.macfunc).s0.regno = 0;
          (yyval.macfunc).s1.regno = 0;
	}
    break;

  case 312:
#line 4073 "bfin-parse.y"
    {
	  (yyval.macfunc).n = IS_A1 ((yyvsp[-1].reg));
	  (yyval.macfunc).op = 0;
	  (yyval.macfunc).s0 = (yyvsp[0].macfunc).s0;
	  (yyval.macfunc).s1 = (yyvsp[0].macfunc).s1;
	}
    break;

  case 313:
#line 4080 "bfin-parse.y"
    {
	  (yyval.macfunc).n = IS_A1 ((yyvsp[-1].reg));
	  (yyval.macfunc).op = 1;
	  (yyval.macfunc).s0 = (yyvsp[0].macfunc).s0;
	  (yyval.macfunc).s1 = (yyvsp[0].macfunc).s1;
	}
    break;

  case 314:
#line 4087 "bfin-parse.y"
    {
	  (yyval.macfunc).n = IS_A1 ((yyvsp[-1].reg));
	  (yyval.macfunc).op = 2;
	  (yyval.macfunc).s0 = (yyvsp[0].macfunc).s0;
	  (yyval.macfunc).s1 = (yyvsp[0].macfunc).s1;
	}
    break;

  case 315:
#line 4097 "bfin-parse.y"
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      (yyval.macfunc).s0 = (yyvsp[-2].reg);
              (yyval.macfunc).s1 = (yyvsp[0].reg);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
    break;

  case 316:
#line 4110 "bfin-parse.y"
    {
	(yyval.r0).r0 = 0;
	}
    break;

  case 317:
#line 4114 "bfin-parse.y"
    {
	(yyval.r0).r0 = 1;
	}
    break;

  case 318:
#line 4118 "bfin-parse.y"
    {
	(yyval.r0).r0 = 2;
	}
    break;

  case 319:
#line 4122 "bfin-parse.y"
    {
	(yyval.r0).r0 = 3;
	}
    break;

  case 320:
#line 4129 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = (yyvsp[0].reg).regno;
	(yyval.modcodes).x0 = (yyvsp[-1].r0).r0;
	(yyval.modcodes).s0 = 0;
	}
    break;

  case 321:
#line 4135 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 0x18;
	(yyval.modcodes).x0 = (yyvsp[-1].r0).r0;
	(yyval.modcodes).s0 = 0;
	}
    break;

  case 322:
#line 4141 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = (yyvsp[-2].reg).regno;
	(yyval.modcodes).x0 = (yyvsp[-1].r0).r0;
	(yyval.modcodes).s0 = 1;
	}
    break;

  case 323:
#line 4147 "bfin-parse.y"
    {
	(yyval.modcodes).r0 = 0x18;
	(yyval.modcodes).x0 = (yyvsp[-1].r0).r0;
	(yyval.modcodes).s0 = 1;
	}
    break;

  case 324:
#line 4157 "bfin-parse.y"
    {
	Expr_Node_Value val;
	val.s_value = S_GET_NAME((yyvsp[0].symbol));
	(yyval.expr) = Expr_Node_Create (Expr_Node_Reloc, val, NULL, NULL);
	}
    break;

  case 325:
#line 4166 "bfin-parse.y"
    { (yyval.value) = BFD_RELOC_BFIN_GOT; }
    break;

  case 326:
#line 4168 "bfin-parse.y"
    { (yyval.value) = BFD_RELOC_BFIN_GOT17M4; }
    break;

  case 327:
#line 4170 "bfin-parse.y"
    { (yyval.value) = BFD_RELOC_BFIN_FUNCDESC_GOT17M4; }
    break;

  case 328:
#line 4174 "bfin-parse.y"
    {
	Expr_Node_Value val;
	val.i_value = (yyvsp[0].value);
	(yyval.expr) = Expr_Node_Create (Expr_Node_GOT_Reloc, val, (yyvsp[-2].expr), NULL);
	}
    break;

  case 329:
#line 4182 "bfin-parse.y"
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
    break;

  case 330:
#line 4186 "bfin-parse.y"
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
    break;

  case 331:
#line 4193 "bfin-parse.y"
    {
	(yyval.expr) = (yyvsp[-2].expr);
	}
    break;

  case 332:
#line 4199 "bfin-parse.y"
    {
	Expr_Node_Value val;
	val.i_value = (yyvsp[0].value);
	(yyval.expr) = Expr_Node_Create (Expr_Node_Constant, val, NULL, NULL);
	}
    break;

  case 333:
#line 4205 "bfin-parse.y"
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
    break;

  case 334:
#line 4209 "bfin-parse.y"
    {
	(yyval.expr) = (yyvsp[-1].expr);
	}
    break;

  case 335:
#line 4213 "bfin-parse.y"
    {
	(yyval.expr) = unary (Expr_Op_Type_COMP, (yyvsp[0].expr));
	}
    break;

  case 336:
#line 4217 "bfin-parse.y"
    {
	(yyval.expr) = unary (Expr_Op_Type_NEG, (yyvsp[0].expr));
	}
    break;

  case 337:
#line 4223 "bfin-parse.y"
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
    break;

  case 338:
#line 4229 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_Mult, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 339:
#line 4233 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_Div, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 340:
#line 4237 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_Mod, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 341:
#line 4241 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_Add, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 342:
#line 4245 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_Sub, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 343:
#line 4249 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_Lshift, (yyvsp[-2].expr), (yyvsp[0].expr));	
	}
    break;

  case 344:
#line 4253 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_Rshift, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 345:
#line 4257 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_BAND, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 346:
#line 4261 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_LOR, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 347:
#line 4265 "bfin-parse.y"
    {
	(yyval.expr) = binary (Expr_Op_Type_BOR, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
    break;

  case 348:
#line 4269 "bfin-parse.y"
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 7103 "bfin-parse.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 4275 "bfin-parse.y"


EXPR_T
mkexpr (int x, SYMBOL_T s)
{
  EXPR_T e = (EXPR_T) ALLOCATE (sizeof (struct expression_cell));
  e->value = x;
  EXPR_SYMBOL(e) = s;
  return e;
}

static int
value_match (Expr_Node *expr, int sz, int sign, int mul, int issigned)
{
  long umax = (1L << sz) - 1;
  long min = -1L << (sz - 1);
  long max = (1L << (sz - 1)) - 1;
	
  long v = EXPR_VALUE (expr);

  if ((v % mul) != 0)
    {
      error ("%s:%d: Value Error -- Must align to %d\n", __FILE__, __LINE__, mul); 
      return 0;
    }

  v /= mul;

  if (sign)
    v = -v;

  if (issigned)
    {
      if (v >= min && v <= max) return 1;

#ifdef DEBUG
      fprintf(stderr, "signed value %lx out of range\n", v * mul);
#endif
      return 0;
    }
  if (v <= umax && v >= 0) 
    return 1;
#ifdef DEBUG
  fprintf(stderr, "unsigned value %lx out of range\n", v * mul);
#endif
  return 0;
}

/* Return the expression structure that allows symbol operations.
   If the left and right children are constants, do the operation.  */
static Expr_Node *
binary (Expr_Op_Type op, Expr_Node *x, Expr_Node *y)
{
  Expr_Node_Value val;

  if (x->type == Expr_Node_Constant && y->type == Expr_Node_Constant)
    {
      switch (op)
	{
        case Expr_Op_Type_Add: 
	  x->value.i_value += y->value.i_value;
	  break;
        case Expr_Op_Type_Sub: 
	  x->value.i_value -= y->value.i_value;
	  break;
        case Expr_Op_Type_Mult: 
	  x->value.i_value *= y->value.i_value;
	  break;
        case Expr_Op_Type_Div: 
	  if (y->value.i_value == 0)
	    error ("Illegal Expression:  Division by zero.");
	  else
	    x->value.i_value /= y->value.i_value;
	  break;
        case Expr_Op_Type_Mod: 
	  x->value.i_value %= y->value.i_value;
	  break;
        case Expr_Op_Type_Lshift: 
	  x->value.i_value <<= y->value.i_value;
	  break;
        case Expr_Op_Type_Rshift: 
	  x->value.i_value >>= y->value.i_value;
	  break;
        case Expr_Op_Type_BAND: 
	  x->value.i_value &= y->value.i_value;
	  break;
        case Expr_Op_Type_BOR: 
	  x->value.i_value |= y->value.i_value;
	  break;
        case Expr_Op_Type_BXOR: 
	  x->value.i_value ^= y->value.i_value;
	  break;
        case Expr_Op_Type_LAND: 
	  x->value.i_value = x->value.i_value && y->value.i_value;
	  break;
        case Expr_Op_Type_LOR: 
	  x->value.i_value = x->value.i_value || y->value.i_value;
	  break;

	default:
	  error ("%s:%d: Internal compiler error\n", __FILE__, __LINE__); 
	}
      return x;
    }
  /* Canonicalize order to EXPR OP CONSTANT.  */
  if (x->type == Expr_Node_Constant)
    {
      Expr_Node *t = x;
      x = y;
      y = t;
    }
  /* Canonicalize subtraction of const to addition of negated const.  */
  if (op == Expr_Op_Type_Sub && y->type == Expr_Node_Constant)
    {
      op = Expr_Op_Type_Add;
      y->value.i_value = -y->value.i_value;
    }
  if (y->type == Expr_Node_Constant && x->type == Expr_Node_Binop
      && x->Right_Child->type == Expr_Node_Constant)
    {
      if (op == x->value.op_value && x->value.op_value == Expr_Op_Type_Add)
	{
	  x->Right_Child->value.i_value += y->value.i_value;
	  return x;
	}
    }

  /* Create a new expression structure.  */
  val.op_value = op;
  return Expr_Node_Create (Expr_Node_Binop, val, x, y);
}

static Expr_Node *
unary (Expr_Op_Type op, Expr_Node *x) 
{
  if (x->type == Expr_Node_Constant)
    {
      switch (op)
	{
	case Expr_Op_Type_NEG: 
	  x->value.i_value = -x->value.i_value;
	  break;
	case Expr_Op_Type_COMP:
	  x->value.i_value = ~x->value.i_value;
	  break;
	default:
	  error ("%s:%d: Internal compiler error\n", __FILE__, __LINE__); 
	}
      return x;
    }
  else
    {
      /* Create a new expression structure.  */
      Expr_Node_Value val;
      val.op_value = op;
      return Expr_Node_Create (Expr_Node_Unop, val, x, NULL);
    }
}

int debug_codeselection = 0;
static void
notethat (char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  if (debug_codeselection)
    {
      vfprintf (errorf, format, ap);
    }
  va_end (ap);
}

#ifdef TEST
main (int argc, char **argv)
{
  yyparse();
}
#endif


