/* Target-dependent code for the Matsushita MN10300 for GDB, the GNU debugger.
   Copyright 1996, 1997 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "obstack.h"
#include "target.h"
#include "value.h"
#include "bfd.h"
#include "gdb_string.h"
#include "gdbcore.h"
#include "symfile.h"


/* The main purpose of this file is dealing with prologues to extract
   information about stack frames and saved registers.

   For reference here's how prologues look on the mn10300:

     With frame pointer:
	movm [d2,d3,a2,a3],sp
	mov sp,a3
	add <size>,sp

     Without frame pointer:
	movm [d2,d3,a2,a3],sp (if needed)
        add <size>,sp

   One day we might keep the stack pointer constant, that won't
   change the code for prologues, but it will make the frame
   pointerless case much more common.  */
	
/* Analyze the prologue to determine where registers are saved,
   the end of the prologue, etc etc.  Return the end of the prologue
   scanned.

   We store into FI (if non-null) several tidbits of information:

    * stack_size -- size of this stack frame.  Note that if we stop in
    certain parts of the prologue/epilogue we may claim the size of the
    current frame is zero.  This happens when the current frame has
    not been allocated yet or has already been deallocated.

    * fsr -- Addresses of registers saved in the stack by this frame.

    * status -- A (relatively) generic status indicator.  It's a bitmask
    with the following bits: 

      MY_FRAME_IN_SP: The base of the current frame is actually in
      the stack pointer.  This can happen for frame pointerless
      functions, or cases where we're stopped in the prologue/epilogue
      itself.  For these cases mn10300_analyze_prologue will need up
      update fi->frame before returning or analyzing the register
      save instructions.

      MY_FRAME_IN_FP: The base of the current frame is in the
      frame pointer register ($a2).

      NO_MORE_FRAMES: Set this if the current frame is "start" or
      if the first instruction looks like mov <imm>,sp.  This tells
      frame chain to not bother trying to unwind past this frame.  */

#define MY_FRAME_IN_SP 0x1
#define MY_FRAME_IN_FP 0x2
#define NO_MORE_FRAMES 0x4
 
static CORE_ADDR
mn10300_analyze_prologue (fi, pc)
    struct frame_info *fi;
    CORE_ADDR pc;
{
  CORE_ADDR func_addr, func_end, addr, stop;
  CORE_ADDR stack_size;
  unsigned char buf[4];
  int status, found_movm = 0;
  char *name;

  /* Use the PC in the frame if it's provided to look up the
     start of this function.  */
  pc = (fi ? fi->pc : pc);

  /* Find the start of this function.  */
  status = find_pc_partial_function (pc, &name, &func_addr, &func_end);

  /* Do nothing if we couldn't find the start of this function or if we're
     stopped at the first instruction in the prologue.  */
  if (status == 0)
    return pc;

  /* If we're in start, then give up.  */
  if (strcmp (name, "start") == 0)
    {
      fi->status = NO_MORE_FRAMES;
      return pc;
    }

  /* At the start of a function our frame is in the stack pointer.  */
  if (fi)
    fi->status = MY_FRAME_IN_SP;

  /* Get the next two bytes into buf, we need two because rets is a two
     byte insn and the first isn't enough to uniquely identify it.  */
  status = target_read_memory (pc, buf, 2);
  if (status != 0)
    return pc;

  /* If we're physically on an "rets" instruction, then our frame has
     already been deallocated.  Note this can also be true for retf
     and ret if they specify a size of zero.

     In this case fi->frame is bogus, we need to fix it.  */
  if (fi && buf[0] == 0xf0 && buf[1] == 0xfc)
    {
      if (fi->next == NULL)
	fi->frame = read_sp ();
      return fi->pc;
    }

  /* Similarly if we're stopped on the first insn of a prologue as our
     frame hasn't been allocated yet.  */
  if (fi && fi->pc == func_addr)
    {
      if (fi->next == NULL)
	fi->frame = read_sp ();
      return fi->pc;
    }

  /* Figure out where to stop scanning.  */
  stop = fi ? fi->pc : func_end;

  /* Don't walk off the end of the function.  */
  stop = stop > func_end ? func_end : stop;

  /* Start scanning on the first instruction of this function.  */
  addr = func_addr;

  /* Suck in two bytes.  */
  status = target_read_memory (addr, buf, 2);
  if (status != 0)
    {
      if (fi && fi->next == NULL && fi->status & MY_FRAME_IN_SP)
	fi->frame = read_sp ();
      return addr;
    }

  /* First see if this insn sets the stack pointer; if so, it's something
     we won't understand, so quit now.   */
  if (buf[0] == 0xf2 && (buf[1] & 0xf3) == 0xf0)
    {
      if (fi)
	fi->status = NO_MORE_FRAMES;
      return addr;
    }

  /* Now look for movm [regs],sp, which saves the callee saved registers.

     At this time we don't know if fi->frame is valid, so we only note
     that we encountered a movm instruction.  Later, we'll set the entries
     in fsr.regs as needed.  */
  if (buf[0] == 0xcf)
    {
      found_movm = 1;
      addr += 2;

      /* Quit now if we're beyond the stop point.  */
      if (addr >= stop)
	{
	  /* Fix fi->frame since it's bogus at this point.  */
	  if (fi && fi->next == NULL)
	    fi->frame = read_sp ();

	  /* Note if/where callee saved registers were saved.  */
	  if (fi && found_movm)
	    {
	      fi->fsr.regs[7] = fi->frame;
	      fi->fsr.regs[6] = fi->frame + 4;
	      fi->fsr.regs[3] = fi->frame + 8;
	      fi->fsr.regs[2] = fi->frame + 12;
	    }
	  return addr;
	}

      /* Get the next two bytes so the prologue scan can continue.  */
      status = target_read_memory (addr, buf, 2);
      if (status != 0)
	{
	  /* Fix fi->frame since it's bogus at this point.  */
	  if (fi && fi->next == NULL)
	    fi->frame = read_sp ();

	  /* Note if/where callee saved registers were saved.  */
	  if (fi && found_movm)
	    {
	      fi->fsr.regs[7] = fi->frame;
	      fi->fsr.regs[6] = fi->frame + 4;
	      fi->fsr.regs[3] = fi->frame + 8;
	      fi->fsr.regs[2] = fi->frame + 12;
	    }
	  return addr;
	}
    }

  /* Now see if we set up a frame pointer via "mov sp,a3" */
  if (buf[0] == 0x3f)
    {
      addr += 1;

      /* The frame pointer is now valid.  */
      if (fi)
	{
	  fi->status |= MY_FRAME_IN_FP;
	  fi->status &= ~MY_FRAME_IN_SP;
	}

      /* Quit now if we're beyond the stop point.  */
      if (addr >= stop)
	{
	  /* Note if/where callee saved registers were saved.  */
	  if (fi && found_movm)
	    {
	      fi->fsr.regs[7] = fi->frame;
	      fi->fsr.regs[6] = fi->frame + 4;
	      fi->fsr.regs[3] = fi->frame + 8;
	      fi->fsr.regs[2] = fi->frame + 12;
	    }
	  return addr;
	}

      /* Get two more bytes so scanning can continue.  */
      status = target_read_memory (addr, buf, 2);
      if (status != 0)
	{
	  /* Note if/where callee saved registers were saved.  */
	  if (fi && found_movm)
	    {
	      fi->fsr.regs[7] = fi->frame;
	      fi->fsr.regs[6] = fi->frame + 4;
	      fi->fsr.regs[3] = fi->frame + 8;
	      fi->fsr.regs[2] = fi->frame + 12;
	    }
	  return addr;
	}
    }
  
  /* Next we should allocate the local frame.  No more prologue insns
     are found after allocating the local frame.
       
     Search for add imm8,sp (0xf8feXX)
        or	add imm16,sp (0xfafeXXXX)
        or	add imm32,sp (0xfcfeXXXXXXXX).
       
     If none of the above was found, then this prologue has no 
     additional stack.  */
  status = target_read_memory (addr, buf, 2);
  if (status != 0)
    {
      /* Fix fi->frame if it's bogus at this point.  */
      if (fi && fi->next == NULL && (fi->status & MY_FRAME_IN_SP))
	fi->frame = read_sp ();

      /* Note if/where callee saved registers were saved.  */
      if (fi && found_movm)
	{
	  fi->fsr.regs[7] = fi->frame;
	  fi->fsr.regs[6] = fi->frame + 4;
	  fi->fsr.regs[3] = fi->frame + 8;
	  fi->fsr.regs[2] = fi->frame + 12;
	}
      return addr;
    }

  if (buf[0] == 0xf8 && buf[1] == 0xfe)
    {
      /* Suck in one more byte, it'll hold the size of the current frame.  */
      status = target_read_memory (addr + 2, buf, 1);
      if (status != 0)
	{
	  /* Fix fi->frame if it's bogus at this point.  */
	  if (fi && fi->next == NULL && (fi->status & MY_FRAME_IN_SP))
	    fi->frame = read_sp ();

	  /* Note if/where callee saved registers were saved.  */
	  if (fi && found_movm)
	    {
	      fi->fsr.regs[7] = fi->frame;
	      fi->fsr.regs[6] = fi->frame + 4;
	      fi->fsr.regs[3] = fi->frame + 8;
	      fi->fsr.regs[2] = fi->frame + 12;
	    }
	  return addr;
	}

      /* Note the size of the stack in the frame info structure.  */
      stack_size = extract_signed_integer (buf, 1);
      if (fi)
	fi->stack_size = stack_size;

      /* We just consumed 3 bytes.  */
      addr += 3;

      /* No more prologue insns follow, so begin preparation to return.  */
      /* Fix fi->frame if it's bogus at this point.  */
      if (fi && fi->next == NULL && (fi->status & MY_FRAME_IN_SP))
	fi->frame = read_sp () - stack_size;

      /* Note if/where callee saved registers were saved.  */
      if (fi && found_movm)
	{
	  fi->fsr.regs[7] = fi->frame;
	  fi->fsr.regs[6] = fi->frame + 4;
	  fi->fsr.regs[3] = fi->frame + 8;
	  fi->fsr.regs[2] = fi->frame + 12;
	}
      return addr;
    }

  if (buf[0] == 0xfa && buf[1] == 0xfe)
    {
      /* Suck in two more bytes, they'll hold the size of the current frame.  */
      status = target_read_memory (addr + 2, buf, 2);
      if (status != 0)
	{
	  /* Fix fi->frame if it's bogus at this point.  */
	  if (fi && fi->next == NULL && (fi->status & MY_FRAME_IN_SP))
	    fi->frame = read_sp ();

	  /* Note if/where callee saved registers were saved.  */
	  if (fi && found_movm)
	    {
	      fi->fsr.regs[7] = fi->frame;
	      fi->fsr.regs[6] = fi->frame + 4;
	      fi->fsr.regs[3] = fi->frame + 8;
	      fi->fsr.regs[2] = fi->frame + 12;
	    }
	  return addr;
	}

      /* Note the size of the stack in the frame info structure.  */
      stack_size = extract_signed_integer (buf, 2);
      if (fi)
	fi->stack_size = stack_size;

      /* We just consumed 4 bytes.  */
      addr += 4;

      /* No more prologue insns follow, so begin preparation to return.  */
      /* Fix fi->frame if it's bogus at this point.  */
      if (fi && fi->next == NULL && (fi->status & MY_FRAME_IN_SP))
	fi->frame = read_sp () - stack_size;

      /* Note if/where callee saved registers were saved.  */
      if (fi && found_movm)
	{
	  fi->fsr.regs[7] = fi->frame;
	  fi->fsr.regs[6] = fi->frame + 4;
	  fi->fsr.regs[3] = fi->frame + 8;
	  fi->fsr.regs[2] = fi->frame + 12;
	}
      return addr;
    }

  if (buf[0] == 0xfc && buf[1] == 0xfe)
    {
      /* Suck in four more bytes, they'll hold the size of the current
	 frame.  */
      status = target_read_memory (addr + 2, buf, 4);
      if (status != 0)
	{
	  /* Fix fi->frame if it's bogus at this point.  */
	  if (fi && fi->next == NULL && (fi->status & MY_FRAME_IN_SP))
	    fi->frame = read_sp ();

	  /* Note if/where callee saved registers were saved.  */
	  if (fi && found_movm)
	    {
	      fi->fsr.regs[7] = fi->frame;
	      fi->fsr.regs[6] = fi->frame + 4;
	      fi->fsr.regs[3] = fi->frame + 8;
	      fi->fsr.regs[2] = fi->frame + 12;
	    }
	  return addr;
	}

      /* Note the size of the stack in the frame info structure.  */
      stack_size = extract_signed_integer (buf, 4);
      if (fi)
	fi->stack_size = stack_size;

      /* We just consumed 6 bytes.  */
      addr += 6;

      /* No more prologue insns follow, so begin preparation to return.  */
      /* Fix fi->frame if it's bogus at this point.  */
      if (fi && fi->next == NULL && (fi->status & MY_FRAME_IN_SP))
	fi->frame = read_sp () - stack_size;

      /* Note if/where callee saved registers were saved.  */
      if (fi && found_movm)
	{
	  fi->fsr.regs[7] = fi->frame;
	  fi->fsr.regs[6] = fi->frame + 4;
	  fi->fsr.regs[3] = fi->frame + 8;
	  fi->fsr.regs[2] = fi->frame + 12;
	}
      return addr;
    }

  /* We never found an insn which allocates local stack space, regardless
     this is the end of the prologue.  */
  /* Fix fi->frame if it's bogus at this point.  */
  if (fi && fi->next == NULL && (fi->status & MY_FRAME_IN_SP))
    fi->frame = read_sp ();

  /* Note if/where callee saved registers were saved.  */
  if (fi && found_movm)
    {
      fi->fsr.regs[7] = fi->frame;
      fi->fsr.regs[6] = fi->frame + 4;
      fi->fsr.regs[3] = fi->frame + 8;
      fi->fsr.regs[2] = fi->frame + 12;
    }
  return addr;
}
  
/* Function: frame_chain
   Figure out and return the caller's frame pointer given current
   frame_info struct.

   We don't handle dummy frames yet but we would probably just return the
   stack pointer that was in use at the time the function call was made?  */

CORE_ADDR
mn10300_frame_chain (fi)
     struct frame_info *fi;
{
  struct frame_info dummy_frame;

  /* Walk through the prologue to determine the stack size,
     location of saved registers, end of the prologue, etc.  */
  if (fi->status == 0)
    mn10300_analyze_prologue (fi, (CORE_ADDR)0);

  /* Quit now if mn10300_analyze_prologue set NO_MORE_FRAMES.  */
  if (fi->status & NO_MORE_FRAMES)
    return 0;

  /* Now that we've analyzed our prologue, determine the frame
     pointer for our caller.

       If our caller has a frame pointer, then we need to
       find the entry value of $a3 to our function.

	 If fsr.regs[7] is nonzero, then it's at the memory
	 location pointed to by fsr.regs[7].

	 Else it's still in $a3.

       If our caller does not have a frame pointer, then his
       frame base is fi->frame + -caller's stack size.  */
       
  /* The easiest way to get that info is to analyze our caller's frame.

     So we set up a dummy frame and call mn10300_analyze_prologue to
     find stuff for us.  */
  dummy_frame.pc = FRAME_SAVED_PC (fi);
  dummy_frame.frame = fi->frame;
  memset (dummy_frame.fsr.regs, '\000', sizeof dummy_frame.fsr.regs);
  dummy_frame.status = 0;
  dummy_frame.stack_size = 0;
  mn10300_analyze_prologue (&dummy_frame);

  if (dummy_frame.status & MY_FRAME_IN_FP)
    {
      /* Our caller has a frame pointer.  So find the frame in $a3 or
         in the stack.  */
      if (fi->fsr.regs[7])
	return (read_memory_integer (fi->fsr.regs[FP_REGNUM], REGISTER_SIZE));
      else
	return read_register (FP_REGNUM);
    }
  else
    {
      int adjust = 0;

      adjust += (fi->fsr.regs[2] ? 4 : 0);
      adjust += (fi->fsr.regs[3] ? 4 : 0);
      adjust += (fi->fsr.regs[6] ? 4 : 0);
      adjust += (fi->fsr.regs[7] ? 4 : 0);

      /* Our caller does not have a frame pointer.  So his frame starts
	 at the base of our frame (fi->frame) + register save space
	 + <his size>.  */
      return fi->frame + adjust + -dummy_frame.stack_size;
    }
}

/* Function: skip_prologue
   Return the address of the first inst past the prologue of the function.  */

CORE_ADDR
mn10300_skip_prologue (pc)
     CORE_ADDR pc;
{
  /* We used to check the debug symbols, but that can lose if
     we have a null prologue.  */
  return mn10300_analyze_prologue (NULL, pc);
}


/* Function: pop_frame
   This routine gets called when either the user uses the `return'
   command, or the call dummy breakpoint gets hit.  */

void
mn10300_pop_frame (frame)
     struct frame_info *frame;
{
  int regnum;

  if (PC_IN_CALL_DUMMY(frame->pc, frame->frame, frame->frame))
    generic_pop_dummy_frame ();
  else
    {
      write_register (PC_REGNUM, FRAME_SAVED_PC (frame));

      /* Restore any saved registers.  */
      for (regnum = 0; regnum < NUM_REGS; regnum++)
	if (frame->fsr.regs[regnum] != 0)
	  {
	    ULONGEST value;

	    value = read_memory_unsigned_integer (frame->fsr.regs[regnum],
						  REGISTER_RAW_SIZE (regnum));
	    write_register (regnum, value);
	  }

      /* Actually cut back the stack.  */
      write_register (SP_REGNUM, FRAME_FP (frame));

      /* Don't we need to set the PC?!?  XXX FIXME.  */
    }

  /* Throw away any cached frame information.  */
  flush_cached_frames ();
}

/* Function: push_arguments
   Setup arguments for a call to the target.  Arguments go in
   order on the stack.  */

CORE_ADDR
mn10300_push_arguments (nargs, args, sp, struct_return, struct_addr)
     int nargs;
     value_ptr *args;
     CORE_ADDR sp;
     unsigned char struct_return;
     CORE_ADDR struct_addr;
{
  int argnum = 0;
  int len = 0;
  int stack_offset = 0;
  int regsused = struct_return ? 1 : 0;

  /* This should be a nop, but align the stack just in case something
     went wrong.  Stacks are four byte aligned on the mn10300.  */
  sp &= ~3;

  /* Now make space on the stack for the args.

     XXX This doesn't appear to handle pass-by-invisible reference
     arguments.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      int arg_length = (TYPE_LENGTH (VALUE_TYPE (args[argnum])) + 3) & ~3;

      while (regsused < 2 && arg_length > 0)
	{
	  regsused++;
	  arg_length -= 4;
	}
      len += arg_length;
    }

  /* Allocate stack space.  */
  sp -= len;

  regsused = struct_return ? 1 : 0;
  /* Push all arguments onto the stack. */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      int len;
      char *val;

      /* XXX Check this.  What about UNIONS?  */
      if (TYPE_CODE (VALUE_TYPE (*args)) == TYPE_CODE_STRUCT
	  && TYPE_LENGTH (VALUE_TYPE (*args)) > 8)
	{
	  /* XXX Wrong, we want a pointer to this argument.  */
          len = TYPE_LENGTH (VALUE_TYPE (*args));
          val = (char *)VALUE_CONTENTS (*args);
	}
      else
	{
	  len = TYPE_LENGTH (VALUE_TYPE (*args));
	  val = (char *)VALUE_CONTENTS (*args);
	}

      while (regsused < 2 && len > 0)
	{
	  write_register (regsused, extract_unsigned_integer (val, 4));
	  val += 4;
	  len -= 4;
	  regsused++;
	}

      while (len > 0)
	{
	  write_memory (sp + stack_offset, val, 4);
	  len -= 4;
	  val += 4;
	  stack_offset += 4;
	}

      args++;
    }

  /* Make space for the flushback area.  */
  sp -= 8;
  return sp;
}

/* Function: push_return_address (pc)
   Set up the return address for the inferior function call.
   Needed for targets where we don't actually execute a JSR/BSR instruction */
 
CORE_ADDR
mn10300_push_return_address (pc, sp)
     CORE_ADDR pc;
     CORE_ADDR sp;
{
  unsigned char buf[4];

  store_unsigned_integer (buf, 4, CALL_DUMMY_ADDRESS ());
  write_memory (sp - 4, buf, 4);
  return sp - 4;
}

/* Function: store_struct_return (addr,sp)
   Store the structure value return address for an inferior function
   call.  */
 
CORE_ADDR
mn10300_store_struct_return (addr, sp)
     CORE_ADDR addr;
     CORE_ADDR sp;
{
  /* The structure return address is passed as the first argument.  */
  write_register (0, addr);
  return sp;
}
 
/* Function: frame_saved_pc 
   Find the caller of this frame.  We do this by seeing if RP_REGNUM
   is saved in the stack anywhere, otherwise we get it from the
   registers.  If the inner frame is a dummy frame, return its PC
   instead of RP, because that's where "caller" of the dummy-frame
   will be found.  */

CORE_ADDR
mn10300_frame_saved_pc (fi)
     struct frame_info *fi;
{
  int adjust = 0;

  adjust += (fi->fsr.regs[2] ? 4 : 0);
  adjust += (fi->fsr.regs[3] ? 4 : 0);
  adjust += (fi->fsr.regs[6] ? 4 : 0);
  adjust += (fi->fsr.regs[7] ? 4 : 0);

  return (read_memory_integer (fi->frame + adjust, REGISTER_SIZE));
}

void
get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval)
     char *raw_buffer;
     int *optimized;
     CORE_ADDR *addrp;
     struct frame_info *frame;
     int regnum;
     enum lval_type *lval;
{
  generic_get_saved_register (raw_buffer, optimized, addrp, 
			      frame, regnum, lval);
}

/* Function: init_extra_frame_info
   Setup the frame's frame pointer, pc, and frame addresses for saved
   registers.  Most of the work is done in mn10300_analyze_prologue().

   Note that when we are called for the last frame (currently active frame),
   that fi->pc and fi->frame will already be setup.  However, fi->frame will
   be valid only if this routine uses FP.  For previous frames, fi-frame will
   always be correct.  mn10300_analyze_prologue will fix fi->frame if
   it's not valid.

   We can be called with the PC in the call dummy under two circumstances.
   First, during normal backtracing, second, while figuring out the frame
   pointer just prior to calling the target function (see run_stack_dummy).  */

void
mn10300_init_extra_frame_info (fi)
     struct frame_info *fi;
{
  if (fi->next)
    fi->pc = FRAME_SAVED_PC (fi->next);

  memset (fi->fsr.regs, '\000', sizeof fi->fsr.regs);
  fi->status = 0;
  fi->stack_size = 0;

  mn10300_analyze_prologue (fi, 0);
}

void
_initialize_mn10300_tdep ()
{
/*  printf("_initialize_mn10300_tdep\n"); */

  tm_print_insn = print_insn_mn10300;
}

