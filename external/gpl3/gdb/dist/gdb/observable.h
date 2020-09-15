/* Observers

   Copyright (C) 2016-2020 Free Software Foundation, Inc.

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

#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include "gdbsupport/observable.h"

struct bpstats;
struct so_list;
struct objfile;
struct thread_info;
struct inferior;
struct process_stratum_target;
struct trace_state_variable;

namespace gdb
{

namespace observers
{

/* The inferior has stopped for real.  The BS argument describes the
   breakpoints were are stopped at, if any.  Second argument
   PRINT_FRAME non-zero means display the location where the
   inferior has stopped.

   gdb notifies all normal_stop observers when the inferior execution
   has just stopped, the associated messages and annotations have been
   printed, and the control is about to be returned to the user.

   Note that the normal_stop notification is not emitted when the
   execution stops due to a breakpoint, and this breakpoint has a
   condition that is not met.  If the breakpoint has any associated
   commands list, the commands are executed after the notification is
   emitted.  */
extern observable<struct bpstats */* bs */, int /* print_frame */> normal_stop;

/* The inferior was stopped by a signal.  */
extern observable<enum gdb_signal /* siggnal */> signal_received;

/* We are done with a step/next/si/ni command.  */
extern observable<> end_stepping_range;

/* The inferior was terminated by a signal.  */
extern observable<enum gdb_signal /* siggnal */> signal_exited;

/* The inferior program is finished.  */
extern observable<int /* exitstatus */> exited;

/* Reverse execution: target ran out of history info.  */
extern observable<> no_history;

/* A synchronous command finished.  */
extern observable<> sync_execution_done;

/* An error was caught while executing a command.  */
extern observable<> command_error;

/* The target's register contents have changed.  */
extern observable<struct target_ops */* target */> target_changed;

/* The executable being debugged by GDB has changed: The user
   decided to debug a different program, or the program he was
   debugging has been modified since being loaded by the debugger
   (by being recompiled, for instance).  */
extern observable<> executable_changed;

/* gdb has just connected to an inferior.  For 'run', gdb calls this
   observer while the inferior is still stopped at the entry-point
   instruction.  For 'attach' and 'core', gdb calls this observer
   immediately after connecting to the inferior, and before any
   information on the inferior has been printed.  */
extern observable<struct target_ops */* target */,
		  int /* from_tty */> inferior_created;

/* The status of process record for inferior inferior in gdb has
   changed.  The process record is started if STARTED is true, and
   the process record is stopped if STARTED is false.

   When STARTED is true, METHOD indicates the short name of the
   method used for recording.  If the method supports multiple
   formats, FORMAT indicates which one is being used, otherwise it
   is NULL.  When STARTED is false, they are both NULL.  */
extern observable<struct inferior */* inferior */, int /* started */,
		  const char */* method */, const char */* format */>
    record_changed;

/* The shared library specified by SOLIB has been loaded.  Note that
   when gdb calls this observer, the library's symbols probably
   haven't been loaded yet.  */
extern observable<struct so_list */* solib */> solib_loaded;

/* The shared library specified by SOLIB has been unloaded.  Note
   that when gdb calls this observer, the library's symbols have not
   been unloaded yet, and thus are still available.  */
extern observable<struct so_list */* solib */> solib_unloaded;

/* The symbol file specified by OBJFILE has been loaded.  Called
   with OBJFILE equal to NULL to indicate previously loaded symbol
   table data has now been invalidated.  */
extern observable<struct objfile */* objfile */> new_objfile;

/* The object file specified by OBJFILE is about to be freed.  */
extern observable<struct objfile */* objfile */> free_objfile;

/* The thread specified by T has been created.  */
extern observable<struct thread_info */* t */> new_thread;

/* The thread specified by T has exited.  The SILENT argument
   indicates that gdb is removing the thread from its tables without
   wanting to notify the user about it.  */
extern observable<struct thread_info */* t */, int /* silent */> thread_exit;

/* An explicit stop request was issued to PTID.  If PTID equals
   minus_one_ptid, the request applied to all threads.  If
   ptid_is_pid(PTID) returns true, the request applied to all
   threads of the process pointed at by PTID.  Otherwise, the
   request applied to the single thread pointed at by PTID.  */
extern observable<ptid_t /* ptid */> thread_stop_requested;

/* The target was resumed.  The PTID parameter specifies which
   thread was resume, and may be RESUME_ALL if all threads are
   resumed.  */
extern observable<ptid_t /* ptid */> target_resumed;

/* The target is about to be proceeded.  */
extern observable<> about_to_proceed;

/* A new breakpoint B has been created.  */
extern observable<struct breakpoint */* b */> breakpoint_created;

/* A breakpoint has been destroyed.  The argument B is the
   pointer to the destroyed breakpoint.  */
extern observable<struct breakpoint */* b */> breakpoint_deleted;

/* A breakpoint has been modified in some way.  The argument B
   is the modified breakpoint.  */
extern observable<struct breakpoint */* b */> breakpoint_modified;

/* The trace frame is changed to TFNUM (e.g., by using the 'tfind'
   command).  If TFNUM is negative, it means gdb resumes live
   debugging.  The number of the tracepoint associated with this
   traceframe is TPNUM.  */
extern observable<int /* tfnum */, int /* tpnum */> traceframe_changed;

/* The current architecture has changed.  The argument NEWARCH is a
   pointer to the new architecture.  */
extern observable<struct gdbarch */* newarch */> architecture_changed;

/* The thread's ptid has changed.  The OLD_PTID parameter specifies
   the old value, and NEW_PTID specifies the new value.  */
extern observable<process_stratum_target * /* target */,
		  ptid_t /* old_ptid */, ptid_t /* new_ptid */>
  thread_ptid_changed;

/* The inferior INF has been added to the list of inferiors.  At
   this point, it might not be associated with any process.  */
extern observable<struct inferior */* inf */> inferior_added;

/* The inferior identified by INF has been attached to a
   process.  */
extern observable<struct inferior */* inf */> inferior_appeared;

/* Either the inferior associated with INF has been detached from
   the process, or the process has exited.  */
extern observable<struct inferior */* inf */> inferior_exit;

/* The inferior INF has been removed from the list of inferiors.
   This method is called immediately before freeing INF.  */
extern observable<struct inferior */* inf */> inferior_removed;

/* Bytes from DATA to DATA + LEN have been written to the inferior
   at ADDR.  */
extern observable<struct inferior */* inferior */, CORE_ADDR /* addr */,
		  ssize_t /* len */, const bfd_byte */* data */>
    memory_changed;

/* Called before a top-level prompt is displayed.  CURRENT_PROMPT is
   the current top-level prompt.  */
extern observable<const char */* current_prompt */> before_prompt;

/* Variable gdb_datadir has been set.  The value may not necessarily
   change.  */
extern observable<> gdb_datadir_changed;

/* The parameter of some 'set' commands in console are changed.
   This method is called after a command 'set param value'.  PARAM
   is the parameter of 'set' command, and VALUE is the value of
   changed parameter.  */
extern observable<const char */* param */, const char */* value */>
    command_param_changed;

/* The new trace state variable TSV is created.  */
extern observable<const struct trace_state_variable */* tsv */> tsv_created;

/* The trace state variable TSV is deleted.  If TSV is NULL, all
   trace state variables are deleted.  */
extern observable<const struct trace_state_variable */* tsv */> tsv_deleted;

/* The trace state value TSV is modified.  */
extern observable<const struct trace_state_variable */* tsv */> tsv_modified;

/* An inferior function at ADDRESS is about to be called in thread
   THREAD.  */
extern observable<ptid_t /* thread */, CORE_ADDR /* address */>
    inferior_call_pre;

/* The inferior function at ADDRESS has just been called.  This
   observer is called even if the inferior exits during the call.
   THREAD is the thread in which the function was called, which may
   be different from the current thread.  */
extern observable<ptid_t /* thread */, CORE_ADDR /* address */>
    inferior_call_post;

/* A register in the inferior has been modified by the gdb user.  */
extern observable<struct frame_info */* frame */, int /* regnum */>
    register_changed;

/* The user-selected inferior, thread and/or frame has changed.  The
   user_select_what flag specifies if the inferior, thread and/or
   frame has changed.  */
extern observable<user_selected_what /* selection */>
    user_selected_context_changed;

/* This is notified when the source styling setting has changed and
   should be reconsulted.  */
extern observable<> source_styling_changed;

/* The CLI's notion of the current source has changed.  This differs
   from user_selected_context_changed in that it is also set by the
   "list" command.  */

extern observable<> current_source_symtab_and_line_changed;

} /* namespace observers */

} /* namespace gdb */

#endif /* OBSERVABLE_H */
