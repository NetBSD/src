/* GDB self-testing.
   Copyright (C) 2016 Free Software Foundation, Inc.

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

#ifndef SELFTEST_H
#define SELFTEST_H

/* A test is just a function that does some checks and throws an
   exception if something has gone wrong.  */

typedef void self_test_function (void);

/* Register a new self-test.  */

extern void register_self_test (self_test_function *function);

/* Run all the self tests.  This print a message describing the number
   of test and the number of failures.  */

extern void run_self_tests (void);

/* Check that VALUE is true, and, if not, throw an exception.  */

#define SELF_CHECK(VALUE)						\
  do {									\
    if (!(VALUE))							\
      error (_("self-test failed at %s:%d"), __FILE__, __LINE__);	\
  } while (0)

#endif /* SELFTEST_H */
