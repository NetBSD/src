// OBSOLETE /* Target machine description for generic Motorola 88000, for GDB.
// OBSOLETE 
// OBSOLETE    Copyright 1986, 1987, 1988, 1989, 1990, 1991, 1993, 1994, 1996,
// OBSOLETE    1998, 1999, 2000, 2002 Free Software Foundation, Inc.
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
// OBSOLETE #include "doublest.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE /* g++ support is not yet included.  */
// OBSOLETE 
// OBSOLETE /* We cache information about saved registers in the frame structure,
// OBSOLETE    to save us from having to re-scan function prologues every time
// OBSOLETE    a register in a non-current frame is accessed.  */
// OBSOLETE 
// OBSOLETE #define EXTRA_FRAME_INFO 	\
// OBSOLETE 	struct frame_saved_regs *fsr;	\
// OBSOLETE 	CORE_ADDR locals_pointer;	\
// OBSOLETE 	CORE_ADDR args_pointer;
// OBSOLETE 
// OBSOLETE /* Zero the frame_saved_regs pointer when the frame is initialized,
// OBSOLETE    so that FRAME_FIND_SAVED_REGS () will know to allocate and
// OBSOLETE    initialize a frame_saved_regs struct the first time it is called.
// OBSOLETE    Set the arg_pointer to -1, which is not valid; 0 and other values
// OBSOLETE    indicate real, cached values.  */
// OBSOLETE 
// OBSOLETE #define INIT_EXTRA_FRAME_INFO(fromleaf, fi) \
// OBSOLETE 	init_extra_frame_info (fromleaf, fi)
// OBSOLETE extern void init_extra_frame_info ();
// OBSOLETE 
// OBSOLETE /* Offset from address of function to start of its code.
// OBSOLETE    Zero on most machines.  */
// OBSOLETE 
// OBSOLETE #define FUNCTION_START_OFFSET 0
// OBSOLETE 
// OBSOLETE /* Advance PC across any function entry prologue instructions
// OBSOLETE    to reach some "real" code.  */
// OBSOLETE 
// OBSOLETE extern CORE_ADDR m88k_skip_prologue (CORE_ADDR);
// OBSOLETE #define SKIP_PROLOGUE(frompc) (m88k_skip_prologue (frompc))
// OBSOLETE 
// OBSOLETE /* The m88k kernel aligns all instructions on 4-byte boundaries.  The
// OBSOLETE    kernel also uses the least significant two bits for its own hocus
// OBSOLETE    pocus.  When gdb receives an address from the kernel, it needs to
// OBSOLETE    preserve those right-most two bits, but gdb also needs to be careful
// OBSOLETE    to realize that those two bits are not really a part of the address
// OBSOLETE    of an instruction.  Shrug.  */
// OBSOLETE 
// OBSOLETE extern CORE_ADDR m88k_addr_bits_remove (CORE_ADDR);
// OBSOLETE #define ADDR_BITS_REMOVE(addr) m88k_addr_bits_remove (addr)
// OBSOLETE 
// OBSOLETE /* Immediately after a function call, return the saved pc.
// OBSOLETE    Can't always go through the frames for this because on some machines
// OBSOLETE    the new frame is not set up until the new function executes
// OBSOLETE    some instructions.  */
// OBSOLETE 
// OBSOLETE #define SAVED_PC_AFTER_CALL(frame) \
// OBSOLETE   (ADDR_BITS_REMOVE (read_register (SRP_REGNUM)))
// OBSOLETE 
// OBSOLETE /* Stack grows downward.  */
// OBSOLETE 
// OBSOLETE #define INNER_THAN(lhs,rhs) ((lhs) < (rhs))
// OBSOLETE 
// OBSOLETE /* Sequence of bytes for breakpoint instruction.  */
// OBSOLETE 
// OBSOLETE /* instruction 0xF000D1FF is 'tb0 0,r0,511'
// OBSOLETE    If Bit bit 0 of r0 is clear (always true),
// OBSOLETE    initiate exception processing (trap).
// OBSOLETE  */
// OBSOLETE #define BREAKPOINT {0xF0, 0x00, 0xD1, 0xFF}
// OBSOLETE 
// OBSOLETE /* Amount PC must be decremented by after a breakpoint.
// OBSOLETE    This is often the number of bytes in BREAKPOINT
// OBSOLETE    but not always.  */
// OBSOLETE 
// OBSOLETE #define DECR_PC_AFTER_BREAK 0
// OBSOLETE 
// OBSOLETE /* Say how long (ordinary) registers are.  This is a piece of bogosity
// OBSOLETE    used in push_word and a few other places; REGISTER_RAW_SIZE is the
// OBSOLETE    real way to know how big a register is.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_SIZE 4
// OBSOLETE 
// OBSOLETE /* Number of machine registers */
// OBSOLETE 
// OBSOLETE #define GP_REGS (38)
// OBSOLETE #define FP_REGS (32)
// OBSOLETE #define NUM_REGS (GP_REGS + FP_REGS)
// OBSOLETE 
// OBSOLETE /* Initializer for an array of names of registers.
// OBSOLETE    There should be NUM_REGS strings in this initializer.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_NAMES {\
// OBSOLETE 			  "r0",\
// OBSOLETE 			  "r1",\
// OBSOLETE 			  "r2",\
// OBSOLETE 			  "r3",\
// OBSOLETE 			  "r4",\
// OBSOLETE 			  "r5",\
// OBSOLETE 			  "r6",\
// OBSOLETE 			  "r7",\
// OBSOLETE 			  "r8",\
// OBSOLETE 			  "r9",\
// OBSOLETE 			  "r10",\
// OBSOLETE 			  "r11",\
// OBSOLETE 			  "r12",\
// OBSOLETE 			  "r13",\
// OBSOLETE 			  "r14",\
// OBSOLETE 			  "r15",\
// OBSOLETE 			  "r16",\
// OBSOLETE 			  "r17",\
// OBSOLETE 			  "r18",\
// OBSOLETE 			  "r19",\
// OBSOLETE 			  "r20",\
// OBSOLETE 			  "r21",\
// OBSOLETE 			  "r22",\
// OBSOLETE 			  "r23",\
// OBSOLETE 			  "r24",\
// OBSOLETE 			  "r25",\
// OBSOLETE 			  "r26",\
// OBSOLETE 			  "r27",\
// OBSOLETE 			  "r28",\
// OBSOLETE 			  "r29",\
// OBSOLETE 			  "r30",\
// OBSOLETE 			  "r31",\
// OBSOLETE 			  "psr",\
// OBSOLETE 			  "fpsr",\
// OBSOLETE 			  "fpcr",\
// OBSOLETE 			  "sxip",\
// OBSOLETE 			  "snip",\
// OBSOLETE 			  "sfip",\
// OBSOLETE 			  "x0",\
// OBSOLETE 			  "x1",\
// OBSOLETE 			  "x2",\
// OBSOLETE 			  "x3",\
// OBSOLETE 			  "x4",\
// OBSOLETE 			  "x5",\
// OBSOLETE 			  "x6",\
// OBSOLETE 			  "x7",\
// OBSOLETE 			  "x8",\
// OBSOLETE 			  "x9",\
// OBSOLETE 			  "x10",\
// OBSOLETE 			  "x11",\
// OBSOLETE 			  "x12",\
// OBSOLETE 			  "x13",\
// OBSOLETE 			  "x14",\
// OBSOLETE 			  "x15",\
// OBSOLETE 			  "x16",\
// OBSOLETE 			  "x17",\
// OBSOLETE 			  "x18",\
// OBSOLETE 			  "x19",\
// OBSOLETE 			  "x20",\
// OBSOLETE 			  "x21",\
// OBSOLETE 			  "x22",\
// OBSOLETE 			  "x23",\
// OBSOLETE 			  "x24",\
// OBSOLETE 			  "x25",\
// OBSOLETE 			  "x26",\
// OBSOLETE 			  "x27",\
// OBSOLETE 			  "x28",\
// OBSOLETE 			  "x29",\
// OBSOLETE 			  "x30",\
// OBSOLETE 			  "x31",\
// OBSOLETE 			  "vbr",\
// OBSOLETE 			  "dmt0",\
// OBSOLETE 			  "dmd0",\
// OBSOLETE 			  "dma0",\
// OBSOLETE 			  "dmt1",\
// OBSOLETE 			  "dmd1",\
// OBSOLETE 			  "dma1",\
// OBSOLETE 			  "dmt2",\
// OBSOLETE 			  "dmd2",\
// OBSOLETE 			  "dma2",\
// OBSOLETE 			  "sr0",\
// OBSOLETE 			  "sr1",\
// OBSOLETE 			  "sr2",\
// OBSOLETE 			  "sr3",\
// OBSOLETE 			  "fpecr",\
// OBSOLETE 			  "fphs1",\
// OBSOLETE 			  "fpls1",\
// OBSOLETE 			  "fphs2",\
// OBSOLETE 			  "fpls2",\
// OBSOLETE 			  "fppt",\
// OBSOLETE 			  "fprh",\
// OBSOLETE 			  "fprl",\
// OBSOLETE 			  "fpit",\
// OBSOLETE 			  "fpsr",\
// OBSOLETE 			  "fpcr",\
// OBSOLETE 		      }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Register numbers of various important registers.
// OBSOLETE    Note that some of these values are "real" register numbers,
// OBSOLETE    and correspond to the general registers of the machine,
// OBSOLETE    and some are "phony" register numbers which are too large
// OBSOLETE    to be actual register numbers as far as the user is concerned
// OBSOLETE    but do serve to get the desired values when passed to read_register.  */
// OBSOLETE 
// OBSOLETE #define R0_REGNUM 0		/* Contains the constant zero */
// OBSOLETE #define SRP_REGNUM 1		/* Contains subroutine return pointer */
// OBSOLETE #define RV_REGNUM 2		/* Contains simple return values */
// OBSOLETE #define SRA_REGNUM 12		/* Contains address of struct return values */
// OBSOLETE #define SP_REGNUM 31		/* Contains address of top of stack */
// OBSOLETE 
// OBSOLETE /* Instruction pointer notes...
// OBSOLETE 
// OBSOLETE    On the m88100:
// OBSOLETE 
// OBSOLETE    * cr04 = sxip.  On exception, contains the excepting pc (probably).
// OBSOLETE    On rte, is ignored.
// OBSOLETE 
// OBSOLETE    * cr05 = snip.  On exception, contains the NPC (next pc).  On rte,
// OBSOLETE    pc is loaded from here.
// OBSOLETE 
// OBSOLETE    * cr06 = sfip.  On exception, contains the NNPC (next next pc).  On
// OBSOLETE    rte, the NPC is loaded from here.
// OBSOLETE 
// OBSOLETE    * lower two bits of each are flag bits.  Bit 1 is V means address
// OBSOLETE    is valid.  If address is not valid, bit 0 is ignored.  Otherwise,
// OBSOLETE    bit 0 is E and asks for an exception to be taken if this
// OBSOLETE    instruction is executed.
// OBSOLETE 
// OBSOLETE    On the m88110:
// OBSOLETE 
// OBSOLETE    * cr04 = exip.  On exception, contains the address of the excepting
// OBSOLETE    pc (always).  On rte, pc is loaded from here.  Bit 0, aka the D
// OBSOLETE    bit, is a flag saying that the offending instruction was in a
// OBSOLETE    branch delay slot.  If set, then cr05 contains the NPC.
// OBSOLETE 
// OBSOLETE    * cr05 = enip.  On exception, if the instruction pointed to by cr04
// OBSOLETE    was in a delay slot as indicated by the bit 0 of cr04, aka the D
// OBSOLETE    bit, the cr05 contains the NPC.  Otherwise ignored.
// OBSOLETE 
// OBSOLETE    * cr06 is invalid  */
// OBSOLETE 
// OBSOLETE /* Note that the Harris Unix kernels emulate the m88100's behavior on
// OBSOLETE    the m88110.  */
// OBSOLETE 
// OBSOLETE #define SXIP_REGNUM 35		/* On m88100, Contains Shadow Execute
// OBSOLETE 				   Instruction Pointer.  */
// OBSOLETE #define SNIP_REGNUM 36		/* On m88100, Contains Shadow Next
// OBSOLETE 				   Instruction Pointer.  */
// OBSOLETE #define SFIP_REGNUM 37		/* On m88100, Contains Shadow Fetched
// OBSOLETE 				   Intruction pointer.  */
// OBSOLETE 
// OBSOLETE #define EXIP_REGNUM 35		/* On m88110, Contains Exception
// OBSOLETE 				   Executing Instruction Pointer.  */
// OBSOLETE #define ENIP_REGNUM 36		/* On m88110, Contains the Exception
// OBSOLETE 				   Next Instruction Pointer.  */
// OBSOLETE 
// OBSOLETE #define PC_REGNUM SXIP_REGNUM	/* Program Counter */
// OBSOLETE #define NPC_REGNUM SNIP_REGNUM	/* Next Program Counter */
// OBSOLETE #define M88K_NNPC_REGNUM SFIP_REGNUM        /* Next Next Program Counter */
// OBSOLETE 
// OBSOLETE 
// OBSOLETE #define PSR_REGNUM 32		/* Processor Status Register */
// OBSOLETE #define FPSR_REGNUM 33		/* Floating Point Status Register */
// OBSOLETE #define FPCR_REGNUM 34		/* Floating Point Control Register */
// OBSOLETE #define XFP_REGNUM 38		/* First Extended Float Register */
// OBSOLETE #define X0_REGNUM XFP_REGNUM	/* Which also contains the constant zero */
// OBSOLETE 
// OBSOLETE /* This is rather a confusing lie.  Our m88k port using a stack pointer value
// OBSOLETE    for the frame address.  Hence, the frame address and the frame pointer are
// OBSOLETE    only indirectly related.  The value of this macro is the register number
// OBSOLETE    fetched by the machine "independent" portions of gdb when they want to know
// OBSOLETE    about a frame address.  Thus, we lie here and claim that FP_REGNUM is
// OBSOLETE    SP_REGNUM.  */
// OBSOLETE #define FP_REGNUM SP_REGNUM	/* Reg fetched to locate frame when pgm stops */
// OBSOLETE #define ACTUAL_FP_REGNUM 30
// OBSOLETE 
// OBSOLETE /* PSR status bit definitions.  */
// OBSOLETE 
// OBSOLETE #define PSR_MODE		0x80000000
// OBSOLETE #define PSR_BYTE_ORDER		0x40000000
// OBSOLETE #define PSR_SERIAL_MODE		0x20000000
// OBSOLETE #define PSR_CARRY		0x10000000
// OBSOLETE #define PSR_SFU_DISABLE		0x000003f0
// OBSOLETE #define PSR_SFU1_DISABLE	0x00000008
// OBSOLETE #define PSR_MXM			0x00000004
// OBSOLETE #define PSR_IND			0x00000002
// OBSOLETE #define PSR_SFRZ		0x00000001
// OBSOLETE 
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* The following two comments come from the days prior to the m88110
// OBSOLETE    port.  The m88110 handles the instruction pointers differently.  I
// OBSOLETE    do not know what any m88110 kernels do as the m88110 port I'm
// OBSOLETE    working with is for an embedded system.  rich@cygnus.com
// OBSOLETE    13-sept-93.  */
// OBSOLETE 
// OBSOLETE /* BCS requires that the SXIP_REGNUM (or PC_REGNUM) contain the
// OBSOLETE    address of the next instr to be executed when a breakpoint occurs.
// OBSOLETE    Because the kernel gets the next instr (SNIP_REGNUM), the instr in
// OBSOLETE    SNIP needs to be put back into SFIP, and the instr in SXIP should
// OBSOLETE    be shifted to SNIP */
// OBSOLETE 
// OBSOLETE /* Are you sitting down?  It turns out that the 88K BCS (binary
// OBSOLETE    compatibility standard) folks originally felt that the debugger
// OBSOLETE    should be responsible for backing up the IPs, not the kernel (as is
// OBSOLETE    usually done).  Well, they have reversed their decision, and in
// OBSOLETE    future releases our kernel will be handling the backing up of the
// OBSOLETE    IPs.  So, eventually, we won't need to do the SHIFT_INST_REGS
// OBSOLETE    stuff.  But, for now, since there are 88K systems out there that do
// OBSOLETE    need the debugger to do the IP shifting, and since there will be
// OBSOLETE    systems where the kernel does the shifting, the code is a little
// OBSOLETE    more complex than perhaps it needs to be (we still go inside
// OBSOLETE    SHIFT_INST_REGS, and if the shifting hasn't occurred then gdb goes
// OBSOLETE    ahead and shifts).  */
// OBSOLETE 
// OBSOLETE extern int target_is_m88110;
// OBSOLETE #define SHIFT_INST_REGS() \
// OBSOLETE if (!target_is_m88110) \
// OBSOLETE { \
// OBSOLETE     CORE_ADDR pc = read_register (PC_REGNUM); \
// OBSOLETE     CORE_ADDR npc = read_register (NPC_REGNUM); \
// OBSOLETE     if (pc != npc) \
// OBSOLETE     { \
// OBSOLETE 	write_register (M88K_NNPC_REGNUM, npc); \
// OBSOLETE 	write_register (NPC_REGNUM, pc); \
// OBSOLETE     } \
// OBSOLETE }
// OBSOLETE 
// OBSOLETE     /* Storing the following registers is a no-op. */
// OBSOLETE #define CANNOT_STORE_REGISTER(regno)	(((regno) == R0_REGNUM) \
// OBSOLETE 					 || ((regno) == X0_REGNUM))
// OBSOLETE 
// OBSOLETE   /* Number of bytes of storage in the actual machine representation
// OBSOLETE      for register N.  On the m88k,  the general purpose registers are 4
// OBSOLETE      bytes and the 88110 extended registers are 10 bytes. */
// OBSOLETE 
// OBSOLETE #define REGISTER_RAW_SIZE(N) ((N) < XFP_REGNUM ? 4 : 10)
// OBSOLETE 
// OBSOLETE   /* Total amount of space needed to store our copies of the machine's
// OBSOLETE      register state, the array `registers'.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_BYTES ((GP_REGS * REGISTER_RAW_SIZE(0)) \
// OBSOLETE 			+ (FP_REGS * REGISTER_RAW_SIZE(XFP_REGNUM)))
// OBSOLETE 
// OBSOLETE   /* Index within `registers' of the first byte of the space for
// OBSOLETE      register N.  */
// OBSOLETE 
// OBSOLETE #define REGISTER_BYTE(N) (((N) * REGISTER_RAW_SIZE(0)) \
// OBSOLETE 			  + ((N) >= XFP_REGNUM \
// OBSOLETE 			     ? (((N) - XFP_REGNUM) \
// OBSOLETE 				* REGISTER_RAW_SIZE(XFP_REGNUM)) \
// OBSOLETE 			     : 0))
// OBSOLETE 
// OBSOLETE   /* Number of bytes of storage in the program's representation for
// OBSOLETE      register N.  On the m88k, all registers are 4 bytes excepting the
// OBSOLETE      m88110 extended registers which are 8 byte doubles. */
// OBSOLETE 
// OBSOLETE #define REGISTER_VIRTUAL_SIZE(N) ((N) < XFP_REGNUM ? 4 : 8)
// OBSOLETE 
// OBSOLETE   /* Largest value REGISTER_RAW_SIZE can have.  */
// OBSOLETE 
// OBSOLETE #define MAX_REGISTER_RAW_SIZE (REGISTER_RAW_SIZE(XFP_REGNUM))
// OBSOLETE 
// OBSOLETE   /* Largest value REGISTER_VIRTUAL_SIZE can have.
// OBSOLETE      Are FPS1, FPS2, FPR "virtual" regisers? */
// OBSOLETE 
// OBSOLETE #define MAX_REGISTER_VIRTUAL_SIZE (REGISTER_RAW_SIZE(XFP_REGNUM))
// OBSOLETE 
// OBSOLETE /* Return the GDB type object for the "standard" data type
// OBSOLETE    of data in register N.  */
// OBSOLETE 
// OBSOLETE struct type *m88k_register_type (int regnum);
// OBSOLETE #define REGISTER_VIRTUAL_TYPE(N) m88k_register_type (N)
// OBSOLETE 
// OBSOLETE /* The 88k call/return conventions call for "small" values to be returned
// OBSOLETE    into consecutive registers starting from r2.  */
// OBSOLETE 
// OBSOLETE #define DEPRECATED_EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
// OBSOLETE   memcpy ((VALBUF), &(((char *)REGBUF)[REGISTER_BYTE(RV_REGNUM)]), TYPE_LENGTH (TYPE))
// OBSOLETE 
// OBSOLETE #define DEPRECATED_EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) (*(int *)(REGBUF))
// OBSOLETE 
// OBSOLETE /* Write into appropriate registers a function return value
// OBSOLETE    of type TYPE, given in virtual format.  */
// OBSOLETE 
// OBSOLETE #define STORE_RETURN_VALUE(TYPE,VALBUF) \
// OBSOLETE   write_register_bytes (2*REGISTER_RAW_SIZE(0), (VALBUF), TYPE_LENGTH (TYPE))
// OBSOLETE 
// OBSOLETE /* In COFF, if PCC says a parameter is a short or a char, do not
// OBSOLETE    change it to int (it seems the convention is to change it). */
// OBSOLETE 
// OBSOLETE #define BELIEVE_PCC_PROMOTION 1
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
// OBSOLETE extern CORE_ADDR frame_chain ();
// OBSOLETE extern int frame_chain_valid ();
// OBSOLETE extern int frameless_function_invocation ();
// OBSOLETE 
// OBSOLETE #define FRAME_CHAIN(thisframe) \
// OBSOLETE 	frame_chain (thisframe)
// OBSOLETE 
// OBSOLETE #define	FRAMELESS_FUNCTION_INVOCATION(frame)	\
// OBSOLETE 	(frameless_function_invocation (frame))
// OBSOLETE 
// OBSOLETE /* Define other aspects of the stack frame.  */
// OBSOLETE 
// OBSOLETE #define FRAME_SAVED_PC(FRAME)	\
// OBSOLETE 	frame_saved_pc (FRAME)
// OBSOLETE extern CORE_ADDR frame_saved_pc ();
// OBSOLETE 
// OBSOLETE #define FRAME_ARGS_ADDRESS(fi)	\
// OBSOLETE 	frame_args_address (fi)
// OBSOLETE extern CORE_ADDR frame_args_address ();
// OBSOLETE 
// OBSOLETE #define FRAME_LOCALS_ADDRESS(fi) \
// OBSOLETE 	frame_locals_address (fi)
// OBSOLETE extern CORE_ADDR frame_locals_address ();
// OBSOLETE 
// OBSOLETE /* Return number of args passed to a frame.
// OBSOLETE    Can return -1, meaning no way to tell.  */
// OBSOLETE 
// OBSOLETE #define FRAME_NUM_ARGS(fi)  (-1)
// OBSOLETE 
// OBSOLETE /* Return number of bytes at start of arglist that are not really args.  */
// OBSOLETE 
// OBSOLETE #define FRAME_ARGS_SKIP 0
// OBSOLETE 
// OBSOLETE /* Put here the code to store, into a struct frame_saved_regs,
// OBSOLETE    the addresses of the saved registers of frame described by FRAME_INFO.
// OBSOLETE    This includes special registers such as pc and fp saved in special
// OBSOLETE    ways in the stack frame.  sp is even more special:
// OBSOLETE    the address we return for it IS the sp for the next frame.  */
// OBSOLETE 
// OBSOLETE /* On the 88k, parameter registers get stored into the so called "homing"
// OBSOLETE    area.  This *always* happens when you compiled with GCC and use -g.
// OBSOLETE    Also, (with GCC and -g) the saving of the parameter register values
// OBSOLETE    always happens right within the function prologue code, so these register
// OBSOLETE    values can generally be relied upon to be already copied into their
// OBSOLETE    respective homing slots by the time you will normally try to look at
// OBSOLETE    them (we hope).
// OBSOLETE 
// OBSOLETE    Note that homing area stack slots are always at *positive* offsets from
// OBSOLETE    the frame pointer.  Thus, the homing area stack slots for the parameter
// OBSOLETE    registers (passed values) for a given function are actually part of the
// OBSOLETE    frame area of the caller.  This is unusual, but it should not present
// OBSOLETE    any special problems for GDB.
// OBSOLETE 
// OBSOLETE    Note also that on the 88k, we are only interested in finding the
// OBSOLETE    registers that might have been saved in memory.  This is a subset of
// OBSOLETE    the whole set of registers because the standard calling sequence allows
// OBSOLETE    the called routine to clobber many registers.
// OBSOLETE 
// OBSOLETE    We could manage to locate values for all of the so called "preserved"
// OBSOLETE    registers (some of which may get saved within any particular frame) but
// OBSOLETE    that would require decoding all of the tdesc information.  That would be
// OBSOLETE    nice information for GDB to have, but it is not strictly manditory if we
// OBSOLETE    can live without the ability to look at values within (or backup to)
// OBSOLETE    previous frames.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE struct frame_saved_regs;
// OBSOLETE struct frame_info;
// OBSOLETE 
// OBSOLETE void frame_find_saved_regs (struct frame_info *fi,
// OBSOLETE 			    struct frame_saved_regs *fsr);
// OBSOLETE 
// OBSOLETE #define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs) \
// OBSOLETE         frame_find_saved_regs (frame_info, &frame_saved_regs)
// OBSOLETE 
// OBSOLETE 
// OBSOLETE #define POP_FRAME pop_frame ()
// OBSOLETE extern void pop_frame ();
// OBSOLETE 
// OBSOLETE /* Call function stuff contributed by Kevin Buettner of Motorola.  */
// OBSOLETE 
// OBSOLETE #define CALL_DUMMY_LOCATION AFTER_TEXT_END
// OBSOLETE 
// OBSOLETE extern void m88k_push_dummy_frame ();
// OBSOLETE #define PUSH_DUMMY_FRAME	m88k_push_dummy_frame()
// OBSOLETE 
// OBSOLETE #define CALL_DUMMY { 				\
// OBSOLETE 0x67ff00c0,	/*   0:   subu	#sp,#sp,0xc0 */ \
// OBSOLETE 0x243f0004,	/*   4:   st	#r1,#sp,0x4 */ \
// OBSOLETE 0x245f0008,	/*   8:   st	#r2,#sp,0x8 */ \
// OBSOLETE 0x247f000c,	/*   c:   st	#r3,#sp,0xc */ \
// OBSOLETE 0x249f0010,	/*  10:   st	#r4,#sp,0x10 */ \
// OBSOLETE 0x24bf0014,	/*  14:   st	#r5,#sp,0x14 */ \
// OBSOLETE 0x24df0018,	/*  18:   st	#r6,#sp,0x18 */ \
// OBSOLETE 0x24ff001c,	/*  1c:   st	#r7,#sp,0x1c */ \
// OBSOLETE 0x251f0020,	/*  20:   st	#r8,#sp,0x20 */ \
// OBSOLETE 0x253f0024,	/*  24:   st	#r9,#sp,0x24 */ \
// OBSOLETE 0x255f0028,	/*  28:   st	#r10,#sp,0x28 */ \
// OBSOLETE 0x257f002c,	/*  2c:   st	#r11,#sp,0x2c */ \
// OBSOLETE 0x259f0030,	/*  30:   st	#r12,#sp,0x30 */ \
// OBSOLETE 0x25bf0034,	/*  34:   st	#r13,#sp,0x34 */ \
// OBSOLETE 0x25df0038,	/*  38:   st	#r14,#sp,0x38 */ \
// OBSOLETE 0x25ff003c,	/*  3c:   st	#r15,#sp,0x3c */ \
// OBSOLETE 0x261f0040,	/*  40:   st	#r16,#sp,0x40 */ \
// OBSOLETE 0x263f0044,	/*  44:   st	#r17,#sp,0x44 */ \
// OBSOLETE 0x265f0048,	/*  48:   st	#r18,#sp,0x48 */ \
// OBSOLETE 0x267f004c,	/*  4c:   st	#r19,#sp,0x4c */ \
// OBSOLETE 0x269f0050,	/*  50:   st	#r20,#sp,0x50 */ \
// OBSOLETE 0x26bf0054,	/*  54:   st	#r21,#sp,0x54 */ \
// OBSOLETE 0x26df0058,	/*  58:   st	#r22,#sp,0x58 */ \
// OBSOLETE 0x26ff005c,	/*  5c:   st	#r23,#sp,0x5c */ \
// OBSOLETE 0x271f0060,	/*  60:   st	#r24,#sp,0x60 */ \
// OBSOLETE 0x273f0064,	/*  64:   st	#r25,#sp,0x64 */ \
// OBSOLETE 0x275f0068,	/*  68:   st	#r26,#sp,0x68 */ \
// OBSOLETE 0x277f006c,	/*  6c:   st	#r27,#sp,0x6c */ \
// OBSOLETE 0x279f0070,	/*  70:   st	#r28,#sp,0x70 */ \
// OBSOLETE 0x27bf0074,	/*  74:   st	#r29,#sp,0x74 */ \
// OBSOLETE 0x27df0078,	/*  78:   st	#r30,#sp,0x78 */ \
// OBSOLETE 0x63df0000,	/*  7c:   addu	#r30,#sp,0x0 */ \
// OBSOLETE 0x145f0000,	/*  80:   ld	#r2,#sp,0x0 */ \
// OBSOLETE 0x147f0004,	/*  84:   ld	#r3,#sp,0x4 */ \
// OBSOLETE 0x149f0008,	/*  88:   ld	#r4,#sp,0x8 */ \
// OBSOLETE 0x14bf000c,	/*  8c:   ld	#r5,#sp,0xc */ \
// OBSOLETE 0x14df0010,	/*  90:   ld	#r6,#sp,0x10 */ \
// OBSOLETE 0x14ff0014,	/*  94:   ld	#r7,#sp,0x14 */ \
// OBSOLETE 0x151f0018,	/*  98:   ld	#r8,#sp,0x18 */ \
// OBSOLETE 0x153f001c,	/*  9c:   ld	#r9,#sp,0x1c */ \
// OBSOLETE 0x5c200000,	/*  a0:   or.u	#r1,#r0,0x0 */ \
// OBSOLETE 0x58210000,	/*  a4:   or	#r1,#r1,0x0 */ \
// OBSOLETE 0xf400c801,	/*  a8:   jsr	#r1 */ \
// OBSOLETE 0xf000d1ff	/*  ac:   tb0	0x0,#r0,0x1ff */ \
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #define CALL_DUMMY_START_OFFSET 0x80
// OBSOLETE #define CALL_DUMMY_LENGTH 0xb0
// OBSOLETE 
// OBSOLETE /* FIXME: byteswapping.  */
// OBSOLETE #define FIX_CALL_DUMMY(dummy, pc, fun, nargs, args, type, gcc_p)	\
// OBSOLETE { 									\
// OBSOLETE   *(unsigned long *)((char *) (dummy) + 0xa0) |=			\
// OBSOLETE 	(((unsigned long) (fun)) >> 16);				\
// OBSOLETE   *(unsigned long *)((char *) (dummy) + 0xa4) |=			\
// OBSOLETE 	(((unsigned long) (fun)) & 0xffff);				\
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Stack must be aligned on 64-bit boundaries when synthesizing
// OBSOLETE    function calls. */
// OBSOLETE 
// OBSOLETE #define STACK_ALIGN(addr) (((addr) + 7) & -8)
// OBSOLETE 
// OBSOLETE #define STORE_STRUCT_RETURN(addr, sp) \
// OBSOLETE     write_register (SRA_REGNUM, (addr))
// OBSOLETE 
// OBSOLETE #define NEED_TEXT_START_END 1
// OBSOLETE 
// OBSOLETE /* According to the MC88100 RISC Microprocessor User's Manual, section
// OBSOLETE    6.4.3.1.2:
// OBSOLETE 
// OBSOLETE    ... can be made to return to a particular instruction by placing a
// OBSOLETE    valid instruction address in the SNIP and the next sequential
// OBSOLETE    instruction address in the SFIP (with V bits set and E bits clear).
// OBSOLETE    The rte resumes execution at the instruction pointed to by the 
// OBSOLETE    SNIP, then the SFIP.
// OBSOLETE 
// OBSOLETE    The E bit is the least significant bit (bit 0).  The V (valid) bit is
// OBSOLETE    bit 1.  This is why we logical or 2 into the values we are writing
// OBSOLETE    below.  It turns out that SXIP plays no role when returning from an
// OBSOLETE    exception so nothing special has to be done with it.  We could even
// OBSOLETE    (presumably) give it a totally bogus value.
// OBSOLETE 
// OBSOLETE    -- Kevin Buettner
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE extern void m88k_target_write_pc (CORE_ADDR pc, ptid_t ptid);
// OBSOLETE #define TARGET_WRITE_PC(VAL, PID) m88k_target_write_pc (VAL, PID)
