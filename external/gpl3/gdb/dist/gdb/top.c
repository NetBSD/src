/* Top level stuff for GDB, the GNU debugger.

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
#include "gdbcmd.h"
#include "cli/cli-cmds.h"
#include "cli/cli-script.h"
#include "cli/cli-setshow.h"
#include "cli/cli-decode.h"
#include "symtab.h"
#include "inferior.h"
#include "exceptions.h"
#include <signal.h>
#include "target.h"
#include "target-dcache.h"
#include "breakpoint.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "language.h"
#include "terminal.h"		/* For job_control.  */
#include "annotate.h"
#include "completer.h"
#include "top.h"
#include "version.h"
#include "serial.h"
#include "doublest.h"
#include "gdb_assert.h"
#include "main.h"
#include "event-loop.h"
#include "gdbthread.h"
#include "python/python.h"
#include "interps.h"
#include "observer.h"
#include "maint.h"
#include "filenames.h"

/* readline include files.  */
#include "readline/readline.h"
#include "readline/history.h"

/* readline defines this.  */
#undef savestring

#include <sys/types.h>

#include "event-top.h"
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include "ui-out.h"
#include "cli-out.h"
#include "tracepoint.h"

extern void initialize_all_files (void);

#define PROMPT(X) the_prompts.prompt_stack[the_prompts.top + X].prompt
#define PREFIX(X) the_prompts.prompt_stack[the_prompts.top + X].prefix
#define SUFFIX(X) the_prompts.prompt_stack[the_prompts.top + X].suffix

/* Default command line prompt.  This is overriden in some configs.  */

#ifndef DEFAULT_PROMPT
#define DEFAULT_PROMPT	"(gdb) "
#endif

/* Initialization file name for gdb.  This is host-dependent.  */

const char gdbinit[] = GDBINIT;

int inhibit_gdbinit = 0;

/* If nonzero, and GDB has been configured to be able to use windows,
   attempt to open them upon startup.  */

int use_windows = 0;

extern char lang_frame_mismatch_warn[];		/* language.c */

/* Flag for whether we want to confirm potentially dangerous
   operations.  Default is yes.  */

int confirm = 1;

static void
show_confirm (struct ui_file *file, int from_tty,
	      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Whether to confirm potentially "
			    "dangerous operations is %s.\n"),
		    value);
}

/* stdio stream that command input is being read from.  Set to stdin
   normally.  Set by source_command to the file we are sourcing.  Set
   to NULL if we are executing a user-defined command or interacting
   via a GUI.  */

FILE *instream;

/* Flag to indicate whether a user defined command is currently running.  */

int in_user_command;

/* Current working directory.  */

char *current_directory;

/* The directory name is actually stored here (usually).  */
char gdb_dirbuf[1024];

/* Function to call before reading a command, if nonzero.
   The function receives two args: an input stream,
   and a prompt string.  */

void (*window_hook) (FILE *, char *);

/* Buffer used for reading command lines, and the size
   allocated for it so far.  */

char *saved_command_line;
int saved_command_line_size = 100;

/* Nonzero if the current command is modified by "server ".  This
   affects things like recording into the command history, commands
   repeating on RETURN, etc.  This is so a user interface (emacs, GUI,
   whatever) can issue its own commands and also send along commands
   from the user, and have the user not notice that the user interface
   is issuing commands too.  */
int server_command;

/* Timeout limit for response from target.  */

/* The default value has been changed many times over the years.  It 
   was originally 5 seconds.  But that was thought to be a long time 
   to sit and wait, so it was changed to 2 seconds.  That was thought
   to be plenty unless the connection was going through some terminal 
   server or multiplexer or other form of hairy serial connection.

   In mid-1996, remote_timeout was moved from remote.c to top.c and 
   it began being used in other remote-* targets.  It appears that the
   default was changed to 20 seconds at that time, perhaps because the
   Renesas E7000 ICE didn't always respond in a timely manner.

   But if 5 seconds is a long time to sit and wait for retransmissions,
   20 seconds is far worse.  This demonstrates the difficulty of using 
   a single variable for all protocol timeouts.

   As remote.c is used much more than remote-e7000.c, it was changed 
   back to 2 seconds in 1999.  */

int remote_timeout = 2;

/* Non-zero tells remote* modules to output debugging info.  */

int remote_debug = 0;

/* Sbrk location on entry to main.  Used for statistics only.  */
#ifdef HAVE_SBRK
char *lim_at_start;
#endif

/* Hooks for alternate command interfaces.  */

/* Called after most modules have been initialized, but before taking
   users command file.

   If the UI fails to initialize and it wants GDB to continue using
   the default UI, then it should clear this hook before returning.  */

void (*deprecated_init_ui_hook) (char *argv0);

/* This hook is called from within gdb's many mini-event loops which
   could steal control from a real user interface's event loop.  It
   returns non-zero if the user is requesting a detach, zero
   otherwise.  */

int (*deprecated_ui_loop_hook) (int);


/* Called from print_frame_info to list the line we stopped in.  */

void (*deprecated_print_frame_info_listing_hook) (struct symtab * s, 
						  int line,
						  int stopline, 
						  int noerror);
/* Replaces most of query.  */

int (*deprecated_query_hook) (const char *, va_list);

/* Replaces most of warning.  */

void (*deprecated_warning_hook) (const char *, va_list);

/* These three functions support getting lines of text from the user.
   They are used in sequence.  First deprecated_readline_begin_hook is
   called with a text string that might be (for example) a message for
   the user to type in a sequence of commands to be executed at a
   breakpoint.  If this function calls back to a GUI, it might take
   this opportunity to pop up a text interaction window with this
   message.  Next, deprecated_readline_hook is called with a prompt
   that is emitted prior to collecting the user input.  It can be
   called multiple times.  Finally, deprecated_readline_end_hook is
   called to notify the GUI that we are done with the interaction
   window and it can close it.  */

void (*deprecated_readline_begin_hook) (char *, ...);
char *(*deprecated_readline_hook) (char *);
void (*deprecated_readline_end_hook) (void);

/* Called as appropriate to notify the interface that we have attached
   to or detached from an already running process.  */

void (*deprecated_attach_hook) (void);
void (*deprecated_detach_hook) (void);

/* Called during long calculations to allow GUI to repair window
   damage, and to check for stop buttons, etc...  */

void (*deprecated_interactive_hook) (void);

/* Tell the GUI someone changed the register REGNO.  -1 means
   that the caller does not know which register changed or
   that several registers have changed (see value_assign).  */
void (*deprecated_register_changed_hook) (int regno);

/* Called when going to wait for the target.  Usually allows the GUI
   to run while waiting for target events.  */

ptid_t (*deprecated_target_wait_hook) (ptid_t ptid,
				       struct target_waitstatus *status,
				       int options);

/* Used by UI as a wrapper around command execution.  May do various
   things like enabling/disabling buttons, etc...  */

void (*deprecated_call_command_hook) (struct cmd_list_element * c, 
				      char *cmd, int from_tty);

/* Called after a `set' command has finished.  Is only run if the
   `set' command succeeded.  */

void (*deprecated_set_hook) (struct cmd_list_element * c);

/* Called when the current thread changes.  Argument is thread id.  */

void (*deprecated_context_hook) (int id);

/* Handler for SIGHUP.  */

#ifdef SIGHUP
/* NOTE 1999-04-29: This function will be static again, once we modify
   gdb to use the event loop as the default command loop and we merge
   event-top.c into this file, top.c.  */
/* static */ void
quit_cover (void)
{
  /* Stop asking user for confirmation --- we're exiting.  This
     prevents asking the user dumb questions.  */
  confirm = 0;
  quit_command ((char *) 0, 0);
}
#endif /* defined SIGHUP */

/* Line number we are currently in, in a file which is being sourced.  */
/* NOTE 1999-04-29: This variable will be static again, once we modify
   gdb to use the event loop as the default command loop and we merge
   event-top.c into this file, top.c.  */
/* static */ int source_line_number;

/* Name of the file we are sourcing.  */
/* NOTE 1999-04-29: This variable will be static again, once we modify
   gdb to use the event loop as the default command loop and we merge
   event-top.c into this file, top.c.  */
/* static */ const char *source_file_name;

/* Clean up on error during a "source" command (or execution of a
   user-defined command).  */

void
do_restore_instream_cleanup (void *stream)
{
  /* Restore the previous input stream.  */
  instream = stream;
}

/* Read commands from STREAM.  */
void
read_command_file (FILE *stream)
{
  struct cleanup *cleanups;

  cleanups = make_cleanup (do_restore_instream_cleanup, instream);
  instream = stream;
  command_loop ();
  do_cleanups (cleanups);
}

void (*pre_init_ui_hook) (void);

#ifdef __MSDOS__
static void
do_chdir_cleanup (void *old_dir)
{
  chdir (old_dir);
  xfree (old_dir);
}
#endif

struct cleanup *
prepare_execute_command (void)
{
  struct value *mark;
  struct cleanup *cleanup;

  mark = value_mark ();
  cleanup = make_cleanup_value_free_to_mark (mark);

  /* With multiple threads running while the one we're examining is
     stopped, the dcache can get stale without us being able to detect
     it.  For the duration of the command, though, use the dcache to
     help things like backtrace.  */
  if (non_stop)
    target_dcache_invalidate ();

  return cleanup;
}

/* Tell the user if the language has changed (except first time) after
   executing a command.  */

void
check_frame_language_change (void)
{
  static int warned = 0;

  /* First make sure that a new frame has been selected, in case the
     command or the hooks changed the program state.  */
  deprecated_safe_get_selected_frame ();
  if (current_language != expected_language)
    {
      if (language_mode == language_mode_auto && info_verbose)
	{
	  language_info (1);	/* Print what changed.  */
	}
      warned = 0;
    }

  /* Warn the user if the working language does not match the language
     of the current frame.  Only warn the user if we are actually
     running the program, i.e. there is a stack.  */
  /* FIXME: This should be cacheing the frame and only running when
     the frame changes.  */

  if (has_stack_frames ())
    {
      enum language flang;

      flang = get_frame_language ();
      if (!warned
	  && flang != language_unknown
	  && flang != current_language->la_language)
	{
	  printf_filtered ("%s\n", lang_frame_mismatch_warn);
	  warned = 1;
	}
    }
}

/* Execute the line P as a command, in the current user context.
   Pass FROM_TTY as second argument to the defining function.  */

void
execute_command (char *p, int from_tty)
{
  struct cleanup *cleanup_if_error, *cleanup;
  struct cmd_list_element *c;
  char *line;

  cleanup_if_error = make_bpstat_clear_actions_cleanup ();
  cleanup = prepare_execute_command ();

  /* Force cleanup of any alloca areas if using C alloca instead of
     a builtin alloca.  */
  alloca (0);

  /* This can happen when command_line_input hits end of file.  */
  if (p == NULL)
    {
      do_cleanups (cleanup);
      discard_cleanups (cleanup_if_error);
      return;
    }

  target_log_command (p);

  while (*p == ' ' || *p == '\t')
    p++;
  if (*p)
    {
      const char *cmd = p;
      char *arg;
      line = p;

      /* If trace-commands is set then this will print this command.  */
      print_command_trace (p);

      c = lookup_cmd (&cmd, cmdlist, "", 0, 1);
      p = (char *) cmd;

      /* Pass null arg rather than an empty one.  */
      arg = *p ? p : 0;

      /* FIXME: cagney/2002-02-02: The c->type test is pretty dodgy
         while the is_complete_command(cfunc) test is just plain
         bogus.  They should both be replaced by a test of the form
         c->strip_trailing_white_space_p.  */
      /* NOTE: cagney/2002-02-02: The function.cfunc in the below
         can't be replaced with func.  This is because it is the
         cfunc, and not the func, that has the value that the
         is_complete_command hack is testing for.  */
      /* Clear off trailing whitespace, except for set and complete
         command.  */
      if (arg
	  && c->type != set_cmd
	  && !is_complete_command (c))
	{
	  p = arg + strlen (arg) - 1;
	  while (p >= arg && (*p == ' ' || *p == '\t'))
	    p--;
	  *(p + 1) = '\0';
	}

      /* If this command has been pre-hooked, run the hook first.  */
      execute_cmd_pre_hook (c);

      if (c->flags & DEPRECATED_WARN_USER)
	deprecated_cmd_warning (line);

      /* c->user_commands would be NULL in the case of a python command.  */
      if (c->class == class_user && c->user_commands)
	execute_user_command (c, arg);
      else if (c->type == set_cmd)
	do_set_command (arg, from_tty, c);
      else if (c->type == show_cmd)
	do_show_command (arg, from_tty, c);
      else if (!cmd_func_p (c))
	error (_("That is not a command, just a help topic."));
      else if (deprecated_call_command_hook)
	deprecated_call_command_hook (c, arg, from_tty);
      else
	cmd_func (c, arg, from_tty);

      /* If the interpreter is in sync mode (we're running a user
	 command's list, running command hooks or similars), and we
	 just ran a synchronous command that started the target, wait
	 for that command to end.  */
      if (!interpreter_async && sync_execution)
	{
	  while (gdb_do_one_event () >= 0)
	    if (!sync_execution)
	      break;
	}

      /* If this command has been post-hooked, run the hook last.  */
      execute_cmd_post_hook (c);

    }

  check_frame_language_change ();

  do_cleanups (cleanup);
  discard_cleanups (cleanup_if_error);
}

/* Run execute_command for P and FROM_TTY.  Capture its output into the
   returned string, do not display it to the screen.  BATCH_FLAG will be
   temporarily set to true.  */

char *
execute_command_to_string (char *p, int from_tty)
{
  struct ui_file *str_file;
  struct cleanup *cleanup;
  char *retval;

  /* GDB_STDOUT should be better already restored during these
     restoration callbacks.  */
  cleanup = set_batch_flag_and_make_cleanup_restore_page_info ();

  make_cleanup_restore_integer (&interpreter_async);
  interpreter_async = 0;

  str_file = mem_fileopen ();

  make_cleanup_ui_file_delete (str_file);
  make_cleanup_restore_ui_file (&gdb_stdout);
  make_cleanup_restore_ui_file (&gdb_stderr);
  make_cleanup_restore_ui_file (&gdb_stdlog);
  make_cleanup_restore_ui_file (&gdb_stdtarg);
  make_cleanup_restore_ui_file (&gdb_stdtargerr);

  if (ui_out_redirect (current_uiout, str_file) < 0)
    warning (_("Current output protocol does not support redirection"));
  else
    make_cleanup_ui_out_redirect_pop (current_uiout);

  gdb_stdout = str_file;
  gdb_stderr = str_file;
  gdb_stdlog = str_file;
  gdb_stdtarg = str_file;
  gdb_stdtargerr = str_file;

  execute_command (p, from_tty);

  retval = ui_file_xstrdup (str_file, NULL);

  do_cleanups (cleanup);

  return retval;
}

/* Read commands from `instream' and execute them
   until end of file or error reading instream.  */

void
command_loop (void)
{
  struct cleanup *old_chain;
  char *command;
  int stdin_is_tty = ISATTY (stdin);

  while (instream && !feof (instream))
    {
      if (window_hook && instream == stdin)
	(*window_hook) (instream, get_prompt ());

      clear_quit_flag ();
      if (instream == stdin && stdin_is_tty)
	reinitialize_more_filter ();
      old_chain = make_cleanup (null_cleanup, 0);

      /* Get a command-line.  This calls the readline package.  */
      command = command_line_input (instream == stdin ?
				    get_prompt () : (char *) NULL,
				    instream == stdin, "prompt");
      if (command == 0)
	{
	  do_cleanups (old_chain);
	  return;
	}

      make_command_stats_cleanup (1);

      execute_command (command, instream == stdin);

      /* Do any commands attached to breakpoint we are stopped at.  */
      bpstat_do_actions ();

      do_cleanups (old_chain);
    }
}

/* When nonzero, cause dont_repeat to do nothing.  This should only be
   set via prevent_dont_repeat.  */

static int suppress_dont_repeat = 0;

/* Commands call this if they do not want to be repeated by null lines.  */

void
dont_repeat (void)
{
  if (suppress_dont_repeat || server_command)
    return;

  /* If we aren't reading from standard input, we are saving the last
     thing read from stdin in line and don't want to delete it.  Null
     lines won't repeat here in any case.  */
  if (instream == stdin)
    *saved_command_line = 0;
}

/* Prevent dont_repeat from working, and return a cleanup that
   restores the previous state.  */

struct cleanup *
prevent_dont_repeat (void)
{
  struct cleanup *result = make_cleanup_restore_integer (&suppress_dont_repeat);

  suppress_dont_repeat = 1;
  return result;
}


/* Read a line from the stream "instream" without command line editing.

   It prints PROMPT_ARG once at the start.
   Action is compatible with "readline", e.g. space for the result is
   malloc'd and should be freed by the caller.

   A NULL return means end of file.  */
char *
gdb_readline (char *prompt_arg)
{
  int c;
  char *result;
  int input_index = 0;
  int result_size = 80;

  if (prompt_arg)
    {
      /* Don't use a _filtered function here.  It causes the assumed
         character position to be off, since the newline we read from
         the user is not accounted for.  */
      fputs_unfiltered (prompt_arg, gdb_stdout);
      gdb_flush (gdb_stdout);
    }

  result = (char *) xmalloc (result_size);

  while (1)
    {
      /* Read from stdin if we are executing a user defined command.
         This is the right thing for prompt_for_continue, at least.  */
      c = fgetc (instream ? instream : stdin);

      if (c == EOF)
	{
	  if (input_index > 0)
	    /* The last line does not end with a newline.  Return it, and
	       if we are called again fgetc will still return EOF and
	       we'll return NULL then.  */
	    break;
	  xfree (result);
	  return NULL;
	}

      if (c == '\n')
	{
	  if (input_index > 0 && result[input_index - 1] == '\r')
	    input_index--;
	  break;
	}

      result[input_index++] = c;
      while (input_index >= result_size)
	{
	  result_size *= 2;
	  result = (char *) xrealloc (result, result_size);
	}
    }

  result[input_index++] = '\0';
  return result;
}

/* Variables which control command line editing and history
   substitution.  These variables are given default values at the end
   of this file.  */
static int command_editing_p;

/* NOTE 1999-04-29: This variable will be static again, once we modify
   gdb to use the event loop as the default command loop and we merge
   event-top.c into this file, top.c.  */

/* static */ int history_expansion_p;

static int write_history_p;
static void
show_write_history_p (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Saving of the history record on exit is %s.\n"),
		    value);
}

/* The variable associated with the "set/show history size"
   command.  */
static unsigned int history_size_setshow_var;

static void
show_history_size (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("The size of the command history is %s.\n"),
		    value);
}

static char *history_filename;
static void
show_history_filename (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("The filename in which to record "
			    "the command history is \"%s\".\n"),
		    value);
}

/* This is like readline(), but it has some gdb-specific behavior.
   gdb may want readline in both the synchronous and async modes during
   a single gdb invocation.  At the ordinary top-level prompt we might
   be using the async readline.  That means we can't use
   rl_pre_input_hook, since it doesn't work properly in async mode.
   However, for a secondary prompt (" >", such as occurs during a
   `define'), gdb wants a synchronous response.

   We used to call readline() directly, running it in synchronous
   mode.  But mixing modes this way is not supported, and as of
   readline 5.x it no longer works; the arrow keys come unbound during
   the synchronous call.  So we make a nested call into the event
   loop.  That's what gdb_readline_wrapper is for.  */

/* A flag set as soon as gdb_readline_wrapper_line is called; we can't
   rely on gdb_readline_wrapper_result, which might still be NULL if
   the user types Control-D for EOF.  */
static int gdb_readline_wrapper_done;

/* The result of the current call to gdb_readline_wrapper, once a newline
   is seen.  */
static char *gdb_readline_wrapper_result;

/* Any intercepted hook.  Operate-and-get-next sets this, expecting it
   to be called after the newline is processed (which will redisplay
   the prompt).  But in gdb_readline_wrapper we will not get a new
   prompt until the next call, or until we return to the event loop.
   So we disable this hook around the newline and restore it before we
   return.  */
static void (*saved_after_char_processing_hook) (void);

/* This function is called when readline has seen a complete line of
   text.  */

static void
gdb_readline_wrapper_line (char *line)
{
  gdb_assert (!gdb_readline_wrapper_done);
  gdb_readline_wrapper_result = line;
  gdb_readline_wrapper_done = 1;

  /* Prevent operate-and-get-next from acting too early.  */
  saved_after_char_processing_hook = after_char_processing_hook;
  after_char_processing_hook = NULL;

  /* Prevent parts of the prompt from being redisplayed if annotations
     are enabled, and readline's state getting out of sync.  */
  if (async_command_editing_p)
    rl_callback_handler_remove ();
}

struct gdb_readline_wrapper_cleanup
  {
    void (*handler_orig) (char *);
    int already_prompted_orig;
  };

static void
gdb_readline_wrapper_cleanup (void *arg)
{
  struct gdb_readline_wrapper_cleanup *cleanup = arg;

  rl_already_prompted = cleanup->already_prompted_orig;

  gdb_assert (input_handler == gdb_readline_wrapper_line);
  input_handler = cleanup->handler_orig;
  gdb_readline_wrapper_result = NULL;
  gdb_readline_wrapper_done = 0;

  after_char_processing_hook = saved_after_char_processing_hook;
  saved_after_char_processing_hook = NULL;

  xfree (cleanup);
}

char *
gdb_readline_wrapper (char *prompt)
{
  struct cleanup *back_to;
  struct gdb_readline_wrapper_cleanup *cleanup;
  char *retval;

  cleanup = xmalloc (sizeof (*cleanup));
  cleanup->handler_orig = input_handler;
  input_handler = gdb_readline_wrapper_line;

  cleanup->already_prompted_orig = rl_already_prompted;

  back_to = make_cleanup (gdb_readline_wrapper_cleanup, cleanup);

  /* Display our prompt and prevent double prompt display.  */
  display_gdb_prompt (prompt);
  rl_already_prompted = 1;

  if (after_char_processing_hook)
    (*after_char_processing_hook) ();
  gdb_assert (after_char_processing_hook == NULL);

  while (gdb_do_one_event () >= 0)
    if (gdb_readline_wrapper_done)
      break;

  retval = gdb_readline_wrapper_result;
  do_cleanups (back_to);
  return retval;
}


/* The current saved history number from operate-and-get-next.
   This is -1 if not valid.  */
static int operate_saved_history = -1;

/* This is put on the appropriate hook and helps operate-and-get-next
   do its work.  */
static void
gdb_rl_operate_and_get_next_completion (void)
{
  int delta = where_history () - operate_saved_history;

  /* The `key' argument to rl_get_previous_history is ignored.  */
  rl_get_previous_history (delta, 0);
  operate_saved_history = -1;

  /* readline doesn't automatically update the display for us.  */
  rl_redisplay ();

  after_char_processing_hook = NULL;
  rl_pre_input_hook = NULL;
}

/* This is a gdb-local readline command handler.  It accepts the
   current command line (like RET does) and, if this command was taken
   from the history, arranges for the next command in the history to
   appear on the command line when the prompt returns.
   We ignore the arguments.  */
static int
gdb_rl_operate_and_get_next (int count, int key)
{
  int where;

  /* Use the async hook.  */
  after_char_processing_hook = gdb_rl_operate_and_get_next_completion;

  /* Find the current line, and find the next line to use.  */
  where = where_history();

  if ((history_is_stifled () && (history_length >= history_max_entries))
      || (where >= history_length - 1))
    operate_saved_history = where;
  else
    operate_saved_history = where + 1;

  return rl_newline (1, key);
}

/* Read one line from the command input stream `instream'
   into the local static buffer `linebuffer' (whose current length
   is `linelength').
   The buffer is made bigger as necessary.
   Returns the address of the start of the line.

   NULL is returned for end of file.

   *If* the instream == stdin & stdin is a terminal, the line read
   is copied into the file line saver (global var char *line,
   length linesize) so that it can be duplicated.

   This routine either uses fancy command line editing or
   simple input as the user has requested.  */

char *
command_line_input (char *prompt_arg, int repeat, char *annotation_suffix)
{
  static char *linebuffer = 0;
  static unsigned linelength = 0;
  char *p;
  char *p1;
  char *rl;
  char *local_prompt = prompt_arg;
  char *nline;
  char got_eof = 0;

  /* The annotation suffix must be non-NULL.  */
  if (annotation_suffix == NULL)
    annotation_suffix = "";

  if (annotation_level > 1 && instream == stdin)
    {
      local_prompt = alloca ((prompt_arg == NULL ? 0 : strlen (prompt_arg))
			     + strlen (annotation_suffix) + 40);
      if (prompt_arg == NULL)
	local_prompt[0] = '\0';
      else
	strcpy (local_prompt, prompt_arg);
      strcat (local_prompt, "\n\032\032");
      strcat (local_prompt, annotation_suffix);
      strcat (local_prompt, "\n");
    }

  if (linebuffer == 0)
    {
      linelength = 80;
      linebuffer = (char *) xmalloc (linelength);
    }

  p = linebuffer;

  /* Control-C quits instantly if typed while in this loop
     since it should not wait until the user types a newline.  */
  immediate_quit++;
  QUIT;
#ifdef STOP_SIGNAL
  if (job_control)
    signal (STOP_SIGNAL, handle_stop_sig);
#endif

  while (1)
    {
      /* Make sure that all output has been output.  Some machines may
         let you get away with leaving out some of the gdb_flush, but
         not all.  */
      wrap_here ("");
      gdb_flush (gdb_stdout);
      gdb_flush (gdb_stderr);

      if (source_file_name != NULL)
	++source_line_number;

      if (annotation_level > 1 && instream == stdin)
	{
	  puts_unfiltered ("\n\032\032pre-");
	  puts_unfiltered (annotation_suffix);
	  puts_unfiltered ("\n");
	}

      /* Don't use fancy stuff if not talking to stdin.  */
      if (deprecated_readline_hook && input_from_terminal_p ())
	{
	  rl = (*deprecated_readline_hook) (local_prompt);
	}
      else if (command_editing_p && input_from_terminal_p ())
	{
	  rl = gdb_readline_wrapper (local_prompt);
	}
      else
	{
	  rl = gdb_readline (local_prompt);
	}

      if (annotation_level > 1 && instream == stdin)
	{
	  puts_unfiltered ("\n\032\032post-");
	  puts_unfiltered (annotation_suffix);
	  puts_unfiltered ("\n");
	}

      if (!rl || rl == (char *) EOF)
	{
	  got_eof = 1;
	  break;
	}
      if (strlen (rl) + 1 + (p - linebuffer) > linelength)
	{
	  linelength = strlen (rl) + 1 + (p - linebuffer);
	  nline = (char *) xrealloc (linebuffer, linelength);
	  p += nline - linebuffer;
	  linebuffer = nline;
	}
      p1 = rl;
      /* Copy line.  Don't copy null at end.  (Leaves line alone
         if this was just a newline).  */
      while (*p1)
	*p++ = *p1++;

      xfree (rl);		/* Allocated in readline.  */

      if (p == linebuffer || *(p - 1) != '\\')
	break;

      p--;			/* Put on top of '\'.  */
      local_prompt = (char *) 0;
    }

#ifdef STOP_SIGNAL
  if (job_control)
    signal (STOP_SIGNAL, SIG_DFL);
#endif
  immediate_quit--;

  if (got_eof)
    return NULL;

#define SERVER_COMMAND_LENGTH 7
  server_command =
    (p - linebuffer > SERVER_COMMAND_LENGTH)
    && strncmp (linebuffer, "server ", SERVER_COMMAND_LENGTH) == 0;
  if (server_command)
    {
      /* Note that we don't set `line'.  Between this and the check in
         dont_repeat, this insures that repeating will still do the
         right thing.  */
      *p = '\0';
      return linebuffer + SERVER_COMMAND_LENGTH;
    }

  /* Do history expansion if that is wished.  */
  if (history_expansion_p && instream == stdin
      && ISATTY (instream))
    {
      char *history_value;
      int expanded;

      *p = '\0';		/* Insert null now.  */
      expanded = history_expand (linebuffer, &history_value);
      if (expanded)
	{
	  /* Print the changes.  */
	  printf_unfiltered ("%s\n", history_value);

	  /* If there was an error, call this function again.  */
	  if (expanded < 0)
	    {
	      xfree (history_value);
	      return command_line_input (prompt_arg, repeat,
					 annotation_suffix);
	    }
	  if (strlen (history_value) > linelength)
	    {
	      linelength = strlen (history_value) + 1;
	      linebuffer = (char *) xrealloc (linebuffer, linelength);
	    }
	  strcpy (linebuffer, history_value);
	  p = linebuffer + strlen (linebuffer);
	}
      xfree (history_value);
    }

  /* If we just got an empty line, and that is supposed to repeat the
     previous command, return the value in the global buffer.  */
  if (repeat && p == linebuffer)
    return saved_command_line;
  for (p1 = linebuffer; *p1 == ' ' || *p1 == '\t'; p1++);
  if (repeat && !*p1)
    return saved_command_line;

  *p = 0;

  /* Add line to history if appropriate.  */
  if (*linebuffer && input_from_terminal_p ())
    add_history (linebuffer);

  /* Note: lines consisting solely of comments are added to the command
     history.  This is useful when you type a command, and then
     realize you don't want to execute it quite yet.  You can comment
     out the command and then later fetch it from the value history
     and remove the '#'.  The kill ring is probably better, but some
     people are in the habit of commenting things out.  */
  if (*p1 == '#')
    *p1 = '\0';			/* Found a comment.  */

  /* Save into global buffer if appropriate.  */
  if (repeat)
    {
      if (linelength > saved_command_line_size)
	{
	  saved_command_line = xrealloc (saved_command_line, linelength);
	  saved_command_line_size = linelength;
	}
      strcpy (saved_command_line, linebuffer);
      return saved_command_line;
    }

  return linebuffer;
}

/* Print the GDB banner.  */
void
print_gdb_version (struct ui_file *stream)
{
  /* From GNU coding standards, first line is meant to be easy for a
     program to parse, and is just canonical program name and version
     number, which starts after last space.  */

  fprintf_filtered (stream, "GNU gdb %s%s\n", PKGVERSION, version);

  /* Second line is a copyright notice.  */

  fprintf_filtered (stream,
		    "Copyright (C) 2014 Free Software Foundation, Inc.\n");

  /* Following the copyright is a brief statement that the program is
     free software, that users are free to copy and change it on
     certain conditions, that it is covered by the GNU GPL, and that
     there is no warranty.  */

  fprintf_filtered (stream, "\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\
\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.  Type \"show copying\"\n\
and \"show warranty\" for details.\n");

  /* After the required info we print the configuration information.  */

  fprintf_filtered (stream, "This GDB was configured as \"");
  if (strcmp (host_name, target_name) != 0)
    {
      fprintf_filtered (stream, "--host=%s --target=%s",
			host_name, target_name);
    }
  else
    {
      fprintf_filtered (stream, "%s", host_name);
    }
  fprintf_filtered (stream, "\".\n\
Type \"show configuration\" for configuration details.");

  if (REPORT_BUGS_TO[0])
    {
      fprintf_filtered (stream,
			_("\nFor bug reporting instructions, please see:\n"));
      fprintf_filtered (stream, "%s.\n", REPORT_BUGS_TO);
    }
  fprintf_filtered (stream,
		    _("Find the GDB manual and other documentation \
resources online at:\n<http://www.gnu.org/software/gdb/documentation/>.\n"));
  fprintf_filtered (stream, _("For help, type \"help\".\n"));
  fprintf_filtered (stream, _("Type \"apropos word\" to search for \
commands related to \"word\"."));
}

/* Print the details of GDB build-time configuration.  */
void
print_gdb_configuration (struct ui_file *stream)
{
  fprintf_filtered (stream, _("\
This GDB was configured as follows:\n\
   configure --host=%s --target=%s\n\
"), host_name, target_name);
  fprintf_filtered (stream, _("\
             --with-auto-load-dir=%s\n\
             --with-auto-load-safe-path=%s\n\
"), AUTO_LOAD_DIR, AUTO_LOAD_SAFE_PATH);
#if HAVE_LIBEXPAT
  fprintf_filtered (stream, _("\
             --with-expat\n\
"));
#else
  fprintf_filtered (stream, _("\
             --without-expat\n\
"));
#endif
  if (GDB_DATADIR[0])
    fprintf_filtered (stream, _("\
             --with-gdb-datadir=%s%s\n\
"), GDB_DATADIR, GDB_DATADIR_RELOCATABLE ? " (relocatable)" : "");
#ifdef ICONV_BIN
  fprintf_filtered (stream, _("\
             --with-iconv-bin=%s%s\n\
"), ICONV_BIN, ICONV_BIN_RELOCATABLE ? " (relocatable)" : "");
#endif
  if (JIT_READER_DIR[0])
    fprintf_filtered (stream, _("\
             --with-jit-reader-dir=%s%s\n\
"), JIT_READER_DIR, JIT_READER_DIR_RELOCATABLE ? " (relocatable)" : "");
#if HAVE_LIBUNWIND_IA64_H
  fprintf_filtered (stream, _("\
             --with-libunwind-ia64\n\
"));
#else
  fprintf_filtered (stream, _("\
             --without-libunwind-ia64\n\
"));
#endif
#if HAVE_LIBLZMA
  fprintf_filtered (stream, _("\
             --with-lzma\n\
"));
#else
  fprintf_filtered (stream, _("\
             --without-lzma\n\
"));
#endif
#ifdef WITH_PYTHON_PATH
  fprintf_filtered (stream, _("\
             --with-python=%s%s\n\
"), WITH_PYTHON_PATH, PYTHON_PATH_RELOCATABLE ? " (relocatable)" : "");
#endif
#ifdef RELOC_SRCDIR
  fprintf_filtered (stream, _("\
             --with-relocated-sources=%s\n\
"), RELOC_SRCDIR);
#endif
  if (DEBUGDIR[0])
    fprintf_filtered (stream, _("\
             --with-separate-debug-dir=%s%s\n\
"), DEBUGDIR, DEBUGDIR_RELOCATABLE ? " (relocatable)" : "");
  if (TARGET_SYSTEM_ROOT[0])
    fprintf_filtered (stream, _("\
             --with-sysroot=%s%s\n\
"), TARGET_SYSTEM_ROOT, TARGET_SYSTEM_ROOT_RELOCATABLE ? " (relocatable)" : "");
  if (SYSTEM_GDBINIT[0])
    fprintf_filtered (stream, _("\
             --with-system-gdbinit=%s%s\n\
"), SYSTEM_GDBINIT, SYSTEM_GDBINIT_RELOCATABLE ? " (relocatable)" : "");
#if HAVE_ZLIB_H
  fprintf_filtered (stream, _("\
             --with-zlib\n\
"));
#else
  fprintf_filtered (stream, _("\
             --without-zlib\n\
"));
#endif
#if HAVE_LIBBABELTRACE
    fprintf_filtered (stream, _("\
             --with-babeltrace\n\
"));
#else
    fprintf_filtered (stream, _("\
             --without-babeltrace\n\
"));
#endif
    /* We assume "relocatable" will be printed at least once, thus we always
       print this text.  It's a reasonably safe assumption for now.  */
    fprintf_filtered (stream, _("\n\
(\"Relocatable\" means the directory can be moved with the GDB installation\n\
tree, and GDB will still find it.)\n\
"));
}


/* The current top level prompt, settable with "set prompt", and/or
   with the python `gdb.prompt_hook' hook.  */
static char *top_prompt;

/* Access method for the GDB prompt string.  */

char *
get_prompt (void)
{
  return top_prompt;
}

/* Set method for the GDB prompt string.  */

void
set_prompt (const char *s)
{
  char *p = xstrdup (s);

  xfree (top_prompt);
  top_prompt = p;
}


struct qt_args
{
  char *args;
  int from_tty;
};

/* Callback for iterate_over_inferiors.  Kills or detaches the given
   inferior, depending on how we originally gained control of it.  */

static int
kill_or_detach (struct inferior *inf, void *args)
{
  struct qt_args *qt = args;
  struct thread_info *thread;

  if (inf->pid == 0)
    return 0;

  thread = any_thread_of_process (inf->pid);
  if (thread != NULL)
    {
      switch_to_thread (thread->ptid);

      /* Leave core files alone.  */
      if (target_has_execution)
	{
	  if (inf->attach_flag)
	    target_detach (qt->args, qt->from_tty);
	  else
	    target_kill ();
	}
    }

  return 0;
}

/* Callback for iterate_over_inferiors.  Prints info about what GDB
   will do to each inferior on a "quit".  ARG points to a struct
   ui_out where output is to be collected.  */

static int
print_inferior_quit_action (struct inferior *inf, void *arg)
{
  struct ui_file *stb = arg;

  if (inf->pid == 0)
    return 0;

  if (inf->attach_flag)
    fprintf_filtered (stb,
		      _("\tInferior %d [%s] will be detached.\n"), inf->num,
		      target_pid_to_str (pid_to_ptid (inf->pid)));
  else
    fprintf_filtered (stb,
		      _("\tInferior %d [%s] will be killed.\n"), inf->num,
		      target_pid_to_str (pid_to_ptid (inf->pid)));

  return 0;
}

/* If necessary, make the user confirm that we should quit.  Return
   non-zero if we should quit, zero if we shouldn't.  */

int
quit_confirm (void)
{
  struct ui_file *stb;
  struct cleanup *old_chain;
  char *str;
  int qr;

  /* Don't even ask if we're only debugging a core file inferior.  */
  if (!have_live_inferiors ())
    return 1;

  /* Build the query string as a single string.  */
  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  fprintf_filtered (stb, _("A debugging session is active.\n\n"));
  iterate_over_inferiors (print_inferior_quit_action, stb);
  fprintf_filtered (stb, _("\nQuit anyway? "));

  str = ui_file_xstrdup (stb, NULL);
  make_cleanup (xfree, str);

  qr = query ("%s", str);
  do_cleanups (old_chain);
  return qr;
}

/* Quit without asking for confirmation.  */

void
quit_force (char *args, int from_tty)
{
  int exit_code = 0;
  struct qt_args qt;
  volatile struct gdb_exception ex;

  /* An optional expression may be used to cause gdb to terminate with the 
     value of that expression.  */
  if (args)
    {
      struct value *val = parse_and_eval (args);

      exit_code = (int) value_as_long (val);
    }
  else if (return_child_result)
    exit_code = return_child_result_value;

  qt.args = args;
  qt.from_tty = from_tty;

  /* Wrappers to make the code below a bit more readable.  */
#define DO_TRY \
  TRY_CATCH (ex, RETURN_MASK_ALL)

#define DO_PRINT_EX \
  if (ex.reason < 0) \
    exception_print (gdb_stderr, ex)

  /* We want to handle any quit errors and exit regardless.  */

  /* Get out of tfind mode, and kill or detach all inferiors.  */
  DO_TRY
    {
      disconnect_tracing ();
      iterate_over_inferiors (kill_or_detach, &qt);
    }
  DO_PRINT_EX;

  /* Give all pushed targets a chance to do minimal cleanup, and pop
     them all out.  */
  DO_TRY
    {
      pop_all_targets ();
    }
  DO_PRINT_EX;

  /* Save the history information if it is appropriate to do so.  */
  DO_TRY
    {
      if (write_history_p && history_filename
	  && input_from_terminal_p ())
	write_history (history_filename);
    }
  DO_PRINT_EX;

  /* Do any final cleanups before exiting.  */
  DO_TRY
    {
      do_final_cleanups (all_cleanups ());
    }
  DO_PRINT_EX;

  exit (exit_code);
}

/* Returns whether GDB is running on a terminal and input is
   currently coming from that terminal.  */

int
input_from_terminal_p (void)
{
  if (batch_flag)
    return 0;

  if (gdb_has_a_terminal () && instream == stdin)
    return 1;

  /* If INSTREAM is unset, and we are not in a user command, we
     must be in Insight.  That's like having a terminal, for our
     purposes.  */
  if (instream == NULL && !in_user_command)
    return 1;

  return 0;
}

static void
dont_repeat_command (char *ignored, int from_tty)
{
  /* Can't call dont_repeat here because we're not necessarily reading
     from stdin.  */
  *saved_command_line = 0;
}

/* Functions to manipulate command line editing control variables.  */

/* Number of commands to print in each call to show_commands.  */
#define Hist_print 10
void
show_commands (char *args, int from_tty)
{
  /* Index for history commands.  Relative to history_base.  */
  int offset;

  /* Number of the history entry which we are planning to display next.
     Relative to history_base.  */
  static int num = 0;

  /* Print out some of the commands from the command history.  */

  if (args)
    {
      if (args[0] == '+' && args[1] == '\0')
	/* "info editing +" should print from the stored position.  */
	;
      else
	/* "info editing <exp>" should print around command number <exp>.  */
	num = (parse_and_eval_long (args) - history_base) - Hist_print / 2;
    }
  /* "show commands" means print the last Hist_print commands.  */
  else
    {
      num = history_length - Hist_print;
    }

  if (num < 0)
    num = 0;

  /* If there are at least Hist_print commands, we want to display the last
     Hist_print rather than, say, the last 6.  */
  if (history_length - num < Hist_print)
    {
      num = history_length - Hist_print;
      if (num < 0)
	num = 0;
    }

  for (offset = num;
       offset < num + Hist_print && offset < history_length;
       offset++)
    {
      printf_filtered ("%5d  %s\n", history_base + offset,
		       (history_get (history_base + offset))->line);
    }

  /* The next command we want to display is the next one that we haven't
     displayed yet.  */
  num += Hist_print;

  /* If the user repeats this command with return, it should do what
     "show commands +" does.  This is unnecessary if arg is null,
     because "show commands +" is not useful after "show commands".  */
  if (from_tty && args)
    {
      args[0] = '+';
      args[1] = '\0';
    }
}

/* Called by do_setshow_command.  */
static void
set_history_size_command (char *args, int from_tty, struct cmd_list_element *c)
{
  /* Readline's history interface works with 'int', so it can only
     handle history sizes up to INT_MAX.  The command itself is
     uinteger, so UINT_MAX means "unlimited", but we only get that if
     the user does "set history size 0" -- "set history size <UINT_MAX>"
     throws out-of-range.  */
  if (history_size_setshow_var > INT_MAX
      && history_size_setshow_var != UINT_MAX)
    {
      unsigned int new_value = history_size_setshow_var;

      /* Restore previous value before throwing.  */
      if (history_is_stifled ())
	history_size_setshow_var = history_max_entries;
      else
	history_size_setshow_var = UINT_MAX;

      error (_("integer %u out of range"), new_value);
    }

  /* Commit the new value to readline's history.  */
  if (history_size_setshow_var == UINT_MAX)
    unstifle_history ();
  else
    stifle_history (history_size_setshow_var);
}

void
set_history (char *args, int from_tty)
{
  printf_unfiltered (_("\"set history\" must be followed "
		       "by the name of a history subcommand.\n"));
  help_list (sethistlist, "set history ", -1, gdb_stdout);
}

void
show_history (char *args, int from_tty)
{
  cmd_show_list (showhistlist, from_tty, "");
}

int info_verbose = 0;		/* Default verbose msgs off.  */

/* Called by do_setshow_command.  An elaborate joke.  */
void
set_verbose (char *args, int from_tty, struct cmd_list_element *c)
{
  const char *cmdname = "verbose";
  struct cmd_list_element *showcmd;

  showcmd = lookup_cmd_1 (&cmdname, showlist, NULL, 1);
  gdb_assert (showcmd != NULL && showcmd != CMD_LIST_AMBIGUOUS);

  if (info_verbose)
    {
      c->doc = "Set verbose printing of informational messages.";
      showcmd->doc = "Show verbose printing of informational messages.";
    }
  else
    {
      c->doc = "Set verbosity.";
      showcmd->doc = "Show verbosity.";
    }
}

/* Init the history buffer.  Note that we are called after the init file(s)
   have been read so that the user can change the history file via his
   .gdbinit file (for instance).  The GDBHISTFILE environment variable
   overrides all of this.  */

void
init_history (void)
{
  char *tmpenv;

  tmpenv = getenv ("HISTSIZE");
  if (tmpenv)
    {
      int var;

      var = atoi (tmpenv);
      if (var < 0)
	{
	  /* Prefer ending up with no history rather than overflowing
	     readline's history interface, which uses signed 'int'
	     everywhere.  */
	  var = 0;
	}

      history_size_setshow_var = var;
    }
  /* If the init file hasn't set a size yet, pick the default.  */
  else if (history_size_setshow_var == 0)
    history_size_setshow_var = 256;

  /* Note that unlike "set history size 0", "HISTSIZE=0" really sets
     the history size to 0...  */
  stifle_history (history_size_setshow_var);

  tmpenv = getenv ("GDBHISTFILE");
  if (tmpenv)
    history_filename = xstrdup (tmpenv);
  else if (!history_filename)
    {
      /* We include the current directory so that if the user changes
         directories the file written will be the same as the one
         that was read.  */
#ifdef __MSDOS__
      /* No leading dots in file names are allowed on MSDOS.  */
      history_filename = concat (current_directory, "/_gdb_history",
				 (char *)NULL);
#else
      history_filename = concat (current_directory, "/.gdb_history",
				 (char *)NULL);
#endif
    }
  read_history (history_filename);
}

static void
show_prompt (struct ui_file *file, int from_tty,
	     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Gdb's prompt is \"%s\".\n"), value);
}

static void
show_async_command_editing_p (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Editing of command lines as "
			    "they are typed is %s.\n"),
		    value);
}

static void
show_annotation_level (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Annotation_level is %s.\n"), value);
}

static void
show_exec_done_display_p (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Notification of completion for "
			    "asynchronous execution commands is %s.\n"),
		    value);
}

/* "set" command for the gdb_datadir configuration variable.  */

static void
set_gdb_datadir (char *args, int from_tty, struct cmd_list_element *c)
{
  observer_notify_gdb_datadir_changed ();
}

static void
set_history_filename (char *args, int from_tty, struct cmd_list_element *c)
{
  /* We include the current directory so that if the user changes
     directories the file written will be the same as the one
     that was read.  */
  if (!IS_ABSOLUTE_PATH (history_filename))
    history_filename = reconcat (history_filename, current_directory, "/", 
				 history_filename, (char *) NULL);
}

static void
init_main (void)
{
  /* Initialize the prompt to a simple "(gdb) " prompt or to whatever
     the DEFAULT_PROMPT is.  */
  set_prompt (DEFAULT_PROMPT);

  /* Set things up for annotation_level > 1, if the user ever decides
     to use it.  */
  async_annotation_suffix = "prompt";

  /* Set the important stuff up for command editing.  */
  command_editing_p = 1;
  history_expansion_p = 0;
  write_history_p = 0;

  /* Setup important stuff for command line editing.  */
  rl_completion_word_break_hook = gdb_completion_word_break_characters;
  rl_completion_entry_function = readline_line_completion_function;
  rl_completer_word_break_characters = default_word_break_characters ();
  rl_completer_quote_characters = get_gdb_completer_quote_characters ();
  rl_readline_name = "gdb";
  rl_terminal_name = getenv ("TERM");

  /* The name for this defun comes from Bash, where it originated.
     15 is Control-o, the same binding this function has in Bash.  */
  rl_add_defun ("operate-and-get-next", gdb_rl_operate_and_get_next, 15);

  add_setshow_string_cmd ("prompt", class_support,
			  &top_prompt,
			  _("Set gdb's prompt"),
			  _("Show gdb's prompt"),
			  NULL, NULL,
			  show_prompt,
			  &setlist, &showlist);

  add_com ("dont-repeat", class_support, dont_repeat_command, _("\
Don't repeat this command.\nPrimarily \
used inside of user-defined commands that should not be repeated when\n\
hitting return."));

  add_setshow_boolean_cmd ("editing", class_support,
			   &async_command_editing_p, _("\
Set editing of command lines as they are typed."), _("\
Show editing of command lines as they are typed."), _("\
Use \"on\" to enable the editing, and \"off\" to disable it.\n\
Without an argument, command line editing is enabled.  To edit, use\n\
EMACS-like or VI-like commands like control-P or ESC."),
			   set_async_editing_command,
			   show_async_command_editing_p,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("save", no_class, &write_history_p, _("\
Set saving of the history record on exit."), _("\
Show saving of the history record on exit."), _("\
Use \"on\" to enable the saving, and \"off\" to disable it.\n\
Without an argument, saving is enabled."),
			   NULL,
			   show_write_history_p,
			   &sethistlist, &showhistlist);

  add_setshow_uinteger_cmd ("size", no_class, &history_size_setshow_var, _("\
Set the size of the command history,"), _("\
Show the size of the command history,"), _("\
ie. the number of previous commands to keep a record of.\n\
If set to \"unlimited\", the number of commands kept in the history\n\
list is unlimited.  This defaults to the value of the environment\n\
variable \"HISTSIZE\", or to 256 if this variable is not set."),
			    set_history_size_command,
			    show_history_size,
			    &sethistlist, &showhistlist);

  add_setshow_filename_cmd ("filename", no_class, &history_filename, _("\
Set the filename in which to record the command history"), _("\
Show the filename in which to record the command history"), _("\
(the list of previous commands of which a record is kept)."),
			    set_history_filename,
			    show_history_filename,
			    &sethistlist, &showhistlist);

  add_setshow_boolean_cmd ("confirm", class_support, &confirm, _("\
Set whether to confirm potentially dangerous operations."), _("\
Show whether to confirm potentially dangerous operations."), NULL,
			   NULL,
			   show_confirm,
			   &setlist, &showlist);

  add_setshow_zinteger_cmd ("annotate", class_obscure, &annotation_level, _("\
Set annotation_level."), _("\
Show annotation_level."), _("\
0 == normal;     1 == fullname (for use when running under emacs)\n\
2 == output annotated suitably for use by programs that control GDB."),
			    NULL,
			    show_annotation_level,
			    &setlist, &showlist);

  add_setshow_boolean_cmd ("exec-done-display", class_support,
			   &exec_done_display_p, _("\
Set notification of completion for asynchronous execution commands."), _("\
Show notification of completion for asynchronous execution commands."), _("\
Use \"on\" to enable the notification, and \"off\" to disable it."),
			   NULL,
			   show_exec_done_display_p,
			   &setlist, &showlist);

  add_setshow_filename_cmd ("data-directory", class_maintenance,
                           &gdb_datadir, _("Set GDB's data directory."),
                           _("Show GDB's data directory."),
                           _("\
When set, GDB uses the specified path to search for data files."),
                           set_gdb_datadir, NULL,
                           &setlist,
                           &showlist);
}

void
gdb_init (char *argv0)
{
  if (pre_init_ui_hook)
    pre_init_ui_hook ();

  /* Run the init function of each source file.  */

#ifdef __MSDOS__
  /* Make sure we return to the original directory upon exit, come
     what may, since the OS doesn't do that for us.  */
  make_final_cleanup (do_chdir_cleanup, xstrdup (current_directory));
#endif

  init_cmd_lists ();	    /* This needs to be done first.  */
  initialize_targets ();    /* Setup target_terminal macros for utils.c.  */
  initialize_utils ();	    /* Make errors and warnings possible.  */

  /* Here is where we call all the _initialize_foo routines.  */
  initialize_all_files ();

  /* This creates the current_program_space.  Do this after all the
     _initialize_foo routines have had a chance to install their
     per-sspace data keys.  Also do this before
     initialize_current_architecture is called, because it accesses
     exec_bfd of the current program space.  */
  initialize_progspace ();
  initialize_inferiors ();
  initialize_current_architecture ();
  init_cli_cmds();
  initialize_event_loop ();
  init_main ();			/* But that omits this file!  Do it now.  */

  initialize_stdin_serial ();

  async_init_signals ();

  /* We need a default language for parsing expressions, so simple
     things like "set width 0" won't fail if no language is explicitly
     set in a config file or implicitly set by reading an executable
     during startup.  */
  set_language (language_c);
  expected_language = current_language;	/* Don't warn about the change.  */

  /* Allow another UI to initialize.  If the UI fails to initialize,
     and it wants GDB to revert to the CLI, it should clear
     deprecated_init_ui_hook.  */
  if (deprecated_init_ui_hook)
    deprecated_init_ui_hook (argv0);

#ifdef HAVE_PYTHON
  /* Python initialization can require various commands to be
     installed.  For example "info pretty-printer" needs the "info"
     prefix to be installed.  Keep things simple and just do final
     python initialization here.  */
  finish_python_initialization ();
#endif
}
