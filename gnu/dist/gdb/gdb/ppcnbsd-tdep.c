/* Target-dependent code for PowerPC systems running NetBSD.
   Copyright 2002 Free Software Foundation, Inc.
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
#include "gdbcore.h"
#include "gdbarch.h"
#include "regcache.h"
#include "target.h"
#include "breakpoint.h"
#include "value.h"

#include "ppc-tdep.h"
#include "ppcnbsd-tdep.h"
#include "nbsd-tdep.h"

#include "solib-svr4.h"

#define REG_FIXREG_OFFSET(x)	((x) * 4)
#define REG_LR_OFFSET		(32 * 4)
#define REG_CR_OFFSET		(33 * 4)
#define REG_XER_OFFSET		(34 * 4)
#define REG_CTR_OFFSET		(35 * 4)
#define REG_PC_OFFSET		(36 * 4)
#define SIZEOF_STRUCT_REG	(37 * 4)

#define FPREG_FPR_OFFSET(x)	((x) * 8)
#define FPREG_FPSCR_OFFSET	(32 * 8)
#define SIZEOF_STRUCT_FPREG	(33 * 8)

void
ppcnbsd_supply_reg (char *regs, int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  int i;

  for (i = 0; i <= 31; i++)
    {
      if (regno == i || regno == -1)
	supply_register (i, regs + REG_FIXREG_OFFSET (i));
    }

  if (regno == tdep->ppc_lr_regnum || regno == -1)
    supply_register (tdep->ppc_lr_regnum, regs + REG_LR_OFFSET);

  if (regno == tdep->ppc_cr_regnum || regno == -1)
    supply_register (tdep->ppc_cr_regnum, regs + REG_CR_OFFSET);

  if (regno == tdep->ppc_xer_regnum || regno == -1)
    supply_register (tdep->ppc_xer_regnum, regs + REG_XER_OFFSET);

  if (regno == tdep->ppc_ctr_regnum || regno == -1)
    supply_register (tdep->ppc_ctr_regnum, regs + REG_CTR_OFFSET);

  if (regno == PC_REGNUM || regno == -1)
    supply_register (PC_REGNUM, regs + REG_PC_OFFSET);
}

void
ppcnbsd_fill_reg (char *regs, int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  int i;

  for (i = 0; i <= 31; i++)
    {
      if (regno == i || regno == -1)
	regcache_collect (i, regs + REG_FIXREG_OFFSET (i));
    }

  if (regno == tdep->ppc_lr_regnum || regno == -1)
    regcache_collect (tdep->ppc_lr_regnum, regs + REG_LR_OFFSET);

  if (regno == tdep->ppc_cr_regnum || regno == -1)
    regcache_collect (tdep->ppc_cr_regnum, regs + REG_CR_OFFSET);

  if (regno == tdep->ppc_xer_regnum || regno == -1)
    regcache_collect (tdep->ppc_xer_regnum, regs + REG_XER_OFFSET);

  if (regno == tdep->ppc_ctr_regnum || regno == -1)
    regcache_collect (tdep->ppc_ctr_regnum, regs + REG_CTR_OFFSET);

  if (regno == PC_REGNUM || regno == -1)
    regcache_collect (PC_REGNUM, regs + REG_PC_OFFSET);
}

void
ppcnbsd_supply_fpreg (char *fpregs, int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  int i;

  for (i = FP0_REGNUM; i <= FP0_REGNUM + 31; i++)
    {
      if (regno == i || regno == -1)
	supply_register (i, fpregs + FPREG_FPR_OFFSET (i - FP0_REGNUM));
    }

  if (regno == tdep->ppc_fpscr_regnum || regno == -1)
    supply_register (tdep->ppc_fpscr_regnum, fpregs + FPREG_FPSCR_OFFSET);
}

void
ppcnbsd_fill_fpreg (char *fpregs, int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  int i;

  for (i = FP0_REGNUM; i <= FP0_REGNUM + 31; i++)
    {
      if (regno == i || regno == -1)
	regcache_collect (i, fpregs + FPREG_FPR_OFFSET (i - FP0_REGNUM));
    }

  if (regno == tdep->ppc_fpscr_regnum || regno == -1)
    regcache_collect (tdep->ppc_fpscr_regnum, fpregs + FPREG_FPSCR_OFFSET);
}

static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size, int which,
                      CORE_ADDR ignore)
{
  char *regs, *fpregs;

  /* We get everything from one section.  */
  if (which != 0)
    return;

  regs = core_reg_sect;
  fpregs = core_reg_sect + SIZEOF_STRUCT_REG;

  /* Integer registers.  */
  ppcnbsd_supply_reg (regs, -1);

  /* Floating point registers.  */
  ppcnbsd_supply_fpreg (fpregs, -1);
}

static void
fetch_elfcore_registers (char *core_reg_sect, unsigned core_reg_size, int which,
                         CORE_ADDR ignore)
{
  switch (which)
    {
    case 0:  /* Integer registers.  */
      if (core_reg_size != SIZEOF_STRUCT_REG)
	warning ("Wrong size register set in core file.");
      else
	ppcnbsd_supply_reg (core_reg_sect, -1);
      break;

    case 2:  /* Floating point registers.  */
      if (core_reg_size != SIZEOF_STRUCT_FPREG)
	warning ("Wrong size FP register set in core file.");
      else
	ppcnbsd_supply_fpreg (core_reg_sect, -1);
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}

static struct core_fns ppcnbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

static struct core_fns ppcnbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,		/* core_read_registers */
  NULL					/* next */
};

static const unsigned int sigtramp_sigcontext1[] = {
	0x3821fff0,	/* addi r1, r1, -16 */
	0x4e800021,	/* blrl */
	0x38610010,	/* addi r1, r1, 16 */
	0x38000127,	/* li r0, 295 */
	0x44000002,	/* sc */
	0x38000001,	/* li r0, 1 */
	0x44000002	/* sc */
};

static const unsigned int sigtramp_siginfo2[] = {
	0x7fc3f378,	/* mr r3, r30 */
	0x38000134,	/* li r0, 308 */
	0x44000002,	/* sc */
	0x38000001,	/* li r0, 1 */
	0x44000002	/* sc */
};

static int
ppcnbsd_where_in_sigtramp (CORE_ADDR pc, const int sigtramp[], size_t len)
{
  int insn, insn2;
  int i, j;

  /* Get the current opcode.  */
  if (read_memory_nobpt (pc, (char *) &insn, sizeof(insn)) != 0)
    return -1;

  for (i = 0; i < len; i += sizeof(int))
    {
      /* Does it match the current location?  If not, skip to the next one  */
      if (insn != sigtramp[i/sizeof(int)])
	continue;

      /* Now see if the entire sigtramp matches.  */
      for (j = 0; j < len; j += sizeof(int))
	{
	   if (j == i)
		continue;
	   if (read_memory_nobpt (pc-i+j, (char *) &insn2, sizeof(insn2)) != 0)
	     break;
	   if (insn2 != sigtramp[j/sizeof(int)])
	     break;
	}
	if (j == len)
	  return i;
    }
  return -1;
}

static int
ppcnbsd_pc_in_sigtramp (CORE_ADDR pc, char *func_name)
{
  int rv;

  if (pc == 0)
	return 0;

  rv = nbsd_pc_in_sigtramp (pc, func_name);
  if (rv)
    return rv;

  rv = ppcnbsd_where_in_sigtramp (pc, sigtramp_sigcontext1,
				  sizeof(sigtramp_sigcontext1));
  if (rv >= 0)
    return 1;

  rv = ppcnbsd_where_in_sigtramp (pc, sigtramp_siginfo2,
				  sizeof(sigtramp_siginfo2));
  return rv != -1;
}

#define PPCNBSD_SIGCONTEXT_PC_OFFSET (16 + 152)
#define PPCNBSD_SIGCONTEXT_LR_OFFSET (16 + 136)
#define PPCNBSD_SIGCONTEXT_FP_OFFSET (16 + 12)
#define PPCNBSD_SIGINFO_PC_OFFSET (16 + 128 + 48 + 34*4)
#define PPCNBSD_SIGINFO_LR_OFFSET (16 + 128 + 48 + 33*4)
#define PPCNBSD_SIGINFO_FP_OFFSET (16 + 128 + 48 +  1*4)

static CORE_ADDR
ppcnbsd_frame_saved_pc (struct frame_info *fi)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  int wordsize = tdep->wordsize;
  CORE_ADDR addr;

  if (!fi->signal_handler_caller
      && (fi->next == NULL || !fi->next->signal_handler_caller))
    {
      addr = rs6000_frame_saved_pc (fi);
      if (addr)
        return addr;
      return read_memory_unsigned_integer (FRAME_CHAIN (fi)
					     + tdep->lr_frame_offset,
					   wordsize);
  }

  if (ppcnbsd_where_in_sigtramp (fi->pc, sigtramp_siginfo2,
				 sizeof(sigtramp_siginfo2)) >= 0)
    {
      if (fi->next != NULL && fi->next->signal_handler_caller)
	addr = fi->next->frame + PPCNBSD_SIGINFO_LR_OFFSET;
      else
	addr = fi->frame + PPCNBSD_SIGINFO_PC_OFFSET;
    }
  else if (ppcnbsd_where_in_sigtramp (fi->pc, sigtramp_sigcontext1,
				      sizeof(sigtramp_sigcontext1)) >= 0)
    {
      if (fi->next != NULL && fi->next->signal_handler_caller)
	addr = fi->next->frame + PPCNBSD_SIGCONTEXT_LR_OFFSET;
      else
	addr = fi->frame + PPCNBSD_SIGCONTEXT_PC_OFFSET;
    }
  else
    return 0;

  return read_memory_unsigned_integer (addr, wordsize);
}

static CORE_ADDR
ppcnbsd_frame_chain (struct frame_info *fi)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  int wordsize = tdep->wordsize;
  CORE_ADDR addr;

  if (!fi->signal_handler_caller)
    {
      addr = rs6000_frame_chain (fi);
      if (addr && read_memory_unsigned_integer (addr + tdep->lr_frame_offset,
						wordsize) == 0)
	addr = 0;
      return addr;
    }

  if (ppcnbsd_where_in_sigtramp (fi->pc, sigtramp_siginfo2,
				 sizeof(sigtramp_siginfo2)) >= 0)
    addr = fi->frame + PPCNBSD_SIGINFO_FP_OFFSET;
  else if (ppcnbsd_where_in_sigtramp (fi->pc, sigtramp_sigcontext1,
				      sizeof(sigtramp_sigcontext1)) >= 0)
    addr = fi->frame + PPCNBSD_SIGCONTEXT_FP_OFFSET;
  else
    return 0;
  return read_memory_unsigned_integer (addr, wordsize);
}

static void
ppcnbsd_init_extra_frame_info (int fromleaf, struct frame_info *fi)
{
	rs6000_init_extra_frame_info (fromleaf, fi);
	fi->signal_handler_caller = 0;
}


static void
ppcnbsd_init_abi (struct gdbarch_info info,
                  struct gdbarch *gdbarch)
{
  /* Stop at main.  */
  set_gdbarch_frame_chain_valid (gdbarch, generic_func_frame_chain_valid);

  set_gdbarch_pc_in_sigtramp (gdbarch, ppcnbsd_pc_in_sigtramp);
  set_gdbarch_frame_chain (gdbarch, ppcnbsd_frame_chain);
  set_gdbarch_frame_saved_pc (gdbarch, ppcnbsd_frame_saved_pc);
  set_gdbarch_init_extra_frame_info (gdbarch, ppcnbsd_init_extra_frame_info);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
                                nbsd_ilp32_solib_svr4_fetch_link_map_offsets);
}

void
_initialize_ppcnbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_powerpc, GDB_OSABI_NETBSD_ELF,
			  ppcnbsd_init_abi);

  add_core_fns (&ppcnbsd_core_fns);
  add_core_fns (&ppcnbsd_elfcore_fns);
}
