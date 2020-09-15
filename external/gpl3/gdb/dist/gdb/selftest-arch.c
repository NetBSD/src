/* GDB self-test for each gdbarch.
   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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

#if GDB_SELF_TEST
#include "gdbsupport/selftest.h"
#include "selftest-arch.h"
#include "arch-utils.h"

namespace selftests {

/* A kind of selftest that calls the test function once for each gdbarch known
   to GDB.  */

struct gdbarch_selftest : public selftest
{
  gdbarch_selftest (self_test_foreach_arch_function *function_)
  : function (function_)
  {}

  void operator() () const override
  {
    const char **arches = gdbarch_printable_names ();
    bool pass = true;

    for (int i = 0; arches[i] != NULL; i++)
      {
	if (strcmp ("fr300", arches[i]) == 0)
	  {
	    /* PR 20946 */
	    continue;
	  }
	else if (strcmp ("powerpc:EC603e", arches[i]) == 0
		 || strcmp ("powerpc:e500mc", arches[i]) == 0
		 || strcmp ("powerpc:e500mc64", arches[i]) == 0
		 || strcmp ("powerpc:titan", arches[i]) == 0
		 || strcmp ("powerpc:vle", arches[i]) == 0
		 || strcmp ("powerpc:e5500", arches[i]) == 0
		 || strcmp ("powerpc:e6500", arches[i]) == 0)
	  {
	    /* PR 19797 */
	    continue;
	  }

	QUIT;

	try
	  {
	    struct gdbarch_info info;

	    gdbarch_info_init (&info);
	    info.bfd_arch_info = bfd_scan_arch (arches[i]);

	    struct gdbarch *gdbarch = gdbarch_find_by_info (info);
	    SELF_CHECK (gdbarch != NULL);

	    function (gdbarch);
	  }
	catch (const gdb_exception_error &ex)
	  {
	    pass = false;
	    exception_fprintf (gdb_stderr, ex,
			       _("Self test failed: arch %s: "), arches[i]);
	  }

	reset ();
      }

    SELF_CHECK (pass);
  }

  self_test_foreach_arch_function *function;
};

void
register_test_foreach_arch (const std::string &name,
			    self_test_foreach_arch_function *function)
{
  register_test (name, new gdbarch_selftest (function));
}

void
reset ()
{
  /* Clear GDB internal state.  */
  registers_changed ();
  reinit_frame_cache ();
}
} // namespace selftests
#endif /* GDB_SELF_TEST */
