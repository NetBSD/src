/* Copyright (C) 2009-2013 Free Software Foundation, Inc.

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
#include "lynx-low.h"

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <sys/ptrace.h>

/* The following two typedefs are defined in a .h file which is not
   in the standard include path (/sys/include/family/ppc/ucontext.h),
   so we just duplicate them here.  */

/* General register context */
typedef struct usr_econtext_s
{
        uint32_t        uec_iregs[32];
        uint32_t        uec_inum;
        uint32_t        uec_srr0;
        uint32_t        uec_srr1;
        uint32_t        uec_lr;
        uint32_t        uec_ctr;
        uint32_t        uec_cr;
        uint32_t        uec_xer;
        uint32_t        uec_dar;
        uint32_t        uec_mq;
        uint32_t        uec_msr;
        uint32_t        uec_sregs[16];
        uint32_t        uec_ss_count;
        uint32_t        uec_ss_addr1;
        uint32_t        uec_ss_addr2;
        uint32_t        uec_ss_code1;
        uint32_t        uec_ss_code2;
} usr_econtext_t;

/* Floating point register context */
typedef struct usr_fcontext_s
{
        uint64_t        ufc_freg[32];
        uint32_t        ufc_fpscr[2];
} usr_fcontext_t;

/* Index of for various registers inside the regcache.  */
#define R0_REGNUM    0
#define F0_REGNUM    32
#define PC_REGNUM    64
#define MSR_REGNUM   65
#define CR_REGNUM    66
#define LR_REGNUM    67
#define CTR_REGNUM   68
#define XER_REGNUM   69
#define FPSCR_REGNUM 70

/* Defined in auto-generated file powerpc-32.c.  */
extern void init_registers_powerpc_32 (void);

/* The fill_function for the general-purpose register set.  */

static void
lynx_ppc_fill_gregset (struct regcache *regcache, char *buf)
{
  int i;

  /* r0 - r31 */
  for (i = 0; i < 32; i++)
    collect_register (regcache, R0_REGNUM + i,
                      buf + offsetof (usr_econtext_t, uec_iregs[i]));

  /* The other registers provided in the GP register context.  */
  collect_register (regcache, PC_REGNUM,
                    buf + offsetof (usr_econtext_t, uec_srr0));
  collect_register (regcache, MSR_REGNUM,
                    buf + offsetof (usr_econtext_t, uec_srr1));
  collect_register (regcache, CR_REGNUM,
                    buf + offsetof (usr_econtext_t, uec_cr));
  collect_register (regcache, LR_REGNUM,
                    buf + offsetof (usr_econtext_t, uec_lr));
  collect_register (regcache, CTR_REGNUM,
                    buf + offsetof (usr_econtext_t, uec_ctr));
  collect_register (regcache, XER_REGNUM,
                    buf + offsetof (usr_econtext_t, uec_xer));
}

/* The store_function for the general-purpose register set.  */

static void
lynx_ppc_store_gregset (struct regcache *regcache, const char *buf)
{
  int i;

  /* r0 - r31 */
  for (i = 0; i < 32; i++)
    supply_register (regcache, R0_REGNUM + i,
                      buf + offsetof (usr_econtext_t, uec_iregs[i]));

  /* The other registers provided in the GP register context.  */
  supply_register (regcache, PC_REGNUM,
                   buf + offsetof (usr_econtext_t, uec_srr0));
  supply_register (regcache, MSR_REGNUM,
                   buf + offsetof (usr_econtext_t, uec_srr1));
  supply_register (regcache, CR_REGNUM,
                   buf + offsetof (usr_econtext_t, uec_cr));
  supply_register (regcache, LR_REGNUM,
                   buf + offsetof (usr_econtext_t, uec_lr));
  supply_register (regcache, CTR_REGNUM,
                   buf + offsetof (usr_econtext_t, uec_ctr));
  supply_register (regcache, XER_REGNUM,
                   buf + offsetof (usr_econtext_t, uec_xer));
}

/* The fill_function for the floating-point register set.  */

static void
lynx_ppc_fill_fpregset (struct regcache *regcache, char *buf)
{
  int i;

  /* f0 - f31 */
  for (i = 0; i < 32; i++)
    collect_register (regcache, F0_REGNUM + i,
                      buf + offsetof (usr_fcontext_t, ufc_freg[i]));

  /* fpscr */
  collect_register (regcache, FPSCR_REGNUM,
                    buf + offsetof (usr_fcontext_t, ufc_fpscr));
}

/* The store_function for the floating-point register set.  */

static void
lynx_ppc_store_fpregset (struct regcache *regcache, const char *buf)
{
  int i;

  /* f0 - f31 */
  for (i = 0; i < 32; i++)
    supply_register (regcache, F0_REGNUM + i,
                     buf + offsetof (usr_fcontext_t, ufc_freg[i]));

  /* fpscr */
  supply_register (regcache, FPSCR_REGNUM,
                   buf + offsetof (usr_fcontext_t, ufc_fpscr));
}

/* Implements the lynx_target_ops.arch_setup routine.  */

static void
lynx_ppc_arch_setup (void)
{
  init_registers_powerpc_32 ();
}

/* Description of all the powerpc-lynx register sets.  */

struct lynx_regset_info lynx_target_regsets[] = {
  /* General Purpose Registers.  */
  {PTRACE_GETREGS, PTRACE_SETREGS, sizeof(usr_econtext_t),
   lynx_ppc_fill_gregset, lynx_ppc_store_gregset},
  /* Floating Point Registers.  */
  { PTRACE_GETFPREGS, PTRACE_SETFPREGS, sizeof(usr_fcontext_t),
    lynx_ppc_fill_fpregset, lynx_ppc_store_fpregset },
  /* End of list marker.  */
  {0, 0, -1, NULL, NULL }
};

/* The lynx_target_ops vector for powerpc-lynxos.  */

struct lynx_target_ops the_low_target = {
  lynx_ppc_arch_setup,
};
