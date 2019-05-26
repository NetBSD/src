/* Header file to load module for 'compile' command.
   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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

#ifndef GDB_COMPILE_OBJECT_LOAD_H
#define GDB_COMPILE_OBJECT_LOAD_H

#include "compile-internal.h"

struct munmap_list;

struct compile_module
{
  /* objfile for the compiled module.  */
  struct objfile *objfile;

  /* .c file OBJFILE was built from.  It needs to be xfree-d.  */
  char *source_file;

  /* Inferior function GCC_FE_WRAPPER_FUNCTION.  */
  struct symbol *func_sym;

  /* Inferior registers address or NULL if the inferior function does not
     require any.  */
  CORE_ADDR regs_addr;

  /* The "scope" of this compilation.  */
  enum compile_i_scope_types scope;

  /* User data for SCOPE in use.  */
  void *scope_data;

  /* Inferior parameter out value type or NULL if the inferior function does not
     have one.  */
  struct type *out_value_type;

  /* If the inferior function has an out value, this is its address.
     Otherwise it is zero.  */
  CORE_ADDR out_value_addr;

  /* Track inferior memory reserved by inferior mmap.  */
  struct munmap_list *munmap_list_head;
};

extern struct compile_module *compile_object_load
  (const compile_file_names &fnames,
   enum compile_i_scope_types scope, void *scope_data);
extern void munmap_list_free (struct munmap_list *head);

#endif /* GDB_COMPILE_OBJECT_LOAD_H */
