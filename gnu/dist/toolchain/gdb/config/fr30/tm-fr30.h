/* Parameters for execution on a Fujitsu FR30 processor.
   Copyright 1999, Free Software Foundation, Inc.

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

#define FR30_GENREGS		16
#define FR30_DEDICATEDREGS	8
#define FR30_REGSIZE		4	/* bytes */

#define NUM_REGS (FR30_GENREGS + FR30_DEDICATEDREGS)
#define REGISTER_BYTES ((FR30_GENREGS + FR30_DEDICATEDREGS)*FR30_REGSIZE)

/* Index within `registers' of the first byte of the space for
   register N.  */
#define REGISTER_BYTE(N) ((N) * FR30_REGSIZE)

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */
#define REGISTER_NAMES \
{ "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", \
  "r9", "r10", "r11", "r12", "r13", "r14", "r15", \
  "pc", "ps", "tbr", "rp", "ssp", "usp", "mdh", "mdl" }

/* Offset from address of function to start of its code.
   Zero on most machines.  */
#define FUNCTION_START_OFFSET 0

/* Amount PC must be decremented by after a breakpoint.
   This is often the number of bytes in BREAKPOINT
   but not always.  */

#define DECR_PC_AFTER_BREAK 0

/* Stack grows downward.  */

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

#define TARGET_BYTE_ORDER BIG_ENDIAN

#define R0_REGNUM  0
#define R1_REGNUM  1
#define R2_REGNUM  2
#define R3_REGNUM  3
#define R4_REGNUM  4
#define R5_REGNUM  5
#define R6_REGNUM  6
#define R7_REGNUM  7
#define R8_REGNUM  8
#define R9_REGNUM  9
#define R10_REGNUM  10
#define R11_REGNUM  11
#define R12_REGNUM  12
#define R13_REGNUM  13
#define FP_REGNUM 14		/* Frame pointer */
#define SP_REGNUM 15		/* Stack pointer */
#define PC_REGNUM 16		/* Program counter */
#define RP_REGNUM 19		/* Return pointer */

#define FIRST_ARGREG	R4_REGNUM	/* first arg (or struct ret val addr) */
#define LAST_ARGREG	R7_REGNUM	/* fourth (or third arg) */
#define RETVAL_REG	R4_REGNUM	/* return vaue */

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */
#define REGISTER_SIZE FR30_REGSIZE

/* Number of bytes of storage in the actual machine representation
   for register N.  */
#define REGISTER_RAW_SIZE(N) FR30_REGSIZE

/* Largest value REGISTER_RAW_SIZE can have.  */
#define MAX_REGISTER_RAW_SIZE FR30_REGSIZE

/* Number of bytes of storage in the program's representation
   for register N.  */
#define REGISTER_VIRTUAL_SIZE(N) REGISTER_RAW_SIZE(N)

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */
#define MAX_REGISTER_VIRTUAL_SIZE FR30_REGSIZE

extern void fr30_pop_frame PARAMS ((void));
#define POP_FRAME fr30_pop_frame()

#define USE_GENERIC_DUMMY_FRAMES 1
#define CALL_DUMMY                   {0}
#define CALL_DUMMY_START_OFFSET      (0)
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)
#define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
#define FIX_CALL_DUMMY(DUMMY, START, FUNADDR, NARGS, ARGS, TYPE, GCCP)
#define CALL_DUMMY_ADDRESS()         entry_point_address ()
#define PUSH_RETURN_ADDRESS(PC, SP)  (write_register(RP_REGNUM, CALL_DUMMY_ADDRESS()), SP)
#define PUSH_DUMMY_FRAME	generic_push_dummy_frame ()

/* Number of bytes at start of arglist that are not really args.  */
#define FRAME_ARGS_SKIP 0

/* Return the GDB type object for the "standard" data type
   of data in register N.  */
#define REGISTER_VIRTUAL_TYPE(REG) builtin_type_int

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */
#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
    memcpy (VALBUF, REGBUF + REGISTER_BYTE(RETVAL_REG) +  \
	(TYPE_LENGTH(TYPE) < 4 ? 4 - TYPE_LENGTH(TYPE) : 0), TYPE_LENGTH (TYPE))

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */
#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  extract_address (REGBUF + REGISTER_BYTE (RETVAL_REG), \
		   REGISTER_RAW_SIZE (RETVAL_REG))

#define STORE_STRUCT_RETURN(ADDR, SP) \
  { write_register (RETVAL_REG, (ADDR)); }

#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)
#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)

/* Return number of args passed to a frame.
   Can return -1, meaning no way to tell.  */
#define FRAME_NUM_ARGS(fi) (-1)

/* Forward decls for prototypes */
struct frame_info;
struct frame_saved_regs;
struct type;
struct value;

#define EXTRA_FRAME_INFO \
  struct frame_saved_regs fsr;  \
  int framesize;		\
  int frameoffset;		\
  int framereg;

extern CORE_ADDR fr30_frame_chain PARAMS ((struct frame_info * fi));
#define FRAME_CHAIN(fi) fr30_frame_chain (fi)

extern CORE_ADDR fr30_frame_saved_pc PARAMS ((struct frame_info *));
#define FRAME_SAVED_PC(fi) (fr30_frame_saved_pc (fi))

#define SAVED_PC_AFTER_CALL(fi) read_register (RP_REGNUM)

extern CORE_ADDR fr30_skip_prologue PARAMS ((CORE_ADDR pc));
#define SKIP_PROLOGUE(pc) (fr30_skip_prologue (pc))

/* Write into appropriate registers a function return value of type
   TYPE, given in virtual format.  VALBUF is in the target byte order;
   it's typically the VALUE_CONTENTS of some struct value, and those
   are in the target's byte order.  */
extern void fr30_store_return_value PARAMS ((struct type * type, char *valbuf));

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  (fr30_store_return_value ((TYPE), (VALBUF)))

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */
#define FRAME_FIND_SAVED_REGS(fi, regaddr) regaddr = fi->fsr

/* Use INT #BREAKPOINT_INTNUM instruction for breakpoint */
#define FR30_BREAKOP	0x1f	/* opcode, type D instruction */
#define BREAKPOINT_INTNUM 9	/* one of the reserved traps */
#define BREAKPOINT {FR30_BREAKOP, BREAKPOINT_INTNUM}

/* Define this for Wingdb */
#define TARGET_FR30

/* IEEE format floating point */
#define IEEE_FLOAT

/* Define other aspects of the stack frame.  */

/* An expression that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  */
extern int fr30_frameless_function_invocation PARAMS ((struct frame_info * frame));
#define FRAMELESS_FUNCTION_INVOCATION(FI) (fr30_frameless_function_invocation (FI));

extern void fr30_init_extra_frame_info PARAMS ((struct frame_info * fi));
#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) fr30_init_extra_frame_info (fi)

#define FRAME_CHAIN_VALID(FP, FI)	generic_file_frame_chain_valid (FP, FI)

extern CORE_ADDR
  fr30_push_arguments PARAMS ((int nargs, struct value ** args, CORE_ADDR sp,
			       int struct_return,
			       CORE_ADDR struct_addr));
#define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
  (fr30_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR))

#define PC_IN_CALL_DUMMY(PC, SP, FP) generic_pc_in_call_dummy (PC, SP, FP)

/* Fujitsu's ABI requires all structs to be passed using a pointer.
   That is obviously not very efficient, so I am leaving the definitions
   to make gdb work with GCC style struct passing, in case we decide
   to go for better performance, rather than for compatibility with
   Fujitsu (just change STRUCT_ALWAYS_BY_ADDR to 0) */

#define STRUCT_ALWAYS_BY_ADDR	1

#if(STRUCT_ALWAYS_BY_ADDR)
#define REG_STRUCT_HAS_ADDR(gcc_p,type)		1
#else
/* more standard GCC (optimized) */
#define REG_STRUCT_HAS_ADDR(gcc_p,type)		\
		((TYPE_LENGTH(type) > 4) && (TYPE_LENGTH(type) & 0x3))
#endif
/* alway return struct by value by input pointer */
#define USE_STRUCT_CONVENTION(GCC_P, TYPE)	1

/* The stack should always be aligned on a four-word boundary.  */
#define STACK_ALIGN(len) (((len) + 3) & ~3)

/* I think the comment about this in value_arg_coerce is wrong; this
   should be true on any system where you can rely on the prototyping
   information.  When this is true, value_arg_coerce will promote
   floats to doubles iff the function is not prototyped.  */
#define COERCE_FLOAT_TO_DOUBLE(formal, actual) (1)
