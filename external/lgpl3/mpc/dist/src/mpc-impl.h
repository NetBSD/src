/* mpc-impl.h -- Internal include file for mpc.

Copyright (C) 2002, 2004, 2005, 2008, 2009, 2010, 2011, 2012 INRIA

This file is part of GNU MPC.

GNU MPC is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see http://www.gnu.org/licenses/ .
*/

#ifndef __MPC_IMPL_H
#define __MPC_IMPL_H
#define __MPC_LIBRARY_BUILD
   /* to indicate we are inside the library build */

#include "config.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "mpc.h"

/*
 * Miscellaneous useful macros
 */

#define MPC_MIN(h,i) ((h) < (i) ? (h) : (i))
#define MPC_MAX(h,i) ((h) > (i) ? (h) : (i))

/* Safe absolute value (to avoid possible integer overflow) */
/* type is the target (unsigned) type (copied from mpfr-impl.h) */
#ifdef SAFE_ABS
#undef SAFE_ABS
#endif
#define SAFE_ABS(type,x) ((x) >= 0 ? (type)(x) : -(type)(x))


/*
 * MPFR constants and macros
 */

#ifndef BITS_PER_MP_LIMB
#define BITS_PER_MP_LIMB mp_bits_per_limb
#endif

#define MPFR_SIGNBIT(x) (mpfr_signbit (x) ? -1 : 1)
#define MPC_MPFR_SIGN(x) (mpfr_zero_p (x) ? 0 : MPFR_SIGNBIT (x))
   /* should be called MPFR_SIGN, but this is taken in mpfr.h */
#define MPFR_CHANGE_SIGN(x) mpfr_neg(x,x,MPFR_RNDN)
#define MPFR_COPYSIGN(x,y,z,rnd) (mpfr_nan_p (z) ? \
   mpfr_setsign (x, y, 0, rnd) : \
   mpfr_copysign (x, y, z, rnd))
   /* work around spurious signs in nan */
#define MPFR_ADD_ONE_ULP(x) \
  (mpfr_sgn (x) > 0 ? mpfr_nextabove (x) : mpfr_nextbelow (x))
#define MPFR_SUB_ONE_ULP(x) \
  (mpfr_sgn (x) > 0 ? mpfr_nextbelow (x) : mpfr_nextabove (x))
   /* drop unused rounding mode from macros */
#define MPFR_SWAP(a,b) do { mpfr_srcptr tmp; tmp = a; a = b; b = tmp; } while (0)


/*
 * MPC macros
 */

#define MPC_PREC_RE(x) (mpfr_get_prec(mpc_realref(x)))
#define MPC_PREC_IM(x) (mpfr_get_prec(mpc_imagref(x)))
#define MPC_MAX_PREC(x) MPC_MAX(MPC_PREC_RE(x), MPC_PREC_IM(x))

#define INV_RND(r) \
   (((r) == MPFR_RNDU) ? MPFR_RNDD : (((r) == MPFR_RNDD) ? MPFR_RNDU : (r)))

/* Return non-zero if 'rnd' rounds towards zero, for a number of sign 'sgn' */
#define MPC_IS_LIKE_RNDZ(rnd, sgn) \
  ((rnd==MPFR_RNDZ) || (sgn<0 && rnd==MPFR_RNDU) || (sgn>0 && rnd==MPFR_RNDD))

/* Return non-zero if 'rnd' rounds away from zero for a number of sign 'sgn' */
#define MPC_IS_LIKE_RNDA(rnd, sgn) \
  ((sgn<0 && rnd==MPFR_RNDD) || (sgn>0 && rnd==MPFR_RNDU))

#define mpc_inf_p(z) (mpfr_inf_p(mpc_realref(z))||mpfr_inf_p(mpc_imagref(z)))
   /* Convention in C99 (G.3): z is regarded as an infinity if at least one of
      its parts is infinite */
#define mpc_zero_p(z) (mpfr_zero_p(mpc_realref(z))&&mpfr_zero_p(mpc_imagref(z)))
   /* Convention in C99 (G.3): z is regarded as a zero if each of its parts is
      a zero */
#define mpc_fin_p(z) (mpfr_number_p(mpc_realref(z))&&mpfr_number_p(mpc_imagref(z)))
   /* Convention in C99 (G.3): z is regarded as finite if both its parts are */
#define mpc_nan_p(z) ((mpfr_nan_p(mpc_realref(z)) && !mpfr_inf_p(mpc_imagref(z))) || (mpfr_nan_p(mpc_imagref(z)) && !mpfr_inf_p(mpc_realref(z))))
   /* Consider as NaN all other numbers containing at least one NaN */


/*
 * ASSERT macros
 */

#ifdef NDEBUG
#define MPC_ASSERT(expr) \
  do {                   \
  } while (0)
#else
#define MPC_ASSERT(expr)                                        \
  do {                                                          \
    if (!(expr))                                                \
      {                                                         \
        fprintf (stderr, "%s:%d: MPC assertion failed: %s\n",   \
                 __FILE__, __LINE__, #expr);                    \
        abort();                                                \
      }                                                         \
  } while (0)
#endif


/*
 * Debug macros
 */

#define MPC_OUT(x)                                              \
do {                                                            \
  printf (#x "[%lu,%lu]=", (unsigned long int) MPC_PREC_RE (x), \
      (unsigned long int) MPC_PREC_IM (x));                     \
  mpc_out_str (stdout, 2, 0, x, MPC_RNDNN);                     \
  printf ("\n");                                                \
} while (0)

#define MPFR_OUT(x)                                             \
do {                                                            \
  printf (#x "[%lu]=", (unsigned long int) mpfr_get_prec (x));  \
  mpfr_out_str (stdout, 2, 0, x, MPFR_RNDN);                     \
  printf ("\n");                                                \
} while (0)


/*
 * Constants
 */

#ifndef MUL_KARATSUBA_THRESHOLD
#define MUL_KARATSUBA_THRESHOLD 23
#endif


/*
 * Define internal functions
 */

#if defined (__cplusplus)
extern "C" {
#endif


__MPC_DECLSPEC int  mpc_mul_naive (mpc_ptr, mpc_srcptr, mpc_srcptr, mpc_rnd_t);
__MPC_DECLSPEC int  mpc_mul_karatsuba (mpc_ptr, mpc_srcptr, mpc_srcptr, mpc_rnd_t);
__MPC_DECLSPEC int  mpc_fma_naive (mpc_ptr, mpc_srcptr, mpc_srcptr, mpc_srcptr, mpc_rnd_t);
__MPC_DECLSPEC int  mpc_pow_usi (mpc_ptr, mpc_srcptr, unsigned long, int, mpc_rnd_t);
__MPC_DECLSPEC char* mpc_alloc_str (size_t);
__MPC_DECLSPEC char* mpc_realloc_str (char*, size_t, size_t);
__MPC_DECLSPEC void mpc_free_str (char*);
__MPC_DECLSPEC mpfr_prec_t mpc_ceil_log2 (mpfr_prec_t);
__MPC_DECLSPEC int set_pi_over_2 (mpfr_ptr, int, mpfr_rnd_t);
__MPC_DECLSPEC int mpc_fix_inf (mpfr_t x, mpfr_rnd_t rnd);
__MPC_DECLSPEC int mpc_fix_zero (mpfr_t x, mpfr_rnd_t rnd);

#if defined (__cplusplus)
}
#endif


#endif
