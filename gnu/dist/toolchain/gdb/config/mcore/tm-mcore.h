/* Parameters for execution on a Motorola MCore.
   Copyright (C) 1995 Free Software Foundation, Inc.

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
   Boston, MA 02111-1307, USA. */

/* The mcore is little endian (by default) */
#define TARGET_BYTE_ORDER_DEFAULT LITTLE_ENDIAN

/* All registers are 32 bits */
#define REGISTER_SIZE 4
#define MAX_REGISTER_RAW_SIZE 4

#define REGISTER_VIRTUAL_TYPE(REG) builtin_type_int

#define REGISTER_BYTE(REG) ((REG) * REGISTER_SIZE)
#define REGISTER_VIRTUAL_SIZE(REG) 4
#define REGISTER_RAW_SIZE(REG) 4

#define MAX_REGISTER_VIRTUAL_SIZE 4

#define REGISTER_BYTES (NUM_REGS * REGISTER_SIZE)

extern char *mcore_register_names[];
#define REGISTER_NAME(I) mcore_register_names[I]

/* Registers. The Motorola MCore contains:

   16 32-bit general purpose registers (r0-r15)
   16 32-bit alternate file registers (ar0-ar15)
   32 32-bit control registers (cr0-cr31)
   + 1 pc
   ------
   65 registers */
#define NUM_REGS 65
#define PC_REGNUM 64
#define SP_REGNUM 0
#define FP_REGNUM (SP_REGNUM)
#define PR_REGNUM 15
#define FIRST_ARGREG 2
#define LAST_ARGREG 7
#define RETVAL_REGNUM 2

/* Offset from address of function to start of its code.
   Zero on most machines.  */
#define FUNCTION_START_OFFSET 0

#define DECR_PC_AFTER_BREAK 0

/* BREAKPOINT_FROM_PC uses the program counter value to determine
   the breakpoint that should be used. */
extern breakpoint_from_pc_fn mcore_breakpoint_from_pc;
#define BREAKPOINT_FROM_PC(PCPTR, LENPTR) mcore_breakpoint_from_pc (PCPTR, LENPTR)

#define INNER_THAN(LHS,RHS) ((LHS) < (RHS))

#define SAVED_PC_AFTER_CALL(FRAME)  read_register (PR_REGNUM)

struct frame_info;
struct type;
struct value;

extern void mcore_init_extra_frame_info (struct frame_info *fi);
#define INIT_EXTRA_FRAME_INFO(FROMLEAF, FI) mcore_init_extra_frame_info ((FI))
#define INIT_FRAME_PC		/* Not necessary */
#define FRAME_INIT_SAVED_REGS(FI)	/* handled by init_extra_frame_info */

extern CORE_ADDR mcore_frame_chain (struct frame_info *fi);
#define FRAME_CHAIN(FI) mcore_frame_chain ((FI))
#define FRAME_CHAIN_VALID(FP, FI) generic_file_frame_chain_valid ((FP), (FI))

extern CORE_ADDR mcore_frame_saved_pc (struct frame_info *);
#define FRAME_SAVED_PC(FI) (mcore_frame_saved_pc ((FI)))

/* Extracting/storing return values. */
extern void mcore_store_return_value (struct type *type, char *valbuf);
#define STORE_RETURN_VALUE(TYPE, VALBUF) mcore_store_return_value ((TYPE), (VALBUF))

extern void mcore_extract_return_value (struct type *type, char *regbut, char *valbuf);
#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
    mcore_extract_return_value ((TYPE), (REGBUF), (VALBUF));

#define STORE_STRUCT_RETURN(ADDR, SP)	/* handled by mcore_push_arguments */

extern CORE_ADDR mcore_extract_struct_value_address (char *regbuf);
#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
    mcore_extract_struct_value_address (REGBUF)

extern CORE_ADDR mcore_skip_prologue (CORE_ADDR pc);
#define SKIP_PROLOGUE(PC) (PC) = mcore_skip_prologue ((PC))

#define FRAME_ARGS_SKIP 0
extern CORE_ADDR mcore_frame_args_address (struct frame_info *fi);
#define FRAME_ARGS_ADDRESS(FI) mcore_frame_args_address ((FI))
extern CORE_ADDR mcore_frame_locals_address (struct frame_info *fi);
#define FRAME_LOCALS_ADDRESS(FI) mcore_frame_locals_address ((FI))
#define FRAME_NUM_ARGS(FI) (-1)


extern void mcore_pop_frame (struct frame_info *fi);
#define POP_FRAME mcore_pop_frame (get_current_frame ())

#define USE_GENERIC_DUMMY_FRAMES 1
#define CALL_DUMMY                   {0}
#define CALL_DUMMY_START_OFFSET      (0)
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)
#define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
#define FIX_CALL_DUMMY(DUMMY, START, FUNADDR, NARGS, ARGS, TYPE, GCCP)
#define CALL_DUMMY_ADDRESS()         entry_point_address ()
#define SIZEOF_CALL_DUMMY_WORDS      0
#define SAVE_DUMMY_FRAME_TOS(SP)     generic_save_dummy_frame_tos (SP)

extern CORE_ADDR mcore_push_return_address PARAMS ((CORE_ADDR, CORE_ADDR));
#define PUSH_RETURN_ADDRESS(PC, SP)  mcore_push_return_address (PC, SP)

#define PUSH_DUMMY_FRAME	generic_push_dummy_frame ()

extern CORE_ADDR mcore_push_arguments PARAMS ((int, struct value **, CORE_ADDR,
					       unsigned char, CORE_ADDR));
#define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
  (SP) = mcore_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR)

#define PC_IN_CALL_DUMMY(PC, SP, FP) generic_pc_in_call_dummy (PC, SP, FP)

/* MCore will never pass a sturcture by reference. It will always be split
   between registers and stack. */
#define REG_STRUCT_HAS_ADDR(GCC_P, TYPE) 0

extern use_struct_convention_fn mcore_use_struct_convention;
#define USE_STRUCT_CONVENTION(GCC_P, TYPE) mcore_use_struct_convention (GCC_P, TYPE)

/* override the default get_saved_register function with
   one that takes account of generic CALL_DUMMY frames */
#define GET_SAVED_REGISTER(raw_buffer, optimized, addrp, frame, regnum, lval) \
    generic_get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval)

/* Cons up virtual frame pointer for trace */
extern void mcore_virtual_frame_pointer PARAMS ((CORE_ADDR, long *, long *));
#define TARGET_VIRTUAL_FRAME_POINTER(PC, REGP, OFFP) \
	mcore_virtual_frame_pointer ((PC), (REGP), (OFFP))

/* MCore can be bi-endian. */
#define TARGET_BYTE_ORDER_SELECTABLE_P 1

/* For PE, gcc will tell us what th real type of
   arguments are when it promotes arguments. */
#define BELIEVE_PCC_PROMOTION 1
