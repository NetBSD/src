/* MI Command Set - stack commands.
   Copyright (C) 2000-2017 Free Software Foundation, Inc.
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
#include "language.h"
#include "valprint.h"
#include "utils.h"
#include "mi-getopt.h"
#include "extension.h"
#include <ctype.h>
#include "mi-parse.h"

enum what_to_list { locals, arguments, all };

static void list_args_or_locals (enum what_to_list what,
				 enum print_values values,
				 struct frame_info *fi,
				 int skip_unavailable);

/* True if we want to allow Python-based frame filters.  */
static int frame_filters = 0;

void
mi_cmd_enable_frame_filters (const char *command, char **argv, int argc)
{
  if (argc != 0)
    error (_("-enable-frame-filters: no arguments allowed"));
  frame_filters = 1;
}

/* Like apply_ext_lang_frame_filter, but take a print_values */

static enum ext_lang_bt_status
mi_apply_ext_lang_frame_filter (struct frame_info *frame, int flags,
				enum print_values print_values,
				struct ui_out *out,
				int frame_low, int frame_high)
{
  /* ext_lang_frame_args's MI options are compatible with MI print
     values.  */
  return apply_ext_lang_frame_filter (frame, flags,
				      (enum ext_lang_frame_args) print_values,
				      out,
				      frame_low, frame_high);
}

/* Print a list of the stack frames.  Args can be none, in which case
   we want to print the whole backtrace, or a pair of numbers
   specifying the frame numbers at which to start and stop the
   display.  If the two numbers are equal, a single frame will be
   displayed.  */

void
mi_cmd_stack_list_frames (const char *command, char **argv, int argc)
{
  int frame_low;
  int frame_high;
  int i;
  struct cleanup *cleanup_stack;
  struct frame_info *fi;
  enum ext_lang_bt_status result = EXT_LANG_BT_ERROR;
  int raw_arg = 0;
  int oind = 0;
  enum opt
    {
      NO_FRAME_FILTERS
    };
  static const struct mi_opt opts[] =
    {
      {"-no-frame-filters", NO_FRAME_FILTERS, 0},
      { 0, 0, 0 }
    };

  /* Parse arguments.  In this instance we are just looking for
     --no-frame-filters.  */
  while (1)
    {
      char *oarg;
      int opt = mi_getopt ("-stack-list-frames", argc, argv,
			   opts, &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case NO_FRAME_FILTERS:
	  raw_arg = oind;
	  break;
	}
    }

  /* After the last option is parsed, there should either be low -
     high range, or no further arguments.  */
  if ((argc - oind != 0) && (argc - oind != 2))
    error (_("-stack-list-frames: Usage: [--no-frame-filters] [FRAME_LOW FRAME_HIGH]"));

  /* If there is a range, set it.  */
  if (argc - oind == 2)
    {
      frame_low = atoi (argv[0 + oind]);
      frame_high = atoi (argv[1 + oind]);
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

  if (! raw_arg && frame_filters)
    {
      int flags = PRINT_LEVEL | PRINT_FRAME_INFO;
      int py_frame_low = frame_low;

      /* We cannot pass -1 to frame_low, as that would signify a
      relative backtrace from the tail of the stack.  So, in the case
      of frame_low == -1, assign and increment it.  */
      if (py_frame_low == -1)
	py_frame_low++;

      result = apply_ext_lang_frame_filter (get_current_frame (), flags,
					    NO_VALUES,  current_uiout,
					    py_frame_low, frame_high);
    }

  /* Run the inbuilt backtrace if there are no filters registered, or
     if "--no-frame-filters" has been specified from the command.  */
  if (! frame_filters || raw_arg  || result == EXT_LANG_BT_NO_FILTERS)
    {
      /* Now let's print the frames up to frame_high, or until there are
	 frames in the stack.  */
      for (;
	   fi && (i <= frame_high || frame_high == -1);
	   i++, fi = get_prev_frame (fi))
	{
	  QUIT;
	  /* Print the location and the address always, even for level 0.
	     If args is 0, don't print the arguments.  */
	  print_frame_info (fi, 1, LOC_AND_ADDRESS, 0 /* args */, 0);
	}
    }

  do_cleanups (cleanup_stack);
}

void
mi_cmd_stack_info_depth (const char *command, char **argv, int argc)
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

  current_uiout->field_int ("depth", i);
}

/* Print a list of the locals for the current frame.  With argument of
   0, print only the names, with argument of 1 print also the
   values.  */

void
mi_cmd_stack_list_locals (const char *command, char **argv, int argc)
{
  struct frame_info *frame;
  int raw_arg = 0;
  enum ext_lang_bt_status result = EXT_LANG_BT_ERROR;
  enum print_values print_value;
  int oind = 0;
  int skip_unavailable = 0;

  if (argc > 1)
    {
      enum opt
      {
	NO_FRAME_FILTERS,
	SKIP_UNAVAILABLE,
      };
      static const struct mi_opt opts[] =
	{
	  {"-no-frame-filters", NO_FRAME_FILTERS, 0},
	  {"-skip-unavailable", SKIP_UNAVAILABLE, 0},
	  { 0, 0, 0 }
	};

      while (1)
	{
	  char *oarg;
	  /* Don't parse 'print-values' as an option.  */
	  int opt = mi_getopt ("-stack-list-locals", argc - 1, argv,
			       opts, &oind, &oarg);

	  if (opt < 0)
	    break;
	  switch ((enum opt) opt)
	    {
	    case NO_FRAME_FILTERS:
	      raw_arg = oind;
	    case SKIP_UNAVAILABLE:
	      skip_unavailable = 1;
	      break;
	    }
	}
    }

  /* After the last option is parsed, there should be only
     'print-values'.  */
  if (argc - oind != 1)
    error (_("-stack-list-locals: Usage: [--no-frame-filters] "
	     "[--skip-unavailable] PRINT_VALUES"));

  frame = get_selected_frame (NULL);
  print_value = mi_parse_print_values (argv[oind]);

   if (! raw_arg && frame_filters)
     {
       int flags = PRINT_LEVEL | PRINT_LOCALS;

       result = mi_apply_ext_lang_frame_filter (frame, flags, print_value,
						current_uiout, 0, 0);
     }

   /* Run the inbuilt backtrace if there are no filters registered, or
      if "--no-frame-filters" has been specified from the command.  */
   if (! frame_filters || raw_arg  || result == EXT_LANG_BT_NO_FILTERS)
     {
       list_args_or_locals (locals, print_value, frame,
			    skip_unavailable);
     }
}

/* Print a list of the arguments for the current frame.  With argument
   of 0, print only the names, with argument of 1 print also the
   values.  */

void
mi_cmd_stack_list_args (const char *command, char **argv, int argc)
{
  int frame_low;
  int frame_high;
  int i;
  struct frame_info *fi;
  struct cleanup *cleanup_stack_args;
  enum print_values print_values;
  struct ui_out *uiout = current_uiout;
  int raw_arg = 0;
  int oind = 0;
  int skip_unavailable = 0;
  enum ext_lang_bt_status result = EXT_LANG_BT_ERROR;
  enum opt
  {
    NO_FRAME_FILTERS,
    SKIP_UNAVAILABLE,
  };
  static const struct mi_opt opts[] =
    {
      {"-no-frame-filters", NO_FRAME_FILTERS, 0},
      {"-skip-unavailable", SKIP_UNAVAILABLE, 0},
      { 0, 0, 0 }
    };

  while (1)
    {
      char *oarg;
      int opt = mi_getopt_allow_unknown ("-stack-list-args", argc, argv,
					 opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case NO_FRAME_FILTERS:
	  raw_arg = oind;
	  break;
	case SKIP_UNAVAILABLE:
	  skip_unavailable = 1;
	  break;
	}
    }

  if (argc - oind != 1 && argc - oind != 3)
    error (_("-stack-list-arguments: Usage: "	\
	     "[--no-frame-filters] [--skip-unavailable] "
	     "PRINT_VALUES [FRAME_LOW FRAME_HIGH]"));

  if (argc - oind == 3)
    {
      frame_low = atoi (argv[1 + oind]);
      frame_high = atoi (argv[2 + oind]);
    }
  else
    {
      /* Called with no arguments, it means we want args for the whole
         backtrace.  */
      frame_low = -1;
      frame_high = -1;
    }

  print_values = mi_parse_print_values (argv[oind]);

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

  if (! raw_arg && frame_filters)
    {
      int flags = PRINT_LEVEL | PRINT_ARGS;
      int py_frame_low = frame_low;

      /* We cannot pass -1 to frame_low, as that would signify a
      relative backtrace from the tail of the stack.  So, in the case
      of frame_low == -1, assign and increment it.  */
      if (py_frame_low == -1)
	py_frame_low++;

      result = mi_apply_ext_lang_frame_filter (get_current_frame (), flags,
					       print_values, current_uiout,
					       py_frame_low, frame_high);
    }

     /* Run the inbuilt backtrace if there are no filters registered, or
      if "--no-frame-filters" has been specified from the command.  */
   if (! frame_filters || raw_arg  || result == EXT_LANG_BT_NO_FILTERS)
     {
      /* Now let's print the frames up to frame_high, or until there are
	 frames in the stack.  */
      for (;
	   fi && (i <= frame_high || frame_high == -1);
	   i++, fi = get_prev_frame (fi))
	{
	  struct cleanup *cleanup_frame;

	  QUIT;
	  cleanup_frame = make_cleanup_ui_out_tuple_begin_end (uiout, "frame");
	  uiout->field_int ("level", i);
	  list_args_or_locals (arguments, print_values, fi, skip_unavailable);
	  do_cleanups (cleanup_frame);
	}
    }
  do_cleanups (cleanup_stack_args);
}

/* Print a list of the local variables (including arguments) for the 
   current frame.  ARGC must be 1 and ARGV[0] specify if only the names,
   or both names and values of the variables must be printed.  See 
   parse_print_value for possible values.  */

void
mi_cmd_stack_list_variables (const char *command, char **argv, int argc)
{
  struct frame_info *frame;
  int raw_arg = 0;
  enum ext_lang_bt_status result = EXT_LANG_BT_ERROR;
  enum print_values print_value;
  int oind = 0;
  int skip_unavailable = 0;

  if (argc > 1)
    {
      enum opt
      {
	NO_FRAME_FILTERS,
	SKIP_UNAVAILABLE,
      };
      static const struct mi_opt opts[] =
	{
	  {"-no-frame-filters", NO_FRAME_FILTERS, 0},
	  {"-skip-unavailable", SKIP_UNAVAILABLE, 0},
	  { 0, 0, 0 }
	};

      while (1)
	{
	  char *oarg;
	  /* Don't parse 'print-values' as an option.  */
	  int opt = mi_getopt ("-stack-list-variables", argc - 1,
			       argv, opts, &oind, &oarg);
	  if (opt < 0)
	    break;
	  switch ((enum opt) opt)
	    {
	    case NO_FRAME_FILTERS:
	      raw_arg = oind;
	      break;
	    case SKIP_UNAVAILABLE:
	      skip_unavailable = 1;
	      break;
	    }
	}
    }

  /* After the last option is parsed, there should be only
     'print-values'.  */
  if (argc - oind != 1)
    error (_("-stack-list-variables: Usage: [--no-frame-filters] " \
	     "[--skip-unavailable] PRINT_VALUES"));

   frame = get_selected_frame (NULL);
   print_value = mi_parse_print_values (argv[oind]);

   if (! raw_arg && frame_filters)
     {
       int flags = PRINT_LEVEL | PRINT_ARGS | PRINT_LOCALS;

       result = mi_apply_ext_lang_frame_filter (frame, flags,
						print_value,
						current_uiout, 0, 0);
     }

   /* Run the inbuilt backtrace if there are no filters registered, or
      if "--no-frame-filters" has been specified from the command.  */
   if (! frame_filters || raw_arg  || result == EXT_LANG_BT_NO_FILTERS)
     {
       list_args_or_locals (all, print_value, frame,
			    skip_unavailable);
     }
}

/* Print single local or argument.  ARG must be already read in.  For
   WHAT and VALUES see list_args_or_locals.

   Errors are printed as if they would be the parameter value.  Use
   zeroed ARG iff it should not be printed according to VALUES.  If
   SKIP_UNAVAILABLE is true, only print ARG if it is available.  */

static void
list_arg_or_local (const struct frame_arg *arg, enum what_to_list what,
		   enum print_values values, int skip_unavailable)
{
  struct cleanup *old_chain;
  struct ui_out *uiout = current_uiout;

  gdb_assert (!arg->val || !arg->error);
  gdb_assert ((values == PRINT_NO_VALUES && arg->val == NULL
	       && arg->error == NULL)
	      || values == PRINT_SIMPLE_VALUES
	      || (values == PRINT_ALL_VALUES
		  && (arg->val != NULL || arg->error != NULL)));
  gdb_assert (arg->entry_kind == print_entry_values_no
	      || (arg->entry_kind == print_entry_values_only
	          && (arg->val || arg->error)));

  if (skip_unavailable && arg->val != NULL
      && (value_entirely_unavailable (arg->val)
	  /* A scalar object that does not have all bits available is
	     also considered unavailable, because all bits contribute
	     to its representation.  */
	  || (val_print_scalar_type_p (value_type (arg->val))
	      && !value_bytes_available (arg->val,
					 value_embedded_offset (arg->val),
					 TYPE_LENGTH (value_type (arg->val))))))
    return;

  old_chain = make_cleanup (null_cleanup, NULL);

  if (values != PRINT_NO_VALUES || what == all)
    make_cleanup_ui_out_tuple_begin_end (uiout, NULL);

  string_file stb;

  stb.puts (SYMBOL_PRINT_NAME (arg->sym));
  if (arg->entry_kind == print_entry_values_only)
    stb.puts ("@entry");
  uiout->field_stream ("name", stb);

  if (what == all && SYMBOL_IS_ARGUMENT (arg->sym))
    uiout->field_int ("arg", 1);

  if (values == PRINT_SIMPLE_VALUES)
    {
      check_typedef (arg->sym->type);
      type_print (arg->sym->type, "", &stb, -1);
      uiout->field_stream ("type", stb);
    }

  if (arg->val || arg->error)
    {
      const char *error_message = NULL;

      if (arg->error)
	error_message = arg->error;
      else
	{
	  TRY
	    {
	      struct value_print_options opts;

	      get_no_prettyformat_print_options (&opts);
	      opts.deref_ref = 1;
	      common_val_print (arg->val, &stb, 0, &opts,
				language_def (SYMBOL_LANGUAGE (arg->sym)));
	    }
	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      error_message = except.message;
	    }
	  END_CATCH
	}
      if (error_message != NULL)
	stb.printf (_("<error reading variable: %s>"), error_message);
      uiout->field_stream ("value", stb);
    }

  do_cleanups (old_chain);
}

/* Print a list of the objects for the frame FI in a certain form,
   which is determined by VALUES.  The objects can be locals,
   arguments or both, which is determined by WHAT.  If SKIP_UNAVAILABLE
   is true, only print the arguments or local variables whose values
   are available.  */

static void
list_args_or_locals (enum what_to_list what, enum print_values values,
		     struct frame_info *fi, int skip_unavailable)
{
  const struct block *block;
  struct symbol *sym;
  struct block_iterator iter;
  struct cleanup *cleanup_list;
  struct type *type;
  const char *name_of_result;
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
				      NULL).symbol;
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
		list_arg_or_local (&arg, what, values, skip_unavailable);
	      if (entryarg.entry_kind != print_entry_values_no)
		list_arg_or_local (&entryarg, what, values, skip_unavailable);
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
mi_cmd_stack_select_frame (const char *command, char **argv, int argc)
{
  if (argc == 0 || argc > 1)
    error (_("-stack-select-frame: Usage: FRAME_SPEC"));

  select_frame_command (argv[0], 1 /* not used */ );
}

void
mi_cmd_stack_info_frame (const char *command, char **argv, int argc)
{
  if (argc > 0)
    error (_("-stack-info-frame: No arguments allowed"));

  print_frame_info (get_selected_frame (NULL), 1, LOC_AND_ADDRESS, 0, 1);
}
