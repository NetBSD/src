/* Target definitions for delta68.
   Copyright 1993, 1994, 1998, 1999, 2000 Free Software Foundation, Inc.

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

/* Define BPT_VECTOR if it is different than the default.
   This is the vector number used by traps to indicate a breakpoint. */

#define BPT_VECTOR 0x1

#define GCC_COMPILED_FLAG_SYMBOL "gcc_compiled%"
#define GCC2_COMPILED_FLAG_SYMBOL "gcc2_compiled%"

/* Amount PC must be decremented by after a breakpoint.
   On the Delta, the kernel decrements it for us.  */

#define DECR_PC_AFTER_BREAK 0

/* Not sure what happens if we try to store this register, but
   phdm@info.ucl.ac.be says we need this define.  */

#define CANNOT_STORE_REGISTER(regno)	(regno == FPI_REGNUM)

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

/* When it returns a float/double value, use fp0 in sysV68.  */
/* When it returns a pointer value, use a0 in sysV68.  */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF)			\
  if (TYPE_CODE (TYPE) == TYPE_CODE_FLT)				\
    REGISTER_CONVERT_TO_VIRTUAL (FP0_REGNUM, TYPE,			\
				 &REGBUF[REGISTER_BYTE (FP0_REGNUM)],	\
				 VALBUF);				\
  else									\
    memcpy ((VALBUF),							\
	    (char *) ((REGBUF) +					\
		      (TYPE_CODE(TYPE) == TYPE_CODE_PTR ? 8 * 4 :	\
		       (TYPE_LENGTH(TYPE) >= 4 ? 0 : 4 - TYPE_LENGTH(TYPE)))), \
	    TYPE_LENGTH(TYPE))

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

/* When it returns a float/double value, use fp0 in sysV68.  */
/* When it returns a pointer value, use a0 in sysV68.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  if (TYPE_CODE (TYPE) == TYPE_CODE_FLT)				\
      {									\
	char raw_buf[REGISTER_RAW_SIZE (FP0_REGNUM)];			\
	REGISTER_CONVERT_TO_RAW (TYPE, FP0_REGNUM, VALBUF, raw_buf);	\
	write_register_bytes (REGISTER_BYTE (FP0_REGNUM),		\
			      raw_buf, REGISTER_RAW_SIZE (FP0_REGNUM)); \
      }									\
  else									\
    write_register_bytes ((TYPE_CODE(TYPE) == TYPE_CODE_PTR ? 8 * 4 : 0), \
			  VALBUF, TYPE_LENGTH (TYPE))

/* Return number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

extern int delta68_frame_num_args PARAMS ((struct frame_info * fi));
#define FRAME_NUM_ARGS(fi) (delta68_frame_num_args ((fi)))

/* On M68040 versions of sysV68 R3V7.1, ptrace(PT_WRITE_I) does not clear
   the processor's instruction cache as it should.  */
#define CLEAR_INSN_CACHE()	clear_insn_cache()

#include "m68k/tm-m68k.h"

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#undef EXTRACT_STRUCT_VALUE_ADDRESS
#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF)\
	(*(CORE_ADDR *)((char*)(REGBUF) + 8 * 4))

extern int delta68_in_sigtramp PARAMS ((CORE_ADDR pc, char * name));
#define IN_SIGTRAMP(pc,name) delta68_in_sigtramp (pc, name)

extern CORE_ADDR delta68_frame_saved_pc PARAMS ((struct frame_info * fi));
#undef FRAME_SAVED_PC
#define FRAME_SAVED_PC(fi) delta68_frame_saved_pc (fi)

extern CORE_ADDR delta68_frame_args_address PARAMS ((struct frame_info * fi));
#undef FRAME_ARGS_ADDRESS
#define FRAME_ARGS_ADDRESS(fi) delta68_frame_args_address (fi)
