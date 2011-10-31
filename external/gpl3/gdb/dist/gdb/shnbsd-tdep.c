/* Target-dependent code for SuperH running NetBSD, for GDB.
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "defs.h"
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "regset.h"
#include "value.h"
#include "osabi.h"

#include "trad-frame.h"
#include "tramp-frame.h"

#include "gdb_assert.h"
#include "gdb_string.h"

#include "solib-svr4.h"

#include "sh-tdep.h"
#include "shnbsd-tdep.h"
#include "nbsd-tdep.h"

/* Convert an r0-r15 register number into an offset into a ptrace
   register structure.  */
static const int regmap[] =
{
  (20 * 4),	/* r0 */
  (19 * 4),	/* r1 */
  (18 * 4),	/* r2 */ 
  (17 * 4),	/* r3 */ 
  (16 * 4),	/* r4 */
  (15 * 4),	/* r5 */
  (14 * 4),	/* r6 */
  (13 * 4),	/* r7 */
  (12 * 4),	/* r8 */ 
  (11 * 4),	/* r9 */
  (10 * 4),	/* r10 */
  ( 9 * 4),	/* r11 */
  ( 8 * 4),	/* r12 */
  ( 7 * 4),	/* r13 */
  ( 6 * 4),	/* r14 */
  ( 5 * 4),	/* r15 */
};

/* Sizeof `struct reg' in <machine/reg.h>.  */
#define SHNBSD_SIZEOF_GREGS	(22 * 4)

/* From <machine/mcontext.h>.  */
static const int shnbsd_mc_reg_offset[] =
{
  (20 * 4),	/* r0 */
  (19 * 4),	/* r1 */
  (18 * 4),	/* r2 */ 
  (17 * 4),	/* r3 */ 
  (16 * 4),	/* r4 */
  (15 * 4),	/* r5 */
  (14 * 4),	/* r6 */
  (13 * 4),	/* r7 */
  (12 * 4),	/* r8 */ 
  (11 * 4),	/* r9 */
  (10 * 4),	/* r10 */
  ( 9 * 4),	/* r11 */
  ( 8 * 4),	/* r12 */
  ( 7 * 4),	/* r13 */
  ( 6 * 4),	/* r14 */
  (21 * 4),	/* r15/sp */
  ( 1 * 4),	/* pc */
  ( 5 * 4),	/* pr */
  ( 0 * 4),	/* gbr */
  -1,
  ( 4 * 4),	/* mach */
  ( 3 * 4),	/* macl */
  ( 2 * 4),	/* sr */
};

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
shnbsd_supply_gregset (const struct regset *regset,
		       struct regcache *regcache,
		       int regnum, const void *gregs, size_t len)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  const gdb_byte *regs = gregs;
  int i;

  gdb_assert (len >= SHNBSD_SIZEOF_GREGS);

  if (regnum == gdbarch_pc_regnum (gdbarch) || regnum == -1)
    regcache_raw_supply (regcache, gdbarch_pc_regnum (gdbarch),
			 regs + (0 * 4));

  if (regnum == SR_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, SR_REGNUM, regs + (1 * 4));

  if (regnum == PR_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, PR_REGNUM, regs + (2 * 4));

  if (regnum == MACH_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, MACH_REGNUM, regs + (3 * 4));

  if (regnum == MACL_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, MACL_REGNUM, regs + (4 * 4));

  for (i = R0_REGNUM; i <= (R0_REGNUM + 15); i++)
    {
      if (regnum == i || regnum == -1)
	regcache_raw_supply (regcache, i, regs + regmap[i - R0_REGNUM]);
    }

  if (regnum == GBR_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, GBR_REGNUM, regs + (21 * 4));
}

/* Collect register REGNUM in the general-purpose register set
   REGSET. from register cache REGCACHE into the buffer specified by
   GREGS and LEN.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
shnbsd_collect_gregset (const struct regset *regset,
			const struct regcache *regcache,
			int regnum, void *gregs, size_t len)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  gdb_byte *regs = gregs;
  int i;

  gdb_assert (len >= SHNBSD_SIZEOF_GREGS);

  if (regnum == gdbarch_pc_regnum (gdbarch) || regnum == -1)
    regcache_raw_collect (regcache, gdbarch_pc_regnum (gdbarch),
			  regs + (0 * 4));

  if (regnum == SR_REGNUM || regnum == -1)
    regcache_raw_collect (regcache, SR_REGNUM, regs + (1 * 4));

  if (regnum == PR_REGNUM || regnum == -1)
    regcache_raw_collect (regcache, PR_REGNUM, regs + (2 * 4));

  if (regnum == MACH_REGNUM || regnum == -1)
    regcache_raw_collect (regcache, MACH_REGNUM, regs + (3 * 4));

  if (regnum == MACL_REGNUM || regnum == -1)
    regcache_raw_collect (regcache, MACL_REGNUM, regs + (4 * 4));

  for (i = R0_REGNUM; i <= (R0_REGNUM + 15); i++)
    {
      if (regnum == i || regnum == -1)
	regcache_raw_collect (regcache, i, regs + regmap[i - R0_REGNUM]);
    }

  if (regnum == GBR_REGNUM || regnum == -1)
    regcache_raw_collect (regcache, GBR_REGNUM, regs + (21 * 4));
}

/* SH register sets.  */

static struct regset shnbsd_gregset =
{
  NULL,
  shnbsd_supply_gregset
};

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */

const struct regset *
shnbsd_regset_from_core_section (struct gdbarch *gdbarch,
				 const char *sect_name, size_t sect_size)
{
  if (strcmp (sect_name, ".reg") == 0 && sect_size >= SHNBSD_SIZEOF_GREGS)
    return &shnbsd_gregset;

  return NULL;
}

void
shnbsd_supply_reg (struct regcache *regcache, const char *regs, int regnum)
{
  shnbsd_supply_gregset (&shnbsd_gregset, regcache, regnum,
			 regs, SHNBSD_SIZEOF_GREGS);
}

void
shnbsd_fill_reg (const struct regcache *regcache, char *regs, int regnum)
{
  shnbsd_collect_gregset (&shnbsd_gregset, regcache, regnum,
			  regs, SHNBSD_SIZEOF_GREGS);
}

static void
shnbsd_sigtramp_cache_init (const struct tramp_frame *,
			     struct frame_info *,
			     struct trad_frame_cache *,
			     CORE_ADDR);

/* The siginfo signal trampoline for NetBSD/sh3 versions 2.0 and later */
static const struct tramp_frame shnbsd_sigtramp_si2 =
{
  SIGTRAMP_FRAME,
  2,
  {
    { 0x64f3, -1 },			/* mov     r15,r4 */
    { 0xd002, -1 },			/* mov.l   .LSYS_setcontext */
    { 0xc380, -1 },			/* trapa   #-128 */
    { 0xa003, -1 },			/* bra     .Lskip1 */
    { 0x0009, -1 },			/* nop */
    { 0x0009, -1 },			/* nop */
 /* .LSYS_setcontext */
    { 0x0134, -1 }, { 0x0000, -1 },     /* 0x134 */
 /* .Lskip1 */
    { 0x6403, -1 },			/* mov     r0,r4 */
    { 0xd002, -1 },			/* mov.l   .LSYS_exit  */
    { 0xc380, -1 },			/* trapa   #-128 */
    { 0xa003, -1 },			/* bra     .Lskip2 */
    { 0x0009, -1 },			/* nop */
    { 0x0009, -1 },			/* nop */
 /* .LSYS_exit */
    { 0x0001, -1 }, { 0x0000, -1 },     /* 0x1 */
/* .Lskip2 */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  shnbsd_sigtramp_cache_init
};

static void
shnbsd_sigtramp_cache_init (const struct tramp_frame *self,
			    struct frame_info *next_frame,
			    struct trad_frame_cache *this_cache,
			    CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (next_frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int sp_regnum = gdbarch_sp_regnum (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (next_frame, sp_regnum);
  CORE_ADDR base;
  const int *reg_offset;
  int num_regs;
  int i;

  reg_offset = shnbsd_mc_reg_offset;
  num_regs = ARRAY_SIZE (shnbsd_mc_reg_offset);
  /* SP already points at the ucontext. */
  base = sp;
  /* offsetof(ucontext_t, uc_mcontext) == 36 */
  base += 36;

  for (i = 0; i < num_regs; i++)
    if (reg_offset[i] != -1)
      trad_frame_set_reg_addr (this_cache, i, base + reg_offset[i]);

  /* Construct the frame ID using the function start.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static void
shnbsd_init_abi (struct gdbarch_info info,
                  struct gdbarch *gdbarch)
{
  set_gdbarch_regset_from_core_section
    (gdbarch, shnbsd_regset_from_core_section);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, nbsd_ilp32_solib_svr4_fetch_link_map_offsets);

  tramp_frame_prepend_unwinder (gdbarch, &shnbsd_sigtramp_si2);
}


static enum gdb_osabi
shnbsd_core_osabi_sniffer (bfd *abfd)
{
  if (strcmp (bfd_get_target (abfd), "netbsd-core") == 0)
    return GDB_OSABI_NETBSD_AOUT;

  return GDB_OSABI_UNKNOWN;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_shnbsd_tdep;

void
_initialize_shnbsd_tdep (void)
{
  /* BFD doesn't set a flavour for NetBSD style a.out core files.  */
  gdbarch_register_osabi_sniffer (bfd_arch_sh, bfd_target_unknown_flavour,
                                  shnbsd_core_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_sh, 0, GDB_OSABI_NETBSD_ELF,
			  shnbsd_init_abi);
}
