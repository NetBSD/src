/* Everything about catch/throw catchpoints, for GDB.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
#include "annotate.h"
#include "valprint.h"
#include "cli/cli-utils.h"
#include "completer.h"
#include "gdb_obstack.h"
#include "mi/mi-common.h"
#include "exceptions.h"
#include "linespec.h"
#include "probe.h"
#include "objfiles.h"
#include "cp-abi.h"
#include "gdb_regex.h"
#include "cp-support.h"

/* Enums for exception-handling support.  */
enum exception_event_kind
{
  EX_EVENT_THROW,
  EX_EVENT_RETHROW,
  EX_EVENT_CATCH
};

/* Each spot where we may place an exception-related catchpoint has
   two names: the SDT probe point and the function name.  This
   structure holds both.  */

struct exception_names
{
  /* The name of the probe point to try, in the form accepted by
     'parse_probes'.  */

  const char *probe;

  /* The name of the corresponding function.  */

  const char *function;
};

/* Names of the probe points and functions on which to break.  This is
   indexed by exception_event_kind.  */
static const struct exception_names exception_functions[] =
{
  { "-probe-stap libstdcxx:throw", "__cxa_throw" },
  { "-probe-stap libstdcxx:rethrow", "__cxa_rethrow" },
  { "-probe-stap libstdcxx:catch", "__cxa_begin_catch" }
};

static struct breakpoint_ops gnu_v3_exception_catchpoint_ops;

/* The type of an exception catchpoint.  */

struct exception_catchpoint
{
  /* The base class.  */

  struct breakpoint base;

  /* The kind of exception catchpoint.  */

  enum exception_event_kind kind;

  /* If non-NULL, an xmalloc'd string holding the source form of the
     regular expression to match against.  */

  char *exception_rx;

  /* If non-NULL, an xmalloc'd, compiled regular expression which is
     used to determine which exceptions to stop on.  */

  regex_t *pattern;
};



/* A helper function that fetches exception probe arguments.  This
   fills in *ARG0 (if non-NULL) and *ARG1 (which must be non-NULL).
   It will throw an exception on any kind of failure.  */

static void
fetch_probe_arguments (struct value **arg0, struct value **arg1)
{
  struct frame_info *frame = get_selected_frame (_("No frame selected"));
  CORE_ADDR pc = get_frame_pc (frame);
  struct probe *pc_probe;
  const struct sym_probe_fns *pc_probe_fns;
  unsigned n_args;

  pc_probe = find_probe_by_pc (pc);
  if (pc_probe == NULL
      || strcmp (pc_probe->provider, "libstdcxx") != 0
      || (strcmp (pc_probe->name, "catch") != 0
	  && strcmp (pc_probe->name, "throw") != 0
	  && strcmp (pc_probe->name, "rethrow") != 0))
    error (_("not stopped at a C++ exception catchpoint"));

  n_args = get_probe_argument_count (pc_probe, frame);
  if (n_args < 2)
    error (_("C++ exception catchpoint has too few arguments"));

  if (arg0 != NULL)
    *arg0 = evaluate_probe_argument (pc_probe, 0, frame);
  *arg1 = evaluate_probe_argument (pc_probe, 1, frame);

  if ((arg0 != NULL && *arg0 == NULL) || *arg1 == NULL)
    error (_("error computing probe argument at c++ exception catchpoint"));
}



/* A helper function that returns a value indicating the kind of the
   exception catchpoint B.  */

static enum exception_event_kind
classify_exception_breakpoint (struct breakpoint *b)
{
  struct exception_catchpoint *cp = (struct exception_catchpoint *) b;

  return cp->kind;
}

/* Implement the 'dtor' method.  */

static void
dtor_exception_catchpoint (struct breakpoint *self)
{
  struct exception_catchpoint *cp = (struct exception_catchpoint *) self;

  xfree (cp->exception_rx);
  if (cp->pattern != NULL)
    regfree (cp->pattern);
  bkpt_breakpoint_ops.dtor (self);
}

/* Implement the 'check_status' method.  */

static void
check_status_exception_catchpoint (struct bpstats *bs)
{
  struct exception_catchpoint *self
    = (struct exception_catchpoint *) bs->breakpoint_at;
  char *typename = NULL;
  volatile struct gdb_exception e;

  bkpt_breakpoint_ops.check_status (bs);
  if (bs->stop == 0)
    return;

  if (self->pattern == NULL)
    return;

  TRY_CATCH (e, RETURN_MASK_ERROR)
    {
      struct value *typeinfo_arg;
      char *canon;

      fetch_probe_arguments (NULL, &typeinfo_arg);
      typename = cplus_typename_from_type_info (typeinfo_arg);

      canon = cp_canonicalize_string (typename);
      if (canon != NULL)
	{
	  xfree (typename);
	  typename = canon;
	}
    }

  if (e.reason < 0)
    exception_print (gdb_stderr, e);
  else if (regexec (self->pattern, typename, 0, NULL, 0) != 0)
    bs->stop = 0;

  xfree (typename);
}

/* Implement the 're_set' method.  */

static void
re_set_exception_catchpoint (struct breakpoint *self)
{
  struct symtabs_and_lines sals = {0};
  struct symtabs_and_lines sals_end = {0};
  volatile struct gdb_exception e;
  struct cleanup *cleanup;
  enum exception_event_kind kind = classify_exception_breakpoint (self);
  int pass;

  for (pass = 0; sals.sals == NULL && pass < 2; ++pass)
    {
      TRY_CATCH (e, RETURN_MASK_ERROR)
	{
	  char *spec;

	  if (pass == 0)
	    {
	      spec = ASTRDUP (exception_functions[kind].probe);
	      sals = parse_probes (&spec, NULL);
	    }
	  else
	    {
	      spec = ASTRDUP (exception_functions[kind].function);
	      self->ops->decode_linespec (self, &spec, &sals);
	    }
	}
      /* NOT_FOUND_ERROR just means the breakpoint will be pending, so
	 let it through.  */
      if (e.reason < 0 && e.error != NOT_FOUND_ERROR)
	throw_exception (e);
    }

  cleanup = make_cleanup (xfree, sals.sals);
  update_breakpoint_locations (self, sals, sals_end);
  do_cleanups (cleanup);
}

static enum print_stop_action
print_it_exception_catchpoint (bpstat bs)
{
  struct ui_out *uiout = current_uiout;
  struct breakpoint *b = bs->breakpoint_at;
  int bp_temp;
  enum exception_event_kind kind = classify_exception_breakpoint (b);

  annotate_catchpoint (b->number);

  bp_temp = b->disposition == disp_del;
  ui_out_text (uiout, 
	       bp_temp ? "Temporary catchpoint "
		       : "Catchpoint ");
  if (!ui_out_is_mi_like_p (uiout))
    ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout,
	       (kind == EX_EVENT_THROW ? " (exception thrown), "
		: (kind == EX_EVENT_CATCH ? " (exception caught), "
		   : " (exception rethrown), ")));
  if (ui_out_is_mi_like_p (uiout))
    {
      ui_out_field_string (uiout, "reason", 
			   async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
      ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
      ui_out_field_int (uiout, "bkptno", b->number);
    }
  return PRINT_SRC_AND_LOC;
}

static void
print_one_exception_catchpoint (struct breakpoint *b, 
				struct bp_location **last_loc)
{
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;
  enum exception_event_kind kind = classify_exception_breakpoint (b);

  get_user_print_options (&opts);
  if (opts.addressprint)
    {
      annotate_field (4);
      if (b->loc == NULL || b->loc->shlib_disabled)
	ui_out_field_string (uiout, "addr", "<PENDING>");
      else
	ui_out_field_core_addr (uiout, "addr",
				b->loc->gdbarch, b->loc->address);
    }
  annotate_field (5);
  if (b->loc)
    *last_loc = b->loc;

  switch (kind)
    {
    case EX_EVENT_THROW:
      ui_out_field_string (uiout, "what", "exception throw");
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string (uiout, "catch-type", "throw");
      break;

    case EX_EVENT_RETHROW:
      ui_out_field_string (uiout, "what", "exception rethrow");
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string (uiout, "catch-type", "rethrow");
      break;

    case EX_EVENT_CATCH:
      ui_out_field_string (uiout, "what", "exception catch");
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string (uiout, "catch-type", "catch");
      break;
    }
}

/* Implement the 'print_one_detail' method.  */

static void
print_one_detail_exception_catchpoint (const struct breakpoint *b,
				       struct ui_out *uiout)
{
  const struct exception_catchpoint *cp
    = (const struct exception_catchpoint *) b;

  if (cp->exception_rx != NULL)
    {
      ui_out_text (uiout, _("\tmatching: "));
      ui_out_field_string (uiout, "regexp", cp->exception_rx);
      ui_out_text (uiout, "\n");
    }
}

static void
print_mention_exception_catchpoint (struct breakpoint *b)
{
  struct ui_out *uiout = current_uiout;
  int bp_temp;
  enum exception_event_kind kind = classify_exception_breakpoint (b);

  bp_temp = b->disposition == disp_del;
  ui_out_text (uiout, bp_temp ? _("Temporary catchpoint ")
			      : _("Catchpoint "));
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, (kind == EX_EVENT_THROW ? _(" (throw)")
		       : (kind == EX_EVENT_CATCH ? _(" (catch)")
			  : _(" (rethrow)"))));
}

/* Implement the "print_recreate" breakpoint_ops method for throw and
   catch catchpoints.  */

static void
print_recreate_exception_catchpoint (struct breakpoint *b, 
				     struct ui_file *fp)
{
  int bp_temp;
  enum exception_event_kind kind = classify_exception_breakpoint (b);

  bp_temp = b->disposition == disp_del;
  fprintf_unfiltered (fp, bp_temp ? "tcatch " : "catch ");
  switch (kind)
    {
    case EX_EVENT_THROW:
      fprintf_unfiltered (fp, "throw");
      break;
    case EX_EVENT_CATCH:
      fprintf_unfiltered (fp, "catch");
      break;
    case EX_EVENT_RETHROW:
      fprintf_unfiltered (fp, "rethrow");
      break;
    }
  print_recreate_thread (b, fp);
}

static void
handle_gnu_v3_exceptions (int tempflag, char *except_rx, char *cond_string,
			  enum exception_event_kind ex_event, int from_tty)
{
  struct exception_catchpoint *cp;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);
  regex_t *pattern = NULL;

  if (except_rx != NULL)
    {
      pattern = XNEW (regex_t);
      make_cleanup (xfree, pattern);

      compile_rx_or_error (pattern, except_rx,
			   _("invalid type-matching regexp"));
    }

  cp = XCNEW (struct exception_catchpoint);
  make_cleanup (xfree, cp);

  init_catchpoint (&cp->base, get_current_arch (), tempflag, cond_string,
		   &gnu_v3_exception_catchpoint_ops);
  /* We need to reset 'type' in order for code in breakpoint.c to do
     the right thing.  */
  cp->base.type = bp_breakpoint;
  cp->kind = ex_event;
  cp->exception_rx = except_rx;
  cp->pattern = pattern;

  re_set_exception_catchpoint (&cp->base);

  install_breakpoint (0, &cp->base, 1);
  discard_cleanups (cleanup);
}

/* Look for an "if" token in *STRING.  The "if" token must be preceded
   by whitespace.
   
   If there is any non-whitespace text between *STRING and the "if"
   token, then it is returned in a newly-xmalloc'd string.  Otherwise,
   this returns NULL.
   
   STRING is updated to point to the "if" token, if it exists, or to
   the end of the string.  */

static char *
extract_exception_regexp (char **string)
{
  char *start;
  char *last, *last_space;

  start = skip_spaces (*string);

  last = start;
  last_space = start;
  while (*last != '\0')
    {
      char *if_token = last;

      /* Check for the "if".  */
      if (check_for_argument (&if_token, "if", 2))
	break;

      /* No "if" token here.  Skip to the next word start.  */
      last_space = skip_to_space (last);
      last = skip_spaces (last_space);
    }

  *string = last;
  if (last_space > start)
    return savestring (start, last_space - start);
  return NULL;
}

/* Deal with "catch catch", "catch throw", and "catch rethrow"
   commands.  */

static void
catch_exception_command_1 (enum exception_event_kind ex_event, char *arg,
			   int tempflag, int from_tty)
{
  char *except_rx;
  char *cond_string = NULL;
  struct cleanup *cleanup;

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);

  except_rx = extract_exception_regexp (&arg);
  cleanup = make_cleanup (xfree, except_rx);

  cond_string = ep_parse_optional_if_clause (&arg);

  if ((*arg != '\0') && !isspace (*arg))
    error (_("Junk at end of arguments."));

  if (ex_event != EX_EVENT_THROW
      && ex_event != EX_EVENT_CATCH
      && ex_event != EX_EVENT_RETHROW)
    error (_("Unsupported or unknown exception event; cannot catch it"));

  handle_gnu_v3_exceptions (tempflag, except_rx, cond_string,
			    ex_event, from_tty);

  discard_cleanups (cleanup);
}

/* Implementation of "catch catch" command.  */

static void
catch_catch_command (char *arg, int from_tty, struct cmd_list_element *command)
{
  int tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  catch_exception_command_1 (EX_EVENT_CATCH, arg, tempflag, from_tty);
}

/* Implementation of "catch throw" command.  */

static void
catch_throw_command (char *arg, int from_tty, struct cmd_list_element *command)
{
  int tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  catch_exception_command_1 (EX_EVENT_THROW, arg, tempflag, from_tty);
}

/* Implementation of "catch rethrow" command.  */

static void
catch_rethrow_command (char *arg, int from_tty,
		       struct cmd_list_element *command)
{
  int tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  catch_exception_command_1 (EX_EVENT_RETHROW, arg, tempflag, from_tty);
}



/* Implement the 'make_value' method for the $_exception
   internalvar.  */

static struct value *
compute_exception (struct gdbarch *argc, struct internalvar *var, void *ignore)
{
  struct value *arg0, *arg1;
  struct type *obj_type;

  fetch_probe_arguments (&arg0, &arg1);

  /* ARG0 is a pointer to the exception object.  ARG1 is a pointer to
     the std::type_info for the exception.  Now we find the type from
     the type_info and cast the result.  */
  obj_type = cplus_type_from_type_info (arg1);
  return value_ind (value_cast (make_pointer_type (obj_type, NULL), arg0));
}

/* Implementation of the '$_exception' variable.  */

static const struct internalvar_funcs exception_funcs =
{
  compute_exception,
  NULL,
  NULL
};



static void
initialize_throw_catchpoint_ops (void)
{
  struct breakpoint_ops *ops;

  initialize_breakpoint_ops ();

  /* GNU v3 exception catchpoints.  */
  ops = &gnu_v3_exception_catchpoint_ops;
  *ops = bkpt_breakpoint_ops;
  ops->dtor = dtor_exception_catchpoint;
  ops->re_set = re_set_exception_catchpoint;
  ops->print_it = print_it_exception_catchpoint;
  ops->print_one = print_one_exception_catchpoint;
  ops->print_mention = print_mention_exception_catchpoint;
  ops->print_recreate = print_recreate_exception_catchpoint;
  ops->print_one_detail = print_one_detail_exception_catchpoint;
  ops->check_status = check_status_exception_catchpoint;
}

initialize_file_ftype _initialize_break_catch_throw;

void
_initialize_break_catch_throw (void)
{
  initialize_throw_catchpoint_ops ();

  /* Add catch and tcatch sub-commands.  */
  add_catch_command ("catch", _("\
Catch an exception, when caught."),
		     catch_catch_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("throw", _("\
Catch an exception, when thrown."),
		     catch_throw_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("rethrow", _("\
Catch an exception, when rethrown."),
		     catch_rethrow_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);

  create_internalvar_type_lazy ("_exception", &exception_funcs, NULL);
}
