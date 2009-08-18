// script_test_2.cc -- linker script test 2 for gold  -*- C++ -*-

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

// A test of some uses of the SECTIONS clause.  Look at
// script_test_2.t to make sense of this test.

#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdint.h>

extern char start_test_area[];
extern char start_test_area_1[];
extern char start_data[];
extern char end_data[];
extern char start_fill[];
extern char end_fill[];
extern char end_test_area[];
extern char test_addr[];
extern char test_addr_alias[];

int
main(int, char**)
{
  assert(reinterpret_cast<uintptr_t>(start_test_area) == 0x20000001);
  assert(reinterpret_cast<uintptr_t>(start_test_area_1) == 0x20000010);

  // We should see the string from script_test_2b.o next.  The
  // subalign should move it up to 0x20000020.
  for (int i = 0; i < 16; ++i)
    assert(start_test_area_1[i] == 0);
  assert(strcmp(start_test_area_1 + 16, "test bb") == 0);

  // Next the string from script_test_2a.o, after the subalign.
  for (int i = 16 + 7; i < 48; ++i)
    assert(start_test_area_1[i] == 0);
  assert(strcmp(start_test_area_1 + 48, "test aa") == 0);

  // Move four bytes forward to start_data.
  assert(reinterpret_cast<uintptr_t>(start_test_area_1 + 48 + 8 + 4)
	 == reinterpret_cast<uintptr_t>(start_data));
  assert(memcmp(start_data, "\1\2\0\4\0\0\0\010\0\0\0\0\0\0\0", 15) == 0
	 || memcmp(start_data, "\1\0\2\0\0\0\4\0\0\0\0\0\0\0\010", 15) == 0);
  assert(end_data == start_data + 15);

  // Check that FILL works as expected.
  assert(start_fill == end_data);
  assert(memcmp(start_fill, "\x12\x34\x56\x78\x12\x34\x56\0", 8) == 0);
  assert(end_fill == start_fill + 8);

  assert(end_test_area == end_fill);

  assert(test_addr == start_test_area_1);
  assert(test_addr_alias == test_addr);
}
