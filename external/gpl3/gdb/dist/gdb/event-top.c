/* Top level stuff for GDB, the GNU debugger.

   Copyright (C) 1999-2016 Free Software Foundation, Inc.

   Written by Elena Zannoni <ezannoni@cygnus.com> of Cygnus Solutions.

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
#include "top.h"
#include "inferior.h"
#include "infrun.h"
#include "target.h"
#include "terminal.h"		/* for job_control */
#include "event-loop.h"
#include "event-top.h"
#include "interps.h"
#include <signal.h>
#include "cli/cli-script.h"     /* for reset_command_nest_depth */
#include "main.h"
#include "gdbthread.h"
#include "observer.h"
#include "continuations.h"
#include "gdbcmd.h"		/* for dont_repeat() */
#include "annotate.h"
#include "maint.h"
#include "buffer.h"
#include "ser-event.h"
#include "gdb_select.h"

/* readline include files.  */
#include "readline/readline.h"
#include "readline/history.h"

/* readline defines this.  */
#undef savestring

static char *top_level_prompt (void);

/* Signal handlers.  */
#ifdef SIGQUIT
static void handle_sigquit (int sig);
#endif
#ifdef SIGHUP
static void handle_sighup (int sig);
#endif
static void handle_sigfpe (int sig);

/* Functions to be invoked by the event loop in response to
   signals.  */
#if defined (SIGQUIT) || defined (SIGHUP)
static void async_do_nothing (gdb_client_data);
#endif
#ifdef SIGHUP
static void async_disconnect (gdb_client_data);
#endif
static void async_float_handler (gdb_client_data);
#ifdef STOP_SIGNAL
static void async_stop_sig (gdb_client_data);
#endif
static void async_sigterm_handler (gdb_client_data arg);

/* Instead of invoking (and waiting for) readline to read the command
   line and pass it back for processing, we use readline's alternate
   interface, via callback functions, so that the event loop can react
   to other event sources while we wait for input.  */

/* Important variables for the event loop.  */

/* This is used to determine if GDB is using the readline library or
   its own simplified form of readline.  It is used by the asynchronous
   form of the set editing command.
   ezannoni: as of 1999-04-29 I expect that this
   variable will not be used after gdb is changed to use the event
   loop as default engine, and event-top.c is merged into top.c.  */
int set_editing_cmd_var;

/* This is used to display the notification of the completion of an
   asynchronous execution command.  */
int exec_done_display_p = 0;

/* Used by the stdin event handler to compensate for missed stdin events.
   Setting this to a non-zero value inside an stdin callback makes the callback
   run again.  */
int call_stdin_event_handler_again_p;

/* Signal handling variables.  */
/* Each of these is a pointer to a function that the event loop will
   invoke if the corresponding signal has received.  The real signal
   handlers mark these functions as ready to be executed and the event
   loop, in a later iteration, calls them.  See the function
   invoke_async_signal_handler.  */
static struct async_signal_handler *sigint_token;
#ifdef SIGHUP
static struct async_signal_handler *sighup_token;
#endif
#ifdef SIGQUIT
static struct async_signal_handler *sigquit_token;
#endif
static struct async_signal_handler *sigfpe_token;
#ifdef STOP_SIGNAL
static struct async_signal_handler *sigtstp_token;
#endif
static struct async_signal_handler *async_sigterm_token;

/* This hook is called by gdb_rl_callback_read_char_wrapper after each
   character is processed.  */
void (*after_char_processing_hook) (void);


/* Wrapper function for calling into the readline library.  This takes
   care of a couple things:

   - The event loop expects the callback function to have a parameter,
     while readline expects none.

   - Propagation of GDB exceptions/errors thrown from INPUT_HANDLER
     across readline requires special handling.

   On the exceptions issue:

   DWARF-based unwinding cannot cross code built without -fexceptions.
   Any exception that tries to propagate through such code will fail
   and the result is a call to std::terminate.  While some ABIs, such
   as x86-64, require all code to be built with exception tables,
   others don't.

   This is a problem when GDB calls some non-EH-aware C library code,
   that calls into GDB again through a callback, and that GDB callback
   code throws a C++ exception.  Turns out this is exactly what
   happens with GDB's readline callback.

   In such cases, we must catch and save any C++ exception that might
   be thrown from the GDB callback before returning to the
   non-EH-aware code.  When the non-EH-aware function itself returns
   back to GDB, we then rethrow the original C++ exception.

   In the readline case however, the right thing to do is to longjmp
   out of the callback, rather than do a normal return -- there's no
   way for the callback to return to readline an indication that an
   error happened, so a normal return would have rl_callback_read_char
   potentially continue processing further input, redisplay the
   prompt, etc.  Instead of raw setjmp/longjmp however, we use our
   sjlj-based TRY/CATCH mechanism, which knows to handle multiple
   levels of active setjmp/longjmp frames, needed in order to handle
   the readline callback recursing, as happens with e.g., secondary
   prompts / queries, through gdb_readline_wrapper.  */

static void
gdb_rl_callback_read_char_wrapper (gdb_client_data client_data)
{
  struct gdb_exception gdb_expt = exception_none;

  /* C++ exceptions can't normally be thrown across readline (unless
     it is built with -fexceptions, but it won't by default on many
     ABIs).  So we instead wrap the readline call with a sjlj-based
     TRY/CATCH, and rethrow the GDB exception once back in GDB.  */
  TRY_SJLJ
    {
      rl_callback_read_char ();
      if (after_char_processing_hook)
	(*after_char_processing_hook) ();
    }
  CATCH_SJLJ (ex, RETURN_MASK_ALL)
    {
      gdb_expt = ex;
    }
  END_CATCH_SJLJ

  /* Rethrow using the normal EH mechanism.  */
  if (gdb_expt.reason < 0)
    throw_exception (gdb_expt);
}

/* GDB's readline callback handler.  Calls the current INPUT_HANDLER,
   and propagates GDB exceptions/errors thrown from INPUT_HANDLER back
   across readline.  See gdb_rl_callback_read_char_wrapper.  */

static void
gdb_rl_callback_handler (char *rl)
{
  struct gdb_exception gdb_rl_expt = exception_none;
  struct ui *ui = current_ui;

  TRY
    {
      ui->input_handler (rl);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      gdb_rl_expt = ex;
    }
  END_CATCH

  /* If we caught a GDB exception, longjmp out of the readline
     callback.  There's no other way for the callback to signal to
     readline that an error happened.  A normal return would have
     readline potentially continue processing further input, redisplay
     the prompt, etc.  (This is what GDB historically did when it was
     a C program.)  Note that since we're long jumping, local variable
     dtors are NOT run automatically.  */
  if (gdb_rl_expt.reason < 0)
    throw_exception_sjlj (gdb_rl_expt);
}

/* Change the function to be invoked every time there is a character
   ready on stdin.  This is used when the user sets the editing off,
   therefore bypassing readline, and letting gdb handle the input
   itself, via gdb_readline_no_editing_callback.  Also it is used in
   the opposite case in which the user sets editing on again, by
   restoring readline handling of the input.

   NOTE: this operates on input_fd, not instream.  If we are reading
   commands from a file, instream will point to the file.  However, we
   always read commands from a file with editing off.  This means that
   the 'set editing on/off' will have effect only on the interactive
   session.  */

void
change_line_handler (int editing)
{
  struct ui *ui = current_ui;

  /* We can only have one instance of readline, so we only allow
     editing on the main UI.  */
  if (ui != main_ui)
    return;

  /* Don't try enabling editing if the interpreter doesn't support it
     (e.g., MI).  */
  if (!interp_supports_command_editing (top_level_interpreter ())
      || !interp_supports_command_editing (command_interp ()))
    return;

  if (editing)
    {
      gdb_assert (ui == main_ui);

      /* Turn on editing by using readline.  */
      ui->call_readline = gdb_rl_callback_read_char_wrapper;
    }
  else
    {
      /* Turn off editing by using gdb_readline_no_editing_callback.  */
      if (ui->command_editing)
	gdb_rl_callback_handler_remove ();
      ui->call_readline = gdb_readline_no_editing_callback;
    }
  ui->command_editing = editing;
}

/* The functions below are wrappers for rl_callback_handler_remove and
   rl_callback_handler_install that keep track of whether the callback
   handler is installed in readline.  This is necessary because after
   handling a target event of a background execution command, we may
   need to reinstall the callback handler if it was removed due to a
   secondary prompt.  See gdb_readline_wrapper_line.  We don't
   unconditionally install the handler for every target event because
   that also clears the line buffer, thus installing it while the user
   is typing would lose input.  */

/* Whether we've registered a callback handler with readline.  */
static int callback_handler_installed;

/* See event-top.h, and above.  */

void
gdb_rl_callback_handler_remove (void)
{
  gdb_assert (current_ui == main_ui);

  rl_callback_handler_remove ();
  callback_handler_installed = 0;
}

/* See event-top.h, and above.  Note this wrapper doesn't have an
   actual callback parameter because we always install
   INPUT_HANDLER.  */

void
gdb_rl_callback_handler_install (const char *prompt)
{
  gdb_assert (current_ui == main_ui);

  /* Calling rl_callback_handler_install resets readline's input
     buffer.  Calling this when we were already processing input
     therefore loses input.  */
  gdb_assert (!callback_handler_installed);

  rl_callback_handler_install (prompt, gdb_rl_callback_handler);
  callback_handler_installed = 1;
}

/* See event-top.h, and above.  */

void
gdb_rl_callback_handler_reinstall (void)
{
  gdb_assert (current_ui == main_ui);

  if (!callback_handler_installed)
    {
      /* Passing NULL as prompt argument tells readline to not display
	 a prompt.  */
      gdb_rl_callback_handler_install (NULL);
    }
}

/* Displays the prompt.  If the argument NEW_PROMPT is NULL, the
   prompt that is displayed is the current top level prompt.
   Otherwise, it displays whatever NEW_PROMPT is as a local/secondary
   prompt.

   This is used after each gdb command has completed, and in the
   following cases:

   1. When the user enters a command line which is ended by '\'
   indicating that the command will continue on the next line.  In
   that case the prompt that is displayed is the empty string.

   2. When the user is entering 'commands' for a breakpoint, or
   actions for a tracepoint.  In this case the prompt will be '>'

   3. On prompting for pagination.  */

void
display_gdb_prompt (const char *new_prompt)
{
  char *actual_gdb_prompt = NULL;
  struct cleanup *old_chain;

  annotate_display_prompt ();

  /* Reset the nesting depth used when trace-commands is set.  */
  reset_command_nest_depth ();

  old_chain = make_cleanup (free_current_contents, &actual_gdb_prompt);

  /* Do not call the python hook on an explicit prompt change as
     passed to this function, as this forms a secondary/local prompt,
     IE, displayed but not set.  */
  if (! new_prompt)
    {
      struct ui *ui = current_ui;

      if (ui->prompt_state == PROMPTED)
	internal_error (__FILE__, __LINE__, _("double prompt"));
      else if (ui->prompt_state == PROMPT_BLOCKED)
	{
	  /* This is to trick readline into not trying to display the
	     prompt.  Even though we display the prompt using this
	     function, readline still tries to do its own display if
	     we don't call rl_callback_handler_install and
	     rl_callback_handler_remove (which readline detects
	     because a global variable is not set).  If readline did
	     that, it could mess up gdb signal handlers for SIGINT.
	     Readline assumes that between calls to rl_set_signals and
	     rl_clear_signals gdb doesn't do anything with the signal
	     handlers.  Well, that's not the case, because when the
	     target executes we change the SIGINT signal handler.  If
	     we allowed readline to display the prompt, the signal
	     handler change would happen exactly between the calls to
	     the above two functions.  Calling
	     rl_callback_handler_remove(), does the job.  */

	  if (current_ui->command_editing)
	    gdb_rl_callback_handler_remove ();
	  do_cleanups (old_chain);
	  return;
	}
      else if (ui->prompt_state == PROMPT_NEEDED)
	{
	  /* Display the top level prompt.  */
	  actual_gdb_prompt = top_level_prompt ();
	  ui->prompt_state = PROMPTED;
	}
    }
  else
    actual_gdb_prompt = xstrdup (new_prompt);

  if (current_ui->command_editing)
    {
      gdb_rl_callback_handler_remove ();
      gdb_rl_callback_handler_install (actual_gdb_prompt);
    }
  /* new_prompt at this point can be the top of the stack or the one
     passed in.  It can't be NULL.  */
  else
    {
      /* Don't use a _filtered function here.  It causes the assumed
         character position to be off, since the newline we read from
         the user is not accounted for.  */
      fputs_unfiltered (actual_gdb_prompt, gdb_stdout);
      gdb_flush (gdb_stdout);
    }

  do_cleanups (old_chain);
}

/* Return the top level prompt, as specified by "set prompt", possibly
   overriden by the python gdb.prompt_hook hook, and then composed
   with the prompt prefix and suffix (annotations).  The caller is
   responsible for freeing the returned string.  */

static char *
top_level_prompt (void)
{
  char *prompt;

  /* Give observers a chance of changing the prompt.  E.g., the python
     `gdb.prompt_hook' is installed as an observer.  */
  observer_notify_before_prompt (get_prompt ());

  prompt = get_prompt ();

  if (annotation_level >= 2)
    {
      /* Prefix needs to have new line at end.  */
      const char prefix[] = "\n\032\032pre-prompt\n";

      /* Suffix needs to have a new line at end and \032 \032 at
	 beginning.  */
      const char suffix[] = "\n\032\032prompt\n";

      return concat (prefix, prompt, suffix, (char *) NULL);
    }

  return xstrdup (prompt);
}

/* See top.h.  */

struct ui *main_ui;
struct ui *current_ui;
struct ui *ui_list;

/* A cleanup handler that restores the current UI.  */

static void
restore_ui_cleanup (void *data)
{
  current_ui = (struct ui *) data;
}

/* See top.h.  */

struct cleanup *
make_cleanup_restore_current_ui (void)
{
  return make_cleanup (restore_ui_cleanup, current_ui);
}

/* See top.h.  */

void
switch_thru_all_uis_init (struct switch_thru_all_uis *state)
{
  state->iter = ui_list;
  state->old_chain = make_cleanup_restore_current_ui ();
}

/* See top.h.  */

int
switch_thru_all_uis_cond (struct switch_thru_all_uis *state)
{
  if (state->iter != NULL)
    {
      current_ui = state->iter;
      return 1;
    }
  else
    {
      do_cleanups (state->old_chain);
      return 0;
    }
}

/* See top.h.  */

void
switch_thru_all_uis_next (struct switch_thru_all_uis *state)
{
  state->iter = state->iter->next;
}

/* Get a pointer to the current UI's line buffer.  This is used to
   construct a whole line of input from partial input.  */

static struct buffer *
get_command_line_buffer (void)
{
  return &current_ui->line_buffer;
}

/* When there is an event ready on the stdin file descriptor, instead
   of calling readline directly throught the callback function, or
   instead of calling gdb_readline_no_editing_callback, give gdb a
   chance to detect errors and do something.  */

void
stdin_event_handler (int error, gdb_client_data client_data)
{
  struct ui *ui = (struct ui *) client_data;

  if (error)
    {
      /* Switch to the main UI, so diagnostics always go there.  */
      current_ui = main_ui;

      delete_file_handler (ui->input_fd);
      if (main_ui == ui)
	{
	  /* If stdin died, we may as well kill gdb.  */
	  printf_unfiltered (_("error detected on stdin\n"));
	  quit_command ((char *) 0, 0);
	}
      else
	{
	  /* Simply delete the UI.  */
	  delete_ui (ui);
	}
    }
  else
    {
      /* Switch to the UI whose input descriptor woke up the event
	 loop.  */
      current_ui = ui;

      /* This makes sure a ^C immediately followed by further input is
	 always processed in that order.  E.g,. with input like
	 "^Cprint 1\n", the SIGINT handler runs, marks the async
	 signal handler, and then select/poll may return with stdin
	 ready, instead of -1/EINTR.  The
	 gdb.base/double-prompt-target-event-error.exp test exercises
	 this.  */
      QUIT;

      do
	{
	  call_stdin_event_handler_again_p = 0;
	  ui->call_readline (client_data);
	}
      while (call_stdin_event_handler_again_p != 0);
    }
}

/* See top.h.  */

void
ui_register_input_event_handler (struct ui *ui)
{
  add_file_handler (ui->input_fd, stdin_event_handler, ui);
}

/* See top.h.  */

void
ui_unregister_input_event_handler (struct ui *ui)
{
  delete_file_handler (ui->input_fd);
}

/* Re-enable stdin after the end of an execution command in
   synchronous mode, or after an error from the target, and we aborted
   the exec operation.  */

void
async_enable_stdin (void)
{
  struct ui *ui = current_ui;

  if (ui->prompt_state == PROMPT_BLOCKED)
    {
      target_terminal_ours ();
      ui_register_input_event_handler (ui);
      ui->prompt_state = PROMPT_NEEDED;
    }
}

/* Disable reads from stdin (the console) marking the command as
   synchronous.  */

void
async_disable_stdin (void)
{
  struct ui *ui = current_ui;

  ui->prompt_state = PROMPT_BLOCKED;
  delete_file_handler (ui->input_fd);
}


/* Handle a gdb command line.  This function is called when
   handle_line_of_input has concatenated one or more input lines into
   a whole command.  */

void
command_handler (char *command)
{
  struct ui *ui = current_ui;
  struct cleanup *stat_chain;
  char *c;

  if (ui->instream == ui->stdin_stream)
    reinitialize_more_filter ();

  stat_chain = make_command_stats_cleanup (1);

  /* Do not execute commented lines.  */
  for (c = command; *c == ' ' || *c == '\t'; c++)
    ;
  if (c[0] != '#')
    {
      execute_command (command, ui->instream == ui->stdin_stream);

      /* Do any commands attached to breakpoint we stopped at.  */
      bpstat_do_actions ();
    }

  do_cleanups (stat_chain);
}

/* Append RL, an input line returned by readline or one of its
   emulations, to CMD_LINE_BUFFER.  Returns the command line if we
   have a whole command line ready to be processed by the command
   interpreter or NULL if the command line isn't complete yet (input
   line ends in a backslash).  Takes ownership of RL.  */

static char *
command_line_append_input_line (struct buffer *cmd_line_buffer, char *rl)
{
  char *cmd;
  size_t len;

  len = strlen (rl);

  if (len > 0 && rl[len - 1] == '\\')
    {
      /* Don't copy the backslash and wait for more.  */
      buffer_grow (cmd_line_buffer, rl, len - 1);
      cmd = NULL;
    }
  else
    {
      /* Copy whole line including terminating null, and we're
	 done.  */
      buffer_grow (cmd_line_buffer, rl, len + 1);
      cmd = cmd_line_buffer->buffer;
    }

  /* Allocated in readline.  */
  xfree (rl);

  return cmd;
}

/* Handle a line of input coming from readline.

   If the read line ends with a continuation character (backslash),
   save the partial input in CMD_LINE_BUFFER (except the backslash),
   and return NULL.  Otherwise, save the partial input and return a
   pointer to CMD_LINE_BUFFER's buffer (null terminated), indicating a
   whole command line is ready to be executed.

   Returns EOF on end of file.

   If REPEAT, handle command repetitions:

     - If the input command line is NOT empty, the command returned is
       copied into the global 'saved_command_line' var so that it can
       be repeated later.

     - OTOH, if the input command line IS empty, return the previously
       saved command instead of the empty input line.
*/

char *
handle_line_of_input (struct buffer *cmd_line_buffer,
		      char *rl, int repeat, char *annotation_suffix)
{
  struct ui *ui = current_ui;
  int from_tty = ui->instream == ui->stdin_stream;
  char *p1;
  char *cmd;

  if (rl == NULL)
    return (char *) EOF;

  cmd = command_line_append_input_line (cmd_line_buffer, rl);
  if (cmd == NULL)
    return NULL;

  /* We have a complete command line now.  Prepare for the next
     command, but leave ownership of memory to the buffer .  */
  cmd_line_buffer->used_size = 0;

  if (from_tty && annotation_level > 1)
    {
      printf_unfiltered (("\n\032\032post-"));
      puts_unfiltered (annotation_suffix);
      printf_unfiltered (("\n"));
    }

#define SERVER_COMMAND_PREFIX "server "
  if (startswith (cmd, SERVER_COMMAND_PREFIX))
    {
      /* Note that we don't set `saved_command_line'.  Between this
         and the check in dont_repeat, this insures that repeating
         will still do the right thing.  */
      return cmd + strlen (SERVER_COMMAND_PREFIX);
    }

  /* Do history expansion if that is wished.  */
  if (history_expansion_p && from_tty && input_interactive_p (current_ui))
    {
      char *history_value;
      int expanded;

      expanded = history_expand (cmd, &history_value);
      if (expanded)
	{
	  size_t len;

	  /* Print the changes.  */
	  printf_unfiltered ("%s\n", history_value);

	  /* If there was an error, call this function again.  */
	  if (expanded < 0)
	    {
	      xfree (history_value);
	      return cmd;
	    }

	  /* history_expand returns an allocated string.  Just replace
	     our buffer with it.  */
	  len = strlen (history_value);
	  xfree (buffer_finish (cmd_line_buffer));
	  cmd_line_buffer->buffer = history_value;
	  cmd_line_buffer->buffer_size = len + 1;
	  cmd = history_value;
	}
    }

  /* If we just got an empty line, and that is supposed to repeat the
     previous command, return the previously saved command.  */
  for (p1 = cmd; *p1 == ' ' || *p1 == '\t'; p1++)
    ;
  if (repeat && *p1 == '\0')
    return saved_command_line;

  /* Add command to history if appropriate.  Note: lines consisting
     solely of comments are also added to the command history.  This
     is useful when you type a command, and then realize you don't
     want to execute it quite yet.  You can comment out the command
     and then later fetch it from the value history and remove the
     '#'.  The kill ring is probably better, but some people are in
     the habit of commenting things out.  */
  if (*cmd != '\0' && from_tty && input_interactive_p (current_ui))
    gdb_add_history (cmd);

  /* Save into global buffer if appropriate.  */
  if (repeat)
    {
      xfree (saved_command_line);
      saved_command_line = xstrdup (cmd);
      return saved_command_line;
    }
  else
    return cmd;
}

/* Handle a complete line of input.  This is called by the callback
   mechanism within the readline library.  Deal with incomplete
   commands as well, by saving the partial input in a global
   buffer.

   NOTE: This is the asynchronous version of the command_line_input
   function.  */

void
command_line_handler (char *rl)
{
  struct buffer *line_buffer = get_command_line_buffer ();
  struct ui *ui = current_ui;
  char *cmd;

  cmd = handle_line_of_input (line_buffer, rl, 1, "prompt");
  if (cmd == (char *) EOF)
    {
      /* stdin closed.  The connection with the terminal is gone.
	 This happens at the end of a testsuite run, after Expect has
	 hung up but GDB is still alive.  In such a case, we just quit
	 gdb killing the inferior program too.  */
      printf_unfiltered ("quit\n");
      execute_command ("quit", 1);
    }
  else if (cmd == NULL)
    {
      /* We don't have a full line yet.  Print an empty prompt.  */
      display_gdb_prompt ("");
    }
  else
    {
      ui->prompt_state = PROMPT_NEEDED;

      command_handler (cmd);

      if (ui->prompt_state != PROMPTED)
	display_gdb_prompt (0);
    }
}

/* Does reading of input from terminal w/o the editing features
   provided by the readline library.  Calls the line input handler
   once we have a whole input line.  */

void
gdb_readline_no_editing_callback (gdb_client_data client_data)
{
  int c;
  char *result;
  struct buffer line_buffer;
  static int done_once = 0;
  struct ui *ui = current_ui;

  buffer_init (&line_buffer);

  /* Unbuffer the input stream, so that, later on, the calls to fgetc
     fetch only one char at the time from the stream.  The fgetc's will
     get up to the first newline, but there may be more chars in the
     stream after '\n'.  If we buffer the input and fgetc drains the
     stream, getting stuff beyond the newline as well, a select, done
     afterwards will not trigger.  */
  if (!done_once && !ISATTY (ui->instream))
    {
      setbuf (ui->instream, NULL);
      done_once = 1;
    }

  /* We still need the while loop here, even though it would seem
     obvious to invoke gdb_readline_no_editing_callback at every
     character entered.  If not using the readline library, the
     terminal is in cooked mode, which sends the characters all at
     once.  Poll will notice that the input fd has changed state only
     after enter is pressed.  At this point we still need to fetch all
     the chars entered.  */

  while (1)
    {
      /* Read from stdin if we are executing a user defined command.
         This is the right thing for prompt_for_continue, at least.  */
      c = fgetc (ui->instream != NULL ? ui->instream : ui->stdin_stream);

      if (c == EOF)
	{
	  if (line_buffer.used_size > 0)
	    {
	      /* The last line does not end with a newline.  Return it, and
		 if we are called again fgetc will still return EOF and
		 we'll return NULL then.  */
	      break;
	    }
	  xfree (buffer_finish (&line_buffer));
	  ui->input_handler (NULL);
	  return;
	}

      if (c == '\n')
	{
	  if (line_buffer.used_size > 0
	      && line_buffer.buffer[line_buffer.used_size - 1] == '\r')
	    line_buffer.used_size--;
	  break;
	}

      buffer_grow_char (&line_buffer, c);
    }

  buffer_grow_char (&line_buffer, '\0');
  result = buffer_finish (&line_buffer);
  ui->input_handler (result);
}


/* The serial event associated with the QUIT flag.  set_quit_flag sets
   this, and check_quit_flag clears it.  Used by interruptible_select
   to be able to do interruptible I/O with no race with the SIGINT
   handler.  */
static struct serial_event *quit_serial_event;

/* Initialization of signal handlers and tokens.  There is a function
   handle_sig* for each of the signals GDB cares about.  Specifically:
   SIGINT, SIGFPE, SIGQUIT, SIGTSTP, SIGHUP, SIGWINCH.  These
   functions are the actual signal handlers associated to the signals
   via calls to signal().  The only job for these functions is to
   enqueue the appropriate event/procedure with the event loop.  Such
   procedures are the old signal handlers.  The event loop will take
   care of invoking the queued procedures to perform the usual tasks
   associated with the reception of the signal.  */
/* NOTE: 1999-04-30 This is the asynchronous version of init_signals.
   init_signals will become obsolete as we move to have to event loop
   as the default for gdb.  */
void
async_init_signals (void)
{
  initialize_async_signal_handlers ();

  quit_serial_event = make_serial_event ();

  signal (SIGINT, handle_sigint);
  sigint_token =
    create_async_signal_handler (async_request_quit, NULL);
  signal (SIGTERM, handle_sigterm);
  async_sigterm_token
    = create_async_signal_handler (async_sigterm_handler, NULL);

  /* If SIGTRAP was set to SIG_IGN, then the SIG_IGN will get passed
     to the inferior and breakpoints will be ignored.  */
#ifdef SIGTRAP
  signal (SIGTRAP, SIG_DFL);
#endif

#ifdef SIGQUIT
  /* If we initialize SIGQUIT to SIG_IGN, then the SIG_IGN will get
     passed to the inferior, which we don't want.  It would be
     possible to do a "signal (SIGQUIT, SIG_DFL)" after we fork, but
     on BSD4.3 systems using vfork, that can affect the
     GDB process as well as the inferior (the signal handling tables
     might be in memory, shared between the two).  Since we establish
     a handler for SIGQUIT, when we call exec it will set the signal
     to SIG_DFL for us.  */
  signal (SIGQUIT, handle_sigquit);
  sigquit_token =
    create_async_signal_handler (async_do_nothing, NULL);
#endif
#ifdef SIGHUP
  if (signal (SIGHUP, handle_sighup) != SIG_IGN)
    sighup_token =
      create_async_signal_handler (async_disconnect, NULL);
  else
    sighup_token =
      create_async_signal_handler (async_do_nothing, NULL);
#endif
  signal (SIGFPE, handle_sigfpe);
  sigfpe_token =
    create_async_signal_handler (async_float_handler, NULL);

#ifdef STOP_SIGNAL
  sigtstp_token =
    create_async_signal_handler (async_stop_sig, NULL);
#endif
}

/* See defs.h.  */

void
quit_serial_event_set (void)
{
  serial_event_set (quit_serial_event);
}

/* See defs.h.  */

void
quit_serial_event_clear (void)
{
  serial_event_clear (quit_serial_event);
}

/* Return the selectable file descriptor of the serial event
   associated with the quit flag.  */

static int
quit_serial_event_fd (void)
{
  return serial_event_fd (quit_serial_event);
}

/* See defs.h.  */

void
default_quit_handler (void)
{
  if (check_quit_flag ())
    {
      if (target_terminal_is_ours ())
	quit ();
      else
	target_pass_ctrlc ();
    }
}

/* See defs.h.  */
quit_handler_ftype *quit_handler = default_quit_handler;

/* Data for make_cleanup_override_quit_handler.  Wrap the previous
   handler pointer in a data struct because it's not portable to cast
   a function pointer to a data pointer, which is what make_cleanup
   expects.  */
struct quit_handler_cleanup_data
{
  /* The previous quit handler.  */
  quit_handler_ftype *prev_handler;
};

/* Cleanup call that restores the previous quit handler.  */

static void
restore_quit_handler (void *arg)
{
  struct quit_handler_cleanup_data *data
    = (struct quit_handler_cleanup_data *) arg;

  quit_handler = data->prev_handler;
}

/* Destructor for the quit handler cleanup.  */

static void
restore_quit_handler_dtor (void *arg)
{
  xfree (arg);
}

/* See defs.h.  */

struct cleanup *
make_cleanup_override_quit_handler (quit_handler_ftype *new_quit_handler)
{
  struct cleanup *old_chain;
  struct quit_handler_cleanup_data *data;

  data = XNEW (struct quit_handler_cleanup_data);
  data->prev_handler = quit_handler;
  old_chain = make_cleanup_dtor (restore_quit_handler, data,
				 restore_quit_handler_dtor);
  quit_handler = new_quit_handler;
  return old_chain;
}

/* Handle a SIGINT.  */

void
handle_sigint (int sig)
{
  signal (sig, handle_sigint);

  /* We could be running in a loop reading in symfiles or something so
     it may be quite a while before we get back to the event loop.  So
     set quit_flag to 1 here.  Then if QUIT is called before we get to
     the event loop, we will unwind as expected.  */
  set_quit_flag ();

  /* In case nothing calls QUIT before the event loop is reached, the
     event loop handles it.  */
  mark_async_signal_handler (sigint_token);
}

/* See gdb_select.h.  */

int
interruptible_select (int n,
		      fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		      struct timeval *timeout)
{
  fd_set my_readfds;
  int fd;
  int res;

  if (readfds == NULL)
    {
      readfds = &my_readfds;
      FD_ZERO (&my_readfds);
    }

  fd = quit_serial_event_fd ();
  FD_SET (fd, readfds);
  if (n <= fd)
    n = fd + 1;

  do
    {
      res = gdb_select (n, readfds, writefds, exceptfds, timeout);
    }
  while (res == -1 && errno == EINTR);

  if (res == 1 && FD_ISSET (fd, readfds))
    {
      errno = EINTR;
      return -1;
    }
  return res;
}

/* Handle GDB exit upon receiving SIGTERM if target_can_async_p ().  */

static void
async_sigterm_handler (gdb_client_data arg)
{
  quit_force (NULL, 0);
}

/* See defs.h.  */
volatile int sync_quit_force_run;

/* Quit GDB if SIGTERM is received.
   GDB would quit anyway, but this way it will clean up properly.  */
void
handle_sigterm (int sig)
{
  signal (sig, handle_sigterm);

  sync_quit_force_run = 1;
  set_quit_flag ();

  mark_async_signal_handler (async_sigterm_token);
}

/* Do the quit.  All the checks have been done by the caller.  */
void
async_request_quit (gdb_client_data arg)
{
  /* If the quit_flag has gotten reset back to 0 by the time we get
     back here, that means that an exception was thrown to unwind the
     current command before we got back to the event loop.  So there
     is no reason to call quit again here.  */
  QUIT;
}

#ifdef SIGQUIT
/* Tell the event loop what to do if SIGQUIT is received.
   See event-signal.c.  */
static void
handle_sigquit (int sig)
{
  mark_async_signal_handler (sigquit_token);
  signal (sig, handle_sigquit);
}
#endif

#if defined (SIGQUIT) || defined (SIGHUP)
/* Called by the event loop in response to a SIGQUIT or an
   ignored SIGHUP.  */
static void
async_do_nothing (gdb_client_data arg)
{
  /* Empty function body.  */
}
#endif

#ifdef SIGHUP
/* Tell the event loop what to do if SIGHUP is received.
   See event-signal.c.  */
static void
handle_sighup (int sig)
{
  mark_async_signal_handler (sighup_token);
  signal (sig, handle_sighup);
}

/* Called by the event loop to process a SIGHUP.  */
static void
async_disconnect (gdb_client_data arg)
{

  TRY
    {
      quit_cover ();
    }

  CATCH (exception, RETURN_MASK_ALL)
    {
      fputs_filtered ("Could not kill the program being debugged",
		      gdb_stderr);
      exception_print (gdb_stderr, exception);
    }
  END_CATCH

  TRY
    {
      pop_all_targets ();
    }
  CATCH (exception, RETURN_MASK_ALL)
    {
    }
  END_CATCH

  signal (SIGHUP, SIG_DFL);	/*FIXME: ???????????  */
  raise (SIGHUP);
}
#endif

#ifdef STOP_SIGNAL
void
handle_stop_sig (int sig)
{
  mark_async_signal_handler (sigtstp_token);
  signal (sig, handle_stop_sig);
}

static void
async_stop_sig (gdb_client_data arg)
{
  char *prompt = get_prompt ();

#if STOP_SIGNAL == SIGTSTP
  signal (SIGTSTP, SIG_DFL);
#if HAVE_SIGPROCMASK
  {
    sigset_t zero;

    sigemptyset (&zero);
    sigprocmask (SIG_SETMASK, &zero, 0);
  }
#elif HAVE_SIGSETMASK
  sigsetmask (0);
#endif
  raise (SIGTSTP);
  signal (SIGTSTP, handle_stop_sig);
#else
  signal (STOP_SIGNAL, handle_stop_sig);
#endif
  printf_unfiltered ("%s", prompt);
  gdb_flush (gdb_stdout);

  /* Forget about any previous command -- null line now will do
     nothing.  */
  dont_repeat ();
}
#endif /* STOP_SIGNAL */

/* Tell the event loop what to do if SIGFPE is received.
   See event-signal.c.  */
static void
handle_sigfpe (int sig)
{
  mark_async_signal_handler (sigfpe_token);
  signal (sig, handle_sigfpe);
}

/* Event loop will call this functin to process a SIGFPE.  */
static void
async_float_handler (gdb_client_data arg)
{
  /* This message is based on ANSI C, section 4.7.  Note that integer
     divide by zero causes this, so "float" is a misnomer.  */
  error (_("Erroneous arithmetic operation."));
}


/* Set things up for readline to be invoked via the alternate
   interface, i.e. via a callback function
   (gdb_rl_callback_read_char), and hook up instream to the event
   loop.  */

void
gdb_setup_readline (int editing)
{
  struct ui *ui = current_ui;

  /* This function is a noop for the sync case.  The assumption is
     that the sync setup is ALL done in gdb_init, and we would only
     mess it up here.  The sync stuff should really go away over
     time.  */
  if (!batch_silent)
    gdb_stdout = stdio_fileopen (ui->outstream);
  gdb_stderr = stderr_fileopen (ui->errstream);
  gdb_stdlog = gdb_stderr;  /* for moment */
  gdb_stdtarg = gdb_stderr; /* for moment */
  gdb_stdtargerr = gdb_stderr; /* for moment */

  /* If the input stream is connected to a terminal, turn on editing.
     However, that is only allowed on the main UI, as we can only have
     one instance of readline.  */
  if (ISATTY (ui->instream) && editing && ui == main_ui)
    {
      /* Tell gdb that we will be using the readline library.  This
	 could be overwritten by a command in .gdbinit like 'set
	 editing on' or 'off'.  */
      ui->command_editing = 1;

      /* When a character is detected on instream by select or poll,
	 readline will be invoked via this callback function.  */
      ui->call_readline = gdb_rl_callback_read_char_wrapper;

      /* Tell readline to use the same input stream that gdb uses.  */
      rl_instream = ui->instream;
    }
  else
    {
      ui->command_editing = 0;
      ui->call_readline = gdb_readline_no_editing_callback;
    }

  /* Now create the event source for this UI's input file descriptor.
     Another source is going to be the target program (inferior), but
     that must be registered only when it actually exists (I.e. after
     we say 'run' or after we connect to a remote target.  */
  ui_register_input_event_handler (ui);
}

/* Disable command input through the standard CLI channels.  Used in
   the suspend proc for interpreters that use the standard gdb readline
   interface, like the cli & the mi.  */

void
gdb_disable_readline (void)
{
  struct ui *ui = current_ui;

  /* FIXME - It is too heavyweight to delete and remake these every
     time you run an interpreter that needs readline.  It is probably
     better to have the interpreters cache these, which in turn means
     that this needs to be moved into interpreter specific code.  */

#if 0
  ui_file_delete (gdb_stdout);
  ui_file_delete (gdb_stderr);
  gdb_stdlog = NULL;
  gdb_stdtarg = NULL;
  gdb_stdtargerr = NULL;
#endif

  if (ui->command_editing)
    gdb_rl_callback_handler_remove ();
  delete_file_handler (ui->input_fd);
}
