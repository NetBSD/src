/* Test file for multiple mpfr.h inclusion and intmax_t related functions

Copyright 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
Contributed by the AriC and Caramel projects, INRIA.

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

#if HAVE_STDINT_H

#if _MPFR_EXP_FORMAT == 4
/* If mpfr_exp_t is defined as intmax_t, intmax_t must be defined before
   the inclusion of mpfr.h (this test doesn't use mpfr-impl.h). */
# include <stdint.h>
#endif

/* Assume that this is in fact a header inclusion for some library
   that uses MPFR, i.e. this inclusion is hidden in another one.
   MPFR currently (rev 6704) fails to handle this case. */
#include <mpfr.h>

#include <stdint.h>

#define MPFR_USE_INTMAX_T
#include <mpfr.h>

int
main (void)
{
  mpfr_t x;
  intmax_t j;

  mpfr_init_set_ui (x, 1, MPFR_RNDN);
  j = mpfr_get_uj (x, MPFR_RNDN);
  mpfr_clear (x);
  return j == 1 ? 0 : 1;
}

#else  /* HAVE_STDINT_H */

/* The test is disabled. */

int
main (void)
{
  return 77;
}

#endif  /* HAVE_STDINT_H */
