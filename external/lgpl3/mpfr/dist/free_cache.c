/* mpfr_free_cache - Free the cache used by MPFR for internal consts.

Copyright 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
Contributed by the Arenaire and Cacao projects, INRIA.

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

#include "mpfr-impl.h"

#if 0
static void
free_l2b (void)
{
  int i, b;

  for (b = 2; b <= BASE_MAX; b++)
    for (i = 0; i < 2; i++)
      {
        mpfr_ptr p = __gmpfr_l2b[b-2][i];
        if (p != NULL)
          {
            mpfr_clear (p);
            (*__gmp_free_func) (p, sizeof (mpfr_t));
          }
      }
}
#endif

void
mpfr_free_cache (void)
{
  mpfr_clear_cache (__gmpfr_cache_const_pi);
  mpfr_clear_cache (__gmpfr_cache_const_log2);
  mpfr_clear_cache (__gmpfr_cache_const_euler);
  mpfr_clear_cache (__gmpfr_cache_const_catalan);
  /* free_l2b (); */
}
