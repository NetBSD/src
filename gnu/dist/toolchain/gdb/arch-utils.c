/* Dynamic architecture support for GDB, the GNU debugger.
   Copyright 1998-1999, Free Software Foundation, Inc.

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

#include "defs.h"

#if GDB_MULTI_ARCH
#include "gdbcmd.h"
#include "inferior.h"		/* enum CALL_DUMMY_LOCATION et.al. */
#else
/* Just include everything in sight so that the every old definition
   of macro is visible. */
#include "gdb_string.h"
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "breakpoint.h"
#include "gdb_wait.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "target.h"
#include "gdbthread.h"
#include "annotate.h"
#include "symfile.h"		/* for overlay functions */
#endif

/* Convenience macro for allocting typesafe memory. */

#ifndef XMALLOC
#define XMALLOC(TYPE) (TYPE*) xmalloc (sizeof (TYPE))
#endif


/* Use the program counter to determine the contents and size
   of a breakpoint instruction.  If no target-dependent macro
   BREAKPOINT_FROM_PC has been defined to implement this function,
   assume that the breakpoint doesn't depend on the PC, and
   use the values of the BIG_BREAKPOINT and LITTLE_BREAKPOINT macros.
   Return a pointer to a string of bytes that encode a breakpoint
   instruction, stores the length of the string to *lenptr,
   and optionally adjust the pc to point to the correct memory location
   for inserting the breakpoint.  */

unsigned char *
legacy_breakpoint_from_pc (CORE_ADDR * pcptr, int *lenptr)
{
  /* {BIG_,LITTLE_}BREAKPOINT is the sequence of bytes we insert for a
     breakpoint.  On some machines, breakpoints are handled by the
     target environment and we don't have to worry about them here.  */
#ifdef BIG_BREAKPOINT
  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
    {
      static unsigned char big_break_insn[] = BIG_BREAKPOINT;
      *lenptr = sizeof (big_break_insn);
      return big_break_insn;
    }
#endif
#ifdef LITTLE_BREAKPOINT
  if (TARGET_BYTE_ORDER != BIG_ENDIAN)
    {
      static unsigned char little_break_insn[] = LITTLE_BREAKPOINT;
      *lenptr = sizeof (little_break_insn);
      return little_break_insn;
    }
#endif
#ifdef BREAKPOINT
  {
    static unsigned char break_insn[] = BREAKPOINT;
    *lenptr = sizeof (break_insn);
    return break_insn;
  }
#endif
  *lenptr = 0;
  return NULL;
}

int
generic_frameless_function_invocation_not (struct frame_info *fi)
{
  return 0;
}

char *
legacy_register_name (int i)
{
#ifdef REGISTER_NAMES
  static char *names[] = REGISTER_NAMES;
  if (i < 0 || i >= (sizeof (names) / sizeof (*names)))
    return NULL;
  else
    return names[i];
#else
  internal_error ("legacy_register_name: called.");
  return NULL;
#endif
}

#if defined (CALL_DUMMY)
LONGEST legacy_call_dummy_words[] = CALL_DUMMY;
#else
LONGEST legacy_call_dummy_words[1];
#endif
int legacy_sizeof_call_dummy_words = sizeof (legacy_call_dummy_words);

void
generic_remote_translate_xfer_address (CORE_ADDR gdb_addr, int gdb_len,
				       CORE_ADDR * rem_addr, int *rem_len)
{
  *rem_addr = gdb_addr;
  *rem_len = gdb_len;
}

/* */

extern initialize_file_ftype __initialize_gdbarch_utils;

void
__initialize_gdbarch_utils (void)
{
}
