/* Target-dependent code for NetBSD/arm.

   Copyright (C) 2002-2019 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "regset.h"

#include "arch/arm.h"
#include "arm-tdep.h"
#include "nbsd-tdep.h"
#include "solib-svr4.h"

/* Description of the longjmp buffer.  */
#define ARM_NBSD_JB_PC 24
#define ARM_NBSD_JB_ELEMENT_SIZE INT_REGISTER_SIZE

/* For compatibility with previous implemenations of GDB on arm/NetBSD,
   override the default little-endian breakpoint.  */
static const gdb_byte arm_nbsd_arm_le_breakpoint[] = {0x11, 0x00, 0x00, 0xe6};
static const gdb_byte arm_nbsd_arm_be_breakpoint[] = {0xe6, 0x00, 0x00, 0x11};
static const gdb_byte arm_nbsd_thumb_le_breakpoint[] = {0xfe, 0xde};
static const gdb_byte arm_nbsd_thumb_be_breakpoint[] = {0xde, 0xfe};

/* Register maps.  */

static const struct regcache_map_entry arm_nbsd_gregmap[] =
  {
    { 13, ARM_A1_REGNUM, 4 }, /* r0 ... r12 */
    { 1, ARM_SP_REGNUM, 4 },
    { 1, ARM_LR_REGNUM, 4 },
    { 1, ARM_PC_REGNUM, 4 },
    { 1, ARM_PS_REGNUM, 4 },
    { 0 }
  };

static const struct regcache_map_entry arm_nbsd_vfpregmap[] =
  {
    { 1, ARM_FPS_REGNUM, 4 },		/* fpexc */
    { 1, ARM_FPSCR_REGNUM, 4 },		/* fpscr */
    { 1, REGCACHE_MAP_SKIP, 4 },	/* fpinst */
    { 1, REGCACHE_MAP_SKIP, 4 },	/* fpinst2 */
    { 32, ARM_D0_REGNUM, 8 }, /* d0 ... d31 */	/* really 33, not 32 */
    { 1, REGCACHE_MAP_SKIP, 8 },	/* fstmx format */
    { 0 }
  };

/* Register set definitions.  */

const struct regset arm_nbsd_gregset =
  {
    arm_nbsd_gregmap,
    regcache_supply_regset, regcache_collect_regset
  };

const struct regset arm_nbsd_vfpregset =
  {
    arm_nbsd_vfpregmap,
    regcache_supply_regset, regcache_collect_regset
  };

/* Implement the "regset_from_core_section" gdbarch method.  */

#define ARM_NBSD_SIZEOF_GREGSET (17 * 4)
#define ARM_NBSD_SIZEOF_VFPREGSET (4 * 4 + 33 * 8)

static void
arm_nbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  cb (".reg", ARM_NBSD_SIZEOF_GREGSET, ARM_NBSD_SIZEOF_GREGSET,
      &arm_nbsd_gregset, NULL, cb_data);

  // XXX: Don't see it in core.
  if (tdep->vfp_register_count > 0)
    cb (".reg2", ARM_NBSD_SIZEOF_VFPREGSET, ARM_NBSD_SIZEOF_VFPREGSET,
	&arm_nbsd_vfpregset, "VFP floating-point", cb_data);
}
static void
arm_netbsd_init_abi_common (struct gdbarch_info info,
			    struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  tdep->lowest_pc = 0x8000;
  switch (info.byte_order_for_code)
    {
    case BFD_ENDIAN_LITTLE:
      tdep->arm_breakpoint = arm_nbsd_arm_le_breakpoint;
      tdep->thumb_breakpoint = arm_nbsd_thumb_le_breakpoint;
      tdep->arm_breakpoint_size = sizeof (arm_nbsd_arm_le_breakpoint);
      tdep->thumb_breakpoint_size = sizeof (arm_nbsd_thumb_le_breakpoint);
      break;

    case BFD_ENDIAN_BIG:
      tdep->arm_breakpoint = arm_nbsd_arm_be_breakpoint;
      tdep->thumb_breakpoint = arm_nbsd_thumb_be_breakpoint;
      tdep->arm_breakpoint_size = sizeof (arm_nbsd_arm_be_breakpoint);
      tdep->thumb_breakpoint_size = sizeof (arm_nbsd_thumb_be_breakpoint);
      break;

    default:
      internal_error (__FILE__, __LINE__,
		      _("arm_gdbarch_init: bad byte order for float format"));
    }

  tdep->jb_pc = ARM_NBSD_JB_PC;
  tdep->jb_elt_size = ARM_NBSD_JB_ELEMENT_SIZE;

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, arm_software_single_step);
  /* Core support */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, arm_nbsd_iterate_over_regset_sections);

}

static void
arm_netbsd_aout_init_abi (struct gdbarch_info info,
			  struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  arm_netbsd_init_abi_common (info, gdbarch);
  if (tdep->fp_model == ARM_FLOAT_AUTO)
    tdep->fp_model = ARM_FLOAT_SOFT_FPA;
}

static void
arm_netbsd_elf_init_abi (struct gdbarch_info info,
			 struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  arm_netbsd_init_abi_common (info, gdbarch);
  if (tdep->fp_model == ARM_FLOAT_AUTO)
    tdep->fp_model = ARM_FLOAT_SOFT_VFP;

  /* NetBSD ELF uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* for single stepping; see PR/50773 */
  set_gdbarch_skip_solib_resolver (gdbarch, nbsd_skip_solib_resolver);
}

void
_initialize_arm_netbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_NETBSD,
                          arm_netbsd_elf_init_abi);
}
