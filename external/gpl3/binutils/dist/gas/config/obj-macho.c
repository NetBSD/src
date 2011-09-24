/* Mach-O object file format
   Copyright 2009 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3,
   or (at your option) any later version.

   GAS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#define OBJ_HEADER "obj-macho.h"

#include "as.h"
#include "mach-o.h"

static void
obj_mach_o_weak (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  int c;
  symbolS *symbolP;

  do
    {
      /* Get symbol name.  */
      name = input_line_pointer;
      c = get_symbol_end ();
      symbolP = symbol_find_or_make (name);
      S_SET_WEAK (symbolP);
      *input_line_pointer = c;
      SKIP_WHITESPACE ();

      if (c != ',')
        break;
      input_line_pointer++;
      SKIP_WHITESPACE ();
    }
  while (*input_line_pointer != '\n');
  demand_empty_rest_of_line ();
}

const pseudo_typeS mach_o_pseudo_table[] =
{
  {"weak", obj_mach_o_weak, 0},

  {NULL, NULL, 0}
};
