/* GNU/Linux/PowerPC specific low level interface, for the remote server for
   GDB.
   Copyright (C) 1995-2013 Free Software Foundation, Inc.

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

#include "server.h"
#include "linux-low.h"

#include <elf.h>
#include <asm/ptrace.h>

/* These are in <asm/cputable.h> in current kernels.  */
#define PPC_FEATURE_HAS_VSX		0x00000080
#define PPC_FEATURE_HAS_ALTIVEC         0x10000000
#define PPC_FEATURE_HAS_SPE             0x00800000
#define PPC_FEATURE_CELL                0x00010000
#define PPC_FEATURE_HAS_DFP             0x00000400

static unsigned long ppc_hwcap;


/* Defined in auto-generated file powerpc-32l.c.  */
void init_registers_powerpc_32l (void);
/* Defined in auto-generated file powerpc-altivec32l.c.  */
void init_registers_powerpc_altivec32l (void);
/* Defined in auto-generated file powerpc-cell32l.c.  */
void init_registers_powerpc_cell32l (void);
/* Defined in auto-generated file powerpc-vsx32l.c.  */
void init_registers_powerpc_vsx32l (void);
/* Defined in auto-generated file powerpc-isa205-32l.c.  */
void init_registers_powerpc_isa205_32l (void);
/* Defined in auto-generated file powerpc-isa205-altivec32l.c.  */
void init_registers_powerpc_isa205_altivec32l (void);
/* Defined in auto-generated file powerpc-isa205-vsx32l.c.  */
void init_registers_powerpc_isa205_vsx32l (void);
/* Defined in auto-generated file powerpc-e500l.c.  */
void init_registers_powerpc_e500l (void);
/* Defined in auto-generated file powerpc-64l.c.  */
void init_registers_powerpc_64l (void);
/* Defined in auto-generated file powerpc-altivec64l.c.  */
void init_registers_powerpc_altivec64l (void);
/* Defined in auto-generated file powerpc-cell64l.c.  */
void init_registers_powerpc_cell64l (void);
/* Defined in auto-generated file powerpc-vsx64l.c.  */
void init_registers_powerpc_vsx64l (void);
/* Defined in auto-generated file powerpc-isa205-64l.c.  */
void init_registers_powerpc_isa205_64l (void);
/* Defined in auto-generated file powerpc-isa205-altivec64l.c.  */
void init_registers_powerpc_isa205_altivec64l (void);
/* Defined in auto-generated file powerpc-isa205-vsx64l.c.  */
void init_registers_powerpc_isa205_vsx64l (void);

#define ppc_num_regs 73

/* This sometimes isn't defined.  */
#ifndef PT_ORIG_R3
#define PT_ORIG_R3 34
#endif
#ifndef PT_TRAP
#define PT_TRAP 40
#endif

#ifdef __powerpc64__
/* We use a constant for FPSCR instead of PT_FPSCR, because
   many shipped PPC64 kernels had the wrong value in ptrace.h.  */
static int ppc_regmap[] =
 {PT_R0 * 8,     PT_R1 * 8,     PT_R2 * 8,     PT_R3 * 8,
  PT_R4 * 8,     PT_R5 * 8,     PT_R6 * 8,     PT_R7 * 8,
  PT_R8 * 8,     PT_R9 * 8,     PT_R10 * 8,    PT_R11 * 8,
  PT_R12 * 8,    PT_R13 * 8,    PT_R14 * 8,    PT_R15 * 8,
  PT_R16 * 8,    PT_R17 * 8,    PT_R18 * 8,    PT_R19 * 8,
  PT_R20 * 8,    PT_R21 * 8,    PT_R22 * 8,    PT_R23 * 8,
  PT_R24 * 8,    PT_R25 * 8,    PT_R26 * 8,    PT_R27 * 8,
  PT_R28 * 8,    PT_R29 * 8,    PT_R30 * 8,    PT_R31 * 8,
  PT_FPR0*8,     PT_FPR0*8 + 8, PT_FPR0*8+16,  PT_FPR0*8+24,
  PT_FPR0*8+32,  PT_FPR0*8+40,  PT_FPR0*8+48,  PT_FPR0*8+56,
  PT_FPR0*8+64,  PT_FPR0*8+72,  PT_FPR0*8+80,  PT_FPR0*8+88,
  PT_FPR0*8+96,  PT_FPR0*8+104,  PT_FPR0*8+112,  PT_FPR0*8+120,
  PT_FPR0*8+128, PT_FPR0*8+136,  PT_FPR0*8+144,  PT_FPR0*8+152,
  PT_FPR0*8+160,  PT_FPR0*8+168,  PT_FPR0*8+176,  PT_FPR0*8+184,
  PT_FPR0*8+192,  PT_FPR0*8+200,  PT_FPR0*8+208,  PT_FPR0*8+216,
  PT_FPR0*8+224,  PT_FPR0*8+232,  PT_FPR0*8+240,  PT_FPR0*8+248,
  PT_NIP * 8,    PT_MSR * 8,    PT_CCR * 8,    PT_LNK * 8,
  PT_CTR * 8,    PT_XER * 8,    PT_FPR0*8 + 256,
  PT_ORIG_R3 * 8, PT_TRAP * 8 };
#else
/* Currently, don't check/send MQ.  */
static int ppc_regmap[] =
 {PT_R0 * 4,     PT_R1 * 4,     PT_R2 * 4,     PT_R3 * 4,
  PT_R4 * 4,     PT_R5 * 4,     PT_R6 * 4,     PT_R7 * 4,
  PT_R8 * 4,     PT_R9 * 4,     PT_R10 * 4,    PT_R11 * 4,
  PT_R12 * 4,    PT_R13 * 4,    PT_R14 * 4,    PT_R15 * 4,
  PT_R16 * 4,    PT_R17 * 4,    PT_R18 * 4,    PT_R19 * 4,
  PT_R20 * 4,    PT_R21 * 4,    PT_R22 * 4,    PT_R23 * 4,
  PT_R24 * 4,    PT_R25 * 4,    PT_R26 * 4,    PT_R27 * 4,
  PT_R28 * 4,    PT_R29 * 4,    PT_R30 * 4,    PT_R31 * 4,
  PT_FPR0*4,     PT_FPR0*4 + 8, PT_FPR0*4+16,  PT_FPR0*4+24,
  PT_FPR0*4+32,  PT_FPR0*4+40,  PT_FPR0*4+48,  PT_FPR0*4+56,
  PT_FPR0*4+64,  PT_FPR0*4+72,  PT_FPR0*4+80,  PT_FPR0*4+88,
  PT_FPR0*4+96,  PT_FPR0*4+104,  PT_FPR0*4+112,  PT_FPR0*4+120,
  PT_FPR0*4+128, PT_FPR0*4+136,  PT_FPR0*4+144,  PT_FPR0*4+152,
  PT_FPR0*4+160,  PT_FPR0*4+168,  PT_FPR0*4+176,  PT_FPR0*4+184,
  PT_FPR0*4+192,  PT_FPR0*4+200,  PT_FPR0*4+208,  PT_FPR0*4+216,
  PT_FPR0*4+224,  PT_FPR0*4+232,  PT_FPR0*4+240,  PT_FPR0*4+248,
  PT_NIP * 4,    PT_MSR * 4,    PT_CCR * 4,    PT_LNK * 4,
  PT_CTR * 4,    PT_XER * 4,    PT_FPSCR * 4,
  PT_ORIG_R3 * 4, PT_TRAP * 4
 };

static int ppc_regmap_e500[] =
 {PT_R0 * 4,     PT_R1 * 4,     PT_R2 * 4,     PT_R3 * 4,
  PT_R4 * 4,     PT_R5 * 4,     PT_R6 * 4,     PT_R7 * 4,
  PT_R8 * 4,     PT_R9 * 4,     PT_R10 * 4,    PT_R11 * 4,
  PT_R12 * 4,    PT_R13 * 4,    PT_R14 * 4,    PT_R15 * 4,
  PT_R16 * 4,    PT_R17 * 4,    PT_R18 * 4,    PT_R19 * 4,
  PT_R20 * 4,    PT_R21 * 4,    PT_R22 * 4,    PT_R23 * 4,
  PT_R24 * 4,    PT_R25 * 4,    PT_R26 * 4,    PT_R27 * 4,
  PT_R28 * 4,    PT_R29 * 4,    PT_R30 * 4,    PT_R31 * 4,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  PT_NIP * 4,    PT_MSR * 4,    PT_CCR * 4,    PT_LNK * 4,
  PT_CTR * 4,    PT_XER * 4,    -1,
  PT_ORIG_R3 * 4, PT_TRAP * 4
 };
#endif

static int
ppc_cannot_store_register (int regno)
{
#ifndef __powerpc64__
  /* Some kernels do not allow us to store fpscr.  */
  if (!(ppc_hwcap & PPC_FEATURE_HAS_SPE) && regno == find_regno ("fpscr"))
    return 2;
#endif

  /* Some kernels do not allow us to store orig_r3 or trap.  */
  if (regno == find_regno ("orig_r3")
      || regno == find_regno ("trap"))
    return 2;

  return 0;
}

static int
ppc_cannot_fetch_register (int regno)
{
  return 0;
}

static void
ppc_collect_ptrace_register (struct regcache *regcache, int regno, char *buf)
{
  int size = register_size (regno);

  memset (buf, 0, sizeof (long));

  if (size < sizeof (long))
    collect_register (regcache, regno, buf + sizeof (long) - size);
  else
    collect_register (regcache, regno, buf);
}

static void
ppc_supply_ptrace_register (struct regcache *regcache,
			    int regno, const char *buf)
{
  int size = register_size (regno);
  if (size < sizeof (long))
    supply_register (regcache, regno, buf + sizeof (long) - size);
  else
    supply_register (regcache, regno, buf);
}


#define INSTR_SC        0x44000002
#define NR_spu_run      0x0116

/* If the PPU thread is currently stopped on a spu_run system call,
   return to FD and ADDR the file handle and NPC parameter address
   used with the system call.  Return non-zero if successful.  */
static int
parse_spufs_run (struct regcache *regcache, int *fd, CORE_ADDR *addr)
{
  CORE_ADDR curr_pc;
  int curr_insn;
  int curr_r0;

  if (register_size (0) == 4)
    {
      unsigned int pc, r0, r3, r4;
      collect_register_by_name (regcache, "pc", &pc);
      collect_register_by_name (regcache, "r0", &r0);
      collect_register_by_name (regcache, "orig_r3", &r3);
      collect_register_by_name (regcache, "r4", &r4);
      curr_pc = (CORE_ADDR) pc;
      curr_r0 = (int) r0;
      *fd = (int) r3;
      *addr = (CORE_ADDR) r4;
    }
  else
    {
      unsigned long pc, r0, r3, r4;
      collect_register_by_name (regcache, "pc", &pc);
      collect_register_by_name (regcache, "r0", &r0);
      collect_register_by_name (regcache, "orig_r3", &r3);
      collect_register_by_name (regcache, "r4", &r4);
      curr_pc = (CORE_ADDR) pc;
      curr_r0 = (int) r0;
      *fd = (int) r3;
      *addr = (CORE_ADDR) r4;
    }

  /* Fetch instruction preceding current NIP.  */
  if ((*the_target->read_memory) (curr_pc - 4,
				  (unsigned char *) &curr_insn, 4) != 0)
    return 0;
  /* It should be a "sc" instruction.  */
  if (curr_insn != INSTR_SC)
    return 0;
  /* System call number should be NR_spu_run.  */
  if (curr_r0 != NR_spu_run)
    return 0;

  return 1;
}

static CORE_ADDR
ppc_get_pc (struct regcache *regcache)
{
  CORE_ADDR addr;
  int fd;

  if (parse_spufs_run (regcache, &fd, &addr))
    {
      unsigned int pc;
      (*the_target->read_memory) (addr, (unsigned char *) &pc, 4);
      return ((CORE_ADDR)1 << 63)
	| ((CORE_ADDR)fd << 32) | (CORE_ADDR) (pc - 4);
    }
  else if (register_size (0) == 4)
    {
      unsigned int pc;
      collect_register_by_name (regcache, "pc", &pc);
      return (CORE_ADDR) pc;
    }
  else
    {
      unsigned long pc;
      collect_register_by_name (regcache, "pc", &pc);
      return (CORE_ADDR) pc;
    }
}

static void
ppc_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  CORE_ADDR addr;
  int fd;

  if (parse_spufs_run (regcache, &fd, &addr))
    {
      unsigned int newpc = pc;
      (*the_target->write_memory) (addr, (unsigned char *) &newpc, 4);
    }
  else if (register_size (0) == 4)
    {
      unsigned int newpc = pc;
      supply_register_by_name (regcache, "pc", &newpc);
    }
  else
    {
      unsigned long newpc = pc;
      supply_register_by_name (regcache, "pc", &newpc);
    }
}


static int
ppc_get_hwcap (unsigned long *valp)
{
  int wordsize = register_size (0);
  unsigned char *data = alloca (2 * wordsize);
  int offset = 0;

  while ((*the_target->read_auxv) (offset, data, 2 * wordsize) == 2 * wordsize)
    {
      if (wordsize == 4)
	{
	  unsigned int *data_p = (unsigned int *)data;
	  if (data_p[0] == AT_HWCAP)
	    {
	      *valp = data_p[1];
	      return 1;
	    }
	}
      else
	{
	  unsigned long *data_p = (unsigned long *)data;
	  if (data_p[0] == AT_HWCAP)
	    {
	      *valp = data_p[1];
	      return 1;
	    }
	}

      offset += 2 * wordsize;
    }

  *valp = 0;
  return 0;
}

static void
ppc_arch_setup (void)
{
#ifdef __powerpc64__
  long msr;
  struct regcache *regcache;

  /* On a 64-bit host, assume 64-bit inferior process with no
     AltiVec registers.  Reset ppc_hwcap to ensure that the
     collect_register call below does not fail.  */
  init_registers_powerpc_64l ();
  ppc_hwcap = 0;

  /* Only if the high bit of the MSR is set, we actually have
     a 64-bit inferior.  */
  regcache = new_register_cache ();
  fetch_inferior_registers (regcache, find_regno ("msr"));
  collect_register_by_name (regcache, "msr", &msr);
  free_register_cache (regcache);
  if (msr < 0)
    {
      ppc_get_hwcap (&ppc_hwcap);
      if (ppc_hwcap & PPC_FEATURE_CELL)
	init_registers_powerpc_cell64l ();
      else if (ppc_hwcap & PPC_FEATURE_HAS_VSX)
	{
	  /* Power ISA 2.05 (implemented by Power 6 and newer processors)
	     increases the FPSCR from 32 bits to 64 bits. Even though Power 7
	     supports this ISA version, it doesn't have PPC_FEATURE_ARCH_2_05
	     set, only PPC_FEATURE_ARCH_2_06.  Since for now the only bits
	     used in the higher half of the register are for Decimal Floating
	     Point, we check if that feature is available to decide the size
	     of the FPSCR.  */
	  if (ppc_hwcap & PPC_FEATURE_HAS_DFP)
	    init_registers_powerpc_isa205_vsx64l ();
	  else
	    init_registers_powerpc_vsx64l ();
	}
      else if (ppc_hwcap & PPC_FEATURE_HAS_ALTIVEC)
	{
	  if (ppc_hwcap & PPC_FEATURE_HAS_DFP)
	    init_registers_powerpc_isa205_altivec64l ();
	  else
	    init_registers_powerpc_altivec64l ();
	}

      return;
    }
#endif

  /* OK, we have a 32-bit inferior.  */
  init_registers_powerpc_32l ();

  ppc_get_hwcap (&ppc_hwcap);
  if (ppc_hwcap & PPC_FEATURE_CELL)
    init_registers_powerpc_cell32l ();
  else if (ppc_hwcap & PPC_FEATURE_HAS_VSX)
    {
      if (ppc_hwcap & PPC_FEATURE_HAS_DFP)
	init_registers_powerpc_isa205_vsx32l ();
      else
	init_registers_powerpc_vsx32l ();
    }
  else if (ppc_hwcap & PPC_FEATURE_HAS_ALTIVEC)
    {
      if (ppc_hwcap & PPC_FEATURE_HAS_DFP)
	init_registers_powerpc_isa205_altivec32l ();
      else
	init_registers_powerpc_altivec32l ();
    }

  /* On 32-bit machines, check for SPE registers.
     Set the low target's regmap field as appropriately.  */
#ifndef __powerpc64__
  the_low_target.regmap = ppc_regmap;
  if (ppc_hwcap & PPC_FEATURE_HAS_SPE)
    {
      init_registers_powerpc_e500l ();
      the_low_target.regmap = ppc_regmap_e500;
   }

  /* If the FPSCR is 64-bit wide, we need to fetch the whole 64-bit
     slot and not just its second word.  The PT_FPSCR supplied in a
     32-bit GDB compilation doesn't reflect this.  */
  if (register_size (70) == 8)
    ppc_regmap[70] = (48 + 2*32) * sizeof (long);
#endif
}

/* Correct in either endianness.
   This instruction is "twge r2, r2", which GDB uses as a software
   breakpoint.  */
static const unsigned int ppc_breakpoint = 0x7d821008;
#define ppc_breakpoint_len 4

static int
ppc_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  if (where & ((CORE_ADDR)1 << 63))
    {
      char mem_annex[32];
      sprintf (mem_annex, "%d/mem", (int)((where >> 32) & 0x7fffffff));
      (*the_target->qxfer_spu) (mem_annex, (unsigned char *) &insn,
				NULL, where & 0xffffffff, 4);
      if (insn == 0x3fff)
	return 1;
    }
  else
    {
      (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
      if (insn == ppc_breakpoint)
	return 1;
      /* If necessary, recognize more trap instructions here.  GDB only uses
	 the one.  */
    }

  return 0;
}

/* Provide only a fill function for the general register set.  ps_lgetregs
   will use this for NPTL support.  */

static void ppc_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < 32; i++)
    ppc_collect_ptrace_register (regcache, i, (char *) buf + ppc_regmap[i]);

  for (i = 64; i < 70; i++)
    ppc_collect_ptrace_register (regcache, i, (char *) buf + ppc_regmap[i]);

  for (i = 71; i < 73; i++)
    ppc_collect_ptrace_register (regcache, i, (char *) buf + ppc_regmap[i]);
}

#ifndef PTRACE_GETVSXREGS
#define PTRACE_GETVSXREGS 27
#define PTRACE_SETVSXREGS 28
#endif

#define SIZEOF_VSXREGS 32*8

static void
ppc_fill_vsxregset (struct regcache *regcache, void *buf)
{
  int i, base;
  char *regset = buf;

  if (!(ppc_hwcap & PPC_FEATURE_HAS_VSX))
    return;

  base = find_regno ("vs0h");
  for (i = 0; i < 32; i++)
    collect_register (regcache, base + i, &regset[i * 8]);
}

static void
ppc_store_vsxregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const char *regset = buf;

  if (!(ppc_hwcap & PPC_FEATURE_HAS_VSX))
    return;

  base = find_regno ("vs0h");
  for (i = 0; i < 32; i++)
    supply_register (regcache, base + i, &regset[i * 8]);
}

#ifndef PTRACE_GETVRREGS
#define PTRACE_GETVRREGS 18
#define PTRACE_SETVRREGS 19
#endif

#define SIZEOF_VRREGS 33*16+4

static void
ppc_fill_vrregset (struct regcache *regcache, void *buf)
{
  int i, base;
  char *regset = buf;

  if (!(ppc_hwcap & PPC_FEATURE_HAS_ALTIVEC))
    return;

  base = find_regno ("vr0");
  for (i = 0; i < 32; i++)
    collect_register (regcache, base + i, &regset[i * 16]);

  collect_register_by_name (regcache, "vscr", &regset[32 * 16 + 12]);
  collect_register_by_name (regcache, "vrsave", &regset[33 * 16]);
}

static void
ppc_store_vrregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const char *regset = buf;

  if (!(ppc_hwcap & PPC_FEATURE_HAS_ALTIVEC))
    return;

  base = find_regno ("vr0");
  for (i = 0; i < 32; i++)
    supply_register (regcache, base + i, &regset[i * 16]);

  supply_register_by_name (regcache, "vscr", &regset[32 * 16 + 12]);
  supply_register_by_name (regcache, "vrsave", &regset[33 * 16]);
}

#ifndef PTRACE_GETEVRREGS
#define PTRACE_GETEVRREGS	20
#define PTRACE_SETEVRREGS	21
#endif

struct gdb_evrregset_t
{
  unsigned long evr[32];
  unsigned long long acc;
  unsigned long spefscr;
};

static void
ppc_fill_evrregset (struct regcache *regcache, void *buf)
{
  int i, ev0;
  struct gdb_evrregset_t *regset = buf;

  if (!(ppc_hwcap & PPC_FEATURE_HAS_SPE))
    return;

  ev0 = find_regno ("ev0h");
  for (i = 0; i < 32; i++)
    collect_register (regcache, ev0 + i, &regset->evr[i]);

  collect_register_by_name (regcache, "acc", &regset->acc);
  collect_register_by_name (regcache, "spefscr", &regset->spefscr);
}

static void
ppc_store_evrregset (struct regcache *regcache, const void *buf)
{
  int i, ev0;
  const struct gdb_evrregset_t *regset = buf;

  if (!(ppc_hwcap & PPC_FEATURE_HAS_SPE))
    return;

  ev0 = find_regno ("ev0h");
  for (i = 0; i < 32; i++)
    supply_register (regcache, ev0 + i, &regset->evr[i]);

  supply_register_by_name (regcache, "acc", &regset->acc);
  supply_register_by_name (regcache, "spefscr", &regset->spefscr);
}

struct regset_info target_regsets[] = {
  /* List the extra register sets before GENERAL_REGS.  That way we will
     fetch them every time, but still fall back to PTRACE_PEEKUSER for the
     general registers.  Some kernels support these, but not the newer
     PPC_PTRACE_GETREGS.  */
  { PTRACE_GETVSXREGS, PTRACE_SETVSXREGS, 0, SIZEOF_VSXREGS, EXTENDED_REGS,
  ppc_fill_vsxregset, ppc_store_vsxregset },
  { PTRACE_GETVRREGS, PTRACE_SETVRREGS, 0, SIZEOF_VRREGS, EXTENDED_REGS,
    ppc_fill_vrregset, ppc_store_vrregset },
  { PTRACE_GETEVRREGS, PTRACE_SETEVRREGS, 0, 32 * 4 + 8 + 4, EXTENDED_REGS,
    ppc_fill_evrregset, ppc_store_evrregset },
  { 0, 0, 0, 0, GENERAL_REGS, ppc_fill_gregset, NULL },
  { 0, 0, 0, -1, -1, NULL, NULL }
};

struct linux_target_ops the_low_target = {
  ppc_arch_setup,
  ppc_num_regs,
  ppc_regmap,
  NULL,
  ppc_cannot_fetch_register,
  ppc_cannot_store_register,
  NULL, /* fetch_register */
  ppc_get_pc,
  ppc_set_pc,
  (const unsigned char *) &ppc_breakpoint,
  ppc_breakpoint_len,
  NULL,
  0,
  ppc_breakpoint_at,
  NULL,
  NULL,
  NULL,
  NULL,
  ppc_collect_ptrace_register,
  ppc_supply_ptrace_register,
};
