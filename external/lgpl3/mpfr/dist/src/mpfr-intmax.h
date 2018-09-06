/* MPFR internal header related to intmax_t.

Copyright 2004-2018 Free Software Foundation, Inc.
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

#ifndef __MPFR_INTMAX_H__
#define __MPFR_INTMAX_H__

/* The ISO C99 standard specifies that in C++ implementations the
 * INTMAX_MAX, ... macros should only be defined if explicitly requested.
 */
#if defined __cplusplus
# define __STDC_LIMIT_MACROS
# define __STDC_CONSTANT_MACROS
#endif

/* The definition of MPFR_USE_INTMAX_T is needed on systems for which
 * the current (non-standard) macro tests in mpfr.h is not sufficient.
 * This will force the support of intmax_t/uintmax_t if <inttypes.h>
 * and/or <stdint.h> are available. This also avoids a failure in the
 * tests (replace the macro tests in mpfr.h by just
 *   #if defined (MPFR_USE_INTMAX_T)
 * to simulate such a system and reproduce the problem).
 * Note: if this makes the build fail on some systems (because these
 * headers are broken), we will need a configure test to undefine
 * HAVE_INTTYPES_H and HAVE_STDINT_H in such a case.
 */

#if HAVE_INTTYPES_H
# include <inttypes.h>
# define MPFR_USE_INTMAX_T
#endif

#if HAVE_STDINT_H
# include <stdint.h>
# define MPFR_USE_INTMAX_T
#endif

/* Largest signed integer type available for the MPFR build. */
#if defined(MPFR_USE_INTMAX_T)
typedef intmax_t mpfr_intmax_t;
typedef uintmax_t mpfr_uintmax_t;
#elif defined(HAVE_LONG_LONG)
typedef long long mpfr_intmax_t;
typedef unsigned long long mpfr_uintmax_t;
# define MPFR_INTMAX_MAX LLONG_MAX
#else
typedef long mpfr_intmax_t;
typedef unsigned long mpfr_uintmax_t;
# define MPFR_INTMAX_MAX LONG_MAX
#endif

#endif
