/* Instruction printing code for the ARC.
   Copyright (C) 1994-2016 Free Software Foundation, Inc.

   Contributed by Claudiu Zissulescu (claziss@synopsys.com)

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include <assert.h>
#include "dis-asm.h"
#include "opcode/arc.h"
#include "elf/arc.h"
#include "arc-dis.h"
#include "arc-ext.h"
#include "elf-bfd.h"
#include "libiberty.h"
#include "opintl.h"

/* Structure used to iterate over, and extract the values for, operands of
   an opcode.  */

struct arc_operand_iterator
{
  enum
    {
      OPERAND_ITERATOR_STANDARD,
      OPERAND_ITERATOR_LONG
    } mode;

  /* The array of 32-bit values that make up this instruction.  All
     required values have been pre-loaded into this array during the
     find_format call.  */
  unsigned *insn;

  union
  {
    struct
    {
      /* The opcode this iterator is operating on.  */
      const struct arc_opcode *opcode;

      /* The index into the opcodes operand index list.  */
      const unsigned char *opidx;
    } standard;

    struct
    {
      /* The long instruction opcode this iterator is operating on.  */
      const struct arc_long_opcode *long_opcode;

      /* Two indexes into the opcodes operand index lists.  */
      const unsigned char *opidx_base, *opidx_limm;
    } long_insn;
  } state;
};

/* Globals variables.  */

static const char * const regnames[64] =
{
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "r24", "r25", "gp", "fp", "sp", "ilink", "r30", "blink",

  "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",
  "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",
  "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",
  "r56", "r57", "ACCL", "ACCH", "lp_count", "rezerved", "LIMM", "pcl"
};

static const char * const addrtypenames[ARC_NUM_ADDRTYPES] =
{
  "bd", "jid", "lbd", "mbd", "sd", "sm", "xa", "xd",
  "cd", "cbd", "cjid", "clbd", "cm", "csd", "cxa", "cxd"
};

static int addrtypenames_max = ARC_NUM_ADDRTYPES - 1;

static const char * const addrtypeunknown = "unknown";

/* This structure keeps track which instruction class(es)
   should be ignored durring disassembling.  */

typedef struct skipclass
{
  insn_class_t     insn_class;
  insn_subclass_t  subclass;
  struct skipclass *nxt;
} skipclass_t, *linkclass;

/* Intial classes of instructions to be consider first when
   disassembling.  */
static linkclass decodelist = NULL;

/* Macros section.  */

#ifdef DEBUG
# define pr_debug(fmt, args...) fprintf (stderr, fmt, ##args)
#else
# define pr_debug(fmt, args...)
#endif

#define ARRANGE_ENDIAN(info, buf)					\
  (info->endian == BFD_ENDIAN_LITTLE ? bfd_getm32 (bfd_getl32 (buf))	\
   : bfd_getb32 (buf))

#define BITS(word,s,e)  (((word) << (sizeof (word) * 8 - 1 - e)) >>	\
			 (s + (sizeof (word) * 8 - 1 - e)))
#define OPCODE(word)	(BITS ((word), 27, 31))

#define OPCODE_AC(word)   (BITS ((word), 11, 15))

/* Functions implementation.  */

/* Return TRUE when two classes are not opcode conflicting.  */

static bfd_boolean
is_compatible_p (insn_class_t     classA,
		 insn_subclass_t  sclassA,
		 insn_class_t     classB,
		 insn_subclass_t  sclassB)
{
  if (classA == DSP && sclassB == DPX)
    return FALSE;
  if (sclassA == DPX && classB == DSP)
    return FALSE;
  return TRUE;
}

/* Add a new element to the decode list.  */

static void
add_to_decodelist (insn_class_t     insn_class,
		   insn_subclass_t  subclass)
{
  linkclass t = (linkclass) xmalloc (sizeof (skipclass_t));

  t->insn_class = insn_class;
  t->subclass = subclass;
  t->nxt = decodelist;
  decodelist = t;
}

/* Return TRUE if we need to skip the opcode from being
   disassembled.  */

static bfd_boolean
skip_this_opcode (const struct arc_opcode *  opcode,
		  struct disassemble_info *  info)
{
  linkclass t = decodelist;
  bfd_boolean addme = TRUE;

  /* Check opcode for major 0x06, return if it is not in.  */
  if (OPCODE (opcode->opcode) != 0x06)
    return FALSE;

  while (t != NULL
	 && is_compatible_p (t->insn_class, t->subclass,
			     opcode->insn_class, opcode->subclass))
    {
      if ((t->insn_class == opcode->insn_class)
	  && (t->subclass == opcode->subclass))
	addme = FALSE;
      t = t->nxt;
    }

  /* If we found an incompatibility then we must skip.  */
  if (t != NULL)
    return TRUE;

  /* Even if we do not precisely know the if the right mnemonics
     is correctly displayed, keep the disassmbled code class
     consistent.  */
  if (addme)
    {
      switch (opcode->insn_class)
	{
	case DSP:
	case FLOAT:
	  /* Add to the conflict list only the classes which
	     counts.  */
	  add_to_decodelist (opcode->insn_class, opcode->subclass);
	  /* Warn if we have to decode an opcode and no preferred
	     classes have been chosen.  */
	  info->fprintf_func (info->stream, _("\n\
Warning: disassembly may be wrong due to guessed opcode class choice.\n\
 Use -M<class[,class]> to select the correct opcode class(es).\n\t\t\t\t"));
	  break;
	default:
	  break;
	}
    }
  return FALSE;
}

static bfd_vma
bfd_getm32 (unsigned int data)
{
  bfd_vma value = 0;

  value = ((data & 0xff00) | (data & 0xff)) << 16;
  value |= ((data & 0xff0000) | (data & 0xff000000)) >> 16;
  return value;
}

static bfd_boolean
special_flag_p (const char *opname,
		const char *flgname)
{
  const struct arc_flag_special *flg_spec;
  unsigned i, j, flgidx;

  for (i = 0; i < arc_num_flag_special; i++)
    {
      flg_spec = &arc_flag_special_cases[i];

      if (strcmp (opname, flg_spec->name))
	continue;

      /* Found potential special case instruction.  */
      for (j=0;; ++j)
	{
	  flgidx = flg_spec->flags[j];
	  if (flgidx == 0)
	    break; /* End of the array.  */

	  if (strcmp (flgname, arc_flag_operands[flgidx].name) == 0)
	    return TRUE;
	}
    }
  return FALSE;
}

/* Find opcode from ARC_TABLE given the instruction described by INSN and
   INSNLEN.  The ISA_MASK restricts the possible matches in ARC_TABLE.  */

static const struct arc_opcode *
find_format_from_table (struct disassemble_info *info,
			const struct arc_opcode *arc_table,
                        unsigned *insn,
			unsigned int insn_len,
                        unsigned isa_mask,
			bfd_boolean *has_limm,
			bfd_boolean overlaps)
{
  unsigned int i = 0;
  const struct arc_opcode *opcode = NULL;
  const unsigned char *opidx;
  const unsigned char *flgidx;

  do
    {
      bfd_boolean invalid = FALSE;

      opcode = &arc_table[i++];

      if (ARC_SHORT (opcode->mask) && (insn_len == 2))
	{
	  if (OPCODE_AC (opcode->opcode) != OPCODE_AC (insn[0]))
	    continue;
	}
      else if (!ARC_SHORT (opcode->mask) && (insn_len == 4))
	{
	  if (OPCODE (opcode->opcode) != OPCODE (insn[0]))
	    continue;
	}
      else
	continue;

      if ((insn[0] ^ opcode->opcode) & opcode->mask)
	continue;

      if (!(opcode->cpu & isa_mask))
	continue;

      *has_limm = FALSE;

      /* Possible candidate, check the operands.  */
      for (opidx = opcode->operands; *opidx; opidx++)
	{
	  int value;
	  const struct arc_operand *operand = &arc_operands[*opidx];

	  if (operand->flags & ARC_OPERAND_FAKE)
	    continue;

	  if (operand->extract)
	    value = (*operand->extract) (insn[0], &invalid);
	  else
	    value = (insn[0] >> operand->shift) & ((1 << operand->bits) - 1);

	  /* Check for LIMM indicator.  If it is there, then make sure
	     we pick the right format.  */
	  if (operand->flags & ARC_OPERAND_IR
	      && !(operand->flags & ARC_OPERAND_LIMM))
	    {
	      if ((value == 0x3E && insn_len == 4)
		  || (value == 0x1E && insn_len == 2))
		{
		  invalid = TRUE;
		  break;
		}
	    }

	  if (operand->flags & ARC_OPERAND_LIMM
	      && !(operand->flags & ARC_OPERAND_DUPLICATE))
	    *has_limm = TRUE;
	}

      /* Check the flags.  */
      for (flgidx = opcode->flags; *flgidx; flgidx++)
	{
	  /* Get a valid flag class.  */
	  const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
	  const unsigned *flgopridx;
	  int foundA = 0, foundB = 0;
	  unsigned int value;

	  /* Check first the extensions.  */
	  if (cl_flags->flag_class & F_CLASS_EXTEND)
	    {
	      value = (insn[0] & 0x1F);
	      if (arcExtMap_condCodeName (value))
		continue;
	    }

	  for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	    {
	      const struct arc_flag_operand *flg_operand =
		&arc_flag_operands[*flgopridx];

	      value = (insn[0] >> flg_operand->shift)
		& ((1 << flg_operand->bits) - 1);
	      if (value == flg_operand->code)
		foundA = 1;
	      if (value)
		foundB = 1;
	    }

	  if (!foundA && foundB)
	    {
	      invalid = TRUE;
	      break;
	    }
	}

      if (invalid)
	continue;

      if (insn_len == 4
	  && overlaps
	  && skip_this_opcode (opcode, info))
	continue;

      /* The instruction is valid.  */
      return opcode;
    }
  while (opcode->mask);

  return NULL;
}

/* Find long instructions matching values in INSN array.  */

static const struct arc_long_opcode *
find_format_long_instructions (unsigned *insn,
                               unsigned int *insn_len,
                               unsigned isa_mask,
                               bfd_vma memaddr,
                               struct disassemble_info *info)
{
  unsigned int i;
  unsigned limm = 0;
  bfd_boolean limm_loaded = FALSE;

  for (i = 0; i < arc_num_long_opcodes; ++i)
    {
      bfd_byte buffer[4];
      int status;
      const struct arc_opcode *opcode;

      opcode = &arc_long_opcodes[i].base_opcode;

      if (ARC_SHORT (opcode->mask) && (*insn_len == 2))
        {
          if (OPCODE_AC (opcode->opcode) != OPCODE_AC (insn[0]))
            continue;
        }
      else if (!ARC_SHORT (opcode->mask) && (*insn_len == 4))
        {
          if (OPCODE (opcode->opcode) != OPCODE (insn[0]))
            continue;
        }
      else
        continue;

      if ((insn[0] ^ opcode->opcode) & opcode->mask)
        continue;

      if (!(opcode->cpu & isa_mask))
        continue;

      if (!limm_loaded)
        {
          status = (*info->read_memory_func) (memaddr + *insn_len, buffer,
                                              4, info);
          if (status != 0)
            return NULL;

          limm = ARRANGE_ENDIAN (info, buffer);
          limm_loaded = TRUE;
        }

      /* Check the second word using the mask and template.  */
      if ((limm & arc_long_opcodes[i].limm_mask)
          != arc_long_opcodes[i].limm_template)
        continue;

      (*insn_len) += 4;
      insn[1] = limm;
      return &arc_long_opcodes[i];
    }

  return NULL;
}

/* Find opcode for INSN, trying various different sources.  The instruction
   length in INSN_LEN will be updated if the instruction requires a LIMM
   extension, and the additional values loaded into the INSN array (which
   must be big enough).

   A pointer to the opcode is placed into OPCODE_RESULT, and ITER is
   initialised, ready to iterate over the operands of the found opcode.

   This function returns TRUE in almost all cases, FALSE is reserved to
   indicate an error (failing to find an opcode is not an error) a
   returned result of FALSE would indicate that the disassembler can't
   continue.

   If no matching opcode is found then the returned result will be TRUE,
   the value placed into OPCODE_RESULT will be NULL, ITER will be
   undefined, and INSN_LEN will be unchanged.

   If a matching opcode is found, then the returned result will be TRUE,
   the opcode pointer is placed into OPCODE_RESULT, INSN_LEN will be
   increased by 4 if the instruction requires a LIMM, and the LIMM value
   will have been loaded into the INSN[1].  Finally, ITER will have been
   initialised so that calls to OPERAND_ITERATOR_NEXT will iterate over
   the opcode's operands.  */

static bfd_boolean
find_format (bfd_vma                       memaddr,
	     unsigned *                    insn,
	     unsigned int *                insn_len,
             unsigned                      isa_mask,
	     struct disassemble_info *     info,
             const struct arc_opcode **    opcode_result,
             struct arc_operand_iterator * iter)
{
  const struct arc_opcode *opcode = NULL;
  bfd_boolean needs_limm;
  const extInstruction_t *einsn;

  /* First, try the extension instructions.  */
  einsn = arcExtMap_insn (OPCODE (insn[0]), insn[0]);
  if (einsn != NULL)
    {
      const char *errmsg = NULL;

      opcode = arcExtMap_genOpcode (einsn, isa_mask, &errmsg);
      if (opcode == NULL)
	{
	  (*info->fprintf_func) (info->stream, "\
An error occured while generating the extension instruction operations");
	  *opcode_result = NULL;
	  return FALSE;
	}

      opcode = find_format_from_table (info, opcode, insn, *insn_len,
				       isa_mask, &needs_limm, FALSE);
    }

  /* Then, try finding the first match in the opcode table.  */
  if (opcode == NULL)
    opcode = find_format_from_table (info, arc_opcodes, insn, *insn_len,
				     isa_mask, &needs_limm, TRUE);

  if (needs_limm && opcode != NULL)
    {
      bfd_byte buffer[4];
      int status;

      status = (*info->read_memory_func) (memaddr + *insn_len, buffer,
                                          4, info);
      if (status != 0)
        {
          opcode = NULL;
        }
      else
        {
          insn[1] = ARRANGE_ENDIAN (info, buffer);
          *insn_len += 4;
        }
    }

  if (opcode == NULL)
    {
      const struct arc_long_opcode *long_opcode;

      /* No instruction found yet, try the long instructions.  */
      long_opcode =
        find_format_long_instructions (insn, insn_len, isa_mask,
                                       memaddr, info);

      if (long_opcode != NULL)
        {
          iter->mode = OPERAND_ITERATOR_LONG;
          iter->insn = insn;
          iter->state.long_insn.long_opcode = long_opcode;
          iter->state.long_insn.opidx_base =
            long_opcode->base_opcode.operands;
          iter->state.long_insn.opidx_limm =
            long_opcode->operands;
          opcode = &long_opcode->base_opcode;
        }
    }
  else
    {
      iter->mode = OPERAND_ITERATOR_STANDARD;
      iter->insn = insn;
      iter->state.standard.opcode = opcode;
      iter->state.standard.opidx = opcode->operands;
    }

  *opcode_result = opcode;
  return TRUE;
}

static void
print_flags (const struct arc_opcode *opcode,
	     unsigned *insn,
	     struct disassemble_info *info)
{
  const unsigned char *flgidx;
  unsigned int value;

  /* Now extract and print the flags.  */
  for (flgidx = opcode->flags; *flgidx; flgidx++)
    {
      /* Get a valid flag class.  */
      const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
      const unsigned *flgopridx;

      /* Check first the extensions.  */
      if (cl_flags->flag_class & F_CLASS_EXTEND)
	{
	  const char *name;
	  value = (insn[0] & 0x1F);

	  name = arcExtMap_condCodeName (value);
	  if (name)
	    {
	      (*info->fprintf_func) (info->stream, ".%s", name);
	      continue;
	    }
	}

      for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	{
	  const struct arc_flag_operand *flg_operand =
	    &arc_flag_operands[*flgopridx];

	  if (!flg_operand->favail)
	    continue;

	  value = (insn[0] >> flg_operand->shift)
	    & ((1 << flg_operand->bits) - 1);
	  if (value == flg_operand->code)
	    {
	       /* FIXME!: print correctly nt/t flag.  */
	      if (!special_flag_p (opcode->name, flg_operand->name))
		(*info->fprintf_func) (info->stream, ".");
	      else if (info->insn_type == dis_dref)
		{
		  switch (flg_operand->name[0])
		    {
		    case 'b':
		      info->data_size = 1;
		      break;
		    case 'h':
		    case 'w':
		      info->data_size = 2;
		      break;
		    default:
		      info->data_size = 4;
		      break;
		    }
		}
	      if (flg_operand->name[0] == 'd'
		  && flg_operand->name[1] == 0)
		info->branch_delay_insns = 1;

	      /* Check if it is a conditional flag.  */
	      if (cl_flags->flag_class & F_CLASS_COND)
		{
		  if (info->insn_type == dis_jsr)
		    info->insn_type = dis_condjsr;
		  else if (info->insn_type == dis_branch)
		    info->insn_type = dis_condbranch;
		}

	      (*info->fprintf_func) (info->stream, "%s", flg_operand->name);
	    }
	}
    }
}

static const char *
get_auxreg (const struct arc_opcode *opcode,
	    int value,
	    unsigned isa_mask)
{
  const char *name;
  unsigned int i;
  const struct arc_aux_reg *auxr = &arc_aux_regs[0];

  if (opcode->insn_class != AUXREG)
    return NULL;

  name = arcExtMap_auxRegName (value);
  if (name)
    return name;

  for (i = 0; i < arc_num_aux_regs; i++, auxr++)
    {
      if (!(auxr->cpu & isa_mask))
	continue;

      if (auxr->subclass != NONE)
	return NULL;

      if (auxr->address == value)
	return auxr->name;
    }
  return NULL;
}

/* Convert a value representing an address type to a string used to refer to
   the address type in assembly code.  */

static const char *
get_addrtype (int value)
{
  if (value < 0 || value > addrtypenames_max)
    return addrtypeunknown;

  return addrtypenames[value];
}

/* Calculate the instruction length for an instruction starting with MSB
   and LSB, the most and least significant byte.  The ISA_MASK is used to
   filter the instructions considered to only those that are part of the
   current architecture.

   The instruction lengths are calculated from the ARC_OPCODE table, and
   cached for later use.  */

static unsigned int
arc_insn_length (bfd_byte msb, bfd_byte lsb, struct disassemble_info *info)
{
  bfd_byte major_opcode = msb >> 3;

  switch (info->mach)
    {
    case bfd_mach_arc_arc700:
      /* The nps400 extension set requires this special casing of the
	 instruction length calculation.  Right now this is not causing any
	 problems as none of the known extensions overlap in opcode space,
	 but, if they ever do then we might need to start carrying
	 information around in the elf about which extensions are in use.  */
      if (major_opcode == 0xb)
        {
          bfd_byte minor_opcode = lsb & 0x1f;

          if (minor_opcode < 4)
            return 2;
        }
    case bfd_mach_arc_arc600:
      return (major_opcode > 0xb) ? 2 : 4;
      break;

    case bfd_mach_arc_arcv2:
      return (major_opcode > 0x7) ? 2 : 4;
      break;

    default:
      abort ();
    }
}

/* Extract and return the value of OPERAND from the instruction whose value
   is held in the array INSN.  */

static int
extract_operand_value (const struct arc_operand *operand, unsigned *insn)
{
  int value;

  /* Read the limm operand, if required.  */
  if (operand->flags & ARC_OPERAND_LIMM)
    /* The second part of the instruction value will have been loaded as
       part of the find_format call made earlier.  */
    value = insn[1];
  else
    {
      if (operand->extract)
        value = (*operand->extract) (insn[0], (int *) NULL);
      else
        {
          if (operand->flags & ARC_OPERAND_ALIGNED32)
            {
              value = (insn[0] >> operand->shift)
                & ((1 << (operand->bits - 2)) - 1);
              value = value << 2;
            }
          else
            {
              value = (insn[0] >> operand->shift) & ((1 << operand->bits) - 1);
            }
          if (operand->flags & ARC_OPERAND_SIGNED)
            {
              int signbit = 1 << (operand->bits - 1);
              value = (value ^ signbit) - signbit;
            }
        }
    }

  return value;
}

/* Find the next operand, and the operands value from ITER.  Return TRUE if
   there is another operand, otherwise return FALSE.  If there is an
   operand returned then the operand is placed into OPERAND, and the value
   into VALUE.  If there is no operand returned then OPERAND and VALUE are
   unchanged.  */

static bfd_boolean
operand_iterator_next (struct arc_operand_iterator *iter,
                       const struct arc_operand **operand,
                       int *value)
{
  if (iter->mode == OPERAND_ITERATOR_STANDARD)
    {
      if (*iter->state.standard.opidx == 0)
        {
          *operand = NULL;
          return FALSE;
        }

      *operand = &arc_operands[*iter->state.standard.opidx];
      *value = extract_operand_value (*operand, iter->insn);
      iter->state.standard.opidx++;
    }
  else
    {
      const struct arc_operand *operand_base, *operand_limm;
      int value_base, value_limm;

      if (*iter->state.long_insn.opidx_limm == 0)
        {
          *operand = NULL;
          return FALSE;
        }

      operand_base = &arc_operands[*iter->state.long_insn.opidx_base];
      operand_limm = &arc_operands[*iter->state.long_insn.opidx_limm];

      if (operand_base->flags & ARC_OPERAND_LIMM)
        {
          /* We've reached the end of the operand list.  */
          *operand = NULL;
          return FALSE;
        }

      value_base = value_limm = 0;
      if (!(operand_limm->flags & ARC_OPERAND_IGNORE))
        {
          /* This should never happen.  If it does then the use of
             extract_operand_value below will access memory beyond
             the insn array.  */
          assert ((operand_limm->flags & ARC_OPERAND_LIMM) == 0);

          *operand = operand_limm;
          value_limm = extract_operand_value (*operand, &iter->insn[1]);
        }

      if (!(operand_base->flags & ARC_OPERAND_IGNORE))
        {
          *operand = operand_base;
          value_base = extract_operand_value (*operand, iter->insn);
        }

      /* This is a bit of a fudge.  There's no reason why simply ORing
         together the two values is the right thing to do, however, for all
         the cases we currently have, it is the right thing, so, for now,
         I've put off solving the more complex problem.  */
      *value = value_base | value_limm;

      iter->state.long_insn.opidx_base++;
      iter->state.long_insn.opidx_limm++;
    }
  return TRUE;
}

/* Helper for parsing the options.  */

static void
parse_option (char *option)
{
  if (CONST_STRNEQ (option, "dsp"))
    add_to_decodelist (DSP, NONE);

  else if (CONST_STRNEQ (option, "spfp"))
    add_to_decodelist (FLOAT, SPX);

  else if (CONST_STRNEQ (option, "dpfp"))
    add_to_decodelist (FLOAT, DPX);

  else if (CONST_STRNEQ (option, "quarkse_em"))
    add_to_decodelist (FLOAT, QUARKSE);

  else if (CONST_STRNEQ (option, "fpuda"))
    add_to_decodelist (FLOAT, DPA);

  else if (CONST_STRNEQ (option, "fpud"))
    {
      add_to_decodelist (FLOAT, SP);
      add_to_decodelist (FLOAT, CVT);
    }

  else if (CONST_STRNEQ (option, "fpus"))
    {
      add_to_decodelist (FLOAT, DP);
      add_to_decodelist (FLOAT, CVT);
    }
  else
    fprintf (stderr, _("Unrecognised disassembler option: %s\n"), option);
}

/* Go over the options list and parse it.  */

static void
parse_disassembler_options (char *options)
{
  if (options == NULL)
    return;

  while (*options)
    {
      /* Skip empty options.  */
      if (*options == ',')
	{
	  ++ options;
	  continue;
	}

      parse_option (options);

      while (*options != ',' && *options != '\0')
	++ options;
    }
}

/* Disassemble ARC instructions.  */

static int
print_insn_arc (bfd_vma memaddr,
		struct disassemble_info *info)
{
  bfd_byte buffer[4];
  unsigned int lowbyte, highbyte;
  int status;
  unsigned int insn_len;
  unsigned insn[2] = { 0, 0 };
  unsigned isa_mask;
  const struct arc_opcode *opcode;
  bfd_boolean need_comma;
  bfd_boolean open_braket;
  int size;
  const struct arc_operand *operand;
  int value;
  struct arc_operand_iterator iter;
  Elf_Internal_Ehdr *header = NULL;

  if (info->disassembler_options)
    {
      parse_disassembler_options (info->disassembler_options);

      /* Avoid repeated parsing of the options.  */
      info->disassembler_options = NULL;
    }

  memset (&iter, 0, sizeof (iter));
  lowbyte  = ((info->endian == BFD_ENDIAN_LITTLE) ? 1 : 0);
  highbyte = ((info->endian == BFD_ENDIAN_LITTLE) ? 0 : 1);

  if (info->section && info->section->owner)
    header = elf_elfheader (info->section->owner);

  switch (info->mach)
    {
    case bfd_mach_arc_arc700:
      isa_mask = ARC_OPCODE_ARC700;
      break;

    case bfd_mach_arc_arc600:
      isa_mask = ARC_OPCODE_ARC600;
      break;

    case bfd_mach_arc_arcv2:
    default:
      isa_mask = ARC_OPCODE_ARCv2EM;
      if ((header->e_flags & EF_ARC_MACH_MSK) == EF_ARC_CPU_ARCV2HS)
	{
	  isa_mask = ARC_OPCODE_ARCv2HS;
	  /* FPU instructions are not extensions for HS.  */
	  add_to_decodelist (FLOAT, SP);
	  add_to_decodelist (FLOAT, DP);
	  add_to_decodelist (FLOAT, CVT);
	}
      break;
    }

  /* This variable may be set by the instruction decoder.  It suggests
     the number of bytes objdump should display on a single line.  If
     the instruction decoder sets this, it should always set it to
     the same value in order to get reasonable looking output.  */

  info->bytes_per_line  = 8;

  /* In the next lines, we set two info variables control the way
     objdump displays the raw data.  For example, if bytes_per_line is
     8 and bytes_per_chunk is 4, the output will look like this:
     00:   00000000 00000000
     with the chunks displayed according to "display_endian".  */

  if (info->section
      && !(info->section->flags & SEC_CODE))
    {
      /* This is not a CODE section.  */
      switch (info->section->size)
	{
	case 1:
	case 2:
	case 4:
	  size = info->section->size;
	  break;
	default:
	  size = (info->section->size & 0x01) ? 1 : 4;
	  break;
	}
      info->bytes_per_chunk = 1;
      info->display_endian = info->endian;
    }
  else
    {
      size = 2;
      info->bytes_per_chunk = 2;
      info->display_endian = info->endian;
    }

  /* Read the insn into a host word.  */
  status = (*info->read_memory_func) (memaddr, buffer, size, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }

  if (info->section
      && !(info->section->flags & SEC_CODE))
    {
      /* Data section.  */
      unsigned long data;

      data = bfd_get_bits (buffer, size * 8,
			   info->display_endian == BFD_ENDIAN_BIG);
      switch (size)
	{
	case 1:
	  (*info->fprintf_func) (info->stream, ".byte\t0x%02lx", data);
	  break;
	case 2:
	  (*info->fprintf_func) (info->stream, ".short\t0x%04lx", data);
	  break;
	case 4:
	  (*info->fprintf_func) (info->stream, ".word\t0x%08lx", data);
	  break;
	default:
	  abort ();
	}
      return size;
    }

  insn_len = arc_insn_length (buffer[lowbyte], buffer[highbyte], info);
  pr_debug ("instruction length = %d bytes\n", insn_len);

  switch (insn_len)
    {
    case 2:
      insn[0] = (buffer[lowbyte] << 8) | buffer[highbyte];
      break;

    default:
      /* An unknown instruction is treated as being length 4.  This is
         possibly not the best solution, but matches the behaviour that was
         in place before the table based instruction length look-up was
         introduced.  */
    case 4:
      /* This is a long instruction: Read the remaning 2 bytes.  */
      status = (*info->read_memory_func) (memaddr + 2, &buffer[2], 2, info);
      if (status != 0)
	{
	  (*info->memory_error_func) (status, memaddr + 2, info);
	  return -1;
	}
      insn[0] = ARRANGE_ENDIAN (info, buffer);
      break;
    }

  /* Set some defaults for the insn info.  */
  info->insn_info_valid    = 1;
  info->branch_delay_insns = 0;
  info->data_size	   = 0;
  info->insn_type	   = dis_nonbranch;
  info->target		   = 0;
  info->target2		   = 0;

  /* FIXME to be moved in dissasemble_init_for_target.  */
  info->disassembler_needs_relocs = TRUE;

  /* Find the first match in the opcode table.  */
  if (!find_format (memaddr, insn, &insn_len, isa_mask, info, &opcode, &iter))
    return -1;

  if (!opcode)
    {
      if (insn_len == 2)
        (*info->fprintf_func) (info->stream, ".long %#04x", insn[0]);
      else
        (*info->fprintf_func) (info->stream, ".long %#08x", insn[0]);

      info->insn_type = dis_noninsn;
      return insn_len;
    }

  /* Print the mnemonic.  */
  (*info->fprintf_func) (info->stream, "%s", opcode->name);

  /* Preselect the insn class.  */
  switch (opcode->insn_class)
    {
    case BRANCH:
    case JUMP:
      if (!strncmp (opcode->name, "bl", 2)
	  || !strncmp (opcode->name, "jl", 2))
	{
	  if (opcode->subclass == COND)
	    info->insn_type = dis_condjsr;
	  else
	    info->insn_type = dis_jsr;
	}
      else
	{
	  if (opcode->subclass == COND)
	    info->insn_type = dis_condbranch;
	  else
	    info->insn_type = dis_branch;
	}
      break;
    case MEMORY:
      info->insn_type = dis_dref; /* FIXME! DB indicates mov as memory! */
      break;
    default:
      info->insn_type = dis_nonbranch;
      break;
    }

  pr_debug ("%s: 0x%08x\n", opcode->name, opcode->opcode);

  print_flags (opcode, insn, info);

  if (opcode->operands[0] != 0)
    (*info->fprintf_func) (info->stream, "\t");

  need_comma = FALSE;
  open_braket = FALSE;

  /* Now extract and print the operands.  */
  operand = NULL;
  while (operand_iterator_next (&iter, &operand, &value))
    {
      if (open_braket && (operand->flags & ARC_OPERAND_BRAKET))
	{
	  (*info->fprintf_func) (info->stream, "]");
	  open_braket = FALSE;
	  continue;
	}

      /* Only take input from real operands.  */
      if (ARC_OPERAND_IS_FAKE (operand))
	continue;

      if ((operand->flags & ARC_OPERAND_IGNORE)
	  && (operand->flags & ARC_OPERAND_IR)
          && value == -1)
	continue;

      if (operand->flags & ARC_OPERAND_COLON)
        {
          (*info->fprintf_func) (info->stream, ":");
          continue;
        }

      if (need_comma)
	(*info->fprintf_func) (info->stream, ",");

      if (!open_braket && (operand->flags & ARC_OPERAND_BRAKET))
	{
	  (*info->fprintf_func) (info->stream, "[");
	  open_braket = TRUE;
	  need_comma = FALSE;
	  continue;
	}

      need_comma = TRUE;

      /* Print the operand as directed by the flags.  */
      if (operand->flags & ARC_OPERAND_IR)
	{
	  const char *rname;

	  assert (value >=0 && value < 64);
	  rname = arcExtMap_coreRegName (value);
	  if (!rname)
	    rname = regnames[value];
	  (*info->fprintf_func) (info->stream, "%s", rname);
	  if (operand->flags & ARC_OPERAND_TRUNCATE)
	    {
	      rname = arcExtMap_coreRegName (value + 1);
	      if (!rname)
		rname = regnames[value + 1];
	      (*info->fprintf_func) (info->stream, "%s", rname);
	    }
	}
      else if (operand->flags & ARC_OPERAND_LIMM)
	{
	  const char *rname = get_auxreg (opcode, value, isa_mask);

	  if (rname && open_braket)
	    (*info->fprintf_func) (info->stream, "%s", rname);
	  else
	    {
	      (*info->fprintf_func) (info->stream, "%#x", value);
	      if (info->insn_type == dis_branch
		  || info->insn_type == dis_jsr)
		info->target = (bfd_vma) value;
	    }
	}
      else if (operand->flags & ARC_OPERAND_PCREL)
	{
	   /* PCL relative.  */
	  if (info->flags & INSN_HAS_RELOC)
	    memaddr = 0;
	  (*info->print_address_func) ((memaddr & ~3) + value, info);

	  info->target = (bfd_vma) (memaddr & ~3) + value;
	}
      else if (operand->flags & ARC_OPERAND_SIGNED)
	{
	  const char *rname = get_auxreg (opcode, value, isa_mask);
	  if (rname && open_braket)
	    (*info->fprintf_func) (info->stream, "%s", rname);
	  else
	    (*info->fprintf_func) (info->stream, "%d", value);
	}
      else if (operand->flags & ARC_OPERAND_ADDRTYPE)
        {
          const char *addrtype = get_addrtype (value);
          (*info->fprintf_func) (info->stream, "%s", addrtype);
          /* A colon follow an address type.  */
          need_comma = FALSE;
        }
      else
	{
	  if (operand->flags & ARC_OPERAND_TRUNCATE
	      && !(operand->flags & ARC_OPERAND_ALIGNED32)
	      && !(operand->flags & ARC_OPERAND_ALIGNED16)
	      && value > 0 && value <= 14)
	    (*info->fprintf_func) (info->stream, "r13-%s",
				   regnames[13 + value - 1]);
	  else
	    {
	      const char *rname = get_auxreg (opcode, value, isa_mask);
	      if (rname && open_braket)
		(*info->fprintf_func) (info->stream, "%s", rname);
	      else
		(*info->fprintf_func) (info->stream, "%#x", value);
	    }
	}
    }

  return insn_len;
}


disassembler_ftype
arc_get_disassembler (bfd *abfd)
{
  /* Read the extenssion insns and registers, if any.  */
  build_ARC_extmap (abfd);
#ifdef DEBUG
  dump_ARC_extmap ();
#endif

  return print_insn_arc;
}

/* Disassemble ARC instructions.  Used by debugger.  */

struct arcDisState
arcAnalyzeInstr (bfd_vma memaddr,
		 struct disassemble_info *info)
{
  struct arcDisState ret;
  memset (&ret, 0, sizeof (struct arcDisState));

  ret.instructionLen = print_insn_arc (memaddr, info);

#if 0
  ret.words[0] = insn[0];
  ret.words[1] = insn[1];
  ret._this = &ret;
  ret.coreRegName = _coreRegName;
  ret.auxRegName = _auxRegName;
  ret.condCodeName = _condCodeName;
  ret.instName = _instName;
#endif

  return ret;
}

void
print_arc_disassembler_options (FILE *stream)
{
  fprintf (stream, _("\n\
The following ARC specific disassembler options are supported for use \n\
with -M switch (multiple options should be separated by commas):\n"));

  fprintf (stream, _("\
  dsp             Recognize DSP instructions.\n"));
  fprintf (stream, _("\
  spfp            Recognize FPX SP instructions.\n"));
  fprintf (stream, _("\
  dpfp            Recognize FPX DP instructions.\n"));
  fprintf (stream, _("\
  quarkse_em      Recognize FPU QuarkSE-EM instructions.\n"));
  fprintf (stream, _("\
  fpuda           Recognize double assist FPU instructions.\n"));
  fprintf (stream, _("\
  fpus            Recognize single precision FPU instructions.\n"));
  fprintf (stream, _("\
  fpud            Recognize double precision FPU instructions.\n"));
}


/* Local variables:
   eval: (c-set-style "gnu")
   indent-tabs-mode: t
   End:  */
