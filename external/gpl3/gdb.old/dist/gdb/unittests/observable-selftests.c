/* Self tests for gdb::observers, GDB notifications to observers.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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
#include "gdbsupport/observable.h"

namespace selftests {
namespace observers {

gdb::observers::observable<int> test_notification ("test_notification");

static int test_first_observer = 0;
static int test_second_observer = 0;
static int test_third_observer = 0;

static void
test_first_notification_function (int arg)
{
  test_first_observer++;
}

static void
test_second_notification_function (int arg)
{
  test_second_observer++;
}

static void
test_third_notification_function (int arg)
{
  test_third_observer++;
}

static void
notify_check_counters (int one, int two, int three)
{
  /* Reset.  */
  test_first_observer = 0;
  test_second_observer = 0;
  test_third_observer = 0;
  /* Notify.  */
  test_notification.notify (0);
  /* Check.  */
  SELF_CHECK (one == test_first_observer);
  SELF_CHECK (two == test_second_observer);
  SELF_CHECK (three == test_third_observer);
}

static void
run_tests ()
{
  /* First, try sending a notification without any observer
     attached.  */
  notify_check_counters (0, 0, 0);

  const gdb::observers::token token1 {}, token2 {} , token3 {};

  /* Now, attach one observer, and send a notification.  */
  test_notification.attach (&test_second_notification_function, token2);
  notify_check_counters (0, 1, 0);

  /* Remove the observer, and send a notification.  */
  test_notification.detach (token2);
  notify_check_counters (0, 0, 0);

  /* With a new observer.  */
  test_notification.attach (&test_first_notification_function, token1);
  notify_check_counters (1, 0, 0);

  /* With 2 observers.  */
  test_notification.attach (&test_second_notification_function, token2);
  notify_check_counters (1, 1, 0);

  /* With 3 observers.  */
  test_notification.attach (&test_third_notification_function, token3);
  notify_check_counters (1, 1, 1);

  /* Remove middle observer.  */
  test_notification.detach (token2);
  notify_check_counters (1, 0, 1);

  /* Remove first observer.  */
  test_notification.detach (token1);
  notify_check_counters (0, 0, 1);

  /* Remove last observer.  */
  test_notification.detach (token3);
  notify_check_counters (0, 0, 0);

  /* Go back to 3 observers, and remove them in a different
     order...  */
  test_notification.attach (&test_first_notification_function, token1);
  test_notification.attach (&test_second_notification_function, token2);
  test_notification.attach (&test_third_notification_function, token3);
  notify_check_counters (1, 1, 1);

  /* Remove the third observer.  */
  test_notification.detach (token3);
  notify_check_counters (1, 1, 0);

  /* Remove the second observer.  */
  test_notification.detach (token2);
  notify_check_counters (1, 0, 0);

  /* Remove first observer, no more observers.  */
  test_notification.detach (token1);
  notify_check_counters (0, 0, 0);
}

} /* namespace observers */
} /* namespace selftests */

void _initialize_observer_selftest ();
void
_initialize_observer_selftest ()
{
  selftests::register_test ("gdb::observers",
			    selftests::observers::run_tests);
}
