/* Parameters for execution on a TI TMS320C80 (MVP) processor.
   Copyright 1997
   Free Software Foundation, Inc. 

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

#ifndef TM_TIC80_H
#define TM_TIC80_H

/* Forward declare structs used in prototypes */
struct frame_info;
struct type;
struct value;
struct symbol;
struct frame_saved_regs;

#define TARGET_BYTE_ORDER LITTLE_ENDIAN

#define NUM_REGS 38

#define REGISTER_NAMES \
{ "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", \
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", \
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", \
  "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31", \
  "pc", "npc", \
  "a0", "a1", "a2", "a3", \
}

/* Various dedicated register numbers
   FIXME: Shadow updates in sim/tic80/sim-calls.c */

#define SP_REGNUM 1		/* Contains address of top of stack */
#define ARG0_REGNUM 2		/* Contains argument 1 (r3 has high word) */
#define RET_REGNUM  2		/* Contains function return value */
#define ARGLAST_REGNUM 12	/* Contains argument 6 (r13 has high word) */
#define FP_REGNUM 30		/* Contains address of executing stack frame */
#define LR_REGNUM 31		/* Contains address of caller (link register) */
#define PC_REGNUM 32		/* Contains program counter (FIXME?) */
#define NPC_REGNUM 33		/* Contains the next program counter (FIXME?) */
#define A0_REGNUM 34		/* Accumulator register 0 */
#define A3_REGNUM 37		/* Accumulator register 1 */

#define R0_REGNUM 0		/* General Purpose Register 0 - for sim */
#define Rn_REGNUM 31		/* Last General Purpose Register - for sim */
#define An_REGNUM A3_REGNUM	/* Last Accumulator register - for sim */

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */

#define REGISTER_BYTES (((NUM_REGS - 4) * 4) + (4 * 8))

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N) \
   (((N) >= A0_REGNUM) ? (((N) - A0_REGNUM) * 8 + A0_REGNUM * 4) : ((N) * 4))

/* Most registers are 4 bytes */

#define REGISTER_SIZE 4

/* Some registers are 8 bytes.  */

#define REGISTER_RAW_SIZE(N) \
     (((N) >= A0_REGNUM) ? 8 : 4)

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE (8)

/* All regs are 4 bytes.  */

#define REGISTER_VIRTUAL_SIZE(N) (REGISTER_RAW_SIZE(N))

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE (MAX_REGISTER_RAW_SIZE)

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) /* FIXME? */ \
	(((N) >= A0_REGNUM) ? builtin_type_float : builtin_type_int)

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Stack grows downward.  */

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/* Sequence of bytes for breakpoint instruction.
   This is padded out to the size of a machine word. */

#define BREAKPOINT 		{0x49, 0x80, 0x00, 0x00}	/* FIXME! */

/* Amount PC must be decremented by after a breakpoint.
   This is often the number of bytes in BREAKPOINT
   but not always.  */

#define DECR_PC_AFTER_BREAK	0	/* FIXME! */

/* Discard from the stack the innermost frame, restoring all registers.  */

#define POP_FRAME tic80_pop_frame(get_current_frame ())
extern struct frame_info *tic80_pop_frame PARAMS ((struct frame_info * frame));

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

/* Set VAL to the number of args passed to frame described by FI.
   Can set VAL to -1, meaning no way to tell.  */
/* We can't tell how many args there are */

#define FRAME_NUM_ARGS(fi) (-1)

#define FRAME_ARGS_SKIP 0
#define FRAME_ARGS_ADDRESS(fi)   (fi)->frame
#define FRAME_LOCALS_ADDRESS(fi) (fi)->frame

/* Define other aspects of the stack frame. 
   We keep the offsets of all saved registers, 'cause we need 'em a lot!
   We also keep the current size of the stack frame, and the offset of
   the frame pointer from the stack pointer (for frameless functions, and
   when we're still in the prologue of a function with a frame) */

#define EXTRA_FRAME_INFO  	\
  struct frame_saved_regs fsr;	\
  int framesize;		\
  int frameoffset;		\
  int framereg;

extern void tic80_init_extra_frame_info PARAMS ((struct frame_info * fi));
#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) tic80_init_extra_frame_info (fi)
#define INIT_FRAME_PC		/* Not necessary */

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs)	    \
   tic80_frame_find_saved_regs(frame_info, &(frame_saved_regs))
extern void tic80_frame_find_saved_regs PARAMS ((struct frame_info *, struct frame_saved_regs *));

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

#define SKIP_PROLOGUE(pc) (tic80_skip_prologue (pc))
extern CORE_ADDR tic80_skip_prologue PARAMS ((CORE_ADDR pc));

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#define SAVED_PC_AFTER_CALL(frame) read_register (LR_REGNUM)

/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer. */

#define FRAME_CHAIN(thisframe) (CORE_ADDR) tic80_frame_chain (thisframe)
extern CORE_ADDR tic80_frame_chain PARAMS ((struct frame_info *));

#define FRAME_SAVED_PC(FRAME)	tic80_frame_saved_pc (FRAME)
extern CORE_ADDR tic80_frame_saved_pc PARAMS ((struct frame_info *));

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. 

   We store structs through a pointer passed in R2 */

#define STORE_STRUCT_RETURN(STRUCT_ADDR, SP)	\
	write_register (ARG0_REGNUM, STRUCT_ADDR)

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
  memcpy ((VALBUF), \
	  (char *)(REGBUF) + REGISTER_BYTE (RET_REGNUM) + \
	  ((TYPE_LENGTH (TYPE) > 4 ? 8 : 4) - TYPE_LENGTH (TYPE)), \
	  TYPE_LENGTH (TYPE))

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  write_register_bytes(REGISTER_BYTE (RET_REGNUM) + \
		       ((TYPE_LENGTH (TYPE) > 4 ? 8:4) - TYPE_LENGTH (TYPE)),\
		       (VALBUF), TYPE_LENGTH (TYPE));



/* PUSH_ARGUMENTS */
extern CORE_ADDR tic80_push_arguments PARAMS ((int nargs,
					       struct value ** args,
					       CORE_ADDR sp,
					       unsigned char struct_return,
					       CORE_ADDR struct_addr));

#define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
  (tic80_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR))

/* PUSH_RETURN_ADDRESS */
extern CORE_ADDR tic80_push_return_address PARAMS ((CORE_ADDR, CORE_ADDR));
#define PUSH_RETURN_ADDRESS(PC, SP)	tic80_push_return_address (PC, SP)

/* override the standard get_saved_register function with 
   one that takes account of generic CALL_DUMMY frames */
#define GET_SAVED_REGISTER(raw_buffer, optimized, addrp, frame, regnum, lval) \
      generic_get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval)

#define USE_GENERIC_DUMMY_FRAMES 1
#define CALL_DUMMY                   {0}
#define CALL_DUMMY_LENGTH            (0)
#define CALL_DUMMY_START_OFFSET      (0)
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)
#define FIX_CALL_DUMMY(DUMMY1, STARTADDR, FUNADDR, NARGS, ARGS, TYPE, GCCP)
#define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
#define CALL_DUMMY_ADDRESS()         entry_point_address ()

/* generic dummy frame stuff */

#define PUSH_DUMMY_FRAME             generic_push_dummy_frame ()
#define PC_IN_CALL_DUMMY(PC, SP, FP) generic_pc_in_call_dummy (PC, SP, FP)

#endif /* TM_TIC80_H */
