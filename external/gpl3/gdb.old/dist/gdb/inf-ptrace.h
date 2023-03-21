/* Low level child interface to ptrace.

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

#ifndef INF_PTRACE_H
#define INF_PTRACE_H

#include "inf-child.h"

/* An abstract prototype ptrace target.  The client can override it
   with local methods.  */

struct inf_ptrace_target : public inf_child_target
{
  ~inf_ptrace_target () override = 0;

  void attach (const char *, int) override;

  void detach (inferior *inf, int) override;

  void resume (ptid_t, int, enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, int) override;

  void files_info () override;

  void kill () override;

  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void mourn_inferior () override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

protected:
  /* Cleanup the inferior after a successful ptrace detach.  */
  void detach_success (inferior *inf);
};

#ifndef __NetBSD__
/* Return which PID to pass to ptrace in order to observe/control the
   tracee identified by PTID.

   Unlike most other Operating Systems, NetBSD tracks both pid and lwp
   and avoids this function.  */

extern pid_t get_ptrace_pid (ptid_t);
#endif

#endif
