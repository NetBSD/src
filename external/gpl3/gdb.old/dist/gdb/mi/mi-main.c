/* MI Command Set.

   Copyright (C) 2000-2017 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "target.h"
#include "inferior.h"
#include "infrun.h"
#include "top.h"
#include "gdbthread.h"
#include "mi-cmds.h"
#include "mi-parse.h"
#include "mi-getopt.h"
#include "mi-console.h"
#include "ui-out.h"
#include "mi-out.h"
#include "interps.h"
#include "event-loop.h"
#include "event-top.h"
#include "gdbcore.h"		/* For write_memory().  */
#include "value.h"
#include "regcache.h"
#include "gdb.h"
#include "frame.h"
#include "mi-main.h"
#include "mi-common.h"
#include "language.h"
#include "valprint.h"
#include "inferior.h"
#include "osdata.h"
#include "splay-tree.h"
#include "tracepoint.h"
#include "ctf.h"
#include "ada-lang.h"
#include "linespec.h"
#include "extension.h"
#include "gdbcmd.h"
#include "observer.h"
#include "common/gdb_optional.h"

#include <ctype.h>
#include "run-time-clock.h"
#include <chrono>

enum
  {
    FROM_TTY = 0
  };

int mi_debug_p;

/* This is used to pass the current command timestamp down to
   continuation routines.  */
static struct mi_timestamp *current_command_ts;

static int do_timings = 0;

char *current_token;
/* Few commands would like to know if options like --thread-group were
   explicitly specified.  This variable keeps the current parsed
   command including all option, and make it possible.  */
static struct mi_parse *current_context;

int running_result_record_printed = 1;

/* Flag indicating that the target has proceeded since the last
   command was issued.  */
int mi_proceeded;

extern void _initialize_mi_main (void);
static void mi_cmd_execute (struct mi_parse *parse);

static void mi_execute_cli_command (const char *cmd, int args_p,
				    const char *args);
static void mi_execute_async_cli_command (const char *cli_command,
					  char **argv, int argc);
static int register_changed_p (int regnum, struct regcache *,
			       struct regcache *);
static void output_register (struct frame_info *, int regnum, int format,
			     int skip_unavailable);

/* Controls whether the frontend wants MI in async mode.  */
static int mi_async = 0;

/* The set command writes to this variable.  If the inferior is
   executing, mi_async is *not* updated.  */
static int mi_async_1 = 0;

static void
set_mi_async_command (char *args, int from_tty,
		      struct cmd_list_element *c)
{
  if (have_live_inferiors ())
    {
      mi_async_1 = mi_async;
      error (_("Cannot change this setting while the inferior is running."));
    }

  mi_async = mi_async_1;
}

static void
show_mi_async_command (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c,
		       const char *value)
{
  fprintf_filtered (file,
		    _("Whether MI is in asynchronous mode is %s.\n"),
		    value);
}

/* A wrapper for target_can_async_p that takes the MI setting into
   account.  */

int
mi_async_p (void)
{
  return mi_async && target_can_async_p ();
}

/* Command implementations.  FIXME: Is this libgdb?  No.  This is the MI
   layer that calls libgdb.  Any operation used in the below should be
   formalized.  */

static void timestamp (struct mi_timestamp *tv);

static void print_diff (struct ui_file *file, struct mi_timestamp *start,
			struct mi_timestamp *end);

void
mi_cmd_gdb_exit (const char *command, char **argv, int argc)
{
  struct mi_interp *mi = (struct mi_interp *) current_interpreter ();

  /* We have to print everything right here because we never return.  */
  if (current_token)
    fputs_unfiltered (current_token, mi->raw_stdout);
  fputs_unfiltered ("^exit\n", mi->raw_stdout);
  mi_out_put (current_uiout, mi->raw_stdout);
  gdb_flush (mi->raw_stdout);
  /* FIXME: The function called is not yet a formal libgdb function.  */
  quit_force (NULL, FROM_TTY);
}

void
mi_cmd_exec_next (const char *command, char **argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-next", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("next", argv, argc);
}

void
mi_cmd_exec_next_instruction (const char *command, char **argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-nexti", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("nexti", argv, argc);
}

void
mi_cmd_exec_step (const char *command, char **argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-step", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("step", argv, argc);
}

void
mi_cmd_exec_step_instruction (const char *command, char **argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-stepi", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("stepi", argv, argc);
}

void
mi_cmd_exec_finish (const char *command, char **argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-finish", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("finish", argv, argc);
}

void
mi_cmd_exec_return (const char *command, char **argv, int argc)
{
  /* This command doesn't really execute the target, it just pops the
     specified number of frames.  */
  if (argc)
    /* Call return_command with from_tty argument equal to 0 so as to
       avoid being queried.  */
    return_command (*argv, 0);
  else
    /* Call return_command with from_tty argument equal to 0 so as to
       avoid being queried.  */
    return_command (NULL, 0);

  /* Because we have called return_command with from_tty = 0, we need
     to print the frame here.  */
  print_stack_frame (get_selected_frame (NULL), 1, LOC_AND_ADDRESS, 1);
}

void
mi_cmd_exec_jump (const char *args, char **argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  mi_execute_async_cli_command ("jump", argv, argc);
}

static void
proceed_thread (struct thread_info *thread, int pid)
{
  if (!is_stopped (thread->ptid))
    return;

  if (pid != 0 && ptid_get_pid (thread->ptid) != pid)
    return;

  switch_to_thread (thread->ptid);
  clear_proceed_status (0);
  proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
}

static int
proceed_thread_callback (struct thread_info *thread, void *arg)
{
  int pid = *(int *)arg;

  proceed_thread (thread, pid);
  return 0;
}

static void
exec_continue (char **argv, int argc)
{
  prepare_execution_command (&current_target, mi_async_p ());

  if (non_stop)
    {
      /* In non-stop mode, 'resume' always resumes a single thread.
	 Therefore, to resume all threads of the current inferior, or
	 all threads in all inferiors, we need to iterate over
	 threads.

	 See comment on infcmd.c:proceed_thread_callback for rationale.  */
      if (current_context->all || current_context->thread_group != -1)
	{
	  int pid = 0;
	  struct cleanup *back_to = make_cleanup_restore_current_thread ();

	  if (!current_context->all)
	    {
	      struct inferior *inf
		= find_inferior_id (current_context->thread_group);

	      pid = inf->pid;
	    }
	  iterate_over_threads (proceed_thread_callback, &pid);
	  do_cleanups (back_to);
	}
      else
	{
	  continue_1 (0);
	}
    }
  else
    {
      scoped_restore save_multi = make_scoped_restore (&sched_multi);

      if (current_context->all)
	{
	  sched_multi = 1;
	  continue_1 (0);
	}
      else
	{
	  /* In all-stop mode, -exec-continue traditionally resumed
	     either all threads, or one thread, depending on the
	     'scheduler-locking' variable.  Let's continue to do the
	     same.  */
	  continue_1 (1);
	}
    }
}

static void
exec_reverse_continue (char **argv, int argc)
{
  enum exec_direction_kind dir = execution_direction;

  if (dir == EXEC_REVERSE)
    error (_("Already in reverse mode."));

  if (!target_can_execute_reverse)
    error (_("Target %s does not support this command."), target_shortname);

  scoped_restore save_exec_dir = make_scoped_restore (&execution_direction,
						      EXEC_REVERSE);
  exec_continue (argv, argc);
}

void
mi_cmd_exec_continue (const char *command, char **argv, int argc)
{
  if (argc > 0 && strcmp (argv[0], "--reverse") == 0)
    exec_reverse_continue (argv + 1, argc - 1);
  else
    exec_continue (argv, argc);
}

static int
interrupt_thread_callback (struct thread_info *thread, void *arg)
{
  int pid = *(int *)arg;

  if (!is_running (thread->ptid))
    return 0;

  if (ptid_get_pid (thread->ptid) != pid)
    return 0;

  target_stop (thread->ptid);
  return 0;
}

/* Interrupt the execution of the target.  Note how we must play
   around with the token variables, in order to display the current
   token in the result of the interrupt command, and the previous
   execution token when the target finally stops.  See comments in
   mi_cmd_execute.  */

void
mi_cmd_exec_interrupt (const char *command, char **argv, int argc)
{
  /* In all-stop mode, everything stops, so we don't need to try
     anything specific.  */
  if (!non_stop)
    {
      interrupt_target_1 (0);
      return;
    }

  if (current_context->all)
    {
      /* This will interrupt all threads in all inferiors.  */
      interrupt_target_1 (1);
    }
  else if (current_context->thread_group != -1)
    {
      struct inferior *inf = find_inferior_id (current_context->thread_group);

      iterate_over_threads (interrupt_thread_callback, &inf->pid);
    }
  else
    {
      /* Interrupt just the current thread -- either explicitly
	 specified via --thread or whatever was current before
	 MI command was sent.  */
      interrupt_target_1 (0);
    }
}

/* Callback for iterate_over_inferiors which starts the execution
   of the given inferior.

   ARG is a pointer to an integer whose value, if non-zero, indicates
   that the program should be stopped when reaching the main subprogram
   (similar to what the CLI "start" command does).  */

static int
run_one_inferior (struct inferior *inf, void *arg)
{
  int start_p = *(int *) arg;
  const char *run_cmd = start_p ? "start" : "run";
  struct target_ops *run_target = find_run_target ();
  int async_p = mi_async && run_target->to_can_async_p (run_target);

  if (inf->pid != 0)
    {
      if (inf->pid != ptid_get_pid (inferior_ptid))
	{
	  struct thread_info *tp;

	  tp = any_thread_of_process (inf->pid);
	  if (!tp)
	    error (_("Inferior has no threads."));

	  switch_to_thread (tp->ptid);
	}
    }
  else
    {
      set_current_inferior (inf);
      switch_to_thread (null_ptid);
      set_current_program_space (inf->pspace);
    }
  mi_execute_cli_command (run_cmd, async_p,
			  async_p ? "&" : NULL);
  return 0;
}

void
mi_cmd_exec_run (const char *command, char **argv, int argc)
{
  int start_p = 0;

  /* Parse the command options.  */
  enum opt
    {
      START_OPT,
    };
  static const struct mi_opt opts[] =
    {
	{"-start", START_OPT, 0},
	{NULL, 0, 0},
    };

  int oind = 0;
  char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-exec-run", argc, argv, opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case START_OPT:
	  start_p = 1;
	  break;
	}
    }

  /* This command does not accept any argument.  Make sure the user
     did not provide any.  */
  if (oind != argc)
    error (_("Invalid argument: %s"), argv[oind]);

  if (current_context->all)
    {
      struct cleanup *back_to = save_current_space_and_thread ();

      iterate_over_inferiors (run_one_inferior, &start_p);
      do_cleanups (back_to);
    }
  else
    {
      const char *run_cmd = start_p ? "start" : "run";
      struct target_ops *run_target = find_run_target ();
      int async_p = mi_async && run_target->to_can_async_p (run_target);

      mi_execute_cli_command (run_cmd, async_p,
			      async_p ? "&" : NULL);
    }
}


static int
find_thread_of_process (struct thread_info *ti, void *p)
{
  int pid = *(int *)p;

  if (ptid_get_pid (ti->ptid) == pid && !is_exited (ti->ptid))
    return 1;

  return 0;
}

void
mi_cmd_target_detach (const char *command, char **argv, int argc)
{
  if (argc != 0 && argc != 1)
    error (_("Usage: -target-detach [pid | thread-group]"));

  if (argc == 1)
    {
      struct thread_info *tp;
      char *end = argv[0];
      int pid;

      /* First see if we are dealing with a thread-group id.  */
      if (*argv[0] == 'i')
	{
	  struct inferior *inf;
	  int id = strtoul (argv[0] + 1, &end, 0);

	  if (*end != '\0')
	    error (_("Invalid syntax of thread-group id '%s'"), argv[0]);

	  inf = find_inferior_id (id);
	  if (!inf)
	    error (_("Non-existent thread-group id '%d'"), id);

	  pid = inf->pid;
	}
      else
	{
	  /* We must be dealing with a pid.  */
	  pid = strtol (argv[0], &end, 10);

	  if (*end != '\0')
	    error (_("Invalid identifier '%s'"), argv[0]);
	}

      /* Pick any thread in the desired process.  Current
	 target_detach detaches from the parent of inferior_ptid.  */
      tp = iterate_over_threads (find_thread_of_process, &pid);
      if (!tp)
	error (_("Thread group is empty"));

      switch_to_thread (tp->ptid);
    }

  detach_command (NULL, 0);
}

void
mi_cmd_target_flash_erase (const char *command, char **argv, int argc)
{
  flash_erase_command (NULL, 0);
}

void
mi_cmd_thread_select (const char *command, char **argv, int argc)
{
  enum gdb_rc rc;
  char *mi_error_message;
  ptid_t previous_ptid = inferior_ptid;

  if (argc != 1)
    error (_("-thread-select: USAGE: threadnum."));

  rc = gdb_thread_select (current_uiout, argv[0], &mi_error_message);

  /* If thread switch did not succeed don't notify or print.  */
  if (rc == GDB_RC_FAIL)
    {
      make_cleanup (xfree, mi_error_message);
      error ("%s", mi_error_message);
    }

  print_selected_thread_frame (current_uiout,
			       USER_SELECTED_THREAD | USER_SELECTED_FRAME);

  /* Notify if the thread has effectively changed.  */
  if (!ptid_equal (inferior_ptid, previous_ptid))
    {
      observer_notify_user_selected_context_changed (USER_SELECTED_THREAD
						     | USER_SELECTED_FRAME);
    }
}

void
mi_cmd_thread_list_ids (const char *command, char **argv, int argc)
{
  enum gdb_rc rc;
  char *mi_error_message;

  if (argc != 0)
    error (_("-thread-list-ids: No arguments required."));

  rc = gdb_list_thread_ids (current_uiout, &mi_error_message);

  if (rc == GDB_RC_FAIL)
    {
      make_cleanup (xfree, mi_error_message);
      error ("%s", mi_error_message);
    }
}

void
mi_cmd_thread_info (const char *command, char **argv, int argc)
{
  if (argc != 0 && argc != 1)
    error (_("Invalid MI command"));

  print_thread_info (current_uiout, argv[0], -1);
}

struct collect_cores_data
{
  int pid;

  VEC (int) *cores;
};

static int
collect_cores (struct thread_info *ti, void *xdata)
{
  struct collect_cores_data *data = (struct collect_cores_data *) xdata;

  if (ptid_get_pid (ti->ptid) == data->pid)
    {
      int core = target_core_of_thread (ti->ptid);

      if (core != -1)
	VEC_safe_push (int, data->cores, core);
    }

  return 0;
}

static int *
unique (int *b, int *e)
{
  int *d = b;

  while (++b != e)
    if (*d != *b)
      *++d = *b;
  return ++d;
}

struct print_one_inferior_data
{
  int recurse;
  VEC (int) *inferiors;
};

static int
print_one_inferior (struct inferior *inferior, void *xdata)
{
  struct print_one_inferior_data *top_data
    = (struct print_one_inferior_data *) xdata;
  struct ui_out *uiout = current_uiout;

  if (VEC_empty (int, top_data->inferiors)
      || bsearch (&(inferior->pid), VEC_address (int, top_data->inferiors),
		  VEC_length (int, top_data->inferiors), sizeof (int),
		  compare_positive_ints))
    {
      struct collect_cores_data data;
      struct cleanup *back_to
	= make_cleanup_ui_out_tuple_begin_end (uiout, NULL);

      uiout->field_fmt ("id", "i%d", inferior->num);
      uiout->field_string ("type", "process");
      if (inferior->has_exit_code)
	uiout->field_string ("exit-code",
			     int_string (inferior->exit_code, 8, 0, 0, 1));
      if (inferior->pid != 0)
	uiout->field_int ("pid", inferior->pid);

      if (inferior->pspace->pspace_exec_filename != NULL)
	{
	  uiout->field_string ("executable",
			       inferior->pspace->pspace_exec_filename);
	}

      data.cores = 0;
      if (inferior->pid != 0)
	{
	  data.pid = inferior->pid;
	  iterate_over_threads (collect_cores, &data);
	}

      if (!VEC_empty (int, data.cores))
	{
	  int *b, *e;
	  struct cleanup *back_to_2 =
	    make_cleanup_ui_out_list_begin_end (uiout, "cores");

	  qsort (VEC_address (int, data.cores),
		 VEC_length (int, data.cores), sizeof (int),
		 compare_positive_ints);

	  b = VEC_address (int, data.cores);
	  e = b + VEC_length (int, data.cores);
	  e = unique (b, e);

	  for (; b != e; ++b)
	    uiout->field_int (NULL, *b);

	  do_cleanups (back_to_2);
	}

      if (top_data->recurse)
	print_thread_info (uiout, NULL, inferior->pid);

      do_cleanups (back_to);
    }

  return 0;
}

/* Output a field named 'cores' with a list as the value.  The
   elements of the list are obtained by splitting 'cores' on
   comma.  */

static void
output_cores (struct ui_out *uiout, const char *field_name, const char *xcores)
{
  struct cleanup *back_to = make_cleanup_ui_out_list_begin_end (uiout,
								field_name);
  char *cores = xstrdup (xcores);
  char *p = cores;

  make_cleanup (xfree, cores);

  for (p = strtok (p, ","); p;  p = strtok (NULL, ","))
    uiout->field_string (NULL, p);

  do_cleanups (back_to);
}

static void
free_vector_of_ints (void *xvector)
{
  VEC (int) **vector = (VEC (int) **) xvector;

  VEC_free (int, *vector);
}

static void
do_nothing (splay_tree_key k)
{
}

static void
free_vector_of_osdata_items (splay_tree_value xvalue)
{
  VEC (osdata_item_s) *value = (VEC (osdata_item_s) *) xvalue;

  /* We don't free the items itself, it will be done separately.  */
  VEC_free (osdata_item_s, value);
}

static int
splay_tree_int_comparator (splay_tree_key xa, splay_tree_key xb)
{
  int a = xa;
  int b = xb;

  return a - b;
}

static void
free_splay_tree (void *xt)
{
  splay_tree t = (splay_tree) xt;
  splay_tree_delete (t);
}

static void
list_available_thread_groups (VEC (int) *ids, int recurse)
{
  struct osdata *data;
  struct osdata_item *item;
  int ix_items;
  struct ui_out *uiout = current_uiout;
  struct cleanup *cleanup;

  /* This keeps a map from integer (pid) to VEC (struct osdata_item *)*
     The vector contains information about all threads for the given pid.
     This is assigned an initial value to avoid "may be used uninitialized"
     warning from gcc.  */
  splay_tree tree = NULL;

  /* get_osdata will throw if it cannot return data.  */
  data = get_osdata ("processes");
  cleanup = make_cleanup_osdata_free (data);

  if (recurse)
    {
      struct osdata *threads = get_osdata ("threads");

      make_cleanup_osdata_free (threads);
      tree = splay_tree_new (splay_tree_int_comparator,
			     do_nothing,
			     free_vector_of_osdata_items);
      make_cleanup (free_splay_tree, tree);

      for (ix_items = 0;
	   VEC_iterate (osdata_item_s, threads->items,
			ix_items, item);
	   ix_items++)
	{
	  const char *pid = get_osdata_column (item, "pid");
	  int pid_i = strtoul (pid, NULL, 0);
	  VEC (osdata_item_s) *vec = 0;

	  splay_tree_node n = splay_tree_lookup (tree, pid_i);
	  if (!n)
	    {
	      VEC_safe_push (osdata_item_s, vec, item);
	      splay_tree_insert (tree, pid_i, (splay_tree_value)vec);
	    }
	  else
	    {
	      vec = (VEC (osdata_item_s) *) n->value;
	      VEC_safe_push (osdata_item_s, vec, item);
	      n->value = (splay_tree_value) vec;
	    }
	}
    }

  make_cleanup_ui_out_list_begin_end (uiout, "groups");

  for (ix_items = 0;
       VEC_iterate (osdata_item_s, data->items,
		    ix_items, item);
       ix_items++)
    {
      struct cleanup *back_to;

      const char *pid = get_osdata_column (item, "pid");
      const char *cmd = get_osdata_column (item, "command");
      const char *user = get_osdata_column (item, "user");
      const char *cores = get_osdata_column (item, "cores");

      int pid_i = strtoul (pid, NULL, 0);

      /* At present, the target will return all available processes
	 and if information about specific ones was required, we filter
	 undesired processes here.  */
      if (ids && bsearch (&pid_i, VEC_address (int, ids),
			  VEC_length (int, ids),
			  sizeof (int), compare_positive_ints) == NULL)
	continue;


      back_to = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);

      uiout->field_fmt ("id", "%s", pid);
      uiout->field_string ("type", "process");
      if (cmd)
	uiout->field_string ("description", cmd);
      if (user)
	uiout->field_string ("user", user);
      if (cores)
	output_cores (uiout, "cores", cores);

      if (recurse)
	{
	  splay_tree_node n = splay_tree_lookup (tree, pid_i);
	  if (n)
	    {
	      VEC (osdata_item_s) *children = (VEC (osdata_item_s) *) n->value;
	      struct osdata_item *child;
	      int ix_child;

	      make_cleanup_ui_out_list_begin_end (uiout, "threads");

	      for (ix_child = 0;
		   VEC_iterate (osdata_item_s, children, ix_child, child);
		   ++ix_child)
		{
		  struct cleanup *back_to_2 =
		    make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
		  const char *tid = get_osdata_column (child, "tid");
		  const char *tcore = get_osdata_column (child, "core");

		  uiout->field_string ("id", tid);
		  if (tcore)
		    uiout->field_string ("core", tcore);

		  do_cleanups (back_to_2);
		}
	    }
	}

      do_cleanups (back_to);
    }

  do_cleanups (cleanup);
}

void
mi_cmd_list_thread_groups (const char *command, char **argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  struct cleanup *back_to;
  int available = 0;
  int recurse = 0;
  VEC (int) *ids = 0;

  enum opt
  {
    AVAILABLE_OPT, RECURSE_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"-available", AVAILABLE_OPT, 0},
      {"-recurse", RECURSE_OPT, 1},
      { 0, 0, 0 }
    };

  int oind = 0;
  char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-list-thread-groups", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case AVAILABLE_OPT:
	  available = 1;
	  break;
	case RECURSE_OPT:
	  if (strcmp (oarg, "0") == 0)
	    ;
	  else if (strcmp (oarg, "1") == 0)
	    recurse = 1;
	  else
	    error (_("only '0' and '1' are valid values "
		     "for the '--recurse' option"));
	  break;
	}
    }

  for (; oind < argc; ++oind)
    {
      char *end;
      int inf;

      if (*(argv[oind]) != 'i')
	error (_("invalid syntax of group id '%s'"), argv[oind]);

      inf = strtoul (argv[oind] + 1, &end, 0);

      if (*end != '\0')
	error (_("invalid syntax of group id '%s'"), argv[oind]);
      VEC_safe_push (int, ids, inf);
    }
  if (VEC_length (int, ids) > 1)
    qsort (VEC_address (int, ids),
	   VEC_length (int, ids),
	   sizeof (int), compare_positive_ints);

  back_to = make_cleanup (free_vector_of_ints, &ids);

  if (available)
    {
      list_available_thread_groups (ids, recurse);
    }
  else if (VEC_length (int, ids) == 1)
    {
      /* Local thread groups, single id.  */
      int id = *VEC_address (int, ids);
      struct inferior *inf = find_inferior_id (id);

      if (!inf)
	error (_("Non-existent thread group id '%d'"), id);

      print_thread_info (uiout, NULL, inf->pid);
    }
  else
    {
      struct print_one_inferior_data data;

      data.recurse = recurse;
      data.inferiors = ids;

      /* Local thread groups.  Either no explicit ids -- and we
	 print everything, or several explicit ids.  In both cases,
	 we print more than one group, and have to use 'groups'
	 as the top-level element.  */
      make_cleanup_ui_out_list_begin_end (uiout, "groups");
      update_thread_list ();
      iterate_over_inferiors (print_one_inferior, &data);
    }

  do_cleanups (back_to);
}

void
mi_cmd_data_list_register_names (const char *command, char **argv, int argc)
{
  struct gdbarch *gdbarch;
  struct ui_out *uiout = current_uiout;
  int regnum, numregs;
  int i;
  struct cleanup *cleanup;

  /* Note that the test for a valid register must include checking the
     gdbarch_register_name because gdbarch_num_regs may be allocated
     for the union of the register sets within a family of related
     processors.  In this case, some entries of gdbarch_register_name
     will change depending upon the particular processor being
     debugged.  */

  gdbarch = get_current_arch ();
  numregs = gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);

  cleanup = make_cleanup_ui_out_list_begin_end (uiout, "register-names");

  if (argc == 0)		/* No args, just do all the regs.  */
    {
      for (regnum = 0;
	   regnum < numregs;
	   regnum++)
	{
	  if (gdbarch_register_name (gdbarch, regnum) == NULL
	      || *(gdbarch_register_name (gdbarch, regnum)) == '\0')
	    uiout->field_string (NULL, "");
	  else
	    uiout->field_string (NULL, gdbarch_register_name (gdbarch, regnum));
	}
    }

  /* Else, list of register #s, just do listed regs.  */
  for (i = 0; i < argc; i++)
    {
      regnum = atoi (argv[i]);
      if (regnum < 0 || regnum >= numregs)
	error (_("bad register number"));

      if (gdbarch_register_name (gdbarch, regnum) == NULL
	  || *(gdbarch_register_name (gdbarch, regnum)) == '\0')
	uiout->field_string (NULL, "");
      else
	uiout->field_string (NULL, gdbarch_register_name (gdbarch, regnum));
    }
  do_cleanups (cleanup);
}

void
mi_cmd_data_list_changed_registers (const char *command, char **argv, int argc)
{
  static struct regcache *this_regs = NULL;
  struct ui_out *uiout = current_uiout;
  struct regcache *prev_regs;
  struct gdbarch *gdbarch;
  int regnum, numregs, changed;
  int i;
  struct cleanup *cleanup;

  /* The last time we visited this function, the current frame's
     register contents were saved in THIS_REGS.  Move THIS_REGS over
     to PREV_REGS, and refresh THIS_REGS with the now-current register
     contents.  */

  prev_regs = this_regs;
  this_regs = frame_save_as_regcache (get_selected_frame (NULL));
  cleanup = make_cleanup_regcache_xfree (prev_regs);

  /* Note that the test for a valid register must include checking the
     gdbarch_register_name because gdbarch_num_regs may be allocated
     for the union of the register sets within a family of related
     processors.  In this case, some entries of gdbarch_register_name
     will change depending upon the particular processor being
     debugged.  */

  gdbarch = get_regcache_arch (this_regs);
  numregs = gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);

  make_cleanup_ui_out_list_begin_end (uiout, "changed-registers");

  if (argc == 0)
    {
      /* No args, just do all the regs.  */
      for (regnum = 0;
	   regnum < numregs;
	   regnum++)
	{
	  if (gdbarch_register_name (gdbarch, regnum) == NULL
	      || *(gdbarch_register_name (gdbarch, regnum)) == '\0')
	    continue;
	  changed = register_changed_p (regnum, prev_regs, this_regs);
	  if (changed < 0)
	    error (_("-data-list-changed-registers: "
		     "Unable to read register contents."));
	  else if (changed)
	    uiout->field_int (NULL, regnum);
	}
    }

  /* Else, list of register #s, just do listed regs.  */
  for (i = 0; i < argc; i++)
    {
      regnum = atoi (argv[i]);

      if (regnum >= 0
	  && regnum < numregs
	  && gdbarch_register_name (gdbarch, regnum) != NULL
	  && *gdbarch_register_name (gdbarch, regnum) != '\000')
	{
	  changed = register_changed_p (regnum, prev_regs, this_regs);
	  if (changed < 0)
	    error (_("-data-list-changed-registers: "
		     "Unable to read register contents."));
	  else if (changed)
	    uiout->field_int (NULL, regnum);
	}
      else
	error (_("bad register number"));
    }
  do_cleanups (cleanup);
}

static int
register_changed_p (int regnum, struct regcache *prev_regs,
		    struct regcache *this_regs)
{
  struct gdbarch *gdbarch = get_regcache_arch (this_regs);
  gdb_byte prev_buffer[MAX_REGISTER_SIZE];
  gdb_byte this_buffer[MAX_REGISTER_SIZE];
  enum register_status prev_status;
  enum register_status this_status;

  /* First time through or after gdbarch change consider all registers
     as changed.  */
  if (!prev_regs || get_regcache_arch (prev_regs) != gdbarch)
    return 1;

  /* Get register contents and compare.  */
  prev_status = regcache_cooked_read (prev_regs, regnum, prev_buffer);
  this_status = regcache_cooked_read (this_regs, regnum, this_buffer);

  if (this_status != prev_status)
    return 1;
  else if (this_status == REG_VALID)
    return memcmp (prev_buffer, this_buffer,
		   register_size (gdbarch, regnum)) != 0;
  else
    return 0;
}

/* Return a list of register number and value pairs.  The valid
   arguments expected are: a letter indicating the format in which to
   display the registers contents.  This can be one of: x
   (hexadecimal), d (decimal), N (natural), t (binary), o (octal), r
   (raw).  After the format argument there can be a sequence of
   numbers, indicating which registers to fetch the content of.  If
   the format is the only argument, a list of all the registers with
   their values is returned.  */

void
mi_cmd_data_list_register_values (const char *command, char **argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  struct frame_info *frame;
  struct gdbarch *gdbarch;
  int regnum, numregs, format;
  int i;
  struct cleanup *list_cleanup;
  int skip_unavailable = 0;
  int oind = 0;
  enum opt
  {
    SKIP_UNAVAILABLE,
  };
  static const struct mi_opt opts[] =
    {
      {"-skip-unavailable", SKIP_UNAVAILABLE, 0},
      { 0, 0, 0 }
    };

  /* Note that the test for a valid register must include checking the
     gdbarch_register_name because gdbarch_num_regs may be allocated
     for the union of the register sets within a family of related
     processors.  In this case, some entries of gdbarch_register_name
     will change depending upon the particular processor being
     debugged.  */

  while (1)
    {
      char *oarg;
      int opt = mi_getopt ("-data-list-register-values", argc, argv,
			   opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case SKIP_UNAVAILABLE:
	  skip_unavailable = 1;
	  break;
	}
    }

  if (argc - oind < 1)
    error (_("-data-list-register-values: Usage: "
	     "-data-list-register-values [--skip-unavailable] <format>"
	     " [<regnum1>...<regnumN>]"));

  format = (int) argv[oind][0];

  frame = get_selected_frame (NULL);
  gdbarch = get_frame_arch (frame);
  numregs = gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);

  list_cleanup = make_cleanup_ui_out_list_begin_end (uiout, "register-values");

  if (argc - oind == 1)
    {
      /* No args, beside the format: do all the regs.  */
      for (regnum = 0;
	   regnum < numregs;
	   regnum++)
	{
	  if (gdbarch_register_name (gdbarch, regnum) == NULL
	      || *(gdbarch_register_name (gdbarch, regnum)) == '\0')
	    continue;

	  output_register (frame, regnum, format, skip_unavailable);
	}
    }

  /* Else, list of register #s, just do listed regs.  */
  for (i = 1 + oind; i < argc; i++)
    {
      regnum = atoi (argv[i]);

      if (regnum >= 0
	  && regnum < numregs
	  && gdbarch_register_name (gdbarch, regnum) != NULL
	  && *gdbarch_register_name (gdbarch, regnum) != '\000')
	output_register (frame, regnum, format, skip_unavailable);
      else
	error (_("bad register number"));
    }
  do_cleanups (list_cleanup);
}

/* Output one register REGNUM's contents in the desired FORMAT.  If
   SKIP_UNAVAILABLE is true, skip the register if it is
   unavailable.  */

static void
output_register (struct frame_info *frame, int regnum, int format,
		 int skip_unavailable)
{
  struct ui_out *uiout = current_uiout;
  struct value *val = value_of_register (regnum, frame);
  struct cleanup *tuple_cleanup;
  struct value_print_options opts;

  if (skip_unavailable && !value_entirely_available (val))
    return;

  tuple_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
  uiout->field_int ("number", regnum);

  if (format == 'N')
    format = 0;

  if (format == 'r')
    format = 'z';

  string_file stb;

  get_formatted_print_options (&opts, format);
  opts.deref_ref = 1;
  val_print (value_type (val),
	     value_embedded_offset (val), 0,
	     &stb, 0, val, &opts, current_language);
  uiout->field_stream ("value", stb);

  do_cleanups (tuple_cleanup);
}

/* Write given values into registers. The registers and values are
   given as pairs.  The corresponding MI command is
   -data-write-register-values <format>
                               [<regnum1> <value1>...<regnumN> <valueN>] */
void
mi_cmd_data_write_register_values (const char *command, char **argv, int argc)
{
  struct regcache *regcache;
  struct gdbarch *gdbarch;
  int numregs, i;

  /* Note that the test for a valid register must include checking the
     gdbarch_register_name because gdbarch_num_regs may be allocated
     for the union of the register sets within a family of related
     processors.  In this case, some entries of gdbarch_register_name
     will change depending upon the particular processor being
     debugged.  */

  regcache = get_current_regcache ();
  gdbarch = get_regcache_arch (regcache);
  numregs = gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);

  if (argc == 0)
    error (_("-data-write-register-values: Usage: -data-write-register-"
	     "values <format> [<regnum1> <value1>...<regnumN> <valueN>]"));

  if (!target_has_registers)
    error (_("-data-write-register-values: No registers."));

  if (!(argc - 1))
    error (_("-data-write-register-values: No regs and values specified."));

  if ((argc - 1) % 2)
    error (_("-data-write-register-values: "
	     "Regs and vals are not in pairs."));

  for (i = 1; i < argc; i = i + 2)
    {
      int regnum = atoi (argv[i]);

      if (regnum >= 0 && regnum < numregs
	  && gdbarch_register_name (gdbarch, regnum)
	  && *gdbarch_register_name (gdbarch, regnum))
	{
	  LONGEST value;

	  /* Get the value as a number.  */
	  value = parse_and_eval_address (argv[i + 1]);

	  /* Write it down.  */
	  regcache_cooked_write_signed (regcache, regnum, value);
	}
      else
	error (_("bad register number"));
    }
}

/* Evaluate the value of the argument.  The argument is an
   expression. If the expression contains spaces it needs to be
   included in double quotes.  */

void
mi_cmd_data_evaluate_expression (const char *command, char **argv, int argc)
{
  struct value *val;
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  if (argc != 1)
    error (_("-data-evaluate-expression: "
	     "Usage: -data-evaluate-expression expression"));

  expression_up expr = parse_expression (argv[0]);

  val = evaluate_expression (expr.get ());

  string_file stb;

  /* Print the result of the expression evaluation.  */
  get_user_print_options (&opts);
  opts.deref_ref = 0;
  common_val_print (val, &stb, 0, &opts, current_language);

  uiout->field_stream ("value", stb);
}

/* This is the -data-read-memory command.

   ADDR: start address of data to be dumped.
   WORD-FORMAT: a char indicating format for the ``word''.  See
   the ``x'' command.
   WORD-SIZE: size of each ``word''; 1,2,4, or 8 bytes.
   NR_ROW: Number of rows.
   NR_COL: The number of colums (words per row).
   ASCHAR: (OPTIONAL) Append an ascii character dump to each row.  Use
   ASCHAR for unprintable characters.

   Reads SIZE*NR_ROW*NR_COL bytes starting at ADDR from memory and
   displayes them.  Returns:

   {addr="...",rowN={wordN="..." ,... [,ascii="..."]}, ...}

   Returns:
   The number of bytes read is SIZE*ROW*COL.  */

void
mi_cmd_data_read_memory (const char *command, char **argv, int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct ui_out *uiout = current_uiout;
  CORE_ADDR addr;
  long total_bytes, nr_cols, nr_rows;
  char word_format;
  struct type *word_type;
  long word_size;
  char word_asize;
  char aschar;
  int nr_bytes;
  long offset = 0;
  int oind = 0;
  char *oarg;
  enum opt
  {
    OFFSET_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"o", OFFSET_OPT, 1},
      { 0, 0, 0 }
    };

  while (1)
    {
      int opt = mi_getopt ("-data-read-memory", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case OFFSET_OPT:
	  offset = atol (oarg);
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  if (argc < 5 || argc > 6)
    error (_("-data-read-memory: Usage: "
	     "ADDR WORD-FORMAT WORD-SIZE NR-ROWS NR-COLS [ASCHAR]."));

  /* Extract all the arguments. */

  /* Start address of the memory dump.  */
  addr = parse_and_eval_address (argv[0]) + offset;
  /* The format character to use when displaying a memory word.  See
     the ``x'' command.  */
  word_format = argv[1][0];
  /* The size of the memory word.  */
  word_size = atol (argv[2]);
  switch (word_size)
    {
    case 1:
      word_type = builtin_type (gdbarch)->builtin_int8;
      word_asize = 'b';
      break;
    case 2:
      word_type = builtin_type (gdbarch)->builtin_int16;
      word_asize = 'h';
      break;
    case 4:
      word_type = builtin_type (gdbarch)->builtin_int32;
      word_asize = 'w';
      break;
    case 8:
      word_type = builtin_type (gdbarch)->builtin_int64;
      word_asize = 'g';
      break;
    default:
      word_type = builtin_type (gdbarch)->builtin_int8;
      word_asize = 'b';
    }
  /* The number of rows.  */
  nr_rows = atol (argv[3]);
  if (nr_rows <= 0)
    error (_("-data-read-memory: invalid number of rows."));

  /* Number of bytes per row.  */
  nr_cols = atol (argv[4]);
  if (nr_cols <= 0)
    error (_("-data-read-memory: invalid number of columns."));

  /* The un-printable character when printing ascii.  */
  if (argc == 6)
    aschar = *argv[5];
  else
    aschar = 0;

  /* Create a buffer and read it in.  */
  total_bytes = word_size * nr_rows * nr_cols;

  std::unique_ptr<gdb_byte[]> mbuf (new gdb_byte[total_bytes]);

  /* Dispatch memory reads to the topmost target, not the flattened
     current_target.  */
  nr_bytes = target_read (current_target.beneath,
			  TARGET_OBJECT_MEMORY, NULL, mbuf.get (),
			  addr, total_bytes);
  if (nr_bytes <= 0)
    error (_("Unable to read memory."));

  /* Output the header information.  */
  uiout->field_core_addr ("addr", gdbarch, addr);
  uiout->field_int ("nr-bytes", nr_bytes);
  uiout->field_int ("total-bytes", total_bytes);
  uiout->field_core_addr ("next-row", gdbarch, addr + word_size * nr_cols);
  uiout->field_core_addr ("prev-row", gdbarch, addr - word_size * nr_cols);
  uiout->field_core_addr ("next-page", gdbarch, addr + total_bytes);
  uiout->field_core_addr ("prev-page", gdbarch, addr - total_bytes);

  /* Build the result as a two dimentional table.  */
  {
    int row;
    int row_byte;
    struct cleanup *cleanup_list;

    string_file stream;

    cleanup_list = make_cleanup_ui_out_list_begin_end (uiout, "memory");
    for (row = 0, row_byte = 0;
	 row < nr_rows;
	 row++, row_byte += nr_cols * word_size)
      {
	int col;
	int col_byte;
	struct cleanup *cleanup_tuple;
	struct cleanup *cleanup_list_data;
	struct value_print_options opts;

	cleanup_tuple = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
	uiout->field_core_addr ("addr", gdbarch, addr + row_byte);
	/* ui_out_field_core_addr_symbolic (uiout, "saddr", addr +
	   row_byte); */
	cleanup_list_data = make_cleanup_ui_out_list_begin_end (uiout, "data");
	get_formatted_print_options (&opts, word_format);
	for (col = 0, col_byte = row_byte;
	     col < nr_cols;
	     col++, col_byte += word_size)
	  {
	    if (col_byte + word_size > nr_bytes)
	      {
		uiout->field_string (NULL, "N/A");
	      }
	    else
	      {
		stream.clear ();
		print_scalar_formatted (&mbuf[col_byte], word_type, &opts,
					word_asize, &stream);
		uiout->field_stream (NULL, stream);
	      }
	  }
	do_cleanups (cleanup_list_data);
	if (aschar)
	  {
	    int byte;

	    stream.clear ();
	    for (byte = row_byte;
		 byte < row_byte + word_size * nr_cols; byte++)
	      {
		if (byte >= nr_bytes)
		  stream.putc ('X');
		else if (mbuf[byte] < 32 || mbuf[byte] > 126)
		  stream.putc (aschar);
		else
		  stream.putc (mbuf[byte]);
	      }
	    uiout->field_stream ("ascii", stream);
	  }
	do_cleanups (cleanup_tuple);
      }
    do_cleanups (cleanup_list);
  }
}

void
mi_cmd_data_read_memory_bytes (const char *command, char **argv, int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct ui_out *uiout = current_uiout;
  struct cleanup *cleanups;
  CORE_ADDR addr;
  LONGEST length;
  memory_read_result_s *read_result;
  int ix;
  VEC(memory_read_result_s) *result;
  long offset = 0;
  int unit_size = gdbarch_addressable_memory_unit_size (gdbarch);
  int oind = 0;
  char *oarg;
  enum opt
  {
    OFFSET_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"o", OFFSET_OPT, 1},
      { 0, 0, 0 }
    };

  while (1)
    {
      int opt = mi_getopt ("-data-read-memory-bytes", argc, argv, opts,
			   &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case OFFSET_OPT:
	  offset = atol (oarg);
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  if (argc != 2)
    error (_("Usage: [ -o OFFSET ] ADDR LENGTH."));

  addr = parse_and_eval_address (argv[0]) + offset;
  length = atol (argv[1]);

  result = read_memory_robust (current_target.beneath, addr, length);

  cleanups = make_cleanup (free_memory_read_result_vector, &result);

  if (VEC_length (memory_read_result_s, result) == 0)
    error (_("Unable to read memory."));

  make_cleanup_ui_out_list_begin_end (uiout, "memory");
  for (ix = 0;
       VEC_iterate (memory_read_result_s, result, ix, read_result);
       ++ix)
    {
      struct cleanup *t = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
      char *data, *p;
      int i;
      int alloc_len;

      uiout->field_core_addr ("begin", gdbarch, read_result->begin);
      uiout->field_core_addr ("offset", gdbarch, read_result->begin - addr);
      uiout->field_core_addr ("end", gdbarch, read_result->end);

      alloc_len = (read_result->end - read_result->begin) * 2 * unit_size + 1;
      data = (char *) xmalloc (alloc_len);

      for (i = 0, p = data;
	   i < ((read_result->end - read_result->begin) * unit_size);
	   ++i, p += 2)
	{
	  sprintf (p, "%02x", read_result->data[i]);
	}
      uiout->field_string ("contents", data);
      xfree (data);
      do_cleanups (t);
    }
  do_cleanups (cleanups);
}

/* Implementation of the -data-write_memory command.

   COLUMN_OFFSET: optional argument. Must be preceded by '-o'. The
   offset from the beginning of the memory grid row where the cell to
   be written is.
   ADDR: start address of the row in the memory grid where the memory
   cell is, if OFFSET_COLUMN is specified.  Otherwise, the address of
   the location to write to.
   FORMAT: a char indicating format for the ``word''.  See
   the ``x'' command.
   WORD_SIZE: size of each ``word''; 1,2,4, or 8 bytes
   VALUE: value to be written into the memory address.

   Writes VALUE into ADDR + (COLUMN_OFFSET * WORD_SIZE).

   Prints nothing.  */

void
mi_cmd_data_write_memory (const char *command, char **argv, int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR addr;
  long word_size;
  /* FIXME: ezannoni 2000-02-17 LONGEST could possibly not be big
     enough when using a compiler other than GCC.  */
  LONGEST value;
  gdb_byte *buffer;
  struct cleanup *old_chain;
  long offset = 0;
  int oind = 0;
  char *oarg;
  enum opt
  {
    OFFSET_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"o", OFFSET_OPT, 1},
      { 0, 0, 0 }
    };

  while (1)
    {
      int opt = mi_getopt ("-data-write-memory", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case OFFSET_OPT:
	  offset = atol (oarg);
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  if (argc != 4)
    error (_("-data-write-memory: Usage: "
	     "[-o COLUMN_OFFSET] ADDR FORMAT WORD-SIZE VALUE."));

  /* Extract all the arguments.  */
  /* Start address of the memory dump.  */
  addr = parse_and_eval_address (argv[0]);
  /* The size of the memory word.  */
  word_size = atol (argv[2]);

  /* Calculate the real address of the write destination.  */
  addr += (offset * word_size);

  /* Get the value as a number.  */
  value = parse_and_eval_address (argv[3]);
  /* Get the value into an array.  */
  buffer = (gdb_byte *) xmalloc (word_size);
  old_chain = make_cleanup (xfree, buffer);
  store_signed_integer (buffer, word_size, byte_order, value);
  /* Write it down to memory.  */
  write_memory_with_notification (addr, buffer, word_size);
  /* Free the buffer.  */
  do_cleanups (old_chain);
}

/* Implementation of the -data-write-memory-bytes command.

   ADDR: start address
   DATA: string of bytes to write at that address
   COUNT: number of bytes to be filled (decimal integer).  */

void
mi_cmd_data_write_memory_bytes (const char *command, char **argv, int argc)
{
  CORE_ADDR addr;
  char *cdata;
  gdb_byte *data;
  gdb_byte *databuf;
  size_t len_hex, len_bytes, len_units, i, steps, remaining_units;
  long int count_units;
  struct cleanup *back_to;
  int unit_size;

  if (argc != 2 && argc != 3)
    error (_("Usage: ADDR DATA [COUNT]."));

  addr = parse_and_eval_address (argv[0]);
  cdata = argv[1];
  len_hex = strlen (cdata);
  unit_size = gdbarch_addressable_memory_unit_size (get_current_arch ());

  if (len_hex % (unit_size * 2) != 0)
    error (_("Hex-encoded '%s' must represent an integral number of "
	     "addressable memory units."),
	   cdata);

  len_bytes = len_hex / 2;
  len_units = len_bytes / unit_size;

  if (argc == 3)
    count_units = strtoul (argv[2], NULL, 10);
  else
    count_units = len_units;

  databuf = XNEWVEC (gdb_byte, len_bytes);
  back_to = make_cleanup (xfree, databuf);

  for (i = 0; i < len_bytes; ++i)
    {
      int x;
      if (sscanf (cdata + i * 2, "%02x", &x) != 1)
        error (_("Invalid argument"));
      databuf[i] = (gdb_byte) x;
    }

  if (len_units < count_units)
    {
      /* Pattern is made of less units than count:
         repeat pattern to fill memory.  */
      data = (gdb_byte *) xmalloc (count_units * unit_size);
      make_cleanup (xfree, data);

      /* Number of times the pattern is entirely repeated.  */
      steps = count_units / len_units;
      /* Number of remaining addressable memory units.  */
      remaining_units = count_units % len_units;
      for (i = 0; i < steps; i++)
        memcpy (data + i * len_bytes, databuf, len_bytes);

      if (remaining_units > 0)
        memcpy (data + steps * len_bytes, databuf,
		remaining_units * unit_size);
    }
  else
    {
      /* Pattern is longer than or equal to count:
         just copy count addressable memory units.  */
      data = databuf;
    }

  write_memory_with_notification (addr, data, count_units);

  do_cleanups (back_to);
}

void
mi_cmd_enable_timings (const char *command, char **argv, int argc)
{
  if (argc == 0)
    do_timings = 1;
  else if (argc == 1)
    {
      if (strcmp (argv[0], "yes") == 0)
	do_timings = 1;
      else if (strcmp (argv[0], "no") == 0)
	do_timings = 0;
      else
	goto usage_error;
    }
  else
    goto usage_error;

  return;

 usage_error:
  error (_("-enable-timings: Usage: %s {yes|no}"), command);
}

void
mi_cmd_list_features (const char *command, char **argv, int argc)
{
  if (argc == 0)
    {
      struct cleanup *cleanup = NULL;
      struct ui_out *uiout = current_uiout;

      cleanup = make_cleanup_ui_out_list_begin_end (uiout, "features");
      uiout->field_string (NULL, "frozen-varobjs");
      uiout->field_string (NULL, "pending-breakpoints");
      uiout->field_string (NULL, "thread-info");
      uiout->field_string (NULL, "data-read-memory-bytes");
      uiout->field_string (NULL, "breakpoint-notifications");
      uiout->field_string (NULL, "ada-task-info");
      uiout->field_string (NULL, "language-option");
      uiout->field_string (NULL, "info-gdb-mi-command");
      uiout->field_string (NULL, "undefined-command-error-code");
      uiout->field_string (NULL, "exec-run-start-option");

      if (ext_lang_initialized_p (get_ext_lang_defn (EXT_LANG_PYTHON)))
	uiout->field_string (NULL, "python");

      do_cleanups (cleanup);
      return;
    }

  error (_("-list-features should be passed no arguments"));
}

void
mi_cmd_list_target_features (const char *command, char **argv, int argc)
{
  if (argc == 0)
    {
      struct cleanup *cleanup = NULL;
      struct ui_out *uiout = current_uiout;

      cleanup = make_cleanup_ui_out_list_begin_end (uiout, "features");
      if (mi_async_p ())
	uiout->field_string (NULL, "async");
      if (target_can_execute_reverse)
	uiout->field_string (NULL, "reverse");
      do_cleanups (cleanup);
      return;
    }

  error (_("-list-target-features should be passed no arguments"));
}

void
mi_cmd_add_inferior (const char *command, char **argv, int argc)
{
  struct inferior *inf;

  if (argc != 0)
    error (_("-add-inferior should be passed no arguments"));

  inf = add_inferior_with_spaces ();

  current_uiout->field_fmt ("inferior", "i%d", inf->num);
}

/* Callback used to find the first inferior other than the current
   one.  */

static int
get_other_inferior (struct inferior *inf, void *arg)
{
  if (inf == current_inferior ())
    return 0;

  return 1;
}

void
mi_cmd_remove_inferior (const char *command, char **argv, int argc)
{
  int id;
  struct inferior *inf;

  if (argc != 1)
    error (_("-remove-inferior should be passed a single argument"));

  if (sscanf (argv[0], "i%d", &id) != 1)
    error (_("the thread group id is syntactically invalid"));

  inf = find_inferior_id (id);
  if (!inf)
    error (_("the specified thread group does not exist"));

  if (inf->pid != 0)
    error (_("cannot remove an active inferior"));

  if (inf == current_inferior ())
    {
      struct thread_info *tp = 0;
      struct inferior *new_inferior
	= iterate_over_inferiors (get_other_inferior, NULL);

      if (new_inferior == NULL)
	error (_("Cannot remove last inferior"));

      set_current_inferior (new_inferior);
      if (new_inferior->pid != 0)
	tp = any_thread_of_process (new_inferior->pid);
      switch_to_thread (tp ? tp->ptid : null_ptid);
      set_current_program_space (new_inferior->pspace);
    }

  delete_inferior (inf);
}



/* Execute a command within a safe environment.
   Return <0 for error; >=0 for ok.

   args->action will tell mi_execute_command what action
   to perfrom after the given command has executed (display/suppress
   prompt, display error).  */

static void
captured_mi_execute_command (struct ui_out *uiout, struct mi_parse *context)
{
  struct mi_interp *mi = (struct mi_interp *) command_interp ();
  struct cleanup *cleanup;

  if (do_timings)
    current_command_ts = context->cmd_start;

  current_token = xstrdup (context->token);
  cleanup = make_cleanup (free_current_contents, &current_token);

  running_result_record_printed = 0;
  mi_proceeded = 0;
  switch (context->op)
    {
    case MI_COMMAND:
      /* A MI command was read from the input stream.  */
      if (mi_debug_p)
	/* FIXME: gdb_???? */
	fprintf_unfiltered (mi->raw_stdout,
			    " token=`%s' command=`%s' args=`%s'\n",
			    context->token, context->command, context->args);

      mi_cmd_execute (context);

      /* Print the result if there were no errors.

	 Remember that on the way out of executing a command, you have
	 to directly use the mi_interp's uiout, since the command
	 could have reset the interpreter, in which case the current
	 uiout will most likely crash in the mi_out_* routines.  */
      if (!running_result_record_printed)
	{
	  fputs_unfiltered (context->token, mi->raw_stdout);
	  /* There's no particularly good reason why target-connect results
	     in not ^done.  Should kill ^connected for MI3.  */
	  fputs_unfiltered (strcmp (context->command, "target-select") == 0
			    ? "^connected" : "^done", mi->raw_stdout);
	  mi_out_put (uiout, mi->raw_stdout);
	  mi_out_rewind (uiout);
	  mi_print_timing_maybe (mi->raw_stdout);
	  fputs_unfiltered ("\n", mi->raw_stdout);
	}
      else
	/* The command does not want anything to be printed.  In that
	   case, the command probably should not have written anything
	   to uiout, but in case it has written something, discard it.  */
	mi_out_rewind (uiout);
      break;

    case CLI_COMMAND:
      {
	char *argv[2];

	/* A CLI command was read from the input stream.  */
	/* This "feature" will be removed as soon as we have a
	   complete set of mi commands.  */
	/* Echo the command on the console.  */
	fprintf_unfiltered (gdb_stdlog, "%s\n", context->command);
	/* Call the "console" interpreter.  */
	argv[0] = (char *) INTERP_CONSOLE;
	argv[1] = context->command;
	mi_cmd_interpreter_exec ("-interpreter-exec", argv, 2);

	/* If we changed interpreters, DON'T print out anything.  */
	if (current_interp_named_p (INTERP_MI)
	    || current_interp_named_p (INTERP_MI1)
	    || current_interp_named_p (INTERP_MI2)
	    || current_interp_named_p (INTERP_MI3))
	  {
	    if (!running_result_record_printed)
	      {
		fputs_unfiltered (context->token, mi->raw_stdout);
		fputs_unfiltered ("^done", mi->raw_stdout);
		mi_out_put (uiout, mi->raw_stdout);
		mi_out_rewind (uiout);
		mi_print_timing_maybe (mi->raw_stdout);
		fputs_unfiltered ("\n", mi->raw_stdout);
	      }
	    else
	      mi_out_rewind (uiout);
	  }
	break;
      }
    }

  do_cleanups (cleanup);
}

/* Print a gdb exception to the MI output stream.  */

static void
mi_print_exception (const char *token, struct gdb_exception exception)
{
  struct mi_interp *mi = (struct mi_interp *) current_interpreter ();

  fputs_unfiltered (token, mi->raw_stdout);
  fputs_unfiltered ("^error,msg=\"", mi->raw_stdout);
  if (exception.message == NULL)
    fputs_unfiltered ("unknown error", mi->raw_stdout);
  else
    fputstr_unfiltered (exception.message, '"', mi->raw_stdout);
  fputs_unfiltered ("\"", mi->raw_stdout);

  switch (exception.error)
    {
      case UNDEFINED_COMMAND_ERROR:
	fputs_unfiltered (",code=\"undefined-command\"", mi->raw_stdout);
	break;
    }

  fputs_unfiltered ("\n", mi->raw_stdout);
}

/* Determine whether the parsed command already notifies the
   user_selected_context_changed observer.  */

static int
command_notifies_uscc_observer (struct mi_parse *command)
{
  if (command->op == CLI_COMMAND)
    {
      /* CLI commands "thread" and "inferior" already send it.  */
      return (strncmp (command->command, "thread ", 7) == 0
	      || strncmp (command->command, "inferior ", 9) == 0);
    }
  else /* MI_COMMAND */
    {
      if (strcmp (command->command, "interpreter-exec") == 0
	  && command->argc > 1)
	{
	  /* "thread" and "inferior" again, but through -interpreter-exec.  */
	  return (strncmp (command->argv[1], "thread ", 7) == 0
		  || strncmp (command->argv[1], "inferior ", 9) == 0);
	}

      else
	/* -thread-select already sends it.  */
	return strcmp (command->command, "thread-select") == 0;
    }
}

void
mi_execute_command (const char *cmd, int from_tty)
{
  char *token;
  std::unique_ptr<struct mi_parse> command;

  /* This is to handle EOF (^D). We just quit gdb.  */
  /* FIXME: we should call some API function here.  */
  if (cmd == 0)
    quit_force (NULL, from_tty);

  target_log_command (cmd);

  TRY
    {
      command = mi_parse (cmd, &token);
    }
  CATCH (exception, RETURN_MASK_ALL)
    {
      mi_print_exception (token, exception);
      xfree (token);
    }
  END_CATCH

  if (command != NULL)
    {
      ptid_t previous_ptid = inferior_ptid;

      gdb::optional<scoped_restore_tmpl<int>> restore_suppress;

      if (command->cmd != NULL && command->cmd->suppress_notification != NULL)
	restore_suppress.emplace (command->cmd->suppress_notification, 1);

      command->token = token;

      if (do_timings)
	{
	  command->cmd_start = new mi_timestamp ();
	  timestamp (command->cmd_start);
	}

      TRY
	{
	  captured_mi_execute_command (current_uiout, command.get ());
	}
      CATCH (result, RETURN_MASK_ALL)
	{
	  /* Like in start_event_loop, enable input and force display
	     of the prompt.  Otherwise, any command that calls
	     async_disable_stdin, and then throws, will leave input
	     disabled.  */
	  async_enable_stdin ();
	  current_ui->prompt_state = PROMPT_NEEDED;

	  /* The command execution failed and error() was called
	     somewhere.  */
	  mi_print_exception (command->token, result);
	  mi_out_rewind (current_uiout);
	}
      END_CATCH

      bpstat_do_actions ();

      if (/* The notifications are only output when the top-level
	     interpreter (specified on the command line) is MI.  */
	  interp_ui_out (top_level_interpreter ())->is_mi_like_p ()
	  /* Don't try report anything if there are no threads --
	     the program is dead.  */
	  && thread_count () != 0
	  /* If the command already reports the thread change, no need to do it
	     again.  */
	  && !command_notifies_uscc_observer (command.get ()))
	{
	  struct mi_interp *mi = (struct mi_interp *) top_level_interpreter ();
	  int report_change = 0;

	  if (command->thread == -1)
	    {
	      report_change = (!ptid_equal (previous_ptid, null_ptid)
			       && !ptid_equal (inferior_ptid, previous_ptid)
			       && !ptid_equal (inferior_ptid, null_ptid));
	    }
	  else if (!ptid_equal (inferior_ptid, null_ptid))
	    {
	      struct thread_info *ti = inferior_thread ();

	      report_change = (ti->global_num != command->thread);
	    }

	  if (report_change)
	    {
		observer_notify_user_selected_context_changed
		  (USER_SELECTED_THREAD | USER_SELECTED_FRAME);
	    }
	}
    }
}

static void
mi_cmd_execute (struct mi_parse *parse)
{
  struct cleanup *cleanup;

  cleanup = prepare_execute_command ();

  if (parse->all && parse->thread_group != -1)
    error (_("Cannot specify --thread-group together with --all"));

  if (parse->all && parse->thread != -1)
    error (_("Cannot specify --thread together with --all"));

  if (parse->thread_group != -1 && parse->thread != -1)
    error (_("Cannot specify --thread together with --thread-group"));

  if (parse->frame != -1 && parse->thread == -1)
    error (_("Cannot specify --frame without --thread"));

  if (parse->thread_group != -1)
    {
      struct inferior *inf = find_inferior_id (parse->thread_group);
      struct thread_info *tp = 0;

      if (!inf)
	error (_("Invalid thread group for the --thread-group option"));

      set_current_inferior (inf);
      /* This behaviour means that if --thread-group option identifies
	 an inferior with multiple threads, then a random one will be
	 picked.  This is not a problem -- frontend should always
	 provide --thread if it wishes to operate on a specific
	 thread.  */
      if (inf->pid != 0)
	tp = any_live_thread_of_process (inf->pid);
      switch_to_thread (tp ? tp->ptid : null_ptid);
      set_current_program_space (inf->pspace);
    }

  if (parse->thread != -1)
    {
      struct thread_info *tp = find_thread_global_id (parse->thread);

      if (!tp)
	error (_("Invalid thread id: %d"), parse->thread);

      if (is_exited (tp->ptid))
	error (_("Thread id: %d has terminated"), parse->thread);

      switch_to_thread (tp->ptid);
    }

  if (parse->frame != -1)
    {
      struct frame_info *fid;
      int frame = parse->frame;

      fid = find_relative_frame (get_current_frame (), &frame);
      if (frame == 0)
	/* find_relative_frame was successful */
	select_frame (fid);
      else
	error (_("Invalid frame id: %d"), frame);
    }

  if (parse->language != language_unknown)
    {
      make_cleanup_restore_current_language ();
      set_language (parse->language);
    }

  current_context = parse;

  if (parse->cmd->argv_func != NULL)
    {
      parse->cmd->argv_func (parse->command, parse->argv, parse->argc);
    }
  else if (parse->cmd->cli.cmd != 0)
    {
      /* FIXME: DELETE THIS. */
      /* The operation is still implemented by a cli command.  */
      /* Must be a synchronous one.  */
      mi_execute_cli_command (parse->cmd->cli.cmd, parse->cmd->cli.args_p,
			      parse->args);
    }
  else
    {
      /* FIXME: DELETE THIS.  */
      string_file stb;

      stb.puts ("Undefined mi command: ");
      stb.putstr (parse->command, '"');
      stb.puts (" (missing implementation)");

      error_stream (stb);
    }
  do_cleanups (cleanup);
}

/* FIXME: This is just a hack so we can get some extra commands going.
   We don't want to channel things through the CLI, but call libgdb directly.
   Use only for synchronous commands.  */

void
mi_execute_cli_command (const char *cmd, int args_p, const char *args)
{
  if (cmd != 0)
    {
      struct cleanup *old_cleanups;
      char *run;

      if (args_p)
	run = xstrprintf ("%s %s", cmd, args);
      else
	run = xstrdup (cmd);
      if (mi_debug_p)
	/* FIXME: gdb_???? */
	fprintf_unfiltered (gdb_stdout, "cli=%s run=%s\n",
			    cmd, run);
      old_cleanups = make_cleanup (xfree, run);
      execute_command (run, 0 /* from_tty */ );
      do_cleanups (old_cleanups);
      return;
    }
}

void
mi_execute_async_cli_command (const char *cli_command, char **argv, int argc)
{
  struct cleanup *old_cleanups;
  char *run;

  if (mi_async_p ())
    run = xstrprintf ("%s %s&", cli_command, argc ? *argv : "");
  else
    run = xstrprintf ("%s %s", cli_command, argc ? *argv : "");
  old_cleanups = make_cleanup (xfree, run);

  execute_command (run, 0 /* from_tty */ );

  /* Do this before doing any printing.  It would appear that some
     print code leaves garbage around in the buffer.  */
  do_cleanups (old_cleanups);
}

void
mi_load_progress (const char *section_name,
		  unsigned long sent_so_far,
		  unsigned long total_section,
		  unsigned long total_sent,
		  unsigned long grand_total)
{
  using namespace std::chrono;
  static steady_clock::time_point last_update;
  static char *previous_sect_name = NULL;
  int new_section;
  struct ui_out *saved_uiout;
  struct ui_out *uiout;
  struct mi_interp *mi = (struct mi_interp *) current_interpreter ();

  /* This function is called through deprecated_show_load_progress
     which means uiout may not be correct.  Fix it for the duration
     of this function.  */
  saved_uiout = current_uiout;

  if (current_interp_named_p (INTERP_MI)
      || current_interp_named_p (INTERP_MI2))
    current_uiout = mi_out_new (2);
  else if (current_interp_named_p (INTERP_MI1))
    current_uiout = mi_out_new (1);
  else if (current_interp_named_p (INTERP_MI3))
    current_uiout = mi_out_new (3);
  else
    return;

  uiout = current_uiout;

  new_section = (previous_sect_name ?
		 strcmp (previous_sect_name, section_name) : 1);
  if (new_section)
    {
      struct cleanup *cleanup_tuple;

      xfree (previous_sect_name);
      previous_sect_name = xstrdup (section_name);

      if (current_token)
	fputs_unfiltered (current_token, mi->raw_stdout);
      fputs_unfiltered ("+download", mi->raw_stdout);
      cleanup_tuple = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
      uiout->field_string ("section", section_name);
      uiout->field_int ("section-size", total_section);
      uiout->field_int ("total-size", grand_total);
      do_cleanups (cleanup_tuple);
      mi_out_put (uiout, mi->raw_stdout);
      fputs_unfiltered ("\n", mi->raw_stdout);
      gdb_flush (mi->raw_stdout);
    }

  steady_clock::time_point time_now = steady_clock::now ();
  if (time_now - last_update > milliseconds (500))
    {
      struct cleanup *cleanup_tuple;

      last_update = time_now;
      if (current_token)
	fputs_unfiltered (current_token, mi->raw_stdout);
      fputs_unfiltered ("+download", mi->raw_stdout);
      cleanup_tuple = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
      uiout->field_string ("section", section_name);
      uiout->field_int ("section-sent", sent_so_far);
      uiout->field_int ("section-size", total_section);
      uiout->field_int ("total-sent", total_sent);
      uiout->field_int ("total-size", grand_total);
      do_cleanups (cleanup_tuple);
      mi_out_put (uiout, mi->raw_stdout);
      fputs_unfiltered ("\n", mi->raw_stdout);
      gdb_flush (mi->raw_stdout);
    }

  xfree (uiout);
  current_uiout = saved_uiout;
}

static void
timestamp (struct mi_timestamp *tv)
{
  using namespace std::chrono;

  tv->wallclock = steady_clock::now ();
  run_time_clock::now (tv->utime, tv->stime);
}

static void
print_diff_now (struct ui_file *file, struct mi_timestamp *start)
{
  struct mi_timestamp now;

  timestamp (&now);
  print_diff (file, start, &now);
}

void
mi_print_timing_maybe (struct ui_file *file)
{
  /* If the command is -enable-timing then do_timings may be true
     whilst current_command_ts is not initialized.  */
  if (do_timings && current_command_ts)
    print_diff_now (file, current_command_ts);
}

static void
print_diff (struct ui_file *file, struct mi_timestamp *start,
	    struct mi_timestamp *end)
{
  using namespace std::chrono;

  duration<double> wallclock = end->wallclock - start->wallclock;
  duration<double> utime = end->utime - start->utime;
  duration<double> stime = end->stime - start->stime;

  fprintf_unfiltered
    (file,
     ",time={wallclock=\"%0.5f\",user=\"%0.5f\",system=\"%0.5f\"}",
     wallclock.count (), utime.count (), stime.count ());
}

void
mi_cmd_trace_define_variable (const char *command, char **argv, int argc)
{
  LONGEST initval = 0;
  struct trace_state_variable *tsv;
  char *name = 0;

  if (argc != 1 && argc != 2)
    error (_("Usage: -trace-define-variable VARIABLE [VALUE]"));

  name = argv[0];
  if (*name++ != '$')
    error (_("Name of trace variable should start with '$'"));

  validate_trace_state_variable_name (name);

  tsv = find_trace_state_variable (name);
  if (!tsv)
    tsv = create_trace_state_variable (name);

  if (argc == 2)
    initval = value_as_long (parse_and_eval (argv[1]));

  tsv->initial_value = initval;
}

void
mi_cmd_trace_list_variables (const char *command, char **argv, int argc)
{
  if (argc != 0)
    error (_("-trace-list-variables: no arguments allowed"));

  tvariables_info_1 ();
}

void
mi_cmd_trace_find (const char *command, char **argv, int argc)
{
  char *mode;

  if (argc == 0)
    error (_("trace selection mode is required"));

  mode = argv[0];

  if (strcmp (mode, "none") == 0)
    {
      tfind_1 (tfind_number, -1, 0, 0, 0);
      return;
    }

  check_trace_running (current_trace_status ());

  if (strcmp (mode, "frame-number") == 0)
    {
      if (argc != 2)
	error (_("frame number is required"));
      tfind_1 (tfind_number, atoi (argv[1]), 0, 0, 0);
    }
  else if (strcmp (mode, "tracepoint-number") == 0)
    {
      if (argc != 2)
	error (_("tracepoint number is required"));
      tfind_1 (tfind_tp, atoi (argv[1]), 0, 0, 0);
    }
  else if (strcmp (mode, "pc") == 0)
    {
      if (argc != 2)
	error (_("PC is required"));
      tfind_1 (tfind_pc, 0, parse_and_eval_address (argv[1]), 0, 0);
    }
  else if (strcmp (mode, "pc-inside-range") == 0)
    {
      if (argc != 3)
	error (_("Start and end PC are required"));
      tfind_1 (tfind_range, 0, parse_and_eval_address (argv[1]),
	       parse_and_eval_address (argv[2]), 0);
    }
  else if (strcmp (mode, "pc-outside-range") == 0)
    {
      if (argc != 3)
	error (_("Start and end PC are required"));
      tfind_1 (tfind_outside, 0, parse_and_eval_address (argv[1]),
	       parse_and_eval_address (argv[2]), 0);
    }
  else if (strcmp (mode, "line") == 0)
    {
      struct symtabs_and_lines sals;
      struct symtab_and_line sal;
      static CORE_ADDR start_pc, end_pc;
      struct cleanup *back_to;

      if (argc != 2)
	error (_("Line is required"));

      sals = decode_line_with_current_source (argv[1],
					      DECODE_LINE_FUNFIRSTLINE);
      back_to = make_cleanup (xfree, sals.sals);

      sal = sals.sals[0];

      if (sal.symtab == 0)
	error (_("Could not find the specified line"));

      if (sal.line > 0 && find_line_pc_range (sal, &start_pc, &end_pc))
	tfind_1 (tfind_range, 0, start_pc, end_pc - 1, 0);
      else
	error (_("Could not find the specified line"));

      do_cleanups (back_to);
    }
  else
    error (_("Invalid mode '%s'"), mode);

  if (has_stack_frames () || get_traceframe_number () >= 0)
    print_stack_frame (get_selected_frame (NULL), 1, LOC_AND_ADDRESS, 1);
}

void
mi_cmd_trace_save (const char *command, char **argv, int argc)
{
  int target_saves = 0;
  int generate_ctf = 0;
  char *filename;
  int oind = 0;
  char *oarg;

  enum opt
  {
    TARGET_SAVE_OPT, CTF_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"r", TARGET_SAVE_OPT, 0},
      {"ctf", CTF_OPT, 0},
      { 0, 0, 0 }
    };

  while (1)
    {
      int opt = mi_getopt ("-trace-save", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case TARGET_SAVE_OPT:
	  target_saves = 1;
	  break;
	case CTF_OPT:
	  generate_ctf = 1;
	  break;
	}
    }

  if (argc - oind != 1)
    error (_("Exactly one argument required "
	     "(file in which to save trace data)"));

  filename = argv[oind];

  if (generate_ctf)
    trace_save_ctf (filename, target_saves);
  else
    trace_save_tfile (filename, target_saves);
}

void
mi_cmd_trace_start (const char *command, char **argv, int argc)
{
  start_tracing (NULL);
}

void
mi_cmd_trace_status (const char *command, char **argv, int argc)
{
  trace_status_mi (0);
}

void
mi_cmd_trace_stop (const char *command, char **argv, int argc)
{
  stop_tracing (NULL);
  trace_status_mi (1);
}

/* Implement the "-ada-task-info" command.  */

void
mi_cmd_ada_task_info (const char *command, char **argv, int argc)
{
  if (argc != 0 && argc != 1)
    error (_("Invalid MI command"));

  print_ada_task_info (current_uiout, argv[0], current_inferior ());
}

/* Print EXPRESSION according to VALUES.  */

static void
print_variable_or_computed (const char *expression, enum print_values values)
{
  struct cleanup *old_chain;
  struct value *val;
  struct type *type;
  struct ui_out *uiout = current_uiout;

  string_file stb;

  expression_up expr = parse_expression (expression);

  if (values == PRINT_SIMPLE_VALUES)
    val = evaluate_type (expr.get ());
  else
    val = evaluate_expression (expr.get ());

  old_chain = make_cleanup (null_cleanup, NULL);
  if (values != PRINT_NO_VALUES)
    make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
  uiout->field_string ("name", expression);

  switch (values)
    {
    case PRINT_SIMPLE_VALUES:
      type = check_typedef (value_type (val));
      type_print (value_type (val), "", &stb, -1);
      uiout->field_stream ("type", stb);
      if (TYPE_CODE (type) != TYPE_CODE_ARRAY
	  && TYPE_CODE (type) != TYPE_CODE_STRUCT
	  && TYPE_CODE (type) != TYPE_CODE_UNION)
	{
	  struct value_print_options opts;

	  get_no_prettyformat_print_options (&opts);
	  opts.deref_ref = 1;
	  common_val_print (val, &stb, 0, &opts, current_language);
	  uiout->field_stream ("value", stb);
	}
      break;
    case PRINT_ALL_VALUES:
      {
	struct value_print_options opts;

	get_no_prettyformat_print_options (&opts);
	opts.deref_ref = 1;
	common_val_print (val, &stb, 0, &opts, current_language);
	uiout->field_stream ("value", stb);
      }
      break;
    }

  do_cleanups (old_chain);
}

/* Implement the "-trace-frame-collected" command.  */

void
mi_cmd_trace_frame_collected (const char *command, char **argv, int argc)
{
  struct cleanup *old_chain;
  struct bp_location *tloc;
  int stepping_frame;
  struct collection_list *clist;
  struct collection_list tracepoint_list, stepping_list;
  struct traceframe_info *tinfo;
  int oind = 0;
  enum print_values var_print_values = PRINT_ALL_VALUES;
  enum print_values comp_print_values = PRINT_ALL_VALUES;
  int registers_format = 'x';
  int memory_contents = 0;
  struct ui_out *uiout = current_uiout;
  enum opt
  {
    VAR_PRINT_VALUES,
    COMP_PRINT_VALUES,
    REGISTERS_FORMAT,
    MEMORY_CONTENTS,
  };
  static const struct mi_opt opts[] =
    {
      {"-var-print-values", VAR_PRINT_VALUES, 1},
      {"-comp-print-values", COMP_PRINT_VALUES, 1},
      {"-registers-format", REGISTERS_FORMAT, 1},
      {"-memory-contents", MEMORY_CONTENTS, 0},
      { 0, 0, 0 }
    };

  while (1)
    {
      char *oarg;
      int opt = mi_getopt ("-trace-frame-collected", argc, argv, opts,
			   &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case VAR_PRINT_VALUES:
	  var_print_values = mi_parse_print_values (oarg);
	  break;
	case COMP_PRINT_VALUES:
	  comp_print_values = mi_parse_print_values (oarg);
	  break;
	case REGISTERS_FORMAT:
	  registers_format = oarg[0];
	case MEMORY_CONTENTS:
	  memory_contents = 1;
	  break;
	}
    }

  if (oind != argc)
    error (_("Usage: -trace-frame-collected "
	     "[--var-print-values PRINT_VALUES] "
	     "[--comp-print-values PRINT_VALUES] "
	     "[--registers-format FORMAT]"
	     "[--memory-contents]"));

  /* This throws an error is not inspecting a trace frame.  */
  tloc = get_traceframe_location (&stepping_frame);

  /* This command only makes sense for the current frame, not the
     selected frame.  */
  old_chain = make_cleanup_restore_current_thread ();
  select_frame (get_current_frame ());

  encode_actions (tloc, &tracepoint_list, &stepping_list);

  if (stepping_frame)
    clist = &stepping_list;
  else
    clist = &tracepoint_list;

  tinfo = get_traceframe_info ();

  /* Explicitly wholly collected variables.  */
  {
    struct cleanup *list_cleanup;
    int i;

    list_cleanup = make_cleanup_ui_out_list_begin_end (uiout,
						       "explicit-variables");

    const std::vector<std::string> &wholly_collected
      = clist->wholly_collected ();
    for (size_t i = 0; i < wholly_collected.size (); i++)
      {
	const std::string &str = wholly_collected[i];
	print_variable_or_computed (str.c_str (), var_print_values);
      }

    do_cleanups (list_cleanup);
  }

  /* Computed expressions.  */
  {
    struct cleanup *list_cleanup;
    char *p;
    int i;

    list_cleanup
      = make_cleanup_ui_out_list_begin_end (uiout,
					    "computed-expressions");

    const std::vector<std::string> &computed = clist->computed ();
    for (size_t i = 0; i < computed.size (); i++)
      {
	const std::string &str = computed[i];
	print_variable_or_computed (str.c_str (), comp_print_values);
      }

    do_cleanups (list_cleanup);
  }

  /* Registers.  Given pseudo-registers, and that some architectures
     (like MIPS) actually hide the raw registers, we don't go through
     the trace frame info, but instead consult the register cache for
     register availability.  */
  {
    struct cleanup *list_cleanup;
    struct frame_info *frame;
    struct gdbarch *gdbarch;
    int regnum;
    int numregs;

    list_cleanup = make_cleanup_ui_out_list_begin_end (uiout, "registers");

    frame = get_selected_frame (NULL);
    gdbarch = get_frame_arch (frame);
    numregs = gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);

    for (regnum = 0; regnum < numregs; regnum++)
      {
	if (gdbarch_register_name (gdbarch, regnum) == NULL
	    || *(gdbarch_register_name (gdbarch, regnum)) == '\0')
	  continue;

	output_register (frame, regnum, registers_format, 1);
      }

    do_cleanups (list_cleanup);
  }

  /* Trace state variables.  */
  {
    struct cleanup *list_cleanup;
    int tvar;
    char *tsvname;
    int i;

    list_cleanup = make_cleanup_ui_out_list_begin_end (uiout, "tvars");

    tsvname = NULL;
    make_cleanup (free_current_contents, &tsvname);

    for (i = 0; VEC_iterate (int, tinfo->tvars, i, tvar); i++)
      {
	struct cleanup *cleanup_child;
	struct trace_state_variable *tsv;

	tsv = find_trace_state_variable_by_number (tvar);

	cleanup_child = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);

	if (tsv != NULL)
	  {
	    tsvname = (char *) xrealloc (tsvname, strlen (tsv->name) + 2);
	    tsvname[0] = '$';
	    strcpy (tsvname + 1, tsv->name);
	    uiout->field_string ("name", tsvname);

	    tsv->value_known = target_get_trace_state_variable_value (tsv->number,
								      &tsv->value);
	    uiout->field_int ("current", tsv->value);
	  }
	else
	  {
	    uiout->field_skip ("name");
	    uiout->field_skip ("current");
	  }

	do_cleanups (cleanup_child);
      }

    do_cleanups (list_cleanup);
  }

  /* Memory.  */
  {
    struct cleanup *list_cleanup;
    VEC(mem_range_s) *available_memory = NULL;
    struct mem_range *r;
    int i;

    traceframe_available_memory (&available_memory, 0, ULONGEST_MAX);
    make_cleanup (VEC_cleanup(mem_range_s), &available_memory);

    list_cleanup = make_cleanup_ui_out_list_begin_end (uiout, "memory");

    for (i = 0; VEC_iterate (mem_range_s, available_memory, i, r); i++)
      {
	struct cleanup *cleanup_child;
	gdb_byte *data;
	struct gdbarch *gdbarch = target_gdbarch ();

	cleanup_child = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);

	uiout->field_core_addr ("address", gdbarch, r->start);
	uiout->field_int ("length", r->length);

	data = (gdb_byte *) xmalloc (r->length);
	make_cleanup (xfree, data);

	if (memory_contents)
	  {
	    if (target_read_memory (r->start, data, r->length) == 0)
	      {
		int m;
		char *data_str, *p;

		data_str = (char *) xmalloc (r->length * 2 + 1);
		make_cleanup (xfree, data_str);

		for (m = 0, p = data_str; m < r->length; ++m, p += 2)
		  sprintf (p, "%02x", data[m]);
		uiout->field_string ("contents", data_str);
	      }
	    else
	      uiout->field_skip ("contents");
	  }
	do_cleanups (cleanup_child);
      }

    do_cleanups (list_cleanup);
  }

  do_cleanups (old_chain);
}

void
_initialize_mi_main (void)
{
  struct cmd_list_element *c;

  add_setshow_boolean_cmd ("mi-async", class_run,
			   &mi_async_1, _("\
Set whether MI asynchronous mode is enabled."), _("\
Show whether MI asynchronous mode is enabled."), _("\
Tells GDB whether MI should be in asynchronous mode."),
			   set_mi_async_command,
			   show_mi_async_command,
			   &setlist,
			   &showlist);

  /* Alias old "target-async" to "mi-async".  */
  c = add_alias_cmd ("target-async", "mi-async", class_run, 0, &setlist);
  deprecate_cmd (c, "set mi-async");
  c = add_alias_cmd ("target-async", "mi-async", class_run, 0, &showlist);
  deprecate_cmd (c, "show mi-async");
}
