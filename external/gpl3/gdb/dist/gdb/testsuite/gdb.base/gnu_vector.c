/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2013 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Ken Werner <ken.werner@de.ibm.com>  */

typedef int __attribute__ ((vector_size (4 * sizeof(int)))) int4;
typedef unsigned int __attribute__ ((vector_size (4 * sizeof(unsigned int)))) uint4;
typedef char __attribute__ ((vector_size (4 * sizeof(char)))) char4;
typedef float __attribute__ ((vector_size (4 * sizeof(float)))) float4;

typedef int __attribute__ ((vector_size (2 * sizeof(int)))) int2;
typedef long long __attribute__ ((vector_size (2 * sizeof(long long)))) longlong2;
typedef float __attribute__ ((vector_size (2 * sizeof(float)))) float2;
typedef double __attribute__ ((vector_size (2 * sizeof(double)))) double2;

int ia = 2;
int ib = 1;
float fa = 2;
float fb = 1;
long long lla __attribute__ ((mode(DI))) = 0x0000000100000001ll;
char4 c4 = {1, 2, 3, 4};
int4 i4a = {2, 4, 8, 16};
int4 i4b = {1, 2, 8, 4};
float4 f4a = {2, 4, 8, 16};
float4 f4b = {1, 2, 8, 4};
uint4 ui4 = {2, 4, 8, 16};
int2 i2 = {1, 2};
longlong2 ll2 = {1, 2};
float2 f2 = {1, 2};
double2 d2 = {1, 2};

union
{
  int i;
  char cv __attribute__ ((vector_size (sizeof (int))));
} union_with_vector_1;

struct
{
  int i;
  char cv __attribute__ ((vector_size (sizeof (int))));
  float4 f4;
} struct_with_vector_1;

int
main ()
{
  return 0;
}
