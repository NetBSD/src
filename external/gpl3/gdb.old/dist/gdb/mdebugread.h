/* Read a symbol table in ECOFF format (Third-Eye).

   Copyright (C) 2004-2023 Free Software Foundation, Inc.

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

#ifndef MDEBUGREAD_H
#define MDEBUGREAD_H

#include "coff/sym.h"		/* Needed for PDR below.  */
#include "coff/symconst.h"

/* Specific information about a procedure.  Architectures (ab)uses
   this to save memory.  */

struct mdebug_extra_func_info
{
  long numargs;		/* Number of args to procedure (was iopt).  */
  PDR pdr;		/* Procedure descriptor record.  */
};

/* Special symbol found in blocks associated with routines.  We can
   hang mdebug_extra_func_info's off of this.  */

#define MDEBUG_EFI_SYMBOL_NAME "__GDB_EFI_INFO__"

extern void mdebug_build_psymtabs (minimal_symbol_reader &,
				   struct objfile *,
				   const struct ecoff_debug_swap *,
				   struct ecoff_debug_info *);

extern void elfmdebug_build_psymtabs (struct objfile *,
				      const struct ecoff_debug_swap *,
				      asection *);

#endif /* MDEBUGREAD_H */
