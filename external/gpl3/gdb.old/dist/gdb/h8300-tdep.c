/* Target-machine dependent code for Renesas H8/300, for GDB.

   Copyright (C) 1988-2023 Free Software Foundation, Inc.

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

/*
   Contributed by Steve Chamberlain
   sac@cygnus.com
 */

#include "defs.h"
#include "value.h"
#include "arch-utils.h"
#include "regcache.h"
#include "gdbcore.h"
#include "objfiles.h"
#include "dis-asm.h"
#include "dwarf2/frame.h"
#include "frame-base.h"
#include "frame-unwind.h"

enum gdb_regnum
{
  E_R0_REGNUM, E_ER0_REGNUM = E_R0_REGNUM, E_ARG0_REGNUM = E_R0_REGNUM,
  E_RET0_REGNUM = E_R0_REGNUM,
  E_R1_REGNUM, E_ER1_REGNUM = E_R1_REGNUM, E_RET1_REGNUM = E_R1_REGNUM,
  E_R2_REGNUM, E_ER2_REGNUM = E_R2_REGNUM, E_ARGLAST_REGNUM = E_R2_REGNUM,
  E_R3_REGNUM, E_ER3_REGNUM = E_R3_REGNUM,
  E_R4_REGNUM, E_ER4_REGNUM = E_R4_REGNUM,
  E_R5_REGNUM, E_ER5_REGNUM = E_R5_REGNUM,
  E_R6_REGNUM, E_ER6_REGNUM = E_R6_REGNUM, E_FP_REGNUM = E_R6_REGNUM,
  E_SP_REGNUM,
  E_CCR_REGNUM,
  E_PC_REGNUM,
  E_CYCLES_REGNUM,
  E_TICK_REGNUM, E_EXR_REGNUM = E_TICK_REGNUM,
  E_INST_REGNUM, E_TICKS_REGNUM = E_INST_REGNUM,
  E_INSTS_REGNUM,
  E_MACH_REGNUM,
  E_MACL_REGNUM,
  E_SBR_REGNUM,
  E_VBR_REGNUM
};

#define H8300_MAX_NUM_REGS 18

#define E_PSEUDO_CCR_REGNUM(gdbarch) (gdbarch_num_regs (gdbarch))
#define E_PSEUDO_EXR_REGNUM(gdbarch) (gdbarch_num_regs (gdbarch)+1)

struct h8300_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  CORE_ADDR sp_offset;
  CORE_ADDR pc;

  /* Flag showing that a frame has been created in the prologue code.  */
  int uses_fp;

  /* Saved registers.  */
  CORE_ADDR saved_regs[H8300_MAX_NUM_REGS];
  CORE_ADDR saved_sp;
};

enum
{
  h8300_reg_size = 2,
  h8300h_reg_size = 4,
  h8300_max_reg_size = 4,
};

static int is_h8300hmode (struct gdbarch *gdbarch);
static int is_h8300smode (struct gdbarch *gdbarch);
static int is_h8300sxmode (struct gdbarch *gdbarch);
static int is_h8300_normal_mode (struct gdbarch *gdbarch);

#define BINWORD(gdbarch) ((is_h8300hmode (gdbarch) \
		  && !is_h8300_normal_mode (gdbarch)) \
		 ? h8300h_reg_size : h8300_reg_size)

/* Normal frames.  */

/* Allocate and initialize a frame cache.  */

static void
h8300_init_frame_cache (struct gdbarch *gdbarch,
			struct h8300_frame_cache *cache)
{
  int i;

  /* Base address.  */
  cache->base = 0;
  cache->sp_offset = 0;
  cache->pc = 0;

  /* Frameless until proven otherwise.  */
  cache->uses_fp = 0;

  /* Saved registers.  We initialize these to -1 since zero is a valid
     offset (that's where %fp is supposed to be stored).  */
  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    cache->saved_regs[i] = -1;
}

#define IS_MOVB_RnRm(x)		(((x) & 0xff88) == 0x0c88)
#define IS_MOVW_RnRm(x)		(((x) & 0xff88) == 0x0d00)
#define IS_MOVL_RnRm(x)		(((x) & 0xff88) == 0x0f80)
#define IS_MOVB_Rn16_SP(x)	(((x) & 0xfff0) == 0x6ee0)
#define IS_MOVB_EXT(x)		((x) == 0x7860)
#define IS_MOVB_Rn24_SP(x)	(((x) & 0xfff0) == 0x6aa0)
#define IS_MOVW_Rn16_SP(x)	(((x) & 0xfff0) == 0x6fe0)
#define IS_MOVW_EXT(x)		((x) == 0x78e0)
#define IS_MOVW_Rn24_SP(x)	(((x) & 0xfff0) == 0x6ba0)
/* Same instructions as mov.w, just prefixed with 0x0100.  */
#define IS_MOVL_PRE(x)		((x) == 0x0100)
#define IS_MOVL_Rn16_SP(x)	(((x) & 0xfff0) == 0x6fe0)
#define IS_MOVL_EXT(x)		((x) == 0x78e0)
#define IS_MOVL_Rn24_SP(x)	(((x) & 0xfff0) == 0x6ba0)

#define IS_PUSHFP_MOVESPFP(x)	((x) == 0x6df60d76)
#define IS_PUSH_FP(x)		((x) == 0x01006df6)
#define IS_MOV_SP_FP(x)		((x) == 0x0ff6)
#define IS_SUB2_SP(x)		((x) == 0x1b87)
#define IS_SUB4_SP(x)		((x) == 0x1b97)
#define IS_ADD_IMM_SP(x)	((x) == 0x7a1f)
#define IS_SUB_IMM_SP(x)	((x) == 0x7a3f)
#define IS_SUBL4_SP(x)		((x) == 0x1acf)
#define IS_MOV_IMM_Rn(x)	(((x) & 0xfff0) == 0x7905)
#define IS_SUB_RnSP(x)		(((x) & 0xff0f) == 0x1907)
#define IS_ADD_RnSP(x)		(((x) & 0xff0f) == 0x0907)
#define IS_PUSH(x)		(((x) & 0xfff0) == 0x6df0)

/* If the instruction at PC is an argument register spill, return its
   length.  Otherwise, return zero.

   An argument register spill is an instruction that moves an argument
   from the register in which it was passed to the stack slot in which
   it really lives.  It is a byte, word, or longword move from an
   argument register to a negative offset from the frame pointer.
   
   CV, 2003-06-16: Or, in optimized code or when the `register' qualifier
   is used, it could be a byte, word or long move to registers r3-r5.  */

static int
h8300_is_argument_spill (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int w = read_memory_unsigned_integer (pc, 2, byte_order);

  if ((IS_MOVB_RnRm (w) || IS_MOVW_RnRm (w) || IS_MOVL_RnRm (w))
      && (w & 0x70) <= 0x20	/* Rs is R0, R1 or R2 */
      && (w & 0x7) >= 0x3 && (w & 0x7) <= 0x5)	/* Rd is R3, R4 or R5 */
    return 2;

  if (IS_MOVB_Rn16_SP (w)
      && 8 <= (w & 0xf) && (w & 0xf) <= 10)	/* Rs is R0L, R1L, or R2L  */
    {
      /* ... and d:16 is negative.  */
      if (read_memory_integer (pc + 2, 2, byte_order) < 0)
	return 4;
    }
  else if (IS_MOVB_EXT (w))
    {
      if (IS_MOVB_Rn24_SP (read_memory_unsigned_integer (pc + 2,
							 2, byte_order)))
	{
	  ULONGEST disp = read_memory_unsigned_integer (pc + 4, 4, byte_order);

	  /* ... and d:24 is negative.  */
	  if ((disp & 0x00800000) != 0)
	    return 8;
	}
    }
  else if (IS_MOVW_Rn16_SP (w)
	   && (w & 0xf) <= 2)	/* Rs is R0, R1, or R2 */
    {
      /* ... and d:16 is negative.  */
      if (read_memory_integer (pc + 2, 2, byte_order) < 0)
	return 4;
    }
  else if (IS_MOVW_EXT (w))
    {
      if (IS_MOVW_Rn24_SP (read_memory_unsigned_integer (pc + 2,
							 2, byte_order)))
	{
	  ULONGEST disp = read_memory_unsigned_integer (pc + 4, 4, byte_order);

	  /* ... and d:24 is negative.  */
	  if ((disp & 0x00800000) != 0)
	    return 8;
	}
    }
  else if (IS_MOVL_PRE (w))
    {
      int w2 = read_memory_integer (pc + 2, 2, byte_order);

      if (IS_MOVL_Rn16_SP (w2)
	  && (w2 & 0xf) <= 2)	/* Rs is ER0, ER1, or ER2 */
	{
	  /* ... and d:16 is negative.  */
	  if (read_memory_integer (pc + 4, 2, byte_order) < 0)
	    return 6;
	}
      else if (IS_MOVL_EXT (w2))
	{
	  if (IS_MOVL_Rn24_SP (read_memory_integer (pc + 4, 2, byte_order)))
	    {
	      ULONGEST disp = read_memory_unsigned_integer (pc + 6, 4,
							    byte_order);

	      /* ... and d:24 is negative.  */
	      if ((disp & 0x00800000) != 0)
		return 10;
	    }
	}
    }

  return 0;
}

/* Do a full analysis of the prologue at PC and update CACHE
   accordingly.  Bail out early if CURRENT_PC is reached.  Return the
   address where the analysis stopped.

   We handle all cases that can be generated by gcc.

   For allocating a stack frame:

   mov.w r6,@-sp
   mov.w sp,r6
   mov.w #-n,rN
   add.w rN,sp

   mov.w r6,@-sp
   mov.w sp,r6
   subs  #2,sp
   (repeat)

   mov.l er6,@-sp
   mov.l sp,er6
   add.l #-n,sp

   mov.w r6,@-sp
   mov.w sp,r6
   subs  #4,sp
   (repeat)

   For saving registers:

   mov.w rN,@-sp
   mov.l erN,@-sp
   stm.l reglist,@-sp

   */

static CORE_ADDR
h8300_analyze_prologue (struct gdbarch *gdbarch,
			CORE_ADDR pc, CORE_ADDR current_pc,
			struct h8300_frame_cache *cache)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int op;
  int regno, i, spill_size;

  cache->sp_offset = 0;

  if (pc >= current_pc)
    return current_pc;

  op = read_memory_unsigned_integer (pc, 4, byte_order);

  if (IS_PUSHFP_MOVESPFP (op))
    {
      cache->saved_regs[E_FP_REGNUM] = 0;
      cache->uses_fp = 1;
      pc += 4;
    }
  else if (IS_PUSH_FP (op))
    {
      cache->saved_regs[E_FP_REGNUM] = 0;
      pc += 4;
      if (pc >= current_pc)
	return current_pc;
      op = read_memory_unsigned_integer (pc, 2, byte_order);
      if (IS_MOV_SP_FP (op))
	{
	  cache->uses_fp = 1;
	  pc += 2;
	}
    }

  while (pc < current_pc)
    {
      op = read_memory_unsigned_integer (pc, 2, byte_order);
      if (IS_SUB2_SP (op))
	{
	  cache->sp_offset += 2;
	  pc += 2;
	}
      else if (IS_SUB4_SP (op))
	{
	  cache->sp_offset += 4;
	  pc += 2;
	}
      else if (IS_ADD_IMM_SP (op))
	{
	  cache->sp_offset += -read_memory_integer (pc + 2, 2, byte_order);
	  pc += 4;
	}
      else if (IS_SUB_IMM_SP (op))
	{
	  cache->sp_offset += read_memory_integer (pc + 2, 2, byte_order);
	  pc += 4;
	}
      else if (IS_SUBL4_SP (op))
	{
	  cache->sp_offset += 4;
	  pc += 2;
	}
      else if (IS_MOV_IMM_Rn (op))
	{
	  int offset = read_memory_integer (pc + 2, 2, byte_order);
	  regno = op & 0x000f;
	  op = read_memory_unsigned_integer (pc + 4, 2, byte_order);
	  if (IS_ADD_RnSP (op) && (op & 0x00f0) == regno)
	    {
	      cache->sp_offset -= offset;
	      pc += 6;
	    }
	  else if (IS_SUB_RnSP (op) && (op & 0x00f0) == regno)
	    {
	      cache->sp_offset += offset;
	      pc += 6;
	    }
	  else
	    break;
	}
      else if (IS_PUSH (op))
	{
	  regno = op & 0x000f;
	  cache->sp_offset += 2;
	  cache->saved_regs[regno] = cache->sp_offset;
	  pc += 2;
	}
      else if (op == 0x0100)
	{
	  op = read_memory_unsigned_integer (pc + 2, 2, byte_order);
	  if (IS_PUSH (op))
	    {
	      regno = op & 0x000f;
	      cache->sp_offset += 4;
	      cache->saved_regs[regno] = cache->sp_offset;
	      pc += 4;
	    }
	  else
	    break;
	}
      else if ((op & 0xffcf) == 0x0100)
	{
	  int op1;
	  op1 = read_memory_unsigned_integer (pc + 2, 2, byte_order);
	  if (IS_PUSH (op1))
	    {
	      /* Since the prefix is 0x01x0, this is not a simple pushm but a
		 stm.l reglist,@-sp */
	      i = ((op & 0x0030) >> 4) + 1;
	      regno = op1 & 0x000f;
	      for (; i > 0; regno++, --i)
		{
		  cache->sp_offset += 4;
		  cache->saved_regs[regno] = cache->sp_offset;
		}
	      pc += 4;
	    }
	  else
	    break;
	}
      else
	break;
    }

  /* Check for spilling an argument register to the stack frame.
     This could also be an initializing store from non-prologue code,
     but I don't think there's any harm in skipping that.  */
  while ((spill_size = h8300_is_argument_spill (gdbarch, pc)) > 0
	 && pc + spill_size <= current_pc)
    pc += spill_size;

  return pc;
}

static struct h8300_frame_cache *
h8300_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct h8300_frame_cache *cache;
  int i;
  CORE_ADDR current_pc;

  if (*this_cache)
    return (struct h8300_frame_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct h8300_frame_cache);
  h8300_init_frame_cache (gdbarch, cache);
  *this_cache = cache;

  /* In principle, for normal frames, %fp holds the frame pointer,
     which holds the base address for the current stack frame.
     However, for functions that don't need it, the frame pointer is
     optional.  For these "frameless" functions the frame pointer is
     actually the frame pointer of the calling frame.  */

  cache->base = get_frame_register_unsigned (this_frame, E_FP_REGNUM);
  if (cache->base == 0)
    return cache;

  cache->saved_regs[E_PC_REGNUM] = -BINWORD (gdbarch);

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  if (cache->pc != 0)
    h8300_analyze_prologue (gdbarch, cache->pc, current_pc, cache);

  if (!cache->uses_fp)
    {
      /* We didn't find a valid frame, which means that CACHE->base
	 currently holds the frame pointer for our calling frame.  If
	 we're at the start of a function, or somewhere half-way its
	 prologue, the function's frame probably hasn't been fully
	 setup yet.  Try to reconstruct the base address for the stack
	 frame by looking at the stack pointer.  For truly "frameless"
	 functions this might work too.  */

      cache->base = get_frame_register_unsigned (this_frame, E_SP_REGNUM)
		    + cache->sp_offset;
      cache->saved_sp = cache->base + BINWORD (gdbarch);
      cache->saved_regs[E_PC_REGNUM] = 0;
    }
  else
    {
      cache->saved_sp = cache->base + 2 * BINWORD (gdbarch);
      cache->saved_regs[E_PC_REGNUM] = -BINWORD (gdbarch);
    }

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    if (cache->saved_regs[i] != -1)
      cache->saved_regs[i] = cache->base - cache->saved_regs[i];

  return cache;
}

static void
h8300_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		     struct frame_id *this_id)
{
  struct h8300_frame_cache *cache =
    h8300_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  *this_id = frame_id_build (cache->saved_sp, cache->pc);
}

static struct value *
h8300_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			   int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct h8300_frame_cache *cache =
    h8300_frame_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0);

  if (regnum == E_SP_REGNUM && cache->saved_sp)
    return frame_unwind_got_constant (this_frame, regnum, cache->saved_sp);

  if (regnum < gdbarch_num_regs (gdbarch)
      && cache->saved_regs[regnum] != -1)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind h8300_frame_unwind = {
  "h8300 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  h8300_frame_this_id,
  h8300_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
h8300_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct h8300_frame_cache *cache = h8300_frame_cache (this_frame, this_cache);
  return cache->base;
}

static const struct frame_base h8300_frame_base = {
  &h8300_frame_unwind,
  h8300_frame_base_address,
  h8300_frame_base_address,
  h8300_frame_base_address
};

static CORE_ADDR
h8300_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0 , func_end = 0;

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      struct symtab_and_line sal;
      struct h8300_frame_cache cache;

      /* Found a function.  */
      sal = find_pc_line (func_addr, 0);
      if (sal.end && sal.end < func_end)
	/* Found a line number, use it as end of prologue.  */
	return sal.end;

      /* No useable line symbol.  Use prologue parsing method.  */
      h8300_init_frame_cache (gdbarch, &cache);
      return h8300_analyze_prologue (gdbarch, func_addr, func_end, &cache);
    }

  /* No function symbol -- just return the PC.  */
  return (CORE_ADDR) pc;
}

/* Function: push_dummy_call
   Setup the function arguments for calling a function in the inferior.
   In this discussion, a `word' is 16 bits on the H8/300s, and 32 bits
   on the H8/300H.

   There are actually two ABI's here: -mquickcall (the default) and
   -mno-quickcall.  With -mno-quickcall, all arguments are passed on
   the stack after the return address, word-aligned.  With
   -mquickcall, GCC tries to use r0 -- r2 to pass registers.  Since
   GCC doesn't indicate in the object file which ABI was used to
   compile it, GDB only supports the default --- -mquickcall.

   Here are the rules for -mquickcall, in detail:

   Each argument, whether scalar or aggregate, is padded to occupy a
   whole number of words.  Arguments smaller than a word are padded at
   the most significant end; those larger than a word are padded at
   the least significant end.

   The initial arguments are passed in r0 -- r2.  Earlier arguments go in
   lower-numbered registers.  Multi-word arguments are passed in
   consecutive registers, with the most significant end in the
   lower-numbered register.

   If an argument doesn't fit entirely in the remaining registers, it
   is passed entirely on the stack.  Stack arguments begin just after
   the return address.  Once an argument has overflowed onto the stack
   this way, all subsequent arguments are passed on the stack.

   The above rule has odd consequences.  For example, on the h8/300s,
   if a function takes two longs and an int as arguments:
   - the first long will be passed in r0/r1,
   - the second long will be passed entirely on the stack, since it
     doesn't fit in r2,
   - and the int will be passed on the stack, even though it could fit
     in r2.

   A weird exception: if an argument is larger than a word, but not a
   whole number of words in length (before padding), it is passed on
   the stack following the rules for stack arguments above, even if
   there are sufficient registers available to hold it.  Stranger
   still, the argument registers are still `used up' --- even though
   there's nothing in them.

   So, for example, on the h8/300s, if a function expects a three-byte
   structure and an int, the structure will go on the stack, and the
   int will go in r2, not r0.
  
   If the function returns an aggregate type (struct, union, or class)
   by value, the caller must allocate space to hold the return value,
   and pass the callee a pointer to this space as an invisible first
   argument, in R0.

   For varargs functions, the last fixed argument and all the variable
   arguments are always passed on the stack.  This means that calls to
   varargs functions don't work properly unless there is a prototype
   in scope.

   Basically, this ABI is not good, for the following reasons:
   - You can't call vararg functions properly unless a prototype is in scope.
   - Structure passing is inconsistent, to no purpose I can see.
   - It often wastes argument registers, of which there are only three
     to begin with.  */

static CORE_ADDR
h8300_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		       struct regcache *regcache, CORE_ADDR bp_addr,
		       int nargs, struct value **args, CORE_ADDR sp,
		       function_call_return_method return_method,
		       CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int stack_alloc = 0, stack_offset = 0;
  int wordsize = BINWORD (gdbarch);
  int reg = E_ARG0_REGNUM;
  int argument;

  /* First, make sure the stack is properly aligned.  */
  sp = align_down (sp, wordsize);

  /* Now make sure there's space on the stack for the arguments.  We
     may over-allocate a little here, but that won't hurt anything.  */
  for (argument = 0; argument < nargs; argument++)
    stack_alloc += align_up (value_type (args[argument])->length (), wordsize);
  sp -= stack_alloc;

  /* Now load as many arguments as possible into registers, and push
     the rest onto the stack.
     If we're returning a structure by value, then we must pass a
     pointer to the buffer for the return value as an invisible first
     argument.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, reg++, struct_addr);

  for (argument = 0; argument < nargs; argument++)
    {
      struct type *type = value_type (args[argument]);
      int len = type->length ();
      char *contents = (char *) value_contents (args[argument]).data ();

      /* Pad the argument appropriately.  */
      int padded_len = align_up (len, wordsize);
      /* Use std::vector here to get zero initialization.  */
      std::vector<gdb_byte> padded (padded_len);

      memcpy ((len < wordsize ? padded.data () + padded_len - len
	       : padded.data ()),
	      contents, len);

      /* Could the argument fit in the remaining registers?  */
      if (padded_len <= (E_ARGLAST_REGNUM - reg + 1) * wordsize)
	{
	  /* Are we going to pass it on the stack anyway, for no good
	     reason?  */
	  if (len > wordsize && len % wordsize)
	    {
	      /* I feel so unclean.  */
	      write_memory (sp + stack_offset, padded.data (), padded_len);
	      stack_offset += padded_len;

	      /* That's right --- even though we passed the argument
		 on the stack, we consume the registers anyway!  Love
		 me, love my dog.  */
	      reg += padded_len / wordsize;
	    }
	  else
	    {
	      /* Heavens to Betsy --- it's really going in registers!
		 Note that on the h8/300s, there are gaps between the
		 registers in the register file.  */
	      int offset;

	      for (offset = 0; offset < padded_len; offset += wordsize)
		{
		  ULONGEST word
		    = extract_unsigned_integer (&padded[offset],
						wordsize, byte_order);
		  regcache_cooked_write_unsigned (regcache, reg++, word);
		}
	    }
	}
      else
	{
	  /* It doesn't fit in registers!  Onto the stack it goes.  */
	  write_memory (sp + stack_offset, padded.data (), padded_len);
	  stack_offset += padded_len;

	  /* Once one argument has spilled onto the stack, all
	     subsequent arguments go on the stack.  */
	  reg = E_ARGLAST_REGNUM + 1;
	}
    }

  /* Store return address.  */
  sp -= wordsize;
  write_memory_unsigned_integer (sp, wordsize, byte_order, bp_addr);

  /* Update stack pointer.  */
  regcache_cooked_write_unsigned (regcache, E_SP_REGNUM, sp);

  /* Return the new stack pointer minus the return address slot since
     that's what DWARF2/GCC uses as the frame's CFA.  */
  return sp + wordsize;
}

/* Function: extract_return_value
   Figure out where in REGBUF the called function has left its return value.
   Copy that into VALBUF.  Be sure to account for CPU type.   */

static void
h8300_extract_return_value (struct type *type, struct regcache *regcache,
			    gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = type->length ();
  ULONGEST c, addr;

  switch (len)
    {
    case 1:
    case 2:
      regcache_cooked_read_unsigned (regcache, E_RET0_REGNUM, &c);
      store_unsigned_integer (valbuf, len, byte_order, c);
      break;
    case 4:			/* Needs two registers on plain H8/300 */
      regcache_cooked_read_unsigned (regcache, E_RET0_REGNUM, &c);
      store_unsigned_integer (valbuf, 2, byte_order, c);
      regcache_cooked_read_unsigned (regcache, E_RET1_REGNUM, &c);
      store_unsigned_integer (valbuf + 2, 2, byte_order, c);
      break;
    case 8:			/* long long is now 8 bytes.  */
      if (type->code () == TYPE_CODE_INT)
	{
	  regcache_cooked_read_unsigned (regcache, E_RET0_REGNUM, &addr);
	  c = read_memory_unsigned_integer ((CORE_ADDR) addr, len, byte_order);
	  store_unsigned_integer (valbuf, len, byte_order, c);
	}
      else
	{
	  error (_("I don't know how this 8 byte value is returned."));
	}
      break;
    }
}

static void
h8300h_extract_return_value (struct type *type, struct regcache *regcache,
			     gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST c;

  switch (type->length ())
    {
    case 1:
    case 2:
    case 4:
      regcache_cooked_read_unsigned (regcache, E_RET0_REGNUM, &c);
      store_unsigned_integer (valbuf, type->length (), byte_order, c);
      break;
    case 8:			/* long long is now 8 bytes.  */
      if (type->code () == TYPE_CODE_INT)
	{
	  regcache_cooked_read_unsigned (regcache, E_RET0_REGNUM, &c);
	  store_unsigned_integer (valbuf, 4, byte_order, c);
	  regcache_cooked_read_unsigned (regcache, E_RET1_REGNUM, &c);
	  store_unsigned_integer (valbuf + 4, 4, byte_order, c);
	}
      else
	{
	  error (_("I don't know how this 8 byte value is returned."));
	}
      break;
    }
}

static int
h8300_use_struct_convention (struct type *value_type)
{
  /* Types of 1, 2 or 4 bytes are returned in R0/R1, everything else on the
     stack.  */

  if (value_type->code () == TYPE_CODE_STRUCT
      || value_type->code () == TYPE_CODE_UNION)
    return 1;
  return !(value_type->length () == 1
	   || value_type->length () == 2
	   || value_type->length () == 4);
}

static int
h8300h_use_struct_convention (struct type *value_type)
{
  /* Types of 1, 2 or 4 bytes are returned in R0, INT types of 8 bytes are
     returned in R0/R1, everything else on the stack.  */
  if (value_type->code () == TYPE_CODE_STRUCT
      || value_type->code () == TYPE_CODE_UNION)
    return 1;
  return !(value_type->length () == 1
	   || value_type->length () == 2
	   || value_type->length () == 4
	   || (value_type->length () == 8
	       && value_type->code () == TYPE_CODE_INT));
}

/* Function: store_return_value
   Place the appropriate value in the appropriate registers.
   Primarily used by the RETURN command.  */

static void
h8300_store_return_value (struct type *type, struct regcache *regcache,
			  const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST val;

  switch (type->length ())
    {
    case 1:
    case 2:			/* short...  */
      val = extract_unsigned_integer (valbuf, type->length (), byte_order);
      regcache_cooked_write_unsigned (regcache, E_RET0_REGNUM, val);
      break;
    case 4:			/* long, float */
      val = extract_unsigned_integer (valbuf, type->length (), byte_order);
      regcache_cooked_write_unsigned (regcache, E_RET0_REGNUM,
				      (val >> 16) & 0xffff);
      regcache_cooked_write_unsigned (regcache, E_RET1_REGNUM, val & 0xffff);
      break;
    case 8:			/* long long, double and long double
				   are all defined as 4 byte types so
				   far so this shouldn't happen.  */
      error (_("I don't know how to return an 8 byte value."));
      break;
    }
}

static void
h8300h_store_return_value (struct type *type, struct regcache *regcache,
			   const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST val;

  switch (type->length ())
    {
    case 1:
    case 2:
    case 4:			/* long, float */
      val = extract_unsigned_integer (valbuf, type->length (), byte_order);
      regcache_cooked_write_unsigned (regcache, E_RET0_REGNUM, val);
      break;
    case 8:
      val = extract_unsigned_integer (valbuf, type->length (), byte_order);
      regcache_cooked_write_unsigned (regcache, E_RET0_REGNUM,
				      (val >> 32) & 0xffffffff);
      regcache_cooked_write_unsigned (regcache, E_RET1_REGNUM,
				      val & 0xffffffff);
      break;
    }
}

static enum return_value_convention
h8300_return_value (struct gdbarch *gdbarch, struct value *function,
		    struct type *type, struct regcache *regcache,
		    gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (h8300_use_struct_convention (type))
    return RETURN_VALUE_STRUCT_CONVENTION;
  if (writebuf)
    h8300_store_return_value (type, regcache, writebuf);
  else if (readbuf)
    h8300_extract_return_value (type, regcache, readbuf);
  return RETURN_VALUE_REGISTER_CONVENTION;
}

static enum return_value_convention
h8300h_return_value (struct gdbarch *gdbarch, struct value *function,
		     struct type *type, struct regcache *regcache,
		     gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (h8300h_use_struct_convention (type))
    {
      if (readbuf)
	{
	  ULONGEST addr;

	  regcache_raw_read_unsigned (regcache, E_R0_REGNUM, &addr);
	  read_memory (addr, readbuf, type->length ());
	}

      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }
  if (writebuf)
    h8300h_store_return_value (type, regcache, writebuf);
  else if (readbuf)
    h8300h_extract_return_value (type, regcache, readbuf);
  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Implementation of 'register_sim_regno' gdbarch method.  */

static int
h8300_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  /* Only makes sense to supply raw registers.  */
  gdb_assert (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch));

  /* We hide the raw ccr from the user by making it nameless.  Because
     the default register_sim_regno hook returns
     LEGACY_SIM_REGNO_IGNORE for unnamed registers, we need to
     override it.  The sim register numbering is compatible with
     gdb's.  */
  return regnum;
}

static const char *
h8300_register_name_common (const char *regnames[], int numregs,
			    struct gdbarch *gdbarch, int regno)
{
  gdb_assert (numregs == gdbarch_num_cooked_regs (gdbarch));
  return regnames[regno];
}

static const char *
h8300_register_name (struct gdbarch *gdbarch, int regno)
{
  /* The register names change depending on which h8300 processor
     type is selected.  */
  static const char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6",
    "sp", "", "pc", "cycles", "tick", "inst",
    "ccr",			/* pseudo register */
  };
  return h8300_register_name_common(register_names, ARRAY_SIZE(register_names),
				    gdbarch, regno);
}

static const char *
h8300h_register_name (struct gdbarch *gdbarch, int regno)
{
  static const char *register_names[] = {
    "er0", "er1", "er2", "er3", "er4", "er5", "er6",
    "sp", "", "pc", "cycles", "tick", "inst",
    "ccr",			/* pseudo register */
  };
  return h8300_register_name_common(register_names, ARRAY_SIZE(register_names),
				    gdbarch, regno);
}

static const char *
h8300s_register_name (struct gdbarch *gdbarch, int regno)
{
  static const char *register_names[] = {
    "er0", "er1", "er2", "er3", "er4", "er5", "er6",
    "sp", "", "pc", "cycles", "", "tick", "inst",
    "mach", "macl",
    "ccr", "exr"		/* pseudo registers */
  };
  return h8300_register_name_common(register_names, ARRAY_SIZE(register_names),
				    gdbarch, regno);
}

static const char *
h8300sx_register_name (struct gdbarch *gdbarch, int regno)
{
  static const char *register_names[] = {
    "er0", "er1", "er2", "er3", "er4", "er5", "er6",
    "sp", "", "pc", "cycles", "", "tick", "inst",
    "mach", "macl", "sbr", "vbr",
    "ccr", "exr"		/* pseudo registers */
  };
  return h8300_register_name_common(register_names, ARRAY_SIZE(register_names),
				    gdbarch, regno);
}

static void
h8300_print_register (struct gdbarch *gdbarch, struct ui_file *file,
		      frame_info_ptr frame, int regno)
{
  LONGEST rval;
  const char *name = gdbarch_register_name (gdbarch, regno);

  if (*name == '\0')
    return;

  rval = get_frame_register_signed (frame, regno);

  gdb_printf (file, "%-14s ", name);
  if ((regno == E_PSEUDO_CCR_REGNUM (gdbarch)) || \
      (regno == E_PSEUDO_EXR_REGNUM (gdbarch) && is_h8300smode (gdbarch)))
    {
      gdb_printf (file, "0x%02x        ", (unsigned char) rval);
      print_longest (file, 'u', 1, rval);
    }
  else
    {
      gdb_printf (file, "0x%s  ", phex ((ULONGEST) rval,
					BINWORD (gdbarch)));
      print_longest (file, 'd', 1, rval);
    }
  if (regno == E_PSEUDO_CCR_REGNUM (gdbarch))
    {
      /* CCR register */
      int C, Z, N, V;
      unsigned char l = rval & 0xff;
      gdb_printf (file, "\t");
      gdb_printf (file, "I-%d ", (l & 0x80) != 0);
      gdb_printf (file, "UI-%d ", (l & 0x40) != 0);
      gdb_printf (file, "H-%d ", (l & 0x20) != 0);
      gdb_printf (file, "U-%d ", (l & 0x10) != 0);
      N = (l & 0x8) != 0;
      Z = (l & 0x4) != 0;
      V = (l & 0x2) != 0;
      C = (l & 0x1) != 0;
      gdb_printf (file, "N-%d ", N);
      gdb_printf (file, "Z-%d ", Z);
      gdb_printf (file, "V-%d ", V);
      gdb_printf (file, "C-%d ", C);
      if ((C | Z) == 0)
	gdb_printf (file, "u> ");
      if ((C | Z) == 1)
	gdb_printf (file, "u<= ");
      if (C == 0)
	gdb_printf (file, "u>= ");
      if (C == 1)
	gdb_printf (file, "u< ");
      if (Z == 0)
	gdb_printf (file, "!= ");
      if (Z == 1)
	gdb_printf (file, "== ");
      if ((N ^ V) == 0)
	gdb_printf (file, ">= ");
      if ((N ^ V) == 1)
	gdb_printf (file, "< ");
      if ((Z | (N ^ V)) == 0)
	gdb_printf (file, "> ");
      if ((Z | (N ^ V)) == 1)
	gdb_printf (file, "<= ");
    }
  else if (regno == E_PSEUDO_EXR_REGNUM (gdbarch) && is_h8300smode (gdbarch))
    {
      /* EXR register */
      unsigned char l = rval & 0xff;
      gdb_printf (file, "\t");
      gdb_printf (file, "T-%d - - - ", (l & 0x80) != 0);
      gdb_printf (file, "I2-%d ", (l & 4) != 0);
      gdb_printf (file, "I1-%d ", (l & 2) != 0);
      gdb_printf (file, "I0-%d", (l & 1) != 0);
    }
  gdb_printf (file, "\n");
}

static void
h8300_print_registers_info (struct gdbarch *gdbarch, struct ui_file *file,
			    frame_info_ptr frame, int regno, int cpregs)
{
  if (regno < 0)
    {
      for (regno = E_R0_REGNUM; regno <= E_SP_REGNUM; ++regno)
	h8300_print_register (gdbarch, file, frame, regno);
      h8300_print_register (gdbarch, file, frame,
			    E_PSEUDO_CCR_REGNUM (gdbarch));
      h8300_print_register (gdbarch, file, frame, E_PC_REGNUM);
      if (is_h8300smode (gdbarch))
	{
	  h8300_print_register (gdbarch, file, frame,
				E_PSEUDO_EXR_REGNUM (gdbarch));
	  if (is_h8300sxmode (gdbarch))
	    {
	      h8300_print_register (gdbarch, file, frame, E_SBR_REGNUM);
	      h8300_print_register (gdbarch, file, frame, E_VBR_REGNUM);
	    }
	  h8300_print_register (gdbarch, file, frame, E_MACH_REGNUM);
	  h8300_print_register (gdbarch, file, frame, E_MACL_REGNUM);
	  h8300_print_register (gdbarch, file, frame, E_CYCLES_REGNUM);
	  h8300_print_register (gdbarch, file, frame, E_TICKS_REGNUM);
	  h8300_print_register (gdbarch, file, frame, E_INSTS_REGNUM);
	}
      else
	{
	  h8300_print_register (gdbarch, file, frame, E_CYCLES_REGNUM);
	  h8300_print_register (gdbarch, file, frame, E_TICK_REGNUM);
	  h8300_print_register (gdbarch, file, frame, E_INST_REGNUM);
	}
    }
  else
    {
      if (regno == E_CCR_REGNUM)
	h8300_print_register (gdbarch, file, frame,
			      E_PSEUDO_CCR_REGNUM (gdbarch));
      else if (regno == E_PSEUDO_EXR_REGNUM (gdbarch)
	       && is_h8300smode (gdbarch))
	h8300_print_register (gdbarch, file, frame,
			      E_PSEUDO_EXR_REGNUM (gdbarch));
      else
	h8300_print_register (gdbarch, file, frame, regno);
    }
}

static struct type *
h8300_register_type (struct gdbarch *gdbarch, int regno)
{
  if (regno < 0 || regno >= gdbarch_num_cooked_regs (gdbarch))
    internal_error (_("h8300_register_type: illegal register number %d"),
		    regno);
  else
    {
      switch (regno)
	{
	case E_PC_REGNUM:
	  return builtin_type (gdbarch)->builtin_func_ptr;
	case E_SP_REGNUM:
	case E_FP_REGNUM:
	  return builtin_type (gdbarch)->builtin_data_ptr;
	default:
	  if (regno == E_PSEUDO_CCR_REGNUM (gdbarch))
	    return builtin_type (gdbarch)->builtin_uint8;
	  else if (regno == E_PSEUDO_EXR_REGNUM (gdbarch))
	    return builtin_type (gdbarch)->builtin_uint8;
	  else if (is_h8300hmode (gdbarch))
	    return builtin_type (gdbarch)->builtin_int32;
	  else
	    return builtin_type (gdbarch)->builtin_int16;
	}
    }
}

/* Helpers for h8300_pseudo_register_read.  We expose ccr/exr as
   pseudo-registers to users with smaller sizes than the corresponding
   raw registers.  These helpers extend/narrow the values.  */

static enum register_status
pseudo_from_raw_register (struct gdbarch *gdbarch, readable_regcache *regcache,
			  gdb_byte *buf, int pseudo_regno, int raw_regno)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  enum register_status status;
  ULONGEST val;

  status = regcache->raw_read (raw_regno, &val);
  if (status == REG_VALID)
    store_unsigned_integer (buf,
			    register_size (gdbarch, pseudo_regno),
			    byte_order, val);
  return status;
}

/* See pseudo_from_raw_register.  */

static void
raw_from_pseudo_register (struct gdbarch *gdbarch, struct regcache *regcache,
			  const gdb_byte *buf, int raw_regno, int pseudo_regno)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST val;

  val = extract_unsigned_integer (buf, register_size (gdbarch, pseudo_regno),
				  byte_order);
  regcache_raw_write_unsigned (regcache, raw_regno, val);
}

static enum register_status
h8300_pseudo_register_read (struct gdbarch *gdbarch,
			    readable_regcache *regcache, int regno,
			    gdb_byte *buf)
{
  if (regno == E_PSEUDO_CCR_REGNUM (gdbarch))
    {
      return pseudo_from_raw_register (gdbarch, regcache, buf,
				       regno, E_CCR_REGNUM);
    }
  else if (regno == E_PSEUDO_EXR_REGNUM (gdbarch))
    {
      return pseudo_from_raw_register (gdbarch, regcache, buf,
				       regno, E_EXR_REGNUM);
    }
  else
    return regcache->raw_read (regno, buf);
}

static void
h8300_pseudo_register_write (struct gdbarch *gdbarch,
			     struct regcache *regcache, int regno,
			     const gdb_byte *buf)
{
  if (regno == E_PSEUDO_CCR_REGNUM (gdbarch))
    raw_from_pseudo_register (gdbarch, regcache, buf, E_CCR_REGNUM, regno);
  else if (regno == E_PSEUDO_EXR_REGNUM (gdbarch))
    raw_from_pseudo_register (gdbarch, regcache, buf, E_EXR_REGNUM, regno);
  else
    regcache->raw_write (regno, buf);
}

static int
h8300_dbg_reg_to_regnum (struct gdbarch *gdbarch, int regno)
{
  if (regno == E_CCR_REGNUM)
    return E_PSEUDO_CCR_REGNUM (gdbarch);
  return regno;
}

static int
h8300s_dbg_reg_to_regnum (struct gdbarch *gdbarch, int regno)
{
  if (regno == E_CCR_REGNUM)
    return E_PSEUDO_CCR_REGNUM (gdbarch);
  if (regno == E_EXR_REGNUM)
    return E_PSEUDO_EXR_REGNUM (gdbarch);
  return regno;
}

/*static unsigned char breakpoint[] = { 0x7A, 0xFF }; *//* ??? */
constexpr gdb_byte h8300_break_insn[] = { 0x01, 0x80 };	/* Sleep */

typedef BP_MANIPULATION (h8300_break_insn) h8300_breakpoint;

static struct gdbarch *
h8300_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;

  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  if (info.bfd_arch_info->arch != bfd_arch_h8300)
    return NULL;

  gdbarch = gdbarch_alloc (&info, 0);

  set_gdbarch_register_sim_regno (gdbarch, h8300_register_sim_regno);

  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_h8300:
      set_gdbarch_num_regs (gdbarch, 13);
      set_gdbarch_num_pseudo_regs (gdbarch, 1);
      set_gdbarch_dwarf2_reg_to_regnum (gdbarch, h8300_dbg_reg_to_regnum);
      set_gdbarch_stab_reg_to_regnum (gdbarch, h8300_dbg_reg_to_regnum);
      set_gdbarch_register_name (gdbarch, h8300_register_name);
      set_gdbarch_ptr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
      set_gdbarch_addr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
      set_gdbarch_return_value (gdbarch, h8300_return_value);
      break;
    case bfd_mach_h8300h:
    case bfd_mach_h8300hn:
      set_gdbarch_num_regs (gdbarch, 13);
      set_gdbarch_num_pseudo_regs (gdbarch, 1);
      set_gdbarch_dwarf2_reg_to_regnum (gdbarch, h8300_dbg_reg_to_regnum);
      set_gdbarch_stab_reg_to_regnum (gdbarch, h8300_dbg_reg_to_regnum);
      set_gdbarch_register_name (gdbarch, h8300h_register_name);
      if (info.bfd_arch_info->mach != bfd_mach_h8300hn)
	{
	  set_gdbarch_ptr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
	  set_gdbarch_addr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
	}
      else
	{
	  set_gdbarch_ptr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
	  set_gdbarch_addr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
	}
      set_gdbarch_return_value (gdbarch, h8300h_return_value);
      break;
    case bfd_mach_h8300s:
    case bfd_mach_h8300sn:
      set_gdbarch_num_regs (gdbarch, 16);
      set_gdbarch_num_pseudo_regs (gdbarch, 2);
      set_gdbarch_dwarf2_reg_to_regnum (gdbarch, h8300s_dbg_reg_to_regnum);
      set_gdbarch_stab_reg_to_regnum (gdbarch, h8300s_dbg_reg_to_regnum);
      set_gdbarch_register_name (gdbarch, h8300s_register_name);
      if (info.bfd_arch_info->mach != bfd_mach_h8300sn)
	{
	  set_gdbarch_ptr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
	  set_gdbarch_addr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
	}
      else
	{
	  set_gdbarch_ptr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
	  set_gdbarch_addr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
	}
      set_gdbarch_return_value (gdbarch, h8300h_return_value);
      break;
    case bfd_mach_h8300sx:
    case bfd_mach_h8300sxn:
      set_gdbarch_num_regs (gdbarch, 18);
      set_gdbarch_num_pseudo_regs (gdbarch, 2);
      set_gdbarch_dwarf2_reg_to_regnum (gdbarch, h8300s_dbg_reg_to_regnum);
      set_gdbarch_stab_reg_to_regnum (gdbarch, h8300s_dbg_reg_to_regnum);
      set_gdbarch_register_name (gdbarch, h8300sx_register_name);
      if (info.bfd_arch_info->mach != bfd_mach_h8300sxn)
	{
	  set_gdbarch_ptr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
	  set_gdbarch_addr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
	}
      else
	{
	  set_gdbarch_ptr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
	  set_gdbarch_addr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
	}
      set_gdbarch_return_value (gdbarch, h8300h_return_value);
      break;
    }

  set_gdbarch_pseudo_register_read (gdbarch, h8300_pseudo_register_read);
  set_gdbarch_pseudo_register_write (gdbarch, h8300_pseudo_register_write);

  /*
   * Basic register fields and methods.
   */

  set_gdbarch_sp_regnum (gdbarch, E_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, E_PC_REGNUM);
  set_gdbarch_register_type (gdbarch, h8300_register_type);
  set_gdbarch_print_registers_info (gdbarch, h8300_print_registers_info);

  /*
   * Frame Info
   */
  set_gdbarch_skip_prologue (gdbarch, h8300_skip_prologue);

  /* Frame unwinder.  */
  frame_base_set_default (gdbarch, &h8300_frame_base);

  /* 
   * Miscellany
   */
  /* Stack grows up.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       h8300_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       h8300_breakpoint::bp_from_kind);
  set_gdbarch_push_dummy_call (gdbarch, h8300_push_dummy_call);

  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_int_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);

  set_gdbarch_wchar_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_wchar_signed (gdbarch, 0);

  set_gdbarch_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_long_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_single);

  set_gdbarch_believe_pcc_promotion (gdbarch, 1);

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &h8300_frame_unwind);

  return gdbarch;

}

void _initialize_h8300_tdep ();
void
_initialize_h8300_tdep ()
{
  gdbarch_register (bfd_arch_h8300, h8300_gdbarch_init);
}

static int
is_h8300hmode (struct gdbarch *gdbarch)
{
  return gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sx
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sxn
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300s
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sn
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300h
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300hn;
}

static int
is_h8300smode (struct gdbarch *gdbarch)
{
  return gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sx
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sxn
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300s
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sn;
}

static int
is_h8300sxmode (struct gdbarch *gdbarch)
{
  return gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sx
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sxn;
}

static int
is_h8300_normal_mode (struct gdbarch *gdbarch)
{
  return gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sxn
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300sn
    || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_h8300hn;
}
