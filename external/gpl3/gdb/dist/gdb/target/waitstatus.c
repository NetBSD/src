/* Target waitstatus implementations.

   Copyright (C) 1990-2020 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "waitstatus.h"

/* Return a pretty printed form of target_waitstatus.  */

std::string
target_waitstatus_to_string (const struct target_waitstatus *ws)
{
  const char *kind_str = "status->kind = ";

  switch (ws->kind)
    {
    case TARGET_WAITKIND_EXITED:
      return string_printf ("%sexited, status = %d",
			    kind_str, ws->value.integer);
    case TARGET_WAITKIND_STOPPED:
      return string_printf ("%sstopped, signal = %s",
			    kind_str,
			    gdb_signal_to_symbol_string (ws->value.sig));
    case TARGET_WAITKIND_SIGNALLED:
      return string_printf ("%ssignalled, signal = %s",
			    kind_str,
			    gdb_signal_to_symbol_string (ws->value.sig));
    case TARGET_WAITKIND_LOADED:
      return string_printf ("%sloaded", kind_str);
    case TARGET_WAITKIND_FORKED:
      return string_printf ("%sforked", kind_str);
    case TARGET_WAITKIND_VFORKED:
      return string_printf ("%svforked", kind_str);
    case TARGET_WAITKIND_EXECD:
      return string_printf ("%sexecd", kind_str);
    case TARGET_WAITKIND_VFORK_DONE:
      return string_printf ("%svfork-done", kind_str);
    case TARGET_WAITKIND_SYSCALL_ENTRY:
      return string_printf ("%sentered syscall", kind_str);
    case TARGET_WAITKIND_SYSCALL_RETURN:
      return string_printf ("%sexited syscall", kind_str);
    case TARGET_WAITKIND_SPURIOUS:
      return string_printf ("%sspurious", kind_str);
    case TARGET_WAITKIND_IGNORE:
      return string_printf ("%signore", kind_str);
    case TARGET_WAITKIND_NO_HISTORY:
      return string_printf ("%sno-history", kind_str);
    case TARGET_WAITKIND_NO_RESUMED:
      return string_printf ("%sno-resumed", kind_str);
    case TARGET_WAITKIND_THREAD_CREATED:
      return string_printf ("%sthread created", kind_str);
    case TARGET_WAITKIND_THREAD_EXITED:
      return string_printf ("%sthread exited, status = %d",
			    kind_str, ws->value.integer);
    default:
      return string_printf ("%sunknown???", kind_str);
    }
}
