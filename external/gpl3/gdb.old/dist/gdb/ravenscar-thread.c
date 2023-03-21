/* Ada Ravenscar thread support.

   Copyright (C) 2004-2020 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "gdbthread.h"
#include "ada-lang.h"
#include "target.h"
#include "inferior.h"
#include "command.h"
#include "ravenscar-thread.h"
#include "observable.h"
#include "gdbcmd.h"
#include "top.h"
#include "regcache.h"
#include "objfiles.h"
#include <unordered_map>

/* This module provides support for "Ravenscar" tasks (Ada) when
   debugging on bare-metal targets.

   The typical situation is when debugging a bare-metal target over
   the remote protocol. In that situation, the system does not know
   about high-level concepts such as threads, only about some code
   running on one or more CPUs. And since the remote protocol does not
   provide any handling for CPUs, the de facto standard for handling
   them is to have one thread per CPU, where the thread's ptid has
   its lwp field set to the CPU number (eg: 1 for the first CPU,
   2 for the second one, etc).  This module will make that assumption.

   This module then creates and maintains the list of threads based
   on the list of Ada tasks, with one thread per Ada task. The convention
   is that threads corresponding to the CPUs (see assumption above)
   have a ptid_t of the form (PID, LWP, 0), while threads corresponding
   to our Ada tasks have a ptid_t of the form (PID, 0, TID) where TID
   is the Ada task's ID as extracted from Ada runtime information.

   Switching to a given Ada task (or its underlying thread) is performed
   by fetching the registers of that task from the memory area where
   the registers were saved.  For any of the other operations, the
   operation is performed by first finding the CPU on which the task
   is running, switching to its corresponding ptid, and then performing
   the operation on that ptid using the target beneath us.  */

/* If true, ravenscar task support is enabled.  */
static bool ravenscar_task_support = true;

static const char running_thread_name[] = "__gnat_running_thread_table";

static const char known_tasks_name[] = "system__tasking__debug__known_tasks";
static const char first_task_name[] = "system__tasking__debug__first_task";

static const char ravenscar_runtime_initializer[]
  = "system__bb__threads__initialize";

static const target_info ravenscar_target_info = {
  "ravenscar",
  N_("Ravenscar tasks."),
  N_("Ravenscar tasks support.")
};

struct ravenscar_thread_target final : public target_ops
{
  ravenscar_thread_target ()
    : m_base_ptid (inferior_ptid)
  {
  }

  const target_info &info () const override
  { return ravenscar_target_info; }

  strata stratum () const override { return thread_stratum; }

  ptid_t wait (ptid_t, struct target_waitstatus *, int) override;
  void resume (ptid_t, int, enum gdb_signal) override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  void prepare_to_store (struct regcache *) override;

  bool stopped_by_sw_breakpoint () override;

  bool stopped_by_hw_breakpoint () override;

  bool stopped_by_watchpoint () override;

  bool stopped_data_address (CORE_ADDR *) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  bool thread_alive (ptid_t ptid) override;

  int core_of_thread (ptid_t ptid) override;

  void update_thread_list () override;

  std::string pid_to_str (ptid_t) override;

  ptid_t get_ada_task_ptid (long lwp, long thread) override;

  struct btrace_target_info *enable_btrace (ptid_t ptid,
					    const struct btrace_config *conf)
    override
  {
    ptid = get_base_thread_from_ravenscar_task (ptid);
    return beneath ()->enable_btrace (ptid, conf);
  }

  void mourn_inferior () override;

  void close () override
  {
    delete this;
  }

  thread_info *add_active_thread ();

private:

  /* PTID of the last thread that received an event.
     This can be useful to determine the associated task that received
     the event, to make it the current task.  */
  ptid_t m_base_ptid;

  ptid_t active_task (int cpu);
  bool task_is_currently_active (ptid_t ptid);
  bool runtime_initialized ();
  int get_thread_base_cpu (ptid_t ptid);
  ptid_t get_base_thread_from_ravenscar_task (ptid_t ptid);
  void add_thread (struct ada_task_info *task);

  /* Like switch_to_thread, but uses the base ptid for the thread.  */
  void set_base_thread_from_ravenscar_task (ptid_t ptid)
  {
    process_stratum_target *proc_target
      = as_process_stratum_target (this->beneath ());
    ptid_t underlying = get_base_thread_from_ravenscar_task (ptid);
    switch_to_thread (find_thread_ptid (proc_target, underlying));
  }

  /* This maps a TID to the CPU on which it was running.  This is
     needed because sometimes the runtime will report an active task
     that hasn't yet been put on the list of tasks that is read by
     ada-tasks.c.  */
  std::unordered_map<long, int> m_cpu_map;
};

/* Return true iff PTID corresponds to a ravenscar task.  */

static bool
is_ravenscar_task (ptid_t ptid)
{
  /* By construction, ravenscar tasks have their LWP set to zero.
     Also make sure that the TID is nonzero, as some remotes, when
     asked for the list of threads, will return the first thread
     as having its TID set to zero.  For instance, TSIM version
     2.0.48 for LEON3 sends 'm0' as a reply to the 'qfThreadInfo'
     query, which the remote protocol layer then treats as a thread
     whose TID is 0.  This is obviously not a ravenscar task.  */
  return ptid.lwp () == 0 && ptid.tid () != 0;
}

/* Given PTID, which can be either a ravenscar task or a CPU thread,
   return which CPU that ptid is running on.

   This assume that PTID is a valid ptid_t.  Otherwise, a gdb_assert
   will be triggered.  */

int
ravenscar_thread_target::get_thread_base_cpu (ptid_t ptid)
{
  int base_cpu;

  if (is_ravenscar_task (ptid))
    {
      /* Prefer to not read inferior memory if possible, to avoid
	 reentrancy problems with xfer_partial.  */
      auto iter = m_cpu_map.find (ptid.tid ());

      if (iter != m_cpu_map.end ())
	base_cpu = iter->second;
      else
	{
	  struct ada_task_info *task_info = ada_get_task_info_from_ptid (ptid);

	  gdb_assert (task_info != NULL);
	  base_cpu = task_info->base_cpu;
	}
    }
  else
    {
      /* We assume that the LWP of the PTID is equal to the CPU number.  */
      base_cpu = ptid.lwp ();
    }

  return base_cpu;
}

/* Given a ravenscar task (identified by its ptid_t PTID), return true
   if this task is the currently active task on the cpu that task is
   running on.

   In other words, this function determine which CPU this task is
   currently running on, and then return nonzero if the CPU in question
   is executing the code for that task.  If that's the case, then
   that task's registers are in the CPU bank.  Otherwise, the task
   is currently suspended, and its registers have been saved in memory.  */

bool
ravenscar_thread_target::task_is_currently_active (ptid_t ptid)
{
  ptid_t active_task_ptid = active_task (get_thread_base_cpu (ptid));

  return ptid == active_task_ptid;
}

/* Return the CPU thread (as a ptid_t) on which the given ravenscar
   task is running.

   This is the thread that corresponds to the CPU on which the task
   is running.  */

ptid_t
ravenscar_thread_target::get_base_thread_from_ravenscar_task (ptid_t ptid)
{
  int base_cpu;

  if (!is_ravenscar_task (ptid))
    return ptid;

  base_cpu = get_thread_base_cpu (ptid);
  return ptid_t (ptid.pid (), base_cpu, 0);
}

/* Fetch the ravenscar running thread from target memory, make sure
   there's a corresponding thread in the thread list, and return it.
   If the runtime is not initialized, return NULL.  */

thread_info *
ravenscar_thread_target::add_active_thread ()
{
  process_stratum_target *proc_target
    = as_process_stratum_target (this->beneath ());

  int base_cpu;

  gdb_assert (!is_ravenscar_task (m_base_ptid));
  base_cpu = get_thread_base_cpu (m_base_ptid);

  if (!runtime_initialized ())
    return nullptr;

  /* Make sure we set m_base_ptid before calling active_task
     as the latter relies on it.  */
  ptid_t active_ptid = active_task (base_cpu);
  gdb_assert (active_ptid != null_ptid);

  /* The running thread may not have been added to
     system.tasking.debug's list yet; so ravenscar_update_thread_list
     may not always add it to the thread list.  Add it here.  */
  thread_info *active_thr = find_thread_ptid (proc_target, active_ptid);
  if (active_thr == nullptr)
    {
      active_thr = ::add_thread (proc_target, active_ptid);
      m_cpu_map[active_ptid.tid ()] = base_cpu;
    }
  return active_thr;
}

/* The Ravenscar Runtime exports a symbol which contains the ID of
   the thread that is currently running.  Try to locate that symbol
   and return its associated minimal symbol.
   Return NULL if not found.  */

static struct bound_minimal_symbol
get_running_thread_msymbol ()
{
  struct bound_minimal_symbol msym;

  msym = lookup_minimal_symbol (running_thread_name, NULL, NULL);
  if (!msym.minsym)
    /* Older versions of the GNAT runtime were using a different
       (less ideal) name for the symbol where the active thread ID
       is stored.  If we couldn't find the symbol using the latest
       name, then try the old one.  */
    msym = lookup_minimal_symbol ("running_thread", NULL, NULL);

  return msym;
}

/* Return True if the Ada Ravenscar run-time can be found in the
   application.  */

static bool
has_ravenscar_runtime ()
{
  struct bound_minimal_symbol msym_ravenscar_runtime_initializer
    = lookup_minimal_symbol (ravenscar_runtime_initializer, NULL, NULL);
  struct bound_minimal_symbol msym_known_tasks
    = lookup_minimal_symbol (known_tasks_name, NULL, NULL);
  struct bound_minimal_symbol msym_first_task
    = lookup_minimal_symbol (first_task_name, NULL, NULL);
  struct bound_minimal_symbol msym_running_thread
    = get_running_thread_msymbol ();

  return (msym_ravenscar_runtime_initializer.minsym
	  && (msym_known_tasks.minsym || msym_first_task.minsym)
	  && msym_running_thread.minsym);
}

/* Return True if the Ada Ravenscar run-time can be found in the
   application, and if it has been initialized on target.  */

bool
ravenscar_thread_target::runtime_initialized ()
{
  return active_task (1) != null_ptid;
}

/* Return the ID of the thread that is currently running.
   Return 0 if the ID could not be determined.  */

static CORE_ADDR
get_running_thread_id (int cpu)
{
  struct bound_minimal_symbol object_msym = get_running_thread_msymbol ();
  int object_size;
  int buf_size;
  gdb_byte *buf;
  CORE_ADDR object_addr;
  struct type *builtin_type_void_data_ptr
    = builtin_type (target_gdbarch ())->builtin_data_ptr;

  if (!object_msym.minsym)
    return 0;

  object_size = TYPE_LENGTH (builtin_type_void_data_ptr);
  object_addr = (BMSYMBOL_VALUE_ADDRESS (object_msym)
		 + (cpu - 1) * object_size);
  buf_size = object_size;
  buf = (gdb_byte *) alloca (buf_size);
  read_memory (object_addr, buf, buf_size);
  return extract_typed_address (buf, builtin_type_void_data_ptr);
}

void
ravenscar_thread_target::resume (ptid_t ptid, int step,
				 enum gdb_signal siggnal)
{
  /* If we see a wildcard resume, we simply pass that on.  Otherwise,
     arrange to resume the base ptid.  */
  inferior_ptid = m_base_ptid;
  if (ptid.is_pid ())
    {
      /* We only have one process, so resume all threads of it.  */
      ptid = minus_one_ptid;
    }
  else if (ptid != minus_one_ptid)
    ptid = m_base_ptid;
  beneath ()->resume (ptid, step, siggnal);
}

ptid_t
ravenscar_thread_target::wait (ptid_t ptid,
			       struct target_waitstatus *status,
			       int options)
{
  process_stratum_target *beneath
    = as_process_stratum_target (this->beneath ());
  ptid_t event_ptid;

  if (ptid != minus_one_ptid)
    ptid = m_base_ptid;
  event_ptid = beneath->wait (ptid, status, 0);
  /* Find any new threads that might have been created, and return the
     active thread.

     Only do it if the program is still alive, though.  Otherwise,
     this causes problems when debugging through the remote protocol,
     because we might try switching threads (and thus sending packets)
     after the remote has disconnected.  */
  if (status->kind != TARGET_WAITKIND_EXITED
      && status->kind != TARGET_WAITKIND_SIGNALLED
      && runtime_initialized ())
    {
      m_base_ptid = event_ptid;
      this->update_thread_list ();
      return this->add_active_thread ()->ptid;
    }
  return event_ptid;
}

/* Add the thread associated to the given TASK to the thread list
   (if the thread has already been added, this is a no-op).  */

void
ravenscar_thread_target::add_thread (struct ada_task_info *task)
{
  if (find_thread_ptid (current_inferior (), task->ptid) == NULL)
    {
      ::add_thread (current_inferior ()->process_target (), task->ptid);
      m_cpu_map[task->ptid.tid ()] = task->base_cpu;
    }
}

void
ravenscar_thread_target::update_thread_list ()
{
  /* iterate_over_live_ada_tasks requires that inferior_ptid be set,
     but this isn't always the case in target methods.  So, we ensure
     it here.  */
  scoped_restore save_ptid = make_scoped_restore (&inferior_ptid,
						  m_base_ptid);

  /* Do not clear the thread list before adding the Ada task, to keep
     the thread that the process stratum has included into it
     (m_base_ptid) and the running thread, that may not have been included
     to system.tasking.debug's list yet.  */

  iterate_over_live_ada_tasks ([=] (struct ada_task_info *task)
			       {
				 this->add_thread (task);
			       });
}

ptid_t
ravenscar_thread_target::active_task (int cpu)
{
  CORE_ADDR tid = get_running_thread_id (cpu);

  if (tid == 0)
    return null_ptid;
  else
    return ptid_t (m_base_ptid.pid (), 0, tid);
}

bool
ravenscar_thread_target::thread_alive (ptid_t ptid)
{
  /* Ravenscar tasks are non-terminating.  */
  return true;
}

std::string
ravenscar_thread_target::pid_to_str (ptid_t ptid)
{
  if (!is_ravenscar_task (ptid))
    return beneath ()->pid_to_str (ptid);

  return string_printf ("Ravenscar Thread %#x", (int) ptid.tid ());
}

/* Temporarily set the ptid of a regcache to some other value.  When
   this object is destroyed, the regcache's original ptid is
   restored.  */

class temporarily_change_regcache_ptid
{
public:

  temporarily_change_regcache_ptid (struct regcache *regcache, ptid_t new_ptid)
    : m_regcache (regcache),
      m_save_ptid (regcache->ptid ())
  {
    m_regcache->set_ptid (new_ptid);
  }

  ~temporarily_change_regcache_ptid ()
  {
    m_regcache->set_ptid (m_save_ptid);
  }

private:

  /* The regcache.  */
  struct regcache *m_regcache;
  /* The saved ptid.  */
  ptid_t m_save_ptid;
};

void
ravenscar_thread_target::fetch_registers (struct regcache *regcache, int regnum)
{
  ptid_t ptid = regcache->ptid ();

  if (runtime_initialized () && is_ravenscar_task (ptid))
    {
      if (task_is_currently_active (ptid))
	{
	  ptid_t base = get_base_thread_from_ravenscar_task (ptid);
	  temporarily_change_regcache_ptid changer (regcache, base);
	  beneath ()->fetch_registers (regcache, regnum);
	}
      else
	{
	  struct gdbarch *gdbarch = regcache->arch ();
	  struct ravenscar_arch_ops *arch_ops
	    = gdbarch_ravenscar_ops (gdbarch);

	  arch_ops->fetch_registers (regcache, regnum);
	}
    }
  else
    beneath ()->fetch_registers (regcache, regnum);
}

void
ravenscar_thread_target::store_registers (struct regcache *regcache,
					  int regnum)
{
  ptid_t ptid = regcache->ptid ();

  if (runtime_initialized () && is_ravenscar_task (ptid))
    {
      if (task_is_currently_active (ptid))
	{
	  ptid_t base = get_base_thread_from_ravenscar_task (ptid);
	  temporarily_change_regcache_ptid changer (regcache, base);
	  beneath ()->store_registers (regcache, regnum);
	}
      else
	{
	  struct gdbarch *gdbarch = regcache->arch ();
	  struct ravenscar_arch_ops *arch_ops
	    = gdbarch_ravenscar_ops (gdbarch);

	  arch_ops->store_registers (regcache, regnum);
	}
    }
  else
    beneath ()->store_registers (regcache, regnum);
}

void
ravenscar_thread_target::prepare_to_store (struct regcache *regcache)
{
  ptid_t ptid = regcache->ptid ();

  if (runtime_initialized () && is_ravenscar_task (ptid))
    {
      if (task_is_currently_active (ptid))
	{
	  ptid_t base = get_base_thread_from_ravenscar_task (ptid);
	  temporarily_change_regcache_ptid changer (regcache, base);
	  beneath ()->prepare_to_store (regcache);
	}
      else
	{
	  /* Nothing.  */
	}
    }
  else
    beneath ()->prepare_to_store (regcache);
}

/* Implement the to_stopped_by_sw_breakpoint target_ops "method".  */

bool
ravenscar_thread_target::stopped_by_sw_breakpoint ()
{
  scoped_restore_current_thread saver;
  set_base_thread_from_ravenscar_task (inferior_ptid);
  return beneath ()->stopped_by_sw_breakpoint ();
}

/* Implement the to_stopped_by_hw_breakpoint target_ops "method".  */

bool
ravenscar_thread_target::stopped_by_hw_breakpoint ()
{
  scoped_restore_current_thread saver;
  set_base_thread_from_ravenscar_task (inferior_ptid);
  return beneath ()->stopped_by_hw_breakpoint ();
}

/* Implement the to_stopped_by_watchpoint target_ops "method".  */

bool
ravenscar_thread_target::stopped_by_watchpoint ()
{
  scoped_restore_current_thread saver;
  set_base_thread_from_ravenscar_task (inferior_ptid);
  return beneath ()->stopped_by_watchpoint ();
}

/* Implement the to_stopped_data_address target_ops "method".  */

bool
ravenscar_thread_target::stopped_data_address (CORE_ADDR *addr_p)
{
  scoped_restore_current_thread saver;
  set_base_thread_from_ravenscar_task (inferior_ptid);
  return beneath ()->stopped_data_address (addr_p);
}

void
ravenscar_thread_target::mourn_inferior ()
{
  m_base_ptid = null_ptid;
  target_ops *beneath = this->beneath ();
  unpush_target (this);
  beneath->mourn_inferior ();
}

/* Implement the to_core_of_thread target_ops "method".  */

int
ravenscar_thread_target::core_of_thread (ptid_t ptid)
{
  scoped_restore_current_thread saver;
  set_base_thread_from_ravenscar_task (inferior_ptid);
  return beneath ()->core_of_thread (inferior_ptid);
}

/* Implement the target xfer_partial method.  */

enum target_xfer_status
ravenscar_thread_target::xfer_partial (enum target_object object,
				       const char *annex,
				       gdb_byte *readbuf,
				       const gdb_byte *writebuf,
				       ULONGEST offset, ULONGEST len,
				       ULONGEST *xfered_len)
{
  scoped_restore save_ptid = make_scoped_restore (&inferior_ptid);
  /* Calling get_base_thread_from_ravenscar_task can read memory from
     the inferior.  However, that function is written to prefer our
     internal map, so it should not result in recursive calls in
     practice.  */
  inferior_ptid = get_base_thread_from_ravenscar_task (inferior_ptid);
  return beneath ()->xfer_partial (object, annex, readbuf, writebuf,
				   offset, len, xfered_len);
}

/* Observer on inferior_created: push ravenscar thread stratum if needed.  */

static void
ravenscar_inferior_created (struct target_ops *target, int from_tty)
{
  const char *err_msg;

  if (!ravenscar_task_support
      || gdbarch_ravenscar_ops (target_gdbarch ()) == NULL
      || !has_ravenscar_runtime ())
    return;

  err_msg = ada_get_tcb_types_info ();
  if (err_msg != NULL)
    {
      warning (_("%s. Task/thread support disabled."), err_msg);
      return;
    }

  ravenscar_thread_target *rtarget = new ravenscar_thread_target ();
  push_target (target_ops_up (rtarget));
  thread_info *thr = rtarget->add_active_thread ();
  if (thr != nullptr)
    switch_to_thread (thr);
}

ptid_t
ravenscar_thread_target::get_ada_task_ptid (long lwp, long thread)
{
  return ptid_t (m_base_ptid.pid (), 0, thread);
}

/* Command-list for the "set/show ravenscar" prefix command.  */
static struct cmd_list_element *set_ravenscar_list;
static struct cmd_list_element *show_ravenscar_list;

/* Implement the "show ravenscar task-switching" command.  */

static void
show_ravenscar_task_switching_command (struct ui_file *file, int from_tty,
				       struct cmd_list_element *c,
				       const char *value)
{
  if (ravenscar_task_support)
    fprintf_filtered (file, _("\
Support for Ravenscar task/thread switching is enabled\n"));
  else
    fprintf_filtered (file, _("\
Support for Ravenscar task/thread switching is disabled\n"));
}

/* Module startup initialization function, automagically called by
   init.c.  */

void _initialize_ravenscar ();
void
_initialize_ravenscar ()
{
  /* Notice when the inferior is created in order to push the
     ravenscar ops if needed.  */
  gdb::observers::inferior_created.attach (ravenscar_inferior_created);

  add_basic_prefix_cmd ("ravenscar", no_class,
			_("Prefix command for changing Ravenscar-specific settings."),
			&set_ravenscar_list, "set ravenscar ", 0, &setlist);

  add_show_prefix_cmd ("ravenscar", no_class,
		       _("Prefix command for showing Ravenscar-specific settings."),
		       &show_ravenscar_list, "show ravenscar ", 0, &showlist);

  add_setshow_boolean_cmd ("task-switching", class_obscure,
                           &ravenscar_task_support, _("\
Enable or disable support for GNAT Ravenscar tasks."), _("\
Show whether support for GNAT Ravenscar tasks is enabled."),
                           _("\
Enable or disable support for task/thread switching with the GNAT\n\
Ravenscar run-time library for bareboard configuration."),
			   NULL, show_ravenscar_task_switching_command,
			   &set_ravenscar_list, &show_ravenscar_list);
}
