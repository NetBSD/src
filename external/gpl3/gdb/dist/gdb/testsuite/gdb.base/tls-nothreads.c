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

__thread int tls_tbss_1;
__thread int tls_tbss_2;
__thread int tls_tbss_3;

__thread int tls_tdata_1 = 21;
__thread int tls_tdata_2 = 22;
__thread int tls_tdata_3 = 23;

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

  tls_tbss_1 = 24;	/* main-breakpoint-1 */
  tls_tbss_2 = 25;
  tls_tbss_3 = 26;

  tls_tdata_1 = 42;
  tls_tdata_2 = 43;
  tls_tdata_3 = 44;

  use_it (tls_tbss_1);
  use_it (tls_tbss_2);
  use_it (tls_tbss_3);
  use_it (tls_tdata_1);
  use_it (tls_tdata_2);
  use_it (tls_tdata_3);

  use_it (100);		/* main-breakpoint-2 */

  return 0;
}
