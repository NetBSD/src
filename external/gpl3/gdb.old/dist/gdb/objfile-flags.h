/* Definition of objfile flags.

   Copyright (C) 1992-2023 Free Software Foundation, Inc.

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

#if !defined (OBJFILE_FLAGS_H)
#define OBJFILE_FLAGS_H

#include "gdbsupport/enum-flags.h"

/* Defines for the objfile flags field.  Defined in a separate file to
   break circular header dependencies.  */

enum objfile_flag : unsigned
  {
    /* When an object file has its functions reordered (currently
       Irix-5.2 shared libraries exhibit this behaviour), we will need
       an expensive algorithm to locate a partial symtab or symtab via
       an address.  To avoid this penalty for normal object files, we
       use this flag, whose setting is determined upon symbol table
       read in.  */
    OBJF_REORDERED = 1 << 0,	/* Functions are reordered */

    /* Distinguish between an objfile for a shared library and a
       "vanilla" objfile.  This may come from a target's
       implementation of the solib interface, from add-symbol-file, or
       any other mechanism that loads dynamic objects.  */
    OBJF_SHARED = 1 << 1,	/* From a shared library */

    /* User requested that this objfile be read in it's entirety.  */
    OBJF_READNOW = 1 << 2,	/* Immediate full read */

    /* This objfile was created because the user explicitly caused it
       (e.g., used the add-symbol-file command).  This bit offers a
       way for run_command to remove old objfile entries which are no
       longer valid (i.e., are associated with an old inferior), but
       to preserve ones that the user explicitly loaded via the
       add-symbol-file command.  */
    OBJF_USERLOADED = 1 << 3,	/* User loaded */

    /* Set if we have tried to read partial symtabs for this objfile.
       This is used to allow lazy reading of partial symtabs.  */
    OBJF_PSYMTABS_READ = 1 << 4,

    /* Set if this is the main symbol file (as opposed to symbol file
       for dynamically loaded code).  */
    OBJF_MAINLINE = 1 << 5,

    /* ORIGINAL_NAME and OBFD->FILENAME correspond to text description
       unrelated to filesystem names.  It can be for example
       "<image in memory>".  */
    OBJF_NOT_FILENAME = 1 << 6,

    /* User requested that we do not read this objfile's symbolic
       information.  */
    OBJF_READNEVER = 1 << 7,
  };

DEF_ENUM_FLAGS_TYPE (enum objfile_flag, objfile_flags);

#endif /* !defined (OBJFILE_FLAGS_H) */
