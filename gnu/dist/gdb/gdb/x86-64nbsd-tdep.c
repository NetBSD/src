/* Target-dependent code for NetBSD running on x86-64, for GDB.
   Based on the Linux version.

   Copyright 2001 Free Software Foundation, Inc.

   Contributed by Wasabi Systems, Inc.

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
#include "inferior.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "gdb_string.h"
#include "osabi.h"
#include "regcache.h"
#include "solib-svr4.h"
#include "x86-64-tdep.h"
#include "dwarf2cfi.h"
#include "frame.h"
#include "arch-utils.h"

#include "nbsd-tdep.h"

#define NETBSD_SIGTRAMP_INSN0	0x48
#define NETBSD_SIGTRAMP_OFFSET0	0
#define NETBSD_SIGTRAMP_INSN1	0x57
#define NETBSD_SIGTRAMP_OFFSET1	3
#define NETBSD_SIGTRAMP_INSN2	0xb8
#define NETBSD_SIGTRAMP_OFFSET2	4
#define NETBSD_SIGTRAMP_INSN3	0xcd
#define NETBSD_SIGTRAMP_OFFSET3	9
#define NETBSD_SIGTRAMP_INSN4	0xb8
#define NETBSD_SIGTRAMP_OFFSET4	11
#define NETBSD_SIGTRAMP_INSN5	0x0f
#define NETBSD_SIGTRAMP_OFFSET5	16

#define NETBSD_SIGTRAMP_NINSNS	6

static const unsigned char nbsd_sigtramp_code[] = {
  /* movq %rsp,%rdi */
  NETBSD_SIGTRAMP_INSN0, 0x89, 0xe7,
  /* pushq %rdi */
  NETBSD_SIGTRAMP_INSN1,
  /* movl $SYS___sigreturn14 ,%eax */
  NETBSD_SIGTRAMP_INSN2, 0x27, 0x01, 0x00, 0x00,
  /* int $0x80 */
  NETBSD_SIGTRAMP_INSN3, 0x80,
  /* movl $SYS_exit, %eax */
  NETBSD_SIGTRAMP_INSN4, 0x01, 0x00, 0x00, 0x00,
  /* syscall */
  NETBSD_SIGTRAMP_INSN5, 0x05,
};

static const unsigned char nbsd_sigtramp_insbytes[] = {
  NETBSD_SIGTRAMP_INSN0, NETBSD_SIGTRAMP_INSN1,
  NETBSD_SIGTRAMP_INSN2, NETBSD_SIGTRAMP_INSN3,
  NETBSD_SIGTRAMP_INSN4, NETBSD_SIGTRAMP_INSN5
};

static const unsigned char nbsd_sigtramp_offsets[] = {
  NETBSD_SIGTRAMP_OFFSET0, NETBSD_SIGTRAMP_OFFSET1,
  NETBSD_SIGTRAMP_OFFSET2, NETBSD_SIGTRAMP_OFFSET3,
  NETBSD_SIGTRAMP_OFFSET4, NETBSD_SIGTRAMP_OFFSET5
};

#define NETBSD_SIGTRAMP_LEN (sizeof nbsd_sigtramp_code)

/* If PC is in a sigtramp routine, return the address of the start of
   the routine.  Otherwise, return 0.  */

static CORE_ADDR
x86_64_nbsd_sigtramp_start (CORE_ADDR pc)
{
  unsigned char buf[NETBSD_SIGTRAMP_LEN], offset;
  CORE_ADDR spc = 0;
  int i;

  for (i = 0; i < NETBSD_SIGTRAMP_NINSNS; i++) {
    offset = nbsd_sigtramp_offsets[i];
    spc = pc - offset;

    if (read_memory_nobpt (spc, (char *) buf, NETBSD_SIGTRAMP_LEN) != 0)
      return 0;

    if (buf[offset] == nbsd_sigtramp_insbytes[i])
      break;
  }
  if (i == NETBSD_SIGTRAMP_NINSNS)
    return 0;

  if (memcmp (buf, nbsd_sigtramp_code, NETBSD_SIGTRAMP_LEN) != 0)
    return 0;

  return spc;
}

/* Assuming FRAME is for a NetBSD sigtramp routine, return the
   address of the associated sigcontext structure.  */
static CORE_ADDR
x86_64_nbsd_sigcontext_addr (struct frame_info *frame)
{
  CORE_ADDR pc;
  ULONGEST rsp;

  pc = x86_64_nbsd_sigtramp_start (get_frame_pc (frame));
  if (pc)
    {
      if (frame->next)
	/* If this isn't the top frame, the next frame must be for the
	   signal handler itself.  The sigcontext structure is part of
	   the user context. */
	return frame->next->frame;

      /* This is the top frame. */
      rsp = read_register (SP_REGNUM);
      return rsp;

    }

  error ("Couldn't recognize signal trampoline.");
  return 0;
}

#define NETBSD_SIGCONTEXT_PC_OFFSET 360
#define NETBSD_SIGCONTEXT_FP_OFFSET 384

/* Assuming FRAME is for a NetBSD sigtramp routine, return the
   saved program counter.  */

static CORE_ADDR
x86_64_nbsd_sigtramp_saved_pc (struct frame_info *frame)
{
  CORE_ADDR addr;

  addr = x86_64_nbsd_sigcontext_addr (frame);
  return read_memory_integer (addr + NETBSD_SIGCONTEXT_PC_OFFSET, 8);
}

/* Immediately after a function call, return the saved pc.  */

CORE_ADDR
x86_64_nbsd_saved_pc_after_call (struct frame_info *frame)
{
  if (frame->signal_handler_caller)
    return x86_64_nbsd_sigtramp_saved_pc (frame);
  return read_memory_integer (read_register (SP_REGNUM), 8);
}

/* Saved Pc.  Get it from sigcontext if within sigtramp.  */
CORE_ADDR
x86_64_nbsd_frame_saved_pc (struct frame_info *frame)
{
  if (frame->signal_handler_caller)
    return x86_64_nbsd_sigtramp_saved_pc (frame);
  return cfi_get_ra (frame);
}

CORE_ADDR
x86_64_nbsd_frame_chain (struct frame_info *fi)
{
  CORE_ADDR fp, addr;

  if (fi->signal_handler_caller) {
    addr = x86_64_nbsd_sigcontext_addr (fi);
    fp = read_memory_integer (addr + NETBSD_SIGCONTEXT_FP_OFFSET, 8) + 8;
  } else {
    if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
      fp = fi->frame;
    else
      fp = cfi_frame_chain (fi);
  }
  return fp;
}

/* Return whether PC is in a NetBSD sigtramp routine.  */

int
x86_64_nbsd_in_sigtramp (CORE_ADDR pc, char *name)
{
  return (nbsd_pc_in_sigtramp (pc, name)
	  || x86_64_nbsd_sigtramp_start (pc) != 0);
}

/* NetBSD ELF.  */
static void
x86_64_nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
#if 0
  set_gdbarch_in_solib_call_trampoline (gdbarch,
                                        generic_in_solib_call_trampoline);
#endif
  set_solib_svr4_fetch_link_map_offsets (gdbarch,
                                 nbsd_lp64_solib_svr4_fetch_link_map_offsets);
  set_gdbarch_pc_in_sigtramp (gdbarch, x86_64_nbsd_in_sigtramp);
  set_gdbarch_frame_chain (gdbarch, x86_64_nbsd_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, func_frame_chain_valid);
  set_gdbarch_frame_saved_pc (gdbarch, x86_64_nbsd_frame_saved_pc);
  set_gdbarch_saved_pc_after_call (gdbarch, x86_64_nbsd_saved_pc_after_call);
}

void
_initialize_x86_64nbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_i386, GDB_OSABI_NETBSD_ELF,
                          x86_64_nbsd_init_abi);
}
