/* Dynamic architecture support for GDB, the GNU debugger.
   Copyright 1998-2000, Free Software Foundation, Inc.

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

#ifndef GDBARCH_UTILS_H
#define GDBARCH_UTILS_H

/* gdbarch trace variable */
extern int gdbarch_debug;

/* Fallback for register convertible. */
extern gdbarch_register_convertible_ftype generic_register_convertible_not;

/* Helper function for targets that don't know how my arguments are
   being passed */
extern gdbarch_frame_num_args_ftype frame_num_args_unknown;

/* Implementation of breakpoint from PC using any of the deprecated
   macros BREAKPOINT, LITTLE_BREAKPOINT, BIG_BREAPOINT.  For legacy
   targets that don't yet implement their own breakpoint_from_pc(). */
extern gdbarch_breakpoint_from_pc_ftype legacy_breakpoint_from_pc;

/* Frameless functions not identifable. */
extern gdbarch_frameless_function_invocation_ftype generic_frameless_function_invocation_not;

/* Map onto old REGISTER_NAMES. */
extern char *legacy_register_name (int i);

/* Backward compatible call_dummy_words. */
extern LONGEST legacy_call_dummy_words[];
extern int legacy_sizeof_call_dummy_words;

/* Typical remote_translate_xfer_address */
extern gdbarch_remote_translate_xfer_address_ftype generic_remote_translate_xfer_address;

#endif
