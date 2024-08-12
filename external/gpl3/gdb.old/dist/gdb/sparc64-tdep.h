/* Target-dependent code for UltraSPARC.

   Copyright (C) 2003-2023 Free Software Foundation, Inc.

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

#ifndef SPARC64_TDEP_H
#define SPARC64_TDEP_H 1

class frame_info_ptr;
struct gdbarch;
struct regcache;
struct sparc_gregmap;
struct trad_frame_saved_reg;

#include "sparc-tdep.h"

/* The stack pointer is offset from the stack frame by a BIAS of 2047
   (0x7ff) for 64-bit code.  BIAS is likely to be defined on SPARC
   hosts, so undefine it first.  */
#undef BIAS
#define BIAS 2047

/* Register offsets for the general-purpose register set.  */

/* UltraSPARC doesn't have %psr.  */
#define r_tstate_offset r_psr_offset

/* UltraSPARC doesn't have %wim either.  */
#define r_fprs_offset r_wim_offset

/* Register numbers of various important registers.  */

enum sparc64_regnum
{
  SPARC64_F32_REGNUM		/* %f32 */
  = SPARC_F0_REGNUM + 32,
  SPARC64_F62_REGNUM		/* %f62 */
  = SPARC64_F32_REGNUM + 15,
  SPARC64_PC_REGNUM,		/* %pc */
  SPARC64_NPC_REGNUM,		/* %npc */
  SPARC64_STATE_REGNUM,
  SPARC64_FSR_REGNUM,		/* %fsr */
  SPARC64_FPRS_REGNUM,		/* %fprs */
  SPARC64_Y_REGNUM,		/* %y */
};

/* Pseudo registers.  */
enum sparc64_pseudo_regnum
{
  SPARC64_CWP_REGNUM = 0,	/* %cwp */
  SPARC64_PSTATE_REGNUM,	/* %pstate */
  SPARC64_ASI_REGNUM,		/* %asi */
  SPARC64_CCR_REGNUM,		/* %ccr */
  SPARC64_D0_REGNUM,		/* %d0 */
  SPARC64_D10_REGNUM		/* %d10 */
  = SPARC64_D0_REGNUM + 5,
  SPARC64_D30_REGNUM		/* %d30 */
  = SPARC64_D0_REGNUM + 15,
  SPARC64_D32_REGNUM		/* %d32 */
  = SPARC64_D0_REGNUM + 16,
  SPARC64_D62_REGNUM		/* %d62 */
  = SPARC64_D0_REGNUM + 31,
  SPARC64_Q0_REGNUM,		/* %q0 */
  SPARC64_Q8_REGNUM		/* %q8 */
  = SPARC64_Q0_REGNUM + 2,
  SPARC64_Q28_REGNUM		/* %q28 */
  = SPARC64_Q0_REGNUM + 7,
  SPARC64_Q32_REGNUM		/* %q32 */
  = SPARC64_Q0_REGNUM + 8,
  SPARC64_Q60_REGNUM		/* %q60 */
  = SPARC64_Q0_REGNUM + 15
};

/* Processor state bits.  */
#define SPARC64_PSTATE_AG	0x001
#define SPARC64_PSTATE_IE	0x002
#define SPARC64_PSTATE_PRIV	0x004
#define SPARC64_PSTATE_AM	0x008
#define SPARC64_PSTATE_PEF	0x010
#define SPARC64_PSTATE_RED	0x020
#define SPARC64_PSTATE_TLE	0x100
#define SPARC64_PSTATE_CLE	0x200
#define SPARC64_PSTATE_PID0	0x400
#define SPARC64_PSTATE_PID1	0x800

extern void sparc64_init_abi (struct gdbarch_info info,
			      struct gdbarch *gdbarch);

extern void sparc64_supply_gregset (const struct sparc_gregmap *gregmap,
				    struct regcache *regcache,
				    int regnum, const void *gregs);
extern void sparc64_collect_gregset (const struct sparc_gregmap *gregmap,
				     const struct regcache *regcache,
				     int regnum, void *gregs);
extern void sparc64_supply_fpregset (const struct sparc_fpregmap *fpregmap,
				     struct regcache *regcache,
				     int regnum, const void *fpregs);
extern void sparc64_collect_fpregset (const struct sparc_fpregmap *fpregmap,
				      const struct regcache *regcache,
				      int regnum, void *fpregs);

/* Functions and variables exported from sparc64-sol2-tdep.c.  */

/* Register offsets for Solaris 2.  */
extern const struct sparc_gregmap sparc64_sol2_gregmap;
extern const struct sparc_fpregmap sparc64_sol2_fpregmap;

/* Variables exported from sparc64-fbsd-tdep.c.  */

/* Register offsets for FreeBSD/sparc64.  */
extern const struct sparc_gregmap sparc64fbsd_gregmap;

/* Functions and variables exported from sparc64-netbsd-tdep.c.  */

/* Register offsets for NetBSD/sparc64.  */
extern const struct sparc_gregmap sparc64nbsd_gregmap;

extern struct trad_frame_saved_reg *
  sparc64nbsd_sigcontext_saved_regs (CORE_ADDR sigcontext_addr,
				     frame_info_ptr next_frame);

extern const struct sparc_fpregmap sparc64_bsd_fpregmap;

extern void sparc64_forget_process (pid_t pid);

#endif /* sparc64-tdep.h */
