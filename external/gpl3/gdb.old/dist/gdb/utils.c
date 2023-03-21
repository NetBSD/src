/* General utility routines for GDB, the GNU debugger.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "gdbsupport/gdb_wait.h"
#include "event-top.h"
#include "gdbthread.h"
#include "fnmatch.h"
#include "gdb_bfd.h"
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#ifdef TUI
#include "tui/tui.h"		/* For tui_get_command_dimension.   */
#endif

#ifdef __GO32__
#include <pc.h>
#endif

#include <signal.h>
#include "gdbcmd.h"
#include "serial.h"
#include "bfd.h"
#include "target.h"
#include "gdb-demangle.h"
#include "expression.h"
#include "language.h"
#include "charset.h"
#include "annotate.h"
#include "filenames.h"
#include "symfile.h"
#include "gdb_obstack.h"
#include "gdbcore.h"
#include "top.h"
#include "main.h"
#include "solist.h"

#include "inferior.h"		/* for signed_pointer_to_address */

#include "gdb_curses.h"

#include "readline/readline.h"

#include <chrono>

#include "interps.h"
#include "gdb_regex.h"
#include "gdbsupport/job-control.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/gdb_optional.h"
#include "cp-support.h"
#include <algorithm>
#include "gdbsupport/pathstuff.h"
#include "cli/cli-style.h"
#include "gdbsupport/scope-exit.h"
#include "gdbarch.h"
#include "cli-out.h"
#include "gdbsupport/gdb-safe-ctype.h"

void (*deprecated_error_begin_hook) (void);

/* Prototypes for local functions */

static void vfprintf_maybe_filtered (struct ui_file *, const char *,
				     va_list, bool, bool)
  ATTRIBUTE_PRINTF (2, 0);

static void fputs_maybe_filtered (const char *, struct ui_file *, int);

static void prompt_for_continue (void);

static void set_screen_size (void);
static void set_width (void);

/* Time spent in prompt_for_continue in the currently executing command
   waiting for user to respond.
   Initialized in make_command_stats_cleanup.
   Modified in prompt_for_continue and defaulted_query.
   Used in report_command_stats.  */

static std::chrono::steady_clock::duration prompt_for_continue_wait_time;

/* A flag indicating whether to timestamp debugging messages.  */

static bool debug_timestamp = false;

/* True means that strings with character values >0x7F should be printed
   as octal escapes.  False means just print the value (e.g. it's an
   international character, and the terminal or window can cope.)  */

bool sevenbit_strings = false;
static void
show_sevenbit_strings (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Printing of 8-bit characters "
			    "in strings as \\nnn is %s.\n"),
		    value);
}

/* String to be printed before warning messages, if any.  */

const char *warning_pre_print = "\nwarning: ";

bool pagination_enabled = true;
static void
show_pagination_enabled (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("State of pagination is %s.\n"), value);
}




/* Print a warning message.  The first argument STRING is the warning
   message, used as an fprintf format string, the second is the
   va_list of arguments for that string.  A warning is unfiltered (not
   paginated) so that the user does not need to page through each
   screen full of warnings when there are lots of them.  */

void
vwarning (const char *string, va_list args)
{
  if (deprecated_warning_hook)
    (*deprecated_warning_hook) (string, args);
  else
    {
      gdb::optional<target_terminal::scoped_restore_terminal_state> term_state;
      if (target_supports_terminal_ours ())
	{
	  term_state.emplace ();
	  target_terminal::ours_for_output ();
	}
      if (filtered_printing_initialized ())
	wrap_here ("");		/* Force out any buffered output.  */
      gdb_flush (gdb_stdout);
      if (warning_pre_print)
	fputs_unfiltered (warning_pre_print, gdb_stderr);
      vfprintf_unfiltered (gdb_stderr, string, args);
      fprintf_unfiltered (gdb_stderr, "\n");
    }
}

/* Print an error message and return to command level.
   The first argument STRING is the error message, used as a fprintf string,
   and the remaining args are passed as arguments to it.  */

void
verror (const char *string, va_list args)
{
  throw_verror (GENERIC_ERROR, string, args);
}

void
error_stream (const string_file &stream)
{
  error (("%s"), stream.c_str ());
}

/* Emit a message and abort.  */

static void ATTRIBUTE_NORETURN
abort_with_message (const char *msg)
{
  if (current_ui == NULL)
    fputs (msg, stderr);
  else
    fputs_unfiltered (msg, gdb_stderr);

  abort ();		/* ARI: abort */
}

/* Dump core trying to increase the core soft limit to hard limit first.  */

void
dump_core (void)
{
#ifdef HAVE_SETRLIMIT
  struct rlimit rlim = { (rlim_t) RLIM_INFINITY, (rlim_t) RLIM_INFINITY };

  setrlimit (RLIMIT_CORE, &rlim);
#endif /* HAVE_SETRLIMIT */

  abort ();		/* ARI: abort */
}

/* Check whether GDB will be able to dump core using the dump_core
   function.  Returns zero if GDB cannot or should not dump core.
   If LIMIT_KIND is LIMIT_CUR the user's soft limit will be respected.
   If LIMIT_KIND is LIMIT_MAX only the hard limit will be respected.  */

int
can_dump_core (enum resource_limit_kind limit_kind)
{
#ifdef HAVE_GETRLIMIT
  struct rlimit rlim;

  /* Be quiet and assume we can dump if an error is returned.  */
  if (getrlimit (RLIMIT_CORE, &rlim) != 0)
    return 1;

  switch (limit_kind)
    {
    case LIMIT_CUR:
      if (rlim.rlim_cur == 0)
	return 0;
      /* Fall through.  */

    case LIMIT_MAX:
      if (rlim.rlim_max == 0)
	return 0;
    }
#endif /* HAVE_GETRLIMIT */

  return 1;
}

/* Print a warning that we cannot dump core.  */

void
warn_cant_dump_core (const char *reason)
{
  fprintf_unfiltered (gdb_stderr,
		      _("%s\nUnable to dump core, use `ulimit -c"
			" unlimited' before executing GDB next time.\n"),
		      reason);
}

/* Check whether GDB will be able to dump core using the dump_core
   function, and print a warning if we cannot.  */

static int
can_dump_core_warn (enum resource_limit_kind limit_kind,
		    const char *reason)
{
  int core_dump_allowed = can_dump_core (limit_kind);

  if (!core_dump_allowed)
    warn_cant_dump_core (reason);

  return core_dump_allowed;
}

/* Allow the user to configure the debugger behavior with respect to
   what to do when an internal problem is detected.  */

const char internal_problem_ask[] = "ask";
const char internal_problem_yes[] = "yes";
const char internal_problem_no[] = "no";
static const char *const internal_problem_modes[] =
{
  internal_problem_ask,
  internal_problem_yes,
  internal_problem_no,
  NULL
};

/* Print a message reporting an internal error/warning.  Ask the user
   if they want to continue, dump core, or just exit.  Return
   something to indicate a quit.  */

struct internal_problem
{
  const char *name;
  int user_settable_should_quit;
  const char *should_quit;
  int user_settable_should_dump_core;
  const char *should_dump_core;
};

/* Report a problem, internal to GDB, to the user.  Once the problem
   has been reported, and assuming GDB didn't quit, the caller can
   either allow execution to resume or throw an error.  */

static void ATTRIBUTE_PRINTF (4, 0)
internal_vproblem (struct internal_problem *problem,
		   const char *file, int line, const char *fmt, va_list ap)
{
  static int dejavu;
  int quit_p;
  int dump_core_p;
  std::string reason;

  /* Don't allow infinite error/warning recursion.  */
  {
    static const char msg[] = "Recursive internal problem.\n";

    switch (dejavu)
      {
      case 0:
	dejavu = 1;
	break;
      case 1:
	dejavu = 2;
	abort_with_message (msg);
      default:
	dejavu = 3;
        /* Newer GLIBC versions put the warn_unused_result attribute
           on write, but this is one of those rare cases where
           ignoring the return value is correct.  Casting to (void)
           does not fix this problem.  This is the solution suggested
           at http://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509.  */
	if (write (STDERR_FILENO, msg, sizeof (msg)) != sizeof (msg))
          abort (); /* ARI: abort */
	exit (1);
      }
  }

  /* Create a string containing the full error/warning message.  Need
     to call query with this full string, as otherwize the reason
     (error/warning) and question become separated.  Format using a
     style similar to a compiler error message.  Include extra detail
     so that the user knows that they are living on the edge.  */
  {
    std::string msg = string_vprintf (fmt, ap);
    reason = string_printf ("%s:%d: %s: %s\n"
			    "A problem internal to GDB has been detected,\n"
			    "further debugging may prove unreliable.",
			    file, line, problem->name, msg.c_str ());
  }

  /* Fall back to abort_with_message if gdb_stderr is not set up.  */
  if (current_ui == NULL)
    {
      fputs (reason.c_str (), stderr);
      abort_with_message ("\n");
    }

  /* Try to get the message out and at the start of a new line.  */
  gdb::optional<target_terminal::scoped_restore_terminal_state> term_state;
  if (target_supports_terminal_ours ())
    {
      term_state.emplace ();
      target_terminal::ours_for_output ();
    }
  if (filtered_printing_initialized ())
    begin_line ();

  /* Emit the message unless query will emit it below.  */
  if (problem->should_quit != internal_problem_ask
      || !confirm
      || !filtered_printing_initialized ())
    fprintf_unfiltered (gdb_stderr, "%s\n", reason.c_str ());

  if (problem->should_quit == internal_problem_ask)
    {
      /* Default (yes/batch case) is to quit GDB.  When in batch mode
	 this lessens the likelihood of GDB going into an infinite
	 loop.  */
      if (!confirm || !filtered_printing_initialized ())
	quit_p = 1;
      else
        quit_p = query (_("%s\nQuit this debugging session? "),
			reason.c_str ());
    }
  else if (problem->should_quit == internal_problem_yes)
    quit_p = 1;
  else if (problem->should_quit == internal_problem_no)
    quit_p = 0;
  else
    internal_error (__FILE__, __LINE__, _("bad switch"));

  fputs_unfiltered (_("\nThis is a bug, please report it."), gdb_stderr);
  if (REPORT_BUGS_TO[0])
    fprintf_unfiltered (gdb_stderr, _("  For instructions, see:\n%s."),
			REPORT_BUGS_TO);
  fputs_unfiltered ("\n\n", gdb_stderr);

  if (problem->should_dump_core == internal_problem_ask)
    {
      if (!can_dump_core_warn (LIMIT_MAX, reason.c_str ()))
	dump_core_p = 0;
      else if (!filtered_printing_initialized ())
	dump_core_p = 1;
      else
	{
	  /* Default (yes/batch case) is to dump core.  This leaves a GDB
	     `dropping' so that it is easier to see that something went
	     wrong in GDB.  */
	  dump_core_p = query (_("%s\nCreate a core file of GDB? "),
			       reason.c_str ());
	}
    }
  else if (problem->should_dump_core == internal_problem_yes)
    dump_core_p = can_dump_core_warn (LIMIT_MAX, reason.c_str ());
  else if (problem->should_dump_core == internal_problem_no)
    dump_core_p = 0;
  else
    internal_error (__FILE__, __LINE__, _("bad switch"));

  if (quit_p)
    {
      if (dump_core_p)
	dump_core ();
      else
	exit (1);
    }
  else
    {
      if (dump_core_p)
	{
#ifdef HAVE_WORKING_FORK
	  if (fork () == 0)
	    dump_core ();
#endif
	}
    }

  dejavu = 0;
}

static struct internal_problem internal_error_problem = {
  "internal-error", 1, internal_problem_ask, 1, internal_problem_ask
};

void
internal_verror (const char *file, int line, const char *fmt, va_list ap)
{
  internal_vproblem (&internal_error_problem, file, line, fmt, ap);
  throw_quit (_("Command aborted."));
}

static struct internal_problem internal_warning_problem = {
  "internal-warning", 1, internal_problem_ask, 1, internal_problem_ask
};

void
internal_vwarning (const char *file, int line, const char *fmt, va_list ap)
{
  internal_vproblem (&internal_warning_problem, file, line, fmt, ap);
}

static struct internal_problem demangler_warning_problem = {
  "demangler-warning", 1, internal_problem_ask, 0, internal_problem_no
};

void
demangler_vwarning (const char *file, int line, const char *fmt, va_list ap)
{
  internal_vproblem (&demangler_warning_problem, file, line, fmt, ap);
}

void
demangler_warning (const char *file, int line, const char *string, ...)
{
  va_list ap;

  va_start (ap, string);
  demangler_vwarning (file, line, string, ap);
  va_end (ap);
}

/* When GDB reports an internal problem (error or warning) it gives
   the user the opportunity to quit GDB and/or create a core file of
   the current debug session.  This function registers a few commands
   that make it possible to specify that GDB should always or never
   quit or create a core file, without asking.  The commands look
   like:

   maint set PROBLEM-NAME quit ask|yes|no
   maint show PROBLEM-NAME quit
   maint set PROBLEM-NAME corefile ask|yes|no
   maint show PROBLEM-NAME corefile

   Where PROBLEM-NAME is currently "internal-error" or
   "internal-warning".  */

static void
add_internal_problem_command (struct internal_problem *problem)
{
  struct cmd_list_element **set_cmd_list;
  struct cmd_list_element **show_cmd_list;
  char *set_doc;
  char *show_doc;

  set_cmd_list = XNEW (struct cmd_list_element *);
  show_cmd_list = XNEW (struct cmd_list_element *);
  *set_cmd_list = NULL;
  *show_cmd_list = NULL;

  set_doc = xstrprintf (_("Configure what GDB does when %s is detected."),
			problem->name);

  show_doc = xstrprintf (_("Show what GDB does when %s is detected."),
			 problem->name);

  add_basic_prefix_cmd (problem->name, class_maintenance, set_doc,
			set_cmd_list,
			concat ("maintenance set ", problem->name, " ",
				(char *) NULL),
			0/*allow-unknown*/, &maintenance_set_cmdlist);

  add_show_prefix_cmd (problem->name, class_maintenance, show_doc,
		       show_cmd_list,
		       concat ("maintenance show ", problem->name, " ",
			       (char *) NULL),
		       0/*allow-unknown*/, &maintenance_show_cmdlist);

  if (problem->user_settable_should_quit)
    {
      set_doc = xstrprintf (_("Set whether GDB should quit "
			      "when an %s is detected."),
			    problem->name);
      show_doc = xstrprintf (_("Show whether GDB will quit "
			       "when an %s is detected."),
			     problem->name);
      add_setshow_enum_cmd ("quit", class_maintenance,
			    internal_problem_modes,
			    &problem->should_quit,
			    set_doc,
			    show_doc,
			    NULL, /* help_doc */
			    NULL, /* setfunc */
			    NULL, /* showfunc */
			    set_cmd_list,
			    show_cmd_list);

      xfree (set_doc);
      xfree (show_doc);
    }

  if (problem->user_settable_should_dump_core)
    {
      set_doc = xstrprintf (_("Set whether GDB should create a core "
			      "file of GDB when %s is detected."),
			    problem->name);
      show_doc = xstrprintf (_("Show whether GDB will create a core "
			       "file of GDB when %s is detected."),
			     problem->name);
      add_setshow_enum_cmd ("corefile", class_maintenance,
			    internal_problem_modes,
			    &problem->should_dump_core,
			    set_doc,
			    show_doc,
			    NULL, /* help_doc */
			    NULL, /* setfunc */
			    NULL, /* showfunc */
			    set_cmd_list,
			    show_cmd_list);

      xfree (set_doc);
      xfree (show_doc);
    }
}

/* Return a newly allocated string, containing the PREFIX followed
   by the system error message for errno (separated by a colon).  */

static std::string
perror_string (const char *prefix)
{
  const char *err = safe_strerror (errno);
  return std::string (prefix) + ": " + err;
}

/* Print the system error message for errno, and also mention STRING
   as the file name for which the error was encountered.  Use ERRCODE
   for the thrown exception.  Then return to command level.  */

void
throw_perror_with_name (enum errors errcode, const char *string)
{
  std::string combined = perror_string (string);

  /* I understand setting these is a matter of taste.  Still, some people
     may clear errno but not know about bfd_error.  Doing this here is not
     unreasonable.  */
  bfd_set_error (bfd_error_no_error);
  errno = 0;

  throw_error (errcode, _("%s."), combined.c_str ());
}

/* See throw_perror_with_name, ERRCODE defaults here to GENERIC_ERROR.  */

void
perror_with_name (const char *string)
{
  throw_perror_with_name (GENERIC_ERROR, string);
}

/* Same as perror_with_name except that it prints a warning instead
   of throwing an error.  */

void
perror_warning_with_name (const char *string)
{
  std::string combined = perror_string (string);
  warning (_("%s"), combined.c_str ());
}

/* Print the system error message for ERRCODE, and also mention STRING
   as the file name for which the error was encountered.  */

void
print_sys_errmsg (const char *string, int errcode)
{
  const char *err = safe_strerror (errcode);
  /* We want anything which was printed on stdout to come out first, before
     this message.  */
  gdb_flush (gdb_stdout);
  fprintf_unfiltered (gdb_stderr, "%s: %s.\n", string, err);
}

/* Control C eventually causes this to be called, at a convenient time.  */

void
quit (void)
{
  if (sync_quit_force_run)
    {
      sync_quit_force_run = 0;
      quit_force (NULL, 0);
    }

#ifdef __MSDOS__
  /* No steenking SIGINT will ever be coming our way when the
     program is resumed.  Don't lie.  */
  throw_quit ("Quit");
#else
  if (job_control
      /* If there is no terminal switching for this target, then we can't
         possibly get screwed by the lack of job control.  */
      || !target_supports_terminal_ours ())
    throw_quit ("Quit");
  else
    throw_quit ("Quit (expect signal SIGINT when the program is resumed)");
#endif
}

/* See defs.h.  */

void
maybe_quit (void)
{
  if (sync_quit_force_run)
    quit ();

  quit_handler ();
}


/* Called when a memory allocation fails, with the number of bytes of
   memory requested in SIZE.  */

void
malloc_failure (long size)
{
  if (size > 0)
    {
      internal_error (__FILE__, __LINE__,
		      _("virtual memory exhausted: can't allocate %ld bytes."),
		      size);
    }
  else
    {
      internal_error (__FILE__, __LINE__, _("virtual memory exhausted."));
    }
}

/* See common/errors.h.  */

void
flush_streams ()
{
  gdb_stdout->flush ();
  gdb_stderr->flush ();
}

/* My replacement for the read system call.
   Used like `read' but keeps going if `read' returns too soon.  */

int
myread (int desc, char *addr, int len)
{
  int val;
  int orglen = len;

  while (len > 0)
    {
      val = read (desc, addr, len);
      if (val < 0)
	return val;
      if (val == 0)
	return orglen - len;
      len -= val;
      addr += val;
    }
  return orglen;
}

void
print_spaces (int n, struct ui_file *file)
{
  fputs_unfiltered (n_spaces (n), file);
}

/* Print a host address.  */

void
gdb_print_host_address_1 (const void *addr, struct ui_file *stream)
{
  fprintf_filtered (stream, "%s", host_address_to_string (addr));
}



/* An RAII class that sets up to handle input and then tears down
   during destruction.  */

class scoped_input_handler
{
public:

  scoped_input_handler ()
    : m_quit_handler (&quit_handler, default_quit_handler),
      m_ui (NULL)
  {
    target_terminal::ours ();
    ui_register_input_event_handler (current_ui);
    if (current_ui->prompt_state == PROMPT_BLOCKED)
      m_ui = current_ui;
  }

  ~scoped_input_handler ()
  {
    if (m_ui != NULL)
      ui_unregister_input_event_handler (m_ui);
  }

  DISABLE_COPY_AND_ASSIGN (scoped_input_handler);

private:

  /* Save and restore the terminal state.  */
  target_terminal::scoped_restore_terminal_state m_term_state;

  /* Save and restore the quit handler.  */
  scoped_restore_tmpl<quit_handler_ftype *> m_quit_handler;

  /* The saved UI, if non-NULL.  */
  struct ui *m_ui;
};



/* This function supports the query, nquery, and yquery functions.
   Ask user a y-or-n question and return 0 if answer is no, 1 if
   answer is yes, or default the answer to the specified default
   (for yquery or nquery).  DEFCHAR may be 'y' or 'n' to provide a
   default answer, or '\0' for no default.
   CTLSTR is the control string and should end in "? ".  It should
   not say how to answer, because we do that.
   ARGS are the arguments passed along with the CTLSTR argument to
   printf.  */

static int ATTRIBUTE_PRINTF (1, 0)
defaulted_query (const char *ctlstr, const char defchar, va_list args)
{
  int retval;
  int def_value;
  char def_answer, not_def_answer;
  const char *y_string, *n_string;

  /* Set up according to which answer is the default.  */
  if (defchar == '\0')
    {
      def_value = 1;
      def_answer = 'Y';
      not_def_answer = 'N';
      y_string = "y";
      n_string = "n";
    }
  else if (defchar == 'y')
    {
      def_value = 1;
      def_answer = 'Y';
      not_def_answer = 'N';
      y_string = "[y]";
      n_string = "n";
    }
  else
    {
      def_value = 0;
      def_answer = 'N';
      not_def_answer = 'Y';
      y_string = "y";
      n_string = "[n]";
    }

  /* Automatically answer the default value if the user did not want
     prompts or the command was issued with the server prefix.  */
  if (!confirm || server_command)
    return def_value;

  /* If input isn't coming from the user directly, just say what
     question we're asking, and then answer the default automatically.  This
     way, important error messages don't get lost when talking to GDB
     over a pipe.  */
  if (current_ui->instream != current_ui->stdin_stream
      || !input_interactive_p (current_ui)
      /* Restrict queries to the main UI.  */
      || current_ui != main_ui)
    {
      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();
      wrap_here ("");
      vfprintf_filtered (gdb_stdout, ctlstr, args);

      printf_filtered (_("(%s or %s) [answered %c; "
			 "input not from terminal]\n"),
		       y_string, n_string, def_answer);

      return def_value;
    }

  if (deprecated_query_hook)
    {
      target_terminal::scoped_restore_terminal_state term_state;
      return deprecated_query_hook (ctlstr, args);
    }

  /* Format the question outside of the loop, to avoid reusing args.  */
  std::string question = string_vprintf (ctlstr, args);
  std::string prompt
    = string_printf (_("%s%s(%s or %s) %s"),
		     annotation_level > 1 ? "\n\032\032pre-query\n" : "",
		     question.c_str (), y_string, n_string,
		     annotation_level > 1 ? "\n\032\032query\n" : "");

  /* Used to add duration we waited for user to respond to
     prompt_for_continue_wait_time.  */
  using namespace std::chrono;
  steady_clock::time_point prompt_started = steady_clock::now ();

  scoped_input_handler prepare_input;

  while (1)
    {
      char *response, answer;

      gdb_flush (gdb_stdout);
      response = gdb_readline_wrapper (prompt.c_str ());

      if (response == NULL)	/* C-d  */
	{
	  printf_filtered ("EOF [assumed %c]\n", def_answer);
	  retval = def_value;
	  break;
	}

      answer = response[0];
      xfree (response);

      if (answer >= 'a')
	answer -= 040;
      /* Check answer.  For the non-default, the user must specify
         the non-default explicitly.  */
      if (answer == not_def_answer)
	{
	  retval = !def_value;
	  break;
	}
      /* Otherwise, if a default was specified, the user may either
         specify the required input or have it default by entering
         nothing.  */
      if (answer == def_answer
	  || (defchar != '\0' && answer == '\0'))
	{
	  retval = def_value;
	  break;
	}
      /* Invalid entries are not defaulted and require another selection.  */
      printf_filtered (_("Please answer %s or %s.\n"),
		       y_string, n_string);
    }

  /* Add time spend in this routine to prompt_for_continue_wait_time.  */
  prompt_for_continue_wait_time += steady_clock::now () - prompt_started;

  if (annotation_level > 1)
    printf_filtered (("\n\032\032post-query\n"));
  return retval;
}


/* Ask user a y-or-n question and return 0 if answer is no, 1 if
   answer is yes, or 0 if answer is defaulted.
   Takes three args which are given to printf to print the question.
   The first, a control string, should end in "? ".
   It should not say how to answer, because we do that.  */

int
nquery (const char *ctlstr, ...)
{
  va_list args;
  int ret;

  va_start (args, ctlstr);
  ret = defaulted_query (ctlstr, 'n', args);
  va_end (args);
  return ret;
}

/* Ask user a y-or-n question and return 0 if answer is no, 1 if
   answer is yes, or 1 if answer is defaulted.
   Takes three args which are given to printf to print the question.
   The first, a control string, should end in "? ".
   It should not say how to answer, because we do that.  */

int
yquery (const char *ctlstr, ...)
{
  va_list args;
  int ret;

  va_start (args, ctlstr);
  ret = defaulted_query (ctlstr, 'y', args);
  va_end (args);
  return ret;
}

/* Ask user a y-or-n question and return 1 iff answer is yes.
   Takes three args which are given to printf to print the question.
   The first, a control string, should end in "? ".
   It should not say how to answer, because we do that.  */

int
query (const char *ctlstr, ...)
{
  va_list args;
  int ret;

  va_start (args, ctlstr);
  ret = defaulted_query (ctlstr, '\0', args);
  va_end (args);
  return ret;
}

/* A helper for parse_escape that converts a host character to a
   target character.  C is the host character.  If conversion is
   possible, then the target character is stored in *TARGET_C and the
   function returns 1.  Otherwise, the function returns 0.  */

static int
host_char_to_target (struct gdbarch *gdbarch, int c, int *target_c)
{
  char the_char = c;
  int result = 0;

  auto_obstack host_data;

  convert_between_encodings (target_charset (gdbarch), host_charset (),
			     (gdb_byte *) &the_char, 1, 1,
			     &host_data, translit_none);

  if (obstack_object_size (&host_data) == 1)
    {
      result = 1;
      *target_c = *(char *) obstack_base (&host_data);
    }

  return result;
}

/* Parse a C escape sequence.  STRING_PTR points to a variable
   containing a pointer to the string to parse.  That pointer
   should point to the character after the \.  That pointer
   is updated past the characters we use.  The value of the
   escape sequence is returned.

   A negative value means the sequence \ newline was seen,
   which is supposed to be equivalent to nothing at all.

   If \ is followed by a null character, we return a negative
   value and leave the string pointer pointing at the null character.

   If \ is followed by 000, we return 0 and leave the string pointer
   after the zeros.  A value of 0 does not mean end of string.  */

int
parse_escape (struct gdbarch *gdbarch, const char **string_ptr)
{
  int target_char = -2;	/* Initialize to avoid GCC warnings.  */
  int c = *(*string_ptr)++;

  switch (c)
    {
      case '\n':
	return -2;
      case 0:
	(*string_ptr)--;
	return 0;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
	{
	  int i = host_hex_value (c);
	  int count = 0;
	  while (++count < 3)
	    {
	      c = (**string_ptr);
	      if (ISDIGIT (c) && c != '8' && c != '9')
		{
		  (*string_ptr)++;
		  i *= 8;
		  i += host_hex_value (c);
		}
	      else
		{
		  break;
		}
	    }
	  return i;
	}

    case 'a':
      c = '\a';
      break;
    case 'b':
      c = '\b';
      break;
    case 'f':
      c = '\f';
      break;
    case 'n':
      c = '\n';
      break;
    case 'r':
      c = '\r';
      break;
    case 't':
      c = '\t';
      break;
    case 'v':
      c = '\v';
      break;

    default:
      break;
    }

  if (!host_char_to_target (gdbarch, c, &target_char))
    error (_("The escape sequence `\\%c' is equivalent to plain `%c',"
	     " which has no equivalent\nin the `%s' character set."),
	   c, c, target_charset (gdbarch));
  return target_char;
}

/* Print the character C on STREAM as part of the contents of a literal
   string whose delimiter is QUOTER.  Note that this routine should only
   be called for printing things which are independent of the language
   of the program being debugged.

   printchar will normally escape backslashes and instances of QUOTER. If
   QUOTER is 0, printchar won't escape backslashes or any quoting character.
   As a side effect, if you pass the backslash character as the QUOTER,
   printchar will escape backslashes as usual, but not any other quoting
   character. */

static void
printchar (int c, do_fputc_ftype do_fputc, ui_file *stream, int quoter)
{
  c &= 0xFF;			/* Avoid sign bit follies */

  if (c < 0x20 ||		/* Low control chars */
      (c >= 0x7F && c < 0xA0) ||	/* DEL, High controls */
      (sevenbit_strings && c >= 0x80))
    {				/* high order bit set */
      do_fputc ('\\', stream);

      switch (c)
	{
	case '\n':
	  do_fputc ('n', stream);
	  break;
	case '\b':
	  do_fputc ('b', stream);
	  break;
	case '\t':
	  do_fputc ('t', stream);
	  break;
	case '\f':
	  do_fputc ('f', stream);
	  break;
	case '\r':
	  do_fputc ('r', stream);
	  break;
	case '\033':
	  do_fputc ('e', stream);
	  break;
	case '\007':
	  do_fputc ('a', stream);
	  break;
	default:
	  {
	    do_fputc ('0' + ((c >> 6) & 0x7), stream);
	    do_fputc ('0' + ((c >> 3) & 0x7), stream);
	    do_fputc ('0' + ((c >> 0) & 0x7), stream);
	    break;
	  }
	}
    }
  else
    {
      if (quoter != 0 && (c == '\\' || c == quoter))
	do_fputc ('\\', stream);
      do_fputc (c, stream);
    }
}

/* Print the character C on STREAM as part of the contents of a
   literal string whose delimiter is QUOTER.  Note that these routines
   should only be call for printing things which are independent of
   the language of the program being debugged.  */

void
fputstr_filtered (const char *str, int quoter, struct ui_file *stream)
{
  while (*str)
    printchar (*str++, fputc_filtered, stream, quoter);
}

void
fputstr_unfiltered (const char *str, int quoter, struct ui_file *stream)
{
  while (*str)
    printchar (*str++, fputc_unfiltered, stream, quoter);
}

void
fputstrn_filtered (const char *str, int n, int quoter,
		   struct ui_file *stream)
{
  for (int i = 0; i < n; i++)
    printchar (str[i], fputc_filtered, stream, quoter);
}

void
fputstrn_unfiltered (const char *str, int n, int quoter,
		     do_fputc_ftype do_fputc, struct ui_file *stream)
{
  for (int i = 0; i < n; i++)
    printchar (str[i], do_fputc, stream, quoter);
}


/* Number of lines per page or UINT_MAX if paging is disabled.  */
static unsigned int lines_per_page;
static void
show_lines_per_page (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Number of lines gdb thinks are in a page is %s.\n"),
		    value);
}

/* Number of chars per line or UINT_MAX if line folding is disabled.  */
static unsigned int chars_per_line;
static void
show_chars_per_line (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Number of characters gdb thinks "
		      "are in a line is %s.\n"),
		    value);
}

/* Current count of lines printed on this page, chars on this line.  */
static unsigned int lines_printed, chars_printed;

/* True if pagination is disabled for just one command.  */

static bool pagination_disabled_for_command;

/* Buffer and start column of buffered text, for doing smarter word-
   wrapping.  When someone calls wrap_here(), we start buffering output
   that comes through fputs_filtered().  If we see a newline, we just
   spit it out and forget about the wrap_here().  If we see another
   wrap_here(), we spit it out and remember the newer one.  If we see
   the end of the line, we spit out a newline, the indent, and then
   the buffered output.  */

static bool filter_initialized = false;

/* Contains characters which are waiting to be output (they have
   already been counted in chars_printed).  */
static std::string wrap_buffer;

/* String to indent by if the wrap occurs.  Must not be NULL if wrap_column
   is non-zero.  */
static const char *wrap_indent;

/* Column number on the screen where wrap_buffer begins, or 0 if wrapping
   is not in effect.  */
static int wrap_column;

/* The style applied at the time that wrap_here was called.  */
static ui_file_style wrap_style;


/* Initialize the number of lines per page and chars per line.  */

void
init_page_info (void)
{
  if (batch_flag)
    {
      lines_per_page = UINT_MAX;
      chars_per_line = UINT_MAX;
    }
  else
#if defined(TUI)
  if (!tui_get_command_dimension (&chars_per_line, &lines_per_page))
#endif
    {
      int rows, cols;

#if defined(__GO32__)
      rows = ScreenRows ();
      cols = ScreenCols ();
      lines_per_page = rows;
      chars_per_line = cols;
#else
      /* Make sure Readline has initialized its terminal settings.  */
      rl_reset_terminal (NULL);

      /* Get the screen size from Readline.  */
      rl_get_screen_size (&rows, &cols);
      lines_per_page = rows;
      chars_per_line = cols;

      /* Readline should have fetched the termcap entry for us.
         Only try to use tgetnum function if rl_get_screen_size
         did not return a useful value. */
      if (((rows <= 0) && (tgetnum ((char *) "li") < 0))
	/* Also disable paging if inside Emacs.  $EMACS was used
	   before Emacs v25.1, $INSIDE_EMACS is used since then.  */
	  || getenv ("EMACS") || getenv ("INSIDE_EMACS"))
	{
	  /* The number of lines per page is not mentioned in the terminal
	     description or EMACS environment variable is set.  This probably
	     means that paging is not useful, so disable paging.  */
	  lines_per_page = UINT_MAX;
	}

      /* If the output is not a terminal, don't paginate it.  */
      if (!gdb_stdout->isatty ())
	lines_per_page = UINT_MAX;
#endif
    }

  /* We handle SIGWINCH ourselves.  */
  rl_catch_sigwinch = 0;

  set_screen_size ();
  set_width ();
}

/* Return nonzero if filtered printing is initialized.  */
int
filtered_printing_initialized (void)
{
  return filter_initialized;
}

set_batch_flag_and_restore_page_info::set_batch_flag_and_restore_page_info ()
  : m_save_lines_per_page (lines_per_page),
    m_save_chars_per_line (chars_per_line),
    m_save_batch_flag (batch_flag)
{
  batch_flag = 1;
  init_page_info ();
}

set_batch_flag_and_restore_page_info::~set_batch_flag_and_restore_page_info ()
{
  batch_flag = m_save_batch_flag;
  chars_per_line = m_save_chars_per_line;
  lines_per_page = m_save_lines_per_page;

  set_screen_size ();
  set_width ();
}

/* Set the screen size based on LINES_PER_PAGE and CHARS_PER_LINE.  */

static void
set_screen_size (void)
{
  int rows = lines_per_page;
  int cols = chars_per_line;

  /* If we get 0 or negative ROWS or COLS, treat as "infinite" size.
     A negative number can be seen here with the "set width/height"
     commands and either:

     - the user specified "unlimited", which maps to UINT_MAX, or
     - the user specified some number between INT_MAX and UINT_MAX.

     Cap "infinity" to approximately sqrt(INT_MAX) so that we don't
     overflow in rl_set_screen_size, which multiplies rows and columns
     to compute the number of characters on the screen.  */

  const int sqrt_int_max = INT_MAX >> (sizeof (int) * 8 / 2);

  if (rows <= 0 || rows > sqrt_int_max)
    {
      rows = sqrt_int_max;
      lines_per_page = UINT_MAX;
    }

  if (cols <= 0 || cols > sqrt_int_max)
    {
      cols = sqrt_int_max;
      chars_per_line = UINT_MAX;
    }

  /* Update Readline's idea of the terminal size.  */
  rl_set_screen_size (rows, cols);
}

/* Reinitialize WRAP_BUFFER.  */

static void
set_width (void)
{
  if (chars_per_line == 0)
    init_page_info ();

  wrap_buffer.clear ();
  filter_initialized = true;
}

static void
set_width_command (const char *args, int from_tty, struct cmd_list_element *c)
{
  set_screen_size ();
  set_width ();
}

static void
set_height_command (const char *args, int from_tty, struct cmd_list_element *c)
{
  set_screen_size ();
}

/* See utils.h.  */

void
set_screen_width_and_height (int width, int height)
{
  lines_per_page = height;
  chars_per_line = width;

  set_screen_size ();
  set_width ();
}

/* The currently applied style.  */

static ui_file_style applied_style;

/* Emit an ANSI style escape for STYLE.  If STREAM is nullptr, emit to
   the wrap buffer; otherwise emit to STREAM.  */

static void
emit_style_escape (const ui_file_style &style,
		   struct ui_file *stream = nullptr)
{
  applied_style = style;

  if (stream == nullptr)
    wrap_buffer.append (style.to_ansi ());
  else
    stream->puts (style.to_ansi ().c_str ());
}

/* Set the current output style.  This will affect future uses of the
   _filtered output functions.  */

static void
set_output_style (struct ui_file *stream, const ui_file_style &style)
{
  if (!stream->can_emit_style_escape ())
    return;

  /* Note that we may not pass STREAM here, when we want to emit to
     the wrap buffer, not directly to STREAM.  */
  if (stream == gdb_stdout)
    stream = nullptr;
  emit_style_escape (style, stream);
}

/* See utils.h.  */

void
reset_terminal_style (struct ui_file *stream)
{
  if (stream->can_emit_style_escape ())
    {
      /* Force the setting, regardless of what we think the setting
	 might already be.  */
      applied_style = ui_file_style ();
      wrap_buffer.append (applied_style.to_ansi ());
    }
}

/* Wait, so the user can read what's on the screen.  Prompt the user
   to continue by pressing RETURN.  'q' is also provided because
   telling users what to do in the prompt is more user-friendly than
   expecting them to think of Ctrl-C/SIGINT.  */

static void
prompt_for_continue (void)
{
  char cont_prompt[120];
  /* Used to add duration we waited for user to respond to
     prompt_for_continue_wait_time.  */
  using namespace std::chrono;
  steady_clock::time_point prompt_started = steady_clock::now ();
  bool disable_pagination = pagination_disabled_for_command;

  /* Clear the current styling.  */
  if (gdb_stdout->can_emit_style_escape ())
    emit_style_escape (ui_file_style (), gdb_stdout);

  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032pre-prompt-for-continue\n"));

  strcpy (cont_prompt,
	  "--Type <RET> for more, q to quit, "
	  "c to continue without paging--");
  if (annotation_level > 1)
    strcat (cont_prompt, "\n\032\032prompt-for-continue\n");

  /* We must do this *before* we call gdb_readline_wrapper, else it
     will eventually call us -- thinking that we're trying to print
     beyond the end of the screen.  */
  reinitialize_more_filter ();

  scoped_input_handler prepare_input;

  /* Call gdb_readline_wrapper, not readline, in order to keep an
     event loop running.  */
  gdb::unique_xmalloc_ptr<char> ignore (gdb_readline_wrapper (cont_prompt));

  /* Add time spend in this routine to prompt_for_continue_wait_time.  */
  prompt_for_continue_wait_time += steady_clock::now () - prompt_started;

  if (annotation_level > 1)
    printf_unfiltered (("\n\032\032post-prompt-for-continue\n"));

  if (ignore != NULL)
    {
      char *p = ignore.get ();

      while (*p == ' ' || *p == '\t')
	++p;
      if (p[0] == 'q')
	/* Do not call quit here; there is no possibility of SIGINT.  */
	throw_quit ("Quit");
      if (p[0] == 'c')
	disable_pagination = true;
    }

  /* Now we have to do this again, so that GDB will know that it doesn't
     need to save the ---Type <return>--- line at the top of the screen.  */
  reinitialize_more_filter ();
  pagination_disabled_for_command = disable_pagination;

  dont_repeat ();		/* Forget prev cmd -- CR won't repeat it.  */
}

/* Initialize timer to keep track of how long we waited for the user.  */

void
reset_prompt_for_continue_wait_time (void)
{
  using namespace std::chrono;

  prompt_for_continue_wait_time = steady_clock::duration::zero ();
}

/* Fetch the cumulative time spent in prompt_for_continue.  */

std::chrono::steady_clock::duration
get_prompt_for_continue_wait_time ()
{
  return prompt_for_continue_wait_time;
}

/* Reinitialize filter; ie. tell it to reset to original values.  */

void
reinitialize_more_filter (void)
{
  lines_printed = 0;
  chars_printed = 0;
  pagination_disabled_for_command = false;
}

/* Flush the wrap buffer to STREAM, if necessary.  */

static void
flush_wrap_buffer (struct ui_file *stream)
{
  if (stream == gdb_stdout && !wrap_buffer.empty ())
    {
      stream->puts (wrap_buffer.c_str ());
      wrap_buffer.clear ();
    }
}

/* See utils.h.  */

void
gdb_flush (struct ui_file *stream)
{
  flush_wrap_buffer (stream);
  stream->flush ();
}

/* Indicate that if the next sequence of characters overflows the line,
   a newline should be inserted here rather than when it hits the end.
   If INDENT is non-null, it is a string to be printed to indent the
   wrapped part on the next line.  INDENT must remain accessible until
   the next call to wrap_here() or until a newline is printed through
   fputs_filtered().

   If the line is already overfull, we immediately print a newline and
   the indentation, and disable further wrapping.

   If we don't know the width of lines, but we know the page height,
   we must not wrap words, but should still keep track of newlines
   that were explicitly printed.

   INDENT should not contain tabs, as that will mess up the char count
   on the next line.  FIXME.

   This routine is guaranteed to force out any output which has been
   squirreled away in the wrap_buffer, so wrap_here ((char *)0) can be
   used to force out output from the wrap_buffer.  */

void
wrap_here (const char *indent)
{
  /* This should have been allocated, but be paranoid anyway.  */
  gdb_assert (filter_initialized);

  flush_wrap_buffer (gdb_stdout);
  if (chars_per_line == UINT_MAX)	/* No line overflow checking.  */
    {
      wrap_column = 0;
    }
  else if (chars_printed >= chars_per_line)
    {
      puts_filtered ("\n");
      if (indent != NULL)
	puts_filtered (indent);
      wrap_column = 0;
    }
  else
    {
      wrap_column = chars_printed;
      if (indent == NULL)
	wrap_indent = "";
      else
	wrap_indent = indent;
      wrap_style = applied_style;
    }
}

/* Print input string to gdb_stdout, filtered, with wrap, 
   arranging strings in columns of n chars.  String can be
   right or left justified in the column.  Never prints 
   trailing spaces.  String should never be longer than
   width.  FIXME: this could be useful for the EXAMINE 
   command, which currently doesn't tabulate very well.  */

void
puts_filtered_tabular (char *string, int width, int right)
{
  int spaces = 0;
  int stringlen;
  char *spacebuf;

  gdb_assert (chars_per_line > 0);
  if (chars_per_line == UINT_MAX)
    {
      fputs_filtered (string, gdb_stdout);
      fputs_filtered ("\n", gdb_stdout);
      return;
    }

  if (((chars_printed - 1) / width + 2) * width >= chars_per_line)
    fputs_filtered ("\n", gdb_stdout);

  if (width >= chars_per_line)
    width = chars_per_line - 1;

  stringlen = strlen (string);

  if (chars_printed > 0)
    spaces = width - (chars_printed - 1) % width - 1;
  if (right)
    spaces += width - stringlen;

  spacebuf = (char *) alloca (spaces + 1);
  spacebuf[spaces] = '\0';
  while (spaces--)
    spacebuf[spaces] = ' ';

  fputs_filtered (spacebuf, gdb_stdout);
  fputs_filtered (string, gdb_stdout);
}


/* Ensure that whatever gets printed next, using the filtered output
   commands, starts at the beginning of the line.  I.e. if there is
   any pending output for the current line, flush it and start a new
   line.  Otherwise do nothing.  */

void
begin_line (void)
{
  if (chars_printed > 0)
    {
      puts_filtered ("\n");
    }
}


/* Like fputs but if FILTER is true, pause after every screenful.

   Regardless of FILTER can wrap at points other than the final
   character of a line.

   Unlike fputs, fputs_maybe_filtered does not return a value.
   It is OK for LINEBUFFER to be NULL, in which case just don't print
   anything.

   Note that a longjmp to top level may occur in this routine (only if
   FILTER is true) (since prompt_for_continue may do so) so this
   routine should not be called when cleanups are not in place.  */

static void
fputs_maybe_filtered (const char *linebuffer, struct ui_file *stream,
		      int filter)
{
  const char *lineptr;

  if (linebuffer == 0)
    return;

  /* Don't do any filtering if it is disabled.  */
  if (stream != gdb_stdout
      || !pagination_enabled
      || pagination_disabled_for_command
      || batch_flag
      || (lines_per_page == UINT_MAX && chars_per_line == UINT_MAX)
      || top_level_interpreter () == NULL
      || top_level_interpreter ()->interp_ui_out ()->is_mi_like_p ())
    {
      flush_wrap_buffer (stream);
      stream->puts (linebuffer);
      return;
    }

  auto buffer_clearer
    = make_scope_exit ([&] ()
		       {
			 wrap_buffer.clear ();
			 wrap_column = 0;
			 wrap_indent = "";
		       });

  /* Go through and output each character.  Show line extension
     when this is necessary; prompt user for new page when this is
     necessary.  */

  lineptr = linebuffer;
  while (*lineptr)
    {
      /* Possible new page.  Note that PAGINATION_DISABLED_FOR_COMMAND
	 might be set during this loop, so we must continue to check
	 it here.  */
      if (filter && (lines_printed >= lines_per_page - 1)
	  && !pagination_disabled_for_command)
	prompt_for_continue ();

      while (*lineptr && *lineptr != '\n')
	{
	  int skip_bytes;

	  /* Print a single line.  */
	  if (*lineptr == '\t')
	    {
	      wrap_buffer.push_back ('\t');
	      /* Shifting right by 3 produces the number of tab stops
	         we have already passed, and then adding one and
	         shifting left 3 advances to the next tab stop.  */
	      chars_printed = ((chars_printed >> 3) + 1) << 3;
	      lineptr++;
	    }
	  else if (*lineptr == '\033'
		   && skip_ansi_escape (lineptr, &skip_bytes))
	    {
	      wrap_buffer.append (lineptr, skip_bytes);
	      /* Note that we don't consider this a character, so we
		 don't increment chars_printed here.  */
	      lineptr += skip_bytes;
	    }
	  else
	    {
	      wrap_buffer.push_back (*lineptr);
	      chars_printed++;
	      lineptr++;
	    }

	  if (chars_printed >= chars_per_line)
	    {
	      unsigned int save_chars = chars_printed;

	      /* If we change the style, below, we'll want to reset it
		 before continuing to print.  If there is no wrap
		 column, then we'll only reset the style if the pager
		 prompt is given; and to avoid emitting style
		 sequences in the middle of a run of text, we track
		 this as well.  */
	      ui_file_style save_style;
	      bool did_paginate = false;

	      chars_printed = 0;
	      lines_printed++;
	      if (wrap_column)
		{
		  save_style = wrap_style;
		  if (stream->can_emit_style_escape ())
		    emit_style_escape (ui_file_style (), stream);
		  /* If we aren't actually wrapping, don't output
		     newline -- if chars_per_line is right, we
		     probably just overflowed anyway; if it's wrong,
		     let us keep going.  */
		  /* XXX: The ideal thing would be to call
		     'stream->putc' here, but we can't because it
		     currently calls 'fputc_unfiltered', which ends up
		     calling us, which generates an infinite
		     recursion.  */
		  stream->puts ("\n");
		}
	      else
		{
		  save_style = applied_style;
		  flush_wrap_buffer (stream);
		}

	      /* Possible new page.  Note that
		 PAGINATION_DISABLED_FOR_COMMAND might be set during
		 this loop, so we must continue to check it here.  */
	      if (lines_printed >= lines_per_page - 1
		  && !pagination_disabled_for_command)
		{
		  prompt_for_continue ();
		  did_paginate = true;
		}

	      /* Now output indentation and wrapped string.  */
	      if (wrap_column)
		{
		  stream->puts (wrap_indent);
		  if (stream->can_emit_style_escape ())
		    emit_style_escape (save_style, stream);
		  /* FIXME, this strlen is what prevents wrap_indent from
		     containing tabs.  However, if we recurse to print it
		     and count its chars, we risk trouble if wrap_indent is
		     longer than (the user settable) chars_per_line.
		     Note also that this can set chars_printed > chars_per_line
		     if we are printing a long string.  */
		  chars_printed = strlen (wrap_indent)
		    + (save_chars - wrap_column);
		  wrap_column = 0;	/* And disable fancy wrap */
		}
	      else if (did_paginate && stream->can_emit_style_escape ())
		emit_style_escape (save_style, stream);
	    }
	}

      if (*lineptr == '\n')
	{
	  chars_printed = 0;
	  wrap_here ((char *) 0);	/* Spit out chars, cancel
					   further wraps.  */
	  lines_printed++;
	  /* XXX: The ideal thing would be to call
	     'stream->putc' here, but we can't because it
	     currently calls 'fputc_unfiltered', which ends up
	     calling us, which generates an infinite
	     recursion.  */
	  stream->puts ("\n");
	  lineptr++;
	}
    }

  buffer_clearer.release ();
}

void
fputs_filtered (const char *linebuffer, struct ui_file *stream)
{
  fputs_maybe_filtered (linebuffer, stream, 1);
}

void
fputs_unfiltered (const char *linebuffer, struct ui_file *stream)
{
  fputs_maybe_filtered (linebuffer, stream, 0);
}

/* See utils.h.  */

void
fputs_styled (const char *linebuffer, const ui_file_style &style,
	      struct ui_file *stream)
{
  /* This just makes it so we emit somewhat fewer escape
     sequences.  */
  if (style.is_default ())
    fputs_maybe_filtered (linebuffer, stream, 1);
  else
    {
      set_output_style (stream, style);
      fputs_maybe_filtered (linebuffer, stream, 1);
      set_output_style (stream, ui_file_style ());
    }
}

/* See utils.h.  */

void
fputs_styled_unfiltered (const char *linebuffer, const ui_file_style &style,
			 struct ui_file *stream)
{
  /* This just makes it so we emit somewhat fewer escape
     sequences.  */
  if (style.is_default ())
    fputs_maybe_filtered (linebuffer, stream, 0);
  else
    {
      set_output_style (stream, style);
      fputs_maybe_filtered (linebuffer, stream, 0);
      set_output_style (stream, ui_file_style ());
    }
}

/* See utils.h.  */

void
fputs_highlighted (const char *str, const compiled_regex &highlight,
		   struct ui_file *stream)
{
  regmatch_t pmatch;

  while (*str && highlight.exec (str, 1, &pmatch, 0) == 0)
    {
      size_t n_highlight = pmatch.rm_eo - pmatch.rm_so;

      /* Output the part before pmatch with current style.  */
      while (pmatch.rm_so > 0)
	{
	  fputc_filtered (*str, stream);
	  pmatch.rm_so--;
	  str++;
	}

      /* Output pmatch with the highlight style.  */
      set_output_style (stream, highlight_style.style ());
      while (n_highlight > 0)
	{
	  fputc_filtered (*str, stream);
	  n_highlight--;
	  str++;
	}
      set_output_style (stream, ui_file_style ());
    }

  /* Output the trailing part of STR not matching HIGHLIGHT.  */
  if (*str)
    fputs_filtered (str, stream);
}

int
putchar_unfiltered (int c)
{
  return fputc_unfiltered (c, gdb_stdout);
}

/* Write character C to gdb_stdout using GDB's paging mechanism and return C.
   May return nonlocally.  */

int
putchar_filtered (int c)
{
  return fputc_filtered (c, gdb_stdout);
}

int
fputc_unfiltered (int c, struct ui_file *stream)
{
  char buf[2];

  buf[0] = c;
  buf[1] = 0;
  fputs_unfiltered (buf, stream);
  return c;
}

int
fputc_filtered (int c, struct ui_file *stream)
{
  char buf[2];

  buf[0] = c;
  buf[1] = 0;
  fputs_filtered (buf, stream);
  return c;
}

/* puts_debug is like fputs_unfiltered, except it prints special
   characters in printable fashion.  */

void
puts_debug (char *prefix, char *string, char *suffix)
{
  int ch;

  /* Print prefix and suffix after each line.  */
  static int new_line = 1;
  static int return_p = 0;
  static const char *prev_prefix = "";
  static const char *prev_suffix = "";

  if (*string == '\n')
    return_p = 0;

  /* If the prefix is changing, print the previous suffix, a new line,
     and the new prefix.  */
  if ((return_p || (strcmp (prev_prefix, prefix) != 0)) && !new_line)
    {
      fputs_unfiltered (prev_suffix, gdb_stdlog);
      fputs_unfiltered ("\n", gdb_stdlog);
      fputs_unfiltered (prefix, gdb_stdlog);
    }

  /* Print prefix if we printed a newline during the previous call.  */
  if (new_line)
    {
      new_line = 0;
      fputs_unfiltered (prefix, gdb_stdlog);
    }

  prev_prefix = prefix;
  prev_suffix = suffix;

  /* Output characters in a printable format.  */
  while ((ch = *string++) != '\0')
    {
      switch (ch)
	{
	default:
	  if (gdb_isprint (ch))
	    fputc_unfiltered (ch, gdb_stdlog);

	  else
	    fprintf_unfiltered (gdb_stdlog, "\\x%02x", ch & 0xff);
	  break;

	case '\\':
	  fputs_unfiltered ("\\\\", gdb_stdlog);
	  break;
	case '\b':
	  fputs_unfiltered ("\\b", gdb_stdlog);
	  break;
	case '\f':
	  fputs_unfiltered ("\\f", gdb_stdlog);
	  break;
	case '\n':
	  new_line = 1;
	  fputs_unfiltered ("\\n", gdb_stdlog);
	  break;
	case '\r':
	  fputs_unfiltered ("\\r", gdb_stdlog);
	  break;
	case '\t':
	  fputs_unfiltered ("\\t", gdb_stdlog);
	  break;
	case '\v':
	  fputs_unfiltered ("\\v", gdb_stdlog);
	  break;
	}

      return_p = ch == '\r';
    }

  /* Print suffix if we printed a newline.  */
  if (new_line)
    {
      fputs_unfiltered (suffix, gdb_stdlog);
      fputs_unfiltered ("\n", gdb_stdlog);
    }
}


/* Print a variable number of ARGS using format FORMAT.  If this
   information is going to put the amount written (since the last call
   to REINITIALIZE_MORE_FILTER or the last page break) over the page size,
   call prompt_for_continue to get the users permission to continue.

   Unlike fprintf, this function does not return a value.

   We implement three variants, vfprintf (takes a vararg list and stream),
   fprintf (takes a stream to write on), and printf (the usual).

   Note also that this may throw a quit (since prompt_for_continue may
   do so).  */

static void
vfprintf_maybe_filtered (struct ui_file *stream, const char *format,
			 va_list args, bool filter, bool gdbfmt)
{
  if (gdbfmt)
    {
      ui_out_flags flags = disallow_ui_out_field;
      if (!filter)
	flags |= unfiltered_output;
      cli_ui_out (stream, flags).vmessage (applied_style, format, args);
    }
  else
    {
      std::string str = string_vprintf (format, args);
      fputs_maybe_filtered (str.c_str (), stream, filter);
    }
}


void
vfprintf_filtered (struct ui_file *stream, const char *format, va_list args)
{
  vfprintf_maybe_filtered (stream, format, args, true, true);
}

void
vfprintf_unfiltered (struct ui_file *stream, const char *format, va_list args)
{
  if (debug_timestamp && stream == gdb_stdlog)
    {
      using namespace std::chrono;
      int len, need_nl;

      string_file sfile;
      cli_ui_out (&sfile, 0).vmessage (ui_file_style (), format, args);
      std::string linebuffer = std::move (sfile.string ());

      steady_clock::time_point now = steady_clock::now ();
      seconds s = duration_cast<seconds> (now.time_since_epoch ());
      microseconds us = duration_cast<microseconds> (now.time_since_epoch () - s);

      len = linebuffer.size ();
      need_nl = (len > 0 && linebuffer[len - 1] != '\n');

      std::string timestamp = string_printf ("%ld.%06ld %s%s",
					     (long) s.count (),
					     (long) us.count (),
					     linebuffer.c_str (),
					     need_nl ? "\n": "");
      fputs_unfiltered (timestamp.c_str (), stream);
    }
  else
    vfprintf_maybe_filtered (stream, format, args, false, true);
}

void
vprintf_filtered (const char *format, va_list args)
{
  vfprintf_maybe_filtered (gdb_stdout, format, args, true, false);
}

void
vprintf_unfiltered (const char *format, va_list args)
{
  vfprintf_unfiltered (gdb_stdout, format, args);
}

void
fprintf_filtered (struct ui_file *stream, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vfprintf_filtered (stream, format, args);
  va_end (args);
}

void
fprintf_unfiltered (struct ui_file *stream, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vfprintf_unfiltered (stream, format, args);
  va_end (args);
}

/* Like fprintf_filtered, but prints its result indented.
   Called as fprintfi_filtered (spaces, stream, format, ...);  */

void
fprintfi_filtered (int spaces, struct ui_file *stream, const char *format,
		   ...)
{
  va_list args;

  va_start (args, format);
  print_spaces_filtered (spaces, stream);

  vfprintf_filtered (stream, format, args);
  va_end (args);
}

/* See utils.h.  */

void
fprintf_styled (struct ui_file *stream, const ui_file_style &style,
		const char *format, ...)
{
  va_list args;

  set_output_style (stream, style);
  va_start (args, format);
  vfprintf_filtered (stream, format, args);
  va_end (args);
  set_output_style (stream, ui_file_style ());
}

/* See utils.h.  */

void
vfprintf_styled (struct ui_file *stream, const ui_file_style &style,
		 const char *format, va_list args)
{
  set_output_style (stream, style);
  vfprintf_filtered (stream, format, args);
  set_output_style (stream, ui_file_style ());
}

/* See utils.h.  */

void
vfprintf_styled_no_gdbfmt (struct ui_file *stream, const ui_file_style &style,
			   bool filter, const char *format, va_list args)
{
  std::string str = string_vprintf (format, args);
  if (!str.empty ())
    {
      if (!style.is_default ())
	set_output_style (stream, style);
      fputs_maybe_filtered (str.c_str (), stream, filter);
      if (!style.is_default ())
	set_output_style (stream, ui_file_style ());
    }
}

void
printf_filtered (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vfprintf_filtered (gdb_stdout, format, args);
  va_end (args);
}


void
printf_unfiltered (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vfprintf_unfiltered (gdb_stdout, format, args);
  va_end (args);
}

/* Like printf_filtered, but prints it's result indented.
   Called as printfi_filtered (spaces, format, ...);  */

void
printfi_filtered (int spaces, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  print_spaces_filtered (spaces, gdb_stdout);
  vfprintf_filtered (gdb_stdout, format, args);
  va_end (args);
}

/* Easy -- but watch out!

   This routine is *not* a replacement for puts()!  puts() appends a newline.
   This one doesn't, and had better not!  */

void
puts_filtered (const char *string)
{
  fputs_filtered (string, gdb_stdout);
}

void
puts_unfiltered (const char *string)
{
  fputs_unfiltered (string, gdb_stdout);
}

/* Return a pointer to N spaces and a null.  The pointer is good
   until the next call to here.  */
char *
n_spaces (int n)
{
  char *t;
  static char *spaces = 0;
  static int max_spaces = -1;

  if (n > max_spaces)
    {
      xfree (spaces);
      spaces = (char *) xmalloc (n + 1);
      for (t = spaces + n; t != spaces;)
	*--t = ' ';
      spaces[n] = '\0';
      max_spaces = n;
    }

  return spaces + max_spaces - n;
}

/* Print N spaces.  */
void
print_spaces_filtered (int n, struct ui_file *stream)
{
  fputs_filtered (n_spaces (n), stream);
}

/* C++/ObjC demangler stuff.  */

/* fprintf_symbol_filtered attempts to demangle NAME, a symbol in language
   LANG, using demangling args ARG_MODE, and print it filtered to STREAM.
   If the name is not mangled, or the language for the name is unknown, or
   demangling is off, the name is printed in its "raw" form.  */

void
fprintf_symbol_filtered (struct ui_file *stream, const char *name,
			 enum language lang, int arg_mode)
{
  char *demangled;

  if (name != NULL)
    {
      /* If user wants to see raw output, no problem.  */
      if (!demangle)
	{
	  fputs_filtered (name, stream);
	}
      else
	{
	  demangled = language_demangle (language_def (lang), name, arg_mode);
	  fputs_filtered (demangled ? demangled : name, stream);
	  if (demangled != NULL)
	    {
	      xfree (demangled);
	    }
	}
    }
}

/* True if CH is a character that can be part of a symbol name.  I.e.,
   either a number, a letter, or a '_'.  */

static bool
valid_identifier_name_char (int ch)
{
  return (ISALNUM (ch) || ch == '_');
}

/* Skip to end of token, or to END, whatever comes first.  Input is
   assumed to be a C++ operator name.  */

static const char *
cp_skip_operator_token (const char *token, const char *end)
{
  const char *p = token;
  while (p != end && !ISSPACE (*p) && *p != '(')
    {
      if (valid_identifier_name_char (*p))
	{
	  while (p != end && valid_identifier_name_char (*p))
	    p++;
	  return p;
	}
      else
	{
	  /* Note, ordered such that among ops that share a prefix,
	     longer comes first.  This is so that the loop below can
	     bail on first match.  */
	  static const char *ops[] =
	    {
	      "[",
	      "]",
	      "~",
	      ",",
	      "-=", "--", "->", "-",
	      "+=", "++", "+",
	      "*=", "*",
	      "/=", "/",
	      "%=", "%",
	      "|=", "||", "|",
	      "&=", "&&", "&",
	      "^=", "^",
	      "!=", "!",
	      "<<=", "<=", "<<", "<",
	      ">>=", ">=", ">>", ">",
	      "==", "=",
	    };

	  for (const char *op : ops)
	    {
	      size_t oplen = strlen (op);
	      size_t lencmp = std::min<size_t> (oplen, end - p);

	      if (strncmp (p, op, lencmp) == 0)
		return p + lencmp;
	    }
	  /* Some unidentified character.  Return it.  */
	  return p + 1;
	}
    }

  return p;
}

/* Advance STRING1/STRING2 past whitespace.  */

static void
skip_ws (const char *&string1, const char *&string2, const char *end_str2)
{
  while (ISSPACE (*string1))
    string1++;
  while (string2 < end_str2 && ISSPACE (*string2))
    string2++;
}

/* True if STRING points at the start of a C++ operator name.  START
   is the start of the string that STRING points to, hence when
   reading backwards, we must not read any character before START.  */

static bool
cp_is_operator (const char *string, const char *start)
{
  return ((string == start
	   || !valid_identifier_name_char (string[-1]))
	  && strncmp (string, CP_OPERATOR_STR, CP_OPERATOR_LEN) == 0
	  && !valid_identifier_name_char (string[CP_OPERATOR_LEN]));
}

/* If *NAME points at an ABI tag, skip it and return true.  Otherwise
   leave *NAME unmodified and return false.  (see GCC's abi_tag
   attribute), such names are demangled as e.g.,
   "function[abi:cxx11]()".  */

static bool
skip_abi_tag (const char **name)
{
  const char *p = *name;

  if (startswith (p, "[abi:"))
    {
      p += 5;

      while (valid_identifier_name_char (*p))
	p++;

      if (*p == ']')
	{
	  p++;
	  *name = p;
	  return true;
	}
    }
  return false;
}

/* See utils.h.  */

int
strncmp_iw_with_mode (const char *string1, const char *string2,
		      size_t string2_len, strncmp_iw_mode mode,
		      enum language language,
		      completion_match_for_lcd *match_for_lcd)
{
  const char *string1_start = string1;
  const char *end_str2 = string2 + string2_len;
  bool skip_spaces = true;
  bool have_colon_op = (language == language_cplus
			|| language == language_rust
			|| language == language_fortran);

  while (1)
    {
      if (skip_spaces
	  || ((ISSPACE (*string1) && !valid_identifier_name_char (*string2))
	      || (ISSPACE (*string2) && !valid_identifier_name_char (*string1))))
	{
	  skip_ws (string1, string2, end_str2);
	  skip_spaces = false;
	}

      /* Skip [abi:cxx11] tags in the symbol name if the lookup name
	 doesn't include them.  E.g.:

	 string1: function[abi:cxx1](int)
	 string2: function

	 string1: function[abi:cxx1](int)
	 string2: function(int)

	 string1: Struct[abi:cxx1]::function()
	 string2: Struct::function()

	 string1: function(Struct[abi:cxx1], int)
	 string2: function(Struct, int)
      */
      if (string2 == end_str2
	  || (*string2 != '[' && !valid_identifier_name_char (*string2)))
	{
	  const char *abi_start = string1;

	  /* There can be more than one tag.  */
	  while (*string1 == '[' && skip_abi_tag (&string1))
	    ;

	  if (match_for_lcd != NULL && abi_start != string1)
	    match_for_lcd->mark_ignored_range (abi_start, string1);

	  while (ISSPACE (*string1))
	    string1++;
	}

      if (*string1 == '\0' || string2 == end_str2)
	break;

      /* Handle the :: operator.  */
      if (have_colon_op && string1[0] == ':' && string1[1] == ':')
	{
	  if (*string2 != ':')
	    return 1;

	  string1++;
	  string2++;

	  if (string2 == end_str2)
	    break;

	  if (*string2 != ':')
	    return 1;

	  string1++;
	  string2++;

	  while (ISSPACE (*string1))
	    string1++;
	  while (string2 < end_str2 && ISSPACE (*string2))
	    string2++;
	  continue;
	}

      /* Handle C++ user-defined operators.  */
      else if (language == language_cplus
	       && *string1 == 'o')
	{
	  if (cp_is_operator (string1, string1_start))
	    {
	      /* An operator name in STRING1.  Check STRING2.  */
	      size_t cmplen
		= std::min<size_t> (CP_OPERATOR_LEN, end_str2 - string2);
	      if (strncmp (string1, string2, cmplen) != 0)
		return 1;

	      string1 += cmplen;
	      string2 += cmplen;

	      if (string2 != end_str2)
		{
		  /* Check for "operatorX" in STRING2.  */
		  if (valid_identifier_name_char (*string2))
		    return 1;

		  skip_ws (string1, string2, end_str2);
		}

	      /* Handle operator().  */
	      if (*string1 == '(')
		{
		  if (string2 == end_str2)
		    {
		      if (mode == strncmp_iw_mode::NORMAL)
			return 0;
		      else
			{
			  /* Don't break for the regular return at the
			     bottom, because "operator" should not
			     match "operator()", since this open
			     parentheses is not the parameter list
			     start.  */
			  return *string1 != '\0';
			}
		    }

		  if (*string1 != *string2)
		    return 1;

		  string1++;
		  string2++;
		}

	      while (1)
		{
		  skip_ws (string1, string2, end_str2);

		  /* Skip to end of token, or to END, whatever comes
		     first.  */
		  const char *end_str1 = string1 + strlen (string1);
		  const char *p1 = cp_skip_operator_token (string1, end_str1);
		  const char *p2 = cp_skip_operator_token (string2, end_str2);

		  cmplen = std::min (p1 - string1, p2 - string2);
		  if (p2 == end_str2)
		    {
		      if (strncmp (string1, string2, cmplen) != 0)
			return 1;
		    }
		  else
		    {
		      if (p1 - string1 != p2 - string2)
			return 1;
		      if (strncmp (string1, string2, cmplen) != 0)
			return 1;
		    }

		  string1 += cmplen;
		  string2 += cmplen;

		  if (*string1 == '\0' || string2 == end_str2)
		    break;
		  if (*string1 == '(' || *string2 == '(')
		    break;
		}

	      continue;
	    }
	}

      if (case_sensitivity == case_sensitive_on && *string1 != *string2)
	break;
      if (case_sensitivity == case_sensitive_off
	  && (TOLOWER ((unsigned char) *string1)
	      != TOLOWER ((unsigned char) *string2)))
	break;

      /* If we see any non-whitespace, non-identifier-name character
	 (any of "()<>*&" etc.), then skip spaces the next time
	 around.  */
      if (!ISSPACE (*string1) && !valid_identifier_name_char (*string1))
	skip_spaces = true;

      string1++;
      string2++;
    }

  if (string2 == end_str2)
    {
      if (mode == strncmp_iw_mode::NORMAL)
	{
	  /* Strip abi tag markers from the matched symbol name.
	     Usually the ABI marker will be found on function name
	     (automatically added because the function returns an
	     object marked with an ABI tag).  However, it's also
	     possible to see a marker in one of the function
	     parameters, for example.

	     string2 (lookup name):
	       func
	     symbol name:
	       function(some_struct[abi:cxx11], int)

	     and for completion LCD computation we want to say that
	     the match was for:
	       function(some_struct, int)
	  */
	  if (match_for_lcd != NULL)
	    {
	      while ((string1 = strstr (string1, "[abi:")) != NULL)
		{
		  const char *abi_start = string1;

		  /* There can be more than one tag.  */
		  while (skip_abi_tag (&string1) && *string1 == '[')
		    ;

		  if (abi_start != string1)
		    match_for_lcd->mark_ignored_range (abi_start, string1);
		}
	    }

	  return 0;
	}
      else
	return (*string1 != '\0' && *string1 != '(');
    }
  else
    return 1;
}

/* See utils.h.  */

int
strncmp_iw (const char *string1, const char *string2, size_t string2_len)
{
  return strncmp_iw_with_mode (string1, string2, string2_len,
			       strncmp_iw_mode::NORMAL, language_minimal);
}

/* See utils.h.  */

int
strcmp_iw (const char *string1, const char *string2)
{
  return strncmp_iw_with_mode (string1, string2, strlen (string2),
			       strncmp_iw_mode::MATCH_PARAMS, language_minimal);
}

/* This is like strcmp except that it ignores whitespace and treats
   '(' as the first non-NULL character in terms of ordering.  Like
   strcmp (and unlike strcmp_iw), it returns negative if STRING1 <
   STRING2, 0 if STRING2 = STRING2, and positive if STRING1 > STRING2
   according to that ordering.

   If a list is sorted according to this function and if you want to
   find names in the list that match some fixed NAME according to
   strcmp_iw(LIST_ELT, NAME), then the place to start looking is right
   where this function would put NAME.

   This function must be neutral to the CASE_SENSITIVITY setting as the user
   may choose it during later lookup.  Therefore this function always sorts
   primarily case-insensitively and secondarily case-sensitively.

   Here are some examples of why using strcmp to sort is a bad idea:

   Whitespace example:

   Say your partial symtab contains: "foo<char *>", "goo".  Then, if
   we try to do a search for "foo<char*>", strcmp will locate this
   after "foo<char *>" and before "goo".  Then lookup_partial_symbol
   will start looking at strings beginning with "goo", and will never
   see the correct match of "foo<char *>".

   Parenthesis example:

   In practice, this is less like to be an issue, but I'll give it a
   shot.  Let's assume that '$' is a legitimate character to occur in
   symbols.  (Which may well even be the case on some systems.)  Then
   say that the partial symbol table contains "foo$" and "foo(int)".
   strcmp will put them in this order, since '$' < '('.  Now, if the
   user searches for "foo", then strcmp will sort "foo" before "foo$".
   Then lookup_partial_symbol will notice that strcmp_iw("foo$",
   "foo") is false, so it won't proceed to the actual match of
   "foo(int)" with "foo".  */

int
strcmp_iw_ordered (const char *string1, const char *string2)
{
  const char *saved_string1 = string1, *saved_string2 = string2;
  enum case_sensitivity case_pass = case_sensitive_off;

  for (;;)
    {
      /* C1 and C2 are valid only if *string1 != '\0' && *string2 != '\0'.
	 Provide stub characters if we are already at the end of one of the
	 strings.  */
      char c1 = 'X', c2 = 'X';

      while (*string1 != '\0' && *string2 != '\0')
	{
	  while (ISSPACE (*string1))
	    string1++;
	  while (ISSPACE (*string2))
	    string2++;

	  switch (case_pass)
	  {
	    case case_sensitive_off:
	      c1 = TOLOWER ((unsigned char) *string1);
	      c2 = TOLOWER ((unsigned char) *string2);
	      break;
	    case case_sensitive_on:
	      c1 = *string1;
	      c2 = *string2;
	      break;
	  }
	  if (c1 != c2)
	    break;

	  if (*string1 != '\0')
	    {
	      string1++;
	      string2++;
	    }
	}

      switch (*string1)
	{
	  /* Characters are non-equal unless they're both '\0'; we want to
	     make sure we get the comparison right according to our
	     comparison in the cases where one of them is '\0' or '('.  */
	case '\0':
	  if (*string2 == '\0')
	    break;
	  else
	    return -1;
	case '(':
	  if (*string2 == '\0')
	    return 1;
	  else
	    return -1;
	default:
	  if (*string2 == '\0' || *string2 == '(')
	    return 1;
	  else if (c1 > c2)
	    return 1;
	  else if (c1 < c2)
	    return -1;
	  /* PASSTHRU */
	}

      if (case_pass == case_sensitive_on)
	return 0;
      
      /* Otherwise the strings were equal in case insensitive way, make
	 a more fine grained comparison in a case sensitive way.  */

      case_pass = case_sensitive_on;
      string1 = saved_string1;
      string2 = saved_string2;
    }
}

/* See utils.h.  */

bool
streq (const char *lhs, const char *rhs)
{
  return !strcmp (lhs, rhs);
}

/* See utils.h.  */

int
streq_hash (const void *lhs, const void *rhs)
{
  return streq ((const char *) lhs, (const char *) rhs);
}



/*
   ** subset_compare()
   **    Answer whether string_to_compare is a full or partial match to
   **    template_string.  The partial match must be in sequence starting
   **    at index 0.
 */
int
subset_compare (const char *string_to_compare, const char *template_string)
{
  int match;

  if (template_string != NULL && string_to_compare != NULL
      && strlen (string_to_compare) <= strlen (template_string))
    match =
      (startswith (template_string, string_to_compare));
  else
    match = 0;
  return match;
}

static void
show_debug_timestamp (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Timestamping debugging messages is %s.\n"),
		    value);
}


/* See utils.h.  */

CORE_ADDR
address_significant (gdbarch *gdbarch, CORE_ADDR addr)
{
  /* Clear insignificant bits of a target address and sign extend resulting
     address, avoiding shifts larger or equal than the width of a CORE_ADDR.
     The local variable ADDR_BIT stops the compiler reporting a shift overflow
     when it won't occur.  Skip updating of target address if current target
     has not set gdbarch significant_addr_bit.  */
  int addr_bit = gdbarch_significant_addr_bit (gdbarch);

  if (addr_bit && (addr_bit < (sizeof (CORE_ADDR) * HOST_CHAR_BIT)))
    {
      CORE_ADDR sign = (CORE_ADDR) 1 << (addr_bit - 1);
      addr &= ((CORE_ADDR) 1 << addr_bit) - 1;
      addr = (addr ^ sign) - sign;
    }

  return addr;
}

const char *
paddress (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  /* Truncate address to the size of a target address, avoiding shifts
     larger or equal than the width of a CORE_ADDR.  The local
     variable ADDR_BIT stops the compiler reporting a shift overflow
     when it won't occur.  */
  /* NOTE: This assumes that the significant address information is
     kept in the least significant bits of ADDR - the upper bits were
     either zero or sign extended.  Should gdbarch_address_to_pointer or
     some ADDRESS_TO_PRINTABLE() be used to do the conversion?  */

  int addr_bit = gdbarch_addr_bit (gdbarch);

  if (addr_bit < (sizeof (CORE_ADDR) * HOST_CHAR_BIT))
    addr &= ((CORE_ADDR) 1 << addr_bit) - 1;
  return hex_string (addr);
}

/* This function is described in "defs.h".  */

const char *
print_core_address (struct gdbarch *gdbarch, CORE_ADDR address)
{
  int addr_bit = gdbarch_addr_bit (gdbarch);

  if (addr_bit < (sizeof (CORE_ADDR) * HOST_CHAR_BIT))
    address &= ((CORE_ADDR) 1 << addr_bit) - 1;

  /* FIXME: cagney/2002-05-03: Need local_address_string() function
     that returns the language localized string formatted to a width
     based on gdbarch_addr_bit.  */
  if (addr_bit <= 32)
    return hex_string_custom (address, 8);
  else
    return hex_string_custom (address, 16);
}

/* Callback hash_f for htab_create_alloc or htab_create_alloc_ex.  */

hashval_t
core_addr_hash (const void *ap)
{
  const CORE_ADDR *addrp = (const CORE_ADDR *) ap;

  return *addrp;
}

/* Callback eq_f for htab_create_alloc or htab_create_alloc_ex.  */

int
core_addr_eq (const void *ap, const void *bp)
{
  const CORE_ADDR *addr_ap = (const CORE_ADDR *) ap;
  const CORE_ADDR *addr_bp = (const CORE_ADDR *) bp;

  return *addr_ap == *addr_bp;
}

/* Convert a string back into a CORE_ADDR.  */
CORE_ADDR
string_to_core_addr (const char *my_string)
{
  CORE_ADDR addr = 0;

  if (my_string[0] == '0' && TOLOWER (my_string[1]) == 'x')
    {
      /* Assume that it is in hex.  */
      int i;

      for (i = 2; my_string[i] != '\0'; i++)
	{
	  if (ISDIGIT (my_string[i]))
	    addr = (my_string[i] - '0') + (addr * 16);
	  else if (ISXDIGIT (my_string[i]))
	    addr = (TOLOWER (my_string[i]) - 'a' + 0xa) + (addr * 16);
	  else
	    error (_("invalid hex \"%s\""), my_string);
	}
    }
  else
    {
      /* Assume that it is in decimal.  */
      int i;

      for (i = 0; my_string[i] != '\0'; i++)
	{
	  if (ISDIGIT (my_string[i]))
	    addr = (my_string[i] - '0') + (addr * 10);
	  else
	    error (_("invalid decimal \"%s\""), my_string);
	}
    }

  return addr;
}

#if GDB_SELF_TEST

static void
gdb_realpath_check_trailer (const char *input, const char *trailer)
{
  gdb::unique_xmalloc_ptr<char> result = gdb_realpath (input);

  size_t len = strlen (result.get ());
  size_t trail_len = strlen (trailer);

  SELF_CHECK (len >= trail_len
	      && strcmp (result.get () + len - trail_len, trailer) == 0);
}

static void
gdb_realpath_tests ()
{
  /* A file which contains a directory prefix.  */
  gdb_realpath_check_trailer ("./xfullpath.exp", "/xfullpath.exp");
  /* A file which contains a directory prefix.  */
  gdb_realpath_check_trailer ("../../defs.h", "/defs.h");
  /* A one-character filename.  */
  gdb_realpath_check_trailer ("./a", "/a");
  /* A file in the root directory.  */
  gdb_realpath_check_trailer ("/root_file_which_should_exist",
			      "/root_file_which_should_exist");
  /* A file which does not have a directory prefix.  */
  gdb_realpath_check_trailer ("xfullpath.exp", "xfullpath.exp");
  /* A one-char filename without any directory prefix.  */
  gdb_realpath_check_trailer ("a", "a");
  /* An empty filename.  */
  gdb_realpath_check_trailer ("", "");
}

/* Test the gdb_argv::as_array_view method.  */

static void
gdb_argv_as_array_view_test ()
{
  {
    gdb_argv argv;

    gdb::array_view<char *> view = argv.as_array_view ();

    SELF_CHECK (view.data () == nullptr);
    SELF_CHECK (view.size () == 0);
  }
  {
    gdb_argv argv ("une bonne 50");

    gdb::array_view<char *> view = argv.as_array_view ();

    SELF_CHECK (view.size () == 3);
    SELF_CHECK (strcmp (view[0], "une") == 0);
    SELF_CHECK (strcmp (view[1], "bonne") == 0);
    SELF_CHECK (strcmp (view[2], "50") == 0);
  }
}

#endif /* GDB_SELF_TEST */

/* Allocation function for the libiberty hash table which uses an
   obstack.  The obstack is passed as DATA.  */

void *
hashtab_obstack_allocate (void *data, size_t size, size_t count)
{
  size_t total = size * count;
  void *ptr = obstack_alloc ((struct obstack *) data, total);

  memset (ptr, 0, total);
  return ptr;
}

/* Trivial deallocation function for the libiberty splay tree and hash
   table - don't deallocate anything.  Rely on later deletion of the
   obstack.  DATA will be the obstack, although it is not needed
   here.  */

void
dummy_obstack_deallocate (void *object, void *data)
{
  return;
}

/* Simple, portable version of dirname that does not modify its
   argument.  */

std::string
ldirname (const char *filename)
{
  std::string dirname;
  const char *base = lbasename (filename);

  while (base > filename && IS_DIR_SEPARATOR (base[-1]))
    --base;

  if (base == filename)
    return dirname;

  dirname = std::string (filename, base - filename);

  /* On DOS based file systems, convert "d:foo" to "d:.", so that we
     create "d:./bar" later instead of the (different) "d:/bar".  */
  if (base - filename == 2 && IS_ABSOLUTE_PATH (base)
      && !IS_DIR_SEPARATOR (filename[0]))
    dirname[base++ - filename] = '.';

  return dirname;
}

/* See utils.h.  */

void
gdb_argv::reset (const char *s)
{
  char **argv = buildargv (s);

  freeargv (m_argv);
  m_argv = argv;
}

#define AMBIGUOUS_MESS1	".\nMatching formats:"
#define AMBIGUOUS_MESS2	\
  ".\nUse \"set gnutarget format-name\" to specify the format."

std::string
gdb_bfd_errmsg (bfd_error_type error_tag, char **matching)
{
  char **p;

  /* Check if errmsg just need simple return.  */
  if (error_tag != bfd_error_file_ambiguously_recognized || matching == NULL)
    return bfd_errmsg (error_tag);

  std::string ret (bfd_errmsg (error_tag));
  ret += AMBIGUOUS_MESS1;

  for (p = matching; *p; p++)
    {
      ret += " ";
      ret += *p;
    }
  ret += AMBIGUOUS_MESS2;

  xfree (matching);

  return ret;
}

/* Return ARGS parsed as a valid pid, or throw an error.  */

int
parse_pid_to_attach (const char *args)
{
  unsigned long pid;
  char *dummy;

  if (!args)
    error_no_arg (_("process-id to attach"));

  dummy = (char *) args;
  pid = strtoul (args, &dummy, 0);
  /* Some targets don't set errno on errors, grrr!  */
  if ((pid == 0 && dummy == args) || dummy != &args[strlen (args)])
    error (_("Illegal process-id: %s."), args);

  return pid;
}

/* Substitute all occurrences of string FROM by string TO in *STRINGP.  *STRINGP
   must come from xrealloc-compatible allocator and it may be updated.  FROM
   needs to be delimited by IS_DIR_SEPARATOR or DIRNAME_SEPARATOR (or be
   located at the start or end of *STRINGP.  */

void
substitute_path_component (char **stringp, const char *from, const char *to)
{
  char *string = *stringp, *s;
  const size_t from_len = strlen (from);
  const size_t to_len = strlen (to);

  for (s = string;;)
    {
      s = strstr (s, from);
      if (s == NULL)
	break;

      if ((s == string || IS_DIR_SEPARATOR (s[-1])
	   || s[-1] == DIRNAME_SEPARATOR)
          && (s[from_len] == '\0' || IS_DIR_SEPARATOR (s[from_len])
	      || s[from_len] == DIRNAME_SEPARATOR))
	{
	  char *string_new;

	  string_new
	    = (char *) xrealloc (string, (strlen (string) + to_len + 1));

	  /* Relocate the current S pointer.  */
	  s = s - string + string_new;
	  string = string_new;

	  /* Replace from by to.  */
	  memmove (&s[to_len], &s[from_len], strlen (&s[from_len]) + 1);
	  memcpy (s, to, to_len);

	  s += to_len;
	}
      else
	s++;
    }

  *stringp = string;
}

#ifdef HAVE_WAITPID

#ifdef SIGALRM

/* SIGALRM handler for waitpid_with_timeout.  */

static void
sigalrm_handler (int signo)
{
  /* Nothing to do.  */
}

#endif

/* Wrapper to wait for child PID to die with TIMEOUT.
   TIMEOUT is the time to stop waiting in seconds.
   If TIMEOUT is zero, pass WNOHANG to waitpid.
   Returns PID if it was successfully waited for, otherwise -1.

   Timeouts are currently implemented with alarm and SIGALRM.
   If the host does not support them, this waits "forever".
   It would be odd though for a host to have waitpid and not SIGALRM.  */

pid_t
wait_to_die_with_timeout (pid_t pid, int *status, int timeout)
{
  pid_t waitpid_result;

  gdb_assert (pid > 0);
  gdb_assert (timeout >= 0);

  if (timeout > 0)
    {
#ifdef SIGALRM
#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
      struct sigaction sa, old_sa;

      sa.sa_handler = sigalrm_handler;
      sigemptyset (&sa.sa_mask);
      sa.sa_flags = 0;
      sigaction (SIGALRM, &sa, &old_sa);
#else
      sighandler_t ofunc;

      ofunc = signal (SIGALRM, sigalrm_handler);
#endif

      alarm (timeout);
#endif

      waitpid_result = waitpid (pid, status, 0);

#ifdef SIGALRM
      alarm (0);
#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
      sigaction (SIGALRM, &old_sa, NULL);
#else
      signal (SIGALRM, ofunc);
#endif
#endif
    }
  else
    waitpid_result = waitpid (pid, status, WNOHANG);

  if (waitpid_result == pid)
    return pid;
  else
    return -1;
}

#endif /* HAVE_WAITPID */

/* Provide fnmatch compatible function for FNM_FILE_NAME matching of host files.
   Both FNM_FILE_NAME and FNM_NOESCAPE must be set in FLAGS.

   It handles correctly HAVE_DOS_BASED_FILE_SYSTEM and
   HAVE_CASE_INSENSITIVE_FILE_SYSTEM.  */

int
gdb_filename_fnmatch (const char *pattern, const char *string, int flags)
{
  gdb_assert ((flags & FNM_FILE_NAME) != 0);

  /* It is unclear how '\' escaping vs. directory separator should coexist.  */
  gdb_assert ((flags & FNM_NOESCAPE) != 0);

#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  {
    char *pattern_slash, *string_slash;

    /* Replace '\' by '/' in both strings.  */

    pattern_slash = (char *) alloca (strlen (pattern) + 1);
    strcpy (pattern_slash, pattern);
    pattern = pattern_slash;
    for (; *pattern_slash != 0; pattern_slash++)
      if (IS_DIR_SEPARATOR (*pattern_slash))
	*pattern_slash = '/';

    string_slash = (char *) alloca (strlen (string) + 1);
    strcpy (string_slash, string);
    string = string_slash;
    for (; *string_slash != 0; string_slash++)
      if (IS_DIR_SEPARATOR (*string_slash))
	*string_slash = '/';
  }
#endif /* HAVE_DOS_BASED_FILE_SYSTEM */

#ifdef HAVE_CASE_INSENSITIVE_FILE_SYSTEM
  flags |= FNM_CASEFOLD;
#endif /* HAVE_CASE_INSENSITIVE_FILE_SYSTEM */

  return fnmatch (pattern, string, flags);
}

/* Return the number of path elements in PATH.
   / = 1
   /foo = 2
   /foo/ = 2
   foo/bar = 2
   foo/ = 1  */

int
count_path_elements (const char *path)
{
  int count = 0;
  const char *p = path;

  if (HAS_DRIVE_SPEC (p))
    {
      p = STRIP_DRIVE_SPEC (p);
      ++count;
    }

  while (*p != '\0')
    {
      if (IS_DIR_SEPARATOR (*p))
	++count;
      ++p;
    }

  /* Backup one if last character is /, unless it's the only one.  */
  if (p > path + 1 && IS_DIR_SEPARATOR (p[-1]))
    --count;

  /* Add one for the file name, if present.  */
  if (p > path && !IS_DIR_SEPARATOR (p[-1]))
    ++count;

  return count;
}

/* Remove N leading path elements from PATH.
   N must be non-negative.
   If PATH has more than N path elements then return NULL.
   If PATH has exactly N path elements then return "".
   See count_path_elements for a description of how we do the counting.  */

const char *
strip_leading_path_elements (const char *path, int n)
{
  int i = 0;
  const char *p = path;

  gdb_assert (n >= 0);

  if (n == 0)
    return p;

  if (HAS_DRIVE_SPEC (p))
    {
      p = STRIP_DRIVE_SPEC (p);
      ++i;
    }

  while (i < n)
    {
      while (*p != '\0' && !IS_DIR_SEPARATOR (*p))
	++p;
      if (*p == '\0')
	{
	  if (i + 1 == n)
	    return "";
	  return NULL;
	}
      ++p;
      ++i;
    }

  return p;
}

/* See utils.h.  */

void
copy_bitwise (gdb_byte *dest, ULONGEST dest_offset,
	      const gdb_byte *source, ULONGEST source_offset,
	      ULONGEST nbits, int bits_big_endian)
{
  unsigned int buf, avail;

  if (nbits == 0)
    return;

  if (bits_big_endian)
    {
      /* Start from the end, then work backwards.  */
      dest_offset += nbits - 1;
      dest += dest_offset / 8;
      dest_offset = 7 - dest_offset % 8;
      source_offset += nbits - 1;
      source += source_offset / 8;
      source_offset = 7 - source_offset % 8;
    }
  else
    {
      dest += dest_offset / 8;
      dest_offset %= 8;
      source += source_offset / 8;
      source_offset %= 8;
    }

  /* Fill BUF with DEST_OFFSET bits from the destination and 8 -
     SOURCE_OFFSET bits from the source.  */
  buf = *(bits_big_endian ? source-- : source++) >> source_offset;
  buf <<= dest_offset;
  buf |= *dest & ((1 << dest_offset) - 1);

  /* NBITS: bits yet to be written; AVAIL: BUF's fill level.  */
  nbits += dest_offset;
  avail = dest_offset + 8 - source_offset;

  /* Flush 8 bits from BUF, if appropriate.  */
  if (nbits >= 8 && avail >= 8)
    {
      *(bits_big_endian ? dest-- : dest++) = buf;
      buf >>= 8;
      avail -= 8;
      nbits -= 8;
    }

  /* Copy the middle part.  */
  if (nbits >= 8)
    {
      size_t len = nbits / 8;

      /* Use a faster method for byte-aligned copies.  */
      if (avail == 0)
	{
	  if (bits_big_endian)
	    {
	      dest -= len;
	      source -= len;
	      memcpy (dest + 1, source + 1, len);
	    }
	  else
	    {
	      memcpy (dest, source, len);
	      dest += len;
	      source += len;
	    }
	}
      else
	{
	  while (len--)
	    {
	      buf |= *(bits_big_endian ? source-- : source++) << avail;
	      *(bits_big_endian ? dest-- : dest++) = buf;
	      buf >>= 8;
	    }
	}
      nbits %= 8;
    }

  /* Write the last byte.  */
  if (nbits)
    {
      if (avail < nbits)
	buf |= *source << avail;

      buf &= (1 << nbits) - 1;
      *dest = (*dest & (~0U << nbits)) | buf;
    }
}

void _initialize_utils ();
void
_initialize_utils ()
{
  add_setshow_uinteger_cmd ("width", class_support, &chars_per_line, _("\
Set number of characters where GDB should wrap lines of its output."), _("\
Show number of characters where GDB should wrap lines of its output."), _("\
This affects where GDB wraps its output to fit the screen width.\n\
Setting this to \"unlimited\" or zero prevents GDB from wrapping its output."),
			    set_width_command,
			    show_chars_per_line,
			    &setlist, &showlist);

  add_setshow_uinteger_cmd ("height", class_support, &lines_per_page, _("\
Set number of lines in a page for GDB output pagination."), _("\
Show number of lines in a page for GDB output pagination."), _("\
This affects the number of lines after which GDB will pause\n\
its output and ask you whether to continue.\n\
Setting this to \"unlimited\" or zero causes GDB never pause during output."),
			    set_height_command,
			    show_lines_per_page,
			    &setlist, &showlist);

  add_setshow_boolean_cmd ("pagination", class_support,
			   &pagination_enabled, _("\
Set state of GDB output pagination."), _("\
Show state of GDB output pagination."), _("\
When pagination is ON, GDB pauses at end of each screenful of\n\
its output and asks you whether to continue.\n\
Turning pagination off is an alternative to \"set height unlimited\"."),
			   NULL,
			   show_pagination_enabled,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("sevenbit-strings", class_support,
			   &sevenbit_strings, _("\
Set printing of 8-bit characters in strings as \\nnn."), _("\
Show printing of 8-bit characters in strings as \\nnn."), NULL,
			   NULL,
			   show_sevenbit_strings,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("timestamp", class_maintenance,
			    &debug_timestamp, _("\
Set timestamping of debugging messages."), _("\
Show timestamping of debugging messages."), _("\
When set, debugging messages will be marked with seconds and microseconds."),
			   NULL,
			   show_debug_timestamp,
			   &setdebuglist, &showdebuglist);

  add_internal_problem_command (&internal_error_problem);
  add_internal_problem_command (&internal_warning_problem);
  add_internal_problem_command (&demangler_warning_problem);

#if GDB_SELF_TEST
  selftests::register_test ("gdb_realpath", gdb_realpath_tests);
  selftests::register_test ("gdb_argv_array_view", gdb_argv_as_array_view_test);
#endif
}
