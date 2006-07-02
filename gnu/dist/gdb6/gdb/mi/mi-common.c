/* Interface for common GDB/MI data
   Copyright (C) 2005 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "defs.h"
#include "mi-common.h"

static const char * const async_reason_string_lookup[] =
{
  "breakpoint-hit",
  "watchpoint-trigger",
  "read-watchpoint-trigger",
  "access-watchpoint-trigger",
  "function-finished",
  "location-reached",
  "watchpoint-scope",
  "end-stepping-range",
  "exited-signalled",
  "exited",
  "exited-normally",
  "signal-received",
  NULL
};

const char *
async_reason_lookup (enum async_reply_reason reason)
{
  return async_reason_string_lookup[reason];
}

void
_initialize_gdb_mi_common (void)
{
  if (ARRAY_SIZE (async_reason_string_lookup) != EXEC_ASYNC_LAST + 1)
    internal_error (__FILE__, __LINE__,
		    _("async_reason_string_lookup is inconsistent"));
}
