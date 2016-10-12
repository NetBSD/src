/* CLI Definitions for GDB, the GNU debugger.

   Copyright (C) 2002-2016 Free Software Foundation, Inc.

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
#include "cli-interp.h"
#include "interps.h"
#include "event-top.h"
#include "ui-out.h"
#include "cli-out.h"
#include "top.h"		/* for "execute_command" */
#include "event-top.h"
#include "infrun.h"
#include "observer.h"
#include "gdbthread.h"
#include "thread-fsm.h"

/* The console interpreter.  */
struct cli_interp
{
  /* The ui_out for the console interpreter.  */
  struct ui_out *cli_uiout;
};

/* Suppress notification struct.  */
struct cli_suppress_notification cli_suppress_notification =
  {
    0   /* user_selected_context_changed */
  };

/* Returns the INTERP's data cast as cli_interp if INTERP is a CLI,
   and returns NULL otherwise.  */

static struct cli_interp *
as_cli_interp (struct interp *interp)
{
  if (strcmp (interp_name (interp), INTERP_CONSOLE) == 0)
    return (struct cli_interp *) interp_data (interp);
  return NULL;
}

/* Longjmp-safe wrapper for "execute_command".  */
static struct gdb_exception safe_execute_command (struct ui_out *uiout,
						  char *command, 
						  int from_tty);

/* See cli-interp.h.

   Breakpoint hits should always be mirrored to a console.  Deciding
   what to mirror to a console wrt to breakpoints and random stops
   gets messy real fast.  E.g., say "s" trips on a breakpoint.  We'd
   clearly want to mirror the event to the console in this case.  But
   what about more complicated cases like "s&; thread n; s&", and one
   of those steps spawning a new thread, and that thread hitting a
   breakpoint?  It's impossible in general to track whether the thread
   had any relation to the commands that had been executed.  So we
   just simplify and always mirror breakpoints and random events to
   all consoles.

   OTOH, we should print the source line to the console when stepping
   or other similar commands, iff the step was started by that console
   (or in MI's case, by a console command), but not if it was started
   with MI's -exec-step or similar.  */

int
should_print_stop_to_console (struct interp *console_interp,
			      struct thread_info *tp)
{
  if ((bpstat_what (tp->control.stop_bpstat).main_action
       == BPSTAT_WHAT_STOP_NOISY)
      || tp->thread_fsm == NULL
      || tp->thread_fsm->command_interp == console_interp
      || !thread_fsm_finished_p (tp->thread_fsm))
    return 1;
  return 0;
}

/* Observers for several run control events.  If the interpreter is
   quiet (i.e., another interpreter is being run with
   interpreter-exec), print nothing.  */

/* Observer for the normal_stop notification.  */

static void
cli_on_normal_stop (struct bpstats *bs, int print_frame)
{
  struct switch_thru_all_uis state;

  if (!print_frame)
    return;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct interp *interp = top_level_interpreter ();
      struct cli_interp *cli = as_cli_interp (interp);
      struct thread_info *thread;

      if (cli == NULL)
	continue;

      thread = inferior_thread ();
      if (should_print_stop_to_console (interp, thread))
	print_stop_event (cli->cli_uiout);
    }
}

/* Observer for the signal_received notification.  */

static void
cli_on_signal_received (enum gdb_signal siggnal)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct cli_interp *cli = as_cli_interp (top_level_interpreter ());

      if (cli == NULL)
	continue;

      print_signal_received_reason (cli->cli_uiout, siggnal);
    }
}

/* Observer for the end_stepping_range notification.  */

static void
cli_on_end_stepping_range (void)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct cli_interp *cli = as_cli_interp (top_level_interpreter ());

      if (cli == NULL)
	continue;

      print_end_stepping_range_reason (cli->cli_uiout);
    }
}

/* Observer for the signalled notification.  */

static void
cli_on_signal_exited (enum gdb_signal siggnal)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct cli_interp *cli = as_cli_interp (top_level_interpreter ());

      if (cli == NULL)
	continue;

      print_signal_exited_reason (cli->cli_uiout, siggnal);
    }
}

/* Observer for the exited notification.  */

static void
cli_on_exited (int exitstatus)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct cli_interp *cli = as_cli_interp (top_level_interpreter ());

      if (cli == NULL)
	continue;

      print_exited_reason (cli->cli_uiout, exitstatus);
    }
}

/* Observer for the no_history notification.  */

static void
cli_on_no_history (void)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct cli_interp *cli = as_cli_interp (top_level_interpreter ());

      if (cli == NULL)
	continue;

      print_no_history_reason (cli->cli_uiout);
    }
}

/* Observer for the sync_execution_done notification.  */

static void
cli_on_sync_execution_done (void)
{
  struct cli_interp *cli = as_cli_interp (top_level_interpreter ());

  if (cli == NULL)
    return;

  display_gdb_prompt (NULL);
}

/* Observer for the command_error notification.  */

static void
cli_on_command_error (void)
{
  struct cli_interp *cli = as_cli_interp (top_level_interpreter ());

  if (cli == NULL)
    return;

  display_gdb_prompt (NULL);
}

/* Observer for the user_selected_context_changed notification.  */

static void
cli_on_user_selected_context_changed (user_selected_what selection)
{
  struct switch_thru_all_uis state;
  struct thread_info *tp;

  /* This event is suppressed.  */
  if (cli_suppress_notification.user_selected_context)
    return;

  tp = find_thread_ptid (inferior_ptid);

  SWITCH_THRU_ALL_UIS (state)
    {
      struct cli_interp *cli = as_cli_interp (top_level_interpreter ());

      if (cli == NULL)
	continue;

      if (selection & USER_SELECTED_INFERIOR)
	print_selected_inferior (cli->cli_uiout);

      if (tp != NULL
	  && ((selection & (USER_SELECTED_THREAD | USER_SELECTED_FRAME))))
	print_selected_thread_frame (cli->cli_uiout, selection);
    }
}

/* pre_command_loop implementation.  */

void
cli_interpreter_pre_command_loop (struct interp *self)
{
  display_gdb_prompt (0);
}

/* These implement the cli out interpreter: */

static void *
cli_interpreter_init (struct interp *self, int top_level)
{
  return interp_data (self);
}

static int
cli_interpreter_resume (void *data)
{
  struct ui *ui = current_ui;
  struct cli_interp *cli = (struct cli_interp *) data;
  struct ui_file *stream;

  /*sync_execution = 1; */

  /* gdb_setup_readline will change gdb_stdout.  If the CLI was
     previously writing to gdb_stdout, then set it to the new
     gdb_stdout afterwards.  */

  stream = cli_out_set_stream (cli->cli_uiout, gdb_stdout);
  if (stream != gdb_stdout)
    {
      cli_out_set_stream (cli->cli_uiout, stream);
      stream = NULL;
    }

  gdb_setup_readline (1);

  ui->input_handler = command_line_handler;

  if (stream != NULL)
    cli_out_set_stream (cli->cli_uiout, gdb_stdout);

  return 1;
}

static int
cli_interpreter_suspend (void *data)
{
  gdb_disable_readline ();
  return 1;
}

static struct gdb_exception
cli_interpreter_exec (void *data, const char *command_str)
{
  struct cli_interp *cli = (struct cli_interp *) data;
  struct ui_file *old_stream;
  struct gdb_exception result;

  /* FIXME: cagney/2003-02-01: Need to const char *propogate
     safe_execute_command.  */
  char *str = (char *) alloca (strlen (command_str) + 1);
  strcpy (str, command_str);

  /* gdb_stdout could change between the time cli_uiout was
     initialized and now.  Since we're probably using a different
     interpreter which has a new ui_file for gdb_stdout, use that one
     instead of the default.

     It is important that it gets reset everytime, since the user
     could set gdb to use a different interpreter.  */
  old_stream = cli_out_set_stream (cli->cli_uiout, gdb_stdout);
  result = safe_execute_command (cli->cli_uiout, str, 1);
  cli_out_set_stream (cli->cli_uiout, old_stream);
  return result;
}

int
cli_interpreter_supports_command_editing (struct interp *interp)
{
  return 1;
}

static struct gdb_exception
safe_execute_command (struct ui_out *command_uiout, char *command, int from_tty)
{
  struct gdb_exception e = exception_none;
  struct ui_out *saved_uiout;

  /* Save and override the global ``struct ui_out'' builder.  */
  saved_uiout = current_uiout;
  current_uiout = command_uiout;

  TRY
    {
      execute_command (command, from_tty);
    }
  CATCH (exception, RETURN_MASK_ALL)
    {
      e = exception;
    }
  END_CATCH

  /* Restore the global builder.  */
  current_uiout = saved_uiout;

  /* FIXME: cagney/2005-01-13: This shouldn't be needed.  Instead the
     caller should print the exception.  */
  exception_print (gdb_stderr, e);
  return e;
}

static struct ui_out *
cli_ui_out (struct interp *self)
{
  struct cli_interp *cli = (struct cli_interp *) interp_data (self);

  return cli->cli_uiout;
}

/* The CLI interpreter's vtable.  */

static const struct interp_procs cli_interp_procs = {
  cli_interpreter_init,		/* init_proc */
  cli_interpreter_resume,	/* resume_proc */
  cli_interpreter_suspend,	/* suspend_proc */
  cli_interpreter_exec,		/* exec_proc */
  cli_ui_out,			/* ui_out_proc */
  NULL,                       	/* set_logging_proc */
  cli_interpreter_pre_command_loop, /* pre_command_loop_proc */
  cli_interpreter_supports_command_editing, /* supports_command_editing_proc */
};

/* Factory for CLI interpreters.  */

static struct interp *
cli_interp_factory (const char *name)
{
  struct cli_interp *cli = XNEW (struct cli_interp);

  /* Create a default uiout builder for the CLI.  */
  cli->cli_uiout = cli_out_new (gdb_stdout);

  return interp_new (name, &cli_interp_procs, cli);
}

/* Standard gdb initialization hook.  */
extern initialize_file_ftype _initialize_cli_interp; /* -Wmissing-prototypes */

void
_initialize_cli_interp (void)
{
  interp_factory_register (INTERP_CONSOLE, cli_interp_factory);

  /* If changing this, remember to update tui-interp.c as well.  */
  observer_attach_normal_stop (cli_on_normal_stop);
  observer_attach_end_stepping_range (cli_on_end_stepping_range);
  observer_attach_signal_received (cli_on_signal_received);
  observer_attach_signal_exited (cli_on_signal_exited);
  observer_attach_exited (cli_on_exited);
  observer_attach_no_history (cli_on_no_history);
  observer_attach_sync_execution_done (cli_on_sync_execution_done);
  observer_attach_command_error (cli_on_command_error);
  observer_attach_user_selected_context_changed
    (cli_on_user_selected_context_changed);
}
