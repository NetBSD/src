/* Abstract base class inherited by all process_stratum targets

   Copyright (C) 2018-2020 Free Software Foundation, Inc.

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

#ifndef PROCESS_STRATUM_TARGET_H
#define PROCESS_STRATUM_TARGET_H

#include "target.h"
#include <set>

/* Abstract base class inherited by all process_stratum targets.  */

class process_stratum_target : public target_ops
{
public:
  ~process_stratum_target () override = 0;

  strata stratum () const final override { return process_stratum; }

  /* Return a string representation of this target's open connection.
     This string is used to distinguish different instances of a given
     target type.  For example, when remote debugging, the target is
     called "remote", but since we may have more than one remote
     target open, connection_string() returns the connection serial
     connection name, e.g., "localhost:10001", "192.168.0.1:20000",
     etc.  This string is shown in several places, e.g., in "info
     connections" and "info inferiors".  */
  virtual const char *connection_string () { return nullptr; }

  /* We must default these because they must be implemented by any
     target that can run.  */
  bool can_async_p () override { return false; }
  bool supports_non_stop () override { return false; }
  bool supports_disable_randomization () override { return false; }

  /* This default implementation returns the inferior's address
     space.  */
  struct address_space *thread_address_space (ptid_t ptid) override;

  /* This default implementation always returns target_gdbarch ().  */
  struct gdbarch *thread_architecture (ptid_t ptid) override;

  /* Default implementations for process_stratum targets.  Return true
     if there's a selected inferior, false otherwise.  */
  bool has_all_memory () override;
  bool has_memory () override;
  bool has_stack () override;
  bool has_registers () override;
  bool has_execution (inferior *inf) override;

  /* True if any thread is, or may be executing.  We need to track
     this separately because until we fully sync the thread list, we
     won't know whether the target is fully stopped, even if we see
     stop events for all known threads, because any of those threads
     may have spawned new threads we haven't heard of yet.  */
  bool threads_executing = false;

  /* The connection number.  Visible in "info connections".  */
  int connection_number = 0;
};

/* Downcast TARGET to process_stratum_target.  */

static inline process_stratum_target *
as_process_stratum_target (target_ops *target)
{
  gdb_assert (target->stratum () == process_stratum);
  return static_cast<process_stratum_target *> (target);
}

/* Return a collection of targets that have non-exited inferiors.  */

extern std::set<process_stratum_target *> all_non_exited_process_targets ();

/* Switch to the first inferior (and program space) of TARGET, and
   switch to no thread selected.  */

extern void switch_to_target_no_thread (process_stratum_target *target);

#endif /* !defined (PROCESS_STRATUM_TARGET_H) */
