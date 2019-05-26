/* Error reporting facilities.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "errors.h"

/* See common/errors.h.  */

void
warning (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vwarning (fmt, ap);
  va_end (ap);
}

/* See common/errors.h.  */

void
error (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  verror (fmt, ap);
  va_end (ap);
}

/* See common/errors.h.  */

void
internal_error (const char *file, int line, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  internal_verror (file, line, fmt, ap);
  va_end (ap);
}

/* See common/errors.h.  */

void
internal_warning (const char *file, int line, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  internal_vwarning (file, line, fmt, ap);
  va_end (ap);
}
