/* tc-fr30.c -- Assembler for the Fujitsu FR30.
   Copyright (C) 1998-2020 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "as.h"
#include "safe-ctype.h"
#include "subsegs.h"
#include "symcat.h"
#include "opcodes/fr30-desc.h"
#include "opcodes/fr30-opc.h"
#include "cgen.h"

/* Structure to hold all of the different components describing
   an individual instruction.  */
typedef struct
{
  const CGEN_INSN *	insn;
  const CGEN_INSN *	orig_insn;
  CGEN_FIELDS		fields;
#if CGEN_INT_INSN_P
  CGEN_INSN_INT         buffer [1];
#define INSN_VALUE(buf) (*(buf))
#else
  unsigned char         buffer [CGEN_MAX_INSN_SIZE];
#define INSN_VALUE(buf) (buf)
#endif
  char *		addr;
  fragS *		frag;
  int                   num_fixups;
  fixS *                fixups [GAS_CGEN_MAX_FIXUPS];
  int                   indices [MAX_OPERAND_INSTANCES];
}
fr30_insn;

const char comment_chars[]        = ";";
const char line_comment_chars[]   = "#";
const char line_separator_chars[] = "|";
const char EXP_CHARS[]            = "eE";
const char FLT_CHARS[]            = "dD";

#define FR30_SHORTOPTS ""
const char * md_shortopts = FR30_SHORTOPTS;

struct option md_longopts[] =
{
  {NULL, no_argument, NULL, 0}
};
size_t md_longopts_size = sizeof (md_longopts);

int
md_parse_option (int c ATTRIBUTE_UNUSED,
		 const char *arg ATTRIBUTE_UNUSED)
{
  switch (c)
    {
    default:
      return 0;
    }
  return 1;
}

void
md_show_usage (FILE * stream)
{
  fprintf (stream, _(" FR30 specific command line options:\n"));
}

/* The target specific pseudo-ops which we support.  */
const pseudo_typeS md_pseudo_table[] =
{
  { "word",	cons,		4 },
  { NULL, 	NULL, 		0 }
};


void
md_begin (void)
{
  /* Initialize the `cgen' interface.  */

  /* Set the machine number and endian.  */
  gas_cgen_cpu_desc = fr30_cgen_cpu_open (CGEN_CPU_OPEN_MACHS, 0,
					  CGEN_CPU_OPEN_ENDIAN,
					  CGEN_ENDIAN_BIG,
					  CGEN_CPU_OPEN_END);
  fr30_cgen_init_asm (gas_cgen_cpu_desc);

  /* This is a callback from cgen to gas to parse operands.  */
  cgen_set_parse_operand_fn (gas_cgen_cpu_desc, gas_cgen_parse_operand);
}

void
md_assemble (char *str)
{
  static int last_insn_had_delay_slot = 0;
  fr30_insn insn;
  char *errmsg;

  /* Initialize GAS's cgen interface for a new instruction.  */
  gas_cgen_init_parse ();

  insn.insn = fr30_cgen_assemble_insn
    (gas_cgen_cpu_desc, str, & insn.fields, insn.buffer, & errmsg);

  if (!insn.insn)
    {
      as_bad ("%s", errmsg);
      return;
    }

  /* Doesn't really matter what we pass for RELAX_P here.  */
  gas_cgen_finish_insn (insn.insn, insn.buffer,
			CGEN_FIELDS_BITSIZE (& insn.fields), 1, NULL);

  /* Warn about invalid insns in delay slots.  */
  if (last_insn_had_delay_slot
      && CGEN_INSN_ATTR_VALUE (insn.insn, CGEN_INSN_NOT_IN_DELAY_SLOT))
    as_warn (_("Instruction %s not allowed in a delay slot."),
	     CGEN_INSN_NAME (insn.insn));

  last_insn_had_delay_slot
    = CGEN_INSN_ATTR_VALUE (insn.insn, CGEN_INSN_DELAY_SLOT);
}

/* The syntax in the manual says constants begin with '#'.
   We just ignore it.  */

void
md_operand (expressionS * expressionP)
{
  if (* input_line_pointer == '#')
    {
      input_line_pointer ++;
      expression (expressionP);
    }
}

valueT
md_section_align (segT segment, valueT size)
{
  int align = bfd_section_alignment (segment);

  return ((size + (1 << align) - 1) & -(1 << align));
}

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}

/* Interface to relax_segment.  */

/* FIXME: Build table by hand, get it working, then machine generate.  */

const relax_typeS md_relax_table[] =
{
/* The fields are:
   1) most positive reach of this state,
   2) most negative reach of this state,
   3) how many bytes this mode will add to the size of the current frag
   4) which index into the table to try if we can't fit into this one.  */

  /* The first entry must be unused because an `rlx_more' value of zero ends
     each list.  */
  {1, 1, 0, 0},

  /* The displacement used by GAS is from the end of the 2 byte insn,
     so we subtract 2 from the following.  */
  /* 16 bit insn, 8 bit disp -> 10 bit range.
     This doesn't handle a branch in the right slot at the border:
     the "& -4" isn't taken into account.  It's not important enough to
     complicate things over it, so we subtract an extra 2 (or + 2 in -ve
     case).  */
  {511 - 2 - 2, -512 - 2 + 2, 0, 2 },
  /* 32 bit insn, 24 bit disp -> 26 bit range.  */
  {0x2000000 - 1 - 2, -0x2000000 - 2, 2, 0 },
  /* Same thing, but with leading nop for alignment.  */
  {0x2000000 - 1 - 2, -0x2000000 - 2, 4, 0 }
};

/* Return an initial guess of the length by which a fragment must grow to
   hold a branch to reach its destination.
   Also updates fr_type/fr_subtype as necessary.

   Called just before doing relaxation.
   Any symbol that is now undefined will not become defined.
   The guess for fr_var is ACTUALLY the growth beyond fr_fix.
   Whatever we do to grow fr_fix or fr_var contributes to our returned value.
   Although it may not be explicit in the frag, pretend fr_var starts with a
   0 value.  */

int
md_estimate_size_before_relax (fragS * fragP, segT segment)
{
  /* The only thing we have to handle here are symbols outside of the
     current segment.  They may be undefined or in a different segment in
     which case linker scripts may place them anywhere.
     However, we can't finish the fragment here and emit the reloc as insn
     alignment requirements may move the insn about.  */

  if (S_GET_SEGMENT (fragP->fr_symbol) != segment)
    {
      /* The symbol is undefined in this segment.
	 Change the relaxation subtype to the max allowable and leave
	 all further handling to md_convert_frag.  */
      fragP->fr_subtype = 2;

      {
	const CGEN_INSN * insn;
	int               i;

	/* Update the recorded insn.
	   Fortunately we don't have to look very far.
	   FIXME: Change this to record in the instruction the next higher
	   relaxable insn to use.  */
	for (i = 0, insn = fragP->fr_cgen.insn; i < 4; i++, insn++)
	  {
	    if ((strcmp (CGEN_INSN_MNEMONIC (insn),
			 CGEN_INSN_MNEMONIC (fragP->fr_cgen.insn))
		 == 0)
		&& CGEN_INSN_ATTR_VALUE (insn, CGEN_INSN_RELAXED))
	      break;
	  }
	if (i == 4)
	  abort ();

	fragP->fr_cgen.insn = insn;
	return 2;
      }
    }

  /* Return the size of the variable part of the frag.  */
  return md_relax_table[fragP->fr_subtype].rlx_length;
}

/* *fragP has been relaxed to its final size, and now needs to have
   the bytes inside it modified to conform to the new size.

   Called after relaxation is finished.
   fragP->fr_type == rs_machine_dependent.
   fragP->fr_subtype is the subtype of what the address relaxed to.  */

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED,
		 segT sec ATTRIBUTE_UNUSED,
		 fragS *fragP ATTRIBUTE_UNUSED)
{
}

/* Functions concerning relocs.  */

/* The location from which a PC relative jump should be calculated,
   given a PC relative reloc.  */

long
md_pcrel_from_section (fixS * fixP, segT sec)
{
  if (fixP->fx_addsy != (symbolS *) NULL
      && (! S_IS_DEFINED (fixP->fx_addsy)
	  || S_GET_SEGMENT (fixP->fx_addsy) != sec))
    /* The symbol is undefined (or is defined but not in this section).
       Let the linker figure it out.  */
    return 0;

  return (fixP->fx_frag->fr_address + fixP->fx_where) & ~1;
}

/* Return the bfd reloc type for OPERAND of INSN at fixup FIXP.
   Returns BFD_RELOC_NONE if no reloc type can be found.
   *FIXP may be modified if desired.  */

bfd_reloc_code_real_type
md_cgen_lookup_reloc (const CGEN_INSN *insn ATTRIBUTE_UNUSED,
		      const CGEN_OPERAND *operand,
		      fixS *fixP)
{
  switch (operand->type)
    {
    case FR30_OPERAND_LABEL9:  fixP->fx_pcrel = 1; return BFD_RELOC_FR30_9_PCREL;
    case FR30_OPERAND_LABEL12: fixP->fx_pcrel = 1; return BFD_RELOC_FR30_12_PCREL;
    case FR30_OPERAND_DISP10:  return BFD_RELOC_FR30_10_IN_8;
    case FR30_OPERAND_DISP9:   return BFD_RELOC_FR30_9_IN_8;
    case FR30_OPERAND_DISP8:   return BFD_RELOC_FR30_8_IN_8;
    case FR30_OPERAND_UDISP6:  return BFD_RELOC_FR30_6_IN_4;
    case FR30_OPERAND_I8:      return BFD_RELOC_8;
    case FR30_OPERAND_I32:     return BFD_RELOC_FR30_48;
    case FR30_OPERAND_I20:     return BFD_RELOC_FR30_20;
    default : /* Avoid -Wall warning.  */
      break;
    }

  return BFD_RELOC_NONE;
}

/* Write a value out to the object file, using the appropriate endianness.  */

void
md_number_to_chars (char * buf, valueT val, int n)
{
  number_to_chars_bigendian (buf, val, n);
}

const char *
md_atof (int type, char * litP, int * sizeP)
{
  return ieee_md_atof (type, litP, sizeP, TRUE);
}

/* Worker function for fr30_is_colon_insn().  */
static int
restore_colon (char *next_i_l_p, char *nul_char)
{
  /* Restore the colon, and advance input_line_pointer to
     the end of the new symbol.  */
  *input_line_pointer = *nul_char;
  input_line_pointer = next_i_l_p;
  *nul_char = *next_i_l_p;
  *next_i_l_p = 0;
  return 1;
}

/* Determines if the symbol starting at START and ending in
   a colon that was at the location pointed to by INPUT_LINE_POINTER
   (but which has now been replaced bu a NUL) is in fact an
   LDI:8, LDI:20, LDI:32, CALL:D. JMP:D, RET:D or Bcc:D instruction.
   If it is, then it restores the colon, advances INPUT_LINE_POINTER
   to the real end of the instruction/symbol, saves the char there to
   NUL_CHAR and pokes a NUL, and returns 1.  Otherwise it returns 0.  */
int
fr30_is_colon_insn (char *start, char *nul_char)
{
  char * i_l_p = input_line_pointer;

  if (*nul_char == '"')
    ++i_l_p;

  /* Check to see if the symbol parsed so far is 'ldi'.  */
  if (   (start[0] != 'l' && start[0] != 'L')
      || (start[1] != 'd' && start[1] != 'D')
      || (start[2] != 'i' && start[2] != 'I')
      || start[3] != 0)
    {
      /* Nope - check to see a 'd' follows the colon.  */
      if (   (i_l_p[1] == 'd' || i_l_p[1] == 'D')
	  && (i_l_p[2] == ' ' || i_l_p[2] == '\t' || i_l_p[2] == '\n'))
	{
	  /* Yup - it might be delay slot instruction.  */
	  int           i;
	  static const char * delay_insns [] =
	  {
	    "call", "jmp", "ret", "bra", "bno",
	    "beq",  "bne", "bc",  "bnc", "bn",
	    "bp",   "bv",  "bnv", "blt", "bge",
	    "ble",  "bgt", "bls", "bhi"
	  };

	  for (i = sizeof (delay_insns) / sizeof (delay_insns[0]); i--;)
	    {
	      const char * insn = delay_insns[i];
	      int    len  = strlen (insn);

	      if (start [len] != 0)
		continue;

	      while (len --)
		if (TOLOWER (start [len]) != insn [len])
		  break;

	      if (len == -1)
		return restore_colon (i_l_p + 1, nul_char);
	    }
	}

      /* Nope - it is a normal label.  */
      return 0;
    }

  /* Check to see if the text following the colon is '8'.  */
  if (i_l_p[1] == '8' && (i_l_p[2] == ' ' || i_l_p[2] == '\t'))
    return restore_colon (i_l_p + 2, nul_char);

  /* Check to see if the text following the colon is '20'.  */
  else if (i_l_p[1] == '2' && i_l_p[2] =='0'
	   && (i_l_p[3] == ' ' || i_l_p[3] == '\t'))
    return restore_colon (i_l_p + 3, nul_char);

  /* Check to see if the text following the colon is '32'.  */
  else if (i_l_p[1] == '3' && i_l_p[2] =='2'
	   && (i_l_p[3] == ' ' || i_l_p[3] == '\t'))
    return restore_colon (i_l_p + 3, nul_char);

  return 0;
}

bfd_boolean
fr30_fix_adjustable (fixS * fixP)
{
  /* We need the symbol name for the VTABLE entries.  */
  if (fixP->fx_r_type == BFD_RELOC_VTABLE_INHERIT
      || fixP->fx_r_type == BFD_RELOC_VTABLE_ENTRY)
    return 0;

  return 1;
}
