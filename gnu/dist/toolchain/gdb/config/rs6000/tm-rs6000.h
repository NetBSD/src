/* Parameters for target execution on an RS6000, for GDB, the GNU debugger.
   Copyright 1986, 1987, 1989, 1991, 1992, 1993, 1994, 1997
   Free Software Foundation, Inc.
   Contributed by IBM Corporation.

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

/* Forward decls for prototypes */
struct frame_info;
struct type;
struct value;

/* Minimum possible text address in AIX */

#define TEXT_SEGMENT_BASE	0x10000000

/* Load segment of a given pc value. */

#define	PC_LOAD_SEGMENT(PC)	pc_load_segment_name(PC)
extern char *pc_load_segment_name PARAMS ((CORE_ADDR));

/* AIX cc seems to get this right.  */

#define BELIEVE_PCC_PROMOTION 1

/* return true if a given `pc' value is in `call dummy' function. */
/* FIXME: This just checks for the end of the stack, which is broken
   for things like stepping through gcc nested function stubs.  */
#define	PC_IN_CALL_DUMMY(STOP_PC, STOP_SP, STOP_FRAME_ADDR)	\
	(STOP_SP < STOP_PC && STOP_PC < STACK_END_ADDR)

#if 0
extern unsigned int text_start, data_start;
extern char *corefile;
#endif
extern int inferior_pid;

/* We are missing register descriptions in the system header files. Sigh! */

struct regs
  {
    int gregs[32];		/* general purpose registers */
    int pc;			/* program conter       */
    int ps;			/* processor status, or machine state */
  };

struct fp_status
  {
    double fpregs[32];		/* floating GP registers */
  };


/* To be used by skip_prologue. */

struct rs6000_framedata
  {
    int offset;			/* total size of frame --- the distance
				   by which we decrement sp to allocate
				   the frame */
    int saved_gpr;		/* smallest # of saved gpr */
    int saved_fpr;		/* smallest # of saved fpr */
    int alloca_reg;		/* alloca register number (frame ptr) */
    char frameless;		/* true if frameless functions. */
    char nosavedpc;		/* true if pc not saved. */
    int gpr_offset;		/* offset of saved gprs from prev sp */
    int fpr_offset;		/* offset of saved fprs from prev sp */
    int lr_offset;		/* offset of saved lr */
    int cr_offset;		/* offset of saved cr */
  };

/* Define the byte order of the machine.  */

#define TARGET_BYTE_ORDER_DEFAULT	BIG_ENDIAN

/* AIX's assembler doesn't grok dollar signs in identifiers.
   So we use dots instead.  This item must be coordinated with G++. */
#undef CPLUS_MARKER
#define CPLUS_MARKER '.'

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

extern CORE_ADDR rs6000_skip_prologue PARAMS ((CORE_ADDR));
#define SKIP_PROLOGUE(pc) (rs6000_skip_prologue (pc))

extern CORE_ADDR skip_prologue PARAMS ((CORE_ADDR, struct rs6000_framedata *));


/* If PC is in some function-call trampoline code, return the PC
   where the function itself actually starts.  If not, return NULL.  */

#define	SKIP_TRAMPOLINE_CODE(pc)	skip_trampoline_code (pc)
extern CORE_ADDR skip_trampoline_code PARAMS ((CORE_ADDR));

/* Number of trap signals we need to skip over, once the inferior process
   starts running. */

#define	START_INFERIOR_TRAPS_EXPECTED	2

/* AIX has a couple of strange returns from wait().  */

#define CHILD_SPECIAL_WAITSTATUS(ourstatus, hoststatus) ( \
  /* "stop after load" status.  */ \
  (hoststatus) == 0x57c ? (ourstatus)->kind = TARGET_WAITKIND_LOADED, 1 : \
  \
  /* signal 0. I have no idea why wait(2) returns with this status word.  */ \
  /* It looks harmless. */ \
  (hoststatus) == 0x7f ? (ourstatus)->kind = TARGET_WAITKIND_SPURIOUS, 1 : \
  \
  /* A normal waitstatus.  Let the usual macros deal with it.  */ \
  0)

/* In xcoff, we cannot process line numbers when we see them. This is
   mainly because we don't know the boundaries of the include files. So,
   we postpone that, and then enter and sort(?) the whole line table at
   once, when we are closing the current symbol table in end_symtab(). */

#define	PROCESS_LINENUMBER_HOOK()	aix_process_linenos ()
extern void aix_process_linenos PARAMS ((void));

/* Immediately after a function call, return the saved pc.
   Can't go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#define	SAVED_PC_AFTER_CALL(frame) read_register (LR_REGNUM)

/* Address of end of stack space.  */

#define STACK_END_ADDR 0x2ff80000

/* Stack grows downward.  */

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/* This is how arguments pushed onto stack or passed in registers.
   Stack must be aligned on 64-bit boundaries when synthesizing
   function calls.  We don't need STACK_ALIGN, PUSH_ARGUMENTS will
   handle it. */

#define	PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \
  (rs6000_push_arguments((nargs), (args), (sp), (struct_return), (struct_addr)))
extern CORE_ADDR rs6000_push_arguments PARAMS ((int, struct value **, CORE_ADDR, int, CORE_ADDR));

/* BREAKPOINT_FROM_PC uses the program counter value to determine the
   breakpoint that should be used */
extern breakpoint_from_pc_fn rs6000_breakpoint_from_pc;
#define BREAKPOINT_FROM_PC(pcptr, lenptr) rs6000_breakpoint_from_pc (pcptr, lenptr)

/* Amount PC must be decremented by after a breakpoint.
   This is often the number of bytes in BREAKPOINT
   but not always.  */

#define DECR_PC_AFTER_BREAK 0

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */
#define REGISTER_SIZE 4


/* Return the name of register number REG.  This may return "" to
   indicate a register number that's not used on this variant.
   (Register numbers may be sparse for consistency between variants.)  */
#define REGISTER_NAME(reg) (rs6000_register_name(reg))
extern char *rs6000_register_name (int reg);

/* Number of machine registers */
#define NUM_REGS 183

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define FP_REGNUM 1		/* Contains address of executing stack frame */
#define SP_REGNUM 1		/* Contains address of top of stack */
#define	TOC_REGNUM 2		/* TOC register */
#define FP0_REGNUM 32		/* Floating point register 0 */
#define	GP0_REGNUM 0		/* GPR register 0 */
#define FP0_REGNUM 32		/* FPR (Floating point) register 0 */
#define FPLAST_REGNUM 63	/* Last floating point register */

/* Special purpose registers... */
/* P.S. keep these in the same order as in /usr/mstsave.h `mstsave'
   structure, for easier processing */

#define PC_REGNUM 64		/* Program counter (instruction address %iar) */
#define PS_REGNUM 65		/* Processor (or machine) status (%msr) */
#define	CR_REGNUM 66		/* Condition register */
#define	LR_REGNUM 67		/* Link register */
#define	CTR_REGNUM 68		/* Count register */
#define	XER_REGNUM 69		/* Fixed point exception registers */
#define	MQ_REGNUM 70		/* Multiply/quotient register */

/* These #defines are used to parse core files and talk to ptrace, so they
   must remain fixed.  */
#define	FIRST_UISA_SP_REGNUM 64	/* first special register number */
#define LAST_UISA_SP_REGNUM  70	/* last special register number */

/* This is the offset in REG_NAMES at which the `set processor'
   command starts plugging in its names.  */
#define FIRST_VARIANT_REGISTER 66

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.
   32 4-byte gpr's
   32 8-byte fpr's
   7  4-byte UISA special purpose registers, 
   16 4-byte segment registers,
   32 4-byte standard OEA special-purpose registers,
   and up to 64 4-byte non-standard OEA special purpose regs.
   total: (+ (* 32 4) (* 32 8) (* 7 4) (* 16 4) (* 32 4) (* 64 4)) 860 bytes
   Keep some extra space for now, in case to add more. */
#define REGISTER_BYTES 880


/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N)  \
 (								\
  ((N) > FPLAST_REGNUM) ? ((((N) - FPLAST_REGNUM -1) * 4) + 384)\
  :((N) >= FP0_REGNUM) ? ((((N) - FP0_REGNUM) * 8) + 128)	\
  :((N) * 4) )

/* Number of bytes of storage in the actual machine representation
   for register N. */
/* Note that the unsigned cast here forces the result of the
   subtraction to very high positive values if N < FP0_REGNUM */

#define REGISTER_RAW_SIZE(N) (((unsigned)(N) - FP0_REGNUM) < 32 ? 8 : 4)

/* Number of bytes of storage in the program's representation
   for register N.  On the RS6000, all regs are 4 bytes
   except the floating point regs which are 8-byte doubles.  */

#define REGISTER_VIRTUAL_SIZE(N) (((unsigned)(N) - FP0_REGNUM) < 32 ? 8 : 4)

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 8

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 8

/* convert a dbx stab register number (from `r' declaration) to a gdb REGNUM */

#define STAB_REG_TO_REGNUM(value)	(value)

/* Nonzero if register N requires conversion
   from raw format to virtual format.
   The register format for rs6000 floating point registers is always
   double, we need a conversion if the memory format is float.  */

#define REGISTER_CONVERTIBLE(N) ((N) >= FP0_REGNUM && (N) <= FPLAST_REGNUM)

/* Convert data from raw format for register REGNUM in buffer FROM
   to virtual format with type TYPE in buffer TO.  */

#define REGISTER_CONVERT_TO_VIRTUAL(REGNUM,TYPE,FROM,TO) \
{ \
  if (TYPE_LENGTH (TYPE) != REGISTER_RAW_SIZE (REGNUM)) \
    { \
      double val = extract_floating ((FROM), REGISTER_RAW_SIZE (REGNUM)); \
      store_floating ((TO), TYPE_LENGTH (TYPE), val); \
    } \
  else \
    memcpy ((TO), (FROM), REGISTER_RAW_SIZE (REGNUM)); \
}

/* Convert data from virtual format with type TYPE in buffer FROM
   to raw format for register REGNUM in buffer TO.  */

#define REGISTER_CONVERT_TO_RAW(TYPE,REGNUM,FROM,TO)	\
{ \
  if (TYPE_LENGTH (TYPE) != REGISTER_RAW_SIZE (REGNUM)) \
    { \
      double val = extract_floating ((FROM), TYPE_LENGTH (TYPE)); \
      store_floating ((TO), REGISTER_RAW_SIZE (REGNUM), val); \
    } \
  else \
    memcpy ((TO), (FROM), REGISTER_RAW_SIZE (REGNUM)); \
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
 (((unsigned)(N) - FP0_REGNUM) < 32 ? builtin_type_double : builtin_type_int)

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */
/* in RS6000, struct return addresses are passed as an extra parameter in r3.
   In function return, callee is not responsible of returning this address back.
   Since gdb needs to find it, we will store in a designated variable
   `rs6000_struct_return_address'. */

extern CORE_ADDR rs6000_struct_return_address;

#define STORE_STRUCT_RETURN(ADDR, SP)	\
  { write_register (3, (ADDR));		\
    rs6000_struct_return_address = (ADDR); }

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

/* #define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
   memcpy (VALBUF, REGBUF, TYPE_LENGTH (TYPE)) */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
  extract_return_value(TYPE,REGBUF,VALBUF)
extern void extract_return_value PARAMS ((struct type *, char[], char *));

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  {									\
    if (TYPE_CODE (TYPE) == TYPE_CODE_FLT)				\
									\
     /* Floating point values are returned starting from FPR1 and up.	\
	Say a double_double_double type could be returned in		\
	FPR1/FPR2/FPR3 triple. */					\
									\
      write_register_bytes (REGISTER_BYTE (FP0_REGNUM+1), (VALBUF),	\
						TYPE_LENGTH (TYPE));	\
    else								\
      /* Everything else is returned in GPR3 and up. */			\
      write_register_bytes (REGISTER_BYTE (GP0_REGNUM+3), (VALBUF),	\
						TYPE_LENGTH (TYPE));	\
  }


/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF)	rs6000_struct_return_address

/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer. */

/* In the case of the RS6000, the frame's nominal address
   is the address of a 4-byte word containing the calling frame's address.  */

#define FRAME_CHAIN(thisframe) rs6000_frame_chain (thisframe)
CORE_ADDR rs6000_frame_chain PARAMS ((struct frame_info *));

/* Define other aspects of the stack frame.  */

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */

extern int rs6000_frameless_function_invocation (struct frame_info *);
#define FRAMELESS_FUNCTION_INVOCATION(FI) \
  (rs6000_frameless_function_invocation (FI))

#define INIT_FRAME_PC_FIRST(fromleaf, prev) \
  prev->pc = (fromleaf ? SAVED_PC_AFTER_CALL (prev->next) : \
	      prev->next ? FRAME_SAVED_PC (prev->next) : read_pc ());
#define INIT_FRAME_PC(fromleaf, prev)	/* nothing */
extern void rs6000_init_extra_frame_info (int fromleaf, struct frame_info *);
#define	INIT_EXTRA_FRAME_INFO(fromleaf, fi) rs6000_init_extra_frame_info (fromleaf, fi)

/* If the kernel has to deliver a signal, it pushes a sigcontext
   structure on the stack and then calls the signal handler, passing
   the address of the sigcontext in an argument register. Usually
   the signal handler doesn't save this register, so we have to
   access the sigcontext structure via an offset from the signal handler
   frame.
   The following constants were determined by experimentation on AIX 3.2.  */
#define SIG_FRAME_PC_OFFSET 96
#define SIG_FRAME_LR_OFFSET 108
#define SIG_FRAME_FP_OFFSET 284

/* Default offset from SP where the LR is stored */
#define	DEFAULT_LR_SAVE 8

/* Return saved PC from a frame */
#define FRAME_SAVED_PC(FRAME)  rs6000_frame_saved_pc (FRAME)

extern unsigned long rs6000_frame_saved_pc (struct frame_info *);

extern CORE_ADDR rs6000_frame_args_address PARAMS ((struct frame_info *));
#define FRAME_ARGS_ADDRESS(FI) rs6000_frame_args_address (FI)

#define FRAME_LOCALS_ADDRESS(FI)	FRAME_ARGS_ADDRESS(FI)


/* Set VAL to the number of args passed to frame described by FI.
   Can set VAL to -1, meaning no way to tell.  */

/* We can't tell how many args there are
   now that the C compiler delays popping them.  */

#define FRAME_NUM_ARGS(fi) (-1)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 8	/* Not sure on this. FIXMEmgo */

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */
/* In the following implementation for RS6000, we did *not* save sp. I am
   not sure if it will be needed. The following macro takes care of gpr's
   and fpr's only. */

extern void rs6000_frame_init_saved_regs PARAMS ((struct frame_info *));
#define FRAME_INIT_SAVED_REGS(FI) rs6000_frame_init_saved_regs (FI)

/* Things needed for making the inferior call functions.  */

/* Push an empty stack frame, to record the current PC, etc.  */
/* Change these names into rs6k_{push, pop}_frame(). FIXMEmgo. */

#define PUSH_DUMMY_FRAME	push_dummy_frame ()
extern void push_dummy_frame PARAMS ((void));

/* Discard from the stack the innermost frame, 
   restoring all saved registers.  */

#define POP_FRAME	pop_frame ()
extern void pop_frame PARAMS ((void));

/* This sequence of words is the instructions:

   mflr r0              // 0x7c0802a6
   // save fpr's
   stfd r?, num(r1)     // 0xd8010000 there should be 32 of this??
   // save gpr's
   stm  r0, num(r1)     // 0xbc010000
   stu  r1, num(r1)     // 0x94210000

   // the function we want to branch might be in a different load 
   // segment. reset the toc register. Note that the actual toc address
   // will be fix by fix_call_dummy () along with function address.

   st   r2, 0x14(r1)    // 0x90410014 save toc register
   liu  r2, 0x1234      // 0x3c401234 reset a new toc value 0x12345678
   oril r2, r2,0x5678   // 0x60425678   

   // load absolute address 0x12345678 to r0
   liu  r0, 0x1234      // 0x3c001234
   oril r0, r0,0x5678   // 0x60005678
   mtctr        r0              // 0x7c0903a6 ctr <- r0
   bctrl                        // 0x4e800421 jump subroutine 0x12345678 (%ctr)
   cror 0xf, 0xf, 0xf   // 0x4def7b82
   brpt                 // 0x7d821008, breakpoint
   cror 0xf, 0xf, 0xf   // 0x4def7b82 (for 8 byte alignment)


   We actually start executing by saving the toc register first, since the pushing 
   of the registers is done by PUSH_DUMMY_FRAME.  If this were real code,
   the arguments for the function called by the `bctrl' would be pushed
   between the `stu' and the `bctrl', and we could allow it to execute through.
   But the arguments have to be pushed by GDB after the PUSH_DUMMY_FRAME is done,
   and we cannot allow to push the registers again.
 */

#define CALL_DUMMY {0x7c0802a6, 0xd8010000, 0xbc010000, 0x94210000, \
		    0x90410014, 0x3c401234, 0x60425678,		    \
		    0x3c001234, 0x60005678, 0x7c0903a6, 0x4e800421, \
		    0x4def7b82, 0x7d821008, 0x4def7b82 }


/* keep this as multiple of 8 (%sp requires 8 byte alignment) */
#define CALL_DUMMY_LENGTH 56

#define CALL_DUMMY_START_OFFSET 16

/* Insert the specified number of args and function address into a
   call sequence of the above form stored at DUMMYNAME.  */

#define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, type, gcc_p) \
  rs6000_fix_call_dummy (dummyname, pc, fun, nargs, args, type, gcc_p)
extern void rs6000_fix_call_dummy PARAMS ((char *, CORE_ADDR, CORE_ADDR,
					   int, struct value **,
					   struct type *, int));

/* Hook in rs6000-tdep.c for determining the TOC address when
   calling functions in the inferior.  */
extern
CORE_ADDR (*find_toc_address_hook) PARAMS ((CORE_ADDR));

/* xcoffread.c provides a function to determine the TOC offset
   for a given object file.
   It is used under native AIX configurations for determining the
   TOC address when calling functions in the inferior.  */

struct objfile;
extern CORE_ADDR get_toc_offset PARAMS ((struct objfile *));

/* Usually a function pointer's representation is simply the address
   of the function. On the RS/6000 however, a function pointer is
   represented by a pointer to a TOC entry. This TOC entry contains
   three words, the first word is the address of the function, the
   second word is the TOC pointer (r2), and the third word is the
   static chain value.  Throughout GDB it is currently assumed that a
   function pointer contains the address of the function, which is not
   easy to fix.  In addition, the conversion of a function address to
   a function pointer would require allocation of a TOC entry in the
   inferior's memory space, with all its drawbacks.  To be able to
   call C++ virtual methods in the inferior (which are called via
   function pointers), find_function_addr uses this macro to get the
   function address from a function pointer.  */

#define CONVERT_FROM_FUNC_PTR_ADDR(ADDR) \
  (is_magic_function_pointer (ADDR) ? read_memory_integer (ADDR, 4) : (ADDR))
     extern int is_magic_function_pointer PARAMS ((CORE_ADDR));

/* Flag for machine-specific stuff in shared files.  FIXME */
#define IBM6000_TARGET

/* RS6000/AIX does not support PT_STEP.  Has to be simulated.  */

#define SOFTWARE_SINGLE_STEP_P 1
     extern void rs6000_software_single_step PARAMS ((unsigned int, int));
#define SOFTWARE_SINGLE_STEP(sig,bp_p) rs6000_software_single_step (sig, bp_p)

/* If the current gcc for for this target does not produce correct debugging
   information for float parameters, both prototyped and unprototyped, then
   define this macro.  This forces gdb to  always assume that floats are
   passed as doubles and then converted in the callee.

   For the PowerPC, it appears that the debug info marks the parameters as
   floats regardless of whether the function is prototyped, but the actual
   values are always passed in as doubles.  Thus by setting this to 1, both
   types of calls will work. */

#define COERCE_FLOAT_TO_DOUBLE(formal, actual) (1)
