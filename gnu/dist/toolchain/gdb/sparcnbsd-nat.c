/* Functions specific to running gdb native on a SPARC running NetBSD.
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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "gdbcore.h"

#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <machine/reg.h>
#include <machine/pcb.h>

/* We don't store all registers immediately when requested, since they
   get sent over in large chunks anyway.  Instead, we accumulate most
   of the changes and send them over once.  "deferred_stores" keeps
   track of which sets of registers we have locally-changed copies of,
   so we only need send the groups that have changed.  */

#define	INT_REGS	1
#define	STACK_REGS	2
#define	FP_REGS		4

static void
supply_regs (regs)
     char *regs;
{
  CORE_ADDR sp;
  char regbuf[16 * 4];
  int i;

  /* %g0 is always 0.  */
  memset (regbuf, 0, sizeof (regbuf));
  supply_register (G0_REGNUM, regbuf);

  /* Global regs start 16 bytes into the buffer. */
  for (i = 1; i < 8; i++)
    supply_register (G0_REGNUM + i, regs + (16 + (i * 4)));

  /* Output registers start 48 bytes into the buffer. */
  for (i = 0; i < 8; i++)
    supply_register (O0_REGNUM + i, regs + (48 + (i * 4)));

  supply_register (PS_REGNUM,  regs + 0);
  supply_register (PC_REGNUM,  regs + 4);
  supply_register (NPC_REGNUM, regs + 8);
  supply_register (Y_REGNUM,   regs + 12);

  sp = *(CORE_ADDR *)&registers[REGISTER_BYTE (SP_REGNUM)];
  if (0 != target_read_memory (sp, regbuf,
			       16 * REGISTER_RAW_SIZE (L0_REGNUM)))
    {
      /* fprintf_unfiltered so user can still use gdb */
      fprintf_unfiltered (gdb_stderr,
	  "Couldn't read input and local registers from stack\n");
    }
  else
    {
      for (i = 0; i < 16; i++)
	supply_register (L0_REGNUM + i, &regbuf[i * 4]);
    }
}

static void
supply_fakeregs ()
{

    /* If we don't set these valid, read_register_bytes() rereads
       all the regs every time it is called!  FIXME.  */
    register_valid[WIM_REGNUM] = 1;	/* Not true yet, FIXME */
    register_valid[TBR_REGNUM] = 1;	/* Not true yet, FIXME */
    register_valid[CPS_REGNUM] = 1;	/* Not true yet, FIXME */
}

static void
supply_fpregs (fregs)
     char *fregs;
{
  int i;
  
  /* 32 floats */
  for (i = 0; i < 32; i++)
    supply_register (FP0_REGNUM + i, fregs + (i * 4));

  /* %fsr */
  supply_register (FPS_REGNUM, fregs + 128);
}

static void
fetch_core_registers PARAMS ((char *, unsigned int, int, CORE_ADDR));

/* Fetch one or more registers from the inferior.  REGNO == -1 to get
   them all.  We actually fetch more than requested, when convenient,
   marking them as valid so we won't fetch them again.  */

void
fetch_inferior_registers (regno)
     int regno;
{
  struct regs inferior_registers;
  struct fp_status inferior_fp_registers;
  int i;

  /* We should never be called with deferred stores, because a prerequisite
     for writing regs is to have fetched them all (PREPARE_TO_STORE), sigh.  */
  if (deferred_stores)
    abort ();

  DO_DEFERRED_STORES;

  /* Global and Out regs are fetched directly, as well as the control
     registers.  If we're getting one of the in or local regs,
     and the stack pointer has not yet been fetched,
     we have to do that first, since they're found in memory relative
     to the stack pointer.  */
  if (regno < O7_REGNUM		/* including -1 */
      || regno >= Y_REGNUM
      || (!register_valid[SP_REGNUM] && regno <= I7_REGNUM))
    {
      if (0 != ptrace (PTRACE_GETREGS, inferior_pid,
		       (PTRACE_ARG3_TYPE) & inferior_registers, 0))
	perror ("ptrace_getregs");

      supply_regs ((char *) &inferior_registers);
      supply_fakeregs ();
    }

  /* Floating point registers */
  if (regno == -1 ||
      regno == FPS_REGNUM ||
      (regno >= FP0_REGNUM && regno <= FP0_REGNUM + 31))
    {
      if (0 != ptrace (PTRACE_GETFPREGS, inferior_pid,
		       (PTRACE_ARG3_TYPE) & inferior_fp_registers, 0))
	perror ("ptrace_getfpregs");

      supply_fpregs ((char *) &inferior_fp_registers);
    }
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

void
store_inferior_registers (regno)
     int regno;
{
  struct regs inferior_registers;
  struct fp_status inferior_fp_registers;
  int wanna_store = INT_REGS + STACK_REGS + FP_REGS;

  /* First decide which pieces of machine-state we need to modify.  
     Default for regno == -1 case is all pieces.  */
  if (regno >= 0)
    if (FP0_REGNUM <= regno && regno < FP0_REGNUM + 32)
      {
	wanna_store = FP_REGS;
      }
    else
      {
	if (regno == SP_REGNUM)
	  wanna_store = INT_REGS + STACK_REGS;
	else if (regno < L0_REGNUM || regno > I7_REGNUM)
	  wanna_store = INT_REGS;
	else if (regno == FPS_REGNUM)
	  wanna_store = FP_REGS;
	else
	  wanna_store = STACK_REGS;
      }

  /* See if we're forcing the stores to happen now, or deferring. */
  if (regno == -2)
    {
      wanna_store = deferred_stores;
      deferred_stores = 0;
    }
  else
    {
      if (wanna_store == STACK_REGS)
	{
	  /* Fall through and just store one stack reg.  If we deferred
	     it, we'd have to store them all, or remember more info.  */
	}
      else
	{
	  deferred_stores |= wanna_store;
	  return;
	}
    }

  if (wanna_store & STACK_REGS)
    {
      CORE_ADDR sp = *(CORE_ADDR *) & registers[REGISTER_BYTE (SP_REGNUM)];

      if (regno < 0 || regno == SP_REGNUM)
	{
	  if (!register_valid[L0_REGNUM + 5])
	    abort ();
	  target_write_memory (sp,
			       &registers[REGISTER_BYTE (L0_REGNUM)],
			       16 * REGISTER_RAW_SIZE (L0_REGNUM));
	}
      else
	{
	  if (!register_valid[regno])
	    abort ();
	  target_write_memory (sp + REGISTER_BYTE (regno) - REGISTER_BYTE (L0_REGNUM),
			       &registers[REGISTER_BYTE (regno)],
			       REGISTER_RAW_SIZE (regno));
	}

    }

  if (wanna_store & INT_REGS)
    {
      if (!register_valid[G1_REGNUM])
	abort ();

      memcpy (&inferior_registers.r_g1, &registers[REGISTER_BYTE (G1_REGNUM)],
	      15 * REGISTER_RAW_SIZE (G1_REGNUM));

      inferior_registers.r_ps =
	*(int *) &registers[REGISTER_BYTE (PS_REGNUM)];
      inferior_registers.r_pc =
	*(int *) &registers[REGISTER_BYTE (PC_REGNUM)];
      inferior_registers.r_npc =
	*(int *) &registers[REGISTER_BYTE (NPC_REGNUM)];
      inferior_registers.r_y =
	*(int *) &registers[REGISTER_BYTE (Y_REGNUM)];

      if (0 != ptrace (PTRACE_SETREGS, inferior_pid,
		       (PTRACE_ARG3_TYPE) & inferior_registers, 0))
	perror ("ptrace_setregs");
    }

  if (wanna_store & FP_REGS)
    {
      if (!register_valid[FP0_REGNUM + 9])
	abort ();
      memcpy (&inferior_fp_registers, &registers[REGISTER_BYTE (FP0_REGNUM)],
	      sizeof inferior_fp_registers.fpu_fr);
      memcpy (&inferior_fp_registers.Fpu_fsr,
	      &registers[REGISTER_BYTE (FPS_REGNUM)], sizeof (FPU_FSR_TYPE));
      if (0 !=
	  ptrace (PTRACE_SETFPREGS, inferior_pid,
		  (PTRACE_ARG3_TYPE) & inferior_fp_registers, 0))
	perror ("ptrace_setfpregs");
    }
}

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;		/* reg addr, unused in this version */
{
  struct md_coredump *core = (struct md_coredump *) core_reg_sect;

  /* Integer registers. */
  supply_regs ((char *) &core->md_tf);

  /* Floating point registers. */
  supply_fpregs ((char *) &core->md_fpstate);
}

static void
fetch_elfcore_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  switch (which)
    {
    case 0:  /* Integer registers */
      if (core_reg_size != sizeof (struct reg))
	warning ("Wrong size register set in core file.");
      else
	supply_regs (core_reg_sect);
      break;

    case 2:  /* Floating point registers */
      if (core_reg_size != sizeof (struct fpreg))
	warning ("Wrong size FP register set in core file.");
      else
	supply_fpregs (core_reg_sect);
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it. */
      break;
    }
}


/* Register that we are able to handle sparc core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns sparcnbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

static struct core_fns sparcnbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,		/* core_read_registers */
  NULL					/* next */
};

void
_initialize_sparcnbsd_nat ()
{
  add_core_fns (&sparcnbsd_core_fns);
  add_core_fns (&sparcnbsd_elfcore_fns);
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
  struct rwindow win;
  int i;
  u_long sp;

  /* We only do integer registers */
  sp = pcb->pcb_sp;

  supply_register(SP_REGNUM, (char *)&pcb->pcb_sp);
  supply_register(PC_REGNUM, (char *)&pcb->pcb_pc);
  supply_register(O7_REGNUM, (char *)&pcb->pcb_pc);
  supply_register(PS_REGNUM, (char *)&pcb->pcb_psr);
  supply_register(WIM_REGNUM, (char *)&pcb->pcb_wim);
  /*
   * Read last register window saved on stack.
   */
  if (target_read_memory(sp, (char *)&win, sizeof win)) {
    printf("cannot read register window at sp=%x\n", pcb->pcb_sp);
    bzero((char *)&win, sizeof win);
  }
  for (i = 0; i < sizeof(win.rw_local); ++i)
    supply_register(i + L0_REGNUM, (char *)&win.rw_local[i]);
  for (i = 0; i < sizeof(win.rw_in); ++i)
    supply_register(i + I0_REGNUM, (char *)&win.rw_in[i]);
  /*
   * read the globals & outs saved on the stack (for a trap frame).
   */
  sp += 92 + 12; /* XXX - MINFRAME + R_Y */
  for (i = 1; i < 14; ++i) {
    u_long val;
    
    if (target_read_memory(sp + i*4, (char *)&val, sizeof val) == 0)
      supply_register(i, (char *)&val);
  }
#if 0
  if (kvread(pcb.pcb_cpctxp, &cps) == 0)
    supply_register(CPS_REGNUM, (char *)&cps);
#endif

  /* The kernel does not use the FPU, so ignore it. */
  registers_fetched ();
}
#endif	/* FETCH_KCORE_REGISTERS */
