/* Common target dependent code for GDB on AArch64 systems.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.
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
#include "ax.h"
#include "ax-gdb.h"

#include "aarch64-tdep.h"

#include "elf-bfd.h"
#include "elf/aarch64.h"

#include "vec.h"

#include "record.h"
#include "record-full.h"

#include "features/aarch64.c"

#include "arch/aarch64-insn.h"

#include "opcode/aarch64.h"

#define submask(x) ((1L << ((x) + 1)) - 1)
#define bit(obj,st) (((obj) >> (st)) & 1)
#define bits(obj,st,fn) (((obj) >> (st)) & submask ((fn) - (st)))

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
  /* The program counter at the start of the function.  It is used to
     identify this frame as a prologue frame.  */
  CORE_ADDR func;

  /* The program counter at the time this frame was created; i.e. where
     this function was called from.  It is used to identify this frame as a
     stub frame.  */
  CORE_ADDR prev_pc;

  /* The stack pointer at the time this frame was created; i.e. the
     caller's stack pointer when this function was called.  It is used
     to identify this frame.  */
  CORE_ADDR prev_sp;

  /* Is the target available to read from?  */
  int available_p;

  /* The frame base for this frame is just prev_sp - frame size.
     FRAMESIZE is the distance from the frame pointer to the
     initial stack pointer.  */
  int framesize;

  /* The register used to hold the frame pointer for this frame.  */
  int framereg;

  /* Saved register offsets.  */
  struct trad_frame_saved_reg *saved_regs;
};

static void
show_aarch64_debug (struct ui_file *file, int from_tty,
                    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("AArch64 debugging is %s.\n"), value);
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
      aarch64_inst inst;

      insn = read_memory_unsigned_integer (start, 4, byte_order_for_code);

      if (aarch64_decode_insn (insn, &inst, 1) != 0)
	break;

      if (inst.opcode->iclass == addsub_imm
	  && (inst.opcode->op == OP_ADD
	      || strcmp ("sub", inst.opcode->name) == 0))
	{
	  unsigned rd = inst.operands[0].reg.regno;
	  unsigned rn = inst.operands[1].reg.regno;

	  gdb_assert (aarch64_num_of_operands (inst.opcode) == 3);
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rd_SP);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_Rn_SP);
	  gdb_assert (inst.operands[2].type == AARCH64_OPND_AIMM);

	  if (inst.opcode->op == OP_ADD)
	    {
	      regs[rd] = pv_add_constant (regs[rn],
					  inst.operands[2].imm.value);
	    }
	  else
	    {
	      regs[rd] = pv_add_constant (regs[rn],
					  -inst.operands[2].imm.value);
	    }
	}
      else if (inst.opcode->iclass == pcreladdr
	       && inst.operands[1].type == AARCH64_OPND_ADDR_ADRP)
	{
	  gdb_assert (aarch64_num_of_operands (inst.opcode) == 2);
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rd);

	  regs[inst.operands[0].reg.regno] = pv_unknown ();
	}
      else if (inst.opcode->iclass == branch_imm)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->iclass == condbranch)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->iclass == branch_reg)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->iclass == compbranch)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->op == OP_MOVZ)
	{
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rd);
	  regs[inst.operands[0].reg.regno] = pv_unknown ();
	}
      else if (inst.opcode->iclass == log_shift
	       && strcmp (inst.opcode->name, "orr") == 0)
	{
	  unsigned rd = inst.operands[0].reg.regno;
	  unsigned rn = inst.operands[1].reg.regno;
	  unsigned rm = inst.operands[2].reg.regno;

	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rd);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_Rn);
	  gdb_assert (inst.operands[2].type == AARCH64_OPND_Rm_SFT);

	  if (inst.operands[2].shifter.amount == 0
	      && rn == AARCH64_SP_REGNUM)
	    regs[rd] = regs[rm];
	  else
	    {
	      if (aarch64_debug)
		{
		  debug_printf ("aarch64: prologue analysis gave up "
				"addr=%s opcode=0x%x (orr x register)\n",
				core_addr_to_string_nz (start), insn);
		}
	      break;
	    }
	}
      else if (inst.opcode->op == OP_STUR)
	{
	  unsigned rt = inst.operands[0].reg.regno;
	  unsigned rn = inst.operands[1].addr.base_regno;
	  int is64
	    = (aarch64_get_qualifier_esize (inst.operands[0].qualifier) == 8);

	  gdb_assert (aarch64_num_of_operands (inst.opcode) == 2);
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rt);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_ADDR_SIMM9);
	  gdb_assert (!inst.operands[1].addr.offset.is_reg);

	  pv_area_store (stack, pv_add_constant (regs[rn],
						 inst.operands[1].addr.offset.imm),
			 is64 ? 8 : 4, regs[rt]);
	}
      else if ((inst.opcode->iclass == ldstpair_off
		|| (inst.opcode->iclass == ldstpair_indexed
		    && inst.operands[2].addr.preind))
	       && strcmp ("stp", inst.opcode->name) == 0)
	{
	  /* STP with addressing mode Pre-indexed and Base register.  */
	  unsigned rt1 = inst.operands[0].reg.regno;
	  unsigned rt2 = inst.operands[1].reg.regno;
	  unsigned rn = inst.operands[2].addr.base_regno;
	  int32_t imm = inst.operands[2].addr.offset.imm;

	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rt);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_Rt2);
	  gdb_assert (inst.operands[2].type == AARCH64_OPND_ADDR_SIMM7);
	  gdb_assert (!inst.operands[2].addr.offset.is_reg);

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

	  if (inst.operands[2].addr.writeback)
	    regs[rn] = pv_add_constant (regs[rn], imm);

	}
      else if (inst.opcode->iclass == testbranch)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else
	{
	  if (aarch64_debug)
	    {
	      debug_printf ("aarch64: prologue analysis gave up addr=%s"
			    " opcode=0x%x\n",
			    core_addr_to_string_nz (start), insn);
	    }
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
  CORE_ADDR func_addr, limit_pc;

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

  cache->prev_pc = prev_pc;

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

      frame_loc = get_frame_register_unsigned (this_frame, AARCH64_FP_REGNUM);
      if (frame_loc == 0)
	return;

      cache->framereg = AARCH64_FP_REGNUM;
      cache->framesize = 16;
      cache->saved_regs[29].addr = 0;
      cache->saved_regs[30].addr = 8;
    }
}

/* Fill in *CACHE with information about the prologue of *THIS_FRAME.  This
   function may throw an exception if the inferior's registers or memory is
   not available.  */

static void
aarch64_make_prologue_cache_1 (struct frame_info *this_frame,
			       struct aarch64_prologue_cache *cache)
{
  CORE_ADDR unwound_fp;
  int reg;

  aarch64_scan_prologue (this_frame, cache);

  if (cache->framereg == -1)
    return;

  unwound_fp = get_frame_register_unsigned (this_frame, cache->framereg);
  if (unwound_fp == 0)
    return;

  cache->prev_sp = unwound_fp + cache->framesize;

  /* Calculate actual addresses of saved registers using offsets
     determined by aarch64_analyze_prologue.  */
  for (reg = 0; reg < gdbarch_num_regs (get_frame_arch (this_frame)); reg++)
    if (trad_frame_addr_p (cache->saved_regs, reg))
      cache->saved_regs[reg].addr += cache->prev_sp;

  cache->func = get_frame_func (this_frame);

  cache->available_p = 1;
}

/* Allocate and fill in *THIS_CACHE with information about the prologue of
   *THIS_FRAME.  Do not do this is if *THIS_CACHE was already allocated.
   Return a pointer to the current aarch64_prologue_cache in
   *THIS_CACHE.  */

static struct aarch64_prologue_cache *
aarch64_make_prologue_cache (struct frame_info *this_frame, void **this_cache)
{
  struct aarch64_prologue_cache *cache;

  if (*this_cache != NULL)
    return (struct aarch64_prologue_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct aarch64_prologue_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);
  *this_cache = cache;

  TRY
    {
      aarch64_make_prologue_cache_1 (this_frame, cache);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw_exception (ex);
    }
  END_CATCH

  return cache;
}

/* Implement the "stop_reason" frame_unwind method.  */

static enum unwind_stop_reason
aarch64_prologue_frame_unwind_stop_reason (struct frame_info *this_frame,
					   void **this_cache)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_prologue_cache (this_frame, this_cache);

  if (!cache->available_p)
    return UNWIND_UNAVAILABLE;

  /* Halt the backtrace at "_start".  */
  if (cache->prev_pc <= gdbarch_tdep (get_frame_arch (this_frame))->lowest_pc)
    return UNWIND_OUTERMOST;

  /* We've hit a wall, stop.  */
  if (cache->prev_sp == 0)
    return UNWIND_OUTERMOST;

  return UNWIND_NO_REASON;
}

/* Our frame ID for a normal frame is the current function's starting
   PC and the caller's SP when we were called.  */

static void
aarch64_prologue_this_id (struct frame_info *this_frame,
			  void **this_cache, struct frame_id *this_id)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_prologue_cache (this_frame, this_cache);

  if (!cache->available_p)
    *this_id = frame_id_build_unavailable_stack (cache->func);
  else
    *this_id = frame_id_build (cache->prev_sp, cache->func);
}

/* Implement the "prev_register" frame_unwind method.  */

static struct value *
aarch64_prologue_prev_register (struct frame_info *this_frame,
				void **this_cache, int prev_regnum)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_prologue_cache (this_frame, this_cache);

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
  aarch64_prologue_frame_unwind_stop_reason,
  aarch64_prologue_this_id,
  aarch64_prologue_prev_register,
  NULL,
  default_frame_sniffer
};

/* Allocate and fill in *THIS_CACHE with information about the prologue of
   *THIS_FRAME.  Do not do this is if *THIS_CACHE was already allocated.
   Return a pointer to the current aarch64_prologue_cache in
   *THIS_CACHE.  */

static struct aarch64_prologue_cache *
aarch64_make_stub_cache (struct frame_info *this_frame, void **this_cache)
{
  struct aarch64_prologue_cache *cache;

  if (*this_cache != NULL)
    return (struct aarch64_prologue_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct aarch64_prologue_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);
  *this_cache = cache;

  TRY
    {
      cache->prev_sp = get_frame_register_unsigned (this_frame,
						    AARCH64_SP_REGNUM);
      cache->prev_pc = get_frame_pc (this_frame);
      cache->available_p = 1;
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw_exception (ex);
    }
  END_CATCH

  return cache;
}

/* Implement the "stop_reason" frame_unwind method.  */

static enum unwind_stop_reason
aarch64_stub_frame_unwind_stop_reason (struct frame_info *this_frame,
				       void **this_cache)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_stub_cache (this_frame, this_cache);

  if (!cache->available_p)
    return UNWIND_UNAVAILABLE;

  return UNWIND_NO_REASON;
}

/* Our frame ID for a stub frame is the current SP and LR.  */

static void
aarch64_stub_this_id (struct frame_info *this_frame,
		      void **this_cache, struct frame_id *this_id)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_stub_cache (this_frame, this_cache);

  if (cache->available_p)
    *this_id = frame_id_build (cache->prev_sp, cache->prev_pc);
  else
    *this_id = frame_id_build_unavailable_stack (cache->prev_pc);
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
  if (in_plt_section (addr_in_block)
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
  aarch64_stub_frame_unwind_stop_reason,
  aarch64_stub_this_id,
  aarch64_prologue_prev_register,
  NULL,
  aarch64_stub_unwind_sniffer
};

/* Return the frame base address of *THIS_FRAME.  */

static CORE_ADDR
aarch64_normal_frame_base (struct frame_info *this_frame, void **this_cache)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_prologue_cache (this_frame, this_cache);

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
  /* Value to pass on stack.  It can be NULL if this item is for stack
     padding.  */
  const gdb_byte *data;

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
      if (TYPE_VECTOR (t))
	{
	  /* Use the natural alignment for vector types (the same for
	     scalar type), but the maximum alignment is 128-bit.  */
	  if (TYPE_LENGTH (t) > 16)
	    return 16;
	  else
	    return TYPE_LENGTH (t);
	}
      else
	return aarch64_type_align (TYPE_TARGET_TYPE (t));
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

/* Return 1 if *TY is a homogeneous floating-point aggregate or
   homogeneous short-vector aggregate as defined in the AAPCS64 ABI
   document; otherwise return 0.  */

static int
is_hfa_or_hva (struct type *ty)
{
  switch (TYPE_CODE (ty))
    {
    case TYPE_CODE_ARRAY:
      {
	struct type *target_ty = TYPE_TARGET_TYPE (ty);

	if (TYPE_VECTOR (ty))
	  return 0;

	if (TYPE_LENGTH (ty) <= 4 /* HFA or HVA has at most 4 members.  */
	    && (TYPE_CODE (target_ty) == TYPE_CODE_FLT /* HFA */
		|| (TYPE_CODE (target_ty) == TYPE_CODE_ARRAY /* HVA */
		    && TYPE_VECTOR (target_ty))))
	  return 1;
	break;
      }

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      {
	/* HFA or HVA has at most four members.  */
	if (TYPE_NFIELDS (ty) > 0 && TYPE_NFIELDS (ty) <= 4)
	  {
	    struct type *member0_type;

	    member0_type = check_typedef (TYPE_FIELD_TYPE (ty, 0));
	    if (TYPE_CODE (member0_type) == TYPE_CODE_FLT
		|| (TYPE_CODE (member0_type) == TYPE_CODE_ARRAY
		    && TYPE_VECTOR (member0_type)))
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
	   struct value *arg)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = TYPE_LENGTH (type);
  enum type_code typecode = TYPE_CODE (type);
  int regnum = AARCH64_X0_REGNUM + info->ngrn;
  const bfd_byte *buf = value_contents (arg);

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
	{
	  debug_printf ("arg %d in %s = 0x%s\n", info->argnum,
			gdbarch_register_name (gdbarch, regnum),
			phex (regval, X_REGISTER_SIZE));
	}
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
	   int len, const bfd_byte *buf)
{
  if (info->nsrn < 8)
    {
      int regnum = AARCH64_V0_REGNUM + info->nsrn;
      gdb_byte reg[V_REGISTER_SIZE];

      info->argnum++;
      info->nsrn++;

      memset (reg, 0, sizeof (reg));
      /* PCS C.1, the argument is allocated to the least significant
	 bits of V register.  */
      memcpy (reg, buf, len);
      regcache_cooked_write (regcache, regnum, reg);

      if (aarch64_debug)
	{
	  debug_printf ("arg %d in %s\n", info->argnum,
			gdbarch_register_name (gdbarch, regnum));
	}
      return 1;
    }
  info->nsrn = 8;
  return 0;
}

/* Marshall an argument onto the stack.  */

static void
pass_on_stack (struct aarch64_call_info *info, struct type *type,
	       struct value *arg)
{
  const bfd_byte *buf = value_contents (arg);
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
    {
      debug_printf ("arg %d len=%d @ sp + %d\n", info->argnum, len,
		    info->nsaa);
    }

  item.len = len;
  item.data = buf;
  VEC_safe_push (stack_item_t, info->si, &item);

  info->nsaa += len;
  if (info->nsaa & (align - 1))
    {
      /* Push stack alignment padding.  */
      int pad = align - (info->nsaa & (align - 1));

      item.len = pad;
      item.data = NULL;

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
		    struct value *arg)
{
  int len = TYPE_LENGTH (type);
  int nregs = (len + X_REGISTER_SIZE - 1) / X_REGISTER_SIZE;

  /* PCS C.13 - Pass in registers if we have enough spare */
  if (info->ngrn + nregs <= 8)
    {
      pass_in_x (gdbarch, regcache, info, type, arg);
      info->ngrn += nregs;
    }
  else
    {
      info->ngrn = 8;
      pass_on_stack (info, type, arg);
    }
}

/* Pass a value in a V register, or on the stack if insufficient are
   available.  */

static void
pass_in_v_or_stack (struct gdbarch *gdbarch,
		    struct regcache *regcache,
		    struct aarch64_call_info *info,
		    struct type *type,
		    struct value *arg)
{
  if (!pass_in_v (gdbarch, regcache, info, TYPE_LENGTH (type),
		  value_contents (arg)))
    pass_on_stack (info, type, arg);
}

/* Implement the "push_dummy_call" gdbarch method.  */

static CORE_ADDR
aarch64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			 struct regcache *regcache, CORE_ADDR bp_addr,
			 int nargs,
			 struct value **args, CORE_ADDR sp, int struct_return,
			 CORE_ADDR struct_addr)
{
  int argnum;
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
	{
	  debug_printf ("struct return in %s = 0x%s\n",
			gdbarch_register_name (gdbarch,
					       AARCH64_STRUCT_RETURN_REGNUM),
			paddress (gdbarch, struct_addr));
	}
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
	  pass_in_x_or_stack (gdbarch, regcache, &info, arg_type, arg);
	  break;

	case TYPE_CODE_COMPLEX:
	  if (info.nsrn <= 6)
	    {
	      const bfd_byte *buf = value_contents (arg);
	      struct type *target_type =
		check_typedef (TYPE_TARGET_TYPE (arg_type));

	      pass_in_v (gdbarch, regcache, &info,
			 TYPE_LENGTH (target_type), buf);
	      pass_in_v (gdbarch, regcache, &info,
			 TYPE_LENGTH (target_type),
			 buf + TYPE_LENGTH (target_type));
	    }
	  else
	    {
	      info.nsrn = 8;
	      pass_on_stack (&info, arg_type, arg);
	    }
	  break;
	case TYPE_CODE_FLT:
	  pass_in_v_or_stack (gdbarch, regcache, &info, arg_type, arg);
	  break;

	case TYPE_CODE_STRUCT:
	case TYPE_CODE_ARRAY:
	case TYPE_CODE_UNION:
	  if (is_hfa_or_hva (arg_type))
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

		      pass_in_v_or_stack (gdbarch, regcache, &info,
					  field_type, field);
		    }
		}
	      else
		{
		  info.nsrn = 8;
		  pass_on_stack (&info, arg_type, arg);
		}
	    }
	  else if (TYPE_CODE (arg_type) == TYPE_CODE_ARRAY
		   && TYPE_VECTOR (arg_type) && (len == 16 || len == 8))
	    {
	      /* Short vector types are passed in V registers.  */
	      pass_in_v_or_stack (gdbarch, regcache, &info, arg_type, arg);
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
	      pass_in_x_or_stack (gdbarch, regcache, &info, arg_type, arg);
	    }
	  else
	    /* PCS C.15 / C.18 multiple values pass.  */
	    pass_in_x_or_stack (gdbarch, regcache, &info, arg_type, arg);
	  break;

	default:
	  pass_in_x_or_stack (gdbarch, regcache, &info, arg_type, arg);
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
      if (si->data != NULL)
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
static const gdb_byte aarch64_default_breakpoint[] = {0x00, 0x00, 0x20, 0xd4};

/* Implement the "breakpoint_from_pc" gdbarch method.  */

static const gdb_byte *
aarch64_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr,
			    int *lenptr)
{
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
  else if (is_hfa_or_hva (type))
    {
      int elements = TYPE_NFIELDS (type);
      struct type *member_type = check_typedef (TYPE_FIELD_TYPE (type, 0));
      int len = TYPE_LENGTH (member_type);
      int i;

      for (i = 0; i < elements; i++)
	{
	  int regno = AARCH64_V0_REGNUM + i;
	  bfd_byte buf[V_REGISTER_SIZE];

	  if (aarch64_debug)
	    {
	      debug_printf ("read HFA or HVA return value element %d from %s\n",
			    i + 1,
			    gdbarch_register_name (gdbarch, regno));
	    }
	  regcache_cooked_read (regs, regno, buf);

	  memcpy (valbuf, buf, len);
	  valbuf += len;
	}
    }
  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type)
	   && (TYPE_LENGTH (type) == 16 || TYPE_LENGTH (type) == 8))
    {
      /* Short vector is returned in V register.  */
      gdb_byte buf[V_REGISTER_SIZE];

      regcache_cooked_read (regs, AARCH64_V0_REGNUM, buf);
      memcpy (valbuf, buf, TYPE_LENGTH (type));
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
  type = check_typedef (type);

  if (is_hfa_or_hva (type))
    {
      /* v0-v7 are used to return values and one register is allocated
	 for one member.  However, HFA or HVA has at most four members.  */
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
  else if (is_hfa_or_hva (type))
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
	    {
	      debug_printf ("write HFA or HVA return value element %d to %s\n",
			    i + 1,
			    gdbarch_register_name (gdbarch, regno));
	    }

	  memcpy (tmpbuf, valbuf, len);
	  regcache_cooked_write (regs, regno, tmpbuf);
	  valbuf += len;
	}
    }
  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type)
	   && (TYPE_LENGTH (type) == 8 || TYPE_LENGTH (type) == 16))
    {
      /* Short vector.  */
      gdb_byte buf[V_REGISTER_SIZE];

      memcpy (buf, valbuf, TYPE_LENGTH (type));
      regcache_cooked_write (regs, AARCH64_V0_REGNUM, buf);
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

  if (TYPE_CODE (valtype) == TYPE_CODE_STRUCT
      || TYPE_CODE (valtype) == TYPE_CODE_UNION
      || TYPE_CODE (valtype) == TYPE_CODE_ARRAY)
    {
      if (aarch64_return_in_memory (gdbarch, valtype))
	{
	  if (aarch64_debug)
	    debug_printf ("return value in memory\n");
	  return RETURN_VALUE_STRUCT_CONVENTION;
	}
    }

  if (writebuf)
    aarch64_store_return_value (valtype, regcache, writebuf);

  if (readbuf)
    aarch64_extract_return_value (valtype, regcache, readbuf);

  if (aarch64_debug)
    debug_printf ("return value in registers\n");

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

/* Implement the "gen_return_address" gdbarch method.  */

static void
aarch64_gen_return_address (struct gdbarch *gdbarch,
			    struct agent_expr *ax, struct axs_value *value,
			    CORE_ADDR scope)
{
  value->type = register_type (gdbarch, AARCH64_LR_REGNUM);
  value->kind = axs_lvalue_register;
  value->u.reg = AARCH64_LR_REGNUM;
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
      if (status != REG_VALID)
	mark_value_bytes_unavailable (result_value, 0,
				      TYPE_LENGTH (value_type (result_value)));
      else
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

/* Callback function for user_reg_add.  */

static struct value *
value_of_aarch64_user_reg (struct frame_info *frame, const void *baton)
{
  const int *reg_p = (const int *) baton;

  return value_of_register (*reg_p, frame);
}


/* Implement the "software_single_step" gdbarch method, needed to
   single step through atomic sequences on AArch64.  */

static int
aarch64_software_single_step (struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct address_space *aspace = get_frame_address_space (frame);
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  const int insn_size = 4;
  const int atomic_sequence_length = 16; /* Instruction sequence length.  */
  CORE_ADDR pc = get_frame_pc (frame);
  CORE_ADDR breaks[2] = { -1, -1 };
  CORE_ADDR loc = pc;
  CORE_ADDR closing_insn = 0;
  uint32_t insn = read_memory_unsigned_integer (loc, insn_size,
						byte_order_for_code);
  int index;
  int insn_count;
  int bc_insn_count = 0; /* Conditional branch instruction count.  */
  int last_breakpoint = 0; /* Defaults to 0 (no breakpoints placed).  */
  aarch64_inst inst;

  if (aarch64_decode_insn (insn, &inst, 1) != 0)
    return 0;

  /* Look for a Load Exclusive instruction which begins the sequence.  */
  if (inst.opcode->iclass != ldstexcl || bit (insn, 22) == 0)
    return 0;

  for (insn_count = 0; insn_count < atomic_sequence_length; ++insn_count)
    {
      loc += insn_size;
      insn = read_memory_unsigned_integer (loc, insn_size,
					   byte_order_for_code);

      if (aarch64_decode_insn (insn, &inst, 1) != 0)
	return 0;
      /* Check if the instruction is a conditional branch.  */
      if (inst.opcode->iclass == condbranch)
	{
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_ADDR_PCREL19);

	  if (bc_insn_count >= 1)
	    return 0;

	  /* It is, so we'll try to set a breakpoint at the destination.  */
	  breaks[1] = loc + inst.operands[0].imm.value;

	  bc_insn_count++;
	  last_breakpoint++;
	}

      /* Look for the Store Exclusive which closes the atomic sequence.  */
      if (inst.opcode->iclass == ldstexcl && bit (insn, 22) == 0)
	{
	  closing_insn = loc;
	  break;
	}
    }

  /* We didn't find a closing Store Exclusive instruction, fall back.  */
  if (!closing_insn)
    return 0;

  /* Insert breakpoint after the end of the atomic sequence.  */
  breaks[0] = loc + insn_size;

  /* Check for duplicated breakpoints, and also check that the second
     breakpoint is not within the atomic sequence.  */
  if (last_breakpoint
      && (breaks[1] == breaks[0]
	  || (breaks[1] >= pc && breaks[1] <= closing_insn)))
    last_breakpoint = 0;

  /* Insert the breakpoint at the end of the sequence, and one at the
     destination of the conditional branch, if it exists.  */
  for (index = 0; index <= last_breakpoint; index++)
    insert_single_step_breakpoint (gdbarch, aspace, breaks[index]);

  return 1;
}

struct displaced_step_closure
{
  /* It is true when condition instruction, such as B.CON, TBZ, etc,
     is being displaced stepping.  */
  int cond;

  /* PC adjustment offset after displaced stepping.  */
  int32_t pc_adjust;
};

/* Data when visiting instructions for displaced stepping.  */

struct aarch64_displaced_step_data
{
  struct aarch64_insn_data base;

  /* The address where the instruction will be executed at.  */
  CORE_ADDR new_addr;
  /* Buffer of instructions to be copied to NEW_ADDR to execute.  */
  uint32_t insn_buf[DISPLACED_MODIFIED_INSNS];
  /* Number of instructions in INSN_BUF.  */
  unsigned insn_count;
  /* Registers when doing displaced stepping.  */
  struct regcache *regs;

  struct displaced_step_closure *dsc;
};

/* Implementation of aarch64_insn_visitor method "b".  */

static void
aarch64_displaced_step_b (const int is_bl, const int32_t offset,
			  struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;
  int64_t new_offset = data->insn_addr - dsd->new_addr + offset;

  if (can_encode_int32 (new_offset, 28))
    {
      /* Emit B rather than BL, because executing BL on a new address
	 will get the wrong address into LR.  In order to avoid this,
	 we emit B, and update LR if the instruction is BL.  */
      emit_b (dsd->insn_buf, 0, new_offset);
      dsd->insn_count++;
    }
  else
    {
      /* Write NOP.  */
      emit_nop (dsd->insn_buf);
      dsd->insn_count++;
      dsd->dsc->pc_adjust = offset;
    }

  if (is_bl)
    {
      /* Update LR.  */
      regcache_cooked_write_unsigned (dsd->regs, AARCH64_LR_REGNUM,
				      data->insn_addr + 4);
    }
}

/* Implementation of aarch64_insn_visitor method "b_cond".  */

static void
aarch64_displaced_step_b_cond (const unsigned cond, const int32_t offset,
			       struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;

  /* GDB has to fix up PC after displaced step this instruction
     differently according to the condition is true or false.  Instead
     of checking COND against conditional flags, we can use
     the following instructions, and GDB can tell how to fix up PC
     according to the PC value.

     B.COND TAKEN    ; If cond is true, then jump to TAKEN.
     INSN1     ;
     TAKEN:
     INSN2
  */

  emit_bcond (dsd->insn_buf, cond, 8);
  dsd->dsc->cond = 1;
  dsd->dsc->pc_adjust = offset;
  dsd->insn_count = 1;
}

/* Dynamically allocate a new register.  If we know the register
   statically, we should make it a global as above instead of using this
   helper function.  */

static struct aarch64_register
aarch64_register (unsigned num, int is64)
{
  return (struct aarch64_register) { num, is64 };
}

/* Implementation of aarch64_insn_visitor method "cb".  */

static void
aarch64_displaced_step_cb (const int32_t offset, const int is_cbnz,
			   const unsigned rn, int is64,
			   struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;

  /* The offset is out of range for a compare and branch
     instruction.  We can use the following instructions instead:

	 CBZ xn, TAKEN   ; xn == 0, then jump to TAKEN.
	 INSN1     ;
	 TAKEN:
	 INSN2
  */
  emit_cb (dsd->insn_buf, is_cbnz, aarch64_register (rn, is64), 8);
  dsd->insn_count = 1;
  dsd->dsc->cond = 1;
  dsd->dsc->pc_adjust = offset;
}

/* Implementation of aarch64_insn_visitor method "tb".  */

static void
aarch64_displaced_step_tb (const int32_t offset, int is_tbnz,
			   const unsigned rt, unsigned bit,
			   struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;

  /* The offset is out of range for a test bit and branch
     instruction We can use the following instructions instead:

     TBZ xn, #bit, TAKEN ; xn[bit] == 0, then jump to TAKEN.
     INSN1         ;
     TAKEN:
     INSN2

  */
  emit_tb (dsd->insn_buf, is_tbnz, bit, aarch64_register (rt, 1), 8);
  dsd->insn_count = 1;
  dsd->dsc->cond = 1;
  dsd->dsc->pc_adjust = offset;
}

/* Implementation of aarch64_insn_visitor method "adr".  */

static void
aarch64_displaced_step_adr (const int32_t offset, const unsigned rd,
			    const int is_adrp, struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;
  /* We know exactly the address the ADR{P,} instruction will compute.
     We can just write it to the destination register.  */
  CORE_ADDR address = data->insn_addr + offset;

  if (is_adrp)
    {
      /* Clear the lower 12 bits of the offset to get the 4K page.  */
      regcache_cooked_write_unsigned (dsd->regs, AARCH64_X0_REGNUM + rd,
				      address & ~0xfff);
    }
  else
      regcache_cooked_write_unsigned (dsd->regs, AARCH64_X0_REGNUM + rd,
				      address);

  dsd->dsc->pc_adjust = 4;
  emit_nop (dsd->insn_buf);
  dsd->insn_count = 1;
}

/* Implementation of aarch64_insn_visitor method "ldr_literal".  */

static void
aarch64_displaced_step_ldr_literal (const int32_t offset, const int is_sw,
				    const unsigned rt, const int is64,
				    struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;
  CORE_ADDR address = data->insn_addr + offset;
  struct aarch64_memory_operand zero = { MEMORY_OPERAND_OFFSET, 0 };

  regcache_cooked_write_unsigned (dsd->regs, AARCH64_X0_REGNUM + rt,
				  address);

  if (is_sw)
    dsd->insn_count = emit_ldrsw (dsd->insn_buf, aarch64_register (rt, 1),
				  aarch64_register (rt, 1), zero);
  else
    dsd->insn_count = emit_ldr (dsd->insn_buf, aarch64_register (rt, is64),
				aarch64_register (rt, 1), zero);

  dsd->dsc->pc_adjust = 4;
}

/* Implementation of aarch64_insn_visitor method "others".  */

static void
aarch64_displaced_step_others (const uint32_t insn,
			       struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;

  aarch64_emit_insn (dsd->insn_buf, insn);
  dsd->insn_count = 1;

  if ((insn & 0xfffffc1f) == 0xd65f0000)
    {
      /* RET */
      dsd->dsc->pc_adjust = 0;
    }
  else
    dsd->dsc->pc_adjust = 4;
}

static const struct aarch64_insn_visitor visitor =
{
  aarch64_displaced_step_b,
  aarch64_displaced_step_b_cond,
  aarch64_displaced_step_cb,
  aarch64_displaced_step_tb,
  aarch64_displaced_step_adr,
  aarch64_displaced_step_ldr_literal,
  aarch64_displaced_step_others,
};

/* Implement the "displaced_step_copy_insn" gdbarch method.  */

struct displaced_step_closure *
aarch64_displaced_step_copy_insn (struct gdbarch *gdbarch,
				  CORE_ADDR from, CORE_ADDR to,
				  struct regcache *regs)
{
  struct displaced_step_closure *dsc = NULL;
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  uint32_t insn = read_memory_unsigned_integer (from, 4, byte_order_for_code);
  struct aarch64_displaced_step_data dsd;
  aarch64_inst inst;

  if (aarch64_decode_insn (insn, &inst, 1) != 0)
    return NULL;

  /* Look for a Load Exclusive instruction which begins the sequence.  */
  if (inst.opcode->iclass == ldstexcl && bit (insn, 22))
    {
      /* We can't displaced step atomic sequences.  */
      return NULL;
    }

  dsc = XCNEW (struct displaced_step_closure);
  dsd.base.insn_addr = from;
  dsd.new_addr = to;
  dsd.regs = regs;
  dsd.dsc = dsc;
  dsd.insn_count = 0;
  aarch64_relocate_instruction (insn, &visitor,
				(struct aarch64_insn_data *) &dsd);
  gdb_assert (dsd.insn_count <= DISPLACED_MODIFIED_INSNS);

  if (dsd.insn_count != 0)
    {
      int i;

      /* Instruction can be relocated to scratch pad.  Copy
	 relocated instruction(s) there.  */
      for (i = 0; i < dsd.insn_count; i++)
	{
	  if (debug_displaced)
	    {
	      debug_printf ("displaced: writing insn ");
	      debug_printf ("%.8x", dsd.insn_buf[i]);
	      debug_printf (" at %s\n", paddress (gdbarch, to + i * 4));
	    }
	  write_memory_unsigned_integer (to + i * 4, 4, byte_order_for_code,
					 (ULONGEST) dsd.insn_buf[i]);
	}
    }
  else
    {
      xfree (dsc);
      dsc = NULL;
    }

  return dsc;
}

/* Implement the "displaced_step_fixup" gdbarch method.  */

void
aarch64_displaced_step_fixup (struct gdbarch *gdbarch,
			      struct displaced_step_closure *dsc,
			      CORE_ADDR from, CORE_ADDR to,
			      struct regcache *regs)
{
  if (dsc->cond)
    {
      ULONGEST pc;

      regcache_cooked_read_unsigned (regs, AARCH64_PC_REGNUM, &pc);
      if (pc - to == 8)
	{
	  /* Condition is true.  */
	}
      else if (pc - to == 4)
	{
	  /* Condition is false.  */
	  dsc->pc_adjust = 4;
	}
      else
	gdb_assert_not_reached ("Unexpected PC value after displaced stepping");
    }

  if (dsc->pc_adjust != 0)
    {
      if (debug_displaced)
	{
	  debug_printf ("displaced: fixup: set PC to %s:%d\n",
			paddress (gdbarch, from), dsc->pc_adjust);
	}
      regcache_cooked_write_unsigned (regs, AARCH64_PC_REGNUM,
				      from + dsc->pc_adjust);
    }
}

/* Implement the "displaced_step_hw_singlestep" gdbarch method.  */

int
aarch64_displaced_step_hw_singlestep (struct gdbarch *gdbarch,
				      struct displaced_step_closure *closure)
{
  return 1;
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

  tdep = XCNEW (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  /* This should be low enough for everything.  */
  tdep->lowest_pc = 0x20;
  tdep->jb_pc = -1;		/* Longjump support not enabled by default.  */
  tdep->jb_elt_size = 8;

  set_gdbarch_push_dummy_call (gdbarch, aarch64_push_dummy_call);
  set_gdbarch_frame_align (gdbarch, aarch64_frame_align);

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
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);
  set_gdbarch_software_single_step (gdbarch, aarch64_software_single_step);

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

  set_gdbarch_gen_return_address (gdbarch, aarch64_gen_return_address);

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

  /* Debug this file's internals.  */
  add_setshow_boolean_cmd ("aarch64", class_maintenance, &aarch64_debug, _("\
Set AArch64 debugging."), _("\
Show AArch64 debugging."), _("\
When on, AArch64 specific debugging is enabled."),
			    NULL,
			    show_aarch64_debug,
			    &setdebuglist, &showdebuglist);
}

/* AArch64 process record-replay related structures, defines etc.  */

#define REG_ALLOC(REGS, LENGTH, RECORD_BUF) \
        do  \
          { \
            unsigned int reg_len = LENGTH; \
            if (reg_len) \
              { \
                REGS = XNEWVEC (uint32_t, reg_len); \
                memcpy(&REGS[0], &RECORD_BUF[0], sizeof(uint32_t)*LENGTH); \
              } \
          } \
        while (0)

#define MEM_ALLOC(MEMS, LENGTH, RECORD_BUF) \
        do  \
          { \
            unsigned int mem_len = LENGTH; \
            if (mem_len) \
            { \
              MEMS =  XNEWVEC (struct aarch64_mem_r, mem_len);  \
              memcpy(&MEMS->len, &RECORD_BUF[0], \
                     sizeof(struct aarch64_mem_r) * LENGTH); \
            } \
          } \
          while (0)

/* AArch64 record/replay structures and enumerations.  */

struct aarch64_mem_r
{
  uint64_t len;    /* Record length.  */
  uint64_t addr;   /* Memory address.  */
};

enum aarch64_record_result
{
  AARCH64_RECORD_SUCCESS,
  AARCH64_RECORD_FAILURE,
  AARCH64_RECORD_UNSUPPORTED,
  AARCH64_RECORD_UNKNOWN
};

typedef struct insn_decode_record_t
{
  struct gdbarch *gdbarch;
  struct regcache *regcache;
  CORE_ADDR this_addr;                 /* Address of insn to be recorded.  */
  uint32_t aarch64_insn;               /* Insn to be recorded.  */
  uint32_t mem_rec_count;              /* Count of memory records.  */
  uint32_t reg_rec_count;              /* Count of register records.  */
  uint32_t *aarch64_regs;              /* Registers to be recorded.  */
  struct aarch64_mem_r *aarch64_mems;  /* Memory locations to be recorded.  */
} insn_decode_record;

/* Record handler for data processing - register instructions.  */

static unsigned int
aarch64_record_data_proc_reg (insn_decode_record *aarch64_insn_r)
{
  uint8_t reg_rd, insn_bits24_27, insn_bits21_23;
  uint32_t record_buf[4];

  reg_rd = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);
  insn_bits21_23 = bits (aarch64_insn_r->aarch64_insn, 21, 23);

  if (!bit (aarch64_insn_r->aarch64_insn, 28))
    {
      uint8_t setflags;

      /* Logical (shifted register).  */
      if (insn_bits24_27 == 0x0a)
	setflags = (bits (aarch64_insn_r->aarch64_insn, 29, 30) == 0x03);
      /* Add/subtract.  */
      else if (insn_bits24_27 == 0x0b)
	setflags = bit (aarch64_insn_r->aarch64_insn, 29);
      else
	return AARCH64_RECORD_UNKNOWN;

      record_buf[0] = reg_rd;
      aarch64_insn_r->reg_rec_count = 1;
      if (setflags)
	record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_CPSR_REGNUM;
    }
  else
    {
      if (insn_bits24_27 == 0x0b)
	{
	  /* Data-processing (3 source).  */
	  record_buf[0] = reg_rd;
	  aarch64_insn_r->reg_rec_count = 1;
	}
      else if (insn_bits24_27 == 0x0a)
	{
	  if (insn_bits21_23 == 0x00)
	    {
	      /* Add/subtract (with carry).  */
	      record_buf[0] = reg_rd;
	      aarch64_insn_r->reg_rec_count = 1;
	      if (bit (aarch64_insn_r->aarch64_insn, 29))
		{
		  record_buf[1] = AARCH64_CPSR_REGNUM;
		  aarch64_insn_r->reg_rec_count = 2;
		}
	    }
	  else if (insn_bits21_23 == 0x02)
	    {
	      /* Conditional compare (register) and conditional compare
		 (immediate) instructions.  */
	      record_buf[0] = AARCH64_CPSR_REGNUM;
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	  else if (insn_bits21_23 == 0x04 || insn_bits21_23 == 0x06)
	    {
	      /* CConditional select.  */
	      /* Data-processing (2 source).  */
	      /* Data-processing (1 source).  */
	      record_buf[0] = reg_rd;
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	  else
	    return AARCH64_RECORD_UNKNOWN;
	}
    }

  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for data processing - immediate instructions.  */

static unsigned int
aarch64_record_data_proc_imm (insn_decode_record *aarch64_insn_r)
{
  uint8_t reg_rd, insn_bit23, insn_bits24_27, setflags;
  uint32_t record_buf[4];

  reg_rd = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  insn_bit23 = bit (aarch64_insn_r->aarch64_insn, 23);
  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);

  if (insn_bits24_27 == 0x00                     /* PC rel addressing.  */
     || insn_bits24_27 == 0x03                   /* Bitfield and Extract.  */
     || (insn_bits24_27 == 0x02 && insn_bit23))  /* Move wide (immediate).  */
    {
      record_buf[0] = reg_rd;
      aarch64_insn_r->reg_rec_count = 1;
    }
  else if (insn_bits24_27 == 0x01)
    {
      /* Add/Subtract (immediate).  */
      setflags = bit (aarch64_insn_r->aarch64_insn, 29);
      record_buf[0] = reg_rd;
      aarch64_insn_r->reg_rec_count = 1;
      if (setflags)
	record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_CPSR_REGNUM;
    }
  else if (insn_bits24_27 == 0x02 && !insn_bit23)
    {
      /* Logical (immediate).  */
      setflags = bits (aarch64_insn_r->aarch64_insn, 29, 30) == 0x03;
      record_buf[0] = reg_rd;
      aarch64_insn_r->reg_rec_count = 1;
      if (setflags)
	record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_CPSR_REGNUM;
    }
  else
    return AARCH64_RECORD_UNKNOWN;

  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for branch, exception generation and system instructions.  */

static unsigned int
aarch64_record_branch_except_sys (insn_decode_record *aarch64_insn_r)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (aarch64_insn_r->gdbarch);
  uint8_t insn_bits24_27, insn_bits28_31, insn_bits22_23;
  uint32_t record_buf[4];

  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);
  insn_bits28_31 = bits (aarch64_insn_r->aarch64_insn, 28, 31);
  insn_bits22_23 = bits (aarch64_insn_r->aarch64_insn, 22, 23);

  if (insn_bits28_31 == 0x0d)
    {
      /* Exception generation instructions. */
      if (insn_bits24_27 == 0x04)
	{
	  if (!bits (aarch64_insn_r->aarch64_insn, 2, 4)
	      && !bits (aarch64_insn_r->aarch64_insn, 21, 23)
	      && bits (aarch64_insn_r->aarch64_insn, 0, 1) == 0x01)
	    {
	      ULONGEST svc_number;

	      regcache_raw_read_unsigned (aarch64_insn_r->regcache, 8,
					  &svc_number);
	      return tdep->aarch64_syscall_record (aarch64_insn_r->regcache,
						   svc_number);
	    }
	  else
	    return AARCH64_RECORD_UNSUPPORTED;
	}
      /* System instructions. */
      else if (insn_bits24_27 == 0x05 && insn_bits22_23 == 0x00)
	{
	  uint32_t reg_rt, reg_crn;

	  reg_rt = bits (aarch64_insn_r->aarch64_insn, 0, 4);
	  reg_crn = bits (aarch64_insn_r->aarch64_insn, 12, 15);

	  /* Record rt in case of sysl and mrs instructions.  */
	  if (bit (aarch64_insn_r->aarch64_insn, 21))
	    {
	      record_buf[0] = reg_rt;
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	  /* Record cpsr for hint and msr(immediate) instructions.  */
	  else if (reg_crn == 0x02 || reg_crn == 0x04)
	    {
	      record_buf[0] = AARCH64_CPSR_REGNUM;
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	}
      /* Unconditional branch (register).  */
      else if((insn_bits24_27 & 0x0e) == 0x06)
	{
	  record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_PC_REGNUM;
	  if (bits (aarch64_insn_r->aarch64_insn, 21, 22) == 0x01)
	    record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_LR_REGNUM;
	}
      else
	return AARCH64_RECORD_UNKNOWN;
    }
  /* Unconditional branch (immediate).  */
  else if ((insn_bits28_31 & 0x07) == 0x01 && (insn_bits24_27 & 0x0c) == 0x04)
    {
      record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_PC_REGNUM;
      if (bit (aarch64_insn_r->aarch64_insn, 31))
	record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_LR_REGNUM;
    }
  else
    /* Compare & branch (immediate), Test & branch (immediate) and
       Conditional branch (immediate).  */
    record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_PC_REGNUM;

  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for advanced SIMD load and store instructions.  */

static unsigned int
aarch64_record_asimd_load_store (insn_decode_record *aarch64_insn_r)
{
  CORE_ADDR address;
  uint64_t addr_offset = 0;
  uint32_t record_buf[24];
  uint64_t record_buf_mem[24];
  uint32_t reg_rn, reg_rt;
  uint32_t reg_index = 0, mem_index = 0;
  uint8_t opcode_bits, size_bits;

  reg_rt = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  reg_rn = bits (aarch64_insn_r->aarch64_insn, 5, 9);
  size_bits = bits (aarch64_insn_r->aarch64_insn, 10, 11);
  opcode_bits = bits (aarch64_insn_r->aarch64_insn, 12, 15);
  regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn, &address);

  if (record_debug)
    debug_printf ("Process record: Advanced SIMD load/store\n");

  /* Load/store single structure.  */
  if (bit (aarch64_insn_r->aarch64_insn, 24))
    {
      uint8_t sindex, scale, selem, esize, replicate = 0;
      scale = opcode_bits >> 2;
      selem = ((opcode_bits & 0x02) |
              bit (aarch64_insn_r->aarch64_insn, 21)) + 1;
      switch (scale)
        {
        case 1:
          if (size_bits & 0x01)
            return AARCH64_RECORD_UNKNOWN;
          break;
        case 2:
          if ((size_bits >> 1) & 0x01)
            return AARCH64_RECORD_UNKNOWN;
          if (size_bits & 0x01)
            {
              if (!((opcode_bits >> 1) & 0x01))
                scale = 3;
              else
                return AARCH64_RECORD_UNKNOWN;
            }
          break;
        case 3:
          if (bit (aarch64_insn_r->aarch64_insn, 22) && !(opcode_bits & 0x01))
            {
              scale = size_bits;
              replicate = 1;
              break;
            }
          else
            return AARCH64_RECORD_UNKNOWN;
        default:
          break;
        }
      esize = 8 << scale;
      if (replicate)
        for (sindex = 0; sindex < selem; sindex++)
          {
            record_buf[reg_index++] = reg_rt + AARCH64_V0_REGNUM;
            reg_rt = (reg_rt + 1) % 32;
          }
      else
        {
          for (sindex = 0; sindex < selem; sindex++)
	    {
	      if (bit (aarch64_insn_r->aarch64_insn, 22))
		record_buf[reg_index++] = reg_rt + AARCH64_V0_REGNUM;
	      else
		{
		  record_buf_mem[mem_index++] = esize / 8;
		  record_buf_mem[mem_index++] = address + addr_offset;
		}
	      addr_offset = addr_offset + (esize / 8);
	      reg_rt = (reg_rt + 1) % 32;
	    }
        }
    }
  /* Load/store multiple structure.  */
  else
    {
      uint8_t selem, esize, rpt, elements;
      uint8_t eindex, rindex;

      esize = 8 << size_bits;
      if (bit (aarch64_insn_r->aarch64_insn, 30))
        elements = 128 / esize;
      else
        elements = 64 / esize;

      switch (opcode_bits)
        {
        /*LD/ST4 (4 Registers).  */
        case 0:
          rpt = 1;
          selem = 4;
          break;
        /*LD/ST1 (4 Registers).  */
        case 2:
          rpt = 4;
          selem = 1;
          break;
        /*LD/ST3 (3 Registers).  */
        case 4:
          rpt = 1;
          selem = 3;
          break;
        /*LD/ST1 (3 Registers).  */
        case 6:
          rpt = 3;
          selem = 1;
          break;
        /*LD/ST1 (1 Register).  */
        case 7:
          rpt = 1;
          selem = 1;
          break;
        /*LD/ST2 (2 Registers).  */
        case 8:
          rpt = 1;
          selem = 2;
          break;
        /*LD/ST1 (2 Registers).  */
        case 10:
          rpt = 2;
          selem = 1;
          break;
        default:
          return AARCH64_RECORD_UNSUPPORTED;
          break;
        }
      for (rindex = 0; rindex < rpt; rindex++)
        for (eindex = 0; eindex < elements; eindex++)
          {
            uint8_t reg_tt, sindex;
            reg_tt = (reg_rt + rindex) % 32;
            for (sindex = 0; sindex < selem; sindex++)
              {
                if (bit (aarch64_insn_r->aarch64_insn, 22))
                  record_buf[reg_index++] = reg_tt + AARCH64_V0_REGNUM;
                else
                  {
                    record_buf_mem[mem_index++] = esize / 8;
                    record_buf_mem[mem_index++] = address + addr_offset;
                  }
                addr_offset = addr_offset + (esize / 8);
                reg_tt = (reg_tt + 1) % 32;
              }
          }
    }

  if (bit (aarch64_insn_r->aarch64_insn, 23))
    record_buf[reg_index++] = reg_rn;

  aarch64_insn_r->reg_rec_count = reg_index;
  aarch64_insn_r->mem_rec_count = mem_index / 2;
  MEM_ALLOC (aarch64_insn_r->aarch64_mems, aarch64_insn_r->mem_rec_count,
             record_buf_mem);
  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
             record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for load and store instructions.  */

static unsigned int
aarch64_record_load_store (insn_decode_record *aarch64_insn_r)
{
  uint8_t insn_bits24_27, insn_bits28_29, insn_bits10_11;
  uint8_t insn_bit23, insn_bit21;
  uint8_t opc, size_bits, ld_flag, vector_flag;
  uint32_t reg_rn, reg_rt, reg_rt2;
  uint64_t datasize, offset;
  uint32_t record_buf[8];
  uint64_t record_buf_mem[8];
  CORE_ADDR address;

  insn_bits10_11 = bits (aarch64_insn_r->aarch64_insn, 10, 11);
  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);
  insn_bits28_29 = bits (aarch64_insn_r->aarch64_insn, 28, 29);
  insn_bit21 = bit (aarch64_insn_r->aarch64_insn, 21);
  insn_bit23 = bit (aarch64_insn_r->aarch64_insn, 23);
  ld_flag = bit (aarch64_insn_r->aarch64_insn, 22);
  vector_flag = bit (aarch64_insn_r->aarch64_insn, 26);
  reg_rt = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  reg_rn = bits (aarch64_insn_r->aarch64_insn, 5, 9);
  reg_rt2 = bits (aarch64_insn_r->aarch64_insn, 10, 14);
  size_bits = bits (aarch64_insn_r->aarch64_insn, 30, 31);

  /* Load/store exclusive.  */
  if (insn_bits24_27 == 0x08 && insn_bits28_29 == 0x00)
    {
      if (record_debug)
	debug_printf ("Process record: load/store exclusive\n");

      if (ld_flag)
	{
	  record_buf[0] = reg_rt;
	  aarch64_insn_r->reg_rec_count = 1;
	  if (insn_bit21)
	    {
	      record_buf[1] = reg_rt2;
	      aarch64_insn_r->reg_rec_count = 2;
	    }
	}
      else
	{
	  if (insn_bit21)
	    datasize = (8 << size_bits) * 2;
	  else
	    datasize = (8 << size_bits);
	  regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
				      &address);
	  record_buf_mem[0] = datasize / 8;
	  record_buf_mem[1] = address;
	  aarch64_insn_r->mem_rec_count = 1;
	  if (!insn_bit23)
	    {
	      /* Save register rs.  */
	      record_buf[0] = bits (aarch64_insn_r->aarch64_insn, 16, 20);
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	}
    }
  /* Load register (literal) instructions decoding.  */
  else if ((insn_bits24_27 & 0x0b) == 0x08 && insn_bits28_29 == 0x01)
    {
      if (record_debug)
	debug_printf ("Process record: load register (literal)\n");
      if (vector_flag)
        record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
      else
        record_buf[0] = reg_rt;
      aarch64_insn_r->reg_rec_count = 1;
    }
  /* All types of load/store pair instructions decoding.  */
  else if ((insn_bits24_27 & 0x0a) == 0x08 && insn_bits28_29 == 0x02)
    {
      if (record_debug)
	debug_printf ("Process record: load/store pair\n");

      if (ld_flag)
        {
          if (vector_flag)
            {
              record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
              record_buf[1] = reg_rt2 + AARCH64_V0_REGNUM;
            }
          else
            {
              record_buf[0] = reg_rt;
              record_buf[1] = reg_rt2;
            }
          aarch64_insn_r->reg_rec_count = 2;
        }
      else
        {
          uint16_t imm7_off;
          imm7_off = bits (aarch64_insn_r->aarch64_insn, 15, 21);
          if (!vector_flag)
            size_bits = size_bits >> 1;
          datasize = 8 << (2 + size_bits);
          offset = (imm7_off & 0x40) ? (~imm7_off & 0x007f) + 1 : imm7_off;
          offset = offset << (2 + size_bits);
          regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
                                      &address);
          if (!((insn_bits24_27 & 0x0b) == 0x08 && insn_bit23))
            {
              if (imm7_off & 0x40)
                address = address - offset;
              else
                address = address + offset;
            }

          record_buf_mem[0] = datasize / 8;
          record_buf_mem[1] = address;
          record_buf_mem[2] = datasize / 8;
          record_buf_mem[3] = address + (datasize / 8);
          aarch64_insn_r->mem_rec_count = 2;
        }
      if (bit (aarch64_insn_r->aarch64_insn, 23))
        record_buf[aarch64_insn_r->reg_rec_count++] = reg_rn;
    }
  /* Load/store register (unsigned immediate) instructions.  */
  else if ((insn_bits24_27 & 0x0b) == 0x09 && insn_bits28_29 == 0x03)
    {
      opc = bits (aarch64_insn_r->aarch64_insn, 22, 23);
      if (!(opc >> 1))
        if (opc & 0x01)
          ld_flag = 0x01;
        else
          ld_flag = 0x0;
      else
        if (size_bits != 0x03)
          ld_flag = 0x01;
        else
          return AARCH64_RECORD_UNKNOWN;

      if (record_debug)
	{
	  debug_printf ("Process record: load/store (unsigned immediate):"
			" size %x V %d opc %x\n", size_bits, vector_flag,
			opc);
	}

      if (!ld_flag)
        {
          offset = bits (aarch64_insn_r->aarch64_insn, 10, 21);
          datasize = 8 << size_bits;
          regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
                                      &address);
          offset = offset << size_bits;
          address = address + offset;

          record_buf_mem[0] = datasize >> 3;
          record_buf_mem[1] = address;
          aarch64_insn_r->mem_rec_count = 1;
        }
      else
        {
          if (vector_flag)
            record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
          else
            record_buf[0] = reg_rt;
          aarch64_insn_r->reg_rec_count = 1;
        }
    }
  /* Load/store register (register offset) instructions.  */
  else if ((insn_bits24_27 & 0x0b) == 0x08 && insn_bits28_29 == 0x03
	   && insn_bits10_11 == 0x02 && insn_bit21)
    {
      if (record_debug)
	debug_printf ("Process record: load/store (register offset)\n");
      opc = bits (aarch64_insn_r->aarch64_insn, 22, 23);
      if (!(opc >> 1))
        if (opc & 0x01)
          ld_flag = 0x01;
        else
          ld_flag = 0x0;
      else
        if (size_bits != 0x03)
          ld_flag = 0x01;
        else
          return AARCH64_RECORD_UNKNOWN;

      if (!ld_flag)
        {
          ULONGEST reg_rm_val;

          regcache_raw_read_unsigned (aarch64_insn_r->regcache,
                     bits (aarch64_insn_r->aarch64_insn, 16, 20), &reg_rm_val);
          if (bit (aarch64_insn_r->aarch64_insn, 12))
            offset = reg_rm_val << size_bits;
          else
            offset = reg_rm_val;
          datasize = 8 << size_bits;
          regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
                                      &address);
          address = address + offset;
          record_buf_mem[0] = datasize >> 3;
          record_buf_mem[1] = address;
          aarch64_insn_r->mem_rec_count = 1;
        }
      else
        {
          if (vector_flag)
            record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
          else
            record_buf[0] = reg_rt;
          aarch64_insn_r->reg_rec_count = 1;
        }
    }
  /* Load/store register (immediate and unprivileged) instructions.  */
  else if ((insn_bits24_27 & 0x0b) == 0x08 && insn_bits28_29 == 0x03
	   && !insn_bit21)
    {
      if (record_debug)
	{
	  debug_printf ("Process record: load/store "
			"(immediate and unprivileged)\n");
	}
      opc = bits (aarch64_insn_r->aarch64_insn, 22, 23);
      if (!(opc >> 1))
        if (opc & 0x01)
          ld_flag = 0x01;
        else
          ld_flag = 0x0;
      else
        if (size_bits != 0x03)
          ld_flag = 0x01;
        else
          return AARCH64_RECORD_UNKNOWN;

      if (!ld_flag)
        {
          uint16_t imm9_off;
          imm9_off = bits (aarch64_insn_r->aarch64_insn, 12, 20);
          offset = (imm9_off & 0x0100) ? (((~imm9_off) & 0x01ff) + 1) : imm9_off;
          datasize = 8 << size_bits;
          regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
                                      &address);
          if (insn_bits10_11 != 0x01)
            {
              if (imm9_off & 0x0100)
                address = address - offset;
              else
                address = address + offset;
            }
          record_buf_mem[0] = datasize >> 3;
          record_buf_mem[1] = address;
          aarch64_insn_r->mem_rec_count = 1;
        }
      else
        {
          if (vector_flag)
            record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
          else
            record_buf[0] = reg_rt;
          aarch64_insn_r->reg_rec_count = 1;
        }
      if (insn_bits10_11 == 0x01 || insn_bits10_11 == 0x03)
        record_buf[aarch64_insn_r->reg_rec_count++] = reg_rn;
    }
  /* Advanced SIMD load/store instructions.  */
  else
    return aarch64_record_asimd_load_store (aarch64_insn_r);

  MEM_ALLOC (aarch64_insn_r->aarch64_mems, aarch64_insn_r->mem_rec_count,
             record_buf_mem);
  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
             record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for data processing SIMD and floating point instructions.  */

static unsigned int
aarch64_record_data_proc_simd_fp (insn_decode_record *aarch64_insn_r)
{
  uint8_t insn_bit21, opcode, rmode, reg_rd;
  uint8_t insn_bits24_27, insn_bits28_31, insn_bits10_11, insn_bits12_15;
  uint8_t insn_bits11_14;
  uint32_t record_buf[2];

  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);
  insn_bits28_31 = bits (aarch64_insn_r->aarch64_insn, 28, 31);
  insn_bits10_11 = bits (aarch64_insn_r->aarch64_insn, 10, 11);
  insn_bits12_15 = bits (aarch64_insn_r->aarch64_insn, 12, 15);
  insn_bits11_14 = bits (aarch64_insn_r->aarch64_insn, 11, 14);
  opcode = bits (aarch64_insn_r->aarch64_insn, 16, 18);
  rmode = bits (aarch64_insn_r->aarch64_insn, 19, 20);
  reg_rd = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  insn_bit21 = bit (aarch64_insn_r->aarch64_insn, 21);

  if (record_debug)
    debug_printf ("Process record: data processing SIMD/FP: ");

  if ((insn_bits28_31 & 0x05) == 0x01 && insn_bits24_27 == 0x0e)
    {
      /* Floating point - fixed point conversion instructions.  */
      if (!insn_bit21)
	{
	  if (record_debug)
	    debug_printf ("FP - fixed point conversion");

	  if ((opcode >> 1) == 0x0 && rmode == 0x03)
	    record_buf[0] = reg_rd;
	  else
	    record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
	}
      /* Floating point - conditional compare instructions.  */
      else if (insn_bits10_11 == 0x01)
	{
	  if (record_debug)
	    debug_printf ("FP - conditional compare");

	  record_buf[0] = AARCH64_CPSR_REGNUM;
	}
      /* Floating point - data processing (2-source) and
         conditional select instructions.  */
      else if (insn_bits10_11 == 0x02 || insn_bits10_11 == 0x03)
	{
	  if (record_debug)
	    debug_printf ("FP - DP (2-source)");

	  record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
	}
      else if (insn_bits10_11 == 0x00)
	{
	  /* Floating point - immediate instructions.  */
	  if ((insn_bits12_15 & 0x01) == 0x01
	      || (insn_bits12_15 & 0x07) == 0x04)
	    {
	      if (record_debug)
		debug_printf ("FP - immediate");
	      record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
	    }
	  /* Floating point - compare instructions.  */
	  else if ((insn_bits12_15 & 0x03) == 0x02)
	    {
	      if (record_debug)
		debug_printf ("FP - immediate");
	      record_buf[0] = AARCH64_CPSR_REGNUM;
	    }
	  /* Floating point - integer conversions instructions.  */
	  else if (insn_bits12_15 == 0x00)
	    {
	      /* Convert float to integer instruction.  */
	      if (!(opcode >> 1) || ((opcode >> 1) == 0x02 && !rmode))
		{
		  if (record_debug)
		    debug_printf ("float to int conversion");

		  record_buf[0] = reg_rd + AARCH64_X0_REGNUM;
		}
	      /* Convert integer to float instruction.  */
	      else if ((opcode >> 1) == 0x01 && !rmode)
		{
		  if (record_debug)
		    debug_printf ("int to float conversion");

		  record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
		}
	      /* Move float to integer instruction.  */
	      else if ((opcode >> 1) == 0x03)
		{
		  if (record_debug)
		    debug_printf ("move float to int");

		  if (!(opcode & 0x01))
		    record_buf[0] = reg_rd + AARCH64_X0_REGNUM;
		  else
		    record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
		}
	      else
		return AARCH64_RECORD_UNKNOWN;
            }
	  else
	    return AARCH64_RECORD_UNKNOWN;
        }
      else
	return AARCH64_RECORD_UNKNOWN;
    }
  else if ((insn_bits28_31 & 0x09) == 0x00 && insn_bits24_27 == 0x0e)
    {
      if (record_debug)
	debug_printf ("SIMD copy");

      /* Advanced SIMD copy instructions.  */
      if (!bits (aarch64_insn_r->aarch64_insn, 21, 23)
	  && !bit (aarch64_insn_r->aarch64_insn, 15)
	  && bit (aarch64_insn_r->aarch64_insn, 10))
	{
	  if (insn_bits11_14 == 0x05 || insn_bits11_14 == 0x07)
	    record_buf[0] = reg_rd + AARCH64_X0_REGNUM;
	  else
	    record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
	}
      else
	record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
    }
  /* All remaining floating point or advanced SIMD instructions.  */
  else
    {
      if (record_debug)
	debug_printf ("all remain");

      record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
    }

  if (record_debug)
    debug_printf ("\n");

  aarch64_insn_r->reg_rec_count++;
  gdb_assert (aarch64_insn_r->reg_rec_count == 1);
  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Decodes insns type and invokes its record handler.  */

static unsigned int
aarch64_record_decode_insn_handler (insn_decode_record *aarch64_insn_r)
{
  uint32_t ins_bit25, ins_bit26, ins_bit27, ins_bit28;

  ins_bit25 = bit (aarch64_insn_r->aarch64_insn, 25);
  ins_bit26 = bit (aarch64_insn_r->aarch64_insn, 26);
  ins_bit27 = bit (aarch64_insn_r->aarch64_insn, 27);
  ins_bit28 = bit (aarch64_insn_r->aarch64_insn, 28);

  /* Data processing - immediate instructions.  */
  if (!ins_bit26 && !ins_bit27 && ins_bit28)
    return aarch64_record_data_proc_imm (aarch64_insn_r);

  /* Branch, exception generation and system instructions.  */
  if (ins_bit26 && !ins_bit27 && ins_bit28)
    return aarch64_record_branch_except_sys (aarch64_insn_r);

  /* Load and store instructions.  */
  if (!ins_bit25 && ins_bit27)
    return aarch64_record_load_store (aarch64_insn_r);

  /* Data processing - register instructions.  */
  if (ins_bit25 && !ins_bit26 && ins_bit27)
    return aarch64_record_data_proc_reg (aarch64_insn_r);

  /* Data processing - SIMD and floating point instructions.  */
  if (ins_bit25 && ins_bit26 && ins_bit27)
    return aarch64_record_data_proc_simd_fp (aarch64_insn_r);

  return AARCH64_RECORD_UNSUPPORTED;
}

/* Cleans up local record registers and memory allocations.  */

static void
deallocate_reg_mem (insn_decode_record *record)
{
  xfree (record->aarch64_regs);
  xfree (record->aarch64_mems);
}

/* Parse the current instruction and record the values of the registers and
   memory that will be changed in current instruction to record_arch_list
   return -1 if something is wrong.  */

int
aarch64_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
			CORE_ADDR insn_addr)
{
  uint32_t rec_no = 0;
  uint8_t insn_size = 4;
  uint32_t ret = 0;
  gdb_byte buf[insn_size];
  insn_decode_record aarch64_record;

  memset (&buf[0], 0, insn_size);
  memset (&aarch64_record, 0, sizeof (insn_decode_record));
  target_read_memory (insn_addr, &buf[0], insn_size);
  aarch64_record.aarch64_insn
    = (uint32_t) extract_unsigned_integer (&buf[0],
					   insn_size,
					   gdbarch_byte_order (gdbarch));
  aarch64_record.regcache = regcache;
  aarch64_record.this_addr = insn_addr;
  aarch64_record.gdbarch = gdbarch;

  ret = aarch64_record_decode_insn_handler (&aarch64_record);
  if (ret == AARCH64_RECORD_UNSUPPORTED)
    {
      printf_unfiltered (_("Process record does not support instruction "
			   "0x%0x at address %s.\n"),
			 aarch64_record.aarch64_insn,
			 paddress (gdbarch, insn_addr));
      ret = -1;
    }

  if (0 == ret)
    {
      /* Record registers.  */
      record_full_arch_list_add_reg (aarch64_record.regcache,
				     AARCH64_PC_REGNUM);
      /* Always record register CPSR.  */
      record_full_arch_list_add_reg (aarch64_record.regcache,
				     AARCH64_CPSR_REGNUM);
      if (aarch64_record.aarch64_regs)
	for (rec_no = 0; rec_no < aarch64_record.reg_rec_count; rec_no++)
	  if (record_full_arch_list_add_reg (aarch64_record.regcache,
					     aarch64_record.aarch64_regs[rec_no]))
	    ret = -1;

      /* Record memories.  */
      if (aarch64_record.aarch64_mems)
	for (rec_no = 0; rec_no < aarch64_record.mem_rec_count; rec_no++)
	  if (record_full_arch_list_add_mem
	      ((CORE_ADDR)aarch64_record.aarch64_mems[rec_no].addr,
	       aarch64_record.aarch64_mems[rec_no].len))
	    ret = -1;

      if (record_full_arch_list_add_end ())
	ret = -1;
    }

  deallocate_reg_mem (&aarch64_record);
  return ret;
}
