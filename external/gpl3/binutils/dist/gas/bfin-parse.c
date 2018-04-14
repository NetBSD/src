/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 20 "./config/bfin-parse.y" /* yacc.c:339  */


#include "as.h"

#include "bfin-aux.h"  /* Opcode generating auxiliaries.  */
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


static int value_match (Expr_Node *, int, int, int, int);

extern FILE *errorf;
extern INSTR_T insn;

static Expr_Node *binary (Expr_Op_Type, Expr_Node *, Expr_Node *);
static Expr_Node *unary  (Expr_Op_Type, Expr_Node *);

static void notethat (const char *, ...);

extern char *yytext;
int yyerror (const char *);

/* Used to set SRCx fields to all 1s as described in the PRM.  */
static Register reg7 = {REG_R7, 0};

void error (const char *format, ...)
{
    va_list ap;
    static char buffer[2000];

    va_start (ap, format);
    vsprintf (buffer, format, ap);
    va_end (ap);

    as_bad ("%s", buffer);
}

int
yyerror (const char *msg)
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
in_range_p (Expr_Node *exp, int from, int to, unsigned int mask)
{
  int val = EXPR_VALUE (exp);
  if (exp->type != Expr_Node_Constant)
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
#define uimm8(x) EXPR_VALUE (x)
#define imm16(x) EXPR_VALUE (x)
#define uimm16s4(x) ((EXPR_VALUE (x)) >> 2)
#define uimm16(x) EXPR_VALUE (x)

/* Return true if a value is inside a range.  */
#define IN_RANGE(x, low, high) \
  (((EXPR_VALUE(x)) >= (low)) && (EXPR_VALUE(x)) <= ((high)))

/* Auxiliary functions.  */

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

  if (aa->w == ab->w && aa->P != ab->P)
    return yyerror ("Destination Dreg sizes (full or half) must match");

  if (aa->w && ab->w)
    {
      if (aa->P && (aa->dst.regno - ab->dst.regno) != 1)
	return yyerror ("Destination Dregs (full) must differ by one");
      if (!aa->P && aa->dst.regno != ab->dst.regno)
	return yyerror ("Destination Dregs (half) must match");
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

static int
is_store (INSTR_T x)
{
  if (!x)
    return 0;

  if ((x->value & 0xf000) == 0x8000)
    {
      int aop = ((x->value >> 9) & 0x3);
      int w = ((x->value >> 11) & 0x1);
      if (!w || aop == 3)
	return 0;
      return 1;
    }

  if (((x->value & 0xFF60) == 0x9E60) ||  /* dagMODim_0 */
      ((x->value & 0xFFF0) == 0x9F60))    /* dagMODik_0 */
    return 0;

  /* decode_dspLDST_0 */
  if ((x->value & 0xFC00) == 0x9C00)
    {
      int w = ((x->value >> 9) & 0x1);
      if (w)
	return 1;
    }

  return 0;
}

static INSTR_T
gen_multi_instr_1 (INSTR_T dsp32, INSTR_T dsp16_grp1, INSTR_T dsp16_grp2)
{
  int mask1 = dsp32 ? insn_regmask (dsp32->value, dsp32->next->value) : 0;
  int mask2 = dsp16_grp1 ? insn_regmask (dsp16_grp1->value, 0) : 0;
  int mask3 = dsp16_grp2 ? insn_regmask (dsp16_grp2->value, 0) : 0;

  if ((mask1 & mask2) || (mask1 & mask3) || (mask2 & mask3))
    yyerror ("resource conflict in multi-issue instruction");

  /* Anomaly 05000074 */
  if (ENABLE_AC_05000074
      && dsp32 != NULL && dsp16_grp1 != NULL
      && (dsp32->value & 0xf780) == 0xc680
      && ((dsp16_grp1->value & 0xfe40) == 0x9240
	  || (dsp16_grp1->value & 0xfe08) == 0xba08
	  || (dsp16_grp1->value & 0xfc00) == 0xbc00))
    yyerror ("anomaly 05000074 - Multi-Issue Instruction with \
dsp32shiftimm in slot1 and P-reg Store in slot2 Not Supported");

  if (is_store (dsp16_grp1) && is_store (dsp16_grp2))
    yyerror ("Only one instruction in multi-issue instruction can be a store");

  return bfin_gen_multi_instr (dsp32, dsp16_grp1, dsp16_grp2);
}


#line 490 "bfin-parse.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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
#line 444 "./config/bfin-parse.y" /* yacc.c:355  */

  INSTR_T instr;
  Expr_Node *expr;
  SYMBOL_T symbol;
  long value;
  Register reg;
  Macfunc macfunc;
  struct { int r0; int s0; int x0; int aop; } modcodes;
  struct { int r0; } r0;
  Opt_mode mod;

#line 888 "bfin-parse.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_BFIN_PARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 905 "bfin-parse.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  156
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1309

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  174
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  47
/* YYNRULES -- Number of rules.  */
#define YYNRULES  354
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1021

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   428

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
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
     165,   166,   167,   168,   169,   170,   171,   172,   173
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   645,   645,   646,   658,   660,   693,   720,   731,   735,
     773,   793,   798,   808,   818,   823,   828,   846,   864,   878,
     891,   907,   929,   947,   972,   994,   999,  1009,  1020,  1031,
    1045,  1060,  1076,  1092,  1103,  1117,  1143,  1161,  1166,  1172,
    1184,  1195,  1206,  1217,  1228,  1239,  1250,  1276,  1290,  1300,
    1345,  1364,  1375,  1386,  1397,  1408,  1419,  1435,  1452,  1468,
    1479,  1490,  1523,  1534,  1547,  1558,  1597,  1607,  1617,  1637,
    1647,  1657,  1668,  1682,  1693,  1706,  1716,  1728,  1743,  1754,
    1760,  1782,  1793,  1804,  1812,  1838,  1868,  1897,  1928,  1942,
    1953,  1967,  2001,  2019,  2044,  2056,  2074,  2085,  2096,  2107,
    2120,  2131,  2142,  2153,  2164,  2175,  2208,  2218,  2231,  2251,
    2262,  2273,  2286,  2299,  2310,  2321,  2332,  2343,  2353,  2364,
    2375,  2387,  2398,  2409,  2423,  2436,  2448,  2460,  2471,  2482,
    2493,  2505,  2517,  2528,  2539,  2550,  2560,  2566,  2572,  2578,
    2584,  2590,  2596,  2602,  2608,  2614,  2620,  2631,  2642,  2653,
    2664,  2675,  2686,  2697,  2703,  2717,  2728,  2739,  2750,  2761,
    2771,  2784,  2792,  2800,  2824,  2835,  2846,  2857,  2868,  2879,
    2891,  2904,  2913,  2924,  2935,  2947,  2958,  2969,  2980,  2994,
    3006,  3032,  3062,  3073,  3098,  3135,  3163,  3188,  3199,  3210,
    3221,  3247,  3266,  3280,  3304,  3316,  3335,  3381,  3418,  3434,
    3453,  3467,  3486,  3502,  3510,  3519,  3530,  3542,  3556,  3564,
    3574,  3586,  3597,  3607,  3618,  3629,  3635,  3640,  3645,  3651,
    3659,  3665,  3671,  3677,  3683,  3689,  3697,  3711,  3715,  3725,
    3729,  3734,  3739,  3744,  3751,  3755,  3762,  3766,  3771,  3776,
    3784,  3788,  3795,  3799,  3807,  3812,  3818,  3827,  3832,  3838,
    3844,  3850,  3859,  3862,  3866,  3873,  3876,  3880,  3887,  3892,
    3898,  3904,  3910,  3915,  3923,  3926,  3933,  3936,  3943,  3947,
    3951,  3955,  3962,  3965,  3972,  3977,  3984,  3991,  4003,  4007,
    4011,  4018,  4021,  4031,  4034,  4043,  4049,  4058,  4062,  4069,
    4073,  4077,  4081,  4088,  4092,  4099,  4107,  4115,  4123,  4131,
    4138,  4145,  4153,  4163,  4168,  4173,  4178,  4186,  4189,  4193,
    4202,  4209,  4216,  4223,  4238,  4244,  4257,  4270,  4288,  4295,
    4302,  4312,  4325,  4329,  4333,  4337,  4344,  4350,  4356,  4362,
    4372,  4381,  4383,  4385,  4389,  4397,  4401,  4408,  4414,  4420,
    4424,  4428,  4432,  4438,  4444,  4448,  4452,  4456,  4460,  4464,
    4468,  4472,  4476,  4480,  4484
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "BYTEOP16P", "BYTEOP16M", "BYTEOP1P",
  "BYTEOP2P", "BYTEOP3P", "BYTEUNPACK", "BYTEPACK", "PACK", "SAA",
  "ALIGN8", "ALIGN16", "ALIGN24", "VIT_MAX", "EXTRACT", "DEPOSIT",
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
  "any_gotrel", "got", "got_or_expr", "pltpc", "eterm", "expr", "expr_1", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
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
     425,   426,   427,   428
};
# endif

#define YYPACT_NINF -869

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-869)))

#define YYTABLE_NINF -214

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     862,  -869,   -96,   -14,  -869,   653,   618,  -869,  -869,   -22,
      -7,    20,    71,    85,  -869,  -869,  -869,  -869,  -869,  -869,
    -869,  -869,    58,   176,  -869,  -869,  -869,   -14,   -14,    48,
     -14,   167,   231,  -869,   327,   -14,   -14,   376,  -869,    53,
      56,    94,    96,   120,   126,   114,    64,   139,   144,   419,
     115,   171,   185,   199,   207,   230,  -869,   324,   250,   258,
      43,   358,    25,   419,  -869,   387,  -869,   -39,    13,   325,
     223,   245,   390,   300,  -869,  -869,   443,   -14,   -14,   -14,
    -869,  -869,  -869,  -869,  -869,   582,   152,   170,   178,   496,
     453,   203,   259,     7,  -869,  -869,  -869,    26,   -46,   448,
     455,   458,   464,   111,  -869,  -869,  -869,  -869,   -14,   463,
     -10,  -869,    -9,  -869,    32,  -869,  -869,   308,  -869,  -869,
     102,  -869,  -869,   479,   492,   497,  -869,   505,  -869,   508,
    -869,   523,  -869,  -869,  -869,   526,   541,   561,  -869,   530,
     567,   581,   586,   602,   611,   625,  -869,  -869,   549,   632,
      57,   589,   221,   172,   637,   614,  -869,  1008,  -869,  -869,
    -869,   365,     4,  -869,   584,   394,   365,   365,   365,   498,
     365,    -6,   -14,  -869,  -869,   507,  -869,  -869,   301,   510,
     519,  -869,  -869,   524,   -14,   -14,   -14,   -14,   -14,   -14,
     -14,   -14,   -14,   -14,  -869,  -869,  -869,  -869,  -869,  -869,
     548,   554,   563,   576,   583,  -869,  -869,  -869,   587,   592,
     597,   601,  -869,   598,   673,   -19,   279,   293,  -869,  -869,
     663,   698,   719,   723,   728,   594,   599,    63,   733,   691,
     603,   604,   300,   605,  -869,  -869,  -869,   606,  -869,   225,
     607,   271,  -869,   608,  -869,  -869,  -869,  -869,  -869,  -869,
     609,   610,   739,   208,   -25,   676,   538,   740,   741,   615,
     394,  -869,   300,  -869,   617,   680,   620,   709,   612,   621,
     710,   626,   627,   -41,    -3,    14,    17,   628,   281,   349,
    -869,   631,   633,   634,   636,   638,   639,   640,   641,   690,
     -14,    62,   767,   -14,  -869,  -869,  -869,   769,   -14,   643,
     644,  -869,    -8,   507,  -869,   773,   764,   646,   647,   648,
     651,   365,   652,   -14,   -14,   -14,   675,  -869,   666,  -869,
     134,   166,   276,   -14,  -869,   630,   642,  -869,   483,   368,
     368,  -869,  -869,   532,   532,   780,   786,   787,   788,   779,
     790,   791,   792,   793,   794,   795,   659,  -869,  -869,  -869,
    -869,   -14,   -14,   -14,   797,   798,   318,  -869,   799,  -869,
    -869,   662,   664,   667,   669,   670,   671,   806,   807,   765,
     340,   390,   390,   245,   677,   384,   365,   809,   811,   682,
     493,  -869,   706,   297,   317,   319,   815,   365,   365,   365,
     816,   817,   226,  -869,  -869,  -869,  -869,   707,   818,    37,
     -14,   -14,   -14,   824,   812,   688,   692,   823,   245,   693,
     694,   -14,   827,  -869,   828,  -869,  -869,   830,   831,   833,
     685,  -869,  -869,  -869,  -869,  -869,  -869,   -14,   697,   842,
     -14,   704,   -14,   -14,   -14,   844,   -14,   -14,   -14,  -869,
     845,   712,   774,   -14,   714,   182,   715,   716,   785,  -869,
    1008,  -869,  -869,   724,  -869,   365,   365,   849,   853,   766,
     100,  -869,  -869,  -869,   729,   763,   796,  -869,   800,  -869,
     829,   832,   300,   768,   771,   776,   777,   770,   775,   781,
     783,   784,  -869,  -869,  -869,   903,   662,   664,   662,   -58,
     -15,   772,   782,   789,    33,  -869,   802,  -869,   902,   907,
     910,   472,   281,   445,   924,  -869,   801,  -869,   925,   -14,
     803,   804,   808,   813,   926,   805,   810,   819,   820,   820,
    -869,  -869,   820,   820,   821,  -869,  -869,  -869,   826,   825,
     834,   835,   836,   837,   838,   839,   840,  -869,   840,   841,
     843,   917,   918,   562,   859,  -869,   919,   860,   864,   861,
     865,   868,   869,  -869,   846,   863,   870,   872,   866,   908,
     909,   911,   914,   912,   913,   915,  -869,   857,   931,   916,
     867,   934,   871,   875,   876,   944,   920,   -14,   891,   921,
     922,  -869,  -869,   365,  -869,  -869,   927,  -869,   928,   929,
       5,    10,  -869,   964,   -14,   -14,   -14,   968,   959,   970,
     961,   981,   933,  -869,  -869,  -869,  1050,   119,  -869,  1052,
     559,  -869,  -869,  -869,  1054,   930,   211,   247,   932,  -869,
     664,   662,  -869,  -869,   -14,   923,  1056,   -14,   935,   936,
    -869,   937,   938,  -869,   941,  -869,  -869,  1057,  1058,  1060,
     989,  -869,  -869,  -869,   953,  -869,  -869,  -869,  -869,   -14,
     -14,   940,  1059,  1061,  -869,   546,   365,   365,   967,  -869,
    -869,  1063,  -869,  -869,   840,  1070,   942,  -869,  1003,  1082,
     -14,  -869,  -869,  -869,  -869,  1011,  1084,  1014,  1015,   278,
    -869,  -869,  -869,   365,  -869,  -869,  -869,   952,  -869,   984,
     216,   956,   954,  1091,  1093,  -869,  -869,   287,   365,   365,
     962,   365,  -869,  -869,   365,  -869,   365,   965,   969,   971,
     972,   973,   974,   975,   976,   977,   -14,  1035,  -869,  -869,
    -869,   978,  1036,   979,   980,  1045,  -869,  1001,  -869,  1019,
    -869,  -869,  -869,  -869,   982,   598,   983,   985,   598,  1055,
    -869,   407,  -869,  1051,   990,   991,   390,   995,  1004,  1005,
     574,  -869,  1006,  1007,  1016,  1017,  1012,  1018,  1020,  1021,
    -869,  1022,  -869,   390,  1075,  -869,  1151,  -869,  1144,  1155,
    -869,  -869,  1023,  -869,  1024,  1025,  1026,  1158,  1164,   -14,
    1165,  -869,  -869,  -869,  1166,  -869,  -869,  -869,  1167,   365,
     -14,  1168,  1170,  1171,  -869,  -869,   940,   598,  1030,  1037,
    1172,  -869,  1174,  -869,  -869,  1169,  1040,  1041,   598,  -869,
     598,   598,  -869,   -14,  -869,  -869,  -869,  -869,   365,  -869,
     664,   300,  -869,  -869,  -869,  1042,  1043,   664,  -869,  -869,
    -869,   372,  1180,  -869,  1135,  -869,   300,  1182,  -869,  -869,
    -869,   940,  -869,  1183,  1184,  1053,  1048,  1062,  1128,  1065,
    1064,  1066,  1068,  1067,  1071,  1072,  -869,  -869,  1081,  -869,
     596,   635,  1145,  -869,  -869,  -869,  -869,  -869,  -869,  1147,
    -869,  -869,  -869,  -869,  -869,  1073,  1076,  1074,  1179,  -869,
    1126,  -869,  1077,  1078,   -14,   619,  1121,   -14,  -869,  1094,
    1079,   -14,   -14,   -14,  1083,  1195,  1196,  1190,   365,  -869,
    1200,  -869,  1162,   -14,   -14,   -14,  1079,  -869,  -869,  -869,
    -869,  1085,   954,  1086,  1087,  1102,  -869,  1088,  1089,  1090,
    -869,  1080,   843,  -869,   843,  1092,  1218,  -869,  1095,  1097,
    -869,  -869,  -869,  -869,  -869,  1096,  1098,  1099,  1100,   350,
    -869,  -869,  -869,  -869,  1101,  1215,  1220,  -869,   595,  -869,
      84,  -869,   591,  -869,  -869,  -869,   312,   375,  1208,  1105,
    1106,   378,   402,   403,   418,   426,   460,   476,   481,   616,
    -869,   119,  -869,  1107,   -14,   -14,  1119,  -869,  1123,  -869,
    1120,  -869,  1130,  -869,  1131,  -869,  1133,  -869,  1134,  -869,
    1136,  -869,  1110,  1112,  1188,  1113,  1114,  1115,  1116,  1117,
    1118,  1122,  1124,  1125,  1127,  -869,  -869,  1245,  1079,  1079,
    -869,  -869,  -869,  -869,  -869,  -869,  -869,  -869,  -869,  -869,
    -869
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     7,     0,     0,   204,     0,     0,   227,   228,     0,
       0,     0,     0,     0,   136,   138,   137,   139,   140,   141,
     221,   142,     0,     0,   143,   144,   145,     0,     0,     0,
       0,     0,     0,    11,     0,     0,     0,     0,   215,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   220,   216,     0,     0,
       0,     0,     0,     0,     8,     0,     3,     0,     0,     0,
       0,     0,     0,   229,   314,    79,     0,     0,     0,     0,
     330,   338,   339,   354,   203,   343,     0,     0,     0,     0,
       0,     0,     0,   322,   323,   325,   324,     0,     0,     0,
       0,     0,     0,     0,   147,   146,   152,   153,     0,     0,
     338,   212,   338,   214,     0,   155,   156,   339,   158,   157,
       0,   160,   159,     0,     0,     0,   174,     0,   172,     0,
     176,     0,   178,   226,   225,     0,     0,     0,   322,     0,
       0,     0,     0,     0,     0,     0,   218,   217,     0,     0,
       0,     0,     0,   307,     0,     0,     1,     0,     4,   310,
     311,   312,     0,    45,     0,     0,     0,     0,     0,     0,
       0,    44,     0,   318,    48,   281,   320,   319,     0,     9,
       0,   341,   342,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   167,   170,   168,   169,   165,   166,
       0,     0,     0,     0,     0,   278,   279,   280,     0,     0,
       0,    80,    82,   252,     0,   252,     0,     0,   287,   288,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   313,
       0,     0,   229,   255,    62,    58,    56,    60,    61,    81,
       0,     0,    83,     0,   327,   326,    26,    14,    27,    15,
       0,     0,     0,     0,    50,     0,     0,     0,     0,     0,
       0,   317,   229,    47,     0,   208,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   307,   307,
     329,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   294,   293,   309,   308,     0,     0,
       0,   328,     0,   281,   202,     0,     0,    37,    25,     0,
       0,     0,     0,     0,     0,     0,     0,    39,     0,    55,
       0,     0,     0,     0,   340,   351,   353,   346,   352,   348,
     347,   344,   345,   349,   350,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   293,   289,   290,   291,
     292,     0,     0,     0,     0,     0,     0,    52,     0,    46,
     164,   258,   264,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   307,     0,     0,     0,    85,
       0,    49,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   109,   119,   120,   118,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      84,     0,     0,   148,     0,   337,   149,     0,     0,     0,
       0,   173,   171,   175,   177,   154,   308,     0,     0,   308,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   219,
       0,   134,     0,     0,     0,     0,     0,     0,     0,   285,
       0,     6,    59,     0,   321,     0,     0,     0,     0,     0,
       0,    90,   104,    99,     0,     0,     0,   233,     0,   232,
       0,     0,   229,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    78,    66,    67,     0,   258,   264,   258,   242,
     244,     0,     0,     0,     0,   163,     0,    24,     0,     0,
       0,     0,   307,   307,     0,   312,     0,   315,   308,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   283,   283,
      73,    74,   283,   283,     0,    75,    69,    70,     0,     0,
       0,     0,     0,     0,     0,     0,   266,   106,   266,     0,
     244,     0,     0,   307,     0,   316,     0,     0,   209,     0,
       0,     0,     0,   286,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   131,     0,     0,   132,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   100,    88,     0,   114,   116,    40,   282,     0,     0,
       0,     0,    10,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    91,   105,   108,     0,   236,    51,     0,
       0,    35,   254,   253,     0,     0,     0,     0,     0,   103,
     264,   258,   115,   117,     0,     0,   308,     0,     0,     0,
      12,     0,   339,   335,     0,   336,   197,     0,     0,     0,
       0,   256,   257,    57,     0,    76,    77,    71,    72,     0,
       0,     0,     0,     0,    41,     0,     0,     0,     0,    92,
     107,     0,    38,   101,   266,   308,     0,    13,     0,     0,
       0,   151,   150,   162,   161,     0,     0,     0,     0,     0,
     127,   125,   126,     0,   224,   223,   222,     0,   130,     0,
       0,     0,     0,     0,     0,   190,     5,     0,     0,     0,
       0,     0,   230,   231,     0,   313,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   237,   238,
     239,     0,     0,     0,     0,     0,   259,     0,   260,     0,
     261,   265,   102,    93,     0,   252,     0,     0,   252,     0,
     195,     0,   196,     0,     0,     0,     0,     0,     0,     0,
       0,   121,     0,     0,     0,     0,     0,     0,     0,     0,
      89,     0,   186,     0,   205,   210,     0,   179,     0,     0,
     182,   183,     0,   135,     0,     0,     0,     0,     0,     0,
       0,   201,   191,   184,     0,   199,    54,    53,     0,     0,
       0,     0,     0,     0,    33,   110,     0,   252,    96,     0,
       0,   243,     0,   245,   246,     0,     0,     0,   252,   194,
     252,   252,   187,     0,   331,   332,   333,   334,     0,    28,
     264,   229,   284,   129,   128,     0,     0,   264,    95,    42,
      43,     0,     0,   267,     0,   189,   229,     0,   180,   192,
     181,     0,   133,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   122,    98,     0,    68,
       0,     0,     0,   263,   262,   193,   188,   185,    65,     0,
      36,    87,   234,   235,    94,     0,     0,     0,     0,    86,
     206,   123,     0,     0,     0,     0,     0,     0,   124,     0,
     272,     0,     0,     0,     0,     0,     0,     0,     0,   112,
       0,   111,     0,     0,     0,     0,   272,   268,   271,   270,
     269,     0,     0,     0,     0,     0,    63,     0,     0,     0,
      97,   247,   244,    20,   244,     0,     0,   207,     0,     0,
      18,    19,   200,   198,    64,     0,    30,     0,     0,   236,
      23,    22,    21,   113,     0,     0,     0,   273,     0,    29,
       0,    31,     0,    32,   240,   241,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     249,   236,   248,     0,     0,     0,     0,   275,     0,   274,
       0,   296,     0,   298,     0,   297,     0,   295,     0,   303,
       0,   304,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   251,   250,     0,   272,   272,
     276,   277,   300,   302,   301,   299,   305,   306,    34,    16,
      17
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -869,  -869,  -869,  -133,    41,  -216,  -733,  -868,   313,  -869,
    -509,  -869,  -198,  -869,  -458,  -460,  -515,  -869,  -804,  -869,
    -869,   986,    23,  -869,   -31,  -869,   421,  -205,  -869,  -869,
    -253,     2,    22,  -171,   987,  -206,   -56,    46,  -869,   -17,
    -869,  -869,  -869,  1247,  -869,   -27,     0
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    65,    66,    67,   370,   179,   751,   721,   957,   608,
     611,   940,   357,   381,   495,   497,   659,   911,   916,   949,
     230,   319,   645,    69,   126,   231,   354,   298,   951,   953,
     299,   371,   372,    72,    73,    74,   177,    98,    75,    82,
     817,   633,   634,   118,    83,    84,    85
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     106,   107,    70,   109,   111,   113,   355,   115,   116,   119,
     122,   128,   130,   132,   173,   176,   379,   359,   134,   117,
     117,   374,    71,   660,   302,   428,   431,   604,   603,   304,
     605,   662,   239,   232,     7,     8,     7,     8,   157,     7,
       8,    68,   420,   174,   294,   295,   410,   262,    77,   398,
     153,   404,   306,   242,   409,    78,   373,   266,   267,   195,
     197,   199,   233,   856,   236,   238,    76,  -211,  -213,   450,
     150,   956,   172,   427,   430,    99,   263,   181,   182,   183,
     420,   264,   289,   104,   313,   314,   315,   442,   369,   408,
     100,   159,     7,     8,    77,   139,   244,   420,   147,   606,
     420,    78,   930,   993,   607,   534,   151,   154,   881,   155,
     159,   171,   175,   290,   183,   160,   161,   101,   443,   245,
     183,   158,   510,   421,   535,    77,   250,   269,   270,   251,
     229,   252,    78,   253,   241,   584,   254,   397,   255,   133,
       7,     8,   609,   356,   261,   317,   256,   610,    79,   760,
    -211,  -213,   451,    80,    81,   240,   316,   615,   616,    70,
     732,   422,    77,   733,    77,   182,   305,   704,   102,    78,
     509,    78,   706,    77,   243,   617,    77,   194,   423,    71,
      78,   424,   103,    78,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,    79,   196,   257,   258,    68,    80,
      81,   105,   303,   198,  1019,  1020,   307,   308,   309,   310,
     108,   312,   963,   964,    77,   123,   181,   182,   124,   775,
     776,    78,   965,   966,   777,    79,   135,   183,   235,    77,
      80,    81,    77,   394,   259,   778,    78,     7,     8,    78,
      77,   294,   295,   395,   718,   719,   720,    78,   163,   625,
     628,   530,     7,     8,   296,   297,   592,   125,   164,   127,
     183,   531,    79,   441,    79,    77,   445,    80,    81,    80,
      81,   447,    78,   260,     7,     8,    79,   140,    80,    81,
     164,    80,    81,   129,   237,    77,   461,   462,   463,   131,
     666,   466,   165,    77,   396,   467,   473,   624,   627,   166,
      78,   470,   136,   770,   360,     7,     8,   137,   167,   168,
     169,   471,   170,   771,    79,   173,   176,   576,   361,    80,
      81,    77,   518,   468,   486,   487,   488,   469,    78,    79,
     383,   384,    79,   141,    80,   110,   385,    80,    81,   571,
      79,    77,   520,   572,   522,    80,    81,   142,    78,   146,
     294,   295,   459,     7,     8,    77,   519,   521,   523,    77,
     870,   143,    78,   296,   426,    79,    78,   874,   727,   144,
      80,    81,   728,   536,   537,   538,   387,   388,   292,    77,
     293,    77,   389,   152,   547,    79,    78,   156,    78,    77,
      80,    81,   145,    79,     7,     8,    78,   875,    80,   112,
     554,     7,     8,   557,   729,   559,   560,   561,   730,   563,
     564,   565,   148,   941,   506,   942,   569,   511,   294,   295,
     149,    79,   162,     7,     8,   164,    80,    81,   525,   526,
     527,   296,   429,   585,   186,   320,   321,   159,    77,   190,
     191,    79,   160,   505,   784,    78,    80,    81,   785,   544,
     491,   492,    70,   294,   295,    79,    77,   579,   580,    79,
      80,    81,   178,    78,    80,    81,   296,   508,   180,   969,
     954,   955,    71,   970,   623,   718,   719,   720,   234,    79,
     268,    79,   635,   246,    80,    81,    80,    81,   265,   114,
     247,    68,   632,   248,    80,    81,   578,   578,   374,   249,
     409,   200,   201,   202,   271,   203,   204,   622,   205,   206,
     207,   208,   209,   210,   294,   295,   138,   272,    94,    95,
      96,   211,   273,   212,   213,     7,     8,   296,   626,   214,
     274,   215,   971,   275,    77,   976,   972,   809,   120,   977,
     812,    78,   646,    80,    81,   647,   648,   184,   276,   186,
     697,   277,   188,   189,   190,   191,    79,   280,   216,   978,
     980,    80,    81,   979,   981,   217,   278,   708,   709,   710,
     218,   219,   220,   192,   193,   982,   814,   815,   816,   983,
     221,   222,   223,   984,   287,   224,   279,   985,   184,   185,
     186,   187,   281,   188,   189,   190,   191,   734,   186,   857,
     737,   188,   189,   190,   191,   871,   282,   294,   295,   306,
     865,   283,   866,   867,   192,   193,   291,   986,   225,   226,
     879,   987,   748,   749,   700,   515,   516,   284,   400,   401,
     402,   705,   261,   988,    79,   403,   285,   989,   990,    80,
      81,   301,   991,   765,   296,   665,   184,   185,   186,   187,
     286,   188,   189,   190,   191,   306,   896,   288,   227,   228,
     311,   781,   300,    80,    81,   343,   344,   322,   345,   318,
     294,   346,   192,   193,   347,   348,   349,   350,   323,   347,
     348,   349,   350,   723,   724,   324,   754,   755,   362,   799,
     821,   351,   352,   353,   825,   826,   186,   756,   757,   188,
     189,   190,   191,   789,   294,   295,   184,   836,   186,   187,
     335,   188,   189,   190,   191,    93,   336,    94,    95,    96,
     192,   193,    97,   363,   772,   337,   907,   908,   909,   910,
     961,   962,   192,   193,   967,   968,   954,   955,   338,   786,
     787,    86,   578,   358,   364,   339,    87,    88,   365,   340,
      89,    90,   847,   366,   341,    91,    92,   367,   375,   342,
     356,   376,   368,   852,   393,   377,   378,   380,   382,   386,
     390,   391,   392,   399,   411,   405,   406,   412,   407,   414,
     417,   413,   416,   418,   419,   415,   868,   440,   432,   425,
     433,   434,   444,   435,   446,   436,   437,   438,   453,   454,
     464,   465,   439,   455,   456,   474,   457,   448,   449,   458,
     460,   475,   476,   477,   478,   479,   480,   481,   482,   483,
     484,   485,   489,   490,   494,   498,   496,   499,   500,   501,
     851,   502,   503,   493,   512,   504,   513,   517,   507,   514,
     524,   528,   529,   533,   532,   539,   541,   540,   543,   553,
     542,   546,   548,   549,   545,   550,   551,   906,   552,   869,
     913,   555,    -2,     1,   917,   918,   919,   556,   558,   562,
     566,   568,   876,     2,   567,   570,   927,   928,   929,   573,
     574,   932,   575,   577,   581,     3,     4,     5,   582,     6,
     586,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,   583,   587,   593,   590,   597,   602,   591,
     594,   588,   598,   612,   589,   595,   596,   619,   599,   924,
     600,   601,   620,   613,   618,   621,   614,   995,   996,   629,
     631,   640,   663,   664,   668,   670,   688,    39,    40,   691,
     676,   637,   630,   679,   690,   638,   641,   636,   692,   695,
     639,   642,   693,   694,    41,    42,    43,    44,   649,    45,
     643,    46,   644,   650,    47,    48,   651,   687,   159,   707,
     698,   652,   653,   711,   712,   713,   714,   654,   699,   655,
     656,   657,   658,   661,    49,   610,   715,    50,    51,    52,
     675,    53,    54,    55,    56,    57,    58,    59,    60,     2,
     667,   669,   671,   716,    61,    62,   672,    63,    64,   673,
     674,     3,     4,     5,   677,     6,   678,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,   680,
     681,   683,   682,   684,   685,   717,   686,   722,   689,   725,
     696,   736,   743,   744,   701,   745,   746,   735,   747,   702,
     703,   726,   758,   731,   752,   761,   753,   739,   759,   738,
     763,   740,   750,    39,    40,   742,   762,   764,   766,   767,
     741,   768,   769,   773,   774,   779,   782,   780,   783,   788,
      41,    42,    43,    44,   790,    45,   791,    46,   792,   793,
      47,    48,   800,   802,   794,   795,   796,   797,   798,   801,
     803,   804,   805,   806,   807,   813,   808,   810,   818,   811,
      49,   819,   820,    50,    51,    52,   822,    53,    54,    55,
      56,    57,    58,    59,    60,   823,   824,   827,   828,   831,
      61,    62,   837,    63,    64,   832,   838,   829,   830,   839,
     840,   833,   834,   845,   841,   842,   835,   843,   844,   846,
     848,   849,   858,   853,   850,   854,   855,   860,   859,   861,
     862,   863,   864,   872,   873,   877,   878,   880,   882,   883,
     885,   420,   884,   894,   902,   897,   898,   903,   912,   914,
     921,   922,   923,   886,   887,   888,   891,   925,   889,   890,
     892,   893,   926,   900,   899,   901,   904,   905,   935,   944,
     959,   915,   939,   973,   920,   960,   931,   933,   934,   936,
     937,   938,   945,   943,   946,   997,   999,   947,   998,  1007,
     948,   950,   952,   958,   974,   975,  1000,  1001,   994,  1002,
    1003,  1005,  1004,  1006,  1008,  1009,  1010,  1011,  1012,  1013,
    1018,   895,   992,  1014,   121,  1015,  1016,     0,  1017,   452,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   472
};

static const yytype_int16 yycheck[] =
{
      27,    28,     0,    30,    31,    32,   211,    34,    35,    36,
      37,    42,    43,    44,    70,    71,   232,   215,    45,    36,
      37,   227,     0,   538,   157,   278,   279,   487,   486,    25,
     488,   540,    25,    89,    29,    30,    29,    30,    77,    29,
      30,     0,    83,    70,    69,    70,   262,   103,    62,   254,
      25,   256,    71,    27,   260,    69,   227,    25,    26,    86,
      87,    88,    89,   796,    91,    92,   162,    77,    77,    77,
      27,   939,    70,   278,   279,    97,   103,    77,    78,    79,
      83,   108,    25,    25,    90,    91,    92,    25,    25,   260,
      97,    97,    29,    30,    62,    49,   142,    83,    57,   157,
      83,    69,   906,   971,   162,    68,    63,    82,   841,    63,
      97,    70,    71,    56,   114,   102,   103,    97,    56,   165,
     120,   160,   375,   164,    87,    62,    15,    25,    26,    18,
      89,    20,    69,    22,    93,    35,    25,   162,    27,    25,
      29,    30,   157,   162,   103,   172,    35,   162,   162,   664,
     160,   160,   160,   167,   168,   148,   162,   124,   125,   157,
     620,   164,    62,   621,    62,   165,   162,   162,    97,    69,
     375,    69,   162,    62,   148,   142,    62,    25,   164,   157,
      69,   164,    97,    69,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   162,    25,    85,    86,   157,   167,
     168,    25,   161,    25,  1008,  1009,   165,   166,   167,   168,
     162,   170,   128,   129,    62,   162,   216,   217,   162,     3,
       4,    69,   138,   139,     8,   162,   162,   227,    25,    62,
     167,   168,    62,    25,   123,    19,    69,    29,    30,    69,
      62,    69,    70,    35,   125,   126,   127,    69,    25,   502,
     503,    25,    29,    30,    82,    83,   472,   163,    35,   163,
     260,    35,   162,   290,   162,    62,   293,   167,   168,   167,
     168,   298,    69,   162,    29,    30,   162,   162,   167,   168,
      35,   167,   168,   163,    25,    62,   313,   314,   315,   163,
     543,   157,    69,    62,   253,   161,   323,   502,   503,    76,
      69,    25,   163,    25,    25,    29,    30,   163,    85,    86,
      87,    35,    89,    35,   162,   371,   372,   450,    25,   167,
     168,    62,    25,   157,   351,   352,   353,   161,    69,   162,
     105,   106,   162,   162,   167,   168,   111,   167,   168,   157,
     162,    62,    25,   161,    25,   167,   168,   162,    69,    25,
      69,    70,   311,    29,    30,    62,   383,   384,   385,    62,
     820,   162,    69,    82,    83,   162,    69,   827,   157,   162,
     167,   168,   161,   400,   401,   402,   105,   106,   157,    62,
     159,    62,   111,    25,   411,   162,    69,     0,    69,    62,
     167,   168,   162,   162,    29,    30,    69,    25,   167,   168,
     427,    29,    30,   430,   157,   432,   433,   434,   161,   436,
     437,   438,   162,   922,   373,   924,   443,   376,    69,    70,
     162,   162,    97,    29,    30,    35,   167,   168,   387,   388,
     389,    82,    83,   460,    66,   134,   135,    97,    62,    71,
      72,   162,   102,   103,   157,    69,   167,   168,   161,   408,
     132,   133,   450,    69,    70,   162,    62,   455,   456,   162,
     167,   168,   162,    69,   167,   168,    82,    83,    25,   157,
     120,   121,   450,   161,   501,   125,   126,   127,    25,   162,
     172,   162,   509,    35,   167,   168,   167,   168,    25,   162,
      35,   450,   509,    35,   167,   168,   455,   456,   704,    35,
     706,     5,     6,     7,    25,     9,    10,    35,    12,    13,
      14,    15,    16,    17,    69,    70,    97,    25,    99,   100,
     101,    25,    25,    27,    28,    29,    30,    82,    83,    33,
      25,    35,   157,    25,    62,   157,   161,   735,   162,   161,
     738,    69,   519,   167,   168,   522,   523,    64,    25,    66,
     577,    25,    69,    70,    71,    72,   162,    27,    62,   157,
     157,   167,   168,   161,   161,    69,    25,   594,   595,   596,
      74,    75,    76,    90,    91,   157,   169,   170,   171,   161,
      84,    85,    86,   157,    35,    89,    25,   161,    64,    65,
      66,    67,    25,    69,    70,    71,    72,   624,    66,   797,
     627,    69,    70,    71,    72,   821,    25,    69,    70,    71,
     808,    25,   810,   811,    90,    91,    27,   157,   122,   123,
     836,   161,   649,   650,   583,   132,   133,    25,    90,    91,
      92,   590,   591,   157,   162,    97,    25,   161,   157,   167,
     168,    27,   161,   670,    82,    83,    64,    65,    66,    67,
      25,    69,    70,    71,    72,    71,   861,    25,   162,   163,
     162,   692,    25,   167,   168,    64,    65,   157,    67,   162,
      69,    70,    90,    91,    78,    79,    80,    81,   159,    78,
      79,    80,    81,   124,   125,   161,   140,   141,    25,   716,
     746,    90,    91,    92,   120,   121,    66,   656,   657,    69,
      70,    71,    72,   701,    69,    70,    64,   763,    66,    67,
     162,    69,    70,    71,    72,    97,   162,    99,   100,   101,
      90,    91,   104,    25,   683,   162,   107,   108,   109,   110,
     135,   136,    90,    91,   143,   144,   120,   121,   162,   698,
     699,    88,   701,    70,    25,   162,    93,    94,    25,   162,
      97,    98,   779,    25,   162,   102,   103,   163,    25,   162,
     162,    70,   163,   790,    25,   162,   162,   162,   162,   162,
     162,   162,   162,    97,   157,    35,    35,    97,   163,    70,
      70,   161,   161,   157,   157,   173,   813,    97,   157,   161,
     157,   157,    25,   157,    25,   157,   157,   157,    25,    35,
     125,   135,   161,   157,   157,    25,   158,   164,   164,   158,
     158,    25,    25,    25,    35,    25,    25,    25,    25,    25,
      25,   162,    25,    25,   162,   158,   162,   158,   158,   158,
     789,    25,    25,    34,    25,    70,    25,   131,   161,   157,
      25,    25,    25,    25,   137,    21,   158,    35,    25,   164,
     158,   157,    25,    25,   161,    25,    25,   884,    25,   818,
     887,   164,     0,     1,   891,   892,   893,    25,   164,    25,
      25,    97,   831,    11,   162,   161,   903,   904,   905,   164,
     164,   912,    97,   159,    35,    23,    24,    25,    35,    27,
     161,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,   157,   161,   157,    97,   157,    25,    97,
     159,   135,   157,   161,   134,   159,   159,    35,   157,   898,
     157,   157,    35,   161,   142,    35,   157,   974,   975,    25,
      25,    25,    35,    35,    35,    91,    25,    95,    96,    25,
      97,   157,   161,    97,    97,   157,   161,   164,    97,    25,
     157,   161,    97,    97,   112,   113,   114,   115,   157,   117,
     161,   119,   162,   157,   122,   123,   161,   130,    97,    25,
      69,   157,   157,    25,    35,    25,    35,   161,    76,   162,
     162,   162,   162,   162,   142,   162,    25,   145,   146,   147,
     164,   149,   150,   151,   152,   153,   154,   155,   156,    11,
     161,   161,   161,    90,   162,   163,   161,   165,   166,   161,
     161,    23,    24,    25,   164,    27,   164,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,   161,
     161,   157,   161,   161,   161,    25,   161,    25,   162,    25,
     160,    25,    25,    25,   157,    25,    97,   164,   135,   161,
     161,   161,   125,   161,    35,    25,    35,   161,    35,   164,
      97,   164,   162,    95,    96,   164,   164,    25,    97,    25,
     172,    97,    97,   161,   130,   159,    25,   163,    25,   157,
     112,   113,   114,   115,   159,   117,   157,   119,   157,   157,
     122,   123,    97,    97,   161,   161,   161,   161,   161,   161,
     161,   161,    97,   142,   125,    90,   164,   164,    97,   164,
     142,   161,   161,   145,   146,   147,   161,   149,   150,   151,
     152,   153,   154,   155,   156,   161,   161,   161,   161,   157,
     162,   163,    97,   165,   166,   157,    25,   161,   161,    35,
      25,   161,   161,    25,   161,   161,   164,   162,   162,    25,
      25,    25,   162,    25,    27,    25,    25,    25,   161,    25,
      31,   161,   161,   161,   161,    25,    71,    25,    25,    25,
     162,    83,   159,   132,    35,    70,    69,    91,    97,   125,
      25,    25,    32,   161,   159,   161,   159,    27,   162,   161,
     159,   159,    70,   157,   161,   161,   159,   159,   136,    21,
      25,   162,   162,    35,   161,    25,   161,   161,   161,   161,
     161,   161,   157,   161,   157,   136,   136,   161,   135,    71,
     162,   162,   162,   162,   159,   159,   136,   136,   161,   136,
     136,   161,   136,   161,   161,   161,   161,   161,   161,   161,
      35,   860,   969,   161,    37,   161,   161,    -1,   161,   303,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   322
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    11,    23,    24,    25,    27,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    95,
      96,   112,   113,   114,   115,   117,   119,   122,   123,   142,
     145,   146,   147,   149,   150,   151,   152,   153,   154,   155,
     156,   162,   163,   165,   166,   175,   176,   177,   178,   197,
     205,   206,   207,   208,   209,   212,   162,    62,    69,   162,
     167,   168,   213,   218,   219,   220,    88,    93,    94,    97,
      98,   102,   103,    97,    99,   100,   101,   104,   211,    97,
      97,    97,    97,    97,    25,    25,   219,   219,   162,   219,
     168,   219,   168,   219,   162,   219,   219,   213,   217,   219,
     162,   217,   219,   162,   162,   163,   198,   163,   198,   163,
     198,   163,   198,    25,   219,   162,   163,   163,    97,   211,
     162,   162,   162,   162,   162,   162,    25,   178,   162,   162,
      27,    63,    25,    25,    82,   211,     0,    77,   160,    97,
     102,   103,    97,    25,    35,    69,    76,    85,    86,    87,
      89,   178,   205,   210,   219,   178,   210,   210,   162,   179,
      25,   220,   220,   220,    64,    65,    66,    67,    69,    70,
      71,    72,    90,    91,    25,   219,    25,   219,    25,   219,
       5,     6,     7,     9,    10,    12,    13,    14,    15,    16,
      17,    25,    27,    28,    33,    35,    62,    69,    74,    75,
      76,    84,    85,    86,    89,   122,   123,   162,   163,   178,
     194,   199,   210,   219,    25,    25,   219,    25,   219,    25,
     148,   178,    27,   148,   142,   165,    35,    35,    35,    35,
      15,    18,    20,    22,    25,    27,    35,    85,    86,   123,
     162,   178,   210,   219,   219,    25,    25,    26,   172,    25,
      26,    25,    25,    25,    25,    25,    25,    25,    25,    25,
      27,    25,    25,    25,    25,    25,    25,    35,    25,    25,
      56,    27,   157,   159,    69,    70,    82,    83,   201,   204,
      25,    27,   177,   178,    25,   162,    71,   178,   178,   178,
     178,   162,   178,    90,    91,    92,   162,   219,   162,   195,
     134,   135,   157,   159,   161,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   162,   162,   162,   162,   162,
     162,   162,   162,    64,    65,    67,    70,    78,    79,    80,
      81,    90,    91,    92,   200,   201,   162,   186,    70,   186,
      25,    25,    25,    25,    25,    25,    25,   163,   163,    25,
     178,   205,   206,   207,   209,    25,    70,   162,   162,   179,
     162,   187,   162,   105,   106,   111,   162,   105,   106,   111,
     162,   162,   162,    25,    25,    35,   178,   162,   201,    97,
      90,    91,    92,    97,   201,    35,    35,   163,   207,   209,
     179,   157,    97,   161,    70,   173,   161,    70,   157,   157,
      83,   164,   164,   164,   164,   161,    83,   201,   204,    83,
     201,   204,   157,   157,   157,   157,   157,   157,   157,   161,
      97,   219,    25,    56,    25,   219,    25,   219,   164,   164,
      77,   160,   195,    25,    35,   157,   157,   158,   158,   178,
     158,   219,   219,   219,   125,   135,   157,   161,   157,   161,
      25,    35,   208,   219,    25,    25,    25,    25,    35,    25,
      25,    25,    25,    25,    25,   162,   219,   219,   219,    25,
      25,   132,   133,    34,   162,   188,   162,   189,   158,   158,
     158,   158,    25,    25,    70,   103,   178,   161,    83,   201,
     204,   178,    25,    25,   157,   132,   133,   131,    25,   219,
      25,   219,    25,   219,    25,   178,   178,   178,    25,    25,
      25,    35,   137,    25,    68,    87,   219,   219,   219,    21,
      35,   158,   158,    25,   178,   161,   157,   219,    25,    25,
      25,    25,    25,   164,   219,   164,    25,   219,   164,   219,
     219,   219,    25,   219,   219,   219,    25,   162,    97,   219,
     161,   157,   161,   164,   164,    97,   177,   159,   178,   205,
     205,    35,    35,   157,    35,   219,   161,   161,   135,   134,
      97,    97,   179,   157,   159,   159,   159,   157,   157,   157,
     157,   157,    25,   188,   189,   188,   157,   162,   183,   157,
     162,   184,   161,   161,   157,   124,   125,   142,   142,    35,
      35,    35,    35,   219,   201,   204,    83,   201,   204,    25,
     161,    25,   213,   215,   216,   219,   164,   157,   157,   157,
      25,   161,   161,   161,   162,   196,   196,   196,   196,   157,
     157,   161,   157,   157,   161,   162,   162,   162,   162,   190,
     190,   162,   184,    35,    35,    83,   204,   161,    35,   161,
      91,   161,   161,   161,   161,   164,    97,   164,   164,    97,
     161,   161,   161,   157,   161,   161,   161,   130,    25,   162,
      97,    25,    97,    97,    97,    25,   160,   219,    69,    76,
     178,   157,   161,   161,   162,   178,   162,    25,   219,   219,
     219,    25,    35,    25,    35,    25,    90,    25,   125,   126,
     127,   181,    25,   124,   125,    25,   161,   157,   161,   157,
     161,   161,   189,   188,   219,   164,    25,   219,   164,   161,
     164,   172,   164,    25,    25,    25,    97,   135,   219,   219,
     162,   180,    35,    35,   140,   141,   178,   178,   125,    35,
     190,    25,   164,    97,    25,   219,    97,    25,    97,    97,
      25,    35,   178,   161,   130,     3,     4,     8,    19,   159,
     163,   198,    25,    25,   157,   161,   178,   178,   157,   205,
     159,   157,   157,   157,   161,   161,   161,   161,   161,   219,
      97,   161,    97,   161,   161,    97,   142,   125,   164,   186,
     164,   164,   186,    90,   169,   170,   171,   214,    97,   161,
     161,   210,   161,   161,   161,   120,   121,   161,   161,   161,
     161,   157,   157,   161,   161,   164,   210,    97,    25,    35,
      25,   161,   161,   162,   162,    25,    25,   219,    25,    25,
      27,   178,   219,    25,    25,    25,   180,   186,   162,   161,
      25,    25,    31,   161,   161,   186,   186,   186,   219,   178,
     189,   179,   161,   161,   189,    25,   178,    25,    71,   179,
      25,   180,    25,    25,   159,   162,   161,   159,   161,   162,
     161,   159,   159,   159,   132,   200,   201,    70,    69,   161,
     157,   161,    35,    91,   159,   159,   219,   107,   108,   109,
     110,   191,    97,   219,   125,   162,   192,   219,   219,   219,
     161,    25,    25,    32,   178,    27,    70,   219,   219,   219,
     192,   161,   198,   161,   161,   136,   161,   161,   161,   162,
     185,   184,   184,   161,    21,   157,   157,   161,   162,   193,
     162,   202,   162,   203,   120,   121,   181,   182,   162,    25,
      25,   135,   136,   128,   129,   138,   139,   143,   144,   157,
     161,   157,   161,    35,   159,   159,   157,   161,   157,   161,
     157,   161,   157,   161,   157,   161,   157,   161,   157,   161,
     157,   161,   182,   181,   161,   219,   219,   136,   135,   136,
     136,   136,   136,   136,   136,   161,   161,    71,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,    35,   192,
     192
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   174,   175,   175,   176,   176,   176,   176,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   178,   178,   179,
     179,   179,   179,   179,   180,   180,   181,   181,   181,   181,
     182,   182,   183,   183,   184,   184,   184,   185,   185,   185,
     185,   185,   186,   186,   186,   187,   187,   187,   188,   188,
     188,   188,   188,   188,   189,   189,   190,   190,   191,   191,
     191,   191,   192,   192,   193,   193,   193,   193,   194,   194,
     194,   195,   195,   196,   196,   197,   198,   199,   199,   200,
     200,   200,   200,   201,   201,   202,   202,   202,   202,   202,
     202,   202,   202,   203,   203,   203,   203,   204,   204,   204,
     205,   206,   207,   208,   208,   208,   208,   208,   209,   209,
     209,   210,   211,   211,   211,   211,   212,   212,   212,   212,
     213,   214,   214,   214,   215,   216,   216,   217,   218,   218,
     218,   218,   218,   219,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     2,     6,     4,     1,     1,     2,
       5,     1,     6,     6,     3,     3,    17,    17,    11,    11,
      11,    12,    12,    12,     5,     3,     3,     3,     8,    13,
      12,    13,    13,     8,    17,     6,     9,     3,     6,     3,
       5,     6,     8,     8,     2,     2,     4,     3,     2,     4,
       3,     6,     4,     7,     7,     3,     3,     6,     3,     4,
       3,     3,     3,    11,    11,     9,     5,     5,     9,     5,
       5,     6,     6,     5,     5,     5,     6,     6,     5,     1,
       3,     3,     3,     3,     4,     4,     9,     9,     5,     7,
       4,     6,     6,     7,     9,     8,     8,    11,     9,     4,
       5,     6,     7,     6,     4,     6,     5,     6,     6,     4,
       8,    10,    10,    12,     5,     6,     5,     6,     4,     4,
       4,     7,     9,     9,     9,     6,     6,     6,     8,     8,
       6,     5,     5,     8,     4,     7,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     4,     4,
       6,     6,     2,     2,     4,     2,     2,     2,     2,     2,
       2,     6,     6,     5,     4,     3,     3,     3,     3,     3,
       3,     4,     2,     4,     2,     4,     2,     4,     2,     7,
       8,     8,     7,     7,     7,     9,     7,     8,     9,     8,
       6,     7,     8,     9,     8,     7,     7,     6,    11,     7,
      11,     7,     3,     2,     1,     7,     9,    11,     3,     5,
       7,     2,     2,     2,     2,     1,     1,     2,     2,     4,
       1,     1,     6,     6,     6,     2,     2,     1,     1,     0,
       5,     5,     3,     3,     3,     3,     0,     1,     1,     1,
       1,     1,     0,     3,     0,     3,     3,     0,     3,     3,
       5,     5,     0,     3,     3,     0,     3,     3,     0,     3,
       3,     3,     5,     5,     0,     3,     0,     3,     1,     1,
       1,     1,     0,     3,     3,     3,     5,     5,     1,     1,
       1,     0,     3,     0,     3,     4,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     5,
       5,     5,     5,     3,     3,     5,     5,     0,     1,     1,
       2,     2,     2,     3,     1,     5,     5,     3,     2,     2,
       2,     3,     1,     1,     1,     1,     3,     3,     3,     3,
       1,     1,     1,     1,     3,     1,     1,     3,     1,     1,
       3,     2,     2,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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
#ifndef YYINITDEPTH
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
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
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
      YYSIZE_T yyn = 0;
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

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
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 647 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  insn = (yyvsp[0].instr);
	  if (insn == (INSTR_T) 0)
	    return NO_INSN_GENERATED;
	  else if (insn == (INSTR_T) - 1)
	    return SEMANTIC_ERROR;
	  else
	    return INSN_GENERATED;
	}
#line 2724 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 5:
#line 661 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (((yyvsp[-5].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-3].instr)) && is_group2 ((yyvsp[-1].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-5].instr), (yyvsp[-3].instr), (yyvsp[-1].instr));
	      else if (is_group2 ((yyvsp[-3].instr)) && is_group1 ((yyvsp[-1].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-5].instr), (yyvsp[-1].instr), (yyvsp[-3].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 2 and slot 3 must be 16-bit instruction group");
	    }
	  else if (((yyvsp[-3].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-5].instr)) && is_group2 ((yyvsp[-1].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-3].instr), (yyvsp[-5].instr), (yyvsp[-1].instr));
	      else if (is_group2 ((yyvsp[-5].instr)) && is_group1 ((yyvsp[-1].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-3].instr), (yyvsp[-1].instr), (yyvsp[-5].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 1 and slot 3 must be 16-bit instruction group");
	    }
	  else if (((yyvsp[-1].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-5].instr)) && is_group2 ((yyvsp[-3].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-1].instr), (yyvsp[-5].instr), (yyvsp[-3].instr));
	      else if (is_group2 ((yyvsp[-5].instr)) && is_group1 ((yyvsp[-3].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-1].instr), (yyvsp[-3].instr), (yyvsp[-5].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 1 and slot 2 must be 16-bit instruction group");
	    }
	  else
	    error ("\nIllegal Multi Issue Construct, at least any one of the slot must be DSP32 instruction group\n");
	}
#line 2760 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 6:
#line 694 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (((yyvsp[-3].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-1].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-3].instr), (yyvsp[-1].instr), 0);
	      else if (is_group2 ((yyvsp[-1].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-3].instr), 0, (yyvsp[-1].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 2 must be the 16-bit instruction group");
	    }
	  else if (((yyvsp[-1].instr)->value & 0xf800) == 0xc000)
	    {
	      if (is_group1 ((yyvsp[-3].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-1].instr), (yyvsp[-3].instr), 0);
	      else if (is_group2 ((yyvsp[-3].instr)))
		(yyval.instr) = gen_multi_instr_1 ((yyvsp[-1].instr), 0, (yyvsp[-3].instr));
	      else
		return yyerror ("Wrong 16 bit instructions groups, slot 1 must be the 16-bit instruction group");
	    }
	  else if (is_group1 ((yyvsp[-3].instr)) && is_group2 ((yyvsp[-1].instr)))
	      (yyval.instr) = gen_multi_instr_1 (0, (yyvsp[-3].instr), (yyvsp[-1].instr));
	  else if (is_group2 ((yyvsp[-3].instr)) && is_group1 ((yyvsp[-1].instr)))
	    (yyval.instr) = gen_multi_instr_1 (0, (yyvsp[-1].instr), (yyvsp[-3].instr));
	  else
	    return yyerror ("Wrong 16 bit instructions groups, slot 1 and slot 2 must be the 16-bit instruction group");
	}
#line 2791 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 7:
#line 721 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.instr) = 0;
	yyerror ("");
	yyerrok;
	}
#line 2801 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 8:
#line 732 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.instr) = DSP32MAC (3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0);
	}
#line 2809 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 9:
#line 736 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 2847 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 10:
#line 774 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 2868 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 11:
#line 794 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("dsp32alu: DISALGNEXCPT\n");
	  (yyval.instr) = DSP32ALU (18, 0, 0, 0, 0, 0, 0, 0, 3);
	}
#line 2877 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 12:
#line 799 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && !IS_A1 ((yyvsp[-2].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = ( A0 += A1 )\n");
	      (yyval.instr) = DSP32ALU (11, 0, 0, &(yyvsp[-5].reg), &reg7, &reg7, 0, 0, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 2891 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 13:
#line 809 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_A1 ((yyvsp[-2].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs_half = ( A0 += A1 )\n");
	      (yyval.instr) = DSP32ALU (11, IS_H ((yyvsp[-5].reg)), 0, &(yyvsp[-5].reg), &reg7, &reg7, 0, 0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 2905 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 14:
#line 819 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("dsp32alu: A_ZERO_DOT_H = dregs_hi\n");
	  (yyval.instr) = DSP32ALU (9, IS_H ((yyvsp[0].reg)), 0, 0, &(yyvsp[0].reg), 0, 0, 0, 0);
	}
#line 2914 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 15:
#line 824 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("dsp32alu: A_ZERO_DOT_H = dregs_hi\n");
	  (yyval.instr) = DSP32ALU (9, IS_H ((yyvsp[0].reg)), 0, 0, &(yyvsp[0].reg), 0, 0, 0, 2);
	}
#line 2923 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 16:
#line 830 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-15].reg)) || !IS_DREG ((yyvsp[-13].reg)))
	    return yyerror ("Dregs expected");
	  else if (REG_SAME ((yyvsp[-15].reg), (yyvsp[-13].reg)))
	    return yyerror ("Illegal dest register combination");
	  else if (!valid_dreg_pair (&(yyvsp[-8].reg), (yyvsp[-6].expr)))
	    return yyerror ("Bad dreg pair");
	  else if (!valid_dreg_pair (&(yyvsp[-4].reg), (yyvsp[-2].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: (dregs , dregs ) = BYTEOP16P (dregs_pair , dregs_pair ) (aligndir)\n");
	      (yyval.instr) = DSP32ALU (21, 0, &(yyvsp[-15].reg), &(yyvsp[-13].reg), &(yyvsp[-8].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0, 0);
	    }
	}
#line 2943 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 17:
#line 848 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-15].reg)) || !IS_DREG ((yyvsp[-13].reg)))
	    return yyerror ("Dregs expected");
	  else if (REG_SAME ((yyvsp[-15].reg), (yyvsp[-13].reg)))
	    return yyerror ("Illegal dest register combination");
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
#line 2963 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 18:
#line 865 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-9].reg)) || !IS_DREG ((yyvsp[-7].reg)))
	    return yyerror ("Dregs expected");
	  else if (REG_SAME ((yyvsp[-9].reg), (yyvsp[-7].reg)))
	    return yyerror ("Illegal dest register combination");
	  else if (!valid_dreg_pair (&(yyvsp[-3].reg), (yyvsp[-1].expr)))
	    return yyerror ("Bad dreg pair");
	  else
	    {
	      notethat ("dsp32alu: (dregs , dregs ) = BYTEUNPACK dregs_pair (aligndir)\n");
	      (yyval.instr) = DSP32ALU (24, 0, &(yyvsp[-9].reg), &(yyvsp[-7].reg), &(yyvsp[-3].reg), 0, (yyvsp[0].r0).r0, 0, 1);
	    }
	}
#line 2981 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 19:
#line 879 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-9].reg), (yyvsp[-7].reg)))
	    return yyerror ("Illegal dest register combination");

	  if (IS_DREG ((yyvsp[-9].reg)) && IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: (dregs , dregs ) = SEARCH dregs (searchmod)\n");
	      (yyval.instr) = DSP32ALU (13, 0, &(yyvsp[-9].reg), &(yyvsp[-7].reg), &(yyvsp[-3].reg), 0, 0, 0, (yyvsp[-1].r0).r0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 2998 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 20:
#line 893 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-10].reg), (yyvsp[-4].reg)))
	    return yyerror ("Illegal dest register combination");

	  if (IS_DREG ((yyvsp[-10].reg)) && IS_DREG ((yyvsp[-4].reg)))
	    {
	      notethat ("dsp32alu: dregs = A1.l + A1.h, dregs = A0.l + A0.h  \n");
	      (yyval.instr) = DSP32ALU (12, 0, &(yyvsp[-10].reg), &(yyvsp[-4].reg), &reg7, &reg7, 0, 0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3015 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 21:
#line 908 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-11].reg), (yyvsp[-5].reg)))
	    return yyerror ("Resource conflict in dest reg");

	  if (IS_DREG ((yyvsp[-11].reg)) && IS_DREG ((yyvsp[-5].reg)) && !REG_SAME ((yyvsp[-9].reg), (yyvsp[-7].reg))
	      && IS_A1 ((yyvsp[-3].reg)) && !IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = A1 + A0 , dregs = A1 - A0 (amod1)\n");
	      (yyval.instr) = DSP32ALU (17, 0, &(yyvsp[-11].reg), &(yyvsp[-5].reg), &reg7, &reg7, (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, 0);

	    }
	  else if (IS_DREG ((yyvsp[-11].reg)) && IS_DREG ((yyvsp[-5].reg)) && !REG_SAME ((yyvsp[-9].reg), (yyvsp[-7].reg))
		   && !IS_A1 ((yyvsp[-3].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = A0 + A1 , dregs = A0 - A1 (amod1)\n");
	      (yyval.instr) = DSP32ALU (17, 0, &(yyvsp[-11].reg), &(yyvsp[-5].reg), &reg7, &reg7, (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3040 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 22:
#line 930 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3059 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 23:
#line 948 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!REG_SAME ((yyvsp[-9].reg), (yyvsp[-3].reg)) || !REG_SAME ((yyvsp[-7].reg), (yyvsp[-1].reg)))
	    return yyerror ("Differing source registers");

	  if (!IS_DREG ((yyvsp[-11].reg)) || !IS_DREG ((yyvsp[-9].reg)) || !IS_DREG ((yyvsp[-7].reg)) || !IS_DREG ((yyvsp[-5].reg)))
	    return yyerror ("Dregs expected");

	  if (REG_SAME ((yyvsp[-11].reg), (yyvsp[-5].reg)))
	    return yyerror ("Resource conflict in dest reg");

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
#line 3087 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 24:
#line 973 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3113 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 25:
#line 995 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("dsp32alu: Ax = ABS Ax\n");
	  (yyval.instr) = DSP32ALU (16, IS_A1 ((yyvsp[-2].reg)), 0, 0, &reg7, &reg7, 0, 0, IS_A1 ((yyvsp[0].reg)));
	}
#line 3122 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 26:
#line 1000 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu: A0.l = reg_half\n");
	      (yyval.instr) = DSP32ALU (9, IS_H ((yyvsp[0].reg)), 0, 0, &(yyvsp[0].reg), 0, 0, 0, 0);
	    }
	  else
	    return yyerror ("A0.l = Rx.l expected");
	}
#line 3136 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 27:
#line 1010 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu: A1.l = reg_half\n");
	      (yyval.instr) = DSP32ALU (9, IS_H ((yyvsp[0].reg)), 0, 0, &(yyvsp[0].reg), 0, 0, 0, 2);
	    }
	  else
	    return yyerror ("A1.l = Rx.l expected");
	}
#line 3150 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 28:
#line 1021 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs = ALIGN8 (dregs , dregs )\n");
	      (yyval.instr) = DSP32SHIFT (13, &(yyvsp[-7].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), (yyvsp[-5].r0).r0, 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 3164 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 29:
#line 1032 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3182 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 30:
#line 1046 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3200 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 31:
#line 1062 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3218 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 32:
#line 1078 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3236 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 33:
#line 1093 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = BYTEPACK (dregs , dregs )\n");
	      (yyval.instr) = DSP32ALU (24, 0, 0, &(yyvsp[-7].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg), 0, 0, 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 3250 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 34:
#line 1105 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3267 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 35:
#line 1118 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3297 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 36:
#line 1144 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3318 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 37:
#line 1162 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("dsp32alu: Ax = - Ax\n");
	  (yyval.instr) = DSP32ALU (14, IS_A1 ((yyvsp[-2].reg)), 0, 0, &reg7, &reg7, 0, 0, IS_A1 ((yyvsp[0].reg)));
	}
#line 3327 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 38:
#line 1167 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("dsp32alu: dregs_lo = dregs_lo +- dregs_lo (amod1)\n");
	  (yyval.instr) = DSP32ALU (2 | (yyvsp[-2].r0).r0, IS_H ((yyvsp[-5].reg)), 0, &(yyvsp[-5].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg),
			 (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, HL2 ((yyvsp[-3].reg), (yyvsp[-1].reg)));
	}
#line 3337 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 39:
#line 1173 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (EXPR_VALUE ((yyvsp[0].expr)) == 0 && !REG_SAME ((yyvsp[-2].reg), (yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: A1 = A0 = 0\n");
	      (yyval.instr) = DSP32ALU (8, 0, 0, 0, &reg7, &reg7, 0, 0, 2);
	    }
	  else
	    return yyerror ("Bad value, 0 expected");
	}
#line 3351 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 40:
#line 1185 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-4].reg), (yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: Ax = Ax (S)\n");
	      (yyval.instr) = DSP32ALU (8, 0, 0, 0, &reg7, &reg7, 1, 0, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Registers must be equal");
	}
#line 3365 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 41:
#line 1196 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: dregs_half = dregs (RND)\n");
	      (yyval.instr) = DSP32ALU (12, IS_H ((yyvsp[-5].reg)), 0, &(yyvsp[-5].reg), &(yyvsp[-3].reg), 0, 0, 0, 3);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 3379 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 42:
#line 1207 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: dregs_half = dregs (+-) dregs (RND12)\n");
	      (yyval.instr) = DSP32ALU (5, IS_H ((yyvsp[-7].reg)), 0, &(yyvsp[-7].reg), &(yyvsp[-5].reg), &(yyvsp[-3].reg), 0, 0, (yyvsp[-4].r0).r0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 3393 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 43:
#line 1218 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32alu: dregs_half = dregs -+ dregs (RND20)\n");
	      (yyval.instr) = DSP32ALU (5, IS_H ((yyvsp[-7].reg)), 0, &(yyvsp[-7].reg), &(yyvsp[-5].reg), &(yyvsp[-3].reg), 0, 1, (yyvsp[-4].r0).r0 | 2);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 3407 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 44:
#line 1229 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!REG_SAME ((yyvsp[-1].reg), (yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu: An = Am\n");
	      (yyval.instr) = DSP32ALU (8, 0, 0, 0, &reg7, &reg7, IS_A1 ((yyvsp[-1].reg)), 0, 3);
	    }
	  else
	    return yyerror ("Accu reg arguments must differ");
	}
#line 3421 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 45:
#line 1240 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32alu: An = dregs\n");
	      (yyval.instr) = DSP32ALU (9, 0, 0, 0, &(yyvsp[0].reg), 0, 1, 0, IS_A1 ((yyvsp[-1].reg)) << 1);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 3435 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 46:
#line 1251 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3464 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 47:
#line 1277 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("LDIMMhalf: pregs_half = imm16\n");

	  if (!IS_DREG ((yyvsp[-2].reg)) && !IS_PREG ((yyvsp[-2].reg)) && !IS_IREG ((yyvsp[-2].reg))
	      && !IS_MREG ((yyvsp[-2].reg)) && !IS_BREG ((yyvsp[-2].reg)) && !IS_LREG ((yyvsp[-2].reg)))
	    return yyerror ("Wrong register for load immediate");

	  if (!IS_IMM ((yyvsp[0].expr), 16) && !IS_UIMM ((yyvsp[0].expr), 16))
	    return yyerror ("Constant out of range");

	  (yyval.instr) = LDIMMHALF_R (&(yyvsp[-2].reg), IS_H ((yyvsp[-2].reg)), 0, 0, (yyvsp[0].expr));
	}
#line 3481 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 48:
#line 1291 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("dsp32alu: An = 0\n");

	  if (imm7 ((yyvsp[0].expr)) != 0)
	    return yyerror ("0 expected");

	  (yyval.instr) = DSP32ALU (8, 0, 0, 0, 0, 0, 0, 0, IS_A1 ((yyvsp[-1].reg)));
	}
#line 3494 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 49:
#line 1301 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3542 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 50:
#line 1346 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_H ((yyvsp[-2].reg)))
	    return yyerror ("Low reg expected");

	  if (IS_DREG ((yyvsp[-2].reg)) && (yyvsp[0].reg).regno == REG_A0x)
	    {
	      notethat ("dsp32alu: dregs_lo = A0.x\n");
	      (yyval.instr) = DSP32ALU (10, 0, 0, &(yyvsp[-2].reg), &reg7, &reg7, 0, 0, 0);
	    }
	  else if (IS_DREG ((yyvsp[-2].reg)) && (yyvsp[0].reg).regno == REG_A1x)
	    {
	      notethat ("dsp32alu: dregs_lo = A1.x\n");
	      (yyval.instr) = DSP32ALU (10, 0, 0, &(yyvsp[-2].reg), &reg7, &reg7, 0, 0, 1);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3564 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 51:
#line 1365 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: dregs = dregs .|. dregs (amod0)\n");
	      (yyval.instr) = DSP32ALU (0, 0, 0, &(yyvsp[-5].reg), &(yyvsp[-3].reg), &(yyvsp[-1].reg), (yyvsp[0].modcodes).s0, (yyvsp[0].modcodes).x0, (yyvsp[-2].r0).r0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3578 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 52:
#line 1376 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ALU2op: dregs = dregs_byte\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[-1].reg), 12 | ((yyvsp[0].r0).r0 ? 0: 1));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3592 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 53:
#line 1387 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-6].reg), (yyvsp[-4].reg)) && REG_SAME ((yyvsp[-2].reg), (yyvsp[0].reg)) && !REG_SAME ((yyvsp[-6].reg), (yyvsp[-2].reg)))
	    {
	      notethat ("dsp32alu: A1 = ABS A1 , A0 = ABS A0\n");
	      (yyval.instr) = DSP32ALU (16, 0, 0, 0, &reg7, &reg7, 0, 0, 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3606 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 54:
#line 1398 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-6].reg), (yyvsp[-4].reg)) && REG_SAME ((yyvsp[-2].reg), (yyvsp[0].reg)) && !REG_SAME ((yyvsp[-6].reg), (yyvsp[-2].reg)))
	    {
	      notethat ("dsp32alu: A1 = - A1 , A0 = - A0\n");
	      (yyval.instr) = DSP32ALU (14, 0, 0, 0, &reg7, &reg7, 0, 0, 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3620 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 55:
#line 1409 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_A1 ((yyvsp[-2].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: A0 -= A1\n");
	      (yyval.instr) = DSP32ALU (11, 0, 0, 0, &reg7, &reg7, (yyvsp[0].r0).r0, 0, 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3634 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 56:
#line 1420 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3653 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 57:
#line 1436 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3673 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 58:
#line 1453 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3692 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 59:
#line 1469 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_A1 ((yyvsp[-3].reg)) && IS_A1 ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32alu: A0 += A1 (W32)\n");
	      (yyval.instr) = DSP32ALU (11, 0, 0, 0, &reg7, &reg7, (yyvsp[0].r0).r0, 0, 2);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3706 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 60:
#line 1480 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_IREG ((yyvsp[-2].reg)) && IS_MREG ((yyvsp[0].reg)))
	    {
	      notethat ("dagMODim: iregs += mregs\n");
	      (yyval.instr) = DAGMODIM (&(yyvsp[-2].reg), &(yyvsp[0].reg), 0, 0);
	    }
	  else
	    return yyerror ("iregs += mregs expected");
	}
#line 3720 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 61:
#line 1491 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3756 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 62:
#line 1524 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs *= dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3770 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 63:
#line 1535 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3786 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 64:
#line 1548 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-10].reg), (yyvsp[-9].reg)) && REG_SAME ((yyvsp[-4].reg), (yyvsp[-3].reg)) && !REG_SAME ((yyvsp[-10].reg), (yyvsp[-4].reg)))
	    {
	      notethat ("dsp32alu: A1 = A1 (S) , A0 = A0 (S)\n");
	      (yyval.instr) = DSP32ALU (8, 0, 0, 0, &reg7, &reg7, 1, 0, 2);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 3800 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 65:
#line 1559 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3841 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 66:
#line 1598 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("COMP3op: dregs = dregs | dregs\n");
	      (yyval.instr) = COMP3OP (&(yyvsp[-4].reg), &(yyvsp[-2].reg), &(yyvsp[0].reg), 3);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 3855 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 67:
#line 1608 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("COMP3op: dregs = dregs ^ dregs\n");
	      (yyval.instr) = COMP3OP (&(yyvsp[-4].reg), &(yyvsp[-2].reg), &(yyvsp[0].reg), 4);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 3869 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 68:
#line 1618 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 3893 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 69:
#line 1638 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((yyvsp[-2].reg).regno == REG_A0 && (yyvsp[0].reg).regno == REG_A1)
	    {
	      notethat ("CCflag: CC = A0 == A1\n");
	      (yyval.instr) = CCFLAG (0, 0, 5, 0, 0);
	    }
	  else
	    return yyerror ("AREGs are in bad order or same");
	}
#line 3907 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 70:
#line 1648 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((yyvsp[-2].reg).regno == REG_A0 && (yyvsp[0].reg).regno == REG_A1)
	    {
	      notethat ("CCflag: CC = A0 < A1\n");
	      (yyval.instr) = CCFLAG (0, 0, 6, 0, 0);
	    }
	  else
	    return yyerror ("AREGs are in bad order or same");
	}
#line 3921 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 71:
#line 1658 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	      || (IS_PREG ((yyvsp[-3].reg)) && IS_PREG ((yyvsp[-1].reg))))
	    {
	      notethat ("CCflag: CC = dpregs < dpregs\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), (yyvsp[-1].reg).regno & CODE_MASK, (yyvsp[0].r0).r0, 0, IS_PREG ((yyvsp[-3].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Bad register in comparison");
	}
#line 3936 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 72:
#line 1669 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-3].reg)) && !IS_PREG ((yyvsp[-3].reg)))
	    return yyerror ("Bad register in comparison");

	  if (((yyvsp[0].r0).r0 == 1 && IS_IMM ((yyvsp[-1].expr), 3))
	      || ((yyvsp[0].r0).r0 == 3 && IS_UIMM ((yyvsp[-1].expr), 3)))
	    {
	      notethat ("CCflag: CC = dpregs < (u)imm3\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), imm3 ((yyvsp[-1].expr)), (yyvsp[0].r0).r0, 1, IS_PREG ((yyvsp[-3].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Bad constant value");
	}
#line 3954 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 73:
#line 1683 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	      || (IS_PREG ((yyvsp[-2].reg)) && IS_PREG ((yyvsp[0].reg))))
	    {
	      notethat ("CCflag: CC = dpregs == dpregs\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-2].reg), (yyvsp[0].reg).regno & CODE_MASK, 0, 0, IS_PREG ((yyvsp[-2].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Bad register in comparison");
	}
#line 3969 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 74:
#line 1694 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-2].reg)) && !IS_PREG ((yyvsp[-2].reg)))
	    return yyerror ("Bad register in comparison");

	  if (IS_IMM ((yyvsp[0].expr), 3))
	    {
	      notethat ("CCflag: CC = dpregs == imm3\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-2].reg), imm3 ((yyvsp[0].expr)), 0, 1, IS_PREG ((yyvsp[-2].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Bad constant range");
	}
#line 3986 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 75:
#line 1707 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((yyvsp[-2].reg).regno == REG_A0 && (yyvsp[0].reg).regno == REG_A1)
	    {
	      notethat ("CCflag: CC = A0 <= A1\n");
	      (yyval.instr) = CCFLAG (0, 0, 7, 0, 0);
	    }
	  else
	    return yyerror ("AREGs are in bad order or same");
	}
#line 4000 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 76:
#line 1717 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	      || (IS_PREG ((yyvsp[-3].reg)) && IS_PREG ((yyvsp[-1].reg))))
	    {
	      notethat ("CCflag: CC = dpregs <= dpregs (..)\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), (yyvsp[-1].reg).regno & CODE_MASK,
			   1 + (yyvsp[0].r0).r0, 0, IS_PREG ((yyvsp[-3].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Bad register in comparison");
	}
#line 4016 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 77:
#line 1729 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-3].reg)) && !IS_PREG ((yyvsp[-3].reg)))
	    return yyerror ("Bad register in comparison");

	  if (((yyvsp[0].r0).r0 == 1 && IS_IMM ((yyvsp[-1].expr), 3))
	      || ((yyvsp[0].r0).r0 == 3 && IS_UIMM ((yyvsp[-1].expr), 3)))
	    {
	      notethat ("CCflag: CC = dpregs <= (u)imm3\n");
	      (yyval.instr) = CCFLAG (&(yyvsp[-3].reg), imm3 ((yyvsp[-1].expr)), 1 + (yyvsp[0].r0).r0, 1, IS_PREG ((yyvsp[-3].reg)) ? 1 : 0);
	    }
	  else
	    return yyerror ("Bad constant value");
	}
#line 4034 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 78:
#line 1744 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("COMP3op: dregs = dregs & dregs\n");
	      (yyval.instr) = COMP3OP (&(yyvsp[-4].reg), &(yyvsp[-2].reg), &(yyvsp[0].reg), 2);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 4048 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 79:
#line 1755 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("CC2stat operation\n");
	  (yyval.instr) = bfin_gen_cc2stat ((yyvsp[0].modcodes).r0, (yyvsp[0].modcodes).x0, (yyvsp[0].modcodes).s0);
	}
#line 4057 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 80:
#line 1761 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((IS_GENREG ((yyvsp[-2].reg)) && IS_GENREG ((yyvsp[0].reg)))
	      || (IS_GENREG ((yyvsp[-2].reg)) && IS_DAGREG ((yyvsp[0].reg)))
	      || (IS_DAGREG ((yyvsp[-2].reg)) && IS_GENREG ((yyvsp[0].reg)))
	      || (IS_DAGREG ((yyvsp[-2].reg)) && IS_DAGREG ((yyvsp[0].reg)))
	      || (IS_GENREG ((yyvsp[-2].reg)) && (yyvsp[0].reg).regno == REG_USP)
	      || ((yyvsp[-2].reg).regno == REG_USP && IS_GENREG ((yyvsp[0].reg)))
	      || ((yyvsp[-2].reg).regno == REG_USP && (yyvsp[0].reg).regno == REG_USP)
	      || (IS_DREG ((yyvsp[-2].reg)) && IS_SYSREG ((yyvsp[0].reg)))
	      || (IS_PREG ((yyvsp[-2].reg)) && IS_SYSREG ((yyvsp[0].reg)))
	      || (IS_SYSREG ((yyvsp[-2].reg)) && IS_GENREG ((yyvsp[0].reg)))
	      || (IS_ALLREG ((yyvsp[-2].reg)) && IS_EMUDAT ((yyvsp[0].reg)))
	      || (IS_EMUDAT ((yyvsp[-2].reg)) && IS_ALLREG ((yyvsp[0].reg)))
	      || (IS_SYSREG ((yyvsp[-2].reg)) && (yyvsp[0].reg).regno == REG_USP))
	    {
	      (yyval.instr) = bfin_gen_regmv (&(yyvsp[0].reg), &(yyvsp[-2].reg));
	    }
	  else
	    return yyerror ("Unsupported register move");
	}
#line 4082 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 81:
#line 1783 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("CC2dreg: CC = dregs\n");
	      (yyval.instr) = bfin_gen_cc2dreg (1, &(yyvsp[0].reg));
	    }
	  else
	    return yyerror ("Only 'CC = Dreg' supported");
	}
#line 4096 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 82:
#line 1794 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)))
	    {
	      notethat ("CC2dreg: dregs = CC\n");
	      (yyval.instr) = bfin_gen_cc2dreg (0, &(yyvsp[-2].reg));
	    }
	  else
	    return yyerror ("Only 'Dreg = CC' supported");
	}
#line 4110 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 83:
#line 1805 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("CC2dreg: CC =! CC\n");
	  (yyval.instr) = bfin_gen_cc2dreg (3, 0);
	}
#line 4119 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 84:
#line 1813 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4148 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 85:
#line 1839 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4181 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 86:
#line 1870 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4212 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 87:
#line 1898 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4244 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 88:
#line 1929 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4261 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 89:
#line 1943 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-1].reg)) && !IS_H ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs_half = ASHIFT dregs_half BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (0, &(yyvsp[-6].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0, HL2 ((yyvsp[-6].reg), (yyvsp[-3].reg)));
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 4275 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 90:
#line 1954 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4292 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 91:
#line 1968 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4330 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 92:
#line 2002 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_UIMM ((yyvsp[-1].expr), 4))
	    {
	      if ((yyvsp[0].modcodes).s0)
		{
		  notethat ("dsp32shiftimm: dregs_half = dregs_half << uimm4 (S)\n");
		  (yyval.instr) = DSP32SHIFTIMM (0x0, &(yyvsp[-5].reg), imm5 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), (yyvsp[0].modcodes).s0, HL2 ((yyvsp[-5].reg), (yyvsp[-3].reg)));
		}
	      else
		{
		  notethat ("dsp32shiftimm: dregs_half = dregs_half << uimm4\n");
		  (yyval.instr) = DSP32SHIFTIMM (0x0, &(yyvsp[-5].reg), imm5 ((yyvsp[-1].expr)), &(yyvsp[-3].reg), 2, HL2 ((yyvsp[-5].reg), (yyvsp[-3].reg)));
		}
	    }
	  else
	    return yyerror ("Bad shift value");
	}
#line 4352 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 93:
#line 2020 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4379 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 94:
#line 2045 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG_L ((yyvsp[-8].reg)) && IS_DREG_L ((yyvsp[-4].reg)) && IS_DREG_L ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = EXPADJ (dregs , dregs_lo )\n");
	      (yyval.instr) = DSP32SHIFT (7, &(yyvsp[-8].reg), &(yyvsp[-2].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0);
	    }
	  else
	    return yyerror ("Bad shift value or register");
	}
#line 4393 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 95:
#line 2057 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4412 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 96:
#line 2075 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs = DEPOSIT (dregs , dregs )\n");
	      (yyval.instr) = DSP32SHIFT (10, &(yyvsp[-7].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), 2, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4426 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 97:
#line 2086 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-10].reg)) && IS_DREG ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-4].reg)))
	    {
	      notethat ("dsp32shift: dregs = DEPOSIT (dregs , dregs ) (X)\n");
	      (yyval.instr) = DSP32SHIFT (10, &(yyvsp[-10].reg), &(yyvsp[-4].reg), &(yyvsp[-6].reg), 3, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4440 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 98:
#line 2097 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-8].reg)) && IS_DREG ((yyvsp[-4].reg)) && IS_DREG_L ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: dregs = EXTRACT (dregs, dregs_lo ) (.)\n");
	      (yyval.instr) = DSP32SHIFT (10, &(yyvsp[-8].reg), &(yyvsp[-2].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4454 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 99:
#line 2108 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4471 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 100:
#line 2121 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-4].reg), (yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: Ax = LSHIFT Ax BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (3, 0, &(yyvsp[0].reg), 0, 1, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4485 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 101:
#line 2132 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = LSHIFT dregs_hi BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (0, &(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-2].reg), 2, HL2 ((yyvsp[-5].reg), (yyvsp[-2].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4499 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 102:
#line 2143 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG_L ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs = LSHIFT dregs BY dregs_lo (V )\n");
	      (yyval.instr) = DSP32SHIFT ((yyvsp[0].r0).r0 ? 1: 2, &(yyvsp[-6].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), 2, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4513 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 103:
#line 2154 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs = SHIFT dregs BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (2, &(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-2].reg), 2, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4527 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 104:
#line 2165 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-3].reg), (yyvsp[-2].reg)) && IS_IMM ((yyvsp[0].expr), 6) >= 0)
	    {
	      notethat ("dsp32shiftimm: Ax = Ax >> imm6\n");
	      (yyval.instr) = DSP32SHIFTIMM (3, 0, -imm6 ((yyvsp[0].expr)), 0, 1, IS_A1 ((yyvsp[-3].reg)));
	    }
	  else
	    return yyerror ("Accu register expected");
	}
#line 4541 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 105:
#line 2176 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4578 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 106:
#line 2209 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("dsp32shiftimm:  dregs_half =  dregs_half >> uimm5\n");
	      (yyval.instr) = DSP32SHIFTIMM (0, &(yyvsp[-4].reg), -uimm5 ((yyvsp[0].expr)), &(yyvsp[-2].reg), 2, HL2 ((yyvsp[-4].reg), (yyvsp[-2].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4592 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 107:
#line 2219 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4607 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 108:
#line 2232 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4630 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 109:
#line 2252 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG_L ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = ONES dregs\n");
	      (yyval.instr) = DSP32SHIFT (6, &(yyvsp[-3].reg), 0, &(yyvsp[0].reg), 3, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4644 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 110:
#line 2263 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-7].reg)) && IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dsp32shift: dregs = PACK (dregs_hi , dregs_hi )\n");
	      (yyval.instr) = DSP32SHIFT (4, &(yyvsp[-7].reg), &(yyvsp[-1].reg), &(yyvsp[-3].reg), HL2 ((yyvsp[-3].reg), (yyvsp[-1].reg)), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4658 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 111:
#line 2274 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4674 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 112:
#line 2287 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4690 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 113:
#line 2300 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-11].reg)) && !IS_H ((yyvsp[-11].reg)) && !REG_SAME ((yyvsp[-5].reg), (yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = CC = BXOR (A0 , A1 , CC)\n");
	      (yyval.instr) = DSP32SHIFT (12, &(yyvsp[-11].reg), 0, 0, 1, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4704 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 114:
#line 2311 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-4].reg), (yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: Ax = ROT Ax BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (3, 0, &(yyvsp[0].reg), 0, 2, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4718 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 115:
#line 2322 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_DREG_L ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs = ROT dregs BY dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (2, &(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-2].reg), 3, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4732 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 116:
#line 2333 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_IMM ((yyvsp[0].expr), 6))
	    {
	      notethat ("dsp32shiftimm: An = ROT An BY imm6\n");
	      (yyval.instr) = DSP32SHIFTIMM (3, 0, imm6 ((yyvsp[0].expr)), 0, 2, IS_A1 ((yyvsp[-4].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4746 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 117:
#line 2344 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-5].reg)) && IS_DREG ((yyvsp[-2].reg)) && IS_IMM ((yyvsp[0].expr), 6))
	    {
	      (yyval.instr) = DSP32SHIFTIMM (2, &(yyvsp[-5].reg), imm6 ((yyvsp[0].expr)), &(yyvsp[-2].reg), 3, IS_A1 ((yyvsp[-5].reg)));
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4759 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 118:
#line 2354 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG_L ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = SIGNBITS An\n");
	      (yyval.instr) = DSP32SHIFT (6, &(yyvsp[-3].reg), 0, 0, IS_A1 ((yyvsp[0].reg)), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4773 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 119:
#line 2365 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG_L ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = SIGNBITS dregs\n");
	      (yyval.instr) = DSP32SHIFT (5, &(yyvsp[-3].reg), 0, &(yyvsp[0].reg), 0, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4787 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 120:
#line 2376 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG_L ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = SIGNBITS dregs_lo\n");
	      (yyval.instr) = DSP32SHIFT (5, &(yyvsp[-3].reg), 0, &(yyvsp[0].reg), 1 + IS_H ((yyvsp[0].reg)), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4801 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 121:
#line 2388 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG_L ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: dregs_lo = VIT_MAX (dregs) (..)\n");
	      (yyval.instr) = DSP32SHIFT (9, &(yyvsp[-6].reg), 0, &(yyvsp[-2].reg), ((yyvsp[0].r0).r0 ? 0 : 1), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4815 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 122:
#line 2399 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-8].reg)) && IS_DREG ((yyvsp[-4].reg)) && IS_DREG ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: dregs = VIT_MAX (dregs, dregs) (ASR)\n");
	      (yyval.instr) = DSP32SHIFT (9, &(yyvsp[-8].reg), &(yyvsp[-2].reg), &(yyvsp[-4].reg), 2 | ((yyvsp[0].r0).r0 ? 0 : 1), 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4829 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 123:
#line 2410 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (REG_SAME ((yyvsp[-6].reg), (yyvsp[-4].reg)))
	    return yyerror ("Illegal source register combination");

	  if (IS_DREG ((yyvsp[-6].reg)) && IS_DREG ((yyvsp[-4].reg)) && !IS_A1 ((yyvsp[-2].reg)))
	    {
	      notethat ("dsp32shift: BITMUX (dregs , dregs , A0) (ASR)\n");
	      (yyval.instr) = DSP32SHIFT (8, 0, &(yyvsp[-6].reg), &(yyvsp[-4].reg), (yyvsp[0].r0).r0, 0);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4846 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 124:
#line 2424 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_A1 ((yyvsp[-8].reg)) && !IS_A1 ((yyvsp[-5].reg)) && IS_A1 ((yyvsp[-3].reg)))
	    {
	      notethat ("dsp32shift: A0 = BXORSHIFT (A0 , A1 , CC )\n");
	      (yyval.instr) = DSP32SHIFT (12, 0, 0, 0, 0, 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 4860 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 125:
#line 2437 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: BITCLR (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 4);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4874 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 126:
#line 2449 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: BITCLR (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 2);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4888 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 127:
#line 2461 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: BITCLR (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 3);
	    }
	  else
	    return yyerror ("Register mismatch");
	}
#line 4902 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 128:
#line 2472 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: CC =! BITTST (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 0);
	    }
	  else
	    return yyerror ("Register mismatch or value error");
	}
#line 4916 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 129:
#line 2483 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_UIMM ((yyvsp[-1].expr), 5))
	    {
	      notethat ("LOGI2op: CC = BITTST (dregs , uimm5 )\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-3].reg), uimm5 ((yyvsp[-1].expr)), 1);
	    }
	  else
	    return yyerror ("Register mismatch or value error");
	}
#line 4930 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 130:
#line 2494 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4945 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 131:
#line 2506 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 4960 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 132:
#line 2518 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL10 ((yyvsp[0].expr)))
	    {
	      notethat ("BRCC: IF !CC JUMP  pcrel11m2\n");
	      (yyval.instr) = BRCC (0, 0, (yyvsp[0].expr));
	    }
	  else
	    return yyerror ("Bad jump offset");
	}
#line 4974 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 133:
#line 2529 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL10 ((yyvsp[-3].expr)))
	    {
	      notethat ("BRCC: IF !CC JUMP  pcrel11m2\n");
	      (yyval.instr) = BRCC (0, 1, (yyvsp[-3].expr));
	    }
	  else
	    return yyerror ("Bad jump offset");
	}
#line 4988 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 134:
#line 2540 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL10 ((yyvsp[0].expr)))
	    {
	      notethat ("BRCC: IF CC JUMP  pcrel11m2\n");
	      (yyval.instr) = BRCC (1, 0, (yyvsp[0].expr));
	    }
	  else
	    return yyerror ("Bad jump offset");
	}
#line 5002 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 135:
#line 2551 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL10 ((yyvsp[-3].expr)))
	    {
	      notethat ("BRCC: IF !CC JUMP  pcrel11m2\n");
	      (yyval.instr) = BRCC (1, 1, (yyvsp[-3].expr));
	    }
	  else
	    return yyerror ("Bad jump offset");
	}
#line 5016 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 136:
#line 2561 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: NOP\n");
	  (yyval.instr) = PROGCTRL (0, 0);
	}
#line 5025 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 137:
#line 2567 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: RTS\n");
	  (yyval.instr) = PROGCTRL (1, 0);
	}
#line 5034 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 138:
#line 2573 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: RTI\n");
	  (yyval.instr) = PROGCTRL (1, 1);
	}
#line 5043 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 139:
#line 2579 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: RTX\n");
	  (yyval.instr) = PROGCTRL (1, 2);
	}
#line 5052 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 140:
#line 2585 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: RTN\n");
	  (yyval.instr) = PROGCTRL (1, 3);
	}
#line 5061 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 141:
#line 2591 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: RTE\n");
	  (yyval.instr) = PROGCTRL (1, 4);
	}
#line 5070 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 142:
#line 2597 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: IDLE\n");
	  (yyval.instr) = PROGCTRL (2, 0);
	}
#line 5079 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 143:
#line 2603 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: CSYNC\n");
	  (yyval.instr) = PROGCTRL (2, 3);
	}
#line 5088 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 144:
#line 2609 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: SSYNC\n");
	  (yyval.instr) = PROGCTRL (2, 4);
	}
#line 5097 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 145:
#line 2615 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("ProgCtrl: EMUEXCPT\n");
	  (yyval.instr) = PROGCTRL (2, 5);
	}
#line 5106 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 146:
#line 2621 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ProgCtrl: CLI dregs\n");
	      (yyval.instr) = PROGCTRL (3, (yyvsp[0].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Dreg expected for CLI");
	}
#line 5120 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 147:
#line 2632 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ProgCtrl: STI dregs\n");
	      (yyval.instr) = PROGCTRL (4, (yyvsp[0].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Dreg expected for STI");
	}
#line 5134 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 148:
#line 2643 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: JUMP (pregs )\n");
	      (yyval.instr) = PROGCTRL (5, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Bad register for indirect jump");
	}
#line 5148 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 149:
#line 2654 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: CALL (pregs )\n");
	      (yyval.instr) = PROGCTRL (6, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Bad register for indirect call");
	}
#line 5162 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 150:
#line 2665 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: CALL (PC + pregs )\n");
	      (yyval.instr) = PROGCTRL (7, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Bad register for indirect call");
	}
#line 5176 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 151:
#line 2676 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("ProgCtrl: JUMP (PC + pregs )\n");
	      (yyval.instr) = PROGCTRL (8, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Bad register for indirect jump");
	}
#line 5190 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 152:
#line 2687 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_UIMM ((yyvsp[0].expr), 4))
	    {
	      notethat ("ProgCtrl: RAISE uimm4\n");
	      (yyval.instr) = PROGCTRL (9, uimm4 ((yyvsp[0].expr)));
	    }
	  else
	    return yyerror ("Bad value for RAISE");
	}
#line 5204 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 153:
#line 2698 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
		notethat ("ProgCtrl: EMUEXCPT\n");
		(yyval.instr) = PROGCTRL (10, uimm4 ((yyvsp[0].expr)));
	}
#line 5213 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 154:
#line 2704 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      if ((yyvsp[-1].reg).regno == REG_SP || (yyvsp[-1].reg).regno == REG_FP)
		return yyerror ("Bad register for TESTSET");

	      notethat ("ProgCtrl: TESTSET (pregs )\n");
	      (yyval.instr) = PROGCTRL (11, (yyvsp[-1].reg).regno & CODE_MASK);
	    }
	  else
	    return yyerror ("Preg expected");
	}
#line 5230 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 155:
#line 2718 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL12 ((yyvsp[0].expr)))
	    {
	      notethat ("UJUMP: JUMP pcrel12\n");
	      (yyval.instr) = UJUMP ((yyvsp[0].expr));
	    }
	  else
	    return yyerror ("Bad value for relative jump");
	}
#line 5244 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 156:
#line 2729 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL12 ((yyvsp[0].expr)))
	    {
	      notethat ("UJUMP: JUMP_DOT_S pcrel12\n");
	      (yyval.instr) = UJUMP((yyvsp[0].expr));
	    }
	  else
	    return yyerror ("Bad value for relative jump");
	}
#line 5258 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 157:
#line 2740 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL24 ((yyvsp[0].expr)))
	    {
	      notethat ("CALLa: jump.l pcrel24\n");
	      (yyval.instr) = CALLA ((yyvsp[0].expr), 0);
	    }
	  else
	    return yyerror ("Bad value for long jump");
	}
#line 5272 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 158:
#line 2751 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL24 ((yyvsp[0].expr)))
	    {
	      notethat ("CALLa: jump.l pcrel24\n");
	      (yyval.instr) = CALLA ((yyvsp[0].expr), 2);
	    }
	  else
	    return yyerror ("Bad value for long jump");
	}
#line 5286 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 159:
#line 2762 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL24 ((yyvsp[0].expr)))
	    {
	      notethat ("CALLa: CALL pcrel25m2\n");
	      (yyval.instr) = CALLA ((yyvsp[0].expr), 1);
	    }
	  else
	    return yyerror ("Bad call address");
	}
#line 5300 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 160:
#line 2772 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL24 ((yyvsp[0].expr)))
	    {
	      notethat ("CALLa: CALL pcrel25m2\n");
	      (yyval.instr) = CALLA ((yyvsp[0].expr), 2);
	    }
	  else
	    return yyerror ("Bad call address");
	}
#line 5314 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 161:
#line 2785 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[-1].reg), 8);
	  else
	    return yyerror ("Bad registers for DIVQ");
	}
#line 5325 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 162:
#line 2793 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[-1].reg)))
	    (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[-1].reg), 9);
	  else
	    return yyerror ("Bad registers for DIVS");
	}
#line 5336 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 163:
#line 2801 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 5363 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 164:
#line 2825 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-3].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs = ~dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-3].reg), &(yyvsp[0].reg), 15);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 5377 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 165:
#line 2836 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs >>= dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 1);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 5391 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 166:
#line 2847 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("LOGI2op: dregs >>= uimm5\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-2].reg), uimm5 ((yyvsp[0].expr)), 6);
	    }
	  else
	    return yyerror ("Dregs expected or value error");
	}
#line 5405 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 167:
#line 2858 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs >>>= dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 0);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 5419 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 168:
#line 2869 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("ALU2op: dregs <<= dregs\n");
	      (yyval.instr) = ALU2OP (&(yyvsp[-2].reg), &(yyvsp[0].reg), 2);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 5433 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 169:
#line 2880 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("LOGI2op: dregs <<= uimm5\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-2].reg), uimm5 ((yyvsp[0].expr)), 7);
	    }
	  else
	    return yyerror ("Dregs expected or const value error");
	}
#line 5447 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 170:
#line 2892 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_UIMM ((yyvsp[0].expr), 5))
	    {
	      notethat ("LOGI2op: dregs >>>= uimm5\n");
	      (yyval.instr) = LOGI2OP ((yyvsp[-2].reg), uimm5 ((yyvsp[0].expr)), 5);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 5461 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 171:
#line 2905 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("CaCTRL: FLUSH [ pregs ]\n");
	  if (IS_PREG ((yyvsp[-1].reg)))
	    (yyval.instr) = CACTRL (&(yyvsp[-1].reg), 0, 2);
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
#line 5473 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 172:
#line 2914 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("CaCTRL: FLUSH [ pregs ++ ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[0].reg), 1, 2);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
#line 5487 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 173:
#line 2925 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("CaCTRL: FLUSHINV [ pregs ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[-1].reg), 0, 1);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
#line 5501 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 174:
#line 2936 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("CaCTRL: FLUSHINV [ pregs ++ ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[0].reg), 1, 1);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
#line 5515 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 175:
#line 2948 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("CaCTRL: IFLUSH [ pregs ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[-1].reg), 0, 3);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
#line 5529 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 176:
#line 2959 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("CaCTRL: IFLUSH [ pregs ++ ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[0].reg), 1, 3);
	    }
	  else
	    return yyerror ("Bad register(s) for FLUSH");
	}
#line 5543 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 177:
#line 2970 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("CaCTRL: PREFETCH [ pregs ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[-1].reg), 0, 0);
	    }
	  else
	    return yyerror ("Bad register(s) for PREFETCH");
	}
#line 5557 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 178:
#line 2981 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PREG ((yyvsp[0].reg)))
	    {
	      notethat ("CaCTRL: PREFETCH [ pregs ++ ]\n");
	      (yyval.instr) = CACTRL (&(yyvsp[0].reg), 1, 0);
	    }
	  else
	    return yyerror ("Bad register(s) for PREFETCH");
	}
#line 5571 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 179:
#line 2995 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg expected for source operand");
	  if (!IS_PREG ((yyvsp[-4].reg)))
	    return yyerror ("Preg expected in address");

	  notethat ("LDST: B [ pregs <post_op> ] = dregs\n");
	  (yyval.instr) = LDST (&(yyvsp[-4].reg), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 2, 0, 1);
	}
#line 5585 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 180:
#line 3007 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  Expr_Node *tmp = (yyvsp[-3].expr);

	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg expected for source operand");
	  if (!IS_PREG ((yyvsp[-5].reg)))
	    return yyerror ("Preg expected in address");

	  if (IS_RELOC ((yyvsp[-3].expr)))
	    return yyerror ("Plain symbol used as offset");

	  if ((yyvsp[-4].r0).r0)
	    tmp = unary (Expr_Op_Type_NEG, tmp);

	  if (in_range_p (tmp, -32768, 32767, 0))
	    {
	      notethat ("LDST: B [ pregs + imm16 ] = dregs\n");
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-5].reg), &(yyvsp[0].reg), 1, 2, 0, (yyvsp[-3].expr));
	    }
	  else
	    return yyerror ("Displacement out of range");
	}
#line 5612 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 181:
#line 3033 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  Expr_Node *tmp = (yyvsp[-3].expr);

	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg expected for source operand");
	  if (!IS_PREG ((yyvsp[-5].reg)))
	    return yyerror ("Preg expected in address");

	  if ((yyvsp[-4].r0).r0)
	    tmp = unary (Expr_Op_Type_NEG, tmp);

	  if (IS_RELOC ((yyvsp[-3].expr)))
	    return yyerror ("Plain symbol used as offset");

	  if (in_range_p (tmp, 0, 30, 1))
	    {
	      notethat ("LDSTii: W [ pregs +- uimm5m2 ] = dregs\n");
	      (yyval.instr) = LDSTII (&(yyvsp[-5].reg), &(yyvsp[0].reg), tmp, 1, 1);
	    }
	  else if (in_range_p (tmp, -65536, 65535, 1))
	    {
	      notethat ("LDSTidxI: W [ pregs + imm17m2 ] = dregs\n");
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-5].reg), &(yyvsp[0].reg), 1, 1, 0, tmp);
	    }
	  else
	    return yyerror ("Displacement out of range");
	}
#line 5644 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 182:
#line 3063 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg expected for source operand");
	  if (!IS_PREG ((yyvsp[-4].reg)))
	    return yyerror ("Preg expected in address");

	  notethat ("LDST: W [ pregs <post_op> ] = dregs\n");
	  (yyval.instr) = LDST (&(yyvsp[-4].reg), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 1, 0, 1);
	}
#line 5658 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 183:
#line 3074 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg expected for source operand");
	  if ((yyvsp[-3].modcodes).x0 == 2)
	    {
	      if (!IS_IREG ((yyvsp[-4].reg)) && !IS_PREG ((yyvsp[-4].reg)))
		return yyerror ("Ireg or Preg expected in address");
	    }
	  else if (!IS_IREG ((yyvsp[-4].reg)))
	    return yyerror ("Ireg expected in address");

	  if (IS_IREG ((yyvsp[-4].reg)))
	    {
	      notethat ("dspLDST: W [ iregs <post_op> ] = dregs_half\n");
	      (yyval.instr) = DSPLDST (&(yyvsp[-4].reg), 1 + IS_H ((yyvsp[0].reg)), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 1);
	    }
	  else
	    {
	      notethat ("LDSTpmod: W [ pregs ] = dregs_half\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-4].reg), &(yyvsp[0].reg), &(yyvsp[-4].reg), 1 + IS_H ((yyvsp[0].reg)), 1);
	    }
	}
#line 5685 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 184:
#line 3099 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  Expr_Node *tmp = (yyvsp[-3].expr);
	  int ispreg = IS_PREG ((yyvsp[0].reg));

	  if (!IS_PREG ((yyvsp[-5].reg)))
	    return yyerror ("Preg expected in address");

	  if (!IS_DREG ((yyvsp[0].reg)) && !ispreg)
	    return yyerror ("Preg expected for source operand");

	  if ((yyvsp[-4].r0).r0)
	    tmp = unary (Expr_Op_Type_NEG, tmp);

	  if (IS_RELOC ((yyvsp[-3].expr)))
	    return yyerror ("Plain symbol used as offset");

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
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-5].reg), &(yyvsp[0].reg), 1, 0, ispreg ? 1 : 0, tmp);
	    }
	  else
	    return yyerror ("Displacement out of range");
	}
#line 5725 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 185:
#line 3136 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  Expr_Node *tmp = (yyvsp[-2].expr);
	  if (!IS_DREG ((yyvsp[-8].reg)))
	    return yyerror ("Dreg expected for destination operand");
	  if (!IS_PREG ((yyvsp[-4].reg)))
	    return yyerror ("Preg expected in address");

	  if ((yyvsp[-3].r0).r0)
	    tmp = unary (Expr_Op_Type_NEG, tmp);

	  if (IS_RELOC ((yyvsp[-2].expr)))
	    return yyerror ("Plain symbol used as offset");

	  if (in_range_p (tmp, 0, 30, 1))
	    {
	      notethat ("LDSTii: dregs = W [ pregs + uimm5m2 ] (.)\n");
	      (yyval.instr) = LDSTII (&(yyvsp[-4].reg), &(yyvsp[-8].reg), tmp, 0, 1 << (yyvsp[0].r0).r0);
	    }
	  else if (in_range_p (tmp, -65536, 65535, 1))
	    {
	      notethat ("LDSTidxI: dregs = W [ pregs + imm17m2 ] (.)\n");
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-4].reg), &(yyvsp[-8].reg), 0, 1, (yyvsp[0].r0).r0, tmp);
	    }
	  else
	    return yyerror ("Displacement out of range");
	}
#line 5756 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 186:
#line 3164 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-6].reg)))
	    return yyerror ("Dreg expected for source operand");
	  if ((yyvsp[-1].modcodes).x0 == 2)
	    {
	      if (!IS_IREG ((yyvsp[-2].reg)) && !IS_PREG ((yyvsp[-2].reg)))
		return yyerror ("Ireg or Preg expected in address");
	    }
	  else if (!IS_IREG ((yyvsp[-2].reg)))
	    return yyerror ("Ireg expected in address");

	  if (IS_IREG ((yyvsp[-2].reg)))
	    {
	      notethat ("dspLDST: dregs_half = W [ iregs <post_op> ]\n");
	      (yyval.instr) = DSPLDST(&(yyvsp[-2].reg), 1 + IS_H ((yyvsp[-6].reg)), &(yyvsp[-6].reg), (yyvsp[-1].modcodes).x0, 0);
	    }
	  else
	    {
	      notethat ("LDSTpmod: dregs_half = W [ pregs <post_op> ]\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-2].reg), &(yyvsp[-6].reg), &(yyvsp[-2].reg), 1 + IS_H ((yyvsp[-6].reg)), 0);
	    }
	}
#line 5783 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 187:
#line 3189 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-7].reg)))
	    return yyerror ("Dreg expected for destination operand");
	  if (!IS_PREG ((yyvsp[-3].reg)))
	    return yyerror ("Preg expected in address");

	  notethat ("LDST: dregs = W [ pregs <post_op> ] (.)\n");
	  (yyval.instr) = LDST (&(yyvsp[-3].reg), &(yyvsp[-7].reg), (yyvsp[-2].modcodes).x0, 1, (yyvsp[0].r0).r0, 0);
	}
#line 5797 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 188:
#line 3200 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-8].reg)))
	    return yyerror ("Dreg expected for destination operand");
	  if (!IS_PREG ((yyvsp[-4].reg)) || !IS_PREG ((yyvsp[-2].reg)))
	    return yyerror ("Preg expected in address");

	  notethat ("LDSTpmod: dregs = W [ pregs ++ pregs ] (.)\n");
	  (yyval.instr) = LDSTPMOD (&(yyvsp[-4].reg), &(yyvsp[-8].reg), &(yyvsp[-2].reg), 3, (yyvsp[0].r0).r0);
	}
#line 5811 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 189:
#line 3211 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-7].reg)))
	    return yyerror ("Dreg expected for destination operand");
	  if (!IS_PREG ((yyvsp[-3].reg)) || !IS_PREG ((yyvsp[-1].reg)))
	    return yyerror ("Preg expected in address");

	  notethat ("LDSTpmod: dregs_half = W [ pregs ++ pregs ]\n");
	  (yyval.instr) = LDSTPMOD (&(yyvsp[-3].reg), &(yyvsp[-7].reg), &(yyvsp[-1].reg), 1 + IS_H ((yyvsp[-7].reg)), 0);
	}
#line 5825 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 190:
#line 3222 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_IREG ((yyvsp[-4].reg)) && !IS_PREG ((yyvsp[-4].reg)))
	    return yyerror ("Ireg or Preg expected in address");
	  else if (IS_IREG ((yyvsp[-4].reg)) && !IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg expected for source operand");
	  else if (IS_PREG ((yyvsp[-4].reg)) && !IS_DREG ((yyvsp[0].reg)) && !IS_PREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg or Preg expected for source operand");

	  if (IS_IREG ((yyvsp[-4].reg)))
	    {
	      notethat ("dspLDST: [ iregs <post_op> ] = dregs\n");
	      (yyval.instr) = DSPLDST(&(yyvsp[-4].reg), 0, &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 1);
	    }
	  else if (IS_DREG ((yyvsp[0].reg)))
	    {
	      notethat ("LDST: [ pregs <post_op> ] = dregs\n");
	      (yyval.instr) = LDST (&(yyvsp[-4].reg), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 0, 0, 1);
	    }
	  else
	    {
	      notethat ("LDST: [ pregs <post_op> ] = pregs\n");
	      (yyval.instr) = LDST (&(yyvsp[-4].reg), &(yyvsp[0].reg), (yyvsp[-3].modcodes).x0, 0, 1, 1);
	    }
	}
#line 5854 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 191:
#line 3248 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg expected for source operand");

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
	    return yyerror ("Preg ++ Preg or Ireg ++ Mreg expected in address");
	}
#line 5876 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 192:
#line 3267 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dreg expected for source operand");

	  if (IS_PREG ((yyvsp[-5].reg)) && IS_PREG ((yyvsp[-3].reg)))
	    {
	      notethat ("LDSTpmod: W [ pregs ++ pregs ] = dregs_half\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-5].reg), &(yyvsp[0].reg), &(yyvsp[-3].reg), 1 + IS_H ((yyvsp[0].reg)), 1);
	    }
	  else
	    return yyerror ("Preg ++ Preg expected in address");
	}
#line 5893 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 193:
#line 3281 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  Expr_Node *tmp = (yyvsp[-2].expr);
	  if (!IS_DREG ((yyvsp[-8].reg)))
	    return yyerror ("Dreg expected for destination operand");
	  if (!IS_PREG ((yyvsp[-4].reg)))
	    return yyerror ("Preg expected in address");

	  if ((yyvsp[-3].r0).r0)
	    tmp = unary (Expr_Op_Type_NEG, tmp);

	  if (IS_RELOC ((yyvsp[-2].expr)))
	    return yyerror ("Plain symbol used as offset");

	  if (in_range_p (tmp, -32768, 32767, 0))
	    {
	      notethat ("LDSTidxI: dregs = B [ pregs + imm16 ] (%c)\n",
		       (yyvsp[0].r0).r0 ? 'X' : 'Z');
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-4].reg), &(yyvsp[-8].reg), 0, 2, (yyvsp[0].r0).r0, tmp);
	    }
	  else
	    return yyerror ("Displacement out of range");
	}
#line 5920 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 194:
#line 3305 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-7].reg)))
	    return yyerror ("Dreg expected for destination operand");
	  if (!IS_PREG ((yyvsp[-3].reg)))
	    return yyerror ("Preg expected in address");

	  notethat ("LDST: dregs = B [ pregs <post_op> ] (%c)\n",
		    (yyvsp[0].r0).r0 ? 'X' : 'Z');
	  (yyval.instr) = LDST (&(yyvsp[-3].reg), &(yyvsp[-7].reg), (yyvsp[-2].modcodes).x0, 2, (yyvsp[0].r0).r0, 0);
	}
#line 5935 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 195:
#line 3317 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-6].reg)))
	    return yyerror ("Dreg expected for destination operand");

	  if (IS_IREG ((yyvsp[-3].reg)) && IS_MREG ((yyvsp[-1].reg)))
	    {
	      notethat ("dspLDST: dregs = [ iregs ++ mregs ]\n");
	      (yyval.instr) = DSPLDST(&(yyvsp[-3].reg), (yyvsp[-1].reg).regno & CODE_MASK, &(yyvsp[-6].reg), 3, 0);
	    }
	  else if (IS_PREG ((yyvsp[-3].reg)) && IS_PREG ((yyvsp[-1].reg)))
	    {
	      notethat ("LDSTpmod: dregs = [ pregs ++ pregs ]\n");
	      (yyval.instr) = LDSTPMOD (&(yyvsp[-3].reg), &(yyvsp[-6].reg), &(yyvsp[-1].reg), 0, 0);
	    }
	  else
	    return yyerror ("Preg ++ Preg or Ireg ++ Mreg expected in address");
	}
#line 5957 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 196:
#line 3336 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  Expr_Node *tmp = (yyvsp[-1].expr);
	  int ispreg = IS_PREG ((yyvsp[-6].reg));
	  int isgot = IS_RELOC((yyvsp[-1].expr));

	  if (!IS_PREG ((yyvsp[-3].reg)))
	    return yyerror ("Preg expected in address");

	  if (!IS_DREG ((yyvsp[-6].reg)) && !ispreg)
	    return yyerror ("Dreg or Preg expected for destination operand");

	  if (tmp->type == Expr_Node_Reloc
	      && strcmp (tmp->value.s_value,
			 "_current_shared_library_p5_offset_") != 0)
	    return yyerror ("Plain symbol used as offset");

	  if ((yyvsp[-2].r0).r0)
	    tmp = unary (Expr_Op_Type_NEG, tmp);

	  if (isgot)
	    {
	      notethat ("LDSTidxI: dpregs = [ pregs + sym@got ]\n");
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-3].reg), &(yyvsp[-6].reg), 0, 0, ispreg ? 1 : 0, tmp);
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
	      (yyval.instr) = LDSTIDXI (&(yyvsp[-3].reg), &(yyvsp[-6].reg), 0, 0, ispreg ? 1 : 0, tmp);

	    }
	  else
	    return yyerror ("Displacement out of range");
	}
#line 6006 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 197:
#line 3382 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_IREG ((yyvsp[-2].reg)) && !IS_PREG ((yyvsp[-2].reg)))
	    return yyerror ("Ireg or Preg expected in address");
	  else if (IS_IREG ((yyvsp[-2].reg)) && !IS_DREG ((yyvsp[-5].reg)))
	    return yyerror ("Dreg expected in destination operand");
	  else if (IS_PREG ((yyvsp[-2].reg)) && !IS_DREG ((yyvsp[-5].reg)) && !IS_PREG ((yyvsp[-5].reg))
		   && ((yyvsp[-2].reg).regno != REG_SP || !IS_ALLREG ((yyvsp[-5].reg)) || (yyvsp[-1].modcodes).x0 != 0))
	    return yyerror ("Dreg or Preg expected in destination operand");

	  if (IS_IREG ((yyvsp[-2].reg)))
	    {
	      notethat ("dspLDST: dregs = [ iregs <post_op> ]\n");
	      (yyval.instr) = DSPLDST (&(yyvsp[-2].reg), 0, &(yyvsp[-5].reg), (yyvsp[-1].modcodes).x0, 0);
	    }
	  else if (IS_DREG ((yyvsp[-5].reg)))
	    {
	      notethat ("LDST: dregs = [ pregs <post_op> ]\n");
	      (yyval.instr) = LDST (&(yyvsp[-2].reg), &(yyvsp[-5].reg), (yyvsp[-1].modcodes).x0, 0, 0, 0);
	    }
	  else if (IS_PREG ((yyvsp[-5].reg)))
	    {
	      if (REG_SAME ((yyvsp[-5].reg), (yyvsp[-2].reg)) && (yyvsp[-1].modcodes).x0 != 2)
		return yyerror ("Pregs can't be same");

	      notethat ("LDST: pregs = [ pregs <post_op> ]\n");
	      (yyval.instr) = LDST (&(yyvsp[-2].reg), &(yyvsp[-5].reg), (yyvsp[-1].modcodes).x0, 0, 1, 0);
	    }
	  else
	    {
	      notethat ("PushPopReg: allregs = [ SP ++ ]\n");
	      (yyval.instr) = PUSHPOPREG (&(yyvsp[-5].reg), 0);
	    }
	}
#line 6044 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 198:
#line 3419 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 6063 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 199:
#line 3435 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 6085 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 200:
#line 3454 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 6102 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 201:
#line 3468 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 6124 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 202:
#line 3487 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 6141 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 203:
#line 3503 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_URANGE (16, (yyvsp[0].expr), 0, 4))
	    (yyval.instr) = LINKAGE (0, uimm16s4 ((yyvsp[0].expr)));
	  else
	    return yyerror ("Bad constant for LINK");
	}
#line 6152 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 204:
#line 3511 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
		notethat ("linkage: UNLINK\n");
		(yyval.instr) = LINKAGE (1, 0);
	}
#line 6161 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 205:
#line 3520 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_PCREL4 ((yyvsp[-4].expr)) && IS_LPPCREL10 ((yyvsp[-2].expr)) && IS_CREG ((yyvsp[0].reg)))
	    {
	      notethat ("LoopSetup: LSETUP (pcrel4 , lppcrel10 ) counters\n");
	      (yyval.instr) = LOOPSETUP ((yyvsp[-4].expr), &(yyvsp[0].reg), 0, (yyvsp[-2].expr), 0);
	    }
	  else
	    return yyerror ("Bad register or values for LSETUP");

	}
#line 6176 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 206:
#line 3531 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 6191 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 207:
#line 3543 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 6207 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 208:
#line 3557 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_RELOC ((yyvsp[-1].expr)))
	    return yyerror ("Invalid expression in loop statement");
	  if (!IS_CREG ((yyvsp[0].reg)))
            return yyerror ("Invalid loop counter register");
	(yyval.instr) = bfin_gen_loop ((yyvsp[-1].expr), &(yyvsp[0].reg), 0, 0);
	}
#line 6219 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 209:
#line 3565 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_RELOC ((yyvsp[-3].expr)) && IS_PREG ((yyvsp[0].reg)) && IS_CREG ((yyvsp[-2].reg)))
	    {
	      notethat ("Loop: LOOP expr counters = pregs\n");
	      (yyval.instr) = bfin_gen_loop ((yyvsp[-3].expr), &(yyvsp[-2].reg), 1, &(yyvsp[0].reg));
	    }
	  else
	    return yyerror ("Bad register or values for LOOP");
	}
#line 6233 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 210:
#line 3575 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_RELOC ((yyvsp[-5].expr)) && IS_PREG ((yyvsp[-2].reg)) && IS_CREG ((yyvsp[-4].reg)) && EXPR_VALUE ((yyvsp[0].expr)) == 1)
	    {
	      notethat ("Loop: LOOP expr counters = pregs >> 1\n");
	      (yyval.instr) = bfin_gen_loop ((yyvsp[-5].expr), &(yyvsp[-4].reg), 3, &(yyvsp[-2].reg));
	    }
	  else
	    return yyerror ("Bad register or values for LOOP");
	}
#line 6247 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 211:
#line 3587 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  Expr_Node_Value val;
	  val.i_value = (yyvsp[0].value);
	  Expr_Node *tmp = Expr_Node_Create (Expr_Node_Constant, val, NULL, NULL);
	  bfin_loop_attempt_create_label (tmp, 1);
	  if (!IS_RELOC (tmp))
	    return yyerror ("Invalid expression in LOOP_BEGIN statement");
	  bfin_loop_beginend (tmp, 1);
	  (yyval.instr) = 0;
	}
#line 6262 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 212:
#line 3598 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_RELOC ((yyvsp[0].expr)))
	    return yyerror ("Invalid expression in LOOP_BEGIN statement");

	  bfin_loop_beginend ((yyvsp[0].expr), 1);
	  (yyval.instr) = 0;
	}
#line 6274 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 213:
#line 3608 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  Expr_Node_Value val;
	  val.i_value = (yyvsp[0].value);
	  Expr_Node *tmp = Expr_Node_Create (Expr_Node_Constant, val, NULL, NULL);
	  bfin_loop_attempt_create_label (tmp, 1);
	  if (!IS_RELOC (tmp))
	    return yyerror ("Invalid expression in LOOP_END statement");
	  bfin_loop_beginend (tmp, 0);
	  (yyval.instr) = 0;
	}
#line 6289 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 214:
#line 3619 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_RELOC ((yyvsp[0].expr)))
	    return yyerror ("Invalid expression in LOOP_END statement");

	  bfin_loop_beginend ((yyvsp[0].expr), 0);
	  (yyval.instr) = 0;
	}
#line 6301 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 215:
#line 3630 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("psedoDEBUG: ABORT\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, 3, 0);
	}
#line 6310 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 216:
#line 3636 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("pseudoDEBUG: DBG\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, 7, 0);
	}
#line 6319 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 217:
#line 3641 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("pseudoDEBUG: DBG REG_A\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, IS_A1 ((yyvsp[0].reg)), 0);
	}
#line 6328 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 218:
#line 3646 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("pseudoDEBUG: DBG allregs\n");
	  (yyval.instr) = bfin_gen_pseudodbg (0, (yyvsp[0].reg).regno & CODE_MASK, ((yyvsp[0].reg).regno & CLASS_MASK) >> 4);
	}
#line 6337 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 219:
#line 3652 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[-1].reg)))
	    return yyerror ("Dregs expected");
	  notethat ("pseudoDEBUG: DBGCMPLX (dregs )\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, 6, ((yyvsp[-1].reg).regno & CODE_MASK) >> 4);
	}
#line 6348 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 220:
#line 3660 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("psedoDEBUG: DBGHALT\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, 5, 0);
	}
#line 6357 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 221:
#line 3666 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("psedoDEBUG: HLT\n");
	  (yyval.instr) = bfin_gen_pseudodbg (3, 4, 0);
	}
#line 6366 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 222:
#line 3672 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("pseudodbg_assert: DBGA (regs_lo/hi , uimm16 )\n");
	  (yyval.instr) = bfin_gen_pseudodbg_assert (IS_H ((yyvsp[-3].reg)), &(yyvsp[-3].reg), uimm16 ((yyvsp[-1].expr)));
	}
#line 6375 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 223:
#line 3678 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("pseudodbg_assert: DBGAH (regs , uimm16 )\n");
	  (yyval.instr) = bfin_gen_pseudodbg_assert (3, &(yyvsp[-3].reg), uimm16 ((yyvsp[-1].expr)));
	}
#line 6384 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 224:
#line 3684 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  notethat ("psedodbg_assert: DBGAL (regs , uimm16 )\n");
	  (yyval.instr) = bfin_gen_pseudodbg_assert (2, &(yyvsp[-3].reg), uimm16 ((yyvsp[-1].expr)));
	}
#line 6393 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 225:
#line 3690 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_UIMM ((yyvsp[0].expr), 8))
	    return yyerror ("Constant out of range");
	  notethat ("psedodbg_assert: OUTC uimm8\n");
	  (yyval.instr) = bfin_gen_pseudochr (uimm8 ((yyvsp[0].expr)));
	}
#line 6404 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 226:
#line 3698 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (!IS_DREG ((yyvsp[0].reg)))
	    return yyerror ("Dregs expected");
	  notethat ("psedodbg_assert: OUTC dreg\n");
	  (yyval.instr) = bfin_gen_pseudodbg (2, (yyvsp[0].reg).regno & CODE_MASK, 0);
	}
#line 6415 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 227:
#line 3712 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.reg) = (yyvsp[0].reg);
	}
#line 6423 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 228:
#line 3716 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.reg) = (yyvsp[0].reg);
	}
#line 6431 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 229:
#line 3725 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.mod).MM = 0;
	(yyval.mod).mod = 0;
	}
#line 6440 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 230:
#line 3730 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.mod).MM = 1;
	(yyval.mod).mod = (yyvsp[-1].value);
	}
#line 6449 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 231:
#line 3735 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.mod).MM = 1;
	(yyval.mod).mod = (yyvsp[-3].value);
	}
#line 6458 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 232:
#line 3740 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.mod).MM = 0;
	(yyval.mod).mod = (yyvsp[-1].value);
	}
#line 6467 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 233:
#line 3745 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.mod).MM = 1;
	(yyval.mod).mod = 0;
	}
#line 6476 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 234:
#line 3752 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6484 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 235:
#line 3756 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6492 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 236:
#line 3762 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	}
#line 6501 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 237:
#line 3767 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 1;
	(yyval.modcodes).x0 = 0;
	}
#line 6510 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 238:
#line 3772 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 1;
	}
#line 6519 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 239:
#line 3777 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 1;
	(yyval.modcodes).x0 = 1;
	}
#line 6528 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 240:
#line 3785 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6536 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 241:
#line 3789 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6544 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 242:
#line 3795 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	}
#line 6553 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 243:
#line 3800 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = (yyvsp[-1].modcodes).s0;
	(yyval.modcodes).x0 = (yyvsp[-1].modcodes).x0;
	}
#line 6562 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 244:
#line 3807 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	(yyval.modcodes).aop = 0;
	}
#line 6572 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 245:
#line 3813 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	(yyval.modcodes).aop = 1;
	}
#line 6582 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 246:
#line 3819 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 1;
	(yyval.modcodes).x0 = 0;
	(yyval.modcodes).aop = 1;
	}
#line 6592 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 247:
#line 3827 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	}
#line 6602 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 248:
#line 3833 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 2 + (yyvsp[-1].r0).r0;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).x0 = 0;
	}
#line 6612 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 249:
#line 3839 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = (yyvsp[-1].modcodes).s0;
	(yyval.modcodes).x0 = (yyvsp[-1].modcodes).x0;
	}
#line 6622 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 250:
#line 3845 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 2 + (yyvsp[-3].r0).r0;
	(yyval.modcodes).s0 = (yyvsp[-1].modcodes).s0;
	(yyval.modcodes).x0 = (yyvsp[-1].modcodes).x0;
	}
#line 6632 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 251:
#line 3851 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 2 + (yyvsp[-1].r0).r0;
	(yyval.modcodes).s0 = (yyvsp[-3].modcodes).s0;
	(yyval.modcodes).x0 = (yyvsp[-3].modcodes).x0;
	}
#line 6642 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 252:
#line 3859 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6650 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 253:
#line 3863 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6658 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 254:
#line 3867 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6666 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 255:
#line 3873 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6674 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 256:
#line 3877 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6682 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 257:
#line 3881 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6690 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 258:
#line 3887 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).aop = 0;
	}
#line 6700 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 259:
#line 3893 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).aop = 3;
	}
#line 6710 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 260:
#line 3899 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 1;
	(yyval.modcodes).aop = 3;
	}
#line 6720 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 261:
#line 3905 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 0;
	(yyval.modcodes).aop = 3;
	}
#line 6730 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 262:
#line 3911 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 1;
	}
#line 6739 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 263:
#line 3916 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 1;
	}
#line 6748 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 264:
#line 3923 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6756 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 265:
#line 3927 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6764 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 266:
#line 3933 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 0;
	}
#line 6772 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 267:
#line 3937 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).s0 = 1;
	}
#line 6780 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 268:
#line 3944 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6788 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 269:
#line 3948 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6796 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 270:
#line 3952 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 3;
	}
#line 6804 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 271:
#line 3956 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 2;
	}
#line 6812 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 272:
#line 3962 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6820 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 273:
#line 3966 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6828 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 274:
#line 3973 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).r0 = 0;
	(yyval.modcodes).s0 = 1;
	}
#line 6837 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 275:
#line 3978 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	if ((yyvsp[-1].value) != M_T)
	  return yyerror ("Bad modifier");
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 0;
	}
#line 6848 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 276:
#line 3985 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	if ((yyvsp[-3].value) != M_T)
	  return yyerror ("Bad modifier");
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 1;
	}
#line 6859 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 277:
#line 3992 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	if ((yyvsp[-1].value) != M_T)
	  return yyerror ("Bad modifier");
	(yyval.modcodes).r0 = 1;
	(yyval.modcodes).s0 = 1;
	}
#line 6870 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 278:
#line 4004 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6878 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 279:
#line 4008 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6886 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 280:
#line 4012 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 2;
	}
#line 6894 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 281:
#line 4018 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6902 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 282:
#line 4022 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((yyvsp[-1].value) == M_W32)
	    (yyval.r0).r0 = 1;
	  else
	    return yyerror ("Only (W32) allowed");
	}
#line 6913 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 283:
#line 4031 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6921 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 284:
#line 4035 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if ((yyvsp[-1].value) == M_IU)
	    (yyval.r0).r0 = 3;
	  else
	    return yyerror ("(IU) expected");
	}
#line 6932 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 285:
#line 4044 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.reg) = (yyvsp[-1].reg);
	}
#line 6940 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 286:
#line 4050 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.reg) = (yyvsp[-2].reg);
	}
#line 6948 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 287:
#line 4059 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6956 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 288:
#line 4063 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6964 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 289:
#line 4070 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 6972 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 290:
#line 4074 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 6980 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 291:
#line 4078 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 2;
	}
#line 6988 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 292:
#line 4082 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 3;
	}
#line 6996 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 293:
#line 4089 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 7004 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 294:
#line 4093 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 7012 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 295:
#line 4100 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 1;	/* HL.  */
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 0;	/* aop.  */
	}
#line 7023 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 296:
#line 4108 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 1;	/* HL.  */
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 1;	/* aop.  */
	}
#line 7034 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 297:
#line 4116 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 0;	/* HL.  */
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 0;	/* aop.  */
	}
#line 7045 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 298:
#line 4124 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 0;	/* HL.  */
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 1;
	}
#line 7056 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 299:
#line 4132 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 1;	/* HL.  */
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 0;	/* aop.  */
	}
#line 7067 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 300:
#line 4139 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 1;	/* HL.  */
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 1;	/* aop.  */
	}
#line 7078 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 301:
#line 4146 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 0;	/* HL.  */
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 0;	/* aop.  */
	}
#line 7089 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 302:
#line 4154 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 0;	/* HL.  */
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* x.  */
	  (yyval.modcodes).aop = 1;	/* aop.  */
	}
#line 7100 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 303:
#line 4164 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* HL.  */
	}
#line 7109 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 304:
#line 4169 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).s0 = 0;	/* s.  */
	  (yyval.modcodes).x0 = 1;	/* HL.  */
	}
#line 7118 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 305:
#line 4174 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 0;	/* HL.  */
	}
#line 7127 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 306:
#line 4179 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).s0 = 1;	/* s.  */
	  (yyval.modcodes).x0 = 1;	/* HL.  */
	}
#line 7136 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 307:
#line 4186 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).x0 = 2;
	}
#line 7144 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 308:
#line 4190 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).x0 = 0;
	}
#line 7152 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 309:
#line 4194 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.modcodes).x0 = 1;
	}
#line 7160 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 310:
#line 4203 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.reg) = (yyvsp[-1].reg);
	}
#line 7168 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 311:
#line 4210 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.reg) = (yyvsp[-1].reg);
	}
#line 7176 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 312:
#line 4217 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.reg) = (yyvsp[-1].reg);
	}
#line 7184 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 313:
#line 4224 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 7203 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 314:
#line 4239 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.macfunc) = (yyvsp[0].macfunc);
	  (yyval.macfunc).w = 0; (yyval.macfunc).P = 0;
	  (yyval.macfunc).dst.regno = 0;
	}
#line 7213 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 315:
#line 4245 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 7229 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 316:
#line 4258 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 7245 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 317:
#line 4271 "./config/bfin-parse.y" /* yacc.c:1646  */
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
#line 7264 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 318:
#line 4289 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.macfunc).n = IS_A1 ((yyvsp[-1].reg));
	  (yyval.macfunc).op = 0;
	  (yyval.macfunc).s0 = (yyvsp[0].macfunc).s0;
	  (yyval.macfunc).s1 = (yyvsp[0].macfunc).s1;
	}
#line 7275 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 319:
#line 4296 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.macfunc).n = IS_A1 ((yyvsp[-1].reg));
	  (yyval.macfunc).op = 1;
	  (yyval.macfunc).s0 = (yyvsp[0].macfunc).s0;
	  (yyval.macfunc).s1 = (yyvsp[0].macfunc).s1;
	}
#line 7286 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 320:
#line 4303 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.macfunc).n = IS_A1 ((yyvsp[-1].reg));
	  (yyval.macfunc).op = 2;
	  (yyval.macfunc).s0 = (yyvsp[0].macfunc).s0;
	  (yyval.macfunc).s1 = (yyvsp[0].macfunc).s1;
	}
#line 7297 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 321:
#line 4313 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  if (IS_DREG ((yyvsp[-2].reg)) && IS_DREG ((yyvsp[0].reg)))
	    {
	      (yyval.macfunc).s0 = (yyvsp[-2].reg);
              (yyval.macfunc).s1 = (yyvsp[0].reg);
	    }
	  else
	    return yyerror ("Dregs expected");
	}
#line 7311 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 322:
#line 4326 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 0;
	}
#line 7319 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 323:
#line 4330 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 1;
	}
#line 7327 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 324:
#line 4334 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 2;
	}
#line 7335 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 325:
#line 4338 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.r0).r0 = 3;
	}
#line 7343 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 326:
#line 4345 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = (yyvsp[0].reg).regno;
	  (yyval.modcodes).x0 = (yyvsp[-1].r0).r0;
	  (yyval.modcodes).s0 = 0;
	}
#line 7353 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 327:
#line 4351 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 0x18;
	  (yyval.modcodes).x0 = (yyvsp[-1].r0).r0;
	  (yyval.modcodes).s0 = 0;
	}
#line 7363 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 328:
#line 4357 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = (yyvsp[-2].reg).regno;
	  (yyval.modcodes).x0 = (yyvsp[-1].r0).r0;
	  (yyval.modcodes).s0 = 1;
	}
#line 7373 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 329:
#line 4363 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	  (yyval.modcodes).r0 = 0x18;
	  (yyval.modcodes).x0 = (yyvsp[-1].r0).r0;
	  (yyval.modcodes).s0 = 1;
	}
#line 7383 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 330:
#line 4373 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	Expr_Node_Value val;
	val.s_value = S_GET_NAME((yyvsp[0].symbol));
	(yyval.expr) = Expr_Node_Create (Expr_Node_Reloc, val, NULL, NULL);
	}
#line 7393 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 331:
#line 4382 "./config/bfin-parse.y" /* yacc.c:1646  */
    { (yyval.value) = BFD_RELOC_BFIN_GOT; }
#line 7399 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 332:
#line 4384 "./config/bfin-parse.y" /* yacc.c:1646  */
    { (yyval.value) = BFD_RELOC_BFIN_GOT17M4; }
#line 7405 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 333:
#line 4386 "./config/bfin-parse.y" /* yacc.c:1646  */
    { (yyval.value) = BFD_RELOC_BFIN_FUNCDESC_GOT17M4; }
#line 7411 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 334:
#line 4390 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	Expr_Node_Value val;
	val.i_value = (yyvsp[0].value);
	(yyval.expr) = Expr_Node_Create (Expr_Node_GOT_Reloc, val, (yyvsp[-2].expr), NULL);
	}
#line 7421 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 335:
#line 4398 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
#line 7429 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 336:
#line 4402 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
#line 7437 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 337:
#line 4409 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = (yyvsp[-2].expr);
	}
#line 7445 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 338:
#line 4415 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	Expr_Node_Value val;
	val.i_value = (yyvsp[0].value);
	(yyval.expr) = Expr_Node_Create (Expr_Node_Constant, val, NULL, NULL);
	}
#line 7455 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 339:
#line 4421 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
#line 7463 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 340:
#line 4425 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = (yyvsp[-1].expr);
	}
#line 7471 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 341:
#line 4429 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = unary (Expr_Op_Type_COMP, (yyvsp[0].expr));
	}
#line 7479 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 342:
#line 4433 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = unary (Expr_Op_Type_NEG, (yyvsp[0].expr));
	}
#line 7487 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 343:
#line 4439 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
#line 7495 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 344:
#line 4445 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_Mult, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7503 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 345:
#line 4449 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_Div, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7511 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 346:
#line 4453 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_Mod, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7519 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 347:
#line 4457 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_Add, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7527 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 348:
#line 4461 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_Sub, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7535 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 349:
#line 4465 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_Lshift, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7543 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 350:
#line 4469 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_Rshift, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7551 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 351:
#line 4473 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_BAND, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7559 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 352:
#line 4477 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_LOR, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7567 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 353:
#line 4481 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = binary (Expr_Op_Type_BOR, (yyvsp[-2].expr), (yyvsp[0].expr));
	}
#line 7575 "bfin-parse.c" /* yacc.c:1646  */
    break;

  case 354:
#line 4485 "./config/bfin-parse.y" /* yacc.c:1646  */
    {
	(yyval.expr) = (yyvsp[0].expr);
	}
#line 7583 "bfin-parse.c" /* yacc.c:1646  */
    break;


#line 7587 "bfin-parse.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 4491 "./config/bfin-parse.y" /* yacc.c:1906  */


EXPR_T
mkexpr (int x, SYMBOL_T s)
{
  EXPR_T e = XNEW (struct expression_cell);
  e->value = x;
  EXPR_SYMBOL(e) = s;
  return e;
}

static int
value_match (Expr_Node *exp, int sz, int sign, int mul, int issigned)
{
  int umax = (1 << sz) - 1;
  int min = -(1 << (sz - 1));
  int max = (1 << (sz - 1)) - 1;

  int v = (EXPR_VALUE (exp)) & 0xffffffff;

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
	  error ("%s:%d: Internal assembler error\n", __FILE__, __LINE__);
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
	  error ("%s:%d: Internal assembler error\n", __FILE__, __LINE__);
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
notethat (const char *format, ...)
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

