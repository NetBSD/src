/* Target-dependent code for Linux running on i386's, for GDB.
   Copyright (C) 2000 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "frame.h"
#include "value.h"


/* Recognizing signal handler frames.  */

/* Linux has two flavors of signals.  Normal signal handlers, and
   "realtime" (RT) signals.  The RT signals can provide additional
   information to the signal handler if the SA_SIGINFO flag is set
   when establishing a signal handler using `sigaction'.  It is not
   unlikely that future versions of Linux will support SA_SIGINFO for
   normal signals too.  */

/* When the i386 Linux kernel calls a signal handler and the
   SA_RESTORER flag isn't set, the return address points to a bit of
   code on the stack.  This function returns whether the PC appears to
   be within this bit of code.

   The instruction sequence for normal signals is
       pop    %eax
       mov    $0x77,%eax
       int    $0x80
   or 0x58 0xb8 0x77 0x00 0x00 0x00 0xcd 0x80.

   Checking for the code sequence should be somewhat reliable, because
   the effect is to call the system call sigreturn.  This is unlikely
   to occur anywhere other than a signal trampoline.

   It kind of sucks that we have to read memory from the process in
   order to identify a signal trampoline, but there doesn't seem to be
   any other way.  The IN_SIGTRAMP macro in tm-linux.h arranges to
   only call us if no function name could be identified, which should
   be the case since the code is on the stack.

   Detection of signal trampolines for handlers that set the
   SA_RESTORER flag is in general not possible.  Unfortunately this is
   what the GNU C Library has been doing for quite some time now.
   However, as of version 2.1.2, the GNU C Library uses signal
   trampolines (named __restore and __restore_rt) that are identical
   to the ones used by the kernel.  Therefore, these trampolines are
   supported too.  */

#define LINUX_SIGTRAMP_INSN0 (0x58)	/* pop %eax */
#define LINUX_SIGTRAMP_OFFSET0 (0)
#define LINUX_SIGTRAMP_INSN1 (0xb8)	/* mov $NNNN,%eax */
#define LINUX_SIGTRAMP_OFFSET1 (1)
#define LINUX_SIGTRAMP_INSN2 (0xcd)	/* int */
#define LINUX_SIGTRAMP_OFFSET2 (6)

static const unsigned char linux_sigtramp_code[] =
{
  LINUX_SIGTRAMP_INSN0,					/* pop %eax */
  LINUX_SIGTRAMP_INSN1, 0x77, 0x00, 0x00, 0x00,		/* mov $0x77,%eax */
  LINUX_SIGTRAMP_INSN2, 0x80				/* int $0x80 */
};

#define LINUX_SIGTRAMP_LEN (sizeof linux_sigtramp_code)

/* If PC is in a sigtramp routine, return the address of the start of
   the routine.  Otherwise, return 0.  */

static CORE_ADDR
i386_linux_sigtramp_start (CORE_ADDR pc)
{
  unsigned char buf[LINUX_SIGTRAMP_LEN];

  /* We only recognize a signal trampoline if PC is at the start of
     one of the three instructions.  We optimize for finding the PC at
     the start, as will be the case when the trampoline is not the
     first frame on the stack.  We assume that in the case where the
     PC is not at the start of the instruction sequence, there will be
     a few trailing readable bytes on the stack.  */

  if (read_memory_nobpt (pc, (char *) buf, LINUX_SIGTRAMP_LEN) != 0)
    return 0;

  if (buf[0] != LINUX_SIGTRAMP_INSN0)
    {
      int adjust;

      switch (buf[0])
	{
	case LINUX_SIGTRAMP_INSN1:
	  adjust = LINUX_SIGTRAMP_OFFSET1;
	  break;
	case LINUX_SIGTRAMP_INSN2:
	  adjust = LINUX_SIGTRAMP_OFFSET2;
	  break;
	default:
	  return 0;
	}

      pc -= adjust;

      if (read_memory_nobpt (pc, (char *) buf, LINUX_SIGTRAMP_LEN) != 0)
	return 0;
    }

  if (memcmp (buf, linux_sigtramp_code, LINUX_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* This function does the same for RT signals.  Here the instruction
   sequence is
       mov    $0xad,%eax
       int    $0x80
   or 0xb8 0xad 0x00 0x00 0x00 0xcd 0x80.

   The effect is to call the system call rt_sigreturn.  */

#define LINUX_RT_SIGTRAMP_INSN0 (0xb8)	/* mov $NNNN,%eax */
#define LINUX_RT_SIGTRAMP_OFFSET0 (0)
#define LINUX_RT_SIGTRAMP_INSN1 (0xcd)	/* int */
#define LINUX_RT_SIGTRAMP_OFFSET1 (5)

static const unsigned char linux_rt_sigtramp_code[] =
{
  LINUX_RT_SIGTRAMP_INSN0, 0xad, 0x00, 0x00, 0x00,	/* mov $0xad,%eax */
  LINUX_RT_SIGTRAMP_INSN1, 0x80				/* int $0x80 */
};

#define LINUX_RT_SIGTRAMP_LEN (sizeof linux_rt_sigtramp_code)

/* If PC is in a RT sigtramp routine, return the address of the start
   of the routine.  Otherwise, return 0.  */

static CORE_ADDR
i386_linux_rt_sigtramp_start (CORE_ADDR pc)
{
  unsigned char buf[LINUX_RT_SIGTRAMP_LEN];

  /* We only recognize a signal trampoline if PC is at the start of
     one of the two instructions.  We optimize for finding the PC at
     the start, as will be the case when the trampoline is not the
     first frame on the stack.  We assume that in the case where the
     PC is not at the start of the instruction sequence, there will be
     a few trailing readable bytes on the stack.  */

  if (read_memory_nobpt (pc, (char *) buf, LINUX_RT_SIGTRAMP_LEN) != 0)
    return 0;

  if (buf[0] != LINUX_RT_SIGTRAMP_INSN0)
    {
      if (buf[0] != LINUX_RT_SIGTRAMP_INSN1)
	return 0;

      pc -= LINUX_RT_SIGTRAMP_OFFSET1;

      if (read_memory_nobpt (pc, (char *) buf, LINUX_RT_SIGTRAMP_LEN) != 0)
	return 0;
    }

  if (memcmp (buf, linux_rt_sigtramp_code, LINUX_RT_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* Return whether PC is in a Linux sigtramp routine.  */

int
i386_linux_in_sigtramp (CORE_ADDR pc, char *name)
{
  if (name)
    return STREQ ("__restore", name) || STREQ ("__restore_rt", name);
  
  return (i386_linux_sigtramp_start (pc) != 0
	  || i386_linux_rt_sigtramp_start (pc) != 0);
}

/* Assuming FRAME is for a Linux sigtramp routine, return the address
   of the associated sigcontext structure.  */

CORE_ADDR
i386_linux_sigcontext_addr (struct frame_info *frame)
{
  CORE_ADDR pc;

  pc = i386_linux_sigtramp_start (frame->pc);
  if (pc)
    {
      CORE_ADDR sp;

      if (frame->next)
	/* If this isn't the top frame, the next frame must be for the
	   signal handler itself.  The sigcontext structure lives on
	   the stack, right after the signum argument.  */
	return frame->next->frame + 12;

      /* This is the top frame.  We'll have to find the address of the
	 sigcontext structure by looking at the stack pointer.  Keep
	 in mind that the first instruction of the sigtramp code is
	 "pop %eax".  If the PC is at this instruction, adjust the
	 returned value accordingly.  */
      sp = read_register (SP_REGNUM);
      if (pc == frame->pc)
	return sp + 4;
      return sp;
    }

  pc = i386_linux_rt_sigtramp_start (frame->pc);
  if (pc)
    {
      if (frame->next)
	/* If this isn't the top frame, the next frame must be for the
	   signal handler itself.  The sigcontext structure is part of
	   the user context.  A pointer to the user context is passed
	   as the third argument to the signal handler.  */
	return read_memory_integer (frame->next->frame + 16, 4) + 20;

      /* This is the top frame.  Again, use the stack pointer to find
	 the address of the sigcontext structure.  */
      return read_memory_integer (read_register (SP_REGNUM) + 8, 4) + 20;
    }

  error ("Couldn't recognize signal trampoline.");
  return 0;
}

/* Offset to saved PC in sigcontext, from <asm/sigcontext.h>.  */
#define LINUX_SIGCONTEXT_PC_OFFSET (56)

/* Assuming FRAME is for a Linux sigtramp routine, return the saved
   program counter.  */

CORE_ADDR
i386_linux_sigtramp_saved_pc (struct frame_info *frame)
{
  CORE_ADDR addr;
  addr = i386_linux_sigcontext_addr (frame);
  return read_memory_integer (addr + LINUX_SIGCONTEXT_PC_OFFSET, 4);
}

/* Offset to saved SP in sigcontext, from <asm/sigcontext.h>.  */
#define LINUX_SIGCONTEXT_SP_OFFSET (28)

/* Assuming FRAME is for a Linux sigtramp routine, return the saved
   stack pointer.  */

CORE_ADDR
i386_linux_sigtramp_saved_sp (struct frame_info *frame)
{
  CORE_ADDR addr;
  addr = i386_linux_sigcontext_addr (frame);
  return read_memory_integer (addr + LINUX_SIGCONTEXT_SP_OFFSET, 4);
}

/* Immediately after a function call, return the saved pc.  */

CORE_ADDR
i386_linux_saved_pc_after_call (struct frame_info *frame)
{
  if (frame->signal_handler_caller)
    return i386_linux_sigtramp_saved_pc (frame);

  return read_memory_integer (read_register (SP_REGNUM), 4);
}
