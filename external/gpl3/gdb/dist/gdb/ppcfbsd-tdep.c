/* Target-dependent code for PowerPC systems running FreeBSD.

   Copyright (C) 2013-2014 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "frame.h"
#include "gdbcore.h"
#include "frame-unwind.h"
#include "gdbtypes.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "symtab.h"
#include "target.h"
#include "trad-frame.h"

#include "gdb_assert.h"
#include <string.h>

#include "ppc-tdep.h"
#include "ppc64-tdep.h"
#include "ppcfbsd-tdep.h"
#include "solib-svr4.h"


/* 32-bit regset descriptions.  */

static const struct ppc_reg_offsets ppc32_fbsd_reg_offsets =
  {
	/* General-purpose registers.  */
	/* .r0_offset = */     0,
	/* .gpr_size = */      4,
	/* .xr_size = */       4,
	/* .pc_offset = */     144,
	/* .ps_offset = */     -1,
	/* .cr_offset = */     132,
	/* .lr_offset = */     128,
	/* .ctr_offset = */    140,
	/* .xer_offset = */    136,
	/* .mq_offset = */     -1,

	/* Floating-point registers.  */
	/* .f0_offset = */     0,
	/* .fpscr_offset = */  256,
	/* .fpscr_size = */    8,
#ifdef NOTYET
	/* AltiVec registers.  */
	/* .vr0_offset = */    0,
	/* .vscr_offset = */   512 + 12,
	/* .vrsave_offset = */ 512
#endif
  };

/* 64-bit regset descriptions.  */

static const struct ppc_reg_offsets ppc64_fbsd_reg_offsets =
  {
	/* General-purpose registers.  */
	/* .r0_offset = */     0,
	/* .gpr_size = */      8,
	/* .xr_size = */       8,
	/* .pc_offset = */     288,
	/* .ps_offset = */     -1,
	/* .cr_offset = */     264,
	/* .lr_offset = */     256,
	/* .ctr_offset = */    280,
	/* .xer_offset = */    272,
	/* .mq_offset = */     -1,

	/* Floating-point registers.  */
	/* .f0_offset = */     0,
	/* .fpscr_offset = */  256,
	/* .fpscr_size = */    8,
#ifdef NOYET
	/* AltiVec registers.  */
	/* .vr0_offset = */    0,
	/* .vscr_offset = */   512 + 12,
	/* .vrsave_offset = */ 528
#endif
  };

/* 32-bit general-purpose register set.  */

static const struct regset ppc32_fbsd_gregset = {
  &ppc32_fbsd_reg_offsets,
  ppc_supply_gregset,
  ppc_collect_gregset,
  NULL
};

/* 64-bit general-purpose register set.  */

static const struct regset ppc64_fbsd_gregset = {
  &ppc64_fbsd_reg_offsets,
  ppc_supply_gregset,
  ppc_collect_gregset,
  NULL
};

/* 32-/64-bit floating-point register set.  */

static struct regset ppc32_fbsd_fpregset = {
  &ppc32_fbsd_reg_offsets,
  ppc_supply_fpregset,
  ppc_collect_fpregset
};

const struct regset *
ppc_fbsd_gregset (int wordsize)
{
  return wordsize == 8 ? &ppc64_fbsd_gregset : &ppc32_fbsd_gregset;
}

const struct regset *
ppc_fbsd_fpregset (void)
{
  return &ppc32_fbsd_fpregset;
}

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */

static const struct regset *
ppcfbsd_regset_from_core_section (struct gdbarch *gdbarch,
				  const char *sect_name, size_t sect_size)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  if (strcmp (sect_name, ".reg") == 0 && sect_size >= 148)
    {
      if (tdep->wordsize == 4)
	return &ppc32_fbsd_gregset;
      else
	return &ppc64_fbsd_gregset;
    }
  if (strcmp (sect_name, ".reg2") == 0 && sect_size >= 264)
    return &ppc32_fbsd_fpregset;
  return NULL;
}

/* Default page size.  */

static const int ppcfbsd_page_size = 4096;

/* Offset for sigreturn(2).  */

static const int ppcfbsd_sigreturn_offset[] = {
  0xc,				/* FreeBSD 32-bit  */
  -1
};

/* Signal trampolines.  */

static int
ppcfbsd_sigtramp_frame_sniffer (const struct frame_unwind *self,
				struct frame_info *this_frame,
				void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc = get_frame_pc (this_frame);
  CORE_ADDR start_pc = (pc & ~(ppcfbsd_page_size - 1));
  const int *offset;
  const char *name;

  /* A stack trampoline is detected if no name is associated
   to the current pc and if it points inside a trampoline
   sequence.  */

  find_pc_partial_function (pc, &name, NULL, NULL);

  /* If we have a name, we have no trampoline, return.  */
  if (name)
    return 0;

  for (offset = ppcfbsd_sigreturn_offset; *offset != -1; offset++)
    {
      gdb_byte buf[2 * PPC_INSN_SIZE];
      unsigned long insn;

      if (!safe_frame_unwind_memory (this_frame, start_pc + *offset,
				     buf, sizeof buf))
	continue;

      /* Check for "li r0,SYS_sigreturn".  */
      insn = extract_unsigned_integer (buf, PPC_INSN_SIZE, byte_order);
      if (insn != 0x380001a1)
	continue;

      /* Check for "sc".  */
      insn = extract_unsigned_integer (buf + PPC_INSN_SIZE,
				       PPC_INSN_SIZE, byte_order);
      if (insn != 0x44000002)
	continue;

      return 1;
    }

  return 0;
}

static struct trad_frame_cache *
ppcfbsd_sigtramp_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct trad_frame_cache *cache;
  CORE_ADDR addr, base, func;
  gdb_byte buf[PPC_INSN_SIZE];
  int i;

  if (*this_cache)
    return *this_cache;

  cache = trad_frame_cache_zalloc (this_frame);
  *this_cache = cache;

  func = get_frame_pc (this_frame);
  func &= ~(ppcfbsd_page_size - 1);
  if (!safe_frame_unwind_memory (this_frame, func, buf, sizeof buf))
    return cache;

  base = get_frame_register_unsigned (this_frame, gdbarch_sp_regnum (gdbarch));
  addr = base + 0x10 + 2 * tdep->wordsize;
  for (i = 0; i < ppc_num_gprs; i++, addr += tdep->wordsize)
    {
      int regnum = i + tdep->ppc_gp0_regnum;
      trad_frame_set_reg_addr (cache, regnum, addr);
    }
  trad_frame_set_reg_addr (cache, tdep->ppc_lr_regnum, addr);
  addr += tdep->wordsize;
  trad_frame_set_reg_addr (cache, tdep->ppc_cr_regnum, addr);
  addr += tdep->wordsize;
  trad_frame_set_reg_addr (cache, tdep->ppc_xer_regnum, addr);
  addr += tdep->wordsize;
  trad_frame_set_reg_addr (cache, tdep->ppc_ctr_regnum, addr);
  addr += tdep->wordsize;
  trad_frame_set_reg_addr (cache, gdbarch_pc_regnum (gdbarch), addr);
  /* SRR0?  */
  addr += tdep->wordsize;

  /* Construct the frame ID using the function start.  */
  trad_frame_set_id (cache, frame_id_build (base, func));

  return cache;
}

static void
ppcfbsd_sigtramp_frame_this_id (struct frame_info *this_frame,
				void **this_cache, struct frame_id *this_id)
{
  struct trad_frame_cache *cache =
    ppcfbsd_sigtramp_frame_cache (this_frame, this_cache);

  trad_frame_get_id (cache, this_id);
}

static struct value *
ppcfbsd_sigtramp_frame_prev_register (struct frame_info *this_frame,
				      void **this_cache, int regnum)
{
  struct trad_frame_cache *cache =
    ppcfbsd_sigtramp_frame_cache (this_frame, this_cache);

  return trad_frame_get_register (cache, this_frame, regnum);
}

static const struct frame_unwind ppcfbsd_sigtramp_frame_unwind = {
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  ppcfbsd_sigtramp_frame_this_id,
  ppcfbsd_sigtramp_frame_prev_register,
  NULL,
  ppcfbsd_sigtramp_frame_sniffer
};

static enum return_value_convention
ppcfbsd_return_value (struct gdbarch *gdbarch, struct value *function,
		      struct type *valtype, struct regcache *regcache,
		      gdb_byte *readbuf, const gdb_byte *writebuf)
{
  return ppc_sysv_abi_broken_return_value (gdbarch, function, valtype,
					   regcache, readbuf, writebuf);
}


static void
ppcfbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* FreeBSD doesn't support the 128-bit `long double' from the psABI.  */
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);

  if (tdep->wordsize == 4)
    {
      set_gdbarch_return_value (gdbarch, ppcfbsd_return_value);

      set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
      set_solib_svr4_fetch_link_map_offsets (gdbarch,
					     svr4_ilp32_fetch_link_map_offsets);

      frame_unwind_append_unwinder (gdbarch, &ppcfbsd_sigtramp_frame_unwind);
      set_gdbarch_gcore_bfd_target (gdbarch, "elf32-powerpc");
    }

  if (tdep->wordsize == 8)
    {
      set_gdbarch_convert_from_func_ptr_addr
	(gdbarch, ppc64_convert_from_func_ptr_addr);
      set_gdbarch_elf_make_msymbol_special (gdbarch,
					    ppc64_elf_make_msymbol_special);

      set_gdbarch_skip_trampoline_code (gdbarch, ppc64_skip_trampoline_code);
      set_solib_svr4_fetch_link_map_offsets (gdbarch,
					     svr4_lp64_fetch_link_map_offsets);
      set_gdbarch_gcore_bfd_target (gdbarch, "elf64-powerpc");
    }

  set_gdbarch_regset_from_core_section
    (gdbarch, ppcfbsd_regset_from_core_section);

  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */

void _initialize_ppcfbsd_tdep (void);

void
_initialize_ppcfbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_powerpc, bfd_mach_ppc, GDB_OSABI_FREEBSD_ELF,
			  ppcfbsd_init_abi);
  gdbarch_register_osabi (bfd_arch_powerpc, bfd_mach_ppc64,
			  GDB_OSABI_FREEBSD_ELF,
			  ppcfbsd_init_abi);
  gdbarch_register_osabi (bfd_arch_rs6000, 0, GDB_OSABI_FREEBSD_ELF,
			  ppcfbsd_init_abi);
}
