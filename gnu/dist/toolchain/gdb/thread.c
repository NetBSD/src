/* Multi-process/thread control for GDB, the GNU debugger.
   Copyright 1986, 1987, 1988, 1993, 1998, 1999, 2000

   Contributed by Lynx Real-Time Systems, Inc.  Los Gatos, CA.
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "environ.h"
#include "value.h"
#include "target.h"
#include "gdbthread.h"
#include "command.h"
#include "gdbcmd.h"

#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#ifdef UI_OUT
#include "ui-out.h"
#endif

/*#include "lynxos-core.h" */

/* Definition of struct thread_info exported to gdbthread.h */

/* Prototypes for exported functions. */

void _initialize_thread PARAMS ((void));

/* Prototypes for local functions. */

static struct thread_info *thread_list = NULL;
static int highest_thread_num;

static struct thread_info *find_thread_id PARAMS ((int num));

static void thread_command PARAMS ((char *tidstr, int from_tty));
static void thread_apply_all_command PARAMS ((char *, int));
static int thread_alive PARAMS ((struct thread_info *));
static void info_threads_command PARAMS ((char *, int));
static void thread_apply_command PARAMS ((char *, int));
static void restore_current_thread PARAMS ((int));
static void switch_to_thread PARAMS ((int pid));
static void prune_threads PARAMS ((void));

void
init_thread_list ()
{
  struct thread_info *tp, *tpnext;

  if (!thread_list)
    return;

  for (tp = thread_list; tp; tp = tpnext)
    {
      tpnext = tp->next;
      free (tp);
    }

  thread_list = NULL;
  highest_thread_num = 0;
}

/* add_thread now returns a pointer to the new thread_info, 
   so that back_ends can initialize their private data.  */

struct thread_info *
add_thread (pid)
     int pid;
{
  struct thread_info *tp;

  tp = (struct thread_info *) xmalloc (sizeof (struct thread_info));

  tp->pid = pid;
  tp->num = ++highest_thread_num;
  tp->prev_pc = 0;
  tp->prev_func_start = 0;
  tp->prev_func_name = NULL;
  tp->step_range_start = 0;
  tp->step_range_end = 0;
  tp->step_frame_address = 0;
  tp->step_resume_breakpoint = 0;
  tp->through_sigtramp_breakpoint = 0;
  tp->handling_longjmp = 0;
  tp->trap_expected = 0;
  tp->another_trap = 0;
  tp->stepping_through_solib_after_catch = 0;
  tp->stepping_through_solib_catchpoints = NULL;
  tp->stepping_through_sigtramp = 0;
  tp->next = thread_list;
  tp->private = NULL;
  thread_list = tp;
  return tp;
}

void
delete_thread (pid)
     int pid;
{
  struct thread_info *tp, *tpprev;

  tpprev = NULL;

  for (tp = thread_list; tp; tpprev = tp, tp = tp->next)
    if (tp->pid == pid)
      break;

  if (!tp)
    return;

  if (tpprev)
    tpprev->next = tp->next;
  else
    thread_list = tp->next;

  /* NOTE: this will take care of any left-over step_resume breakpoints,
     but not any user-specified thread-specific breakpoints. */
  if (tp->step_resume_breakpoint)
    delete_breakpoint (tp->step_resume_breakpoint);

  /* FIXME: do I ever need to call the back-end to give it a
     chance at this private data before deleting the thread?  */
  if (tp->private)
    free (tp->private);

  free (tp);

  return;
}

static struct thread_info *
find_thread_id (num)
     int num;
{
  struct thread_info *tp;

  for (tp = thread_list; tp; tp = tp->next)
    if (tp->num == num)
      return tp;

  return NULL;
}

/* Find a thread_info by matching 'pid'.  */
struct thread_info *
find_thread_pid (pid)
     int pid;
{
  struct thread_info *tp;

  for (tp = thread_list; tp; tp = tp->next)
    if (tp->pid == pid)
      return tp;

  return NULL;
}

/*
 * Thread iterator function.
 *
 * Calls a callback function once for each thread, so long as
 * the callback function returns false.  If the callback function
 * returns true, the iteration will end and the current thread
 * will be returned.  This can be useful for implementing a 
 * search for a thread with arbitrary attributes, or for applying
 * some operation to every thread.
 *
 * FIXME: some of the existing functionality, such as 
 * "Thread apply all", might be rewritten using this functionality.
 */

struct thread_info *
iterate_over_threads (callback, data)
     int (*callback) ();
     void *data;
{
  struct thread_info *tp;

  for (tp = thread_list; tp; tp = tp->next)
    if ((*callback) (tp, data))
      return tp;

  return NULL;
}

int
valid_thread_id (num)
     int num;
{
  struct thread_info *tp;

  for (tp = thread_list; tp; tp = tp->next)
    if (tp->num == num)
      return 1;

  return 0;
}

int
pid_to_thread_id (pid)
     int pid;
{
  struct thread_info *tp;

  for (tp = thread_list; tp; tp = tp->next)
    if (tp->pid == pid)
      return tp->num;

  return 0;
}

int
thread_id_to_pid (num)
     int num;
{
  struct thread_info *thread = find_thread_id (num);
  if (thread)
    return thread->pid;
  else
    return -1;
}

int
in_thread_list (pid)
     int pid;
{
  struct thread_info *tp;

  for (tp = thread_list; tp; tp = tp->next)
    if (tp->pid == pid)
      return 1;

  return 0;			/* Never heard of 'im */
}
#ifdef UI_OUT
/* Print a list of thread ids currently known, and the total number of
   threads. To be used from within catch_errors. */
static int 
do_captured_list_thread_ids (void *arg)
{
  struct thread_info *tp;
  int num = 0;

  ui_out_list_begin (uiout, "thread-ids");

  for (tp = thread_list; tp; tp = tp->next)
    {
      num++;
      ui_out_field_int (uiout, "thread-id", tp->num);
    }

  ui_out_list_end (uiout);
  ui_out_field_int (uiout, "number-of-threads", num);
  return GDB_RC_OK;
}

/* Official gdblib interface function to get a list of thread ids and
   the total number. */
enum gdb_rc
gdb_list_thread_ids (/* output object */)
{
  return catch_errors (do_captured_list_thread_ids, NULL,
		       NULL, RETURN_MASK_ALL);
}
#endif

/* Load infrun state for the thread PID.  */

void
load_infrun_state (pid, prev_pc, prev_func_start, prev_func_name,
		   trap_expected, step_resume_breakpoint,
		   through_sigtramp_breakpoint, step_range_start,
		   step_range_end, step_frame_address,
		   handling_longjmp, another_trap,
		   stepping_through_solib_after_catch,
		   stepping_through_solib_catchpoints,
		   stepping_through_sigtramp)
     int pid;
     CORE_ADDR *prev_pc;
     CORE_ADDR *prev_func_start;
     char **prev_func_name;
     int *trap_expected;
     struct breakpoint **step_resume_breakpoint;
     struct breakpoint **through_sigtramp_breakpoint;
     CORE_ADDR *step_range_start;
     CORE_ADDR *step_range_end;
     CORE_ADDR *step_frame_address;
     int *handling_longjmp;
     int *another_trap;
     int *stepping_through_solib_after_catch;
     bpstat *stepping_through_solib_catchpoints;
     int *stepping_through_sigtramp;
{
  struct thread_info *tp;

  /* If we can't find the thread, then we're debugging a single threaded
     process.  No need to do anything in that case.  */
  tp = find_thread_id (pid_to_thread_id (pid));
  if (tp == NULL)
    return;

  *prev_pc = tp->prev_pc;
  *prev_func_start = tp->prev_func_start;
  *prev_func_name = tp->prev_func_name;
  *step_resume_breakpoint = tp->step_resume_breakpoint;
  *step_range_start = tp->step_range_start;
  *step_range_end = tp->step_range_end;
  *step_frame_address = tp->step_frame_address;
  *through_sigtramp_breakpoint = tp->through_sigtramp_breakpoint;
  *handling_longjmp = tp->handling_longjmp;
  *trap_expected = tp->trap_expected;
  *another_trap = tp->another_trap;
  *stepping_through_solib_after_catch = tp->stepping_through_solib_after_catch;
  *stepping_through_solib_catchpoints = tp->stepping_through_solib_catchpoints;
  *stepping_through_sigtramp = tp->stepping_through_sigtramp;
}

/* Save infrun state for the thread PID.  */

void
save_infrun_state (pid, prev_pc, prev_func_start, prev_func_name,
		   trap_expected, step_resume_breakpoint,
		   through_sigtramp_breakpoint, step_range_start,
		   step_range_end, step_frame_address,
		   handling_longjmp, another_trap,
		   stepping_through_solib_after_catch,
		   stepping_through_solib_catchpoints,
		   stepping_through_sigtramp)
     int pid;
     CORE_ADDR prev_pc;
     CORE_ADDR prev_func_start;
     char *prev_func_name;
     int trap_expected;
     struct breakpoint *step_resume_breakpoint;
     struct breakpoint *through_sigtramp_breakpoint;
     CORE_ADDR step_range_start;
     CORE_ADDR step_range_end;
     CORE_ADDR step_frame_address;
     int handling_longjmp;
     int another_trap;
     int stepping_through_solib_after_catch;
     bpstat stepping_through_solib_catchpoints;
     int stepping_through_sigtramp;
{
  struct thread_info *tp;

  /* If we can't find the thread, then we're debugging a single-threaded
     process.  Nothing to do in that case.  */
  tp = find_thread_id (pid_to_thread_id (pid));
  if (tp == NULL)
    return;

  tp->prev_pc = prev_pc;
  tp->prev_func_start = prev_func_start;
  tp->prev_func_name = prev_func_name;
  tp->step_resume_breakpoint = step_resume_breakpoint;
  tp->step_range_start = step_range_start;
  tp->step_range_end = step_range_end;
  tp->step_frame_address = step_frame_address;
  tp->through_sigtramp_breakpoint = through_sigtramp_breakpoint;
  tp->handling_longjmp = handling_longjmp;
  tp->trap_expected = trap_expected;
  tp->another_trap = another_trap;
  tp->stepping_through_solib_after_catch = stepping_through_solib_after_catch;
  tp->stepping_through_solib_catchpoints = stepping_through_solib_catchpoints;
  tp->stepping_through_sigtramp = stepping_through_sigtramp;
}

/* Return true if TP is an active thread. */
static int
thread_alive (tp)
     struct thread_info *tp;
{
  if (tp->pid == -1)
    return 0;
  if (!target_thread_alive (tp->pid))
    {
      tp->pid = -1;		/* Mark it as dead */
      return 0;
    }
  return 1;
}

static void
prune_threads ()
{
  struct thread_info *tp, *next;

  for (tp = thread_list; tp; tp = next)
    {
      next = tp->next;
      if (!thread_alive (tp))
	delete_thread (tp->pid);
    }
}

/* Print information about currently known threads 

 * Note: this has the drawback that it _really_ switches
 *       threads, which frees the frame cache.  A no-side
 *       effects info-threads command would be nicer.
 */

static void
info_threads_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  struct thread_info *tp;
  int current_pid;
  struct frame_info *cur_frame;
  int saved_frame_level = selected_frame_level;
  int counter;
  char *extra_info;

  /* Avoid coredumps which would happen if we tried to access a NULL
     selected_frame.  */
  if (!target_has_stack)
    error ("No stack.");

  prune_threads ();
  target_find_new_threads ();
  current_pid = inferior_pid;
  for (tp = thread_list; tp; tp = tp->next)
    {
      if (tp->pid == current_pid)
	printf_filtered ("* ");
      else
	printf_filtered ("  ");

#ifdef HPUXHPPA
      printf_filtered ("%d %s", tp->num, target_tid_to_str (tp->pid));
#else
      printf_filtered ("%d %s", tp->num, target_pid_to_str (tp->pid));
#endif

      extra_info = target_extra_thread_info (tp);
      if (extra_info)
	printf_filtered (" (%s)", extra_info);
      puts_filtered ("  ");

      switch_to_thread (tp->pid);
      if (selected_frame)
	print_only_stack_frame (selected_frame, -1, 0);
      else
	printf_filtered ("[No stack.]\n");
    }

  switch_to_thread (current_pid);

  /* Code below copied from "up_silently_base" in "stack.c".
   * It restores the frame set by the user before the "info threads"
   * command.  We have finished the info-threads display by switching
   * back to the current thread.  That switch has put us at the top
   * of the stack (leaf frame).
   */
  counter = saved_frame_level;
  cur_frame = find_relative_frame (selected_frame, &counter);
  if (counter != 0)
    {
      /* Ooops, can't restore, tell user where we are. */
      warning ("Couldn't restore frame in current thread, at frame 0");
      print_stack_frame (selected_frame, -1, 0);
    }
  else
    {
      select_frame (cur_frame, saved_frame_level);
    }

  /* re-show current frame. */
  show_stack_frame (cur_frame);
}

/* Switch from one thread to another. */

static void
switch_to_thread (pid)
     int pid;
{
  if (pid == inferior_pid)
    return;

  inferior_pid = pid;
  flush_cached_frames ();
  registers_changed ();
  stop_pc = read_pc ();
  select_frame (get_current_frame (), 0);
}

static void
restore_current_thread (pid)
     int pid;
{
  if (pid != inferior_pid)
    {
      switch_to_thread (pid);
      print_stack_frame (get_current_frame (), 0, -1);
    }
}

/* Apply a GDB command to a list of threads.  List syntax is a whitespace
   seperated list of numbers, or ranges, or the keyword `all'.  Ranges consist
   of two numbers seperated by a hyphen.  Examples:

   thread apply 1 2 7 4 backtrace       Apply backtrace cmd to threads 1,2,7,4
   thread apply 2-7 9 p foo(1)  Apply p foo(1) cmd to threads 2->7 & 9
   thread apply all p x/i $pc   Apply x/i $pc cmd to all threads
 */

static void
thread_apply_all_command (cmd, from_tty)
     char *cmd;
     int from_tty;
{
  struct thread_info *tp;
  struct cleanup *old_chain;

  if (cmd == NULL || *cmd == '\000')
    error ("Please specify a command following the thread ID list");

  old_chain = make_cleanup ((make_cleanup_func) restore_current_thread,
			    (void *) inferior_pid);

  for (tp = thread_list; tp; tp = tp->next)
    if (thread_alive (tp))
      {
	switch_to_thread (tp->pid);
#ifdef HPUXHPPA
	printf_filtered ("\nThread %d (%s):\n",
			 tp->num,
			 target_tid_to_str (inferior_pid));
#else
	printf_filtered ("\nThread %d (%s):\n", tp->num,
			 target_pid_to_str (inferior_pid));
#endif
	execute_command (cmd, from_tty);
      }
}

static void
thread_apply_command (tidlist, from_tty)
     char *tidlist;
     int from_tty;
{
  char *cmd;
  char *p;
  struct cleanup *old_chain;

  if (tidlist == NULL || *tidlist == '\000')
    error ("Please specify a thread ID list");

  for (cmd = tidlist; *cmd != '\000' && !isalpha (*cmd); cmd++);

  if (*cmd == '\000')
    error ("Please specify a command following the thread ID list");

  old_chain = make_cleanup ((make_cleanup_func) restore_current_thread,
			    (void *) inferior_pid);

  while (tidlist < cmd)
    {
      struct thread_info *tp;
      int start, end;

      start = strtol (tidlist, &p, 10);
      if (p == tidlist)
	error ("Error parsing %s", tidlist);
      tidlist = p;

      while (*tidlist == ' ' || *tidlist == '\t')
	tidlist++;

      if (*tidlist == '-')	/* Got a range of IDs? */
	{
	  tidlist++;		/* Skip the - */
	  end = strtol (tidlist, &p, 10);
	  if (p == tidlist)
	    error ("Error parsing %s", tidlist);
	  tidlist = p;

	  while (*tidlist == ' ' || *tidlist == '\t')
	    tidlist++;
	}
      else
	end = start;

      for (; start <= end; start++)
	{
	  tp = find_thread_id (start);

	  if (!tp)
	    warning ("Unknown thread %d.", start);
	  else if (!thread_alive (tp))
	    warning ("Thread %d has terminated.", start);
	  else
	    {
	      switch_to_thread (tp->pid);
#ifdef HPUXHPPA
	      printf_filtered ("\nThread %d (%s):\n", tp->num,
			       target_tid_to_str (inferior_pid));
#else
	      printf_filtered ("\nThread %d (%s):\n", tp->num,
			       target_pid_to_str (inferior_pid));
#endif
	      execute_command (cmd, from_tty);
	    }
	}
    }
}

/* Switch to the specified thread.  Will dispatch off to thread_apply_command
   if prefix of arg is `apply'.  */

static void
thread_command (tidstr, from_tty)
     char *tidstr;
     int from_tty;
{
  if (!tidstr)
    {
      /* Don't generate an error, just say which thread is current. */
      if (target_has_stack)
	printf_filtered ("[Current thread is %d (%s)]\n",
			 pid_to_thread_id (inferior_pid),
#if defined(HPUXHPPA)
			 target_tid_to_str (inferior_pid)
#else
			 target_pid_to_str (inferior_pid)
#endif
	  );
      else
	error ("No stack.");
      return;
    }

  gdb_thread_select (tidstr);
}

static int
do_captured_thread_select (void *tidstr)
{
  int num;
  struct thread_info *tp;

  num = atoi ((char *)tidstr);

  tp = find_thread_id (num);

#ifdef UI_OUT
  if (!tp)
    error ("Thread ID %d not known.", num);
#else
  if (!tp)
    error ("Thread ID %d not known.  Use the \"info threads\" command to\n\
see the IDs of currently known threads.", num);
#endif

  if (!thread_alive (tp))
    error ("Thread ID %d has terminated.\n", num);

  switch_to_thread (tp->pid);

#ifdef UI_OUT
  ui_out_text (uiout, "[Switching to thread ");
  ui_out_field_int (uiout, "new-thread-id", pid_to_thread_id (inferior_pid));
  ui_out_text (uiout, " (");
#if defined(HPUXHPPA)
  ui_out_text (uiout, target_tid_to_str (inferior_pid));
#else
  ui_out_text (uiout, target_pid_to_str (inferior_pid));
#endif
  ui_out_text (uiout, ")]");
#else /* UI_OUT */
  printf_filtered ("[Switching to thread %d (%s)]\n",
		   pid_to_thread_id (inferior_pid),
#if defined(HPUXHPPA)
		   target_tid_to_str (inferior_pid)
#else
		   target_pid_to_str (inferior_pid)
#endif
    );
#endif /* UI_OUT */

  print_stack_frame (selected_frame, selected_frame_level, 1);
  return GDB_RC_OK;
}

enum gdb_rc
gdb_thread_select (char *tidstr)
{
  return catch_errors (do_captured_thread_select, tidstr,
		       NULL, RETURN_MASK_ALL);
}

/* Commands with a prefix of `thread'.  */
struct cmd_list_element *thread_cmd_list = NULL;

void
_initialize_thread ()
{
  static struct cmd_list_element *thread_apply_list = NULL;

  add_info ("threads", info_threads_command,
	    "IDs of currently known threads.");

  add_prefix_cmd ("thread", class_run, thread_command,
		  "Use this command to switch between threads.\n\
The new thread ID must be currently known.", &thread_cmd_list, "thread ", 1,
		  &cmdlist);

  add_prefix_cmd ("apply", class_run, thread_apply_command,
		  "Apply a command to a list of threads.",
		  &thread_apply_list, "apply ", 1, &thread_cmd_list);

  add_cmd ("all", class_run, thread_apply_all_command,
	   "Apply a command to all threads.",
	   &thread_apply_list);

  if (!xdb_commands)
    add_com_alias ("t", "thread", class_run, 1);
}
