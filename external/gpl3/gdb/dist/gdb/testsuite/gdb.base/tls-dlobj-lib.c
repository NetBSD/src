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

/* This program needs to be compiled with preprocessor symbol set to
   a small integer, e.g. "gcc -DN=1 ..."  With N defined, the CONCAT2
   and CONCAT3 macros will construct suitable names for the global
   variables and functions.  */

#define CONCAT2(a,b) CONCAT2_(a,b)
#define CONCAT2_(a,b) a ## b

#define CONCAT3(a,b,c) CONCAT3_(a,b,c)
#define CONCAT3_(a,b,c) a ## b ## c

/* For N=1, this ends up being...
   __thread int tls_lib1_tbss_1;
   __thread int tls_lib1_tbss_2;
   __thread int tls_lib1_tdata_1 = 196;
   __thread int tls_lib1_tdata_2 = 197; */

__thread int CONCAT3(tls_lib, N, _tbss_1);
__thread int CONCAT3(tls_lib, N, _tbss_2);
__thread int CONCAT3(tls_lib, N, _tdata_1) = CONCAT2(N, 96);
__thread int CONCAT3(tls_lib, N,  _tdata_2) = CONCAT2(N, 97);

/* Substituting for N, define function:

    int get_tls_libN_var (int which) .  */

int
CONCAT3(get_tls_lib, N, _var) (int which)
{
  switch (which)
    {
    case 0:
      return -1;
    case 1:
      return CONCAT3(tls_lib, N, _tbss_1);
    case 2:
      return CONCAT3(tls_lib, N, _tbss_2);
    case 3:
      return CONCAT3(tls_lib, N, _tdata_1);
    case 4:
      return CONCAT3(tls_lib, N, _tdata_2);
    }
  return -1;
}

/* Substituting for N, define function:

   void set_tls_libN_var (int which, int val) .  */

void
CONCAT3(set_tls_lib, N, _var) (int which, int val)
{
  switch (which)
    {
    case 0:
      break;
    case 1:
      CONCAT3(tls_lib, N, _tbss_1) = val;
      break;
    case 2:
      CONCAT3(tls_lib, N, _tbss_2) = val;
      break;
    case 3:
      CONCAT3(tls_lib, N, _tdata_1) = val;
      break;
    case 4:
      CONCAT3(tls_lib, N, _tdata_2) = val;
      break;
    }
}
