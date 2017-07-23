/* DWARF2 EH unwinding support for RISC-V Linux.
   Copyright (C) 2014 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#ifndef inhibit_libc
/* Examine the code and attempt to identify a signal frame. */

#include <stdlib.h>
#include <asm-generic/unistd.h>

#define MD_FALLBACK_FRAME_STATE_FOR riscv_fallback_frame_state

static _Unwind_Reason_Code
riscv_fallback_frame_state (struct _Unwind_Context *context,
			    _Unwind_FrameState *fs)
{
  unsigned int *pc = (unsigned int *) context->ra;

  /* Signal frames begin with the following code sequence:
      li v0, __NR_rt_sigreturn
      scall */
  if (((unsigned long)pc & 0x3) != 0
      || pc[0] != RISCV_ITYPE (ADDI, GP_RETURN, 0, __NR_rt_sigreturn)
      || pc[1] != RISCV_ITYPE (SCALL, 0, 0, 0))
    return _URC_END_OF_STACK;

  /* TODO: Actually implement this. */
  abort();
}
#endif
