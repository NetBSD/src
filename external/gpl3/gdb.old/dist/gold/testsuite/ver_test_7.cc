// ver_test_7.cc -- test weak duplicate symbol with version

// Copyright (C) 2008-2020 Free Software Foundation, Inc.
// Written by Ian Lance Taylor <iant@google.com>

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

// This tests having a weak symbol which matches an entry in the
// version script following a hidden definition from .symver.  There
// was a bug in which the weak symbol would cause the earlier symbol
// to become globally visible when it should have been hidden.

extern "C" int t2_2() __attribute__ ((weak));

extern "C"
int
t2_2()
{
  return 23;
}
