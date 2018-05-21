/* Target-dependent code for FreeBSD/aarch64.

   Copyright (C) 2017-2018 Free Software Foundation, Inc.

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

#include "gdbarch.h"
#include "nbsd-tdep.h"
#include "aarch64-tdep.h"
#include "aarch64-nbsd-tdep.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "target.h"
#include "tramp-frame.h"
#include "trad-frame.h"

/* In a signal frame, sp points to a 'struct sigframe' which is
   defined as:

   struct sigframe {
	   siginfo_t	sf_si;
	   ucontext_t	sf_uc;
   };

   ucontext_t is defined as:

   struct __ucontext {
	   sigset_t	uc_sigmask;
	   mcontext_t	uc_mcontext;
	   ...
   };

   The mcontext_t contains the general purpose register set followed
   by the floating point register set.  The floating point register
   set is only valid if the _MC_FP_VALID flag is set in mc_flags.  */

#define AARCH64_MCONTEXT_REG_SIZE               8
#define AARCH64_MCONTEXT_FPREG_SIZE             16
#define AARCH64_SIGFRAME_UCONTEXT_OFFSET	80
#define AARCH64_UCONTEXT_MCONTEXT_OFFSET	16
#define	AARCH64_MCONTEXT_FPREGS_OFFSET		272
#define	AARCH64_MCONTEXT_FLAGS_OFFSET		800
#define AARCH64_MCONTEXT_FLAG_FP_VALID		0x1

/* Implement the "init" method of struct tramp_frame.  */

static void
aarch64_nbsd_sigframe_init (const struct tramp_frame *self,
			     struct frame_info *this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, AARCH64_SP_REGNUM);
  CORE_ADDR mcontext_addr =
    sp
    + AARCH64_SIGFRAME_UCONTEXT_OFFSET
    + AARCH64_UCONTEXT_MCONTEXT_OFFSET;
  gdb_byte buf[4];
  int i;

  for (i = 0; i < 30; i++)
    {
      trad_frame_set_reg_addr (this_cache,
			       AARCH64_X0_REGNUM + i,
			       mcontext_addr + i * AARCH64_MCONTEXT_REG_SIZE);
    }
  trad_frame_set_reg_addr (this_cache, AARCH64_LR_REGNUM,
			   mcontext_addr + 30 * AARCH64_MCONTEXT_REG_SIZE);
  trad_frame_set_reg_addr (this_cache, AARCH64_SP_REGNUM,
			   mcontext_addr + 31 * AARCH64_MCONTEXT_REG_SIZE);
  trad_frame_set_reg_addr (this_cache, AARCH64_PC_REGNUM,
			   mcontext_addr + 32 * AARCH64_MCONTEXT_REG_SIZE);
  trad_frame_set_reg_addr (this_cache, AARCH64_CPSR_REGNUM,
			   mcontext_addr + 33 * AARCH64_MCONTEXT_REG_SIZE);

  if (target_read_memory (mcontext_addr + AARCH64_MCONTEXT_FLAGS_OFFSET, buf,
			  4) == 0
      && (extract_unsigned_integer (buf, 4, byte_order)
	  & AARCH64_MCONTEXT_FLAG_FP_VALID))
    {
      for (i = 0; i < 32; i++)
	{
	  trad_frame_set_reg_addr (this_cache, AARCH64_V0_REGNUM + i,
				   mcontext_addr
				   + AARCH64_MCONTEXT_FPREGS_OFFSET
				   + i * AARCH64_MCONTEXT_FPREG_SIZE);
	}
      trad_frame_set_reg_addr (this_cache, AARCH64_FPSR_REGNUM,
			       mcontext_addr + AARCH64_MCONTEXT_FPREGS_OFFSET
			       + 32 * AARCH64_MCONTEXT_FPREG_SIZE);
      trad_frame_set_reg_addr (this_cache, AARCH64_FPCR_REGNUM,
			       mcontext_addr + AARCH64_MCONTEXT_FPREGS_OFFSET
			       + 32 * AARCH64_MCONTEXT_FPREG_SIZE + 4);
    }

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static const struct tramp_frame aarch64_nbsd_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    {0x910003e0, -1},		/* mov  x0, sp  */
    {0x91014000, -1},		/* add  x0, x0, #SF_UC  */
    {0xd2803428, -1},		/* mov  x8, #SYS_sigreturn  */
    {0xd4000001, -1},		/* svc  0x0  */
    {TRAMP_SENTINEL_INSN, -1}
  },
  aarch64_nbsd_sigframe_init
};

/* Register maps.  */

static const struct regcache_map_entry aarch64_nbsd_gregmap[] =
  {
    { 30, AARCH64_X0_REGNUM, 8 }, /* x0 ... x29 */
    { 1, AARCH64_LR_REGNUM, 8 },
    { 1, AARCH64_SP_REGNUM, 8 },
    { 1, AARCH64_PC_REGNUM, 8 },
    { 1, AARCH64_CPSR_REGNUM, 4 },
    { 0 }
  };

static const struct regcache_map_entry aarch64_nbsd_fpregmap[] =
  {
    { 32, AARCH64_V0_REGNUM, 16 }, /* v0 ... v31 */
    { 1, AARCH64_FPSR_REGNUM, 4 },
    { 1, AARCH64_FPCR_REGNUM, 4 },
    { 0 }
  };

/* Register set definitions.  */

const struct regset aarch64_nbsd_gregset =
  {
    aarch64_nbsd_gregmap,
    regcache_supply_regset, regcache_collect_regset
  };

const struct regset aarch64_nbsd_fpregset =
  {
    aarch64_nbsd_fpregmap,
    regcache_supply_regset, regcache_collect_regset
  };

/* Implement the "regset_from_core_section" gdbarch method.  */

static void
aarch64_nbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					   iterate_over_regset_sections_cb *cb,
					   void *cb_data,
					   const struct regcache *regcache)
{
  cb (".reg", AARCH64_NBSD_SIZEOF_GREGSET, &aarch64_nbsd_gregset,
      NULL, cb_data);
  cb (".reg2", AARCH64_NBSD_SIZEOF_FPREGSET, &aarch64_nbsd_fpregset,
      NULL, cb_data);
}

/* Implement the 'init_osabi' method of struct gdb_osabi_handler.  */

static void
aarch64_nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 svr4_lp64_fetch_link_map_offsets);

  tramp_frame_prepend_unwinder (gdbarch, &aarch64_nbsd_sigframe);

  /* Enable longjmp.  */
  tdep->jb_pc = 13;

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, aarch64_nbsd_iterate_over_regset_sections);
}

void
_initialize_aarch64_nbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_aarch64, 0, GDB_OSABI_FREEBSD,
			  aarch64_nbsd_init_abi);
}
