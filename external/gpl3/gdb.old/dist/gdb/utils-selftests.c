/* Self tests for general utility routines for GDB, the GNU debugger.

   Copyright (C) 2016-2017 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "selftest.h"

#if GDB_SELF_TEST

namespace selftests {

/* common-utils self tests.  Defined here instead of in
   common/common-utils.c because that file is shared with
   gdbserver.  */

static void
common_utils_tests (void)
{
  SELF_CHECK (string_printf ("%s", "") == "");
  SELF_CHECK (string_printf ("%d comes before 2", 1) == "1 comes before 2");
  SELF_CHECK (string_printf ("hello %s", "world") == "hello world");

#define X10 "0123456789"
#define X100 X10 X10 X10 X10 X10 X10 X10 X10 X10 X10
#define X1000 X100 X100 X100 X100 X100 X100 X100 X100 X100 X100
#define X10000 X1000 X1000 X1000 X1000 X1000 X1000 X1000 X1000 X1000 X1000
#define X100000 X10000 X10000 X10000 X10000 X10000 X10000 X10000 X10000 X10000 X10000
  SELF_CHECK (string_printf ("%s", X10) == X10);
  SELF_CHECK (string_printf ("%s", X100) == X100);
  SELF_CHECK (string_printf ("%s", X1000) == X1000);
  SELF_CHECK (string_printf ("%s", X10000) == X10000);
  SELF_CHECK (string_printf ("%s", X100000) == X100000);
}

} /* namespace selftests */

#endif

void
_initialize_utils_selftests (void)
{
#if GDB_SELF_TEST
  register_self_test (selftests::common_utils_tests);
#endif
}
