// OBSOLETE /* Parameters for execution on a Fujitsu FR30 processor.
// OBSOLETE    Copyright 1999, 2000 Free Software Foundation, Inc.
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
// OBSOLETE #define FR30_GENREGS		16
// OBSOLETE #define FR30_DEDICATEDREGS	8
// OBSOLETE #define FR30_REGSIZE		4	/* bytes */
// OBSOLETE 
// OBSOLETE #define NUM_REGS (FR30_GENREGS + FR30_DEDICATEDREGS)
// OBSOLETE #define REGISTER_BYTES ((FR30_GENREGS + FR30_DEDICATEDREGS)*FR30_REGSIZE)
// OBSOLETE 
// OBSOLETE /* Index within `registers' of the first byte of the space for
// OBSOLETE    register N.  */
// OBSOLETE #define REGISTER_BYTE(N) ((N) * FR30_REGSIZE)
// OBSOLETE 
// OBSOLETE /* Initializer for an array of names of registers.
// OBSOLETE    There should be NUM_REGS strings in this initializer.  */
// OBSOLETE #define REGISTER_NAMES \
// OBSOLETE { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", \
// OBSOLETE   "r9", "r10", "r11", "r12", "r13", "r14", "r15", \
// OBSOLETE   "pc", "ps", "tbr", "rp", "ssp", "usp", "mdh", "mdl" }
// OBSOLETE 
// OBSOLETE /* Offset from address of function to start of its code.
// OBSOLETE    Zero on most machines.  */
// OBSOLETE #define FUNCTION_START_OFFSET 0
// OBSOLETE 
// OBSOLETE /* Amount PC must be decremented by after a breakpoint.
// OBSOLETE    This is often the number of bytes in BREAKPOINT
// OBSOLETE    but not always.  */
// OBSOLETE 
// OBSOLETE #define DECR_PC_AFTER_BREAK 0
// OBSOLETE 
// OBSOLETE /* Stack grows downward.  */
// OBSOLETE 
// OBSOLETE #define INNER_THAN(lhs,rhs) ((lhs) < (rhs))
// OBSOLETE 
// OBSOLETE #define R0_REGNUM  0
// OBSOLETE #define R1_REGNUM  1
// OBSOLETE #define R2_REGNUM  2
// OBSOLETE #define R3_REGNUM  3
// OBSOLETE #define R4_REGNUM  4
// OBSOLETE #define R5_REGNUM  5
// OBSOLETE #define R6_REGNUM  6
// OBSOLETE #define R7_REGNUM  7
// OBSOLETE #define R8_REGNUM  8
// OBSOLETE #define R9_REGNUM  9
// OBSOLETE #define R10_REGNUM  10
// OBSOLETE #define R11_REGNUM  11
// OBSOLETE #define R12_REGNUM  12
// OBSOLETE #define R13_REGNUM  13
// OBSOLETE #define FP_REGNUM 14		/* Frame pointer */
// OBSOLETE #define SP_REGNUM 15		/* Stack pointer */
// OBSOLETE #define PC_REGNUM 16		/* Program counter */
// OBSOLETE #define RP_REGNUM 19		/* Return pointer */
// OBSOLETE 
// OBSOLETE #define FIRST_ARGREG	R4_REGNUM	/* first arg (or struct ret val addr) */
// OBSOLETE #define LAST_ARGREG	R7_REGNUM	/* fourth (or third arg) */
// OBSOLETE #define RETVAL_REG	R4_REGNUM	/* return vaue */
// OBSOLETE 
// OBSOLETE /* Say how long (ordinary) registers are.  This is a piece of bogosity
// OBSOLETE    used in push_word and a few other places; REGISTER_RAW_SIZE is the
// OBSOLETE    real way to know how big a register is.  */
// OBSOLETE #define REGISTER_SIZE FR30_REGSIZE
// OBSOLETE 
// OBSOLETE /* Number of bytes of storage in the actual machine representation
// OBSOLETE    for register N.  */
// OBSOLETE #define REGISTER_RAW_SIZE(N) FR30_REGSIZE
// OBSOLETE 
// OBSOLETE /* Largest value REGISTER_RAW_SIZE can have.  */
// OBSOLETE #define MAX_REGISTER_RAW_SIZE FR30_REGSIZE
// OBSOLETE 
// OBSOLETE /* Number of bytes of storage in the program's representation
// OBSOLETE    for register N.  */
// OBSOLETE #define REGISTER_VIRTUAL_SIZE(N) REGISTER_RAW_SIZE(N)
// OBSOLETE 
// OBSOLETE /* Largest value REGISTER_VIRTUAL_SIZE can have.  */
// OBSOLETE #define MAX_REGISTER_VIRTUAL_SIZE FR30_REGSIZE
// OBSOLETE 
// OBSOLETE extern void fr30_pop_frame (void);
// OBSOLETE #define POP_FRAME fr30_pop_frame()
// OBSOLETE 
// OBSOLETE #define USE_GENERIC_DUMMY_FRAMES 1
// OBSOLETE #define CALL_DUMMY                   {0}
// OBSOLETE #define CALL_DUMMY_START_OFFSET      (0)
// OBSOLETE #define CALL_DUMMY_BREAKPOINT_OFFSET (0)
// OBSOLETE #define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
// OBSOLETE #define FIX_CALL_DUMMY(DUMMY, START, FUNADDR, NARGS, ARGS, TYPE, GCCP)
// OBSOLETE #define CALL_DUMMY_ADDRESS()         entry_point_address ()
// OBSOLETE #define PUSH_RETURN_ADDRESS(PC, SP)  (write_register(RP_REGNUM, CALL_DUMMY_ADDRESS()), SP)
// OBSOLETE #define PUSH_DUMMY_FRAME	generic_push_dummy_frame ()
// OBSOLETE 
// OBSOLETE /* Number of bytes at start of arglist that are not really args.  */
// OBSOLETE #define FRAME_ARGS_SKIP 0
// OBSOLETE 
// OBSOLETE /* Return the GDB type object for the "standard" data type
// OBSOLETE    of data in register N.  */
// OBSOLETE #define REGISTER_VIRTUAL_TYPE(REG) builtin_type_int
// OBSOLETE 
// OBSOLETE /* Extract from an array REGBUF containing the (raw) register state
// OBSOLETE    a function return value of type TYPE, and copy that, in virtual format,
// OBSOLETE    into VALBUF.  */
// OBSOLETE #define DEPRECATED_EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
// OBSOLETE     memcpy (VALBUF, REGBUF + REGISTER_BYTE(RETVAL_REG) +  \
// OBSOLETE 	(TYPE_LENGTH(TYPE) < 4 ? 4 - TYPE_LENGTH(TYPE) : 0), TYPE_LENGTH (TYPE))
// OBSOLETE 
// OBSOLETE /* Extract from an array REGBUF containing the (raw) register state
// OBSOLETE    the address in which a function should return its structure value,
// OBSOLETE    as a CORE_ADDR (or an expression that can be used as one).  */
// OBSOLETE #define DEPRECATED_EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
// OBSOLETE   extract_address (REGBUF + REGISTER_BYTE (RETVAL_REG), \
// OBSOLETE 		   REGISTER_RAW_SIZE (RETVAL_REG))
// OBSOLETE 
// OBSOLETE #define STORE_STRUCT_RETURN(ADDR, SP) \
// OBSOLETE   { write_register (RETVAL_REG, (ADDR)); }
// OBSOLETE 
// OBSOLETE #define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)
// OBSOLETE #define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)
// OBSOLETE 
// OBSOLETE /* Return number of args passed to a frame.
// OBSOLETE    Can return -1, meaning no way to tell.  */
// OBSOLETE #define FRAME_NUM_ARGS(fi) (-1)
// OBSOLETE 
// OBSOLETE /* Forward decls for prototypes */
// OBSOLETE struct frame_info;
// OBSOLETE struct frame_saved_regs;
// OBSOLETE struct type;
// OBSOLETE struct value;
// OBSOLETE 
// OBSOLETE #define EXTRA_FRAME_INFO \
// OBSOLETE   struct frame_saved_regs fsr;  \
// OBSOLETE   int framesize;		\
// OBSOLETE   int frameoffset;		\
// OBSOLETE   int framereg;
// OBSOLETE 
// OBSOLETE extern CORE_ADDR fr30_frame_chain (struct frame_info *fi);
// OBSOLETE #define FRAME_CHAIN(fi) fr30_frame_chain (fi)
// OBSOLETE 
// OBSOLETE extern CORE_ADDR fr30_frame_saved_pc (struct frame_info *);
// OBSOLETE #define FRAME_SAVED_PC(fi) (fr30_frame_saved_pc (fi))
// OBSOLETE 
// OBSOLETE #define SAVED_PC_AFTER_CALL(fi) read_register (RP_REGNUM)
// OBSOLETE 
// OBSOLETE extern CORE_ADDR fr30_skip_prologue (CORE_ADDR pc);
// OBSOLETE #define SKIP_PROLOGUE(pc) (fr30_skip_prologue (pc))
// OBSOLETE 
// OBSOLETE /* Write into appropriate registers a function return value of type
// OBSOLETE    TYPE, given in virtual format.  VALBUF is in the target byte order;
// OBSOLETE    it's typically the VALUE_CONTENTS of some struct value, and those
// OBSOLETE    are in the target's byte order.  */
// OBSOLETE extern void fr30_store_return_value (struct type *type, char *valbuf);
// OBSOLETE 
// OBSOLETE #define STORE_RETURN_VALUE(TYPE,VALBUF) \
// OBSOLETE   (fr30_store_return_value ((TYPE), (VALBUF)))
// OBSOLETE 
// OBSOLETE /* Put here the code to store, into a struct frame_saved_regs,
// OBSOLETE    the addresses of the saved registers of frame described by FRAME_INFO.
// OBSOLETE    This includes special registers such as pc and fp saved in special
// OBSOLETE    ways in the stack frame.  sp is even more special:
// OBSOLETE    the address we return for it IS the sp for the next frame.  */
// OBSOLETE #define FRAME_FIND_SAVED_REGS(fi, regaddr) regaddr = fi->fsr
// OBSOLETE 
// OBSOLETE /* Use INT #BREAKPOINT_INTNUM instruction for breakpoint */
// OBSOLETE #define FR30_BREAKOP	0x1f	/* opcode, type D instruction */
// OBSOLETE #define BREAKPOINT_INTNUM 9	/* one of the reserved traps */
// OBSOLETE #define BREAKPOINT {FR30_BREAKOP, BREAKPOINT_INTNUM}
// OBSOLETE 
// OBSOLETE /* Define this for Wingdb */
// OBSOLETE #define TARGET_FR30
// OBSOLETE 
// OBSOLETE /* Define other aspects of the stack frame.  */
// OBSOLETE 
// OBSOLETE /* An expression that tells us whether the function invocation represented
// OBSOLETE    by FI does not have a frame on the stack associated with it.  */
// OBSOLETE extern int fr30_frameless_function_invocation (struct frame_info *frame);
// OBSOLETE #define FRAMELESS_FUNCTION_INVOCATION(FI) (fr30_frameless_function_invocation (FI));
// OBSOLETE 
// OBSOLETE extern void fr30_init_extra_frame_info (struct frame_info *fi);
// OBSOLETE #define INIT_EXTRA_FRAME_INFO(fromleaf, fi) fr30_init_extra_frame_info (fi)
// OBSOLETE 
// OBSOLETE #define FRAME_CHAIN_VALID(FP, FI)	generic_file_frame_chain_valid (FP, FI)
// OBSOLETE 
// OBSOLETE extern CORE_ADDR
// OBSOLETE fr30_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
// OBSOLETE 		     int struct_return, CORE_ADDR struct_addr);
// OBSOLETE #define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
// OBSOLETE   (fr30_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR))
// OBSOLETE 
// OBSOLETE #define PC_IN_CALL_DUMMY(PC, SP, FP) generic_pc_in_call_dummy (PC, SP, FP)
// OBSOLETE 
// OBSOLETE /* Fujitsu's ABI requires all structs to be passed using a pointer.
// OBSOLETE    That is obviously not very efficient, so I am leaving the definitions
// OBSOLETE    to make gdb work with GCC style struct passing, in case we decide
// OBSOLETE    to go for better performance, rather than for compatibility with
// OBSOLETE    Fujitsu (just change STRUCT_ALWAYS_BY_ADDR to 0) */
// OBSOLETE 
// OBSOLETE #define STRUCT_ALWAYS_BY_ADDR	1
// OBSOLETE 
// OBSOLETE #if(STRUCT_ALWAYS_BY_ADDR)
// OBSOLETE #define REG_STRUCT_HAS_ADDR(gcc_p,type)		1
// OBSOLETE #else
// OBSOLETE /* more standard GCC (optimized) */
// OBSOLETE #define REG_STRUCT_HAS_ADDR(gcc_p,type)		\
// OBSOLETE 		((TYPE_LENGTH(type) > 4) && (TYPE_LENGTH(type) & 0x3))
// OBSOLETE #endif
// OBSOLETE /* alway return struct by value by input pointer */
// OBSOLETE #define USE_STRUCT_CONVENTION(GCC_P, TYPE)	1
// OBSOLETE 
// OBSOLETE /* The stack should always be aligned on a four-word boundary.  */
// OBSOLETE #define STACK_ALIGN(len) (((len) + 3) & ~3)
// OBSOLETE 
// OBSOLETE /* I think the comment about this in value_arg_coerce is wrong; this
// OBSOLETE    should be true on any system where you can rely on the prototyping
// OBSOLETE    information.  When this is true, value_arg_coerce will promote
// OBSOLETE    floats to doubles iff the function is not prototyped.  */
// OBSOLETE #define COERCE_FLOAT_TO_DOUBLE(formal, actual) (1)
