/* Disassembler code for Renesas RX.
   Copyright 2008, 2009, 2012 Free Software Foundation, Inc.
   Contributed by Red Hat.
   Written by DJ Delorie.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>

#include "bfd.h"
#include "dis-asm.h"
#include "opcode/rx.h"

typedef struct
{
  bfd_vma pc;
  disassemble_info * dis;
} RX_Data;

static int
rx_get_byte (void * vdata)
{
  bfd_byte buf[1];
  RX_Data *rx_data = (RX_Data *) vdata;

  rx_data->dis->read_memory_func (rx_data->pc,
				  buf,
				  1,
				  rx_data->dis);

  rx_data->pc ++;
  return buf[0];
}

static char const * size_names[] =
{
  "", ".b", ".ub", ".b", ".w", ".uw", ".w", ".a", ".l"
};

static char const * opsize_names[] =
{
  "", ".b", ".b", ".b", ".w", ".w", ".w", ".a", ".l"
};

static char const * register_names[] =
{
  /* general registers */
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  /* control register */
  "psw", "pc", "usp", "fpsw", "", "", "", "wr",
  "bpsw", "bpc", "isp", "fintv", "intb", "", "", "",
  "pbp", "pben", "", "", "", "", "", "",
  "bbpsw", "bbpc", "", "", "", "", "", ""
};

static char const * condition_names[] =
{
  /* condition codes */
  "eq", "ne", "c", "nc", "gtu", "leu", "pz", "n",
  "ge", "lt", "gt", "le", "o", "no", "always", "never"
};

static const char * flag_names[] =
{
  "c", "z", "s", "o", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "i", "u", "", "", "", "", "", ""
  "", "", "", "", "", "", "", "",
};

int
print_insn_rx (bfd_vma addr, disassemble_info * dis)
{
  int rv;
  RX_Data rx_data;
  RX_Opcode_Decoded opcode;
  const char * s;

  rx_data.pc = addr;
  rx_data.dis = dis;

  rv = rx_decode_opcode (addr, &opcode, rx_get_byte, &rx_data);

  dis->bytes_per_line = 10;

#define PR (dis->fprintf_func)
#define PS (dis->stream)
#define PC(c) PR (PS, "%c", c)

  for (s = opcode.syntax; *s; s++)
    {
      if (*s != '%')
	{
	  PC (*s);
	}
      else
	{
	  RX_Opcode_Operand * oper;
	  int do_size = 0;
	  int do_hex = 0;
	  int do_addr = 0;

	  s ++;

	  if (*s == 'S')
	    {
	      do_size = 1;
	      s++;
	    }
	  if (*s == 'x')
	    {
	      do_hex = 1;
	      s++;
	    }
	  if (*s == 'a')
	    {
	      do_addr = 1;
	      s++;
	    }

	  switch (*s)
	    {
	    case '%':
	      PC ('%');
	      break;

	    case 's':
	      PR (PS, "%s", opsize_names[opcode.size]);
	      break;

	    case '0':
	    case '1':
	    case '2':
	      oper = opcode.op + *s - '0';
	      if (do_size)
		{
		  if (oper->type == RX_Operand_Indirect)
		    PR (PS, "%s", size_names[oper->size]);
		}
	      else
		switch (oper->type)
		  {
		  case RX_Operand_Immediate:
		    if (do_addr)
		      dis->print_address_func (oper->addend, dis);
		    else if (do_hex
			     || oper->addend > 999
			     || oper->addend < -999)
		      PR (PS, "%#x", oper->addend);
		    else
		      PR (PS, "%d", oper->addend);
		    break;
		  case RX_Operand_Register:
		  case RX_Operand_TwoReg:
		    PR (PS, "%s", register_names[oper->reg]);
		    break;
		  case RX_Operand_Indirect:
		    if (oper->addend)
		      PR (PS, "%d[%s]", oper->addend, register_names[oper->reg]);
		    else
		      PR (PS, "[%s]", register_names[oper->reg]);
		    break;
		  case RX_Operand_Postinc:
		    PR (PS, "[%s+]", register_names[oper->reg]);
		    break;
		  case RX_Operand_Predec:
		    PR (PS, "[-%s]", register_names[oper->reg]);
		    break;
		  case RX_Operand_Condition:
		    PR (PS, "%s", condition_names[oper->reg]);
		    break;
		  case RX_Operand_Flag:
		    PR (PS, "%s", flag_names[oper->reg]);
		    break;
		  default:
		    PR (PS, "[???]");
		    break;
		  }
	    }
	}
    }

  return rv;
}
