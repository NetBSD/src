/* Generic opcode table support for targets using CGEN. -*- C -*-
   CGEN: Cpu tools GENerator

This file is used to generate m32r-opc.c.

Copyright (C) 1998 Free Software Foundation, Inc.

This file is part of the GNU Binutils and GDB, the GNU debugger.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include "ansidecl.h"
#include "libiberty.h"
#include "bfd.h"
#include "symcat.h"
#include "m32r-opc.h"

/* Look up instruction INSN_VALUE and extract its fields.
   If non-null INSN is the insn table entry.
   Otherwise INSN_VALUE is examined to compute it.
   LENGTH is the bit length of INSN_VALUE if known, otherwise 0.
   ALIAS_P is non-zero if alias insns are to be included in the search.
   The result a pointer to the insn table entry, or NULL if the instruction
   wasn't recognized.  */

const CGEN_INSN *
m32r_cgen_lookup_insn (insn, insn_value, length, fields, alias_p)
     const CGEN_INSN *insn;
     cgen_insn_t insn_value;
     int length;
     CGEN_FIELDS *fields;
{
  char buf[16];

  if (!insn)
    {
      const CGEN_INSN_LIST *insn_list;

#ifdef CGEN_INT_INSN
      switch (length)
	{
	case 8:
	  buf[0] = insn_value;
	  break;
	case 16:
	  if (cgen_current_endian == CGEN_ENDIAN_BIG)
	    bfd_putb16 (insn_value, buf);
	  else
	    bfd_putl16 (insn_value, buf);
	  break;
	case 32:
	  if (cgen_current_endian == CGEN_ENDIAN_BIG)
	    bfd_putb32 (insn_value, buf);
	  else
	    bfd_putl32 (insn_value, buf);
	  break;
	default:
	  abort ();
	}
#else
      abort (); /* FIXME: unfinished */
#endif

      /* The instructions are stored in hash lists.
	 Pick the first one and keep trying until we find the right one.  */

      insn_list = CGEN_DIS_LOOKUP_INSN (buf, insn_value);
      while (insn_list != NULL)
	{
	  insn = insn_list->insn;

	  if (alias_p
	      || ! CGEN_INSN_ATTR (insn, CGEN_INSN_ALIAS))
	    {
	      /* Basic bit mask must be correct.  */
	      /* ??? May wish to allow target to defer this check until the
		 extract handler.  */
	      if ((insn_value & CGEN_INSN_MASK (insn)) == CGEN_INSN_VALUE (insn))
		{
		  length = (*CGEN_EXTRACT_FN (insn)) (insn, NULL, insn_value, fields);
		  if (length > 0)
		    return insn;
		}
	    }

	  insn_list = CGEN_DIS_NEXT_INSN (insn_list);
	}
    }
  else
    {
      /* Sanity check: can't pass an alias insn if ! alias_p.  */
      if (! alias_p
	  && CGEN_INSN_ATTR (insn, CGEN_INSN_ALIAS))
	abort ();

      length = (*CGEN_EXTRACT_FN (insn)) (insn, NULL, insn_value, fields);
      if (length > 0)
	return insn;
    }

  return NULL;
}

/* Fill in the operand instances used by insn INSN_VALUE.
   If non-null INS is the insn table entry.
   Otherwise INSN_VALUE is examined to compute it.
   LENGTH is the number of bits in INSN_VALUE if known, otherwise 0.
   INDICES is a pointer to a buffer of MAX_OPERAND_INSTANCES ints to be filled
   in.
   The result a pointer to the insn table entry, or NULL if the instruction
   wasn't recognized.  */

const CGEN_INSN *
m32r_cgen_get_insn_operands (insn, insn_value, length, indices)
     const CGEN_INSN *insn;
     cgen_insn_t insn_value;
     int length;
     int *indices;
{
  CGEN_FIELDS fields;
  const CGEN_OPERAND_INSTANCE *opinst;
  int i;

  /* FIXME: ALIAS insns are in transition from being record in the insn table
     to being recorded separately as macros.  They don't have semantic code
     so they can't be used here.  Thus we currently always ignore the INSN
     argument.  */
  insn = m32r_cgen_lookup_insn (NULL, insn_value, length, &fields, 0);
  if (! insn)
    return NULL;

  for (i = 0, opinst = CGEN_INSN_OPERANDS (insn);
       opinst != NULL
	 && CGEN_OPERAND_INSTANCE_TYPE (opinst) != CGEN_OPERAND_INSTANCE_END;
       ++i, ++opinst)
    {
      const CGEN_OPERAND *op = CGEN_OPERAND_INSTANCE_OPERAND (opinst);
      if (op == NULL)
	indices[i] = CGEN_OPERAND_INSTANCE_INDEX (opinst);
      else
	indices[i] = m32r_cgen_get_operand (CGEN_OPERAND_INDEX (op), &fields);
    }

  return insn;
}
/* Attributes.  */

static const CGEN_ATTR_ENTRY MACH_attr[] =
{
  { "m32r", MACH_M32R },
  { "max", MACH_MAX },
  { 0, 0 }
};

const CGEN_ATTR_TABLE m32r_cgen_operand_attr_table[] =
{
  { "ABS-ADDR", NULL },
  { "FAKE", NULL },
  { "HASH-PREFIX", NULL },
  { "NEGATIVE", NULL },
  { "PC", NULL },
  { "PCREL-ADDR", NULL },
  { "RELAX", NULL },
  { "RELOC", NULL },
  { "SIGN-OPT", NULL },
  { "UNSIGNED", NULL },
  { 0, 0 }
};

const CGEN_ATTR_TABLE m32r_cgen_insn_attr_table[] =
{
  { "MACH", & MACH_attr[0] },
  { "ALIAS", NULL },
  { "COND-CTI", NULL },
  { "FILL-SLOT", NULL },
  { "PARALLEL", NULL },
  { "RELAX", NULL },
  { "RELAXABLE", NULL },
  { "UNCOND-CTI", NULL },
  { 0, 0 }
};

CGEN_KEYWORD_ENTRY m32r_cgen_opval_h_gr_entries[] = 
{
  { "fp", 13 },
  { "lr", 14 },
  { "sp", 15 },
  { "r0", 0 },
  { "r1", 1 },
  { "r2", 2 },
  { "r3", 3 },
  { "r4", 4 },
  { "r5", 5 },
  { "r6", 6 },
  { "r7", 7 },
  { "r8", 8 },
  { "r9", 9 },
  { "r10", 10 },
  { "r11", 11 },
  { "r12", 12 },
  { "r13", 13 },
  { "r14", 14 },
  { "r15", 15 }
};

CGEN_KEYWORD m32r_cgen_opval_h_gr = 
{
  & m32r_cgen_opval_h_gr_entries[0],
  19
};

CGEN_KEYWORD_ENTRY m32r_cgen_opval_h_cr_entries[] = 
{
  { "psw", 0 },
  { "cbr", 1 },
  { "spi", 2 },
  { "spu", 3 },
  { "bpc", 6 },
  { "cr0", 0 },
  { "cr1", 1 },
  { "cr2", 2 },
  { "cr3", 3 },
  { "cr4", 4 },
  { "cr5", 5 },
  { "cr6", 6 }
};

CGEN_KEYWORD m32r_cgen_opval_h_cr = 
{
  & m32r_cgen_opval_h_cr_entries[0],
  12
};


/* The hardware table.  */

#define HW_ENT(n) m32r_cgen_hw_entries[n]
static const CGEN_HW_ENTRY m32r_cgen_hw_entries[] =
{
  { HW_H_PC, & HW_ENT (HW_H_PC + 1), "h-pc", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_MEMORY, & HW_ENT (HW_H_MEMORY + 1), "h-memory", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_SINT, & HW_ENT (HW_H_SINT + 1), "h-sint", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_UINT, & HW_ENT (HW_H_UINT + 1), "h-uint", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_ADDR, & HW_ENT (HW_H_ADDR + 1), "h-addr", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_IADDR, & HW_ENT (HW_H_IADDR + 1), "h-iaddr", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_HI16, & HW_ENT (HW_H_HI16 + 1), "h-hi16", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_SLO16, & HW_ENT (HW_H_SLO16 + 1), "h-slo16", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_ULO16, & HW_ENT (HW_H_ULO16 + 1), "h-ulo16", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_GR, & HW_ENT (HW_H_GR + 1), "h-gr", CGEN_ASM_KEYWORD, (PTR) & m32r_cgen_opval_h_gr },
  { HW_H_CR, & HW_ENT (HW_H_CR + 1), "h-cr", CGEN_ASM_KEYWORD, (PTR) & m32r_cgen_opval_h_cr },
  { HW_H_ACCUM, & HW_ENT (HW_H_ACCUM + 1), "h-accum", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_COND, & HW_ENT (HW_H_COND + 1), "h-cond", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_SM, & HW_ENT (HW_H_SM + 1), "h-sm", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_BSM, & HW_ENT (HW_H_BSM + 1), "h-bsm", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_IE, & HW_ENT (HW_H_IE + 1), "h-ie", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_BIE, & HW_ENT (HW_H_BIE + 1), "h-bie", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_BCOND, & HW_ENT (HW_H_BCOND + 1), "h-bcond", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_BPC, & HW_ENT (HW_H_BPC + 1), "h-bpc", CGEN_ASM_KEYWORD, (PTR) 0 },
  { HW_H_LOCK, & HW_ENT (HW_H_LOCK + 1), "h-lock", CGEN_ASM_KEYWORD, (PTR) 0 },
  { 0 }
};

/* The operand table.  */

#define OPERAND(op) CONCAT2 (M32R_OPERAND_,op)
#define OP_ENT(op) m32r_cgen_operand_table[OPERAND (op)]

const CGEN_OPERAND m32r_cgen_operand_table[MAX_OPERANDS] =
{
/* pc: program counter */
  { "pc", & HW_ENT (HW_H_PC), 0, 0,
    { 0, 0|(1<<CGEN_OPERAND_FAKE)|(1<<CGEN_OPERAND_PC), { 0 } }  },
/* sr: source register */
  { "sr", & HW_ENT (HW_H_GR), 12, 4,
    { 0, 0|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* dr: destination register */
  { "dr", & HW_ENT (HW_H_GR), 4, 4,
    { 0, 0|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* src1: source register 1 */
  { "src1", & HW_ENT (HW_H_GR), 4, 4,
    { 0, 0|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* src2: source register 2 */
  { "src2", & HW_ENT (HW_H_GR), 12, 4,
    { 0, 0|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* scr: source control register */
  { "scr", & HW_ENT (HW_H_CR), 12, 4,
    { 0, 0|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* dcr: destination control register */
  { "dcr", & HW_ENT (HW_H_CR), 4, 4,
    { 0, 0|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* simm8: 8 bit signed immediate */
  { "simm8", & HW_ENT (HW_H_SINT), 8, 8,
    { 0, 0|(1<<CGEN_OPERAND_HASH_PREFIX), { 0 } }  },
/* simm16: 16 bit signed immediate */
  { "simm16", & HW_ENT (HW_H_SINT), 16, 16,
    { 0, 0|(1<<CGEN_OPERAND_HASH_PREFIX), { 0 } }  },
/* uimm4: 4 bit trap number */
  { "uimm4", & HW_ENT (HW_H_UINT), 12, 4,
    { 0, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* uimm5: 5 bit shift count */
  { "uimm5", & HW_ENT (HW_H_UINT), 11, 5,
    { 0, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* uimm16: 16 bit unsigned immediate */
  { "uimm16", & HW_ENT (HW_H_UINT), 16, 16,
    { 0, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* hash: # prefix */
  { "hash", & HW_ENT (HW_H_SINT), 0, 0,
    { 0, 0, { 0 } }  },
/* hi16: high 16 bit immediate, sign optional */
  { "hi16", & HW_ENT (HW_H_HI16), 16, 16,
    { 0, 0|(1<<CGEN_OPERAND_SIGN_OPT)|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* slo16: 16 bit signed immediate, for low() */
  { "slo16", & HW_ENT (HW_H_SLO16), 16, 16,
    { 0, 0, { 0 } }  },
/* ulo16: 16 bit unsigned immediate, for low() */
  { "ulo16", & HW_ENT (HW_H_ULO16), 16, 16,
    { 0, 0|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* uimm24: 24 bit address */
  { "uimm24", & HW_ENT (HW_H_ADDR), 8, 24,
    { 0, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_ABS_ADDR)|(1<<CGEN_OPERAND_UNSIGNED), { 0 } }  },
/* disp8: 8 bit displacement */
  { "disp8", & HW_ENT (HW_H_IADDR), 8, 8,
    { 0, 0|(1<<CGEN_OPERAND_RELAX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), { 0 } }  },
/* disp16: 16 bit displacement */
  { "disp16", & HW_ENT (HW_H_IADDR), 16, 16,
    { 0, 0|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), { 0 } }  },
/* disp24: 24 bit displacement */
  { "disp24", & HW_ENT (HW_H_IADDR), 8, 24,
    { 0, 0|(1<<CGEN_OPERAND_RELAX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), { 0 } }  },
/* condbit: condition bit */
  { "condbit", & HW_ENT (HW_H_COND), 0, 0,
    { 0, 0|(1<<CGEN_OPERAND_FAKE), { 0 } }  },
/* accum: accumulator */
  { "accum", & HW_ENT (HW_H_ACCUM), 0, 0,
    { 0, 0|(1<<CGEN_OPERAND_FAKE), { 0 } }  },
};

/* Operand references.  */

#define INPUT CGEN_OPERAND_INSTANCE_INPUT
#define OUTPUT CGEN_OPERAND_INSTANCE_OUTPUT

static const CGEN_OPERAND_INSTANCE fmt_0_add_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_1_add3_ops[] = {
  { INPUT, & HW_ENT (HW_H_SLO16), CGEN_MODE_HI, & OP_ENT (SLO16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_2_and3_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { INPUT, & HW_ENT (HW_H_UINT), CGEN_MODE_USI, & OP_ENT (UIMM16), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_3_or3_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { INPUT, & HW_ENT (HW_H_ULO16), CGEN_MODE_UHI, & OP_ENT (ULO16), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_4_addi_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { INPUT, & HW_ENT (HW_H_SINT), CGEN_MODE_SI, & OP_ENT (SIMM8), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_5_addv_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_6_addv3_ops[] = {
  { INPUT, & HW_ENT (HW_H_SINT), CGEN_MODE_SI, & OP_ENT (SIMM16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_7_addx_ops[] = {
  { INPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_8_bc8_ops[] = {
  { INPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_IADDR), CGEN_MODE_VM, & OP_ENT (DISP8), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_10_bc24_ops[] = {
  { INPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_IADDR), CGEN_MODE_VM, & OP_ENT (DISP24), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_12_beq_ops[] = {
  { INPUT, & HW_ENT (HW_H_IADDR), CGEN_MODE_VM, & OP_ENT (DISP16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_13_beqz_ops[] = {
  { INPUT, & HW_ENT (HW_H_IADDR), CGEN_MODE_VM, & OP_ENT (DISP16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_14_bl8_ops[] = {
  { INPUT, & HW_ENT (HW_H_IADDR), CGEN_MODE_VM, & OP_ENT (DISP8), 0 },
  { INPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, 0, 14 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_15_bl24_ops[] = {
  { INPUT, & HW_ENT (HW_H_IADDR), CGEN_MODE_VM, & OP_ENT (DISP24), 0 },
  { INPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, 0, 14 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_18_bra8_ops[] = {
  { INPUT, & HW_ENT (HW_H_IADDR), CGEN_MODE_VM, & OP_ENT (DISP8), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_19_bra24_ops[] = {
  { INPUT, & HW_ENT (HW_H_IADDR), CGEN_MODE_VM, & OP_ENT (DISP24), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_20_cmp_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_21_cmpi_ops[] = {
  { INPUT, & HW_ENT (HW_H_SINT), CGEN_MODE_SI, & OP_ENT (SIMM16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_22_cmpui_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { INPUT, & HW_ENT (HW_H_UINT), CGEN_MODE_USI, & OP_ENT (UIMM16), 0 },
  { OUTPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_24_div_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_26_jl_ops[] = {
  { INPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, 0, 14 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_27_jmp_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_28_ld_ops[] = {
  { INPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_SI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_30_ld_d_ops[] = {
  { INPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_SI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_SLO16), CGEN_MODE_HI, & OP_ENT (SLO16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_32_ldb_ops[] = {
  { INPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_QI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_33_ldb_d_ops[] = {
  { INPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_QI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_SLO16), CGEN_MODE_HI, & OP_ENT (SLO16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_34_ldh_ops[] = {
  { INPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_HI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_35_ldh_d_ops[] = {
  { INPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_HI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_SLO16), CGEN_MODE_HI, & OP_ENT (SLO16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_36_ld_plus_ops[] = {
  { INPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_SI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_37_ld24_ops[] = {
  { INPUT, & HW_ENT (HW_H_ADDR), CGEN_MODE_VM, & OP_ENT (UIMM24), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_38_ldi8_ops[] = {
  { INPUT, & HW_ENT (HW_H_SINT), CGEN_MODE_SI, & OP_ENT (SIMM8), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_39_ldi16_ops[] = {
  { INPUT, & HW_ENT (HW_H_SLO16), CGEN_MODE_HI, & OP_ENT (SLO16), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_40_lock_ops[] = {
  { INPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_SI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { OUTPUT, & HW_ENT (HW_H_LOCK), CGEN_MODE_UBI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_41_machi_ops[] = {
  { INPUT, & HW_ENT (HW_H_ACCUM), CGEN_MODE_DI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_ACCUM), CGEN_MODE_DI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_43_mulhi_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_ACCUM), CGEN_MODE_DI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_45_mv_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_46_mvfachi_ops[] = {
  { INPUT, & HW_ENT (HW_H_ACCUM), CGEN_MODE_DI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_48_mvfc_ops[] = {
  { INPUT, & HW_ENT (HW_H_CR), CGEN_MODE_USI, & OP_ENT (SCR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_49_mvtachi_ops[] = {
  { INPUT, & HW_ENT (HW_H_ACCUM), CGEN_MODE_DI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { OUTPUT, & HW_ENT (HW_H_ACCUM), CGEN_MODE_DI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_51_mvtc_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_CR), CGEN_MODE_USI, & OP_ENT (DCR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_53_rac_ops[] = {
  { INPUT, & HW_ENT (HW_H_ACCUM), CGEN_MODE_DI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_ACCUM), CGEN_MODE_DI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_57_rte_ops[] = {
  { INPUT, & HW_ENT (HW_H_BCOND), CGEN_MODE_VM, 0, 0 },
  { INPUT, & HW_ENT (HW_H_BIE), CGEN_MODE_VM, 0, 0 },
  { INPUT, & HW_ENT (HW_H_BPC), CGEN_MODE_VM, 0, 0 },
  { INPUT, & HW_ENT (HW_H_BSM), CGEN_MODE_VM, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_COND), CGEN_MODE_UBI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_IE), CGEN_MODE_VM, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_SM), CGEN_MODE_VM, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_58_seth_ops[] = {
  { INPUT, & HW_ENT (HW_H_HI16), CGEN_MODE_UHI, & OP_ENT (HI16), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_59_sll3_ops[] = {
  { INPUT, & HW_ENT (HW_H_SINT), CGEN_MODE_SI, & OP_ENT (SIMM16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SR), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_60_slli_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { INPUT, & HW_ENT (HW_H_UINT), CGEN_MODE_USI, & OP_ENT (UIMM5), 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (DR), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_61_st_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_SI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_63_st_d_ops[] = {
  { INPUT, & HW_ENT (HW_H_SLO16), CGEN_MODE_HI, & OP_ENT (SLO16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_SI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_65_stb_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_QI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_66_stb_d_ops[] = {
  { INPUT, & HW_ENT (HW_H_SLO16), CGEN_MODE_HI, & OP_ENT (SLO16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_QI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_67_sth_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_HI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_68_sth_d_ops[] = {
  { INPUT, & HW_ENT (HW_H_SLO16), CGEN_MODE_HI, & OP_ENT (SLO16), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_HI, 0, 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_69_st_plus_ops[] = {
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_SI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_70_trap_ops[] = {
  { INPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_CR), CGEN_MODE_SI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_UINT), CGEN_MODE_USI, & OP_ENT (UIMM4), 0 },
  { OUTPUT, & HW_ENT (HW_H_PC), CGEN_MODE_USI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_CR), CGEN_MODE_SI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_CR), CGEN_MODE_SI, 0, 6 },
  { 0 }
};

static const CGEN_OPERAND_INSTANCE fmt_71_unlock_ops[] = {
  { INPUT, & HW_ENT (HW_H_LOCK), CGEN_MODE_UBI, 0, 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC1), 0 },
  { INPUT, & HW_ENT (HW_H_GR), CGEN_MODE_SI, & OP_ENT (SRC2), 0 },
  { OUTPUT, & HW_ENT (HW_H_MEMORY), CGEN_MODE_SI, 0, 0 },
  { OUTPUT, & HW_ENT (HW_H_LOCK), CGEN_MODE_UBI, 0, 0 },
  { 0 }
};

#undef INPUT
#undef OUTPUT

#define A(a) (1 << CONCAT2 (CGEN_INSN_,a))
#define MNEM CGEN_SYNTAX_MNEMONIC /* syntax value for mnemonic */
#define OP(field) CGEN_SYNTAX_MAKE_FIELD (OPERAND (field))

/* The instruction table.  */

const CGEN_INSN m32r_cgen_insn_table_entries[MAX_INSNS] =
{
  /* null first entry, end of all hash chains */
  { { 0 }, 0 },
/* add $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "add", "add",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0xa0,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(PARALLEL), { (1<<MACH_M32R) } }
  },
/* add3 $dr,$sr,$hash$slo16 */
  {
    { 1, 1, 1, 1 },
    "add3", "add3",
    { MNEM, ' ', OP (DR), ',', OP (SR), ',', OP (HASH), OP (SLO16), 0 },
    { 32, 32, 0xf0f00000 }, 0x80a00000,
    & fmt_1_add3_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* and $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "and", "and",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0xc0,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(PARALLEL), { (1<<MACH_M32R) } }
  },
/* and3 $dr,$sr,$uimm16 */
  {
    { 1, 1, 1, 1 },
    "and3", "and3",
    { MNEM, ' ', OP (DR), ',', OP (SR), ',', OP (UIMM16), 0 },
    { 32, 32, 0xf0f00000 }, 0x80c00000,
    & fmt_2_and3_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* or $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "or", "or",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0xe0,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(PARALLEL), { (1<<MACH_M32R) } }
  },
/* or3 $dr,$sr,$hash$ulo16 */
  {
    { 1, 1, 1, 1 },
    "or3", "or3",
    { MNEM, ' ', OP (DR), ',', OP (SR), ',', OP (HASH), OP (ULO16), 0 },
    { 32, 32, 0xf0f00000 }, 0x80e00000,
    & fmt_3_or3_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* xor $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "xor", "xor",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0xd0,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(PARALLEL), { (1<<MACH_M32R) } }
  },
/* xor3 $dr,$sr,$uimm16 */
  {
    { 1, 1, 1, 1 },
    "xor3", "xor3",
    { MNEM, ' ', OP (DR), ',', OP (SR), ',', OP (UIMM16), 0 },
    { 32, 32, 0xf0f00000 }, 0x80d00000,
    & fmt_2_and3_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* addi $dr,$simm8 */
  {
    { 1, 1, 1, 1 },
    "addi", "addi",
    { MNEM, ' ', OP (DR), ',', OP (SIMM8), 0 },
    { 16, 16, 0xf000 }, 0x4000,
    & fmt_4_addi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* addv $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "addv", "addv",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x80,
    & fmt_5_addv_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* addv3 $dr,$sr,$simm16 */
  {
    { 1, 1, 1, 1 },
    "addv3", "addv3",
    { MNEM, ' ', OP (DR), ',', OP (SR), ',', OP (SIMM16), 0 },
    { 32, 32, 0xf0f00000 }, 0x80800000,
    & fmt_6_addv3_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* addx $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "addx", "addx",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x90,
    & fmt_7_addx_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* bc $disp8 */
  {
    { 1, 1, 1, 1 },
    "bc8", "bc",
    { MNEM, ' ', OP (DISP8), 0 },
    { 16, 16, 0xff00 }, 0x7c00,
    & fmt_8_bc8_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(RELAXABLE)|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bc.s $disp8 */
  {
    { 1, 1, 1, 1 },
    "bc8.s", "bc.s",
    { MNEM, ' ', OP (DISP8), 0 },
    { 16, 16, 0xff00 }, 0x7c00,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS)|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bc $disp24 */
  {
    { 1, 1, 1, 1 },
    "bc24", "bc",
    { MNEM, ' ', OP (DISP24), 0 },
    { 32, 32, 0xff000000 }, 0xfc000000,
    & fmt_10_bc24_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(RELAX)|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bc.l $disp24 */
  {
    { 1, 1, 1, 1 },
    "bc24.l", "bc.l",
    { MNEM, ' ', OP (DISP24), 0 },
    { 32, 32, 0xff000000 }, 0xfc000000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS)|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* beq $src1,$src2,$disp16 */
  {
    { 1, 1, 1, 1 },
    "beq", "beq",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), ',', OP (DISP16), 0 },
    { 32, 32, 0xf0f00000 }, 0xb0000000,
    & fmt_12_beq_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* beqz $src2,$disp16 */
  {
    { 1, 1, 1, 1 },
    "beqz", "beqz",
    { MNEM, ' ', OP (SRC2), ',', OP (DISP16), 0 },
    { 32, 32, 0xfff00000 }, 0xb0800000,
    & fmt_13_beqz_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bgez $src2,$disp16 */
  {
    { 1, 1, 1, 1 },
    "bgez", "bgez",
    { MNEM, ' ', OP (SRC2), ',', OP (DISP16), 0 },
    { 32, 32, 0xfff00000 }, 0xb0b00000,
    & fmt_13_beqz_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bgtz $src2,$disp16 */
  {
    { 1, 1, 1, 1 },
    "bgtz", "bgtz",
    { MNEM, ' ', OP (SRC2), ',', OP (DISP16), 0 },
    { 32, 32, 0xfff00000 }, 0xb0d00000,
    & fmt_13_beqz_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* blez $src2,$disp16 */
  {
    { 1, 1, 1, 1 },
    "blez", "blez",
    { MNEM, ' ', OP (SRC2), ',', OP (DISP16), 0 },
    { 32, 32, 0xfff00000 }, 0xb0c00000,
    & fmt_13_beqz_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bltz $src2,$disp16 */
  {
    { 1, 1, 1, 1 },
    "bltz", "bltz",
    { MNEM, ' ', OP (SRC2), ',', OP (DISP16), 0 },
    { 32, 32, 0xfff00000 }, 0xb0a00000,
    & fmt_13_beqz_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bnez $src2,$disp16 */
  {
    { 1, 1, 1, 1 },
    "bnez", "bnez",
    { MNEM, ' ', OP (SRC2), ',', OP (DISP16), 0 },
    { 32, 32, 0xfff00000 }, 0xb0900000,
    & fmt_13_beqz_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bl $disp8 */
  {
    { 1, 1, 1, 1 },
    "bl8", "bl",
    { MNEM, ' ', OP (DISP8), 0 },
    { 16, 16, 0xff00 }, 0x7e00,
    & fmt_14_bl8_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(FILL_SLOT)|A(RELAXABLE)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* bl.s $disp8 */
  {
    { 1, 1, 1, 1 },
    "bl8.s", "bl.s",
    { MNEM, ' ', OP (DISP8), 0 },
    { 16, 16, 0xff00 }, 0x7e00,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(FILL_SLOT)|A(ALIAS)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* bl $disp24 */
  {
    { 1, 1, 1, 1 },
    "bl24", "bl",
    { MNEM, ' ', OP (DISP24), 0 },
    { 32, 32, 0xff000000 }, 0xfe000000,
    & fmt_15_bl24_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(RELAX)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* bl.l $disp24 */
  {
    { 1, 1, 1, 1 },
    "bl24.l", "bl.l",
    { MNEM, ' ', OP (DISP24), 0 },
    { 32, 32, 0xff000000 }, 0xfe000000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* bnc $disp8 */
  {
    { 1, 1, 1, 1 },
    "bnc8", "bnc",
    { MNEM, ' ', OP (DISP8), 0 },
    { 16, 16, 0xff00 }, 0x7d00,
    & fmt_8_bc8_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(RELAXABLE)|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bnc.s $disp8 */
  {
    { 1, 1, 1, 1 },
    "bnc8.s", "bnc.s",
    { MNEM, ' ', OP (DISP8), 0 },
    { 16, 16, 0xff00 }, 0x7d00,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS)|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bnc $disp24 */
  {
    { 1, 1, 1, 1 },
    "bnc24", "bnc",
    { MNEM, ' ', OP (DISP24), 0 },
    { 32, 32, 0xff000000 }, 0xfd000000,
    & fmt_10_bc24_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(RELAX)|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bnc.l $disp24 */
  {
    { 1, 1, 1, 1 },
    "bnc24.l", "bnc.l",
    { MNEM, ' ', OP (DISP24), 0 },
    { 32, 32, 0xff000000 }, 0xfd000000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS)|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bne $src1,$src2,$disp16 */
  {
    { 1, 1, 1, 1 },
    "bne", "bne",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), ',', OP (DISP16), 0 },
    { 32, 32, 0xf0f00000 }, 0xb0100000,
    & fmt_12_beq_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(COND_CTI), { (1<<MACH_M32R) } }
  },
/* bra $disp8 */
  {
    { 1, 1, 1, 1 },
    "bra8", "bra",
    { MNEM, ' ', OP (DISP8), 0 },
    { 16, 16, 0xff00 }, 0x7f00,
    & fmt_18_bra8_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(FILL_SLOT)|A(RELAXABLE)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* bra.s $disp8 */
  {
    { 1, 1, 1, 1 },
    "bra8.s", "bra.s",
    { MNEM, ' ', OP (DISP8), 0 },
    { 16, 16, 0xff00 }, 0x7f00,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* bra $disp24 */
  {
    { 1, 1, 1, 1 },
    "bra24", "bra",
    { MNEM, ' ', OP (DISP24), 0 },
    { 32, 32, 0xff000000 }, 0xff000000,
    & fmt_19_bra24_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(RELAX)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* bra.l $disp24 */
  {
    { 1, 1, 1, 1 },
    "bra24.l", "bra.l",
    { MNEM, ' ', OP (DISP24), 0 },
    { 32, 32, 0xff000000 }, 0xff000000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* cmp $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "cmp", "cmp",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x40,
    & fmt_20_cmp_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* cmpi $src2,$simm16 */
  {
    { 1, 1, 1, 1 },
    "cmpi", "cmpi",
    { MNEM, ' ', OP (SRC2), ',', OP (SIMM16), 0 },
    { 32, 32, 0xfff00000 }, 0x80400000,
    & fmt_21_cmpi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* cmpu $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "cmpu", "cmpu",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x50,
    & fmt_20_cmp_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* cmpui $src2,$uimm16 */
  {
    { 1, 1, 1, 1 },
    "cmpui", "cmpui",
    { MNEM, ' ', OP (SRC2), ',', OP (UIMM16), 0 },
    { 32, 32, 0xfff00000 }, 0x80500000,
    & fmt_22_cmpui_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* div $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "div", "div",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 32, 32, 0xf0f0ffff }, 0x90000000,
    & fmt_24_div_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* divu $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "divu", "divu",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 32, 32, 0xf0f0ffff }, 0x90100000,
    & fmt_24_div_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* rem $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "rem", "rem",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 32, 32, 0xf0f0ffff }, 0x90200000,
    & fmt_24_div_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* remu $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "remu", "remu",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 32, 32, 0xf0f0ffff }, 0x90300000,
    & fmt_24_div_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* jl $sr */
  {
    { 1, 1, 1, 1 },
    "jl", "jl",
    { MNEM, ' ', OP (SR), 0 },
    { 16, 16, 0xfff0 }, 0x1ec0,
    & fmt_26_jl_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(FILL_SLOT)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* jmp $sr */
  {
    { 1, 1, 1, 1 },
    "jmp", "jmp",
    { MNEM, ' ', OP (SR), 0 },
    { 16, 16, 0xfff0 }, 0x1fc0,
    & fmt_27_jmp_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* ld $dr,@$sr */
  {
    { 1, 1, 1, 1 },
    "ld", "ld",
    { MNEM, ' ', OP (DR), ',', '@', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x20c0,
    & fmt_28_ld_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ld $dr,@($sr) */
  {
    { 1, 1, 1, 1 },
    "ld-2", "ld",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ')', 0 },
    { 16, 16, 0xf0f0 }, 0x20c0,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ld $dr,@($slo16,$sr) */
  {
    { 1, 1, 1, 1 },
    "ld-d", "ld",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SLO16), ',', OP (SR), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0c00000,
    & fmt_30_ld_d_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ld $dr,@($sr,$slo16) */
  {
    { 1, 1, 1, 1 },
    "ld-d2", "ld",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ',', OP (SLO16), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0c00000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ldb $dr,@$sr */
  {
    { 1, 1, 1, 1 },
    "ldb", "ldb",
    { MNEM, ' ', OP (DR), ',', '@', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x2080,
    & fmt_32_ldb_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldb $dr,@($sr) */
  {
    { 1, 1, 1, 1 },
    "ldb-2", "ldb",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ')', 0 },
    { 16, 16, 0xf0f0 }, 0x2080,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ldb $dr,@($slo16,$sr) */
  {
    { 1, 1, 1, 1 },
    "ldb-d", "ldb",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SLO16), ',', OP (SR), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0800000,
    & fmt_33_ldb_d_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldb $dr,@($sr,$slo16) */
  {
    { 1, 1, 1, 1 },
    "ldb-d2", "ldb",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ',', OP (SLO16), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0800000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ldh $dr,@$sr */
  {
    { 1, 1, 1, 1 },
    "ldh", "ldh",
    { MNEM, ' ', OP (DR), ',', '@', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x20a0,
    & fmt_34_ldh_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldh $dr,@($sr) */
  {
    { 1, 1, 1, 1 },
    "ldh-2", "ldh",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ')', 0 },
    { 16, 16, 0xf0f0 }, 0x20a0,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ldh $dr,@($slo16,$sr) */
  {
    { 1, 1, 1, 1 },
    "ldh-d", "ldh",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SLO16), ',', OP (SR), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0a00000,
    & fmt_35_ldh_d_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldh $dr,@($sr,$slo16) */
  {
    { 1, 1, 1, 1 },
    "ldh-d2", "ldh",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ',', OP (SLO16), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0a00000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ldub $dr,@$sr */
  {
    { 1, 1, 1, 1 },
    "ldub", "ldub",
    { MNEM, ' ', OP (DR), ',', '@', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x2090,
    & fmt_32_ldb_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldub $dr,@($sr) */
  {
    { 1, 1, 1, 1 },
    "ldub-2", "ldub",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ')', 0 },
    { 16, 16, 0xf0f0 }, 0x2090,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ldub $dr,@($slo16,$sr) */
  {
    { 1, 1, 1, 1 },
    "ldub-d", "ldub",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SLO16), ',', OP (SR), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0900000,
    & fmt_33_ldb_d_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldub $dr,@($sr,$slo16) */
  {
    { 1, 1, 1, 1 },
    "ldub-d2", "ldub",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ',', OP (SLO16), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0900000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* lduh $dr,@$sr */
  {
    { 1, 1, 1, 1 },
    "lduh", "lduh",
    { MNEM, ' ', OP (DR), ',', '@', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x20b0,
    & fmt_34_ldh_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* lduh $dr,@($sr) */
  {
    { 1, 1, 1, 1 },
    "lduh-2", "lduh",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ')', 0 },
    { 16, 16, 0xf0f0 }, 0x20b0,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* lduh $dr,@($slo16,$sr) */
  {
    { 1, 1, 1, 1 },
    "lduh-d", "lduh",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SLO16), ',', OP (SR), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0b00000,
    & fmt_35_ldh_d_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* lduh $dr,@($sr,$slo16) */
  {
    { 1, 1, 1, 1 },
    "lduh-d2", "lduh",
    { MNEM, ' ', OP (DR), ',', '@', '(', OP (SR), ',', OP (SLO16), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0b00000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ld $dr,@$sr+ */
  {
    { 1, 1, 1, 1 },
    "ld-plus", "ld",
    { MNEM, ' ', OP (DR), ',', '@', OP (SR), '+', 0 },
    { 16, 16, 0xf0f0 }, 0x20e0,
    & fmt_36_ld_plus_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ld24 $dr,$uimm24 */
  {
    { 1, 1, 1, 1 },
    "ld24", "ld24",
    { MNEM, ' ', OP (DR), ',', OP (UIMM24), 0 },
    { 32, 32, 0xf0000000 }, 0xe0000000,
    & fmt_37_ld24_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldi $dr,$simm8 */
  {
    { 1, 1, 1, 1 },
    "ldi8", "ldi",
    { MNEM, ' ', OP (DR), ',', OP (SIMM8), 0 },
    { 16, 16, 0xf000 }, 0x6000,
    & fmt_38_ldi8_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldi8 $dr,$simm8 */
  {
    { 1, 1, 1, 1 },
    "ldi8a", "ldi8",
    { MNEM, ' ', OP (DR), ',', OP (SIMM8), 0 },
    { 16, 16, 0xf000 }, 0x6000,
    & fmt_38_ldi8_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* ldi $dr,$hash$slo16 */
  {
    { 1, 1, 1, 1 },
    "ldi16", "ldi",
    { MNEM, ' ', OP (DR), ',', OP (HASH), OP (SLO16), 0 },
    { 32, 32, 0xf0ff0000 }, 0x90f00000,
    & fmt_39_ldi16_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* ldi16 $dr,$hash$slo16 */
  {
    { 1, 1, 1, 1 },
    "ldi16a", "ldi16",
    { MNEM, ' ', OP (DR), ',', OP (HASH), OP (SLO16), 0 },
    { 32, 32, 0xf0ff0000 }, 0x90f00000,
    & fmt_39_ldi16_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* lock $dr,@$sr */
  {
    { 1, 1, 1, 1 },
    "lock", "lock",
    { MNEM, ' ', OP (DR), ',', '@', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x20d0,
    & fmt_40_lock_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* machi $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "machi", "machi",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x3040,
    & fmt_41_machi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* maclo $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "maclo", "maclo",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x3050,
    & fmt_41_machi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* macwhi $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "macwhi", "macwhi",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x3060,
    & fmt_41_machi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* macwlo $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "macwlo", "macwlo",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x3070,
    & fmt_41_machi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mul $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "mul", "mul",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x1060,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mulhi $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "mulhi", "mulhi",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x3000,
    & fmt_43_mulhi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mullo $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "mullo", "mullo",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x3010,
    & fmt_43_mulhi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mulwhi $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "mulwhi", "mulwhi",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x3020,
    & fmt_43_mulhi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mulwlo $src1,$src2 */
  {
    { 1, 1, 1, 1 },
    "mulwlo", "mulwlo",
    { MNEM, ' ', OP (SRC1), ',', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x3030,
    & fmt_43_mulhi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mv $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "mv", "mv",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x1080,
    & fmt_45_mv_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mvfachi $dr */
  {
    { 1, 1, 1, 1 },
    "mvfachi", "mvfachi",
    { MNEM, ' ', OP (DR), 0 },
    { 16, 16, 0xf0ff }, 0x50f0,
    & fmt_46_mvfachi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mvfaclo $dr */
  {
    { 1, 1, 1, 1 },
    "mvfaclo", "mvfaclo",
    { MNEM, ' ', OP (DR), 0 },
    { 16, 16, 0xf0ff }, 0x50f1,
    & fmt_46_mvfachi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mvfacmi $dr */
  {
    { 1, 1, 1, 1 },
    "mvfacmi", "mvfacmi",
    { MNEM, ' ', OP (DR), 0 },
    { 16, 16, 0xf0ff }, 0x50f2,
    & fmt_46_mvfachi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mvfc $dr,$scr */
  {
    { 1, 1, 1, 1 },
    "mvfc", "mvfc",
    { MNEM, ' ', OP (DR), ',', OP (SCR), 0 },
    { 16, 16, 0xf0f0 }, 0x1090,
    & fmt_48_mvfc_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mvtachi $src1 */
  {
    { 1, 1, 1, 1 },
    "mvtachi", "mvtachi",
    { MNEM, ' ', OP (SRC1), 0 },
    { 16, 16, 0xf0ff }, 0x5070,
    & fmt_49_mvtachi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mvtaclo $src1 */
  {
    { 1, 1, 1, 1 },
    "mvtaclo", "mvtaclo",
    { MNEM, ' ', OP (SRC1), 0 },
    { 16, 16, 0xf0ff }, 0x5071,
    & fmt_49_mvtachi_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* mvtc $sr,$dcr */
  {
    { 1, 1, 1, 1 },
    "mvtc", "mvtc",
    { MNEM, ' ', OP (SR), ',', OP (DCR), 0 },
    { 16, 16, 0xf0f0 }, 0x10a0,
    & fmt_51_mvtc_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* neg $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "neg", "neg",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x30,
    & fmt_45_mv_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* nop */
  {
    { 1, 1, 1, 1 },
    "nop", "nop",
    { MNEM, 0 },
    { 16, 16, 0xffff }, 0x7000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* not $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "not", "not",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0xb0,
    & fmt_45_mv_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* rac */
  {
    { 1, 1, 1, 1 },
    "rac", "rac",
    { MNEM, 0 },
    { 16, 16, 0xffff }, 0x5090,
    & fmt_53_rac_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* rach */
  {
    { 1, 1, 1, 1 },
    "rach", "rach",
    { MNEM, 0 },
    { 16, 16, 0xffff }, 0x5080,
    & fmt_53_rac_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* rte */
  {
    { 1, 1, 1, 1 },
    "rte", "rte",
    { MNEM, 0 },
    { 16, 16, 0xffff }, 0x10d6,
    & fmt_57_rte_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* seth $dr,$hash$hi16 */
  {
    { 1, 1, 1, 1 },
    "seth", "seth",
    { MNEM, ' ', OP (DR), ',', OP (HASH), OP (HI16), 0 },
    { 32, 32, 0xf0ff0000 }, 0xd0c00000,
    & fmt_58_seth_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* sll $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "sll", "sll",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x1040,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* sll3 $dr,$sr,$simm16 */
  {
    { 1, 1, 1, 1 },
    "sll3", "sll3",
    { MNEM, ' ', OP (DR), ',', OP (SR), ',', OP (SIMM16), 0 },
    { 32, 32, 0xf0f00000 }, 0x90c00000,
    & fmt_59_sll3_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* slli $dr,$uimm5 */
  {
    { 1, 1, 1, 1 },
    "slli", "slli",
    { MNEM, ' ', OP (DR), ',', OP (UIMM5), 0 },
    { 16, 16, 0xf0e0 }, 0x5040,
    & fmt_60_slli_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* sra $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "sra", "sra",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x1020,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* sra3 $dr,$sr,$simm16 */
  {
    { 1, 1, 1, 1 },
    "sra3", "sra3",
    { MNEM, ' ', OP (DR), ',', OP (SR), ',', OP (SIMM16), 0 },
    { 32, 32, 0xf0f00000 }, 0x90a00000,
    & fmt_59_sll3_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* srai $dr,$uimm5 */
  {
    { 1, 1, 1, 1 },
    "srai", "srai",
    { MNEM, ' ', OP (DR), ',', OP (UIMM5), 0 },
    { 16, 16, 0xf0e0 }, 0x5020,
    & fmt_60_slli_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* srl $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "srl", "srl",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x1000,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* srl3 $dr,$sr,$simm16 */
  {
    { 1, 1, 1, 1 },
    "srl3", "srl3",
    { MNEM, ' ', OP (DR), ',', OP (SR), ',', OP (SIMM16), 0 },
    { 32, 32, 0xf0f00000 }, 0x90800000,
    & fmt_59_sll3_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* srli $dr,$uimm5 */
  {
    { 1, 1, 1, 1 },
    "srli", "srli",
    { MNEM, ' ', OP (DR), ',', OP (UIMM5), 0 },
    { 16, 16, 0xf0e0 }, 0x5000,
    & fmt_60_slli_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* st $src1,@$src2 */
  {
    { 1, 1, 1, 1 },
    "st", "st",
    { MNEM, ' ', OP (SRC1), ',', '@', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x2040,
    & fmt_61_st_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* st $src1,@($src2) */
  {
    { 1, 1, 1, 1 },
    "st-2", "st",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SRC2), ')', 0 },
    { 16, 16, 0xf0f0 }, 0x2040,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* st $src1,@($slo16,$src2) */
  {
    { 1, 1, 1, 1 },
    "st-d", "st",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SLO16), ',', OP (SRC2), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0400000,
    & fmt_63_st_d_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* st $src1,@($src2,$slo16) */
  {
    { 1, 1, 1, 1 },
    "st-d2", "st",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SRC2), ',', OP (SLO16), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0400000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* stb $src1,@$src2 */
  {
    { 1, 1, 1, 1 },
    "stb", "stb",
    { MNEM, ' ', OP (SRC1), ',', '@', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x2000,
    & fmt_65_stb_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* stb $src1,@($src2) */
  {
    { 1, 1, 1, 1 },
    "stb-2", "stb",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SRC2), ')', 0 },
    { 16, 16, 0xf0f0 }, 0x2000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* stb $src1,@($slo16,$src2) */
  {
    { 1, 1, 1, 1 },
    "stb-d", "stb",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SLO16), ',', OP (SRC2), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0000000,
    & fmt_66_stb_d_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* stb $src1,@($src2,$slo16) */
  {
    { 1, 1, 1, 1 },
    "stb-d2", "stb",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SRC2), ',', OP (SLO16), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0000000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* sth $src1,@$src2 */
  {
    { 1, 1, 1, 1 },
    "sth", "sth",
    { MNEM, ' ', OP (SRC1), ',', '@', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x2020,
    & fmt_67_sth_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* sth $src1,@($src2) */
  {
    { 1, 1, 1, 1 },
    "sth-2", "sth",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SRC2), ')', 0 },
    { 16, 16, 0xf0f0 }, 0x2020,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* sth $src1,@($slo16,$src2) */
  {
    { 1, 1, 1, 1 },
    "sth-d", "sth",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SLO16), ',', OP (SRC2), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0200000,
    & fmt_68_sth_d_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* sth $src1,@($src2,$slo16) */
  {
    { 1, 1, 1, 1 },
    "sth-d2", "sth",
    { MNEM, ' ', OP (SRC1), ',', '@', '(', OP (SRC2), ',', OP (SLO16), ')', 0 },
    { 32, 32, 0xf0f00000 }, 0xa0200000,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* st $src1,@+$src2 */
  {
    { 1, 1, 1, 1 },
    "st-plus", "st",
    { MNEM, ' ', OP (SRC1), ',', '@', '+', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x2060,
    & fmt_69_st_plus_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* st $src1,@-$src2 */
  {
    { 1, 1, 1, 1 },
    "st-minus", "st",
    { MNEM, ' ', OP (SRC1), ',', '@', '-', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x2070,
    & fmt_69_st_plus_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* sub $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "sub", "sub",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x20,
    & fmt_0_add_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* subv $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "subv", "subv",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x0,
    & fmt_5_addv_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* subx $dr,$sr */
  {
    { 1, 1, 1, 1 },
    "subx", "subx",
    { MNEM, ' ', OP (DR), ',', OP (SR), 0 },
    { 16, 16, 0xf0f0 }, 0x10,
    & fmt_7_addx_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* trap $uimm4 */
  {
    { 1, 1, 1, 1 },
    "trap", "trap",
    { MNEM, ' ', OP (UIMM4), 0 },
    { 16, 16, 0xfff0 }, 0x10f0,
    & fmt_70_trap_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0|A(FILL_SLOT)|A(UNCOND_CTI), { (1<<MACH_M32R) } }
  },
/* unlock $src1,@$src2 */
  {
    { 1, 1, 1, 1 },
    "unlock", "unlock",
    { MNEM, ' ', OP (SRC1), ',', '@', OP (SRC2), 0 },
    { 16, 16, 0xf0f0 }, 0x2050,
    & fmt_71_unlock_ops[0],
    { CGEN_INSN_NBOOL_ATTRS, 0, { (1<<MACH_M32R) } }
  },
/* push $src1 */
  {
    { 1, 1, 1, 1 },
    "push", "push",
    { MNEM, ' ', OP (SRC1), 0 },
    { 16, 16, 0xf0ff }, 0x207f,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
/* pop $dr */
  {
    { 1, 1, 1, 1 },
    "pop", "pop",
    { MNEM, ' ', OP (DR), 0 },
    { 16, 16, 0xf0ff }, 0x20ef,
    0,
    { CGEN_INSN_NBOOL_ATTRS, 0|A(ALIAS), { (1<<MACH_M32R) } }
  },
};

#undef A
#undef MNEM
#undef OP

CGEN_INSN_TABLE m32r_cgen_insn_table =
{
  & m32r_cgen_insn_table_entries[0],
  sizeof (CGEN_INSN),
  MAX_INSNS,
  NULL,
  m32r_cgen_asm_hash_insn, CGEN_ASM_HASH_SIZE,
  m32r_cgen_dis_hash_insn, CGEN_DIS_HASH_SIZE
};

/* The hash functions are recorded here to help keep assembler code out of
   the disassembler and vice versa.  */

unsigned int
m32r_cgen_asm_hash_insn (insn)
     const char * insn;
{
  return CGEN_ASM_HASH (insn);
}

unsigned int
m32r_cgen_dis_hash_insn (buf, value)
     const char * buf;
     unsigned long value;
{
  return CGEN_DIS_HASH (buf, value);
}

CGEN_OPCODE_DATA m32r_cgen_opcode_data = 
{
  & m32r_cgen_hw_entries[0],
  & m32r_cgen_insn_table,
};

void
m32r_cgen_init_tables (mach)
    int mach;
{
}

/* Main entry point for stuffing values in cgen_fields.  */

void
m32r_cgen_set_operand (opindex, valuep, fields)
     int opindex;
     const long * valuep;
     CGEN_FIELDS * fields;
{
  switch (opindex)
    {
    case M32R_OPERAND_SR :
      fields->f_r2 = * valuep;
      break;
    case M32R_OPERAND_DR :
      fields->f_r1 = * valuep;
      break;
    case M32R_OPERAND_SRC1 :
      fields->f_r1 = * valuep;
      break;
    case M32R_OPERAND_SRC2 :
      fields->f_r2 = * valuep;
      break;
    case M32R_OPERAND_SCR :
      fields->f_r2 = * valuep;
      break;
    case M32R_OPERAND_DCR :
      fields->f_r1 = * valuep;
      break;
    case M32R_OPERAND_SIMM8 :
      fields->f_simm8 = * valuep;
      break;
    case M32R_OPERAND_SIMM16 :
      fields->f_simm16 = * valuep;
      break;
    case M32R_OPERAND_UIMM4 :
      fields->f_uimm4 = * valuep;
      break;
    case M32R_OPERAND_UIMM5 :
      fields->f_uimm5 = * valuep;
      break;
    case M32R_OPERAND_UIMM16 :
      fields->f_uimm16 = * valuep;
      break;
    case M32R_OPERAND_HASH :
      fields->f_nil = * valuep;
      break;
    case M32R_OPERAND_HI16 :
      fields->f_hi16 = * valuep;
      break;
    case M32R_OPERAND_SLO16 :
      fields->f_simm16 = * valuep;
      break;
    case M32R_OPERAND_ULO16 :
      fields->f_uimm16 = * valuep;
      break;
    case M32R_OPERAND_UIMM24 :
      fields->f_uimm24 = * valuep;
      break;
    case M32R_OPERAND_DISP8 :
      fields->f_disp8 = * valuep;
      break;
    case M32R_OPERAND_DISP16 :
      fields->f_disp16 = * valuep;
      break;
    case M32R_OPERAND_DISP24 :
      fields->f_disp24 = * valuep;
      break;

    default :
      fprintf (stderr, "Unrecognized field %d while setting operand.\n",
		       opindex);
      abort ();
  }
}

/* Main entry point for getting values from cgen_fields.  */

long
m32r_cgen_get_operand (opindex, fields)
     int opindex;
     const CGEN_FIELDS * fields;
{
  long value;

  switch (opindex)
    {
    case M32R_OPERAND_SR :
      value = fields->f_r2;
      break;
    case M32R_OPERAND_DR :
      value = fields->f_r1;
      break;
    case M32R_OPERAND_SRC1 :
      value = fields->f_r1;
      break;
    case M32R_OPERAND_SRC2 :
      value = fields->f_r2;
      break;
    case M32R_OPERAND_SCR :
      value = fields->f_r2;
      break;
    case M32R_OPERAND_DCR :
      value = fields->f_r1;
      break;
    case M32R_OPERAND_SIMM8 :
      value = fields->f_simm8;
      break;
    case M32R_OPERAND_SIMM16 :
      value = fields->f_simm16;
      break;
    case M32R_OPERAND_UIMM4 :
      value = fields->f_uimm4;
      break;
    case M32R_OPERAND_UIMM5 :
      value = fields->f_uimm5;
      break;
    case M32R_OPERAND_UIMM16 :
      value = fields->f_uimm16;
      break;
    case M32R_OPERAND_HASH :
      value = fields->f_nil;
      break;
    case M32R_OPERAND_HI16 :
      value = fields->f_hi16;
      break;
    case M32R_OPERAND_SLO16 :
      value = fields->f_simm16;
      break;
    case M32R_OPERAND_ULO16 :
      value = fields->f_uimm16;
      break;
    case M32R_OPERAND_UIMM24 :
      value = fields->f_uimm24;
      break;
    case M32R_OPERAND_DISP8 :
      value = fields->f_disp8;
      break;
    case M32R_OPERAND_DISP16 :
      value = fields->f_disp16;
      break;
    case M32R_OPERAND_DISP24 :
      value = fields->f_disp24;
      break;

    default :
      fprintf (stderr, "Unrecognized field %d while getting operand.\n",
		       opindex);
      abort ();
  }

  return value;
}

