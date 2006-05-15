/* Win32 termcap emulation.

   Copyright 2005 Free Software Foundation, Inc.

   Contributed by CodeSourcery, LLC.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without eve nthe implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St., Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include <stdlib.h>

/* Each of the files below is a minimal implementation of the standard
   termcap function with the same name, suitable for use in a Windows
   console window.  */

int
tgetent (char *buffer, char *termtype)
{
  return -1;
}

int
tgetnum (char *name)
{
  return -1;
}

int
tgetflag (char *name)
{
  return -1;
}

char *
tgetstr (char *name, char **area)
{
  return NULL;
}

int
tputs (char *string, int nlines, int (*outfun) ())
{
  while (*string)
    outfun (*string++);
}

char *
tgoto (const char *cap, int col, int row)
{
  return NULL;
}
