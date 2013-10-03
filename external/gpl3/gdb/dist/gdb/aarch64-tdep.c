/* Common target dependent code for GDB on AArch64 systems.

   Copyright (C) 2009-2013 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"

#include "frame.h"
#include "inferior.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "gdb_string.h"
#include "dis-asm.h"
#include "regcache.h"
#include "reggroups.h"
#include "doublest.h"
#include "value.h"
#include "arch-utils.h"
#include "osabi.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "objfiles.h"
#include "dwarf2-frame.h"
#include "gdbtypes.h"
#include "prologue-value.h"
#include "target-descriptions.h"
#include "user-regs.h"
#include "language.h"
#include "infcall.h"

#include "aarch64-tdep.h"

#include "elf-bfd.h"
#include "elf/aarch64.h"

#include "gdb_assert.h"
#include "vec.h"

#include "features/aarch64.c"
#include "features/aarch64-without-fpu.c"

/* Pseudo register base numbers.  */
#define AARCH64_Q0_REGNUM 0
#define AARCH64_D0_REGNUM (AARCH64_Q0_REGNUM + 32)
#define AARCH64_S0_REGNUM (AARCH64_D0_REGNUM + 32)
#define AARCH64_H0_REGNUM (AARCH64_S0_REGNUM + 32)
#define AARCH64_B0_REGNUM (AARCH64_H0_REGNUM + 32)

/* The standard register names, and all the valid aliases for them.  */
static const struct
{
  const char *const name;
  int regnum;
} aarch64_register_aliases[] =
{
  /* 64-bit register names.  */
  {"fp", AARCH64_FP_REGNUM},
  {"lr", AARCH64_LR_REGNUM},
  {"sp", AARCH64_SP_REGNUM},

  /* 32-bit register names.  */
  {"w0", AARCH64_X0_REGNUM + 0},
  {"w1", AARCH64_X0_REGNUM + 1},
  {"w2", AARCH64_X0_REGNUM + 2},
  {"w3", AARCH64_X0_REGNUM + 3},
  {"w4", AARCH64_X0_REGNUM + 4},
  {"w5", AARCH64_X0_REGNUM + 5},
  {"w6", AARCH64_X0_REGNUM + 6},
  {"w7", AARCH64_X0_REGNUM + 7},
  {"w8", AARCH64_X0_REGNUM + 8},
  {"w9", AARCH64_X0_REGNUM + 9},
  {"w10", AARCH64_X0_REGNUM + 10},
  {"w11", AARCH64_X0_REGNUM + 11},
  {"w12", AARCH64_X0_REGNUM + 12},
  {"w13", AARCH64_X0_REGNUM + 13},
  {"w14", AARCH64_X0_REGNUM + 14},
  {"w15", AARCH64_X0_REGNUM + 15},
  {"w16", AARCH64_X0_REGNUM + 16},
  {"w17", AARCH64_X0_REGNUM + 17},
  {"w18", AARCH64_X0_REGNUM + 18},
  {"w19", AARCH64_X0_REGNUM + 19},
  {"w20", AARCH64_X0_REGNUM + 20},
  {"w21", AARCH64_X0_REGNUM + 21},
  {"w22", AARCH64_X0_REGNUM + 22},
  {"w23", AARCH64_X0_REGNUM + 23},
  {"w24", AARCH64_X0_REGNUM + 24},
  {"w25", AARCH64_X0_REGNUM + 25},
  {"w26", AARCH64_X0_REGNUM + 26},
  {"w27", AARCH64_X0_REGNUM + 27},
  {"w28", AARCH64_X0_REGNUM + 28},
  {"w29", AARCH64_X0_REGNUM + 29},
  {"w30", AARCH64_X0_REGNUM + 30},

  /*  specials */
  {"ip0", AARCH64_X0_REGNUM + 16},
  {"ip1", AARCH64_X0_REGNUM + 17}
};

/* The required core 'R' registers.  */
static const char *const aarch64_r_register_names[] =
{
  /* These registers must appear in consecutive RAW register number
     order and they must begin with AARCH64_X0_REGNUM! */
  "x0", "x1", "x2", "x3",
  "x4", "x5", "x6", "x7",
  "x8", "x9", "x10", "x11",
  "x12", "x13", "x14", "x15",
  "x16", "x17", "x18", "x19",
  "x20", "x21", "x22", "x23",
  "x24", "x25", "x26", "x27",
  "x28", "x29", "x30", "sp",
  "pc", "cpsr"
};

/* The FP/SIMD 'V' registers.  */
static const char *const aarch64_v_register_names[] =
{
  /* These registers must appear in consecutive RAW register number
     order and they must begin with AARCH64_V0_REGNUM! */
  "v0", "v1", "v2", "v3",
  "v4", "v5", "v6", "v7",
  "v8", "v9", "v10", "v11",
  "v12", "v13", "v14", "v15",
  "v16", "v17", "v18", "v19",
  "v20", "v21", "v22", "v23",
  "v24", "v25", "v26", "v27",
  "v28", "v29", "v30", "v31",
  "fpsr",
  "fpcr"
};

/* AArch64 prologue cache structure.  */
struct aarch64_prologue_cache
{
  /* The stack pointer at the time this frame was created; i.e. the
     caller's stack pointer when this function was called.  It is used
     to identify this frame.  */
  CORE_ADDR prev_sp;

  /* The frame base for this frame is just prev_sp - frame size.
     FRAMESIZE is the distance from the frame pointer to the
     initial stack pointer.  */
  int framesize;

  /* The register used to hold the frame pointer for this frame.  */
  int framereg;

  /* Saved register offsets.  */
  struct trad_frame_saved_reg *saved_regs;
};

/* Toggle this file's internal debugging dump.  */
static int aarch64_debug;

static void
show_aarch64_debug (struct ui_file *file, int from_tty,
                    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("AArch64 debugging is %s.\n"), value);
}

/* Extract a signed value from a bit field within an instruction
   encoding.

   INSN is the instruction opcode.

   WIDTH specifies the width of the bit field to extract (in bits).

   OFFSET specifies the least significant bit of the field where bits
   are numbered zero counting from least to most significant.  */

static int32_t
extract_signed_bitfield (uint32_t insn, unsigned width, unsigned offset)
{
  unsigned shift_l = sizeof (int32_t) * 8 - (offset + width);
  unsigned shift_r = sizeof (int32_t) * 8 - width;

  return ((int32_t) insn << shift_l) >> shift_r;
}

/* Determine if specified bits within an instruction opcode matches a
   specific pattern.

   INSN is the instruction opcode.

   MASK specifies the bits within the opcode that are to be tested
   agsinst for a match with PATTERN.  */

static int
decode_masked_match (uint32_t insn, uint32_t mask, uint32_t pattern)
{
  return (insn & mask) == pattern;
}

/* Decode an opcode if it represents an immediate ADD or SUB instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   RD receives the 'rd' field from the decoded instruction.
   RN receives the 'rn' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */
static int
decode_add_sub_imm (CORE_ADDR addr, uint32_t insn, unsigned *rd, unsigned *rn,
		    int32_t *imm)
{
  if ((insn & 0x9f000000) == 0x91000000)
    {
      unsigned shift;
      unsigned op_is_sub;

      *rd = (insn >> 0) & 0x1f;
      *rn = (insn >> 5) & 0x1f;
      *imm = (insn >> 10) & 0xfff;
      shift = (insn >> 22) & 0x3;
      op_is_sub = (insn >> 30) & 0x1;

      switch (shift)
	{
	case 0:
	  break;
	case 1:
	  *imm <<= 12;
	  break;
	default:
	  /* UNDEFINED */
	  return 0;
	}

      if (op_is_sub)
	*imm = -*imm;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x add x%u, x%u, #%d\n",
			    core_addr_to_string_nz (addr), insn, *rd, *rn,
			    *imm);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents an ADRP instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   RD receives the 'rd' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_adrp (CORE_ADDR addr, uint32_t insn, unsigned *rd)
{
  if (decode_masked_match (insn, 0x9f000000, 0x90000000))
    {
      *rd = (insn >> 0) & 0x1f;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x adrp x%u, #?\n",
			    core_addr_to_string_nz (addr), insn, *rd);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents an branch immediate or branch
   and link immediate instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   LINK receives the 'link' bit from the decoded instruction.
   OFFSET receives the immediate offset from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_b (CORE_ADDR addr, uint32_t insn, unsigned *link, int32_t *offset)
{
  /* b  0001 01ii iiii iiii iiii iiii iiii iiii */
  /* bl 1001 01ii iiii iiii iiii iiii iiii iiii */
  if (decode_masked_match (insn, 0x7c000000, 0x14000000))
    {
      *link = insn >> 31;
      *offset = extract_signed_bitfield (insn, 26, 0) << 2;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x %s 0x%s\n",
			    core_addr_to_string_nz (addr), insn,
			    *link ? "bl" : "b",
			    core_addr_to_string_nz (addr + *offset));

      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a conditional branch instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   COND receives the branch condition field from the decoded
   instruction.
   OFFSET receives the immediate offset from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_bcond (CORE_ADDR addr, uint32_t insn, unsigned *cond, int32_t *offset)
{
  if (decode_masked_match (insn, 0xfe000000, 0x54000000))
    {
      *cond = (insn >> 0) & 0xf;
      *offset = extract_signed_bitfield (insn, 19, 5) << 2;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x b<%u> 0x%s\n",
			    core_addr_to_string_nz (addr), insn, *cond,
			    core_addr_to_string_nz (addr + *offset));
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a branch via register instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   LINK receives the 'link' bit from the decoded instruction.
   RN receives the 'rn' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_br (CORE_ADDR addr, uint32_t insn, unsigned *link, unsigned *rn)
{
  /*         8   4   0   6   2   8   4   0 */
  /* blr  110101100011111100000000000rrrrr */
  /* br   110101100001111100000000000rrrrr */
  if (decode_masked_match (insn, 0xffdffc1f, 0xd61f0000))
    {
      *link = (insn >> 21) & 1;
      *rn = (insn >> 5) & 0x1f;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x %s 0x%x\n",
			    core_addr_to_string_nz (addr), insn,
			    *link ? "blr" : "br", *rn);

      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a CBZ or CBNZ instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   IS64 receives the 'sf' field from the decoded instruction.
   OP receives the 'op' field from the decoded instruction.
   RN receives the 'rn' field from the decoded instruction.
   OFFSET receives the 'imm19' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_cb (CORE_ADDR addr,
	   uint32_t insn, int *is64, unsigned *op, unsigned *rn,
	   int32_t *offset)
{
  if (decode_masked_match (insn, 0x7e000000, 0x34000000))
    {
      /* cbz  T011 010o iiii iiii iiii iiii iiir rrrr */
      /* cbnz T011 010o iiii iiii iiii iiii iiir rrrr */

      *rn = (insn >> 0) & 0x1f;
      *is64 = (insn >> 31) & 0x1;
      *op = (insn >> 24) & 0x1;
      *offset = extract_signed_bitfield (insn, 19, 5) << 2;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x %s 0x%s\n",
			    core_addr_to_string_nz (addr), insn,
			    *op ? "cbnz" : "cbz",
			    core_addr_to_string_nz (addr + *offset));
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a ERET instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_eret (CORE_ADDR addr, uint32_t insn)
{
  /* eret 1101 0110 1001 1111 0000 0011 1110 0000 */
  if (insn == 0xd69f03e0)
    {
      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog, "decode: 0x%s 0x%x eret\n",
			    core_addr_to_string_nz (addr), insn);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a MOVZ instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   RD receives the 'rd' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_movz (CORE_ADDR addr, uint32_t insn, unsigned *rd)
{
  if (decode_masked_match (insn, 0xff800000, 0x52800000))
    {
      *rd = (insn >> 0) & 0x1f;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x movz x%u, #?\n",
			    core_addr_to_string_nz (addr), insn, *rd);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a ORR (shifted register)
   instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   RD receives the 'rd' field from the decoded instruction.
   RN receives the 'rn' field from the decoded instruction.
   RM receives the 'rm' field from the decoded instruction.
   IMM receives the 'imm6' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_orr_shifted_register_x (CORE_ADDR addr,
			       uint32_t insn, unsigned *rd, unsigned *rn,
			       unsigned *rm, int32_t *imm)
{
  if (decode_masked_match (insn, 0xff200000, 0xaa000000))
    {
      *rd = (insn >> 0) & 0x1f;
      *rn = (insn >> 5) & 0x1f;
      *rm = (insn >> 16) & 0x1f;
      *imm = (insn >> 10) & 0x3f;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x orr x%u, x%u, x%u, #%u\n",
			    core_addr_to_string_nz (addr), insn, *rd,
			    *rn, *rm, *imm);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a RET instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   RN receives the 'rn' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_ret (CORE_ADDR addr, uint32_t insn, unsigned *rn)
{
  if (decode_masked_match (insn, 0xfffffc1f, 0xd65f0000))
    {
      *rn = (insn >> 5) & 0x1f;
      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x ret x%u\n",
			    core_addr_to_string_nz (addr), insn, *rn);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents the following instruction:
   STP rt, rt2, [rn, #imm]

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   RT1 receives the 'rt' field from the decoded instruction.
   RT2 receives the 'rt2' field from the decoded instruction.
   RN receives the 'rn' field from the decoded instruction.
   IMM receives the 'imm' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_stp_offset (CORE_ADDR addr,
		   uint32_t insn,
		   unsigned *rt1, unsigned *rt2, unsigned *rn, int32_t *imm)
{
  if (decode_masked_match (insn, 0xffc00000, 0xa9000000))
    {
      *rt1 = (insn >> 0) & 0x1f;
      *rn = (insn >> 5) & 0x1f;
      *rt2 = (insn >> 10) & 0x1f;
      *imm = extract_signed_bitfield (insn, 7, 15);
      *imm <<= 3;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x stp x%u, x%u, [x%u + #%d]\n",
			    core_addr_to_string_nz (addr), insn,
			    *rt1, *rt2, *rn, *imm);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents the following instruction:
   STP rt, rt2, [rn, #imm]!

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   RT1 receives the 'rt' field from the decoded instruction.
   RT2 receives the 'rt2' field from the decoded instruction.
   RN receives the 'rn' field from the decoded instruction.
   IMM receives the 'imm' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_stp_offset_wb (CORE_ADDR addr,
		      uint32_t insn,
		      unsigned *rt1, unsigned *rt2, unsigned *rn,
		      int32_t *imm)
{
  if (decode_masked_match (insn, 0xffc00000, 0xa9800000))
    {
      *rt1 = (insn >> 0) & 0x1f;
      *rn = (insn >> 5) & 0x1f;
      *rt2 = (insn >> 10) & 0x1f;
      *imm = extract_signed_bitfield (insn, 7, 15);
      *imm <<= 3;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x stp x%u, x%u, [x%u + #%d]!\n",
			    core_addr_to_string_nz (addr), insn,
			    *rt1, *rt2, *rn, *imm);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents the following instruction:
   STUR rt, [rn, #imm]

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   IS64 receives size field from the decoded instruction.
   RT receives the 'rt' field from the decoded instruction.
   RN receives the 'rn' field from the decoded instruction.
   IMM receives the 'imm' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_stur (CORE_ADDR addr, uint32_t insn, int *is64, unsigned *rt,
	     unsigned *rn, int32_t *imm)
{
  if (decode_masked_match (insn, 0xbfe00c00, 0xb8000000))
    {
      *is64 = (insn >> 30) & 1;
      *rt = (insn >> 0) & 0x1f;
      *rn = (insn >> 5) & 0x1f;
      *imm = extract_signed_bitfield (insn, 9, 12);

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x stur %c%u, [x%u + #%d]\n",
			    core_addr_to_string_nz (addr), insn,
			    *is64 ? 'x' : 'w', *rt, *rn, *imm);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a TB or TBNZ instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   OP receives the 'op' field from the decoded instruction.
   BIT receives the bit position field from the decoded instruction.
   RT receives 'rt' field from the decoded instruction.
   IMM receives 'imm' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

static int
decode_tb (CORE_ADDR addr,
	   uint32_t insn, unsigned *op, unsigned *bit, unsigned *rt,
	   int32_t *imm)
{
  if (decode_masked_match (insn, 0x7e000000, 0x36000000))
    {
      /* tbz  b011 0110 bbbb biii iiii iiii iiir rrrr */
      /* tbnz B011 0111 bbbb biii iiii iiii iiir rrrr */

      *rt = (insn >> 0) & 0x1f;
      *op = insn & (1 << 24);
      *bit = ((insn >> (31 - 4)) & 0x20) | ((insn >> 19) & 0x1f);
      *imm = extract_signed_bitfield (insn, 14, 5) << 2;

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "decode: 0x%s 0x%x %s x%u, #%u, 0x%s\n",
			    core_addr_to_string_nz (addr), insn,
			    *op ? "tbnz" : "tbz", *rt, *bit,
			    core_addr_to_string_nz (addr + *imm));
      return 1;
    }
  return 0;
}

/* Analyze a prologue, looking for a recognizable stack frame
   and frame pointer.  Scan until we encounter a store that could
   clobber the stack frame unexpectedly, or an unknown instruction.  */

static CORE_ADDR
aarch64_analyze_prologue (struct gdbarch *gdbarch,
			  CORE_ADDR start, CORE_ADDR limit,
			  struct aarch64_prologue_cache *cache)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  int i;
  pv_t regs[AARCH64_X_REGISTER_COUNT];
  struct pv_area *stack;
  struct cleanup *back_to;

  for (i = 0; i < AARCH64_X_REGISTER_COUNT; i++)
    regs[i] = pv_register (i, 0);
  stack = make_pv_area (AARCH64_SP_REGNUM, gdbarch_addr_bit (gdbarch));
  back_to = make_cleanup_free_pv_area (stack);

  for (; start < limit; start += 4)
    {
      uint32_t insn;
      unsigned rd;
      unsigned rn;
      unsigned rm;
      unsigned rt;
      unsigned rt1;
      unsigned rt2;
      int op_is_sub;
      int32_t imm;
      unsigned cond;
      unsigned is64;
      unsigned is_link;
      unsigned op;
      unsigned bit;
      int32_t offset;

      insn = read_memory_unsigned_integer (start, 4, byte_order_for_code);

      if (decode_add_sub_imm (start, insn, &rd, &rn, &imm))
	regs[rd] = pv_add_constant (regs[rn], imm);
      else if (decode_adrp (start, insn, &rd))
	regs[rd] = pv_unknown ();
      else if (decode_b (start, insn, &is_link, &offset))
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (decode_bcond (start, insn, &cond, &offset))
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (decode_br (start, insn, &is_link, &rn))
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (decode_cb (start, insn, &is64, &op, &rn, &offset))
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (decode_eret (start, insn))
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (decode_movz (start, insn, &rd))
	regs[rd] = pv_unknown ();
      else
	if (decode_orr_shifted_register_x (start, insn, &rd, &rn, &rm, &imm))
	{
	  if (imm == 0 && rn == 31)
	    regs[rd] = regs[rm];
	  else
	    {
	      if (aarch64_debug)
		fprintf_unfiltered
		  (gdb_stdlog,
		   "aarch64: prologue analysis gave up addr=0x%s "
		   "opcode=0x%x (orr x register)\n",
		   core_addr_to_string_nz (start),
		   insn);
	      break;
	    }
	}
      else if (decode_ret (start, insn, &rn))
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (decode_stur (start, insn, &is64, &rt, &rn, &offset))
	{
	  pv_area_store (stack, pv_add_constant (regs[rn], offset),
			 is64 ? 8 : 4, regs[rt]);
	}
      else if (decode_stp_offset (start, insn, &rt1, &rt2, &rn, &imm))
	{
	  /* If recording this store would invalidate the store area
	     (perhaps because rn is not known) then we should abandon
	     further prologue analysis.  */
	  if (pv_area_store_would_trash (stack,
					 pv_add_constant (regs[rn], imm)))
	    break;

	  if (pv_area_store_would_trash (stack,
					 pv_add_constant (regs[rn], imm + 8)))
	    break;

	  pv_area_store (stack, pv_add_constant (regs[rn], imm), 8,
			 regs[rt1]);
	  pv_area_store (stack, pv_add_constant (regs[rn], imm + 8), 8,
			 regs[rt2]);
	}
      else if (decode_stp_offset_wb (start, insn, &rt1, &rt2, &rn, &imm))
	{
	  /* If recording this store would invalidate the store area
	     (perhaps because rn is not known) then we should abandon
	     further prologue analysis.  */
	  if (pv_area_store_would_trash (stack,
					 pv_add_constant (regs[rn], imm)))
	    break;

	  if (pv_area_store_would_trash (stack,
					 pv_add_constant (regs[rn], imm + 8)))
	    break;

	  pv_area_store (stack, pv_add_constant (regs[rn], imm), 8,
			 regs[rt1]);
	  pv_area_store (stack, pv_add_constant (regs[rn], imm + 8), 8,
			 regs[rt2]);
	  regs[rn] = pv_add_constant (regs[rn], imm);
	}
      else if (decode_tb (start, insn, &op, &bit, &rn, &offset))
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else
	{
	  if (aarch64_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"aarch64: prologue analysis gave up addr=0x%s"
				" opcode=0x%x\n",
				core_addr_to_string_nz (start), insn);
	  break;
	}
    }

  if (cache == NULL)
    {
      do_cleanups (back_to);
      return start;
    }

  if (pv_is_register (regs[AARCH64_FP_REGNUM], AARCH64_SP_REGNUM))
    {
      /* Frame pointer is fp.  Frame size is constant.  */
      cache->framereg = AARCH64_FP_REGNUM;
      cache->framesize = -regs[AARCH64_FP_REGNUM].k;
    }
  else if (pv_is_register (regs[AARCH64_SP_REGNUM], AARCH64_SP_REGNUM))
    {
      /* Try the stack pointer.  */
      cache->framesize = -regs[AARCH64_SP_REGNUM].k;
      cache->framereg = AARCH64_SP_REGNUM;
    }
  else
    {
      /* We're just out of luck.  We don't know where the frame is.  */
      cache->framereg = -1;
      cache->framesize = 0;
    }

  for (i = 0; i < AARCH64_X_REGISTER_COUNT; i++)
    {
      CORE_ADDR offset;

      if (pv_area_find_reg (stack, gdbarch, i, &offset))
	cache->saved_regs[i].addr = offset;
    }

  do_cleanups (back_to);
  return start;
}

/* Implement the "skip_prologue" gdbarch method.  */

static CORE_ADDR
aarch64_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  unsigned long inst;
  CORE_ADDR skip_pc;
  CORE_ADDR func_addr, limit_pc;
  struct symtab_and_line sal;

  /* See if we can determine the end of the prologue via the symbol
     table.  If so, then return either PC, or the PC after the
     prologue, whichever is greater.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
	return max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to
     provide that bound, then use an arbitrary large number as the
     upper bound.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 128;	/* Magic.  */

  /* Try disassembling prologue.  */
  return aarch64_analyze_prologue (gdbarch, pc, limit_pc, NULL);
}

/* Scan the function prologue for THIS_FRAME and populate the prologue
   cache CACHE.  */

static void
aarch64_scan_prologue (struct frame_info *this_frame,
		       struct aarch64_prologue_cache *cache)
{
  CORE_ADDR block_addr = get_frame_address_in_block (this_frame);
  CORE_ADDR prologue_start;
  CORE_ADDR prologue_end;
  CORE_ADDR prev_pc = get_frame_pc (this_frame);
  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  /* Assume we do not find a frame.  */
  cache->framereg = -1;
  cache->framesize = 0;

  if (find_pc_partial_function (block_addr, NULL, &prologue_start,
				&prologue_end))
    {
      struct symtab_and_line sal = find_pc_line (prologue_start, 0);

      if (sal.line == 0)
	{
	  /* No line info so use the current PC.  */
	  prologue_end = prev_pc;
	}
      else if (sal.end < prologue_end)
	{
	  /* The next line begins after the function end.  */
	  prologue_end = sal.end;
	}

      prologue_end = min (prologue_end, prev_pc);
      aarch64_analyze_prologue (gdbarch, prologue_start, prologue_end, cache);
    }
  else
    {
      CORE_ADDR frame_loc;
      LONGEST saved_fp;
      LONGEST saved_lr;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

      frame_loc = get_frame_register_unsigned (this_frame, AARCH64_FP_REGNUM);
      if (frame_loc == 0)
	return;

      cache->framereg = AARCH64_FP_REGNUM;
      cache->framesize = 16;
      cache->saved_regs[29].addr = 0;
      cache->saved_regs[30].addr = 8;
    }
}

/* Allocate an aarch64_prologue_cache and fill it with information
   about the prologue of *THIS_FRAME.  */

static struct aarch64_prologue_cache *
aarch64_make_prologue_cache (struct frame_info *this_frame)
{
  struct aarch64_prologue_cache *cache;
  CORE_ADDR unwound_fp;
  int reg;

  cache = FRAME_OBSTACK_ZALLOC (struct aarch64_prologue_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  aarch64_scan_prologue (this_frame, cache);

  if (cache->framereg == -1)
    return cache;

  unwound_fp = get_frame_register_unsigned (this_frame, cache->framereg);
  if (unwound_fp == 0)
    return cache;

  cache->prev_sp = unwound_fp + cache->framesize;

  /* Calculate actual addresses of saved registers using offsets
     determined by aarch64_analyze_prologue.  */
  for (reg = 0; reg < gdbarch_num_regs (get_frame_arch (this_frame)); reg++)
    if (trad_frame_addr_p (cache->saved_regs, reg))
      cache->saved_regs[reg].addr += cache->prev_sp;

  return cache;
}

/* Our frame ID for a normal frame is the current function's starting
   PC and the caller's SP when we were called.  */

static void
aarch64_prologue_this_id (struct frame_info *this_frame,
			  void **this_cache, struct frame_id *this_id)
{
  struct aarch64_prologue_cache *cache;
  struct frame_id id;
  CORE_ADDR pc, func;

  if (*this_cache == NULL)
    *this_cache = aarch64_make_prologue_cache (this_frame);
  cache = *this_cache;

  /* This is meant to halt the backtrace at "_start".  */
  pc = get_frame_pc (this_frame);
  if (pc <= gdbarch_tdep (get_frame_arch (this_frame))->lowest_pc)
    return;

  /* If we've hit a wall, stop.  */
  if (cache->prev_sp == 0)
    return;

  func = get_frame_func (this_frame);
  id = frame_id_build (cache->prev_sp, func);
  *this_id = id;
}

/* Implement the "prev_register" frame_unwind method.  */

static struct value *
aarch64_prologue_prev_register (struct frame_info *this_frame,
				void **this_cache, int prev_regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct aarch64_prologue_cache *cache;

  if (*this_cache == NULL)
    *this_cache = aarch64_make_prologue_cache (this_frame);
  cache = *this_cache;

  /* If we are asked to unwind the PC, then we need to return the LR
     instead.  The prologue may save PC, but it will point into this
     frame's prologue, not the next frame's resume location.  */
  if (prev_regnum == AARCH64_PC_REGNUM)
    {
      CORE_ADDR lr;

      lr = frame_unwind_register_unsigned (this_frame, AARCH64_LR_REGNUM);
      return frame_unwind_got_constant (this_frame, prev_regnum, lr);
    }

  /* SP is generally not saved to the stack, but this frame is
     identified by the next frame's stack pointer at the time of the
     call.  The value was already reconstructed into PREV_SP.  */
  /*
         +----------+  ^
         | saved lr |  |
      +->| saved fp |--+
      |  |          |
      |  |          |     <- Previous SP
      |  +----------+
      |  | saved lr |
      +--| saved fp |<- FP
         |          |
         |          |<- SP
         +----------+  */
  if (prev_regnum == AARCH64_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, prev_regnum,
				      cache->prev_sp);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs,
				       prev_regnum);
}

/* AArch64 prologue unwinder.  */
struct frame_unwind aarch64_prologue_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  aarch64_prologue_this_id,
  aarch64_prologue_prev_register,
  NULL,
  default_frame_sniffer
};

/* Allocate an aarch64_prologue_cache and fill it with information
   about the prologue of *THIS_FRAME.  */

static struct aarch64_prologue_cache *
aarch64_make_stub_cache (struct frame_info *this_frame)
{
  int reg;
  struct aarch64_prologue_cache *cache;
  CORE_ADDR unwound_fp;

  cache = FRAME_OBSTACK_ZALLOC (struct aarch64_prologue_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  cache->prev_sp
    = get_frame_register_unsigned (this_frame, AARCH64_SP_REGNUM);

  return cache;
}

/* Our frame ID for a stub frame is the current SP and LR.  */

static void
aarch64_stub_this_id (struct frame_info *this_frame,
		      void **this_cache, struct frame_id *this_id)
{
  struct aarch64_prologue_cache *cache;

  if (*this_cache == NULL)
    *this_cache = aarch64_make_stub_cache (this_frame);
  cache = *this_cache;

  *this_id = frame_id_build (cache->prev_sp, get_frame_pc (this_frame));
}

/* Implement the "sniffer" frame_unwind method.  */

static int
aarch64_stub_unwind_sniffer (const struct frame_unwind *self,
			     struct frame_info *this_frame,
			     void **this_prologue_cache)
{
  CORE_ADDR addr_in_block;
  gdb_byte dummy[4];

  addr_in_block = get_frame_address_in_block (this_frame);
  if (in_plt_section (addr_in_block, NULL)
      /* We also use the stub winder if the target memory is unreadable
	 to avoid having the prologue unwinder trying to read it.  */
      || target_read_memory (get_frame_pc (this_frame), dummy, 4) != 0)
    return 1;

  return 0;
}

/* AArch64 stub unwinder.  */
struct frame_unwind aarch64_stub_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  aarch64_stub_this_id,
  aarch64_prologue_prev_register,
  NULL,
  aarch64_stub_unwind_sniffer
};

/* Return the frame base address of *THIS_FRAME.  */

static CORE_ADDR
aarch64_normal_frame_base (struct frame_info *this_frame, void **this_cache)
{
  struct aarch64_prologue_cache *cache;

  if (*this_cache == NULL)
    *this_cache = aarch64_make_prologue_cache (this_frame);
  cache = *this_cache;

  return cache->prev_sp - cache->framesize;
}

/* AArch64 default frame base information.  */
struct frame_base aarch64_normal_base =
{
  &aarch64_prologue_unwind,
  aarch64_normal_frame_base,
  aarch64_normal_frame_base,
  aarch64_normal_frame_base
};

/* Assuming THIS_FRAME is a dummy, return the frame ID of that
   dummy frame.  The frame ID's base needs to match the TOS value
   saved by save_dummy_frame_tos () and returned from
   aarch64_push_dummy_call, and the PC needs to match the dummy
   frame's breakpoint.  */

static struct frame_id
aarch64_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  return frame_id_build (get_frame_register_unsigned (this_frame,
						      AARCH64_SP_REGNUM),
			 get_frame_pc (this_frame));
}

/* Implement the "unwind_pc" gdbarch method.  */

static CORE_ADDR
aarch64_unwind_pc (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  CORE_ADDR pc
    = frame_unwind_register_unsigned (this_frame, AARCH64_PC_REGNUM);

  return pc;
}

/* Implement the "unwind_sp" gdbarch method.  */

static CORE_ADDR
aarch64_unwind_sp (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  return frame_unwind_register_unsigned (this_frame, AARCH64_SP_REGNUM);
}

/* Return the value of the REGNUM register in the previous frame of
   *THIS_FRAME.  */

static struct value *
aarch64_dwarf2_prev_register (struct frame_info *this_frame,
			      void **this_cache, int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR lr;

  switch (regnum)
    {
    case AARCH64_PC_REGNUM:
      lr = frame_unwind_register_unsigned (this_frame, AARCH64_LR_REGNUM);
      return frame_unwind_got_constant (this_frame, regnum, lr);

    default:
      internal_error (__FILE__, __LINE__,
		      _("Unexpected register %d"), regnum);
    }
}

/* Implement the "init_reg" dwarf2_frame_ops method.  */

static void
aarch64_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			       struct dwarf2_frame_state_reg *reg,
			       struct frame_info *this_frame)
{
  switch (regnum)
    {
    case AARCH64_PC_REGNUM:
      reg->how = DWARF2_FRAME_REG_FN;
      reg->loc.fn = aarch64_dwarf2_prev_register;
      break;
    case AARCH64_SP_REGNUM:
      reg->how = DWARF2_FRAME_REG_CFA;
      break;
    }
}

/* When arguments must be pushed onto the stack, they go on in reverse
   order.  The code below implements a FILO (stack) to do this.  */

typedef struct
{
  /* Value to pass on stack.  */
  const void *data;

  /* Size in bytes of value to pass on stack.  */
  int len;
} stack_item_t;

DEF_VEC_O (stack_item_t);

/* Return the alignment (in bytes) of the given type.  */

static int
aarch64_type_align (struct type *t)
{
  int n;
  int align;
  int falign;

  t = check_typedef (t);
  switch (TYPE_CODE (t))
    {
    default:
      /* Should never happen.  */
      internal_error (__FILE__, __LINE__, _("unknown type alignment"));
      return 4;

    case TYPE_CODE_PTR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_BITSTRING:
    case TYPE_CODE_REF:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
      return TYPE_LENGTH (t);

    case TYPE_CODE_ARRAY:
    case TYPE_CODE_COMPLEX:
      return aarch64_type_align (TYPE_TARGET_TYPE (t));

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      align = 1;
      for (n = 0; n < TYPE_NFIELDS (t); n++)
	{
	  falign = aarch64_type_align (TYPE_FIELD_TYPE (t, n));
	  if (falign > align)
	    align = falign;
	}
      return align;
    }
}

/* Return 1 if *TY is a homogeneous floating-point aggregate as
   defined in the AAPCS64 ABI document; otherwise return 0.  */

static int
is_hfa (struct type *ty)
{
  switch (TYPE_CODE (ty))
    {
    case TYPE_CODE_ARRAY:
      {
	struct type *target_ty = TYPE_TARGET_TYPE (ty);
	if (TYPE_CODE (target_ty) == TYPE_CODE_FLT && TYPE_LENGTH (ty) <= 4)
	  return 1;
	break;
      }

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      {
	if (TYPE_NFIELDS (ty) > 0 && TYPE_NFIELDS (ty) <= 4)
	  {
	    struct type *member0_type;

	    member0_type = check_typedef (TYPE_FIELD_TYPE (ty, 0));
	    if (TYPE_CODE (member0_type) == TYPE_CODE_FLT)
	      {
		int i;

		for (i = 0; i < TYPE_NFIELDS (ty); i++)
		  {
		    struct type *member1_type;

		    member1_type = check_typedef (TYPE_FIELD_TYPE (ty, i));
		    if (TYPE_CODE (member0_type) != TYPE_CODE (member1_type)
			|| (TYPE_LENGTH (member0_type)
			    != TYPE_LENGTH (member1_type)))
		      return 0;
		  }
		return 1;
	      }
	  }
	return 0;
      }

    default:
      break;
    }

  return 0;
}

/* AArch64 function call information structure.  */
struct aarch64_call_info
{
  /* the current argument number.  */
  unsigned argnum;

  /* The next general purpose register number, equivalent to NGRN as
     described in the AArch64 Procedure Call Standard.  */
  unsigned ngrn;

  /* The next SIMD and floating point register number, equivalent to
     NSRN as described in the AArch64 Procedure Call Standard.  */
  unsigned nsrn;

  /* The next stacked argument address, equivalent to NSAA as
     described in the AArch64 Procedure Call Standard.  */
  unsigned nsaa;

  /* Stack item vector.  */
  VEC(stack_item_t) *si;
};

/* Pass a value in a sequence of consecutive X registers.  The caller
   is responsbile for ensuring sufficient registers are available.  */

static void
pass_in_x (struct gdbarch *gdbarch, struct regcache *regcache,
	   struct aarch64_call_info *info, struct type *type,
	   const bfd_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = TYPE_LENGTH (type);
  enum type_code typecode = TYPE_CODE (type);
  int regnum = AARCH64_X0_REGNUM + info->ngrn;

  info->argnum++;

  while (len > 0)
    {
      int partial_len = len < X_REGISTER_SIZE ? len : X_REGISTER_SIZE;
      CORE_ADDR regval = extract_unsigned_integer (buf, partial_len,
						   byte_order);


      /* Adjust sub-word struct/union args when big-endian.  */
      if (byte_order == BFD_ENDIAN_BIG
	  && partial_len < X_REGISTER_SIZE
	  && (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION))
	regval <<= ((X_REGISTER_SIZE - partial_len) * TARGET_CHAR_BIT);

      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog, "arg %d in %s = 0x%s\n",
			    info->argnum,
			    gdbarch_register_name (gdbarch, regnum),
			    phex (regval, X_REGISTER_SIZE));
      regcache_cooked_write_unsigned (regcache, regnum, regval);
      len -= partial_len;
      buf += partial_len;
      regnum++;
    }
}

/* Attempt to marshall a value in a V register.  Return 1 if
   successful, or 0 if insufficient registers are available.  This
   function, unlike the equivalent pass_in_x() function does not
   handle arguments spread across multiple registers.  */

static int
pass_in_v (struct gdbarch *gdbarch,
	   struct regcache *regcache,
	   struct aarch64_call_info *info,
	   const bfd_byte *buf)
{
  if (info->nsrn < 8)
    {
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      int regnum = AARCH64_V0_REGNUM + info->nsrn;

      info->argnum++;
      info->nsrn++;

      regcache_cooked_write (regcache, regnum, buf);
      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog, "arg %d in %s\n",
			    info->argnum,
			    gdbarch_register_name (gdbarch, regnum));
      return 1;
    }
  info->nsrn = 8;
  return 0;
}

/* Marshall an argument onto the stack.  */

static void
pass_on_stack (struct aarch64_call_info *info, struct type *type,
	       const bfd_byte *buf)
{
  int len = TYPE_LENGTH (type);
  int align;
  stack_item_t item;

  info->argnum++;

  align = aarch64_type_align (type);

  /* PCS C.17 Stack should be aligned to the larger of 8 bytes or the
     Natural alignment of the argument's type.  */
  align = align_up (align, 8);

  /* The AArch64 PCS requires at most doubleword alignment.  */
  if (align > 16)
    align = 16;

  if (aarch64_debug)
    fprintf_unfiltered (gdb_stdlog, "arg %d len=%d @ sp + %d\n",
			info->argnum, len, info->nsaa);

  item.len = len;
  item.data = buf;
  VEC_safe_push (stack_item_t, info->si, &item);

  info->nsaa += len;
  if (info->nsaa & (align - 1))
    {
      /* Push stack alignment padding.  */
      int pad = align - (info->nsaa & (align - 1));

      item.len = pad;
      item.data = buf;

      VEC_safe_push (stack_item_t, info->si, &item);
      info->nsaa += pad;
    }
}

/* Marshall an argument into a sequence of one or more consecutive X
   registers or, if insufficient X registers are available then onto
   the stack.  */

static void
pass_in_x_or_stack (struct gdbarch *gdbarch, struct regcache *regcache,
		    struct aarch64_call_info *info, struct type *type,
		    const bfd_byte *buf)
{
  int len = TYPE_LENGTH (type);
  int nregs = (len + X_REGISTER_SIZE - 1) / X_REGISTER_SIZE;

  /* PCS C.13 - Pass in registers if we have enough spare */
  if (info->ngrn + nregs <= 8)
    {
      pass_in_x (gdbarch, regcache, info, type, buf);
      info->ngrn += nregs;
    }
  else
    {
      info->ngrn = 8;
      pass_on_stack (info, type, buf);
    }
}

/* Pass a value in a V register, or on the stack if insufficient are
   available.  */

static void
pass_in_v_or_stack (struct gdbarch *gdbarch,
		    struct regcache *regcache,
		    struct aarch64_call_info *info,
		    struct type *type,
		    const bfd_byte *buf)
{
  if (!pass_in_v (gdbarch, regcache, info, buf))
    pass_on_stack (info, type, buf);
}

/* Implement the "push_dummy_call" gdbarch method.  */

static CORE_ADDR
aarch64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			 struct regcache *regcache, CORE_ADDR bp_addr,
			 int nargs,
			 struct value **args, CORE_ADDR sp, int struct_return,
			 CORE_ADDR struct_addr)
{
  int nstack = 0;
  int argnum;
  int x_argreg;
  int v_argreg;
  struct aarch64_call_info info;
  struct type *func_type;
  struct type *return_type;
  int lang_struct_return;

  memset (&info, 0, sizeof (info));

  /* We need to know what the type of the called function is in order
     to determine the number of named/anonymous arguments for the
     actual argument placement, and the return type in order to handle
     return value correctly.

     The generic code above us views the decision of return in memory
     or return in registers as a two stage processes.  The language
     handler is consulted first and may decide to return in memory (eg
     class with copy constructor returned by value), this will cause
     the generic code to allocate space AND insert an initial leading
     argument.

     If the language code does not decide to pass in memory then the
     target code is consulted.

     If the language code decides to pass in memory we want to move
     the pointer inserted as the initial argument from the argument
     list and into X8, the conventional AArch64 struct return pointer
     register.

     This is slightly awkward, ideally the flag "lang_struct_return"
     would be passed to the targets implementation of push_dummy_call.
     Rather that change the target interface we call the language code
     directly ourselves.  */

  func_type = check_typedef (value_type (function));

  /* Dereference function pointer types.  */
  if (TYPE_CODE (func_type) == TYPE_CODE_PTR)
    func_type = TYPE_TARGET_TYPE (func_type);

  gdb_assert (TYPE_CODE (func_type) == TYPE_CODE_FUNC
	      || TYPE_CODE (func_type) == TYPE_CODE_METHOD);

  /* If language_pass_by_reference () returned true we will have been
     given an additional initial argument, a hidden pointer to the
     return slot in memory.  */
  return_type = TYPE_TARGET_TYPE (func_type);
  lang_struct_return = language_pass_by_reference (return_type);

  /* Set the return address.  For the AArch64, the return breakpoint
     is always at BP_ADDR.  */
  regcache_cooked_write_unsigned (regcache, AARCH64_LR_REGNUM, bp_addr);

  /* If we were given an initial argument for the return slot because
     lang_struct_return was true, lose it.  */
  if (lang_struct_return)
    {
      args++;
      nargs--;
    }

  /* The struct_return pointer occupies X8.  */
  if (struct_return || lang_struct_return)
    {
      if (aarch64_debug)
	fprintf_unfiltered (gdb_stdlog, "struct return in %s = 0x%s\n",
			    gdbarch_register_name
			    (gdbarch,
			     AARCH64_STRUCT_RETURN_REGNUM),
			    paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, AARCH64_STRUCT_RETURN_REGNUM,
				      struct_addr);
    }

  for (argnum = 0; argnum < nargs; argnum++)
    {
      struct value *arg = args[argnum];
      struct type *arg_type;
      int len;

      arg_type = check_typedef (value_type (arg));
      len = TYPE_LENGTH (arg_type);

      switch (TYPE_CODE (arg_type))
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_ENUM:
	  if (len < 4)
	    {
	      /* Promote to 32 bit integer.  */
	      if (TYPE_UNSIGNED (arg_type))
		arg_type = builtin_type (gdbarch)->builtin_uint32;
	      else
		arg_type = builtin_type (gdbarch)->builtin_int32;
	      arg = value_cast (arg_type, arg);
	    }
	  pass_in_x_or_stack (gdbarch, regcache, &info, arg_type,
			      value_contents (arg));
	  break;

	case TYPE_CODE_COMPLEX:
	  if (info.nsrn <= 6)
	    {
	      const bfd_byte *buf = value_contents (arg);
	      struct type *target_type =
		check_typedef (TYPE_TARGET_TYPE (arg_type));

	      pass_in_v (gdbarch, regcache, &info, buf);
	      pass_in_v (gdbarch, regcache, &info,
			 buf + TYPE_LENGTH (target_type));
	    }
	  else
	    {
	      info.nsrn = 8;
	      pass_on_stack (&info, arg_type, value_contents (arg));
	    }
	  break;
	case TYPE_CODE_FLT:
	  pass_in_v_or_stack (gdbarch, regcache, &info, arg_type,
			      value_contents (arg));
	  break;

	case TYPE_CODE_STRUCT:
	case TYPE_CODE_ARRAY:
	case TYPE_CODE_UNION:
	  if (is_hfa (arg_type))
	    {
	      int elements = TYPE_NFIELDS (arg_type);

	      /* Homogeneous Aggregates */
	      if (info.nsrn + elements < 8)
		{
		  int i;

		  for (i = 0; i < elements; i++)
		    {
		      /* We know that we have sufficient registers
			 available therefore this will never fallback
			 to the stack.  */
		      struct value *field =
			value_primitive_field (arg, 0, i, arg_type);
		      struct type *field_type =
			check_typedef (value_type (field));

		      pass_in_v_or_stack (gdbarch, regcache, &info, field_type,
					  value_contents_writeable (field));
		    }
		}
	      else
		{
		  info.nsrn = 8;
		  pass_on_stack (&info, arg_type, value_contents (arg));
		}
	    }
	  else if (len > 16)
	    {
	      /* PCS B.7 Aggregates larger than 16 bytes are passed by
		 invisible reference.  */

	      /* Allocate aligned storage.  */
	      sp = align_down (sp - len, 16);

	      /* Write the real data into the stack.  */
	      write_memory (sp, value_contents (arg), len);

	      /* Construct the indirection.  */
	      arg_type = lookup_pointer_type (arg_type);
	      arg = value_from_pointer (arg_type, sp);
	      pass_in_x_or_stack (gdbarch, regcache, &info, arg_type,
				  value_contents (arg));
	    }
	  else
	    /* PCS C.15 / C.18 multiple values pass.  */
	    pass_in_x_or_stack (gdbarch, regcache, &info, arg_type,
				value_contents (arg));
	  break;

	default:
	  pass_in_x_or_stack (gdbarch, regcache, &info, arg_type,
			      value_contents (arg));
	  break;
	}
    }

  /* Make sure stack retains 16 byte alignment.  */
  if (info.nsaa & 15)
    sp -= 16 - (info.nsaa & 15);

  while (!VEC_empty (stack_item_t, info.si))
    {
      stack_item_t *si = VEC_last (stack_item_t, info.si);

      sp -= si->len;
      write_memory (sp, si->data, si->len);
      VEC_pop (stack_item_t, info.si);
    }

  VEC_free (stack_item_t, info.si);

  /* Finally, update the SP register.  */
  regcache_cooked_write_unsigned (regcache, AARCH64_SP_REGNUM, sp);

  return sp;
}

/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
aarch64_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Align the stack to sixteen bytes.  */
  return sp & ~(CORE_ADDR) 15;
}

/* Return the type for an AdvSISD Q register.  */

static struct type *
aarch64_vnq_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (tdep->vnq_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnq",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_uint128;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int128;
      append_composite_type_field (t, "s", elem);

      tdep->vnq_type = t;
    }

  return tdep->vnq_type;
}

/* Return the type for an AdvSISD D register.  */

static struct type *
aarch64_vnd_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (tdep->vnd_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnd",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_double;
      append_composite_type_field (t, "f", elem);

      elem = builtin_type (gdbarch)->builtin_uint64;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int64;
      append_composite_type_field (t, "s", elem);

      tdep->vnd_type = t;
    }

  return tdep->vnd_type;
}

/* Return the type for an AdvSISD S register.  */

static struct type *
aarch64_vns_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (tdep->vns_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vns",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_float;
      append_composite_type_field (t, "f", elem);

      elem = builtin_type (gdbarch)->builtin_uint32;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int32;
      append_composite_type_field (t, "s", elem);

      tdep->vns_type = t;
    }

  return tdep->vns_type;
}

/* Return the type for an AdvSISD H register.  */

static struct type *
aarch64_vnh_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (tdep->vnh_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnh",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_uint16;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int16;
      append_composite_type_field (t, "s", elem);

      tdep->vnh_type = t;
    }

  return tdep->vnh_type;
}

/* Return the type for an AdvSISD B register.  */

static struct type *
aarch64_vnb_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (tdep->vnb_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnb",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_uint8;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int8;
      append_composite_type_field (t, "s", elem);

      tdep->vnb_type = t;
    }

  return tdep->vnb_type;
}

/* Implement the "dwarf2_reg_to_regnum" gdbarch method.  */

static int
aarch64_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (reg >= AARCH64_DWARF_X0 && reg <= AARCH64_DWARF_X0 + 30)
    return AARCH64_X0_REGNUM + reg - AARCH64_DWARF_X0;

  if (reg == AARCH64_DWARF_SP)
    return AARCH64_SP_REGNUM;

  if (reg >= AARCH64_DWARF_V0 && reg <= AARCH64_DWARF_V0 + 31)
    return AARCH64_V0_REGNUM + reg - AARCH64_DWARF_V0;

  return -1;
}


/* Implement the "print_insn" gdbarch method.  */

static int
aarch64_gdb_print_insn (bfd_vma memaddr, disassemble_info *info)
{
  info->symbols = NULL;
  return print_insn_aarch64 (memaddr, info);
}

/* AArch64 BRK software debug mode instruction.
   Note that AArch64 code is always little-endian.
   1101.0100.0010.0000.0000.0000.0000.0000 = 0xd4200000.  */
static const char aarch64_default_breakpoint[] = {0x00, 0x00, 0x20, 0xd4};

/* Implement the "breakpoint_from_pc" gdbarch method.  */

static const unsigned char *
aarch64_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr,
			    int *lenptr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  *lenptr = sizeof (aarch64_default_breakpoint);
  return aarch64_default_breakpoint;
}

/* Extract from an array REGS containing the (raw) register state a
   function return value of type TYPE, and copy that, in virtual
   format, into VALBUF.  */

static void
aarch64_extract_return_value (struct type *type, struct regcache *regs,
			      gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = get_regcache_arch (regs);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      bfd_byte buf[V_REGISTER_SIZE];
      int len = TYPE_LENGTH (type);

      regcache_cooked_read (regs, AARCH64_V0_REGNUM, buf);
      memcpy (valbuf, buf, len);
    }
  else if (TYPE_CODE (type) == TYPE_CODE_INT
	   || TYPE_CODE (type) == TYPE_CODE_CHAR
	   || TYPE_CODE (type) == TYPE_CODE_BOOL
	   || TYPE_CODE (type) == TYPE_CODE_PTR
	   || TYPE_CODE (type) == TYPE_CODE_REF
	   || TYPE_CODE (type) == TYPE_CODE_ENUM)
    {
      /* If the the type is a plain integer, then the access is
	 straight-forward.  Otherwise we have to play around a bit
	 more.  */
      int len = TYPE_LENGTH (type);
      int regno = AARCH64_X0_REGNUM;
      ULONGEST tmp;

      while (len > 0)
	{
	  /* By using store_unsigned_integer we avoid having to do
	     anything special for small big-endian values.  */
	  regcache_cooked_read_unsigned (regs, regno++, &tmp);
	  store_unsigned_integer (valbuf,
				  (len > X_REGISTER_SIZE
				   ? X_REGISTER_SIZE : len), byte_order, tmp);
	  len -= X_REGISTER_SIZE;
	  valbuf += X_REGISTER_SIZE;
	}
    }
  else if (TYPE_CODE (type) == TYPE_CODE_COMPLEX)
    {
      int regno = AARCH64_V0_REGNUM;
      bfd_byte buf[V_REGISTER_SIZE];
      struct type *target_type = check_typedef (TYPE_TARGET_TYPE (type));
      int len = TYPE_LENGTH (target_type);

      regcache_cooked_read (regs, regno, buf);
      memcpy (valbuf, buf, len);
      valbuf += len;
      regcache_cooked_read (regs, regno + 1, buf);
      memcpy (valbuf, buf, len);
      valbuf += len;
    }
  else if (is_hfa (type))
    {
      int elements = TYPE_NFIELDS (type);
      struct type *member_type = check_typedef (TYPE_FIELD_TYPE (type, 0));
      int len = TYPE_LENGTH (member_type);
      int i;

      for (i = 0; i < elements; i++)
	{
	  int regno = AARCH64_V0_REGNUM + i;
	  bfd_byte buf[X_REGISTER_SIZE];

	  if (aarch64_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"read HFA return value element %d from %s\n",
				i + 1,
				gdbarch_register_name (gdbarch, regno));
	  regcache_cooked_read (regs, regno, buf);

	  memcpy (valbuf, buf, len);
	  valbuf += len;
	}
    }
  else
    {
      /* For a structure or union the behaviour is as if the value had
         been stored to word-aligned memory and then loaded into
         registers with 64-bit load instruction(s).  */
      int len = TYPE_LENGTH (type);
      int regno = AARCH64_X0_REGNUM;
      bfd_byte buf[X_REGISTER_SIZE];

      while (len > 0)
	{
	  regcache_cooked_read (regs, regno++, buf);
	  memcpy (valbuf, buf, len > X_REGISTER_SIZE ? X_REGISTER_SIZE : len);
	  len -= X_REGISTER_SIZE;
	  valbuf += X_REGISTER_SIZE;
	}
    }
}


/* Will a function return an aggregate type in memory or in a
   register?  Return 0 if an aggregate type can be returned in a
   register, 1 if it must be returned in memory.  */

static int
aarch64_return_in_memory (struct gdbarch *gdbarch, struct type *type)
{
  int nRc;
  enum type_code code;

  CHECK_TYPEDEF (type);

  /* In the AArch64 ABI, "integer" like aggregate types are returned
     in registers.  For an aggregate type to be integer like, its size
     must be less than or equal to 4 * X_REGISTER_SIZE.  */

  if (is_hfa (type))
    {
      /* PCS B.5 If the argument is a Named HFA, then the argument is
         used unmodified.  */
      return 0;
    }

  if (TYPE_LENGTH (type) > 16)
    {
      /* PCS B.6 Aggregates larger than 16 bytes are passed by
         invisible reference.  */

      return 1;
    }

  return 0;
}

/* Write into appropriate registers a function return value of type
   TYPE, given in virtual format.  */

static void
aarch64_store_return_value (struct type *type, struct regcache *regs,
			    const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = get_regcache_arch (regs);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      bfd_byte buf[V_REGISTER_SIZE];
      int len = TYPE_LENGTH (type);

      memcpy (buf, valbuf, len > V_REGISTER_SIZE ? V_REGISTER_SIZE : len);
      regcache_cooked_write (regs, AARCH64_V0_REGNUM, buf);
    }
  else if (TYPE_CODE (type) == TYPE_CODE_INT
	   || TYPE_CODE (type) == TYPE_CODE_CHAR
	   || TYPE_CODE (type) == TYPE_CODE_BOOL
	   || TYPE_CODE (type) == TYPE_CODE_PTR
	   || TYPE_CODE (type) == TYPE_CODE_REF
	   || TYPE_CODE (type) == TYPE_CODE_ENUM)
    {
      if (TYPE_LENGTH (type) <= X_REGISTER_SIZE)
	{
	  /* Values of one word or less are zero/sign-extended and
	     returned in r0.  */
	  bfd_byte tmpbuf[X_REGISTER_SIZE];
	  LONGEST val = unpack_long (type, valbuf);

	  store_signed_integer (tmpbuf, X_REGISTER_SIZE, byte_order, val);
	  regcache_cooked_write (regs, AARCH64_X0_REGNUM, tmpbuf);
	}
      else
	{
	  /* Integral values greater than one word are stored in
	     consecutive registers starting with r0.  This will always
	     be a multiple of the regiser size.  */
	  int len = TYPE_LENGTH (type);
	  int regno = AARCH64_X0_REGNUM;

	  while (len > 0)
	    {
	      regcache_cooked_write (regs, regno++, valbuf);
	      len -= X_REGISTER_SIZE;
	      valbuf += X_REGISTER_SIZE;
	    }
	}
    }
  else if (is_hfa (type))
    {
      int elements = TYPE_NFIELDS (type);
      struct type *member_type = check_typedef (TYPE_FIELD_TYPE (type, 0));
      int len = TYPE_LENGTH (member_type);
      int i;

      for (i = 0; i < elements; i++)
	{
	  int regno = AARCH64_V0_REGNUM + i;
	  bfd_byte tmpbuf[MAX_REGISTER_SIZE];

	  if (aarch64_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"write HFA return value element %d to %s\n",
				i + 1,
				gdbarch_register_name (gdbarch, regno));

	  memcpy (tmpbuf, valbuf, len);
	  regcache_cooked_write (regs, regno, tmpbuf);
	  valbuf += len;
	}
    }
  else
    {
      /* For a structure or union the behaviour is as if the value had
	 been stored to word-aligned memory and then loaded into
	 registers with 64-bit load instruction(s).  */
      int len = TYPE_LENGTH (type);
      int regno = AARCH64_X0_REGNUM;
      bfd_byte tmpbuf[X_REGISTER_SIZE];

      while (len > 0)
	{
	  memcpy (tmpbuf, valbuf,
		  len > X_REGISTER_SIZE ? X_REGISTER_SIZE : len);
	  regcache_cooked_write (regs, regno++, tmpbuf);
	  len -= X_REGISTER_SIZE;
	  valbuf += X_REGISTER_SIZE;
	}
    }
}

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
aarch64_return_value (struct gdbarch *gdbarch, struct value *func_value,
		      struct type *valtype, struct regcache *regcache,
		      gdb_byte *readbuf, const gdb_byte *writebuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (TYPE_CODE (valtype) == TYPE_CODE_STRUCT
      || TYPE_CODE (valtype) == TYPE_CODE_UNION
      || TYPE_CODE (valtype) == TYPE_CODE_ARRAY)
    {
      if (aarch64_return_in_memory (gdbarch, valtype))
	{
	  if (aarch64_debug)
	    fprintf_unfiltered (gdb_stdlog, "return value in memory\n");
	  return RETURN_VALUE_STRUCT_CONVENTION;
	}
    }

  if (writebuf)
    aarch64_store_return_value (valtype, regcache, writebuf);

  if (readbuf)
    aarch64_extract_return_value (valtype, regcache, readbuf);

  if (aarch64_debug)
    fprintf_unfiltered (gdb_stdlog, "return value in registers\n");

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Implement the "get_longjmp_target" gdbarch method.  */

static int
aarch64_get_longjmp_target (struct frame_info *frame, CORE_ADDR *pc)
{
  CORE_ADDR jb_addr;
  gdb_byte buf[X_REGISTER_SIZE];
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  jb_addr = get_frame_register_unsigned (frame, AARCH64_X0_REGNUM);

  if (target_read_memory (jb_addr + tdep->jb_pc * tdep->jb_elt_size, buf,
			  X_REGISTER_SIZE))
    return 0;

  *pc = extract_unsigned_integer (buf, X_REGISTER_SIZE, byte_order);
  return 1;
}


/* Return the pseudo register name corresponding to register regnum.  */

static const char *
aarch64_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  static const char *const q_name[] =
    {
      "q0", "q1", "q2", "q3",
      "q4", "q5", "q6", "q7",
      "q8", "q9", "q10", "q11",
      "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19",
      "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27",
      "q28", "q29", "q30", "q31",
    };

  static const char *const d_name[] =
    {
      "d0", "d1", "d2", "d3",
      "d4", "d5", "d6", "d7",
      "d8", "d9", "d10", "d11",
      "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19",
      "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27",
      "d28", "d29", "d30", "d31",
    };

  static const char *const s_name[] =
    {
      "s0", "s1", "s2", "s3",
      "s4", "s5", "s6", "s7",
      "s8", "s9", "s10", "s11",
      "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19",
      "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27",
      "s28", "s29", "s30", "s31",
    };

  static const char *const h_name[] =
    {
      "h0", "h1", "h2", "h3",
      "h4", "h5", "h6", "h7",
      "h8", "h9", "h10", "h11",
      "h12", "h13", "h14", "h15",
      "h16", "h17", "h18", "h19",
      "h20", "h21", "h22", "h23",
      "h24", "h25", "h26", "h27",
      "h28", "h29", "h30", "h31",
    };

  static const char *const b_name[] =
    {
      "b0", "b1", "b2", "b3",
      "b4", "b5", "b6", "b7",
      "b8", "b9", "b10", "b11",
      "b12", "b13", "b14", "b15",
      "b16", "b17", "b18", "b19",
      "b20", "b21", "b22", "b23",
      "b24", "b25", "b26", "b27",
      "b28", "b29", "b30", "b31",
    };

  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum >= AARCH64_Q0_REGNUM && regnum < AARCH64_Q0_REGNUM + 32)
    return q_name[regnum - AARCH64_Q0_REGNUM];

  if (regnum >= AARCH64_D0_REGNUM && regnum < AARCH64_D0_REGNUM + 32)
    return d_name[regnum - AARCH64_D0_REGNUM];

  if (regnum >= AARCH64_S0_REGNUM && regnum < AARCH64_S0_REGNUM + 32)
    return s_name[regnum - AARCH64_S0_REGNUM];

  if (regnum >= AARCH64_H0_REGNUM && regnum < AARCH64_H0_REGNUM + 32)
    return h_name[regnum - AARCH64_H0_REGNUM];

  if (regnum >= AARCH64_B0_REGNUM && regnum < AARCH64_B0_REGNUM + 32)
    return b_name[regnum - AARCH64_B0_REGNUM];

  internal_error (__FILE__, __LINE__,
		  _("aarch64_pseudo_register_name: bad register number %d"),
		  regnum);
}

/* Implement the "pseudo_register_type" tdesc_arch_data method.  */

static struct type *
aarch64_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum >= AARCH64_Q0_REGNUM && regnum < AARCH64_Q0_REGNUM + 32)
    return aarch64_vnq_type (gdbarch);

  if (regnum >= AARCH64_D0_REGNUM && regnum < AARCH64_D0_REGNUM + 32)
    return aarch64_vnd_type (gdbarch);

  if (regnum >= AARCH64_S0_REGNUM && regnum < AARCH64_S0_REGNUM + 32)
    return aarch64_vns_type (gdbarch);

  if (regnum >= AARCH64_H0_REGNUM && regnum < AARCH64_H0_REGNUM + 32)
    return aarch64_vnh_type (gdbarch);

  if (regnum >= AARCH64_B0_REGNUM && regnum < AARCH64_B0_REGNUM + 32)
    return aarch64_vnb_type (gdbarch);

  internal_error (__FILE__, __LINE__,
		  _("aarch64_pseudo_register_type: bad register number %d"),
		  regnum);
}

/* Implement the "pseudo_register_reggroup_p" tdesc_arch_data method.  */

static int
aarch64_pseudo_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				    struct reggroup *group)
{
  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum >= AARCH64_Q0_REGNUM && regnum < AARCH64_Q0_REGNUM + 32)
    return group == all_reggroup || group == vector_reggroup;
  else if (regnum >= AARCH64_D0_REGNUM && regnum < AARCH64_D0_REGNUM + 32)
    return (group == all_reggroup || group == vector_reggroup
	    || group == float_reggroup);
  else if (regnum >= AARCH64_S0_REGNUM && regnum < AARCH64_S0_REGNUM + 32)
    return (group == all_reggroup || group == vector_reggroup
	    || group == float_reggroup);
  else if (regnum >= AARCH64_H0_REGNUM && regnum < AARCH64_H0_REGNUM + 32)
    return group == all_reggroup || group == vector_reggroup;
  else if (regnum >= AARCH64_B0_REGNUM && regnum < AARCH64_B0_REGNUM + 32)
    return group == all_reggroup || group == vector_reggroup;

  return group == all_reggroup;
}

/* Implement the "pseudo_register_read_value" gdbarch method.  */

static struct value *
aarch64_pseudo_read_value (struct gdbarch *gdbarch,
			   struct regcache *regcache,
			   int regnum)
{
  gdb_byte reg_buf[MAX_REGISTER_SIZE];
  struct value *result_value;
  gdb_byte *buf;

  result_value = allocate_value (register_type (gdbarch, regnum));
  VALUE_LVAL (result_value) = lval_register;
  VALUE_REGNUM (result_value) = regnum;
  buf = value_contents_raw (result_value);

  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum >= AARCH64_Q0_REGNUM && regnum < AARCH64_Q0_REGNUM + 32)
    {
      enum register_status status;
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_Q0_REGNUM;
      status = regcache_raw_read (regcache, v_regnum, reg_buf);
      if (status != REG_VALID)
	mark_value_bytes_unavailable (result_value, 0,
				      TYPE_LENGTH (value_type (result_value)));
      else
	memcpy (buf, reg_buf, Q_REGISTER_SIZE);
      return result_value;
    }

  if (regnum >= AARCH64_D0_REGNUM && regnum < AARCH64_D0_REGNUM + 32)
    {
      enum register_status status;
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_D0_REGNUM;
      status = regcache_raw_read (regcache, v_regnum, reg_buf);
      if (status != REG_VALID)
	mark_value_bytes_unavailable (result_value, 0,
				      TYPE_LENGTH (value_type (result_value)));
      else
	memcpy (buf, reg_buf, D_REGISTER_SIZE);
      return result_value;
    }

  if (regnum >= AARCH64_S0_REGNUM && regnum < AARCH64_S0_REGNUM + 32)
    {
      enum register_status status;
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_S0_REGNUM;
      status = regcache_raw_read (regcache, v_regnum, reg_buf);
      memcpy (buf, reg_buf, S_REGISTER_SIZE);
      return result_value;
    }

  if (regnum >= AARCH64_H0_REGNUM && regnum < AARCH64_H0_REGNUM + 32)
    {
      enum register_status status;
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_H0_REGNUM;
      status = regcache_raw_read (regcache, v_regnum, reg_buf);
      if (status != REG_VALID)
	mark_value_bytes_unavailable (result_value, 0,
				      TYPE_LENGTH (value_type (result_value)));
      else
	memcpy (buf, reg_buf, H_REGISTER_SIZE);
      return result_value;
    }

  if (regnum >= AARCH64_B0_REGNUM && regnum < AARCH64_B0_REGNUM + 32)
    {
      enum register_status status;
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_B0_REGNUM;
      status = regcache_raw_read (regcache, v_regnum, reg_buf);
      if (status != REG_VALID)
	mark_value_bytes_unavailable (result_value, 0,
				      TYPE_LENGTH (value_type (result_value)));
      else
	memcpy (buf, reg_buf, B_REGISTER_SIZE);
      return result_value;
    }

  gdb_assert_not_reached ("regnum out of bound");
}

/* Implement the "pseudo_register_write" gdbarch method.  */

static void
aarch64_pseudo_write (struct gdbarch *gdbarch, struct regcache *regcache,
		      int regnum, const gdb_byte *buf)
{
  gdb_byte reg_buf[MAX_REGISTER_SIZE];

  /* Ensure the register buffer is zero, we want gdb writes of the
     various 'scalar' pseudo registers to behavior like architectural
     writes, register width bytes are written the remainder are set to
     zero.  */
  memset (reg_buf, 0, sizeof (reg_buf));

  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum >= AARCH64_Q0_REGNUM && regnum < AARCH64_Q0_REGNUM + 32)
    {
      /* pseudo Q registers */
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_Q0_REGNUM;
      memcpy (reg_buf, buf, Q_REGISTER_SIZE);
      regcache_raw_write (regcache, v_regnum, reg_buf);
      return;
    }

  if (regnum >= AARCH64_D0_REGNUM && regnum < AARCH64_D0_REGNUM + 32)
    {
      /* pseudo D registers */
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_D0_REGNUM;
      memcpy (reg_buf, buf, D_REGISTER_SIZE);
      regcache_raw_write (regcache, v_regnum, reg_buf);
      return;
    }

  if (regnum >= AARCH64_S0_REGNUM && regnum < AARCH64_S0_REGNUM + 32)
    {
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_S0_REGNUM;
      memcpy (reg_buf, buf, S_REGISTER_SIZE);
      regcache_raw_write (regcache, v_regnum, reg_buf);
      return;
    }

  if (regnum >= AARCH64_H0_REGNUM && regnum < AARCH64_H0_REGNUM + 32)
    {
      /* pseudo H registers */
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_H0_REGNUM;
      memcpy (reg_buf, buf, H_REGISTER_SIZE);
      regcache_raw_write (regcache, v_regnum, reg_buf);
      return;
    }

  if (regnum >= AARCH64_B0_REGNUM && regnum < AARCH64_B0_REGNUM + 32)
    {
      /* pseudo B registers */
      unsigned v_regnum;

      v_regnum = AARCH64_V0_REGNUM + regnum - AARCH64_B0_REGNUM;
      memcpy (reg_buf, buf, B_REGISTER_SIZE);
      regcache_raw_write (regcache, v_regnum, reg_buf);
      return;
    }

  gdb_assert_not_reached ("regnum out of bound");
}

/* Implement the "write_pc" gdbarch method.  */

static void
aarch64_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  regcache_cooked_write_unsigned (regcache, AARCH64_PC_REGNUM, pc);
}

/* Callback function for user_reg_add.  */

static struct value *
value_of_aarch64_user_reg (struct frame_info *frame, const void *baton)
{
  const int *reg_p = baton;

  return value_of_register (*reg_p, frame);
}


/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called e.g. at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
aarch64_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch_tdep *tdep;
  struct gdbarch *gdbarch;
  struct gdbarch_list *best_arch;
  struct tdesc_arch_data *tdesc_data = NULL;
  const struct target_desc *tdesc = info.target_desc;
  int i;
  int have_fpa_registers = 1;
  int valid_p = 1;
  const struct tdesc_feature *feature;
  int num_regs = 0;
  int num_pseudo_regs = 0;

  /* Ensure we always have a target descriptor.  */
  if (!tdesc_has_registers (tdesc))
    tdesc = tdesc_aarch64;

  gdb_assert (tdesc);

  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.core");

  if (feature == NULL)
    return NULL;

  tdesc_data = tdesc_data_alloc ();

  /* Validate the descriptor provides the mandatory core R registers
     and allocate their numbers.  */
  for (i = 0; i < ARRAY_SIZE (aarch64_r_register_names); i++)
    valid_p &=
      tdesc_numbered_register (feature, tdesc_data, AARCH64_X0_REGNUM + i,
			       aarch64_r_register_names[i]);

  num_regs = AARCH64_X0_REGNUM + i;

  /* Look for the V registers.  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.fpu");
  if (feature)
    {
      /* Validate the descriptor provides the mandatory V registers
         and allocate their numbers.  */
      for (i = 0; i < ARRAY_SIZE (aarch64_v_register_names); i++)
	valid_p &=
	  tdesc_numbered_register (feature, tdesc_data, AARCH64_V0_REGNUM + i,
				   aarch64_v_register_names[i]);

      num_regs = AARCH64_V0_REGNUM + i;

      num_pseudo_regs += 32;	/* add the Qn scalar register pseudos */
      num_pseudo_regs += 32;	/* add the Dn scalar register pseudos */
      num_pseudo_regs += 32;	/* add the Sn scalar register pseudos */
      num_pseudo_regs += 32;	/* add the Hn scalar register pseudos */
      num_pseudo_regs += 32;	/* add the Bn scalar register pseudos */
    }

  if (!valid_p)
    {
      tdesc_data_cleanup (tdesc_data);
      return NULL;
    }

  /* AArch64 code is always little-endian.  */
  info.byte_order_for_code = BFD_ENDIAN_LITTLE;

  /* If there is already a candidate, use it.  */
  for (best_arch = gdbarch_list_lookup_by_info (arches, &info);
       best_arch != NULL;
       best_arch = gdbarch_list_lookup_by_info (best_arch->next, &info))
    {
      /* Found a match.  */
      break;
    }

  if (best_arch != NULL)
    {
      if (tdesc_data != NULL)
	tdesc_data_cleanup (tdesc_data);
      return best_arch->gdbarch;
    }

  tdep = xcalloc (1, sizeof (struct gdbarch_tdep));
  gdbarch = gdbarch_alloc (&info, tdep);

  /* This should be low enough for everything.  */
  tdep->lowest_pc = 0x20;
  tdep->jb_pc = -1;		/* Longjump support not enabled by default.  */
  tdep->jb_elt_size = 8;

  set_gdbarch_push_dummy_call (gdbarch, aarch64_push_dummy_call);
  set_gdbarch_frame_align (gdbarch, aarch64_frame_align);

  set_gdbarch_write_pc (gdbarch, aarch64_write_pc);

  /* Frame handling.  */
  set_gdbarch_dummy_id (gdbarch, aarch64_dummy_id);
  set_gdbarch_unwind_pc (gdbarch, aarch64_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, aarch64_unwind_sp);

  /* Advance PC across function entry code.  */
  set_gdbarch_skip_prologue (gdbarch, aarch64_skip_prologue);

  /* The stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Breakpoint manipulation.  */
  set_gdbarch_breakpoint_from_pc (gdbarch, aarch64_breakpoint_from_pc);
  set_gdbarch_cannot_step_breakpoint (gdbarch, 1);
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  /* Information about registers, etc.  */
  set_gdbarch_sp_regnum (gdbarch, AARCH64_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, AARCH64_PC_REGNUM);
  set_gdbarch_num_regs (gdbarch, num_regs);

  set_gdbarch_num_pseudo_regs (gdbarch, num_pseudo_regs);
  set_gdbarch_pseudo_register_read_value (gdbarch, aarch64_pseudo_read_value);
  set_gdbarch_pseudo_register_write (gdbarch, aarch64_pseudo_write);
  set_tdesc_pseudo_register_name (gdbarch, aarch64_pseudo_register_name);
  set_tdesc_pseudo_register_type (gdbarch, aarch64_pseudo_register_type);
  set_tdesc_pseudo_register_reggroup_p (gdbarch,
					aarch64_pseudo_register_reggroup_p);

  /* ABI */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_bit (gdbarch, 64);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 64);
  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
  set_gdbarch_long_double_format (gdbarch, floatformats_ia64_quad);

  /* Internal <-> external register number maps.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, aarch64_dwarf_reg_to_regnum);

  /* Returning results.  */
  set_gdbarch_return_value (gdbarch, aarch64_return_value);

  /* Disassembly.  */
  set_gdbarch_print_insn (gdbarch, aarch64_gdb_print_insn);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 1);

  /* Hook in the ABI-specific overrides, if they have been registered.  */
  info.target_desc = tdesc;
  info.tdep_info = (void *) tdesc_data;
  gdbarch_init_osabi (info, gdbarch);

  dwarf2_frame_set_init_reg (gdbarch, aarch64_dwarf2_frame_init_reg);

  /* Add some default predicates.  */
  frame_unwind_append_unwinder (gdbarch, &aarch64_stub_unwind);
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &aarch64_prologue_unwind);

  frame_base_set_default (gdbarch, &aarch64_normal_base);

  /* Now we have tuned the configuration, set a few final things,
     based on what the OS ABI has told us.  */

  if (tdep->jb_pc >= 0)
    set_gdbarch_get_longjmp_target (gdbarch, aarch64_get_longjmp_target);

  tdesc_use_registers (gdbarch, tdesc, tdesc_data);

  /* Add standard register aliases.  */
  for (i = 0; i < ARRAY_SIZE (aarch64_register_aliases); i++)
    user_reg_add (gdbarch, aarch64_register_aliases[i].name,
		  value_of_aarch64_user_reg,
		  &aarch64_register_aliases[i].regnum);

  return gdbarch;
}

static void
aarch64_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (tdep == NULL)
    return;

  fprintf_unfiltered (file, _("aarch64_dump_tdep: Lowest pc = 0x%s"),
		      paddress (gdbarch, tdep->lowest_pc));
}

/* Suppress warning from -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_aarch64_tdep;

void
_initialize_aarch64_tdep (void)
{
  gdbarch_register (bfd_arch_aarch64, aarch64_gdbarch_init,
		    aarch64_dump_tdep);

  initialize_tdesc_aarch64 ();
  initialize_tdesc_aarch64_without_fpu ();

  /* Debug this file's internals.  */
  add_setshow_boolean_cmd ("aarch64", class_maintenance, &aarch64_debug, _("\
Set AArch64 debugging."), _("\
Show AArch64 debugging."), _("\
When on, AArch64 specific debugging is enabled."),
			    NULL,
			    show_aarch64_debug,
			    &setdebuglist, &showdebuglist);
}
