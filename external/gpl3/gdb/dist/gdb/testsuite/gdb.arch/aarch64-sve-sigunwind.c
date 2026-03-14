/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

/* Exercise unwinding AArch64's SVE registers from a signal frame.   */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/prctl.h>
#include <unistd.h>

static int second_vl = 0;

static void
initialize_sve_state_main ()
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
  __asm __volatile ("ptrue p0.d");
  __asm __volatile ("ptrue p1.d");
  __asm __volatile ("ptrue p2.d");
  __asm __volatile ("ptrue p3.d");
  __asm __volatile ("ptrue p4.d");
  __asm __volatile ("ptrue p5.d");
  __asm __volatile ("ptrue p6.d");
  __asm __volatile ("ptrue p7.d");
  __asm __volatile ("ptrue p8.d");
  __asm __volatile ("ptrue p9.d");
  __asm __volatile ("ptrue p10.d");
  __asm __volatile ("ptrue p11.d");
  __asm __volatile ("ptrue p12.d");
  __asm __volatile ("ptrue p13.d");
  __asm __volatile ("ptrue p14.d");
  __asm __volatile ("ptrue p15.d");
  __asm __volatile ("setffr");
}

static void
initialize_sve_state_sighandler ()
{
  __asm __volatile ("dup z0.b, -2");
  __asm __volatile ("dup z1.b, -2");
  __asm __volatile ("dup z2.b, -2");
  __asm __volatile ("dup z3.b, -2");
  __asm __volatile ("dup z4.b, -2");
  __asm __volatile ("dup z5.b, -2");
  __asm __volatile ("dup z6.b, -2");
  __asm __volatile ("dup z7.b, -2");
  __asm __volatile ("dup z8.b, -2");
  __asm __volatile ("dup z9.b, -2");
  __asm __volatile ("dup z10.b, -2");
  __asm __volatile ("dup z11.b, -2");
  __asm __volatile ("dup z12.b, -2");
  __asm __volatile ("dup z13.b, -2");
  __asm __volatile ("dup z14.b, -2");
  __asm __volatile ("dup z15.b, -2");
  __asm __volatile ("dup z16.b, -2");
  __asm __volatile ("dup z17.b, -2");
  __asm __volatile ("dup z18.b, -2");
  __asm __volatile ("dup z19.b, -2");
  __asm __volatile ("dup z20.b, -2");
  __asm __volatile ("dup z21.b, -2");
  __asm __volatile ("dup z22.b, -2");
  __asm __volatile ("dup z23.b, -2");
  __asm __volatile ("dup z24.b, -2");
  __asm __volatile ("dup z25.b, -2");
  __asm __volatile ("dup z26.b, -2");
  __asm __volatile ("dup z27.b, -2");
  __asm __volatile ("dup z28.b, -2");
  __asm __volatile ("dup z29.b, -2");
  __asm __volatile ("dup z30.b, -2");
  __asm __volatile ("dup z31.b, -2");
  __asm __volatile ("pfalse p0.b");
  __asm __volatile ("pfalse p1.b");
  __asm __volatile ("pfalse p2.b");
  __asm __volatile ("pfalse p3.b");
  __asm __volatile ("pfalse p4.b");
  __asm __volatile ("pfalse p5.b");
  __asm __volatile ("pfalse p6.b");
  __asm __volatile ("pfalse p7.b");
  __asm __volatile ("pfalse p8.b");
  __asm __volatile ("pfalse p9.b");
  __asm __volatile ("pfalse p10.b");
  __asm __volatile ("pfalse p11.b");
  __asm __volatile ("pfalse p12.b");
  __asm __volatile ("pfalse p13.b");
  __asm __volatile ("pfalse p14.b");
  __asm __volatile ("pfalse p15.b");
  __asm __volatile ("setffr");
}

/* Set new value for the SVE vector length.
   Return the value that was set.  */

static int
set_vl (int vl)
{
  int rc;

  rc = prctl (PR_SVE_SET_VL, vl, 0, 0, 0);
  if (rc < 0)
    {
      perror ("FAILED to PR_SVE_SET_VL");
      exit (EXIT_FAILURE);
    }

  return rc & PR_SVE_VL_LEN_MASK;
}

static void
sighandler (int sig, siginfo_t *info, void *ucontext)
{
  /* Set vector length to the second value.  */
  second_vl = set_vl (second_vl);
  initialize_sve_state_sighandler ();
  printf ("sighandler: second_vl = %d\n", second_vl); /* Break here.  */
}

int
main (int argc, char *argv[])
{
  if (argc != 3)
    {
      fprintf (stderr, "Usage: %s <first vl> <second vl>\n", argv[0]);
      return 1;
    }

  int first_vl = atoi (argv[1]);
  second_vl = atoi (argv[2]);

  if (first_vl == 0 || second_vl == 0)
    {
      fprintf (stderr, "Invalid vector length.\n");
      return 1;
    }

  /* Set vector length to the first value.  */
  first_vl = set_vl (first_vl);

  printf ("main: first_vl = %d\n", first_vl);

  unsigned char buf[256];

  /* Use an SVE register to make the kernel start saving the SVE bank.  */
  asm volatile ("mov z0.b, #255\n\t"
		"str z0, %0"
		:
		: "m" (buf)
		: "z0", "memory");

  initialize_sve_state_main ();

  struct sigaction sigact;
  sigact.sa_sigaction = sighandler;
  sigact.sa_flags = SA_SIGINFO;
  sigaction (SIGUSR1, &sigact, NULL);

  kill (getpid (), SIGUSR1);

  return 0;
}
