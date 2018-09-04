/* exceptions -- test file for exceptions

Copyright (C) 2015 INRIA

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

#include "mpc-tests.h"

/* Return non-zero if 'rnd' rounds towards zero, for a number of sign 'sgn' */
#define MPC_IS_LIKE_RNDZ(rnd, sgn) \
  ((rnd==MPFR_RNDZ) || (sgn<0 && rnd==MPFR_RNDU) || (sgn>0 && rnd==MPFR_RNDD))

#define MPFR_SGN(x) (mpfr_signbit (x) ? -1 : 1)

static void
foo (int f(mpc_ptr, mpc_srcptr, mpc_rnd_t), char *s)
{
  mpc_t z, t;
  int rnd_re, rnd_im, rnd;
#define N 5
  mpfr_exp_t exy[N][2] = {{200, 800}, {800, 200}, {-50, 50}, {-10, 1000},
                          {0, 1000}};
  int n, inex, inex_re, inex_im, sgn;

  mpc_init2 (z, MPFR_PREC_MIN);
  mpc_init2 (t, MPFR_PREC_MIN);
  for (n = 0; n < N; n++)
    for (sgn = 0; sgn < 4; sgn++)
    {
      if (exy[n][0])
        mpfr_set_ui_2exp (mpc_realref (z), 1, exy[n][0], MPFR_RNDN);
      else
        mpfr_set_ui (mpc_realref (z), 0, MPFR_RNDN);
      if (sgn & 1)
        mpfr_neg (mpc_realref (z), mpc_realref (z), MPFR_RNDN);
      if (exy[n][1])
        mpfr_set_ui_2exp (mpc_imagref (z), 1, exy[n][1], MPFR_RNDN);
      else
        mpfr_set_ui (mpc_imagref (z), 0, MPFR_RNDN);
      if (sgn & 2)
        mpfr_neg (mpc_imagref (z), mpc_imagref (z), MPFR_RNDN);

      inex = f (t, z, MPC_RNDZZ);
      inex_re = MPC_INEX_RE(inex);
      inex_im = MPC_INEX_IM(inex);

      if (inex_re != 0 && mpfr_inf_p (mpc_realref (t)))
        {
          fprintf (stderr, "Error, wrong real part with rounding towards zero\n");
          fprintf (stderr, "f = %s\n", s);
          fprintf (stderr, "z=");
          mpc_out_str (stderr, 2, 0, z, MPC_RNDNN);
          fprintf (stderr, "\nt=");
          mpc_out_str (stderr, 2, 0, t, MPC_RNDNN);
          fprintf (stderr, "\n");
          exit (1);
        }

      if (inex_im != 0 && mpfr_inf_p (mpc_imagref (t)))
        {
          fprintf (stderr, "Error, wrong imag part with rounding towards zero\n");
          fprintf (stderr, "f = %s\n", s);
          fprintf (stderr, "z=");
          mpc_out_str (stderr, 2, 0, z, MPC_RNDNN);
          fprintf (stderr, "\nt=");
          mpc_out_str (stderr, 2, 0, t, MPC_RNDNN);
          fprintf (stderr, "\n");
          exit (1);
        }

      rnd_re = mpfr_signbit (mpc_realref (t)) == 0 ? MPFR_RNDU : MPFR_RNDD;
      rnd_im = mpfr_signbit (mpc_imagref (t)) == 0 ? MPFR_RNDU : MPFR_RNDD;
      rnd = MPC_RND(rnd_re,rnd_im); /* round away */

      inex = f (t, z, rnd);
      inex_re = MPC_INEX_RE(inex);
      inex_im = MPC_INEX_IM(inex);

      if (inex_re != 0 && mpfr_zero_p (mpc_realref (t)))
        {
          fprintf (stderr, "Error, wrong real part with rounding away from zero\n");
          fprintf (stderr, "f = %s\n", s);
          fprintf (stderr, "z=");
          mpc_out_str (stderr, 2, 0, z, MPC_RNDNN);
          fprintf (stderr, "\nt=");
          mpc_out_str (stderr, 2, 0, t, MPC_RNDNN);
          fprintf (stderr, "\n");
          fprintf (stderr, "rnd=%s\n", mpfr_print_rnd_mode (rnd_re));
          exit (1);
        }

      if (inex_im != 0 && mpfr_zero_p (mpc_imagref (t)))
        {
          fprintf (stderr, "Error, wrong imag part with rounding away from zero\n");
          fprintf (stderr, "f = %s\n", s);
          fprintf (stderr, "z=");
          mpc_out_str (stderr, 2, 0, z, MPC_RNDNN);
          fprintf (stderr, "\nt=");
          mpc_out_str (stderr, 2, 0, t, MPC_RNDNN);
          fprintf (stderr, "\n");
          fprintf (stderr, "rnd=%s\n", mpfr_print_rnd_mode (rnd_im));
          exit (1);
        }
    }

  mpc_clear (z);
  mpc_clear (t);
}

int
main (void)
{
  test_start ();

  foo (mpc_sqr, "sqr");
  foo (mpc_conj, "conj");
  foo (mpc_neg, "neg");
  foo (mpc_sqrt, "sqrt");
  foo (mpc_set, "set");
  foo (mpc_proj, "proj");
  foo (mpc_exp, "exp");
  foo (mpc_exp, "exp");
  foo (mpc_log, "log");
  foo (mpc_log10, "log10");
  foo (mpc_sin, "sin");
  foo (mpc_cos, "cos");
  foo (mpc_tan, "tan");
  foo (mpc_sinh, "sinh");
  foo (mpc_cosh, "cosh");
  foo (mpc_tanh, "tanh");
  foo (mpc_asin, "asin");
  foo (mpc_acos, "acos");
  foo (mpc_atan, "atan");
  foo (mpc_asinh, "asinh");
  foo (mpc_acosh, "acosh");
  foo (mpc_atanh, "atanh");

  test_end ();

  return 0;
}
