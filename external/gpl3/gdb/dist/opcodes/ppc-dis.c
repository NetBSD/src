/* ppc-dis.c -- Disassemble PowerPC instructions
   Copyright 1994, 1995, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
   2008, 2009, 2010 Free Software Foundation, Inc.
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

#include <stdio.h>
#include "sysdep.h"
#include "dis-asm.h"
#include "opintl.h"
#include "opcode/ppc.h"

/* This file provides several disassembler functions, all of which use
   the disassembler interface defined in dis-asm.h.  Several functions
   are provided because this file handles disassembly for the PowerPC
   in both big and little endian mode and also for the POWER (RS/6000)
   chip.  */
static int print_insn_powerpc (bfd_vma, struct disassemble_info *, int,
			       ppc_cpu_t);

struct dis_private
{
  /* Stash the result of parsing disassembler_options here.  */
  ppc_cpu_t dialect;
};

#define POWERPC_DIALECT(INFO) \
  (((struct dis_private *) ((INFO)->private_data))->dialect)

struct ppc_mopt {
  const char *opt;
  ppc_cpu_t cpu;
  ppc_cpu_t sticky;
};

struct ppc_mopt ppc_opts[] = {
  { "403",     (PPC_OPCODE_PPC | PPC_OPCODE_403),
    0 },
  { "405",     (PPC_OPCODE_PPC | PPC_OPCODE_403 | PPC_OPCODE_405),
    0 },
  { "440",     (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE | PPC_OPCODE_440
		| PPC_OPCODE_ISEL | PPC_OPCODE_RFMCI),
    0 },
  { "464",     (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE | PPC_OPCODE_440
		| PPC_OPCODE_ISEL | PPC_OPCODE_RFMCI),
    0 },
  { "476",     (PPC_OPCODE_PPC | PPC_OPCODE_ISEL | PPC_OPCODE_440
		| PPC_OPCODE_476 | PPC_OPCODE_POWER4 | PPC_OPCODE_POWER5),
    0 },
  { "601",     (PPC_OPCODE_PPC | PPC_OPCODE_601),
    0 },
  { "603",     (PPC_OPCODE_PPC),
    0 },
  { "604",     (PPC_OPCODE_PPC),
    0 },
  { "620",     (PPC_OPCODE_PPC | PPC_OPCODE_64),
    0 },
  { "7400",    (PPC_OPCODE_PPC | PPC_OPCODE_ALTIVEC),
    0 },
  { "7410",    (PPC_OPCODE_PPC | PPC_OPCODE_ALTIVEC),
    0 },
  { "7450",    (PPC_OPCODE_PPC | PPC_OPCODE_ALTIVEC),
    0 },
  { "7455",    (PPC_OPCODE_PPC | PPC_OPCODE_ALTIVEC),
    0 },
  { "750cl",   (PPC_OPCODE_PPC | PPC_OPCODE_PPCPS)
    , 0 },
  { "a2",      (PPC_OPCODE_PPC | PPC_OPCODE_ISEL | PPC_OPCODE_POWER4
		| PPC_OPCODE_POWER5 | PPC_OPCODE_CACHELCK | PPC_OPCODE_64
		| PPC_OPCODE_A2),
    0 },
  { "altivec", (PPC_OPCODE_PPC),
    PPC_OPCODE_ALTIVEC },
  { "any",     0,
    PPC_OPCODE_ANY },
  { "booke",   (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE),
    0 },
  { "booke32", (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE),
    0 },
  { "cell",    (PPC_OPCODE_PPC | PPC_OPCODE_64 | PPC_OPCODE_POWER4
		| PPC_OPCODE_CELL | PPC_OPCODE_ALTIVEC),
    0 },
  { "com",     (PPC_OPCODE_COMMON),
    0 },
  { "e300",    (PPC_OPCODE_PPC | PPC_OPCODE_E300),
    0 },
  { "e500",    (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE | PPC_OPCODE_SPE
		| PPC_OPCODE_ISEL | PPC_OPCODE_EFS | PPC_OPCODE_BRLOCK
		| PPC_OPCODE_PMR | PPC_OPCODE_CACHELCK | PPC_OPCODE_RFMCI
		| PPC_OPCODE_E500),
    0 },
  { "e500mc",  (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE | PPC_OPCODE_ISEL
		| PPC_OPCODE_PMR | PPC_OPCODE_CACHELCK | PPC_OPCODE_RFMCI
		| PPC_OPCODE_E500MC),
    0 },
  { "e500mc64",  (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE | PPC_OPCODE_ISEL
		| PPC_OPCODE_PMR | PPC_OPCODE_CACHELCK | PPC_OPCODE_RFMCI
		| PPC_OPCODE_E500MC | PPC_OPCODE_64 | PPC_OPCODE_POWER5
		| PPC_OPCODE_POWER6 | PPC_OPCODE_POWER7),
    0 },
  { "e500x2",  (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE | PPC_OPCODE_SPE
		| PPC_OPCODE_ISEL | PPC_OPCODE_EFS | PPC_OPCODE_BRLOCK
		| PPC_OPCODE_PMR | PPC_OPCODE_CACHELCK | PPC_OPCODE_RFMCI
		| PPC_OPCODE_E500),
    0 },
  { "efs",     (PPC_OPCODE_PPC | PPC_OPCODE_EFS),
    0 },
  { "power4",  (PPC_OPCODE_PPC | PPC_OPCODE_64 | PPC_OPCODE_POWER4),
    0 },
  { "power5",  (PPC_OPCODE_PPC | PPC_OPCODE_64 | PPC_OPCODE_POWER4
		| PPC_OPCODE_POWER5),
    0 },
  { "power6",  (PPC_OPCODE_PPC | PPC_OPCODE_64 | PPC_OPCODE_POWER4
		| PPC_OPCODE_POWER5 | PPC_OPCODE_POWER6 | PPC_OPCODE_ALTIVEC),
    0 },
  { "power7",  (PPC_OPCODE_PPC | PPC_OPCODE_ISEL | PPC_OPCODE_64
		| PPC_OPCODE_POWER4 | PPC_OPCODE_POWER5 | PPC_OPCODE_POWER6
		| PPC_OPCODE_POWER7 | PPC_OPCODE_ALTIVEC | PPC_OPCODE_VSX),
    0 },
  { "ppc",     (PPC_OPCODE_PPC),
    0 },
  { "ppc32",   (PPC_OPCODE_PPC),
    0 },
  { "ppc64",   (PPC_OPCODE_PPC | PPC_OPCODE_64),
    0 },
  { "ppc64bridge", (PPC_OPCODE_PPC | PPC_OPCODE_64_BRIDGE),
    0 },
  { "ppcps",   (PPC_OPCODE_PPC | PPC_OPCODE_PPCPS),
    0 },
  { "pwr",     (PPC_OPCODE_POWER),
    0 },
  { "pwr2",    (PPC_OPCODE_POWER | PPC_OPCODE_POWER2),
    0 },
  { "pwr4",    (PPC_OPCODE_PPC | PPC_OPCODE_64 | PPC_OPCODE_POWER4),
    0 },
  { "pwr5",    (PPC_OPCODE_PPC | PPC_OPCODE_64 | PPC_OPCODE_POWER4
		| PPC_OPCODE_POWER5),
    0 },
  { "pwr5x",   (PPC_OPCODE_PPC | PPC_OPCODE_64 | PPC_OPCODE_POWER4
		| PPC_OPCODE_POWER5),
    0 },
  { "pwr6",    (PPC_OPCODE_PPC | PPC_OPCODE_64 | PPC_OPCODE_POWER4
		| PPC_OPCODE_POWER5 | PPC_OPCODE_POWER6 | PPC_OPCODE_ALTIVEC),
    0 },
  { "pwr7",    (PPC_OPCODE_PPC | PPC_OPCODE_ISEL | PPC_OPCODE_64
		| PPC_OPCODE_POWER4 | PPC_OPCODE_POWER5 | PPC_OPCODE_POWER6
		| PPC_OPCODE_POWER7 | PPC_OPCODE_ALTIVEC | PPC_OPCODE_VSX),
    0 },
  { "pwrx",    (PPC_OPCODE_POWER | PPC_OPCODE_POWER2),
    0 },
  { "spe",     (PPC_OPCODE_PPC | PPC_OPCODE_EFS),
    PPC_OPCODE_SPE },
  { "titan",   (PPC_OPCODE_PPC | PPC_OPCODE_BOOKE | PPC_OPCODE_PMR
		| PPC_OPCODE_RFMCI | PPC_OPCODE_TITAN),
    0 },
  { "vsx",     (PPC_OPCODE_PPC),
    PPC_OPCODE_VSX },
};

/* Handle -m and -M options that set cpu type, and .machine arg.  */

ppc_cpu_t
ppc_parse_cpu (ppc_cpu_t ppc_cpu, const char *arg)
{
  /* Sticky bits.  */
  ppc_cpu_t retain_flags = ppc_cpu & (PPC_OPCODE_ALTIVEC | PPC_OPCODE_VSX
				      | PPC_OPCODE_SPE | PPC_OPCODE_ANY);
  unsigned int i;

  for (i = 0; i < sizeof (ppc_opts) / sizeof (ppc_opts[0]); i++)
    if (strcmp (ppc_opts[i].opt, arg) == 0)
      {
	if (ppc_opts[i].sticky)
	  {
	    retain_flags |= ppc_opts[i].sticky;
	    if ((ppc_cpu & ~(ppc_cpu_t) (PPC_OPCODE_ALTIVEC | PPC_OPCODE_VSX
					 | PPC_OPCODE_SPE | PPC_OPCODE_ANY)) != 0)
	      break;
	  }
	ppc_cpu = ppc_opts[i].cpu;
	break;
      }
  if (i >= sizeof (ppc_opts) / sizeof (ppc_opts[0]))
    return 0;

  ppc_cpu |= retain_flags;
  return ppc_cpu;
}

/* Determine which set of machines to disassemble for.  */

static int
powerpc_init_dialect (struct disassemble_info *info)
{
  ppc_cpu_t dialect = 0;
  char *arg;
  struct dis_private *priv = calloc (sizeof (*priv), 1);

  if (priv == NULL)
    return FALSE;

  arg = info->disassembler_options;
  while (arg != NULL)
    {
      ppc_cpu_t new_cpu = 0;
      char *end = strchr (arg, ',');

      if (end != NULL)
	*end = 0;

      if ((new_cpu = ppc_parse_cpu (dialect, arg)) != 0)
	dialect = new_cpu;
      else if (strcmp (arg, "32") == 0)
	dialect &= ~(ppc_cpu_t) PPC_OPCODE_64;
      else if (strcmp (arg, "64") == 0)
	dialect |= PPC_OPCODE_64;
      else
	fprintf (stderr, _("warning: ignoring unknown -M%s option\n"), arg);

      if (end != NULL)
	*end++ = ',';
      arg = end;
    }

  if ((dialect & ~(ppc_cpu_t) PPC_OPCODE_64) == 0)
    {
      if (info->mach == bfd_mach_ppc64)
	dialect |= PPC_OPCODE_64;
      else
	dialect &= ~(ppc_cpu_t) PPC_OPCODE_64;
      /* Choose a reasonable default.  */
      dialect |= (PPC_OPCODE_PPC | PPC_OPCODE_COMMON | PPC_OPCODE_601
		  | PPC_OPCODE_ALTIVEC);
    }

  info->private_data = priv;
  POWERPC_DIALECT(info) = dialect;

  return TRUE;
}

/* Print a big endian PowerPC instruction.  */

int
print_insn_big_powerpc (bfd_vma memaddr, struct disassemble_info *info)
{
  if (info->private_data == NULL && !powerpc_init_dialect (info))
    return -1;
  return print_insn_powerpc (memaddr, info, 1, POWERPC_DIALECT(info));
}

/* Print a little endian PowerPC instruction.  */

int
print_insn_little_powerpc (bfd_vma memaddr, struct disassemble_info *info)
{
  if (info->private_data == NULL && !powerpc_init_dialect (info))
    return -1;
  return print_insn_powerpc (memaddr, info, 0, POWERPC_DIALECT(info));
}

/* Print a POWER (RS/6000) instruction.  */

int
print_insn_rs6000 (bfd_vma memaddr, struct disassemble_info *info)
{
  return print_insn_powerpc (memaddr, info, 1, PPC_OPCODE_POWER);
}

/* Extract the operand value from the PowerPC or POWER instruction.  */

static long
operand_value_powerpc (const struct powerpc_operand *operand,
		       unsigned long insn, ppc_cpu_t dialect)
{
  long value;
  int invalid;
  /* Extract the value from the instruction.  */
  if (operand->extract)
    value = (*operand->extract) (insn, dialect, &invalid);
  else
    {
      value = (insn >> operand->shift) & operand->bitm;
      if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
	{
	  /* BITM is always some number of zeros followed by some
	     number of ones, followed by some numer of zeros.  */
	  unsigned long top = operand->bitm;
	  /* top & -top gives the rightmost 1 bit, so this
	     fills in any trailing zeros.  */
	  top |= (top & -top) - 1;
	  top &= ~(top >> 1);
	  value = (value ^ top) - top;
	}
    }

  return value;
}

/* Determine whether the optional operand(s) should be printed.  */

static int
skip_optional_operands (const unsigned char *opindex,
			unsigned long insn, ppc_cpu_t dialect)
{
  const struct powerpc_operand *operand;

  for (; *opindex != 0; opindex++)
    {
      operand = &powerpc_operands[*opindex];
      if ((operand->flags & PPC_OPERAND_NEXT) != 0
	  || ((operand->flags & PPC_OPERAND_OPTIONAL) != 0
	      && operand_value_powerpc (operand, insn, dialect) != 0))
	return 0;
    }

  return 1;
}

/* Print a PowerPC or POWER instruction.  */

static int
print_insn_powerpc (bfd_vma memaddr,
		    struct disassemble_info *info,
		    int bigendian,
		    ppc_cpu_t dialect)
{
  bfd_byte buffer[4];
  int status;
  unsigned long insn;
  const struct powerpc_opcode *opcode;
  const struct powerpc_opcode *opcode_end;
  unsigned long op;
  ppc_cpu_t dialect_orig = dialect;

  status = (*info->read_memory_func) (memaddr, buffer, 4, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }

  if (bigendian)
    insn = bfd_getb32 (buffer);
  else
    insn = bfd_getl32 (buffer);

  /* Get the major opcode of the instruction.  */
  op = PPC_OP (insn);

  /* Find the first match in the opcode table.  We could speed this up
     a bit by doing a binary search on the major opcode.  */
  opcode_end = powerpc_opcodes + powerpc_num_opcodes;
 again:
  for (opcode = powerpc_opcodes; opcode < opcode_end; opcode++)
    {
      unsigned long table_op;
      const unsigned char *opindex;
      const struct powerpc_operand *operand;
      int invalid;
      int need_comma;
      int need_paren;
      int skip_optional;

      table_op = PPC_OP (opcode->opcode);
      if (op < table_op)
	break;
      if (op > table_op)
	continue;

      if ((insn & opcode->mask) != opcode->opcode
	  || (opcode->flags & dialect) == 0
	  || (opcode->deprecated & dialect_orig) != 0)
	continue;

      /* Make two passes over the operands.  First see if any of them
	 have extraction functions, and, if they do, make sure the
	 instruction is valid.  */
      invalid = 0;
      for (opindex = opcode->operands; *opindex != 0; opindex++)
	{
	  operand = powerpc_operands + *opindex;
	  if (operand->extract)
	    (*operand->extract) (insn, dialect, &invalid);
	}
      if (invalid)
	continue;

      /* The instruction is valid.  */
      if (opcode->operands[0] != 0)
	(*info->fprintf_func) (info->stream, "%-7s ", opcode->name);
      else
	(*info->fprintf_func) (info->stream, "%s", opcode->name);

      /* Now extract and print the operands.  */
      need_comma = 0;
      need_paren = 0;
      skip_optional = -1;
      for (opindex = opcode->operands; *opindex != 0; opindex++)
	{
	  long value;

	  operand = powerpc_operands + *opindex;

	  /* Operands that are marked FAKE are simply ignored.  We
	     already made sure that the extract function considered
	     the instruction to be valid.  */
	  if ((operand->flags & PPC_OPERAND_FAKE) != 0)
	    continue;

	  /* If all of the optional operands have the value zero,
	     then don't print any of them.  */
	  if ((operand->flags & PPC_OPERAND_OPTIONAL) != 0)
	    {
	      if (skip_optional < 0)
		skip_optional = skip_optional_operands (opindex, insn,
							dialect);
	      if (skip_optional)
		continue;
	    }

	  value = operand_value_powerpc (operand, insn, dialect);

	  if (need_comma)
	    {
	      (*info->fprintf_func) (info->stream, ",");
	      need_comma = 0;
	    }

	  /* Print the operand as directed by the flags.  */
	  if ((operand->flags & PPC_OPERAND_GPR) != 0
	      || ((operand->flags & PPC_OPERAND_GPR_0) != 0 && value != 0))
	    (*info->fprintf_func) (info->stream, "r%ld", value);
	  else if ((operand->flags & PPC_OPERAND_FPR) != 0)
	    (*info->fprintf_func) (info->stream, "f%ld", value);
	  else if ((operand->flags & PPC_OPERAND_VR) != 0)
	    (*info->fprintf_func) (info->stream, "v%ld", value);
	  else if ((operand->flags & PPC_OPERAND_VSR) != 0)
	    (*info->fprintf_func) (info->stream, "vs%ld", value);
	  else if ((operand->flags & PPC_OPERAND_RELATIVE) != 0)
	    (*info->print_address_func) (memaddr + value, info);
	  else if ((operand->flags & PPC_OPERAND_ABSOLUTE) != 0)
	    (*info->print_address_func) ((bfd_vma) value & 0xffffffff, info);
	  else if ((operand->flags & PPC_OPERAND_FSL) != 0) 
	    (*info->fprintf_func) (info->stream, "fsl%ld", value);
	  else if ((operand->flags & PPC_OPERAND_FCR) != 0)
	    (*info->fprintf_func) (info->stream, "fcr%ld", value);
	  else if ((operand->flags & PPC_OPERAND_UDI) != 0)
	    (*info->fprintf_func) (info->stream, "%ld", value);
	  else if ((operand->flags & PPC_OPERAND_CR) != 0
		   && (dialect & PPC_OPCODE_PPC) != 0)
	    {
	      if (operand->bitm == 7)
		(*info->fprintf_func) (info->stream, "cr%ld", value);
	      else
		{
		  static const char *cbnames[4] = { "lt", "gt", "eq", "so" };
		  int cr;
		  int cc;

		  cr = value >> 2;
		  if (cr != 0)
		    (*info->fprintf_func) (info->stream, "4*cr%d+", cr);
		  cc = value & 3;
		  (*info->fprintf_func) (info->stream, "%s", cbnames[cc]);
		}
	    }
	  else
	    (*info->fprintf_func) (info->stream, "%ld", value);

	  if (need_paren)
	    {
	      (*info->fprintf_func) (info->stream, ")");
	      need_paren = 0;
	    }

	  if ((operand->flags & PPC_OPERAND_PARENS) == 0)
	    need_comma = 1;
	  else
	    {
	      (*info->fprintf_func) (info->stream, "(");
	      need_paren = 1;
	    }
	}

      /* We have found and printed an instruction; return.  */
      return 4;
    }

  if ((dialect & PPC_OPCODE_ANY) != 0)
    {
      dialect = ~(ppc_cpu_t) PPC_OPCODE_ANY;
      goto again;
    }

  /* We could not find a match.  */
  (*info->fprintf_func) (info->stream, ".long 0x%lx", insn);

  return 4;
}

void
print_ppc_disassembler_options (FILE *stream)
{
  unsigned int i, col;

  fprintf (stream, _("\n\
The following PPC specific disassembler options are supported for use with\n\
the -M switch:\n"));

  for (col = 0, i = 0; i < sizeof (ppc_opts) / sizeof (ppc_opts[0]); i++)
    {
      col += fprintf (stream, " %s,", ppc_opts[i].opt);
      if (col > 66)
	{
	  fprintf (stream, "\n");
	  col = 0;
	}
    }
  fprintf (stream, " 32, 64\n");
}
