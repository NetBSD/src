/* Target-dependent code for NetBSD/alpha.

   Copyright (C) 2002-2017 Free Software Foundation, Inc.

   Contributed by Wasabi Systems, Inc.

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
#include "frame.h"
#include "gdbcore.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "value.h"

#include "alpha-tdep.h"
#include "nbsd-tdep.h"
#include "solib-svr4.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "tramp-frame.h"
#include "target.h"

/* Core file support.  */

/* Even though NetBSD/alpha used ELF since day one, it used the
   traditional a.out-style core dump format before NetBSD 1.6.  */

/* Sizeof `struct reg' in <machine/reg.h>.  */
#define ALPHANBSD_SIZEOF_GREGS	(32 * 8)

/* Sizeof `struct fpreg' in <machine/reg.h.  */
#define ALPHANBSD_SIZEOF_FPREGS	((32 * 8) + 8)

/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
alphanbsd_supply_fpregset (const struct regset *regset,
			   struct regcache *regcache,
			   int regnum, const void *fpregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) fpregs;
  int i;

  gdb_assert (len >= ALPHANBSD_SIZEOF_FPREGS);

  for (i = ALPHA_FP0_REGNUM; i < ALPHA_FP0_REGNUM + 31; i++)
    {
      if (regnum == i || regnum == -1)
	regcache_raw_supply (regcache, i, regs + (i - ALPHA_FP0_REGNUM) * 8);
    }

  if (regnum == ALPHA_FPCR_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, ALPHA_FPCR_REGNUM, regs + 32 * 8);
}

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
alphanbsd_aout_supply_gregset (const struct regset *regset,
			       struct regcache *regcache,
			       int regnum, const void *gregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) gregs;
  int i;

  /* Table to map a GDB register number to a trapframe register index.  */
  static const int regmap[] =
  {
     0,   1,   2,   3,
     4,   5,   6,   7,
     8,   9,  10,  11,
    12,  13,  14,  15, 
    30,  31,  32,  16, 
    17,  18,  19,  20,
    21,  22,  23,  24,
    25,  29,  26
  };

  gdb_assert (len >= ALPHANBSD_SIZEOF_GREGS);

  for (i = 0; i < ARRAY_SIZE(regmap); i++)
    {
      if (regnum == i || regnum == -1)
	regcache_raw_supply (regcache, i, regs + regmap[i] * 8);
    }

  if (regnum == ALPHA_PC_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, ALPHA_PC_REGNUM, regs + 31 * 8);

  if (len >= ALPHANBSD_SIZEOF_GREGS + ALPHANBSD_SIZEOF_FPREGS)
    {
      regs += ALPHANBSD_SIZEOF_GREGS;
      len -= ALPHANBSD_SIZEOF_GREGS;
      alphanbsd_supply_fpregset (regset, regcache, regnum, regs, len);
    }
}

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
alphanbsd_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) gregs;
  int i;

  if (len >= ALPHANBSD_SIZEOF_GREGS + ALPHANBSD_SIZEOF_FPREGS)
    {
      alphanbsd_aout_supply_gregset (regset, regcache, regnum, gregs, len);
      return;
    }

  for (i = 0; i < ALPHA_ZERO_REGNUM; i++)
    {
      if (regnum == i || regnum == -1)
	regcache_raw_supply (regcache, i, regs + i * 8);
    }

  if (regnum == ALPHA_PC_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, ALPHA_PC_REGNUM, regs + 31 * 8);
}

/* NetBSD/alpha register sets.  */

static const struct regset alphanbsd_gregset =
{
  NULL,
  alphanbsd_supply_gregset,
  NULL,
  REGSET_VARIABLE_SIZE
};

static const struct regset alphanbsd_fpregset =
{
  NULL,
  alphanbsd_supply_fpregset
};

/* Iterate over supported core file register note sections. */

void
alphanbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  cb (".reg", ALPHANBSD_SIZEOF_GREGS, &alphanbsd_gregset, NULL, cb_data);
  cb (".reg2", ALPHANBSD_SIZEOF_FPREGS, &alphanbsd_fpregset, NULL, cb_data);
}


/* Signal trampolines.  */

static void
alphanbsd_sigtramp_cache_init (const struct tramp_frame *,
			       struct frame_info *,
			       struct trad_frame_cache *,
			       CORE_ADDR);
/* Under NetBSD/alpha, signal handler invocations can be identified by the
   designated code sequence that is used to return from a signal handler.
   In particular, the return address of a signal handler points to the
   following code sequences: */
static const struct tramp_frame alphanbsd_sigtramp_sc1 = {
  SIGTRAMP_FRAME,
  4,
  {
    { 0xa61e0000, 0xffffffff },		/* ldq a0, 0(sp) */
    { 0x23de0010, 0xffffffff },		/* lda sp, 16(sp) */
    { 0x201f0127, 0xffffffff },		/* lda v0, 295 */
    { 0x00000083, 0xffffffff },		/* call_pal callsys */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  alphanbsd_sigtramp_cache_init
};

/* The siginfo signal trampoline for NetBSD/alpha introduced in 2.0 */
static const struct tramp_frame alphanbsd_sigtramp_si2 =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x221e0080, -1 },		/* lda	a0,128(sp) */
    { 0x201f0134, -1 },		/* lda	v0,308 */
    { 0x00000083, -1 },		/* callsys */
    { 0x47e00410, -1 },		/* mov	v0,a0 */
    { 0x201f0001, -1 },		/* lda	v0,1 */
    { 0x00000083, -1 },		/* callsys */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  alphanbsd_sigtramp_cache_init
};
/* The siginfo signal trampoline for NetBSD/alpha introduced in 4.0 */
static const struct tramp_frame alphanbsd_sigtramp_si4 =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x27ba0000, 0xffff0000 },
    { 0x23bd0000, 0xffff0000 },	/* ldgp	gp,0(ra) */
    { 0x221e0080, -1 },		/* lda	a0,128(sp) */
    { 0x201f0134, -1 },		/* lda	v0,308 */
    { 0x00000083, -1 },		/* callsys */
    { 0x221fffff, -1 },		/* lda	a0,-1 */
    { 0x201f0001, -1 },		/* lda	v0,1 */
    { 0x00000083, -1 },		/* callsys */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  alphanbsd_sigtramp_cache_init
};

static void
alphanbsd_sigtramp_cache_init (const struct tramp_frame *self,
			       struct frame_info *this_frame,
			       struct trad_frame_cache *this_cache,
			       CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  CORE_ADDR addr, sp;
  int i;

  sp = get_frame_register_unsigned (this_frame, ALPHA_SP_REGNUM);

  if (self == &alphanbsd_sigtramp_sc1) {
    addr = sp;
  } else {
    addr = sp + 128 + 56;
  }
 
  for (i = 0; i < 32; i++, addr += ALPHA_REGISTER_SIZE)
    {
      trad_frame_set_reg_addr (this_cache, i, addr);
    }
  trad_frame_set_reg_addr (this_cache, ALPHA_PC_REGNUM, addr);

  /* Construct the frame ID using the function start.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

#ifdef notyet
#define RETCODE_NWORDS          4
#define RETCODE_SIZE            (RETCODE_NWORDS * 4)

static LONGEST
alphanbsd_sigtramp_offset (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  gdb_byte ret[RETCODE_SIZE], w[4];
  LONGEST off;
  int i;

  if (target_read_memory (pc, w, 4) != 0)
    return -1;

  for (i = 0; i < RETCODE_NWORDS; i++)
    {
      if (memcmp (w, sigtramp_retcode + (i * 4), 4) == 0)
	break;
    }
  if (i == RETCODE_NWORDS)
    return (-1);

  off = i * 4;
  pc -= off;

  if (target_read_memory (pc, ret, sizeof (ret)) != 0)
    return -1;

  if (memcmp (ret, sigtramp_retcode, RETCODE_SIZE) == 0)
    return off;

  return -1;
}

static int
alphanbsd_pc_in_sigtramp (struct gdbarch *gdbarch,
		 	  CORE_ADDR pc, const char *func_name)
{
  return (nbsd_pc_in_sigtramp (pc, func_name)
	  || alphanbsd_sigtramp_offset (gdbarch, pc) >= 0);
}

static CORE_ADDR
alphanbsd_sigcontext_addr (struct frame_info *frame)
{
  /* FIXME: This is not correct for all versions of NetBSD/alpha.
     We will probably need to disassemble the trampoline to figure
     out which trampoline frame type we have.  */
  if (!get_next_frame (frame))
    return 0;
  return get_frame_base (get_next_frame (frame));
}
#endif


static void
alphanbsd_init_abi (struct gdbarch_info info,
                    struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Hook into the DWARF CFI frame unwinder.  */
  alpha_dwarf2_init_abi (info, gdbarch);

  /* Hook into the MDEBUG frame unwinder.  */
  alpha_mdebug_init_abi (info, gdbarch);

  /* NetBSD/alpha does not provide single step support via ptrace(2); we
     must use software single-stepping.  */
  set_gdbarch_software_single_step (gdbarch, alpha_software_single_step);
  /* NetBSD/alpha has SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);
  set_gdbarch_skip_solib_resolver (gdbarch, nbsd_skip_solib_resolver);

#ifdef notyet
  tdep->dynamic_sigtramp_offset = alphanbsd_sigtramp_offset;
  tdep->pc_in_sigtramp = alphanbsd_pc_in_sigtramp;
  tdep->sigcontext_addr = alphanbsd_sigcontext_addr;
#endif

  tdep->jb_pc = 2;
  tdep->jb_elt_size = 8;

  tramp_frame_prepend_unwinder (gdbarch, &alphanbsd_sigtramp_sc1);
  tramp_frame_prepend_unwinder (gdbarch, &alphanbsd_sigtramp_si2);
  tramp_frame_prepend_unwinder (gdbarch, &alphanbsd_sigtramp_si4);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, alphanbsd_iterate_over_regset_sections);
}


static enum gdb_osabi
alphanbsd_core_osabi_sniffer (bfd *abfd)
{
  if (strcmp (bfd_get_target (abfd), "netbsd-core") == 0)
    return GDB_OSABI_NETBSD;

  return GDB_OSABI_UNKNOWN;
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_alphanbsd_tdep (void);

void
_initialize_alphanbsd_tdep (void)
{
  /* BFD doesn't set a flavour for NetBSD style a.out core files.  */
  gdbarch_register_osabi_sniffer (bfd_arch_alpha, bfd_target_unknown_flavour,
                                  alphanbsd_core_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_alpha, 0, GDB_OSABI_NETBSD,
                          alphanbsd_init_abi);
}
