/* Definitions to make GDB run on a mips box under 4.3bsd.
   Copyright 1986, 1987, 1989, 1991, 1992, 1993, 1994, 1995
   Free Software Foundation, Inc.
   Contributed by Per Bothner (bothner@cs.wisc.edu) at U.Wisconsin
   and by Alessandro Forin (af@cs.cmu.edu) at CMU..

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

#ifndef TM_MIPS_H
#define TM_MIPS_H 1

struct frame_info;
struct symbol;
struct type;
struct value;

#include <bfd.h>
#include "coff/sym.h"		/* Needed for PDR below.  */
#include "coff/symconst.h"

#if !defined (TARGET_BYTE_ORDER_DEFAULT)
#define TARGET_BYTE_ORDER_DEFAULT LITTLE_ENDIAN
#endif

#if !defined (GDB_TARGET_IS_MIPS64)
#define GDB_TARGET_IS_MIPS64 0
#endif

#if !defined (MIPS_EABI)
#define MIPS_EABI 0
#endif

#if !defined (TARGET_MONITOR_PROMPT)
#define TARGET_MONITOR_PROMPT "<IDT>"
#endif

/* PC should be masked to remove possible MIPS16 flag */
#if !defined (GDB_TARGET_MASK_DISAS_PC)
#define GDB_TARGET_MASK_DISAS_PC(addr) UNMAKE_MIPS16_ADDR(addr)
#endif
#if !defined (GDB_TARGET_UNMASK_DISAS_PC)
#define GDB_TARGET_UNMASK_DISAS_PC(addr) MAKE_MIPS16_ADDR(addr)
#endif

/* Floating point is IEEE compliant */
#define IEEE_FLOAT

/* The name of the usual type of MIPS processor that is in the target
   system.  */

#define DEFAULT_MIPS_TYPE "generic"

/* Remove useless bits from an instruction address.  */

#define ADDR_BITS_REMOVE(addr) mips_addr_bits_remove(addr)
CORE_ADDR mips_addr_bits_remove PARAMS ((CORE_ADDR addr));

/* Remove useless bits from the stack pointer.  */

#define TARGET_READ_SP() ADDR_BITS_REMOVE (read_register (SP_REGNUM))

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

#define SKIP_PROLOGUE(pc) (mips_skip_prologue (pc, 0))
extern CORE_ADDR mips_skip_prologue PARAMS ((CORE_ADDR addr, int lenient));

/* Return non-zero if PC points to an instruction which will cause a step
   to execute both the instruction at PC and an instruction at PC+4.  */
extern int mips_step_skips_delay PARAMS ((CORE_ADDR));
#define STEP_SKIPS_DELAY_P (1)
#define STEP_SKIPS_DELAY(pc) (mips_step_skips_delay (pc))

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#define SAVED_PC_AFTER_CALL(frame)	read_register(RA_REGNUM)

/* Are we currently handling a signal */

extern int in_sigtramp PARAMS ((CORE_ADDR, char *));
#define IN_SIGTRAMP(pc, name)	in_sigtramp(pc, name)

/* Stack grows downward.  */

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/* BREAKPOINT_FROM_PC uses the program counter value to determine whether a
   16- or 32-bit breakpoint should be used.  It returns a pointer
   to a string of bytes that encode a breakpoint instruction, stores
   the length of the string to *lenptr, and adjusts the pc (if necessary) to
   point to the actual memory location where the breakpoint should be
   inserted.  */

extern breakpoint_from_pc_fn mips_breakpoint_from_pc;
#define BREAKPOINT_FROM_PC(pcptr, lenptr) mips_breakpoint_from_pc(pcptr, lenptr)

/* Amount PC must be decremented by after a breakpoint.
   This is often the number of bytes in BREAKPOINT
   but not always.  */

#define DECR_PC_AFTER_BREAK 0

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */

#define REGISTER_SIZE 4

/* The size of a register.  This is predefined in tm-mips64.h.  We
   can't use REGISTER_SIZE because that is used for various other
   things.  */

#ifndef MIPS_REGSIZE
#define MIPS_REGSIZE 4
#endif

/* The sizes of floating point registers.  */

#define MIPS_FPU_SINGLE_REGSIZE 4
#define MIPS_FPU_DOUBLE_REGSIZE 8

/* Number of machine registers */

#ifndef NUM_REGS
#define NUM_REGS 90
#endif

/* Given the register index, return the name of the corresponding
   register. */
extern char *mips_register_name PARAMS ((int regnr));
#define REGISTER_NAME(i) mips_register_name (i)

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

#ifndef MIPS_REGISTER_NAMES
#define MIPS_REGISTER_NAMES 	\
    {	"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3", \
	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7", \
	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7", \
	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra", \
	"sr",	"lo",	"hi",	"bad",	"cause","pc",    \
	"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7", \
	"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15", \
	"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",\
	"f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",\
	"fsr",  "fir",  "fp",	"", \
	"",	"",	"",	"",	"",	"",	"",	"", \
	"",	"",	"",	"",	"",	"",	"",	"", \
    }
#endif

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define ZERO_REGNUM 0		/* read-only register, always 0 */
#define V0_REGNUM 2		/* Function integer return value */
#define A0_REGNUM 4		/* Loc of first arg during a subr call */
#if MIPS_EABI
#define MIPS_LAST_ARG_REGNUM 11	/* EABI uses R4 through R11 for args */
#define MIPS_NUM_ARG_REGS 8
#else
#define MIPS_LAST_ARG_REGNUM 7	/* old ABI uses R4 through R7 for args */
#define MIPS_NUM_ARG_REGS 4
#endif
#define T9_REGNUM 25		/* Contains address of callee in PIC */
#define SP_REGNUM 29		/* Contains address of top of stack */
#define RA_REGNUM 31		/* Contains return address value */
#define PS_REGNUM 32		/* Contains processor status */
#define HI_REGNUM 34		/* Multiple/divide temp */
#define LO_REGNUM 33		/* ... */
#define BADVADDR_REGNUM 35	/* bad vaddr for addressing exception */
#define CAUSE_REGNUM 36		/* describes last exception */
#define PC_REGNUM 37		/* Contains program counter */
#define FP0_REGNUM 38		/* Floating point register 0 (single float) */
#define FPA0_REGNUM (FP0_REGNUM+12)	/* First float argument register */
#if MIPS_EABI			/* EABI uses F12 through F19 for args */
#define MIPS_LAST_FP_ARG_REGNUM (FP0_REGNUM+19)
#define MIPS_NUM_FP_ARG_REGS 8
#else /* old ABI uses F12 through F15 for args */
#define MIPS_LAST_FP_ARG_REGNUM (FP0_REGNUM+15)
#define MIPS_NUM_FP_ARG_REGS 4
#endif
#define FCRCS_REGNUM 70		/* FP control/status */
#define FCRIR_REGNUM 71		/* FP implementation/revision */
#define FP_REGNUM 72		/* Pseudo register that contains true address of executing stack frame */
#define	UNUSED_REGNUM 73	/* Never used, FIXME */
#define	FIRST_EMBED_REGNUM 74	/* First CP0 register for embedded use */
#define	PRID_REGNUM 89		/* Processor ID */
#define	LAST_EMBED_REGNUM 89	/* Last one */

/* Define DO_REGISTERS_INFO() to do machine-specific formatting
   of register dumps. */

#define DO_REGISTERS_INFO(_regnum, fp) mips_do_registers_info(_regnum, fp)
extern void mips_do_registers_info PARAMS ((int, int));

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */

#define REGISTER_BYTES (NUM_REGS*MIPS_REGSIZE)

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N) ((N) * MIPS_REGSIZE)

/* Number of bytes of storage in the actual machine representation for
   register N.  NOTE: This indirectly defines the register size
   transfered by the GDB protocol. */

extern int mips_register_raw_size PARAMS ((int reg_nr));
#define REGISTER_RAW_SIZE(N) (mips_register_raw_size ((N)))


/* Covert between the RAW and VIRTUAL registers.

   Some MIPS (SR, FSR, FIR) have a `raw' size of MIPS_REGSIZE but are
   really 32 bit registers.  This is a legacy of the 64 bit MIPS GDB
   protocol which transfers 64 bits for 32 bit registers. */

extern int mips_register_convertible PARAMS ((int reg_nr));
#define REGISTER_CONVERTIBLE(N) (mips_register_convertible ((N)))
     

void mips_register_convert_to_virtual PARAMS ((int reg_nr, struct type *virtual_type, char *raw_buf, char *virt_buf));
#define REGISTER_CONVERT_TO_VIRTUAL(N,VIRTUAL_TYPE,RAW_BUF,VIRT_BUF) \
  mips_register_convert_to_virtual (N,VIRTUAL_TYPE,RAW_BUF,VIRT_BUF)

void mips_register_convert_to_raw PARAMS ((struct type *virtual_type, int reg_nr, char *virt_buf, char *raw_buf));
#define REGISTER_CONVERT_TO_RAW(VIRTUAL_TYPE,N,VIRT_BUF,RAW_BUF) \
  mips_register_convert_to_raw (VIRTUAL_TYPE,N,VIRT_BUF,RAW_BUF)

/* Number of bytes of storage in the program's representation
   for register N. */

#define REGISTER_VIRTUAL_SIZE(N) TYPE_LENGTH (REGISTER_VIRTUAL_TYPE (N))

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 8

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 8

/* Return the GDB type object for the "standard" data type of data in
   register N.  */

#ifndef REGISTER_VIRTUAL_TYPE
#define REGISTER_VIRTUAL_TYPE(N) \
	(((N) >= FP0_REGNUM && (N) < FP0_REGNUM+32) ? builtin_type_float \
	 : ((N) == 32 /*SR*/) ? builtin_type_uint32 \
	 : ((N) >= 70 && (N) <= 89) ? builtin_type_uint32 \
	 : builtin_type_int)
#endif

/* All mips targets store doubles in a register pair with the least
   significant register in the lower numbered register.
   If the target is big endian, double register values need conversion
   between memory and register formats.  */

#define REGISTER_CONVERT_TO_TYPE(n, type, buffer)			\
  do {if (TARGET_BYTE_ORDER == BIG_ENDIAN				\
	  && REGISTER_RAW_SIZE (n) == 4					\
	  && (n) >= FP0_REGNUM && (n) < FP0_REGNUM + 32			\
	  && TYPE_CODE(type) == TYPE_CODE_FLT				\
	  && TYPE_LENGTH(type) == 8) {					\
        char __temp[4];							\
	memcpy (__temp, ((char *)(buffer))+4, 4);			\
	memcpy (((char *)(buffer))+4, (buffer), 4); 			\
	memcpy (((char *)(buffer)), __temp, 4); }} while (0)

#define REGISTER_CONVERT_FROM_TYPE(n, type, buffer)			\
  do {if (TARGET_BYTE_ORDER == BIG_ENDIAN				\
	  && REGISTER_RAW_SIZE (n) == 4					\
	  && (n) >= FP0_REGNUM && (n) < FP0_REGNUM + 32			\
	  && TYPE_CODE(type) == TYPE_CODE_FLT				\
	  && TYPE_LENGTH(type) == 8) {					\
        char __temp[4];							\
	memcpy (__temp, ((char *)(buffer))+4, 4);			\
	memcpy (((char *)(buffer))+4, (buffer), 4); 			\
	memcpy (((char *)(buffer)), __temp, 4); }} while (0)

/* Store the address of the place in which to copy the structure the
   subroutine will return.  Handled by mips_push_arguments.  */

#define STORE_STRUCT_RETURN(addr, sp)
/**/

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  XXX floats */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
  mips_extract_return_value(TYPE, REGBUF, VALBUF)
extern void
mips_extract_return_value PARAMS ((struct type *, char[], char *));

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  mips_store_return_value(TYPE, VALBUF)
extern void mips_store_return_value PARAMS ((struct type *, char *));

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */
/* The address is passed in a0 upon entry to the function, but when
   the function exits, the compiler has copied the value to v0.  This
   convention is specified by the System V ABI, so I think we can rely
   on it.  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  (extract_address (REGBUF + REGISTER_BYTE (V0_REGNUM), \
		    REGISTER_RAW_SIZE (V0_REGNUM)))

extern use_struct_convention_fn mips_use_struct_convention;
#define USE_STRUCT_CONVENTION(gcc_p, type) mips_use_struct_convention (gcc_p, type)

/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer. */

#define FRAME_CHAIN(thisframe) (CORE_ADDR) mips_frame_chain (thisframe)
extern CORE_ADDR mips_frame_chain PARAMS ((struct frame_info *));

/* Define other aspects of the stack frame.  */


/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */
/* We handle this differently for mips, and maybe we should not */

#define FRAMELESS_FUNCTION_INVOCATION(FI)  (0)

/* Saved Pc.  */

#define FRAME_SAVED_PC(FRAME)	(mips_frame_saved_pc(FRAME))
extern CORE_ADDR mips_frame_saved_pc PARAMS ((struct frame_info *));

#define FRAME_ARGS_ADDRESS(fi)	(fi)->frame

#define FRAME_LOCALS_ADDRESS(fi) (fi)->frame

/* Return number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

#define FRAME_NUM_ARGS(fi)	(mips_frame_num_args(fi))
extern int mips_frame_num_args PARAMS ((struct frame_info *));

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

#define FRAME_INIT_SAVED_REGS(frame_info) \
  do { \
    if ((frame_info)->saved_regs == NULL) \
      mips_find_saved_regs (frame_info); \
    (frame_info)->saved_regs[SP_REGNUM] = (frame_info)->frame; \
  } while (0)
extern void mips_find_saved_regs PARAMS ((struct frame_info *));


/* Things needed for making the inferior call functions.  */

/* Stack must be aligned on 32-bit boundaries when synthesizing
   function calls.  We don't need STACK_ALIGN, PUSH_ARGUMENTS will
   handle it. */

extern CORE_ADDR mips_push_arguments PARAMS ((int, struct value **, CORE_ADDR, int, CORE_ADDR));
#define PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \
  (mips_push_arguments((nargs), (args), (sp), (struct_return), (struct_addr)))

extern CORE_ADDR mips_push_return_address PARAMS ((CORE_ADDR pc, CORE_ADDR sp));
#define PUSH_RETURN_ADDRESS(PC, SP) (mips_push_return_address ((PC), (SP)))

/* Push an empty stack frame, to record the current PC, etc.  */

#define PUSH_DUMMY_FRAME 	mips_push_dummy_frame()
extern void mips_push_dummy_frame PARAMS ((void));

/* Discard from the stack the innermost frame, restoring all registers.  */

#define POP_FRAME		mips_pop_frame()
extern void mips_pop_frame PARAMS ((void));

#if !GDB_MULTI_ARCH
#define CALL_DUMMY { 0 }
#endif

#define CALL_DUMMY_START_OFFSET (0)

#define CALL_DUMMY_BREAKPOINT_OFFSET (0)

/* On Irix, $t9 ($25) contains the address of the callee (used for PIC).
   It doesn't hurt to do this on other systems; $t9 will be ignored.  */
#define FIX_CALL_DUMMY(dummyname, start_sp, fun, nargs, args, rettype, gcc_p) \
    write_register(T9_REGNUM, fun)

#define CALL_DUMMY_LOCATION AT_ENTRY_POINT

#define CALL_DUMMY_ADDRESS() (mips_call_dummy_address ())
extern CORE_ADDR mips_call_dummy_address PARAMS ((void));

/* There's a mess in stack frame creation.  See comments in blockframe.c
   near reference to INIT_FRAME_PC_FIRST.  */

#define	INIT_FRAME_PC(fromleaf, prev)	/* nada */

#define INIT_FRAME_PC_FIRST(fromleaf, prev) \
   mips_init_frame_pc_first(fromleaf, prev)
extern void mips_init_frame_pc_first PARAMS ((int, struct frame_info *));

/* Special symbol found in blocks associated with routines.  We can hang
   mips_extra_func_info_t's off of this.  */

#define MIPS_EFI_SYMBOL_NAME "__GDB_EFI_INFO__"
extern void ecoff_relocate_efi PARAMS ((struct symbol *, CORE_ADDR));

/* Specific information about a procedure.
   This overlays the MIPS's PDR records, 
   mipsread.c (ab)uses this to save memory */

typedef struct mips_extra_func_info
  {
    long numargs;		/* number of args to procedure (was iopt) */
    bfd_vma high_addr;		/* upper address bound */
    long frame_adjust;		/* offset of FP from SP (used on MIPS16) */
    PDR pdr;			/* Procedure descriptor record */
  }
 *mips_extra_func_info_t;

extern void mips_init_extra_frame_info PARAMS ((int fromleaf, struct frame_info *));
#define INIT_EXTRA_FRAME_INFO(fromleaf, fci) \
  mips_init_extra_frame_info(fromleaf, fci)

extern void mips_print_extra_frame_info PARAMS ((struct frame_info * frame));
#define	PRINT_EXTRA_FRAME_INFO(fi) \
  mips_print_extra_frame_info (fi)

/* It takes two values to specify a frame on the MIPS.

   In fact, the *PC* is the primary value that sets up a frame.  The
   PC is looked up to see what function it's in; symbol information
   from that function tells us which register is the frame pointer
   base, and what offset from there is the "virtual frame pointer".
   (This is usually an offset from SP.)  On most non-MIPS machines,
   the primary value is the SP, and the PC, if needed, disambiguates
   multiple functions with the same SP.  But on the MIPS we can't do
   that since the PC is not stored in the same part of the frame every
   time.  This does not seem to be a very clever way to set up frames,
   but there is nothing we can do about that).  */

#define SETUP_ARBITRARY_FRAME(argc, argv) setup_arbitrary_frame (argc, argv)
extern struct frame_info *setup_arbitrary_frame PARAMS ((int, CORE_ADDR *));

/* Convert a dbx stab register number (from `r' declaration) to a gdb REGNUM */

#define STAB_REG_TO_REGNUM(num) ((num) < 32 ? (num) : (num)+FP0_REGNUM-38)

/* Convert a ecoff register number to a gdb REGNUM */

#define ECOFF_REG_TO_REGNUM(num) ((num) < 32 ? (num) : (num)+FP0_REGNUM-32)

#if !GDB_MULTI_ARCH
/* If the current gcc for for this target does not produce correct debugging
   information for float parameters, both prototyped and unprototyped, then
   define this macro.  This forces gdb to  always assume that floats are
   passed as doubles and then converted in the callee.

   For the mips chip, it appears that the debug info marks the parameters as
   floats regardless of whether the function is prototyped, but the actual
   values are passed as doubles for the non-prototyped case and floats for
   the prototyped case.  Thus we choose to make the non-prototyped case work
   for C and break the prototyped case, since the non-prototyped case is
   probably much more common.  (FIXME). */

#define COERCE_FLOAT_TO_DOUBLE(formal, actual) (current_language -> la_language == language_c)
#endif

/* Select the default mips disassembler */

#define TM_PRINT_INSN_MACH 0


/* These are defined in mdebugread.c and are used in mips-tdep.c  */
extern CORE_ADDR sigtramp_address, sigtramp_end;
extern void fixup_sigtramp PARAMS ((void));

/* Defined in mips-tdep.c and used in remote-mips.c */
extern char *mips_read_processor_type PARAMS ((void));

/* Functions for dealing with MIPS16 call and return stubs.  */
#define IN_SOLIB_CALL_TRAMPOLINE(pc, name)	mips_in_call_stub (pc, name)
#define IN_SOLIB_RETURN_TRAMPOLINE(pc, name)	mips_in_return_stub (pc, name)
#define SKIP_TRAMPOLINE_CODE(pc)		mips_skip_stub (pc)
#define IGNORE_HELPER_CALL(pc)			mips_ignore_helper (pc)
extern int mips_in_call_stub PARAMS ((CORE_ADDR pc, char *name));
extern int mips_in_return_stub PARAMS ((CORE_ADDR pc, char *name));
extern CORE_ADDR mips_skip_stub PARAMS ((CORE_ADDR pc));
extern int mips_ignore_helper PARAMS ((CORE_ADDR pc));

#ifndef TARGET_MIPS
#define TARGET_MIPS
#endif

/* Definitions and declarations used by mips-tdep.c and remote-mips.c  */
#define MIPS_INSTLEN 4		/* Length of an instruction */
#define MIPS16_INSTLEN 2	/* Length of an instruction on MIPS16 */
#define MIPS_NUMREGS 32		/* Number of integer or float registers */
typedef unsigned long t_inst;	/* Integer big enough to hold an instruction */

/* MIPS16 function addresses are odd (bit 0 is set).  Here are some
   macros to test, set, or clear bit 0 of addresses.  */
#define IS_MIPS16_ADDR(addr)	 ((addr) & 1)
#define MAKE_MIPS16_ADDR(addr)	 ((addr) | 1)
#define UNMAKE_MIPS16_ADDR(addr) ((addr) & ~1)

#endif /* TM_MIPS_H */

/* Macros for setting and testing a bit in a minimal symbol that
   marks it as 16-bit function.  The MSB of the minimal symbol's
   "info" field is used for this purpose. This field is already
   being used to store the symbol size, so the assumption is
   that the symbol size cannot exceed 2^31.

   ELF_MAKE_MSYMBOL_SPECIAL
   tests whether an ELF symbol is "special", i.e. refers
   to a 16-bit function, and sets a "special" bit in a 
   minimal symbol to mark it as a 16-bit function
   MSYMBOL_IS_SPECIAL   tests the "special" bit in a minimal symbol
   MSYMBOL_SIZE         returns the size of the minimal symbol, i.e.
   the "info" field with the "special" bit masked out
 */

#define ELF_MAKE_MSYMBOL_SPECIAL(sym,msym) \
 { \
  if (((elf_symbol_type *)(sym))->internal_elf_sym.st_other == STO_MIPS16) { \
    MSYMBOL_INFO (msym) = (char *) (((long) MSYMBOL_INFO (msym)) | 0x80000000); \
    SYMBOL_VALUE_ADDRESS (msym) |= 1; \
  } \
 }

#define MSYMBOL_IS_SPECIAL(msym) \
  (((long) MSYMBOL_INFO (msym) & 0x80000000) != 0)
#define MSYMBOL_SIZE(msym) \
  ((long) MSYMBOL_INFO (msym) & 0x7fffffff)


/* Command to set the processor type. */
extern void mips_set_processor_type_command (char *, int);
