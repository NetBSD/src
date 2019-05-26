/* Self tests for gdbarch for GDB, the GNU debugger.

   Copyright (C) 2017-2019 Free Software Foundation, Inc.

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
#include "common/selftest.h"
#include "selftest-arch.h"
#include "inferior.h"
#include "gdbthread.h"
#include "target.h"
#include "test-target.h"
#include "target-float.h"
#include "common/def-vector.h"

namespace selftests {

/* Test gdbarch methods register_to_value and value_to_register.  */

static void
register_to_value_test (struct gdbarch *gdbarch)
{
  const struct builtin_type *builtin = builtin_type (gdbarch);
  struct type *types[] =
    {
      builtin->builtin_void,
      builtin->builtin_char,
      builtin->builtin_short,
      builtin->builtin_int,
      builtin->builtin_long,
      builtin->builtin_signed_char,
      builtin->builtin_unsigned_short,
      builtin->builtin_unsigned_int,
      builtin->builtin_unsigned_long,
      builtin->builtin_float,
      builtin->builtin_double,
      builtin->builtin_long_double,
      builtin->builtin_complex,
      builtin->builtin_double_complex,
      builtin->builtin_string,
      builtin->builtin_bool,
      builtin->builtin_long_long,
      builtin->builtin_unsigned_long_long,
      builtin->builtin_int8,
      builtin->builtin_uint8,
      builtin->builtin_int16,
      builtin->builtin_uint16,
      builtin->builtin_int32,
      builtin->builtin_uint32,
      builtin->builtin_int64,
      builtin->builtin_uint64,
      builtin->builtin_int128,
      builtin->builtin_uint128,
      builtin->builtin_char16,
      builtin->builtin_char32,
    };

  /* Error out if debugging something, because we're going to push the
     test target, which would pop any existing target.  */
  if (current_top_target ()->stratum () >= process_stratum)
   error (_("target already pushed"));

  /* Create a mock environment.  An inferior with a thread, with a
     process_stratum target pushed.  */

  test_target_ops mock_target;
  ptid_t mock_ptid (1, 1);
  inferior mock_inferior (mock_ptid.pid ());
  address_space mock_aspace {};
  mock_inferior.gdbarch = gdbarch;
  mock_inferior.aspace = &mock_aspace;
  thread_info mock_thread (&mock_inferior, mock_ptid);

  scoped_restore restore_thread_list
    = make_scoped_restore (&mock_inferior.thread_list, &mock_thread);

  /* Add the mock inferior to the inferior list so that look ups by
     target+ptid can find it.  */
  scoped_restore restore_inferior_list
    = make_scoped_restore (&inferior_list);
  inferior_list = &mock_inferior;

  /* Switch to the mock inferior.  */
  scoped_restore_current_inferior restore_current_inferior;
  set_current_inferior (&mock_inferior);

  /* Push the process_stratum target so we can mock accessing
     registers.  */
  push_target (&mock_target);

  /* Pop it again on exit (return/exception).  */
  SCOPE_EXIT { pop_all_targets_at_and_above (process_stratum); };

  /* Switch to the mock thread.  */
  scoped_restore restore_inferior_ptid
    = make_scoped_restore (&inferior_ptid, mock_ptid);

  struct frame_info *frame = get_current_frame ();
  const int num_regs = gdbarch_num_cooked_regs (gdbarch);

  /* Test gdbarch methods register_to_value and value_to_register with
     different combinations of register numbers and types.  */
  for (const auto &type : types)
    {
      for (auto regnum = 0; regnum < num_regs; regnum++)
	{
	  if (gdbarch_convert_register_p (gdbarch, regnum, type))
	    {
	      std::vector<gdb_byte> expected (TYPE_LENGTH (type), 0);

	      if (TYPE_CODE (type) == TYPE_CODE_FLT)
		{
		  /* Generate valid float format.  */
		  target_float_from_string (expected.data (), type, "1.25");
		}
	      else
		{
		  for (auto j = 0; j < expected.size (); j++)
		    expected[j] = (regnum + j) % 16;
		}

	      gdbarch_value_to_register (gdbarch, frame, regnum, type,
					 expected.data ());

	      /* Allocate two bytes more for overflow check.  */
	      std::vector<gdb_byte> buf (TYPE_LENGTH (type) + 2, 0);
	      int optim, unavail, ok;

	      /* Set the fingerprint in the last two bytes.  */
	      buf [TYPE_LENGTH (type)]= 'w';
	      buf [TYPE_LENGTH (type) + 1]= 'l';
	      ok = gdbarch_register_to_value (gdbarch, frame, regnum, type,
					      buf.data (), &optim, &unavail);

	      SELF_CHECK (ok);
	      SELF_CHECK (!optim);
	      SELF_CHECK (!unavail);

	      SELF_CHECK (buf[TYPE_LENGTH (type)] == 'w');
	      SELF_CHECK (buf[TYPE_LENGTH (type) + 1] == 'l');

	      for (auto k = 0; k < TYPE_LENGTH(type); k++)
		SELF_CHECK (buf[k] == expected[k]);
	    }
	}
    }
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

void
_initialize_gdbarch_selftests (void)
{
#if GDB_SELF_TEST
  selftests::register_test_foreach_arch ("register_to_value",
					 selftests::register_to_value_test);
#endif
}
