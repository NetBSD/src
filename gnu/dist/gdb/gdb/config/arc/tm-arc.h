// OBSOLETE /* Parameters for target machine ARC, for GDB, the GNU debugger.
// OBSOLETE    Copyright 1995, 1996, 1998, 1999, 2000 Free Software Foundation, Inc.
// OBSOLETE    Contributed by Cygnus Support.
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
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE /* Used by arc-tdep.c to set the default cpu type.  */
// OBSOLETE #define DEFAULT_ARC_CPU_TYPE "base"
// OBSOLETE 
// OBSOLETE /* Offset from address of function to start of its code.
// OBSOLETE    Zero on most machines.  */
// OBSOLETE #define FUNCTION_START_OFFSET 0
// OBSOLETE 
// OBSOLETE /* Advance PC across any function entry prologue instructions
// OBSOLETE    to reach some "real" code.  */
// OBSOLETE 
// OBSOLETE #define SKIP_PROLOGUE(pc) (arc_skip_prologue (pc, 0))
// OBSOLETE extern CORE_ADDR arc_skip_prologue (CORE_ADDR, int);
// OBSOLETE 
// OBSOLETE #define PROLOGUE_FRAMELESS_P(pc) arc_prologue_frameless_p(pc)
// OBSOLETE extern int arc_prologue_frameless_p (CORE_ADDR);
// OBSOLETE 
// OBSOLETE /* Sequence of bytes for breakpoint instruction.
// OBSOLETE    ??? The current value is "sr -1,[-1]" and is for the simulator only.
// OBSOLETE    The simulator watches for this and does the right thing.
// OBSOLETE    The hardware version will have to associate with each breakpoint
// OBSOLETE    the sequence "flag 1; nop; nop; nop".  IE: The breakpoint insn will not
// OBSOLETE    be a fixed set of bits but instead will be a branch to a semi-random
// OBSOLETE    address.  Presumably this will be cleaned up for "second silicon".  */
// OBSOLETE #define BIG_BREAKPOINT { 0x12, 0x1f, 0xff, 0xff }
// OBSOLETE #define LITTLE_BREAKPOINT { 0xff, 0xff, 0x1f, 0x12 }
// OBSOLETE 
// OBSOLETE /* Given the exposed pipeline, there isn't any one correct value.
// OBSOLETE    However, this value must be 4.  GDB can't handle any other value (other than
// OBSOLETE    zero).  See for example infrun.c:
// OBSOLETE    "prev_pc != stop_pc - DECR_PC_AFTER_BREAK"  */
// OBSOLETE /* FIXME */
// OBSOLETE #define DECR_PC_AFTER_BREAK 8
// OBSOLETE 
// OBSOLETE /* We don't have a reliable single step facility.
// OBSOLETE    ??? We do have a cycle single step facility, but that won't work.  */
// OBSOLETE #define SOFTWARE_SINGLE_STEP_P() 1
// OBSOLETE extern void arc_software_single_step (enum target_signal, int);
// OBSOLETE #define SOFTWARE_SINGLE_STEP(sig,bp_p) arc_software_single_step (sig, bp_p)
// OBSOLETE 
// OBSOLETE /* FIXME: Need to set STEP_SKIPS_DELAY.  */
// OBSOLETE 
// OBSOLETE /* Given a pc value as defined by the hardware, return the real address.
// OBSOLETE    Remember that on the ARC blink contains that status register which
// OBSOLETE    includes PC + flags (so we have to mask out the flags).  */
// OBSOLETE #define ARC_PC_TO_REAL_ADDRESS(pc) (((pc) & 0xffffff) << 2)
// OBSOLETE 
// OBSOLETE /* Immediately after a function call, return the saved pc.
// OBSOLETE    Can't always go through the frames for this because on some machines
// OBSOLETE    the new frame is not set up until the new function
// OBSOLETE    executes some instructions.  */
// OBSOLETE 
// OBSOLETE #define SAVED_PC_AFTER_CALL(frame) \
// OBSOLETE   (ARC_PC_TO_REAL_ADDRESS (read_register (BLINK_REGNUM)))
// OBSOLETE 
// OBSOLETE /* Stack grows upward */
// OBSOLETE 
// OBSOLETE #define INNER_THAN(lhs,rhs) ((lhs) < (rhs))
// OBSOLETE 
// OBSOLETE /* Say how long (ordinary) registers are.  This is a piece of bogosity
// OBSOLETE    used in push_word and a few other places; REGISTER_RAW_SIZE is the
// OBSOLETE    real way to know how big a register is.  */
// OBSOLETE #define REGISTER_SIZE 4
// OBSOLETE 
// OBSOLETE /* Number of machine registers */
// OBSOLETE #define NUM_REGS 92
// OBSOLETE 
// OBSOLETE /* Initializer for an array of names of registers.
// OBSOLETE    There should be NUM_REGS strings in this initializer.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_NAMES \
// OBSOLETE { \
// OBSOLETE     /*  0 */ "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7", \
// OBSOLETE     /*  8 */ "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15", \
// OBSOLETE     /* 16 */ "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", \
// OBSOLETE     /* 24 */ "r24", "r25", "r26", "fp", "sp", "ilink1", "ilink2", "blink", \
// OBSOLETE     /* 32 */ "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39", \
// OBSOLETE     /* 40 */ "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47", \
// OBSOLETE     /* 48 */ "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55", \
// OBSOLETE     /* 56 */ "r56", "mlo", "mmid", "mhi", "lp_count", \
// OBSOLETE     /* 61 */ "status", "sema", "lp_start", "lp_end", "identity", "debug", \
// OBSOLETE     /* 67 */ "aux10", "aux11", "aux12", "aux13", "aux14", \
// OBSOLETE     /* 72 */ "aux15", "aux16", "aux17", "aux18", "aux19", \
// OBSOLETE     /* 77 */ "aux1a", "aux1b", "aux1c", "aux1d", "aux1e", \
// OBSOLETE     /* 82 */ "aux1f", "aux20", "aux21", "aux22", \
// OBSOLETE     /* 86 */ "aux30", "aux31", "aux32", "aux33", "aux40", \
// OBSOLETE     /* 91 */ "pc" \
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Register numbers of various important registers (used to index
// OBSOLETE    into arrays of register names and register values).  */
// OBSOLETE 
// OBSOLETE #define R0_REGNUM   0		/* First local register           */
// OBSOLETE #define R59_REGNUM 59		/* Last local register            */
// OBSOLETE #define FP_REGNUM  27		/* Contains address of executing stack frame */
// OBSOLETE #define SP_REGNUM  28		/* stack pointer */
// OBSOLETE #define BLINK_REGNUM 31		/* link register */
// OBSOLETE #define	STA_REGNUM 61		/* processor status word */
// OBSOLETE #define PC_REGNUM  91		/* instruction pointer */
// OBSOLETE #define AUX_BEG_REGNUM  61	/* aux reg begins */
// OBSOLETE #define AUX_END_REGNUM  90	/* aux reg ends, pc not real aux reg */
// OBSOLETE 
// OBSOLETE /* Fake registers used to mark immediate data.  */
// OBSOLETE #define SHIMM_FLAG_REGNUM 61
// OBSOLETE #define LIMM_REGNUM 62
// OBSOLETE #define SHIMM_REGNUM 63
// OBSOLETE 
// OBSOLETE #define AUX_REG_MAP \
// OBSOLETE { \
// OBSOLETE    {  0,  1,  2,  3,  4,  5, \
// OBSOLETE      16, -1, -1, -1, -1, \
// OBSOLETE      -1, -1, -1, -1, -1, \
// OBSOLETE      -1, -1, -1, -1, 30, \
// OBSOLETE      -1, 32, 33, -1, \
// OBSOLETE       48, 49, 50, 51, 64, \
// OBSOLETE       0 \
// OBSOLETE     }, \
// OBSOLETE    {  0,  1,  2,  3,  4,  5, \
// OBSOLETE      16, -1, -1, -1, -1, \
// OBSOLETE      -1, -1, -1, -1, -1, \
// OBSOLETE      -1, -1, -1, -1, 30, \
// OBSOLETE      31, 32, 33, -1, \
// OBSOLETE      -1, -1, -1, -1, -1, \
// OBSOLETE       0 \
// OBSOLETE     }, \
// OBSOLETE    {  0,  1,  2,  3,  4,  5, \
// OBSOLETE       16, 17, 18, 19, 20, \
// OBSOLETE       21, 22, 23, 24, 25, \
// OBSOLETE       26, 27, 28, 29, 30, \
// OBSOLETE       31, 32, 33, 34, \
// OBSOLETE      -1, -1, -1, -1, -1, \
// OBSOLETE       0 \
// OBSOLETE     } \
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #define PFP_REGNUM R0_REGNUM	/* Previous frame pointer */
// OBSOLETE 
// OBSOLETE /* Total amount of space needed to store our copies of the machine's
// OBSOLETE    register state, the array `registers'.  */
// OBSOLETE #define REGISTER_BYTES (NUM_REGS * 4)
// OBSOLETE 
// OBSOLETE /* Index within `registers' of the first byte of the space for register N.  */
// OBSOLETE #define REGISTER_BYTE(N) (4*(N))
// OBSOLETE 
// OBSOLETE /* Number of bytes of storage in the actual machine representation
// OBSOLETE    for register N. */
// OBSOLETE #define REGISTER_RAW_SIZE(N) 4
// OBSOLETE 
// OBSOLETE /* Number of bytes of storage in the program's representation for register N. */
// OBSOLETE #define REGISTER_VIRTUAL_SIZE(N) 4
// OBSOLETE 
// OBSOLETE /* Largest value REGISTER_RAW_SIZE can have.  */
// OBSOLETE #define MAX_REGISTER_RAW_SIZE 4
// OBSOLETE 
// OBSOLETE /* Largest value REGISTER_VIRTUAL_SIZE can have.  */
// OBSOLETE #define MAX_REGISTER_VIRTUAL_SIZE 4
// OBSOLETE 
// OBSOLETE /* Return the GDB type object for the "standard" data type
// OBSOLETE    of data in register N.  */
// OBSOLETE #define REGISTER_VIRTUAL_TYPE(N) (builtin_type_int)
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Macros for understanding function return values... */
// OBSOLETE 
// OBSOLETE /* Does the specified function use the "struct returning" convention
// OBSOLETE    or the "value returning" convention?  The "value returning" convention
// OBSOLETE    almost invariably returns the entire value in registers.  The
// OBSOLETE    "struct returning" convention often returns the entire value in
// OBSOLETE    memory, and passes a pointer (out of or into the function) saying
// OBSOLETE    where the value (is or should go).
// OBSOLETE 
// OBSOLETE    Since this sometimes depends on whether it was compiled with GCC,
// OBSOLETE    this is also an argument.  This is used in call_function to build a
// OBSOLETE    stack, and in value_being_returned to print return values.
// OBSOLETE 
// OBSOLETE    On arc, a structure is always retunred with pointer in r0. */
// OBSOLETE 
// OBSOLETE #define USE_STRUCT_CONVENTION(gcc_p, type) 1
// OBSOLETE 
// OBSOLETE /* Extract from an array REGBUF containing the (raw) register state
// OBSOLETE    a function return value of type TYPE, and copy that, in virtual format,
// OBSOLETE    into VALBUF.  This is only called if USE_STRUCT_CONVENTION for this
// OBSOLETE    type is 0.
// OBSOLETE  */
// OBSOLETE #define DEPRECATED_EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
// OBSOLETE 	memcpy(VALBUF, REGBUF+REGISTER_BYTE(R0_REGNUM), TYPE_LENGTH (TYPE))
// OBSOLETE 
// OBSOLETE /* If USE_STRUCT_CONVENTION produces a 1, 
// OBSOLETE    extract from an array REGBUF containing the (raw) register state
// OBSOLETE    the address in which a function should return its structure value,
// OBSOLETE    as a CORE_ADDR (or an expression that can be used as one). */
// OBSOLETE #define DEPRECATED_EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
// OBSOLETE    (error("Don't know where large structure is returned on arc"), 0)
// OBSOLETE 
// OBSOLETE /* Write into appropriate registers a function return value
// OBSOLETE    of type TYPE, given in virtual format, for "value returning" functions.
// OBSOLETE    For 'return' command:  not (yet) implemented for arc.  */
// OBSOLETE #define STORE_RETURN_VALUE(TYPE,VALBUF) \
// OBSOLETE     error ("Returning values from functions is not implemented in arc gdb")
// OBSOLETE 
// OBSOLETE /* Store the address of the place in which to copy the structure the
// OBSOLETE    subroutine will return.  This is called from call_function. */
// OBSOLETE #define STORE_STRUCT_RETURN(ADDR, SP) \
// OBSOLETE     error ("Returning values from functions is not implemented in arc gdb")
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Describe the pointer in each stack frame to the previous stack frame
// OBSOLETE    (its caller).  */
// OBSOLETE 
// OBSOLETE /* We cache information about saved registers in the frame structure,
// OBSOLETE    to save us from having to re-scan function prologues every time
// OBSOLETE    a register in a non-current frame is accessed.  */
// OBSOLETE 
// OBSOLETE #define EXTRA_FRAME_INFO \
// OBSOLETE 	struct frame_saved_regs *fsr; \
// OBSOLETE 	CORE_ADDR arg_pointer;
// OBSOLETE 
// OBSOLETE /* Zero the frame_saved_regs pointer when the frame is initialized,
// OBSOLETE    so that FRAME_FIND_SAVED_REGS () will know to allocate and
// OBSOLETE    initialize a frame_saved_regs struct the first time it is called.
// OBSOLETE    Set the arg_pointer to -1, which is not valid; 0 and other values
// OBSOLETE    indicate real, cached values.  */
// OBSOLETE 
// OBSOLETE #define INIT_EXTRA_FRAME_INFO(fromleaf, fi) \
// OBSOLETE 	((fi)->fsr = 0, (fi)->arg_pointer = -1)
// OBSOLETE 
// OBSOLETE /* FRAME_CHAIN takes a frame's nominal address
// OBSOLETE    and produces the frame's chain-pointer.
// OBSOLETE    However, if FRAME_CHAIN_VALID returns zero,
// OBSOLETE    it means the given frame is the outermost one and has no caller.  */
// OBSOLETE /* On the arc, we get the chain pointer by reading the PFP saved
// OBSOLETE    on the stack. */
// OBSOLETE /* The PFP and RPC is in fp and fp+4.  */
// OBSOLETE 
// OBSOLETE #define FRAME_CHAIN(thisframe) \
// OBSOLETE   (read_memory_integer (FRAME_FP (thisframe), 4))
// OBSOLETE 
// OBSOLETE /* FRAME_CHAIN_VALID returns zero if the given frame is the outermost one
// OBSOLETE    and has no caller.  */
// OBSOLETE #define FRAME_CHAIN_VALID(chain, thisframe) nonnull_frame_chain_valid (chain, thisframe)
// OBSOLETE 
// OBSOLETE /* An expression that tells us whether the function invocation represented
// OBSOLETE    by FI does not have a frame on the stack associated with it. */
// OBSOLETE 
// OBSOLETE #define FRAMELESS_FUNCTION_INVOCATION(FI) \
// OBSOLETE      (((FI)->signal_handler_caller) ? 0 : frameless_look_for_prologue (FI))
// OBSOLETE 
// OBSOLETE /* Where is the PC for a specific frame.
// OBSOLETE    A leaf function may never save blink, so we have to check for that here.  */
// OBSOLETE 
// OBSOLETE #define FRAME_SAVED_PC(frame) (arc_frame_saved_pc (frame))
// OBSOLETE struct frame_info;		/* in case frame.h not included yet */
// OBSOLETE CORE_ADDR arc_frame_saved_pc (struct frame_info *);
// OBSOLETE 
// OBSOLETE /* If the argument is on the stack, it will be here.
// OBSOLETE    We cache this value in the frame info if we've already looked it up.  */
// OBSOLETE /* ??? Is the arg_pointer check necessary?  */
// OBSOLETE 
// OBSOLETE #define FRAME_ARGS_ADDRESS(fi) \
// OBSOLETE   (((fi)->arg_pointer != -1) ? (fi)->arg_pointer : (fi)->frame)
// OBSOLETE 
// OBSOLETE /* This is the same except it should return 0 when
// OBSOLETE    it does not really know where the args are, rather than guessing.
// OBSOLETE    This value is not cached since it is only used infrequently.  */
// OBSOLETE 
// OBSOLETE #define FRAME_LOCALS_ADDRESS(fi)	((fi)->frame)
// OBSOLETE 
// OBSOLETE /* Set NUMARGS to the number of args passed to a frame.
// OBSOLETE    Can return -1, meaning no way to tell.  */
// OBSOLETE 
// OBSOLETE #define FRAME_NUM_ARGS(fi)	(-1)
// OBSOLETE 
// OBSOLETE /* Return number of bytes at start of arglist that are not really args.  */
// OBSOLETE 
// OBSOLETE #define FRAME_ARGS_SKIP 0
// OBSOLETE 
// OBSOLETE /* Produce the positions of the saved registers in a stack frame.  */
// OBSOLETE 
// OBSOLETE #define FRAME_FIND_SAVED_REGS(frame_info_addr, sr) \
// OBSOLETE 	frame_find_saved_regs (frame_info_addr, &sr)
// OBSOLETE extern void frame_find_saved_regs ();	/* See arc-tdep.c */
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Things needed for making calls to functions in the inferior process */
// OBSOLETE void arc_push_dummy_frame (void);
// OBSOLETE #define PUSH_DUMMY_FRAME \
// OBSOLETE 	arc_push_dummy_frame ()
// OBSOLETE 
// OBSOLETE /* Discard from the stack the innermost frame, restoring all registers.  */
// OBSOLETE void arc_pop_frame (void);
// OBSOLETE #define POP_FRAME \
// OBSOLETE 	arc_pop_frame ()
// OBSOLETE 
// OBSOLETE /* This sequence of words is the instructions  bl xxxx, flag 1 */
// OBSOLETE #define CALL_DUMMY { 0x28000000, 0x1fbe8001 }
// OBSOLETE #define CALL_DUMMY_LENGTH 8
// OBSOLETE 
// OBSOLETE /* Start execution at beginning of dummy */
// OBSOLETE #define CALL_DUMMY_START_OFFSET 0
// OBSOLETE 
// OBSOLETE /* Insert the specified number of args and function address
// OBSOLETE    into a call sequence of the above form stored at 'dummyname'. */
// OBSOLETE #define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, type, gcc_p) \
// OBSOLETE { \
// OBSOLETE         int from, to, delta, loc; \
// OBSOLETE         loc = (int)(read_register (SP_REGNUM) - CALL_DUMMY_LENGTH); \
// OBSOLETE         from = loc + 4; \
// OBSOLETE         to = (int)(fun); \
// OBSOLETE         delta = (to - from) >> 2; \
// OBSOLETE         *((char *)(dummyname) + 1) = (delta & 0x1); \
// OBSOLETE         *((char *)(dummyname) + 2) = ((delta >> 1) & 0xff); \
// OBSOLETE         *((char *)(dummyname) + 3) = ((delta >> 9) & 0xff); \
// OBSOLETE         *((char *)(dummyname) + 4) = ((delta >> 17) & 0x7); \
// OBSOLETE }
