/* collection of junk waiting time to sort out
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of the GNU Simulators.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef M32R_SIM_H
#define M32R_SIM_H

/* gdb register numbers */
#define PSW_REGNUM	16
#define CBR_REGNUM	17
#define SPI_REGNUM	18
#define SPU_REGNUM	19
#define BPC_REGNUM	20
#define PC_REGNUM	21
#define ACCL_REGNUM	22
#define ACCH_REGNUM	23
#define ACC1L_REGNUM	24
#define ACC1H_REGNUM	25
#define BBPSW_REGNUM	26
#define BBPC_REGNUM	27

extern int m32r_decode_gdb_ctrl_regnum (int);

/* Cover macros for hardware accesses.
   FIXME: Eventually move to cgen.  */
#define GET_H_SM() ((CPU (h_psw) & 0x80) != 0)

extern SI a_m32r_h_gr_get (SIM_CPU *, UINT);
extern void a_m32r_h_gr_set (SIM_CPU *, UINT, SI);
extern USI a_m32r_h_cr_get (SIM_CPU *, UINT);
extern void a_m32r_h_cr_set (SIM_CPU *, UINT, USI);

extern USI m32rbf_h_cr_get_handler (SIM_CPU *, UINT);
extern void m32rbf_h_cr_set_handler (SIM_CPU *, UINT, USI);

extern UQI m32rbf_h_psw_get_handler (SIM_CPU *);
extern void m32rbf_h_psw_set_handler (SIM_CPU *, UQI);

extern DI m32rbf_h_accum_get_handler (SIM_CPU *);
extern void m32rbf_h_accum_set_handler (SIM_CPU *, DI);

extern USI m32rxf_h_cr_get_handler (SIM_CPU *, UINT);
extern void m32rxf_h_cr_set_handler (SIM_CPU *, UINT, USI);
extern UQI m32rxf_h_psw_get_handler (SIM_CPU *);
extern void m32rxf_h_psw_set_handler (SIM_CPU *, UQI);
extern DI m32rxf_h_accum_get_handler (SIM_CPU *);
extern void m32rxf_h_accum_set_handler (SIM_CPU *, DI);

extern DI m32rxf_h_accums_get_handler (SIM_CPU *, UINT);
extern void m32rxf_h_accums_set_handler (SIM_CPU *, UINT, DI);

/* Misc. profile data.  */

typedef struct {
  /* nop insn slot filler count */
  unsigned int fillnop_count;
  /* number of parallel insns */
  unsigned int parallel_count;

  /* FIXME: generalize this to handle all insn lengths, move to common.  */
  /* number of short insns, not including parallel ones */
  unsigned int short_count;
  /* number of long insns */
  unsigned int long_count;

  /* Working area for computing cycle counts.  */
  unsigned long insn_cycles; /* FIXME: delete */
  unsigned long cti_stall;
  unsigned long load_stall;
  unsigned long biggest_cycles;

  /* Bitmask of registers loaded by previous insn.  */
  unsigned int load_regs;
  /* Bitmask of registers loaded by current insn.  */
  unsigned int load_regs_pending;
} M32R_MISC_PROFILE;

/* Initialize the working area.  */
void m32r_init_insn_cycles (SIM_CPU *, int);
/* Update the totals for the insn.  */
void m32r_record_insn_cycles (SIM_CPU *, int);

/* This is invoked by the nop pattern in the .cpu file.  */
#define PROFILE_COUNT_FILLNOPS(cpu, addr) \
do { \
  if (PROFILE_INSN_P (cpu) \
      && (addr & 3) != 0) \
    ++ CPU_M32R_MISC_PROFILE (cpu)->fillnop_count; \
} while (0)

/* This is invoked by the execute section of mloop{,x}.in.  */
#define PROFILE_COUNT_PARINSNS(cpu) \
do { \
  if (PROFILE_INSN_P (cpu)) \
    ++ CPU_M32R_MISC_PROFILE (cpu)->parallel_count; \
} while (0)

/* This is invoked by the execute section of mloop{,x}.in.  */
#define PROFILE_COUNT_SHORTINSNS(cpu) \
do { \
  if (PROFILE_INSN_P (cpu)) \
    ++ CPU_M32R_MISC_PROFILE (cpu)->short_count; \
} while (0)

/* This is invoked by the execute section of mloop{,x}.in.  */
#define PROFILE_COUNT_LONGINSNS(cpu) \
do { \
  if (PROFILE_INSN_P (cpu)) \
    ++ CPU_M32R_MISC_PROFILE (cpu)->long_count; \
} while (0)

#define GETTWI GETTSI
#define SETTWI SETTSI

/* Additional execution support.  */

/* Result of semantic function is one of
   - next address, branch only
   - NEW_PC_SKIP, sc/snc insn
   - NEW_PC_2, 2 byte non-branch non-sc/snc insn
   - NEW_PC_4, 4 byte non-branch insn
   The special values have bit 1 set so it's cheap to distinguish them.
   This works because all cti's are defined to zero the bottom two bits
   Note that the m32rx no longer doesn't implement its semantics with
   functions, so this isn't used.  It's kept around should it be needed
   again.  */
/* FIXME: replace 0xffff0001 with 1?  */
#define NEW_PC_BASE 0xffff0001
#define NEW_PC_SKIP NEW_PC_BASE
#define NEW_PC_2 (NEW_PC_BASE + 2)
#define NEW_PC_4 (NEW_PC_BASE + 4)
#define NEW_PC_BRANCH_P(addr) (((addr) & 1) == 0)

/* Modify "next pc" support to handle parallel execution.
   This is for the non-pbb case.  The m32rx no longer implements this.
   It's kept around should it be needed again.  */
#if defined (WANT_CPU_M32RXF) && ! WITH_SCACHE_PBB_M32RXF
#undef SEM_NEXT_VPC
#define SEM_NEXT_VPC(abuf, len) (NEW_PC_BASE + (len))
#undef SEM_SKIP_INSN
#define SEM_SKIP_INSN(cpu, sc, vpcvar, yes) FIXME
#endif

/* Hardware/device support.
   ??? Will eventually want to move device stuff to config files.  */

/* Exception, Interrupt, and Trap addresses */
#define EIT_SYSBREAK_ADDR	0x10
#define EIT_RSVD_INSN_ADDR	0x20
#define EIT_ADDR_EXCP_ADDR	0x30
#define EIT_TRAP_BASE_ADDR	0x40
#define EIT_EXTERN_ADDR		0x80
#define EIT_RESET_ADDR		0x7ffffff0
#define EIT_WAKEUP_ADDR		0x7ffffff0

/* Special purpose traps.  */
#define TRAP_SYSCALL	0
#define TRAP_BREAKPOINT	1

/* Support for the MSPR register (Cache Purge Control Register)
   and the MCCR register (Cache Control Register) are needed in order for
   overlays to work correctly with the scache.
   MSPR no longer exists but is supported for upward compatibility with
   early overlay support.  */

/* Cache Purge Control (only exists on early versions of chips) */
#define MSPR_ADDR 0xfffffff7
#define MSPR_PURGE 1

/* Lock Control Register (not supported) */
#define MLCR_ADDR 0xfffffff7
#define MLCR_LM 1

/* Power Management Control Register (not supported) */
#define MPMR_ADDR 0xfffffffb

/* Cache Control Register */
#define MCCR_ADDR 0xffffffff
#define MCCR_CP 0x80
/* not supported */
#define MCCR_CM0 2
#define MCCR_CM1 1

/* Serial device addresses.  */
#ifdef M32R_EVA /* orig eva board, no longer supported */
#define UART_INCHAR_ADDR	0xff102013
#define UART_OUTCHAR_ADDR	0xff10200f
#define UART_STATUS_ADDR	0xff102006
/* Indicate ready bit is inverted.  */
#define UART_INPUT_READY0
#else
/* These are the values for the MSA2000 board.
   ??? Will eventually need to move this to a config file.  */
#define UART_INCHAR_ADDR	0xff004009
#define UART_OUTCHAR_ADDR	0xff004007
#define UART_STATUS_ADDR	0xff004002
#endif

#define UART_INPUT_READY	0x4
#define UART_OUTPUT_READY	0x1

/* Start address and length of all device support.  */
#define M32R_DEVICE_ADDR	0xff000000
#define M32R_DEVICE_LEN		0x01000000

/* sim_core_attach device argument.  */
extern device m32r_devices;

/* FIXME: Temporary, until device support ready.  */
struct _device { int foo; };

/* Handle the trap insn.  */
USI m32r_trap (SIM_CPU *, PCADDR, int);

#endif /* M32R_SIM_H */
