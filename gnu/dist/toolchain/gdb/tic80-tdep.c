/* Target-dependent code for the TI TMS320C80 (MVP) for GDB, the GNU debugger.
   Copyright 1996, Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "value.h"
#include "frame.h"
#include "inferior.h"
#include "obstack.h"
#include "target.h"
#include "bfd.h"
#include "gdb_string.h"
#include "gdbcore.h"
#include "symfile.h"

/* Function: frame_find_saved_regs
   Return the frame_saved_regs structure for the frame.
   Doesn't really work for dummy frames, but it does pass back
   an empty frame_saved_regs, so I guess that's better than total failure */

void
tic80_frame_find_saved_regs (fi, regaddr)
     struct frame_info *fi;
     struct frame_saved_regs *regaddr;
{
  memcpy (regaddr, &fi->fsr, sizeof (struct frame_saved_regs));
}

/* Function: skip_prologue
   Find end of function prologue.  */

CORE_ADDR
tic80_skip_prologue (pc)
     CORE_ADDR pc;
{
  CORE_ADDR func_addr, func_end;
  struct symtab_and_line sal;

  /* See what the symbol table says */

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      sal = find_pc_line (func_addr, 0);

      if (sal.line != 0 && sal.end < func_end)
	return sal.end;
      else
	/* Either there's no line info, or the line after the prologue is after
	   the end of the function.  In this case, there probably isn't a
	   prologue.  */
	return pc;
    }

  /* We can't find the start of this function, so there's nothing we can do. */
  return pc;
}

/* Function: tic80_scan_prologue
   This function decodes the target function prologue to determine:
   1) the size of the stack frame
   2) which registers are saved on it
   3) the offsets of saved regs
   4) the frame size
   This information is stored in the "extra" fields of the frame_info.  */

static void
tic80_scan_prologue (fi)
     struct frame_info *fi;
{
  struct symtab_and_line sal;
  CORE_ADDR prologue_start, prologue_end, current_pc;

  /* Assume there is no frame until proven otherwise.  */
  fi->framereg = SP_REGNUM;
  fi->framesize = 0;
  fi->frameoffset = 0;

  /* this code essentially duplicates skip_prologue, 
     but we need the start address below.  */

  if (find_pc_partial_function (fi->pc, NULL, &prologue_start, &prologue_end))
    {
      sal = find_pc_line (prologue_start, 0);

      if (sal.line == 0)	/* no line info, use current PC */
	if (prologue_start != entry_point_address ())
	  prologue_end = fi->pc;
	else
	  return;		/* _start has no frame or prologue */
      else if (sal.end < prologue_end)	/* next line begins after fn end */
	prologue_end = sal.end;	/* (probably means no prologue)  */
    }
  else
/* FIXME */
    prologue_end = prologue_start + 40;		/* We're in the boondocks: allow for */
  /* 16 pushes, an add, and "mv fp,sp" */

  prologue_end = min (prologue_end, fi->pc);

  /* Now search the prologue looking for instructions that set up the
     frame pointer, adjust the stack pointer, and save registers.  */

  for (current_pc = prologue_start; current_pc < prologue_end; current_pc += 4)
    {
      unsigned int insn;
      int regno;
      int offset = 0;

      insn = read_memory_unsigned_integer (current_pc, 4);

      if ((insn & 0x301000) == 0x301000)	/* Long immediate? */
/* FIXME - set offset for long immediate instructions */
	current_pc += 4;
      else
	{
	  offset = insn & 0x7fff;	/* extract 15-bit offset */
	  if (offset & 0x4000)	/* if negative, sign-extend */
	    offset = -(0x8000 - offset);
	}

      if ((insn & 0x7fd0000) == 0x590000)	/* st.{w,d} reg, xx(r1) */
	{
	  regno = ((insn >> 27) & 0x1f);
	  fi->fsr.regs[regno] = offset;
	  if (insn & 0x8000)	/* 64-bit store (st.d)? */
	    fi->fsr.regs[regno + 1] = offset + 4;
	}
      else if ((insn & 0xffff8000) == 0x086c8000)	/* addu xx, r1, r1 */
	fi->framesize = -offset;
      else if ((insn & 0xffff8000) == 0xf06c8000)	/* addu xx, r1, r30 */
	{
	  fi->framereg = FP_REGNUM;	/* fp is now valid */
	  fi->frameoffset = offset;
	  break;		/* end of stack adjustments */
	}
      else if (insn == 0xf03b2001)	/* addu r1, r0, r30 */
	{
	  fi->framereg = FP_REGNUM;	/* fp is now valid */
	  fi->frameoffset = 0;
	  break;		/* end of stack adjustments */
	}
      else
/* FIXME - handle long immediate instructions */
	break;			/* anything else isn't prologue */
    }
}

/* Function: init_extra_frame_info
   This function actually figures out the frame address for a given pc and
   sp.  This is tricky on the c80 because we sometimes don't use an explicit
   frame pointer, and the previous stack pointer isn't necessarily recorded
   on the stack.  The only reliable way to get this info is to
   examine the prologue.  */

void
tic80_init_extra_frame_info (fi)
     struct frame_info *fi;
{
  int reg;

  if (fi->next)
    fi->pc = FRAME_SAVED_PC (fi->next);

  /* Because zero is a valid register offset relative to SP, we initialize
     the offsets to -1 to indicate unused entries.  */
  for (reg = 0; reg < NUM_REGS; reg++)
    fi->fsr.regs[reg] = -1;

  if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
    {
      /* We need to setup fi->frame here because run_stack_dummy gets it wrong
         by assuming it's always FP.  */
      fi->frame = generic_read_register_dummy (fi->pc, fi->frame, SP_REGNUM);
      fi->framesize = 0;
      fi->frameoffset = 0;
      return;
    }
  else
    {
      tic80_scan_prologue (fi);

      if (!fi->next)		/* this is the innermost frame? */
	fi->frame = read_register (fi->framereg);
      else
	/* not the innermost frame */
	/* If this function uses FP as the frame register, and the function
	   it called saved the FP, get the saved FP.  */ if (fi->framereg == FP_REGNUM &&
			     fi->next->fsr.regs[FP_REGNUM] != (unsigned) -1)
	fi->frame = read_memory_integer (fi->next->fsr.regs[FP_REGNUM], 4);

      /* Convert SP-relative offsets of saved registers to real addresses.  */
      for (reg = 0; reg < NUM_REGS; reg++)
	if (fi->fsr.regs[reg] == (unsigned) -1)
	  fi->fsr.regs[reg] = 0;	/* unused entry */
	else
	  fi->fsr.regs[reg] += fi->frame - fi->frameoffset;
    }
}

/* Function: find_callers_reg
   Find REGNUM on the stack.  Otherwise, it's in an active register.  One thing
   we might want to do here is to check REGNUM against the clobber mask, and
   somehow flag it as invalid if it isn't saved on the stack somewhere.  This
   would provide a graceful failure mode when trying to get the value of
   caller-saves registers for an inner frame.  */

CORE_ADDR
tic80_find_callers_reg (fi, regnum)
     struct frame_info *fi;
     int regnum;
{
  for (; fi; fi = fi->next)
    if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
      return generic_read_register_dummy (fi->pc, fi->frame, regnum);
    else if (fi->fsr.regs[regnum] != 0)
      return read_memory_integer (fi->fsr.regs[regnum],
				  REGISTER_RAW_SIZE (regnum));
  return read_register (regnum);
}

/* Function: frame_chain
   Given a GDB frame, determine the address of the calling function's frame.
   This will be used to create a new GDB frame struct, and then
   INIT_EXTRA_FRAME_INFO and INIT_FRAME_PC will be called for the new frame.
   For c80, we save the frame size when we initialize the frame_info.  */

CORE_ADDR
tic80_frame_chain (fi)
     struct frame_info *fi;
{
  CORE_ADDR fn_start, callers_pc, fp;

  /* is this a dummy frame? */
  if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
    return fi->frame;		/* dummy frame same as caller's frame */

  /* is caller-of-this a dummy frame? */
  callers_pc = FRAME_SAVED_PC (fi);	/* find out who called us: */
  fp = tic80_find_callers_reg (fi, FP_REGNUM);
  if (PC_IN_CALL_DUMMY (callers_pc, fp, fp))
    return fp;			/* dummy frame's frame may bear no relation to ours */

  if (find_pc_partial_function (fi->pc, 0, &fn_start, 0))
    if (fn_start == entry_point_address ())
      return 0;			/* in _start fn, don't chain further */

  if (fi->framereg == FP_REGNUM)
    return tic80_find_callers_reg (fi, FP_REGNUM);
  else
    return fi->frame + fi->framesize;
}

/* Function: pop_frame
   Discard from the stack the innermost frame,
   restoring all saved registers.  */

struct frame_info *
tic80_pop_frame (frame)
     struct frame_info *frame;
{
  int regnum;

  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    generic_pop_dummy_frame ();
  else
    {
      for (regnum = 0; regnum < NUM_REGS; regnum++)
	if (frame->fsr.regs[regnum] != 0)
	  write_register (regnum,
			  read_memory_integer (frame->fsr.regs[regnum], 4));

      write_register (PC_REGNUM, FRAME_SAVED_PC (frame));
      write_register (SP_REGNUM, read_register (FP_REGNUM));
#if 0
      if (read_register (PSW_REGNUM) & 0x80)
	write_register (SPU_REGNUM, read_register (SP_REGNUM));
      else
	write_register (SPI_REGNUM, read_register (SP_REGNUM));
#endif
    }
  flush_cached_frames ();
  return NULL;
}

/* Function: frame_saved_pc
   Find the caller of this frame.  We do this by seeing if LR_REGNUM is saved
   in the stack anywhere, otherwise we get it from the registers. */

CORE_ADDR
tic80_frame_saved_pc (fi)
     struct frame_info *fi;
{
  if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
    return generic_read_register_dummy (fi->pc, fi->frame, PC_REGNUM);
  else
    return tic80_find_callers_reg (fi, LR_REGNUM);
}

/* Function: tic80_push_return_address (pc, sp)
   Set up the return address for the inferior function call.
   Necessary for targets that don't actually execute a JSR/BSR instruction 
   (ie. when using an empty CALL_DUMMY) */

CORE_ADDR
tic80_push_return_address (pc, sp)
     CORE_ADDR pc;
     CORE_ADDR sp;
{
  write_register (LR_REGNUM, CALL_DUMMY_ADDRESS ());
  return sp;
}


/* Function: push_arguments
   Setup the function arguments for calling a function in the inferior.

   On the TI C80 architecture, there are six register pairs (R2/R3 to R12/13)
   which are dedicated for passing function arguments.  Up to the first six
   arguments (depending on size) may go into these registers.
   The rest go on the stack.

   Arguments that are smaller than 4 bytes will still take up a whole
   register or a whole 32-bit word on the stack, and will be
   right-justified in the register or the stack word.  This includes
   chars, shorts, and small aggregate types.

   Arguments that are four bytes or less in size are placed in the
   even-numbered register of a register pair, and the odd-numbered
   register is not used.

   Arguments of 8 bytes size (such as floating point doubles) are placed
   in a register pair.  The least significant 32-bit word is placed in
   the even-numbered register, and the most significant word in the
   odd-numbered register.

   Aggregate types with sizes between 4 and 8 bytes are passed
   entirely on the stack, and are left-justified within the
   double-word (as opposed to aggregates smaller than 4 bytes
   which are right-justified).

   Aggregates of greater than 8 bytes are first copied onto the stack, 
   and then a pointer to the copy is passed in the place of the normal
   argument (either in a register if available, or on the stack).

   Functions that must return an aggregate type can return it in the 
   normal return value registers (R2 and R3) if its size is 8 bytes or
   less.  For larger return values, the caller must allocate space for 
   the callee to copy the return value to.  A pointer to this space is
   passed as an implicit first argument, always in R0. */

CORE_ADDR
tic80_push_arguments (nargs, args, sp, struct_return, struct_addr)
     int nargs;
     value_ptr *args;
     CORE_ADDR sp;
     unsigned char struct_return;
     CORE_ADDR struct_addr;
{
  int stack_offset, stack_alloc;
  int argreg;
  int argnum;
  struct type *type;
  CORE_ADDR regval;
  char *val;
  char valbuf[4];
  int len;
  int odd_sized_struct;
  int is_struct;

  /* first force sp to a 4-byte alignment */
  sp = sp & ~3;

  argreg = ARG0_REGNUM;
  /* The "struct return pointer" pseudo-argument goes in R0 */
  if (struct_return)
    write_register (argreg++, struct_addr);

  /* Now make sure there's space on the stack */
  for (argnum = 0, stack_alloc = 0;
       argnum < nargs; argnum++)
    stack_alloc += ((TYPE_LENGTH (VALUE_TYPE (args[argnum])) + 3) & ~3);
  sp -= stack_alloc;		/* make room on stack for args */


  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  There are 16 bytes
     in four registers available.  Loop thru args from first to last.  */

  argreg = ARG0_REGNUM;
  for (argnum = 0, stack_offset = 0; argnum < nargs; argnum++)
    {
      type = VALUE_TYPE (args[argnum]);
      len = TYPE_LENGTH (type);
      memset (valbuf, 0, sizeof (valbuf));
      val = (char *) VALUE_CONTENTS (args[argnum]);

/* FIXME -- tic80 can take doubleword arguments in register pairs */
      is_struct = (type->code == TYPE_CODE_STRUCT);
      odd_sized_struct = 0;

      if (!is_struct)
	{
	  if (len < 4)
	    {			/* value gets right-justified in the register or stack word */
	      memcpy (valbuf + (4 - len), val, len);
	      val = valbuf;
	    }
	  if (len > 4 && (len & 3) != 0)
	    odd_sized_struct = 1;	/* such structs go entirely on stack */
	}
      else
	{
	  /* Structs are always passed by reference. */
	  write_register (argreg, sp + stack_offset);
	  argreg++;
	}

      while (len > 0)
	{
	  if (is_struct || argreg > ARGLAST_REGNUM || odd_sized_struct)
	    {			/* must go on the stack */
	      write_memory (sp + stack_offset, val, 4);
	      stack_offset += 4;
	    }
	  /* NOTE WELL!!!!!  This is not an "else if" clause!!!
	     That's because some things get passed on the stack
	     AND in the registers!   */
	  if (!is_struct && argreg <= ARGLAST_REGNUM)
	    {			/* there's room in a register */
	      regval = extract_address (val, REGISTER_RAW_SIZE (argreg));
	      write_register (argreg, regval);
	      argreg += 2;	/* FIXME -- what about doubleword args? */
	    }
	  /* Store the value 4 bytes at a time.  This means that things
	     larger than 4 bytes may go partly in registers and partly
	     on the stack.  */
	  len -= REGISTER_RAW_SIZE (argreg);
	  val += REGISTER_RAW_SIZE (argreg);
	}
    }
  return sp;
}

/* Function: tic80_write_sp
   Because SP is really a read-only register that mirrors either SPU or SPI,
   we must actually write one of those two as well, depending on PSW. */

void
tic80_write_sp (val)
     CORE_ADDR val;
{
#if 0
  unsigned long psw = read_register (PSW_REGNUM);

  if (psw & 0x80)		/* stack mode: user or interrupt */
    write_register (SPU_REGNUM, val);
  else
    write_register (SPI_REGNUM, val);
#endif
  write_register (SP_REGNUM, val);
}

void
_initialize_tic80_tdep ()
{
  tm_print_insn = print_insn_tic80;
}
