/* OBSOLETE /* Target-specific definition for the Mitsubishi D30V */
/* OBSOLETE    Copyright 1997, 1998, 1999, 2000 Free Software Foundation, Inc. */
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
/* OBSOLETE    Boston, MA 02111-1307, USA.  */ */
/* OBSOLETE  */
/* OBSOLETE #ifndef TM_D30V_H */
/* OBSOLETE #define TM_D30V_H */
/* OBSOLETE  */
/* OBSOLETE #include "regcache.h" */
/* OBSOLETE  */
/* OBSOLETE /* Offset from address of function to start of its code. */
/* OBSOLETE    Zero on most machines.  */ */
/* OBSOLETE  */
/* OBSOLETE #define FUNCTION_START_OFFSET 0 */
/* OBSOLETE  */
/* OBSOLETE /* these are the addresses the D30V-EVA board maps data */ */
/* OBSOLETE /* and instruction memory to. */ */
/* OBSOLETE  */
/* OBSOLETE #define DMEM_START	0x20000000 */
/* OBSOLETE #define IMEM_START	0x00000000	/* was 0x10000000 */ */
/* OBSOLETE #define STACK_START	0x20007ffe */
/* OBSOLETE  */
/* OBSOLETE /* Forward decls for prototypes */ */
/* OBSOLETE struct frame_info; */
/* OBSOLETE struct frame_saved_regs; */
/* OBSOLETE struct type; */
/* OBSOLETE struct value; */
/* OBSOLETE  */
/* OBSOLETE /* Advance PC across any function entry prologue instructions */
/* OBSOLETE    to reach some "real" code.  */ */
/* OBSOLETE  */
/* OBSOLETE extern CORE_ADDR d30v_skip_prologue (CORE_ADDR); */
/* OBSOLETE #define SKIP_PROLOGUE(ip) (d30v_skip_prologue (ip)) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Stack grows downward.  */ */
/* OBSOLETE #define INNER_THAN(lhs,rhs) ((lhs) < (rhs)) */
/* OBSOLETE  */
/* OBSOLETE /* for a breakpoint, use "dbt || nop" */ */
/* OBSOLETE #define BREAKPOINT {0x00, 0xb0, 0x00, 0x00,\ */
/* OBSOLETE 		    0x00, 0xf0, 0x00, 0x00} */
/* OBSOLETE  */
/* OBSOLETE /* If your kernel resets the pc after the trap happens you may need to */
/* OBSOLETE    define this before including this file.  */ */
/* OBSOLETE #define DECR_PC_AFTER_BREAK 0 */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_NAMES \ */
/* OBSOLETE {   "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",	\ */
/* OBSOLETE     "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",	\ */
/* OBSOLETE     "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",	\ */
/* OBSOLETE     "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",	\ */
/* OBSOLETE     "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",	\ */
/* OBSOLETE     "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",	\ */
/* OBSOLETE     "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",	\ */
/* OBSOLETE     "r56", "r57", "r58", "r59", "r60", "r61", "r62", "r63",	\ */
/* OBSOLETE     "spi", "spu", \ */
/* OBSOLETE     "psw", "bpsw", "pc", "bpc", "dpsw", "dpc", "cr6", "rpt_c",	\ */
/* OBSOLETE     "rpt_s", "rpt_e", "mod_s", "mod_e", "cr12", "cr13", "iba", "eit_vb",\ */
/* OBSOLETE     "int_s", "int_m", "a0", "a1" \ */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE #define NUM_REGS 86 */
/* OBSOLETE  */
/* OBSOLETE /* Register numbers of various important registers. */
/* OBSOLETE    Note that some of these values are "real" register numbers, */
/* OBSOLETE    and correspond to the general registers of the machine, */
/* OBSOLETE    and some are "phony" register numbers which are too large */
/* OBSOLETE    to be actual register numbers as far as the user is concerned */
/* OBSOLETE    but do serve to get the desired values when passed to read_register.  */ */
/* OBSOLETE  */
/* OBSOLETE #define R0_REGNUM	0 */
/* OBSOLETE #define FP_REGNUM	61 */
/* OBSOLETE #define LR_REGNUM 	62 */
/* OBSOLETE #define SP_REGNUM 	63 */
/* OBSOLETE #define SPI_REGNUM	64	/* Interrupt stack pointer */ */
/* OBSOLETE #define SPU_REGNUM	65	/* User stack pointer */ */
/* OBSOLETE #define CREGS_START	66 */
/* OBSOLETE  */
/* OBSOLETE #define PSW_REGNUM 	(CREGS_START + 0)	/* psw, bpsw, or dpsw??? */ */
/* OBSOLETE #define     PSW_SM (((unsigned long)0x80000000) >> 0)	/* Stack mode: 0/SPI */ */
/* OBSOLETE 							/*             1/SPU */ */
/* OBSOLETE #define     PSW_EA (((unsigned long)0x80000000) >> 2)	/* Execution status */ */
/* OBSOLETE #define     PSW_DB (((unsigned long)0x80000000) >> 3)	/* Debug mode */ */
/* OBSOLETE #define     PSW_DS (((unsigned long)0x80000000) >> 4)	/* Debug EIT status */ */
/* OBSOLETE #define     PSW_IE (((unsigned long)0x80000000) >> 5)	/* Interrupt enable */ */
/* OBSOLETE #define     PSW_RP (((unsigned long)0x80000000) >> 6)	/* Repeat enable */ */
/* OBSOLETE #define     PSW_MD (((unsigned long)0x80000000) >> 7)	/* Modulo enable */ */
/* OBSOLETE #define     PSW_F0 (((unsigned long)0x80000000) >> 17)	/* F0 flag */ */
/* OBSOLETE #define     PSW_F1 (((unsigned long)0x80000000) >> 19)	/* F1 flag */ */
/* OBSOLETE #define     PSW_F2 (((unsigned long)0x80000000) >> 21)	/* F2 flag */ */
/* OBSOLETE #define     PSW_F3 (((unsigned long)0x80000000) >> 23)	/* F3 flag */ */
/* OBSOLETE #define     PSW_S  (((unsigned long)0x80000000) >> 25)	/* Saturation flag */ */
/* OBSOLETE #define     PSW_V  (((unsigned long)0x80000000) >> 27)	/* Overflow flag */ */
/* OBSOLETE #define     PSW_VA (((unsigned long)0x80000000) >> 29)	/* Accum. overflow */ */
/* OBSOLETE #define     PSW_C  (((unsigned long)0x80000000) >> 31)	/* Carry/Borrow flag */ */
/* OBSOLETE  */
/* OBSOLETE #define BPSW_REGNUM	(CREGS_START + 1)	/* Backup PSW (on interrupt) */ */
/* OBSOLETE #define PC_REGNUM 	(CREGS_START + 2)	/* pc, bpc, or dpc??? */ */
/* OBSOLETE #define BPC_REGNUM 	(CREGS_START + 3)	/* Backup PC (on interrupt) */ */
/* OBSOLETE #define DPSW_REGNUM	(CREGS_START + 4)	/* Backup PSW (on debug trap) */ */
/* OBSOLETE #define DPC_REGNUM 	(CREGS_START + 5)	/* Backup PC (on debug trap) */ */
/* OBSOLETE #define RPT_C_REGNUM	(CREGS_START + 7)	/* Loop count */ */
/* OBSOLETE #define RPT_S_REGNUM	(CREGS_START + 8)	/* Loop start address */ */
/* OBSOLETE #define RPT_E_REGNUM	(CREGS_START + 9)	/* Loop end address */ */
/* OBSOLETE #define MOD_S_REGNUM	(CREGS_START + 10) */
/* OBSOLETE #define MOD_E_REGNUM	(CREGS_START + 11) */
/* OBSOLETE #define IBA_REGNUM	(CREGS_START + 14)	/* Instruction break address */ */
/* OBSOLETE #define EIT_VB_REGNUM	(CREGS_START + 15)	/* Vector base address */ */
/* OBSOLETE #define INT_S_REGNUM	(CREGS_START + 16)	/* Interrupt status */ */
/* OBSOLETE #define INT_M_REGNUM	(CREGS_START + 17)	/* Interrupt mask */ */
/* OBSOLETE #define A0_REGNUM 	84 */
/* OBSOLETE #define A1_REGNUM 	85 */
/* OBSOLETE  */
/* OBSOLETE /* Say how much memory is needed to store a copy of the register set */ */
/* OBSOLETE #define REGISTER_BYTES    ((NUM_REGS - 2) * 4 + 2 * 8) */
/* OBSOLETE  */
/* OBSOLETE /* Index within `registers' of the first byte of the space for */
/* OBSOLETE    register N.  */ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_BYTE(N)  \ */
/* OBSOLETE ( ((N) >= A0_REGNUM) ? ( ((N) - A0_REGNUM) * 8 + A0_REGNUM * 4 ) : ((N) * 4) ) */
/* OBSOLETE  */
/* OBSOLETE /* Number of bytes of storage in the actual machine representation */
/* OBSOLETE    for register N.  */ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_RAW_SIZE(N) ( ((N) >= A0_REGNUM) ? 8 : 4 ) */
/* OBSOLETE  */
/* OBSOLETE /* Number of bytes of storage in the program's representation */
/* OBSOLETE    for register N.  */ */
/* OBSOLETE #define REGISTER_VIRTUAL_SIZE(N) REGISTER_RAW_SIZE(N) */
/* OBSOLETE  */
/* OBSOLETE /* Largest value REGISTER_RAW_SIZE can have.  */ */
/* OBSOLETE  */
/* OBSOLETE #define MAX_REGISTER_RAW_SIZE 8 */
/* OBSOLETE  */
/* OBSOLETE /* Largest value REGISTER_VIRTUAL_SIZE can have.  */ */
/* OBSOLETE  */
/* OBSOLETE #define MAX_REGISTER_VIRTUAL_SIZE 8 */
/* OBSOLETE  */
/* OBSOLETE /* Return the GDB type object for the "standard" data type */
/* OBSOLETE    of data in register N.  */ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_VIRTUAL_TYPE(N) \ */
/* OBSOLETE ( ((N) < A0_REGNUM ) ? builtin_type_long : builtin_type_long_long) */
/* OBSOLETE  */
/* OBSOLETE /* Writing to r0 is a noop (not an error or exception or anything like */
/* OBSOLETE    that, however).  */ */
/* OBSOLETE  */
/* OBSOLETE #define CANNOT_STORE_REGISTER(regno) ((regno) == R0_REGNUM) */
/* OBSOLETE  */
/* OBSOLETE void d30v_do_registers_info (int regnum, int fpregs); */
/* OBSOLETE  */
/* OBSOLETE #define DO_REGISTERS_INFO d30v_do_registers_info */
/* OBSOLETE  */
/* OBSOLETE /* Store the address of the place in which to copy the structure the */
/* OBSOLETE    subroutine will return.  This is called from call_function.  */
/* OBSOLETE  */
/* OBSOLETE    We store structs through a pointer passed in R2 */ */
/* OBSOLETE  */
/* OBSOLETE #define STORE_STRUCT_RETURN(ADDR, SP) \ */
/* OBSOLETE     { write_register (2, (ADDR));  } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Write into appropriate registers a function return value */
/* OBSOLETE    of type TYPE, given in virtual format.   */
/* OBSOLETE  */
/* OBSOLETE    Things always get returned in R2/R3 */ */
/* OBSOLETE  */
/* OBSOLETE #define STORE_RETURN_VALUE(TYPE,VALBUF) \ */
/* OBSOLETE   write_register_bytes (REGISTER_BYTE(2), VALBUF, TYPE_LENGTH (TYPE)) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Extract from an array REGBUF containing the (raw) register state */
/* OBSOLETE    the address in which a function should return its structure value, */
/* OBSOLETE    as a CORE_ADDR (or an expression that can be used as one).  */ */
/* OBSOLETE #define DEPRECATED_EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) (((CORE_ADDR *)(REGBUF))[2]) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Define other aspects of the stack frame.  */
/* OBSOLETE    we keep a copy of the worked out return pc lying around, since it */
/* OBSOLETE    is a useful bit of info */ */
/* OBSOLETE  */
/* OBSOLETE #define EXTRA_FRAME_INFO \ */
/* OBSOLETE     CORE_ADDR return_pc; \ */
/* OBSOLETE     CORE_ADDR dummy; \ */
/* OBSOLETE     int frameless; \ */
/* OBSOLETE     int size; */
/* OBSOLETE  */
/* OBSOLETE #define INIT_EXTRA_FRAME_INFO(fromleaf, fi) \ */
/* OBSOLETE     d30v_init_extra_frame_info(fromleaf, fi) */
/* OBSOLETE  */
/* OBSOLETE extern void d30v_init_extra_frame_info (int fromleaf, struct frame_info *fi); */
/* OBSOLETE  */
/* OBSOLETE /* A macro that tells us whether the function invocation represented */
/* OBSOLETE    by FI does not have a frame on the stack associated with it.  If it */
/* OBSOLETE    does not, FRAMELESS is set to 1, else 0.  */ */
/* OBSOLETE  */
/* OBSOLETE #define FRAMELESS_FUNCTION_INVOCATION(FI) \ */
/* OBSOLETE   (frameless_look_for_prologue (FI)) */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR d30v_frame_chain (struct frame_info *frame); */
/* OBSOLETE #define FRAME_CHAIN(FRAME)       d30v_frame_chain(FRAME) */
/* OBSOLETE extern int d30v_frame_chain_valid (CORE_ADDR, struct frame_info *); */
/* OBSOLETE #define FRAME_CHAIN_VALID(chain, thisframe) d30v_frame_chain_valid (chain, thisframe) */
/* OBSOLETE #define FRAME_SAVED_PC(FRAME)    ((FRAME)->return_pc) */
/* OBSOLETE #define FRAME_ARGS_ADDRESS(fi)   (fi)->frame */
/* OBSOLETE #define FRAME_LOCALS_ADDRESS(fi) (fi)->frame */
/* OBSOLETE  */
/* OBSOLETE void d30v_init_frame_pc (int fromleaf, struct frame_info *prev); */
/* OBSOLETE #define INIT_FRAME_PC_FIRST(fromleaf, prev)	d30v_init_frame_pc(fromleaf, prev) */
/* OBSOLETE #define INIT_FRAME_PC(fromleaf, prev)	/* nada */ */
/* OBSOLETE  */
/* OBSOLETE /* Immediately after a function call, return the saved pc.  We can't */ */
/* OBSOLETE /* use frame->return_pc beause that is determined by reading R62 off the */ */
/* OBSOLETE /* stack and that may not be written yet. */ */
/* OBSOLETE  */
/* OBSOLETE #define SAVED_PC_AFTER_CALL(frame) (read_register(LR_REGNUM)) */
/* OBSOLETE  */
/* OBSOLETE /* Set VAL to the number of args passed to frame described by FI. */
/* OBSOLETE    Can set VAL to -1, meaning no way to tell.  */ */
/* OBSOLETE /* We can't tell how many args there are */ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_NUM_ARGS(fi) (-1) */
/* OBSOLETE  */
/* OBSOLETE /* Return number of bytes at start of arglist that are not really args.  */ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_ARGS_SKIP 0 */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Put here the code to store, into a struct frame_saved_regs, */
/* OBSOLETE    the addresses of the saved registers of frame described by FRAME_INFO. */
/* OBSOLETE    This includes special registers such as pc and fp saved in special */
/* OBSOLETE    ways in the stack frame.  sp is even more special: */
/* OBSOLETE    the address we return for it IS the sp for the next frame.  */ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs)	    \ */
/* OBSOLETE    d30v_frame_find_saved_regs(frame_info, &(frame_saved_regs)) */
/* OBSOLETE  */
/* OBSOLETE extern void d30v_frame_find_saved_regs (struct frame_info *, */
/* OBSOLETE 					struct frame_saved_regs *); */
/* OBSOLETE  */
/* OBSOLETE /* DUMMY FRAMES.  Need these to support inferior function calls. */
/* OBSOLETE    They work like this on D30V: */
/* OBSOLETE    First we set a breakpoint at 0 or __start. */
/* OBSOLETE    Then we push all the registers onto the stack. */
/* OBSOLETE    Then put the function arguments in the proper registers and set r13 */
/* OBSOLETE    to our breakpoint address. */
/* OBSOLETE    Finally call the function directly. */
/* OBSOLETE    When it hits the breakpoint, clear the break point and pop the old */
/* OBSOLETE    register contents off the stack. */ */
/* OBSOLETE  */
/* OBSOLETE #define CALL_DUMMY		{ 0 } */
/* OBSOLETE #define PUSH_DUMMY_FRAME */
/* OBSOLETE #define CALL_DUMMY_START_OFFSET	0 */
/* OBSOLETE #define CALL_DUMMY_LOCATION	AT_ENTRY_POINT */
/* OBSOLETE #define CALL_DUMMY_BREAKPOINT_OFFSET (0) */
/* OBSOLETE  */
/* OBSOLETE extern CORE_ADDR d30v_call_dummy_address (void); */
/* OBSOLETE #define CALL_DUMMY_ADDRESS() d30v_call_dummy_address() */
/* OBSOLETE  */
/* OBSOLETE #define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, type, gcc_p) \ */
/* OBSOLETE sp = d30v_fix_call_dummy (dummyname, pc, fun, nargs, args, type, gcc_p) */
/* OBSOLETE  */
/* OBSOLETE #define PC_IN_CALL_DUMMY(pc, sp, frame_address)	( pc == IMEM_START + 4 ) */
/* OBSOLETE  */
/* OBSOLETE extern CORE_ADDR d30v_fix_call_dummy (char *, CORE_ADDR, CORE_ADDR, */
/* OBSOLETE 				      int, struct value **, */
/* OBSOLETE 				      struct type *, int); */
/* OBSOLETE #define PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \ */
/* OBSOLETE   (d30v_push_arguments((nargs), (args), (sp), (struct_return), (struct_addr))) */
/* OBSOLETE extern CORE_ADDR d30v_push_arguments (int, struct value **, CORE_ADDR, int, */
/* OBSOLETE 				      CORE_ADDR); */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Extract from an array REGBUF containing the (raw) register state */
/* OBSOLETE    a function return value of type TYPE, and copy that, in virtual format, */
/* OBSOLETE    into VALBUF.  */ */
/* OBSOLETE  */
/* OBSOLETE #define DEPRECATED_EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \ */
/* OBSOLETE d30v_extract_return_value(TYPE, REGBUF, VALBUF) */
/* OBSOLETE extern void d30v_extract_return_value (struct type *, char *, char *); */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Discard from the stack the innermost frame, */
/* OBSOLETE    restoring all saved registers.  */ */
/* OBSOLETE #define POP_FRAME d30v_pop_frame(); */
/* OBSOLETE extern void d30v_pop_frame (void); */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_SIZE 4 */
/* OBSOLETE  */
/* OBSOLETE /* Need to handle SP special, as we need to select between spu and spi.  */ */
/* OBSOLETE #if 0				/* XXX until the simulator is fixed */ */
/* OBSOLETE #define TARGET_READ_SP() ((read_register (PSW_REGNUM) & PSW_SM) \ */
/* OBSOLETE 			  ? read_register (SPU_REGNUM) \ */
/* OBSOLETE 			  : read_register (SPI_REGNUM)) */
/* OBSOLETE  */
/* OBSOLETE #define TARGET_WRITE_SP(val) ((read_register (PSW_REGNUM) & PSW_SM) \ */
/* OBSOLETE 			  ? write_register (SPU_REGNUM, (val)) \ */
/* OBSOLETE 			  : write_register (SPI_REGNUM, (val))) */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE #define STACK_ALIGN(len)	(((len) + 7 ) & ~7) */
/* OBSOLETE  */
/* OBSOLETE /* Turn this on to cause remote-sim.c to use sim_set/clear_breakpoint. */ */
/* OBSOLETE  */
/* OBSOLETE #define SIM_HAS_BREAKPOINTS */
/* OBSOLETE  */
/* OBSOLETE #endif /* TM_D30V_H */ */
