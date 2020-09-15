/* This testcase is part of GDB, the GNU debugger.

   Copyright 1993-2020 Free Software Foundation, Inc.

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

#if defined __x86_64__
# define ASM_REG "rax"
#elif defined __i386__
# define ASM_REG "eax"
#else
# error "port me"
#endif

/* A class small enough that it fits in a register.  */
struct small
{
  int x;
  int method ();
};

int
small::method ()
{
  return x + 5;
}

int
register_class ()
{
  /* Given the use of the GNU register-asm local variables extension,
     the compiler puts this variable in a register.  This means that
     GDB can't call any methods for this variable, which is what we
     want to test.  */
  register small v asm (ASM_REG);

  int i;

  /* Perform a computation sufficiently complicated that optimizing
     compilers won't optimize out the variable.  If some compiler
     constant-folds this whole loop, maybe using a parameter to this
     function here would help.  */
  v.x = 0;
  for (i = 0; i < 13; ++i)
    v.x += i;
  --v.x; /* v.x is now 77 */
  return v.x + 5; /* set breakpoint here */
}

int
main ()
{
  register_class ();
  return 0;
}
