/* This testcase is part of GDB, the GNU debugger.

   Copyright 2004 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

static volatile int done;

extern int
callee (int param)
{
  return param * done + 1;
}

extern int
caller (int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
  return callee (a1 << a2 * a3 / a4 + a6 & a6 % a7 - a8) + done;
}

static void
catcher (int sig)
{
  done = 1;
} /* handler */

static void
thrower (void)
{
  *(char *)0 = 0;
}

main ()
{
  signal (SIGSEGV, catcher);
  thrower ();
}
