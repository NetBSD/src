/* Utility for handling interrupted syscalls by signals.

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

#ifndef GDBSUPPORT_EINTR_H
#define GDBSUPPORT_EINTR_H

#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace gdb
{
/* Repeat a system call interrupted with a signal.

   A utility for handling interrupted syscalls, which return with error
   and set the errno to EINTR.  The interrupted syscalls can be repeated,
   until successful completion.  This utility avoids wrapping code with
   manual checks for such errors which are highly repetitive.

   For example, with:

   ssize_t ret;
   do
     {
       errno = 0;
       ret = ::write (pipe[1], "+", 1);
     }
   while (ret == -1 && errno == EINTR);

   You could wrap it by writing the wrapped form:

   ssize_t ret = gdb::handle_eintr (-1, ::write, pipe[1], "+", 1);

   ERRVAL specifies the failure value indicating that the call to the
   F function with ARGS... arguments was possibly interrupted with a
   signal.  */

template<typename ErrorValType, typename Fun, typename... Args>
inline auto
handle_eintr (ErrorValType errval, const Fun &f, const Args &... args)
  -> decltype (f (args...))
{
  decltype (f (args...)) ret;

  do
    {
      errno = 0;
      ret = f (args...);
    }
  while (ret == errval && errno == EINTR);

  return ret;
}

#ifdef HAVE_WAITPID
inline pid_t
waitpid (pid_t pid, int *wstatus, int options)
{
  return gdb::handle_eintr (-1, ::waitpid, pid, wstatus, options);
}
#endif /* HAVE_WAITPID */

inline int
open (const char *pathname, int flags)
{
  return gdb::handle_eintr (-1, ::open, pathname, flags);
}

#ifdef HAVE_WAIT
inline pid_t
wait (int *wstatus)
{
  return gdb::handle_eintr (-1, ::wait, wstatus);
}
#endif /* HAVE_WAIT */

inline int
close (int fd)
{
  return gdb::handle_eintr (-1, ::close, fd);
}

inline ssize_t
read (int fd, void *buf, size_t count)
{
  return gdb::handle_eintr (-1, ::read, fd, buf, count);
}

template<typename... Args> int fcntl (int fd, int op, Args... args)
{
  return gdb::handle_eintr (-1, ::fcntl, fd, op, args...);
}

inline ssize_t
write (int fd, const void *buf, size_t count)
{
  return gdb::handle_eintr (-1, ::write, fd, buf, count);
}

} /* namespace gdb */

#endif /* GDBSUPPORT_EINTR_H */
