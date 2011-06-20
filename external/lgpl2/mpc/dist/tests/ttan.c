/* test file for mpc_tan.

Copyright (C) INRIA, 2008, 2011

This file is part of the MPC Library.

The MPC Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The MPC Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the MPC Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#include <stdlib.h>
#include "mpc-tests.h"

static void
pure_real_argument (void)
{
  /* tan(x -i*0) = tan(x) -i*0 */
  /* tan(x +i*0) = tan(x) +i*0 */
  mpfr_t x;
  mpfr_t tan_x;
  mpc_t z;
  mpc_t tan_z;

  mpfr_init2 (x, 79);
  mpfr_init2 (tan_x, 113);
  mpc_init2 (z, 79);
  mpc_init2 (tan_z, 113);

  /* tan(1 +i*0) = tan(1) +i*0 */
  mpc_set_ui_ui (z, 1, 0, MPC_RNDNN);
  mpfr_set_ui (x, 1, GMP_RNDN);
  mpfr_tan (tan_x, x, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_RE (tan_z), tan_x) != 0
      || !mpfr_zero_p (MPC_IM (tan_z)) || mpfr_signbit (MPC_IM (tan_z)))
    {
      printf ("mpc_tan(1 + i * 0) failed\n");
      exit (1);
    }

  /* tan(1 -i*0) = tan(1) -i*0 */
  mpc_conj (z, z, MPC_RNDNN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_RE (tan_z), tan_x) != 0
      || !mpfr_zero_p (MPC_IM (tan_z)) || !mpfr_signbit (MPC_IM (tan_z)))
    {
      printf ("mpc_tan(1 - i * 0) failed\n");
      exit (1);
    }

  /* tan(Pi/2 +i*0) = +Inf +i*0 */
  mpfr_const_pi (x, GMP_RNDN);
  mpfr_div_2ui (x, x, 1, GMP_RNDN);
  mpfr_set (MPC_RE (z), x, GMP_RNDN);
  mpfr_set_ui (MPC_IM (z), 0, GMP_RNDN);
  mpfr_tan (tan_x, x, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_RE (tan_z), tan_x) != 0
      || !mpfr_zero_p (MPC_IM (tan_z)) || mpfr_signbit (MPC_IM (tan_z)))
    {
      printf ("mpc_tan(Pi/2 + i * 0) failed\n");
      exit (1);
    }

  /* tan(Pi/2 -i*0) = +Inf -i*0 */
  mpc_conj (z, z, MPC_RNDNN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_RE (tan_z), tan_x) != 0
      || !mpfr_zero_p (MPC_IM (tan_z)) || !mpfr_signbit (MPC_IM (tan_z)))
    {
      printf ("mpc_tan(Pi/2 - i * 0) failed\n");
      exit (1);
    }

  /* tan(-Pi/2 +i*0) = -Inf +i*0 */
  mpfr_neg (x, x, GMP_RNDN);
  mpc_neg (z, z, MPC_RNDNN);
  mpfr_tan (tan_x, x, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_RE (tan_z), tan_x) != 0
      || !mpfr_zero_p (MPC_IM (tan_z)) || mpfr_signbit (MPC_IM (tan_z)))
    {
      printf ("mpc_tan(-Pi/2 + i * 0) failed\n");
      exit (1);
    }

  /* tan(-Pi/2 -i*0) = -Inf -i*0 */
  mpc_conj (z, z, MPC_RNDNN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_RE (tan_z), tan_x) != 0
      || !mpfr_zero_p (MPC_IM (tan_z)) || !mpfr_signbit (MPC_IM (tan_z)))
    {
      printf ("mpc_tan(-Pi/2 - i * 0) failed\n");
      exit (1);
    }

  mpc_clear (tan_z);
  mpc_clear (z);
  mpfr_clear (tan_x);
  mpfr_clear (x);
}

static void
pure_imaginary_argument (void)
{
  /* tan(-0 +i*y) = -0 +i*tanh(y) */
  /* tan(+0 +i*y) = +0 +i*tanh(y) */
  mpfr_t y;
  mpfr_t tanh_y;
  mpc_t z;
  mpc_t tan_z;
  mpfr_prec_t prec = (mpfr_prec_t) 111;

  mpfr_init2 (y, 2);
  mpfr_init2 (tanh_y, prec);
  mpc_init2 (z, 2);
  mpc_init2 (tan_z, prec);

  /* tan(0 +i) = +0 +i*tanh(1) */
  mpc_set_ui_ui (z, 0, 1, MPC_RNDNN);
  mpfr_set_ui (y, 1, GMP_RNDN);
  mpfr_tanh (tanh_y, y, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_IM (tan_z), tanh_y) != 0
      || !mpfr_zero_p (MPC_RE (tan_z)) || mpfr_signbit (MPC_RE (tan_z)))
    {
      mpc_t c99;

      mpc_init2 (c99, prec);
      mpfr_set_ui (MPC_RE (c99), 0, GMP_RNDN);
      mpfr_set (MPC_IM (c99), tanh_y, GMP_RNDN);

      TEST_FAILED ("mpc_tan", z, tan_z, c99, MPC_RNDNN);
    }

  /* tan(0 -i) = +0 +i*tanh(-1) */
  mpc_conj (z, z, MPC_RNDNN);
  mpfr_neg (tanh_y, tanh_y, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_IM (tan_z), tanh_y) != 0
      || !mpfr_zero_p (MPC_RE (tan_z)) || mpfr_signbit (MPC_RE (tan_z)))
    {
      mpc_t c99;

      mpc_init2 (c99, prec);
      mpfr_set_ui (MPC_RE (c99), 0, GMP_RNDN);
      mpfr_set (MPC_IM (c99), tanh_y, GMP_RNDN);

      TEST_FAILED ("mpc_tan", z, tan_z, c99, MPC_RNDNN);
    }

  /* tan(-0 +i) = -0 +i*tanh(1) */
  mpc_neg (z, z, MPC_RNDNN);
  mpfr_neg (tanh_y, tanh_y, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_IM (tan_z), tanh_y) != 0
      || !mpfr_zero_p (MPC_RE (tan_z)) || !mpfr_signbit (MPC_RE (tan_z)))
    {
      mpc_t c99;

      mpc_init2 (c99, prec);
      mpfr_set_ui (MPC_RE (c99), 0, GMP_RNDN);
      mpfr_set (MPC_IM (c99), tanh_y, GMP_RNDN);

      TEST_FAILED ("mpc_tan", z, tan_z, c99, MPC_RNDNN);
    }

  /* tan(-0 -i) = -0 +i*tanh(-1) */
  mpc_conj (z, z, MPC_RNDNN);
  mpfr_neg (tanh_y, tanh_y, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpfr_cmp (MPC_IM (tan_z), tanh_y) != 0
      || !mpfr_zero_p (MPC_RE (tan_z)) || !mpfr_signbit (MPC_RE (tan_z)))
    {
      mpc_t c99;

      mpc_init2 (c99, prec);
      mpfr_set_ui (MPC_RE (c99), 0, GMP_RNDN);
      mpfr_set (MPC_IM (c99), tanh_y, GMP_RNDN);

      TEST_FAILED ("mpc_tan", z, tan_z, c99, MPC_RNDNN);
    }

  mpc_clear (tan_z);
  mpc_clear (z);
  mpfr_clear (tanh_y);
  mpfr_clear (y);
}

static void
check_53 (void)
{
  mpc_t z;
  mpc_t tan_z;
  mpc_t t;

  mpc_init2 (z, 53);
  mpc_init2 (tan_z, 53);
  mpc_init2 (t, 53);

  /* Let's play around the poles */
  /* x = Re(z) = Pi/2 rounded to nearest to 53 bits precision */
  /* y = Im(z) = Pi/2 - Re(z) rounded to nearest to 53 bits precision */
  mpfr_set_str (MPC_RE (z), "3243F6A8885A30p-53", 16, GMP_RNDN);
  mpfr_set_str (MPC_IM (z), "11A62633145C07p-106", 16, GMP_RNDN);
  mpfr_set_str (MPC_RE (t), "1D02967C31CDB5", 16, GMP_RNDN);
  mpfr_set_str (MPC_IM (t), "1D02967C31CDB5", 16, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpc_cmp (tan_z, t) != 0)
    TEST_FAILED ("mpc_tan", z, tan_z, t, MPC_RNDNN);

  mpfr_set_str (MPC_RE (t), "1D02967C31CDB4", 16, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDDU);
  if (mpc_cmp (tan_z, t) != 0)
    TEST_FAILED ("mpc_tan", z, tan_z, t, MPC_RNDDU);

  mpfr_set_str (MPC_IM (t), "1D02967C31CDB4", 16, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDZD);
  if (mpc_cmp (tan_z, t) != 0)
    TEST_FAILED ("mpc_tan", z, tan_z, t, MPC_RNDZD);

  /* Re(z) = x + 2^(-52) */
  /* Im(z) = y - 2^(-52) */
  mpfr_set_str (MPC_RE (z), "1921FB54442D19p-52", 16, GMP_RNDN);
  mpfr_set_str (MPC_IM (z), "-172CECE675D1FDp-105", 16, GMP_RNDN);
  mpfr_set_str (MPC_RE (t), "-B0BD0AA4A3B3D", 16, GMP_RNDN);
  mpfr_set_str (MPC_IM (t), "-B0BD0AA4A3B3D", 16, GMP_RNDN);
  mpc_tan (tan_z, z, MPC_RNDNN);
  if (mpc_cmp (tan_z, t) != 0)
    TEST_FAILED ("mpc_tan", z, tan_z, t, MPC_RNDNN);

  mpc_clear (t);
  mpc_clear (tan_z);
  mpc_clear (z);
}

int
main (void)
{
  DECL_FUNC (CC, f, mpc_tan);

  test_start ();

  data_check (f, "tan.dat");
  tgeneric (f, 2, 512, 7, 4);

  pure_real_argument ();
  pure_imaginary_argument ();
  check_53 ();

  test_end ();

  return 0;
}
