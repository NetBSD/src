/* Target-dependent code for Lattice Mico32 processor, for GDB.
   Contributed by Jon Beniston <jon@beniston.com>

   Copyright (C) 2009-2013 Free Software Foundation, Inc.

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
#include "frame-unwind.h"
#include "frame-base.h"
#include "inferior.h"
#include "dis-asm.h"
#include "symfile.h"
#include "remote.h"
#include "gdbcore.h"
#include "gdb/sim-lm32.h"
#include "gdb/callback.h"
#include "gdb/remote-sim.h"
#include "sim-regno.h"
#include "arch-utils.h"
#include "regcache.h"
#include "trad-frame.h"
#include "reggroups.h"
#include "opcodes/lm32-desc.h"

#include "gdb_string.h"

/* Macros to extract fields from an instruction.  */
#define LM32_OPCODE(insn)       ((insn >> 26) & 0x3f)
#define LM32_REG0(insn)         ((insn >> 21) & 0x1f)
#define LM32_REG1(insn)         ((insn >> 16) & 0x1f)
#define LM32_REG2(insn)         ((insn >> 11) & 0x1f)
#define LM32_IMM16(insn)        ((((long)insn & 0xffff) << 16) >> 16)

struct gdbarch_tdep
{
  /* gdbarch target dependent data here.  Currently unused for LM32.  */
};

struct lm32_frame_cache
{
  /* The frame's base.  Used when constructing a frame ID.  */
  CORE_ADDR base;
  CORE_ADDR pc;
  /* Size of frame.  */
  int size;
  /* Table indicating the location of each and every register.  */
  struct trad_frame_saved_reg *saved_regs;
};

/* Add the available register groups.  */

static void
lm32_add_reggroups (struct gdbarch *gdbarch)
{
  reggroup_add (gdbarch, general_reggroup);
  reggroup_add (gdbarch, all_reggroup);
  reggroup_add (gdbarch, system_reggroup);
}

/* Return whether a given register is in a given group.  */

static int
lm32_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			  struct reggroup *group)
{
  if (group == general_reggroup)
    return ((regnum >= SIM_LM32_R0_REGNUM) && (regnum <= SIM_LM32_RA_REGNUM))
      || (regnum == SIM_LM32_PC_REGNUM);
  else if (group == system_reggroup)
    return ((regnum >= SIM_LM32_EA_REGNUM) && (regnum <= SIM_LM32_BA_REGNUM))
      || ((regnum >= SIM_LM32_EID_REGNUM) && (regnum <= SIM_LM32_IP_REGNUM));
  return default_register_reggroup_p (gdbarch, regnum, group);
}

/* Return a name that corresponds to the given register number.  */

static const char *
lm32_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "gp", "fp", "sp", "ra", "ea", "ba",
    "PC", "EID", "EBA", "DEBA", "IE", "IM", "IP"
  };

  if ((reg_nr < 0) || (reg_nr >= ARRAY_SIZE (register_names)))
    return NULL;
  else
    return register_names[reg_nr];
}

/* Return type of register.  */

static struct type *
lm32_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  return builtin_type (gdbarch)->builtin_int32;
}

/* Return non-zero if a register can't be written.  */

static int
lm32_cannot_store_register (struct gdbarch *gdbarch, int regno)
{
  return (regno == SIM_LM32_R0_REGNUM) || (regno == SIM_LM32_EID_REGNUM);
}

/* Analyze a function's prologue.  */

static CORE_ADDR
lm32_analyze_prologue (struct gdbarch *gdbarch,
		       CORE_ADDR pc, CORE_ADDR limit,
		       struct lm32_frame_cache *info)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned long instruction;

  /* Keep reading though instructions, until we come across an instruction 
     that isn't likely to be part of the prologue.  */
  info->size = 0;
  for (; pc < limit; pc += 4)
    {

      /* Read an instruction.  */
      instruction = read_memory_integer (pc, 4, byte_order);

      if ((LM32_OPCODE (instruction) == OP_SW)
	  && (LM32_REG0 (instruction) == SIM_LM32_SP_REGNUM))
	{
	  /* Any stack displaced store is likely part of the prologue.
	     Record that the register is being saved, and the offset 
	     into the stack.  */
	  info->saved_regs[LM32_REG1 (instruction)].addr =
	    LM32_IMM16 (instruction);
	}
      else if ((LM32_OPCODE (instruction) == OP_ADDI)
	       && (LM32_REG1 (instruction) == SIM_LM32_SP_REGNUM))
	{
	  /* An add to the SP is likely to be part of the prologue.
	     Adjust stack size by whatever the instruction adds to the sp.  */
	  info->size -= LM32_IMM16 (instruction);
	}
      else if (			/* add fp,fp,sp */
		((LM32_OPCODE (instruction) == OP_ADD)
		 && (LM32_REG2 (instruction) == SIM_LM32_FP_REGNUM)
		 && (LM32_REG0 (instruction) == SIM_LM32_FP_REGNUM)
		 && (LM32_REG1 (instruction) == SIM_LM32_SP_REGNUM))
		/* mv fp,imm */
		|| ((LM32_OPCODE (instruction) == OP_ADDI)
		    && (LM32_REG1 (instruction) == SIM_LM32_FP_REGNUM)
		    && (LM32_REG0 (instruction) == SIM_LM32_R0_REGNUM)))
	{
	  /* Likely to be in the prologue for functions that require 
	     a frame pointer.  */
	}
      else
	{
	  /* Any other instruction is likely not to be part of the
	     prologue.  */
	  break;
	}
    }

  return pc;
}

/* Return PC of first non prologue instruction, for the function at the 
   specified address.  */

static CORE_ADDR
lm32_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, limit_pc;
  struct lm32_frame_cache frame_info;
  struct trad_frame_saved_reg saved_regs[SIM_LM32_NUM_REGS];

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
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
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 100;	/* Magic.  */

  frame_info.saved_regs = saved_regs;
  return lm32_analyze_prologue (gdbarch, pc, limit_pc, &frame_info);
}

/* Create a breakpoint instruction.  */

static const gdb_byte *
lm32_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr,
			 int *lenptr)
{
  static const gdb_byte breakpoint[4] = { OP_RAISE << 2, 0, 0, 2 };

  *lenptr = sizeof (breakpoint);
  return breakpoint;
}

/* Setup registers and stack for faking a call to a function in the 
   inferior.  */

static CORE_ADDR
lm32_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		      struct regcache *regcache, CORE_ADDR bp_addr,
		      int nargs, struct value **args, CORE_ADDR sp,
		      int struct_return, CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int first_arg_reg = SIM_LM32_R1_REGNUM;
  int num_arg_regs = 8;
  int i;

  /* Set the return address.  */
  regcache_cooked_write_signed (regcache, SIM_LM32_RA_REGNUM, bp_addr);

  /* If we're returning a large struct, a pointer to the address to
     store it at is passed as a first hidden parameter.  */
  if (struct_return)
    {
      regcache_cooked_write_unsigned (regcache, first_arg_reg, struct_addr);
      first_arg_reg++;
      num_arg_regs--;
      sp -= 4;
    }

  /* Setup parameters.  */
  for (i = 0; i < nargs; i++)
    {
      struct value *arg = args[i];
      struct type *arg_type = check_typedef (value_type (arg));
      gdb_byte *contents;
      ULONGEST val;

      /* Promote small integer types to int.  */
      switch (TYPE_CODE (arg_type))
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_ENUM:
	  if (TYPE_LENGTH (arg_type) < 4)
	    {
	      arg_type = builtin_type (gdbarch)->builtin_int32;
	      arg = value_cast (arg_type, arg);
	    }
	  break;
	}

      /* FIXME: Handle structures.  */

      contents = (gdb_byte *) value_contents (arg);
      val = extract_unsigned_integer (contents, TYPE_LENGTH (arg_type),
				      byte_order);

      /* First num_arg_regs parameters are passed by registers, 
         and the rest are passed on the stack.  */
      if (i < num_arg_regs)
	regcache_cooked_write_unsigned (regcache, first_arg_reg + i, val);
      else
	{
	  write_memory (sp, (void *) &val, TYPE_LENGTH (arg_type));
	  sp -= 4;
	}
    }

  /* Update stack pointer.  */
  regcache_cooked_write_signed (regcache, SIM_LM32_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

/* Extract return value after calling a function in the inferior.  */

static void
lm32_extract_return_value (struct type *type, struct regcache *regcache,
			   gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST l;
  CORE_ADDR return_buffer;

  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
      && TYPE_CODE (type) != TYPE_CODE_UNION
      && TYPE_CODE (type) != TYPE_CODE_ARRAY && TYPE_LENGTH (type) <= 4)
    {
      /* Return value is returned in a single register.  */
      regcache_cooked_read_unsigned (regcache, SIM_LM32_R1_REGNUM, &l);
      store_unsigned_integer (valbuf, TYPE_LENGTH (type), byte_order, l);
    }
  else if ((TYPE_CODE (type) == TYPE_CODE_INT) && (TYPE_LENGTH (type) == 8))
    {
      /* 64-bit values are returned in a register pair.  */
      regcache_cooked_read_unsigned (regcache, SIM_LM32_R1_REGNUM, &l);
      memcpy (valbuf, &l, 4);
      regcache_cooked_read_unsigned (regcache, SIM_LM32_R2_REGNUM, &l);
      memcpy (valbuf + 4, &l, 4);
    }
  else
    {
      /* Aggregate types greater than a single register are returned
         in memory.  FIXME: Unless they are only 2 regs?.  */
      regcache_cooked_read_unsigned (regcache, SIM_LM32_R1_REGNUM, &l);
      return_buffer = l;
      read_memory (return_buffer, valbuf, TYPE_LENGTH (type));
    }
}

/* Write into appropriate registers a function return value of type
   TYPE, given in virtual format.  */
static void
lm32_store_return_value (struct type *type, struct regcache *regcache,
			 const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST val;
  int len = TYPE_LENGTH (type);

  if (len <= 4)
    {
      val = extract_unsigned_integer (valbuf, len, byte_order);
      regcache_cooked_write_unsigned (regcache, SIM_LM32_R1_REGNUM, val);
    }
  else if (len <= 8)
    {
      val = extract_unsigned_integer (valbuf, 4, byte_order);
      regcache_cooked_write_unsigned (regcache, SIM_LM32_R1_REGNUM, val);
      val = extract_unsigned_integer (valbuf + 4, len - 4, byte_order);
      regcache_cooked_write_unsigned (regcache, SIM_LM32_R2_REGNUM, val);
    }
  else
    error (_("lm32_store_return_value: type length too large."));
}

/* Determine whether a functions return value is in a register or memory.  */
static enum return_value_convention
lm32_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *valtype, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum type_code code = TYPE_CODE (valtype);

  if (code == TYPE_CODE_STRUCT
      || code == TYPE_CODE_UNION
      || code == TYPE_CODE_ARRAY || TYPE_LENGTH (valtype) > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    lm32_extract_return_value (valtype, regcache, readbuf);
  if (writebuf)
    lm32_store_return_value (valtype, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

static CORE_ADDR
lm32_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  return frame_unwind_register_unsigned (next_frame, SIM_LM32_PC_REGNUM);
}

static CORE_ADDR
lm32_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  return frame_unwind_register_unsigned (next_frame, SIM_LM32_SP_REGNUM);
}

static struct frame_id
lm32_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, SIM_LM32_SP_REGNUM);

  return frame_id_build (sp, get_frame_pc (this_frame));
}

/* Put here the code to store, into fi->saved_regs, the addresses of
   the saved registers of frame described by FRAME_INFO.  This
   includes special registers such as pc and fp saved in special ways
   in the stack frame.  sp is even more special: the address we return
   for it IS the sp for the next frame.  */

static struct lm32_frame_cache *
lm32_frame_cache (struct frame_info *this_frame, void **this_prologue_cache)
{
  CORE_ADDR current_pc;
  ULONGEST prev_sp;
  ULONGEST this_base;
  struct lm32_frame_cache *info;
  int i;

  if ((*this_prologue_cache))
    return (*this_prologue_cache);

  info = FRAME_OBSTACK_ZALLOC (struct lm32_frame_cache);
  (*this_prologue_cache) = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  info->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  lm32_analyze_prologue (get_frame_arch (this_frame),
			 info->pc, current_pc, info);

  /* Compute the frame's base, and the previous frame's SP.  */
  this_base = get_frame_register_unsigned (this_frame, SIM_LM32_SP_REGNUM);
  prev_sp = this_base + info->size;
  info->base = this_base;

  /* Convert callee save offsets into addresses.  */
  for (i = 0; i < gdbarch_num_regs (get_frame_arch (this_frame)) - 1; i++)
    {
      if (trad_frame_addr_p (info->saved_regs, i))
	info->saved_regs[i].addr = this_base + info->saved_regs[i].addr;
    }

  /* The call instruction moves the caller's PC in the callee's RA register.
     Since this is an unwind, do the reverse.  Copy the location of RA register
     into PC (the address / regnum) so that a request for PC will be
     converted into a request for the RA register.  */
  info->saved_regs[SIM_LM32_PC_REGNUM] = info->saved_regs[SIM_LM32_RA_REGNUM];

  /* The previous frame's SP needed to be computed.  Save the computed
     value.  */
  trad_frame_set_value (info->saved_regs, SIM_LM32_SP_REGNUM, prev_sp);

  return info;
}

static void
lm32_frame_this_id (struct frame_info *this_frame, void **this_cache,
		    struct frame_id *this_id)
{
  struct lm32_frame_cache *cache = lm32_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
lm32_frame_prev_register (struct frame_info *this_frame,
			  void **this_prologue_cache, int regnum)
{
  struct lm32_frame_cache *info;

  info = lm32_frame_cache (this_frame, this_prologue_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static const struct frame_unwind lm32_frame_unwind = {
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  lm32_frame_this_id,
  lm32_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
lm32_frame_base_address (struct frame_info *this_frame, void **this_cache)
{
  struct lm32_frame_cache *info = lm32_frame_cache (this_frame, this_cache);

  return info->base;
}

static const struct frame_base lm32_frame_base = {
  &lm32_frame_unwind,
  lm32_frame_base_address,
  lm32_frame_base_address,
  lm32_frame_base_address
};

static CORE_ADDR
lm32_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Align to the size of an instruction (so that they can safely be
     pushed onto the stack.  */
  return sp & ~3;
}

static struct gdbarch *
lm32_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;

  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found, create a new architecture from the information provided.  */
  tdep = XMALLOC (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  /* Type sizes.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 32);

  /* Register info.  */
  set_gdbarch_num_regs (gdbarch, SIM_LM32_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, SIM_LM32_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, SIM_LM32_PC_REGNUM);
  set_gdbarch_register_name (gdbarch, lm32_register_name);
  set_gdbarch_register_type (gdbarch, lm32_register_type);
  set_gdbarch_cannot_store_register (gdbarch, lm32_cannot_store_register);

  /* Frame info.  */
  set_gdbarch_skip_prologue (gdbarch, lm32_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_decr_pc_after_break (gdbarch, 0);
  set_gdbarch_frame_args_skip (gdbarch, 0);

  /* Frame unwinding.  */
  set_gdbarch_frame_align (gdbarch, lm32_frame_align);
  frame_base_set_default (gdbarch, &lm32_frame_base);
  set_gdbarch_unwind_pc (gdbarch, lm32_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, lm32_unwind_sp);
  set_gdbarch_dummy_id (gdbarch, lm32_dummy_id);
  frame_unwind_append_unwinder (gdbarch, &lm32_frame_unwind);

  /* Breakpoints.  */
  set_gdbarch_breakpoint_from_pc (gdbarch, lm32_breakpoint_from_pc);
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  /* Calling functions in the inferior.  */
  set_gdbarch_push_dummy_call (gdbarch, lm32_push_dummy_call);
  set_gdbarch_return_value (gdbarch, lm32_return_value);

  /* Instruction disassembler.  */
  set_gdbarch_print_insn (gdbarch, print_insn_lm32);

  lm32_add_reggroups (gdbarch);
  set_gdbarch_register_reggroup_p (gdbarch, lm32_register_reggroup_p);

  return gdbarch;
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_lm32_tdep;

void
_initialize_lm32_tdep (void)
{
  register_gdbarch_init (bfd_arch_lm32, lm32_gdbarch_init);
}
