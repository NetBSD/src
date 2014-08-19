/* ppc-opc.c -- PowerPC opcode list
   Copyright 1994, 1995, 1996, 1997, 1998, 2000, 2001, 2002, 2003, 2004,
   2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
   Free Software Foundation, Inc.
   Written by Ian Lance Taylor, Cygnus Support

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include "opcode/ppc.h"
#include "opintl.h"

/* This file holds the PowerPC opcode table.  The opcode table
   includes almost all of the extended instruction mnemonics.  This
   permits the disassembler to use them, and simplifies the assembler
   logic, at the cost of increasing the table size.  The table is
   strictly constant data, so the compiler should be able to put it in
   the .text section.

   This file also holds the operand table.  All knowledge about
   inserting operands into instructions and vice-versa is kept in this
   file.  */

/* Local insertion and extraction functions.  */

static unsigned long insert_arx (unsigned long, long, ppc_cpu_t, const char **);
static long extract_arx (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_ary (unsigned long, long, ppc_cpu_t, const char **);
static long extract_ary (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_bat (unsigned long, long, ppc_cpu_t, const char **);
static long extract_bat (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_bba (unsigned long, long, ppc_cpu_t, const char **);
static long extract_bba (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_bdm (unsigned long, long, ppc_cpu_t, const char **);
static long extract_bdm (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_bdp (unsigned long, long, ppc_cpu_t, const char **);
static long extract_bdp (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_bo (unsigned long, long, ppc_cpu_t, const char **);
static long extract_bo (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_boe (unsigned long, long, ppc_cpu_t, const char **);
static long extract_boe (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_fxm (unsigned long, long, ppc_cpu_t, const char **);
static long extract_fxm (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_li20 (unsigned long, long, ppc_cpu_t, const char **);
static long extract_li20 (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_ls (unsigned long, long, ppc_cpu_t, const char **);
static unsigned long insert_mbe (unsigned long, long, ppc_cpu_t, const char **);
static long extract_mbe (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_mb6 (unsigned long, long, ppc_cpu_t, const char **);
static long extract_mb6 (unsigned long, ppc_cpu_t, int *);
static long extract_nb (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_nbi (unsigned long, long, ppc_cpu_t, const char **);
static unsigned long insert_nsi (unsigned long, long, ppc_cpu_t, const char **);
static long extract_nsi (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_oimm (unsigned long, long, ppc_cpu_t, const char **);
static long extract_oimm (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_ral (unsigned long, long, ppc_cpu_t, const char **);
static unsigned long insert_ram (unsigned long, long, ppc_cpu_t, const char **);
static unsigned long insert_raq (unsigned long, long, ppc_cpu_t, const char **);
static unsigned long insert_ras (unsigned long, long, ppc_cpu_t, const char **);
static unsigned long insert_rbs (unsigned long, long, ppc_cpu_t, const char **);
static long extract_rbs (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_rbx (unsigned long, long, ppc_cpu_t, const char **);
static unsigned long insert_rx (unsigned long, long, ppc_cpu_t, const char **);
static long extract_rx (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_ry (unsigned long, long, ppc_cpu_t, const char **);
static long extract_ry (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_sh6 (unsigned long, long, ppc_cpu_t, const char **);
static long extract_sh6 (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_sci8 (unsigned long, long, ppc_cpu_t, const char **);
static long extract_sci8 (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_sci8n (unsigned long, long, ppc_cpu_t, const char **);
static long extract_sci8n (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_sd4h (unsigned long, long, ppc_cpu_t, const char **);
static long extract_sd4h (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_sd4w (unsigned long, long, ppc_cpu_t, const char **);
static long extract_sd4w (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_spr (unsigned long, long, ppc_cpu_t, const char **);
static long extract_spr (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_sprg (unsigned long, long, ppc_cpu_t, const char **);
static long extract_sprg (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_tbr (unsigned long, long, ppc_cpu_t, const char **);
static long extract_tbr (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_xt6 (unsigned long, long, ppc_cpu_t, const char **);
static long extract_xt6 (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_xa6 (unsigned long, long, ppc_cpu_t, const char **);
static long extract_xa6 (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_xb6 (unsigned long, long, ppc_cpu_t, const char **);
static long extract_xb6 (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_xb6s (unsigned long, long, ppc_cpu_t, const char **);
static long extract_xb6s (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_xc6 (unsigned long, long, ppc_cpu_t, const char **);
static long extract_xc6 (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_dm (unsigned long, long, ppc_cpu_t, const char **);
static long extract_dm (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_vlesi (unsigned long, long, ppc_cpu_t, const char **);
static long extract_vlesi (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_vlensi (unsigned long, long, ppc_cpu_t, const char **);
static long extract_vlensi (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_vleui (unsigned long, long, ppc_cpu_t, const char **);
static long extract_vleui (unsigned long, ppc_cpu_t, int *);
static unsigned long insert_vleil (unsigned long, long, ppc_cpu_t, const char **);
static long extract_vleil (unsigned long, ppc_cpu_t, int *);

/* The operands table.

   The fields are bitm, shift, insert, extract, flags.

   We used to put parens around the various additions, like the one
   for BA just below.  However, that caused trouble with feeble
   compilers with a limit on depth of a parenthesized expression, like
   (reportedly) the compiler in Microsoft Developer Studio 5.  So we
   omit the parens, since the macros are never used in a context where
   the addition will be ambiguous.  */

const struct powerpc_operand powerpc_operands[] =
{
  /* The zero index is used to indicate the end of the list of
     operands.  */
#define UNUSED 0
  { 0, 0, NULL, NULL, 0 },

  /* The BA field in an XL form instruction.  */
#define BA UNUSED + 1
  /* The BI field in a B form or XL form instruction.  */
#define BI BA
#define BI_MASK (0x1f << 16)
  { 0x1f, 16, NULL, NULL, PPC_OPERAND_CR_BIT },

  /* The BA field in an XL form instruction when it must be the same
     as the BT field in the same instruction.  */
#define BAT BA + 1
  { 0x1f, 16, insert_bat, extract_bat, PPC_OPERAND_FAKE },

  /* The BB field in an XL form instruction.  */
#define BB BAT + 1
#define BB_MASK (0x1f << 11)
  { 0x1f, 11, NULL, NULL, PPC_OPERAND_CR_BIT },

  /* The BB field in an XL form instruction when it must be the same
     as the BA field in the same instruction.  */
#define BBA BB + 1
  /* The VB field in a VX form instruction when it must be the same
     as the VA field in the same instruction.  */
#define VBA BBA
  { 0x1f, 11, insert_bba, extract_bba, PPC_OPERAND_FAKE },

  /* The BD field in a B form instruction.  The lower two bits are
     forced to zero.  */
#define BD BBA + 1
  { 0xfffc, 0, NULL, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The BD field in a B form instruction when absolute addressing is
     used.  */
#define BDA BD + 1
  { 0xfffc, 0, NULL, NULL, PPC_OPERAND_ABSOLUTE | PPC_OPERAND_SIGNED },

  /* The BD field in a B form instruction when the - modifier is used.
     This sets the y bit of the BO field appropriately.  */
#define BDM BDA + 1
  { 0xfffc, 0, insert_bdm, extract_bdm,
      PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The BD field in a B form instruction when the - modifier is used
     and absolute address is used.  */
#define BDMA BDM + 1
  { 0xfffc, 0, insert_bdm, extract_bdm,
      PPC_OPERAND_ABSOLUTE | PPC_OPERAND_SIGNED },

  /* The BD field in a B form instruction when the + modifier is used.
     This sets the y bit of the BO field appropriately.  */
#define BDP BDMA + 1
  { 0xfffc, 0, insert_bdp, extract_bdp,
      PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The BD field in a B form instruction when the + modifier is used
     and absolute addressing is used.  */
#define BDPA BDP + 1
  { 0xfffc, 0, insert_bdp, extract_bdp,
      PPC_OPERAND_ABSOLUTE | PPC_OPERAND_SIGNED },

  /* The BF field in an X or XL form instruction.  */
#define BF BDPA + 1
  /* The CRFD field in an X form instruction.  */
#define CRFD BF
  /* The CRD field in an XL form instruction.  */
#define CRD BF
  { 0x7, 23, NULL, NULL, PPC_OPERAND_CR_REG },

  /* The BF field in an X or XL form instruction.  */
#define BFF BF + 1
  { 0x7, 23, NULL, NULL, 0 },

  /* An optional BF field.  This is used for comparison instructions,
     in which an omitted BF field is taken as zero.  */
#define OBF BFF + 1
  { 0x7, 23, NULL, NULL, PPC_OPERAND_CR_REG | PPC_OPERAND_OPTIONAL },

  /* The BFA field in an X or XL form instruction.  */
#define BFA OBF + 1
  { 0x7, 18, NULL, NULL, PPC_OPERAND_CR_REG },

  /* The BO field in a B form instruction.  Certain values are
     illegal.  */
#define BO BFA + 1
#define BO_MASK (0x1f << 21)
  { 0x1f, 21, insert_bo, extract_bo, 0 },

  /* The BO field in a B form instruction when the + or - modifier is
     used.  This is like the BO field, but it must be even.  */
#define BOE BO + 1
  { 0x1e, 21, insert_boe, extract_boe, 0 },

#define BH BOE + 1
  { 0x3, 11, NULL, NULL, PPC_OPERAND_OPTIONAL },

  /* The BT field in an X or XL form instruction.  */
#define BT BH + 1
  { 0x1f, 21, NULL, NULL, PPC_OPERAND_CR_BIT },

  /* The BI16 field in a BD8 form instruction.  */
#define BI16 BT + 1
  { 0x3, 8, NULL, NULL, PPC_OPERAND_CR_BIT },

  /* The BI32 field in a BD15 form instruction.  */
#define BI32 BI16 + 1
  { 0xf, 16, NULL, NULL, PPC_OPERAND_CR_BIT },

  /* The BO32 field in a BD15 form instruction.  */
#define BO32 BI32 + 1
  { 0x3, 20, NULL, NULL, 0 },

  /* The B8 field in a BD8 form instruction.  */
#define B8 BO32 + 1
  { 0x1fe, -1, NULL, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The B15 field in a BD15 form instruction.  The lowest bit is
     forced to zero.  */
#define B15 B8 + 1
  { 0xfffe, 0, NULL, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The B24 field in a BD24 form instruction.  The lowest bit is
     forced to zero.  */
#define B24 B15 + 1
  { 0x1fffffe, 0, NULL, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The condition register number portion of the BI field in a B form
     or XL form instruction.  This is used for the extended
     conditional branch mnemonics, which set the lower two bits of the
     BI field.  This field is optional.  */
#define CR B24 + 1
  { 0x7, 18, NULL, NULL, PPC_OPERAND_CR_REG | PPC_OPERAND_OPTIONAL },

  /* The CRB field in an X form instruction.  */
#define CRB CR + 1
  /* The MB field in an M form instruction.  */
#define MB CRB
#define MB_MASK (0x1f << 6)
  { 0x1f, 6, NULL, NULL, 0 },

  /* The CRD32 field in an XL form instruction.  */
#define CRD32 CRB + 1
  { 0x3, 21, NULL, NULL, PPC_OPERAND_CR_REG },

  /* The CRFS field in an X form instruction.  */
#define CRFS CRD32 + 1
  { 0x7, 0, NULL, NULL, PPC_OPERAND_CR_REG },

#define CRS CRFS + 1
  { 0x3, 18, NULL, NULL, PPC_OPERAND_CR_REG | PPC_OPERAND_OPTIONAL },

  /* The CT field in an X form instruction.  */
#define CT CRS + 1
  /* The MO field in an mbar instruction.  */
#define MO CT
  { 0x1f, 21, NULL, NULL, PPC_OPERAND_OPTIONAL },

  /* The D field in a D form instruction.  This is a displacement off
     a register, and implies that the next operand is a register in
     parentheses.  */
#define D CT + 1
  { 0xffff, 0, NULL, NULL, PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED },

  /* The D8 field in a D form instruction.  This is a displacement off
     a register, and implies that the next operand is a register in
     parentheses.  */
#define D8 D + 1
  { 0xff, 0, NULL, NULL, PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED },

  /* The DQ field in a DQ form instruction.  This is like D, but the
     lower four bits are forced to zero. */
#define DQ D8 + 1
  { 0xfff0, 0, NULL, NULL,
    PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED | PPC_OPERAND_DQ },

  /* The DS field in a DS form instruction.  This is like D, but the
     lower two bits are forced to zero.  */
#define DS DQ + 1
  { 0xfffc, 0, NULL, NULL,
    PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED | PPC_OPERAND_DS },

  /* The DUIS or BHRBE fields in a XFX form instruction, 10 bits
     unsigned imediate */
#define DUIS DS + 1
#define BHRBE DUIS
  { 0x3ff, 11, NULL, NULL, 0 },

  /* The E field in a wrteei instruction.  */
  /* And the W bit in the pair singles instructions.  */
  /* And the ST field in a VX form instruction.  */
#define E DUIS + 1
#define PSW E
#define ST E
  { 0x1, 15, NULL, NULL, 0 },

  /* The FL1 field in a POWER SC form instruction.  */
#define FL1 E + 1
  /* The U field in an X form instruction.  */
#define U FL1
  { 0xf, 12, NULL, NULL, 0 },

  /* The FL2 field in a POWER SC form instruction.  */
#define FL2 FL1 + 1
  { 0x7, 2, NULL, NULL, 0 },

  /* The FLM field in an XFL form instruction.  */
#define FLM FL2 + 1
  { 0xff, 17, NULL, NULL, 0 },

  /* The FRA field in an X or A form instruction.  */
#define FRA FLM + 1
#define FRA_MASK (0x1f << 16)
  { 0x1f, 16, NULL, NULL, PPC_OPERAND_FPR },

  /* The FRAp field of DFP instructions.  */
#define FRAp FRA + 1
  { 0x1e, 16, NULL, NULL, PPC_OPERAND_FPR },

  /* The FRB field in an X or A form instruction.  */
#define FRB FRAp + 1
#define FRB_MASK (0x1f << 11)
  { 0x1f, 11, NULL, NULL, PPC_OPERAND_FPR },

  /* The FRBp field of DFP instructions.  */
#define FRBp FRB + 1
  { 0x1e, 11, NULL, NULL, PPC_OPERAND_FPR },

  /* The FRC field in an A form instruction.  */
#define FRC FRBp + 1
#define FRC_MASK (0x1f << 6)
  { 0x1f, 6, NULL, NULL, PPC_OPERAND_FPR },

  /* The FRS field in an X form instruction or the FRT field in a D, X
     or A form instruction.  */
#define FRS FRC + 1
#define FRT FRS
  { 0x1f, 21, NULL, NULL, PPC_OPERAND_FPR },

  /* The FRSp field of stfdp or the FRTp field of lfdp and DFP
     instructions.  */
#define FRSp FRS + 1
#define FRTp FRSp
  { 0x1e, 21, NULL, NULL, PPC_OPERAND_FPR },

  /* The FXM field in an XFX instruction.  */
#define FXM FRSp + 1
  { 0xff, 12, insert_fxm, extract_fxm, 0 },

  /* Power4 version for mfcr.  */
#define FXM4 FXM + 1
  { 0xff, 12, insert_fxm, extract_fxm, PPC_OPERAND_OPTIONAL },

  /* The IMM20 field in an LI instruction.  */
#define IMM20 FXM4 + 1
  { 0xfffff, PPC_OPSHIFT_INV, insert_li20, extract_li20, PPC_OPERAND_SIGNED},

  /* The L field in a D or X form instruction.  */
#define L IMM20 + 1
  /* The R field in a HTM X form instruction.  */
#define HTM_R L
  { 0x1, 21, NULL, NULL, PPC_OPERAND_OPTIONAL },

  /* The LEV field in a POWER SVC form instruction.  */
#define SVC_LEV L + 1
  { 0x7f, 5, NULL, NULL, 0 },

  /* The LEV field in an SC form instruction.  */
#define LEV SVC_LEV + 1
  { 0x7f, 5, NULL, NULL, PPC_OPERAND_OPTIONAL },

  /* The LI field in an I form instruction.  The lower two bits are
     forced to zero.  */
#define LI LEV + 1
  { 0x3fffffc, 0, NULL, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The LI field in an I form instruction when used as an absolute
     address.  */
#define LIA LI + 1
  { 0x3fffffc, 0, NULL, NULL, PPC_OPERAND_ABSOLUTE | PPC_OPERAND_SIGNED },

  /* The LS or WC field in an X (sync or wait) form instruction.  */
#define LS LIA + 1
#define WC LS
  { 0x3, 21, NULL, NULL, PPC_OPERAND_OPTIONAL },

  /* The ME field in an M form instruction.  */
#define ME LS + 1
#define ME_MASK (0x1f << 1)
  { 0x1f, 1, NULL, NULL, 0 },

  /* The MB and ME fields in an M form instruction expressed a single
     operand which is a bitmask indicating which bits to select.  This
     is a two operand form using PPC_OPERAND_NEXT.  See the
     description in opcode/ppc.h for what this means.  */
#define MBE ME + 1
  { 0x1f, 6, NULL, NULL, PPC_OPERAND_OPTIONAL | PPC_OPERAND_NEXT },
  { -1, 0, insert_mbe, extract_mbe, 0 },

  /* The MB or ME field in an MD or MDS form instruction.  The high
     bit is wrapped to the low end.  */
#define MB6 MBE + 2
#define ME6 MB6
#define MB6_MASK (0x3f << 5)
  { 0x3f, 5, insert_mb6, extract_mb6, 0 },

  /* The NB field in an X form instruction.  The value 32 is stored as
     0.  */
#define NB MB6 + 1
  { 0x1f, 11, NULL, extract_nb, PPC_OPERAND_PLUS1 },

  /* The NBI field in an lswi instruction, which has special value
     restrictions.  The value 32 is stored as 0.  */
#define NBI NB + 1
  { 0x1f, 11, insert_nbi, extract_nb, PPC_OPERAND_PLUS1 },

  /* The NSI field in a D form instruction.  This is the same as the
     SI field, only negated.  */
#define NSI NBI + 1
  { 0xffff, 0, insert_nsi, extract_nsi,
      PPC_OPERAND_NEGATIVE | PPC_OPERAND_SIGNED },

  /* The RA field in an D, DS, DQ, X, XO, M, or MDS form instruction.  */
#define RA NSI + 1
#define RA_MASK (0x1f << 16)
  { 0x1f, 16, NULL, NULL, PPC_OPERAND_GPR },

  /* As above, but 0 in the RA field means zero, not r0.  */
#define RA0 RA + 1
  { 0x1f, 16, NULL, NULL, PPC_OPERAND_GPR_0 },

  /* The RA field in the DQ form lq or an lswx instruction, which have special
     value restrictions.  */
#define RAQ RA0 + 1
#define RAX RAQ
  { 0x1f, 16, insert_raq, NULL, PPC_OPERAND_GPR_0 },

  /* The RA field in a D or X form instruction which is an updating
     load, which means that the RA field may not be zero and may not
     equal the RT field.  */
#define RAL RAQ + 1
  { 0x1f, 16, insert_ral, NULL, PPC_OPERAND_GPR_0 },

  /* The RA field in an lmw instruction, which has special value
     restrictions.  */
#define RAM RAL + 1
  { 0x1f, 16, insert_ram, NULL, PPC_OPERAND_GPR_0 },

  /* The RA field in a D or X form instruction which is an updating
     store or an updating floating point load, which means that the RA
     field may not be zero.  */
#define RAS RAM + 1
  { 0x1f, 16, insert_ras, NULL, PPC_OPERAND_GPR_0 },

  /* The RA field of the tlbwe, dccci and iccci instructions,
     which are optional.  */
#define RAOPT RAS + 1
  { 0x1f, 16, NULL, NULL, PPC_OPERAND_GPR | PPC_OPERAND_OPTIONAL },

  /* The RB field in an X, XO, M, or MDS form instruction.  */
#define RB RAOPT + 1
#define RB_MASK (0x1f << 11)
  { 0x1f, 11, NULL, NULL, PPC_OPERAND_GPR },

  /* The RB field in an X form instruction when it must be the same as
     the RS field in the instruction.  This is used for extended
     mnemonics like mr.  */
#define RBS RB + 1
  { 0x1f, 11, insert_rbs, extract_rbs, PPC_OPERAND_FAKE },

  /* The RB field in an lswx instruction, which has special value
     restrictions.  */
#define RBX RBS + 1
  { 0x1f, 11, insert_rbx, NULL, PPC_OPERAND_GPR },

  /* The RB field of the dccci and iccci instructions, which are optional.  */
#define RBOPT RBX + 1
  { 0x1f, 11, NULL, NULL, PPC_OPERAND_GPR | PPC_OPERAND_OPTIONAL },

  /* The RS field in a D, DS, X, XFX, XS, M, MD or MDS form
     instruction or the RT field in a D, DS, X, XFX or XO form
     instruction.  */
#define RS RBOPT + 1
#define RT RS
#define RT_MASK (0x1f << 21)
#define RD RS
  { 0x1f, 21, NULL, NULL, PPC_OPERAND_GPR },

  /* The RS and RT fields of the DS form stq and DQ form lq instructions,
     which have special value restrictions.  */
#define RSQ RS + 1
#define RTQ RSQ
  { 0x1e, 21, NULL, NULL, PPC_OPERAND_GPR },

  /* The RS field of the tlbwe instruction, which is optional.  */
#define RSO RSQ + 1
#define RTO RSO
  { 0x1f, 21, NULL, NULL, PPC_OPERAND_GPR | PPC_OPERAND_OPTIONAL },

  /* The RX field of the SE_RR form instruction.  */
#define RX RSO + 1
  { 0x1f, PPC_OPSHIFT_INV, insert_rx, extract_rx, PPC_OPERAND_GPR },

  /* The ARX field of the SE_RR form instruction.  */
#define ARX RX + 1
  { 0x1f, PPC_OPSHIFT_INV, insert_arx, extract_arx, PPC_OPERAND_GPR },

  /* The RY field of the SE_RR form instruction.  */
#define RY ARX + 1
#define RZ RY
  { 0x1f, PPC_OPSHIFT_INV, insert_ry, extract_ry, PPC_OPERAND_GPR },

  /* The ARY field of the SE_RR form instruction.  */
#define ARY RY + 1
  { 0x1f, PPC_OPSHIFT_INV, insert_ary, extract_ary, PPC_OPERAND_GPR },

  /* The SCLSCI8 field in a D form instruction.  */
#define SCLSCI8 ARY + 1
  { 0xffffffff, PPC_OPSHIFT_INV, insert_sci8, extract_sci8, 0 },

  /* The SCLSCI8N field in a D form instruction.  This is the same as the
     SCLSCI8 field, only negated.  */
#define SCLSCI8N SCLSCI8 + 1
  { 0xffffffff, PPC_OPSHIFT_INV, insert_sci8n, extract_sci8n,
      PPC_OPERAND_NEGATIVE | PPC_OPERAND_SIGNED },

  /* The SD field of the SD4 form instruction.  */
#define SE_SD SCLSCI8N + 1
  { 0xf, 8, NULL, NULL, PPC_OPERAND_PARENS },

  /* The SD field of the SD4 form instruction, for halfword.  */
#define SE_SDH SE_SD + 1
  { 0x1e, PPC_OPSHIFT_INV, insert_sd4h, extract_sd4h, PPC_OPERAND_PARENS },

  /* The SD field of the SD4 form instruction, for word.  */
#define SE_SDW SE_SDH + 1
  { 0x3c, PPC_OPSHIFT_INV, insert_sd4w, extract_sd4w, PPC_OPERAND_PARENS },

  /* The SH field in an X or M form instruction.  */
#define SH SE_SDW + 1
#define SH_MASK (0x1f << 11)
  /* The other UIMM field in a EVX form instruction.  */
#define EVUIMM SH
  { 0x1f, 11, NULL, NULL, 0 },

  /* The SI field in a HTM X form instruction.  */
#define HTM_SI SH + 1
  { 0x1f, 11, NULL, NULL, PPC_OPERAND_SIGNED },

  /* The SH field in an MD form instruction.  This is split.  */
#define SH6 HTM_SI + 1
#define SH6_MASK ((0x1f << 11) | (1 << 1))
  { 0x3f, PPC_OPSHIFT_INV, insert_sh6, extract_sh6, 0 },

  /* The SH field of the tlbwe instruction, which is optional.  */
#define SHO SH6 + 1
  { 0x1f, 11, NULL, NULL, PPC_OPERAND_OPTIONAL },

  /* The SI field in a D form instruction.  */
#define SI SHO + 1
  { 0xffff, 0, NULL, NULL, PPC_OPERAND_SIGNED },

  /* The SI field in a D form instruction when we accept a wide range
     of positive values.  */
#define SISIGNOPT SI + 1
  { 0xffff, 0, NULL, NULL, PPC_OPERAND_SIGNED | PPC_OPERAND_SIGNOPT },

  /* The SI8 field in a D form instruction.  */
#define SI8 SISIGNOPT + 1
  { 0xff, 0, NULL, NULL, PPC_OPERAND_SIGNED },

  /* The SPR field in an XFX form instruction.  This is flipped--the
     lower 5 bits are stored in the upper 5 and vice- versa.  */
#define SPR SI8 + 1
#define PMR SPR
#define TMR SPR
#define SPR_MASK (0x3ff << 11)
  { 0x3ff, 11, insert_spr, extract_spr, 0 },

  /* The BAT index number in an XFX form m[ft]ibat[lu] instruction.  */
#define SPRBAT SPR + 1
#define SPRBAT_MASK (0x3 << 17)
  { 0x3, 17, NULL, NULL, 0 },

  /* The SPRG register number in an XFX form m[ft]sprg instruction.  */
#define SPRG SPRBAT + 1
  { 0x1f, 16, insert_sprg, extract_sprg, 0 },

  /* The SR field in an X form instruction.  */
#define SR SPRG + 1
  /* The 4-bit UIMM field in a VX form instruction.  */
#define UIMM4 SR
  { 0xf, 16, NULL, NULL, 0 },

  /* The STRM field in an X AltiVec form instruction.  */
#define STRM SR + 1
  /* The T field in a tlbilx form instruction.  */
#define T STRM
  { 0x3, 21, NULL, NULL, 0 },

  /* The ESYNC field in an X (sync) form instruction.  */
#define ESYNC STRM + 1
  { 0xf, 16, insert_ls, NULL, PPC_OPERAND_OPTIONAL },

  /* The SV field in a POWER SC form instruction.  */
#define SV ESYNC + 1
  { 0x3fff, 2, NULL, NULL, 0 },

  /* The TBR field in an XFX form instruction.  This is like the SPR
     field, but it is optional.  */
#define TBR SV + 1
  { 0x3ff, 11, insert_tbr, extract_tbr, PPC_OPERAND_OPTIONAL },

  /* The TO field in a D or X form instruction.  */
#define TO TBR + 1
#define DUI TO
#define TO_MASK (0x1f << 21)
  { 0x1f, 21, NULL, NULL, 0 },

  /* The UI field in a D form instruction.  */
#define UI TO + 1
  { 0xffff, 0, NULL, NULL, 0 },

  /* The IMM field in an SE_IM5 instruction.  */
#define UI5 UI + 1
  { 0x1f, 4, NULL, NULL, 0 },

  /* The OIMM field in an SE_OIM5 instruction.  */
#define OIMM5 UI5 + 1
  { 0x1f, PPC_OPSHIFT_INV, insert_oimm, extract_oimm, PPC_OPERAND_PLUS1 },

  /* The UI7 field in an SE_LI instruction.  */
#define UI7 OIMM5 + 1
  { 0x7f, 4, NULL, NULL, 0 },

  /* The VA field in a VA, VX or VXR form instruction.  */
#define VA UI7 + 1
  { 0x1f, 16, NULL, NULL, PPC_OPERAND_VR },

  /* The VB field in a VA, VX or VXR form instruction.  */
#define VB VA + 1
  { 0x1f, 11, NULL, NULL, PPC_OPERAND_VR },

  /* The VC field in a VA form instruction.  */
#define VC VB + 1
  { 0x1f, 6, NULL, NULL, PPC_OPERAND_VR },

  /* The VD or VS field in a VA, VX, VXR or X form instruction.  */
#define VD VC + 1
#define VS VD
  { 0x1f, 21, NULL, NULL, PPC_OPERAND_VR },

  /* The SIMM field in a VX form instruction, and TE in Z form.  */
#define SIMM VD + 1
#define TE SIMM
  { 0x1f, 16, NULL, NULL, PPC_OPERAND_SIGNED},

  /* The UIMM field in a VX form instruction.  */
#define UIMM SIMM + 1
#define DCTL UIMM
  { 0x1f, 16, NULL, NULL, 0 },

  /* The 3-bit UIMM field in a VX form instruction.  */
#define UIMM3 UIMM + 1
  { 0x7, 16, NULL, NULL, 0 },

  /* The SIX field in a VX form instruction.  */
#define SIX UIMM3 + 1
  { 0xf, 11, NULL, NULL, 0 },

  /* The PS field in a VX form instruction.  */
#define PS SIX + 1
  { 0x1, 9, NULL, NULL, 0 },

  /* The SHB field in a VA form instruction.  */
#define SHB PS + 1
  { 0xf, 6, NULL, NULL, 0 },

  /* The other UIMM field in a half word EVX form instruction.  */
#define EVUIMM_2 SHB + 1
  { 0x3e, 10, NULL, NULL, PPC_OPERAND_PARENS },

  /* The other UIMM field in a word EVX form instruction.  */
#define EVUIMM_4 EVUIMM_2 + 1
  { 0x7c, 9, NULL, NULL, PPC_OPERAND_PARENS },

  /* The other UIMM field in a double EVX form instruction.  */
#define EVUIMM_8 EVUIMM_4 + 1
  { 0xf8, 8, NULL, NULL, PPC_OPERAND_PARENS },

  /* The WS field.  */
#define WS EVUIMM_8 + 1
  { 0x7, 11, NULL, NULL, 0 },

  /* PowerPC paired singles extensions.  */
  /* W bit in the pair singles instructions for x type instructions.  */
#define PSWM WS + 1
  /* The BO16 field in a BD8 form instruction.  */
#define BO16 PSWM
  {  0x1, 10, 0, 0, 0 },

  /* IDX bits for quantization in the pair singles instructions.  */
#define PSQ PSWM + 1
  {  0x7, 12, 0, 0, 0 },

  /* IDX bits for quantization in the pair singles x-type instructions.  */
#define PSQM PSQ + 1
  {  0x7, 7, 0, 0, 0 },

  /* Smaller D field for quantization in the pair singles instructions.  */
#define PSD PSQM + 1
  {  0xfff, 0, 0, 0,  PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED },

#define A_L PSD + 1
#define W A_L
#define MTMSRD_L W
  { 0x1, 16, NULL, NULL, PPC_OPERAND_OPTIONAL },

#define RMC MTMSRD_L + 1
  { 0x3, 9, NULL, NULL, 0 },

#define R RMC + 1
  { 0x1, 16, NULL, NULL, 0 },

#define SP R + 1
  { 0x3, 19, NULL, NULL, 0 },

#define S SP + 1
  { 0x1, 20, NULL, NULL, 0 },

  /* The S field in a XL form instruction.  */
#define SXL S + 1
  { 0x1, 11, NULL, NULL, PPC_OPERAND_OPTIONAL },

  /* SH field starting at bit position 16.  */
#define SH16 SXL + 1
  /* The DCM and DGM fields in a Z form instruction.  */
#define DCM SH16
#define DGM DCM
  { 0x3f, 10, NULL, NULL, 0 },

  /* The EH field in larx instruction.  */
#define EH SH16 + 1
  { 0x1, 0, NULL, NULL, PPC_OPERAND_OPTIONAL },

  /* The L field in an mtfsf or XFL form instruction.  */
  /* The A field in a HTM X form instruction.  */
#define XFL_L EH + 1
#define HTM_A XFL_L
  { 0x1, 25, NULL, NULL, PPC_OPERAND_OPTIONAL},

  /* Xilinx APU related masks and macros */
#define FCRT XFL_L + 1
#define FCRT_MASK (0x1f << 21)
  { 0x1f, 21, 0, 0, PPC_OPERAND_FCR },

  /* Xilinx FSL related masks and macros */  
#define FSL FCRT + 1
#define FSL_MASK (0x1f << 11)
  { 0x1f, 11, 0, 0, PPC_OPERAND_FSL },  

  /* Xilinx UDI related masks and macros */  
#define URT FSL + 1
  { 0x1f, 21, 0, 0, PPC_OPERAND_UDI },

#define URA URT + 1
  { 0x1f, 16, 0, 0, PPC_OPERAND_UDI },

#define URB URA + 1
  { 0x1f, 11, 0, 0, PPC_OPERAND_UDI },

#define URC URB + 1
  { 0x1f, 6, 0, 0, PPC_OPERAND_UDI },

  /* The VLESIMM field in a D form instruction.  */
#define VLESIMM URC + 1
  { 0xffff, PPC_OPSHIFT_INV, insert_vlesi, extract_vlesi,
      PPC_OPERAND_SIGNED | PPC_OPERAND_SIGNOPT },

  /* The VLENSIMM field in a D form instruction.  */
#define VLENSIMM VLESIMM + 1
  { 0xffff, PPC_OPSHIFT_INV, insert_vlensi, extract_vlensi,
      PPC_OPERAND_NEGATIVE | PPC_OPERAND_SIGNED | PPC_OPERAND_SIGNOPT },

  /* The VLEUIMM field in a D form instruction.  */
#define VLEUIMM VLENSIMM + 1
  { 0xffff, PPC_OPSHIFT_INV, insert_vleui, extract_vleui, 0 },

  /* The VLEUIMML field in a D form instruction.  */
#define VLEUIMML VLEUIMM + 1
  { 0xffff, PPC_OPSHIFT_INV, insert_vleil, extract_vleil, 0 },

  /* The XT and XS fields in an XX1 or XX3 form instruction.  This is split.  */
#define XS6 VLEUIMML + 1
#define XT6 XS6
  { 0x3f, PPC_OPSHIFT_INV, insert_xt6, extract_xt6, PPC_OPERAND_VSR },

  /* The XA field in an XX3 form instruction.  This is split.  */
#define XA6 XT6 + 1
  { 0x3f, PPC_OPSHIFT_INV, insert_xa6, extract_xa6, PPC_OPERAND_VSR },

  /* The XB field in an XX2 or XX3 form instruction.  This is split.  */
#define XB6 XA6 + 1
  { 0x3f, PPC_OPSHIFT_INV, insert_xb6, extract_xb6, PPC_OPERAND_VSR },

  /* The XB field in an XX3 form instruction when it must be the same as
     the XA field in the instruction.  This is used in extended mnemonics
     like xvmovdp.  This is split.  */
#define XB6S XB6 + 1
  { 0x3f, PPC_OPSHIFT_INV, insert_xb6s, extract_xb6s, PPC_OPERAND_FAKE },

  /* The XC field in an XX4 form instruction.  This is split.  */
#define XC6 XB6S + 1
  { 0x3f, PPC_OPSHIFT_INV, insert_xc6, extract_xc6, PPC_OPERAND_VSR },

  /* The DM or SHW field in an XX3 form instruction.  */
#define DM XC6 + 1
#define SHW DM
  { 0x3, 8, NULL, NULL, 0 },

  /* The DM field in an extended mnemonic XX3 form instruction.  */
#define DMEX DM + 1
  { 0x3, 8, insert_dm, extract_dm, 0 },

  /* The UIM field in an XX2 form instruction.  */
#define UIM DMEX + 1
  /* The 2-bit UIMM field in a VX form instruction.  */
#define UIMM2 UIM
  { 0x3, 16, NULL, NULL, 0 },

#define ERAT_T UIM + 1
  { 0x7, 21, NULL, NULL, 0 },
};

const unsigned int num_powerpc_operands = (sizeof (powerpc_operands)
					   / sizeof (powerpc_operands[0]));

/* The functions used to insert and extract complicated operands.  */

/* The ARX, ARY, RX and RY operands are alternate encodings of GPRs.  */

static unsigned long
insert_arx (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value >= 8 && value < 24)
    return insn | ((value - 8) & 0xf);
  else
    {
      *errmsg = _("invalid register");
      return 0;
    }
}

static long
extract_arx (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{ 
  return (insn & 0xf) + 8;
}

static unsigned long
insert_ary (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value >= 8 && value < 24)
    return insn | (((value - 8) & 0xf) << 4);
  else
    {
      *errmsg = _("invalid register");
      return 0;
    }
}

static long
extract_ary (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 4) & 0xf) + 8;
}

static unsigned long
insert_rx (unsigned long insn,
	   long value,
	   ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	   const char **errmsg)
{
  if (value >= 0 && value < 8)
    return insn | value;
  else if (value >= 24 && value <= 31)
    return insn | (value - 16);
  else
    {
      *errmsg = _("invalid register");
      return 0;
    }
}

static long
extract_rx (unsigned long insn,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    int *invalid ATTRIBUTE_UNUSED)
{
  int value = insn & 0xf;
  if (value >= 0 && value < 8)
    return value;
  else
    return value + 16;
}

static unsigned long
insert_ry (unsigned long insn,
	   long value,
	   ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	   const char **errmsg)
{
  if (value >= 0 && value < 8)
    return insn | (value << 4);
  else if (value >= 24 && value <= 31)
    return insn | ((value - 16) << 4);
  else
    {
      *errmsg = _("invalid register");
      return 0;
    }
}

static long
extract_ry (unsigned long insn,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    int *invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 4) & 0xf;
  if (value >= 0 && value < 8)
    return value;
  else
    return value + 16;
}

/* The BA field in an XL form instruction when it must be the same as
   the BT field in the same instruction.  This operand is marked FAKE.
   The insertion function just copies the BT field into the BA field,
   and the extraction function just checks that the fields are the
   same.  */

static unsigned long
insert_bat (unsigned long insn,
	    long value ATTRIBUTE_UNUSED,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | (((insn >> 21) & 0x1f) << 16);
}

static long
extract_bat (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid)
{
  if (((insn >> 21) & 0x1f) != ((insn >> 16) & 0x1f))
    *invalid = 1;
  return 0;
}

/* The BB field in an XL form instruction when it must be the same as
   the BA field in the same instruction.  This operand is marked FAKE.
   The insertion function just copies the BA field into the BB field,
   and the extraction function just checks that the fields are the
   same.  */

static unsigned long
insert_bba (unsigned long insn,
	    long value ATTRIBUTE_UNUSED,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | (((insn >> 16) & 0x1f) << 11);
}

static long
extract_bba (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid)
{
  if (((insn >> 16) & 0x1f) != ((insn >> 11) & 0x1f))
    *invalid = 1;
  return 0;
}

/* The BD field in a B form instruction when the - modifier is used.
   This modifier means that the branch is not expected to be taken.
   For chips built to versions of the architecture prior to version 2
   (ie. not Power4 compatible), we set the y bit of the BO field to 1
   if the offset is negative.  When extracting, we require that the y
   bit be 1 and that the offset be positive, since if the y bit is 0
   we just want to print the normal form of the instruction.
   Power4 compatible targets use two bits, "a", and "t", instead of
   the "y" bit.  "at" == 00 => no hint, "at" == 01 => unpredictable,
   "at" == 10 => not taken, "at" == 11 => taken.  The "t" bit is 00001
   in BO field, the "a" bit is 00010 for branch on CR(BI) and 01000
   for branch on CTR.  We only handle the taken/not-taken hint here.
   Note that we don't relax the conditions tested here when
   disassembling with -Many because insns using extract_bdm and
   extract_bdp always occur in pairs.  One or the other will always
   be valid.  */

#define ISA_V2 (PPC_OPCODE_POWER4 | PPC_OPCODE_E500MC | PPC_OPCODE_TITAN)

static unsigned long
insert_bdm (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if ((dialect & ISA_V2) == 0)
    {
      if ((value & 0x8000) != 0)
	insn |= 1 << 21;
    }
  else
    {
      if ((insn & (0x14 << 21)) == (0x04 << 21))
	insn |= 0x02 << 21;
      else if ((insn & (0x14 << 21)) == (0x10 << 21))
	insn |= 0x08 << 21;
    }
  return insn | (value & 0xfffc);
}

static long
extract_bdm (unsigned long insn,
	     ppc_cpu_t dialect,
	     int *invalid)
{
  if ((dialect & ISA_V2) == 0)
    {
      if (((insn & (1 << 21)) == 0) != ((insn & (1 << 15)) == 0))
	*invalid = 1;
    }
  else
    {
      if ((insn & (0x17 << 21)) != (0x06 << 21)
	  && (insn & (0x1d << 21)) != (0x18 << 21))
	*invalid = 1;
    }

  return ((insn & 0xfffc) ^ 0x8000) - 0x8000;
}

/* The BD field in a B form instruction when the + modifier is used.
   This is like BDM, above, except that the branch is expected to be
   taken.  */

static unsigned long
insert_bdp (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if ((dialect & ISA_V2) == 0)
    {
      if ((value & 0x8000) == 0)
	insn |= 1 << 21;
    }
  else
    {
      if ((insn & (0x14 << 21)) == (0x04 << 21))
	insn |= 0x03 << 21;
      else if ((insn & (0x14 << 21)) == (0x10 << 21))
	insn |= 0x09 << 21;
    }
  return insn | (value & 0xfffc);
}

static long
extract_bdp (unsigned long insn,
	     ppc_cpu_t dialect,
	     int *invalid)
{
  if ((dialect & ISA_V2) == 0)
    {
      if (((insn & (1 << 21)) == 0) == ((insn & (1 << 15)) == 0))
	*invalid = 1;
    }
  else
    {
      if ((insn & (0x17 << 21)) != (0x07 << 21)
	  && (insn & (0x1d << 21)) != (0x19 << 21))
	*invalid = 1;
    }

  return ((insn & 0xfffc) ^ 0x8000) - 0x8000;
}

static inline int
valid_bo_pre_v2 (long value)
{
  /* Certain encodings have bits that are required to be zero.
     These are (z must be zero, y may be anything):
	 0000y 
	 0001y 
	 001zy
	 0100y 
	 0101y 
	 011zy
	 1z00y
	 1z01y
	 1z1zz
  */
  if ((value & 0x14) == 0)
    return 1;
  else if ((value & 0x14) == 0x4)
    return (value & 0x2) == 0;
  else if ((value & 0x14) == 0x10)
    return (value & 0x8) == 0;
  else
    return value == 0x14;
}

static inline int
valid_bo_post_v2 (long value)
{
  /* Certain encodings have bits that are required to be zero.
     These are (z must be zero, a & t may be anything):
	 0000z
	 0001z
	 001at
	 0100z
	 0101z
	 011at
	 1a00t
	 1a01t
	 1z1zz
  */
  if ((value & 0x14) == 0)
    return (value & 0x1) == 0;
  else if ((value & 0x14) == 0x14)
    return value == 0x14;
  else
    return 1;
}

/* Check for legal values of a BO field.  */

static int
valid_bo (long value, ppc_cpu_t dialect, int extract)
{
  int valid_y = valid_bo_pre_v2 (value);
  int valid_at = valid_bo_post_v2 (value);

  /* When disassembling with -Many, accept either encoding on the
     second pass through opcodes.  */
  if (extract && dialect == ~(ppc_cpu_t) PPC_OPCODE_ANY)
    return valid_y || valid_at;
  if ((dialect & ISA_V2) == 0)
    return valid_y;
  else
    return valid_at;
}

/* The BO field in a B form instruction.  Warn about attempts to set
   the field to an illegal value.  */

static unsigned long
insert_bo (unsigned long insn,
	   long value,
	   ppc_cpu_t dialect,
	   const char **errmsg)
{
  if (!valid_bo (value, dialect, 0))
    *errmsg = _("invalid conditional option");
  else if (PPC_OP (insn) == 19 && (insn & 0x400) && ! (value & 4))
    *errmsg = _("invalid counter access");
  return insn | ((value & 0x1f) << 21);
}

static long
extract_bo (unsigned long insn,
	    ppc_cpu_t dialect,
	    int *invalid)
{
  long value;

  value = (insn >> 21) & 0x1f;
  if (!valid_bo (value, dialect, 1))
    *invalid = 1;
  return value;
}

/* The BO field in a B form instruction when the + or - modifier is
   used.  This is like the BO field, but it must be even.  When
   extracting it, we force it to be even.  */

static unsigned long
insert_boe (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect,
	    const char **errmsg)
{
  if (!valid_bo (value, dialect, 0))
    *errmsg = _("invalid conditional option");
  else if (PPC_OP (insn) == 19 && (insn & 0x400) && ! (value & 4))
    *errmsg = _("invalid counter access");
  else if ((value & 1) != 0)
    *errmsg = _("attempt to set y bit when using + or - modifier");

  return insn | ((value & 0x1f) << 21);
}

static long
extract_boe (unsigned long insn,
	     ppc_cpu_t dialect,
	     int *invalid)
{
  long value;

  value = (insn >> 21) & 0x1f;
  if (!valid_bo (value, dialect, 1))
    *invalid = 1;
  return value & 0x1e;
}

/* FXM mask in mfcr and mtcrf instructions.  */

static unsigned long
insert_fxm (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect,
	    const char **errmsg)
{
  /* If we're handling the mfocrf and mtocrf insns ensure that exactly
     one bit of the mask field is set.  */
  if ((insn & (1 << 20)) != 0)
    {
      if (value == 0 || (value & -value) != value)
	{
	  *errmsg = _("invalid mask field");
	  value = 0;
	}
    }

  /* If the optional field on mfcr is missing that means we want to use
     the old form of the instruction that moves the whole cr.  In that
     case we'll have VALUE zero.  There doesn't seem to be a way to
     distinguish this from the case where someone writes mfcr %r3,0.  */
  else if (value == 0)
    ;

  /* If only one bit of the FXM field is set, we can use the new form
     of the instruction, which is faster.  Unlike the Power4 branch hint
     encoding, this is not backward compatible.  Do not generate the
     new form unless -mpower4 has been given, or -many and the two
     operand form of mfcr was used.  */
  else if ((value & -value) == value
	   && ((dialect & PPC_OPCODE_POWER4) != 0
	       || ((dialect & PPC_OPCODE_ANY) != 0
		   && (insn & (0x3ff << 1)) == 19 << 1)))
    insn |= 1 << 20;

  /* Any other value on mfcr is an error.  */
  else if ((insn & (0x3ff << 1)) == 19 << 1)
    {
      *errmsg = _("ignoring invalid mfcr mask");
      value = 0;
    }

  return insn | ((value & 0xff) << 12);
}

static long
extract_fxm (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid)
{
  long mask = (insn >> 12) & 0xff;

  /* Is this a Power4 insn?  */
  if ((insn & (1 << 20)) != 0)
    {
      /* Exactly one bit of MASK should be set.  */
      if (mask == 0 || (mask & -mask) != mask)
	*invalid = 1;
    }

  /* Check that non-power4 form of mfcr has a zero MASK.  */
  else if ((insn & (0x3ff << 1)) == 19 << 1)
    {
      if (mask != 0)
	*invalid = 1;
    }

  return mask;
}

static unsigned long
insert_li20 (unsigned long insn,
	     long value,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0xf0000) >> 5) | ((value & 0x0f800) << 5) | (value & 0x7ff);
}

static long
extract_li20 (unsigned long insn,
	      ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  long ext = ((insn & 0x4000) == 0x4000) ? 0xfff00000 : 0x00000000;

  return ext
         | (((insn >> 11) & 0xf) << 16)
         | (((insn >> 17) & 0xf) << 12)
         | (((insn >> 16) & 0x1) << 11)
         | (insn & 0x7ff);
}

/* The LS field in a sync instruction that accepts 2 operands
   Values 2 and 3 are reserved,
     must be treated as 0 for future compatibility
   Values 0 and 1 can be accepted, if field ESYNC is zero
   Otherwise L = complement of ESYNC-bit2 (1<<18) */

static unsigned long
insert_ls (unsigned long insn,
	   long value,
	   ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  unsigned long ls;

  ls = (insn >> 21) & 0x03;
  if (value == 0)
    {
      if (ls > 1)
	return insn & ~(0x3 << 21);
      return insn;
    }
  if ((value & 0x2) != 0)
    return (insn & ~(0x3 << 21)) | ((value & 0xf) << 16);
  return (insn & ~(0x3 << 21)) | (0x1 << 21) | ((value & 0xf) << 16);
}

/* The MB and ME fields in an M form instruction expressed as a single
   operand which is itself a bitmask.  The extraction function always
   marks it as invalid, since we never want to recognize an
   instruction which uses a field of this type.  */

static unsigned long
insert_mbe (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg)
{
  unsigned long uval, mask;
  int mb, me, mx, count, last;

  uval = value;

  if (uval == 0)
    {
      *errmsg = _("illegal bitmask");
      return insn;
    }

  mb = 0;
  me = 32;
  if ((uval & 1) != 0)
    last = 1;
  else
    last = 0;
  count = 0;

  /* mb: location of last 0->1 transition */
  /* me: location of last 1->0 transition */
  /* count: # transitions */

  for (mx = 0, mask = 1L << 31; mx < 32; ++mx, mask >>= 1)
    {
      if ((uval & mask) && !last)
	{
	  ++count;
	  mb = mx;
	  last = 1;
	}
      else if (!(uval & mask) && last)
	{
	  ++count;
	  me = mx;
	  last = 0;
	}
    }
  if (me == 0)
    me = 32;

  if (count != 2 && (count != 0 || ! last))
    *errmsg = _("illegal bitmask");

  return insn | (mb << 6) | ((me - 1) << 1);
}

static long
extract_mbe (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid)
{
  long ret;
  int mb, me;
  int i;

  *invalid = 1;

  mb = (insn >> 6) & 0x1f;
  me = (insn >> 1) & 0x1f;
  if (mb < me + 1)
    {
      ret = 0;
      for (i = mb; i <= me; i++)
	ret |= 1L << (31 - i);
    }
  else if (mb == me + 1)
    ret = ~0;
  else /* (mb > me + 1) */
    {
      ret = ~0;
      for (i = me + 1; i < mb; i++)
	ret &= ~(1L << (31 - i));
    }
  return ret;
}

/* The MB or ME field in an MD or MDS form instruction.  The high bit
   is wrapped to the low end.  */

static unsigned long
insert_mb6 (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x1f) << 6) | (value & 0x20);
}

static long
extract_mb6 (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 6) & 0x1f) | (insn & 0x20);
}

/* The NB field in an X form instruction.  The value 32 is stored as
   0.  */

static long
extract_nb (unsigned long insn,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    int *invalid ATTRIBUTE_UNUSED)
{
  long ret;

  ret = (insn >> 11) & 0x1f;
  if (ret == 0)
    ret = 32;
  return ret;
}

/* The NB field in an lswi instruction, which has special value
   restrictions.  The value 32 is stored as 0.  */

static unsigned long
insert_nbi (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  long rtvalue = (insn & RT_MASK) >> 21;
  long ravalue = (insn & RA_MASK) >> 16;

  if (value == 0)
    value = 32;
  if (rtvalue + (value + 3) / 4 > (rtvalue > ravalue ? ravalue + 32
						     : ravalue))
    *errmsg = _("address register in load range");
  return insn | ((value & 0x1f) << 11);
}

/* The NSI field in a D form instruction.  This is the same as the SI
   field, only negated.  The extraction function always marks it as
   invalid, since we never want to recognize an instruction which uses
   a field of this type.  */

static unsigned long
insert_nsi (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | (-value & 0xffff);
}

static long
extract_nsi (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid)
{
  *invalid = 1;
  return -(((insn & 0xffff) ^ 0x8000) - 0x8000);
}

/* The RA field in a D or X form instruction which is an updating
   load, which means that the RA field may not be zero and may not
   equal the RT field.  */

static unsigned long
insert_ral (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg)
{
  if (value == 0
      || (unsigned long) value == ((insn >> 21) & 0x1f))
    *errmsg = "invalid register operand when updating";
  return insn | ((value & 0x1f) << 16);
}

/* The RA field in an lmw instruction, which has special value
   restrictions.  */

static unsigned long
insert_ram (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg)
{
  if ((unsigned long) value >= ((insn >> 21) & 0x1f))
    *errmsg = _("index register in load range");
  return insn | ((value & 0x1f) << 16);
}

/* The RA field in the DQ form lq or an lswx instruction, which have special
   value restrictions.  */

static unsigned long
insert_raq (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg)
{
  long rtvalue = (insn & RT_MASK) >> 21;

  if (value == rtvalue)
    *errmsg = _("source and target register operands must be different");
  return insn | ((value & 0x1f) << 16);
}

/* The RA field in a D or X form instruction which is an updating
   store or an updating floating point load, which means that the RA
   field may not be zero.  */

static unsigned long
insert_ras (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg)
{
  if (value == 0)
    *errmsg = _("invalid register operand when updating");
  return insn | ((value & 0x1f) << 16);
}

/* The RB field in an X form instruction when it must be the same as
   the RS field in the instruction.  This is used for extended
   mnemonics like mr.  This operand is marked FAKE.  The insertion
   function just copies the BT field into the BA field, and the
   extraction function just checks that the fields are the same.  */

static unsigned long
insert_rbs (unsigned long insn,
	    long value ATTRIBUTE_UNUSED,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | (((insn >> 21) & 0x1f) << 11);
}

static long
extract_rbs (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid)
{
  if (((insn >> 21) & 0x1f) != ((insn >> 11) & 0x1f))
    *invalid = 1;
  return 0;
}

/* The RB field in an lswx instruction, which has special value
   restrictions.  */

static unsigned long
insert_rbx (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg)
{
  long rtvalue = (insn & RT_MASK) >> 21;

  if (value == rtvalue)
    *errmsg = _("source and target register operands must be different");
  return insn | ((value & 0x1f) << 11);
}

/* The SCI8 field is made up of SCL and {U,N}I8 fields.  */
static unsigned long
insert_sci8 (unsigned long insn,
	     long value,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     const char **errmsg)
{
  unsigned int fill_scale = 0;
  unsigned long ui8 = value;

  if ((ui8 & 0xffffff00) == 0)
    ;
  else if ((ui8 & 0xffffff00) == 0xffffff00)
    fill_scale = 0x400;
  else if ((ui8 & 0xffff00ff) == 0)
    {
      fill_scale = 1 << 8;
      ui8 >>= 8;
    }
  else if ((ui8 & 0xffff00ff) == 0xffff00ff)
    {
      fill_scale = 0x400 | (1 << 8);
      ui8 >>= 8;
    }
  else if ((ui8 & 0xff00ffff) == 0)
    {
      fill_scale = 2 << 8;
      ui8 >>= 16;
    }
  else if ((ui8 & 0xff00ffff) == 0xff00ffff)
    {
      fill_scale = 0x400 | (2 << 8);
      ui8 >>= 16;
    }
  else if ((ui8 & 0x00ffffff) == 0)
    {
      fill_scale = 3 << 8;
      ui8 >>= 24;
    }
  else if ((ui8 & 0x00ffffff) == 0x00ffffff)
    {
      fill_scale = 0x400 | (3 << 8);
      ui8 >>= 24;
    }
  else
    {
      *errmsg = _("illegal immediate value");
      ui8 = 0;
    }

  return insn | fill_scale | (ui8 & 0xff);
}

static long
extract_sci8 (unsigned long insn,
	      ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  int fill = insn & 0x400;
  int scale_factor = (insn & 0x300) >> 5;
  long value = (insn & 0xff) << scale_factor;

  if (fill != 0)
    value |= ~((long) 0xff << scale_factor);
  return value;
}

static unsigned long
insert_sci8n (unsigned long insn,
	      long value,
	      ppc_cpu_t dialect,
	      const char **errmsg)
{
  return insert_sci8 (insn, -value, dialect, errmsg);
}

static long
extract_sci8n (unsigned long insn,
	       ppc_cpu_t dialect,
	       int *invalid)
{
  return -extract_sci8 (insn, dialect, invalid);
}

static unsigned long
insert_sd4h (unsigned long insn,
	     long value,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x1e) << 7);
}

static long
extract_sd4h (unsigned long insn,
	      ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 8) & 0xf) << 1;
}

static unsigned long
insert_sd4w (unsigned long insn,
	     long value,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x3c) << 6);
}

static long
extract_sd4w (unsigned long insn,
	      ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 8) & 0xf) << 2;
}

static unsigned long
insert_oimm (unsigned long insn,
	     long value,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | (((value - 1) & 0x1f) << 4);
}

static long
extract_oimm (unsigned long insn,
	      ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 4) & 0x1f) + 1;
}

/* The SH field in an MD form instruction.  This is split.  */

static unsigned long
insert_sh6 (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x1f) << 11) | ((value & 0x20) >> 4);
}

static long
extract_sh6 (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 11) & 0x1f) | ((insn << 4) & 0x20);
}

/* The SPR field in an XFX form instruction.  This is flipped--the
   lower 5 bits are stored in the upper 5 and vice- versa.  */

static unsigned long
insert_spr (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x1f) << 16) | ((value & 0x3e0) << 6);
}

static long
extract_spr (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 16) & 0x1f) | ((insn >> 6) & 0x3e0);
}

/* Some dialects have 8 SPRG registers instead of the standard 4.  */
#define ALLOW8_SPRG (PPC_OPCODE_BOOKE | PPC_OPCODE_405 | PPC_OPCODE_VLE)

static unsigned long
insert_sprg (unsigned long insn,
	     long value,
	     ppc_cpu_t dialect,
	     const char **errmsg)
{
  if (value > 7
      || (value > 3 && (dialect & ALLOW8_SPRG) == 0))
    *errmsg = _("invalid sprg number");

  /* If this is mfsprg4..7 then use spr 260..263 which can be read in
     user mode.  Anything else must use spr 272..279.  */
  if (value <= 3 || (insn & 0x100) != 0)
    value |= 0x10;

  return insn | ((value & 0x17) << 16);
}

static long
extract_sprg (unsigned long insn,
	      ppc_cpu_t dialect,
	      int *invalid)
{
  unsigned long val = (insn >> 16) & 0x1f;

  /* mfsprg can use 260..263 and 272..279.  mtsprg only uses spr 272..279
     If not BOOKE, 405 or VLE, then both use only 272..275.  */
  if ((val - 0x10 > 3 && (dialect & ALLOW8_SPRG) == 0)
      || (val - 0x10 > 7 && (insn & 0x100) != 0)
      || val <= 3
      || (val & 8) != 0)
    *invalid = 1;
  return val & 7;
}

/* The TBR field in an XFX instruction.  This is just like SPR, but it
   is optional.  When TBR is omitted, it must be inserted as 268 (the
   magic number of the TB register).  These functions treat 0
   (indicating an omitted optional operand) as 268.  This means that
   ``mftb 4,0'' is not handled correctly.  This does not matter very
   much, since the architecture manual does not define mftb as
   accepting any values other than 268 or 269.  */

#define TB (268)

static unsigned long
insert_tbr (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value == 0)
    value = TB;
  return insn | ((value & 0x1f) << 16) | ((value & 0x3e0) << 6);
}

static long
extract_tbr (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  long ret;

  ret = ((insn >> 16) & 0x1f) | ((insn >> 6) & 0x3e0);
  if (ret == TB)
    ret = 0;
  return ret;
}

/* The XT and XS fields in an XX1 or XX3 form instruction.  This is split.  */

static unsigned long
insert_xt6 (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x1f) << 21) | ((value & 0x20) >> 5);
}

static long
extract_xt6 (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn << 5) & 0x20) | ((insn >> 21) & 0x1f);
}

/* The XA field in an XX3 form instruction.  This is split.  */

static unsigned long
insert_xa6 (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x1f) << 16) | ((value & 0x20) >> 3);
}

static long
extract_xa6 (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn << 3) & 0x20) | ((insn >> 16) & 0x1f);
}

/* The XB field in an XX3 form instruction.  This is split.  */

static unsigned long
insert_xb6 (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x1f) << 11) | ((value & 0x20) >> 4);
}

static long
extract_xb6 (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn << 4) & 0x20) | ((insn >> 11) & 0x1f);
}

/* The XB field in an XX3 form instruction when it must be the same as
   the XA field in the instruction.  This is used for extended
   mnemonics like xvmovdp.  This operand is marked FAKE.  The insertion
   function just copies the XA field into the XB field, and the
   extraction function just checks that the fields are the same.  */

static unsigned long
insert_xb6s (unsigned long insn,
	    long value ATTRIBUTE_UNUSED,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | (((insn >> 16) & 0x1f) << 11) | (((insn >> 2) & 0x1) << 1);
}

static long
extract_xb6s (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid)
{
  if ((((insn >> 16) & 0x1f) != ((insn >> 11) & 0x1f))
      || (((insn >> 2) & 0x1) != ((insn >> 1) & 0x1)))
    *invalid = 1;
  return 0;
}

/* The XC field in an XX4 form instruction.  This is split.  */

static unsigned long
insert_xc6 (unsigned long insn,
	    long value,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x1f) << 6) | ((value & 0x20) >> 2);
}

static long
extract_xc6 (unsigned long insn,
	     ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn << 2) & 0x20) | ((insn >> 6) & 0x1f);
}

static unsigned long
insert_dm (unsigned long insn,
	   long value,
	   ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	   const char **errmsg)
{
  if (value != 0 && value != 1)
    *errmsg = _("invalid constant");
  return insn | (((value) ? 3 : 0) << 8);
}

static long
extract_dm (unsigned long insn,
	    ppc_cpu_t dialect ATTRIBUTE_UNUSED,
	    int *invalid)
{
  long value;

  value = (insn >> 8) & 3;
  if (value != 0 && value != 3)
    *invalid = 1;
  return (value) ? 1 : 0;
}
/* The VLESIMM field in an I16A form instruction.  This is split.  */

static unsigned long
insert_vlesi (unsigned long insn,
            long value,
            ppc_cpu_t dialect ATTRIBUTE_UNUSED,
            const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0xf800) << 10) | (value & 0x7ff);
}

static long
extract_vlesi (unsigned long insn,
             ppc_cpu_t dialect ATTRIBUTE_UNUSED,
             int *invalid ATTRIBUTE_UNUSED)
{
  long value = ((insn >> 10) & 0xf800) | (insn & 0x7ff);
  value = (value ^ 0x8000) - 0x8000;
  return value;
}

static unsigned long
insert_vlensi (unsigned long insn,
            long value,
            ppc_cpu_t dialect ATTRIBUTE_UNUSED,
            const char **errmsg ATTRIBUTE_UNUSED)
{
  value = -value;
  return insn | ((value & 0xf800) << 10) | (value & 0x7ff);
}
static long
extract_vlensi (unsigned long insn,
             ppc_cpu_t dialect ATTRIBUTE_UNUSED,
             int *invalid ATTRIBUTE_UNUSED)
{
  long value = ((insn >> 10) & 0xf800) | (insn & 0x7ff);
  value = (value ^ 0x8000) - 0x8000;
  /* Don't use for disassembly.  */
  *invalid = 1;
  return -value;
}

/* The VLEUIMM field in an I16A form instruction.  This is split.  */

static unsigned long
insert_vleui (unsigned long insn,
            long value,
            ppc_cpu_t dialect ATTRIBUTE_UNUSED,
            const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0xf800) << 10) | (value & 0x7ff);
}

static long
extract_vleui (unsigned long insn,
             ppc_cpu_t dialect ATTRIBUTE_UNUSED,
             int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 10) & 0xf800) | (insn & 0x7ff);
}

/* The VLEUIMML field in an I16L form instruction.  This is split.  */

static unsigned long
insert_vleil (unsigned long insn,
            long value,
            ppc_cpu_t dialect ATTRIBUTE_UNUSED,
            const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0xf800) << 5) | (value & 0x7ff);
}

static long
extract_vleil (unsigned long insn,
             ppc_cpu_t dialect ATTRIBUTE_UNUSED,
             int *invalid ATTRIBUTE_UNUSED)
{
  return ((insn >> 5) & 0xf800) | (insn & 0x7ff);
}


/* Macros used to form opcodes.  */

/* The main opcode.  */
#define OP(x) ((((unsigned long)(x)) & 0x3f) << 26)
#define OP_MASK OP (0x3f)

/* The main opcode combined with a trap code in the TO field of a D
   form instruction.  Used for extended mnemonics for the trap
   instructions.  */
#define OPTO(x,to) (OP (x) | ((((unsigned long)(to)) & 0x1f) << 21))
#define OPTO_MASK (OP_MASK | TO_MASK)

/* The main opcode combined with a comparison size bit in the L field
   of a D form or X form instruction.  Used for extended mnemonics for
   the comparison instructions.  */
#define OPL(x,l) (OP (x) | ((((unsigned long)(l)) & 1) << 21))
#define OPL_MASK OPL (0x3f,1)

/* The main opcode combined with an update code in D form instruction.
   Used for extended mnemonics for VLE memory instructions.  */
#define OPVUP(x,vup) (OP (x) | ((((unsigned long)(vup)) & 0xff) << 8))
#define OPVUP_MASK OPVUP (0x3f,  0xff)

/* An A form instruction.  */
#define A(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 1) | (((unsigned long)(rc)) & 1))
#define A_MASK A (0x3f, 0x1f, 1)

/* An A_MASK with the FRB field fixed.  */
#define AFRB_MASK (A_MASK | FRB_MASK)

/* An A_MASK with the FRC field fixed.  */
#define AFRC_MASK (A_MASK | FRC_MASK)

/* An A_MASK with the FRA and FRC fields fixed.  */
#define AFRAFRC_MASK (A_MASK | FRA_MASK | FRC_MASK)

/* An AFRAFRC_MASK, but with L bit clear.  */
#define AFRALFRC_MASK (AFRAFRC_MASK & ~((unsigned long) 1 << 16))

/* A B form instruction.  */
#define B(op, aa, lk) (OP (op) | ((((unsigned long)(aa)) & 1) << 1) | ((lk) & 1))
#define B_MASK B (0x3f, 1, 1)

/* A BD8 form instruction.  This is a 16-bit instruction.  */
#define BD8(op, aa, lk) (((((unsigned long)(op)) & 0x3f) << 10) | (((aa) & 1) << 9) | (((lk) & 1) << 8))
#define BD8_MASK BD8 (0x3f, 1, 1)

/* Another BD8 form instruction.  This is a 16-bit instruction.  */
#define BD8IO(op) ((((unsigned long)(op)) & 0x1f) << 11)
#define BD8IO_MASK BD8IO (0x1f)

/* A BD8 form instruction for simplified mnemonics.  */
#define EBD8IO(op, bo, bi) (BD8IO ((op)) | ((bo) << 10) | ((bi) << 8))
/* A mask that excludes BO32 and BI32.  */
#define EBD8IO1_MASK 0xf800
/* A mask that includes BO32 and excludes BI32.  */
#define EBD8IO2_MASK 0xfc00
/* A mask that include BO32 AND BI32.  */
#define EBD8IO3_MASK 0xff00

/* A BD15 form instruction.  */
#define BD15(op, aa, lk) (OP (op) | ((((unsigned long)(aa)) & 0xf) << 22) | ((lk) & 1))
#define BD15_MASK BD15 (0x3f, 0xf, 1)

/* A BD15 form instruction for extended conditional branch mnemonics.  */
#define EBD15(op, aa, bo, lk) (((op) & 0x3f) << 26) | (((aa) & 0xf) << 22) | (((bo) & 0x3) << 20) | ((lk) & 1)
#define EBD15_MASK 0xfff00001

/* A BD15 form instruction for extended conditional branch mnemonics with BI.  */
#define EBD15BI(op, aa, bo, bi, lk) (((op) & 0x3f) << 26) \
                                    | (((aa) & 0xf) << 22) \
                                    | (((bo) & 0x3) << 20) \
                                    | (((bi) & 0x3) << 16) \
                                    | ((lk) & 1)
#define EBD15BI_MASK  0xfff30001

/* A BD24 form instruction.  */
#define BD24(op, aa, lk) (OP (op) | ((((unsigned long)(aa)) & 1) << 25) | ((lk) & 1))
#define BD24_MASK BD24 (0x3f, 1, 1)

/* A B form instruction setting the BO field.  */
#define BBO(op, bo, aa, lk) (B ((op), (aa), (lk)) | ((((unsigned long)(bo)) & 0x1f) << 21))
#define BBO_MASK BBO (0x3f, 0x1f, 1, 1)

/* A BBO_MASK with the y bit of the BO field removed.  This permits
   matching a conditional branch regardless of the setting of the y
   bit.  Similarly for the 'at' bits used for power4 branch hints.  */
#define Y_MASK	 (((unsigned long) 1) << 21)
#define AT1_MASK (((unsigned long) 3) << 21)
#define AT2_MASK (((unsigned long) 9) << 21)
#define BBOY_MASK  (BBO_MASK &~ Y_MASK)
#define BBOAT_MASK (BBO_MASK &~ AT1_MASK)

/* A B form instruction setting the BO field and the condition bits of
   the BI field.  */
#define BBOCB(op, bo, cb, aa, lk) \
  (BBO ((op), (bo), (aa), (lk)) | ((((unsigned long)(cb)) & 0x3) << 16))
#define BBOCB_MASK BBOCB (0x3f, 0x1f, 0x3, 1, 1)

/* A BBOCB_MASK with the y bit of the BO field removed.  */
#define BBOYCB_MASK (BBOCB_MASK &~ Y_MASK)
#define BBOATCB_MASK (BBOCB_MASK &~ AT1_MASK)
#define BBOAT2CB_MASK (BBOCB_MASK &~ AT2_MASK)

/* A BBOYCB_MASK in which the BI field is fixed.  */
#define BBOYBI_MASK (BBOYCB_MASK | BI_MASK)
#define BBOATBI_MASK (BBOAT2CB_MASK | BI_MASK)

/* A VLE C form instruction.  */
#define C_LK(x, lk) (((((unsigned long)(x)) & 0x7fff) << 1) | ((lk) & 1))
#define C_LK_MASK C_LK(0x7fff, 1)
#define C(x) ((((unsigned long)(x)) & 0xffff))
#define C_MASK C(0xffff)

/* An Context form instruction.  */
#define CTX(op, xop)   (OP (op) | (((unsigned long)(xop)) & 0x7))
#define CTX_MASK CTX(0x3f, 0x7)

/* An User Context form instruction.  */
#define UCTX(op, xop)  (OP (op) | (((unsigned long)(xop)) & 0x1f))
#define UCTX_MASK UCTX(0x3f, 0x1f)

/* The main opcode mask with the RA field clear.  */
#define DRA_MASK (OP_MASK | RA_MASK)

/* A DS form instruction.  */
#define DSO(op, xop) (OP (op) | ((xop) & 0x3))
#define DS_MASK DSO (0x3f, 3)

/* An EVSEL form instruction.  */
#define EVSEL(op, xop) (OP (op) | (((unsigned long)(xop)) & 0xff) << 3)
#define EVSEL_MASK EVSEL(0x3f, 0xff)

/* An IA16 form instruction.  */
#define IA16(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x1f) << 11)
#define IA16_MASK IA16(0x3f, 0x1f)

/* An I16A form instruction.  */
#define I16A(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x1f) << 11)
#define I16A_MASK I16A(0x3f, 0x1f)

/* An I16L form instruction.  */
#define I16L(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x1f) << 11)
#define I16L_MASK I16L(0x3f, 0x1f)

/* An IM7 form instruction.  */
#define IM7(op) ((((unsigned long)(op)) & 0x1f) << 11)
#define IM7_MASK IM7(0x1f)

/* An M form instruction.  */
#define M(op, rc) (OP (op) | ((rc) & 1))
#define M_MASK M (0x3f, 1)

/* An LI20 form instruction.  */
#define LI20(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x1) << 15)
#define LI20_MASK LI20(0x3f, 0x1)

/* An M form instruction with the ME field specified.  */
#define MME(op, me, rc) (M ((op), (rc)) | ((((unsigned long)(me)) & 0x1f) << 1))

/* An M_MASK with the MB and ME fields fixed.  */
#define MMBME_MASK (M_MASK | MB_MASK | ME_MASK)

/* An M_MASK with the SH and ME fields fixed.  */
#define MSHME_MASK (M_MASK | SH_MASK | ME_MASK)

/* An MD form instruction.  */
#define MD(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x7) << 2) | ((rc) & 1))
#define MD_MASK MD (0x3f, 0x7, 1)

/* An MD_MASK with the MB field fixed.  */
#define MDMB_MASK (MD_MASK | MB6_MASK)

/* An MD_MASK with the SH field fixed.  */
#define MDSH_MASK (MD_MASK | SH6_MASK)

/* An MDS form instruction.  */
#define MDS(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0xf) << 1) | ((rc) & 1))
#define MDS_MASK MDS (0x3f, 0xf, 1)

/* An MDS_MASK with the MB field fixed.  */
#define MDSMB_MASK (MDS_MASK | MB6_MASK)

/* An SC form instruction.  */
#define SC(op, sa, lk) (OP (op) | ((((unsigned long)(sa)) & 1) << 1) | ((lk) & 1))
#define SC_MASK (OP_MASK | (((unsigned long)0x3ff) << 16) | (((unsigned long)1) << 1) | 1)

/* An SCI8 form instruction.  */
#define SCI8(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 11))
#define SCI8_MASK SCI8(0x3f, 0x1f)

/* An SCI8 form instruction.  */
#define SCI8BF(op, fop, xop) (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 11) | (((fop) & 7) << 23))
#define SCI8BF_MASK SCI8BF(0x3f, 7, 0x1f)

/* An SD4 form instruction.  This is a 16-bit instruction.  */
#define SD4(op) ((((unsigned long)(op)) & 0xf) << 12) 
#define SD4_MASK SD4(0xf)

/* An SE_IM5 form instruction.  This is a 16-bit instruction.  */
#define SE_IM5(op, xop) (((((unsigned long)(op)) & 0x3f) << 10) | (((xop) & 0x1) << 9))
#define SE_IM5_MASK SE_IM5(0x3f, 1)

/* An SE_R form instruction.  This is a 16-bit instruction.  */
#define SE_R(op, xop) (((((unsigned long)(op)) & 0x3f) << 10) | (((xop) & 0x3f) << 4))
#define SE_R_MASK SE_R(0x3f, 0x3f)

/* An SE_RR form instruction.  This is a 16-bit instruction.  */
#define SE_RR(op, xop) (((((unsigned long)(op)) & 0x3f) << 10) | (((xop) & 0x3) << 8))
#define SE_RR_MASK SE_RR(0x3f, 3)

/* A VX form instruction.  */
#define VX(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x7ff))

/* The mask for an VX form instruction.  */
#define VX_MASK	VX(0x3f, 0x7ff)

/* A VX_MASK with the VA field fixed.  */
#define VXVA_MASK (VX_MASK | (0x1f << 16))

/* A VX_MASK with the VB field fixed.  */
#define VXVB_MASK (VX_MASK | (0x1f << 11))

/* A VX_MASK with the VA and VB fields fixed.  */
#define VXVAVB_MASK (VX_MASK | (0x1f << 16) | (0x1f << 11))

/* A VX_MASK with the VD and VA fields fixed.  */
#define VXVDVA_MASK (VX_MASK | (0x1f << 21) | (0x1f << 16))

/* A VX_MASK with a UIMM4 field.  */
#define VXUIMM4_MASK (VX_MASK | (0x1 << 20))

/* A VX_MASK with a UIMM3 field.  */
#define VXUIMM3_MASK (VX_MASK | (0x3 << 19))

/* A VX_MASK with a UIMM2 field.  */
#define VXUIMM2_MASK (VX_MASK | (0x7 << 18))

/* A VX_MASK with a PS field.  */
#define VXPS_MASK (VX_MASK & ~(0x1 << 9))

/* A VA form instruction.  */
#define VXA(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x03f))

/* The mask for an VA form instruction.  */
#define VXA_MASK VXA(0x3f, 0x3f)

/* A VXA_MASK with a SHB field.  */
#define VXASHB_MASK (VXA_MASK | (1 << 10))

/* A VXR form instruction.  */
#define VXR(op, xop, rc) (OP (op) | (((rc) & 1) << 10) | (((unsigned long)(xop)) & 0x3ff))

/* The mask for a VXR form instruction.  */
#define VXR_MASK VXR(0x3f, 0x3ff, 1)

/* An X form instruction.  */
#define X(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1))

/* An EX form instruction.  */
#define EX(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x7ff))

/* The mask for an EX form instruction.  */
#define EX_MASK EX (0x3f, 0x7ff)

/* An XX2 form instruction.  */
#define XX2(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 2))

/* An XX3 form instruction.  */
#define XX3(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0xff) << 3))

/* An XX3 form instruction with the RC bit specified.  */
#define XX3RC(op, xop, rc) (OP (op) | (((rc) & 1) << 10) | ((((unsigned long)(xop)) & 0x7f) << 3))

/* An XX4 form instruction.  */
#define XX4(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3) << 4))

/* A Z form instruction.  */
#define Z(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 1))

/* An X form instruction with the RC bit specified.  */
#define XRC(op, xop, rc) (X ((op), (xop)) | ((rc) & 1))

/* A Z form instruction with the RC bit specified.  */
#define ZRC(op, xop, rc) (Z ((op), (xop)) | ((rc) & 1))

/* The mask for an X form instruction.  */
#define X_MASK XRC (0x3f, 0x3ff, 1)

/* An X form wait instruction with everything filled in except the WC field.  */
#define XWC_MASK (XRC (0x3f, 0x3ff, 1) | (7 << 23) | RA_MASK | RB_MASK)

/* The mask for an XX1 form instruction.  */
#define XX1_MASK X (0x3f, 0x3ff)

/* An XX1_MASK with the RB field fixed.  */
#define XX1RB_MASK (XX1_MASK | RB_MASK)

/* The mask for an XX2 form instruction.  */
#define XX2_MASK (XX2 (0x3f, 0x1ff) | (0x1f << 16))

/* The mask for an XX2 form instruction with the UIM bits specified.  */
#define XX2UIM_MASK (XX2 (0x3f, 0x1ff) | (7 << 18))

/* The mask for an XX2 form instruction with the BF bits specified.  */
#define XX2BF_MASK (XX2_MASK | (3 << 21) | (1))

/* The mask for an XX3 form instruction.  */
#define XX3_MASK XX3 (0x3f, 0xff)

/* The mask for an XX3 form instruction with the BF bits specified.  */
#define XX3BF_MASK (XX3 (0x3f, 0xff) | (3 << 21) | (1))

/* The mask for an XX3 form instruction with the DM or SHW bits specified.  */
#define XX3DM_MASK (XX3 (0x3f, 0x1f) | (1 << 10))
#define XX3SHW_MASK XX3DM_MASK

/* The mask for an XX4 form instruction.  */
#define XX4_MASK XX4 (0x3f, 0x3)

/* An X form wait instruction with everything filled in except the WC field.  */
#define XWC_MASK (XRC (0x3f, 0x3ff, 1) | (7 << 23) | RA_MASK | RB_MASK)

/* The mask for a Z form instruction.  */
#define Z_MASK ZRC (0x3f, 0x1ff, 1)
#define Z2_MASK ZRC (0x3f, 0xff, 1)

/* An X_MASK with the RA field fixed.  */
#define XRA_MASK (X_MASK | RA_MASK)

/* An XRA_MASK with the W field clear.  */
#define XWRA_MASK (XRA_MASK & ~((unsigned long) 1 << 16))

/* An X_MASK with the RB field fixed.  */
#define XRB_MASK (X_MASK | RB_MASK)

/* An X_MASK with the RT field fixed.  */
#define XRT_MASK (X_MASK | RT_MASK)

/* An XRT_MASK mask with the L bits clear.  */
#define XLRT_MASK (XRT_MASK & ~((unsigned long) 0x3 << 21))

/* An X_MASK with the RA and RB fields fixed.  */
#define XRARB_MASK (X_MASK | RA_MASK | RB_MASK)

/* An XRARB_MASK, but with the L bit clear.  */
#define XRLARB_MASK (XRARB_MASK & ~((unsigned long) 1 << 16))

/* An X_MASK with the RT and RA fields fixed.  */
#define XRTRA_MASK (X_MASK | RT_MASK | RA_MASK)

/* An X_MASK with the RT and RB fields fixed.  */
#define XRTRB_MASK (X_MASK | RT_MASK | RB_MASK)

/* An XRTRA_MASK, but with L bit clear.  */
#define XRTLRA_MASK (XRTRA_MASK & ~((unsigned long) 1 << 21))

/* An X_MASK with the RT, RA and RB fields fixed.  */
#define XRTRARB_MASK (X_MASK | RT_MASK | RA_MASK | RB_MASK)

/* An XRTRARB_MASK, but with L bit clear.  */
#define XRTLRARB_MASK (XRTRARB_MASK & ~((unsigned long) 1 << 21))

/* An XRTRARB_MASK, but with A bit clear.  */
#define XRTARARB_MASK (XRTRARB_MASK & ~((unsigned long) 1 << 25))

/* An XRTRARB_MASK, but with BF bits clear.  */
#define XRTBFRARB_MASK (XRTRARB_MASK & ~((unsigned long) 7 << 23))

/* An X form instruction with the L bit specified.  */
#define XOPL(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 1) << 21))

/* An X form instruction with the L bits specified.  */
#define XOPL2(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 3) << 21))

/* An X form instruction with the L bit and RC bit specified.  */
#define XRCL(op, xop, l, rc) (XRC ((op), (xop), (rc)) | ((((unsigned long)(l)) & 1) << 21))

/* An X form instruction with RT fields specified */
#define XRT(op, xop, rt) (X ((op), (xop)) \
        | ((((unsigned long)(rt)) & 0x1f) << 21))

/* An X form instruction with RT and RA fields specified */
#define XRTRA(op, xop, rt, ra) (X ((op), (xop)) \
        | ((((unsigned long)(rt)) & 0x1f) << 21) \
        | ((((unsigned long)(ra)) & 0x1f) << 16))

/* The mask for an X form comparison instruction.  */
#define XCMP_MASK (X_MASK | (((unsigned long)1) << 22))

/* The mask for an X form comparison instruction with the L field
   fixed.  */
#define XCMPL_MASK (XCMP_MASK | (((unsigned long)1) << 21))

/* An X form trap instruction with the TO field specified.  */
#define XTO(op, xop, to) (X ((op), (xop)) | ((((unsigned long)(to)) & 0x1f) << 21))
#define XTO_MASK (X_MASK | TO_MASK)

/* An X form tlb instruction with the SH field specified.  */
#define XTLB(op, xop, sh) (X ((op), (xop)) | ((((unsigned long)(sh)) & 0x1f) << 11))
#define XTLB_MASK (X_MASK | SH_MASK)

/* An X form sync instruction.  */
#define XSYNC(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 3) << 21))

/* An X form sync instruction with everything filled in except the LS field.  */
#define XSYNC_MASK (0xff9fffff)

/* An X form sync instruction with everything filled in except the L and E fields.  */
#define XSYNCLE_MASK (0xff90ffff)

/* An X_MASK, but with the EH bit clear.  */
#define XEH_MASK (X_MASK & ~((unsigned long )1))

/* An X form AltiVec dss instruction.  */
#define XDSS(op, xop, a) (X ((op), (xop)) | ((((unsigned long)(a)) & 1) << 25))
#define XDSS_MASK XDSS(0x3f, 0x3ff, 1)

/* An XFL form instruction.  */
#define XFL(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1) | (((unsigned long)(rc)) & 1))
#define XFL_MASK XFL (0x3f, 0x3ff, 1)

/* An X form isel instruction.  */
#define XISEL(op, xop)	(OP (op) | ((((unsigned long)(xop)) & 0x1f) << 1))
#define XISEL_MASK	XISEL(0x3f, 0x1f)

/* An XL form instruction with the LK field set to 0.  */
#define XL(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1))

/* An XL form instruction which uses the LK field.  */
#define XLLK(op, xop, lk) (XL ((op), (xop)) | ((lk) & 1))

/* The mask for an XL form instruction.  */
#define XL_MASK XLLK (0x3f, 0x3ff, 1)

/* An XL_MASK with the RT, RA and RB fields fixed, but S bit clear.  */
#define XLS_MASK ((XL_MASK | RT_MASK | RA_MASK | RB_MASK) & ~(1 << 11))

/* An XL form instruction which explicitly sets the BO field.  */
#define XLO(op, bo, xop, lk) \
  (XLLK ((op), (xop), (lk)) | ((((unsigned long)(bo)) & 0x1f) << 21))
#define XLO_MASK (XL_MASK | BO_MASK)

/* An XL form instruction which explicitly sets the y bit of the BO
   field.  */
#define XLYLK(op, xop, y, lk) (XLLK ((op), (xop), (lk)) | ((((unsigned long)(y)) & 1) << 21))
#define XLYLK_MASK (XL_MASK | Y_MASK)

/* An XL form instruction which sets the BO field and the condition
   bits of the BI field.  */
#define XLOCB(op, bo, cb, xop, lk) \
  (XLO ((op), (bo), (xop), (lk)) | ((((unsigned long)(cb)) & 3) << 16))
#define XLOCB_MASK XLOCB (0x3f, 0x1f, 0x3, 0x3ff, 1)

/* An XL_MASK or XLYLK_MASK or XLOCB_MASK with the BB field fixed.  */
#define XLBB_MASK (XL_MASK | BB_MASK)
#define XLYBB_MASK (XLYLK_MASK | BB_MASK)
#define XLBOCBBB_MASK (XLOCB_MASK | BB_MASK)

/* A mask for branch instructions using the BH field.  */
#define XLBH_MASK (XL_MASK | (0x1c << 11))

/* An XL_MASK with the BO and BB fields fixed.  */
#define XLBOBB_MASK (XL_MASK | BO_MASK | BB_MASK)

/* An XL_MASK with the BO, BI and BB fields fixed.  */
#define XLBOBIBB_MASK (XL_MASK | BO_MASK | BI_MASK | BB_MASK)

/* An X form mbar instruction with MO field.  */
#define XMBAR(op, xop, mo) (X ((op), (xop)) | ((((unsigned long)(mo)) & 1) << 21))

/* An XO form instruction.  */
#define XO(op, xop, oe, rc) \
  (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 1) | ((((unsigned long)(oe)) & 1) << 10) | (((unsigned long)(rc)) & 1))
#define XO_MASK XO (0x3f, 0x1ff, 1, 1)

/* An XO_MASK with the RB field fixed.  */
#define XORB_MASK (XO_MASK | RB_MASK)

/* An XOPS form instruction for paired singles.  */
#define XOPS(op, xop, rc) \
  (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1) | (((unsigned long)(rc)) & 1))
#define XOPS_MASK XOPS (0x3f, 0x3ff, 1)


/* An XS form instruction.  */
#define XS(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 2) | (((unsigned long)(rc)) & 1))
#define XS_MASK XS (0x3f, 0x1ff, 1)

/* A mask for the FXM version of an XFX form instruction.  */
#define XFXFXM_MASK (X_MASK | (1 << 11) | (1 << 20))

/* An XFX form instruction with the FXM field filled in.  */
#define XFXM(op, xop, fxm, p4) \
  (X ((op), (xop)) | ((((unsigned long)(fxm)) & 0xff) << 12) \
   | ((unsigned long)(p4) << 20))

/* An XFX form instruction with the SPR field filled in.  */
#define XSPR(op, xop, spr) \
  (X ((op), (xop)) | ((((unsigned long)(spr)) & 0x1f) << 16) | ((((unsigned long)(spr)) & 0x3e0) << 6))
#define XSPR_MASK (X_MASK | SPR_MASK)

/* An XFX form instruction with the SPR field filled in except for the
   SPRBAT field.  */
#define XSPRBAT_MASK (XSPR_MASK &~ SPRBAT_MASK)

/* An XFX form instruction with the SPR field filled in except for the
   SPRG field.  */
#define XSPRG_MASK (XSPR_MASK & ~(0x1f << 16))

/* An X form instruction with everything filled in except the E field.  */
#define XE_MASK (0xffff7fff)

/* An X form user context instruction.  */
#define XUC(op, xop)  (OP (op) | (((unsigned long)(xop)) & 0x1f))
#define XUC_MASK      XUC(0x3f, 0x1f)

/* An XW form instruction.  */
#define XW(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x3f) << 1) | ((rc) & 1))
/* The mask for a G form instruction. rc not supported at present.  */
#define XW_MASK XW (0x3f, 0x3f, 0)

/* An APU form instruction.  */
#define APU(op, xop, rc) (OP (op) | (((unsigned long)(xop)) & 0x3ff) << 1 | ((rc) & 1))

/* The mask for an APU form instruction.  */
#define APU_MASK APU (0x3f, 0x3ff, 1)
#define APU_RT_MASK (APU_MASK | RT_MASK)
#define APU_RA_MASK (APU_MASK | RA_MASK)

/* The BO encodings used in extended conditional branch mnemonics.  */
#define BODNZF	(0x0)
#define BODNZFP	(0x1)
#define BODZF	(0x2)
#define BODZFP	(0x3)
#define BODNZT	(0x8)
#define BODNZTP	(0x9)
#define BODZT	(0xa)
#define BODZTP	(0xb)

#define BOF	(0x4)
#define BOFP	(0x5)
#define BOFM4	(0x6)
#define BOFP4	(0x7)
#define BOT	(0xc)
#define BOTP	(0xd)
#define BOTM4	(0xe)
#define BOTP4	(0xf)

#define BODNZ	(0x10)
#define BODNZP	(0x11)
#define BODZ	(0x12)
#define BODZP	(0x13)
#define BODNZM4 (0x18)
#define BODNZP4 (0x19)
#define BODZM4	(0x1a)
#define BODZP4	(0x1b)

#define BOU	(0x14)

/* The BO16 encodings used in extended VLE conditional branch mnemonics.  */
#define BO16F   (0x0)
#define BO16T   (0x1)

/* The BO32 encodings used in extended VLE conditional branch mnemonics.  */
#define BO32F   (0x0)
#define BO32T   (0x1)
#define BO32DNZ (0x2)
#define BO32DZ  (0x3)

/* The BI condition bit encodings used in extended conditional branch
   mnemonics.  */
#define CBLT	(0)
#define CBGT	(1)
#define CBEQ	(2)
#define CBSO	(3)

/* The TO encodings used in extended trap mnemonics.  */
#define TOLGT	(0x1)
#define TOLLT	(0x2)
#define TOEQ	(0x4)
#define TOLGE	(0x5)
#define TOLNL	(0x5)
#define TOLLE	(0x6)
#define TOLNG	(0x6)
#define TOGT	(0x8)
#define TOGE	(0xc)
#define TONL	(0xc)
#define TOLT	(0x10)
#define TOLE	(0x14)
#define TONG	(0x14)
#define TONE	(0x18)
#define TOU	(0x1f)

/* Smaller names for the flags so each entry in the opcodes table will
   fit on a single line.  */
#define PPCNONE	0
#undef	PPC
#define PPC	PPC_OPCODE_PPC
#define PPCCOM	PPC_OPCODE_PPC | PPC_OPCODE_COMMON
#define POWER4	PPC_OPCODE_POWER4
#define POWER5	PPC_OPCODE_POWER5
#define POWER6	PPC_OPCODE_POWER6
#define POWER7	PPC_OPCODE_POWER7
#define POWER8	PPC_OPCODE_POWER8
#define CELL	PPC_OPCODE_CELL
#define PPC64	PPC_OPCODE_64 | PPC_OPCODE_64_BRIDGE
#define NON32	(PPC_OPCODE_64 | PPC_OPCODE_POWER4	\
		 | PPC_OPCODE_EFS | PPC_OPCODE_E500MC | PPC_OPCODE_TITAN)
#define PPC403	PPC_OPCODE_403
#define PPC405	PPC_OPCODE_405
#define PPC440	PPC_OPCODE_440
#define PPC464	PPC440
#define PPC476	PPC_OPCODE_476
#define PPC750	PPC
#define PPC7450 PPC
#define PPC860	PPC
#define PPCPS	PPC_OPCODE_PPCPS
#define PPCVEC	PPC_OPCODE_ALTIVEC
#define PPCVEC2	PPC_OPCODE_ALTIVEC2
#define PPCVSX	PPC_OPCODE_VSX
#define PPCVSX2	PPC_OPCODE_VSX
#define POWER	PPC_OPCODE_POWER
#define POWER2	PPC_OPCODE_POWER | PPC_OPCODE_POWER2
#define PWR2COM PPC_OPCODE_POWER | PPC_OPCODE_POWER2 | PPC_OPCODE_COMMON
#define PPCPWR2 PPC_OPCODE_PPC | PPC_OPCODE_POWER | PPC_OPCODE_POWER2 | PPC_OPCODE_COMMON
#define COM	PPC_OPCODE_POWER | PPC_OPCODE_PPC | PPC_OPCODE_COMMON
#define M601	PPC_OPCODE_POWER | PPC_OPCODE_601
#define PWRCOM	PPC_OPCODE_POWER | PPC_OPCODE_601 | PPC_OPCODE_COMMON
#define MFDEC1	PPC_OPCODE_POWER
#define MFDEC2	PPC_OPCODE_PPC | PPC_OPCODE_601 | PPC_OPCODE_BOOKE | PPC_OPCODE_TITAN
#define BOOKE	PPC_OPCODE_BOOKE
#define NO371	PPC_OPCODE_BOOKE | PPC_OPCODE_PPCPS | PPC_OPCODE_EFS | PPC_OPCODE_VLE
#define PPCE300 PPC_OPCODE_E300
#define PPCSPE	PPC_OPCODE_SPE | PPC_OPCODE_VLE
#define PPCISEL PPC_OPCODE_ISEL | PPC_OPCODE_VLE
#define PPCEFS	PPC_OPCODE_EFS | PPC_OPCODE_VLE
#define PPCBRLK PPC_OPCODE_BRLOCK
#define PPCPMR	PPC_OPCODE_PMR
#define PPCTMR  PPC_OPCODE_TMR
#define PPCCHLK PPC_OPCODE_CACHELCK
#define PPCRFMCI	PPC_OPCODE_RFMCI
#define E500MC  PPC_OPCODE_E500MC
#define PPCA2	PPC_OPCODE_A2
#define TITAN   PPC_OPCODE_TITAN  
#define MULHW   PPC_OPCODE_405 | PPC_OPCODE_440 | TITAN | PPC_OPCODE_VLE
#define E500	PPC_OPCODE_E500
#define E6500	PPC_OPCODE_E6500
#define PPCVLE  PPC_OPCODE_VLE
#define PPCHTM  PPC_OPCODE_HTM

/* The opcode table.

   The format of the opcode table is:

   NAME		OPCODE		MASK	     FLAGS	ANTI		{OPERANDS}

   NAME is the name of the instruction.
   OPCODE is the instruction opcode.
   MASK is the opcode mask; this is used to tell the disassembler
     which bits in the actual opcode must match OPCODE.
   FLAGS are flags indicating which processors support the instruction.
   ANTI indicates which processors don't support the instruction.
   OPERANDS is the list of operands.

   The disassembler reads the table in order and prints the first
   instruction which matches, so this table is sorted to put more
   specific instructions before more general instructions.

   This table must be sorted by major opcode.  Please try to keep it
   vaguely sorted within major opcode too, except of course where
   constrained otherwise by disassembler operation.  */

const struct powerpc_opcode powerpc_opcodes[] = {
{"attn",	X(0,256),	X_MASK,   POWER4|PPCA2,	PPC476,		{0}},
{"tdlgti",	OPTO(2,TOLGT),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdllti",	OPTO(2,TOLLT),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdeqi",	OPTO(2,TOEQ),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdlgei",	OPTO(2,TOLGE),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdlnli",	OPTO(2,TOLNL),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdllei",	OPTO(2,TOLLE),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdlngi",	OPTO(2,TOLNG),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdgti",	OPTO(2,TOGT),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdgei",	OPTO(2,TOGE),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdnli",	OPTO(2,TONL),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdlti",	OPTO(2,TOLT),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdlei",	OPTO(2,TOLE),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdngi",	OPTO(2,TONG),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdnei",	OPTO(2,TONE),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdui",	OPTO(2,TOU),	OPTO_MASK,   PPC64,	PPCNONE,	{RA, SI}},
{"tdi",		OP(2),		OP_MASK,     PPC64,	PPCNONE,	{TO, RA, SI}},

{"twlgti",	OPTO(3,TOLGT),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tlgti",	OPTO(3,TOLGT),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twllti",	OPTO(3,TOLLT),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tllti",	OPTO(3,TOLLT),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"tweqi",	OPTO(3,TOEQ),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"teqi",	OPTO(3,TOEQ),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twlgei",	OPTO(3,TOLGE),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tlgei",	OPTO(3,TOLGE),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twlnli",	OPTO(3,TOLNL),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tlnli",	OPTO(3,TOLNL),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twllei",	OPTO(3,TOLLE),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tllei",	OPTO(3,TOLLE),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twlngi",	OPTO(3,TOLNG),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tlngi",	OPTO(3,TOLNG),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twgti",	OPTO(3,TOGT),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tgti",	OPTO(3,TOGT),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twgei",	OPTO(3,TOGE),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tgei",	OPTO(3,TOGE),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twnli",	OPTO(3,TONL),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tnli",	OPTO(3,TONL),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twlti",	OPTO(3,TOLT),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tlti",	OPTO(3,TOLT),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twlei",	OPTO(3,TOLE),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tlei",	OPTO(3,TOLE),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twngi",	OPTO(3,TONG),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tngi",	OPTO(3,TONG),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twnei",	OPTO(3,TONE),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tnei",	OPTO(3,TONE),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twui",	OPTO(3,TOU),	OPTO_MASK,   PPCCOM,	PPCNONE,	{RA, SI}},
{"tui",		OPTO(3,TOU),	OPTO_MASK,   PWRCOM,	PPCNONE,	{RA, SI}},
{"twi",		OP(3),		OP_MASK,     PPCCOM,	PPCNONE,	{TO, RA, SI}},
{"ti",		OP(3),		OP_MASK,     PWRCOM,	PPCNONE,	{TO, RA, SI}},

{"ps_cmpu0",	X  (4,	 0), X_MASK|(3<<21), PPCPS,	PPCNONE,	{BF, FRA, FRB}},
{"vaddubm",	VX (4,	 0),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmaxub",	VX (4,	 2),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vrlb",	VX (4,	 4),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpequb",	VXR(4,	 6,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmuloub",	VX (4,	 8),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vaddfp",	VX (4,	10),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"psq_lx",	XW (4,	 6,0),	XW_MASK,     PPCPS,	PPCNONE,	{FRT,RA,RB,PSWM,PSQM}},
{"vmrghb",	VX (4,	12),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"psq_stx",	XW (4,	 7,0),	XW_MASK,     PPCPS,	PPCNONE,	{FRS,RA,RB,PSWM,PSQM}},
{"vpkuhum",	VX (4,	14),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"mulhhwu",	XRC(4,	 8,0),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulhhwu.",	XRC(4,	 8,1),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_sum0",	A  (4,	10,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_sum0.",	A  (4,	10,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_sum1",	A  (4,	11,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_sum1.",	A  (4,	11,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_muls0",	A  (4,	12,0),	AFRB_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRC}},
{"machhwu",	XO (4,	12,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_muls0.",	A  (4,	12,1),	AFRB_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRC}},
{"machhwu.",	XO (4,	12,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_muls1",	A  (4,	13,0),	AFRB_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRC}},
{"ps_muls1.",	A  (4,	13,1),	AFRB_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRC}},
{"ps_madds0",	A  (4,	14,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_madds0.",	A  (4,	14,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_madds1",	A  (4,	15,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_madds1.",	A  (4,	15,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"vmhaddshs",	VXA(4,	32),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"vmhraddshs",	VXA(4,	33),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"vmladduhm",	VXA(4,	34),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"ps_div",	A  (4,	18,0),	AFRC_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"vmsumubm",	VXA(4,	36),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"ps_div.",	A  (4,	18,1),	AFRC_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"vmsummbm",	VXA(4,	37),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"vmsumuhm",	VXA(4,	38),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"vmsumuhs",	VXA(4,	39),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"ps_sub",	A  (4,	20,0),	AFRC_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"vmsumshm",	VXA(4,	40),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"ps_sub.",	A  (4,	20,1),	AFRC_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"vmsumshs",	VXA(4,	41),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"ps_add",	A  (4,	21,0),	AFRC_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"vsel",	VXA(4,	42),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"ps_add.",	A  (4,	21,1),	AFRC_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"vperm",	VXA(4,	43),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, VC}},
{"vsldoi",	VXA(4,	44),	VXASHB_MASK, PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB, SHB}},
{"ps_sel",	A  (4,	23,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"vpermxor",	VXA(4,	45),	VXA_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB, VC}},
{"vmaddfp",	VXA(4,	46),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VC, VB}},
{"ps_sel.",	A  (4,	23,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"vnmsubfp",	VXA(4,	47),	VXA_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VC, VB}},
{"ps_res",	A  (4,	24,0), AFRAFRC_MASK, PPCPS,	PPCNONE,	{FRT, FRB}},
{"ps_res.",	A  (4,	24,1), AFRAFRC_MASK, PPCPS,	PPCNONE,	{FRT, FRB}},
{"ps_mul",	A  (4,	25,0), AFRB_MASK,    PPCPS,	PPCNONE,	{FRT, FRA, FRC}},
{"ps_mul.",	A  (4,	25,1),	AFRB_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRC}},
{"ps_rsqrte",	A  (4,	26,0), AFRAFRC_MASK, PPCPS,	PPCNONE,	{FRT, FRB}},
{"ps_rsqrte.",	A  (4,	26,1), AFRAFRC_MASK, PPCPS,	PPCNONE,	{FRT, FRB}},
{"ps_msub",	A  (4,	28,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_msub.",	A  (4,	28,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_madd",	A  (4,	29,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_madd.",	A  (4,	29,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_nmsub",	A  (4,	30,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_nmsub.",	A  (4,	30,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_nmadd",	A  (4,	31,0),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_nmadd.",	A  (4,	31,1),	A_MASK,      PPCPS,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"ps_cmpo0",	X  (4,	32), X_MASK|(3<<21), PPCPS,	PPCNONE,	{BF, FRA, FRB}},
{"vaddeuqm",	VXA(4,	60),	VXA_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB, VC}},
{"vaddecuq",	VXA(4,	61),	VXA_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB, VC}},
{"vsubeuqm",	VXA(4,	62),	VXA_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB, VC}},
{"vsubecuq",	VXA(4,	63),	VXA_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB, VC}},
{"vadduhm",	VX (4,	64),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmaxuh",	VX (4,	66),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vrlh",	VX (4,	68),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpequh",	VXR(4,	70,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmulouh",	VX (4,	72),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vsubfp",	VX (4,	74),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"psq_lux",	XW (4,	38,0),	XW_MASK,     PPCPS,	PPCNONE,	{FRT,RA,RB,PSWM,PSQM}},
{"vmrghh",	VX (4,	76),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"psq_stux",	XW (4,	39,0),	XW_MASK,     PPCPS,	PPCNONE,	{FRS,RA,RB,PSWM,PSQM}},
{"vpkuwum",	VX (4,	78),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"ps_neg",	XRC(4,	40,0),	XRA_MASK,    PPCPS,	PPCNONE,	{FRT, FRB}},
{"mulhhw",	XRC(4,	40,0),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_neg.",	XRC(4,	40,1),	XRA_MASK,    PPCPS,	PPCNONE,	{FRT, FRB}},
{"mulhhw.",	XRC(4,	40,1),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"machhw",	XO (4,	44,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"machhw.",	XO (4,	44,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmachhw",	XO (4,	46,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmachhw.",	XO (4,	46,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_cmpu1",	X  (4,	64), X_MASK|(3<<21), PPCPS,	PPCNONE,	{BF, FRA, FRB}},
{"vadduwm",	VX (4,	128),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmaxuw",	VX (4,	130),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vrlw",	VX (4,	132),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpequw",	VXR(4,	134,0), VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmulouw",	VX (4,  136),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vmuluwm",	VX (4,  137),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vmrghw",	VX (4,	140),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vpkuhus",	VX (4,	142),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"ps_mr",	XRC(4,	72,0),	XRA_MASK,    PPCPS,	PPCNONE,	{FRT, FRB}},
{"ps_mr.",	XRC(4,	72,1),	XRA_MASK,    PPCPS,	PPCNONE,	{FRT, FRB}},
{"machhwsu",	XO (4,	76,0,0),XO_MASK,     MULHW|PPCVLE,  PPCNONE,	{RT, RA, RB}},
{"machhwsu.",	XO (4,	76,0,1),XO_MASK,     MULHW|PPCVLE,  PPCNONE,	{RT, RA, RB}},
{"ps_cmpo1",	X  (4,	96), X_MASK|(3<<21), PPCPS,	PPCNONE,	{BF, FRA, FRB}},
{"vaddudm",	VX (4, 192),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vmaxud",	VX (4, 194),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vrld",	VX (4, 196),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vcmpeqfp",	VXR(4, 198,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpequd",	VXR(4, 199,0),	VXR_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vpkuwus",	VX (4, 206),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"machhws",	XO (4, 108,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"machhws.",	XO (4, 108,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmachhws",	XO (4, 110,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmachhws.",	XO (4, 110,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vadduqm",	VX (4, 256),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vmaxsb",	VX (4, 258),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vslb",	VX (4, 260),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmulosb",	VX (4, 264),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vrefp",	VX (4, 266),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"vmrglb",	VX (4, 268),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vpkshus",	VX (4, 270),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"ps_nabs",	XRC(4, 136,0),	XRA_MASK,    PPCPS,	PPCNONE,	{FRT, FRB}},
{"mulchwu",	XRC(4, 136,0),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_nabs.",	XRC(4, 136,1),	XRA_MASK,    PPCPS,	PPCNONE,	{FRT, FRB}},
{"mulchwu.",	XRC(4, 136,1),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"macchwu",	XO (4, 140,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"macchwu.",	XO (4, 140,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vaddcuq",	VX (4, 320),	VX_MASK,     PPCVEC2,      PPCNONE,	{VD, VA, VB}},
{"vmaxsh",	VX (4, 322),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vslh",	VX (4, 324),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmulosh",	VX (4, 328),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vrsqrtefp",	VX (4, 330),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"vmrglh",	VX (4, 332),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vpkswus",	VX (4, 334),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"mulchw",	XRC(4, 168,0),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulchw.",	XRC(4, 168,1),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"macchw",	XO (4, 172,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"macchw.",	XO (4, 172,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmacchw",	XO (4, 174,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmacchw.",	XO (4, 174,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vaddcuw",	VX (4, 384),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmaxsw",	VX (4, 386),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vslw",	VX (4, 388),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmulosw",	VX (4, 392),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vexptefp",	VX (4, 394),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"vmrglw",	VX (4, 396),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vpkshss",	VX (4, 398),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"macchwsu",	XO (4, 204,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"macchwsu.",	XO (4, 204,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vmaxsd",	VX (4, 450),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vsl",		VX (4, 452),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpgefp",	VXR(4, 454,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vlogefp",	VX (4, 458),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"vpkswss",	VX (4, 462),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"macchws",	XO (4, 236,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"macchws.",	XO (4, 236,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmacchws",	XO (4, 238,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmacchws.",	XO (4, 238,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evaddw",	VX (4, 512),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vaddubs",	VX (4, 512),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evaddiw",	VX (4, 514),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB, UIMM}},
{"vminub",	VX (4, 514),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evsubfw",	VX (4, 516),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evsubw",	VX (4, 516),	VX_MASK,     PPCSPE,	PPCNONE,	{RS, RB, RA}},
{"vsrb",	VX (4, 516),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evsubifw",	VX (4, 518),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, UIMM, RB}},
{"evsubiw",	VX (4, 518),	VX_MASK,     PPCSPE,	PPCNONE,	{RS, RB, UIMM}},
{"vcmpgtub",	VXR(4, 518,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evabs",	VX (4, 520),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"vmuleub",	VX (4, 520),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evneg",	VX (4, 521),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evextsb",	VX (4, 522),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"vrfin",	VX (4, 522),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"evextsh",	VX (4, 523),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evrndw",	VX (4, 524),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"vspltb",	VX (4, 524),	VXUIMM4_MASK,PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM4}},
{"evcntlzw",	VX (4, 525),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evcntlsw",	VX (4, 526),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"vupkhsb",	VX (4, 526),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"brinc",	VX (4, 527),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"ps_abs",	XRC(4, 264,0),	XRA_MASK,    PPCPS,	PPCNONE,	{FRT, FRB}},
{"ps_abs.",	XRC(4, 264,1),	XRA_MASK,    PPCPS,	PPCNONE,	{FRT, FRB}},
{"evand",	VX (4, 529),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evandc",	VX (4, 530),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evxor",	VX (4, 534),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmr",	VX (4, 535),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, BBA}},
{"evor",	VX (4, 535),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evnor",	VX (4, 536),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evnot",	VX (4, 536),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, BBA}},
{"get",		APU(4, 268,0),	APU_RA_MASK, PPC405,	PPCNONE,	{RT, FSL}},
{"eveqv",	VX (4, 537),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evorc",	VX (4, 539),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evnand",	VX (4, 542),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evsrwu",	VX (4, 544),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evsrws",	VX (4, 545),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evsrwiu",	VX (4, 546),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, EVUIMM}},
{"evsrwis",	VX (4, 547),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, EVUIMM}},
{"evslw",	VX (4, 548),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evslwi",	VX (4, 550),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, EVUIMM}},
{"evrlw",	VX (4, 552),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evsplati",	VX (4, 553),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, SIMM}},
{"evrlwi",	VX (4, 554),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, EVUIMM}},
{"evsplatfi",	VX (4, 555),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, SIMM}},
{"evmergehi",	VX (4, 556),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmergelo",	VX (4, 557),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmergehilo",	VX (4, 558),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmergelohi",	VX (4, 559),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evcmpgtu",	VX (4, 560),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"evcmpgts",	VX (4, 561),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"evcmpltu",	VX (4, 562),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"evcmplts",	VX (4, 563),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"evcmpeq",	VX (4, 564),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"cget",	APU(4, 284,0),	APU_RA_MASK, PPC405,	PPCNONE,	{RT, FSL}},
{"vadduhs",	VX (4, 576),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vminuh",	VX (4, 578),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vsrh",	VX (4, 580),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpgtuh",	VXR(4, 582,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmuleuh",	VX (4, 584),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vrfiz",	VX (4, 586),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"vsplth",	VX (4, 588),	VXUIMM3_MASK,PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM3}},
{"vupkhsh",	VX (4, 590),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"nget",	APU(4, 300,0),	APU_RA_MASK, PPC405,	PPCNONE,	{RT, FSL}},
{"evsel",	EVSEL(4,79),	EVSEL_MASK,  PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB, CRFS}},
{"ncget",	APU(4, 316,0),	APU_RA_MASK, PPC405,	PPCNONE,	{RT, FSL}},
{"evfsadd",	VX (4, 640),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vadduws",	VX (4, 640),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evfssub",	VX (4, 641),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vminuw",	VX (4, 642),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evfsabs",	VX (4, 644),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"vsrw",	VX (4, 644),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evfsnabs",	VX (4, 645),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evfsneg",	VX (4, 646),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"vcmpgtuw",	VXR(4, 646,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmuleuw",	VX (4, 648),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evfsmul",	VX (4, 648),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evfsdiv",	VX (4, 649),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vrfip",	VX (4, 650),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"evfscmpgt",	VX (4, 652),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"vspltw",	VX (4, 652),	VXUIMM2_MASK,PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM2}},
{"evfscmplt",	VX (4, 653),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"evfscmpeq",	VX (4, 654),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"vupklsb",	VX (4, 654),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"evfscfui",	VX (4, 656),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfscfsi",	VX (4, 657),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfscfuf",	VX (4, 658),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfscfsf",	VX (4, 659),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfsctui",	VX (4, 660),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfsctsi",	VX (4, 661),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfsctuf",	VX (4, 662),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfsctsf",	VX (4, 663),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfsctuiz",	VX (4, 664),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"put",		APU(4, 332,0),	APU_RT_MASK, PPC405,	PPCNONE,	{RA, FSL}},
{"evfsctsiz",	VX (4, 666),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RB}},
{"evfststgt",	VX (4, 668),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"evfststlt",	VX (4, 669),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"evfststeq",	VX (4, 670),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"cput",	APU(4, 348,0),	APU_RT_MASK, PPC405,	PPCNONE,	{RA, FSL}},
{"efsadd",	VX (4, 704),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"efssub",	VX (4, 705),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vminud",	VX (4, 706),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"efsabs",	VX (4, 708),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA}},
{"vsr",		VX (4, 708),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"efsnabs",	VX (4, 709),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA}},
{"efsneg",	VX (4, 710),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA}},
{"vcmpgtfp",	VXR(4, 710,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpgtud",	VXR(4, 711,0),	VXR_MASK,    PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"efsmul",	VX (4, 712),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"efsdiv",	VX (4, 713),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vrfim",	VX (4, 714),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"efscmpgt",	VX (4, 716),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efscmplt",	VX (4, 717),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efscmpeq",	VX (4, 718),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"vupklsh",	VX (4, 718),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"efscfd",	VX (4, 719),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efscfui",	VX (4, 720),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efscfsi",	VX (4, 721),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efscfuf",	VX (4, 722),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efscfsf",	VX (4, 723),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efsctui",	VX (4, 724),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efsctsi",	VX (4, 725),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efsctuf",	VX (4, 726),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efsctsf",	VX (4, 727),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efsctuiz",	VX (4, 728),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"nput",	APU(4, 364,0),	APU_RT_MASK, PPC405,	PPCNONE,	{RA, FSL}},
{"efsctsiz",	VX (4, 730),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efststgt",	VX (4, 732),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efststlt",	VX (4, 733),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efststeq",	VX (4, 734),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efdadd",	VX (4, 736),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"efdsub",	VX (4, 737),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"efdcfuid",	VX (4, 738),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdcfsid",	VX (4, 739),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdabs",	VX (4, 740),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA}},
{"efdnabs",	VX (4, 741),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA}},
{"efdneg",	VX (4, 742),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA}},
{"efdmul",	VX (4, 744),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"efddiv",	VX (4, 745),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"efdctuidz",	VX (4, 746),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdctsidz",	VX (4, 747),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdcmpgt",	VX (4, 748),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efdcmplt",	VX (4, 749),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efdcmpeq",	VX (4, 750),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efdcfs",	VX (4, 751),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdcfui",	VX (4, 752),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdcfsi",	VX (4, 753),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdcfuf",	VX (4, 754),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdcfsf",	VX (4, 755),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdctui",	VX (4, 756),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdctsi",	VX (4, 757),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdctuf",	VX (4, 758),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdctsf",	VX (4, 759),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdctuiz",	VX (4, 760),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"ncput",	APU(4, 380,0),	APU_RT_MASK, PPC405,	PPCNONE,	{RA, FSL}},
{"efdctsiz",	VX (4, 762),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{RS, RB}},
{"efdtstgt",	VX (4, 764),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efdtstlt",	VX (4, 765),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"efdtsteq",	VX (4, 766),	VX_MASK,     PPCEFS|PPCVLE, PPCNONE,	{CRFD, RA, RB}},
{"evlddx",	VX (4, 768),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vaddsbs",	VX (4, 768),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evldd",	VX (4, 769),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_8, RA}},
{"evldwx",	VX (4, 770),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vminsb",	VX (4, 770),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evldw",	VX (4, 771),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_8, RA}},
{"evldhx",	VX (4, 772),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vsrab",	VX (4, 772),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evldh",	VX (4, 773),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_8, RA}},
{"vcmpgtsb",	VXR(4, 774,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evlhhesplatx",VX (4, 776),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vmulesb",	VX (4, 776),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evlhhesplat",	VX (4, 777),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_2, RA}},
{"vcfux",	VX (4, 778),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM}},
{"vcuxwfp",	VX (4, 778),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM}},
{"evlhhousplatx",VX(4, 780),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vspltisb",	VX (4, 780),	VXVB_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, SIMM}},
{"evlhhousplat",VX (4, 781),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_2, RA}},
{"evlhhossplatx",VX(4, 782),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vpkpx",	VX (4, 782),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evlhhossplat",VX (4, 783),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_2, RA}},
{"mullhwu",	XRC(4, 392,0),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evlwhex",	VX (4, 784),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"mullhwu.",	XRC(4, 392,1),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evlwhe",	VX (4, 785),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"evlwhoux",	VX (4, 788),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evlwhou",	VX (4, 789),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"evlwhosx",	VX (4, 790),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evlwhos",	VX (4, 791),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"maclhwu",	XO (4, 396,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evlwwsplatx",	VX (4, 792),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"maclhwu.",	XO (4, 396,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evlwwsplat",	VX (4, 793),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"evlwhsplatx",	VX (4, 796),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evlwhsplat",	VX (4, 797),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"evstddx",	VX (4, 800),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evstdd",	VX (4, 801),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_8, RA}},
{"evstdwx",	VX (4, 802),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evstdw",	VX (4, 803),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_8, RA}},
{"evstdhx",	VX (4, 804),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evstdh",	VX (4, 805),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_8, RA}},
{"evstwhex",	VX (4, 816),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evstwhe",	VX (4, 817),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"evstwhox",	VX (4, 820),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evstwho",	VX (4, 821),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"evstwwex",	VX (4, 824),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evstwwe",	VX (4, 825),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"evstwwox",	VX (4, 828),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evstwwo",	VX (4, 829),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, EVUIMM_4, RA}},
{"vaddshs",	VX (4, 832),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vminsh",	VX (4, 834),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vsrah",	VX (4, 836),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpgtsh",	VXR(4, 838,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmulesh",	VX (4, 840),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcfsx",	VX (4, 842),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM}},
{"vcsxwfp",	VX (4, 842),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM}},
{"vspltish",	VX (4, 844),	VXVB_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, SIMM}},
{"vupkhpx",	VX (4, 846),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"mullhw",	XRC(4, 424,0),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mullhw.",	XRC(4, 424,1),	X_MASK,      MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"maclhw",	XO (4, 428,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"maclhw.",	XO (4, 428,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmaclhw",	XO (4, 430,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmaclhw.",	XO (4, 430,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vaddsws",	VX (4, 896),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vminsw",	VX (4, 898),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vsraw",	VX (4, 900),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpgtsw",	VXR(4, 902,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmulesw",	VX (4, 904),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vctuxs",	VX (4, 906),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM}},
{"vcfpuxws",	VX (4, 906),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM}},
{"vspltisw",	VX (4, 908),	VXVB_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, SIMM}},
{"maclhwsu",	XO (4, 460,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"maclhwsu.",	XO (4, 460,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vminsd",	VX (4, 962),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vsrad",	VX (4, 964),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vcmpbfp",	VXR(4, 966,0),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpgtsd",	VXR(4, 967,0),	VXR_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vctsxs",	VX (4, 970),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM}},
{"vcfpsxws",	VX (4, 970),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VB, UIMM}},
{"vupklpx",	VX (4, 974),	VXVA_MASK,   PPCVEC|PPCVLE, PPCNONE,	{VD, VB}},
{"maclhws",	XO (4, 492,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"maclhws.",	XO (4, 492,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmaclhws",	XO (4, 494,0,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmaclhws.",	XO (4, 494,0,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vsububm",	VX (4,1024),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"bcdadd.",	VX (4,1025),	VXPS_MASK,   PPCVEC2,	    PPCNONE,	{VD, VA, VB, PS}},
{"vavgub",	VX (4,1026),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vabsdub",	VX (4,1027),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"evmhessf",	VX (4,1027),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vand",	VX (4,1028),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpequb.",	VXR(4,	 6,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi0fcm.",	APU(4, 515,0), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"udi0fcm",	APU(4, 515,1), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"evmhossf",	VX (4,1031),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vpmsumb",	VX (4,1032),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmheumi",	VX (4,1032),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhesmi",	VX (4,1033),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vmaxfp",	VX (4,1034),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evmhesmf",	VX (4,1035),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhoumi",	VX (4,1036),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vslo",	VX (4,1036),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evmhosmi",	VX (4,1037),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhosmf",	VX (4,1039),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"machhwuo",	XO (4,	12,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"machhwuo.",	XO (4,	12,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_merge00",	XOPS(4,528,0),	XOPS_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"ps_merge00.",	XOPS(4,528,1),	XOPS_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"evmhessfa",	VX (4,1059),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhossfa",	VX (4,1063),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmheumia",	VX (4,1064),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhesmia",	VX (4,1065),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhesmfa",	VX (4,1067),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhoumia",	VX (4,1068),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhosmia",	VX (4,1069),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhosmfa",	VX (4,1071),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vsubuhm",	VX (4,1088),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"bcdsub.",	VX (4,1089),	VXPS_MASK,   PPCVEC2,	    PPCNONE,	{VD, VA, VB, PS}},
{"vavguh",	VX (4,1090),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vabsduh",	VX (4,1091),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vandc",	VX (4,1092),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vcmpequh.",	VXR(4,	70,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi1fcm.",	APU(4, 547,0), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"udi1fcm",	APU(4, 547,1), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},   
{"evmwhssf",	VX (4,1095),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vpmsumh",	VX (4,1096),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmwlumi",	VX (4,1096),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vminfp",	VX (4,1098),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evmwhumi",	VX (4,1100),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vsro",	VX (4,1100),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evmwhsmi",	VX (4,1101),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vpkudum",	VX (4,1102),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmwhsmf",	VX (4,1103),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwssf",	VX (4,1107),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"machhwo",	XO (4,	44,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmwumi",	VX (4,1112),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"machhwo.",	XO (4,	44,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmwsmi",	VX (4,1113),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwsmf",	VX (4,1115),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"nmachhwo",	XO (4,	46,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmachhwo.",	XO (4,	46,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_merge01",	XOPS(4,560,0),	XOPS_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"ps_merge01.",	XOPS(4,560,1),	XOPS_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"evmwhssfa",	VX (4,1127),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwlumia",	VX (4,1128),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwhumia",	VX (4,1132),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwhsmia",	VX (4,1133),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwhsmfa",	VX (4,1135),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwssfa",	VX (4,1139),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwumia",	VX (4,1144),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwsmia",	VX (4,1145),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwsmfa",	VX (4,1147),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vsubuwm",	VX (4,1152),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vavguw",	VX (4,1154),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vabsduw",	VX (4,1155),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vmr",		VX (4,1156),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VBA}},
{"vor",		VX (4,1156),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vpmsumw",	VX (4,1160),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vcmpequw.",	VXR(4, 134,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi2fcm.",	APU(4, 579,0), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"udi2fcm",	APU(4, 579,1), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},   
{"machhwsuo",	XO (4,	76,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"machhwsuo.",	XO (4,	76,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_merge10",	XOPS(4,592,0),	XOPS_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"ps_merge10.",	XOPS(4,592,1),	XOPS_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"vsubudm",	VX (4,1216),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evaddusiaaw",	VX (4,1216),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evaddssiaaw",	VX (4,1217),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evsubfusiaaw",VX (4,1218),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evsubfssiaaw",VX (4,1219),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evmra",	VX (4,1220),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"vxor",	VX (4,1220),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evdivws",	VX (4,1222),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vcmpeqfp.",	VXR(4, 198,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi3fcm.",	APU(4, 611,0), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"vcmpequd.",	VXR(4, 199,1),	VXR_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"udi3fcm",	APU(4, 611,1), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},   
{"evdivwu",	VX (4,1223),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vpmsumd",	VX (4,1224),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evaddumiaaw",	VX (4,1224),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evaddsmiaaw",	VX (4,1225),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evsubfumiaaw",VX (4,1226),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"evsubfsmiaaw",VX (4,1227),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA}},
{"vpkudus",	VX (4,1230),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"machhwso",	XO (4, 108,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"machhwso.",	XO (4, 108,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmachhwso",	XO (4, 110,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmachhwso.",	XO (4, 110,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ps_merge11",	XOPS(4,624,0),	XOPS_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"ps_merge11.",	XOPS(4,624,1),	XOPS_MASK,   PPCPS,	PPCNONE,	{FRT, FRA, FRB}},
{"vsubuqm",	VX (4,1280),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"evmheusiaaw",	VX (4,1280),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhessiaaw",	VX (4,1281),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vavgsb",	VX (4,1282),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evmhessfaaw",	VX (4,1283),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhousiaaw",	VX (4,1284),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vnot",	VX (4,1284),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VBA}},
{"vnor",	VX (4,1284),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evmhossiaaw",	VX (4,1285),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"udi4fcm.",	APU(4, 643,0), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"udi4fcm",	APU(4, 643,1), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"evmhossfaaw",	VX (4,1287),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmheumiaaw",	VX (4,1288),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vcipher",	VX (4,1288),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vcipherlast",	VX (4,1289),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmhesmiaaw",	VX (4,1289),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhesmfaaw",	VX (4,1291),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vgbbd",	VX (4,1292),	VXVA_MASK,   PPCVEC2,	    PPCNONE,	{VD, VB}},
{"evmhoumiaaw",	VX (4,1292),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhosmiaaw",	VX (4,1293),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhosmfaaw",	VX (4,1295),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"macchwuo",	XO (4, 140,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"macchwuo.",	XO (4, 140,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmhegumiaa",	VX (4,1320),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhegsmiaa",	VX (4,1321),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhegsmfaa",	VX (4,1323),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhogumiaa",	VX (4,1324),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhogsmiaa",	VX (4,1325),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhogsmfaa",	VX (4,1327),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vsubcuq",	VX (4,1344),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmwlusiaaw",	VX (4,1344),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwlssiaaw",	VX (4,1345),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vavgsh",	VX (4,1346),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vorc",	VX (4,1348),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"udi5fcm.",	APU(4, 675,0), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"udi5fcm",	APU(4, 675,1), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"vncipher",	VX (4,1352),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmwlumiaaw",	VX (4,1352),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vncipherlast",VX (4,1353),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmwlsmiaaw",	VX (4,1353),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vbpermq",	VX (4,1356),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vpksdus",	VX (4,1358),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmwssfaa",	VX (4,1363),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"macchwo",	XO (4, 172,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmwumiaa",	VX (4,1368),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"macchwo.",	XO (4, 172,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmwsmiaa",	VX (4,1369),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwsmfaa",	VX (4,1371),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"nmacchwo",	XO (4, 174,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmacchwo.",	XO (4, 174,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmheusianw",	VX (4,1408),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vsubcuw",	VX (4,1408),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evmhessianw",	VX (4,1409),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vavgsw",	VX (4,1410),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"evmhessfanw",	VX (4,1411),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vnand",	VX (4,1412),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmhousianw",	VX (4,1412),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhossianw",	VX (4,1413),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"udi6fcm.",	APU(4, 707,0), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"udi6fcm",	APU(4, 707,1), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"evmhossfanw",	VX (4,1415),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmheumianw",	VX (4,1416),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhesmianw",	VX (4,1417),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhesmfanw",	VX (4,1419),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhoumianw",	VX (4,1420),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhosmianw",	VX (4,1421),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhosmfanw",	VX (4,1423),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"macchwsuo",	XO (4, 204,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"macchwsuo.",	XO (4, 204,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmhegumian",	VX (4,1448),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhegsmian",	VX (4,1449),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhegsmfan",	VX (4,1451),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhogumian",	VX (4,1452),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhogsmian",	VX (4,1453),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmhogsmfan",	VX (4,1455),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwlusianw",	VX (4,1472),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwlssianw",	VX (4,1473),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vsld",	VX (4,1476),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"vcmpgefp.",	VXR(4, 454,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi7fcm.",	APU(4, 739,0), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"udi7fcm",	APU(4, 739,1), APU_MASK, PPC405|PPC440, PPC476,		{URT, URA, URB}},
{"vsbox",	VX (4,1480),	VXVB_MASK,   PPCVEC2,	    PPCNONE,	{VD, VA}},
{"evmwlumianw",	VX (4,1480),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwlsmianw",	VX (4,1481),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"vpksdss",	VX (4,1486),	VX_MASK,     PPCVEC2,	    PPCNONE,	{VD, VA, VB}},
{"evmwssfan",	VX (4,1491),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"macchwso",	XO (4, 236,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmwumian",	VX (4,1496),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"macchwso.",	XO (4, 236,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"evmwsmian",	VX (4,1497),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"evmwsmfan",	VX (4,1499),	VX_MASK,     PPCSPE|PPCVLE, PPCNONE,	{RS, RA, RB}},
{"nmacchwso",	XO (4, 238,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmacchwso.",	XO (4, 238,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vsububs",	VX (4,1536),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"mfvscr",	VX (4,1540),	VXVAVB_MASK, PPCVEC|PPCVLE, PPCNONE,	{VD}},
{"vcmpgtub.",	VXR(4, 518,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi8fcm.",	APU(4, 771,0),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"udi8fcm",	APU(4, 771,1),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"vsum4ubs",	VX (4,1544),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vsubuhs",	VX (4,1600),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"mtvscr",	VX (4,1604),	VXVDVA_MASK, PPCVEC|PPCVLE, PPCNONE,	{VB}},
{"vcmpgtuh.",	VXR(4, 582,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vsum4shs",	VX (4,1608),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi9fcm.",	APU(4, 804,0),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"udi9fcm",	APU(4, 804,1),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"vupkhsw",	VX (4,1614),	VXVA_MASK,   PPCVEC2,	    PPCNONE,	{VD, VB}},
{"vsubuws",	VX (4,1664),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vshasigmaw",	VX (4,1666),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, ST, SIX}},
{"veqv",	VX (4,1668),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vcmpgtuw.",	VXR(4, 646,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi10fcm.",	APU(4, 835,0),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"udi10fcm",	APU(4, 835,1),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"vsum2sws",	VX (4,1672),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmrgow",	VX (4,1676),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vshasigmad",	VX (4,1730),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, ST, SIX}},
{"vsrd",	VX (4,1732),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"vcmpgtfp.",	VXR(4, 710,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi11fcm.",	APU(4, 867,0),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"vcmpgtud.",	VXR(4, 711,1),	VXR_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"udi11fcm",	APU(4, 867,1),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"vupklsw",	VX (4,1742),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vsubsbs",	VX (4,1792),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vclzb",	VX (4,1794),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vpopcntb",	VX (4,1795),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vcmpgtsb.",	VXR(4, 774,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi12fcm.",	APU(4, 899,0),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"udi12fcm",	APU(4, 899,1),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"vsum4sbs",	VX (4,1800),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"maclhwuo",	XO (4, 396,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"maclhwuo.",	XO (4, 396,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vsubshs",	VX (4,1856),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vclzh",	VX (4,1858),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vpopcnth",	VX (4,1859),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vcmpgtsh.",	VXR(4, 838,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi13fcm.",	APU(4, 931,0),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"udi13fcm",	APU(4, 931,1),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"maclhwo",	XO (4, 428,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"maclhwo.",	XO (4, 428,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmaclhwo",	XO (4, 430,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmaclhwo.",	XO (4, 430,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vsubsws",	VX (4,1920),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vclzw",	VX (4,1922),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vpopcntw",	VX (4,1923),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vcmpgtsw.",	VXR(4, 902,1),	VXR_MASK,    PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"udi14fcm.",	APU(4, 963,0),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"udi14fcm",	APU(4, 963,1),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"vsumsws",	VX (4,1928),	VX_MASK,     PPCVEC|PPCVLE, PPCNONE,	{VD, VA, VB}},
{"vmrgew",	VX (4,1932),	VX_MASK,     PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"maclhwsuo",	XO (4, 460,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"maclhwsuo.",	XO (4, 460,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"vclzd",	VX (4,1986),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vpopcntd",	VX (4,1987),	VXVA_MASK,   PPCVEC2,	PPCNONE,	{VD, VB}},
{"vcmpbfp.",	VXR(4, 966,1),	VXR_MASK,    PPCVEC,	PPCNONE,	{VD, VA, VB}},
{"udi15fcm.",	APU(4, 995,0),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"vcmpgtsd.",	VXR(4, 967,1),	VXR_MASK,    PPCVEC2,	PPCNONE,	{VD, VA, VB}},
{"udi15fcm",	APU(4, 995,1),	APU_MASK,    PPC440,	PPC476,		{URT, URA, URB}},
{"maclhwso",	XO (4, 492,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"maclhwso.",	XO (4, 492,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmaclhwso",	XO (4, 494,1,0),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"nmaclhwso.",	XO (4, 494,1,1),XO_MASK,     MULHW|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"dcbz_l",	X  (4,1014),	XRT_MASK,    PPCPS,	PPCNONE,	{RA, RB}},

{"mulli",	OP(7),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA, SI}},
{"muli",	OP(7),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, RA, SI}},

{"subfic",	OP(8),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA, SI}},
{"sfi",		OP(8),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, RA, SI}},

{"dozi",	OP(9),		OP_MASK,     M601,	PPCNONE,	{RT, RA, SI}},

{"cmplwi",	OPL(10,0),	OPL_MASK,    PPCCOM,	PPCNONE,	{OBF, RA, UI}},
{"cmpldi",	OPL(10,1),	OPL_MASK,    PPC64,	PPCNONE,	{OBF, RA, UI}},
{"cmpli",	OP(10),		OP_MASK,     PPC,	PPCNONE,	{BF, L, RA, UI}},
{"cmpli",	OP(10),		OP_MASK,     PWRCOM,	PPC,		{BF, RA, UI}},

{"cmpwi",	OPL(11,0),	OPL_MASK,    PPCCOM,	PPCNONE,	{OBF, RA, SI}},
{"cmpdi",	OPL(11,1),	OPL_MASK,    PPC64,	PPCNONE,	{OBF, RA, SI}},
{"cmpi",	OP(11),		OP_MASK,     PPC,	PPCNONE,	{BF, L, RA, SI}},
{"cmpi",	OP(11),		OP_MASK,     PWRCOM,	PPC,		{BF, RA, SI}},

{"addic",	OP(12),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA, SI}},
{"ai",		OP(12),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, RA, SI}},
{"subic",	OP(12),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA, NSI}},

{"addic.",	OP(13),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA, SI}},
{"ai.",		OP(13),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, RA, SI}},
{"subic.",	OP(13),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA, NSI}},

{"li",		OP(14),		DRA_MASK,    PPCCOM,	PPCNONE,	{RT, SI}},
{"lil",		OP(14),		DRA_MASK,    PWRCOM,	PPCNONE,	{RT, SI}},
{"addi",	OP(14),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA0, SI}},
{"cal",		OP(14),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, D, RA0}},
{"subi",	OP(14),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA0, NSI}},
{"la",		OP(14),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, D, RA0}},

{"lis",		OP(15),		DRA_MASK,    PPCCOM,	PPCNONE,	{RT, SISIGNOPT}},
{"liu",		OP(15),		DRA_MASK,    PWRCOM,	PPCNONE,	{RT, SISIGNOPT}},
{"addis",	OP(15),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA0, SISIGNOPT}},
{"cau",		OP(15),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, RA0, SISIGNOPT}},
{"subis",	OP(15),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, RA0, NSI}},

{"bdnz-",    BBO(16,BODNZ,0,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDM}},
{"bdnz+",    BBO(16,BODNZ,0,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDP}},
{"bdnz",     BBO(16,BODNZ,0,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BD}},
{"bdn",      BBO(16,BODNZ,0,0),		BBOATBI_MASK,  PWRCOM,	 PPCNONE,	{BD}},
{"bdnzl-",   BBO(16,BODNZ,0,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDM}},
{"bdnzl+",   BBO(16,BODNZ,0,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDP}},
{"bdnzl",    BBO(16,BODNZ,0,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BD}},
{"bdnl",     BBO(16,BODNZ,0,1),		BBOATBI_MASK,  PWRCOM,	 PPCNONE,	{BD}},
{"bdnza-",   BBO(16,BODNZ,1,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDMA}},
{"bdnza+",   BBO(16,BODNZ,1,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDPA}},
{"bdnza",    BBO(16,BODNZ,1,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDA}},
{"bdna",     BBO(16,BODNZ,1,0),		BBOATBI_MASK,  PWRCOM,	 PPCNONE,	{BDA}},
{"bdnzla-",  BBO(16,BODNZ,1,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDMA}},
{"bdnzla+",  BBO(16,BODNZ,1,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDPA}},
{"bdnzla",   BBO(16,BODNZ,1,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDA}},
{"bdnla",    BBO(16,BODNZ,1,1),		BBOATBI_MASK,  PWRCOM,	 PPCNONE,	{BDA}},
{"bdz-",     BBO(16,BODZ,0,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDM}},
{"bdz+",     BBO(16,BODZ,0,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDP}},
{"bdz",      BBO(16,BODZ,0,0),		BBOATBI_MASK,  COM,	 PPCNONE,	{BD}},
{"bdzl-",    BBO(16,BODZ,0,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDM}},
{"bdzl+",    BBO(16,BODZ,0,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDP}},
{"bdzl",     BBO(16,BODZ,0,1),		BBOATBI_MASK,  COM,	 PPCNONE,	{BD}},
{"bdza-",    BBO(16,BODZ,1,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDMA}},
{"bdza+",    BBO(16,BODZ,1,0),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDPA}},
{"bdza",     BBO(16,BODZ,1,0),		BBOATBI_MASK,  COM,	 PPCNONE,	{BDA}},
{"bdzla-",   BBO(16,BODZ,1,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDMA}},
{"bdzla+",   BBO(16,BODZ,1,1),		BBOATBI_MASK,  PPCCOM,	 PPCNONE,	{BDPA}},
{"bdzla",    BBO(16,BODZ,1,1),		BBOATBI_MASK,  COM,	 PPCNONE,	{BDA}},

{"bge-",     BBOCB(16,BOF,CBLT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bge+",     BBOCB(16,BOF,CBLT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bge",      BBOCB(16,BOF,CBLT,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bnl-",     BBOCB(16,BOF,CBLT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bnl+",     BBOCB(16,BOF,CBLT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bnl",      BBOCB(16,BOF,CBLT,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bgel-",    BBOCB(16,BOF,CBLT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bgel+",    BBOCB(16,BOF,CBLT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bgel",     BBOCB(16,BOF,CBLT,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bnll-",    BBOCB(16,BOF,CBLT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bnll+",    BBOCB(16,BOF,CBLT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bnll",     BBOCB(16,BOF,CBLT,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bgea-",    BBOCB(16,BOF,CBLT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bgea+",    BBOCB(16,BOF,CBLT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bgea",     BBOCB(16,BOF,CBLT,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bnla-",    BBOCB(16,BOF,CBLT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnla+",    BBOCB(16,BOF,CBLT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnla",     BBOCB(16,BOF,CBLT,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bgela-",   BBOCB(16,BOF,CBLT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bgela+",   BBOCB(16,BOF,CBLT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bgela",    BBOCB(16,BOF,CBLT,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bnlla-",   BBOCB(16,BOF,CBLT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnlla+",   BBOCB(16,BOF,CBLT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnlla",    BBOCB(16,BOF,CBLT,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"ble-",     BBOCB(16,BOF,CBGT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"ble+",     BBOCB(16,BOF,CBGT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"ble",      BBOCB(16,BOF,CBGT,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bng-",     BBOCB(16,BOF,CBGT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bng+",     BBOCB(16,BOF,CBGT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bng",      BBOCB(16,BOF,CBGT,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"blel-",    BBOCB(16,BOF,CBGT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"blel+",    BBOCB(16,BOF,CBGT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"blel",     BBOCB(16,BOF,CBGT,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bngl-",    BBOCB(16,BOF,CBGT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bngl+",    BBOCB(16,BOF,CBGT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bngl",     BBOCB(16,BOF,CBGT,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"blea-",    BBOCB(16,BOF,CBGT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"blea+",    BBOCB(16,BOF,CBGT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"blea",     BBOCB(16,BOF,CBGT,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bnga-",    BBOCB(16,BOF,CBGT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnga+",    BBOCB(16,BOF,CBGT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnga",     BBOCB(16,BOF,CBGT,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"blela-",   BBOCB(16,BOF,CBGT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"blela+",   BBOCB(16,BOF,CBGT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"blela",    BBOCB(16,BOF,CBGT,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bngla-",   BBOCB(16,BOF,CBGT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bngla+",   BBOCB(16,BOF,CBGT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bngla",    BBOCB(16,BOF,CBGT,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bne-",     BBOCB(16,BOF,CBEQ,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bne+",     BBOCB(16,BOF,CBEQ,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bne",      BBOCB(16,BOF,CBEQ,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bnel-",    BBOCB(16,BOF,CBEQ,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bnel+",    BBOCB(16,BOF,CBEQ,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bnel",     BBOCB(16,BOF,CBEQ,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bnea-",    BBOCB(16,BOF,CBEQ,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnea+",    BBOCB(16,BOF,CBEQ,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnea",     BBOCB(16,BOF,CBEQ,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bnela-",   BBOCB(16,BOF,CBEQ,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnela+",   BBOCB(16,BOF,CBEQ,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnela",    BBOCB(16,BOF,CBEQ,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bns-",     BBOCB(16,BOF,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bns+",     BBOCB(16,BOF,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bns",      BBOCB(16,BOF,CBSO,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bnu-",     BBOCB(16,BOF,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bnu+",     BBOCB(16,BOF,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bnu",      BBOCB(16,BOF,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BD}},
{"bnsl-",    BBOCB(16,BOF,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bnsl+",    BBOCB(16,BOF,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bnsl",     BBOCB(16,BOF,CBSO,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bnul-",    BBOCB(16,BOF,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bnul+",    BBOCB(16,BOF,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bnul",     BBOCB(16,BOF,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BD}},
{"bnsa-",    BBOCB(16,BOF,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnsa+",    BBOCB(16,BOF,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnsa",     BBOCB(16,BOF,CBSO,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bnua-",    BBOCB(16,BOF,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnua+",    BBOCB(16,BOF,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnua",     BBOCB(16,BOF,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDA}},
{"bnsla-",   BBOCB(16,BOF,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnsla+",   BBOCB(16,BOF,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnsla",    BBOCB(16,BOF,CBSO,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bnula-",   BBOCB(16,BOF,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bnula+",   BBOCB(16,BOF,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bnula",    BBOCB(16,BOF,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDA}},

{"blt-",     BBOCB(16,BOT,CBLT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"blt+",     BBOCB(16,BOT,CBLT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"blt",      BBOCB(16,BOT,CBLT,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bltl-",    BBOCB(16,BOT,CBLT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bltl+",    BBOCB(16,BOT,CBLT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bltl",     BBOCB(16,BOT,CBLT,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"blta-",    BBOCB(16,BOT,CBLT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"blta+",    BBOCB(16,BOT,CBLT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"blta",     BBOCB(16,BOT,CBLT,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bltla-",   BBOCB(16,BOT,CBLT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bltla+",   BBOCB(16,BOT,CBLT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bltla",    BBOCB(16,BOT,CBLT,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bgt-",     BBOCB(16,BOT,CBGT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bgt+",     BBOCB(16,BOT,CBGT,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bgt",      BBOCB(16,BOT,CBGT,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bgtl-",    BBOCB(16,BOT,CBGT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bgtl+",    BBOCB(16,BOT,CBGT,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bgtl",     BBOCB(16,BOT,CBGT,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bgta-",    BBOCB(16,BOT,CBGT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bgta+",    BBOCB(16,BOT,CBGT,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bgta",     BBOCB(16,BOT,CBGT,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bgtla-",   BBOCB(16,BOT,CBGT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bgtla+",   BBOCB(16,BOT,CBGT,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bgtla",    BBOCB(16,BOT,CBGT,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"beq-",     BBOCB(16,BOT,CBEQ,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"beq+",     BBOCB(16,BOT,CBEQ,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"beq",      BBOCB(16,BOT,CBEQ,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"beql-",    BBOCB(16,BOT,CBEQ,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"beql+",    BBOCB(16,BOT,CBEQ,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"beql",     BBOCB(16,BOT,CBEQ,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"beqa-",    BBOCB(16,BOT,CBEQ,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"beqa+",    BBOCB(16,BOT,CBEQ,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"beqa",     BBOCB(16,BOT,CBEQ,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"beqla-",   BBOCB(16,BOT,CBEQ,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"beqla+",   BBOCB(16,BOT,CBEQ,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"beqla",    BBOCB(16,BOT,CBEQ,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bso-",     BBOCB(16,BOT,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bso+",     BBOCB(16,BOT,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bso",      BBOCB(16,BOT,CBSO,0,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bun-",     BBOCB(16,BOT,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bun+",     BBOCB(16,BOT,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bun",      BBOCB(16,BOT,CBSO,0,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BD}},
{"bsol-",    BBOCB(16,BOT,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bsol+",    BBOCB(16,BOT,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bsol",     BBOCB(16,BOT,CBSO,0,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BD}},
{"bunl-",    BBOCB(16,BOT,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDM}},
{"bunl+",    BBOCB(16,BOT,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDP}},
{"bunl",     BBOCB(16,BOT,CBSO,0,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BD}},
{"bsoa-",    BBOCB(16,BOT,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bsoa+",    BBOCB(16,BOT,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bsoa",     BBOCB(16,BOT,CBSO,1,0),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"buna-",    BBOCB(16,BOT,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"buna+",    BBOCB(16,BOT,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"buna",     BBOCB(16,BOT,CBSO,1,0),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDA}},
{"bsola-",   BBOCB(16,BOT,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bsola+",   BBOCB(16,BOT,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bsola",    BBOCB(16,BOT,CBSO,1,1),	BBOATCB_MASK,  COM,	 PPCNONE,	{CR, BDA}},
{"bunla-",   BBOCB(16,BOT,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDMA}},
{"bunla+",   BBOCB(16,BOT,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDPA}},
{"bunla",    BBOCB(16,BOT,CBSO,1,1),	BBOATCB_MASK,  PPCCOM,	 PPCNONE,	{CR, BDA}},

{"bdnzf-",   BBO(16,BODNZF,0,0),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDM}},
{"bdnzf+",   BBO(16,BODNZF,0,0),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDP}},
{"bdnzf",    BBO(16,BODNZF,0,0),	BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BD}},
{"bdnzfl-",  BBO(16,BODNZF,0,1),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDM}},
{"bdnzfl+",  BBO(16,BODNZF,0,1),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDP}},
{"bdnzfl",   BBO(16,BODNZF,0,1),	BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BD}},
{"bdnzfa-",  BBO(16,BODNZF,1,0),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDMA}},
{"bdnzfa+",  BBO(16,BODNZF,1,0),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDPA}},
{"bdnzfa",   BBO(16,BODNZF,1,0),	BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bdnzfla-", BBO(16,BODNZF,1,1),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDMA}},
{"bdnzfla+", BBO(16,BODNZF,1,1),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDPA}},
{"bdnzfla",  BBO(16,BODNZF,1,1),	BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bdzf-",    BBO(16,BODZF,0,0),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDM}},
{"bdzf+",    BBO(16,BODZF,0,0),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDP}},
{"bdzf",     BBO(16,BODZF,0,0),		BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BD}},
{"bdzfl-",   BBO(16,BODZF,0,1),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDM}},
{"bdzfl+",   BBO(16,BODZF,0,1),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDP}},
{"bdzfl",    BBO(16,BODZF,0,1),		BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BD}},
{"bdzfa-",   BBO(16,BODZF,1,0),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDMA}},
{"bdzfa+",   BBO(16,BODZF,1,0),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDPA}},
{"bdzfa",    BBO(16,BODZF,1,0),		BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bdzfla-",  BBO(16,BODZF,1,1),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDMA}},
{"bdzfla+",  BBO(16,BODZF,1,1),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDPA}},
{"bdzfla",   BBO(16,BODZF,1,1),		BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BDA}},

{"bf-",      BBO(16,BOF,0,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDM}},
{"bf+",      BBO(16,BOF,0,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDP}},
{"bf",	     BBO(16,BOF,0,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BD}},
{"bbf",      BBO(16,BOF,0,0),		BBOAT_MASK,    PWRCOM,	 PPCNONE,	{BI, BD}},
{"bfl-",     BBO(16,BOF,0,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDM}},
{"bfl+",     BBO(16,BOF,0,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDP}},
{"bfl",      BBO(16,BOF,0,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BD}},
{"bbfl",     BBO(16,BOF,0,1),		BBOAT_MASK,    PWRCOM,	 PPCNONE,	{BI, BD}},
{"bfa-",     BBO(16,BOF,1,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDMA}},
{"bfa+",     BBO(16,BOF,1,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDPA}},
{"bfa",      BBO(16,BOF,1,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bbfa",     BBO(16,BOF,1,0),		BBOAT_MASK,    PWRCOM,	 PPCNONE,	{BI, BDA}},
{"bfla-",    BBO(16,BOF,1,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDMA}},
{"bfla+",    BBO(16,BOF,1,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDPA}},
{"bfla",     BBO(16,BOF,1,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bbfla",    BBO(16,BOF,1,1),		BBOAT_MASK,    PWRCOM,	 PPCNONE,	{BI, BDA}},

{"bdnzt-",   BBO(16,BODNZT,0,0),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDM}},
{"bdnzt+",   BBO(16,BODNZT,0,0),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDP}},
{"bdnzt",    BBO(16,BODNZT,0,0),	BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BD}},
{"bdnztl-",  BBO(16,BODNZT,0,1),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDM}},
{"bdnztl+",  BBO(16,BODNZT,0,1),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDP}},
{"bdnztl",   BBO(16,BODNZT,0,1),	BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BD}},
{"bdnzta-",  BBO(16,BODNZT,1,0),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDMA}},
{"bdnzta+",  BBO(16,BODNZT,1,0),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDPA}},
{"bdnzta",   BBO(16,BODNZT,1,0),	BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bdnztla-", BBO(16,BODNZT,1,1),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDMA}},
{"bdnztla+", BBO(16,BODNZT,1,1),	BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDPA}},
{"bdnztla",  BBO(16,BODNZT,1,1),	BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bdzt-",    BBO(16,BODZT,0,0),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDM}},
{"bdzt+",    BBO(16,BODZT,0,0),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDP}},
{"bdzt",     BBO(16,BODZT,0,0),		BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BD}},
{"bdztl-",   BBO(16,BODZT,0,1),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDM}},
{"bdztl+",   BBO(16,BODZT,0,1),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDP}},
{"bdztl",    BBO(16,BODZT,0,1),		BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BD}},
{"bdzta-",   BBO(16,BODZT,1,0),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDMA}},
{"bdzta+",   BBO(16,BODZT,1,0),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDPA}},
{"bdzta",    BBO(16,BODZT,1,0),		BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bdztla-",  BBO(16,BODZT,1,1),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDMA}},
{"bdztla+",  BBO(16,BODZT,1,1),		BBOY_MASK,     PPCCOM,   ISA_V2,	{BI, BDPA}},
{"bdztla",   BBO(16,BODZT,1,1),		BBOY_MASK,     PPCCOM,	 PPCNONE,	{BI, BDA}},

{"bt-",      BBO(16,BOT,0,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDM}},
{"bt+",      BBO(16,BOT,0,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDP}},
{"bt",	     BBO(16,BOT,0,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BD}},
{"bbt",      BBO(16,BOT,0,0),		BBOAT_MASK,    PWRCOM,	 PPCNONE,	{BI, BD}},
{"btl-",     BBO(16,BOT,0,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDM}},
{"btl+",     BBO(16,BOT,0,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDP}},
{"btl",      BBO(16,BOT,0,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BD}},
{"bbtl",     BBO(16,BOT,0,1),		BBOAT_MASK,    PWRCOM,	 PPCNONE,	{BI, BD}},
{"bta-",     BBO(16,BOT,1,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDMA}},
{"bta+",     BBO(16,BOT,1,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDPA}},
{"bta",      BBO(16,BOT,1,0),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bbta",     BBO(16,BOT,1,0),		BBOAT_MASK,    PWRCOM,	 PPCNONE,	{BI, BDA}},
{"btla-",    BBO(16,BOT,1,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDMA}},
{"btla+",    BBO(16,BOT,1,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDPA}},
{"btla",     BBO(16,BOT,1,1),		BBOAT_MASK,    PPCCOM,	 PPCNONE,	{BI, BDA}},
{"bbtla",    BBO(16,BOT,1,1),		BBOAT_MASK,    PWRCOM,	 PPCNONE,	{BI, BDA}},

{"bc-",		B(16,0,0),	B_MASK,      PPCCOM,	PPCNONE,	{BOE, BI, BDM}},
{"bc+",		B(16,0,0),	B_MASK,      PPCCOM,	PPCNONE,	{BOE, BI, BDP}},
{"bc",		B(16,0,0),	B_MASK,      COM,	PPCNONE,	{BO, BI, BD}},
{"bcl-",	B(16,0,1),	B_MASK,      PPCCOM,	PPCNONE,	{BOE, BI, BDM}},
{"bcl+",	B(16,0,1),	B_MASK,      PPCCOM,	PPCNONE,	{BOE, BI, BDP}},
{"bcl",		B(16,0,1),	B_MASK,      COM,	PPCNONE,	{BO, BI, BD}},
{"bca-",	B(16,1,0),	B_MASK,      PPCCOM,	PPCNONE,	{BOE, BI, BDMA}},
{"bca+",	B(16,1,0),	B_MASK,      PPCCOM,	PPCNONE,	{BOE, BI, BDPA}},
{"bca",		B(16,1,0),	B_MASK,      COM,	PPCNONE,	{BO, BI, BDA}},
{"bcla-",	B(16,1,1),	B_MASK,      PPCCOM,	PPCNONE,	{BOE, BI, BDMA}},
{"bcla+",	B(16,1,1),	B_MASK,      PPCCOM,	PPCNONE,	{BOE, BI, BDPA}},
{"bcla",	B(16,1,1),	B_MASK,      COM,	PPCNONE,	{BO, BI, BDA}},

{"svc",		SC(17,0,0),	SC_MASK,     POWER,	PPCNONE,	{SVC_LEV, FL1, FL2}},
{"svcl",	SC(17,0,1),	SC_MASK,     POWER,	PPCNONE,	{SVC_LEV, FL1, FL2}},
{"sc",		SC(17,1,0),	SC_MASK,     PPC,	PPCNONE,	{LEV}},
{"svca",	SC(17,1,0),	SC_MASK,     PWRCOM,	PPCNONE,	{SV}},
{"svcla",	SC(17,1,1),	SC_MASK,     POWER,	PPCNONE,	{SV}},

{"b",		B(18,0,0),	B_MASK,      COM,	PPCNONE,	{LI}},
{"bl",		B(18,0,1),	B_MASK,      COM,	PPCNONE,	{LI}},
{"ba",		B(18,1,0),	B_MASK,      COM,	PPCNONE,	{LIA}},
{"bla",		B(18,1,1),	B_MASK,      COM,	PPCNONE,	{LIA}},

{"mcrf",      XL(19,0), XLBB_MASK|(3<<21)|(3<<16), COM,	PPCNONE,	{BF, BFA}},

{"bdnzlr",   XLO(19,BODNZ,16,0),	XLBOBIBB_MASK, PPCCOM,	 PPCNONE,	{0}},
{"bdnzlr-",  XLO(19,BODNZ,16,0),	XLBOBIBB_MASK, PPCCOM,   ISA_V2,	{0}},
{"bdnzlrl",  XLO(19,BODNZ,16,1),	XLBOBIBB_MASK, PPCCOM,	 PPCNONE,	{0}},
{"bdnzlrl-", XLO(19,BODNZ,16,1),	XLBOBIBB_MASK, PPCCOM,   ISA_V2,	{0}},
{"bdnzlr+",  XLO(19,BODNZP,16,0),	XLBOBIBB_MASK, PPCCOM,   ISA_V2,	{0}},
{"bdnzlrl+", XLO(19,BODNZP,16,1),	XLBOBIBB_MASK, PPCCOM,   ISA_V2,	{0}},
{"bdzlr",    XLO(19,BODZ,16,0),		XLBOBIBB_MASK, PPCCOM,	 PPCNONE,	{0}},
{"bdzlr-",   XLO(19,BODZ,16,0),		XLBOBIBB_MASK, PPCCOM,   ISA_V2,	{0}},
{"bdzlrl",   XLO(19,BODZ,16,1),		XLBOBIBB_MASK, PPCCOM,	 PPCNONE,	{0}},
{"bdzlrl-",  XLO(19,BODZ,16,1),		XLBOBIBB_MASK, PPCCOM,   ISA_V2,	{0}},
{"bdzlr+",   XLO(19,BODZP,16,0),	XLBOBIBB_MASK, PPCCOM,   ISA_V2,	{0}},
{"bdzlrl+",  XLO(19,BODZP,16,1),	XLBOBIBB_MASK, PPCCOM,   ISA_V2,	{0}},
{"blr",      XLO(19,BOU,16,0),		XLBOBIBB_MASK, PPCCOM,	 PPCNONE,	{0}},
{"br",	     XLO(19,BOU,16,0),		XLBOBIBB_MASK, PWRCOM,	 PPCNONE,	{0}},
{"blrl",     XLO(19,BOU,16,1),		XLBOBIBB_MASK, PPCCOM,	 PPCNONE,	{0}},
{"brl",      XLO(19,BOU,16,1),		XLBOBIBB_MASK, PWRCOM,	 PPCNONE,	{0}},
{"bdnzlr-",  XLO(19,BODNZM4,16,0),	XLBOBIBB_MASK, ISA_V2,	 PPCNONE,	{0}},
{"bdnzlrl-", XLO(19,BODNZM4,16,1),	XLBOBIBB_MASK, ISA_V2,	 PPCNONE,	{0}},
{"bdnzlr+",  XLO(19,BODNZP4,16,0),	XLBOBIBB_MASK, ISA_V2,	 PPCNONE,	{0}},
{"bdnzlrl+", XLO(19,BODNZP4,16,1),	XLBOBIBB_MASK, ISA_V2,	 PPCNONE,	{0}},
{"bdzlr-",   XLO(19,BODZM4,16,0),	XLBOBIBB_MASK, ISA_V2,	 PPCNONE,	{0}},
{"bdzlrl-",  XLO(19,BODZM4,16,1),	XLBOBIBB_MASK, ISA_V2,	 PPCNONE,	{0}},
{"bdzlr+",   XLO(19,BODZP4,16,0),	XLBOBIBB_MASK, ISA_V2,	 PPCNONE,	{0}},
{"bdzlrl+",  XLO(19,BODZP4,16,1),	XLBOBIBB_MASK, ISA_V2,	 PPCNONE,	{0}},

{"bgelr",    XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bgelr-",   XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bger",     XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnllr",    XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnllr-",   XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnlr",     XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bgelrl",   XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bgelrl-",  XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgerl",    XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnllrl",   XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnllrl-",  XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnlrl",    XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"blelr",    XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"blelr-",   XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bler",     XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnglr",    XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnglr-",   XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bngr",     XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"blelrl",   XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"blelrl-",  XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"blerl",    XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnglrl",   XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnglrl-",  XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bngrl",    XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnelr",    XLOCB(19,BOF,CBEQ,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnelr-",   XLOCB(19,BOF,CBEQ,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bner",     XLOCB(19,BOF,CBEQ,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnelrl",   XLOCB(19,BOF,CBEQ,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnelrl-",  XLOCB(19,BOF,CBEQ,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnerl",    XLOCB(19,BOF,CBEQ,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnslr",    XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnslr-",   XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnsr",     XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnulr",    XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnulr-",   XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnslrl",   XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnslrl-",  XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnsrl",    XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bnulrl",   XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnulrl-",  XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgelr+",   XLOCB(19,BOFP,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnllr+",   XLOCB(19,BOFP,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgelrl+",  XLOCB(19,BOFP,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnllrl+",  XLOCB(19,BOFP,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"blelr+",   XLOCB(19,BOFP,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnglr+",   XLOCB(19,BOFP,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"blelrl+",  XLOCB(19,BOFP,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnglrl+",  XLOCB(19,BOFP,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnelr+",   XLOCB(19,BOFP,CBEQ,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnelrl+",  XLOCB(19,BOFP,CBEQ,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnslr+",   XLOCB(19,BOFP,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnulr+",   XLOCB(19,BOFP,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnslrl+",  XLOCB(19,BOFP,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnulrl+",  XLOCB(19,BOFP,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgelr-",   XLOCB(19,BOFM4,CBLT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnllr-",   XLOCB(19,BOFM4,CBLT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgelrl-",  XLOCB(19,BOFM4,CBLT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnllrl-",  XLOCB(19,BOFM4,CBLT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"blelr-",   XLOCB(19,BOFM4,CBGT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnglr-",   XLOCB(19,BOFM4,CBGT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"blelrl-",  XLOCB(19,BOFM4,CBGT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnglrl-",  XLOCB(19,BOFM4,CBGT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnelr-",   XLOCB(19,BOFM4,CBEQ,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnelrl-",  XLOCB(19,BOFM4,CBEQ,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnslr-",   XLOCB(19,BOFM4,CBSO,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnulr-",   XLOCB(19,BOFM4,CBSO,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnslrl-",  XLOCB(19,BOFM4,CBSO,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnulrl-",  XLOCB(19,BOFM4,CBSO,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgelr+",   XLOCB(19,BOFP4,CBLT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnllr+",   XLOCB(19,BOFP4,CBLT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgelrl+",  XLOCB(19,BOFP4,CBLT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnllrl+",  XLOCB(19,BOFP4,CBLT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"blelr+",   XLOCB(19,BOFP4,CBGT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnglr+",   XLOCB(19,BOFP4,CBGT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"blelrl+",  XLOCB(19,BOFP4,CBGT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnglrl+",  XLOCB(19,BOFP4,CBGT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnelr+",   XLOCB(19,BOFP4,CBEQ,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnelrl+",  XLOCB(19,BOFP4,CBEQ,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnslr+",   XLOCB(19,BOFP4,CBSO,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnulr+",   XLOCB(19,BOFP4,CBSO,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnslrl+",  XLOCB(19,BOFP4,CBSO,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnulrl+",  XLOCB(19,BOFP4,CBSO,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bltlr",    XLOCB(19,BOT,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bltlr-",   XLOCB(19,BOT,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltr",     XLOCB(19,BOT,CBLT,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bltlrl",   XLOCB(19,BOT,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bltlrl-",  XLOCB(19,BOT,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltrl",    XLOCB(19,BOT,CBLT,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bgtlr",    XLOCB(19,BOT,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bgtlr-",   XLOCB(19,BOT,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgtr",     XLOCB(19,BOT,CBGT,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bgtlrl",   XLOCB(19,BOT,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bgtlrl-",  XLOCB(19,BOT,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgtrl",    XLOCB(19,BOT,CBGT,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"beqlr",    XLOCB(19,BOT,CBEQ,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"beqlr-",   XLOCB(19,BOT,CBEQ,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"beqr",     XLOCB(19,BOT,CBEQ,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"beqlrl",   XLOCB(19,BOT,CBEQ,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"beqlrl-",  XLOCB(19,BOT,CBEQ,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"beqrl",    XLOCB(19,BOT,CBEQ,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bsolr",    XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bsolr-",   XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsor",     XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bunlr",    XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bunlr-",   XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsolrl",   XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bsolrl-",  XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsorl",    XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, PWRCOM,	 PPCNONE,	{CR}},
{"bunlrl",   XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bunlrl-",  XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltlr+",   XLOCB(19,BOTP,CBLT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltlrl+",  XLOCB(19,BOTP,CBLT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgtlr+",   XLOCB(19,BOTP,CBGT,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgtlrl+",  XLOCB(19,BOTP,CBGT,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"beqlr+",   XLOCB(19,BOTP,CBEQ,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"beqlrl+",  XLOCB(19,BOTP,CBEQ,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsolr+",   XLOCB(19,BOTP,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bunlr+",   XLOCB(19,BOTP,CBSO,16,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsolrl+",  XLOCB(19,BOTP,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bunlrl+",  XLOCB(19,BOTP,CBSO,16,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltlr-",   XLOCB(19,BOTM4,CBLT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bltlrl-",  XLOCB(19,BOTM4,CBLT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgtlr-",   XLOCB(19,BOTM4,CBGT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgtlrl-",  XLOCB(19,BOTM4,CBGT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"beqlr-",   XLOCB(19,BOTM4,CBEQ,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"beqlrl-",  XLOCB(19,BOTM4,CBEQ,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bsolr-",   XLOCB(19,BOTM4,CBSO,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bunlr-",   XLOCB(19,BOTM4,CBSO,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bsolrl-",  XLOCB(19,BOTM4,CBSO,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bunlrl-",  XLOCB(19,BOTM4,CBSO,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bltlr+",   XLOCB(19,BOTP4,CBLT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bltlrl+",  XLOCB(19,BOTP4,CBLT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgtlr+",   XLOCB(19,BOTP4,CBGT,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgtlrl+",  XLOCB(19,BOTP4,CBGT,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"beqlr+",   XLOCB(19,BOTP4,CBEQ,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"beqlrl+",  XLOCB(19,BOTP4,CBEQ,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bsolr+",   XLOCB(19,BOTP4,CBSO,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bunlr+",   XLOCB(19,BOTP4,CBSO,16,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bsolrl+",  XLOCB(19,BOTP4,CBSO,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bunlrl+",  XLOCB(19,BOTP4,CBSO,16,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},

{"bdnzflr",  XLO(19,BODNZF,16,0),	XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bdnzflr-", XLO(19,BODNZF,16,0),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdnzflrl", XLO(19,BODNZF,16,1),	XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bdnzflrl-",XLO(19,BODNZF,16,1),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdnzflr+", XLO(19,BODNZFP,16,0),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdnzflrl+",XLO(19,BODNZFP,16,1),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdzflr",   XLO(19,BODZF,16,0),	XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bdzflr-",  XLO(19,BODZF,16,0),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdzflrl",  XLO(19,BODZF,16,1),	XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bdzflrl-", XLO(19,BODZF,16,1),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdzflr+",  XLO(19,BODZFP,16,0),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdzflrl+", XLO(19,BODZFP,16,1),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bflr",     XLO(19,BOF,16,0),		XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bflr-",    XLO(19,BOF,16,0),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bbfr",     XLO(19,BOF,16,0),		XLBOBB_MASK,   PWRCOM,	 PPCNONE,	{BI}},
{"bflrl",    XLO(19,BOF,16,1),		XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bflrl-",   XLO(19,BOF,16,1),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bbfrl",    XLO(19,BOF,16,1),		XLBOBB_MASK,   PWRCOM,	 PPCNONE,	{BI}},
{"bflr+",    XLO(19,BOFP,16,0),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bflrl+",   XLO(19,BOFP,16,1),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bflr-",    XLO(19,BOFM4,16,0),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"bflrl-",   XLO(19,BOFM4,16,1),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"bflr+",    XLO(19,BOFP4,16,0),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"bflrl+",   XLO(19,BOFP4,16,1),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"bdnztlr",  XLO(19,BODNZT,16,0),	XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bdnztlr-", XLO(19,BODNZT,16,0),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdnztlrl", XLO(19,BODNZT,16,1),	XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bdnztlrl-",XLO(19,BODNZT,16,1),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdnztlr+", XLO(19,BODNZTP,16,0),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdnztlrl+",XLO(19,BODNZTP,16,1),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdztlr",   XLO(19,BODZT,16,0),	XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bdztlr-",  XLO(19,BODZT,16,0),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdztlrl",  XLO(19,BODZT,16,1),	XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bdztlrl-", XLO(19,BODZT,16,1),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdztlr+",  XLO(19,BODZTP,16,0),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bdztlrl+", XLO(19,BODZTP,16,1),	XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"btlr",     XLO(19,BOT,16,0),		XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"btlr-",    XLO(19,BOT,16,0),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bbtr",     XLO(19,BOT,16,0),		XLBOBB_MASK,   PWRCOM,	 PPCNONE,	{BI}},
{"btlrl",    XLO(19,BOT,16,1),		XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"btlrl-",   XLO(19,BOT,16,1),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bbtrl",    XLO(19,BOT,16,1),		XLBOBB_MASK,   PWRCOM,	 PPCNONE,	{BI}},
{"btlr+",    XLO(19,BOTP,16,0),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"btlrl+",   XLO(19,BOTP,16,1),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"btlr-",    XLO(19,BOTM4,16,0),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"btlrl-",   XLO(19,BOTM4,16,1),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"btlr+",    XLO(19,BOTP4,16,0),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"btlrl+",   XLO(19,BOTP4,16,1),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},

{"bclr-",    XLYLK(19,16,0,0),		XLYBB_MASK,    PPCCOM,	 PPCNONE,	{BOE, BI}},
{"bclrl-",   XLYLK(19,16,0,1),		XLYBB_MASK,    PPCCOM,	 PPCNONE,	{BOE, BI}},
{"bclr+",    XLYLK(19,16,1,0),		XLYBB_MASK,    PPCCOM,	 PPCNONE,	{BOE, BI}},
{"bclrl+",   XLYLK(19,16,1,1),		XLYBB_MASK,    PPCCOM,	 PPCNONE,	{BOE, BI}},
{"bclr",     XLLK(19,16,0),		XLBH_MASK,     PPCCOM,	 PPCNONE,	{BO, BI, BH}},
{"bcr",      XLLK(19,16,0),		XLBB_MASK,     PWRCOM,	 PPCNONE,	{BO, BI}},
{"bclrl",    XLLK(19,16,1),		XLBH_MASK,     PPCCOM,	 PPCNONE,	{BO, BI, BH}},
{"bcrl",     XLLK(19,16,1),		XLBB_MASK,     PWRCOM,	 PPCNONE,	{BO, BI}},

{"rfid",	XL(19,18),	0xffffffff,  PPC64,	PPCNONE,	{0}},

{"crnot",	XL(19,33),	XL_MASK,     PPCCOM,	PPCNONE,	{BT, BA, BBA}},
{"crnor",	XL(19,33),	XL_MASK,     COM,	PPCNONE,	{BT, BA, BB}},
{"rfmci",	X(19,38),   0xffffffff, PPCRFMCI|PPCA2|PPC476, PPCNONE,	{0}},

{"rfdi",	XL(19,39),	0xffffffff,  E500MC,	PPCNONE,	{0}},
{"rfi",		XL(19,50),	0xffffffff,  COM,	PPCNONE,	{0}},
{"rfci",	XL(19,51), 0xffffffff, PPC403|BOOKE|PPCE300|PPCA2|PPC476, PPCNONE, {0}},

{"rfsvc",	XL(19,82),	0xffffffff,  POWER,	PPCNONE,	{0}},

{"rfgi",	XL(19,102),   0xffffffff, E500MC|PPCA2,	PPCNONE,	{0}},

{"crandc",	XL(19,129),	XL_MASK,     COM,	PPCNONE,	{BT, BA, BB}},

{"rfebb",	XL(19,146),	XLS_MASK,    POWER8,	PPCNONE,	{SXL}},

{"isync",	XL(19,150),	0xffffffff,  PPCCOM,	PPCNONE,	{0}},
{"ics",		XL(19,150),	0xffffffff,  PWRCOM,	PPCNONE,	{0}},

{"crclr",	XL(19,193),	XL_MASK,     PPCCOM,	PPCNONE,	{BT, BAT, BBA}},
{"crxor",	XL(19,193),	XL_MASK,     COM,	PPCNONE,	{BT, BA, BB}},

{"dnh",		X(19,198),	X_MASK,      E500MC,	PPCNONE,	{DUI, DUIS}},

{"crnand",	XL(19,225),	XL_MASK,     COM,	PPCNONE,	{BT, BA, BB}},

{"crand",	XL(19,257),	XL_MASK,     COM,	PPCNONE,	{BT, BA, BB}},

{"hrfid",	XL(19,274),	0xffffffff, POWER5|CELL, PPC476,	{0}},

{"crset",	XL(19,289),	XL_MASK,     PPCCOM,	PPCNONE,	{BT, BAT, BBA}},
{"creqv",	XL(19,289),	XL_MASK,     COM,	PPCNONE,	{BT, BA, BB}},

{"doze",	XL(19,402),	0xffffffff,  POWER6,	PPCNONE,	{0}},

{"crorc",	XL(19,417),	XL_MASK,     COM,	PPCNONE,	{BT, BA, BB}},

{"nap",		XL(19,434),	0xffffffff,  POWER6,	PPCNONE,	{0}},

{"crmove",	XL(19,449),	XL_MASK,     PPCCOM,	PPCNONE,	{BT, BA, BBA}},
{"cror",	XL(19,449),	XL_MASK,     COM,	PPCNONE,	{BT, BA, BB}},

{"sleep",	XL(19,466),	0xffffffff,  POWER6,	PPCNONE,	{0}},
{"rvwinkle",	XL(19,498),	0xffffffff,  POWER6,	PPCNONE,	{0}},

{"bctr",    XLO(19,BOU,528,0),		XLBOBIBB_MASK, COM,	 PPCNONE,	{0}},
{"bctrl",   XLO(19,BOU,528,1),		XLBOBIBB_MASK, COM,	 PPCNONE,	{0}},

{"bgectr",  XLOCB(19,BOF,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bgectr-", XLOCB(19,BOF,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnlctr",  XLOCB(19,BOF,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnlctr-", XLOCB(19,BOF,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgectrl", XLOCB(19,BOF,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bgectrl-",XLOCB(19,BOF,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnlctrl", XLOCB(19,BOF,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnlctrl-",XLOCB(19,BOF,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"blectr",  XLOCB(19,BOF,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"blectr-", XLOCB(19,BOF,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bngctr",  XLOCB(19,BOF,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bngctr-", XLOCB(19,BOF,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"blectrl", XLOCB(19,BOF,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"blectrl-",XLOCB(19,BOF,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bngctrl", XLOCB(19,BOF,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bngctrl-",XLOCB(19,BOF,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnectr",  XLOCB(19,BOF,CBEQ,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnectr-", XLOCB(19,BOF,CBEQ,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnectrl", XLOCB(19,BOF,CBEQ,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnectrl-",XLOCB(19,BOF,CBEQ,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnsctr",  XLOCB(19,BOF,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnsctr-", XLOCB(19,BOF,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnuctr",  XLOCB(19,BOF,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnuctr-", XLOCB(19,BOF,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnsctrl", XLOCB(19,BOF,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnsctrl-",XLOCB(19,BOF,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnuctrl", XLOCB(19,BOF,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bnuctrl-",XLOCB(19,BOF,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgectr+", XLOCB(19,BOFP,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnlctr+", XLOCB(19,BOFP,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgectrl+",XLOCB(19,BOFP,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnlctrl+",XLOCB(19,BOFP,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"blectr+", XLOCB(19,BOFP,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bngctr+", XLOCB(19,BOFP,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"blectrl+",XLOCB(19,BOFP,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bngctrl+",XLOCB(19,BOFP,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnectr+", XLOCB(19,BOFP,CBEQ,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnectrl+",XLOCB(19,BOFP,CBEQ,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnsctr+", XLOCB(19,BOFP,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnuctr+", XLOCB(19,BOFP,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnsctrl+",XLOCB(19,BOFP,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bnuctrl+",XLOCB(19,BOFP,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgectr-", XLOCB(19,BOFM4,CBLT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnlctr-", XLOCB(19,BOFM4,CBLT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgectrl-",XLOCB(19,BOFM4,CBLT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnlctrl-",XLOCB(19,BOFM4,CBLT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"blectr-", XLOCB(19,BOFM4,CBGT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bngctr-", XLOCB(19,BOFM4,CBGT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"blectrl-",XLOCB(19,BOFM4,CBGT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bngctrl-",XLOCB(19,BOFM4,CBGT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnectr-", XLOCB(19,BOFM4,CBEQ,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnectrl-",XLOCB(19,BOFM4,CBEQ,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnsctr-", XLOCB(19,BOFM4,CBSO,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnuctr-", XLOCB(19,BOFM4,CBSO,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnsctrl-",XLOCB(19,BOFM4,CBSO,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnuctrl-",XLOCB(19,BOFM4,CBSO,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgectr+", XLOCB(19,BOFP4,CBLT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnlctr+", XLOCB(19,BOFP4,CBLT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgectrl+",XLOCB(19,BOFP4,CBLT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnlctrl+",XLOCB(19,BOFP4,CBLT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"blectr+", XLOCB(19,BOFP4,CBGT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bngctr+", XLOCB(19,BOFP4,CBGT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"blectrl+",XLOCB(19,BOFP4,CBGT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bngctrl+",XLOCB(19,BOFP4,CBGT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnectr+", XLOCB(19,BOFP4,CBEQ,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnectrl+",XLOCB(19,BOFP4,CBEQ,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnsctr+", XLOCB(19,BOFP4,CBSO,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnuctr+", XLOCB(19,BOFP4,CBSO,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnsctrl+",XLOCB(19,BOFP4,CBSO,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bnuctrl+",XLOCB(19,BOFP4,CBSO,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bltctr",  XLOCB(19,BOT,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bltctr-", XLOCB(19,BOT,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltctrl", XLOCB(19,BOT,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bltctrl-",XLOCB(19,BOT,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgtctr",  XLOCB(19,BOT,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bgtctr-", XLOCB(19,BOT,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgtctrl", XLOCB(19,BOT,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bgtctrl-",XLOCB(19,BOT,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"beqctr",  XLOCB(19,BOT,CBEQ,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"beqctr-", XLOCB(19,BOT,CBEQ,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"beqctrl", XLOCB(19,BOT,CBEQ,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"beqctrl-",XLOCB(19,BOT,CBEQ,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsoctr",  XLOCB(19,BOT,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bsoctr-", XLOCB(19,BOT,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bunctr",  XLOCB(19,BOT,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bunctr-", XLOCB(19,BOT,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsoctrl", XLOCB(19,BOT,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bsoctrl-",XLOCB(19,BOT,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bunctrl", XLOCB(19,BOT,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,	 PPCNONE,	{CR}},
{"bunctrl-",XLOCB(19,BOT,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltctr+", XLOCB(19,BOTP,CBLT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltctrl+",XLOCB(19,BOTP,CBLT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgtctr+", XLOCB(19,BOTP,CBGT,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bgtctrl+",XLOCB(19,BOTP,CBGT,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"beqctr+", XLOCB(19,BOTP,CBEQ,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"beqctrl+",XLOCB(19,BOTP,CBEQ,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsoctr+", XLOCB(19,BOTP,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bunctr+", XLOCB(19,BOTP,CBSO,528,0),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bsoctrl+",XLOCB(19,BOTP,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bunctrl+",XLOCB(19,BOTP,CBSO,528,1),	XLBOCBBB_MASK, PPCCOM,   ISA_V2,	{CR}},
{"bltctr-", XLOCB(19,BOTM4,CBLT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bltctrl-",XLOCB(19,BOTM4,CBLT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgtctr-", XLOCB(19,BOTM4,CBGT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgtctrl-",XLOCB(19,BOTM4,CBGT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"beqctr-", XLOCB(19,BOTM4,CBEQ,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"beqctrl-",XLOCB(19,BOTM4,CBEQ,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bsoctr-", XLOCB(19,BOTM4,CBSO,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bunctr-", XLOCB(19,BOTM4,CBSO,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bsoctrl-",XLOCB(19,BOTM4,CBSO,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bunctrl-",XLOCB(19,BOTM4,CBSO,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bltctr+", XLOCB(19,BOTP4,CBLT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bltctrl+",XLOCB(19,BOTP4,CBLT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgtctr+", XLOCB(19,BOTP4,CBGT,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bgtctrl+",XLOCB(19,BOTP4,CBGT,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"beqctr+", XLOCB(19,BOTP4,CBEQ,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"beqctrl+",XLOCB(19,BOTP4,CBEQ,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bsoctr+", XLOCB(19,BOTP4,CBSO,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bunctr+", XLOCB(19,BOTP4,CBSO,528,0),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bsoctrl+",XLOCB(19,BOTP4,CBSO,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},
{"bunctrl+",XLOCB(19,BOTP4,CBSO,528,1),	XLBOCBBB_MASK, ISA_V2,	 PPCNONE,	{CR}},

{"bfctr",   XLO(19,BOF,528,0),		XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bfctr-",  XLO(19,BOF,528,0),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bfctrl",  XLO(19,BOF,528,1),		XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"bfctrl-", XLO(19,BOF,528,1),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bfctr+",  XLO(19,BOFP,528,0),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bfctrl+", XLO(19,BOFP,528,1),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"bfctr-",  XLO(19,BOFM4,528,0),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"bfctrl-", XLO(19,BOFM4,528,1),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"bfctr+",  XLO(19,BOFP4,528,0),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"bfctrl+", XLO(19,BOFP4,528,1),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"btctr",   XLO(19,BOT,528,0),		XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"btctr-",  XLO(19,BOT,528,0),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"btctrl",  XLO(19,BOT,528,1),		XLBOBB_MASK,   PPCCOM,	 PPCNONE,	{BI}},
{"btctrl-", XLO(19,BOT,528,1),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"btctr+",  XLO(19,BOTP,528,0),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"btctrl+", XLO(19,BOTP,528,1),		XLBOBB_MASK,   PPCCOM,   ISA_V2,	{BI}},
{"btctr-",  XLO(19,BOTM4,528,0),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"btctrl-", XLO(19,BOTM4,528,1),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"btctr+",  XLO(19,BOTP4,528,0),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},
{"btctrl+", XLO(19,BOTP4,528,1),	XLBOBB_MASK,   ISA_V2,	 PPCNONE,	{BI}},

{"bcctr-",  XLYLK(19,528,0,0),		XLYBB_MASK,    PPCCOM,	 PPCNONE,	{BOE, BI}},
{"bcctrl-", XLYLK(19,528,0,1),		XLYBB_MASK,    PPCCOM,	 PPCNONE,	{BOE, BI}},
{"bcctr+",  XLYLK(19,528,1,0),		XLYBB_MASK,    PPCCOM,	 PPCNONE,	{BOE, BI}},
{"bcctrl+", XLYLK(19,528,1,1),		XLYBB_MASK,    PPCCOM,	 PPCNONE,	{BOE, BI}},
{"bcctr",   XLLK(19,528,0),		XLBH_MASK,     PPCCOM,	 PPCNONE,	{BO, BI, BH}},
{"bcc",     XLLK(19,528,0),		XLBB_MASK,     PWRCOM,	 PPCNONE,	{BO, BI}},
{"bcctrl",  XLLK(19,528,1),		XLBH_MASK,     PPCCOM,	 PPCNONE,	{BO, BI, BH}},
{"bccl",    XLLK(19,528,1),		XLBB_MASK,     PWRCOM,	 PPCNONE,	{BO, BI}},

{"bctar-",  XLYLK(19,560,0,0),		XLYBB_MASK,    POWER8,	 PPCNONE,	{BOE, BI}},
{"bctarl-", XLYLK(19,560,0,1),		XLYBB_MASK,    POWER8,	 PPCNONE,	{BOE, BI}},
{"bctar+",  XLYLK(19,560,1,0),		XLYBB_MASK,    POWER8,	 PPCNONE,	{BOE, BI}},
{"bctarl+", XLYLK(19,560,1,1),		XLYBB_MASK,    POWER8,	 PPCNONE,	{BOE, BI}},
{"bctar",   XLLK(19,560,0),		XLBH_MASK,     POWER8,	 PPCNONE,	{BO, BI, BH}},
{"bctarl",  XLLK(19,560,1),		XLBH_MASK,     POWER8,	 PPCNONE,	{BO, BI, BH}},

{"rlwimi",	M(20,0),	M_MASK,      PPCCOM,	PPCNONE,	{RA, RS, SH, MBE, ME}},
{"rlimi",	M(20,0),	M_MASK,      PWRCOM,	PPCNONE,	{RA, RS, SH, MBE, ME}},

{"rlwimi.",	M(20,1),	M_MASK,      PPCCOM,	PPCNONE,	{RA, RS, SH, MBE, ME}},
{"rlimi.",	M(20,1),	M_MASK,      PWRCOM,	PPCNONE,	{RA, RS, SH, MBE, ME}},

{"rotlwi",	MME(21,31,0),	MMBME_MASK,  PPCCOM,	PPCNONE,	{RA, RS, SH}},
{"clrlwi",	MME(21,31,0),	MSHME_MASK,  PPCCOM,	PPCNONE,	{RA, RS, MB}},
{"rlwinm",	M(21,0),	M_MASK,      PPCCOM,	PPCNONE,	{RA, RS, SH, MBE, ME}},
{"rlinm",	M(21,0),	M_MASK,      PWRCOM,	PPCNONE,	{RA, RS, SH, MBE, ME}},
{"rotlwi.",	MME(21,31,1),	MMBME_MASK,  PPCCOM,	PPCNONE,	{RA, RS, SH}},
{"clrlwi.",	MME(21,31,1),	MSHME_MASK,  PPCCOM,	PPCNONE,	{RA, RS, MB}},
{"rlwinm.",	M(21,1),	M_MASK,      PPCCOM,	PPCNONE,	{RA, RS, SH, MBE, ME}},
{"rlinm.",	M(21,1),	M_MASK,      PWRCOM,	PPCNONE,	{RA, RS, SH, MBE, ME}},

{"rlmi",	M(22,0),	M_MASK,      M601,	PPCNONE,	{RA, RS, RB, MBE, ME}},
{"rlmi.",	M(22,1),	M_MASK,      M601,	PPCNONE,	{RA, RS, RB, MBE, ME}},

{"rotlw",	MME(23,31,0),	MMBME_MASK,  PPCCOM,	PPCNONE,	{RA, RS, RB}},
{"rlwnm",	M(23,0),	M_MASK,      PPCCOM,	PPCNONE,	{RA, RS, RB, MBE, ME}},
{"rlnm",	M(23,0),	M_MASK,      PWRCOM,	PPCNONE,	{RA, RS, RB, MBE, ME}},
{"rotlw.",	MME(23,31,1),	MMBME_MASK,  PPCCOM,	PPCNONE,	{RA, RS, RB}},
{"rlwnm.",	M(23,1),	M_MASK,      PPCCOM,	PPCNONE,	{RA, RS, RB, MBE, ME}},
{"rlnm.",	M(23,1),	M_MASK,      PWRCOM,	PPCNONE,	{RA, RS, RB, MBE, ME}},

{"nop",		OP(24),		0xffffffff,  PPCCOM,	PPCNONE,	{0}},
{"ori",		OP(24),		OP_MASK,     PPCCOM,	PPCNONE,	{RA, RS, UI}},
{"oril",	OP(24),		OP_MASK,     PWRCOM,	PPCNONE,	{RA, RS, UI}},

{"oris",	OP(25),		OP_MASK,     PPCCOM,	PPCNONE,	{RA, RS, UI}},
{"oriu",	OP(25),		OP_MASK,     PWRCOM,	PPCNONE,	{RA, RS, UI}},

{"xnop",	OP(26),		0xffffffff,  PPCCOM,	PPCNONE,	{0}},
{"xori",	OP(26),		OP_MASK,     PPCCOM,	PPCNONE,	{RA, RS, UI}},
{"xoril",	OP(26),		OP_MASK,     PWRCOM,	PPCNONE,	{RA, RS, UI}},

{"xoris",	OP(27),		OP_MASK,     PPCCOM,	PPCNONE,	{RA, RS, UI}},
{"xoriu",	OP(27),		OP_MASK,     PWRCOM,	PPCNONE,	{RA, RS, UI}},

{"andi.",	OP(28),		OP_MASK,     PPCCOM,	PPCNONE,	{RA, RS, UI}},
{"andil.",	OP(28),		OP_MASK,     PWRCOM,	PPCNONE,	{RA, RS, UI}},

{"andis.",	OP(29),		OP_MASK,     PPCCOM,	PPCNONE,	{RA, RS, UI}},
{"andiu.",	OP(29),		OP_MASK,     PWRCOM,	PPCNONE,	{RA, RS, UI}},

{"rotldi",	MD(30,0,0),	MDMB_MASK,   PPC64,	PPCNONE,	{RA, RS, SH6}},
{"clrldi",	MD(30,0,0),	MDSH_MASK,   PPC64,	PPCNONE,	{RA, RS, MB6}},
{"rldicl",	MD(30,0,0),	MD_MASK,     PPC64,	PPCNONE,	{RA, RS, SH6, MB6}},
{"rotldi.",	MD(30,0,1),	MDMB_MASK,   PPC64,	PPCNONE,	{RA, RS, SH6}},
{"clrldi.",	MD(30,0,1),	MDSH_MASK,   PPC64,	PPCNONE,	{RA, RS, MB6}},
{"rldicl.",	MD(30,0,1),	MD_MASK,     PPC64,	PPCNONE,	{RA, RS, SH6, MB6}},

{"rldicr",	MD(30,1,0),	MD_MASK,     PPC64,	PPCNONE,	{RA, RS, SH6, ME6}},
{"rldicr.",	MD(30,1,1),	MD_MASK,     PPC64,	PPCNONE,	{RA, RS, SH6, ME6}},

{"rldic",	MD(30,2,0),	MD_MASK,     PPC64,	PPCNONE,	{RA, RS, SH6, MB6}},
{"rldic.",	MD(30,2,1),	MD_MASK,     PPC64,	PPCNONE,	{RA, RS, SH6, MB6}},

{"rldimi",	MD(30,3,0),	MD_MASK,     PPC64,	PPCNONE,	{RA, RS, SH6, MB6}},
{"rldimi.",	MD(30,3,1),	MD_MASK,     PPC64,	PPCNONE,	{RA, RS, SH6, MB6}},

{"rotld",	MDS(30,8,0),	MDSMB_MASK,  PPC64,	PPCNONE,	{RA, RS, RB}},
{"rldcl",	MDS(30,8,0),	MDS_MASK,    PPC64,	PPCNONE,	{RA, RS, RB, MB6}},
{"rotld.",	MDS(30,8,1),	MDSMB_MASK,  PPC64,	PPCNONE,	{RA, RS, RB}},
{"rldcl.",	MDS(30,8,1),	MDS_MASK,    PPC64,	PPCNONE,	{RA, RS, RB, MB6}},

{"rldcr",	MDS(30,9,0),	MDS_MASK,    PPC64,	PPCNONE,	{RA, RS, RB, ME6}},
{"rldcr.",	MDS(30,9,1),	MDS_MASK,    PPC64,	PPCNONE,	{RA, RS, RB, ME6}},

{"cmpw",	XOPL(31,0,0),	XCMPL_MASK,  PPCCOM,	PPCNONE,	{OBF, RA, RB}},
{"cmpd",	XOPL(31,0,1),	XCMPL_MASK,  PPC64,	PPCNONE,	{OBF, RA, RB}},
{"cmp",		X(31,0),	XCMP_MASK,   PPC|PPCVLE, PPCNONE,	{BF, L, RA, RB}},
{"cmp",		X(31,0),	XCMPL_MASK,  PWRCOM,	PPC,		{BF, RA, RB}},

{"twlgt",	XTO(31,4,TOLGT), XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tlgt",	XTO(31,4,TOLGT), XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twllt",	XTO(31,4,TOLLT), XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tllt",	XTO(31,4,TOLLT), XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"tweq",	XTO(31,4,TOEQ),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"teq",		XTO(31,4,TOEQ),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twlge",	XTO(31,4,TOLGE), XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tlge",	XTO(31,4,TOLGE), XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twlnl",	XTO(31,4,TOLNL), XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tlnl",	XTO(31,4,TOLNL), XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twlle",	XTO(31,4,TOLLE), XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tlle",	XTO(31,4,TOLLE), XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twlng",	XTO(31,4,TOLNG), XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tlng",	XTO(31,4,TOLNG), XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twgt",	XTO(31,4,TOGT),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tgt",		XTO(31,4,TOGT),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twge",	XTO(31,4,TOGE),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tge",		XTO(31,4,TOGE),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twnl",	XTO(31,4,TONL),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tnl",		XTO(31,4,TONL),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twlt",	XTO(31,4,TOLT),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tlt",		XTO(31,4,TOLT),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twle",	XTO(31,4,TOLE),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tle",		XTO(31,4,TOLE),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twng",	XTO(31,4,TONG),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tng",		XTO(31,4,TONG),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"twne",	XTO(31,4,TONE),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tne",		XTO(31,4,TONE),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"trap",	XTO(31,4,TOU),	 0xffffffff, PPCCOM|PPCVLE, PPCNONE,	{0}},
{"twu",		XTO(31,4,TOU),	 XTO_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RA, RB}},
{"tu",		XTO(31,4,TOU),	 XTO_MASK,   PWRCOM,	PPCNONE,	{RA, RB}},
{"tw",		X(31,4),	 X_MASK, PPCCOM|PPCVLE, PPCNONE,	{TO, RA, RB}},
{"t",		X(31,4),	 X_MASK,     PWRCOM,	PPCNONE,	{TO, RA, RB}},

{"lvsl",	X(31,6),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VD, RA0, RB}},
{"lvebx",	X(31,7),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VD, RA0, RB}},
{"lbfcmx",	APU(31,7,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"subfc",	XO(31,8,0,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sf",		XO(31,8,0,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"subc",	XO(31,8,0,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RB, RA}},
{"subfc.",	XO(31,8,0,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sf.",		XO(31,8,0,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"subc.",	XO(31,8,0,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RB, RA}},
 
{"mulhdu",	XO(31,9,0,0),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulhdu.",	XO(31,9,0,1),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
 
{"addc",	XO(31,10,0,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"a",		XO(31,10,0,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"addc.",	XO(31,10,0,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"a.",		XO(31,10,0,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"mulhwu",	XO(31,11,0,0),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulhwu.",	XO(31,11,0,1),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"lxsiwzx",	X(31,12),	XX1_MASK,    PPCVSX2,	PPCNONE,	{XT6, RA0, RB}},

{"isellt",	X(31,15),	X_MASK,      PPCISEL,	PPCNONE,	{RT, RA0, RB}},

{"tlbilxlpid",	XTO(31,18,0),	XTO_MASK, E500MC|PPCA2,	PPCNONE,	{0}},
{"tlbilxpid",	XTO(31,18,1),	XTO_MASK, E500MC|PPCA2,	PPCNONE,	{0}},
{"tlbilxva",	XTO(31,18,3),	XTO_MASK, E500MC|PPCA2,	PPCNONE,	{RA0, RB}},
{"tlbilx",	X(31,18),	X_MASK,   E500MC|PPCA2,	PPCNONE,	{T, RA0, RB}},

{"mfcr",	XFXM(31,19,0,0), XFXFXM_MASK, POWER4,	PPCNONE,	{RT, FXM4}},
{"mfcr",	XFXM(31,19,0,0), XRARB_MASK, COM|PPCVLE, POWER4,	{RT}},
{"mfocrf",	XFXM(31,19,0,1), XFXFXM_MASK, COM|PPCVLE, PPCNONE,	{RT, FXM}},

{"lwarx",	X(31,20),	XEH_MASK,    PPC|PPCVLE, PPCNONE,	{RT, RA0, RB, EH}},
 
{"ldx",		X(31,21),	X_MASK,	     PPC64|PPCVLE, PPCNONE,	{RT, RA0, RB}},
 
{"icbt",	X(31,22),	X_MASK,	     BOOKE|PPCE300|PPCA2|PPC476|PPCVLE, PPCNONE, {CT, RA0, RB}},
 
{"lwzx",	X(31,23),	X_MASK,	     PPCCOM|PPCVLE, PPCNONE,	{RT, RA0, RB}},
{"lx",		X(31,23),	X_MASK,      PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"slw",		XRC(31,24,0),	X_MASK,	     PPCCOM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"sl",		XRC(31,24,0),	X_MASK,      PWRCOM,	PPCNONE,	{RA, RS, RB}},
{"slw.",	XRC(31,24,1),	X_MASK,	     PPCCOM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"sl.",		XRC(31,24,1),	X_MASK,      PWRCOM,	PPCNONE,	{RA, RS, RB}},

{"cntlzw",	XRC(31,26,0),	XRB_MASK,    PPCCOM|PPCVLE, PPCNONE,	{RA, RS}},
{"cntlz",	XRC(31,26,0),	XRB_MASK,    PWRCOM,	PPCNONE,	{RA, RS}},
{"cntlzw.",	XRC(31,26,1),	XRB_MASK,    PPCCOM|PPCVLE, PPCNONE,	{RA, RS}},
{"cntlz.",	XRC(31,26,1),	XRB_MASK,    PWRCOM,	PPCNONE,	{RA, RS}},

{"sld",		XRC(31,27,0),	X_MASK,      PPC64,	PPCNONE,	{RA, RS, RB}},
{"sld.",	XRC(31,27,1),	X_MASK,      PPC64,	PPCNONE,	{RA, RS, RB}},

{"and",		XRC(31,28,0),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"and.",	XRC(31,28,1),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RA, RS, RB}},

{"maskg",	XRC(31,29,0),	X_MASK,      M601,	PPCA2,		{RA, RS, RB}},
{"maskg.",	XRC(31,29,1),	X_MASK,      M601,	PPCA2,		{RA, RS, RB}},

{"ldepx",	X(31,29),	X_MASK,      E500MC|PPCA2|PPCVLE, PPCNONE, {RT, RA0, RB}},

{"waitasec",	X(31,30),	XRTRARB_MASK,POWER8,	PPCNONE,	{0}},

{"lwepx",	X(31,31),	X_MASK,	     E500MC|PPCA2|PPCVLE, PPCNONE, {RT, RA0, RB}},

{"cmplw",	XOPL(31,32,0),	XCMPL_MASK,  PPCCOM|PPCVLE, PPCNONE,	{OBF, RA, RB}},
{"cmpld",	XOPL(31,32,1),	XCMPL_MASK,  PPC64,	PPCNONE,	{OBF, RA, RB}},
{"cmpl",	X(31,32),	XCMP_MASK,   PPC|PPCVLE, PPCNONE,	{BF, L, RA, RB}},
{"cmpl",	X(31,32),	XCMPL_MASK,  PWRCOM,	PPC,		{BF, RA, RB}},

{"lvsr",	X(31,38),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VD, RA0, RB}},
{"lvehx",	X(31,39),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VD, RA0, RB}},
{"lhfcmx",	APU(31,39,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"mviwsplt",	X(31,46),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA, RB}},

{"iselgt",	X(31,47),	X_MASK,      PPCISEL,	PPCNONE,	{RT, RA0, RB}},

{"lvewx",	X(31,71),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VD, RA0, RB}},

{"addg6s",	XO(31,74,0,0),	XO_MASK,     POWER6,	PPCNONE,	{RT, RA, RB}},

{"lxsiwax",	X(31,76),	XX1_MASK,    PPCVSX2,	PPCNONE,	{XT6, RA0, RB}},

{"iseleq",	X(31,79),	X_MASK,      PPCISEL,	PPCNONE,	{RT, RA0, RB}},

{"isel",	XISEL(31,15), XISEL_MASK, PPCISEL|TITAN|PPCVLE, PPCNONE, {RT, RA0, RB, CRB}},

{"subf",	XO(31,40,0,0),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sub",		XO(31,40,0,0),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RB, RA}},
{"subf.",	XO(31,40,0,1),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sub.",	XO(31,40,0,1),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RB, RA}},

{"mfvsrd",	X(31,51),	XX1RB_MASK,   PPCVSX2,	PPCNONE,	{RA, XS6}},
{"mffprd",	X(31,51),	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{RA, FRS}},
{"mfvrd",	X(31,51)|1,	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{RA, VS}},
{"eratilx",	X(31,51),	X_MASK,	     PPCA2,	PPCNONE,	{ERAT_T, RA, RB}},

{"lbarx",	X(31,52),	XEH_MASK,    POWER7|PPCVLE, PPCNONE,	{RT, RA0, RB, EH}},

{"ldux",	X(31,53),	X_MASK,      PPC64|PPCVLE, PPCNONE,	{RT, RAL, RB}},
 
{"dcbst",	X(31,54),	XRT_MASK,    PPC|PPCVLE, PPCNONE,	{RA0, RB}},
 
{"lwzux",	X(31,55),	X_MASK,	     PPCCOM|PPCVLE, PPCNONE,	{RT, RAL, RB}},
{"lux",		X(31,55),	X_MASK,      PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"cntlzd",	XRC(31,58,0),	XRB_MASK,    PPC64|PPCVLE, PPCNONE,	{RA, RS}},
{"cntlzd.",	XRC(31,58,1),	XRB_MASK,    PPC64|PPCVLE, PPCNONE,	{RA, RS}},

{"andc",	XRC(31,60,0),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"andc.",	XRC(31,60,1),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RA, RS, RB}},

{"waitrsv",	X(31,62)|(1<<21), 0xffffffff, POWER7|E500MC|PPCA2, PPCNONE, {0}},
{"waitimpl",	X(31,62)|(2<<21), 0xffffffff, POWER7|E500MC|PPCA2, PPCNONE, {0}},
{"wait",	X(31,62),	XWC_MASK,    POWER7|E500MC|PPCA2|PPCVLE, PPCNONE, {WC}},
 
{"dcbstep",	XRT(31,63,0),	XRT_MASK,    E500MC|PPCA2|PPCVLE, PPCNONE, {RA0, RB}},

{"tdlgt",	XTO(31,68,TOLGT), XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdllt",	XTO(31,68,TOLLT), XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdeq",	XTO(31,68,TOEQ),  XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdlge",	XTO(31,68,TOLGE), XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdlnl",	XTO(31,68,TOLNL), XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdlle",	XTO(31,68,TOLLE), XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdlng",	XTO(31,68,TOLNG), XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdgt",	XTO(31,68,TOGT),  XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdge",	XTO(31,68,TOGE),  XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdnl",	XTO(31,68,TONL),  XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdlt",	XTO(31,68,TOLT),  XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdle",	XTO(31,68,TOLE),  XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdng",	XTO(31,68,TONG),  XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdne",	XTO(31,68,TONE),  XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"tdu",		XTO(31,68,TOU),   XTO_MASK,  PPC64,	PPCNONE,	{RA, RB}},
{"td",		X(31,68),	X_MASK,      PPC64|PPCVLE, PPCNONE,	{TO, RA, RB}},

{"lwfcmx",	APU(31,71,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},
{"mulhd",	XO(31,73,0,0),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulhd.",	XO(31,73,0,1),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
 
{"mulhw",	XO(31,75,0,0),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulhw.",	XO(31,75,0,1),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"dlmzb",	XRC(31,78,0), X_MASK, PPC403|PPC440|TITAN|PPCVLE, PPCNONE, {RA, RS, RB}},
{"dlmzb.",	XRC(31,78,1), X_MASK, PPC403|PPC440|TITAN|PPCVLE, PPCNONE, {RA, RS, RB}},

{"mtsrd",	X(31,82),  XRB_MASK|(1<<20), PPC64,	PPCNONE,	{SR, RS}},

{"mfmsr",	X(31,83),	XRARB_MASK,  COM|PPCVLE, PPCNONE,	{RT}},
 
{"ldarx",	X(31,84),	XEH_MASK,    PPC64|PPCVLE, PPCNONE,	{RT, RA0, RB, EH}},

{"dcbfl",	XOPL(31,86,1),	XRT_MASK,    POWER5,	PPC476,		{RA0, RB}},
{"dcbf",	X(31,86),	XLRT_MASK,   PPC|PPCVLE, PPCNONE,	{RA0, RB, L}},

{"lbzx",	X(31,87),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RT, RA0, RB}},
 
{"lbepx",	X(31,95),	X_MASK,	     E500MC|PPCA2|PPCVLE, PPCNONE, {RT, RA0, RB}},

{"dni",		XRC(31,97,1),	XRB_MASK,    E6500,	PPCNONE,	{DUI, DCTL}},

{"lvx",		X(31,103),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VD, RA0, RB}},
{"lqfcmx",	APU(31,103,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"neg",		XO(31,104,0,0),	XORB_MASK,   COM|PPCVLE, PPCNONE,	{RT, RA}},
{"neg.",	XO(31,104,0,1),	XORB_MASK,   COM|PPCVLE, PPCNONE,	{RT, RA}},

{"mul",		XO(31,107,0,0),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},
{"mul.",	XO(31,107,0,1),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},

{"mvidsplt",	X(31,110),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA, RB}},

{"mtsrdin",	X(31,114),	XRA_MASK,    PPC64,	PPCNONE,	{RS, RB}},

{"mffprwz",	X(31,115),	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{RA, FRS}},
{"mfvrwz",	X(31,115)|1,	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{RA, VS}},
{"mfvsrwz",	X(31,115),	XX1RB_MASK,   PPCVSX2,	PPCNONE,	{RA, XS6}},

{"lharx",	X(31,116),	XEH_MASK,    POWER7|PPCVLE, PPCNONE,	{RT, RA0, RB, EH}},

{"clf",		X(31,118),	XTO_MASK,    POWER,	PPCNONE,	{RA, RB}},

{"lbzux",	X(31,119),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RT, RAL, RB}},
 
{"popcntb",	X(31,122),	XRB_MASK,    POWER5|PPCVLE, PPCNONE,	{RA, RS}},

{"not",		XRC(31,124,0),	X_MASK,      COM,	PPCNONE,	{RA, RS, RBS}},
{"nor",		XRC(31,124,0),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"not.",	XRC(31,124,1),	X_MASK,      COM,	PPCNONE,	{RA, RS, RBS}},
{"nor.",	XRC(31,124,1),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},

{"dcbfep",	XRT(31,127,0),	XRT_MASK,    E500MC|PPCA2|PPCVLE, PPCNONE, {RA0, RB}},
 
{"wrtee",	X(31,131),	XRARB_MASK,  PPC403|BOOKE|PPCA2|PPC476|PPCVLE, PPCNONE, {RS}},
 
{"dcbtstls",	X(31,134),	X_MASK,	     PPCCHLK|PPC476|TITAN|PPCVLE, PPCNONE, {CT, RA0, RB}},

{"stvebx",	X(31,135),	X_MASK,      PPCVEC,	PPCNONE,	{VS, RA0, RB}},
{"stbfcmx",	APU(31,135,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"subfe",	XO(31,136,0,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sfe",		XO(31,136,0,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"subfe.",	XO(31,136,0,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sfe.",	XO(31,136,0,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"adde",	XO(31,138,0,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ae",		XO(31,138,0,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"adde.",	XO(31,138,0,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ae.",		XO(31,138,0,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"stxsiwx",	X(31,140),	XX1_MASK,    PPCVSX2,	PPCNONE,	{XS6, RA0, RB}},

{"msgsndp",	XRTRA(31,142,0,0), XRTRA_MASK, POWER8,	PPCNONE,	{RB}},
{"dcbtstlse",	X(31,142),	X_MASK,      PPCCHLK,	E500MC,		{CT, RA0, RB}},

{"mtcr",	XFXM(31,144,0xff,0), XRARB_MASK, COM,	PPCNONE,	{RS}},
{"mtcrf",	XFXM(31,144,0,0), XFXFXM_MASK, COM|PPCVLE, PPCNONE,	{FXM, RS}},
{"mtocrf",	XFXM(31,144,0,1), XFXFXM_MASK, COM|PPCVLE, PPCNONE,	{FXM, RS}},

{"mtmsr",	X(31,146),	XRLARB_MASK, COM|PPCVLE, PPCNONE,	{RS, A_L}},

{"mtsle",	X(31,147),    XRTLRARB_MASK, POWER8,	PPCNONE,	{L}},

{"eratsx",	XRC(31,147,0),	X_MASK,	     PPCA2,	PPCNONE,	{RT, RA0, RB}},
{"eratsx.",	XRC(31,147,1),	X_MASK,	     PPCA2,	PPCNONE,	{RT, RA0, RB}},

{"stdx",	X(31,149),	X_MASK,      PPC64|PPCVLE, PPCNONE,	{RS, RA0, RB}},
 
{"stwcx.",	XRC(31,150,1),	X_MASK,	     PPC|PPCVLE, PPCNONE,	{RS, RA0, RB}},
 
{"stwx",	X(31,151),	X_MASK,      PPCCOM|PPCVLE, PPCNONE,	{RS, RA0, RB}},
{"stx",		X(31,151),	X_MASK,      PWRCOM,	PPCNONE,	{RS, RA, RB}},

{"slq",		XRC(31,152,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"slq.",	XRC(31,152,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"sle",		XRC(31,153,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"sle.",	XRC(31,153,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"prtyw",	X(31,154),	XRB_MASK, POWER6|PPCA2|PPC476, PPCNONE,	{RA, RS}},

{"stdepx",	X(31,157),	X_MASK,	     E500MC|PPCA2|PPCVLE, PPCNONE, {RS, RA0, RB}},
 
{"stwepx",	X(31,159),	X_MASK,	     E500MC|PPCA2|PPCVLE, PPCNONE, {RS, RA0, RB}},
 
{"wrteei",	X(31,163),	XE_MASK,     PPC403|BOOKE|PPCA2|PPC476|PPCVLE, PPCNONE, {E}},
 
{"dcbtls",	X(31,166),	X_MASK,	     PPCCHLK|PPC476|TITAN|PPCVLE, PPCNONE, {CT, RA0, RB}},

{"stvehx",	X(31,167),	X_MASK,      PPCVEC,	PPCNONE,	{VS, RA0, RB}},
{"sthfcmx",	APU(31,167,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"msgclrp",	XRTRA(31,174,0,0), XRTRA_MASK, POWER8,	PPCNONE,	{RB}},
{"dcbtlse",	X(31,174),	X_MASK,      PPCCHLK,	E500MC,		{CT, RA0, RB}},

{"mtmsrd",	X(31,178),	XRLARB_MASK, PPC64,	PPCNONE,	{RS, A_L}},

{"mtvsrd",	X(31,179),	XX1RB_MASK,   PPCVSX2,	PPCNONE,	{XT6, RA}},
{"mtfprd",	X(31,179),	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{FRT, RA}},
{"mtvrd",	X(31,179)|1,	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{VD, RA}},
{"eratre",	X(31,179),	X_MASK,	     PPCA2,	PPCNONE,	{RT, RA, WS}},

{"stdux",	X(31,181),	X_MASK,      PPC64|PPCVLE, PPCNONE,	{RS, RAS, RB}},

{"stqcx.",	XRC(31,182,1),	X_MASK,      POWER8,	PPCNONE,	{RSQ, RA0, RB}},
{"wchkall",	X(31,182),	X_MASK,      PPCA2,	PPCNONE,	{OBF}},

{"stwux",	X(31,183),	X_MASK,      PPCCOM|PPCVLE, PPCNONE,	{RS, RAS, RB}},
{"stux",	X(31,183),	X_MASK,      PWRCOM,	PPCNONE,	{RS, RA0, RB}},

{"sliq",	XRC(31,184,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},
{"sliq.",	XRC(31,184,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},

{"prtyd",	X(31,186),	XRB_MASK, POWER6|PPCA2,	PPCNONE,	{RA, RS}},

{"icblq.",	XRC(31,198,1),	X_MASK,      E6500,	PPCNONE,	{CT, RA0, RB}},

{"stvewx",	X(31,199),	X_MASK,      PPCVEC,	PPCNONE,	{VS, RA0, RB}},
{"stwfcmx",	APU(31,199,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"subfze",	XO(31,200,0,0),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"sfze",	XO(31,200,0,0),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},
{"subfze.",	XO(31,200,0,1),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"sfze.",	XO(31,200,0,1),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},

{"addze",	XO(31,202,0,0),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"aze",		XO(31,202,0,0),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},
{"addze.",	XO(31,202,0,1),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"aze.",	XO(31,202,0,1),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},

{"msgsnd",	XRTRA(31,206,0,0), XRTRA_MASK, E500MC|PPCA2|PPCVLE, PPCNONE, {RB}},

{"mtsr",	X(31,210), XRB_MASK|(1<<20), COM,	NON32,  	{SR, RS}},

{"mtfprwa",	X(31,211),	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{FRT, RA}},
{"mtvrwa",	X(31,211)|1,	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{VD, RA}},
{"mtvsrwa",	X(31,211),	XX1RB_MASK,   PPCVSX2,	PPCNONE,	{XT6, RA}},
{"eratwe",	X(31,211),	X_MASK,	     PPCA2,	PPCNONE,	{RS, RA, WS}},

{"ldawx.",	XRC(31,212,1),	X_MASK,	     PPCA2,	PPCNONE,	{RT, RA0, RB}},

{"stdcx.",	XRC(31,214,1),	X_MASK,      PPC64|PPCVLE, PPCNONE,	{RS, RA0, RB}},
 
{"stbx",	X(31,215),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RS, RA0, RB}},

{"sllq",	XRC(31,216,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"sllq.",	XRC(31,216,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"sleq",	XRC(31,217,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"sleq.",	XRC(31,217,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"stbepx",	X(31,223),	X_MASK,      E500MC|PPCA2|PPCVLE, PPCNONE, {RS, RA0, RB}},
 
{"icblc",	X(31,230),	X_MASK,	PPCCHLK|PPC476|TITAN|PPCVLE, PPCNONE, {CT, RA0, RB}},

{"stvx",	X(31,231),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VS, RA0, RB}},
{"stqfcmx",	APU(31,231,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"subfme",	XO(31,232,0,0),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"sfme",	XO(31,232,0,0),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},
{"subfme.",	XO(31,232,0,1),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"sfme.",	XO(31,232,0,1),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},

{"mulld",	XO(31,233,0,0),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulld.",	XO(31,233,0,1),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
 
{"addme",	XO(31,234,0,0),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"ame",		XO(31,234,0,0),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},
{"addme.",	XO(31,234,0,1),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"ame.",	XO(31,234,0,1),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},

{"mullw",	XO(31,235,0,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"muls",	XO(31,235,0,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"mullw.",	XO(31,235,0,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"muls.",	XO(31,235,0,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"icblce",	X(31,238),	X_MASK,      PPCCHLK,	E500MC|PPCA2,	{CT, RA, RB}},
{"msgclr",	XRTRA(31,238,0,0),XRTRA_MASK, E500MC|PPCA2|PPCVLE, PPCNONE, {RB}},
{"mtsrin",	X(31,242),	XRA_MASK,    PPC,	NON32,  	{RS, RB}},
{"mtsri",	X(31,242),	XRA_MASK,    POWER,	NON32,		{RS, RB}},

{"mtfprwz",	X(31,243),	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{FRT, RA}},
{"mtvrwz",	X(31,243)|1,	XX1RB_MASK|1, PPCVSX2,	PPCNONE,	{VD, RA}},
{"mtvsrwz",	X(31,243),	XX1RB_MASK,   PPCVSX2,	PPCNONE,	{XT6, RA}},

{"dcbtstt",	XRT(31,246,0x10), XRT_MASK,  POWER7,	PPCNONE,	{RA0, RB}},
{"dcbtst",	X(31,246),	X_MASK,      POWER4,	PPCNONE,	{RA0, RB, CT}},
{"dcbtst",	X(31,246),	X_MASK,      PPC|PPCVLE, POWER4,	{CT, RA0, RB}},
 
{"stbux",	X(31,247),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RS, RAS, RB}},

{"slliq",	XRC(31,248,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},
{"slliq.",	XRC(31,248,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},

{"bpermd",	X(31,252),	X_MASK,   POWER7|PPCA2,	PPCNONE,	{RA, RS, RB}},

{"dcbtstep",	XRT(31,255,0),	X_MASK,   E500MC|PPCA2|PPCVLE, PPCNONE,	{RT, RA0, RB}},

{"mfdcrx",	X(31,259),	X_MASK, BOOKE|PPCA2|PPC476|PPCVLE, TITAN, {RS, RA}},
{"mfdcrx.",	XRC(31,259,1),	X_MASK,      PPCA2,	PPCNONE,	{RS, RA}},

{"lvexbx",	X(31,261),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},

{"icbt",	X(31,262),	XRT_MASK,    PPC403,	PPCNONE,	{RA, RB}},

{"lvepxl",	X(31,263),	X_MASK,      PPCVEC2|PPCVLE, PPCNONE,	{VD, RA0, RB}},

{"ldfcmx",	APU(31,263,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},
{"doz",		XO(31,264,0,0),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},
{"doz.",	XO(31,264,0,1),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},

{"add",		XO(31,266,0,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"cax",		XO(31,266,0,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"add.",	XO(31,266,0,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"cax.",	XO(31,266,0,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"ehpriv",	X(31,270),	0xffffffff, E500MC|PPCA2|PPCVLE, PPCNONE, {0}},

{"tlbiel",	X(31,274),	XRTLRA_MASK, POWER4,	PPC476,		{RB, L}},

{"mfapidi",	X(31,275),	X_MASK,      BOOKE,	TITAN,  	{RT, RA}},

{"lqarx",	X(31,276),	XEH_MASK,    POWER8,	PPCNONE,	{RTQ, RAX, RBX, EH}},

{"lscbx",	XRC(31,277,0),	X_MASK,      M601,	PPCNONE,	{RT, RA, RB}},
{"lscbx.",	XRC(31,277,1),	X_MASK,      M601,	PPCNONE,	{RT, RA, RB}},

{"dcbtt",	XRT(31,278,0x10), XRT_MASK,  POWER7,	PPCNONE,	{RA0, RB}},
{"dcbt",	X(31,278),	X_MASK,      POWER4,	PPCNONE,	{RA0, RB, CT}},
{"dcbt",	X(31,278),	X_MASK,      PPC|PPCVLE, POWER4,	{CT, RA0, RB}},
 
{"lhzx",	X(31,279),	X_MASK,      COM|PPCVLE, PPCNONE,	{RT, RA0, RB}},

{"cdtbcd",	X(31,282),	XRB_MASK,    POWER6,	PPCNONE,	{RA, RS}},

{"eqv",		XRC(31,284,0),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"eqv.",	XRC(31,284,1),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RA, RS, RB}},

{"lhepx",	X(31,287),	X_MASK,   E500MC|PPCA2|PPCVLE, PPCNONE,	{RT, RA0, RB}},

{"mfdcrux",	X(31,291),	X_MASK,      PPC464|PPCVLE, PPCNONE,	{RS, RA}},

{"lvexhx",	X(31,293),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},
{"lvepx",	X(31,295),	X_MASK,      PPCVEC2|PPCVLE, PPCNONE,	{VD, RA0, RB}},

{"mfbhrbe",	X(31,302),	X_MASK,      POWER8,	PPCNONE,	{RT, BHRBE}},

{"tlbie",	X(31,306),	XRTLRA_MASK, PPC,	TITAN,  	{RB, L}},
{"tlbi",	X(31,306),	XRT_MASK,    POWER,	PPCNONE,	{RA0, RB}},

{"eciwx",	X(31,310),	X_MASK,      PPC,	TITAN,  	{RT, RA0, RB}},

{"lhzux",	X(31,311),	X_MASK,      COM|PPCVLE, PPCNONE,	{RT, RAL, RB}},

{"cbcdtd",	X(31,314),	XRB_MASK,    POWER6,	PPCNONE,	{RA, RS}},

{"xor",		XRC(31,316,0),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"xor.",	XRC(31,316,1),	X_MASK,	     COM|PPCVLE, PPCNONE,	{RA, RS, RB}},

{"dcbtep",	XRT(31,319,0),	X_MASK,   E500MC|PPCA2|PPCVLE, PPCNONE,	{RT, RA0, RB}},

{"mfexisr",	XSPR(31,323, 64), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfexier",	XSPR(31,323, 66), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbr0",	XSPR(31,323,128), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbr1",	XSPR(31,323,129), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbr2",	XSPR(31,323,130), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbr3",	XSPR(31,323,131), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbr4",	XSPR(31,323,132), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbr5",	XSPR(31,323,133), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbr6",	XSPR(31,323,134), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbr7",	XSPR(31,323,135), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbear",	XSPR(31,323,144), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfbesr",	XSPR(31,323,145), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfiocr",	XSPR(31,323,160), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmacr0",	XSPR(31,323,192), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmact0",	XSPR(31,323,193), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmada0",	XSPR(31,323,194), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmasa0",	XSPR(31,323,195), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmacc0",	XSPR(31,323,196), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmacr1",	XSPR(31,323,200), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmact1",	XSPR(31,323,201), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmada1",	XSPR(31,323,202), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmasa1",	XSPR(31,323,203), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmacc1",	XSPR(31,323,204), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmacr2",	XSPR(31,323,208), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmact2",	XSPR(31,323,209), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmada2",	XSPR(31,323,210), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmasa2",	XSPR(31,323,211), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmacc2",	XSPR(31,323,212), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmacr3",	XSPR(31,323,216), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmact3",	XSPR(31,323,217), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmada3",	XSPR(31,323,218), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmasa3",	XSPR(31,323,219), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmacc3",	XSPR(31,323,220), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdmasr",	XSPR(31,323,224), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdcr",	X(31,323), X_MASK, PPC403|BOOKE|PPCA2|PPC476|PPCVLE, TITAN, {RT, SPR}},
{"mfdcr.",	XRC(31,323,1),	X_MASK,      PPCA2,	PPCNONE,	{RT, SPR}},

{"lvexwx",	X(31,325),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},

{"dcread",	X(31,326),	X_MASK,  PPC476|TITAN,	PPCNONE,	{RT, RA0, RB}},

{"div",		XO(31,331,0,0),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},
{"div.",	XO(31,331,0,1),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},

{"lxvdsx",	X(31,332),	XX1_MASK,    PPCVSX,	PPCNONE,	{XT6, RA0, RB}},

{"mfpmr",	X(31,334),	X_MASK, PPCPMR|PPCE300|PPCVLE, PPCNONE,	{RT, PMR}},
{"mftmr",	X(31,366),	X_MASK,	PPCTMR|E6500,	PPCNONE,	{RT, TMR}},

{"mfmq",	XSPR(31,339,  0), XSPR_MASK, M601,	PPCNONE,	{RT}},
{"mfxer",	XSPR(31,339,  1), XSPR_MASK, COM|PPCVLE, PPCNONE,	{RT}},
{"mfrtcu",	XSPR(31,339,  4), XSPR_MASK, COM,	TITAN,  	{RT}},
{"mfrtcl",	XSPR(31,339,  5), XSPR_MASK, COM,	TITAN,  	{RT}},
{"mfdec",	XSPR(31,339,  6), XSPR_MASK, MFDEC1,	PPCNONE,	{RT}},
{"mflr",	XSPR(31,339,  8), XSPR_MASK, COM|PPCVLE, PPCNONE,	{RT}},
{"mfctr",	XSPR(31,339,  9), XSPR_MASK, COM|PPCVLE, PPCNONE,	{RT}},
{"mftid",	XSPR(31,339, 17), XSPR_MASK, POWER,	PPCNONE,	{RT}},
{"mfdsisr",	XSPR(31,339, 18), XSPR_MASK, COM,	TITAN,  	{RT}},
{"mfdar",	XSPR(31,339, 19), XSPR_MASK, COM,	TITAN,  	{RT}},
{"mfdec",	XSPR(31,339, 22), XSPR_MASK, MFDEC2,	MFDEC1,		{RT}},
{"mfsdr0",	XSPR(31,339, 24), XSPR_MASK, POWER,	PPCNONE,	{RT}},
{"mfsdr1",	XSPR(31,339, 25), XSPR_MASK, COM,	TITAN,  	{RT}},
{"mfsrr0",	XSPR(31,339, 26), XSPR_MASK, COM,	PPCNONE,	{RT}},
{"mfsrr1",	XSPR(31,339, 27), XSPR_MASK, COM,	PPCNONE,	{RT}},
{"mfcfar",	XSPR(31,339, 28), XSPR_MASK, POWER6,	PPCNONE,	{RT}},
{"mfpid",	XSPR(31,339, 48), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfcsrr0",	XSPR(31,339, 58), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfcsrr1",	XSPR(31,339, 59), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfdear",	XSPR(31,339, 61), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfesr",	XSPR(31,339, 62), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivpr",	XSPR(31,339, 63), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfcmpa",	XSPR(31,339,144), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcmpb",	XSPR(31,339,145), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcmpc",	XSPR(31,339,146), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcmpd",	XSPR(31,339,147), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mficr",	XSPR(31,339,148), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfder",	XSPR(31,339,149), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcounta",	XSPR(31,339,150), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcountb",	XSPR(31,339,151), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcmpe",	XSPR(31,339,152), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcmpf",	XSPR(31,339,153), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcmpg",	XSPR(31,339,154), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfcmph",	XSPR(31,339,155), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mflctrl1",	XSPR(31,339,156), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mflctrl2",	XSPR(31,339,157), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfictrl",	XSPR(31,339,158), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfbar",	XSPR(31,339,159), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfvrsave",	XSPR(31,339,256), XSPR_MASK, PPCVEC,	PPCNONE,	{RT}},
{"mfusprg0",	XSPR(31,339,256), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfsprg",	XSPR(31,339,256), XSPRG_MASK, PPC|PPCVLE, PPCNONE,	{RT, SPRG}},
{"mfsprg4",	XSPR(31,339,260), XSPR_MASK, PPC405|BOOKE|PPCVLE, PPCNONE, {RT}},
{"mfsprg5",	XSPR(31,339,261), XSPR_MASK, PPC405|BOOKE|PPCVLE, PPCNONE, {RT}},
{"mfsprg6",	XSPR(31,339,262), XSPR_MASK, PPC405|BOOKE|PPCVLE, PPCNONE, {RT}},
{"mfsprg7",	XSPR(31,339,263), XSPR_MASK, PPC405|BOOKE|PPCVLE, PPCNONE, {RT}},
{"mftb",	XSPR(31,339,268), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mftbl",	XSPR(31,339,268), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mftbu",	XSPR(31,339,269), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfsprg0",	XSPR(31,339,272), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RT}},
{"mfsprg1",	XSPR(31,339,273), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RT}},
{"mfsprg2",	XSPR(31,339,274), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RT}},
{"mfsprg3",	XSPR(31,339,275), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RT}},
{"mfasr",	XSPR(31,339,280), XSPR_MASK, PPC64,	PPCNONE,	{RT}},
{"mfear",	XSPR(31,339,282), XSPR_MASK, PPC,	TITAN,  	{RT}},
{"mfpir",	XSPR(31,339,286), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfpvr",	XSPR(31,339,287), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RT}},
{"mfdbsr",	XSPR(31,339,304), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfdbcr0",	XSPR(31,339,308), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfdbcr1",	XSPR(31,339,309), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfdbcr2",	XSPR(31,339,310), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfiac1",	XSPR(31,339,312), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfiac2",	XSPR(31,339,313), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfiac3",	XSPR(31,339,314), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfiac4",	XSPR(31,339,315), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfdac1",	XSPR(31,339,316), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfdac2",	XSPR(31,339,317), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfdvc1",	XSPR(31,339,318), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfdvc2",	XSPR(31,339,319), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mftsr",	XSPR(31,339,336), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mftcr",	XSPR(31,339,340), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor0",	XSPR(31,339,400), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor1",	XSPR(31,339,401), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor2",	XSPR(31,339,402), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor3",	XSPR(31,339,403), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor4",	XSPR(31,339,404), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor5",	XSPR(31,339,405), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor6",	XSPR(31,339,406), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor7",	XSPR(31,339,407), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor8",	XSPR(31,339,408), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor9",	XSPR(31,339,409), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor10",	XSPR(31,339,410), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor11",	XSPR(31,339,411), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor12",	XSPR(31,339,412), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor13",	XSPR(31,339,413), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor14",	XSPR(31,339,414), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfivor15",	XSPR(31,339,415), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RT}},
{"mfspefscr",	XSPR(31,339,512), XSPR_MASK, PPCSPE,	PPCNONE,	{RT}},
{"mfbbear",	XSPR(31,339,513), XSPR_MASK, PPCBRLK,	PPCNONE,	{RT}},
{"mfbbtar",	XSPR(31,339,514), XSPR_MASK, PPCBRLK,	PPCNONE,	{RT}},
{"mfivor32",	XSPR(31,339,528), XSPR_MASK, PPCSPE,	PPCNONE,	{RT}},
{"mfibatu",	XSPR(31,339,528), XSPRBAT_MASK, PPC,	TITAN,  	{RT, SPRBAT}},
{"mfivor33",	XSPR(31,339,529), XSPR_MASK, PPCSPE,	PPCNONE,	{RT}},
{"mfibatl",	XSPR(31,339,529), XSPRBAT_MASK, PPC,	TITAN,  	{RT, SPRBAT}},
{"mfivor34",	XSPR(31,339,530), XSPR_MASK, PPCSPE,	PPCNONE,	{RT}},
{"mfivor35",	XSPR(31,339,531), XSPR_MASK, PPCPMR,	PPCNONE,	{RT}},
{"mfdbatu",	XSPR(31,339,536), XSPRBAT_MASK, PPC,	TITAN,  	{RT, SPRBAT}},
{"mfdbatl",	XSPR(31,339,537), XSPRBAT_MASK, PPC,	TITAN,  	{RT, SPRBAT}},
{"mfic_cst",	XSPR(31,339,560), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfic_adr",	XSPR(31,339,561), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfic_dat",	XSPR(31,339,562), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfdc_cst",	XSPR(31,339,568), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfdc_adr",	XSPR(31,339,569), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfdc_dat",	XSPR(31,339,570), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmcsrr0",	XSPR(31,339,570), XSPR_MASK, PPCRFMCI,	PPCNONE,	{RT}},
{"mfmcsrr1",	XSPR(31,339,571), XSPR_MASK, PPCRFMCI,	PPCNONE,	{RT}},
{"mfmcsr",	XSPR(31,339,572), XSPR_MASK, PPCRFMCI,	PPCNONE,	{RT}},
{"mfmcar",	XSPR(31,339,573), XSPR_MASK, PPCRFMCI,	TITAN,  	{RT}},
{"mfdpdr",	XSPR(31,339,630), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfdpir",	XSPR(31,339,631), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfimmr",	XSPR(31,339,638), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmi_ctr",	XSPR(31,339,784), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmi_ap",	XSPR(31,339,786), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmi_epn",	XSPR(31,339,787), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmi_twc",	XSPR(31,339,789), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmi_rpn",	XSPR(31,339,790), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_ctr",	XSPR(31,339,792), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfm_casid",	XSPR(31,339,793), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_ap",	XSPR(31,339,794), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_epn",	XSPR(31,339,795), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_twb",	XSPR(31,339,796), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_twc",	XSPR(31,339,797), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_rpn",	XSPR(31,339,798), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfm_tw",	XSPR(31,339,799), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmi_dbcam",	XSPR(31,339,816), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmi_dbram0",	XSPR(31,339,817), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmi_dbram1",	XSPR(31,339,818), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_dbcam",	XSPR(31,339,824), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_dbram0",	XSPR(31,339,825), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfmd_dbram1",	XSPR(31,339,826), XSPR_MASK, PPC860,	PPCNONE,	{RT}},
{"mfivndx",	XSPR(31,339,880), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfdvndx",	XSPR(31,339,881), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfivlim",	XSPR(31,339,882), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfdvlim",	XSPR(31,339,883), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfclcsr",	XSPR(31,339,884), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfccr1",	XSPR(31,339,888), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfppr",	XSPR(31,339,896), XSPR_MASK, POWER7,	PPCNONE,	{RT}},
{"mfppr32",	XSPR(31,339,898), XSPR_MASK, POWER7,	PPCNONE,	{RT}},
{"mfrstcfg",	XSPR(31,339,923), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfdcdbtrl",	XSPR(31,339,924), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfdcdbtrh",	XSPR(31,339,925), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mficdbtr",	XSPR(31,339,927), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfummcr0",	XSPR(31,339,936), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfupmc1",	XSPR(31,339,937), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfupmc2",	XSPR(31,339,938), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfusia",	XSPR(31,339,939), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfummcr1",	XSPR(31,339,940), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfupmc3",	XSPR(31,339,941), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfupmc4",	XSPR(31,339,942), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfzpr",	XSPR(31,339,944), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfpid",	XSPR(31,339,945), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfmmucr",	XSPR(31,339,946), XSPR_MASK, TITAN,	PPCNONE,	{RT}},
{"mfccr0",	XSPR(31,339,947), XSPR_MASK, PPC405|TITAN, PPCNONE,	{RT}},
{"mfiac3",	XSPR(31,339,948), XSPR_MASK, PPC405,	PPCNONE,	{RT}},
{"mfiac4",	XSPR(31,339,949), XSPR_MASK, PPC405,	PPCNONE,	{RT}},
{"mfdvc1",	XSPR(31,339,950), XSPR_MASK, PPC405,	PPCNONE,	{RT}},
{"mfdvc2",	XSPR(31,339,951), XSPR_MASK, PPC405,	PPCNONE,	{RT}},
{"mfmmcr0",	XSPR(31,339,952), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfpmc1",	XSPR(31,339,953), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfsgr",	XSPR(31,339,953), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdcwr",	XSPR(31,339,954), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfpmc2",	XSPR(31,339,954), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfsia",	XSPR(31,339,955), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfsler",	XSPR(31,339,955), XSPR_MASK, PPC405,	PPCNONE,	{RT}},
{"mfmmcr1",	XSPR(31,339,956), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfsu0r",	XSPR(31,339,956), XSPR_MASK, PPC405,	PPCNONE,	{RT}},
{"mfdbcr1",	XSPR(31,339,957), XSPR_MASK, PPC405,	PPCNONE,	{RT}},
{"mfpmc3",	XSPR(31,339,957), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfpmc4",	XSPR(31,339,958), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mficdbdr",	XSPR(31,339,979), XSPR_MASK, PPC403|TITAN, PPCNONE,	{RT}},
{"mfesr",	XSPR(31,339,980), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdear",	XSPR(31,339,981), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfevpr",	XSPR(31,339,982), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfcdbcr",	XSPR(31,339,983), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mftsr",	XSPR(31,339,984), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mftcr",	XSPR(31,339,986), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfpit",	XSPR(31,339,987), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mftbhi",	XSPR(31,339,988), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mftblo",	XSPR(31,339,989), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfsrr2",	XSPR(31,339,990), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfsrr3",	XSPR(31,339,991), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdbsr",	XSPR(31,339,1008), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdbcr0",	XSPR(31,339,1010), XSPR_MASK, PPC405,	PPCNONE,	{RT}},
{"mfdbdr",	XSPR(31,339,1011), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mfiac1",	XSPR(31,339,1012), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfiac2",	XSPR(31,339,1013), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdac1",	XSPR(31,339,1014), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfdac2",	XSPR(31,339,1015), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfl2cr",	XSPR(31,339,1017), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfdccr",	XSPR(31,339,1018), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mficcr",	XSPR(31,339,1019), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfictc",	XSPR(31,339,1019), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfpbl1",	XSPR(31,339,1020), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfthrm1",	XSPR(31,339,1020), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfpbu1",	XSPR(31,339,1021), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfthrm2",	XSPR(31,339,1021), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfpbl2",	XSPR(31,339,1022), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfthrm3",	XSPR(31,339,1022), XSPR_MASK, PPC750,	PPCNONE,	{RT}},
{"mfpbu2",	XSPR(31,339,1023), XSPR_MASK, PPC403,	PPCNONE,	{RT}},
{"mfspr",	X(31,339),	X_MASK,      COM|PPCVLE, PPCNONE,	{RT, SPR}},
 
{"lwax",	X(31,341),	X_MASK,      PPC64|PPCVLE, PPCNONE,	{RT, RA0, RB}},

{"dst",		XDSS(31,342,0),	XDSS_MASK,   PPCVEC,	PPCNONE,	{RA, RB, STRM}},

{"lhax",	X(31,343),	X_MASK,      COM|PPCVLE, PPCNONE,	{RT, RA0, RB}},

{"lvxl",	X(31,359),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VD, RA0, RB}},

{"abs",		XO(31,360,0,0),	XORB_MASK,   M601,	PPCNONE,	{RT, RA}},
{"abs.",	XO(31,360,0,1),	XORB_MASK,   M601,	PPCNONE,	{RT, RA}},

{"divs",	XO(31,363,0,0),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},
{"divs.",	XO(31,363,0,1),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},

{"tlbia",	X(31,370),	0xffffffff,  PPC,	TITAN,  	{0}},

{"mftbl",	XSPR(31,371,268), XSPR_MASK, PPC,	NO371,		{RT}},
{"mftbu",	XSPR(31,371,269), XSPR_MASK, PPC,	NO371,		{RT}},
{"mftb",	X(31,371),	X_MASK,      PPC|PPCA2,	NO371|POWER7,	{RT, TBR}},

{"lwaux",	X(31,373),	X_MASK,      PPC64|PPCVLE, PPCNONE,	{RT, RAL, RB}},

{"dstst",	XDSS(31,374,0),	XDSS_MASK,   PPCVEC,	PPCNONE,	{RA, RB, STRM}},

{"lhaux",	X(31,375),	X_MASK,      COM|PPCVLE, PPCNONE,	{RT, RAL, RB}},

{"popcntw",	X(31,378),	XRB_MASK, POWER7|PPCA2,	PPCNONE,	{RA, RS}},

{"mtdcrx",	X(31,387),	X_MASK, BOOKE|PPCA2|PPC476|PPCVLE, TITAN, {RA, RS}},
{"mtdcrx.",	XRC(31,387,1),	X_MASK,	     PPCA2,	PPCNONE,	{RA, RS}},

{"stvexbx",	X(31,389),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"dcblc",	X(31,390),	X_MASK, PPCCHLK|PPC476|TITAN|PPCVLE, PPCNONE, {CT, RA0, RB}},
{"stdfcmx",	APU(31,391,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"divdeu",	XO(31,393,0,0),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divdeu.",	XO(31,393,0,1),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divweu",	XO(31,395,0,0),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divweu.",	XO(31,395,0,1),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},

{"dcblce",	X(31,398),	X_MASK,      PPCCHLK,	E500MC,		{CT, RA, RB}},

{"slbmte",	X(31,402),	XRA_MASK,    PPC64,	PPCNONE,	{RS, RB}},

{"pbt.",	XRC(31,404,1),	X_MASK,      POWER8,	PPCNONE,	{RS, RA0, RB}},

{"icswx",	XRC(31,406,0),	X_MASK,   POWER7|PPCA2,	PPCNONE,	{RS, RA, RB}},
{"icswx.",	XRC(31,406,1),	X_MASK,   POWER7|PPCA2,	PPCNONE,	{RS, RA, RB}},

{"sthx",	X(31,407),	X_MASK,      COM|PPCVLE, PPCNONE,	{RS, RA0, RB}},

{"orc",		XRC(31,412,0),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"orc.",	XRC(31,412,1),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},

{"sthepx",	X(31,415),	X_MASK,      E500MC|PPCA2|PPCVLE, PPCNONE, {RS, RA0, RB}},

{"mtdcrux",	X(31,419),	X_MASK,      PPC464|PPCVLE, PPCNONE,	{RA, RS}},

{"stvexhx",	X(31,421),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"dcblq.",	XRC(31,422,1),	X_MASK,      E6500,	PPCNONE,	{CT, RA0, RB}},

{"divde",	XO(31,425,0,0),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divde.",	XO(31,425,0,1),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divwe",	XO(31,427,0,0),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divwe.",	XO(31,427,0,1),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},

{"clrbhrb",	X(31,430),	0xffffffff,  POWER8,	PPCNONE,	{0}},

{"slbie",	X(31,434),	XRTRA_MASK,  PPC64,	PPCNONE,	{RB}},

{"ecowx",	X(31,438),	X_MASK,      PPC,	TITAN,  	{RT, RA0, RB}},

{"sthux",	X(31,439),	X_MASK,      COM|PPCVLE, PPCNONE,	{RS, RAS, RB}},

{"mdors",	0x7f9ce378,	0xffffffff,  E500MC,	PPCNONE,	{0}},

{"miso",	0x7f5ad378,	0xffffffff,  E6500,	PPCNONE,	{0}},

/* The "yield", "mdoio" and "mdoom" instructions are extended mnemonics for
   "or rX,rX,rX", with rX being r27, r29 and r30 respectively.  */
{"yield",	0x7f7bdb78,	0xffffffff,  POWER7,	 PPCNONE,	{0}},
{"mdoio",	0x7fbdeb78,	0xffffffff,  POWER7,	 PPCNONE,	{0}},
{"mdoom",	0x7fdef378,	0xffffffff,  POWER7,	 PPCNONE,	{0}},
{"mr",		XRC(31,444,0),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RBS}},
{"or",		XRC(31,444,0),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"mr.",		XRC(31,444,1),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RBS}},
{"or.",		XRC(31,444,1),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},

{"mtexisr",	XSPR(31,451, 64), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtexier",	XSPR(31,451, 66), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbr0",	XSPR(31,451,128), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbr1",	XSPR(31,451,129), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbr2",	XSPR(31,451,130), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbr3",	XSPR(31,451,131), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbr4",	XSPR(31,451,132), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbr5",	XSPR(31,451,133), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbr6",	XSPR(31,451,134), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbr7",	XSPR(31,451,135), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbear",	XSPR(31,451,144), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtbesr",	XSPR(31,451,145), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtiocr",	XSPR(31,451,160), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmacr0",	XSPR(31,451,192), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmact0",	XSPR(31,451,193), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmada0",	XSPR(31,451,194), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmasa0",	XSPR(31,451,195), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmacc0",	XSPR(31,451,196), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmacr1",	XSPR(31,451,200), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmact1",	XSPR(31,451,201), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmada1",	XSPR(31,451,202), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmasa1",	XSPR(31,451,203), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmacc1",	XSPR(31,451,204), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmacr2",	XSPR(31,451,208), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmact2",	XSPR(31,451,209), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmada2",	XSPR(31,451,210), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmasa2",	XSPR(31,451,211), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmacc2",	XSPR(31,451,212), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmacr3",	XSPR(31,451,216), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmact3",	XSPR(31,451,217), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmada3",	XSPR(31,451,218), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmasa3",	XSPR(31,451,219), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmacc3",	XSPR(31,451,220), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdmasr",	XSPR(31,451,224), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdcr",	X(31,451), X_MASK, PPC403|BOOKE|PPCA2|PPC476|PPCVLE, TITAN, {SPR, RS}},
{"mtdcr.",	XRC(31,451,1), X_MASK,       PPCA2,	PPCNONE,	{SPR, RS}},

{"stvexwx",	X(31,453),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"dccci",	X(31,454), XRT_MASK, PPC403|PPC440|TITAN|PPCA2, PPCNONE, {RAOPT, RBOPT}},
{"dci",		X(31,454),	XRARB_MASK, PPCA2|PPC476|PPCVLE, PPCNONE, {CT}},
 
{"divdu",	XO(31,457,0,0),	XO_MASK,  PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"divdu.",	XO(31,457,0,1),	XO_MASK,  PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"divwu",	XO(31,459,0,0),	XO_MASK,  PPC|PPCVLE,	PPCNONE,	{RT, RA, RB}},
{"divwu.",	XO(31,459,0,1),	XO_MASK,  PPC|PPCVLE,	PPCNONE,	{RT, RA, RB}},

{"mtpmr",	X(31,462),	X_MASK, PPCPMR|PPCE300|PPCVLE, PPCNONE,	{PMR, RS}},
{"mttmr",	X(31,494),	X_MASK,	PPCTMR|E6500,	PPCNONE,	{TMR, RS}},

{"mtmq",	XSPR(31,467,  0), XSPR_MASK, M601,	PPCNONE,	{RS}},
{"mtxer",	XSPR(31,467,  1), XSPR_MASK, COM|PPCVLE, PPCNONE,	{RS}},
{"mtlr",	XSPR(31,467,  8), XSPR_MASK, COM|PPCVLE, PPCNONE,	{RS}},
{"mtctr", 	XSPR(31,467,  9), XSPR_MASK, COM|PPCVLE, PPCNONE,	{RS}},
{"mttid",	XSPR(31,467, 17), XSPR_MASK, POWER,	PPCNONE,	{RS}},
{"mtdsisr",	XSPR(31,467, 18), XSPR_MASK, COM,	TITAN,  	{RS}},
{"mtdar",	XSPR(31,467, 19), XSPR_MASK, COM,	TITAN,  	{RS}},
{"mtrtcu",	XSPR(31,467, 20), XSPR_MASK, COM,	TITAN,  	{RS}},
{"mtrtcl",	XSPR(31,467, 21), XSPR_MASK, COM,	TITAN,  	{RS}},
{"mtdec",	XSPR(31,467, 22), XSPR_MASK, COM,	PPCNONE,	{RS}},
{"mtsdr0",	XSPR(31,467, 24), XSPR_MASK, POWER,	PPCNONE,	{RS}},
{"mtsdr1",	XSPR(31,467, 25), XSPR_MASK, COM,	TITAN,  	{RS}},
{"mtsrr0",	XSPR(31,467, 26), XSPR_MASK, COM|PPCVLE, PPCNONE,	{RS}},
{"mtsrr1",	XSPR(31,467, 27), XSPR_MASK, COM|PPCVLE, PPCNONE,	{RS}},
{"mtcfar",	XSPR(31,467, 28), XSPR_MASK, POWER6,	PPCNONE,	{RS}},
{"mtpid",	XSPR(31,467, 48), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdecar",	XSPR(31,467, 54), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtcsrr0",	XSPR(31,467, 58), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtcsrr1",	XSPR(31,467, 59), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdear",	XSPR(31,467, 61), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtesr",	XSPR(31,467, 62), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivpr",	XSPR(31,467, 63), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtcmpa",	XSPR(31,467,144), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcmpb",	XSPR(31,467,145), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcmpc",	XSPR(31,467,146), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcmpd",	XSPR(31,467,147), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mticr",	XSPR(31,467,148), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtder",	XSPR(31,467,149), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcounta",	XSPR(31,467,150), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcountb",	XSPR(31,467,151), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcmpe",	XSPR(31,467,152), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcmpf",	XSPR(31,467,153), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcmpg",	XSPR(31,467,154), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtcmph",	XSPR(31,467,155), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtlctrl1",	XSPR(31,467,156), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtlctrl2",	XSPR(31,467,157), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtictrl",	XSPR(31,467,158), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtbar",	XSPR(31,467,159), XSPR_MASK, PPC860,	PPCNONE,	{RS}},
{"mtvrsave",	XSPR(31,467,256), XSPR_MASK, PPCVEC,	PPCNONE,	{RS}},
{"mtusprg0",	XSPR(31,467,256), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtsprg",	XSPR(31,467,256), XSPRG_MASK, PPC|PPCVLE, PPCNONE,	{SPRG, RS}},
{"mtsprg0",	XSPR(31,467,272), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RS}},
{"mtsprg1",	XSPR(31,467,273), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RS}},
{"mtsprg2",	XSPR(31,467,274), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RS}},
{"mtsprg3",	XSPR(31,467,275), XSPR_MASK, PPC|PPCVLE, PPCNONE,	{RS}},
{"mtsprg4",	XSPR(31,467,276), XSPR_MASK, PPC405|BOOKE|PPCVLE, PPCNONE, {RS}},
{"mtsprg5",	XSPR(31,467,277), XSPR_MASK, PPC405|BOOKE|PPCVLE, PPCNONE, {RS}},
{"mtsprg6",	XSPR(31,467,278), XSPR_MASK, PPC405|BOOKE|PPCVLE, PPCNONE, {RS}},
{"mtsprg7",	XSPR(31,467,279), XSPR_MASK, PPC405|BOOKE|PPCVLE, PPCNONE, {RS}},
{"mtasr",	XSPR(31,467,280), XSPR_MASK, PPC64,	PPCNONE,	{RS}},
{"mtear",	XSPR(31,467,282), XSPR_MASK, PPC,	TITAN,  	{RS}},
{"mttbl",	XSPR(31,467,284), XSPR_MASK, PPC,	PPCNONE,	{RS}},
{"mttbu",	XSPR(31,467,285), XSPR_MASK, PPC,	PPCNONE,	{RS}},
{"mtdbsr",	XSPR(31,467,304), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdbcr0",	XSPR(31,467,308), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdbcr1",	XSPR(31,467,309), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdbcr2",	XSPR(31,467,310), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtiac1",	XSPR(31,467,312), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtiac2",	XSPR(31,467,313), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtiac3",	XSPR(31,467,314), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtiac4",	XSPR(31,467,315), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdac1",	XSPR(31,467,316), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdac2",	XSPR(31,467,317), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdvc1",	XSPR(31,467,318), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtdvc2",	XSPR(31,467,319), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mttsr",	XSPR(31,467,336), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mttcr",	XSPR(31,467,340), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor0",	XSPR(31,467,400), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor1",	XSPR(31,467,401), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor2",	XSPR(31,467,402), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor3",	XSPR(31,467,403), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor4",	XSPR(31,467,404), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor5",	XSPR(31,467,405), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor6",	XSPR(31,467,406), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor7",	XSPR(31,467,407), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor8",	XSPR(31,467,408), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor9",	XSPR(31,467,409), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor10",	XSPR(31,467,410), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor11",	XSPR(31,467,411), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor12",	XSPR(31,467,412), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor13",	XSPR(31,467,413), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor14",	XSPR(31,467,414), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtivor15",	XSPR(31,467,415), XSPR_MASK, BOOKE|PPCVLE, PPCNONE,	{RS}},
{"mtspefscr",	XSPR(31,467,512), XSPR_MASK, PPCSPE,	PPCNONE,	{RS}},
{"mtbbear",	XSPR(31,467,513), XSPR_MASK, PPCBRLK,	PPCNONE,	{RS}},
{"mtbbtar",	XSPR(31,467,514), XSPR_MASK, PPCBRLK,	PPCNONE,	{RS}},
{"mtivor32",	XSPR(31,467,528), XSPR_MASK, PPCSPE,	PPCNONE,	{RS}},
{"mtibatu",	XSPR(31,467,528), XSPRBAT_MASK, PPC,	TITAN,  	{SPRBAT, RS}},
{"mtivor33",	XSPR(31,467,529), XSPR_MASK, PPCSPE,	PPCNONE,	{RS}},
{"mtibatl",	XSPR(31,467,529), XSPRBAT_MASK, PPC,	TITAN,  	{SPRBAT, RS}},
{"mtivor34",	XSPR(31,467,530), XSPR_MASK, PPCSPE,	PPCNONE,	{RS}},
{"mtivor35",	XSPR(31,467,531), XSPR_MASK, PPCPMR,	PPCNONE,	{RS}},
{"mtdbatu",	XSPR(31,467,536), XSPRBAT_MASK, PPC,	TITAN,  	{SPRBAT, RS}},
{"mtdbatl",	XSPR(31,467,537), XSPRBAT_MASK, PPC,	TITAN,  	{SPRBAT, RS}},
{"mtmcsrr0",	XSPR(31,467,570), XSPR_MASK, PPCRFMCI|PPCVLE, PPCNONE,	{RS}},
{"mtmcsrr1",	XSPR(31,467,571), XSPR_MASK, PPCRFMCI|PPCVLE, PPCNONE,	{RS}},
{"mtmcsr",	XSPR(31,467,572), XSPR_MASK, PPCRFMCI,	PPCNONE,	{RS}},
{"mtivndx",	XSPR(31,467,880), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mtdvndx",	XSPR(31,467,881), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mtivlim",	XSPR(31,467,882), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mtdvlim",	XSPR(31,467,883), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mtclcsr",	XSPR(31,467,884), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mtccr1",	XSPR(31,467,888), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mtppr",	XSPR(31,467,896), XSPR_MASK, POWER7,	PPCNONE,	{RS}},
{"mtppr32",	XSPR(31,467,898), XSPR_MASK, POWER7,	PPCNONE,	{RS}},
{"mtummcr0",	XSPR(31,467,936), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtupmc1",	XSPR(31,467,937), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtupmc2",	XSPR(31,467,938), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtusia",	XSPR(31,467,939), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtummcr1",	XSPR(31,467,940), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtupmc3",	XSPR(31,467,941), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtupmc4",	XSPR(31,467,942), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtzpr",	XSPR(31,467,944), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtpid",	XSPR(31,467,945), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtrmmucr",	XSPR(31,467,946), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mtccr0",	XSPR(31,467,947), XSPR_MASK, PPC405|TITAN, PPCNONE,	{RS}},
{"mtiac3",	XSPR(31,467,948), XSPR_MASK, PPC405,	PPCNONE,	{RS}},
{"mtiac4",	XSPR(31,467,949), XSPR_MASK, PPC405,	PPCNONE,	{RS}},
{"mtdvc1",	XSPR(31,467,950), XSPR_MASK, PPC405,	PPCNONE,	{RS}},
{"mtdvc2",	XSPR(31,467,951), XSPR_MASK, PPC405,	PPCNONE,	{RS}},
{"mtmmcr0",	XSPR(31,467,952), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtpmc1",	XSPR(31,467,953), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtsgr",	XSPR(31,467,953), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdcwr",	XSPR(31,467,954), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtpmc2",	XSPR(31,467,954), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtsia",	XSPR(31,467,955), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtsler",	XSPR(31,467,955), XSPR_MASK, PPC405,	PPCNONE,	{RS}},
{"mtmmcr1",	XSPR(31,467,956), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtsu0r",	XSPR(31,467,956), XSPR_MASK, PPC405,	PPCNONE,	{RS}},
{"mtdbcr1",	XSPR(31,467,957), XSPR_MASK, PPC405,	PPCNONE,	{RS}},
{"mtpmc3",	XSPR(31,467,957), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtpmc4",	XSPR(31,467,958), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mticdbdr",	XSPR(31,467,979), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtesr",	XSPR(31,467,980), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdear",	XSPR(31,467,981), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtevpr",	XSPR(31,467,982), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtcdbcr",	XSPR(31,467,983), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mttsr",	XSPR(31,467,984), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mttcr",	XSPR(31,467,986), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtpit",	XSPR(31,467,987), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mttbhi",	XSPR(31,467,988), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mttblo",	XSPR(31,467,989), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtsrr2",	XSPR(31,467,990), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtsrr3",	XSPR(31,467,991), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdbsr",	XSPR(31,467,1008), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdbdr",	XSPR(31,467,1011), XSPR_MASK, TITAN,	PPCNONE,	{RS}},
{"mtdbcr0",	XSPR(31,467,1010), XSPR_MASK, PPC405,	PPCNONE,	{RS}},
{"mtiac1",	XSPR(31,467,1012), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtiac2",	XSPR(31,467,1013), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdac1",	XSPR(31,467,1014), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtdac2",	XSPR(31,467,1015), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtl2cr",	XSPR(31,467,1017), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtdccr",	XSPR(31,467,1018), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mticcr",	XSPR(31,467,1019), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtictc",	XSPR(31,467,1019), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtpbl1",	XSPR(31,467,1020), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtthrm1",	XSPR(31,467,1020), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtpbu1",	XSPR(31,467,1021), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtthrm2",	XSPR(31,467,1021), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtpbl2",	XSPR(31,467,1022), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtthrm3",	XSPR(31,467,1022), XSPR_MASK, PPC750,	PPCNONE,	{RS}},
{"mtpbu2",	XSPR(31,467,1023), XSPR_MASK, PPC403,	PPCNONE,	{RS}},
{"mtspr",	X(31,467),	X_MASK,      COM|PPCVLE, PPCNONE,	{SPR, RS}},
 
{"dcbi",	X(31,470),	XRT_MASK,    PPC|PPCVLE, PPCNONE,	{RA0, RB}},

{"nand",	XRC(31,476,0),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"nand.",	XRC(31,476,1),	X_MASK,      COM|PPCVLE, PPCNONE,	{RA, RS, RB}},

{"dsn", 	X(31,483),	XRT_MASK,    E500MC|PPCVLE, PPCNONE,	{RA, RB}},

{"dcread",	X(31,486),	X_MASK,  PPC403|PPC440|PPCVLE, PPCA2|PPC476, {RT, RA0, RB}},

{"icbtls",	X(31,486),	X_MASK, PPCCHLK|PPC476|TITAN|PPCVLE, PPCNONE, {CT, RA0, RB}},

{"stvxl",	X(31,487),	X_MASK,      PPCVEC|PPCVLE, PPCNONE,	{VS, RA0, RB}},

{"nabs",	XO(31,488,0,0),	XORB_MASK,   M601,	PPCNONE,	{RT, RA}},
{"nabs.",	XO(31,488,0,1),	XORB_MASK,   M601,	PPCNONE,	{RT, RA}},

{"divd",	XO(31,489,0,0),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"divd.",	XO(31,489,0,1),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"divw",	XO(31,491,0,0),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"divw.",	XO(31,491,0,1),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"icbtlse",	X(31,494),	X_MASK,      PPCCHLK,	E500MC,		{CT, RA, RB}},

{"slbia",	X(31,498),	0xffffffff,  PPC64,	PPCNONE,	{0}},

{"cli",		X(31,502),	XRB_MASK,    POWER,	PPCNONE,	{RT, RA}},

{"popcntd",	X(31,506),	XRB_MASK, POWER7|PPCA2,	PPCNONE,	{RA, RS}},

{"cmpb",	X(31,508),	X_MASK, POWER6|PPCA2|PPC476, PPCNONE,	{RA, RS, RB}},

{"mcrxr",	X(31,512), XRARB_MASK|(3<<21), COM|PPCVLE, POWER7,	{BF}},

{"lbdx",	X(31,515),	X_MASK,      E500MC|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"bblels",	X(31,518),	X_MASK,      PPCBRLK,	PPCNONE,	{0}},

{"lvlx",	X(31,519),	X_MASK,      CELL,	PPCNONE,	{VD, RA0, RB}},
{"lbfcmux",	APU(31,519,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"subfco",	XO(31,8,1,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sfo",		XO(31,8,1,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"subco",	XO(31,8,1,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RB, RA}},
{"subfco.",	XO(31,8,1,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sfo.",	XO(31,8,1,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"subco.",	XO(31,8,1,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RB, RA}},
 
{"addco",	XO(31,10,1,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ao",		XO(31,10,1,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"addco.",	XO(31,10,1,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"ao.",		XO(31,10,1,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"lxsspx",	X(31,524),	XX1_MASK,    PPCVSX2,	PPCNONE,	{XT6, RA0, RB}},

{"clcs",	X(31,531),	XRB_MASK,    M601,	PPCNONE,	{RT, RA}},

{"ldbrx",	X(31,532),	X_MASK, CELL|POWER7|PPCA2, PPCNONE,	{RT, RA0, RB}},

{"lswx",	X(31,533),	X_MASK,  PPCCOM|PPCVLE, E500|E500MC,	{RT, RAX, RBX}},
{"lsx",		X(31,533),	X_MASK,      PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"lwbrx",	X(31,534),	X_MASK,  PPCCOM|PPCVLE, PPCNONE,	{RT, RA0, RB}},
{"lbrx",	X(31,534),	X_MASK,      PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"lfsx",	X(31,535),	X_MASK,      COM,	PPCEFS,		{FRT, RA0, RB}},

{"srw",		XRC(31,536,0),	X_MASK,  PPCCOM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"sr",		XRC(31,536,0),	X_MASK,      PWRCOM,	PPCNONE,	{RA, RS, RB}},
{"srw.",	XRC(31,536,1),	X_MASK,  PPCCOM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"sr.",		XRC(31,536,1),	X_MASK,      PWRCOM,	PPCNONE,	{RA, RS, RB}},

{"rrib",	XRC(31,537,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"rrib.",	XRC(31,537,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"srd",		XRC(31,539,0),	X_MASK,      PPC64,	PPCNONE,	{RA, RS, RB}},
{"srd.",	XRC(31,539,1),	X_MASK,      PPC64,	PPCNONE,	{RA, RS, RB}},

{"maskir",	XRC(31,541,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"maskir.",	XRC(31,541,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"lhdx",	X(31,547),	X_MASK,      E500MC|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"lvtrx",	X(31,549),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},

{"bbelr",	X(31,550),	X_MASK,      PPCBRLK,	PPCNONE,	{0}},

{"lvrx",	X(31,551),	X_MASK,      CELL,	PPCNONE,	{VD, RA0, RB}},
{"lhfcmux",	APU(31,551,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"subfo",	XO(31,40,1,0),	XO_MASK,     PPC,	PPCNONE,	{RT, RA, RB}},
{"subo",	XO(31,40,1,0),	XO_MASK,     PPC,	PPCNONE,	{RT, RB, RA}},
{"subfo.",	XO(31,40,1,1),	XO_MASK,     PPC,	PPCNONE,	{RT, RA, RB}},
{"subo.",	XO(31,40,1,1),	XO_MASK,     PPC,	PPCNONE,	{RT, RB, RA}},

{"tlbsync",	X(31,566),	0xffffffff, PPC|PPCVLE, PPCNONE,	{0}},

{"lfsux",	X(31,567),	X_MASK,      COM,	PPCEFS,		{FRT, RAS, RB}},

{"lwdx",	X(31,579),	X_MASK,      E500MC|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"lvtlx",	X(31,581),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},

{"lwfcmux",	APU(31,583,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"lxsdx",	X(31,588),	XX1_MASK,    PPCVSX,	PPCNONE,	{XT6, RA0, RB}},

{"mfsr",	X(31,595), XRB_MASK|(1<<20), COM,	NON32,  	{RT, SR}},

{"lswi",	X(31,597),	X_MASK,  PPCCOM|PPCVLE, E500|E500MC,	{RT, RAX, NBI}},
{"lsi",		X(31,597),	X_MASK,      PWRCOM,	PPCNONE,	{RT, RA0, NB}},

{"lwsync",	XSYNC(31,598,1), 0xffffffff, PPC,	E500,		{0}},
{"ptesync",	XSYNC(31,598,2), 0xffffffff, PPC64,	PPCNONE,	{0}},
{"sync",	X(31,598),	XSYNCLE_MASK,E6500,	PPCNONE,	{LS, ESYNC}},
{"sync",	X(31,598),	XSYNC_MASK, PPCCOM|PPCVLE, BOOKE|PPC476, {LS}},
{"msync",	X(31,598),	0xffffffff, BOOKE|PPCA2|PPC476, PPCNONE, {0}},
{"sync",	X(31,598),	0xffffffff, BOOKE|PPC476, E6500,	{0}},
{"lwsync",	X(31,598),	0xffffffff, E500,	PPCNONE,	{0}},
{"dcs",		X(31,598),	0xffffffff,  PWRCOM,	PPCNONE,	{0}},

{"lfdx",	X(31,599),	X_MASK,      COM,	PPCEFS,		{FRT, RA0, RB}},

{"mffgpr",	XRC(31,607,0),	XRA_MASK,    POWER6,	POWER7,		{FRT, RB}},
{"lfdepx",	X(31,607),	X_MASK,   E500MC|PPCA2|PPCVLE, PPCNONE, {FRT, RA0, RB}},

{"lddx",	X(31,611),	X_MASK,      E500MC|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"lvswx",	X(31,613),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},

{"lqfcmux",	APU(31,615,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"nego",	XO(31,104,1,0),	XORB_MASK,   COM|PPCVLE, PPCNONE,	{RT, RA}},
{"nego.",	XO(31,104,1,1),	XORB_MASK,   COM|PPCVLE, PPCNONE,	{RT, RA}},

{"mulo",	XO(31,107,1,0),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},
{"mulo.",	XO(31,107,1,1),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},

{"mfsri",	X(31,627),	X_MASK,      M601,	PPCNONE,	{RT, RA, RB}},

{"dclst",	X(31,630),	XRB_MASK,    M601,	PPCNONE,	{RS, RA}},

{"lfdux",	X(31,631),	X_MASK,      COM,	PPCEFS,		{FRT, RAS, RB}},

{"stbdx",	X(31,643),	X_MASK,      E500MC,	PPCNONE,	{RS, RA, RB}},

{"stvlx",	X(31,647),	X_MASK,      CELL,	PPCNONE,	{VS, RA0, RB}},
{"stbfcmux",	APU(31,647,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"stxsspx",	X(31,652),	XX1_MASK,    PPCVSX2,	PPCNONE,	{XS6, RA0, RB}},

{"tbegin.",	XRC(31,654,1), XRTLRARB_MASK,PPCHTM,	PPCNONE,	{HTM_R}},

{"subfeo",	XO(31,136,1,0),	XO_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sfeo",	XO(31,136,1,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"subfeo.",	XO(31,136,1,1),	XO_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"sfeo.",	XO(31,136,1,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"addeo",	XO(31,138,1,0),	XO_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"aeo",		XO(31,138,1,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"addeo.",	XO(31,138,1,1),	XO_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"aeo.",	XO(31,138,1,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"mfsrin",	X(31,659),	XRA_MASK,    PPC,	NON32,  	{RT, RB}},

{"stdbrx",	X(31,660),	X_MASK, CELL|POWER7|PPCA2, PPCNONE,	{RS, RA0, RB}},

{"stswx",	X(31,661),	X_MASK, PPCCOM|PPCVLE,	E500|E500MC,	{RS, RA0, RB}},
{"stsx",	X(31,661),	X_MASK,      PWRCOM,	PPCNONE,	{RS, RA0, RB}},

{"stwbrx",	X(31,662),	X_MASK, PPCCOM|PPCVLE,	PPCNONE,	{RS, RA0, RB}},
{"stbrx",	X(31,662),	X_MASK,      PWRCOM,	PPCNONE,	{RS, RA0, RB}},

{"stfsx",	X(31,663),	X_MASK,      COM,	PPCEFS,		{FRS, RA0, RB}},

{"srq",		XRC(31,664,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"srq.",	XRC(31,664,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"sre",		XRC(31,665,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"sre.",	XRC(31,665,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"sthdx",	X(31,675),	X_MASK,      E500MC,	PPCNONE,	{RS, RA, RB}},

{"stvfrx",	X(31,677),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"stvrx",	X(31,679),	X_MASK,      CELL,	PPCNONE,	{VS, RA0, RB}},
{"sthfcmux",	APU(31,679,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"tendall.",	XRC(31,686,1)|(1<<25), XRTRARB_MASK, PPCHTM, PPCNONE,	{0}},
{"tend.",	XRC(31,686,1), XRTARARB_MASK, PPCHTM,	PPCNONE,	{HTM_A}},

{"stbcx.",	XRC(31,694,1),	X_MASK,      POWER7,	PPCNONE,	{RS, RA0, RB}},

{"stfsux",	X(31,695),	X_MASK,      COM,	PPCEFS,		{FRS, RAS, RB}},

{"sriq",	XRC(31,696,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},
{"sriq.",	XRC(31,696,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},

{"stwdx",	X(31,707),	X_MASK,      E500MC,	PPCNONE,	{RS, RA, RB}},

{"stvflx",	X(31,709),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"stwfcmux",	APU(31,711,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"stxsdx",	X(31,716),	XX1_MASK,    PPCVSX,	PPCNONE,	{XS6, RA0, RB}},

{"tcheck",	X(31,718),   XRTBFRARB_MASK, PPCHTM,	PPCNONE,	{BF}},

{"subfzeo",	XO(31,200,1,0),	XORB_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"sfzeo",	XO(31,200,1,0),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},
{"subfzeo.",	XO(31,200,1,1),	XORB_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"sfzeo.",	XO(31,200,1,1),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},

{"addzeo",	XO(31,202,1,0),	XORB_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"azeo",	XO(31,202,1,0),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},
{"addzeo.",	XO(31,202,1,1),	XORB_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"azeo.",	XO(31,202,1,1),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},

{"stswi",	X(31,725),	X_MASK, PPCCOM|PPCVLE,	E500|E500MC,	{RS, RA0, NB}},
{"stsi",	X(31,725),	X_MASK,      PWRCOM,	PPCNONE,	{RS, RA0, NB}},

{"sthcx.",	XRC(31,726,1),	X_MASK,      POWER7,	PPCNONE,	{RS, RA0, RB}},

{"stfdx",	X(31,727),	X_MASK,      COM,	PPCEFS,		{FRS, RA0, RB}},

{"srlq",	XRC(31,728,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"srlq.",	XRC(31,728,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"sreq",	XRC(31,729,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"sreq.",	XRC(31,729,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"mftgpr",	XRC(31,735,0),	XRA_MASK,    POWER6,	POWER7,		{RT, FRB}},
{"stfdepx",	X(31,735),	X_MASK, E500MC|PPCA2|PPCVLE, PPCNONE,	{FRS, RA0, RB}},

{"stddx",	X(31,739),	X_MASK,      E500MC,	PPCNONE,	{RS, RA, RB}},

{"stvswx",	X(31,741),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"stqfcmux",	APU(31,743,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"subfmeo",	XO(31,232,1,0),	XORB_MASK,   PPCCOM,	PPCNONE,	{RT, RA}},
{"sfmeo",	XO(31,232,1,0),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},
{"subfmeo.",	XO(31,232,1,1),	XORB_MASK,   PPCCOM,	PPCNONE,	{RT, RA}},
{"sfmeo.",	XO(31,232,1,1),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},

{"mulldo",	XO(31,233,1,0),	XO_MASK,  PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulldo.",	XO(31,233,1,1),	XO_MASK,  PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"addmeo",	XO(31,234,1,0),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"ameo",	XO(31,234,1,0),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},
{"addmeo.",	XO(31,234,1,1),	XORB_MASK,   PPCCOM|PPCVLE, PPCNONE,	{RT, RA}},
{"ameo.",	XO(31,234,1,1),	XORB_MASK,   PWRCOM,	PPCNONE,	{RT, RA}},

{"mullwo",	XO(31,235,1,0),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulso",	XO(31,235,1,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"mullwo.",	XO(31,235,1,1),	XO_MASK,     PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"mulso.",	XO(31,235,1,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"tsuspend.",	XRCL(31,750,0,1), XRTRARB_MASK,PPCHTM,	PPCNONE,	{0}},
{"tresume.",	XRCL(31,750,1,1), XRTRARB_MASK,PPCHTM,	PPCNONE,	{0}},
{"tsr.",	XRC(31,750,1),    XRTLRARB_MASK,PPCHTM,	PPCNONE,	{L}},

{"dcba",	X(31,758), XRT_MASK, PPC405|PPC7450|BOOKE|PPCA2|PPC476|PPCVLE, PPCNONE, {RA0, RB}},
{"dcbal",	XOPL(31,758,1), XRT_MASK,    E500MC,	PPCNONE,	{RA0, RB}},

{"stfdux",	X(31,759),	X_MASK,      COM,	PPCEFS,		{FRS, RAS, RB}},

{"srliq",	XRC(31,760,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},
{"srliq.",	XRC(31,760,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},

{"lvsm",	X(31,773),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},
{"stvepxl",	X(31,775),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},
{"lvlxl",	X(31,775),	X_MASK,      CELL,	PPCNONE,	{VD, RA0, RB}},
{"ldfcmux",	APU(31,775,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"dozo",	XO(31,264,1,0),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},
{"dozo.",	XO(31,264,1,1),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},

{"addo",	XO(31,266,1,0),	XO_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"caxo",	XO(31,266,1,0),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},
{"addo.",	XO(31,266,1,1),	XO_MASK, PPCCOM|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"caxo.",	XO(31,266,1,1),	XO_MASK,     PWRCOM,	PPCNONE,	{RT, RA, RB}},

{"lxvw4x",	X(31,780),	XX1_MASK,    PPCVSX,	PPCNONE,	{XT6, RA0, RB}},

{"tabortwc.",	XRC(31,782,1),	X_MASK,      PPCHTM,	PPCNONE,	{TO, RA, RB}},

{"tlbivax",	X(31,786),	XRT_MASK, BOOKE|PPCA2|PPC476|PPCVLE, PPCNONE, {RA0, RB}},

{"lwzcix",	X(31,789),	X_MASK,      POWER6,	PPCNONE,	{RT, RA0, RB}},

{"lhbrx",	X(31,790),	X_MASK,      COM|PPCVLE, PPCNONE,	{RT, RA0, RB}},

{"lfdpx",	X(31,791),	X_MASK,      POWER6,	POWER7,		{FRTp, RA0, RB}},
{"lfqx",	X(31,791),	X_MASK,      POWER2,	PPCNONE,	{FRT, RA, RB}},

{"sraw",	XRC(31,792,0),	X_MASK,  PPCCOM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"sra",		XRC(31,792,0),	X_MASK,      PWRCOM,	PPCNONE,	{RA, RS, RB}},
{"sraw.",	XRC(31,792,1),	X_MASK,	 PPCCOM|PPCVLE, PPCNONE,	{RA, RS, RB}},
{"sra.",	XRC(31,792,1),	X_MASK,      PWRCOM,	PPCNONE,	{RA, RS, RB}},

{"srad",	XRC(31,794,0),	X_MASK,      PPC64,	PPCNONE,	{RA, RS, RB}},
{"srad.",	XRC(31,794,1),	X_MASK,      PPC64,	PPCNONE,	{RA, RS, RB}},

{"lfddx",	X(31,803),	X_MASK,      E500MC|PPCVLE, PPCNONE,	{FRT, RA, RB}},

{"lvtrxl",	X(31,805),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},
{"stvepx",	X(31,807),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},
{"lvrxl",	X(31,807),	X_MASK,      CELL,	PPCNONE,	{VD, RA0, RB}},

{"tabortdc.",	XRC(31,814,1),	X_MASK,      PPCHTM,	PPCNONE,	{TO, RA, RB}},

{"rac",		X(31,818),	X_MASK,      M601,	PPCNONE,	{RT, RA, RB}},

{"erativax",	X(31,819),	X_MASK,	     PPCA2,	PPCNONE,	{RS, RA0, RB}},

{"lhzcix",	X(31,821),	X_MASK,      POWER6,	PPCNONE,	{RT, RA0, RB}},

{"dss",		XDSS(31,822,0),	XDSS_MASK,   PPCVEC,	PPCNONE,	{STRM}},

{"lfqux",	X(31,823),	X_MASK,      POWER2,	PPCNONE,	{FRT, RA, RB}},

{"srawi",	XRC(31,824,0),	X_MASK,  PPCCOM|PPCVLE, PPCNONE,	{RA, RS, SH}},
{"srai",	XRC(31,824,0),	X_MASK,      PWRCOM,	PPCNONE,	{RA, RS, SH}},
{"srawi.",	XRC(31,824,1),	X_MASK,	 PPCCOM|PPCVLE, PPCNONE,	{RA, RS, SH}},
{"srai.",	XRC(31,824,1),	X_MASK,      PWRCOM,	PPCNONE,	{RA, RS, SH}},

{"sradi",	XS(31,413,0),	XS_MASK,     PPC64|PPCVLE, PPCNONE,	{RA, RS, SH6}},
{"sradi.",	XS(31,413,1),	XS_MASK,     PPC64|PPCVLE, PPCNONE,	{RA, RS, SH6}},

{"lvtlxl",	X(31,837),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},

{"divo",	XO(31,331,1,0),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},
{"divo.",	XO(31,331,1,1),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},

{"lxvd2x",	X(31,844),	XX1_MASK,    PPCVSX,	PPCNONE,	{XT6, RA0, RB}},
{"lxvx",	X(31,844),	XX1_MASK,    PPCVSX,	PPCNONE,	{XT6, RA0, RB}},

{"tabortwci.",	XRC(31,846,1),	X_MASK,      PPCHTM,	PPCNONE,	{TO, RA, HTM_SI}},

{"tlbsrx.",	XRC(31,850,1),	XRT_MASK,    PPCA2,	PPCNONE,	{RA0, RB}},

{"slbmfev",	X(31,851),	XRA_MASK,    PPC64,	PPCNONE,	{RT, RB}},

{"lbzcix",	X(31,853),	X_MASK,      POWER6,	PPCNONE,	{RT, RA0, RB}},

{"eieio",	X(31,854),	0xffffffff,  PPC,   BOOKE|PPCA2|PPC476,	{0}},
{"mbar",	X(31,854),	X_MASK, BOOKE|PPCA2|PPC476|PPCVLE, PPCNONE, {MO}},
{"eieio",	XMBAR(31,854,1),0xffffffff,  E500,	PPCNONE,	{0}},
{"eieio",	X(31,854),	0xffffffff, PPCA2|PPC476, PPCNONE,	{0}},

{"lfiwax",	X(31,855),	X_MASK, POWER6|PPCA2|PPC476, PPCNONE,	{FRT, RA0, RB}},

{"lvswxl",	X(31,869),	X_MASK,      PPCVEC2,	PPCNONE,	{VD, RA0, RB}},

{"abso",	XO(31,360,1,0),	XORB_MASK,   M601,	PPCNONE,	{RT, RA}},
{"abso.",	XO(31,360,1,1),	XORB_MASK,   M601,	PPCNONE,	{RT, RA}},

{"divso",	XO(31,363,1,0),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},
{"divso.",	XO(31,363,1,1),	XO_MASK,     M601,	PPCNONE,	{RT, RA, RB}},

{"tabortdci.",	XRC(31,878,1),	X_MASK,      PPCHTM,	PPCNONE,	{TO, RA, HTM_SI}},

{"ldcix",	X(31,885),	X_MASK,      POWER6,	PPCNONE,	{RT, RA0, RB}},

{"lfiwzx",	X(31,887),	X_MASK,   POWER7|PPCA2,	PPCNONE,	{FRT, RA0, RB}},

{"stvlxl",	X(31,903),	X_MASK,      CELL,	PPCNONE,	{VS, RA0, RB}},
{"stdfcmux",	APU(31,903,0), 	APU_MASK,    PPC405,	PPCNONE,	{FCRT, RA, RB}},

{"divdeuo",	XO(31,393,1,0),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divdeuo.",	XO(31,393,1,1),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divweuo",	XO(31,395,1,0),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divweuo.",	XO(31,395,1,1),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},

{"stxvw4x",	X(31,908),	XX1_MASK,    PPCVSX,	PPCNONE,	{XS6, RA0, RB}},

{"tabort.",	XRC(31,910,1),	XRTRB_MASK,  PPCHTM,	PPCNONE,	{RA}},

{"tlbsx",	XRC(31,914,0),	X_MASK, PPC403|BOOKE|PPCA2|PPC476, PPCNONE, {RTO, RA0, RB}},
{"tlbsx.",	XRC(31,914,1),	X_MASK, PPC403|BOOKE|PPCA2|PPC476, PPCNONE, {RTO, RA0, RB}},

{"slbmfee",	X(31,915),	XRA_MASK,    PPC64,	PPCNONE,	{RT, RB}},

{"stwcix",	X(31,917),	X_MASK,      POWER6,	PPCNONE,	{RS, RA0, RB}},

{"sthbrx",	X(31,918),	X_MASK,      COM,	PPCNONE,	{RS, RA0, RB}},

{"stfdpx",	X(31,919),	X_MASK,      POWER6,	POWER7,		{FRSp, RA0, RB}},
{"stfqx",	X(31,919),	X_MASK,      POWER2,	PPCNONE,	{FRS, RA0, RB}},

{"sraq",	XRC(31,920,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"sraq.",	XRC(31,920,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"srea",	XRC(31,921,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},
{"srea.",	XRC(31,921,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, RB}},

{"extsh",	XRC(31,922,0),	XRB_MASK,    PPCCOM|PPCVLE, PPCNONE,	{RA, RS}},
{"exts",	XRC(31,922,0),	XRB_MASK,    PWRCOM,	PPCNONE,	{RA, RS}},
{"extsh.",	XRC(31,922,1),	XRB_MASK,    PPCCOM|PPCVLE, PPCNONE,	{RA, RS}},
{"exts.",	XRC(31,922,1),	XRB_MASK,    PWRCOM,	PPCNONE,	{RA, RS}},

{"stfddx",	X(31,931),	X_MASK,      E500MC,	PPCNONE,	{FRS, RA, RB}},

{"stvfrxl",	X(31,933),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"wclrone",	XOPL2(31,934,2),XRT_MASK,    PPCA2,	PPCNONE,	{RA0, RB}},
{"wclrall",	X(31,934),	XRARB_MASK,  PPCA2,	PPCNONE,	{L}},
{"wclr",	X(31,934),	X_MASK,	     PPCA2,	PPCNONE,	{L, RA0, RB}},

{"stvrxl",	X(31,935),	X_MASK,      CELL,	PPCNONE,	{VS, RA0, RB}},

{"divdeo",	XO(31,425,1,0),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divdeo.",	XO(31,425,1,1),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divweo",	XO(31,427,1,0),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},
{"divweo.",	XO(31,427,1,1),	XO_MASK,  POWER7|PPCA2,	PPCNONE,	{RT, RA, RB}},

{"treclaim.",	XRC(31,942,1),	XRTRB_MASK,  PPCHTM,	PPCNONE,	{RA}},

{"tlbrehi",	XTLB(31,946,0),	XTLB_MASK,   PPC403,	PPCA2,		{RT, RA}},
{"tlbrelo",	XTLB(31,946,1),	XTLB_MASK,   PPC403,	PPCA2,		{RT, RA}},
{"tlbre",	X(31,946),	X_MASK, PPC403|BOOKE|PPCA2|PPC476, PPCNONE, {RSO, RAOPT, SHO}},

{"sthcix",	X(31,949),	X_MASK,      POWER6,	PPCNONE,	{RS, RA0, RB}},

{"icswepx",	XRC(31,950,0),	X_MASK,      PPCA2,	PPCNONE,	{RS, RA, RB}},
{"icswepx.",	XRC(31,950,1),	X_MASK,      PPCA2,	PPCNONE,	{RS, RA, RB}},

{"stfqux",	X(31,951),	X_MASK,      POWER2,	PPCNONE,	{FRS, RA, RB}},

{"sraiq",	XRC(31,952,0),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},
{"sraiq.",	XRC(31,952,1),	X_MASK,      M601,	PPCNONE,	{RA, RS, SH}},

{"extsb",	XRC(31,954,0),	XRB_MASK, PPC|PPCVLE,	PPCNONE,	{RA, RS}},
{"extsb.",	XRC(31,954,1),	XRB_MASK, PPC|PPCVLE,	PPCNONE,	{RA, RS}},

{"stvflxl",	X(31,965),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"iccci",	X(31,966), XRT_MASK, PPC403|PPC440|TITAN|PPCA2, PPCNONE, {RAOPT, RBOPT}},
{"ici",		X(31,966),	XRARB_MASK,  PPCA2|PPC476|PPCVLE, PPCNONE, {CT}},
 
{"divduo",	XO(31,457,1,0),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"divduo.",	XO(31,457,1,1),	XO_MASK,     PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
 
{"divwuo",	XO(31,459,1,0),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"divwuo.",	XO(31,459,1,1),	XO_MASK,     PPC|PPCVLE, PPCNONE,	{RT, RA, RB}},

{"stxvd2x",	X(31,972),	XX1_MASK,    PPCVSX,	PPCNONE,	{XS6, RA0, RB}},
{"stxvx",	X(31,972),	XX1_MASK,    PPCVSX,	PPCNONE,	{XS6, RA0, RB}},

{"tlbld",	X(31,978),	XRTRA_MASK,  PPC, PPC403|BOOKE|PPCA2|PPC476, {RB}},
{"tlbwehi",	XTLB(31,978,0),	XTLB_MASK,   PPC403,	PPCNONE,	{RT, RA}},
{"tlbwelo",	XTLB(31,978,1),	XTLB_MASK,   PPC403,	PPCNONE,	{RT, RA}},
{"tlbwe",	X(31,978),	X_MASK, PPC403|BOOKE|PPCA2|PPC476, PPCNONE, {RSO, RAOPT, SHO}},

{"stbcix",	X(31,981),	X_MASK,      POWER6,	PPCNONE,	{RS, RA0, RB}},

{"icbi",	X(31,982),	XRT_MASK,    PPC|PPCVLE, PPCNONE,	{RA0, RB}},

{"stfiwx",	X(31,983),	X_MASK,      PPC,	PPCEFS,		{FRS, RA0, RB}},

{"extsw",	XRC(31,986,0),  XRB_MASK, PPC64|PPCVLE, PPCNONE,	{RA, RS}},
{"extsw.",	XRC(31,986,1),	XRB_MASK, PPC64|PPCVLE, PPCNONE,	{RA, RS}},

{"icbiep",	XRT(31,991,0),	XRT_MASK, E500MC|PPCA2|PPCVLE, PPCNONE, {RA0, RB}},

{"stvswxl",	X(31,997),	X_MASK,      PPCVEC2,	PPCNONE,	{VS, RA0, RB}},

{"icread",	X(31,998), XRT_MASK, PPC403|PPC440|PPC476|TITAN|PPCVLE, PPCNONE, {RA0, RB}},

{"nabso",	XO(31,488,1,0),	XORB_MASK,   M601,	PPCNONE,	{RT, RA}},
{"nabso.",	XO(31,488,1,1),	XORB_MASK,   M601,	PPCNONE,	{RT, RA}},

{"divdo",	XO(31,489,1,0),	XO_MASK,  PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
{"divdo.",	XO(31,489,1,1),	XO_MASK,  PPC64|PPCVLE, PPCNONE,	{RT, RA, RB}},
 
{"divwo",	XO(31,491,1,0),	XO_MASK,   PPC|PPCVLE,	PPCNONE,	{RT, RA, RB}},
{"divwo.",	XO(31,491,1,1),	XO_MASK,   PPC|PPCVLE,	PPCNONE,	{RT, RA, RB}},

{"trechkpt.",	XRC(31,1006,1),	XRTRARB_MASK,PPCHTM,	PPCNONE,	{0}},

{"tlbli",	X(31,1010),	XRTRA_MASK,  PPC,	TITAN,  	{RB}},

{"stdcix",	X(31,1013),	X_MASK,      POWER6,	PPCNONE,	{RS, RA0, RB}},

{"dcbz",	X(31,1014),	XRT_MASK,    PPC|PPCVLE, PPCNONE,	{RA0, RB}},
{"dclz",	X(31,1014),	XRT_MASK,    PPC,	PPCNONE,	{RA0, RB}},

{"dcbzep",	XRT(31,1023,0),	XRT_MASK, E500MC|PPCA2|PPCVLE, PPCNONE,	{RA0, RB}},

{"dcbzl",	XOPL(31,1014,1), XRT_MASK, POWER4|E500MC, PPC476,	{RA0, RB}},

{"cctpl",	0x7c210b78,	0xffffffff,  CELL,	PPCNONE,	{0}},
{"cctpm",	0x7c421378,	0xffffffff,  CELL,	PPCNONE,	{0}},
{"cctph",	0x7c631b78,	0xffffffff,  CELL,	PPCNONE,	{0}},

{"dstt",	XDSS(31,342,1),	XDSS_MASK,   PPCVEC,	PPCNONE,	{RA, RB, STRM}},
{"dststt",	XDSS(31,374,1),	XDSS_MASK,   PPCVEC,	PPCNONE,	{RA, RB, STRM}},
{"dssall",	XDSS(31,822,1),	XDSS_MASK,   PPCVEC,	PPCNONE,	{0}},

{"db8cyc",	0x7f9ce378,	0xffffffff,  CELL,	PPCNONE,	{0}},
{"db10cyc",	0x7fbdeb78,	0xffffffff,  CELL,	PPCNONE,	{0}},
{"db12cyc",	0x7fdef378,	0xffffffff,  CELL,	PPCNONE,	{0}},
{"db16cyc",	0x7ffffb78,	0xffffffff,  CELL,	PPCNONE,	{0}},

{"lwz",		OP(32),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, D, RA0}},
{"l",		OP(32),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, D, RA0}},

{"lwzu",	OP(33),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, D, RAL}},
{"lu",		OP(33),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, D, RA0}},

{"lbz",		OP(34),		OP_MASK,     COM,	PPCNONE,	{RT, D, RA0}},

{"lbzu",	OP(35),		OP_MASK,     COM,	PPCNONE,	{RT, D, RAL}},

{"stw",		OP(36),		OP_MASK,     PPCCOM,	PPCNONE,	{RS, D, RA0}},
{"st",		OP(36),		OP_MASK,     PWRCOM,	PPCNONE,	{RS, D, RA0}},

{"stwu",	OP(37),		OP_MASK,     PPCCOM,	PPCNONE,	{RS, D, RAS}},
{"stu",		OP(37),		OP_MASK,     PWRCOM,	PPCNONE,	{RS, D, RA0}},

{"stb",		OP(38),		OP_MASK,     COM,	PPCNONE,	{RS, D, RA0}},

{"stbu",	OP(39),		OP_MASK,     COM,	PPCNONE,	{RS, D, RAS}},

{"lhz",		OP(40),		OP_MASK,     COM,	PPCNONE,	{RT, D, RA0}},

{"lhzu",	OP(41),		OP_MASK,     COM,	PPCNONE,	{RT, D, RAL}},

{"lha",		OP(42),		OP_MASK,     COM,	PPCNONE,	{RT, D, RA0}},

{"lhau",	OP(43),		OP_MASK,     COM,	PPCNONE,	{RT, D, RAL}},

{"sth",		OP(44),		OP_MASK,     COM,	PPCNONE,	{RS, D, RA0}},

{"sthu",	OP(45),		OP_MASK,     COM,	PPCNONE,	{RS, D, RAS}},

{"lmw",		OP(46),		OP_MASK,     PPCCOM,	PPCNONE,	{RT, D, RAM}},
{"lm",		OP(46),		OP_MASK,     PWRCOM,	PPCNONE,	{RT, D, RA0}},

{"stmw",	OP(47),		OP_MASK,     PPCCOM,	PPCNONE,	{RS, D, RA0}},
{"stm",		OP(47),		OP_MASK,     PWRCOM,	PPCNONE,	{RS, D, RA0}},

{"lfs",		OP(48),		OP_MASK,     COM,	PPCEFS,		{FRT, D, RA0}},

{"lfsu",	OP(49),		OP_MASK,     COM,	PPCEFS,		{FRT, D, RAS}},

{"lfd",		OP(50),		OP_MASK,     COM,	PPCEFS,		{FRT, D, RA0}},

{"lfdu",	OP(51),		OP_MASK,     COM,	PPCEFS,		{FRT, D, RAS}},

{"stfs",	OP(52),		OP_MASK,     COM,	PPCEFS,		{FRS, D, RA0}},

{"stfsu",	OP(53),		OP_MASK,     COM,	PPCEFS,		{FRS, D, RAS}},

{"stfd",	OP(54),		OP_MASK,     COM,	PPCEFS,		{FRS, D, RA0}},

{"stfdu",	OP(55),		OP_MASK,     COM,	PPCEFS,		{FRS, D, RAS}},

{"lq",		OP(56),		OP_MASK,     POWER4,	PPC476,		{RTQ, DQ, RAQ}},
{"psq_l",	OP(56),		OP_MASK,     PPCPS,	PPCNONE,	{FRT,PSD,RA,PSW,PSQ}},
{"lfq",		OP(56),		OP_MASK,     POWER2,	PPCNONE,	{FRT, D, RA0}},

{"lfdp",	OP(57),		OP_MASK,     POWER6,	POWER7,		{FRTp, DS, RA0}},
{"psq_lu",	OP(57),		OP_MASK,     PPCPS,	PPCNONE,	{FRT,PSD,RA,PSW,PSQ}},
{"lfqu",	OP(57),		OP_MASK,     POWER2,	PPCNONE,	{FRT, D, RA0}},

{"ld",		DSO(58,0),	DS_MASK,     PPC64,	PPCNONE,	{RT, DS, RA0}},
{"ldu",		DSO(58,1),	DS_MASK,     PPC64,	PPCNONE,	{RT, DS, RAL}},
{"lwa",		DSO(58,2),	DS_MASK,     PPC64,	PPCNONE,	{RT, DS, RA0}},

{"dadd",	XRC(59,2,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},
{"dadd.",	XRC(59,2,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},

{"dqua",	ZRC(59,3,0),	Z2_MASK,     POWER6,	PPCNONE,	{FRT,FRA,FRB,RMC}},
{"dqua.",	ZRC(59,3,1),	Z2_MASK,     POWER6,	PPCNONE,	{FRT,FRA,FRB,RMC}},

{"fdivs",	A(59,18,0),	AFRC_MASK,   PPC,	PPCEFS,		{FRT, FRA, FRB}},
{"fdivs.",	A(59,18,1),	AFRC_MASK,   PPC,	PPCEFS,		{FRT, FRA, FRB}},

{"fsubs",	A(59,20,0),	AFRC_MASK,   PPC,	PPCEFS,		{FRT, FRA, FRB}},
{"fsubs.",	A(59,20,1),	AFRC_MASK,   PPC,	PPCEFS,		{FRT, FRA, FRB}},

{"fadds",	A(59,21,0),	AFRC_MASK,   PPC,	PPCEFS,		{FRT, FRA, FRB}},
{"fadds.",	A(59,21,1),	AFRC_MASK,   PPC,	PPCEFS,		{FRT, FRA, FRB}},

{"fsqrts",	A(59,22,0),    AFRAFRC_MASK, PPC,	TITAN,  	{FRT, FRB}},
{"fsqrts.",	A(59,22,1),    AFRAFRC_MASK, PPC,	TITAN,  	{FRT, FRB}},

{"fres",	A(59,24,0),   AFRAFRC_MASK,  POWER7,	PPCNONE,	{FRT, FRB}},
{"fres",	A(59,24,0),   AFRALFRC_MASK, PPC,	POWER7,		{FRT, FRB, A_L}},
{"fres.",	A(59,24,1),   AFRAFRC_MASK,  POWER7,	PPCNONE,	{FRT, FRB}},
{"fres.",	A(59,24,1),   AFRALFRC_MASK, PPC,	POWER7,		{FRT, FRB, A_L}},

{"fmuls",	A(59,25,0),	AFRB_MASK,   PPC,	PPCEFS,		{FRT, FRA, FRC}},
{"fmuls.",	A(59,25,1),	AFRB_MASK,   PPC,	PPCEFS,		{FRT, FRA, FRC}},

{"frsqrtes",	A(59,26,0),   AFRAFRC_MASK,  POWER7,	PPCNONE,	{FRT, FRB}},
{"frsqrtes",	A(59,26,0),   AFRALFRC_MASK, POWER5,	POWER7,		{FRT, FRB, A_L}},
{"frsqrtes.",	A(59,26,1),   AFRAFRC_MASK,  POWER7,	PPCNONE,	{FRT, FRB}},
{"frsqrtes.",	A(59,26,1),   AFRALFRC_MASK, POWER5,	POWER7,		{FRT, FRB, A_L}},

{"fmsubs",	A(59,28,0),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fmsubs.",	A(59,28,1),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},

{"fmadds",	A(59,29,0),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fmadds.",	A(59,29,1),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},

{"fnmsubs",	A(59,30,0),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fnmsubs.",	A(59,30,1),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},

{"fnmadds",	A(59,31,0),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fnmadds.",	A(59,31,1),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},

{"dmul",	XRC(59,34,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},
{"dmul.",	XRC(59,34,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},

{"drrnd",	ZRC(59,35,0),	Z2_MASK,     POWER6,	PPCNONE,	{FRT, FRA, FRB, RMC}},
{"drrnd.",	ZRC(59,35,1),	Z2_MASK,     POWER6,	PPCNONE,	{FRT, FRA, FRB, RMC}},

{"dscli",	ZRC(59,66,0),	Z_MASK,      POWER6,	PPCNONE,	{FRT, FRA, SH16}},
{"dscli.",	ZRC(59,66,1),	Z_MASK,      POWER6,	PPCNONE,	{FRT, FRA, SH16}},

{"dquai",	ZRC(59,67,0),	Z2_MASK,     POWER6,	PPCNONE,	{TE, FRT,FRB,RMC}},
{"dquai.",	ZRC(59,67,1),	Z2_MASK,     POWER6,	PPCNONE,	{TE, FRT,FRB,RMC}},

{"dscri",	ZRC(59,98,0),	Z_MASK,      POWER6,	PPCNONE,	{FRT, FRA, SH16}},
{"dscri.",	ZRC(59,98,1),	Z_MASK,      POWER6,	PPCNONE,	{FRT, FRA, SH16}},

{"drintx",	ZRC(59,99,0),	Z2_MASK,     POWER6,	PPCNONE,	{R, FRT, FRB, RMC}},
{"drintx.",	ZRC(59,99,1),	Z2_MASK,     POWER6,	PPCNONE,	{R, FRT, FRB, RMC}},

{"dcmpo",	X(59,130),	X_MASK,      POWER6,	PPCNONE,	{BF,  FRA, FRB}},

{"dtstex",	X(59,162),	X_MASK,      POWER6,	PPCNONE,	{BF,  FRA, FRB}},
{"dtstdc",	Z(59,194),	Z_MASK,      POWER6,	PPCNONE,	{BF,  FRA, DCM}},
{"dtstdg",	Z(59,226),	Z_MASK,      POWER6,	PPCNONE,	{BF,  FRA, DGM}},

{"drintn",	ZRC(59,227,0),	Z2_MASK,     POWER6,	PPCNONE,	{R, FRT, FRB, RMC}},
{"drintn.",	ZRC(59,227,1),	Z2_MASK,     POWER6,	PPCNONE,	{R, FRT, FRB, RMC}},

{"dctdp",	XRC(59,258,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRB}},
{"dctdp.",	XRC(59,258,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRB}},

{"dctfix",	XRC(59,290,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRB}},
{"dctfix.",	XRC(59,290,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRB}},

{"ddedpd",	XRC(59,322,0),	X_MASK,      POWER6,	PPCNONE,	{SP, FRT, FRB}}, 
{"ddedpd.",	XRC(59,322,1),	X_MASK,      POWER6,	PPCNONE,	{SP, FRT, FRB}}, 

{"dxex",	XRC(59,354,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRB}},
{"dxex.",	XRC(59,354,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRB}},

{"dsub",	XRC(59,514,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},
{"dsub.",	XRC(59,514,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},

{"ddiv",	XRC(59,546,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},
{"ddiv.",	XRC(59,546,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},

{"dcmpu",	X(59,642),	X_MASK,      POWER6,	PPCNONE,	{BF,  FRA, FRB}},

{"dtstsf",	X(59,674),	X_MASK,      POWER6,	PPCNONE,	{BF,  FRA, FRB}},

{"drsp",	XRC(59,770,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRB}},
{"drsp.",	XRC(59,770,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRB}},

{"dcffix",	XRC(59,802,0), X_MASK|FRA_MASK, POWER7,	PPCNONE,	{FRT, FRB}},
{"dcffix.",	XRC(59,802,1), X_MASK|FRA_MASK, POWER7,	PPCNONE,	{FRT, FRB}},

{"denbcd",	XRC(59,834,0),	X_MASK,      POWER6,	PPCNONE,	{S, FRT, FRB}},
{"denbcd.",	XRC(59,834,1),	X_MASK,      POWER6,	PPCNONE,	{S, FRT, FRB}},

{"fcfids",	XRC(59,846,0),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},
{"fcfids.",	XRC(59,846,1),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},

{"diex",	XRC(59,866,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},
{"diex.",	XRC(59,866,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRA, FRB}},

{"fcfidus",	XRC(59,974,0),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},
{"fcfidus.",	XRC(59,974,1),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},

{"xsaddsp",	XX3(60,0),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xsmaddasp",	XX3(60,1),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xxsldwi",	XX3(60,2),	XX3SHW_MASK, PPCVSX,	PPCNONE,	{XT6, XA6, XB6, SHW}},
{"xxsel",	XX4(60,3),	XX4_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6, XC6}},
{"xssubsp",	XX3(60,8),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xsmaddmsp",	XX3(60,9),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xxspltd",	XX3(60,10),	XX3DM_MASK,  PPCVSX,	PPCNONE,	{XT6, XA6, XB6S, DMEX}},
{"xxmrghd",	XX3(60,10),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xxswapd",	XX3(60,10)|(2<<8), XX3_MASK, PPCVSX,	PPCNONE,	{XT6, XA6, XB6S}},
{"xxmrgld",	XX3(60,10)|(3<<8), XX3_MASK, PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xxpermdi",	XX3(60,10),	XX3DM_MASK,  PPCVSX,	PPCNONE,	{XT6, XA6, XB6, DM}},
{"xsrsqrtesp",	XX2(60,10),	XX2_MASK,    PPCVSX2,	PPCNONE,	{XT6, XB6}},
{"xssqrtsp",	XX2(60,11),	XX2_MASK,    PPCVSX2,	PPCNONE,	{XT6, XB6}},
{"xsmulsp",	XX3(60,16),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xsmsubasp",	XX3(60,17),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xxmrghw",	XX3(60,18),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsdivsp",	XX3(60,24),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xsmsubmsp",	XX3(60,25),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xsresp",	XX2(60,26),	XX2_MASK,    PPCVSX2,	PPCNONE,	{XT6, XB6}},
{"xsadddp",	XX3(60,32),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsmaddadp",	XX3(60,33),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xscmpudp",	XX3(60,35),	XX3BF_MASK,  PPCVSX,	PPCNONE,	{BF, XA6, XB6}},
{"xssubdp",	XX3(60,40),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsmaddmdp",	XX3(60,41),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xscmpodp",	XX3(60,43),	XX3BF_MASK,  PPCVSX,	PPCNONE,	{BF, XA6, XB6}},
{"xsmuldp",	XX3(60,48),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsmsubadp",	XX3(60,49),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xxmrglw",	XX3(60,50),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsdivdp",	XX3(60,56),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsmsubmdp",	XX3(60,57),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xstdivdp",	XX3(60,61),	XX3BF_MASK,  PPCVSX,	PPCNONE,	{BF, XA6, XB6}},
{"xvaddsp",	XX3(60,64),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmaddasp",	XX3(60,65),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpeqsp",	XX3RC(60,67,0),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpeqsp.",	XX3RC(60,67,1),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvsubsp",	XX3(60,72),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xscvdpuxws",	XX2(60,72),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvmaddmsp",	XX3(60,73),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsrdpi",	XX2(60,73),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsrsqrtedp",	XX2(60,74),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xssqrtdp",	XX2(60,75),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcmpgtsp",	XX3RC(60,75,0),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpgtsp.",	XX3RC(60,75,1),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmulsp",	XX3(60,80),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmsubasp",	XX3(60,81),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpgesp",	XX3RC(60,83,0),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpgesp.",	XX3RC(60,83,1),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvdivsp",	XX3(60,88),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xscvdpsxws",	XX2(60,88),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvmsubmsp",	XX3(60,89),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsrdpiz",	XX2(60,89),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsredp",	XX2(60,90),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvtdivsp",	XX3(60,93),	XX3BF_MASK,  PPCVSX,	PPCNONE,	{BF, XA6, XB6}},
{"xvadddp",	XX3(60,96),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmaddadp",	XX3(60,97),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpeqdp",	XX3RC(60,99,0),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpeqdp.",	XX3RC(60,99,1),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvsubdp",	XX3(60,104),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmaddmdp",	XX3(60,105),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsrdpip",	XX2(60,105),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xstsqrtdp",	XX2(60,106),	XX2BF_MASK,  PPCVSX,	PPCNONE,	{BF, XB6}},
{"xsrdpic",	XX2(60,107),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcmpgtdp",	XX3RC(60,107,0), XX3_MASK,   PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpgtdp.",	XX3RC(60,107,1), XX3_MASK,   PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmuldp",	XX3(60,112),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmsubadp",	XX3(60,113),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpgedp",	XX3RC(60,115,0), XX3_MASK,   PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcmpgedp.",	XX3RC(60,115,1), XX3_MASK,   PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvdivdp",	XX3(60,120),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmsubmdp",	XX3(60,121),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsrdpim",	XX2(60,121),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvtdivdp",	XX3(60,125),	XX3BF_MASK,  PPCVSX,	PPCNONE,	{BF, XA6, XB6}},
{"xsnmaddasp",	XX3(60,129),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xxland",	XX3(60,130),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcvspuxws",	XX2(60,136),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsnmaddmsp",	XX3(60,137),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrspi",	XX2(60,137),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xxlandc",	XX3(60,138),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrsqrtesp",	XX2(60,138),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvsqrtsp",	XX2(60,139),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsnmsubasp",	XX3(60,145),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xxlor",	XX3(60,146),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcvspsxws",	XX2(60,152),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsnmsubmsp",	XX3(60,153),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrspiz",	XX2(60,153),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xxlxor",	XX3(60,154),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvresp",	XX2(60,154),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsmaxdp",	XX3(60,160),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsnmaddadp",	XX3(60,161),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xxlnor",	XX3(60,162),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xxspltw",	XX2(60,164),	XX2UIM_MASK, PPCVSX,	PPCNONE,	{XT6, XB6, UIM}},
{"xsmindp",	XX3(60,168),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcvuxwsp",	XX2(60,168),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsnmaddmdp",	XX3(60,169),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrspip",	XX2(60,169),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvtsqrtsp",	XX2(60,170),	XX2BF_MASK,  PPCVSX,	PPCNONE,	{BF, XB6}},
{"xxlorc",	XX3(60,170),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrspic",	XX2(60,171),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xscpsgndp",	XX3(60,176),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xsnmsubadp",	XX3(60,177),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xxlnand",	XX3(60,178),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcvsxwsp",	XX2(60,184),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsnmsubmdp",	XX3(60,185),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrspim",	XX2(60,185),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xxleqv",	XX3(60,186),	XX3_MASK,    PPCVSX2,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmaxsp",	XX3(60,192),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvnmaddasp",	XX3(60,193),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvminsp",	XX3(60,200),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcvdpuxws",	XX2(60,200),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvnmaddmsp",	XX3(60,201),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrdpi",	XX2(60,201),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvrsqrtedp",	XX2(60,202),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvsqrtdp",	XX2(60,203),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvmovsp",	XX3(60,208),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6S}},
{"xvcpsgnsp",	XX3(60,208),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvnmsubasp",	XX3(60,209),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcvdpsxws",	XX2(60,216),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvnmsubmsp",	XX3(60,217),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrdpiz",	XX2(60,217),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvredp",	XX2(60,218),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvmaxdp",	XX3(60,224),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvnmaddadp",	XX3(60,225),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvmindp",	XX3(60,232),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvnmaddmdp",	XX3(60,233),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcvuxwdp",	XX2(60,232),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvrdpip",	XX2(60,233),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvtsqrtdp",	XX2(60,234),	XX2BF_MASK,  PPCVSX,	PPCNONE,	{BF, XB6}},
{"xvrdpic",	XX2(60,235),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvmovdp",	XX3(60,240),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6S}},
{"xvcpsgndp",	XX3(60,240),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvnmsubadp",	XX3(60,241),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvcvsxwdp",	XX2(60,248),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvnmsubmdp",	XX3(60,249),	XX3_MASK,    PPCVSX,	PPCNONE,	{XT6, XA6, XB6}},
{"xvrdpim",	XX2(60,249),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xscvdpsp",	XX2(60,265),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xscvdpspn",	XX2(60,267),	XX2_MASK,    PPCVSX2,	PPCNONE,	{XT6, XB6}},
{"xsrsp",	XX2(60,281),	XX2_MASK,    PPCVSX2,	PPCNONE,	{XT6, XB6}},
{"xscvuxdsp",	XX2(60,296),	XX2_MASK,    PPCVSX2,	PPCNONE,	{XT6, XB6}},
{"xscvsxdsp",	XX2(60,312),	XX2_MASK,    PPCVSX2,	PPCNONE,	{XT6, XB6}},
{"xscvdpuxds",	XX2(60,328),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xscvspdp",	XX2(60,329),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xscvspdpn",	XX2(60,331),	XX2_MASK,    PPCVSX2,	PPCNONE,	{XT6, XB6}},
{"xscvdpsxds",	XX2(60,344),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsabsdp",	XX2(60,345),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xscvuxddp",	XX2(60,360),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsnabsdp",	XX2(60,361),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xscvsxddp",	XX2(60,376),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xsnegdp",	XX2(60,377),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvspuxds",	XX2(60,392),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvdpsp",	XX2(60,393),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvspsxds",	XX2(60,408),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvabssp",	XX2(60,409),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvuxdsp",	XX2(60,424),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvnabssp",	XX2(60,425),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvsxdsp",	XX2(60,440),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvnegsp",	XX2(60,441),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvdpuxds",	XX2(60,456),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvspdp",	XX2(60,457),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvdpsxds",	XX2(60,472),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvabsdp",	XX2(60,473),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvuxddp",	XX2(60,488),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvnabsdp",	XX2(60,489),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvcvsxddp",	XX2(60,504),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},
{"xvnegdp",	XX2(60,505),	XX2_MASK,    PPCVSX,	PPCNONE,	{XT6, XB6}},

{"psq_st",	OP(60),		OP_MASK,     PPCPS,	PPCNONE,	{FRS,PSD,RA,PSW,PSQ}},
{"stfq",	OP(60),		OP_MASK,     POWER2,	PPCNONE,	{FRS, D, RA}},

{"stfdp",	OP(61),		OP_MASK,     POWER6,	POWER7,		{FRSp, DS, RA0}},
{"psq_stu",	OP(61),		OP_MASK,     PPCPS,	PPCNONE,	{FRS,PSD,RA,PSW,PSQ}},
{"stfqu",	OP(61),		OP_MASK,     POWER2,	PPCNONE,	{FRS, D, RA}},

{"std",		DSO(62,0),	DS_MASK,     PPC64,	PPCNONE,	{RS, DS, RA0}},
{"stdu",	DSO(62,1),	DS_MASK,     PPC64,	PPCNONE,	{RS, DS, RAS}},
{"stq",		DSO(62,2),	DS_MASK,     POWER4,	PPC476,		{RSQ, DS, RA0}},

{"fcmpu",	X(63,0),     X_MASK|(3<<21), COM,	PPCEFS,		{BF, FRA, FRB}},

{"daddq",	XRC(63,2,0),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, FRBp}},
{"daddq.",	XRC(63,2,1),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, FRBp}},

{"dquaq",	ZRC(63,3,0),	Z2_MASK,     POWER6,	PPCNONE,	{FRTp, FRAp, FRBp, RMC}},
{"dquaq.",	ZRC(63,3,1),	Z2_MASK,     POWER6,	PPCNONE,	{FRTp, FRAp, FRBp, RMC}},

{"fcpsgn",	XRC(63,8,0),	X_MASK, POWER6|PPCA2|PPC476, PPCNONE,	{FRT, FRA, FRB}},
{"fcpsgn.",	XRC(63,8,1),	X_MASK, POWER6|PPCA2|PPC476, PPCNONE,	{FRT, FRA, FRB}},

{"frsp",	XRC(63,12,0),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},
{"frsp.",	XRC(63,12,1),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},

{"fctiw",	XRC(63,14,0),	XRA_MASK,    PPCCOM,	PPCEFS,		{FRT, FRB}},
{"fcir",	XRC(63,14,0),	XRA_MASK,    PWR2COM,	PPCNONE,	{FRT, FRB}},
{"fctiw.",	XRC(63,14,1),	XRA_MASK,    PPCCOM,	PPCEFS,		{FRT, FRB}},
{"fcir.",	XRC(63,14,1),	XRA_MASK,    PWR2COM,	PPCNONE,	{FRT, FRB}},

{"fctiwz",	XRC(63,15,0),	XRA_MASK,    PPCCOM,	PPCEFS,		{FRT, FRB}},
{"fcirz",	XRC(63,15,0),	XRA_MASK,    PWR2COM,	PPCNONE,	{FRT, FRB}},
{"fctiwz.",	XRC(63,15,1),	XRA_MASK,    PPCCOM,	PPCEFS,		{FRT, FRB}},
{"fcirz.",	XRC(63,15,1),	XRA_MASK,    PWR2COM,	PPCNONE,	{FRT, FRB}},

{"fdiv",	A(63,18,0),	AFRC_MASK,   PPCCOM,	PPCEFS,		{FRT, FRA, FRB}},
{"fd",		A(63,18,0),	AFRC_MASK,   PWRCOM,	PPCNONE,	{FRT, FRA, FRB}},
{"fdiv.",	A(63,18,1),	AFRC_MASK,   PPCCOM,	PPCEFS,		{FRT, FRA, FRB}},
{"fd.",		A(63,18,1),	AFRC_MASK,   PWRCOM,	PPCNONE,	{FRT, FRA, FRB}},

{"fsub",	A(63,20,0),	AFRC_MASK,   PPCCOM,	PPCEFS,		{FRT, FRA, FRB}},
{"fs",		A(63,20,0),	AFRC_MASK,   PWRCOM,	PPCNONE,	{FRT, FRA, FRB}},
{"fsub.",	A(63,20,1),	AFRC_MASK,   PPCCOM,	PPCEFS,		{FRT, FRA, FRB}},
{"fs.",		A(63,20,1),	AFRC_MASK,   PWRCOM,	PPCNONE,	{FRT, FRA, FRB}},

{"fadd",	A(63,21,0),	AFRC_MASK,   PPCCOM,	PPCEFS,		{FRT, FRA, FRB}},
{"fa",		A(63,21,0),	AFRC_MASK,   PWRCOM,	PPCNONE,	{FRT, FRA, FRB}},
{"fadd.",	A(63,21,1),	AFRC_MASK,   PPCCOM,	PPCEFS,		{FRT, FRA, FRB}},
{"fa.",		A(63,21,1),	AFRC_MASK,   PWRCOM,	PPCNONE,	{FRT, FRA, FRB}},

{"fsqrt",	A(63,22,0),    AFRAFRC_MASK, PPCPWR2,	TITAN,  	{FRT, FRB}},
{"fsqrt.",	A(63,22,1),    AFRAFRC_MASK, PPCPWR2,	TITAN,  	{FRT, FRB}},

{"fsel",	A(63,23,0),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fsel.",	A(63,23,1),	A_MASK,      PPC,	PPCEFS,		{FRT, FRA, FRC, FRB}},

{"fre",		A(63,24,0),   AFRAFRC_MASK,  POWER7,	PPCNONE,	{FRT, FRB}},
{"fre",		A(63,24,0),   AFRALFRC_MASK, POWER5,	POWER7,		{FRT, FRB, A_L}},
{"fre.",	A(63,24,1),   AFRAFRC_MASK,  POWER7,	PPCNONE,	{FRT, FRB}},
{"fre.",	A(63,24,1),   AFRALFRC_MASK, POWER5,	POWER7,		{FRT, FRB, A_L}},

{"fmul",	A(63,25,0),	AFRB_MASK,   PPCCOM,	PPCEFS,		{FRT, FRA, FRC}},
{"fm",		A(63,25,0),	AFRB_MASK,   PWRCOM,	PPCNONE,	{FRT, FRA, FRC}},
{"fmul.",	A(63,25,1),	AFRB_MASK,   PPCCOM,	PPCEFS,		{FRT, FRA, FRC}},
{"fm.",		A(63,25,1),	AFRB_MASK,   PWRCOM,	PPCNONE,	{FRT, FRA, FRC}},

{"frsqrte",	A(63,26,0),   AFRAFRC_MASK,  POWER7,	PPCNONE,	{FRT, FRB}},
{"frsqrte",	A(63,26,0),   AFRALFRC_MASK, PPC,	POWER7,		{FRT, FRB, A_L}},
{"frsqrte.",	A(63,26,1),   AFRAFRC_MASK,  POWER7,	PPCNONE,	{FRT, FRB}},
{"frsqrte.",	A(63,26,1),   AFRALFRC_MASK, PPC,	POWER7,		{FRT, FRB, A_L}},

{"fmsub",	A(63,28,0),	A_MASK,      PPCCOM,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fms",		A(63,28,0),	A_MASK,      PWRCOM,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"fmsub.",	A(63,28,1),	A_MASK,      PPCCOM,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fms.",	A(63,28,1),	A_MASK,      PWRCOM,	PPCNONE,	{FRT, FRA, FRC, FRB}},

{"fmadd",	A(63,29,0),	A_MASK,      PPCCOM,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fma",		A(63,29,0),	A_MASK,      PWRCOM,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"fmadd.",	A(63,29,1),	A_MASK,      PPCCOM,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fma.",	A(63,29,1),	A_MASK,      PWRCOM,	PPCNONE,	{FRT, FRA, FRC, FRB}},

{"fnmsub",	A(63,30,0),	A_MASK,      PPCCOM,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fnms",	A(63,30,0),	A_MASK,      PWRCOM,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"fnmsub.",	A(63,30,1),	A_MASK,      PPCCOM,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fnms.",	A(63,30,1),	A_MASK,      PWRCOM,	PPCNONE,	{FRT, FRA, FRC, FRB}},

{"fnmadd",	A(63,31,0),	A_MASK,      PPCCOM,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fnma",	A(63,31,0),	A_MASK,      PWRCOM,	PPCNONE,	{FRT, FRA, FRC, FRB}},
{"fnmadd.",	A(63,31,1),	A_MASK,      PPCCOM,	PPCEFS,		{FRT, FRA, FRC, FRB}},
{"fnma.",	A(63,31,1),	A_MASK,      PWRCOM,	PPCNONE,	{FRT, FRA, FRC, FRB}},

{"fcmpo",	X(63,32),    X_MASK|(3<<21), COM,	PPCEFS,		{BF, FRA, FRB}},

{"dmulq",	XRC(63,34,0),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, FRBp}},
{"dmulq.",	XRC(63,34,1),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, FRBp}},

{"drrndq",	ZRC(63,35,0),	Z2_MASK,     POWER6,	PPCNONE,	{FRTp, FRA, FRBp, RMC}},
{"drrndq.",	ZRC(63,35,1),	Z2_MASK,     POWER6,	PPCNONE,	{FRTp, FRA, FRBp, RMC}},

{"mtfsb1",	XRC(63,38,0),	XRARB_MASK,  COM,	PPCNONE,	{BT}},
{"mtfsb1.",	XRC(63,38,1),	XRARB_MASK,  COM,	PPCNONE,	{BT}},

{"fneg",	XRC(63,40,0),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},
{"fneg.",	XRC(63,40,1),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},

{"mcrfs",      X(63,64), XRB_MASK|(3<<21)|(3<<16), COM,	PPCNONE,	{BF, BFA}},

{"dscliq",	ZRC(63,66,0),	Z_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, SH16}},
{"dscliq.",	ZRC(63,66,1),	Z_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, SH16}},

{"dquaiq",	ZRC(63,67,0),	Z2_MASK,     POWER6,	PPCNONE,	{TE, FRTp, FRBp, RMC}},
{"dquaiq.",	ZRC(63,67,1),	Z2_MASK,     POWER6,	PPCNONE,	{TE, FRTp, FRBp, RMC}},

{"mtfsb0",	XRC(63,70,0),	XRARB_MASK,  COM,	PPCNONE,	{BT}},
{"mtfsb0.",	XRC(63,70,1),	XRARB_MASK,  COM,	PPCNONE,	{BT}},

{"fmr",		XRC(63,72,0),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},
{"fmr.",	XRC(63,72,1),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},

{"dscriq",	ZRC(63,98,0),	Z_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, SH16}},
{"dscriq.",	ZRC(63,98,1),	Z_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, SH16}},

{"drintxq",	ZRC(63,99,0),	Z2_MASK,     POWER6,	PPCNONE,	{R, FRTp, FRBp, RMC}},
{"drintxq.",	ZRC(63,99,1),	Z2_MASK,     POWER6,	PPCNONE,	{R, FRTp, FRBp, RMC}},

{"ftdiv",	X(63,128),   X_MASK|(3<<21), POWER7,	PPCNONE,	{BF, FRA, FRB}},

{"dcmpoq",	X(63,130),	X_MASK,      POWER6,	PPCNONE,	{BF, FRAp, FRBp}},

{"mtfsfi",  XRC(63,134,0), XWRA_MASK|(3<<21)|(1<<11), POWER6|PPCA2|PPC476, PPCNONE, {BFF, U, W}},
{"mtfsfi",  XRC(63,134,0), XRA_MASK|(3<<21)|(1<<11), COM, POWER6|PPCA2|PPC476, {BFF, U}},
{"mtfsfi.", XRC(63,134,1), XWRA_MASK|(3<<21)|(1<<11), POWER6|PPCA2|PPC476, PPCNONE, {BFF, U, W}},
{"mtfsfi.", XRC(63,134,1), XRA_MASK|(3<<21)|(1<<11), COM, POWER6|PPCA2|PPC476, {BFF, U}},

{"fnabs",	XRC(63,136,0),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},
{"fnabs.",	XRC(63,136,1),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},

{"fctiwu",	XRC(63,142,0),	XRA_MASK,    POWER7,	PPCNONE,	{FRT, FRB}},
{"fctiwu.",	XRC(63,142,1),	XRA_MASK,    POWER7,	PPCNONE,	{FRT, FRB}},
{"fctiwuz",	XRC(63,143,0),	XRA_MASK,    POWER7,	PPCNONE,	{FRT, FRB}},
{"fctiwuz.",	XRC(63,143,1),	XRA_MASK,    POWER7,	PPCNONE,	{FRT, FRB}},

{"ftsqrt",	X(63,160), X_MASK|(3<<21|FRA_MASK), POWER7, PPCNONE,	{BF, FRB}},

{"dtstexq",	X(63,162),	X_MASK,      POWER6,	PPCNONE,	{BF, FRAp, FRBp}},
{"dtstdcq",	Z(63,194),	Z_MASK,      POWER6,	PPCNONE,	{BF, FRAp, DCM}},
{"dtstdgq",	Z(63,226),	Z_MASK,      POWER6,	PPCNONE,	{BF, FRAp, DGM}},

{"drintnq",	ZRC(63,227,0),	Z2_MASK,     POWER6,	PPCNONE,	{R, FRTp, FRBp, RMC}},
{"drintnq.",	ZRC(63,227,1),	Z2_MASK,     POWER6,	PPCNONE,	{R, FRTp, FRBp, RMC}},

{"dctqpq",	XRC(63,258,0),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRB}},
{"dctqpq.",	XRC(63,258,1),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRB}},

{"fabs",	XRC(63,264,0),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},
{"fabs.",	XRC(63,264,1),	XRA_MASK,    COM,	PPCEFS,		{FRT, FRB}},

{"dctfixq",	XRC(63,290,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRBp}},
{"dctfixq.",	XRC(63,290,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRBp}},

{"ddedpdq",	XRC(63,322,0),	X_MASK,      POWER6,	PPCNONE,	{SP, FRTp, FRBp}},
{"ddedpdq.",	XRC(63,322,1),	X_MASK,      POWER6,	PPCNONE,	{SP, FRTp, FRBp}},

{"dxexq",	XRC(63,354,0),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRBp}},
{"dxexq.",	XRC(63,354,1),	X_MASK,      POWER6,	PPCNONE,	{FRT, FRBp}},

{"frin",	XRC(63,392,0),	XRA_MASK,    POWER5,	PPCNONE,	{FRT, FRB}},
{"frin.",	XRC(63,392,1),	XRA_MASK,    POWER5,	PPCNONE,	{FRT, FRB}},
{"friz",	XRC(63,424,0),	XRA_MASK,    POWER5,	PPCNONE,	{FRT, FRB}},
{"friz.",	XRC(63,424,1),	XRA_MASK,    POWER5,	PPCNONE,	{FRT, FRB}},
{"frip",	XRC(63,456,0),	XRA_MASK,    POWER5,	PPCNONE,	{FRT, FRB}},
{"frip.",	XRC(63,456,1),	XRA_MASK,    POWER5,	PPCNONE,	{FRT, FRB}},
{"frim",	XRC(63,488,0),	XRA_MASK,    POWER5,	PPCNONE,	{FRT, FRB}},
{"frim.",	XRC(63,488,1),	XRA_MASK,    POWER5,	PPCNONE,	{FRT, FRB}},

{"dsubq",	XRC(63,514,0),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, FRBp}},
{"dsubq.",	XRC(63,514,1),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, FRBp}},

{"ddivq",	XRC(63,546,0),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, FRBp}},
{"ddivq.",	XRC(63,546,1),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRAp, FRBp}},

{"mffs",	XRC(63,583,0),	XRARB_MASK,  COM,	PPCEFS,		{FRT}},
{"mffs.",	XRC(63,583,1),	XRARB_MASK,  COM,	PPCEFS,		{FRT}},

{"dcmpuq",	X(63,642),	X_MASK,      POWER6,	PPCNONE,	{BF, FRAp, FRBp}},

{"dtstsfq",	X(63,674),	X_MASK,      POWER6,	PPCNONE,	{BF, FRA, FRBp}},

{"mtfsf",	XFL(63,711,0),	XFL_MASK, POWER6|PPCA2|PPC476, PPCNONE,	{FLM, FRB, XFL_L, W}},
{"mtfsf",	XFL(63,711,0),	XFL_MASK,    COM, POWER6|PPCA2|PPC476|PPCEFS,	{FLM, FRB}},
{"mtfsf.",	XFL(63,711,1),	XFL_MASK, POWER6|PPCA2|PPC476, PPCNONE,	{FLM, FRB, XFL_L, W}},
{"mtfsf.",	XFL(63,711,1),	XFL_MASK,    COM, POWER6|PPCA2|PPC476|PPCEFS,	{FLM, FRB}},

{"drdpq",	XRC(63,770,0),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRBp}},
{"drdpq.",	XRC(63,770,1),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRBp}},

{"dcffixq",	XRC(63,802,0),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRB}},
{"dcffixq.",	XRC(63,802,1),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRB}},

{"fctid",	XRC(63,814,0),	XRA_MASK,    PPC64,	PPCNONE,	{FRT, FRB}},
{"fctid",	XRC(63,814,0),	XRA_MASK,    PPC476,	PPCNONE,	{FRT, FRB}},
{"fctid.",	XRC(63,814,1),	XRA_MASK,    PPC64,	PPCNONE,	{FRT, FRB}},
{"fctid.",	XRC(63,814,1),	XRA_MASK,    PPC476,	PPCNONE,	{FRT, FRB}},

{"fctidz",	XRC(63,815,0),	XRA_MASK,    PPC64,	PPCNONE,	{FRT, FRB}},
{"fctidz",	XRC(63,815,0),	XRA_MASK,    PPC476,	PPCNONE,	{FRT, FRB}},
{"fctidz.",	XRC(63,815,1),	XRA_MASK,    PPC64,	PPCNONE,	{FRT, FRB}},
{"fctidz.",	XRC(63,815,1),	XRA_MASK,    PPC476,	PPCNONE,	{FRT, FRB}},

{"denbcdq",	XRC(63,834,0),	X_MASK,      POWER6,	PPCNONE,	{S, FRTp, FRBp}},
{"denbcdq.",	XRC(63,834,1),	X_MASK,      POWER6,	PPCNONE,	{S, FRTp, FRBp}},

{"fmrgow",	X(63,838),	X_MASK,      PPCVSX2,	PPCNONE,	{FRT, FRA, FRB}},

{"fcfid",	XRC(63,846,0),	XRA_MASK,    PPC64,	PPCNONE,	{FRT, FRB}},
{"fcfid",	XRC(63,846,0),	XRA_MASK,    PPC476,	PPCNONE,	{FRT, FRB}},
{"fcfid.",	XRC(63,846,1),	XRA_MASK,    PPC64,	PPCNONE,	{FRT, FRB}},
{"fcfid.",	XRC(63,846,1),	XRA_MASK,    PPC476,	PPCNONE,	{FRT, FRB}},

{"diexq",	XRC(63,866,0),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRA, FRBp}},
{"diexq.",	XRC(63,866,1),	X_MASK,      POWER6,	PPCNONE,	{FRTp, FRA, FRBp}},

{"fctidu",	XRC(63,942,0),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},
{"fctidu.",	XRC(63,942,1),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},

{"fctiduz",	XRC(63,943,0),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},
{"fctiduz.",	XRC(63,943,1),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},

{"fmrgew",	X(63,966),	X_MASK,      PPCVSX2,	PPCNONE,	{FRT, FRA, FRB}},

{"fcfidu",	XRC(63,974,0),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},
{"fcfidu.",	XRC(63,974,1),	XRA_MASK, POWER7|PPCA2,	PPCNONE,	{FRT, FRB}},
};

const int powerpc_num_opcodes =
  sizeof (powerpc_opcodes) / sizeof (powerpc_opcodes[0]);

/* The VLE opcode table.

   The format of this opcode table is the same as the main opcode table.  */

const struct powerpc_opcode vle_opcodes[] = {

{"se_illegal",	C(0),		C_MASK,		PPCVLE,	PPCNONE,	{}},
{"se_isync",	C(1),		C_MASK,		PPCVLE,	PPCNONE,	{}},
{"se_sc",	C(2),		C_MASK,		PPCVLE,	PPCNONE,	{}},
{"se_blr",	C_LK(2,0),	C_LK_MASK,	PPCVLE,	PPCNONE,	{}},
{"se_blrl",	C_LK(2,1),	C_LK_MASK,	PPCVLE,	PPCNONE,	{}},
{"se_bctr",	C_LK(3,0),	C_LK_MASK,	PPCVLE,	PPCNONE,	{}},
{"se_bctrl",	C_LK(3,1),	C_LK_MASK,	PPCVLE,	PPCNONE,	{}},
{"se_rfi",	C(8),		C_MASK,		PPCVLE,	PPCNONE,	{}},
{"se_rfci",	C(9),		C_MASK,		PPCVLE,	PPCNONE,	{}},
{"se_rfdi",	C(10),		C_MASK,		PPCVLE,	PPCNONE,	{}},
{"se_rfmci",	C(11),		C_MASK,		PPCVLE,	PPCNONE,	{}},
{"se_not",	SE_R(0,2),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_neg",	SE_R(0,3),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_mflr",	SE_R(0,8),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_mtlr",	SE_R(0,9),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_mfctr",	SE_R(0,10),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_mtctr",	SE_R(0,11),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_extzb",	SE_R(0,12),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_extsb",	SE_R(0,13),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_extzh",	SE_R(0,14),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_extsh",	SE_R(0,15),	SE_R_MASK,	PPCVLE,	PPCNONE,	{RX}},
{"se_mr",	SE_RR(0,1),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_mtar",	SE_RR(0,2),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{ARX, RY}},
{"se_mfar",	SE_RR(0,3),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, ARY}},
{"se_add",	SE_RR(1,0),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_mullw",	SE_RR(1,1),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_sub",	SE_RR(1,2),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_subf",	SE_RR(1,3),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_cmp",	SE_RR(3,0),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_cmpl",	SE_RR(3,1),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_cmph",	SE_RR(3,2),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_cmphl",	SE_RR(3,3),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
 
{"e_cmpi",	SCI8BF(6,0,21),	SCI8BF_MASK,	PPCVLE,	PPCNONE,	{CRD32, RA, SCLSCI8}},
{"e_cmpli",	SCI8BF(6,1,21),	SCI8BF_MASK,	PPCVLE,	PPCNONE,	{CRD32, RA, SCLSCI8}},
{"e_addi",	SCI8(6,16),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8}},
{"e_subi",	SCI8(6,16),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8N}},
{"e_addi.",	SCI8(6,17),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8}},
{"e_addic",	SCI8(6,18),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8}},
{"e_subic",	SCI8(6,18),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8N}},
{"e_addic.",	SCI8(6,19),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8}},
{"e_subic.",	SCI8(6,19),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8N}},
{"e_mulli",	SCI8(6,20),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8}},
{"e_subfic",	SCI8(6,22),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8}},
{"e_subfic.",	SCI8(6,23),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SCLSCI8}},
{"e_andi",	SCI8(6,24),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SCLSCI8}},
{"e_andi.",	SCI8(6,25),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SCLSCI8}},
{"e_nop",	SCI8(6,26),	0xffffffff,	PPCVLE,	PPCNONE,	{0}},
{"e_ori",	SCI8(6,26),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SCLSCI8}},
{"e_ori.",	SCI8(6,27),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SCLSCI8}},
{"e_xori",	SCI8(6,28),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SCLSCI8}},
{"e_xori.",	SCI8(6,29),	SCI8_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SCLSCI8}},
{"e_lbzu",	OPVUP(6,0),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_lhau",	OPVUP(6,3),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_lhzu",	OPVUP(6,1),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_lmw",	OPVUP(6,8),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_lwzu",	OPVUP(6,2),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_stbu",	OPVUP(6,4),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_sthu",	OPVUP(6,5),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_stwu",	OPVUP(6,6),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_stmw",	OPVUP(6,9),	OPVUP_MASK,	PPCVLE,	PPCNONE,	{RT, D8, RA0}},
{"e_add16i",	OP(7),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, RA, SI}},
{"e_la",	OP(7),		OP_MASK,    	PPCVLE,	PPCNONE,	{RT, D, RA0}},
{"e_sub16i",	OP(7),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, RA, NSI}},

{"se_addi",	SE_IM5(8,0),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, OIMM5}},
{"se_cmpli",	SE_IM5(8,1),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, OIMM5}},
{"se_subi",	SE_IM5(9,0),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, OIMM5}},
{"se_subi.",	SE_IM5(9,1),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, OIMM5}},
{"se_cmpi",	SE_IM5(10,1),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
{"se_bmaski",	SE_IM5(11,0),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
{"se_andi",	SE_IM5(11,1),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
 
{"e_lbz",	OP(12),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, D, RA0}},
{"e_stb",	OP(13),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, D, RA0}},
{"e_lha",	OP(14),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, D, RA0}},

{"se_srw",	SE_RR(16,0),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_sraw",	SE_RR(16,1),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_slw",	SE_RR(16,2),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_nop",	SE_RR(17,0),	0xffff,		PPCVLE,	PPCNONE,	{0}},
{"se_or",	SE_RR(17,0),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_andc",	SE_RR(17,1),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_and",	SE_RR(17,2),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_and.",	SE_RR(17,3),	SE_RR_MASK,	PPCVLE,	PPCNONE,	{RX, RY}},
{"se_li",	IM7(9),		IM7_MASK,	PPCVLE,	PPCNONE,	{RX, UI7}},
 
{"e_lwz",	OP(20),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, D, RA0}},
{"e_stw",	OP(21),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, D, RA0}},
{"e_lhz",	OP(22),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, D, RA0}},
{"e_sth",	OP(23),		OP_MASK,	PPCVLE,	PPCNONE,	{RT, D, RA0}},

{"se_bclri",	SE_IM5(24,0),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
{"se_bgeni",	SE_IM5(24,1),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
{"se_bseti",	SE_IM5(25,0),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
{"se_btsti",	SE_IM5(25,1),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
{"se_srwi",	SE_IM5(26,0),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
{"se_srawi",	SE_IM5(26,1),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},
{"se_slwi",	SE_IM5(27,0),	SE_IM5_MASK,	PPCVLE,	PPCNONE,	{RX, UI5}},

{"e_lis",	I16L(28,28),	I16L_MASK,	PPCVLE,	PPCNONE,	{RD, VLEUIMML}},
{"e_and2is.",	I16L(28,29),	I16L_MASK,	PPCVLE,	PPCNONE,	{RD, VLEUIMML}},
{"e_or2is",	I16L(28,26),	I16L_MASK,	PPCVLE,	PPCNONE,	{RD, VLEUIMML}},
{"e_and2i.",	I16L(28,25),	I16L_MASK,	PPCVLE,	PPCNONE,	{RD, VLEUIMML}},
{"e_or2i",	I16L(28,24),	I16L_MASK,	PPCVLE,	PPCNONE,	{RD, VLEUIMML}},
{"e_cmphl16i",	IA16(28,23),	IA16_MASK,	PPCVLE,	PPCNONE,	{RA, VLEUIMM}},
{"e_cmph16i",	IA16(28,22),	IA16_MASK,	PPCVLE,	PPCNONE,	{RA, VLESIMM}},
{"e_cmpl16i",	I16A(28,21),	I16A_MASK,	PPCVLE,	PPCNONE,	{RA, VLEUIMM}},
{"e_cmplwi",	I16A(28,21),	I16A_MASK,	PPCVLE,	PPCNONE,	{RA, VLESIMM}},
{"e_mull2i",	I16A(28,20),	I16A_MASK,	PPCVLE,	PPCNONE,	{RA, VLESIMM}},
{"e_cmp16i",	IA16(28,19),	IA16_MASK,	PPCVLE,	PPCNONE,	{RA, VLESIMM}},
{"e_cmpwi",	IA16(28,19),	IA16_MASK,	PPCVLE,	PPCNONE,	{RA, VLESIMM}},
{"e_sub2is",	I16A(28,18),	I16A_MASK,	PPCVLE,	PPCNONE,	{RA, VLENSIMM}},
{"e_add2is",	I16A(28,18),	I16A_MASK,	PPCVLE,	PPCNONE,	{RA, VLESIMM}},
{"e_sub2i.",	I16A(28,17),	I16A_MASK,	PPCVLE,	PPCNONE,	{RA, VLENSIMM}},
{"e_add2i.",	I16A(28,17),	I16A_MASK,	PPCVLE,	PPCNONE,	{RA, VLESIMM}},
{"e_li",	LI20(28,0),	LI20_MASK,	PPCVLE,	PPCNONE,	{RT, IMM20}},
{"e_rlwimi",	M(29,0),	M_MASK,		PPCVLE,	PPCNONE,	{RA, RS, SH, MB, ME}},
{"e_rlwinm",	M(29,1),	M_MASK,		PPCVLE,	PPCNONE,	{RA, RT, SH, MBE, ME}},
{"e_b",		BD24(30,0,0),	BD24_MASK,	PPCVLE,	PPCNONE,	{B24}},
{"e_bl",	BD24(30,0,1),	BD24_MASK,	PPCVLE,	PPCNONE,	{B24}},
{"e_bdnz",	EBD15(30,8,BO32DNZ,0),	EBD15_MASK, PPCVLE, PPCNONE,	{B15}},
{"e_bdnzl",	EBD15(30,8,BO32DNZ,1),	EBD15_MASK, PPCVLE, PPCNONE,	{B15}},
{"e_bdz",	EBD15(30,8,BO32DZ,0),	EBD15_MASK, PPCVLE, PPCNONE,	{B15}},
{"e_bdzl",	EBD15(30,8,BO32DZ,1),	EBD15_MASK, PPCVLE, PPCNONE,	{B15}},
{"e_bge",	EBD15BI(30,8,BO32F,CBLT,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bgel",	EBD15BI(30,8,BO32F,CBLT,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bnl",	EBD15BI(30,8,BO32F,CBLT,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bnll",	EBD15BI(30,8,BO32F,CBLT,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_blt",	EBD15BI(30,8,BO32T,CBLT,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bltl",	EBD15BI(30,8,BO32T,CBLT,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bgt",	EBD15BI(30,8,BO32T,CBGT,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bgtl",	EBD15BI(30,8,BO32T,CBGT,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_ble",	EBD15BI(30,8,BO32F,CBGT,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_blel",	EBD15BI(30,8,BO32F,CBGT,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bng",	EBD15BI(30,8,BO32F,CBGT,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bngl",	EBD15BI(30,8,BO32F,CBGT,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bne",	EBD15BI(30,8,BO32F,CBEQ,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bnel",	EBD15BI(30,8,BO32F,CBEQ,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_beq",	EBD15BI(30,8,BO32T,CBEQ,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_beql",	EBD15BI(30,8,BO32T,CBEQ,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bso",	EBD15BI(30,8,BO32T,CBSO,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bsol",	EBD15BI(30,8,BO32T,CBSO,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bun",	EBD15BI(30,8,BO32T,CBSO,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bunl",	EBD15BI(30,8,BO32T,CBSO,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bns",	EBD15BI(30,8,BO32F,CBSO,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bnsl",	EBD15BI(30,8,BO32F,CBSO,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bnu",	EBD15BI(30,8,BO32F,CBSO,0), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bnul",	EBD15BI(30,8,BO32F,CBSO,1), EBD15BI_MASK, PPCVLE, PPCNONE, {CRS,B15}},
{"e_bc",	BD15(30,8,0),	BD15_MASK,	PPCVLE,	PPCNONE,	{BO32, BI32, B15}},
{"e_bcl",	BD15(30,8,1),	BD15_MASK,	PPCVLE,	PPCNONE,	{BO32, BI32, B15}},

{"e_bf",	EBD15(30,8,BO32F,0), EBD15_MASK, PPCVLE, PPCNONE,	{BI32,B15}},
{"e_bfl",	EBD15(30,8,BO32F,1), EBD15_MASK, PPCVLE, PPCNONE,	{BI32,B15}},
{"e_bt",	EBD15(30,8,BO32T,0), EBD15_MASK, PPCVLE, PPCNONE,	{BI32,B15}},
{"e_btl",	EBD15(30,8,BO32T,1), EBD15_MASK, PPCVLE, PPCNONE,	{BI32,B15}},
 
{"e_cmph",	X(31,14),	X_MASK,		PPCVLE,	PPCNONE,	{CRD, RA, RB}},
{"e_cmphl",	X(31,46),	X_MASK,		PPCVLE,	PPCNONE,	{CRD, RA, RB}},
{"e_crandc",	XL(31,129),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BB}},
{"e_crnand",	XL(31,225),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BB}},
{"e_crnot",	XL(31,33),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BBA}},
{"e_crnor",	XL(31,33),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BB}},
{"e_crclr",	XL(31,193),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BAT, BBA}},
{"e_crxor",	XL(31,193),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BB}},
{"e_mcrf",	XL(31,16),	XL_MASK,	PPCVLE,	PPCNONE,	{CRD, CR}},
{"e_slwi",	EX(31,112),	EX_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SH}},
{"e_slwi.",	EX(31,113),	EX_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SH}},
 
{"e_crand",	XL(31,257),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BB}},
 
{"e_rlw",	EX(31,560),	EX_MASK,	PPCVLE,	PPCNONE,	{RA, RS, RB}},
{"e_rlw.",	EX(31,561),	EX_MASK,	PPCVLE,	PPCNONE,	{RA, RS, RB}},
 
{"e_crset",	XL(31,289),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BAT, BBA}},
{"e_creqv",	XL(31,289),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BB}},
 
{"e_rlwi",	EX(31,624),	EX_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SH}},
{"e_rlwi.",	EX(31,625),	EX_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SH}},
 
{"e_crorc",	XL(31,417),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BB}},

{"e_crmove",	XL(31,449),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BBA}},
{"e_cror",	XL(31,449),	XL_MASK,	PPCVLE,	PPCNONE,	{BT, BA, BB}},

{"mtmas1",	XSPR(31,467,625), XSPR_MASK,	PPCVLE,	PPCNONE,	{RS}},

{"e_srwi",	EX(31,1136),	EX_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SH}},
{"e_srwi.",	EX(31,1137),	EX_MASK,	PPCVLE,	PPCNONE,	{RA, RS, SH}},
 
{"se_lbz",	SD4(8),		SD4_MASK,	PPCVLE,	PPCNONE,	{RZ, SE_SD, RX}},
 
{"se_stb",	SD4(9),		SD4_MASK,	PPCVLE,	PPCNONE,	{RZ, SE_SD, RX}},
 
{"se_lhz",	SD4(10),	SD4_MASK,	PPCVLE,	PPCNONE,	{RZ, SE_SDH, RX}},
 
{"se_sth",	SD4(11),	SD4_MASK,	PPCVLE,	PPCNONE,	{RZ, SE_SDH, RX}},
 
{"se_lwz",	SD4(12),	SD4_MASK,	PPCVLE,	PPCNONE,	{RZ, SE_SDW, RX}},
 
{"se_stw",	SD4(13),	SD4_MASK,	PPCVLE,	PPCNONE,	{RZ, SE_SDW, RX}},
 
{"se_bge",	EBD8IO(28,0,0),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bnl",	EBD8IO(28,0,0),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_ble",	EBD8IO(28,0,1),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bng",	EBD8IO(28,0,1),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bne",	EBD8IO(28,0,2),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bns",	EBD8IO(28,0,3),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bnu",	EBD8IO(28,0,3),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bf",	EBD8IO(28,0,0),	EBD8IO2_MASK,	PPCVLE,	PPCNONE,	{BI16, B8}},
{"se_blt",	EBD8IO(28,1,0),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bgt",	EBD8IO(28,1,1),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_beq",	EBD8IO(28,1,2),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bso",	EBD8IO(28,1,3),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bun",	EBD8IO(28,1,3),	EBD8IO3_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bt",	EBD8IO(28,1,0),	EBD8IO2_MASK,	PPCVLE,	PPCNONE,	{BI16, B8}},
{"se_bc",	BD8IO(28),	BD8IO_MASK,	PPCVLE,	PPCNONE,	{BO16, BI16, B8}},
{"se_b",	BD8(58,0,0),	BD8_MASK,	PPCVLE,	PPCNONE,	{B8}},
{"se_bl",	BD8(58,0,1),	BD8_MASK,	PPCVLE,	PPCNONE,	{B8}},
};

const int vle_num_opcodes =
  sizeof (vle_opcodes) / sizeof (vle_opcodes[0]);

/* The macro table.  This is only used by the assembler.  */

/* The expressions of the form (-x ! 31) & (x | 31) have the value 0
   when x=0; 32-x when x is between 1 and 31; are negative if x is
   negative; and are 32 or more otherwise.  This is what you want
   when, for instance, you are emulating a right shift by a
   rotate-left-and-mask, because the underlying instructions support
   shifts of size 0 but not shifts of size 32.  By comparison, when
   extracting x bits from some word you want to use just 32-x, because
   the underlying instructions don't support extracting 0 bits but do
   support extracting the whole word (32 bits in this case).  */

const struct powerpc_macro powerpc_macros[] = {
{"extldi",   4,	PPC64,	"rldicr %0,%1,%3,(%2)-1"},
{"extldi.",  4,	PPC64,	"rldicr. %0,%1,%3,(%2)-1"},
{"extrdi",   4,	PPC64,	"rldicl %0,%1,((%2)+(%3))&((%2)+(%3)<>64),64-(%2)"},
{"extrdi.",  4,	PPC64,	"rldicl. %0,%1,((%2)+(%3))&((%2)+(%3)<>64),64-(%2)"},
{"insrdi",   4,	PPC64,	"rldimi %0,%1,64-((%2)+(%3)),%3"},
{"insrdi.",  4,	PPC64,	"rldimi. %0,%1,64-((%2)+(%3)),%3"},
{"rotrdi",   3,	PPC64,	"rldicl %0,%1,(-(%2)!63)&((%2)|63),0"},
{"rotrdi.",  3,	PPC64,	"rldicl. %0,%1,(-(%2)!63)&((%2)|63),0"},
{"sldi",     3,	PPC64,	"rldicr %0,%1,%2,63-(%2)"},
{"sldi.",    3,	PPC64,	"rldicr. %0,%1,%2,63-(%2)"},
{"srdi",     3,	PPC64,	"rldicl %0,%1,(-(%2)!63)&((%2)|63),%2"},
{"srdi.",    3,	PPC64,	"rldicl. %0,%1,(-(%2)!63)&((%2)|63),%2"},
{"clrrdi",   3,	PPC64,	"rldicr %0,%1,0,63-(%2)"},
{"clrrdi.",  3,	PPC64,	"rldicr. %0,%1,0,63-(%2)"},
{"clrlsldi", 4,	PPC64,	"rldic %0,%1,%3,(%2)-(%3)"},
{"clrlsldi.",4, PPC64,	"rldic. %0,%1,%3,(%2)-(%3)"},

{"extlwi",   4,	PPCCOM,	"rlwinm %0,%1,%3,0,(%2)-1"},
{"extlwi.",  4,	PPCCOM,	"rlwinm. %0,%1,%3,0,(%2)-1"},
{"extrwi",   4,	PPCCOM,	"rlwinm %0,%1,((%2)+(%3))&((%2)+(%3)<>32),32-(%2),31"},
{"extrwi.",  4,	PPCCOM,	"rlwinm. %0,%1,((%2)+(%3))&((%2)+(%3)<>32),32-(%2),31"},
{"inslwi",   4,	PPCCOM,	"rlwimi %0,%1,(-(%3)!31)&((%3)|31),%3,(%2)+(%3)-1"},
{"inslwi.",  4,	PPCCOM,	"rlwimi. %0,%1,(-(%3)!31)&((%3)|31),%3,(%2)+(%3)-1"},
{"insrwi",   4,	PPCCOM,	"rlwimi %0,%1,32-((%2)+(%3)),%3,(%2)+(%3)-1"},
{"insrwi.",  4,	PPCCOM,	"rlwimi. %0,%1,32-((%2)+(%3)),%3,(%2)+(%3)-1"},
{"rotrwi",   3,	PPCCOM,	"rlwinm %0,%1,(-(%2)!31)&((%2)|31),0,31"},
{"rotrwi.",  3,	PPCCOM,	"rlwinm. %0,%1,(-(%2)!31)&((%2)|31),0,31"},
{"slwi",     3,	PPCCOM,	"rlwinm %0,%1,%2,0,31-(%2)"},
{"sli",      3,	PWRCOM,	"rlinm %0,%1,%2,0,31-(%2)"},
{"slwi.",    3,	PPCCOM,	"rlwinm. %0,%1,%2,0,31-(%2)"},
{"sli.",     3,	PWRCOM,	"rlinm. %0,%1,%2,0,31-(%2)"},
{"srwi",     3,	PPCCOM,	"rlwinm %0,%1,(-(%2)!31)&((%2)|31),%2,31"},
{"sri",      3,	PWRCOM,	"rlinm %0,%1,(-(%2)!31)&((%2)|31),%2,31"},
{"srwi.",    3,	PPCCOM,	"rlwinm. %0,%1,(-(%2)!31)&((%2)|31),%2,31"},
{"sri.",     3,	PWRCOM,	"rlinm. %0,%1,(-(%2)!31)&((%2)|31),%2,31"},
{"clrrwi",   3,	PPCCOM,	"rlwinm %0,%1,0,0,31-(%2)"},
{"clrrwi.",  3,	PPCCOM,	"rlwinm. %0,%1,0,0,31-(%2)"},
{"clrlslwi", 4,	PPCCOM,	"rlwinm %0,%1,%3,(%2)-(%3),31-(%3)"},
{"clrlslwi.",4, PPCCOM,	"rlwinm. %0,%1,%3,(%2)-(%3),31-(%3)"},

{"e_extlwi", 4,	PPCVLE, "e_rlwinm %0,%1,%3,0,(%2)-1"},
{"e_extrwi", 4,	PPCVLE, "e_rlwinm %0,%1,((%2)+(%3))&((%2)+(%3)<>32),32-(%2),31"},
{"e_inslwi", 4,	PPCVLE, "e_rlwimi %0,%1,(-(%3)!31)&((%3)|31),%3,(%2)+(%3)-1"},
{"e_insrwi", 4,	PPCVLE, "e_rlwimi %0,%1,32-((%2)+(%3)),%3,(%2)+(%3)-1"},
{"e_rotlwi", 3,	PPCVLE, "e_rlwinm %0,%1,%2,0,31"},
{"e_rotrwi", 3,	PPCVLE, "e_rlwinm %0,%1,(-(%2)!31)&((%2)|31),0,31"},
{"e_slwi",   3,	PPCVLE, "e_rlwinm %0,%1,%2,0,31-(%2)"},
{"e_srwi",   3,	PPCVLE, "e_rlwinm %0,%1,(-(%2)!31)&((%2)|31),%2,31"},
{"e_clrlwi", 3,	PPCVLE, "e_rlwinm %0,%1,0,%2,31"},
{"e_clrrwi", 3,	PPCVLE, "e_rlwinm %0,%1,0,0,31-(%2)"},
{"e_clrlslwi",4, PPCVLE, "e_rlwinm %0,%1,%3,(%2)-(%3),31-(%3)"},
};

const int powerpc_num_macros =
  sizeof (powerpc_macros) / sizeof (powerpc_macros[0]);
