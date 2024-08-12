/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

/* Exercise AArch64's Scalable Vector/Matrix Extension signal frame handling
   for GDB.  */

#include <stdio.h>
#include <sys/auxv.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#ifndef HWCAP_SVE
#define HWCAP_SVE (1 << 22)
#endif

#ifndef HWCAP2_SME
#define HWCAP2_SME (1 << 23)
#endif

#ifndef HWCAP2_SME2
#define HWCAP2_SME2 (1UL << 37)
#define HWCAP2_SME2P1 (1UL << 38)
#endif

#ifndef PR_SVE_SET_VL
#define PR_SVE_SET_VL 50
#define PR_SVE_GET_VL 51
#define PR_SVE_VL_LEN_MASK 0xffff
#endif

#ifndef PR_SME_SET_VL
#define PR_SME_SET_VL 63
#define PR_SME_GET_VL 64
#define PR_SME_VL_LEN_MASK 0xffff
#endif

static int count = 0;

static void
handler (int sig)
{
  count++; /* handler */
}

static void
enable_za ()
{
  /* smstart za */
  __asm __volatile (".word 0xD503457F");
}

static void
disable_za ()
{
  /* smstop za */
  __asm __volatile (".word 0xD503447F");
}

static void
enable_sm ()
{
  /* smstart sm */
  __asm __volatile (".word 0xD503437F");
}

static void
disable_sm ()
{
  /* smstop sm */
  __asm __volatile (".word 0xD503427F");
}

static void
initialize_fpsimd_state ()
{
  char buffer[16];

  for (int i = 0; i < 16; i++)
    buffer[i] = 0x55;

  __asm __volatile ("mov x0, %0\n\t" \
		    : : "r" (buffer));

  __asm __volatile ("ldr q0, [x0]");
  __asm __volatile ("ldr q1, [x0]");
  __asm __volatile ("ldr q2, [x0]");
  __asm __volatile ("ldr q3, [x0]");
  __asm __volatile ("ldr q4, [x0]");
  __asm __volatile ("ldr q5, [x0]");
  __asm __volatile ("ldr q6, [x0]");
  __asm __volatile ("ldr q7, [x0]");
  __asm __volatile ("ldr q8, [x0]");
  __asm __volatile ("ldr q9, [x0]");
  __asm __volatile ("ldr q10, [x0]");
  __asm __volatile ("ldr q11, [x0]");
  __asm __volatile ("ldr q12, [x0]");
  __asm __volatile ("ldr q13, [x0]");
  __asm __volatile ("ldr q14, [x0]");
  __asm __volatile ("ldr q15, [x0]");
  __asm __volatile ("ldr q16, [x0]");
  __asm __volatile ("ldr q17, [x0]");
  __asm __volatile ("ldr q18, [x0]");
  __asm __volatile ("ldr q19, [x0]");
  __asm __volatile ("ldr q20, [x0]");
  __asm __volatile ("ldr q21, [x0]");
  __asm __volatile ("ldr q22, [x0]");
  __asm __volatile ("ldr q23, [x0]");
  __asm __volatile ("ldr q24, [x0]");
  __asm __volatile ("ldr q25, [x0]");
  __asm __volatile ("ldr q26, [x0]");
  __asm __volatile ("ldr q27, [x0]");
  __asm __volatile ("ldr q28, [x0]");
  __asm __volatile ("ldr q29, [x0]");
  __asm __volatile ("ldr q30, [x0]");
  __asm __volatile ("ldr q31, [x0]");
}

static void
initialize_za_state ()
{
  /* zero za */
  __asm __volatile (".word 0xC00800FF");

  char buffer[256];

  for (int i = 0; i < 256; i++)
    buffer[i] = 0xaa;

  __asm __volatile ("mov x0, %0\n\t" \
		    : : "r" (buffer));

  /* Initialize loop boundaries.  */
  __asm __volatile ("mov w12, 0");
  __asm __volatile ("mov w17, 256");

  /* loop: ldr za[w12, 0], [x0] */
  __asm __volatile ("loop: .word 0xe1000000");
  __asm __volatile ("add w12, w12, 1");
  __asm __volatile ("cmp w12, w17");
  __asm __volatile ("bne loop");
}

static void
initialize_zt_state ()
{
  unsigned long hwcap2 = getauxval (AT_HWCAP2);

  if (!(hwcap2 & HWCAP2_SME2) && !(hwcap2 & HWCAP2_SME2P1))
    return;

  char buffer[64];

  for (int i = 0; i < 64; i++)
    buffer[i] = 0xff;

  __asm __volatile ("mov x0, %0\n\t" \
		    : : "r" (buffer));

  /* Initialize ZT0.  */
  /* ldr zt0, x0 */
  __asm __volatile (".word 0xe11f8000");
}

static void
initialize_sve_state ()
{
  __asm __volatile ("dup z0.b, -1");
  __asm __volatile ("dup z1.b, -1");
  __asm __volatile ("dup z2.b, -1");
  __asm __volatile ("dup z3.b, -1");
  __asm __volatile ("dup z4.b, -1");
  __asm __volatile ("dup z5.b, -1");
  __asm __volatile ("dup z6.b, -1");
  __asm __volatile ("dup z7.b, -1");
  __asm __volatile ("dup z8.b, -1");
  __asm __volatile ("dup z9.b, -1");
  __asm __volatile ("dup z10.b, -1");
  __asm __volatile ("dup z11.b, -1");
  __asm __volatile ("dup z12.b, -1");
  __asm __volatile ("dup z13.b, -1");
  __asm __volatile ("dup z14.b, -1");
  __asm __volatile ("dup z15.b, -1");
  __asm __volatile ("dup z16.b, -1");
  __asm __volatile ("dup z17.b, -1");
  __asm __volatile ("dup z18.b, -1");
  __asm __volatile ("dup z19.b, -1");
  __asm __volatile ("dup z20.b, -1");
  __asm __volatile ("dup z21.b, -1");
  __asm __volatile ("dup z22.b, -1");
  __asm __volatile ("dup z23.b, -1");
  __asm __volatile ("dup z24.b, -1");
  __asm __volatile ("dup z25.b, -1");
  __asm __volatile ("dup z26.b, -1");
  __asm __volatile ("dup z27.b, -1");
  __asm __volatile ("dup z28.b, -1");
  __asm __volatile ("dup z29.b, -1");
  __asm __volatile ("dup z30.b, -1");
  __asm __volatile ("dup z31.b, -1");
  __asm __volatile ("ptrue p0.b");
  __asm __volatile ("ptrue p1.b");
  __asm __volatile ("ptrue p2.b");
  __asm __volatile ("ptrue p3.b");
  __asm __volatile ("ptrue p4.b");
  __asm __volatile ("ptrue p5.b");
  __asm __volatile ("ptrue p6.b");
  __asm __volatile ("ptrue p7.b");
  __asm __volatile ("ptrue p8.b");
  __asm __volatile ("ptrue p9.b");
  __asm __volatile ("ptrue p10.b");
  __asm __volatile ("ptrue p11.b");
  __asm __volatile ("ptrue p12.b");
  __asm __volatile ("ptrue p13.b");
  __asm __volatile ("ptrue p14.b");
  __asm __volatile ("ptrue p15.b");
  __asm __volatile ("setffr");
}

static int get_vl_size ()
{
  int res = prctl (PR_SVE_GET_VL, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SVE_GET_VL (%d)\n", res);
      return -1;
    }
  return (res & PR_SVE_VL_LEN_MASK);
}

static int get_svl_size ()
{
  int res = prctl (PR_SME_GET_VL, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SME_GET_VL (%d)\n", res);
      return -1;
    }
  return (res & PR_SVE_VL_LEN_MASK);
}

static int set_vl_size (int new_vl)
{
  int res = prctl (PR_SVE_SET_VL, new_vl, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SVE_SET_VL (%d)\n", res);
      return -1;
    }

  res = get_vl_size ();
  if (res != new_vl)
    {
      printf ("Unexpected VL value (%d)\n", res);
      return -1;
    }

  return res;
}

static int set_svl_size (int new_svl)
{
  int res = prctl (PR_SME_SET_VL, new_svl, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SME_SET_VL (%d)\n", res);
      return -1;
    }

  res = get_svl_size ();
  if (res != new_svl)
    {
      printf ("Unexpected SVL value (%d)\n", res);
      return -1;
    }

  return res;
}

/* Enable register states based on STATE.

   0 - FPSIMD
   1 - SVE
   2 - SSVE
   3 - ZA (+ SME2 ZT0)
   4 - ZA and SSVE (+ SME2 ZT0).  */

void enable_states (int state)
{
  disable_za ();
  disable_sm ();
  initialize_fpsimd_state ();

  if (state == 1)
    {
      initialize_sve_state ();
    }
  else if (state == 2)
    {
      enable_sm ();
      initialize_sve_state ();
    }
  else if (state == 3)
    {
      enable_za ();
      initialize_za_state ();
      initialize_zt_state ();
    }
  else if (state == 4)
    {
      enable_za ();
      enable_sm ();
      initialize_sve_state ();
      initialize_za_state ();
      initialize_zt_state ();
    }

  return;
}

static int
test_id_to_state (int id)
{
  return (id / 25);
}

static int
test_id_to_vl (int id)
{
  return 16 << ((id / 5) % 5);
}

static int
test_id_to_svl (int id)
{
  return 16 << (id % 5);
}

static void
dummy ()
{
}

int
main (int argc, char **argv)
{
  if (getauxval (AT_HWCAP) & HWCAP_SVE && getauxval (AT_HWCAP2) & HWCAP2_SME)
    {
      int id_start = ID_START;
      int id_end = ID_END;
#ifdef SIGILL
      signal (SIGILL, handler);
#endif

      int signal_count = 0;
      for (int id = id_start; id <= id_end; id++)
	{
	  int state = test_id_to_state (id);
	  int vl = test_id_to_vl (id);
	  int svl = test_id_to_svl (id);

	  if (set_vl_size (vl) == -1 || set_svl_size (svl) == -1)
	    continue;

	  signal_count++;
	  enable_states (state);
	  dummy (); /* stop before SIGILL */
	  __asm __volatile (".word 0xDEADBEEF"); /* illegal instruction */
	  while (signal_count != count);
	}
    }
  else
    {
      printf ("SKIP: no HWCAP_SVE or HWCAP2_SME on this system\n");
      return -1;
    }

  return 0;
}
