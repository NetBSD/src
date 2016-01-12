/* Setting environment variables, with out-of-memory checking.
   Copyright (C) 2001-2002, 2005-2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>

/* Specification.  */
#include "xsetenv.h"

#include "setenv.h"
#include "error.h"
#include "exit.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Set NAME to VALUE in the environment.
   If REPLACE is nonzero, overwrite an existing value.
   With error checking.  */
void
xsetenv (const char *name, const char *value, int replace)
{
  if (setenv (name, value, replace) < 0)
    error (EXIT_FAILURE, 0, _("memory exhausted"));
}
