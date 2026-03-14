/* RAII class to install a separate handler for a given signal

   Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_SCOPED_SIGNAL_HANDLER_H
#define GDBSUPPORT_SCOPED_SIGNAL_HANDLER_H

#include <signal.h>

#undef HAVE_SIGACTION

/* RAII class to set a signal handler for a scope, that will take care of
   unsetting the handler when the scope is left.
   This class will try to use sigaction whenever available, following the
   recommendation on the man page for signal, and only fallback to signal
   if necessary.  */
template <int SIG>
class scoped_signal_handler
{
public:
  scoped_signal_handler (sighandler_t handler)
  {
#if defined (HAVE_SIGACTION)
    struct sigaction act;

    act.sa_handler = handler;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (SIG, &act, &m_prev_handler);
#else
    /* The return of the function call is the previous signal handler, or
       SIG_ERR if the function doesn't succeed.  */
    m_prev_handler = signal (SIG, handler);
    /* According to the GNU libc manual, the only way signal fails is if
       the signum given is invalid, so we should be safe to assert.  */
    gdb_assert (m_prev_handler != SIG_ERR);
#endif
  }

  ~scoped_signal_handler ()
  {
#if defined (HAVE_SIGACTION)
    sigaction (SIG, &m_prev_handler, nullptr);
#else
    signal (SIG, m_prev_handler);
#endif
  }

  DISABLE_COPY_AND_ASSIGN (scoped_signal_handler);
private:
#if defined (HAVE_SIGACTION)
  struct sigaction m_prev_handler;
#else
  sighandler_t m_prev_handler;
#endif
};

#endif /* GDBSUPPORT_SCOPED_SIGNAL_HANDLER_H */
