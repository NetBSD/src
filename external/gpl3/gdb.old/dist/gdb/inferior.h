/* Variables that describe the inferior process running under GDB:
   Where it is, why it stopped, and how to step it.

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
struct gdb_environ;
struct continuation;

/* For bpstat.  */
#include "breakpoint.h"

/* For enum gdb_signal.  */
#include "target.h"

/* For struct frame_id.  */
#include "frame.h"

#include "progspace.h"
#include "registry.h"

#include "symfile-add-flags.h"

struct infcall_suspend_state;
struct infcall_control_state;

extern struct infcall_suspend_state *save_infcall_suspend_state (void);
extern struct infcall_control_state *save_infcall_control_state (void);

extern void restore_infcall_suspend_state (struct infcall_suspend_state *);
extern void restore_infcall_control_state (struct infcall_control_state *);

extern struct cleanup *make_cleanup_restore_infcall_suspend_state
					    (struct infcall_suspend_state *);
extern struct cleanup *make_cleanup_restore_infcall_control_state
					    (struct infcall_control_state *);

extern void discard_infcall_suspend_state (struct infcall_suspend_state *);
extern void discard_infcall_control_state (struct infcall_control_state *);

extern struct regcache *
  get_infcall_suspend_state_regcache (struct infcall_suspend_state *);

/* Save value of inferior_ptid so that it may be restored by
   a later call to do_cleanups().  Returns the struct cleanup
   pointer needed for later doing the cleanup.  */
extern struct cleanup * save_inferior_ptid (void);

extern void set_sigint_trap (void);

extern void clear_sigint_trap (void);

/* Set/get file name for default use for standard in/out in the inferior.  */

extern void set_inferior_io_terminal (const char *terminal_name);
extern const char *get_inferior_io_terminal (void);

/* Collected pid, tid, etc. of the debugged inferior.  When there's
   no inferior, ptid_get_pid (inferior_ptid) will be 0.  */

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

extern void term_info (char *, int);

extern void child_terminal_ours (struct target_ops *self);

extern void child_terminal_ours_for_output (struct target_ops *self);

extern void child_terminal_inferior (struct target_ops *self);

extern void child_terminal_init (struct target_ops *self);

extern void child_terminal_init_with_pgrp (int pgrp);

/* From fork-child.c */

/* Report an error that happened when starting to trace the inferior
   (i.e., when the "traceme_fun" callback is called on fork_inferior)
   and bail out.  This function does not return.  */

extern void trace_start_error (const char *fmt, ...)
  ATTRIBUTE_NORETURN;

/* Like "trace_start_error", but the error message is constructed by
   combining STRING with the system error message for errno.  This
   function does not return.  */

extern void trace_start_error_with_name (const char *string)
  ATTRIBUTE_NORETURN;

extern int fork_inferior (const char *, const std::string &, char **,
			  void (*)(void),
			  void (*)(int), void (*)(void), char *,
                          void (*)(const char *,
                                   char * const *, char * const *));


extern void startup_inferior (int);

extern char *construct_inferior_arguments (int, char **);

/* From infcmd.c */

/* Initial inferior setup.  Determines the exec file is not yet known,
   takes any necessary post-attaching actions, fetches the target
   description and syncs the shared library list.  */

extern void setup_inferior (int from_tty);

extern void post_create_inferior (struct target_ops *, int);

extern void attach_command (char *, int);

extern char *get_inferior_args (void);

extern void set_inferior_args (char *);

extern void set_inferior_args_vector (int, char **);

extern void registers_info (char *, int);

extern void continue_1 (int all_threads);

extern void interrupt_target_1 (int all_threads);

extern void delete_longjmp_breakpoint_cleanup (void *arg);

extern void detach_command (char *, int);

extern void notice_new_inferior (ptid_t, int, int);

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

/* Address at which inferior stopped.  */

extern CORE_ADDR stop_pc;

/* Nonzero if stopped due to completion of a stack dummy routine.  */

extern enum stop_stack_kind stop_stack_dummy;

/* Nonzero if program stopped due to a random (unexpected) signal in
   inferior process.  */

extern int stopped_by_random_signal;

/* STEP_OVER_ALL means step over all subroutine calls.
   STEP_OVER_UNDEBUGGABLE means step over calls to undebuggable functions.
   STEP_OVER_NONE means don't step over any subroutine calls.  */

enum step_over_calls_kind
  {
    STEP_OVER_NONE,
    STEP_OVER_ALL,
    STEP_OVER_UNDEBUGGABLE
  };

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

/* Number of traps that happen between exec'ing the shell to run an
   inferior and when we finally get to the inferior code, not counting
   the exec for the shell.  This is 1 on all supported
   implementations.  */
#define START_INFERIOR_TRAPS_EXPECTED	1

struct private_inferior;

/* Inferior process specific part of `struct infcall_control_state'.

   Inferior thread counterpart is `struct thread_control_state'.  */

struct inferior_control_state
{
  /* See the definition of stop_kind above.  */
  enum stop_kind stop_soon;
};

/* GDB represents the state of each program execution with an object
   called an inferior.  An inferior typically corresponds to a process
   but is more general and applies also to targets that do not have a
   notion of processes.  Each run of an executable creates a new
   inferior, as does each attachment to an existing process.
   Inferiors have unique internal identifiers that are different from
   target process ids.  Each inferior may in turn have multiple
   threads running in it.  */

class inferior
{
public:
  explicit inferior (int pid);
  ~inferior ();

  /* Pointer to next inferior in singly-linked list of inferiors.  */
  struct inferior *next = NULL;

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
  inferior_control_state control {NO_STOP_QUIETLY};

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

  /* The name of terminal device to use for I/O.  */
  char *terminal = NULL;

  /* Environment to use for running inferior,
     in format described in environ.h.  */
  gdb_environ *environment = NULL;

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
  int detaching = 0;

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
  private_inferior *priv = NULL;

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
extern void detach_inferior (int pid);

extern void exit_inferior (int pid);

extern void exit_inferior_silent (int pid);

extern void exit_inferior_num_silent (int num);

extern void inferior_appeared (struct inferior *inf, int pid);

/* Get rid of all inferiors.  */
extern void discard_all_inferiors (void);

/* Translate the integer inferior id (GDB's homegrown id, not the system's)
   into a "pid" (which may be overloaded with extra inferior information).  */
extern int gdb_inferior_id_to_pid (int);

/* Translate a target 'pid' into the integer inferior id (GDB's
   homegrown id, not the system's).  */
extern int pid_to_gdb_inferior_id (int pid);

/* Boolean test for an already-known pid.  */
extern int in_inferior_list (int pid);

/* Boolean test for an already-known inferior id (GDB's homegrown id,
   not the system's).  */
extern int valid_gdb_inferior_id (int num);

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

/* Return a pointer to the current inferior.  It is an error to call
   this if there is no current inferior.  */
extern struct inferior *current_inferior (void);

extern void set_current_inferior (struct inferior *);

extern struct cleanup *save_current_inferior (void);

/* Traverse all inferiors.  */

#define ALL_INFERIORS(I) \
  for ((I) = inferior_list; (I); (I) = (I)->next)

/* Traverse all non-exited inferiors.  */

#define ALL_NON_EXITED_INFERIORS(I) \
  ALL_INFERIORS (I)		    \
    if ((I)->pid != 0)

extern struct inferior *inferior_list;

/* Prune away automatically added inferiors that aren't required
   anymore.  */
extern void prune_inferiors (void);

extern int number_of_inferiors (void);

extern struct inferior *add_inferior_with_spaces (void);

/* Print the current selected inferior.  */
extern void print_selected_inferior (struct ui_out *uiout);

#endif /* !defined (INFERIOR_H) */
