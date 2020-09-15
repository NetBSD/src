/* Error table.
   Copyright (C) 2019-2020 Free Software Foundation, Inc.

   This file is part of libctf.

   libctf is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#include <ctf-impl.h>
#include <stddef.h>
#include <string.h>

/* This construct is due to Bruno Haible: much thanks.  */

/* Give each structure member a unique name.  The name does not matter, so we
   use the line number in ctf-error.h to uniquify them.  */

#define ERRSTRFIELD(line) ERRSTRFIELD1 (line)
#define ERRSTRFIELD1(line) ctf_errstr##line

/* The error message strings, each in a unique structure member precisely big
   enough for that error, plus a str member to access them all as a string
   table.  */

static const union _ctf_errlist_t
{
  __extension__ struct
  {
#define _CTF_STR(n, s) char ERRSTRFIELD (__LINE__) [sizeof (s)];
#include "ctf-error.h"
#undef _CTF_STR
  };
  char str[1];
} _ctf_errlist =
  {
   {
#define _CTF_STR(n, s) N_(s),
#include "ctf-error.h"
#undef _CTF_STR
   }
  };

/* Offsets to each member in the string table, computed using offsetof.  */

static const unsigned int _ctf_erridx[] =
  {
#define _CTF_STR(n, s) [n - ECTF_BASE] = offsetof (union _ctf_errlist_t, ERRSTRFIELD (__LINE__)),
#include "ctf-error.h"
#undef _CTF_STR
  };

const char *
ctf_errmsg (int error)
{
  const char *str;

  if (error >= ECTF_BASE && (error - ECTF_BASE) < ECTF_NERR)
    str = _ctf_errlist.str + _ctf_erridx[error - ECTF_BASE];
  else
    str = (const char *) strerror (error);

  return (str ? gettext (str) : _("Unknown error"));
}

int
ctf_errno (ctf_file_t * fp)
{
  return fp->ctf_errno;
}
