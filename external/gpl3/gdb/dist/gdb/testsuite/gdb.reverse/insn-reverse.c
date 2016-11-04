/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2016 Free Software Foundation, Inc.

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

#if (defined __aarch64__)
#include <arm_neon.h>
#endif

#if (defined __aarch64__)
static void
load (void)
{
  int buf[8];

  asm ("ld1 { v1.8b }, [%[buf]]\n"
       "ld1 { v2.8b, v3.8b }, [%[buf]]\n"
       "ld1 { v3.8b, v4.8b, v5.8b }, [%[buf]]\n"
       :
       : [buf] "r" (buf)
       : /* No clobbers */);
}

static void
move (void)
{
  float32x2_t b1_ = vdup_n_f32(123.0f);
  float32_t a1_ = 0;
  float64x1_t b2_ = vdup_n_f64(456.0f);
  float64_t a2_ = 0;

  asm ("ins %0.s[0], %w1\n"
       : "=w"(b1_)
       : "r"(a1_), "0"(b1_)
       : /* No clobbers */);

  asm ("ins %0.d[1], %x1\n"
       : "=w"(b2_)
       : "r"(a2_), "0"(b2_)
       : /* No clobbers */);
}

static void
adv_simd_mod_imm (void)
{
  float32x2_t a1 = {2.0, 4.0};

  asm ("bic %0.2s, #1\n"
       "bic %0.2s, #1, lsl #8\n"
       : "=w"(a1)
       : "0"(a1)
       : /* No clobbers */);
}

static void
adv_simd_scalar_index (void)
{
  float64x2_t b_ = {0.0, 0.0};
  float64_t a_ = 1.0;
  float64_t result;

  asm ("fmla %d0,%d1,%2.d[1]"
       : "=w"(result)
       : "w"(a_), "w"(b_)
       : /* No clobbers */);
}

static void
adv_simd_smlal (void)
{
  asm ("smlal v13.2d, v8.2s, v0.2s");
}

static void
adv_simd_vect_shift (void)
{
  asm ("fcvtzs s0, s0, #1");
}
#elif (defined __arm__)
static void
ext_reg_load (void)
{
  char in[8];

  asm ("vldr d0, [%0]" : : "r" (in));
  asm ("vldr s3, [%0]" : : "r" (in));

  asm ("vldm %0, {d3-d4}" : : "r" (in));
  asm ("vldm %0, {s9-s11}" : : "r" (in));
}

static void
ext_reg_mov (void)
{
  int i, j;
  double d;

  i = 1;
  j = 2;

  asm ("vmov s4, s5, %0, %1" : "=r" (i), "=r" (j): );
  asm ("vmov s7, s8, %0, %1" : "=r" (i), "=r" (j): );
  asm ("vmov %0, %1, s10, s11" : : "r" (i), "r" (j));
  asm ("vmov %0, %1, s1, s2" : : "r" (i), "r" (j));

  asm ("vmov %P2, %0, %1" : "=r" (i), "=r" (j): "w" (d));
  asm ("vmov %1, %2, %P0" : "=w" (d) : "r" (i), "r" (j));
}

static void
ext_reg_push_pop (void)
{
  double d;

  asm ("vpush {%P0}" : : "w" (d));
  asm ("vpop {%P0}" : : "w" (d));
}
#endif

typedef void (*testcase_ftype) (void);

/* Functions testing instruction decodings.  GDB will read n_testcases
   to know how many functions to test.  */

static testcase_ftype testcases[] =
{
#if (defined __aarch64__)
  load,
  move,
  adv_simd_mod_imm,
  adv_simd_scalar_index,
  adv_simd_smlal,
  adv_simd_vect_shift,
#elif (defined __arm__)
  ext_reg_load,
  ext_reg_mov,
  ext_reg_push_pop,
#endif
};

static int n_testcases = (sizeof (testcases) / sizeof (testcase_ftype));

int
main ()
{
  int i = 0;

  for (i = 0; i < n_testcases; i++)
    testcases[i] ();

  return 0;
}
