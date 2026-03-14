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

__thread int tls_main_tbss_1;
__thread int tls_main_tbss_2;
__thread int tls_main_tdata_1 = 96;
__thread int tls_main_tdata_2 = 97;

extern __thread int tls_lib1_tbss_1;
extern __thread int tls_lib1_tbss_2;
extern __thread int tls_lib1_tdata_1;
extern __thread int tls_lib1_tdata_2;

extern __thread int tls_lib2_tbss_1;
extern __thread int tls_lib2_tbss_2;
extern __thread int tls_lib2_tdata_1;
extern __thread int tls_lib2_tdata_2;

extern __thread int tls_lib3_tbss_1;
extern __thread int tls_lib3_tbss_2;
extern __thread int tls_lib3_tdata_1;
extern __thread int tls_lib3_tdata_2;

extern void lib1_func ();
extern void lib2_func ();
extern void lib3_func ();

volatile int data;

void
use_it (int a)
{
  data = a;
}

int
main (int argc, char **argv)
{
  use_it (-1);

  tls_main_tbss_1 = 51;	/* main-breakpoint-1 */
  tls_main_tbss_2 = 52;
  tls_main_tdata_1 = 53;
  tls_main_tdata_2 = 54;

  tls_lib1_tbss_1 = 151;
  tls_lib1_tbss_2 = 152;
  tls_lib1_tdata_1 = 153;
  tls_lib1_tdata_2 = 154;

  tls_lib2_tbss_1 = 251;
  tls_lib2_tbss_2 = 252;
  tls_lib2_tdata_1 = 253;
  tls_lib2_tdata_2 = 254;

  tls_lib3_tbss_1 = 351;
  tls_lib3_tbss_2 = 352;
  tls_lib3_tdata_1 = 353;
  tls_lib3_tdata_2 = 354;

  lib1_func ();
  lib2_func ();
  lib3_func ();

  /* Attempt to keep variables in the main program from being optimized
     away.  */
  use_it (tls_main_tbss_1);
  use_it (tls_main_tbss_2);
  use_it (tls_main_tdata_1);
  use_it (tls_main_tdata_2);

  use_it (100);		/* main-breakpoint-2 */

  return 0;
}
