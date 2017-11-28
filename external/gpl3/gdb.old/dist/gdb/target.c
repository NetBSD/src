/* Select target systems and architectures at runtime for GDB.

   Copyright (C) 1990-2016 Free Software Foundation, Inc.

   Contributed by Cygnus Support.

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
#include "target.h"
#include "target-dcache.h"
#include "gdbcmd.h"
#include "symtab.h"
#include "inferior.h"
#include "infrun.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "dcache.h"
#include <signal.h>
#include "regcache.h"
#include "gdbcore.h"
#include "target-descriptions.h"
#include "gdbthread.h"
#include "solib.h"
#include "exec.h"
#include "inline-frame.h"
#include "tracepoint.h"
#include "gdb/fileio.h"
#include "agent.h"
#include "auxv.h"
#include "target-debug.h"
#include "top.h"
#include "event-top.h"

static void target_info (char *, int);

static void generic_tls_error (void) ATTRIBUTE_NORETURN;

static void default_terminal_info (struct target_ops *, const char *, int);

static int default_watchpoint_addr_within_range (struct target_ops *,
						 CORE_ADDR, CORE_ADDR, int);

static int default_region_ok_for_hw_watchpoint (struct target_ops *,
						CORE_ADDR, int);

static void default_rcmd (struct target_ops *, const char *, struct ui_file *);

static ptid_t default_get_ada_task_ptid (struct target_ops *self,
					 long lwp, long tid);

static int default_follow_fork (struct target_ops *self, int follow_child,
				int detach_fork);

static void default_mourn_inferior (struct target_ops *self);

static int default_search_memory (struct target_ops *ops,
				  CORE_ADDR start_addr,
				  ULONGEST search_space_len,
				  const gdb_byte *pattern,
				  ULONGEST pattern_len,
				  CORE_ADDR *found_addrp);

static int default_verify_memory (struct target_ops *self,
				  const gdb_byte *data,
				  CORE_ADDR memaddr, ULONGEST size);

static struct address_space *default_thread_address_space
     (struct target_ops *self, ptid_t ptid);

static void tcomplain (void) ATTRIBUTE_NORETURN;

static int return_zero (struct target_ops *);

static int return_zero_has_execution (struct target_ops *, ptid_t);

static void target_command (char *, int);

static struct target_ops *find_default_run_target (char *);

static struct gdbarch *default_thread_architecture (struct target_ops *ops,
						    ptid_t ptid);

static int dummy_find_memory_regions (struct target_ops *self,
				      find_memory_region_ftype ignore1,
				      void *ignore2);

static char *dummy_make_corefile_notes (struct target_ops *self,
					bfd *ignore1, int *ignore2);

static char *default_pid_to_str (struct target_ops *ops, ptid_t ptid);

static enum exec_direction_kind default_execution_direction
    (struct target_ops *self);

static struct target_ops debug_target;

#include "target-delegates.c"

static void init_dummy_target (void);

static void update_current_target (void);

/* Vector of existing target structures. */
typedef struct target_ops *target_ops_p;
DEF_VEC_P (target_ops_p);
static VEC (target_ops_p) *target_structs;

/* The initial current target, so that there is always a semi-valid
   current target.  */

static struct target_ops dummy_target;

/* Top of target stack.  */

static struct target_ops *target_stack;

/* The target structure we are currently using to talk to a process
   or file or whatever "inferior" we have.  */

struct target_ops current_target;

/* Command list for target.  */

static struct cmd_list_element *targetlist = NULL;

/* Nonzero if we should trust readonly sections from the
   executable when reading memory.  */

static int trust_readonly = 0;

/* Nonzero if we should show true memory content including
   memory breakpoint inserted by gdb.  */

static int show_memory_breakpoints = 0;

/* These globals control whether GDB attempts to perform these
   operations; they are useful for targets that need to prevent
   inadvertant disruption, such as in non-stop mode.  */

int may_write_registers = 1;

int may_write_memory = 1;

int may_insert_breakpoints = 1;

int may_insert_tracepoints = 1;

int may_insert_fast_tracepoints = 1;

int may_stop = 1;

/* Non-zero if we want to see trace of target level stuff.  */

static unsigned int targetdebug = 0;

static void
set_targetdebug  (char *args, int from_tty, struct cmd_list_element *c)
{
  update_current_target ();
}

static void
show_targetdebug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Target debugging is %s.\n"), value);
}

static void setup_target_debug (void);

/* The user just typed 'target' without the name of a target.  */

static void
target_command (char *arg, int from_tty)
{
  fputs_filtered ("Argument required (target name).  Try `help target'\n",
		  gdb_stdout);
}

/* Default target_has_* methods for process_stratum targets.  */

int
default_child_has_all_memory (struct target_ops *ops)
{
  /* If no inferior selected, then we can't read memory here.  */
  if (ptid_equal (inferior_ptid, null_ptid))
    return 0;

  return 1;
}

int
default_child_has_memory (struct target_ops *ops)
{
  /* If no inferior selected, then we can't read memory here.  */
  if (ptid_equal (inferior_ptid, null_ptid))
    return 0;

  return 1;
}

int
default_child_has_stack (struct target_ops *ops)
{
  /* If no inferior selected, there's no stack.  */
  if (ptid_equal (inferior_ptid, null_ptid))
    return 0;

  return 1;
}

int
default_child_has_registers (struct target_ops *ops)
{
  /* Can't read registers from no inferior.  */
  if (ptid_equal (inferior_ptid, null_ptid))
    return 0;

  return 1;
}

int
default_child_has_execution (struct target_ops *ops, ptid_t the_ptid)
{
  /* If there's no thread selected, then we can't make it run through
     hoops.  */
  if (ptid_equal (the_ptid, null_ptid))
    return 0;

  return 1;
}


int
target_has_all_memory_1 (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_has_all_memory (t))
      return 1;

  return 0;
}

int
target_has_memory_1 (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_has_memory (t))
      return 1;

  return 0;
}

int
target_has_stack_1 (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_has_stack (t))
      return 1;

  return 0;
}

int
target_has_registers_1 (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_has_registers (t))
      return 1;

  return 0;
}

int
target_has_execution_1 (ptid_t the_ptid)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_has_execution (t, the_ptid))
      return 1;

  return 0;
}

int
target_has_execution_current (void)
{
  return target_has_execution_1 (inferior_ptid);
}

/* Complete initialization of T.  This ensures that various fields in
   T are set, if needed by the target implementation.  */

void
complete_target_initialization (struct target_ops *t)
{
  /* Provide default values for all "must have" methods.  */

  if (t->to_has_all_memory == NULL)
    t->to_has_all_memory = return_zero;

  if (t->to_has_memory == NULL)
    t->to_has_memory = return_zero;

  if (t->to_has_stack == NULL)
    t->to_has_stack = return_zero;

  if (t->to_has_registers == NULL)
    t->to_has_registers = return_zero;

  if (t->to_has_execution == NULL)
    t->to_has_execution = return_zero_has_execution;

  /* These methods can be called on an unpushed target and so require
     a default implementation if the target might plausibly be the
     default run target.  */
  gdb_assert (t->to_can_run == NULL || (t->to_can_async_p != NULL
					&& t->to_supports_non_stop != NULL));

  install_delegators (t);
}

/* This is used to implement the various target commands.  */

static void
open_target (char *args, int from_tty, struct cmd_list_element *command)
{
  struct target_ops *ops = (struct target_ops *) get_cmd_context (command);

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog, "-> %s->to_open (...)\n",
			ops->to_shortname);

  ops->to_open (args, from_tty);

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog, "<- %s->to_open (%s, %d)\n",
			ops->to_shortname, args, from_tty);
}

/* Add possible target architecture T to the list and add a new
   command 'target T->to_shortname'.  Set COMPLETER as the command's
   completer if not NULL.  */

void
add_target_with_completer (struct target_ops *t,
			   completer_ftype *completer)
{
  struct cmd_list_element *c;

  complete_target_initialization (t);

  VEC_safe_push (target_ops_p, target_structs, t);

  if (targetlist == NULL)
    add_prefix_cmd ("target", class_run, target_command, _("\
Connect to a target machine or process.\n\
The first argument is the type or protocol of the target machine.\n\
Remaining arguments are interpreted by the target protocol.  For more\n\
information on the arguments for a particular protocol, type\n\
`help target ' followed by the protocol name."),
		    &targetlist, "target ", 0, &cmdlist);
  c = add_cmd (t->to_shortname, no_class, NULL, t->to_doc, &targetlist);
  set_cmd_sfunc (c, open_target);
  set_cmd_context (c, t);
  if (completer != NULL)
    set_cmd_completer (c, completer);
}

/* Add a possible target architecture to the list.  */

void
add_target (struct target_ops *t)
{
  add_target_with_completer (t, NULL);
}

/* See target.h.  */

void
add_deprecated_target_alias (struct target_ops *t, char *alias)
{
  struct cmd_list_element *c;
  char *alt;

  /* If we use add_alias_cmd, here, we do not get the deprecated warning,
     see PR cli/15104.  */
  c = add_cmd (alias, no_class, NULL, t->to_doc, &targetlist);
  set_cmd_sfunc (c, open_target);
  set_cmd_context (c, t);
  alt = xstrprintf ("target %s", t->to_shortname);
  deprecate_cmd (c, alt);
}

/* Stub functions */

void
target_kill (void)
{
  current_target.to_kill (&current_target);
}

void
target_load (const char *arg, int from_tty)
{
  target_dcache_invalidate ();
  (*current_target.to_load) (&current_target, arg, from_tty);
}

/* Possible terminal states.  */

enum terminal_state
  {
    /* The inferior's terminal settings are in effect.  */
    terminal_is_inferior = 0,

    /* Some of our terminal settings are in effect, enough to get
       proper output.  */
    terminal_is_ours_for_output = 1,

    /* Our terminal settings are in effect, for output and input.  */
    terminal_is_ours = 2
  };

static enum terminal_state terminal_state = terminal_is_ours;

/* See target.h.  */

void
target_terminal_init (void)
{
  (*current_target.to_terminal_init) (&current_target);

  terminal_state = terminal_is_ours;
}

/* See target.h.  */

int
target_terminal_is_inferior (void)
{
  return (terminal_state == terminal_is_inferior);
}

/* See target.h.  */

int
target_terminal_is_ours (void)
{
  return (terminal_state == terminal_is_ours);
}

/* See target.h.  */

void
target_terminal_inferior (void)
{
  struct ui *ui = current_ui;

  /* A background resume (``run&'') should leave GDB in control of the
     terminal.  */
  if (ui->prompt_state != PROMPT_BLOCKED)
    return;

  /* Since we always run the inferior in the main console (unless "set
     inferior-tty" is in effect), when some UI other than the main one
     calls target_terminal_inferior/target_terminal_inferior, then we
     leave the main UI's terminal settings as is.  */
  if (ui != main_ui)
    return;

  if (terminal_state == terminal_is_inferior)
    return;

  /* If GDB is resuming the inferior in the foreground, install
     inferior's terminal modes.  */
  (*current_target.to_terminal_inferior) (&current_target);
  terminal_state = terminal_is_inferior;

  /* If the user hit C-c before, pretend that it was hit right
     here.  */
  if (check_quit_flag ())
    target_pass_ctrlc ();
}

/* See target.h.  */

void
target_terminal_ours (void)
{
  struct ui *ui = current_ui;

  /* See target_terminal_inferior.  */
  if (ui != main_ui)
    return;

  if (terminal_state == terminal_is_ours)
    return;

  (*current_target.to_terminal_ours) (&current_target);
  terminal_state = terminal_is_ours;
}

/* See target.h.  */

void
target_terminal_ours_for_output (void)
{
  struct ui *ui = current_ui;

  /* See target_terminal_inferior.  */
  if (ui != main_ui)
    return;

  if (terminal_state != terminal_is_inferior)
    return;
  (*current_target.to_terminal_ours_for_output) (&current_target);
  terminal_state = terminal_is_ours_for_output;
}

/* See target.h.  */

int
target_supports_terminal_ours (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_terminal_ours != delegate_terminal_ours
	  && t->to_terminal_ours != tdefault_terminal_ours)
	return 1;
    }

  return 0;
}

/* Restore the terminal to its previous state (helper for
   make_cleanup_restore_target_terminal). */

static void
cleanup_restore_target_terminal (void *arg)
{
  enum terminal_state *previous_state = (enum terminal_state *) arg;

  switch (*previous_state)
    {
    case terminal_is_ours:
      target_terminal_ours ();
      break;
    case terminal_is_ours_for_output:
      target_terminal_ours_for_output ();
      break;
    case terminal_is_inferior:
      target_terminal_inferior ();
      break;
    }
}

/* See target.h. */

struct cleanup *
make_cleanup_restore_target_terminal (void)
{
  enum terminal_state *ts = XNEW (enum terminal_state);

  *ts = terminal_state;

  return make_cleanup_dtor (cleanup_restore_target_terminal, ts, xfree);
}

static void
tcomplain (void)
{
  error (_("You can't do that when your target is `%s'"),
	 current_target.to_shortname);
}

void
noprocess (void)
{
  error (_("You can't do that without a process to debug."));
}

static void
default_terminal_info (struct target_ops *self, const char *args, int from_tty)
{
  printf_unfiltered (_("No saved terminal information.\n"));
}

/* A default implementation for the to_get_ada_task_ptid target method.

   This function builds the PTID by using both LWP and TID as part of
   the PTID lwp and tid elements.  The pid used is the pid of the
   inferior_ptid.  */

static ptid_t
default_get_ada_task_ptid (struct target_ops *self, long lwp, long tid)
{
  return ptid_build (ptid_get_pid (inferior_ptid), lwp, tid);
}

static enum exec_direction_kind
default_execution_direction (struct target_ops *self)
{
  if (!target_can_execute_reverse)
    return EXEC_FORWARD;
  else if (!target_can_async_p ())
    return EXEC_FORWARD;
  else
    gdb_assert_not_reached ("\
to_execution_direction must be implemented for reverse async");
}

/* Go through the target stack from top to bottom, copying over zero
   entries in current_target, then filling in still empty entries.  In
   effect, we are doing class inheritance through the pushed target
   vectors.

   NOTE: cagney/2003-10-17: The problem with this inheritance, as it
   is currently implemented, is that it discards any knowledge of
   which target an inherited method originally belonged to.
   Consequently, new new target methods should instead explicitly and
   locally search the target stack for the target that can handle the
   request.  */

static void
update_current_target (void)
{
  struct target_ops *t;

  /* First, reset current's contents.  */
  memset (&current_target, 0, sizeof (current_target));

  /* Install the delegators.  */
  install_delegators (&current_target);

  current_target.to_stratum = target_stack->to_stratum;

#define INHERIT(FIELD, TARGET) \
      if (!current_target.FIELD) \
	current_target.FIELD = (TARGET)->FIELD

  /* Do not add any new INHERITs here.  Instead, use the delegation
     mechanism provided by make-target-delegates.  */
  for (t = target_stack; t; t = t->beneath)
    {
      INHERIT (to_shortname, t);
      INHERIT (to_longname, t);
      INHERIT (to_attach_no_wait, t);
      INHERIT (to_have_steppable_watchpoint, t);
      INHERIT (to_have_continuable_watchpoint, t);
      INHERIT (to_has_thread_control, t);
    }
#undef INHERIT

  /* Finally, position the target-stack beneath the squashed
     "current_target".  That way code looking for a non-inherited
     target method can quickly and simply find it.  */
  current_target.beneath = target_stack;

  if (targetdebug)
    setup_target_debug ();
}

/* Push a new target type into the stack of the existing target accessors,
   possibly superseding some of the existing accessors.

   Rather than allow an empty stack, we always have the dummy target at
   the bottom stratum, so we can call the function vectors without
   checking them.  */

void
push_target (struct target_ops *t)
{
  struct target_ops **cur;

  /* Check magic number.  If wrong, it probably means someone changed
     the struct definition, but not all the places that initialize one.  */
  if (t->to_magic != OPS_MAGIC)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Magic number of %s target struct wrong\n",
			  t->to_shortname);
      internal_error (__FILE__, __LINE__,
		      _("failed internal consistency check"));
    }

  /* Find the proper stratum to install this target in.  */
  for (cur = &target_stack; (*cur) != NULL; cur = &(*cur)->beneath)
    {
      if ((int) (t->to_stratum) >= (int) (*cur)->to_stratum)
	break;
    }

  /* If there's already targets at this stratum, remove them.  */
  /* FIXME: cagney/2003-10-15: I think this should be popping all
     targets to CUR, and not just those at this stratum level.  */
  while ((*cur) != NULL && t->to_stratum == (*cur)->to_stratum)
    {
      /* There's already something at this stratum level.  Close it,
         and un-hook it from the stack.  */
      struct target_ops *tmp = (*cur);

      (*cur) = (*cur)->beneath;
      tmp->beneath = NULL;
      target_close (tmp);
    }

  /* We have removed all targets in our stratum, now add the new one.  */
  t->beneath = (*cur);
  (*cur) = t;

  update_current_target ();
}

/* Remove a target_ops vector from the stack, wherever it may be.
   Return how many times it was removed (0 or 1).  */

int
unpush_target (struct target_ops *t)
{
  struct target_ops **cur;
  struct target_ops *tmp;

  if (t->to_stratum == dummy_stratum)
    internal_error (__FILE__, __LINE__,
		    _("Attempt to unpush the dummy target"));

  /* Look for the specified target.  Note that we assume that a target
     can only occur once in the target stack.  */

  for (cur = &target_stack; (*cur) != NULL; cur = &(*cur)->beneath)
    {
      if ((*cur) == t)
	break;
    }

  /* If we don't find target_ops, quit.  Only open targets should be
     closed.  */
  if ((*cur) == NULL)
    return 0;			

  /* Unchain the target.  */
  tmp = (*cur);
  (*cur) = (*cur)->beneath;
  tmp->beneath = NULL;

  update_current_target ();

  /* Finally close the target.  Note we do this after unchaining, so
     any target method calls from within the target_close
     implementation don't end up in T anymore.  */
  target_close (t);

  return 1;
}

/* Unpush TARGET and assert that it worked.  */

static void
unpush_target_and_assert (struct target_ops *target)
{
  if (!unpush_target (target))
    {
      fprintf_unfiltered (gdb_stderr,
			  "pop_all_targets couldn't find target %s\n",
			  target->to_shortname);
      internal_error (__FILE__, __LINE__,
		      _("failed internal consistency check"));
    }
}

void
pop_all_targets_above (enum strata above_stratum)
{
  while ((int) (current_target.to_stratum) > (int) above_stratum)
    unpush_target_and_assert (target_stack);
}

/* See target.h.  */

void
pop_all_targets_at_and_above (enum strata stratum)
{
  while ((int) (current_target.to_stratum) >= (int) stratum)
    unpush_target_and_assert (target_stack);
}

void
pop_all_targets (void)
{
  pop_all_targets_above (dummy_stratum);
}

/* Return 1 if T is now pushed in the target stack.  Return 0 otherwise.  */

int
target_is_pushed (struct target_ops *t)
{
  struct target_ops *cur;

  /* Check magic number.  If wrong, it probably means someone changed
     the struct definition, but not all the places that initialize one.  */
  if (t->to_magic != OPS_MAGIC)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Magic number of %s target struct wrong\n",
			  t->to_shortname);
      internal_error (__FILE__, __LINE__,
		      _("failed internal consistency check"));
    }

  for (cur = target_stack; cur != NULL; cur = cur->beneath)
    if (cur == t)
      return 1;

  return 0;
}

/* Default implementation of to_get_thread_local_address.  */

static void
generic_tls_error (void)
{
  throw_error (TLS_GENERIC_ERROR,
	       _("Cannot find thread-local variables on this target"));
}

/* Using the objfile specified in OBJFILE, find the address for the
   current thread's thread-local storage with offset OFFSET.  */
CORE_ADDR
target_translate_tls_address (struct objfile *objfile, CORE_ADDR offset)
{
  volatile CORE_ADDR addr = 0;
  struct target_ops *target = &current_target;

  if (gdbarch_fetch_tls_load_module_address_p (target_gdbarch ()))
    {
      ptid_t ptid = inferior_ptid;

      TRY
	{
	  CORE_ADDR lm_addr;
	  
	  /* Fetch the load module address for this objfile.  */
	  lm_addr = gdbarch_fetch_tls_load_module_address (target_gdbarch (),
	                                                   objfile);

	  addr = target->to_get_thread_local_address (target, ptid,
						      lm_addr, offset);
	}
      /* If an error occurred, print TLS related messages here.  Otherwise,
         throw the error to some higher catcher.  */
      CATCH (ex, RETURN_MASK_ALL)
	{
	  int objfile_is_library = (objfile->flags & OBJF_SHARED);

	  switch (ex.error)
	    {
	    case TLS_NO_LIBRARY_SUPPORT_ERROR:
	      error (_("Cannot find thread-local variables "
		       "in this thread library."));
	      break;
	    case TLS_LOAD_MODULE_NOT_FOUND_ERROR:
	      if (objfile_is_library)
		error (_("Cannot find shared library `%s' in dynamic"
		         " linker's load module list"), objfile_name (objfile));
	      else
		error (_("Cannot find executable file `%s' in dynamic"
		         " linker's load module list"), objfile_name (objfile));
	      break;
	    case TLS_NOT_ALLOCATED_YET_ERROR:
	      if (objfile_is_library)
		error (_("The inferior has not yet allocated storage for"
		         " thread-local variables in\n"
		         "the shared library `%s'\n"
		         "for %s"),
		       objfile_name (objfile), target_pid_to_str (ptid));
	      else
		error (_("The inferior has not yet allocated storage for"
		         " thread-local variables in\n"
		         "the executable `%s'\n"
		         "for %s"),
		       objfile_name (objfile), target_pid_to_str (ptid));
	      break;
	    case TLS_GENERIC_ERROR:
	      if (objfile_is_library)
		error (_("Cannot find thread-local storage for %s, "
		         "shared library %s:\n%s"),
		       target_pid_to_str (ptid),
		       objfile_name (objfile), ex.message);
	      else
		error (_("Cannot find thread-local storage for %s, "
		         "executable file %s:\n%s"),
		       target_pid_to_str (ptid),
		       objfile_name (objfile), ex.message);
	      break;
	    default:
	      throw_exception (ex);
	      break;
	    }
	}
      END_CATCH
    }
  /* It wouldn't be wrong here to try a gdbarch method, too; finding
     TLS is an ABI-specific thing.  But we don't do that yet.  */
  else
    error (_("Cannot find thread-local variables on this target"));

  return addr;
}

const char *
target_xfer_status_to_string (enum target_xfer_status status)
{
#define CASE(X) case X: return #X
  switch (status)
    {
      CASE(TARGET_XFER_E_IO);
      CASE(TARGET_XFER_UNAVAILABLE);
    default:
      return "<unknown>";
    }
#undef CASE
};


#undef	MIN
#define MIN(A, B) (((A) <= (B)) ? (A) : (B))

/* target_read_string -- read a null terminated string, up to LEN bytes,
   from MEMADDR in target.  Set *ERRNOP to the errno code, or 0 if successful.
   Set *STRING to a pointer to malloc'd memory containing the data; the caller
   is responsible for freeing it.  Return the number of bytes successfully
   read.  */

int
target_read_string (CORE_ADDR memaddr, char **string, int len, int *errnop)
{
  int tlen, offset, i;
  gdb_byte buf[4];
  int errcode = 0;
  char *buffer;
  int buffer_allocated;
  char *bufptr;
  unsigned int nbytes_read = 0;

  gdb_assert (string);

  /* Small for testing.  */
  buffer_allocated = 4;
  buffer = (char *) xmalloc (buffer_allocated);
  bufptr = buffer;

  while (len > 0)
    {
      tlen = MIN (len, 4 - (memaddr & 3));
      offset = memaddr & 3;

      errcode = target_read_memory (memaddr & ~3, buf, sizeof buf);
      if (errcode != 0)
	{
	  /* The transfer request might have crossed the boundary to an
	     unallocated region of memory.  Retry the transfer, requesting
	     a single byte.  */
	  tlen = 1;
	  offset = 0;
	  errcode = target_read_memory (memaddr, buf, 1);
	  if (errcode != 0)
	    goto done;
	}

      if (bufptr - buffer + tlen > buffer_allocated)
	{
	  unsigned int bytes;

	  bytes = bufptr - buffer;
	  buffer_allocated *= 2;
	  buffer = (char *) xrealloc (buffer, buffer_allocated);
	  bufptr = buffer + bytes;
	}

      for (i = 0; i < tlen; i++)
	{
	  *bufptr++ = buf[i + offset];
	  if (buf[i + offset] == '\000')
	    {
	      nbytes_read += i + 1;
	      goto done;
	    }
	}

      memaddr += tlen;
      len -= tlen;
      nbytes_read += tlen;
    }
done:
  *string = buffer;
  if (errnop != NULL)
    *errnop = errcode;
  return nbytes_read;
}

struct target_section_table *
target_get_section_table (struct target_ops *target)
{
  return (*target->to_get_section_table) (target);
}

/* Find a section containing ADDR.  */

struct target_section *
target_section_by_addr (struct target_ops *target, CORE_ADDR addr)
{
  struct target_section_table *table = target_get_section_table (target);
  struct target_section *secp;

  if (table == NULL)
    return NULL;

  for (secp = table->sections; secp < table->sections_end; secp++)
    {
      if (addr >= secp->addr && addr < secp->endaddr)
	return secp;
    }
  return NULL;
}


/* Helper for the memory xfer routines.  Checks the attributes of the
   memory region of MEMADDR against the read or write being attempted.
   If the access is permitted returns true, otherwise returns false.
   REGION_P is an optional output parameter.  If not-NULL, it is
   filled with a pointer to the memory region of MEMADDR.  REG_LEN
   returns LEN trimmed to the end of the region.  This is how much the
   caller can continue requesting, if the access is permitted.  A
   single xfer request must not straddle memory region boundaries.  */

static int
memory_xfer_check_region (gdb_byte *readbuf, const gdb_byte *writebuf,
			  ULONGEST memaddr, ULONGEST len, ULONGEST *reg_len,
			  struct mem_region **region_p)
{
  struct mem_region *region;

  region = lookup_mem_region (memaddr);

  if (region_p != NULL)
    *region_p = region;

  switch (region->attrib.mode)
    {
    case MEM_RO:
      if (writebuf != NULL)
	return 0;
      break;

    case MEM_WO:
      if (readbuf != NULL)
	return 0;
      break;

    case MEM_FLASH:
      /* We only support writing to flash during "load" for now.  */
      if (writebuf != NULL)
	error (_("Writing to flash memory forbidden in this context"));
      break;

    case MEM_NONE:
      return 0;
    }

  /* region->hi == 0 means there's no upper bound.  */
  if (memaddr + len < region->hi || region->hi == 0)
    *reg_len = len;
  else
    *reg_len = region->hi - memaddr;

  return 1;
}

/* Read memory from more than one valid target.  A core file, for
   instance, could have some of memory but delegate other bits to
   the target below it.  So, we must manually try all targets.  */

enum target_xfer_status
raw_memory_xfer_partial (struct target_ops *ops, gdb_byte *readbuf,
			 const gdb_byte *writebuf, ULONGEST memaddr, LONGEST len,
			 ULONGEST *xfered_len)
{
  enum target_xfer_status res;

  do
    {
      res = ops->to_xfer_partial (ops, TARGET_OBJECT_MEMORY, NULL,
				  readbuf, writebuf, memaddr, len,
				  xfered_len);
      if (res == TARGET_XFER_OK)
	break;

      /* Stop if the target reports that the memory is not available.  */
      if (res == TARGET_XFER_UNAVAILABLE)
	break;

      /* We want to continue past core files to executables, but not
	 past a running target's memory.  */
      if (ops->to_has_all_memory (ops))
	break;

      ops = ops->beneath;
    }
  while (ops != NULL);

  /* The cache works at the raw memory level.  Make sure the cache
     gets updated with raw contents no matter what kind of memory
     object was originally being written.  Note we do write-through
     first, so that if it fails, we don't write to the cache contents
     that never made it to the target.  */
  if (writebuf != NULL
      && !ptid_equal (inferior_ptid, null_ptid)
      && target_dcache_init_p ()
      && (stack_cache_enabled_p () || code_cache_enabled_p ()))
    {
      DCACHE *dcache = target_dcache_get ();

      /* Note that writing to an area of memory which wasn't present
	 in the cache doesn't cause it to be loaded in.  */
      dcache_update (dcache, res, memaddr, writebuf, *xfered_len);
    }

  return res;
}

/* Perform a partial memory transfer.
   For docs see target.h, to_xfer_partial.  */

static enum target_xfer_status
memory_xfer_partial_1 (struct target_ops *ops, enum target_object object,
		       gdb_byte *readbuf, const gdb_byte *writebuf, ULONGEST memaddr,
		       ULONGEST len, ULONGEST *xfered_len)
{
  enum target_xfer_status res;
  ULONGEST reg_len;
  struct mem_region *region;
  struct inferior *inf;

  /* For accesses to unmapped overlay sections, read directly from
     files.  Must do this first, as MEMADDR may need adjustment.  */
  if (readbuf != NULL && overlay_debugging)
    {
      struct obj_section *section = find_pc_overlay (memaddr);

      if (pc_in_unmapped_range (memaddr, section))
	{
	  struct target_section_table *table
	    = target_get_section_table (ops);
	  const char *section_name = section->the_bfd_section->name;

	  memaddr = overlay_mapped_address (memaddr, section);
	  return section_table_xfer_memory_partial (readbuf, writebuf,
						    memaddr, len, xfered_len,
						    table->sections,
						    table->sections_end,
						    section_name);
	}
    }

  /* Try the executable files, if "trust-readonly-sections" is set.  */
  if (readbuf != NULL && trust_readonly)
    {
      struct target_section *secp;
      struct target_section_table *table;

      secp = target_section_by_addr (ops, memaddr);
      if (secp != NULL
	  && (bfd_get_section_flags (secp->the_bfd_section->owner,
				     secp->the_bfd_section)
	      & SEC_READONLY))
	{
	  table = target_get_section_table (ops);
	  return section_table_xfer_memory_partial (readbuf, writebuf,
						    memaddr, len, xfered_len,
						    table->sections,
						    table->sections_end,
						    NULL);
	}
    }

  /* Try GDB's internal data cache.  */

  if (!memory_xfer_check_region (readbuf, writebuf, memaddr, len, &reg_len,
				 &region))
    return TARGET_XFER_E_IO;

  if (!ptid_equal (inferior_ptid, null_ptid))
    inf = find_inferior_ptid (inferior_ptid);
  else
    inf = NULL;

  if (inf != NULL
      && readbuf != NULL
      /* The dcache reads whole cache lines; that doesn't play well
	 with reading from a trace buffer, because reading outside of
	 the collected memory range fails.  */
      && get_traceframe_number () == -1
      && (region->attrib.cache
	  || (stack_cache_enabled_p () && object == TARGET_OBJECT_STACK_MEMORY)
	  || (code_cache_enabled_p () && object == TARGET_OBJECT_CODE_MEMORY)))
    {
      DCACHE *dcache = target_dcache_get_or_init ();

      return dcache_read_memory_partial (ops, dcache, memaddr, readbuf,
					 reg_len, xfered_len);
    }

  /* If none of those methods found the memory we wanted, fall back
     to a target partial transfer.  Normally a single call to
     to_xfer_partial is enough; if it doesn't recognize an object
     it will call the to_xfer_partial of the next target down.
     But for memory this won't do.  Memory is the only target
     object which can be read from more than one valid target.
     A core file, for instance, could have some of memory but
     delegate other bits to the target below it.  So, we must
     manually try all targets.  */

  res = raw_memory_xfer_partial (ops, readbuf, writebuf, memaddr, reg_len,
				 xfered_len);

  /* If we still haven't got anything, return the last error.  We
     give up.  */
  return res;
}

/* Perform a partial memory transfer.  For docs see target.h,
   to_xfer_partial.  */

static enum target_xfer_status
memory_xfer_partial (struct target_ops *ops, enum target_object object,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST memaddr, ULONGEST len, ULONGEST *xfered_len)
{
  enum target_xfer_status res;

  /* Zero length requests are ok and require no work.  */
  if (len == 0)
    return TARGET_XFER_EOF;

  /* Fill in READBUF with breakpoint shadows, or WRITEBUF with
     breakpoint insns, thus hiding out from higher layers whether
     there are software breakpoints inserted in the code stream.  */
  if (readbuf != NULL)
    {
      res = memory_xfer_partial_1 (ops, object, readbuf, NULL, memaddr, len,
				   xfered_len);

      if (res == TARGET_XFER_OK && !show_memory_breakpoints)
	breakpoint_xfer_memory (readbuf, NULL, NULL, memaddr, *xfered_len);
    }
  else
    {
      gdb_byte *buf;
      struct cleanup *old_chain;

      /* A large write request is likely to be partially satisfied
	 by memory_xfer_partial_1.  We will continually malloc
	 and free a copy of the entire write request for breakpoint
	 shadow handling even though we only end up writing a small
	 subset of it.  Cap writes to a limit specified by the target
	 to mitigate this.  */
      len = min (ops->to_get_memory_xfer_limit (ops), len);

      buf = (gdb_byte *) xmalloc (len);
      old_chain = make_cleanup (xfree, buf);
      memcpy (buf, writebuf, len);

      breakpoint_xfer_memory (NULL, buf, writebuf, memaddr, len);
      res = memory_xfer_partial_1 (ops, object, NULL, buf, memaddr, len,
				   xfered_len);

      do_cleanups (old_chain);
    }

  return res;
}

static void
restore_show_memory_breakpoints (void *arg)
{
  show_memory_breakpoints = (uintptr_t) arg;
}

struct cleanup *
make_show_memory_breakpoints_cleanup (int show)
{
  int current = show_memory_breakpoints;

  show_memory_breakpoints = show;
  return make_cleanup (restore_show_memory_breakpoints,
		       (void *) (uintptr_t) current);
}

/* For docs see target.h, to_xfer_partial.  */

enum target_xfer_status
target_xfer_partial (struct target_ops *ops,
		     enum target_object object, const char *annex,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST offset, ULONGEST len,
		     ULONGEST *xfered_len)
{
  enum target_xfer_status retval;

  gdb_assert (ops->to_xfer_partial != NULL);

  /* Transfer is done when LEN is zero.  */
  if (len == 0)
    return TARGET_XFER_EOF;

  if (writebuf && !may_write_memory)
    error (_("Writing to memory is not allowed (addr %s, len %s)"),
	   core_addr_to_string_nz (offset), plongest (len));

  *xfered_len = 0;

  /* If this is a memory transfer, let the memory-specific code
     have a look at it instead.  Memory transfers are more
     complicated.  */
  if (object == TARGET_OBJECT_MEMORY || object == TARGET_OBJECT_STACK_MEMORY
      || object == TARGET_OBJECT_CODE_MEMORY)
    retval = memory_xfer_partial (ops, object, readbuf,
				  writebuf, offset, len, xfered_len);
  else if (object == TARGET_OBJECT_RAW_MEMORY)
    {
      /* Skip/avoid accessing the target if the memory region
	 attributes block the access.  Check this here instead of in
	 raw_memory_xfer_partial as otherwise we'd end up checking
	 this twice in the case of the memory_xfer_partial path is
	 taken; once before checking the dcache, and another in the
	 tail call to raw_memory_xfer_partial.  */
      if (!memory_xfer_check_region (readbuf, writebuf, offset, len, &len,
				     NULL))
	return TARGET_XFER_E_IO;

      /* Request the normal memory object from other layers.  */
      retval = raw_memory_xfer_partial (ops, readbuf, writebuf, offset, len,
					xfered_len);
    }
  else
    retval = ops->to_xfer_partial (ops, object, annex, readbuf,
				   writebuf, offset, len, xfered_len);

  if (targetdebug)
    {
      const unsigned char *myaddr = NULL;

      fprintf_unfiltered (gdb_stdlog,
			  "%s:target_xfer_partial "
			  "(%d, %s, %s, %s, %s, %s) = %d, %s",
			  ops->to_shortname,
			  (int) object,
			  (annex ? annex : "(null)"),
			  host_address_to_string (readbuf),
			  host_address_to_string (writebuf),
			  core_addr_to_string_nz (offset),
			  pulongest (len), retval,
			  pulongest (*xfered_len));

      if (readbuf)
	myaddr = readbuf;
      if (writebuf)
	myaddr = writebuf;
      if (retval == TARGET_XFER_OK && myaddr != NULL)
	{
	  int i;

	  fputs_unfiltered (", bytes =", gdb_stdlog);
	  for (i = 0; i < *xfered_len; i++)
	    {
	      if ((((intptr_t) &(myaddr[i])) & 0xf) == 0)
		{
		  if (targetdebug < 2 && i > 0)
		    {
		      fprintf_unfiltered (gdb_stdlog, " ...");
		      break;
		    }
		  fprintf_unfiltered (gdb_stdlog, "\n");
		}

	      fprintf_unfiltered (gdb_stdlog, " %02x", myaddr[i] & 0xff);
	    }
	}

      fputc_unfiltered ('\n', gdb_stdlog);
    }

  /* Check implementations of to_xfer_partial update *XFERED_LEN
     properly.  Do assertion after printing debug messages, so that we
     can find more clues on assertion failure from debugging messages.  */
  if (retval == TARGET_XFER_OK || retval == TARGET_XFER_UNAVAILABLE)
    gdb_assert (*xfered_len > 0);

  return retval;
}

/* Read LEN bytes of target memory at address MEMADDR, placing the
   results in GDB's memory at MYADDR.  Returns either 0 for success or
   -1 if any error occurs.

   If an error occurs, no guarantee is made about the contents of the data at
   MYADDR.  In particular, the caller should not depend upon partial reads
   filling the buffer with good data.  There is no way for the caller to know
   how much good data might have been transfered anyway.  Callers that can
   deal with partial reads should call target_read (which will retry until
   it makes no progress, and then return how much was transferred).  */

int
target_read_memory (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  /* Dispatch to the topmost target, not the flattened current_target.
     Memory accesses check target->to_has_(all_)memory, and the
     flattened target doesn't inherit those.  */
  if (target_read (current_target.beneath, TARGET_OBJECT_MEMORY, NULL,
		   myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* See target/target.h.  */

int
target_read_uint32 (CORE_ADDR memaddr, uint32_t *result)
{
  gdb_byte buf[4];
  int r;

  r = target_read_memory (memaddr, buf, sizeof buf);
  if (r != 0)
    return r;
  *result = extract_unsigned_integer (buf, sizeof buf,
				      gdbarch_byte_order (target_gdbarch ()));
  return 0;
}

/* Like target_read_memory, but specify explicitly that this is a read
   from the target's raw memory.  That is, this read bypasses the
   dcache, breakpoint shadowing, etc.  */

int
target_read_raw_memory (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  /* See comment in target_read_memory about why the request starts at
     current_target.beneath.  */
  if (target_read (current_target.beneath, TARGET_OBJECT_RAW_MEMORY, NULL,
		   myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Like target_read_memory, but specify explicitly that this is a read from
   the target's stack.  This may trigger different cache behavior.  */

int
target_read_stack (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  /* See comment in target_read_memory about why the request starts at
     current_target.beneath.  */
  if (target_read (current_target.beneath, TARGET_OBJECT_STACK_MEMORY, NULL,
		   myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Like target_read_memory, but specify explicitly that this is a read from
   the target's code.  This may trigger different cache behavior.  */

int
target_read_code (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  /* See comment in target_read_memory about why the request starts at
     current_target.beneath.  */
  if (target_read (current_target.beneath, TARGET_OBJECT_CODE_MEMORY, NULL,
		   myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Write LEN bytes from MYADDR to target memory at address MEMADDR.
   Returns either 0 for success or -1 if any error occurs.  If an
   error occurs, no guarantee is made about how much data got written.
   Callers that can deal with partial writes should call
   target_write.  */

int
target_write_memory (CORE_ADDR memaddr, const gdb_byte *myaddr, ssize_t len)
{
  /* See comment in target_read_memory about why the request starts at
     current_target.beneath.  */
  if (target_write (current_target.beneath, TARGET_OBJECT_MEMORY, NULL,
		    myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Write LEN bytes from MYADDR to target raw memory at address
   MEMADDR.  Returns either 0 for success or -1 if any error occurs.
   If an error occurs, no guarantee is made about how much data got
   written.  Callers that can deal with partial writes should call
   target_write.  */

int
target_write_raw_memory (CORE_ADDR memaddr, const gdb_byte *myaddr, ssize_t len)
{
  /* See comment in target_read_memory about why the request starts at
     current_target.beneath.  */
  if (target_write (current_target.beneath, TARGET_OBJECT_RAW_MEMORY, NULL,
		    myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Fetch the target's memory map.  */

VEC(mem_region_s) *
target_memory_map (void)
{
  VEC(mem_region_s) *result;
  struct mem_region *last_one, *this_one;
  int ix;
  result = current_target.to_memory_map (&current_target);
  if (result == NULL)
    return NULL;

  qsort (VEC_address (mem_region_s, result),
	 VEC_length (mem_region_s, result),
	 sizeof (struct mem_region), mem_region_cmp);

  /* Check that regions do not overlap.  Simultaneously assign
     a numbering for the "mem" commands to use to refer to
     each region.  */
  last_one = NULL;
  for (ix = 0; VEC_iterate (mem_region_s, result, ix, this_one); ix++)
    {
      this_one->number = ix;

      if (last_one && last_one->hi > this_one->lo)
	{
	  warning (_("Overlapping regions in memory map: ignoring"));
	  VEC_free (mem_region_s, result);
	  return NULL;
	}
      last_one = this_one;
    }

  return result;
}

void
target_flash_erase (ULONGEST address, LONGEST length)
{
  current_target.to_flash_erase (&current_target, address, length);
}

void
target_flash_done (void)
{
  current_target.to_flash_done (&current_target);
}

static void
show_trust_readonly (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Mode for reading from readonly sections is %s.\n"),
		    value);
}

/* Target vector read/write partial wrapper functions.  */

static enum target_xfer_status
target_read_partial (struct target_ops *ops,
		     enum target_object object,
		     const char *annex, gdb_byte *buf,
		     ULONGEST offset, ULONGEST len,
		     ULONGEST *xfered_len)
{
  return target_xfer_partial (ops, object, annex, buf, NULL, offset, len,
			      xfered_len);
}

static enum target_xfer_status
target_write_partial (struct target_ops *ops,
		      enum target_object object,
		      const char *annex, const gdb_byte *buf,
		      ULONGEST offset, LONGEST len, ULONGEST *xfered_len)
{
  return target_xfer_partial (ops, object, annex, NULL, buf, offset, len,
			      xfered_len);
}

/* Wrappers to perform the full transfer.  */

/* For docs on target_read see target.h.  */

LONGEST
target_read (struct target_ops *ops,
	     enum target_object object,
	     const char *annex, gdb_byte *buf,
	     ULONGEST offset, LONGEST len)
{
  LONGEST xfered_total = 0;
  int unit_size = 1;

  /* If we are reading from a memory object, find the length of an addressable
     unit for that architecture.  */
  if (object == TARGET_OBJECT_MEMORY
      || object == TARGET_OBJECT_STACK_MEMORY
      || object == TARGET_OBJECT_CODE_MEMORY
      || object == TARGET_OBJECT_RAW_MEMORY)
    unit_size = gdbarch_addressable_memory_unit_size (target_gdbarch ());

  while (xfered_total < len)
    {
      ULONGEST xfered_partial;
      enum target_xfer_status status;

      status = target_read_partial (ops, object, annex,
				    buf + xfered_total * unit_size,
				    offset + xfered_total, len - xfered_total,
				    &xfered_partial);

      /* Call an observer, notifying them of the xfer progress?  */
      if (status == TARGET_XFER_EOF)
	return xfered_total;
      else if (status == TARGET_XFER_OK)
	{
	  xfered_total += xfered_partial;
	  QUIT;
	}
      else
	return TARGET_XFER_E_IO;

    }
  return len;
}

/* Assuming that the entire [begin, end) range of memory cannot be
   read, try to read whatever subrange is possible to read.

   The function returns, in RESULT, either zero or one memory block.
   If there's a readable subrange at the beginning, it is completely
   read and returned.  Any further readable subrange will not be read.
   Otherwise, if there's a readable subrange at the end, it will be
   completely read and returned.  Any readable subranges before it
   (obviously, not starting at the beginning), will be ignored.  In
   other cases -- either no readable subrange, or readable subrange(s)
   that is neither at the beginning, or end, nothing is returned.

   The purpose of this function is to handle a read across a boundary
   of accessible memory in a case when memory map is not available.
   The above restrictions are fine for this case, but will give
   incorrect results if the memory is 'patchy'.  However, supporting
   'patchy' memory would require trying to read every single byte,
   and it seems unacceptable solution.  Explicit memory map is
   recommended for this case -- and target_read_memory_robust will
   take care of reading multiple ranges then.  */

static void
read_whatever_is_readable (struct target_ops *ops,
			   const ULONGEST begin, const ULONGEST end,
			   int unit_size,
			   VEC(memory_read_result_s) **result)
{
  gdb_byte *buf = (gdb_byte *) xmalloc (end - begin);
  ULONGEST current_begin = begin;
  ULONGEST current_end = end;
  int forward;
  memory_read_result_s r;
  ULONGEST xfered_len;

  /* If we previously failed to read 1 byte, nothing can be done here.  */
  if (end - begin <= 1)
    {
      xfree (buf);
      return;
    }

  /* Check that either first or the last byte is readable, and give up
     if not.  This heuristic is meant to permit reading accessible memory
     at the boundary of accessible region.  */
  if (target_read_partial (ops, TARGET_OBJECT_MEMORY, NULL,
			   buf, begin, 1, &xfered_len) == TARGET_XFER_OK)
    {
      forward = 1;
      ++current_begin;
    }
  else if (target_read_partial (ops, TARGET_OBJECT_MEMORY, NULL,
				buf + (end - begin) - 1, end - 1, 1,
				&xfered_len) == TARGET_XFER_OK)
    {
      forward = 0;
      --current_end;
    }
  else
    {
      xfree (buf);
      return;
    }

  /* Loop invariant is that the [current_begin, current_end) was previously
     found to be not readable as a whole.

     Note loop condition -- if the range has 1 byte, we can't divide the range
     so there's no point trying further.  */
  while (current_end - current_begin > 1)
    {
      ULONGEST first_half_begin, first_half_end;
      ULONGEST second_half_begin, second_half_end;
      LONGEST xfer;
      ULONGEST middle = current_begin + (current_end - current_begin) / 2;

      if (forward)
	{
	  first_half_begin = current_begin;
	  first_half_end = middle;
	  second_half_begin = middle;
	  second_half_end = current_end;
	}
      else
	{
	  first_half_begin = middle;
	  first_half_end = current_end;
	  second_half_begin = current_begin;
	  second_half_end = middle;
	}

      xfer = target_read (ops, TARGET_OBJECT_MEMORY, NULL,
			  buf + (first_half_begin - begin) * unit_size,
			  first_half_begin,
			  first_half_end - first_half_begin);

      if (xfer == first_half_end - first_half_begin)
	{
	  /* This half reads up fine.  So, the error must be in the
	     other half.  */
	  current_begin = second_half_begin;
	  current_end = second_half_end;
	}
      else
	{
	  /* This half is not readable.  Because we've tried one byte, we
	     know some part of this half if actually readable.  Go to the next
	     iteration to divide again and try to read.

	     We don't handle the other half, because this function only tries
	     to read a single readable subrange.  */
	  current_begin = first_half_begin;
	  current_end = first_half_end;
	}
    }

  if (forward)
    {
      /* The [begin, current_begin) range has been read.  */
      r.begin = begin;
      r.end = current_begin;
      r.data = buf;
    }
  else
    {
      /* The [current_end, end) range has been read.  */
      LONGEST region_len = end - current_end;

      r.data = (gdb_byte *) xmalloc (region_len * unit_size);
      memcpy (r.data, buf + (current_end - begin) * unit_size,
	      region_len * unit_size);
      r.begin = current_end;
      r.end = end;
      xfree (buf);
    }
  VEC_safe_push(memory_read_result_s, (*result), &r);
}

void
free_memory_read_result_vector (void *x)
{
  VEC(memory_read_result_s) **v = (VEC(memory_read_result_s) **) x;
  memory_read_result_s *current;
  int ix;

  for (ix = 0; VEC_iterate (memory_read_result_s, *v, ix, current); ++ix)
    {
      xfree (current->data);
    }
  VEC_free (memory_read_result_s, *v);
}

VEC(memory_read_result_s) *
read_memory_robust (struct target_ops *ops,
		    const ULONGEST offset, const LONGEST len)
{
  VEC(memory_read_result_s) *result = 0;
  int unit_size = gdbarch_addressable_memory_unit_size (target_gdbarch ());
  struct cleanup *cleanup = make_cleanup (free_memory_read_result_vector,
					  &result);

  LONGEST xfered_total = 0;
  while (xfered_total < len)
    {
      struct mem_region *region = lookup_mem_region (offset + xfered_total);
      LONGEST region_len;

      /* If there is no explicit region, a fake one should be created.  */
      gdb_assert (region);

      if (region->hi == 0)
	region_len = len - xfered_total;
      else
	region_len = region->hi - offset;

      if (region->attrib.mode == MEM_NONE || region->attrib.mode == MEM_WO)
	{
	  /* Cannot read this region.  Note that we can end up here only
	     if the region is explicitly marked inaccessible, or
	     'inaccessible-by-default' is in effect.  */
	  xfered_total += region_len;
	}
      else
	{
	  LONGEST to_read = min (len - xfered_total, region_len);
	  gdb_byte *buffer = (gdb_byte *) xmalloc (to_read * unit_size);
	  struct cleanup *inner_cleanup = make_cleanup (xfree, buffer);

	  LONGEST xfered_partial =
	      target_read (ops, TARGET_OBJECT_MEMORY, NULL,
			   (gdb_byte *) buffer,
			   offset + xfered_total, to_read);
	  /* Call an observer, notifying them of the xfer progress?  */
	  if (xfered_partial <= 0)
	    {
	      /* Got an error reading full chunk.  See if maybe we can read
		 some subrange.  */
	      do_cleanups (inner_cleanup);
	      read_whatever_is_readable (ops, offset + xfered_total,
					 offset + xfered_total + to_read,
					 unit_size, &result);
	      xfered_total += to_read;
	    }
	  else
	    {
	      struct memory_read_result r;

	      discard_cleanups (inner_cleanup);
	      r.data = buffer;
	      r.begin = offset + xfered_total;
	      r.end = r.begin + xfered_partial;
	      VEC_safe_push (memory_read_result_s, result, &r);
	      xfered_total += xfered_partial;
	    }
	  QUIT;
	}
    }

  discard_cleanups (cleanup);
  return result;
}


/* An alternative to target_write with progress callbacks.  */

LONGEST
target_write_with_progress (struct target_ops *ops,
			    enum target_object object,
			    const char *annex, const gdb_byte *buf,
			    ULONGEST offset, LONGEST len,
			    void (*progress) (ULONGEST, void *), void *baton)
{
  LONGEST xfered_total = 0;
  int unit_size = 1;

  /* If we are writing to a memory object, find the length of an addressable
     unit for that architecture.  */
  if (object == TARGET_OBJECT_MEMORY
      || object == TARGET_OBJECT_STACK_MEMORY
      || object == TARGET_OBJECT_CODE_MEMORY
      || object == TARGET_OBJECT_RAW_MEMORY)
    unit_size = gdbarch_addressable_memory_unit_size (target_gdbarch ());

  /* Give the progress callback a chance to set up.  */
  if (progress)
    (*progress) (0, baton);

  while (xfered_total < len)
    {
      ULONGEST xfered_partial;
      enum target_xfer_status status;

      status = target_write_partial (ops, object, annex,
				     buf + xfered_total * unit_size,
				     offset + xfered_total, len - xfered_total,
				     &xfered_partial);

      if (status != TARGET_XFER_OK)
	return status == TARGET_XFER_EOF ? xfered_total : TARGET_XFER_E_IO;

      if (progress)
	(*progress) (xfered_partial, baton);

      xfered_total += xfered_partial;
      QUIT;
    }
  return len;
}

/* For docs on target_write see target.h.  */

LONGEST
target_write (struct target_ops *ops,
	      enum target_object object,
	      const char *annex, const gdb_byte *buf,
	      ULONGEST offset, LONGEST len)
{
  return target_write_with_progress (ops, object, annex, buf, offset, len,
				     NULL, NULL);
}

/* Read OBJECT/ANNEX using OPS.  Store the result in *BUF_P and return
   the size of the transferred data.  PADDING additional bytes are
   available in *BUF_P.  This is a helper function for
   target_read_alloc; see the declaration of that function for more
   information.  */

static LONGEST
target_read_alloc_1 (struct target_ops *ops, enum target_object object,
		     const char *annex, gdb_byte **buf_p, int padding)
{
  size_t buf_alloc, buf_pos;
  gdb_byte *buf;

  /* This function does not have a length parameter; it reads the
     entire OBJECT).  Also, it doesn't support objects fetched partly
     from one target and partly from another (in a different stratum,
     e.g. a core file and an executable).  Both reasons make it
     unsuitable for reading memory.  */
  gdb_assert (object != TARGET_OBJECT_MEMORY);

  /* Start by reading up to 4K at a time.  The target will throttle
     this number down if necessary.  */
  buf_alloc = 4096;
  buf = (gdb_byte *) xmalloc (buf_alloc);
  buf_pos = 0;
  while (1)
    {
      ULONGEST xfered_len;
      enum target_xfer_status status;

      status = target_read_partial (ops, object, annex, &buf[buf_pos],
				    buf_pos, buf_alloc - buf_pos - padding,
				    &xfered_len);

      if (status == TARGET_XFER_EOF)
	{
	  /* Read all there was.  */
	  if (buf_pos == 0)
	    xfree (buf);
	  else
	    *buf_p = buf;
	  return buf_pos;
	}
      else if (status != TARGET_XFER_OK)
	{
	  /* An error occurred.  */
	  xfree (buf);
	  return TARGET_XFER_E_IO;
	}

      buf_pos += xfered_len;

      /* If the buffer is filling up, expand it.  */
      if (buf_alloc < buf_pos * 2)
	{
	  buf_alloc *= 2;
	  buf = (gdb_byte *) xrealloc (buf, buf_alloc);
	}

      QUIT;
    }
}

/* Read OBJECT/ANNEX using OPS.  Store the result in *BUF_P and return
   the size of the transferred data.  See the declaration in "target.h"
   function for more information about the return value.  */

LONGEST
target_read_alloc (struct target_ops *ops, enum target_object object,
		   const char *annex, gdb_byte **buf_p)
{
  return target_read_alloc_1 (ops, object, annex, buf_p, 0);
}

/* Read OBJECT/ANNEX using OPS.  The result is NUL-terminated and
   returned as a string, allocated using xmalloc.  If an error occurs
   or the transfer is unsupported, NULL is returned.  Empty objects
   are returned as allocated but empty strings.  A warning is issued
   if the result contains any embedded NUL bytes.  */

char *
target_read_stralloc (struct target_ops *ops, enum target_object object,
		      const char *annex)
{
  gdb_byte *buffer;
  char *bufstr;
  LONGEST i, transferred;

  transferred = target_read_alloc_1 (ops, object, annex, &buffer, 1);
  bufstr = (char *) buffer;

  if (transferred < 0)
    return NULL;

  if (transferred == 0)
    return xstrdup ("");

  bufstr[transferred] = 0;

  /* Check for embedded NUL bytes; but allow trailing NULs.  */
  for (i = strlen (bufstr); i < transferred; i++)
    if (bufstr[i] != 0)
      {
	warning (_("target object %d, annex %s, "
		   "contained unexpected null characters"),
		 (int) object, annex ? annex : "(none)");
	break;
      }

  return bufstr;
}

/* Memory transfer methods.  */

void
get_target_memory (struct target_ops *ops, CORE_ADDR addr, gdb_byte *buf,
		   LONGEST len)
{
  /* This method is used to read from an alternate, non-current
     target.  This read must bypass the overlay support (as symbols
     don't match this target), and GDB's internal cache (wrong cache
     for this target).  */
  if (target_read (ops, TARGET_OBJECT_RAW_MEMORY, NULL, buf, addr, len)
      != len)
    memory_error (TARGET_XFER_E_IO, addr);
}

ULONGEST
get_target_memory_unsigned (struct target_ops *ops, CORE_ADDR addr,
			    int len, enum bfd_endian byte_order)
{
  gdb_byte buf[sizeof (ULONGEST)];

  gdb_assert (len <= sizeof (buf));
  get_target_memory (ops, addr, buf, len);
  return extract_unsigned_integer (buf, len, byte_order);
}

/* See target.h.  */

int
target_insert_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  if (!may_insert_breakpoints)
    {
      warning (_("May not insert breakpoints"));
      return 1;
    }

  return current_target.to_insert_breakpoint (&current_target,
					      gdbarch, bp_tgt);
}

/* See target.h.  */

int
target_remove_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt,
			  enum remove_bp_reason reason)
{
  /* This is kind of a weird case to handle, but the permission might
     have been changed after breakpoints were inserted - in which case
     we should just take the user literally and assume that any
     breakpoints should be left in place.  */
  if (!may_insert_breakpoints)
    {
      warning (_("May not remove breakpoints"));
      return 1;
    }

  return current_target.to_remove_breakpoint (&current_target,
					      gdbarch, bp_tgt, reason);
}

static void
target_info (char *args, int from_tty)
{
  struct target_ops *t;
  int has_all_mem = 0;

  if (symfile_objfile != NULL)
    printf_unfiltered (_("Symbols from \"%s\".\n"),
		       objfile_name (symfile_objfile));

  for (t = target_stack; t != NULL; t = t->beneath)
    {
      if (!(*t->to_has_memory) (t))
	continue;

      if ((int) (t->to_stratum) <= (int) dummy_stratum)
	continue;
      if (has_all_mem)
	printf_unfiltered (_("\tWhile running this, "
			     "GDB does not access memory from...\n"));
      printf_unfiltered ("%s:\n", t->to_longname);
      (t->to_files_info) (t);
      has_all_mem = (*t->to_has_all_memory) (t);
    }
}

/* This function is called before any new inferior is created, e.g.
   by running a program, attaching, or connecting to a target.
   It cleans up any state from previous invocations which might
   change between runs.  This is a subset of what target_preopen
   resets (things which might change between targets).  */

void
target_pre_inferior (int from_tty)
{
  /* Clear out solib state.  Otherwise the solib state of the previous
     inferior might have survived and is entirely wrong for the new
     target.  This has been observed on GNU/Linux using glibc 2.3.  How
     to reproduce:

     bash$ ./foo&
     [1] 4711
     bash$ ./foo&
     [1] 4712
     bash$ gdb ./foo
     [...]
     (gdb) attach 4711
     (gdb) detach
     (gdb) attach 4712
     Cannot access memory at address 0xdeadbeef
  */

  /* In some OSs, the shared library list is the same/global/shared
     across inferiors.  If code is shared between processes, so are
     memory regions and features.  */
  if (!gdbarch_has_global_solist (target_gdbarch ()))
    {
      no_shared_libraries (NULL, from_tty);

      invalidate_target_mem_regions ();

      target_clear_description ();
    }

  /* attach_flag may be set if the previous process associated with
     the inferior was attached to.  */
  current_inferior ()->attach_flag = 0;

  current_inferior ()->highest_thread_num = 0;

  agent_capability_invalidate ();
}

/* Callback for iterate_over_inferiors.  Gets rid of the given
   inferior.  */

static int
dispose_inferior (struct inferior *inf, void *args)
{
  struct thread_info *thread;

  thread = any_thread_of_process (inf->pid);
  if (thread)
    {
      switch_to_thread (thread->ptid);

      /* Core inferiors actually should be detached, not killed.  */
      if (target_has_execution)
	target_kill ();
      else
	target_detach (NULL, 0);
    }

  return 0;
}

/* This is to be called by the open routine before it does
   anything.  */

void
target_preopen (int from_tty)
{
  dont_repeat ();

  if (have_inferiors ())
    {
      if (!from_tty
	  || !have_live_inferiors ()
	  || query (_("A program is being debugged already.  Kill it? ")))
	iterate_over_inferiors (dispose_inferior, NULL);
      else
	error (_("Program not killed."));
    }

  /* Calling target_kill may remove the target from the stack.  But if
     it doesn't (which seems like a win for UDI), remove it now.  */
  /* Leave the exec target, though.  The user may be switching from a
     live process to a core of the same program.  */
  pop_all_targets_above (file_stratum);

  target_pre_inferior (from_tty);
}

/* Detach a target after doing deferred register stores.  */

void
target_detach (const char *args, int from_tty)
{
  if (gdbarch_has_global_breakpoints (target_gdbarch ()))
    /* Don't remove global breakpoints here.  They're removed on
       disconnection from the target.  */
    ;
  else
    /* If we're in breakpoints-always-inserted mode, have to remove
       them before detaching.  */
    remove_breakpoints_pid (ptid_get_pid (inferior_ptid));

  prepare_for_detach ();

  current_target.to_detach (&current_target, args, from_tty);
}

void
target_disconnect (const char *args, int from_tty)
{
  /* If we're in breakpoints-always-inserted mode or if breakpoints
     are global across processes, we have to remove them before
     disconnecting.  */
  remove_breakpoints ();

  current_target.to_disconnect (&current_target, args, from_tty);
}

ptid_t
target_wait (ptid_t ptid, struct target_waitstatus *status, int options)
{
  return (current_target.to_wait) (&current_target, ptid, status, options);
}

/* See target.h.  */

ptid_t
default_target_wait (struct target_ops *ops,
		     ptid_t ptid, struct target_waitstatus *status,
		     int options)
{
  status->kind = TARGET_WAITKIND_IGNORE;
  return minus_one_ptid;
}

char *
target_pid_to_str (ptid_t ptid)
{
  return (*current_target.to_pid_to_str) (&current_target, ptid);
}

const char *
target_thread_name (struct thread_info *info)
{
  return current_target.to_thread_name (&current_target, info);
}

void
target_resume (ptid_t ptid, int step, enum gdb_signal signal)
{
  target_dcache_invalidate ();

  current_target.to_resume (&current_target, ptid, step, signal);

  registers_changed_ptid (ptid);
  /* We only set the internal executing state here.  The user/frontend
     running state is set at a higher level.  */
  set_executing (ptid, 1);
  clear_inline_frame_state (ptid);
}

void
target_pass_signals (int numsigs, unsigned char *pass_signals)
{
  (*current_target.to_pass_signals) (&current_target, numsigs, pass_signals);
}

void
target_program_signals (int numsigs, unsigned char *program_signals)
{
  (*current_target.to_program_signals) (&current_target,
					numsigs, program_signals);
}

static int
default_follow_fork (struct target_ops *self, int follow_child,
		     int detach_fork)
{
  /* Some target returned a fork event, but did not know how to follow it.  */
  internal_error (__FILE__, __LINE__,
		  _("could not find a target to follow fork"));
}

/* Look through the list of possible targets for a target that can
   follow forks.  */

int
target_follow_fork (int follow_child, int detach_fork)
{
  return current_target.to_follow_fork (&current_target,
					follow_child, detach_fork);
}

/* Target wrapper for follow exec hook.  */

void
target_follow_exec (struct inferior *inf, char *execd_pathname)
{
  current_target.to_follow_exec (&current_target, inf, execd_pathname);
}

static void
default_mourn_inferior (struct target_ops *self)
{
  internal_error (__FILE__, __LINE__,
		  _("could not find a target to follow mourn inferior"));
}

void
target_mourn_inferior (void)
{
  current_target.to_mourn_inferior (&current_target);

  /* We no longer need to keep handles on any of the object files.
     Make sure to release them to avoid unnecessarily locking any
     of them while we're not actually debugging.  */
  bfd_cache_close_all ();
}

/* Look for a target which can describe architectural features, starting
   from TARGET.  If we find one, return its description.  */

const struct target_desc *
target_read_description (struct target_ops *target)
{
  return target->to_read_description (target);
}

/* This implements a basic search of memory, reading target memory and
   performing the search here (as opposed to performing the search in on the
   target side with, for example, gdbserver).  */

int
simple_search_memory (struct target_ops *ops,
		      CORE_ADDR start_addr, ULONGEST search_space_len,
		      const gdb_byte *pattern, ULONGEST pattern_len,
		      CORE_ADDR *found_addrp)
{
  /* NOTE: also defined in find.c testcase.  */
#define SEARCH_CHUNK_SIZE 16000
  const unsigned chunk_size = SEARCH_CHUNK_SIZE;
  /* Buffer to hold memory contents for searching.  */
  gdb_byte *search_buf;
  unsigned search_buf_size;
  struct cleanup *old_cleanups;

  search_buf_size = chunk_size + pattern_len - 1;

  /* No point in trying to allocate a buffer larger than the search space.  */
  if (search_space_len < search_buf_size)
    search_buf_size = search_space_len;

  search_buf = (gdb_byte *) malloc (search_buf_size);
  if (search_buf == NULL)
    error (_("Unable to allocate memory to perform the search."));
  old_cleanups = make_cleanup (free_current_contents, &search_buf);

  /* Prime the search buffer.  */

  if (target_read (ops, TARGET_OBJECT_MEMORY, NULL,
		   search_buf, start_addr, search_buf_size) != search_buf_size)
    {
      warning (_("Unable to access %s bytes of target "
		 "memory at %s, halting search."),
	       pulongest (search_buf_size), hex_string (start_addr));
      do_cleanups (old_cleanups);
      return -1;
    }

  /* Perform the search.

     The loop is kept simple by allocating [N + pattern-length - 1] bytes.
     When we've scanned N bytes we copy the trailing bytes to the start and
     read in another N bytes.  */

  while (search_space_len >= pattern_len)
    {
      gdb_byte *found_ptr;
      unsigned nr_search_bytes = min (search_space_len, search_buf_size);

      found_ptr = (gdb_byte *) memmem (search_buf, nr_search_bytes,
				       pattern, pattern_len);

      if (found_ptr != NULL)
	{
	  CORE_ADDR found_addr = start_addr + (found_ptr - search_buf);

	  *found_addrp = found_addr;
	  do_cleanups (old_cleanups);
	  return 1;
	}

      /* Not found in this chunk, skip to next chunk.  */

      /* Don't let search_space_len wrap here, it's unsigned.  */
      if (search_space_len >= chunk_size)
	search_space_len -= chunk_size;
      else
	search_space_len = 0;

      if (search_space_len >= pattern_len)
	{
	  unsigned keep_len = search_buf_size - chunk_size;
	  CORE_ADDR read_addr = start_addr + chunk_size + keep_len;
	  int nr_to_read;

	  /* Copy the trailing part of the previous iteration to the front
	     of the buffer for the next iteration.  */
	  gdb_assert (keep_len == pattern_len - 1);
	  memcpy (search_buf, search_buf + chunk_size, keep_len);

	  nr_to_read = min (search_space_len - keep_len, chunk_size);

	  if (target_read (ops, TARGET_OBJECT_MEMORY, NULL,
			   search_buf + keep_len, read_addr,
			   nr_to_read) != nr_to_read)
	    {
	      warning (_("Unable to access %s bytes of target "
			 "memory at %s, halting search."),
		       plongest (nr_to_read),
		       hex_string (read_addr));
	      do_cleanups (old_cleanups);
	      return -1;
	    }

	  start_addr += chunk_size;
	}
    }

  /* Not found.  */

  do_cleanups (old_cleanups);
  return 0;
}

/* Default implementation of memory-searching.  */

static int
default_search_memory (struct target_ops *self,
		       CORE_ADDR start_addr, ULONGEST search_space_len,
		       const gdb_byte *pattern, ULONGEST pattern_len,
		       CORE_ADDR *found_addrp)
{
  /* Start over from the top of the target stack.  */
  return simple_search_memory (current_target.beneath,
			       start_addr, search_space_len,
			       pattern, pattern_len, found_addrp);
}

/* Search SEARCH_SPACE_LEN bytes beginning at START_ADDR for the
   sequence of bytes in PATTERN with length PATTERN_LEN.

   The result is 1 if found, 0 if not found, and -1 if there was an error
   requiring halting of the search (e.g. memory read error).
   If the pattern is found the address is recorded in FOUND_ADDRP.  */

int
target_search_memory (CORE_ADDR start_addr, ULONGEST search_space_len,
		      const gdb_byte *pattern, ULONGEST pattern_len,
		      CORE_ADDR *found_addrp)
{
  return current_target.to_search_memory (&current_target, start_addr,
					  search_space_len,
					  pattern, pattern_len, found_addrp);
}

/* Look through the currently pushed targets.  If none of them will
   be able to restart the currently running process, issue an error
   message.  */

void
target_require_runnable (void)
{
  struct target_ops *t;

  for (t = target_stack; t != NULL; t = t->beneath)
    {
      /* If this target knows how to create a new program, then
	 assume we will still be able to after killing the current
	 one.  Either killing and mourning will not pop T, or else
	 find_default_run_target will find it again.  */
      if (t->to_create_inferior != NULL)
	return;

      /* Do not worry about targets at certain strata that can not
	 create inferiors.  Assume they will be pushed again if
	 necessary, and continue to the process_stratum.  */
      if (t->to_stratum == thread_stratum
	  || t->to_stratum == record_stratum
	  || t->to_stratum == arch_stratum)
	continue;

      error (_("The \"%s\" target does not support \"run\".  "
	       "Try \"help target\" or \"continue\"."),
	     t->to_shortname);
    }

  /* This function is only called if the target is running.  In that
     case there should have been a process_stratum target and it
     should either know how to create inferiors, or not...  */
  internal_error (__FILE__, __LINE__, _("No targets found"));
}

/* Whether GDB is allowed to fall back to the default run target for
   "run", "attach", etc. when no target is connected yet.  */
static int auto_connect_native_target = 1;

static void
show_auto_connect_native_target (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Whether GDB may automatically connect to the "
		      "native target is %s.\n"),
		    value);
}

/* Look through the list of possible targets for a target that can
   execute a run or attach command without any other data.  This is
   used to locate the default process stratum.

   If DO_MESG is not NULL, the result is always valid (error() is
   called for errors); else, return NULL on error.  */

static struct target_ops *
find_default_run_target (char *do_mesg)
{
  struct target_ops *runable = NULL;

  if (auto_connect_native_target)
    {
      struct target_ops *t;
      int count = 0;
      int i;

      for (i = 0; VEC_iterate (target_ops_p, target_structs, i, t); ++i)
	{
	  if (t->to_can_run != delegate_can_run && target_can_run (t))
	    {
	      runable = t;
	      ++count;
	    }
	}

      if (count != 1)
	runable = NULL;
    }

  if (runable == NULL)
    {
      if (do_mesg)
	error (_("Don't know how to %s.  Try \"help target\"."), do_mesg);
      else
	return NULL;
    }

  return runable;
}

/* See target.h.  */

struct target_ops *
find_attach_target (void)
{
  struct target_ops *t;

  /* If a target on the current stack can attach, use it.  */
  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_attach != NULL)
	break;
    }

  /* Otherwise, use the default run target for attaching.  */
  if (t == NULL)
    t = find_default_run_target ("attach");

  return t;
}

/* See target.h.  */

struct target_ops *
find_run_target (void)
{
  struct target_ops *t;

  /* If a target on the current stack can attach, use it.  */
  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_create_inferior != NULL)
	break;
    }

  /* Otherwise, use the default run target.  */
  if (t == NULL)
    t = find_default_run_target ("run");

  return t;
}

/* Implement the "info proc" command.  */

int
target_info_proc (const char *args, enum info_proc_what what)
{
  struct target_ops *t;

  /* If we're already connected to something that can get us OS
     related data, use it.  Otherwise, try using the native
     target.  */
  if (current_target.to_stratum >= process_stratum)
    t = current_target.beneath;
  else
    t = find_default_run_target (NULL);

  for (; t != NULL; t = t->beneath)
    {
      if (t->to_info_proc != NULL)
	{
	  t->to_info_proc (t, args, what);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_info_proc (\"%s\", %d)\n", args, what);

	  return 1;
	}
    }

  return 0;
}

static int
find_default_supports_disable_randomization (struct target_ops *self)
{
  struct target_ops *t;

  t = find_default_run_target (NULL);
  if (t && t->to_supports_disable_randomization)
    return (t->to_supports_disable_randomization) (t);
  return 0;
}

int
target_supports_disable_randomization (void)
{
  struct target_ops *t;

  for (t = &current_target; t != NULL; t = t->beneath)
    if (t->to_supports_disable_randomization)
      return t->to_supports_disable_randomization (t);

  return 0;
}

char *
target_get_osdata (const char *type)
{
  struct target_ops *t;

  /* If we're already connected to something that can get us OS
     related data, use it.  Otherwise, try using the native
     target.  */
  if (current_target.to_stratum >= process_stratum)
    t = current_target.beneath;
  else
    t = find_default_run_target ("get OS data");

  if (!t)
    return NULL;

  return target_read_stralloc (t, TARGET_OBJECT_OSDATA, type);
}

static struct address_space *
default_thread_address_space (struct target_ops *self, ptid_t ptid)
{
  struct inferior *inf;

  /* Fall-back to the "main" address space of the inferior.  */
  inf = find_inferior_ptid (ptid);

  if (inf == NULL || inf->aspace == NULL)
    internal_error (__FILE__, __LINE__,
		    _("Can't determine the current "
		      "address space of thread %s\n"),
		    target_pid_to_str (ptid));

  return inf->aspace;
}

/* Determine the current address space of thread PTID.  */

struct address_space *
target_thread_address_space (ptid_t ptid)
{
  struct address_space *aspace;

  aspace = current_target.to_thread_address_space (&current_target, ptid);
  gdb_assert (aspace != NULL);

  return aspace;
}


/* Target file operations.  */

static struct target_ops *
default_fileio_target (void)
{
  /* If we're already connected to something that can perform
     file I/O, use it. Otherwise, try using the native target.  */
  if (current_target.to_stratum >= process_stratum)
    return current_target.beneath;
  else
    return find_default_run_target ("file I/O");
}

/* File handle for target file operations.  */

typedef struct
{
  /* The target on which this file is open.  */
  struct target_ops *t;

  /* The file descriptor on the target.  */
  int fd;
} fileio_fh_t;

DEF_VEC_O (fileio_fh_t);

/* Vector of currently open file handles.  The value returned by
   target_fileio_open and passed as the FD argument to other
   target_fileio_* functions is an index into this vector.  This
   vector's entries are never freed; instead, files are marked as
   closed, and the handle becomes available for reuse.  */
static VEC (fileio_fh_t) *fileio_fhandles;

/* Macro to check whether a fileio_fh_t represents a closed file.  */
#define is_closed_fileio_fh(fd) ((fd) < 0)

/* Index into fileio_fhandles of the lowest handle that might be
   closed.  This permits handle reuse without searching the whole
   list each time a new file is opened.  */
static int lowest_closed_fd;

/* Acquire a target fileio file descriptor.  */

static int
acquire_fileio_fd (struct target_ops *t, int fd)
{
  fileio_fh_t *fh;

  gdb_assert (!is_closed_fileio_fh (fd));

  /* Search for closed handles to reuse.  */
  for (;
       VEC_iterate (fileio_fh_t, fileio_fhandles,
                    lowest_closed_fd, fh);
       lowest_closed_fd++)
    if (is_closed_fileio_fh (fh->fd))
      break;

  /* Push a new handle if no closed handles were found.  */
  if (lowest_closed_fd == VEC_length (fileio_fh_t, fileio_fhandles))
    fh = VEC_safe_push (fileio_fh_t, fileio_fhandles, NULL);

  /* Fill in the handle.  */
  fh->t = t;
  fh->fd = fd;

  /* Return its index, and start the next lookup at
     the next index.  */
  return lowest_closed_fd++;
}

/* Release a target fileio file descriptor.  */

static void
release_fileio_fd (int fd, fileio_fh_t *fh)
{
  fh->fd = -1;
  lowest_closed_fd = min (lowest_closed_fd, fd);
}

/* Return a pointer to the fileio_fhandle_t corresponding to FD.  */

#define fileio_fd_to_fh(fd) \
  VEC_index (fileio_fh_t, fileio_fhandles, (fd))

/* Helper for target_fileio_open and
   target_fileio_open_warn_if_slow.  */

static int
target_fileio_open_1 (struct inferior *inf, const char *filename,
		      int flags, int mode, int warn_if_slow,
		      int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_open != NULL)
	{
	  int fd = t->to_fileio_open (t, inf, filename, flags, mode,
				      warn_if_slow, target_errno);

	  if (fd < 0)
	    fd = -1;
	  else
	    fd = acquire_fileio_fd (t, fd);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_open (%d,%s,0x%x,0%o,%d)"
				" = %d (%d)\n",
				inf == NULL ? 0 : inf->num,
				filename, flags, mode,
				warn_if_slow, fd,
				fd != -1 ? 0 : *target_errno);
	  return fd;
	}
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* See target.h.  */

int
target_fileio_open (struct inferior *inf, const char *filename,
		    int flags, int mode, int *target_errno)
{
  return target_fileio_open_1 (inf, filename, flags, mode, 0,
			       target_errno);
}

/* See target.h.  */

int
target_fileio_open_warn_if_slow (struct inferior *inf,
				 const char *filename,
				 int flags, int mode, int *target_errno)
{
  return target_fileio_open_1 (inf, filename, flags, mode, 1,
			       target_errno);
}

/* See target.h.  */

int
target_fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
		      ULONGEST offset, int *target_errno)
{
  fileio_fh_t *fh = fileio_fd_to_fh (fd);
  int ret = -1;

  if (is_closed_fileio_fh (fh->fd))
    *target_errno = EBADF;
  else
    ret = fh->t->to_fileio_pwrite (fh->t, fh->fd, write_buf,
				   len, offset, target_errno);

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog,
			"target_fileio_pwrite (%d,...,%d,%s) "
			"= %d (%d)\n",
			fd, len, pulongest (offset),
			ret, ret != -1 ? 0 : *target_errno);
  return ret;
}

/* See target.h.  */

int
target_fileio_pread (int fd, gdb_byte *read_buf, int len,
		     ULONGEST offset, int *target_errno)
{
  fileio_fh_t *fh = fileio_fd_to_fh (fd);
  int ret = -1;

  if (is_closed_fileio_fh (fh->fd))
    *target_errno = EBADF;
  else
    ret = fh->t->to_fileio_pread (fh->t, fh->fd, read_buf,
				  len, offset, target_errno);

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog,
			"target_fileio_pread (%d,...,%d,%s) "
			"= %d (%d)\n",
			fd, len, pulongest (offset),
			ret, ret != -1 ? 0 : *target_errno);
  return ret;
}

/* See target.h.  */

int
target_fileio_fstat (int fd, struct stat *sb, int *target_errno)
{
  fileio_fh_t *fh = fileio_fd_to_fh (fd);
  int ret = -1;

  if (is_closed_fileio_fh (fh->fd))
    *target_errno = EBADF;
  else
    ret = fh->t->to_fileio_fstat (fh->t, fh->fd, sb, target_errno);

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog,
			"target_fileio_fstat (%d) = %d (%d)\n",
			fd, ret, ret != -1 ? 0 : *target_errno);
  return ret;
}

/* See target.h.  */

int
target_fileio_close (int fd, int *target_errno)
{
  fileio_fh_t *fh = fileio_fd_to_fh (fd);
  int ret = -1;

  if (is_closed_fileio_fh (fh->fd))
    *target_errno = EBADF;
  else
    {
      ret = fh->t->to_fileio_close (fh->t, fh->fd, target_errno);
      release_fileio_fd (fd, fh);
    }

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog,
			"target_fileio_close (%d) = %d (%d)\n",
			fd, ret, ret != -1 ? 0 : *target_errno);
  return ret;
}

/* See target.h.  */

int
target_fileio_unlink (struct inferior *inf, const char *filename,
		      int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_unlink != NULL)
	{
	  int ret = t->to_fileio_unlink (t, inf, filename,
					 target_errno);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_unlink (%d,%s)"
				" = %d (%d)\n",
				inf == NULL ? 0 : inf->num, filename,
				ret, ret != -1 ? 0 : *target_errno);
	  return ret;
	}
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* See target.h.  */

char *
target_fileio_readlink (struct inferior *inf, const char *filename,
			int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_readlink != NULL)
	{
	  char *ret = t->to_fileio_readlink (t, inf, filename,
					     target_errno);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_readlink (%d,%s)"
				" = %s (%d)\n",
				inf == NULL ? 0 : inf->num,
				filename, ret? ret : "(nil)",
				ret? 0 : *target_errno);
	  return ret;
	}
    }

  *target_errno = FILEIO_ENOSYS;
  return NULL;
}

static void
target_fileio_close_cleanup (void *opaque)
{
  int fd = *(int *) opaque;
  int target_errno;

  target_fileio_close (fd, &target_errno);
}

/* Read target file FILENAME, in the filesystem as seen by INF.  If
   INF is NULL, use the filesystem seen by the debugger (GDB or, for
   remote targets, the remote stub).  Store the result in *BUF_P and
   return the size of the transferred data.  PADDING additional bytes
   are available in *BUF_P.  This is a helper function for
   target_fileio_read_alloc; see the declaration of that function for
   more information.  */

static LONGEST
target_fileio_read_alloc_1 (struct inferior *inf, const char *filename,
			    gdb_byte **buf_p, int padding)
{
  struct cleanup *close_cleanup;
  size_t buf_alloc, buf_pos;
  gdb_byte *buf;
  LONGEST n;
  int fd;
  int target_errno;

  fd = target_fileio_open (inf, filename, FILEIO_O_RDONLY, 0700,
			   &target_errno);
  if (fd == -1)
    return -1;

  close_cleanup = make_cleanup (target_fileio_close_cleanup, &fd);

  /* Start by reading up to 4K at a time.  The target will throttle
     this number down if necessary.  */
  buf_alloc = 4096;
  buf = (gdb_byte *) xmalloc (buf_alloc);
  buf_pos = 0;
  while (1)
    {
      n = target_fileio_pread (fd, &buf[buf_pos],
			       buf_alloc - buf_pos - padding, buf_pos,
			       &target_errno);
      if (n < 0)
	{
	  /* An error occurred.  */
	  do_cleanups (close_cleanup);
	  xfree (buf);
	  return -1;
	}
      else if (n == 0)
	{
	  /* Read all there was.  */
	  do_cleanups (close_cleanup);
	  if (buf_pos == 0)
	    xfree (buf);
	  else
	    *buf_p = buf;
	  return buf_pos;
	}

      buf_pos += n;

      /* If the buffer is filling up, expand it.  */
      if (buf_alloc < buf_pos * 2)
	{
	  buf_alloc *= 2;
	  buf = (gdb_byte *) xrealloc (buf, buf_alloc);
	}

      QUIT;
    }
}

/* See target.h.  */

LONGEST
target_fileio_read_alloc (struct inferior *inf, const char *filename,
			  gdb_byte **buf_p)
{
  return target_fileio_read_alloc_1 (inf, filename, buf_p, 0);
}

/* See target.h.  */

char *
target_fileio_read_stralloc (struct inferior *inf, const char *filename)
{
  gdb_byte *buffer;
  char *bufstr;
  LONGEST i, transferred;

  transferred = target_fileio_read_alloc_1 (inf, filename, &buffer, 1);
  bufstr = (char *) buffer;

  if (transferred < 0)
    return NULL;

  if (transferred == 0)
    return xstrdup ("");

  bufstr[transferred] = 0;

  /* Check for embedded NUL bytes; but allow trailing NULs.  */
  for (i = strlen (bufstr); i < transferred; i++)
    if (bufstr[i] != 0)
      {
	warning (_("target file %s "
		   "contained unexpected null characters"),
		 filename);
	break;
      }

  return bufstr;
}


static int
default_region_ok_for_hw_watchpoint (struct target_ops *self,
				     CORE_ADDR addr, int len)
{
  return (len <= gdbarch_ptr_bit (target_gdbarch ()) / TARGET_CHAR_BIT);
}

static int
default_watchpoint_addr_within_range (struct target_ops *target,
				      CORE_ADDR addr,
				      CORE_ADDR start, int length)
{
  return addr >= start && addr < start + length;
}

static struct gdbarch *
default_thread_architecture (struct target_ops *ops, ptid_t ptid)
{
  return target_gdbarch ();
}

static int
return_zero (struct target_ops *ignore)
{
  return 0;
}

static int
return_zero_has_execution (struct target_ops *ignore, ptid_t ignore2)
{
  return 0;
}

/*
 * Find the next target down the stack from the specified target.
 */

struct target_ops *
find_target_beneath (struct target_ops *t)
{
  return t->beneath;
}

/* See target.h.  */

struct target_ops *
find_target_at (enum strata stratum)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_stratum == stratum)
      return t;

  return NULL;
}



/* See target.h  */

void
target_announce_detach (int from_tty)
{
  pid_t pid;
  char *exec_file;

  if (!from_tty)
    return;

  exec_file = get_exec_file (0);
  if (exec_file == NULL)
    exec_file = "";

  pid = ptid_get_pid (inferior_ptid);
  printf_unfiltered (_("Detaching from program: %s, %s\n"), exec_file,
		     target_pid_to_str (pid_to_ptid (pid)));
  gdb_flush (gdb_stdout);
}

/* The inferior process has died.  Long live the inferior!  */

void
generic_mourn_inferior (void)
{
  ptid_t ptid;

  ptid = inferior_ptid;
  inferior_ptid = null_ptid;

  /* Mark breakpoints uninserted in case something tries to delete a
     breakpoint while we delete the inferior's threads (which would
     fail, since the inferior is long gone).  */
  mark_breakpoints_out ();

  if (!ptid_equal (ptid, null_ptid))
    {
      int pid = ptid_get_pid (ptid);
      exit_inferior (pid);
    }

  /* Note this wipes step-resume breakpoints, so needs to be done
     after exit_inferior, which ends up referencing the step-resume
     breakpoints through clear_thread_inferior_resources.  */
  breakpoint_init_inferior (inf_exited);

  registers_changed ();

  reopen_exec_file ();
  reinit_frame_cache ();

  if (deprecated_detach_hook)
    deprecated_detach_hook ();
}

/* Convert a normal process ID to a string.  Returns the string in a
   static buffer.  */

char *
normal_pid_to_str (ptid_t ptid)
{
  static char buf[32];

  xsnprintf (buf, sizeof buf, "process %d", ptid_get_pid (ptid));
  return buf;
}

static char *
default_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  return normal_pid_to_str (ptid);
}

/* Error-catcher for target_find_memory_regions.  */
static int
dummy_find_memory_regions (struct target_ops *self,
			   find_memory_region_ftype ignore1, void *ignore2)
{
  error (_("Command not implemented for this target."));
  return 0;
}

/* Error-catcher for target_make_corefile_notes.  */
static char *
dummy_make_corefile_notes (struct target_ops *self,
			   bfd *ignore1, int *ignore2)
{
  error (_("Command not implemented for this target."));
  return NULL;
}

/* Set up the handful of non-empty slots needed by the dummy target
   vector.  */

static void
init_dummy_target (void)
{
  dummy_target.to_shortname = "None";
  dummy_target.to_longname = "None";
  dummy_target.to_doc = "";
  dummy_target.to_supports_disable_randomization
    = find_default_supports_disable_randomization;
  dummy_target.to_stratum = dummy_stratum;
  dummy_target.to_has_all_memory = return_zero;
  dummy_target.to_has_memory = return_zero;
  dummy_target.to_has_stack = return_zero;
  dummy_target.to_has_registers = return_zero;
  dummy_target.to_has_execution = return_zero_has_execution;
  dummy_target.to_magic = OPS_MAGIC;

  install_dummy_methods (&dummy_target);
}


void
target_close (struct target_ops *targ)
{
  gdb_assert (!target_is_pushed (targ));

  if (targ->to_xclose != NULL)
    targ->to_xclose (targ);
  else if (targ->to_close != NULL)
    targ->to_close (targ);

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog, "target_close ()\n");
}

int
target_thread_alive (ptid_t ptid)
{
  return current_target.to_thread_alive (&current_target, ptid);
}

void
target_update_thread_list (void)
{
  current_target.to_update_thread_list (&current_target);
}

void
target_stop (ptid_t ptid)
{
  if (!may_stop)
    {
      warning (_("May not interrupt or stop the target, ignoring attempt"));
      return;
    }

  (*current_target.to_stop) (&current_target, ptid);
}

void
target_interrupt (ptid_t ptid)
{
  if (!may_stop)
    {
      warning (_("May not interrupt or stop the target, ignoring attempt"));
      return;
    }

  (*current_target.to_interrupt) (&current_target, ptid);
}

/* See target.h.  */

void
target_pass_ctrlc (void)
{
  (*current_target.to_pass_ctrlc) (&current_target);
}

/* See target.h.  */

void
default_target_pass_ctrlc (struct target_ops *ops)
{
  target_interrupt (inferior_ptid);
}

/* See target/target.h.  */

void
target_stop_and_wait (ptid_t ptid)
{
  struct target_waitstatus status;
  int was_non_stop = non_stop;

  non_stop = 1;
  target_stop (ptid);

  memset (&status, 0, sizeof (status));
  target_wait (ptid, &status, 0);

  non_stop = was_non_stop;
}

/* See target/target.h.  */

void
target_continue_no_signal (ptid_t ptid)
{
  target_resume (ptid, 0, GDB_SIGNAL_0);
}

/* Concatenate ELEM to LIST, a comma separate list, and return the
   result.  The LIST incoming argument is released.  */

static char *
str_comma_list_concat_elem (char *list, const char *elem)
{
  if (list == NULL)
    return xstrdup (elem);
  else
    return reconcat (list, list, ", ", elem, (char *) NULL);
}

/* Helper for target_options_to_string.  If OPT is present in
   TARGET_OPTIONS, append the OPT_STR (string version of OPT) in RET.
   Returns the new resulting string.  OPT is removed from
   TARGET_OPTIONS.  */

static char *
do_option (int *target_options, char *ret,
	   int opt, char *opt_str)
{
  if ((*target_options & opt) != 0)
    {
      ret = str_comma_list_concat_elem (ret, opt_str);
      *target_options &= ~opt;
    }

  return ret;
}

char *
target_options_to_string (int target_options)
{
  char *ret = NULL;

#define DO_TARG_OPTION(OPT) \
  ret = do_option (&target_options, ret, OPT, #OPT)

  DO_TARG_OPTION (TARGET_WNOHANG);

  if (target_options != 0)
    ret = str_comma_list_concat_elem (ret, "unknown???");

  if (ret == NULL)
    ret = xstrdup ("");
  return ret;
}

static void
debug_print_register (const char * func,
		      struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  fprintf_unfiltered (gdb_stdlog, "%s ", func);
  if (regno >= 0 && regno < gdbarch_num_regs (gdbarch)
      && gdbarch_register_name (gdbarch, regno) != NULL
      && gdbarch_register_name (gdbarch, regno)[0] != '\0')
    fprintf_unfiltered (gdb_stdlog, "(%s)",
			gdbarch_register_name (gdbarch, regno));
  else
    fprintf_unfiltered (gdb_stdlog, "(%d)", regno);
  if (regno >= 0 && regno < gdbarch_num_regs (gdbarch))
    {
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      int i, size = register_size (gdbarch, regno);
      gdb_byte buf[MAX_REGISTER_SIZE];

      regcache_raw_collect (regcache, regno, buf);
      fprintf_unfiltered (gdb_stdlog, " = ");
      for (i = 0; i < size; i++)
	{
	  fprintf_unfiltered (gdb_stdlog, "%02x", buf[i]);
	}
      if (size <= sizeof (LONGEST))
	{
	  ULONGEST val = extract_unsigned_integer (buf, size, byte_order);

	  fprintf_unfiltered (gdb_stdlog, " %s %s",
			      core_addr_to_string_nz (val), plongest (val));
	}
    }
  fprintf_unfiltered (gdb_stdlog, "\n");
}

void
target_fetch_registers (struct regcache *regcache, int regno)
{
  current_target.to_fetch_registers (&current_target, regcache, regno);
  if (targetdebug)
    debug_print_register ("target_fetch_registers", regcache, regno);
}

void
target_store_registers (struct regcache *regcache, int regno)
{
  if (!may_write_registers)
    error (_("Writing to registers is not allowed (regno %d)"), regno);

  current_target.to_store_registers (&current_target, regcache, regno);
  if (targetdebug)
    {
      debug_print_register ("target_store_registers", regcache, regno);
    }
}

int
target_core_of_thread (ptid_t ptid)
{
  return current_target.to_core_of_thread (&current_target, ptid);
}

int
simple_verify_memory (struct target_ops *ops,
		      const gdb_byte *data, CORE_ADDR lma, ULONGEST size)
{
  LONGEST total_xfered = 0;

  while (total_xfered < size)
    {
      ULONGEST xfered_len;
      enum target_xfer_status status;
      gdb_byte buf[1024];
      ULONGEST howmuch = min (sizeof (buf), size - total_xfered);

      status = target_xfer_partial (ops, TARGET_OBJECT_MEMORY, NULL,
				    buf, NULL, lma + total_xfered, howmuch,
				    &xfered_len);
      if (status == TARGET_XFER_OK
	  && memcmp (data + total_xfered, buf, xfered_len) == 0)
	{
	  total_xfered += xfered_len;
	  QUIT;
	}
      else
	return 0;
    }
  return 1;
}

/* Default implementation of memory verification.  */

static int
default_verify_memory (struct target_ops *self,
		       const gdb_byte *data, CORE_ADDR memaddr, ULONGEST size)
{
  /* Start over from the top of the target stack.  */
  return simple_verify_memory (current_target.beneath,
			       data, memaddr, size);
}

int
target_verify_memory (const gdb_byte *data, CORE_ADDR memaddr, ULONGEST size)
{
  return current_target.to_verify_memory (&current_target,
					  data, memaddr, size);
}

/* The documentation for this function is in its prototype declaration in
   target.h.  */

int
target_insert_mask_watchpoint (CORE_ADDR addr, CORE_ADDR mask,
			       enum target_hw_bp_type rw)
{
  return current_target.to_insert_mask_watchpoint (&current_target,
						   addr, mask, rw);
}

/* The documentation for this function is in its prototype declaration in
   target.h.  */

int
target_remove_mask_watchpoint (CORE_ADDR addr, CORE_ADDR mask,
			       enum target_hw_bp_type rw)
{
  return current_target.to_remove_mask_watchpoint (&current_target,
						   addr, mask, rw);
}

/* The documentation for this function is in its prototype declaration
   in target.h.  */

int
target_masked_watch_num_registers (CORE_ADDR addr, CORE_ADDR mask)
{
  return current_target.to_masked_watch_num_registers (&current_target,
						       addr, mask);
}

/* The documentation for this function is in its prototype declaration
   in target.h.  */

int
target_ranged_break_num_registers (void)
{
  return current_target.to_ranged_break_num_registers (&current_target);
}

/* See target.h.  */

int
target_supports_btrace (enum btrace_format format)
{
  return current_target.to_supports_btrace (&current_target, format);
}

/* See target.h.  */

struct btrace_target_info *
target_enable_btrace (ptid_t ptid, const struct btrace_config *conf)
{
  return current_target.to_enable_btrace (&current_target, ptid, conf);
}

/* See target.h.  */

void
target_disable_btrace (struct btrace_target_info *btinfo)
{
  current_target.to_disable_btrace (&current_target, btinfo);
}

/* See target.h.  */

void
target_teardown_btrace (struct btrace_target_info *btinfo)
{
  current_target.to_teardown_btrace (&current_target, btinfo);
}

/* See target.h.  */

enum btrace_error
target_read_btrace (struct btrace_data *btrace,
		    struct btrace_target_info *btinfo,
		    enum btrace_read_type type)
{
  return current_target.to_read_btrace (&current_target, btrace, btinfo, type);
}

/* See target.h.  */

const struct btrace_config *
target_btrace_conf (const struct btrace_target_info *btinfo)
{
  return current_target.to_btrace_conf (&current_target, btinfo);
}

/* See target.h.  */

void
target_stop_recording (void)
{
  current_target.to_stop_recording (&current_target);
}

/* See target.h.  */

void
target_save_record (const char *filename)
{
  current_target.to_save_record (&current_target, filename);
}

/* See target.h.  */

int
target_supports_delete_record (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_delete_record != delegate_delete_record
	&& t->to_delete_record != tdefault_delete_record)
      return 1;

  return 0;
}

/* See target.h.  */

void
target_delete_record (void)
{
  current_target.to_delete_record (&current_target);
}

/* See target.h.  */

int
target_record_is_replaying (ptid_t ptid)
{
  return current_target.to_record_is_replaying (&current_target, ptid);
}

/* See target.h.  */

int
target_record_will_replay (ptid_t ptid, int dir)
{
  return current_target.to_record_will_replay (&current_target, ptid, dir);
}

/* See target.h.  */

void
target_record_stop_replaying (void)
{
  current_target.to_record_stop_replaying (&current_target);
}

/* See target.h.  */

void
target_goto_record_begin (void)
{
  current_target.to_goto_record_begin (&current_target);
}

/* See target.h.  */

void
target_goto_record_end (void)
{
  current_target.to_goto_record_end (&current_target);
}

/* See target.h.  */

void
target_goto_record (ULONGEST insn)
{
  current_target.to_goto_record (&current_target, insn);
}

/* See target.h.  */

void
target_insn_history (int size, int flags)
{
  current_target.to_insn_history (&current_target, size, flags);
}

/* See target.h.  */

void
target_insn_history_from (ULONGEST from, int size, int flags)
{
  current_target.to_insn_history_from (&current_target, from, size, flags);
}

/* See target.h.  */

void
target_insn_history_range (ULONGEST begin, ULONGEST end, int flags)
{
  current_target.to_insn_history_range (&current_target, begin, end, flags);
}

/* See target.h.  */

void
target_call_history (int size, int flags)
{
  current_target.to_call_history (&current_target, size, flags);
}

/* See target.h.  */

void
target_call_history_from (ULONGEST begin, int size, int flags)
{
  current_target.to_call_history_from (&current_target, begin, size, flags);
}

/* See target.h.  */

void
target_call_history_range (ULONGEST begin, ULONGEST end, int flags)
{
  current_target.to_call_history_range (&current_target, begin, end, flags);
}

/* See target.h.  */

const struct frame_unwind *
target_get_unwinder (void)
{
  return current_target.to_get_unwinder (&current_target);
}

/* See target.h.  */

const struct frame_unwind *
target_get_tailcall_unwinder (void)
{
  return current_target.to_get_tailcall_unwinder (&current_target);
}

/* See target.h.  */

void
target_prepare_to_generate_core (void)
{
  current_target.to_prepare_to_generate_core (&current_target);
}

/* See target.h.  */

void
target_done_generating_core (void)
{
  current_target.to_done_generating_core (&current_target);
}

static void
setup_target_debug (void)
{
  memcpy (&debug_target, &current_target, sizeof debug_target);

  init_debug_target (&current_target);
}


static char targ_desc[] =
"Names of targets and files being debugged.\nShows the entire \
stack of targets currently in use (including the exec-file,\n\
core-file, and process, if any), as well as the symbol file name.";

static void
default_rcmd (struct target_ops *self, const char *command,
	      struct ui_file *output)
{
  error (_("\"monitor\" command not supported by this target."));
}

static void
do_monitor_command (char *cmd,
		 int from_tty)
{
  target_rcmd (cmd, gdb_stdtarg);
}

/* Print the name of each layers of our target stack.  */

static void
maintenance_print_target_stack (char *cmd, int from_tty)
{
  struct target_ops *t;

  printf_filtered (_("The current target stack is:\n"));

  for (t = target_stack; t != NULL; t = t->beneath)
    {
      printf_filtered ("  - %s (%s)\n", t->to_shortname, t->to_longname);
    }
}

/* See target.h.  */

void
target_async (int enable)
{
  infrun_async (enable);
  current_target.to_async (&current_target, enable);
}

/* See target.h.  */

void
target_thread_events (int enable)
{
  current_target.to_thread_events (&current_target, enable);
}

/* Controls if targets can report that they can/are async.  This is
   just for maintainers to use when debugging gdb.  */
int target_async_permitted = 1;

/* The set command writes to this variable.  If the inferior is
   executing, target_async_permitted is *not* updated.  */
static int target_async_permitted_1 = 1;

static void
maint_set_target_async_command (char *args, int from_tty,
				struct cmd_list_element *c)
{
  if (have_live_inferiors ())
    {
      target_async_permitted_1 = target_async_permitted;
      error (_("Cannot change this setting while the inferior is running."));
    }

  target_async_permitted = target_async_permitted_1;
}

static void
maint_show_target_async_command (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
  fprintf_filtered (file,
		    _("Controlling the inferior in "
		      "asynchronous mode is %s.\n"), value);
}

/* Return true if the target operates in non-stop mode even with "set
   non-stop off".  */

static int
target_always_non_stop_p (void)
{
  return current_target.to_always_non_stop_p (&current_target);
}

/* See target.h.  */

int
target_is_non_stop_p (void)
{
  return (non_stop
	  || target_non_stop_enabled == AUTO_BOOLEAN_TRUE
	  || (target_non_stop_enabled == AUTO_BOOLEAN_AUTO
	      && target_always_non_stop_p ()));
}

/* Controls if targets can report that they always run in non-stop
   mode.  This is just for maintainers to use when debugging gdb.  */
enum auto_boolean target_non_stop_enabled = AUTO_BOOLEAN_AUTO;

/* The set command writes to this variable.  If the inferior is
   executing, target_non_stop_enabled is *not* updated.  */
static enum auto_boolean target_non_stop_enabled_1 = AUTO_BOOLEAN_AUTO;

/* Implementation of "maint set target-non-stop".  */

static void
maint_set_target_non_stop_command (char *args, int from_tty,
				   struct cmd_list_element *c)
{
  if (have_live_inferiors ())
    {
      target_non_stop_enabled_1 = target_non_stop_enabled;
      error (_("Cannot change this setting while the inferior is running."));
    }

  target_non_stop_enabled = target_non_stop_enabled_1;
}

/* Implementation of "maint show target-non-stop".  */

static void
maint_show_target_non_stop_command (struct ui_file *file, int from_tty,
				    struct cmd_list_element *c,
				    const char *value)
{
  if (target_non_stop_enabled == AUTO_BOOLEAN_AUTO)
    fprintf_filtered (file,
		      _("Whether the target is always in non-stop mode "
			"is %s (currently %s).\n"), value,
		      target_always_non_stop_p () ? "on" : "off");
  else
    fprintf_filtered (file,
		      _("Whether the target is always in non-stop mode "
			"is %s.\n"), value);
}

/* Temporary copies of permission settings.  */

static int may_write_registers_1 = 1;
static int may_write_memory_1 = 1;
static int may_insert_breakpoints_1 = 1;
static int may_insert_tracepoints_1 = 1;
static int may_insert_fast_tracepoints_1 = 1;
static int may_stop_1 = 1;

/* Make the user-set values match the real values again.  */

void
update_target_permissions (void)
{
  may_write_registers_1 = may_write_registers;
  may_write_memory_1 = may_write_memory;
  may_insert_breakpoints_1 = may_insert_breakpoints;
  may_insert_tracepoints_1 = may_insert_tracepoints;
  may_insert_fast_tracepoints_1 = may_insert_fast_tracepoints;
  may_stop_1 = may_stop;
}

/* The one function handles (most of) the permission flags in the same
   way.  */

static void
set_target_permissions (char *args, int from_tty,
			struct cmd_list_element *c)
{
  if (target_has_execution)
    {
      update_target_permissions ();
      error (_("Cannot change this setting while the inferior is running."));
    }

  /* Make the real values match the user-changed values.  */
  may_write_registers = may_write_registers_1;
  may_insert_breakpoints = may_insert_breakpoints_1;
  may_insert_tracepoints = may_insert_tracepoints_1;
  may_insert_fast_tracepoints = may_insert_fast_tracepoints_1;
  may_stop = may_stop_1;
  update_observer_mode ();
}

/* Set memory write permission independently of observer mode.  */

static void
set_write_memory_permission (char *args, int from_tty,
			struct cmd_list_element *c)
{
  /* Make the real values match the user-changed values.  */
  may_write_memory = may_write_memory_1;
  update_observer_mode ();
}


void
initialize_targets (void)
{
  init_dummy_target ();
  push_target (&dummy_target);

  add_info ("target", target_info, targ_desc);
  add_info ("files", target_info, targ_desc);

  add_setshow_zuinteger_cmd ("target", class_maintenance, &targetdebug, _("\
Set target debugging."), _("\
Show target debugging."), _("\
When non-zero, target debugging is enabled.  Higher numbers are more\n\
verbose."),
			     set_targetdebug,
			     show_targetdebug,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("trust-readonly-sections", class_support,
			   &trust_readonly, _("\
Set mode for reading from readonly sections."), _("\
Show mode for reading from readonly sections."), _("\
When this mode is on, memory reads from readonly sections (such as .text)\n\
will be read from the object file instead of from the target.  This will\n\
result in significant performance improvement for remote targets."),
			   NULL,
			   show_trust_readonly,
			   &setlist, &showlist);

  add_com ("monitor", class_obscure, do_monitor_command,
	   _("Send a command to the remote monitor (remote targets only)."));

  add_cmd ("target-stack", class_maintenance, maintenance_print_target_stack,
           _("Print the name of each layer of the internal target stack."),
           &maintenanceprintlist);

  add_setshow_boolean_cmd ("target-async", no_class,
			   &target_async_permitted_1, _("\
Set whether gdb controls the inferior in asynchronous mode."), _("\
Show whether gdb controls the inferior in asynchronous mode."), _("\
Tells gdb whether to control the inferior in asynchronous mode."),
			   maint_set_target_async_command,
			   maint_show_target_async_command,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  add_setshow_auto_boolean_cmd ("target-non-stop", no_class,
				&target_non_stop_enabled_1, _("\
Set whether gdb always controls the inferior in non-stop mode."), _("\
Show whether gdb always controls the inferior in non-stop mode."), _("\
Tells gdb whether to control the inferior in non-stop mode."),
			   maint_set_target_non_stop_command,
			   maint_show_target_non_stop_command,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  add_setshow_boolean_cmd ("may-write-registers", class_support,
			   &may_write_registers_1, _("\
Set permission to write into registers."), _("\
Show permission to write into registers."), _("\
When this permission is on, GDB may write into the target's registers.\n\
Otherwise, any sort of write attempt will result in an error."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-write-memory", class_support,
			   &may_write_memory_1, _("\
Set permission to write into target memory."), _("\
Show permission to write into target memory."), _("\
When this permission is on, GDB may write into the target's memory.\n\
Otherwise, any sort of write attempt will result in an error."),
			   set_write_memory_permission, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-insert-breakpoints", class_support,
			   &may_insert_breakpoints_1, _("\
Set permission to insert breakpoints in the target."), _("\
Show permission to insert breakpoints in the target."), _("\
When this permission is on, GDB may insert breakpoints in the program.\n\
Otherwise, any sort of insertion attempt will result in an error."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-insert-tracepoints", class_support,
			   &may_insert_tracepoints_1, _("\
Set permission to insert tracepoints in the target."), _("\
Show permission to insert tracepoints in the target."), _("\
When this permission is on, GDB may insert tracepoints in the program.\n\
Otherwise, any sort of insertion attempt will result in an error."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-insert-fast-tracepoints", class_support,
			   &may_insert_fast_tracepoints_1, _("\
Set permission to insert fast tracepoints in the target."), _("\
Show permission to insert fast tracepoints in the target."), _("\
When this permission is on, GDB may insert fast tracepoints.\n\
Otherwise, any sort of insertion attempt will result in an error."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-interrupt", class_support,
			   &may_stop_1, _("\
Set permission to interrupt or signal the target."), _("\
Show permission to interrupt or signal the target."), _("\
When this permission is on, GDB may interrupt/stop the target's execution.\n\
Otherwise, any attempt to interrupt or stop will be ignored."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("auto-connect-native-target", class_support,
			   &auto_connect_native_target, _("\
Set whether GDB may automatically connect to the native target."), _("\
Show whether GDB may automatically connect to the native target."), _("\
When on, and GDB is not connected to a target yet, GDB\n\
attempts \"run\" and other commands with the native target."),
			   NULL, show_auto_connect_native_target,
			   &setlist, &showlist);
}
