/* s12z-decode.c -- Freescale S12Z disassembly
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

#include "opcode/s12z.h"

#include "bfd.h"

#include "s12z-opc.h"


typedef int (* insn_bytes_f) (struct mem_read_abstraction_base *);

typedef void (*operands_f) (struct mem_read_abstraction_base *,
			    int *n_operands, struct operand **operand);

typedef enum operator (*discriminator_f) (struct mem_read_abstraction_base *,
					  enum operator hint);

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
x_opr_n_bytes (struct mem_read_abstraction_base *mra, int offset)
{
  bfd_byte xb;
  int status = mra->read (mra, offset, 1, &xb);
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
opr_n_bytes_p1 (struct mem_read_abstraction_base *mra)
{
  return 1 + x_opr_n_bytes (mra, 0);
}

static int
opr_n_bytes2 (struct mem_read_abstraction_base *mra)
{
  int s = x_opr_n_bytes (mra, 0);
  s += x_opr_n_bytes (mra, s);
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
bfextins_n_bytes (struct mem_read_abstraction_base *mra)
{
  bfd_byte bb;
  int status = mra->read (mra, 0, 1, &bb);
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
    n += x_opr_n_bytes (mra, n - 1);

  return n;
}

static int
single (struct mem_read_abstraction_base *mra ATTRIBUTE_UNUSED)
{
  return 1;
}

static int
two (struct mem_read_abstraction_base *mra ATTRIBUTE_UNUSED)
{
  return 2;
}

static int
three (struct mem_read_abstraction_base *mra ATTRIBUTE_UNUSED)
{
  return 3;
}

static int
four (struct mem_read_abstraction_base *mra ATTRIBUTE_UNUSED)
{
  return 4;
}

static int
five (struct mem_read_abstraction_base *mra ATTRIBUTE_UNUSED)
{
  return 5;
}

static int
pcrel_15bit (struct mem_read_abstraction_base *mra)
{
  bfd_byte byte;
  int status = mra->read (mra, 0, 1, &byte);
  if (status < 0)
    return status;
  return (byte & 0x80) ? 3 : 2;
}



static int
xysp_reg_from_postbyte (uint8_t postbyte)
{
  int reg = -1;
  switch ((postbyte & 0x30) >> 4)
    {
    case 0:
      reg = REG_X;
      break;
    case 1:
      reg = REG_Y;
      break;
    case 2:
      reg = REG_S;
      break;
    default:
      reg = REG_P;
    }
  return reg;
}

static struct operand * create_immediate_operand (int value)
{
  struct immediate_operand *op = malloc (sizeof (*op));

  ((struct operand *)op)->cl = OPND_CL_IMMEDIATE;
  op->value = value;
  ((struct operand *)op)->osize = -1;

  return (struct operand *) op;
}

static struct operand * create_bitfield_operand (int width, int offset)
{
  struct bitfield_operand *op = malloc (sizeof (*op));

  ((struct operand *)op)->cl = OPND_CL_BIT_FIELD;
  op->width = width;
  op->offset = offset;
  ((struct operand *)op)->osize = -1;

  return (struct operand *) op;
}

static struct operand *
create_register_operand_with_size (int reg, short osize)
{
  struct register_operand *op = malloc (sizeof (*op));

  ((struct operand *)op)->cl = OPND_CL_REGISTER;
  op->reg = reg;
  ((struct operand *)op)->osize = osize;

  return (struct operand *) op;
}

static struct operand *
create_register_operand (int reg)
{
  return create_register_operand_with_size (reg, -1);
}

static struct operand * create_register_all_operand (void)
{
  struct register_operand *op = malloc (sizeof (*op));

  ((struct operand *)op)->cl = OPND_CL_REGISTER_ALL;
  ((struct operand *)op)->osize = -1;

  return (struct operand *) op;
}

static struct operand * create_register_all16_operand (void)
{
  struct register_operand *op = malloc (sizeof (*op));

  ((struct operand *)op)->cl = OPND_CL_REGISTER_ALL16;
  ((struct operand *)op)->osize = -1;

  return (struct operand *) op;
}


static struct operand *
create_simple_memory_operand (bfd_vma addr, bfd_vma base, bool relative)
{
  struct simple_memory_operand *op = malloc (sizeof (*op));

  ((struct operand *)op)->cl = OPND_CL_SIMPLE_MEMORY;
  op->addr = addr;
  op->base = base;
  op->relative = relative;
  ((struct operand *)op)->osize = -1;

  assert (relative || base == 0);

  return (struct operand *) op;
}

static struct operand *
create_memory_operand (bool indirect, int base, int n_regs, int reg0, int reg1)
{
  struct memory_operand *op = malloc (sizeof (*op));

  ((struct operand *)op)->cl = OPND_CL_MEMORY;
  op->indirect = indirect;
  op->base_offset = base;
  op->mutation = OPND_RM_NONE;
  op->n_regs = n_regs;
  op->regs[0] = reg0;
  op->regs[1] = reg1;
  ((struct operand *)op)->osize = -1;

  return (struct operand *) op;
}

static struct operand *
create_memory_auto_operand (enum op_reg_mutation mutation, int reg)
{
  struct memory_operand *op = malloc (sizeof (*op));

  ((struct operand *)op)->cl = OPND_CL_MEMORY;
  op->indirect = false;
  op->base_offset = 0;
  op->mutation = mutation;
  op->n_regs = 1;
  op->regs[0] = reg;
  op->regs[1] = -1;
  ((struct operand *)op)->osize = -1;

  return (struct operand *) op;
}



static void
z_ext24_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operand)
{
  uint8_t buffer[3];
  int status = mra->read (mra, 0, 3, buffer);
  if (status < 0)
    return;

  int i;
  uint32_t addr = 0;
  for (i = 0; i < 3; ++i)
    {
      addr <<= 8;
      addr |= buffer[i];
    }

  operand[(*n_operands)++] = create_simple_memory_operand (addr, 0, false);
}


static uint32_t
z_decode_signed_value (struct mem_read_abstraction_base *mra, int offset, short size)
{
  assert (size >0);
  assert (size <= 4);
  bfd_byte buffer[4];
  if (0 > mra->read (mra, offset, size, buffer))
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

static uint32_t
decode_signed_value (struct mem_read_abstraction_base *mra, short size)
{
  return z_decode_signed_value (mra, 0, size);
}

static void
x_imm1 (struct mem_read_abstraction_base *mra,
	int offset,
	int *n_operands, struct operand **operand)
{
  bfd_byte byte;
  int status = mra->read (mra, offset, 1, &byte);
  if (status < 0)
    return;

  operand[(*n_operands)++] = create_immediate_operand (byte);
}

/* An eight bit immediate operand.  */
static void
imm1_decode (struct mem_read_abstraction_base *mra,
	int *n_operands, struct operand **operand)
{
  x_imm1 (mra, 0, n_operands, operand);
}

static void
trap_decode (struct mem_read_abstraction_base *mra,
	     int *n_operands, struct operand **operand)
{
  x_imm1 (mra, -1, n_operands, operand);
}


static struct operand *
x_opr_decode_with_size (struct mem_read_abstraction_base *mra, int offset,
			short osize)
{
  bfd_byte postbyte;
  int status = mra->read (mra, offset, 1, &postbyte);
  if (status < 0)
    return NULL;
  offset++;

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

  struct operand *operand = NULL;
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

        operand = create_immediate_operand (n);
	break;
      }
    case OPR_REG:
      {
	uint8_t x = (postbyte & 0x07);
        operand = create_register_operand (x);
	break;
      }
    case OPR_OFXYS:
      {
        operand = create_memory_operand (false, postbyte & 0x0F, 1,
					 xysp_reg_from_postbyte (postbyte), -1);
	break;
      }
    case OPR_REG_DIRECT:
      {
        operand = create_memory_operand (false, 0, 2, postbyte & 0x07,
					 xysp_reg_from_postbyte (postbyte));
	break;
      }
    case OPR_REG_INDIRECT:
      {
        operand = create_memory_operand (true, 0, 2, postbyte & 0x07,
					 (postbyte & 0x10) ? REG_Y : REG_X);
	break;
      }

    case OPR_IDX_INDIRECT:
      {
	uint8_t x1;
	mra->read (mra, offset, 1, &x1);
	int idx = x1;

	if (postbyte & 0x01)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 8;
	  }

        operand = create_memory_operand (true, idx, 1,
					 xysp_reg_from_postbyte (postbyte), -1);
	break;
      }

    case OPR_IDX3_DIRECT:
      {
	uint8_t x[3];
	mra->read (mra, offset, 3, x);
	int idx = x[0] << 16 | x[1] << 8 | x[2];

	if (x[0] & 0x80)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 24;
	  }

        operand = create_memory_operand (false, idx, 1,
					 xysp_reg_from_postbyte (postbyte), -1);
	break;
      }

    case OPR_IDX3_DIRECT_REG:
      {
	uint8_t x[3];
	mra->read (mra, offset, 3, x);
	int idx = x[0] << 16 | x[1] << 8 | x[2];

	if (x[0] & 0x80)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 24;
	  }

        operand = create_memory_operand (false, idx, 1, postbyte & 0x07, -1);
	break;
      }

    case OPR_IDX3_INDIRECT:
      {
	uint8_t x[3];
	mra->read (mra, offset, 3, x);
	int idx = x[0] << 16 | x[1] << 8 | x[2];

	if (x[0] & 0x80)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 24;
	  }

	operand = create_memory_operand (true, idx, 1,
					 xysp_reg_from_postbyte (postbyte), -1);
	break;
      }

    case OPR_IDX_DIRECT:
      {
	uint8_t x1;
	mra->read (mra, offset, 1, &x1);
	int idx = x1;

	if (postbyte & 0x01)
	  {
	    /* Deal with negative values */
	    idx -= 0x1UL << 8;
	  }

        operand = create_memory_operand (false, idx, 1,
					 xysp_reg_from_postbyte (postbyte), -1);
	break;
      }

    case OPR_IDX2_REG:
      {
	uint8_t x[2];
	mra->read (mra, offset, 2, x);
	uint32_t idx = x[1] | x[0] << 8 ;
	idx |= (postbyte & 0x30) << 12;

        operand = create_memory_operand (false, idx, 1, postbyte & 0x07, -1);
	break;
      }

    case OPR_XY_PRE_INC:
      {
	operand = create_memory_auto_operand (OPND_RM_PRE_INC,
					      (postbyte & 0x10) ? REG_Y: REG_X);
	break;
      }
    case OPR_XY_POST_INC:
      {
	operand = create_memory_auto_operand (OPND_RM_POST_INC,
					      (postbyte & 0x10) ? REG_Y: REG_X);
	break;
      }
    case OPR_XY_PRE_DEC:
      {
	operand = create_memory_auto_operand (OPND_RM_PRE_DEC,
					      (postbyte & 0x10) ? REG_Y: REG_X);
	break;
      }
    case OPR_XY_POST_DEC:
      {
	operand = create_memory_auto_operand (OPND_RM_POST_DEC,
					      (postbyte & 0x10) ? REG_Y: REG_X);
	break;
      }
    case OPR_S_PRE_DEC:
      {
	operand = create_memory_auto_operand (OPND_RM_PRE_DEC, REG_S);
	break;
      }
    case OPR_S_POST_INC:
      {
	operand = create_memory_auto_operand (OPND_RM_POST_INC, REG_S);
	break;
      }

    case OPR_EXT18:
      {
	const size_t size = 2;
	bfd_byte buffer[4];
	status = mra->read (mra, offset, size, buffer);
	if (status < 0)
	  operand = NULL;

	uint32_t ext18 = 0;
	for (i = 0; i < size; ++i)
	  {
	    ext18 <<= 8;
	    ext18 |= buffer[i];
	  }

	ext18 |= (postbyte & 0x01) << 16;
	ext18 |= (postbyte & 0x04) << 15;

	operand = create_simple_memory_operand (ext18, 0, false);
	break;
      }

    case OPR_EXT1:
      {
	uint8_t x1 = 0;
	mra->read (mra, offset, 1, &x1);
	int16_t addr;
	addr = x1;
	addr |= (postbyte & 0x3f) << 8;

	operand = create_simple_memory_operand (addr, 0, false);
	break;
      }

    case OPR_EXT3_DIRECT:
      {
	const size_t size = 3;
	bfd_byte buffer[4];
	status = mra->read (mra, offset, size, buffer);
	if (status < 0)
	  operand = NULL;

	uint32_t ext24 = 0;
	for (i = 0; i < size; ++i)
	  {
	    ext24 |= buffer[i] << (8 * (size - i - 1));
	  }

	operand = create_simple_memory_operand (ext24, 0, false);
	break;
      }

    case OPR_EXT3_INDIRECT:
      {
	const size_t size = 3;
	bfd_byte buffer[4];
	status = mra->read (mra, offset, size, buffer);
	if (status < 0)
	  operand = NULL;

	uint32_t ext24 = 0;
	for (i = 0; i < size; ++i)
	  {
	    ext24 |= buffer[i] << (8 * (size - i - 1));
	  }

        operand = create_memory_operand (true, ext24, 0, -1, -1);
	break;
      }

    default:
      printf ("Unknown OPR mode #0x%x (%d)", postbyte, mode);
      abort ();
    }

  operand->osize = osize;

  return operand;
}

static struct operand *
x_opr_decode (struct mem_read_abstraction_base *mra, int offset)
{
  return x_opr_decode_with_size (mra, offset, -1);
}

static void
z_opr_decode (struct mem_read_abstraction_base *mra,
	      int *n_operands, struct operand **operand)
{
  operand[(*n_operands)++] = x_opr_decode (mra, 0);
}

static void
z_opr_decode2 (struct mem_read_abstraction_base *mra,
	       int *n_operands, struct operand **operand)
{
  int n = x_opr_n_bytes (mra, 0);

  operand[(*n_operands)++] = x_opr_decode (mra, 0);
  operand[(*n_operands)++] = x_opr_decode (mra, n);
}

static void
imm1234 (struct mem_read_abstraction_base *mra, int base,
	 int *n_operands, struct operand **operand)
{
  bfd_byte opcode;
  int status = mra->read (mra, -1, 1, &opcode);
  if (status < 0)
    return;

  opcode -= base;

  int size = registers[opcode & 0xF].bytes;

  uint32_t imm = decode_signed_value (mra, size);

  operand[(*n_operands)++] = create_immediate_operand (imm);
}


/* Special case of LD and CMP with register S and IMM operand */
static void
reg_s_imm (struct mem_read_abstraction_base *mra, int *n_operands,
	   struct operand **operand)
{
  operand[(*n_operands)++] = create_register_operand (REG_S);

  uint32_t imm = decode_signed_value (mra, 3);
  operand[(*n_operands)++] = create_immediate_operand (imm);
}

/* Special case of LD, CMP and ST with register S and OPR operand */
static void
reg_s_opr (struct mem_read_abstraction_base *mra, int *n_operands,
	   struct operand **operand)
{
  operand[(*n_operands)++] = create_register_operand (REG_S);
  operand[(*n_operands)++] = x_opr_decode (mra, 0);
}

static void
z_imm1234_8base (struct mem_read_abstraction_base *mra, int *n_operands,
		 struct operand **operand)
{
  imm1234 (mra, 8, n_operands, operand);
}

static void
z_imm1234_0base (struct mem_read_abstraction_base *mra, int *n_operands,
		 struct operand **operand)
{
  imm1234 (mra, 0, n_operands, operand);
}


static void
z_tfr (struct mem_read_abstraction_base *mra, int *n_operands,
       struct operand **operand)
{
  bfd_byte byte;
  int status = mra->read (mra, 0, 1, &byte);
  if (status < 0)
    return;

  operand[(*n_operands)++] = create_register_operand (byte >> 4);
  operand[(*n_operands)++] = create_register_operand (byte & 0x0F);
}

static void
z_reg (struct mem_read_abstraction_base *mra, int *n_operands,
       struct operand **operand)
{
  bfd_byte byte;
  int status = mra->read (mra, -1, 1, &byte);
  if (status < 0)
    return;

  operand[(*n_operands)++] = create_register_operand (byte & 0x07);
}


static void
reg_xy (struct mem_read_abstraction_base *mra,
	int *n_operands, struct operand **operand)
{
  bfd_byte byte;
  int status = mra->read (mra, -1, 1, &byte);
  if (status < 0)
    return;

  operand[(*n_operands)++] =
    create_register_operand ((byte & 0x01) ? REG_Y : REG_X);
}

static void
lea_reg_xys_opr (struct mem_read_abstraction_base *mra,
		 int *n_operands, struct operand **operand)
{
  bfd_byte byte;
  int status = mra->read (mra, -1, 1, &byte);
  if (status < 0)
    return;

  int reg_xys = -1;
  switch (byte & 0x03)
    {
    case 0x00:
      reg_xys = REG_X;
      break;
    case 0x01:
      reg_xys = REG_Y;
      break;
    case 0x02:
      reg_xys = REG_S;
      break;
    }

  operand[(*n_operands)++] = create_register_operand (reg_xys);
  operand[(*n_operands)++] = x_opr_decode (mra, 0);
}

static void
lea_reg_xys (struct mem_read_abstraction_base *mra,
	     int *n_operands, struct operand **operand)
{
  bfd_byte byte;
  int status = mra->read (mra, -1, 1, &byte);
  if (status < 0)
    return;

  int reg_n = -1;
  switch (byte & 0x03)
    {
    case 0x00:
      reg_n = REG_X;
      break;
    case 0x01:
      reg_n = REG_Y;
      break;
    case 0x02:
      reg_n = REG_S;
      break;
    }

  status = mra->read (mra, 0, 1, &byte);
  if (status < 0)
    return;

  operand[(*n_operands)++] = create_register_operand (reg_n);
  operand[(*n_operands)++] = create_memory_operand (false, (int8_t) byte,
						    1, reg_n, -1);
}


/* PC Relative offsets of size 15 or 7 bits */
static void
rel_15_7 (struct mem_read_abstraction_base *mra, int offset,
	  int *n_operands, struct operand **operands)
{
  bfd_byte upper;
  int status = mra->read (mra, offset - 1, 1, &upper);
  if (status < 0)
    return;

  bool rel_size = (upper & 0x80);

  int16_t addr = upper;
  if (rel_size)
    {
      /* 15 bits.  Get the next byte */
      bfd_byte lower;
      status = mra->read (mra, offset, 1, &lower);
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

  operands[(*n_operands)++] =
    create_simple_memory_operand (addr, mra->posn (mra) - 1, true);
}


/* PC Relative offsets of size 15 or 7 bits */
static void
decode_rel_15_7 (struct mem_read_abstraction_base *mra,
		 int *n_operands, struct operand **operand)
{
  rel_15_7 (mra, 1, n_operands, operand);
}

static int shift_n_bytes (struct mem_read_abstraction_base *);
static int mov_imm_opr_n_bytes (struct mem_read_abstraction_base *);
static int loop_prim_n_bytes (struct mem_read_abstraction_base *);
static int bm_rel_n_bytes (struct mem_read_abstraction_base *);
static int mul_n_bytes (struct mem_read_abstraction_base *);
static int bm_n_bytes (struct mem_read_abstraction_base *);

static void psh_pul_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operand);
static void shift_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operand);
static void mul_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operand);
static void bm_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operand);
static void bm_rel_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operand);
static void mov_imm_opr (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operand);
static void loop_primitive_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operands);
static void bit_field_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operands);
static void exg_sex_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operands);


static enum operator shift_discrim (struct mem_read_abstraction_base *mra, enum operator hint);
static enum operator psh_pul_discrim (struct mem_read_abstraction_base *mra, enum operator hint);
static enum operator mul_discrim (struct mem_read_abstraction_base *mra, enum operator hint);
static enum operator loop_primitive_discrim (struct mem_read_abstraction_base *mra, enum operator hint);
static enum operator bit_field_discrim (struct mem_read_abstraction_base *mra, enum operator hint);
static enum operator exg_sex_discrim (struct mem_read_abstraction_base *mra, enum operator hint);


static void
cmp_xy (struct mem_read_abstraction_base *mra ATTRIBUTE_UNUSED,
	int *n_operands, struct operand **operand)
{
  operand[(*n_operands)++] = create_register_operand (REG_X);
  operand[(*n_operands)++] = create_register_operand (REG_Y);
}

static void
sub_d6_x_y (struct mem_read_abstraction_base *mra ATTRIBUTE_UNUSED,
	    int *n_operands, struct operand **operand)
{
  operand[(*n_operands)++] = create_register_operand (REG_D6);
  operand[(*n_operands)++] = create_register_operand (REG_X);
  operand[(*n_operands)++] = create_register_operand (REG_Y);
}

static void
sub_d6_y_x (struct mem_read_abstraction_base *mra ATTRIBUTE_UNUSED,
	    int *n_operands, struct operand **operand)
{
  operand[(*n_operands)++] = create_register_operand (REG_D6);
  operand[(*n_operands)++] = create_register_operand (REG_Y);
  operand[(*n_operands)++] = create_register_operand (REG_X);
}

static void ld_18bit_decode (struct mem_read_abstraction_base *mra, int *n_operands, struct operand **operand);

static enum operator
mul_discrim (struct mem_read_abstraction_base *mra, enum operator hint)
{
  uint8_t mb;
  int status = mra->read (mra, 0, 1, &mb);
  if (status < 0)
    return OP_INVALID;

  bool signed_op = (mb & 0x80);

  switch (hint)
    {
    case OPBASE_mul:
      return signed_op ? OP_muls : OP_mulu;
      break;
    case OPBASE_div:
      return signed_op ? OP_divs : OP_divu;
      break;
    case OPBASE_mod:
      return signed_op ? OP_mods : OP_modu;
      break;
    case OPBASE_mac:
      return signed_op ? OP_macs : OP_macu;
      break;
    case OPBASE_qmul:
      return signed_op ? OP_qmuls : OP_qmulu;
      break;
    default:
      abort ();
    }

  return OP_INVALID;
}

struct opcode
{
  /* The operation that this opcode performs.  */
  enum operator operator;

  /* The size of this operation.  May be -1 if it is implied
     in the operands or if size is not applicable.  */
  short osize;

  /* Some operations need this function to work out which operation
   is intended.  */
  discriminator_f discriminator;

  /* A function returning the number of bytes in this instruction.  */
  insn_bytes_f insn_bytes;

  operands_f operands;
  operands_f operands2;
};

static const struct opcode page2[] =
  {
    [0x00] = {OP_ld, -1, 0,  opr_n_bytes_p1, reg_s_opr, 0},
    [0x01] = {OP_st, -1, 0,  opr_n_bytes_p1, reg_s_opr, 0},
    [0x02] = {OP_cmp, -1, 0, opr_n_bytes_p1, reg_s_opr, 0},
    [0x03] = {OP_ld, -1, 0,  four, reg_s_imm, 0},
    [0x04] = {OP_cmp, -1, 0, four, reg_s_imm, 0},
    [0x05] = {OP_stop, -1, 0, single, 0, 0},
    [0x06] = {OP_wai, -1, 0,  single, 0, 0},
    [0x07] = {OP_sys, -1, 0,  single, 0, 0},
    [0x08] = {0xFFFF, -1, bit_field_discrim,  bfextins_n_bytes, bit_field_decode, 0},  /* BFEXT / BFINS */
    [0x09] = {0xFFFF, -1, bit_field_discrim,  bfextins_n_bytes, bit_field_decode, 0},
    [0x0a] = {0xFFFF, -1, bit_field_discrim,  bfextins_n_bytes, bit_field_decode, 0},
    [0x0b] = {0xFFFF, -1, bit_field_discrim,  bfextins_n_bytes, bit_field_decode, 0},
    [0x0c] = {0xFFFF, -1, bit_field_discrim,  bfextins_n_bytes, bit_field_decode, 0},
    [0x0d] = {0xFFFF, -1, bit_field_discrim,  bfextins_n_bytes, bit_field_decode, 0},
    [0x0e] = {0xFFFF, -1, bit_field_discrim,  bfextins_n_bytes, bit_field_decode, 0},
    [0x0f] = {0xFFFF, -1, bit_field_discrim,  bfextins_n_bytes, bit_field_decode, 0},
    [0x10] = {OP_minu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x11] = {OP_minu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x12] = {OP_minu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x13] = {OP_minu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x14] = {OP_minu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x15] = {OP_minu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x16] = {OP_minu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x17] = {OP_minu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x18] = {OP_maxu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x19] = {OP_maxu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x1a] = {OP_maxu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x1b] = {OP_maxu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x1c] = {OP_maxu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x1d] = {OP_maxu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x1e] = {OP_maxu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x1f] = {OP_maxu, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x20] = {OP_mins, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x21] = {OP_mins, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x22] = {OP_mins, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x23] = {OP_mins, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x24] = {OP_mins, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x25] = {OP_mins, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x26] = {OP_mins, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x27] = {OP_mins, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x28] = {OP_maxs, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x29] = {OP_maxs, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x2a] = {OP_maxs, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x2b] = {OP_maxs, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x2c] = {OP_maxs, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x2d] = {OP_maxs, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x2e] = {OP_maxs, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x2f] = {OP_maxs, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x30] = {OPBASE_div, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x31] = {OPBASE_div, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x32] = {OPBASE_div, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x33] = {OPBASE_div, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x34] = {OPBASE_div, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x35] = {OPBASE_div, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x36] = {OPBASE_div, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x37] = {OPBASE_div, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x38] = {OPBASE_mod, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x39] = {OPBASE_mod, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x3a] = {OPBASE_mod, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x3b] = {OPBASE_mod, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x3c] = {OPBASE_mod, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x3d] = {OPBASE_mod, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x3e] = {OPBASE_mod, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x3f] = {OPBASE_mod, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x40] = {OP_abs, -1, 0, single, z_reg, 0},
    [0x41] = {OP_abs, -1, 0, single, z_reg, 0},
    [0x42] = {OP_abs, -1, 0, single, z_reg, 0},
    [0x43] = {OP_abs, -1, 0, single, z_reg, 0},
    [0x44] = {OP_abs, -1, 0, single, z_reg, 0},
    [0x45] = {OP_abs, -1, 0, single, z_reg, 0},
    [0x46] = {OP_abs, -1, 0, single, z_reg, 0},
    [0x47] = {OP_abs, -1, 0, single, z_reg, 0},
    [0x48] = {OPBASE_mac, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x49] = {OPBASE_mac, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4a] = {OPBASE_mac, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4b] = {OPBASE_mac, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4c] = {OPBASE_mac, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4d] = {OPBASE_mac, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4e] = {OPBASE_mac, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4f] = {OPBASE_mac, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x50] = {OP_adc, -1, 0, three, z_reg, z_imm1234_0base},
    [0x51] = {OP_adc, -1, 0, three, z_reg, z_imm1234_0base},
    [0x52] = {OP_adc, -1, 0, three, z_reg, z_imm1234_0base},
    [0x53] = {OP_adc, -1, 0, three, z_reg, z_imm1234_0base},
    [0x54] = {OP_adc, -1, 0, two,   z_reg, z_imm1234_0base},
    [0x55] = {OP_adc, -1, 0, two,   z_reg, z_imm1234_0base},
    [0x56] = {OP_adc, -1, 0, five,  z_reg, z_imm1234_0base},
    [0x57] = {OP_adc, -1, 0, five,  z_reg, z_imm1234_0base},
    [0x58] = {OP_bit, -1, 0, three, z_reg, z_imm1234_8base},
    [0x59] = {OP_bit, -1, 0, three, z_reg, z_imm1234_8base},
    [0x5a] = {OP_bit, -1, 0, three, z_reg, z_imm1234_8base},
    [0x5b] = {OP_bit, -1, 0, three, z_reg, z_imm1234_8base},
    [0x5c] = {OP_bit, -1, 0, two,   z_reg, z_imm1234_8base},
    [0x5d] = {OP_bit, -1, 0, two,   z_reg, z_imm1234_8base},
    [0x5e] = {OP_bit, -1, 0, five,  z_reg, z_imm1234_8base},
    [0x5f] = {OP_bit, -1, 0, five,  z_reg, z_imm1234_8base},
    [0x60] = {OP_adc, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x61] = {OP_adc, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x62] = {OP_adc, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x63] = {OP_adc, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x64] = {OP_adc, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x65] = {OP_adc, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x66] = {OP_adc, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x67] = {OP_adc, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x68] = {OP_bit, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x69] = {OP_bit, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6a] = {OP_bit, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6b] = {OP_bit, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6c] = {OP_bit, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6d] = {OP_bit, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6e] = {OP_bit, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6f] = {OP_bit, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x70] = {OP_sbc, -1, 0, three, z_reg, z_imm1234_0base},
    [0x71] = {OP_sbc, -1, 0, three, z_reg, z_imm1234_0base},
    [0x72] = {OP_sbc, -1, 0, three, z_reg, z_imm1234_0base},
    [0x73] = {OP_sbc, -1, 0, three, z_reg, z_imm1234_0base},
    [0x74] = {OP_sbc, -1, 0, two,   z_reg, z_imm1234_0base},
    [0x75] = {OP_sbc, -1, 0, two,   z_reg, z_imm1234_0base},
    [0x76] = {OP_sbc, -1, 0, five,  z_reg, z_imm1234_0base},
    [0x77] = {OP_sbc, -1, 0, five,  z_reg, z_imm1234_0base},
    [0x78] = {OP_eor, -1, 0, three, z_reg, z_imm1234_8base},
    [0x79] = {OP_eor, -1, 0, three, z_reg, z_imm1234_8base},
    [0x7a] = {OP_eor, -1, 0, three, z_reg, z_imm1234_8base},
    [0x7b] = {OP_eor, -1, 0, three, z_reg, z_imm1234_8base},
    [0x7c] = {OP_eor, -1, 0, two,   z_reg, z_imm1234_8base},
    [0x7d] = {OP_eor, -1, 0, two,   z_reg, z_imm1234_8base},
    [0x7e] = {OP_eor, -1, 0, five,  z_reg, z_imm1234_8base},
    [0x7f] = {OP_eor, -1, 0, five,  z_reg, z_imm1234_8base},
    [0x80] = {OP_sbc, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x81] = {OP_sbc, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x82] = {OP_sbc, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x83] = {OP_sbc, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x84] = {OP_sbc, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x85] = {OP_sbc, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x86] = {OP_sbc, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x87] = {OP_sbc, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x88] = {OP_eor, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x89] = {OP_eor, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x8a] = {OP_eor, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x8b] = {OP_eor, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x8c] = {OP_eor, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x8d] = {OP_eor, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x8e] = {OP_eor, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x8f] = {OP_eor, -1, 0,  opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x90] = {OP_rti, -1, 0,  single, 0, 0},
    [0x91] = {OP_clb, -1, 0,   two, z_tfr, 0},
    [0x92] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x93] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x94] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x95] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x96] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x97] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x98] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x99] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x9a] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x9b] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x9c] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x9d] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x9e] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0x9f] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xa0] = {OP_sat, -1, 0, single, z_reg, 0},
    [0xa1] = {OP_sat, -1, 0, single, z_reg, 0},
    [0xa2] = {OP_sat, -1, 0, single, z_reg, 0},
    [0xa3] = {OP_sat, -1, 0, single, z_reg, 0},
    [0xa4] = {OP_sat, -1, 0, single, z_reg, 0},
    [0xa5] = {OP_sat, -1, 0, single, z_reg, 0},
    [0xa6] = {OP_sat, -1, 0, single, z_reg, 0},
    [0xa7] = {OP_sat, -1, 0, single, z_reg, 0},
    [0xa8] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xa9] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xaa] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xab] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xac] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xad] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xae] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xaf] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xb0] = {OPBASE_qmul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0xb1] = {OPBASE_qmul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0xb2] = {OPBASE_qmul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0xb3] = {OPBASE_qmul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0xb4] = {OPBASE_qmul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0xb5] = {OPBASE_qmul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0xb6] = {OPBASE_qmul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0xb7] = {OPBASE_qmul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0xb8] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xb9] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xba] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xbb] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xbc] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xbd] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xbe] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xbf] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc0] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc1] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc2] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc3] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc4] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc5] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc6] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc7] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc8] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xc9] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xca] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xcb] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xcc] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xcd] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xce] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xcf] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd0] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd1] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd2] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd3] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd4] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd5] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd6] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd7] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd8] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xd9] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xda] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xdb] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xdc] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xdd] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xde] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xdf] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe0] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe1] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe2] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe3] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe4] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe5] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe6] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe7] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe8] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xe9] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xea] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xeb] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xec] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xed] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xee] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xef] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf0] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf1] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf2] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf3] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf4] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf5] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf6] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf7] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf8] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xf9] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xfa] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xfb] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xfc] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xfd] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xfe] = {OP_trap, -1, 0,  single, trap_decode, 0},
    [0xff] = {OP_trap, -1, 0,  single, trap_decode, 0},
  };

static const struct opcode page1[] =
  {
    [0x00] = {OP_bgnd, -1, 0, single, 0, 0},
    [0x01] = {OP_nop, -1, 0,  single, 0, 0},
    [0x02] = {OP_brclr, -1, 0, bm_rel_n_bytes, bm_rel_decode, 0},
    [0x03] = {OP_brset, -1, 0, bm_rel_n_bytes, bm_rel_decode, 0},
    [0x04] = {0xFFFF, -1, psh_pul_discrim,   two, psh_pul_decode, 0}, /* psh/pul */
    [0x05] = {OP_rts, -1, 0,  single, 0, 0},
    [0x06] = {OP_lea, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x07] = {OP_lea, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x08] = {OP_lea, -1, 0, opr_n_bytes_p1, lea_reg_xys_opr, 0},
    [0x09] = {OP_lea, -1, 0, opr_n_bytes_p1, lea_reg_xys_opr, 0},
    [0x0a] = {OP_lea, -1, 0, opr_n_bytes_p1, lea_reg_xys_opr, 0},
    [0x0b] = {0xFFFF, -1, loop_primitive_discrim, loop_prim_n_bytes, loop_primitive_decode, 0}, /* Loop primitives TBcc / DBcc */
    [0x0c] = {OP_mov, 0, 0, mov_imm_opr_n_bytes, mov_imm_opr, 0},
    [0x0d] = {OP_mov, 1, 0, mov_imm_opr_n_bytes, mov_imm_opr, 0},
    [0x0e] = {OP_mov, 2, 0, mov_imm_opr_n_bytes, mov_imm_opr, 0},
    [0x0f] = {OP_mov, 3, 0, mov_imm_opr_n_bytes, mov_imm_opr, 0},
    [0x10] = {0xFFFF, -1, shift_discrim,  shift_n_bytes, shift_decode, 0},  /* lsr/lsl/asl/asr/rol/ror */
    [0x11] = {0xFFFF, -1, shift_discrim,  shift_n_bytes, shift_decode, 0},
    [0x12] = {0xFFFF, -1, shift_discrim,  shift_n_bytes, shift_decode, 0},
    [0x13] = {0xFFFF, -1, shift_discrim,  shift_n_bytes, shift_decode, 0},
    [0x14] = {0xFFFF, -1, shift_discrim,  shift_n_bytes, shift_decode, 0},
    [0x15] = {0xFFFF, -1, shift_discrim,  shift_n_bytes, shift_decode, 0},
    [0x16] = {0xFFFF, -1, shift_discrim,  shift_n_bytes, shift_decode, 0},
    [0x17] = {0xFFFF, -1, shift_discrim,  shift_n_bytes, shift_decode, 0},
    [0x18] = {OP_lea, -1, 0,  two, lea_reg_xys, NULL},
    [0x19] = {OP_lea, -1, 0,  two, lea_reg_xys, NULL},
    [0x1a] = {OP_lea, -1, 0,  two, lea_reg_xys, NULL},
    /* 0x1b PG2 */
    [0x1c] = {OP_mov, 0, 0, opr_n_bytes2, z_opr_decode2, 0},
    [0x1d] = {OP_mov, 1, 0, opr_n_bytes2, z_opr_decode2, 0},
    [0x1e] = {OP_mov, 2, 0, opr_n_bytes2, z_opr_decode2, 0},
    [0x1f] = {OP_mov, 3, 0, opr_n_bytes2, z_opr_decode2, 0},
    [0x20] = {OP_bra, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x21] = {OP_bsr, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x22] = {OP_bhi, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x23] = {OP_bls, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x24] = {OP_bcc, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x25] = {OP_bcs, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x26] = {OP_bne, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x27] = {OP_beq, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x28] = {OP_bvc, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x29] = {OP_bvs, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x2a] = {OP_bpl, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x2b] = {OP_bmi, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x2c] = {OP_bge, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x2d] = {OP_blt, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x2e] = {OP_bgt, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x2f] = {OP_ble, -1, 0,  pcrel_15bit, decode_rel_15_7, 0},
    [0x30] = {OP_inc, -1, 0, single, z_reg, 0},
    [0x31] = {OP_inc, -1, 0, single, z_reg, 0},
    [0x32] = {OP_inc, -1, 0, single, z_reg, 0},
    [0x33] = {OP_inc, -1, 0, single, z_reg, 0},
    [0x34] = {OP_inc, -1, 0, single, z_reg, 0},
    [0x35] = {OP_inc, -1, 0, single, z_reg, 0},
    [0x36] = {OP_inc, -1, 0, single, z_reg, 0},
    [0x37] = {OP_inc, -1, 0, single, z_reg, 0},
    [0x38] = {OP_clr, -1, 0, single, z_reg, 0},
    [0x39] = {OP_clr, -1, 0, single, z_reg, 0},
    [0x3a] = {OP_clr, -1, 0, single, z_reg, 0},
    [0x3b] = {OP_clr, -1, 0, single, z_reg, 0},
    [0x3c] = {OP_clr, -1, 0, single, z_reg, 0},
    [0x3d] = {OP_clr, -1, 0, single, z_reg, 0},
    [0x3e] = {OP_clr, -1, 0, single, z_reg, 0},
    [0x3f] = {OP_clr, -1, 0, single, z_reg, 0},
    [0x40] = {OP_dec, -1, 0, single, z_reg, 0},
    [0x41] = {OP_dec, -1, 0, single, z_reg, 0},
    [0x42] = {OP_dec, -1, 0, single, z_reg, 0},
    [0x43] = {OP_dec, -1, 0, single, z_reg, 0},
    [0x44] = {OP_dec, -1, 0, single, z_reg, 0},
    [0x45] = {OP_dec, -1, 0, single, z_reg, 0},
    [0x46] = {OP_dec, -1, 0, single, z_reg, 0},
    [0x47] = {OP_dec, -1, 0, single, z_reg, 0},
    [0x48] = {OPBASE_mul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x49] = {OPBASE_mul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4a] = {OPBASE_mul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4b] = {OPBASE_mul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4c] = {OPBASE_mul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4d] = {OPBASE_mul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4e] = {OPBASE_mul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x4f] = {OPBASE_mul, -1, mul_discrim, mul_n_bytes, mul_decode, 0},
    [0x50] = {OP_add, -1, 0, three, z_reg, z_imm1234_0base},
    [0x51] = {OP_add, -1, 0, three, z_reg, z_imm1234_0base},
    [0x52] = {OP_add, -1, 0, three, z_reg, z_imm1234_0base},
    [0x53] = {OP_add, -1, 0, three, z_reg, z_imm1234_0base},
    [0x54] = {OP_add, -1, 0, two,   z_reg, z_imm1234_0base},
    [0x55] = {OP_add, -1, 0, two,   z_reg, z_imm1234_0base},
    [0x56] = {OP_add, -1, 0, five,  z_reg, z_imm1234_0base},
    [0x57] = {OP_add, -1, 0, five,  z_reg, z_imm1234_0base},
    [0x58] = {OP_and, -1, 0, three, z_reg, z_imm1234_8base},
    [0x59] = {OP_and, -1, 0, three, z_reg, z_imm1234_8base},
    [0x5a] = {OP_and, -1, 0, three, z_reg, z_imm1234_8base},
    [0x5b] = {OP_and, -1, 0, three, z_reg, z_imm1234_8base},
    [0x5c] = {OP_and, -1, 0, two,   z_reg, z_imm1234_8base},
    [0x5d] = {OP_and, -1, 0, two,   z_reg, z_imm1234_8base},
    [0x5e] = {OP_and, -1, 0, five,  z_reg, z_imm1234_8base},
    [0x5f] = {OP_and, -1, 0, five,  z_reg, z_imm1234_8base},
    [0x60] = {OP_add, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x61] = {OP_add, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x62] = {OP_add, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x63] = {OP_add, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x64] = {OP_add, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x65] = {OP_add, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x66] = {OP_add, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x67] = {OP_add, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x68] = {OP_and, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x69] = {OP_and, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6a] = {OP_and, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6b] = {OP_and, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6c] = {OP_and, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6d] = {OP_and, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6e] = {OP_and, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x6f] = {OP_and, -1, 0, opr_n_bytes_p1, z_reg, z_opr_decode},
    [0x70] = {OP_sub, -1, 0, three, z_reg, z_imm1234_0base},
    [0x71] = {OP_sub, -1, 0, three, z_reg, z_imm1234_0base},
    [0x72] = {OP_sub, -1, 0, three, z_reg, z_imm1234_0base},
    [0x73] = {OP_sub, -1, 0, three, z_reg, z_imm1234_0base},
    [0x74] = {OP_sub, -1, 0, two,   z_reg, z_imm1234_0base},
    [0x75] = {OP_sub, -1, 0, two,   z_reg, z_imm1234_0base},
    [0x76] = {OP_sub, -1, 0, five,  z_reg, z_imm1234_0base},
    [0x77] = {OP_sub, -1, 0, five,  z_reg, z_imm1234_0base},
    [0x78] = {OP_or, -1, 0, three, z_reg, z_imm1234_8base},
    [0x79] = {OP_or, -1, 0, three, z_reg, z_imm1234_8base},
    [0x7a] = {OP_or, -1, 0, three, z_reg, z_imm1234_8base},
    [0x7b] = {OP_or, -1, 0, three, z_reg, z_imm1234_8base},
    [0x7c] = {OP_or, -1, 0, two,   z_reg, z_imm1234_8base},
    [0x7d] = {OP_or, -1, 0, two,   z_reg, z_imm1234_8base},
    [0x7e] = {OP_or, -1, 0, five,  z_reg, z_imm1234_8base},
    [0x7f] = {OP_or, -1, 0, five,  z_reg, z_imm1234_8base},
    [0x80] = {OP_sub, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x81] = {OP_sub, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x82] = {OP_sub, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x83] = {OP_sub, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x84] = {OP_sub, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x85] = {OP_sub, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x86] = {OP_sub, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x87] = {OP_sub, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x88] = {OP_or, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x89] = {OP_or, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x8a] = {OP_or, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x8b] = {OP_or, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x8c] = {OP_or, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x8d] = {OP_or, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x8e] = {OP_or, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x8f] = {OP_or, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0x90] = {OP_ld, -1, 0, three,  z_reg, z_imm1234_0base},
    [0x91] = {OP_ld, -1, 0, three,  z_reg, z_imm1234_0base},
    [0x92] = {OP_ld, -1, 0, three,  z_reg, z_imm1234_0base},
    [0x93] = {OP_ld, -1, 0, three,  z_reg, z_imm1234_0base},
    [0x94] = {OP_ld, -1, 0, two,    z_reg, z_imm1234_0base},
    [0x95] = {OP_ld, -1, 0, two,    z_reg, z_imm1234_0base},
    [0x96] = {OP_ld, -1, 0, five,   z_reg, z_imm1234_0base},
    [0x97] = {OP_ld, -1, 0, five,   z_reg, z_imm1234_0base},
    [0x98] = {OP_ld, -1, 0, four,   reg_xy, z_imm1234_0base},
    [0x99] = {OP_ld, -1, 0, four,   reg_xy, z_imm1234_0base},
    [0x9a] = {OP_clr, -1, 0, single, reg_xy, 0},
    [0x9b] = {OP_clr, -1, 0, single, reg_xy, 0},
    [0x9c] = {OP_inc, 0, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0x9d] = {OP_inc, 1, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0x9e] = {OP_tfr, -1, 0, two, z_tfr, NULL},
    [0x9f] = {OP_inc, 3, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xa0] = {OP_ld, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xa1] = {OP_ld, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xa2] = {OP_ld, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xa3] = {OP_ld, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xa4] = {OP_ld, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xa5] = {OP_ld, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xa6] = {OP_ld, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xa7] = {OP_ld, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xa8] = {OP_ld, -1, 0, opr_n_bytes_p1, reg_xy, z_opr_decode},
    [0xa9] = {OP_ld, -1, 0, opr_n_bytes_p1, reg_xy, z_opr_decode},
    [0xaa] = {OP_jmp, -1, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xab] = {OP_jsr, -1, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xac] = {OP_dec, 0, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xad] = {OP_dec, 1, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xae] = {0xFFFF, -1, exg_sex_discrim,   two, exg_sex_decode, 0},  /* EXG / SEX */
    [0xaf] = {OP_dec, 3, 0, opr_n_bytes_p1, 0, z_opr_decode},
    [0xb0] = {OP_ld, -1, 0, four,  z_reg, z_ext24_decode},
    [0xb1] = {OP_ld, -1, 0, four,  z_reg, z_ext24_decode},
    [0xb2] = {OP_ld, -1, 0, four,  z_reg, z_ext24_decode},
    [0xb3] = {OP_ld, -1, 0, four,  z_reg, z_ext24_decode},
    [0xb4] = {OP_ld, -1, 0, four,  z_reg, z_ext24_decode},
    [0xb5] = {OP_ld, -1, 0, four,  z_reg, z_ext24_decode},
    [0xb6] = {OP_ld, -1, 0, four,  z_reg, z_ext24_decode},
    [0xb7] = {OP_ld, -1, 0, four,  z_reg, z_ext24_decode},
    [0xb8] = {OP_ld, -1, 0, four,  reg_xy, z_ext24_decode},
    [0xb9] = {OP_ld, -1, 0, four,  reg_xy, z_ext24_decode},
    [0xba] = {OP_jmp, -1, 0, four, z_ext24_decode, 0},
    [0xbb] = {OP_jsr, -1, 0, four, z_ext24_decode, 0},
    [0xbc] = {OP_clr, 0, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xbd] = {OP_clr, 1, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xbe] = {OP_clr, 2, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xbf] = {OP_clr, 3, 0, opr_n_bytes_p1, z_opr_decode, 0},
    [0xc0] = {OP_st, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xc1] = {OP_st, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xc2] = {OP_st, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xc3] = {OP_st, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xc4] = {OP_st, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xc5] = {OP_st, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xc6] = {OP_st, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xc7] = {OP_st, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xc8] = {OP_st, -1, 0, opr_n_bytes_p1, reg_xy, z_opr_decode},
    [0xc9] = {OP_st, -1, 0, opr_n_bytes_p1, reg_xy, z_opr_decode},
    [0xca] = {OP_ld, -1, 0, three, reg_xy, ld_18bit_decode},
    [0xcb] = {OP_ld, -1, 0, three, reg_xy, ld_18bit_decode},
    [0xcc] = {OP_com, 0, 0, opr_n_bytes_p1, NULL, z_opr_decode},
    [0xcd] = {OP_com, 1, 0, opr_n_bytes_p1, NULL, z_opr_decode},
    [0xce] = {OP_andcc, -1, 0, two, imm1_decode, 0},
    [0xcf] = {OP_com, 3, 0, opr_n_bytes_p1, NULL, z_opr_decode},
    [0xd0] = {OP_st, -1, 0, four,  z_reg, z_ext24_decode},
    [0xd1] = {OP_st, -1, 0, four,  z_reg, z_ext24_decode},
    [0xd2] = {OP_st, -1, 0, four,  z_reg, z_ext24_decode},
    [0xd3] = {OP_st, -1, 0, four,  z_reg, z_ext24_decode},
    [0xd4] = {OP_st, -1, 0, four,  z_reg, z_ext24_decode},
    [0xd5] = {OP_st, -1, 0, four,  z_reg, z_ext24_decode},
    [0xd6] = {OP_st, -1, 0, four,  z_reg, z_ext24_decode},
    [0xd7] = {OP_st, -1, 0, four,  z_reg, z_ext24_decode},
    [0xd8] = {OP_st, -1, 0, four,  reg_xy, z_ext24_decode},
    [0xd9] = {OP_st, -1, 0, four,  reg_xy, z_ext24_decode},
    [0xda] = {OP_ld, -1, 0, three, reg_xy, ld_18bit_decode},
    [0xdb] = {OP_ld, -1, 0, three, reg_xy, ld_18bit_decode},
    [0xdc] = {OP_neg, 0, 0, opr_n_bytes_p1, NULL, z_opr_decode},
    [0xdd] = {OP_neg, 1, 0, opr_n_bytes_p1, NULL, z_opr_decode},
    [0xde] = {OP_orcc, -1, 0,  two,  imm1_decode, 0},
    [0xdf] = {OP_neg,  3, 0, opr_n_bytes_p1, NULL, z_opr_decode},
    [0xe0] = {OP_cmp, -1, 0, three,  z_reg, z_imm1234_0base},
    [0xe1] = {OP_cmp, -1, 0, three,  z_reg, z_imm1234_0base},
    [0xe2] = {OP_cmp, -1, 0, three,  z_reg, z_imm1234_0base},
    [0xe3] = {OP_cmp, -1, 0, three,  z_reg, z_imm1234_0base},
    [0xe4] = {OP_cmp, -1, 0, two,    z_reg, z_imm1234_0base},
    [0xe5] = {OP_cmp, -1, 0, two,    z_reg, z_imm1234_0base},
    [0xe6] = {OP_cmp, -1, 0, five,   z_reg, z_imm1234_0base},
    [0xe7] = {OP_cmp, -1, 0, five,   z_reg, z_imm1234_0base},
    [0xe8] = {OP_cmp, -1, 0, four,   reg_xy, z_imm1234_0base},
    [0xe9] = {OP_cmp, -1, 0, four,   reg_xy, z_imm1234_0base},
    [0xea] = {OP_ld, -1, 0, three, reg_xy, ld_18bit_decode},
    [0xeb] = {OP_ld, -1, 0, three, reg_xy, ld_18bit_decode},
    [0xec] = {OP_bclr, -1, 0, bm_n_bytes, bm_decode, 0},
    [0xed] = {OP_bset, -1, 0, bm_n_bytes, bm_decode, 0},
    [0xee] = {OP_btgl, -1, 0, bm_n_bytes, bm_decode, 0},
    [0xef] = {OP_INVALID, -1, 0, NULL, NULL, NULL}, /* SPARE */
    [0xf0] = {OP_cmp, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xf1] = {OP_cmp, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xf2] = {OP_cmp, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xf3] = {OP_cmp, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xf4] = {OP_cmp, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xf5] = {OP_cmp, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xf6] = {OP_cmp, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xf7] = {OP_cmp, -1, 0, opr_n_bytes_p1, z_reg,    z_opr_decode},
    [0xf8] = {OP_cmp, -1, 0, opr_n_bytes_p1, reg_xy, z_opr_decode},
    [0xf9] = {OP_cmp, -1, 0, opr_n_bytes_p1, reg_xy, z_opr_decode},
    [0xfa] = {OP_ld, -1, 0,  three, reg_xy, ld_18bit_decode},
    [0xfb] = {OP_ld, -1, 0,  three, reg_xy, ld_18bit_decode},
    [0xfc] = {OP_cmp, -1, 0, single, cmp_xy, 0},
    [0xfd] = {OP_sub, -1, 0, single, sub_d6_x_y, 0},
    [0xfe] = {OP_sub, -1, 0, single, sub_d6_y_x, 0},
    [0xff] = {OP_swi, -1, 0, single, 0, 0}
  };

static const int oprregs1[] =
  {
    REG_D3, REG_D2, REG_D1, REG_D0, REG_CCL, REG_CCH
  };

static const int oprregs2[] =
  {
    REG_Y,  REG_X,  REG_D7, REG_D6, REG_D5,  REG_D4
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
mul_decode (struct mem_read_abstraction_base *mra,
	    int *n_operands, struct operand **operand)
{
  uint8_t mb;
  int status = mra->read (mra, 0, 1, &mb);
  if (status < 0)
    return;

  uint8_t byte;
  status = mra->read (mra, -1, 1, &byte);
  if (status < 0)
    return;

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
  operand[(*n_operands)++] = create_register_operand (byte & 0x07);

  switch (mode)
    {
    case MUL_REG_IMM:
      {
	int size = (mb & 0x3);
	operand[(*n_operands)++] =
	  create_register_operand_with_size ((mb & 0x38) >> 3, size);
	uint32_t imm = z_decode_signed_value (mra, 1, size + 1);
	operand[(*n_operands)++] = create_immediate_operand (imm);
      }
      break;
    case MUL_REG_REG:
      operand[(*n_operands)++] = create_register_operand ((mb & 0x38) >> 3);
      operand[(*n_operands)++] = create_register_operand (mb & 0x07);
      break;
    case MUL_REG_OPR:
      operand[(*n_operands)++] = create_register_operand ((mb & 0x38) >> 3);
      operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1, mb & 0x3);
      break;
    case MUL_OPR_OPR:
      {
	int first = x_opr_n_bytes (mra, 1);
	operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1,
							   (mb & 0x30) >> 4);
	operand[(*n_operands)++] = x_opr_decode_with_size (mra, first + 1,
							   (mb & 0x0c) >> 2);
	break;
      }
    }
}


static int
mul_n_bytes (struct mem_read_abstraction_base *mra)
{
  int nx = 2;
  uint8_t mb;
  int status = mra->read (mra, 0, 1, &mb);
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
      nx += x_opr_n_bytes (mra, 1);
      break;
    case MUL_OPR_OPR:
      {
	int first = x_opr_n_bytes (mra, nx - 1);
	nx += first;
	int second = x_opr_n_bytes (mra, nx - 1);
	nx += second;
      }
      break;
    }

  return nx;
}


/* The NXP documentation is vague about BM_RESERVED0 and BM_RESERVED1,
   and contains obvious typos.
   However the Freescale tools and experiments with the chip itself
   seem to indicate that they behave like BM_REG_IMM and BM_OPR_REG
   respectively.  */

enum BM_MODE
{
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
bm_decode (struct mem_read_abstraction_base *mra,
	   int *n_operands, struct operand **operand)
{
  uint8_t bm;
  int status = mra->read (mra, 0, 1, &bm);
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
    case BM_RESERVED0:
      operand[(*n_operands)++] = create_register_operand (bm & 0x07);
      break;
    case BM_OPR_B:
      operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1, 0);
      break;
    case BM_OPR_W:
      operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1, 1);
      break;
    case BM_OPR_L:
      operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1, 3);
      break;
    case BM_OPR_REG:
    case BM_RESERVED1:
      {
	uint8_t xb;
	mra->read (mra, 1, 1, &xb);
	/* Don't emit a size suffix for register operands */
	if ((xb & 0xF8) != 0xB8)
	  operand[(*n_operands)++] =
	    x_opr_decode_with_size (mra, 1, (bm & 0x0c) >> 2);
	else
	  operand[(*n_operands)++] = x_opr_decode (mra, 1);
      }
      break;
    }

  uint8_t imm = 0;
  switch (mode)
    {
    case BM_REG_IMM:
      imm = (bm & 0x38) >> 3;
      operand[(*n_operands)++] = create_immediate_operand (imm);
      break;
    case BM_OPR_L:
      imm |= (bm & 0x03) << 3;
      /* fallthrough */
    case BM_OPR_W:
      imm |= (bm & 0x01) << 3;
      /* fallthrough */
    case BM_OPR_B:
      imm |= (bm & 0x70) >> 4;
      operand[(*n_operands)++] = create_immediate_operand (imm);
      break;
    case BM_OPR_REG:
    case BM_RESERVED1:
      operand[(*n_operands)++] = create_register_operand ((bm & 0x70) >> 4);
      break;
    case BM_RESERVED0:
      assert (0);
      break;
    }
}


static void
bm_rel_decode (struct mem_read_abstraction_base *mra,
	       int *n_operands, struct operand **operand)
{
  uint8_t bm;
  int status = mra->read (mra, 0, 1, &bm);
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

  int n = 1;
  switch (mode)
    {
    case BM_REG_IMM:
    case BM_RESERVED0:
      operand[(*n_operands)++] = create_register_operand (bm & 0x07);
      break;
    case BM_OPR_B:
      operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1, 0);
      n = 1 + x_opr_n_bytes (mra, 1);
      break;
    case BM_OPR_W:
      operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1, 1);
      n = 1 + x_opr_n_bytes (mra, 1);
      break;
    case BM_OPR_L:
      operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1, 3);
      n = 1 + x_opr_n_bytes (mra, 1);
      break;
    case BM_OPR_REG:
    case BM_RESERVED1:
      {
	uint8_t xb;
	mra->read (mra, +1, 1, &xb);
	/* Don't emit a size suffix for register operands */
	if ((xb & 0xF8) != 0xB8)
	  {
	    short os = (bm & 0x0c) >> 2;
	    operand[(*n_operands)++] = x_opr_decode_with_size (mra, 1, os);
	  }
	else
	  operand[(*n_operands)++] = x_opr_decode (mra, 1);

      }
      break;
    }

  int imm = 0;
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
      operand[(*n_operands)++] = create_immediate_operand (imm);
      break;
    case BM_RESERVED0:
      imm = (bm & 0x38) >> 3;
      operand[(*n_operands)++] = create_immediate_operand (imm);
      break;
    case BM_REG_IMM:
      imm = (bm & 0xF8) >> 3;
      operand[(*n_operands)++] = create_immediate_operand (imm);
      break;
    case BM_OPR_REG:
    case BM_RESERVED1:
      operand[(*n_operands)++] = create_register_operand ((bm & 0x70) >> 4);
      n += x_opr_n_bytes (mra, 1);
      break;
    }

  rel_15_7 (mra, n + 1, n_operands, operand);
}

static int
bm_n_bytes (struct mem_read_abstraction_base *mra)
{
  uint8_t bm;
  int status = mra->read (mra, 0, 1, &bm);
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
    case BM_RESERVED0:
      break;

    case BM_OPR_B:
    case BM_OPR_W:
    case BM_OPR_L:
      n += x_opr_n_bytes (mra, 1);
      break;
    case BM_OPR_REG:
    case BM_RESERVED1:
      n += x_opr_n_bytes (mra, 1);
      break;
    }

  return n;
}

static int
bm_rel_n_bytes (struct mem_read_abstraction_base *mra)
{
  int n = 1 + bm_n_bytes (mra);

  bfd_byte rb;
  int status = mra->read (mra, n - 2, 1, &rb);
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
shift_n_bytes (struct mem_read_abstraction_base *mra)
{
  bfd_byte sb;
  int status = mra->read (mra, 0, 1, &sb);
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
      return 2 + x_opr_n_bytes (mra, 1);
      break;
    case SB_REG_OPR_OPR:
      {
	int opr1 = x_opr_n_bytes (mra, 1);
	int opr2 = 0;
	if ((sb & 0x30) != 0x20)
	  opr2 = x_opr_n_bytes (mra, opr1 + 1);
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

mov_imm_opr_n_bytes (struct mem_read_abstraction_base *mra)
{
  bfd_byte byte;
  int status = mra->read (mra, -1, 1,  &byte);
  if (status < 0)
    return status;

  int size = byte - 0x0c + 1;

  return size + x_opr_n_bytes (mra, size) + 1;
}

static void
mov_imm_opr (struct mem_read_abstraction_base *mra,
	     int *n_operands, struct operand **operand)
{
  bfd_byte byte;
  int status = mra->read (mra, -1, 1, &byte);
  if (status < 0)
    return ;

  int size = byte - 0x0c + 1;
  uint32_t imm = decode_signed_value (mra, size);

  operand[(*n_operands)++] = create_immediate_operand (imm);
  operand[(*n_operands)++] = x_opr_decode (mra, size);
}



static void
ld_18bit_decode (struct mem_read_abstraction_base *mra,
		 int *n_operands, struct operand **operand)
{
  size_t size = 3;
  bfd_byte buffer[3];
  int status = mra->read (mra, 0, 2, buffer + 1);
  if (status < 0)
    return ;

  status = mra->read (mra, -1, 1, buffer);
  if (status < 0)
    return ;

  buffer[0] = (buffer[0] & 0x30) >> 4;

  size_t i;
  uint32_t imm = 0;
  for (i = 0; i < size; ++i)
    {
      imm |= buffer[i] << (8 * (size - i - 1));
    }

  operand[(*n_operands)++] = create_immediate_operand (imm);
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


static int
loop_prim_n_bytes (struct mem_read_abstraction_base *mra)
{
  int mx = 0;
  uint8_t lb;
  mra->read (mra, mx++, 1, &lb);

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
      mx += x_opr_n_bytes (mra, mx) ;
    }

  uint8_t rb;
  mra->read (mra, mx++, 1, &rb);
  if (rb & 0x80)
    mx++;

  return mx + 1;
}




static enum operator
exg_sex_discrim (struct mem_read_abstraction_base *mra, enum operator hint ATTRIBUTE_UNUSED)
{
  uint8_t eb;
  int status = mra->read (mra, 0, 1, &eb);
  if (status < 0)
    return OP_INVALID;

  struct operand *op0 = create_register_operand ((eb & 0xf0) >> 4);
  struct operand *op1 = create_register_operand (eb & 0xf);

  const struct reg *r0 = registers + ((struct register_operand *) op0)->reg;
  const struct reg *r1 = registers + ((struct register_operand *) op1)->reg;

  enum operator operator = (r0->bytes < r1->bytes) ? OP_sex : OP_exg;

  free (op0);
  free (op1);
  
  return operator;
}


static void
exg_sex_decode (struct mem_read_abstraction_base *mra,
		int *n_operands, struct operand **operands)
{
  uint8_t eb;
  int status = mra->read (mra, 0, 1, &eb);
  if (status < 0)
    return;

  /* Ship out the operands.  */
  operands[(*n_operands)++] =  create_register_operand ((eb & 0xf0) >> 4);
  operands[(*n_operands)++] =  create_register_operand (eb & 0xf);
}

static enum operator
loop_primitive_discrim (struct mem_read_abstraction_base *mra,
			enum operator hint ATTRIBUTE_UNUSED)
{
  uint8_t lb;
  int status = mra->read (mra, 0, 1, &lb);
  if (status < 0)
    return OP_INVALID;

  enum operator opbase = (lb & 0x80) ? OP_dbNE : OP_tbNE;
  return opbase + ((lb & 0x70) >> 4);
}

static void
loop_primitive_decode (struct mem_read_abstraction_base *mra,
		  int *n_operands, struct operand **operands)
{
  int offs = 1;
  uint8_t lb;
  int status = mra->read (mra, 0, 1, &lb);
  if (status < 0)
    return ;

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
      operands[(*n_operands)++] = create_register_operand (lb & 0x07);
      break;
    case LP_XY:
      operands[(*n_operands)++] =
	create_register_operand ((lb & 0x01) + REG_X);
      break;
    case LP_OPR:
      offs += x_opr_n_bytes (mra, 1);
      operands[(*n_operands)++] = x_opr_decode_with_size (mra, 1, lb & 0x03);
      break;
    }

  rel_15_7 (mra, offs + 1, n_operands, operands);
}


static enum operator
shift_discrim (struct mem_read_abstraction_base *mra,  enum operator hint ATTRIBUTE_UNUSED)
{
  size_t i;
  uint8_t sb;
  int status = mra->read (mra, 0, 1, &sb);
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

  if (mode == SB_ROT)
    return (dir == SB_LEFT) ? OP_rol : OP_ror;

  if (type == SB_LOGICAL)
    return (dir == SB_LEFT) ? OP_lsl : OP_lsr;

  return (dir == SB_LEFT) ? OP_asl : OP_asr;
}


static void
shift_decode (struct mem_read_abstraction_base *mra,  int *n_operands, struct operand **operands)
{
  size_t i;

  uint8_t byte;
  int status = mra->read (mra, -1, 1, &byte);
  if (status < 0)
    return ;

  uint8_t sb;
  status = mra->read (mra, 0, 1, &sb);
  if (status < 0)
    return ;

  enum SB_MODE mode = -1;
  for (i = 0; i < sizeof (sb_table) / sizeof (sb_table[0]); ++i)
    {
      const struct sb *sbe = sb_table + i;
      if ((sb & sbe->mask) == sbe->value)
	mode = sbe->mode;
    }

  short osize = -1;
  switch (mode)
    {
    case SB_REG_OPR_EFF:
    case SB_ROT:
    case SB_REG_OPR_OPR:
      osize = sb & 0x03;
      break;
    case SB_OPR_N:
      {
	uint8_t xb;
	mra->read (mra, 1, 1, &xb);
	/* The size suffix is not printed if the OPR operand refers
	   directly to a register, because the size is implied by the
	   size of that register. */
	if ((xb & 0xF8) != 0xB8)
	  osize = sb & 0x03;
      }
      break;
    default:
      break;
    };

  /* Destination register */
  switch (mode)
    {
    case SB_REG_REG_N_EFF:
    case SB_REG_REG_N:
      operands[(*n_operands)++] = create_register_operand (byte & 0x07);
      break;
    case SB_REG_OPR_EFF:
    case SB_REG_OPR_OPR:
      operands[(*n_operands)++] = create_register_operand (byte & 0x07);
      break;

    case SB_ROT:
      operands[(*n_operands)++] = x_opr_decode_with_size (mra, 1, osize);
      break;

    default:
      break;
    }

  /* Source register */
  switch (mode)
    {
    case SB_REG_REG_N_EFF:
    case SB_REG_REG_N:
      operands[(*n_operands)++] =
	create_register_operand_with_size (sb & 0x07, osize);
      break;

    case SB_REG_OPR_OPR:
      operands[(*n_operands)++] = x_opr_decode_with_size (mra, 1, osize);
      break;

    default:
      break;
    }

  /* 3rd arg */
  switch (mode)
    {
    case SB_REG_OPR_EFF:
    case SB_OPR_N:
      operands[(*n_operands)++] = x_opr_decode_with_size (mra, 1, osize);
      break;

    case SB_REG_REG_N:
      {
        uint8_t xb;
        mra->read (mra, 1, 1, &xb);

        /* This case is slightly unusual.
           If XB matches the binary pattern 0111XXXX, then instead of
           interpreting this as a general OPR postbyte in the IMMe4 mode,
           the XB byte is interpreted in s special way.  */
        if ((xb & 0xF0) == 0x70)
          {
            if (byte & 0x10)
              {
                int shift = ((sb & 0x08) >> 3) | ((xb & 0x0f) << 1);
                operands[(*n_operands)++] = create_immediate_operand (shift);
              }
            else
              {
                /* This should not happen.  */
                abort ();
              }
          }
        else
          {
            operands[(*n_operands)++] = x_opr_decode (mra, 1);
          }
      }
      break;
    case SB_REG_OPR_OPR:
      {
	uint8_t xb;
	int n = x_opr_n_bytes (mra, 1);
	mra->read (mra, 1 + n, 1, &xb);

	if ((xb & 0xF0) == 0x70)
	  {
	    int imm = xb & 0x0F;
	    imm <<= 1;
	    imm |= (sb & 0x08) >> 3;
	    operands[(*n_operands)++] = create_immediate_operand (imm);
	  }
	else
	  {
	    operands[(*n_operands)++] = x_opr_decode (mra, 1 + n);
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
      {
        int imm = (sb & 0x08) ? 2 : 1;
        operands[(*n_operands)++] = create_immediate_operand (imm);
      }
      break;

    default:
      break;
    }
}

static enum operator
psh_pul_discrim (struct mem_read_abstraction_base *mra,
		 enum operator hint ATTRIBUTE_UNUSED)
{
  uint8_t byte;
  int status = mra->read (mra, 0, 1, &byte);
  if (status != 0)
    return OP_INVALID;

  return (byte & 0x80) ? OP_pull: OP_push;
}


static void
psh_pul_decode (struct mem_read_abstraction_base *mra,
		int *n_operands, struct operand **operand)
{
  uint8_t byte;
  int status = mra->read (mra, 0, 1, &byte);
  if (status != 0)
    return;
  int bit;
  if (byte & 0x40)
    {
      if ((byte & 0x3F) == 0)
        {
	  operand[(*n_operands)++] = create_register_all16_operand ();
        }
      else
	for (bit = 5; bit >= 0; --bit)
	  {
	    if (byte & (0x1 << bit))
	      {
		operand[(*n_operands)++] = create_register_operand (oprregs2[bit]);
	      }
	  }
    }
  else
    {
      if ((byte & 0x3F) == 0)
        {
	  operand[(*n_operands)++] = create_register_all_operand ();
        }
      else
	for (bit = 5; bit >= 0; --bit)
	  {
	    if (byte & (0x1 << bit))
	      {
		operand[(*n_operands)++] = create_register_operand (oprregs1[bit]);
	      }
	  }
    }
}

static enum operator
bit_field_discrim (struct mem_read_abstraction_base *mra, enum operator hint ATTRIBUTE_UNUSED)
{
  int status;
  bfd_byte bb;
  status = mra->read (mra, 0, 1, &bb);
  if (status != 0)
    return OP_INVALID;

  return  (bb & 0x80) ? OP_bfins : OP_bfext;
}

static void
bit_field_decode (struct mem_read_abstraction_base *mra,
		  int *n_operands, struct operand **operands)
{
  int status;

  bfd_byte byte2;
  status = mra->read (mra, -1, 1, &byte2);
  if (status != 0)
    return;

  bfd_byte bb;
  status = mra->read (mra, 0, 1, &bb);
  if (status != 0)
    return;

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
  int reg1 = byte2 & 0x07;
  /* First operand */
  switch (mode)
    {
    case BB_REG_REG_REG:
    case BB_REG_REG_IMM:
    case BB_REG_OPR_REG:
    case BB_REG_OPR_IMM:
      operands[(*n_operands)++] = create_register_operand (reg1);
      break;
    case BB_OPR_REG_REG:
      operands[(*n_operands)++] = x_opr_decode_with_size (mra, 1,
							  (bb >> 2) & 0x03);
      break;
    case BB_OPR_REG_IMM:
      operands[(*n_operands)++] = x_opr_decode_with_size (mra, 2,
							  (bb >> 2) & 0x03);
      break;
    }

  /* Second operand */
  switch (mode)
    {
    case BB_REG_REG_REG:
    case BB_REG_REG_IMM:
      {
        int reg_src = (bb >> 2) & 0x07;
        operands[(*n_operands)++] = create_register_operand (reg_src);
      }
      break;
    case BB_OPR_REG_REG:
    case BB_OPR_REG_IMM:
      {
        int reg_src = (byte2 & 0x07);
        operands[(*n_operands)++] = create_register_operand (reg_src);
      }
      break;
    case BB_REG_OPR_REG:
      operands[(*n_operands)++] = x_opr_decode_with_size (mra, 1,
							  (bb >> 2) & 0x03);
      break;
    case BB_REG_OPR_IMM:
      operands[(*n_operands)++] = x_opr_decode_with_size (mra, 2,
							  (bb >> 2) & 0x03);
      break;
    }

  /* Third operand */
  switch (mode)
    {
    case BB_REG_REG_REG:
    case BB_OPR_REG_REG:
    case BB_REG_OPR_REG:
      {
        int reg_parm = bb & 0x03;
	operands[(*n_operands)++] = create_register_operand (reg_parm);
      }
      break;
    case BB_REG_REG_IMM:
    case BB_OPR_REG_IMM:
    case BB_REG_OPR_IMM:
      {
        bfd_byte i1;
        mra->read (mra, 1, 1, &i1);
        int offset = i1 & 0x1f;
        int width = bb & 0x03;
        width <<= 3;
        width |= i1 >> 5;
        operands[(*n_operands)++] = create_bitfield_operand (width, offset);
      }
      break;
    }
}


/* Decode the next instruction at MRA, according to OPC.
   The operation to be performed is returned.
   The number of operands, will be placed in N_OPERANDS.
   The operands themselved into OPERANDS.  */
static enum operator
decode_operation (const struct opcode *opc,
		  struct mem_read_abstraction_base *mra,
		  int *n_operands, struct operand **operands)
{
  enum operator op = opc->operator;
  if (opc->discriminator)
    op = opc->discriminator (mra, opc->operator);

  if (opc->operands)
    opc->operands (mra, n_operands, operands);

  if (opc->operands2)
    opc->operands2 (mra, n_operands, operands);

  return op;
}

int
decode_s12z (enum operator *myoperator, short *osize,
	     int *n_operands, struct operand **operands,
	     struct mem_read_abstraction_base *mra)
{
  int n_bytes = 0;
  bfd_byte byte;

  int status = mra->read (mra, 0, 1, &byte);
  if (status != 0)
    return status;

  mra->advance (mra);

  const struct opcode *opc = page1 + byte;
  if (byte == PAGE2_PREBYTE)
    {
      /* Opcodes in page2 have an additional byte */
      n_bytes++;

      bfd_byte byte2;
      mra->read (mra, 0, 1, &byte2);
      mra->advance (mra);
      opc = page2 + byte2;
    }
  *myoperator = decode_operation (opc, mra, n_operands, operands);
  *osize = opc->osize;

  /* Return the number of bytes in the instruction.  */
  n_bytes += (opc && opc->insn_bytes) ? opc->insn_bytes (mra) : 0;

  return n_bytes;
}

