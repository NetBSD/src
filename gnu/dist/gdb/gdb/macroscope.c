/* Functions for deciding which macros are currently in scope.
   Copyright 2002 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

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

#include "macroscope.h"
#include "symtab.h"
#include "target.h"
#include "frame.h"
#include "inferior.h"


struct macro_scope *
sal_macro_scope (struct symtab_and_line sal)
{
  struct macro_source_file *main;
  struct macro_scope *ms;

  if (! sal.symtab
      || ! sal.symtab->macro_table)
    return 0;

  ms = (struct macro_scope *) xmalloc (sizeof (*ms));

  main = macro_main (sal.symtab->macro_table);
  ms->file = macro_lookup_inclusion (main, sal.symtab->filename);

  if (! ms->file)
    internal_error
      (__FILE__, __LINE__,
       "\n"
       "the symtab `%s' refers to a preprocessor macro table which doesn't\n"
       "have any record of processing a file by that name.\n",
       sal.symtab->filename);

  ms->line = sal.line;

  return ms;
}


struct macro_scope *
default_macro_scope (void)
{
  struct symtab_and_line sal;
  struct macro_source_file *main;
  struct macro_scope *ms;

  /* If there's a selected frame, use its PC.  */ 
  if (selected_frame)
    sal = find_pc_line (selected_frame->pc, 0);
  
  /* If the target has any registers at all, then use its PC.  Why we
     would have registers but no stack, I'm not sure.  */
  else if (target_has_registers)
    sal = find_pc_line (read_pc (), 0);

  /* If all else fails, fall back to the current listing position.  */
  else
    {
      /* Don't call select_source_symtab here.  That can raise an
         error if symbols aren't loaded, but GDB calls the expression
         evaluator in all sorts of contexts.

         For example, commands like `set width' call the expression
         evaluator to evaluate their numeric arguments.  If the
         current language is C, then that may call this function to
         choose a scope for macro expansion.  If you don't have any
         symbol files loaded, then select_source_symtab will raise an
         error.  But `set width' shouldn't raise an error just because
         it can't decide which scope to macro-expand its argument in.  */
      sal.symtab = current_source_symtab;
      sal.line = current_source_line;
    }

  return sal_macro_scope (sal);
}


/* Look up the definition of the macro named NAME in scope at the source
   location given by BATON, which must be a pointer to a `struct
   macro_scope' structure.  */
struct macro_definition *
standard_macro_lookup (const char *name, void *baton)
{
  struct macro_scope *ms = (struct macro_scope *) baton;

  return macro_lookup_definition (ms->file, ms->line, name);
}
