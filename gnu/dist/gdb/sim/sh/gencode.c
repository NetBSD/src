/* Simulator/Opcode generator for the Hitachi Super-H architecture.

   Written by Steve Chamberlain of Cygnus Support.
   sac@cygnus.com

   This file is part of SH sim


		THIS SOFTWARE IS NOT COPYRIGHTED

   Cygnus offers the following for use in the public domain.  Cygnus
   makes no warranty with regard to the software or it's performance
   and the user accepts the software "AS IS" with all faults.

   CYGNUS DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
   THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

*/

/* This program generates the opcode table for the assembler and
   the simulator code

   -t		prints a pretty table for the assembler manual
   -s		generates the simulator code jump table
   -d		generates a define table
   -x		generates the simulator code switch statement
   default 	used to generate the opcode tables

*/

#include <stdio.h>

#define MAX_NR_STUFF 42

typedef struct
{
  char *defs;
  char *refs;
  char *name;
  char *code;
  char *stuff[MAX_NR_STUFF];
  int index;
} op;


op tab[] =
{

  { "n", "", "add #<imm>,<REG_N>", "0111nnnni8*1....",
    "R[n] += SEXT(i);",
    "if (i == 0) {",
    "  UNDEF(n); /* see #ifdef PARANOID */",
    "  break;",
    "}",
  },
  { "n", "mn", "add <REG_M>,<REG_N>", "0011nnnnmmmm1100",
    "R[n] += R[m];",
  },

  { "n", "mn", "addc <REG_M>,<REG_N>", "0011nnnnmmmm1110",
    "ult = R[n] + T;",
    "SET_SR_T (ult < R[n]);",
    "R[n] = ult + R[m];",
    "SET_SR_T (T || (R[n] < ult));",
  },

  { "n", "mn", "addv <REG_M>,<REG_N>", "0011nnnnmmmm1111",
    "ult = R[n] + R[m];",
    "SET_SR_T ((~(R[n] ^ R[m]) & (ult ^ R[n])) >> 31);",
    "R[n] = ult;",
  },

  { "0", "", "and #<imm>,R0", "11001001i8*1....",
    "R0 &= i;",
  },
  { "n", "nm", "and <REG_M>,<REG_N>", "0010nnnnmmmm1001",
    "R[n] &= R[m];",
  },
  { "", "0", "and.b #<imm>,@(R0,GBR)", "11001101i8*1....",
    "MA (1);",
    "WBAT (GBR + R0, RBAT (GBR + R0) & i);",
  },

  { "", "", "bf <bdisp8>", "10001011i8p1....",
    "if (!T) {",
    "  SET_NIP (PC + 4 + (SEXT(i) * 2));",
    "  cycles += 2;",
    "}",
  },

  { "", "", "bf.s <bdisp8>", "10001111i8p1....",
    "if (!T) {",
    "  SET_NIP (PC + 4 + (SEXT (i) * 2));",
    "  cycles += 2;",
    "  Delay_Slot (PC + 2);",
    "}",
  },

  { "", "", "bra <bdisp12>", "1010i12.........",
    "SET_NIP (PC + 4 + (SEXT12 (i) * 2));",
    "cycles += 2;",
    "Delay_Slot (PC + 2);",
  },

  { "", "n", "braf <REG_N>", "0000nnnn00100011",
    "SET_NIP (PC + 4 + R[n]);",
    "cycles += 2;",
    "Delay_Slot (PC + 2);",
  },

  { "", "", "bsr <bdisp12>", "1011i12.........",
    "PR = PH2T (PC + 4);",
    "SET_NIP (PC + 4 + (SEXT12 (i) * 2));",
    "cycles += 2;",
    "Delay_Slot (PC + 2);",
  },

  { "", "n", "bsrf <REG_N>", "0000nnnn00000011",
    "PR = PH2T (PC) + 4;",
    "SET_NIP (PC + 4 + R[n]);",
    "cycles += 2;",
    "Delay_Slot (PC + 2);",
  },

  { "", "", "bt <bdisp8>", "10001001i8p1....",
    "if (T) {",
    "  SET_NIP (PC + 4 + (SEXT (i) * 2));",
    "  cycles += 2;",
    "}",
  },

  { "", "", "bt.s <bdisp8>", "10001101i8p1....",
    "if (T) {",
    "  SET_NIP (PC + 4 + (SEXT (i) * 2));",
    "  cycles += 2;",
    "  Delay_Slot (PC + 2);",
    "}",
  },

  { "", "", "clrmac", "0000000000101000",
    "MACH = 0;",
    "MACL = 0;",
  },

  { "", "", "clrs", "0000000001001000",
    "SET_SR_S (0);",
  },

  { "", "", "clrt", "0000000000001000",
    "SET_SR_T (0);",
  },

  { "", "0", "cmp/eq #<imm>,R0", "10001000i8*1....",
    "SET_SR_T (R0 == SEXT (i));",
  },
  { "", "mn", "cmp/eq <REG_M>,<REG_N>", "0011nnnnmmmm0000",
    "SET_SR_T (R[n] == R[m]);",
  },
  { "", "mn", "cmp/ge <REG_M>,<REG_N>", "0011nnnnmmmm0011",
    "SET_SR_T (R[n] >= R[m]);",
  },
  { "", "mn", "cmp/gt <REG_M>,<REG_N>", "0011nnnnmmmm0111",
    "SET_SR_T (R[n] > R[m]);",
  },
  { "", "mn", "cmp/hi <REG_M>,<REG_N>", "0011nnnnmmmm0110",
    "SET_SR_T (UR[n] > UR[m]);",
  },
  { "", "mn", "cmp/hs <REG_M>,<REG_N>", "0011nnnnmmmm0010",
    "SET_SR_T (UR[n] >= UR[m]);",
  },
  { "", "n", "cmp/pl <REG_N>", "0100nnnn00010101",
    "SET_SR_T (R[n] > 0);",
  },
  { "", "n", "cmp/pz <REG_N>", "0100nnnn00010001",
    "SET_SR_T (R[n] >= 0);",
  },
  { "", "mn", "cmp/str <REG_M>,<REG_N>", "0010nnnnmmmm1100",
    "ult = R[n] ^ R[m];",
    "SET_SR_T (((ult & 0xff000000) == 0)",
    "          | ((ult & 0xff0000) == 0)",
    "          | ((ult & 0xff00) == 0)",
    "          | ((ult & 0xff) == 0));",
  },

  { "", "mn", "div0s <REG_M>,<REG_N>", "0010nnnnmmmm0111",
    "SET_SR_Q ((R[n] & sbit) != 0);",
    "SET_SR_M ((R[m] & sbit) != 0);",
    "SET_SR_T (M != Q);",
  },

  { "", "", "div0u", "0000000000011001",
    "SET_SR_M (0);",
    "SET_SR_Q (0);",
    "SET_SR_T (0);",
  },

  { "", "", "div1 <REG_M>,<REG_N>", "0011nnnnmmmm0100",
    "div1 (R, m, n/*, T*/);",
  },

  { "", "nm", "dmuls.l <REG_M>,<REG_N>", "0011nnnnmmmm1101",
    "dmul (1/*signed*/, R[n], R[m]);",
  },

  { "", "nm", "dmulu.l <REG_M>,<REG_N>", "0011nnnnmmmm0101",
    "dmul (0/*unsigned*/, R[n], R[m]);",
  },

  { "n", "n", "dt <REG_N>", "0100nnnn00010000",
    "R[n]--;",
    "SET_SR_T (R[n] == 0);",
  },

  { "n", "m", "exts.b <REG_M>,<REG_N>", "0110nnnnmmmm1110",
    "R[n] = SEXT (R[m]);",
  },
  { "n", "m", "exts.w <REG_M>,<REG_N>", "0110nnnnmmmm1111",
    "R[n] = SEXTW (R[m]);",
  },

  { "n", "m", "extu.b <REG_M>,<REG_N>", "0110nnnnmmmm1100",
    "R[n] = (R[m] & 0xff);",
  },
  { "n", "m", "extu.w <REG_M>,<REG_N>", "0110nnnnmmmm1101",
    "R[n] = (R[m] & 0xffff);",
  },

  /* sh3e */
  { "", "", "fabs <FREG_N>", "1111nnnn01011101",
    "FP_UNARY (n, fabs);",
    "/* FIXME: FR(n) &= 0x7fffffff; */",
  },

  /* sh3e */
  { "", "", "fadd <FREG_M>,<FREG_N>", "1111nnnnmmmm0000",
    "FP_OP (n, +, m);",
  },

  /* sh3e */
  { "", "", "fcmp/eq <FREG_M>,<FREG_N>", "1111nnnnmmmm0100",
    "FP_CMP (n, ==, m);",
  },
  /* sh3e */
  { "", "", "fcmp/gt <FREG_M>,<FREG_N>", "1111nnnnmmmm0101",
    "FP_CMP (n, >, m);",
  },

  /* sh4 */
  { "", "", "fcnvds <DR_N>,FPUL", "1111nnnn10111101",
    "if (! FPSCR_PR || n & 1)",
    "  RAISE_EXCEPTION (SIGILL);",
    "else",
    "{",
    "  union",
    "  {",
    "    int i;",
    "    float f;",
    "  } u;",
    "  u.f = DR(n);",
    "  FPUL = u.i;",
    "}",
  },

  /* sh4 */
  { "", "", "fcnvsd FPUL,<DR_N>", "1111nnnn10101101",
    "if (! FPSCR_PR || n & 1)",
    "  RAISE_EXCEPTION (SIGILL);",
    "else",
    "{",
    "  union",
    "  {",
    "    int i;",
    "    float f;",
    "  } u;",
    "  u.i = FPUL;",
    "  SET_DR(n, u.f);",
    "}",
  },

  /* sh3e */
  { "", "", "fdiv <FREG_M>,<FREG_N>", "1111nnnnmmmm0011",
    "FP_OP (n, /, m);",
    "/* FIXME: check for DP and (n & 1) == 0? */",
  },

  /* sh4 */
  { "", "", "fipr <FV_M>,<FV_N>", "1111nnmm11101101",
    "/* FIXME: not implemented */",
    "RAISE_EXCEPTION (SIGILL);",
    "/* FIXME: check for DP and (n & 1) == 0? */",
  },

  /* sh3e */
  { "", "", "fldi0 <FREG_N>", "1111nnnn10001101",
    "SET_FR (n, (float)0.0);",
    "/* FIXME: check for DP and (n & 1) == 0? */",
  },

  /* sh3e */
  { "", "", "fldi1 <FREG_N>", "1111nnnn10011101",
    "SET_FR (n, (float)1.0);",
    "/* FIXME: check for DP and (n & 1) == 0? */",
  },

  /* sh3e */
  { "", "", "flds <FREG_N>,FPUL", "1111nnnn00011101",
    "  union",
    "  {",
    "    int i;",
    "    float f;",
    "  } u;",
    "  u.f = FR(n);",
    "  FPUL = u.i;",
  },

  /* sh3e */
  { "", "", "float FPUL,<FREG_N>", "1111nnnn00101101",
    /* sh4 */
    "if (FPSCR_PR)",
    "  SET_DR (n, (double)FPUL);",
    "else",
    "{",
    "  SET_FR (n, (float)FPUL);",
    "}",
  },

  /* sh3e */
  { "", "", "fmac <FREG_0>,<FREG_M>,<FREG_N>", "1111nnnnmmmm1110",
    "SET_FR (n, FR(m) * FR(0) + FR(n));",
    "/* FIXME: check for DP and (n & 1) == 0? */",
  },

  /* sh3e */
  { "", "", "fmov <FREG_M>,<FREG_N>", "1111nnnnmmmm1100",
    /* sh4 */
    "if (FPSCR_SZ) {",
    "  int ni = XD_TO_XF (n);",
    "  int mi = XD_TO_XF (m);",
    "  SET_XF (ni + 0, XF (mi + 0));",
    "  SET_XF (ni + 1, XF (mi + 1));",
    "}",
    "else",
    "{",
    "  SET_FR (n, FR (m));",
    "}",
  },
  /* sh3e */
  { "", "", "fmov.s <FREG_M>,@<REG_N>", "1111nnnnmmmm1010",
    /* sh4 */
    "if (FPSCR_SZ) {",
    "  MA (2);",
    "  WDAT (R[n], m);",
    "}",
    "else",
    "{",
    "  MA (1);",
    "  WLAT (R[n], FI(m));",
    "}",
  },
  /* sh3e */
  { "", "", "fmov.s @<REG_M>,<FREG_N>", "1111nnnnmmmm1000",
    /* sh4 */
    "if (FPSCR_SZ) {",
    "  MA (2);",
    "  RDAT (R[m], n);",
    "}",
    "else",
    "{",
    "  MA (1);",
    "  SET_FI(n, RLAT(R[m]));",
    "}",
  },
  /* sh3e */
  { "", "", "fmov.s @<REG_M>+,<FREG_N>", "1111nnnnmmmm1001",
    /* sh4 */
    "if (FPSCR_SZ) {",
    "  MA (2);",
    "  RDAT (R[m], n);",
    "  R[m] += 8;",
    "}",
    "else",
    "{",
    "  MA (1);",
    "  SET_FI (n, RLAT (R[m]));",
    "  R[m] += 4;",
    "}",
  },
  /* sh3e */
  { "", "", "fmov.s <FREG_M>,@-<REG_N>", "1111nnnnmmmm1011",
    /* sh4 */
    "if (FPSCR_SZ) {",
    "  MA (2);",
    "  R[n] -= 8;",
    "  WDAT (R[n], m);",
    "}",
    "else",
    "{",
    "  MA (1);",
    "  R[n] -= 4;",
    "  WLAT (R[n], FI(m));",
    "}",
  },
  /* sh3e */
  { "", "", "fmov.s @(R0,<REG_M>),<FREG_N>", "1111nnnnmmmm0110",
    /* sh4 */
    "if (FPSCR_SZ) {",
    "  MA (2);",
    "  RDAT (R[0]+R[m], n);",
    "}",
    "else",
    "{",
    "  MA (1);",
    "  SET_FI(n, RLAT(R[0] + R[m]));",
    "}",
  },
  /* sh3e */
  { "", "", "fmov.s <FREG_M>,@(R0,<REG_N>)", "1111nnnnmmmm0111",
    /* sh4 */
    "if (FPSCR_SZ) {",
    "  MA (2);",
    "  WDAT (R[0]+R[n], m);",
    "}",
    "else",
    "{",
    "  MA (1);",
    "  WLAT((R[0]+R[n]), FI(m));",
    "}",
  },

  /* sh4: See fmov instructions above for move to/from extended fp registers */

  /* sh3e */
  { "", "", "fmul <FREG_M>,<FREG_N>", "1111nnnnmmmm0010",
    "FP_OP(n, *, m);",
  },

  /* sh3e */
  { "", "", "fneg <FREG_N>", "1111nnnn01001101",
    "FP_UNARY(n, -);",
  },

  /* sh4 */
  { "", "", "frchg", "1111101111111101",
    "SET_FPSCR (GET_FPSCR() ^ FPSCR_MASK_FR);",
  },

  /* sh4 */
  { "", "", "fschg", "1111001111111101",
    "SET_FPSCR (GET_FPSCR() ^ FPSCR_MASK_SZ);",
  },

  /* sh3e */
  { "", "", "fsqrt <FREG_N>", "1111nnnn01101101",
    "FP_UNARY(n, sqrt);",
  },

  /* sh3e */
  { "", "", "fsub <FREG_M>,<FREG_N>", "1111nnnnmmmm0001",
    "FP_OP(n, -, m);",
  },

  /* sh3e */
  { "", "", "ftrc <FREG_N>, FPUL", "1111nnnn00111101",
    /* sh4 */
    "if (FPSCR_PR) {",
    "  if (DR(n) != DR(n)) /* NaN */",
    "    FPUL = 0x80000000;",
    "  else",
    "    FPUL =  (int)DR(n);",
    "}",
    "else",
    "if (FR(n) != FR(n)) /* NaN */",
    "  FPUL = 0x80000000;",
    "else",
    "  FPUL = (int)FR(n);",
  },

  /* sh3e */
  { "", "", "fsts FPUL,<FREG_N>", "1111nnnn00001101",
    "  union",
    "  {",
    "    int i;",
    "    float f;",
    "  } u;",
    "  u.i = FPUL;",
    "  SET_FR (n, u.f);",
  },

  { "", "n", "jmp @<REG_N>", "0100nnnn00101011",
    "SET_NIP (PT2H (R[n]));",
    "cycles += 2;",
    "Delay_Slot (PC + 2);",
  },

  { "", "n", "jsr @<REG_N>", "0100nnnn00001011",
    "PR = PH2T (PC + 4);",
    "if (~doprofile)",
    "  gotcall (PR, R[n]);",
    "SET_NIP (PT2H (R[n]));",
    "cycles += 2;",
    "Delay_Slot (PC + 2);",
  },

  { "", "n", "ldc <REG_N>,<CREG_M>", "0100nnnnmmmm1110",
    "CREG (m) = R[n];",
    "/* FIXME: user mode */",
  },
  { "", "n", "ldc <REG_N>,SR", "0100nnnn00001110",
    "SET_SR (R[n]);",
    "/* FIXME: user mode */",
  },
  { "", "n", "ldc <REG_N>,MOD", "0100nnnn01011110",
    "SET_MOD (R[n]);",
  },
#if 0
  { "", "n", "ldc <REG_N>,DBR", "0100nnnn11111010",
    "DBR = R[n];",
    "/* FIXME: user mode */",
  },
#endif
  { "", "n", "ldc.l @<REG_N>+,<CREG_M>", "0100nnnnmmmm0111",
    "MA (1);",
    "CREG (m) = RLAT (R[n]);",
    "R[n] += 4;",
    "/* FIXME: user mode */",
  },
  { "", "n", "ldc.l @<REG_N>+,SR", "0100nnnn00000111",
    "MA (1);",
    "SET_SR (RLAT (R[n]));",
    "R[n] += 4;",
    "/* FIXME: user mode */",
  },
  { "", "n", "ldc.l @<REG_N>+,MOD", "0100nnnn01010111",
    "MA (1);",
    "SET_MOD (RLAT (R[n]));",
    "R[n] += 4;",
  },
#if 0
  { "", "n", "ldc.l @<REG_N>+,DBR", "0100nnnn11110110",
    "MA (1);",
    "DBR = RLAT (R[n]);",
    "R[n] += 4;",
    "/* FIXME: user mode */",
  },
#endif

  /* sh-dsp */
  { "", "", "ldre @(<disp>,PC)", "10001110i8p1....",
    "RE = SEXT (i) * 2 + 4 + PH2T (PC);",
  },
  { "", "", "ldrs @(<disp>,PC)", "10001100i8p1....",
    "RS = SEXT (i) * 2 + 4 + PH2T (PC);",
  },

  { "", "n", "lds <REG_N>,<SREG_M>", "0100nnnnssss1010",
    "SREG (m) = R[n];",
  },
  { "", "n", "lds.l @<REG_N>+,<SREG_M>", "0100nnnnssss0110",
    "MA (1);",
    "SREG (m) = RLAT(R[n]);",
    "R[n] += 4;",
  },
  /* sh3e / sh-dsp (lds <REG_N>,DSR) */
  { "", "n", "lds <REG_N>,FPSCR", "0100nnnn01101010",
    "SET_FPSCR(R[n]);",
  },
  /* sh3e / sh-dsp (lds.l @<REG_N>+,DSR) */
  { "", "n", "lds.l @<REG_N>+,FPSCR", "0100nnnn01100110",
    "MA (1);",
    "SET_FPSCR (RLAT(R[n]));",
    "R[n] += 4;",
  },

  { "", "", "ldtlb", "0000000000111000",
    "/* FIXME: XXX*/ abort();",
  },

  { "", "nm", "mac.l @<REG_M>+,@<REG_N>+", "0000nnnnmmmm1111",
    "trap (255,R0,memory,maskl,maskw, endianw);",
    "/* FIXME: mac.l support */",
  },

  { "", "nm", "mac.w @<REG_M>+,@<REG_N>+", "0100nnnnmmmm1111",
    "macw(R0,memory,n,m,endianw);",
  },

  { "n", "", "mov #<imm>,<REG_N>", "1110nnnni8*1....",
    "R[n] = SEXT(i);",
  },
  { "n", "m", "mov <REG_M>,<REG_N>", "0110nnnnmmmm0011",
    "R[n] = R[m];",
  },

  { "0", "", "mov.b @(<disp>,GBR),R0", "11000100i8*1....",
    "MA (1);",
    "R0 = RSBAT (i + GBR);",
    "L (0);",
  },
  { "0", "m", "mov.b @(<disp>,<REG_M>),R0", "10000100mmmmi4*1",
    "MA (1);",
    "R0 = RSBAT (i + R[m]);",
    "L (0);",
  },
  { "n", "0m", "mov.b @(R0,<REG_M>),<REG_N>", "0000nnnnmmmm1100",
    "MA (1);",
    "R[n] = RSBAT (R0 + R[m]);",
    "L (n);",
  },
  { "n", "m", "mov.b @<REG_M>+,<REG_N>", "0110nnnnmmmm0100",
    "MA (1);",
    "R[n] = RSBAT (R[m]);",
    "R[m] += 1;",
    "L (n);",
  },
  { "", "mn", "mov.b <REG_M>,@<REG_N>", "0010nnnnmmmm0000",
    "MA (1);",
    "WBAT (R[n], R[m]);",
  },
  { "", "0", "mov.b R0,@(<disp>,GBR)", "11000000i8*1....",
    "MA (1);",
    "WBAT (i + GBR, R0);",
  },
  { "", "m0", "mov.b R0,@(<disp>,<REG_M>)", "10000000mmmmi4*1",
    "MA (1);",
    "WBAT (i + R[m], R0);",
  },
  { "", "mn0", "mov.b <REG_M>,@(R0,<REG_N>)", "0000nnnnmmmm0100",
    "MA (1);",
    "WBAT (R[n] + R0, R[m]);",
  },
  { "", "nm", "mov.b <REG_M>,@-<REG_N>", "0010nnnnmmmm0100",
    "MA (1);",
    "R[n] -= 1;",
    "WBAT (R[n], R[m]);",
  },
  { "n", "m", "mov.b @<REG_M>,<REG_N>", "0110nnnnmmmm0000",
    "MA (1);",
    "R[n] = RSBAT (R[m]);",
    "L (n);",
  },

  { "0", "", "mov.l @(<disp>,GBR),R0", "11000110i8*4....",
    "MA (1);",
    "R0 = RLAT (i + GBR);",
    "L (0);",
  },
  { "n", "", "mov.l @(<disp>,PC),<REG_N>", "1101nnnni8p4....",
    "MA (1);",
    "R[n] = RLAT ((PH2T (PC) & ~3) + 4 + i);",
    "L (n);",
  },
  { "n", "m", "mov.l @(<disp>,<REG_M>),<REG_N>", "0101nnnnmmmmi4*4",
    "MA (1);",
    "R[n] = RLAT (i + R[m]);",
    "L (n);",
  },
  { "n", "m0", "mov.l @(R0,<REG_M>),<REG_N>", "0000nnnnmmmm1110",
    "MA (1);",
    "R[n] = RLAT (R0 + R[m]);",
    "L (n);",
  },
  { "nm", "m", "mov.l @<REG_M>+,<REG_N>", "0110nnnnmmmm0110",
    "MA (1);",
    "R[n] = RLAT (R[m]);",
    "R[m] += 4;",
    "L (n);",
  },
  { "n", "m", "mov.l @<REG_M>,<REG_N>", "0110nnnnmmmm0010",
    "MA (1);",
    "R[n] = RLAT (R[m]);",
    "L (n);",
  },
  { "", "0", "mov.l R0,@(<disp>,GBR)", "11000010i8*4....",
    "MA (1);",
    "WLAT (i + GBR, R0);",
  },
  { "", "nm", "mov.l <REG_M>,@(<disp>,<REG_N>)", "0001nnnnmmmmi4*4",
    "MA (1);",
    "WLAT (i + R[n], R[m]);",
  },
  { "", "nm0", "mov.l <REG_M>,@(R0,<REG_N>)", "0000nnnnmmmm0110",
    "MA (1);",
    "WLAT (R0 + R[n], R[m]);",
  },
  { "", "nm", "mov.l <REG_M>,@-<REG_N>", "0010nnnnmmmm0110",
    "MA (1) ;",
    "R[n] -= 4;",
    "WLAT (R[n], R[m]);",
  },
  { "", "nm", "mov.l <REG_M>,@<REG_N>", "0010nnnnmmmm0010",
    "MA (1);",
    "WLAT (R[n], R[m]);",
  },

  { "0", "", "mov.w @(<disp>,GBR),R0", "11000101i8*2....",
    "MA (1)",
    ";R0 = RSWAT (i + GBR);",
    "L (0);",
  },
  { "n", "", "mov.w @(<disp>,PC),<REG_N>", "1001nnnni8p2....",
    "MA (1);",
    "R[n] = RSWAT (PH2T (PC + 4 + i));",
    "L (n);",
  },
  { "0", "m", "mov.w @(<disp>,<REG_M>),R0", "10000101mmmmi4*2",
    "MA (1);",
    "R0 = RSWAT (i + R[m]);",
    "L (0);",
  },
  { "n", "m0", "mov.w @(R0,<REG_M>),<REG_N>", "0000nnnnmmmm1101",
    "MA (1);",
    "R[n] = RSWAT (R0 + R[m]);",
    "L (n);",
  },
  { "nm", "n", "mov.w @<REG_M>+,<REG_N>", "0110nnnnmmmm0101",
    "MA (1);",
    "R[n] = RSWAT (R[m]);",
    "R[m] += 2;",
    "L (n);",
  },
  { "n", "m", "mov.w @<REG_M>,<REG_N>", "0110nnnnmmmm0001",
    "MA (1);",
    "R[n] = RSWAT (R[m]);",
    "L (n);",
  },
  { "", "0", "mov.w R0,@(<disp>,GBR)", "11000001i8*2....",
    "MA (1);",
    "WWAT (i + GBR, R0);",
  },
  { "", "0m", "mov.w R0,@(<disp>,<REG_M>)", "10000001mmmmi4*2",
    "MA (1);",
    "WWAT (i + R[m], R0);",
  },
  { "", "m0n", "mov.w <REG_M>,@(R0,<REG_N>)", "0000nnnnmmmm0101",
    "MA (1);",
    "WWAT (R0 + R[n], R[m]);",
  },
  { "n", "mn", "mov.w <REG_M>,@-<REG_N>", "0010nnnnmmmm0101",
    "MA (1);",
    "R[n] -= 2;",
    "WWAT (R[n], R[m]);",
  },
  { "", "nm", "mov.w <REG_M>,@<REG_N>", "0010nnnnmmmm0001",
    "MA (1);",
    "WWAT (R[n], R[m]);",
  },

  { "0", "", "mova @(<disp>,PC),R0", "11000111i8p4....",
    "R0 = ((i + 4 + PH2T (PC)) & ~0x3);",
  },

  { "0", "", "movca.l @R0, <REG_N>", "0000nnnn11000011",
    "/* FIXME: Not implemented */",
    "RAISE_EXCEPTION (SIGILL);",
  },

  { "n", "", "movt <REG_N>", "0000nnnn00101001",
    "R[n] = T;",
  },

  { "", "mn", "mul.l <REG_M>,<REG_N>", "0000nnnnmmmm0111",
    "MACL = ((int)R[n]) * ((int)R[m]);",
  },
#if 0
  { "", "nm", "mul.l <REG_M>,<REG_N>", "0000nnnnmmmm0111",
    "MACL = R[n] * R[m];",
  },
#endif

  /* muls.w - see muls */
  { "", "mn", "muls <REG_M>,<REG_N>", "0010nnnnmmmm1111",
    "MACL = ((int)(short)R[n]) * ((int)(short)R[m]);",
  },

  /* mulu.w - see mulu */
  { "", "mn", "mulu <REG_M>,<REG_N>", "0010nnnnmmmm1110",
    "MACL = (((unsigned int)(unsigned short)R[n])",
    "        * ((unsigned int)(unsigned short)R[m]));",
  },

  { "n", "m", "neg <REG_M>,<REG_N>", "0110nnnnmmmm1011",
    "R[n] = - R[m];",
  },

  { "n", "m", "negc <REG_M>,<REG_N>", "0110nnnnmmmm1010",
    "ult = -T;",
    "SET_SR_T (ult > 0);",
    "R[n] = ult - R[m];",
    "SET_SR_T (T || (R[n] > ult));",
  },

  { "", "", "nop", "0000000000001001",
    "/* nop */",
  },

  { "n", "m", "not <REG_M>,<REG_N>", "0110nnnnmmmm0111",
    "R[n] = ~R[m];",
  },

  { "0", "", "ocbi @<REG_N>", "0000nnnn10010011",
    "/* FIXME: Not implemented */",
    "RAISE_EXCEPTION (SIGILL);",
  },

  { "0", "", "ocbp @<REG_N>", "0000nnnn10100011",
    "/* FIXME: Not implemented */",
    "RAISE_EXCEPTION (SIGILL);",
  },

  { "", "n", "ocbwb @<REG_N>", "0000nnnn10110011",
    "RSBAT (R[n]); /* Take exceptions like byte load.  */",
    "/* FIXME: Cache not implemented */",
  },

  { "0", "", "or #<imm>,R0", "11001011i8*1....",
    "R0 |= i;",
  },
  { "n", "m", "or <REG_M>,<REG_N>", "0010nnnnmmmm1011",
    "R[n] |= R[m];",
  },
  { "", "0", "or.b #<imm>,@(R0,GBR)", "11001111i8*1....",
    "MA (1);",
    "WBAT (R0 + GBR, (RBAT (R0 + GBR) | i));",
  },

  { "", "n", "pref @<REG_N>", "0000nnnn10000011",
    "/* Except for the effect on the cache - which is not simulated -",
    "   this is like a nop.  */",
  },

  { "n", "n", "rotcl <REG_N>", "0100nnnn00100100",
    "ult = R[n] < 0;",
    "R[n] = (R[n] << 1) | T;",
    "SET_SR_T (ult);",
  },

  { "n", "n", "rotcr <REG_N>", "0100nnnn00100101",
    "ult = R[n] & 1;",
    "R[n] = (UR[n] >> 1) | (T << 31);",
    "SET_SR_T (ult);",
  },

  { "n", "n", "rotl <REG_N>", "0100nnnn00000100",
    "SET_SR_T (R[n] < 0);",
    "R[n] <<= 1;",
    "R[n] |= T;",
  },

  { "n", "n", "rotr <REG_N>", "0100nnnn00000101",
    "SET_SR_T (R[n] & 1);",
    "R[n] = UR[n] >> 1;",
    "R[n] |= (T << 31);",
  },

  { "", "", "rte", "0000000000101011", 
#if 0
    /* SH-[12] */
    "int tmp = PC;",
    "SET_NIP (PT2H (RLAT (R[15]) + 2));",
    "R[15] += 4;",
    "SET_SR (RLAT (R[15]) & 0x3f3);",
    "R[15] += 4;",
    "Delay_Slot (PC + 2);",
#else
    "SET_SR (SSR);",
    "SET_NIP (PT2H (SPC));",
    "cycles += 2;",
    "Delay_Slot (PC + 2);",
#endif
  },

  { "", "", "rts", "0000000000001011",
    "SET_NIP (PT2H (PR));",
    "cycles += 2;",
    "Delay_Slot (PC + 2);",
  },

  /* sh-dsp */
  { "", "n", "setrc <REG_N>", "0100nnnn00010100",
    "SET_RC (R[n]);",
  },
  { "", "n", "setrc #<imm>", "10000010i8*1....",
    /* It would be more realistic to let loop_start point to some static
       memory that contains an illegal opcode and then give a bus error when
       the loop is eventually encountered, but it seems not only simpler,
       but also more debugging-friendly to just catch the failure here.  */
    "if (BUSERROR (RS | RE, maskw))",
    "  RAISE_EXCEPTION (SIGILL);",
    "else {",
    "  SET_RC (i);",
    "  loop = get_loop_bounds (RS, RE, memory, mem_end, maskw, endianw);",
    "  CHECK_INSN_PTR (insn_ptr);",
    "}",
  },

  { "", "", "sets", "0000000001011000",
    "SET_SR_S (1);",
  },

  { "", "", "sett", "0000000000011000",
    "SET_SR_T (1);",
  },

  { "n", "mn", "shad <REG_M>,<REG_N>", "0100nnnnmmmm1100",
    "R[n] = (R[m] < 0) ? (R[n] >> ((-R[m])&0x1f)) : (R[n] << (R[m] & 0x1f));",
  },

  { "n", "n", "shal <REG_N>", "0100nnnn00100000",
    "SET_SR_T (R[n] < 0);",
    "R[n] <<= 1;",
  },

  { "n", "n", "shar <REG_N>", "0100nnnn00100001",
    "SET_SR_T (R[n] & 1);",
    "R[n] = R[n] >> 1;",
  },

  { "n", "mn", "shld <REG_M>,<REG_N>", "0100nnnnmmmm1101",
    "R[n] = (R[m] < 0) ? (UR[n] >> ((-R[m])&0x1f)): (R[n] << (R[m] & 0x1f));",
  },

  { "n", "n", "shll <REG_N>", "0100nnnn00000000",
    "SET_SR_T (R[n] < 0);",
    "R[n] <<= 1;",
  },

  { "n", "n", "shll2 <REG_N>", "0100nnnn00001000",
    "R[n] <<= 2;",
  },
  { "n", "n", "shll8 <REG_N>", "0100nnnn00011000",
    "R[n] <<= 8;",
  },
  { "n", "n", "shll16 <REG_N>", "0100nnnn00101000",
    "R[n] <<= 16;",
  },

  { "n", "n", "shlr <REG_N>", "0100nnnn00000001",
    "SET_SR_T (R[n] & 1);",
    "R[n] = UR[n] >> 1;",
  },

  { "n", "n", "shlr2 <REG_N>", "0100nnnn00001001",
    "R[n] = UR[n] >> 2;",
  },
  { "n", "n", "shlr8 <REG_N>", "0100nnnn00011001",
    "R[n] = UR[n] >> 8;",
  },
  { "n", "n", "shlr16 <REG_N>", "0100nnnn00101001",
    "R[n] = UR[n] >> 16;",
  },

  { "", "", "sleep", "0000000000011011",
    "nip = PC;",
    "trap (0xc3, R0, memory, maskl, maskw, endianw);",
  },

  { "n", "", "stc <CREG_M>,<REG_N>", "0000nnnnmmmm0010",
    "R[n] = CREG (m);",
  },

#if 0
  { "n", "", "stc SGR,<REG_N>", "0000nnnn00111010",
    "R[n] = SGR;",
  },
  { "n", "", "stc DBR,<REG_N>", "0000nnnn11111010",
    "R[n] = DBR;",
  },
#endif
  { "n", "n", "stc.l <CREG_M>,@-<REG_N>", "0100nnnnmmmm0011",
    "MA (1);",
    "R[n] -= 4;",
    "WLAT (R[n], CREG (m));",
  },
#if 0
  { "n", "n", "stc.l SGR,@-<REG_N>", "0100nnnn00110010",
    "MA (1);",
    "R[n] -= 4;",
    "WLAT (R[n], SGR);",
  },
  { "n", "n", "stc.l DBR,@-<REG_N>", "0100nnnn11110010",
    "MA (1);",
    "R[n] -= 4;",
    "WLAT (R[n], DBR);",
  },
#endif

  { "n", "", "sts <SREG_M>,<REG_N>", "0000nnnnssss1010",
    "R[n] = SREG (m);",
  },
  { "n", "n", "sts.l <SREG_M>,@-<REG_N>", "0100nnnnssss0010",
    "MA (1);",
    "R[n] -= 4;",
    "WLAT (R[n], SREG (m));",
  },

  { "n", "nm", "sub <REG_M>,<REG_N>", "0011nnnnmmmm1000",
    "R[n] -= R[m];",
  },

  { "n", "nm", "subc <REG_M>,<REG_N>", "0011nnnnmmmm1010",
    "ult = R[n] - T;",
    "SET_SR_T (ult > R[n]);",
    "R[n] = ult - R[m];",
    "SET_SR_T (T || (R[n] > ult));",
  },

  { "n", "nm", "subv <REG_M>,<REG_N>", "0011nnnnmmmm1011",
    "ult = R[n] - R[m];",
    "SET_SR_T (((R[n] ^ R[m]) & (ult ^ R[n])) >> 31);",
    "R[n] = ult;",
  },

  { "n", "nm", "swap.b <REG_M>,<REG_N>", "0110nnnnmmmm1000",
    "R[n] = ((R[m] & 0xffff0000)",
    "        | ((R[m] << 8) & 0xff00)",
    "        | ((R[m] >> 8) & 0x00ff));",
  },
  { "n", "nm", "swap.w <REG_M>,<REG_N>", "0110nnnnmmmm1001",
    "R[n] = (((R[m] << 16) & 0xffff0000)",
    "        | ((R[m] >> 16) & 0x00ffff));",
  },

  { "", "n", "tas.b @<REG_N>", "0100nnnn00011011",
    "MA (1);",
    "ult = RBAT(R[n]);",
    "SET_SR_T (ult == 0);",
    "WBAT(R[n],ult|0x80);",
  },

  { "0", "", "trapa #<imm>", "11000011i8*1....", 
#if 0
    /* SH-[12] */
    "long imm = 0xff & i;",
    "if (i==0xc3)",
    "  PC-=2;",
    "if (i<20||i==34||i==0xc3)",
    "  trap(i,R,memory,maskl,maskw,endianw);",
    "else {",
    "  R[15]-=4;",
    "  WLAT(R[15],GET_SR());",
    "  R[15]-=4;",
    "  WLAT(R[15],PC+2);",
    "  PC=RLAT(VBR+(imm<<2))-2;",
    "}",
#else
    "if (i == 0xc3)",
    "  {",
    "    nip = PC;",
    "    trap (i, R, memory, maskl, maskw,endianw);",
    "  }",
    "else if (i < 20 || i==34 || i==0xc3)",
    "  trap (i, R, memory, maskl, maskw,endianw);",
    "else if (!SR_BL) {",
    "  /* FIXME: TRA = (imm << 2); */",
    "  SSR = GET_SR();",
    "  SPC = PH2T (PC + 2);",
    "  SET_SR (GET_SR() | SR_MASK_MD | SR_MASK_BL | SR_MASK_RB);",
    "  /* FIXME: EXPEVT = 0x00000160; */",
    "  SET_NIP (PT2H (VBR + 0x00000100));",
    "}",
#endif
  },

  { "", "mn", "tst <REG_M>,<REG_N>", "0010nnnnmmmm1000",
    "SET_SR_T ((R[n] & R[m]) == 0);",
  },
  { "", "0", "tst #<imm>,R0", "11001000i8*1....",
    "SET_SR_T ((R0 & i) == 0);",
  },
  { "", "0", "tst.b #<imm>,@(R0,GBR)", "11001100i8*1....",
    "MA (1);",
    "SET_SR_T ((RBAT (GBR+R0) & i) == 0);",
  },

  { "", "0", "xor #<imm>,R0", "11001010i8*1....",
    "R0 ^= i;",
  },
  { "n", "mn", "xor <REG_M>,<REG_N>", "0010nnnnmmmm1010",
    "R[n] ^= R[m];",
  },
  { "", "0", "xor.b #<imm>,@(R0,GBR)", "11001110i8*1....",
    "MA (1);",
    "ult = RBAT (GBR+R0);",
    "ult ^= i;",
    "WBAT (GBR + R0, ult);",
  },

  { "n", "nm", "xtrct <REG_M>,<REG_N>", "0010nnnnmmmm1101",
    "R[n] = (((R[n] >> 16) & 0xffff)",
    "        | ((R[m] << 16) & 0xffff0000));",
  },

#if 0
  { "divs.l <REG_M>,<REG_N>", "0100nnnnmmmm1110",
    "divl(0,R[n],R[m]);",
  },
  { "divu.l <REG_M>,<REG_N>", "0100nnnnmmmm1101",
    "divl(0,R[n],R[m]);",
  },
#endif

  {0, 0}};

op movsxy_tab[] =
{
/* If this is disabled, the simulator speeds up by about 12% on a
   450 MHz PIII - 9% with ACE_FAST.
   Maybe we should have separate simulator loops?  */
#if 1
  { "n", "n", "movs.w @-<REG_N>,<DSP_REG_M>", "111101NNMMMM0000",
    "MA (1);",
    "R[n] -= 2;",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "DSP_GRD (m) = SIGN32 (DSP_R (m));",
  },
  { "", "n",  "movs.w @<REG_N>,<DSP_REG_M>",  "111101NNMMMM0100",
    "MA (1);",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "DSP_GRD (m) = SIGN32 (DSP_R (m));",
  },
  { "n", "n", "movs.w @<REG_N>+,<DSP_REG_M>", "111101NNMMMM1000",
    "MA (1);",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "DSP_GRD (m) = SIGN32 (DSP_R (m));",
    "R[n] += 2;",
  },
  { "n", "n8","movs.w @<REG_N>+REG_8,<DSP_REG_M>", "111101NNMMMM1100",
    "MA (1);",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "DSP_GRD (m) = SIGN32 (DSP_R (m));",
    "R[n] += R[8];",
  },
  { "n", "n", "movs.w @-<REG_N>,<DSP_GRD_M>", "111101NNGGGG0000",
    "MA (1);",
    "R[n] -= 2;",
    "DSP_R (m) = RSWAT (R[n]);",
  },
  { "", "n",  "movs.w @<REG_N>,<DSP_GRD_M>",  "111101NNGGGG0100",
    "MA (1);",
    "DSP_R (m) = RSWAT (R[n]);",
  },
  { "n", "n", "movs.w @<REG_N>+,<DSP_GRD_M>", "111101NNGGGG1000",
    "MA (1);",
    "DSP_R (m) = RSWAT (R[n]);",
    "R[n] += 2;",
  },
  { "n", "n8","movs.w @<REG_N>+REG_8,<DSP_GRD_M>", "111101NNGGGG1100",
    "MA (1);",
    "DSP_R (m) = RSWAT (R[n]);",
    "R[n] += R[8];",
  },
  { "n", "n", "<DSP_REG_M>,movs.w @-<REG_N>", "111101NNMMMM0001",
    "MA (1);",
    "R[n] -= 2;",
    "WWAT (R[n], DSP_R (m) >> 16);",
  },
  { "", "n",  "movs.w <DSP_REG_M>,@<REG_N>",  "111101NNMMMM0101",
    "MA (1);",
    "WWAT (R[n], DSP_R (m) >> 16);",
  },
  { "n", "n", "movs.w <DSP_REG_M>,@<REG_N>+", "111101NNMMMM1001",
    "MA (1);",
    "WWAT (R[n], DSP_R (m) >> 16);",
    "R[n] += 2;",
  },
  { "n", "n8","movs.w <DSP_REG_M>,@<REG_N>+REG_8", "111101NNMMMM1101",
    "MA (1);",
    "WWAT (R[n], DSP_R (m) >> 16);",
    "R[n] += R[8];",
  },
  { "n", "n", "movs.w <DSP_GRD_M>,@-<REG_N>", "111101NNGGGG0001",
    "MA (1);",
    "R[n] -= 2;",
    "WWAT (R[n], SEXT (DSP_R (m)));",
  },
  { "", "n",  "movs.w <DSP_GRD_M>,@<REG_N>",  "111101NNGGGG0101",
    "MA (1);",
    "WWAT (R[n], SEXT (DSP_R (m)));",
  },
  { "n", "n", "movs.w <DSP_GRD_M>,@<REG_N>+", "111101NNGGGG1001",
    "MA (1);",
    "WWAT (R[n], SEXT (DSP_R (m)));",
    "R[n] += 2;",
  },
  { "n", "n8","movs.w <DSP_GRD_M>,@<REG_N>+REG_8", "111101NNGGGG1101",
    "MA (1);",
    "WWAT (R[n], SEXT (DSP_R (m)));",
    "R[n] += R[8];",
  },
  { "n", "n", "movs.l @-<REG_N>,<DSP_REG_M>", "111101NNMMMM0010",
    "MA (1);",
    "R[n] -= 4;",
    "DSP_R (m) = RLAT (R[n]);",
    "DSP_GRD (m) = SIGN32 (DSP_R (m));",
  },
  { "", "n",  "movs.l @<REG_N>,<DSP_REG_M>",  "111101NNMMMM0110",
    "MA (1);",
    "DSP_R (m) = RLAT (R[n]);",
    "DSP_GRD (m) = SIGN32 (DSP_R (m));",
  },
  { "n", "n", "movs.l @<REG_N>+,<DSP_REG_M>", "111101NNMMMM1010",
    "MA (1);",
    "DSP_R (m) = RLAT (R[n]);",
    "DSP_GRD (m) = SIGN32 (DSP_R (m));",
    "R[n] += 4;",
  },
  { "n", "n8","movs.l @<REG_N>+REG_8,<DSP_REG_M>", "111101NNMMMM1110",
    "MA (1);",
    "DSP_R (m) = RLAT (R[n]);",
    "DSP_GRD (m) = SIGN32 (DSP_R (m));",
    "R[n] += R[8];",
  },
  { "n", "n", "<DSP_REG_M>,movs.l @-<REG_N>", "111101NNMMMM0011",
    "MA (1);",
    "R[n] -= 4;",
    "WLAT (R[n], DSP_R (m));",
  },
  { "", "n",  "movs.l <DSP_REG_M>,@<REG_N>",  "111101NNMMMM0111",
    "MA (1);",
    "WLAT (R[n], DSP_R (m));",
  },
  { "n", "n", "movs.l <DSP_REG_M>,@<REG_N>+", "111101NNMMMM1011",
    "MA (1);",
    "WLAT (R[n], DSP_R (m));",
    "R[n] += 4;",
  },
  { "n", "n8","movs.l <DSP_REG_M>,@<REG_N>+REG_8", "111101NNMMMM1111",
    "MA (1);",
    "WLAT (R[n], DSP_R (m));",
    "R[n] += R[8];",
  },
  { "n", "n", "<DSP_GRD_M>,movs.l @-<REG_N>", "111101NNGGGG0011",
    "MA (1);",
    "R[n] -= 4;",
    "WLAT (R[n], SEXT (DSP_R (m)));",
  },
  { "", "n",  "movs.l <DSP_GRD_M>,@<REG_N>",  "111101NNGGGG0111",
    "MA (1);",
    "WLAT (R[n], SEXT (DSP_R (m)));",
  },
  { "n", "n", "movs.l <DSP_GRD_M>,@<REG_N>+", "111101NNGGGG1011",
    "MA (1);",
    "WLAT (R[n], SEXT (DSP_R (m)));",
    "R[n] += 4;",
  },
  { "n", "n8","movs.l <DSP_GRD_M>,@<REG_N>+REG_8", "111101NNGGGG1111",
    "MA (1);",
    "WLAT (R[n], SEXT (DSP_R (m)));",
    "R[n] += R[8];",
  },
  { "", "n", "movx.w @<REG_x>,<DSP_XX>",   "111100xxXX000100",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "iword &= 0xfd53; goto top;",
  },
  { "n", "n", "movx.w @<REG_x>+,<DSP_XX>", "111100xxXX001000",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "R[n] += ((R[n] & 0xffff) == MOD_ME) ? MOD_DELTA : 2;",
    "iword &= 0xfd53; goto top;",
  },
  { "n", "n8","movx.w @<REG_x>+REG_8,<DSP_XX>", "111100xxXX001000",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "R[n] += ((R[n] & 0xffff) == MOD_ME) ? MOD_DELTA : R[8];",
    "iword &= 0xfd53; goto top;",
  },
  { "", "n", "movx.w <DSP_Aa>,@<REG_x>",   "111100xxaa100100",
    "WWAT (R[n], DSP_R (m) >> 16);",
    "iword &= 0xfd53; goto top;",
  },
  { "n", "n", "movx.w <DSP_Aa>,@<REG_x>+", "111100xxaa101000",
    "WWAT (R[n], DSP_R (m) >> 16);",
    "R[n] += ((R[n] & 0xffff) == MOD_ME) ? MOD_DELTA : 2;",
    "iword &= 0xfd53; goto top;",
  },
  { "n", "n8","movx.w <DSP_Aa>,@<REG_x>+REG_8","111100xxaa101000",
    "WWAT (R[n], DSP_R (m) >> 16);",
    "R[n] += ((R[n] & 0xffff) == MOD_ME) ? MOD_DELTA : R[8];",
    "iword &= 0xfd53; goto top;",
  },
  { "", "n", "movy.w @<REG_y>,<DSP_YY>",   "111100yyYY000001",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
  },
  { "n", "n", "movy.w @<REG_x>+,<DSP_YY>", "111100yyYY000010",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "R[n] += ((R[n] | ~0xffff) == MOD_ME) ? MOD_DELTA : 2;",
  },
  { "n", "n9","movy.w @<REG_x>+REG_9,<DSP_YY>", "111100yyYY000010",
    "DSP_R (m) = RSWAT (R[n]) << 16;",
    "R[n] += ((R[n] | ~0xffff) == MOD_ME) ? MOD_DELTA : R[9];",
  },
  { "", "n", "movy.w <DSP_Aa>,@<REG_x>",   "111100yyAA010001",
    "WWAT (R[n], DSP_R (m) >> 16);",
  },
  { "n", "n", "movy.w <DSP_Aa>,@<REG_x>+", "111100yyAA010010",
    "WWAT (R[n], DSP_R (m) >> 16);",
    "R[n] += ((R[n] | ~0xffff) == MOD_ME) ? MOD_DELTA : 2;",
  },
  { "n", "n9", "movy.w <DSP_Aa>,@<REG_x>+REG_9", "111100yyAA010010",
    "WWAT (R[n], DSP_R (m) >> 16);",
    "R[n] += ((R[n] | ~0xffff) == MOD_ME) ? MOD_DELTA : R[9];",
  },
  { "", "", "nopx nopy", "1111000000000000",
    "/* nop */",
  },
  { "", "", "ppi", "1111100000000000",
    "ppi_insn (RIAT (nip));",
    "nip += 2;",
    "iword &= 0xf7ff; goto top;",
  },
#endif
  {0, 0}};

op ppi_tab[] =
{
  { "","", "pshl #<imm>,dz",	"00000iiim16.zzzz",
    "int Sz = DSP_R (z) & 0xffff0000;",
    "",
    "if (i < 16)",
    "  res = Sz << i;",
    "else if (i >= 128 - 16)",
    "  res = Sz >> 128 - i;",
    "else",
    "  {",
    "    RAISE_EXCEPTION (SIGILL);",
    "    return;",
    "  }",
    "res &= 0xffff0000;",
    "res_grd = 0;",
    "goto logical;",
  },
  { "","", "psha #<imm>,dz",	"00010iiim32.zzzz",
    "int Sz = DSP_R (z);",
    "int Sz_grd = GET_DSP_GRD (z);",
    "",
    "if (i < 32)",
    "  {",
    "    if (i == 32)",
    "      {",
    "        res = 0;",
    "        res_grd = Sz;",
    "      }",
    "    else",
    "      {",
    "        res = Sz << i;",
    "        res_grd = Sz_grd << i | (unsigned) Sz >> 32 - i;",
    "      }",
    "    res_grd = SEXT (res_grd);",
    "    carry = res_grd & 1;",
    "  }",
    "else if (i >= 96)",
    "  {",
    "    i = 128 - i;",
    "    if (i == 32)",
    "      {",
    "        res_grd = SIGN32 (Sz_grd);",
    "        res = Sz_grd;",
    "      }",
    "    else",
    "      {",
    "        res = Sz >> i | Sz_grd << 32 - i;",
    "        res_grd = Sz_grd >> i;",
    "      }",
    "    carry = Sz >> (i - 1) & 1;",
    "  }",
    "else",
    "  {",
    "    RAISE_EXCEPTION (SIGILL);",
    "    return;",
    "  }",
    "COMPUTE_OVERFLOW;",
    "greater_equal = 0;",
  },
  { "","", "pmuls Se,Sf,Dg",	"0100eeffxxyygguu",
    "res = (DSP_R (e)) >> 16 * (DSP_R (f) >> 16) * 2;",
    "if (res == 0x80000000)",
    "  res = 0x7fffffff;",
    "DSP_R (g) = res;",
    "DSP_GRD (g) = SIGN32 (res);",
    "return;",
  },
  { "","", "psub Sx,Sy,Du pmuls Se,Sf,Dg",	"0110eeffxxyygguu",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = (DSP_R (e)) >> 16 * (DSP_R (f) >> 16) * 2;",
    "if (res == 0x80000000)",
    "  res = 0x7fffffff;",
    "DSP_R (g) = res;",
    "DSP_GRD (g) = SIGN32 (res);",
    "",
    "z = u;",
    "res = Sx - Sy;",
    "carry = (unsigned) res > (unsigned) Sx;",
    "res_grd = Sx_grd - Sy_grd - carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "padd Sx,Sy,Du pmuls Se,Sf,Dg",	"0111eeffxxyygguu",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = (DSP_R (e)) >> 16 * (DSP_R (f) >> 16) * 2;",
    "if (res == 0x80000000)",
    "  res = 0x7fffffff;",
    "DSP_R (g) = res;",
    "DSP_GRD (g) = SIGN32 (res);",
    "",
    "z = u;",
    "res = Sx + Sy;",
    "carry = (unsigned) res < (unsigned) Sx;",
    "res_grd = Sx_grd + Sy_grd + carry;",
    "COMPUTE_OVERFLOW;",
  },
  { "","", "psubc Sx,Sy,Dz",		"10100000xxyyzzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = Sx - Sy - (DSR & 1);",
    "carry = (unsigned) res > (unsigned) Sx || (res == Sx && Sy);",
    "res_grd = Sx_grd + Sy_grd + carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
    "DSR &= ~0xf1;\n",
    "if (res || res_grd)\n",
    "  DSR |= greater_equal | res_grd >> 2 & DSR_MASK_N | overflow;\n",
    "else\n",
    "  DSR |= DSR_MASK_Z | overflow;\n",
    "DSR |= carry;\n",
    "goto assign_z;\n",
  },
  { "","", "paddc Sx,Sy,Dz",	"10110000xxyyzzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = Sx + Sy + (DSR & 1);",
    "carry = (unsigned) res < (unsigned) Sx || (res == Sx && Sy);",
    "res_grd = Sx_grd + Sy_grd + carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
    "DSR &= ~0xf1;\n",
    "if (res || res_grd)\n",
    "  DSR |= greater_equal | res_grd >> 2 & DSR_MASK_N | overflow;\n",
    "else\n",
    "  DSR |= DSR_MASK_Z | overflow;\n",
    "DSR |= carry;\n",
    "goto assign_z;\n",
  },
  { "","", "pcmp Sx,Sy",	"10000100xxyy....",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "z = 17; /* Ignore result.  */",
    "res = Sx - Sy;",
    "carry = (unsigned) res > (unsigned) Sx;",
    "res_grd = Sx_grd - Sy_grd - carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "pwsb Sx,Sy,Dz",	"10100100xxyyzzzz",
  },
  { "","", "pwad Sx,Sy,Dz",	"10110100xxyyzzzz",
  },
  { "","", "pabs Sx,Dz",	"10001000xx..zzzz",
    "res = DSP_R (x);",
    "res_grd = GET_DSP_GRD (x);",
    "if (res >= 0)",
    "  carry = 0;",
    "else",
    "  {",
    "    res = -res;",
    "    carry = (res != 0); /* The manual has a bug here.  */", 
    "    res_grd = -res_grd - carry;", 
    "  }",
    "COMPUTE_OVERFLOW;",
    "/* ??? The re-computing of overflow after",
    "   saturation processing is specific to pabs.  */",
    "overflow = res_grd != SIGN32 (res) ? DSR_MASK_V : 0;",
    "ADD_SUB_GE;",
  },
  { "","", "prnd Sx,Dz",	"10011000xx..zzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "",
    "res = Sx + 0x8000;",
    "carry = (unsigned) res < (unsigned) Sx;",
    "res_grd = Sx_grd + carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "pabs Sy,Dz",	"10101000..yyzzzz",
    "res = DSP_R (y);",
    "res_grd = 0;",
    "overflow = 0;",
    "greater_equal = DSR_MASK_G;",
    "if (res >= 0)",
    "  carry = 0;",
    "else",
    "  {",
    "    res = -res;",
    "    carry = 1;",
    "    if (res < 0)",
    "      {",
    "        if (S)",
    "          res = 0x7fffffff;",
    "        else",
    "          {",
    "            overflow = DSR_MASK_V;",
    "            greater_equal = 0;",
    "          }",
    "      }",
    "  }",
  },
  { "","", "prnd Sy,Dz",	"10111000..yyzzzz",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = Sy + 0x8000;",
    "carry = (unsigned) res < (unsigned) Sy;",
    "res_grd = Sy_grd + carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) pshl Sx,Sy,Dz",	"100000ccxxyyzzzz",
    "int Sx = DSP_R (x) & 0xffff0000;",
    "int Sy = DSP_R (y) >> 16 & 0x7f;",
    "",
    "if (Sy < 16)",
    "  res = Sx << Sy;",
    "else if (Sy >= 128 - 16)",
    "  res = Sx >> 128 - Sy;",
    "else",
    "  {",
    "    RAISE_EXCEPTION (SIGILL);",
    "    return;",
    "  }",
    "goto cond_logical;",
  },
  { "","", "(if cc) psha Sx,Sy,Dz",	"100100ccxxyyzzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int Sy = DSP_R (y) >> 16 & 0x7f;",
    "",
    "if (Sy < 32)",
    "  {",
    "    if (Sy == 32)",
    "      {",
    "        res = 0;",
    "        res_grd = Sx;",
    "      }",
    "    else",
    "      {",
    "        res = Sx << Sy;",
    "        res_grd = Sx_grd << Sy | (unsigned) Sx >> 32 - Sy;",
    "      }",
    "    res_grd = SEXT (res_grd);",
    "    carry = res_grd & 1;",
    "  }",
    "else if (Sy >= 96)",
    "  {",
    "    Sy = 128 - Sy;",
    "    if (Sy == 32)",
    "      {",
    "        res_grd = SIGN32 (Sx_grd);",
    "        res = Sx_grd;",
    "      }",
    "    else",
    "      {",
    "        res = Sx >> Sy | Sx_grd << 32 - Sy;",
    "        res_grd = Sx_grd >> Sy;",
    "      }",
    "    carry = Sx >> (Sy - 1) & 1;",
    "  }",
    "else",
    "  {",
    "    RAISE_EXCEPTION (SIGILL);",
    "    return;",
    "  }",
    "COMPUTE_OVERFLOW;",
    "greater_equal = 0;",
  },
  { "","", "(if cc) psub Sx,Sy,Dz",	"101000ccxxyyzzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = Sx - Sy;",
    "carry = (unsigned) res > (unsigned) Sx;",
    "res_grd = Sx_grd - Sy_grd - carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) padd Sx,Sy,Dz",	"101100ccxxyyzzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = Sx + Sy;",
    "carry = (unsigned) res < (unsigned) Sx;",
    "res_grd = Sx_grd + Sy_grd + carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) pand Sx,Sy,Dz",	"100101ccxxyyzzzz",
    "res = DSP_R (x) & DSP_R (y);",
  "cond_logical:",
    "res &= 0xffff0000;",
    "res_grd = 0;",
    "if (iword & 0x200)\n",
    "  goto assign_z;\n",
  "logical:",
    "carry = 0;",
    "overflow = 0;",
    "greater_equal = 0;",
    "DSR &= ~0xf1;\n",
    "if (res)\n",
    "  DSR |= res >> 26 & DSR_MASK_N;\n",
    "else\n",
    "  DSR |= DSR_MASK_Z;\n",
    "goto assign_dc;\n",
  },
  { "","", "(if cc) pxor Sx,Sy,Dz",	"101001ccxxyyzzzz",
    "res = DSP_R (x) ^ DSP_R (y);",
    "goto cond_logical;",
  },
  { "","", "(if cc) por Sx,Sy,Dz",	"101101ccxxyyzzzz",
    "res = DSP_R (x) | DSP_R (y);",
    "goto cond_logical;",
  },
  { "","", "(if cc) pdec Sx,Dz",	"100010ccxx..zzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "",
    "res = Sx - 0x10000;",
    "carry = res > Sx;",
    "res_grd = Sx_grd - carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
    "res &= 0xffff0000;",
  },
  { "","", "(if cc) pinc Sx,Dz",	"100110ccxx..zzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "",
    "res = Sx + 0x10000;",
    "carry = res < Sx;",
    "res_grd = Sx_grd + carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
    "res &= 0xffff0000;",
  },
  { "","", "(if cc) pdec Sy,Dz",	"101010cc..yyzzzz",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = Sy - 0x10000;",
    "carry = res > Sy;",
    "res_grd = Sy_grd - carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
    "res &= 0xffff0000;",
  },
  { "","", "(if cc) pinc Sy,Dz",	"101110cc..yyzzzz",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = Sy + 0x10000;",
    "carry = res < Sy;",
    "res_grd = Sy_grd + carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
    "res &= 0xffff0000;",
  },
  { "","", "(if cc) pclr Dz",		"100011cc....zzzz",
    "res = 0;",
    "res_grd = 0;",
    "carry = 0;",
    "overflow = 0;",
    "greater_equal = 1;",
  },
  { "","", "(if cc) pdmsb Sx,Dz",	"100111ccxx..zzzz",
    "unsigned Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "int i = 16;",
    "",
    "if (Sx_grd < 0)",
    "  {",
    "    Sx_grd = ~Sx_grd;",
    "    Sx = ~Sx;",
    "  }",
    "if (Sx_grd)",
    "  {",
    "    Sx = Sx_grd;",
    "    res = -2;",
    "  }",
    "else if (Sx)",
    "  res = 30;",
    "else",
    "  res = 31;",
    "do",
    "  {",
    "    if (Sx & ~0 << i)",
    "      {",
    "        res -= i;",
    "        Sx >>= i;",
    "      }",
    "  }",
    "while (i >>= 1);",
    "res <<= 16;",
    "res_grd = SIGN32 (res);",
    "carry = 0;",
    "overflow = 0;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) pdmsb Sy,Dz",	"101111cc..yyzzzz",
    "unsigned Sy = DSP_R (y);",
    "int i;",
    "",
    "if (Sy < 0)",
    "  Sy = ~Sy;",
    "Sy <<= 1;",
    "res = 31;",
    "do",
    "  {",
    "    if (Sy & ~0 << i)",
    "      {",
    "        res -= i;",
    "        Sy >>= i;",
    "      }",
    "  }",
    "while (i >>= 1);",
    "res <<= 16;",
    "res_grd = SIGN32 (res);",
    "carry = 0;",
    "overflow = 0;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) pneg Sx,Dz",	"110010ccxx..zzzz",
    "int Sx = DSP_R (x);",
    "int Sx_grd = GET_DSP_GRD (x);",
    "",
    "res = 0 - Sx;",
    "carry = res != 0;",
    "res_grd = 0 - Sx_grd - carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) pcopy Sx,Dz",	"110110ccxx..zzzz",
    "res = DSP_R (x);",
    "res_grd = GET_DSP_GRD (x);",
    "carry = 0;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) pneg Sy,Dz",	"111010cc..yyzzzz",
    "int Sy = DSP_R (y);",
    "int Sy_grd = SIGN32 (Sy);",
    "",
    "res = 0 - Sy;",
    "carry = res != 0;",
    "res_grd = 0 - Sy_grd - carry;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) pcopy Sy,Dz",	"111110cc..yyzzzz",
    "res = DSP_R (y);",
    "res_grd = SIGN32 (res);",
    "carry = 0;",
    "COMPUTE_OVERFLOW;",
    "ADD_SUB_GE;",
  },
  { "","", "(if cc) psts MACH,Dz",	"110011cc....zzzz",
    "res = MACH;",
    "res_grd = SIGN32 (res);",
    "goto assign_z;",
  },
  { "","", "(if cc) psts MACL,Dz",	"110111cc....zzzz",
    "res = MACL;",
    "res_grd = SIGN32 (res);",
    "goto assign_z;",
  },
  { "","", "(if cc) plds Dz,MACH",	"111011cc....zzzz",
    "if (0xa05f >> z & 1)",
    "  RAISE_EXCEPTION (SIGILL);",
    "else",
    "  MACH = DSP_R (z);",
    "return;",
  },
  { "","", "(if cc) plds Dz,MACL",	"111111cc....zzzz",
    "if (0xa05f >> z & 1)",
    "  RAISE_EXCEPTION (SIGILL);",
    "else",
    "  MACL = DSP_R (z) = res;",
    "return;",
  },
  {0, 0}
};

/* Tables of things to put into enums for sh-opc.h */
static char *nibble_type_list[] =
{
  "HEX_0",
  "HEX_1",
  "HEX_2",
  "HEX_3",
  "HEX_4",
  "HEX_5",
  "HEX_6",
  "HEX_7",
  "HEX_8",
  "HEX_9",
  "HEX_A",
  "HEX_B",
  "HEX_C",
  "HEX_D",
  "HEX_E",
  "HEX_F",
  "REG_N",
  "REG_M",
  "BRANCH_12",
  "BRANCH_8",
  "DISP_8",
  "DISP_4",
  "IMM_4",
  "IMM_4BY2",
  "IMM_4BY4",
  "PCRELIMM_8BY2",
  "PCRELIMM_8BY4",
  "IMM_8",
  "IMM_8BY2",
  "IMM_8BY4",
  0
};
static
char *arg_type_list[] =
{
  "A_END",
  "A_BDISP12",
  "A_BDISP8",
  "A_DEC_M",
  "A_DEC_N",
  "A_DISP_GBR",
  "A_DISP_PC",
  "A_DISP_REG_M",
  "A_DISP_REG_N",
  "A_GBR",
  "A_IMM",
  "A_INC_M",
  "A_INC_N",
  "A_IND_M",
  "A_IND_N",
  "A_IND_R0_REG_M",
  "A_IND_R0_REG_N",
  "A_MACH",
  "A_MACL",
  "A_PR",
  "A_R0",
  "A_R0_GBR",
  "A_REG_M",
  "A_REG_N",
  "A_SR",
  "A_VBR",
  "A_SSR",
  "A_SPC",
  0,
};

static void
make_enum_list (name, s)
     char *name;
     char **s;
{
  int i = 1;
  printf ("typedef enum {\n");
  while (*s)
    {
      printf ("\t%s,\n", *s);
      s++;
      i++;
    }
  printf ("} %s;\n", name);
}

static int
qfunc (a, b)
     op *a;
     op *b;
{
  char bufa[9];
  char bufb[9];
  int diff;

  memcpy (bufa, a->code, 4);
  memcpy (bufa + 4, a->code + 12, 4);
  bufa[8] = 0;

  memcpy (bufb, b->code, 4);
  memcpy (bufb + 4, b->code + 12, 4);
  bufb[8] = 0;
  diff = strcmp (bufa, bufb);
  /* Stabilize the sort, so that later entries can override more general
     preceding entries.  */
  return diff ? diff : a - b;
}

static void
sorttab ()
{
  op *p = tab;
  int len = 0;

  while (p->name)
    {
      p++;
      len++;
    }
  qsort (tab, len, sizeof (*p), qfunc);
}

static void
gengastab ()
{
  op *p;
  sorttab ();
  for (p = tab; p->name; p++)
    {
      printf ("%s %-30s\n", p->code, p->name);
    }


}

/* Convert a string of 4 binary digits into an int */

static
int
bton (s)
     char *s;

{
  int n = 0;
  int v = 8;
  while (v)
    {
      if (*s == '1')
	n |= v;
      v >>= 1;
      s++;
    }
  return n;
}

static unsigned char table[1 << 16];

/* Take an opcode expand all varying fields in it out and fill all the
  right entries in 'table' with the opcode index*/

static void
expand_opcode (shift, val, i, s)
     int shift;
     int val;
     int i;
     char *s;
{
  int j;

  if (*s == 0)
    {
      table[val] = i;
    }
  else
    {
      switch (s[0])
	{

	case '0':
	case '1':
	  {
	    int m, mv;

	    val |= bton (s) << shift;
	    if (s[2] == '0' || s[2] == '1')
	      expand_opcode (shift - 4, val, i, s + 4);
	    else if (s[2] == 'N')
	      for (j = 0; j < 4; j++)
		expand_opcode (shift - 4, val | (j << shift), i, s + 4);
	    else if (s[2] == 'x')
	      for (j = 0; j < 4; j += 2)
		for (m = 0; m < 32; m++)
		  {
		    /* Ignore illegal nopy */
		    if ((m & 7) == 0 && m != 0)
		      continue;
		    mv = m & 3 | (m & 4) << 2 | (m & 8) << 3 | (m & 16) << 4;
		    expand_opcode (shift - 4, val | mv | (j << shift), i,
				   s + 4);
		  }
	    else if (s[2] == 'y')
	      for (j = 0; j < 2; j++)
		expand_opcode (shift - 4, val | (j << shift), i, s + 4);
	    break;
	  }
	case 'n':
	case 'm':
	  for (j = 0; j < 16; j++)
	    {
	      expand_opcode (shift - 4, val | (j << shift), i, s + 4);

	    }
	  break;
	case 'M':
	  /* A1, A0,X0,X1,Y0,Y1,M0,A1G,M1,M1G */
	  for (j = 5; j < 16; j++)
	    if (j != 6)
	      expand_opcode (shift - 4, val | (j << shift), i, s + 4);
	  break;
	case 'G':
	  /* A1G, A0G: */
	  for (j = 13; j <= 15; j +=2)
	    expand_opcode (shift - 4, val | (j << shift), i, s + 4);
	  break;
	case 's':
	  /* System registers mach, macl, pr: */
	  for (j = 0; j < 3; j++)
	    expand_opcode (shift - 4, val | (j << shift), i, s + 4);
	  /* System registers fpul, fpscr/dsr, a0, x0, x1, y0, y1: */
	  for (j = 5; j < 12; j++)
	    expand_opcode (shift - 4, val | (j << shift), i, s + 4);
	  break;
	case 'X':
	case 'a':
	  val |= bton (s) << shift;
	  for (j = 0; j < 16; j += 8)
	    expand_opcode (shift - 4, val | (j << shift), i, s + 4);
	  break;
	case 'Y':
	case 'A':
	  val |= bton (s) << shift;
	  for (j = 0; j < 8; j += 4)
	    expand_opcode (shift - 4, val | (j << shift), i, s + 4);
	  break;

	default:
	  for (j = 0; j < (1 << (shift + 4)); j++)
	    {
	      table[val | j] = i;
	    }
	}
    }
}

/* Print the jump table used to index an opcode into a switch
   statement entry. */

static void
dumptable (name, size, start)
     char *name;
     int size;
     int start;
{
  int lump = 256;
  int online = 16;

  int i = start;

  printf ("unsigned char %s[%d]={\n", name, size);
  while (i < start + size)
    {
      int j = 0;

      printf ("/* 0x%x */\n", i);

      while (j < lump)
	{
	  int k = 0;
	  while (k < online)
	    {
	      printf ("%2d", table[i + j + k]);
	      if (j + k < lump)
		printf (",");

	      k++;
	    }
	  j += k;
	  printf ("\n");
	}
      i += j;
    }
  printf ("};\n");
}


static void
filltable (p)
     op *p;
{
  static int index = 1;

  sorttab ();
  for (; p->name; p++)
    {
      p->index = index++;
      expand_opcode (12, 0, p->index, p->code);
    }
}

/* Table already contais all the switch case tags for 16-bit opcode double
   data transfer (ddt) insns, and the switch case tag for processing parallel
   processing insns (ppi) for code 0xf800 (ppi nopx nopy).  Copy the
   latter tag to represent all combinations of ppi with ddt.  */
static void
ppi_moves ()
{
  int i;

  for (i = 0xf000; i < 0xf400; i++)
    if (table[i])
      table[i + 0x800] = table[0xf800];
}

static void
gensim_caselist (p)
     op *p;
{
  for (; p->name; p++)
    {
      int j;
      int sextbit = -1;
      int needm = 0;
      int needn = 0;
      
      char *s = p->code;

      printf ("  /* %s %s */\n", p->name, p->code);
      printf ("  case %d:      \n", p->index);

      printf ("    {\n");
      while (*s)
	{
	  switch (*s)
	    {
	    case '0':
	    case '1':
	      s += 2;
	      break;
	    case '.':
	      s += 4;
	      break;
	    case 'n':
	      printf ("      int n = (iword >>8) & 0xf;\n");
	      needn = 1;
	      s += 4;
	      break;
	    case 'N':
	      printf ("      int n = (((iword >> 8) - 2) & 0x3) + 2;\n");
	      s += 2;
	      break;
	    case 'x':
	      printf ("      int n = ((iword >> 9) & 1) + 4;\n");
	      needn = 1;
	      s += 2;
	      break;
	    case 'y':
	      printf ("      int n = ((iword >> 8) & 1) + 4;\n");
	      needn = 1;
	      s += 2;
	      break;
	    case 'm':
	      needm = 1;
	    case 's':
	    case 'M':
	    case 'G':
	      printf ("      int m = (iword >>4) & 0xf;\n");
	      s += 4;
	      break;
	    case 'X':
	      printf ("      int m = ((iword >> 7) & 1) + 8;\n");
	      s += 2;
	      break;
	    case 'a':
	      printf ("      int m = 7 - ((iword >> 6) & 2);\n");
	      s += 2;
	      break;
	    case 'Y':
	      printf ("      int m = ((iword >> 6) & 1) + 10;\n");
	      s += 2;
	      break;
	    case 'A':
	      printf ("      int m = 7 - ((iword >> 5) & 2);\n");
	      s += 2;
	      break;

	    case 'i':
	      printf ("      int i = (iword & 0x");

	      switch (s[1])
		{
		case '4':
		  printf ("f");
		  break;
		case '8':
		  printf ("ff");
		  break;
		case '1':
		  sextbit = 12;

		  printf ("fff");
		  break;
		}
	      printf (")");

	      switch (s[3])
		{
		case '1':
		  break;
		case '2':
		  printf ("<<1");
		  break;
		case '4':
		  printf ("<<2");
		  break;
		}
	      printf (";\n");
	      s += 4;
	    }
	}
      if (sextbit > 0)
	{
	  printf ("      i = (i ^ (1<<%d))-(1<<%d);\n",
		  sextbit - 1, sextbit - 1);
	}

      if (needm && needn)
	printf ("      TB(m,n);\n");  
      else if (needm)
	printf ("      TL(m);\n");
      else if (needn)
	printf ("      TL(n);\n");

      {
	/* Do the refs */
	char *r;
	for (r = p->refs; *r; r++)
	  {
	    if (*r == '0') printf("      CREF(0);\n"); 
	    if (*r == '8') printf("      CREF(8);\n"); 
	    if (*r == '9') printf("      CREF(9);\n"); 
	    if (*r == 'n') printf("      CREF(n);\n"); 
	    if (*r == 'm') printf("      CREF(m);\n"); 
	  }
      }

      printf ("      {\n");
      for (j = 0; j < MAX_NR_STUFF; j++)
	{
	  if (p->stuff[j])
	    {
	      printf ("        %s\n", p->stuff[j]);
	    }
	}
      printf ("      }\n");

      {
	/* Do the defs */
	char *r;
	for (r = p->defs; *r; r++) 
	  {
	    if (*r == '0') printf("      CDEF(0);\n"); 
	    if (*r == 'n') printf("      CDEF(n);\n"); 
	    if (*r == 'm') printf("      CDEF(m);\n"); 
	  }
      }

      printf ("      break;\n");
      printf ("    }\n");
    }
}

static void
gensim ()
{
  printf ("{\n");
  printf ("  switch (jump_table[iword]) {\n");

  gensim_caselist (tab);
  gensim_caselist (movsxy_tab);

  printf ("  default:\n");
  printf ("    {\n");
  printf ("      RAISE_EXCEPTION (SIGILL);\n");
  printf ("    }\n");
  printf ("  }\n");
  printf ("}\n");
}

static void
gendefines ()
{
  op *p;
  filltable (tab);
  for (p = tab; p->name; p++)
    {
      char *s = p->name;
      printf ("#define OPC_");
      while (*s) {
	if (isupper(*s)) 
	  *s = tolower(*s);
	if (isalpha(*s)) printf("%c", *s);
	if (*s == ' ') printf("_");
	if (*s == '@') printf("ind_");
	if (*s == ',') printf("_");
	s++;
      }
      printf(" %d\n",p->index);
    }
}

static int ppi_index;

/* Take an ppi code, expand all varying fields in it and fill all the
   right entries in 'table' with the opcode index.  */

static void
expand_ppi_code (val, i, s)
     int val;
     int i;
     char *s;
{
  int j;

  for (;;)
    {
      switch (s[0])
	{
	/* The last eight bits are disregarded for the switch table.  */
	case 'm':
	case 'x':
	case '.':
	  table[val] = i;
	  return;
	case '0':
	  val += val;
	  s++;
	  break;
	case '1':
	  val += val + 1;
	  s++;
	  break;
	case 'i':
	case 'e': case 'f':
	  val += val;
	  s++;
	  expand_ppi_code (val, i, s);
	  val++;
	  break;
	case 'c':
	  val <<= 2;
	  s += 2;
	  val++;
	  expand_ppi_code (val, ppi_index++, s);
	  val++;
	  expand_ppi_code (val, i, s);
	  val++;
	  break;
	}
    }
}

static void
ppi_filltable ()
{
  op *p;
  ppi_index = 1;

  for (p = ppi_tab; p->name; p++)
    {
      p->index = ppi_index++;
      expand_ppi_code (0, p->index, p->code);
    }
}

static void
ppi_gensim ()
{
  op *p = ppi_tab;

  printf ("#define DSR_MASK_G 0x80\n");
  printf ("#define DSR_MASK_Z 0x40\n");
  printf ("#define DSR_MASK_N 0x20\n");
  printf ("#define DSR_MASK_V 0x10\n");
  printf ("\n");
  printf ("#define COMPUTE_OVERFLOW do {\\\n");
  printf ("  overflow = res_grd != SIGN32 (res) ? DSR_MASK_V : 0; \\\n");
  printf ("  if (overflow && S) \\\n");
  printf ("    { \\\n");
  printf ("      if (res_grd & 0x80) \\\n");
  printf ("        { \\\n");
  printf ("          res = 0x80000000; \\\n");
  printf ("          res_grd |=  0xff; \\\n");
  printf ("        } \\\n");
  printf ("      else \\\n");
  printf ("        { \\\n");
  printf ("          res = 0x7fffffff; \\\n");
  printf ("          res_grd &= ~0xff; \\\n");
  printf ("        } \\\n");
  printf ("      overflow = 0; \\\n");
  printf ("    } \\\n");
  printf ("} while (0)\n");
  printf ("\n");
  printf ("#define ADD_SUB_GE \\\n");
  printf ("  (greater_equal = ~(overflow << 3 & res_grd) & DSR_MASK_G)\n");
  printf ("\n");
  printf ("static void\n");
  printf ("ppi_insn (iword)\n");
  printf ("     int iword;\n");
  printf ("{\n");
  printf ("  static char e_tab[] = { 8,  9, 10,  5};\n");
  printf ("  static char f_tab[] = {10, 11,  8,  5};\n");
  printf ("  static char x_tab[] = { 8,  9,  7,  5};\n");
  printf ("  static char y_tab[] = {10, 11, 12, 14};\n");
  printf ("  static char g_tab[] = {12, 14,  7,  5};\n");
  printf ("  static char u_tab[] = { 8, 10,  7,  5};\n");
  printf ("\n");
  printf ("  int z;\n");
  printf ("  int res, res_grd;\n");
  printf ("  int carry, overflow, greater_equal;\n");
  printf ("\n");
  printf ("  switch (ppi_table[iword >> 8]) {\n");

  for (; p->name; p++)
    {
      int shift, j;
      int cond = 0;
      int havedecl = 0;
      
      char *s = p->code;

      printf ("  /* %s %s */\n", p->name, p->code);
      printf ("  case %d:      \n", p->index);

      printf ("    {\n");
      for (shift = 16; *s; )
	{
	  switch (*s)
	    {
	    case 'i':
	      printf ("      int i = (iword >> 4) & 0x7f;\n");
	      s += 6;
	      break;
	    case 'e':
	    case 'f':
	    case 'x':
	    case 'y':
	    case 'g':
	    case 'u':
	      shift -= 2;
	      printf ("      int %c = %c_tab[(iword >> %d) & 3];\n",
		      *s, *s, shift);
	      havedecl = 1;
	      s += 2;
	      break;
	    case 'c':
	      printf ("      if ((((iword >> 8) ^ DSR) & 1) == 0)\n");
	      printf ("\tbreak;\n");
	      printf ("    }\n");
	      printf ("  case %d:      \n", p->index + 1);
	      printf ("    {\n");
	      cond = 1;
	    case '0':
	    case '1':
	    case '.':
	      shift -= 2;
	      s += 2;
	      break;
	    case 'z':
	      if (havedecl)
		printf ("\n");
	      printf ("      z = iword & 0xf;\n");
	      havedecl = 2;
	      s += 4;
	      break;
	    }
	}
      if (havedecl == 1)
	printf ("\n");
      else if (havedecl == 2)
	printf ("      {\n");
      for (j = 0; j < MAX_NR_STUFF; j++)
	{
	  if (p->stuff[j])
	    {
	      printf ("      %s%s\n",
		      (havedecl == 2 ? "  " : ""),
		      p->stuff[j]);
	    }
	}
      if (havedecl == 2)
	printf ("      }\n");
      if (cond)
	{
	  printf ("      if (iword & 0x200)\n");
	  printf ("        goto assign_z;\n");
	}
      printf ("      break;\n");
      printf ("    }\n");
    }

  printf ("  default:\n");
  printf ("    {\n");
  printf ("      RAISE_EXCEPTION (SIGILL);\n");
  printf ("      return;\n");
  printf ("    }\n");
  printf ("  }\n");
  printf ("  DSR &= ~0xf1;\n");
  printf ("  if (res || res_grd)\n");
  printf ("    DSR |= greater_equal | res_grd >> 2 & DSR_MASK_N | overflow;\n");
  printf ("  else\n");
  printf ("    DSR |= DSR_MASK_Z | overflow;\n");
  printf (" assign_dc:\n");
  printf ("  switch (DSR >> 1 & 7)\n");
  printf ("    {\n");
  printf ("    case 0: /* Carry Mode */\n");
  printf ("      DSR |= carry;\n");
  printf ("    case 1: /* Negative Value Mode */\n");
  printf ("      DSR |= res_grd >> 7 & 1;\n");
  printf ("    case 2: /* Zero Value Mode */\n");
  printf ("      DSR |= DSR >> 6 & 1;\n");
  printf ("    case 3: /* Overflow mode\n");
  printf ("      DSR |= overflow >> 4;\n");
  printf ("    case 4: /* Signed Greater Than Mode */\n");
  printf ("      DSR |= DSR >> 7 & 1;\n");
  printf ("    case 4: /* Signed Greater Than Or Equal Mode */\n");
  printf ("      DSR |= greater_equal >> 7;\n");
  printf ("    }\n");
  printf (" assign_z:\n");
  printf ("  if (0xa05f >> z & 1)\n");
  printf ("    {\n");
  printf ("      RAISE_EXCEPTION (SIGILL);\n");
  printf ("      return;\n");
  printf ("    }\n");
  printf ("  DSP_R (z) = res;\n");
  printf ("  DSP_GRD (z) = res_grd;\n");
  printf ("}\n");
}

int
main (ac, av)
     int ac;
     char **av;
{
  /* verify the table before anything else */
  {
    op *p;
    for (p = tab; p->name; p++)
      {
	/* check that the code field contains 16 bits */
	if (strlen (p->code) != 16)
	  {
	    fprintf (stderr, "Code `%s' length wrong (%d) for `%s'\n",
		     p->code, strlen (p->code), p->name);
	    abort ();
	  }
      }
  }

  /* now generate the requested data */
  if (ac > 1)
    {
      if (strcmp (av[1], "-t") == 0)
	{
	  gengastab ();
	}
      else if (strcmp (av[1], "-d") == 0)
	{
	  gendefines ();
	}
      else if (strcmp (av[1], "-s") == 0)
	{
	  filltable (tab);
	  dumptable ("sh_jump_table", 1 << 16, 0);

	  memset (table, 0, sizeof table);
	  filltable (movsxy_tab);
	  ppi_moves ();
	  dumptable ("sh_dsp_table", 1 << 12, 0xf000);

	  memset (table, 0, sizeof table);
	  ppi_filltable ();
	  dumptable ("ppi_table", 1 << 8, 0);
	}
      else if (strcmp (av[1], "-x") == 0)
	{
	  filltable (tab);
	  filltable (movsxy_tab);
	  gensim ();
	}
      else if (strcmp (av[1], "-p") == 0)
	{
	  ppi_filltable ();
	  ppi_gensim ();
	}
    }
  else
    fprintf (stderr, "Opcode table generation no longer supported.\n");
  return 0;
}
