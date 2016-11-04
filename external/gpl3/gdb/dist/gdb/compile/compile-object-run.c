/* Call module for 'compile' command.

   Copyright (C) 2014-2016 Free Software Foundation, Inc.

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

#include "defs.h"
#include "compile-object-run.h"
#include "value.h"
#include "infcall.h"
#include "objfiles.h"
#include "compile-internal.h"
#include "dummy-frame.h"
#include "block.h"
#include "valprint.h"
#include "compile.h"

/* Helper for do_module_cleanup.  */

struct do_module_cleanup
{
  /* Boolean to set true upon a call of do_module_cleanup.
     The pointer may be NULL.  */
  int *executedp;

  /* .c file OBJFILE was built from.  It needs to be xfree-d.  */
  char *source_file;

  /* Copy from struct compile_module.  */
  enum compile_i_scope_types scope;
  void *scope_data;

  /* Copy from struct compile_module.  */
  struct type *out_value_type;
  CORE_ADDR out_value_addr;

  /* Copy from struct compile_module.  */
  struct munmap_list *munmap_list_head;

  /* objfile_name of our objfile.  */
  char objfile_name_string[1];
};

/* Cleanup everything after the inferior function dummy frame gets
   discarded.  */

static dummy_frame_dtor_ftype do_module_cleanup;
static void
do_module_cleanup (void *arg, int registers_valid)
{
  struct do_module_cleanup *data = (struct do_module_cleanup *) arg;
  struct objfile *objfile;

  if (data->executedp != NULL)
    {
      *data->executedp = 1;

      /* This code cannot be in compile_object_run as OUT_VALUE_TYPE
	 no longer exists there.  */
      if (data->scope == COMPILE_I_PRINT_ADDRESS_SCOPE
	  || data->scope == COMPILE_I_PRINT_VALUE_SCOPE)
	{
	  struct value *addr_value;
	  struct type *ptr_type = lookup_pointer_type (data->out_value_type);

	  addr_value = value_from_pointer (ptr_type, data->out_value_addr);

	  /* SCOPE_DATA would be stale unlesse EXECUTEDP != NULL.  */
	  compile_print_value (value_ind (addr_value), data->scope_data);
	}
    }

  ALL_OBJFILES (objfile)
    if ((objfile->flags & OBJF_USERLOADED) == 0
        && (strcmp (objfile_name (objfile), data->objfile_name_string) == 0))
      {
	free_objfile (objfile);

	/* It may be a bit too pervasive in this dummy_frame dtor callback.  */
	clear_symtab_users (0);

	break;
      }

  /* Delete the .c file.  */
  unlink (data->source_file);
  xfree (data->source_file);

  munmap_list_free (data->munmap_list_head);

  /* Delete the .o file.  */
  unlink (data->objfile_name_string);
  xfree (data);
}

/* Perform inferior call of MODULE.  This function may throw an error.
   This function may leave files referenced by MODULE on disk until
   the inferior call dummy frame is discarded.  This function may throw errors.
   Thrown errors and left MODULE files are unrelated events.  Caller must no
   longer touch MODULE's memory after this function has been called.  */

void
compile_object_run (struct compile_module *module)
{
  struct value *func_val;
  struct frame_id dummy_id;
  struct cleanup *cleanups;
  struct do_module_cleanup *data;
  const char *objfile_name_s = objfile_name (module->objfile);
  int dtor_found, executed = 0;
  struct symbol *func_sym = module->func_sym;
  CORE_ADDR regs_addr = module->regs_addr;
  struct objfile *objfile = module->objfile;

  data = (struct do_module_cleanup *) xmalloc (sizeof (*data)
					       + strlen (objfile_name_s));
  data->executedp = &executed;
  data->source_file = xstrdup (module->source_file);
  strcpy (data->objfile_name_string, objfile_name_s);
  data->scope = module->scope;
  data->scope_data = module->scope_data;
  data->out_value_type = module->out_value_type;
  data->out_value_addr = module->out_value_addr;
  data->munmap_list_head = module->munmap_list_head;

  xfree (module->source_file);
  xfree (module);
  module = NULL;

  TRY
    {
      struct type *func_type = SYMBOL_TYPE (func_sym);
      htab_t copied_types;
      int current_arg = 0;
      struct value **vargs;

      /* OBJFILE may disappear while FUNC_TYPE still will be in use.  */
      copied_types = create_copied_types_hash (objfile);
      func_type = copy_type_recursive (objfile, func_type, copied_types);
      htab_delete (copied_types);

      gdb_assert (TYPE_CODE (func_type) == TYPE_CODE_FUNC);
      func_val = value_from_pointer (lookup_pointer_type (func_type),
				   BLOCK_START (SYMBOL_BLOCK_VALUE (func_sym)));

      vargs = XALLOCAVEC (struct value *, TYPE_NFIELDS (func_type));
      if (TYPE_NFIELDS (func_type) >= 1)
	{
	  gdb_assert (regs_addr != 0);
	  vargs[current_arg] = value_from_pointer
			  (TYPE_FIELD_TYPE (func_type, current_arg), regs_addr);
	  ++current_arg;
	}
      if (TYPE_NFIELDS (func_type) >= 2)
	{
	  gdb_assert (data->out_value_addr != 0);
	  vargs[current_arg] = value_from_pointer
	       (TYPE_FIELD_TYPE (func_type, current_arg), data->out_value_addr);
	  ++current_arg;
	}
      gdb_assert (current_arg == TYPE_NFIELDS (func_type));
      call_function_by_hand_dummy (func_val, TYPE_NFIELDS (func_type), vargs,
				   do_module_cleanup, data);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      /* In the case of DTOR_FOUND or in the case of EXECUTED nothing
	 needs to be done.  */
      dtor_found = find_dummy_frame_dtor (do_module_cleanup, data);
      if (!executed)
	data->executedp = NULL;
      gdb_assert (!(dtor_found && executed));
      if (!dtor_found && !executed)
	do_module_cleanup (data, 0);
      throw_exception (ex);
    }
  END_CATCH

  dtor_found = find_dummy_frame_dtor (do_module_cleanup, data);
  gdb_assert (!dtor_found && executed);
}
