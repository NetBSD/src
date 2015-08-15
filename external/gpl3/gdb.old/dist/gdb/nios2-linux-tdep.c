/* Target-dependent code for GNU/Linux on Nios II.
   Copyright (C) 2012-2014 Free Software Foundation, Inc.
   Contributed by Mentor Graphics, Inc.

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
#include "frame.h"
#include "gdb_assert.h"
#include <string.h>
#include "osabi.h"
#include "solib-svr4.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "symtab.h"
#include "regset.h"
#include "regcache.h"
#include "linux-tdep.h"
#include "glibc-tdep.h"
#include "nios2-tdep.h"

#include "features/nios2-linux.c"

/* Core file and register set support.  */

/* Map from the normal register enumeration order to the order that
   registers appear in core files, which corresponds to the order
   of the register slots in the kernel's struct pt_regs.  */

static const int reg_offsets[NIOS2_NUM_REGS] =
{
  -1,  8,  9, 10, 11, 12, 13, 14,	/* r0 - r7 */
  0,  1,  2,  3,  4,  5,  6,  7,	/* r8 - r15 */
  23, 24, 25, 26, 27, 28, 29, 30,	/* r16 - r23 */
  -1, -1, 19, 18, 17, 21, -1, 16,	/* et bt gp sp fp ea sstatus ra */
  21,					/* pc */
  -1, 20, -1, -1, -1, -1, -1, -1,	/* status estatus ...  */
  -1, -1, -1, -1, -1, -1, -1, -1
};

/* Implement the supply_regset hook for core files.  */

static void
nios2_supply_gregset (const struct regset *regset,
		      struct regcache *regcache,
		      int regnum, const void *gregs_buf, size_t len)
{
  const gdb_byte *gregs = gregs_buf;
  int regno;
  static const gdb_byte zero_buf[4] = {0, 0, 0, 0};

  for (regno = NIOS2_Z_REGNUM; regno <= NIOS2_MPUACC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      {
	if (reg_offsets[regno] != -1)
	  regcache_raw_supply (regcache, regno,
			       gregs + 4 * reg_offsets[regno]);
	else
	  regcache_raw_supply (regcache, regno, zero_buf);
      }
}

static struct regset nios2_core_regset =
{
  NULL,
  nios2_supply_gregset,
  NULL,
  NULL
};

/* Implement the regset_from_core_section gdbarch method.  */

static const struct regset *
nios2_regset_from_core_section (struct gdbarch *gdbarch,
                                const char *sect_name, size_t sect_size)
{
  if (strcmp (sect_name, ".reg") == 0)
    return &nios2_core_regset;

  return NULL;
}

/* Initialize a trad-frame cache corresponding to the tramp-frame.
   FUNC is the address of the instruction TRAMP[0] in memory.  */

static void
nios2_linux_rt_sigreturn_init (const struct tramp_frame *self,
			       struct frame_info *next_frame,
			       struct trad_frame_cache *this_cache,
			       CORE_ADDR func)
{
  CORE_ADDR base = func + 41 * 4;
  int i;

  for (i = 0; i < 23; i++)
    trad_frame_set_reg_addr (this_cache, i + 1, base + i * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_RA_REGNUM, base + 23 * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_FP_REGNUM, base + 24 * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_GP_REGNUM, base + 25 * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_PC_REGNUM, base + 27 * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_SP_REGNUM, base + 28 * 4);

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

static struct tramp_frame nios2_linux_rt_sigreturn_tramp_frame =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x00800004 | (139 << 6), -1 },  /* movi r2,__NR_rt_sigreturn */
    { 0x003b683a, -1 },               /* trap */
    { TRAMP_SENTINEL_INSN }
  },
  nios2_linux_rt_sigreturn_init
};

/* When FRAME is at a syscall instruction, return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
nios2_linux_syscall_next_pc (struct frame_info *frame)
{
  CORE_ADDR pc = get_frame_pc (frame);
  ULONGEST syscall_nr = get_frame_register_unsigned (frame, NIOS2_R2_REGNUM);

  /* If we are about to make a sigreturn syscall, use the unwinder to
     decode the signal frame.  */
  if (syscall_nr == 139 /* rt_sigreturn */)
    return frame_unwind_caller_pc (frame);

  return pc + NIOS2_OPCODE_SIZE;
}

/* Hook function for gdbarch_register_osabi.  */

static void
nios2_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  linux_init_abi (info, gdbarch);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 svr4_ilp32_fetch_link_map_offsets);
  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);
  /* Core file support.  */
  set_gdbarch_regset_from_core_section (gdbarch,
                                        nios2_regset_from_core_section);
  /* Linux signal frame unwinders.  */
  tramp_frame_prepend_unwinder (gdbarch,
                                &nios2_linux_rt_sigreturn_tramp_frame);

  tdep->syscall_next_pc = nios2_linux_syscall_next_pc;

  /* Index of target address word in glibc jmp_buf.  */
  tdep->jb_pc = 10;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */

extern initialize_file_ftype _initialize_nios2_linux_tdep;

void
_initialize_nios2_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_nios2, 0, GDB_OSABI_LINUX,
                          nios2_linux_init_abi);

  initialize_tdesc_nios2_linux ();
}
