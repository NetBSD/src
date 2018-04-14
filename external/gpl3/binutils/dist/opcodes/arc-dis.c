/* Instruction printing code for the ARC.
   Copyright (C) 1994-2018 Free Software Foundation, Inc.

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
  /* The complete instruction value to extract operands from.  */
  unsigned long long insn;

  /* The LIMM if this is being tracked separately.  This field is only
     valid if we find the LIMM operand in the operand list.  */
  unsigned limm;

  /* The opcode this iterator is operating on.  */
  const struct arc_opcode *opcode;

  /* The index into the opcodes operand index list.  */
  const unsigned char *opidx;
};

/* A private data used by ARC decoder.  */
struct arc_disassemble_info
{
  /* The current disassembled arc opcode.  */
  const struct arc_opcode *opcode;

  /* Instruction length w/o limm field.  */
  unsigned insn_len;

  /* TRUE if we have limm.  */
  bfd_boolean limm_p;

  /* LIMM value, if exists.  */
  unsigned limm;

  /* Condition code, if exists.  */
  unsigned condition_code;

  /* Writeback mode.  */
  unsigned writeback_mode;

  /* Number of operands.  */
  unsigned operands_count;

  struct arc_insn_operand operands[MAX_INSN_ARGS];
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

/* ISA mask value enforced via disassembler info options.  ARC_OPCODE_NONE
   value means that no CPU is enforced.  */

static unsigned enforced_isa_mask = ARC_OPCODE_NONE;

/* True if we want to print using only hex numbers.  */
static bfd_boolean print_hex = FALSE;

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
#define OPCODE_32BIT_INSN(word)	(BITS ((word), 27, 31))

/* Functions implementation.  */

/* Initialize private data.  */
static bfd_boolean
init_arc_disasm_info (struct disassemble_info *info)
{
  struct arc_disassemble_info *arc_infop
    = calloc (sizeof (*arc_infop), 1);

  if (arc_infop == NULL)
    return FALSE;

  info->private_data = arc_infop;
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
skip_this_opcode (const struct arc_opcode *opcode)
{
  linkclass t = decodelist;

  /* Check opcode for major 0x06, return if it is not in.  */
  if (arc_opcode_len (opcode) == 4
      && OPCODE_32BIT_INSN (opcode->opcode) != 0x06)
    return FALSE;

  /* or not a known truble class.  */
  switch (opcode->insn_class)
    {
    case FLOAT:
    case DSP:
    case ARITH:
      break;
    default:
      return FALSE;
    }

  while (t != NULL)
    {
      if ((t->insn_class == opcode->insn_class)
	  && (t->subclass == opcode->subclass))
	return FALSE;
      t = t->nxt;
    }

  return TRUE;
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
                        unsigned long long insn,
			unsigned int insn_len,
                        unsigned isa_mask,
			bfd_boolean *has_limm,
			bfd_boolean overlaps)
{
  unsigned int i = 0;
  const struct arc_opcode *opcode = NULL;
  const struct arc_opcode *t_op = NULL;
  const unsigned char *opidx;
  const unsigned char *flgidx;
  bfd_boolean warn_p = FALSE;

  do
    {
      bfd_boolean invalid = FALSE;

      opcode = &arc_table[i++];

      if (!(opcode->cpu & isa_mask))
	continue;

      if (arc_opcode_len (opcode) != (int) insn_len)
	continue;

      if ((insn & opcode->mask) != opcode->opcode)
	continue;

      *has_limm = FALSE;

      /* Possible candidate, check the operands.  */
      for (opidx = opcode->operands; *opidx; opidx++)
	{
	  int value, limmind;
	  const struct arc_operand *operand = &arc_operands[*opidx];

	  if (operand->flags & ARC_OPERAND_FAKE)
	    continue;

	  if (operand->extract)
	    value = (*operand->extract) (insn, &invalid);
	  else
	    value = (insn >> operand->shift) & ((1 << operand->bits) - 1);

	  /* Check for LIMM indicator.  If it is there, then make sure
	     we pick the right format.  */
	  limmind = (isa_mask & ARC_OPCODE_ARCV2) ? 0x1E : 0x3E;
	  if (operand->flags & ARC_OPERAND_IR
	      && !(operand->flags & ARC_OPERAND_LIMM))
	    {
	      if ((value == 0x3E && insn_len == 4)
		  || (value == limmind && insn_len == 2))
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
	      value = (insn & 0x1F);
	      if (arcExtMap_condCodeName (value))
		continue;
	    }

	  /* Check for the implicit flags.  */
	  if (cl_flags->flag_class & F_CLASS_IMPLICIT)
	    continue;

	  for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	    {
	      const struct arc_flag_operand *flg_operand =
		&arc_flag_operands[*flgopridx];

	      value = (insn >> flg_operand->shift)
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
	  && overlaps)
	{
	  warn_p = TRUE;
	  t_op = opcode;
	  if (skip_this_opcode (opcode))
	    continue;
	}

      /* The instruction is valid.  */
      return opcode;
    }
  while (opcode->mask);

  if (warn_p)
    {
      info->fprintf_func (info->stream,
			  _("\nWarning: disassembly may be wrong due to "
			    "guessed opcode class choice.\n"
			    "Use -M<class[,class]> to select the correct "
			    "opcode class(es).\n\t\t\t\t"));
      return t_op;
    }

  return NULL;
}

/* Find opcode for INSN, trying various different sources.  The instruction
   length in INSN_LEN will be updated if the instruction requires a LIMM
   extension.

   A pointer to the opcode is placed into OPCODE_RESULT, and ITER is
   initialised, ready to iterate over the operands of the found opcode.  If
   the found opcode requires a LIMM then the LIMM value will be loaded into a
   field of ITER.

   This function returns TRUE in almost all cases, FALSE is reserved to
   indicate an error (failing to find an opcode is not an error) a returned
   result of FALSE would indicate that the disassembler can't continue.

   If no matching opcode is found then the returned result will be TRUE, the
   value placed into OPCODE_RESULT will be NULL, ITER will be undefined, and
   INSN_LEN will be unchanged.

   If a matching opcode is found, then the returned result will be TRUE, the
   opcode pointer is placed into OPCODE_RESULT, INSN_LEN will be increased by
   4 if the instruction requires a LIMM, and the LIMM value will have been
   loaded into a field of ITER.  Finally, ITER will have been initialised so
   that calls to OPERAND_ITERATOR_NEXT will iterate over the opcode's
   operands.  */

static bfd_boolean
find_format (bfd_vma                       memaddr,
	     unsigned long long            insn,
	     unsigned int *                insn_len,
             unsigned                      isa_mask,
	     struct disassemble_info *     info,
             const struct arc_opcode **    opcode_result,
             struct arc_operand_iterator * iter)
{
  const struct arc_opcode *opcode = NULL;
  bfd_boolean needs_limm;
  const extInstruction_t *einsn, *i;
  unsigned limm = 0;
  struct arc_disassemble_info *arc_infop = info->private_data;

  /* First, try the extension instructions.  */
  if (*insn_len == 4)
    {
      einsn = arcExtMap_insn (OPCODE_32BIT_INSN (insn), insn);
      for (i = einsn; (i != NULL) && (opcode == NULL); i = i->next)
	{
	  const char *errmsg = NULL;

	  opcode = arcExtMap_genOpcode (i, isa_mask, &errmsg);
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
          limm = ARRANGE_ENDIAN (info, buffer);
          *insn_len += 4;
        }
    }

  if (opcode != NULL)
    {
      iter->insn = insn;
      iter->limm = limm;
      iter->opcode = opcode;
      iter->opidx = opcode->operands;
    }

  *opcode_result = opcode;

  /* Update private data.  */
  arc_infop->opcode = opcode;
  arc_infop->limm = (needs_limm) ? limm : 0;
  arc_infop->limm_p = needs_limm;

  return TRUE;
}

static void
print_flags (const struct arc_opcode *opcode,
	     unsigned long long *insn,
	     struct disassemble_info *info)
{
  const unsigned char *flgidx;
  unsigned int value;
  struct arc_disassemble_info *arc_infop = info->private_data;

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

	  /* Implicit flags are only used for the insn decoder.  */
	  if (cl_flags->flag_class & F_CLASS_IMPLICIT)
	    {
	      if (cl_flags->flag_class & F_CLASS_COND)
		arc_infop->condition_code = flg_operand->code;
	      else if (cl_flags->flag_class & F_CLASS_WB)
		arc_infop->writeback_mode = flg_operand->code;
	      else if (cl_flags->flag_class & F_CLASS_ZZ)
		info->data_size = flg_operand->code;
	      continue;
	    }

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
		  arc_infop->condition_code = flg_operand->code;
		}

	      /* Check for the write back modes.  */
	      if (cl_flags->flag_class & F_CLASS_WB)
		arc_infop->writeback_mode = flg_operand->code;

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
	    return 6;
	  else if (minor_opcode == 0x10 || minor_opcode == 0x11)
	    return 8;
        }
      if (major_opcode == 0xa)
        {
          return 8;
        }
      /* Fall through.  */
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
extract_operand_value (const struct arc_operand *operand,
		       unsigned long long insn,
		       unsigned limm)
{
  int value;

  /* Read the limm operand, if required.  */
  if (operand->flags & ARC_OPERAND_LIMM)
    /* The second part of the instruction value will have been loaded as
       part of the find_format call made earlier.  */
    value = limm;
  else
    {
      if (operand->extract)
        value = (*operand->extract) (insn, (int *) NULL);
      else
        {
          if (operand->flags & ARC_OPERAND_ALIGNED32)
            {
              value = (insn >> operand->shift)
                & ((1 << (operand->bits - 2)) - 1);
              value = value << 2;
            }
          else
            {
              value = (insn >> operand->shift) & ((1 << operand->bits) - 1);
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
  if (*iter->opidx == 0)
    {
      *operand = NULL;
      return FALSE;
    }

  *operand = &arc_operands[*iter->opidx];
  *value = extract_operand_value (*operand, iter->insn, iter->limm);
  iter->opidx++;

  return TRUE;
}

/* Helper for parsing the options.  */

static void
parse_option (const char *option)
{
  if (disassembler_options_cmp (option, "dsp") == 0)
    add_to_decodelist (DSP, NONE);

  else if (disassembler_options_cmp (option, "spfp") == 0)
    add_to_decodelist (FLOAT, SPX);

  else if (disassembler_options_cmp (option, "dpfp") == 0)
    add_to_decodelist (FLOAT, DPX);

  else if (disassembler_options_cmp (option, "quarkse_em") == 0)
    {
      add_to_decodelist (FLOAT, DPX);
      add_to_decodelist (FLOAT, SPX);
      add_to_decodelist (FLOAT, QUARKSE1);
      add_to_decodelist (FLOAT, QUARKSE2);
    }

  else if (disassembler_options_cmp (option, "fpuda") == 0)
    add_to_decodelist (FLOAT, DPA);

  else if (disassembler_options_cmp (option, "fpus") == 0)
    {
      add_to_decodelist (FLOAT, SP);
      add_to_decodelist (FLOAT, CVT);
    }

  else if (disassembler_options_cmp (option, "fpud") == 0)
    {
      add_to_decodelist (FLOAT, DP);
      add_to_decodelist (FLOAT, CVT);
    }
  else if (CONST_STRNEQ (option, "hex"))
    print_hex = TRUE;
  else
    fprintf (stderr, _("Unrecognised disassembler option: %s\n"), option);
}

#define ARC_CPU_TYPE_A6xx(NAME,EXTRA)			\
  { #NAME, ARC_OPCODE_ARC600, "ARC600" }
#define ARC_CPU_TYPE_A7xx(NAME,EXTRA)			\
  { #NAME, ARC_OPCODE_ARC700, "ARC700" }
#define ARC_CPU_TYPE_AV2EM(NAME,EXTRA)			\
  { #NAME,  ARC_OPCODE_ARCv2EM, "ARC EM" }
#define ARC_CPU_TYPE_AV2HS(NAME,EXTRA)			\
  { #NAME,  ARC_OPCODE_ARCv2HS, "ARC HS" }
#define ARC_CPU_TYPE_NONE				\
  { 0, 0, 0 }

/* A table of CPU names and opcode sets.  */
static const struct cpu_type
{
  const char *name;
  unsigned flags;
  const char *isa;
}
  cpu_types[] =
{
  #include "elf/arc-cpu.def"
};

/* Helper for parsing the CPU options.  Accept any of the ARC architectures
   values.  OPTION should be a value passed to cpu=.  */

static unsigned
parse_cpu_option (const char *option)
{
  int i;

  for (i = 0; cpu_types[i].name; ++i)
    {
      if (!disassembler_options_cmp (cpu_types[i].name, option))
	{
	  return cpu_types[i].flags;
	}
    }

  fprintf (stderr, _("Unrecognised disassembler CPU option: %s\n"), option);
  return ARC_OPCODE_NONE;
}

/* Go over the options list and parse it.  */

static void
parse_disassembler_options (const char *options)
{
  const char *option;

  if (options == NULL)
    return;

  /* Disassembler might be reused for difference CPU's, and cpu option set for
     the first one shouldn't be applied to second (which might not have
     explicit cpu in its options.  Therefore it is required to reset enforced
     CPU when new options are being parsed.  */
  enforced_isa_mask = ARC_OPCODE_NONE;

  FOR_EACH_DISASSEMBLER_OPTION (option, options)
    {
      /* A CPU option?  Cannot use STRING_COMMA_LEN because strncmp is also a
	 preprocessor macro.  */
      if (strncmp (option, "cpu=", 4) == 0)
	/* Strip leading `cpu=`.  */
	enforced_isa_mask = parse_cpu_option (option + 4);
      else
	parse_option (option);
    }
}

/* Return the instruction type for an instruction described by OPCODE.  */

static enum dis_insn_type
arc_opcode_to_insn_type (const struct arc_opcode *opcode)
{
  enum dis_insn_type insn_type;

  switch (opcode->insn_class)
    {
    case BRANCH:
    case BBIT0:
    case BBIT1:
    case BI:
    case BIH:
    case BRCC:
    case EI:
    case JLI:
    case JUMP:
    case LOOP:
      if (!strncmp (opcode->name, "bl", 2)
	  || !strncmp (opcode->name, "jl", 2))
	{
	  if (opcode->subclass == COND)
	    insn_type = dis_condjsr;
	  else
	    insn_type = dis_jsr;
	}
      else
	{
	  if (opcode->subclass == COND)
	    insn_type = dis_condbranch;
	  else
	    insn_type = dis_branch;
	}
      break;
    case LOAD:
    case STORE:
    case MEMORY:
    case ENTER:
    case PUSH:
    case POP:
      insn_type = dis_dref;
      break;
    case LEAVE:
      insn_type = dis_branch;
      break;
    default:
      insn_type = dis_nonbranch;
      break;
    }

  return insn_type;
}

/* Disassemble ARC instructions.  */

static int
print_insn_arc (bfd_vma memaddr,
		struct disassemble_info *info)
{
  bfd_byte buffer[8];
  unsigned int highbyte, lowbyte;
  int status;
  unsigned int insn_len;
  unsigned long long insn = 0;
  unsigned isa_mask = ARC_OPCODE_NONE;
  const struct arc_opcode *opcode;
  bfd_boolean need_comma;
  bfd_boolean open_braket;
  int size;
  const struct arc_operand *operand;
  int value, vpcl;
  struct arc_operand_iterator iter;
  struct arc_disassemble_info *arc_infop;
  bfd_boolean rpcl = FALSE, rset = FALSE;

  if (info->disassembler_options)
    {
      parse_disassembler_options (info->disassembler_options);

      /* Avoid repeated parsing of the options.  */
      info->disassembler_options = NULL;
    }

  if (info->private_data == NULL && !init_arc_disasm_info (info))
    return -1;

  memset (&iter, 0, sizeof (iter));
  highbyte  = ((info->endian == BFD_ENDIAN_LITTLE) ? 1 : 0);
  lowbyte = ((info->endian == BFD_ENDIAN_LITTLE) ? 0 : 1);

  /* Figure out CPU type, unless it was enforced via disassembler options.  */
  if (enforced_isa_mask == ARC_OPCODE_NONE)
    {
      Elf_Internal_Ehdr *header = NULL;

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
	  /* TODO: Perhaps remove definition of header since it is only used at
	     this location.  */
	  if (header != NULL
	      && (header->e_flags & EF_ARC_MACH_MSK) == EF_ARC_CPU_ARCV2HS)
	    isa_mask = ARC_OPCODE_ARCv2HS;
	  break;
	}
    }
  else
    isa_mask = enforced_isa_mask;

  if (isa_mask == ARC_OPCODE_ARCv2HS)
    {
      /* FPU instructions are not extensions for HS.  */
      add_to_decodelist (FLOAT, SP);
      add_to_decodelist (FLOAT, DP);
      add_to_decodelist (FLOAT, CVT);
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

  insn_len = arc_insn_length (buffer[highbyte], buffer[lowbyte], info);
  pr_debug ("instruction length = %d bytes\n", insn_len);
  arc_infop = info->private_data;
  arc_infop->insn_len = insn_len;

  switch (insn_len)
    {
    case 2:
      insn = (buffer[highbyte] << 8) | buffer[lowbyte];
      break;

    case 4:
      {
	/* This is a long instruction: Read the remaning 2 bytes.  */
	status = (*info->read_memory_func) (memaddr + 2, &buffer[2], 2, info);
	if (status != 0)
	  {
	    (*info->memory_error_func) (status, memaddr + 2, info);
	    return -1;
	  }
	insn = (unsigned long long) ARRANGE_ENDIAN (info, buffer);
      }
      break;

    case 6:
      {
	status = (*info->read_memory_func) (memaddr + 2, &buffer[2], 4, info);
	if (status != 0)
	  {
	    (*info->memory_error_func) (status, memaddr + 2, info);
	    return -1;
	  }
	insn = (unsigned long long) ARRANGE_ENDIAN (info, &buffer[2]);
	insn |= ((unsigned long long) buffer[highbyte] << 40)
	  | ((unsigned long long) buffer[lowbyte] << 32);
      }
      break;

    case 8:
      {
	status = (*info->read_memory_func) (memaddr + 2, &buffer[2], 6, info);
	if (status != 0)
	  {
	    (*info->memory_error_func) (status, memaddr + 2, info);
	    return -1;
	  }
	insn =
	  ((((unsigned long long) ARRANGE_ENDIAN (info, buffer)) << 32)
	   | ((unsigned long long) ARRANGE_ENDIAN (info, &buffer[4])));
      }
      break;

    default:
      /* There is no instruction whose length is not 2, 4, 6, or 8.  */
      abort ();
    }

  pr_debug ("instruction value = %llx\n", insn);

  /* Set some defaults for the insn info.  */
  info->insn_info_valid    = 1;
  info->branch_delay_insns = 0;
  info->data_size	   = 4;
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
      switch (insn_len)
	{
	case 2:
	  (*info->fprintf_func) (info->stream, ".shor\t%#04llx",
				 insn & 0xffff);
	  break;
	case 4:
	  (*info->fprintf_func) (info->stream, ".word\t%#08llx",
				 insn & 0xffffffff);
	  break;
	case 6:
	  (*info->fprintf_func) (info->stream, ".long\t%#08llx",
				 insn & 0xffffffff);
	  (*info->fprintf_func) (info->stream, ".long\t%#04llx",
				 (insn >> 32) & 0xffff);
	  break;
	case 8:
	  (*info->fprintf_func) (info->stream, ".long\t%#08llx",
				 insn & 0xffffffff);
	  (*info->fprintf_func) (info->stream, ".long\t%#08llx",
				 insn >> 32);
	  break;
	default:
	  abort ();
	}

      info->insn_type = dis_noninsn;
      return insn_len;
    }

  /* Print the mnemonic.  */
  (*info->fprintf_func) (info->stream, "%s", opcode->name);

  /* Preselect the insn class.  */
  info->insn_type = arc_opcode_to_insn_type (opcode);

  pr_debug ("%s: 0x%08llx\n", opcode->name, opcode->opcode);

  print_flags (opcode, &insn, info);

  if (opcode->operands[0] != 0)
    (*info->fprintf_func) (info->stream, "\t");

  need_comma = FALSE;
  open_braket = FALSE;
  arc_infop->operands_count = 0;

  /* Now extract and print the operands.  */
  operand = NULL;
  vpcl = 0;
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

      if (operand->flags & ARC_OPERAND_PCREL)
	{
	  rpcl = TRUE;
	  vpcl = value;
	  rset = TRUE;

	  info->target = (bfd_vma) (memaddr & ~3) + value;
	}
      else if (!(operand->flags & ARC_OPERAND_IR))
	{
	  vpcl = value;
	  rset = TRUE;
	}

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
	  if (value == 63)
	    rpcl = TRUE;
	  else
	    rpcl = FALSE;
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
      else if (operand->flags & ARC_OPERAND_SIGNED)
	{
	  const char *rname = get_auxreg (opcode, value, isa_mask);
	  if (rname && open_braket)
	    (*info->fprintf_func) (info->stream, "%s", rname);
	  else
	    {
	      if (print_hex)
		(*info->fprintf_func) (info->stream, "%#x", value);
	      else
		(*info->fprintf_func) (info->stream, "%d", value);
	    }
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
	      && value >= 0 && value <= 14)
	    {
	      /* Leave/Enter mnemonics.  */
	      switch (value)
		{
		case 0:
		  need_comma = FALSE;
		  break;
		case 1:
		  (*info->fprintf_func) (info->stream, "r13");
		  break;
		default:
		  (*info->fprintf_func) (info->stream, "r13-%s",
					 regnames[13 + value - 1]);
		  break;
		}
	      rpcl = FALSE;
	      rset = FALSE;
	    }
	  else
	    {
	      const char *rname = get_auxreg (opcode, value, isa_mask);
	      if (rname && open_braket)
		(*info->fprintf_func) (info->stream, "%s", rname);
	      else
		(*info->fprintf_func) (info->stream, "%#x", value);
	    }
	}

      if (operand->flags & ARC_OPERAND_LIMM)
	{
	  arc_infop->operands[arc_infop->operands_count].kind
	    = ARC_OPERAND_KIND_LIMM;
	  /* It is not important to have exactly the LIMM indicator
	     here.  */
	  arc_infop->operands[arc_infop->operands_count].value = 63;
	}
      else
	{
	  arc_infop->operands[arc_infop->operands_count].value = value;
	  arc_infop->operands[arc_infop->operands_count].kind
	    = (operand->flags & ARC_OPERAND_IR
	       ? ARC_OPERAND_KIND_REG
	       : ARC_OPERAND_KIND_SHIMM);
	}
      arc_infop->operands_count ++;
    }

  /* Pretty print extra info for pc-relative operands.  */
  if (rpcl && rset)
    {
      if (info->flags & INSN_HAS_RELOC)
	/* If the instruction has a reloc associated with it, then the
	   offset field in the instruction will actually be the addend
	   for the reloc.  (We are using REL type relocs).  In such
	   cases, we can ignore the pc when computing addresses, since
	   the addend is not currently pc-relative.  */
	memaddr = 0;

      (*info->fprintf_func) (info->stream, "\t;");
      (*info->print_address_func) ((memaddr & ~3) + vpcl, info);
    }

  return insn_len;
}


disassembler_ftype
arc_get_disassembler (bfd *abfd)
{
  /* BFD my be absent, if opcodes is invoked from the debugger that
     has connected to remote target and doesn't have an ELF file.  */
  if (abfd != NULL)
    {
      /* Read the extension insns and registers, if any.  */
      build_ARC_extmap (abfd);
#ifdef DEBUG
      dump_ARC_extmap ();
#endif
    }

  return print_insn_arc;
}

void
print_arc_disassembler_options (FILE *stream)
{
  int i;

  fprintf (stream, _("\n\
The following ARC specific disassembler options are supported for use \n\
with -M switch (multiple options should be separated by commas):\n"));

  /* cpu=... options.  */
  for (i = 0; cpu_types[i].name; ++i)
    {
      /* As of now all value CPU values are less than 16 characters.  */
      fprintf (stream, "  cpu=%-16s\tEnforce %s ISA.\n",
	       cpu_types[i].name, cpu_types[i].isa);
    }

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
  fprintf (stream, _("\
  hex             Use only hexadecimal number to print immediates.\n"));
}

void arc_insn_decode (bfd_vma addr,
		      struct disassemble_info *info,
		      disassembler_ftype disasm_func,
		      struct arc_instruction *insn)
{
  const struct arc_opcode *opcode;
  struct arc_disassemble_info *arc_infop;

  /* Ensure that insn would be in the reset state.  */
  memset (insn, 0, sizeof (struct arc_instruction));

  /* There was an error when disassembling, for example memory read error.  */
  if (disasm_func (addr, info) < 0)
    {
      insn->valid = FALSE;
      return;
    }

  assert (info->private_data != NULL);
  arc_infop = info->private_data;

  insn->length  = arc_infop->insn_len;;
  insn->address = addr;

  /* Quick exit if memory at this address is not an instruction.  */
  if (info->insn_type == dis_noninsn)
    {
      insn->valid = FALSE;
      return;
    }

  insn->valid = TRUE;

  opcode = (const struct arc_opcode *) arc_infop->opcode;
  insn->insn_class = opcode->insn_class;
  insn->limm_value = arc_infop->limm;
  insn->limm_p     = arc_infop->limm_p;

  insn->is_control_flow = (info->insn_type == dis_branch
			   || info->insn_type == dis_condbranch
			   || info->insn_type == dis_jsr
			   || info->insn_type == dis_condjsr);

  insn->has_delay_slot = info->branch_delay_insns;
  insn->writeback_mode
    = (enum arc_ldst_writeback_mode) arc_infop->writeback_mode;
  insn->data_size_mode = info->data_size;
  insn->condition_code = arc_infop->condition_code;
  memcpy (insn->operands, arc_infop->operands,
	  sizeof (struct arc_insn_operand) * MAX_INSN_ARGS);
  insn->operands_count = arc_infop->operands_count;
}

/* Local variables:
   eval: (c-set-style "gnu")
   indent-tabs-mode: t
   End:  */
