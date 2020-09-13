/* Abstract base class inherited by all process_stratum targets

   Copyright (C) 2018-2019 Free Software Foundation, Inc.

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

/* Abstract base class inherited by all process_stratum targets.  */

class process_stratum_target : public target_ops
{
public:
  ~process_stratum_target () override = 0;

  strata stratum () const override { return process_stratum; }

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
  bool has_execution (ptid_t the_ptid) override;
};

#endif /* !defined (PROCESS_STRATUM_TARGET_H) */
