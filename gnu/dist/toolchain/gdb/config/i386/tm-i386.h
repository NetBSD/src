/* Macro definitions for GDB on an Intel i[345]86.
   Copyright (C) 1995, 1996, 2000 Free Software Foundation, Inc.

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

#ifndef TM_I386_H
#define TM_I386_H 1

/* Forward decl's for prototypes */
struct frame_info;
struct frame_saved_regs;
struct type;

#define TARGET_BYTE_ORDER LITTLE_ENDIAN

/* The format used for `long double' on almost all i386 targets is the
   i387 extended floating-point format.  In fact, of all targets in the
   GCC 2.95 tree, only OSF/1 does it different, and insists on having
   a `long double' that's not `long' at all.  */

#define TARGET_LONG_DOUBLE_FORMAT &floatformat_i387_ext

/* Although the i386 extended floating-point has only 80 significant
   bits, a `long double' actually takes up 96, probably to enforce
   alignment.  */

#define TARGET_LONG_DOUBLE_BIT 96

/* Used for example in valprint.c:print_floating() to enable checking
   for NaN's */

#define IEEE_FLOAT

/* Number of traps that happen between exec'ing the shell to run an
   inferior, and when we finally get to the inferior code.  This is 2
   on most implementations. */

#define START_INFERIOR_TRAPS_EXPECTED 2

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Advance PC across any function entry prologue instructions to reach some
   "real" code.  */

#define SKIP_PROLOGUE(frompc)   (i386_skip_prologue (frompc))

extern int i386_skip_prologue PARAMS ((int));

/* Immediately after a function call, return the saved pc.  Can't always go
   through the frames for this because on some machines the new frame is not
   set up until the new function executes some instructions.  */

#define SAVED_PC_AFTER_CALL(frame) (read_memory_integer (read_register (SP_REGNUM), 4))

/* Stack grows downward.  */

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/* Sequence of bytes for breakpoint instruction.  */

#define BREAKPOINT {0xcc}

/* Amount PC must be decremented by after a breakpoint.  This is often the
   number of bytes in BREAKPOINT but not always. */

#define DECR_PC_AFTER_BREAK 1

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */

#define REGISTER_SIZE 4

/* This register file is parameterized by two macros:
   HAVE_I387_REGS --- register file should include i387 registers
   HAVE_SSE_REGS  --- register file should include SSE registers
   If HAVE_SSE_REGS is #defined, then HAVE_I387_REGS must also be #defined.
   
   However, GDB code should not test those macros with #ifdef, since
   that makes code which is annoying to multi-arch.  Instead, GDB code
   should check the values of NUM_GREGS, NUM_FREGS, and NUM_SSE_REGS,
   which will eventually get mapped onto architecture vector entries.

   It's okay to use the macros in tm-*.h files, though, since those
   files will get completely replaced when we multi-arch anyway.  */

/* Number of general registers, present on every 32-bit x86 variant.  */
#define NUM_GREGS (16)

/* Number of floating-point unit registers.  */
#ifdef HAVE_I387_REGS
#define NUM_FREGS (16)
#else
#define NUM_FREGS (0)
#endif

/* Number of SSE registers.  */
#ifdef HAVE_SSE_REGS
#define NUM_SSE_REGS (9)
#else
#define NUM_SSE_REGS (0)
#endif

#define NUM_REGS (NUM_GREGS + NUM_FREGS + NUM_SSE_REGS)

/* Largest number of registers we could have in any configuration.  */
#define MAX_NUM_REGS (16 + 16 + 9)

/* Initializer for an array of names of registers.  There should be at least
   NUM_REGS strings in this initializer.  Any excess ones are simply ignored.
   The order of the first 8 registers must match the compiler's numbering
   scheme (which is the same as the 386 scheme) and also regmap in the various
   *-nat.c files. */

#define REGISTER_NAMES { "eax",   "ecx",    "edx",   "ebx",	\
			 "esp",   "ebp",    "esi",   "edi",	\
			 "eip",   "eflags", "cs",    "ss",	\
			 "ds",    "es",     "fs",    "gs",	\
			 "st0",   "st1",    "st2",   "st3",	\
			 "st4",   "st5",    "st6",   "st7",	\
			 "fctrl", "fstat",  "ftag",  "fiseg",	\
                         "fioff", "foseg",  "fooff", "fop",	\
			 "xmm0",  "xmm1",   "xmm2",  "xmm3",	\
			 "xmm4",  "xmm5",   "xmm6",  "xmm7",	\
                         "mxcsr"				\
		       }

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define FP_REGNUM 5		/* (ebp) Contains address of executing stack
				   frame */
#define SP_REGNUM 4		/* (usp) Contains address of top of stack */
#define PC_REGNUM 8		/* (eip) Contains program counter */
#define PS_REGNUM 9		/* (ps)  Contains processor status */

/* These registers are present only if HAVE_I387_REGS is #defined.
   We promise that FP0 .. FP7 will always be consecutive register numbers.  */
#define FP0_REGNUM   16		/* first FPU floating-point register */
#define FP7_REGNUM   23		/* last  FPU floating-point register */

/* All of these control registers (except for FCOFF and FDOFF) are
   sixteen bits long (at most) in the FPU, but are zero-extended to
   thirty-two bits in GDB's register file.  This makes it easier to
   compute the size of the control register file, and somewhat easier
   to convert to and from the FSAVE instruction's 32-bit format.  */
#define FIRST_FPU_CTRL_REGNUM 24
#define FCTRL_REGNUM 24	        /* FPU control word */
#define FPC_REGNUM   24		/* old name for FCTRL_REGNUM */
#define FSTAT_REGNUM 25		/* FPU status word */
#define FTAG_REGNUM  26		/* FPU register tag word */
#define FCS_REGNUM   27		/* FPU instruction's code segment selector
				   16 bits, called "FPU Instruction Pointer
				   Selector" in the x86 manuals  */
#define FCOFF_REGNUM 28		/* FPU instruction's offset within segment
				   ("Fpu Code OFFset") */
#define FDS_REGNUM   29		/* FPU operand's data segment */
#define FDOFF_REGNUM 30		/* FPU operand's offset within segment */
#define FOP_REGNUM   31		/* FPU opcode, bottom eleven bits */
#define LAST_FPU_CTRL_REGNUM 31

/* These registers are present only if HAVE_SSE_REGS is #defined.
   We promise that XMM0 .. XMM7 will always have consecutive reg numbers. */
#define XMM0_REGNUM  32		/* first SSE data register */
#define XMM7_REGNUM  39		/* last  SSE data register */
#define MXCSR_REGNUM 40		/* Streaming SIMD Extension control/status */

#define IS_FP_REGNUM(n) (FP0_REGNUM <= (n) && (n) <= FP7_REGNUM)
#define IS_SSE_REGNUM(n) (XMM0_REGNUM <= (n) && (n) <= XMM7_REGNUM)

#define FPU_REG_RAW_SIZE (10)

/* Sizes of individual register sets.  These cover the entire register
   file, so summing up the sizes of those portions actually present
   yields REGISTER_BYTES.  */
#define SIZEOF_GREGS (NUM_GREGS * 4)
#define SIZEOF_FPU_REGS (8 * FPU_REG_RAW_SIZE)
#define SIZEOF_FPU_CTRL_REGS \
  ((LAST_FPU_CTRL_REGNUM - FIRST_FPU_CTRL_REGNUM + 1) * 4)
#define SIZEOF_SSE_REGS (8 * 16 + 4)


/* Total amount of space needed to store our copies of the machine's register
   state, the array `registers'. */
#ifdef HAVE_SSE_REGS
#define REGISTER_BYTES \
  (SIZEOF_GREGS + SIZEOF_FPU_REGS + SIZEOF_FPU_CTRL_REGS + SIZEOF_SSE_REGS)
#else
#ifdef HAVE_I387_REGS
#define REGISTER_BYTES (SIZEOF_GREGS + SIZEOF_FPU_REGS + SIZEOF_FPU_CTRL_REGS)
#else
#define REGISTER_BYTES (SIZEOF_GREGS)
#endif
#endif

/* Index within `registers' of the first byte of the space for register N. */
#define REGISTER_BYTE(n) (i386_register_byte[(n)])
extern int i386_register_byte[];

/* Number of bytes of storage in the actual machine representation for
   register N.  */
#define REGISTER_RAW_SIZE(n) (i386_register_raw_size[(n)])
extern int i386_register_raw_size[];

/* Largest value REGISTER_RAW_SIZE can have.  */
#define MAX_REGISTER_RAW_SIZE 16

/* Number of bytes of storage in the program's representation
   for register N. */
#define REGISTER_VIRTUAL_SIZE(n) (i386_register_virtual_size[(n)])
extern int i386_register_virtual_size[];

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */
#define MAX_REGISTER_VIRTUAL_SIZE 16

/* Return the GDB type object for the "standard" data type of data in 
   register N.  Perhaps si and di should go here, but potentially they
   could be used for things other than address.  */

#define REGISTER_VIRTUAL_TYPE(N)				\
  (((N) == PC_REGNUM || (N) == FP_REGNUM || (N) == SP_REGNUM)	\
   ? lookup_pointer_type (builtin_type_void)			\
   : IS_FP_REGNUM(N) ? builtin_type_long_double			\
   : IS_SSE_REGNUM(N) ? builtin_type_v4sf			\
   : builtin_type_int)

/* REGISTER_CONVERTIBLE(N) is true iff register N's virtual format is
   different from its raw format.  Note that this definition assumes
   that the host supports IEEE 32-bit floats, since it doesn't say
   that SSE registers need conversion.  Even if we can't find a
   counterexample, this is still sloppy.  */
#define REGISTER_CONVERTIBLE(n) (IS_FP_REGNUM (n))

/* Convert data from raw format for register REGNUM in buffer FROM to
   virtual format with type TYPE in buffer TO.  */

#define REGISTER_CONVERT_TO_VIRTUAL(regnum, type, from, to) \
  i386_register_convert_to_virtual ((regnum), (type), (from), (to));
extern void i386_register_convert_to_virtual (int regnum, struct type *type,
					      char *from, char *to);

/* Convert data from virtual format with type TYPE in buffer FROM to
   raw format for register REGNUM in buffer TO.  */

#define REGISTER_CONVERT_TO_RAW(type, regnum, from, to) \
  i386_register_convert_to_raw ((type), (regnum), (from), (to));
extern void i386_register_convert_to_raw (struct type *type, int regnum,
					  char *from, char *to);

/* Print out the i387 floating point state.  */
#ifdef HAVE_I387_REGS
extern void i387_float_info (void);
#define FLOAT_INFO { i387_float_info (); }
#endif


/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */

#define STORE_STRUCT_RETURN(ADDR, SP) \
  { char buf[REGISTER_SIZE];	\
    (SP) -= sizeof (ADDR);	\
    store_address (buf, sizeof (ADDR), ADDR);	\
    write_memory ((SP), buf, sizeof (ADDR)); }

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

#define EXTRACT_RETURN_VALUE(type, regbuf, valbuf) \
  i386_extract_return_value ((type), (regbuf), (valbuf))
extern void i386_extract_return_value (struct type *type, char *regbuf,
				       char *valbuf);

/* Write into appropriate registers a function return value of type TYPE, given
   in virtual format.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  {    	       	       	       	       	       	       	       	       	     \
    if (TYPE_CODE (TYPE) == TYPE_CODE_FLT)				     \
      write_register_bytes (REGISTER_BYTE (FP0_REGNUM), (VALBUF),	     \
			    TYPE_LENGTH (TYPE));			     \
    else								     \
      write_register_bytes (0, (VALBUF), TYPE_LENGTH (TYPE));  		     \
  }

/* Extract from an array REGBUF containing the (raw) register state the address
   in which a function should return its structure value, as a CORE_ADDR (or an
   expression that can be used as one).  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) (*(int *)(REGBUF))

/* The following redefines make backtracing through sigtramp work.
   They manufacture a fake sigtramp frame and obtain the saved pc in sigtramp
   from the sigcontext structure which is pushed by the kernel on the
   user stack, along with a pointer to it.  */

/* FRAME_CHAIN takes a frame's nominal address and produces the frame's
   chain-pointer.
   In the case of the i386, the frame's nominal address
   is the address of a 4-byte word containing the calling frame's address.  */

#define FRAME_CHAIN(thisframe)  \
  ((thisframe)->signal_handler_caller \
   ? (thisframe)->frame \
   : (!inside_entry_file ((thisframe)->pc) \
      ? read_memory_integer ((thisframe)->frame, 4) \
      : 0))

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */

#define FRAMELESS_FUNCTION_INVOCATION(FI) \
     (((FI)->signal_handler_caller) ? 0 : frameless_look_for_prologue(FI))

/* Saved Pc.  Get it from sigcontext if within sigtramp.  */

#define FRAME_SAVED_PC(FRAME) \
  (((FRAME)->signal_handler_caller \
    ? sigtramp_saved_pc (FRAME) \
    : read_memory_integer ((FRAME)->frame + 4, 4)) \
   )

extern CORE_ADDR sigtramp_saved_pc PARAMS ((struct frame_info *));

#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)

#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)

/* Return number of args passed to a frame.  Can return -1, meaning no way
   to tell, which is typical now that the C compiler delays popping them.  */

#define FRAME_NUM_ARGS(fi) (i386_frame_num_args(fi))

extern int i386_frame_num_args PARAMS ((struct frame_info *));

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 8

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

extern void i386_frame_init_saved_regs PARAMS ((struct frame_info *));
#define FRAME_INIT_SAVED_REGS(FI) i386_frame_init_saved_regs (FI)



/* Things needed for making the inferior call functions.  */

/* "An argument's size is increased, if necessary, to make it a
   multiple of [32 bit] words.  This may require tail padding,
   depending on the size of the argument" - from the x86 ABI.  */
#define PARM_BOUNDARY 32

/* Push an empty stack frame, to record the current PC, etc.  */

#define PUSH_DUMMY_FRAME { i386_push_dummy_frame (); }

extern void i386_push_dummy_frame PARAMS ((void));

/* Discard from the stack the innermost frame, restoring all registers.  */

#define POP_FRAME  { i386_pop_frame (); }

extern void i386_pop_frame PARAMS ((void));


/* this is 
 *   call 11223344 (32 bit relative)
 *   int3
 */

#define CALL_DUMMY { 0x223344e8, 0xcc11 }

#define CALL_DUMMY_LENGTH 8

#define CALL_DUMMY_START_OFFSET 0	/* Start execution at beginning of dummy */

#define CALL_DUMMY_BREAKPOINT_OFFSET 5

/* Insert the specified number of args and function address
   into a call sequence of the above form stored at DUMMYNAME.  */

#define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, type, gcc_p)   \
{ \
	int from, to, delta, loc; \
	loc = (int)(read_register (SP_REGNUM) - CALL_DUMMY_LENGTH); \
	from = loc + 5; \
	to = (int)(fun); \
	delta = to - from; \
	*((char *)(dummyname) + 1) = (delta & 0xff); \
	*((char *)(dummyname) + 2) = ((delta >> 8) & 0xff); \
	*((char *)(dummyname) + 3) = ((delta >> 16) & 0xff); \
	*((char *)(dummyname) + 4) = ((delta >> 24) & 0xff); \
}

extern void print_387_control_word PARAMS ((unsigned int));
extern void print_387_status_word PARAMS ((unsigned int));

/* Offset from SP to first arg on stack at first instruction of a function */

#define SP_ARG0 (1 * 4)

#endif /* ifndef TM_I386_H */
