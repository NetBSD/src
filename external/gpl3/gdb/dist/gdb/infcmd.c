/* Memory-access and commands for "inferior" process, for GDB.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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
#include <signal.h>
#include "symtab.h"
#include "gdbtypes.h"
#include "frame.h"
#include "inferior.h"
#include "infrun.h"
#include "environ.h"
#include "value.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "gdbcore.h"
#include "target.h"
#include "language.h"
#include "objfiles.h"
#include "completer.h"
#include "ui-out.h"
#include "event-top.h"
#include "parser-defs.h"
#include "regcache.h"
#include "reggroups.h"
#include "block.h"
#include "solib.h"
#include <ctype.h>
#include "observer.h"
#include "target-descriptions.h"
#include "user-regs.h"
#include "cli/cli-decode.h"
#include "gdbthread.h"
#include "valprint.h"
#include "inline-frame.h"
#include "tracepoint.h"
#include "inf-loop.h"
#include "continuations.h"
#include "linespec.h"
#include "cli/cli-utils.h"
#include "infcall.h"
#include "thread-fsm.h"
#include "top.h"
#include "interps.h"

/* Local functions: */

static void nofp_registers_info (char *, int);

static void until_next_command (int);

static void until_command (char *, int);

static void path_info (char *, int);

static void path_command (char *, int);

static void unset_command (char *, int);

static void float_info (char *, int);

static void disconnect_command (char *, int);

static void unset_environment_command (char *, int);

static void set_environment_command (char *, int);

static void environment_info (char *, int);

static void program_info (char *, int);

static void finish_command (char *, int);

static void signal_command (char *, int);

static void jump_command (char *, int);

static void step_1 (int, int, char *);

static void next_command (char *, int);

static void step_command (char *, int);

static void run_command (char *, int);

void _initialize_infcmd (void);

#define ERROR_NO_INFERIOR \
   if (!target_has_execution) error (_("The program is not being run."));

/* Scratch area where string containing arguments to give to the
   program will be stored by 'set args'.  As soon as anything is
   stored, notice_args_set will move it into per-inferior storage.
   Arguments are separated by spaces.  Empty string (pointer to '\0')
   means no args.  */

static char *inferior_args_scratch;

/* Scratch area where 'set inferior-tty' will store user-provided value.
   We'll immediate copy it into per-inferior storage.  */

static char *inferior_io_terminal_scratch;

/* Pid of our debugged inferior, or 0 if no inferior now.
   Since various parts of infrun.c test this to see whether there is a program
   being debugged it should be nonzero (currently 3 is used) for remote
   debugging.  */

ptid_t inferior_ptid;

/* Address at which inferior stopped.  */

CORE_ADDR stop_pc;

/* Nonzero if stopped due to completion of a stack dummy routine.  */

enum stop_stack_kind stop_stack_dummy;

/* Nonzero if stopped due to a random (unexpected) signal in inferior
   process.  */

int stopped_by_random_signal;

/* See inferior.h.  */

int startup_with_shell = 1;


/* Accessor routines.  */

/* Set the io terminal for the current inferior.  Ownership of
   TERMINAL_NAME is not transferred.  */

void 
set_inferior_io_terminal (const char *terminal_name)
{
  xfree (current_inferior ()->terminal);

  if (terminal_name != NULL && *terminal_name != '\0')
    current_inferior ()->terminal = xstrdup (terminal_name);
  else
    current_inferior ()->terminal = NULL;
}

const char *
get_inferior_io_terminal (void)
{
  return current_inferior ()->terminal;
}

static void
set_inferior_tty_command (char *args, int from_tty,
			  struct cmd_list_element *c)
{
  /* CLI has assigned the user-provided value to inferior_io_terminal_scratch.
     Now route it to current inferior.  */
  set_inferior_io_terminal (inferior_io_terminal_scratch);
}

static void
show_inferior_tty_command (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  /* Note that we ignore the passed-in value in favor of computing it
     directly.  */
  const char *inferior_io_terminal = get_inferior_io_terminal ();

  if (inferior_io_terminal == NULL)
    inferior_io_terminal = "";
  fprintf_filtered (gdb_stdout,
		    _("Terminal for future runs of program being debugged "
		      "is \"%s\".\n"), inferior_io_terminal);
}

char *
get_inferior_args (void)
{
  if (current_inferior ()->argc != 0)
    {
      char *n;

      n = construct_inferior_arguments (current_inferior ()->argc,
					current_inferior ()->argv);
      set_inferior_args (n);
      xfree (n);
    }

  if (current_inferior ()->args == NULL)
    current_inferior ()->args = xstrdup ("");

  return current_inferior ()->args;
}

/* Set the arguments for the current inferior.  Ownership of
   NEWARGS is not transferred.  */

void
set_inferior_args (char *newargs)
{
  xfree (current_inferior ()->args);
  current_inferior ()->args = newargs ? xstrdup (newargs) : NULL;
  current_inferior ()->argc = 0;
  current_inferior ()->argv = 0;
}

void
set_inferior_args_vector (int argc, char **argv)
{
  current_inferior ()->argc = argc;
  current_inferior ()->argv = argv;
}

/* Notice when `set args' is run.  */

static void
set_args_command (char *args, int from_tty, struct cmd_list_element *c)
{
  /* CLI has assigned the user-provided value to inferior_args_scratch.
     Now route it to current inferior.  */
  set_inferior_args (inferior_args_scratch);
}

/* Notice when `show args' is run.  */

static void
show_args_command (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  /* Note that we ignore the passed-in value in favor of computing it
     directly.  */
  deprecated_show_value_hack (file, from_tty, c, get_inferior_args ());
}


/* Compute command-line string given argument vector.  This does the
   same shell processing as fork_inferior.  */

char *
construct_inferior_arguments (int argc, char **argv)
{
  char *result;

  if (startup_with_shell)
    {
#ifdef __MINGW32__
      /* This holds all the characters considered special to the
	 Windows shells.  */
      static const char special[] = "\"!&*|[]{}<>?`~^=;, \t\n";
      static const char quote = '"';
#else
      /* This holds all the characters considered special to the
	 typical Unix shells.  We include `^' because the SunOS
	 /bin/sh treats it as a synonym for `|'.  */
      static const char special[] = "\"!#$&*()\\|[]{}<>?'`~^; \t\n";
      static const char quote = '\'';
#endif
      int i;
      int length = 0;
      char *out, *cp;

      /* We over-compute the size.  It shouldn't matter.  */
      for (i = 0; i < argc; ++i)
	length += 3 * strlen (argv[i]) + 1 + 2 * (argv[i][0] == '\0');

      result = (char *) xmalloc (length);
      out = result;

      for (i = 0; i < argc; ++i)
	{
	  if (i > 0)
	    *out++ = ' ';

	  /* Need to handle empty arguments specially.  */
	  if (argv[i][0] == '\0')
	    {
	      *out++ = quote;
	      *out++ = quote;
	    }
	  else
	    {
#ifdef __MINGW32__
	      int quoted = 0;

	      if (strpbrk (argv[i], special))
		{
		  quoted = 1;
		  *out++ = quote;
		}
#endif
	      for (cp = argv[i]; *cp; ++cp)
		{
		  if (*cp == '\n')
		    {
		      /* A newline cannot be quoted with a backslash (it
			 just disappears), only by putting it inside
			 quotes.  */
		      *out++ = quote;
		      *out++ = '\n';
		      *out++ = quote;
		    }
		  else
		    {
#ifdef __MINGW32__
		      if (*cp == quote)
#else
		      if (strchr (special, *cp) != NULL)
#endif
			*out++ = '\\';
		      *out++ = *cp;
		    }
		}
#ifdef __MINGW32__
	      if (quoted)
		*out++ = quote;
#endif
	    }
	}
      *out = '\0';
    }
  else
    {
      /* In this case we can't handle arguments that contain spaces,
	 tabs, or newlines -- see breakup_args().  */
      int i;
      int length = 0;

      for (i = 0; i < argc; ++i)
	{
	  char *cp = strchr (argv[i], ' ');
	  if (cp == NULL)
	    cp = strchr (argv[i], '\t');
	  if (cp == NULL)
	    cp = strchr (argv[i], '\n');
	  if (cp != NULL)
	    error (_("can't handle command-line "
		     "argument containing whitespace"));
	  length += strlen (argv[i]) + 1;
	}

      result = (char *) xmalloc (length);
      result[0] = '\0';
      for (i = 0; i < argc; ++i)
	{
	  if (i > 0)
	    strcat (result, " ");
	  strcat (result, argv[i]);
	}
    }

  return result;
}


/* This function strips the '&' character (indicating background
   execution) that is added as *the last* of the arguments ARGS of a
   command.  A copy of the incoming ARGS without the '&' is returned,
   unless the resulting string after stripping is empty, in which case
   NULL is returned.  *BG_CHAR_P is an output boolean that indicates
   whether the '&' character was found.  */

static char *
strip_bg_char (const char *args, int *bg_char_p)
{
  const char *p;

  if (args == NULL || *args == '\0')
    {
      *bg_char_p = 0;
      return NULL;
    }

  p = args + strlen (args);
  if (p[-1] == '&')
    {
      p--;
      while (p > args && isspace (p[-1]))
	p--;

      *bg_char_p = 1;
      if (p != args)
	return savestring (args, p - args);
      else
	return NULL;
    }

  *bg_char_p = 0;
  return xstrdup (args);
}

/* Common actions to take after creating any sort of inferior, by any
   means (running, attaching, connecting, et cetera).  The target
   should be stopped.  */

void
post_create_inferior (struct target_ops *target, int from_tty)
{

  /* Be sure we own the terminal in case write operations are performed.  */ 
  target_terminal_ours_for_output ();

  /* If the target hasn't taken care of this already, do it now.
     Targets which need to access registers during to_open,
     to_create_inferior, or to_attach should do it earlier; but many
     don't need to.  */
  target_find_description ();

  /* Now that we know the register layout, retrieve current PC.  But
     if the PC is unavailable (e.g., we're opening a core file with
     missing registers info), ignore it.  */
  stop_pc = 0;
  TRY
    {
      stop_pc = regcache_read_pc (get_current_regcache ());
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw_exception (ex);
    }
  END_CATCH

  if (exec_bfd)
    {
      const unsigned solib_add_generation
	= current_program_space->solib_add_generation;

      /* Create the hooks to handle shared library load and unload
	 events.  */
      solib_create_inferior_hook (from_tty);

      if (current_program_space->solib_add_generation == solib_add_generation)
	{
	  /* The platform-specific hook should load initial shared libraries,
	     but didn't.  FROM_TTY will be incorrectly 0 but such solib
	     targets should be fixed anyway.  Call it only after the solib
	     target has been initialized by solib_create_inferior_hook.  */

	  if (info_verbose)
	    warning (_("platform-specific solib_create_inferior_hook did "
		       "not load initial shared libraries."));

	  /* If the solist is global across processes, there's no need to
	     refetch it here.  */
	  if (!gdbarch_has_global_solist (target_gdbarch ()))
	    solib_add (NULL, 0, auto_solib_add);
	}
    }

  /* If the user sets watchpoints before execution having started,
     then she gets software watchpoints, because GDB can't know which
     target will end up being pushed, or if it supports hardware
     watchpoints or not.  breakpoint_re_set takes care of promoting
     watchpoints to hardware watchpoints if possible, however, if this
     new inferior doesn't load shared libraries or we don't pull in
     symbols from any other source on this target/arch,
     breakpoint_re_set is never called.  Call it now so that software
     watchpoints get a chance to be promoted to hardware watchpoints
     if the now pushed target supports hardware watchpoints.  */
  breakpoint_re_set ();

  observer_notify_inferior_created (target, from_tty);
}

/* Kill the inferior if already running.  This function is designed
   to be called when we are about to start the execution of the program
   from the beginning.  Ask the user to confirm that he wants to restart
   the program being debugged when FROM_TTY is non-null.  */

static void
kill_if_already_running (int from_tty)
{
  if (! ptid_equal (inferior_ptid, null_ptid) && target_has_execution)
    {
      /* Bail out before killing the program if we will not be able to
	 restart it.  */
      target_require_runnable ();

      if (from_tty
	  && !query (_("The program being debugged has been started already.\n\
Start it from the beginning? ")))
	error (_("Program not restarted."));
      target_kill ();
    }
}

/* See inferior.h.  */

void
prepare_execution_command (struct target_ops *target, int background)
{
  /* If we get a request for running in the bg but the target
     doesn't support it, error out.  */
  if (background && !target->to_can_async_p (target))
    error (_("Asynchronous execution not supported on this target."));

  if (!background)
    {
      /* If we get a request for running in the fg, then we need to
	 simulate synchronous (fg) execution.  Note no cleanup is
	 necessary for this.  stdin is re-enabled whenever an error
	 reaches the top level.  */
      all_uis_on_sync_execution_starting ();
    }
}

/* Implement the "run" command.  If TBREAK_AT_MAIN is set, then insert
   a temporary breakpoint at the begining of the main program before
   running the program.  */

static void
run_command_1 (char *args, int from_tty, int tbreak_at_main)
{
  const char *exec_file;
  struct cleanup *old_chain;
  ptid_t ptid;
  struct ui_out *uiout = current_uiout;
  struct target_ops *run_target;
  int async_exec;
  struct cleanup *args_chain;

  dont_repeat ();

  kill_if_already_running (from_tty);

  init_wait_for_inferior ();
  clear_breakpoint_hit_counts ();

  /* Clean up any leftovers from other runs.  Some other things from
     this function should probably be moved into target_pre_inferior.  */
  target_pre_inferior (from_tty);

  /* The comment here used to read, "The exec file is re-read every
     time we do a generic_mourn_inferior, so we just have to worry
     about the symbol file."  The `generic_mourn_inferior' function
     gets called whenever the program exits.  However, suppose the
     program exits, and *then* the executable file changes?  We need
     to check again here.  Since reopen_exec_file doesn't do anything
     if the timestamp hasn't changed, I don't see the harm.  */
  reopen_exec_file ();
  reread_symbols ();

  args = strip_bg_char (args, &async_exec);
  args_chain = make_cleanup (xfree, args);

  /* Do validation and preparation before possibly changing anything
     in the inferior.  */

  run_target = find_run_target ();

  prepare_execution_command (run_target, async_exec);

  if (non_stop && !run_target->to_supports_non_stop (run_target))
    error (_("The target does not support running in non-stop mode."));

  /* Done.  Can now set breakpoints, change inferior args, etc.  */

  /* Insert the temporary breakpoint if a location was specified.  */
  if (tbreak_at_main)
    tbreak_command (main_name (), 0);

  exec_file = get_exec_file (0);

  /* We keep symbols from add-symbol-file, on the grounds that the
     user might want to add some symbols before running the program
     (right?).  But sometimes (dynamic loading where the user manually
     introduces the new symbols with add-symbol-file), the code which
     the symbols describe does not persist between runs.  Currently
     the user has to manually nuke all symbols between runs if they
     want them to go away (PR 2207).  This is probably reasonable.  */

  /* If there were other args, beside '&', process them.  */
  if (args != NULL)
    set_inferior_args (args);

  if (from_tty)
    {
      uiout->field_string (NULL, "Starting program");
      uiout->text (": ");
      if (exec_file)
	uiout->field_string ("execfile", exec_file);
      uiout->spaces (1);
      /* We call get_inferior_args() because we might need to compute
	 the value now.  */
      uiout->field_string ("infargs", get_inferior_args ());
      uiout->text ("\n");
      uiout->flush ();
    }

  /* Done with ARGS.  */
  do_cleanups (args_chain);

  /* We call get_inferior_args() because we might need to compute
     the value now.  */
  run_target->to_create_inferior (run_target, exec_file,
				  std::string (get_inferior_args ()),
				  environ_vector (current_inferior ()->environment),
				  from_tty);
  /* to_create_inferior should push the target, so after this point we
     shouldn't refer to run_target again.  */
  run_target = NULL;

  /* We're starting off a new process.  When we get out of here, in
     non-stop mode, finish the state of all threads of that process,
     but leave other threads alone, as they may be stopped in internal
     events --- the frontend shouldn't see them as stopped.  In
     all-stop, always finish the state of all threads, as we may be
     resuming more than just the new process.  */
  if (non_stop)
    ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));
  else
    ptid = minus_one_ptid;
  old_chain = make_cleanup (finish_thread_state_cleanup, &ptid);

  /* Pass zero for FROM_TTY, because at this point the "run" command
     has done its thing; now we are setting up the running program.  */
  post_create_inferior (&current_target, 0);

  /* Start the target running.  Do not use -1 continuation as it would skip
     breakpoint right at the entry point.  */
  proceed (regcache_read_pc (get_current_regcache ()), GDB_SIGNAL_0);

  /* Since there was no error, there's no need to finish the thread
     states here.  */
  discard_cleanups (old_chain);
}

static void
run_command (char *args, int from_tty)
{
  run_command_1 (args, from_tty, 0);
}

/* Start the execution of the program up until the beginning of the main
   program.  */

static void
start_command (char *args, int from_tty)
{
  /* Some languages such as Ada need to search inside the program
     minimal symbols for the location where to put the temporary
     breakpoint before starting.  */
  if (!have_minimal_symbols ())
    error (_("No symbol table loaded.  Use the \"file\" command."));

  /* Run the program until reaching the main procedure...  */
  run_command_1 (args, from_tty, 1);
} 

static int
proceed_thread_callback (struct thread_info *thread, void *arg)
{
  /* We go through all threads individually instead of compressing
     into a single target `resume_all' request, because some threads
     may be stopped in internal breakpoints/events, or stopped waiting
     for its turn in the displaced stepping queue (that is, they are
     running && !executing).  The target side has no idea about why
     the thread is stopped, so a `resume_all' command would resume too
     much.  If/when GDB gains a way to tell the target `hold this
     thread stopped until I say otherwise', then we can optimize
     this.  */
  if (!is_stopped (thread->ptid))
    return 0;

  switch_to_thread (thread->ptid);
  clear_proceed_status (0);
  proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
  return 0;
}

static void
ensure_valid_thread (void)
{
  if (ptid_equal (inferior_ptid, null_ptid)
      || is_exited (inferior_ptid))
    error (_("Cannot execute this command without a live selected thread."));
}

/* If the user is looking at trace frames, any resumption of execution
   is likely to mix up recorded and live target data.  So simply
   disallow those commands.  */

static void
ensure_not_tfind_mode (void)
{
  if (get_traceframe_number () >= 0)
    error (_("Cannot execute this command while looking at trace frames."));
}

/* Throw an error indicating the current thread is running.  */

static void
error_is_running (void)
{
  error (_("Cannot execute this command while "
	   "the selected thread is running."));
}

/* Calls error_is_running if the current thread is running.  */

static void
ensure_not_running (void)
{
  if (is_running (inferior_ptid))
    error_is_running ();
}

void
continue_1 (int all_threads)
{
  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();

  if (non_stop && all_threads)
    {
      /* Don't error out if the current thread is running, because
	 there may be other stopped threads.  */
      struct cleanup *old_chain;

      /* Backup current thread and selected frame.  */
      old_chain = make_cleanup_restore_current_thread ();

      iterate_over_threads (proceed_thread_callback, NULL);

      if (current_ui->prompt_state == PROMPT_BLOCKED)
	{
	  /* If all threads in the target were already running,
	     proceed_thread_callback ends up never calling proceed,
	     and so nothing calls this to put the inferior's terminal
	     settings in effect and remove stdin from the event loop,
	     which we must when running a foreground command.  E.g.:

	      (gdb) c -a&
	      Continuing.
	      <all threads are running now>
	      (gdb) c -a
	      Continuing.
	      <no thread was resumed, but the inferior now owns the terminal>
	  */
	  target_terminal_inferior ();
	}

      /* Restore selected ptid.  */
      do_cleanups (old_chain);
    }
  else
    {
      ensure_valid_thread ();
      ensure_not_running ();
      clear_proceed_status (0);
      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }
}

/* continue [-a] [proceed-count] [&]  */

static void
continue_command (char *args, int from_tty)
{
  int async_exec;
  int all_threads = 0;
  struct cleanup *args_chain;

  ERROR_NO_INFERIOR;

  /* Find out whether we must run in the background.  */
  args = strip_bg_char (args, &async_exec);
  args_chain = make_cleanup (xfree, args);

  if (args != NULL)
    {
      if (startswith (args, "-a"))
	{
	  all_threads = 1;
	  args += sizeof ("-a") - 1;
	  if (*args == '\0')
	    args = NULL;
	}
    }

  if (!non_stop && all_threads)
    error (_("`-a' is meaningless in all-stop mode."));

  if (args != NULL && all_threads)
    error (_("Can't resume all threads and specify "
	     "proceed count simultaneously."));

  /* If we have an argument left, set proceed count of breakpoint we
     stopped at.  */
  if (args != NULL)
    {
      bpstat bs = NULL;
      int num, stat;
      int stopped = 0;
      struct thread_info *tp;

      if (non_stop)
	tp = find_thread_ptid (inferior_ptid);
      else
	{
	  ptid_t last_ptid;
	  struct target_waitstatus ws;

	  get_last_target_status (&last_ptid, &ws);
	  tp = find_thread_ptid (last_ptid);
	}
      if (tp != NULL)
	bs = tp->control.stop_bpstat;

      while ((stat = bpstat_num (&bs, &num)) != 0)
	if (stat > 0)
	  {
	    set_ignore_count (num,
			      parse_and_eval_long (args) - 1,
			      from_tty);
	    /* set_ignore_count prints a message ending with a period.
	       So print two spaces before "Continuing.".  */
	    if (from_tty)
	      printf_filtered ("  ");
	    stopped = 1;
	  }

      if (!stopped && from_tty)
	{
	  printf_filtered
	    ("Not stopped at any breakpoint; argument ignored.\n");
	}
    }

  /* Done with ARGS.  */
  do_cleanups (args_chain);

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();

  if (!non_stop || !all_threads)
    {
      ensure_valid_thread ();
      ensure_not_running ();
    }

  prepare_execution_command (&current_target, async_exec);

  if (from_tty)
    printf_filtered (_("Continuing.\n"));

  continue_1 (all_threads);
}

/* Record the starting point of a "step" or "next" command.  */

static void
set_step_frame (void)
{
  struct symtab_and_line sal;
  CORE_ADDR pc;
  struct frame_info *frame = get_current_frame ();
  struct thread_info *tp = inferior_thread ();

  find_frame_sal (frame, &sal);
  set_step_info (frame, sal);
  pc = get_frame_pc (frame);
  tp->control.step_start_function = find_pc_function (pc);
}

/* Step until outside of current statement.  */

static void
step_command (char *count_string, int from_tty)
{
  step_1 (0, 0, count_string);
}

/* Likewise, but skip over subroutine calls as if single instructions.  */

static void
next_command (char *count_string, int from_tty)
{
  step_1 (1, 0, count_string);
}

/* Likewise, but step only one instruction.  */

static void
stepi_command (char *count_string, int from_tty)
{
  step_1 (0, 1, count_string);
}

static void
nexti_command (char *count_string, int from_tty)
{
  step_1 (1, 1, count_string);
}

void
delete_longjmp_breakpoint_cleanup (void *arg)
{
  int thread = * (int *) arg;
  delete_longjmp_breakpoint (thread);
}

/* Data for the FSM that manages the step/next/stepi/nexti
   commands.  */

struct step_command_fsm
{
  /* The base class.  */
  struct thread_fsm thread_fsm;

  /* How many steps left in a "step N"-like command.  */
  int count;

  /* If true, this is a next/nexti, otherwise a step/stepi.  */
  int skip_subroutines;

  /* If true, this is a stepi/nexti, otherwise a step/step.  */
  int single_inst;
};

static void step_command_fsm_clean_up (struct thread_fsm *self,
				       struct thread_info *thread);
static int step_command_fsm_should_stop (struct thread_fsm *self,
					 struct thread_info *thread);
static enum async_reply_reason
  step_command_fsm_async_reply_reason (struct thread_fsm *self);

/* step_command_fsm's vtable.  */

static struct thread_fsm_ops step_command_fsm_ops =
{
  NULL,
  step_command_fsm_clean_up,
  step_command_fsm_should_stop,
  NULL,	/* return_value */
  step_command_fsm_async_reply_reason,
};

/* Allocate a new step_command_fsm.  */

static struct step_command_fsm *
new_step_command_fsm (struct interp *cmd_interp)
{
  struct step_command_fsm *sm;

  sm = XCNEW (struct step_command_fsm);
  thread_fsm_ctor (&sm->thread_fsm, &step_command_fsm_ops, cmd_interp);

  return sm;
}

/* Prepare for a step/next/etc. command.  Any target resource
   allocated here is undone in the FSM's clean_up method.  */

static void
step_command_fsm_prepare (struct step_command_fsm *sm,
			  int skip_subroutines, int single_inst,
			  int count, struct thread_info *thread)
{
  sm->skip_subroutines = skip_subroutines;
  sm->single_inst = single_inst;
  sm->count = count;

  /* Leave the si command alone.  */
  if (!sm->single_inst || sm->skip_subroutines)
    set_longjmp_breakpoint (thread, get_frame_id (get_current_frame ()));

  thread->control.stepping_command = 1;
}

static int prepare_one_step (struct step_command_fsm *sm);

static void
step_1 (int skip_subroutines, int single_inst, char *count_string)
{
  int count;
  int async_exec;
  struct cleanup *args_chain;
  struct thread_info *thr;
  struct step_command_fsm *step_sm;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  count_string = strip_bg_char (count_string, &async_exec);
  args_chain = make_cleanup (xfree, count_string);

  prepare_execution_command (&current_target, async_exec);

  count = count_string ? parse_and_eval_long (count_string) : 1;

  /* Done with ARGS.  */
  do_cleanups (args_chain);

  clear_proceed_status (1);

  /* Setup the execution command state machine to handle all the COUNT
     steps.  */
  thr = inferior_thread ();
  step_sm = new_step_command_fsm (command_interp ());
  thr->thread_fsm = &step_sm->thread_fsm;

  step_command_fsm_prepare (step_sm, skip_subroutines,
			    single_inst, count, thr);

  /* Do only one step for now, before returning control to the event
     loop.  Let the continuation figure out how many other steps we
     need to do, and handle them one at the time, through
     step_once.  */
  if (!prepare_one_step (step_sm))
    proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
  else
    {
      int proceeded;

      /* Stepped into an inline frame.  Pretend that we've
	 stopped.  */
      thread_fsm_clean_up (thr->thread_fsm, thr);
      proceeded = normal_stop ();
      if (!proceeded)
	inferior_event_handler (INF_EXEC_COMPLETE, NULL);
      all_uis_check_sync_execution_done ();
    }
}

/* Implementation of the 'should_stop' FSM method for stepping
   commands.  Called after we are done with one step operation, to
   check whether we need to step again, before we print the prompt and
   return control to the user.  If count is > 1, returns false, as we
   will need to keep going.  */

static int
step_command_fsm_should_stop (struct thread_fsm *self, struct thread_info *tp)
{
  struct step_command_fsm *sm = (struct step_command_fsm *) self;

  if (tp->control.stop_step)
    {
      /* There are more steps to make, and we did stop due to
	 ending a stepping range.  Do another step.  */
      if (--sm->count > 0)
	return prepare_one_step (sm);

      thread_fsm_set_finished (self);
    }

  return 1;
}

/* Implementation of the 'clean_up' FSM method for stepping commands.  */

static void
step_command_fsm_clean_up (struct thread_fsm *self, struct thread_info *thread)
{
  struct step_command_fsm *sm = (struct step_command_fsm *) self;

  if (!sm->single_inst || sm->skip_subroutines)
    delete_longjmp_breakpoint (thread->global_num);
}

/* Implementation of the 'async_reply_reason' FSM method for stepping
   commands.  */

static enum async_reply_reason
step_command_fsm_async_reply_reason (struct thread_fsm *self)
{
  return EXEC_ASYNC_END_STEPPING_RANGE;
}

/* Prepare for one step in "step N".  The actual target resumption is
   done by the caller.  Return true if we're done and should thus
   report a stop to the user.  Returns false if the target needs to be
   resumed.  */

static int
prepare_one_step (struct step_command_fsm *sm)
{
  if (sm->count > 0)
    {
      struct frame_info *frame = get_current_frame ();

      /* Don't assume THREAD is a valid thread id.  It is set to -1 if
	 the longjmp breakpoint was not required.  Use the
	 INFERIOR_PTID thread instead, which is the same thread when
	 THREAD is set.  */
      struct thread_info *tp = inferior_thread ();

      set_step_frame ();

      if (!sm->single_inst)
	{
	  CORE_ADDR pc;

	  /* Step at an inlined function behaves like "down".  */
	  if (!sm->skip_subroutines
	      && inline_skipped_frames (inferior_ptid))
	    {
	      ptid_t resume_ptid;

	      /* Pretend that we've ran.  */
	      resume_ptid = user_visible_resume_ptid (1);
	      set_running (resume_ptid, 1);

	      step_into_inline_frame (inferior_ptid);
	      sm->count--;
	      return prepare_one_step (sm);
	    }

	  pc = get_frame_pc (frame);
	  find_pc_line_pc_range (pc,
				 &tp->control.step_range_start,
				 &tp->control.step_range_end);

	  tp->control.may_range_step = 1;

	  /* If we have no line info, switch to stepi mode.  */
	  if (tp->control.step_range_end == 0 && step_stop_if_no_debug)
	    {
	      tp->control.step_range_start = tp->control.step_range_end = 1;
	      tp->control.may_range_step = 0;
	    }
	  else if (tp->control.step_range_end == 0)
	    {
	      const char *name;

	      if (find_pc_partial_function (pc, &name,
					    &tp->control.step_range_start,
					    &tp->control.step_range_end) == 0)
		error (_("Cannot find bounds of current function"));

	      target_terminal_ours_for_output ();
	      printf_filtered (_("Single stepping until exit from function %s,"
				 "\nwhich has no line number information.\n"),
			       name);
	    }
	}
      else
	{
	  /* Say we are stepping, but stop after one insn whatever it does.  */
	  tp->control.step_range_start = tp->control.step_range_end = 1;
	  if (!sm->skip_subroutines)
	    /* It is stepi.
	       Don't step over function calls, not even to functions lacking
	       line numbers.  */
	    tp->control.step_over_calls = STEP_OVER_NONE;
	}

      if (sm->skip_subroutines)
	tp->control.step_over_calls = STEP_OVER_ALL;

      return 0;
    }

  /* Done.  */
  thread_fsm_set_finished (&sm->thread_fsm);
  return 1;
}


/* Continue program at specified address.  */

static void
jump_command (char *arg, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  CORE_ADDR addr;
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  struct symbol *fn;
  struct symbol *sfn;
  int async_exec;
  struct cleanup *args_chain;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  /* Find out whether we must run in the background.  */
  arg = strip_bg_char (arg, &async_exec);
  args_chain = make_cleanup (xfree, arg);

  prepare_execution_command (&current_target, async_exec);

  if (!arg)
    error_no_arg (_("starting address"));

  sals = decode_line_with_last_displayed (arg, DECODE_LINE_FUNFIRSTLINE);
  if (sals.nelts != 1)
    {
      error (_("Unreasonable jump request"));
    }

  sal = sals.sals[0];
  xfree (sals.sals);

  /* Done with ARGS.  */
  do_cleanups (args_chain);

  if (sal.symtab == 0 && sal.pc == 0)
    error (_("No source file has been specified."));

  resolve_sal_pc (&sal);	/* May error out.  */

  /* See if we are trying to jump to another function.  */
  fn = get_frame_function (get_current_frame ());
  sfn = find_pc_function (sal.pc);
  if (fn != NULL && sfn != fn)
    {
      if (!query (_("Line %d is not in `%s'.  Jump anyway? "), sal.line,
		  SYMBOL_PRINT_NAME (fn)))
	{
	  error (_("Not confirmed."));
	  /* NOTREACHED */
	}
    }

  if (sfn != NULL)
    {
      struct obj_section *section;

      fixup_symbol_section (sfn, 0);
      section = SYMBOL_OBJ_SECTION (symbol_objfile (sfn), sfn);
      if (section_is_overlay (section)
	  && !section_is_mapped (section))
	{
	  if (!query (_("WARNING!!!  Destination is in "
			"unmapped overlay!  Jump anyway? ")))
	    {
	      error (_("Not confirmed."));
	      /* NOTREACHED */
	    }
	}
    }

  addr = sal.pc;

  if (from_tty)
    {
      printf_filtered (_("Continuing at "));
      fputs_filtered (paddress (gdbarch, addr), gdb_stdout);
      printf_filtered (".\n");
    }

  clear_proceed_status (0);
  proceed (addr, GDB_SIGNAL_0);
}

/* Continue program giving it specified signal.  */

static void
signal_command (char *signum_exp, int from_tty)
{
  enum gdb_signal oursig;
  int async_exec;
  struct cleanup *args_chain;

  dont_repeat ();		/* Too dangerous.  */
  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  /* Find out whether we must run in the background.  */
  signum_exp = strip_bg_char (signum_exp, &async_exec);
  args_chain = make_cleanup (xfree, signum_exp);

  prepare_execution_command (&current_target, async_exec);

  if (!signum_exp)
    error_no_arg (_("signal number"));

  /* It would be even slicker to make signal names be valid expressions,
     (the type could be "enum $signal" or some such), then the user could
     assign them to convenience variables.  */
  oursig = gdb_signal_from_name (signum_exp);

  if (oursig == GDB_SIGNAL_UNKNOWN)
    {
      /* No, try numeric.  */
      int num = parse_and_eval_long (signum_exp);

      if (num == 0)
	oursig = GDB_SIGNAL_0;
      else
	oursig = gdb_signal_from_command (num);
    }

  do_cleanups (args_chain);

  /* Look for threads other than the current that this command ends up
     resuming too (due to schedlock off), and warn if they'll get a
     signal delivered.  "signal 0" is used to suppress a previous
     signal, but if the current thread is no longer the one that got
     the signal, then the user is potentially suppressing the signal
     of the wrong thread.  */
  if (!non_stop)
    {
      struct thread_info *tp;
      ptid_t resume_ptid;
      int must_confirm = 0;

      /* This indicates what will be resumed.  Either a single thread,
	 a whole process, or all threads of all processes.  */
      resume_ptid = user_visible_resume_ptid (0);

      ALL_NON_EXITED_THREADS (tp)
	{
	  if (ptid_equal (tp->ptid, inferior_ptid))
	    continue;
	  if (!ptid_match (tp->ptid, resume_ptid))
	    continue;

	  if (tp->suspend.stop_signal != GDB_SIGNAL_0
	      && signal_pass_state (tp->suspend.stop_signal))
	    {
	      if (!must_confirm)
		printf_unfiltered (_("Note:\n"));
	      printf_unfiltered (_("  Thread %s previously stopped with signal %s, %s.\n"),
				 print_thread_id (tp),
				 gdb_signal_to_name (tp->suspend.stop_signal),
				 gdb_signal_to_string (tp->suspend.stop_signal));
	      must_confirm = 1;
	    }
	}

      if (must_confirm
	  && !query (_("Continuing thread %s (the current thread) with specified signal will\n"
		       "still deliver the signals noted above to their respective threads.\n"
		       "Continue anyway? "),
		     print_thread_id (inferior_thread ())))
	error (_("Not confirmed."));
    }

  if (from_tty)
    {
      if (oursig == GDB_SIGNAL_0)
	printf_filtered (_("Continuing with no signal.\n"));
      else
	printf_filtered (_("Continuing with signal %s.\n"),
			 gdb_signal_to_name (oursig));
    }

  clear_proceed_status (0);
  proceed ((CORE_ADDR) -1, oursig);
}

/* Queue a signal to be delivered to the current thread.  */

static void
queue_signal_command (char *signum_exp, int from_tty)
{
  enum gdb_signal oursig;
  struct thread_info *tp;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  if (signum_exp == NULL)
    error_no_arg (_("signal number"));

  /* It would be even slicker to make signal names be valid expressions,
     (the type could be "enum $signal" or some such), then the user could
     assign them to convenience variables.  */
  oursig = gdb_signal_from_name (signum_exp);

  if (oursig == GDB_SIGNAL_UNKNOWN)
    {
      /* No, try numeric.  */
      int num = parse_and_eval_long (signum_exp);

      if (num == 0)
	oursig = GDB_SIGNAL_0;
      else
	oursig = gdb_signal_from_command (num);
    }

  if (oursig != GDB_SIGNAL_0
      && !signal_pass_state (oursig))
    error (_("Signal handling set to not pass this signal to the program."));

  tp = inferior_thread ();
  tp->suspend.stop_signal = oursig;
}

/* Data for the FSM that manages the until (with no argument)
   command.  */

struct until_next_fsm
{
  /* The base class.  */
  struct thread_fsm thread_fsm;

  /* The thread that as current when the command was executed.  */
  int thread;
};

static int until_next_fsm_should_stop (struct thread_fsm *self,
				       struct thread_info *thread);
static void until_next_fsm_clean_up (struct thread_fsm *self,
				     struct thread_info *thread);
static enum async_reply_reason
  until_next_fsm_async_reply_reason (struct thread_fsm *self);

/* until_next_fsm's vtable.  */

static struct thread_fsm_ops until_next_fsm_ops =
{
  NULL, /* dtor */
  until_next_fsm_clean_up,
  until_next_fsm_should_stop,
  NULL, /* return_value */
  until_next_fsm_async_reply_reason,
};

/* Allocate a new until_next_fsm.  */

static struct until_next_fsm *
new_until_next_fsm (struct interp *cmd_interp, int thread)
{
  struct until_next_fsm *sm;

  sm = XCNEW (struct until_next_fsm);
  thread_fsm_ctor (&sm->thread_fsm, &until_next_fsm_ops, cmd_interp);

  sm->thread = thread;

  return sm;
}

/* Implementation of the 'should_stop' FSM method for the until (with
   no arg) command.  */

static int
until_next_fsm_should_stop (struct thread_fsm *self,
			    struct thread_info *tp)
{
  if (tp->control.stop_step)
    thread_fsm_set_finished (self);

  return 1;
}

/* Implementation of the 'clean_up' FSM method for the until (with no
   arg) command.  */

static void
until_next_fsm_clean_up (struct thread_fsm *self, struct thread_info *thread)
{
  struct until_next_fsm *sm = (struct until_next_fsm *) self;

  delete_longjmp_breakpoint (thread->global_num);
}

/* Implementation of the 'async_reply_reason' FSM method for the until
   (with no arg) command.  */

static enum async_reply_reason
until_next_fsm_async_reply_reason (struct thread_fsm *self)
{
  return EXEC_ASYNC_END_STEPPING_RANGE;
}

/* Proceed until we reach a different source line with pc greater than
   our current one or exit the function.  We skip calls in both cases.

   Note that eventually this command should probably be changed so
   that only source lines are printed out when we hit the breakpoint
   we set.  This may involve changes to wait_for_inferior and the
   proceed status code.  */

static void
until_next_command (int from_tty)
{
  struct frame_info *frame;
  CORE_ADDR pc;
  struct symbol *func;
  struct symtab_and_line sal;
  struct thread_info *tp = inferior_thread ();
  int thread = tp->global_num;
  struct cleanup *old_chain;
  struct until_next_fsm *sm;

  clear_proceed_status (0);
  set_step_frame ();

  frame = get_current_frame ();

  /* Step until either exited from this function or greater
     than the current line (if in symbolic section) or pc (if
     not).  */

  pc = get_frame_pc (frame);
  func = find_pc_function (pc);

  if (!func)
    {
      struct bound_minimal_symbol msymbol = lookup_minimal_symbol_by_pc (pc);

      if (msymbol.minsym == NULL)
	error (_("Execution is not within a known function."));

      tp->control.step_range_start = BMSYMBOL_VALUE_ADDRESS (msymbol);
      /* The upper-bound of step_range is exclusive.  In order to make PC
	 within the range, set the step_range_end with PC + 1.  */
      tp->control.step_range_end = pc + 1;
    }
  else
    {
      sal = find_pc_line (pc, 0);

      tp->control.step_range_start = BLOCK_START (SYMBOL_BLOCK_VALUE (func));
      tp->control.step_range_end = sal.end;
    }
  tp->control.may_range_step = 1;

  tp->control.step_over_calls = STEP_OVER_ALL;

  set_longjmp_breakpoint (tp, get_frame_id (frame));
  old_chain = make_cleanup (delete_longjmp_breakpoint_cleanup, &thread);

  sm = new_until_next_fsm (command_interp (), tp->global_num);
  tp->thread_fsm = &sm->thread_fsm;
  discard_cleanups (old_chain);

  proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
}

static void
until_command (char *arg, int from_tty)
{
  int async_exec;
  struct cleanup *args_chain;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  /* Find out whether we must run in the background.  */
  arg = strip_bg_char (arg, &async_exec);
  args_chain = make_cleanup (xfree, arg);

  prepare_execution_command (&current_target, async_exec);

  if (arg)
    until_break_command (arg, from_tty, 0);
  else
    until_next_command (from_tty);

  /* Done with ARGS.  */
  do_cleanups (args_chain);
}

static void
advance_command (char *arg, int from_tty)
{
  int async_exec;
  struct cleanup *args_chain;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  if (arg == NULL)
    error_no_arg (_("a location"));

  /* Find out whether we must run in the background.  */
  arg = strip_bg_char (arg, &async_exec);
  args_chain = make_cleanup (xfree, arg);

  prepare_execution_command (&current_target, async_exec);

  until_break_command (arg, from_tty, 1);

  /* Done with ARGS.  */
  do_cleanups (args_chain);
}

/* Return the value of the result of a function at the end of a 'finish'
   command/BP.  DTOR_DATA (if not NULL) can represent inferior registers
   right after an inferior call has finished.  */

struct value *
get_return_value (struct value *function, struct type *value_type)
{
  struct regcache *stop_regs;
  struct gdbarch *gdbarch;
  struct value *value;
  struct cleanup *cleanup;

  stop_regs = regcache_dup (get_current_regcache ());
  cleanup = make_cleanup_regcache_xfree (stop_regs);

  gdbarch = get_regcache_arch (stop_regs);

  value_type = check_typedef (value_type);
  gdb_assert (TYPE_CODE (value_type) != TYPE_CODE_VOID);

  /* FIXME: 2003-09-27: When returning from a nested inferior function
     call, it's possible (with no help from the architecture vector)
     to locate and return/print a "struct return" value.  This is just
     a more complicated case of what is already being done in the
     inferior function call code.  In fact, when inferior function
     calls are made async, this will likely be made the norm.  */

  switch (gdbarch_return_value (gdbarch, function, value_type,
  				NULL, NULL, NULL))
    {
    case RETURN_VALUE_REGISTER_CONVENTION:
    case RETURN_VALUE_ABI_RETURNS_ADDRESS:
    case RETURN_VALUE_ABI_PRESERVES_ADDRESS:
      value = allocate_value (value_type);
      gdbarch_return_value (gdbarch, function, value_type, stop_regs,
			    value_contents_raw (value), NULL);
      break;
    case RETURN_VALUE_STRUCT_CONVENTION:
      value = NULL;
      break;
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }

  do_cleanups (cleanup);

  return value;
}

/* The captured function return value/type and its position in the
   value history.  */

struct return_value_info
{
  /* The captured return value.  May be NULL if we weren't able to
     retrieve it.  See get_return_value.  */
  struct value *value;

  /* The return type.  In some cases, we'll not be able extract the
     return value, but we always know the type.  */
  struct type *type;

  /* If we captured a value, this is the value history index.  */
  int value_history_index;
};

/* Helper for print_return_value.  */

static void
print_return_value_1 (struct ui_out *uiout, struct return_value_info *rv)
{
  if (rv->value != NULL)
    {
      struct value_print_options opts;

      /* Print it.  */
      uiout->text ("Value returned is ");
      uiout->field_fmt ("gdb-result-var", "$%d",
			 rv->value_history_index);
      uiout->text (" = ");
      get_no_prettyformat_print_options (&opts);

      string_file stb;

      value_print (rv->value, &stb, &opts);
      uiout->field_stream ("return-value", stb);
      uiout->text ("\n");
    }
  else
    {
      std::string type_name = type_to_string (rv->type);
      uiout->text ("Value returned has type: ");
      uiout->field_string ("return-type", type_name.c_str ());
      uiout->text (".");
      uiout->text (" Cannot determine contents\n");
    }
}

/* Print the result of a function at the end of a 'finish' command.
   RV points at an object representing the captured return value/type
   and its position in the value history.  */

void
print_return_value (struct ui_out *uiout, struct return_value_info *rv)
{
  if (rv->type == NULL || TYPE_CODE (rv->type) == TYPE_CODE_VOID)
    return;

  TRY
    {
      /* print_return_value_1 can throw an exception in some
	 circumstances.  We need to catch this so that we still
	 delete the breakpoint.  */
      print_return_value_1 (uiout, rv);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      exception_print (gdb_stdout, ex);
    }
  END_CATCH
}

/* Data for the FSM that manages the finish command.  */

struct finish_command_fsm
{
  /* The base class.  */
  struct thread_fsm thread_fsm;

  /* The momentary breakpoint set at the function's return address in
     the caller.  */
  struct breakpoint *breakpoint;

  /* The function that we're stepping out of.  */
  struct symbol *function;

  /* If the FSM finishes successfully, this stores the function's
     return value.  */
  struct return_value_info return_value;
};

static int finish_command_fsm_should_stop (struct thread_fsm *self,
					   struct thread_info *thread);
static void finish_command_fsm_clean_up (struct thread_fsm *self,
					 struct thread_info *thread);
static struct return_value_info *
  finish_command_fsm_return_value (struct thread_fsm *self);
static enum async_reply_reason
  finish_command_fsm_async_reply_reason (struct thread_fsm *self);

/* finish_command_fsm's vtable.  */

static struct thread_fsm_ops finish_command_fsm_ops =
{
  NULL, /* dtor */
  finish_command_fsm_clean_up,
  finish_command_fsm_should_stop,
  finish_command_fsm_return_value,
  finish_command_fsm_async_reply_reason,
  NULL, /* should_notify_stop */
};

/* Allocate a new finish_command_fsm.  */

static struct finish_command_fsm *
new_finish_command_fsm (struct interp *cmd_interp)
{
  struct finish_command_fsm *sm;

  sm = XCNEW (struct finish_command_fsm);
  thread_fsm_ctor (&sm->thread_fsm, &finish_command_fsm_ops, cmd_interp);

  return sm;
}

/* Implementation of the 'should_stop' FSM method for the finish
   commands.  Detects whether the thread stepped out of the function
   successfully, and if so, captures the function's return value and
   marks the FSM finished.  */

static int
finish_command_fsm_should_stop (struct thread_fsm *self,
				struct thread_info *tp)
{
  struct finish_command_fsm *f = (struct finish_command_fsm *) self;
  struct return_value_info *rv = &f->return_value;

  if (f->function != NULL
      && bpstat_find_breakpoint (tp->control.stop_bpstat,
				 f->breakpoint) != NULL)
    {
      /* We're done.  */
      thread_fsm_set_finished (self);

      rv->type = TYPE_TARGET_TYPE (SYMBOL_TYPE (f->function));
      if (rv->type == NULL)
	internal_error (__FILE__, __LINE__,
			_("finish_command: function has no target type"));

      if (TYPE_CODE (rv->type) != TYPE_CODE_VOID)
	{
	  struct value *func;

	  func = read_var_value (f->function, NULL, get_current_frame ());
	  rv->value = get_return_value (func, rv->type);
	  if (rv->value != NULL)
	    rv->value_history_index = record_latest_value (rv->value);
	}
    }
  else if (tp->control.stop_step)
    {
      /* Finishing from an inline frame, or reverse finishing.  In
	 either case, there's no way to retrieve the return value.  */
      thread_fsm_set_finished (self);
    }

  return 1;
}

/* Implementation of the 'clean_up' FSM method for the finish
   commands.  */

static void
finish_command_fsm_clean_up (struct thread_fsm *self,
			     struct thread_info *thread)
{
  struct finish_command_fsm *f = (struct finish_command_fsm *) self;

  if (f->breakpoint != NULL)
    {
      delete_breakpoint (f->breakpoint);
      f->breakpoint = NULL;
    }
  delete_longjmp_breakpoint (thread->global_num);
}

/* Implementation of the 'return_value' FSM method for the finish
   commands.  */

static struct return_value_info *
finish_command_fsm_return_value (struct thread_fsm *self)
{
  struct finish_command_fsm *f = (struct finish_command_fsm *) self;

  return &f->return_value;
}

/* Implementation of the 'async_reply_reason' FSM method for the
   finish commands.  */

static enum async_reply_reason
finish_command_fsm_async_reply_reason (struct thread_fsm *self)
{
  if (execution_direction == EXEC_REVERSE)
    return EXEC_ASYNC_END_STEPPING_RANGE;
  else
    return EXEC_ASYNC_FUNCTION_FINISHED;
}

/* finish_backward -- helper function for finish_command.  */

static void
finish_backward (struct finish_command_fsm *sm)
{
  struct symtab_and_line sal;
  struct thread_info *tp = inferior_thread ();
  CORE_ADDR pc;
  CORE_ADDR func_addr;

  pc = get_frame_pc (get_current_frame ());

  if (find_pc_partial_function (pc, NULL, &func_addr, NULL) == 0)
    error (_("Cannot find bounds of current function"));

  sal = find_pc_line (func_addr, 0);

  tp->control.proceed_to_finish = 1;
  /* Special case: if we're sitting at the function entry point,
     then all we need to do is take a reverse singlestep.  We
     don't need to set a breakpoint, and indeed it would do us
     no good to do so.

     Note that this can only happen at frame #0, since there's
     no way that a function up the stack can have a return address
     that's equal to its entry point.  */

  if (sal.pc != pc)
    {
      struct frame_info *frame = get_selected_frame (NULL);
      struct gdbarch *gdbarch = get_frame_arch (frame);
      struct symtab_and_line sr_sal;

      /* Set a step-resume at the function's entry point.  Once that's
	 hit, we'll do one more step backwards.  */
      init_sal (&sr_sal);
      sr_sal.pc = sal.pc;
      sr_sal.pspace = get_frame_program_space (frame);
      insert_step_resume_breakpoint_at_sal (gdbarch,
					    sr_sal, null_frame_id);

      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }
  else
    {
      /* We're almost there -- we just need to back up by one more
	 single-step.  */
      tp->control.step_range_start = tp->control.step_range_end = 1;
      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }
}

/* finish_forward -- helper function for finish_command.  FRAME is the
   frame that called the function we're about to step out of.  */

static void
finish_forward (struct finish_command_fsm *sm, struct frame_info *frame)
{
  struct frame_id frame_id = get_frame_id (frame);
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct symtab_and_line sal;
  struct thread_info *tp = inferior_thread ();

  sal = find_pc_line (get_frame_pc (frame), 0);
  sal.pc = get_frame_pc (frame);

  sm->breakpoint = set_momentary_breakpoint (gdbarch, sal,
					     get_stack_frame_id (frame),
					     bp_finish);

  /* set_momentary_breakpoint invalidates FRAME.  */
  frame = NULL;

  set_longjmp_breakpoint (tp, frame_id);

  /* We want to print return value, please...  */
  tp->control.proceed_to_finish = 1;

  proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
}

/* Skip frames for "finish".  */

static struct frame_info *
skip_finish_frames (struct frame_info *frame)
{
  struct frame_info *start;

  do
    {
      start = frame;

      frame = skip_tailcall_frames (frame);
      if (frame == NULL)
	break;

      frame = skip_unwritable_frames (frame);
      if (frame == NULL)
	break;
    }
  while (start != frame);

  return frame;
}

/* "finish": Set a temporary breakpoint at the place the selected
   frame will return to, then continue.  */

static void
finish_command (char *arg, int from_tty)
{
  struct frame_info *frame;
  int async_exec;
  struct cleanup *args_chain;
  struct finish_command_fsm *sm;
  struct thread_info *tp;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  /* Find out whether we must run in the background.  */
  arg = strip_bg_char (arg, &async_exec);
  args_chain = make_cleanup (xfree, arg);

  prepare_execution_command (&current_target, async_exec);

  if (arg)
    error (_("The \"finish\" command does not take any arguments."));

  /* Done with ARGS.  */
  do_cleanups (args_chain);

  frame = get_prev_frame (get_selected_frame (_("No selected frame.")));
  if (frame == 0)
    error (_("\"finish\" not meaningful in the outermost frame."));

  clear_proceed_status (0);

  tp = inferior_thread ();

  sm = new_finish_command_fsm (command_interp ());

  tp->thread_fsm = &sm->thread_fsm;

  /* Finishing from an inline frame is completely different.  We don't
     try to show the "return value" - no way to locate it.  */
  if (get_frame_type (get_selected_frame (_("No selected frame.")))
      == INLINE_FRAME)
    {
      /* Claim we are stepping in the calling frame.  An empty step
	 range means that we will stop once we aren't in a function
	 called by that frame.  We don't use the magic "1" value for
	 step_range_end, because then infrun will think this is nexti,
	 and not step over the rest of this inlined function call.  */
      struct symtab_and_line empty_sal;

      init_sal (&empty_sal);
      set_step_info (frame, empty_sal);
      tp->control.step_range_start = get_frame_pc (frame);
      tp->control.step_range_end = tp->control.step_range_start;
      tp->control.step_over_calls = STEP_OVER_ALL;

      /* Print info on the selected frame, including level number but not
	 source.  */
      if (from_tty)
	{
	  printf_filtered (_("Run till exit from "));
	  print_stack_frame (get_selected_frame (NULL), 1, LOCATION, 0);
	}

      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
      return;
    }

  /* Find the function we will return from.  */

  sm->function = find_pc_function (get_frame_pc (get_selected_frame (NULL)));

  /* Print info on the selected frame, including level number but not
     source.  */
  if (from_tty)
    {
      if (execution_direction == EXEC_REVERSE)
	printf_filtered (_("Run back to call of "));
      else
	{
	  if (sm->function != NULL && TYPE_NO_RETURN (sm->function->type)
	      && !query (_("warning: Function %s does not return normally.\n"
			   "Try to finish anyway? "),
			 SYMBOL_PRINT_NAME (sm->function)))
	    error (_("Not confirmed."));
	  printf_filtered (_("Run till exit from "));
	}

      print_stack_frame (get_selected_frame (NULL), 1, LOCATION, 0);
    }

  if (execution_direction == EXEC_REVERSE)
    finish_backward (sm);
  else
    {
      frame = skip_finish_frames (frame);

      if (frame == NULL)
	error (_("Cannot find the caller frame."));

      finish_forward (sm, frame);
    }
}


static void
program_info (char *args, int from_tty)
{
  bpstat bs;
  int num, stat;
  struct thread_info *tp;
  ptid_t ptid;

  if (!target_has_execution)
    {
      printf_filtered (_("The program being debugged is not being run.\n"));
      return;
    }

  if (non_stop)
    ptid = inferior_ptid;
  else
    {
      struct target_waitstatus ws;

      get_last_target_status (&ptid, &ws);
    }

  if (ptid_equal (ptid, null_ptid) || is_exited (ptid))
    error (_("Invalid selected thread."));
  else if (is_running (ptid))
    error (_("Selected thread is running."));

  tp = find_thread_ptid (ptid);
  bs = tp->control.stop_bpstat;
  stat = bpstat_num (&bs, &num);

  target_files_info ();
  printf_filtered (_("Program stopped at %s.\n"),
		   paddress (target_gdbarch (), stop_pc));
  if (tp->control.stop_step)
    printf_filtered (_("It stopped after being stepped.\n"));
  else if (stat != 0)
    {
      /* There may be several breakpoints in the same place, so this
         isn't as strange as it seems.  */
      while (stat != 0)
	{
	  if (stat < 0)
	    {
	      printf_filtered (_("It stopped at a breakpoint "
				 "that has since been deleted.\n"));
	    }
	  else
	    printf_filtered (_("It stopped at breakpoint %d.\n"), num);
	  stat = bpstat_num (&bs, &num);
	}
    }
  else if (tp->suspend.stop_signal != GDB_SIGNAL_0)
    {
      printf_filtered (_("It stopped with signal %s, %s.\n"),
		       gdb_signal_to_name (tp->suspend.stop_signal),
		       gdb_signal_to_string (tp->suspend.stop_signal));
    }

  if (from_tty)
    {
      printf_filtered (_("Type \"info stack\" or \"info "
			 "registers\" for more information.\n"));
    }
}

static void
environment_info (char *var, int from_tty)
{
  if (var)
    {
      char *val = get_in_environ (current_inferior ()->environment, var);

      if (val)
	{
	  puts_filtered (var);
	  puts_filtered (" = ");
	  puts_filtered (val);
	  puts_filtered ("\n");
	}
      else
	{
	  puts_filtered ("Environment variable \"");
	  puts_filtered (var);
	  puts_filtered ("\" not defined.\n");
	}
    }
  else
    {
      char **vector = environ_vector (current_inferior ()->environment);

      while (*vector)
	{
	  puts_filtered (*vector++);
	  puts_filtered ("\n");
	}
    }
}

static void
set_environment_command (char *arg, int from_tty)
{
  char *p, *val, *var;
  int nullset = 0;

  if (arg == 0)
    error_no_arg (_("environment variable and value"));

  /* Find seperation between variable name and value.  */
  p = (char *) strchr (arg, '=');
  val = (char *) strchr (arg, ' ');

  if (p != 0 && val != 0)
    {
      /* We have both a space and an equals.  If the space is before the
         equals, walk forward over the spaces til we see a nonspace 
         (possibly the equals).  */
      if (p > val)
	while (*val == ' ')
	  val++;

      /* Now if the = is after the char following the spaces,
         take the char following the spaces.  */
      if (p > val)
	p = val - 1;
    }
  else if (val != 0 && p == 0)
    p = val;

  if (p == arg)
    error_no_arg (_("environment variable to set"));

  if (p == 0 || p[1] == 0)
    {
      nullset = 1;
      if (p == 0)
	p = arg + strlen (arg);	/* So that savestring below will work.  */
    }
  else
    {
      /* Not setting variable value to null.  */
      val = p + 1;
      while (*val == ' ' || *val == '\t')
	val++;
    }

  while (p != arg && (p[-1] == ' ' || p[-1] == '\t'))
    p--;

  var = savestring (arg, p - arg);
  if (nullset)
    {
      printf_filtered (_("Setting environment variable "
			 "\"%s\" to null value.\n"),
		       var);
      set_in_environ (current_inferior ()->environment, var, "");
    }
  else
    set_in_environ (current_inferior ()->environment, var, val);
  xfree (var);
}

static void
unset_environment_command (char *var, int from_tty)
{
  if (var == 0)
    {
      /* If there is no argument, delete all environment variables.
         Ask for confirmation if reading from the terminal.  */
      if (!from_tty || query (_("Delete all environment variables? ")))
	{
	  free_environ (current_inferior ()->environment);
	  current_inferior ()->environment = make_environ ();
	}
    }
  else
    unset_in_environ (current_inferior ()->environment, var);
}

/* Handle the execution path (PATH variable).  */

static const char path_var_name[] = "PATH";

static void
path_info (char *args, int from_tty)
{
  puts_filtered ("Executable and object file path: ");
  puts_filtered (get_in_environ (current_inferior ()->environment,
				 path_var_name));
  puts_filtered ("\n");
}

/* Add zero or more directories to the front of the execution path.  */

static void
path_command (char *dirname, int from_tty)
{
  char *exec_path;
  const char *env;

  dont_repeat ();
  env = get_in_environ (current_inferior ()->environment, path_var_name);
  /* Can be null if path is not set.  */
  if (!env)
    env = "";
  exec_path = xstrdup (env);
  mod_path (dirname, &exec_path);
  set_in_environ (current_inferior ()->environment, path_var_name, exec_path);
  xfree (exec_path);
  if (from_tty)
    path_info ((char *) NULL, from_tty);
}


/* Print out the register NAME with value VAL, to FILE, in the default
   fashion.  */

static void
default_print_one_register_info (struct ui_file *file,
				 const char *name,
				 struct value *val)
{
  struct type *regtype = value_type (val);
  int print_raw_format;

  fputs_filtered (name, file);
  print_spaces_filtered (15 - strlen (name), file);

  print_raw_format = (value_entirely_available (val)
		      && !value_optimized_out (val));

  /* If virtual format is floating, print it that way, and in raw
     hex.  */
  if (TYPE_CODE (regtype) == TYPE_CODE_FLT
      || TYPE_CODE (regtype) == TYPE_CODE_DECFLOAT)
    {
      struct value_print_options opts;
      const gdb_byte *valaddr = value_contents_for_printing (val);
      enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (regtype));

      get_user_print_options (&opts);
      opts.deref_ref = 1;

      val_print (regtype,
		 value_embedded_offset (val), 0,
		 file, 0, val, &opts, current_language);

      if (print_raw_format)
	{
	  fprintf_filtered (file, "\t(raw ");
	  print_hex_chars (file, valaddr, TYPE_LENGTH (regtype), byte_order);
	  fprintf_filtered (file, ")");
	}
    }
  else
    {
      struct value_print_options opts;

      /* Print the register in hex.  */
      get_formatted_print_options (&opts, 'x');
      opts.deref_ref = 1;
      val_print (regtype,
		 value_embedded_offset (val), 0,
		 file, 0, val, &opts, current_language);
      /* If not a vector register, print it also according to its
	 natural format.  */
      if (print_raw_format && TYPE_VECTOR (regtype) == 0)
	{
	  get_user_print_options (&opts);
	  opts.deref_ref = 1;
	  fprintf_filtered (file, "\t");
	  val_print (regtype,
		     value_embedded_offset (val), 0,
		     file, 0, val, &opts, current_language);
	}
    }

  fprintf_filtered (file, "\n");
}

/* Print out the machine register regnum.  If regnum is -1, print all
   registers (print_all == 1) or all non-float and non-vector
   registers (print_all == 0).

   For most machines, having all_registers_info() print the
   register(s) one per line is good enough.  If a different format is
   required, (eg, for MIPS or Pyramid 90x, which both have lots of
   regs), or there is an existing convention for showing all the
   registers, define the architecture method PRINT_REGISTERS_INFO to
   provide that format.  */

void
default_print_registers_info (struct gdbarch *gdbarch,
			      struct ui_file *file,
			      struct frame_info *frame,
			      int regnum, int print_all)
{
  int i;
  const int numregs = gdbarch_num_regs (gdbarch)
		      + gdbarch_num_pseudo_regs (gdbarch);

  for (i = 0; i < numregs; i++)
    {
      /* Decide between printing all regs, non-float / vector regs, or
         specific reg.  */
      if (regnum == -1)
	{
	  if (print_all)
	    {
	      if (!gdbarch_register_reggroup_p (gdbarch, i, all_reggroup))
		continue;
	    }
	  else
	    {
	      if (!gdbarch_register_reggroup_p (gdbarch, i, general_reggroup))
		continue;
	    }
	}
      else
	{
	  if (i != regnum)
	    continue;
	}

      /* If the register name is empty, it is undefined for this
         processor, so don't display anything.  */
      if (gdbarch_register_name (gdbarch, i) == NULL
	  || *(gdbarch_register_name (gdbarch, i)) == '\0')
	continue;

      default_print_one_register_info (file,
				       gdbarch_register_name (gdbarch, i),
				       value_of_register (i, frame));
    }
}

void
registers_info (char *addr_exp, int fpregs)
{
  struct frame_info *frame;
  struct gdbarch *gdbarch;

  if (!target_has_registers)
    error (_("The program has no registers now."));
  frame = get_selected_frame (NULL);
  gdbarch = get_frame_arch (frame);

  if (!addr_exp)
    {
      gdbarch_print_registers_info (gdbarch, gdb_stdout,
				    frame, -1, fpregs);
      return;
    }

  while (*addr_exp != '\0')
    {
      char *start;
      const char *end;

      /* Skip leading white space.  */
      addr_exp = skip_spaces (addr_exp);

      /* Discard any leading ``$''.  Check that there is something
         resembling a register following it.  */
      if (addr_exp[0] == '$')
	addr_exp++;
      if (isspace ((*addr_exp)) || (*addr_exp) == '\0')
	error (_("Missing register name"));

      /* Find the start/end of this register name/num/group.  */
      start = addr_exp;
      while ((*addr_exp) != '\0' && !isspace ((*addr_exp)))
	addr_exp++;
      end = addr_exp;

      /* Figure out what we've found and display it.  */

      /* A register name?  */
      {
	int regnum = user_reg_map_name_to_regnum (gdbarch, start, end - start);

	if (regnum >= 0)
	  {
	    /* User registers lie completely outside of the range of
	       normal registers.  Catch them early so that the target
	       never sees them.  */
	    if (regnum >= gdbarch_num_regs (gdbarch)
			  + gdbarch_num_pseudo_regs (gdbarch))
	      {
		struct value *regval = value_of_user_reg (regnum, frame);
		const char *regname = user_reg_map_regnum_to_name (gdbarch,
								   regnum);

		/* Print in the same fashion
		   gdbarch_print_registers_info's default
		   implementation prints.  */
		default_print_one_register_info (gdb_stdout,
						 regname,
						 regval);
	      }
	    else
	      gdbarch_print_registers_info (gdbarch, gdb_stdout,
					    frame, regnum, fpregs);
	    continue;
	  }
      }

      /* A register group?  */
      {
	struct reggroup *group;

	for (group = reggroup_next (gdbarch, NULL);
	     group != NULL;
	     group = reggroup_next (gdbarch, group))
	  {
	    /* Don't bother with a length check.  Should the user
	       enter a short register group name, go with the first
	       group that matches.  */
	    if (strncmp (start, reggroup_name (group), end - start) == 0)
	      break;
	  }
	if (group != NULL)
	  {
	    int regnum;

	    for (regnum = 0;
		 regnum < gdbarch_num_regs (gdbarch)
			  + gdbarch_num_pseudo_regs (gdbarch);
		 regnum++)
	      {
		if (gdbarch_register_reggroup_p (gdbarch, regnum, group))
		  gdbarch_print_registers_info (gdbarch,
						gdb_stdout, frame,
						regnum, fpregs);
	      }
	    continue;
	  }
      }

      /* Nothing matched.  */
      error (_("Invalid register `%.*s'"), (int) (end - start), start);
    }
}

static void
all_registers_info (char *addr_exp, int from_tty)
{
  registers_info (addr_exp, 1);
}

static void
nofp_registers_info (char *addr_exp, int from_tty)
{
  registers_info (addr_exp, 0);
}

static void
print_vector_info (struct ui_file *file,
		   struct frame_info *frame, const char *args)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  if (gdbarch_print_vector_info_p (gdbarch))
    gdbarch_print_vector_info (gdbarch, file, frame, args);
  else
    {
      int regnum;
      int printed_something = 0;

      for (regnum = 0;
	   regnum < gdbarch_num_regs (gdbarch)
		    + gdbarch_num_pseudo_regs (gdbarch);
	   regnum++)
	{
	  if (gdbarch_register_reggroup_p (gdbarch, regnum, vector_reggroup))
	    {
	      printed_something = 1;
	      gdbarch_print_registers_info (gdbarch, file, frame, regnum, 1);
	    }
	}
      if (!printed_something)
	fprintf_filtered (file, "No vector information\n");
    }
}

static void
vector_info (char *args, int from_tty)
{
  if (!target_has_registers)
    error (_("The program has no registers now."));

  print_vector_info (gdb_stdout, get_selected_frame (NULL), args);
}

/* Kill the inferior process.  Make us have no inferior.  */

static void
kill_command (char *arg, int from_tty)
{
  /* FIXME:  This should not really be inferior_ptid (or target_has_execution).
     It should be a distinct flag that indicates that a target is active, cuz
     some targets don't have processes!  */

  if (ptid_equal (inferior_ptid, null_ptid))
    error (_("The program is not being run."));
  if (!query (_("Kill the program being debugged? ")))
    error (_("Not confirmed."));
  target_kill ();

  /* If we still have other inferiors to debug, then don't mess with
     with their threads.  */
  if (!have_inferiors ())
    {
      init_thread_list ();		/* Destroy thread info.  */

      /* Killing off the inferior can leave us with a core file.  If
	 so, print the state we are left in.  */
      if (target_has_stack)
	{
	  printf_filtered (_("In %s,\n"), target_longname);
	  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
	}
    }
  bfd_cache_close_all ();
}

/* Used in `attach&' command.  ARG is a point to an integer
   representing a process id.  Proceed threads of this process iff
   they stopped due to debugger request, and when they did, they
   reported a clean stop (GDB_SIGNAL_0).  Do not proceed threads
   that have been explicitly been told to stop.  */

static int
proceed_after_attach_callback (struct thread_info *thread,
			       void *arg)
{
  int pid = * (int *) arg;

  if (ptid_get_pid (thread->ptid) == pid
      && !is_exited (thread->ptid)
      && !is_executing (thread->ptid)
      && !thread->stop_requested
      && thread->suspend.stop_signal == GDB_SIGNAL_0)
    {
      switch_to_thread (thread->ptid);
      clear_proceed_status (0);
      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }

  return 0;
}

static void
proceed_after_attach (int pid)
{
  /* Don't error out if the current thread is running, because
     there may be other stopped threads.  */
  struct cleanup *old_chain;

  /* Backup current thread and selected frame.  */
  old_chain = make_cleanup_restore_current_thread ();

  iterate_over_threads (proceed_after_attach_callback, &pid);

  /* Restore selected ptid.  */
  do_cleanups (old_chain);
}

/* See inferior.h.  */

void
setup_inferior (int from_tty)
{
  struct inferior *inferior;

  inferior = current_inferior ();
  inferior->needs_setup = 0;

  /* If no exec file is yet known, try to determine it from the
     process itself.  */
  if (get_exec_file (0) == NULL)
    exec_file_locate_attach (ptid_get_pid (inferior_ptid), 1, from_tty);
  else
    {
      reopen_exec_file ();
      reread_symbols ();
    }

  /* Take any necessary post-attaching actions for this platform.  */
  target_post_attach (ptid_get_pid (inferior_ptid));

  post_create_inferior (&current_target, from_tty);
}

/* What to do after the first program stops after attaching.  */
enum attach_post_wait_mode
{
  /* Do nothing.  Leaves threads as they are.  */
  ATTACH_POST_WAIT_NOTHING,

  /* Re-resume threads that are marked running.  */
  ATTACH_POST_WAIT_RESUME,

  /* Stop all threads.  */
  ATTACH_POST_WAIT_STOP,
};

/* Called after we've attached to a process and we've seen it stop for
   the first time.  If ASYNC_EXEC is true, re-resume threads that
   should be running.  Else if ATTACH, */

static void
attach_post_wait (const char *args, int from_tty, enum attach_post_wait_mode mode)
{
  struct inferior *inferior;

  inferior = current_inferior ();
  inferior->control.stop_soon = NO_STOP_QUIETLY;

  if (inferior->needs_setup)
    setup_inferior (from_tty);

  if (mode == ATTACH_POST_WAIT_RESUME)
    {
      /* The user requested an `attach&', so be sure to leave threads
	 that didn't get a signal running.  */

      /* Immediatelly resume all suspended threads of this inferior,
	 and this inferior only.  This should have no effect on
	 already running threads.  If a thread has been stopped with a
	 signal, leave it be.  */
      if (non_stop)
	proceed_after_attach (inferior->pid);
      else
	{
	  if (inferior_thread ()->suspend.stop_signal == GDB_SIGNAL_0)
	    {
	      clear_proceed_status (0);
	      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
	    }
	}
    }
  else if (mode == ATTACH_POST_WAIT_STOP)
    {
      /* The user requested a plain `attach', so be sure to leave
	 the inferior stopped.  */

      /* At least the current thread is already stopped.  */

      /* In all-stop, by definition, all threads have to be already
	 stopped at this point.  In non-stop, however, although the
	 selected thread is stopped, others may still be executing.
	 Be sure to explicitly stop all threads of the process.  This
	 should have no effect on already stopped threads.  */
      if (non_stop)
	target_stop (pid_to_ptid (inferior->pid));
      else if (target_is_non_stop_p ())
	{
	  struct thread_info *thread;
	  struct thread_info *lowest = inferior_thread ();
	  int pid = current_inferior ()->pid;

	  stop_all_threads ();

	  /* It's not defined which thread will report the attach
	     stop.  For consistency, always select the thread with
	     lowest GDB number, which should be the main thread, if it
	     still exists.  */
	  ALL_NON_EXITED_THREADS (thread)
	    {
	      if (ptid_get_pid (thread->ptid) == pid)
		{
		  if (thread->inf->num < lowest->inf->num
		      || thread->per_inf_num < lowest->per_inf_num)
		    lowest = thread;
		}
	    }

	  switch_to_thread (lowest->ptid);
	}

      /* Tell the user/frontend where we're stopped.  */
      normal_stop ();
      if (deprecated_attach_hook)
	deprecated_attach_hook ();
    }
}

struct attach_command_continuation_args
{
  char *args;
  int from_tty;
  enum attach_post_wait_mode mode;
};

static void
attach_command_continuation (void *args, int err)
{
  struct attach_command_continuation_args *a
    = (struct attach_command_continuation_args *) args;

  if (err)
    return;

  attach_post_wait (a->args, a->from_tty, a->mode);
}

static void
attach_command_continuation_free_args (void *args)
{
  struct attach_command_continuation_args *a
    = (struct attach_command_continuation_args *) args;

  xfree (a->args);
  xfree (a);
}

/* "attach" command entry point.  Takes a program started up outside
   of gdb and ``attaches'' to it.  This stops it cold in its tracks
   and allows us to start debugging it.  */

void
attach_command (char *args, int from_tty)
{
  int async_exec;
  struct cleanup *args_chain;
  struct target_ops *attach_target;
  struct inferior *inferior = current_inferior ();
  enum attach_post_wait_mode mode;

  dont_repeat ();		/* Not for the faint of heart */

  if (gdbarch_has_global_solist (target_gdbarch ()))
    /* Don't complain if all processes share the same symbol
       space.  */
    ;
  else if (target_has_execution)
    {
      if (query (_("A program is being debugged already.  Kill it? ")))
	target_kill ();
      else
	error (_("Not killed."));
    }

  /* Clean up any leftovers from other runs.  Some other things from
     this function should probably be moved into target_pre_inferior.  */
  target_pre_inferior (from_tty);

  args = strip_bg_char (args, &async_exec);
  args_chain = make_cleanup (xfree, args);

  attach_target = find_attach_target ();

  prepare_execution_command (attach_target, async_exec);

  if (non_stop && !attach_target->to_supports_non_stop (attach_target))
    error (_("Cannot attach to this target in non-stop mode"));

  attach_target->to_attach (attach_target, args, from_tty);
  /* to_attach should push the target, so after this point we
     shouldn't refer to attach_target again.  */
  attach_target = NULL;

  /* Set up the "saved terminal modes" of the inferior
     based on what modes we are starting it with.  */
  target_terminal_init ();

  /* Install inferior's terminal modes.  This may look like a no-op,
     as we've just saved them above, however, this does more than
     restore terminal settings:

     - installs a SIGINT handler that forwards SIGINT to the inferior.
       Otherwise a Ctrl-C pressed just while waiting for the initial
       stop would end up as a spurious Quit.

     - removes stdin from the event loop, which we need if attaching
       in the foreground, otherwise on targets that report an initial
       stop on attach (which are most) we'd process input/commands
       while we're in the event loop waiting for that stop.  That is,
       before the attach continuation runs and the command is really
       finished.  */
  target_terminal_inferior ();

  /* Set up execution context to know that we should return from
     wait_for_inferior as soon as the target reports a stop.  */
  init_wait_for_inferior ();
  clear_proceed_status (0);

  inferior->needs_setup = 1;

  if (target_is_non_stop_p ())
    {
      /* If we find that the current thread isn't stopped, explicitly
	 do so now, because we're going to install breakpoints and
	 poke at memory.  */

      if (async_exec)
	/* The user requested an `attach&'; stop just one thread.  */
	target_stop (inferior_ptid);
      else
	/* The user requested an `attach', so stop all threads of this
	   inferior.  */
	target_stop (pid_to_ptid (ptid_get_pid (inferior_ptid)));
    }

  mode = async_exec ? ATTACH_POST_WAIT_RESUME : ATTACH_POST_WAIT_STOP;

  /* Some system don't generate traps when attaching to inferior.
     E.g. Mach 3 or GNU hurd.  */
  if (!target_attach_no_wait)
    {
      struct attach_command_continuation_args *a;

      /* Careful here.  See comments in inferior.h.  Basically some
	 OSes don't ignore SIGSTOPs on continue requests anymore.  We
	 need a way for handle_inferior_event to reset the stop_signal
	 variable after an attach, and this is what
	 STOP_QUIETLY_NO_SIGSTOP is for.  */
      inferior->control.stop_soon = STOP_QUIETLY_NO_SIGSTOP;

      /* Wait for stop.  */
      a = XNEW (struct attach_command_continuation_args);
      a->args = xstrdup (args);
      a->from_tty = from_tty;
      a->mode = mode;
      add_inferior_continuation (attach_command_continuation, a,
				 attach_command_continuation_free_args);
      /* Done with ARGS.  */
      do_cleanups (args_chain);

      if (!target_is_async_p ())
	mark_infrun_async_event_handler ();
      return;
    }

  /* Done with ARGS.  */
  do_cleanups (args_chain);

  attach_post_wait (args, from_tty, mode);
}

/* We had just found out that the target was already attached to an
   inferior.  PTID points at a thread of this new inferior, that is
   the most likely to be stopped right now, but not necessarily so.
   The new inferior is assumed to be already added to the inferior
   list at this point.  If LEAVE_RUNNING, then leave the threads of
   this inferior running, except those we've explicitly seen reported
   as stopped.  */

void
notice_new_inferior (ptid_t ptid, int leave_running, int from_tty)
{
  struct cleanup* old_chain;
  enum attach_post_wait_mode mode;

  old_chain = make_cleanup (null_cleanup, NULL);

  mode = leave_running ? ATTACH_POST_WAIT_RESUME : ATTACH_POST_WAIT_NOTHING;

  if (!ptid_equal (inferior_ptid, null_ptid))
    make_cleanup_restore_current_thread ();

  /* Avoid reading registers -- we haven't fetched the target
     description yet.  */
  switch_to_thread_no_regs (find_thread_ptid (ptid));

  /* When we "notice" a new inferior we need to do all the things we
     would normally do if we had just attached to it.  */

  if (is_executing (inferior_ptid))
    {
      struct attach_command_continuation_args *a;
      struct inferior *inferior = current_inferior ();

      /* We're going to install breakpoints, and poke at memory,
	 ensure that the inferior is stopped for a moment while we do
	 that.  */
      target_stop (inferior_ptid);

      inferior->control.stop_soon = STOP_QUIETLY_REMOTE;

      /* Wait for stop before proceeding.  */
      a = XNEW (struct attach_command_continuation_args);
      a->args = xstrdup ("");
      a->from_tty = from_tty;
      a->mode = mode;
      add_inferior_continuation (attach_command_continuation, a,
				 attach_command_continuation_free_args);

      do_cleanups (old_chain);
      return;
    }

  attach_post_wait ("" /* args */, from_tty, mode);

  do_cleanups (old_chain);
}

/*
 * detach_command --
 * takes a program previously attached to and detaches it.
 * The program resumes execution and will no longer stop
 * on signals, etc.  We better not have left any breakpoints
 * in the program or it'll die when it hits one.  For this
 * to work, it may be necessary for the process to have been
 * previously attached.  It *might* work if the program was
 * started via the normal ptrace (PTRACE_TRACEME).
 */

void
detach_command (char *args, int from_tty)
{
  dont_repeat ();		/* Not for the faint of heart.  */

  if (ptid_equal (inferior_ptid, null_ptid))
    error (_("The program is not being run."));

  query_if_trace_running (from_tty);

  disconnect_tracing ();

  target_detach (args, from_tty);

  /* The current inferior process was just detached successfully.  Get
     rid of breakpoints that no longer make sense.  Note we don't do
     this within target_detach because that is also used when
     following child forks, and in that case we will want to transfer
     breakpoints to the child, not delete them.  */
  breakpoint_init_inferior (inf_exited);

  /* If the solist is global across inferiors, don't clear it when we
     detach from a single inferior.  */
  if (!gdbarch_has_global_solist (target_gdbarch ()))
    no_shared_libraries (NULL, from_tty);

  /* If we still have inferiors to debug, then don't mess with their
     threads.  */
  if (!have_inferiors ())
    init_thread_list ();

  if (deprecated_detach_hook)
    deprecated_detach_hook ();
}

/* Disconnect from the current target without resuming it (leaving it
   waiting for a debugger).

   We'd better not have left any breakpoints in the program or the
   next debugger will get confused.  Currently only supported for some
   remote targets, since the normal attach mechanisms don't work on
   stopped processes on some native platforms (e.g. GNU/Linux).  */

static void
disconnect_command (char *args, int from_tty)
{
  dont_repeat ();		/* Not for the faint of heart.  */
  query_if_trace_running (from_tty);
  disconnect_tracing ();
  target_disconnect (args, from_tty);
  no_shared_libraries (NULL, from_tty);
  init_thread_list ();
  if (deprecated_detach_hook)
    deprecated_detach_hook ();
}

void 
interrupt_target_1 (int all_threads)
{
  ptid_t ptid;

  if (all_threads)
    ptid = minus_one_ptid;
  else
    ptid = inferior_ptid;

  if (non_stop)
    target_stop (ptid);
  else
    target_interrupt (ptid);

  /* Tag the thread as having been explicitly requested to stop, so
     other parts of gdb know not to resume this thread automatically,
     if it was stopped due to an internal event.  Limit this to
     non-stop mode, as when debugging a multi-threaded application in
     all-stop mode, we will only get one stop event --- it's undefined
     which thread will report the event.  */
  if (non_stop)
    set_stop_requested (ptid, 1);
}

/* interrupt [-a]
   Stop the execution of the target while running in async mode, in
   the background.  In all-stop, stop the whole process.  In non-stop
   mode, stop the current thread only by default, or stop all threads
   if the `-a' switch is used.  */

static void
interrupt_command (char *args, int from_tty)
{
  if (target_can_async_p ())
    {
      int all_threads = 0;

      dont_repeat ();		/* Not for the faint of heart.  */

      if (args != NULL
	  && startswith (args, "-a"))
	all_threads = 1;

      if (!non_stop && all_threads)
	error (_("-a is meaningless in all-stop mode."));

      interrupt_target_1 (all_threads);
    }
}

/* See inferior.h.  */

void
default_print_float_info (struct gdbarch *gdbarch, struct ui_file *file,
			  struct frame_info *frame, const char *args)
{
  int regnum;
  int printed_something = 0;

  for (regnum = 0;
       regnum < gdbarch_num_regs (gdbarch)
	 + gdbarch_num_pseudo_regs (gdbarch);
       regnum++)
    {
      if (gdbarch_register_reggroup_p (gdbarch, regnum, float_reggroup))
	{
	  printed_something = 1;
	  gdbarch_print_registers_info (gdbarch, file, frame, regnum, 1);
	}
    }
  if (!printed_something)
    fprintf_filtered (file, "No floating-point info "
		      "available for this processor.\n");
}

static void
float_info (char *args, int from_tty)
{
  struct frame_info *frame;

  if (!target_has_registers)
    error (_("The program has no registers now."));

  frame = get_selected_frame (NULL);
  gdbarch_print_float_info (get_frame_arch (frame), gdb_stdout, frame, args);
}

static void
unset_command (char *args, int from_tty)
{
  printf_filtered (_("\"unset\" must be followed by the "
		     "name of an unset subcommand.\n"));
  help_list (unsetlist, "unset ", all_commands, gdb_stdout);
}

/* Implement `info proc' family of commands.  */

static void
info_proc_cmd_1 (char *args, enum info_proc_what what, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (!target_info_proc (args, what))
    {
      if (gdbarch_info_proc_p (gdbarch))
	gdbarch_info_proc (gdbarch, args, what);
      else
	error (_("Not supported on this target."));
    }
}

/* Implement `info proc' when given without any futher parameters.  */

static void
info_proc_cmd (char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_MINIMAL, from_tty);
}

/* Implement `info proc mappings'.  */

static void
info_proc_cmd_mappings (char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_MAPPINGS, from_tty);
}

/* Implement `info proc stat'.  */

static void
info_proc_cmd_stat (char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_STAT, from_tty);
}

/* Implement `info proc status'.  */

static void
info_proc_cmd_status (char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_STATUS, from_tty);
}

/* Implement `info proc cwd'.  */

static void
info_proc_cmd_cwd (char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_CWD, from_tty);
}

/* Implement `info proc cmdline'.  */

static void
info_proc_cmd_cmdline (char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_CMDLINE, from_tty);
}

/* Implement `info proc exe'.  */

static void
info_proc_cmd_exe (char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_EXE, from_tty);
}

/* Implement `info proc all'.  */

static void
info_proc_cmd_all (char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_ALL, from_tty);
}

void
_initialize_infcmd (void)
{
  static struct cmd_list_element *info_proc_cmdlist;
  struct cmd_list_element *c = NULL;
  const char *cmd_name;

  /* Add the filename of the terminal connected to inferior I/O.  */
  add_setshow_optional_filename_cmd ("inferior-tty", class_run,
				     &inferior_io_terminal_scratch, _("\
Set terminal for future runs of program being debugged."), _("\
Show terminal for future runs of program being debugged."), _("\
Usage: set inferior-tty [TTY]\n\n\
If TTY is omitted, the default behavior of using the same terminal as GDB\n\
is restored."),
				     set_inferior_tty_command,
				     show_inferior_tty_command,
				     &setlist, &showlist);
  cmd_name = "inferior-tty";
  c = lookup_cmd (&cmd_name, setlist, "", -1, 1);
  gdb_assert (c != NULL);
  add_alias_cmd ("tty", c, class_alias, 0, &cmdlist);

  cmd_name = "args";
  add_setshow_string_noescape_cmd (cmd_name, class_run,
				   &inferior_args_scratch, _("\
Set argument list to give program being debugged when it is started."), _("\
Show argument list to give program being debugged when it is started."), _("\
Follow this command with any number of args, to be passed to the program."),
				   set_args_command,
				   show_args_command,
				   &setlist, &showlist);
  c = lookup_cmd (&cmd_name, setlist, "", -1, 1);
  gdb_assert (c != NULL);
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("environment", no_class, environment_info, _("\
The environment to give the program, or one variable's value.\n\
With an argument VAR, prints the value of environment variable VAR to\n\
give the program being debugged.  With no arguments, prints the entire\n\
environment to be given to the program."), &showlist);
  set_cmd_completer (c, noop_completer);

  add_prefix_cmd ("unset", no_class, unset_command,
		  _("Complement to certain \"set\" commands."),
		  &unsetlist, "unset ", 0, &cmdlist);

  c = add_cmd ("environment", class_run, unset_environment_command, _("\
Cancel environment variable VAR for the program.\n\
This does not affect the program until the next \"run\" command."),
	       &unsetlist);
  set_cmd_completer (c, noop_completer);

  c = add_cmd ("environment", class_run, set_environment_command, _("\
Set environment variable value to give the program.\n\
Arguments are VAR VALUE where VAR is variable name and VALUE is value.\n\
VALUES of environment variables are uninterpreted strings.\n\
This does not affect the program until the next \"run\" command."),
	       &setlist);
  set_cmd_completer (c, noop_completer);

  c = add_com ("path", class_files, path_command, _("\
Add directory DIR(s) to beginning of search path for object files.\n\
$cwd in the path means the current working directory.\n\
This path is equivalent to the $PATH shell variable.  It is a list of\n\
directories, separated by colons.  These directories are searched to find\n\
fully linked executable files and separately compiled object files as \
needed."));
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("paths", no_class, path_info, _("\
Current search path for finding object files.\n\
$cwd in the path means the current working directory.\n\
This path is equivalent to the $PATH shell variable.  It is a list of\n\
directories, separated by colons.  These directories are searched to find\n\
fully linked executable files and separately compiled object files as \
needed."),
	       &showlist);
  set_cmd_completer (c, noop_completer);

  add_prefix_cmd ("kill", class_run, kill_command,
		  _("Kill execution of program being debugged."),
		  &killlist, "kill ", 0, &cmdlist);

  add_com ("attach", class_run, attach_command, _("\
Attach to a process or file outside of GDB.\n\
This command attaches to another target, of the same type as your last\n\
\"target\" command (\"info files\" will show your target stack).\n\
The command may take as argument a process id or a device file.\n\
For a process id, you must have permission to send the process a signal,\n\
and it must have the same effective uid as the debugger.\n\
When using \"attach\" with a process id, the debugger finds the\n\
program running in the process, looking first in the current working\n\
directory, or (if not found there) using the source file search path\n\
(see the \"directory\" command).  You can also use the \"file\" command\n\
to specify the program, and to load its symbol table."));

  add_prefix_cmd ("detach", class_run, detach_command, _("\
Detach a process or file previously attached.\n\
If a process, it is no longer traced, and it continues its execution.  If\n\
you were debugging a file, the file is closed and gdb no longer accesses it."),
		  &detachlist, "detach ", 0, &cmdlist);

  add_com ("disconnect", class_run, disconnect_command, _("\
Disconnect from a target.\n\
The target will wait for another debugger to connect.  Not available for\n\
all targets."));

  c = add_com ("signal", class_run, signal_command, _("\
Continue program with the specified signal.\n\
Usage: signal SIGNAL\n\
The SIGNAL argument is processed the same as the handle command.\n\
\n\
An argument of \"0\" means continue the program without sending it a signal.\n\
This is useful in cases where the program stopped because of a signal,\n\
and you want to resume the program while discarding the signal.\n\
\n\
In a multi-threaded program the signal is delivered to, or discarded from,\n\
the current thread only."));
  set_cmd_completer (c, signal_completer);

  c = add_com ("queue-signal", class_run, queue_signal_command, _("\
Queue a signal to be delivered to the current thread when it is resumed.\n\
Usage: queue-signal SIGNAL\n\
The SIGNAL argument is processed the same as the handle command.\n\
It is an error if the handling state of SIGNAL is \"nopass\".\n\
\n\
An argument of \"0\" means remove any currently queued signal from\n\
the current thread.  This is useful in cases where the program stopped\n\
because of a signal, and you want to resume it while discarding the signal.\n\
\n\
In a multi-threaded program the signal is queued with, or discarded from,\n\
the current thread only."));
  set_cmd_completer (c, signal_completer);

  add_com ("stepi", class_run, stepi_command, _("\
Step one instruction exactly.\n\
Usage: stepi [N]\n\
Argument N means step N times (or till program stops for another \
reason)."));
  add_com_alias ("si", "stepi", class_alias, 0);

  add_com ("nexti", class_run, nexti_command, _("\
Step one instruction, but proceed through subroutine calls.\n\
Usage: nexti [N]\n\
Argument N means step N times (or till program stops for another \
reason)."));
  add_com_alias ("ni", "nexti", class_alias, 0);

  add_com ("finish", class_run, finish_command, _("\
Execute until selected stack frame returns.\n\
Usage: finish\n\
Upon return, the value returned is printed and put in the value history."));
  add_com_alias ("fin", "finish", class_run, 1);

  add_com ("next", class_run, next_command, _("\
Step program, proceeding through subroutine calls.\n\
Usage: next [N]\n\
Unlike \"step\", if the current source line calls a subroutine,\n\
this command does not enter the subroutine, but instead steps over\n\
the call, in effect treating it as a single source line."));
  add_com_alias ("n", "next", class_run, 1);

  add_com ("step", class_run, step_command, _("\
Step program until it reaches a different source line.\n\
Usage: step [N]\n\
Argument N means step N times (or till program stops for another \
reason)."));
  add_com_alias ("s", "step", class_run, 1);

  c = add_com ("until", class_run, until_command, _("\
Execute until the program reaches a source line greater than the current\n\
or a specified location (same args as break command) within the current \
frame."));
  set_cmd_completer (c, location_completer);
  add_com_alias ("u", "until", class_run, 1);

  c = add_com ("advance", class_run, advance_command, _("\
Continue the program up to the given location (same form as args for break \
command).\n\
Execution will also stop upon exit from the current stack frame."));
  set_cmd_completer (c, location_completer);

  c = add_com ("jump", class_run, jump_command, _("\
Continue program being debugged at specified line or address.\n\
Usage: jump <location>\n\
Give as argument either LINENUM or *ADDR, where ADDR is an expression\n\
for an address to start at."));
  set_cmd_completer (c, location_completer);
  add_com_alias ("j", "jump", class_run, 1);

  add_com ("continue", class_run, continue_command, _("\
Continue program being debugged, after signal or breakpoint.\n\
Usage: continue [N]\n\
If proceeding from breakpoint, a number N may be used as an argument,\n\
which means to set the ignore count of that breakpoint to N - 1 (so that\n\
the breakpoint won't break until the Nth time it is reached).\n\
\n\
If non-stop mode is enabled, continue only the current thread,\n\
otherwise all the threads in the program are continued.  To \n\
continue all stopped threads in non-stop mode, use the -a option.\n\
Specifying -a and an ignore count simultaneously is an error."));
  add_com_alias ("c", "cont", class_run, 1);
  add_com_alias ("fg", "cont", class_run, 1);

  c = add_com ("run", class_run, run_command, _("\
Start debugged program.  You may specify arguments to give it.\n\
Args may include \"*\", or \"[...]\"; they are expanded using \"sh\".\n\
Input and output redirection with \">\", \"<\", or \">>\" are also \
allowed.\n\n\
With no arguments, uses arguments last specified (with \"run\" \
or \"set args\").\n\
To cancel previous arguments and run with no arguments,\n\
use \"set args\" without arguments."));
  set_cmd_completer (c, filename_completer);
  add_com_alias ("r", "run", class_run, 1);

  c = add_com ("start", class_run, start_command, _("\
Run the debugged program until the beginning of the main procedure.\n\
You may specify arguments to give to your program, just as with the\n\
\"run\" command."));
  set_cmd_completer (c, filename_completer);

  add_com ("interrupt", class_run, interrupt_command,
	   _("Interrupt the execution of the debugged program.\n\
If non-stop mode is enabled, interrupt only the current thread,\n\
otherwise all the threads in the program are stopped.  To \n\
interrupt all running threads in non-stop mode, use the -a option."));

  c = add_info ("registers", nofp_registers_info, _("\
List of integer registers and their contents, for selected stack frame.\n\
Register name as argument means describe only that register."));
  add_info_alias ("r", "registers", 1);
  set_cmd_completer (c, reg_or_group_completer);

  c = add_info ("all-registers", all_registers_info, _("\
List of all registers and their contents, for selected stack frame.\n\
Register name as argument means describe only that register."));
  set_cmd_completer (c, reg_or_group_completer);

  add_info ("program", program_info,
	    _("Execution status of the program."));

  add_info ("float", float_info,
	    _("Print the status of the floating point unit\n"));

  add_info ("vector", vector_info,
	    _("Print the status of the vector unit\n"));

  add_prefix_cmd ("proc", class_info, info_proc_cmd,
		  _("\
Show /proc process information about any running process.\n\
Specify any process id, or use the program being debugged by default."),
		  &info_proc_cmdlist, "info proc ",
		  1/*allow-unknown*/, &infolist);

  add_cmd ("mappings", class_info, info_proc_cmd_mappings, _("\
List of mapped memory regions."),
	   &info_proc_cmdlist);

  add_cmd ("stat", class_info, info_proc_cmd_stat, _("\
List process info from /proc/PID/stat."),
	   &info_proc_cmdlist);

  add_cmd ("status", class_info, info_proc_cmd_status, _("\
List process info from /proc/PID/status."),
	   &info_proc_cmdlist);

  add_cmd ("cwd", class_info, info_proc_cmd_cwd, _("\
List current working directory of the process."),
	   &info_proc_cmdlist);

  add_cmd ("cmdline", class_info, info_proc_cmd_cmdline, _("\
List command line arguments of the process."),
	   &info_proc_cmdlist);

  add_cmd ("exe", class_info, info_proc_cmd_exe, _("\
List absolute filename for executable of the process."),
	   &info_proc_cmdlist);

  add_cmd ("all", class_info, info_proc_cmd_all, _("\
List all available /proc info."),
	   &info_proc_cmdlist);
}
