/* MPFR internal header related to Static Assertions

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

#ifndef __MPFR_STATIC_ASSERT_H__
#define __MPFR_STATIC_ASSERT_H__

#include "mpfr-cvers.h"  /* for __MPFR_GNUC */

/* How to use:
   ===========
   MPFR_DECL_STATIC_ASSERT:
   + to check a condition at compile time within the declaration section.
   MPFR_STAT_STATIC_ASSERT:
   + to check a condition at compile time within the statement section.
*/

#ifdef MPFR_USE_STATIC_ASSERT

/* C11 version */
# if defined (__STDC_VERSION__)
#  if (__STDC_VERSION__ >= 201112L)
#   define MPFR_DECL_STATIC_ASSERT(c) _Static_assert((c), #c )
#   define MPFR_STAT_STATIC_ASSERT(c) _Static_assert((c), #c )
#  endif
# endif

/* Default version which should be compatible with nearly all compilers */
# if !defined(MPFR_DECL_STATIC_ASSERT)
#  if __MPFR_GNUC(4,8)
/* Get rid of annoying warnings "typedef '...' locally defined but not used"
   (new in GCC 4.8). Thanks to Jonathan Wakely for this solution:
   https://gcc.gnu.org/ml/gcc-help/2013-07/msg00142.html */
#   define MPFR_TYPEDEF_UNUSED __attribute__ ((unused))
#  else
#   define MPFR_TYPEDEF_UNUSED
#  endif
#  define MPFR_ASSERT_CAT(a,b) MPFR_ASSERT_CAT_(a,b)
#  define MPFR_ASSERT_CAT_(a,b) a ## b
#  define MPFR_STAT_STATIC_ASSERT(c) do {                                    \
 typedef enum { MPFR_ASSERT_CAT(MPFR_STATIC_ASSERT_CONST_,__LINE__) = !(c) } \
 MPFR_ASSERT_CAT(MPFR_ASSERT_,__LINE__)[!!(c) ? 1 : -1]                      \
   MPFR_TYPEDEF_UNUSED; } while (0)
#  define MPFR_DECL_STATIC_ASSERT(c)                                         \
 typedef enum { MPFR_ASSERT_CAT(MPFR_STATIC_ASSERT_CONST_,__LINE__) = !(c) } \
 MPFR_ASSERT_CAT(MPFR_ASSERT_,__LINE__)[!!(c) ? 1 : -1];
# endif

#else

/* No support: default to classic assertions */
# define MPFR_DECL_STATIC_ASSERT(c) /* Nothing */
# define MPFR_STAT_STATIC_ASSERT(c) MPFR_ASSERTN(c)

#endif

#endif
