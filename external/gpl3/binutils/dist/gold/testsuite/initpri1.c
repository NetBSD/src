/* initpri1.c -- test constructor priorities.

   Copyright 2007, 2008 Free Software Foundation, Inc.
   Copied from the gcc testsuite, where the test was contributed by
   Mark Mitchell <mark@codesourcery.com>.

   This file is part of gold.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.

   This is a test of a common symbol in the main program and a
   versioned symbol in a shared library.  The common symbol in the
   main program should override the shared library symbol.  */

#include <stdlib.h>

int i;
int j;

void c1() __attribute__((constructor (500)));
void c2() __attribute__((constructor (700)));
void c3() __attribute__((constructor (600)));

void c1() {
  if (i++ != 0)
    abort ();
}

void c2() {
  if (i++ != 2)
    abort ();
}

void c3() {
  if (i++ != 1)
    abort ();
}

void d1() __attribute__((destructor (500)));
void d2() __attribute__((destructor (700)));
void d3() __attribute__((destructor (600)));

void d1() {
  if (--i != 0)
    abort ();
}

void d2() {
  if (--i != 2)
    abort ();
}

void d3() {
  if (j != 4)
    abort ();
  if (--i != 1)
    abort ();
}

void cd4() __attribute__((constructor (800), destructor (800)));

void cd4() {
  if (i != 3)
    abort ();
  ++j;
}

void cd5() __attribute__((constructor, destructor));

void cd5() {
  if (i != 3)
    abort();
  ++j;
}

int main () {
  if (i != 3)
    return 1;
  if (j != 2)
    abort ();
  return 0;
}
