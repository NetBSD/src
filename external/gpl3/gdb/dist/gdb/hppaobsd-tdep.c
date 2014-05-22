/* Target-dependent code for OpenBSD/hppa

   Copyright (C) 2004-2013 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "regcache.h"
#include "regset.h"

#include "gdb_assert.h"
#include "gdb_string.h"

#include "hppa-tdep.h"
#include "hppabsd-tdep.h"

/* Core file support.  */

/* Sizeof `struct reg' in <machine/reg.h>.  */
#define HPPAOBSD_SIZEOF_GREGS	(34 * 4)

/* Sizeof `struct fpreg' in <machine/reg.h>.  */
#define HPPAOBSD_SIZEOF_FPREGS	(32 * 8)

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
hppaobsd_supply_gregset (const struct regset *regset,
			 struct regcache *regcache,
			 int regnum, const void *gregs, size_t len)
{
  const gdb_byte *regs = gregs;
  size_t offset;
  int i;

  gdb_assert (len >= HPPAOBSD_SIZEOF_GREGS);

  for (i = HPPA_R1_REGNUM, offset = 4; i <= HPPA_R31_REGNUM; i++, offset += 4)
    {
      if (regnum == -1 || regnum == i)
	regcache_raw_supply (regcache, i, regs + offset);
    }

  if (regnum == -1 || regnum == HPPA_SAR_REGNUM)
    regcache_raw_supply (regcache, HPPA_SAR_REGNUM, regs);
  if (regnum == -1 || regnum == HPPA_PCOQ_HEAD_REGNUM)
    regcache_raw_supply (regcache, HPPA_PCOQ_HEAD_REGNUM, regs + 32 * 4);
  if (regnum == -1 || regnum == HPPA_PCOQ_TAIL_REGNUM)
    regcache_raw_supply (regcache, HPPA_PCOQ_TAIL_REGNUM, regs + 33 * 4);
}

/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
hppaobsd_supply_fpregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *fpregs, size_t len)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  const gdb_byte *regs = fpregs;
  int i;

  gdb_assert (len >= HPPAOBSD_SIZEOF_FPREGS);

  for (i = HPPA_FP0_REGNUM; i <= HPPA_FP31R_REGNUM; i++)
    {
      if (regnum == i || regnum == -1)
	regcache_raw_supply (regcache, i, regs + (i - HPPA_FP0_REGNUM) * 4);
    }
}

/* OpenBSD/hppa register sets.  */

static struct regset hppaobsd_gregset =
{
  NULL,
  hppaobsd_supply_gregset
};

static struct regset hppaobsd_fpregset =
{
  NULL,
  hppaobsd_supply_fpregset
};

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */

static const struct regset *
hppaobsd_regset_from_core_section (struct gdbarch *gdbarch,
				  const char *sect_name, size_t sect_size)
{
  if (strcmp (sect_name, ".reg") == 0 && sect_size >= HPPAOBSD_SIZEOF_GREGS)
    return &hppaobsd_gregset;

  if (strcmp (sect_name, ".reg2") == 0 && sect_size >= HPPAOBSD_SIZEOF_FPREGS)
    return &hppaobsd_fpregset;

  return NULL;
}


static void
hppaobsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* Obviously OpenBSD is BSD-based.  */
  hppabsd_init_abi (info, gdbarch);

  /* Core file support.  */
  set_gdbarch_regset_from_core_section
    (gdbarch, hppaobsd_regset_from_core_section);
}


/* OpenBSD uses uses the traditional NetBSD core file format, even for
   ports that use ELF.  */
#define GDB_OSABI_NETBSD_CORE GDB_OSABI_OPENBSD_ELF

static enum gdb_osabi
hppaobsd_core_osabi_sniffer (bfd *abfd)
{
  if (strcmp (bfd_get_target (abfd), "netbsd-core") == 0)
    return GDB_OSABI_NETBSD_CORE;

  return GDB_OSABI_UNKNOWN;
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_hppabsd_tdep (void);

void
_initialize_hppabsd_tdep (void)
{
  /* BFD doesn't set a flavour for NetBSD style a.out core files.  */
  gdbarch_register_osabi_sniffer (bfd_arch_hppa, bfd_target_unknown_flavour,
				  hppaobsd_core_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_hppa, 0, GDB_OSABI_OPENBSD_ELF,
			  hppaobsd_init_abi);
}
