/* Parameters for execution on a Matsushita mn10300 processor.
   Copyright 1996, 1997 Free Software Foundation, Inc. 

   Contributed by Geoffrey Noer <noer@cygnus.com>

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

/* The mn10300 is little endian.  */
#define TARGET_BYTE_ORDER_DEFAULT LITTLE_ENDIAN

/* All registers are 32bits (phew!).  */
#define REGISTER_SIZE 4
#define MAX_REGISTER_RAW_SIZE 4
#define NUM_REGS 32

#define REGISTER_VIRTUAL_TYPE(REG) builtin_type_int

#define REGISTER_BYTE(REG) ((REG) * 4)
#define REGISTER_VIRTUAL_SIZE(REG) 4
#define REGISTER_RAW_SIZE(REG) 4

#define MAX_REGISTER_VIRTUAL_SIZE 4

#define REGISTER_BYTES (NUM_REGS * REGISTER_SIZE)

extern char *mn10300_register_name PARAMS ((int regnr));
#define REGISTER_NAME(i) (mn10300_register_name (i))

#define D2_REGNUM 2
#define D3_REGNUM 3
#define A2_REGNUM 6
#define A3_REGNUM 7
#define SP_REGNUM 8
#define PC_REGNUM 9
#define MDR_REGNUM 10
#define PSW_REGNUM 11
#define LIR_REGNUM 12
#define LAR_REGNUM 13
#define E0_REGNUM 14

/* Pseudo register that contains true address of executing stack frame */
#define FP_REGNUM 31

/* BREAKPOINT_FROM_PC uses the program counter value to determine the
   breakpoint that should be used */
extern breakpoint_from_pc_fn mn10300_breakpoint_from_pc;
#define BREAKPOINT_FROM_PC(pcptr, lenptr) mn10300_breakpoint_from_pc (pcptr, lenptr)

#define FUNCTION_START_OFFSET 0

#define DECR_PC_AFTER_BREAK 0

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

extern CORE_ADDR mn10300_saved_pc_after_call PARAMS ((struct frame_info * frame));
#define SAVED_PC_AFTER_CALL(frame) \
  mn10300_saved_pc_after_call (frame)

struct frame_info;
struct type;
struct value;

extern void mn10300_init_extra_frame_info PARAMS ((struct frame_info *));
#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) mn10300_init_extra_frame_info (fi)
#define INIT_FRAME_PC		/* Not necessary */

#define FRAME_INIT_SAVED_REGS(fi)	/* handled by init_extra_frame_info */

extern CORE_ADDR mn10300_frame_chain PARAMS ((struct frame_info *));
#define FRAME_CHAIN(fi) mn10300_frame_chain (fi)
#define FRAME_CHAIN_VALID(FP, FI)	generic_file_frame_chain_valid (FP, FI)

extern CORE_ADDR mn10300_find_callers_reg PARAMS ((struct frame_info *, int));
extern CORE_ADDR mn10300_frame_saved_pc PARAMS ((struct frame_info *));
#define FRAME_SAVED_PC(FI) (mn10300_frame_saved_pc (FI))

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF. */

extern void mn10300_extract_return_value PARAMS ((struct type * type, char *regbuf, char *valbuf));
#define EXTRACT_RETURN_VALUE(TYPE, REGBUF, VALBUF) \
  mn10300_extract_return_value (TYPE, REGBUF, VALBUF)

CORE_ADDR mn10300_extract_struct_value_address PARAMS ((char *regbuf));
#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  mn10300_extract_struct_value_address (REGBUF)

void mn10300_store_return_value PARAMS ((struct type * type, char *valbuf));
#define STORE_RETURN_VALUE(TYPE, VALBUF) \
  mn10300_store_return_value (TYPE, VALBUF)


extern CORE_ADDR mn10300_store_struct_return (CORE_ADDR addr, CORE_ADDR sp);
#define STORE_STRUCT_RETURN(STRUCT_ADDR, SP) \
  (mn10300_store_struct_return (STRUCT_ADDR, SP))

extern CORE_ADDR mn10300_skip_prologue PARAMS ((CORE_ADDR));
#define SKIP_PROLOGUE(pc) (mn10300_skip_prologue (pc))

#define FRAME_ARGS_SKIP 0

#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)
#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)
#define FRAME_NUM_ARGS(fi) (-1)

extern void mn10300_pop_frame PARAMS ((struct frame_info *));
#define POP_FRAME mn10300_pop_frame (get_current_frame ())

#define USE_GENERIC_DUMMY_FRAMES 1
#define CALL_DUMMY                   {0}
#define CALL_DUMMY_START_OFFSET      (0)
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)
#define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
#define FIX_CALL_DUMMY(DUMMY, START, FUNADDR, NARGS, ARGS, TYPE, GCCP)
#define CALL_DUMMY_ADDRESS()         entry_point_address ()

#define TARGET_READ_FP() read_sp ()

extern CORE_ADDR mn10300_push_return_address PARAMS ((CORE_ADDR, CORE_ADDR));
#define PUSH_RETURN_ADDRESS(PC, SP)  mn10300_push_return_address (PC, SP)

#define PUSH_DUMMY_FRAME	generic_push_dummy_frame ()
#define SAVE_DUMMY_FRAME_TOS(SP) generic_save_dummy_frame_tos (SP)

extern CORE_ADDR
  mn10300_push_arguments PARAMS ((int, struct value **, CORE_ADDR,
				  unsigned char, CORE_ADDR));
#define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
  (mn10300_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR))

#define PC_IN_CALL_DUMMY(PC, SP, FP) generic_pc_in_call_dummy (PC, SP, FP)

#define REG_STRUCT_HAS_ADDR(gcc_p,TYPE) \
	(TYPE_LENGTH (TYPE) > 8)

extern use_struct_convention_fn mn10300_use_struct_convention;
#define USE_STRUCT_CONVENTION(GCC_P, TYPE) mn10300_use_struct_convention (GCC_P, TYPE)

/* override the default get_saved_register function with
   one that takes account of generic CALL_DUMMY frames */
#define GET_SAVED_REGISTER(raw_buffer, optimized, addrp, frame, regnum, lval) \
    generic_get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval)

/* Cons up virtual frame pointer for trace */
extern void mn10300_virtual_frame_pointer PARAMS ((CORE_ADDR, long *, long *));
#define TARGET_VIRTUAL_FRAME_POINTER(PC, REGP, OFFP) \
	mn10300_virtual_frame_pointer ((PC), (REGP), (OFFP))
