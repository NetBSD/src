/* This testcase is part of GDB, the GNU debugger.

   Copyright 2024 Free Software Foundation, Inc.

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

static void inline_func_a (void);
static void inline_func_b (void);
static void normal_func (void);

volatile int global_var = 0;

static void __attribute__((noinline))
normal_func (void)
{
  /* Do some work.  */
  ++global_var;
  ++global_var;

  /* Now the inline function.  */
  inline_func_a ();

  /* Do some work.  */
  ++global_var;		/* After inline function.  */
  ++global_var;
}

static inline void __attribute__((__always_inline__))
inline_func_a (void)
{
  inline_func_b ();
}

static inline void __attribute__((__always_inline__))
inline_func_b (void)
{
  ++global_var;
  ++global_var;
}

int
main ()
{
  normal_func ();
  return 0;
}
