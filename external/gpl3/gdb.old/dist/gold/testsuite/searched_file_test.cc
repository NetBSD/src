// searched_file_test.cc -- test -l:foo.a for gold

// Copyright (C) 2009-2020 Free Software Foundation, Inc.
// Written by Chris Demetriou <cgd@google.com>.

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

// The Linux kernel builds an object file using a linker script, and
// then links against that object file using the -R option.  This is a
// test for that usage.

#include <cstdlib>

extern int zero_from_lib;
        
int
main(int, char**)
{
  exit(zero_from_lib);
}

