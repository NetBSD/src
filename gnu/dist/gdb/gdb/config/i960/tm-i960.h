// OBSOLETE /* Parameters for target machine Intel 960, for GDB, the GNU debugger.
// OBSOLETE 
// OBSOLETE    Copyright 1990, 1991, 1993, 1994, 1996, 1998, 1999, 2000, 2002 Free
// OBSOLETE    Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    Contributed by Intel Corporation.
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
// OBSOLETE /* Definitions to target GDB to any i960.  */
// OBSOLETE 
// OBSOLETE #ifndef I80960
// OBSOLETE #define I80960
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #include "doublest.h"
// OBSOLETE 
// OBSOLETE /* Hook for the SYMBOL_CLASS of a parameter when decoding DBX symbol
// OBSOLETE    information.  In the i960, parameters can be stored as locals or as
// OBSOLETE    args, depending on the type of the debug record.
// OBSOLETE 
// OBSOLETE    From empirical observation, gcc960 uses N_LSYM to indicate
// OBSOLETE    arguments passed in registers and then copied immediately
// OBSOLETE    to the frame, and N_PSYM to indicate arguments passed in a
// OBSOLETE    g14-relative argument block.  */
// OBSOLETE 
// OBSOLETE #define	DBX_PARM_SYMBOL_CLASS(type) ((type == N_LSYM)? LOC_LOCAL_ARG: LOC_ARG)
// OBSOLETE 
// OBSOLETE /* Offset from address of function to start of its code.
// OBSOLETE    Zero on most machines.  */
// OBSOLETE 
// OBSOLETE #define FUNCTION_START_OFFSET 0
// OBSOLETE 
// OBSOLETE /* Advance ip across any function entry prologue instructions
// OBSOLETE    to reach some "real" code.  */
// OBSOLETE 
// OBSOLETE #define SKIP_PROLOGUE(ip)	(i960_skip_prologue (ip))
// OBSOLETE extern CORE_ADDR i960_skip_prologue ();
// OBSOLETE 
// OBSOLETE /* Immediately after a function call, return the saved ip.
// OBSOLETE    Can't always go through the frames for this because on some machines
// OBSOLETE    the new frame is not set up until the new function
// OBSOLETE    executes some instructions.  */
// OBSOLETE 
// OBSOLETE #define SAVED_PC_AFTER_CALL(frame) (saved_pc_after_call (frame))
// OBSOLETE extern CORE_ADDR saved_pc_after_call ();
// OBSOLETE 
// OBSOLETE /* Stack grows upward */
// OBSOLETE 
// OBSOLETE #define INNER_THAN(lhs,rhs) ((lhs) > (rhs))
// OBSOLETE 
// OBSOLETE /* Say how long (ordinary) registers are.  This is a piece of bogosity
// OBSOLETE    used in push_word and a few other places; REGISTER_RAW_SIZE is the
// OBSOLETE    real way to know how big a register is.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_SIZE 4
// OBSOLETE 
// OBSOLETE /* Number of machine registers */
// OBSOLETE #define NUM_REGS 40
// OBSOLETE 
// OBSOLETE /* Initializer for an array of names of registers.
// OBSOLETE    There should be NUM_REGS strings in this initializer.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_NAMES { \
// OBSOLETE 	/*  0 */ "pfp", "sp",  "rip", "r3",  "r4",  "r5",  "r6",  "r7", \
// OBSOLETE 	/*  8 */ "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",\
// OBSOLETE 	/* 16 */ "g0",  "g1",  "g2",  "g3",  "g4",  "g5",  "g6",  "g7", \
// OBSOLETE 	/* 24 */ "g8",  "g9",  "g10", "g11", "g12", "g13", "g14", "fp", \
// OBSOLETE 	/* 32 */ "pcw", "ac",  "tc",  "ip",  "fp0", "fp1", "fp2", "fp3",\
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Register numbers of various important registers (used to index
// OBSOLETE    into arrays of register names and register values).  */
// OBSOLETE 
// OBSOLETE #define R0_REGNUM   0		/* First local register           */
// OBSOLETE #define SP_REGNUM   1		/* Contains address of top of stack */
// OBSOLETE #define RIP_REGNUM  2		/* Return instruction pointer (local r2) */
// OBSOLETE #define R15_REGNUM 15		/* Last local register            */
// OBSOLETE #define G0_REGNUM  16		/* First global register  */
// OBSOLETE #define G13_REGNUM 29		/* g13 - holds struct return address */
// OBSOLETE #define G14_REGNUM 30		/* g14 - ptr to arg block / leafproc return address */
// OBSOLETE #define FP_REGNUM  31		/* Contains address of executing stack frame */
// OBSOLETE #define	PCW_REGNUM 32		/* process control word */
// OBSOLETE #define	ACW_REGNUM 33		/* arithmetic control word */
// OBSOLETE #define	TCW_REGNUM 34		/* trace control word */
// OBSOLETE #define IP_REGNUM  35		/* instruction pointer */
// OBSOLETE #define FP0_REGNUM 36		/* First floating point register */
// OBSOLETE 
// OBSOLETE /* Some registers have more than one name */
// OBSOLETE 
// OBSOLETE #define PC_REGNUM  IP_REGNUM	/* GDB refers to ip as the Program Counter */
// OBSOLETE #define PFP_REGNUM R0_REGNUM	/* Previous frame pointer */
// OBSOLETE 
// OBSOLETE /* Total amount of space needed to store our copies of the machine's
// OBSOLETE    register state, the array `registers'.  */
// OBSOLETE #define REGISTER_BYTES ((36*4) + (4*10))
// OBSOLETE 
// OBSOLETE /* Index within `registers' of the first byte of the space for register N.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_BYTE(N) ( (N) < FP0_REGNUM ? \
// OBSOLETE 				(4*(N)) : ((10*(N)) - (6*FP0_REGNUM)) )
// OBSOLETE 
// OBSOLETE /* The i960 has register windows, sort of.  */
// OBSOLETE 
// OBSOLETE extern void i960_get_saved_register (char *raw_buffer,
// OBSOLETE 				     int *optimized,
// OBSOLETE 				     CORE_ADDR *addrp,
// OBSOLETE 				     struct frame_info *frame,
// OBSOLETE 				     int regnum,
// OBSOLETE 				     enum lval_type *lval);
// OBSOLETE 
// OBSOLETE #define GET_SAVED_REGISTER(raw_buffer, optimized, addrp, frame, regnum, lval) \
// OBSOLETE   i960_get_saved_register(raw_buffer, optimized, addrp, frame, regnum, lval)
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Number of bytes of storage in the actual machine representation
// OBSOLETE    for register N.  On the i960, all regs are 4 bytes except for floating
// OBSOLETE    point, which are 10.  NINDY only sends us 8 byte values for these,
// OBSOLETE    which is a pain, but VxWorks handles this correctly, so we must.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_RAW_SIZE(N)		( (N) < FP0_REGNUM ? 4 : 10 )
// OBSOLETE 
// OBSOLETE /* Number of bytes of storage in the program's representation for register N. */
// OBSOLETE 
// OBSOLETE #define REGISTER_VIRTUAL_SIZE(N)	( (N) < FP0_REGNUM ? 4 : 8 )
// OBSOLETE 
// OBSOLETE /* Largest value REGISTER_RAW_SIZE can have.  */
// OBSOLETE 
// OBSOLETE #define MAX_REGISTER_RAW_SIZE 10
// OBSOLETE 
// OBSOLETE /* Largest value REGISTER_VIRTUAL_SIZE can have.  */
// OBSOLETE 
// OBSOLETE #define MAX_REGISTER_VIRTUAL_SIZE 8
// OBSOLETE 
// OBSOLETE #include "floatformat.h"
// OBSOLETE 
// OBSOLETE #define TARGET_LONG_DOUBLE_FORMAT &floatformat_i960_ext
// OBSOLETE 
// OBSOLETE /* Return the GDB type object for the "standard" data type
// OBSOLETE    of data in register N.  */
// OBSOLETE 
// OBSOLETE struct type *i960_register_type (int regnum);
// OBSOLETE #define REGISTER_VIRTUAL_TYPE(N) i960_register_type (N)
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
// OBSOLETE    On i960, a structure is returned in registers g0-g3, if it will fit.
// OBSOLETE    If it's more than 16 bytes long, g13 pointed to it on entry.  */
// OBSOLETE 
// OBSOLETE extern use_struct_convention_fn i960_use_struct_convention;
// OBSOLETE #define USE_STRUCT_CONVENTION(gcc_p, type) i960_use_struct_convention (gcc_p, type)
// OBSOLETE 
// OBSOLETE /* Extract from an array REGBUF containing the (raw) register state
// OBSOLETE    a function return value of type TYPE, and copy that, in virtual format,
// OBSOLETE    into VALBUF.  This is only called if USE_STRUCT_CONVENTION for this
// OBSOLETE    type is 0.
// OBSOLETE 
// OBSOLETE    On the i960 we just take as many bytes as we need from G0 through G3.  */
// OBSOLETE 
// OBSOLETE #define DEPRECATED_EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
// OBSOLETE 	memcpy(VALBUF, REGBUF+REGISTER_BYTE(G0_REGNUM), TYPE_LENGTH (TYPE))
// OBSOLETE 
// OBSOLETE /* If USE_STRUCT_CONVENTION produces a 1, 
// OBSOLETE    extract from an array REGBUF containing the (raw) register state
// OBSOLETE    the address in which a function should return its structure value,
// OBSOLETE    as a CORE_ADDR (or an expression that can be used as one).
// OBSOLETE 
// OBSOLETE    Address of where to put structure was passed in in global
// OBSOLETE    register g13 on entry.  God knows what's in g13 now.  The
// OBSOLETE    (..., 0) below is to make it appear to return a value, though
// OBSOLETE    actually all it does is call error().  */
// OBSOLETE 
// OBSOLETE #define DEPRECATED_EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
// OBSOLETE    (error("Don't know where large structure is returned on i960"), 0)
// OBSOLETE 
// OBSOLETE /* Write into appropriate registers a function return value
// OBSOLETE    of type TYPE, given in virtual format, for "value returning" functions.
// OBSOLETE 
// OBSOLETE    For 'return' command:  not (yet) implemented for i960.  */
// OBSOLETE 
// OBSOLETE #define STORE_RETURN_VALUE(TYPE,VALBUF) \
// OBSOLETE     error ("Returning values from functions is not implemented in i960 gdb")
// OBSOLETE 
// OBSOLETE /* Store the address of the place in which to copy the structure the
// OBSOLETE    subroutine will return.  This is called from call_function. */
// OBSOLETE 
// OBSOLETE #define STORE_STRUCT_RETURN(ADDR, SP) \
// OBSOLETE     error ("Returning values from functions is not implemented in i960 gdb")
// OBSOLETE 
// OBSOLETE /* Describe the pointer in each stack frame to the previous stack frame
// OBSOLETE    (its caller).  */
// OBSOLETE 
// OBSOLETE /* FRAME_CHAIN takes a frame's nominal address
// OBSOLETE    and produces the frame's chain-pointer.
// OBSOLETE 
// OBSOLETE    However, if FRAME_CHAIN_VALID returns zero,
// OBSOLETE    it means the given frame is the outermost one and has no caller.  */
// OBSOLETE 
// OBSOLETE /* We cache information about saved registers in the frame structure,
// OBSOLETE    to save us from having to re-scan function prologues every time
// OBSOLETE    a register in a non-current frame is accessed.  */
// OBSOLETE 
// OBSOLETE #define EXTRA_FRAME_INFO 	\
// OBSOLETE 	struct frame_saved_regs *fsr;	\
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
// OBSOLETE /* On the i960, we get the chain pointer by reading the PFP saved
// OBSOLETE    on the stack and clearing the status bits.  */
// OBSOLETE 
// OBSOLETE #define FRAME_CHAIN(thisframe) \
// OBSOLETE   (read_memory_integer (FRAME_FP(thisframe), 4) & ~0xf)
// OBSOLETE 
// OBSOLETE /* FRAME_CHAIN_VALID returns zero if the given frame is the outermost one
// OBSOLETE    and has no caller.
// OBSOLETE 
// OBSOLETE    On the i960, each various target system type must define FRAME_CHAIN_VALID,
// OBSOLETE    since it differs between NINDY and VxWorks, the two currently supported
// OBSOLETE    targets types.  We leave it undefined here.  */
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* A macro that tells us whether the function invocation represented
// OBSOLETE    by FI does not have a frame on the stack associated with it.  If it
// OBSOLETE    does not, FRAMELESS is set to 1, else 0.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR leafproc_return (CORE_ADDR ip);
// OBSOLETE #define FRAMELESS_FUNCTION_INVOCATION(FI) \
// OBSOLETE   (leafproc_return ((FI)->pc) != 0)
// OBSOLETE 
// OBSOLETE /* Note that in the i960 architecture the return pointer is saved in the
// OBSOLETE    *caller's* stack frame.
// OBSOLETE 
// OBSOLETE    Make sure to zero low-order bits because of bug in 960CA A-step part
// OBSOLETE    (instruction addresses should always be word-aligned anyway).  */
// OBSOLETE 
// OBSOLETE #define FRAME_SAVED_PC(frame) \
// OBSOLETE 			((read_memory_integer(FRAME_CHAIN(frame)+8,4)) & ~3)
// OBSOLETE 
// OBSOLETE /* On the i960, FRAME_ARGS_ADDRESS should return the value of
// OBSOLETE    g14 as passed into the frame, if known.  We need a function for this.
// OBSOLETE    We cache this value in the frame info if we've already looked it up.  */
// OBSOLETE 
// OBSOLETE #define FRAME_ARGS_ADDRESS(fi) 	\
// OBSOLETE   (((fi)->arg_pointer != -1)? (fi)->arg_pointer: frame_args_address (fi, 0))
// OBSOLETE extern CORE_ADDR frame_args_address ();		/* i960-tdep.c */
// OBSOLETE 
// OBSOLETE /* This is the same except it should return 0 when
// OBSOLETE    it does not really know where the args are, rather than guessing.
// OBSOLETE    This value is not cached since it is only used infrequently.  */
// OBSOLETE 
// OBSOLETE #define	FRAME_ARGS_ADDRESS_CORRECT(fi)	(frame_args_address (fi, 1))
// OBSOLETE 
// OBSOLETE #define FRAME_LOCALS_ADDRESS(fi)	(fi)->frame
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
// OBSOLETE extern void frame_find_saved_regs ();	/* See i960-tdep.c */
// OBSOLETE 
// OBSOLETE /* Things needed for making calls to functions in the inferior process */
// OBSOLETE 
// OBSOLETE /* Push an empty stack frame, to record the current ip, etc.
// OBSOLETE 
// OBSOLETE    Not (yet?) implemented for i960.  */
// OBSOLETE 
// OBSOLETE #define PUSH_DUMMY_FRAME	\
// OBSOLETE error("Function calls into the inferior process are not supported on the i960")
// OBSOLETE 
// OBSOLETE /* Discard from the stack the innermost frame, restoring all registers.  */
// OBSOLETE 
// OBSOLETE 
// OBSOLETE void i960_pop_frame (void);
// OBSOLETE #define POP_FRAME \
// OBSOLETE 	i960_pop_frame ()
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* This sequence of words is the instructions
// OBSOLETE 
// OBSOLETE    callx 0x00000000
// OBSOLETE    fmark
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE /* #define CALL_DUMMY { 0x86003000, 0x00000000, 0x66003e00 } */
// OBSOLETE 
// OBSOLETE 																			    /* #define CALL_DUMMY_START_OFFSET 0 *//* Start execution at beginning of dummy */
// OBSOLETE 
// OBSOLETE /* Indicate that we don't support calling inferior child functions.  */
// OBSOLETE 
// OBSOLETE #undef CALL_DUMMY
// OBSOLETE 
// OBSOLETE /* Insert the specified number of args and function address
// OBSOLETE    into a call sequence of the above form stored at 'dummyname'.
// OBSOLETE 
// OBSOLETE    Ignore arg count on i960.  */
// OBSOLETE 
// OBSOLETE /* #define FIX_CALL_DUMMY(dummyname, fun, nargs) *(((int *)dummyname)+1) = fun */
// OBSOLETE 
// OBSOLETE #undef FIX_CALL_DUMMY
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Interface definitions for kernel debugger KDB */
// OBSOLETE /* (Not relevant to i960.) */
