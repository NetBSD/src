// script_test_1.cc -- linker script test 1 for gold  -*- C++ -*-

// Copyright 2008 Free Software Foundation, Inc.
// Written by Ian Lance Taylor <iant@google.com>.

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

// A test for a linker script which sets symbols to values.

#include <cassert>
#include <cstddef>
#include <stdint.h>

extern char a, b, c, d, e, f, g;
int sym = 3;
int common_sym;

int
main(int, char**)
{
  assert(reinterpret_cast<intptr_t>(&a) == 123);
  assert(reinterpret_cast<intptr_t>(&b) == reinterpret_cast<intptr_t>(&a) * 2);
  assert(reinterpret_cast<intptr_t>(&c)
	 == reinterpret_cast<intptr_t>(&b) + 3 * 6);
  assert(reinterpret_cast<intptr_t>(&d)
	 == (reinterpret_cast<intptr_t>(&b) + 3) * 6);
  assert(reinterpret_cast<int*>(&e) == &sym);
  assert(reinterpret_cast<intptr_t>(&f)
	 == reinterpret_cast<intptr_t>(&sym) + 10);
  assert(reinterpret_cast<int*>(&g) == &common_sym);
  return 0;
}
