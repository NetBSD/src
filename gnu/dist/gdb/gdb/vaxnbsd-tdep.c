/* Target-dependent code for NetBSD running on DEC VAX, for GDB.
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
#include "vax-tdep.h"
#include "dwarf2cfi.h"
#include "frame.h"
#include "arch-utils.h"

#include "nbsd-tdep.h"

#define NETBSD_SIGTRAMP_INSN0		0xbb
#define NETBSD_SIGTRAMP_OFFSET0		0
#define NETBSD_SIGTRAMP_INSN1		0xc2
#define NETBSD_SIGTRAMP_OFFSET1		2
#define NETBSD_SIGTRAMP_INSN2		0xd0
#define NETBSD_SIGTRAMP_OFFSET2		5
#define NETBSD_SIGTRAMP_INSN3		0xfb
#define NETBSD_SIGTRAMP_OFFSET3		9
#define NETBSD_SIGTRAMP_INSN4		0xba
#define NETBSD_SIGTRAMP_OFFSET4		12
#define NETBSD_SIGTRAMP_INSN5		0xbc
#define NETBSD_SIGTRAMP_OFFSET5		14
#define NETBSD_SIGTRAMP_INSN6		0xbc
#define NETBSD_SIGTRAMP_OFFSET6		18

#define NETBSD_SIGTRAMP_NINSNS		7

static const unsigned char nbsd_sigtramp_code[] = {
  NETBSD_SIGTRAMP_INSN0, 0x3f,			/* pushr $0x3f */
  NETBSD_SIGTRAMP_INSN1, 0x0c, 0x5e,		/* subl2 $0xc, %sp */
  NETBSD_SIGTRAMP_INSN2, 0xae, 0x23, 0x50,	/* movl 0x24(%sp),%r0 */
  NETBSD_SIGTRAMP_INSN3, 0x03, 0x60,		/* calls $3,(%r0) */
  NETBSD_SIGTRAMP_INSN4, 0x3f,			/* popr $0x3f */
  NETBSD_SIGTRAMP_INSN5, 0x8f, 0x27, 0x01,	/* chmk $SYS_sigreturn14 */
  NETBSD_SIGTRAMP_INSN6, 0x01,			/* chmk $SYS_exit */
};

static const unsigned char nbsd_sigtramp_insbytes[] = {
  NETBSD_SIGTRAMP_INSN0, NETBSD_SIGTRAMP_INSN1,
  NETBSD_SIGTRAMP_INSN2, NETBSD_SIGTRAMP_INSN3,
  NETBSD_SIGTRAMP_INSN4, NETBSD_SIGTRAMP_INSN5,
  NETBSD_SIGTRAMP_INSN6,
};

static const unsigned char nbsd_sigtramp_offsets[] = {
  NETBSD_SIGTRAMP_OFFSET0, NETBSD_SIGTRAMP_OFFSET1,
  NETBSD_SIGTRAMP_OFFSET2, NETBSD_SIGTRAMP_OFFSET3,
  NETBSD_SIGTRAMP_OFFSET4, NETBSD_SIGTRAMP_OFFSET5,
  NETBSD_SIGTRAMP_OFFSET6,
};

#define NETBSD_SIGTRAMP_LEN (sizeof nbsd_sigtramp_code)

/* If PC is in a sigtramp routine, return the address of the start of
   the routine.  Otherwise, return 0.  */

static CORE_ADDR
vax_nbsd_sigtramp_start (CORE_ADDR pc)
{
  unsigned char buf[NETBSD_SIGTRAMP_LEN], offset;
  CORE_ADDR spc = 0;
  int i, j;

  for (i = 0; i < NETBSD_SIGTRAMP_NINSNS; i++) {
    offset = nbsd_sigtramp_offsets[i];
    spc = pc - offset;

    if (read_memory_nobpt (spc, (char *) buf, sizeof(buf)) != 0)
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
vax_nbsd_sigcontext_addr (struct frame_info *frame)
{
#if 0
  CORE_ADDR pc;

  pc = vax_nbsd_sigtramp_start (get_frame_pc (frame));
  if (pc)
    {
      if (frame->next)
	/* If this isn't the top frame, the next frame must be for the
	   signal handler itself.  The sigcontext structure is part of
	   the user context. */
        return read_memory_integer(frame->next->frame + 8, 4);
	return frame->next->frame;

      /* This is the top frame. */
      return read_register (VAX_AP_REGNUM);

    }
#endif
  if (frame->next)
    return read_memory_integer(frame->next->frame + 8, 4);
  return read_register (VAX_AP_REGNUM);

#if 0
  error ("Couldn't recognize signal trampoline.");
  return 0;
#endif
}

/* Assuming FRAME is for a NetBSD sigtramp routine, return the
   saved program counter.  */

static CORE_ADDR
vax_nbsd_sigtramp_saved_pc (struct frame_info *frame)
{
  CORE_ADDR addr;
  int argc;

  addr = vax_nbsd_sigcontext_addr (frame);	/* AP */
  argc = read_memory_integer (addr, 4);		/* get arg count */
  if (argc == 3)
    {
      addr = read_memory_integer (addr + 12, 4); /* get sigcontext addr */
      return read_memory_integer (addr + 20, 4); /* get PC in sigcontext */
    }
  else
    {
      addr = read_memory_integer (addr + 8, 4); /* get ucontext addr */
      return read_memory_integer (addr + 96, 4); /* get PC in ucontext */
    }
}

/* Immediately after a function call, return the saved pc.  */

CORE_ADDR
vax_nbsd_saved_pc_after_call (struct frame_info *frame)
{
  if (frame->signal_handler_caller)
    return vax_nbsd_sigtramp_saved_pc (frame);
  return read_memory_integer (read_register (VAX_FP_REGNUM) + 16, 4);
}

/* Saved Pc.  Get it from sigcontext if within sigtramp.  */
CORE_ADDR
vax_nbsd_frame_saved_pc (struct frame_info *frame)
{
  if (frame->signal_handler_caller)
    return vax_nbsd_sigtramp_saved_pc (frame);
  return read_memory_integer (frame->frame + 16, 4);
}

CORE_ADDR
vax_nbsd_frame_chain (struct frame_info *frame)
{
  CORE_ADDR fp, addr;
  int argc;

  if (frame->signal_handler_caller) {
    addr = vax_nbsd_sigcontext_addr (frame);
    argc = read_memory_integer (addr, 4);	/* get sigcontext addr */
    if (argc == 3)
      {
        addr = read_memory_integer (addr + 12, 4); /* get sigcontext addr */
        fp = read_memory_integer (addr + 12, 4); /* get FP in sigcontext */
      }
    else
      {
        addr = read_memory_integer (addr + 8, 4); /* get ucontext addr */
        fp = read_memory_integer (addr + 88, 4); /* get FP in ucontext */
      }
  } else {
    if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
      fp = frame->frame;
    else
      fp = read_memory_integer (frame->frame + 12, 4);
  }
  return fp;
}

/* Return whether PC is in a NetBSD sigtramp routine.  */

int
vax_nbsd_in_sigtramp (CORE_ADDR pc, char *name)
{
  return (nbsd_pc_in_sigtramp (pc, name)
	  || vax_nbsd_sigtramp_start (pc) != 0);
}

/* NetBSD ELF.  */
static void
vax_nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
#if 0
  set_gdbarch_in_solib_call_trampoline (gdbarch,
                                        generic_in_solib_call_trampoline);
#endif
  set_solib_svr4_fetch_link_map_offsets (gdbarch,
                                 nbsd_ilp32_solib_svr4_fetch_link_map_offsets);
  set_gdbarch_pc_in_sigtramp (gdbarch, vax_nbsd_in_sigtramp);
  set_gdbarch_frame_chain (gdbarch, vax_nbsd_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, func_frame_chain_valid);
  set_gdbarch_frame_saved_pc (gdbarch, vax_nbsd_frame_saved_pc);
  set_gdbarch_saved_pc_after_call (gdbarch, vax_nbsd_saved_pc_after_call);
}

static void
fetch_elfcore_registers (char *core_reg_sect, unsigned core_reg_size,
                         int which, CORE_ADDR ignore)
{
  int regnum;
  switch (which)
    {
    case 0:  /* Integer registers */

      for (regnum = 0; regnum <= VAX_PS_REGNUM; regnum++)
        {
	  if (regnum * 4 >= core_reg_size)
	    break;
          supply_register (regnum, core_reg_sect + 4 * regnum);
        }
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}

static struct core_fns vaxnbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,		/* core_read_registers */
  NULL					/* next */
};

void
_initialize_vaxnbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_vax, GDB_OSABI_NETBSD_ELF,
                          vax_nbsd_init_abi);
  add_core_fns (&vaxnbsd_elfcore_fns);
}
