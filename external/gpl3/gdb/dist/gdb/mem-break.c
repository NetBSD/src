/* Simulate breakpoints by patching locations in the target system, for GDB.

   Copyright (C) 1990-2013 Free Software Foundation, Inc.

   Contributed by Cygnus Support.  Written by John Gilmore.

   This file is part of GDB.

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

#include "defs.h"
#include "symtab.h"
#include "breakpoint.h"
#include "inferior.h"
#include "target.h"
#include "gdb_string.h"


/* Insert a breakpoint on targets that don't have any better
   breakpoint support.  We read the contents of the target location
   and stash it, then overwrite it with a breakpoint instruction.
   BP_TGT->placed_address is the target location in the target
   machine.  BP_TGT->shadow_contents is some memory allocated for
   saving the target contents.  It is guaranteed by the caller to be
   long enough to save BREAKPOINT_LEN bytes (this is accomplished via
   BREAKPOINT_MAX).  */

int
default_memory_insert_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt)
{
  int val;
  const unsigned char *bp;
  gdb_byte *readbuf;

  /* Determine appropriate breakpoint contents and size for this address.  */
  bp = gdbarch_breakpoint_from_pc
       (gdbarch, &bp_tgt->placed_address, &bp_tgt->placed_size);
  if (bp == NULL)
    error (_("Software breakpoints not implemented for this target."));

  /* Save the memory contents in the shadow_contents buffer and then
     write the breakpoint instruction.  */
  bp_tgt->shadow_len = bp_tgt->placed_size;
  readbuf = alloca (bp_tgt->placed_size);
  val = target_read_memory (bp_tgt->placed_address, readbuf,
			    bp_tgt->placed_size);
  if (val == 0)
    {
      memcpy (bp_tgt->shadow_contents, readbuf, bp_tgt->placed_size);
      val = target_write_raw_memory (bp_tgt->placed_address, bp,
				     bp_tgt->placed_size);
    }

  return val;
}


int
default_memory_remove_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt)
{
  return target_write_raw_memory (bp_tgt->placed_address, bp_tgt->shadow_contents,
				  bp_tgt->placed_size);
}


int
memory_insert_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  return gdbarch_memory_insert_breakpoint (gdbarch, bp_tgt);
}

int
memory_remove_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  return gdbarch_memory_remove_breakpoint (gdbarch, bp_tgt);
}
