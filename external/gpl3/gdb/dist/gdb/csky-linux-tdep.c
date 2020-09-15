/* Target-dependent code for GNU/Linux on CSKY.

   Copyright (C) 2012-2020 Free Software Foundation, Inc.

   Contributed by C-SKY Microsystems and Mentor Graphics.

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
#include "glibc-tdep.h"
#include "linux-tdep.h"
#include "gdbarch.h"
#include "solib-svr4.h"
#include "regset.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "csky-tdep.h"

/* Functions, definitions, and data structures for C-Sky core file debug.  */

/* General regset pc, r1, r0, psr, r2-r31 for CK810.  */
#define SIZEOF_CSKY_GREGSET 34*4
/* Float regset fesr fsr fr0-fr31 for CK810.  */
#define SIZEOF_CSKY_FREGSET 34*4

/* Offset mapping table from core_section to regcache of general
   registers for ck810.  */
static const int csky_gregset_offset[] =
{
  72,  1,  0, 89,  2,  /* pc, r1, r0, psr, r2.  */
   3,  4,  5,  6,  7,  /* r3 ~ r32.  */
   8,  9, 10, 11, 12,
  13, 14, 15, 16, 17,
  18, 19, 20, 21, 22,
  23, 24, 25, 26, 27,
  28, 29, 30, 31
};

/* Offset mapping table from core_section to regcache of float
   registers for ck810.  */

static const int csky_fregset_offset[] =
{
  122, 123, 40, 41, 42,     /* fcr, fesr, fr0 ~ fr2.  */
   43,  44, 45, 46, 47,     /* fr3 ~ fr15.  */
   48,  49, 50, 51, 52,
   53,  54, 55
};

/* Implement the supply_regset hook for GP registers in core files.  */

static void
csky_supply_gregset (const struct regset *regset,
		     struct regcache *regcache, int regnum,
		     const void *regs, size_t len)
{
  int i, gregset_num;
  const gdb_byte *gregs = (const gdb_byte *) regs ;

  gdb_assert (len >= SIZEOF_CSKY_GREGSET);
  gregset_num = ARRAY_SIZE (csky_gregset_offset);

  for (i = 0; i < gregset_num; i++)
    {
      if ((regnum == csky_gregset_offset[i] || regnum == -1)
	  && csky_gregset_offset[i] != -1)
	regcache->raw_supply (csky_gregset_offset[i], gregs + 4 * i);
    }
}

/* Implement the collect_regset hook for GP registers in core files.  */

static void
csky_collect_gregset (const struct regset *regset,
		      const struct regcache *regcache,
		      int regnum, void *gregs_buf, size_t len)
{
  int regno, gregset_num;
  gdb_byte *gregs = (gdb_byte *) gregs_buf ;

  gdb_assert (len >= SIZEOF_CSKY_GREGSET);
  gregset_num = ARRAY_SIZE (csky_gregset_offset);

  for (regno = 0; regno < gregset_num; regno++)
    {
      if ((regnum == csky_gregset_offset[regno] || regnum == -1)
	  && csky_gregset_offset[regno] != -1)
	regcache->raw_collect (regno,
			       gregs + 4 + csky_gregset_offset[regno]);
    }
}

/* Implement the supply_regset hook for FP registers in core files.  */

static void
csky_supply_fregset (const struct regset *regset,
		     struct regcache *regcache, int regnum,
		     const void *regs, size_t len)
{
  int i;
  int offset = 0;
  struct gdbarch *gdbarch = regcache->arch ();
  const gdb_byte *fregs = (const gdb_byte *) regs;
  int fregset_num = ARRAY_SIZE (csky_fregset_offset);

  gdb_assert (len >= SIZEOF_CSKY_FREGSET);
  for (i = 0; i < fregset_num; i++)
    {
      if ((regnum == csky_fregset_offset[i] || regnum == -1)
	  && csky_fregset_offset[i] != -1)
	{
	  int num = csky_fregset_offset[i];
	  offset += register_size (gdbarch, num);
	  regcache->raw_supply (csky_fregset_offset[i], fregs + offset);
	}
    }
}

/* Implement the collect_regset hook for FP registers in core files.  */

static void
csky_collect_fregset (const struct regset *regset,
		      const struct regcache *regcache,
		      int regnum, void *fregs_buf, size_t len)
{
  int regno;
  struct gdbarch *gdbarch = regcache->arch ();
  gdb_byte *fregs = (gdb_byte *) fregs_buf ;
  int fregset_num = ARRAY_SIZE (csky_fregset_offset);
  int offset = 0;

  gdb_assert (len >= SIZEOF_CSKY_FREGSET);
  for (regno = 0; regno < fregset_num; regno++)
    {
      if ((regnum == csky_fregset_offset[regno] || regnum == -1)
	  && csky_fregset_offset[regno] != -1)
	{
	  offset += register_size (gdbarch, csky_fregset_offset[regno]);
	  regcache->raw_collect (regno, fregs + offset);
	}
    }
}

static const struct regset csky_regset_general =
{
  NULL,
  csky_supply_gregset,
  csky_collect_gregset
};

static const struct regset csky_regset_float =
{
  NULL,
  csky_supply_fregset,
  csky_collect_fregset
};

/* Iterate over core file register note sections.  */

static void
csky_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  cb (".reg", sizeof (csky_gregset_offset), sizeof (csky_gregset_offset),
      &csky_regset_general, NULL, cb_data);
  cb (".reg2", sizeof (csky_fregset_offset), sizeof (csky_fregset_offset),
      &csky_regset_float, NULL, cb_data);
}

static void
csky_linux_rt_sigreturn_init (const struct tramp_frame *self,
			      struct frame_info *this_frame,
			      struct trad_frame_cache *this_cache,
			      CORE_ADDR func)
{
  int i;
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, 14);

  CORE_ADDR base = sp + CSKY_SIGINFO_OFFSET + CSKY_SIGINFO_SIZE
		   + CSKY_UCONTEXT_SIGCONTEXT
		   + CSKY_SIGCONTEXT_SC_USP
		   + CSKY_SIGCONTEXT_SC_A0;

  /* Set addrs of R0 ~ R13.  */
  for (i = 0; i < 14; i++)
   trad_frame_set_reg_addr (this_cache, i, base + i * 4);

  /* Set addrs of SP(R14) and R15.  */
  trad_frame_set_reg_addr (this_cache, 14, base - 4);
  trad_frame_set_reg_addr (this_cache, 15, base + 4 * 14);

  /* Set addrs of R16 ~ R31.  */
  for (i = 15; i < 31; i++)
   trad_frame_set_reg_addr (this_cache, i, base + i * 4);

  /* Set addrs of PSR and PC.  */
  trad_frame_set_reg_addr (this_cache, 89, base + 4 * 33);
  trad_frame_set_reg_addr (this_cache, 72, base + 4 * 34);

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static struct tramp_frame
csky_linux_rt_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { CSKY_MOVI_R7_173, ULONGEST_MAX },
    { CSKY_TRAP_0, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  csky_linux_rt_sigreturn_init
};

/* Hook function for gdbarch_register_osabi.  */

static void
csky_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);
  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 svr4_ilp32_fetch_link_map_offsets);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections (
    gdbarch, csky_linux_iterate_over_regset_sections);

  /* Append tramp frame unwinder for SIGNAL.  */

  tramp_frame_prepend_unwinder (gdbarch,
				&csky_linux_rt_sigreturn_tramp_frame);
}

void _initialize_csky_linux_tdep ();
void
_initialize_csky_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_csky, 0, GDB_OSABI_LINUX,
			  csky_linux_init_abi);
}
