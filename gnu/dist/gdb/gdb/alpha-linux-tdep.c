/* Target-dependent code for GNU/Linux on Alpha.
   Copyright 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "frame.h"
#include "gdbcore.h"
#include "value.h"

#include "alpha-tdep.h"

/* Under GNU/Linux, signal handler invocations can be identified by the
   designated code sequence that is used to return from a signal
   handler.  In particular, the return address of a signal handler
   points to the following sequence (the first instruction is quadword
   aligned):
  
   bis $30,$30,$16
   addq $31,0x67,$0
   call_pal callsys 
      
   Each instruction has a unique encoding, so we simply attempt to
   match the instruction the pc is pointing to with any of the above
   instructions.  If there is a hit, we know the offset to the start
   of the designated sequence and can then check whether we really are
   executing in a designated sequence.  If not, -1 is returned,
   otherwise the offset from the start of the desingated sequence is
   returned.
   
   There is a slight chance of false hits: code could jump into the
   middle of the designated sequence, in which case there is no
   guarantee that we are in the middle of a sigreturn syscall.  Don't
   think this will be a problem in praxis, though.  */
LONGEST
alpha_linux_sigtramp_offset (CORE_ADDR pc)
{
  unsigned int i[3], w;
  long off;

  if (read_memory_nobpt (pc, (char *) &w, 4) != 0)
    return -1;

  off = -1;
  switch (w)
    {
    case 0x47de0410:
      off = 0;
      break;			/* bis $30,$30,$16 */
    case 0x43ecf400:
      off = 4;
      break;			/* addq $31,0x67,$0 */
    case 0x00000083:
      off = 8;
      break;			/* call_pal callsys */
    default:
      return -1;
    }
  pc -= off;
  if (pc & 0x7)
    {
      /* designated sequence is not quadword aligned */
      return -1;
    }
  if (read_memory_nobpt (pc, (char *) i, sizeof (i)) != 0)
    return -1;

  if (i[0] == 0x47de0410 && i[1] == 0x43ecf400 && i[2] == 0x00000083)
    return off;

  return -1;
}

static int
alpha_linux_pc_in_sigtramp (CORE_ADDR pc, char *func_name)
{
  return (alpha_linux_sigtramp_offset (pc) >= 0);
}

static CORE_ADDR
alpha_linux_sigcontext_addr (struct frame_info *frame)
{
  return (frame->frame - 0x298); /* sizeof(struct sigcontext) */
}

static void
alpha_linux_init_abi (struct gdbarch_info info,
                      struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  set_gdbarch_pc_in_sigtramp (gdbarch, alpha_linux_pc_in_sigtramp);

  tdep->dynamic_sigtramp_offset = alpha_linux_sigtramp_offset;
  tdep->sigcontext_addr = alpha_linux_sigcontext_addr;

  tdep->jb_pc = 2;
  tdep->jb_elt_size = 8;
}

void
_initialize_alpha_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_alpha, GDB_OSABI_LINUX,
                          alpha_linux_init_abi);
}
