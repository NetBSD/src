/* Target-dependent code for NetBSD on RISC-V processors.
   Copyright (C) 2018-2020 Free Software Foundation, Inc.

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
#include "nbsd-tdep.h"
#include "osabi.h"
#include "riscv-tdep.h"
#include "riscv-nbsd-tdep.h"
#include "solib-svr4.h"
#include "target.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "gdbarch.h"
#include "inferior.h"

/* Register maps.  */

static const struct regcache_map_entry riscv_nbsd_gregmap[] =
  {
    { 31, RISCV_RA_REGNUM, 0 }, /* x1 to x31 */
    { 1,  RISCV_PC_REGNUM, 0 },
    { 0 }
  };

static const struct regcache_map_entry riscv_nbsd_fpregmap[] =
  {
    { 32, RISCV_FIRST_FP_REGNUM, 16 },
    { 1, RISCV_CSR_FCSR_REGNUM, 8 },
    { 0 }
  };

/* Supply the general-purpose registers stored in GREGS to REGCACHE.
   This function only exists to supply the always-zero x0 in addition
   to the registers in GREGS.  */

static void
riscv_nbsd_supply_gregset (const struct regset *regset,
			   struct regcache *regcache, int regnum,
			   const void *gregs, size_t len)
{
  regcache->supply_regset (&riscv_nbsd_gregset, regnum, gregs, len);
  if (regnum == -1 || regnum == RISCV_ZERO_REGNUM)
    regcache->raw_supply_zeroed (RISCV_ZERO_REGNUM);
}

/* Register set definitions.  */

const struct regset riscv_nbsd_gregset =
  {
    riscv_nbsd_gregmap,
    riscv_nbsd_supply_gregset, regcache_collect_regset
  };

const struct regset riscv_nbsd_fpregset =
  {
    riscv_nbsd_fpregmap,
    regcache_supply_regset, regcache_collect_regset
  };

/* Implement the "iterate_over_regset_sections" gdbarch method.  */

static void
riscv_nbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  cb (".reg", RISCV_NBSD_NUM_GREGS * riscv_isa_xlen (gdbarch),
      RISCV_NBSD_NUM_GREGS * riscv_isa_xlen (gdbarch),
      &riscv_nbsd_gregset, NULL, cb_data);
  cb (".reg2", RISCV_NBSD_SIZEOF_FPREGSET, RISCV_NBSD_SIZEOF_FPREGSET,
      &riscv_nbsd_fpregset, NULL, cb_data);
}

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

#define RISCV_SIGFRAME_UCONTEXT_OFFSET 		80
#define RISCV_UCONTEXT_MCONTEXT_OFFSET		16
#define RISCV_MCONTEXT_FLAG_FP_VALID		0x1

/* Implement the "init" method of struct tramp_frame.  */

static void
riscv_nbsd_sigframe_init (const struct tramp_frame *self,
			  struct frame_info *this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, RISCV_SP_REGNUM);
  CORE_ADDR mcontext_addr
    = (sp
       + RISCV_SIGFRAME_UCONTEXT_OFFSET
       + RISCV_UCONTEXT_MCONTEXT_OFFSET);
  gdb_byte buf[4];

  trad_frame_set_reg_regmap (this_cache, riscv_nbsd_gregmap, mcontext_addr,
			     RISCV_NBSD_NUM_GREGS * riscv_isa_xlen (gdbarch));

  CORE_ADDR fpregs_addr
    = mcontext_addr + RISCV_NBSD_NUM_GREGS * riscv_isa_xlen (gdbarch);
  CORE_ADDR fp_flags_addr
    = fpregs_addr + RISCV_NBSD_SIZEOF_FPREGSET;
  if (target_read_memory (fp_flags_addr, buf, 4) == 0
      && (extract_unsigned_integer (buf, 4, byte_order)
	  & RISCV_MCONTEXT_FLAG_FP_VALID))
    trad_frame_set_reg_regmap (this_cache, riscv_nbsd_fpregmap, fpregs_addr,
			       RISCV_NBSD_SIZEOF_FPREGSET);

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

/* RISC-V supports 16-bit instructions ("C") as well as 32-bit
   instructions.  The signal trampoline on NetBSD uses a mix of
   these, but tramp_frame assumes a fixed instruction size.  To cope,
   claim that all instructions are 16 bits and use two "slots" for
   32-bit instructions.  */

static const struct tramp_frame riscv_nbsd_sigframe =
{
  SIGTRAMP_FRAME,
  2,
  {
    {0x850a, ULONGEST_MAX},		/* mov  a0, sp  */
    {0x0513, ULONGEST_MAX},		/* addi a0, a0, #SF_UC  */
    {0x0505, ULONGEST_MAX},
    {0x0293, ULONGEST_MAX},		/* li   t0, #SYS_sigreturn  */
    {0x1a10, ULONGEST_MAX},
    {0x0073, ULONGEST_MAX},		/* ecall  */
    {0x0000, ULONGEST_MAX},
    {TRAMP_SENTINEL_INSN, ULONGEST_MAX}
  },
  riscv_nbsd_sigframe_init
};


#if 0
/* Implement the "get_thread_local_address" gdbarch method.  */

static CORE_ADDR
riscv_nbsd_get_thread_local_address (struct gdbarch *gdbarch, ptid_t ptid,
				     CORE_ADDR lm_addr, CORE_ADDR offset)
{
  struct regcache *regcache;

  regcache = get_thread_arch_regcache (current_inferior ()->process_target (),
				       ptid, gdbarch);

  target_fetch_registers (regcache, RISCV_TP_REGNUM);

  ULONGEST tp;
  if (regcache->cooked_read (RISCV_TP_REGNUM, &tp) != REG_VALID)
    error (_("Unable to fetch %%tp"));

  /* %tp points to the end of the TCB which contains two pointers.
      The first pointer in the TCB points to the DTV array.  */
  CORE_ADDR dtv_addr = tp - (gdbarch_ptr_bit (gdbarch) / 8) * 2;
  return nbsd_get_thread_local_address (gdbarch, dtv_addr, lm_addr, offset);
}
#endif


/* Implement the 'init_osabi' method of struct gdb_osabi_handler.  */

static void
riscv_nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* Generic NetBSD support.  */
  nbsd_init_abi (info, gdbarch);

  set_gdbarch_software_single_step (gdbarch, riscv_software_single_step);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 (riscv_isa_xlen (gdbarch) == 4
					  ? svr4_ilp32_fetch_link_map_offsets
					  : svr4_lp64_fetch_link_map_offsets));

  tramp_frame_prepend_unwinder (gdbarch, &riscv_nbsd_sigframe);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, riscv_nbsd_iterate_over_regset_sections);

  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
#if 0
  set_gdbarch_get_thread_local_address (gdbarch,
					riscv_nbsd_get_thread_local_address);
#endif
}

void _initialize_riscv_nbsd_tdep ();
void
_initialize_riscv_nbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_riscv, 0, GDB_OSABI_FREEBSD,
			  riscv_nbsd_init_abi);
}
