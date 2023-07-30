/* Self tests for string_view for GDB, the GNU debugger.

   Copyright (C) 2018-2023 Free Software Foundation, Inc.

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

/* No need to test string_view if we're using C++17, since we're going to use
   the "real" version.  */
#if __cplusplus < 201703L

#define GNULIB_NAMESPACE gnulib

#include "diagnostics.h"

/* Since this file uses GNULIB_NAMESPACE, some code defined in headers ends up
   using system functions rather than gnulib replacements.  This is not really
   a problem for this test, but it generates some warnings with Clang, silence
   them.  */
DIAGNOSTIC_PUSH
DIAGNOSTIC_IGNORE_USER_DEFINED_WARNINGS

#include "defs.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/gdb_string_view.h"

/* Used by the included .cc files below.  Included here because the
   included test files are wrapped in a namespace.  */
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

DIAGNOSTIC_POP

/* libstdc++'s testsuite uses VERIFY.  */
#define VERIFY SELF_CHECK

/* Used to disable testing features not supported by
   gdb::string_view.  */
#define GDB_STRING_VIEW

namespace selftests {
namespace string_view {

/* The actual tests live in separate files, which were originally
   copied over from libstdc++'s testsuite.  To preserve the structure
   and help with comparison with the original tests, the file names
   have been preserved, and only minimal modification was done to have
   them compile against gdb::string_view instead of std::string_view:

     - std::string_view->gdb::string_view, etc.
     - ATTRIBUTE_UNUSED in a few places
     - wrap each file in a namespace so they can all be compiled as a
       single unit.
     - libstdc++'s license and formatting style was preserved.
*/

#include "basic_string_view/capacity/1.cc"
#include "basic_string_view/cons/char/1.cc"
#include "basic_string_view/cons/char/2.cc"
#include "basic_string_view/cons/char/3.cc"
#include "basic_string_view/element_access/char/1.cc"
#include "basic_string_view/element_access/char/empty.cc"
#include "basic_string_view/element_access/char/front_back.cc"
#include "basic_string_view/inserters/char/2.cc"
#include "basic_string_view/modifiers/remove_prefix/char/1.cc"
#include "basic_string_view/modifiers/remove_suffix/char/1.cc"
#include "basic_string_view/modifiers/swap/char/1.cc"
#include "basic_string_view/operations/compare/char/1.cc"
#include "basic_string_view/operations/compare/char/13650.cc"
#include "basic_string_view/operations/copy/char/1.cc"
#include "basic_string_view/operations/data/char/1.cc"
#include "basic_string_view/operations/find/char/1.cc"
#include "basic_string_view/operations/find/char/2.cc"
#include "basic_string_view/operations/find/char/3.cc"
#include "basic_string_view/operations/find/char/4.cc"
#include "basic_string_view/operations/rfind/char/1.cc"
#include "basic_string_view/operations/rfind/char/2.cc"
#include "basic_string_view/operations/rfind/char/3.cc"
#include "basic_string_view/operations/substr/char/1.cc"
#include "basic_string_view/operators/char/2.cc"

static void
run_tests ()
{
  capacity_1::main ();
  cons_1::main ();
  cons_2::main ();
  cons_3::main ();
  element_access_1::main ();
  element_access_empty::main ();
  element_access_front_back::main ();
  inserters_2::main ();
  modifiers_remove_prefix::main ();
  modifiers_remove_suffix::main ();
  modifiers_swap::test01 ();
  operations_compare_1::main ();
  operations_compare_13650::main ();
  operations_copy_1::main ();
  operations_data_1::main ();
  operations_find_1::main ();
  operations_find_2::main ();
  operations_find_3::main ();
  operations_find_4::main ();
  operations_rfind_1::main ();
  operations_rfind_2::main ();
  operations_rfind_3::main ();
  operations_substr_1::main ();
  operators_2::main ();

  constexpr gdb::string_view sv_empty;
  SELF_CHECK (sv_empty.empty ());

  std::string std_string = "fika";
  gdb::string_view sv1 (std_string);
  SELF_CHECK (sv1 == "fika");

  constexpr const char *fika = "fika";
  gdb::string_view sv2 (fika);
  SELF_CHECK (sv2 == "fika");

  constexpr gdb::string_view sv3 (fika, 3);
  SELF_CHECK (sv3 == "fik");

  constexpr gdb::string_view sv4 (sv3);
  SELF_CHECK (sv4 == "fik");

  constexpr gdb::string_view::iterator it_begin = sv4.begin ();
  static_assert (*it_begin == 'f', "");

  constexpr gdb::string_view::iterator it_end = sv4.end ();
  static_assert (*it_end == 'a', "");

  const gdb::string_view::reverse_iterator it_rbegin = sv4.rbegin ();
  SELF_CHECK (*it_rbegin == 'k');

  const gdb::string_view::reverse_iterator it_rend = sv4.rend ();
  SELF_CHECK (*(it_rend - 1) == 'f');

  constexpr gdb::string_view::size_type size = sv4.size ();
  static_assert (size == 3, "");

  constexpr gdb::string_view::size_type length = sv4.length ();
  static_assert (length == 3, "");

  constexpr gdb::string_view::size_type max_size = sv4.max_size ();
  static_assert (max_size > 0, "");

  constexpr bool empty = sv4.empty ();
  static_assert (!empty, "");

  constexpr char c1 = sv4[1];
  static_assert (c1 == 'i', "");

  constexpr char c2 = sv4.at (2);
  static_assert (c2 == 'k', "");

  constexpr char front = sv4.front ();
  static_assert (front == 'f', "");

  constexpr char back = sv4.back ();
  static_assert (back == 'k', "");

  constexpr const char *data = sv4.data ();
  static_assert (data == fika, "");
}

} /* namespace string_view */
} /* namespace selftests */

#endif /* __cplusplus < 201703L */

void _initialize_string_view_selftests ();
void
_initialize_string_view_selftests ()
{
#if defined(GDB_STRING_VIEW)
  selftests::register_test ("string_view", selftests::string_view::run_tests);
#endif
}
