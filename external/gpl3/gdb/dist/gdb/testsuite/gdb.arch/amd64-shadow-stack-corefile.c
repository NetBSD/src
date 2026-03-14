/* This test program is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

#include <stdio.h>

/* Call the return instruction before function epilogue to trigger a
   control-flow exception.  */
void
function ()
{
  unsigned long ssp;
  #ifndef __ILP32__
    asm volatile ("xor %0, %0; rdsspq %0" : "=r" (ssp));
  #else
    asm volatile ("xor %0, %0; rdsspd %0" : "=r" (ssp));
  #endif

  /* Print ssp to stdout so that the testcase can capture it.  */
  printf ("%p\n", (void *) ssp);
  fflush (stdout);

  /* Manually cause a control-flow exception by executing a return
     instruction before function epilogue, so the address atop the stack
     is not the return instruction.  */
  __asm__ volatile ("ret\n");
}

int
main (void)
{
  function (); /* Break here.  */
}
