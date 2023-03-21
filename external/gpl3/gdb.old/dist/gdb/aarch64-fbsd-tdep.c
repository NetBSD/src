/* Target-dependent code for FreeBSD/aarch64.

   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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
#include "fbsd-tdep.h"
#include "aarch64-tdep.h"
#include "aarch64-fbsd-tdep.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "target.h"
#include "tramp-frame.h"
#include "trad-frame.h"

/* Register maps.  */

static const struct regcache_map_entry aarch64_fbsd_gregmap[] =
  {
    { 30, AARCH64_X0_REGNUM, 8 }, /* x0 ... x29 */
    { 1, AARCH64_LR_REGNUM, 8 },
    { 1, AARCH64_SP_REGNUM, 8 },
    { 1, AARCH64_PC_REGNUM, 8 },
    { 1, AARCH64_CPSR_REGNUM, 4 },
    { 0 }
  };

static const struct regcache_map_entry aarch64_fbsd_fpregmap[] =
  {
    { 32, AARCH64_V0_REGNUM, 16 }, /* v0 ... v31 */
    { 1, AARCH64_FPSR_REGNUM, 4 },
    { 1, AARCH64_FPCR_REGNUM, 4 },
    { 0 }
  };

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

#define AARCH64_SIGFRAME_UCONTEXT_OFFSET	80
#define AARCH64_UCONTEXT_MCONTEXT_OFFSET	16
#define	AARCH64_MCONTEXT_FPREGS_OFFSET		272
#define	AARCH64_MCONTEXT_FLAGS_OFFSET		800
#define AARCH64_MCONTEXT_FLAG_FP_VALID		0x1

/* Implement the "init" method of struct tramp_frame.  */

static void
aarch64_fbsd_sigframe_init (const struct tramp_frame *self,
			     struct frame_info *this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, AARCH64_SP_REGNUM);
  CORE_ADDR mcontext_addr
    = (sp
       + AARCH64_SIGFRAME_UCONTEXT_OFFSET
       + AARCH64_UCONTEXT_MCONTEXT_OFFSET);
  gdb_byte buf[4];

  trad_frame_set_reg_regmap (this_cache, aarch64_fbsd_gregmap, mcontext_addr,
			     regcache_map_entry_size (aarch64_fbsd_gregmap));

  if (target_read_memory (mcontext_addr + AARCH64_MCONTEXT_FLAGS_OFFSET, buf,
			  4) == 0
      && (extract_unsigned_integer (buf, 4, byte_order)
	  & AARCH64_MCONTEXT_FLAG_FP_VALID))
    trad_frame_set_reg_regmap (this_cache, aarch64_fbsd_fpregmap,
			       mcontext_addr + AARCH64_MCONTEXT_FPREGS_OFFSET,
			       regcache_map_entry_size (aarch64_fbsd_fpregmap));

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static const struct tramp_frame aarch64_fbsd_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    {0x910003e0, ULONGEST_MAX},		/* mov  x0, sp  */
    {0x91014000, ULONGEST_MAX},		/* add  x0, x0, #SF_UC  */
    {0xd2803428, ULONGEST_MAX},		/* mov  x8, #SYS_sigreturn  */
    {0xd4000001, ULONGEST_MAX},		/* svc  0x0  */
    {TRAMP_SENTINEL_INSN, ULONGEST_MAX}
  },
  aarch64_fbsd_sigframe_init
};

/* Register set definitions.  */

const struct regset aarch64_fbsd_gregset =
  {
    aarch64_fbsd_gregmap,
    regcache_supply_regset, regcache_collect_regset
  };

const struct regset aarch64_fbsd_fpregset =
  {
    aarch64_fbsd_fpregmap,
    regcache_supply_regset, regcache_collect_regset
  };

/* Implement the "iterate_over_regset_sections" gdbarch method.  */

static void
aarch64_fbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					   iterate_over_regset_sections_cb *cb,
					   void *cb_data,
					   const struct regcache *regcache)
{
  cb (".reg", AARCH64_FBSD_SIZEOF_GREGSET, AARCH64_FBSD_SIZEOF_GREGSET,
      &aarch64_fbsd_gregset, NULL, cb_data);
  cb (".reg2", AARCH64_FBSD_SIZEOF_FPREGSET, AARCH64_FBSD_SIZEOF_FPREGSET,
      &aarch64_fbsd_fpregset, NULL, cb_data);
}

/* Implement the 'init_osabi' method of struct gdb_osabi_handler.  */

static void
aarch64_fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Generic FreeBSD support.  */
  fbsd_init_abi (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 svr4_lp64_fetch_link_map_offsets);

  tramp_frame_prepend_unwinder (gdbarch, &aarch64_fbsd_sigframe);

  /* Enable longjmp.  */
  tdep->jb_pc = 13;

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, aarch64_fbsd_iterate_over_regset_sections);
}

void _initialize_aarch64_fbsd_tdep ();
void
_initialize_aarch64_fbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_aarch64, 0, GDB_OSABI_FREEBSD,
			  aarch64_fbsd_init_abi);
}
