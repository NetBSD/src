/* tabort_defalloc2 -- Test for abort due to reaching out of memory

Copyright 2012-2018 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#define MPFR_NEED_LONGLONG_H
#include "mpfr-test.h"

/* Don't test with mini-gmp, which does not fully implement GMP
 * memory functions.
 *
 * Don't test if GCC's AddressSanitizer is used because it reports
 * the error before GMP can do the abort.
 */

#if !defined(__SANITIZE_ADDRESS__)

int
main (int argc, char **argv)
{
  void *ptr;

  tests_start_mpfr ();
  tests_expect_abort ();

  printf ("[tabort_defalloc2] Check for good handling of abort"
          " in memory function.\n");
  ptr = mpfr_allocate_func (128);
  ptr = mpfr_reallocate_func (ptr, 128, (size_t) -1);

  tests_end_mpfr ();
  return -1; /* Should not be executed */
}

#else

int
main (void)
{
  return 77;
}

#endif
