/* Low level interface to ptrace, for GDB when running under Unix.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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

#ifndef INFLOW_H
#define INFLOW_H

#include <unistd.h>
#include <signal.h>
#include "gdbsupport/job-control.h"

/* RAII class used to ignore SIGTTOU in a scope.  */

class scoped_ignore_sigttou
{
public:
  scoped_ignore_sigttou ()
  {
#ifdef SIGTTOU
    if (job_control)
      m_osigttou = signal (SIGTTOU, SIG_IGN);
#endif
  }

  ~scoped_ignore_sigttou ()
  {
#ifdef SIGTTOU
    if (job_control)
      signal (SIGTTOU, m_osigttou);
#endif
  }

  DISABLE_COPY_AND_ASSIGN (scoped_ignore_sigttou);

private:
#ifdef SIGTTOU
  sighandler_t m_osigttou = NULL;
#endif
};

#endif /* inflow.h */
