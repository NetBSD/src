/* Call module for 'compile' command.

   Copyright (C) 2014-2015 Free Software Foundation, Inc.

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

/* Helper for do_module_cleanup.  */

struct do_module_cleanup
{
  /* Boolean to set true upon a call of do_module_cleanup.
     The pointer may be NULL.  */
  int *executedp;

  /* .c file OBJFILE was built from.  It needs to be xfree-d.  */
  char *source_file;

  /* objfile_name of our objfile.  */
  char objfile_name_string[1];
};

/* Cleanup everything after the inferior function dummy frame gets
   discarded.  */

static dummy_frame_dtor_ftype do_module_cleanup;
static void
do_module_cleanup (void *arg)
{
  struct do_module_cleanup *data = arg;
  struct objfile *objfile;

  if (data->executedp != NULL)
    *data->executedp = 1;

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
  volatile struct gdb_exception ex;
  const char *objfile_name_s = objfile_name (module->objfile);
  int dtor_found, executed = 0;
  CORE_ADDR func_addr = module->func_addr;
  CORE_ADDR regs_addr = module->regs_addr;

  data = xmalloc (sizeof (*data) + strlen (objfile_name_s));
  data->executedp = &executed;
  data->source_file = xstrdup (module->source_file);
  strcpy (data->objfile_name_string, objfile_name_s);

  xfree (module->source_file);
  xfree (module);

  TRY_CATCH (ex, RETURN_MASK_ERROR)
    {
      func_val = value_from_pointer
		 (builtin_type (target_gdbarch ())->builtin_func_ptr,
		  func_addr);

      if (regs_addr == 0)
	call_function_by_hand_dummy (func_val, 0, NULL,
				     do_module_cleanup, data);
      else
	{
	  struct value *arg_val;

	  arg_val = value_from_pointer
		    (builtin_type (target_gdbarch ())->builtin_func_ptr,
		     regs_addr);
	  call_function_by_hand_dummy (func_val, 1, &arg_val,
				       do_module_cleanup, data);
	}
    }
  dtor_found = find_dummy_frame_dtor (do_module_cleanup, data);
  if (!executed)
    data->executedp = NULL;
  if (ex.reason >= 0)
    gdb_assert (!dtor_found && executed);
  else
    {
      /* In the case od DTOR_FOUND or in the case of EXECUTED nothing
	 needs to be done.  */
      gdb_assert (!(dtor_found && executed));
      if (!dtor_found && !executed)
	do_module_cleanup (data);
      throw_exception (ex);
    }
}
