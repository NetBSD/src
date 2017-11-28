/* Everything about signal catchpoints, for GDB.

   Copyright (C) 2011-2016 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include <ctype.h>
#include "breakpoint.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "infrun.h"
#include "annotate.h"
#include "valprint.h"
#include "cli/cli-utils.h"
#include "completer.h"
#include "gdb_obstack.h"

#define INTERNAL_SIGNAL(x) ((x) == GDB_SIGNAL_TRAP || (x) == GDB_SIGNAL_INT)

typedef enum gdb_signal gdb_signal_type;

DEF_VEC_I (gdb_signal_type);

/* An instance of this type is used to represent a signal catchpoint.
   It includes a "struct breakpoint" as a kind of base class; users
   downcast to "struct breakpoint *" when needed.  A breakpoint is
   really of this type iff its ops pointer points to
   SIGNAL_CATCHPOINT_OPS.  */

struct signal_catchpoint
{
  /* The base class.  */

  struct breakpoint base;

  /* Signal numbers used for the 'catch signal' feature.  If no signal
     has been specified for filtering, its value is NULL.  Otherwise,
     it holds a list of all signals to be caught.  */

  VEC (gdb_signal_type) *signals_to_be_caught;

  /* If SIGNALS_TO_BE_CAUGHT is NULL, then all "ordinary" signals are
     caught.  If CATCH_ALL is non-zero, then internal signals are
     caught as well.  If SIGNALS_TO_BE_CAUGHT is non-NULL, then this
     field is ignored.  */

  int catch_all;
};

/* The breakpoint_ops structure to be used in signal catchpoints.  */

static struct breakpoint_ops signal_catchpoint_ops;

/* Count of each signal.  */

static unsigned int *signal_catch_counts;



/* A convenience wrapper for gdb_signal_to_name that returns the
   integer value if the name is not known.  */

static const char *
signal_to_name_or_int (enum gdb_signal sig)
{
  const char *result = gdb_signal_to_name (sig);

  if (strcmp (result, "?") == 0)
    result = plongest (sig);

  return result;
}



/* Implement the "dtor" breakpoint_ops method for signal
   catchpoints.  */

static void
signal_catchpoint_dtor (struct breakpoint *b)
{
  struct signal_catchpoint *c = (struct signal_catchpoint *) b;

  VEC_free (gdb_signal_type, c->signals_to_be_caught);

  base_breakpoint_ops.dtor (b);
}

/* Implement the "insert_location" breakpoint_ops method for signal
   catchpoints.  */

static int
signal_catchpoint_insert_location (struct bp_location *bl)
{
  struct signal_catchpoint *c = (struct signal_catchpoint *) bl->owner;
  int i;

  if (c->signals_to_be_caught != NULL)
    {
      gdb_signal_type iter;

      for (i = 0;
	   VEC_iterate (gdb_signal_type, c->signals_to_be_caught, i, iter);
	   i++)
	++signal_catch_counts[iter];
    }
  else
    {
      for (i = 0; i < GDB_SIGNAL_LAST; ++i)
	{
	  if (c->catch_all || !INTERNAL_SIGNAL (i))
	    ++signal_catch_counts[i];
	}
    }

  signal_catch_update (signal_catch_counts);

  return 0;
}

/* Implement the "remove_location" breakpoint_ops method for signal
   catchpoints.  */

static int
signal_catchpoint_remove_location (struct bp_location *bl,
				   enum remove_bp_reason reason)
{
  struct signal_catchpoint *c = (struct signal_catchpoint *) bl->owner;
  int i;

  if (c->signals_to_be_caught != NULL)
    {
      gdb_signal_type iter;

      for (i = 0;
	   VEC_iterate (gdb_signal_type, c->signals_to_be_caught, i, iter);
	   i++)
	{
	  gdb_assert (signal_catch_counts[iter] > 0);
	  --signal_catch_counts[iter];
	}
    }
  else
    {
      for (i = 0; i < GDB_SIGNAL_LAST; ++i)
	{
	  if (c->catch_all || !INTERNAL_SIGNAL (i))
	    {
	      gdb_assert (signal_catch_counts[i] > 0);
	      --signal_catch_counts[i];
	    }
	}
    }

  signal_catch_update (signal_catch_counts);

  return 0;
}

/* Implement the "breakpoint_hit" breakpoint_ops method for signal
   catchpoints.  */

static int
signal_catchpoint_breakpoint_hit (const struct bp_location *bl,
				  struct address_space *aspace,
				  CORE_ADDR bp_addr,
				  const struct target_waitstatus *ws)
{
  const struct signal_catchpoint *c
    = (const struct signal_catchpoint *) bl->owner;
  gdb_signal_type signal_number;

  if (ws->kind != TARGET_WAITKIND_STOPPED)
    return 0;

  signal_number = ws->value.sig;

  /* If we are catching specific signals in this breakpoint, then we
     must guarantee that the called signal is the same signal we are
     catching.  */
  if (c->signals_to_be_caught)
    {
      int i;
      gdb_signal_type iter;

      for (i = 0;
           VEC_iterate (gdb_signal_type, c->signals_to_be_caught, i, iter);
           i++)
	if (signal_number == iter)
	  return 1;
      /* Not the same.  */
      gdb_assert (!iter);
      return 0;
    }
  else
    return c->catch_all || !INTERNAL_SIGNAL (signal_number);
}

/* Implement the "print_it" breakpoint_ops method for signal
   catchpoints.  */

static enum print_stop_action
signal_catchpoint_print_it (bpstat bs)
{
  struct breakpoint *b = bs->breakpoint_at;
  ptid_t ptid;
  struct target_waitstatus last;
  const char *signal_name;
  struct ui_out *uiout = current_uiout;

  get_last_target_status (&ptid, &last);

  signal_name = signal_to_name_or_int (last.value.sig);

  annotate_catchpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);

  printf_filtered (_("Catchpoint %d (signal %s), "), b->number, signal_name);

  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" breakpoint_ops method for signal
   catchpoints.  */

static void
signal_catchpoint_print_one (struct breakpoint *b,
			     struct bp_location **last_loc)
{
  struct signal_catchpoint *c = (struct signal_catchpoint *) b;
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  get_user_print_options (&opts);

  /* Field 4, the address, is omitted (which makes the columns
     not line up too nicely with the headers, but the effect
     is relatively readable).  */
  if (opts.addressprint)
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);

  if (c->signals_to_be_caught
      && VEC_length (gdb_signal_type, c->signals_to_be_caught) > 1)
    ui_out_text (uiout, "signals \"");
  else
    ui_out_text (uiout, "signal \"");

  if (c->signals_to_be_caught)
    {
      int i;
      gdb_signal_type iter;
      struct obstack text;
      struct cleanup *cleanup;

      obstack_init (&text);
      cleanup = make_cleanup_obstack_free (&text);

      for (i = 0;
           VEC_iterate (gdb_signal_type, c->signals_to_be_caught, i, iter);
           i++)
        {
	  const char *name = signal_to_name_or_int (iter);

	  if (i > 0)
	    obstack_grow (&text, " ", 1);
	  obstack_grow (&text, name, strlen (name));
        }
      obstack_grow (&text, "", 1);
      ui_out_field_string (uiout, "what", (const char *) obstack_base (&text));
      do_cleanups (cleanup);
    }
  else
    ui_out_field_string (uiout, "what",
			 c->catch_all ? "<any signal>" : "<standard signals>");
  ui_out_text (uiout, "\" ");

  if (ui_out_is_mi_like_p (uiout))
    ui_out_field_string (uiout, "catch-type", "signal");
}

/* Implement the "print_mention" breakpoint_ops method for signal
   catchpoints.  */

static void
signal_catchpoint_print_mention (struct breakpoint *b)
{
  struct signal_catchpoint *c = (struct signal_catchpoint *) b;

  if (c->signals_to_be_caught)
    {
      int i;
      gdb_signal_type iter;

      if (VEC_length (gdb_signal_type, c->signals_to_be_caught) > 1)
        printf_filtered (_("Catchpoint %d (signals"), b->number);
      else
        printf_filtered (_("Catchpoint %d (signal"), b->number);

      for (i = 0;
           VEC_iterate (gdb_signal_type, c->signals_to_be_caught, i, iter);
           i++)
        {
	  const char *name = signal_to_name_or_int (iter);

	  printf_filtered (" %s", name);
        }
      printf_filtered (")");
    }
  else if (c->catch_all)
    printf_filtered (_("Catchpoint %d (any signal)"), b->number);
  else
    printf_filtered (_("Catchpoint %d (standard signals)"), b->number);
}

/* Implement the "print_recreate" breakpoint_ops method for signal
   catchpoints.  */

static void
signal_catchpoint_print_recreate (struct breakpoint *b, struct ui_file *fp)
{
  struct signal_catchpoint *c = (struct signal_catchpoint *) b;

  fprintf_unfiltered (fp, "catch signal");

  if (c->signals_to_be_caught)
    {
      int i;
      gdb_signal_type iter;

      for (i = 0;
           VEC_iterate (gdb_signal_type, c->signals_to_be_caught, i, iter);
           i++)
	fprintf_unfiltered (fp, " %s", signal_to_name_or_int (iter));
    }
  else if (c->catch_all)
    fprintf_unfiltered (fp, " all");
  fputc_unfiltered ('\n', fp);
}

/* Implement the "explains_signal" breakpoint_ops method for signal
   catchpoints.  */

static int
signal_catchpoint_explains_signal (struct breakpoint *b, enum gdb_signal sig)
{
  return 1;
}

/* Create a new signal catchpoint.  TEMPFLAG is true if this should be
   a temporary catchpoint.  FILTER is the list of signals to catch; it
   can be NULL, meaning all signals.  CATCH_ALL is a flag indicating
   whether signals used internally by gdb should be caught; it is only
   valid if FILTER is NULL.  If FILTER is NULL and CATCH_ALL is zero,
   then internal signals like SIGTRAP are not caught.  */

static void
create_signal_catchpoint (int tempflag, VEC (gdb_signal_type) *filter,
			  int catch_all)
{
  struct signal_catchpoint *c;
  struct gdbarch *gdbarch = get_current_arch ();

  c = XNEW (struct signal_catchpoint);
  init_catchpoint (&c->base, gdbarch, tempflag, NULL, &signal_catchpoint_ops);
  c->signals_to_be_caught = filter;
  c->catch_all = catch_all;

  install_breakpoint (0, &c->base, 1);
}


/* Splits the argument using space as delimiter.  Returns an xmalloc'd
   filter list, or NULL if no filtering is required.  */

static VEC (gdb_signal_type) *
catch_signal_split_args (char *arg, int *catch_all)
{
  VEC (gdb_signal_type) *result = NULL;
  struct cleanup *cleanup = make_cleanup (VEC_cleanup (gdb_signal_type),
					  &result);
  int first = 1;

  while (*arg != '\0')
    {
      int num;
      gdb_signal_type signal_number;
      char *one_arg, *endptr;
      struct cleanup *inner_cleanup;

      one_arg = extract_arg (&arg);
      if (one_arg == NULL)
	break;
      inner_cleanup = make_cleanup (xfree, one_arg);

      /* Check for the special flag "all".  */
      if (strcmp (one_arg, "all") == 0)
	{
	  arg = skip_spaces (arg);
	  if (*arg != '\0' || !first)
	    error (_("'all' cannot be caught with other signals"));
	  *catch_all = 1;
	  gdb_assert (result == NULL);
	  do_cleanups (inner_cleanup);
	  discard_cleanups (cleanup);
	  return NULL;
	}

      first = 0;

      /* Check if the user provided a signal name or a number.  */
      num = (int) strtol (one_arg, &endptr, 0);
      if (*endptr == '\0')
	signal_number = gdb_signal_from_command (num);
      else
	{
	  signal_number = gdb_signal_from_name (one_arg);
	  if (signal_number == GDB_SIGNAL_UNKNOWN)
	    error (_("Unknown signal name '%s'."), one_arg);
	}

      VEC_safe_push (gdb_signal_type, result, signal_number);
      do_cleanups (inner_cleanup);
    }

  discard_cleanups (cleanup);
  return result;
}

/* Implement the "catch signal" command.  */

static void
catch_signal_command (char *arg, int from_tty,
		      struct cmd_list_element *command)
{
  int tempflag, catch_all = 0;
  VEC (gdb_signal_type) *filter;

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  arg = skip_spaces (arg);

  /* The allowed syntax is:
     catch signal
     catch signal <name | number> [<name | number> ... <name | number>]

     Let's check if there's a signal name.  */

  if (arg != NULL)
    filter = catch_signal_split_args (arg, &catch_all);
  else
    filter = NULL;

  create_signal_catchpoint (tempflag, filter, catch_all);
}

static void
initialize_signal_catchpoint_ops (void)
{
  struct breakpoint_ops *ops;

  initialize_breakpoint_ops ();

  ops = &signal_catchpoint_ops;
  *ops = base_breakpoint_ops;
  ops->dtor = signal_catchpoint_dtor;
  ops->insert_location = signal_catchpoint_insert_location;
  ops->remove_location = signal_catchpoint_remove_location;
  ops->breakpoint_hit = signal_catchpoint_breakpoint_hit;
  ops->print_it = signal_catchpoint_print_it;
  ops->print_one = signal_catchpoint_print_one;
  ops->print_mention = signal_catchpoint_print_mention;
  ops->print_recreate = signal_catchpoint_print_recreate;
  ops->explains_signal = signal_catchpoint_explains_signal;
}

initialize_file_ftype _initialize_break_catch_sig;

void
_initialize_break_catch_sig (void)
{
  initialize_signal_catchpoint_ops ();

  signal_catch_counts = XCNEWVEC (unsigned int, GDB_SIGNAL_LAST);

  add_catch_command ("signal", _("\
Catch signals by their names and/or numbers.\n\
Usage: catch signal [[NAME|NUMBER] [NAME|NUMBER]...|all]\n\
Arguments say which signals to catch.  If no arguments\n\
are given, every \"normal\" signal will be caught.\n\
The argument \"all\" means to also catch signals used by GDB.\n\
Arguments, if given, should be one or more signal names\n\
(if your system supports that), or signal numbers."),
		     catch_signal_command,
		     signal_completer,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
}
