/* Target-dependent code for OSF/1 on Alpha.
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

/* Under OSF/1, the __sigtramp routine is frameless and has a frame
   size of zero, but we are able to backtrace through it.  */
static CORE_ADDR
alpha_osf1_skip_sigtramp_frame (struct frame_info *frame, CORE_ADDR pc)
{
  char *name;

  find_pc_partial_function (pc, &name, (CORE_ADDR *) NULL, (CORE_ADDR *) NULL);
  if (PC_IN_SIGTRAMP (pc, name))
    return frame->frame;
  return 0;
}

static int
alpha_osf1_pc_in_sigtramp (CORE_ADDR pc, char *func_name)
{
  return (func_name != NULL && STREQ ("__sigtramp", func_name));
}

static CORE_ADDR
alpha_osf1_sigcontext_addr (struct frame_info *frame)
{
  return (read_memory_integer (frame->next ? frame->next->frame
					   : frame->frame, 8));
}

static void
alpha_osf1_init_abi (struct gdbarch_info info,
                     struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  set_gdbarch_pc_in_sigtramp (gdbarch, alpha_osf1_pc_in_sigtramp);
  /* The next/step support via procfs on OSF1 is broken when running
     on multi-processor machines. We need to use software single stepping
     instead.  */
  set_gdbarch_software_single_step (gdbarch, alpha_software_single_step);

  tdep->skip_sigtramp_frame = alpha_osf1_skip_sigtramp_frame;
  tdep->sigcontext_addr = alpha_osf1_sigcontext_addr;

  tdep->jb_pc = 2;
  tdep->jb_elt_size = 8;
}

void
_initialize_alpha_osf1_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_alpha, GDB_OSABI_OSF1, alpha_osf1_init_abi);
}
