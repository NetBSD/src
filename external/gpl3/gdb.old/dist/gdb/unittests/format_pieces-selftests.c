/* Self tests for format_pieces for GDB, the GNU debugger.

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
#include "common/format.h"
#include "common/selftest.h"

namespace selftests {
namespace format_pieces {

/* Verify that parsing STR gives pieces equal to EXPECTED_PIECES.  */

static void
check (const char *str, const std::vector<format_piece> &expected_pieces)
{
  ::format_pieces pieces (&str);

  SELF_CHECK ((pieces.end () - pieces.begin ()) == expected_pieces.size ());
  SELF_CHECK (std::equal (pieces.begin (), pieces.end (),
			  expected_pieces.begin ()));
}

static void
test_escape_sequences ()
{
  check ("This is an escape sequence: \\e",
    {
      format_piece ("This is an escape sequence: \e", literal_piece),
    });
}

static void
test_format_specifier ()
{
  check ("Hello %d%llx%%d", /* ARI: %ll */
    {
      format_piece ("Hello ", literal_piece),
      format_piece ("%d", int_arg),
      format_piece ("", literal_piece),
      format_piece ("%llx", long_long_arg), /* ARI: %ll */
      format_piece ("%%d", literal_piece),
    });
}

static void
run_tests ()
{
  test_escape_sequences ();
  test_format_specifier ();
}

} /* namespace format_pieces */
} /* namespace selftests */

void
_initialize_format_pieces_selftests ()
{
  selftests::register_test ("format_pieces",
			    selftests::format_pieces::run_tests);
}
