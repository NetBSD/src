/* Native-dependent code for GNU/Linux x86 (i386 and x86-64).

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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

#include "x86-linux.h"
#include "x86-linux-dregs.h"
#include "nat/gdb_ptrace.h"
#include <sys/user.h>

/* Per-thread arch-specific data we want to keep.  */

struct arch_lwp_info
{
  /* Non-zero if our copy differs from what's recorded in the
     thread.  */
  int debug_registers_changed;
};

/* See nat/x86-linux.h.  */

void
lwp_set_debug_registers_changed (struct lwp_info *lwp, int value)
{
  if (lwp_arch_private_info (lwp) == NULL)
    lwp_set_arch_private_info (lwp, XCNEW (struct arch_lwp_info));

  lwp_arch_private_info (lwp)->debug_registers_changed = value;
}

/* See nat/x86-linux.h.  */

int
lwp_debug_registers_changed (struct lwp_info *lwp)
{
  struct arch_lwp_info *info = lwp_arch_private_info (lwp);

  /* NULL means either that this is the main thread still going
     through the shell, or that no watchpoint has been set yet.
     The debug registers are unchanged in either case.  */
  if (info == NULL)
    return 0;

  return info->debug_registers_changed;
}

/* See nat/x86-linux.h.  */

void
x86_linux_new_thread (struct lwp_info *lwp)
{
  lwp_set_debug_registers_changed (lwp, 1);
}

/* See nat/x86-linux.h.  */

void
x86_linux_delete_thread (struct arch_lwp_info *arch_lwp)
{
  xfree (arch_lwp);
}

/* See nat/x86-linux.h.  */

void
x86_linux_prepare_to_resume (struct lwp_info *lwp)
{
  x86_linux_update_debug_registers (lwp);
}

#ifdef __x86_64__
/* Value of CS segment register:
     64bit process: 0x33
     32bit process: 0x23  */
#define AMD64_LINUX_USER64_CS 0x33

/* Value of DS segment register:
     LP64 process: 0x0
     X32 process: 0x2b  */
#define AMD64_LINUX_X32_DS 0x2b
#endif

/* See nat/x86-linux.h.  */

x86_linux_arch_size
x86_linux_ptrace_get_arch_size (int tid)
{
#ifdef __x86_64__
  unsigned long cs;
  unsigned long ds;

  /* Get CS register.  */
  errno = 0;
  cs = ptrace (PTRACE_PEEKUSER, tid,
	       offsetof (struct user_regs_struct, cs), 0);
  if (errno != 0)
    perror_with_name (_("Couldn't get CS register"));

  bool is_64bit = cs == AMD64_LINUX_USER64_CS;

  /* Get DS register.  */
  errno = 0;
  ds = ptrace (PTRACE_PEEKUSER, tid,
	       offsetof (struct user_regs_struct, ds), 0);
  if (errno != 0)
    perror_with_name (_("Couldn't get DS register"));

  bool is_x32 = ds == AMD64_LINUX_X32_DS;

  return x86_linux_arch_size (is_64bit, is_x32);
#else
  return x86_linux_arch_size (false, false);
#endif
}
