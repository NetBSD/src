/* Instruction printing code for the ARC.
   Copyright (C) 1994-2015 Free Software Foundation, Inc.

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
#include "arc-dis.h"
#include "arc-ext.h"


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
#define FIELDA(word)	(BITS ((word), 21, 26))
#define FIELDB(word)	(BITS ((word), 15, 20))
#define FIELDC(word)	(BITS ((word),  9, 14))

#define OPCODE_AC(word)   (BITS ((word), 11, 15))

/* Functions implementation.  */

static bfd_vma
bfd_getm32 (unsigned int data)
{
  bfd_vma value = 0;

  value = ((data & 0xff00) | (data & 0xff)) << 16;
  value |= ((data & 0xff0000) | (data & 0xff000000)) >> 16;
  return value;
}

static int
special_flag_p (const char *opname,
		const char *flgname)
{
  const struct arc_flag_special *flg_spec;
  size_t len;
  unsigned i, j, flgidx;

  for (i = 0; i < arc_num_flag_special; i++)
    {
      flg_spec = &arc_flag_special_cases[i];
      len = strlen (flg_spec->name);

      if (strncmp (opname, flg_spec->name, len) != 0)
	continue;

      /* Found potential special case instruction.  */
      for (j=0;; ++j)
	{
	  flgidx = flg_spec->flags[j];
	  if (flgidx == 0)
	    break; /* End of the array.  */

	  if (strcmp (flgname, arc_flag_operands[flgidx].name) == 0)
	    return 1;
	}
    }
  return 0;
}

/* Disassemble ARC instructions.  */

static int
print_insn_arc (bfd_vma memaddr,
		struct disassemble_info *info)
{
  bfd_byte buffer[4];
  unsigned int lowbyte, highbyte;
  int status;
  unsigned int i;
  int insnLen = 0;
  unsigned insn[2] = { 0, 0 };
  unsigned isa_mask;
  const unsigned char *opidx;
  const unsigned char *flgidx;
  const struct arc_opcode *opcode;
  const char *instrName;
  int flags;
  bfd_boolean need_comma;
  bfd_boolean open_braket;


  lowbyte  = ((info->endian == BFD_ENDIAN_LITTLE) ? 1 : 0);
  highbyte = ((info->endian == BFD_ENDIAN_LITTLE) ? 0 : 1);

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
      isa_mask = ARC_OPCODE_ARCv2HS | ARC_OPCODE_ARCv2EM;
      break;
    }

  /* Read the insn into a host word.  */
  status = (*info->read_memory_func) (memaddr, buffer, 2, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }

  if (info->section
      && !(info->section->flags & SEC_CODE))
    {
      /* Sort of data section, just print a 32 bit number.  */
      insnLen = 4;
      status = (*info->read_memory_func) (memaddr + 2, &buffer[2], 2, info);
      if (status != 0)
	{
	  (*info->memory_error_func) (status, memaddr + 2, info);
	  return -1;
	}
      insn[0] = ARRANGE_ENDIAN (info, buffer);
      (*info->fprintf_func) (info->stream, ".long %#08x", insn[0]);
      return insnLen;
    }

  if ((((buffer[lowbyte] & 0xf8) > 0x38)
       && ((buffer[lowbyte] & 0xf8) != 0x48))
      || ((info->mach == bfd_mach_arc_arcv2)
	  && ((buffer[lowbyte] & 0xF8) == 0x48)) /* FIXME! ugly.  */
      )
    {
      /* This is a short instruction.  */
      insnLen = 2;
      insn[0] = (buffer[lowbyte] << 8) | buffer[highbyte];
    }
  else
    {
      insnLen = 4;

      /* This is a long instruction: Read the remaning 2 bytes.  */
      status = (*info->read_memory_func) (memaddr + 2, &buffer[2], 2, info);
      if (status != 0)
	{
	  (*info->memory_error_func) (status, memaddr + 2, info);
	  return -1;
	}
      insn[0] = ARRANGE_ENDIAN (info, buffer);
    }

  /* This variable may be set by the instruction decoder.  It suggests
     the number of bytes objdump should display on a single line.  If
     the instruction decoder sets this, it should always set it to
     the same value in order to get reasonable looking output.  */
  info->bytes_per_line  = 8;

  /* The next two variables control the way objdump displays the raw data.
     For example, if bytes_per_line is 8 and bytes_per_chunk is 4, the
     output will look like this:
     00:   00000000 00000000
     with the chunks displayed according to "display_endian".  */
  info->bytes_per_chunk = 2;
  info->display_endian  = info->endian;

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
  for (i = 0; i < arc_num_opcodes; i++)
    {
      bfd_boolean invalid = FALSE;

      opcode = &arc_opcodes[i];

      if (ARC_SHORT (opcode->mask) && (insnLen == 2))
	{
	  if (OPCODE_AC (opcode->opcode) != OPCODE_AC (insn[0]))
	    continue;
	}
      else if (!ARC_SHORT (opcode->mask) && (insnLen == 4))
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
	      if ((value == 0x3E && insnLen == 4)
		  || (value == 0x1E && insnLen == 2))
		{
		  invalid = TRUE;
		  break;
		}
	    }
	}

      /* Check the flags.  */
      for (flgidx = opcode->flags; *flgidx; flgidx++)
	{
	  /* Get a valid flag class.  */
	  const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
	  const unsigned *flgopridx;
	  int foundA = 0, foundB = 0;

	  for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	    {
	      const struct arc_flag_operand *flg_operand = &arc_flag_operands[*flgopridx];
	      unsigned int value;

	      value = (insn[0] >> flg_operand->shift) & ((1 << flg_operand->bits) - 1);
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

      /* The instruction is valid.  */
      goto found;
    }

  /* No instruction found.  Try the extenssions.  */
  instrName = arcExtMap_instName (OPCODE (insn[0]), insn[0], &flags);
  if (instrName)
    {
      opcode = &arc_opcodes[0];
      (*info->fprintf_func) (info->stream, "%s", instrName);
      goto print_flags;
    }

  if (insnLen == 2)
    (*info->fprintf_func) (info->stream, ".long %#04x", insn[0]);
  else
    (*info->fprintf_func) (info->stream, ".long %#08x", insn[0]);

  info->insn_type = dis_noninsn;
  return insnLen;

 found:
  /* Print the mnemonic.  */
  (*info->fprintf_func) (info->stream, "%s", opcode->name);

  /* Preselect the insn class.  */
  switch (opcode->class)
    {
    case BRANCH:
    case JUMP:
      if (!strncmp (opcode->name, "bl", 2)
	  || !strncmp (opcode->name, "jl", 2))
	info->insn_type = dis_jsr;
      else
	info->insn_type = dis_branch;
      break;
    case MEMORY:
      info->insn_type = dis_dref; /* FIXME! DB indicates mov as memory! */
      break;
    default:
      info->insn_type = dis_nonbranch;
      break;
    }

  pr_debug ("%s: 0x%08x\n", opcode->name, opcode->opcode);

 print_flags:
  /* Now extract and print the flags.  */
  for (flgidx = opcode->flags; *flgidx; flgidx++)
    {
      /* Get a valid flag class.  */
      const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
      const unsigned *flgopridx;

      for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	{
	  const struct arc_flag_operand *flg_operand = &arc_flag_operands[*flgopridx];
	  unsigned int value;

	  if (!flg_operand->favail)
	    continue;

	  value = (insn[0] >> flg_operand->shift) & ((1 << flg_operand->bits) - 1);
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
	      (*info->fprintf_func) (info->stream, "%s", flg_operand->name);
	    }

	  if (flg_operand->name[0] == 'd'
	      && flg_operand->name[1] == 0)
	    info->branch_delay_insns = 1;
	}
    }

  if (opcode->operands[0] != 0)
    (*info->fprintf_func) (info->stream, "\t");

  need_comma = FALSE;
  open_braket = FALSE;

  /* Now extract and print the operands.  */
  for (opidx = opcode->operands; *opidx; opidx++)
    {
      const struct arc_operand *operand = &arc_operands[*opidx];
      int value;

      if (open_braket && (operand->flags & ARC_OPERAND_BRAKET))
	{
	  (*info->fprintf_func) (info->stream, "]");
	  open_braket = FALSE;
	  continue;
	}

      /* Only take input from real operands.  */
      if ((operand->flags & ARC_OPERAND_FAKE)
	  && !(operand->flags & ARC_OPERAND_BRAKET))
	continue;

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

      if (operand->flags & ARC_OPERAND_IGNORE
	  && (operand->flags & ARC_OPERAND_IR
	      && value == -1))
	continue;

      if (need_comma)
	(*info->fprintf_func) (info->stream, ",");

      if (!open_braket && (operand->flags & ARC_OPERAND_BRAKET))
	{
	  (*info->fprintf_func) (info->stream, "[");
	  open_braket = TRUE;
	  need_comma = FALSE;
	  continue;
	}

      /* Read the limm operand, if required.  */
      if (operand->flags & ARC_OPERAND_LIMM
	  && !(operand->flags & ARC_OPERAND_DUPLICATE))
	{
	  status = (*info->read_memory_func) (memaddr + insnLen, buffer,
					      4, info);
	  if (status != 0)
	    {
	      (*info->memory_error_func) (status, memaddr + insnLen, info);
	      return -1;
	    }
	  insn[1] = ARRANGE_ENDIAN (info, buffer);
	}

      /* Print the operand as directed by the flags.  */
      if (operand->flags & ARC_OPERAND_IR)
	{
	  assert (value >=0 && value < 64);
	  (*info->fprintf_func) (info->stream, "%s", regnames[value]);
	  if (operand->flags & ARC_OPERAND_TRUNCATE)
	    (*info->fprintf_func) (info->stream, "%s", regnames[value+1]);
	}
      else if (operand->flags & ARC_OPERAND_LIMM)
	{
	  (*info->fprintf_func) (info->stream, "%#x", insn[1]);
	  if (info->insn_type == dis_branch
	      || info->insn_type == dis_jsr)
	    info->target = (bfd_vma) insn[1];
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
	(*info->fprintf_func) (info->stream, "%d", value);
      else
	if (operand->flags & ARC_OPERAND_TRUNCATE
	    && !(operand->flags & ARC_OPERAND_ALIGNED32)
	    && !(operand->flags & ARC_OPERAND_ALIGNED16)
	    && value > 0 && value <= 14)
	  (*info->fprintf_func) (info->stream, "r13-%s",
				 regnames[13 + value - 1]);
	else
	  (*info->fprintf_func) (info->stream, "%#x", value);

      need_comma = TRUE;

      /* Adjust insn len.  */
      if (operand->flags & ARC_OPERAND_LIMM
	  && !(operand->flags & ARC_OPERAND_DUPLICATE))
	insnLen += 4;
    }

  return insnLen;
}


disassembler_ftype
arc_get_disassembler (bfd *abfd)
{
  /* Read the extenssion insns and registers, if any.  */
  build_ARC_extmap (abfd);
  dump_ARC_extmap ();

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

/* Local variables:
   eval: (c-set-style "gnu")
   indent-tabs-mode: t
   End:  */
