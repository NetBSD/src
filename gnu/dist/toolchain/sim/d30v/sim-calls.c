/*  This file is part of the program psim.

    Copyright (C) 1994-1996, Andrew Cagney <cagney@highland.com.au>
    Copyright (C) 1997, Free Software Foundation

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
    */


#include <stdarg.h>
#include <ctype.h>

#include "sim-main.h"
#include "sim-options.h"

#include "bfd.h"
#include "sim-utils.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

static unsigned long extmem_size = 1024*1024*8;	/* 8 meg is the maximum listed in the arch. manual */

static const char * get_insn_name (sim_cpu *, int);

#define SIM_ADDR unsigned


#define OPTION_TRACE_CALL	200
#define OPTION_TRACE_TRAPDUMP	201
#define OPTION_EXTMEM_SIZE	202

static SIM_RC
d30v_option_handler (SIM_DESC sd,
		     sim_cpu *cpu,
		     int opt,
		     char *arg,
		     int command_p)
{
  char *suffix;

  switch (opt)
    {
    default:
      break;

    case OPTION_TRACE_CALL:
      if (arg == NULL || strcmp (arg, "yes") == 0 || strcmp (arg, "on") == 0)
	TRACE_CALL_P = 1;
      else if (strcmp (arg, "no") == 0 || strcmp (arg, "off") == 0)
	TRACE_CALL_P = 0;
      else
	{
	  sim_io_eprintf (sd, "Unreconized --trace-call option `%s'\n", arg);
	  return SIM_RC_FAIL;
	}
      return SIM_RC_OK;

    case OPTION_TRACE_TRAPDUMP:
      if (arg == NULL || strcmp (arg, "yes") == 0 || strcmp (arg, "on") == 0)
	TRACE_TRAP_P = 1;
      else if (strcmp (arg, "no") == 0 || strcmp (arg, "off") == 0)
	TRACE_TRAP_P = 0;
      else
	{
	  sim_io_eprintf (sd, "Unreconized --trace-call option `%s'\n", arg);
	  return SIM_RC_FAIL;
	}
      return SIM_RC_OK;

    case OPTION_EXTMEM_SIZE:
      if (arg == NULL || !isdigit (*arg))
	{
	  sim_io_eprintf (sd, "Invalid memory size `%s'", arg);
	  return SIM_RC_FAIL;
	}

      suffix = arg;
      extmem_size = strtol (arg, &suffix, 0);
      if (*suffix == 'm' || *suffix == 'M')
	extmem_size <<= 20;
      else if (*suffix == 'k' || *suffix == 'K')
	extmem_size <<= 10;
      sim_do_commandf (sd, "memory delete 0x80000000");
      sim_do_commandf (sd, "memory region 0x80000000,0x%lx", extmem_size);

      return SIM_RC_OK;
    }

  sim_io_eprintf (sd, "Unknown option (%d)\n", opt);
  return SIM_RC_FAIL;
}

static const OPTION d30v_options[] =
{
  { {"trace-call", optional_argument, NULL, OPTION_TRACE_CALL},
      '\0', "on|off", "Enable tracing of calls and returns, checking saved registers",
      d30v_option_handler },
  { {"trace-trapdump", optional_argument, NULL, OPTION_TRACE_TRAPDUMP},
      '\0', "on|off",
#if TRAPDUMP
    "Traps 0..30 dump out all of the registers (defaults on)",
#else
    "Traps 0..30 dump out all of the registers",
#endif
      d30v_option_handler },
  { {"extmem-size", required_argument, NULL, OPTION_EXTMEM_SIZE},
    '\0', "size", "Change size of external memory, default 8 meg",
    d30v_option_handler },
  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL }
};

/* Return name of an insn, used by insn profiling.  */

static const char *
get_insn_name (sim_cpu *cpu, int i)
{
  return itable[i].name;
}

/* Structures used by the simulator, for gdb just have static structures */

SIM_DESC
sim_open (SIM_OPEN_KIND kind,
	  host_callback *callback,
	  struct _bfd *abfd,
	  char **argv)
{
  SIM_DESC sd = sim_state_alloc (kind, callback);

  /* FIXME: watchpoints code shouldn't need this */
  STATE_WATCHPOINTS (sd)->pc = &(PC);
  STATE_WATCHPOINTS (sd)->sizeof_pc = sizeof (PC);
  STATE_WATCHPOINTS (sd)->interrupt_handler = d30v_interrupt_event;

  /* Initialize the mechanism for doing insn profiling.  */
  CPU_INSN_NAME (STATE_CPU (sd, 0)) = get_insn_name;
  CPU_MAX_INSNS (STATE_CPU (sd, 0)) = nr_itable_entries;

#ifdef TRAPDUMP
  TRACE_TRAP_P = TRAPDUMP;
#endif

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    return 0;
  sim_add_option_table (sd, NULL, d30v_options);

  /* Memory and EEPROM */
  /* internal instruction RAM - fixed */
  sim_do_commandf (sd, "memory region 0,0x10000");
  /* internal data RAM - fixed */
  sim_do_commandf (sd, "memory region 0x20000000,0x8000");
  /* control register dummy area */
  sim_do_commandf (sd, "memory region 0x40000000,0x10000");
  /* external RAM */
  sim_do_commandf (sd, "memory region 0x80000000,0x%lx", extmem_size);
  /* EIT RAM */
  sim_do_commandf (sd, "memory region 0xfffff000,0x1000");

  /* getopt will print the error message so we just have to exit if this fails.
     FIXME: Hmmm...  in the case of gdb we need getopt to call
     print_filtered.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  /* check for/establish the a reference program image */
  if (sim_analyze_program (sd,
			   (STATE_PROG_ARGV (sd) != NULL
			    ? *STATE_PROG_ARGV (sd)
			    : NULL),
			   abfd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  /* establish any remaining configuration options */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  return sd;
}


void
sim_close (SIM_DESC sd, int quitting)
{
  /* Uninstall the modules to avoid memory leaks,
     file descriptor leaks, etc.  */
  sim_module_uninstall (sd);
}


SIM_RC
sim_create_inferior (SIM_DESC sd,
		     struct _bfd *abfd,
		     char **argv,
		     char **envp)
{
  /* clear all registers */
  memset (&STATE_CPU (sd, 0)->regs, 0, sizeof (STATE_CPU (sd, 0)->regs));
  EIT_VB = EIT_VB_DEFAULT;
  STATE_CPU (sd, 0)->unit = any_unit;
  sim_module_init (sd);
  if (abfd != NULL)
    PC = bfd_get_start_address (abfd);
  else
    PC = 0xfffff000; /* reset value */
  return SIM_RC_OK;
}

void
sim_do_command (SIM_DESC sd, char *cmd)
{
  if (sim_args_command (sd, cmd) != SIM_RC_OK)
    sim_io_printf (sd, "Unknown command `%s'\n", cmd);
}

/* The following register definitions were ripped off from
   gdb/config/tm-d30v.h.  If any of those defs changes, this table needs to
   be updated.  */

#define NUM_REGS 86

#define R0_REGNUM	0
#define FP_REGNUM	11
#define LR_REGNUM 	62
#define SP_REGNUM 	63
#define SPI_REGNUM	64	/* Interrupt stack pointer */
#define SPU_REGNUM	65	/* User stack pointer */
#define CREGS_START	66

#define PSW_REGNUM 	(CREGS_START + 0) /* psw, bpsw, or dpsw??? */
#define    PSW_SM 0x80000000	/* Stack mode: 0 == interrupt (SPI),
					       1 == user (SPU) */
#define BPSW_REGNUM	(CREGS_START + 1) /* Backup PSW (on interrupt) */
#define PC_REGNUM 	(CREGS_START + 2) /* pc, bpc, or dpc??? */
#define BPC_REGNUM 	(CREGS_START + 3) /* Backup PC (on interrupt) */
#define DPSW_REGNUM	(CREGS_START + 4) /* Backup PSW (on debug trap) */
#define DPC_REGNUM 	(CREGS_START + 5) /* Backup PC (on debug trap) */
#define RPT_C_REGNUM	(CREGS_START + 7) /* Loop count */
#define RPT_S_REGNUM	(CREGS_START + 8) /* Loop start address*/
#define RPT_E_REGNUM	(CREGS_START + 9) /* Loop end address */
#define MOD_S_REGNUM	(CREGS_START + 10)
#define MOD_E_REGNUM	(CREGS_START + 11)
#define IBA_REGNUM	(CREGS_START + 14) /* Instruction break address */
#define EIT_VB_REGNUM	(CREGS_START + 15) /* Vector base address */
#define INT_S_REGNUM	(CREGS_START + 16) /* Interrupt status */
#define INT_M_REGNUM	(CREGS_START + 17) /* Interrupt mask */
#define A0_REGNUM 	84
#define A1_REGNUM 	85

int
sim_fetch_register (sd, regno, buf, length)
     SIM_DESC sd;
     int regno;
     unsigned char *buf;
     int length;
{
  if (regno < A0_REGNUM)
    {
      unsigned32 reg;

      if (regno <= R0_REGNUM + 63)
	reg = sd->cpu[0].regs.general_purpose[regno];
      else if (regno <= SPU_REGNUM)
	reg = sd->cpu[0].regs.sp[regno - SPI_REGNUM];
      else
	reg = sd->cpu[0].regs.control[regno - CREGS_START];

      buf[0] = reg >> 24;
      buf[1] = reg >> 16;
      buf[2] = reg >> 8;
      buf[3] = reg;
    }
  else if (regno < NUM_REGS)
    {
      unsigned32 reg;

      reg = sd->cpu[0].regs.accumulator[regno - A0_REGNUM] >> 32;

      buf[0] = reg >> 24;
      buf[1] = reg >> 16;
      buf[2] = reg >> 8;
      buf[3] = reg;

      reg = sd->cpu[0].regs.accumulator[regno - A0_REGNUM];

      buf[4] = reg >> 24;
      buf[5] = reg >> 16;
      buf[6] = reg >> 8;
      buf[7] = reg;
    }
  else
    abort ();
  return -1;
}

int
sim_store_register (sd, regno, buf, length)
     SIM_DESC sd;
     int regno;
     unsigned char *buf;
     int length;
{
  if (regno < A0_REGNUM)
    {
      unsigned32 reg;

      reg = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

      if (regno <= R0_REGNUM + 63)
	sd->cpu[0].regs.general_purpose[regno] = reg;
      else if (regno <= SPU_REGNUM)
	sd->cpu[0].regs.sp[regno - SPI_REGNUM] = reg;
      else
	sd->cpu[0].regs.control[regno - CREGS_START] = reg;
    }
  else if (regno < NUM_REGS)
    {
      unsigned32 reg;

      reg = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

      sd->cpu[0].regs.accumulator[regno - A0_REGNUM] = (unsigned64)reg << 32;

      reg = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];

      sd->cpu[0].regs.accumulator[regno - A0_REGNUM] |= reg;
    }
  else
    abort ();
  return -1;
}
