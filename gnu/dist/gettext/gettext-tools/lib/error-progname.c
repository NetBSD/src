/* Use of program name in error-reporting functions.
   Copyright (C) 2001-2003 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Specification.  */
#include "error-progname.h"

#include <stdio.h>

#include "progname.h"


/* Indicates whether errors and warnings get prefixed with program_name.
   Default is true.  */
bool error_with_progname = true;

/* Print program_name prefix on stderr if and only if error_with_progname
   is true.  */
void
maybe_print_progname ()
{
  if (error_with_progname)
    fprintf (stderr, "%s: ", program_name);
}
