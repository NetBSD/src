/* Top level stuff for GDB, the GNU debugger.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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
#include "infrun.h"
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
#include "main.h"
#include "event-loop.h"
#include "gdbthread.h"
#include "extension.h"
#include "interps.h"
#include "observer.h"
#include "maint.h"
#include "filenames.h"
#include "frame.h"
#include "buffer.h"
#include "gdb_select.h"

/* readline include files.  */
#include "readline/readline.h"
#include "readline/history.h"

/* readline defines this.  */
#undef savestring

#include <sys/types.h>

#include "event-top.h"
#include <sys/stat.h>
#include <ctype.h>
#include "ui-out.h"
#include "cli-out.h"
#include "tracepoint.h"
#include "inf-loop.h"

#if defined(TUI)
# include "tui/tui.h"
#endif

#ifndef O_NOCTTY
# define O_NOCTTY 0
#endif

extern void initialize_all_files (void);

#define PROMPT(X) the_prompts.prompt_stack[the_prompts.top + X].prompt
#define PREFIX(X) the_prompts.prompt_stack[the_prompts.top + X].prefix
#define SUFFIX(X) the_prompts.prompt_stack[the_prompts.top + X].suffix

/* Default command line prompt.  This is overriden in some configs.  */

#ifndef DEFAULT_PROMPT
#define DEFAULT_PROMPT	"(gdb) "
#endif

/* Generate a function that exports a pointer to a field of the
   current UI.  */

#define gen_ret_current_ui_field_ptr(type, name)	\
type *							\
current_ui_## name ## _ptr (void)			\
{							\
  return &current_ui->m_ ## name;		\
}

gen_ret_current_ui_field_ptr (struct ui_file *, gdb_stdout)
gen_ret_current_ui_field_ptr (struct ui_file *, gdb_stdin)
gen_ret_current_ui_field_ptr (struct ui_file *, gdb_stderr)
gen_ret_current_ui_field_ptr (struct ui_file *, gdb_stdlog)
gen_ret_current_ui_field_ptr (struct ui_out *, current_uiout)

/* Initialization file name for gdb.  This is host-dependent.  */

const char gdbinit[] = GDBINIT;

int inhibit_gdbinit = 0;

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

/* Flag to indicate whether a user defined command is currently running.  */

int in_user_command;

/* Current working directory.  */

char *current_directory;

/* The directory name is actually stored here (usually).  */
char gdb_dirbuf[1024];

/* The last command line executed on the console.  Used for command
   repetitions.  */
char *saved_command_line;

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
char *(*deprecated_readline_hook) (const char *);
void (*deprecated_readline_end_hook) (void);

/* Called as appropriate to notify the interface that we have attached
   to or detached from an already running process.  */

void (*deprecated_attach_hook) (void);
void (*deprecated_detach_hook) (void);

/* Called during long calculations to allow GUI to repair window
   damage, and to check for stop buttons, etc...  */

void (*deprecated_interactive_hook) (void);

/* Called when going to wait for the target.  Usually allows the GUI
   to run while waiting for target events.  */

ptid_t (*deprecated_target_wait_hook) (ptid_t ptid,
				       struct target_waitstatus *status,
				       int options);

/* Used by UI as a wrapper around command execution.  May do various
   things like enabling/disabling buttons, etc...  */

void (*deprecated_call_command_hook) (struct cmd_list_element * c, 
				      char *cmd, int from_tty);

/* Called when the current thread changes.  Argument is thread id.  */

void (*deprecated_context_hook) (int id);

/* The highest UI number ever assigned.  */
static int highest_ui_num;

/* See top.h.  */

struct ui *
new_ui (FILE *instream, FILE *outstream, FILE *errstream)
{
  struct ui *ui;

  ui = XCNEW (struct ui);

  ui->num = ++highest_ui_num;
  ui->stdin_stream = instream;
  ui->instream = instream;
  ui->outstream = outstream;
  ui->errstream = errstream;

  ui->input_fd = fileno (ui->instream);

  ui->input_interactive_p = ISATTY (ui->instream);

  ui->m_gdb_stdin = stdio_fileopen (ui->instream);
  ui->m_gdb_stdout = stdio_fileopen (ui->outstream);
  ui->m_gdb_stderr = stderr_fileopen (ui->errstream);
  ui->m_gdb_stdlog = ui->m_gdb_stderr;

  ui->prompt_state = PROMPT_NEEDED;

  if (ui_list == NULL)
    ui_list = ui;
  else
    {
      struct ui *last;

      for (last = ui_list; last->next != NULL; last = last->next)
	;
      last->next = ui;
    }

  return ui;
}

static void
free_ui (struct ui *ui)
{
  ui_file_delete (ui->m_gdb_stdin);
  ui_file_delete (ui->m_gdb_stdout);
  ui_file_delete (ui->m_gdb_stderr);

  xfree (ui);
}

void
delete_ui (struct ui *todel)
{
  struct ui *ui, *uiprev;

  uiprev = NULL;

  for (ui = ui_list; ui != NULL; uiprev = ui, ui = ui->next)
    if (ui == todel)
      break;

  gdb_assert (ui != NULL);

  if (uiprev != NULL)
    uiprev->next = ui->next;
  else
    ui_list = ui->next;

  free_ui (ui);
}

/* Cleanup that deletes a UI.  */

static void
delete_ui_cleanup (void *void_ui)
{
  struct ui *ui = (struct ui *) void_ui;

  delete_ui (ui);
}

/* See top.h.  */

struct cleanup *
make_delete_ui_cleanup (struct ui *ui)
{
  return make_cleanup (delete_ui_cleanup, ui);
}

/* Open file named NAME for read/write, making sure not to make it the
   controlling terminal.  */

static FILE *
open_terminal_stream (const char *name)
{
  int fd;

  fd = open (name, O_RDWR | O_NOCTTY);
  if (fd < 0)
    perror_with_name  (_("opening terminal failed"));

  return fdopen (fd, "w+");
}

/* Implementation of the "new-ui" command.  */

static void
new_ui_command (char *args, int from_tty)
{
  struct ui *ui;
  struct interp *interp;
  FILE *stream[3] = { NULL, NULL, NULL };
  int i;
  int res;
  int argc;
  char **argv;
  const char *interpreter_name;
  const char *tty_name;
  struct cleanup *success_chain;
  struct cleanup *failure_chain;

  dont_repeat ();

  argv = gdb_buildargv (args);
  success_chain = make_cleanup_freeargv (argv);
  argc = countargv (argv);

  if (argc < 2)
    error (_("usage: new-ui <interpreter> <tty>"));

  interpreter_name = argv[0];
  tty_name = argv[1];

  make_cleanup_restore_current_ui ();

  failure_chain = make_cleanup (null_cleanup, NULL);

  /* Open specified terminal, once for each of
     stdin/stdout/stderr.  */
  for (i = 0; i < 3; i++)
    {
      stream[i] = open_terminal_stream (tty_name);
      make_cleanup_fclose (stream[i]);
    }

  ui = new_ui (stream[0], stream[1], stream[2]);
  make_cleanup (delete_ui_cleanup, ui);

  ui->async = 1;

  current_ui = ui;

  set_top_level_interpreter (interpreter_name);

  interp_pre_command_loop (top_level_interpreter ());

  discard_cleanups (failure_chain);

  /* This restores the previous UI and frees argv.  */
  do_cleanups (success_chain);

  printf_unfiltered ("New UI allocated\n");
}

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
  struct ui *ui = current_ui;

  /* Restore the previous input stream.  */
  ui->instream = (FILE *) stream;
}

/* Read commands from STREAM.  */
void
read_command_file (FILE *stream)
{
  struct ui *ui = current_ui;
  struct cleanup *cleanups;

  cleanups = make_cleanup (do_restore_instream_cleanup, ui->instream);
  ui->instream = stream;

  /* Read commands from `instream' and execute them until end of file
     or error reading instream.  */

  while (ui->instream != NULL && !feof (ui->instream))
    {
      char *command;

      /* Get a command-line.  This calls the readline package.  */
      command = command_line_input (NULL, 0, NULL);
      if (command == NULL)
	break;
      command_handler (command);
    }

  do_cleanups (cleanups);
}

void (*pre_init_ui_hook) (void);

#ifdef __MSDOS__
static void
do_chdir_cleanup (void *old_dir)
{
  chdir ((const char *) old_dir);
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
  struct frame_info *frame;

  /* First make sure that a new frame has been selected, in case the
     command or the hooks changed the program state.  */
  frame = deprecated_safe_get_selected_frame ();
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

      flang = get_frame_language (frame);
      if (!warned
	  && flang != language_unknown
	  && flang != current_language->la_language)
	{
	  printf_filtered ("%s\n", lang_frame_mismatch_warn);
	  warned = 1;
	}
    }
}

/* See top.h.  */

void
wait_sync_command_done (void)
{
  /* Processing events may change the current UI.  */
  struct cleanup *old_chain = make_cleanup_restore_current_ui ();
  struct ui *ui = current_ui;

  while (gdb_do_one_event () >= 0)
    if (ui->prompt_state != PROMPT_BLOCKED)
      break;

  do_cleanups (old_chain);
}

/* See top.h.  */

void
maybe_wait_sync_command_done (int was_sync)
{
  /* If the interpreter is in sync mode (we're running a user
     command's list, running command hooks or similars), and we
     just ran a synchronous command that started the target, wait
     for that command to end.  */
  if (!current_ui->async
      && !was_sync
      && current_ui->prompt_state == PROMPT_BLOCKED)
    wait_sync_command_done ();
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
      int was_sync = current_ui->prompt_state == PROMPT_BLOCKED;

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

      if (c->deprecated_warn_user)
	deprecated_cmd_warning (line);

      /* c->user_commands would be NULL in the case of a python command.  */
      if (c->theclass == class_user && c->user_commands)
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

      maybe_wait_sync_command_done (was_sync);

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

  make_cleanup_restore_integer (&current_ui->async);
  current_ui->async = 0;

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


/* When nonzero, cause dont_repeat to do nothing.  This should only be
   set via prevent_dont_repeat.  */

static int suppress_dont_repeat = 0;

/* Commands call this if they do not want to be repeated by null lines.  */

void
dont_repeat (void)
{
  struct ui *ui = current_ui;

  if (suppress_dont_repeat || server_command)
    return;

  /* If we aren't reading from standard input, we are saving the last
     thing read from stdin in line and don't want to delete it.  Null
     lines won't repeat here in any case.  */
  if (ui->instream == ui->stdin_stream)
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

   It prints PROMPT once at the start.
   Action is compatible with "readline", e.g. space for the result is
   malloc'd and should be freed by the caller.

   A NULL return means end of file.  */

static char *
gdb_readline_no_editing (const char *prompt)
{
  struct buffer line_buffer;
  struct ui *ui = current_ui;
  /* Read from stdin if we are executing a user defined command.  This
     is the right thing for prompt_for_continue, at least.  */
  FILE *stream = ui->instream != NULL ? ui->instream : stdin;
  int fd = fileno (stream);

  buffer_init (&line_buffer);

  if (prompt != NULL)
    {
      /* Don't use a _filtered function here.  It causes the assumed
         character position to be off, since the newline we read from
         the user is not accounted for.  */
      fputs_unfiltered (prompt, gdb_stdout);
      gdb_flush (gdb_stdout);
    }

  while (1)
    {
      int c;
      int numfds;
      fd_set readfds;

      QUIT;

      /* Wait until at least one byte of data is available.  Control-C
	 can interrupt interruptible_select, but not fgetc.  */
      FD_ZERO (&readfds);
      FD_SET (fd, &readfds);
      if (interruptible_select (fd + 1, &readfds, NULL, NULL, NULL) == -1)
	{
	  if (errno == EINTR)
	    {
	      /* If this was ctrl-c, the QUIT above handles it.  */
	      continue;
	    }
	  perror_with_name (("select"));
	}

      c = fgetc (stream);

      if (c == EOF)
	{
	  if (line_buffer.used_size > 0)
	    /* The last line does not end with a newline.  Return it, and
	       if we are called again fgetc will still return EOF and
	       we'll return NULL then.  */
	    break;
	  xfree (buffer_finish (&line_buffer));
	  return NULL;
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
  return buffer_finish (&line_buffer);
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
   command.  The value -1 means unlimited, and -2 means undefined.  */
static int history_size_setshow_var = -2;

static void
show_history_size (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("The size of the command history is %s.\n"),
		    value);
}

/* Variable associated with the "history remove-duplicates" option.
   The value -1 means unlimited.  */
static int history_remove_duplicates = 0;

static void
show_history_remove_duplicates (struct ui_file *file, int from_tty,
				struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("The number of history entries to look back at for "
		      "duplicates is %s.\n"),
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


/* See top.h.  */

int
gdb_in_secondary_prompt_p (struct ui *ui)
{
  return ui->secondary_prompt_depth > 0;
}


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
     are enabled, and readline's state getting out of sync.  We'll
     reinstall the callback handler, which puts the terminal in raw
     mode (or in readline lingo, in prepped state), when we're next
     ready to process user input, either in display_gdb_prompt, or if
     we're handling an asynchronous target event and running in the
     background, just before returning to the event loop to process
     further input (or more target events).  */
  if (current_ui->command_editing)
    gdb_rl_callback_handler_remove ();
}

struct gdb_readline_wrapper_cleanup
  {
    void (*handler_orig) (char *);
    int already_prompted_orig;

    /* Whether the target was async.  */
    int target_is_async_orig;
  };

static void
gdb_readline_wrapper_cleanup (void *arg)
{
  struct ui *ui = current_ui;
  struct gdb_readline_wrapper_cleanup *cleanup
    = (struct gdb_readline_wrapper_cleanup *) arg;

  if (ui->command_editing)
    rl_already_prompted = cleanup->already_prompted_orig;

  gdb_assert (ui->input_handler == gdb_readline_wrapper_line);
  ui->input_handler = cleanup->handler_orig;

  /* Don't restore our input handler in readline yet.  That would make
     readline prep the terminal (putting it in raw mode), while the
     line we just read may trigger execution of a command that expects
     the terminal in the default cooked/canonical mode, such as e.g.,
     running Python's interactive online help utility.  See
     gdb_readline_wrapper_line for when we'll reinstall it.  */

  gdb_readline_wrapper_result = NULL;
  gdb_readline_wrapper_done = 0;
  ui->secondary_prompt_depth--;
  gdb_assert (ui->secondary_prompt_depth >= 0);

  after_char_processing_hook = saved_after_char_processing_hook;
  saved_after_char_processing_hook = NULL;

  if (cleanup->target_is_async_orig)
    target_async (1);

  xfree (cleanup);
}

char *
gdb_readline_wrapper (const char *prompt)
{
  struct ui *ui = current_ui;
  struct cleanup *back_to;
  struct gdb_readline_wrapper_cleanup *cleanup;
  char *retval;

  cleanup = XNEW (struct gdb_readline_wrapper_cleanup);
  cleanup->handler_orig = ui->input_handler;
  ui->input_handler = gdb_readline_wrapper_line;

  if (ui->command_editing)
    cleanup->already_prompted_orig = rl_already_prompted;
  else
    cleanup->already_prompted_orig = 0;

  cleanup->target_is_async_orig = target_is_async_p ();

  ui->secondary_prompt_depth++;
  back_to = make_cleanup (gdb_readline_wrapper_cleanup, cleanup);

  /* Processing events may change the current UI.  */
  make_cleanup_restore_current_ui ();

  if (cleanup->target_is_async_orig)
    target_async (0);

  /* Display our prompt and prevent double prompt display.  */
  display_gdb_prompt (prompt);
  if (ui->command_editing)
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

/* Number of user commands executed during this session.  */

static int command_count = 0;

/* Add the user command COMMAND to the input history list.  */

void
gdb_add_history (const char *command)
{
  command_count++;

  if (history_remove_duplicates != 0)
    {
      int lookbehind;
      int lookbehind_threshold;

      /* The lookbehind threshold for finding a duplicate history entry is
	 bounded by command_count because we can't meaningfully delete
	 history entries that are already stored in the history file since
	 the history file is appended to.  */
      if (history_remove_duplicates == -1
	  || history_remove_duplicates > command_count)
	lookbehind_threshold = command_count;
      else
	lookbehind_threshold = history_remove_duplicates;

      using_history ();
      for (lookbehind = 0; lookbehind < lookbehind_threshold; lookbehind++)
	{
	  HIST_ENTRY *temp = previous_history ();

	  if (temp == NULL)
	    break;

	  if (strcmp (temp->line, command) == 0)
	    {
	      HIST_ENTRY *prev = remove_history (where_history ());
	      command_count--;
	      free_history_entry (prev);
	      break;
	    }
	}
      using_history ();
    }

  add_history (command);
}

/* Safely append new history entries to the history file in a corruption-free
   way using an intermediate local history file.  */

static void
gdb_safe_append_history (void)
{
  int ret, saved_errno;
  char *local_history_filename;
  struct cleanup *old_chain;

  local_history_filename
    = xstrprintf ("%s-gdb%d~", history_filename, getpid ());
  old_chain = make_cleanup (xfree, local_history_filename);

  ret = rename (history_filename, local_history_filename);
  saved_errno = errno;
  if (ret < 0 && saved_errno != ENOENT)
    {
      warning (_("Could not rename %s to %s: %s"),
	       history_filename, local_history_filename,
	       safe_strerror (saved_errno));
    }
  else
    {
      if (ret < 0)
	{
	  /* If the rename failed with ENOENT then either the global history
	     file never existed in the first place or another GDB process is
	     currently appending to it (and has thus temporarily renamed it).
	     Since we can't distinguish between these two cases, we have to
	     conservatively assume the first case and therefore must write out
	     (not append) our known history to our local history file and try
	     to move it back anyway.  Otherwise a global history file would
	     never get created!  */
	   gdb_assert (saved_errno == ENOENT);
	   write_history (local_history_filename);
	}
      else
	{
	  append_history (command_count, local_history_filename);
	  if (history_is_stifled ())
	    history_truncate_file (local_history_filename, history_max_entries);
	}

      ret = rename (local_history_filename, history_filename);
      saved_errno = errno;
      if (ret < 0 && saved_errno != EEXIST)
        warning (_("Could not rename %s to %s: %s"),
		 local_history_filename, history_filename,
		 safe_strerror (saved_errno));
    }

  do_cleanups (old_chain);
}

/* Read one line from the command input stream `instream' into a local
   static buffer.  The buffer is made bigger as necessary.  Returns
   the address of the start of the line.

   NULL is returned for end of file.

   *If* input is from an interactive stream (stdin), the line read is
   copied into the global 'saved_command_line' so that it can be
   repeated.

   This routine either uses fancy command line editing or simple input
   as the user has requested.  */

char *
command_line_input (const char *prompt_arg, int repeat, char *annotation_suffix)
{
  static struct buffer cmd_line_buffer;
  static int cmd_line_buffer_initialized;
  struct ui *ui = current_ui;
  const char *prompt = prompt_arg;
  char *cmd;
  int from_tty = ui->instream == ui->stdin_stream;

  /* The annotation suffix must be non-NULL.  */
  if (annotation_suffix == NULL)
    annotation_suffix = "";

  if (from_tty && annotation_level > 1)
    {
      char *local_prompt;

      local_prompt
	= (char *) alloca ((prompt == NULL ? 0 : strlen (prompt))
			   + strlen (annotation_suffix) + 40);
      if (prompt == NULL)
	local_prompt[0] = '\0';
      else
	strcpy (local_prompt, prompt);
      strcat (local_prompt, "\n\032\032");
      strcat (local_prompt, annotation_suffix);
      strcat (local_prompt, "\n");

      prompt = local_prompt;
    }

  if (!cmd_line_buffer_initialized)
    {
      buffer_init (&cmd_line_buffer);
      cmd_line_buffer_initialized = 1;
    }

  /* Starting a new command line.  */
  cmd_line_buffer.used_size = 0;

#ifdef STOP_SIGNAL
  if (job_control)
    signal (STOP_SIGNAL, handle_stop_sig);
#endif

  while (1)
    {
      char *rl;

      /* Make sure that all output has been output.  Some machines may
         let you get away with leaving out some of the gdb_flush, but
         not all.  */
      wrap_here ("");
      gdb_flush (gdb_stdout);
      gdb_flush (gdb_stderr);

      if (source_file_name != NULL)
	++source_line_number;

      if (from_tty && annotation_level > 1)
	{
	  puts_unfiltered ("\n\032\032pre-");
	  puts_unfiltered (annotation_suffix);
	  puts_unfiltered ("\n");
	}

      /* Don't use fancy stuff if not talking to stdin.  */
      if (deprecated_readline_hook
	  && from_tty
	  && input_interactive_p (current_ui))
	{
	  rl = (*deprecated_readline_hook) (prompt);
	}
      else if (command_editing_p
	       && from_tty
	       && input_interactive_p (current_ui))
	{
	  rl = gdb_readline_wrapper (prompt);
	}
      else
	{
	  rl = gdb_readline_no_editing (prompt);
	}

      cmd = handle_line_of_input (&cmd_line_buffer, rl,
				  repeat, annotation_suffix);
      if (cmd == (char *) EOF)
	{
	  cmd = NULL;
	  break;
	}
      if (cmd != NULL)
	break;

      prompt = NULL;
    }

#ifdef STOP_SIGNAL
  if (job_control)
    signal (STOP_SIGNAL, SIG_DFL);
#endif

  return cmd;
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
		    "Copyright (C) 2016 Free Software Foundation, Inc.\n");

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
#if HAVE_GUILE
  fprintf_filtered (stream, _("\
             --with-guile\n\
"));
#else
  fprintf_filtered (stream, _("\
             --without-guile\n\
"));
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
  struct qt_args *qt = (struct qt_args *) args;
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
  struct ui_file *stb = (struct ui_file *) arg;

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

/* Prepare to exit GDB cleanly by undoing any changes made to the
   terminal so that we leave the terminal in the state we acquired it.  */

static void
undo_terminal_modifications_before_exit (void)
{
  struct ui *saved_top_level = current_ui;

  target_terminal_ours ();

  current_ui = main_ui;

#if defined(TUI)
  tui_disable ();
#endif
  gdb_disable_readline ();

  current_ui = saved_top_level;
}


/* Quit without asking for confirmation.  */

void
quit_force (char *args, int from_tty)
{
  int exit_code = 0;
  struct qt_args qt;

  undo_terminal_modifications_before_exit ();

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

  /* We want to handle any quit errors and exit regardless.  */

  /* Get out of tfind mode, and kill or detach all inferiors.  */
  TRY
    {
      disconnect_tracing ();
      iterate_over_inferiors (kill_or_detach, &qt);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      exception_print (gdb_stderr, ex);
    }
  END_CATCH

  /* Give all pushed targets a chance to do minimal cleanup, and pop
     them all out.  */
  TRY
    {
      pop_all_targets ();
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      exception_print (gdb_stderr, ex);
    }
  END_CATCH

  /* Save the history information if it is appropriate to do so.  */
  TRY
    {
      if (write_history_p && history_filename)
	{
	  struct ui *ui;
	  int save = 0;

	  /* History is currently shared between all UIs.  If there's
	     any UI with a terminal, save history.  */
	  ALL_UIS (ui)
	    {
	      if (input_interactive_p (ui))
		{
		  save = 1;
		  break;
		}
	    }

	  if (save)
	    gdb_safe_append_history ();
	}
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      exception_print (gdb_stderr, ex);
    }
  END_CATCH

  /* Do any final cleanups before exiting.  */
  TRY
    {
      do_final_cleanups (all_cleanups ());
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      exception_print (gdb_stderr, ex);
    }
  END_CATCH

  exit (exit_code);
}

/* The value of the "interactive-mode" setting.  */
static enum auto_boolean interactive_mode = AUTO_BOOLEAN_AUTO;

/* Implement the "show interactive-mode" option.  */

static void
show_interactive_mode (struct ui_file *file, int from_tty,
                       struct cmd_list_element *c,
                       const char *value)
{
  if (interactive_mode == AUTO_BOOLEAN_AUTO)
    fprintf_filtered (file, "Debugger's interactive mode "
		            "is %s (currently %s).\n",
                      value, input_interactive_p (current_ui) ? "on" : "off");
  else
    fprintf_filtered (file, "Debugger's interactive mode is %s.\n", value);
}

/* Returns whether GDB is running on an interactive terminal.  */

int
input_interactive_p (struct ui *ui)
{
  if (batch_flag)
    return 0;

  if (interactive_mode != AUTO_BOOLEAN_AUTO)
    return interactive_mode == AUTO_BOOLEAN_TRUE;

  return ui->input_interactive_p;
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

/* Update the size of our command history file to HISTORY_SIZE.

   A HISTORY_SIZE of -1 stands for unlimited.  */

static void
set_readline_history_size (int history_size)
{
  gdb_assert (history_size >= -1);

  if (history_size == -1)
    unstifle_history ();
  else
    stifle_history (history_size);
}

/* Called by do_setshow_command.  */
static void
set_history_size_command (char *args, int from_tty, struct cmd_list_element *c)
{
  set_readline_history_size (history_size_setshow_var);
}

void
set_history (char *args, int from_tty)
{
  printf_unfiltered (_("\"set history\" must be followed "
		       "by the name of a history subcommand.\n"));
  help_list (sethistlist, "set history ", all_commands, gdb_stdout);
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

  tmpenv = getenv ("GDBHISTSIZE");
  if (tmpenv)
    {
      long var;
      int saved_errno;
      char *endptr;

      tmpenv = skip_spaces (tmpenv);
      errno = 0;
      var = strtol (tmpenv, &endptr, 10);
      saved_errno = errno;
      endptr = skip_spaces (endptr);

      /* If GDBHISTSIZE is non-numeric then ignore it.  If GDBHISTSIZE is the
	 empty string, a negative number or a huge positive number (larger than
	 INT_MAX) then set the history size to unlimited.  Otherwise set our
	 history size to the number we have read.  This behavior is consistent
	 with how bash handles HISTSIZE.  */
      if (*endptr != '\0')
	;
      else if (*tmpenv == '\0'
	       || var < 0
	       || var > INT_MAX
	       /* On targets where INT_MAX == LONG_MAX, we have to look at
		  errno after calling strtol to distinguish between a value that
		  is exactly INT_MAX and an overflowing value that was clamped
		  to INT_MAX.  */
	       || (var == INT_MAX && saved_errno == ERANGE))
	history_size_setshow_var = -1;
      else
	history_size_setshow_var = var;
    }

  /* If neither the init file nor GDBHISTSIZE has set a size yet, pick the
     default.  */
  if (history_size_setshow_var == -2)
    history_size_setshow_var = 256;

  set_readline_history_size (history_size_setshow_var);

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

/* "set editing" command.  */

static void
set_editing (char *args, int from_tty, struct cmd_list_element *c)
{
  change_line_handler (set_editing_cmd_var);
  /* Update the control variable so that MI's =cmd-param-changed event
     shows the correct value. */
  set_editing_cmd_var = current_ui->command_editing;
}

static void
show_editing (struct ui_file *file, int from_tty,
	      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Editing of command lines as "
			    "they are typed is %s.\n"),
		    current_ui->command_editing ? _("on") : _("off"));
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

/* New values of the "data-directory" parameter are staged here.  */
static char *staged_gdb_datadir;

/* "set" command for the gdb_datadir configuration variable.  */

static void
set_gdb_datadir (char *args, int from_tty, struct cmd_list_element *c)
{
  set_gdb_data_directory (staged_gdb_datadir);
  observer_notify_gdb_datadir_changed ();
}

/* "show" command for the gdb_datadir configuration variable.  */

static void
show_gdb_datadir (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("GDB's data directory is \"%s\".\n"),
		    gdb_datadir);
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
  struct cmd_list_element *c;

  /* Initialize the prompt to a simple "(gdb) " prompt or to whatever
     the DEFAULT_PROMPT is.  */
  set_prompt (DEFAULT_PROMPT);

  /* Set the important stuff up for command editing.  */
  command_editing_p = 1;
  history_expansion_p = 0;
  write_history_p = 0;

  /* Setup important stuff for command line editing.  */
  rl_completion_word_break_hook = gdb_completion_word_break_characters;
  rl_completion_entry_function = readline_line_completion_function;
  rl_completer_word_break_characters = default_word_break_characters ();
  rl_completer_quote_characters = get_gdb_completer_quote_characters ();
  rl_completion_display_matches_hook = cli_display_match_list;
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
			   &set_editing_cmd_var, _("\
Set editing of command lines as they are typed."), _("\
Show editing of command lines as they are typed."), _("\
Use \"on\" to enable the editing, and \"off\" to disable it.\n\
Without an argument, command line editing is enabled.  To edit, use\n\
EMACS-like or VI-like commands like control-P or ESC."),
			   set_editing,
			   show_editing,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("save", no_class, &write_history_p, _("\
Set saving of the history record on exit."), _("\
Show saving of the history record on exit."), _("\
Use \"on\" to enable the saving, and \"off\" to disable it.\n\
Without an argument, saving is enabled."),
			   NULL,
			   show_write_history_p,
			   &sethistlist, &showhistlist);

  add_setshow_zuinteger_unlimited_cmd ("size", no_class,
				       &history_size_setshow_var, _("\
Set the size of the command history,"), _("\
Show the size of the command history,"), _("\
ie. the number of previous commands to keep a record of.\n\
If set to \"unlimited\", the number of commands kept in the history\n\
list is unlimited.  This defaults to the value of the environment\n\
variable \"GDBHISTSIZE\", or to 256 if this variable is not set."),
			    set_history_size_command,
			    show_history_size,
			    &sethistlist, &showhistlist);

  add_setshow_zuinteger_unlimited_cmd ("remove-duplicates", no_class,
				       &history_remove_duplicates, _("\
Set how far back in history to look for and remove duplicate entries."), _("\
Show how far back in history to look for and remove duplicate entries."), _("\
If set to a nonzero value N, GDB will look back at the last N history entries\n\
and remove the first history entry that is a duplicate of the most recent\n\
entry, each time a new history entry is added.\n\
If set to \"unlimited\", this lookbehind is unbounded.\n\
Only history entries added during this session are considered for removal.\n\
If set to 0, removal of duplicate history entries is disabled.\n\
By default this option is set to 0."),
			   NULL,
			   show_history_remove_duplicates,
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
                           &staged_gdb_datadir, _("Set GDB's data directory."),
                           _("Show GDB's data directory."),
                           _("\
When set, GDB uses the specified path to search for data files."),
                           set_gdb_datadir, show_gdb_datadir,
                           &setlist,
                           &showlist);

  add_setshow_auto_boolean_cmd ("interactive-mode", class_support,
                                &interactive_mode, _("\
Set whether GDB's standard input is a terminal."), _("\
Show whether GDB's standard input is a terminal."), _("\
If on, GDB assumes that standard input is a terminal.  In practice, it\n\
means that GDB should wait for the user to answer queries associated to\n\
commands entered at the command prompt.  If off, GDB assumes that standard\n\
input is not a terminal, and uses the default answer to all queries.\n\
If auto (the default), determine which mode to use based on the standard\n\
input settings."),
                        NULL,
                        show_interactive_mode,
                        &setlist, &showlist);

  c = add_cmd ("new-ui", class_support, new_ui_command, _("\
Create a new UI.  It takes two arguments:\n\
The first argument is the name of the interpreter to run.\n\
The second argument is the terminal the UI runs on.\n"), &cmdlist);
  set_cmd_completer (c, interpreter_completer);
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

  init_page_info ();

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
  init_main ();			/* But that omits this file!  Do it now.  */

  initialize_stdin_serial ();

  /* Take a snapshot of our tty state before readline/ncurses have had a chance
     to alter it.  */
  set_initial_gdb_ttystate ();

  async_init_signals ();

  /* We need a default language for parsing expressions, so simple
     things like "set width 0" won't fail if no language is explicitly
     set in a config file or implicitly set by reading an executable
     during startup.  */
  set_language (language_c);
  expected_language = current_language;	/* Don't warn about the change.  */

  /* Python initialization, for example, can require various commands to be
     installed.  For example "info pretty-printer" needs the "info"
     prefix to be installed.  Keep things simple and just do final
     script initialization here.  */
  finish_ext_lang_initialization ();
}
