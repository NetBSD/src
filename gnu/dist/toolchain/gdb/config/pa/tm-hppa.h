/* Parameters for execution on any Hewlett-Packard PA-RISC machine.
   Copyright 1986, 1987, 1989-1993, 1995, 1999, 2000 Free Software Foundation, Inc. 

   Contributed by the Center for Software Science at the
   University of Utah (pa-gdb-bugs@cs.utah.edu).

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

/* Forward declarations of some types we use in prototypes */

struct frame_info;
struct frame_saved_regs;
struct value;
struct type;
struct inferior_status;

/* Target system byte order. */

#define	TARGET_BYTE_ORDER	BIG_ENDIAN

/* By default assume we don't have to worry about software floating point.  */
#ifndef SOFT_FLOAT
#define SOFT_FLOAT 0
#endif

/* Get at various relevent fields of an instruction word. */

#define MASK_5 0x1f
#define MASK_11 0x7ff
#define MASK_14 0x3fff
#define MASK_21 0x1fffff

/* This macro gets bit fields using HP's numbering (MSB = 0) */
#ifndef GET_FIELD
#define GET_FIELD(X, FROM, TO) \
  ((X) >> (31 - (TO)) & ((1 << ((TO) - (FROM) + 1)) - 1))
#endif

/* Watch out for NaNs */

#define IEEE_FLOAT

/* On the PA, any pass-by-value structure > 8 bytes is actually
   passed via a pointer regardless of its type or the compiler
   used.  */

#define REG_STRUCT_HAS_ADDR(gcc_p,type) \
  (TYPE_LENGTH (type) > 8)

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

extern CORE_ADDR hppa_skip_prologue PARAMS ((CORE_ADDR));
#define SKIP_PROLOGUE(pc) (hppa_skip_prologue (pc))

/* If PC is in some function-call trampoline code, return the PC
   where the function itself actually starts.  If not, return NULL.  */

#define	SKIP_TRAMPOLINE_CODE(pc) skip_trampoline_code (pc, NULL)
extern CORE_ADDR skip_trampoline_code PARAMS ((CORE_ADDR, char *));

/* Return non-zero if we are in an appropriate trampoline. */

#define IN_SOLIB_CALL_TRAMPOLINE(pc, name) \
   in_solib_call_trampoline (pc, name)
extern int in_solib_call_trampoline PARAMS ((CORE_ADDR, char *));

#define IN_SOLIB_RETURN_TRAMPOLINE(pc, name) \
  in_solib_return_trampoline (pc, name)
extern int in_solib_return_trampoline PARAMS ((CORE_ADDR, char *));

/* Immediately after a function call, return the saved pc.
   Can't go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#undef	SAVED_PC_AFTER_CALL
#define SAVED_PC_AFTER_CALL(frame) saved_pc_after_call (frame)
extern CORE_ADDR saved_pc_after_call PARAMS ((struct frame_info *));

/* Stack grows upward */
#define INNER_THAN(lhs,rhs) ((lhs) > (rhs))

/* elz: adjust the quantity to the next highest value which is 64-bit aligned.
   This is used in valops.c, when the sp is adjusted.
   On hppa the sp must always be kept 64-bit aligned */

#define STACK_ALIGN(arg) ( ((arg)%8) ? (((arg)+7)&-8) : (arg))
#define NO_EXTRA_ALIGNMENT_NEEDED 1

/* Sequence of bytes for breakpoint instruction.  */

#define BREAKPOINT {0x00, 0x01, 0x00, 0x04}
#define BREAKPOINT32 0x10004

/* Amount PC must be decremented by after a breakpoint.
   This is often the number of bytes in BREAKPOINT
   but not always.

   Not on the PA-RISC */

#define DECR_PC_AFTER_BREAK 0

/* Sometimes we may pluck out a minimal symbol that has a negative
   address.

   An example of this occurs when an a.out is linked against a foo.sl.
   The foo.sl defines a global bar(), and the a.out declares a signature
   for bar().  However, the a.out doesn't directly call bar(), but passes
   its address in another call.

   If you have this scenario and attempt to "break bar" before running,
   gdb will find a minimal symbol for bar() in the a.out.  But that
   symbol's address will be negative.  What this appears to denote is
   an index backwards from the base of the procedure linkage table (PLT)
   into the data linkage table (DLT), the end of which is contiguous
   with the start of the PLT.  This is clearly not a valid address for
   us to set a breakpoint on.

   Note that one must be careful in how one checks for a negative address.
   0xc0000000 is a legitimate address of something in a shared text
   segment, for example.  Since I don't know what the possible range
   is of these "really, truly negative" addresses that come from the
   minimal symbols, I'm resorting to the gross hack of checking the
   top byte of the address for all 1's.  Sigh.
 */
#define PC_REQUIRES_RUN_BEFORE_USE(pc) \
  (! target_has_stack && (pc & 0xFF000000))

/* return instruction is bv r0(rp) or bv,n r0(rp) */

#define ABOUT_TO_RETURN(pc) ((read_memory_integer (pc, 4) | 0x2) == 0xE840C002)

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */

#define REGISTER_SIZE 4

/* Number of machine registers */

#define NUM_REGS 128

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.
   They are in rows of eight entries  */

#define REGISTER_NAMES	\
 {"flags",  "r1",      "rp",      "r3",    "r4",     "r5",      "r6",     "r7",    \
  "r8",     "r9",      "r10",     "r11",   "r12",    "r13",     "r14",    "r15",   \
  "r16",    "r17",     "r18",     "r19",   "r20",    "r21",     "r22",    "r23",   \
  "r24",    "r25",     "r26",     "dp",    "ret0",   "ret1",    "sp",     "r31",   \
  "sar",    "pcoqh",   "pcsqh",   "pcoqt", "pcsqt",  "eiem",    "iir",    "isr",   \
  "ior",    "ipsw",    "goto",    "sr4",   "sr0",    "sr1",     "sr2",    "sr3",   \
  "sr5",    "sr6",     "sr7",     "cr0",   "cr8",    "cr9",     "ccr",    "cr12",  \
  "cr13",   "cr24",    "cr25",    "cr26",  "mpsfu_high","mpsfu_low","mpsfu_ovflo","pad",\
  "fpsr",    "fpe1",   "fpe2",    "fpe3",  "fpe4",   "fpe5",    "fpe6",   "fpe7",  \
  "fr4",     "fr4R",   "fr5",     "fr5R",  "fr6",    "fr6R",    "fr7",    "fr7R",  \
  "fr8",     "fr8R",   "fr9",     "fr9R",  "fr10",   "fr10R",   "fr11",   "fr11R", \
  "fr12",    "fr12R",  "fr13",    "fr13R", "fr14",   "fr14R",   "fr15",   "fr15R", \
  "fr16",    "fr16R",  "fr17",    "fr17R", "fr18",   "fr18R",   "fr19",   "fr19R", \
  "fr20",    "fr20R",  "fr21",    "fr21R", "fr22",   "fr22R",   "fr23",   "fr23R", \
  "fr24",    "fr24R",  "fr25",    "fr25R", "fr26",   "fr26R",   "fr27",   "fr27R", \
  "fr28",    "fr28R",  "fr29",    "fr29R", "fr30",   "fr30R",   "fr31",   "fr31R"}

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define R0_REGNUM 0		/* Doesn't actually exist, used as base for
				   other r registers.  */
#define FLAGS_REGNUM 0		/* Various status flags */
#define RP_REGNUM 2		/* return pointer */
#define FP_REGNUM 3		/* Contains address of executing stack */
				/* frame */
#define SP_REGNUM 30		/* Contains address of top of stack */
#define SAR_REGNUM 32		/* Shift Amount Register */
#define IPSW_REGNUM 41		/* Interrupt Processor Status Word */
#define PCOQ_HEAD_REGNUM 33	/* instruction offset queue head */
#define PCSQ_HEAD_REGNUM 34	/* instruction space queue head */
#define PCOQ_TAIL_REGNUM 35	/* instruction offset queue tail */
#define PCSQ_TAIL_REGNUM 36	/* instruction space queue tail */
#define EIEM_REGNUM 37		/* External Interrupt Enable Mask */
#define IIR_REGNUM 38		/* Interrupt Instruction Register */
#define IOR_REGNUM 40		/* Interrupt Offset Register */
#define SR4_REGNUM 43		/* space register 4 */
#define RCR_REGNUM 51		/* Recover Counter (also known as cr0) */
#define CCR_REGNUM 54		/* Coprocessor Configuration Register */
#define TR0_REGNUM 57		/* Temporary Registers (cr24 -> cr31) */
#define CR27_REGNUM 60		/* Base register for thread-local storage, cr27 */
#define FP0_REGNUM 64		/* floating point reg. 0 (fspr) */
#define FP4_REGNUM 72

#define ARG0_REGNUM 26		/* The first argument of a callee. */
#define ARG1_REGNUM 25		/* The second argument of a callee. */
#define ARG2_REGNUM 24		/* The third argument of a callee. */
#define ARG3_REGNUM 23		/* The fourth argument of a callee. */

/* compatibility with the rest of gdb. */
#define PC_REGNUM PCOQ_HEAD_REGNUM
#define NPC_REGNUM PCOQ_TAIL_REGNUM

/*
 * Processor Status Word Masks
 */

#define PSW_T   0x01000000	/* Taken Branch Trap Enable */
#define PSW_H   0x00800000	/* Higher-Privilege Transfer Trap Enable */
#define PSW_L   0x00400000	/* Lower-Privilege Transfer Trap Enable */
#define PSW_N   0x00200000	/* PC Queue Front Instruction Nullified */
#define PSW_X   0x00100000	/* Data Memory Break Disable */
#define PSW_B   0x00080000	/* Taken Branch in Previous Cycle */
#define PSW_C   0x00040000	/* Code Address Translation Enable */
#define PSW_V   0x00020000	/* Divide Step Correction */
#define PSW_M   0x00010000	/* High-Priority Machine Check Disable */
#define PSW_CB  0x0000ff00	/* Carry/Borrow Bits */
#define PSW_R   0x00000010	/* Recovery Counter Enable */
#define PSW_Q   0x00000008	/* Interruption State Collection Enable */
#define PSW_P   0x00000004	/* Protection ID Validation Enable */
#define PSW_D   0x00000002	/* Data Address Translation Enable */
#define PSW_I   0x00000001	/* External, Power Failure, Low-Priority */
				/* Machine Check Interruption Enable */

/* When fetching register values from an inferior or a core file,
   clean them up using this macro.  BUF is a char pointer to
   the raw value of the register in the registers[] array.  */

#define	CLEAN_UP_REGISTER_VALUE(regno, buf) \
  do {	\
    if ((regno) == PCOQ_HEAD_REGNUM || (regno) == PCOQ_TAIL_REGNUM) \
      (buf)[sizeof(CORE_ADDR) -1] &= ~0x3; \
  } while (0)

/* Define DO_REGISTERS_INFO() to do machine-specific formatting
   of register dumps. */

#define DO_REGISTERS_INFO(_regnum, fp) pa_do_registers_info (_regnum, fp)
extern void pa_do_registers_info PARAMS ((int, int));

#if 0
#define STRCAT_REGISTER(regnum, fpregs, stream, precision) pa_do_strcat_registers_info (regnum, fpregs, stream, precision)
extern void pa_do_strcat_registers_info (int, int, struct ui_file *, enum precision_type);
#endif

/* PA specific macro to see if the current instruction is nullified. */
#ifndef INSTRUCTION_NULLIFIED
#define INSTRUCTION_NULLIFIED \
    (((int)read_register (IPSW_REGNUM) & 0x00200000) && \
     !((int)read_register (FLAGS_REGNUM) & 0x2))
#endif

/* Number of bytes of storage in the actual machine representation
   for register N.  On the PA-RISC, all regs are 4 bytes, including
   the FP registers (they're accessed as two 4 byte halves).  */

#define REGISTER_RAW_SIZE(N) 4

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */
#define REGISTER_BYTES (NUM_REGS * 4)

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N) (N) * 4

/* Number of bytes of storage in the program's representation
   for register N. */

#define REGISTER_VIRTUAL_SIZE(N) REGISTER_RAW_SIZE(N)

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 4

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 8

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
 ((N) < FP4_REGNUM ? builtin_type_int : builtin_type_float)

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */

#define STORE_STRUCT_RETURN(ADDR, SP) {write_register (28, (ADDR)); }

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF. 

   elz: changed what to return when length is > 4: the stored result is 
   in register 28 and in register 29, with the lower order word being in reg 29, 
   so we must start reading it from somehere in the middle of reg28

   FIXME: Not sure what to do for soft float here.  */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
  { \
    if (TYPE_CODE (TYPE) == TYPE_CODE_FLT && !SOFT_FLOAT) \
      memcpy ((VALBUF), \
	      ((char *)(REGBUF)) + REGISTER_BYTE (FP4_REGNUM), \
	      TYPE_LENGTH (TYPE)); \
    else \
      memcpy ((VALBUF), \
	      (char *)(REGBUF) + REGISTER_BYTE (28) + \
	      (TYPE_LENGTH (TYPE) > 4 ? (8 - TYPE_LENGTH (TYPE)) : (4 - TYPE_LENGTH (TYPE))), \
	      TYPE_LENGTH (TYPE)); \
  }


 /* elz: decide whether the function returning a value of type type
    will put it on the stack or in the registers.
    The pa calling convention says that:
    register 28 (called ret0 by gdb) contains any ASCII char,
    and any non_floating point value up to 32-bits.
    reg 28 and 29 contain non-floating point up tp 64 bits and larger
    than 32 bits. (higer order word in reg 28).
    fr4: floating point up to 64 bits
    sr1: space identifier (32-bit)
    stack: any lager than 64-bit, with the address in r28
  */
extern use_struct_convention_fn hppa_use_struct_convention;
#define USE_STRUCT_CONVENTION(gcc_p,type) hppa_use_struct_convention (gcc_p,type)

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.

   For software floating point the return value goes into the integer
   registers.  But we don't have any flag to key this on, so we always
   store the value into the integer registers, and if it's a float value,
   then we put it in the float registers too.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  write_register_bytes (REGISTER_BYTE (28),(VALBUF), TYPE_LENGTH (TYPE)) ; \
  if (!SOFT_FLOAT) \
    write_register_bytes ((TYPE_CODE(TYPE) == TYPE_CODE_FLT \
			   ? REGISTER_BYTE (FP4_REGNUM) \
			   : REGISTER_BYTE (28)),		\
			  (VALBUF), TYPE_LENGTH (TYPE))

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  (*(int *)((REGBUF) + REGISTER_BYTE (28)))

/* elz: Return a large value, which is stored on the stack at addr.
   This is defined only for the hppa, at this moment. 
   The above macro EXTRACT_STRUCT_VALUE_ADDRESS is not called anymore,
   because it assumes that on exit from a called function which returns
   a large structure on the stack, the address of the ret structure is 
   still in register 28. Unfortunately this register is usually overwritten
   by the called function itself, on hppa. This is specified in the calling
   convention doc. As far as I know, the only way to get the return value
   is to have the caller tell us where it told the callee to put it, rather
   than have the callee tell us.
 */
#define VALUE_RETURNED_FROM_STACK(valtype,addr) \
  hppa_value_returned_from_stack (valtype, addr)

/*
 * This macro defines the register numbers (from REGISTER_NAMES) that
 * are effectively unavailable to the user through ptrace().  It allows
 * us to include the whole register set in REGISTER_NAMES (inorder to
 * better support remote debugging).  If it is used in
 * fetch/store_inferior_registers() gdb will not complain about I/O errors
 * on fetching these registers.  If all registers in REGISTER_NAMES
 * are available, then return false (0).
 */

#define CANNOT_STORE_REGISTER(regno)            \
                   ((regno) == 0) ||     \
                   ((regno) == PCSQ_HEAD_REGNUM) || \
                   ((regno) >= PCSQ_TAIL_REGNUM && (regno) < IPSW_REGNUM) ||  \
                   ((regno) > IPSW_REGNUM && (regno) < FP4_REGNUM)

#define INIT_EXTRA_FRAME_INFO(fromleaf, frame) init_extra_frame_info (fromleaf, frame)
extern void init_extra_frame_info PARAMS ((int, struct frame_info *));

/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer.

   FRAME_CHAIN_COMBINE takes the chain pointer and the frame's nominal address
   and produces the nominal address of the caller frame.

   However, if FRAME_CHAIN_VALID returns zero,
   it means the given frame is the outermost one and has no caller.
   In that case, FRAME_CHAIN_COMBINE is not used.  */

/* In the case of the PA-RISC, the frame's nominal address
   is the address of a 4-byte word containing the calling frame's
   address (previous FP).  */

#define FRAME_CHAIN(thisframe) frame_chain (thisframe)
extern CORE_ADDR frame_chain PARAMS ((struct frame_info *));

extern int hppa_frame_chain_valid PARAMS ((CORE_ADDR, struct frame_info *));
#define FRAME_CHAIN_VALID(chain, thisframe) hppa_frame_chain_valid (chain, thisframe)

#define FRAME_CHAIN_COMBINE(chain, thisframe) (chain)

/* Define other aspects of the stack frame.  */

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */
#define FRAMELESS_FUNCTION_INVOCATION(FI) \
  (frameless_function_invocation (FI))
extern int frameless_function_invocation PARAMS ((struct frame_info *));

extern CORE_ADDR hppa_frame_saved_pc PARAMS ((struct frame_info * frame));
#define FRAME_SAVED_PC(FRAME) hppa_frame_saved_pc (FRAME)

#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)

#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)
/* Set VAL to the number of args passed to frame described by FI.
   Can set VAL to -1, meaning no way to tell.  */

/* We can't tell how many args there are
   now that the C compiler delays popping them.  */
#define FRAME_NUM_ARGS(fi) (-1)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs) \
  hppa_frame_find_saved_regs (frame_info, &frame_saved_regs)
extern void
hppa_frame_find_saved_regs PARAMS ((struct frame_info *,
				    struct frame_saved_regs *));


/* Things needed for making the inferior call functions.  */

/* Push an empty stack frame, to record the current PC, etc. */

#define PUSH_DUMMY_FRAME push_dummy_frame (inf_status)
extern void push_dummy_frame PARAMS ((struct inferior_status *));

/* Discard from the stack the innermost frame, 
   restoring all saved registers.  */
#define POP_FRAME  hppa_pop_frame ()
extern void hppa_pop_frame PARAMS ((void));

#define INSTRUCTION_SIZE 4

#ifndef PA_LEVEL_0

/* Non-level zero PA's have space registers (but they don't always have
   floating-point, do they????  */

/* This sequence of words is the instructions

   ; Call stack frame has already been built by gdb. Since we could be calling 
   ; a varargs function, and we do not have the benefit of a stub to put things in
   ; the right place, we load the first 4 word of arguments into both the general
   ; and fp registers.
   call_dummy
   ldw -36(sp), arg0
   ldw -40(sp), arg1
   ldw -44(sp), arg2
   ldw -48(sp), arg3
   ldo -36(sp), r1
   fldws 0(0, r1), fr4
   fldds -4(0, r1), fr5
   fldws -8(0, r1), fr6
   fldds -12(0, r1), fr7
   ldil 0, r22                  ; FUNC_LDIL_OFFSET must point here
   ldo 0(r22), r22                      ; FUNC_LDO_OFFSET must point here
   ldsid (0,r22), r4
   ldil 0, r1                   ; SR4EXPORT_LDIL_OFFSET must point here
   ldo 0(r1), r1                        ; SR4EXPORT_LDO_OFFSET must point here
   ldsid (0,r1), r20
   combt,=,n r4, r20, text_space        ; If target is in data space, do a
   ble 0(sr5, r22)                      ; "normal" procedure call
   copy r31, r2
   break 4, 8 
   mtsp r21, sr0
   ble,n 0(sr0, r22)
   text_space                           ; Otherwise, go through _sr4export,
   ble (sr4, r1)                        ; which will return back here.
   stw r31,-24(r30)
   break 4, 8
   mtsp r21, sr0
   ble,n 0(sr0, r22)
   nop                          ; To avoid kernel bugs 
   nop                          ; and keep the dummy 8 byte aligned

   The dummy decides if the target is in text space or data space. If
   it's in data space, there's no problem because the target can
   return back to the dummy. However, if the target is in text space,
   the dummy calls the secret, undocumented routine _sr4export, which
   calls a function in text space and can return to any space. Instead
   of including fake instructions to represent saved registers, we
   know that the frame is associated with the call dummy and treat it
   specially.

   The trailing NOPs are needed to avoid a bug in HPUX, BSD and OSF1 
   kernels.   If the memory at the location pointed to by the PC is
   0xffffffff then a ptrace step call will fail (even if the instruction
   is nullified).

   The code to pop a dummy frame single steps three instructions
   starting with the last mtsp.  This includes the nullified "instruction"
   following the ble (which is uninitialized junk).  If the 
   "instruction" following the last BLE is 0xffffffff, then the ptrace
   will fail and the dummy frame is not correctly popped.

   By placing a NOP in the delay slot of the BLE instruction we can be 
   sure that we never try to execute a 0xffffffff instruction and
   avoid the kernel bug.  The second NOP is needed to keep the call
   dummy 8 byte aligned.  */

/* Define offsets into the call dummy for the target function address */
#define FUNC_LDIL_OFFSET (INSTRUCTION_SIZE * 9)
#define FUNC_LDO_OFFSET (INSTRUCTION_SIZE * 10)

/* Define offsets into the call dummy for the _sr4export address */
#define SR4EXPORT_LDIL_OFFSET (INSTRUCTION_SIZE * 12)
#define SR4EXPORT_LDO_OFFSET (INSTRUCTION_SIZE * 13)

#define CALL_DUMMY {0x4BDA3FB9, 0x4BD93FB1, 0x4BD83FA9, 0x4BD73FA1,\
                    0x37C13FB9, 0x24201004, 0x2C391005, 0x24311006,\
                    0x2C291007, 0x22C00000, 0x36D60000, 0x02C010A4,\
                    0x20200000, 0x34210000, 0x002010b4, 0x82842022,\
                    0xe6c06000, 0x081f0242, 0x00010004, 0x00151820,\
                    0xe6c00002, 0xe4202000, 0x6bdf3fd1, 0x00010004,\
                    0x00151820, 0xe6c00002, 0x08000240, 0x08000240}

#define CALL_DUMMY_LENGTH (INSTRUCTION_SIZE * 28)
#define REG_PARM_STACK_SPACE 16

#else /* defined PA_LEVEL_0 */

/* This is the call dummy for a level 0 PA.  Level 0's don't have space
   registers (or floating point??), so we skip all that inter-space call stuff,
   and avoid touching the fp regs.

   call_dummy

   ldw -36(%sp), %arg0
   ldw -40(%sp), %arg1
   ldw -44(%sp), %arg2
   ldw -48(%sp), %arg3
   ldil 0, %r31                 ; FUNC_LDIL_OFFSET must point here
   ldo 0(%r31), %r31            ; FUNC_LDO_OFFSET must point here
   ble 0(%sr0, %r31)
   copy %r31, %r2
   break 4, 8 
   nop                          ; restore_pc_queue expects these
   bv,n 0(%r22)                 ; instructions to be here...
   nop
 */

/* Define offsets into the call dummy for the target function address */
#define FUNC_LDIL_OFFSET (INSTRUCTION_SIZE * 4)
#define FUNC_LDO_OFFSET (INSTRUCTION_SIZE * 5)

#define CALL_DUMMY {0x4bda3fb9, 0x4bd93fb1, 0x4bd83fa9, 0x4bd73fa1,\
		    0x23e00000, 0x37ff0000, 0xe7e00000, 0x081f0242,\
		    0x00010004, 0x08000240, 0xeac0c002, 0x08000240}

#define CALL_DUMMY_LENGTH (INSTRUCTION_SIZE * 12)

#endif

#define CALL_DUMMY_START_OFFSET 0

/* If we've reached a trap instruction within the call dummy, then
   we'll consider that to mean that we've reached the call dummy's
   end after its successful completion. */
#define CALL_DUMMY_HAS_COMPLETED(pc, sp, frame_address) \
  (PC_IN_CALL_DUMMY((pc), (sp), (frame_address)) && \
   (read_memory_integer((pc), 4) == BREAKPOINT32))

/*
 * Insert the specified number of args and function address
 * into a call sequence of the above form stored at DUMMYNAME.
 *
 * On the hppa we need to call the stack dummy through $$dyncall.
 * Therefore our version of FIX_CALL_DUMMY takes an extra argument,
 * real_pc, which is the location where gdb should start up the
 * inferior to do the function call.
 */

#define FIX_CALL_DUMMY hppa_fix_call_dummy

extern CORE_ADDR
  hppa_fix_call_dummy PARAMS ((char *, CORE_ADDR, CORE_ADDR, int,
			       struct value **, struct type *, int));

#define PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \
  (hppa_push_arguments((nargs), (args), (sp), (struct_return), (struct_addr)))
extern CORE_ADDR
  hppa_push_arguments PARAMS ((int, struct value **, CORE_ADDR, int,
			       CORE_ADDR));

/* The low two bits of the PC on the PA contain the privilege level.  Some
   genius implementing a (non-GCC) compiler apparently decided this means
   that "addresses" in a text section therefore include a privilege level,
   and thus symbol tables should contain these bits.  This seems like a
   bonehead thing to do--anyway, it seems to work for our purposes to just
   ignore those bits.  */
#define SMASH_TEXT_ADDRESS(addr) ((addr) &= ~0x3)

#define	GDB_TARGET_IS_HPPA

#define BELIEVE_PCC_PROMOTION 1

/*
 * Unwind table and descriptor.
 */

struct unwind_table_entry
  {
    CORE_ADDR region_start;
    CORE_ADDR region_end;

    unsigned int Cannot_unwind:1;	/* 0 */
    unsigned int Millicode:1;	/* 1 */
    unsigned int Millicode_save_sr0:1;	/* 2 */
    unsigned int Region_description:2;	/* 3..4 */
    unsigned int reserved1:1;	/* 5 */
    unsigned int Entry_SR:1;	/* 6 */
    unsigned int Entry_FR:4;	/* number saved *//* 7..10 */
    unsigned int Entry_GR:5;	/* number saved *//* 11..15 */
    unsigned int Args_stored:1;	/* 16 */
    unsigned int Variable_Frame:1;	/* 17 */
    unsigned int Separate_Package_Body:1;	/* 18 */
    unsigned int Frame_Extension_Millicode:1;	/* 19 */
    unsigned int Stack_Overflow_Check:1;	/* 20 */
    unsigned int Two_Instruction_SP_Increment:1;	/* 21 */
    unsigned int Ada_Region:1;	/* 22 */
    unsigned int cxx_info:1;	/* 23 */
    unsigned int cxx_try_catch:1;	/* 24 */
    unsigned int sched_entry_seq:1;	/* 25 */
    unsigned int reserved2:1;	/* 26 */
    unsigned int Save_SP:1;	/* 27 */
    unsigned int Save_RP:1;	/* 28 */
    unsigned int Save_MRP_in_frame:1;	/* 29 */
    unsigned int extn_ptr_defined:1;	/* 30 */
    unsigned int Cleanup_defined:1;	/* 31 */

    unsigned int MPE_XL_interrupt_marker:1;	/* 0 */
    unsigned int HP_UX_interrupt_marker:1;	/* 1 */
    unsigned int Large_frame:1;	/* 2 */
    unsigned int Pseudo_SP_Set:1;	/* 3 */
    unsigned int reserved4:1;	/* 4 */
    unsigned int Total_frame_size:27;	/* 5..31 */

    /* This is *NOT* part of an actual unwind_descriptor in an object
       file.  It is *ONLY* part of the "internalized" descriptors that
       we create from those in a file.
     */
    struct
      {
	unsigned int stub_type:4;	/* 0..3 */
	unsigned int padding:28;	/* 4..31 */
      }
    stub_unwind;
  };

/* HP linkers also generate unwinds for various linker-generated stubs.
   GDB reads in the stubs from the $UNWIND_END$ subspace, then 
   "converts" them into normal unwind entries using some of the reserved
   fields to store the stub type.  */

struct stub_unwind_entry
  {
    /* The offset within the executable for the associated stub.  */
    unsigned stub_offset;

    /* The type of stub this unwind entry describes.  */
    char type;

    /* Unknown.  Not needed by GDB at this time.  */
    char prs_info;

    /* Length (in instructions) of the associated stub.  */
    short stub_length;
  };

/* Sizes (in bytes) of the native unwind entries.  */
#define UNWIND_ENTRY_SIZE 16
#define STUB_UNWIND_ENTRY_SIZE 8

/* The gaps represent linker stubs used in MPE and space for future
   expansion.  */
enum unwind_stub_types
  {
    LONG_BRANCH = 1,
    PARAMETER_RELOCATION = 2,
    EXPORT = 10,
    IMPORT = 11,
    IMPORT_SHLIB = 12,
  };

/* We use the objfile->obj_private pointer for two things:

 * 1.  An unwind table;
 *
 * 2.  A pointer to any associated shared library object.
 *
 * #defines are used to help refer to these objects.
 */

/* Info about the unwind table associated with an object file.

 * This is hung off of the "objfile->obj_private" pointer, and
 * is allocated in the objfile's psymbol obstack.  This allows
 * us to have unique unwind info for each executable and shared
 * library that we are debugging.
 */
struct obj_unwind_info
  {
    struct unwind_table_entry *table;	/* Pointer to unwind info */
    struct unwind_table_entry *cache;	/* Pointer to last entry we found */
    int last;			/* Index of last entry */
  };

typedef struct obj_private_struct
  {
    struct obj_unwind_info *unwind_info;	/* a pointer */
    struct so_list *so_info;	/* a pointer  */
    CORE_ADDR dp;
  }
obj_private_data_t;

#if 0
extern void target_write_pc
PARAMS ((CORE_ADDR, int))
     extern CORE_ADDR target_read_pc PARAMS ((int));
     extern CORE_ADDR skip_trampoline_code PARAMS ((CORE_ADDR, char *));
#endif

#define TARGET_READ_PC(pid) target_read_pc (pid)
     extern CORE_ADDR target_read_pc PARAMS ((int));

#define TARGET_WRITE_PC(v,pid) target_write_pc (v,pid)
     extern void target_write_pc PARAMS ((CORE_ADDR, int));

#define TARGET_READ_FP() target_read_fp (inferior_pid)
     extern CORE_ADDR target_read_fp PARAMS ((int));

/* For a number of horrible reasons we may have to adjust the location
   of variables on the stack.  Ugh.  */
#define HPREAD_ADJUST_STACK_ADDRESS(ADDR) hpread_adjust_stack_address(ADDR)

     extern int hpread_adjust_stack_address PARAMS ((CORE_ADDR));

/* If the current gcc for for this target does not produce correct debugging
   information for float parameters, both prototyped and unprototyped, then
   define this macro.  This forces gdb to  always assume that floats are
   passed as doubles and then converted in the callee.

   For the pa, it appears that the debug info marks the parameters as
   floats regardless of whether the function is prototyped, but the actual
   values are passed as doubles for the non-prototyped case and floats for
   the prototyped case.  Thus we choose to make the non-prototyped case work
   for C and break the prototyped case, since the non-prototyped case is
   probably much more common.  (FIXME). */

#define COERCE_FLOAT_TO_DOUBLE(formal, actual) (current_language -> la_language == language_c)

/* Here's how to step off a permanent breakpoint.  */
#define SKIP_PERMANENT_BREAKPOINT (hppa_skip_permanent_breakpoint)
extern void hppa_skip_permanent_breakpoint (void);

/* On HP-UX, certain system routines (millicode) have names beginning
   with $ or $$, e.g. $$dyncall, which handles inter-space procedure
   calls on PA-RISC.  Tell the expression parser to check for those
   when parsing tokens that begin with "$".  */
#define SYMBOLS_CAN_START_WITH_DOLLAR (1)
