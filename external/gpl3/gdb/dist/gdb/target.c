/* Select target systems and architectures at runtime for GDB.

   Copyright (C) 1990-2014 Free Software Foundation, Inc.

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
#include <errno.h>
#include <string.h>
#include "target.h"
#include "target-dcache.h"
#include "gdbcmd.h"
#include "symtab.h"
#include "inferior.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "dcache.h"
#include <signal.h>
#include "regcache.h"
#include "gdb_assert.h"
#include "gdbcore.h"
#include "exceptions.h"
#include "target-descriptions.h"
#include "gdbthread.h"
#include "solib.h"
#include "exec.h"
#include "inline-frame.h"
#include "tracepoint.h"
#include "gdb/fileio.h"
#include "agent.h"

static void target_info (char *, int);

static void default_terminal_info (const char *, int);

static int default_watchpoint_addr_within_range (struct target_ops *,
						 CORE_ADDR, CORE_ADDR, int);

static int default_region_ok_for_hw_watchpoint (CORE_ADDR, int);

static void tcomplain (void) ATTRIBUTE_NORETURN;

static int nomemory (CORE_ADDR, char *, int, int, struct target_ops *);

static int return_zero (void);

static int return_one (void);

static int return_minus_one (void);

static void *return_null (void);

void target_ignore (void);

static void target_command (char *, int);

static struct target_ops *find_default_run_target (char *);

static target_xfer_partial_ftype default_xfer_partial;

static target_xfer_partial_ftype current_xfer_partial;

static struct gdbarch *default_thread_architecture (struct target_ops *ops,
						    ptid_t ptid);

static void init_dummy_target (void);

static struct target_ops debug_target;

static void debug_to_open (char *, int);

static void debug_to_prepare_to_store (struct regcache *);

static void debug_to_files_info (struct target_ops *);

static int debug_to_insert_breakpoint (struct gdbarch *,
				       struct bp_target_info *);

static int debug_to_remove_breakpoint (struct gdbarch *,
				       struct bp_target_info *);

static int debug_to_can_use_hw_breakpoint (int, int, int);

static int debug_to_insert_hw_breakpoint (struct gdbarch *,
					  struct bp_target_info *);

static int debug_to_remove_hw_breakpoint (struct gdbarch *,
					  struct bp_target_info *);

static int debug_to_insert_watchpoint (CORE_ADDR, int, int,
				       struct expression *);

static int debug_to_remove_watchpoint (CORE_ADDR, int, int,
				       struct expression *);

static int debug_to_stopped_by_watchpoint (void);

static int debug_to_stopped_data_address (struct target_ops *, CORE_ADDR *);

static int debug_to_watchpoint_addr_within_range (struct target_ops *,
						  CORE_ADDR, CORE_ADDR, int);

static int debug_to_region_ok_for_hw_watchpoint (CORE_ADDR, int);

static int debug_to_can_accel_watchpoint_condition (CORE_ADDR, int, int,
						    struct expression *);

static void debug_to_terminal_init (void);

static void debug_to_terminal_inferior (void);

static void debug_to_terminal_ours_for_output (void);

static void debug_to_terminal_save_ours (void);

static void debug_to_terminal_ours (void);

static void debug_to_load (char *, int);

static int debug_to_can_run (void);

static void debug_to_stop (ptid_t);

/* Pointer to array of target architecture structures; the size of the
   array; the current index into the array; the allocated size of the
   array.  */
struct target_ops **target_structs;
unsigned target_struct_size;
unsigned target_struct_allocsize;
#define	DEFAULT_ALLOCSIZE	10

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
  if (t->to_xfer_partial == NULL)
    t->to_xfer_partial = default_xfer_partial;

  if (t->to_has_all_memory == NULL)
    t->to_has_all_memory = (int (*) (struct target_ops *)) return_zero;

  if (t->to_has_memory == NULL)
    t->to_has_memory = (int (*) (struct target_ops *)) return_zero;

  if (t->to_has_stack == NULL)
    t->to_has_stack = (int (*) (struct target_ops *)) return_zero;

  if (t->to_has_registers == NULL)
    t->to_has_registers = (int (*) (struct target_ops *)) return_zero;

  if (t->to_has_execution == NULL)
    t->to_has_execution = (int (*) (struct target_ops *, ptid_t)) return_zero;
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

  if (!target_structs)
    {
      target_struct_allocsize = DEFAULT_ALLOCSIZE;
      target_structs = (struct target_ops **) xmalloc
	(target_struct_allocsize * sizeof (*target_structs));
    }
  if (target_struct_size >= target_struct_allocsize)
    {
      target_struct_allocsize *= 2;
      target_structs = (struct target_ops **)
	xrealloc ((char *) target_structs,
		  target_struct_allocsize * sizeof (*target_structs));
    }
  target_structs[target_struct_size++] = t;

  if (targetlist == NULL)
    add_prefix_cmd ("target", class_run, target_command, _("\
Connect to a target machine or process.\n\
The first argument is the type or protocol of the target machine.\n\
Remaining arguments are interpreted by the target protocol.  For more\n\
information on the arguments for a particular protocol, type\n\
`help target ' followed by the protocol name."),
		    &targetlist, "target ", 0, &cmdlist);
  c = add_cmd (t->to_shortname, no_class, t->to_open, t->to_doc,
	       &targetlist);
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
  c = add_cmd (alias, no_class, t->to_open, t->to_doc, &targetlist);
  alt = xstrprintf ("target %s", t->to_shortname);
  deprecate_cmd (c, alt);
}

/* Stub functions */

void
target_ignore (void)
{
}

void
target_kill (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_kill != NULL)
      {
	if (targetdebug)
	  fprintf_unfiltered (gdb_stdlog, "target_kill ()\n");

        t->to_kill (t);
	return;
      }

  noprocess ();
}

void
target_load (char *arg, int from_tty)
{
  target_dcache_invalidate ();
  (*current_target.to_load) (arg, from_tty);
}

void
target_create_inferior (char *exec_file, char *args,
			char **env, int from_tty)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_create_inferior != NULL)	
	{
	  t->to_create_inferior (t, exec_file, args, env, from_tty);
	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_create_inferior (%s, %s, xxx, %d)\n",
				exec_file, args, from_tty);
	  return;
	}
    }

  internal_error (__FILE__, __LINE__,
		  _("could not find a target to create inferior"));
}

void
target_terminal_inferior (void)
{
  /* A background resume (``run&'') should leave GDB in control of the
     terminal.  Use target_can_async_p, not target_is_async_p, since at
     this point the target is not async yet.  However, if sync_execution
     is not set, we know it will become async prior to resume.  */
  if (target_can_async_p () && !sync_execution)
    return;

  /* If GDB is resuming the inferior in the foreground, install
     inferior's terminal modes.  */
  (*current_target.to_terminal_inferior) ();
}

static int
nomemory (CORE_ADDR memaddr, char *myaddr, int len, int write,
	  struct target_ops *t)
{
  errno = EIO;			/* Can't read/write this location.  */
  return 0;			/* No bytes handled.  */
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
default_terminal_info (const char *args, int from_tty)
{
  printf_unfiltered (_("No saved terminal information.\n"));
}

/* A default implementation for the to_get_ada_task_ptid target method.

   This function builds the PTID by using both LWP and TID as part of
   the PTID lwp and tid elements.  The pid used is the pid of the
   inferior_ptid.  */

static ptid_t
default_get_ada_task_ptid (long lwp, long tid)
{
  return ptid_build (ptid_get_pid (inferior_ptid), lwp, tid);
}

static enum exec_direction_kind
default_execution_direction (void)
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

#define INHERIT(FIELD, TARGET) \
      if (!current_target.FIELD) \
	current_target.FIELD = (TARGET)->FIELD

  for (t = target_stack; t; t = t->beneath)
    {
      INHERIT (to_shortname, t);
      INHERIT (to_longname, t);
      INHERIT (to_doc, t);
      /* Do not inherit to_open.  */
      /* Do not inherit to_close.  */
      /* Do not inherit to_attach.  */
      INHERIT (to_post_attach, t);
      INHERIT (to_attach_no_wait, t);
      /* Do not inherit to_detach.  */
      /* Do not inherit to_disconnect.  */
      /* Do not inherit to_resume.  */
      /* Do not inherit to_wait.  */
      /* Do not inherit to_fetch_registers.  */
      /* Do not inherit to_store_registers.  */
      INHERIT (to_prepare_to_store, t);
      INHERIT (deprecated_xfer_memory, t);
      INHERIT (to_files_info, t);
      INHERIT (to_insert_breakpoint, t);
      INHERIT (to_remove_breakpoint, t);
      INHERIT (to_can_use_hw_breakpoint, t);
      INHERIT (to_insert_hw_breakpoint, t);
      INHERIT (to_remove_hw_breakpoint, t);
      /* Do not inherit to_ranged_break_num_registers.  */
      INHERIT (to_insert_watchpoint, t);
      INHERIT (to_remove_watchpoint, t);
      /* Do not inherit to_insert_mask_watchpoint.  */
      /* Do not inherit to_remove_mask_watchpoint.  */
      INHERIT (to_stopped_data_address, t);
      INHERIT (to_have_steppable_watchpoint, t);
      INHERIT (to_have_continuable_watchpoint, t);
      INHERIT (to_stopped_by_watchpoint, t);
      INHERIT (to_watchpoint_addr_within_range, t);
      INHERIT (to_region_ok_for_hw_watchpoint, t);
      INHERIT (to_can_accel_watchpoint_condition, t);
      /* Do not inherit to_masked_watch_num_registers.  */
      INHERIT (to_terminal_init, t);
      INHERIT (to_terminal_inferior, t);
      INHERIT (to_terminal_ours_for_output, t);
      INHERIT (to_terminal_ours, t);
      INHERIT (to_terminal_save_ours, t);
      INHERIT (to_terminal_info, t);
      /* Do not inherit to_kill.  */
      INHERIT (to_load, t);
      /* Do no inherit to_create_inferior.  */
      INHERIT (to_post_startup_inferior, t);
      INHERIT (to_insert_fork_catchpoint, t);
      INHERIT (to_remove_fork_catchpoint, t);
      INHERIT (to_insert_vfork_catchpoint, t);
      INHERIT (to_remove_vfork_catchpoint, t);
      /* Do not inherit to_follow_fork.  */
      INHERIT (to_insert_exec_catchpoint, t);
      INHERIT (to_remove_exec_catchpoint, t);
      INHERIT (to_set_syscall_catchpoint, t);
      INHERIT (to_has_exited, t);
      /* Do not inherit to_mourn_inferior.  */
      INHERIT (to_can_run, t);
      /* Do not inherit to_pass_signals.  */
      /* Do not inherit to_program_signals.  */
      /* Do not inherit to_thread_alive.  */
      /* Do not inherit to_find_new_threads.  */
      /* Do not inherit to_pid_to_str.  */
      INHERIT (to_extra_thread_info, t);
      INHERIT (to_thread_name, t);
      INHERIT (to_stop, t);
      /* Do not inherit to_xfer_partial.  */
      INHERIT (to_rcmd, t);
      INHERIT (to_pid_to_exec_file, t);
      INHERIT (to_log_command, t);
      INHERIT (to_stratum, t);
      /* Do not inherit to_has_all_memory.  */
      /* Do not inherit to_has_memory.  */
      /* Do not inherit to_has_stack.  */
      /* Do not inherit to_has_registers.  */
      /* Do not inherit to_has_execution.  */
      INHERIT (to_has_thread_control, t);
      INHERIT (to_can_async_p, t);
      INHERIT (to_is_async_p, t);
      INHERIT (to_async, t);
      INHERIT (to_find_memory_regions, t);
      INHERIT (to_make_corefile_notes, t);
      INHERIT (to_get_bookmark, t);
      INHERIT (to_goto_bookmark, t);
      /* Do not inherit to_get_thread_local_address.  */
      INHERIT (to_can_execute_reverse, t);
      INHERIT (to_execution_direction, t);
      INHERIT (to_thread_architecture, t);
      /* Do not inherit to_read_description.  */
      INHERIT (to_get_ada_task_ptid, t);
      /* Do not inherit to_search_memory.  */
      INHERIT (to_supports_multi_process, t);
      INHERIT (to_supports_enable_disable_tracepoint, t);
      INHERIT (to_supports_string_tracing, t);
      INHERIT (to_trace_init, t);
      INHERIT (to_download_tracepoint, t);
      INHERIT (to_can_download_tracepoint, t);
      INHERIT (to_download_trace_state_variable, t);
      INHERIT (to_enable_tracepoint, t);
      INHERIT (to_disable_tracepoint, t);
      INHERIT (to_trace_set_readonly_regions, t);
      INHERIT (to_trace_start, t);
      INHERIT (to_get_trace_status, t);
      INHERIT (to_get_tracepoint_status, t);
      INHERIT (to_trace_stop, t);
      INHERIT (to_trace_find, t);
      INHERIT (to_get_trace_state_variable_value, t);
      INHERIT (to_save_trace_data, t);
      INHERIT (to_upload_tracepoints, t);
      INHERIT (to_upload_trace_state_variables, t);
      INHERIT (to_get_raw_trace_data, t);
      INHERIT (to_get_min_fast_tracepoint_insn_len, t);
      INHERIT (to_set_disconnected_tracing, t);
      INHERIT (to_set_circular_trace_buffer, t);
      INHERIT (to_set_trace_buffer_size, t);
      INHERIT (to_set_trace_notes, t);
      INHERIT (to_get_tib_address, t);
      INHERIT (to_set_permissions, t);
      INHERIT (to_static_tracepoint_marker_at, t);
      INHERIT (to_static_tracepoint_markers_by_strid, t);
      INHERIT (to_traceframe_info, t);
      INHERIT (to_use_agent, t);
      INHERIT (to_can_use_agent, t);
      INHERIT (to_augmented_libraries_svr4_read, t);
      INHERIT (to_magic, t);
      INHERIT (to_supports_evaluation_of_breakpoint_conditions, t);
      INHERIT (to_can_run_breakpoint_commands, t);
      /* Do not inherit to_memory_map.  */
      /* Do not inherit to_flash_erase.  */
      /* Do not inherit to_flash_done.  */
    }
#undef INHERIT

  /* Clean up a target struct so it no longer has any zero pointers in
     it.  Some entries are defaulted to a method that print an error,
     others are hard-wired to a standard recursive default.  */

#define de_fault(field, value) \
  if (!current_target.field)               \
    current_target.field = value

  de_fault (to_open,
	    (void (*) (char *, int))
	    tcomplain);
  de_fault (to_close,
	    (void (*) (void))
	    target_ignore);
  de_fault (to_post_attach,
	    (void (*) (int))
	    target_ignore);
  de_fault (to_prepare_to_store,
	    (void (*) (struct regcache *))
	    noprocess);
  de_fault (deprecated_xfer_memory,
	    (int (*) (CORE_ADDR, gdb_byte *, int, int,
		      struct mem_attrib *, struct target_ops *))
	    nomemory);
  de_fault (to_files_info,
	    (void (*) (struct target_ops *))
	    target_ignore);
  de_fault (to_insert_breakpoint,
	    memory_insert_breakpoint);
  de_fault (to_remove_breakpoint,
	    memory_remove_breakpoint);
  de_fault (to_can_use_hw_breakpoint,
	    (int (*) (int, int, int))
	    return_zero);
  de_fault (to_insert_hw_breakpoint,
	    (int (*) (struct gdbarch *, struct bp_target_info *))
	    return_minus_one);
  de_fault (to_remove_hw_breakpoint,
	    (int (*) (struct gdbarch *, struct bp_target_info *))
	    return_minus_one);
  de_fault (to_insert_watchpoint,
	    (int (*) (CORE_ADDR, int, int, struct expression *))
	    return_minus_one);
  de_fault (to_remove_watchpoint,
	    (int (*) (CORE_ADDR, int, int, struct expression *))
	    return_minus_one);
  de_fault (to_stopped_by_watchpoint,
	    (int (*) (void))
	    return_zero);
  de_fault (to_stopped_data_address,
	    (int (*) (struct target_ops *, CORE_ADDR *))
	    return_zero);
  de_fault (to_watchpoint_addr_within_range,
	    default_watchpoint_addr_within_range);
  de_fault (to_region_ok_for_hw_watchpoint,
	    default_region_ok_for_hw_watchpoint);
  de_fault (to_can_accel_watchpoint_condition,
            (int (*) (CORE_ADDR, int, int, struct expression *))
            return_zero);
  de_fault (to_terminal_init,
	    (void (*) (void))
	    target_ignore);
  de_fault (to_terminal_inferior,
	    (void (*) (void))
	    target_ignore);
  de_fault (to_terminal_ours_for_output,
	    (void (*) (void))
	    target_ignore);
  de_fault (to_terminal_ours,
	    (void (*) (void))
	    target_ignore);
  de_fault (to_terminal_save_ours,
	    (void (*) (void))
	    target_ignore);
  de_fault (to_terminal_info,
	    default_terminal_info);
  de_fault (to_load,
	    (void (*) (char *, int))
	    tcomplain);
  de_fault (to_post_startup_inferior,
	    (void (*) (ptid_t))
	    target_ignore);
  de_fault (to_insert_fork_catchpoint,
	    (int (*) (int))
	    return_one);
  de_fault (to_remove_fork_catchpoint,
	    (int (*) (int))
	    return_one);
  de_fault (to_insert_vfork_catchpoint,
	    (int (*) (int))
	    return_one);
  de_fault (to_remove_vfork_catchpoint,
	    (int (*) (int))
	    return_one);
  de_fault (to_insert_exec_catchpoint,
	    (int (*) (int))
	    return_one);
  de_fault (to_remove_exec_catchpoint,
	    (int (*) (int))
	    return_one);
  de_fault (to_set_syscall_catchpoint,
	    (int (*) (int, int, int, int, int *))
	    return_one);
  de_fault (to_has_exited,
	    (int (*) (int, int, int *))
	    return_zero);
  de_fault (to_can_run,
	    return_zero);
  de_fault (to_extra_thread_info,
	    (char *(*) (struct thread_info *))
	    return_null);
  de_fault (to_thread_name,
	    (char *(*) (struct thread_info *))
	    return_null);
  de_fault (to_stop,
	    (void (*) (ptid_t))
	    target_ignore);
  current_target.to_xfer_partial = current_xfer_partial;
  de_fault (to_rcmd,
	    (void (*) (char *, struct ui_file *))
	    tcomplain);
  de_fault (to_pid_to_exec_file,
	    (char *(*) (int))
	    return_null);
  de_fault (to_async,
	    (void (*) (void (*) (enum inferior_event_type, void*), void*))
	    tcomplain);
  de_fault (to_thread_architecture,
	    default_thread_architecture);
  current_target.to_read_description = NULL;
  de_fault (to_get_ada_task_ptid,
            (ptid_t (*) (long, long))
            default_get_ada_task_ptid);
  de_fault (to_supports_multi_process,
	    (int (*) (void))
	    return_zero);
  de_fault (to_supports_enable_disable_tracepoint,
	    (int (*) (void))
	    return_zero);
  de_fault (to_supports_string_tracing,
	    (int (*) (void))
	    return_zero);
  de_fault (to_trace_init,
	    (void (*) (void))
	    tcomplain);
  de_fault (to_download_tracepoint,
	    (void (*) (struct bp_location *))
	    tcomplain);
  de_fault (to_can_download_tracepoint,
	    (int (*) (void))
	    return_zero);
  de_fault (to_download_trace_state_variable,
	    (void (*) (struct trace_state_variable *))
	    tcomplain);
  de_fault (to_enable_tracepoint,
	    (void (*) (struct bp_location *))
	    tcomplain);
  de_fault (to_disable_tracepoint,
	    (void (*) (struct bp_location *))
	    tcomplain);
  de_fault (to_trace_set_readonly_regions,
	    (void (*) (void))
	    tcomplain);
  de_fault (to_trace_start,
	    (void (*) (void))
	    tcomplain);
  de_fault (to_get_trace_status,
	    (int (*) (struct trace_status *))
	    return_minus_one);
  de_fault (to_get_tracepoint_status,
	    (void (*) (struct breakpoint *, struct uploaded_tp *))
	    tcomplain);
  de_fault (to_trace_stop,
	    (void (*) (void))
	    tcomplain);
  de_fault (to_trace_find,
	    (int (*) (enum trace_find_type, int, CORE_ADDR, CORE_ADDR, int *))
	    return_minus_one);
  de_fault (to_get_trace_state_variable_value,
	    (int (*) (int, LONGEST *))
	    return_zero);
  de_fault (to_save_trace_data,
	    (int (*) (const char *))
	    tcomplain);
  de_fault (to_upload_tracepoints,
	    (int (*) (struct uploaded_tp **))
	    return_zero);
  de_fault (to_upload_trace_state_variables,
	    (int (*) (struct uploaded_tsv **))
	    return_zero);
  de_fault (to_get_raw_trace_data,
	    (LONGEST (*) (gdb_byte *, ULONGEST, LONGEST))
	    tcomplain);
  de_fault (to_get_min_fast_tracepoint_insn_len,
	    (int (*) (void))
	    return_minus_one);
  de_fault (to_set_disconnected_tracing,
	    (void (*) (int))
	    target_ignore);
  de_fault (to_set_circular_trace_buffer,
	    (void (*) (int))
	    target_ignore);
  de_fault (to_set_trace_buffer_size,
	    (void (*) (LONGEST))
	    target_ignore);
  de_fault (to_set_trace_notes,
	    (int (*) (const char *, const char *, const char *))
	    return_zero);
  de_fault (to_get_tib_address,
	    (int (*) (ptid_t, CORE_ADDR *))
	    tcomplain);
  de_fault (to_set_permissions,
	    (void (*) (void))
	    target_ignore);
  de_fault (to_static_tracepoint_marker_at,
	    (int (*) (CORE_ADDR, struct static_tracepoint_marker *))
	    return_zero);
  de_fault (to_static_tracepoint_markers_by_strid,
	    (VEC(static_tracepoint_marker_p) * (*) (const char *))
	    tcomplain);
  de_fault (to_traceframe_info,
	    (struct traceframe_info * (*) (void))
	    return_null);
  de_fault (to_supports_evaluation_of_breakpoint_conditions,
	    (int (*) (void))
	    return_zero);
  de_fault (to_can_run_breakpoint_commands,
	    (int (*) (void))
	    return_zero);
  de_fault (to_use_agent,
	    (int (*) (int))
	    tcomplain);
  de_fault (to_can_use_agent,
	    (int (*) (void))
	    return_zero);
  de_fault (to_augmented_libraries_svr4_read,
	    (int (*) (void))
	    return_zero);
  de_fault (to_execution_direction, default_execution_direction);

#undef de_fault

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

void
pop_all_targets_above (enum strata above_stratum)
{
  while ((int) (current_target.to_stratum) > (int) above_stratum)
    {
      if (!unpush_target (target_stack))
	{
	  fprintf_unfiltered (gdb_stderr,
			      "pop_all_targets couldn't find target %s\n",
			      target_stack->to_shortname);
	  internal_error (__FILE__, __LINE__,
			  _("failed internal consistency check"));
	  break;
	}
    }
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

  for (cur = &target_stack; (*cur) != NULL; cur = &(*cur)->beneath)
    if (*cur == t)
      return 1;

  return 0;
}

/* Using the objfile specified in OBJFILE, find the address for the
   current thread's thread-local storage with offset OFFSET.  */
CORE_ADDR
target_translate_tls_address (struct objfile *objfile, CORE_ADDR offset)
{
  volatile CORE_ADDR addr = 0;
  struct target_ops *target;

  for (target = current_target.beneath;
       target != NULL;
       target = target->beneath)
    {
      if (target->to_get_thread_local_address != NULL)
	break;
    }

  if (target != NULL
      && gdbarch_fetch_tls_load_module_address_p (target_gdbarch ()))
    {
      ptid_t ptid = inferior_ptid;
      volatile struct gdb_exception ex;

      TRY_CATCH (ex, RETURN_MASK_ALL)
	{
	  CORE_ADDR lm_addr;
	  
	  /* Fetch the load module address for this objfile.  */
	  lm_addr = gdbarch_fetch_tls_load_module_address (target_gdbarch (),
	                                                   objfile);
	  /* If it's 0, throw the appropriate exception.  */
	  if (lm_addr == 0)
	    throw_error (TLS_LOAD_MODULE_NOT_FOUND_ERROR,
			 _("TLS load module not found"));

	  addr = target->to_get_thread_local_address (target, ptid,
						      lm_addr, offset);
	}
      /* If an error occurred, print TLS related messages here.  Otherwise,
         throw the error to some higher catcher.  */
      if (ex.reason < 0)
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
    }
  /* It wouldn't be wrong here to try a gdbarch method, too; finding
     TLS is an ABI-specific thing.  But we don't do that yet.  */
  else
    error (_("Cannot find thread-local variables on this target"));

  return addr;
}

const char *
target_xfer_error_to_string (enum target_xfer_error err)
{
#define CASE(X) case X: return #X
  switch (err)
    {
      CASE(TARGET_XFER_E_IO);
      CASE(TARGET_XFER_E_UNAVAILABLE);
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
  buffer = xmalloc (buffer_allocated);
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
	  buffer = xrealloc (buffer, buffer_allocated);
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
  struct target_ops *t;

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog, "target_get_section_table ()\n");

  for (t = target; t != NULL; t = t->beneath)
    if (t->to_get_section_table != NULL)
      return (*t->to_get_section_table) (t);

  return NULL;
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

/* Read memory from the live target, even if currently inspecting a
   traceframe.  The return is the same as that of target_read.  */

static LONGEST
target_read_live_memory (enum target_object object,
			 ULONGEST memaddr, gdb_byte *myaddr, LONGEST len)
{
  LONGEST ret;
  struct cleanup *cleanup;

  /* Switch momentarily out of tfind mode so to access live memory.
     Note that this must not clear global state, such as the frame
     cache, which must still remain valid for the previous traceframe.
     We may be _building_ the frame cache at this point.  */
  cleanup = make_cleanup_restore_traceframe_number ();
  set_traceframe_number (-1);

  ret = target_read (current_target.beneath, object, NULL,
		     myaddr, memaddr, len);

  do_cleanups (cleanup);
  return ret;
}

/* Using the set of read-only target sections of OPS, read live
   read-only memory.  Note that the actual reads start from the
   top-most target again.

   For interface/parameters/return description see target.h,
   to_xfer_partial.  */

static LONGEST
memory_xfer_live_readonly_partial (struct target_ops *ops,
				   enum target_object object,
				   gdb_byte *readbuf, ULONGEST memaddr,
				   LONGEST len)
{
  struct target_section *secp;
  struct target_section_table *table;

  secp = target_section_by_addr (ops, memaddr);
  if (secp != NULL
      && (bfd_get_section_flags (secp->the_bfd_section->owner,
				 secp->the_bfd_section)
	  & SEC_READONLY))
    {
      struct target_section *p;
      ULONGEST memend = memaddr + len;

      table = target_get_section_table (ops);

      for (p = table->sections; p < table->sections_end; p++)
	{
	  if (memaddr >= p->addr)
	    {
	      if (memend <= p->endaddr)
		{
		  /* Entire transfer is within this section.  */
		  return target_read_live_memory (object, memaddr,
						  readbuf, len);
		}
	      else if (memaddr >= p->endaddr)
		{
		  /* This section ends before the transfer starts.  */
		  continue;
		}
	      else
		{
		  /* This section overlaps the transfer.  Just do half.  */
		  len = p->endaddr - memaddr;
		  return target_read_live_memory (object, memaddr,
						  readbuf, len);
		}
	    }
	}
    }

  return 0;
}

/* Read memory from more than one valid target.  A core file, for
   instance, could have some of memory but delegate other bits to
   the target below it.  So, we must manually try all targets.  */

static LONGEST
raw_memory_xfer_partial (struct target_ops *ops, void *readbuf,
			 const void *writebuf, ULONGEST memaddr, LONGEST len)
{
  LONGEST res;

  do
    {
      res = ops->to_xfer_partial (ops, TARGET_OBJECT_MEMORY, NULL,
				  readbuf, writebuf, memaddr, len);
      if (res > 0)
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
      dcache_update (dcache, res, memaddr, writebuf, len);
    }

  return res;
}

/* Perform a partial memory transfer.
   For docs see target.h, to_xfer_partial.  */

static LONGEST
memory_xfer_partial_1 (struct target_ops *ops, enum target_object object,
		       void *readbuf, const void *writebuf, ULONGEST memaddr,
		       LONGEST len)
{
  LONGEST res;
  int reg_len;
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
						    memaddr, len,
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
						    memaddr, len,
						    table->sections,
						    table->sections_end,
						    NULL);
	}
    }

  /* If reading unavailable memory in the context of traceframes, and
     this address falls within a read-only section, fallback to
     reading from live memory.  */
  if (readbuf != NULL && get_traceframe_number () != -1)
    {
      VEC(mem_range_s) *available;

      /* If we fail to get the set of available memory, then the
	 target does not support querying traceframe info, and so we
	 attempt reading from the traceframe anyway (assuming the
	 target implements the old QTro packet then).  */
      if (traceframe_available_memory (&available, memaddr, len))
	{
	  struct cleanup *old_chain;

	  old_chain = make_cleanup (VEC_cleanup(mem_range_s), &available);

	  if (VEC_empty (mem_range_s, available)
	      || VEC_index (mem_range_s, available, 0)->start != memaddr)
	    {
	      /* Don't read into the traceframe's available
		 memory.  */
	      if (!VEC_empty (mem_range_s, available))
		{
		  LONGEST oldlen = len;

		  len = VEC_index (mem_range_s, available, 0)->start - memaddr;
		  gdb_assert (len <= oldlen);
		}

	      do_cleanups (old_chain);

	      /* This goes through the topmost target again.  */
	      res = memory_xfer_live_readonly_partial (ops, object,
						       readbuf, memaddr, len);
	      if (res > 0)
		return res;

	      /* No use trying further, we know some memory starting
		 at MEMADDR isn't available.  */
	      return TARGET_XFER_E_UNAVAILABLE;
	    }

	  /* Don't try to read more than how much is available, in
	     case the target implements the deprecated QTro packet to
	     cater for older GDBs (the target's knowledge of read-only
	     sections may be outdated by now).  */
	  len = VEC_index (mem_range_s, available, 0)->length;

	  do_cleanups (old_chain);
	}
    }

  /* Try GDB's internal data cache.  */
  region = lookup_mem_region (memaddr);
  /* region->hi == 0 means there's no upper bound.  */
  if (memaddr + len < region->hi || region->hi == 0)
    reg_len = len;
  else
    reg_len = region->hi - memaddr;

  switch (region->attrib.mode)
    {
    case MEM_RO:
      if (writebuf != NULL)
	return -1;
      break;

    case MEM_WO:
      if (readbuf != NULL)
	return -1;
      break;

    case MEM_FLASH:
      /* We only support writing to flash during "load" for now.  */
      if (writebuf != NULL)
	error (_("Writing to flash memory forbidden in this context"));
      break;

    case MEM_NONE:
      return -1;
    }

  if (!ptid_equal (inferior_ptid, null_ptid))
    inf = find_inferior_pid (ptid_get_pid (inferior_ptid));
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
					 reg_len);
    }

  /* If none of those methods found the memory we wanted, fall back
     to a target partial transfer.  Normally a single call to
     to_xfer_partial is enough; if it doesn't recognize an object
     it will call the to_xfer_partial of the next target down.
     But for memory this won't do.  Memory is the only target
     object which can be read from more than one valid target.  */
  res = raw_memory_xfer_partial (ops, readbuf, writebuf, memaddr, reg_len);

  /* If we still haven't got anything, return the last error.  We
     give up.  */
  return res;
}

/* Perform a partial memory transfer.  For docs see target.h,
   to_xfer_partial.  */

static LONGEST
memory_xfer_partial (struct target_ops *ops, enum target_object object,
		     void *readbuf, const void *writebuf, ULONGEST memaddr,
		     LONGEST len)
{
  int res;

  /* Zero length requests are ok and require no work.  */
  if (len == 0)
    return 0;

  /* Fill in READBUF with breakpoint shadows, or WRITEBUF with
     breakpoint insns, thus hiding out from higher layers whether
     there are software breakpoints inserted in the code stream.  */
  if (readbuf != NULL)
    {
      res = memory_xfer_partial_1 (ops, object, readbuf, NULL, memaddr, len);

      if (res > 0 && !show_memory_breakpoints)
	breakpoint_xfer_memory (readbuf, NULL, NULL, memaddr, res);
    }
  else
    {
      void *buf;
      struct cleanup *old_chain;

      /* A large write request is likely to be partially satisfied
	 by memory_xfer_partial_1.  We will continually malloc
	 and free a copy of the entire write request for breakpoint
	 shadow handling even though we only end up writing a small
	 subset of it.  Cap writes to 4KB to mitigate this.  */
      len = min (4096, len);

      buf = xmalloc (len);
      old_chain = make_cleanup (xfree, buf);
      memcpy (buf, writebuf, len);

      breakpoint_xfer_memory (NULL, buf, writebuf, memaddr, len);
      res = memory_xfer_partial_1 (ops, object, NULL, buf, memaddr, len);

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

LONGEST
target_xfer_partial (struct target_ops *ops,
		     enum target_object object, const char *annex,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST offset, LONGEST len)
{
  LONGEST retval;

  gdb_assert (ops->to_xfer_partial != NULL);

  if (writebuf && !may_write_memory)
    error (_("Writing to memory is not allowed (addr %s, len %s)"),
	   core_addr_to_string_nz (offset), plongest (len));

  /* If this is a memory transfer, let the memory-specific code
     have a look at it instead.  Memory transfers are more
     complicated.  */
  if (object == TARGET_OBJECT_MEMORY || object == TARGET_OBJECT_STACK_MEMORY
      || object == TARGET_OBJECT_CODE_MEMORY)
    retval = memory_xfer_partial (ops, object, readbuf,
				  writebuf, offset, len);
  else if (object == TARGET_OBJECT_RAW_MEMORY)
    {
      /* Request the normal memory object from other layers.  */
      retval = raw_memory_xfer_partial (ops, readbuf, writebuf, offset, len);
    }
  else
    retval = ops->to_xfer_partial (ops, object, annex, readbuf,
				   writebuf, offset, len);

  if (targetdebug)
    {
      const unsigned char *myaddr = NULL;

      fprintf_unfiltered (gdb_stdlog,
			  "%s:target_xfer_partial "
			  "(%d, %s, %s, %s, %s, %s) = %s",
			  ops->to_shortname,
			  (int) object,
			  (annex ? annex : "(null)"),
			  host_address_to_string (readbuf),
			  host_address_to_string (writebuf),
			  core_addr_to_string_nz (offset),
			  plongest (len), plongest (retval));

      if (readbuf)
	myaddr = readbuf;
      if (writebuf)
	myaddr = writebuf;
      if (retval > 0 && myaddr != NULL)
	{
	  int i;

	  fputs_unfiltered (", bytes =", gdb_stdlog);
	  for (i = 0; i < retval; i++)
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
  return retval;
}

/* Read LEN bytes of target memory at address MEMADDR, placing the
   results in GDB's memory at MYADDR.  Returns either 0 for success or
   a target_xfer_error value if any error occurs.

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
    return TARGET_XFER_E_IO;
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
    return TARGET_XFER_E_IO;
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
    return TARGET_XFER_E_IO;
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
    return TARGET_XFER_E_IO;
}

/* Write LEN bytes from MYADDR to target memory at address MEMADDR.
   Returns either 0 for success or a target_xfer_error value if any
   error occurs.  If an error occurs, no guarantee is made about how
   much data got written.  Callers that can deal with partial writes
   should call target_write.  */

int
target_write_memory (CORE_ADDR memaddr, const gdb_byte *myaddr, ssize_t len)
{
  /* See comment in target_read_memory about why the request starts at
     current_target.beneath.  */
  if (target_write (current_target.beneath, TARGET_OBJECT_MEMORY, NULL,
		    myaddr, memaddr, len) == len)
    return 0;
  else
    return TARGET_XFER_E_IO;
}

/* Write LEN bytes from MYADDR to target raw memory at address
   MEMADDR.  Returns either 0 for success or a target_xfer_error value
   if any error occurs.  If an error occurs, no guarantee is made
   about how much data got written.  Callers that can deal with
   partial writes should call target_write.  */

int
target_write_raw_memory (CORE_ADDR memaddr, const gdb_byte *myaddr, ssize_t len)
{
  /* See comment in target_read_memory about why the request starts at
     current_target.beneath.  */
  if (target_write (current_target.beneath, TARGET_OBJECT_RAW_MEMORY, NULL,
		    myaddr, memaddr, len) == len)
    return 0;
  else
    return TARGET_XFER_E_IO;
}

/* Fetch the target's memory map.  */

VEC(mem_region_s) *
target_memory_map (void)
{
  VEC(mem_region_s) *result;
  struct mem_region *last_one, *this_one;
  int ix;
  struct target_ops *t;

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog, "target_memory_map ()\n");

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_memory_map != NULL)
      break;

  if (t == NULL)
    return NULL;

  result = t->to_memory_map (t);
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
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_flash_erase != NULL)
      {
	if (targetdebug)
	  fprintf_unfiltered (gdb_stdlog, "target_flash_erase (%s, %s)\n",
			      hex_string (address), phex (length, 0));
	t->to_flash_erase (t, address, length);
	return;
      }

  tcomplain ();
}

void
target_flash_done (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_flash_done != NULL)
      {
	if (targetdebug)
	  fprintf_unfiltered (gdb_stdlog, "target_flash_done\n");
	t->to_flash_done (t);
	return;
      }

  tcomplain ();
}

static void
show_trust_readonly (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Mode for reading from readonly sections is %s.\n"),
		    value);
}

/* More generic transfers.  */

static LONGEST
default_xfer_partial (struct target_ops *ops, enum target_object object,
		      const char *annex, gdb_byte *readbuf,
		      const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  if (object == TARGET_OBJECT_MEMORY
      && ops->deprecated_xfer_memory != NULL)
    /* If available, fall back to the target's
       "deprecated_xfer_memory" method.  */
    {
      int xfered = -1;

      errno = 0;
      if (writebuf != NULL)
	{
	  void *buffer = xmalloc (len);
	  struct cleanup *cleanup = make_cleanup (xfree, buffer);

	  memcpy (buffer, writebuf, len);
	  xfered = ops->deprecated_xfer_memory (offset, buffer, len,
						1/*write*/, NULL, ops);
	  do_cleanups (cleanup);
	}
      if (readbuf != NULL)
	xfered = ops->deprecated_xfer_memory (offset, readbuf, len, 
					      0/*read*/, NULL, ops);
      if (xfered > 0)
	return xfered;
      else if (xfered == 0 && errno == 0)
	/* "deprecated_xfer_memory" uses 0, cross checked against
           ERRNO as one indication of an error.  */
	return 0;
      else
	return -1;
    }
  else if (ops->beneath != NULL)
    return ops->beneath->to_xfer_partial (ops->beneath, object, annex,
					  readbuf, writebuf, offset, len);
  else
    return -1;
}

/* The xfer_partial handler for the topmost target.  Unlike the default,
   it does not need to handle memory specially; it just passes all
   requests down the stack.  */

static LONGEST
current_xfer_partial (struct target_ops *ops, enum target_object object,
		      const char *annex, gdb_byte *readbuf,
		      const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  if (ops->beneath != NULL)
    return ops->beneath->to_xfer_partial (ops->beneath, object, annex,
					  readbuf, writebuf, offset, len);
  else
    return -1;
}

/* Target vector read/write partial wrapper functions.  */

static LONGEST
target_read_partial (struct target_ops *ops,
		     enum target_object object,
		     const char *annex, gdb_byte *buf,
		     ULONGEST offset, LONGEST len)
{
  return target_xfer_partial (ops, object, annex, buf, NULL, offset, len);
}

static LONGEST
target_write_partial (struct target_ops *ops,
		      enum target_object object,
		      const char *annex, const gdb_byte *buf,
		      ULONGEST offset, LONGEST len)
{
  return target_xfer_partial (ops, object, annex, NULL, buf, offset, len);
}

/* Wrappers to perform the full transfer.  */

/* For docs on target_read see target.h.  */

LONGEST
target_read (struct target_ops *ops,
	     enum target_object object,
	     const char *annex, gdb_byte *buf,
	     ULONGEST offset, LONGEST len)
{
  LONGEST xfered = 0;

  while (xfered < len)
    {
      LONGEST xfer = target_read_partial (ops, object, annex,
					  (gdb_byte *) buf + xfered,
					  offset + xfered, len - xfered);

      /* Call an observer, notifying them of the xfer progress?  */
      if (xfer == 0)
	return xfered;
      if (xfer < 0)
	return -1;
      xfered += xfer;
      QUIT;
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
			   ULONGEST begin, ULONGEST end,
			   VEC(memory_read_result_s) **result)
{
  gdb_byte *buf = xmalloc (end - begin);
  ULONGEST current_begin = begin;
  ULONGEST current_end = end;
  int forward;
  memory_read_result_s r;

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
			   buf, begin, 1) == 1)
    {
      forward = 1;
      ++current_begin;
    }
  else if (target_read_partial (ops, TARGET_OBJECT_MEMORY, NULL,
				buf + (end-begin) - 1, end - 1, 1) == 1)
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
      ULONGEST middle = current_begin + (current_end - current_begin)/2;

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
			  buf + (first_half_begin - begin),
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
	     know some part of this half if actually redable.  Go to the next
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
      LONGEST rlen = end - current_end;

      r.data = xmalloc (rlen);
      memcpy (r.data, buf + current_end - begin, rlen);
      r.begin = current_end;
      r.end = end;
      xfree (buf);
    }
  VEC_safe_push(memory_read_result_s, (*result), &r);
}

void
free_memory_read_result_vector (void *x)
{
  VEC(memory_read_result_s) *v = x;
  memory_read_result_s *current;
  int ix;

  for (ix = 0; VEC_iterate (memory_read_result_s, v, ix, current); ++ix)
    {
      xfree (current->data);
    }
  VEC_free (memory_read_result_s, v);
}

VEC(memory_read_result_s) *
read_memory_robust (struct target_ops *ops, ULONGEST offset, LONGEST len)
{
  VEC(memory_read_result_s) *result = 0;

  LONGEST xfered = 0;
  while (xfered < len)
    {
      struct mem_region *region = lookup_mem_region (offset + xfered);
      LONGEST rlen;

      /* If there is no explicit region, a fake one should be created.  */
      gdb_assert (region);

      if (region->hi == 0)
	rlen = len - xfered;
      else
	rlen = region->hi - offset;

      if (region->attrib.mode == MEM_NONE || region->attrib.mode == MEM_WO)
	{
	  /* Cannot read this region.  Note that we can end up here only
	     if the region is explicitly marked inaccessible, or
	     'inaccessible-by-default' is in effect.  */
	  xfered += rlen;
	}
      else
	{
	  LONGEST to_read = min (len - xfered, rlen);
	  gdb_byte *buffer = (gdb_byte *)xmalloc (to_read);

	  LONGEST xfer = target_read (ops, TARGET_OBJECT_MEMORY, NULL,
				      (gdb_byte *) buffer,
				      offset + xfered, to_read);
	  /* Call an observer, notifying them of the xfer progress?  */
	  if (xfer <= 0)
	    {
	      /* Got an error reading full chunk.  See if maybe we can read
		 some subrange.  */
	      xfree (buffer);
	      read_whatever_is_readable (ops, offset + xfered,
					 offset + xfered + to_read, &result);
	      xfered += to_read;
	    }
	  else
	    {
	      struct memory_read_result r;
	      r.data = buffer;
	      r.begin = offset + xfered;
	      r.end = r.begin + xfer;
	      VEC_safe_push (memory_read_result_s, result, &r);
	      xfered += xfer;
	    }
	  QUIT;
	}
    }
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
  LONGEST xfered = 0;

  /* Give the progress callback a chance to set up.  */
  if (progress)
    (*progress) (0, baton);

  while (xfered < len)
    {
      LONGEST xfer = target_write_partial (ops, object, annex,
					   (gdb_byte *) buf + xfered,
					   offset + xfered, len - xfered);

      if (xfer == 0)
	return xfered;
      if (xfer < 0)
	return -1;

      if (progress)
	(*progress) (xfer, baton);

      xfered += xfer;
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
  LONGEST n;

  /* This function does not have a length parameter; it reads the
     entire OBJECT).  Also, it doesn't support objects fetched partly
     from one target and partly from another (in a different stratum,
     e.g. a core file and an executable).  Both reasons make it
     unsuitable for reading memory.  */
  gdb_assert (object != TARGET_OBJECT_MEMORY);

  /* Start by reading up to 4K at a time.  The target will throttle
     this number down if necessary.  */
  buf_alloc = 4096;
  buf = xmalloc (buf_alloc);
  buf_pos = 0;
  while (1)
    {
      n = target_read_partial (ops, object, annex, &buf[buf_pos],
			       buf_pos, buf_alloc - buf_pos - padding);
      if (n < 0)
	{
	  /* An error occurred.  */
	  xfree (buf);
	  return -1;
	}
      else if (n == 0)
	{
	  /* Read all there was.  */
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
	  buf = xrealloc (buf, buf_alloc);
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

int
target_insert_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  if (!may_insert_breakpoints)
    {
      warning (_("May not insert breakpoints"));
      return 1;
    }

  return (*current_target.to_insert_breakpoint) (gdbarch, bp_tgt);
}

int
target_remove_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
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

  return (*current_target.to_remove_breakpoint) (gdbarch, bp_tgt);
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
  struct target_ops* t;
  
  if (gdbarch_has_global_breakpoints (target_gdbarch ()))
    /* Don't remove global breakpoints here.  They're removed on
       disconnection from the target.  */
    ;
  else
    /* If we're in breakpoints-always-inserted mode, have to remove
       them before detaching.  */
    remove_breakpoints_pid (ptid_get_pid (inferior_ptid));

  prepare_for_detach ();

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_detach != NULL)
	{
	  t->to_detach (t, args, from_tty);
	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog, "target_detach (%s, %d)\n",
				args, from_tty);
	  return;
	}
    }

  internal_error (__FILE__, __LINE__, _("could not find a target to detach"));
}

void
target_disconnect (char *args, int from_tty)
{
  struct target_ops *t;

  /* If we're in breakpoints-always-inserted mode or if breakpoints
     are global across processes, we have to remove them before
     disconnecting.  */
  remove_breakpoints ();

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_disconnect != NULL)
	{
	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog, "target_disconnect (%s, %d)\n",
				args, from_tty);
	  t->to_disconnect (t, args, from_tty);
	  return;
	}

  tcomplain ();
}

ptid_t
target_wait (ptid_t ptid, struct target_waitstatus *status, int options)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_wait != NULL)
	{
	  ptid_t retval = (*t->to_wait) (t, ptid, status, options);

	  if (targetdebug)
	    {
	      char *status_string;
	      char *options_string;

	      status_string = target_waitstatus_to_string (status);
	      options_string = target_options_to_string (options);
	      fprintf_unfiltered (gdb_stdlog,
				  "target_wait (%d, status, options={%s})"
				  " = %d,   %s\n",
				  ptid_get_pid (ptid), options_string,
				  ptid_get_pid (retval), status_string);
	      xfree (status_string);
	      xfree (options_string);
	    }

	  return retval;
	}
    }

  noprocess ();
}

char *
target_pid_to_str (ptid_t ptid)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_pid_to_str != NULL)
	return (*t->to_pid_to_str) (t, ptid);
    }

  return normal_pid_to_str (ptid);
}

char *
target_thread_name (struct thread_info *info)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_thread_name != NULL)
	return (*t->to_thread_name) (info);
    }

  return NULL;
}

void
target_resume (ptid_t ptid, int step, enum gdb_signal signal)
{
  struct target_ops *t;

  target_dcache_invalidate ();

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_resume != NULL)
	{
	  t->to_resume (t, ptid, step, signal);
	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog, "target_resume (%d, %s, %s)\n",
				ptid_get_pid (ptid),
				step ? "step" : "continue",
				gdb_signal_to_name (signal));

	  registers_changed_ptid (ptid);
	  set_executing (ptid, 1);
	  set_running (ptid, 1);
	  clear_inline_frame_state (ptid);
	  return;
	}
    }

  noprocess ();
}

void
target_pass_signals (int numsigs, unsigned char *pass_signals)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_pass_signals != NULL)
	{
	  if (targetdebug)
	    {
	      int i;

	      fprintf_unfiltered (gdb_stdlog, "target_pass_signals (%d, {",
				  numsigs);

	      for (i = 0; i < numsigs; i++)
		if (pass_signals[i])
		  fprintf_unfiltered (gdb_stdlog, " %s",
				      gdb_signal_to_name (i));

	      fprintf_unfiltered (gdb_stdlog, " })\n");
	    }

	  (*t->to_pass_signals) (numsigs, pass_signals);
	  return;
	}
    }
}

void
target_program_signals (int numsigs, unsigned char *program_signals)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_program_signals != NULL)
	{
	  if (targetdebug)
	    {
	      int i;

	      fprintf_unfiltered (gdb_stdlog, "target_program_signals (%d, {",
				  numsigs);

	      for (i = 0; i < numsigs; i++)
		if (program_signals[i])
		  fprintf_unfiltered (gdb_stdlog, " %s",
				      gdb_signal_to_name (i));

	      fprintf_unfiltered (gdb_stdlog, " })\n");
	    }

	  (*t->to_program_signals) (numsigs, program_signals);
	  return;
	}
    }
}

/* Look through the list of possible targets for a target that can
   follow forks.  */

int
target_follow_fork (int follow_child, int detach_fork)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_follow_fork != NULL)
	{
	  int retval = t->to_follow_fork (t, follow_child, detach_fork);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_follow_fork (%d, %d) = %d\n",
				follow_child, detach_fork, retval);
	  return retval;
	}
    }

  /* Some target returned a fork event, but did not know how to follow it.  */
  internal_error (__FILE__, __LINE__,
		  _("could not find a target to follow fork"));
}

void
target_mourn_inferior (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_mourn_inferior != NULL)	
	{
	  t->to_mourn_inferior (t);
	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog, "target_mourn_inferior ()\n");

          /* We no longer need to keep handles on any of the object files.
             Make sure to release them to avoid unnecessarily locking any
             of them while we're not actually debugging.  */
          bfd_cache_close_all ();

	  return;
	}
    }

  internal_error (__FILE__, __LINE__,
		  _("could not find a target to follow mourn inferior"));
}

/* Look for a target which can describe architectural features, starting
   from TARGET.  If we find one, return its description.  */

const struct target_desc *
target_read_description (struct target_ops *target)
{
  struct target_ops *t;

  for (t = target; t != NULL; t = t->beneath)
    if (t->to_read_description != NULL)
      {
	const struct target_desc *tdesc;

	tdesc = t->to_read_description (t);
	if (tdesc)
	  return tdesc;
      }

  return NULL;
}

/* The default implementation of to_search_memory.
   This implements a basic search of memory, reading target memory and
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

  search_buf = malloc (search_buf_size);
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

      found_ptr = memmem (search_buf, nr_search_bytes,
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
  struct target_ops *t;
  int found;

  /* We don't use INHERIT to set current_target.to_search_memory,
     so we have to scan the target stack and handle targetdebug
     ourselves.  */

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog, "target_search_memory (%s, ...)\n",
			hex_string (start_addr));

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_search_memory != NULL)
      break;

  if (t != NULL)
    {
      found = t->to_search_memory (t, start_addr, search_space_len,
				   pattern, pattern_len, found_addrp);
    }
  else
    {
      /* If a special version of to_search_memory isn't available, use the
	 simple version.  */
      found = simple_search_memory (current_target.beneath,
				    start_addr, search_space_len,
				    pattern, pattern_len, found_addrp);
    }

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog, "  = %d\n", found);

  return found;
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

      /* Do not worry about thread_stratum targets that can not
	 create inferiors.  Assume they will be pushed again if
	 necessary, and continue to the process_stratum.  */
      if (t->to_stratum == thread_stratum
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

/* Look through the list of possible targets for a target that can
   execute a run or attach command without any other data.  This is
   used to locate the default process stratum.

   If DO_MESG is not NULL, the result is always valid (error() is
   called for errors); else, return NULL on error.  */

static struct target_ops *
find_default_run_target (char *do_mesg)
{
  struct target_ops **t;
  struct target_ops *runable = NULL;
  int count;

  count = 0;

  for (t = target_structs; t < target_structs + target_struct_size;
       ++t)
    {
      if ((*t)->to_can_run && target_can_run (*t))
	{
	  runable = *t;
	  ++count;
	}
    }

  if (count != 1)
    {
      if (do_mesg)
	error (_("Don't know how to %s.  Try \"help target\"."), do_mesg);
      else
	return NULL;
    }

  return runable;
}

void
find_default_attach (struct target_ops *ops, char *args, int from_tty)
{
  struct target_ops *t;

  t = find_default_run_target ("attach");
  (t->to_attach) (t, args, from_tty);
  return;
}

void
find_default_create_inferior (struct target_ops *ops,
			      char *exec_file, char *allargs, char **env,
			      int from_tty)
{
  struct target_ops *t;

  t = find_default_run_target ("run");
  (t->to_create_inferior) (t, exec_file, allargs, env, from_tty);
  return;
}

static int
find_default_can_async_p (void)
{
  struct target_ops *t;

  /* This may be called before the target is pushed on the stack;
     look for the default process stratum.  If there's none, gdb isn't
     configured with a native debugger, and target remote isn't
     connected yet.  */
  t = find_default_run_target (NULL);
  if (t && t->to_can_async_p)
    return (t->to_can_async_p) ();
  return 0;
}

static int
find_default_is_async_p (void)
{
  struct target_ops *t;

  /* This may be called before the target is pushed on the stack;
     look for the default process stratum.  If there's none, gdb isn't
     configured with a native debugger, and target remote isn't
     connected yet.  */
  t = find_default_run_target (NULL);
  if (t && t->to_is_async_p)
    return (t->to_is_async_p) ();
  return 0;
}

static int
find_default_supports_non_stop (void)
{
  struct target_ops *t;

  t = find_default_run_target (NULL);
  if (t && t->to_supports_non_stop)
    return (t->to_supports_non_stop) ();
  return 0;
}

int
target_supports_non_stop (void)
{
  struct target_ops *t;

  for (t = &current_target; t != NULL; t = t->beneath)
    if (t->to_supports_non_stop)
      return t->to_supports_non_stop ();

  return 0;
}

/* Implement the "info proc" command.  */

int
target_info_proc (char *args, enum info_proc_what what)
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
find_default_supports_disable_randomization (void)
{
  struct target_ops *t;

  t = find_default_run_target (NULL);
  if (t && t->to_supports_disable_randomization)
    return (t->to_supports_disable_randomization) ();
  return 0;
}

int
target_supports_disable_randomization (void)
{
  struct target_ops *t;

  for (t = &current_target; t != NULL; t = t->beneath)
    if (t->to_supports_disable_randomization)
      return t->to_supports_disable_randomization ();

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

/* Determine the current address space of thread PTID.  */

struct address_space *
target_thread_address_space (ptid_t ptid)
{
  struct address_space *aspace;
  struct inferior *inf;
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_thread_address_space != NULL)
	{
	  aspace = t->to_thread_address_space (t, ptid);
	  gdb_assert (aspace);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_thread_address_space (%s) = %d\n",
				target_pid_to_str (ptid),
				address_space_num (aspace));
	  return aspace;
	}
    }

  /* Fall-back to the "main" address space of the inferior.  */
  inf = find_inferior_pid (ptid_get_pid (ptid));

  if (inf == NULL || inf->aspace == NULL)
    internal_error (__FILE__, __LINE__,
		    _("Can't determine the current "
		      "address space of thread %s\n"),
		    target_pid_to_str (ptid));

  return inf->aspace;
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

/* Open FILENAME on the target, using FLAGS and MODE.  Return a
   target file descriptor, or -1 if an error occurs (and set
   *TARGET_ERRNO).  */
int
target_fileio_open (const char *filename, int flags, int mode,
		    int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_open != NULL)
	{
	  int fd = t->to_fileio_open (filename, flags, mode, target_errno);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_open (%s,0x%x,0%o) = %d (%d)\n",
				filename, flags, mode,
				fd, fd != -1 ? 0 : *target_errno);
	  return fd;
	}
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* Write up to LEN bytes from WRITE_BUF to FD on the target.
   Return the number of bytes written, or -1 if an error occurs
   (and set *TARGET_ERRNO).  */
int
target_fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
		      ULONGEST offset, int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_pwrite != NULL)
	{
	  int ret = t->to_fileio_pwrite (fd, write_buf, len, offset,
					 target_errno);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_pwrite (%d,...,%d,%s) "
				"= %d (%d)\n",
				fd, len, pulongest (offset),
				ret, ret != -1 ? 0 : *target_errno);
	  return ret;
	}
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* Read up to LEN bytes FD on the target into READ_BUF.
   Return the number of bytes read, or -1 if an error occurs
   (and set *TARGET_ERRNO).  */
int
target_fileio_pread (int fd, gdb_byte *read_buf, int len,
		     ULONGEST offset, int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_pread != NULL)
	{
	  int ret = t->to_fileio_pread (fd, read_buf, len, offset,
					target_errno);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_pread (%d,...,%d,%s) "
				"= %d (%d)\n",
				fd, len, pulongest (offset),
				ret, ret != -1 ? 0 : *target_errno);
	  return ret;
	}
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* Close FD on the target.  Return 0, or -1 if an error occurs
   (and set *TARGET_ERRNO).  */
int
target_fileio_close (int fd, int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_close != NULL)
	{
	  int ret = t->to_fileio_close (fd, target_errno);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_close (%d) = %d (%d)\n",
				fd, ret, ret != -1 ? 0 : *target_errno);
	  return ret;
	}
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* Unlink FILENAME on the target.  Return 0, or -1 if an error
   occurs (and set *TARGET_ERRNO).  */
int
target_fileio_unlink (const char *filename, int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_unlink != NULL)
	{
	  int ret = t->to_fileio_unlink (filename, target_errno);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_unlink (%s) = %d (%d)\n",
				filename, ret, ret != -1 ? 0 : *target_errno);
	  return ret;
	}
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* Read value of symbolic link FILENAME on the target.  Return a
   null-terminated string allocated via xmalloc, or NULL if an error
   occurs (and set *TARGET_ERRNO).  */
char *
target_fileio_readlink (const char *filename, int *target_errno)
{
  struct target_ops *t;

  for (t = default_fileio_target (); t != NULL; t = t->beneath)
    {
      if (t->to_fileio_readlink != NULL)
	{
	  char *ret = t->to_fileio_readlink (filename, target_errno);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_fileio_readlink (%s) = %s (%d)\n",
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

/* Read target file FILENAME.  Store the result in *BUF_P and
   return the size of the transferred data.  PADDING additional bytes are
   available in *BUF_P.  This is a helper function for
   target_fileio_read_alloc; see the declaration of that function for more
   information.  */

static LONGEST
target_fileio_read_alloc_1 (const char *filename,
			    gdb_byte **buf_p, int padding)
{
  struct cleanup *close_cleanup;
  size_t buf_alloc, buf_pos;
  gdb_byte *buf;
  LONGEST n;
  int fd;
  int target_errno;

  fd = target_fileio_open (filename, FILEIO_O_RDONLY, 0700, &target_errno);
  if (fd == -1)
    return -1;

  close_cleanup = make_cleanup (target_fileio_close_cleanup, &fd);

  /* Start by reading up to 4K at a time.  The target will throttle
     this number down if necessary.  */
  buf_alloc = 4096;
  buf = xmalloc (buf_alloc);
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
	  buf = xrealloc (buf, buf_alloc);
	}

      QUIT;
    }
}

/* Read target file FILENAME.  Store the result in *BUF_P and return
   the size of the transferred data.  See the declaration in "target.h"
   function for more information about the return value.  */

LONGEST
target_fileio_read_alloc (const char *filename, gdb_byte **buf_p)
{
  return target_fileio_read_alloc_1 (filename, buf_p, 0);
}

/* Read target file FILENAME.  The result is NUL-terminated and
   returned as a string, allocated using xmalloc.  If an error occurs
   or the transfer is unsupported, NULL is returned.  Empty objects
   are returned as allocated but empty strings.  A warning is issued
   if the result contains any embedded NUL bytes.  */

char *
target_fileio_read_stralloc (const char *filename)
{
  gdb_byte *buffer;
  char *bufstr;
  LONGEST i, transferred;

  transferred = target_fileio_read_alloc_1 (filename, &buffer, 1);
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
default_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
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
return_zero (void)
{
  return 0;
}

static int
return_one (void)
{
  return 1;
}

static int
return_minus_one (void)
{
  return -1;
}

static void *
return_null (void)
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
dummy_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  return normal_pid_to_str (ptid);
}

/* Error-catcher for target_find_memory_regions.  */
static int
dummy_find_memory_regions (find_memory_region_ftype ignore1, void *ignore2)
{
  error (_("Command not implemented for this target."));
  return 0;
}

/* Error-catcher for target_make_corefile_notes.  */
static char *
dummy_make_corefile_notes (bfd *ignore1, int *ignore2)
{
  error (_("Command not implemented for this target."));
  return NULL;
}

/* Error-catcher for target_get_bookmark.  */
static gdb_byte *
dummy_get_bookmark (char *ignore1, int ignore2)
{
  tcomplain ();
  return NULL;
}

/* Error-catcher for target_goto_bookmark.  */
static void
dummy_goto_bookmark (gdb_byte *ignore, int from_tty)
{
  tcomplain ();
}

/* Set up the handful of non-empty slots needed by the dummy target
   vector.  */

static void
init_dummy_target (void)
{
  dummy_target.to_shortname = "None";
  dummy_target.to_longname = "None";
  dummy_target.to_doc = "";
  dummy_target.to_attach = find_default_attach;
  dummy_target.to_detach = 
    (void (*)(struct target_ops *, const char *, int))target_ignore;
  dummy_target.to_create_inferior = find_default_create_inferior;
  dummy_target.to_can_async_p = find_default_can_async_p;
  dummy_target.to_is_async_p = find_default_is_async_p;
  dummy_target.to_supports_non_stop = find_default_supports_non_stop;
  dummy_target.to_supports_disable_randomization
    = find_default_supports_disable_randomization;
  dummy_target.to_pid_to_str = dummy_pid_to_str;
  dummy_target.to_stratum = dummy_stratum;
  dummy_target.to_find_memory_regions = dummy_find_memory_regions;
  dummy_target.to_make_corefile_notes = dummy_make_corefile_notes;
  dummy_target.to_get_bookmark = dummy_get_bookmark;
  dummy_target.to_goto_bookmark = dummy_goto_bookmark;
  dummy_target.to_xfer_partial = default_xfer_partial;
  dummy_target.to_has_all_memory = (int (*) (struct target_ops *)) return_zero;
  dummy_target.to_has_memory = (int (*) (struct target_ops *)) return_zero;
  dummy_target.to_has_stack = (int (*) (struct target_ops *)) return_zero;
  dummy_target.to_has_registers = (int (*) (struct target_ops *)) return_zero;
  dummy_target.to_has_execution
    = (int (*) (struct target_ops *, ptid_t)) return_zero;
  dummy_target.to_stopped_by_watchpoint = return_zero;
  dummy_target.to_stopped_data_address =
    (int (*) (struct target_ops *, CORE_ADDR *)) return_zero;
  dummy_target.to_magic = OPS_MAGIC;
}

static void
debug_to_open (char *args, int from_tty)
{
  debug_target.to_open (args, from_tty);

  fprintf_unfiltered (gdb_stdlog, "target_open (%s, %d)\n", args, from_tty);
}

void
target_close (struct target_ops *targ)
{
  gdb_assert (!target_is_pushed (targ));

  if (targ->to_xclose != NULL)
    targ->to_xclose (targ);
  else if (targ->to_close != NULL)
    targ->to_close ();

  if (targetdebug)
    fprintf_unfiltered (gdb_stdlog, "target_close ()\n");
}

void
target_attach (char *args, int from_tty)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_attach != NULL)	
	{
	  t->to_attach (t, args, from_tty);
	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog, "target_attach (%s, %d)\n",
				args, from_tty);
	  return;
	}
    }

  internal_error (__FILE__, __LINE__,
		  _("could not find a target to attach"));
}

int
target_thread_alive (ptid_t ptid)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_thread_alive != NULL)
	{
	  int retval;

	  retval = t->to_thread_alive (t, ptid);
	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog, "target_thread_alive (%d) = %d\n",
				ptid_get_pid (ptid), retval);

	  return retval;
	}
    }

  return 0;
}

void
target_find_new_threads (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_find_new_threads != NULL)
	{
	  t->to_find_new_threads (t);
	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog, "target_find_new_threads ()\n");

	  return;
	}
    }
}

void
target_stop (ptid_t ptid)
{
  if (!may_stop)
    {
      warning (_("May not interrupt or stop the target, ignoring attempt"));
      return;
    }

  (*current_target.to_stop) (ptid);
}

static void
debug_to_post_attach (int pid)
{
  debug_target.to_post_attach (pid);

  fprintf_unfiltered (gdb_stdlog, "target_post_attach (%d)\n", pid);
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
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_fetch_registers != NULL)
	{
	  t->to_fetch_registers (t, regcache, regno);
	  if (targetdebug)
	    debug_print_register ("target_fetch_registers", regcache, regno);
	  return;
	}
    }
}

void
target_store_registers (struct regcache *regcache, int regno)
{
  struct target_ops *t;

  if (!may_write_registers)
    error (_("Writing to registers is not allowed (regno %d)"), regno);

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_store_registers != NULL)
	{
	  t->to_store_registers (t, regcache, regno);
	  if (targetdebug)
	    {
	      debug_print_register ("target_store_registers", regcache, regno);
	    }
	  return;
	}
    }

  noprocess ();
}

int
target_core_of_thread (ptid_t ptid)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_core_of_thread != NULL)
	{
	  int retval = t->to_core_of_thread (t, ptid);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_core_of_thread (%d) = %d\n",
				ptid_get_pid (ptid), retval);
	  return retval;
	}
    }

  return -1;
}

int
target_verify_memory (const gdb_byte *data, CORE_ADDR memaddr, ULONGEST size)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (t->to_verify_memory != NULL)
	{
	  int retval = t->to_verify_memory (t, data, memaddr, size);

	  if (targetdebug)
	    fprintf_unfiltered (gdb_stdlog,
				"target_verify_memory (%s, %s) = %d\n",
				paddress (target_gdbarch (), memaddr),
				pulongest (size),
				retval);
	  return retval;
	}
    }

  tcomplain ();
}

/* The documentation for this function is in its prototype declaration in
   target.h.  */

int
target_insert_mask_watchpoint (CORE_ADDR addr, CORE_ADDR mask, int rw)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_insert_mask_watchpoint != NULL)
      {
	int ret;

	ret = t->to_insert_mask_watchpoint (t, addr, mask, rw);

	if (targetdebug)
	  fprintf_unfiltered (gdb_stdlog, "\
target_insert_mask_watchpoint (%s, %s, %d) = %d\n",
			      core_addr_to_string (addr),
			      core_addr_to_string (mask), rw, ret);

	return ret;
      }

  return 1;
}

/* The documentation for this function is in its prototype declaration in
   target.h.  */

int
target_remove_mask_watchpoint (CORE_ADDR addr, CORE_ADDR mask, int rw)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_remove_mask_watchpoint != NULL)
      {
	int ret;

	ret = t->to_remove_mask_watchpoint (t, addr, mask, rw);

	if (targetdebug)
	  fprintf_unfiltered (gdb_stdlog, "\
target_remove_mask_watchpoint (%s, %s, %d) = %d\n",
			      core_addr_to_string (addr),
			      core_addr_to_string (mask), rw, ret);

	return ret;
      }

  return 1;
}

/* The documentation for this function is in its prototype declaration
   in target.h.  */

int
target_masked_watch_num_registers (CORE_ADDR addr, CORE_ADDR mask)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_masked_watch_num_registers != NULL)
      return t->to_masked_watch_num_registers (t, addr, mask);

  return -1;
}

/* The documentation for this function is in its prototype declaration
   in target.h.  */

int
target_ranged_break_num_registers (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_ranged_break_num_registers != NULL)
      return t->to_ranged_break_num_registers (t);

  return -1;
}

/* See target.h.  */

int
target_supports_btrace (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_supports_btrace != NULL)
      return t->to_supports_btrace ();

  return 0;
}

/* See target.h.  */

struct btrace_target_info *
target_enable_btrace (ptid_t ptid)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_enable_btrace != NULL)
      return t->to_enable_btrace (ptid);

  tcomplain ();
  return NULL;
}

/* See target.h.  */

void
target_disable_btrace (struct btrace_target_info *btinfo)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_disable_btrace != NULL)
      {
	t->to_disable_btrace (btinfo);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_teardown_btrace (struct btrace_target_info *btinfo)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_teardown_btrace != NULL)
      {
	t->to_teardown_btrace (btinfo);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

VEC (btrace_block_s) *
target_read_btrace (struct btrace_target_info *btinfo,
		    enum btrace_read_type type)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_read_btrace != NULL)
      return t->to_read_btrace (btinfo, type);

  tcomplain ();
  return NULL;
}

/* See target.h.  */

void
target_stop_recording (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_stop_recording != NULL)
      {
	t->to_stop_recording ();
	return;
      }

  /* This is optional.  */
}

/* See target.h.  */

void
target_info_record (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_info_record != NULL)
      {
	t->to_info_record ();
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_save_record (const char *filename)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_save_record != NULL)
      {
	t->to_save_record (filename);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

int
target_supports_delete_record (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_delete_record != NULL)
      return 1;

  return 0;
}

/* See target.h.  */

void
target_delete_record (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_delete_record != NULL)
      {
	t->to_delete_record ();
	return;
      }

  tcomplain ();
}

/* See target.h.  */

int
target_record_is_replaying (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_record_is_replaying != NULL)
	return t->to_record_is_replaying ();

  return 0;
}

/* See target.h.  */

void
target_goto_record_begin (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_goto_record_begin != NULL)
      {
	t->to_goto_record_begin ();
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_goto_record_end (void)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_goto_record_end != NULL)
      {
	t->to_goto_record_end ();
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_goto_record (ULONGEST insn)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_goto_record != NULL)
      {
	t->to_goto_record (insn);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_insn_history (int size, int flags)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_insn_history != NULL)
      {
	t->to_insn_history (size, flags);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_insn_history_from (ULONGEST from, int size, int flags)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_insn_history_from != NULL)
      {
	t->to_insn_history_from (from, size, flags);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_insn_history_range (ULONGEST begin, ULONGEST end, int flags)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_insn_history_range != NULL)
      {
	t->to_insn_history_range (begin, end, flags);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_call_history (int size, int flags)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_call_history != NULL)
      {
	t->to_call_history (size, flags);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_call_history_from (ULONGEST begin, int size, int flags)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_call_history_from != NULL)
      {
	t->to_call_history_from (begin, size, flags);
	return;
      }

  tcomplain ();
}

/* See target.h.  */

void
target_call_history_range (ULONGEST begin, ULONGEST end, int flags)
{
  struct target_ops *t;

  for (t = current_target.beneath; t != NULL; t = t->beneath)
    if (t->to_call_history_range != NULL)
      {
	t->to_call_history_range (begin, end, flags);
	return;
      }

  tcomplain ();
}

static void
debug_to_prepare_to_store (struct regcache *regcache)
{
  debug_target.to_prepare_to_store (regcache);

  fprintf_unfiltered (gdb_stdlog, "target_prepare_to_store ()\n");
}

static int
deprecated_debug_xfer_memory (CORE_ADDR memaddr, bfd_byte *myaddr, int len,
			      int write, struct mem_attrib *attrib,
			      struct target_ops *target)
{
  int retval;

  retval = debug_target.deprecated_xfer_memory (memaddr, myaddr, len, write,
						attrib, target);

  fprintf_unfiltered (gdb_stdlog,
		      "target_xfer_memory (%s, xxx, %d, %s, xxx) = %d",
		      paddress (target_gdbarch (), memaddr), len,
		      write ? "write" : "read", retval);

  if (retval > 0)
    {
      int i;

      fputs_unfiltered (", bytes =", gdb_stdlog);
      for (i = 0; i < retval; i++)
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

  return retval;
}

static void
debug_to_files_info (struct target_ops *target)
{
  debug_target.to_files_info (target);

  fprintf_unfiltered (gdb_stdlog, "target_files_info (xxx)\n");
}

static int
debug_to_insert_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt)
{
  int retval;

  retval = debug_target.to_insert_breakpoint (gdbarch, bp_tgt);

  fprintf_unfiltered (gdb_stdlog,
		      "target_insert_breakpoint (%s, xxx) = %ld\n",
		      core_addr_to_string (bp_tgt->placed_address),
		      (unsigned long) retval);
  return retval;
}

static int
debug_to_remove_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt)
{
  int retval;

  retval = debug_target.to_remove_breakpoint (gdbarch, bp_tgt);

  fprintf_unfiltered (gdb_stdlog,
		      "target_remove_breakpoint (%s, xxx) = %ld\n",
		      core_addr_to_string (bp_tgt->placed_address),
		      (unsigned long) retval);
  return retval;
}

static int
debug_to_can_use_hw_breakpoint (int type, int cnt, int from_tty)
{
  int retval;

  retval = debug_target.to_can_use_hw_breakpoint (type, cnt, from_tty);

  fprintf_unfiltered (gdb_stdlog,
		      "target_can_use_hw_breakpoint (%ld, %ld, %ld) = %ld\n",
		      (unsigned long) type,
		      (unsigned long) cnt,
		      (unsigned long) from_tty,
		      (unsigned long) retval);
  return retval;
}

static int
debug_to_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  CORE_ADDR retval;

  retval = debug_target.to_region_ok_for_hw_watchpoint (addr, len);

  fprintf_unfiltered (gdb_stdlog,
		      "target_region_ok_for_hw_watchpoint (%s, %ld) = %s\n",
		      core_addr_to_string (addr), (unsigned long) len,
		      core_addr_to_string (retval));
  return retval;
}

static int
debug_to_can_accel_watchpoint_condition (CORE_ADDR addr, int len, int rw,
					 struct expression *cond)
{
  int retval;

  retval = debug_target.to_can_accel_watchpoint_condition (addr, len,
							   rw, cond);

  fprintf_unfiltered (gdb_stdlog,
		      "target_can_accel_watchpoint_condition "
		      "(%s, %d, %d, %s) = %ld\n",
		      core_addr_to_string (addr), len, rw,
		      host_address_to_string (cond), (unsigned long) retval);
  return retval;
}

static int
debug_to_stopped_by_watchpoint (void)
{
  int retval;

  retval = debug_target.to_stopped_by_watchpoint ();

  fprintf_unfiltered (gdb_stdlog,
		      "target_stopped_by_watchpoint () = %ld\n",
		      (unsigned long) retval);
  return retval;
}

static int
debug_to_stopped_data_address (struct target_ops *target, CORE_ADDR *addr)
{
  int retval;

  retval = debug_target.to_stopped_data_address (target, addr);

  fprintf_unfiltered (gdb_stdlog,
		      "target_stopped_data_address ([%s]) = %ld\n",
		      core_addr_to_string (*addr),
		      (unsigned long)retval);
  return retval;
}

static int
debug_to_watchpoint_addr_within_range (struct target_ops *target,
				       CORE_ADDR addr,
				       CORE_ADDR start, int length)
{
  int retval;

  retval = debug_target.to_watchpoint_addr_within_range (target, addr,
							 start, length);

  fprintf_filtered (gdb_stdlog,
		    "target_watchpoint_addr_within_range (%s, %s, %d) = %d\n",
		    core_addr_to_string (addr), core_addr_to_string (start),
		    length, retval);
  return retval;
}

static int
debug_to_insert_hw_breakpoint (struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  int retval;

  retval = debug_target.to_insert_hw_breakpoint (gdbarch, bp_tgt);

  fprintf_unfiltered (gdb_stdlog,
		      "target_insert_hw_breakpoint (%s, xxx) = %ld\n",
		      core_addr_to_string (bp_tgt->placed_address),
		      (unsigned long) retval);
  return retval;
}

static int
debug_to_remove_hw_breakpoint (struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  int retval;

  retval = debug_target.to_remove_hw_breakpoint (gdbarch, bp_tgt);

  fprintf_unfiltered (gdb_stdlog,
		      "target_remove_hw_breakpoint (%s, xxx) = %ld\n",
		      core_addr_to_string (bp_tgt->placed_address),
		      (unsigned long) retval);
  return retval;
}

static int
debug_to_insert_watchpoint (CORE_ADDR addr, int len, int type,
			    struct expression *cond)
{
  int retval;

  retval = debug_target.to_insert_watchpoint (addr, len, type, cond);

  fprintf_unfiltered (gdb_stdlog,
		      "target_insert_watchpoint (%s, %d, %d, %s) = %ld\n",
		      core_addr_to_string (addr), len, type,
		      host_address_to_string (cond), (unsigned long) retval);
  return retval;
}

static int
debug_to_remove_watchpoint (CORE_ADDR addr, int len, int type,
			    struct expression *cond)
{
  int retval;

  retval = debug_target.to_remove_watchpoint (addr, len, type, cond);

  fprintf_unfiltered (gdb_stdlog,
		      "target_remove_watchpoint (%s, %d, %d, %s) = %ld\n",
		      core_addr_to_string (addr), len, type,
		      host_address_to_string (cond), (unsigned long) retval);
  return retval;
}

static void
debug_to_terminal_init (void)
{
  debug_target.to_terminal_init ();

  fprintf_unfiltered (gdb_stdlog, "target_terminal_init ()\n");
}

static void
debug_to_terminal_inferior (void)
{
  debug_target.to_terminal_inferior ();

  fprintf_unfiltered (gdb_stdlog, "target_terminal_inferior ()\n");
}

static void
debug_to_terminal_ours_for_output (void)
{
  debug_target.to_terminal_ours_for_output ();

  fprintf_unfiltered (gdb_stdlog, "target_terminal_ours_for_output ()\n");
}

static void
debug_to_terminal_ours (void)
{
  debug_target.to_terminal_ours ();

  fprintf_unfiltered (gdb_stdlog, "target_terminal_ours ()\n");
}

static void
debug_to_terminal_save_ours (void)
{
  debug_target.to_terminal_save_ours ();

  fprintf_unfiltered (gdb_stdlog, "target_terminal_save_ours ()\n");
}

static void
debug_to_terminal_info (const char *arg, int from_tty)
{
  debug_target.to_terminal_info (arg, from_tty);

  fprintf_unfiltered (gdb_stdlog, "target_terminal_info (%s, %d)\n", arg,
		      from_tty);
}

static void
debug_to_load (char *args, int from_tty)
{
  debug_target.to_load (args, from_tty);

  fprintf_unfiltered (gdb_stdlog, "target_load (%s, %d)\n", args, from_tty);
}

static void
debug_to_post_startup_inferior (ptid_t ptid)
{
  debug_target.to_post_startup_inferior (ptid);

  fprintf_unfiltered (gdb_stdlog, "target_post_startup_inferior (%d)\n",
		      ptid_get_pid (ptid));
}

static int
debug_to_insert_fork_catchpoint (int pid)
{
  int retval;

  retval = debug_target.to_insert_fork_catchpoint (pid);

  fprintf_unfiltered (gdb_stdlog, "target_insert_fork_catchpoint (%d) = %d\n",
		      pid, retval);

  return retval;
}

static int
debug_to_remove_fork_catchpoint (int pid)
{
  int retval;

  retval = debug_target.to_remove_fork_catchpoint (pid);

  fprintf_unfiltered (gdb_stdlog, "target_remove_fork_catchpoint (%d) = %d\n",
		      pid, retval);

  return retval;
}

static int
debug_to_insert_vfork_catchpoint (int pid)
{
  int retval;

  retval = debug_target.to_insert_vfork_catchpoint (pid);

  fprintf_unfiltered (gdb_stdlog, "target_insert_vfork_catchpoint (%d) = %d\n",
		      pid, retval);

  return retval;
}

static int
debug_to_remove_vfork_catchpoint (int pid)
{
  int retval;

  retval = debug_target.to_remove_vfork_catchpoint (pid);

  fprintf_unfiltered (gdb_stdlog, "target_remove_vfork_catchpoint (%d) = %d\n",
		      pid, retval);

  return retval;
}

static int
debug_to_insert_exec_catchpoint (int pid)
{
  int retval;

  retval = debug_target.to_insert_exec_catchpoint (pid);

  fprintf_unfiltered (gdb_stdlog, "target_insert_exec_catchpoint (%d) = %d\n",
		      pid, retval);

  return retval;
}

static int
debug_to_remove_exec_catchpoint (int pid)
{
  int retval;

  retval = debug_target.to_remove_exec_catchpoint (pid);

  fprintf_unfiltered (gdb_stdlog, "target_remove_exec_catchpoint (%d) = %d\n",
		      pid, retval);

  return retval;
}

static int
debug_to_has_exited (int pid, int wait_status, int *exit_status)
{
  int has_exited;

  has_exited = debug_target.to_has_exited (pid, wait_status, exit_status);

  fprintf_unfiltered (gdb_stdlog, "target_has_exited (%d, %d, %d) = %d\n",
		      pid, wait_status, *exit_status, has_exited);

  return has_exited;
}

static int
debug_to_can_run (void)
{
  int retval;

  retval = debug_target.to_can_run ();

  fprintf_unfiltered (gdb_stdlog, "target_can_run () = %d\n", retval);

  return retval;
}

static struct gdbarch *
debug_to_thread_architecture (struct target_ops *ops, ptid_t ptid)
{
  struct gdbarch *retval;

  retval = debug_target.to_thread_architecture (ops, ptid);

  fprintf_unfiltered (gdb_stdlog, 
		      "target_thread_architecture (%s) = %s [%s]\n",
		      target_pid_to_str (ptid),
		      host_address_to_string (retval),
		      gdbarch_bfd_arch_info (retval)->printable_name);
  return retval;
}

static void
debug_to_stop (ptid_t ptid)
{
  debug_target.to_stop (ptid);

  fprintf_unfiltered (gdb_stdlog, "target_stop (%s)\n",
		      target_pid_to_str (ptid));
}

static void
debug_to_rcmd (char *command,
	       struct ui_file *outbuf)
{
  debug_target.to_rcmd (command, outbuf);
  fprintf_unfiltered (gdb_stdlog, "target_rcmd (%s, ...)\n", command);
}

static char *
debug_to_pid_to_exec_file (int pid)
{
  char *exec_file;

  exec_file = debug_target.to_pid_to_exec_file (pid);

  fprintf_unfiltered (gdb_stdlog, "target_pid_to_exec_file (%d) = %s\n",
		      pid, exec_file);

  return exec_file;
}

static void
setup_target_debug (void)
{
  memcpy (&debug_target, &current_target, sizeof debug_target);

  current_target.to_open = debug_to_open;
  current_target.to_post_attach = debug_to_post_attach;
  current_target.to_prepare_to_store = debug_to_prepare_to_store;
  current_target.deprecated_xfer_memory = deprecated_debug_xfer_memory;
  current_target.to_files_info = debug_to_files_info;
  current_target.to_insert_breakpoint = debug_to_insert_breakpoint;
  current_target.to_remove_breakpoint = debug_to_remove_breakpoint;
  current_target.to_can_use_hw_breakpoint = debug_to_can_use_hw_breakpoint;
  current_target.to_insert_hw_breakpoint = debug_to_insert_hw_breakpoint;
  current_target.to_remove_hw_breakpoint = debug_to_remove_hw_breakpoint;
  current_target.to_insert_watchpoint = debug_to_insert_watchpoint;
  current_target.to_remove_watchpoint = debug_to_remove_watchpoint;
  current_target.to_stopped_by_watchpoint = debug_to_stopped_by_watchpoint;
  current_target.to_stopped_data_address = debug_to_stopped_data_address;
  current_target.to_watchpoint_addr_within_range
    = debug_to_watchpoint_addr_within_range;
  current_target.to_region_ok_for_hw_watchpoint
    = debug_to_region_ok_for_hw_watchpoint;
  current_target.to_can_accel_watchpoint_condition
    = debug_to_can_accel_watchpoint_condition;
  current_target.to_terminal_init = debug_to_terminal_init;
  current_target.to_terminal_inferior = debug_to_terminal_inferior;
  current_target.to_terminal_ours_for_output
    = debug_to_terminal_ours_for_output;
  current_target.to_terminal_ours = debug_to_terminal_ours;
  current_target.to_terminal_save_ours = debug_to_terminal_save_ours;
  current_target.to_terminal_info = debug_to_terminal_info;
  current_target.to_load = debug_to_load;
  current_target.to_post_startup_inferior = debug_to_post_startup_inferior;
  current_target.to_insert_fork_catchpoint = debug_to_insert_fork_catchpoint;
  current_target.to_remove_fork_catchpoint = debug_to_remove_fork_catchpoint;
  current_target.to_insert_vfork_catchpoint = debug_to_insert_vfork_catchpoint;
  current_target.to_remove_vfork_catchpoint = debug_to_remove_vfork_catchpoint;
  current_target.to_insert_exec_catchpoint = debug_to_insert_exec_catchpoint;
  current_target.to_remove_exec_catchpoint = debug_to_remove_exec_catchpoint;
  current_target.to_has_exited = debug_to_has_exited;
  current_target.to_can_run = debug_to_can_run;
  current_target.to_stop = debug_to_stop;
  current_target.to_rcmd = debug_to_rcmd;
  current_target.to_pid_to_exec_file = debug_to_pid_to_exec_file;
  current_target.to_thread_architecture = debug_to_thread_architecture;
}


static char targ_desc[] =
"Names of targets and files being debugged.\nShows the entire \
stack of targets currently in use (including the exec-file,\n\
core-file, and process, if any), as well as the symbol file name.";

static void
do_monitor_command (char *cmd,
		 int from_tty)
{
  if ((current_target.to_rcmd
       == (void (*) (char *, struct ui_file *)) tcomplain)
      || (current_target.to_rcmd == debug_to_rcmd
	  && (debug_target.to_rcmd
	      == (void (*) (char *, struct ui_file *)) tcomplain)))
    error (_("\"monitor\" command not supported by this target."));
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

/* Controls if async mode is permitted.  */
int target_async_permitted = 0;

/* The set command writes to this variable.  If the inferior is
   executing, target_async_permitted is *not* updated.  */
static int target_async_permitted_1 = 0;

static void
set_target_async_command (char *args, int from_tty,
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
show_target_async_command (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c,
			   const char *value)
{
  fprintf_filtered (file,
		    _("Controlling the inferior in "
		      "asynchronous mode is %s.\n"), value);
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
verbose.  Changes do not take effect until the next \"run\" or \"target\"\n\
command."),
			     NULL,
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
			   set_target_async_command,
			   show_target_async_command,
			   &setlist,
			   &showlist);

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
}
