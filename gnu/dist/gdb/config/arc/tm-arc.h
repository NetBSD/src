/* Parameters for target machine ARC, for GDB, the GNU debugger.
   Copyright (C) 1995 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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

/* Used by arc-tdep.c to set the default cpu type.  */
#define DEFAULT_ARC_CPU_TYPE "base"

/* Byte order is selectable.  */
#define	TARGET_BYTE_ORDER_SELECTABLE

/* We have IEEE floating point, if we have any float at all.  */
#define IEEE_FLOAT

/* Offset from address of function to start of its code.
   Zero on most machines.  */
#define FUNCTION_START_OFFSET 0

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  SKIP_PROLOGUE_FRAMELESS_P advances
   the PC past some of the prologue, but stops as soon as it
   knows that the function has a frame.  Its result is equal
   to its input PC if the function is frameless, unequal otherwise.  */

#define SKIP_PROLOGUE(pc) \
  { pc = skip_prologue (pc, 0); }
#define SKIP_PROLOGUE_FRAMELESS_P(pc) \
  { pc = skip_prologue (pc, 1); }
extern CORE_ADDR skip_prologue PARAMS ((CORE_ADDR, int));

/* Sequence of bytes for breakpoint instruction.
   ??? The current value is "sr -1,[-1]" and is for the simulator only.
   The simulator watches for this and does the right thing.
   The hardware version will have to associate with each breakpoint
   the sequence "flag 1; nop; nop; nop".  IE: The breakpoint insn will not
   be a fixed set of bits but instead will be a branch to a semi-random
   address.  Presumably this will be cleaned up for "second silicon".  */
#define BIG_BREAKPOINT { 0x12, 0x1f, 0xff, 0xff }
#define LITTLE_BREAKPOINT { 0xff, 0xff, 0x1f, 0x12 }

/* Given the exposed pipeline, there isn't any one correct value.
   However, this value must be 4.  GDB can't handle any other value (other than
   zero).  See for example infrun.c:
   "prev_pc != stop_pc - DECR_PC_AFTER_BREAK"  */
/* FIXME */
#define DECR_PC_AFTER_BREAK 8

/* We don't have a reliable single step facility.
   ??? We do have a cycle single step facility, but that won't work.  */
#define NO_SINGLE_STEP

/* FIXME: Need to set STEP_SKIPS_DELAY.  */

/* Given a pc value as defined by the hardware, return the real address.
   Remember that on the ARC blink contains that status register which
   includes PC + flags (so we have to mask out the flags).  */
#define ARC_PC_TO_REAL_ADDRESS(pc) (((pc) & 0xffffff) << 2)

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function
   executes some instructions.  */

#define SAVED_PC_AFTER_CALL(frame) \
  (ARC_PC_TO_REAL_ADDRESS (read_register (BLINK_REGNUM)))

/* Stack grows upward */

#define INNER_THAN <

/* Nonzero if instruction at pc is a return instruction.
   This is the "j [blink]" insn (with or without conditionals or delay
   slots).  */

#define ABOUT_TO_RETURN(pc) \
  ((read_memory_integer(pc, 4) & 0xffffff80) == 0x380f8000)

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */
#define REGISTER_SIZE 4

/* Number of machine registers */
#define NUM_REGS 92

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

#define REGISTER_NAMES \
{ \
    /*  0 */ "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7", \
    /*  8 */ "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15", \
    /* 16 */ "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", \
    /* 24 */ "r24", "r25", "r26", "fp", "sp", "ilink1", "ilink2", "blink", \
    /* 32 */ "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39", \
    /* 40 */ "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47", \
    /* 48 */ "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55", \
    /* 56 */ "r56", "mlo", "mmid", "mhi", "lp_count", \
    /* 61 */ "status", "sema", "lp_start", "lp_end", "identity", "debug", \
    /* 67 */ "aux10", "aux11", "aux12", "aux13", "aux14", \
    /* 72 */ "aux15", "aux16", "aux17", "aux18", "aux19", \
    /* 77 */ "aux1a", "aux1b", "aux1c", "aux1d", "aux1e", \
    /* 82 */ "aux1f", "aux20", "aux21", "aux22", \
    /* 86 */ "aux30", "aux31", "aux32", "aux33", "aux40", \
    /* 91 */ "pc" \
}

/* Register numbers of various important registers (used to index
   into arrays of register names and register values).  */

#define R0_REGNUM   0	   /* First local register		*/
#define R59_REGNUM 59	   /* Last local register		*/
#define FP_REGNUM  27	   /* Contains address of executing stack frame */
#define SP_REGNUM  28      /* stack pointer */
#define BLINK_REGNUM 31    /* link register */
#define	STA_REGNUM 61	   /* processor status word */
#define PC_REGNUM  91	   /* instruction pointer */
#define AUX_BEG_REGNUM  61 /* aux reg begins */
#define AUX_END_REGNUM  90 /* aux reg ends, pc not real aux reg */

/* Fake registers used to mark immediate data.  */
#define SHIMM_FLAG_REGNUM 61
#define LIMM_REGNUM 62
#define SHIMM_REGNUM 63

#define AUX_REG_MAP \
{ \
   {  0,  1,  2,  3,  4,  5, \
     16, -1, -1, -1, -1, \
     -1, -1, -1, -1, -1, \
     -1, -1, -1, -1, 30, \
     -1, 32, 33, -1, \
      48, 49, 50, 51, 64, \
      0 \
    }, \
   {  0,  1,  2,  3,  4,  5, \
     16, -1, -1, -1, -1, \
     -1, -1, -1, -1, -1, \
     -1, -1, -1, -1, 30, \
     31, 32, 33, -1, \
     -1, -1, -1, -1, -1, \
      0 \
    }, \
   {  0,  1,  2,  3,  4,  5, \
      16, 17, 18, 19, 20, \
      21, 22, 23, 24, 25, \
      26, 27, 28, 29, 30, \
      31, 32, 33, 34, \
     -1, -1, -1, -1, -1, \
      0 \
    } \
}
   
#define PFP_REGNUM R0_REGNUM	/* Previous frame pointer	*/

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */
#define REGISTER_BYTES (NUM_REGS * 4)

/* Index within `registers' of the first byte of the space for register N.  */
#define REGISTER_BYTE(N) (4*(N))

/* Number of bytes of storage in the actual machine representation
   for register N. */
#define REGISTER_RAW_SIZE(N) 4

/* Number of bytes of storage in the program's representation for register N. */
#define REGISTER_VIRTUAL_SIZE(N) 4

/* Largest value REGISTER_RAW_SIZE can have.  */
#define MAX_REGISTER_RAW_SIZE 4

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */
#define MAX_REGISTER_VIRTUAL_SIZE 4

/* Return the GDB type object for the "standard" data type
   of data in register N.  */
#define REGISTER_VIRTUAL_TYPE(N) (builtin_type_int)


/* Macros for understanding function return values... */

/* Does the specified function use the "struct returning" convention
   or the "value returning" convention?  The "value returning" convention
   almost invariably returns the entire value in registers.  The
   "struct returning" convention often returns the entire value in
   memory, and passes a pointer (out of or into the function) saying
   where the value (is or should go).

   Since this sometimes depends on whether it was compiled with GCC,
   this is also an argument.  This is used in call_function to build a
   stack, and in value_being_returned to print return values.

   On arc, a structure is always retunred with pointer in r0. */

#define USE_STRUCT_CONVENTION(gcc_p, type) 1

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  This is only called if USE_STRUCT_CONVENTION for this
   type is 0.
*/
#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
	memcpy(VALBUF, REGBUF+REGISTER_BYTE(R0_REGNUM), TYPE_LENGTH (TYPE))

/* If USE_STRUCT_CONVENTION produces a 1, 
   extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one). */
#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
   (error("Don't know where large structure is returned on arc"), 0)

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format, for "value returning" functions.
   For 'return' command:  not (yet) implemented for arc.  */
#define STORE_RETURN_VALUE(TYPE,VALBUF) \
    error ("Returning values from functions is not implemented in arc gdb")

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */
#define STORE_STRUCT_RETURN(ADDR, SP) \
    error ("Returning values from functions is not implemented in arc gdb")


/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* We cache information about saved registers in the frame structure,
   to save us from having to re-scan function prologues every time
   a register in a non-current frame is accessed.  */

#define EXTRA_FRAME_INFO \
	struct frame_saved_regs *fsr; \
	CORE_ADDR arg_pointer;

/* Zero the frame_saved_regs pointer when the frame is initialized,
   so that FRAME_FIND_SAVED_REGS () will know to allocate and
   initialize a frame_saved_regs struct the first time it is called.
   Set the arg_pointer to -1, which is not valid; 0 and other values
   indicate real, cached values.  */

#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) \
	((fi)->fsr = 0, (fi)->arg_pointer = -1)

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer.
   However, if FRAME_CHAIN_VALID returns zero,
   it means the given frame is the outermost one and has no caller.  */
/* On the arc, we get the chain pointer by reading the PFP saved
   on the stack. */
/* The PFP and RPC is in fp and fp+4.  */

#define FRAME_CHAIN(thisframe) \
  (read_memory_integer (FRAME_FP (thisframe), 4))

/* FRAME_CHAIN_VALID returns zero if the given frame is the outermost one
   and has no caller.  */
#define FRAME_CHAIN_VALID(chain, thisframe) ((chain) != 0)

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */

#define FRAMELESS_FUNCTION_INVOCATION(FI, FRAMELESS) \
  do { \
    if ((FI)->signal_handler_caller) \
      (FRAMELESS) = 0; \
    else \
      (FRAMELESS) = frameless_look_for_prologue (FI); \
  } while (0)

/* Where is the PC for a specific frame.
   A leaf function may never save blink, so we have to check for that here.  */

#define FRAME_SAVED_PC(frame) (arc_frame_saved_pc (frame))
struct frame_info; /* in case frame.h not included yet */
CORE_ADDR arc_frame_saved_pc PARAMS ((struct frame_info *));

/* If the argument is on the stack, it will be here.
   We cache this value in the frame info if we've already looked it up.  */
/* ??? Is the arg_pointer check necessary?  */

#define FRAME_ARGS_ADDRESS(fi) \
  (((fi)->arg_pointer != -1) ? (fi)->arg_pointer : (fi)->frame)

/* This is the same except it should return 0 when
   it does not really know where the args are, rather than guessing.
   This value is not cached since it is only used infrequently.  */

#define FRAME_LOCALS_ADDRESS(fi)	((fi)->frame)

/* Set NUMARGS to the number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

#define FRAME_NUM_ARGS(numargs, fi)	(numargs = -1)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

/* Produce the positions of the saved registers in a stack frame.  */

#define FRAME_FIND_SAVED_REGS(frame_info_addr, sr) \
	frame_find_saved_regs (frame_info_addr, &sr)
extern void frame_find_saved_regs();		/* See arc-tdep.c */


/* Things needed for making calls to functions in the inferior process */
#define PUSH_DUMMY_FRAME \
	push_dummy_frame ()

/* Discard from the stack the innermost frame, restoring all registers.  */
#define POP_FRAME \
	pop_frame ()

/* This sequence of words is the instructions  bl xxxx, flag 1 */
#define CALL_DUMMY { 0x28000000, 0x1fbe8001 }   
#define CALL_DUMMY_LENGTH 8

/* Start execution at beginning of dummy */
#define CALL_DUMMY_START_OFFSET 0 

/* Insert the specified number of args and function address
   into a call sequence of the above form stored at 'dummyname'.*/  
#define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, type, gcc_p) \
{ \
        int from, to, delta, loc; \
        loc = (int)(read_register (SP_REGNUM) - CALL_DUMMY_LENGTH); \
        from = loc + 4; \
        to = (int)(fun); \
        delta = (to - from) >> 2; \
        *((char *)(dummyname) + 1) = (delta & 0x1); \
        *((char *)(dummyname) + 2) = ((delta >> 1) & 0xff); \
        *((char *)(dummyname) + 3) = ((delta >> 9) & 0xff); \
        *((char *)(dummyname) + 4) = ((delta >> 17) & 0x7); \
}

