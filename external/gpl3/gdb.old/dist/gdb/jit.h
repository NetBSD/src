/* JIT declarations for GDB, the GNU Debugger.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef JIT_H
#define JIT_H

/* When the JIT breakpoint fires, the inferior wants us to take one of
   these actions.  These values are used by the inferior, so the
   values of these enums cannot be changed.  */

typedef enum
{
  JIT_NOACTION = 0,
  JIT_REGISTER,
  JIT_UNREGISTER
} jit_actions_t;

/* This struct describes a single symbol file in a linked list of
   symbol files describing generated code.  As the inferior generates
   code, it adds these entries to the list, and when we attach to the
   inferior, we read them all.  For the first element prev_entry
   should be NULL, and for the last element next_entry should be
   NULL.  */

struct jit_code_entry
{
  CORE_ADDR next_entry;
  CORE_ADDR prev_entry;
  CORE_ADDR symfile_addr;
  ULONGEST symfile_size;
};

/* This is the global descriptor that the inferior uses to communicate
   information to the debugger.  To alert the debugger to take an
   action, the inferior sets the action_flag to the appropriate enum
   value, updates relevant_entry to point to the relevant code entry,
   and calls the function at the well-known symbol with our
   breakpoint.  We then read this descriptor from another global
   well-known symbol.  */

struct jit_descriptor
{
  uint32_t version;
  /* This should be jit_actions_t, but we want to be specific about the
     bit-width.  */
  uint32_t action_flag;
  CORE_ADDR relevant_entry;
  CORE_ADDR first_entry;
};

/* Looks for the descriptor and registration symbols and breakpoints
   the registration function.  If it finds both, it registers all the
   already JITed code.  If it has already found the symbols, then it
   doesn't try again.  */

extern void jit_inferior_created_hook (void);

/* Re-establish the jit breakpoint(s).  */

extern void jit_breakpoint_re_set (void);

/* This function is called by handle_inferior_event when it decides
   that the JIT event breakpoint has fired.  */

extern void jit_event_handler (struct gdbarch *gdbarch);

#endif /* JIT_H */
