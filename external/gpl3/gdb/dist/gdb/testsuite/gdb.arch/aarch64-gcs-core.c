/* This test program is part of GDB, the GNU debugger.

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

#include <stdio.h>
#include <stdlib.h>
#include <sys/auxv.h>
#include <linux/prctl.h>
#include <sys/syscall.h>

/* Feature check for Guarded Control Stack.  */
#ifndef HWCAP_GCS
#define HWCAP_GCS (1ULL << 32)
#endif

#ifndef PR_GET_SHADOW_STACK_STATUS
#define PR_GET_SHADOW_STACK_STATUS 74
#define PR_SET_SHADOW_STACK_STATUS 75
#define PR_SHADOW_STACK_ENABLE (1UL << 0)
#endif

/* We need to use a macro to call prctl because after GCS is enabled, it's not
   possible to return from the function which enabled it.  This is because the
   return address of the calling function isn't on the GCS.  */
#define my_syscall2(num, arg1, arg2)					\
  ({									\
    register long _num __asm__("x8") = (num);				\
    register long _arg1 __asm__("x0") = (long)(arg1);			\
    register long _arg2 __asm__("x1") = (long)(arg2);			\
    register long _arg3 __asm__("x2") = 0;				\
    register long _arg4 __asm__("x3") = 0;				\
    register long _arg5 __asm__("x4") = 0;				\
									\
    __asm__ volatile ("svc #0\n"					\
		      : "=r"(_arg1)					\
		      : "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),	\
			"r"(_arg5), "r"(_num)				\
		      : "memory", "cc");				\
    _arg1;								\
  })

#define get_gcspr(void)							\
  ({									\
    unsigned long *gcspr;						\
									\
    /* Get GCSPR_EL0.  */						\
    asm volatile ("mrs	%0, S3_3_C2_C5_1" : "=r"(gcspr) : : "cc");	\
									\
    gcspr;								\
  })

/* Corrupt the return address to see if GDB will report a SIGSEGV with the
   expected $_siginfo.si_code.  */
static void __attribute__ ((noinline))
function (unsigned long *gcspr)
{
  /* x30 holds the return address.  */
  register long x30 __asm__("x30") __attribute__ ((unused));

  /* Print GCSPR to stdout so that the testcase can capture it.  */
  printf ("%p\n", get_gcspr ());
  fflush (stdout);

  /* Cause a GCS exception.  */
  x30 = 0xbadc0ffee;
  __asm__ volatile ("ret\n");
}

int
main (void)
{
  if (!(getauxval (AT_HWCAP) & HWCAP_GCS))
    {
      fprintf (stderr, "GCS support not found in AT_HWCAP\n");
      return EXIT_FAILURE;
    }

  /* Force shadow stacks on, our tests *should* be fine with or
     without libc support and with or without this having ended
     up tagged for GCS and enabled by the dynamic linker.  We
     can't use the libc prctl() function since we can't return
     from enabling the stack.  Also lock GCS if not already
     locked so we can test behaviour when it's locked.  */
  unsigned long gcs_mode;
  int ret = my_syscall2 (__NR_prctl, PR_GET_SHADOW_STACK_STATUS, &gcs_mode);
  if (ret)
    {
      fprintf (stderr, "Failed to read GCS state: %d\n", ret);
      return EXIT_FAILURE;
    }

  if (!(gcs_mode & PR_SHADOW_STACK_ENABLE))
    {
      gcs_mode = PR_SHADOW_STACK_ENABLE;
      ret = my_syscall2 (__NR_prctl, PR_SET_SHADOW_STACK_STATUS, gcs_mode);
      if (ret)
	{
	  fprintf (stderr, "Failed to configure GCS: %d\n", ret);
	  return EXIT_FAILURE;
	}
    }

  unsigned long *gcspr = get_gcspr ();

  /* Pass gscpr to function just so it's used for something.  */
  function (gcspr);        /* Break here.  */

  /* Avoid returning, in case libc doesn't understand GCS.  */
  exit (EXIT_SUCCESS);
}
