/* Displaced stepping related things.

   Copyright (C) 2020-2025 Free Software Foundation, Inc.

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

#ifndef GDB_DISPLACED_STEPPING_H
#define GDB_DISPLACED_STEPPING_H

#include "gdbsupport/array-view.h"
#include "gdbsupport/byte-vector.h"

struct gdbarch;
struct inferior;
struct target_ops;
struct thread_info;

/* True if we are debugging displaced stepping.  */

extern bool debug_displaced;

/* Print a "displaced" debug statement.  */

#define displaced_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_displaced, "displaced",fmt, ##__VA_ARGS__)

enum displaced_step_prepare_status
{
  /* A displaced stepping buffer was successfully allocated and prepared.  */
  DISPLACED_STEP_PREPARE_STATUS_OK,

  /* This particular instruction can't be displaced stepped, GDB should fall
     back on in-line stepping.  */
  DISPLACED_STEP_PREPARE_STATUS_CANT,

  /* Not enough resources are available at this time, try again later.  */
  DISPLACED_STEP_PREPARE_STATUS_UNAVAILABLE,
};

/* Return a string representation of STATUS.  */

static inline const char *
displaced_step_prepare_status_str (displaced_step_prepare_status status)
{
  switch (status)
  {
  case DISPLACED_STEP_PREPARE_STATUS_OK:
    return "OK";

  case DISPLACED_STEP_PREPARE_STATUS_CANT:
    return "CANT";

  case DISPLACED_STEP_PREPARE_STATUS_UNAVAILABLE:
    return "UNAVAILABLE";
  }

  gdb_assert_not_reached ("invalid displaced_step_prepare_status value");
}

enum displaced_step_finish_status
{
  /* Either the instruction was stepped and fixed up, or the specified thread
     wasn't executing a displaced step (in which case there's nothing to
     finish). */
  DISPLACED_STEP_FINISH_STATUS_OK,

  /* The thread started a displaced step, but didn't complete it.  */
  DISPLACED_STEP_FINISH_STATUS_NOT_EXECUTED,
};

/* Return a string representation of STATUS.  */

static inline const char *
displaced_step_finish_status_str (displaced_step_finish_status status)
{
  switch (status)
  {
  case DISPLACED_STEP_FINISH_STATUS_OK:
    return "OK";

  case DISPLACED_STEP_FINISH_STATUS_NOT_EXECUTED:
    return "NOT_EXECUTED";
  }

  gdb_assert_not_reached ("invalid displaced_step_finish_status value");
}

/* Data returned by a gdbarch displaced_step_copy_insn method, to be passed to
   the matching displaced_step_fixup method.  */

struct displaced_step_copy_insn_closure
{
  virtual ~displaced_step_copy_insn_closure () = 0;
};

using displaced_step_copy_insn_closure_up
  = std::unique_ptr<displaced_step_copy_insn_closure>;

/* A simple displaced step closure that contains only a byte buffer.  */

struct buf_displaced_step_copy_insn_closure : displaced_step_copy_insn_closure
{
  buf_displaced_step_copy_insn_closure (int buf_size)
  : buf (buf_size)
  {}

  /* The content of this buffer is up to the user of the class, but typically
     original instruction bytes, used during fixup to determine what needs to
     be fixed up.  */
  gdb::byte_vector buf;
};

/* Per-inferior displaced stepping state.  */

struct displaced_step_inferior_state
{
  displaced_step_inferior_state ()
  {
    reset ();
  }

  /* Put this object back in its original state.  */
  void reset ()
  {
    failed_before = false;
    in_progress_count = 0;
    unavailable = false;
  }

  /* True if preparing a displaced step ever failed.  If so, we won't
     try displaced stepping for this inferior again.  */
  bool failed_before;

  /* Number of displaced steps in progress for this inferior.  */
  unsigned int in_progress_count;

  /* If true, this tells GDB that it's not worth asking the gdbarch displaced
     stepping implementation to prepare a displaced step, because it would
     return UNAVAILABLE.  This is set and reset by the gdbarch in the
     displaced_step_prepare and displaced_step_finish methods.  */
  bool unavailable;
};

/* Per-thread displaced stepping state.  */

struct displaced_step_thread_state
{
  /* Return true if this thread is currently executing a displaced step.  */
  bool in_progress () const
  {
    return m_original_gdbarch != nullptr;
  }

  /* Return the gdbarch of the thread prior to the step.  */
  gdbarch *get_original_gdbarch () const
  {
    return m_original_gdbarch;
  }

  /* Mark this thread as currently executing a displaced step.

     ORIGINAL_GDBARCH is the current gdbarch of the thread (before the step
     is executed).  */
  void set (gdbarch *original_gdbarch)
  {
    m_original_gdbarch = original_gdbarch;
  }

  /* Mark this thread as no longer executing a displaced step.  */
  void reset ()
  {
    m_original_gdbarch = nullptr;
  }

private:
  gdbarch *m_original_gdbarch = nullptr;
};

/* Control access to multiple displaced stepping buffers at fixed addresses.  */

struct displaced_step_buffers
{
  explicit displaced_step_buffers (gdb::array_view<CORE_ADDR> buffer_addrs)
  {
    gdb_assert (buffer_addrs.size () > 0);

    m_buffers.reserve (buffer_addrs.size ());

    for (CORE_ADDR buffer_addr : buffer_addrs)
      m_buffers.emplace_back (buffer_addr);
  }

  displaced_step_prepare_status prepare (thread_info *thread,
					 CORE_ADDR &displaced_pc);

  displaced_step_finish_status finish (gdbarch *arch, thread_info *thread,
				       const target_waitstatus &status);

  const displaced_step_copy_insn_closure *
    copy_insn_closure_by_addr (CORE_ADDR addr);

  void restore_in_ptid (ptid_t ptid);

private:

  /* State of a single buffer.  */

  struct displaced_step_buffer
  {
    explicit displaced_step_buffer (CORE_ADDR addr)
      : addr (addr)
    {}

    /* Address of the buffer.  */
    const CORE_ADDR addr;

    /* The original PC of the instruction currently being stepped.  */
    CORE_ADDR original_pc = 0;

    /* If set, the thread currently using the buffer.  If unset, the buffer is not
       used.  */
    thread_info *current_thread = nullptr;

    /* Saved copy of the bytes in the displaced buffer, to be restored once the
       buffer is no longer used.  */
    gdb::byte_vector saved_copy;

    /* Closure obtained from gdbarch_displaced_step_copy_insn, to be passed to
       gdbarch_displaced_step_fixup_insn.  */
    displaced_step_copy_insn_closure_up copy_insn_closure;
  };

  std::vector<displaced_step_buffer> m_buffers;
};

/* Default implementation of target_ops::supports_displaced_step.

   Forwards the call to the architecture of THREAD.  */

bool default_supports_displaced_step (target_ops *target, thread_info *thread);

/* Default implementation of target_ops::displaced_step_prepare.

   Forwards the call to the architecture of THREAD.  */

displaced_step_prepare_status default_displaced_step_prepare
  (target_ops *target, thread_info *thread, CORE_ADDR &displaced_pc);

/* Default implementation of target_ops::displaced_step_finish.

   Forwards the call to the architecture of THREAD.  */

displaced_step_finish_status default_displaced_step_finish
  (target_ops *target, thread_info *thread, const target_waitstatus &status);

/* Default implementation of target_ops::displaced_step_restore_all_in_ptid.

   Forwards the call to the architecture of PARENT_INF.  */

void default_displaced_step_restore_all_in_ptid (target_ops *target,
						 inferior *parent_inf,
						 ptid_t child_ptid);

#endif /* GDB_DISPLACED_STEPPING_H */
