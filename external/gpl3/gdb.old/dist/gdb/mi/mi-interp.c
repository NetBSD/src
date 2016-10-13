/* MI Interpreter Definitions and Commands for GDB, the GNU debugger.

   Copyright (C) 2002-2015 Free Software Foundation, Inc.

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
#include "interps.h"
#include "event-top.h"
#include "event-loop.h"
#include "inferior.h"
#include "infrun.h"
#include "ui-out.h"
#include "top.h"
#include "mi-main.h"
#include "mi-cmds.h"
#include "mi-out.h"
#include "mi-console.h"
#include "mi-common.h"
#include "observer.h"
#include "gdbthread.h"
#include "solist.h"
#include "gdb.h"
#include "objfiles.h"
#include "tracepoint.h"
#include "cli-out.h"

/* These are the interpreter setup, etc. functions for the MI
   interpreter.  */

static void mi_execute_command_wrapper (const char *cmd);
static void mi_execute_command_input_handler (char *cmd);
static void mi_command_loop (void *data);

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
static void mi_on_normal_stop (struct bpstats *bs, int print_frame);
static void mi_on_no_history (void);

static void mi_new_thread (struct thread_info *t);
static void mi_thread_exit (struct thread_info *t, int silent);
static void mi_record_changed (struct inferior*, int);
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

static int report_initial_inferior (struct inferior *inf, void *closure);

static void *
mi_interpreter_init (struct interp *interp, int top_level)
{
  struct mi_interp *mi = XNEW (struct mi_interp);
  const char *name;
  int mi_version;

  /* Assign the output channel created at startup to its own global,
     so that we can create a console channel that encapsulates and
     prefixes all gdb_output-type bits coming from the rest of the
     debugger.  */

  raw_stdout = gdb_stdout;

  /* Create MI console channels, each with a different prefix so they
     can be distinguished.  */
  mi->out = mi_console_file_new (raw_stdout, "~", '"');
  mi->err = mi_console_file_new (raw_stdout, "&", '"');
  mi->log = mi->err;
  mi->targ = mi_console_file_new (raw_stdout, "@", '"');
  mi->event_channel = mi_console_file_new (raw_stdout, "=", 0);

  name = interp_name (interp);
  /* INTERP_MI selects the most recent released version.  "mi2" was
     released as part of GDB 6.0.  */
  if (strcmp (name, INTERP_MI) == 0)
    mi_version = 2;
  else if (strcmp (name, INTERP_MI1) == 0)
    mi_version = 1;
  else if (strcmp (name, INTERP_MI2) == 0)
    mi_version = 2;
  else if (strcmp (name, INTERP_MI3) == 0)
    mi_version = 3;
  else
    gdb_assert_not_reached ("unhandled MI version");

  mi->mi_uiout = mi_out_new (mi_version);
  mi->cli_uiout = cli_out_new (mi->out);

  /* There are installed even if MI is not the top level interpreter.
     The callbacks themselves decide whether to be skipped.  */
  observer_attach_signal_received (mi_on_signal_received);
  observer_attach_end_stepping_range (mi_on_end_stepping_range);
  observer_attach_signal_exited (mi_on_signal_exited);
  observer_attach_exited (mi_on_exited);
  observer_attach_no_history (mi_on_no_history);

  if (top_level)
    {
      observer_attach_new_thread (mi_new_thread);
      observer_attach_thread_exit (mi_thread_exit);
      observer_attach_inferior_added (mi_inferior_added);
      observer_attach_inferior_appeared (mi_inferior_appeared);
      observer_attach_inferior_exit (mi_inferior_exit);
      observer_attach_inferior_removed (mi_inferior_removed);
      observer_attach_record_changed (mi_record_changed);
      observer_attach_normal_stop (mi_on_normal_stop);
      observer_attach_target_resumed (mi_on_resume);
      observer_attach_solib_loaded (mi_solib_loaded);
      observer_attach_solib_unloaded (mi_solib_unloaded);
      observer_attach_about_to_proceed (mi_about_to_proceed);
      observer_attach_traceframe_changed (mi_traceframe_changed);
      observer_attach_tsv_created (mi_tsv_created);
      observer_attach_tsv_deleted (mi_tsv_deleted);
      observer_attach_tsv_modified (mi_tsv_modified);
      observer_attach_breakpoint_created (mi_breakpoint_created);
      observer_attach_breakpoint_deleted (mi_breakpoint_deleted);
      observer_attach_breakpoint_modified (mi_breakpoint_modified);
      observer_attach_command_param_changed (mi_command_param_changed);
      observer_attach_memory_changed (mi_memory_changed);
      observer_attach_sync_execution_done (mi_on_sync_execution_done);

      /* The initial inferior is created before this function is
	 called, so we need to report it explicitly.  Use iteration in
	 case future version of GDB creates more than one inferior
	 up-front.  */
      iterate_over_inferiors (report_initial_inferior, mi);
    }

  return mi;
}

static int
mi_interpreter_resume (void *data)
{
  struct mi_interp *mi = data;

  /* As per hack note in mi_interpreter_init, swap in the output
     channels... */
  gdb_setup_readline ();

  /* These overwrite some of the initialization done in
     _intialize_event_loop.  */
  call_readline = gdb_readline2;
  input_handler = mi_execute_command_input_handler;
  async_command_editing_p = 0;
  /* FIXME: This is a total hack for now.  PB's use of the MI
     implicitly relies on a bug in the async support which allows
     asynchronous commands to leak through the commmand loop.  The bug
     involves (but is not limited to) the fact that sync_execution was
     erroneously initialized to 0.  Duplicate by initializing it thus
     here...  */
  sync_execution = 0;

  gdb_stdout = mi->out;
  /* Route error and log output through the MI.  */
  gdb_stderr = mi->err;
  gdb_stdlog = mi->log;
  /* Route target output through the MI.  */
  gdb_stdtarg = mi->targ;
  /* Route target error through the MI as well.  */
  gdb_stdtargerr = mi->targ;

  /* Replace all the hooks that we know about.  There really needs to
     be a better way of doing this... */
  clear_interpreter_hooks ();

  deprecated_show_load_progress = mi_load_progress;

  return 1;
}

static int
mi_interpreter_suspend (void *data)
{
  gdb_disable_readline ();
  return 1;
}

static struct gdb_exception
mi_interpreter_exec (void *data, const char *command)
{
  mi_execute_command_wrapper (command);
  return exception_none;
}

void
mi_cmd_interpreter_exec (char *command, char **argv, int argc)
{
  struct interp *interp_to_use;
  int i;
  char *mi_error_message = NULL;
  struct cleanup *old_chain;

  if (argc < 2)
    error (_("-interpreter-exec: "
	     "Usage: -interpreter-exec interp command"));

  interp_to_use = interp_lookup (argv[0]);
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

  old_chain = make_cleanup (null_cleanup, 0);
  for (i = 1; i < argc; i++)
    {
      struct gdb_exception e = interp_exec (interp_to_use, argv[i]);

      if (e.reason < 0)
	{
	  mi_error_message = xstrdup (e.message);
	  make_cleanup (xfree, mi_error_message);
	  break;
	}
    }

  mi_remove_notify_hooks ();

  if (mi_error_message != NULL)
    error ("%s", mi_error_message);
  do_cleanups (old_chain);
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
  mi_execute_command (cmd, stdin == instream);
}

/* Observer for the synchronous_command_done notification.  */

static void
mi_on_sync_execution_done (void)
{
  /* MI generally prints a prompt after a command, indicating it's
     ready for further input.  However, due to an historical wart, if
     MI async, and a (CLI) synchronous command was issued, then we
     will print the prompt right after printing "^running", even if we
     cannot actually accept any input until the target stops.  See
     mi_on_resume.  However, if the target is async but MI is sync,
     then we need to output the MI prompt now, to replicate gdb's
     behavior when neither the target nor MI are async.  (Note this
     observer is only called by the asynchronous target event handling
     code.)  */
  if (!mi_async_p ())
    {
      fputs_unfiltered ("(gdb) \n", raw_stdout);
      gdb_flush (raw_stdout);
    }
}

/* mi_execute_command_wrapper wrapper suitable for INPUT_HANDLER.  */

static void
mi_execute_command_input_handler (char *cmd)
{
  mi_execute_command_wrapper (cmd);

  /* MI generally prints a prompt after a command, indicating it's
     ready for further input.  However, due to an historical wart, if
     MI is async, and a synchronous command was issued, then we will
     print the prompt right after printing "^running", even if we
     cannot actually accept any input until the target stops.  See
     mi_on_resume.

     If MI is not async, then we print the prompt when the command
     finishes.  If the target is sync, that means output the prompt
     now, as in that case executing a command doesn't return until the
     command is done.  However, if the target is async, we go back to
     the event loop and output the prompt in the
     'synchronous_command_done' observer.  */
  if (!target_is_async_p () || !sync_execution)
    {
      fputs_unfiltered ("(gdb) \n", raw_stdout);
      gdb_flush (raw_stdout);
    }
}

static void
mi_command_loop (void *data)
{
  /* Turn off 8 bit strings in quoted output.  Any character with the
     high bit set is printed using C's octal format.  */
  sevenbit_strings = 1;

  /* Tell the world that we're alive.  */
  fputs_unfiltered ("(gdb) \n", raw_stdout);
  gdb_flush (raw_stdout);

  start_event_loop ();
}

static void
mi_new_thread (struct thread_info *t)
{
  struct mi_interp *mi = top_level_interpreter_data ();
  struct inferior *inf = find_inferior_ptid (t->ptid);

  gdb_assert (inf);

  fprintf_unfiltered (mi->event_channel, 
		      "thread-created,id=\"%d\",group-id=\"i%d\"",
		      t->num, inf->num);
  gdb_flush (mi->event_channel);
}

static void
mi_thread_exit (struct thread_info *t, int silent)
{
  struct mi_interp *mi;
  struct inferior *inf;
  struct cleanup *old_chain;

  if (silent)
    return;

  inf = find_inferior_ptid (t->ptid);

  mi = top_level_interpreter_data ();
  old_chain = make_cleanup_restore_target_terminal ();
  target_terminal_ours ();
  fprintf_unfiltered (mi->event_channel, 
		      "thread-exited,id=\"%d\",group-id=\"i%d\"",
		      t->num, inf->num);
  gdb_flush (mi->event_channel);

  do_cleanups (old_chain);
}

/* Emit notification on changing the state of record.  */

static void
mi_record_changed (struct inferior *inferior, int started)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  fprintf_unfiltered (mi->event_channel,  "record-%s,thread-group=\"i%d\"",
		      started ? "started" : "stopped", inferior->num);

  gdb_flush (mi->event_channel);
}

static void
mi_inferior_added (struct inferior *inf)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  target_terminal_ours ();
  fprintf_unfiltered (mi->event_channel,
		      "thread-group-added,id=\"i%d\"",
		      inf->num);
  gdb_flush (mi->event_channel);
}

static void
mi_inferior_appeared (struct inferior *inf)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  target_terminal_ours ();
  fprintf_unfiltered (mi->event_channel,
		      "thread-group-started,id=\"i%d\",pid=\"%d\"",
		      inf->num, inf->pid);
  gdb_flush (mi->event_channel);
}

static void
mi_inferior_exit (struct inferior *inf)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  target_terminal_ours ();
  if (inf->has_exit_code)
    fprintf_unfiltered (mi->event_channel,
			"thread-group-exited,id=\"i%d\",exit-code=\"%s\"",
			inf->num, int_string (inf->exit_code, 8, 0, 0, 1));
  else
    fprintf_unfiltered (mi->event_channel,
			"thread-group-exited,id=\"i%d\"", inf->num);

  gdb_flush (mi->event_channel);  
}

static void
mi_inferior_removed (struct inferior *inf)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  target_terminal_ours ();
  fprintf_unfiltered (mi->event_channel,
		      "thread-group-removed,id=\"i%d\"",
		      inf->num);
  gdb_flush (mi->event_channel);
}

/* Cleanup that restores a previous current uiout.  */

static void
restore_current_uiout_cleanup (void *arg)
{
  struct ui_out *saved_uiout = arg;

  current_uiout = saved_uiout;
}

/* Return the MI interpreter, if it is active -- either because it's
   the top-level interpreter or the interpreter executing the current
   command.  Returns NULL if the MI interpreter is not being used.  */

static struct interp *
find_mi_interpreter (void)
{
  struct interp *interp;

  interp = top_level_interpreter ();
  if (ui_out_is_mi_like_p (interp_ui_out (interp)))
    return interp;

  interp = command_interp ();
  if (ui_out_is_mi_like_p (interp_ui_out (interp)))
    return interp;

  return NULL;
}

/* Return the MI_INTERP structure of the active MI interpreter.
   Returns NULL if MI is not active.  */

static struct mi_interp *
mi_interp_data (void)
{
  struct interp *interp = find_mi_interpreter ();

  if (interp != NULL)
    return interp_data (interp);
  return NULL;
}

/* Observers for several run control events that print why the
   inferior has stopped to both the the MI event channel and to the MI
   console.  If the MI interpreter is not active, print nothing.  */

/* Observer for the signal_received notification.  */

static void
mi_on_signal_received (enum gdb_signal siggnal)
{
  struct mi_interp *mi = mi_interp_data ();

  if (mi == NULL)
    return;

  print_signal_received_reason (mi->mi_uiout, siggnal);
  print_signal_received_reason (mi->cli_uiout, siggnal);
}

/* Observer for the end_stepping_range notification.  */

static void
mi_on_end_stepping_range (void)
{
  struct mi_interp *mi = mi_interp_data ();

  if (mi == NULL)
    return;

  print_end_stepping_range_reason (mi->mi_uiout);
  print_end_stepping_range_reason (mi->cli_uiout);
}

/* Observer for the signal_exited notification.  */

static void
mi_on_signal_exited (enum gdb_signal siggnal)
{
  struct mi_interp *mi = mi_interp_data ();

  if (mi == NULL)
    return;

  print_signal_exited_reason (mi->mi_uiout, siggnal);
  print_signal_exited_reason (mi->cli_uiout, siggnal);
}

/* Observer for the exited notification.  */

static void
mi_on_exited (int exitstatus)
{
  struct mi_interp *mi = mi_interp_data ();

  if (mi == NULL)
    return;

  print_exited_reason (mi->mi_uiout, exitstatus);
  print_exited_reason (mi->cli_uiout, exitstatus);
}

/* Observer for the no_history notification.  */

static void
mi_on_no_history (void)
{
  struct mi_interp *mi = mi_interp_data ();

  if (mi == NULL)
    return;

  print_no_history_reason (mi->mi_uiout);
  print_no_history_reason (mi->cli_uiout);
}

static void
mi_on_normal_stop (struct bpstats *bs, int print_frame)
{
  /* Since this can be called when CLI command is executing,
     using cli interpreter, be sure to use MI uiout for output,
     not the current one.  */
  struct ui_out *mi_uiout = interp_ui_out (top_level_interpreter ());

  if (print_frame)
    {
      int core;

      if (current_uiout != mi_uiout)
	{
	  /* The normal_stop function has printed frame information
	     into CLI uiout, or some other non-MI uiout.  There's no
	     way we can extract proper fields from random uiout
	     object, so we print the frame again.  In practice, this
	     can only happen when running a CLI command in MI.  */
	  struct ui_out *saved_uiout = current_uiout;
	  struct target_waitstatus last;
	  ptid_t last_ptid;

	  current_uiout = mi_uiout;

	  get_last_target_status (&last_ptid, &last);
	  print_stop_event (&last);

	  current_uiout = saved_uiout;
	}
      /* Otherwise, frame information has already been printed by
	 normal_stop.  */
      else
	{
	  /* Breakpoint hits should always be mirrored to the console.
	     Deciding what to mirror to the console wrt to breakpoints
	     and random stops gets messy real fast.  E.g., say "s"
	     trips on a breakpoint.  We'd clearly want to mirror the
	     event to the console in this case.  But what about more
	     complicated cases like "s&; thread n; s&", and one of
	     those steps spawning a new thread, and that thread
	     hitting a breakpoint?  It's impossible in general to
	     track whether the thread had any relation to the commands
	     that had been executed.  So we just simplify and always
	     mirror breakpoints and random events to the console.

	     Also, CLI execution commands (-interpreter-exec console
	     "next", for example) in async mode have the opposite
	     issue as described in the "then" branch above --
	     normal_stop has already printed frame information to MI
	     uiout, but nothing has printed the same information to
	     the CLI channel.  We should print the source line to the
	     console when stepping or other similar commands, iff the
	     step was started by a console command (but not if it was
	     started with -exec-step or similar).  */
	  struct thread_info *tp = inferior_thread ();

	  if ((!tp->control.stop_step
		  && !tp->control.proceed_to_finish)
	      || (tp->control.command_interp != NULL
		  && tp->control.command_interp != top_level_interpreter ()))
	    {
	      struct mi_interp *mi = top_level_interpreter_data ();
	      struct target_waitstatus last;
	      ptid_t last_ptid;
	      struct cleanup *old_chain;

	      /* Set the current uiout to CLI uiout temporarily.  */
	      old_chain = make_cleanup (restore_current_uiout_cleanup,
					current_uiout);
	      current_uiout = mi->cli_uiout;

	      get_last_target_status (&last_ptid, &last);
	      print_stop_event (&last);

	      do_cleanups (old_chain);
	    }
	}

      ui_out_field_int (mi_uiout, "thread-id",
			pid_to_thread_id (inferior_ptid));
      if (non_stop)
	{
	  struct cleanup *back_to = make_cleanup_ui_out_list_begin_end 
	    (mi_uiout, "stopped-threads");

	  ui_out_field_int (mi_uiout, NULL,
			    pid_to_thread_id (inferior_ptid));
	  do_cleanups (back_to);
	}
      else
	ui_out_field_string (mi_uiout, "stopped-threads", "all");

      core = target_core_of_thread (inferior_ptid);
      if (core != -1)
	ui_out_field_int (mi_uiout, "core", core);
    }
  
  fputs_unfiltered ("*stopped", raw_stdout);
  mi_out_put (mi_uiout, raw_stdout);
  mi_out_rewind (mi_uiout);
  mi_print_timing_maybe ();
  fputs_unfiltered ("\n", raw_stdout);
  gdb_flush (raw_stdout);
}

static void
mi_about_to_proceed (void)
{
  /* Suppress output while calling an inferior function.  */

  if (!ptid_equal (inferior_ptid, null_ptid))
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
  };

/* Emit notification on changing a traceframe.  */

static void
mi_traceframe_changed (int tfnum, int tpnum)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  if (mi_suppress_notification.traceframe)
    return;

  target_terminal_ours ();

  if (tfnum >= 0)
    fprintf_unfiltered (mi->event_channel, "traceframe-changed,"
			"num=\"%d\",tracepoint=\"%d\"\n",
			tfnum, tpnum);
  else
    fprintf_unfiltered (mi->event_channel, "traceframe-changed,end");

  gdb_flush (mi->event_channel);
}

/* Emit notification on creating a trace state variable.  */

static void
mi_tsv_created (const struct trace_state_variable *tsv)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  target_terminal_ours ();

  fprintf_unfiltered (mi->event_channel, "tsv-created,"
		      "name=\"%s\",initial=\"%s\"\n",
		      tsv->name, plongest (tsv->initial_value));

  gdb_flush (mi->event_channel);
}

/* Emit notification on deleting a trace state variable.  */

static void
mi_tsv_deleted (const struct trace_state_variable *tsv)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  target_terminal_ours ();

  if (tsv != NULL)
    fprintf_unfiltered (mi->event_channel, "tsv-deleted,"
			"name=\"%s\"\n", tsv->name);
  else
    fprintf_unfiltered (mi->event_channel, "tsv-deleted\n");

  gdb_flush (mi->event_channel);
}

/* Emit notification on modifying a trace state variable.  */

static void
mi_tsv_modified (const struct trace_state_variable *tsv)
{
  struct mi_interp *mi = top_level_interpreter_data ();
  struct ui_out *mi_uiout = interp_ui_out (top_level_interpreter ());

  target_terminal_ours ();

  fprintf_unfiltered (mi->event_channel,
		      "tsv-modified");

  ui_out_redirect (mi_uiout, mi->event_channel);

  ui_out_field_string (mi_uiout, "name", tsv->name);
  ui_out_field_string (mi_uiout, "initial",
		       plongest (tsv->initial_value));
  if (tsv->value_known)
    ui_out_field_string (mi_uiout, "current", plongest (tsv->value));

  ui_out_redirect (mi_uiout, NULL);

  gdb_flush (mi->event_channel);
}

/* Emit notification about a created breakpoint.  */

static void
mi_breakpoint_created (struct breakpoint *b)
{
  struct mi_interp *mi = top_level_interpreter_data ();
  struct ui_out *mi_uiout = interp_ui_out (top_level_interpreter ());

  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  target_terminal_ours ();
  fprintf_unfiltered (mi->event_channel,
		      "breakpoint-created");
  /* We want the output from gdb_breakpoint_query to go to
     mi->event_channel.  One approach would be to just call
     gdb_breakpoint_query, and then use mi_out_put to send the current
     content of mi_outout into mi->event_channel.  However, that will
     break if anything is output to mi_uiout prior to calling the
     breakpoint_created notifications.  So, we use
     ui_out_redirect.  */
  ui_out_redirect (mi_uiout, mi->event_channel);
  TRY
    {
      gdb_breakpoint_query (mi_uiout, b->number, NULL);
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
    }
  END_CATCH

  ui_out_redirect (mi_uiout, NULL);

  gdb_flush (mi->event_channel);
}

/* Emit notification about deleted breakpoint.  */

static void
mi_breakpoint_deleted (struct breakpoint *b)
{
  struct mi_interp *mi = top_level_interpreter_data ();

  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  target_terminal_ours ();

  fprintf_unfiltered (mi->event_channel, "breakpoint-deleted,id=\"%d\"",
		      b->number);

  gdb_flush (mi->event_channel);
}

/* Emit notification about modified breakpoint.  */

static void
mi_breakpoint_modified (struct breakpoint *b)
{
  struct mi_interp *mi = top_level_interpreter_data ();
  struct ui_out *mi_uiout = interp_ui_out (top_level_interpreter ());

  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  target_terminal_ours ();
  fprintf_unfiltered (mi->event_channel,
		      "breakpoint-modified");
  /* We want the output from gdb_breakpoint_query to go to
     mi->event_channel.  One approach would be to just call
     gdb_breakpoint_query, and then use mi_out_put to send the current
     content of mi_outout into mi->event_channel.  However, that will
     break if anything is output to mi_uiout prior to calling the
     breakpoint_created notifications.  So, we use
     ui_out_redirect.  */
  ui_out_redirect (mi_uiout, mi->event_channel);
  TRY
    {
      gdb_breakpoint_query (mi_uiout, b->number, NULL);
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
    }
  END_CATCH

  ui_out_redirect (mi_uiout, NULL);

  gdb_flush (mi->event_channel);
}

static int
mi_output_running_pid (struct thread_info *info, void *arg)
{
  ptid_t *ptid = arg;

  if (ptid_get_pid (*ptid) == ptid_get_pid (info->ptid))
    fprintf_unfiltered (raw_stdout,
			"*running,thread-id=\"%d\"\n",
			info->num);

  return 0;
}

static int
mi_inferior_count (struct inferior *inf, void *arg)
{
  if (inf->pid != 0)
    {
      int *count_p = arg;
      (*count_p)++;
    }

  return 0;
}

static void
mi_on_resume (ptid_t ptid)
{
  struct thread_info *tp = NULL;

  if (ptid_equal (ptid, minus_one_ptid) || ptid_is_pid (ptid))
    tp = inferior_thread ();
  else
    tp = find_thread_ptid (ptid);

  /* Suppress output while calling an inferior function.  */
  if (tp->control.in_infcall)
    return;

  /* To cater for older frontends, emit ^running, but do it only once
     per each command.  We do it here, since at this point we know
     that the target was successfully resumed, and in non-async mode,
     we won't return back to MI interpreter code until the target
     is done running, so delaying the output of "^running" until then
     will make it impossible for frontend to know what's going on.

     In future (MI3), we'll be outputting "^done" here.  */
  if (!running_result_record_printed && mi_proceeded)
    {
      fprintf_unfiltered (raw_stdout, "%s^running\n",
			  current_token ? current_token : "");
    }

  if (ptid_get_pid (ptid) == -1)
    fprintf_unfiltered (raw_stdout, "*running,thread-id=\"all\"\n");
  else if (ptid_is_pid (ptid))
    {
      int count = 0;

      /* Backwards compatibility.  If there's only one inferior,
	 output "all", otherwise, output each resumed thread
	 individually.  */
      iterate_over_inferiors (mi_inferior_count, &count);

      if (count == 1)
	fprintf_unfiltered (raw_stdout, "*running,thread-id=\"all\"\n");
      else
	iterate_over_threads (mi_output_running_pid, &ptid);
    }
  else
    {
      struct thread_info *ti = find_thread_ptid (ptid);

      gdb_assert (ti);
      fprintf_unfiltered (raw_stdout, "*running,thread-id=\"%d\"\n", ti->num);
    }

  if (!running_result_record_printed && mi_proceeded)
    {
      running_result_record_printed = 1;
      /* This is what gdb used to do historically -- printing prompt even if
	 it cannot actually accept any input.  This will be surely removed
	 for MI3, and may be removed even earlier.  SYNC_EXECUTION is
	 checked here because we only need to emit a prompt if a
	 synchronous command was issued when the target is async.  */
      if (!target_is_async_p () || sync_execution)
	fputs_unfiltered ("(gdb) \n", raw_stdout);
    }
  gdb_flush (raw_stdout);
}

static void
mi_solib_loaded (struct so_list *solib)
{
  struct mi_interp *mi = top_level_interpreter_data ();
  struct ui_out *uiout = interp_ui_out (top_level_interpreter ());

  target_terminal_ours ();

  fprintf_unfiltered (mi->event_channel, "library-loaded");

  ui_out_redirect (uiout, mi->event_channel);

  ui_out_field_string (uiout, "id", solib->so_original_name);
  ui_out_field_string (uiout, "target-name", solib->so_original_name);
  ui_out_field_string (uiout, "host-name", solib->so_name);
  ui_out_field_int (uiout, "symbols-loaded", solib->symbols_loaded);
  if (!gdbarch_has_global_solist (target_gdbarch ()))
    {
      ui_out_field_fmt (uiout, "thread-group", "i%d", current_inferior ()->num);
    }

  ui_out_redirect (uiout, NULL);

  gdb_flush (mi->event_channel);
}

static void
mi_solib_unloaded (struct so_list *solib)
{
  struct mi_interp *mi = top_level_interpreter_data ();
  struct ui_out *uiout = interp_ui_out (top_level_interpreter ());

  target_terminal_ours ();

  fprintf_unfiltered (mi->event_channel, "library-unloaded");

  ui_out_redirect (uiout, mi->event_channel);

  ui_out_field_string (uiout, "id", solib->so_original_name);
  ui_out_field_string (uiout, "target-name", solib->so_original_name);
  ui_out_field_string (uiout, "host-name", solib->so_name);
  if (!gdbarch_has_global_solist (target_gdbarch ()))
    {
      ui_out_field_fmt (uiout, "thread-group", "i%d", current_inferior ()->num);
    }

  ui_out_redirect (uiout, NULL);

  gdb_flush (mi->event_channel);
}

/* Emit notification about the command parameter change.  */

static void
mi_command_param_changed (const char *param, const char *value)
{
  struct mi_interp *mi = top_level_interpreter_data ();
  struct ui_out *mi_uiout = interp_ui_out (top_level_interpreter ());

  if (mi_suppress_notification.cmd_param_changed)
    return;

  target_terminal_ours ();

  fprintf_unfiltered (mi->event_channel,
		      "cmd-param-changed");

  ui_out_redirect (mi_uiout, mi->event_channel);

  ui_out_field_string (mi_uiout, "param", param);
  ui_out_field_string (mi_uiout, "value", value);

  ui_out_redirect (mi_uiout, NULL);

  gdb_flush (mi->event_channel);
}

/* Emit notification about the target memory change.  */

static void
mi_memory_changed (struct inferior *inferior, CORE_ADDR memaddr,
		   ssize_t len, const bfd_byte *myaddr)
{
  struct mi_interp *mi = top_level_interpreter_data ();
  struct ui_out *mi_uiout = interp_ui_out (top_level_interpreter ());
  struct obj_section *sec;

  if (mi_suppress_notification.memory)
    return;

  target_terminal_ours ();

  fprintf_unfiltered (mi->event_channel,
		      "memory-changed");

  ui_out_redirect (mi_uiout, mi->event_channel);

  ui_out_field_fmt (mi_uiout, "thread-group", "i%d", inferior->num);
  ui_out_field_core_addr (mi_uiout, "addr", target_gdbarch (), memaddr);
  ui_out_field_fmt (mi_uiout, "len", "%s", hex_string (len));

  /* Append 'type=code' into notification if MEMADDR falls in the range of
     sections contain code.  */
  sec = find_pc_section (memaddr);
  if (sec != NULL && sec->objfile != NULL)
    {
      flagword flags = bfd_get_section_flags (sec->objfile->obfd,
					      sec->the_bfd_section);

      if (flags & SEC_CODE)
	ui_out_field_string (mi_uiout, "type", "code");
    }

  ui_out_redirect (mi_uiout, NULL);

  gdb_flush (mi->event_channel);
}

static int
report_initial_inferior (struct inferior *inf, void *closure)
{
  /* This function is called from mi_intepreter_init, and since
     mi_inferior_added assumes that inferior is fully initialized
     and top_level_interpreter_data is set, we cannot call
     it here.  */
  struct mi_interp *mi = closure;

  target_terminal_ours ();
  fprintf_unfiltered (mi->event_channel,
		      "thread-group-added,id=\"i%d\"",
		      inf->num);
  gdb_flush (mi->event_channel);
  return 0;
}

static struct ui_out *
mi_ui_out (struct interp *interp)
{
  struct mi_interp *mi = interp_data (interp);

  return mi->mi_uiout;
}

/* Save the original value of raw_stdout here when logging, so we can
   restore correctly when done.  */

static struct ui_file *saved_raw_stdout;

/* Do MI-specific logging actions; save raw_stdout, and change all
   the consoles to use the supplied ui-file(s).  */

static int
mi_set_logging (struct interp *interp, int start_log,
		struct ui_file *out, struct ui_file *logfile)
{
  struct mi_interp *mi = interp_data (interp);

  if (!mi)
    return 0;

  if (start_log)
    {
      /* The tee created already is based on gdb_stdout, which for MI
	 is a console and so we end up in an infinite loop of console
	 writing to ui_file writing to console etc.  So discard the
	 existing tee (it hasn't been used yet, and MI won't ever use
	 it), and create one based on raw_stdout instead.  */
      if (logfile)
	{
	  ui_file_delete (out);
	  out = tee_file_new (raw_stdout, 0, logfile, 0);
	}

      saved_raw_stdout = raw_stdout;
      raw_stdout = out;
    }
  else
    {
      raw_stdout = saved_raw_stdout;
      saved_raw_stdout = NULL;
    }
  
  mi_console_set_raw (mi->out, raw_stdout);
  mi_console_set_raw (mi->err, raw_stdout);
  mi_console_set_raw (mi->log, raw_stdout);
  mi_console_set_raw (mi->targ, raw_stdout);
  mi_console_set_raw (mi->event_channel, raw_stdout);

  return 1;
}

extern initialize_file_ftype _initialize_mi_interp; /* -Wmissing-prototypes */

void
_initialize_mi_interp (void)
{
  static const struct interp_procs procs =
    {
      mi_interpreter_init,	/* init_proc */
      mi_interpreter_resume,	/* resume_proc */
      mi_interpreter_suspend,	/* suspend_proc */
      mi_interpreter_exec,	/* exec_proc */
      mi_ui_out, 		/* ui_out_proc */
      mi_set_logging,		/* set_logging_proc */
      mi_command_loop		/* command_loop_proc */
    };

  /* The various interpreter levels.  */
  interp_add (interp_new (INTERP_MI1, &procs));
  interp_add (interp_new (INTERP_MI2, &procs));
  interp_add (interp_new (INTERP_MI3, &procs));
  interp_add (interp_new (INTERP_MI, &procs));
}
