/* Definitions to target GDB to ARM targets.
   Copyright 1986, 1987, 1988, 1989, 1991, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.

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

#ifndef TM_ARM_H
#define TM_ARM_H

/* Forward declarations for prototypes.  */
struct type;
struct value;

/* Target byte order on ARM defaults to selectable, and defaults to
   little endian.  */
#define TARGET_BYTE_ORDER_SELECTABLE_P	1
#define TARGET_BYTE_ORDER_DEFAULT	LITTLE_ENDIAN

/* IEEE format floating point.  */
#define IEEE_FLOAT
#define TARGET_DOUBLE_FORMAT  (target_byte_order == BIG_ENDIAN \
			       ? &floatformat_ieee_double_big	 \
			       : &floatformat_ieee_double_littlebyte_bigword)

/* When reading symbols, we need to zap the low bit of the address,
   which may be set to 1 for Thumb functions.  */

#define SMASH_TEXT_ADDRESS(addr) ((addr) &= ~0x1)

/* Remove useless bits from addresses in a running program.  */

CORE_ADDR arm_addr_bits_remove (CORE_ADDR);

#define ADDR_BITS_REMOVE(val)	(arm_addr_bits_remove (val))

/* Offset from address of function to start of its code.  Zero on most
   machines.  */

#define FUNCTION_START_OFFSET	0

/* Advance PC across any function entry prologue instructions to reach
   some "real" code.  */

extern CORE_ADDR arm_skip_prologue (CORE_ADDR pc);

#define SKIP_PROLOGUE(pc)  (arm_skip_prologue (pc))

/* Immediately after a function call, return the saved pc.  Can't
   always go through the frames for this because on some machines the
   new frame is not set up until the new function executes some
   instructions.  */

#define SAVED_PC_AFTER_CALL(frame)  arm_saved_pc_after_call (frame)
struct frame_info;
extern CORE_ADDR arm_saved_pc_after_call (struct frame_info *);

/* The following define instruction sequences that will cause ARM
   cpu's to take an undefined instruction trap.  These are used to
   signal a breakpoint to GDB.
   
   The newer ARMv4T cpu's are capable of operating in ARM or Thumb
   modes.  A different instruction is required for each mode.  The ARM
   cpu's can also be big or little endian.  Thus four different
   instructions are needed to support all cases.
   
   Note: ARMv4 defines several new instructions that will take the
   undefined instruction trap.  ARM7TDMI is nominally ARMv4T, but does
   not in fact add the new instructions.  The new undefined
   instructions in ARMv4 are all instructions that had no defined
   behaviour in earlier chips.  There is no guarantee that they will
   raise an exception, but may be treated as NOP's.  In practice, it
   may only safe to rely on instructions matching:
   
   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 
   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   C C C C 0 1 1 x x x x x x x x x x x x x x x x x x x x 1 x x x x
   
   Even this may only true if the condition predicate is true. The
   following use a condition predicate of ALWAYS so it is always TRUE.
   
   There are other ways of forcing a breakpoint.  ARM Linux, RisciX,
   and I suspect NetBSD will all use a software interrupt rather than
   an undefined instruction to force a trap.  This can be handled by
   redefining some or all of the following in a target dependent
   fashion.  */

#define ARM_LE_BREAKPOINT {0xFE,0xDE,0xFF,0xE7}
#define ARM_BE_BREAKPOINT {0xE7,0xFF,0xDE,0xFE}
#define THUMB_LE_BREAKPOINT {0xfe,0xdf}
#define THUMB_BE_BREAKPOINT {0xdf,0xfe}

/* Stack grows downward.  */

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/* !!!! if we're using RDP, then we're inserting breakpoints and
   storing their handles instread of what was in memory.  It is nice
   that this is the same size as a handle - otherwise remote-rdp will
   have to change. */

/* BREAKPOINT_FROM_PC uses the program counter value to determine
   whether a 16- or 32-bit breakpoint should be used.  It returns a
   pointer to a string of bytes that encode a breakpoint instruction,
   stores the length of the string to *lenptr, and adjusts the pc (if
   necessary) to point to the actual memory location where the
   breakpoint should be inserted.  */

extern breakpoint_from_pc_fn arm_breakpoint_from_pc;
#define BREAKPOINT_FROM_PC(pcptr, lenptr) arm_breakpoint_from_pc (pcptr, lenptr)

/* Amount PC must be decremented by after a breakpoint.  This is often
   the number of bytes in BREAKPOINT but not always.  */

#define DECR_PC_AFTER_BREAK 0

/* Code to execute to print interesting information about the floating
   point processor (if any) or emulator.  No need to define if there
   is nothing to do. */
extern void arm_float_info (void);

#define FLOAT_INFO	{ arm_float_info (); }

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */

#define REGISTER_SIZE	4

/* Say how long FP registers are.  Used for documentation purposes and
   code readability in this header.  IEEE extended doubles are 80
   bits.  DWORD aligned they use 96 bits.  */
#define FP_REGISTER_RAW_SIZE	12

/* GCC doesn't support long doubles (extended IEEE values).  The FP
   register virtual size is therefore 64 bits.  Used for documentation
   purposes and code readability in this header.  */
#define FP_REGISTER_VIRTUAL_SIZE	8

/* Status registers are the same size as general purpose registers.
   Used for documentation purposes and code readability in this
   header.  */
#define STATUS_REGISTER_SIZE	REGISTER_SIZE

/* Number of machine registers.  The only define actually required 
   is NUM_REGS.  The other definitions are used for documentation
   purposes and code readability.  */
/* For 26 bit ARM code, a fake copy of the PC is placed in register 25 (PS)
   (and called PS for processor status) so the status bits can be cleared
   from the PC (register 15).  For 32 bit ARM code, a copy of CPSR is placed
   in PS.  */
#define NUM_FREGS	8	/* Number of floating point registers.  */
#define NUM_SREGS	2	/* Number of status registers.  */
#define NUM_GREGS	16	/* Number of general purpose registers.  */
#define NUM_REGS	(NUM_GREGS + NUM_FREGS + NUM_SREGS)

/* An array of names of registers. */
extern char **arm_register_names;

#define REGISTER_NAME(i) arm_register_names[i]

/* Register numbers of various important registers.  Note that some of
   these values are "real" register numbers, and correspond to the
   general registers of the machine, and some are "phony" register
   numbers which are too large to be actual register numbers as far as
   the user is concerned but do serve to get the desired values when
   passed to read_register.  */

#define A1_REGNUM 0		/* first integer-like argument */
#define A4_REGNUM 3		/* last integer-like argument */
#define AP_REGNUM 11
#define FP_REGNUM 11		/* Contains address of executing stack frame */
#define SP_REGNUM 13		/* Contains address of top of stack */
#define LR_REGNUM 14		/* address to return to from a function call */
#define PC_REGNUM 15		/* Contains program counter */
#define F0_REGNUM 16		/* first floating point register */
#define F3_REGNUM 19		/* last floating point argument register */
#define F7_REGNUM 23		/* last floating point register */
#define FPS_REGNUM 24		/* floating point status register */
#define PS_REGNUM 25		/* Contains processor status */

#define THUMB_FP_REGNUM 7	/* R7 is frame register on Thumb */

#define ARM_NUM_ARG_REGS 	4
#define ARM_LAST_ARG_REGNUM 	A4_REGNUM
#define ARM_NUM_FP_ARG_REGS 	4
#define ARM_LAST_FP_ARG_REGNUM	F3_REGNUM

/* Instruction condition field values.  */
#define INST_EQ		0x0
#define INST_NE		0x1
#define INST_CS		0x2
#define INST_CC		0x3
#define INST_MI		0x4
#define INST_PL		0x5
#define INST_VS		0x6
#define INST_VC		0x7
#define INST_HI		0x8
#define INST_LS		0x9
#define INST_GE		0xa
#define INST_LT		0xb
#define INST_GT		0xc
#define INST_LE		0xd
#define INST_AL		0xe
#define INST_NV		0xf

#define FLAG_N		0x80000000
#define FLAG_Z		0x40000000
#define FLAG_C		0x20000000
#define FLAG_V		0x10000000



/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */

#define REGISTER_BYTES ((NUM_GREGS * REGISTER_SIZE) + \
			(NUM_FREGS * FP_REGISTER_RAW_SIZE) + \
			(NUM_SREGS * STATUS_REGISTER_SIZE))

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N) \
     ((N) < F0_REGNUM \
      ? (N) * REGISTER_SIZE \
      : ((N) < PS_REGNUM \
	 ? (NUM_GREGS * REGISTER_SIZE + \
	    ((N) - F0_REGNUM) * FP_REGISTER_RAW_SIZE) \
	 : (NUM_GREGS * REGISTER_SIZE + \
	    NUM_FREGS * FP_REGISTER_RAW_SIZE + \
	    ((N) - FPS_REGNUM) * STATUS_REGISTER_SIZE)))

/* Number of bytes of storage in the actual machine representation for
   register N.  All registers are 4 bytes, except fp0 - fp7, which are
   12 bytes in length.  */
#define REGISTER_RAW_SIZE(N) \
     ((N) < F0_REGNUM ? REGISTER_SIZE : \
      (N) < FPS_REGNUM ? FP_REGISTER_RAW_SIZE : STATUS_REGISTER_SIZE)

/* Number of bytes of storage in a program's representation
   for register N.  */
#define REGISTER_VIRTUAL_SIZE(N) \
	((N) < F0_REGNUM ? REGISTER_SIZE : \
	 (N) < FPS_REGNUM ? FP_REGISTER_VIRTUAL_SIZE : STATUS_REGISTER_SIZE)

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE FP_REGISTER_RAW_SIZE

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */
#define MAX_REGISTER_VIRTUAL_SIZE FP_REGISTER_VIRTUAL_SIZE

/* Nonzero if register N requires conversion from raw format to
   virtual format. */
extern int arm_register_convertible (unsigned int);
#define REGISTER_CONVERTIBLE(REGNUM) (arm_register_convertible (REGNUM))

/* Convert data from raw format for register REGNUM in buffer FROM to
   virtual format with type TYPE in buffer TO. */

extern void arm_register_convert_to_virtual (unsigned int regnum,
					     struct type *type,
					     void *from, void *to);
#define REGISTER_CONVERT_TO_VIRTUAL(REGNUM,TYPE,FROM,TO) \
     arm_register_convert_to_virtual (REGNUM, TYPE, FROM, TO)

/* Convert data from virtual format with type TYPE in buffer FROM to
   raw format for register REGNUM in buffer TO.  */

extern void arm_register_convert_to_raw (unsigned int regnum,
					 struct type *type,
					 void *from, void *to);
#define REGISTER_CONVERT_TO_RAW(TYPE,REGNUM,FROM,TO) \
     arm_register_convert_to_raw (REGNUM, TYPE, FROM, TO)

/* Return the GDB type object for the "standard" data type of data in
   register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
     (((unsigned)(N) - F0_REGNUM) < NUM_FREGS \
      ? builtin_type_double : builtin_type_int)

/* The system C compiler uses a similar structure return convention to gcc */
extern use_struct_convention_fn arm_use_struct_convention;
#define USE_STRUCT_CONVENTION(gcc_p, type) \
     arm_use_struct_convention (gcc_p, type)

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */

#define STORE_STRUCT_RETURN(ADDR, SP) \
     write_register (A1_REGNUM, (ADDR))

/* Extract from an array REGBUF containing the (raw) register state a
   function return value of type TYPE, and copy that, in virtual
   format, into VALBUF.  */

extern void arm_extract_return_value (struct type *, char[], char *);
#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
     arm_extract_return_value ((TYPE), (REGBUF), (VALBUF))

/* Write into appropriate registers a function return value of type
   TYPE, given in virtual format.  */

extern void convert_to_extended (void *dbl, void *ptr);
#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  if (TYPE_CODE (TYPE) == TYPE_CODE_FLT) {				\
    char _buf[MAX_REGISTER_RAW_SIZE];					\
    convert_to_extended (VALBUF, _buf); 					\
    write_register_bytes (REGISTER_BYTE (F0_REGNUM), _buf, MAX_REGISTER_RAW_SIZE); \
  } else								\
    write_register_bytes (0, VALBUF, TYPE_LENGTH (TYPE))

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  (extract_address ((PTR)(REGBUF), REGISTER_RAW_SIZE(0)))

/* Specify that for the native compiler variables for a particular
   lexical context are listed after the beginning LBRAC instead of
   before in the executables list of symbols.  */
#define VARIABLES_INSIDE_BLOCK(desc, gcc_p) (!(gcc_p))


/* Define other aspects of the stack frame.  We keep the offsets of
   all saved registers, 'cause we need 'em a lot!  We also keep the
   current size of the stack frame, and the offset of the frame
   pointer from the stack pointer (for frameless functions, and when
   we're still in the prologue of a function with a frame) */

#define EXTRA_FRAME_INFO  	\
  struct frame_saved_regs fsr;	\
  int framesize;		\
  int frameoffset;		\
  int framereg;

extern void arm_init_extra_frame_info (int fromleaf, struct frame_info * fi);
#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) \
	arm_init_extra_frame_info ((fromleaf), (fi))

/* Return the frame address.  On ARM, it is R11; on Thumb it is R7.  */
CORE_ADDR arm_target_read_fp (void);
#define TARGET_READ_FP() arm_target_read_fp ()

/* Describe the pointer in each stack frame to the previous stack
   frame (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address and produces the
   frame's chain-pointer.

   However, if FRAME_CHAIN_VALID returns zero,
   it means the given frame is the outermost one and has no caller.  */

#define FRAME_CHAIN(thisframe) arm_frame_chain (thisframe)
extern CORE_ADDR arm_frame_chain (struct frame_info *);

extern int arm_frame_chain_valid (CORE_ADDR, struct frame_info *);
#define FRAME_CHAIN_VALID(chain, thisframe) \
     arm_frame_chain_valid (chain, thisframe)

/* Define other aspects of the stack frame.  */

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.

   Sometimes we have functions that do a little setup (like saving the
   vN registers with the stmdb instruction, but DO NOT set up a frame.
   The symbol table will report this as a prologue.  However, it is
   important not to try to parse these partial frames as frames, or we
   will get really confused.

   So I will demand 3 instructions between the start & end of the
   prologue before I call it a real prologue, i.e. at least
         mov ip, sp,
	 stmdb sp!, {}
	 sub sp, ip, #4. */

extern int arm_frameless_function_invocation (struct frame_info *fi);
#define FRAMELESS_FUNCTION_INVOCATION(FI) \
(arm_frameless_function_invocation (FI))
    
/* Saved Pc.  */

#define FRAME_SAVED_PC(FRAME)	arm_frame_saved_pc (FRAME)
extern CORE_ADDR arm_frame_saved_pc (struct frame_info *);

#define FRAME_ARGS_ADDRESS(fi) (fi->frame)

#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)

/* Return number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

#define FRAME_NUM_ARGS(fi) (-1)

/* Return number of bytes at start of arglist that are not really args. */

#define FRAME_ARGS_SKIP 0

/* Put here the code to store, into a struct frame_saved_regs, the
   addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special: the address we
   return for it IS the sp for the next frame.  */

struct frame_saved_regs;
struct frame_info;
void arm_frame_find_saved_regs (struct frame_info * fi,
				struct frame_saved_regs * fsr);

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs) \
	arm_frame_find_saved_regs (frame_info, &(frame_saved_regs));

/* Things needed for making the inferior call functions.  */

#define PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \
     sp = arm_push_arguments ((nargs), (args), (sp), (struct_return), (struct_addr))
extern CORE_ADDR arm_push_arguments (int, struct value **, CORE_ADDR, int,
				     CORE_ADDR);

/* Push an empty stack frame, to record the current PC, etc.  */

void arm_push_dummy_frame (void);

#define PUSH_DUMMY_FRAME arm_push_dummy_frame ()

/* Discard from the stack the innermost frame, restoring all registers.  */

void arm_pop_frame (void);

#define POP_FRAME arm_pop_frame ()

/* This sequence of words is the instructions

   mov  lr,pc
   mov  pc,r4
   illegal

   Note this is 12 bytes.  */

#define CALL_DUMMY {0xe1a0e00f, 0xe1a0f004, 0xe7ffdefe}
#define CALL_DUMMY_START_OFFSET	 0	/* Start execution at beginning of dummy */

#define CALL_DUMMY_BREAKPOINT_OFFSET arm_call_dummy_breakpoint_offset()
extern int arm_call_dummy_breakpoint_offset (void);

/* Insert the specified number of args and function address into a
   call sequence of the above form stored at DUMMYNAME.  */

#define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, type, gcc_p) \
   arm_fix_call_dummy ((dummyname), (pc), (fun), (nargs), (args), (type), (gcc_p))

void arm_fix_call_dummy (char *dummy, CORE_ADDR pc, CORE_ADDR fun,
			 int nargs, struct value ** args,
			 struct type * type, int gcc_p);

CORE_ADDR arm_get_next_pc (CORE_ADDR pc);

/* Macros for setting and testing a bit in a minimal symbol that marks
   it as Thumb function.  The MSB of the minimal symbol's "info" field
   is used for this purpose. This field is already being used to store
   the symbol size, so the assumption is that the symbol size cannot
   exceed 2^31.

   COFF_MAKE_MSYMBOL_SPECIAL
   ELF_MAKE_MSYMBOL_SPECIAL
   
   These macros test whether the COFF or ELF symbol corresponds to a
   thumb function, and set a "special" bit in a minimal symbol to
   indicate that it does.
   
   MSYMBOL_SET_SPECIAL	Actually sets the "special" bit.
   MSYMBOL_IS_SPECIAL   Tests the "special" bit in a minimal symbol.
   MSYMBOL_SIZE         Returns the size of the minimal symbol,
   			i.e. the "info" field with the "special" bit
   			masked out 
   */

extern int coff_sym_is_thumb (int val);

#define MSYMBOL_SET_SPECIAL(msym) \
	MSYMBOL_INFO (msym) = (char *) (((long) MSYMBOL_INFO (msym)) | 0x80000000)
#define MSYMBOL_IS_SPECIAL(msym) \
  (((long) MSYMBOL_INFO (msym) & 0x80000000) != 0)
#define MSYMBOL_SIZE(msym) \
  ((long) MSYMBOL_INFO (msym) & 0x7fffffff)

/* Thumb symbols are of type STT_LOPROC, (synonymous with STT_ARM_TFUNC) */
#define ELF_MAKE_MSYMBOL_SPECIAL(sym,msym) \
	{ if(ELF_ST_TYPE(((elf_symbol_type *)(sym))->internal_elf_sym.st_info) == STT_LOPROC) \
		MSYMBOL_SET_SPECIAL(msym); }

#define COFF_MAKE_MSYMBOL_SPECIAL(val,msym) \
 { if(coff_sym_is_thumb(val)) MSYMBOL_SET_SPECIAL(msym); }

/* The first 0x20 bytes are the trap vectors.  */
#define LOWEST_PC	0x20

#endif /* TM_ARM_H */
