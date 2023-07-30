/* Self tests for optional for GDB, the GNU debugger.

   Copyright (C) 2017-2023 Free Software Foundation, Inc.

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
#include "gdbsupport/selftest.h"
#include "gdbsupport/gdb_optional.h"

/* Used by the included .cc files below.  Included here because the
   included test files are wrapped in a namespace.  */
#include <vector>
#include <string>
#include <memory>

/* libstdc++'s testsuite uses VERIFY.  */
#define VERIFY SELF_CHECK

/* Used to disable testing features not supported by
   gdb::optional.  */
#define GDB_OPTIONAL

namespace selftests {
namespace optional {

/* The actual tests live in separate files, which were originally
   copied over from libstdc++'s testsuite.  To preserve the structure
   and help with comparison with the original tests, the file names
   have been preserved, and only minimal modification was done to have
   them compile against gdb::optional instead of std::optional:

     - std::optional->gdb:optional, etc.
     - ATTRIBUTE_UNUSED in a few places
     - wrap each file in a namespace so they can all be compiled as a
       single unit.
     - libstdc++'s license and formatting style was preserved.
*/

#include "optional/assignment/1.cc"
#include "optional/assignment/2.cc"
#include "optional/assignment/3.cc"
#include "optional/assignment/4.cc"
#include "optional/assignment/5.cc"
#include "optional/assignment/6.cc"
#include "optional/assignment/7.cc"
#include "optional/cons/copy.cc"
#include "optional/cons/default.cc"
#include "optional/cons/move.cc"
#include "optional/cons/value.cc"
#include "optional/in_place.cc"
#include "optional/observers/1.cc"
#include "optional/observers/2.cc"

static void
run_tests ()
{
  assign_1::test ();
  assign_2::test ();
  assign_3::test ();
  assign_4::test ();
  assign_5::test ();
  assign_6::test ();
  assign_7::test ();
  cons_copy::test ();
  cons_default::test ();
  cons_move::test ();
  cons_value::test ();
  in_place::test ();
  observers_1::test ();
  observers_2::test ();
}

} /* namespace optional */
} /* namespace selftests */

void _initialize_optional_selftests ();
void
_initialize_optional_selftests ()
{
  selftests::register_test ("optional", selftests::optional::run_tests);
}
