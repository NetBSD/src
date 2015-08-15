/* Target-machine dependent code for Nios II, for GDB.
   Copyright (C) 2012-2014 Free Software Foundation, Inc.
   Contributed by Peter Brookes (pbrookes@altera.com)
   and Andrew Draper (adraper@altera.com).
   Contributed by Mentor Graphics, Inc.

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
#include "trad-frame.h"
#include "dwarf2-frame.h"
#include "symtab.h"
#include "inferior.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "osabi.h"
#include "target.h"
#include "dis-asm.h"
#include "regcache.h"
#include "value.h"
#include "symfile.h"
#include "arch-utils.h"
#include "floatformat.h"
#include "gdb_assert.h"
#include "infcall.h"
#include "regset.h"
#include "target-descriptions.h"

/* To get entry_point_address.  */
#include "objfiles.h"

/* Nios II ISA specific encodings and macros.  */
#include "opcode/nios2.h"

/* Nios II specific header.  */
#include "nios2-tdep.h"

#include "features/nios2.c"

/* Control debugging information emitted in this file.  */

static int nios2_debug = 0;

/* The following structures are used in the cache for prologue
   analysis; see the reg_value and reg_saved tables in
   struct nios2_unwind_cache, respectively.  */

/* struct reg_value is used to record that a register has the same value
   as reg at the given offset from the start of a function.  */

struct reg_value
{
  int reg;
  unsigned int offset;
};

/* struct reg_saved is used to record that a register value has been saved at
   basereg + addr, for basereg >= 0.  If basereg < 0, that indicates
   that the register is not known to have been saved.  Note that when
   basereg == NIOS2_Z_REGNUM (that is, r0, which holds value 0),
   addr is an absolute address.  */

struct reg_saved
{
  int basereg;
  CORE_ADDR addr;
};

struct nios2_unwind_cache
{
  /* The frame's base, optionally used by the high-level debug info.  */
  CORE_ADDR base;

  /* The previous frame's inner most stack address.  Used as this
     frame ID's stack_addr.  */
  CORE_ADDR cfa;

  /* The address of the first instruction in this function.  */
  CORE_ADDR pc;

  /* Which register holds the return address for the frame.  */
  int return_regnum;

  /* Table indicating what changes have been made to each register.  */
  struct reg_value reg_value[NIOS2_NUM_REGS];

  /* Table indicating where each register has been saved.  */
  struct reg_saved reg_saved[NIOS2_NUM_REGS];
};


/* This array is a mapping from Dwarf-2 register numbering to GDB's.  */

static int nios2_dwarf2gdb_regno_map[] =
{
  0, 1, 2, 3,
  4, 5, 6, 7,
  8, 9, 10, 11,
  12, 13, 14, 15,
  16, 17, 18, 19,
  20, 21, 22, 23,
  24, 25,
  NIOS2_GP_REGNUM,        /* 26 */
  NIOS2_SP_REGNUM,        /* 27 */
  NIOS2_FP_REGNUM,        /* 28 */
  NIOS2_EA_REGNUM,        /* 29 */
  NIOS2_BA_REGNUM,        /* 30 */
  NIOS2_RA_REGNUM,        /* 31 */
  NIOS2_PC_REGNUM,        /* 32 */
  NIOS2_STATUS_REGNUM,    /* 33 */
  NIOS2_ESTATUS_REGNUM,   /* 34 */
  NIOS2_BSTATUS_REGNUM,   /* 35 */
  NIOS2_IENABLE_REGNUM,   /* 36 */
  NIOS2_IPENDING_REGNUM,  /* 37 */
  NIOS2_CPUID_REGNUM,     /* 38 */
  39, /* CTL6 */          /* 39 */
  NIOS2_EXCEPTION_REGNUM, /* 40 */
  NIOS2_PTEADDR_REGNUM,   /* 41 */
  NIOS2_TLBACC_REGNUM,    /* 42 */
  NIOS2_TLBMISC_REGNUM,   /* 43 */
  NIOS2_ECCINJ_REGNUM,    /* 44 */
  NIOS2_BADADDR_REGNUM,   /* 45 */
  NIOS2_CONFIG_REGNUM,    /* 46 */
  NIOS2_MPUBASE_REGNUM,   /* 47 */
  NIOS2_MPUACC_REGNUM     /* 48 */
};


/* Implement the dwarf2_reg_to_regnum gdbarch method.  */

static int
nios2_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int dw_reg)
{
  if (dw_reg < 0 || dw_reg > NIOS2_NUM_REGS)
    {
      warning (_("Dwarf-2 uses unmapped register #%d"), dw_reg);
      return dw_reg;
    }

  return nios2_dwarf2gdb_regno_map[dw_reg];
}

/* Canonical names for the 49 registers.  */

static const char *const nios2_reg_names[NIOS2_NUM_REGS] =
{
  "zero", "at", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "et", "bt", "gp", "sp", "fp", "ea", "sstatus", "ra",
  "pc",
  "status", "estatus", "bstatus", "ienable",
  "ipending", "cpuid", "ctl6", "exception",
  "pteaddr", "tlbacc", "tlbmisc", "eccinj",
  "badaddr", "config", "mpubase", "mpuacc"
};

/* Implement the register_name gdbarch method.  */

static const char *
nios2_register_name (struct gdbarch *gdbarch, int regno)
{
  /* Use mnemonic aliases for GPRs.  */
  if (regno >= 0 && regno < NIOS2_NUM_REGS)
    return nios2_reg_names[regno];
  else
    return tdesc_register_name (gdbarch, regno);
}

/* Implement the register_type gdbarch method.  */

static struct type *
nios2_register_type (struct gdbarch *gdbarch, int regno)
{
  /* If the XML description has register information, use that to
     determine the register type.  */
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_type (gdbarch, regno);

  if (regno == NIOS2_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else if (regno == NIOS2_SP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;
  else
    return builtin_type (gdbarch)->builtin_uint32;
}

/* Given a return value in REGCACHE with a type VALTYPE,
   extract and copy its value into VALBUF.  */

static void
nios2_extract_return_value (struct gdbarch *gdbarch, struct type *valtype,
			    struct regcache *regcache, gdb_byte *valbuf)
{
  int len = TYPE_LENGTH (valtype);

  /* Return values of up to 8 bytes are returned in $r2 $r3.  */
  if (len <= register_size (gdbarch, NIOS2_R2_REGNUM))
    regcache_cooked_read (regcache, NIOS2_R2_REGNUM, valbuf);
  else
    {
      gdb_assert (len <= (register_size (gdbarch, NIOS2_R2_REGNUM)
			  + register_size (gdbarch, NIOS2_R3_REGNUM)));
      regcache_cooked_read (regcache, NIOS2_R2_REGNUM, valbuf);
      regcache_cooked_read (regcache, NIOS2_R3_REGNUM, valbuf + 4);
    }
}

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

static void
nios2_store_return_value (struct gdbarch *gdbarch, struct type *valtype,
			  struct regcache *regcache, const gdb_byte *valbuf)
{
  int len = TYPE_LENGTH (valtype);

  /* Return values of up to 8 bytes are returned in $r2 $r3.  */
  if (len <= register_size (gdbarch, NIOS2_R2_REGNUM))
    regcache_cooked_write (regcache, NIOS2_R2_REGNUM, valbuf);
  else
    {
      gdb_assert (len <= (register_size (gdbarch, NIOS2_R2_REGNUM)
			  + register_size (gdbarch, NIOS2_R3_REGNUM)));
      regcache_cooked_write (regcache, NIOS2_R2_REGNUM, valbuf);
      regcache_cooked_write (regcache, NIOS2_R3_REGNUM, valbuf + 4);
    }
}


/* Set up the default values of the registers.  */

static void
nios2_setup_default (struct nios2_unwind_cache *cache)
{
  int i;

  for (i = 0; i < NIOS2_NUM_REGS; i++)
  {
    /* All registers start off holding their previous values.  */
    cache->reg_value[i].reg    = i;
    cache->reg_value[i].offset = 0;

    /* All registers start off not saved.  */
    cache->reg_saved[i].basereg = -1;
    cache->reg_saved[i].addr    = 0;
  }
}

/* Initialize the unwind cache.  */

static void
nios2_init_cache (struct nios2_unwind_cache *cache, CORE_ADDR pc)
{
  cache->base = 0;
  cache->cfa = 0;
  cache->pc = pc;
  cache->return_regnum = NIOS2_RA_REGNUM;
  nios2_setup_default (cache);
}

/* Helper function to identify when we're in a function epilogue;
   that is, the part of the function from the point at which the
   stack adjustment is made, to the return or sibcall.  On Nios II,
   we want to check that the CURRENT_PC is a return-type instruction
   and that the previous instruction is a stack adjustment.
   START_PC is the beginning of the function in question.  */

static int
nios2_in_epilogue_p (struct gdbarch *gdbarch,
		     CORE_ADDR current_pc,
		     CORE_ADDR start_pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* There has to be a previous instruction in the function.  */
  if (current_pc > start_pc)
    {

      /* Check whether the previous instruction was a stack
	 adjustment.  */
      unsigned int insn
        = read_memory_unsigned_integer (current_pc - NIOS2_OPCODE_SIZE,
					NIOS2_OPCODE_SIZE, byte_order);

      if ((insn & 0xffc0003c) == 0xdec00004	/* ADDI sp, sp, */
	  || (insn & 0xffc1ffff) == 0xdec1883a	/* ADD  sp, sp, */
	  || (insn & 0xffc0003f) == 0xdec00017)	/* LDW  sp, constant(sp) */
	{
	  /* Then check if it's followed by a return or a tail
	     call.  */
          insn = read_memory_unsigned_integer (current_pc, NIOS2_OPCODE_SIZE,
					       byte_order);

	  if (insn == 0xf800283a			/* RET */
	      || insn == 0xe800083a			/* ERET */
	      || (insn & 0x07ffffff) == 0x0000683a	/* JMP */
	      || (insn & 0xffc0003f) == 6)		/* BR */
	    return 1;
	}
    }
  return 0;
}

/* Implement the in_function_epilogue_p gdbarch method.  */

static int
nios2_in_function_epilogue_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr;

  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    return nios2_in_epilogue_p (gdbarch, pc, func_addr);

  return 0;
}

/* Define some instruction patterns supporting wildcard bits via a
   mask.  */

typedef struct
{
  unsigned int insn;
  unsigned int mask;
} wild_insn;

static const wild_insn profiler_insn[] =
{
  { 0x0010e03a, 0x00000000 }, /* nextpc r8 */
  { 0xf813883a, 0x00000000 }, /* mov    r9,ra */
  { 0x02800034, 0x003fffc0 }, /* movhi  r10,257 */
  { 0x52800004, 0x003fffc0 }, /* addi   r10,r10,-31992 */
  { 0x00000000, 0xffffffc0 }, /* call   <mcount> */
  { 0x483f883a, 0x00000000 }  /* mov    ra,r9 */
};

static const wild_insn irqentry_insn[] =
{
  { 0x0031307a, 0x00000000 }, /* rdctl  et,estatus */
  { 0xc600004c, 0x00000000 }, /* andi   et,et,1 */
  { 0xc0000026, 0x003fffc0 }, /* beq    et,zero, <software_exception> */
  { 0x0031313a, 0x00000000 }, /* rdctl  et,ipending */
  { 0xc0000026, 0x003fffc0 }  /* beq    et,zero, <software_exception> */
};


/* Attempt to match SEQUENCE, which is COUNT insns long, at START_PC.  */

static int
nios2_match_sequence (struct gdbarch *gdbarch, CORE_ADDR start_pc,
		      const wild_insn *sequence, int count)
{
  CORE_ADDR pc = start_pc;
  int i;
  unsigned int insn;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  for (i = 0 ; i < count ; i++)
    {
      insn = read_memory_unsigned_integer (pc, NIOS2_OPCODE_SIZE, byte_order);
      if ((insn & ~sequence[i].mask) != sequence[i].insn)
	return 0;

      pc += NIOS2_OPCODE_SIZE;
    }

  return 1;
}

/* Do prologue analysis, returning the PC of the first instruction
   after the function prologue.  Assumes CACHE has already been
   initialized.  THIS_FRAME can be null, in which case we are only
   interested in skipping the prologue.  Otherwise CACHE is filled in
   from the frame information.

   The prologue will consist of the following parts:
     1) Optional profiling instrumentation.  The old version uses six
        instructions.  We step over this if there is an exact match.
	  nextpc r8
	  mov	 r9, ra
	  movhi	 r10, %hiadj(.LP2)
	  addi	 r10, r10, %lo(.LP2)
	  call	 mcount
	  mov	 ra, r9
	The new version uses two or three instructions (the last of
	these might get merged in with the STW which saves RA to the
	stack).  We interpret these.
	  mov	 r8, ra
	  call	 mcount
	  mov	 ra, r8

     2) Optional interrupt entry decision.  Again, we step over
        this if there is an exact match.
	  rdctl  et,estatus
	  andi   et,et,1
	  beq    et,zero, <software_exception>
	  rdctl  et,ipending
	  beq    et,zero, <software_exception>

     3) A stack adjustment or stack which, which will be one of:
	  addi   sp, sp, -constant
	or:
	  movi   r8, constant
	  sub    sp, sp, r8
	or
	  movhi  r8, constant
	  addi   r8, r8, constant
	  sub    sp, sp, r8
	or
	  movhi  rx, %hiadj(newstack)
	  addhi  rx, rx, %lo(newstack)
	  stw    sp, constant(rx)
	  mov    sp, rx

     4) An optional stack check, which can take either of these forms:
	  bgeu   sp, rx, +8
	  break  3
	or
	  bltu   sp, rx, .Lstack_overflow
	  ...
	.Lstack_overflow:
	  break  3

     5) Saving any registers which need to be saved.  These will
        normally just be stored onto the stack:
	  stw    rx, constant(sp)
	but in the large frame case will use r8 as an offset back
	to the cfa:
	  add    r8, r8, sp
	  stw    rx, -constant(r8)

	Saving control registers looks slightly different:
	  rdctl  rx, ctlN
	  stw    rx, constant(sp)

     6) An optional FP setup, either if the user has requested a
        frame pointer or if the function calls alloca.
        This is always:
	  mov    fp, sp

    The prologue instructions may be interleaved, and the register
    saves and FP setup can occur in either order.

    To cope with all this variability we decode all the instructions
    from the start of the prologue until we hit a branch, call or
    return.  For each of the instructions mentioned in 3, 4 and 5 we
    handle the limited cases of stores to the stack and operations
    on constant values.  */

static CORE_ADDR
nios2_analyze_prologue (struct gdbarch *gdbarch, const CORE_ADDR start_pc,
			const CORE_ADDR current_pc,
			struct nios2_unwind_cache *cache,
			struct frame_info *this_frame)
{
  /* Maximum lines of prologue to check.
     Note that this number should not be too large, else we can
     potentially end up iterating through unmapped memory.  */
  CORE_ADDR limit_pc = start_pc + 200;
  int regno;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Does the frame set up the FP register?  */
  int base_reg = 0;

  struct reg_value *value = cache->reg_value;
  struct reg_value temp_value[NIOS2_NUM_REGS];

  int i;

  /* Save the starting PC so we can correct the pc after running
     through the prolog, using symbol info.  */
  CORE_ADDR pc = start_pc;

  /* Is this an exception handler?  */
  int exception_handler = 0;

  /* What was the original value of SP (or fake original value for
     functions which switch stacks?  */
  CORE_ADDR frame_high;

  /* Is this the end of the prologue?  */
  int within_prologue = 1;

  CORE_ADDR prologue_end;

  /* Is this the innermost function?  */
  int innermost = (this_frame ? (frame_relative_level (this_frame) == 0) : 1);

  if (nios2_debug)
    fprintf_unfiltered (gdb_stdlog,
			"{ nios2_analyze_prologue start=%s, current=%s ",
			paddress (gdbarch, start_pc),
			paddress (gdbarch, current_pc));

  /* Set up the default values of the registers.  */
  nios2_setup_default (cache);

  /* If the first few instructions are the profile entry, then skip
     over them.  Newer versions of the compiler use more efficient
     profiling code.  */
  if (nios2_match_sequence (gdbarch, pc, profiler_insn,
			    ARRAY_SIZE (profiler_insn)))
    pc += ARRAY_SIZE (profiler_insn) * NIOS2_OPCODE_SIZE;

  /* If the first few instructions are an interrupt entry, then skip
     over them too.  */
  if (nios2_match_sequence (gdbarch, pc, irqentry_insn,
			    ARRAY_SIZE (irqentry_insn)))
    {
      pc += ARRAY_SIZE (irqentry_insn) * NIOS2_OPCODE_SIZE;
      exception_handler = 1;
    }

  prologue_end = start_pc;

  /* Find the prologue instructions.  */
  while (pc < limit_pc && within_prologue)
    {
      /* Present instruction.  */
      uint32_t insn;

      int prologue_insn = 0;

      if (pc == current_pc)
      {
	/* When we reach the current PC we must save the current
	   register state (for the backtrace) but keep analysing
	   because there might be more to find out (eg. is this an
	   exception handler).  */
	memcpy (temp_value, value, sizeof (temp_value));
	value = temp_value;
	if (nios2_debug)
	  fprintf_unfiltered (gdb_stdlog, "*");
      }

      insn = read_memory_unsigned_integer (pc, NIOS2_OPCODE_SIZE, byte_order);
      pc += NIOS2_OPCODE_SIZE;

      if (nios2_debug)
	fprintf_unfiltered (gdb_stdlog, "[%08X]", insn);

      /* The following instructions can appear in the prologue.  */

      if ((insn & 0x0001ffff) == 0x0001883a)
	{
	  /* ADD   rc, ra, rb  (also used for MOV) */

	  int ra = GET_IW_A (insn);
	  int rb = GET_IW_B (insn);
	  int rc = GET_IW_C (insn);

	  if (rc == NIOS2_SP_REGNUM
	      && rb == 0
	      && value[ra].reg == cache->reg_saved[NIOS2_SP_REGNUM].basereg)
	    {
	      /* If the previous value of SP is available somewhere
		 near the new stack pointer value then this is a
		 stack switch.  */

	      /* If any registers were saved on the stack before then
		 we can't backtrace into them now.  */
	      for (i = 0 ; i < NIOS2_NUM_REGS ; i++)
		{
		  if (cache->reg_saved[i].basereg == NIOS2_SP_REGNUM)
		    cache->reg_saved[i].basereg = -1;
		  if (value[i].reg == NIOS2_SP_REGNUM)
		    value[i].reg = -1;
		}

	      /* Create a fake "high water mark" 4 bytes above where SP
		 was stored and fake up the registers to be consistent
		 with that.  */
	      value[NIOS2_SP_REGNUM].reg = NIOS2_SP_REGNUM;
	      value[NIOS2_SP_REGNUM].offset
		= (value[ra].offset
		   - cache->reg_saved[NIOS2_SP_REGNUM].addr
		   - 4);
	      cache->reg_saved[NIOS2_SP_REGNUM].basereg = NIOS2_SP_REGNUM;
	      cache->reg_saved[NIOS2_SP_REGNUM].addr = -4;
	    }

	  else if (rc != 0)
	    {
	      if (value[rb].reg == 0)
		value[rc].reg = value[ra].reg;
	      else if (value[ra].reg == 0)
		value[rc].reg = value[rb].reg;
	      else
		value[rc].reg = -1;
	      value[rc].offset = value[ra].offset + value[rb].offset;
	    }
	  prologue_insn = 1;
	}

      else if ((insn & 0x0001ffff) == 0x0001983a)
	{
	  /* SUB   rc, ra, rb */

	  int ra = GET_IW_A (insn);
	  int rb = GET_IW_B (insn);
	  int rc = GET_IW_C (insn);

	  if (rc != 0)
	    {
	      if (value[rb].reg == 0)
		value[rc].reg = value[ra].reg;
	      else
		value[rc].reg = -1;
	      value[rc].offset = value[ra].offset - value[rb].offset;
	    }
	}

      else if ((insn & 0x0000003f) == 0x00000004)
	{
	  /* ADDI  rb, ra, immed   (also used for MOVI) */
	  short immed = GET_IW_IMM16 (insn);
	  int ra = GET_IW_A (insn);
	  int rb = GET_IW_B (insn);

	  /* The first stack adjustment is part of the prologue.
	     Any subsequent stack adjustments are either down to
	     alloca or the epilogue so stop analysing when we hit
	     them.  */
	  if (rb == NIOS2_SP_REGNUM
	      && (value[rb].offset != 0 || value[ra].reg != NIOS2_SP_REGNUM))
	    break;

	  if (rb != 0)
	    {
	      value[rb].reg    = value[ra].reg;
	      value[rb].offset = value[ra].offset + immed;
	    }

	  prologue_insn = 1;
	}

      else if ((insn & 0x0000003f) == 0x00000034)
	{
	  /* ORHI  rb, ra, immed   (also used for MOVHI) */
	  unsigned int immed = GET_IW_IMM16 (insn);
	  int ra = GET_IW_A (insn);
	  int rb = GET_IW_B (insn);

	  if (rb != 0)
	    {
  	      value[rb].reg    = (value[ra].reg == 0) ? 0 : -1;
	      value[rb].offset = value[ra].offset | (immed << 16);
	    }
	}

      else if ((insn & IW_OP_MASK) == OP_STW
	       || (insn & IW_OP_MASK) == OP_STWIO)
        {
	  /* STW rb, immediate(ra) */

	  short immed16 = GET_IW_IMM16 (insn);
	  int ra = GET_IW_A (insn);
	  int rb = GET_IW_B (insn);

	  /* Are we storing the original value of a register?
	     For exception handlers the value of EA-4 (return
	     address from interrupts etc) is sometimes stored.  */
	  int orig = value[rb].reg;
	  if (orig > 0
	      && (value[rb].offset == 0
		  || (orig == NIOS2_EA_REGNUM && value[rb].offset == -4)))
	    {
	      /* We are most interested in stores to the stack, but
		 also take note of stores to other places as they
		 might be useful later.  */
	      if ((value[ra].reg == NIOS2_SP_REGNUM
		   && cache->reg_saved[orig].basereg != NIOS2_SP_REGNUM)
		  || cache->reg_saved[orig].basereg == -1)
		{
		  if (pc < current_pc)
		    {
		      /* Save off callee saved registers.  */
		      cache->reg_saved[orig].basereg = value[ra].reg;
		      cache->reg_saved[orig].addr
			= value[ra].offset + GET_IW_IMM16 (insn);
		    }

		  prologue_insn = 1;

		  if (orig == NIOS2_EA_REGNUM || orig == NIOS2_ESTATUS_REGNUM)
		    exception_handler = 1;
		}
	    }
	  else
	    /* Non-stack memory writes are not part of the
	       prologue.  */
	    within_prologue = 0;
        }

      else if ((insn & 0xffc1f83f) == 0x0001303a)
	{
	  /* RDCTL rC, ctlN */
	  int rc = GET_IW_C (insn);
	  int n = GET_IW_CONTROL_REGNUM (insn);

	  if (rc != 0)
	    {
	      value[rc].reg    = NIOS2_STATUS_REGNUM + n;
	      value[rc].offset = 0;
	    }

	  prologue_insn = 1;
        }

      else if ((insn & 0x0000003f) == 0
	       && value[8].reg == NIOS2_RA_REGNUM
	       && value[8].offset == 0
	       && value[NIOS2_SP_REGNUM].reg == NIOS2_SP_REGNUM
	       && value[NIOS2_SP_REGNUM].offset == 0)
	{
	  /* A CALL instruction.  This is treated as a call to mcount
	     if ra has been stored into r8 beforehand and if it's
	     before the stack adjust.
	     Note mcount corrupts r2-r3, r9-r15 & ra.  */
	  for (i = 2 ; i <= 3 ; i++)
	    value[i].reg = -1;
	  for (i = 9 ; i <= 15 ; i++)
	    value[i].reg = -1;
	  value[NIOS2_RA_REGNUM].reg = -1;

	  prologue_insn = 1;
	}

      else if ((insn & 0xf83fffff) == 0xd800012e)
	{
	   /* BGEU sp, rx, +8
	      BREAK 3
	      This instruction sequence is used in stack checking;
	      we can ignore it.  */
	  unsigned int next_insn
	    = read_memory_unsigned_integer (pc, NIOS2_OPCODE_SIZE, byte_order);

	  if (next_insn != 0x003da0fa)
	    within_prologue = 0;
	  else
	    pc += NIOS2_OPCODE_SIZE;
	}

      else if ((insn & 0xf800003f) == 0xd8000036)
	{
	   /* BLTU sp, rx, .Lstackoverflow
	      If the location branched to holds a BREAK 3 instruction
	      then this is also stack overflow detection.  We can
	      ignore it.  */
	  CORE_ADDR target_pc = pc + ((insn & 0x3fffc0) >> 6);
	  unsigned int target_insn
	    = read_memory_unsigned_integer (target_pc, NIOS2_OPCODE_SIZE,
					    byte_order);

	  if (target_insn != 0x003da0fa)
	    within_prologue = 0;
	}

      /* Any other instructions are allowed to be moved up into the
	 prologue.  If we reach a branch, call or return then the
	 prologue is considered over.  We also consider a second stack
	 adjustment as terminating the prologue (see above).  */
      else
	{
	  switch (GET_IW_OP (insn))
	    {
	    case OP_BEQ:
	    case OP_BGE:
	    case OP_BGEU:
	    case OP_BLT:
	    case OP_BLTU:
	    case OP_BNE:
	    case OP_BR:
	    case OP_CALL:
	      within_prologue = 0;
	      break;
	    case OP_OPX:
	      if (GET_IW_OPX (insn) == OPX_RET
		  || GET_IW_OPX (insn) == OPX_ERET
		  || GET_IW_OPX (insn) == OPX_BRET
		  || GET_IW_OPX (insn) == OPX_CALLR
		  || GET_IW_OPX (insn) == OPX_JMP)
		within_prologue = 0;
	      break;
	    default:
	      break;
	    }
	}

      if (prologue_insn)
	prologue_end = pc;
    }

  /* If THIS_FRAME is NULL, we are being called from skip_prologue
     and are only interested in the PROLOGUE_END value, so just
     return that now and skip over the cache updates, which depend
     on having frame information.  */
  if (this_frame == NULL)
    return prologue_end;

  /* If we are in the function epilogue and have already popped
     registers off the stack in preparation for returning, then we
     want to go back to the original register values.  */
  if (innermost && nios2_in_epilogue_p (gdbarch, current_pc, start_pc))
    nios2_setup_default (cache);

  /* Exception handlers use a different return address register.  */
  if (exception_handler)
    cache->return_regnum = NIOS2_EA_REGNUM;

  if (nios2_debug)
    fprintf_unfiltered (gdb_stdlog, "\n-> retreg=%d, ", cache->return_regnum);

  if (cache->reg_value[NIOS2_FP_REGNUM].reg == NIOS2_SP_REGNUM)
    /* If the FP now holds an offset from the CFA then this is a
       normal frame which uses the frame pointer.  */
    base_reg = NIOS2_FP_REGNUM;
  else if (cache->reg_value[NIOS2_SP_REGNUM].reg == NIOS2_SP_REGNUM)
    /* FP doesn't hold an offset from the CFA.  If SP still holds an
       offset from the CFA then we might be in a function which omits
       the frame pointer, or we might be partway through the prologue.
       In both cases we can find the CFA using SP.  */
    base_reg = NIOS2_SP_REGNUM;
  else
    {
      /* Somehow the stack pointer has been corrupted.
	 We can't return.  */
      if (nios2_debug)
	fprintf_unfiltered (gdb_stdlog, "<can't reach cfa> }\n");
      return 0;
    }

  if (cache->reg_value[base_reg].offset == 0
      || cache->reg_saved[NIOS2_RA_REGNUM].basereg != NIOS2_SP_REGNUM
      || cache->reg_saved[cache->return_regnum].basereg != NIOS2_SP_REGNUM)
    {
      /* If the frame didn't adjust the stack, didn't save RA or
	 didn't save EA in an exception handler then it must either
	 be a leaf function (doesn't call any other functions) or it
	 can't return.  If it has called another function then it
	 can't be a leaf, so set base == 0 to indicate that we can't
	 backtrace past it.  */

      if (!innermost)
	{
	  /* If it isn't the innermost function then it can't be a
	     leaf, unless it was interrupted.  Check whether RA for
	     this frame is the same as PC.  If so then it probably
	     wasn't interrupted.  */
	  CORE_ADDR ra
	    = get_frame_register_unsigned (this_frame, NIOS2_RA_REGNUM);

	  if (ra == current_pc)
	    {
	      if (nios2_debug)
		fprintf_unfiltered
		  (gdb_stdlog,
		   "<noreturn ADJUST %s, r31@r%d+?>, r%d@r%d+?> }\n",
		   paddress (gdbarch, cache->reg_value[base_reg].offset),
		   cache->reg_saved[NIOS2_RA_REGNUM].basereg,
		   cache->return_regnum,
		   cache->reg_saved[cache->return_regnum].basereg);
	      return 0;
	    }
	}
    }

  /* Get the value of whichever register we are using for the
     base.  */
  cache->base = get_frame_register_unsigned (this_frame, base_reg);

  /* What was the value of SP at the start of this function (or just
     after the stack switch).  */
  frame_high = cache->base - cache->reg_value[base_reg].offset;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < NIOS2_NUM_REGS; i++)
    if (cache->reg_saved[i].basereg == NIOS2_SP_REGNUM)
      {
	cache->reg_saved[i].basereg = NIOS2_Z_REGNUM;
	cache->reg_saved[i].addr += frame_high;
      }

  for (i = 0; i < NIOS2_NUM_REGS; i++)
    if (cache->reg_saved[i].basereg == NIOS2_GP_REGNUM)
      {
	CORE_ADDR gp = get_frame_register_unsigned (this_frame,
						    NIOS2_GP_REGNUM);

	for ( ; i < NIOS2_NUM_REGS; i++)
	  if (cache->reg_saved[i].basereg == NIOS2_GP_REGNUM)
	    {
	      cache->reg_saved[i].basereg = NIOS2_Z_REGNUM;
	      cache->reg_saved[i].addr += gp;
	    }
      }

  /* Work out what the value of SP was on the first instruction of
     this function.  If we didn't switch stacks then this can be
     trivially computed from the base address.  */
  if (cache->reg_saved[NIOS2_SP_REGNUM].basereg == NIOS2_Z_REGNUM)
    cache->cfa
      = read_memory_unsigned_integer (cache->reg_saved[NIOS2_SP_REGNUM].addr,
				      4, byte_order);
  else
    cache->cfa = frame_high;

  /* Exception handlers restore ESTATUS into STATUS.  */
  if (exception_handler)
    {
      cache->reg_saved[NIOS2_STATUS_REGNUM]
	= cache->reg_saved[NIOS2_ESTATUS_REGNUM];
      cache->reg_saved[NIOS2_ESTATUS_REGNUM].basereg = -1;
    }

  if (nios2_debug)
    fprintf_unfiltered (gdb_stdlog, "cfa=%s }\n",
			paddress (gdbarch, cache->cfa));

  return prologue_end;
}

/* Implement the skip_prologue gdbarch hook.  */

static CORE_ADDR
nios2_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  CORE_ADDR limit_pc;
  CORE_ADDR func_addr;

  struct nios2_unwind_cache cache;

  /* See if we can determine the end of the prologue via the symbol
     table.  If so, then return either PC, or the PC after the
     prologue, whichever is greater.  */
  if (find_pc_partial_function (start_pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
        = skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
        return max (start_pc, post_prologue_pc);
    }

  /* Prologue analysis does the rest....  */
  nios2_init_cache (&cache, start_pc);
  return nios2_analyze_prologue (gdbarch, start_pc, start_pc, &cache, NULL);
}

/* Implement the breakpoint_from_pc gdbarch hook.  */

static const gdb_byte*
nios2_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *bp_addr,
			  int *bp_size)
{
  /* break encoding: 31->27  26->22  21->17  16->11 10->6 5->0 */
  /*                 00000   00000   0x1d    0x2d   11111 0x3a */
  /*                 00000   00000   11101   101101 11111 111010 */
  /* In bytes:       00000000 00111011 01101111 11111010 */
  /*                 0x0       0x3b    0x6f     0xfa */
  static const gdb_byte breakpoint_le[] = {0xfa, 0x6f, 0x3b, 0x0};
  static const gdb_byte breakpoint_be[] = {0x0, 0x3b, 0x6f, 0xfa};

  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);

  *bp_size = 4;
  if (gdbarch_byte_order_for_code (gdbarch) == BFD_ENDIAN_BIG)
    return breakpoint_be;
  else
    return breakpoint_le;
}

/* Implement the print_insn gdbarch method.  */

static int
nios2_print_insn (bfd_vma memaddr, disassemble_info *info)
{
  if (info->endian == BFD_ENDIAN_BIG)
    return print_insn_big_nios2 (memaddr, info);
  else
    return print_insn_little_nios2 (memaddr, info);
}


/* Implement the frame_align gdbarch method.  */

static CORE_ADDR
nios2_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 4);
}


/* Implement the return_value gdbarch method.  */

static enum return_value_convention
nios2_return_value (struct gdbarch *gdbarch, struct value *function,
		    struct type *type, struct regcache *regcache,
		    gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (TYPE_LENGTH (type) > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    nios2_extract_return_value (gdbarch, type, regcache, readbuf);
  if (writebuf)
    nios2_store_return_value (gdbarch, type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Implement the dummy_id gdbarch method.  */

static struct frame_id
nios2_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  return frame_id_build
    (get_frame_register_unsigned (this_frame, NIOS2_SP_REGNUM),
     get_frame_pc (this_frame));
}

/* Implement the push_dummy_call gdbarch method.  */

static CORE_ADDR
nios2_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
                       struct regcache *regcache, CORE_ADDR bp_addr,
                       int nargs, struct value **args, CORE_ADDR sp,
                       int struct_return, CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;
  CORE_ADDR func_addr = find_function_addr (function, NULL);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, NIOS2_RA_REGNUM, bp_addr);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    len += align_up (TYPE_LENGTH (value_type (args[argnum])), 4);
  sp -= len;

  /* Initialize the register pointer.  */
  argreg = NIOS2_FIRST_ARGREG;

  /* The struct_return pointer occupies the first parameter-passing
     register.  */
  if (struct_return)
    regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop through args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      gdb_byte valbuf[MAX_REGISTER_SIZE];
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (value_type (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      val = value_contents (arg);

      /* Copy the argument to general registers or the stack in
         register-sized pieces.  Large arguments are split between
         registers and stack.  */
      while (len > 0)
        {
	  int partial_len = (len < 4 ? len : 4);

	  if (argreg <= NIOS2_LAST_ARGREG)
	    {
	      /* The argument is being passed in a register.  */
	      CORE_ADDR regval = extract_unsigned_integer (val, partial_len,
							   byte_order);

	      regcache_cooked_write_unsigned (regcache, argreg, regval);
	      argreg++;
	    }
	  else
	    {
	      /* The argument is being passed on the stack.  */
	      CORE_ADDR addr = sp + stack_offset;

	      write_memory (addr, val, partial_len);
	      stack_offset += align_up (partial_len, 4);
	    }

	  len -= partial_len;
	  val += partial_len;
	}
    }

  regcache_cooked_write_signed (regcache, NIOS2_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

/* Implement the unwind_pc gdbarch method.  */

static CORE_ADDR
nios2_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  gdb_byte buf[4];

  frame_unwind_register (next_frame, NIOS2_PC_REGNUM, buf);
  return extract_typed_address (buf, builtin_type (gdbarch)->builtin_func_ptr);
}

/* Implement the unwind_sp gdbarch method.  */

static CORE_ADDR
nios2_unwind_sp (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  return frame_unwind_register_unsigned (this_frame, NIOS2_SP_REGNUM);
}

/* Use prologue analysis to fill in the register cache
   *THIS_PROLOGUE_CACHE for THIS_FRAME.  This function initializes
   *THIS_PROLOGUE_CACHE first.  */

static struct nios2_unwind_cache *
nios2_frame_unwind_cache (struct frame_info *this_frame,
			  void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR current_pc;
  struct nios2_unwind_cache *cache;
  int i;

  if (*this_prologue_cache)
    return *this_prologue_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct nios2_unwind_cache);
  *this_prologue_cache = cache;

  /* Zero all fields.  */
  nios2_init_cache (cache, get_frame_func (this_frame));

  /* Prologue analysis does the rest...  */
  current_pc = get_frame_pc (this_frame);
  if (cache->pc != 0)
    nios2_analyze_prologue (gdbarch, cache->pc, current_pc, cache, this_frame);

  return cache;
}

/* Implement the this_id function for the normal unwinder.  */

static void
nios2_frame_this_id (struct frame_info *this_frame, void **this_cache,
		     struct frame_id *this_id)
{
  struct nios2_unwind_cache *cache =
    nios2_frame_unwind_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  *this_id = frame_id_build (cache->cfa, cache->pc);
}

/* Implement the prev_register function for the normal unwinder.  */

static struct value *
nios2_frame_prev_register (struct frame_info *this_frame, void **this_cache,
			   int regnum)
{
  struct nios2_unwind_cache *cache =
    nios2_frame_unwind_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0 && regnum < NIOS2_NUM_REGS);

  /* The PC of the previous frame is stored in the RA register of
     the current frame.  Frob regnum so that we pull the value from
     the correct place.  */
  if (regnum == NIOS2_PC_REGNUM)
    regnum = cache->return_regnum;

  if (regnum == NIOS2_SP_REGNUM && cache->cfa)
    return frame_unwind_got_constant (this_frame, regnum, cache->cfa);

  /* If we've worked out where a register is stored then load it from
     there.  */
  if (cache->reg_saved[regnum].basereg == NIOS2_Z_REGNUM)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->reg_saved[regnum].addr);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

/* Implement the this_base, this_locals, and this_args hooks
   for the normal unwinder.  */

static CORE_ADDR
nios2_frame_base_address (struct frame_info *this_frame, void **this_cache)
{
  struct nios2_unwind_cache *info
    = nios2_frame_unwind_cache (this_frame, this_cache);

  return info->base;
}

/* Data structures for the normal prologue-analysis-based
   unwinder.  */

static const struct frame_unwind nios2_frame_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  nios2_frame_this_id,
  nios2_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static const struct frame_base nios2_frame_base =
{
  &nios2_frame_unwind,
  nios2_frame_base_address,
  nios2_frame_base_address,
  nios2_frame_base_address
};

/* Fill in the register cache *THIS_CACHE for THIS_FRAME for use
   in the stub unwinder.  */

static struct trad_frame_cache *
nios2_stub_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  CORE_ADDR pc;
  CORE_ADDR start_addr;
  CORE_ADDR stack_addr;
  struct trad_frame_cache *this_trad_cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int num_regs = gdbarch_num_regs (gdbarch);

  if (*this_cache != NULL)
    return *this_cache;
  this_trad_cache = trad_frame_cache_zalloc (this_frame);
  *this_cache = this_trad_cache;

  /* The return address is in the link register.  */
  trad_frame_set_reg_realreg (this_trad_cache,
                              gdbarch_pc_regnum (gdbarch),
                              NIOS2_RA_REGNUM);

  /* Frame ID, since it's a frameless / stackless function, no stack
     space is allocated and SP on entry is the current SP.  */
  pc = get_frame_pc (this_frame);
  find_pc_partial_function (pc, NULL, &start_addr, NULL);
  stack_addr = get_frame_register_unsigned (this_frame, NIOS2_SP_REGNUM);
  trad_frame_set_id (this_trad_cache, frame_id_build (start_addr, stack_addr));
  /* Assume that the frame's base is the same as the stack pointer.  */
  trad_frame_set_this_base (this_trad_cache, stack_addr);

  return this_trad_cache;
}

/* Implement the this_id function for the stub unwinder.  */

static void
nios2_stub_frame_this_id (struct frame_info *this_frame, void **this_cache,
                          struct frame_id *this_id)
{
  struct trad_frame_cache *this_trad_cache
    = nios2_stub_frame_cache (this_frame, this_cache);

  trad_frame_get_id (this_trad_cache, this_id);
}

/* Implement the prev_register function for the stub unwinder.  */

static struct value *
nios2_stub_frame_prev_register (struct frame_info *this_frame,
			        void **this_cache, int regnum)
{
  struct trad_frame_cache *this_trad_cache
    = nios2_stub_frame_cache (this_frame, this_cache);

  return trad_frame_get_register (this_trad_cache, this_frame, regnum);
}

/* Implement the sniffer function for the stub unwinder.
   This unwinder is used for cases where the normal
   prologue-analysis-based unwinder can't work,
   such as PLT stubs.  */

static int
nios2_stub_frame_sniffer (const struct frame_unwind *self,
			  struct frame_info *this_frame, void **cache)
{
  gdb_byte dummy[4];
  struct obj_section *s;
  CORE_ADDR pc = get_frame_address_in_block (this_frame);

  /* Use the stub unwinder for unreadable code.  */
  if (target_read_memory (get_frame_pc (this_frame), dummy, 4) != 0)
    return 1;

  if (in_plt_section (pc))
    return 1;

  return 0;
}

/* Implement the this_base, this_locals, and this_args hooks
   for the stub unwinder.  */

static CORE_ADDR
nios2_stub_frame_base_address (struct frame_info *this_frame, void **this_cache)
{
  struct trad_frame_cache *this_trad_cache
    = nios2_stub_frame_cache (this_frame, this_cache);

  return trad_frame_get_this_base (this_trad_cache);
}

/* Define the data structures for the stub unwinder.  */

static const struct frame_unwind nios2_stub_frame_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  nios2_stub_frame_this_id,
  nios2_stub_frame_prev_register,
  NULL,
  nios2_stub_frame_sniffer
};

static const struct frame_base nios2_stub_frame_base =
{
  &nios2_stub_frame_unwind,
  nios2_stub_frame_base_address,
  nios2_stub_frame_base_address,
  nios2_stub_frame_base_address
};

/* Helper function to read an instruction at PC.  */

static unsigned long
nios2_fetch_instruction (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  return read_memory_unsigned_integer (pc, NIOS2_OPCODE_SIZE, byte_order);
}

/* Determine where to set a single step breakpoint while considering
   branch prediction.  */

static CORE_ADDR
nios2_get_next_pc (struct frame_info *frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  unsigned long inst;
  int op;
  int imm16;
  int ra;
  int rb;
  int ras;
  int rbs;
  unsigned int rau;
  unsigned int rbu;

  inst = nios2_fetch_instruction (gdbarch, pc);
  pc += NIOS2_OPCODE_SIZE;

  imm16 = (short) GET_IW_IMM16 (inst);
  ra = GET_IW_A (inst);
  rb = GET_IW_B (inst);
  ras = get_frame_register_signed (frame, ra);
  rbs = get_frame_register_signed (frame, rb);
  rau = get_frame_register_unsigned (frame, ra);
  rbu = get_frame_register_unsigned (frame, rb);

  switch (GET_IW_OP (inst))
    {
    case OP_BEQ:
      if (ras == rbs)
	pc += imm16;
      break;

    case OP_BGE:
      if (ras >= rbs)
        pc += imm16;
      break;

    case OP_BGEU:
      if (rau >= rbu)
        pc += imm16;
      break;

    case OP_BLT:
      if (ras < rbs)
        pc += imm16;
      break;

    case OP_BLTU:
      if (rau < rbu)
        pc += imm16;
      break;

    case OP_BNE:
      if (ras != rbs)
        pc += imm16;
      break;

    case OP_BR:
      pc += imm16;
      break;

    case OP_JMPI:
    case OP_CALL:
      pc = (pc & 0xf0000000) | (GET_IW_IMM26 (inst) << 2);
      break;

    case OP_OPX:
      switch (GET_IW_OPX (inst))
	{
	case OPX_JMP:
	case OPX_CALLR:
	case OPX_RET:
	  pc = ras;
	  break;

	case OPX_TRAP:
	  if (tdep->syscall_next_pc != NULL)
	    return tdep->syscall_next_pc (frame);

	default:
	  break;
	}
      break;
    default:
      break;
    }
  return pc;
}

/* Implement the software_single_step gdbarch method.  */

static int
nios2_software_single_step (struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct address_space *aspace = get_frame_address_space (frame);
  CORE_ADDR next_pc = nios2_get_next_pc (frame, get_frame_pc (frame));

  insert_single_step_breakpoint (gdbarch, aspace, next_pc);

  return 1;
}

/* Implement the get_longjump_target gdbarch method.  */

static int
nios2_get_longjmp_target (struct frame_info *frame, CORE_ADDR *pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR jb_addr = get_frame_register_unsigned (frame, NIOS2_R4_REGNUM);
  gdb_byte buf[4];

  if (target_read_memory (jb_addr + (tdep->jb_pc * 4), buf, 4))
    return 0;

  *pc = extract_unsigned_integer (buf, 4, byte_order);
  return 1;
}

/* Initialize the Nios II gdbarch.  */

static struct gdbarch *
nios2_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  int register_bytes, i;
  struct tdesc_arch_data *tdesc_data = NULL;
  const struct target_desc *tdesc = info.target_desc;

  if (!tdesc_has_registers (tdesc))
    /* Pick a default target description.  */
    tdesc = tdesc_nios2;

  /* Check any target description for validity.  */
  if (tdesc_has_registers (tdesc))
    {
      const struct tdesc_feature *feature;
      int valid_p;

      feature = tdesc_find_feature (tdesc, "org.gnu.gdb.nios2.cpu");
      if (feature == NULL)
	return NULL;

      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      
      for (i = 0; i < NIOS2_NUM_REGS; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data, i,
					    nios2_reg_names[i]);

      if (!valid_p)
	{
	  tdesc_data_cleanup (tdesc_data);
	  return NULL;
	}
    }

  /* Find a candidate among the list of pre-declared architectures.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found, create a new architecture from the information
     provided.  */
  tdep = xcalloc (1, sizeof (struct gdbarch_tdep));
  gdbarch = gdbarch_alloc (&info, tdep);

  /* longjmp support not enabled by default.  */
  tdep->jb_pc = -1;

  /* Data type sizes.  */
  set_gdbarch_ptr_bit (gdbarch, 32);
  set_gdbarch_addr_bit (gdbarch, 32);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);

  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);

  /* The register set.  */
  set_gdbarch_num_regs (gdbarch, NIOS2_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, NIOS2_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, NIOS2_PC_REGNUM);	/* Pseudo register PC */

  set_gdbarch_register_name (gdbarch, nios2_register_name);
  set_gdbarch_register_type (gdbarch, nios2_register_type);

  /* Provide register mappings for stabs and dwarf2.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, nios2_dwarf_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, nios2_dwarf_reg_to_regnum);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Call dummy code.  */
  set_gdbarch_frame_align (gdbarch, nios2_frame_align);

  set_gdbarch_return_value (gdbarch, nios2_return_value);

  set_gdbarch_skip_prologue (gdbarch, nios2_skip_prologue);
  set_gdbarch_in_function_epilogue_p (gdbarch, nios2_in_function_epilogue_p);
  set_gdbarch_breakpoint_from_pc (gdbarch, nios2_breakpoint_from_pc);

  set_gdbarch_dummy_id (gdbarch, nios2_dummy_id);
  set_gdbarch_unwind_pc (gdbarch, nios2_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, nios2_unwind_sp);

  /* The dwarf2 unwinder will normally produce the best results if
     the debug information is available, so register it first.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &nios2_stub_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &nios2_frame_unwind);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, nios2_software_single_step);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  if (tdep->jb_pc >= 0)
    set_gdbarch_get_longjmp_target (gdbarch, nios2_get_longjmp_target);

  frame_base_set_default (gdbarch, &nios2_frame_base);

  set_gdbarch_print_insn (gdbarch, nios2_print_insn);

  /* Enable inferior call support.  */
  set_gdbarch_push_dummy_call (gdbarch, nios2_push_dummy_call);

  if (tdesc_data)
    tdesc_use_registers (gdbarch, tdesc, tdesc_data);

  return gdbarch;
}

extern initialize_file_ftype _initialize_nios2_tdep; /* -Wmissing-prototypes */

void
_initialize_nios2_tdep (void)
{
  gdbarch_register (bfd_arch_nios2, nios2_gdbarch_init, NULL);
  initialize_tdesc_nios2 ();

  /* Allow debugging this file's internals.  */
  add_setshow_boolean_cmd ("nios2", class_maintenance, &nios2_debug,
			   _("Set Nios II debugging."),
			   _("Show Nios II debugging."),
			   _("When on, Nios II specific debugging is enabled."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);
}
