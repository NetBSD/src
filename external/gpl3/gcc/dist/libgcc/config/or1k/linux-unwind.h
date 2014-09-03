/* DWARF2 EH unwinding support for OpenRISC.
   Copyright (C) 2011, 2012
   Free Software Foundation, Inc.

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

#include <signal.h>
#include <sys/ucontext.h>
#include <linux/unistd.h>

#define MD_FALLBACK_FRAME_STATE_FOR or1k_fallback_frame_state

static _Unwind_Reason_Code
or1k_fallback_frame_state (struct _Unwind_Context *context,
			      _Unwind_FrameState *fs)
{
  struct rt_sigframe {
    siginfo_t info;
    struct ucontext uc;
  } *frame = context->cfa;

  struct sigcontext *sc;
  unsigned char *pc = context->ra;
  long new_cfa;
  int i;

  /*
   * Note: These have to be the same as in the kernel.
   * Please see arch/openrisc/kernel/signal.c
   */
  if (!(*(unsigned short *)(pc + 0) == 0xa960
      && *(unsigned short *)(pc + 2) == __NR_rt_sigreturn
      && *(unsigned long *)(pc + 4) == 0x20000001
      && *(unsigned long *)(pc + 8) == 0x15000000))
    return _URC_END_OF_STACK;

  sc = (struct sigcontext *) &frame->uc.uc_mcontext;

  new_cfa = sc->regs.gpr[1];
  fs->regs.cfa_how = CFA_REG_OFFSET;
  fs->regs.cfa_reg = STACK_POINTER_REGNUM;
  fs->regs.cfa_offset = new_cfa - (long) context->cfa;

  for (i = 0; i < 32; ++i)
    {
      fs->regs.reg[i].how = REG_SAVED_OFFSET;
      fs->regs.reg[i].loc.offset = (long)&sc->regs.gpr[i] - new_cfa;
    }

  fs->retaddr_column = 9;
  fs->signal_frame = 1;

  return _URC_NO_REASON;
}

#endif /* ifdef inhibit_libc  */
