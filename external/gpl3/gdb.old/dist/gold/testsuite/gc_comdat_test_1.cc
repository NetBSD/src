// gc_comdat_test_1.cc -- a test case for gold

// Copyright (C) 2009-2020 Free Software Foundation, Inc.
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

// The goal of this program is to verify if comdat's and garbage 
// collection work together.  This file is compiled with -g.  The
// comdat kept function for GetMax is garbage.

int foo();
template <class T>
T GetMax (T a, T b)
{
  return (a > b)?a:b;
}

int bar ()
{
  return GetMax <int> (4,5);
}

int main()
{
  return 0;
}
