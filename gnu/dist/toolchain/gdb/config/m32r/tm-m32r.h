/* Parameters for execution on a Mitsubishi m32r processor.
   Copyright 1996, 1997 Free Software Foundation, Inc. 

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

/* Used by mswin.  */
#define TARGET_M32R 1

/* mvs_check TARGET_BYTE_ORDER BIG_ENDIAN */
#define TARGET_BYTE_ORDER BIG_ENDIAN

/* mvs_check REGISTER_NAMES */
#define REGISTER_NAMES \
{ "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", \
  "r8", "r9", "r10", "r11", "r12", "fp", "lr", "sp", \
  "psw", "cbr", "spi", "spu", "bpc", "pc", "accl", "acch", \
    /*  "cond", "sm", "bsm", "ie", "bie", "bcarry",  */ \
}
/* mvs_check  NUM_REGS */
#define NUM_REGS 			24

/* mvs_check  REGISTER_SIZE */
#define REGISTER_SIZE 			4
/* mvs_check  MAX_REGISTER_RAW_SIZE */
#define MAX_REGISTER_RAW_SIZE		4

/* mvs_check  *_REGNUM */
#define R0_REGNUM	0
#define STRUCT_RETURN_REGNUM 0
#define ARG0_REGNUM	0
#define ARGLAST_REGNUM	3
#define V0_REGNUM	0
#define V1_REGNUM	1
#define FP_REGNUM	13
#define RP_REGNUM	14
#define SP_REGNUM	15
#define PSW_REGNUM	16
#define CBR_REGNUM	17
#define SPI_REGNUM	18
#define SPU_REGNUM	19
#define BPC_REGNUM	20
#define PC_REGNUM	21
#define ACCL_REGNUM	22
#define ACCH_REGNUM	23

/* mvs_check  REGISTER_BYTES */
#define REGISTER_BYTES			(NUM_REGS * 4)

/* mvs_check  REGISTER_VIRTUAL_TYPE */
#define REGISTER_VIRTUAL_TYPE(REG)	builtin_type_int

/* mvs_check  REGISTER_BYTE */
#define REGISTER_BYTE(REG) 		((REG) * 4)
/* mvs_check  REGISTER_VIRTUAL_SIZE */
#define REGISTER_VIRTUAL_SIZE(REG) 	4
/* mvs_check  REGISTER_RAW_SIZE */
#define REGISTER_RAW_SIZE(REG)     	4

/* mvs_check  MAX_REGISTER_VIRTUAL_SIZE */
#define MAX_REGISTER_VIRTUAL_SIZE 	4

/* mvs_check  BREAKPOINT */
#define BREAKPOINT {0x10, 0xf1}

/* mvs_no_check  FUNCTION_START_OFFSET */
#define FUNCTION_START_OFFSET 0

/* mvs_check  DECR_PC_AFTER_BREAK */
#define DECR_PC_AFTER_BREAK 0

/* mvs_check  INNER_THAN */
#define INNER_THAN(lhs,rhs) ((lhs) < (rhs))

/* mvs_check  SAVED_PC_AFTER_CALL */
#define SAVED_PC_AFTER_CALL(fi) read_register (RP_REGNUM)

struct frame_info;
struct frame_saved_regs;
struct type;
struct value;

/* Define other aspects of the stack frame. 
   We keep the offsets of all saved registers, 'cause we need 'em a lot!
   We also keep the current size of the stack frame, and whether 
   the frame pointer is valid (for frameless functions, and when we're
   still in the prologue of a function with a frame) */

/* mvs_check  EXTRA_FRAME_INFO */
#define EXTRA_FRAME_INFO  	\
  struct frame_saved_regs fsr;	\
  int framesize;		\
  int using_frame_pointer;


extern void m32r_init_extra_frame_info PARAMS ((struct frame_info * fi));
/* mvs_check  INIT_EXTRA_FRAME_INFO */
#define INIT_EXTRA_FRAME_INFO(fromleaf, fi) m32r_init_extra_frame_info (fi)
/* mvs_no_check  INIT_FRAME_PC */
#define INIT_FRAME_PC		/* Not necessary */

extern void
m32r_frame_find_saved_regs PARAMS ((struct frame_info * fi,
				    struct frame_saved_regs * regaddr));

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

/* mvs_check  FRAME_FIND_SAVED_REGS */
#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs)	    \
   m32r_frame_find_saved_regs(frame_info, &(frame_saved_regs))

extern CORE_ADDR m32r_frame_chain PARAMS ((struct frame_info * fi));
/* mvs_check  FRAME_CHAIN */
#define FRAME_CHAIN(fi) 		m32r_frame_chain (fi)

#define FRAME_CHAIN_VALID(fp, frame)	generic_file_frame_chain_valid (fp, frame)

extern CORE_ADDR m32r_find_callers_reg PARAMS ((struct frame_info * fi,
						int regnum));
extern CORE_ADDR m32r_frame_saved_pc PARAMS ((struct frame_info *));
/* mvs_check  FRAME_SAVED_PC */
#define FRAME_SAVED_PC(fi)		m32r_frame_saved_pc (fi)

/* mvs_check  EXTRACT_RETURN_VALUE */
#define EXTRACT_RETURN_VALUE(TYPE, REGBUF, VALBUF) \
  memcpy ((VALBUF), \
	  (char *)(REGBUF) + REGISTER_BYTE (V0_REGNUM) + \
	  ((TYPE_LENGTH (TYPE) > 4 ? 8 : 4) - TYPE_LENGTH (TYPE)), \
	  TYPE_LENGTH (TYPE))

/* mvs_check  STORE_RETURN_VALUE */
#define STORE_RETURN_VALUE(TYPE, VALBUF) \
  write_register_bytes(REGISTER_BYTE (V0_REGNUM) + \
		       ((TYPE_LENGTH (TYPE) > 4 ? 8:4) - TYPE_LENGTH (TYPE)),\
		       (VALBUF), TYPE_LENGTH (TYPE));

extern CORE_ADDR m32r_skip_prologue PARAMS ((CORE_ADDR pc));
/* mvs_check  SKIP_PROLOGUE */
#define SKIP_PROLOGUE(pc) (m32r_skip_prologue (pc))

/* mvs_no_check  FRAME_ARGS_SKIP */
#define FRAME_ARGS_SKIP 0

/* mvs_no_check  FRAME_ARGS_ADDRESS */
#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)
/* mvs_no_check  FRAME_LOCALS_ADDRESS */
#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)
/* mvs_no_check  FRAME_NUM_ARGS */
#define FRAME_NUM_ARGS(fi) (-1)

#define COERCE_FLOAT_TO_DOUBLE(formal, actual) (1)

extern void m32r_write_sp (CORE_ADDR val);
#define TARGET_WRITE_SP m32r_write_sp






/* struct passing and returning stuff */
#define STORE_STRUCT_RETURN(STRUCT_ADDR, SP)	\
	write_register (0, STRUCT_ADDR)

extern use_struct_convention_fn m32r_use_struct_convention;
#define USE_STRUCT_CONVENTION(GCC_P, TYPE) m32r_use_struct_convention (GCC_P, TYPE)

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  extract_address (REGBUF + REGISTER_BYTE (V0_REGNUM), \
		   REGISTER_RAW_SIZE (V0_REGNUM))

#define REG_STRUCT_HAS_ADDR(gcc_p,type)     (TYPE_LENGTH (type) > 8)


/* generic dummy frame stuff */

#define PUSH_DUMMY_FRAME             generic_push_dummy_frame ()
#define PC_IN_CALL_DUMMY(PC, SP, FP) generic_pc_in_call_dummy (PC, SP, FP)


/* target-specific dummy_frame stuff */

extern struct frame_info *m32r_pop_frame PARAMS ((struct frame_info * frame));
/* mvs_check  POP_FRAME */
#define POP_FRAME m32r_pop_frame (get_current_frame ())

/* mvs_no_check  STACK_ALIGN */
/* #define STACK_ALIGN(x) ((x + 3) & ~3) */

extern CORE_ADDR m32r_push_return_address PARAMS ((CORE_ADDR, CORE_ADDR));
extern CORE_ADDR m32r_push_arguments PARAMS ((int nargs,
					      struct value ** args,
					      CORE_ADDR sp,
					      unsigned char struct_return,
					      CORE_ADDR struct_addr));



/* mvs_no_check  PUSH_ARGUMENTS */
#define PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
  (m32r_push_arguments (NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR))

#define PUSH_RETURN_ADDRESS(PC, SP)      m32r_push_return_address (PC, SP)

/* override the standard get_saved_register function with 
   one that takes account of generic CALL_DUMMY frames */
#define GET_SAVED_REGISTER(raw_buffer, optimized, addrp, frame, regnum, lval) \
     generic_get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval)


#define USE_GENERIC_DUMMY_FRAMES 1
#define CALL_DUMMY                   {0}
#define CALL_DUMMY_LENGTH            (0)
#define CALL_DUMMY_START_OFFSET      (0)
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)
#define FIX_CALL_DUMMY(DUMMY1, STARTADDR, FUNADDR, NARGS, ARGS, TYPE, GCCP)
#define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
#define CALL_DUMMY_ADDRESS()         entry_point_address ()
