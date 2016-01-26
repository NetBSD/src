// icf_safe_so_test.cc -- a test case for gold

// Copyright 2010 Free Software Foundation, Inc.
// Written by Sriraman Tallam <tmsriram@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

// The goal of this program is to verify if identical code folding
// in safe mode correctly folds functions in a shared object. The
// foo_* functions below should not be folded.  For x86-64,
// foo_glob and bar_glob should be folded as their function pointers
// are addresses of PLT entries in shared objects.  For 32-bit X86,
// the hidden protected and internal symbols can be folded.

int  __attribute__ ((visibility ("protected")))
foo_prot()
{
  return 1;
}

int __attribute__ ((visibility ("hidden")))
foo_hidden()
{
  return 1;
}

int __attribute__ ((visibility ("internal")))
foo_internal()
{
  return 1;
}

static int
foo_static()
{
  return 1;
}

int foo_glob()
{
  return 2;
}

int bar_glob()
{
  return 2;
}

int main()
{
  int (*p)() = foo_glob;
  (void)p;
  foo_static();
  foo_prot();
  foo_hidden();
  foo_internal();
  return 0;
}

