/* Definitions to target GDB to Linux on m680x0
   Copyright (C) 1996,1998 Free Software Foundation, Inc.

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

/* Number of traps that happen between exec'ing the shell to run an
   inferior, and when we finally get to the inferior code.  This is 2
   on most implementations.  */

#define START_INFERIOR_TRAPS_EXPECTED 2

/* The following definitions are appropriate when using the ELF
   format, where floating point values are returned in fp0, pointer
   values in a0 and other values in d0.  */

/* Extract from an array REGBUF containing the (raw) register state a
   function return value of type TYPE, and copy that, in virtual
   format, into VALBUF.  */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
{									\
  if (TYPE_CODE (TYPE) == TYPE_CODE_FLT)				\
    {									\
       REGISTER_CONVERT_TO_VIRTUAL (FP0_REGNUM, TYPE,			\
				    ((char *) (REGBUF)			\
				     + REGISTER_BYTE (FP0_REGNUM)),	\
				    VALBUF);				\
    }									\
  else if (TYPE_CODE (TYPE) == TYPE_CODE_PTR)				\
    memcpy (VALBUF, (char *) (REGBUF) + REGISTER_BYTE (A0_REGNUM),	\
	    TYPE_LENGTH (TYPE));					\
  else									\
    memcpy (VALBUF,							\
	    ((char *) (REGBUF)						\
	     + (TYPE_LENGTH (TYPE) >= 4 ? 0 : 4 - TYPE_LENGTH (TYPE))),	\
	    TYPE_LENGTH (TYPE));					\
}

/* Write into appropriate registers a function return value of type
   TYPE, given in virtual format.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
{									\
  if (TYPE_CODE (TYPE) == TYPE_CODE_FLT)				\
    {									\
      char raw_buffer[REGISTER_RAW_SIZE (FP0_REGNUM)];			\
      REGISTER_CONVERT_TO_RAW (TYPE, FP0_REGNUM, VALBUF, raw_buffer);	\
      write_register_bytes (REGISTER_BYTE (FP0_REGNUM),			\
			    raw_buffer, TYPE_LENGTH (TYPE));		\
    }									\
  else									\
    {									\
      if (TYPE_CODE (TYPE) == TYPE_CODE_PTR)				\
	write_register_bytes (REGISTER_BYTE (A0_REGNUM), VALBUF,	\
			      TYPE_LENGTH (TYPE));			\
      write_register_bytes (0, VALBUF, TYPE_LENGTH (TYPE));		\
    }									\
}

#include "tm-linux.h"
#include "m68k/tm-m68k.h"

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#undef EXTRACT_STRUCT_VALUE_ADDRESS
#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  (*(CORE_ADDR *)((char *) (REGBUF) + REGISTER_BYTE (A0_REGNUM)))

/* Offsets (in target ints) into jmp_buf.  */

#define JB_ELEMENT_SIZE 4
#define JB_PC 7

/* Figure out where the longjmp will land.  Slurp the args out of the stack.
   We expect the first arg to be a pointer to the jmp_buf structure from which
   we extract the pc (JB_PC) that we will land at.  The pc is copied into ADDR.
   This routine returns true on success */

#define GET_LONGJMP_TARGET(ADDR) get_longjmp_target(ADDR)

/* Offset to saved PC in sigcontext, from <asm/sigcontext.h>.  */
#define SIGCONTEXT_PC_OFFSET 26

#undef FRAME_SAVED_PC
#define FRAME_SAVED_PC(FRAME) \
  (((FRAME)->signal_handler_caller \
    ? sigtramp_saved_pc (FRAME) \
    : read_memory_integer ((FRAME)->frame + 4, 4)))

extern CORE_ADDR sigtramp_saved_pc PARAMS ((struct frame_info *));

#define IN_SIGTRAMP(pc,name) in_sigtramp (pc)
extern int in_sigtramp PARAMS ((CORE_ADDR pc));
