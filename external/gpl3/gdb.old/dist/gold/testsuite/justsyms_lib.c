// justsyms_lib.cc -- test --just-symbols for gold

// Copyright (C) 2011-2020 Free Software Foundation, Inc.
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

// This test goes with justsyms_exec.cc.  We compile this file, then
// link it into an executable image with -Ttext and -Tdata to set
// the starting addresses for each segment.

int exported_func(void);

int exported_data = 0x1000;

int
exported_func(void)
{
  return 1;
}
