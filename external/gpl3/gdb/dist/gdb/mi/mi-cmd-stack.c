/* MI Command Set - stack commands.
   Copyright (C) 2000-2013 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "target.h"
#include "frame.h"
#include "value.h"
#include "mi-cmds.h"
#include "ui-out.h"
#include "symtab.h"
#include "block.h"
#include "stack.h"
#include "dictionary.h"
#include "gdb_string.h"
#include "language.h"
#include "valprint.h"
#include "exceptions.h"

enum what_to_list { locals, arguments, all };

static void list_args_or_locals (enum what_to_list what, 
				 enum print_values values,
				 struct frame_info *fi);

/* Print a list of the stack frames.  Args can be none, in which case
   we want to print the whole backtrace, or a pair of numbers
   specifying the frame numbers at which to start and stop the
   display.  If the two numbers are equal, a single frame will be
   displayed.  */

void
mi_cmd_stack_list_frames (char *command, char **argv, int argc)
{
  int frame_low;
  int frame_high;
  int i;
  struct cleanup *cleanup_stack;
  struct frame_info *fi;

  if (argc > 2 || argc == 1)
    error (_("-stack-list-frames: Usage: [FRAME_LOW FRAME_HIGH]"));

  if (argc == 2)
    {
      frame_low = atoi (argv[0]);
      frame_high = atoi (argv[1]);
    }
  else
    {
      /* Called with no arguments, it means we want the whole
         backtrace.  */
      frame_low = -1;
      frame_high = -1;
    }

  /* Let's position fi on the frame at which to start the
     display. Could be the innermost frame if the whole stack needs
     displaying, or if frame_low is 0.  */
  for (i = 0, fi = get_current_frame ();
       fi && i < frame_low;
       i++, fi = get_prev_frame (fi));

  if (fi == NULL)
    error (_("-stack-list-frames: Not enough frames in stack."));

  cleanup_stack = make_cleanup_ui_out_list_begin_end (current_uiout, "stack");

  /* Now let's print the frames up to frame_high, or until there are
     frames in the stack.  */
  for (;
       fi && (i <= frame_high || frame_high == -1);
       i++, fi = get_prev_frame (fi))
    {
      QUIT;
      /* Print the location and the address always, even for level 0.
         If args is 0, don't print the arguments.  */
      print_frame_info (fi, 1, LOC_AND_ADDRESS, 0 /* args */ );
    }

  do_cleanups (cleanup_stack);
}

void
mi_cmd_stack_info_depth (char *command, char **argv, int argc)
{
  int frame_high;
  int i;
  struct frame_info *fi;

  if (argc > 1)
    error (_("-stack-info-depth: Usage: [MAX_DEPTH]"));

  if (argc == 1)
    frame_high = atoi (argv[0]);
  else
    /* Called with no arguments, it means we want the real depth of
       the stack.  */
    frame_high = -1;

  for (i = 0, fi = get_current_frame ();
       fi && (i < frame_high || frame_high == -1);
       i++, fi = get_prev_frame (fi))
    QUIT;

  ui_out_field_int (current_uiout, "depth", i);
}

static enum print_values
parse_print_values (char *name)
{
   if (strcmp (name, "0") == 0
       || strcmp (name, mi_no_values) == 0)
     return PRINT_NO_VALUES;
   else if (strcmp (name, "1") == 0
	    || strcmp (name, mi_all_values) == 0)
     return PRINT_ALL_VALUES;
   else if (strcmp (name, "2") == 0
	    || strcmp (name, mi_simple_values) == 0)
     return PRINT_SIMPLE_VALUES;
   else
     error (_("Unknown value for PRINT_VALUES: must be: \
0 or \"%s\", 1 or \"%s\", 2 or \"%s\""),
	    mi_no_values, mi_all_values, mi_simple_values);
}

/* Print a list of the locals for the current frame.  With argument of
   0, print only the names, with argument of 1 print also the
   values.  */

void
mi_cmd_stack_list_locals (char *command, char **argv, int argc)
{
  struct frame_info *frame;

  if (argc != 1)
    error (_("-stack-list-locals: Usage: PRINT_VALUES"));

   frame = get_selected_frame (NULL);

   list_args_or_locals (locals, parse_print_values (argv[0]), frame);
}

/* Print a list of the arguments for the current frame.  With argument
   of 0, print only the names, with argument of 1 print also the
   values.  */

void
mi_cmd_stack_list_args (char *command, char **argv, int argc)
{
  int frame_low;
  int frame_high;
  int i;
  struct frame_info *fi;
  struct cleanup *cleanup_stack_args;
  enum print_values print_values;
  struct ui_out *uiout = current_uiout;

  if (argc < 1 || argc > 3 || argc == 2)
    error (_("-stack-list-arguments: Usage: "
	     "PRINT_VALUES [FRAME_LOW FRAME_HIGH]"));

  if (argc == 3)
    {
      frame_low = atoi (argv[1]);
      frame_high = atoi (argv[2]);
    }
  else
    {
      /* Called with no arguments, it means we want args for the whole
         backtrace.  */
      frame_low = -1;
      frame_high = -1;
    }

  print_values = parse_print_values (argv[0]);

  /* Let's position fi on the frame at which to start the
     display. Could be the innermost frame if the whole stack needs
     displaying, or if frame_low is 0.  */
  for (i = 0, fi = get_current_frame ();
       fi && i < frame_low;
       i++, fi = get_prev_frame (fi));

  if (fi == NULL)
    error (_("-stack-list-arguments: Not enough frames in stack."));

  cleanup_stack_args
    = make_cleanup_ui_out_list_begin_end (uiout, "stack-args");

  /* Now let's print the frames up to frame_high, or until there are
     frames in the stack.  */
  for (;
       fi && (i <= frame_high || frame_high == -1);
       i++, fi = get_prev_frame (fi))
    {
      struct cleanup *cleanup_frame;

      QUIT;
      cleanup_frame = make_cleanup_ui_out_tuple_begin_end (uiout, "frame");
      ui_out_field_int (uiout, "level", i);
      list_args_or_locals (arguments, print_values, fi);
      do_cleanups (cleanup_frame);
    }

  do_cleanups (cleanup_stack_args);
}

/* Print a list of the local variables (including arguments) for the 
   current frame.  ARGC must be 1 and ARGV[0] specify if only the names,
   or both names and values of the variables must be printed.  See 
   parse_print_value for possible values.  */

void
mi_cmd_stack_list_variables (char *command, char **argv, int argc)
{
  struct frame_info *frame;

  if (argc != 1)
    error (_("Usage: PRINT_VALUES"));

  frame = get_selected_frame (NULL);

  list_args_or_locals (all, parse_print_values (argv[0]), frame);
}

/* Print single local or argument.  ARG must be already read in.  For
   WHAT and VALUES see list_args_or_locals.

   Errors are printed as if they would be the parameter value.  Use
   zeroed ARG iff it should not be printed according to VALUES.  */

static void
list_arg_or_local (const struct frame_arg *arg, enum what_to_list what,
		   enum print_values values)
{
  struct cleanup *old_chain;
  struct cleanup *cleanup_tuple = NULL;
  struct ui_out *uiout = current_uiout;
  struct ui_file *stb;

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  gdb_assert (!arg->val || !arg->error);
  gdb_assert ((values == PRINT_NO_VALUES && arg->val == NULL
	       && arg->error == NULL)
	      || values == PRINT_SIMPLE_VALUES
	      || (values == PRINT_ALL_VALUES
		  && (arg->val != NULL || arg->error != NULL)));
  gdb_assert (arg->entry_kind == print_entry_values_no
	      || (arg->entry_kind == print_entry_values_only
	          && (arg->val || arg->error)));

  if (values != PRINT_NO_VALUES || what == all)
    cleanup_tuple = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);

  fputs_filtered (SYMBOL_PRINT_NAME (arg->sym), stb);
  if (arg->entry_kind == print_entry_values_only)
    fputs_filtered ("@entry", stb);
  ui_out_field_stream (uiout, "name", stb);

  if (what == all && SYMBOL_IS_ARGUMENT (arg->sym))
    ui_out_field_int (uiout, "arg", 1);

  if (values == PRINT_SIMPLE_VALUES)
    {
      check_typedef (arg->sym->type);
      type_print (arg->sym->type, "", stb, -1);
      ui_out_field_stream (uiout, "type", stb);
    }

  if (arg->val || arg->error)
    {
      volatile struct gdb_exception except;

      if (arg->error)
	except.message = arg->error;
      else
	{
	  /* TRY_CATCH has two statements, wrap it in a block.  */

	  TRY_CATCH (except, RETURN_MASK_ERROR)
	    {
	      struct value_print_options opts;

	      get_raw_print_options (&opts);
	      opts.deref_ref = 1;
	      common_val_print (arg->val, stb, 0, &opts,
				language_def (SYMBOL_LANGUAGE (arg->sym)));
	    }
	}
      if (except.message)
	fprintf_filtered (stb, _("<error reading variable: %s>"),
			  except.message);
      ui_out_field_stream (uiout, "value", stb);
    }

  if (values != PRINT_NO_VALUES || what == all)
    do_cleanups (cleanup_tuple);
  do_cleanups (old_chain);
}

/* Print a list of the locals or the arguments for the currently
   selected frame.  If the argument passed is 0, printonly the names
   of the variables, if an argument of 1 is passed, print the values
   as well.  */

static void
list_args_or_locals (enum what_to_list what, enum print_values values,
		     struct frame_info *fi)
{
  struct block *block;
  struct symbol *sym;
  struct block_iterator iter;
  struct cleanup *cleanup_list;
  struct type *type;
  char *name_of_result;
  struct ui_out *uiout = current_uiout;

  block = get_frame_block (fi, 0);

  switch (what)
    {
    case locals:
      name_of_result = "locals";
      break;
    case arguments:
      name_of_result = "args";
      break;
    case all:
      name_of_result = "variables";
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      "unexpected what_to_list: %d", (int) what);
    }

  cleanup_list = make_cleanup_ui_out_list_begin_end (uiout, name_of_result);

  while (block != 0)
    {
      ALL_BLOCK_SYMBOLS (block, iter, sym)
	{
          int print_me = 0;

	  switch (SYMBOL_CLASS (sym))
	    {
	    default:
	    case LOC_UNDEF:	/* catches errors        */
	    case LOC_CONST:	/* constant              */
	    case LOC_TYPEDEF:	/* local typedef         */
	    case LOC_LABEL:	/* local label           */
	    case LOC_BLOCK:	/* local function        */
	    case LOC_CONST_BYTES:	/* loc. byte seq.        */
	    case LOC_UNRESOLVED:	/* unresolved static     */
	    case LOC_OPTIMIZED_OUT:	/* optimized out         */
	      print_me = 0;
	      break;

	    case LOC_ARG:	/* argument              */
	    case LOC_REF_ARG:	/* reference arg         */
	    case LOC_REGPARM_ADDR:	/* indirect register arg */
	    case LOC_LOCAL:	/* stack local           */
	    case LOC_STATIC:	/* static                */
	    case LOC_REGISTER:	/* register              */
	    case LOC_COMPUTED:	/* computed location     */
	      if (what == all)
		print_me = 1;
	      else if (what == locals)
		print_me = !SYMBOL_IS_ARGUMENT (sym);
	      else
		print_me = SYMBOL_IS_ARGUMENT (sym);
	      break;
	    }
	  if (print_me)
	    {
	      struct symbol *sym2;
	      struct frame_arg arg, entryarg;

	      if (SYMBOL_IS_ARGUMENT (sym))
		sym2 = lookup_symbol (SYMBOL_LINKAGE_NAME (sym),
				      block, VAR_DOMAIN,
				      NULL);
	      else
		sym2 = sym;
	      gdb_assert (sym2 != NULL);

	      memset (&arg, 0, sizeof (arg));
	      arg.sym = sym2;
	      arg.entry_kind = print_entry_values_no;
	      memset (&entryarg, 0, sizeof (entryarg));
	      entryarg.sym = sym2;
	      entryarg.entry_kind = print_entry_values_no;

	      switch (values)
		{
		case PRINT_SIMPLE_VALUES:
		  type = check_typedef (sym2->type);
		  if (TYPE_CODE (type) != TYPE_CODE_ARRAY
		      && TYPE_CODE (type) != TYPE_CODE_STRUCT
		      && TYPE_CODE (type) != TYPE_CODE_UNION)
		    {
		case PRINT_ALL_VALUES:
		  if (SYMBOL_IS_ARGUMENT (sym))
		    read_frame_arg (sym2, fi, &arg, &entryarg);
		  else
		    read_frame_local (sym2, fi, &arg);
		    }
		  break;
		}

	      if (arg.entry_kind != print_entry_values_only)
		list_arg_or_local (&arg, what, values);
	      if (entryarg.entry_kind != print_entry_values_no)
		list_arg_or_local (&entryarg, what, values);
	      xfree (arg.error);
	      xfree (entryarg.error);
	    }
	}

      if (BLOCK_FUNCTION (block))
	break;
      else
	block = BLOCK_SUPERBLOCK (block);
    }
  do_cleanups (cleanup_list);
}

void
mi_cmd_stack_select_frame (char *command, char **argv, int argc)
{
  if (argc == 0 || argc > 1)
    error (_("-stack-select-frame: Usage: FRAME_SPEC"));

  select_frame_command (argv[0], 1 /* not used */ );
}

void
mi_cmd_stack_info_frame (char *command, char **argv, int argc)
{
  if (argc > 0)
    error (_("-stack-info-frame: No arguments allowed"));

  print_frame_info (get_selected_frame (NULL), 1, LOC_AND_ADDRESS, 0);
}
