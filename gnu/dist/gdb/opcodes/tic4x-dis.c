/* Print instructions for the Texas TMS320C[34]X, for GDB and GNU Binutils.

   Copyright 2002 Free Software Foundation, Inc.

   Contributed by Michael P. Hayes (m.hayes@elec.canterbury.ac.nz)
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <math.h>
#include "libiberty.h"
#include "dis-asm.h"
#include "opcode/tic4x.h"

#define C4X_DEBUG 0

#define C4X_HASH_SIZE 11 /* 11 and above should give unique entries.  */

typedef enum
  {
    IMMED_SINT,
    IMMED_SUINT,
    IMMED_SFLOAT,
    IMMED_INT,
    IMMED_UINT,
    IMMED_FLOAT
  }
immed_t;

typedef enum
  {
    INDIRECT_SHORT,
    INDIRECT_LONG,
    INDIRECT_C4X
  }
indirect_t;

static int c4x_version = 0;
static int c4x_dp = 0;

static int
c4x_pc_offset (unsigned int op)
{
  /* Determine the PC offset for a C[34]x instruction.
     This could be simplified using some boolean algebra
     but at the expense of readability.  */
  switch (op >> 24)
    {
    case 0x60:	/* br */
    case 0x62:	/* call  (C4x) */
    case 0x64:	/* rptb  (C4x) */
      return 1;
    case 0x61: 	/* brd */
    case 0x63: 	/* laj */
    case 0x65:	/* rptbd (C4x) */
      return 3;
    case 0x66: 	/* swi */
    case 0x67:
      return 0;
    default:
      break;
    }
  
  switch ((op & 0xffe00000) >> 20)
    {
    case 0x6a0:	/* bB */
    case 0x720: /* callB */
    case 0x740: /* trapB */
      return 1;
      
    case 0x6a2: /* bBd */
    case 0x6a6: /* bBat */
    case 0x6aa: /* bBaf */
    case 0x722:	/* lajB */
    case 0x748: /* latB */
    case 0x798: /* rptbd */
      return 3;
      
    default:
      break;
    }
  
  switch ((op & 0xfe200000) >> 20)
    {
    case 0x6e0:	/* dbB */
      return 1;
      
    case 0x6e2:	/* dbBd */
      return 3;
      
    default:
      break;
    }
  
  return 0;
}

static int
c4x_print_char (struct disassemble_info * info, char ch)
{
  if (info != NULL)
    (*info->fprintf_func) (info->stream, "%c", ch);
  return 1;
}

static int
c4x_print_str (struct disassemble_info *info, char *str)
{
  if (info != NULL)
    (*info->fprintf_func) (info->stream, "%s", str);
  return 1;
}

static int
c4x_print_register (struct disassemble_info *info,
		    unsigned long regno)
{
  static c4x_register_t **registertable = NULL;
  unsigned int i;
  
  if (registertable == NULL)
    {
      registertable = (c4x_register_t **)
	xmalloc (sizeof (c4x_register_t *) * REG_TABLE_SIZE);
      for (i = 0; i < c3x_num_registers; i++)
	registertable[c3x_registers[i].regno] = (void *)&c3x_registers[i];
      if (IS_CPU_C4X (c4x_version))
	{
	  /* Add C4x additional registers, overwriting
	     any C3x registers if necessary.  */
	  for (i = 0; i < c4x_num_registers; i++)
	    registertable[c4x_registers[i].regno] = (void *)&c4x_registers[i];
	}
    }
  if ((int) regno > (IS_CPU_C4X (c4x_version) ? C4X_REG_MAX : C3X_REG_MAX))
    return 0;
  if (info != NULL)
    (*info->fprintf_func) (info->stream, "%s", registertable[regno]->name);
  return 1;
}

static int
c4x_print_addr (struct disassemble_info *info,
		unsigned long addr)
{
  if (info != NULL)
    (*info->print_address_func)(addr, info);
  return 1;
}

static int
c4x_print_relative (struct disassemble_info *info,
		    unsigned long pc,
		    long offset,
		    unsigned long opcode)
{
  return c4x_print_addr (info, pc + offset + c4x_pc_offset (opcode));
}

static int
c4x_print_direct (struct disassemble_info *info,
		  unsigned long arg)
{
  if (info != NULL)
    {
      (*info->fprintf_func) (info->stream, "@");
      c4x_print_addr (info, arg + (c4x_dp << 16));
    }
  return 1;
}

/* FIXME: make the floating point stuff not rely on host
   floating point arithmetic.  */
void
c4x_print_ftoa (unsigned int val,
		FILE *stream,
		int (*pfunc)())
{
  int e;
  int s;
  int f;
  double num = 0.0;
  
  e = EXTRS (val, 31, 24);	/* exponent */
  if (e != -128)
    {
      s = EXTRU (val, 23, 23);	/* sign bit */
      f = EXTRU (val, 22, 0);	/* mantissa */
      if (s)
	f += -2 * (1 << 23);
      else
	f += (1 << 23);
      num = f / (double)(1 << 23);
      num = ldexp (num, e);
    }    
  (*pfunc)(stream, "%.9g", num);
}

static int
c4x_print_immed (struct disassemble_info *info,
		 immed_t type,
		 unsigned long arg)
{
  int s;
  int f;
  int e;
  double num = 0.0;
  
  if (info == NULL)
    return 1;
  switch (type)
    {
    case IMMED_SINT:
    case IMMED_INT:
      (*info->fprintf_func) (info->stream, "%d", (long)arg);
      break;
      
    case IMMED_SUINT:
    case IMMED_UINT:
      (*info->fprintf_func) (info->stream, "%u", arg);
      break;
      
    case IMMED_SFLOAT:
      e = EXTRS (arg, 15, 12);
      if (e != -8)
	{
	  s = EXTRU (arg, 11, 11);
	  f = EXTRU (arg, 10, 0);
	  if (s)
	    f += -2 * (1 << 11);
	  else
	    f += (1 << 11);
	  num = f / (double)(1 << 11);
	  num = ldexp (num, e);
	}
      (*info->fprintf_func) (info->stream, "%f", num);
      break;
    case IMMED_FLOAT:
      e = EXTRS (arg, 31, 24);
      if (e != -128)
	{
	  s = EXTRU (arg, 23, 23);
	  f = EXTRU (arg, 22, 0);
	  if (s)
	    f += -2 * (1 << 23);
	  else
	    f += (1 << 23);
	  num = f / (double)(1 << 23);
	  num = ldexp (num, e);
	}
      (*info->fprintf_func) (info->stream, "%f", num);
      break;
    }
  return 1;
}

static int
c4x_print_cond (struct disassemble_info *info,
		unsigned int cond)
{
  static c4x_cond_t **condtable = NULL;
  unsigned int i;
  
  if (condtable == NULL)
    {
      condtable = (c4x_cond_t **)xmalloc (sizeof (c4x_cond_t *) * 32);
      for (i = 0; i < num_conds; i++)
	condtable[c4x_conds[i].cond] = (void *)&c4x_conds[i];
    }
  if (cond > 31 || condtable[cond] == NULL)
    return 0;
  if (info != NULL)
    (*info->fprintf_func) (info->stream, "%s", condtable[cond]->name);
  return 1;
}

static int
c4x_print_indirect (struct disassemble_info *info,
		    indirect_t type,
		    unsigned long arg)
{
  unsigned int aregno;
  unsigned int modn;
  unsigned int disp;
  char *a;

  aregno = 0;
  modn = 0;
  disp = 1;
  switch(type)
    {
    case INDIRECT_C4X:		/* *+ARn(disp) */
      disp = EXTRU (arg, 7, 3);
      aregno = EXTRU (arg, 2, 0) + REG_AR0;
      modn = 0;
      break;
    case INDIRECT_SHORT:
      disp = 1;
      aregno = EXTRU (arg, 2, 0) + REG_AR0;
      modn = EXTRU (arg, 7, 3);
      break;
    case INDIRECT_LONG:
      disp = EXTRU (arg, 7, 0);
      aregno = EXTRU (arg, 10, 8) + REG_AR0;
      modn = EXTRU (arg, 15, 11);
      if (modn > 7 && disp != 0)
	return 0;
      break;
    default:
      abort ();
    }
  if (modn > C3X_MODN_MAX)
    return 0;
  a = c4x_indirects[modn].name;
  while (*a)
    {
      switch (*a)
	{
	case 'a':
	  c4x_print_register (info, aregno);
	  break;
	case 'd':
	  c4x_print_immed (info, IMMED_UINT, disp);
	  break;
	case 'y':
	  c4x_print_str (info, "ir0");
	  break;
	case 'z':
	  c4x_print_str (info, "ir1");
	  break;
	default:
	  c4x_print_char (info, *a);
	  break;
	}
      a++;
    }
  return 1;
}

static int
c4x_print_op (struct disassemble_info *info,
	      unsigned long instruction,
	      c4x_inst_t *p, unsigned long pc)
{
  int val;
  char *s;
  char *parallel = NULL;

  /* Print instruction name.  */
  s = p->name;
  while (*s && parallel == NULL)
    {
      switch (*s)
	{
	case 'B':
	  if (! c4x_print_cond (info, EXTRU (instruction, 20, 16)))
	    return 0;
	  break;
	case 'C':
	  if (! c4x_print_cond (info, EXTRU (instruction, 27, 23)))
	    return 0;
	  break;
	case '_':
	  parallel = s + 1;	/* Skip past `_' in name */
	  break;
	default:
	  c4x_print_char (info, *s);
	  break;
	}
      s++;
    }
  
  /* Print arguments.  */
  s = p->args;
  if (*s)
    c4x_print_char (info, ' ');

  while (*s)
    {
      switch (*s)
	{
	case '*': /* indirect 0--15 */
	  if (! c4x_print_indirect (info, INDIRECT_LONG,
				    EXTRU (instruction, 15, 0)))
	    return 0;
	  break;

	case '#': /* only used for ldp, ldpk */
	  c4x_print_immed (info, IMMED_UINT, EXTRU (instruction, 15, 0));
	  break;

	case '@': /* direct 0--15 */
	  c4x_print_direct (info, EXTRU (instruction, 15, 0));
	  break;

	case 'A': /* address register 24--22 */
	  if (! c4x_print_register (info, EXTRU (instruction, 24, 22) +
				    REG_AR0))
	    return 0;
	  break;

	case 'B': /* 24-bit unsigned int immediate br(d)/call/rptb
		     address 0--23.  */
	  if (IS_CPU_C4X (c4x_version))
	    c4x_print_relative (info, pc, EXTRS (instruction, 23, 0),
				p->opcode);
	  else
	    c4x_print_addr (info, EXTRU (instruction, 23, 0));
	  break;

	case 'C': /* indirect (short C4x) 0--7 */
	  if (! IS_CPU_C4X (c4x_version))
	    return 0;
	  if (! c4x_print_indirect (info, INDIRECT_C4X,
				    EXTRU (instruction, 7, 0)))
	    return 0;
	  break;

	case 'D':
	  /* Cockup if get here...  */
	  break;

	case 'E': /* register 0--7 */
	  if (! c4x_print_register (info, EXTRU (instruction, 7, 0)))
	    return 0;
	  break;

	case 'F': /* 16-bit float immediate 0--15 */
	  c4x_print_immed (info, IMMED_SFLOAT,
			   EXTRU (instruction, 15, 0));
	  break;

	case 'I': /* indirect (short) 0--7 */
	  if (! c4x_print_indirect (info, INDIRECT_SHORT,
				    EXTRU (instruction, 7, 0)))
	    return 0;
	  break;

	case 'J': /* indirect (short) 8--15 */
	  if (! c4x_print_indirect (info, INDIRECT_SHORT,
				    EXTRU (instruction, 15, 8)))
	    return 0;
	  break;

	case 'G': /* register 8--15 */
	  if (! c4x_print_register (info, EXTRU (instruction, 15, 8)))
	    return 0;
	  break;

	case 'H': /* register 16--18 */
	  if (! c4x_print_register (info, EXTRU (instruction, 18, 16)))
	    return 0;
	  break;

	case 'K': /* register 19--21 */
	  if (! c4x_print_register (info, EXTRU (instruction, 21, 19)))
	    return 0;
	  break;

	case 'L': /* register 22--24 */
	  if (! c4x_print_register (info, EXTRU (instruction, 24, 22)))
	    return 0;
	  break;

	case 'M': /* register 22--22 */
	  c4x_print_register (info, EXTRU (instruction, 22, 22) + REG_R2);
	  break;

	case 'N': /* register 23--23 */
	  c4x_print_register (info, EXTRU (instruction, 22, 22) + REG_R0);
	  break;

	case 'O': /* indirect (short C4x) 8--15 */
	  if (! IS_CPU_C4X (c4x_version))
	    return 0;
	  if (! c4x_print_indirect (info, INDIRECT_C4X,
				    EXTRU (instruction, 15, 8)))
	    return 0;
	  break;

	case 'P': /* displacement 0--15 (used by Bcond and BcondD) */
	  c4x_print_relative (info, pc, EXTRS (instruction, 15, 0),
			      p->opcode);
	  break;

	case 'Q': /* register 0--15 */
	  if (! c4x_print_register (info, EXTRU (instruction, 15, 0)))
	    return 0;
	  break;

	case 'R': /* register 16--20 */
	  if (! c4x_print_register (info, EXTRU (instruction, 20, 16)))
	    return 0;
	  break;

	case 'S': /* 16-bit signed immediate 0--15 */
	  c4x_print_immed (info, IMMED_SINT,
			   EXTRS (instruction, 15, 0));
	  break;

	case 'T': /* 5-bit signed immediate 16--20  (C4x stik) */
	  if (! IS_CPU_C4X (c4x_version))
	    return 0;
	  if (! c4x_print_immed (info, IMMED_SUINT,
				 EXTRU (instruction, 20, 16)))
	    return 0;
	  break;

	case 'U': /* 16-bit unsigned int immediate 0--15 */
	  c4x_print_immed (info, IMMED_SUINT, EXTRU (instruction, 15, 0));
	  break;

	case 'V': /* 5/9-bit unsigned vector 0--4/8 */
	  c4x_print_immed (info, IMMED_SUINT,
			   IS_CPU_C4X (c4x_version) ? 
			   EXTRU (instruction, 8, 0) :
			   EXTRU (instruction, 4, 0) & ~0x20);
	  break;

	case 'W': /* 8-bit signed immediate 0--7 */
	  if (! IS_CPU_C4X (c4x_version))
	    return 0;
	  c4x_print_immed (info, IMMED_SINT, EXTRS (instruction, 7, 0));
	  break;

	case 'X': /* expansion register 4--0 */
	  val = EXTRU (instruction, 4, 0) + REG_IVTP;
	  if (val < REG_IVTP || val > REG_TVTP)
	    return 0;
	  if (! c4x_print_register (info, val))
	    return 0;
	  break;

	case 'Y': /* address register 16--20 */
	  val = EXTRU (instruction, 20, 16);
	  if (val < REG_AR0 || val > REG_SP)
	    return 0;
	  if (! c4x_print_register (info, val))
	    return 0;
	  break;

	case 'Z': /* expansion register 16--20 */
	  val = EXTRU (instruction, 20, 16) + REG_IVTP;
	  if (val < REG_IVTP || val > REG_TVTP)
	    return 0;
	  if (! c4x_print_register (info, val))
	    return 0;
	  break;

	case '|':		/* Parallel instruction */
	  c4x_print_str (info, " || ");
	  c4x_print_str (info, parallel);
	  c4x_print_char (info, ' ');
	  break;

	case ';':
	  c4x_print_char (info, ',');
	  break;

	default:
	  c4x_print_char (info, *s);
	  break;
	}
      s++;
    }
  return 1;
}

static void
c4x_hash_opcode (c4x_inst_t **optable,
		 const c4x_inst_t *inst)
{
  int j;
  int opcode = inst->opcode >> (32 - C4X_HASH_SIZE);
  int opmask = inst->opmask >> (32 - C4X_HASH_SIZE);
  
  /* Use a C4X_HASH_SIZE bit index as a hash index.  We should
     have unique entries so there's no point having a linked list
     for each entry? */
  for (j = opcode; j < opmask; j++)
    if ((j & opmask) == opcode)
      {
#if C4X_DEBUG
	/* We should only have collisions for synonyms like
	   ldp for ldi.  */
	if (optable[j] != NULL)
	  printf("Collision at index %d, %s and %s\n",
		 j, optable[j]->name, inst->name);
#endif
	optable[j] = (void *)inst;
      }
}

/* Disassemble the instruction in 'instruction'.
   'pc' should be the address of this instruction, it will
   be used to print the target address if this is a relative jump or call
   the disassembled instruction is written to 'info'.
   The function returns the length of this instruction in words.  */

static int
c4x_disassemble (unsigned long pc,
		 unsigned long instruction,
		 struct disassemble_info *info)
{
  static c4x_inst_t **optable = NULL;
  c4x_inst_t *p;
  int i;
  
  c4x_version = info->mach;
  
  if (optable == NULL)
    {
      optable = (c4x_inst_t **)
	xcalloc (sizeof (c4x_inst_t *), (1 << C4X_HASH_SIZE));
      /* Install opcodes in reverse order so that preferred
	 forms overwrite synonyms.  */
      for (i = c3x_num_insts - 1; i >= 0; i--)
	c4x_hash_opcode (optable, &c3x_insts[i]);
      if (IS_CPU_C4X (c4x_version))
	{
	  for (i = c4x_num_insts - 1; i >= 0; i--)
	    c4x_hash_opcode (optable, &c4x_insts[i]);
	}
    }
  
  /* See if we can pick up any loading of the DP register...  */
  if ((instruction >> 16) == 0x5070 || (instruction >> 16) == 0x1f70)
    c4x_dp = EXTRU (instruction, 15, 0);
  
  p = optable[instruction >> (32 - C4X_HASH_SIZE)];
  if (p != NULL && ((instruction & p->opmask) == p->opcode)
      && c4x_print_op (NULL, instruction, p, pc))
    c4x_print_op (info, instruction, p, pc);
  else
    (*info->fprintf_func) (info->stream, "%08x", instruction);

  /* Return size of insn in words.  */
  return 1;	
}

/* The entry point from objdump and gdb.  */
int
print_insn_tic4x (memaddr, info)
     bfd_vma memaddr;
     struct disassemble_info *info;
{
  int status;
  unsigned long pc;
  unsigned long op;
  bfd_byte buffer[4];
  
  status = (*info->read_memory_func) (memaddr, buffer, 4, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }
  
  pc = memaddr;
  op = bfd_getl32 (buffer);
  info->bytes_per_line = 4;
  info->bytes_per_chunk = 4;
  info->octets_per_byte = 4;
  info->display_endian = BFD_ENDIAN_LITTLE;
  return c4x_disassemble (pc, op, info) * 4;
}
