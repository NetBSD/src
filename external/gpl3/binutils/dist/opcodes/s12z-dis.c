/* s12z-dis.c -- Freescale S12Z disassembly
   Copyright (C) 2018 Free Software Foundation, Inc.

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
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "s12z.h"

#include "bfd.h"
#include "dis-asm.h"


#include "disassemble.h"

static int
read_memory (bfd_vma memaddr, bfd_byte* buffer, int size,
             struct disassemble_info* info)
{
  int status = (*info->read_memory_func) (memaddr, buffer, size, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }
  return 0;
}

typedef int (* insn_bytes_f) (bfd_vma memaddr,
			      struct disassemble_info* info);

typedef void (*operands_f) (bfd_vma memaddr, struct disassemble_info* info);

enum OPR_MODE
  {
    OPR_IMMe4,
    OPR_REG,
    OPR_OFXYS,
    OPR_XY_PRE_INC,
    OPR_XY_POST_INC,
    OPR_XY_PRE_DEC,
    OPR_XY_POST_DEC,
    OPR_S_PRE_DEC,
    OPR_S_POST_INC,
    OPR_REG_DIRECT,
    OPR_REG_INDIRECT,
    OPR_IDX_DIRECT,
    OPR_IDX_INDIRECT,
    OPR_EXT1,
    OPR_IDX2_REG,
    OPR_IDX3_DIRECT,
    OPR_IDX3_INDIRECT,

    OPR_EXT18,
    OPR_IDX3_DIRECT_REG,
    OPR_EXT3_DIRECT,
    OPR_EXT3_INDIRECT
  };

struct opr_pb
{
  uint8_t mask;
  uint8_t value;
  int n_operands;
  enum OPR_MODE mode;
};

static const  struct opr_pb opr_pb[] = {
  {0xF0, 0x70, 1, OPR_IMMe4},
  {0xF8, 0xB8, 1, OPR_REG},
  {0xC0, 0x40, 1, OPR_OFXYS},
  {0xEF, 0xE3, 1, OPR_XY_PRE_INC},
  {0xEF, 0xE7, 1, OPR_XY_POST_INC},
  {0xEF, 0xC3, 1, OPR_XY_PRE_DEC},
  {0xEF, 0xC7, 1, OPR_XY_POST_DEC},
  {0xFF, 0xFB, 1, OPR_S_PRE_DEC},
  {0xFF, 0xFF, 1, OPR_S_POST_INC},
  {0xC8, 0x88, 1, OPR_REG_DIRECT},
  {0xE8, 0xC8, 1, OPR_REG_INDIRECT},

  {0xCE, 0xC0, 2, OPR_IDX_DIRECT},
  {0xCE, 0xC4, 2, OPR_IDX_INDIRECT},
  {0xC0, 0x00, 2, OPR_EXT1},

  {0xC8, 0x80, 3, OPR_IDX2_REG},
  {0xFA, 0xF8, 3, OPR_EXT18},

  {0xCF, 0xC2, 4, OPR_IDX3_DIRECT},
  {0xCF, 0xC6, 4, OPR_IDX3_INDIRECT},

  {0xF8, 0xE8, 4, OPR_IDX3_DIRECT_REG},
  {0xFF, 0xFA, 4, OPR_EXT3_DIRECT},
  {0xFF, 0xFE, 4, OPR_EXT3_INDIRECT},
};


/* Return the number of bytes in a OPR operand, including the XB postbyte.
   It does not include any preceeding opcodes. */
static int
opr_n_bytes (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte xb;
  int status = read_memory (memaddr, &xb, 1, info);
  if (status < 0)
    return status;

  size_t i;
  for (i = 0; i < sizeof (opr_pb) / sizeof (opr_pb[0]); ++i)
    {
      const struct opr_pb *pb = opr_pb + i;
      if ((xb & pb->mask) == pb->value)
	{
	  return pb->n_operands;
	}
    }

  return 1;
}

static int
opr_n_bytes_p1 (bfd_vma memaddr, struct disassemble_info* info)
{
  return 1 + opr_n_bytes (memaddr, info);
}

static int
opr_n_bytes2 (bfd_vma memaddr, struct disassemble_info* info)
{
  int s = opr_n_bytes (memaddr, info);
  s += opr_n_bytes (memaddr + s, info);
  return s + 1;
}

enum BB_MODE
  {
    BB_REG_REG_REG,
    BB_REG_REG_IMM,
    BB_REG_OPR_REG,
    BB_OPR_REG_REG,
    BB_REG_OPR_IMM,
    BB_OPR_REG_IMM
  };

struct opr_bb
{
  uint8_t mask;
  uint8_t value;
  int n_operands;
  bool opr;
  enum BB_MODE mode;
};

static const struct opr_bb bb_modes[] =
  {
    {0x60, 0x00, 2, false, BB_REG_REG_REG},
    {0x60, 0x20, 3, false, BB_REG_REG_IMM},
    {0x70, 0x40, 2, true,  BB_REG_OPR_REG},
    {0x70, 0x50, 2, true,  BB_OPR_REG_REG},
    {0x70, 0x60, 3, true,  BB_REG_OPR_IMM},
    {0x70, 0x70, 3, true,  BB_OPR_REG_IMM}
  };

static int
bfextins_n_bytes (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte bb;
  int status = read_memory (memaddr, &bb, 1, info);
  if (status < 0)
    return status;

  size_t i;
  const struct opr_bb *bbs = 0;
  for (i = 0; i < sizeof (bb_modes) / sizeof (bb_modes[0]); ++i)
    {
      bbs = bb_modes + i;
      if ((bb & bbs->mask) == bbs->value)
	{
	  break;
	}
    }

  int n = bbs->n_operands;
  if (bbs->opr)
    n += opr_n_bytes (memaddr + n - 1, info);

  return n;
}

static int
single (bfd_vma memaddr ATTRIBUTE_UNUSED,
	struct disassemble_info* info ATTRIBUTE_UNUSED)
{
  return 1;
}

static int
two (bfd_vma memaddr ATTRIBUTE_UNUSED,
     struct disassemble_info* info ATTRIBUTE_UNUSED)
{
  return 2;
}

static int
three (bfd_vma memaddr ATTRIBUTE_UNUSED,
       struct disassemble_info* info ATTRIBUTE_UNUSED)
{
  return 3;
}

static int
four (bfd_vma memaddr ATTRIBUTE_UNUSED,
      struct disassemble_info* info ATTRIBUTE_UNUSED)
{
  return 4;
}

static int
five (bfd_vma memaddr ATTRIBUTE_UNUSED,
      struct disassemble_info* info ATTRIBUTE_UNUSED)
{
  return 5;
}

static int
pcrel_15bit (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr, &byte, 1, info);
  if (status < 0)
    return status;
  return (byte & 0x80) ? 3 : 2;
}




static void
operand_separator (struct disassemble_info *info)
{
  if ((info->flags & 0x2))
    {
      (*info->fprintf_func) (info->stream, ", ");
    }
  else
    {
      (*info->fprintf_func) (info->stream, " ");
    }

  info->flags |= 0x2;
}



static void
imm1 (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr, &byte, 1, info);
  if (status < 0)
    return;

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "#%d", byte);
}

static void
trap_decode (bfd_vma memaddr, struct disassemble_info* info)
{
  imm1 (memaddr - 1, info);
}


const struct reg registers[S12Z_N_REGISTERS] =
  {
    {"d2", 2},
    {"d3", 2},
    {"d4", 2},
    {"d5", 2},

    {"d0", 1},
    {"d1", 1},

    {"d6", 4},
    {"d7", 4},

    {"x", 3},
    {"y", 3},
    {"s", 3},
    {"p", 3},
    {"cch", 1},
    {"ccl", 1},
    {"ccw", 2}
  };

static char *
xys_from_postbyte (uint8_t postbyte)
{
  char *reg = "?";
  switch ((postbyte & 0x30) >> 4)
    {
    case 0:
      reg = "x";
      break;
    case 1:
      reg = "y";
      break;
    case 2:
      reg = "s";
      break;
    default:
      reg = "?";
      break;
    }
  return reg;
}

static char *
xysp_from_postbyte (uint8_t postbyte)
{
  char *reg = "?";
  switch ((postbyte & 0x30) >> 4)
    {
    case 0:
      reg = "x";
      break;
    case 1:
      reg = "y";
      break;
    case 2:
      reg = "s";
      break;
    default:
      reg = "p";
      break;
    }
  return reg;
}

/* Render the symbol name whose value is ADDR or the adddress itself if there is
   no symbol. */
static void
decode_possible_symbol (bfd_vma addr, struct disassemble_info *info)
{
  if (!info->symbol_at_address_func (addr, info))
    {
      (*info->fprintf_func) (info->stream, "%" BFD_VMA_FMT "d", addr);
    }
  else
    {
      asymbol *sym = NULL;
      int j;
      for (j = 0; j < info->symtab_size; ++j)
	{
	  sym = info->symtab[j];
	  if (bfd_asymbol_value (sym) == addr)
	    {
	      break;
	    }
	}
      if (j < info->symtab_size)
	(*info->fprintf_func) (info->stream, "%s", bfd_asymbol_name (sym));
    }
}

static void ld_18bit_decode (bfd_vma memaddr, struct disassemble_info* info);

static void
ext24_decode (bfd_vma memaddr, struct disassemble_info* info)
{
  uint8_t buffer[3];
  int status = read_memory (memaddr, buffer, 3, info);
  if (status < 0)
    return;

  int i;
  uint32_t addr = 0;
  for (i = 0; i < 3; ++i)
    {
      addr <<= 8;
      addr |= buffer[i];
    }

  operand_separator (info);
  decode_possible_symbol (addr, info);
}


static uint32_t
decode_signed_value (bfd_vma memaddr, struct disassemble_info* info, short size)
{
  assert (size >0);
  assert (size <= 4);
  bfd_byte buffer[4];
  if (0 > read_memory (memaddr, buffer, size, info))
    {
      return 0;
    }

  int i;
  uint32_t value = 0;
  for (i = 0; i < size; ++i)
    {
      value |= buffer[i] << (8 * (size - i - 1));
    }

  if (buffer[0] & 0x80)
    {
      /* Deal with negative values */
      value -= 0x1UL << (size * 8);
    }
  return value;
}


static void
opr_decode (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte postbyte;
  int status = read_memory (memaddr, &postbyte, 1, info);
  if (status < 0)
    return;

  enum OPR_MODE mode = -1;
  size_t i;
  for (i = 0; i < sizeof (opr_pb) / sizeof (opr_pb[0]); ++i)
    {
      const struct opr_pb *pb = opr_pb + i;
      if ((postbyte & pb->mask) == pb->value)
	{
	  mode = pb->mode;
	  break;
	}
    }

  operand_separator (info);
  switch (mode)
    {
    case OPR_IMMe4:
      {
	int n;
	uint8_t x = (postbyte & 0x0F);
	if (x == 0)
	  n = -1;
	else
	  n = x;

	(*info->fprintf_func) (info->stream, "#%d", n);
	break;
      }
    case OPR_REG:
      {
	uint8_t x = (postbyte & 0x07);
	(*info->fprintf_func) (info->stream, "%s", registers[x].name);
	break;
      }
    case OPR_OFXYS:
      {
	const char *reg  = xys_from_postbyte (postbyte);
	(*info->fprintf_func) (info->stream, "(%d,%s)", postbyte & 0x0F, reg);
	break;
      }
    case OPR_REG_DIRECT:
      {
	(*info->fprintf_func) (info->stream, "(%s,%s)", registers[postbyte & 0x07].name,
			       xys_from_postbyte (postbyte));
	break;
      }
    case OPR_REG_INDIRECT:
      {
	(*info->fprintf_func) (info->stream, "[%s,%s]", registers[postbyte & 0x07].name,
			       (postbyte & 0x10) ? "y": "x");
	break;
      }

    case OPR_IDX_INDIRECT:
      {
	uint8_t x1;
	read_memory (memaddr + 1, &x1, 1, info);
	int idx = x1;

	if (postbyte & 0x01)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 8;
	  }

	(*info->fprintf_func) (info->stream, "[%d,%s]", idx,
			       xysp_from_postbyte (postbyte));
	break;
      }

    case OPR_IDX3_DIRECT:
      {
	uint8_t x[3];
	read_memory (memaddr + 1, x, 3, info);
	int idx = x[0] << 16 | x[1] << 8 | x[2];

	if (x[0] & 0x80)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 24;
	  }

	(*info->fprintf_func) (info->stream, "(%d,%s)", idx,
			       xysp_from_postbyte (postbyte));
	break;
      }

    case OPR_IDX3_DIRECT_REG:
      {
	uint8_t x[3];
	read_memory (memaddr + 1, x, 3, info);
	int idx = x[0] << 16 | x[1] << 8 | x[2];

	if (x[0] & 0x80)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 24;
	  }

	(*info->fprintf_func) (info->stream, "(%d,%s)", idx,
			       registers[postbyte & 0x07].name);
	break;
      }

    case OPR_IDX3_INDIRECT:
      {
	uint8_t x[3];
	read_memory (memaddr + 1, x, 3, info);
	int idx = x[0] << 16 | x[1] << 8 | x[2];

	if (x[0] & 0x80)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 24;
	  }

	(*info->fprintf_func) (info->stream, "[%d,%s]", idx,
			       xysp_from_postbyte (postbyte));
	break;
      }

    case OPR_IDX_DIRECT:
      {
	uint8_t x1;
	read_memory (memaddr + 1, &x1, 1, info);
	int idx = x1;

	if (postbyte & 0x01)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 8;
	  }

	(*info->fprintf_func) (info->stream, "(%d,%s)", idx,
			       xysp_from_postbyte (postbyte));
	break;
      }

    case OPR_IDX2_REG:
      {
	uint8_t x[2];
	read_memory (memaddr + 1, x, 2, info);
	uint32_t offset = x[1] | x[0] << 8 ;
	offset |= (postbyte & 0x30) << 12;

	(*info->fprintf_func) (info->stream, "(%d,%s)", offset,
			       registers[postbyte & 0x07].name);
	break;
      }

    case OPR_XY_PRE_INC:
      {
	(*info->fprintf_func) (info->stream, "(+%s)",
			       (postbyte & 0x10) ? "y": "x");

	break;
      }
    case OPR_XY_POST_INC:
      {
	(*info->fprintf_func) (info->stream, "(%s+)",
			       (postbyte & 0x10) ? "y": "x");

	break;
      }
    case OPR_XY_PRE_DEC:
      {
	(*info->fprintf_func) (info->stream, "(-%s)",
			       (postbyte & 0x10) ? "y": "x");

	break;
      }
    case OPR_XY_POST_DEC:
      {
	(*info->fprintf_func) (info->stream, "(%s-)",
			       (postbyte & 0x10) ? "y": "x");

	break;
      }
    case OPR_S_PRE_DEC:
      {
	(*info->fprintf_func) (info->stream, "(-s)");
	break;
      }
    case OPR_S_POST_INC:
      {
	(*info->fprintf_func) (info->stream, "(s+)");
	break;
      }

    case OPR_EXT18:
      {
	const size_t size = 2;
	bfd_byte buffer[4];
	status = read_memory (memaddr + 1, buffer, size, info);
	if (status < 0)
	  return;

	uint32_t ext18 = 0;
	for (i = 0; i < size; ++i)
	  {
	    ext18 <<= 8;
	    ext18 |= buffer[i];
	  }

	ext18 |= (postbyte & 0x01) << 16;
	ext18 |= (postbyte & 0x04) << 15;

	decode_possible_symbol (ext18, info);
	break;
      }

    case OPR_EXT1:
      {
	uint8_t x1 = 0;
	read_memory (memaddr + 1, &x1, 1, info);
	int16_t addr;
	addr = x1;
	addr |= (postbyte & 0x3f) << 8;

	decode_possible_symbol (addr, info);
	break;
      }

    case OPR_EXT3_DIRECT:
      {
	const size_t size = 3;
	bfd_byte buffer[4];
	status = read_memory (memaddr + 1, buffer, size, info);
	if (status < 0)
	  return;

	uint32_t ext24 = 0;
	for (i = 0; i < size; ++i)
	  {
	    ext24 |= buffer[i] << (8 * (size - i - 1));
	  }

	decode_possible_symbol (ext24, info);
	break;
      }

    case OPR_EXT3_INDIRECT:
      {
	const size_t size = 3;
	bfd_byte buffer[4];
	status = read_memory (memaddr + 1, buffer, size, info);
	if (status < 0)
	  return;

	uint32_t ext24 = 0;
	for (i = 0; i < size; ++i)
	  {
	    ext24 |= buffer[i] << (8 * (size - i - 1));
	  }

	(*info->fprintf_func) (info->stream, "[%d]", ext24);

	break;
      }

    default:
      (*info->fprintf_func) (info->stream, "Unknown OPR mode #0x%x (%d)", postbyte, mode);
    }
}


static void
opr_decode2 (bfd_vma memaddr, struct disassemble_info* info)
{
  int n = opr_n_bytes (memaddr, info);
  opr_decode (memaddr, info);
  opr_decode (memaddr + n, info);
}

static void
imm1234 (bfd_vma memaddr, struct disassemble_info* info, int base)
{
  bfd_byte opcode;
  int status = read_memory (memaddr - 1, &opcode, 1, info);
  if (status < 0)
    return;

  opcode -= base;

  int size = registers[opcode & 0xF].bytes;

  uint32_t imm = decode_signed_value (memaddr, info, size);
  operand_separator (info);
  (*info->fprintf_func) (info->stream, "#%d", imm);
}


/* Special case of LD and CMP with register S and IMM operand */
static void
reg_s_imm (bfd_vma memaddr, struct disassemble_info* info)
{
  operand_separator (info);
  (*info->fprintf_func) (info->stream, "s");

  uint32_t imm = decode_signed_value (memaddr, info, 3);
  operand_separator (info);
  (*info->fprintf_func) (info->stream, "#%d", imm);
}

/* Special case of LD, CMP and ST with register S and OPR operand */
static void
reg_s_opr (bfd_vma memaddr, struct disassemble_info* info)
{
  operand_separator (info);
  (*info->fprintf_func) (info->stream, "s");

  opr_decode (memaddr, info);
}

static void
imm1234_8base (bfd_vma memaddr, struct disassemble_info* info)
{
  imm1234 (memaddr, info, 8);
}

static void
imm1234_0base (bfd_vma memaddr, struct disassemble_info* info)
{
  imm1234 (memaddr, info, 0);
}

static void
tfr (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr, &byte, 1, info);
  if (status < 0)
    return;

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "%s, %s",
			 registers[byte >> 4].name,
			 registers[byte & 0xF].name);
}


static void
reg (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr - 1, &byte, 1, info);
  if (status < 0)
    return;

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "%s", registers[byte & 0x07].name);
}

static void
reg_xy (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr - 1, &byte, 1, info);
  if (status < 0)
    return;

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "%s", (byte & 0x01) ? "y" : "x");
}

static void
lea_reg_xys_opr (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr - 1, &byte, 1, info);
  if (status < 0)
    return;

  char *reg = NULL;
  switch (byte & 0x03)
    {
    case 0x00:
      reg = "x";
      break;
    case 0x01:
      reg = "y";
      break;
    case 0x02:
      reg = "s";
      break;
    }

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "%s", reg);
  opr_decode (memaddr, info);
}



static void
lea_reg_xys (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr - 1, &byte, 1, info);
  if (status < 0)
    return;

  char *reg = NULL;
  switch (byte & 0x03)
    {
    case 0x00:
      reg = "x";
      break;
    case 0x01:
      reg = "y";
      break;
    case 0x02:
      reg = "s";
      break;
    }

  status = read_memory (memaddr, &byte, 1, info);
  if (status < 0)
    return;

  int8_t v = byte;

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "%s, (%d,%s)", reg, v, reg);
}


/* PC Relative offsets of size 15 or 7 bits */
static void
rel_15_7 (bfd_vma memaddr, struct disassemble_info* info, int offset)
{
  bfd_byte upper;
  int status = read_memory (memaddr, &upper, 1, info);
  if (status < 0)
    return;

  bool rel_size = (upper & 0x80);

  int16_t addr = upper;
  if (rel_size)
    {
      /* 15 bits.  Get the next byte */
      bfd_byte lower;
      status = read_memory (memaddr + 1, &lower, 1, info);
      if (status < 0)
	return;

      addr <<= 8;
      addr |= lower;
      addr &= 0x7FFF;

      bool negative = (addr & 0x4000);
      addr &= 0x3FFF;
      if (negative)
	addr = addr - 0x4000;
    }
  else
    {
      /* 7 bits. */
      bool negative = (addr & 0x40);
      addr &= 0x3F;
      if (negative)
	addr = addr - 0x40;
    }

  operand_separator (info);
  if (!info->symbol_at_address_func (addr + memaddr - offset, info))
    {
      (*info->fprintf_func) (info->stream, "*%+d", addr);
    }
  else
    {
      asymbol *sym = NULL;
      int i;
      for (i = 0; i < info->symtab_size; ++i)
	{
	  sym = info->symtab[i];
	  if (bfd_asymbol_value (sym) == addr + memaddr - offset)
	    {
	      break;
	    }
	}
      if (i < info->symtab_size)
	(*info->fprintf_func) (info->stream, "%s", bfd_asymbol_name (sym));
    }
}


/* PC Relative offsets of size 15 or 7 bits */
static void
decode_rel_15_7 (bfd_vma memaddr, struct disassemble_info* info)
{
  rel_15_7 (memaddr, info, 1);
}

struct opcode
{
  const char *mnemonic;
  insn_bytes_f insn_bytes;
  operands_f operands;
  operands_f operands2;
};

static int shift_n_bytes (bfd_vma memaddr, struct disassemble_info* info);
static int mov_imm_opr_n_bytes (bfd_vma memaddr, struct disassemble_info* info);
static int loop_prim_n_bytes (bfd_vma memaddr, struct disassemble_info* info);
static void mov_imm_opr (bfd_vma memaddr, struct disassemble_info* info);
static void bm_rel_decode (bfd_vma memaddr, struct disassemble_info* info);
static int bm_rel_n_bytes (bfd_vma memaddr, struct disassemble_info* info);
static int mul_n_bytes (bfd_vma memaddr, struct disassemble_info* info);
static void mul_decode (bfd_vma memaddr, struct disassemble_info* info);
static int bm_n_bytes (bfd_vma memaddr, struct disassemble_info* info);
static void bm_decode (bfd_vma memaddr, struct disassemble_info* info);

static void
cmp_xy (bfd_vma memaddr ATTRIBUTE_UNUSED, struct disassemble_info* info)
{
  operand_separator (info);
  (*info->fprintf_func) (info->stream, "x, y");
}

static void
sub_d6_x_y (bfd_vma memaddr ATTRIBUTE_UNUSED, struct disassemble_info* info)
{
  operand_separator (info);
  (*info->fprintf_func) (info->stream, "d6, x, y");
}

static void
sub_d6_y_x (bfd_vma memaddr ATTRIBUTE_UNUSED, struct disassemble_info* info)
{
  operand_separator (info);
  (*info->fprintf_func) (info->stream, "d6, y, x");
}

static const char shift_size_table[] = {
  'b', 'w', 'p', 'l'
};

static const struct opcode page2[] =
  {
    [0x00] = {"ld",  opr_n_bytes_p1, 0, reg_s_opr},
    [0x01] = {"st",  opr_n_bytes_p1, 0, reg_s_opr},
    [0x02] = {"cmp", opr_n_bytes_p1, 0, reg_s_opr},
    [0x03] = {"ld",  four, 0, reg_s_imm},
    [0x04] = {"cmp", four, 0, reg_s_imm},
    [0x05] = {"stop", single, 0, 0},
    [0x06] = {"wai",  single, 0, 0},
    [0x07] = {"sys",  single, 0, 0},
    [0x08] = {NULL,  bfextins_n_bytes, 0, 0},  /* BFEXT / BFINS */
    [0x09] = {NULL,  bfextins_n_bytes, 0, 0},
    [0x0a] = {NULL,  bfextins_n_bytes, 0, 0},
    [0x0b] = {NULL,  bfextins_n_bytes, 0, 0},
    [0x0c] = {NULL,  bfextins_n_bytes, 0, 0},
    [0x0d] = {NULL,  bfextins_n_bytes, 0, 0},
    [0x0e] = {NULL,  bfextins_n_bytes, 0, 0},
    [0x0f] = {NULL,  bfextins_n_bytes, 0, 0},
    [0x10] = {"minu", opr_n_bytes_p1, reg, opr_decode},
    [0x11] = {"minu", opr_n_bytes_p1, reg, opr_decode},
    [0x12] = {"minu", opr_n_bytes_p1, reg, opr_decode},
    [0x13] = {"minu", opr_n_bytes_p1, reg, opr_decode},
    [0x14] = {"minu", opr_n_bytes_p1, reg, opr_decode},
    [0x15] = {"minu", opr_n_bytes_p1, reg, opr_decode},
    [0x16] = {"minu", opr_n_bytes_p1, reg, opr_decode},
    [0x17] = {"minu", opr_n_bytes_p1, reg, opr_decode},
    [0x18] = {"maxu", opr_n_bytes_p1, reg, opr_decode},
    [0x19] = {"maxu", opr_n_bytes_p1, reg, opr_decode},
    [0x1a] = {"maxu", opr_n_bytes_p1, reg, opr_decode},
    [0x1b] = {"maxu", opr_n_bytes_p1, reg, opr_decode},
    [0x1c] = {"maxu", opr_n_bytes_p1, reg, opr_decode},
    [0x1d] = {"maxu", opr_n_bytes_p1, reg, opr_decode},
    [0x1e] = {"maxu", opr_n_bytes_p1, reg, opr_decode},
    [0x1f] = {"maxu", opr_n_bytes_p1, reg, opr_decode},
    [0x20] = {"mins", opr_n_bytes_p1, reg, opr_decode},
    [0x21] = {"mins", opr_n_bytes_p1, reg, opr_decode},
    [0x22] = {"mins", opr_n_bytes_p1, reg, opr_decode},
    [0x23] = {"mins", opr_n_bytes_p1, reg, opr_decode},
    [0x24] = {"mins", opr_n_bytes_p1, reg, opr_decode},
    [0x25] = {"mins", opr_n_bytes_p1, reg, opr_decode},
    [0x26] = {"mins", opr_n_bytes_p1, reg, opr_decode},
    [0x27] = {"mins", opr_n_bytes_p1, reg, opr_decode},
    [0x28] = {"maxs", opr_n_bytes_p1, reg, opr_decode},
    [0x29] = {"maxs", opr_n_bytes_p1, reg, opr_decode},
    [0x2a] = {"maxs", opr_n_bytes_p1, reg, opr_decode},
    [0x2b] = {"maxs", opr_n_bytes_p1, reg, opr_decode},
    [0x2c] = {"maxs", opr_n_bytes_p1, reg, opr_decode},
    [0x2d] = {"maxs", opr_n_bytes_p1, reg, opr_decode},
    [0x2e] = {"maxs", opr_n_bytes_p1, reg, opr_decode},
    [0x2f] = {"maxs", opr_n_bytes_p1, reg, opr_decode},
    [0x30] = {"div", mul_n_bytes, mul_decode, 0},
    [0x31] = {"div", mul_n_bytes, mul_decode, 0},
    [0x32] = {"div", mul_n_bytes, mul_decode, 0},
    [0x33] = {"div", mul_n_bytes, mul_decode, 0},
    [0x34] = {"div", mul_n_bytes, mul_decode, 0},
    [0x35] = {"div", mul_n_bytes, mul_decode, 0},
    [0x36] = {"div", mul_n_bytes, mul_decode, 0},
    [0x37] = {"div", mul_n_bytes, mul_decode, 0},
    [0x38] = {"mod", mul_n_bytes, mul_decode, 0},
    [0x39] = {"mod", mul_n_bytes, mul_decode, 0},
    [0x3a] = {"mod", mul_n_bytes, mul_decode, 0},
    [0x3b] = {"mod", mul_n_bytes, mul_decode, 0},
    [0x3c] = {"mod", mul_n_bytes, mul_decode, 0},
    [0x3d] = {"mod", mul_n_bytes, mul_decode, 0},
    [0x3e] = {"mod", mul_n_bytes, mul_decode, 0},
    [0x3f] = {"mod", mul_n_bytes, mul_decode, 0},
    [0x40] = {"abs", single, reg, 0},
    [0x41] = {"abs", single, reg, 0},
    [0x42] = {"abs", single, reg, 0},
    [0x43] = {"abs", single, reg, 0},
    [0x44] = {"abs", single, reg, 0},
    [0x45] = {"abs", single, reg, 0},
    [0x46] = {"abs", single, reg, 0},
    [0x47] = {"abs", single, reg, 0},
    [0x48] = {"mac", mul_n_bytes, mul_decode, 0},
    [0x49] = {"mac", mul_n_bytes, mul_decode, 0},
    [0x4a] = {"mac", mul_n_bytes, mul_decode, 0},
    [0x4b] = {"mac", mul_n_bytes, mul_decode, 0},
    [0x4c] = {"mac", mul_n_bytes, mul_decode, 0},
    [0x4d] = {"mac", mul_n_bytes, mul_decode, 0},
    [0x4e] = {"mac", mul_n_bytes, mul_decode, 0},
    [0x4f] = {"mac", mul_n_bytes, mul_decode, 0},
    [0x50] = {"adc", three, reg, imm1234_0base},
    [0x51] = {"adc", three, reg, imm1234_0base},
    [0x52] = {"adc", three, reg, imm1234_0base},
    [0x53] = {"adc", three, reg, imm1234_0base},
    [0x54] = {"adc", two,   reg, imm1234_0base},
    [0x55] = {"adc", two,   reg, imm1234_0base},
    [0x56] = {"adc", five,  reg, imm1234_0base},
    [0x57] = {"adc", five,  reg, imm1234_0base},
    [0x58] = {"bit", three, reg, imm1234_8base},
    [0x59] = {"bit", three, reg, imm1234_8base},
    [0x5a] = {"bit", three, reg, imm1234_8base},
    [0x5b] = {"bit", three, reg, imm1234_8base},
    [0x5c] = {"bit", two,   reg, imm1234_8base},
    [0x5d] = {"bit", two,   reg, imm1234_8base},
    [0x5e] = {"bit", five,  reg, imm1234_8base},
    [0x5f] = {"bit", five,  reg, imm1234_8base},
    [0x60] = {"adc", opr_n_bytes_p1, reg, opr_decode},
    [0x61] = {"adc", opr_n_bytes_p1, reg, opr_decode},
    [0x62] = {"adc", opr_n_bytes_p1, reg, opr_decode},
    [0x63] = {"adc", opr_n_bytes_p1, reg, opr_decode},
    [0x64] = {"adc", opr_n_bytes_p1, reg, opr_decode},
    [0x65] = {"adc", opr_n_bytes_p1, reg, opr_decode},
    [0x66] = {"adc", opr_n_bytes_p1, reg, opr_decode},
    [0x67] = {"adc", opr_n_bytes_p1, reg, opr_decode},
    [0x68] = {"bit", opr_n_bytes_p1, reg, opr_decode},
    [0x69] = {"bit", opr_n_bytes_p1, reg, opr_decode},
    [0x6a] = {"bit", opr_n_bytes_p1, reg, opr_decode},
    [0x6b] = {"bit", opr_n_bytes_p1, reg, opr_decode},
    [0x6c] = {"bit", opr_n_bytes_p1, reg, opr_decode},
    [0x6d] = {"bit", opr_n_bytes_p1, reg, opr_decode},
    [0x6e] = {"bit", opr_n_bytes_p1, reg, opr_decode},
    [0x6f] = {"bit", opr_n_bytes_p1, reg, opr_decode},
    [0x70] = {"sbc", three, reg, imm1234_0base},
    [0x71] = {"sbc", three, reg, imm1234_0base},
    [0x72] = {"sbc", three, reg, imm1234_0base},
    [0x73] = {"sbc", three, reg, imm1234_0base},
    [0x74] = {"sbc", two,   reg, imm1234_0base},
    [0x75] = {"sbc", two,   reg, imm1234_0base},
    [0x76] = {"sbc", five,  reg, imm1234_0base},
    [0x77] = {"sbc", five,  reg, imm1234_0base},
    [0x78] = {"eor", three, reg, imm1234_8base},
    [0x79] = {"eor", three, reg, imm1234_8base},
    [0x7a] = {"eor", three, reg, imm1234_8base},
    [0x7b] = {"eor", three, reg, imm1234_8base},
    [0x7c] = {"eor", two,   reg, imm1234_8base},
    [0x7d] = {"eor", two,   reg, imm1234_8base},
    [0x7e] = {"eor", five,  reg, imm1234_8base},
    [0x7f] = {"eor", five,  reg, imm1234_8base},
    [0x80] = {"sbc", opr_n_bytes_p1, reg, opr_decode},
    [0x81] = {"sbc", opr_n_bytes_p1, reg, opr_decode},
    [0x82] = {"sbc", opr_n_bytes_p1, reg, opr_decode},
    [0x83] = {"sbc", opr_n_bytes_p1, reg, opr_decode},
    [0x84] = {"sbc", opr_n_bytes_p1, reg, opr_decode},
    [0x85] = {"sbc", opr_n_bytes_p1, reg, opr_decode},
    [0x86] = {"sbc", opr_n_bytes_p1, reg, opr_decode},
    [0x87] = {"sbc", opr_n_bytes_p1, reg, opr_decode},
    [0x88] = {"eor", opr_n_bytes_p1, reg,    opr_decode},
    [0x89] = {"eor", opr_n_bytes_p1, reg,    opr_decode},
    [0x8a] = {"eor", opr_n_bytes_p1, reg,    opr_decode},
    [0x8b] = {"eor", opr_n_bytes_p1, reg,    opr_decode},
    [0x8c] = {"eor", opr_n_bytes_p1, reg,    opr_decode},
    [0x8d] = {"eor", opr_n_bytes_p1, reg,    opr_decode},
    [0x8e] = {"eor", opr_n_bytes_p1, reg,    opr_decode},
    [0x8f] = {"eor", opr_n_bytes_p1, reg,    opr_decode},
    [0x90] = {"rti",  single, 0, 0},
    [0x91] = {"clb",   two, tfr, 0},
    [0x92] = {"trap",  single, trap_decode, 0},
    [0x93] = {"trap",  single, trap_decode, 0},
    [0x94] = {"trap",  single, trap_decode, 0},
    [0x95] = {"trap",  single, trap_decode, 0},
    [0x96] = {"trap",  single, trap_decode, 0},
    [0x97] = {"trap",  single, trap_decode, 0},
    [0x98] = {"trap",  single, trap_decode, 0},
    [0x99] = {"trap",  single, trap_decode, 0},
    [0x9a] = {"trap",  single, trap_decode, 0},
    [0x9b] = {"trap",  single, trap_decode, 0},
    [0x9c] = {"trap",  single, trap_decode, 0},
    [0x9d] = {"trap",  single, trap_decode, 0},
    [0x9e] = {"trap",  single, trap_decode, 0},
    [0x9f] = {"trap",  single, trap_decode, 0},
    [0xa0] = {"sat", single, reg, 0},
    [0xa1] = {"sat", single, reg, 0},
    [0xa2] = {"sat", single, reg, 0},
    [0xa3] = {"sat", single, reg, 0},
    [0xa4] = {"sat", single, reg, 0},
    [0xa5] = {"sat", single, reg, 0},
    [0xa6] = {"sat", single, reg, 0},
    [0xa7] = {"sat", single, reg, 0},
    [0xa8] = {"trap",  single, trap_decode, 0},
    [0xa9] = {"trap",  single, trap_decode, 0},
    [0xaa] = {"trap",  single, trap_decode, 0},
    [0xab] = {"trap",  single, trap_decode, 0},
    [0xac] = {"trap",  single, trap_decode, 0},
    [0xad] = {"trap",  single, trap_decode, 0},
    [0xae] = {"trap",  single, trap_decode, 0},
    [0xaf] = {"trap",  single, trap_decode, 0},
    [0xb0] = {"qmul", mul_n_bytes, mul_decode, 0},
    [0xb1] = {"qmul", mul_n_bytes, mul_decode, 0},
    [0xb2] = {"qmul", mul_n_bytes, mul_decode, 0},
    [0xb3] = {"qmul", mul_n_bytes, mul_decode, 0},
    [0xb4] = {"qmul", mul_n_bytes, mul_decode, 0},
    [0xb5] = {"qmul", mul_n_bytes, mul_decode, 0},
    [0xb6] = {"qmul", mul_n_bytes, mul_decode, 0},
    [0xb7] = {"qmul", mul_n_bytes, mul_decode, 0},
    [0xb8] = {"trap",  single, trap_decode, 0},
    [0xb9] = {"trap",  single, trap_decode, 0},
    [0xba] = {"trap",  single, trap_decode, 0},
    [0xbb] = {"trap",  single, trap_decode, 0},
    [0xbc] = {"trap",  single, trap_decode, 0},
    [0xbd] = {"trap",  single, trap_decode, 0},
    [0xbe] = {"trap",  single, trap_decode, 0},
    [0xbf] = {"trap",  single, trap_decode, 0},
    [0xc0] = {"trap",  single, trap_decode, 0},
    [0xc1] = {"trap",  single, trap_decode, 0},
    [0xc2] = {"trap",  single, trap_decode, 0},
    [0xc3] = {"trap",  single, trap_decode, 0},
    [0xc4] = {"trap",  single, trap_decode, 0},
    [0xc5] = {"trap",  single, trap_decode, 0},
    [0xc6] = {"trap",  single, trap_decode, 0},
    [0xc7] = {"trap",  single, trap_decode, 0},
    [0xc8] = {"trap",  single, trap_decode, 0},
    [0xc9] = {"trap",  single, trap_decode, 0},
    [0xca] = {"trap",  single, trap_decode, 0},
    [0xcb] = {"trap",  single, trap_decode, 0},
    [0xcc] = {"trap",  single, trap_decode, 0},
    [0xcd] = {"trap",  single, trap_decode, 0},
    [0xce] = {"trap",  single, trap_decode, 0},
    [0xcf] = {"trap",  single, trap_decode, 0},
    [0xd0] = {"trap",  single, trap_decode, 0},
    [0xd1] = {"trap",  single, trap_decode, 0},
    [0xd2] = {"trap",  single, trap_decode, 0},
    [0xd3] = {"trap",  single, trap_decode, 0},
    [0xd4] = {"trap",  single, trap_decode, 0},
    [0xd5] = {"trap",  single, trap_decode, 0},
    [0xd6] = {"trap",  single, trap_decode, 0},
    [0xd7] = {"trap",  single, trap_decode, 0},
    [0xd8] = {"trap",  single, trap_decode, 0},
    [0xd9] = {"trap",  single, trap_decode, 0},
    [0xda] = {"trap",  single, trap_decode, 0},
    [0xdb] = {"trap",  single, trap_decode, 0},
    [0xdc] = {"trap",  single, trap_decode, 0},
    [0xdd] = {"trap",  single, trap_decode, 0},
    [0xde] = {"trap",  single, trap_decode, 0},
    [0xdf] = {"trap",  single, trap_decode, 0},
    [0xe0] = {"trap",  single, trap_decode, 0},
    [0xe1] = {"trap",  single, trap_decode, 0},
    [0xe2] = {"trap",  single, trap_decode, 0},
    [0xe3] = {"trap",  single, trap_decode, 0},
    [0xe4] = {"trap",  single, trap_decode, 0},
    [0xe5] = {"trap",  single, trap_decode, 0},
    [0xe6] = {"trap",  single, trap_decode, 0},
    [0xe7] = {"trap",  single, trap_decode, 0},
    [0xe8] = {"trap",  single, trap_decode, 0},
    [0xe9] = {"trap",  single, trap_decode, 0},
    [0xea] = {"trap",  single, trap_decode, 0},
    [0xeb] = {"trap",  single, trap_decode, 0},
    [0xec] = {"trap",  single, trap_decode, 0},
    [0xed] = {"trap",  single, trap_decode, 0},
    [0xee] = {"trap",  single, trap_decode, 0},
    [0xef] = {"trap",  single, trap_decode, 0},
    [0xf0] = {"trap",  single, trap_decode, 0},
    [0xf1] = {"trap",  single, trap_decode, 0},
    [0xf2] = {"trap",  single, trap_decode, 0},
    [0xf3] = {"trap",  single, trap_decode, 0},
    [0xf4] = {"trap",  single, trap_decode, 0},
    [0xf5] = {"trap",  single, trap_decode, 0},
    [0xf6] = {"trap",  single, trap_decode, 0},
    [0xf7] = {"trap",  single, trap_decode, 0},
    [0xf8] = {"trap",  single, trap_decode, 0},
    [0xf9] = {"trap",  single, trap_decode, 0},
    [0xfa] = {"trap",  single, trap_decode, 0},
    [0xfb] = {"trap",  single, trap_decode, 0},
    [0xfc] = {"trap",  single, trap_decode, 0},
    [0xfd] = {"trap",  single, trap_decode, 0},
    [0xfe] = {"trap",  single, trap_decode, 0},
    [0xff] = {"trap",  single, trap_decode, 0},
  };

static const struct opcode page1[] =
  {
    [0x00] = {"bgnd", single, 0, 0},
    [0x01] = {"nop",  single, 0, 0},
    [0x02] = {"brclr", bm_rel_n_bytes, bm_rel_decode, 0},
    [0x03] = {"brset", bm_rel_n_bytes, bm_rel_decode, 0},
    [0x04] = {NULL,   two,    0, 0}, /* psh/pul */
    [0x05] = {"rts",  single, 0, 0},
    [0x06] = {"lea", opr_n_bytes_p1, reg, opr_decode},
    [0x07] = {"lea", opr_n_bytes_p1, reg, opr_decode},
    [0x08] = {"lea", opr_n_bytes_p1, lea_reg_xys_opr, 0},
    [0x09] = {"lea", opr_n_bytes_p1, lea_reg_xys_opr, 0},
    [0x0a] = {"lea", opr_n_bytes_p1, lea_reg_xys_opr, 0},
    [0x0b] = {NULL, loop_prim_n_bytes, 0, 0}, /* Loop primitives TBcc / DBcc */
    [0x0c] = {"mov.b", mov_imm_opr_n_bytes, mov_imm_opr, 0},
    [0x0d] = {"mov.w", mov_imm_opr_n_bytes, mov_imm_opr, 0},
    [0x0e] = {"mov.p", mov_imm_opr_n_bytes, mov_imm_opr, 0},
    [0x0f] = {"mov.l", mov_imm_opr_n_bytes, mov_imm_opr, 0},
    [0x10] = {NULL,   shift_n_bytes, 0, 0},  /* lsr/lsl/asl/asr/rol/ror */
    [0x11] = {NULL,   shift_n_bytes, 0, 0},
    [0x12] = {NULL,   shift_n_bytes, 0, 0},
    [0x13] = {NULL,   shift_n_bytes, 0, 0},
    [0x14] = {NULL,   shift_n_bytes, 0, 0},
    [0x15] = {NULL,   shift_n_bytes, 0, 0},
    [0x16] = {NULL,   shift_n_bytes, 0, 0},
    [0x17] = {NULL,   shift_n_bytes, 0, 0},
    [0x18] = {"lea",  two, lea_reg_xys, NULL},
    [0x19] = {"lea",  two, lea_reg_xys, NULL},
    [0x1a] = {"lea",  two, lea_reg_xys, NULL},
    /* 0x1b PG2 */
    [0x1c] = {"mov.b", opr_n_bytes2, 0, opr_decode2},
    [0x1d] = {"mov.w", opr_n_bytes2, 0, opr_decode2},
    [0x1e] = {"mov.p", opr_n_bytes2, 0, opr_decode2},
    [0x1f] = {"mov.l", opr_n_bytes2, 0, opr_decode2},
    [0x20] = {"bra",  pcrel_15bit, decode_rel_15_7, 0},
    [0x21] = {"bsr",  pcrel_15bit, decode_rel_15_7, 0},
    [0x22] = {"bhi",  pcrel_15bit, decode_rel_15_7, 0},
    [0x23] = {"bls",  pcrel_15bit, decode_rel_15_7, 0},
    [0x24] = {"bcc",  pcrel_15bit, decode_rel_15_7, 0},
    [0x25] = {"bcs",  pcrel_15bit, decode_rel_15_7, 0},
    [0x26] = {"bne",  pcrel_15bit, decode_rel_15_7, 0},
    [0x27] = {"beq",  pcrel_15bit, decode_rel_15_7, 0},
    [0x28] = {"bvc",  pcrel_15bit, decode_rel_15_7, 0},
    [0x29] = {"bvs",  pcrel_15bit, decode_rel_15_7, 0},
    [0x2a] = {"bpl",  pcrel_15bit, decode_rel_15_7, 0},
    [0x2b] = {"bmi",  pcrel_15bit, decode_rel_15_7, 0},
    [0x2c] = {"bge",  pcrel_15bit, decode_rel_15_7, 0},
    [0x2d] = {"blt",  pcrel_15bit, decode_rel_15_7, 0},
    [0x2e] = {"bgt",  pcrel_15bit, decode_rel_15_7, 0},
    [0x2f] = {"ble",  pcrel_15bit, decode_rel_15_7, 0},
    [0x30] = {"inc", single, reg, 0},
    [0x31] = {"inc", single, reg, 0},
    [0x32] = {"inc", single, reg, 0},
    [0x33] = {"inc", single, reg, 0},
    [0x34] = {"inc", single, reg, 0},
    [0x35] = {"inc", single, reg, 0},
    [0x36] = {"inc", single, reg, 0},
    [0x37] = {"inc", single, reg, 0},
    [0x38] = {"clr", single, reg, 0},
    [0x39] = {"clr", single, reg, 0},
    [0x3a] = {"clr", single, reg, 0},
    [0x3b] = {"clr", single, reg, 0},
    [0x3c] = {"clr", single, reg, 0},
    [0x3d] = {"clr", single, reg, 0},
    [0x3e] = {"clr", single, reg, 0},
    [0x3f] = {"clr", single, reg, 0},
    [0x40] = {"dec", single, reg, 0},
    [0x41] = {"dec", single, reg, 0},
    [0x42] = {"dec", single, reg, 0},
    [0x43] = {"dec", single, reg, 0},
    [0x44] = {"dec", single, reg, 0},
    [0x45] = {"dec", single, reg, 0},
    [0x46] = {"dec", single, reg, 0},
    [0x47] = {"dec", single, reg, 0},
    [0x48] = {"mul", mul_n_bytes, mul_decode, 0},
    [0x49] = {"mul", mul_n_bytes, mul_decode, 0},
    [0x4a] = {"mul", mul_n_bytes, mul_decode, 0},
    [0x4b] = {"mul", mul_n_bytes, mul_decode, 0},
    [0x4c] = {"mul", mul_n_bytes, mul_decode, 0},
    [0x4d] = {"mul", mul_n_bytes, mul_decode, 0},
    [0x4e] = {"mul", mul_n_bytes, mul_decode, 0},
    [0x4f] = {"mul", mul_n_bytes, mul_decode, 0},
    [0x50] = {"add", three, reg, imm1234_0base},
    [0x51] = {"add", three, reg, imm1234_0base},
    [0x52] = {"add", three, reg, imm1234_0base},
    [0x53] = {"add", three, reg, imm1234_0base},
    [0x54] = {"add", two,   reg, imm1234_0base},
    [0x55] = {"add", two,   reg, imm1234_0base},
    [0x56] = {"add", five,  reg, imm1234_0base},
    [0x57] = {"add", five,  reg, imm1234_0base},
    [0x58] = {"and", three, reg, imm1234_8base},
    [0x59] = {"and", three, reg, imm1234_8base},
    [0x5a] = {"and", three, reg, imm1234_8base},
    [0x5b] = {"and", three, reg, imm1234_8base},
    [0x5c] = {"and", two,   reg, imm1234_8base},
    [0x5d] = {"and", two,   reg, imm1234_8base},
    [0x5e] = {"and", five,  reg, imm1234_8base},
    [0x5f] = {"and", five,  reg, imm1234_8base},
    [0x60] = {"add", opr_n_bytes_p1, reg, opr_decode},
    [0x61] = {"add", opr_n_bytes_p1, reg, opr_decode},
    [0x62] = {"add", opr_n_bytes_p1, reg, opr_decode},
    [0x63] = {"add", opr_n_bytes_p1, reg, opr_decode},
    [0x64] = {"add", opr_n_bytes_p1, reg, opr_decode},
    [0x65] = {"add", opr_n_bytes_p1, reg, opr_decode},
    [0x66] = {"add", opr_n_bytes_p1, reg, opr_decode},
    [0x67] = {"add", opr_n_bytes_p1, reg, opr_decode},
    [0x68] = {"and", opr_n_bytes_p1, reg, opr_decode},
    [0x69] = {"and", opr_n_bytes_p1, reg, opr_decode},
    [0x6a] = {"and", opr_n_bytes_p1, reg, opr_decode},
    [0x6b] = {"and", opr_n_bytes_p1, reg, opr_decode},
    [0x6c] = {"and", opr_n_bytes_p1, reg, opr_decode},
    [0x6d] = {"and", opr_n_bytes_p1, reg, opr_decode},
    [0x6e] = {"and", opr_n_bytes_p1, reg, opr_decode},
    [0x6f] = {"and", opr_n_bytes_p1, reg, opr_decode},
    [0x70] = {"sub", three, reg, imm1234_0base},
    [0x71] = {"sub", three, reg, imm1234_0base},
    [0x72] = {"sub", three, reg, imm1234_0base},
    [0x73] = {"sub", three, reg, imm1234_0base},
    [0x74] = {"sub", two,   reg, imm1234_0base},
    [0x75] = {"sub", two,   reg, imm1234_0base},
    [0x76] = {"sub", five,  reg, imm1234_0base},
    [0x77] = {"sub", five,  reg, imm1234_0base},
    [0x78] = {"or", three, reg, imm1234_8base},
    [0x79] = {"or", three, reg, imm1234_8base},
    [0x7a] = {"or", three, reg, imm1234_8base},
    [0x7b] = {"or", three, reg, imm1234_8base},
    [0x7c] = {"or", two,   reg, imm1234_8base},
    [0x7d] = {"or", two,   reg, imm1234_8base},
    [0x7e] = {"or", five,  reg, imm1234_8base},
    [0x7f] = {"or", five,  reg, imm1234_8base},
    [0x80] = {"sub", opr_n_bytes_p1, reg,    opr_decode},
    [0x81] = {"sub", opr_n_bytes_p1, reg,    opr_decode},
    [0x82] = {"sub", opr_n_bytes_p1, reg,    opr_decode},
    [0x83] = {"sub", opr_n_bytes_p1, reg,    opr_decode},
    [0x84] = {"sub", opr_n_bytes_p1, reg,    opr_decode},
    [0x85] = {"sub", opr_n_bytes_p1, reg,    opr_decode},
    [0x86] = {"sub", opr_n_bytes_p1, reg,    opr_decode},
    [0x87] = {"sub", opr_n_bytes_p1, reg,    opr_decode},
    [0x88] = {"or", opr_n_bytes_p1, reg,    opr_decode},
    [0x89] = {"or", opr_n_bytes_p1, reg,    opr_decode},
    [0x8a] = {"or", opr_n_bytes_p1, reg,    opr_decode},
    [0x8b] = {"or", opr_n_bytes_p1, reg,    opr_decode},
    [0x8c] = {"or", opr_n_bytes_p1, reg,    opr_decode},
    [0x8d] = {"or", opr_n_bytes_p1, reg,    opr_decode},
    [0x8e] = {"or", opr_n_bytes_p1, reg,    opr_decode},
    [0x8f] = {"or", opr_n_bytes_p1, reg,    opr_decode},
    [0x90] = {"ld", three,  reg, imm1234_0base},
    [0x91] = {"ld", three,  reg, imm1234_0base},
    [0x92] = {"ld", three,  reg, imm1234_0base},
    [0x93] = {"ld", three,  reg, imm1234_0base},
    [0x94] = {"ld", two,    reg, imm1234_0base},
    [0x95] = {"ld", two,    reg, imm1234_0base},
    [0x96] = {"ld", five,   reg, imm1234_0base},
    [0x97] = {"ld", five,   reg, imm1234_0base},
    [0x98] = {"ld", four,   reg_xy, imm1234_0base},
    [0x99] = {"ld", four,   reg_xy, imm1234_0base},
    [0x9a] = {"clr", single, reg_xy, 0},
    [0x9b] = {"clr", single, reg_xy, 0},
    [0x9c] = {"inc.b", opr_n_bytes_p1, 0, opr_decode},
    [0x9d] = {"inc.w", opr_n_bytes_p1, 0, opr_decode},
    [0x9e] = {"tfr", two, tfr, NULL},
    [0x9f] = {"inc.l", opr_n_bytes_p1, 0, opr_decode},
    [0xa0] = {"ld", opr_n_bytes_p1, reg,    opr_decode},
    [0xa1] = {"ld", opr_n_bytes_p1, reg,    opr_decode},
    [0xa2] = {"ld", opr_n_bytes_p1, reg,    opr_decode},
    [0xa3] = {"ld", opr_n_bytes_p1, reg,    opr_decode},
    [0xa4] = {"ld", opr_n_bytes_p1, reg,    opr_decode},
    [0xa5] = {"ld", opr_n_bytes_p1, reg,    opr_decode},
    [0xa6] = {"ld", opr_n_bytes_p1, reg,    opr_decode},
    [0xa7] = {"ld", opr_n_bytes_p1, reg,    opr_decode},
    [0xa8] = {"ld", opr_n_bytes_p1, reg_xy, opr_decode},
    [0xa9] = {"ld", opr_n_bytes_p1, reg_xy, opr_decode},
    [0xaa] = {"jmp", opr_n_bytes_p1, opr_decode, 0},
    [0xab] = {"jsr", opr_n_bytes_p1, opr_decode, 0},
    [0xac] = {"dec.b", opr_n_bytes_p1, 0, opr_decode},
    [0xad] = {"dec.w", opr_n_bytes_p1, 0, opr_decode},
    [0xae] = {NULL,   two, 0, 0},  /* EXG / SEX */
    [0xaf] = {"dec.l", opr_n_bytes_p1, 0, opr_decode},
    [0xb0] = {"ld", four,  reg, ext24_decode},
    [0xb1] = {"ld", four,  reg, ext24_decode},
    [0xb2] = {"ld", four,  reg, ext24_decode},
    [0xb3] = {"ld", four,  reg, ext24_decode},
    [0xb4] = {"ld", four,  reg, ext24_decode},
    [0xb5] = {"ld", four,  reg, ext24_decode},
    [0xb6] = {"ld", four,  reg, ext24_decode},
    [0xb7] = {"ld", four,  reg, ext24_decode},
    [0xb8] = {"ld", four,  reg_xy, ext24_decode},
    [0xb9] = {"ld", four,  reg_xy, ext24_decode},
    [0xba] = {"jmp", four, ext24_decode, 0},
    [0xbb] = {"jsr", four, ext24_decode, 0},
    [0xbc] = {"clr.b", opr_n_bytes_p1, 0, opr_decode},
    [0xbd] = {"clr.w", opr_n_bytes_p1, 0, opr_decode},
    [0xbe] = {"clr.p", opr_n_bytes_p1, 0, opr_decode},
    [0xbf] = {"clr.l", opr_n_bytes_p1, 0, opr_decode},
    [0xc0] = {"st", opr_n_bytes_p1, reg,    opr_decode},
    [0xc1] = {"st", opr_n_bytes_p1, reg,    opr_decode},
    [0xc2] = {"st", opr_n_bytes_p1, reg,    opr_decode},
    [0xc3] = {"st", opr_n_bytes_p1, reg,    opr_decode},
    [0xc4] = {"st", opr_n_bytes_p1, reg,    opr_decode},
    [0xc5] = {"st", opr_n_bytes_p1, reg,    opr_decode},
    [0xc6] = {"st", opr_n_bytes_p1, reg,    opr_decode},
    [0xc7] = {"st", opr_n_bytes_p1, reg,    opr_decode},
    [0xc8] = {"st", opr_n_bytes_p1, reg_xy, opr_decode},
    [0xc9] = {"st", opr_n_bytes_p1, reg_xy, opr_decode},
    [0xca] = {"ld", three, reg_xy, ld_18bit_decode},
    [0xcb] = {"ld", three, reg_xy, ld_18bit_decode},
    [0xcc] = {"com.b", opr_n_bytes_p1, NULL, opr_decode},
    [0xcd] = {"com.w", opr_n_bytes_p1, NULL, opr_decode},
    [0xce] = {"andcc", two, imm1, 0},
    [0xcf] = {"com.l", opr_n_bytes_p1, NULL, opr_decode},
    [0xd0] = {"st", four,  reg, ext24_decode},
    [0xd1] = {"st", four,  reg, ext24_decode},
    [0xd2] = {"st", four,  reg, ext24_decode},
    [0xd3] = {"st", four,  reg, ext24_decode},
    [0xd4] = {"st", four,  reg, ext24_decode},
    [0xd5] = {"st", four,  reg, ext24_decode},
    [0xd6] = {"st", four,  reg, ext24_decode},
    [0xd7] = {"st", four,  reg, ext24_decode},
    [0xd8] = {"st", four,  reg_xy, ext24_decode},
    [0xd9] = {"st", four,  reg_xy, ext24_decode},
    [0xda] = {"ld", three, reg_xy, ld_18bit_decode},
    [0xdb] = {"ld", three, reg_xy, ld_18bit_decode},
    [0xdc] = {"neg.b", opr_n_bytes_p1, NULL, opr_decode},
    [0xdd] = {"neg.w", opr_n_bytes_p1, NULL, opr_decode},
    [0xde] = {"orcc",  two,  imm1, 0},
    [0xdf] = {"neg.l", opr_n_bytes_p1, NULL, opr_decode},
    [0xe0] = {"cmp", three,  reg, imm1234_0base},
    [0xe1] = {"cmp", three,  reg, imm1234_0base},
    [0xe2] = {"cmp", three,  reg, imm1234_0base},
    [0xe3] = {"cmp", three,  reg, imm1234_0base},
    [0xe4] = {"cmp", two,    reg, imm1234_0base},
    [0xe5] = {"cmp", two,    reg, imm1234_0base},
    [0xe6] = {"cmp", five,   reg, imm1234_0base},
    [0xe7] = {"cmp", five,   reg, imm1234_0base},
    [0xe8] = {"cmp", four,   reg_xy, imm1234_0base},
    [0xe9] = {"cmp", four,   reg_xy, imm1234_0base},
    [0xea] = {"ld", three, reg_xy, ld_18bit_decode},
    [0xeb] = {"ld", three, reg_xy, ld_18bit_decode},
    [0xec] = {"bclr", bm_n_bytes, bm_decode, 0},
    [0xed] = {"bset", bm_n_bytes, bm_decode, 0},
    [0xee] = {"btgl", bm_n_bytes, bm_decode, 0},
    [0xef] = {"!!invalid!!", NULL, NULL, NULL}, /* SPARE */
    [0xf0] = {"cmp", opr_n_bytes_p1, reg,    opr_decode},
    [0xf1] = {"cmp", opr_n_bytes_p1, reg,    opr_decode},
    [0xf2] = {"cmp", opr_n_bytes_p1, reg,    opr_decode},
    [0xf3] = {"cmp", opr_n_bytes_p1, reg,    opr_decode},
    [0xf4] = {"cmp", opr_n_bytes_p1, reg,    opr_decode},
    [0xf5] = {"cmp", opr_n_bytes_p1, reg,    opr_decode},
    [0xf6] = {"cmp", opr_n_bytes_p1, reg,    opr_decode},
    [0xf7] = {"cmp", opr_n_bytes_p1, reg,    opr_decode},
    [0xf8] = {"cmp", opr_n_bytes_p1, reg_xy, opr_decode},
    [0xf9] = {"cmp", opr_n_bytes_p1, reg_xy, opr_decode},
    [0xfa] = {"ld",  three, reg_xy, ld_18bit_decode},
    [0xfb] = {"ld",  three, reg_xy, ld_18bit_decode},
    [0xfc] = {"cmp", single, cmp_xy, 0},
    [0xfd] = {"sub", single, sub_d6_x_y, 0},
    [0xfe] = {"sub", single, sub_d6_y_x, 0},
    [0xff] = {"swi", single, 0, 0}
  };


static const char *oprregs1[] =
  {
    "d3", "d2", "d1", "d0", "ccl", "cch"
  };

static const char *oprregs2[] =
  {
    "y", "x", "d7", "d6", "d5", "d4"
  };




enum MUL_MODE
  {
    MUL_REG_REG,
    MUL_REG_OPR,
    MUL_REG_IMM,
    MUL_OPR_OPR
  };

struct mb
{
  uint8_t mask;
  uint8_t value;
  enum MUL_MODE mode;
};

static const struct mb mul_table[] = {
  {0x40, 0x00, MUL_REG_REG},

  {0x47, 0x40, MUL_REG_OPR},
  {0x47, 0x41, MUL_REG_OPR},
  {0x47, 0x43, MUL_REG_OPR},

  {0x47, 0x44, MUL_REG_IMM},
  {0x47, 0x45, MUL_REG_IMM},
  {0x47, 0x47, MUL_REG_IMM},

  {0x43, 0x42, MUL_OPR_OPR},
};

static void
mul_decode (bfd_vma memaddr, struct disassemble_info* info)
{
  uint8_t mb;
  int status = read_memory (memaddr, &mb, 1, info);
  if (status < 0)
    return;


  uint8_t byte;
  status = read_memory (memaddr - 1, &byte, 1, info);
  if (status < 0)
    return;

  (*info->fprintf_func) (info->stream, "%c", (mb & 0x80) ? 's' : 'u');

  enum MUL_MODE mode = -1;
  size_t i;
  for (i = 0; i < sizeof (mul_table) / sizeof (mul_table[0]); ++i)
    {
      const struct mb *mm = mul_table + i;
      if ((mb & mm->mask) == mm->value)
	{
	  mode = mm->mode;
	  break;
	}
    }

  switch (mode)
    {
    case MUL_REG_REG:
      break;
    case MUL_OPR_OPR:
      {
	int size1 = (mb & 0x30) >> 4;
	int size2 = (mb & 0x0c) >> 2;
	(*info->fprintf_func) (info->stream, ".%c%c",
			       shift_size_table [size1],
			       shift_size_table [size2]);
      }
      break;
    default:
      {
	int size = (mb & 0x3);
	(*info->fprintf_func) (info->stream, ".%c", shift_size_table [size]);
      }
      break;
    }

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "%s", registers[byte & 0x7].name);

  switch (mode)
    {
    case MUL_REG_REG:
    case MUL_REG_IMM:
    case MUL_REG_OPR:
      operand_separator (info);
      (*info->fprintf_func) (info->stream, "%s", registers[(mb & 0x38) >> 3].name);
      break;
    default:
      break;
    }

  switch (mode)
    {
    case MUL_REG_IMM:
      operand_separator (info);
      int size = (mb & 0x3);
      uint32_t imm = decode_signed_value (memaddr + 1, info, size + 1);
      (*info->fprintf_func) (info->stream, "#%d", imm);
      break;
    case MUL_REG_REG:
      operand_separator (info);
      (*info->fprintf_func) (info->stream, "%s", registers[mb & 0x07].name);
      break;
    case MUL_REG_OPR:
      opr_decode (memaddr + 1, info);
      break;
    case MUL_OPR_OPR:
      {
	int first = opr_n_bytes (memaddr + 1, info);
	opr_decode (memaddr + 1, info);
	opr_decode (memaddr + first + 1, info);
	break;
      }
    }
}


static int
mul_n_bytes (bfd_vma memaddr, struct disassemble_info* info)
{
  int nx = 2;
  uint8_t mb;
  int status = read_memory (memaddr, &mb, 1, info);
  if (status < 0)
    return 0;

  enum MUL_MODE mode = -1;
  size_t i;
  for (i = 0; i < sizeof (mul_table) / sizeof (mul_table[0]); ++i)
    {
      const struct mb *mm = mul_table + i;
      if ((mb & mm->mask) == mm->value)
	{
	  mode = mm->mode;
	  break;
	}
    }

  int size = (mb & 0x3) + 1;

  switch (mode)
    {
    case MUL_REG_IMM:
      nx += size;
      break;
    case MUL_REG_REG:
      break;
    case MUL_REG_OPR:
      nx += opr_n_bytes (memaddr + 1, info);
      break;
    case MUL_OPR_OPR:
      {
	int first = opr_n_bytes (memaddr + nx - 1, info);
	nx += first;
	int second = opr_n_bytes (memaddr + nx - 1, info);
	nx += second;
      }
      break;
    }

  return nx;
}


enum BM_MODE {
  BM_REG_IMM,
  BM_RESERVED0,
  BM_OPR_B,
  BM_OPR_W,
  BM_OPR_L,
  BM_OPR_REG,
  BM_RESERVED1
};

struct bm
{
  uint8_t mask;
  uint8_t value;
  enum BM_MODE mode;
};

static const  struct bm bm_table[] = {
  { 0xC6, 0x04,     BM_REG_IMM},
  { 0x84, 0x00,     BM_REG_IMM},
  { 0x06, 0x06,     BM_REG_IMM},
  { 0xC6, 0x44,     BM_RESERVED0},
  // 00
  { 0x8F, 0x80,     BM_OPR_B},
  { 0x8E, 0x82,     BM_OPR_W},
  { 0x8C, 0x88,     BM_OPR_L},

  { 0x83, 0x81,     BM_OPR_REG},
  { 0x87, 0x84,     BM_RESERVED1},
};

static void
bm_decode (bfd_vma memaddr, struct disassemble_info* info)
{
  uint8_t bm;
  int status = read_memory (memaddr, &bm, 1, info);
  if (status < 0)
    return;

  size_t i;
  enum BM_MODE mode = -1;
  for (i = 0; i < sizeof (bm_table) / sizeof (bm_table[0]); ++i)
    {
      const struct bm *bme = bm_table + i;
      if ((bm & bme->mask) == bme->value)
	{
	  mode = bme->mode;
	  break;
	}
    }

  switch (mode)
    {
    case BM_REG_IMM:
      operand_separator (info);
      (*info->fprintf_func) (info->stream, "%s", registers[bm & 0x07].name);
      break;
    case BM_OPR_B:
      (*info->fprintf_func) (info->stream, ".%c", 'b');
      opr_decode (memaddr + 1, info);
      break;
    case BM_OPR_W:
      (*info->fprintf_func) (info->stream, ".%c", 'w');
      opr_decode (memaddr + 1, info);
      break;
    case BM_OPR_L:
      (*info->fprintf_func) (info->stream, ".%c", 'l');
      opr_decode (memaddr + 1, info);
      break;
    case BM_OPR_REG:
      {
	uint8_t xb;
	read_memory (memaddr + 1, &xb, 1, info);
	/* Don't emit a size suffix for register operands */
	if ((xb & 0xF8) != 0xB8)
	  (*info->fprintf_func) (info->stream, ".%c", shift_size_table[(bm & 0x0c) >> 2]);
	opr_decode (memaddr + 1, info);
      }
      break;
    case BM_RESERVED0:
    case BM_RESERVED1:
      assert (0);
      break;
    }

  uint8_t imm = 0;
  operand_separator (info);
  switch (mode)
    {
    case BM_REG_IMM:
      {
	imm = (bm & 0xF8) >> 3;
	(*info->fprintf_func) (info->stream, "#%d", imm);
      }
      break;
    case BM_OPR_L:
      imm |= (bm & 0x03) << 3;
      /* fallthrough */
    case BM_OPR_W:
      imm |= (bm & 0x01) << 3;
      /* fallthrough */
    case BM_OPR_B:
      imm |= (bm & 0x70) >> 4;
      (*info->fprintf_func) (info->stream, "#%d", imm);
      break;
    case BM_OPR_REG:
      (*info->fprintf_func) (info->stream, "%s", registers[(bm & 0x70) >> 4].name);
      break;
    case BM_RESERVED0:
    case BM_RESERVED1:
      assert (0);
      break;
    }
}


static void
bm_rel_decode (bfd_vma memaddr, struct disassemble_info* info)
{
  uint8_t bm;
  int status = read_memory (memaddr, &bm, 1, info);
  if (status < 0)
    return;

  size_t i;
  enum BM_MODE mode = -1;
  for (i = 0; i < sizeof (bm_table) / sizeof (bm_table[0]); ++i)
    {
      const struct bm *bme = bm_table + i;
      if ((bm & bme->mask) == bme->value)
	{
	  mode = bme->mode;
	  break;
	}
    }

  switch (mode)
    {
    case BM_REG_IMM:
      break;
    case BM_OPR_B:
      (*info->fprintf_func) (info->stream, ".%c", 'b');
      break;
    case BM_OPR_W:
      (*info->fprintf_func) (info->stream, ".%c", 'w');
      break;
    case BM_OPR_L:
      (*info->fprintf_func) (info->stream, ".%c", 'l');
      break;
    case BM_OPR_REG:
      {
	uint8_t xb;
	read_memory (memaddr + 1, &xb, 1, info);
	/* Don't emit a size suffix for register operands */
	if ((xb & 0xF8) != 0xB8)
	  (*info->fprintf_func) (info->stream, ".%c",
				 shift_size_table[(bm & 0x0C) >> 2]);
      }
      break;
    case BM_RESERVED0:
    case BM_RESERVED1:
      assert (0);
      break;
    }

  int n = 1;
  switch (mode)
    {
    case BM_REG_IMM:
      operand_separator (info);
      (*info->fprintf_func) (info->stream, "%s", registers[bm & 0x07].name);
      break;
    case BM_OPR_B:
    case BM_OPR_W:
    case BM_OPR_L:
      opr_decode (memaddr + 1, info);
      n = 1 + opr_n_bytes (memaddr + 1, info);
      break;
    case BM_OPR_REG:
      opr_decode (memaddr + 1, info);
      break;
    case BM_RESERVED0:
    case BM_RESERVED1:
      assert (0);
      break;
    }


  int imm = 0;
  operand_separator (info);
  switch (mode)
    {
    case BM_OPR_L:
      imm |= (bm & 0x02) << 3;
      /* fall through */
    case BM_OPR_W:
      imm |= (bm & 0x01) << 3;
      /* fall through */
    case BM_OPR_B:
      imm |= (bm & 0x70) >> 4;
      (*info->fprintf_func) (info->stream, "#%d", imm);
      break;
    case BM_REG_IMM:
      imm = (bm & 0xF8) >> 3;
      (*info->fprintf_func) (info->stream, "#%d", imm);
      break;
    case BM_RESERVED0:
    case BM_RESERVED1:
      assert (0);
      break;
    case BM_OPR_REG:
      (*info->fprintf_func) (info->stream, "%s", registers[(bm & 0x70) >> 4].name);
      n += opr_n_bytes (memaddr + 1, info);
      break;
    }

  rel_15_7 (memaddr + n, info, n + 1);
}

static int
bm_n_bytes (bfd_vma memaddr, struct disassemble_info* info)
{
  uint8_t bm;
  int status = read_memory (memaddr, &bm, 1, info);
  if (status < 0)
    return status;

  size_t i;
  enum BM_MODE mode = -1;
  for (i = 0; i < sizeof (bm_table) / sizeof (bm_table[0]); ++i)
    {
      const struct bm *bme = bm_table + i;
      if ((bm & bme->mask) == bme->value)
	{
	  mode = bme->mode;
	  break;
	}
    }

  int n = 2;
  switch (mode)
    {
    case BM_REG_IMM:
      break;

    case BM_OPR_B:
    case BM_OPR_W:
    case BM_OPR_L:
      n += opr_n_bytes (memaddr + 1, info);
      break;
    case BM_OPR_REG:
      n += opr_n_bytes (memaddr + 1, info);
      break;
    default:
      break;
  }

  return n;
}

static int
bm_rel_n_bytes (bfd_vma memaddr, struct disassemble_info* info)
{
  int n = 1 + bm_n_bytes (memaddr, info);

  bfd_byte rb;
  int status = read_memory (memaddr + n - 2, &rb, 1, info);
  if (status != 0)
    return status;

  if (rb & 0x80)
    n++;

  return n;
}





/* shift direction */
enum SB_DIR
  {
    SB_LEFT,
    SB_RIGHT
  };

enum SB_TYPE
  {
    SB_ARITHMETIC,
    SB_LOGICAL
  };


enum SB_MODE
  {
    SB_REG_REG_N_EFF,
    SB_REG_REG_N,
    SB_REG_OPR_EFF,
    SB_ROT,
    SB_REG_OPR_OPR,
    SB_OPR_N
  };

struct sb
{
  uint8_t mask;
  uint8_t value;
  enum SB_MODE mode;
};

static const  struct sb sb_table[] = {
  {0x30, 0x00,     SB_REG_REG_N_EFF},
  {0x30, 0x10,     SB_REG_REG_N},
  {0x34, 0x20,     SB_REG_OPR_EFF},
  {0x34, 0x24,     SB_ROT},
  {0x34, 0x30,     SB_REG_OPR_OPR},
  {0x34, 0x34,     SB_OPR_N},
};

static int
shift_n_bytes (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte sb;
  int status = read_memory (memaddr++, &sb, 1, info);
  if (status != 0)
    return status;

  size_t i;
  enum SB_MODE mode = -1;
  for (i = 0; i < sizeof (sb_table) / sizeof (sb_table[0]); ++i)
    {
      const struct sb *sbe = sb_table + i;
      if ((sb & sbe->mask) == sbe->value)
	mode = sbe->mode;
    }

  switch (mode)
    {
    case SB_REG_REG_N_EFF:
      return 2;
      break;
    case SB_REG_OPR_EFF:
    case SB_ROT:
	return 2 + opr_n_bytes (memaddr, info);
      break;
    case SB_REG_OPR_OPR:
      {
	int opr1 = opr_n_bytes (memaddr, info);
	int opr2 = 0;
	if ((sb & 0x30) != 0x20)
	  opr2 = opr_n_bytes (memaddr + opr1, info);
	return 2 + opr1 + opr2;
      }
      break;
    default:
      return 3;
    }

  /* not reached */
  return -1;
}


static int
mov_imm_opr_n_bytes (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr - 1, &byte, 1, info);
  if (status < 0)
    return status;

  int size = byte - 0x0c + 1;

  return size + opr_n_bytes (memaddr + size, info) + 1;
}

static void
mov_imm_opr (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr - 1, &byte, 1, info);
  if (status < 0)
    return ;

  int size = byte - 0x0c + 1;
  uint32_t imm = decode_signed_value (memaddr, info, size);

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "#%d", imm);
  opr_decode (memaddr + size, info);
}



static void
ld_18bit_decode (bfd_vma memaddr, struct disassemble_info* info)
{
  size_t size = 3;
  bfd_byte buffer[3];
  int status = read_memory (memaddr, buffer + 1, 2, info);
  if (status < 0)
    return ;


  status = read_memory (memaddr - 1, buffer, 1, info);
  if (status < 0)
    return ;

  buffer[0] = (buffer[0] & 0x30) >> 4;

  size_t i;
  uint32_t imm = 0;
  for (i = 0; i < size; ++i)
    {
      imm |= buffer[i] << (8 * (size - i - 1));
    }

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "#%d", imm);
}



/* Loop Primitives */

enum LP_MODE {
  LP_REG,
  LP_XY,
  LP_OPR
};

struct lp
{
  uint8_t mask;
  uint8_t value;
  enum LP_MODE mode;
};

static const struct lp lp_mode[] = {
  {0x08, 0x00, LP_REG},
  {0x0C, 0x08, LP_XY},
  {0x0C, 0x0C, LP_OPR},
};


static const char *lb_condition[] =
  {
    "ne", "eq", "pl", "mi", "gt", "le",
    "??", "??"
  };

static int
loop_prim_n_bytes (bfd_vma memaddr, struct disassemble_info* info)
{
  int mx = 0;
  uint8_t lb;
  read_memory (memaddr + mx++, &lb, 1, info);

  enum LP_MODE mode = -1;
  size_t i;
  for (i = 0; i < sizeof (lp_mode) / sizeof (lp_mode[0]); ++i)
    {
      const struct lp *pb = lp_mode + i;
      if ((lb & pb->mask) == pb->value)
	{
	  mode = pb->mode;
	  break;
	}
    }

  if (mode == LP_OPR)
    {
      mx += opr_n_bytes (memaddr + mx, info) ;
    }

  uint8_t rb;
  read_memory (memaddr + mx++, &rb, 1, info);
  if (rb & 0x80)
    mx++;

  return mx + 1;
}




static int
print_insn_exg_sex (bfd_vma memaddr, struct disassemble_info* info)
{
  uint8_t eb;
  int status = read_memory (memaddr, &eb, 1, info);
  if (status < 0)
    return -1;

  const struct reg *first =  &registers[(eb & 0xf0) >> 4];
  const struct reg *second = &registers[(eb & 0xf)];

  if (first->bytes < second->bytes)
    (*info->fprintf_func) (info->stream, "sex");
  else
    (*info->fprintf_func) (info->stream, "exg");

  operand_separator (info);
  (*info->fprintf_func) (info->stream, "%s", first->name);
  operand_separator (info);
  (*info->fprintf_func) (info->stream, "%s", second->name);
  return 0;
}



static int
print_insn_loop_primitive (bfd_vma memaddr, struct disassemble_info* info)
{
  int offs = 1;
  uint8_t lb;
  int status = read_memory (memaddr, &lb, 1, info);

  char mnemonic[7];
  int x = 0;
  mnemonic[x++] = (lb & 0x80) ? 'd' : 't';
  mnemonic[x++] = 'b';
  stpcpy (mnemonic + x, lb_condition [(lb & 0x70) >> 4]);
  x += 2;

  const char *reg  = NULL;
  enum LP_MODE mode = -1;
  size_t i;
  for (i = 0; i < sizeof (lp_mode) / sizeof (lp_mode[0]); ++i)
    {
      const struct lp *pb = lp_mode + i;
      if ((lb & pb->mask) == pb->value)
	{
	  mode = pb->mode;
	  break;
	}
    }

  switch (mode)
    {
    case LP_REG:
      reg = registers [lb & 0x07].name;
      break;
    case LP_XY:
      reg = (lb & 0x1) ? "y" : "x";
      break;
    case LP_OPR:
      mnemonic[x++] = '.';
      mnemonic[x++] = shift_size_table [lb & 0x03];
      offs += opr_n_bytes (memaddr + 1, info);
      break;
    }

  mnemonic[x++] = '\0';

  (*info->fprintf_func) (info->stream, "%s", mnemonic);

  if (mode == LP_OPR)
    opr_decode (memaddr + 1, info);
  else
    {
      operand_separator (info);
      (*info->fprintf_func) (info->stream, "%s", reg);
    }

  rel_15_7 (memaddr + offs, info, offs + 1);

  return status;
}


static int
print_insn_shift (bfd_vma memaddr, struct disassemble_info* info, uint8_t byte)
{
  size_t i;
  uint8_t sb;
  int status = read_memory (memaddr, &sb, 1, info);
  if (status < 0)
    return status;

  enum SB_DIR  dir = (sb & 0x40) ? SB_LEFT : SB_RIGHT;
  enum SB_TYPE type = (sb & 0x80) ? SB_ARITHMETIC : SB_LOGICAL;
  enum SB_MODE mode = -1;
  for (i = 0; i < sizeof (sb_table) / sizeof (sb_table[0]); ++i)
    {
      const struct sb *sbe = sb_table + i;
      if ((sb & sbe->mask) == sbe->value)
	mode = sbe->mode;
    }

  char mnemonic[6];
  int x = 0;
  if (mode == SB_ROT)
    {
      mnemonic[x++] = 'r';
      mnemonic[x++] = 'o';
    }
  else
    {
      mnemonic[x++] = (type == SB_LOGICAL) ? 'l' : 'a';
      mnemonic[x++] = 's';
    }

  mnemonic[x++] = (dir == SB_LEFT) ? 'l' : 'r';

  switch (mode)
    {
    case SB_REG_OPR_EFF:
    case SB_ROT:
    case SB_REG_OPR_OPR:
      mnemonic[x++] = '.';
      mnemonic[x++] = shift_size_table[sb & 0x03];
      break;
    case SB_OPR_N:
      {
	uint8_t xb;
	read_memory (memaddr + 1, &xb, 1, info);
	/* The size suffix is not printed if the OPR operand refers
	   directly to a register, because the size is implied by the
	   size of that register. */
	if ((xb & 0xF8) != 0xB8)
	  {
	    mnemonic[x++] = '.';
	    mnemonic[x++] = shift_size_table[sb & 0x03];
	  }
      }
      break;
    default:
      break;
    };

  mnemonic[x++] = '\0';

  (*info->fprintf_func) (info->stream, "%s", mnemonic);

  /* Destination register */
  switch (mode)
    {
    case SB_REG_REG_N_EFF:
    case SB_REG_REG_N:
    case SB_REG_OPR_EFF:
    case SB_REG_OPR_OPR:
      operand_separator (info);
      (*info->fprintf_func) (info->stream, "%s", registers[byte & 0x7].name);
      break;

    case SB_ROT:
      opr_decode (memaddr + 1, info);
      break;

    default:
      break;
    }

  /* Source register */
  switch (mode)
    {
    case SB_REG_REG_N_EFF:
    case SB_REG_REG_N:
      operand_separator (info);
      (*info->fprintf_func) (info->stream, "%s", registers[sb & 0x7].name);
      break;

    case SB_REG_OPR_OPR:
      opr_decode (memaddr + 1, info);
      break;

    default:
      break;
    }

  /* 3rd arg */
  switch (mode)
    {
    case SB_REG_OPR_EFF:
    case SB_OPR_N:
      opr_decode (memaddr + 1, info);
      break;

    case SB_REG_REG_N:
      if (sb & 0x08)
	{
	  operand_separator (info);
	  if (byte & 0x10)
	    {
	      uint8_t xb;
	      read_memory (memaddr + 1, &xb, 1, info);
	      int shift = ((sb & 0x08) >> 3) | ((xb & 0x0f) << 1);
	      (*info->fprintf_func) (info->stream, "#%d", shift);
	    }
	  else
	    {
	      (*info->fprintf_func) (info->stream, "%s:%d", __FILE__, __LINE__);
	    }
	}
      else
	{
	  opr_decode (memaddr + 1, info);
	}
      break;
    case SB_REG_OPR_OPR:
      {
      uint8_t xb;
      int n = opr_n_bytes (memaddr + 1, info);
      read_memory (memaddr + 1 + n, &xb, 1, info);

      if ((xb & 0xF0) == 0x70)
	{
	  int imm = xb & 0x0F;
	  imm <<= 1;
	  imm |= (sb & 0x08) >> 3;
	  operand_separator (info);
	  (*info->fprintf_func) (info->stream, "#%d", imm);
	}
      else
	{
	  opr_decode (memaddr + 1 + n, info);
	}
      }
      break;
    default:
      break;
    }

  switch (mode)
    {
    case SB_REG_REG_N_EFF:
    case SB_REG_OPR_EFF:
    case SB_OPR_N:
      operand_separator (info);
      (*info->fprintf_func) (info->stream, "#%d",
			     (sb & 0x08) ? 2 : 1);
      break;

    default:
      break;
    }

  return 0;
}

int
print_insn_s12z (bfd_vma memaddr, struct disassemble_info* info)
{
  bfd_byte byte;
  int status = read_memory (memaddr++, &byte, 1, info);
  if (status != 0)
    return status;

  const struct opcode *opc2 = NULL;
  const struct opcode *opc = page1 + byte;
  if (opc->mnemonic)
    {
      (*info->fprintf_func) (info->stream, "%s", opc->mnemonic);
    }
  else
    {
      /* The special cases ... */
      switch (byte)
	{
	case PAGE2_PREBYTE:
	  {
	    bfd_byte byte2;
	    read_memory (memaddr++, &byte2, 1, info);
	    opc2 = page2 + byte2;
	    if (opc2->mnemonic)
	      {
		(*info->fprintf_func) (info->stream, "%s", opc2->mnemonic);

		if (opc2->operands)
		  {
		    opc2->operands (memaddr, info);
		  }

		if (opc2->operands2)
		  {
		    opc2->operands2 (memaddr, info);
		  }
	      }
	    else if (byte2 >= 0x08 && byte2 <= 0x1F)
	      {
		bfd_byte bb;
		read_memory (memaddr, &bb, 1, info);
		if (bb & 0x80)
		  (*info->fprintf_func) (info->stream, "bfins");
		else
		  (*info->fprintf_func) (info->stream, "bfext");

		enum BB_MODE mode = -1;
		size_t i;
		const struct opr_bb *bbs = 0;
		for (i = 0; i < sizeof (bb_modes) / sizeof (bb_modes[0]); ++i)
		  {
		    bbs = bb_modes + i;
		    if ((bb & bbs->mask) == bbs->value)
		      {
			mode = bbs->mode;
			break;
		      }
		  }

		switch (mode)
		  {
		  case BB_REG_OPR_REG:
		  case BB_REG_OPR_IMM:
		  case BB_OPR_REG_REG:
		  case BB_OPR_REG_IMM:
		    {
		      int size = (bb >> 2) & 0x03;
		      (*info->fprintf_func) (info->stream, ".%c",
					     shift_size_table [size]);
		    }
		    break;
		  default:
		    break;
		  }

		int reg1 = byte2 & 0x07;
		/* First operand */
		switch (mode)
		  {
		  case BB_REG_REG_REG:
		  case BB_REG_REG_IMM:
		  case BB_REG_OPR_REG:
		  case BB_REG_OPR_IMM:
		    operand_separator (info);
		    (*info->fprintf_func) (info->stream, "%s",
					   registers[reg1].name);
		    break;
		  case BB_OPR_REG_REG:
		    opr_decode (memaddr + 1, info);
		    break;
		  case BB_OPR_REG_IMM:
		    opr_decode (memaddr + 2, info);
		    break;
		  }

		/* Second operand */
		switch (mode)
		  {
		  case BB_REG_REG_REG:
		  case BB_REG_REG_IMM:
		    {
		      int reg_src = (bb >> 2) & 0x07;
		      operand_separator (info);
		      (*info->fprintf_func) (info->stream, "%s",
					     registers[reg_src].name);
		    }
		    break;
		  case BB_OPR_REG_REG:
		  case BB_OPR_REG_IMM:
		    {
		      int reg_src = (byte2 & 0x07);
		      operand_separator (info);
		      (*info->fprintf_func) (info->stream, "%s",
					     registers[reg_src].name);
		    }
		    break;
		  case BB_REG_OPR_REG:
		    opr_decode (memaddr + 1, info);
		    break;
		  case BB_REG_OPR_IMM:
		    opr_decode (memaddr + 2, info);
		    break;
		  }

		/* Third operand */
		operand_separator (info);
		switch (mode)
		  {
		  case BB_REG_REG_REG:
		  case BB_OPR_REG_REG:
		  case BB_REG_OPR_REG:
		    {
		      int reg_parm = bb & 0x03;
		      (*info->fprintf_func) (info->stream, "%s",
					     registers[reg_parm].name);
		    }
		    break;
		  case BB_REG_REG_IMM:
		  case BB_OPR_REG_IMM:
		  case BB_REG_OPR_IMM:
		    {
		      bfd_byte i1;
		      read_memory (memaddr + 1, &i1, 1, info);
		      int offset = i1 & 0x1f;
		      int width = bb & 0x03;
		      width <<= 3;
		      width |= i1 >> 5;
		      (*info->fprintf_func) (info->stream, "#%d:%d", width,  offset);
		    }
		    break;
		  }
	      }
	  }
	  break;
	case 0xae: /* EXG / SEX */
	  status = print_insn_exg_sex (memaddr, info);
	  break;
	case 0x0b:  /* Loop Primitives TBcc and DBcc */
	  status = print_insn_loop_primitive (memaddr, info);
	  break;
	case 0x10:  	    /* shift */
	case 0x11:  	    /* shift */
	case 0x12:  	    /* shift */
	case 0x13:  	    /* shift */
	case 0x14:  	    /* shift */
	case 0x15:  	    /* shift */
	case 0x16:  	    /* shift */
	case 0x17:  	    /* shift */
	  status = print_insn_shift (memaddr, info, byte);
	  break;
	case 0x04:  	    /* psh / pul */
	  {
	    read_memory (memaddr, &byte, 1, info);
	    (*info->fprintf_func) (info->stream, (byte & 0x80) ? "pul" : "psh");
	    int bit;
	    if (byte & 0x40)
	      {
		if ((byte & 0x3F) == 0)
		  {
		    operand_separator (info);
		    (*info->fprintf_func) (info->stream, "%s", "ALL16b");
		  }
		else
		  for (bit = 5; bit >= 0; --bit)
		    {
		      if (byte & (0x1 << bit))
			{
			  operand_separator (info);
			  (*info->fprintf_func) (info->stream, "%s", oprregs2[bit]);
			}
		    }
	      }
	    else
	      {
		if ((byte & 0x3F) == 0)
		  {
		    operand_separator (info);
		    (*info->fprintf_func) (info->stream, "%s", "ALL");
		  }
		else
		  for (bit = 5; bit >= 0; --bit)
		    {
		      if (byte & (0x1 << bit))
			{
			  operand_separator (info);
			  (*info->fprintf_func) (info->stream, "%s", oprregs1[bit]);
			}
		    }
	      }
	  }
	  break;
	default:
	  operand_separator (info);
	  (*info->fprintf_func) (info->stream, "???");
	  break;
	}
    }

  if (opc2 == NULL)
    {
      if (opc->operands)
	{
	  opc->operands (memaddr, info);
	}

      if (opc->operands2)
	{
	  opc->operands2 (memaddr, info);
	}
    }

  int n = 0;

  /* Opcodes in page2 have an additional byte */
  if (opc2)
    n++;

  if (opc2 && opc2->insn_bytes == 0)
    return n;

  if (!opc2 && opc->insn_bytes == 0)
    return n;

  if (opc2)
    n += opc2->insn_bytes (memaddr, info);
  else
    n += opc->insn_bytes (memaddr, info);

  return n;
}
