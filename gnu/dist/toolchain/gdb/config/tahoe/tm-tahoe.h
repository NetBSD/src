/* OBSOLETE /* Definitions to make GDB target for a tahoe running 4.3-Reno. */
/* OBSOLETE    Copyright 1986, 1987, 1989, 1991, 1992, 1993 Free Software Foundation, Inc. */
/* OBSOLETE  */
/* OBSOLETE    This file is part of GDB. */
/* OBSOLETE  */
/* OBSOLETE    This program is free software; you can redistribute it and/or modify */
/* OBSOLETE    it under the terms of the GNU General Public License as published by */
/* OBSOLETE    the Free Software Foundation; either version 2 of the License, or */
/* OBSOLETE    (at your option) any later version. */
/* OBSOLETE  */
/* OBSOLETE    This program is distributed in the hope that it will be useful, */
/* OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE    GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE    You should have received a copy of the GNU General Public License */
/* OBSOLETE    along with this program; if not, write to the Free Software */
/* OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330, */
/* OBSOLETE    Boston, MA 02111-1307, USA.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* */
/* OBSOLETE  * Ported by the State University of New York at Buffalo by the Distributed */
/* OBSOLETE  * Computer Systems Lab, Department of Computer Science, 1991. */
/* OBSOLETE  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define TARGET_BYTE_ORDER BIG_ENDIAN */
/* OBSOLETE #define BITS_BIG_ENDIAN 0 */
/* OBSOLETE  */
/* OBSOLETE /* Offset from address of function to start of its code. */
/* OBSOLETE    Zero on most machines.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FUNCTION_START_OFFSET 2 */
/* OBSOLETE  */
/* OBSOLETE /* Advance PC across any function entry prologue instructions */
/* OBSOLETE    to reach some "real" code.  *x/ */
/* OBSOLETE  */
/* OBSOLETE extern CORE_ADDR tahoe_skip_prologue PARAMS ((CORE_ADDR)); */
/* OBSOLETE #define SKIP_PROLOGUE(pc) (tahoe_skip_prologue (pc)) */
/* OBSOLETE  */
/* OBSOLETE /* Immediately after a function call, return the saved pc. */
/* OBSOLETE    Can't always go through the frames for this because on some machines */
/* OBSOLETE    the new frame is not set up until the new function executes */
/* OBSOLETE    some instructions.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define SAVED_PC_AFTER_CALL(frame) FRAME_SAVED_PC(frame) */
/* OBSOLETE  */
/* OBSOLETE /* Wrong for cross-debugging.  I don't know the real values.  *x/ */
/* OBSOLETE #include <machine/param.h> */
/* OBSOLETE #define TARGET_UPAGES UPAGES */
/* OBSOLETE #define TARGET_NBPG NBPG */
/* OBSOLETE  */
/* OBSOLETE /* Address of end of stack space.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define STACK_END_ADDR (0xc0000000 - (TARGET_UPAGES * TARGET_NBPG)) */
/* OBSOLETE  */
/* OBSOLETE /* On BSD, sigtramp is in the u area.  Can't check the exact */
/* OBSOLETE    addresses because for cross-debugging we don't have target include */
/* OBSOLETE    files around.  This should be close enough.  *x/ */
/* OBSOLETE #define IN_SIGTRAMP(pc, name) ((pc) >= STACK_END_ADDR && (pc < 0xc0000000)) */
/* OBSOLETE  */
/* OBSOLETE /* Stack grows downward.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define INNER_THAN(lhs,rhs) ((lhs) < (rhs)) */
/* OBSOLETE  */
/* OBSOLETE /* Sequence of bytes for breakpoint instruction.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define BREAKPOINT {0x30} */
/* OBSOLETE  */
/* OBSOLETE /* Amount PC must be decremented by after a breakpoint. */
/* OBSOLETE    This is often the number of bytes in BREAKPOINT */
/* OBSOLETE    but not always.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define DECR_PC_AFTER_BREAK 0 */
/* OBSOLETE  */
/* OBSOLETE /* Return 1 if P points to an invalid floating point value. */
/* OBSOLETE    LEN is the length in bytes -- not relevant on the Tahoe. *x/ */
/* OBSOLETE  */
/* OBSOLETE #define INVALID_FLOAT(p, len) ((*(short *) p & 0xff80) == 0x8000) */
/* OBSOLETE  */
/* OBSOLETE /* Say how long (ordinary) registers are.  This is a piece of bogosity */
/* OBSOLETE    used in push_word and a few other places; REGISTER_RAW_SIZE is the */
/* OBSOLETE    real way to know how big a register is.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_SIZE 4 */
/* OBSOLETE  */
/* OBSOLETE /* Number of machine registers *x/ */
/* OBSOLETE  */
/* OBSOLETE #define NUM_REGS 19 */
/* OBSOLETE  */
/* OBSOLETE /* Initializer for an array of names of registers. */
/* OBSOLETE    There should be NUM_REGS strings in this initializer.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_NAMES {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "fp", "sp", "pc", "ps", "al", "ah"} */
/* OBSOLETE  */
/* OBSOLETE #define FP_REGNUM 13		/* Contains address of executing stack frame *x/ */
/* OBSOLETE #define SP_REGNUM 14		/* Contains address of top of stack *x/ */
/* OBSOLETE #define PC_REGNUM 15		/* Contains program counter *x/ */
/* OBSOLETE #define PS_REGNUM 16		/* Contains processor status *x/ */
/* OBSOLETE  */
/* OBSOLETE #define AL_REGNUM 17		/* Contains accumulator *x/ */
/* OBSOLETE #define AH_REGNUM 18 */
/* OBSOLETE  */
/* OBSOLETE /* Total amount of space needed to store our copies of the machine's */
/* OBSOLETE    register state, the array `registers'.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_BYTES (19*4) */
/* OBSOLETE  */
/* OBSOLETE /* Index within `registers' of the first byte of the space for */
/* OBSOLETE    register N.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_BYTE(N) ((N) * 4) */
/* OBSOLETE  */
/* OBSOLETE /* Number of bytes of storage in the actual machine representation */
/* OBSOLETE    for register N.  On the tahoe, all regs are 4 bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_RAW_SIZE(N) 4 */
/* OBSOLETE  */
/* OBSOLETE /* Number of bytes of storage in the program's representation */
/* OBSOLETE    for register N.  On the tahoe, all regs are 4 bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_VIRTUAL_SIZE(N) 4 */
/* OBSOLETE  */
/* OBSOLETE /* Largest value REGISTER_RAW_SIZE can have.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define MAX_REGISTER_RAW_SIZE 4 */
/* OBSOLETE  */
/* OBSOLETE /* Largest value REGISTER_VIRTUAL_SIZE can have.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define MAX_REGISTER_VIRTUAL_SIZE 4 */
/* OBSOLETE  */
/* OBSOLETE /* Return the GDB type object for the "standard" data type */
/* OBSOLETE    of data in register N.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_VIRTUAL_TYPE(N) builtin_type_int */
/* OBSOLETE  */
/* OBSOLETE /* Store the address of the place in which to copy the structure the */
/* OBSOLETE    subroutine will return.  This is called from call_function. *x/ */
/* OBSOLETE  */
/* OBSOLETE #define STORE_STRUCT_RETURN(ADDR, SP) \ */
/* OBSOLETE   { write_register (1, (ADDR)); } */
/* OBSOLETE  */
/* OBSOLETE /* Extract from an array REGBUF containing the (raw) register state */
/* OBSOLETE    a function return value of type TYPE, and copy that, in virtual format, */
/* OBSOLETE    into VALBUF.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \ */
/* OBSOLETE   memcpy (VALBUF, REGBUF, TYPE_LENGTH (TYPE)) */
/* OBSOLETE  */
/* OBSOLETE /* Write into appropriate registers a function return value */
/* OBSOLETE    of type TYPE, given in virtual format.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define STORE_RETURN_VALUE(TYPE,VALBUF) \ */
/* OBSOLETE   write_register_bytes (0, VALBUF, TYPE_LENGTH (TYPE)) */
/* OBSOLETE  */
/* OBSOLETE /* Extract from an array REGBUF containing the (raw) register state */
/* OBSOLETE    the address in which a function should return its structure value, */
/* OBSOLETE    as a CORE_ADDR (or an expression that can be used as one).  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) (*(int *)(REGBUF)) */
/* OBSOLETE  */
/* OBSOLETE /* Describe the pointer in each stack frame to the previous stack frame */
/* OBSOLETE    (its caller). */
/* OBSOLETE  */
/* OBSOLETE    FRAME_CHAIN takes a frame's nominal address */
/* OBSOLETE    and produces the frame's chain-pointer. *x/ */
/* OBSOLETE  */
/* OBSOLETE /* In the case of the Tahoe, the frame's nominal address is the FP value, */
/* OBSOLETE    and it points to the old FP *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_CHAIN(thisframe)  \ */
/* OBSOLETE   (!inside_entry_file ((thisframe)->pc) ? \ */
/* OBSOLETE    read_memory_integer ((thisframe)->frame, 4) :\ */
/* OBSOLETE    0) */
/* OBSOLETE  */
/* OBSOLETE /* Define other aspects of the stack frame.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Saved PC *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_SAVED_PC(FRAME) (read_memory_integer ((FRAME)->frame - 8, 4)) */
/* OBSOLETE  */
/* OBSOLETE /* In most of GDB, getting the args address is too important to */
/* OBSOLETE    just say "I don't know". *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_ARGS_ADDRESS(fi) ((fi)->frame) */
/* OBSOLETE  */
/* OBSOLETE /* Address to use as an anchor for finding local variables *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame) */
/* OBSOLETE  */
/* OBSOLETE /* Return number of args passed to a frame. */
/* OBSOLETE    Can return -1, meaning no way to tell.  *x/ */
/* OBSOLETE  */
/* OBSOLETE extern int tahoe_frame_num_args PARAMS ((struct frame_info * fi)); */
/* OBSOLETE #define FRAME_NUM_ARGS(fi) (tahoe_frame_num_args ((fi))) */
/* OBSOLETE  */
/* OBSOLETE /* Return number of bytes at start of arglist that are not really args.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_ARGS_SKIP 0 */
/* OBSOLETE  */
/* OBSOLETE /* Put here the code to store, into a struct frame_saved_regs, */
/* OBSOLETE    the addresses of the saved registers of frame described by FRAME_INFO. */
/* OBSOLETE    This includes special registers such as pc and fp saved in special */
/* OBSOLETE    ways in the stack frame.  sp is even more special: */
/* OBSOLETE    the address we return for it IS the sp for the next frame.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs) \ */
/* OBSOLETE { register int regnum;     \ */
/* OBSOLETE   register int rmask = read_memory_integer ((frame_info)->frame-4, 4) >> 16;\ */
/* OBSOLETE   register CORE_ADDR next_addr;     \ */
/* OBSOLETE   memset (&frame_saved_regs, '\0', sizeof frame_saved_regs);     \ */
/* OBSOLETE   next_addr = (frame_info)->frame - 8;     \ */
/* OBSOLETE   for (regnum = 12; regnum >= 0; regnum--, rmask <<= 1)  \ */
/* OBSOLETE     (frame_saved_regs).regs[regnum] = (rmask & 0x1000) ? (next_addr -= 4) : 0;\ */
/* OBSOLETE   (frame_saved_regs).regs[SP_REGNUM] = (frame_info)->frame + 4;  \ */
/* OBSOLETE   (frame_saved_regs).regs[PC_REGNUM] = (frame_info)->frame - 8;  \ */
/* OBSOLETE   (frame_saved_regs).regs[FP_REGNUM] = (frame_info)->frame;      \ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Things needed for making the inferior call functions.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Push an empty stack frame, to record the current PC, etc.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define PUSH_DUMMY_FRAME \ */
/* OBSOLETE { register CORE_ADDR sp = read_register (SP_REGNUM);	\ */
/* OBSOLETE   register int regnum;					\ */
/* OBSOLETE printf("PUSH_DUMMY_FRAME\n");				\ */
/* OBSOLETE   sp = push_word (sp, read_register (FP_REGNUM));	\ */
/* OBSOLETE   write_register (FP_REGNUM, sp);			\ */
/* OBSOLETE   sp = push_word (sp, 0x1fff0004);   /*SAVE MASK*x/	\ */
/* OBSOLETE   sp = push_word (sp, read_register (PC_REGNUM));	\ */
/* OBSOLETE   for (regnum = 12; regnum >= 0; regnum--)		\ */
/* OBSOLETE     sp = push_word (sp, read_register (regnum));	\ */
/* OBSOLETE   write_register (SP_REGNUM, sp);			\ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Discard from the stack the innermost frame, restoring all registers.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define POP_FRAME  \ */
/* OBSOLETE { register CORE_ADDR fp = read_register (FP_REGNUM);			\ */
/* OBSOLETE   register int regnum;							\ */
/* OBSOLETE   register int regmask = read_memory_integer (fp-4, 4);			\ */
/* OBSOLETE printf("POP_FRAME\n");							\ */
/* OBSOLETE   regmask >>= 16;                                               	\ */
/* OBSOLETE   write_register (SP_REGNUM, fp+4);	                           	\ */
/* OBSOLETE   write_register (PC_REGNUM, read_memory_integer(fp-8, 4));	  	\ */
/* OBSOLETE   write_register (FP_REGNUM, read_memory_integer(fp, 4));  		\ */
/* OBSOLETE   fp -= 8;								\ */
/* OBSOLETE   for (regnum = 12; regnum >= 0; regnum--, regmask <<= 1)		\ */
/* OBSOLETE     if (regmask & 0x1000)                                            	\ */
/* OBSOLETE       write_register (regnum, read_memory_integer (fp-=4, 4));		\ */
/* OBSOLETE   flush_cached_frames ();						\ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* This sequence of words is the instructions */
/* OBSOLETE    calls #69, @#32323232 */
/* OBSOLETE    bpt */
/* OBSOLETE    Note this is 8 bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define CALL_DUMMY {0xbf699f32, 0x32323230} */
/* OBSOLETE  */
/* OBSOLETE /* Start execution at beginning of dummy *x/ */
/* OBSOLETE  */
/* OBSOLETE #define CALL_DUMMY_START_OFFSET 0 */
/* OBSOLETE  */
/* OBSOLETE /* Insert the specified number of args and function address */
/* OBSOLETE    into a call sequence of the above form stored at DUMMYNAME.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, valtype, using_gcc) \ */
/* OBSOLETE { int temp = (int) fun;				\ */
/* OBSOLETE   *((char *) dummyname + 1) = nargs;		\ */
/* OBSOLETE   memcpy((char *)dummyname+3,&temp,4); } */
