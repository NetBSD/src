/* Parameters for execution on a Matsushita mn10200 processor.
   Copyright 1997 Free Software Foundation, Inc. 

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* The mn10200 is little endian.  */
#define TARGET_BYTE_ORDER LITTLE_ENDIAN

/* ints are only 16bits on the mn10200.  */
#undef TARGET_INT_BIT
#define TARGET_INT_BIT 16

/* The mn10200 doesn't support long long types.  */
#undef TARGET_LONG_LONG_BIT
#define TARGET_LONG_LONG_BIT 32

/* The mn10200 doesn't support double or long double either.  */
#undef TARGET_DOUBLE_BIT
#undef TARGET_LONG_DOUBLE_BIT
#define TARGET_DOUBLE_BIT 32
#define TARGET_LONG_DOUBLE_BIT 32

/* Not strictly correct, but the machine independent code is not
   ready to handle any of the basic sizes not being a power of two.  */
#undef TARGET_PTR_BIT
#define TARGET_PTR_BIT 32

/* The mn10200 really has 24 bit registers but the simulator reads/writes
   them as 32bit values, so we claim they're 32bits each.  This may have
   to be tweaked if the Matsushita emulator/board really deals with them
   as 24bits each.  */
#define REGISTER_SIZE 4

#define MAX_REGISTER_RAW_SIZE REGISTER_SIZE
#define NUM_REGS 11

#define REGISTER_BYTES (NUM_REGS * REGISTER_SIZE)

#define REGISTER_NAMES \
{ "d0", "d1", "d2", "d3", "a0", "a1", "a2", "sp", \
  "pc", "mdr", "psw"}

#define FP_REGNUM 6
#define SP_REGNUM 7
#define PC_REGNUM 8
#define MDR_REGNUM 9
#define PSW_REGNUM 10

/* Treat the registers as 32bit values.  */
#define REGISTER_VIRTUAL_TYPE(REG) builtin_type_long

#define REGISTER_BYTE(REG) ((REG) * REGISTER_SIZE)
#define REGISTER_VIRTUAL_SIZE(REG) REGISTER_SIZE
#define REGISTER_RAW_SIZE(REG) REGISTER_SIZE

#define MAX_REGISTER_VIRTUAL_SIZE REGISTER_SIZE

/* The breakpoint instruction must be the same size as te smallest
   instruction in the instruction set.

   The Matsushita mn10x00 processors have single byte instructions
   so we need a single byte breakpoint.  Matsushita hasn't defined
   one, so we defined it ourselves.

   0xff is the only available single byte insn left on the mn10200.  */
#define BREAKPOINT {0xff}

#define FUNCTION_START_OFFSET 0

#define DECR_PC_AFTER_BREAK 0

/* Stacks grow the normal way.  */
#define INNER_THAN <

#define SAVED_PC_AFTER_CALL(frame) \
  (read_memory_integer (read_register (SP_REGNUM), REGISTER_SIZE) & 0xffff)

#ifdef __STDC__
struct frame_info;
struct frame_saved_regs;
struct type;
struct value;
#endif

#define EXTRA_FRAME_INFO struct frame_saved_regs fsr; int status; int stack_size;

extern void mn10200_init_extra_frame_info PARAMS ((struct frame_info *));
#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) mn10200_init_extra_frame_info (fi)
#define INIT_FRAME_PC(x,y)

extern void mn10200_frame_find_saved_regs PARAMS ((struct frame_info *,
						   struct frame_saved_regs *));
#define FRAME_FIND_SAVED_REGS(fi, regaddr) regaddr = fi->fsr

extern CORE_ADDR mn10200_frame_chain PARAMS ((struct frame_info *));
#define FRAME_CHAIN(fi) mn10200_frame_chain (fi)
#define FRAME_CHAIN_VALID(FP, FI)	generic_frame_chain_valid (FP, FI)

extern CORE_ADDR mn10200_find_callers_reg PARAMS ((struct frame_info *, int));
extern CORE_ADDR mn10200_frame_saved_pc   PARAMS ((struct frame_info *));
#define FRAME_SAVED_PC(FI) (mn10200_frame_saved_pc (FI))

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

#define EXTRACT_RETURN_VALUE(TYPE, REGBUF, VALBUF) \
  { \
    if (TYPE_LENGTH (TYPE) > 8) \
      abort (); \
    else if (TYPE_LENGTH (TYPE) > 2 && TYPE_CODE (TYPE) != TYPE_CODE_PTR) \
      { \
	memcpy (VALBUF, REGBUF + REGISTER_BYTE (0), 2); \
	memcpy (VALBUF + 2, REGBUF + REGISTER_BYTE (1), 2); \
      } \
    else if (TYPE_CODE (TYPE) == TYPE_CODE_PTR)\
      { \
        memcpy (VALBUF, REGBUF + REGISTER_BYTE (4), TYPE_LENGTH (TYPE)); \
      } \
    else \
      { \
        memcpy (VALBUF, REGBUF + REGISTER_BYTE (0), TYPE_LENGTH (TYPE)); \
      } \
  }

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  extract_address (REGBUF + REGISTER_BYTE (4), \
		   REGISTER_RAW_SIZE (4))

#define STORE_RETURN_VALUE(TYPE, VALBUF) \
  { \
    if (TYPE_LENGTH (TYPE) > 8) \
      abort (); \
    else if (TYPE_LENGTH (TYPE) > 2 && TYPE_CODE (TYPE) != TYPE_CODE_PTR) \
      { \
	write_register_bytes (REGISTER_BYTE (0), VALBUF, 2); \
	write_register_bytes (REGISTER_BYTE (1), VALBUF + 2, 2); \
      } \
    else if (TYPE_CODE (TYPE) == TYPE_CODE_PTR)\
      { \
        write_register_bytes (REGISTER_BYTE (4), VALBUF, TYPE_LENGTH (TYPE)); \
      } \
    else \
      { \
        write_register_bytes (REGISTER_BYTE (0), VALBUF, TYPE_LENGTH (TYPE)); \
      } \
  }

#define STORE_STRUCT_RETURN(STRUCT_ADDR, SP) \
  (SP) = mn10200_store_struct_return (STRUCT_ADDR, SP)

extern CORE_ADDR mn10200_skip_prologue PARAMS ((CORE_ADDR));
#define SKIP_PROLOGUE(pc) pc = mn10200_skip_prologue (pc)

#define FRAME_ARGS_SKIP 0

#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)
#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)
#define FRAME_NUM_ARGS(val, fi) ((val) = -1)

extern void mn10200_pop_frame PARAMS ((struct frame_info *));
#define POP_FRAME mn10200_pop_frame (get_current_frame ())

#define USE_GENERIC_DUMMY_FRAMES
#define CALL_DUMMY                   {0}
#define CALL_DUMMY_START_OFFSET      (0)
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)
#define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
#define FIX_CALL_DUMMY(DUMMY, START, FUNADDR, NARGS, ARGS, TYPE, GCCP)
#define CALL_DUMMY_ADDRESS()         entry_point_address ()

extern CORE_ADDR mn10200_push_return_address PARAMS ((CORE_ADDR, CORE_ADDR));
#define PUSH_RETURN_ADDRESS(PC, SP)  mn10200_push_return_address (PC, SP)

#define PUSH_DUMMY_FRAME	generic_push_dummy_frame ()

extern CORE_ADDR
mn10200_push_arguments PARAMS ((int, struct value **, CORE_ADDR,
			     unsigned char, CORE_ADDR));
#define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
  (SP) = mn10200_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR)

#define PC_IN_CALL_DUMMY(PC, SP, FP) generic_pc_in_call_dummy (PC, SP)

#define REG_STRUCT_HAS_ADDR(gcc_p,TYPE) \
  	(TYPE_LENGTH (TYPE) > 8)

#define USE_STRUCT_CONVENTION(GCC_P, TYPE) \
  	(TYPE_NFIELDS (TYPE) > 1 || TYPE_LENGTH (TYPE) > 8)

/* Override the default get_saved_register function with
   one that takes account of generic CALL_DUMMY frames.  */
#define GET_SAVED_REGISTER

/* Define this for Wingdb */
#define TARGET_MN10200
