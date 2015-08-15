/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2014 Free Software Foundation, Inc.

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

#ifdef SYMBOL_PREFIX
#define SYMBOL(str)     SYMBOL_PREFIX #str
#else
#define SYMBOL(str)     #str
#endif

int globvar;

static void
begin (void)
{}

/* Called from asm.  */
static void __attribute__((used))
func (void)
{}

static void
marker (int anarg)
{
  /* `set_point' is the label at which to set a fast tracepoint.  The
     insn at the label must be large enough to fit a fast tracepoint
     jump.  */
  asm ("    .global " SYMBOL(set_point) "\n"
       SYMBOL(set_point) ":\n"
#if (defined __x86_64__ || defined __i386__)
       "    call " SYMBOL(func) "\n"
#endif
       );

  ++anarg;

  /* Set up a known 4-byte instruction so we can try to set a shorter
     fast tracepoint at it.  */
  asm ("    .global " SYMBOL(four_byter) "\n"
       SYMBOL(four_byter) ":\n"
#if (defined __i386__)
       "    cmpl $0x1,0x8(%ebp) \n"
#endif
       );
}

static void
end (void)
{}

int
main ()
{
  begin ();

  for (globvar = 1; globvar < 11; ++globvar)
    {
      marker (globvar * 100);
    }

  end ();
  return 0;
}
