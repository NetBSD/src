/* Target-dependent code for FreeBSD/arm.

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

#include "elf/common.h"
#include "target-descriptions.h"
#include "aarch32-tdep.h"
#include "arm-tdep.h"
#include "arm-fbsd-tdep.h"
#include "auxv.h"
#include "fbsd-tdep.h"
#include "gdbcore.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "trad-frame.h"
#include "tramp-frame.h"

/* Register maps.  */

static const struct regcache_map_entry arm_fbsd_gregmap[] =
  {
    { 13, ARM_A1_REGNUM, 4 }, /* r0 ... r12 */
    { 1, ARM_SP_REGNUM, 4 },
    { 1, ARM_LR_REGNUM, 4 },
    { 1, ARM_PC_REGNUM, 4 },
    { 1, ARM_PS_REGNUM, 4 },
    { 0 }
  };

static const struct regcache_map_entry arm_fbsd_vfpregmap[] =
  {
    { 32, ARM_D0_REGNUM, 8 }, /* d0 ... d31 */
    { 1, ARM_FPSCR_REGNUM, 4 },
    { 0 }
  };

/* In a signal frame, sp points to a 'struct sigframe' which is
   defined as:

   struct sigframe {
	   siginfo_t	sf_si;
	   ucontext_t	sf_uc;
	   mcontext_vfp_t sf_vfp;
   };

   ucontext_t is defined as:

   struct __ucontext {
	   sigset_t	uc_sigmask;
	   mcontext_t	uc_mcontext;
	   ...
   };

   mcontext_t is defined as:

   struct {
	   unsigned int __gregs[17];
	   size_t       mc_vfp_size;
	   void         *mc_vfp_ptr;
	   ...
   };

   mcontext_vfp_t is defined as:

   struct {
	  uint64_t      mcv_reg[32];
	  uint32_t      mcv_fpscr;
   };

   If the VFP state is valid, then mc_vfp_ptr will point to sf_vfp in
   the sigframe, otherwise it is NULL.  There is no non-VFP floating
   point register state saved in the signal frame.  */

#define ARM_SIGFRAME_UCONTEXT_OFFSET	64
#define ARM_UCONTEXT_MCONTEXT_OFFSET	16
#define ARM_MCONTEXT_VFP_PTR_OFFSET	72

/* Implement the "init" method of struct tramp_frame.  */

static void
arm_fbsd_sigframe_init (const struct tramp_frame *self,
			struct frame_info *this_frame,
			struct trad_frame_cache *this_cache,
			CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  CORE_ADDR mcontext_addr = (sp
			     + ARM_SIGFRAME_UCONTEXT_OFFSET
			     + ARM_UCONTEXT_MCONTEXT_OFFSET);
  ULONGEST mcontext_vfp_addr;

  trad_frame_set_reg_regmap (this_cache, arm_fbsd_gregmap, mcontext_addr,
			     regcache_map_entry_size (arm_fbsd_gregmap));

  if (safe_read_memory_unsigned_integer (mcontext_addr
					 + ARM_MCONTEXT_VFP_PTR_OFFSET, 4,
					 byte_order,
					 &mcontext_vfp_addr)
      && mcontext_vfp_addr != 0)
    trad_frame_set_reg_regmap (this_cache, arm_fbsd_vfpregmap, mcontext_vfp_addr,
			       regcache_map_entry_size (arm_fbsd_vfpregmap));

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static const struct tramp_frame arm_fbsd_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    {0xe1a0000d, ULONGEST_MAX},		/* mov  r0, sp  */
    {0xe2800040, ULONGEST_MAX},		/* add  r0, r0, #SIGF_UC  */
    {0xe59f700c, ULONGEST_MAX},		/* ldr  r7, [pc, #12]  */
    {0xef0001a1, ULONGEST_MAX},		/* swi  SYS_sigreturn  */
    {TRAMP_SENTINEL_INSN, ULONGEST_MAX}
  },
  arm_fbsd_sigframe_init
};

/* Register set definitions.  */

const struct regset arm_fbsd_gregset =
  {
    arm_fbsd_gregmap,
    regcache_supply_regset, regcache_collect_regset
  };

const struct regset arm_fbsd_vfpregset =
  {
    arm_fbsd_vfpregmap,
    regcache_supply_regset, regcache_collect_regset
  };

/* Implement the "iterate_over_regset_sections" gdbarch method.  */

static void
arm_fbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  cb (".reg", ARM_FBSD_SIZEOF_GREGSET, ARM_FBSD_SIZEOF_GREGSET,
      &arm_fbsd_gregset, NULL, cb_data);

  /* While FreeBSD/arm cores do contain a NT_FPREGSET / ".reg2"
     register set, it is not populated with register values by the
     kernel but just contains all zeroes.  */
  if (tdep->vfp_register_count > 0)
    cb (".reg-arm-vfp", ARM_FBSD_SIZEOF_VFPREGSET, ARM_FBSD_SIZEOF_VFPREGSET,
	&arm_fbsd_vfpregset, "VFP floating-point", cb_data);
}

/* Lookup a target description from a target's AT_HWCAP auxiliary
   vector.  */

const struct target_desc *
arm_fbsd_read_description_auxv (struct target_ops *target)
{
  CORE_ADDR arm_hwcap = 0;

  if (target_auxv_search (target, AT_FREEBSD_HWCAP, &arm_hwcap) != 1)
    return nullptr;

  if (arm_hwcap & HWCAP_VFP)
    {
      if (arm_hwcap & HWCAP_NEON)
	return aarch32_read_description ();
      else if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPD32))
	  == (HWCAP_VFPv3 | HWCAP_VFPD32))
	return arm_read_description (ARM_FP_TYPE_VFPV3);
      else
      return arm_read_description (ARM_FP_TYPE_VFPV2);
    }

  return nullptr;
}

/* Implement the "core_read_description" gdbarch method.  */

static const struct target_desc *
arm_fbsd_core_read_description (struct gdbarch *gdbarch,
				struct target_ops *target,
				bfd *abfd)
{
  return arm_fbsd_read_description_auxv (target);
}

/* Implement the 'init_osabi' method of struct gdb_osabi_handler.  */

static void
arm_fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Generic FreeBSD support.  */
  fbsd_init_abi (info, gdbarch);

  if (tdep->fp_model == ARM_FLOAT_AUTO)
    tdep->fp_model = ARM_FLOAT_SOFT_VFP;

  tramp_frame_prepend_unwinder (gdbarch, &arm_fbsd_sigframe);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  tdep->jb_pc = 24;
  tdep->jb_elt_size = 4;

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, arm_fbsd_iterate_over_regset_sections);
  set_gdbarch_core_read_description (gdbarch, arm_fbsd_core_read_description);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, arm_software_single_step);
}

void _initialize_arm_fbsd_tdep ();
void
_initialize_arm_fbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_FREEBSD,
			  arm_fbsd_init_abi);
}
