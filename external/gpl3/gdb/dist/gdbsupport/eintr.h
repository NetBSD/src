/* Utility for handling interrupted syscalls by signals.

   Copyright (C) 2020 Free Software Foundation, Inc.

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

   ssize_t ret = gdb::handle_eintr<ssize_t> (-1, ::write, pipe[1], "+", 1);

   The RET typename specifies the return type of the wrapped system call, which
   is typically int or ssize_t.  The R argument specifies the failure value
   indicating the interrupted syscall when calling the F function with
   the A... arguments.  */

template <typename Ret, typename Fun, typename... Args>
inline Ret handle_eintr (const Ret &R, const Fun &F, const Args &... A)
{
  Ret ret;
  do
    {
      errno = 0;
      ret = F (A...);
    }
  while (ret == R && errno == EINTR);
  return ret;
}
}

#endif /* GDBSUPPORT_EINTR_H */
