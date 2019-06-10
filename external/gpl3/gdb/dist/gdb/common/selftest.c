/* GDB self-testing.
   Copyright (C) 2016-2019 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "common-exceptions.h"
#include "common-debug.h"
#include "selftest.h"
#include <map>

namespace selftests
{
/* All the tests that have been registered.  Using an std::map allows keeping
   the order of tests stable and easily looking up whether a test name
   exists.  */

static std::map<std::string, std::unique_ptr<selftest>> tests;

/* A selftest that calls the test function without arguments.  */

struct simple_selftest : public selftest
{
  simple_selftest (self_test_function *function_)
  : function (function_)
  {}

  void operator() () const override
  {
    function ();
  }

  self_test_function *function;
};

/* See selftest.h.  */

void
register_test (const std::string &name, selftest *test)
{
  /* Check that no test with this name already exist.  */
  gdb_assert (tests.find (name) == tests.end ());

  tests[name] = std::unique_ptr<selftest> (test);
}

/* See selftest.h.  */

void
register_test (const std::string &name, self_test_function *function)
{
  register_test (name, new simple_selftest (function));
}

/* See selftest.h.  */

void
run_tests (const char *filter)
{
  int ran = 0, failed = 0;

  for (const auto &pair : tests)
    {
      const std::string &name = pair.first;
      const std::unique_ptr<selftest> &test = pair.second;

      if (filter != NULL && *filter != '\0'
	  && name.find (filter) == std::string::npos)
	continue;

      TRY
	{
	  debug_printf (_("Running selftest %s.\n"), name.c_str ());
	  ++ran;
	  (*test) ();
	}
      CATCH (ex, RETURN_MASK_ERROR)
	{
	  ++failed;
	  debug_printf ("Self test failed: %s\n", ex.message);
	}
      END_CATCH

      reset ();
    }

  debug_printf (_("Ran %d unit tests, %d failed\n"),
		ran, failed);
}

/* See selftest.h.  */

void for_each_selftest (for_each_selftest_ftype func)
{
  for (const auto &pair : tests)
    func (pair.first);
}

} // namespace selftests
