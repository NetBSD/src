/* Target-struct-independent code to start (run) and stop an inferior
   process.

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
#include "infrun.h"
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "breakpoint.h"
#include "gdb_wait.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "cli/cli-script.h"
#include "target.h"
#include "gdbthread.h"
#include "annotate.h"
#include "symfile.h"
#include "top.h"
#include <signal.h>
#include "inf-loop.h"
#include "regcache.h"
#include "value.h"
#include "observer.h"
#include "language.h"
#include "solib.h"
#include "main.h"
#include "dictionary.h"
#include "block.h"
#include "mi/mi-common.h"
#include "event-top.h"
#include "record.h"
#include "record-full.h"
#include "inline-frame.h"
#include "jit.h"
#include "tracepoint.h"
#include "continuations.h"
#include "interps.h"
#include "skip.h"
#include "probe.h"
#include "objfiles.h"
#include "completer.h"
#include "target-descriptions.h"
#include "target-dcache.h"
#include "terminal.h"
#include "solist.h"
#include "event-loop.h"
#include "thread-fsm.h"
#include "common/enum-flags.h"

/* Prototypes for local functions */

static void signals_info (char *, int);

static void handle_command (char *, int);

static void sig_print_info (enum gdb_signal);

static void sig_print_header (void);

static void resume_cleanups (void *);

static int hook_stop_stub (void *);

static int restore_selected_frame (void *);

static int follow_fork (void);

static int follow_fork_inferior (int follow_child, int detach_fork);

static void follow_inferior_reset_breakpoints (void);

static void set_schedlock_func (char *args, int from_tty,
				struct cmd_list_element *c);

static int currently_stepping (struct thread_info *tp);

void _initialize_infrun (void);

void nullify_last_target_wait_ptid (void);

static void insert_hp_step_resume_breakpoint_at_frame (struct frame_info *);

static void insert_step_resume_breakpoint_at_caller (struct frame_info *);

static void insert_longjmp_resume_breakpoint (struct gdbarch *, CORE_ADDR);

static int maybe_software_singlestep (struct gdbarch *gdbarch, CORE_ADDR pc);

/* Asynchronous signal handler registered as event loop source for
   when we have pending events ready to be passed to the core.  */
static struct async_event_handler *infrun_async_inferior_event_token;

/* Stores whether infrun_async was previously enabled or disabled.
   Starts off as -1, indicating "never enabled/disabled".  */
static int infrun_is_async = -1;

/* See infrun.h.  */

void
infrun_async (int enable)
{
  if (infrun_is_async != enable)
    {
      infrun_is_async = enable;

      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: infrun_async(%d)\n",
			    enable);

      if (enable)
	mark_async_event_handler (infrun_async_inferior_event_token);
      else
	clear_async_event_handler (infrun_async_inferior_event_token);
    }
}

/* See infrun.h.  */

void
mark_infrun_async_event_handler (void)
{
  mark_async_event_handler (infrun_async_inferior_event_token);
}

/* When set, stop the 'step' command if we enter a function which has
   no line number information.  The normal behavior is that we step
   over such function.  */
int step_stop_if_no_debug = 0;
static void
show_step_stop_if_no_debug (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Mode of the step operation is %s.\n"), value);
}

/* proceed and normal_stop use this to notify the user when the
   inferior stopped in a different thread than it had been running
   in.  */

static ptid_t previous_inferior_ptid;

/* If set (default for legacy reasons), when following a fork, GDB
   will detach from one of the fork branches, child or parent.
   Exactly which branch is detached depends on 'set follow-fork-mode'
   setting.  */

static int detach_fork = 1;

int debug_displaced = 0;
static void
show_debug_displaced (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Displace stepping debugging is %s.\n"), value);
}

unsigned int debug_infrun = 0;
static void
show_debug_infrun (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Inferior debugging is %s.\n"), value);
}


/* Support for disabling address space randomization.  */

int disable_randomization = 1;

static void
show_disable_randomization (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  if (target_supports_disable_randomization ())
    fprintf_filtered (file,
		      _("Disabling randomization of debuggee's "
			"virtual address space is %s.\n"),
		      value);
  else
    fputs_filtered (_("Disabling randomization of debuggee's "
		      "virtual address space is unsupported on\n"
		      "this platform.\n"), file);
}

static void
set_disable_randomization (char *args, int from_tty,
			   struct cmd_list_element *c)
{
  if (!target_supports_disable_randomization ())
    error (_("Disabling randomization of debuggee's "
	     "virtual address space is unsupported on\n"
	     "this platform."));
}

/* User interface for non-stop mode.  */

int non_stop = 0;
static int non_stop_1 = 0;

static void
set_non_stop (char *args, int from_tty,
	      struct cmd_list_element *c)
{
  if (target_has_execution)
    {
      non_stop_1 = non_stop;
      error (_("Cannot change this setting while the inferior is running."));
    }

  non_stop = non_stop_1;
}

static void
show_non_stop (struct ui_file *file, int from_tty,
	       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Controlling the inferior in non-stop mode is %s.\n"),
		    value);
}

/* "Observer mode" is somewhat like a more extreme version of
   non-stop, in which all GDB operations that might affect the
   target's execution have been disabled.  */

int observer_mode = 0;
static int observer_mode_1 = 0;

static void
set_observer_mode (char *args, int from_tty,
		   struct cmd_list_element *c)
{
  if (target_has_execution)
    {
      observer_mode_1 = observer_mode;
      error (_("Cannot change this setting while the inferior is running."));
    }

  observer_mode = observer_mode_1;

  may_write_registers = !observer_mode;
  may_write_memory = !observer_mode;
  may_insert_breakpoints = !observer_mode;
  may_insert_tracepoints = !observer_mode;
  /* We can insert fast tracepoints in or out of observer mode,
     but enable them if we're going into this mode.  */
  if (observer_mode)
    may_insert_fast_tracepoints = 1;
  may_stop = !observer_mode;
  update_target_permissions ();

  /* Going *into* observer mode we must force non-stop, then
     going out we leave it that way.  */
  if (observer_mode)
    {
      pagination_enabled = 0;
      non_stop = non_stop_1 = 1;
    }

  if (from_tty)
    printf_filtered (_("Observer mode is now %s.\n"),
		     (observer_mode ? "on" : "off"));
}

static void
show_observer_mode (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Observer mode is %s.\n"), value);
}

/* This updates the value of observer mode based on changes in
   permissions.  Note that we are deliberately ignoring the values of
   may-write-registers and may-write-memory, since the user may have
   reason to enable these during a session, for instance to turn on a
   debugging-related global.  */

void
update_observer_mode (void)
{
  int newval;

  newval = (!may_insert_breakpoints
	    && !may_insert_tracepoints
	    && may_insert_fast_tracepoints
	    && !may_stop
	    && non_stop);

  /* Let the user know if things change.  */
  if (newval != observer_mode)
    printf_filtered (_("Observer mode is now %s.\n"),
		     (newval ? "on" : "off"));

  observer_mode = observer_mode_1 = newval;
}

/* Tables of how to react to signals; the user sets them.  */

static unsigned char *signal_stop;
static unsigned char *signal_print;
static unsigned char *signal_program;

/* Table of signals that are registered with "catch signal".  A
   non-zero entry indicates that the signal is caught by some "catch
   signal" command.  This has size GDB_SIGNAL_LAST, to accommodate all
   signals.  */
static unsigned char *signal_catch;

/* Table of signals that the target may silently handle.
   This is automatically determined from the flags above,
   and simply cached here.  */
static unsigned char *signal_pass;

#define SET_SIGS(nsigs,sigs,flags) \
  do { \
    int signum = (nsigs); \
    while (signum-- > 0) \
      if ((sigs)[signum]) \
	(flags)[signum] = 1; \
  } while (0)

#define UNSET_SIGS(nsigs,sigs,flags) \
  do { \
    int signum = (nsigs); \
    while (signum-- > 0) \
      if ((sigs)[signum]) \
	(flags)[signum] = 0; \
  } while (0)

/* Update the target's copy of SIGNAL_PROGRAM.  The sole purpose of
   this function is to avoid exporting `signal_program'.  */

void
update_signals_program_target (void)
{
  target_program_signals ((int) GDB_SIGNAL_LAST, signal_program);
}

/* Value to pass to target_resume() to cause all threads to resume.  */

#define RESUME_ALL minus_one_ptid

/* Command list pointer for the "stop" placeholder.  */

static struct cmd_list_element *stop_command;

/* Nonzero if we want to give control to the user when we're notified
   of shared library events by the dynamic linker.  */
int stop_on_solib_events;

/* Enable or disable optional shared library event breakpoints
   as appropriate when the above flag is changed.  */

static void
set_stop_on_solib_events (char *args, int from_tty, struct cmd_list_element *c)
{
  update_solib_breakpoints ();
}

static void
show_stop_on_solib_events (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Stopping for shared library events is %s.\n"),
		    value);
}

/* Nonzero after stop if current stack frame should be printed.  */

static int stop_print_frame;

/* This is a cached copy of the pid/waitstatus of the last event
   returned by target_wait()/deprecated_target_wait_hook().  This
   information is returned by get_last_target_status().  */
static ptid_t target_last_wait_ptid;
static struct target_waitstatus target_last_waitstatus;

static void context_switch (ptid_t ptid);

void init_thread_stepping_state (struct thread_info *tss);

static const char follow_fork_mode_child[] = "child";
static const char follow_fork_mode_parent[] = "parent";

static const char *const follow_fork_mode_kind_names[] = {
  follow_fork_mode_child,
  follow_fork_mode_parent,
  NULL
};

static const char *follow_fork_mode_string = follow_fork_mode_parent;
static void
show_follow_fork_mode_string (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Debugger response to a program "
		      "call of fork or vfork is \"%s\".\n"),
		    value);
}


/* Handle changes to the inferior list based on the type of fork,
   which process is being followed, and whether the other process
   should be detached.  On entry inferior_ptid must be the ptid of
   the fork parent.  At return inferior_ptid is the ptid of the
   followed inferior.  */

static int
follow_fork_inferior (int follow_child, int detach_fork)
{
  int has_vforked;
  ptid_t parent_ptid, child_ptid;

  has_vforked = (inferior_thread ()->pending_follow.kind
		 == TARGET_WAITKIND_VFORKED);
  parent_ptid = inferior_ptid;
  child_ptid = inferior_thread ()->pending_follow.value.related_pid;

  if (has_vforked
      && !non_stop /* Non-stop always resumes both branches.  */
      && current_ui->prompt_state == PROMPT_BLOCKED
      && !(follow_child || detach_fork || sched_multi))
    {
      /* The parent stays blocked inside the vfork syscall until the
	 child execs or exits.  If we don't let the child run, then
	 the parent stays blocked.  If we're telling the parent to run
	 in the foreground, the user will not be able to ctrl-c to get
	 back the terminal, effectively hanging the debug session.  */
      fprintf_filtered (gdb_stderr, _("\
Can not resume the parent process over vfork in the foreground while\n\
holding the child stopped.  Try \"set detach-on-fork\" or \
\"set schedule-multiple\".\n"));
      /* FIXME output string > 80 columns.  */
      return 1;
    }

  if (!follow_child)
    {
      /* Detach new forked process?  */
      if (detach_fork)
	{
	  /* Before detaching from the child, remove all breakpoints
	     from it.  If we forked, then this has already been taken
	     care of by infrun.c.  If we vforked however, any
	     breakpoint inserted in the parent is visible in the
	     child, even those added while stopped in a vfork
	     catchpoint.  This will remove the breakpoints from the
	     parent also, but they'll be reinserted below.  */
	  if (has_vforked)
	    {
	      /* Keep breakpoints list in sync.  */
	      remove_breakpoints_pid (ptid_get_pid (inferior_ptid));
	    }

	  if (info_verbose || debug_infrun)
	    {
	      /* Ensure that we have a process ptid.  */
	      ptid_t process_ptid = pid_to_ptid (ptid_get_pid (child_ptid));

	      target_terminal_ours_for_output ();
	      fprintf_filtered (gdb_stdlog,
				_("Detaching after %s from child %s.\n"),
				has_vforked ? "vfork" : "fork",
				target_pid_to_str (process_ptid));
	    }
	}
      else
	{
	  struct inferior *parent_inf, *child_inf;
	  struct cleanup *old_chain;

	  /* Add process to GDB's tables.  */
	  child_inf = add_inferior (ptid_get_pid (child_ptid));

	  parent_inf = current_inferior ();
	  child_inf->attach_flag = parent_inf->attach_flag;
	  copy_terminal_info (child_inf, parent_inf);
	  child_inf->gdbarch = parent_inf->gdbarch;
	  copy_inferior_target_desc_info (child_inf, parent_inf);

	  old_chain = save_inferior_ptid ();
	  save_current_program_space ();

	  inferior_ptid = child_ptid;
	  add_thread (inferior_ptid);
	  child_inf->symfile_flags = SYMFILE_NO_READ;

	  /* If this is a vfork child, then the address-space is
	     shared with the parent.  */
	  if (has_vforked)
	    {
	      child_inf->pspace = parent_inf->pspace;
	      child_inf->aspace = parent_inf->aspace;

	      /* The parent will be frozen until the child is done
		 with the shared region.  Keep track of the
		 parent.  */
	      child_inf->vfork_parent = parent_inf;
	      child_inf->pending_detach = 0;
	      parent_inf->vfork_child = child_inf;
	      parent_inf->pending_detach = 0;
	    }
	  else
	    {
	      child_inf->aspace = new_address_space ();
	      child_inf->pspace = add_program_space (child_inf->aspace);
	      child_inf->removable = 1;
	      set_current_program_space (child_inf->pspace);
	      clone_program_space (child_inf->pspace, parent_inf->pspace);

	      /* Let the shared library layer (e.g., solib-svr4) learn
		 about this new process, relocate the cloned exec, pull
		 in shared libraries, and install the solib event
		 breakpoint.  If a "cloned-VM" event was propagated
		 better throughout the core, this wouldn't be
		 required.  */
	      solib_create_inferior_hook (0);
	    }

	  do_cleanups (old_chain);
	}

      if (has_vforked)
	{
	  struct inferior *parent_inf;

	  parent_inf = current_inferior ();

	  /* If we detached from the child, then we have to be careful
	     to not insert breakpoints in the parent until the child
	     is done with the shared memory region.  However, if we're
	     staying attached to the child, then we can and should
	     insert breakpoints, so that we can debug it.  A
	     subsequent child exec or exit is enough to know when does
	     the child stops using the parent's address space.  */
	  parent_inf->waiting_for_vfork_done = detach_fork;
	  parent_inf->pspace->breakpoints_not_allowed = detach_fork;
	}
    }
  else
    {
      /* Follow the child.  */
      struct inferior *parent_inf, *child_inf;
      struct program_space *parent_pspace;

      if (info_verbose || debug_infrun)
	{
	  target_terminal_ours_for_output ();
	  fprintf_filtered (gdb_stdlog,
			    _("Attaching after %s %s to child %s.\n"),
			    target_pid_to_str (parent_ptid),
			    has_vforked ? "vfork" : "fork",
			    target_pid_to_str (child_ptid));
	}

      /* Add the new inferior first, so that the target_detach below
	 doesn't unpush the target.  */

      child_inf = add_inferior (ptid_get_pid (child_ptid));

      parent_inf = current_inferior ();
      child_inf->attach_flag = parent_inf->attach_flag;
      copy_terminal_info (child_inf, parent_inf);
      child_inf->gdbarch = parent_inf->gdbarch;
      copy_inferior_target_desc_info (child_inf, parent_inf);

      parent_pspace = parent_inf->pspace;

      /* If we're vforking, we want to hold on to the parent until the
	 child exits or execs.  At child exec or exit time we can
	 remove the old breakpoints from the parent and detach or
	 resume debugging it.  Otherwise, detach the parent now; we'll
	 want to reuse it's program/address spaces, but we can't set
	 them to the child before removing breakpoints from the
	 parent, otherwise, the breakpoints module could decide to
	 remove breakpoints from the wrong process (since they'd be
	 assigned to the same address space).  */

      if (has_vforked)
	{
	  gdb_assert (child_inf->vfork_parent == NULL);
	  gdb_assert (parent_inf->vfork_child == NULL);
	  child_inf->vfork_parent = parent_inf;
	  child_inf->pending_detach = 0;
	  parent_inf->vfork_child = child_inf;
	  parent_inf->pending_detach = detach_fork;
	  parent_inf->waiting_for_vfork_done = 0;
	}
      else if (detach_fork)
	{
	  if (info_verbose || debug_infrun)
	    {
	      /* Ensure that we have a process ptid.  */
	      ptid_t process_ptid = pid_to_ptid (ptid_get_pid (child_ptid));

	      target_terminal_ours_for_output ();
	      fprintf_filtered (gdb_stdlog,
				_("Detaching after fork from "
				  "child %s.\n"),
				target_pid_to_str (process_ptid));
	    }

	  target_detach (NULL, 0);
	}

      /* Note that the detach above makes PARENT_INF dangling.  */

      /* Add the child thread to the appropriate lists, and switch to
	 this new thread, before cloning the program space, and
	 informing the solib layer about this new process.  */

      inferior_ptid = child_ptid;
      add_thread (inferior_ptid);

      /* If this is a vfork child, then the address-space is shared
	 with the parent.  If we detached from the parent, then we can
	 reuse the parent's program/address spaces.  */
      if (has_vforked || detach_fork)
	{
	  child_inf->pspace = parent_pspace;
	  child_inf->aspace = child_inf->pspace->aspace;
	}
      else
	{
	  child_inf->aspace = new_address_space ();
	  child_inf->pspace = add_program_space (child_inf->aspace);
	  child_inf->removable = 1;
	  child_inf->symfile_flags = SYMFILE_NO_READ;
	  set_current_program_space (child_inf->pspace);
	  clone_program_space (child_inf->pspace, parent_pspace);

	  /* Let the shared library layer (e.g., solib-svr4) learn
	     about this new process, relocate the cloned exec, pull in
	     shared libraries, and install the solib event breakpoint.
	     If a "cloned-VM" event was propagated better throughout
	     the core, this wouldn't be required.  */
	  solib_create_inferior_hook (0);
	}
    }

  return target_follow_fork (follow_child, detach_fork);
}

/* Tell the target to follow the fork we're stopped at.  Returns true
   if the inferior should be resumed; false, if the target for some
   reason decided it's best not to resume.  */

static int
follow_fork (void)
{
  int follow_child = (follow_fork_mode_string == follow_fork_mode_child);
  int should_resume = 1;
  struct thread_info *tp;

  /* Copy user stepping state to the new inferior thread.  FIXME: the
     followed fork child thread should have a copy of most of the
     parent thread structure's run control related fields, not just these.
     Initialized to avoid "may be used uninitialized" warnings from gcc.  */
  struct breakpoint *step_resume_breakpoint = NULL;
  struct breakpoint *exception_resume_breakpoint = NULL;
  CORE_ADDR step_range_start = 0;
  CORE_ADDR step_range_end = 0;
  struct frame_id step_frame_id = { 0 };
  struct thread_fsm *thread_fsm = NULL;

  if (!non_stop)
    {
      ptid_t wait_ptid;
      struct target_waitstatus wait_status;

      /* Get the last target status returned by target_wait().  */
      get_last_target_status (&wait_ptid, &wait_status);

      /* If not stopped at a fork event, then there's nothing else to
	 do.  */
      if (wait_status.kind != TARGET_WAITKIND_FORKED
	  && wait_status.kind != TARGET_WAITKIND_VFORKED)
	return 1;

      /* Check if we switched over from WAIT_PTID, since the event was
	 reported.  */
      if (!ptid_equal (wait_ptid, minus_one_ptid)
	  && !ptid_equal (inferior_ptid, wait_ptid))
	{
	  /* We did.  Switch back to WAIT_PTID thread, to tell the
	     target to follow it (in either direction).  We'll
	     afterwards refuse to resume, and inform the user what
	     happened.  */
	  switch_to_thread (wait_ptid);
	  should_resume = 0;
	}
    }

  tp = inferior_thread ();

  /* If there were any forks/vforks that were caught and are now to be
     followed, then do so now.  */
  switch (tp->pending_follow.kind)
    {
    case TARGET_WAITKIND_FORKED:
    case TARGET_WAITKIND_VFORKED:
      {
	ptid_t parent, child;

	/* If the user did a next/step, etc, over a fork call,
	   preserve the stepping state in the fork child.  */
	if (follow_child && should_resume)
	  {
	    step_resume_breakpoint = clone_momentary_breakpoint
					 (tp->control.step_resume_breakpoint);
	    step_range_start = tp->control.step_range_start;
	    step_range_end = tp->control.step_range_end;
	    step_frame_id = tp->control.step_frame_id;
	    exception_resume_breakpoint
	      = clone_momentary_breakpoint (tp->control.exception_resume_breakpoint);
	    thread_fsm = tp->thread_fsm;

	    /* For now, delete the parent's sr breakpoint, otherwise,
	       parent/child sr breakpoints are considered duplicates,
	       and the child version will not be installed.  Remove
	       this when the breakpoints module becomes aware of
	       inferiors and address spaces.  */
	    delete_step_resume_breakpoint (tp);
	    tp->control.step_range_start = 0;
	    tp->control.step_range_end = 0;
	    tp->control.step_frame_id = null_frame_id;
	    delete_exception_resume_breakpoint (tp);
	    tp->thread_fsm = NULL;
	  }

	parent = inferior_ptid;
	child = tp->pending_follow.value.related_pid;

	/* Set up inferior(s) as specified by the caller, and tell the
	   target to do whatever is necessary to follow either parent
	   or child.  */
	if (follow_fork_inferior (follow_child, detach_fork))
	  {
	    /* Target refused to follow, or there's some other reason
	       we shouldn't resume.  */
	    should_resume = 0;
	  }
	else
	  {
	    /* This pending follow fork event is now handled, one way
	       or another.  The previous selected thread may be gone
	       from the lists by now, but if it is still around, need
	       to clear the pending follow request.  */
	    tp = find_thread_ptid (parent);
	    if (tp)
	      tp->pending_follow.kind = TARGET_WAITKIND_SPURIOUS;

	    /* This makes sure we don't try to apply the "Switched
	       over from WAIT_PID" logic above.  */
	    nullify_last_target_wait_ptid ();

	    /* If we followed the child, switch to it...  */
	    if (follow_child)
	      {
		switch_to_thread (child);

		/* ... and preserve the stepping state, in case the
		   user was stepping over the fork call.  */
		if (should_resume)
		  {
		    tp = inferior_thread ();
		    tp->control.step_resume_breakpoint
		      = step_resume_breakpoint;
		    tp->control.step_range_start = step_range_start;
		    tp->control.step_range_end = step_range_end;
		    tp->control.step_frame_id = step_frame_id;
		    tp->control.exception_resume_breakpoint
		      = exception_resume_breakpoint;
		    tp->thread_fsm = thread_fsm;
		  }
		else
		  {
		    /* If we get here, it was because we're trying to
		       resume from a fork catchpoint, but, the user
		       has switched threads away from the thread that
		       forked.  In that case, the resume command
		       issued is most likely not applicable to the
		       child, so just warn, and refuse to resume.  */
		    warning (_("Not resuming: switched threads "
			       "before following fork child."));
		  }

		/* Reset breakpoints in the child as appropriate.  */
		follow_inferior_reset_breakpoints ();
	      }
	    else
	      switch_to_thread (parent);
	  }
      }
      break;
    case TARGET_WAITKIND_SPURIOUS:
      /* Nothing to follow.  */
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      "Unexpected pending_follow.kind %d\n",
		      tp->pending_follow.kind);
      break;
    }

  return should_resume;
}

static void
follow_inferior_reset_breakpoints (void)
{
  struct thread_info *tp = inferior_thread ();

  /* Was there a step_resume breakpoint?  (There was if the user
     did a "next" at the fork() call.)  If so, explicitly reset its
     thread number.  Cloned step_resume breakpoints are disabled on
     creation, so enable it here now that it is associated with the
     correct thread.

     step_resumes are a form of bp that are made to be per-thread.
     Since we created the step_resume bp when the parent process
     was being debugged, and now are switching to the child process,
     from the breakpoint package's viewpoint, that's a switch of
     "threads".  We must update the bp's notion of which thread
     it is for, or it'll be ignored when it triggers.  */

  if (tp->control.step_resume_breakpoint)
    {
      breakpoint_re_set_thread (tp->control.step_resume_breakpoint);
      tp->control.step_resume_breakpoint->loc->enabled = 1;
    }

  /* Treat exception_resume breakpoints like step_resume breakpoints.  */
  if (tp->control.exception_resume_breakpoint)
    {
      breakpoint_re_set_thread (tp->control.exception_resume_breakpoint);
      tp->control.exception_resume_breakpoint->loc->enabled = 1;
    }

  /* Reinsert all breakpoints in the child.  The user may have set
     breakpoints after catching the fork, in which case those
     were never set in the child, but only in the parent.  This makes
     sure the inserted breakpoints match the breakpoint list.  */

  breakpoint_re_set ();
  insert_breakpoints ();
}

/* The child has exited or execed: resume threads of the parent the
   user wanted to be executing.  */

static int
proceed_after_vfork_done (struct thread_info *thread,
			  void *arg)
{
  int pid = * (int *) arg;

  if (ptid_get_pid (thread->ptid) == pid
      && is_running (thread->ptid)
      && !is_executing (thread->ptid)
      && !thread->stop_requested
      && thread->suspend.stop_signal == GDB_SIGNAL_0)
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: resuming vfork parent thread %s\n",
			    target_pid_to_str (thread->ptid));

      switch_to_thread (thread->ptid);
      clear_proceed_status (0);
      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }

  return 0;
}

/* Called whenever we notice an exec or exit event, to handle
   detaching or resuming a vfork parent.  */

static void
handle_vfork_child_exec_or_exit (int exec)
{
  struct inferior *inf = current_inferior ();

  if (inf->vfork_parent)
    {
      int resume_parent = -1;

      /* This exec or exit marks the end of the shared memory region
	 between the parent and the child.  If the user wanted to
	 detach from the parent, now is the time.  */

      if (inf->vfork_parent->pending_detach)
	{
	  struct thread_info *tp;
	  struct cleanup *old_chain;
	  struct program_space *pspace;
	  struct address_space *aspace;

	  /* follow-fork child, detach-on-fork on.  */

	  inf->vfork_parent->pending_detach = 0;

	  if (!exec)
	    {
	      /* If we're handling a child exit, then inferior_ptid
		 points at the inferior's pid, not to a thread.  */
	      old_chain = save_inferior_ptid ();
	      save_current_program_space ();
	      save_current_inferior ();
	    }
	  else
	    old_chain = save_current_space_and_thread ();

	  /* We're letting loose of the parent.  */
	  tp = any_live_thread_of_process (inf->vfork_parent->pid);
	  switch_to_thread (tp->ptid);

	  /* We're about to detach from the parent, which implicitly
	     removes breakpoints from its address space.  There's a
	     catch here: we want to reuse the spaces for the child,
	     but, parent/child are still sharing the pspace at this
	     point, although the exec in reality makes the kernel give
	     the child a fresh set of new pages.  The problem here is
	     that the breakpoints module being unaware of this, would
	     likely chose the child process to write to the parent
	     address space.  Swapping the child temporarily away from
	     the spaces has the desired effect.  Yes, this is "sort
	     of" a hack.  */

	  pspace = inf->pspace;
	  aspace = inf->aspace;
	  inf->aspace = NULL;
	  inf->pspace = NULL;

	  if (debug_infrun || info_verbose)
	    {
	      target_terminal_ours_for_output ();

	      if (exec)
		{
		  fprintf_filtered (gdb_stdlog,
				    _("Detaching vfork parent process "
				      "%d after child exec.\n"),
				    inf->vfork_parent->pid);
		}
	      else
		{
		  fprintf_filtered (gdb_stdlog,
				    _("Detaching vfork parent process "
				      "%d after child exit.\n"),
				    inf->vfork_parent->pid);
		}
	    }

	  target_detach (NULL, 0);

	  /* Put it back.  */
	  inf->pspace = pspace;
	  inf->aspace = aspace;

	  do_cleanups (old_chain);
	}
      else if (exec)
	{
	  /* We're staying attached to the parent, so, really give the
	     child a new address space.  */
	  inf->pspace = add_program_space (maybe_new_address_space ());
	  inf->aspace = inf->pspace->aspace;
	  inf->removable = 1;
	  set_current_program_space (inf->pspace);

	  resume_parent = inf->vfork_parent->pid;

	  /* Break the bonds.  */
	  inf->vfork_parent->vfork_child = NULL;
	}
      else
	{
	  struct cleanup *old_chain;
	  struct program_space *pspace;

	  /* If this is a vfork child exiting, then the pspace and
	     aspaces were shared with the parent.  Since we're
	     reporting the process exit, we'll be mourning all that is
	     found in the address space, and switching to null_ptid,
	     preparing to start a new inferior.  But, since we don't
	     want to clobber the parent's address/program spaces, we
	     go ahead and create a new one for this exiting
	     inferior.  */

	  /* Switch to null_ptid, so that clone_program_space doesn't want
	     to read the selected frame of a dead process.  */
	  old_chain = save_inferior_ptid ();
	  inferior_ptid = null_ptid;

	  /* This inferior is dead, so avoid giving the breakpoints
	     module the option to write through to it (cloning a
	     program space resets breakpoints).  */
	  inf->aspace = NULL;
	  inf->pspace = NULL;
	  pspace = add_program_space (maybe_new_address_space ());
	  set_current_program_space (pspace);
	  inf->removable = 1;
	  inf->symfile_flags = SYMFILE_NO_READ;
	  clone_program_space (pspace, inf->vfork_parent->pspace);
	  inf->pspace = pspace;
	  inf->aspace = pspace->aspace;

	  /* Put back inferior_ptid.  We'll continue mourning this
	     inferior.  */
	  do_cleanups (old_chain);

	  resume_parent = inf->vfork_parent->pid;
	  /* Break the bonds.  */
	  inf->vfork_parent->vfork_child = NULL;
	}

      inf->vfork_parent = NULL;

      gdb_assert (current_program_space == inf->pspace);

      if (non_stop && resume_parent != -1)
	{
	  /* If the user wanted the parent to be running, let it go
	     free now.  */
	  struct cleanup *old_chain = make_cleanup_restore_current_thread ();

	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: resuming vfork parent process %d\n",
				resume_parent);

	  iterate_over_threads (proceed_after_vfork_done, &resume_parent);

	  do_cleanups (old_chain);
	}
    }
}

/* Enum strings for "set|show follow-exec-mode".  */

static const char follow_exec_mode_new[] = "new";
static const char follow_exec_mode_same[] = "same";
static const char *const follow_exec_mode_names[] =
{
  follow_exec_mode_new,
  follow_exec_mode_same,
  NULL,
};

static const char *follow_exec_mode_string = follow_exec_mode_same;
static void
show_follow_exec_mode_string (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Follow exec mode is \"%s\".\n"),  value);
}

/* EXECD_PATHNAME is assumed to be non-NULL.  */

static void
follow_exec (ptid_t ptid, char *execd_pathname)
{
  struct thread_info *th, *tmp;
  struct inferior *inf = current_inferior ();
  int pid = ptid_get_pid (ptid);
  ptid_t process_ptid;

  /* This is an exec event that we actually wish to pay attention to.
     Refresh our symbol table to the newly exec'd program, remove any
     momentary bp's, etc.

     If there are breakpoints, they aren't really inserted now,
     since the exec() transformed our inferior into a fresh set
     of instructions.

     We want to preserve symbolic breakpoints on the list, since
     we have hopes that they can be reset after the new a.out's
     symbol table is read.

     However, any "raw" breakpoints must be removed from the list
     (e.g., the solib bp's), since their address is probably invalid
     now.

     And, we DON'T want to call delete_breakpoints() here, since
     that may write the bp's "shadow contents" (the instruction
     value that was overwritten witha TRAP instruction).  Since
     we now have a new a.out, those shadow contents aren't valid.  */

  mark_breakpoints_out ();

  /* The target reports the exec event to the main thread, even if
     some other thread does the exec, and even if the main thread was
     stopped or already gone.  We may still have non-leader threads of
     the process on our list.  E.g., on targets that don't have thread
     exit events (like remote); or on native Linux in non-stop mode if
     there were only two threads in the inferior and the non-leader
     one is the one that execs (and nothing forces an update of the
     thread list up to here).  When debugging remotely, it's best to
     avoid extra traffic, when possible, so avoid syncing the thread
     list with the target, and instead go ahead and delete all threads
     of the process but one that reported the event.  Note this must
     be done before calling update_breakpoints_after_exec, as
     otherwise clearing the threads' resources would reference stale
     thread breakpoints -- it may have been one of these threads that
     stepped across the exec.  We could just clear their stepping
     states, but as long as we're iterating, might as well delete
     them.  Deleting them now rather than at the next user-visible
     stop provides a nicer sequence of events for user and MI
     notifications.  */
  ALL_THREADS_SAFE (th, tmp)
    if (ptid_get_pid (th->ptid) == pid && !ptid_equal (th->ptid, ptid))
      delete_thread (th->ptid);

  /* We also need to clear any left over stale state for the
     leader/event thread.  E.g., if there was any step-resume
     breakpoint or similar, it's gone now.  We cannot truly
     step-to-next statement through an exec().  */
  th = inferior_thread ();
  th->control.step_resume_breakpoint = NULL;
  th->control.exception_resume_breakpoint = NULL;
  th->control.single_step_breakpoints = NULL;
  th->control.step_range_start = 0;
  th->control.step_range_end = 0;

  /* The user may have had the main thread held stopped in the
     previous image (e.g., schedlock on, or non-stop).  Release
     it now.  */
  th->stop_requested = 0;

  update_breakpoints_after_exec ();

  /* What is this a.out's name?  */
  process_ptid = pid_to_ptid (pid);
  printf_unfiltered (_("%s is executing new program: %s\n"),
		     target_pid_to_str (process_ptid),
		     execd_pathname);

  /* We've followed the inferior through an exec.  Therefore, the
     inferior has essentially been killed & reborn.  */

  gdb_flush (gdb_stdout);

  breakpoint_init_inferior (inf_execd);

  if (*gdb_sysroot != '\0')
    {
      char *name = exec_file_find (execd_pathname, NULL);

      execd_pathname = (char *) alloca (strlen (name) + 1);
      strcpy (execd_pathname, name);
      xfree (name);
    }

  /* Reset the shared library package.  This ensures that we get a
     shlib event when the child reaches "_start", at which point the
     dld will have had a chance to initialize the child.  */
  /* Also, loading a symbol file below may trigger symbol lookups, and
     we don't want those to be satisfied by the libraries of the
     previous incarnation of this process.  */
  no_shared_libraries (NULL, 0);

  if (follow_exec_mode_string == follow_exec_mode_new)
    {
      /* The user wants to keep the old inferior and program spaces
	 around.  Create a new fresh one, and switch to it.  */

      /* Do exit processing for the original inferior before adding
	 the new inferior so we don't have two active inferiors with
	 the same ptid, which can confuse find_inferior_ptid.  */
      exit_inferior_num_silent (current_inferior ()->num);

      inf = add_inferior_with_spaces ();
      inf->pid = pid;
      target_follow_exec (inf, execd_pathname);

      set_current_inferior (inf);
      set_current_program_space (inf->pspace);
      add_thread (ptid);
    }
  else
    {
      /* The old description may no longer be fit for the new image.
	 E.g, a 64-bit process exec'ed a 32-bit process.  Clear the
	 old description; we'll read a new one below.  No need to do
	 this on "follow-exec-mode new", as the old inferior stays
	 around (its description is later cleared/refetched on
	 restart).  */
      target_clear_description ();
    }

  gdb_assert (current_program_space == inf->pspace);

  /* That a.out is now the one to use.  */
  exec_file_attach (execd_pathname, 0);

  /* SYMFILE_DEFER_BP_RESET is used as the proper displacement for PIE
     (Position Independent Executable) main symbol file will get applied by
     solib_create_inferior_hook below.  breakpoint_re_set would fail to insert
     the breakpoints with the zero displacement.  */

  symbol_file_add (execd_pathname,
		   (inf->symfile_flags
		    | SYMFILE_MAINLINE | SYMFILE_DEFER_BP_RESET),
		   NULL, 0);

  if ((inf->symfile_flags & SYMFILE_NO_READ) == 0)
    set_initial_language ();

  /* If the target can specify a description, read it.  Must do this
     after flipping to the new executable (because the target supplied
     description must be compatible with the executable's
     architecture, and the old executable may e.g., be 32-bit, while
     the new one 64-bit), and before anything involving memory or
     registers.  */
  target_find_description ();

  solib_create_inferior_hook (0);

  jit_inferior_created_hook ();

  breakpoint_re_set ();

  /* Reinsert all breakpoints.  (Those which were symbolic have
     been reset to the proper address in the new a.out, thanks
     to symbol_file_command...).  */
  insert_breakpoints ();

  /* The next resume of this inferior should bring it to the shlib
     startup breakpoints.  (If the user had also set bp's on
     "main" from the old (parent) process, then they'll auto-
     matically get reset there in the new process.).  */
}

/* The queue of threads that need to do a step-over operation to get
   past e.g., a breakpoint.  What technique is used to step over the
   breakpoint/watchpoint does not matter -- all threads end up in the
   same queue, to maintain rough temporal order of execution, in order
   to avoid starvation, otherwise, we could e.g., find ourselves
   constantly stepping the same couple threads past their breakpoints
   over and over, if the single-step finish fast enough.  */
struct thread_info *step_over_queue_head;

/* Bit flags indicating what the thread needs to step over.  */

enum step_over_what_flag
  {
    /* Step over a breakpoint.  */
    STEP_OVER_BREAKPOINT = 1,

    /* Step past a non-continuable watchpoint, in order to let the
       instruction execute so we can evaluate the watchpoint
       expression.  */
    STEP_OVER_WATCHPOINT = 2
  };
DEF_ENUM_FLAGS_TYPE (enum step_over_what_flag, step_over_what);

/* Info about an instruction that is being stepped over.  */

struct step_over_info
{
  /* If we're stepping past a breakpoint, this is the address space
     and address of the instruction the breakpoint is set at.  We'll
     skip inserting all breakpoints here.  Valid iff ASPACE is
     non-NULL.  */
  struct address_space *aspace;
  CORE_ADDR address;

  /* The instruction being stepped over triggers a nonsteppable
     watchpoint.  If true, we'll skip inserting watchpoints.  */
  int nonsteppable_watchpoint_p;

  /* The thread's global number.  */
  int thread;
};

/* The step-over info of the location that is being stepped over.

   Note that with async/breakpoint always-inserted mode, a user might
   set a new breakpoint/watchpoint/etc. exactly while a breakpoint is
   being stepped over.  As setting a new breakpoint inserts all
   breakpoints, we need to make sure the breakpoint being stepped over
   isn't inserted then.  We do that by only clearing the step-over
   info when the step-over is actually finished (or aborted).

   Presently GDB can only step over one breakpoint at any given time.
   Given threads that can't run code in the same address space as the
   breakpoint's can't really miss the breakpoint, GDB could be taught
   to step-over at most one breakpoint per address space (so this info
   could move to the address space object if/when GDB is extended).
   The set of breakpoints being stepped over will normally be much
   smaller than the set of all breakpoints, so a flag in the
   breakpoint location structure would be wasteful.  A separate list
   also saves complexity and run-time, as otherwise we'd have to go
   through all breakpoint locations clearing their flag whenever we
   start a new sequence.  Similar considerations weigh against storing
   this info in the thread object.  Plus, not all step overs actually
   have breakpoint locations -- e.g., stepping past a single-step
   breakpoint, or stepping to complete a non-continuable
   watchpoint.  */
static struct step_over_info step_over_info;

/* Record the address of the breakpoint/instruction we're currently
   stepping over.  */

static void
set_step_over_info (struct address_space *aspace, CORE_ADDR address,
		    int nonsteppable_watchpoint_p,
		    int thread)
{
  step_over_info.aspace = aspace;
  step_over_info.address = address;
  step_over_info.nonsteppable_watchpoint_p = nonsteppable_watchpoint_p;
  step_over_info.thread = thread;
}

/* Called when we're not longer stepping over a breakpoint / an
   instruction, so all breakpoints are free to be (re)inserted.  */

static void
clear_step_over_info (void)
{
  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: clear_step_over_info\n");
  step_over_info.aspace = NULL;
  step_over_info.address = 0;
  step_over_info.nonsteppable_watchpoint_p = 0;
  step_over_info.thread = -1;
}

/* See infrun.h.  */

int
stepping_past_instruction_at (struct address_space *aspace,
			      CORE_ADDR address)
{
  return (step_over_info.aspace != NULL
	  && breakpoint_address_match (aspace, address,
				       step_over_info.aspace,
				       step_over_info.address));
}

/* See infrun.h.  */

int
thread_is_stepping_over_breakpoint (int thread)
{
  return (step_over_info.thread != -1
	  && thread == step_over_info.thread);
}

/* See infrun.h.  */

int
stepping_past_nonsteppable_watchpoint (void)
{
  return step_over_info.nonsteppable_watchpoint_p;
}

/* Returns true if step-over info is valid.  */

static int
step_over_info_valid_p (void)
{
  return (step_over_info.aspace != NULL
	  || stepping_past_nonsteppable_watchpoint ());
}


/* Displaced stepping.  */

/* In non-stop debugging mode, we must take special care to manage
   breakpoints properly; in particular, the traditional strategy for
   stepping a thread past a breakpoint it has hit is unsuitable.
   'Displaced stepping' is a tactic for stepping one thread past a
   breakpoint it has hit while ensuring that other threads running
   concurrently will hit the breakpoint as they should.

   The traditional way to step a thread T off a breakpoint in a
   multi-threaded program in all-stop mode is as follows:

   a0) Initially, all threads are stopped, and breakpoints are not
       inserted.
   a1) We single-step T, leaving breakpoints uninserted.
   a2) We insert breakpoints, and resume all threads.

   In non-stop debugging, however, this strategy is unsuitable: we
   don't want to have to stop all threads in the system in order to
   continue or step T past a breakpoint.  Instead, we use displaced
   stepping:

   n0) Initially, T is stopped, other threads are running, and
       breakpoints are inserted.
   n1) We copy the instruction "under" the breakpoint to a separate
       location, outside the main code stream, making any adjustments
       to the instruction, register, and memory state as directed by
       T's architecture.
   n2) We single-step T over the instruction at its new location.
   n3) We adjust the resulting register and memory state as directed
       by T's architecture.  This includes resetting T's PC to point
       back into the main instruction stream.
   n4) We resume T.

   This approach depends on the following gdbarch methods:

   - gdbarch_max_insn_length and gdbarch_displaced_step_location
     indicate where to copy the instruction, and how much space must
     be reserved there.  We use these in step n1.

   - gdbarch_displaced_step_copy_insn copies a instruction to a new
     address, and makes any necessary adjustments to the instruction,
     register contents, and memory.  We use this in step n1.

   - gdbarch_displaced_step_fixup adjusts registers and memory after
     we have successfuly single-stepped the instruction, to yield the
     same effect the instruction would have had if we had executed it
     at its original address.  We use this in step n3.

   - gdbarch_displaced_step_free_closure provides cleanup.

   The gdbarch_displaced_step_copy_insn and
   gdbarch_displaced_step_fixup functions must be written so that
   copying an instruction with gdbarch_displaced_step_copy_insn,
   single-stepping across the copied instruction, and then applying
   gdbarch_displaced_insn_fixup should have the same effects on the
   thread's memory and registers as stepping the instruction in place
   would have.  Exactly which responsibilities fall to the copy and
   which fall to the fixup is up to the author of those functions.

   See the comments in gdbarch.sh for details.

   Note that displaced stepping and software single-step cannot
   currently be used in combination, although with some care I think
   they could be made to.  Software single-step works by placing
   breakpoints on all possible subsequent instructions; if the
   displaced instruction is a PC-relative jump, those breakpoints
   could fall in very strange places --- on pages that aren't
   executable, or at addresses that are not proper instruction
   boundaries.  (We do generally let other threads run while we wait
   to hit the software single-step breakpoint, and they might
   encounter such a corrupted instruction.)  One way to work around
   this would be to have gdbarch_displaced_step_copy_insn fully
   simulate the effect of PC-relative instructions (and return NULL)
   on architectures that use software single-stepping.

   In non-stop mode, we can have independent and simultaneous step
   requests, so more than one thread may need to simultaneously step
   over a breakpoint.  The current implementation assumes there is
   only one scratch space per process.  In this case, we have to
   serialize access to the scratch space.  If thread A wants to step
   over a breakpoint, but we are currently waiting for some other
   thread to complete a displaced step, we leave thread A stopped and
   place it in the displaced_step_request_queue.  Whenever a displaced
   step finishes, we pick the next thread in the queue and start a new
   displaced step operation on it.  See displaced_step_prepare and
   displaced_step_fixup for details.  */

/* Per-inferior displaced stepping state.  */
struct displaced_step_inferior_state
{
  /* Pointer to next in linked list.  */
  struct displaced_step_inferior_state *next;

  /* The process this displaced step state refers to.  */
  int pid;

  /* True if preparing a displaced step ever failed.  If so, we won't
     try displaced stepping for this inferior again.  */
  int failed_before;

  /* If this is not null_ptid, this is the thread carrying out a
     displaced single-step in process PID.  This thread's state will
     require fixing up once it has completed its step.  */
  ptid_t step_ptid;

  /* The architecture the thread had when we stepped it.  */
  struct gdbarch *step_gdbarch;

  /* The closure provided gdbarch_displaced_step_copy_insn, to be used
     for post-step cleanup.  */
  struct displaced_step_closure *step_closure;

  /* The address of the original instruction, and the copy we
     made.  */
  CORE_ADDR step_original, step_copy;

  /* Saved contents of copy area.  */
  gdb_byte *step_saved_copy;
};

/* The list of states of processes involved in displaced stepping
   presently.  */
static struct displaced_step_inferior_state *displaced_step_inferior_states;

/* Get the displaced stepping state of process PID.  */

static struct displaced_step_inferior_state *
get_displaced_stepping_state (int pid)
{
  struct displaced_step_inferior_state *state;

  for (state = displaced_step_inferior_states;
       state != NULL;
       state = state->next)
    if (state->pid == pid)
      return state;

  return NULL;
}

/* Returns true if any inferior has a thread doing a displaced
   step.  */

static int
displaced_step_in_progress_any_inferior (void)
{
  struct displaced_step_inferior_state *state;

  for (state = displaced_step_inferior_states;
       state != NULL;
       state = state->next)
    if (!ptid_equal (state->step_ptid, null_ptid))
      return 1;

  return 0;
}

/* Return true if thread represented by PTID is doing a displaced
   step.  */

static int
displaced_step_in_progress_thread (ptid_t ptid)
{
  struct displaced_step_inferior_state *displaced;

  gdb_assert (!ptid_equal (ptid, null_ptid));

  displaced = get_displaced_stepping_state (ptid_get_pid (ptid));

  return (displaced != NULL && ptid_equal (displaced->step_ptid, ptid));
}

/* Return true if process PID has a thread doing a displaced step.  */

static int
displaced_step_in_progress (int pid)
{
  struct displaced_step_inferior_state *displaced;

  displaced = get_displaced_stepping_state (pid);
  if (displaced != NULL && !ptid_equal (displaced->step_ptid, null_ptid))
    return 1;

  return 0;
}

/* Add a new displaced stepping state for process PID to the displaced
   stepping state list, or return a pointer to an already existing
   entry, if it already exists.  Never returns NULL.  */

static struct displaced_step_inferior_state *
add_displaced_stepping_state (int pid)
{
  struct displaced_step_inferior_state *state;

  for (state = displaced_step_inferior_states;
       state != NULL;
       state = state->next)
    if (state->pid == pid)
      return state;

  state = XCNEW (struct displaced_step_inferior_state);
  state->pid = pid;
  state->next = displaced_step_inferior_states;
  displaced_step_inferior_states = state;

  return state;
}

/* If inferior is in displaced stepping, and ADDR equals to starting address
   of copy area, return corresponding displaced_step_closure.  Otherwise,
   return NULL.  */

struct displaced_step_closure*
get_displaced_step_closure_by_addr (CORE_ADDR addr)
{
  struct displaced_step_inferior_state *displaced
    = get_displaced_stepping_state (ptid_get_pid (inferior_ptid));

  /* If checking the mode of displaced instruction in copy area.  */
  if (displaced && !ptid_equal (displaced->step_ptid, null_ptid)
     && (displaced->step_copy == addr))
    return displaced->step_closure;

  return NULL;
}

/* Remove the displaced stepping state of process PID.  */

static void
remove_displaced_stepping_state (int pid)
{
  struct displaced_step_inferior_state *it, **prev_next_p;

  gdb_assert (pid != 0);

  it = displaced_step_inferior_states;
  prev_next_p = &displaced_step_inferior_states;
  while (it)
    {
      if (it->pid == pid)
	{
	  *prev_next_p = it->next;
	  xfree (it);
	  return;
	}

      prev_next_p = &it->next;
      it = *prev_next_p;
    }
}

static void
infrun_inferior_exit (struct inferior *inf)
{
  remove_displaced_stepping_state (inf->pid);
}

/* If ON, and the architecture supports it, GDB will use displaced
   stepping to step over breakpoints.  If OFF, or if the architecture
   doesn't support it, GDB will instead use the traditional
   hold-and-step approach.  If AUTO (which is the default), GDB will
   decide which technique to use to step over breakpoints depending on
   which of all-stop or non-stop mode is active --- displaced stepping
   in non-stop mode; hold-and-step in all-stop mode.  */

static enum auto_boolean can_use_displaced_stepping = AUTO_BOOLEAN_AUTO;

static void
show_can_use_displaced_stepping (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
  if (can_use_displaced_stepping == AUTO_BOOLEAN_AUTO)
    fprintf_filtered (file,
		      _("Debugger's willingness to use displaced stepping "
			"to step over breakpoints is %s (currently %s).\n"),
		      value, target_is_non_stop_p () ? "on" : "off");
  else
    fprintf_filtered (file,
		      _("Debugger's willingness to use displaced stepping "
			"to step over breakpoints is %s.\n"), value);
}

/* Return non-zero if displaced stepping can/should be used to step
   over breakpoints of thread TP.  */

static int
use_displaced_stepping (struct thread_info *tp)
{
  struct regcache *regcache = get_thread_regcache (tp->ptid);
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct displaced_step_inferior_state *displaced_state;

  displaced_state = get_displaced_stepping_state (ptid_get_pid (tp->ptid));

  return (((can_use_displaced_stepping == AUTO_BOOLEAN_AUTO
	    && target_is_non_stop_p ())
	   || can_use_displaced_stepping == AUTO_BOOLEAN_TRUE)
	  && gdbarch_displaced_step_copy_insn_p (gdbarch)
	  && find_record_target () == NULL
	  && (displaced_state == NULL
	      || !displaced_state->failed_before));
}

/* Clean out any stray displaced stepping state.  */
static void
displaced_step_clear (struct displaced_step_inferior_state *displaced)
{
  /* Indicate that there is no cleanup pending.  */
  displaced->step_ptid = null_ptid;

  if (displaced->step_closure)
    {
      gdbarch_displaced_step_free_closure (displaced->step_gdbarch,
                                           displaced->step_closure);
      displaced->step_closure = NULL;
    }
}

static void
displaced_step_clear_cleanup (void *arg)
{
  struct displaced_step_inferior_state *state
    = (struct displaced_step_inferior_state *) arg;

  displaced_step_clear (state);
}

/* Dump LEN bytes at BUF in hex to FILE, followed by a newline.  */
void
displaced_step_dump_bytes (struct ui_file *file,
                           const gdb_byte *buf,
                           size_t len)
{
  int i;

  for (i = 0; i < len; i++)
    fprintf_unfiltered (file, "%02x ", buf[i]);
  fputs_unfiltered ("\n", file);
}

/* Prepare to single-step, using displaced stepping.

   Note that we cannot use displaced stepping when we have a signal to
   deliver.  If we have a signal to deliver and an instruction to step
   over, then after the step, there will be no indication from the
   target whether the thread entered a signal handler or ignored the
   signal and stepped over the instruction successfully --- both cases
   result in a simple SIGTRAP.  In the first case we mustn't do a
   fixup, and in the second case we must --- but we can't tell which.
   Comments in the code for 'random signals' in handle_inferior_event
   explain how we handle this case instead.

   Returns 1 if preparing was successful -- this thread is going to be
   stepped now; 0 if displaced stepping this thread got queued; or -1
   if this instruction can't be displaced stepped.  */

static int
displaced_step_prepare_throw (ptid_t ptid)
{
  struct cleanup *old_cleanups, *ignore_cleanups;
  struct thread_info *tp = find_thread_ptid (ptid);
  struct regcache *regcache = get_thread_regcache (ptid);
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct address_space *aspace = get_regcache_aspace (regcache);
  CORE_ADDR original, copy;
  ULONGEST len;
  struct displaced_step_closure *closure;
  struct displaced_step_inferior_state *displaced;
  int status;

  /* We should never reach this function if the architecture does not
     support displaced stepping.  */
  gdb_assert (gdbarch_displaced_step_copy_insn_p (gdbarch));

  /* Nor if the thread isn't meant to step over a breakpoint.  */
  gdb_assert (tp->control.trap_expected);

  /* Disable range stepping while executing in the scratch pad.  We
     want a single-step even if executing the displaced instruction in
     the scratch buffer lands within the stepping range (e.g., a
     jump/branch).  */
  tp->control.may_range_step = 0;

  /* We have to displaced step one thread at a time, as we only have
     access to a single scratch space per inferior.  */

  displaced = add_displaced_stepping_state (ptid_get_pid (ptid));

  if (!ptid_equal (displaced->step_ptid, null_ptid))
    {
      /* Already waiting for a displaced step to finish.  Defer this
	 request and place in queue.  */

      if (debug_displaced)
	fprintf_unfiltered (gdb_stdlog,
			    "displaced: deferring step of %s\n",
			    target_pid_to_str (ptid));

      thread_step_over_chain_enqueue (tp);
      return 0;
    }
  else
    {
      if (debug_displaced)
	fprintf_unfiltered (gdb_stdlog,
			    "displaced: stepping %s now\n",
			    target_pid_to_str (ptid));
    }

  displaced_step_clear (displaced);

  old_cleanups = save_inferior_ptid ();
  inferior_ptid = ptid;

  original = regcache_read_pc (regcache);

  copy = gdbarch_displaced_step_location (gdbarch);
  len = gdbarch_max_insn_length (gdbarch);

  if (breakpoint_in_range_p (aspace, copy, len))
    {
      /* There's a breakpoint set in the scratch pad location range
	 (which is usually around the entry point).  We'd either
	 install it before resuming, which would overwrite/corrupt the
	 scratch pad, or if it was already inserted, this displaced
	 step would overwrite it.  The latter is OK in the sense that
	 we already assume that no thread is going to execute the code
	 in the scratch pad range (after initial startup) anyway, but
	 the former is unacceptable.  Simply punt and fallback to
	 stepping over this breakpoint in-line.  */
      if (debug_displaced)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "displaced: breakpoint set in scratch pad.  "
			      "Stepping over breakpoint in-line instead.\n");
	}

      do_cleanups (old_cleanups);
      return -1;
    }

  /* Save the original contents of the copy area.  */
  displaced->step_saved_copy = (gdb_byte *) xmalloc (len);
  ignore_cleanups = make_cleanup (free_current_contents,
				  &displaced->step_saved_copy);
  status = target_read_memory (copy, displaced->step_saved_copy, len);
  if (status != 0)
    throw_error (MEMORY_ERROR,
		 _("Error accessing memory address %s (%s) for "
		   "displaced-stepping scratch space."),
		 paddress (gdbarch, copy), safe_strerror (status));
  if (debug_displaced)
    {
      fprintf_unfiltered (gdb_stdlog, "displaced: saved %s: ",
			  paddress (gdbarch, copy));
      displaced_step_dump_bytes (gdb_stdlog,
				 displaced->step_saved_copy,
				 len);
    };

  closure = gdbarch_displaced_step_copy_insn (gdbarch,
					      original, copy, regcache);
  if (closure == NULL)
    {
      /* The architecture doesn't know how or want to displaced step
	 this instruction or instruction sequence.  Fallback to
	 stepping over the breakpoint in-line.  */
      do_cleanups (old_cleanups);
      return -1;
    }

  /* Save the information we need to fix things up if the step
     succeeds.  */
  displaced->step_ptid = ptid;
  displaced->step_gdbarch = gdbarch;
  displaced->step_closure = closure;
  displaced->step_original = original;
  displaced->step_copy = copy;

  make_cleanup (displaced_step_clear_cleanup, displaced);

  /* Resume execution at the copy.  */
  regcache_write_pc (regcache, copy);

  discard_cleanups (ignore_cleanups);

  do_cleanups (old_cleanups);

  if (debug_displaced)
    fprintf_unfiltered (gdb_stdlog, "displaced: displaced pc to %s\n",
			paddress (gdbarch, copy));

  return 1;
}

/* Wrapper for displaced_step_prepare_throw that disabled further
   attempts at displaced stepping if we get a memory error.  */

static int
displaced_step_prepare (ptid_t ptid)
{
  int prepared = -1;

  TRY
    {
      prepared = displaced_step_prepare_throw (ptid);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      struct displaced_step_inferior_state *displaced_state;

      if (ex.error != MEMORY_ERROR
	  && ex.error != NOT_SUPPORTED_ERROR)
	throw_exception (ex);

      if (debug_infrun)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "infrun: disabling displaced stepping: %s\n",
			      ex.message);
	}

      /* Be verbose if "set displaced-stepping" is "on", silent if
	 "auto".  */
      if (can_use_displaced_stepping == AUTO_BOOLEAN_TRUE)
	{
	  warning (_("disabling displaced stepping: %s"),
		   ex.message);
	}

      /* Disable further displaced stepping attempts.  */
      displaced_state
	= get_displaced_stepping_state (ptid_get_pid (ptid));
      displaced_state->failed_before = 1;
    }
  END_CATCH

  return prepared;
}

static void
write_memory_ptid (ptid_t ptid, CORE_ADDR memaddr,
		   const gdb_byte *myaddr, int len)
{
  struct cleanup *ptid_cleanup = save_inferior_ptid ();

  inferior_ptid = ptid;
  write_memory (memaddr, myaddr, len);
  do_cleanups (ptid_cleanup);
}

/* Restore the contents of the copy area for thread PTID.  */

static void
displaced_step_restore (struct displaced_step_inferior_state *displaced,
			ptid_t ptid)
{
  ULONGEST len = gdbarch_max_insn_length (displaced->step_gdbarch);

  write_memory_ptid (ptid, displaced->step_copy,
		     displaced->step_saved_copy, len);
  if (debug_displaced)
    fprintf_unfiltered (gdb_stdlog, "displaced: restored %s %s\n",
			target_pid_to_str (ptid),
			paddress (displaced->step_gdbarch,
				  displaced->step_copy));
}

/* If we displaced stepped an instruction successfully, adjust
   registers and memory to yield the same effect the instruction would
   have had if we had executed it at its original address, and return
   1.  If the instruction didn't complete, relocate the PC and return
   -1.  If the thread wasn't displaced stepping, return 0.  */

static int
displaced_step_fixup (ptid_t event_ptid, enum gdb_signal signal)
{
  struct cleanup *old_cleanups;
  struct displaced_step_inferior_state *displaced
    = get_displaced_stepping_state (ptid_get_pid (event_ptid));
  int ret;

  /* Was any thread of this process doing a displaced step?  */
  if (displaced == NULL)
    return 0;

  /* Was this event for the pid we displaced?  */
  if (ptid_equal (displaced->step_ptid, null_ptid)
      || ! ptid_equal (displaced->step_ptid, event_ptid))
    return 0;

  old_cleanups = make_cleanup (displaced_step_clear_cleanup, displaced);

  displaced_step_restore (displaced, displaced->step_ptid);

  /* Fixup may need to read memory/registers.  Switch to the thread
     that we're fixing up.  Also, target_stopped_by_watchpoint checks
     the current thread.  */
  switch_to_thread (event_ptid);

  /* Did the instruction complete successfully?  */
  if (signal == GDB_SIGNAL_TRAP
      && !(target_stopped_by_watchpoint ()
	   && (gdbarch_have_nonsteppable_watchpoint (displaced->step_gdbarch)
	       || target_have_steppable_watchpoint)))
    {
      /* Fix up the resulting state.  */
      gdbarch_displaced_step_fixup (displaced->step_gdbarch,
                                    displaced->step_closure,
                                    displaced->step_original,
                                    displaced->step_copy,
                                    get_thread_regcache (displaced->step_ptid));
      ret = 1;
    }
  else
    {
      /* Since the instruction didn't complete, all we can do is
         relocate the PC.  */
      struct regcache *regcache = get_thread_regcache (event_ptid);
      CORE_ADDR pc = regcache_read_pc (regcache);

      pc = displaced->step_original + (pc - displaced->step_copy);
      regcache_write_pc (regcache, pc);
      ret = -1;
    }

  do_cleanups (old_cleanups);

  displaced->step_ptid = null_ptid;

  return ret;
}

/* Data to be passed around while handling an event.  This data is
   discarded between events.  */
struct execution_control_state
{
  ptid_t ptid;
  /* The thread that got the event, if this was a thread event; NULL
     otherwise.  */
  struct thread_info *event_thread;

  struct target_waitstatus ws;
  int stop_func_filled_in;
  CORE_ADDR stop_func_start;
  CORE_ADDR stop_func_end;
  const char *stop_func_name;
  int wait_some_more;

  /* True if the event thread hit the single-step breakpoint of
     another thread.  Thus the event doesn't cause a stop, the thread
     needs to be single-stepped past the single-step breakpoint before
     we can switch back to the original stepping thread.  */
  int hit_singlestep_breakpoint;
};

/* Clear ECS and set it to point at TP.  */

static void
reset_ecs (struct execution_control_state *ecs, struct thread_info *tp)
{
  memset (ecs, 0, sizeof (*ecs));
  ecs->event_thread = tp;
  ecs->ptid = tp->ptid;
}

static void keep_going_pass_signal (struct execution_control_state *ecs);
static void prepare_to_wait (struct execution_control_state *ecs);
static int keep_going_stepped_thread (struct thread_info *tp);
static step_over_what thread_still_needs_step_over (struct thread_info *tp);

/* Are there any pending step-over requests?  If so, run all we can
   now and return true.  Otherwise, return false.  */

static int
start_step_over (void)
{
  struct thread_info *tp, *next;

  /* Don't start a new step-over if we already have an in-line
     step-over operation ongoing.  */
  if (step_over_info_valid_p ())
    return 0;

  for (tp = step_over_queue_head; tp != NULL; tp = next)
    {
      struct execution_control_state ecss;
      struct execution_control_state *ecs = &ecss;
      step_over_what step_what;
      int must_be_in_line;

      next = thread_step_over_chain_next (tp);

      /* If this inferior already has a displaced step in process,
	 don't start a new one.  */
      if (displaced_step_in_progress (ptid_get_pid (tp->ptid)))
	continue;

      step_what = thread_still_needs_step_over (tp);
      must_be_in_line = ((step_what & STEP_OVER_WATCHPOINT)
			 || ((step_what & STEP_OVER_BREAKPOINT)
			     && !use_displaced_stepping (tp)));

      /* We currently stop all threads of all processes to step-over
	 in-line.  If we need to start a new in-line step-over, let
	 any pending displaced steps finish first.  */
      if (must_be_in_line && displaced_step_in_progress_any_inferior ())
	return 0;

      thread_step_over_chain_remove (tp);

      if (step_over_queue_head == NULL)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: step-over queue now empty\n");
	}

      if (tp->control.trap_expected
	  || tp->resumed
	  || tp->executing)
	{
	  internal_error (__FILE__, __LINE__,
			  "[%s] has inconsistent state: "
			  "trap_expected=%d, resumed=%d, executing=%d\n",
			  target_pid_to_str (tp->ptid),
			  tp->control.trap_expected,
			  tp->resumed,
			  tp->executing);
	}

      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: resuming [%s] for step-over\n",
			    target_pid_to_str (tp->ptid));

      /* keep_going_pass_signal skips the step-over if the breakpoint
	 is no longer inserted.  In all-stop, we want to keep looking
	 for a thread that needs a step-over instead of resuming TP,
	 because we wouldn't be able to resume anything else until the
	 target stops again.  In non-stop, the resume always resumes
	 only TP, so it's OK to let the thread resume freely.  */
      if (!target_is_non_stop_p () && !step_what)
	continue;

      switch_to_thread (tp->ptid);
      reset_ecs (ecs, tp);
      keep_going_pass_signal (ecs);

      if (!ecs->wait_some_more)
	error (_("Command aborted."));

      gdb_assert (tp->resumed);

      /* If we started a new in-line step-over, we're done.  */
      if (step_over_info_valid_p ())
	{
	  gdb_assert (tp->control.trap_expected);
	  return 1;
	}

      if (!target_is_non_stop_p ())
	{
	  /* On all-stop, shouldn't have resumed unless we needed a
	     step over.  */
	  gdb_assert (tp->control.trap_expected
		      || tp->step_after_step_resume_breakpoint);

	  /* With remote targets (at least), in all-stop, we can't
	     issue any further remote commands until the program stops
	     again.  */
	  return 1;
	}

      /* Either the thread no longer needed a step-over, or a new
	 displaced stepping sequence started.  Even in the latter
	 case, continue looking.  Maybe we can also start another
	 displaced step on a thread of other process. */
    }

  return 0;
}

/* Update global variables holding ptids to hold NEW_PTID if they were
   holding OLD_PTID.  */
static void
infrun_thread_ptid_changed (ptid_t old_ptid, ptid_t new_ptid)
{
  struct displaced_step_inferior_state *displaced;

  if (ptid_equal (inferior_ptid, old_ptid))
    inferior_ptid = new_ptid;

  for (displaced = displaced_step_inferior_states;
       displaced;
       displaced = displaced->next)
    {
      if (ptid_equal (displaced->step_ptid, old_ptid))
	displaced->step_ptid = new_ptid;
    }
}


/* Resuming.  */

/* Things to clean up if we QUIT out of resume ().  */
static void
resume_cleanups (void *ignore)
{
  if (!ptid_equal (inferior_ptid, null_ptid))
    delete_single_step_breakpoints (inferior_thread ());

  normal_stop ();
}

static const char schedlock_off[] = "off";
static const char schedlock_on[] = "on";
static const char schedlock_step[] = "step";
static const char schedlock_replay[] = "replay";
static const char *const scheduler_enums[] = {
  schedlock_off,
  schedlock_on,
  schedlock_step,
  schedlock_replay,
  NULL
};
static const char *scheduler_mode = schedlock_replay;
static void
show_scheduler_mode (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Mode for locking scheduler "
		      "during execution is \"%s\".\n"),
		    value);
}

static void
set_schedlock_func (char *args, int from_tty, struct cmd_list_element *c)
{
  if (!target_can_lock_scheduler)
    {
      scheduler_mode = schedlock_off;
      error (_("Target '%s' cannot support this command."), target_shortname);
    }
}

/* True if execution commands resume all threads of all processes by
   default; otherwise, resume only threads of the current inferior
   process.  */
int sched_multi = 0;

/* Try to setup for software single stepping over the specified location.
   Return 1 if target_resume() should use hardware single step.

   GDBARCH the current gdbarch.
   PC the location to step over.  */

static int
maybe_software_singlestep (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  int hw_step = 1;

  if (execution_direction == EXEC_FORWARD
      && gdbarch_software_single_step_p (gdbarch)
      && gdbarch_software_single_step (gdbarch, get_current_frame ()))
    {
      hw_step = 0;
    }
  return hw_step;
}

/* See infrun.h.  */

ptid_t
user_visible_resume_ptid (int step)
{
  ptid_t resume_ptid;

  if (non_stop)
    {
      /* With non-stop mode on, threads are always handled
	 individually.  */
      resume_ptid = inferior_ptid;
    }
  else if ((scheduler_mode == schedlock_on)
	   || (scheduler_mode == schedlock_step && step))
    {
      /* User-settable 'scheduler' mode requires solo thread
	 resume.  */
      resume_ptid = inferior_ptid;
    }
  else if ((scheduler_mode == schedlock_replay)
	   && target_record_will_replay (minus_one_ptid, execution_direction))
    {
      /* User-settable 'scheduler' mode requires solo thread resume in replay
	 mode.  */
      resume_ptid = inferior_ptid;
    }
  else if (!sched_multi && target_supports_multi_process ())
    {
      /* Resume all threads of the current process (and none of other
	 processes).  */
      resume_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));
    }
  else
    {
      /* Resume all threads of all processes.  */
      resume_ptid = RESUME_ALL;
    }

  return resume_ptid;
}

/* Return a ptid representing the set of threads that we will resume,
   in the perspective of the target, assuming run control handling
   does not require leaving some threads stopped (e.g., stepping past
   breakpoint).  USER_STEP indicates whether we're about to start the
   target for a stepping command.  */

static ptid_t
internal_resume_ptid (int user_step)
{
  /* In non-stop, we always control threads individually.  Note that
     the target may always work in non-stop mode even with "set
     non-stop off", in which case user_visible_resume_ptid could
     return a wildcard ptid.  */
  if (target_is_non_stop_p ())
    return inferior_ptid;
  else
    return user_visible_resume_ptid (user_step);
}

/* Wrapper for target_resume, that handles infrun-specific
   bookkeeping.  */

static void
do_target_resume (ptid_t resume_ptid, int step, enum gdb_signal sig)
{
  struct thread_info *tp = inferior_thread ();

  /* Install inferior's terminal modes.  */
  target_terminal_inferior ();

  /* Avoid confusing the next resume, if the next stop/resume
     happens to apply to another thread.  */
  tp->suspend.stop_signal = GDB_SIGNAL_0;

  /* Advise target which signals may be handled silently.

     If we have removed breakpoints because we are stepping over one
     in-line (in any thread), we need to receive all signals to avoid
     accidentally skipping a breakpoint during execution of a signal
     handler.

     Likewise if we're displaced stepping, otherwise a trap for a
     breakpoint in a signal handler might be confused with the
     displaced step finishing.  We don't make the displaced_step_fixup
     step distinguish the cases instead, because:

     - a backtrace while stopped in the signal handler would show the
       scratch pad as frame older than the signal handler, instead of
       the real mainline code.

     - when the thread is later resumed, the signal handler would
       return to the scratch pad area, which would no longer be
       valid.  */
  if (step_over_info_valid_p ()
      || displaced_step_in_progress (ptid_get_pid (tp->ptid)))
    target_pass_signals (0, NULL);
  else
    target_pass_signals ((int) GDB_SIGNAL_LAST, signal_pass);

  target_resume (resume_ptid, step, sig);
}

/* Resume the inferior, but allow a QUIT.  This is useful if the user
   wants to interrupt some lengthy single-stepping operation
   (for child processes, the SIGINT goes to the inferior, and so
   we get a SIGINT random_signal, but for remote debugging and perhaps
   other targets, that's not true).

   SIG is the signal to give the inferior (zero for none).  */
void
resume (enum gdb_signal sig)
{
  struct cleanup *old_cleanups = make_cleanup (resume_cleanups, 0);
  struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct thread_info *tp = inferior_thread ();
  CORE_ADDR pc = regcache_read_pc (regcache);
  struct address_space *aspace = get_regcache_aspace (regcache);
  ptid_t resume_ptid;
  /* This represents the user's step vs continue request.  When
     deciding whether "set scheduler-locking step" applies, it's the
     user's intention that counts.  */
  const int user_step = tp->control.stepping_command;
  /* This represents what we'll actually request the target to do.
     This can decay from a step to a continue, if e.g., we need to
     implement single-stepping with breakpoints (software
     single-step).  */
  int step;

  gdb_assert (!thread_is_in_step_over_chain (tp));

  QUIT;

  if (tp->suspend.waitstatus_pending_p)
    {
      if (debug_infrun)
	{
	  char *statstr;

	  statstr = target_waitstatus_to_string (&tp->suspend.waitstatus);
	  fprintf_unfiltered (gdb_stdlog,
			      "infrun: resume: thread %s has pending wait status %s "
			      "(currently_stepping=%d).\n",
			      target_pid_to_str (tp->ptid),  statstr,
			      currently_stepping (tp));
	  xfree (statstr);
	}

      tp->resumed = 1;

      /* FIXME: What should we do if we are supposed to resume this
	 thread with a signal?  Maybe we should maintain a queue of
	 pending signals to deliver.  */
      if (sig != GDB_SIGNAL_0)
	{
	  warning (_("Couldn't deliver signal %s to %s."),
		   gdb_signal_to_name (sig), target_pid_to_str (tp->ptid));
	}

      tp->suspend.stop_signal = GDB_SIGNAL_0;
      discard_cleanups (old_cleanups);

      if (target_can_async_p ())
	target_async (1);
      return;
    }

  tp->stepped_breakpoint = 0;

  /* Depends on stepped_breakpoint.  */
  step = currently_stepping (tp);

  if (current_inferior ()->waiting_for_vfork_done)
    {
      /* Don't try to single-step a vfork parent that is waiting for
	 the child to get out of the shared memory region (by exec'ing
	 or exiting).  This is particularly important on software
	 single-step archs, as the child process would trip on the
	 software single step breakpoint inserted for the parent
	 process.  Since the parent will not actually execute any
	 instruction until the child is out of the shared region (such
	 are vfork's semantics), it is safe to simply continue it.
	 Eventually, we'll see a TARGET_WAITKIND_VFORK_DONE event for
	 the parent, and tell it to `keep_going', which automatically
	 re-sets it stepping.  */
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: resume : clear step\n");
      step = 0;
    }

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: resume (step=%d, signal=%s), "
			"trap_expected=%d, current thread [%s] at %s\n",
			step, gdb_signal_to_symbol_string (sig),
			tp->control.trap_expected,
			target_pid_to_str (inferior_ptid),
			paddress (gdbarch, pc));

  /* Normally, by the time we reach `resume', the breakpoints are either
     removed or inserted, as appropriate.  The exception is if we're sitting
     at a permanent breakpoint; we need to step over it, but permanent
     breakpoints can't be removed.  So we have to test for it here.  */
  if (breakpoint_here_p (aspace, pc) == permanent_breakpoint_here)
    {
      if (sig != GDB_SIGNAL_0)
	{
	  /* We have a signal to pass to the inferior.  The resume
	     may, or may not take us to the signal handler.  If this
	     is a step, we'll need to stop in the signal handler, if
	     there's one, (if the target supports stepping into
	     handlers), or in the next mainline instruction, if
	     there's no handler.  If this is a continue, we need to be
	     sure to run the handler with all breakpoints inserted.
	     In all cases, set a breakpoint at the current address
	     (where the handler returns to), and once that breakpoint
	     is hit, resume skipping the permanent breakpoint.  If
	     that breakpoint isn't hit, then we've stepped into the
	     signal handler (or hit some other event).  We'll delete
	     the step-resume breakpoint then.  */

	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: resume: skipping permanent breakpoint, "
				"deliver signal first\n");

	  clear_step_over_info ();
	  tp->control.trap_expected = 0;

	  if (tp->control.step_resume_breakpoint == NULL)
	    {
	      /* Set a "high-priority" step-resume, as we don't want
		 user breakpoints at PC to trigger (again) when this
		 hits.  */
	      insert_hp_step_resume_breakpoint_at_frame (get_current_frame ());
	      gdb_assert (tp->control.step_resume_breakpoint->loc->permanent);

	      tp->step_after_step_resume_breakpoint = step;
	    }

	  insert_breakpoints ();
	}
      else
	{
	  /* There's no signal to pass, we can go ahead and skip the
	     permanent breakpoint manually.  */
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: resume: skipping permanent breakpoint\n");
	  gdbarch_skip_permanent_breakpoint (gdbarch, regcache);
	  /* Update pc to reflect the new address from which we will
	     execute instructions.  */
	  pc = regcache_read_pc (regcache);

	  if (step)
	    {
	      /* We've already advanced the PC, so the stepping part
		 is done.  Now we need to arrange for a trap to be
		 reported to handle_inferior_event.  Set a breakpoint
		 at the current PC, and run to it.  Don't update
		 prev_pc, because if we end in
		 switch_back_to_stepped_thread, we want the "expected
		 thread advanced also" branch to be taken.  IOW, we
		 don't want this thread to step further from PC
		 (overstep).  */
	      gdb_assert (!step_over_info_valid_p ());
	      insert_single_step_breakpoint (gdbarch, aspace, pc);
	      insert_breakpoints ();

	      resume_ptid = internal_resume_ptid (user_step);
	      do_target_resume (resume_ptid, 0, GDB_SIGNAL_0);
	      discard_cleanups (old_cleanups);
	      tp->resumed = 1;
	      return;
	    }
	}
    }

  /* If we have a breakpoint to step over, make sure to do a single
     step only.  Same if we have software watchpoints.  */
  if (tp->control.trap_expected || bpstat_should_step ())
    tp->control.may_range_step = 0;

  /* If enabled, step over breakpoints by executing a copy of the
     instruction at a different address.

     We can't use displaced stepping when we have a signal to deliver;
     the comments for displaced_step_prepare explain why.  The
     comments in the handle_inferior event for dealing with 'random
     signals' explain what we do instead.

     We can't use displaced stepping when we are waiting for vfork_done
     event, displaced stepping breaks the vfork child similarly as single
     step software breakpoint.  */
  if (tp->control.trap_expected
      && use_displaced_stepping (tp)
      && !step_over_info_valid_p ()
      && sig == GDB_SIGNAL_0
      && !current_inferior ()->waiting_for_vfork_done)
    {
      int prepared = displaced_step_prepare (inferior_ptid);

      if (prepared == 0)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"Got placed in step-over queue\n");

	  tp->control.trap_expected = 0;
	  discard_cleanups (old_cleanups);
	  return;
	}
      else if (prepared < 0)
	{
	  /* Fallback to stepping over the breakpoint in-line.  */

	  if (target_is_non_stop_p ())
	    stop_all_threads ();

	  set_step_over_info (get_regcache_aspace (regcache),
			      regcache_read_pc (regcache), 0, tp->global_num);

	  step = maybe_software_singlestep (gdbarch, pc);

	  insert_breakpoints ();
	}
      else if (prepared > 0)
	{
	  struct displaced_step_inferior_state *displaced;

	  /* Update pc to reflect the new address from which we will
	     execute instructions due to displaced stepping.  */
	  pc = regcache_read_pc (get_thread_regcache (inferior_ptid));

	  displaced = get_displaced_stepping_state (ptid_get_pid (inferior_ptid));
	  step = gdbarch_displaced_step_hw_singlestep (gdbarch,
						       displaced->step_closure);
	}
    }

  /* Do we need to do it the hard way, w/temp breakpoints?  */
  else if (step)
    step = maybe_software_singlestep (gdbarch, pc);

  /* Currently, our software single-step implementation leads to different
     results than hardware single-stepping in one situation: when stepping
     into delivering a signal which has an associated signal handler,
     hardware single-step will stop at the first instruction of the handler,
     while software single-step will simply skip execution of the handler.

     For now, this difference in behavior is accepted since there is no
     easy way to actually implement single-stepping into a signal handler
     without kernel support.

     However, there is one scenario where this difference leads to follow-on
     problems: if we're stepping off a breakpoint by removing all breakpoints
     and then single-stepping.  In this case, the software single-step
     behavior means that even if there is a *breakpoint* in the signal
     handler, GDB still would not stop.

     Fortunately, we can at least fix this particular issue.  We detect
     here the case where we are about to deliver a signal while software
     single-stepping with breakpoints removed.  In this situation, we
     revert the decisions to remove all breakpoints and insert single-
     step breakpoints, and instead we install a step-resume breakpoint
     at the current address, deliver the signal without stepping, and
     once we arrive back at the step-resume breakpoint, actually step
     over the breakpoint we originally wanted to step over.  */
  if (thread_has_single_step_breakpoints_set (tp)
      && sig != GDB_SIGNAL_0
      && step_over_info_valid_p ())
    {
      /* If we have nested signals or a pending signal is delivered
	 immediately after a handler returns, might might already have
	 a step-resume breakpoint set on the earlier handler.  We cannot
	 set another step-resume breakpoint; just continue on until the
	 original breakpoint is hit.  */
      if (tp->control.step_resume_breakpoint == NULL)
	{
	  insert_hp_step_resume_breakpoint_at_frame (get_current_frame ());
	  tp->step_after_step_resume_breakpoint = 1;
	}

      delete_single_step_breakpoints (tp);

      clear_step_over_info ();
      tp->control.trap_expected = 0;

      insert_breakpoints ();
    }

  /* If STEP is set, it's a request to use hardware stepping
     facilities.  But in that case, we should never
     use singlestep breakpoint.  */
  gdb_assert (!(thread_has_single_step_breakpoints_set (tp) && step));

  /* Decide the set of threads to ask the target to resume.  */
  if (tp->control.trap_expected)
    {
      /* We're allowing a thread to run past a breakpoint it has
	 hit, either by single-stepping the thread with the breakpoint
	 removed, or by displaced stepping, with the breakpoint inserted.
	 In the former case, we need to single-step only this thread,
	 and keep others stopped, as they can miss this breakpoint if
	 allowed to run.  That's not really a problem for displaced
	 stepping, but, we still keep other threads stopped, in case
	 another thread is also stopped for a breakpoint waiting for
	 its turn in the displaced stepping queue.  */
      resume_ptid = inferior_ptid;
    }
  else
    resume_ptid = internal_resume_ptid (user_step);

  if (execution_direction != EXEC_REVERSE
      && step && breakpoint_inserted_here_p (aspace, pc))
    {
      /* There are two cases where we currently need to step a
	 breakpoint instruction when we have a signal to deliver:

	 - See handle_signal_stop where we handle random signals that
	 could take out us out of the stepping range.  Normally, in
	 that case we end up continuing (instead of stepping) over the
	 signal handler with a breakpoint at PC, but there are cases
	 where we should _always_ single-step, even if we have a
	 step-resume breakpoint, like when a software watchpoint is
	 set.  Assuming single-stepping and delivering a signal at the
	 same time would takes us to the signal handler, then we could
	 have removed the breakpoint at PC to step over it.  However,
	 some hardware step targets (like e.g., Mac OS) can't step
	 into signal handlers, and for those, we need to leave the
	 breakpoint at PC inserted, as otherwise if the handler
	 recurses and executes PC again, it'll miss the breakpoint.
	 So we leave the breakpoint inserted anyway, but we need to
	 record that we tried to step a breakpoint instruction, so
	 that adjust_pc_after_break doesn't end up confused.

         - In non-stop if we insert a breakpoint (e.g., a step-resume)
	 in one thread after another thread that was stepping had been
	 momentarily paused for a step-over.  When we re-resume the
	 stepping thread, it may be resumed from that address with a
	 breakpoint that hasn't trapped yet.  Seen with
	 gdb.threads/non-stop-fair-events.exp, on targets that don't
	 do displaced stepping.  */

      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: resume: [%s] stepped breakpoint\n",
			    target_pid_to_str (tp->ptid));

      tp->stepped_breakpoint = 1;

      /* Most targets can step a breakpoint instruction, thus
	 executing it normally.  But if this one cannot, just
	 continue and we will hit it anyway.  */
      if (gdbarch_cannot_step_breakpoint (gdbarch))
	step = 0;
    }

  if (debug_displaced
      && tp->control.trap_expected
      && use_displaced_stepping (tp)
      && !step_over_info_valid_p ())
    {
      struct regcache *resume_regcache = get_thread_regcache (tp->ptid);
      struct gdbarch *resume_gdbarch = get_regcache_arch (resume_regcache);
      CORE_ADDR actual_pc = regcache_read_pc (resume_regcache);
      gdb_byte buf[4];

      fprintf_unfiltered (gdb_stdlog, "displaced: run %s: ",
			  paddress (resume_gdbarch, actual_pc));
      read_memory (actual_pc, buf, sizeof (buf));
      displaced_step_dump_bytes (gdb_stdlog, buf, sizeof (buf));
    }

  if (tp->control.may_range_step)
    {
      /* If we're resuming a thread with the PC out of the step
	 range, then we're doing some nested/finer run control
	 operation, like stepping the thread out of the dynamic
	 linker or the displaced stepping scratch pad.  We
	 shouldn't have allowed a range step then.  */
      gdb_assert (pc_in_thread_step_range (pc, tp));
    }

  do_target_resume (resume_ptid, step, sig);
  tp->resumed = 1;
  discard_cleanups (old_cleanups);
}

/* Proceeding.  */

/* See infrun.h.  */

/* Counter that tracks number of user visible stops.  This can be used
   to tell whether a command has proceeded the inferior past the
   current location.  This allows e.g., inferior function calls in
   breakpoint commands to not interrupt the command list.  When the
   call finishes successfully, the inferior is standing at the same
   breakpoint as if nothing happened (and so we don't call
   normal_stop).  */
static ULONGEST current_stop_id;

/* See infrun.h.  */

ULONGEST
get_stop_id (void)
{
  return current_stop_id;
}

/* Called when we report a user visible stop.  */

static void
new_stop_id (void)
{
  current_stop_id++;
}

/* Clear out all variables saying what to do when inferior is continued.
   First do this, then set the ones you want, then call `proceed'.  */

static void
clear_proceed_status_thread (struct thread_info *tp)
{
  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: clear_proceed_status_thread (%s)\n",
			target_pid_to_str (tp->ptid));

  /* If we're starting a new sequence, then the previous finished
     single-step is no longer relevant.  */
  if (tp->suspend.waitstatus_pending_p)
    {
      if (tp->suspend.stop_reason == TARGET_STOPPED_BY_SINGLE_STEP)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: clear_proceed_status: pending "
				"event of %s was a finished step. "
				"Discarding.\n",
				target_pid_to_str (tp->ptid));

	  tp->suspend.waitstatus_pending_p = 0;
	  tp->suspend.stop_reason = TARGET_STOPPED_BY_NO_REASON;
	}
      else if (debug_infrun)
	{
	  char *statstr;

	  statstr = target_waitstatus_to_string (&tp->suspend.waitstatus);
	  fprintf_unfiltered (gdb_stdlog,
			      "infrun: clear_proceed_status_thread: thread %s "
			      "has pending wait status %s "
			      "(currently_stepping=%d).\n",
			      target_pid_to_str (tp->ptid), statstr,
			      currently_stepping (tp));
	  xfree (statstr);
	}
    }

  /* If this signal should not be seen by program, give it zero.
     Used for debugging signals.  */
  if (!signal_pass_state (tp->suspend.stop_signal))
    tp->suspend.stop_signal = GDB_SIGNAL_0;

  thread_fsm_delete (tp->thread_fsm);
  tp->thread_fsm = NULL;

  tp->control.trap_expected = 0;
  tp->control.step_range_start = 0;
  tp->control.step_range_end = 0;
  tp->control.may_range_step = 0;
  tp->control.step_frame_id = null_frame_id;
  tp->control.step_stack_frame_id = null_frame_id;
  tp->control.step_over_calls = STEP_OVER_UNDEBUGGABLE;
  tp->control.step_start_function = NULL;
  tp->stop_requested = 0;

  tp->control.stop_step = 0;

  tp->control.proceed_to_finish = 0;

  tp->control.stepping_command = 0;

  /* Discard any remaining commands or status from previous stop.  */
  bpstat_clear (&tp->control.stop_bpstat);
}

void
clear_proceed_status (int step)
{
  /* With scheduler-locking replay, stop replaying other threads if we're
     not replaying the user-visible resume ptid.

     This is a convenience feature to not require the user to explicitly
     stop replaying the other threads.  We're assuming that the user's
     intent is to resume tracing the recorded process.  */
  if (!non_stop && scheduler_mode == schedlock_replay
      && target_record_is_replaying (minus_one_ptid)
      && !target_record_will_replay (user_visible_resume_ptid (step),
				     execution_direction))
    target_record_stop_replaying ();

  if (!non_stop)
    {
      struct thread_info *tp;
      ptid_t resume_ptid;

      resume_ptid = user_visible_resume_ptid (step);

      /* In all-stop mode, delete the per-thread status of all threads
	 we're about to resume, implicitly and explicitly.  */
      ALL_NON_EXITED_THREADS (tp)
        {
	  if (!ptid_match (tp->ptid, resume_ptid))
	    continue;
	  clear_proceed_status_thread (tp);
	}
    }

  if (!ptid_equal (inferior_ptid, null_ptid))
    {
      struct inferior *inferior;

      if (non_stop)
	{
	  /* If in non-stop mode, only delete the per-thread status of
	     the current thread.  */
	  clear_proceed_status_thread (inferior_thread ());
	}

      inferior = current_inferior ();
      inferior->control.stop_soon = NO_STOP_QUIETLY;
    }

  observer_notify_about_to_proceed ();
}

/* Returns true if TP is still stopped at a breakpoint that needs
   stepping-over in order to make progress.  If the breakpoint is gone
   meanwhile, we can skip the whole step-over dance.  */

static int
thread_still_needs_step_over_bp (struct thread_info *tp)
{
  if (tp->stepping_over_breakpoint)
    {
      struct regcache *regcache = get_thread_regcache (tp->ptid);

      if (breakpoint_here_p (get_regcache_aspace (regcache),
			     regcache_read_pc (regcache))
	  == ordinary_breakpoint_here)
	return 1;

      tp->stepping_over_breakpoint = 0;
    }

  return 0;
}

/* Check whether thread TP still needs to start a step-over in order
   to make progress when resumed.  Returns an bitwise or of enum
   step_over_what bits, indicating what needs to be stepped over.  */

static step_over_what
thread_still_needs_step_over (struct thread_info *tp)
{
  step_over_what what = 0;

  if (thread_still_needs_step_over_bp (tp))
    what |= STEP_OVER_BREAKPOINT;

  if (tp->stepping_over_watchpoint
      && !target_have_steppable_watchpoint)
    what |= STEP_OVER_WATCHPOINT;

  return what;
}

/* Returns true if scheduler locking applies.  STEP indicates whether
   we're about to do a step/next-like command to a thread.  */

static int
schedlock_applies (struct thread_info *tp)
{
  return (scheduler_mode == schedlock_on
	  || (scheduler_mode == schedlock_step
	      && tp->control.stepping_command)
	  || (scheduler_mode == schedlock_replay
	      && target_record_will_replay (minus_one_ptid,
					    execution_direction)));
}

/* Basic routine for continuing the program in various fashions.

   ADDR is the address to resume at, or -1 for resume where stopped.
   SIGGNAL is the signal to give it, or 0 for none,
   or -1 for act according to how it stopped.
   STEP is nonzero if should trap after one instruction.
   -1 means return after that and print nothing.
   You should probably set various step_... variables
   before calling here, if you are stepping.

   You should call clear_proceed_status before calling proceed.  */

void
proceed (CORE_ADDR addr, enum gdb_signal siggnal)
{
  struct regcache *regcache;
  struct gdbarch *gdbarch;
  struct thread_info *tp;
  CORE_ADDR pc;
  struct address_space *aspace;
  ptid_t resume_ptid;
  struct execution_control_state ecss;
  struct execution_control_state *ecs = &ecss;
  struct cleanup *old_chain;
  int started;

  /* If we're stopped at a fork/vfork, follow the branch set by the
     "set follow-fork-mode" command; otherwise, we'll just proceed
     resuming the current thread.  */
  if (!follow_fork ())
    {
      /* The target for some reason decided not to resume.  */
      normal_stop ();
      if (target_can_async_p ())
	inferior_event_handler (INF_EXEC_COMPLETE, NULL);
      return;
    }

  /* We'll update this if & when we switch to a new thread.  */
  previous_inferior_ptid = inferior_ptid;

  regcache = get_current_regcache ();
  gdbarch = get_regcache_arch (regcache);
  aspace = get_regcache_aspace (regcache);
  pc = regcache_read_pc (regcache);
  tp = inferior_thread ();

  /* Fill in with reasonable starting values.  */
  init_thread_stepping_state (tp);

  gdb_assert (!thread_is_in_step_over_chain (tp));

  if (addr == (CORE_ADDR) -1)
    {
      if (pc == stop_pc
	  && breakpoint_here_p (aspace, pc) == ordinary_breakpoint_here
	  && execution_direction != EXEC_REVERSE)
	/* There is a breakpoint at the address we will resume at,
	   step one instruction before inserting breakpoints so that
	   we do not stop right away (and report a second hit at this
	   breakpoint).

	   Note, we don't do this in reverse, because we won't
	   actually be executing the breakpoint insn anyway.
	   We'll be (un-)executing the previous instruction.  */
	tp->stepping_over_breakpoint = 1;
      else if (gdbarch_single_step_through_delay_p (gdbarch)
	       && gdbarch_single_step_through_delay (gdbarch,
						     get_current_frame ()))
	/* We stepped onto an instruction that needs to be stepped
	   again before re-inserting the breakpoint, do so.  */
	tp->stepping_over_breakpoint = 1;
    }
  else
    {
      regcache_write_pc (regcache, addr);
    }

  if (siggnal != GDB_SIGNAL_DEFAULT)
    tp->suspend.stop_signal = siggnal;

  resume_ptid = user_visible_resume_ptid (tp->control.stepping_command);

  /* If an exception is thrown from this point on, make sure to
     propagate GDB's knowledge of the executing state to the
     frontend/user running state.  */
  old_chain = make_cleanup (finish_thread_state_cleanup, &resume_ptid);

  /* Even if RESUME_PTID is a wildcard, and we end up resuming fewer
     threads (e.g., we might need to set threads stepping over
     breakpoints first), from the user/frontend's point of view, all
     threads in RESUME_PTID are now running.  Unless we're calling an
     inferior function, as in that case we pretend the inferior
     doesn't run at all.  */
  if (!tp->control.in_infcall)
   set_running (resume_ptid, 1);

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: proceed (addr=%s, signal=%s)\n",
			paddress (gdbarch, addr),
			gdb_signal_to_symbol_string (siggnal));

  annotate_starting ();

  /* Make sure that output from GDB appears before output from the
     inferior.  */
  gdb_flush (gdb_stdout);

  /* In a multi-threaded task we may select another thread and
     then continue or step.

     But if a thread that we're resuming had stopped at a breakpoint,
     it will immediately cause another breakpoint stop without any
     execution (i.e. it will report a breakpoint hit incorrectly).  So
     we must step over it first.

     Look for threads other than the current (TP) that reported a
     breakpoint hit and haven't been resumed yet since.  */

  /* If scheduler locking applies, we can avoid iterating over all
     threads.  */
  if (!non_stop && !schedlock_applies (tp))
    {
      struct thread_info *current = tp;

      ALL_NON_EXITED_THREADS (tp)
        {
	  /* Ignore the current thread here.  It's handled
	     afterwards.  */
	  if (tp == current)
	    continue;

	  /* Ignore threads of processes we're not resuming.  */
	  if (!ptid_match (tp->ptid, resume_ptid))
	    continue;

	  if (!thread_still_needs_step_over (tp))
	    continue;

	  gdb_assert (!thread_is_in_step_over_chain (tp));

	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: need to step-over [%s] first\n",
				target_pid_to_str (tp->ptid));

	  thread_step_over_chain_enqueue (tp);
	}

      tp = current;
    }

  /* Enqueue the current thread last, so that we move all other
     threads over their breakpoints first.  */
  if (tp->stepping_over_breakpoint)
    thread_step_over_chain_enqueue (tp);

  /* If the thread isn't started, we'll still need to set its prev_pc,
     so that switch_back_to_stepped_thread knows the thread hasn't
     advanced.  Must do this before resuming any thread, as in
     all-stop/remote, once we resume we can't send any other packet
     until the target stops again.  */
  tp->prev_pc = regcache_read_pc (regcache);

  started = start_step_over ();

  if (step_over_info_valid_p ())
    {
      /* Either this thread started a new in-line step over, or some
	 other thread was already doing one.  In either case, don't
	 resume anything else until the step-over is finished.  */
    }
  else if (started && !target_is_non_stop_p ())
    {
      /* A new displaced stepping sequence was started.  In all-stop,
	 we can't talk to the target anymore until it next stops.  */
    }
  else if (!non_stop && target_is_non_stop_p ())
    {
      /* In all-stop, but the target is always in non-stop mode.
	 Start all other threads that are implicitly resumed too.  */
      ALL_NON_EXITED_THREADS (tp)
        {
	  /* Ignore threads of processes we're not resuming.  */
	  if (!ptid_match (tp->ptid, resume_ptid))
	    continue;

	  if (tp->resumed)
	    {
	      if (debug_infrun)
		fprintf_unfiltered (gdb_stdlog,
				    "infrun: proceed: [%s] resumed\n",
				    target_pid_to_str (tp->ptid));
	      gdb_assert (tp->executing || tp->suspend.waitstatus_pending_p);
	      continue;
	    }

	  if (thread_is_in_step_over_chain (tp))
	    {
	      if (debug_infrun)
		fprintf_unfiltered (gdb_stdlog,
				    "infrun: proceed: [%s] needs step-over\n",
				    target_pid_to_str (tp->ptid));
	      continue;
	    }

	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: proceed: resuming %s\n",
				target_pid_to_str (tp->ptid));

	  reset_ecs (ecs, tp);
	  switch_to_thread (tp->ptid);
	  keep_going_pass_signal (ecs);
	  if (!ecs->wait_some_more)
	    error (_("Command aborted."));
	}
    }
  else if (!tp->resumed && !thread_is_in_step_over_chain (tp))
    {
      /* The thread wasn't started, and isn't queued, run it now.  */
      reset_ecs (ecs, tp);
      switch_to_thread (tp->ptid);
      keep_going_pass_signal (ecs);
      if (!ecs->wait_some_more)
	error (_("Command aborted."));
    }

  discard_cleanups (old_chain);

  /* Tell the event loop to wait for it to stop.  If the target
     supports asynchronous execution, it'll do this from within
     target_resume.  */
  if (!target_can_async_p ())
    mark_async_event_handler (infrun_async_inferior_event_token);
}


/* Start remote-debugging of a machine over a serial link.  */

void
start_remote (int from_tty)
{
  struct inferior *inferior;

  inferior = current_inferior ();
  inferior->control.stop_soon = STOP_QUIETLY_REMOTE;

  /* Always go on waiting for the target, regardless of the mode.  */
  /* FIXME: cagney/1999-09-23: At present it isn't possible to
     indicate to wait_for_inferior that a target should timeout if
     nothing is returned (instead of just blocking).  Because of this,
     targets expecting an immediate response need to, internally, set
     things up so that the target_wait() is forced to eventually
     timeout.  */
  /* FIXME: cagney/1999-09-24: It isn't possible for target_open() to
     differentiate to its caller what the state of the target is after
     the initial open has been performed.  Here we're assuming that
     the target has stopped.  It should be possible to eventually have
     target_open() return to the caller an indication that the target
     is currently running and GDB state should be set to the same as
     for an async run.  */
  wait_for_inferior ();

  /* Now that the inferior has stopped, do any bookkeeping like
     loading shared libraries.  We want to do this before normal_stop,
     so that the displayed frame is up to date.  */
  post_create_inferior (&current_target, from_tty);

  normal_stop ();
}

/* Initialize static vars when a new inferior begins.  */

void
init_wait_for_inferior (void)
{
  /* These are meaningless until the first time through wait_for_inferior.  */

  breakpoint_init_inferior (inf_starting);

  clear_proceed_status (0);

  target_last_wait_ptid = minus_one_ptid;

  previous_inferior_ptid = inferior_ptid;

  /* Discard any skipped inlined frames.  */
  clear_inline_frame_state (minus_one_ptid);
}



static void handle_inferior_event (struct execution_control_state *ecs);

static void handle_step_into_function (struct gdbarch *gdbarch,
				       struct execution_control_state *ecs);
static void handle_step_into_function_backward (struct gdbarch *gdbarch,
						struct execution_control_state *ecs);
static void handle_signal_stop (struct execution_control_state *ecs);
static void check_exception_resume (struct execution_control_state *,
				    struct frame_info *);

static void end_stepping_range (struct execution_control_state *ecs);
static void stop_waiting (struct execution_control_state *ecs);
static void keep_going (struct execution_control_state *ecs);
static void process_event_stop_test (struct execution_control_state *ecs);
static int switch_back_to_stepped_thread (struct execution_control_state *ecs);

/* Callback for iterate over threads.  If the thread is stopped, but
   the user/frontend doesn't know about that yet, go through
   normal_stop, as if the thread had just stopped now.  ARG points at
   a ptid.  If PTID is MINUS_ONE_PTID, applies to all threads.  If
   ptid_is_pid(PTID) is true, applies to all threads of the process
   pointed at by PTID.  Otherwise, apply only to the thread pointed by
   PTID.  */

static int
infrun_thread_stop_requested_callback (struct thread_info *info, void *arg)
{
  ptid_t ptid = * (ptid_t *) arg;

  if ((ptid_equal (info->ptid, ptid)
       || ptid_equal (minus_one_ptid, ptid)
       || (ptid_is_pid (ptid)
	   && ptid_get_pid (ptid) == ptid_get_pid (info->ptid)))
      && is_running (info->ptid)
      && !is_executing (info->ptid))
    {
      struct cleanup *old_chain;
      struct execution_control_state ecss;
      struct execution_control_state *ecs = &ecss;

      memset (ecs, 0, sizeof (*ecs));

      old_chain = make_cleanup_restore_current_thread ();

      overlay_cache_invalid = 1;
      /* Flush target cache before starting to handle each event.
	 Target was running and cache could be stale.  This is just a
	 heuristic.  Running threads may modify target memory, but we
	 don't get any event.  */
      target_dcache_invalidate ();

      /* Go through handle_inferior_event/normal_stop, so we always
	 have consistent output as if the stop event had been
	 reported.  */
      ecs->ptid = info->ptid;
      ecs->event_thread = info;
      ecs->ws.kind = TARGET_WAITKIND_STOPPED;
      ecs->ws.value.sig = GDB_SIGNAL_0;

      handle_inferior_event (ecs);

      if (!ecs->wait_some_more)
	{
	  /* Cancel any running execution command.  */
	  thread_cancel_execution_command (info);

	  normal_stop ();
	}

      do_cleanups (old_chain);
    }

  return 0;
}

/* This function is attached as a "thread_stop_requested" observer.
   Cleanup local state that assumed the PTID was to be resumed, and
   report the stop to the frontend.  */

static void
infrun_thread_stop_requested (ptid_t ptid)
{
  struct thread_info *tp;

  /* PTID was requested to stop.  Remove matching threads from the
     step-over queue, so we don't try to resume them
     automatically.  */
  ALL_NON_EXITED_THREADS (tp)
    if (ptid_match (tp->ptid, ptid))
      {
	if (thread_is_in_step_over_chain (tp))
	  thread_step_over_chain_remove (tp);
      }

  iterate_over_threads (infrun_thread_stop_requested_callback, &ptid);
}

static void
infrun_thread_thread_exit (struct thread_info *tp, int silent)
{
  if (ptid_equal (target_last_wait_ptid, tp->ptid))
    nullify_last_target_wait_ptid ();
}

/* Delete the step resume, single-step and longjmp/exception resume
   breakpoints of TP.  */

static void
delete_thread_infrun_breakpoints (struct thread_info *tp)
{
  delete_step_resume_breakpoint (tp);
  delete_exception_resume_breakpoint (tp);
  delete_single_step_breakpoints (tp);
}

/* If the target still has execution, call FUNC for each thread that
   just stopped.  In all-stop, that's all the non-exited threads; in
   non-stop, that's the current thread, only.  */

typedef void (*for_each_just_stopped_thread_callback_func)
  (struct thread_info *tp);

static void
for_each_just_stopped_thread (for_each_just_stopped_thread_callback_func func)
{
  if (!target_has_execution || ptid_equal (inferior_ptid, null_ptid))
    return;

  if (target_is_non_stop_p ())
    {
      /* If in non-stop mode, only the current thread stopped.  */
      func (inferior_thread ());
    }
  else
    {
      struct thread_info *tp;

      /* In all-stop mode, all threads have stopped.  */
      ALL_NON_EXITED_THREADS (tp)
        {
	  func (tp);
	}
    }
}

/* Delete the step resume and longjmp/exception resume breakpoints of
   the threads that just stopped.  */

static void
delete_just_stopped_threads_infrun_breakpoints (void)
{
  for_each_just_stopped_thread (delete_thread_infrun_breakpoints);
}

/* Delete the single-step breakpoints of the threads that just
   stopped.  */

static void
delete_just_stopped_threads_single_step_breakpoints (void)
{
  for_each_just_stopped_thread (delete_single_step_breakpoints);
}

/* A cleanup wrapper.  */

static void
delete_just_stopped_threads_infrun_breakpoints_cleanup (void *arg)
{
  delete_just_stopped_threads_infrun_breakpoints ();
}

/* See infrun.h.  */

void
print_target_wait_results (ptid_t waiton_ptid, ptid_t result_ptid,
			   const struct target_waitstatus *ws)
{
  char *status_string = target_waitstatus_to_string (ws);
  struct ui_file *tmp_stream = mem_fileopen ();
  char *text;

  /* The text is split over several lines because it was getting too long.
     Call fprintf_unfiltered (gdb_stdlog) once so that the text is still
     output as a unit; we want only one timestamp printed if debug_timestamp
     is set.  */

  fprintf_unfiltered (tmp_stream,
		      "infrun: target_wait (%d.%ld.%ld",
		      ptid_get_pid (waiton_ptid),
		      ptid_get_lwp (waiton_ptid),
		      ptid_get_tid (waiton_ptid));
  if (ptid_get_pid (waiton_ptid) != -1)
    fprintf_unfiltered (tmp_stream,
			" [%s]", target_pid_to_str (waiton_ptid));
  fprintf_unfiltered (tmp_stream, ", status) =\n");
  fprintf_unfiltered (tmp_stream,
		      "infrun:   %d.%ld.%ld [%s],\n",
		      ptid_get_pid (result_ptid),
		      ptid_get_lwp (result_ptid),
		      ptid_get_tid (result_ptid),
		      target_pid_to_str (result_ptid));
  fprintf_unfiltered (tmp_stream,
		      "infrun:   %s\n",
		      status_string);

  text = ui_file_xstrdup (tmp_stream, NULL);

  /* This uses %s in part to handle %'s in the text, but also to avoid
     a gcc error: the format attribute requires a string literal.  */
  fprintf_unfiltered (gdb_stdlog, "%s", text);

  xfree (status_string);
  xfree (text);
  ui_file_delete (tmp_stream);
}

/* Select a thread at random, out of those which are resumed and have
   had events.  */

static struct thread_info *
random_pending_event_thread (ptid_t waiton_ptid)
{
  struct thread_info *event_tp;
  int num_events = 0;
  int random_selector;

  /* First see how many events we have.  Count only resumed threads
     that have an event pending.  */
  ALL_NON_EXITED_THREADS (event_tp)
    if (ptid_match (event_tp->ptid, waiton_ptid)
	&& event_tp->resumed
	&& event_tp->suspend.waitstatus_pending_p)
      num_events++;

  if (num_events == 0)
    return NULL;

  /* Now randomly pick a thread out of those that have had events.  */
  random_selector = (int)
    ((num_events * (double) rand ()) / (RAND_MAX + 1.0));

  if (debug_infrun && num_events > 1)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: Found %d events, selecting #%d\n",
			num_events, random_selector);

  /* Select the Nth thread that has had an event.  */
  ALL_NON_EXITED_THREADS (event_tp)
    if (ptid_match (event_tp->ptid, waiton_ptid)
	&& event_tp->resumed
	&& event_tp->suspend.waitstatus_pending_p)
      if (random_selector-- == 0)
	break;

  return event_tp;
}

/* Wrapper for target_wait that first checks whether threads have
   pending statuses to report before actually asking the target for
   more events.  */

static ptid_t
do_target_wait (ptid_t ptid, struct target_waitstatus *status, int options)
{
  ptid_t event_ptid;
  struct thread_info *tp;

  /* First check if there is a resumed thread with a wait status
     pending.  */
  if (ptid_equal (ptid, minus_one_ptid) || ptid_is_pid (ptid))
    {
      tp = random_pending_event_thread (ptid);
    }
  else
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: Waiting for specific thread %s.\n",
			    target_pid_to_str (ptid));

      /* We have a specific thread to check.  */
      tp = find_thread_ptid (ptid);
      gdb_assert (tp != NULL);
      if (!tp->suspend.waitstatus_pending_p)
	tp = NULL;
    }

  if (tp != NULL
      && (tp->suspend.stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
	  || tp->suspend.stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT))
    {
      struct regcache *regcache = get_thread_regcache (tp->ptid);
      struct gdbarch *gdbarch = get_regcache_arch (regcache);
      CORE_ADDR pc;
      int discard = 0;

      pc = regcache_read_pc (regcache);

      if (pc != tp->suspend.stop_pc)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: PC of %s changed.  was=%s, now=%s\n",
				target_pid_to_str (tp->ptid),
				paddress (gdbarch, tp->prev_pc),
				paddress (gdbarch, pc));
	  discard = 1;
	}
      else if (!breakpoint_inserted_here_p (get_regcache_aspace (regcache), pc))
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: previous breakpoint of %s, at %s gone\n",
				target_pid_to_str (tp->ptid),
				paddress (gdbarch, pc));

	  discard = 1;
	}

      if (discard)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: pending event of %s cancelled.\n",
				target_pid_to_str (tp->ptid));

	  tp->suspend.waitstatus.kind = TARGET_WAITKIND_SPURIOUS;
	  tp->suspend.stop_reason = TARGET_STOPPED_BY_NO_REASON;
	}
    }

  if (tp != NULL)
    {
      if (debug_infrun)
	{
	  char *statstr;

	  statstr = target_waitstatus_to_string (&tp->suspend.waitstatus);
	  fprintf_unfiltered (gdb_stdlog,
			      "infrun: Using pending wait status %s for %s.\n",
			      statstr,
			      target_pid_to_str (tp->ptid));
	  xfree (statstr);
	}

      /* Now that we've selected our final event LWP, un-adjust its PC
	 if it was a software breakpoint (and the target doesn't
	 always adjust the PC itself).  */
      if (tp->suspend.stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
	  && !target_supports_stopped_by_sw_breakpoint ())
	{
	  struct regcache *regcache;
	  struct gdbarch *gdbarch;
	  int decr_pc;

	  regcache = get_thread_regcache (tp->ptid);
	  gdbarch = get_regcache_arch (regcache);

	  decr_pc = gdbarch_decr_pc_after_break (gdbarch);
	  if (decr_pc != 0)
	    {
	      CORE_ADDR pc;

	      pc = regcache_read_pc (regcache);
	      regcache_write_pc (regcache, pc + decr_pc);
	    }
	}

      tp->suspend.stop_reason = TARGET_STOPPED_BY_NO_REASON;
      *status = tp->suspend.waitstatus;
      tp->suspend.waitstatus_pending_p = 0;

      /* Wake up the event loop again, until all pending events are
	 processed.  */
      if (target_is_async_p ())
	mark_async_event_handler (infrun_async_inferior_event_token);
      return tp->ptid;
    }

  /* But if we don't find one, we'll have to wait.  */

  if (deprecated_target_wait_hook)
    event_ptid = deprecated_target_wait_hook (ptid, status, options);
  else
    event_ptid = target_wait (ptid, status, options);

  return event_ptid;
}

/* Prepare and stabilize the inferior for detaching it.  E.g.,
   detaching while a thread is displaced stepping is a recipe for
   crashing it, as nothing would readjust the PC out of the scratch
   pad.  */

void
prepare_for_detach (void)
{
  struct inferior *inf = current_inferior ();
  ptid_t pid_ptid = pid_to_ptid (inf->pid);
  struct cleanup *old_chain_1;
  struct displaced_step_inferior_state *displaced;

  displaced = get_displaced_stepping_state (inf->pid);

  /* Is any thread of this process displaced stepping?  If not,
     there's nothing else to do.  */
  if (displaced == NULL || ptid_equal (displaced->step_ptid, null_ptid))
    return;

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"displaced-stepping in-process while detaching");

  old_chain_1 = make_cleanup_restore_integer (&inf->detaching);
  inf->detaching = 1;

  while (!ptid_equal (displaced->step_ptid, null_ptid))
    {
      struct cleanup *old_chain_2;
      struct execution_control_state ecss;
      struct execution_control_state *ecs;

      ecs = &ecss;
      memset (ecs, 0, sizeof (*ecs));

      overlay_cache_invalid = 1;
      /* Flush target cache before starting to handle each event.
	 Target was running and cache could be stale.  This is just a
	 heuristic.  Running threads may modify target memory, but we
	 don't get any event.  */
      target_dcache_invalidate ();

      ecs->ptid = do_target_wait (pid_ptid, &ecs->ws, 0);

      if (debug_infrun)
	print_target_wait_results (pid_ptid, ecs->ptid, &ecs->ws);

      /* If an error happens while handling the event, propagate GDB's
	 knowledge of the executing state to the frontend/user running
	 state.  */
      old_chain_2 = make_cleanup (finish_thread_state_cleanup,
				  &minus_one_ptid);

      /* Now figure out what to do with the result of the result.  */
      handle_inferior_event (ecs);

      /* No error, don't finish the state yet.  */
      discard_cleanups (old_chain_2);

      /* Breakpoints and watchpoints are not installed on the target
	 at this point, and signals are passed directly to the
	 inferior, so this must mean the process is gone.  */
      if (!ecs->wait_some_more)
	{
	  discard_cleanups (old_chain_1);
	  error (_("Program exited while detaching"));
	}
    }

  discard_cleanups (old_chain_1);
}

/* Wait for control to return from inferior to debugger.

   If inferior gets a signal, we may decide to start it up again
   instead of returning.  That is why there is a loop in this function.
   When this function actually returns it means the inferior
   should be left stopped and GDB should read more commands.  */

void
wait_for_inferior (void)
{
  struct cleanup *old_cleanups;
  struct cleanup *thread_state_chain;

  if (debug_infrun)
    fprintf_unfiltered
      (gdb_stdlog, "infrun: wait_for_inferior ()\n");

  old_cleanups
    = make_cleanup (delete_just_stopped_threads_infrun_breakpoints_cleanup,
		    NULL);

  /* If an error happens while handling the event, propagate GDB's
     knowledge of the executing state to the frontend/user running
     state.  */
  thread_state_chain = make_cleanup (finish_thread_state_cleanup, &minus_one_ptid);

  while (1)
    {
      struct execution_control_state ecss;
      struct execution_control_state *ecs = &ecss;
      ptid_t waiton_ptid = minus_one_ptid;

      memset (ecs, 0, sizeof (*ecs));

      overlay_cache_invalid = 1;

      /* Flush target cache before starting to handle each event.
	 Target was running and cache could be stale.  This is just a
	 heuristic.  Running threads may modify target memory, but we
	 don't get any event.  */
      target_dcache_invalidate ();

      ecs->ptid = do_target_wait (waiton_ptid, &ecs->ws, 0);

      if (debug_infrun)
	print_target_wait_results (waiton_ptid, ecs->ptid, &ecs->ws);

      /* Now figure out what to do with the result of the result.  */
      handle_inferior_event (ecs);

      if (!ecs->wait_some_more)
	break;
    }

  /* No error, don't finish the state yet.  */
  discard_cleanups (thread_state_chain);

  do_cleanups (old_cleanups);
}

/* Cleanup that reinstalls the readline callback handler, if the
   target is running in the background.  If while handling the target
   event something triggered a secondary prompt, like e.g., a
   pagination prompt, we'll have removed the callback handler (see
   gdb_readline_wrapper_line).  Need to do this as we go back to the
   event loop, ready to process further input.  Note this has no
   effect if the handler hasn't actually been removed, because calling
   rl_callback_handler_install resets the line buffer, thus losing
   input.  */

static void
reinstall_readline_callback_handler_cleanup (void *arg)
{
  struct ui *ui = current_ui;

  if (!ui->async)
    {
      /* We're not going back to the top level event loop yet.  Don't
	 install the readline callback, as it'd prep the terminal,
	 readline-style (raw, noecho) (e.g., --batch).  We'll install
	 it the next time the prompt is displayed, when we're ready
	 for input.  */
      return;
    }

  if (ui->command_editing && ui->prompt_state != PROMPT_BLOCKED)
    gdb_rl_callback_handler_reinstall ();
}

/* Clean up the FSMs of threads that are now stopped.  In non-stop,
   that's just the event thread.  In all-stop, that's all threads.  */

static void
clean_up_just_stopped_threads_fsms (struct execution_control_state *ecs)
{
  struct thread_info *thr = ecs->event_thread;

  if (thr != NULL && thr->thread_fsm != NULL)
    thread_fsm_clean_up (thr->thread_fsm, thr);

  if (!non_stop)
    {
      ALL_NON_EXITED_THREADS (thr)
        {
	  if (thr->thread_fsm == NULL)
	    continue;
	  if (thr == ecs->event_thread)
	    continue;

	  switch_to_thread (thr->ptid);
	  thread_fsm_clean_up (thr->thread_fsm, thr);
	}

      if (ecs->event_thread != NULL)
	switch_to_thread (ecs->event_thread->ptid);
    }
}

/* Helper for all_uis_check_sync_execution_done that works on the
   current UI.  */

static void
check_curr_ui_sync_execution_done (void)
{
  struct ui *ui = current_ui;

  if (ui->prompt_state == PROMPT_NEEDED
      && ui->async
      && !gdb_in_secondary_prompt_p (ui))
    {
      target_terminal_ours ();
      observer_notify_sync_execution_done ();
      ui_register_input_event_handler (ui);
    }
}

/* See infrun.h.  */

void
all_uis_check_sync_execution_done (void)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      check_curr_ui_sync_execution_done ();
    }
}

/* See infrun.h.  */

void
all_uis_on_sync_execution_starting (void)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      if (current_ui->prompt_state == PROMPT_NEEDED)
	async_disable_stdin ();
    }
}

/* A cleanup that restores the execution direction to the value saved
   in *ARG.  */

static void
restore_execution_direction (void *arg)
{
  enum exec_direction_kind *save_exec_dir = (enum exec_direction_kind *) arg;

  execution_direction = *save_exec_dir;
}

/* Asynchronous version of wait_for_inferior.  It is called by the
   event loop whenever a change of state is detected on the file
   descriptor corresponding to the target.  It can be called more than
   once to complete a single execution command.  In such cases we need
   to keep the state in a global variable ECSS.  If it is the last time
   that this function is called for a single execution command, then
   report to the user that the inferior has stopped, and do the
   necessary cleanups.  */

void
fetch_inferior_event (void *client_data)
{
  struct execution_control_state ecss;
  struct execution_control_state *ecs = &ecss;
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);
  struct cleanup *ts_old_chain;
  enum exec_direction_kind save_exec_dir = execution_direction;
  int cmd_done = 0;
  ptid_t waiton_ptid = minus_one_ptid;

  memset (ecs, 0, sizeof (*ecs));

  /* Events are always processed with the main UI as current UI.  This
     way, warnings, debug output, etc. are always consistently sent to
     the main console.  */
  make_cleanup_restore_current_ui ();
  current_ui = main_ui;

  /* End up with readline processing input, if necessary.  */
  make_cleanup (reinstall_readline_callback_handler_cleanup, NULL);

  /* We're handling a live event, so make sure we're doing live
     debugging.  If we're looking at traceframes while the target is
     running, we're going to need to get back to that mode after
     handling the event.  */
  if (non_stop)
    {
      make_cleanup_restore_current_traceframe ();
      set_current_traceframe (-1);
    }

  if (non_stop)
    /* In non-stop mode, the user/frontend should not notice a thread
       switch due to internal events.  Make sure we reverse to the
       user selected thread and frame after handling the event and
       running any breakpoint commands.  */
    make_cleanup_restore_current_thread ();

  overlay_cache_invalid = 1;
  /* Flush target cache before starting to handle each event.  Target
     was running and cache could be stale.  This is just a heuristic.
     Running threads may modify target memory, but we don't get any
     event.  */
  target_dcache_invalidate ();

  make_cleanup (restore_execution_direction, &save_exec_dir);
  execution_direction = target_execution_direction ();

  ecs->ptid = do_target_wait (waiton_ptid, &ecs->ws,
			      target_can_async_p () ? TARGET_WNOHANG : 0);

  if (debug_infrun)
    print_target_wait_results (waiton_ptid, ecs->ptid, &ecs->ws);

  /* If an error happens while handling the event, propagate GDB's
     knowledge of the executing state to the frontend/user running
     state.  */
  if (!target_is_non_stop_p ())
    ts_old_chain = make_cleanup (finish_thread_state_cleanup, &minus_one_ptid);
  else
    ts_old_chain = make_cleanup (finish_thread_state_cleanup, &ecs->ptid);

  /* Get executed before make_cleanup_restore_current_thread above to apply
     still for the thread which has thrown the exception.  */
  make_bpstat_clear_actions_cleanup ();

  make_cleanup (delete_just_stopped_threads_infrun_breakpoints_cleanup, NULL);

  /* Now figure out what to do with the result of the result.  */
  handle_inferior_event (ecs);

  if (!ecs->wait_some_more)
    {
      struct inferior *inf = find_inferior_ptid (ecs->ptid);
      int should_stop = 1;
      struct thread_info *thr = ecs->event_thread;
      int should_notify_stop = 1;

      delete_just_stopped_threads_infrun_breakpoints ();

      if (thr != NULL)
	{
	  struct thread_fsm *thread_fsm = thr->thread_fsm;

	  if (thread_fsm != NULL)
	    should_stop = thread_fsm_should_stop (thread_fsm, thr);
	}

      if (!should_stop)
	{
	  keep_going (ecs);
	}
      else
	{
	  clean_up_just_stopped_threads_fsms (ecs);

	  if (thr != NULL && thr->thread_fsm != NULL)
	    {
	      should_notify_stop
		= thread_fsm_should_notify_stop (thr->thread_fsm);
	    }

	  if (should_notify_stop)
	    {
	      int proceeded = 0;

	      /* We may not find an inferior if this was a process exit.  */
	      if (inf == NULL || inf->control.stop_soon == NO_STOP_QUIETLY)
		proceeded = normal_stop ();

	      if (!proceeded)
		{
		  inferior_event_handler (INF_EXEC_COMPLETE, NULL);
		  cmd_done = 1;
		}
	    }
	}
    }

  /* No error, don't finish the thread states yet.  */
  discard_cleanups (ts_old_chain);

  /* Revert thread and frame.  */
  do_cleanups (old_chain);

  /* If a UI was in sync execution mode, and now isn't, restore its
     prompt (a synchronous execution command has finished, and we're
     ready for input).  */
  all_uis_check_sync_execution_done ();

  if (cmd_done
      && exec_done_display_p
      && (ptid_equal (inferior_ptid, null_ptid)
	  || !is_running (inferior_ptid)))
    printf_unfiltered (_("completed.\n"));
}

/* Record the frame and location we're currently stepping through.  */
void
set_step_info (struct frame_info *frame, struct symtab_and_line sal)
{
  struct thread_info *tp = inferior_thread ();

  tp->control.step_frame_id = get_frame_id (frame);
  tp->control.step_stack_frame_id = get_stack_frame_id (frame);

  tp->current_symtab = sal.symtab;
  tp->current_line = sal.line;
}

/* Clear context switchable stepping state.  */

void
init_thread_stepping_state (struct thread_info *tss)
{
  tss->stepped_breakpoint = 0;
  tss->stepping_over_breakpoint = 0;
  tss->stepping_over_watchpoint = 0;
  tss->step_after_step_resume_breakpoint = 0;
}

/* Set the cached copy of the last ptid/waitstatus.  */

void
set_last_target_status (ptid_t ptid, struct target_waitstatus status)
{
  target_last_wait_ptid = ptid;
  target_last_waitstatus = status;
}

/* Return the cached copy of the last pid/waitstatus returned by
   target_wait()/deprecated_target_wait_hook().  The data is actually
   cached by handle_inferior_event(), which gets called immediately
   after target_wait()/deprecated_target_wait_hook().  */

void
get_last_target_status (ptid_t *ptidp, struct target_waitstatus *status)
{
  *ptidp = target_last_wait_ptid;
  *status = target_last_waitstatus;
}

void
nullify_last_target_wait_ptid (void)
{
  target_last_wait_ptid = minus_one_ptid;
}

/* Switch thread contexts.  */

static void
context_switch (ptid_t ptid)
{
  if (debug_infrun && !ptid_equal (ptid, inferior_ptid))
    {
      fprintf_unfiltered (gdb_stdlog, "infrun: Switching context from %s ",
			  target_pid_to_str (inferior_ptid));
      fprintf_unfiltered (gdb_stdlog, "to %s\n",
			  target_pid_to_str (ptid));
    }

  switch_to_thread (ptid);
}

/* If the target can't tell whether we've hit breakpoints
   (target_supports_stopped_by_sw_breakpoint), and we got a SIGTRAP,
   check whether that could have been caused by a breakpoint.  If so,
   adjust the PC, per gdbarch_decr_pc_after_break.  */

static void
adjust_pc_after_break (struct thread_info *thread,
		       struct target_waitstatus *ws)
{
  struct regcache *regcache;
  struct gdbarch *gdbarch;
  struct address_space *aspace;
  CORE_ADDR breakpoint_pc, decr_pc;

  /* If we've hit a breakpoint, we'll normally be stopped with SIGTRAP.  If
     we aren't, just return.

     We assume that waitkinds other than TARGET_WAITKIND_STOPPED are not
     affected by gdbarch_decr_pc_after_break.  Other waitkinds which are
     implemented by software breakpoints should be handled through the normal
     breakpoint layer.

     NOTE drow/2004-01-31: On some targets, breakpoints may generate
     different signals (SIGILL or SIGEMT for instance), but it is less
     clear where the PC is pointing afterwards.  It may not match
     gdbarch_decr_pc_after_break.  I don't know any specific target that
     generates these signals at breakpoints (the code has been in GDB since at
     least 1992) so I can not guess how to handle them here.

     In earlier versions of GDB, a target with 
     gdbarch_have_nonsteppable_watchpoint would have the PC after hitting a
     watchpoint affected by gdbarch_decr_pc_after_break.  I haven't found any
     target with both of these set in GDB history, and it seems unlikely to be
     correct, so gdbarch_have_nonsteppable_watchpoint is not checked here.  */

  if (ws->kind != TARGET_WAITKIND_STOPPED)
    return;

  if (ws->value.sig != GDB_SIGNAL_TRAP)
    return;

  /* In reverse execution, when a breakpoint is hit, the instruction
     under it has already been de-executed.  The reported PC always
     points at the breakpoint address, so adjusting it further would
     be wrong.  E.g., consider this case on a decr_pc_after_break == 1
     architecture:

       B1         0x08000000 :   INSN1
       B2         0x08000001 :   INSN2
		  0x08000002 :   INSN3
	    PC -> 0x08000003 :   INSN4

     Say you're stopped at 0x08000003 as above.  Reverse continuing
     from that point should hit B2 as below.  Reading the PC when the
     SIGTRAP is reported should read 0x08000001 and INSN2 should have
     been de-executed already.

       B1         0x08000000 :   INSN1
       B2   PC -> 0x08000001 :   INSN2
		  0x08000002 :   INSN3
		  0x08000003 :   INSN4

     We can't apply the same logic as for forward execution, because
     we would wrongly adjust the PC to 0x08000000, since there's a
     breakpoint at PC - 1.  We'd then report a hit on B1, although
     INSN1 hadn't been de-executed yet.  Doing nothing is the correct
     behaviour.  */
  if (execution_direction == EXEC_REVERSE)
    return;

  /* If the target can tell whether the thread hit a SW breakpoint,
     trust it.  Targets that can tell also adjust the PC
     themselves.  */
  if (target_supports_stopped_by_sw_breakpoint ())
    return;

  /* Note that relying on whether a breakpoint is planted in memory to
     determine this can fail.  E.g,. the breakpoint could have been
     removed since.  Or the thread could have been told to step an
     instruction the size of a breakpoint instruction, and only
     _after_ was a breakpoint inserted at its address.  */

  /* If this target does not decrement the PC after breakpoints, then
     we have nothing to do.  */
  regcache = get_thread_regcache (thread->ptid);
  gdbarch = get_regcache_arch (regcache);

  decr_pc = gdbarch_decr_pc_after_break (gdbarch);
  if (decr_pc == 0)
    return;

  aspace = get_regcache_aspace (regcache);

  /* Find the location where (if we've hit a breakpoint) the
     breakpoint would be.  */
  breakpoint_pc = regcache_read_pc (regcache) - decr_pc;

  /* If the target can't tell whether a software breakpoint triggered,
     fallback to figuring it out based on breakpoints we think were
     inserted in the target, and on whether the thread was stepped or
     continued.  */

  /* Check whether there actually is a software breakpoint inserted at
     that location.

     If in non-stop mode, a race condition is possible where we've
     removed a breakpoint, but stop events for that breakpoint were
     already queued and arrive later.  To suppress those spurious
     SIGTRAPs, we keep a list of such breakpoint locations for a bit,
     and retire them after a number of stop events are reported.  Note
     this is an heuristic and can thus get confused.  The real fix is
     to get the "stopped by SW BP and needs adjustment" info out of
     the target/kernel (and thus never reach here; see above).  */
  if (software_breakpoint_inserted_here_p (aspace, breakpoint_pc)
      || (target_is_non_stop_p ()
	  && moribund_breakpoint_here_p (aspace, breakpoint_pc)))
    {
      struct cleanup *old_cleanups = make_cleanup (null_cleanup, NULL);

      if (record_full_is_used ())
	record_full_gdb_operation_disable_set ();

      /* When using hardware single-step, a SIGTRAP is reported for both
	 a completed single-step and a software breakpoint.  Need to
	 differentiate between the two, as the latter needs adjusting
	 but the former does not.

	 The SIGTRAP can be due to a completed hardware single-step only if 
	  - we didn't insert software single-step breakpoints
	  - this thread is currently being stepped

	 If any of these events did not occur, we must have stopped due
	 to hitting a software breakpoint, and have to back up to the
	 breakpoint address.

	 As a special case, we could have hardware single-stepped a
	 software breakpoint.  In this case (prev_pc == breakpoint_pc),
	 we also need to back up to the breakpoint address.  */

      if (thread_has_single_step_breakpoints_set (thread)
	  || !currently_stepping (thread)
	  || (thread->stepped_breakpoint
	      && thread->prev_pc == breakpoint_pc))
	regcache_write_pc (regcache, breakpoint_pc);

      do_cleanups (old_cleanups);
    }
}

static int
stepped_in_from (struct frame_info *frame, struct frame_id step_frame_id)
{
  for (frame = get_prev_frame (frame);
       frame != NULL;
       frame = get_prev_frame (frame))
    {
      if (frame_id_eq (get_frame_id (frame), step_frame_id))
	return 1;
      if (get_frame_type (frame) != INLINE_FRAME)
	break;
    }

  return 0;
}

/* Auxiliary function that handles syscall entry/return events.
   It returns 1 if the inferior should keep going (and GDB
   should ignore the event), or 0 if the event deserves to be
   processed.  */

static int
handle_syscall_event (struct execution_control_state *ecs)
{
  struct regcache *regcache;
  int syscall_number;

  if (!ptid_equal (ecs->ptid, inferior_ptid))
    context_switch (ecs->ptid);

  regcache = get_thread_regcache (ecs->ptid);
  syscall_number = ecs->ws.value.syscall_number;
  stop_pc = regcache_read_pc (regcache);

  if (catch_syscall_enabled () > 0
      && catching_syscall_number (syscall_number) > 0)
    {
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog, "infrun: syscall number = '%d'\n",
                            syscall_number);

      ecs->event_thread->control.stop_bpstat
	= bpstat_stop_status (get_regcache_aspace (regcache),
			      stop_pc, ecs->ptid, &ecs->ws);

      if (bpstat_causes_stop (ecs->event_thread->control.stop_bpstat))
	{
	  /* Catchpoint hit.  */
	  return 0;
	}
    }

  /* If no catchpoint triggered for this, then keep going.  */
  keep_going (ecs);
  return 1;
}

/* Lazily fill in the execution_control_state's stop_func_* fields.  */

static void
fill_in_stop_func (struct gdbarch *gdbarch,
		   struct execution_control_state *ecs)
{
  if (!ecs->stop_func_filled_in)
    {
      /* Don't care about return value; stop_func_start and stop_func_name
	 will both be 0 if it doesn't work.  */
      find_pc_partial_function (stop_pc, &ecs->stop_func_name,
				&ecs->stop_func_start, &ecs->stop_func_end);
      ecs->stop_func_start
	+= gdbarch_deprecated_function_start_offset (gdbarch);

      if (gdbarch_skip_entrypoint_p (gdbarch))
	ecs->stop_func_start = gdbarch_skip_entrypoint (gdbarch,
							ecs->stop_func_start);

      ecs->stop_func_filled_in = 1;
    }
}


/* Return the STOP_SOON field of the inferior pointed at by PTID.  */

static enum stop_kind
get_inferior_stop_soon (ptid_t ptid)
{
  struct inferior *inf = find_inferior_ptid (ptid);

  gdb_assert (inf != NULL);
  return inf->control.stop_soon;
}

/* Wait for one event.  Store the resulting waitstatus in WS, and
   return the event ptid.  */

static ptid_t
wait_one (struct target_waitstatus *ws)
{
  ptid_t event_ptid;
  ptid_t wait_ptid = minus_one_ptid;

  overlay_cache_invalid = 1;

  /* Flush target cache before starting to handle each event.
     Target was running and cache could be stale.  This is just a
     heuristic.  Running threads may modify target memory, but we
     don't get any event.  */
  target_dcache_invalidate ();

  if (deprecated_target_wait_hook)
    event_ptid = deprecated_target_wait_hook (wait_ptid, ws, 0);
  else
    event_ptid = target_wait (wait_ptid, ws, 0);

  if (debug_infrun)
    print_target_wait_results (wait_ptid, event_ptid, ws);

  return event_ptid;
}

/* Generate a wrapper for target_stopped_by_REASON that works on PTID
   instead of the current thread.  */
#define THREAD_STOPPED_BY(REASON)		\
static int					\
thread_stopped_by_ ## REASON (ptid_t ptid)	\
{						\
  struct cleanup *old_chain;			\
  int res;					\
						\
  old_chain = save_inferior_ptid ();		\
  inferior_ptid = ptid;				\
						\
  res = target_stopped_by_ ## REASON ();	\
						\
  do_cleanups (old_chain);			\
						\
  return res;					\
}

/* Generate thread_stopped_by_watchpoint.  */
THREAD_STOPPED_BY (watchpoint)
/* Generate thread_stopped_by_sw_breakpoint.  */
THREAD_STOPPED_BY (sw_breakpoint)
/* Generate thread_stopped_by_hw_breakpoint.  */
THREAD_STOPPED_BY (hw_breakpoint)

/* Cleanups that switches to the PTID pointed at by PTID_P.  */

static void
switch_to_thread_cleanup (void *ptid_p)
{
  ptid_t ptid = *(ptid_t *) ptid_p;

  switch_to_thread (ptid);
}

/* Save the thread's event and stop reason to process it later.  */

static void
save_waitstatus (struct thread_info *tp, struct target_waitstatus *ws)
{
  struct regcache *regcache;
  struct address_space *aspace;

  if (debug_infrun)
    {
      char *statstr;

      statstr = target_waitstatus_to_string (ws);
      fprintf_unfiltered (gdb_stdlog,
			  "infrun: saving status %s for %d.%ld.%ld\n",
			  statstr,
			  ptid_get_pid (tp->ptid),
			  ptid_get_lwp (tp->ptid),
			  ptid_get_tid (tp->ptid));
      xfree (statstr);
    }

  /* Record for later.  */
  tp->suspend.waitstatus = *ws;
  tp->suspend.waitstatus_pending_p = 1;

  regcache = get_thread_regcache (tp->ptid);
  aspace = get_regcache_aspace (regcache);

  if (ws->kind == TARGET_WAITKIND_STOPPED
      && ws->value.sig == GDB_SIGNAL_TRAP)
    {
      CORE_ADDR pc = regcache_read_pc (regcache);

      adjust_pc_after_break (tp, &tp->suspend.waitstatus);

      if (thread_stopped_by_watchpoint (tp->ptid))
	{
	  tp->suspend.stop_reason
	    = TARGET_STOPPED_BY_WATCHPOINT;
	}
      else if (target_supports_stopped_by_sw_breakpoint ()
	       && thread_stopped_by_sw_breakpoint (tp->ptid))
	{
	  tp->suspend.stop_reason
	    = TARGET_STOPPED_BY_SW_BREAKPOINT;
	}
      else if (target_supports_stopped_by_hw_breakpoint ()
	       && thread_stopped_by_hw_breakpoint (tp->ptid))
	{
	  tp->suspend.stop_reason
	    = TARGET_STOPPED_BY_HW_BREAKPOINT;
	}
      else if (!target_supports_stopped_by_hw_breakpoint ()
	       && hardware_breakpoint_inserted_here_p (aspace,
						       pc))
	{
	  tp->suspend.stop_reason
	    = TARGET_STOPPED_BY_HW_BREAKPOINT;
	}
      else if (!target_supports_stopped_by_sw_breakpoint ()
	       && software_breakpoint_inserted_here_p (aspace,
						       pc))
	{
	  tp->suspend.stop_reason
	    = TARGET_STOPPED_BY_SW_BREAKPOINT;
	}
      else if (!thread_has_single_step_breakpoints_set (tp)
	       && currently_stepping (tp))
	{
	  tp->suspend.stop_reason
	    = TARGET_STOPPED_BY_SINGLE_STEP;
	}
    }
}

/* A cleanup that disables thread create/exit events.  */

static void
disable_thread_events (void *arg)
{
  target_thread_events (0);
}

/* See infrun.h.  */

void
stop_all_threads (void)
{
  /* We may need multiple passes to discover all threads.  */
  int pass;
  int iterations = 0;
  ptid_t entry_ptid;
  struct cleanup *old_chain;

  gdb_assert (target_is_non_stop_p ());

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog, "infrun: stop_all_threads\n");

  entry_ptid = inferior_ptid;
  old_chain = make_cleanup (switch_to_thread_cleanup, &entry_ptid);

  target_thread_events (1);
  make_cleanup (disable_thread_events, NULL);

  /* Request threads to stop, and then wait for the stops.  Because
     threads we already know about can spawn more threads while we're
     trying to stop them, and we only learn about new threads when we
     update the thread list, do this in a loop, and keep iterating
     until two passes find no threads that need to be stopped.  */
  for (pass = 0; pass < 2; pass++, iterations++)
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: stop_all_threads, pass=%d, "
			    "iterations=%d\n", pass, iterations);
      while (1)
	{
	  ptid_t event_ptid;
	  struct target_waitstatus ws;
	  int need_wait = 0;
	  struct thread_info *t;

	  update_thread_list ();

	  /* Go through all threads looking for threads that we need
	     to tell the target to stop.  */
	  ALL_NON_EXITED_THREADS (t)
	    {
	      if (t->executing)
		{
		  /* If already stopping, don't request a stop again.
		     We just haven't seen the notification yet.  */
		  if (!t->stop_requested)
		    {
		      if (debug_infrun)
			fprintf_unfiltered (gdb_stdlog,
					    "infrun:   %s executing, "
					    "need stop\n",
					    target_pid_to_str (t->ptid));
		      target_stop (t->ptid);
		      t->stop_requested = 1;
		    }
		  else
		    {
		      if (debug_infrun)
			fprintf_unfiltered (gdb_stdlog,
					    "infrun:   %s executing, "
					    "already stopping\n",
					    target_pid_to_str (t->ptid));
		    }

		  if (t->stop_requested)
		    need_wait = 1;
		}
	      else
		{
		  if (debug_infrun)
		    fprintf_unfiltered (gdb_stdlog,
					"infrun:   %s not executing\n",
					target_pid_to_str (t->ptid));

		  /* The thread may be not executing, but still be
		     resumed with a pending status to process.  */
		  t->resumed = 0;
		}
	    }

	  if (!need_wait)
	    break;

	  /* If we find new threads on the second iteration, restart
	     over.  We want to see two iterations in a row with all
	     threads stopped.  */
	  if (pass > 0)
	    pass = -1;

	  event_ptid = wait_one (&ws);
	  if (ws.kind == TARGET_WAITKIND_NO_RESUMED)
	    {
	      /* All resumed threads exited.  */
	    }
	  else if (ws.kind == TARGET_WAITKIND_THREAD_EXITED
		   || ws.kind == TARGET_WAITKIND_EXITED
		   || ws.kind == TARGET_WAITKIND_SIGNALLED)
	    {
	      if (debug_infrun)
		{
		  ptid_t ptid = pid_to_ptid (ws.value.integer);

		  fprintf_unfiltered (gdb_stdlog,
				      "infrun: %s exited while "
				      "stopping threads\n",
				      target_pid_to_str (ptid));
		}
	    }
	  else
	    {
	      struct inferior *inf;

	      t = find_thread_ptid (event_ptid);
	      if (t == NULL)
		t = add_thread (event_ptid);

	      t->stop_requested = 0;
	      t->executing = 0;
	      t->resumed = 0;
	      t->control.may_range_step = 0;

	      /* This may be the first time we see the inferior report
		 a stop.  */
	      inf = find_inferior_ptid (event_ptid);
	      if (inf->needs_setup)
		{
		  switch_to_thread_no_regs (t);
		  setup_inferior (0);
		}

	      if (ws.kind == TARGET_WAITKIND_STOPPED
		  && ws.value.sig == GDB_SIGNAL_0)
		{
		  /* We caught the event that we intended to catch, so
		     there's no event pending.  */
		  t->suspend.waitstatus.kind = TARGET_WAITKIND_IGNORE;
		  t->suspend.waitstatus_pending_p = 0;

		  if (displaced_step_fixup (t->ptid, GDB_SIGNAL_0) < 0)
		    {
		      /* Add it back to the step-over queue.  */
		      if (debug_infrun)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "infrun: displaced-step of %s "
					      "canceled: adding back to the "
					      "step-over queue\n",
					      target_pid_to_str (t->ptid));
			}
		      t->control.trap_expected = 0;
		      thread_step_over_chain_enqueue (t);
		    }
		}
	      else
		{
		  enum gdb_signal sig;
		  struct regcache *regcache;

		  if (debug_infrun)
		    {
		      char *statstr;

		      statstr = target_waitstatus_to_string (&ws);
		      fprintf_unfiltered (gdb_stdlog,
					  "infrun: target_wait %s, saving "
					  "status for %d.%ld.%ld\n",
					  statstr,
					  ptid_get_pid (t->ptid),
					  ptid_get_lwp (t->ptid),
					  ptid_get_tid (t->ptid));
		      xfree (statstr);
		    }

		  /* Record for later.  */
		  save_waitstatus (t, &ws);

		  sig = (ws.kind == TARGET_WAITKIND_STOPPED
			 ? ws.value.sig : GDB_SIGNAL_0);

		  if (displaced_step_fixup (t->ptid, sig) < 0)
		    {
		      /* Add it back to the step-over queue.  */
		      t->control.trap_expected = 0;
		      thread_step_over_chain_enqueue (t);
		    }

		  regcache = get_thread_regcache (t->ptid);
		  t->suspend.stop_pc = regcache_read_pc (regcache);

		  if (debug_infrun)
		    {
		      fprintf_unfiltered (gdb_stdlog,
					  "infrun: saved stop_pc=%s for %s "
					  "(currently_stepping=%d)\n",
					  paddress (target_gdbarch (),
						    t->suspend.stop_pc),
					  target_pid_to_str (t->ptid),
					  currently_stepping (t));
		    }
		}
	    }
	}
    }

  do_cleanups (old_chain);

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog, "infrun: stop_all_threads done\n");
}

/* Handle a TARGET_WAITKIND_NO_RESUMED event.  */

static int
handle_no_resumed (struct execution_control_state *ecs)
{
  struct inferior *inf;
  struct thread_info *thread;

  if (target_can_async_p ())
    {
      struct ui *ui;
      int any_sync = 0;

      ALL_UIS (ui)
	{
	  if (ui->prompt_state == PROMPT_BLOCKED)
	    {
	      any_sync = 1;
	      break;
	    }
	}
      if (!any_sync)
	{
	  /* There were no unwaited-for children left in the target, but,
	     we're not synchronously waiting for events either.  Just
	     ignore.  */

	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: TARGET_WAITKIND_NO_RESUMED "
				"(ignoring: bg)\n");
	  prepare_to_wait (ecs);
	  return 1;
	}
    }

  /* Otherwise, if we were running a synchronous execution command, we
     may need to cancel it and give the user back the terminal.

     In non-stop mode, the target can't tell whether we've already
     consumed previous stop events, so it can end up sending us a
     no-resumed event like so:

       #0 - thread 1 is left stopped

       #1 - thread 2 is resumed and hits breakpoint
               -> TARGET_WAITKIND_STOPPED

       #2 - thread 3 is resumed and exits
            this is the last resumed thread, so
	       -> TARGET_WAITKIND_NO_RESUMED

       #3 - gdb processes stop for thread 2 and decides to re-resume
            it.

       #4 - gdb processes the TARGET_WAITKIND_NO_RESUMED event.
            thread 2 is now resumed, so the event should be ignored.

     IOW, if the stop for thread 2 doesn't end a foreground command,
     then we need to ignore the following TARGET_WAITKIND_NO_RESUMED
     event.  But it could be that the event meant that thread 2 itself
     (or whatever other thread was the last resumed thread) exited.

     To address this we refresh the thread list and check whether we
     have resumed threads _now_.  In the example above, this removes
     thread 3 from the thread list.  If thread 2 was re-resumed, we
     ignore this event.  If we find no thread resumed, then we cancel
     the synchronous command show "no unwaited-for " to the user.  */
  update_thread_list ();

  ALL_NON_EXITED_THREADS (thread)
    {
      if (thread->executing
	  || thread->suspend.waitstatus_pending_p)
	{
	  /* There were no unwaited-for children left in the target at
	     some point, but there are now.  Just ignore.  */
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: TARGET_WAITKIND_NO_RESUMED "
				"(ignoring: found resumed)\n");
	  prepare_to_wait (ecs);
	  return 1;
	}
    }

  /* Note however that we may find no resumed thread because the whole
     process exited meanwhile (thus updating the thread list results
     in an empty thread list).  In this case we know we'll be getting
     a process exit event shortly.  */
  ALL_INFERIORS (inf)
    {
      if (inf->pid == 0)
	continue;

      thread = any_live_thread_of_process (inf->pid);
      if (thread == NULL)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: TARGET_WAITKIND_NO_RESUMED "
				"(expect process exit)\n");
	  prepare_to_wait (ecs);
	  return 1;
	}
    }

  /* Go ahead and report the event.  */
  return 0;
}

/* Given an execution control state that has been freshly filled in by
   an event from the inferior, figure out what it means and take
   appropriate action.

   The alternatives are:

   1) stop_waiting and return; to really stop and return to the
   debugger.

   2) keep_going and return; to wait for the next event (set
   ecs->event_thread->stepping_over_breakpoint to 1 to single step
   once).  */

static void
handle_inferior_event_1 (struct execution_control_state *ecs)
{
  enum stop_kind stop_soon;

  if (ecs->ws.kind == TARGET_WAITKIND_IGNORE)
    {
      /* We had an event in the inferior, but we are not interested in
	 handling it at this level.  The lower layers have already
	 done what needs to be done, if anything.

	 One of the possible circumstances for this is when the
	 inferior produces output for the console.  The inferior has
	 not stopped, and we are ignoring the event.  Another possible
	 circumstance is any event which the lower level knows will be
	 reported multiple times without an intervening resume.  */
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_IGNORE\n");
      prepare_to_wait (ecs);
      return;
    }

  if (ecs->ws.kind == TARGET_WAITKIND_THREAD_EXITED)
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_THREAD_EXITED\n");
      prepare_to_wait (ecs);
      return;
    }

  if (ecs->ws.kind == TARGET_WAITKIND_NO_RESUMED
      && handle_no_resumed (ecs))
    return;

  /* Cache the last pid/waitstatus.  */
  set_last_target_status (ecs->ptid, ecs->ws);

  /* Always clear state belonging to the previous time we stopped.  */
  stop_stack_dummy = STOP_NONE;

  if (ecs->ws.kind == TARGET_WAITKIND_NO_RESUMED)
    {
      /* No unwaited-for children left.  IOW, all resumed children
	 have exited.  */
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_NO_RESUMED\n");

      stop_print_frame = 0;
      stop_waiting (ecs);
      return;
    }

  if (ecs->ws.kind != TARGET_WAITKIND_EXITED
      && ecs->ws.kind != TARGET_WAITKIND_SIGNALLED)
    {
      ecs->event_thread = find_thread_ptid (ecs->ptid);
      /* If it's a new thread, add it to the thread database.  */
      if (ecs->event_thread == NULL)
	ecs->event_thread = add_thread (ecs->ptid);

      /* Disable range stepping.  If the next step request could use a
	 range, this will be end up re-enabled then.  */
      ecs->event_thread->control.may_range_step = 0;
    }

  /* Dependent on valid ECS->EVENT_THREAD.  */
  adjust_pc_after_break (ecs->event_thread, &ecs->ws);

  /* Dependent on the current PC value modified by adjust_pc_after_break.  */
  reinit_frame_cache ();

  breakpoint_retire_moribund ();

  /* First, distinguish signals caused by the debugger from signals
     that have to do with the program's own actions.  Note that
     breakpoint insns may cause SIGTRAP or SIGILL or SIGEMT, depending
     on the operating system version.  Here we detect when a SIGILL or
     SIGEMT is really a breakpoint and change it to SIGTRAP.  We do
     something similar for SIGSEGV, since a SIGSEGV will be generated
     when we're trying to execute a breakpoint instruction on a
     non-executable stack.  This happens for call dummy breakpoints
     for architectures like SPARC that place call dummies on the
     stack.  */
  if (ecs->ws.kind == TARGET_WAITKIND_STOPPED
      && (ecs->ws.value.sig == GDB_SIGNAL_ILL
	  || ecs->ws.value.sig == GDB_SIGNAL_SEGV
	  || ecs->ws.value.sig == GDB_SIGNAL_EMT))
    {
      struct regcache *regcache = get_thread_regcache (ecs->ptid);

      if (breakpoint_inserted_here_p (get_regcache_aspace (regcache),
				      regcache_read_pc (regcache)))
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: Treating signal as SIGTRAP\n");
	  ecs->ws.value.sig = GDB_SIGNAL_TRAP;
	}
    }

  /* Mark the non-executing threads accordingly.  In all-stop, all
     threads of all processes are stopped when we get any event
     reported.  In non-stop mode, only the event thread stops.  */
  {
    ptid_t mark_ptid;

    if (!target_is_non_stop_p ())
      mark_ptid = minus_one_ptid;
    else if (ecs->ws.kind == TARGET_WAITKIND_SIGNALLED
	     || ecs->ws.kind == TARGET_WAITKIND_EXITED)
      {
	/* If we're handling a process exit in non-stop mode, even
	   though threads haven't been deleted yet, one would think
	   that there is nothing to do, as threads of the dead process
	   will be soon deleted, and threads of any other process were
	   left running.  However, on some targets, threads survive a
	   process exit event.  E.g., for the "checkpoint" command,
	   when the current checkpoint/fork exits, linux-fork.c
	   automatically switches to another fork from within
	   target_mourn_inferior, by associating the same
	   inferior/thread to another fork.  We haven't mourned yet at
	   this point, but we must mark any threads left in the
	   process as not-executing so that finish_thread_state marks
	   them stopped (in the user's perspective) if/when we present
	   the stop to the user.  */
	mark_ptid = pid_to_ptid (ptid_get_pid (ecs->ptid));
      }
    else
      mark_ptid = ecs->ptid;

    set_executing (mark_ptid, 0);

    /* Likewise the resumed flag.  */
    set_resumed (mark_ptid, 0);
  }

  switch (ecs->ws.kind)
    {
    case TARGET_WAITKIND_LOADED:
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_LOADED\n");
      if (!ptid_equal (ecs->ptid, inferior_ptid))
	context_switch (ecs->ptid);
      /* Ignore gracefully during startup of the inferior, as it might
         be the shell which has just loaded some objects, otherwise
         add the symbols for the newly loaded objects.  Also ignore at
         the beginning of an attach or remote session; we will query
         the full list of libraries once the connection is
         established.  */

      stop_soon = get_inferior_stop_soon (ecs->ptid);
      if (stop_soon == NO_STOP_QUIETLY)
	{
	  struct regcache *regcache;

	  regcache = get_thread_regcache (ecs->ptid);

	  handle_solib_event ();

	  ecs->event_thread->control.stop_bpstat
	    = bpstat_stop_status (get_regcache_aspace (regcache),
				  stop_pc, ecs->ptid, &ecs->ws);

	  if (bpstat_causes_stop (ecs->event_thread->control.stop_bpstat))
	    {
	      /* A catchpoint triggered.  */
	      process_event_stop_test (ecs);
	      return;
	    }

	  /* If requested, stop when the dynamic linker notifies
	     gdb of events.  This allows the user to get control
	     and place breakpoints in initializer routines for
	     dynamically loaded objects (among other things).  */
	  ecs->event_thread->suspend.stop_signal = GDB_SIGNAL_0;
	  if (stop_on_solib_events)
	    {
	      /* Make sure we print "Stopped due to solib-event" in
		 normal_stop.  */
	      stop_print_frame = 1;

	      stop_waiting (ecs);
	      return;
	    }
	}

      /* If we are skipping through a shell, or through shared library
	 loading that we aren't interested in, resume the program.  If
	 we're running the program normally, also resume.  */
      if (stop_soon == STOP_QUIETLY || stop_soon == NO_STOP_QUIETLY)
	{
	  /* Loading of shared libraries might have changed breakpoint
	     addresses.  Make sure new breakpoints are inserted.  */
	  if (stop_soon == NO_STOP_QUIETLY)
	    insert_breakpoints ();
	  resume (GDB_SIGNAL_0);
	  prepare_to_wait (ecs);
	  return;
	}

      /* But stop if we're attaching or setting up a remote
	 connection.  */
      if (stop_soon == STOP_QUIETLY_NO_SIGSTOP
	  || stop_soon == STOP_QUIETLY_REMOTE)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog, "infrun: quietly stopped\n");
	  stop_waiting (ecs);
	  return;
	}

      internal_error (__FILE__, __LINE__,
		      _("unhandled stop_soon: %d"), (int) stop_soon);

    case TARGET_WAITKIND_SPURIOUS:
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_SPURIOUS\n");
      if (!ptid_equal (ecs->ptid, inferior_ptid))
	context_switch (ecs->ptid);
      resume (GDB_SIGNAL_0);
      prepare_to_wait (ecs);
      return;

    case TARGET_WAITKIND_THREAD_CREATED:
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_THREAD_CREATED\n");
      if (!ptid_equal (ecs->ptid, inferior_ptid))
	context_switch (ecs->ptid);
      if (!switch_back_to_stepped_thread (ecs))
	keep_going (ecs);
      return;

    case TARGET_WAITKIND_EXITED:
    case TARGET_WAITKIND_SIGNALLED:
      if (debug_infrun)
	{
	  if (ecs->ws.kind == TARGET_WAITKIND_EXITED)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: TARGET_WAITKIND_EXITED\n");
	  else
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: TARGET_WAITKIND_SIGNALLED\n");
	}

      inferior_ptid = ecs->ptid;
      set_current_inferior (find_inferior_ptid (ecs->ptid));
      set_current_program_space (current_inferior ()->pspace);
      handle_vfork_child_exec_or_exit (0);
      target_terminal_ours ();	/* Must do this before mourn anyway.  */

      /* Clearing any previous state of convenience variables.  */
      clear_exit_convenience_vars ();

      if (ecs->ws.kind == TARGET_WAITKIND_EXITED)
	{
	  /* Record the exit code in the convenience variable $_exitcode, so
	     that the user can inspect this again later.  */
	  set_internalvar_integer (lookup_internalvar ("_exitcode"),
				   (LONGEST) ecs->ws.value.integer);

	  /* Also record this in the inferior itself.  */
	  current_inferior ()->has_exit_code = 1;
	  current_inferior ()->exit_code = (LONGEST) ecs->ws.value.integer;

	  /* Support the --return-child-result option.  */
	  return_child_result_value = ecs->ws.value.integer;

	  observer_notify_exited (ecs->ws.value.integer);
	}
      else
	{
	  struct regcache *regcache = get_thread_regcache (ecs->ptid);
	  struct gdbarch *gdbarch = get_regcache_arch (regcache);

	  if (gdbarch_gdb_signal_to_target_p (gdbarch))
	    {
	      /* Set the value of the internal variable $_exitsignal,
		 which holds the signal uncaught by the inferior.  */
	      set_internalvar_integer (lookup_internalvar ("_exitsignal"),
				       gdbarch_gdb_signal_to_target (gdbarch,
							  ecs->ws.value.sig));
	    }
	  else
	    {
	      /* We don't have access to the target's method used for
		 converting between signal numbers (GDB's internal
		 representation <-> target's representation).
		 Therefore, we cannot do a good job at displaying this
		 information to the user.  It's better to just warn
		 her about it (if infrun debugging is enabled), and
		 give up.  */
	      if (debug_infrun)
		fprintf_filtered (gdb_stdlog, _("\
Cannot fill $_exitsignal with the correct signal number.\n"));
	    }

	  observer_notify_signal_exited (ecs->ws.value.sig);
	}

      gdb_flush (gdb_stdout);
      target_mourn_inferior ();
      stop_print_frame = 0;
      stop_waiting (ecs);
      return;

      /* The following are the only cases in which we keep going;
         the above cases end in a continue or goto.  */
    case TARGET_WAITKIND_FORKED:
    case TARGET_WAITKIND_VFORKED:
      if (debug_infrun)
	{
	  if (ecs->ws.kind == TARGET_WAITKIND_FORKED)
	    fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_FORKED\n");
	  else
	    fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_VFORKED\n");
	}

      /* Check whether the inferior is displaced stepping.  */
      {
	struct regcache *regcache = get_thread_regcache (ecs->ptid);
	struct gdbarch *gdbarch = get_regcache_arch (regcache);

	/* If checking displaced stepping is supported, and thread
	   ecs->ptid is displaced stepping.  */
	if (displaced_step_in_progress_thread (ecs->ptid))
	  {
	    struct inferior *parent_inf
	      = find_inferior_ptid (ecs->ptid);
	    struct regcache *child_regcache;
	    CORE_ADDR parent_pc;

	    /* GDB has got TARGET_WAITKIND_FORKED or TARGET_WAITKIND_VFORKED,
	       indicating that the displaced stepping of syscall instruction
	       has been done.  Perform cleanup for parent process here.  Note
	       that this operation also cleans up the child process for vfork,
	       because their pages are shared.  */
	    displaced_step_fixup (ecs->ptid, GDB_SIGNAL_TRAP);
	    /* Start a new step-over in another thread if there's one
	       that needs it.  */
	    start_step_over ();

	    if (ecs->ws.kind == TARGET_WAITKIND_FORKED)
	      {
		struct displaced_step_inferior_state *displaced
		  = get_displaced_stepping_state (ptid_get_pid (ecs->ptid));

		/* Restore scratch pad for child process.  */
		displaced_step_restore (displaced, ecs->ws.value.related_pid);
	      }

	    /* Since the vfork/fork syscall instruction was executed in the scratchpad,
	       the child's PC is also within the scratchpad.  Set the child's PC
	       to the parent's PC value, which has already been fixed up.
	       FIXME: we use the parent's aspace here, although we're touching
	       the child, because the child hasn't been added to the inferior
	       list yet at this point.  */

	    child_regcache
	      = get_thread_arch_aspace_regcache (ecs->ws.value.related_pid,
						 gdbarch,
						 parent_inf->aspace);
	    /* Read PC value of parent process.  */
	    parent_pc = regcache_read_pc (regcache);

	    if (debug_displaced)
	      fprintf_unfiltered (gdb_stdlog,
				  "displaced: write child pc from %s to %s\n",
				  paddress (gdbarch,
					    regcache_read_pc (child_regcache)),
				  paddress (gdbarch, parent_pc));

	    regcache_write_pc (child_regcache, parent_pc);
	  }
      }

      if (!ptid_equal (ecs->ptid, inferior_ptid))
	context_switch (ecs->ptid);

      /* Immediately detach breakpoints from the child before there's
	 any chance of letting the user delete breakpoints from the
	 breakpoint lists.  If we don't do this early, it's easy to
	 leave left over traps in the child, vis: "break foo; catch
	 fork; c; <fork>; del; c; <child calls foo>".  We only follow
	 the fork on the last `continue', and by that time the
	 breakpoint at "foo" is long gone from the breakpoint table.
	 If we vforked, then we don't need to unpatch here, since both
	 parent and child are sharing the same memory pages; we'll
	 need to unpatch at follow/detach time instead to be certain
	 that new breakpoints added between catchpoint hit time and
	 vfork follow are detached.  */
      if (ecs->ws.kind != TARGET_WAITKIND_VFORKED)
	{
	  /* This won't actually modify the breakpoint list, but will
	     physically remove the breakpoints from the child.  */
	  detach_breakpoints (ecs->ws.value.related_pid);
	}

      delete_just_stopped_threads_single_step_breakpoints ();

      /* In case the event is caught by a catchpoint, remember that
	 the event is to be followed at the next resume of the thread,
	 and not immediately.  */
      ecs->event_thread->pending_follow = ecs->ws;

      stop_pc = regcache_read_pc (get_thread_regcache (ecs->ptid));

      ecs->event_thread->control.stop_bpstat
	= bpstat_stop_status (get_regcache_aspace (get_current_regcache ()),
			      stop_pc, ecs->ptid, &ecs->ws);

      /* If no catchpoint triggered for this, then keep going.  Note
	 that we're interested in knowing the bpstat actually causes a
	 stop, not just if it may explain the signal.  Software
	 watchpoints, for example, always appear in the bpstat.  */
      if (!bpstat_causes_stop (ecs->event_thread->control.stop_bpstat))
	{
	  ptid_t parent;
	  ptid_t child;
	  int should_resume;
	  int follow_child
	    = (follow_fork_mode_string == follow_fork_mode_child);

	  ecs->event_thread->suspend.stop_signal = GDB_SIGNAL_0;

	  should_resume = follow_fork ();

	  parent = ecs->ptid;
	  child = ecs->ws.value.related_pid;

	  /* At this point, the parent is marked running, and the
	     child is marked stopped.  */

	  /* If not resuming the parent, mark it stopped.  */
	  if (follow_child && !detach_fork && !non_stop && !sched_multi)
	    set_running (parent, 0);

	  /* If resuming the child, mark it running.  */
	  if (follow_child || (!detach_fork && (non_stop || sched_multi)))
	    set_running (child, 1);

	  /* In non-stop mode, also resume the other branch.  */
	  if (!detach_fork && (non_stop
			       || (sched_multi && target_is_non_stop_p ())))
	    {
	      if (follow_child)
		switch_to_thread (parent);
	      else
		switch_to_thread (child);

	      ecs->event_thread = inferior_thread ();
	      ecs->ptid = inferior_ptid;
	      keep_going (ecs);
	    }

	  if (follow_child)
	    switch_to_thread (child);
	  else
	    switch_to_thread (parent);

	  ecs->event_thread = inferior_thread ();
	  ecs->ptid = inferior_ptid;

	  if (should_resume)
	    keep_going (ecs);
	  else
	    stop_waiting (ecs);
	  return;
	}
      process_event_stop_test (ecs);
      return;

    case TARGET_WAITKIND_VFORK_DONE:
      /* Done with the shared memory region.  Re-insert breakpoints in
	 the parent, and keep going.  */

      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: TARGET_WAITKIND_VFORK_DONE\n");

      if (!ptid_equal (ecs->ptid, inferior_ptid))
	context_switch (ecs->ptid);

      current_inferior ()->waiting_for_vfork_done = 0;
      current_inferior ()->pspace->breakpoints_not_allowed = 0;
      /* This also takes care of reinserting breakpoints in the
	 previously locked inferior.  */
      keep_going (ecs);
      return;

    case TARGET_WAITKIND_EXECD:
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_EXECD\n");

      if (!ptid_equal (ecs->ptid, inferior_ptid))
	context_switch (ecs->ptid);

      stop_pc = regcache_read_pc (get_thread_regcache (ecs->ptid));

      /* Do whatever is necessary to the parent branch of the vfork.  */
      handle_vfork_child_exec_or_exit (1);

      /* This causes the eventpoints and symbol table to be reset.
         Must do this now, before trying to determine whether to
         stop.  */
      follow_exec (inferior_ptid, ecs->ws.value.execd_pathname);

      /* In follow_exec we may have deleted the original thread and
	 created a new one.  Make sure that the event thread is the
	 execd thread for that case (this is a nop otherwise).  */
      ecs->event_thread = inferior_thread ();

      ecs->event_thread->control.stop_bpstat
	= bpstat_stop_status (get_regcache_aspace (get_current_regcache ()),
			      stop_pc, ecs->ptid, &ecs->ws);

      /* Note that this may be referenced from inside
	 bpstat_stop_status above, through inferior_has_execd.  */
      xfree (ecs->ws.value.execd_pathname);
      ecs->ws.value.execd_pathname = NULL;

      /* If no catchpoint triggered for this, then keep going.  */
      if (!bpstat_causes_stop (ecs->event_thread->control.stop_bpstat))
	{
	  ecs->event_thread->suspend.stop_signal = GDB_SIGNAL_0;
	  keep_going (ecs);
	  return;
	}
      process_event_stop_test (ecs);
      return;

      /* Be careful not to try to gather much state about a thread
         that's in a syscall.  It's frequently a losing proposition.  */
    case TARGET_WAITKIND_SYSCALL_ENTRY:
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog,
			    "infrun: TARGET_WAITKIND_SYSCALL_ENTRY\n");
      /* Getting the current syscall number.  */
      if (handle_syscall_event (ecs) == 0)
	process_event_stop_test (ecs);
      return;

      /* Before examining the threads further, step this thread to
         get it entirely out of the syscall.  (We get notice of the
         event when the thread is just on the verge of exiting a
         syscall.  Stepping one instruction seems to get it back
         into user code.)  */
    case TARGET_WAITKIND_SYSCALL_RETURN:
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog,
			    "infrun: TARGET_WAITKIND_SYSCALL_RETURN\n");
      if (handle_syscall_event (ecs) == 0)
	process_event_stop_test (ecs);
      return;

    case TARGET_WAITKIND_STOPPED:
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_STOPPED\n");
      ecs->event_thread->suspend.stop_signal = ecs->ws.value.sig;
      handle_signal_stop (ecs);
      return;

    case TARGET_WAITKIND_NO_HISTORY:
      if (debug_infrun)
        fprintf_unfiltered (gdb_stdlog, "infrun: TARGET_WAITKIND_NO_HISTORY\n");
      /* Reverse execution: target ran out of history info.  */

      /* Switch to the stopped thread.  */
      if (!ptid_equal (ecs->ptid, inferior_ptid))
	context_switch (ecs->ptid);
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: stopped\n");

      delete_just_stopped_threads_single_step_breakpoints ();
      stop_pc = regcache_read_pc (get_thread_regcache (inferior_ptid));
      observer_notify_no_history ();
      stop_waiting (ecs);
      return;
    }
}

/* A wrapper around handle_inferior_event_1, which also makes sure
   that all temporary struct value objects that were created during
   the handling of the event get deleted at the end.  */

static void
handle_inferior_event (struct execution_control_state *ecs)
{
  struct value *mark = value_mark ();

  handle_inferior_event_1 (ecs);
  /* Purge all temporary values created during the event handling,
     as it could be a long time before we return to the command level
     where such values would otherwise be purged.  */
  value_free_to_mark (mark);
}

/* Restart threads back to what they were trying to do back when we
   paused them for an in-line step-over.  The EVENT_THREAD thread is
   ignored.  */

static void
restart_threads (struct thread_info *event_thread)
{
  struct thread_info *tp;

  /* In case the instruction just stepped spawned a new thread.  */
  update_thread_list ();

  ALL_NON_EXITED_THREADS (tp)
    {
      if (tp == event_thread)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: restart threads: "
				"[%s] is event thread\n",
				target_pid_to_str (tp->ptid));
	  continue;
	}

      if (!(tp->state == THREAD_RUNNING || tp->control.in_infcall))
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: restart threads: "
				"[%s] not meant to be running\n",
				target_pid_to_str (tp->ptid));
	  continue;
	}

      if (tp->resumed)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: restart threads: [%s] resumed\n",
				target_pid_to_str (tp->ptid));
	  gdb_assert (tp->executing || tp->suspend.waitstatus_pending_p);
	  continue;
	}

      if (thread_is_in_step_over_chain (tp))
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: restart threads: "
				"[%s] needs step-over\n",
				target_pid_to_str (tp->ptid));
	  gdb_assert (!tp->resumed);
	  continue;
	}


      if (tp->suspend.waitstatus_pending_p)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: restart threads: "
				"[%s] has pending status\n",
				target_pid_to_str (tp->ptid));
	  tp->resumed = 1;
	  continue;
	}

      /* If some thread needs to start a step-over at this point, it
	 should still be in the step-over queue, and thus skipped
	 above.  */
      if (thread_still_needs_step_over (tp))
	{
	  internal_error (__FILE__, __LINE__,
			  "thread [%s] needs a step-over, but not in "
			  "step-over queue\n",
			  target_pid_to_str (tp->ptid));
	}

      if (currently_stepping (tp))
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: restart threads: [%s] was stepping\n",
				target_pid_to_str (tp->ptid));
	  keep_going_stepped_thread (tp);
	}
      else
	{
	  struct execution_control_state ecss;
	  struct execution_control_state *ecs = &ecss;

	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: restart threads: [%s] continuing\n",
				target_pid_to_str (tp->ptid));
	  reset_ecs (ecs, tp);
	  switch_to_thread (tp->ptid);
	  keep_going_pass_signal (ecs);
	}
    }
}

/* Callback for iterate_over_threads.  Find a resumed thread that has
   a pending waitstatus.  */

static int
resumed_thread_with_pending_status (struct thread_info *tp,
				    void *arg)
{
  return (tp->resumed
	  && tp->suspend.waitstatus_pending_p);
}

/* Called when we get an event that may finish an in-line or
   out-of-line (displaced stepping) step-over started previously.
   Return true if the event is processed and we should go back to the
   event loop; false if the caller should continue processing the
   event.  */

static int
finish_step_over (struct execution_control_state *ecs)
{
  int had_step_over_info;

  displaced_step_fixup (ecs->ptid,
			ecs->event_thread->suspend.stop_signal);

  had_step_over_info = step_over_info_valid_p ();

  if (had_step_over_info)
    {
      /* If we're stepping over a breakpoint with all threads locked,
	 then only the thread that was stepped should be reporting
	 back an event.  */
      gdb_assert (ecs->event_thread->control.trap_expected);

      if (ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP)
	clear_step_over_info ();
    }

  if (!target_is_non_stop_p ())
    return 0;

  /* Start a new step-over in another thread if there's one that
     needs it.  */
  start_step_over ();

  /* If we were stepping over a breakpoint before, and haven't started
     a new in-line step-over sequence, then restart all other threads
     (except the event thread).  We can't do this in all-stop, as then
     e.g., we wouldn't be able to issue any other remote packet until
     these other threads stop.  */
  if (had_step_over_info && !step_over_info_valid_p ())
    {
      struct thread_info *pending;

      /* If we only have threads with pending statuses, the restart
	 below won't restart any thread and so nothing re-inserts the
	 breakpoint we just stepped over.  But we need it inserted
	 when we later process the pending events, otherwise if
	 another thread has a pending event for this breakpoint too,
	 we'd discard its event (because the breakpoint that
	 originally caused the event was no longer inserted).  */
      context_switch (ecs->ptid);
      insert_breakpoints ();

      restart_threads (ecs->event_thread);

      /* If we have events pending, go through handle_inferior_event
	 again, picking up a pending event at random.  This avoids
	 thread starvation.  */

      /* But not if we just stepped over a watchpoint in order to let
	 the instruction execute so we can evaluate its expression.
	 The set of watchpoints that triggered is recorded in the
	 breakpoint objects themselves (see bp->watchpoint_triggered).
	 If we processed another event first, that other event could
	 clobber this info.  */
      if (ecs->event_thread->stepping_over_watchpoint)
	return 0;

      pending = iterate_over_threads (resumed_thread_with_pending_status,
				      NULL);
      if (pending != NULL)
	{
	  struct thread_info *tp = ecs->event_thread;
	  struct regcache *regcache;

	  if (debug_infrun)
	    {
	      fprintf_unfiltered (gdb_stdlog,
				  "infrun: found resumed threads with "
				  "pending events, saving status\n");
	    }

	  gdb_assert (pending != tp);

	  /* Record the event thread's event for later.  */
	  save_waitstatus (tp, &ecs->ws);
	  /* This was cleared early, by handle_inferior_event.  Set it
	     so this pending event is considered by
	     do_target_wait.  */
	  tp->resumed = 1;

	  gdb_assert (!tp->executing);

	  regcache = get_thread_regcache (tp->ptid);
	  tp->suspend.stop_pc = regcache_read_pc (regcache);

	  if (debug_infrun)
	    {
	      fprintf_unfiltered (gdb_stdlog,
				  "infrun: saved stop_pc=%s for %s "
				  "(currently_stepping=%d)\n",
				  paddress (target_gdbarch (),
					    tp->suspend.stop_pc),
				  target_pid_to_str (tp->ptid),
				  currently_stepping (tp));
	    }

	  /* This in-line step-over finished; clear this so we won't
	     start a new one.  This is what handle_signal_stop would
	     do, if we returned false.  */
	  tp->stepping_over_breakpoint = 0;

	  /* Wake up the event loop again.  */
	  mark_async_event_handler (infrun_async_inferior_event_token);

	  prepare_to_wait (ecs);
	  return 1;
	}
    }

  return 0;
}

/* Come here when the program has stopped with a signal.  */

static void
handle_signal_stop (struct execution_control_state *ecs)
{
  struct frame_info *frame;
  struct gdbarch *gdbarch;
  int stopped_by_watchpoint;
  enum stop_kind stop_soon;
  int random_signal;

  gdb_assert (ecs->ws.kind == TARGET_WAITKIND_STOPPED);

  /* Do we need to clean up the state of a thread that has
     completed a displaced single-step?  (Doing so usually affects
     the PC, so do it here, before we set stop_pc.)  */
  if (finish_step_over (ecs))
    return;

  /* If we either finished a single-step or hit a breakpoint, but
     the user wanted this thread to be stopped, pretend we got a
     SIG0 (generic unsignaled stop).  */
  if (ecs->event_thread->stop_requested
      && ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP)
    ecs->event_thread->suspend.stop_signal = GDB_SIGNAL_0;

  stop_pc = regcache_read_pc (get_thread_regcache (ecs->ptid));

  if (debug_infrun)
    {
      struct regcache *regcache = get_thread_regcache (ecs->ptid);
      struct gdbarch *gdbarch = get_regcache_arch (regcache);
      struct cleanup *old_chain = save_inferior_ptid ();

      inferior_ptid = ecs->ptid;

      fprintf_unfiltered (gdb_stdlog, "infrun: stop_pc = %s\n",
                          paddress (gdbarch, stop_pc));
      if (target_stopped_by_watchpoint ())
	{
          CORE_ADDR addr;

	  fprintf_unfiltered (gdb_stdlog, "infrun: stopped by watchpoint\n");

          if (target_stopped_data_address (&current_target, &addr))
            fprintf_unfiltered (gdb_stdlog,
                                "infrun: stopped data address = %s\n",
                                paddress (gdbarch, addr));
          else
            fprintf_unfiltered (gdb_stdlog,
                                "infrun: (no data address available)\n");
	}

      do_cleanups (old_chain);
    }

  /* This is originated from start_remote(), start_inferior() and
     shared libraries hook functions.  */
  stop_soon = get_inferior_stop_soon (ecs->ptid);
  if (stop_soon == STOP_QUIETLY || stop_soon == STOP_QUIETLY_REMOTE)
    {
      if (!ptid_equal (ecs->ptid, inferior_ptid))
	context_switch (ecs->ptid);
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: quietly stopped\n");
      stop_print_frame = 1;
      stop_waiting (ecs);
      return;
    }

  /* This originates from attach_command().  We need to overwrite
     the stop_signal here, because some kernels don't ignore a
     SIGSTOP in a subsequent ptrace(PTRACE_CONT,SIGSTOP) call.
     See more comments in inferior.h.  On the other hand, if we
     get a non-SIGSTOP, report it to the user - assume the backend
     will handle the SIGSTOP if it should show up later.

     Also consider that the attach is complete when we see a
     SIGTRAP.  Some systems (e.g. Windows), and stubs supporting
     target extended-remote report it instead of a SIGSTOP
     (e.g. gdbserver).  We already rely on SIGTRAP being our
     signal, so this is no exception.

     Also consider that the attach is complete when we see a
     GDB_SIGNAL_0.  In non-stop mode, GDB will explicitly tell
     the target to stop all threads of the inferior, in case the
     low level attach operation doesn't stop them implicitly.  If
     they weren't stopped implicitly, then the stub will report a
     GDB_SIGNAL_0, meaning: stopped for no particular reason
     other than GDB's request.  */
  if (stop_soon == STOP_QUIETLY_NO_SIGSTOP
      && (ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_STOP
	  || ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP
	  || ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_0))
    {
      stop_print_frame = 1;
      stop_waiting (ecs);
      ecs->event_thread->suspend.stop_signal = GDB_SIGNAL_0;
      return;
    }

  /* See if something interesting happened to the non-current thread.  If
     so, then switch to that thread.  */
  if (!ptid_equal (ecs->ptid, inferior_ptid))
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: context switch\n");

      context_switch (ecs->ptid);

      if (deprecated_context_hook)
	deprecated_context_hook (ptid_to_global_thread_id (ecs->ptid));
    }

  /* At this point, get hold of the now-current thread's frame.  */
  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);

  /* Pull the single step breakpoints out of the target.  */
  if (ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP)
    {
      struct regcache *regcache;
      struct address_space *aspace;
      CORE_ADDR pc;

      regcache = get_thread_regcache (ecs->ptid);
      aspace = get_regcache_aspace (regcache);
      pc = regcache_read_pc (regcache);

      /* However, before doing so, if this single-step breakpoint was
	 actually for another thread, set this thread up for moving
	 past it.  */
      if (!thread_has_single_step_breakpoint_here (ecs->event_thread,
						   aspace, pc))
	{
	  if (single_step_breakpoint_inserted_here_p (aspace, pc))
	    {
	      if (debug_infrun)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "infrun: [%s] hit another thread's "
				      "single-step breakpoint\n",
				      target_pid_to_str (ecs->ptid));
		}
	      ecs->hit_singlestep_breakpoint = 1;
	    }
	}
      else
	{
	  if (debug_infrun)
	    {
	      fprintf_unfiltered (gdb_stdlog,
				  "infrun: [%s] hit its "
				  "single-step breakpoint\n",
				  target_pid_to_str (ecs->ptid));
	    }
	}
    }
  delete_just_stopped_threads_single_step_breakpoints ();

  if (ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP
      && ecs->event_thread->control.trap_expected
      && ecs->event_thread->stepping_over_watchpoint)
    stopped_by_watchpoint = 0;
  else
    stopped_by_watchpoint = watchpoints_triggered (&ecs->ws);

  /* If necessary, step over this watchpoint.  We'll be back to display
     it in a moment.  */
  if (stopped_by_watchpoint
      && (target_have_steppable_watchpoint
	  || gdbarch_have_nonsteppable_watchpoint (gdbarch)))
    {
      /* At this point, we are stopped at an instruction which has
         attempted to write to a piece of memory under control of
         a watchpoint.  The instruction hasn't actually executed
         yet.  If we were to evaluate the watchpoint expression
         now, we would get the old value, and therefore no change
         would seem to have occurred.

         In order to make watchpoints work `right', we really need
         to complete the memory write, and then evaluate the
         watchpoint expression.  We do this by single-stepping the
	 target.

	 It may not be necessary to disable the watchpoint to step over
	 it.  For example, the PA can (with some kernel cooperation)
	 single step over a watchpoint without disabling the watchpoint.

	 It is far more common to need to disable a watchpoint to step
	 the inferior over it.  If we have non-steppable watchpoints,
	 we must disable the current watchpoint; it's simplest to
	 disable all watchpoints.

	 Any breakpoint at PC must also be stepped over -- if there's
	 one, it will have already triggered before the watchpoint
	 triggered, and we either already reported it to the user, or
	 it didn't cause a stop and we called keep_going.  In either
	 case, if there was a breakpoint at PC, we must be trying to
	 step past it.  */
      ecs->event_thread->stepping_over_watchpoint = 1;
      keep_going (ecs);
      return;
    }

  ecs->event_thread->stepping_over_breakpoint = 0;
  ecs->event_thread->stepping_over_watchpoint = 0;
  bpstat_clear (&ecs->event_thread->control.stop_bpstat);
  ecs->event_thread->control.stop_step = 0;
  stop_print_frame = 1;
  stopped_by_random_signal = 0;

  /* Hide inlined functions starting here, unless we just performed stepi or
     nexti.  After stepi and nexti, always show the innermost frame (not any
     inline function call sites).  */
  if (ecs->event_thread->control.step_range_end != 1)
    {
      struct address_space *aspace = 
	get_regcache_aspace (get_thread_regcache (ecs->ptid));

      /* skip_inline_frames is expensive, so we avoid it if we can
	 determine that the address is one where functions cannot have
	 been inlined.  This improves performance with inferiors that
	 load a lot of shared libraries, because the solib event
	 breakpoint is defined as the address of a function (i.e. not
	 inline).  Note that we have to check the previous PC as well
	 as the current one to catch cases when we have just
	 single-stepped off a breakpoint prior to reinstating it.
	 Note that we're assuming that the code we single-step to is
	 not inline, but that's not definitive: there's nothing
	 preventing the event breakpoint function from containing
	 inlined code, and the single-step ending up there.  If the
	 user had set a breakpoint on that inlined code, the missing
	 skip_inline_frames call would break things.  Fortunately
	 that's an extremely unlikely scenario.  */
      if (!pc_at_non_inline_function (aspace, stop_pc, &ecs->ws)
	  && !(ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP
	       && ecs->event_thread->control.trap_expected
	       && pc_at_non_inline_function (aspace,
					     ecs->event_thread->prev_pc,
					     &ecs->ws)))
	{
	  skip_inline_frames (ecs->ptid);

	  /* Re-fetch current thread's frame in case that invalidated
	     the frame cache.  */
	  frame = get_current_frame ();
	  gdbarch = get_frame_arch (frame);
	}
    }

  if (ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP
      && ecs->event_thread->control.trap_expected
      && gdbarch_single_step_through_delay_p (gdbarch)
      && currently_stepping (ecs->event_thread))
    {
      /* We're trying to step off a breakpoint.  Turns out that we're
	 also on an instruction that needs to be stepped multiple
	 times before it's been fully executing.  E.g., architectures
	 with a delay slot.  It needs to be stepped twice, once for
	 the instruction and once for the delay slot.  */
      int step_through_delay
	= gdbarch_single_step_through_delay (gdbarch, frame);

      if (debug_infrun && step_through_delay)
	fprintf_unfiltered (gdb_stdlog, "infrun: step through delay\n");
      if (ecs->event_thread->control.step_range_end == 0
	  && step_through_delay)
	{
	  /* The user issued a continue when stopped at a breakpoint.
	     Set up for another trap and get out of here.  */
         ecs->event_thread->stepping_over_breakpoint = 1;
         keep_going (ecs);
         return;
	}
      else if (step_through_delay)
	{
	  /* The user issued a step when stopped at a breakpoint.
	     Maybe we should stop, maybe we should not - the delay
	     slot *might* correspond to a line of source.  In any
	     case, don't decide that here, just set 
	     ecs->stepping_over_breakpoint, making sure we 
	     single-step again before breakpoints are re-inserted.  */
	  ecs->event_thread->stepping_over_breakpoint = 1;
	}
    }

  /* See if there is a breakpoint/watchpoint/catchpoint/etc. that
     handles this event.  */
  ecs->event_thread->control.stop_bpstat
    = bpstat_stop_status (get_regcache_aspace (get_current_regcache ()),
			  stop_pc, ecs->ptid, &ecs->ws);

  /* Following in case break condition called a
     function.  */
  stop_print_frame = 1;

  /* This is where we handle "moribund" watchpoints.  Unlike
     software breakpoints traps, hardware watchpoint traps are
     always distinguishable from random traps.  If no high-level
     watchpoint is associated with the reported stop data address
     anymore, then the bpstat does not explain the signal ---
     simply make sure to ignore it if `stopped_by_watchpoint' is
     set.  */

  if (debug_infrun
      && ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP
      && !bpstat_explains_signal (ecs->event_thread->control.stop_bpstat,
				  GDB_SIGNAL_TRAP)
      && stopped_by_watchpoint)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: no user watchpoint explains "
			"watchpoint SIGTRAP, ignoring\n");

  /* NOTE: cagney/2003-03-29: These checks for a random signal
     at one stage in the past included checks for an inferior
     function call's call dummy's return breakpoint.  The original
     comment, that went with the test, read:

     ``End of a stack dummy.  Some systems (e.g. Sony news) give
     another signal besides SIGTRAP, so check here as well as
     above.''

     If someone ever tries to get call dummys on a
     non-executable stack to work (where the target would stop
     with something like a SIGSEGV), then those tests might need
     to be re-instated.  Given, however, that the tests were only
     enabled when momentary breakpoints were not being used, I
     suspect that it won't be the case.

     NOTE: kettenis/2004-02-05: Indeed such checks don't seem to
     be necessary for call dummies on a non-executable stack on
     SPARC.  */

  /* See if the breakpoints module can explain the signal.  */
  random_signal
    = !bpstat_explains_signal (ecs->event_thread->control.stop_bpstat,
			       ecs->event_thread->suspend.stop_signal);

  /* Maybe this was a trap for a software breakpoint that has since
     been removed.  */
  if (random_signal && target_stopped_by_sw_breakpoint ())
    {
      if (program_breakpoint_here_p (gdbarch, stop_pc))
	{
	  struct regcache *regcache;
	  int decr_pc;

	  /* Re-adjust PC to what the program would see if GDB was not
	     debugging it.  */
	  regcache = get_thread_regcache (ecs->event_thread->ptid);
	  decr_pc = gdbarch_decr_pc_after_break (gdbarch);
	  if (decr_pc != 0)
	    {
	      struct cleanup *old_cleanups = make_cleanup (null_cleanup, NULL);

	      if (record_full_is_used ())
		record_full_gdb_operation_disable_set ();

	      regcache_write_pc (regcache, stop_pc + decr_pc);

	      do_cleanups (old_cleanups);
	    }
	}
      else
	{
	  /* A delayed software breakpoint event.  Ignore the trap.  */
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: delayed software breakpoint "
				"trap, ignoring\n");
	  random_signal = 0;
	}
    }

  /* Maybe this was a trap for a hardware breakpoint/watchpoint that
     has since been removed.  */
  if (random_signal && target_stopped_by_hw_breakpoint ())
    {
      /* A delayed hardware breakpoint event.  Ignore the trap.  */
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: delayed hardware breakpoint/watchpoint "
			    "trap, ignoring\n");
      random_signal = 0;
    }

  /* If not, perhaps stepping/nexting can.  */
  if (random_signal)
    random_signal = !(ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP
		      && currently_stepping (ecs->event_thread));

  /* Perhaps the thread hit a single-step breakpoint of _another_
     thread.  Single-step breakpoints are transparent to the
     breakpoints module.  */
  if (random_signal)
    random_signal = !ecs->hit_singlestep_breakpoint;

  /* No?  Perhaps we got a moribund watchpoint.  */
  if (random_signal)
    random_signal = !stopped_by_watchpoint;

  /* For the program's own signals, act according to
     the signal handling tables.  */

  if (random_signal)
    {
      /* Signal not for debugging purposes.  */
      struct inferior *inf = find_inferior_ptid (ecs->ptid);
      enum gdb_signal stop_signal = ecs->event_thread->suspend.stop_signal;

      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog, "infrun: random signal (%s)\n",
			     gdb_signal_to_symbol_string (stop_signal));

      stopped_by_random_signal = 1;

      /* Always stop on signals if we're either just gaining control
	 of the program, or the user explicitly requested this thread
	 to remain stopped.  */
      if (stop_soon != NO_STOP_QUIETLY
	  || ecs->event_thread->stop_requested
	  || (!inf->detaching
	      && signal_stop_state (ecs->event_thread->suspend.stop_signal)))
	{
	  stop_waiting (ecs);
	  return;
	}

      /* Notify observers the signal has "handle print" set.  Note we
	 returned early above if stopping; normal_stop handles the
	 printing in that case.  */
      if (signal_print[ecs->event_thread->suspend.stop_signal])
	{
	  /* The signal table tells us to print about this signal.  */
	  target_terminal_ours_for_output ();
	  observer_notify_signal_received (ecs->event_thread->suspend.stop_signal);
	  target_terminal_inferior ();
	}

      /* Clear the signal if it should not be passed.  */
      if (signal_program[ecs->event_thread->suspend.stop_signal] == 0)
	ecs->event_thread->suspend.stop_signal = GDB_SIGNAL_0;

      if (ecs->event_thread->prev_pc == stop_pc
	  && ecs->event_thread->control.trap_expected
	  && ecs->event_thread->control.step_resume_breakpoint == NULL)
	{
	  int was_in_line;

	  /* We were just starting a new sequence, attempting to
	     single-step off of a breakpoint and expecting a SIGTRAP.
	     Instead this signal arrives.  This signal will take us out
	     of the stepping range so GDB needs to remember to, when
	     the signal handler returns, resume stepping off that
	     breakpoint.  */
	  /* To simplify things, "continue" is forced to use the same
	     code paths as single-step - set a breakpoint at the
	     signal return address and then, once hit, step off that
	     breakpoint.  */
          if (debug_infrun)
            fprintf_unfiltered (gdb_stdlog,
                                "infrun: signal arrived while stepping over "
                                "breakpoint\n");

	  was_in_line = step_over_info_valid_p ();
	  clear_step_over_info ();
	  insert_hp_step_resume_breakpoint_at_frame (frame);
	  ecs->event_thread->step_after_step_resume_breakpoint = 1;
	  /* Reset trap_expected to ensure breakpoints are re-inserted.  */
	  ecs->event_thread->control.trap_expected = 0;

	  if (target_is_non_stop_p ())
	    {
	      /* Either "set non-stop" is "on", or the target is
		 always in non-stop mode.  In this case, we have a bit
		 more work to do.  Resume the current thread, and if
		 we had paused all threads, restart them while the
		 signal handler runs.  */
	      keep_going (ecs);

	      if (was_in_line)
		{
		  restart_threads (ecs->event_thread);
		}
	      else if (debug_infrun)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "infrun: no need to restart threads\n");
		}
	      return;
	    }

	  /* If we were nexting/stepping some other thread, switch to
	     it, so that we don't continue it, losing control.  */
	  if (!switch_back_to_stepped_thread (ecs))
	    keep_going (ecs);
	  return;
	}

      if (ecs->event_thread->suspend.stop_signal != GDB_SIGNAL_0
	  && (pc_in_thread_step_range (stop_pc, ecs->event_thread)
	      || ecs->event_thread->control.step_range_end == 1)
	  && frame_id_eq (get_stack_frame_id (frame),
			  ecs->event_thread->control.step_stack_frame_id)
	  && ecs->event_thread->control.step_resume_breakpoint == NULL)
	{
	  /* The inferior is about to take a signal that will take it
	     out of the single step range.  Set a breakpoint at the
	     current PC (which is presumably where the signal handler
	     will eventually return) and then allow the inferior to
	     run free.

	     Note that this is only needed for a signal delivered
	     while in the single-step range.  Nested signals aren't a
	     problem as they eventually all return.  */
          if (debug_infrun)
            fprintf_unfiltered (gdb_stdlog,
                                "infrun: signal may take us out of "
                                "single-step range\n");

	  clear_step_over_info ();
	  insert_hp_step_resume_breakpoint_at_frame (frame);
	  ecs->event_thread->step_after_step_resume_breakpoint = 1;
	  /* Reset trap_expected to ensure breakpoints are re-inserted.  */
	  ecs->event_thread->control.trap_expected = 0;
	  keep_going (ecs);
	  return;
	}

      /* Note: step_resume_breakpoint may be non-NULL.  This occures
	 when either there's a nested signal, or when there's a
	 pending signal enabled just as the signal handler returns
	 (leaving the inferior at the step-resume-breakpoint without
	 actually executing it).  Either way continue until the
	 breakpoint is really hit.  */

      if (!switch_back_to_stepped_thread (ecs))
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: random signal, keep going\n");

	  keep_going (ecs);
	}
      return;
    }

  process_event_stop_test (ecs);
}

/* Come here when we've got some debug event / signal we can explain
   (IOW, not a random signal), and test whether it should cause a
   stop, or whether we should resume the inferior (transparently).
   E.g., could be a breakpoint whose condition evaluates false; we
   could be still stepping within the line; etc.  */

static void
process_event_stop_test (struct execution_control_state *ecs)
{
  struct symtab_and_line stop_pc_sal;
  struct frame_info *frame;
  struct gdbarch *gdbarch;
  CORE_ADDR jmp_buf_pc;
  struct bpstat_what what;

  /* Handle cases caused by hitting a breakpoint.  */

  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);

  what = bpstat_what (ecs->event_thread->control.stop_bpstat);

  if (what.call_dummy)
    {
      stop_stack_dummy = what.call_dummy;
    }

  /* A few breakpoint types have callbacks associated (e.g.,
     bp_jit_event).  Run them now.  */
  bpstat_run_callbacks (ecs->event_thread->control.stop_bpstat);

  /* If we hit an internal event that triggers symbol changes, the
     current frame will be invalidated within bpstat_what (e.g., if we
     hit an internal solib event).  Re-fetch it.  */
  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);

  switch (what.main_action)
    {
    case BPSTAT_WHAT_SET_LONGJMP_RESUME:
      /* If we hit the breakpoint at longjmp while stepping, we
	 install a momentary breakpoint at the target of the
	 jmp_buf.  */

      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: BPSTAT_WHAT_SET_LONGJMP_RESUME\n");

      ecs->event_thread->stepping_over_breakpoint = 1;

      if (what.is_longjmp)
	{
	  struct value *arg_value;

	  /* If we set the longjmp breakpoint via a SystemTap probe,
	     then use it to extract the arguments.  The destination PC
	     is the third argument to the probe.  */
	  arg_value = probe_safe_evaluate_at_pc (frame, 2);
	  if (arg_value)
	    {
	      jmp_buf_pc = value_as_address (arg_value);
	      jmp_buf_pc = gdbarch_addr_bits_remove (gdbarch, jmp_buf_pc);
	    }
	  else if (!gdbarch_get_longjmp_target_p (gdbarch)
		   || !gdbarch_get_longjmp_target (gdbarch,
						   frame, &jmp_buf_pc))
	    {
	      if (debug_infrun)
		fprintf_unfiltered (gdb_stdlog,
				    "infrun: BPSTAT_WHAT_SET_LONGJMP_RESUME "
				    "(!gdbarch_get_longjmp_target)\n");
	      keep_going (ecs);
	      return;
	    }

	  /* Insert a breakpoint at resume address.  */
	  insert_longjmp_resume_breakpoint (gdbarch, jmp_buf_pc);
	}
      else
	check_exception_resume (ecs, frame);
      keep_going (ecs);
      return;

    case BPSTAT_WHAT_CLEAR_LONGJMP_RESUME:
      {
	struct frame_info *init_frame;

	/* There are several cases to consider.

	   1. The initiating frame no longer exists.  In this case we
	   must stop, because the exception or longjmp has gone too
	   far.

	   2. The initiating frame exists, and is the same as the
	   current frame.  We stop, because the exception or longjmp
	   has been caught.

	   3. The initiating frame exists and is different from the
	   current frame.  This means the exception or longjmp has
	   been caught beneath the initiating frame, so keep going.

	   4. longjmp breakpoint has been placed just to protect
	   against stale dummy frames and user is not interested in
	   stopping around longjmps.  */

	if (debug_infrun)
	  fprintf_unfiltered (gdb_stdlog,
			      "infrun: BPSTAT_WHAT_CLEAR_LONGJMP_RESUME\n");

	gdb_assert (ecs->event_thread->control.exception_resume_breakpoint
		    != NULL);
	delete_exception_resume_breakpoint (ecs->event_thread);

	if (what.is_longjmp)
	  {
	    check_longjmp_breakpoint_for_call_dummy (ecs->event_thread);

	    if (!frame_id_p (ecs->event_thread->initiating_frame))
	      {
		/* Case 4.  */
		keep_going (ecs);
		return;
	      }
	  }

	init_frame = frame_find_by_id (ecs->event_thread->initiating_frame);

	if (init_frame)
	  {
	    struct frame_id current_id
	      = get_frame_id (get_current_frame ());
	    if (frame_id_eq (current_id,
			     ecs->event_thread->initiating_frame))
	      {
		/* Case 2.  Fall through.  */
	      }
	    else
	      {
		/* Case 3.  */
		keep_going (ecs);
		return;
	      }
	  }

	/* For Cases 1 and 2, remove the step-resume breakpoint, if it
	   exists.  */
	delete_step_resume_breakpoint (ecs->event_thread);

	end_stepping_range (ecs);
      }
      return;

    case BPSTAT_WHAT_SINGLE:
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: BPSTAT_WHAT_SINGLE\n");
      ecs->event_thread->stepping_over_breakpoint = 1;
      /* Still need to check other stuff, at least the case where we
	 are stepping and step out of the right range.  */
      break;

    case BPSTAT_WHAT_STEP_RESUME:
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: BPSTAT_WHAT_STEP_RESUME\n");

      delete_step_resume_breakpoint (ecs->event_thread);
      if (ecs->event_thread->control.proceed_to_finish
	  && execution_direction == EXEC_REVERSE)
	{
	  struct thread_info *tp = ecs->event_thread;

	  /* We are finishing a function in reverse, and just hit the
	     step-resume breakpoint at the start address of the
	     function, and we're almost there -- just need to back up
	     by one more single-step, which should take us back to the
	     function call.  */
	  tp->control.step_range_start = tp->control.step_range_end = 1;
	  keep_going (ecs);
	  return;
	}
      fill_in_stop_func (gdbarch, ecs);
      if (stop_pc == ecs->stop_func_start
	  && execution_direction == EXEC_REVERSE)
	{
	  /* We are stepping over a function call in reverse, and just
	     hit the step-resume breakpoint at the start address of
	     the function.  Go back to single-stepping, which should
	     take us back to the function call.  */
	  ecs->event_thread->stepping_over_breakpoint = 1;
	  keep_going (ecs);
	  return;
	}
      break;

    case BPSTAT_WHAT_STOP_NOISY:
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: BPSTAT_WHAT_STOP_NOISY\n");
      stop_print_frame = 1;

      /* Assume the thread stopped for a breapoint.  We'll still check
	 whether a/the breakpoint is there when the thread is next
	 resumed.  */
      ecs->event_thread->stepping_over_breakpoint = 1;

      stop_waiting (ecs);
      return;

    case BPSTAT_WHAT_STOP_SILENT:
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: BPSTAT_WHAT_STOP_SILENT\n");
      stop_print_frame = 0;

      /* Assume the thread stopped for a breapoint.  We'll still check
	 whether a/the breakpoint is there when the thread is next
	 resumed.  */
      ecs->event_thread->stepping_over_breakpoint = 1;
      stop_waiting (ecs);
      return;

    case BPSTAT_WHAT_HP_STEP_RESUME:
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "infrun: BPSTAT_WHAT_HP_STEP_RESUME\n");

      delete_step_resume_breakpoint (ecs->event_thread);
      if (ecs->event_thread->step_after_step_resume_breakpoint)
	{
	  /* Back when the step-resume breakpoint was inserted, we
	     were trying to single-step off a breakpoint.  Go back to
	     doing that.  */
	  ecs->event_thread->step_after_step_resume_breakpoint = 0;
	  ecs->event_thread->stepping_over_breakpoint = 1;
	  keep_going (ecs);
	  return;
	}
      break;

    case BPSTAT_WHAT_KEEP_CHECKING:
      break;
    }

  /* If we stepped a permanent breakpoint and we had a high priority
     step-resume breakpoint for the address we stepped, but we didn't
     hit it, then we must have stepped into the signal handler.  The
     step-resume was only necessary to catch the case of _not_
     stepping into the handler, so delete it, and fall through to
     checking whether the step finished.  */
  if (ecs->event_thread->stepped_breakpoint)
    {
      struct breakpoint *sr_bp
	= ecs->event_thread->control.step_resume_breakpoint;

      if (sr_bp != NULL
	  && sr_bp->loc->permanent
	  && sr_bp->type == bp_hp_step_resume
	  && sr_bp->loc->address == ecs->event_thread->prev_pc)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: stepped permanent breakpoint, stopped in "
				"handler\n");
	  delete_step_resume_breakpoint (ecs->event_thread);
	  ecs->event_thread->step_after_step_resume_breakpoint = 0;
	}
    }

  /* We come here if we hit a breakpoint but should not stop for it.
     Possibly we also were stepping and should stop for that.  So fall
     through and test for stepping.  But, if not stepping, do not
     stop.  */

  /* In all-stop mode, if we're currently stepping but have stopped in
     some other thread, we need to switch back to the stepped thread.  */
  if (switch_back_to_stepped_thread (ecs))
    return;

  if (ecs->event_thread->control.step_resume_breakpoint)
    {
      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog,
			     "infrun: step-resume breakpoint is inserted\n");

      /* Having a step-resume breakpoint overrides anything
         else having to do with stepping commands until
         that breakpoint is reached.  */
      keep_going (ecs);
      return;
    }

  if (ecs->event_thread->control.step_range_end == 0)
    {
      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog, "infrun: no stepping, continue\n");
      /* Likewise if we aren't even stepping.  */
      keep_going (ecs);
      return;
    }

  /* Re-fetch current thread's frame in case the code above caused
     the frame cache to be re-initialized, making our FRAME variable
     a dangling pointer.  */
  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);
  fill_in_stop_func (gdbarch, ecs);

  /* If stepping through a line, keep going if still within it.

     Note that step_range_end is the address of the first instruction
     beyond the step range, and NOT the address of the last instruction
     within it!

     Note also that during reverse execution, we may be stepping
     through a function epilogue and therefore must detect when
     the current-frame changes in the middle of a line.  */

  if (pc_in_thread_step_range (stop_pc, ecs->event_thread)
      && (execution_direction != EXEC_REVERSE
	  || frame_id_eq (get_frame_id (frame),
			  ecs->event_thread->control.step_frame_id)))
    {
      if (debug_infrun)
	fprintf_unfiltered
	  (gdb_stdlog, "infrun: stepping inside range [%s-%s]\n",
	   paddress (gdbarch, ecs->event_thread->control.step_range_start),
	   paddress (gdbarch, ecs->event_thread->control.step_range_end));

      /* Tentatively re-enable range stepping; `resume' disables it if
	 necessary (e.g., if we're stepping over a breakpoint or we
	 have software watchpoints).  */
      ecs->event_thread->control.may_range_step = 1;

      /* When stepping backward, stop at beginning of line range
	 (unless it's the function entry point, in which case
	 keep going back to the call point).  */
      if (stop_pc == ecs->event_thread->control.step_range_start
	  && stop_pc != ecs->stop_func_start
	  && execution_direction == EXEC_REVERSE)
	end_stepping_range (ecs);
      else
	keep_going (ecs);

      return;
    }

  /* We stepped out of the stepping range.  */

  /* If we are stepping at the source level and entered the runtime
     loader dynamic symbol resolution code...

     EXEC_FORWARD: we keep on single stepping until we exit the run
     time loader code and reach the callee's address.

     EXEC_REVERSE: we've already executed the callee (backward), and
     the runtime loader code is handled just like any other
     undebuggable function call.  Now we need only keep stepping
     backward through the trampoline code, and that's handled further
     down, so there is nothing for us to do here.  */

  if (execution_direction != EXEC_REVERSE
      && ecs->event_thread->control.step_over_calls == STEP_OVER_UNDEBUGGABLE
      && in_solib_dynsym_resolve_code (stop_pc))
    {
      CORE_ADDR pc_after_resolver =
	gdbarch_skip_solib_resolver (gdbarch, stop_pc);

      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog,
			     "infrun: stepped into dynsym resolve code\n");

      if (pc_after_resolver)
	{
	  /* Set up a step-resume breakpoint at the address
	     indicated by SKIP_SOLIB_RESOLVER.  */
	  struct symtab_and_line sr_sal;

	  init_sal (&sr_sal);
	  sr_sal.pc = pc_after_resolver;
	  sr_sal.pspace = get_frame_program_space (frame);

	  insert_step_resume_breakpoint_at_sal (gdbarch,
						sr_sal, null_frame_id);
	}

      keep_going (ecs);
      return;
    }

  if (ecs->event_thread->control.step_range_end != 1
      && (ecs->event_thread->control.step_over_calls == STEP_OVER_UNDEBUGGABLE
	  || ecs->event_thread->control.step_over_calls == STEP_OVER_ALL)
      && get_frame_type (frame) == SIGTRAMP_FRAME)
    {
      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog,
			     "infrun: stepped into signal trampoline\n");
      /* The inferior, while doing a "step" or "next", has ended up in
         a signal trampoline (either by a signal being delivered or by
         the signal handler returning).  Just single-step until the
         inferior leaves the trampoline (either by calling the handler
         or returning).  */
      keep_going (ecs);
      return;
    }

  /* If we're in the return path from a shared library trampoline,
     we want to proceed through the trampoline when stepping.  */
  /* macro/2012-04-25: This needs to come before the subroutine
     call check below as on some targets return trampolines look
     like subroutine calls (MIPS16 return thunks).  */
  if (gdbarch_in_solib_return_trampoline (gdbarch,
					  stop_pc, ecs->stop_func_name)
      && ecs->event_thread->control.step_over_calls != STEP_OVER_NONE)
    {
      /* Determine where this trampoline returns.  */
      CORE_ADDR real_stop_pc;

      real_stop_pc = gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc);

      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog,
			     "infrun: stepped into solib return tramp\n");

      /* Only proceed through if we know where it's going.  */
      if (real_stop_pc)
	{
	  /* And put the step-breakpoint there and go until there.  */
	  struct symtab_and_line sr_sal;

	  init_sal (&sr_sal);	/* initialize to zeroes */
	  sr_sal.pc = real_stop_pc;
	  sr_sal.section = find_pc_overlay (sr_sal.pc);
	  sr_sal.pspace = get_frame_program_space (frame);

	  /* Do not specify what the fp should be when we stop since
	     on some machines the prologue is where the new fp value
	     is established.  */
	  insert_step_resume_breakpoint_at_sal (gdbarch,
						sr_sal, null_frame_id);

	  /* Restart without fiddling with the step ranges or
	     other state.  */
	  keep_going (ecs);
	  return;
	}
    }

  /* Check for subroutine calls.  The check for the current frame
     equalling the step ID is not necessary - the check of the
     previous frame's ID is sufficient - but it is a common case and
     cheaper than checking the previous frame's ID.

     NOTE: frame_id_eq will never report two invalid frame IDs as
     being equal, so to get into this block, both the current and
     previous frame must have valid frame IDs.  */
  /* The outer_frame_id check is a heuristic to detect stepping
     through startup code.  If we step over an instruction which
     sets the stack pointer from an invalid value to a valid value,
     we may detect that as a subroutine call from the mythical
     "outermost" function.  This could be fixed by marking
     outermost frames as !stack_p,code_p,special_p.  Then the
     initial outermost frame, before sp was valid, would
     have code_addr == &_start.  See the comment in frame_id_eq
     for more.  */
  if (!frame_id_eq (get_stack_frame_id (frame),
		    ecs->event_thread->control.step_stack_frame_id)
      && (frame_id_eq (frame_unwind_caller_id (get_current_frame ()),
		       ecs->event_thread->control.step_stack_frame_id)
	  && (!frame_id_eq (ecs->event_thread->control.step_stack_frame_id,
			    outer_frame_id)
	      || (ecs->event_thread->control.step_start_function
		  != find_pc_function (stop_pc)))))
    {
      CORE_ADDR real_stop_pc;

      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog, "infrun: stepped into subroutine\n");

      if (ecs->event_thread->control.step_over_calls == STEP_OVER_NONE)
	{
	  /* I presume that step_over_calls is only 0 when we're
	     supposed to be stepping at the assembly language level
	     ("stepi").  Just stop.  */
	  /* And this works the same backward as frontward.  MVS */
	  end_stepping_range (ecs);
	  return;
	}

      /* Reverse stepping through solib trampolines.  */

      if (execution_direction == EXEC_REVERSE
	  && ecs->event_thread->control.step_over_calls != STEP_OVER_NONE
	  && (gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc)
	      || (ecs->stop_func_start == 0
		  && in_solib_dynsym_resolve_code (stop_pc))))
	{
	  /* Any solib trampoline code can be handled in reverse
	     by simply continuing to single-step.  We have already
	     executed the solib function (backwards), and a few 
	     steps will take us back through the trampoline to the
	     caller.  */
	  keep_going (ecs);
	  return;
	}

      if (ecs->event_thread->control.step_over_calls == STEP_OVER_ALL)
	{
	  /* We're doing a "next".

	     Normal (forward) execution: set a breakpoint at the
	     callee's return address (the address at which the caller
	     will resume).

	     Reverse (backward) execution.  set the step-resume
	     breakpoint at the start of the function that we just
	     stepped into (backwards), and continue to there.  When we
	     get there, we'll need to single-step back to the caller.  */

	  if (execution_direction == EXEC_REVERSE)
	    {
	      /* If we're already at the start of the function, we've either
		 just stepped backward into a single instruction function,
		 or stepped back out of a signal handler to the first instruction
		 of the function.  Just keep going, which will single-step back
		 to the caller.  */
	      if (ecs->stop_func_start != stop_pc && ecs->stop_func_start != 0)
		{
		  struct symtab_and_line sr_sal;

		  /* Normal function call return (static or dynamic).  */
		  init_sal (&sr_sal);
		  sr_sal.pc = ecs->stop_func_start;
		  sr_sal.pspace = get_frame_program_space (frame);
		  insert_step_resume_breakpoint_at_sal (gdbarch,
							sr_sal, null_frame_id);
		}
	    }
	  else
	    insert_step_resume_breakpoint_at_caller (frame);

	  keep_going (ecs);
	  return;
	}

      /* If we are in a function call trampoline (a stub between the
         calling routine and the real function), locate the real
         function.  That's what tells us (a) whether we want to step
         into it at all, and (b) what prologue we want to run to the
         end of, if we do step into it.  */
      real_stop_pc = skip_language_trampoline (frame, stop_pc);
      if (real_stop_pc == 0)
	real_stop_pc = gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc);
      if (real_stop_pc != 0)
	ecs->stop_func_start = real_stop_pc;

      if (real_stop_pc != 0 && in_solib_dynsym_resolve_code (real_stop_pc))
	{
	  struct symtab_and_line sr_sal;

	  init_sal (&sr_sal);
	  sr_sal.pc = ecs->stop_func_start;
	  sr_sal.pspace = get_frame_program_space (frame);

	  insert_step_resume_breakpoint_at_sal (gdbarch,
						sr_sal, null_frame_id);
	  keep_going (ecs);
	  return;
	}

      /* If we have line number information for the function we are
	 thinking of stepping into and the function isn't on the skip
	 list, step into it.

         If there are several symtabs at that PC (e.g. with include
         files), just want to know whether *any* of them have line
         numbers.  find_pc_line handles this.  */
      {
	struct symtab_and_line tmp_sal;

	tmp_sal = find_pc_line (ecs->stop_func_start, 0);
	if (tmp_sal.line != 0
	    && !function_name_is_marked_for_skip (ecs->stop_func_name,
						  &tmp_sal))
	  {
	    if (execution_direction == EXEC_REVERSE)
	      handle_step_into_function_backward (gdbarch, ecs);
	    else
	      handle_step_into_function (gdbarch, ecs);
	    return;
	  }
      }

      /* If we have no line number and the step-stop-if-no-debug is
         set, we stop the step so that the user has a chance to switch
         in assembly mode.  */
      if (ecs->event_thread->control.step_over_calls == STEP_OVER_UNDEBUGGABLE
	  && step_stop_if_no_debug)
	{
	  end_stepping_range (ecs);
	  return;
	}

      if (execution_direction == EXEC_REVERSE)
	{
	  /* If we're already at the start of the function, we've either just
	     stepped backward into a single instruction function without line
	     number info, or stepped back out of a signal handler to the first
	     instruction of the function without line number info.  Just keep
	     going, which will single-step back to the caller.  */
	  if (ecs->stop_func_start != stop_pc)
	    {
	      /* Set a breakpoint at callee's start address.
		 From there we can step once and be back in the caller.  */
	      struct symtab_and_line sr_sal;

	      init_sal (&sr_sal);
	      sr_sal.pc = ecs->stop_func_start;
	      sr_sal.pspace = get_frame_program_space (frame);
	      insert_step_resume_breakpoint_at_sal (gdbarch,
						    sr_sal, null_frame_id);
	    }
	}
      else
	/* Set a breakpoint at callee's return address (the address
	   at which the caller will resume).  */
	insert_step_resume_breakpoint_at_caller (frame);

      keep_going (ecs);
      return;
    }

  /* Reverse stepping through solib trampolines.  */

  if (execution_direction == EXEC_REVERSE
      && ecs->event_thread->control.step_over_calls != STEP_OVER_NONE)
    {
      if (gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc)
	  || (ecs->stop_func_start == 0
	      && in_solib_dynsym_resolve_code (stop_pc)))
	{
	  /* Any solib trampoline code can be handled in reverse
	     by simply continuing to single-step.  We have already
	     executed the solib function (backwards), and a few 
	     steps will take us back through the trampoline to the
	     caller.  */
	  keep_going (ecs);
	  return;
	}
      else if (in_solib_dynsym_resolve_code (stop_pc))
	{
	  /* Stepped backward into the solib dynsym resolver.
	     Set a breakpoint at its start and continue, then
	     one more step will take us out.  */
	  struct symtab_and_line sr_sal;

	  init_sal (&sr_sal);
	  sr_sal.pc = ecs->stop_func_start;
	  sr_sal.pspace = get_frame_program_space (frame);
	  insert_step_resume_breakpoint_at_sal (gdbarch, 
						sr_sal, null_frame_id);
	  keep_going (ecs);
	  return;
	}
    }

  stop_pc_sal = find_pc_line (stop_pc, 0);

  /* NOTE: tausq/2004-05-24: This if block used to be done before all
     the trampoline processing logic, however, there are some trampolines 
     that have no names, so we should do trampoline handling first.  */
  if (ecs->event_thread->control.step_over_calls == STEP_OVER_UNDEBUGGABLE
      && ecs->stop_func_name == NULL
      && stop_pc_sal.line == 0)
    {
      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog,
			     "infrun: stepped into undebuggable function\n");

      /* The inferior just stepped into, or returned to, an
         undebuggable function (where there is no debugging information
         and no line number corresponding to the address where the
         inferior stopped).  Since we want to skip this kind of code,
         we keep going until the inferior returns from this
         function - unless the user has asked us not to (via
         set step-mode) or we no longer know how to get back
         to the call site.  */
      if (step_stop_if_no_debug
	  || !frame_id_p (frame_unwind_caller_id (frame)))
	{
	  /* If we have no line number and the step-stop-if-no-debug
	     is set, we stop the step so that the user has a chance to
	     switch in assembly mode.  */
	  end_stepping_range (ecs);
	  return;
	}
      else
	{
	  /* Set a breakpoint at callee's return address (the address
	     at which the caller will resume).  */
	  insert_step_resume_breakpoint_at_caller (frame);
	  keep_going (ecs);
	  return;
	}
    }

  if (ecs->event_thread->control.step_range_end == 1)
    {
      /* It is stepi or nexti.  We always want to stop stepping after
         one instruction.  */
      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog, "infrun: stepi/nexti\n");
      end_stepping_range (ecs);
      return;
    }

  if (stop_pc_sal.line == 0)
    {
      /* We have no line number information.  That means to stop
         stepping (does this always happen right after one instruction,
         when we do "s" in a function with no line numbers,
         or can this happen as a result of a return or longjmp?).  */
      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog, "infrun: no line number info\n");
      end_stepping_range (ecs);
      return;
    }

  /* Look for "calls" to inlined functions, part one.  If the inline
     frame machinery detected some skipped call sites, we have entered
     a new inline function.  */

  if (frame_id_eq (get_frame_id (get_current_frame ()),
		   ecs->event_thread->control.step_frame_id)
      && inline_skipped_frames (ecs->ptid))
    {
      struct symtab_and_line call_sal;

      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: stepped into inlined function\n");

      find_frame_sal (get_current_frame (), &call_sal);

      if (ecs->event_thread->control.step_over_calls != STEP_OVER_ALL)
	{
	  /* For "step", we're going to stop.  But if the call site
	     for this inlined function is on the same source line as
	     we were previously stepping, go down into the function
	     first.  Otherwise stop at the call site.  */

	  if (call_sal.line == ecs->event_thread->current_line
	      && call_sal.symtab == ecs->event_thread->current_symtab)
	    step_into_inline_frame (ecs->ptid);

	  end_stepping_range (ecs);
	  return;
	}
      else
	{
	  /* For "next", we should stop at the call site if it is on a
	     different source line.  Otherwise continue through the
	     inlined function.  */
	  if (call_sal.line == ecs->event_thread->current_line
	      && call_sal.symtab == ecs->event_thread->current_symtab)
	    keep_going (ecs);
	  else
	    end_stepping_range (ecs);
	  return;
	}
    }

  /* Look for "calls" to inlined functions, part two.  If we are still
     in the same real function we were stepping through, but we have
     to go further up to find the exact frame ID, we are stepping
     through a more inlined call beyond its call site.  */

  if (get_frame_type (get_current_frame ()) == INLINE_FRAME
      && !frame_id_eq (get_frame_id (get_current_frame ()),
		       ecs->event_thread->control.step_frame_id)
      && stepped_in_from (get_current_frame (),
			  ecs->event_thread->control.step_frame_id))
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: stepping through inlined function\n");

      if (ecs->event_thread->control.step_over_calls == STEP_OVER_ALL)
	keep_going (ecs);
      else
	end_stepping_range (ecs);
      return;
    }

  if ((stop_pc == stop_pc_sal.pc)
      && (ecs->event_thread->current_line != stop_pc_sal.line
 	  || ecs->event_thread->current_symtab != stop_pc_sal.symtab))
    {
      /* We are at the start of a different line.  So stop.  Note that
         we don't stop if we step into the middle of a different line.
         That is said to make things like for (;;) statements work
         better.  */
      if (debug_infrun)
	 fprintf_unfiltered (gdb_stdlog,
			     "infrun: stepped to a different line\n");
      end_stepping_range (ecs);
      return;
    }

  /* We aren't done stepping.

     Optimize by setting the stepping range to the line.
     (We might not be in the original line, but if we entered a
     new line in mid-statement, we continue stepping.  This makes
     things like for(;;) statements work better.)  */

  ecs->event_thread->control.step_range_start = stop_pc_sal.pc;
  ecs->event_thread->control.step_range_end = stop_pc_sal.end;
  ecs->event_thread->control.may_range_step = 1;
  set_step_info (frame, stop_pc_sal);

  if (debug_infrun)
     fprintf_unfiltered (gdb_stdlog, "infrun: keep going\n");
  keep_going (ecs);
}

/* In all-stop mode, if we're currently stepping but have stopped in
   some other thread, we may need to switch back to the stepped
   thread.  Returns true we set the inferior running, false if we left
   it stopped (and the event needs further processing).  */

static int
switch_back_to_stepped_thread (struct execution_control_state *ecs)
{
  if (!target_is_non_stop_p ())
    {
      struct thread_info *tp;
      struct thread_info *stepping_thread;

      /* If any thread is blocked on some internal breakpoint, and we
	 simply need to step over that breakpoint to get it going
	 again, do that first.  */

      /* However, if we see an event for the stepping thread, then we
	 know all other threads have been moved past their breakpoints
	 already.  Let the caller check whether the step is finished,
	 etc., before deciding to move it past a breakpoint.  */
      if (ecs->event_thread->control.step_range_end != 0)
	return 0;

      /* Check if the current thread is blocked on an incomplete
	 step-over, interrupted by a random signal.  */
      if (ecs->event_thread->control.trap_expected
	  && ecs->event_thread->suspend.stop_signal != GDB_SIGNAL_TRAP)
	{
	  if (debug_infrun)
	    {
	      fprintf_unfiltered (gdb_stdlog,
				  "infrun: need to finish step-over of [%s]\n",
				  target_pid_to_str (ecs->event_thread->ptid));
	    }
	  keep_going (ecs);
	  return 1;
	}

      /* Check if the current thread is blocked by a single-step
	 breakpoint of another thread.  */
      if (ecs->hit_singlestep_breakpoint)
       {
	 if (debug_infrun)
	   {
	     fprintf_unfiltered (gdb_stdlog,
				 "infrun: need to step [%s] over single-step "
				 "breakpoint\n",
				 target_pid_to_str (ecs->ptid));
	   }
	 keep_going (ecs);
	 return 1;
       }

      /* If this thread needs yet another step-over (e.g., stepping
	 through a delay slot), do it first before moving on to
	 another thread.  */
      if (thread_still_needs_step_over (ecs->event_thread))
	{
	  if (debug_infrun)
	    {
	      fprintf_unfiltered (gdb_stdlog,
				  "infrun: thread [%s] still needs step-over\n",
				  target_pid_to_str (ecs->event_thread->ptid));
	    }
	  keep_going (ecs);
	  return 1;
	}

      /* If scheduler locking applies even if not stepping, there's no
	 need to walk over threads.  Above we've checked whether the
	 current thread is stepping.  If some other thread not the
	 event thread is stepping, then it must be that scheduler
	 locking is not in effect.  */
      if (schedlock_applies (ecs->event_thread))
	return 0;

      /* Otherwise, we no longer expect a trap in the current thread.
	 Clear the trap_expected flag before switching back -- this is
	 what keep_going does as well, if we call it.  */
      ecs->event_thread->control.trap_expected = 0;

      /* Likewise, clear the signal if it should not be passed.  */
      if (!signal_program[ecs->event_thread->suspend.stop_signal])
	ecs->event_thread->suspend.stop_signal = GDB_SIGNAL_0;

      /* Do all pending step-overs before actually proceeding with
	 step/next/etc.  */
      if (start_step_over ())
	{
	  prepare_to_wait (ecs);
	  return 1;
	}

      /* Look for the stepping/nexting thread.  */
      stepping_thread = NULL;

      ALL_NON_EXITED_THREADS (tp)
        {
	  /* Ignore threads of processes the caller is not
	     resuming.  */
	  if (!sched_multi
	      && ptid_get_pid (tp->ptid) != ptid_get_pid (ecs->ptid))
	    continue;

	  /* When stepping over a breakpoint, we lock all threads
	     except the one that needs to move past the breakpoint.
	     If a non-event thread has this set, the "incomplete
	     step-over" check above should have caught it earlier.  */
	  if (tp->control.trap_expected)
	    {
	      internal_error (__FILE__, __LINE__,
			      "[%s] has inconsistent state: "
			      "trap_expected=%d\n",
			      target_pid_to_str (tp->ptid),
			      tp->control.trap_expected);
	    }

	  /* Did we find the stepping thread?  */
	  if (tp->control.step_range_end)
	    {
	      /* Yep.  There should only one though.  */
	      gdb_assert (stepping_thread == NULL);

	      /* The event thread is handled at the top, before we
		 enter this loop.  */
	      gdb_assert (tp != ecs->event_thread);

	      /* If some thread other than the event thread is
		 stepping, then scheduler locking can't be in effect,
		 otherwise we wouldn't have resumed the current event
		 thread in the first place.  */
	      gdb_assert (!schedlock_applies (tp));

	      stepping_thread = tp;
	    }
	}

      if (stepping_thread != NULL)
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: switching back to stepped thread\n");

	  if (keep_going_stepped_thread (stepping_thread))
	    {
	      prepare_to_wait (ecs);
	      return 1;
	    }
	}
    }

  return 0;
}

/* Set a previously stepped thread back to stepping.  Returns true on
   success, false if the resume is not possible (e.g., the thread
   vanished).  */

static int
keep_going_stepped_thread (struct thread_info *tp)
{
  struct frame_info *frame;
  struct execution_control_state ecss;
  struct execution_control_state *ecs = &ecss;

  /* If the stepping thread exited, then don't try to switch back and
     resume it, which could fail in several different ways depending
     on the target.  Instead, just keep going.

     We can find a stepping dead thread in the thread list in two
     cases:

     - The target supports thread exit events, and when the target
       tries to delete the thread from the thread list, inferior_ptid
       pointed at the exiting thread.  In such case, calling
       delete_thread does not really remove the thread from the list;
       instead, the thread is left listed, with 'exited' state.

     - The target's debug interface does not support thread exit
       events, and so we have no idea whatsoever if the previously
       stepping thread is still alive.  For that reason, we need to
       synchronously query the target now.  */

  if (is_exited (tp->ptid)
      || !target_thread_alive (tp->ptid))
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: not resuming previously  "
			    "stepped thread, it has vanished\n");

      delete_thread (tp->ptid);
      return 0;
    }

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: resuming previously stepped thread\n");

  reset_ecs (ecs, tp);
  switch_to_thread (tp->ptid);

  stop_pc = regcache_read_pc (get_thread_regcache (tp->ptid));
  frame = get_current_frame ();

  /* If the PC of the thread we were trying to single-step has
     changed, then that thread has trapped or been signaled, but the
     event has not been reported to GDB yet.  Re-poll the target
     looking for this particular thread's event (i.e. temporarily
     enable schedlock) by:

     - setting a break at the current PC
     - resuming that particular thread, only (by setting trap
     expected)

     This prevents us continuously moving the single-step breakpoint
     forward, one instruction at a time, overstepping.  */

  if (stop_pc != tp->prev_pc)
    {
      ptid_t resume_ptid;

      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: expected thread advanced also (%s -> %s)\n",
			    paddress (target_gdbarch (), tp->prev_pc),
			    paddress (target_gdbarch (), stop_pc));

      /* Clear the info of the previous step-over, as it's no longer
	 valid (if the thread was trying to step over a breakpoint, it
	 has already succeeded).  It's what keep_going would do too,
	 if we called it.  Do this before trying to insert the sss
	 breakpoint, otherwise if we were previously trying to step
	 over this exact address in another thread, the breakpoint is
	 skipped.  */
      clear_step_over_info ();
      tp->control.trap_expected = 0;

      insert_single_step_breakpoint (get_frame_arch (frame),
				     get_frame_address_space (frame),
				     stop_pc);

      tp->resumed = 1;
      resume_ptid = internal_resume_ptid (tp->control.stepping_command);
      do_target_resume (resume_ptid, 0, GDB_SIGNAL_0);
    }
  else
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: expected thread still hasn't advanced\n");

      keep_going_pass_signal (ecs);
    }
  return 1;
}

/* Is thread TP in the middle of (software or hardware)
   single-stepping?  (Note the result of this function must never be
   passed directly as target_resume's STEP parameter.)  */

static int
currently_stepping (struct thread_info *tp)
{
  return ((tp->control.step_range_end
	   && tp->control.step_resume_breakpoint == NULL)
	  || tp->control.trap_expected
	  || tp->stepped_breakpoint
	  || bpstat_should_step ());
}

/* Inferior has stepped into a subroutine call with source code that
   we should not step over.  Do step to the first line of code in
   it.  */

static void
handle_step_into_function (struct gdbarch *gdbarch,
			   struct execution_control_state *ecs)
{
  struct compunit_symtab *cust;
  struct symtab_and_line stop_func_sal, sr_sal;

  fill_in_stop_func (gdbarch, ecs);

  cust = find_pc_compunit_symtab (stop_pc);
  if (cust != NULL && compunit_language (cust) != language_asm)
    ecs->stop_func_start = gdbarch_skip_prologue (gdbarch,
						  ecs->stop_func_start);

  stop_func_sal = find_pc_line (ecs->stop_func_start, 0);
  /* Use the step_resume_break to step until the end of the prologue,
     even if that involves jumps (as it seems to on the vax under
     4.2).  */
  /* If the prologue ends in the middle of a source line, continue to
     the end of that source line (if it is still within the function).
     Otherwise, just go to end of prologue.  */
  if (stop_func_sal.end
      && stop_func_sal.pc != ecs->stop_func_start
      && stop_func_sal.end < ecs->stop_func_end)
    ecs->stop_func_start = stop_func_sal.end;

  /* Architectures which require breakpoint adjustment might not be able
     to place a breakpoint at the computed address.  If so, the test
     ``ecs->stop_func_start == stop_pc'' will never succeed.  Adjust
     ecs->stop_func_start to an address at which a breakpoint may be
     legitimately placed.

     Note:  kevinb/2004-01-19:  On FR-V, if this adjustment is not
     made, GDB will enter an infinite loop when stepping through
     optimized code consisting of VLIW instructions which contain
     subinstructions corresponding to different source lines.  On
     FR-V, it's not permitted to place a breakpoint on any but the
     first subinstruction of a VLIW instruction.  When a breakpoint is
     set, GDB will adjust the breakpoint address to the beginning of
     the VLIW instruction.  Thus, we need to make the corresponding
     adjustment here when computing the stop address.  */

  if (gdbarch_adjust_breakpoint_address_p (gdbarch))
    {
      ecs->stop_func_start
	= gdbarch_adjust_breakpoint_address (gdbarch,
					     ecs->stop_func_start);
    }

  if (ecs->stop_func_start == stop_pc)
    {
      /* We are already there: stop now.  */
      end_stepping_range (ecs);
      return;
    }
  else
    {
      /* Put the step-breakpoint there and go until there.  */
      init_sal (&sr_sal);	/* initialize to zeroes */
      sr_sal.pc = ecs->stop_func_start;
      sr_sal.section = find_pc_overlay (ecs->stop_func_start);
      sr_sal.pspace = get_frame_program_space (get_current_frame ());

      /* Do not specify what the fp should be when we stop since on
         some machines the prologue is where the new fp value is
         established.  */
      insert_step_resume_breakpoint_at_sal (gdbarch, sr_sal, null_frame_id);

      /* And make sure stepping stops right away then.  */
      ecs->event_thread->control.step_range_end
        = ecs->event_thread->control.step_range_start;
    }
  keep_going (ecs);
}

/* Inferior has stepped backward into a subroutine call with source
   code that we should not step over.  Do step to the beginning of the
   last line of code in it.  */

static void
handle_step_into_function_backward (struct gdbarch *gdbarch,
				    struct execution_control_state *ecs)
{
  struct compunit_symtab *cust;
  struct symtab_and_line stop_func_sal;

  fill_in_stop_func (gdbarch, ecs);

  cust = find_pc_compunit_symtab (stop_pc);
  if (cust != NULL && compunit_language (cust) != language_asm)
    ecs->stop_func_start = gdbarch_skip_prologue (gdbarch,
						  ecs->stop_func_start);

  stop_func_sal = find_pc_line (stop_pc, 0);

  /* OK, we're just going to keep stepping here.  */
  if (stop_func_sal.pc == stop_pc)
    {
      /* We're there already.  Just stop stepping now.  */
      end_stepping_range (ecs);
    }
  else
    {
      /* Else just reset the step range and keep going.
	 No step-resume breakpoint, they don't work for
	 epilogues, which can have multiple entry paths.  */
      ecs->event_thread->control.step_range_start = stop_func_sal.pc;
      ecs->event_thread->control.step_range_end = stop_func_sal.end;
      keep_going (ecs);
    }
  return;
}

/* Insert a "step-resume breakpoint" at SR_SAL with frame ID SR_ID.
   This is used to both functions and to skip over code.  */

static void
insert_step_resume_breakpoint_at_sal_1 (struct gdbarch *gdbarch,
					struct symtab_and_line sr_sal,
					struct frame_id sr_id,
					enum bptype sr_type)
{
  /* There should never be more than one step-resume or longjmp-resume
     breakpoint per thread, so we should never be setting a new
     step_resume_breakpoint when one is already active.  */
  gdb_assert (inferior_thread ()->control.step_resume_breakpoint == NULL);
  gdb_assert (sr_type == bp_step_resume || sr_type == bp_hp_step_resume);

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: inserting step-resume breakpoint at %s\n",
			paddress (gdbarch, sr_sal.pc));

  inferior_thread ()->control.step_resume_breakpoint
    = set_momentary_breakpoint (gdbarch, sr_sal, sr_id, sr_type);
}

void
insert_step_resume_breakpoint_at_sal (struct gdbarch *gdbarch,
				      struct symtab_and_line sr_sal,
				      struct frame_id sr_id)
{
  insert_step_resume_breakpoint_at_sal_1 (gdbarch,
					  sr_sal, sr_id,
					  bp_step_resume);
}

/* Insert a "high-priority step-resume breakpoint" at RETURN_FRAME.pc.
   This is used to skip a potential signal handler.

   This is called with the interrupted function's frame.  The signal
   handler, when it returns, will resume the interrupted function at
   RETURN_FRAME.pc.  */

static void
insert_hp_step_resume_breakpoint_at_frame (struct frame_info *return_frame)
{
  struct symtab_and_line sr_sal;
  struct gdbarch *gdbarch;

  gdb_assert (return_frame != NULL);
  init_sal (&sr_sal);		/* initialize to zeros */

  gdbarch = get_frame_arch (return_frame);
  sr_sal.pc = gdbarch_addr_bits_remove (gdbarch, get_frame_pc (return_frame));
  sr_sal.section = find_pc_overlay (sr_sal.pc);
  sr_sal.pspace = get_frame_program_space (return_frame);

  insert_step_resume_breakpoint_at_sal_1 (gdbarch, sr_sal,
					  get_stack_frame_id (return_frame),
					  bp_hp_step_resume);
}

/* Insert a "step-resume breakpoint" at the previous frame's PC.  This
   is used to skip a function after stepping into it (for "next" or if
   the called function has no debugging information).

   The current function has almost always been reached by single
   stepping a call or return instruction.  NEXT_FRAME belongs to the
   current function, and the breakpoint will be set at the caller's
   resume address.

   This is a separate function rather than reusing
   insert_hp_step_resume_breakpoint_at_frame in order to avoid
   get_prev_frame, which may stop prematurely (see the implementation
   of frame_unwind_caller_id for an example).  */

static void
insert_step_resume_breakpoint_at_caller (struct frame_info *next_frame)
{
  struct symtab_and_line sr_sal;
  struct gdbarch *gdbarch;

  /* We shouldn't have gotten here if we don't know where the call site
     is.  */
  gdb_assert (frame_id_p (frame_unwind_caller_id (next_frame)));

  init_sal (&sr_sal);		/* initialize to zeros */

  gdbarch = frame_unwind_caller_arch (next_frame);
  sr_sal.pc = gdbarch_addr_bits_remove (gdbarch,
					frame_unwind_caller_pc (next_frame));
  sr_sal.section = find_pc_overlay (sr_sal.pc);
  sr_sal.pspace = frame_unwind_program_space (next_frame);

  insert_step_resume_breakpoint_at_sal (gdbarch, sr_sal,
					frame_unwind_caller_id (next_frame));
}

/* Insert a "longjmp-resume" breakpoint at PC.  This is used to set a
   new breakpoint at the target of a jmp_buf.  The handling of
   longjmp-resume uses the same mechanisms used for handling
   "step-resume" breakpoints.  */

static void
insert_longjmp_resume_breakpoint (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  /* There should never be more than one longjmp-resume breakpoint per
     thread, so we should never be setting a new
     longjmp_resume_breakpoint when one is already active.  */
  gdb_assert (inferior_thread ()->control.exception_resume_breakpoint == NULL);

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: inserting longjmp-resume breakpoint at %s\n",
			paddress (gdbarch, pc));

  inferior_thread ()->control.exception_resume_breakpoint =
    set_momentary_breakpoint_at_pc (gdbarch, pc, bp_longjmp_resume);
}

/* Insert an exception resume breakpoint.  TP is the thread throwing
   the exception.  The block B is the block of the unwinder debug hook
   function.  FRAME is the frame corresponding to the call to this
   function.  SYM is the symbol of the function argument holding the
   target PC of the exception.  */

static void
insert_exception_resume_breakpoint (struct thread_info *tp,
				    const struct block *b,
				    struct frame_info *frame,
				    struct symbol *sym)
{
  TRY
    {
      struct block_symbol vsym;
      struct value *value;
      CORE_ADDR handler;
      struct breakpoint *bp;

      vsym = lookup_symbol (SYMBOL_LINKAGE_NAME (sym), b, VAR_DOMAIN, NULL);
      value = read_var_value (vsym.symbol, vsym.block, frame);
      /* If the value was optimized out, revert to the old behavior.  */
      if (! value_optimized_out (value))
	{
	  handler = value_as_address (value);

	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: exception resume at %lx\n",
				(unsigned long) handler);

	  bp = set_momentary_breakpoint_at_pc (get_frame_arch (frame),
					       handler, bp_exception_resume);

	  /* set_momentary_breakpoint_at_pc invalidates FRAME.  */
	  frame = NULL;

	  bp->thread = tp->global_num;
	  inferior_thread ()->control.exception_resume_breakpoint = bp;
	}
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
      /* We want to ignore errors here.  */
    }
  END_CATCH
}

/* A helper for check_exception_resume that sets an
   exception-breakpoint based on a SystemTap probe.  */

static void
insert_exception_resume_from_probe (struct thread_info *tp,
				    const struct bound_probe *probe,
				    struct frame_info *frame)
{
  struct value *arg_value;
  CORE_ADDR handler;
  struct breakpoint *bp;

  arg_value = probe_safe_evaluate_at_pc (frame, 1);
  if (!arg_value)
    return;

  handler = value_as_address (arg_value);

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog,
			"infrun: exception resume at %s\n",
			paddress (get_objfile_arch (probe->objfile),
				  handler));

  bp = set_momentary_breakpoint_at_pc (get_frame_arch (frame),
				       handler, bp_exception_resume);
  bp->thread = tp->global_num;
  inferior_thread ()->control.exception_resume_breakpoint = bp;
}

/* This is called when an exception has been intercepted.  Check to
   see whether the exception's destination is of interest, and if so,
   set an exception resume breakpoint there.  */

static void
check_exception_resume (struct execution_control_state *ecs,
			struct frame_info *frame)
{
  struct bound_probe probe;
  struct symbol *func;

  /* First see if this exception unwinding breakpoint was set via a
     SystemTap probe point.  If so, the probe has two arguments: the
     CFA and the HANDLER.  We ignore the CFA, extract the handler, and
     set a breakpoint there.  */
  probe = find_probe_by_pc (get_frame_pc (frame));
  if (probe.probe)
    {
      insert_exception_resume_from_probe (ecs->event_thread, &probe, frame);
      return;
    }

  func = get_frame_function (frame);
  if (!func)
    return;

  TRY
    {
      const struct block *b;
      struct block_iterator iter;
      struct symbol *sym;
      int argno = 0;

      /* The exception breakpoint is a thread-specific breakpoint on
	 the unwinder's debug hook, declared as:
	 
	 void _Unwind_DebugHook (void *cfa, void *handler);
	 
	 The CFA argument indicates the frame to which control is
	 about to be transferred.  HANDLER is the destination PC.
	 
	 We ignore the CFA and set a temporary breakpoint at HANDLER.
	 This is not extremely efficient but it avoids issues in gdb
	 with computing the DWARF CFA, and it also works even in weird
	 cases such as throwing an exception from inside a signal
	 handler.  */

      b = SYMBOL_BLOCK_VALUE (func);
      ALL_BLOCK_SYMBOLS (b, iter, sym)
	{
	  if (!SYMBOL_IS_ARGUMENT (sym))
	    continue;

	  if (argno == 0)
	    ++argno;
	  else
	    {
	      insert_exception_resume_breakpoint (ecs->event_thread,
						  b, frame, sym);
	      break;
	    }
	}
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
    }
  END_CATCH
}

static void
stop_waiting (struct execution_control_state *ecs)
{
  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog, "infrun: stop_waiting\n");

  clear_step_over_info ();

  /* Let callers know we don't want to wait for the inferior anymore.  */
  ecs->wait_some_more = 0;

  /* If all-stop, but the target is always in non-stop mode, stop all
     threads now that we're presenting the stop to the user.  */
  if (!non_stop && target_is_non_stop_p ())
    stop_all_threads ();
}

/* Like keep_going, but passes the signal to the inferior, even if the
   signal is set to nopass.  */

static void
keep_going_pass_signal (struct execution_control_state *ecs)
{
  /* Make sure normal_stop is called if we get a QUIT handled before
     reaching resume.  */
  struct cleanup *old_cleanups = make_cleanup (resume_cleanups, 0);

  gdb_assert (ptid_equal (ecs->event_thread->ptid, inferior_ptid));
  gdb_assert (!ecs->event_thread->resumed);

  /* Save the pc before execution, to compare with pc after stop.  */
  ecs->event_thread->prev_pc
    = regcache_read_pc (get_thread_regcache (ecs->ptid));

  if (ecs->event_thread->control.trap_expected)
    {
      struct thread_info *tp = ecs->event_thread;

      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog,
			    "infrun: %s has trap_expected set, "
			    "resuming to collect trap\n",
			    target_pid_to_str (tp->ptid));

      /* We haven't yet gotten our trap, and either: intercepted a
	 non-signal event (e.g., a fork); or took a signal which we
	 are supposed to pass through to the inferior.  Simply
	 continue.  */
      discard_cleanups (old_cleanups);
      resume (ecs->event_thread->suspend.stop_signal);
    }
  else if (step_over_info_valid_p ())
    {
      /* Another thread is stepping over a breakpoint in-line.  If
	 this thread needs a step-over too, queue the request.  In
	 either case, this resume must be deferred for later.  */
      struct thread_info *tp = ecs->event_thread;

      if (ecs->hit_singlestep_breakpoint
	  || thread_still_needs_step_over (tp))
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: step-over already in progress: "
				"step-over for %s deferred\n",
				target_pid_to_str (tp->ptid));
	  thread_step_over_chain_enqueue (tp);
	}
      else
	{
	  if (debug_infrun)
	    fprintf_unfiltered (gdb_stdlog,
				"infrun: step-over in progress: "
				"resume of %s deferred\n",
				target_pid_to_str (tp->ptid));
	}

      discard_cleanups (old_cleanups);
    }
  else
    {
      struct regcache *regcache = get_current_regcache ();
      int remove_bp;
      int remove_wps;
      step_over_what step_what;

      /* Either the trap was not expected, but we are continuing
	 anyway (if we got a signal, the user asked it be passed to
	 the child)
	 -- or --
	 We got our expected trap, but decided we should resume from
	 it.

	 We're going to run this baby now!

	 Note that insert_breakpoints won't try to re-insert
	 already inserted breakpoints.  Therefore, we don't
	 care if breakpoints were already inserted, or not.  */

      /* If we need to step over a breakpoint, and we're not using
	 displaced stepping to do so, insert all breakpoints
	 (watchpoints, etc.) but the one we're stepping over, step one
	 instruction, and then re-insert the breakpoint when that step
	 is finished.  */

      step_what = thread_still_needs_step_over (ecs->event_thread);

      remove_bp = (ecs->hit_singlestep_breakpoint
		   || (step_what & STEP_OVER_BREAKPOINT));
      remove_wps = (step_what & STEP_OVER_WATCHPOINT);

      /* We can't use displaced stepping if we need to step past a
	 watchpoint.  The instruction copied to the scratch pad would
	 still trigger the watchpoint.  */
      if (remove_bp
	  && (remove_wps || !use_displaced_stepping (ecs->event_thread)))
	{
	  set_step_over_info (get_regcache_aspace (regcache),
			      regcache_read_pc (regcache), remove_wps,
			      ecs->event_thread->global_num);
	}
      else if (remove_wps)
	set_step_over_info (NULL, 0, remove_wps, -1);

      /* If we now need to do an in-line step-over, we need to stop
	 all other threads.  Note this must be done before
	 insert_breakpoints below, because that removes the breakpoint
	 we're about to step over, otherwise other threads could miss
	 it.  */
      if (step_over_info_valid_p () && target_is_non_stop_p ())
	stop_all_threads ();

      /* Stop stepping if inserting breakpoints fails.  */
      TRY
	{
	  insert_breakpoints ();
	}
      CATCH (e, RETURN_MASK_ERROR)
	{
	  exception_print (gdb_stderr, e);
	  stop_waiting (ecs);
	  discard_cleanups (old_cleanups);
	  return;
	}
      END_CATCH

      ecs->event_thread->control.trap_expected = (remove_bp || remove_wps);

      discard_cleanups (old_cleanups);
      resume (ecs->event_thread->suspend.stop_signal);
    }

  prepare_to_wait (ecs);
}

/* Called when we should continue running the inferior, because the
   current event doesn't cause a user visible stop.  This does the
   resuming part; waiting for the next event is done elsewhere.  */

static void
keep_going (struct execution_control_state *ecs)
{
  if (ecs->event_thread->control.trap_expected
      && ecs->event_thread->suspend.stop_signal == GDB_SIGNAL_TRAP)
    ecs->event_thread->control.trap_expected = 0;

  if (!signal_program[ecs->event_thread->suspend.stop_signal])
    ecs->event_thread->suspend.stop_signal = GDB_SIGNAL_0;
  keep_going_pass_signal (ecs);
}

/* This function normally comes after a resume, before
   handle_inferior_event exits.  It takes care of any last bits of
   housekeeping, and sets the all-important wait_some_more flag.  */

static void
prepare_to_wait (struct execution_control_state *ecs)
{
  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog, "infrun: prepare_to_wait\n");

  ecs->wait_some_more = 1;

  if (!target_is_async_p ())
    mark_infrun_async_event_handler ();
}

/* We are done with the step range of a step/next/si/ni command.
   Called once for each n of a "step n" operation.  */

static void
end_stepping_range (struct execution_control_state *ecs)
{
  ecs->event_thread->control.stop_step = 1;
  stop_waiting (ecs);
}

/* Several print_*_reason functions to print why the inferior has stopped.
   We always print something when the inferior exits, or receives a signal.
   The rest of the cases are dealt with later on in normal_stop and
   print_it_typical.  Ideally there should be a call to one of these
   print_*_reason functions functions from handle_inferior_event each time
   stop_waiting is called.

   Note that we don't call these directly, instead we delegate that to
   the interpreters, through observers.  Interpreters then call these
   with whatever uiout is right.  */

void
print_end_stepping_range_reason (struct ui_out *uiout)
{
  /* For CLI-like interpreters, print nothing.  */

  if (ui_out_is_mi_like_p (uiout))
    {
      ui_out_field_string (uiout, "reason",
			   async_reason_lookup (EXEC_ASYNC_END_STEPPING_RANGE));
    }
}

void
print_signal_exited_reason (struct ui_out *uiout, enum gdb_signal siggnal)
{
  annotate_signalled ();
  if (ui_out_is_mi_like_p (uiout))
    ui_out_field_string
      (uiout, "reason", async_reason_lookup (EXEC_ASYNC_EXITED_SIGNALLED));
  ui_out_text (uiout, "\nProgram terminated with signal ");
  annotate_signal_name ();
  ui_out_field_string (uiout, "signal-name",
		       gdb_signal_to_name (siggnal));
  annotate_signal_name_end ();
  ui_out_text (uiout, ", ");
  annotate_signal_string ();
  ui_out_field_string (uiout, "signal-meaning",
		       gdb_signal_to_string (siggnal));
  annotate_signal_string_end ();
  ui_out_text (uiout, ".\n");
  ui_out_text (uiout, "The program no longer exists.\n");
}

void
print_exited_reason (struct ui_out *uiout, int exitstatus)
{
  struct inferior *inf = current_inferior ();
  const char *pidstr = target_pid_to_str (pid_to_ptid (inf->pid));

  annotate_exited (exitstatus);
  if (exitstatus)
    {
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string (uiout, "reason", 
			     async_reason_lookup (EXEC_ASYNC_EXITED));
      ui_out_text (uiout, "[Inferior ");
      ui_out_text (uiout, plongest (inf->num));
      ui_out_text (uiout, " (");
      ui_out_text (uiout, pidstr);
      ui_out_text (uiout, ") exited with code ");
      ui_out_field_fmt (uiout, "exit-code", "0%o", (unsigned int) exitstatus);
      ui_out_text (uiout, "]\n");
    }
  else
    {
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason", async_reason_lookup (EXEC_ASYNC_EXITED_NORMALLY));
      ui_out_text (uiout, "[Inferior ");
      ui_out_text (uiout, plongest (inf->num));
      ui_out_text (uiout, " (");
      ui_out_text (uiout, pidstr);
      ui_out_text (uiout, ") exited normally]\n");
    }
}

/* Some targets/architectures can do extra processing/display of
   segmentation faults.  E.g., Intel MPX boundary faults.
   Call the architecture dependent function to handle the fault.  */

static void
handle_segmentation_fault (struct ui_out *uiout)
{
  struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  if (gdbarch_handle_segmentation_fault_p (gdbarch))
    gdbarch_handle_segmentation_fault (gdbarch, uiout);
}

void
print_signal_received_reason (struct ui_out *uiout, enum gdb_signal siggnal)
{
  struct thread_info *thr = inferior_thread ();

  annotate_signal ();

  if (ui_out_is_mi_like_p (uiout))
    ;
  else if (show_thread_that_caused_stop ())
    {
      const char *name;

      ui_out_text (uiout, "\nThread ");
      ui_out_field_fmt (uiout, "thread-id", "%s", print_thread_id (thr));

      name = thr->name != NULL ? thr->name : target_thread_name (thr);
      if (name != NULL)
	{
	  ui_out_text (uiout, " \"");
	  ui_out_field_fmt (uiout, "name", "%s", name);
	  ui_out_text (uiout, "\"");
	}
    }
  else
    ui_out_text (uiout, "\nProgram");

  if (siggnal == GDB_SIGNAL_0 && !ui_out_is_mi_like_p (uiout))
    ui_out_text (uiout, " stopped");
  else
    {
      ui_out_text (uiout, " received signal ");
      annotate_signal_name ();
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason", async_reason_lookup (EXEC_ASYNC_SIGNAL_RECEIVED));
      ui_out_field_string (uiout, "signal-name",
			   gdb_signal_to_name (siggnal));
      annotate_signal_name_end ();
      ui_out_text (uiout, ", ");
      annotate_signal_string ();
      ui_out_field_string (uiout, "signal-meaning",
			   gdb_signal_to_string (siggnal));

      if (siggnal == GDB_SIGNAL_SEGV)
	handle_segmentation_fault (uiout);

      annotate_signal_string_end ();
    }
  ui_out_text (uiout, ".\n");
}

void
print_no_history_reason (struct ui_out *uiout)
{
  ui_out_text (uiout, "\nNo more reverse-execution history.\n");
}

/* Print current location without a level number, if we have changed
   functions or hit a breakpoint.  Print source line if we have one.
   bpstat_print contains the logic deciding in detail what to print,
   based on the event(s) that just occurred.  */

static void
print_stop_location (struct target_waitstatus *ws)
{
  int bpstat_ret;
  enum print_what source_flag;
  int do_frame_printing = 1;
  struct thread_info *tp = inferior_thread ();

  bpstat_ret = bpstat_print (tp->control.stop_bpstat, ws->kind);
  switch (bpstat_ret)
    {
    case PRINT_UNKNOWN:
      /* FIXME: cagney/2002-12-01: Given that a frame ID does (or
	 should) carry around the function and does (or should) use
	 that when doing a frame comparison.  */
      if (tp->control.stop_step
	  && frame_id_eq (tp->control.step_frame_id,
			  get_frame_id (get_current_frame ()))
	  && tp->control.step_start_function == find_pc_function (stop_pc))
	{
	  /* Finished step, just print source line.  */
	  source_flag = SRC_LINE;
	}
      else
	{
	  /* Print location and source line.  */
	  source_flag = SRC_AND_LOC;
	}
      break;
    case PRINT_SRC_AND_LOC:
      /* Print location and source line.  */
      source_flag = SRC_AND_LOC;
      break;
    case PRINT_SRC_ONLY:
      source_flag = SRC_LINE;
      break;
    case PRINT_NOTHING:
      /* Something bogus.  */
      source_flag = SRC_LINE;
      do_frame_printing = 0;
      break;
    default:
      internal_error (__FILE__, __LINE__, _("Unknown value."));
    }

  /* The behavior of this routine with respect to the source
     flag is:
     SRC_LINE: Print only source line
     LOCATION: Print only location
     SRC_AND_LOC: Print location and source line.  */
  if (do_frame_printing)
    print_stack_frame (get_selected_frame (NULL), 0, source_flag, 1);
}

/* See infrun.h.  */

void
print_stop_event (struct ui_out *uiout)
{
  struct cleanup *old_chain;
  struct target_waitstatus last;
  ptid_t last_ptid;
  struct thread_info *tp;

  get_last_target_status (&last_ptid, &last);

  old_chain = make_cleanup_restore_current_uiout ();
  current_uiout = uiout;

  print_stop_location (&last);

  /* Display the auto-display expressions.  */
  do_displays ();

  do_cleanups (old_chain);

  tp = inferior_thread ();
  if (tp->thread_fsm != NULL
      && thread_fsm_finished_p (tp->thread_fsm))
    {
      struct return_value_info *rv;

      rv = thread_fsm_return_value (tp->thread_fsm);
      if (rv != NULL)
	print_return_value (uiout, rv);
    }
}

/* See infrun.h.  */

void
maybe_remove_breakpoints (void)
{
  if (!breakpoints_should_be_inserted_now () && target_has_execution)
    {
      if (remove_breakpoints ())
	{
	  target_terminal_ours_for_output ();
	  printf_filtered (_("Cannot remove breakpoints because "
			     "program is no longer writable.\nFurther "
			     "execution is probably impossible.\n"));
	}
    }
}

/* The execution context that just caused a normal stop.  */

struct stop_context
{
  /* The stop ID.  */
  ULONGEST stop_id;

  /* The event PTID.  */

  ptid_t ptid;

  /* If stopp for a thread event, this is the thread that caused the
     stop.  */
  struct thread_info *thread;

  /* The inferior that caused the stop.  */
  int inf_num;
};

/* Returns a new stop context.  If stopped for a thread event, this
   takes a strong reference to the thread.  */

static struct stop_context *
save_stop_context (void)
{
  struct stop_context *sc = XNEW (struct stop_context);

  sc->stop_id = get_stop_id ();
  sc->ptid = inferior_ptid;
  sc->inf_num = current_inferior ()->num;

  if (!ptid_equal (inferior_ptid, null_ptid))
    {
      /* Take a strong reference so that the thread can't be deleted
	 yet.  */
      sc->thread = inferior_thread ();
      sc->thread->refcount++;
    }
  else
    sc->thread = NULL;

  return sc;
}

/* Release a stop context previously created with save_stop_context.
   Releases the strong reference to the thread as well. */

static void
release_stop_context_cleanup (void *arg)
{
  struct stop_context *sc = (struct stop_context *) arg;

  if (sc->thread != NULL)
    sc->thread->refcount--;
  xfree (sc);
}

/* Return true if the current context no longer matches the saved stop
   context.  */

static int
stop_context_changed (struct stop_context *prev)
{
  if (!ptid_equal (prev->ptid, inferior_ptid))
    return 1;
  if (prev->inf_num != current_inferior ()->num)
    return 1;
  if (prev->thread != NULL && prev->thread->state != THREAD_STOPPED)
    return 1;
  if (get_stop_id () != prev->stop_id)
    return 1;
  return 0;
}

/* See infrun.h.  */

int
normal_stop (void)
{
  struct target_waitstatus last;
  ptid_t last_ptid;
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);
  ptid_t pid_ptid;
  struct switch_thru_all_uis state;

  get_last_target_status (&last_ptid, &last);

  new_stop_id ();

  /* If an exception is thrown from this point on, make sure to
     propagate GDB's knowledge of the executing state to the
     frontend/user running state.  A QUIT is an easy exception to see
     here, so do this before any filtered output.  */
  if (!non_stop)
    make_cleanup (finish_thread_state_cleanup, &minus_one_ptid);
  else if (last.kind == TARGET_WAITKIND_SIGNALLED
	   || last.kind == TARGET_WAITKIND_EXITED)
    {
      /* On some targets, we may still have live threads in the
	 inferior when we get a process exit event.  E.g., for
	 "checkpoint", when the current checkpoint/fork exits,
	 linux-fork.c automatically switches to another fork from
	 within target_mourn_inferior.  */
      if (!ptid_equal (inferior_ptid, null_ptid))
	{
	  pid_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));
	  make_cleanup (finish_thread_state_cleanup, &pid_ptid);
	}
    }
  else if (last.kind != TARGET_WAITKIND_NO_RESUMED)
    make_cleanup (finish_thread_state_cleanup, &inferior_ptid);

  /* As we're presenting a stop, and potentially removing breakpoints,
     update the thread list so we can tell whether there are threads
     running on the target.  With target remote, for example, we can
     only learn about new threads when we explicitly update the thread
     list.  Do this before notifying the interpreters about signal
     stops, end of stepping ranges, etc., so that the "new thread"
     output is emitted before e.g., "Program received signal FOO",
     instead of after.  */
  update_thread_list ();

  if (last.kind == TARGET_WAITKIND_STOPPED && stopped_by_random_signal)
    observer_notify_signal_received (inferior_thread ()->suspend.stop_signal);

  /* As with the notification of thread events, we want to delay
     notifying the user that we've switched thread context until
     the inferior actually stops.

     There's no point in saying anything if the inferior has exited.
     Note that SIGNALLED here means "exited with a signal", not
     "received a signal".

     Also skip saying anything in non-stop mode.  In that mode, as we
     don't want GDB to switch threads behind the user's back, to avoid
     races where the user is typing a command to apply to thread x,
     but GDB switches to thread y before the user finishes entering
     the command, fetch_inferior_event installs a cleanup to restore
     the current thread back to the thread the user had selected right
     after this event is handled, so we're not really switching, only
     informing of a stop.  */
  if (!non_stop
      && !ptid_equal (previous_inferior_ptid, inferior_ptid)
      && target_has_execution
      && last.kind != TARGET_WAITKIND_SIGNALLED
      && last.kind != TARGET_WAITKIND_EXITED
      && last.kind != TARGET_WAITKIND_NO_RESUMED)
    {
      SWITCH_THRU_ALL_UIS (state)
	{
	  target_terminal_ours_for_output ();
	  printf_filtered (_("[Switching to %s]\n"),
			   target_pid_to_str (inferior_ptid));
	  annotate_thread_changed ();
	}
      previous_inferior_ptid = inferior_ptid;
    }

  if (last.kind == TARGET_WAITKIND_NO_RESUMED)
    {
      SWITCH_THRU_ALL_UIS (state)
	if (current_ui->prompt_state == PROMPT_BLOCKED)
	  {
	    target_terminal_ours_for_output ();
	    printf_filtered (_("No unwaited-for children left.\n"));
	  }
    }

  /* Note: this depends on the update_thread_list call above.  */
  maybe_remove_breakpoints ();

  /* If an auto-display called a function and that got a signal,
     delete that auto-display to avoid an infinite recursion.  */

  if (stopped_by_random_signal)
    disable_current_display ();

  SWITCH_THRU_ALL_UIS (state)
    {
      async_enable_stdin ();
    }

  /* Let the user/frontend see the threads as stopped.  */
  do_cleanups (old_chain);

  /* Select innermost stack frame - i.e., current frame is frame 0,
     and current location is based on that.  Handle the case where the
     dummy call is returning after being stopped.  E.g. the dummy call
     previously hit a breakpoint.  (If the dummy call returns
     normally, we won't reach here.)  Do this before the stop hook is
     run, so that it doesn't get to see the temporary dummy frame,
     which is not where we'll present the stop.  */
  if (has_stack_frames ())
    {
      if (stop_stack_dummy == STOP_STACK_DUMMY)
	{
	  /* Pop the empty frame that contains the stack dummy.  This
	     also restores inferior state prior to the call (struct
	     infcall_suspend_state).  */
	  struct frame_info *frame = get_current_frame ();

	  gdb_assert (get_frame_type (frame) == DUMMY_FRAME);
	  frame_pop (frame);
	  /* frame_pop calls reinit_frame_cache as the last thing it
	     does which means there's now no selected frame.  */
	}

      select_frame (get_current_frame ());

      /* Set the current source location.  */
      set_current_sal_from_frame (get_current_frame ());
    }

  /* Look up the hook_stop and run it (CLI internally handles problem
     of stop_command's pre-hook not existing).  */
  if (stop_command != NULL)
    {
      struct stop_context *saved_context = save_stop_context ();
      struct cleanup *old_chain
	= make_cleanup (release_stop_context_cleanup, saved_context);

      catch_errors (hook_stop_stub, stop_command,
		    "Error while running hook_stop:\n", RETURN_MASK_ALL);

      /* If the stop hook resumes the target, then there's no point in
	 trying to notify about the previous stop; its context is
	 gone.  Likewise if the command switches thread or inferior --
	 the observers would print a stop for the wrong
	 thread/inferior.  */
      if (stop_context_changed (saved_context))
	{
	  do_cleanups (old_chain);
	  return 1;
	}
      do_cleanups (old_chain);
    }

  /* Notify observers about the stop.  This is where the interpreters
     print the stop event.  */
  if (!ptid_equal (inferior_ptid, null_ptid))
    observer_notify_normal_stop (inferior_thread ()->control.stop_bpstat,
				 stop_print_frame);
  else
    observer_notify_normal_stop (NULL, stop_print_frame);

  annotate_stopped ();

  if (target_has_execution)
    {
      if (last.kind != TARGET_WAITKIND_SIGNALLED
	  && last.kind != TARGET_WAITKIND_EXITED)
	/* Delete the breakpoint we stopped at, if it wants to be deleted.
	   Delete any breakpoint that is to be deleted at the next stop.  */
	breakpoint_auto_delete (inferior_thread ()->control.stop_bpstat);
    }

  /* Try to get rid of automatically added inferiors that are no
     longer needed.  Keeping those around slows down things linearly.
     Note that this never removes the current inferior.  */
  prune_inferiors ();

  return 0;
}

static int
hook_stop_stub (void *cmd)
{
  execute_cmd_pre_hook ((struct cmd_list_element *) cmd);
  return (0);
}

int
signal_stop_state (int signo)
{
  return signal_stop[signo];
}

int
signal_print_state (int signo)
{
  return signal_print[signo];
}

int
signal_pass_state (int signo)
{
  return signal_program[signo];
}

static void
signal_cache_update (int signo)
{
  if (signo == -1)
    {
      for (signo = 0; signo < (int) GDB_SIGNAL_LAST; signo++)
	signal_cache_update (signo);

      return;
    }

  signal_pass[signo] = (signal_stop[signo] == 0
			&& signal_print[signo] == 0
			&& signal_program[signo] == 1
			&& signal_catch[signo] == 0);
}

int
signal_stop_update (int signo, int state)
{
  int ret = signal_stop[signo];

  signal_stop[signo] = state;
  signal_cache_update (signo);
  return ret;
}

int
signal_print_update (int signo, int state)
{
  int ret = signal_print[signo];

  signal_print[signo] = state;
  signal_cache_update (signo);
  return ret;
}

int
signal_pass_update (int signo, int state)
{
  int ret = signal_program[signo];

  signal_program[signo] = state;
  signal_cache_update (signo);
  return ret;
}

/* Update the global 'signal_catch' from INFO and notify the
   target.  */

void
signal_catch_update (const unsigned int *info)
{
  int i;

  for (i = 0; i < GDB_SIGNAL_LAST; ++i)
    signal_catch[i] = info[i] > 0;
  signal_cache_update (-1);
  target_pass_signals ((int) GDB_SIGNAL_LAST, signal_pass);
}

static void
sig_print_header (void)
{
  printf_filtered (_("Signal        Stop\tPrint\tPass "
		     "to program\tDescription\n"));
}

static void
sig_print_info (enum gdb_signal oursig)
{
  const char *name = gdb_signal_to_name (oursig);
  int name_padding = 13 - strlen (name);

  if (name_padding <= 0)
    name_padding = 0;

  printf_filtered ("%s", name);
  printf_filtered ("%*.*s ", name_padding, name_padding, "                 ");
  printf_filtered ("%s\t", signal_stop[oursig] ? "Yes" : "No");
  printf_filtered ("%s\t", signal_print[oursig] ? "Yes" : "No");
  printf_filtered ("%s\t\t", signal_program[oursig] ? "Yes" : "No");
  printf_filtered ("%s\n", gdb_signal_to_string (oursig));
}

/* Specify how various signals in the inferior should be handled.  */

static void
handle_command (char *args, int from_tty)
{
  char **argv;
  int digits, wordlen;
  int sigfirst, signum, siglast;
  enum gdb_signal oursig;
  int allsigs;
  int nsigs;
  unsigned char *sigs;
  struct cleanup *old_chain;

  if (args == NULL)
    {
      error_no_arg (_("signal to handle"));
    }

  /* Allocate and zero an array of flags for which signals to handle.  */

  nsigs = (int) GDB_SIGNAL_LAST;
  sigs = (unsigned char *) alloca (nsigs);
  memset (sigs, 0, nsigs);

  /* Break the command line up into args.  */

  argv = gdb_buildargv (args);
  old_chain = make_cleanup_freeargv (argv);

  /* Walk through the args, looking for signal oursigs, signal names, and
     actions.  Signal numbers and signal names may be interspersed with
     actions, with the actions being performed for all signals cumulatively
     specified.  Signal ranges can be specified as <LOW>-<HIGH>.  */

  while (*argv != NULL)
    {
      wordlen = strlen (*argv);
      for (digits = 0; isdigit ((*argv)[digits]); digits++)
	{;
	}
      allsigs = 0;
      sigfirst = siglast = -1;

      if (wordlen >= 1 && !strncmp (*argv, "all", wordlen))
	{
	  /* Apply action to all signals except those used by the
	     debugger.  Silently skip those.  */
	  allsigs = 1;
	  sigfirst = 0;
	  siglast = nsigs - 1;
	}
      else if (wordlen >= 1 && !strncmp (*argv, "stop", wordlen))
	{
	  SET_SIGS (nsigs, sigs, signal_stop);
	  SET_SIGS (nsigs, sigs, signal_print);
	}
      else if (wordlen >= 1 && !strncmp (*argv, "ignore", wordlen))
	{
	  UNSET_SIGS (nsigs, sigs, signal_program);
	}
      else if (wordlen >= 2 && !strncmp (*argv, "print", wordlen))
	{
	  SET_SIGS (nsigs, sigs, signal_print);
	}
      else if (wordlen >= 2 && !strncmp (*argv, "pass", wordlen))
	{
	  SET_SIGS (nsigs, sigs, signal_program);
	}
      else if (wordlen >= 3 && !strncmp (*argv, "nostop", wordlen))
	{
	  UNSET_SIGS (nsigs, sigs, signal_stop);
	}
      else if (wordlen >= 3 && !strncmp (*argv, "noignore", wordlen))
	{
	  SET_SIGS (nsigs, sigs, signal_program);
	}
      else if (wordlen >= 4 && !strncmp (*argv, "noprint", wordlen))
	{
	  UNSET_SIGS (nsigs, sigs, signal_print);
	  UNSET_SIGS (nsigs, sigs, signal_stop);
	}
      else if (wordlen >= 4 && !strncmp (*argv, "nopass", wordlen))
	{
	  UNSET_SIGS (nsigs, sigs, signal_program);
	}
      else if (digits > 0)
	{
	  /* It is numeric.  The numeric signal refers to our own
	     internal signal numbering from target.h, not to host/target
	     signal  number.  This is a feature; users really should be
	     using symbolic names anyway, and the common ones like
	     SIGHUP, SIGINT, SIGALRM, etc. will work right anyway.  */

	  sigfirst = siglast = (int)
	    gdb_signal_from_command (atoi (*argv));
	  if ((*argv)[digits] == '-')
	    {
	      siglast = (int)
		gdb_signal_from_command (atoi ((*argv) + digits + 1));
	    }
	  if (sigfirst > siglast)
	    {
	      /* Bet he didn't figure we'd think of this case...  */
	      signum = sigfirst;
	      sigfirst = siglast;
	      siglast = signum;
	    }
	}
      else
	{
	  oursig = gdb_signal_from_name (*argv);
	  if (oursig != GDB_SIGNAL_UNKNOWN)
	    {
	      sigfirst = siglast = (int) oursig;
	    }
	  else
	    {
	      /* Not a number and not a recognized flag word => complain.  */
	      error (_("Unrecognized or ambiguous flag word: \"%s\"."), *argv);
	    }
	}

      /* If any signal numbers or symbol names were found, set flags for
         which signals to apply actions to.  */

      for (signum = sigfirst; signum >= 0 && signum <= siglast; signum++)
	{
	  switch ((enum gdb_signal) signum)
	    {
	    case GDB_SIGNAL_TRAP:
	    case GDB_SIGNAL_INT:
	      if (!allsigs && !sigs[signum])
		{
		  if (query (_("%s is used by the debugger.\n\
Are you sure you want to change it? "),
			     gdb_signal_to_name ((enum gdb_signal) signum)))
		    {
		      sigs[signum] = 1;
		    }
		  else
		    {
		      printf_unfiltered (_("Not confirmed, unchanged.\n"));
		      gdb_flush (gdb_stdout);
		    }
		}
	      break;
	    case GDB_SIGNAL_0:
	    case GDB_SIGNAL_DEFAULT:
	    case GDB_SIGNAL_UNKNOWN:
	      /* Make sure that "all" doesn't print these.  */
	      break;
	    default:
	      sigs[signum] = 1;
	      break;
	    }
	}

      argv++;
    }

  for (signum = 0; signum < nsigs; signum++)
    if (sigs[signum])
      {
	signal_cache_update (-1);
	target_pass_signals ((int) GDB_SIGNAL_LAST, signal_pass);
	target_program_signals ((int) GDB_SIGNAL_LAST, signal_program);

	if (from_tty)
	  {
	    /* Show the results.  */
	    sig_print_header ();
	    for (; signum < nsigs; signum++)
	      if (sigs[signum])
		sig_print_info ((enum gdb_signal) signum);
	  }

	break;
      }

  do_cleanups (old_chain);
}

/* Complete the "handle" command.  */

static VEC (char_ptr) *
handle_completer (struct cmd_list_element *ignore,
		  const char *text, const char *word)
{
  VEC (char_ptr) *vec_signals, *vec_keywords, *return_val;
  static const char * const keywords[] =
    {
      "all",
      "stop",
      "ignore",
      "print",
      "pass",
      "nostop",
      "noignore",
      "noprint",
      "nopass",
      NULL,
    };

  vec_signals = signal_completer (ignore, text, word);
  vec_keywords = complete_on_enum (keywords, word, word);

  return_val = VEC_merge (char_ptr, vec_signals, vec_keywords);
  VEC_free (char_ptr, vec_signals);
  VEC_free (char_ptr, vec_keywords);
  return return_val;
}

enum gdb_signal
gdb_signal_from_command (int num)
{
  if (num >= 1 && num <= 15)
    return (enum gdb_signal) num;
  error (_("Only signals 1-15 are valid as numeric signals.\n\
Use \"info signals\" for a list of symbolic signals."));
}

/* Print current contents of the tables set by the handle command.
   It is possible we should just be printing signals actually used
   by the current target (but for things to work right when switching
   targets, all signals should be in the signal tables).  */

static void
signals_info (char *signum_exp, int from_tty)
{
  enum gdb_signal oursig;

  sig_print_header ();

  if (signum_exp)
    {
      /* First see if this is a symbol name.  */
      oursig = gdb_signal_from_name (signum_exp);
      if (oursig == GDB_SIGNAL_UNKNOWN)
	{
	  /* No, try numeric.  */
	  oursig =
	    gdb_signal_from_command (parse_and_eval_long (signum_exp));
	}
      sig_print_info (oursig);
      return;
    }

  printf_filtered ("\n");
  /* These ugly casts brought to you by the native VAX compiler.  */
  for (oursig = GDB_SIGNAL_FIRST;
       (int) oursig < (int) GDB_SIGNAL_LAST;
       oursig = (enum gdb_signal) ((int) oursig + 1))
    {
      QUIT;

      if (oursig != GDB_SIGNAL_UNKNOWN
	  && oursig != GDB_SIGNAL_DEFAULT && oursig != GDB_SIGNAL_0)
	sig_print_info (oursig);
    }

  printf_filtered (_("\nUse the \"handle\" command "
		     "to change these tables.\n"));
}

/* The $_siginfo convenience variable is a bit special.  We don't know
   for sure the type of the value until we actually have a chance to
   fetch the data.  The type can change depending on gdbarch, so it is
   also dependent on which thread you have selected.

     1. making $_siginfo be an internalvar that creates a new value on
     access.

     2. making the value of $_siginfo be an lval_computed value.  */

/* This function implements the lval_computed support for reading a
   $_siginfo value.  */

static void
siginfo_value_read (struct value *v)
{
  LONGEST transferred;

  /* If we can access registers, so can we access $_siginfo.  Likewise
     vice versa.  */
  validate_registers_access ();

  transferred =
    target_read (&current_target, TARGET_OBJECT_SIGNAL_INFO,
		 NULL,
		 value_contents_all_raw (v),
		 value_offset (v),
		 TYPE_LENGTH (value_type (v)));

  if (transferred != TYPE_LENGTH (value_type (v)))
    error (_("Unable to read siginfo"));
}

/* This function implements the lval_computed support for writing a
   $_siginfo value.  */

static void
siginfo_value_write (struct value *v, struct value *fromval)
{
  LONGEST transferred;

  /* If we can access registers, so can we access $_siginfo.  Likewise
     vice versa.  */
  validate_registers_access ();

  transferred = target_write (&current_target,
			      TARGET_OBJECT_SIGNAL_INFO,
			      NULL,
			      value_contents_all_raw (fromval),
			      value_offset (v),
			      TYPE_LENGTH (value_type (fromval)));

  if (transferred != TYPE_LENGTH (value_type (fromval)))
    error (_("Unable to write siginfo"));
}

static const struct lval_funcs siginfo_value_funcs =
  {
    siginfo_value_read,
    siginfo_value_write
  };

/* Return a new value with the correct type for the siginfo object of
   the current thread using architecture GDBARCH.  Return a void value
   if there's no object available.  */

static struct value *
siginfo_make_value (struct gdbarch *gdbarch, struct internalvar *var,
		    void *ignore)
{
  if (target_has_stack
      && !ptid_equal (inferior_ptid, null_ptid)
      && gdbarch_get_siginfo_type_p (gdbarch))
    {
      struct type *type = gdbarch_get_siginfo_type (gdbarch);

      return allocate_computed_value (type, &siginfo_value_funcs, NULL);
    }

  return allocate_value (builtin_type (gdbarch)->builtin_void);
}


/* infcall_suspend_state contains state about the program itself like its
   registers and any signal it received when it last stopped.
   This state must be restored regardless of how the inferior function call
   ends (either successfully, or after it hits a breakpoint or signal)
   if the program is to properly continue where it left off.  */

struct infcall_suspend_state
{
  struct thread_suspend_state thread_suspend;

  /* Other fields:  */
  CORE_ADDR stop_pc;
  struct regcache *registers;

  /* Format of SIGINFO_DATA or NULL if it is not present.  */
  struct gdbarch *siginfo_gdbarch;

  /* The inferior format depends on SIGINFO_GDBARCH and it has a length of
     TYPE_LENGTH (gdbarch_get_siginfo_type ()).  For different gdbarch the
     content would be invalid.  */
  gdb_byte *siginfo_data;
};

struct infcall_suspend_state *
save_infcall_suspend_state (void)
{
  struct infcall_suspend_state *inf_state;
  struct thread_info *tp = inferior_thread ();
  struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  gdb_byte *siginfo_data = NULL;

  if (gdbarch_get_siginfo_type_p (gdbarch))
    {
      struct type *type = gdbarch_get_siginfo_type (gdbarch);
      size_t len = TYPE_LENGTH (type);
      struct cleanup *back_to;

      siginfo_data = (gdb_byte *) xmalloc (len);
      back_to = make_cleanup (xfree, siginfo_data);

      if (target_read (&current_target, TARGET_OBJECT_SIGNAL_INFO, NULL,
		       siginfo_data, 0, len) == len)
	discard_cleanups (back_to);
      else
	{
	  /* Errors ignored.  */
	  do_cleanups (back_to);
	  siginfo_data = NULL;
	}
    }

  inf_state = XCNEW (struct infcall_suspend_state);

  if (siginfo_data)
    {
      inf_state->siginfo_gdbarch = gdbarch;
      inf_state->siginfo_data = siginfo_data;
    }

  inf_state->thread_suspend = tp->suspend;

  /* run_inferior_call will not use the signal due to its `proceed' call with
     GDB_SIGNAL_0 anyway.  */
  tp->suspend.stop_signal = GDB_SIGNAL_0;

  inf_state->stop_pc = stop_pc;

  inf_state->registers = regcache_dup (regcache);

  return inf_state;
}

/* Restore inferior session state to INF_STATE.  */

void
restore_infcall_suspend_state (struct infcall_suspend_state *inf_state)
{
  struct thread_info *tp = inferior_thread ();
  struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  tp->suspend = inf_state->thread_suspend;

  stop_pc = inf_state->stop_pc;

  if (inf_state->siginfo_gdbarch == gdbarch)
    {
      struct type *type = gdbarch_get_siginfo_type (gdbarch);

      /* Errors ignored.  */
      target_write (&current_target, TARGET_OBJECT_SIGNAL_INFO, NULL,
		    inf_state->siginfo_data, 0, TYPE_LENGTH (type));
    }

  /* The inferior can be gone if the user types "print exit(0)"
     (and perhaps other times).  */
  if (target_has_execution)
    /* NB: The register write goes through to the target.  */
    regcache_cpy (regcache, inf_state->registers);

  discard_infcall_suspend_state (inf_state);
}

static void
do_restore_infcall_suspend_state_cleanup (void *state)
{
  restore_infcall_suspend_state ((struct infcall_suspend_state *) state);
}

struct cleanup *
make_cleanup_restore_infcall_suspend_state
  (struct infcall_suspend_state *inf_state)
{
  return make_cleanup (do_restore_infcall_suspend_state_cleanup, inf_state);
}

void
discard_infcall_suspend_state (struct infcall_suspend_state *inf_state)
{
  regcache_xfree (inf_state->registers);
  xfree (inf_state->siginfo_data);
  xfree (inf_state);
}

struct regcache *
get_infcall_suspend_state_regcache (struct infcall_suspend_state *inf_state)
{
  return inf_state->registers;
}

/* infcall_control_state contains state regarding gdb's control of the
   inferior itself like stepping control.  It also contains session state like
   the user's currently selected frame.  */

struct infcall_control_state
{
  struct thread_control_state thread_control;
  struct inferior_control_state inferior_control;

  /* Other fields:  */
  enum stop_stack_kind stop_stack_dummy;
  int stopped_by_random_signal;

  /* ID if the selected frame when the inferior function call was made.  */
  struct frame_id selected_frame_id;
};

/* Save all of the information associated with the inferior<==>gdb
   connection.  */

struct infcall_control_state *
save_infcall_control_state (void)
{
  struct infcall_control_state *inf_status =
    XNEW (struct infcall_control_state);
  struct thread_info *tp = inferior_thread ();
  struct inferior *inf = current_inferior ();

  inf_status->thread_control = tp->control;
  inf_status->inferior_control = inf->control;

  tp->control.step_resume_breakpoint = NULL;
  tp->control.exception_resume_breakpoint = NULL;

  /* Save original bpstat chain to INF_STATUS; replace it in TP with copy of
     chain.  If caller's caller is walking the chain, they'll be happier if we
     hand them back the original chain when restore_infcall_control_state is
     called.  */
  tp->control.stop_bpstat = bpstat_copy (tp->control.stop_bpstat);

  /* Other fields:  */
  inf_status->stop_stack_dummy = stop_stack_dummy;
  inf_status->stopped_by_random_signal = stopped_by_random_signal;

  inf_status->selected_frame_id = get_frame_id (get_selected_frame (NULL));

  return inf_status;
}

static int
restore_selected_frame (void *args)
{
  struct frame_id *fid = (struct frame_id *) args;
  struct frame_info *frame;

  frame = frame_find_by_id (*fid);

  /* If inf_status->selected_frame_id is NULL, there was no previously
     selected frame.  */
  if (frame == NULL)
    {
      warning (_("Unable to restore previously selected frame."));
      return 0;
    }

  select_frame (frame);

  return (1);
}

/* Restore inferior session state to INF_STATUS.  */

void
restore_infcall_control_state (struct infcall_control_state *inf_status)
{
  struct thread_info *tp = inferior_thread ();
  struct inferior *inf = current_inferior ();

  if (tp->control.step_resume_breakpoint)
    tp->control.step_resume_breakpoint->disposition = disp_del_at_next_stop;

  if (tp->control.exception_resume_breakpoint)
    tp->control.exception_resume_breakpoint->disposition
      = disp_del_at_next_stop;

  /* Handle the bpstat_copy of the chain.  */
  bpstat_clear (&tp->control.stop_bpstat);

  tp->control = inf_status->thread_control;
  inf->control = inf_status->inferior_control;

  /* Other fields:  */
  stop_stack_dummy = inf_status->stop_stack_dummy;
  stopped_by_random_signal = inf_status->stopped_by_random_signal;

  if (target_has_stack)
    {
      /* The point of catch_errors is that if the stack is clobbered,
         walking the stack might encounter a garbage pointer and
         error() trying to dereference it.  */
      if (catch_errors
	  (restore_selected_frame, &inf_status->selected_frame_id,
	   "Unable to restore previously selected frame:\n",
	   RETURN_MASK_ERROR) == 0)
	/* Error in restoring the selected frame.  Select the innermost
	   frame.  */
	select_frame (get_current_frame ());
    }

  xfree (inf_status);
}

static void
do_restore_infcall_control_state_cleanup (void *sts)
{
  restore_infcall_control_state ((struct infcall_control_state *) sts);
}

struct cleanup *
make_cleanup_restore_infcall_control_state
  (struct infcall_control_state *inf_status)
{
  return make_cleanup (do_restore_infcall_control_state_cleanup, inf_status);
}

void
discard_infcall_control_state (struct infcall_control_state *inf_status)
{
  if (inf_status->thread_control.step_resume_breakpoint)
    inf_status->thread_control.step_resume_breakpoint->disposition
      = disp_del_at_next_stop;

  if (inf_status->thread_control.exception_resume_breakpoint)
    inf_status->thread_control.exception_resume_breakpoint->disposition
      = disp_del_at_next_stop;

  /* See save_infcall_control_state for info on stop_bpstat.  */
  bpstat_clear (&inf_status->thread_control.stop_bpstat);

  xfree (inf_status);
}

/* restore_inferior_ptid() will be used by the cleanup machinery
   to restore the inferior_ptid value saved in a call to
   save_inferior_ptid().  */

static void
restore_inferior_ptid (void *arg)
{
  ptid_t *saved_ptid_ptr = (ptid_t *) arg;

  inferior_ptid = *saved_ptid_ptr;
  xfree (arg);
}

/* Save the value of inferior_ptid so that it may be restored by a
   later call to do_cleanups().  Returns the struct cleanup pointer
   needed for later doing the cleanup.  */

struct cleanup *
save_inferior_ptid (void)
{
  ptid_t *saved_ptid_ptr = XNEW (ptid_t);

  *saved_ptid_ptr = inferior_ptid;
  return make_cleanup (restore_inferior_ptid, saved_ptid_ptr);
}

/* See infrun.h.  */

void
clear_exit_convenience_vars (void)
{
  clear_internalvar (lookup_internalvar ("_exitsignal"));
  clear_internalvar (lookup_internalvar ("_exitcode"));
}


/* User interface for reverse debugging:
   Set exec-direction / show exec-direction commands
   (returns error unless target implements to_set_exec_direction method).  */

enum exec_direction_kind execution_direction = EXEC_FORWARD;
static const char exec_forward[] = "forward";
static const char exec_reverse[] = "reverse";
static const char *exec_direction = exec_forward;
static const char *const exec_direction_names[] = {
  exec_forward,
  exec_reverse,
  NULL
};

static void
set_exec_direction_func (char *args, int from_tty,
			 struct cmd_list_element *cmd)
{
  if (target_can_execute_reverse)
    {
      if (!strcmp (exec_direction, exec_forward))
	execution_direction = EXEC_FORWARD;
      else if (!strcmp (exec_direction, exec_reverse))
	execution_direction = EXEC_REVERSE;
    }
  else
    {
      exec_direction = exec_forward;
      error (_("Target does not support this operation."));
    }
}

static void
show_exec_direction_func (struct ui_file *out, int from_tty,
			  struct cmd_list_element *cmd, const char *value)
{
  switch (execution_direction) {
  case EXEC_FORWARD:
    fprintf_filtered (out, _("Forward.\n"));
    break;
  case EXEC_REVERSE:
    fprintf_filtered (out, _("Reverse.\n"));
    break;
  default:
    internal_error (__FILE__, __LINE__,
		    _("bogus execution_direction value: %d"),
		    (int) execution_direction);
  }
}

static void
show_schedule_multiple (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Resuming the execution of threads "
			    "of all processes is %s.\n"), value);
}

/* Implementation of `siginfo' variable.  */

static const struct internalvar_funcs siginfo_funcs =
{
  siginfo_make_value,
  NULL,
  NULL
};

/* Callback for infrun's target events source.  This is marked when a
   thread has a pending status to process.  */

static void
infrun_async_inferior_event_handler (gdb_client_data data)
{
  inferior_event_handler (INF_REG_EVENT, NULL);
}

void
_initialize_infrun (void)
{
  int i;
  int numsigs;
  struct cmd_list_element *c;

  /* Register extra event sources in the event loop.  */
  infrun_async_inferior_event_token
    = create_async_event_handler (infrun_async_inferior_event_handler, NULL);

  add_info ("signals", signals_info, _("\
What debugger does when program gets various signals.\n\
Specify a signal as argument to print info on that signal only."));
  add_info_alias ("handle", "signals", 0);

  c = add_com ("handle", class_run, handle_command, _("\
Specify how to handle signals.\n\
Usage: handle SIGNAL [ACTIONS]\n\
Args are signals and actions to apply to those signals.\n\
If no actions are specified, the current settings for the specified signals\n\
will be displayed instead.\n\
\n\
Symbolic signals (e.g. SIGSEGV) are recommended but numeric signals\n\
from 1-15 are allowed for compatibility with old versions of GDB.\n\
Numeric ranges may be specified with the form LOW-HIGH (e.g. 1-5).\n\
The special arg \"all\" is recognized to mean all signals except those\n\
used by the debugger, typically SIGTRAP and SIGINT.\n\
\n\
Recognized actions include \"stop\", \"nostop\", \"print\", \"noprint\",\n\
\"pass\", \"nopass\", \"ignore\", or \"noignore\".\n\
Stop means reenter debugger if this signal happens (implies print).\n\
Print means print a message if this signal happens.\n\
Pass means let program see this signal; otherwise program doesn't know.\n\
Ignore is a synonym for nopass and noignore is a synonym for pass.\n\
Pass and Stop may be combined.\n\
\n\
Multiple signals may be specified.  Signal numbers and signal names\n\
may be interspersed with actions, with the actions being performed for\n\
all signals cumulatively specified."));
  set_cmd_completer (c, handle_completer);

  if (!dbx_commands)
    stop_command = add_cmd ("stop", class_obscure,
			    not_just_help_class_command, _("\
There is no `stop' command, but you can set a hook on `stop'.\n\
This allows you to set a list of commands to be run each time execution\n\
of the program stops."), &cmdlist);

  add_setshow_zuinteger_cmd ("infrun", class_maintenance, &debug_infrun, _("\
Set inferior debugging."), _("\
Show inferior debugging."), _("\
When non-zero, inferior specific debugging is enabled."),
			     NULL,
			     show_debug_infrun,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("displaced", class_maintenance,
			   &debug_displaced, _("\
Set displaced stepping debugging."), _("\
Show displaced stepping debugging."), _("\
When non-zero, displaced stepping specific debugging is enabled."),
			    NULL,
			    show_debug_displaced,
			    &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("non-stop", no_class,
			   &non_stop_1, _("\
Set whether gdb controls the inferior in non-stop mode."), _("\
Show whether gdb controls the inferior in non-stop mode."), _("\
When debugging a multi-threaded program and this setting is\n\
off (the default, also called all-stop mode), when one thread stops\n\
(for a breakpoint, watchpoint, exception, or similar events), GDB stops\n\
all other threads in the program while you interact with the thread of\n\
interest.  When you continue or step a thread, you can allow the other\n\
threads to run, or have them remain stopped, but while you inspect any\n\
thread's state, all threads stop.\n\
\n\
In non-stop mode, when one thread stops, other threads can continue\n\
to run freely.  You'll be able to step each thread independently,\n\
leave it stopped or free to run as needed."),
			   set_non_stop,
			   show_non_stop,
			   &setlist,
			   &showlist);

  numsigs = (int) GDB_SIGNAL_LAST;
  signal_stop = XNEWVEC (unsigned char, numsigs);
  signal_print = XNEWVEC (unsigned char, numsigs);
  signal_program = XNEWVEC (unsigned char, numsigs);
  signal_catch = XNEWVEC (unsigned char, numsigs);
  signal_pass = XNEWVEC (unsigned char, numsigs);
  for (i = 0; i < numsigs; i++)
    {
      signal_stop[i] = 1;
      signal_print[i] = 1;
      signal_program[i] = 1;
      signal_catch[i] = 0;
    }

  /* Signals caused by debugger's own actions should not be given to
     the program afterwards.

     Do not deliver GDB_SIGNAL_TRAP by default, except when the user
     explicitly specifies that it should be delivered to the target
     program.  Typically, that would occur when a user is debugging a
     target monitor on a simulator: the target monitor sets a
     breakpoint; the simulator encounters this breakpoint and halts
     the simulation handing control to GDB; GDB, noting that the stop
     address doesn't map to any known breakpoint, returns control back
     to the simulator; the simulator then delivers the hardware
     equivalent of a GDB_SIGNAL_TRAP to the program being
     debugged.  */
  signal_program[GDB_SIGNAL_TRAP] = 0;
  signal_program[GDB_SIGNAL_INT] = 0;

  /* Signals that are not errors should not normally enter the debugger.  */
  signal_stop[GDB_SIGNAL_ALRM] = 0;
  signal_print[GDB_SIGNAL_ALRM] = 0;
  signal_stop[GDB_SIGNAL_VTALRM] = 0;
  signal_print[GDB_SIGNAL_VTALRM] = 0;
  signal_stop[GDB_SIGNAL_PROF] = 0;
  signal_print[GDB_SIGNAL_PROF] = 0;
  signal_stop[GDB_SIGNAL_CHLD] = 0;
  signal_print[GDB_SIGNAL_CHLD] = 0;
  signal_stop[GDB_SIGNAL_IO] = 0;
  signal_print[GDB_SIGNAL_IO] = 0;
  signal_stop[GDB_SIGNAL_POLL] = 0;
  signal_print[GDB_SIGNAL_POLL] = 0;
  signal_stop[GDB_SIGNAL_URG] = 0;
  signal_print[GDB_SIGNAL_URG] = 0;
  signal_stop[GDB_SIGNAL_WINCH] = 0;
  signal_print[GDB_SIGNAL_WINCH] = 0;
  signal_stop[GDB_SIGNAL_PRIO] = 0;
  signal_print[GDB_SIGNAL_PRIO] = 0;

  /* These signals are used internally by user-level thread
     implementations.  (See signal(5) on Solaris.)  Like the above
     signals, a healthy program receives and handles them as part of
     its normal operation.  */
  signal_stop[GDB_SIGNAL_LWP] = 0;
  signal_print[GDB_SIGNAL_LWP] = 0;
  signal_stop[GDB_SIGNAL_WAITING] = 0;
  signal_print[GDB_SIGNAL_WAITING] = 0;
  signal_stop[GDB_SIGNAL_CANCEL] = 0;
  signal_print[GDB_SIGNAL_CANCEL] = 0;
  signal_stop[GDB_SIGNAL_LIBRT] = 0;
  signal_print[GDB_SIGNAL_LIBRT] = 0;

  /* Update cached state.  */
  signal_cache_update (-1);

  add_setshow_zinteger_cmd ("stop-on-solib-events", class_support,
			    &stop_on_solib_events, _("\
Set stopping for shared library events."), _("\
Show stopping for shared library events."), _("\
If nonzero, gdb will give control to the user when the dynamic linker\n\
notifies gdb of shared library events.  The most common event of interest\n\
to the user would be loading/unloading of a new library."),
			    set_stop_on_solib_events,
			    show_stop_on_solib_events,
			    &setlist, &showlist);

  add_setshow_enum_cmd ("follow-fork-mode", class_run,
			follow_fork_mode_kind_names,
			&follow_fork_mode_string, _("\
Set debugger response to a program call of fork or vfork."), _("\
Show debugger response to a program call of fork or vfork."), _("\
A fork or vfork creates a new process.  follow-fork-mode can be:\n\
  parent  - the original process is debugged after a fork\n\
  child   - the new process is debugged after a fork\n\
The unfollowed process will continue to run.\n\
By default, the debugger will follow the parent process."),
			NULL,
			show_follow_fork_mode_string,
			&setlist, &showlist);

  add_setshow_enum_cmd ("follow-exec-mode", class_run,
			follow_exec_mode_names,
			&follow_exec_mode_string, _("\
Set debugger response to a program call of exec."), _("\
Show debugger response to a program call of exec."), _("\
An exec call replaces the program image of a process.\n\
\n\
follow-exec-mode can be:\n\
\n\
  new - the debugger creates a new inferior and rebinds the process\n\
to this new inferior.  The program the process was running before\n\
the exec call can be restarted afterwards by restarting the original\n\
inferior.\n\
\n\
  same - the debugger keeps the process bound to the same inferior.\n\
The new executable image replaces the previous executable loaded in\n\
the inferior.  Restarting the inferior after the exec call restarts\n\
the executable the process was running after the exec call.\n\
\n\
By default, the debugger will use the same inferior."),
			NULL,
			show_follow_exec_mode_string,
			&setlist, &showlist);

  add_setshow_enum_cmd ("scheduler-locking", class_run, 
			scheduler_enums, &scheduler_mode, _("\
Set mode for locking scheduler during execution."), _("\
Show mode for locking scheduler during execution."), _("\
off    == no locking (threads may preempt at any time)\n\
on     == full locking (no thread except the current thread may run)\n\
          This applies to both normal execution and replay mode.\n\
step   == scheduler locked during stepping commands (step, next, stepi, nexti).\n\
          In this mode, other threads may run during other commands.\n\
          This applies to both normal execution and replay mode.\n\
replay == scheduler locked in replay mode and unlocked during normal execution."),
			set_schedlock_func,	/* traps on target vector */
			show_scheduler_mode,
			&setlist, &showlist);

  add_setshow_boolean_cmd ("schedule-multiple", class_run, &sched_multi, _("\
Set mode for resuming threads of all processes."), _("\
Show mode for resuming threads of all processes."), _("\
When on, execution commands (such as 'continue' or 'next') resume all\n\
threads of all processes.  When off (which is the default), execution\n\
commands only resume the threads of the current process.  The set of\n\
threads that are resumed is further refined by the scheduler-locking\n\
mode (see help set scheduler-locking)."),
			   NULL,
			   show_schedule_multiple,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("step-mode", class_run, &step_stop_if_no_debug, _("\
Set mode of the step operation."), _("\
Show mode of the step operation."), _("\
When set, doing a step over a function without debug line information\n\
will stop at the first instruction of that function. Otherwise, the\n\
function is skipped and the step command stops at a different source line."),
			   NULL,
			   show_step_stop_if_no_debug,
			   &setlist, &showlist);

  add_setshow_auto_boolean_cmd ("displaced-stepping", class_run,
				&can_use_displaced_stepping, _("\
Set debugger's willingness to use displaced stepping."), _("\
Show debugger's willingness to use displaced stepping."), _("\
If on, gdb will use displaced stepping to step over breakpoints if it is\n\
supported by the target architecture.  If off, gdb will not use displaced\n\
stepping to step over breakpoints, even if such is supported by the target\n\
architecture.  If auto (which is the default), gdb will use displaced stepping\n\
if the target architecture supports it and non-stop mode is active, but will not\n\
use it in all-stop mode (see help set non-stop)."),
				NULL,
				show_can_use_displaced_stepping,
				&setlist, &showlist);

  add_setshow_enum_cmd ("exec-direction", class_run, exec_direction_names,
			&exec_direction, _("Set direction of execution.\n\
Options are 'forward' or 'reverse'."),
			_("Show direction of execution (forward/reverse)."),
			_("Tells gdb whether to execute forward or backward."),
			set_exec_direction_func, show_exec_direction_func,
			&setlist, &showlist);

  /* Set/show detach-on-fork: user-settable mode.  */

  add_setshow_boolean_cmd ("detach-on-fork", class_run, &detach_fork, _("\
Set whether gdb will detach the child of a fork."), _("\
Show whether gdb will detach the child of a fork."), _("\
Tells gdb whether to detach the child of a fork."),
			   NULL, NULL, &setlist, &showlist);

  /* Set/show disable address space randomization mode.  */

  add_setshow_boolean_cmd ("disable-randomization", class_support,
			   &disable_randomization, _("\
Set disabling of debuggee's virtual address space randomization."), _("\
Show disabling of debuggee's virtual address space randomization."), _("\
When this mode is on (which is the default), randomization of the virtual\n\
address space is disabled.  Standalone programs run with the randomization\n\
enabled by default on some platforms."),
			   &set_disable_randomization,
			   &show_disable_randomization,
			   &setlist, &showlist);

  /* ptid initializations */
  inferior_ptid = null_ptid;
  target_last_wait_ptid = minus_one_ptid;

  observer_attach_thread_ptid_changed (infrun_thread_ptid_changed);
  observer_attach_thread_stop_requested (infrun_thread_stop_requested);
  observer_attach_thread_exit (infrun_thread_thread_exit);
  observer_attach_inferior_exit (infrun_inferior_exit);

  /* Explicitly create without lookup, since that tries to create a
     value with a void typed value, and when we get here, gdbarch
     isn't initialized yet.  At this point, we're quite sure there
     isn't another convenience variable of the same name.  */
  create_internalvar_type_lazy ("_siginfo", &siginfo_funcs, NULL);

  add_setshow_boolean_cmd ("observer", no_class,
			   &observer_mode_1, _("\
Set whether gdb controls the inferior in observer mode."), _("\
Show whether gdb controls the inferior in observer mode."), _("\
In observer mode, GDB can get data from the inferior, but not\n\
affect its execution.  Registers and memory may not be changed,\n\
breakpoints may not be set, and the program cannot be interrupted\n\
or signalled."),
			   set_observer_mode,
			   show_observer_mode,
			   &setlist,
			   &showlist);
}
