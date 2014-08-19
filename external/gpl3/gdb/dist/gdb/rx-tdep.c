/* Target-dependent code for the Renesas RX for GDB, the GNU debugger.

   Copyright (C) 2008-2014 Free Software Foundation, Inc.

   Contributed by Red Hat, Inc.

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
#include "arch-utils.h"
#include "prologue-value.h"
#include "target.h"
#include "regcache.h"
#include "opcode/rx.h"
#include "dis-asm.h"
#include "gdbtypes.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "value.h"
#include "gdbcore.h"
#include "dwarf2-frame.h"

#include "elf/rx.h"
#include "elf-bfd.h"

/* Certain important register numbers.  */
enum
{
  RX_SP_REGNUM = 0,
  RX_R1_REGNUM = 1,
  RX_R4_REGNUM = 4,
  RX_FP_REGNUM = 6,
  RX_R15_REGNUM = 15,
  RX_PC_REGNUM = 19,
  RX_ACC_REGNUM = 25,
  RX_NUM_REGS = 26
};

/* Architecture specific data.  */
struct gdbarch_tdep
{
  /* The ELF header flags specify the multilib used.  */
  int elf_flags;
};

/* This structure holds the results of a prologue analysis.  */
struct rx_prologue
{
  /* The offset from the frame base to the stack pointer --- always
     zero or negative.

     Calling this a "size" is a bit misleading, but given that the
     stack grows downwards, using offsets for everything keeps one
     from going completely sign-crazy: you never change anything's
     sign for an ADD instruction; always change the second operand's
     sign for a SUB instruction; and everything takes care of
     itself.  */
  int frame_size;

  /* Non-zero if this function has initialized the frame pointer from
     the stack pointer, zero otherwise.  */
  int has_frame_ptr;

  /* If has_frame_ptr is non-zero, this is the offset from the frame
     base to where the frame pointer points.  This is always zero or
     negative.  */
  int frame_ptr_offset;

  /* The address of the first instruction at which the frame has been
     set up and the arguments are where the debug info says they are
     --- as best as we can tell.  */
  CORE_ADDR prologue_end;

  /* reg_offset[R] is the offset from the CFA at which register R is
     saved, or 1 if register R has not been saved.  (Real values are
     always zero or negative.)  */
  int reg_offset[RX_NUM_REGS];
};

/* Implement the "register_name" gdbarch method.  */
static const char *
rx_register_name (struct gdbarch *gdbarch, int regnr)
{
  static const char *const reg_names[] = {
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",
    "usp",
    "isp",
    "psw",
    "pc",
    "intb",
    "bpsw",
    "bpc",
    "fintv",
    "fpsw",
    "acc"
  };

  return reg_names[regnr];
}

/* Implement the "register_type" gdbarch method.  */
static struct type *
rx_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr == RX_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else if (reg_nr == RX_ACC_REGNUM)
    return builtin_type (gdbarch)->builtin_unsigned_long_long;
  else
    return builtin_type (gdbarch)->builtin_unsigned_long;
}


/* Function for finding saved registers in a 'struct pv_area'; this
   function is passed to pv_area_scan.

   If VALUE is a saved register, ADDR says it was saved at a constant
   offset from the frame base, and SIZE indicates that the whole
   register was saved, record its offset.  */
static void
check_for_saved (void *result_untyped, pv_t addr, CORE_ADDR size, pv_t value)
{
  struct rx_prologue *result = (struct rx_prologue *) result_untyped;

  if (value.kind == pvk_register
      && value.k == 0
      && pv_is_register (addr, RX_SP_REGNUM)
      && size == register_size (target_gdbarch (), value.reg))
    result->reg_offset[value.reg] = addr.k;
}

/* Define a "handle" struct for fetching the next opcode.  */
struct rx_get_opcode_byte_handle
{
  CORE_ADDR pc;
};

/* Fetch a byte on behalf of the opcode decoder.  HANDLE contains
   the memory address of the next byte to fetch.  If successful,
   the address in the handle is updated and the byte fetched is
   returned as the value of the function.  If not successful, -1
   is returned.  */
static int
rx_get_opcode_byte (void *handle)
{
  struct rx_get_opcode_byte_handle *opcdata = handle;
  int status;
  gdb_byte byte;

  status = target_read_memory (opcdata->pc, &byte, 1);
  if (status == 0)
    {
      opcdata->pc += 1;
      return byte;
    }
  else
    return -1;
}

/* Analyze a prologue starting at START_PC, going no further than
   LIMIT_PC.  Fill in RESULT as appropriate.  */
static void
rx_analyze_prologue (CORE_ADDR start_pc,
		     CORE_ADDR limit_pc, struct rx_prologue *result)
{
  CORE_ADDR pc, next_pc;
  int rn;
  pv_t reg[RX_NUM_REGS];
  struct pv_area *stack;
  struct cleanup *back_to;
  CORE_ADDR after_last_frame_setup_insn = start_pc;

  memset (result, 0, sizeof (*result));

  for (rn = 0; rn < RX_NUM_REGS; rn++)
    {
      reg[rn] = pv_register (rn, 0);
      result->reg_offset[rn] = 1;
    }

  stack = make_pv_area (RX_SP_REGNUM, gdbarch_addr_bit (target_gdbarch ()));
  back_to = make_cleanup_free_pv_area (stack);

  /* The call instruction has saved the return address on the stack.  */
  reg[RX_SP_REGNUM] = pv_add_constant (reg[RX_SP_REGNUM], -4);
  pv_area_store (stack, reg[RX_SP_REGNUM], 4, reg[RX_PC_REGNUM]);

  pc = start_pc;
  while (pc < limit_pc)
    {
      int bytes_read;
      struct rx_get_opcode_byte_handle opcode_handle;
      RX_Opcode_Decoded opc;

      opcode_handle.pc = pc;
      bytes_read = rx_decode_opcode (pc, &opc, rx_get_opcode_byte,
				     &opcode_handle);
      next_pc = pc + bytes_read;

      if (opc.id == RXO_pushm	/* pushm r1, r2 */
	  && opc.op[1].type == RX_Operand_Register
	  && opc.op[2].type == RX_Operand_Register)
	{
	  int r1, r2;
	  int r;

	  r1 = opc.op[1].reg;
	  r2 = opc.op[2].reg;
	  for (r = r2; r >= r1; r--)
	    {
	      reg[RX_SP_REGNUM] = pv_add_constant (reg[RX_SP_REGNUM], -4);
	      pv_area_store (stack, reg[RX_SP_REGNUM], 4, reg[r]);
	    }
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RXO_mov	/* mov.l rdst, rsrc */
	       && opc.op[0].type == RX_Operand_Register
	       && opc.op[1].type == RX_Operand_Register
	       && opc.size == RX_Long)
	{
	  int rdst, rsrc;

	  rdst = opc.op[0].reg;
	  rsrc = opc.op[1].reg;
	  reg[rdst] = reg[rsrc];
	  if (rdst == RX_FP_REGNUM && rsrc == RX_SP_REGNUM)
	    after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RXO_mov	/* mov.l rsrc, [-SP] */
	       && opc.op[0].type == RX_Operand_Predec
	       && opc.op[0].reg == RX_SP_REGNUM
	       && opc.op[1].type == RX_Operand_Register
	       && opc.size == RX_Long)
	{
	  int rsrc;

	  rsrc = opc.op[1].reg;
	  reg[RX_SP_REGNUM] = pv_add_constant (reg[RX_SP_REGNUM], -4);
	  pv_area_store (stack, reg[RX_SP_REGNUM], 4, reg[rsrc]);
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RXO_add	/* add #const, rsrc, rdst */
	       && opc.op[0].type == RX_Operand_Register
	       && opc.op[1].type == RX_Operand_Immediate
	       && opc.op[2].type == RX_Operand_Register)
	{
	  int rdst = opc.op[0].reg;
	  int addend = opc.op[1].addend;
	  int rsrc = opc.op[2].reg;
	  reg[rdst] = pv_add_constant (reg[rsrc], addend);
	  /* Negative adjustments to the stack pointer or frame pointer
	     are (most likely) part of the prologue.  */
	  if ((rdst == RX_SP_REGNUM || rdst == RX_FP_REGNUM) && addend < 0)
	    after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RXO_mov
	       && opc.op[0].type == RX_Operand_Indirect
	       && opc.op[1].type == RX_Operand_Register
	       && opc.size == RX_Long
	       && (opc.op[0].reg == RX_SP_REGNUM
		   || opc.op[0].reg == RX_FP_REGNUM)
	       && (RX_R1_REGNUM <= opc.op[1].reg
		   && opc.op[1].reg <= RX_R4_REGNUM))
	{
	  /* This moves an argument register to the stack.  Don't
	     record it, but allow it to be a part of the prologue.  */
	}
      else if (opc.id == RXO_branch
	       && opc.op[0].type == RX_Operand_Immediate
	       && next_pc < opc.op[0].addend)
	{
	  /* When a loop appears as the first statement of a function
	     body, gcc 4.x will use a BRA instruction to branch to the
	     loop condition checking code.  This BRA instruction is
	     marked as part of the prologue.  We therefore set next_pc
	     to this branch target and also stop the prologue scan.
	     The instructions at and beyond the branch target should
	     no longer be associated with the prologue.

	     Note that we only consider forward branches here.  We
	     presume that a forward branch is being used to skip over
	     a loop body.

	     A backwards branch is covered by the default case below.
	     If we were to encounter a backwards branch, that would
	     most likely mean that we've scanned through a loop body.
	     We definitely want to stop the prologue scan when this
	     happens and that is precisely what is done by the default
	     case below.  */

	  after_last_frame_setup_insn = opc.op[0].addend;
	  break;		/* Scan no further if we hit this case.  */
	}
      else
	{
	  /* Terminate the prologue scan.  */
	  break;
	}

      pc = next_pc;
    }

  /* Is the frame size (offset, really) a known constant?  */
  if (pv_is_register (reg[RX_SP_REGNUM], RX_SP_REGNUM))
    result->frame_size = reg[RX_SP_REGNUM].k;

  /* Was the frame pointer initialized?  */
  if (pv_is_register (reg[RX_FP_REGNUM], RX_SP_REGNUM))
    {
      result->has_frame_ptr = 1;
      result->frame_ptr_offset = reg[RX_FP_REGNUM].k;
    }

  /* Record where all the registers were saved.  */
  pv_area_scan (stack, check_for_saved, (void *) result);

  result->prologue_end = after_last_frame_setup_insn;

  do_cleanups (back_to);
}


/* Implement the "skip_prologue" gdbarch method.  */
static CORE_ADDR
rx_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  const char *name;
  CORE_ADDR func_addr, func_end;
  struct rx_prologue p;

  /* Try to find the extent of the function that contains PC.  */
  if (!find_pc_partial_function (pc, &name, &func_addr, &func_end))
    return pc;

  rx_analyze_prologue (pc, func_end, &p);
  return p.prologue_end;
}

/* Given a frame described by THIS_FRAME, decode the prologue of its
   associated function if there is not cache entry as specified by
   THIS_PROLOGUE_CACHE.  Save the decoded prologue in the cache and
   return that struct as the value of this function.  */
static struct rx_prologue *
rx_analyze_frame_prologue (struct frame_info *this_frame,
			   void **this_prologue_cache)
{
  if (!*this_prologue_cache)
    {
      CORE_ADDR func_start, stop_addr;

      *this_prologue_cache = FRAME_OBSTACK_ZALLOC (struct rx_prologue);

      func_start = get_frame_func (this_frame);
      stop_addr = get_frame_pc (this_frame);

      /* If we couldn't find any function containing the PC, then
         just initialize the prologue cache, but don't do anything.  */
      if (!func_start)
	stop_addr = func_start;

      rx_analyze_prologue (func_start, stop_addr, *this_prologue_cache);
    }

  return *this_prologue_cache;
}

/* Given the next frame and a prologue cache, return this frame's
   base.  */
static CORE_ADDR
rx_frame_base (struct frame_info *this_frame, void **this_prologue_cache)
{
  struct rx_prologue *p
    = rx_analyze_frame_prologue (this_frame, this_prologue_cache);

  /* In functions that use alloca, the distance between the stack
     pointer and the frame base varies dynamically, so we can't use
     the SP plus static information like prologue analysis to find the
     frame base.  However, such functions must have a frame pointer,
     to be able to restore the SP on exit.  So whenever we do have a
     frame pointer, use that to find the base.  */
  if (p->has_frame_ptr)
    {
      CORE_ADDR fp = get_frame_register_unsigned (this_frame, RX_FP_REGNUM);
      return fp - p->frame_ptr_offset;
    }
  else
    {
      CORE_ADDR sp = get_frame_register_unsigned (this_frame, RX_SP_REGNUM);
      return sp - p->frame_size;
    }
}

/* Implement the "frame_this_id" method for unwinding frames.  */
static void
rx_frame_this_id (struct frame_info *this_frame,
		  void **this_prologue_cache, struct frame_id *this_id)
{
  *this_id = frame_id_build (rx_frame_base (this_frame, this_prologue_cache),
			     get_frame_func (this_frame));
}

/* Implement the "frame_prev_register" method for unwinding frames.  */
static struct value *
rx_frame_prev_register (struct frame_info *this_frame,
			void **this_prologue_cache, int regnum)
{
  struct rx_prologue *p
    = rx_analyze_frame_prologue (this_frame, this_prologue_cache);
  CORE_ADDR frame_base = rx_frame_base (this_frame, this_prologue_cache);
  int reg_size = register_size (get_frame_arch (this_frame), regnum);

  if (regnum == RX_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, regnum, frame_base);

  /* If prologue analysis says we saved this register somewhere,
     return a description of the stack slot holding it.  */
  else if (p->reg_offset[regnum] != 1)
    return frame_unwind_got_memory (this_frame, regnum,
				    frame_base + p->reg_offset[regnum]);

  /* Otherwise, presume we haven't changed the value of this
     register, and get it from the next frame.  */
  else
    return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind rx_frame_unwind = {
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  rx_frame_this_id,
  rx_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Implement the "unwind_pc" gdbarch method.  */
static CORE_ADDR
rx_unwind_pc (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  ULONGEST pc;

  pc = frame_unwind_register_unsigned (this_frame, RX_PC_REGNUM);
  return pc;
}

/* Implement the "unwind_sp" gdbarch method.  */
static CORE_ADDR
rx_unwind_sp (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  ULONGEST sp;

  sp = frame_unwind_register_unsigned (this_frame, RX_SP_REGNUM);
  return sp;
}

/* Implement the "dummy_id" gdbarch method.  */
static struct frame_id
rx_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  return
    frame_id_build (get_frame_register_unsigned (this_frame, RX_SP_REGNUM),
		    get_frame_pc (this_frame));
}

/* Implement the "push_dummy_call" gdbarch method.  */
static CORE_ADDR
rx_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		    struct regcache *regcache, CORE_ADDR bp_addr, int nargs,
		    struct value **args, CORE_ADDR sp, int struct_return,
		    CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int write_pass;
  int sp_off = 0;
  CORE_ADDR cfa;
  int num_register_candidate_args;

  struct type *func_type = value_type (function);

  /* Dereference function pointer types.  */
  while (TYPE_CODE (func_type) == TYPE_CODE_PTR)
    func_type = TYPE_TARGET_TYPE (func_type);

  /* The end result had better be a function or a method.  */
  gdb_assert (TYPE_CODE (func_type) == TYPE_CODE_FUNC
	      || TYPE_CODE (func_type) == TYPE_CODE_METHOD);

  /* Functions with a variable number of arguments have all of their
     variable arguments and the last non-variable argument passed
     on the stack.

     Otherwise, we can pass up to four arguments on the stack.

     Once computed, we leave this value alone.  I.e. we don't update
     it in case of a struct return going in a register or an argument
     requiring multiple registers, etc.  We rely instead on the value
     of the ``arg_reg'' variable to get these other details correct.  */

  if (TYPE_VARARGS (func_type))
    num_register_candidate_args = TYPE_NFIELDS (func_type) - 1;
  else
    num_register_candidate_args = 4;

  /* We make two passes; the first does the stack allocation,
     the second actually stores the arguments.  */
  for (write_pass = 0; write_pass <= 1; write_pass++)
    {
      int i;
      int arg_reg = RX_R1_REGNUM;

      if (write_pass)
	sp = align_down (sp - sp_off, 4);
      sp_off = 0;

      if (struct_return)
	{
	  struct type *return_type = TYPE_TARGET_TYPE (func_type);

	  gdb_assert (TYPE_CODE (return_type) == TYPE_CODE_STRUCT
		      || TYPE_CODE (func_type) == TYPE_CODE_UNION);

	  if (TYPE_LENGTH (return_type) > 16
	      || TYPE_LENGTH (return_type) % 4 != 0)
	    {
	      if (write_pass)
		regcache_cooked_write_unsigned (regcache, RX_R15_REGNUM,
						struct_addr);
	    }
	}

      /* Push the arguments.  */
      for (i = 0; i < nargs; i++)
	{
	  struct value *arg = args[i];
	  const gdb_byte *arg_bits = value_contents_all (arg);
	  struct type *arg_type = check_typedef (value_type (arg));
	  ULONGEST arg_size = TYPE_LENGTH (arg_type);

	  if (i == 0 && struct_addr != 0 && !struct_return
	      && TYPE_CODE (arg_type) == TYPE_CODE_PTR
	      && extract_unsigned_integer (arg_bits, 4,
					   byte_order) == struct_addr)
	    {
	      /* This argument represents the address at which C++ (and
	         possibly other languages) store their return value.
	         Put this value in R15.  */
	      if (write_pass)
		regcache_cooked_write_unsigned (regcache, RX_R15_REGNUM,
						struct_addr);
	    }
	  else if (TYPE_CODE (arg_type) != TYPE_CODE_STRUCT
		   && TYPE_CODE (arg_type) != TYPE_CODE_UNION)
	    {
	      /* Argument is a scalar.  */
	      if (arg_size == 8)
		{
		  if (i < num_register_candidate_args
		      && arg_reg <= RX_R4_REGNUM - 1)
		    {
		      /* If argument registers are going to be used to pass
		         an 8 byte scalar, the ABI specifies that two registers
		         must be available.  */
		      if (write_pass)
			{
			  regcache_cooked_write_unsigned (regcache, arg_reg,
							  extract_unsigned_integer
							  (arg_bits, 4,
							   byte_order));
			  regcache_cooked_write_unsigned (regcache,
							  arg_reg + 1,
							  extract_unsigned_integer
							  (arg_bits + 4, 4,
							   byte_order));
			}
		      arg_reg += 2;
		    }
		  else
		    {
		      sp_off = align_up (sp_off, 4);
		      /* Otherwise, pass the 8 byte scalar on the stack.  */
		      if (write_pass)
			write_memory (sp + sp_off, arg_bits, 8);
		      sp_off += 8;
		    }
		}
	      else
		{
		  ULONGEST u;

		  gdb_assert (arg_size <= 4);

		  u =
		    extract_unsigned_integer (arg_bits, arg_size, byte_order);

		  if (i < num_register_candidate_args
		      && arg_reg <= RX_R4_REGNUM)
		    {
		      if (write_pass)
			regcache_cooked_write_unsigned (regcache, arg_reg, u);
		      arg_reg += 1;
		    }
		  else
		    {
		      int p_arg_size = 4;

		      if (TYPE_PROTOTYPED (func_type)
			  && i < TYPE_NFIELDS (func_type))
			{
			  struct type *p_arg_type =
			    TYPE_FIELD_TYPE (func_type, i);
			  p_arg_size = TYPE_LENGTH (p_arg_type);
			}

		      sp_off = align_up (sp_off, p_arg_size);

		      if (write_pass)
			write_memory_unsigned_integer (sp + sp_off,
						       p_arg_size, byte_order,
						       u);
		      sp_off += p_arg_size;
		    }
		}
	    }
	  else
	    {
	      /* Argument is a struct or union.  Pass as much of the struct
	         in registers, if possible.  Pass the rest on the stack.  */
	      while (arg_size > 0)
		{
		  if (i < num_register_candidate_args
		      && arg_reg <= RX_R4_REGNUM
		      && arg_size <= 4 * (RX_R4_REGNUM - arg_reg + 1)
		      && arg_size % 4 == 0)
		    {
		      int len = min (arg_size, 4);

		      if (write_pass)
			regcache_cooked_write_unsigned (regcache, arg_reg,
							extract_unsigned_integer
							(arg_bits, len,
							 byte_order));
		      arg_bits += len;
		      arg_size -= len;
		      arg_reg++;
		    }
		  else
		    {
		      sp_off = align_up (sp_off, 4);
		      if (write_pass)
			write_memory (sp + sp_off, arg_bits, arg_size);
		      sp_off += align_up (arg_size, 4);
		      arg_size = 0;
		    }
		}
	    }
	}
    }

  /* Keep track of the stack address prior to pushing the return address.
     This is the value that we'll return.  */
  cfa = sp;

  /* Push the return address.  */
  sp = sp - 4;
  write_memory_unsigned_integer (sp, 4, byte_order, bp_addr);

  /* Update the stack pointer.  */
  regcache_cooked_write_unsigned (regcache, RX_SP_REGNUM, sp);

  return cfa;
}

/* Implement the "return_value" gdbarch method.  */
static enum return_value_convention
rx_return_value (struct gdbarch *gdbarch,
		 struct value *function,
		 struct type *valtype,
		 struct regcache *regcache,
		 gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST valtype_len = TYPE_LENGTH (valtype);

  if (TYPE_LENGTH (valtype) > 16
      || ((TYPE_CODE (valtype) == TYPE_CODE_STRUCT
	   || TYPE_CODE (valtype) == TYPE_CODE_UNION)
	  && TYPE_LENGTH (valtype) % 4 != 0))
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    {
      ULONGEST u;
      int argreg = RX_R1_REGNUM;
      int offset = 0;

      while (valtype_len > 0)
	{
	  int len = min (valtype_len, 4);

	  regcache_cooked_read_unsigned (regcache, argreg, &u);
	  store_unsigned_integer (readbuf + offset, len, byte_order, u);
	  valtype_len -= len;
	  offset += len;
	  argreg++;
	}
    }

  if (writebuf)
    {
      ULONGEST u;
      int argreg = RX_R1_REGNUM;
      int offset = 0;

      while (valtype_len > 0)
	{
	  int len = min (valtype_len, 4);

	  u = extract_unsigned_integer (writebuf + offset, len, byte_order);
	  regcache_cooked_write_unsigned (regcache, argreg, u);
	  valtype_len -= len;
	  offset += len;
	  argreg++;
	}
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Implement the "breakpoint_from_pc" gdbarch method.  */
static const gdb_byte *
rx_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr, int *lenptr)
{
  static gdb_byte breakpoint[] = { 0x00 };
  *lenptr = sizeof breakpoint;
  return breakpoint;
}

/* Allocate and initialize a gdbarch object.  */
static struct gdbarch *
rx_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  int elf_flags;

  /* Extract the elf_flags if available.  */
  if (info.abfd != NULL
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_flags = elf_elfheader (info.abfd)->e_flags;
  else
    elf_flags = 0;


  /* Try to find the architecture in the list of already defined
     architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      if (gdbarch_tdep (arches->gdbarch)->elf_flags != elf_flags)
	continue;

      return arches->gdbarch;
    }

  /* None found, create a new architecture from the information
     provided.  */
  tdep = (struct gdbarch_tdep *) xmalloc (sizeof (struct gdbarch_tdep));
  gdbarch = gdbarch_alloc (&info, tdep);
  tdep->elf_flags = elf_flags;

  set_gdbarch_num_regs (gdbarch, RX_NUM_REGS);
  set_gdbarch_num_pseudo_regs (gdbarch, 0);
  set_gdbarch_register_name (gdbarch, rx_register_name);
  set_gdbarch_register_type (gdbarch, rx_register_type);
  set_gdbarch_pc_regnum (gdbarch, RX_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, RX_SP_REGNUM);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_decr_pc_after_break (gdbarch, 1);
  set_gdbarch_breakpoint_from_pc (gdbarch, rx_breakpoint_from_pc);
  set_gdbarch_skip_prologue (gdbarch, rx_skip_prologue);

  set_gdbarch_print_insn (gdbarch, print_insn_rx);

  set_gdbarch_unwind_pc (gdbarch, rx_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, rx_unwind_sp);

  /* Target builtin data types.  */
  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 32);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  if (elf_flags & E_FLAG_RX_64BIT_DOUBLES)
    {
      set_gdbarch_double_bit (gdbarch, 64);
      set_gdbarch_long_double_bit (gdbarch, 64);
      set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
      set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);
    }
  else
    {
      set_gdbarch_double_bit (gdbarch, 32);
      set_gdbarch_long_double_bit (gdbarch, 32);
      set_gdbarch_double_format (gdbarch, floatformats_ieee_single);
      set_gdbarch_long_double_format (gdbarch, floatformats_ieee_single);
    }

  /* Frame unwinding.  */
#if 0
  /* Note: The test results are better with the dwarf2 unwinder disabled,
     so it's turned off for now.  */
  dwarf2_append_unwinders (gdbarch);
#endif
  frame_unwind_append_unwinder (gdbarch, &rx_frame_unwind);

  /* Methods for saving / extracting a dummy frame's ID.
     The ID's stack address must match the SP value returned by
     PUSH_DUMMY_CALL, and saved by generic_save_dummy_frame_tos.  */
  set_gdbarch_dummy_id (gdbarch, rx_dummy_id);
  set_gdbarch_push_dummy_call (gdbarch, rx_push_dummy_call);
  set_gdbarch_return_value (gdbarch, rx_return_value);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 1);

  return gdbarch;
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_rx_tdep;

/* Register the above initialization routine.  */

void
_initialize_rx_tdep (void)
{
  register_gdbarch_init (bfd_arch_rx, rx_gdbarch_init);
}
