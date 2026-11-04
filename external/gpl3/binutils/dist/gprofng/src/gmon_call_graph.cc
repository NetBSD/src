/* call_graph.c  -  Create call graphs.

   Copyright (C) 1999-2026 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "config.h"
#include "util.h"
#include "Elf.h" /* link to bfd.h */
#include "gp-gmon.h"

#include "symtab.h"
#include "cg_arcs.h"
#include "call_graph.h"
#include "gmon_io.h"
#include "gmon_out.h"

void
cg_tally (bfd_vma from_pc, bfd_vma self_pc,
	  unsigned long count, const char *whoami)
{
  Sym *parent;
  Sym *child;
  Sym_Table *symtab = get_symtab (whoami);

  parent = sym_lookup (symtab, from_pc);
  child = sym_lookup (symtab, self_pc);

  if (child == NULL || parent == NULL)
    return;

  /* If we're doing line-by-line profiling, both the parent and the
     child will probably point to line symbols instead of function
     symbols.  For the parent this is fine, since this identifies the
     line number in the calling routing, but the child should always
     point to a function entry point, so we back up in the symbol
     table until we find it.

     For normal profiling, is_func will be set on all symbols, so this
     code will do nothing.  */
  while (child >= symtab->base && ! child->is_func)
    --child;

  if (child < symtab->base)
    return;

  child->ncalls += count;
  arc_add (parent, child, count);
}

/* Read a record from file IFP describing an arc in the function
   call-graph and the count of how many times the arc has been
   traversed.  FILENAME is the name of file IFP and is provided for
   formatting error-messages only.  */

void
cg_read_rec (FILE *ifp, const char *filename, const char *whoami)
{
  bfd_vma from_pc, self_pc;
  unsigned int count;

  if (gmon_io_read_vma (ifp, &from_pc, whoami)
      || gmon_io_read_vma (ifp, &self_pc, whoami)
      || gmon_io_read_32 (ifp, &count))
    {
      fprintf (stderr, "%s: %s: unexpected end of file\n",
	       whoami, filename);
      done (1);
    }

  DBG (SAMPLEDEBUG,
       printf ("[cg_read_rec] frompc 0x%lx selfpc 0x%lx count %lu\n",
	       (unsigned long) from_pc, (unsigned long) self_pc,
	       (unsigned long) count));
  /* Add this arc:  */
  cg_tally (from_pc, self_pc, count, whoami);
}
