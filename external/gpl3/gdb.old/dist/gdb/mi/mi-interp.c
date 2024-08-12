/* MI Interpreter Definitions and Commands for GDB, the GNU debugger.

   Copyright (C) 2002-2023 Free Software Foundation, Inc.

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

#include "mi-interp.h"

#include "interps.h"
#include "event-top.h"
#include "gdbsupport/event-loop.h"
#include "inferior.h"
#include "infrun.h"
#include "ui-out.h"
#include "top.h"
#include "mi-main.h"
#include "mi-cmds.h"
#include "mi-out.h"
#include "mi-console.h"
#include "mi-common.h"
#include "observable.h"
#include "gdbthread.h"
#include "solist.h"
#include "objfiles.h"
#include "tracepoint.h"
#include "cli-out.h"
#include "thread-fsm.h"
#include "cli/cli-interp.h"
#include "gdbsupport/scope-exit.h"

/* These are the interpreter setup, etc. functions for the MI
   interpreter.  */

static void mi_execute_command_wrapper (const char *cmd);
static void mi_execute_command_input_handler
  (gdb::unique_xmalloc_ptr<char> &&cmd);

/* These are hooks that we put in place while doing interpreter_exec
   so we can report interesting things that happened "behind the MI's
   back" in this command.  */

static int mi_interp_query_hook (const char *ctlstr, va_list ap)
  ATTRIBUTE_PRINTF (1, 0);

static void mi_insert_notify_hooks (void);
static void mi_remove_notify_hooks (void);

static void mi_on_signal_received (enum gdb_signal siggnal);
static void mi_on_end_stepping_range (void);
static void mi_on_signal_exited (enum gdb_signal siggnal);
static void mi_on_exited (int exitstatus);
static void mi_on_normal_stop (struct bpstat *bs, int print_frame);
static void mi_on_no_history (void);

static void mi_new_thread (struct thread_info *t);
static void mi_thread_exit (struct thread_info *t, int silent);
static void mi_record_changed (struct inferior*, int, const char *,
			       const char *);
static void mi_inferior_added (struct inferior *inf);
static void mi_inferior_appeared (struct inferior *inf);
static void mi_inferior_exit (struct inferior *inf);
static void mi_inferior_removed (struct inferior *inf);
static void mi_on_resume (ptid_t ptid);
static void mi_solib_loaded (struct so_list *solib);
static void mi_solib_unloaded (struct so_list *solib);
static void mi_about_to_proceed (void);
static void mi_traceframe_changed (int tfnum, int tpnum);
static void mi_tsv_created (const struct trace_state_variable *tsv);
static void mi_tsv_deleted (const struct trace_state_variable *tsv);
static void mi_tsv_modified (const struct trace_state_variable *tsv);
static void mi_breakpoint_created (struct breakpoint *b);
static void mi_breakpoint_deleted (struct breakpoint *b);
static void mi_breakpoint_modified (struct breakpoint *b);
static void mi_command_param_changed (const char *param, const char *value);
static void mi_memory_changed (struct inferior *inf, CORE_ADDR memaddr,
			       ssize_t len, const bfd_byte *myaddr);
static void mi_on_sync_execution_done (void);

/* Display the MI prompt.  */

static void
display_mi_prompt (struct mi_interp *mi)
{
  struct ui *ui = current_ui;

  gdb_puts ("(gdb) \n", mi->raw_stdout);
  gdb_flush (mi->raw_stdout);
  ui->prompt_state = PROMPTED;
}

/* Returns the INTERP's data cast as mi_interp if INTERP is an MI, and
   returns NULL otherwise.  */

static struct mi_interp *
as_mi_interp (struct interp *interp)
{
  return dynamic_cast<mi_interp *> (interp);
}

/* Observer for the command_error notification.  */

static void
mi_on_command_error ()
{
  mi_interp *mi = as_mi_interp (top_level_interpreter ());
  if (mi != nullptr)
    display_mi_prompt (mi);
}

void
mi_interp::init (bool top_level)
{
  mi_interp *mi = this;

  /* Store the current output channel, so that we can create a console
     channel that encapsulates and prefixes all gdb_output-type bits
     coming from the rest of the debugger.  */
  mi->raw_stdout = gdb_stdout;

  /* Create MI console channels, each with a different prefix so they
     can be distinguished.  */
  mi->out = new mi_console_file (mi->raw_stdout, "~", '"');
  mi->err = new mi_console_file (mi->raw_stdout, "&", '"');
  mi->log = mi->err;
  mi->targ = new mi_console_file (mi->raw_stdout, "@", '"');
  mi->event_channel = new mi_console_file (mi->raw_stdout, "=", 0);
  mi->mi_uiout = mi_out_new (name ());
  gdb_assert (mi->mi_uiout != nullptr);
  mi->cli_uiout = new cli_ui_out (mi->out);

  if (top_level)
    {
      /* The initial inferior is created before this function is called, so we
	 need to report it explicitly when initializing the top-level MI
	 interpreter.

	 This is also called when additional MI interpreters are added (using
	 the new-ui command), when multiple inferiors possibly exist, so we need
	 to use iteration to report all the inferiors.  mi_inferior_added can't
	 be used, because it would print the event on all the other MI UIs.  */

      for (inferior *inf : all_inferiors ())
	{
	  target_terminal::scoped_restore_terminal_state term_state;
	  target_terminal::ours_for_output ();

	  gdb_printf (mi->event_channel,
		      "thread-group-added,id=\"i%d\"",
		      inf->num);

	  gdb_flush (mi->event_channel);
	}
  }
}

void
mi_interp::resume ()
{
  struct mi_interp *mi = this;
  struct ui *ui = current_ui;

  /* As per hack note in mi_interpreter_init, swap in the output
     channels... */
  gdb_setup_readline (0);

  ui->call_readline = gdb_readline_no_editing_callback;
  ui->input_handler = mi_execute_command_input_handler;

  gdb_stdout = mi->out;
  /* Route error and log output through the MI.  */
  gdb_stderr = mi->err;
  gdb_stdlog = mi->log;
  /* Route target output through the MI.  */
  gdb_stdtarg = mi->targ;
  /* Route target error through the MI as well.  */
  gdb_stdtargerr = mi->targ;

  deprecated_show_load_progress = mi_load_progress;
}

void
mi_interp::suspend ()
{
  gdb_disable_readline ();
}

gdb_exception
mi_interp::exec (const char *command)
{
  mi_execute_command_wrapper (command);
  return gdb_exception ();
}

void
mi_cmd_interpreter_exec (const char *command, char **argv, int argc)
{
  struct interp *interp_to_use;
  int i;

  if (argc < 2)
    error (_("-interpreter-exec: "
	     "Usage: -interpreter-exec interp command"));

  interp_to_use = interp_lookup (current_ui, argv[0]);
  if (interp_to_use == NULL)
    error (_("-interpreter-exec: could not find interpreter \"%s\""),
	   argv[0]);

  /* Note that unlike the CLI version of this command, we don't
     actually set INTERP_TO_USE as the current interpreter, as we
     still want gdb_stdout, etc. to point at MI streams.  */

  /* Insert the MI out hooks, making sure to also call the
     interpreter's hooks if it has any.  */
  /* KRS: We shouldn't need this... Events should be installed and
     they should just ALWAYS fire something out down the MI
     channel.  */
  mi_insert_notify_hooks ();

  /* Now run the code.  */

  SCOPE_EXIT
    {
      mi_remove_notify_hooks ();
    };

  for (i = 1; i < argc; i++)
    {
      struct gdb_exception e = interp_exec (interp_to_use, argv[i]);

      if (e.reason < 0)
	error ("%s", e.what ());
    }
}

/* This inserts a number of hooks that are meant to produce
   async-notify ("=") MI messages while running commands in another
   interpreter using mi_interpreter_exec.  The canonical use for this
   is to allow access to the gdb CLI interpreter from within the MI,
   while still producing MI style output when actions in the CLI
   command change GDB's state.  */

static void
mi_insert_notify_hooks (void)
{
  deprecated_query_hook = mi_interp_query_hook;
}

static void
mi_remove_notify_hooks (void)
{
  deprecated_query_hook = NULL;
}

static int
mi_interp_query_hook (const char *ctlstr, va_list ap)
{
  return 1;
}

static void
mi_execute_command_wrapper (const char *cmd)
{
  struct ui *ui = current_ui;

  mi_execute_command (cmd, ui->instream == ui->stdin_stream);
}

/* Observer for the synchronous_command_done notification.  */

static void
mi_on_sync_execution_done (void)
{
  struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

  if (mi == NULL)
    return;

  /* If MI is sync, then output the MI prompt now, indicating we're
     ready for further input.  */
  if (!mi_async_p ())
    display_mi_prompt (mi);
}

/* mi_execute_command_wrapper wrapper suitable for INPUT_HANDLER.  */

static void
mi_execute_command_input_handler (gdb::unique_xmalloc_ptr<char> &&cmd)
{
  struct mi_interp *mi = as_mi_interp (top_level_interpreter ());
  struct ui *ui = current_ui;

  ui->prompt_state = PROMPT_NEEDED;

  mi_execute_command_wrapper (cmd.get ());

  /* Print a prompt, indicating we're ready for further input, unless
     we just started a synchronous command.  In that case, we're about
     to go back to the event loop and will output the prompt in the
     'synchronous_command_done' observer when the target next
     stops.  */
  if (ui->prompt_state == PROMPT_NEEDED)
    display_mi_prompt (mi);
}

void
mi_interp::pre_command_loop ()
{
  struct mi_interp *mi = this;

  /* Turn off 8 bit strings in quoted output.  Any character with the
     high bit set is printed using C's octal format.  */
  sevenbit_strings = 1;

  /* Tell the world that we're alive.  */
  display_mi_prompt (mi);
}

static void
mi_new_thread (struct thread_info *t)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel,
		  "thread-created,id=\"%d\",group-id=\"i%d\"",
		  t->global_num, t->inf->num);
      gdb_flush (mi->event_channel);
    }
}

static void
mi_thread_exit (struct thread_info *t, int silent)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();
      gdb_printf (mi->event_channel,
		  "thread-exited,id=\"%d\",group-id=\"i%d\"",
		  t->global_num, t->inf->num);
      gdb_flush (mi->event_channel);
    }
}

/* Emit notification on changing the state of record.  */

static void
mi_record_changed (struct inferior *inferior, int started, const char *method,
		   const char *format)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      if (started)
	{
	  if (format != NULL)
	    {
	      gdb_printf (mi->event_channel,
			  "record-started,thread-group=\"i%d\","
			  "method=\"%s\",format=\"%s\"",
			  inferior->num, method, format);
	    }
	  else
	    {
	      gdb_printf (mi->event_channel,
			  "record-started,thread-group=\"i%d\","
			  "method=\"%s\"",
			  inferior->num, method);
	    }
	}
      else
	{
	  gdb_printf (mi->event_channel,
		      "record-stopped,thread-group=\"i%d\"",
		      inferior->num);
	}

      gdb_flush (mi->event_channel);
    }
}

static void
mi_inferior_added (struct inferior *inf)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct interp *interp;
      struct mi_interp *mi;

      /* We'll be called once for the initial inferior, before the top
	 level interpreter is set.  */
      interp = top_level_interpreter ();
      if (interp == NULL)
	continue;

      mi = as_mi_interp (interp);
      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel,
		  "thread-group-added,id=\"i%d\"",
		  inf->num);
      gdb_flush (mi->event_channel);
    }
}

static void
mi_inferior_appeared (struct inferior *inf)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel,
		  "thread-group-started,id=\"i%d\",pid=\"%d\"",
		  inf->num, inf->pid);
      gdb_flush (mi->event_channel);
    }
}

static void
mi_inferior_exit (struct inferior *inf)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      if (inf->has_exit_code)
	gdb_printf (mi->event_channel,
		    "thread-group-exited,id=\"i%d\",exit-code=\"%s\"",
		    inf->num, int_string (inf->exit_code, 8, 0, 0, 1));
      else
	gdb_printf (mi->event_channel,
		    "thread-group-exited,id=\"i%d\"", inf->num);

      gdb_flush (mi->event_channel);
    }
}

static void
mi_inferior_removed (struct inferior *inf)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel,
		  "thread-group-removed,id=\"i%d\"",
		  inf->num);
      gdb_flush (mi->event_channel);
    }
}

/* Return the MI interpreter, if it is active -- either because it's
   the top-level interpreter or the interpreter executing the current
   command.  Returns NULL if the MI interpreter is not being used.  */

static struct mi_interp *
find_mi_interp (void)
{
  struct mi_interp *mi;

  mi = as_mi_interp (top_level_interpreter ());
  if (mi != NULL)
    return mi;

  mi = as_mi_interp (command_interp ());
  if (mi != NULL)
    return mi;

  return NULL;
}

/* Observers for several run control events that print why the
   inferior has stopped to both the MI event channel and to the MI
   console.  If the MI interpreter is not active, print nothing.  */

/* Observer for the signal_received notification.  */

static void
mi_on_signal_received (enum gdb_signal siggnal)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = find_mi_interp ();

      if (mi == NULL)
	continue;

      print_signal_received_reason (mi->mi_uiout, siggnal);
      print_signal_received_reason (mi->cli_uiout, siggnal);
    }
}

/* Observer for the end_stepping_range notification.  */

static void
mi_on_end_stepping_range (void)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = find_mi_interp ();

      if (mi == NULL)
	continue;

      print_end_stepping_range_reason (mi->mi_uiout);
      print_end_stepping_range_reason (mi->cli_uiout);
    }
}

/* Observer for the signal_exited notification.  */

static void
mi_on_signal_exited (enum gdb_signal siggnal)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = find_mi_interp ();

      if (mi == NULL)
	continue;

      print_signal_exited_reason (mi->mi_uiout, siggnal);
      print_signal_exited_reason (mi->cli_uiout, siggnal);
    }
}

/* Observer for the exited notification.  */

static void
mi_on_exited (int exitstatus)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = find_mi_interp ();

      if (mi == NULL)
	continue;

      print_exited_reason (mi->mi_uiout, exitstatus);
      print_exited_reason (mi->cli_uiout, exitstatus);
    }
}

/* Observer for the no_history notification.  */

static void
mi_on_no_history (void)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = find_mi_interp ();

      if (mi == NULL)
	continue;

      print_no_history_reason (mi->mi_uiout);
      print_no_history_reason (mi->cli_uiout);
    }
}

static void
mi_on_normal_stop_1 (struct bpstat *bs, int print_frame)
{
  /* Since this can be called when CLI command is executing,
     using cli interpreter, be sure to use MI uiout for output,
     not the current one.  */
  struct ui_out *mi_uiout = top_level_interpreter ()->interp_ui_out ();
  struct mi_interp *mi = (struct mi_interp *) top_level_interpreter ();

  if (print_frame)
    {
      struct thread_info *tp;
      int core;
      struct interp *console_interp;

      tp = inferior_thread ();

      if (tp->thread_fsm () != nullptr
	  && tp->thread_fsm ()->finished_p ())
	{
	  enum async_reply_reason reason;

	  reason = tp->thread_fsm ()->async_reply_reason ();
	  mi_uiout->field_string ("reason", async_reason_lookup (reason));
	}

      console_interp = interp_lookup (current_ui, INTERP_CONSOLE);
      /* We only want to print the displays once, and we want it to
	 look just how it would on the console, so we use this to
	 decide whether the MI stop should include them.  */
      bool console_print = should_print_stop_to_console (console_interp, tp);
      print_stop_event (mi_uiout, !console_print);

      if (console_print)
	print_stop_event (mi->cli_uiout);

      mi_uiout->field_signed ("thread-id", tp->global_num);
      if (non_stop)
	{
	  ui_out_emit_list list_emitter (mi_uiout, "stopped-threads");

	  mi_uiout->field_signed (NULL, tp->global_num);
	}
      else
	mi_uiout->field_string ("stopped-threads", "all");

      core = target_core_of_thread (tp->ptid);
      if (core != -1)
	mi_uiout->field_signed ("core", core);
    }
  
  gdb_puts ("*stopped", mi->raw_stdout);
  mi_out_put (mi_uiout, mi->raw_stdout);
  mi_out_rewind (mi_uiout);
  mi_print_timing_maybe (mi->raw_stdout);
  gdb_puts ("\n", mi->raw_stdout);
  gdb_flush (mi->raw_stdout);
}

static void
mi_on_normal_stop (struct bpstat *bs, int print_frame)
{
  SWITCH_THRU_ALL_UIS ()
    {
      if (as_mi_interp (top_level_interpreter ()) == NULL)
	continue;

      mi_on_normal_stop_1 (bs, print_frame);
    }
}

static void
mi_about_to_proceed (void)
{
  /* Suppress output while calling an inferior function.  */

  if (inferior_ptid != null_ptid)
    {
      struct thread_info *tp = inferior_thread ();

      if (tp->control.in_infcall)
	return;
    }

  mi_proceeded = 1;
}

/* When the element is non-zero, no MI notifications will be emitted in
   response to the corresponding observers.  */

struct mi_suppress_notification mi_suppress_notification =
  {
    0,
    0,
    0,
    0,
  };

/* Emit notification on changing a traceframe.  */

static void
mi_traceframe_changed (int tfnum, int tpnum)
{
  if (mi_suppress_notification.traceframe)
    return;

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      if (tfnum >= 0)
	gdb_printf (mi->event_channel, "traceframe-changed,"
		    "num=\"%d\",tracepoint=\"%d\"",
		    tfnum, tpnum);
      else
	gdb_printf (mi->event_channel, "traceframe-changed,end");

      gdb_flush (mi->event_channel);
    }
}

/* Emit notification on creating a trace state variable.  */

static void
mi_tsv_created (const struct trace_state_variable *tsv)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel, "tsv-created,"
		  "name=\"%s\",initial=\"%s\"",
		  tsv->name.c_str (), plongest (tsv->initial_value));

      gdb_flush (mi->event_channel);
    }
}

/* Emit notification on deleting a trace state variable.  */

static void
mi_tsv_deleted (const struct trace_state_variable *tsv)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      if (tsv != NULL)
	gdb_printf (mi->event_channel, "tsv-deleted,"
		    "name=\"%s\"", tsv->name.c_str ());
      else
	gdb_printf (mi->event_channel, "tsv-deleted");

      gdb_flush (mi->event_channel);
    }
}

/* Emit notification on modifying a trace state variable.  */

static void
mi_tsv_modified (const struct trace_state_variable *tsv)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());
      struct ui_out *mi_uiout;

      if (mi == NULL)
	continue;

      mi_uiout = top_level_interpreter ()->interp_ui_out ();

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel,
		  "tsv-modified");

      ui_out_redirect_pop redir (mi_uiout, mi->event_channel);

      mi_uiout->field_string ("name", tsv->name);
      mi_uiout->field_string ("initial",
			      plongest (tsv->initial_value));
      if (tsv->value_known)
	mi_uiout->field_string ("current", plongest (tsv->value));

      gdb_flush (mi->event_channel);
    }
}

/* Print breakpoint BP on MI's event channel.  */

static void
mi_print_breakpoint_for_event (struct mi_interp *mi, breakpoint *bp)
{
  ui_out *mi_uiout = mi->interp_ui_out ();

  /* We want the output from print_breakpoint to go to
     mi->event_channel.  One approach would be to just call
     print_breakpoint, and then use mi_out_put to send the current
     content of mi_uiout into mi->event_channel.  However, that will
     break if anything is output to mi_uiout prior to calling the
     breakpoint_created notifications.  So, we use
     ui_out_redirect.  */
  ui_out_redirect_pop redir (mi_uiout, mi->event_channel);

  try
    {
      scoped_restore restore_uiout
	= make_scoped_restore (&current_uiout, mi_uiout);

      print_breakpoint (bp);
    }
  catch (const gdb_exception &ex)
    {
      exception_print (gdb_stderr, ex);
    }
}

/* Emit notification about a created breakpoint.  */

static void
mi_breakpoint_created (struct breakpoint *b)
{
  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel,
		  "breakpoint-created");
      mi_print_breakpoint_for_event (mi, b);

      gdb_flush (mi->event_channel);
    }
}

/* Emit notification about deleted breakpoint.  */

static void
mi_breakpoint_deleted (struct breakpoint *b)
{
  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel, "breakpoint-deleted,id=\"%d\"",
		  b->number);

      gdb_flush (mi->event_channel);
    }
}

/* Emit notification about modified breakpoint.  */

static void
mi_breakpoint_modified (struct breakpoint *b)
{
  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();
      gdb_printf (mi->event_channel,
		  "breakpoint-modified");
      mi_print_breakpoint_for_event (mi, b);

      gdb_flush (mi->event_channel);
    }
}

static void
mi_output_running (struct thread_info *thread)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      gdb_printf (mi->raw_stdout,
		  "*running,thread-id=\"%d\"\n",
		  thread->global_num);
    }
}

/* Return true if there are multiple inferiors loaded.  This is used
   for backwards compatibility -- if there's only one inferior, output
   "all", otherwise, output each resumed thread individually.  */

static bool
multiple_inferiors_p ()
{
  int count = 0;
  for (inferior *inf ATTRIBUTE_UNUSED : all_non_exited_inferiors ())
    {
      count++;
      if (count > 1)
	return true;
    }

  return false;
}

static void
mi_on_resume_1 (struct mi_interp *mi,
		process_stratum_target *targ, ptid_t ptid)
{
  /* To cater for older frontends, emit ^running, but do it only once
     per each command.  We do it here, since at this point we know
     that the target was successfully resumed, and in non-async mode,
     we won't return back to MI interpreter code until the target
     is done running, so delaying the output of "^running" until then
     will make it impossible for frontend to know what's going on.

     In future (MI3), we'll be outputting "^done" here.  */
  if (!running_result_record_printed && mi_proceeded)
    {
      gdb_printf (mi->raw_stdout, "%s^running\n",
		  current_token ? current_token : "");
    }

  /* Backwards compatibility.  If doing a wildcard resume and there's
     only one inferior, output "all", otherwise, output each resumed
     thread individually.  */
  if ((ptid == minus_one_ptid || ptid.is_pid ())
      && !multiple_inferiors_p ())
    gdb_printf (mi->raw_stdout, "*running,thread-id=\"all\"\n");
  else
    for (thread_info *tp : all_non_exited_threads (targ, ptid))
      mi_output_running (tp);

  if (!running_result_record_printed && mi_proceeded)
    {
      running_result_record_printed = 1;
      /* This is what gdb used to do historically -- printing prompt
	 even if it cannot actually accept any input.  This will be
	 surely removed for MI3, and may be removed even earlier.  */
      if (current_ui->prompt_state == PROMPT_BLOCKED)
	gdb_puts ("(gdb) \n", mi->raw_stdout);
    }
  gdb_flush (mi->raw_stdout);
}

static void
mi_on_resume (ptid_t ptid)
{
  struct thread_info *tp = NULL;

  process_stratum_target *target = current_inferior ()->process_target ();
  if (ptid == minus_one_ptid || ptid.is_pid ())
    tp = inferior_thread ();
  else
    tp = find_thread_ptid (target, ptid);

  /* Suppress output while calling an inferior function.  */
  if (tp->control.in_infcall)
    return;

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      mi_on_resume_1 (mi, target, ptid);
    }
}

/* See mi-interp.h.  */

void
mi_output_solib_attribs (ui_out *uiout, struct so_list *solib)
{
  struct gdbarch *gdbarch = target_gdbarch ();

  uiout->field_string ("id", solib->so_original_name);
  uiout->field_string ("target-name", solib->so_original_name);
  uiout->field_string ("host-name", solib->so_name);
  uiout->field_signed ("symbols-loaded", solib->symbols_loaded);
  if (!gdbarch_has_global_solist (target_gdbarch ()))
      uiout->field_fmt ("thread-group", "i%d", current_inferior ()->num);

  ui_out_emit_list list_emitter (uiout, "ranges");
  ui_out_emit_tuple tuple_emitter (uiout, NULL);
  if (solib->addr_high != 0)
    {
      uiout->field_core_addr ("from", gdbarch, solib->addr_low);
      uiout->field_core_addr ("to", gdbarch, solib->addr_high);
    }
}

static void
mi_solib_loaded (struct so_list *solib)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());
      struct ui_out *uiout;

      if (mi == NULL)
	continue;

      uiout = top_level_interpreter ()->interp_ui_out ();

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel, "library-loaded");

      ui_out_redirect_pop redir (uiout, mi->event_channel);

      mi_output_solib_attribs (uiout, solib);

      gdb_flush (mi->event_channel);
    }
}

static void
mi_solib_unloaded (struct so_list *solib)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());
      struct ui_out *uiout;

      if (mi == NULL)
	continue;

      uiout = top_level_interpreter ()->interp_ui_out ();

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel, "library-unloaded");

      ui_out_redirect_pop redir (uiout, mi->event_channel);

      uiout->field_string ("id", solib->so_original_name);
      uiout->field_string ("target-name", solib->so_original_name);
      uiout->field_string ("host-name", solib->so_name);
      if (!gdbarch_has_global_solist (target_gdbarch ()))
	{
	  uiout->field_fmt ("thread-group", "i%d", current_inferior ()->num);
	}

      gdb_flush (mi->event_channel);
    }
}

/* Emit notification about the command parameter change.  */

static void
mi_command_param_changed (const char *param, const char *value)
{
  if (mi_suppress_notification.cmd_param_changed)
    return;

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());
      struct ui_out *mi_uiout;

      if (mi == NULL)
	continue;

      mi_uiout = top_level_interpreter ()->interp_ui_out ();

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel, "cmd-param-changed");

      ui_out_redirect_pop redir (mi_uiout, mi->event_channel);

      mi_uiout->field_string ("param", param);
      mi_uiout->field_string ("value", value);

      gdb_flush (mi->event_channel);
    }
}

/* Emit notification about the target memory change.  */

static void
mi_memory_changed (struct inferior *inferior, CORE_ADDR memaddr,
		   ssize_t len, const bfd_byte *myaddr)
{
  if (mi_suppress_notification.memory)
    return;

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());
      struct ui_out *mi_uiout;
      struct obj_section *sec;

      if (mi == NULL)
	continue;

      mi_uiout = top_level_interpreter ()->interp_ui_out ();

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel, "memory-changed");

      ui_out_redirect_pop redir (mi_uiout, mi->event_channel);

      mi_uiout->field_fmt ("thread-group", "i%d", inferior->num);
      mi_uiout->field_core_addr ("addr", target_gdbarch (), memaddr);
      mi_uiout->field_string ("len", hex_string (len));

      /* Append 'type=code' into notification if MEMADDR falls in the range of
	 sections contain code.  */
      sec = find_pc_section (memaddr);
      if (sec != NULL && sec->objfile != NULL)
	{
	  flagword flags = bfd_section_flags (sec->the_bfd_section);

	  if (flags & SEC_CODE)
	    mi_uiout->field_string ("type", "code");
	}

      gdb_flush (mi->event_channel);
    }
}

/* Emit an event when the selection context (inferior, thread, frame)
   changed.  */

static void
mi_user_selected_context_changed (user_selected_what selection)
{
  struct thread_info *tp;

  /* Don't send an event if we're responding to an MI command.  */
  if (mi_suppress_notification.user_selected_context)
    return;

  if (inferior_ptid != null_ptid)
    tp = inferior_thread ();
  else
    tp = NULL;

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());
      struct ui_out *mi_uiout;

      if (mi == NULL)
	continue;

      mi_uiout = top_level_interpreter ()->interp_ui_out ();

      ui_out_redirect_pop redirect_popper (mi_uiout, mi->event_channel);

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      if (selection & USER_SELECTED_INFERIOR)
	print_selected_inferior (mi->cli_uiout);

      if (tp != NULL
	  && (selection & (USER_SELECTED_THREAD | USER_SELECTED_FRAME)))
	{
	  print_selected_thread_frame (mi->cli_uiout, selection);

	  gdb_printf (mi->event_channel,
		      "thread-selected,id=\"%d\"",
		      tp->global_num);

	  if (tp->state != THREAD_RUNNING)
	    {
	      if (has_stack_frames ())
		print_stack_frame_to_uiout (mi_uiout, get_selected_frame (NULL),
					    1, SRC_AND_LOC, 1);
	    }
	}

      gdb_flush (mi->event_channel);
    }
}

ui_out *
mi_interp::interp_ui_out ()
{
  return this->mi_uiout;
}

/* Do MI-specific logging actions; save raw_stdout, and change all
   the consoles to use the supplied ui-file(s).  */

void
mi_interp::set_logging (ui_file_up logfile, bool logging_redirect,
			bool debug_redirect)
{
  struct mi_interp *mi = this;

  if (logfile != NULL)
    {
      mi->saved_raw_stdout = mi->raw_stdout;

      ui_file *logfile_p = logfile.get ();
      mi->logfile_holder = std::move (logfile);

      /* If something is not being redirected, then a tee containing both the
	 logfile and stdout.  */
      ui_file *tee = nullptr;
      if (!logging_redirect || !debug_redirect)
	{
	  tee = new tee_file (mi->raw_stdout, logfile_p);
	  mi->stdout_holder.reset (tee);
	}

      mi->raw_stdout = logging_redirect ? logfile_p : tee;
    }
  else
    {
      mi->logfile_holder.reset ();
      mi->stdout_holder.reset ();
      mi->raw_stdout = mi->saved_raw_stdout;
      mi->saved_raw_stdout = nullptr;
    }

  mi->out->set_raw (mi->raw_stdout);
  mi->err->set_raw (mi->raw_stdout);
  mi->log->set_raw (mi->raw_stdout);
  mi->targ->set_raw (mi->raw_stdout);
  mi->event_channel->set_raw (mi->raw_stdout);
}

/* Factory for MI interpreters.  */

static struct interp *
mi_interp_factory (const char *name)
{
  return new mi_interp (name);
}

void _initialize_mi_interp ();
void
_initialize_mi_interp ()
{
  /* The various interpreter levels.  */
  interp_factory_register (INTERP_MI1, mi_interp_factory);
  interp_factory_register (INTERP_MI2, mi_interp_factory);
  interp_factory_register (INTERP_MI3, mi_interp_factory);
  interp_factory_register (INTERP_MI4, mi_interp_factory);
  interp_factory_register (INTERP_MI, mi_interp_factory);

  gdb::observers::signal_received.attach (mi_on_signal_received, "mi-interp");
  gdb::observers::end_stepping_range.attach (mi_on_end_stepping_range,
					     "mi-interp");
  gdb::observers::signal_exited.attach (mi_on_signal_exited, "mi-interp");
  gdb::observers::exited.attach (mi_on_exited, "mi-interp");
  gdb::observers::no_history.attach (mi_on_no_history, "mi-interp");
  gdb::observers::new_thread.attach (mi_new_thread, "mi-interp");
  gdb::observers::thread_exit.attach (mi_thread_exit, "mi-interp");
  gdb::observers::inferior_added.attach (mi_inferior_added, "mi-interp");
  gdb::observers::inferior_appeared.attach (mi_inferior_appeared, "mi-interp");
  gdb::observers::inferior_exit.attach (mi_inferior_exit, "mi-interp");
  gdb::observers::inferior_removed.attach (mi_inferior_removed, "mi-interp");
  gdb::observers::record_changed.attach (mi_record_changed, "mi-interp");
  gdb::observers::normal_stop.attach (mi_on_normal_stop, "mi-interp");
  gdb::observers::target_resumed.attach (mi_on_resume, "mi-interp");
  gdb::observers::solib_loaded.attach (mi_solib_loaded, "mi-interp");
  gdb::observers::solib_unloaded.attach (mi_solib_unloaded, "mi-interp");
  gdb::observers::about_to_proceed.attach (mi_about_to_proceed, "mi-interp");
  gdb::observers::traceframe_changed.attach (mi_traceframe_changed,
					     "mi-interp");
  gdb::observers::tsv_created.attach (mi_tsv_created, "mi-interp");
  gdb::observers::tsv_deleted.attach (mi_tsv_deleted, "mi-interp");
  gdb::observers::tsv_modified.attach (mi_tsv_modified, "mi-interp");
  gdb::observers::breakpoint_created.attach (mi_breakpoint_created,
					     "mi-interp");
  gdb::observers::breakpoint_deleted.attach (mi_breakpoint_deleted,
					     "mi-interp");
  gdb::observers::breakpoint_modified.attach (mi_breakpoint_modified,
					      "mi-interp");
  gdb::observers::command_param_changed.attach (mi_command_param_changed,
						"mi-interp");
  gdb::observers::command_error.attach (mi_on_command_error, "mi-interp");
  gdb::observers::memory_changed.attach (mi_memory_changed, "mi-interp");
  gdb::observers::sync_execution_done.attach (mi_on_sync_execution_done,
					      "mi-interp");
  gdb::observers::user_selected_context_changed.attach
    (mi_user_selected_context_changed, "mi-interp");
}
