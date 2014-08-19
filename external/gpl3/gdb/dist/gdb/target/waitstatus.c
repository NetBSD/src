/* Target waitstatus implementations.

   Copyright (C) 1990-2014 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#endif

#include "waitstatus.h"

/* Return a pretty printed form of target_waitstatus.
   Space for the result is malloc'd, caller must free.  */

char *
target_waitstatus_to_string (const struct target_waitstatus *ws)
{
  const char *kind_str = "status->kind = ";

  switch (ws->kind)
    {
    case TARGET_WAITKIND_EXITED:
      return xstrprintf ("%sexited, status = %d",
			 kind_str, ws->value.integer);
    case TARGET_WAITKIND_STOPPED:
      return xstrprintf ("%sstopped, signal = %s",
			 kind_str,
			 gdb_signal_to_symbol_string (ws->value.sig));
    case TARGET_WAITKIND_SIGNALLED:
      return xstrprintf ("%ssignalled, signal = %s",
			 kind_str,
			 gdb_signal_to_symbol_string (ws->value.sig));
    case TARGET_WAITKIND_LOADED:
      return xstrprintf ("%sloaded", kind_str);
    case TARGET_WAITKIND_FORKED:
      return xstrprintf ("%sforked", kind_str);
    case TARGET_WAITKIND_VFORKED:
      return xstrprintf ("%svforked", kind_str);
    case TARGET_WAITKIND_EXECD:
      return xstrprintf ("%sexecd", kind_str);
    case TARGET_WAITKIND_VFORK_DONE:
      return xstrprintf ("%svfork-done", kind_str);
    case TARGET_WAITKIND_SYSCALL_ENTRY:
      return xstrprintf ("%sentered syscall", kind_str);
    case TARGET_WAITKIND_SYSCALL_RETURN:
      return xstrprintf ("%sexited syscall", kind_str);
    case TARGET_WAITKIND_SPURIOUS:
      return xstrprintf ("%sspurious", kind_str);
    case TARGET_WAITKIND_IGNORE:
      return xstrprintf ("%signore", kind_str);
    case TARGET_WAITKIND_NO_HISTORY:
      return xstrprintf ("%sno-history", kind_str);
    case TARGET_WAITKIND_NO_RESUMED:
      return xstrprintf ("%sno-resumed", kind_str);
    default:
      return xstrprintf ("%sunknown???", kind_str);
    }
}
