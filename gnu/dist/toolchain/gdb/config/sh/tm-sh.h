/* Target-specific definition for a Hitachi Super-H.
   Copyright (C) 1993 Free Software Foundation, Inc.

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

#define GDB_TARGET_IS_SH

#define IEEE_FLOAT 1

/* Define the bit, byte, and word ordering of the machine.  */

#define TARGET_BYTE_ORDER_SELECTABLE


/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

extern CORE_ADDR sh_skip_prologue PARAMS ((CORE_ADDR));
#define SKIP_PROLOGUE(ip) (sh_skip_prologue (ip))

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.

   The return address is the value saved in the PR register + 4  */

#define SAVED_PC_AFTER_CALL(frame) (ADDR_BITS_REMOVE(read_register(PR_REGNUM)))

/* Stack grows downward.  */

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/* Illegal instruction - used by the simulator for breakpoint
   detection */

#define BREAKPOINT {0xc3, 0xc3}	/* 0xc3c3 is trapa #c3, and it works in big
				   and little endian modes  */

#define BIG_REMOTE_BREAKPOINT    { 0xc3, 0x20 }
#define LITTLE_REMOTE_BREAKPOINT { 0x20, 0xc3 }

/* If your kernel resets the pc after the trap happens you may need to
   define this before including this file.  */
#define DECR_PC_AFTER_BREAK 0

/* Say how long registers are.  */
#define REGISTER_TYPE  long

/* Say how much memory is needed to store a copy of the register set */
#define REGISTER_BYTES    (NUM_REGS*4)

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N)  ((N)*4)

/* Number of bytes of storage in the actual machine representation
   for register N.  */

#define REGISTER_RAW_SIZE(N) 4

#define REGISTER_VIRTUAL_SIZE(N) 4

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 4

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 4

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
	((((N) >= FP0_REGNUM && (N) <= FP15_REGNUM)	\
	  || (N) == FPUL_REGNUM)			\
	 ? builtin_type_float : builtin_type_int)

/* Initializer for an array of names of registers.
   Entries beyond the first NUM_REGS are ignored.  */

extern char **sh_register_names;
#define REGISTER_NAME(i) sh_register_names[i]

#define NUM_REGS 59

/* Register numbers of various important registers.  Note that some of
   these values are "real" register numbers, and correspond to the
   general registers of the machine, and some are "phony" register
   numbers which are too large to be actual register numbers as far as
   the user is concerned but do serve to get the desired values when
   passed to read_register.  */

#define R0_REGNUM	0
#define STRUCT_RETURN_REGNUM 2
#define ARG0_REGNUM     4
#define ARGLAST_REGNUM  7
#define FP_REGNUM 	14
#define SP_REGNUM 	15
#define PC_REGNUM 	16
#define PR_REGNUM 	17
#define GBR_REGNUM 	18
#define VBR_REGNUM 	19
#define MACH_REGNUM 	20
#define MACL_REGNUM 	21
#define SR_REGNUM 	22
#define FPUL_REGNUM	23
#define FPSCR_REGNUM	24
#define FP0_REGNUM	25
#define FP15_REGNUM	40
#define SSR_REGNUM	41
#define SPC_REGNUM	42
#define R0B0_REGNUM	43
#define R0B1_REGNUM	51

#define NUM_REALREGS	59

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function.

   We store structs through a pointer passed in R0 */

#define STORE_STRUCT_RETURN(ADDR, SP) \
    { write_register (STRUCT_RETURN_REGNUM, (ADDR));  }

extern use_struct_convention_fn sh_use_struct_convention;
#define USE_STRUCT_CONVENTION(gcc_p, type) sh_use_struct_convention (gcc_p, type)

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

extern void sh_extract_return_value PARAMS ((struct type *, void *, void *));
#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
	sh_extract_return_value (TYPE, REGBUF, VALBUF)

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.

   Things always get returned in R0/R1 */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  write_register_bytes (REGISTER_BYTE(0), VALBUF, TYPE_LENGTH (TYPE))

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
     extract_address (REGBUF, REGISTER_RAW_SIZE (0))


/* Define other aspects of the stack frame.
   we keep a copy of the worked out return pc lying around, since it
   is a useful bit of info */

#define EXTRA_FRAME_INFO \
    CORE_ADDR return_pc; \
    int leaf_function;   \
    int f_offset;

#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) \
    sh_init_extra_frame_info(fromleaf, fi)
extern void sh_init_extra_frame_info PARAMS ((int, struct frame_info *));

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */

#define FRAMELESS_FUNCTION_INVOCATION(FI) \
  (frameless_look_for_prologue(FI))

#define FRAME_SAVED_PC(FRAME)		((FRAME)->return_pc)
#define FRAME_ARGS_ADDRESS(fi)   	((fi)->frame)
#define FRAME_LOCALS_ADDRESS(fi) 	((fi)->frame)

/* Set VAL to the number of args passed to frame described by FI.
   Can set VAL to -1, meaning no way to tell.  */

/* We can't tell how many args there are */

#define FRAME_NUM_ARGS(fi) (-1)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

extern void sh_frame_find_saved_regs PARAMS ((struct frame_info * fi,
					    struct frame_saved_regs * fsr));

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs)	    \
   sh_frame_find_saved_regs(frame_info, &(frame_saved_regs))

typedef unsigned short INSN_WORD;

extern CORE_ADDR sh_push_arguments PARAMS ((int nargs,
					    struct value ** args,
					    CORE_ADDR sp,
					    unsigned char struct_return,
					    CORE_ADDR struct_addr));

#define USE_GENERIC_DUMMY_FRAMES 1
#define CALL_DUMMY                   {0}
#define CALL_DUMMY_LENGTH            (0)
#define CALL_DUMMY_START_OFFSET      (0)
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)
#define FIX_CALL_DUMMY(DUMMY, STARTADDR, FUNADDR, NARGS, ARGS, TYPE, GCCP)
#define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
#define CALL_DUMMY_ADDRESS()         entry_point_address ()
extern CORE_ADDR sh_push_return_address PARAMS ((CORE_ADDR, CORE_ADDR));
#define PUSH_RETURN_ADDRESS(PC, SP)  sh_push_return_address (PC, SP)


extern CORE_ADDR sh_frame_chain PARAMS ((struct frame_info *));
#define FRAME_CHAIN(FRAME)           sh_frame_chain(FRAME)
#define PUSH_DUMMY_FRAME             generic_push_dummy_frame ()
#define FRAME_CHAIN_VALID(FP, FRAME) generic_file_frame_chain_valid (FP, FRAME)
#define PC_IN_CALL_DUMMY(PC, SP, FP) generic_pc_in_call_dummy (PC, SP, FP)
#define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
    (sh_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR))

/* override the standard get_saved_register function with
   one that takes account of generic CALL_DUMMY frames */
#define GET_SAVED_REGISTER(raw_buffer, optimized, addrp, frame, regnum, lval) \
     generic_get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval)

/* Discard from the stack the innermost frame, restoring all saved
   registers.  */

extern void sh_pop_frame PARAMS ((void));
#define POP_FRAME sh_pop_frame();

#define NOP   {0x20, 0x0b}

#define REGISTER_SIZE 4

#define COERCE_FLOAT_TO_DOUBLE(formal, actual) (1)

#define BELIEVE_PCC_PROMOTION 1

/* Need this for WinGDB.  See gdb/mswin/{regdoc.h, gdbwin.c, gui.cpp}.  */
#define TARGET_SH
