/* Definitions to target GDB to GNU/Linux on 386.
   Copyright 1992, 1993 Free Software Foundation, Inc.

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

#ifndef TM_LINUX_H
#define TM_LINUX_H

#define I386_GNULINUX_TARGET
#define HAVE_I387_REGS
#ifdef HAVE_PTRACE_GETXFPREGS
#define HAVE_SSE_REGS
#endif

#include "i386/tm-i386.h"
#include "tm-linux.h"

/* FIXME: kettenis/2000-03-26: We should get rid of this last piece of
   Linux-specific `long double'-support code, probably by adding code
   to valprint.c:print_floating() to recognize various extended
   floating-point formats.  */

#if defined(HAVE_LONG_DOUBLE) && defined(HOST_I386)
/* The host and target are i386 machines and the compiler supports
   long doubles. Long doubles on the host therefore have the same
   layout as a 387 FPU stack register. */

#define TARGET_ANALYZE_FLOATING					\
  do								\
    {								\
      unsigned expon;						\
								\
      low = extract_unsigned_integer (valaddr, 4);		\
      high = extract_unsigned_integer (valaddr + 4, 4);		\
      expon = extract_unsigned_integer (valaddr + 8, 2);	\
								\
      nonnegative = ((expon & 0x8000) == 0);			\
      is_nan = ((expon & 0x7fff) == 0x7fff)			\
	&& ((high & 0x80000000) == 0x80000000)			\
	&& (((high & 0x7fffffff) | low) != 0);			\
    }								\
  while (0)

#endif

/* The following works around a problem with /usr/include/sys/procfs.h  */
#define sys_quotactl 1

/* When the i386 Linux kernel calls a signal handler, the return
   address points to a bit of code on the stack.  These definitions
   are used to identify this bit of code as a signal trampoline in
   order to support backtracing through calls to signal handlers.  */

#define IN_SIGTRAMP(pc, name) i386_linux_in_sigtramp (pc, name)
extern int i386_linux_in_sigtramp (CORE_ADDR, char *);

/* We need our own version of sigtramp_saved_pc to get the saved PC in
   a sigtramp routine.  */

#define sigtramp_saved_pc i386_linux_sigtramp_saved_pc
extern CORE_ADDR i386_linux_sigtramp_saved_pc (struct frame_info *);

/* Signal trampolines don't have a meaningful frame.  As in tm-i386.h,
   the frame pointer value we use is actually the frame pointer of the
   calling frame--that is, the frame which was in progress when the
   signal trampoline was entered.  gdb mostly treats this frame
   pointer value as a magic cookie.  We detect the case of a signal
   trampoline by looking at the SIGNAL_HANDLER_CALLER field, which is
   set based on IN_SIGTRAMP.

   When a signal trampoline is invoked from a frameless function, we
   essentially have two frameless functions in a row.  In this case,
   we use the same magic cookie for three frames in a row.  We detect
   this case by seeing whether the next frame has
   SIGNAL_HANDLER_CALLER set, and, if it does, checking whether the
   current frame is actually frameless.  In this case, we need to get
   the PC by looking at the SP register value stored in the signal
   context.

   This should work in most cases except in horrible situations where
   a signal occurs just as we enter a function but before the frame
   has been set up.  */

#define FRAMELESS_SIGNAL(FRAME)					\
  ((FRAME)->next != NULL					\
   && (FRAME)->next->signal_handler_caller			\
   && frameless_look_for_prologue (FRAME))

#undef FRAME_CHAIN
#define FRAME_CHAIN(FRAME)					\
  ((FRAME)->signal_handler_caller				\
   ? (FRAME)->frame						\
    : (FRAMELESS_SIGNAL (FRAME)					\
       ? (FRAME)->frame						\
       : (!inside_entry_file ((FRAME)->pc)			\
	  ? read_memory_integer ((FRAME)->frame, 4)		\
	  : 0)))

#undef FRAME_SAVED_PC
#define FRAME_SAVED_PC(FRAME)					\
  ((FRAME)->signal_handler_caller				\
   ? sigtramp_saved_pc (FRAME)					\
   : (FRAMELESS_SIGNAL (FRAME)					\
      ? read_memory_integer (i386_linux_sigtramp_saved_sp ((FRAME)->next), 4) \
      : read_memory_integer ((FRAME)->frame + 4, 4)))

extern CORE_ADDR i386_linux_sigtramp_saved_sp (struct frame_info *);

#undef SAVED_PC_AFTER_CALL
#define SAVED_PC_AFTER_CALL(frame) i386_linux_saved_pc_after_call (frame)
extern CORE_ADDR i386_linux_saved_pc_after_call (struct frame_info *);

/* When we call a function in a shared library, and the PLT sends us
   into the dynamic linker to find the function's real address, we
   need to skip over the dynamic linker call.  This function decides
   when to skip, and where to skip to.  See the comments for
   SKIP_SOLIB_RESOLVER at the top of infrun.c.  */
#define SKIP_SOLIB_RESOLVER i386_linux_skip_solib_resolver
extern CORE_ADDR i386_linux_skip_solib_resolver (CORE_ADDR pc);

/* N_FUN symbols in shared libaries have 0 for their values and need
   to be relocated. */
#define SOFUN_ADDRESS_MAYBE_MISSING

#endif /* #ifndef TM_LINUX_H */
