/* Disassemble h8500 instructions.
   Copyright (C) 1993-2017 Free Software Foundation, Inc.

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

#define DISASSEMBLER_TABLE
#define DEFINE_TABLE

#include "h8500-opc.h"
#include "dis-asm.h"
#include "opintl.h"

/* Maximum length of an instruction.  */
#define MAXLEN 8

#include <setjmp.h>

struct private
{
  /* Points to first byte not fetched.  */
  bfd_byte *max_fetched;
  bfd_byte the_buffer[MAXLEN];
  bfd_vma insn_start;
  OPCODES_SIGJMP_BUF bailout;
};

/* Make sure that bytes from INFO->PRIVATE_DATA->BUFFER (inclusive)
   to ADDR (exclusive) are valid.  Returns 1 for success, longjmps
   on error.  */
#define FETCH_DATA(info, addr) \
  ((addr) <= ((struct private *)(info->private_data))->max_fetched \
   ? 1 : fetch_data ((info), (addr)))

static int
fetch_data (struct disassemble_info *info, bfd_byte *addr)
{
  int status;
  struct private *priv = (struct private *) info->private_data;
  bfd_vma start = priv->insn_start + (priv->max_fetched - priv->the_buffer);

  status = (*info->read_memory_func) (start,
				      priv->max_fetched,
				      addr - priv->max_fetched,
				      info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, start, info);
      OPCODES_SIGLONGJMP (priv->bailout, 1);
    }
  else
    priv->max_fetched = addr;
  return 1;
}

static char *crname[] = { "sr", "ccr", "*", "br", "ep", "dp", "*", "tp" };

int
print_insn_h8500 (bfd_vma addr, disassemble_info *info)
{
  const h8500_opcode_info *opcode;
  void *stream = info->stream;
  fprintf_ftype func = info->fprintf_func;
  struct private priv;
  bfd_byte *buffer = priv.the_buffer;

  info->private_data = (PTR) & priv;
  priv.max_fetched = priv.the_buffer;
  priv.insn_start = addr;
  if (OPCODES_SIGSETJMP (priv.bailout) != 0)
    /* Error return.  */
    return -1;

  /* Run down the table to find the one which matches.  */
  for (opcode = h8500_table; opcode->name; opcode++)
    {
      int byte;
      int rn = 0;
      int rd = 0;
      int rs = 0;
      int disp = 0;
      int abs_val = 0;
      int imm = 0;
      int pcrel = 0;
      int qim = 0;
      int i;
      int cr = 0;

      for (byte = 0; byte < opcode->length; byte++)
	{
	  FETCH_DATA (info, buffer + byte + 1);
	  if ((buffer[byte] & opcode->bytes[byte].mask)
	      != (opcode->bytes[byte].contents))
	    goto next;

	  else
	    {
	      /* Extract any info parts.  */
	      switch (opcode->bytes[byte].insert)
		{
		case 0:
		case FP:
		  break;
		default:
		  /* xgettext:c-format */
		  func (stream, _("can't cope with insert %d\n"),
			opcode->bytes[byte].insert);
		  break;
		case RN:
		  rn = buffer[byte] & 0x7;
		  break;
		case RS:
		  rs = buffer[byte] & 0x7;
		  break;
		case CRB:
		  cr = buffer[byte] & 0x7;
		  if (cr == 0)
		    goto next;
		  break;
		case CRW:
		  cr = buffer[byte] & 0x7;
		  if (cr != 0)
		    goto next;
		  break;
		case DISP16:
		  FETCH_DATA (info, buffer + byte + 2);
		  disp = (buffer[byte] << 8) | (buffer[byte + 1]);
		  break;
		case FPIND_D8:
		case DISP8:
		  disp = ((char) (buffer[byte]));
		  break;
		case RD:
		case RDIND:
		  rd = buffer[byte] & 0x7;
		  break;
		case ABS24:
		  FETCH_DATA (info, buffer + byte + 3);
		  abs_val =
		    (buffer[byte] << 16)
		    | (buffer[byte + 1] << 8)
		    | (buffer[byte + 2]);
		  break;
		case ABS16:
		  FETCH_DATA (info, buffer + byte + 2);
		  abs_val = (buffer[byte] << 8) | (buffer[byte + 1]);
		  break;
		case ABS8:
		  abs_val = (buffer[byte]);
		  break;
		case IMM16:
		  FETCH_DATA (info, buffer + byte + 2);
		  imm = (buffer[byte] << 8) | (buffer[byte + 1]);
		  break;
		case IMM4:
		  imm = (buffer[byte]) & 0xf;
		  break;
		case IMM8:
		case RLIST:
		  imm = (buffer[byte]);
		  break;
		case PCREL16:
		  FETCH_DATA (info, buffer + byte + 2);
		  pcrel = (buffer[byte] << 8) | (buffer[byte + 1]);
		  break;
		case PCREL8:
		  pcrel = (buffer[byte]);
		  break;
		case QIM:
		  switch (buffer[byte] & 0x7)
		    {
		    case 0:
		      qim = 1;
		      break;
		    case 1:
		      qim = 2;
		      break;
		    case 4:
		      qim = -1;
		      break;
		    case 5:
		      qim = -2;
		      break;
		    }
		  break;

		}
	    }
	}
      /* We get here when all the masks have passed so we can output
	 the operands.  */
      FETCH_DATA (info, buffer + opcode->length);
      (func) (stream, "%s\t", opcode->name);
      for (i = 0; i < opcode->nargs; i++)
	{
	  if (i)
	    (func) (stream, ",");
	  switch (opcode->arg_type[i])
	    {
	    case FP:
	      func (stream, "fp");
	      break;
	    case RNIND_D16:
	      func (stream, "@(0x%x:16,r%d)", disp, rn);
	      break;
	    case RNIND_D8:
	      func (stream, "@(0x%x:8 (%d),r%d)", disp & 0xff, disp, rn);
	      break;
	    case RDIND_D16:
	      func (stream, "@(0x%x:16,r%d)", disp, rd);
	      break;
	    case RDIND_D8:
	      func (stream, "@(0x%x:8 (%d), r%d)", disp & 0xff, disp, rd);
	      break;
	    case FPIND_D8:
	      func (stream, "@(0x%x:8 (%d), fp)", disp & 0xff, disp);
	      break;
	    case CRB:
	    case CRW:
	      func (stream, "%s", crname[cr]);
	      break;
	    case RN:
	      func (stream, "r%d", rn);
	      break;
	    case RD:
	      func (stream, "r%d", rd);
	      break;
	    case RS:
	      func (stream, "r%d", rs);
	      break;
	    case RNDEC:
	      func (stream, "@-r%d", rn);
	      break;
	    case RNINC:
	      func (stream, "@r%d+", rn);
	      break;
	    case RNIND:
	      func (stream, "@r%d", rn);
	      break;
	    case RDIND:
	      func (stream, "@r%d", rd);
	      break;
	    case SPINC:
	      func (stream, "@sp+");
	      break;
	    case SPDEC:
	      func (stream, "@-sp");
	      break;
	    case ABS24:
	      func (stream, "@0x%0x:24", abs_val);
	      break;
	    case ABS16:
	      func (stream, "@0x%0x:16", abs_val & 0xffff);
	      break;
	    case ABS8:
	      func (stream, "@0x%0x:8", abs_val & 0xff);
	      break;
	    case IMM16:
	      func (stream, "#0x%0x:16", imm & 0xffff);
	      break;
	    case RLIST:
	      {
		int j;
		int nc = 0;

		func (stream, "(");
		for (j = 0; j < 8; j++)
		  {
		    if (imm & (1 << j))
		      {
			func (stream, "r%d", j);
			if (nc)
			  func (stream, ",");
			nc = 1;
		      }
		  }
		func (stream, ")");
	      }
	      break;
	    case IMM8:
	      func (stream, "#0x%0x:8", imm & 0xff);
	      break;
	    case PCREL16:
	      func (stream, "0x%0x:16",
		    (int)(pcrel + addr + opcode->length) & 0xffff);
	      break;
	    case PCREL8:
	      func (stream, "#0x%0x:8",
		    (int)((char) pcrel + addr + opcode->length) & 0xffff);
	      break;
	    case QIM:
	      func (stream, "#%d:q", qim);
	      break;
	    case IMM4:
	      func (stream, "#%d:4", imm);
	      break;
	    }
	}
      return opcode->length;
    next:
      ;
    }

  /* Couldn't understand anything.  */
  /* xgettext:c-format */
  func (stream, _("%02x\t\t*unknown*"), buffer[0]);
  return 1;
}
