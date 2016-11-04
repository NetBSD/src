// ver_test.h -- a test case for gold

// Copyright (C) 2007-2015 Free Software Foundation, Inc.
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

#ifdef USE_TRACE
#include <cstdio>
#define TRACE printf("In %s, %s()\n", __FILE__, __FUNCTION__);
#else
#define TRACE
#endif

extern bool t1();
extern bool t2();
extern bool t3();
extern bool t4();

extern "C" {

extern int t1_2();
extern int t2_2();
extern int t3_2();
extern int t4_2();
extern int t4_2a();

}
