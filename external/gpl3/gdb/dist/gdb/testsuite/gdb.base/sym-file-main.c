/* Copyright 2013-2014 Free Software Foundation, Inc.
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>

#include "sym-file-loader.h"

void
gdb_add_symbol_file (void *addr, const char *file)
{
  return;
}

void
gdb_remove_symbol_file (void *addr)
{
  return;
}

/* Load a shared library without relying on the standard
   loader to test GDB's commands for adding and removing
   symbol files at runtime.  */

int
main (int argc, const char *argv[])
{
  const char *file = SHLIB_NAME;
  Elf_External_Ehdr *ehdr = NULL;
  struct segment *head_seg = NULL;
  Elf_External_Shdr *text;
  char *text_addr = NULL;
  int (*pbar) () = NULL;
  int (*pfoo) (int) = NULL;

  if (load_shlib (file, &ehdr, &head_seg) != 0)
    return -1;

  /* Get the text section.  */
  text = find_shdr (ehdr, ".text");
  if (text == NULL)
    return -1;

  /* Notify GDB to add the symbol file.  */
  if (translate_offset (GET (text, sh_offset), head_seg, (void **) &text_addr)
      != 0)
    return -1;

  gdb_add_symbol_file (text_addr, file);

  /* Call bar from SHLIB_NAME.  */
  if (lookup_function ("bar", ehdr, head_seg, (void *) &pbar) != 0)
    return -1;

  (*pbar) ();

  /* Call foo from SHLIB_NAME.  */
  if (lookup_function ("foo", ehdr, head_seg, (void *) &pfoo) != 0)
    return -1;

  (*pfoo) (2);

  /* Notify GDB to remove the symbol file.  */
  gdb_remove_symbol_file (text_addr);

  return 0;
}
