/* Self tests for GDB's argument splitting and merging.

   Copyright (C) 2023-2025 Free Software Foundation, Inc.

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
#include "gdbsupport/buildargv.h"
#include "gdbsupport/common-inferior.h"
#include "gdbsupport/remote-args.h"
#include "gdbsupport/gdb_argv_vec.h"

namespace selftests {
namespace remote_args_tests {

/* The data needed to perform a single remote argument test.  */
struct arg_test_desc
{
  /* The original inferior argument string.  */
  std::string input;

  /* The individual arguments once they have been split.  */
  std::vector<std::string> split;

  /* The new inferior argument string, created by joining SPLIT.  */
  std::string joined;
};

/* The list of tests.  */
arg_test_desc desc[] = {
  { "abc", { "abc" }, "abc" },
  { "a b c", { "a", "b", "c" }, "a b c" },
  { "\"a b\" 'c d'", { "a b", "c d" }, "a\\ b c\\ d" },
  { "\\' \\\"", { "'", "\"" }, "\\' \\\"" },
  { "'\\'", { "\\" }, "\\\\" },
  { "\"\\\\\" \"\\\\\\\"\"", { "\\", "\\\"" }, "\\\\ \\\\\\\"" },
  { "\\  \" \" ' '", { " ", " ", " "}, "\\  \\  \\ " },
  { "\"'\"", { "'" }, "\\'" },
  { "'\"' '\\\"'", { "\"", "\\\"" } , "\\\" \\\\\\\""},
  { "\"first arg\" \"\" \"third-arg\" \"'\" \"\\\"\" \"\\\\\\\"\" \" \" \"\"",
    { "first arg", "", "third-arg", "'", "\"", "\\\""," ", "" },
    "first\\ arg '' third-arg \\' \\\" \\\\\\\" \\  ''"},
  { "\"\\a\" \"\\&\" \"\\#\" \"\\<\" \"\\^\"",
    { "\\a", "\\&", "\\#" , "\\<" , "\\^"},
    "\\\\a \\\\\\& \\\\\\# \\\\\\< \\\\\\^" },
  { "1 '\n' 3", { "1", "\n", "3" }, "1 '\n' 3" },
};

/* Run the remote argument passing self tests.  */

static void
self_test ()
{
  int failure_count = 0;
  for (const auto &d : desc)
    {
      if (run_verbose ())
	{
	  if (&d != &desc[0])
	    debug_printf ("------------------------------\n");
	  debug_printf ("Input (%s)\n", d.input.c_str ());
	}

      /* Split argument string into individual arguments.  */
      std::vector<std::string> split_args = gdb::remote_args::split (d.input);

      if (run_verbose ())
	{
	  debug_printf ("Split:\n");

	  size_t len = std::max (split_args.size (), d.split.size ());
	  for (size_t i = 0; i < len; ++i)
	    {
	      const char *got = "N/A";
	      const char *expected = got;

	      if (i < split_args.size ())
		got = split_args[i].c_str ();

	      if (i < d.split.size ())
		expected = d.split[i].c_str ();

	      debug_printf ("  got (%s), expected (%s)\n", got, expected);
	    }
	}

      if (split_args != d.split)
	{
	  ++failure_count;
	  if (run_verbose ())
	    debug_printf ("FAIL\n");
	  continue;
	}

      /* Now join the arguments.  */
      gdb::argv_vec split_args_c_str;
      for (const std::string &s : split_args)
	split_args_c_str.push_back (xstrdup (s.c_str ()));
      std::string joined_args
	= gdb::remote_args::join (split_args_c_str.get ());

      if (run_verbose ())
	debug_printf ("Joined (%s), expected (%s)\n",
		      joined_args.c_str (), d.joined.c_str ());

      if (joined_args != d.joined)
	{
	  ++failure_count;
	  if (run_verbose ())
	    debug_printf ("FAIL\n");
	  continue;
	}

      /* The contents of JOINED_ARGS will not be identical to D.INPUT.
	 There are multiple ways that an argument can be escaped, and our
	 join function just picks one.  However, if we split JOINED_ARGS
	 again then each individual argument should be the same as those in
	 SPLIT_ARGS.  So test that next.  */
      std::vector<std::string> split_args_v2
	= gdb::remote_args::split (joined_args);

      if (split_args_v2 != split_args)
	{
	  ++failure_count;
	  if (run_verbose ())
	    {
	      debug_printf ("Re-split:\n");
	      for (const auto &a : split_args_v2)
		debug_printf ("  got (%s)\n", a.c_str ());
	      debug_printf ("FAIL\n");
	    }
	  continue;
	}

      if (run_verbose ())
	debug_printf ("PASS\n");
    }

  SELF_CHECK (failure_count == 0);
}

} /* namespace remote_args_tests */
} /* namespace selftests */

INIT_GDB_FILE (remote_arg_selftests)
{
  selftests::register_test ("remote-args",
			    selftests::remote_args_tests::self_test);
}
