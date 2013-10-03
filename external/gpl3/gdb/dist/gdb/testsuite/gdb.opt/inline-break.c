/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2012-2013 Free Software Foundation, Inc.

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

/* The file ../gdb.dwarf2/inline-break.S was generated manually from
   this file, and should be regenerated if this file is modified.  */

#ifdef __GNUC__
# define ATTR __attribute__((always_inline))
#else
# define ATTR
#endif

/* A static inlined function that is called once.  */

static inline ATTR int
func1 (int x)
{
  return x * 23;
}

/* A non-static inlined function that is called once.  */

inline ATTR int
func2 (int x)
{
  return x * 17;
}

/* A static inlined function that calls another static inlined
   function.  */

static inline ATTR int
func3b (int x)
{
  return x < 14 ? 1 : 2;
}

static inline ATTR int
func3a (int x)
{
  return func3b (x * 23);
}

/* A non-static inlined function that calls a static inlined
   function.  */

static inline ATTR int
func4b (int x)
{
  return x < 13 ? 1 : 2;
}

inline ATTR int
func4a (int x)
{
  return func4b (x * 17);
}

/* A static inlined function that calls a non-static inlined
   function.  */

inline ATTR int
func5b (int x)
{
  return x < 12 ? 1 : 2;
}

static inline ATTR int
func5a (int x)
{
  return func5b (x * 23);
}

/* A non-static inlined function that calls another non-static inlined
   function.  */

inline ATTR int
func6b (int x)
{
  return x < 14 ? 3 : 2;
}

inline ATTR int
func6a (int x)
{
  return func6b (x * 17);
}

/* A static inlined function that is called more than once.  */

static inline ATTR int
func7b (int x)
{
  return x < 23 ? 1 : 4;
}

static inline ATTR int
func7a (int x)
{
  return func7b (x * 29);
}

/* A non-static inlined function that is called more than once.  */

inline ATTR int
func8b (int x)
{
  return x < 7 ? 11 : 9;
}

static inline ATTR int
func8a (int x)
{
  return func8b (x * 31);
}

/* Entry point.  */

int
main (int argc, char *argv[])
{
  /* Declaring x as volatile here prevents GCC from combining calls.
     If GCC is allowed to combine calls then some of them end up with
     no instructions at all, so there is no specific address for GDB
     to set a breakpoint at.  */
  volatile int x = argc;

  x = func1 (x);

  x = func2 (x);

  x = func3a (x);

  x = func4a (x);

  x = func5a (x);

  x = func6a (x);

  x = func7a (x) + func7b (x);

  x = func8a (x) + func8b (x);

  return x;
}
