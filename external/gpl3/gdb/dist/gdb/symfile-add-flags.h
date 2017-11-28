/* Definition of symfile add flags.

   Copyright (C) 1990-2017 Free Software Foundation, Inc.

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

#if !defined (SYMFILE_ADD_FLAGS_H)
#define SYMFILE_ADD_FLAGS_H

#include "common/enum-flags.h"

/* This enum encodes bit-flags passed as ADD_FLAGS parameter to
   symbol_file_add, etc.  Defined in a separate file to break circular
   header dependencies.  */

enum symfile_add_flag
  {
    /* Be chatty about what you are doing.  */
    SYMFILE_VERBOSE = 1 << 1,

    /* This is the main symbol file (as opposed to symbol file for
       dynamically loaded code).  */
    SYMFILE_MAINLINE = 1 << 2,

    /* Do not call breakpoint_re_set when adding this symbol file.  */
    SYMFILE_DEFER_BP_RESET = 1 << 3,

    /* Do not immediately read symbols for this file.  By default,
       symbols are read when the objfile is created.  */
    SYMFILE_NO_READ = 1 << 4
  };

DEF_ENUM_FLAGS_TYPE (enum symfile_add_flag, symfile_add_flags);

#endif /* !defined(SYMFILE_ADD_FLAGS_H) */
