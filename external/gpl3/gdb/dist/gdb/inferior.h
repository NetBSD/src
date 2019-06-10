/* Variables that describe the inferior process running under GDB:
   Where it is, why it stopped, and how to step it.

   Copyright (C) 1986-2019 Free Software Foundation, Inc.

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

#if !defined (INFERIOR_H)
#define INFERIOR_H 1

struct target_waitstatus;
struct frame_info;
struct ui_file;
struct type;
struct gdbarch;
struct regcache;
struct ui_out;
struct terminal_info;
struct target_desc_info;
struct continuation;
struct inferior;
struct thread_info;

/* For bpstat.  */
#include "breakpoint.h"

/* For enum gdb_signal.  */
#include "target.h"

/* For struct frame_id.  */
#include "frame.h"

/* For gdb_environ.  */
#include "common/environ.h"

#include "progspace.h"
#include "registry.h"

#include "symfile-add-flags.h"
#include "common/refcounted-object.h"
#include "common/forward-scope-exit.h"

#include "common/common-inferior.h"
#include "gdbthread.h"

struct infcall_suspend_state;
struct infcall_control_state;

extern void restore_infcall_suspend_state (struct infcall_suspend_state *);
extern void restore_infcall_control_state (struct infcall_control_state *);

/* A deleter for infcall_suspend_state that calls
   restore_infcall_suspend_state.  */
struct infcall_suspend_state_deleter
{
  void operator() (struct infcall_suspend_state *state) const
  {
    TRY
      {
	restore_infcall_suspend_state (state);
      }
    CATCH (e, RETURN_MASK_ALL)
      {
	/* If we are restoring the inferior state due to an exception,
	   some error message will be printed.  So, only warn the user
	   when we cannot restore during normal execution.  */
	if (!std::uncaught_exception ())
	  warning (_("Failed to restore inferior state: %s"), e.message);
      }
    END_CATCH
  }
};

/* A unique_ptr specialization for infcall_suspend_state.  */
typedef std::unique_ptr<infcall_suspend_state, infcall_suspend_state_deleter>
    infcall_suspend_state_up;

extern infcall_suspend_state_up save_infcall_suspend_state ();

/* A deleter for infcall_control_state that calls
   restore_infcall_control_state.  */
struct infcall_control_state_deleter
{
  void operator() (struct infcall_control_state *state) const
  {
    restore_infcall_control_state (state);
  }
};

/* A unique_ptr specialization for infcall_control_state.  */
typedef std::unique_ptr<infcall_control_state, infcall_control_state_deleter>
    infcall_control_state_up;

extern infcall_control_state_up save_infcall_control_state ();

extern void discard_infcall_suspend_state (struct infcall_suspend_state *);
extern void discard_infcall_control_state (struct infcall_control_state *);

extern readonly_detached_regcache *
  get_infcall_suspend_state_regcache (struct infcall_suspend_state *);

extern void set_sigint_trap (void);

extern void clear_sigint_trap (void);

/* Set/get file name for default use for standard in/out in the inferior.  */

extern void set_inferior_io_terminal (const char *terminal_name);
extern const char *get_inferior_io_terminal (void);

/* Collected pid, tid, etc. of the debugged inferior.  When there's
   no inferior, inferior_ptid.pid () will be 0.  */

extern ptid_t inferior_ptid;

extern void generic_mourn_inferior (void);

extern CORE_ADDR unsigned_pointer_to_address (struct gdbarch *gdbarch,
					      struct type *type,
					      const gdb_byte *buf);
extern void unsigned_address_to_pointer (struct gdbarch *gdbarch,
					 struct type *type, gdb_byte *buf,
					 CORE_ADDR addr);
extern CORE_ADDR signed_pointer_to_address (struct gdbarch *gdbarch,
					    struct type *type,
					    const gdb_byte *buf);
extern void address_to_signed_pointer (struct gdbarch *gdbarch,
				       struct type *type, gdb_byte *buf,
				       CORE_ADDR addr);

extern void reopen_exec_file (void);

/* From misc files */

extern void default_print_registers_info (struct gdbarch *gdbarch,
					  struct ui_file *file,
					  struct frame_info *frame,
					  int regnum, int all);

/* Default implementation of gdbarch_print_float_info.  Print
   the values of all floating point registers.  */

extern void default_print_float_info (struct gdbarch *gdbarch,
				      struct ui_file *file,
				      struct frame_info *frame,
				      const char *args);

extern void child_terminal_info (struct target_ops *self, const char *, int);

extern void info_terminal_command (char *, int);

extern void child_terminal_ours (struct target_ops *self);

extern void child_terminal_ours_for_output (struct target_ops *self);

extern void child_terminal_inferior (struct target_ops *self);

extern void child_terminal_save_inferior (struct target_ops *self);

extern void child_terminal_init (struct target_ops *self);

extern void child_terminal_init_with_pgrp (int pgrp);

extern void child_pass_ctrlc (struct target_ops *self);

extern void child_interrupt (struct target_ops *self);

/* From fork-child.c */

/* Helper function to call STARTUP_INFERIOR with PID and NUM_TRAPS.
   This function already calls set_executing.  Return the ptid_t from
   STARTUP_INFERIOR.  */
extern ptid_t gdb_startup_inferior (pid_t pid, int num_traps);

extern char *construct_inferior_arguments (int, char **);

/* From infcmd.c */

/* Initial inferior setup.  Determines the exec file is not yet known,
   takes any necessary post-attaching actions, fetches the target
   description and syncs the shared library list.  */

extern void setup_inferior (int from_tty);

extern void post_create_inferior (struct target_ops *, int);

extern void attach_command (const char *, int);

extern const char *get_inferior_args (void);

extern void set_inferior_args (const char *);

extern void set_inferior_args_vector (int, char **);

extern void registers_info (const char *, int);

extern void continue_1 (int all_threads);

extern void interrupt_target_1 (int all_threads);

using delete_longjmp_breakpoint_cleanup
  = FORWARD_SCOPE_EXIT (delete_longjmp_breakpoint);

extern void detach_command (const char *, int);

extern void notice_new_inferior (struct thread_info *, int, int);

extern struct value *get_return_value (struct value *function,
				       struct type *value_type);

/* Prepare for execution command.  TARGET is the target that will run
   the command.  BACKGROUND determines whether this is a foreground
   (synchronous) or background (asynchronous) command.  */

extern void prepare_execution_command (struct target_ops *target,
				       int background);

/* Whether to start up the debuggee under a shell.

   If startup-with-shell is set, GDB's "run" will attempt to start up
   the debuggee under a shell.

   This is in order for argument-expansion to occur.  E.g.,

   (gdb) run *

   The "*" gets expanded by the shell into a list of files.

   While this is a nice feature, it may be handy to bypass the shell
   in some cases.  To disable this feature, do "set startup-with-shell
   false".

   The catch-exec traps expected during start-up will be one more if
   the target is started up with a shell.  */
extern int startup_with_shell;

/* Nonzero if stopped due to completion of a stack dummy routine.  */

extern enum stop_stack_kind stop_stack_dummy;

/* Nonzero if program stopped due to a random (unexpected) signal in
   inferior process.  */

extern int stopped_by_random_signal;

/* Print notices on inferior events (attach, detach, etc.), set with
   `set print inferior-events'.  */
extern int print_inferior_events;

/* Anything but NO_STOP_QUIETLY means we expect a trap and the caller
   will handle it themselves.  STOP_QUIETLY is used when running in
   the shell before the child program has been exec'd and when running
   through shared library loading.  STOP_QUIETLY_REMOTE is used when
   setting up a remote connection; it is like STOP_QUIETLY_NO_SIGSTOP
   except that there is no need to hide a signal.  */

/* STOP_QUIETLY_NO_SIGSTOP is used to handle a tricky situation with attach.
   When doing an attach, the kernel stops the debuggee with a SIGSTOP.
   On newer GNU/Linux kernels (>= 2.5.61) the handling of SIGSTOP for
   a ptraced process has changed.  Earlier versions of the kernel
   would ignore these SIGSTOPs, while now SIGSTOP is treated like any
   other signal, i.e. it is not muffled.

   If the gdb user does a 'continue' after the 'attach', gdb passes
   the global variable stop_signal (which stores the signal from the
   attach, SIGSTOP) to the ptrace(PTRACE_CONT,...)  call.  This is
   problematic, because the kernel doesn't ignore such SIGSTOP
   now.  I.e. it is reported back to gdb, which in turn presents it
   back to the user.

   To avoid the problem, we use STOP_QUIETLY_NO_SIGSTOP, which allows
   gdb to clear the value of stop_signal after the attach, so that it
   is not passed back down to the kernel.  */

enum stop_kind
  {
    NO_STOP_QUIETLY = 0,
    STOP_QUIETLY,
    STOP_QUIETLY_REMOTE,
    STOP_QUIETLY_NO_SIGSTOP
  };


/* Possible values for gdbarch_call_dummy_location.  */
#define ON_STACK 1
#define AT_ENTRY_POINT 4

/* Base class for target-specific inferior data.  */

struct private_inferior
{
  virtual ~private_inferior () = 0;
};

/* Inferior process specific part of `struct infcall_control_state'.

   Inferior thread counterpart is `struct thread_control_state'.  */

struct inferior_control_state
{
  inferior_control_state ()
    : stop_soon (NO_STOP_QUIETLY)
  {
  }

  explicit inferior_control_state (enum stop_kind when)
    : stop_soon (when)
  {
  }

  /* See the definition of stop_kind above.  */
  enum stop_kind stop_soon;
};

/* Return a pointer to the current inferior.  */
extern inferior *current_inferior ();

extern void set_current_inferior (inferior *);

/* GDB represents the state of each program execution with an object
   called an inferior.  An inferior typically corresponds to a process
   but is more general and applies also to targets that do not have a
   notion of processes.  Each run of an executable creates a new
   inferior, as does each attachment to an existing process.
   Inferiors have unique internal identifiers that are different from
   target process ids.  Each inferior may in turn have multiple
   threads running in it.

   Inferiors are intrusively refcounted objects.  Unlike thread
   objects, being the user-selected inferior is considered a strong
   reference and is thus accounted for in the inferior object's
   refcount (see set_current_inferior).  When GDB needs to remember
   the selected inferior to later restore it, GDB temporarily bumps
   the inferior object's refcount, to prevent something deleting the
   inferior object before reverting back (e.g., due to a
   "remove-inferiors" command (see
   make_cleanup_restore_current_thread).  All other inferior
   references are considered weak references.  Inferiors are always
   listed exactly once in the inferior list, so placing an inferior in
   the inferior list is an implicit, not counted strong reference.  */

class inferior : public refcounted_object
{
public:
  explicit inferior (int pid);
  ~inferior ();

  /* Returns true if we can delete this inferior.  */
  bool deletable () const { return refcount () == 0; }

  /* Pointer to next inferior in singly-linked list of inferiors.  */
  struct inferior *next = NULL;

  /* This inferior's thread list.  */
  thread_info *thread_list = nullptr;

  /* Returns a range adapter covering the inferior's threads,
     including exited threads.  Used like this:

       for (thread_info *thr : inf->threads ())
	 { .... }
  */
  inf_threads_range threads ()
  { return inf_threads_range (this->thread_list); }

  /* Returns a range adapter covering the inferior's non-exited
     threads.  Used like this:

       for (thread_info *thr : inf->non_exited_threads ())
	 { .... }
  */
  inf_non_exited_threads_range non_exited_threads ()
  { return inf_non_exited_threads_range (this->thread_list); }

  /* Like inferior::threads(), but returns a range adapter that can be
     used with range-for, safely.  I.e., it is safe to delete the
     currently-iterated thread, like this:

     for (thread_info *t : inf->threads_safe ())
       if (some_condition ())
	 delete f;
  */
  inline safe_inf_threads_range threads_safe ()
  { return safe_inf_threads_range (this->thread_list); }

  /* Convenient handle (GDB inferior id).  Unique across all
     inferiors.  */
  int num = 0;

  /* Actual target inferior id, usually, a process id.  This matches
     the ptid_t.pid member of threads of this inferior.  */
  int pid = 0;
  /* True if the PID was actually faked by GDB.  */
  bool fake_pid_p = false;

  /* The highest thread number this inferior ever had.  */
  int highest_thread_num = 0;

  /* State of GDB control of inferior process execution.
     See `struct inferior_control_state'.  */
  inferior_control_state control;

  /* True if this was an auto-created inferior, e.g. created from
     following a fork; false, if this inferior was manually added by
     the user, and we should not attempt to prune it
     automatically.  */
  bool removable = false;

  /* The address space bound to this inferior.  */
  struct address_space *aspace = NULL;

  /* The program space bound to this inferior.  */
  struct program_space *pspace = NULL;

  /* The arguments string to use when running.  */
  char *args = NULL;

  /* The size of elements in argv.  */
  int argc = 0;

  /* The vector version of arguments.  If ARGC is nonzero,
     then we must compute ARGS from this (via the target).
     This is always coming from main's argv and therefore
     should never be freed.  */
  char **argv = NULL;

  /* The current working directory that will be used when starting
     this inferior.  */
  gdb::unique_xmalloc_ptr<char> cwd;

  /* The name of terminal device to use for I/O.  */
  char *terminal = NULL;

  /* The terminal state as set by the last target_terminal::terminal_*
     call.  */
  target_terminal_state terminal_state = target_terminal_state::is_ours;

  /* Environment to use for running inferior,
     in format described in environ.h.  */
  gdb_environ environment;

  /* True if this child process was attached rather than forked.  */
  bool attach_flag = false;

  /* If this inferior is a vfork child, then this is the pointer to
     its vfork parent, if GDB is still attached to it.  */
  inferior *vfork_parent = NULL;

  /* If this process is a vfork parent, this is the pointer to the
     child.  Since a vfork parent is left frozen by the kernel until
     the child execs or exits, a process can only have one vfork child
     at a given time.  */
  inferior *vfork_child = NULL;

  /* True if this inferior should be detached when it's vfork sibling
     exits or execs.  */
  bool pending_detach = false;

  /* True if this inferior is a vfork parent waiting for a vfork child
     not under our control to be done with the shared memory region,
     either by exiting or execing.  */
  bool waiting_for_vfork_done = false;

  /* True if we're in the process of detaching from this inferior.  */
  bool detaching = false;

  /* What is left to do for an execution command after any thread of
     this inferior stops.  For continuations associated with a
     specific thread, see `struct thread_info'.  */
  continuation *continuations = NULL;

  /* True if setup_inferior wasn't called for this inferior yet.
     Until that is done, we must not access inferior memory or
     registers, as we haven't determined the target
     architecture/description.  */
  bool needs_setup = false;

  /* Private data used by the target vector implementation.  */
  std::unique_ptr<private_inferior> priv;

  /* HAS_EXIT_CODE is true if the inferior exited with an exit code.
     In this case, the EXIT_CODE field is also valid.  */
  bool has_exit_code = false;
  LONGEST exit_code = 0;

  /* Default flags to pass to the symbol reading functions.  These are
     used whenever a new objfile is created.  */
  symfile_add_flags symfile_flags = 0;

  /* Info about an inferior's target description (if it's fetched; the
     user supplied description's filename, if any; etc.).  */
  target_desc_info *tdesc_info = NULL;

  /* The architecture associated with the inferior through the
     connection to the target.

     The architecture vector provides some information that is really
     a property of the inferior, accessed through a particular target:
     ptrace operations; the layout of certain RSP packets; the
     solib_ops vector; etc.  To differentiate architecture accesses to
     per-inferior/target properties from
     per-thread/per-frame/per-objfile properties, accesses to
     per-inferior/target properties should be made through
     this gdbarch.  */
  struct gdbarch *gdbarch = NULL;

  /* Data related to displaced stepping.  */
  displaced_step_inferior_state displaced_step_state;

  /* Per inferior data-pointers required by other GDB modules.  */
  REGISTRY_FIELDS;
};

/* Keep a registry of per-inferior data-pointers required by other GDB
   modules.  */

DECLARE_REGISTRY (inferior);

/* Add an inferior to the inferior list, print a message that a new
   inferior is found, and return the pointer to the new inferior.
   Caller may use this pointer to initialize the private inferior
   data.  */
extern struct inferior *add_inferior (int pid);

/* Same as add_inferior, but don't print new inferior notifications to
   the CLI.  */
extern struct inferior *add_inferior_silent (int pid);

extern void delete_inferior (struct inferior *todel);

/* Delete an existing inferior list entry, due to inferior detaching.  */
extern void detach_inferior (inferior *inf);

extern void exit_inferior (inferior *inf);

extern void exit_inferior_silent (inferior *inf);

extern void exit_inferior_num_silent (int num);

extern void inferior_appeared (struct inferior *inf, int pid);

/* Get rid of all inferiors.  */
extern void discard_all_inferiors (void);

/* Search function to lookup an inferior by target 'pid'.  */
extern struct inferior *find_inferior_pid (int pid);

/* Search function to lookup an inferior whose pid is equal to 'ptid.pid'. */
extern struct inferior *find_inferior_ptid (ptid_t ptid);

/* Search function to lookup an inferior by GDB 'num'.  */
extern struct inferior *find_inferior_id (int num);

/* Find an inferior bound to PSPACE, giving preference to the current
   inferior.  */
extern struct inferior *
  find_inferior_for_program_space (struct program_space *pspace);

/* Inferior iterator function.

   Calls a callback function once for each inferior, so long as the
   callback function returns false.  If the callback function returns
   true, the iteration will end and the current inferior will be
   returned.  This can be useful for implementing a search for a
   inferior with arbitrary attributes, or for applying some operation
   to every inferior.

   It is safe to delete the iterated inferior from the callback.  */
extern struct inferior *iterate_over_inferiors (int (*) (struct inferior *,
							 void *),
						void *);

/* Returns true if the inferior list is not empty.  */
extern int have_inferiors (void);

/* Returns the number of live inferiors (real live processes).  */
extern int number_of_live_inferiors (void);

/* Returns true if there are any live inferiors in the inferior list
   (not cores, not executables, real live processes).  */
extern int have_live_inferiors (void);

/* Save/restore the current inferior.  */

class scoped_restore_current_inferior
{
public:
  scoped_restore_current_inferior ()
    : m_saved_inf (current_inferior ())
  {}

  ~scoped_restore_current_inferior ()
  { set_current_inferior (m_saved_inf); }

  DISABLE_COPY_AND_ASSIGN (scoped_restore_current_inferior);

private:
  inferior *m_saved_inf;
};


/* Traverse all inferiors.  */

extern struct inferior *inferior_list;

/* Pull in the internals of the inferiors ranges and iterators.  Must
   be done after struct inferior is defined.  */
#include "inferior-iter.h"

/* Return a range that can be used to walk over all inferiors
   inferiors, with range-for, safely.  I.e., it is safe to delete the
   currently-iterated inferior.  When combined with range-for, this
   allow convenient patterns like this:

     for (inferior *inf : all_inferiors_safe ())
       if (some_condition ())
	 delete inf;
*/

inline all_inferiors_safe_range
all_inferiors_safe ()
{
  return {};
}

/* Returns a range representing all inferiors, suitable to use with
   range-for, like this:

   for (inferior *inf : all_inferiors ())
     [...]
*/

inline all_inferiors_range
all_inferiors ()
{
  return {};
}

/* Return a range that can be used to walk over all inferiors with PID
   not zero, with range-for.  */

inline all_non_exited_inferiors_range
all_non_exited_inferiors ()
{
  return {};
}

/* Prune away automatically added inferiors that aren't required
   anymore.  */
extern void prune_inferiors (void);

extern int number_of_inferiors (void);

extern struct inferior *add_inferior_with_spaces (void);

/* Print the current selected inferior.  */
extern void print_selected_inferior (struct ui_out *uiout);

#endif /* !defined (INFERIOR_H) */
