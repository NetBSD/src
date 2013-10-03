/* Copyright (C) 2010-2013 Free Software Foundation, Inc.

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
#include <limits.h>
#include <sys/ptrace.h>

/* The following two typedefs are defined in a .h file which is not
   in the standard include path (/sys/include/family/x86/ucontext.h),
   so we just duplicate them here.  */

/* General register context */
typedef struct usr_econtext {

    uint32_t    uec_fault;
    uint32_t    uec_es;
    uint32_t    uec_ds;
    uint32_t    uec_edi;
    uint32_t    uec_esi;
    uint32_t    uec_ebp;
    uint32_t    uec_temp;
    uint32_t    uec_ebx;
    uint32_t    uec_edx;
    uint32_t    uec_ecx;
    uint32_t    uec_eax;
    uint32_t    uec_inum;
    uint32_t    uec_ecode;
    uint32_t    uec_eip;
    uint32_t    uec_cs;
    uint32_t    uec_eflags;
    uint32_t    uec_esp;
    uint32_t    uec_ss;
    uint32_t    uec_fs;
    uint32_t    uec_gs;
} usr_econtext_t;

/* Floating point and SIMD register context */
typedef struct usr_fcontext {
        uint16_t         ufc_control;
        uint16_t         ufc_status;
        uint16_t         ufc_tag;
        uint16_t         ufc_opcode;
        uint8_t         *ufc_inst_off;
        uint32_t         ufc_inst_sel;
        uint8_t         *ufc_data_off;
        uint32_t         ufc_data_sel;
        uint32_t         usse_mxcsr;
        uint32_t         usse_mxcsr_mask;
        struct ufp387_real {
                uint16_t umant4;
                uint16_t umant3;
                uint16_t umant2;
                uint16_t umant1;
                uint16_t us_and_e;
                uint16_t ureserved_1;
                uint16_t ureserved_2;
                uint16_t ureserved_3;
        } ufc_reg[8];
        struct uxmm_register {
                uint16_t uchunk_1;
                uint16_t uchunk_2;
                uint16_t uchunk_3;
                uint16_t uchunk_4;
                uint16_t uchunk_5;
                uint16_t uchunk_6;
                uint16_t uchunk_7;
                uint16_t uchunk_8;
        } uxmm_reg[8];
        char ureserved[16][14];
} usr_fcontext_t;

/* The index of various registers inside the regcache.  */

enum lynx_i386_gdb_regnum
{
  I386_EAX_REGNUM,
  I386_ECX_REGNUM,
  I386_EDX_REGNUM,
  I386_EBX_REGNUM,
  I386_ESP_REGNUM,
  I386_EBP_REGNUM,
  I386_ESI_REGNUM,
  I386_EDI_REGNUM,
  I386_EIP_REGNUM,
  I386_EFLAGS_REGNUM,
  I386_CS_REGNUM,
  I386_SS_REGNUM,
  I386_DS_REGNUM,
  I386_ES_REGNUM,
  I386_FS_REGNUM,
  I386_GS_REGNUM,
  I386_ST0_REGNUM,
  I386_FCTRL_REGNUM = I386_ST0_REGNUM + 8,
  I386_FSTAT_REGNUM,
  I386_FTAG_REGNUM,
  I386_FISEG_REGNUM,
  I386_FIOFF_REGNUM,
  I386_FOSEG_REGNUM,
  I386_FOOFF_REGNUM,
  I386_FOP_REGNUM,
  I386_XMM0_REGNUM = 32,
  I386_MXCSR_REGNUM = I386_XMM0_REGNUM + 8,
  I386_SENTINEL_REGUM
};

/* Defined in auto-generated file i386.c.  */
extern void init_registers_i386 (void);

/* The fill_function for the general-purpose register set.  */

static void
lynx_i386_fill_gregset (struct regcache *regcache, char *buf)
{
#define lynx_i386_collect_gp(regnum, fld) \
  collect_register (regcache, regnum, \
                    buf + offsetof (usr_econtext_t, uec_##fld))

  lynx_i386_collect_gp (I386_EAX_REGNUM, eax);
  lynx_i386_collect_gp (I386_ECX_REGNUM, ecx);
  lynx_i386_collect_gp (I386_EDX_REGNUM, edx);
  lynx_i386_collect_gp (I386_EBX_REGNUM, ebx);
  lynx_i386_collect_gp (I386_ESP_REGNUM, esp);
  lynx_i386_collect_gp (I386_EBP_REGNUM, ebp);
  lynx_i386_collect_gp (I386_ESI_REGNUM, esi);
  lynx_i386_collect_gp (I386_EDI_REGNUM, edi);
  lynx_i386_collect_gp (I386_EIP_REGNUM, eip);
  lynx_i386_collect_gp (I386_EFLAGS_REGNUM, eflags);
  lynx_i386_collect_gp (I386_CS_REGNUM, cs);
  lynx_i386_collect_gp (I386_SS_REGNUM, ss);
  lynx_i386_collect_gp (I386_DS_REGNUM, ds);
  lynx_i386_collect_gp (I386_ES_REGNUM, es);
  lynx_i386_collect_gp (I386_FS_REGNUM, fs);
  lynx_i386_collect_gp (I386_GS_REGNUM, gs);
}

/* The store_function for the general-purpose register set.  */

static void
lynx_i386_store_gregset (struct regcache *regcache, const char *buf)
{
#define lynx_i386_supply_gp(regnum, fld) \
  supply_register (regcache, regnum, \
                   buf + offsetof (usr_econtext_t, uec_##fld))

  lynx_i386_supply_gp (I386_EAX_REGNUM, eax);
  lynx_i386_supply_gp (I386_ECX_REGNUM, ecx);
  lynx_i386_supply_gp (I386_EDX_REGNUM, edx);
  lynx_i386_supply_gp (I386_EBX_REGNUM, ebx);
  lynx_i386_supply_gp (I386_ESP_REGNUM, esp);
  lynx_i386_supply_gp (I386_EBP_REGNUM, ebp);
  lynx_i386_supply_gp (I386_ESI_REGNUM, esi);
  lynx_i386_supply_gp (I386_EDI_REGNUM, edi);
  lynx_i386_supply_gp (I386_EIP_REGNUM, eip);
  lynx_i386_supply_gp (I386_EFLAGS_REGNUM, eflags);
  lynx_i386_supply_gp (I386_CS_REGNUM, cs);
  lynx_i386_supply_gp (I386_SS_REGNUM, ss);
  lynx_i386_supply_gp (I386_DS_REGNUM, ds);
  lynx_i386_supply_gp (I386_ES_REGNUM, es);
  lynx_i386_supply_gp (I386_FS_REGNUM, fs);
  lynx_i386_supply_gp (I386_GS_REGNUM, gs);
}

/* Extract the first 16 bits of register REGNUM in the REGCACHE,
   and store these 2 bytes at DEST.

   This is useful to collect certain 16bit registers which are known
   by GDBserver as 32bit registers (such as the Control Register
   for instance).  */

static void
collect_16bit_register (struct regcache *regcache, int regnum, char *dest)
{
  gdb_byte word[4];

  collect_register (regcache, regnum, word);
  memcpy (dest, word, 2);
}

/* The fill_function for the floating-point register set.  */

static void
lynx_i386_fill_fpregset (struct regcache *regcache, char *buf)
{
  int i;

  /* Collect %st0 .. %st7.  */
  for (i = 0; i < 8; i++)
    collect_register (regcache, I386_ST0_REGNUM + i,
                      buf + offsetof (usr_fcontext_t, ufc_reg)
		      + i * sizeof (struct ufp387_real));

  /* Collect the other FPU registers.  */
  collect_16bit_register (regcache, I386_FCTRL_REGNUM,
                          buf + offsetof (usr_fcontext_t, ufc_control));
  collect_16bit_register (regcache, I386_FSTAT_REGNUM,
                          buf + offsetof (usr_fcontext_t, ufc_status));
  collect_16bit_register (regcache, I386_FTAG_REGNUM,
                          buf + offsetof (usr_fcontext_t, ufc_tag));
  collect_register (regcache, I386_FISEG_REGNUM,
                    buf + offsetof (usr_fcontext_t, ufc_inst_sel));
  collect_register (regcache, I386_FIOFF_REGNUM,
                    buf + offsetof (usr_fcontext_t, ufc_inst_off));
  collect_register (regcache, I386_FOSEG_REGNUM,
                    buf + offsetof (usr_fcontext_t, ufc_data_sel));
  collect_register (regcache, I386_FOOFF_REGNUM,
                    buf + offsetof (usr_fcontext_t, ufc_data_off));
  collect_16bit_register (regcache, I386_FOP_REGNUM,
                          buf + offsetof (usr_fcontext_t, ufc_opcode));

  /* Collect the XMM registers.  */
  for (i = 0; i < 8; i++)
    collect_register (regcache, I386_XMM0_REGNUM + i,
                      buf + offsetof (usr_fcontext_t, uxmm_reg)
		      + i * sizeof (struct uxmm_register));
  collect_register (regcache, I386_MXCSR_REGNUM,
                    buf + offsetof (usr_fcontext_t, usse_mxcsr));
}

/* This is the supply counterpart for collect_16bit_register:
   It extracts a 2byte value from BUF, and uses that value to
   set REGNUM's value in the regcache.

   This is useful to supply the value of certain 16bit registers
   which are known by GDBserver as 32bit registers (such as the Control
   Register for instance).  */

static void
supply_16bit_register (struct regcache *regcache, int regnum, const char *buf)
{
  gdb_byte word[4];

  memcpy (word, buf, 2);
  memset (word + 2, 0, 2);
  supply_register (regcache, regnum, word);
}

/* The store_function for the floating-point register set.  */

static void
lynx_i386_store_fpregset (struct regcache *regcache, const char *buf)
{
  int i;

  /* Store the %st0 .. %st7 registers.  */
  for (i = 0; i < 8; i++)
    supply_register (regcache, I386_ST0_REGNUM + i,
                     buf + offsetof (usr_fcontext_t, ufc_reg)
		     + i * sizeof (struct ufp387_real));

  /* Store the other FPU registers.  */
  supply_16bit_register (regcache, I386_FCTRL_REGNUM,
                         buf + offsetof (usr_fcontext_t, ufc_control));
  supply_16bit_register (regcache, I386_FSTAT_REGNUM,
                         buf + offsetof (usr_fcontext_t, ufc_status));
  supply_16bit_register (regcache, I386_FTAG_REGNUM,
                         buf + offsetof (usr_fcontext_t, ufc_tag));
  supply_register (regcache, I386_FISEG_REGNUM,
                   buf + offsetof (usr_fcontext_t, ufc_inst_sel));
  supply_register (regcache, I386_FIOFF_REGNUM,
                   buf + offsetof (usr_fcontext_t, ufc_inst_off));
  supply_register (regcache, I386_FOSEG_REGNUM,
                   buf + offsetof (usr_fcontext_t, ufc_data_sel));
  supply_register (regcache, I386_FOOFF_REGNUM,
                   buf + offsetof (usr_fcontext_t, ufc_data_off));
  supply_16bit_register (regcache, I386_FOP_REGNUM,
                         buf + offsetof (usr_fcontext_t, ufc_opcode));

  /* Store the XMM registers.  */
  for (i = 0; i < 8; i++)
    supply_register (regcache, I386_XMM0_REGNUM + i,
                     buf + offsetof (usr_fcontext_t, uxmm_reg)
		     + i * sizeof (struct uxmm_register));
  supply_register (regcache, I386_MXCSR_REGNUM,
                   buf + offsetof (usr_fcontext_t, usse_mxcsr));
}

/* Implements the lynx_target_ops.arch_setup routine.  */

static void
lynx_i386_arch_setup (void)
{
  init_registers_i386 ();
}

/* Description of all the x86-lynx register sets.  */

struct lynx_regset_info lynx_target_regsets[] = {
  /* General Purpose Registers.  */
  {PTRACE_GETREGS, PTRACE_SETREGS, sizeof(usr_econtext_t),
   lynx_i386_fill_gregset, lynx_i386_store_gregset},
  /* Floating Point Registers.  */
  { PTRACE_GETFPREGS, PTRACE_SETFPREGS, sizeof(usr_fcontext_t),
    lynx_i386_fill_fpregset, lynx_i386_store_fpregset },
  /* End of list marker.  */
  {0, 0, -1, NULL, NULL }
};

/* The lynx_target_ops vector for x86-lynx.  */

struct lynx_target_ops the_low_target = {
  lynx_i386_arch_setup,
};
