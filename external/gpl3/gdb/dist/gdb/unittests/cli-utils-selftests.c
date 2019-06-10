/* Unit tests for the cli-utils.c file.

   Copyright (C) 2018-2019 Free Software Foundation, Inc.

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
#include "cli/cli-utils.h"
#include "common/selftest.h"

namespace selftests {
namespace cli_utils {

static void
test_number_or_range_parser ()
{
  /* Test parsing a simple integer.  */
  {
    number_or_range_parser one ("1");

    SELF_CHECK (!one.finished ());
    SELF_CHECK (one.get_number () == 1);
    SELF_CHECK (one.finished ());
    SELF_CHECK (strcmp (one.cur_tok (), "") == 0);
  }

  /* Test parsing an integer followed by a non integer.  */
  {
    number_or_range_parser one_after ("1 after");

    SELF_CHECK (!one_after.finished ());
    SELF_CHECK (one_after.get_number () == 1);
    SELF_CHECK (one_after.finished ());
    SELF_CHECK (strcmp (one_after.cur_tok (), "after") == 0);
  }

  /* Test parsing a range.  */
  {
    number_or_range_parser one_three ("1-3");

    for (int i = 1; i < 4; i++)
      {
	SELF_CHECK (!one_three.finished ());
	SELF_CHECK (one_three.get_number () == i);
      }
    SELF_CHECK (one_three.finished ());
    SELF_CHECK (strcmp (one_three.cur_tok (), "") == 0);
  }

  /* Test parsing a range followed by a non-integer.  */
  {
    number_or_range_parser one_three_after ("1-3 after");

    for (int i = 1; i < 4; i++)
      {
	SELF_CHECK (!one_three_after.finished ());
	SELF_CHECK (one_three_after.get_number () == i);
      }
    SELF_CHECK (one_three_after.finished ());
    SELF_CHECK (strcmp (one_three_after.cur_tok (), "after") == 0);
  }

  /* Test a negative integer gives an error.  */
  {
    number_or_range_parser minus_one ("-1");

    SELF_CHECK (!minus_one.finished ());
    TRY
      {
	minus_one.get_number ();
	SELF_CHECK (false);
      }
    CATCH (ex, RETURN_MASK_ERROR)
      {
	SELF_CHECK (ex.reason == RETURN_ERROR);
	SELF_CHECK (ex.error == GENERIC_ERROR);
	SELF_CHECK (strcmp (ex.message, "negative value") == 0);
	SELF_CHECK (strcmp (minus_one.cur_tok (), "-1") == 0);
      }
    END_CATCH;
  }

  /* Test that a - followed by not a number does not give an error.  */
  {
    number_or_range_parser nan ("-whatever");

    SELF_CHECK (nan.finished ());
    SELF_CHECK (strcmp (nan.cur_tok (), "-whatever") == 0);
  }
}

static void
test_parse_flags ()
{
  const char *flags = "abc";
  const char *non_flags_args = "non flags args";

  /* Extract twice the same flag, separated by one space.  */
  {
    const char *t1 = "-a -a non flags args";

    SELF_CHECK (parse_flags (&t1, flags) == 1);
    SELF_CHECK (parse_flags (&t1, flags) == 1);
    SELF_CHECK (strcmp (t1, non_flags_args) == 0);
  }

  /* Extract some flags, separated by one or more spaces.  */
  {
    const char *t2 = "-c     -b -c  -b -c    non flags args";

    SELF_CHECK (parse_flags (&t2, flags) == 3);
    SELF_CHECK (parse_flags (&t2, flags) == 2);
    SELF_CHECK (parse_flags (&t2, flags) == 3);
    SELF_CHECK (parse_flags (&t2, flags) == 2);
    SELF_CHECK (parse_flags (&t2, flags) == 3);
    SELF_CHECK (strcmp (t2, non_flags_args) == 0);
  }

  /* Check behaviour where there is no flag to extract.  */
  {
    const char *t3 = non_flags_args;

    SELF_CHECK (parse_flags (&t3, flags) == 0);
    SELF_CHECK (strcmp (t3, non_flags_args) == 0);
  }

  /* Extract 2 known flags in front of unknown flags.  */
  {
    const char *t4 = "-c -b -x -y -z -c";

    SELF_CHECK (parse_flags (&t4, flags) == 3);
    SELF_CHECK (parse_flags (&t4, flags) == 2);
    SELF_CHECK (strcmp (t4, "-x -y -z -c") == 0);
    SELF_CHECK (parse_flags (&t4, flags) == 0);
    SELF_CHECK (strcmp (t4, "-x -y -z -c") == 0);
  }

  /* Check combined flags are not recognised.  */
  {
    const char *t5 = "-c -cb -c";

    SELF_CHECK (parse_flags (&t5, flags) == 3);
    SELF_CHECK (parse_flags (&t5, flags) == 0);
    SELF_CHECK (strcmp (t5, "-cb -c") == 0);
  }
}

static void
test_parse_flags_qcs ()
{
  const char *non_flags_args = "non flags args";

  /* Test parsing of 2 flags out of the known 3.  */
  {
    const char *t1 = "-q -s    non flags args";
    qcs_flags flags;

    SELF_CHECK (parse_flags_qcs ("test_parse_flags_qcs.t1.q",
				 &t1,
				 &flags) == 1);
    SELF_CHECK (flags.quiet && !flags.cont && !flags.silent);
    SELF_CHECK (parse_flags_qcs ("test_parse_flags_qcs.t1.s",
				 &t1,
				 &flags) == 1);
    SELF_CHECK (flags.quiet && !flags.cont && flags.silent);
    SELF_CHECK (strcmp (t1, non_flags_args) == 0);
  }

  /* Test parsing when there is no flag.  */
  {
    const char *t2 = "non flags args";
    qcs_flags flags;

    SELF_CHECK (parse_flags_qcs ("test_parse_flags_qcs.t2",
				 &t2,
				 &flags) == 0);
    SELF_CHECK (!flags.quiet && !flags.cont && !flags.silent);
    SELF_CHECK (strcmp (t2, non_flags_args) == 0);
  }

  /* Test parsing stops at a negative integer.  */
  {
    const char *t3 = "-123 non flags args";
    const char *orig_t3 = t3;
    qcs_flags flags;

    SELF_CHECK (parse_flags_qcs ("test_parse_flags_qcs.t3",
				 &t3,
				 &flags) == 0);
    SELF_CHECK (!flags.quiet && !flags.cont && !flags.silent);
    SELF_CHECK (strcmp (t3, orig_t3) == 0);
  }

  /* Test mutual exclusion between -c and -s.  */
  {
    const char *t4 = "-c -s non flags args";
    qcs_flags flags;

    TRY
      {
	SELF_CHECK (parse_flags_qcs ("test_parse_flags_qcs.t4.cs",
				     &t4,
				     &flags) == 1);

	(void) parse_flags_qcs ("test_parse_flags_qcs.t4.cs",
				&t4,
				&flags);
	SELF_CHECK (false);
      }
    CATCH (ex, RETURN_MASK_ERROR)
      {
	SELF_CHECK (ex.reason == RETURN_ERROR);
	SELF_CHECK (ex.error == GENERIC_ERROR);
	SELF_CHECK
	  (strcmp (ex.message,
		   "test_parse_flags_qcs.t4.cs: "
		   "-c and -s are mutually exclusive") == 0);
      }
    END_CATCH;
  }

}

static void
test_cli_utils ()
{
  selftests::cli_utils::test_number_or_range_parser ();
  selftests::cli_utils::test_parse_flags ();
  selftests::cli_utils::test_parse_flags_qcs ();
}

}
}

void
_initialize_cli_utils_selftests ()
{
  selftests::register_test ("cli_utils",
			    selftests::cli_utils::test_cli_utils);
}
