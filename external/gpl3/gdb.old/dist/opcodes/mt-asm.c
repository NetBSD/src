/* DO NOT EDIT!  -*- buffer-read-only: t -*- vi:set ro:  */
/* Assembler interface for targets using CGEN. -*- C -*-
   CGEN: Cpu tools GENerator

   THIS FILE IS MACHINE GENERATED WITH CGEN.
   - the resultant file is machine generated, cgen-asm.in isn't

   Copyright (C) 1996-2020 Free Software Foundation, Inc.

   This file is part of libopcodes.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */


/* ??? Eventually more and more of this stuff can go to cpu-independent files.
   Keep that in mind.  */

#include "sysdep.h"
#include <stdio.h>
#include "ansidecl.h"
#include "bfd.h"
#include "symcat.h"
#include "mt-desc.h"
#include "mt-opc.h"
#include "opintl.h"
#include "xregex.h"
#include "libiberty.h"
#include "safe-ctype.h"

#undef  min
#define min(a,b) ((a) < (b) ? (a) : (b))
#undef  max
#define max(a,b) ((a) > (b) ? (a) : (b))

static const char * parse_insn_normal
  (CGEN_CPU_DESC, const CGEN_INSN *, const char **, CGEN_FIELDS *);

/* -- assembler routines inserted here.  */

/* -- asm.c */
/* Range checking for signed numbers.  Returns 0 if acceptable
   and 1 if the value is out of bounds for a signed quantity.  */

static int
signed_out_of_bounds (long val)
{
  if ((val < -32768) || (val > 32767))
    return 1;
  return 0;
}

static const char *
parse_loopsize (CGEN_CPU_DESC cd,
		const char **strp,
		int opindex,
		void *arg)
{
  signed long * valuep = (signed long *) arg;
  const char *errmsg;
  bfd_reloc_code_real_type code = BFD_RELOC_NONE;
  enum cgen_parse_operand_result result_type;
  bfd_vma value;

  /* Is it a control transfer instructions?  */
  if (opindex == (CGEN_OPERAND_TYPE) MT_OPERAND_LOOPSIZE)
    {
      code = BFD_RELOC_MT_PCINSN8;
      errmsg = cgen_parse_address (cd, strp, opindex, code,
                                   & result_type, & value);
      *valuep = value;
      return errmsg;
    }

  abort ();
}

static const char *
parse_imm16 (CGEN_CPU_DESC cd,
	     const char **strp,
	     int opindex,
	     void *arg)
{
  signed long * valuep = (signed long *) arg;
  const char *errmsg;
  enum cgen_parse_operand_result result_type;
  bfd_reloc_code_real_type code = BFD_RELOC_NONE;
  bfd_vma value;

  /* Is it a control transfer instructions?  */
  if (opindex == (CGEN_OPERAND_TYPE) MT_OPERAND_IMM16O)
    {
      code = BFD_RELOC_16_PCREL;
      errmsg = cgen_parse_address (cd, strp, opindex, code,
                                   & result_type, & value);
      if (errmsg == NULL)
	{
	  if (signed_out_of_bounds (value))
	    errmsg = _("Operand out of range. Must be between -32768 and 32767.");
	}
      *valuep = value;
      return errmsg;
    }

  /* If it's not a control transfer instruction, then
     we have to check for %OP relocating operators.  */
  if (opindex == (CGEN_OPERAND_TYPE) MT_OPERAND_IMM16L)
    ;
  else if (strncmp (*strp, "%hi16", 5) == 0)
    {
      *strp += 5;
      code = BFD_RELOC_HI16;
    }
  else if (strncmp (*strp, "%lo16", 5) == 0)
    {
      *strp += 5;
      code = BFD_RELOC_LO16;
    }

  /* If we found a %OP relocating operator, then parse it as an address.
     If not, we need to parse it as an integer, either signed or unsigned
     depending on which operand type we have.  */
  if (code != BFD_RELOC_NONE)
    {
       /* %OP relocating operator found.  */
       errmsg = cgen_parse_address (cd, strp, opindex, code,
                                   & result_type, & value);
       if (errmsg == NULL)
	 {
           switch (result_type)
	     {
	     case (CGEN_PARSE_OPERAND_RESULT_NUMBER):
	       if (code == BFD_RELOC_HI16)
		 value = (value >> 16) & 0xFFFF;
	       else if (code == BFD_RELOC_LO16)
		 value = value  & 0xFFFF;
	       else
		 errmsg = _("Biiiig Trouble in parse_imm16!");
	       break;

	     case (CGEN_PARSE_OPERAND_RESULT_QUEUED):
	       /* No special processing for this case.  */
	       break;

	     default:
	       errmsg = _("The percent-operator's operand is not a symbol");
	       break;
             }
	 }
       *valuep = value;
    }
  else
    {
      /* Parse hex values like 0xffff as unsigned, and sign extend
	 them manually.  */
      int parse_signed = (opindex == (CGEN_OPERAND_TYPE)MT_OPERAND_IMM16);

      if ((*strp)[0] == '0'
	  && ((*strp)[1] == 'x' || (*strp)[1] == 'X'))
	parse_signed = 0;

      /* No relocating operator.  Parse as an number.  */
      if (parse_signed)
	{
          /* Parse as as signed integer.  */

          errmsg = cgen_parse_signed_integer (cd, strp, opindex, valuep);

          if (errmsg == NULL)
	    {
#if 0
	      /* Manual range checking is needed for the signed case.  */
	      if (*valuep & 0x8000)
                value = 0xffff0000 | *valuep;
	      else
                value = *valuep;

	      if (signed_out_of_bounds (value))
	        errmsg = _("Operand out of range. Must be between -32768 and 32767.");
	      /* Truncate to 16 bits. This is necessary
		 because cgen will have sign extended *valuep.  */
	      *valuep &= 0xFFFF;
#endif
	    }
	}
      else
	{
          /* MT_OPERAND_IMM16Z.  Parse as an unsigned integer.  */
          errmsg = cgen_parse_unsigned_integer (cd, strp, opindex, (unsigned long *) valuep);

	  if (opindex == (CGEN_OPERAND_TYPE) MT_OPERAND_IMM16
	      && *valuep >= 0x8000
	      && *valuep <= 0xffff)
	    *valuep -= 0x10000;
	}
    }

  return errmsg;
}


static const char *
parse_dup (CGEN_CPU_DESC cd,
	   const char **strp,
	   int opindex,
	   unsigned long *valuep)
{
  const char *errmsg = NULL;

  if (strncmp (*strp, "dup", 3) == 0 || strncmp (*strp, "DUP", 3) == 0)
    {
      *strp += 3;
      *valuep = 1;
    }
  else if (strncmp (*strp, "xx", 2) == 0 || strncmp (*strp, "XX", 2) == 0)
    {
      *strp += 2;
      *valuep = 0;
    }
  else
    errmsg = cgen_parse_unsigned_integer (cd, strp, opindex, valuep);

  return errmsg;
}


static const char *
parse_ball (CGEN_CPU_DESC cd,
	    const char **strp,
	    int opindex,
	    unsigned long *valuep)
{
  const char *errmsg = NULL;

  if (strncmp (*strp, "all", 3) == 0 || strncmp (*strp, "ALL", 3) == 0)
    {
      *strp += 3;
      *valuep = 1;
    }
  else if (strncmp (*strp, "one", 3) == 0 || strncmp (*strp, "ONE", 3) == 0)
    {
      *strp += 3;
      *valuep = 0;
    }
  else
    errmsg = cgen_parse_unsigned_integer (cd, strp, opindex, valuep);

  return errmsg;
}

static const char *
parse_xmode (CGEN_CPU_DESC cd,
	     const char **strp,
	     int opindex,
	     unsigned long *valuep)
{
  const char *errmsg = NULL;

  if (strncmp (*strp, "pm", 2) == 0 || strncmp (*strp, "PM", 2) == 0)
    {
      *strp += 2;
      *valuep = 1;
    }
  else if (strncmp (*strp, "xm", 2) == 0 || strncmp (*strp, "XM", 2) == 0)
    {
      *strp += 2;
      *valuep = 0;
    }
  else
    errmsg = cgen_parse_unsigned_integer (cd, strp, opindex, valuep);

  return errmsg;
}

static const char *
parse_rc (CGEN_CPU_DESC cd,
	  const char **strp,
	  int opindex,
	  unsigned long *valuep)
{
  const char *errmsg = NULL;

  if (strncmp (*strp, "r", 1) == 0 || strncmp (*strp, "R", 1) == 0)
    {
      *strp += 1;
      *valuep = 1;
    }
  else if (strncmp (*strp, "c", 1) == 0 || strncmp (*strp, "C", 1) == 0)
    {
      *strp += 1;
      *valuep = 0;
    }
  else
    errmsg = cgen_parse_unsigned_integer (cd, strp, opindex, valuep);

  return errmsg;
}

static const char *
parse_cbrb (CGEN_CPU_DESC cd,
	    const char **strp,
	    int opindex,
	    unsigned long *valuep)
{
  const char *errmsg = NULL;

  if (strncmp (*strp, "rb", 2) == 0 || strncmp (*strp, "RB", 2) == 0)
    {
      *strp += 2;
      *valuep = 1;
    }
  else if (strncmp (*strp, "cb", 2) == 0 || strncmp (*strp, "CB", 2) == 0)
    {
      *strp += 2;
      *valuep = 0;
    }
  else
    errmsg = cgen_parse_unsigned_integer (cd, strp, opindex, valuep);

  return errmsg;
}

static const char *
parse_rbbc (CGEN_CPU_DESC cd,
	    const char **strp,
	    int opindex,
	    unsigned long *valuep)
{
  const char *errmsg = NULL;

  if (strncmp (*strp, "rt", 2) == 0 || strncmp (*strp, "RT", 2) == 0)
    {
      *strp += 2;
      *valuep = 0;
    }
  else if (strncmp (*strp, "br1", 3) == 0 || strncmp (*strp, "BR1", 3) == 0)
    {
      *strp += 3;
      *valuep = 1;
    }
  else if (strncmp (*strp, "br2", 3) == 0 || strncmp (*strp, "BR2", 3) == 0)
    {
      *strp += 3;
      *valuep = 2;
    }
  else if (strncmp (*strp, "cs", 2) == 0 || strncmp (*strp, "CS", 2) == 0)
    {
      *strp += 2;
      *valuep = 3;
    }
  else
    errmsg = cgen_parse_unsigned_integer (cd, strp, opindex, valuep);

  return errmsg;
}

static const char *
parse_type (CGEN_CPU_DESC cd,
	    const char **strp,
	    int opindex,
	    unsigned long *valuep)
{
  const char *errmsg = NULL;

  if (strncmp (*strp, "odd", 3) == 0 || strncmp (*strp, "ODD", 3) == 0)
    {
      *strp += 3;
      *valuep = 0;
    }
  else if (strncmp (*strp, "even", 4) == 0 || strncmp (*strp, "EVEN", 4) == 0)
    {
      *strp += 4;
      *valuep = 1;
    }
  else if (strncmp (*strp, "oe", 2) == 0 || strncmp (*strp, "OE", 2) == 0)
    {
      *strp += 2;
      *valuep = 2;
    }
  else
    errmsg = cgen_parse_unsigned_integer (cd, strp, opindex, valuep);

 if ((errmsg == NULL) && (*valuep == 3))
    errmsg = _("invalid operand.  type may have values 0,1,2 only.");

  return errmsg;
}

/* -- dis.c */

const char * mt_cgen_parse_operand
  (CGEN_CPU_DESC, int, const char **, CGEN_FIELDS *);

/* Main entry point for operand parsing.

   This function is basically just a big switch statement.  Earlier versions
   used tables to look up the function to use, but
   - if the table contains both assembler and disassembler functions then
     the disassembler contains much of the assembler and vice-versa,
   - there's a lot of inlining possibilities as things grow,
   - using a switch statement avoids the function call overhead.

   This function could be moved into `parse_insn_normal', but keeping it
   separate makes clear the interface between `parse_insn_normal' and each of
   the handlers.  */

const char *
mt_cgen_parse_operand (CGEN_CPU_DESC cd,
			   int opindex,
			   const char ** strp,
			   CGEN_FIELDS * fields)
{
  const char * errmsg = NULL;
  /* Used by scalar operands that still need to be parsed.  */
  long junk ATTRIBUTE_UNUSED;

  switch (opindex)
    {
    case MT_OPERAND_A23 :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_A23, (unsigned long *) (& fields->f_a23));
      break;
    case MT_OPERAND_BALL :
      errmsg = parse_ball (cd, strp, MT_OPERAND_BALL, (unsigned long *) (& fields->f_ball));
      break;
    case MT_OPERAND_BALL2 :
      errmsg = parse_ball (cd, strp, MT_OPERAND_BALL2, (unsigned long *) (& fields->f_ball2));
      break;
    case MT_OPERAND_BANKADDR :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_BANKADDR, (unsigned long *) (& fields->f_bankaddr));
      break;
    case MT_OPERAND_BRC :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_BRC, (unsigned long *) (& fields->f_brc));
      break;
    case MT_OPERAND_BRC2 :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_BRC2, (unsigned long *) (& fields->f_brc2));
      break;
    case MT_OPERAND_CB1INCR :
      errmsg = cgen_parse_signed_integer (cd, strp, MT_OPERAND_CB1INCR, (long *) (& fields->f_cb1incr));
      break;
    case MT_OPERAND_CB1SEL :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CB1SEL, (unsigned long *) (& fields->f_cb1sel));
      break;
    case MT_OPERAND_CB2INCR :
      errmsg = cgen_parse_signed_integer (cd, strp, MT_OPERAND_CB2INCR, (long *) (& fields->f_cb2incr));
      break;
    case MT_OPERAND_CB2SEL :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CB2SEL, (unsigned long *) (& fields->f_cb2sel));
      break;
    case MT_OPERAND_CBRB :
      errmsg = parse_cbrb (cd, strp, MT_OPERAND_CBRB, (unsigned long *) (& fields->f_cbrb));
      break;
    case MT_OPERAND_CBS :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CBS, (unsigned long *) (& fields->f_cbs));
      break;
    case MT_OPERAND_CBX :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CBX, (unsigned long *) (& fields->f_cbx));
      break;
    case MT_OPERAND_CCB :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CCB, (unsigned long *) (& fields->f_ccb));
      break;
    case MT_OPERAND_CDB :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CDB, (unsigned long *) (& fields->f_cdb));
      break;
    case MT_OPERAND_CELL :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CELL, (unsigned long *) (& fields->f_cell));
      break;
    case MT_OPERAND_COLNUM :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_COLNUM, (unsigned long *) (& fields->f_colnum));
      break;
    case MT_OPERAND_CONTNUM :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CONTNUM, (unsigned long *) (& fields->f_contnum));
      break;
    case MT_OPERAND_CR :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CR, (unsigned long *) (& fields->f_cr));
      break;
    case MT_OPERAND_CTXDISP :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_CTXDISP, (unsigned long *) (& fields->f_ctxdisp));
      break;
    case MT_OPERAND_DUP :
      errmsg = parse_dup (cd, strp, MT_OPERAND_DUP, (unsigned long *) (& fields->f_dup));
      break;
    case MT_OPERAND_FBDISP :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_FBDISP, (unsigned long *) (& fields->f_fbdisp));
      break;
    case MT_OPERAND_FBINCR :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_FBINCR, (unsigned long *) (& fields->f_fbincr));
      break;
    case MT_OPERAND_FRDR :
      errmsg = cgen_parse_keyword (cd, strp, & mt_cgen_opval_h_spr, & fields->f_dr);
      break;
    case MT_OPERAND_FRDRRR :
      errmsg = cgen_parse_keyword (cd, strp, & mt_cgen_opval_h_spr, & fields->f_drrr);
      break;
    case MT_OPERAND_FRSR1 :
      errmsg = cgen_parse_keyword (cd, strp, & mt_cgen_opval_h_spr, & fields->f_sr1);
      break;
    case MT_OPERAND_FRSR2 :
      errmsg = cgen_parse_keyword (cd, strp, & mt_cgen_opval_h_spr, & fields->f_sr2);
      break;
    case MT_OPERAND_ID :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_ID, (unsigned long *) (& fields->f_id));
      break;
    case MT_OPERAND_IMM16 :
      errmsg = parse_imm16 (cd, strp, MT_OPERAND_IMM16, (long *) (& fields->f_imm16s));
      break;
    case MT_OPERAND_IMM16L :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_IMM16L, (unsigned long *) (& fields->f_imm16l));
      break;
    case MT_OPERAND_IMM16O :
      errmsg = parse_imm16 (cd, strp, MT_OPERAND_IMM16O, (unsigned long *) (& fields->f_imm16s));
      break;
    case MT_OPERAND_IMM16Z :
      errmsg = parse_imm16 (cd, strp, MT_OPERAND_IMM16Z, (unsigned long *) (& fields->f_imm16u));
      break;
    case MT_OPERAND_INCAMT :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_INCAMT, (unsigned long *) (& fields->f_incamt));
      break;
    case MT_OPERAND_INCR :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_INCR, (unsigned long *) (& fields->f_incr));
      break;
    case MT_OPERAND_LENGTH :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_LENGTH, (unsigned long *) (& fields->f_length));
      break;
    case MT_OPERAND_LOOPSIZE :
      errmsg = parse_loopsize (cd, strp, MT_OPERAND_LOOPSIZE, (unsigned long *) (& fields->f_loopo));
      break;
    case MT_OPERAND_MASK :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_MASK, (unsigned long *) (& fields->f_mask));
      break;
    case MT_OPERAND_MASK1 :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_MASK1, (unsigned long *) (& fields->f_mask1));
      break;
    case MT_OPERAND_MODE :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_MODE, (unsigned long *) (& fields->f_mode));
      break;
    case MT_OPERAND_PERM :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_PERM, (unsigned long *) (& fields->f_perm));
      break;
    case MT_OPERAND_RBBC :
      errmsg = parse_rbbc (cd, strp, MT_OPERAND_RBBC, (unsigned long *) (& fields->f_rbbc));
      break;
    case MT_OPERAND_RC :
      errmsg = parse_rc (cd, strp, MT_OPERAND_RC, (unsigned long *) (& fields->f_rc));
      break;
    case MT_OPERAND_RC1 :
      errmsg = parse_rc (cd, strp, MT_OPERAND_RC1, (unsigned long *) (& fields->f_rc1));
      break;
    case MT_OPERAND_RC2 :
      errmsg = parse_rc (cd, strp, MT_OPERAND_RC2, (unsigned long *) (& fields->f_rc2));
      break;
    case MT_OPERAND_RC3 :
      errmsg = parse_rc (cd, strp, MT_OPERAND_RC3, (unsigned long *) (& fields->f_rc3));
      break;
    case MT_OPERAND_RCNUM :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_RCNUM, (unsigned long *) (& fields->f_rcnum));
      break;
    case MT_OPERAND_RDA :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_RDA, (unsigned long *) (& fields->f_rda));
      break;
    case MT_OPERAND_ROWNUM :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_ROWNUM, (unsigned long *) (& fields->f_rownum));
      break;
    case MT_OPERAND_ROWNUM1 :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_ROWNUM1, (unsigned long *) (& fields->f_rownum1));
      break;
    case MT_OPERAND_ROWNUM2 :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_ROWNUM2, (unsigned long *) (& fields->f_rownum2));
      break;
    case MT_OPERAND_SIZE :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_SIZE, (unsigned long *) (& fields->f_size));
      break;
    case MT_OPERAND_TYPE :
      errmsg = parse_type (cd, strp, MT_OPERAND_TYPE, (unsigned long *) (& fields->f_type));
      break;
    case MT_OPERAND_WR :
      errmsg = cgen_parse_unsigned_integer (cd, strp, MT_OPERAND_WR, (unsigned long *) (& fields->f_wr));
      break;
    case MT_OPERAND_XMODE :
      errmsg = parse_xmode (cd, strp, MT_OPERAND_XMODE, (unsigned long *) (& fields->f_xmode));
      break;

    default :
      /* xgettext:c-format */
      opcodes_error_handler
	(_("internal error: unrecognized field %d while parsing"),
	 opindex);
      abort ();
  }

  return errmsg;
}

cgen_parse_fn * const mt_cgen_parse_handlers[] =
{
  parse_insn_normal,
};

void
mt_cgen_init_asm (CGEN_CPU_DESC cd)
{
  mt_cgen_init_opcode_table (cd);
  mt_cgen_init_ibld_table (cd);
  cd->parse_handlers = & mt_cgen_parse_handlers[0];
  cd->parse_operand = mt_cgen_parse_operand;
#ifdef CGEN_ASM_INIT_HOOK
CGEN_ASM_INIT_HOOK
#endif
}



/* Regex construction routine.

   This translates an opcode syntax string into a regex string,
   by replacing any non-character syntax element (such as an
   opcode) with the pattern '.*'

   It then compiles the regex and stores it in the opcode, for
   later use by mt_cgen_assemble_insn

   Returns NULL for success, an error message for failure.  */

char *
mt_cgen_build_insn_regex (CGEN_INSN *insn)
{
  CGEN_OPCODE *opc = (CGEN_OPCODE *) CGEN_INSN_OPCODE (insn);
  const char *mnem = CGEN_INSN_MNEMONIC (insn);
  char rxbuf[CGEN_MAX_RX_ELEMENTS];
  char *rx = rxbuf;
  const CGEN_SYNTAX_CHAR_TYPE *syn;
  int reg_err;

  syn = CGEN_SYNTAX_STRING (CGEN_OPCODE_SYNTAX (opc));

  /* Mnemonics come first in the syntax string.  */
  if (! CGEN_SYNTAX_MNEMONIC_P (* syn))
    return _("missing mnemonic in syntax string");
  ++syn;

  /* Generate a case sensitive regular expression that emulates case
     insensitive matching in the "C" locale.  We cannot generate a case
     insensitive regular expression because in Turkish locales, 'i' and 'I'
     are not equal modulo case conversion.  */

  /* Copy the literal mnemonic out of the insn.  */
  for (; *mnem; mnem++)
    {
      char c = *mnem;

      if (ISALPHA (c))
	{
	  *rx++ = '[';
	  *rx++ = TOLOWER (c);
	  *rx++ = TOUPPER (c);
	  *rx++ = ']';
	}
      else
	*rx++ = c;
    }

  /* Copy any remaining literals from the syntax string into the rx.  */
  for(; * syn != 0 && rx <= rxbuf + (CGEN_MAX_RX_ELEMENTS - 7 - 4); ++syn)
    {
      if (CGEN_SYNTAX_CHAR_P (* syn))
	{
	  char c = CGEN_SYNTAX_CHAR (* syn);

	  switch (c)
	    {
	      /* Escape any regex metacharacters in the syntax.  */
	    case '.': case '[': case '\\':
	    case '*': case '^': case '$':

#ifdef CGEN_ESCAPE_EXTENDED_REGEX
	    case '?': case '{': case '}':
	    case '(': case ')': case '*':
	    case '|': case '+': case ']':
#endif
	      *rx++ = '\\';
	      *rx++ = c;
	      break;

	    default:
	      if (ISALPHA (c))
		{
		  *rx++ = '[';
		  *rx++ = TOLOWER (c);
		  *rx++ = TOUPPER (c);
		  *rx++ = ']';
		}
	      else
		*rx++ = c;
	      break;
	    }
	}
      else
	{
	  /* Replace non-syntax fields with globs.  */
	  *rx++ = '.';
	  *rx++ = '*';
	}
    }

  /* Trailing whitespace ok.  */
  * rx++ = '[';
  * rx++ = ' ';
  * rx++ = '\t';
  * rx++ = ']';
  * rx++ = '*';

  /* But anchor it after that.  */
  * rx++ = '$';
  * rx = '\0';

  CGEN_INSN_RX (insn) = xmalloc (sizeof (regex_t));
  reg_err = regcomp ((regex_t *) CGEN_INSN_RX (insn), rxbuf, REG_NOSUB);

  if (reg_err == 0)
    return NULL;
  else
    {
      static char msg[80];

      regerror (reg_err, (regex_t *) CGEN_INSN_RX (insn), msg, 80);
      regfree ((regex_t *) CGEN_INSN_RX (insn));
      free (CGEN_INSN_RX (insn));
      (CGEN_INSN_RX (insn)) = NULL;
      return msg;
    }
}


/* Default insn parser.

   The syntax string is scanned and operands are parsed and stored in FIELDS.
   Relocs are queued as we go via other callbacks.

   ??? Note that this is currently an all-or-nothing parser.  If we fail to
   parse the instruction, we return 0 and the caller will start over from
   the beginning.  Backtracking will be necessary in parsing subexpressions,
   but that can be handled there.  Not handling backtracking here may get
   expensive in the case of the m68k.  Deal with later.

   Returns NULL for success, an error message for failure.  */

static const char *
parse_insn_normal (CGEN_CPU_DESC cd,
		   const CGEN_INSN *insn,
		   const char **strp,
		   CGEN_FIELDS *fields)
{
  /* ??? Runtime added insns not handled yet.  */
  const CGEN_SYNTAX *syntax = CGEN_INSN_SYNTAX (insn);
  const char *str = *strp;
  const char *errmsg;
  const char *p;
  const CGEN_SYNTAX_CHAR_TYPE * syn;
#ifdef CGEN_MNEMONIC_OPERANDS
  /* FIXME: wip */
  int past_opcode_p;
#endif

  /* For now we assume the mnemonic is first (there are no leading operands).
     We can parse it without needing to set up operand parsing.
     GAS's input scrubber will ensure mnemonics are lowercase, but we may
     not be called from GAS.  */
  p = CGEN_INSN_MNEMONIC (insn);
  while (*p && TOLOWER (*p) == TOLOWER (*str))
    ++p, ++str;

  if (* p)
    return _("unrecognized instruction");

#ifndef CGEN_MNEMONIC_OPERANDS
  if (* str && ! ISSPACE (* str))
    return _("unrecognized instruction");
#endif

  CGEN_INIT_PARSE (cd);
  cgen_init_parse_operand (cd);
#ifdef CGEN_MNEMONIC_OPERANDS
  past_opcode_p = 0;
#endif

  /* We don't check for (*str != '\0') here because we want to parse
     any trailing fake arguments in the syntax string.  */
  syn = CGEN_SYNTAX_STRING (syntax);

  /* Mnemonics come first for now, ensure valid string.  */
  if (! CGEN_SYNTAX_MNEMONIC_P (* syn))
    abort ();

  ++syn;

  while (* syn != 0)
    {
      /* Non operand chars must match exactly.  */
      if (CGEN_SYNTAX_CHAR_P (* syn))
	{
	  /* FIXME: While we allow for non-GAS callers above, we assume the
	     first char after the mnemonic part is a space.  */
	  /* FIXME: We also take inappropriate advantage of the fact that
	     GAS's input scrubber will remove extraneous blanks.  */
	  if (TOLOWER (*str) == TOLOWER (CGEN_SYNTAX_CHAR (* syn)))
	    {
#ifdef CGEN_MNEMONIC_OPERANDS
	      if (CGEN_SYNTAX_CHAR(* syn) == ' ')
		past_opcode_p = 1;
#endif
	      ++ syn;
	      ++ str;
	    }
	  else if (*str)
	    {
	      /* Syntax char didn't match.  Can't be this insn.  */
	      static char msg [80];

	      /* xgettext:c-format */
	      sprintf (msg, _("syntax error (expected char `%c', found `%c')"),
		       CGEN_SYNTAX_CHAR(*syn), *str);
	      return msg;
	    }
	  else
	    {
	      /* Ran out of input.  */
	      static char msg [80];

	      /* xgettext:c-format */
	      sprintf (msg, _("syntax error (expected char `%c', found end of instruction)"),
		       CGEN_SYNTAX_CHAR(*syn));
	      return msg;
	    }
	  continue;
	}

#ifdef CGEN_MNEMONIC_OPERANDS
      (void) past_opcode_p;
#endif
      /* We have an operand of some sort.  */
      errmsg = cd->parse_operand (cd, CGEN_SYNTAX_FIELD (*syn), &str, fields);
      if (errmsg)
	return errmsg;

      /* Done with this operand, continue with next one.  */
      ++ syn;
    }

  /* If we're at the end of the syntax string, we're done.  */
  if (* syn == 0)
    {
      /* FIXME: For the moment we assume a valid `str' can only contain
	 blanks now.  IE: We needn't try again with a longer version of
	 the insn and it is assumed that longer versions of insns appear
	 before shorter ones (eg: lsr r2,r3,1 vs lsr r2,r3).  */
      while (ISSPACE (* str))
	++ str;

      if (* str != '\0')
	return _("junk at end of line"); /* FIXME: would like to include `str' */

      return NULL;
    }

  /* We couldn't parse it.  */
  return _("unrecognized instruction");
}

/* Main entry point.
   This routine is called for each instruction to be assembled.
   STR points to the insn to be assembled.
   We assume all necessary tables have been initialized.
   The assembled instruction, less any fixups, is stored in BUF.
   Remember that if CGEN_INT_INSN_P then BUF is an int and thus the value
   still needs to be converted to target byte order, otherwise BUF is an array
   of bytes in target byte order.
   The result is a pointer to the insn's entry in the opcode table,
   or NULL if an error occured (an error message will have already been
   printed).

   Note that when processing (non-alias) macro-insns,
   this function recurses.

   ??? It's possible to make this cpu-independent.
   One would have to deal with a few minor things.
   At this point in time doing so would be more of a curiosity than useful
   [for example this file isn't _that_ big], but keeping the possibility in
   mind helps keep the design clean.  */

const CGEN_INSN *
mt_cgen_assemble_insn (CGEN_CPU_DESC cd,
			   const char *str,
			   CGEN_FIELDS *fields,
			   CGEN_INSN_BYTES_PTR buf,
			   char **errmsg)
{
  const char *start;
  CGEN_INSN_LIST *ilist;
  const char *parse_errmsg = NULL;
  const char *insert_errmsg = NULL;
  int recognized_mnemonic = 0;

  /* Skip leading white space.  */
  while (ISSPACE (* str))
    ++ str;

  /* The instructions are stored in hashed lists.
     Get the first in the list.  */
  ilist = CGEN_ASM_LOOKUP_INSN (cd, str);

  /* Keep looking until we find a match.  */
  start = str;
  for ( ; ilist != NULL ; ilist = CGEN_ASM_NEXT_INSN (ilist))
    {
      const CGEN_INSN *insn = ilist->insn;
      recognized_mnemonic = 1;

#ifdef CGEN_VALIDATE_INSN_SUPPORTED
      /* Not usually needed as unsupported opcodes
	 shouldn't be in the hash lists.  */
      /* Is this insn supported by the selected cpu?  */
      if (! mt_cgen_insn_supported (cd, insn))
	continue;
#endif
      /* If the RELAXED attribute is set, this is an insn that shouldn't be
	 chosen immediately.  Instead, it is used during assembler/linker
	 relaxation if possible.  */
      if (CGEN_INSN_ATTR_VALUE (insn, CGEN_INSN_RELAXED) != 0)
	continue;

      str = start;

      /* Skip this insn if str doesn't look right lexically.  */
      if (CGEN_INSN_RX (insn) != NULL &&
	  regexec ((regex_t *) CGEN_INSN_RX (insn), str, 0, NULL, 0) == REG_NOMATCH)
	continue;

      /* Allow parse/insert handlers to obtain length of insn.  */
      CGEN_FIELDS_BITSIZE (fields) = CGEN_INSN_BITSIZE (insn);

      parse_errmsg = CGEN_PARSE_FN (cd, insn) (cd, insn, & str, fields);
      if (parse_errmsg != NULL)
	continue;

      /* ??? 0 is passed for `pc'.  */
      insert_errmsg = CGEN_INSERT_FN (cd, insn) (cd, insn, fields, buf,
						 (bfd_vma) 0);
      if (insert_errmsg != NULL)
        continue;

      /* It is up to the caller to actually output the insn and any
         queued relocs.  */
      return insn;
    }

  {
    static char errbuf[150];
    const char *tmp_errmsg;
#ifdef CGEN_VERBOSE_ASSEMBLER_ERRORS
#define be_verbose 1
#else
#define be_verbose 0
#endif

    if (be_verbose)
      {
	/* If requesting verbose error messages, use insert_errmsg.
	   Failing that, use parse_errmsg.  */
	tmp_errmsg = (insert_errmsg ? insert_errmsg :
		      parse_errmsg ? parse_errmsg :
		      recognized_mnemonic ?
		      _("unrecognized form of instruction") :
		      _("unrecognized instruction"));

	if (strlen (start) > 50)
	  /* xgettext:c-format */
	  sprintf (errbuf, "%s `%.50s...'", tmp_errmsg, start);
	else
	  /* xgettext:c-format */
	  sprintf (errbuf, "%s `%.50s'", tmp_errmsg, start);
      }
    else
      {
	if (strlen (start) > 50)
	  /* xgettext:c-format */
	  sprintf (errbuf, _("bad instruction `%.50s...'"), start);
	else
	  /* xgettext:c-format */
	  sprintf (errbuf, _("bad instruction `%.50s'"), start);
      }

    *errmsg = errbuf;
    return NULL;
  }
}
