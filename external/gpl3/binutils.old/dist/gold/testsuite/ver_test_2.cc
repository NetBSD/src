// ver_test_2.cc -- a test case for gold

// Copyright 2007, 2008 Free Software Foundation, Inc.
// Written by Cary Coutant <ccoutant@google.com>.

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

#include "ver_test.h"

int
t3_2()
{
  TRACE
  return t1_2();
}

// Calls a versioned function in ver_test_4.cc which should be
// overridden by an unversioned function in the main program.

int
t4_2()
{
  TRACE
  return t4_2a();
}
