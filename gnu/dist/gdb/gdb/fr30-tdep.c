// OBSOLETE /* Target-dependent code for the Fujitsu FR30.
// OBSOLETE    Copyright 1999, 2000, 2001 Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "frame.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "obstack.h"
// OBSOLETE #include "target.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "bfd.h"
// OBSOLETE #include "gdb_string.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "symfile.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE /* An expression that tells us whether the function invocation represented
// OBSOLETE    by FI does not have a frame on the stack associated with it.  */
// OBSOLETE int
// OBSOLETE fr30_frameless_function_invocation (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   int frameless;
// OBSOLETE   CORE_ADDR func_start, after_prologue;
// OBSOLETE   func_start = (get_pc_function_start ((fi)->pc) +
// OBSOLETE 		FUNCTION_START_OFFSET);
// OBSOLETE   after_prologue = func_start;
// OBSOLETE   after_prologue = SKIP_PROLOGUE (after_prologue);
// OBSOLETE   frameless = (after_prologue == func_start);
// OBSOLETE   return frameless;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Function: pop_frame
// OBSOLETE    This routine gets called when either the user uses the `return'
// OBSOLETE    command, or the call dummy breakpoint gets hit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30_pop_frame (void)
// OBSOLETE {
// OBSOLETE   struct frame_info *frame = get_current_frame ();
// OBSOLETE   int regnum;
// OBSOLETE   CORE_ADDR sp = read_register (SP_REGNUM);
// OBSOLETE 
// OBSOLETE   if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
// OBSOLETE     generic_pop_dummy_frame ();
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       write_register (PC_REGNUM, FRAME_SAVED_PC (frame));
// OBSOLETE 
// OBSOLETE       for (regnum = 0; regnum < NUM_REGS; regnum++)
// OBSOLETE 	if (frame->fsr.regs[regnum] != 0)
// OBSOLETE 	  {
// OBSOLETE 	    write_register (regnum,
// OBSOLETE 		      read_memory_unsigned_integer (frame->fsr.regs[regnum],
// OBSOLETE 					       REGISTER_RAW_SIZE (regnum)));
// OBSOLETE 	  }
// OBSOLETE       write_register (SP_REGNUM, sp + frame->framesize);
// OBSOLETE     }
// OBSOLETE   flush_cached_frames ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Function: fr30_store_return_value
// OBSOLETE    Put a value where a caller expects to see it.  Used by the 'return'
// OBSOLETE    command.  */
// OBSOLETE void
// OBSOLETE fr30_store_return_value (struct type *type,
// OBSOLETE 			 char *valbuf)
// OBSOLETE {
// OBSOLETE   /* Here's how the FR30 returns values (gleaned from gcc/config/
// OBSOLETE      fr30/fr30.h):
// OBSOLETE 
// OBSOLETE      If the return value is 32 bits long or less, it goes in r4.
// OBSOLETE 
// OBSOLETE      If the return value is 64 bits long or less, it goes in r4 (most
// OBSOLETE      significant word) and r5 (least significant word.
// OBSOLETE 
// OBSOLETE      If the function returns a structure, of any size, the caller
// OBSOLETE      passes the function an invisible first argument where the callee
// OBSOLETE      should store the value.  But GDB doesn't let you do that anyway.
// OBSOLETE 
// OBSOLETE      If you're returning a value smaller than a word, it's not really
// OBSOLETE      necessary to zero the upper bytes of the register; the caller is
// OBSOLETE      supposed to ignore them.  However, the FR30 typically keeps its
// OBSOLETE      values extended to the full register width, so we should emulate
// OBSOLETE      that.  */
// OBSOLETE 
// OBSOLETE   /* The FR30 is big-endian, so if we return a small value (like a
// OBSOLETE      short or a char), we need to position it correctly within the
// OBSOLETE      register.  We round the size up to a register boundary, and then
// OBSOLETE      adjust the offset so as to place the value at the right end.  */
// OBSOLETE   int value_size = TYPE_LENGTH (type);
// OBSOLETE   int returned_size = (value_size + FR30_REGSIZE - 1) & ~(FR30_REGSIZE - 1);
// OBSOLETE   int offset = (REGISTER_BYTE (RETVAL_REG)
// OBSOLETE 		+ (returned_size - value_size));
// OBSOLETE   char *zeros = alloca (returned_size);
// OBSOLETE   memset (zeros, 0, returned_size);
// OBSOLETE 
// OBSOLETE   write_register_bytes (REGISTER_BYTE (RETVAL_REG), zeros, returned_size);
// OBSOLETE   write_register_bytes (offset, valbuf, value_size);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Function: skip_prologue
// OBSOLETE    Return the address of the first code past the prologue of the function.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE fr30_skip_prologue (CORE_ADDR pc)
// OBSOLETE {
// OBSOLETE   CORE_ADDR func_addr, func_end;
// OBSOLETE 
// OBSOLETE   /* See what the symbol table says */
// OBSOLETE 
// OBSOLETE   if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
// OBSOLETE     {
// OBSOLETE       struct symtab_and_line sal;
// OBSOLETE 
// OBSOLETE       sal = find_pc_line (func_addr, 0);
// OBSOLETE 
// OBSOLETE       if (sal.line != 0 && sal.end < func_end)
// OBSOLETE 	{
// OBSOLETE 	  return sal.end;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE /* Either we didn't find the start of this function (nothing we can do),
// OBSOLETE    or there's no line info, or the line after the prologue is after
// OBSOLETE    the end of the function (there probably isn't a prologue). */
// OBSOLETE 
// OBSOLETE   return pc;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Function: push_arguments
// OBSOLETE    Setup arguments and RP for a call to the target.  First four args
// OBSOLETE    go in FIRST_ARGREG -> LAST_ARGREG, subsequent args go on stack...
// OBSOLETE    Structs are passed by reference.  XXX not right now Z.R.
// OBSOLETE    64 bit quantities (doubles and long longs) may be split between
// OBSOLETE    the regs and the stack.
// OBSOLETE    When calling a function that returns a struct, a pointer to the struct
// OBSOLETE    is passed in as a secret first argument (always in FIRST_ARGREG).
// OBSOLETE 
// OBSOLETE    Stack space for the args has NOT been allocated: that job is up to us.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE fr30_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
// OBSOLETE 		     int struct_return, CORE_ADDR struct_addr)
// OBSOLETE {
// OBSOLETE   int argreg;
// OBSOLETE   int argnum;
// OBSOLETE   int stack_offset;
// OBSOLETE   struct stack_arg
// OBSOLETE     {
// OBSOLETE       char *val;
// OBSOLETE       int len;
// OBSOLETE       int offset;
// OBSOLETE     };
// OBSOLETE   struct stack_arg *stack_args =
// OBSOLETE   (struct stack_arg *) alloca (nargs * sizeof (struct stack_arg));
// OBSOLETE   int nstack_args = 0;
// OBSOLETE 
// OBSOLETE   argreg = FIRST_ARGREG;
// OBSOLETE 
// OBSOLETE   /* the struct_return pointer occupies the first parameter-passing reg */
// OBSOLETE   if (struct_return)
// OBSOLETE     write_register (argreg++, struct_addr);
// OBSOLETE 
// OBSOLETE   stack_offset = 0;
// OBSOLETE 
// OBSOLETE   /* Process args from left to right.  Store as many as allowed in
// OBSOLETE      registers, save the rest to be pushed on the stack */
// OBSOLETE   for (argnum = 0; argnum < nargs; argnum++)
// OBSOLETE     {
// OBSOLETE       char *val;
// OBSOLETE       struct value *arg = args[argnum];
// OBSOLETE       struct type *arg_type = check_typedef (VALUE_TYPE (arg));
// OBSOLETE       struct type *target_type = TYPE_TARGET_TYPE (arg_type);
// OBSOLETE       int len = TYPE_LENGTH (arg_type);
// OBSOLETE       enum type_code typecode = TYPE_CODE (arg_type);
// OBSOLETE       CORE_ADDR regval;
// OBSOLETE       int newarg;
// OBSOLETE 
// OBSOLETE       val = (char *) VALUE_CONTENTS (arg);
// OBSOLETE 
// OBSOLETE       {
// OBSOLETE 	/* Copy the argument to general registers or the stack in
// OBSOLETE 	   register-sized pieces.  Large arguments are split between
// OBSOLETE 	   registers and stack.  */
// OBSOLETE 	while (len > 0)
// OBSOLETE 	  {
// OBSOLETE 	    if (argreg <= LAST_ARGREG)
// OBSOLETE 	      {
// OBSOLETE 		int partial_len = len < REGISTER_SIZE ? len : REGISTER_SIZE;
// OBSOLETE 		regval = extract_address (val, partial_len);
// OBSOLETE 
// OBSOLETE 		/* It's a simple argument being passed in a general
// OBSOLETE 		   register.  */
// OBSOLETE 		write_register (argreg, regval);
// OBSOLETE 		argreg++;
// OBSOLETE 		len -= partial_len;
// OBSOLETE 		val += partial_len;
// OBSOLETE 	      }
// OBSOLETE 	    else
// OBSOLETE 	      {
// OBSOLETE 		/* keep for later pushing */
// OBSOLETE 		stack_args[nstack_args].val = val;
// OBSOLETE 		stack_args[nstack_args++].len = len;
// OBSOLETE 		break;
// OBSOLETE 	      }
// OBSOLETE 	  }
// OBSOLETE       }
// OBSOLETE     }
// OBSOLETE   /* now do the real stack pushing, process args right to left */
// OBSOLETE   while (nstack_args--)
// OBSOLETE     {
// OBSOLETE       sp -= stack_args[nstack_args].len;
// OBSOLETE       write_memory (sp, stack_args[nstack_args].val,
// OBSOLETE 		    stack_args[nstack_args].len);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Return adjusted stack pointer.  */
// OBSOLETE   return sp;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void _initialize_fr30_tdep (void);
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_fr30_tdep (void)
// OBSOLETE {
// OBSOLETE   extern int print_insn_fr30 (bfd_vma, disassemble_info *);
// OBSOLETE   tm_print_insn = print_insn_fr30;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Function: check_prologue_cache
// OBSOLETE    Check if prologue for this frame's PC has already been scanned.
// OBSOLETE    If it has, copy the relevant information about that prologue and
// OBSOLETE    return non-zero.  Otherwise do not copy anything and return zero.
// OBSOLETE 
// OBSOLETE    The information saved in the cache includes:
// OBSOLETE    * the frame register number;
// OBSOLETE    * the size of the stack frame;
// OBSOLETE    * the offsets of saved regs (relative to the old SP); and
// OBSOLETE    * the offset from the stack pointer to the frame pointer
// OBSOLETE 
// OBSOLETE    The cache contains only one entry, since this is adequate
// OBSOLETE    for the typical sequence of prologue scan requests we get.
// OBSOLETE    When performing a backtrace, GDB will usually ask to scan
// OBSOLETE    the same function twice in a row (once to get the frame chain,
// OBSOLETE    and once to fill in the extra frame information).
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static struct frame_info prologue_cache;
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE check_prologue_cache (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   if (fi->pc == prologue_cache.pc)
// OBSOLETE     {
// OBSOLETE       fi->framereg = prologue_cache.framereg;
// OBSOLETE       fi->framesize = prologue_cache.framesize;
// OBSOLETE       fi->frameoffset = prologue_cache.frameoffset;
// OBSOLETE       for (i = 0; i <= NUM_REGS; i++)
// OBSOLETE 	fi->fsr.regs[i] = prologue_cache.fsr.regs[i];
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Function: save_prologue_cache
// OBSOLETE    Copy the prologue information from fi to the prologue cache.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE save_prologue_cache (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   prologue_cache.pc = fi->pc;
// OBSOLETE   prologue_cache.framereg = fi->framereg;
// OBSOLETE   prologue_cache.framesize = fi->framesize;
// OBSOLETE   prologue_cache.frameoffset = fi->frameoffset;
// OBSOLETE 
// OBSOLETE   for (i = 0; i <= NUM_REGS; i++)
// OBSOLETE     {
// OBSOLETE       prologue_cache.fsr.regs[i] = fi->fsr.regs[i];
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Function: scan_prologue
// OBSOLETE    Scan the prologue of the function that contains PC, and record what
// OBSOLETE    we find in PI.  PI->fsr must be zeroed by the called.  Returns the
// OBSOLETE    pc after the prologue.  Note that the addresses saved in pi->fsr
// OBSOLETE    are actually just frame relative (negative offsets from the frame
// OBSOLETE    pointer).  This is because we don't know the actual value of the
// OBSOLETE    frame pointer yet.  In some circumstances, the frame pointer can't
// OBSOLETE    be determined till after we have scanned the prologue.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE fr30_scan_prologue (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   int sp_offset, fp_offset;
// OBSOLETE   CORE_ADDR prologue_start, prologue_end, current_pc;
// OBSOLETE 
// OBSOLETE   /* Check if this function is already in the cache of frame information. */
// OBSOLETE   if (check_prologue_cache (fi))
// OBSOLETE     return;
// OBSOLETE 
// OBSOLETE   /* Assume there is no frame until proven otherwise.  */
// OBSOLETE   fi->framereg = SP_REGNUM;
// OBSOLETE   fi->framesize = 0;
// OBSOLETE   fi->frameoffset = 0;
// OBSOLETE 
// OBSOLETE   /* Find the function prologue.  If we can't find the function in
// OBSOLETE      the symbol table, peek in the stack frame to find the PC.  */
// OBSOLETE   if (find_pc_partial_function (fi->pc, NULL, &prologue_start, &prologue_end))
// OBSOLETE     {
// OBSOLETE       /* Assume the prologue is everything between the first instruction
// OBSOLETE          in the function and the first source line.  */
// OBSOLETE       struct symtab_and_line sal = find_pc_line (prologue_start, 0);
// OBSOLETE 
// OBSOLETE       if (sal.line == 0)	/* no line info, use current PC */
// OBSOLETE 	prologue_end = fi->pc;
// OBSOLETE       else if (sal.end < prologue_end)	/* next line begins after fn end */
// OBSOLETE 	prologue_end = sal.end;	/* (probably means no prologue)  */
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* XXX Z.R. What now??? The following is entirely bogus */
// OBSOLETE       prologue_start = (read_memory_integer (fi->frame, 4) & 0x03fffffc) - 12;
// OBSOLETE       prologue_end = prologue_start + 40;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Now search the prologue looking for instructions that set up the
// OBSOLETE      frame pointer, adjust the stack pointer, and save registers.  */
// OBSOLETE 
// OBSOLETE   sp_offset = fp_offset = 0;
// OBSOLETE   for (current_pc = prologue_start; current_pc < prologue_end; current_pc += 2)
// OBSOLETE     {
// OBSOLETE       unsigned int insn;
// OBSOLETE 
// OBSOLETE       insn = read_memory_unsigned_integer (current_pc, 2);
// OBSOLETE 
// OBSOLETE       if ((insn & 0xfe00) == 0x8e00)	/* stm0 or stm1 */
// OBSOLETE 	{
// OBSOLETE 	  int reg, mask = insn & 0xff;
// OBSOLETE 
// OBSOLETE 	  /* scan in one sweep - create virtual 16-bit mask from either insn's mask */
// OBSOLETE 	  if ((insn & 0x0100) == 0)
// OBSOLETE 	    {
// OBSOLETE 	      mask <<= 8;	/* stm0 - move to upper byte in virtual mask */
// OBSOLETE 	    }
// OBSOLETE 
// OBSOLETE 	  /* Calculate offsets of saved registers (to be turned later into addresses). */
// OBSOLETE 	  for (reg = R4_REGNUM; reg <= R11_REGNUM; reg++)
// OBSOLETE 	    if (mask & (1 << (15 - reg)))
// OBSOLETE 	      {
// OBSOLETE 		sp_offset -= 4;
// OBSOLETE 		fi->fsr.regs[reg] = sp_offset;
// OBSOLETE 	      }
// OBSOLETE 	}
// OBSOLETE       else if ((insn & 0xfff0) == 0x1700)	/* st rx,@-r15 */
// OBSOLETE 	{
// OBSOLETE 	  int reg = insn & 0xf;
// OBSOLETE 
// OBSOLETE 	  sp_offset -= 4;
// OBSOLETE 	  fi->fsr.regs[reg] = sp_offset;
// OBSOLETE 	}
// OBSOLETE       else if ((insn & 0xff00) == 0x0f00)	/* enter */
// OBSOLETE 	{
// OBSOLETE 	  fp_offset = fi->fsr.regs[FP_REGNUM] = sp_offset - 4;
// OBSOLETE 	  sp_offset -= 4 * (insn & 0xff);
// OBSOLETE 	  fi->framereg = FP_REGNUM;
// OBSOLETE 	}
// OBSOLETE       else if (insn == 0x1781)	/* st rp,@-sp */
// OBSOLETE 	{
// OBSOLETE 	  sp_offset -= 4;
// OBSOLETE 	  fi->fsr.regs[RP_REGNUM] = sp_offset;
// OBSOLETE 	}
// OBSOLETE       else if (insn == 0x170e)	/* st fp,@-sp */
// OBSOLETE 	{
// OBSOLETE 	  sp_offset -= 4;
// OBSOLETE 	  fi->fsr.regs[FP_REGNUM] = sp_offset;
// OBSOLETE 	}
// OBSOLETE       else if (insn == 0x8bfe)	/* mov sp,fp */
// OBSOLETE 	{
// OBSOLETE 	  fi->framereg = FP_REGNUM;
// OBSOLETE 	}
// OBSOLETE       else if ((insn & 0xff00) == 0xa300)	/* addsp xx */
// OBSOLETE 	{
// OBSOLETE 	  sp_offset += 4 * (signed char) (insn & 0xff);
// OBSOLETE 	}
// OBSOLETE       else if ((insn & 0xff0f) == 0x9b00 &&	/* ldi:20 xx,r0 */
// OBSOLETE 	       read_memory_unsigned_integer (current_pc + 4, 2)
// OBSOLETE 	       == 0xac0f)	/* sub r0,sp */
// OBSOLETE 	{
// OBSOLETE 	  /* large stack adjustment */
// OBSOLETE 	  sp_offset -= (((insn & 0xf0) << 12) | read_memory_unsigned_integer (current_pc + 2, 2));
// OBSOLETE 	  current_pc += 4;
// OBSOLETE 	}
// OBSOLETE       else if (insn == 0x9f80 &&	/* ldi:32 xx,r0 */
// OBSOLETE 	       read_memory_unsigned_integer (current_pc + 6, 2)
// OBSOLETE 	       == 0xac0f)	/* sub r0,sp */
// OBSOLETE 	{
// OBSOLETE 	  /* large stack adjustment */
// OBSOLETE 	  sp_offset -=
// OBSOLETE 	    (read_memory_unsigned_integer (current_pc + 2, 2) << 16 |
// OBSOLETE 	     read_memory_unsigned_integer (current_pc + 4, 2));
// OBSOLETE 	  current_pc += 6;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* The frame size is just the negative of the offset (from the original SP)
// OBSOLETE      of the last thing thing we pushed on the stack.  The frame offset is
// OBSOLETE      [new FP] - [new SP].  */
// OBSOLETE   fi->framesize = -sp_offset;
// OBSOLETE   fi->frameoffset = fp_offset - sp_offset;
// OBSOLETE 
// OBSOLETE   save_prologue_cache (fi);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Function: init_extra_frame_info
// OBSOLETE    Setup the frame's frame pointer, pc, and frame addresses for saved
// OBSOLETE    registers.  Most of the work is done in scan_prologue().
// OBSOLETE 
// OBSOLETE    Note that when we are called for the last frame (currently active frame),
// OBSOLETE    that fi->pc and fi->frame will already be setup.  However, fi->frame will
// OBSOLETE    be valid only if this routine uses FP.  For previous frames, fi-frame will
// OBSOLETE    always be correct (since that is derived from fr30_frame_chain ()).
// OBSOLETE 
// OBSOLETE    We can be called with the PC in the call dummy under two circumstances.
// OBSOLETE    First, during normal backtracing, second, while figuring out the frame
// OBSOLETE    pointer just prior to calling the target function (see run_stack_dummy).  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30_init_extra_frame_info (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   int reg;
// OBSOLETE 
// OBSOLETE   if (fi->next)
// OBSOLETE     fi->pc = FRAME_SAVED_PC (fi->next);
// OBSOLETE 
// OBSOLETE   memset (fi->fsr.regs, '\000', sizeof fi->fsr.regs);
// OBSOLETE 
// OBSOLETE   if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
// OBSOLETE     {
// OBSOLETE       /* We need to setup fi->frame here because run_stack_dummy gets it wrong
// OBSOLETE          by assuming it's always FP.  */
// OBSOLETE       fi->frame = generic_read_register_dummy (fi->pc, fi->frame, SP_REGNUM);
// OBSOLETE       fi->framesize = 0;
// OBSOLETE       fi->frameoffset = 0;
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE   fr30_scan_prologue (fi);
// OBSOLETE 
// OBSOLETE   if (!fi->next)		/* this is the innermost frame? */
// OBSOLETE     fi->frame = read_register (fi->framereg);
// OBSOLETE   else
// OBSOLETE     /* not the innermost frame */
// OBSOLETE     /* If we have an FP,  the callee saved it. */
// OBSOLETE     if (fi->framereg == FP_REGNUM)
// OBSOLETE       if (fi->next->fsr.regs[fi->framereg] != 0)
// OBSOLETE 	fi->frame = read_memory_integer (fi->next->fsr.regs[fi->framereg], 4);
// OBSOLETE 
// OBSOLETE   /* Calculate actual addresses of saved registers using offsets determined
// OBSOLETE      by fr30_scan_prologue.  */
// OBSOLETE   for (reg = 0; reg < NUM_REGS; reg++)
// OBSOLETE     if (fi->fsr.regs[reg] != 0)
// OBSOLETE       {
// OBSOLETE 	fi->fsr.regs[reg] += fi->frame + fi->framesize - fi->frameoffset;
// OBSOLETE       }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Function: find_callers_reg
// OBSOLETE    Find REGNUM on the stack.  Otherwise, it's in an active register.
// OBSOLETE    One thing we might want to do here is to check REGNUM against the
// OBSOLETE    clobber mask, and somehow flag it as invalid if it isn't saved on
// OBSOLETE    the stack somewhere.  This would provide a graceful failure mode
// OBSOLETE    when trying to get the value of caller-saves registers for an inner
// OBSOLETE    frame.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE fr30_find_callers_reg (struct frame_info *fi, int regnum)
// OBSOLETE {
// OBSOLETE   for (; fi; fi = fi->next)
// OBSOLETE     if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
// OBSOLETE       return generic_read_register_dummy (fi->pc, fi->frame, regnum);
// OBSOLETE     else if (fi->fsr.regs[regnum] != 0)
// OBSOLETE       return read_memory_unsigned_integer (fi->fsr.regs[regnum],
// OBSOLETE 					   REGISTER_RAW_SIZE (regnum));
// OBSOLETE 
// OBSOLETE   return read_register (regnum);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Function: frame_chain
// OBSOLETE    Figure out the frame prior to FI.  Unfortunately, this involves
// OBSOLETE    scanning the prologue of the caller, which will also be done
// OBSOLETE    shortly by fr30_init_extra_frame_info.  For the dummy frame, we
// OBSOLETE    just return the stack pointer that was in use at the time the
// OBSOLETE    function call was made.  */
// OBSOLETE 
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE fr30_frame_chain (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   CORE_ADDR fn_start, callers_pc, fp;
// OBSOLETE   struct frame_info caller_fi;
// OBSOLETE   int framereg;
// OBSOLETE 
// OBSOLETE   /* is this a dummy frame? */
// OBSOLETE   if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
// OBSOLETE     return fi->frame;		/* dummy frame same as caller's frame */
// OBSOLETE 
// OBSOLETE   /* is caller-of-this a dummy frame? */
// OBSOLETE   callers_pc = FRAME_SAVED_PC (fi);	/* find out who called us: */
// OBSOLETE   fp = fr30_find_callers_reg (fi, FP_REGNUM);
// OBSOLETE   if (PC_IN_CALL_DUMMY (callers_pc, fp, fp))
// OBSOLETE     return fp;			/* dummy frame's frame may bear no relation to ours */
// OBSOLETE 
// OBSOLETE   if (find_pc_partial_function (fi->pc, 0, &fn_start, 0))
// OBSOLETE     if (fn_start == entry_point_address ())
// OBSOLETE       return 0;			/* in _start fn, don't chain further */
// OBSOLETE 
// OBSOLETE   framereg = fi->framereg;
// OBSOLETE 
// OBSOLETE   /* If the caller is the startup code, we're at the end of the chain.  */
// OBSOLETE   if (find_pc_partial_function (callers_pc, 0, &fn_start, 0))
// OBSOLETE     if (fn_start == entry_point_address ())
// OBSOLETE       return 0;
// OBSOLETE 
// OBSOLETE   memset (&caller_fi, 0, sizeof (caller_fi));
// OBSOLETE   caller_fi.pc = callers_pc;
// OBSOLETE   fr30_scan_prologue (&caller_fi);
// OBSOLETE   framereg = caller_fi.framereg;
// OBSOLETE 
// OBSOLETE   /* If the caller used a frame register, return its value.
// OBSOLETE      Otherwise, return the caller's stack pointer.  */
// OBSOLETE   if (framereg == FP_REGNUM)
// OBSOLETE     return fr30_find_callers_reg (fi, framereg);
// OBSOLETE   else
// OBSOLETE     return fi->frame + fi->framesize;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Function: frame_saved_pc 
// OBSOLETE    Find the caller of this frame.  We do this by seeing if RP_REGNUM
// OBSOLETE    is saved in the stack anywhere, otherwise we get it from the
// OBSOLETE    registers.  If the inner frame is a dummy frame, return its PC
// OBSOLETE    instead of RP, because that's where "caller" of the dummy-frame
// OBSOLETE    will be found.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE fr30_frame_saved_pc (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
// OBSOLETE     return generic_read_register_dummy (fi->pc, fi->frame, PC_REGNUM);
// OBSOLETE   else
// OBSOLETE     return fr30_find_callers_reg (fi, RP_REGNUM);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Function: fix_call_dummy
// OBSOLETE    Pokes the callee function's address into the CALL_DUMMY assembly stub.
// OBSOLETE    Assumes that the CALL_DUMMY looks like this:
// OBSOLETE    jarl <offset24>, r31
// OBSOLETE    trap
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30_fix_call_dummy (char *dummy, CORE_ADDR sp, CORE_ADDR fun, int nargs,
// OBSOLETE 		     struct value **args, struct type *type, int gcc_p)
// OBSOLETE {
// OBSOLETE   long offset24;
// OBSOLETE 
// OBSOLETE   offset24 = (long) fun - (long) entry_point_address ();
// OBSOLETE   offset24 &= 0x3fffff;
// OBSOLETE   offset24 |= 0xff800000;	/* jarl <offset24>, r31 */
// OBSOLETE 
// OBSOLETE   store_unsigned_integer ((unsigned int *) &dummy[2], 2, offset24 & 0xffff);
// OBSOLETE   store_unsigned_integer ((unsigned int *) &dummy[0], 2, offset24 >> 16);
// OBSOLETE   return 0;
// OBSOLETE }
