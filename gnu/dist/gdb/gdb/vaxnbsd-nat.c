/* Functions specific to running gdb native on a VAX running NetBSD
   Copyright 1989, 1992, 1993, 1994, 1996 Free Software Foundation, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "defs.h"

#ifndef FETCH_INFERIOR_REGISTERS
#error Not FETCH_INFERIOR_REGISTERS
#endif /* FETCH_INFERIOR_REGISTERS */

#include "vax-tdep.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>

#include "inferior.h"
#include "regcache.h"
#include "gdbcore.h"

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpset fpregset_t;
#endif

#include "gregset.h"

void
supply_gregset (gregset_t *gregset)
{
  int regnum;

  for (regnum = 0; regnum <= VAX_PS_REGNUM; regnum++)
    supply_register (regnum, (char *) (&gregset->r0 + regnum));
}

void
supply_fpregset (fpregset_t *fpregset)
{
}

void
fill_gregset (gregset_t *gregset, int regno)
{
  if (-1 == regno)
    {
      for (regno = 0; regno <= VAX_PS_REGNUM; regno++)
        regcache_collect (regno, (char *) (&gregset->r0 + regno));
    }
   else if (regno <= VAX_PS_REGNUM)
     regcache_collect (regno, (char *) (&gregset->r0 + regno));
}

void
fill_fpregset(fpregset_t *fpregset, int regno)
{
}

void
fetch_inferior_registers (int regno)
{
  gregset_t inferior_registers;
  int ret;

  ret = ptrace (PT_GETREGS, PIDGET (inferior_ptid),
                (PTRACE_ARG3_TYPE) &inferior_registers, TIDGET (inferior_ptid));
    
  if (ret < 0)
    {
      warning ("unable to fetch general register");
      return;
    } 

  if (regno >= 0)
    {
      if (regno > VAX_PS_REGNUM)
	{
	  warning ("unable to fetch general register %d", regno);
	  return;
	}
      supply_register (regno, (char *) (&inferior_registers.r0 + regno));
    }
  else
    supply_gregset (&inferior_registers);
}

void
store_inferior_registers (int regno)
{
  gregset_t inferior_registers;
  int ret;

  if (regno >= 0)
    {
      ret = ptrace (PT_GETREGS, PIDGET (inferior_ptid),
                    (PTRACE_ARG3_TYPE) &inferior_registers,
		    TIDGET (inferior_ptid));
      if (ret < 0)
	{
           warning ("unable to fetch general registers");
           return;
        }

      if (regno > VAX_PS_REGNUM)
        {
	  warning ("unable to store general register %d", regno);
	  return;
        }
      regcache_collect (regno, (char *) (&inferior_registers.r0 + regno));
    }
  else
    {
      for (regno = 0; regno <= VAX_PS_REGNUM; regno++)
        regcache_collect (regno, (char *) (&inferior_registers.r0 + regno));
    }

  ret = ptrace (PT_SETREGS, PIDGET (inferior_ptid),
                (PTRACE_ARG3_TYPE) &inferior_registers, TIDGET (inferior_ptid));
  
  if (ret < 0)
    warning ("unable to store general registers");
}


/* XXX - Add this to machine/regs.h instead? */
struct md_core {
  struct trapframe intreg;
  /* struct fpreg freg; XXX */
};

static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size, int which,
		      CORE_ADDR ignore)
{
  struct md_core *core_reg;
  gregset_t gregset;

  core_reg = (struct md_core *)core_reg_sect;

  /* We get everything from the .reg section. */
  if (which != 0)
    return;

  if (core_reg_size < sizeof(*core_reg)) {
    fprintf_unfiltered (gdb_stderr, "Couldn't read regs from core file\n");
    return;
  }

  /* Integer registers */
  memcpy(&gregset.r0, &core_reg->intreg.r0, sizeof(gregset.r0)*VAX_AP_REGNUM);
  gregset.ap = core_reg->intreg.ap;
  gregset.fp = core_reg->intreg.fp;
  gregset.sp = core_reg->intreg.sp;
  gregset.pc = core_reg->intreg.pc;
  gregset.psl = core_reg->intreg.psl;

  supply_gregset (&gregset);
}

/* Register that we are able to handle vaxnbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns vaxnbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

void
_initialize_vaxnbsd_nat ()
{
  add_core_fns (&vaxnbsd_core_fns);
}


/*
 * kernel_u_size() is not helpful on NetBSD because
 * the "u" struct is NOT in the core dump file.
 */

#ifdef	FETCH_KCORE_REGISTERS
/*
 * Get registers from a kernel crash dump or live kernel.
 * Called by kcore-nbsd.c:get_kcore_registers().
 */
void
fetch_kcore_registers (pcb)
     struct pcb *pcb;
{

  printf("XXX: fetch_kcore_registers\n");

  /* The kernel does not use the FPU, so ignore it. */
  registers_fetched ();
}
#endif	/* FETCH_KCORE_REGISTERS */
