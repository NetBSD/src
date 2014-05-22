/* Test file for mpfr_version.

Copyright 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
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

#include <stdlib.h>

#include "mpfr-test.h"

int
main (void)
{
  char buffer[256];

#ifdef __MPIR_VERSION
  printf ("[tversion] MPIR: header %d.%d.%d, library %s\n",
          __MPIR_VERSION, __MPIR_VERSION_MINOR, __MPIR_VERSION_PATCHLEVEL,
          mpir_version);
#else
  printf ("[tversion] GMP: header %d.%d.%d, library %s\n",
          __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL,
          gmp_version);
#endif
  printf ("[tversion] MPFR tuning parameters from %s\n", MPFR_TUNE_CASE);

  /* Test the MPFR version. */
  test_version ();

  sprintf (buffer, "%d.%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
           __GNU_MP_VERSION_PATCHLEVEL);
  if (strcmp (buffer, gmp_version) == 0)
    return 0;
  if (__GNU_MP_VERSION_PATCHLEVEL == 0)
    {
      sprintf (buffer, "%d.%d", __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR);
      if (strcmp (buffer, gmp_version) == 0)
        return 0;
    }

  /* In some cases, it may be acceptable to have different versions for
     the header and the library, in particular when shared libraries are
     used (e.g., after a bug-fix upgrade of the library, and versioning
     ensures that this can be done only when the binary interface is
     compatible). However, when recompiling software like here, this
     should never happen (except if GMP has been upgraded between two
     "make check" runs, but there's no reason for that). A difference
     between the versions of gmp.h and libgmp probably indicates either
     a bad configuration or some other inconsistency in the development
     environment, and it is better to fail (in particular for automatic
     installations). */
  printf ("ERROR! The versions of gmp.h (%s) and libgmp (%s) do not "
          "match.\nThe possible causes are:\n", buffer, gmp_version);
  printf ("  * A bad configuration in your include/library search paths.\n"
          "  * An inconsistency in the include/library search paths of\n"
          "    your development environment; an example:\n"
          "      http://gcc.gnu.org/ml/gcc-help/2010-11/msg00359.html\n"
          "  * GMP has been upgraded after the first \"make check\".\n"
          "    In such a case, try again after a \"make clean\".\n"
          "  * A new or non-standard version naming is used in GMP.\n"
          "    In this case, a patch may already be available on the\n"
          "    MPFR web site.  Otherwise please report the problem.\n");
  printf ("In the first two cases, this may lead to errors, in particular"
          " with MPFR.\nIf some other tests fail, please solve that"
          " problem first.\n");

  return 1;
}
