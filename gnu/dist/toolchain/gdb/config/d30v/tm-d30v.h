/* Target-specific definition for the Mitsubishi D30V
   Copyright (C) 1997 Free Software Foundation, Inc.

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

#ifndef TM_D30V_H
#define TM_D30V_H

/* Define the bit, byte, and word ordering of the machine.  */

#define TARGET_BYTE_ORDER	BIG_ENDIAN

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* these are the addresses the D30V-EVA board maps data */
/* and instruction memory to. */

#define DMEM_START	0x20000000
#define IMEM_START	0x00000000	/* was 0x10000000 */
#define STACK_START	0x20007ffe

/* Forward decls for prototypes */
struct frame_info;
struct frame_saved_regs;
struct type;
struct value;

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

extern CORE_ADDR d30v_skip_prologue PARAMS ((CORE_ADDR));
#define SKIP_PROLOGUE(ip) (d30v_skip_prologue (ip))


/* Stack grows downward.  */
#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/* for a breakpoint, use "dbt || nop" */
#define BREAKPOINT {0x00, 0xb0, 0x00, 0x00,\
		    0x00, 0xf0, 0x00, 0x00}

/* If your kernel resets the pc after the trap happens you may need to
   define this before including this file.  */
#define DECR_PC_AFTER_BREAK 0

#define REGISTER_NAMES \
{   "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",	\
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",	\
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",	\
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",	\
    "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",	\
    "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",	\
    "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",	\
    "r56", "r57", "r58", "r59", "r60", "r61", "r62", "r63",	\
    "spi", "spu", \
    "psw", "bpsw", "pc", "bpc", "dpsw", "dpc", "cr6", "rpt_c",	\
    "rpt_s", "rpt_e", "mod_s", "mod_e", "cr12", "cr13", "iba", "eit_vb",\
    "int_s", "int_m", "a0", "a1" \
    }

#define NUM_REGS 86

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define R0_REGNUM	0
#define FP_REGNUM	61
#define LR_REGNUM 	62
#define SP_REGNUM 	63
#define SPI_REGNUM	64	/* Interrupt stack pointer */
#define SPU_REGNUM	65	/* User stack pointer */
#define CREGS_START	66

#define PSW_REGNUM 	(CREGS_START + 0)	/* psw, bpsw, or dpsw??? */
#define     PSW_SM (((unsigned long)0x80000000) >> 0)	/* Stack mode: 0/SPI */
							/*             1/SPU */
#define     PSW_EA (((unsigned long)0x80000000) >> 2)	/* Execution status */
#define     PSW_DB (((unsigned long)0x80000000) >> 3)	/* Debug mode */
#define     PSW_DS (((unsigned long)0x80000000) >> 4)	/* Debug EIT status */
#define     PSW_IE (((unsigned long)0x80000000) >> 5)	/* Interrupt enable */
#define     PSW_RP (((unsigned long)0x80000000) >> 6)	/* Repeat enable */
#define     PSW_MD (((unsigned long)0x80000000) >> 7)	/* Modulo enable */
#define     PSW_F0 (((unsigned long)0x80000000) >> 17)	/* F0 flag */
#define     PSW_F1 (((unsigned long)0x80000000) >> 19)	/* F1 flag */
#define     PSW_F2 (((unsigned long)0x80000000) >> 21)	/* F2 flag */
#define     PSW_F3 (((unsigned long)0x80000000) >> 23)	/* F3 flag */
#define     PSW_S  (((unsigned long)0x80000000) >> 25)	/* Saturation flag */
#define     PSW_V  (((unsigned long)0x80000000) >> 27)	/* Overflow flag */
#define     PSW_VA (((unsigned long)0x80000000) >> 29)	/* Accum. overflow */
#define     PSW_C  (((unsigned long)0x80000000) >> 31)	/* Carry/Borrow flag */

#define BPSW_REGNUM	(CREGS_START + 1)	/* Backup PSW (on interrupt) */
#define PC_REGNUM 	(CREGS_START + 2)	/* pc, bpc, or dpc??? */
#define BPC_REGNUM 	(CREGS_START + 3)	/* Backup PC (on interrupt) */
#define DPSW_REGNUM	(CREGS_START + 4)	/* Backup PSW (on debug trap) */
#define DPC_REGNUM 	(CREGS_START + 5)	/* Backup PC (on debug trap) */
#define RPT_C_REGNUM	(CREGS_START + 7)	/* Loop count */
#define RPT_S_REGNUM	(CREGS_START + 8)	/* Loop start address */
#define RPT_E_REGNUM	(CREGS_START + 9)	/* Loop end address */
#define MOD_S_REGNUM	(CREGS_START + 10)
#define MOD_E_REGNUM	(CREGS_START + 11)
#define IBA_REGNUM	(CREGS_START + 14)	/* Instruction break address */
#define EIT_VB_REGNUM	(CREGS_START + 15)	/* Vector base address */
#define INT_S_REGNUM	(CREGS_START + 16)	/* Interrupt status */
#define INT_M_REGNUM	(CREGS_START + 17)	/* Interrupt mask */
#define A0_REGNUM 	84
#define A1_REGNUM 	85

/* Say how much memory is needed to store a copy of the register set */
#define REGISTER_BYTES    ((NUM_REGS - 2) * 4 + 2 * 8)

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N)  \
( ((N) >= A0_REGNUM) ? ( ((N) - A0_REGNUM) * 8 + A0_REGNUM * 4 ) : ((N) * 4) )

/* Number of bytes of storage in the actual machine representation
   for register N.  */

#define REGISTER_RAW_SIZE(N) ( ((N) >= A0_REGNUM) ? 8 : 4 )

/* Number of bytes of storage in the program's representation
   for register N.  */
#define REGISTER_VIRTUAL_SIZE(N) REGISTER_RAW_SIZE(N)

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 8

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 8

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
( ((N) < A0_REGNUM ) ? builtin_type_long : builtin_type_long_long)

/* Writing to r0 is a noop (not an error or exception or anything like
   that, however).  */

#define CANNOT_STORE_REGISTER(regno) ((regno) == R0_REGNUM)

void d30v_do_registers_info PARAMS ((int regnum, int fpregs));

#define DO_REGISTERS_INFO d30v_do_registers_info

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. 

   We store structs through a pointer passed in R2 */

#define STORE_STRUCT_RETURN(ADDR, SP) \
    { write_register (2, (ADDR));  }


/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  

   Things always get returned in R2/R3 */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  write_register_bytes (REGISTER_BYTE(2), VALBUF, TYPE_LENGTH (TYPE))


/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */
#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) (((CORE_ADDR *)(REGBUF))[2])


/* Define other aspects of the stack frame. 
   we keep a copy of the worked out return pc lying around, since it
   is a useful bit of info */

#define EXTRA_FRAME_INFO \
    CORE_ADDR return_pc; \
    CORE_ADDR dummy; \
    int frameless; \
    int size;

#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) \
    d30v_init_extra_frame_info(fromleaf, fi)

extern void d30v_init_extra_frame_info PARAMS ((int fromleaf, struct frame_info * fi));

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */

#define FRAMELESS_FUNCTION_INVOCATION(FI) \
  (frameless_look_for_prologue (FI))

CORE_ADDR d30v_frame_chain (struct frame_info *frame);
#define FRAME_CHAIN(FRAME)       d30v_frame_chain(FRAME)
extern int d30v_frame_chain_valid PARAMS ((CORE_ADDR, struct frame_info *));
#define FRAME_CHAIN_VALID(chain, thisframe) d30v_frame_chain_valid (chain, thisframe)
#define FRAME_SAVED_PC(FRAME)    ((FRAME)->return_pc)
#define FRAME_ARGS_ADDRESS(fi)   (fi)->frame
#define FRAME_LOCALS_ADDRESS(fi) (fi)->frame

void d30v_init_frame_pc (int fromleaf, struct frame_info *prev);
#define INIT_FRAME_PC_FIRST(fromleaf, prev)	d30v_init_frame_pc(fromleaf, prev)
#define INIT_FRAME_PC(fromleaf, prev)	/* nada */

/* Immediately after a function call, return the saved pc.  We can't */
/* use frame->return_pc beause that is determined by reading R62 off the */
/* stack and that may not be written yet. */

#define SAVED_PC_AFTER_CALL(frame) (read_register(LR_REGNUM))

/* Set VAL to the number of args passed to frame described by FI.
   Can set VAL to -1, meaning no way to tell.  */
/* We can't tell how many args there are */

#define FRAME_NUM_ARGS(fi) (-1)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0


/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs)	    \
   d30v_frame_find_saved_regs(frame_info, &(frame_saved_regs))

extern void d30v_frame_find_saved_regs PARAMS ((struct frame_info *, struct frame_saved_regs *));

/* DUMMY FRAMES.  Need these to support inferior function calls.
   They work like this on D30V:
   First we set a breakpoint at 0 or __start.
   Then we push all the registers onto the stack.
   Then put the function arguments in the proper registers and set r13
   to our breakpoint address.
   Finally call the function directly.
   When it hits the breakpoint, clear the break point and pop the old
   register contents off the stack. */

#define CALL_DUMMY		{ 0 }
#define PUSH_DUMMY_FRAME
#define CALL_DUMMY_START_OFFSET	0
#define CALL_DUMMY_LOCATION	AT_ENTRY_POINT
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)

extern CORE_ADDR d30v_call_dummy_address PARAMS ((void));
#define CALL_DUMMY_ADDRESS() d30v_call_dummy_address()

#define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, type, gcc_p) \
sp = d30v_fix_call_dummy (dummyname, pc, fun, nargs, args, type, gcc_p)

#define PC_IN_CALL_DUMMY(pc, sp, frame_address)	( pc == IMEM_START + 4 )

extern CORE_ADDR d30v_fix_call_dummy PARAMS ((char *, CORE_ADDR, CORE_ADDR,
					      int, struct value **,
					      struct type *, int));
#define PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \
  (d30v_push_arguments((nargs), (args), (sp), (struct_return), (struct_addr)))
extern CORE_ADDR d30v_push_arguments PARAMS ((int, struct value **, CORE_ADDR, int, CORE_ADDR));


/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
d30v_extract_return_value(TYPE, REGBUF, VALBUF)
extern void
d30v_extract_return_value PARAMS ((struct type *, char *, char *));


/* Discard from the stack the innermost frame,
   restoring all saved registers.  */
#define POP_FRAME d30v_pop_frame();
extern void d30v_pop_frame PARAMS ((void));

#define REGISTER_SIZE 4

/* Need to handle SP special, as we need to select between spu and spi.  */
#if 0				/* XXX until the simulator is fixed */
#define TARGET_READ_SP() ((read_register (PSW_REGNUM) & PSW_SM) \
			  ? read_register (SPU_REGNUM) \
			  : read_register (SPI_REGNUM))

#define TARGET_WRITE_SP(val) ((read_register (PSW_REGNUM) & PSW_SM) \
			  ? write_register (SPU_REGNUM, (val)) \
			  : write_register (SPI_REGNUM, (val)))
#endif

#define STACK_ALIGN(len)	(((len) + 7 ) & ~7)

/* Turn this on to cause remote-sim.c to use sim_set/clear_breakpoint. */

#define SIM_HAS_BREAKPOINTS

#endif /* TM_D30V_H */
