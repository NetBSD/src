/* Parameters for execution on a H8/300 series machine.
   Copyright 1992, 1993 Free Software Foundation, Inc.

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

/* Contributed by Steve Chamberlain sac@cygnus.com */

struct frame_info;
struct frame_saved_regs;
struct value;
struct type;

/* 1 if debugging H8/300H application */
extern int h8300hmode;
extern int h8300smode;

/* Number of bytes in a word */

#define BINWORD (h8300hmode?4:2)

#define EXTRA_FRAME_INFO 	\
	struct frame_saved_regs *fsr;	\
	CORE_ADDR from_pc; \
	CORE_ADDR args_pointer;\
        CORE_ADDR locals_pointer ;

/* Zero the frame_saved_regs pointer when the frame is initialized,
   so that FRAME_FIND_SAVED_REGS () will know to allocate and
   initialize a frame_saved_regs struct the first time it is called.
   Set the arg_pointer to -1, which is not valid; 0 and other values
   indicate real, cached values.  */

#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) \
	h8300_init_extra_frame_info (fromleaf, fi)

extern void h8300_init_extra_frame_info ();

#define IEEE_FLOAT
/* Define the bit, byte, and word ordering of the machine.  */
#define TARGET_BYTE_ORDER BIG_ENDIAN
#undef TARGET_INT_BIT
#define TARGET_INT_BIT  16
#undef TARGET_LONG_BIT
#define TARGET_LONG_BIT  32
#undef TARGET_PTR_BIT
#define TARGET_PTR_BIT  (h8300hmode ? 32:16)

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

#define SKIP_PROLOGUE(ip)   (h8300_skip_prologue(ip))
extern CORE_ADDR h8300_skip_prologue ();

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#define SAVED_PC_AFTER_CALL(frame) \
  read_memory_unsigned_integer (read_register (SP_REGNUM), BINWORD)

/* Stack grows downward.  */

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/*#define BREAKPOINT {0x7A, 0xFF} */
#define BREAKPOINT {0x01, 0x80}	/* Sleep */
#define REMOTE_BREAKPOINT { 0x57, 0x30}		/* trapa #3 */
/* If your kernel resets the pc after the trap happens you may need to
   define this before including this file.    */

#define DECR_PC_AFTER_BREAK 0

/* Say how long registers are.  */

#define REGISTER_SIZE 4

#define NUM_REGS 13

#define REGISTER_BYTES (NUM_REGS * 4)

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N)  ((N) * 4)

/* Number of bytes of storage in the actual machine representation
   for register N.  On the H8/300, all regs are 2 bytes.  */

#define REGISTER_RAW_SIZE(N) (h8300hmode ? 4 : 2)

/* Number of bytes of storage in the program's representation
   for register N.  */

#define REGISTER_VIRTUAL_SIZE(N) (h8300hmode ? 4 : 2)

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 4

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 4

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
(h8300hmode ? builtin_type_unsigned_long : builtin_type_unsigned_short)

/* Initializer for an array of names of registers.
   Entries beyond the first NUM_REGS are ignored.  */

#define REGISTER_NAMES \
  {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "sp", "ccr","pc","cycles","tick","inst"}

/* An array of names of registers. */

extern char **h8300_register_names;
#define REGISTER_NAME(i) h8300_register_names[i]

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define ARG0_REGNUM    0	/* first reg in which an arg may be passed */
#define ARGLAST_REGNUM 2	/* last  reg in which an arg may be passed */
#define FP_REGNUM      6	/* Contain saddress of executing stack frame */
#define SP_REGNUM      7	/* Contains address of top of stack */
#define CCR_REGNUM     8	/* Contains processor status */
#define PC_REGNUM      9	/* Contains program counter */

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

/* FIXME: Won't work with both h8/300's.  */

extern void h8300_extract_return_value PARAMS ((struct type *, char *, char *));
#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
    h8300_extract_return_value (TYPE, (char *)(REGBUF), (char *)(VALBUF))

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  Assumes floats are passed
   in d0/d1.  */
/* FIXME: Won't work with both h8/300's.  */

extern void h8300_store_return_value PARAMS ((struct type *, char *));
#define STORE_RETURN_VALUE(TYPE,VALBUF) \
    h8300_store_return_value(TYPE, (char *) (VALBUF))

/* struct passing and returning stuff */
#define STORE_STRUCT_RETURN(STRUCT_ADDR, SP)	\
	write_register (0, STRUCT_ADDR)

#define USE_STRUCT_CONVENTION(gcc_p, type)	(1)

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
     extract_address (REGBUF + REGISTER_BYTE (0), \
		      REGISTER_RAW_SIZE (0))

/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer.

   However, if FRAME_CHAIN_VALID returns zero,
   it means the given frame is the outermost one and has no caller.  */

#define FRAME_CHAIN(FRAME) h8300_frame_chain(FRAME)
CORE_ADDR h8300_frame_chain PARAMS ((struct frame_info *));

/* In the case of the H8/300, the frame's nominal address
   is the address of a 2-byte word containing the calling frame's address.  */

/* Use the alternate method of avoiding running up off the end of
   the frame chain or following frames back into the startup code.
   See the comments in objfile.h */

#define FRAME_CHAIN_VALID(fp,fi) func_frame_chain_valid (fp, fi)

/* Define other aspects of the stack frame.  */

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */
#define FRAMELESS_FUNCTION_INVOCATION(FI) \
  (frameless_look_for_prologue (FI))

/* Any function with a frame looks like this
   SECOND ARG
   FIRST ARG
   RET PC
   SAVED R2
   SAVED R3
   SAVED FP   <-FP POINTS HERE
   LOCALS0
   LOCALS1    <-SP POINTS HERE
 */

#define FRAME_SAVED_PC(FRAME) h8300_frame_saved_pc(FRAME)

#define FRAME_ARGS_ADDRESS(fi) frame_args_address(fi)

#define FRAME_LOCALS_ADDRESS(fi) frame_locals_address(fi);

/* Set VAL to the number of args passed to frame described by FI.
   Can set VAL to -1, meaning no way to tell.  */

/* We can't tell how many args there are
   now that the C compiler delays popping them.  */

#define FRAME_NUM_ARGS(fi) (-1)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs)	    \
   h8300_frame_find_saved_regs(frame_info, &(frame_saved_regs))


typedef unsigned short INSN_WORD;


#define	PRINT_REGISTER_HOOK(regno) print_register_hook(regno)

#define GDB_TARGET_IS_H8300

#define NUM_REALREGS 10
#define NOP { 0x01, 0x80}	/* A sleep insn */

#define BELIEVE_PCC_PROMOTION 1

/*
 * CALL_DUMMY stuff:
 */

#define USE_GENERIC_DUMMY_FRAMES 1
#define CALL_DUMMY			{0}
#define CALL_DUMMY_LENGTH		(0)
#define CALL_DUMMY_ADDRESS()		entry_point_address ()
#define CALL_DUMMY_LOCATION		AT_ENTRY_POINT
#define CALL_DUMMY_START_OFFSET		(0)
#define CALL_DUMMY_BREAKPOINT_OFFSET	(0)

extern CORE_ADDR h8300_push_arguments PARAMS ((int nargs,
					       struct value ** args,
					       CORE_ADDR sp,
					       unsigned char struct_return,
					       CORE_ADDR struct_addr));
extern CORE_ADDR h8300_push_return_address PARAMS ((CORE_ADDR, CORE_ADDR));

#define PC_IN_CALL_DUMMY(PC, SP, FP)	generic_pc_in_call_dummy (PC, SP, FP)
#define FIX_CALL_DUMMY(DUMMY, START_SP, FUNADDR, NARGS, ARGS, TYPE, GCCP)
#define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
  (h8300_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR))
/* Push an empty stack frame, to record the current PC, etc.  */
#define PUSH_DUMMY_FRAME	generic_push_dummy_frame ()
/* Discard from the stack the innermost frame, restoring all registers.  */
#define POP_FRAME		h8300_pop_frame ()
#define PUSH_RETURN_ADDRESS(PC, SP)	h8300_push_return_address (PC, SP)

/* override the standard get_saved_register function with 
   one that takes account of generic CALL_DUMMY frames */
#define GET_SAVED_REGISTER(raw_buffer, optimized, addrp, frame, regnum, lval) \
     generic_get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval)
