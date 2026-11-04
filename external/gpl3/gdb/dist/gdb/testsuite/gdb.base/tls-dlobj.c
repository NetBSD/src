/* This testcase is part of GDB, the GNU debugger.

   Copyright 2024-2025 Free Software Foundation, Inc.

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

#include <dlfcn.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

typedef void (*setter_ftype) (int which, int val);

__thread int tls_main_tbss_1;
__thread int tls_main_tbss_2;
__thread int tls_main_tdata_1 = 96;
__thread int tls_main_tdata_2 = 97;

extern void set_tls_lib10_var (int which, int val);
extern void set_tls_lib11_var (int which, int val);

volatile int data;

static void
set_tls_main_var (int which, int val)
{
  switch (which)
    {
    case 1:
      tls_main_tbss_1 = val;
      break;
    case 2:
      tls_main_tbss_2 = val;
      break;
    case 3:
      tls_main_tdata_1 = val;
      break;
    case 4:
      tls_main_tdata_2 = val;
      break;
    }
}

void
use_it (int a)
{
  data = a;
}

static void *
load_dso (char *dso_name, int n, setter_ftype *setterp)
{
  char buf[80];
  void *sym;
  void *handle = dlopen (dso_name, RTLD_NOW | RTLD_GLOBAL);
  if (handle == NULL)
    {
      fprintf (stderr, "dlopen of DSO '%s' failed: %s\n", dso_name, dlerror ());
      exit (1);
    }
  sprintf (buf, "set_tls_lib%d_var", n);
  sym = dlsym (handle, buf);
  assert (sym != NULL);
  *setterp = sym;

  /* Some libc implementations (for some architectures) refuse to
     initialize TLS data structures (specifically, the DTV) without
     first calling dlsym on one of the TLS symbols.  */
  sprintf (buf, "tls_lib%d_tdata_1", n);
  assert (dlsym (handle, buf) != NULL);

  return handle;
}

int
main (int argc, char **argv)
{
  int i, status;
  setter_ftype s0, s1, s2, s3, s4, s10, s11;
  void *h1 = load_dso (OBJ1, 1, &s1);
  void *h2 = load_dso (OBJ2, 2, &s2);
  void *h3 = load_dso (OBJ3, 3, &s3);
  void *h4 = load_dso (OBJ4, 4, &s4);
  s0 = set_tls_main_var;
  s10 = set_tls_lib10_var;
  s11 = set_tls_lib11_var;

  use_it (0);		/* main-breakpoint-1 */

  /* Set TLS variables in main program and all libraries.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 10 + i);
  for (i = 1; i <= 4; i++)
    s1 (i, 110 + i);
  for (i = 1; i <= 4; i++)
    s2 (i, 210 + i);
  for (i = 1; i <= 4; i++)
    s3 (i, 310 + i);
  for (i = 1; i <= 4; i++)
    s4 (i, 410 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1010 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1110 + i);

  use_it (0);		/* main-breakpoint-2 */

  /* Unload lib2 and lib3.  */
  status = dlclose (h2);
  assert (status == 0);
  status = dlclose (h3);
  assert (status == 0);

  /* Set TLS variables in main program and in libraries which are still
     loaded.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 20 + i);
  for (i = 1; i <= 4; i++)
    s1 (i, 120 + i);
  for (i = 1; i <= 4; i++)
    s4 (i, 420 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1020 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1120 + i);

  use_it (0);		/* main-breakpoint-3 */

  /* Load lib3.  */
  h3 = load_dso (OBJ3, 3, &s3);

  /* Set TLS vars again; currently, only lib2 is not loaded.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 30 + i);
  for (i = 1; i <= 4; i++)
    s1 (i, 130 + i);
  for (i = 1; i <= 4; i++)
    s3 (i, 330 + i);
  for (i = 1; i <= 4; i++)
    s4 (i, 430 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1030 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1130 + i);

  use_it (0);		/* main-breakpoint-4 */

  /* Unload lib1 and lib4; load lib2.  */
  status = dlclose (h1);
  assert (status == 0);
  status = dlclose (h4);
  assert (status == 0);
  h2 = load_dso (OBJ2, 2, &s2);

  /* Set TLS vars; currently, lib2 and lib3 are loaded,
     lib1 and lib4 are not.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 40 + i);
  for (i = 1; i <= 4; i++)
    s2 (i, 240 + i);
  for (i = 1; i <= 4; i++)
    s3 (i, 340 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1040 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1140 + i);

  use_it (0);		/* main-breakpoint-5 */

  /* Load lib4 and lib1.  Unload lib2.  */
  h4 = load_dso (OBJ4, 4, &s4);
  h1 = load_dso (OBJ1, 1, &s1);
  status = dlclose (h2);
  assert (status == 0);

  /* Set TLS vars; currently, lib1, lib3, and lib4 are loaded;
     lib2 is not loaded.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 50 + i);
  for (i = 1; i <= 4; i++)
    s1 (i, 150 + i);
  for (i = 1; i <= 4; i++)
    s3 (i, 350 + i);
  for (i = 1; i <= 4; i++)
    s4 (i, 450 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1050 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1150 + i);

  use_it (0);		/* main-breakpoint-6 */

  /* Load lib2, unload lib1, lib3, and lib4; then load lib3 again.  */
  h2 = load_dso (OBJ2, 2, &s2);
  status = dlclose (h1);
  assert (status == 0);
  status = dlclose (h3);
  assert (status == 0);
  status = dlclose (h4);
  assert (status == 0);
  h3 = load_dso (OBJ3, 3, &s3);

  /* Set TLS vars; currently, lib2 and lib3 are loaded;
     lib1 and lib4 are not loaded.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 60 + i);
  for (i = 1; i <= 4; i++)
    s2 (i, 260 + i);
  for (i = 1; i <= 4; i++)
    s3 (i, 360 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1060 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1160 + i);

  use_it (0);		/* main-breakpoint-7 */

  /* Unload lib3 and lib2, then (re)load lib4, lib3, lib2, and lib1,
     in that order.  */
  status = dlclose (h3);
  assert (status == 0);
  status = dlclose (h2);
  assert (status == 0);
  h4 = load_dso (OBJ4, 4, &s4);
  h3 = load_dso (OBJ3, 3, &s3);
  h2 = load_dso (OBJ2, 2, &s2);
  h1 = load_dso (OBJ1, 1, &s1);

  /* Set TLS vars; currently, lib1, lib2, lib3, and lib4 are all
     loaded.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 70 + i);
  for (i = 1; i <= 4; i++)
    s1 (i, 170 + i);
  for (i = 1; i <= 4; i++)
    s2 (i, 270 + i);
  for (i = 1; i <= 4; i++)
    s3 (i, 370 + i);
  for (i = 1; i <= 4; i++)
    s4 (i, 470 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1070 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1170 + i);

  use_it (0);		/* main-breakpoint-8 */

  /* Unload lib3, lib1, and lib4.  */
  status = dlclose (h3);
  assert (status == 0);
  status = dlclose (h1);
  assert (status == 0);
  status = dlclose (h4);
  assert (status == 0);

  /* Set TLS vars; currently, lib2 is loaded; lib1, lib3, and lib4 are
     not.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 80 + i);
  for (i = 1; i <= 4; i++)
    s2 (i, 280 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1080 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1180 + i);

  use_it (0);		/* main-breakpoint-9 */

  /* Load lib3, unload lib2, load lib4.  */
  h3 = load_dso (OBJ3, 3, &s3);
  status = dlclose (h2);
  assert (status == 0);
  h4 = load_dso (OBJ4, 4, &s4);

  /* Set TLS vars; currently, lib3 and lib4 are loaded; lib1 and lib2
     are not.  */
  for (i = 1; i <= 4; i++)
    s0 (i, 90 + i);
  for (i = 1; i <= 4; i++)
    s3 (i, 390 + i);
  for (i = 1; i <= 4; i++)
    s4 (i, 490 + i);
  for (i = 1; i <= 4; i++)
    s10 (i, 1090 + i);
  for (i = 1; i <= 4; i++)
    s11 (i, 1190 + i);

  use_it (0);		/* main-breakpoint-10 */

  /* Attempt to keep variables in the main program from being optimized
     away.  */
  use_it (tls_main_tbss_1);
  use_it (tls_main_tbss_2);
  use_it (tls_main_tdata_1);
  use_it (tls_main_tdata_2);

  use_it (100);		/* main-breakpoint-last */

  return 0;
}
