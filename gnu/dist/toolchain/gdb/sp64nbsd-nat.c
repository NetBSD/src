/* Functions specific to running gdb native on a UltraSPARC running NetBSD
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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <string.h>

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "gdbcore.h"

static void
supply_regs64 (regs)
     char *regs;
{
  uint64_t tstate;
  CORE_ADDR sp;
  char regbuf[16 * 8];
  int i;

  /* %g0 is always 0.  */
  memset (regbuf, 0, sizeof (regbuf));
  supply_register (G0_REGNUM, regbuf);

  /* Global regs start 32 bytes into the buffer. */
  for (i = 1; i < 8; i++)
    supply_register (G0_REGNUM + i, regs + (32 + (i * 8)));

  /* Output registers start 96 bytes into the buffer. */
  for (i = 0; i < 8; i++)
    supply_register (O0_REGNUM + i, regs + (96 + (i * 8)));

  supply_register (TSTATE_REGNUM, regs + 0);
  supply_register (PC_REGNUM, regs + 8);
  supply_register (NPC_REGNUM, regs + 16);

  /* %y is 32 bits in the reg structure, but GDB treats it as 64 bits.
     Compensate by copying it into the correct half of a temporary
     buffer and providing THAT to the register cache.  */
  memcpy (&regbuf[4], regs + 24, 4);
  supply_register (Y_REGNUM, regbuf);

  /* Decompose tstate into its constituent parts.  */
  memcpy (&tstate, regs + 0, sizeof (tstate));

  *(uint64_t *)regbuf = tstate & TSTATE_CWP;
  supply_register (CWP_REGNUM, regbuf);

  *(uint64_t *)regbuf = (tstate & TSTATE_ASI) >> TSTATE_ASI_SHIFT;
  supply_register (ASI_REGNUM, regbuf);

  *(uint64_t *)regbuf = (tstate & TSTATE_PSTATE) >> TSTATE_PSTATE_SHIFT;
  supply_register (PSTATE_REGNUM, regbuf);

  *(uint64_t *)regbuf = (tstate & TSTATE_CCR) >> TSTATE_CCR_SHIFT;
  supply_register (CCR_REGNUM, regbuf);

  sp = *(CORE_ADDR *)&registers[REGISTER_BYTE (SP_REGNUM)];
  if (sp & 0x1)
    {
      sp += BIAS;
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
	    supply_register (L0_REGNUM + i, &regbuf[i * 8]);
	}
    }
  else
    {
      char tmp[16 * 4];
      sp &= 0x0ffffffff;
      if (0 != target_read_memory (sp, tmp, 16 * 4))
        {
          /* fprintf_unfiltered so user can still use gdb */
          fprintf_unfiltered (gdb_stderr,
              "Couldn't read input and local registers from stack\n");
	}
      else
        {
          memset (regbuf, 0, sizeof (regbuf));
	  for (i = 0; i < 16; i++)
            {
              memcpy (&regbuf[4], &tmp[i * 4], 4);
	      supply_register (L0_REGNUM + i, regbuf);
	    }
	}
    }
}

static void
supply_fakeregs ()
{
  int i;

  /* If we don't set these valid, read_register_bytes()
     thinks we can't store to these regs and calling functions
     does not work. */
  for (i = TPC_REGNUM; i < NUM_REGS; i++)
    register_valid[i] = 1;	/* Not true yet, FIXME */
}

static void
supply_fpregs64 (fregs)
     char *fregs;
{
  int i;

  /* 32 floats */
  for (i = 0; i < 32; i++)
    supply_register (FP0_REGNUM + i, fregs + (i * 4));

  /* 16 doubles */
  for (; i < 48; i++)
    supply_register (FP0_REGNUM + i, fregs + (128 + (i * 8)));

  /* %fsr */
  supply_register (FSR_REGNUM, fregs + 256);
}

static void
unsupply_regs64 (regs)
     struct reg *regs;
{
}

static void
unsupply_fpregs64 (regs)
     struct reg *regs;
{
}

void
nbsd_reg_to_internal (regs)
     char *regs;
{
  supply_regs64 (regs);
}

void
nbsd_fpreg_to_internal (fregs)
     char *fregs;
{
  supply_fpregs64 (fregs);
}

void
nbsd_internal_to_reg (regs)
     char *regs;
{
  unsupply_regs64 (regs);
}

void
nbsd_internal_to_fpreg (fregs)
     char *fregs;
{
  unsupply_fpregs64 (fregs);
}

/* We don't store all registers immediately when requested, since they
   get sent over in large chunks anyway.  Instead, we accumulate most
   of the changes and send them over once.  "deferred_stores" keeps
   track of which sets of registers we have locally-changed copies of,
   so we only need send the groups that have changed.  */

#define	INT_REGS	1
#define	STACK_REGS	2
#define	FP_REGS		4

/* Fetch one or more registers from the inferior.  REGNO == -1 to get
   them all.  We actually fetch more than requested, when convenient,
   marking them as valid so we won't fetch them again.  */

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg64 inferior_registers;
  struct fpreg64 inferior_fp_registers;
  int i;

  /* We should never be called with deferred stores, because a prerequisite
     for writing regs is to have fetched them all (PREPARE_TO_STORE), sigh.  */
  if (deferred_stores) abort();

  DO_DEFERRED_STORES;

  /* Global and Out regs are fetched directly, as well as the control
     registers.  If we're getting one of the in or local regs,
     and the stack pointer has not yet been fetched,
     we have to do that first, since they're found in memory relative
     to the stack pointer.  */
  if (regno < O7_REGNUM  /* including -1 */
      || regno >= PC_REGNUM
      || (!register_valid[SP_REGNUM] && regno <= I7_REGNUM))
    {
      if (0 != ptrace (PT_GETREGS, inferior_pid,
		       (PTRACE_ARG3_TYPE) &inferior_registers, 0))
	perror("ptrace_getregs");

      supply_regs64 ((char *) &inferior_registers);
      supply_fakeregs ();
    }

  /* Floating point registers */
  if (regno == -1 || regno == FSR_REGNUM ||
      (regno >= FP0_REGNUM && regno <= FP0_REGNUM + 63))
    {
      if (0 != ptrace (PT_GETFPREGS, inferior_pid,
		       (PTRACE_ARG3_TYPE) &inferior_fp_registers, 0))
	perror("ptrace_getfpregs");
        
      supply_fpregs64 ((char *) &inferior_fp_registers);
    }
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

void
store_inferior_registers (regno)
     int regno;
{
  struct reg64 inferior_registers;
  struct fpreg64 inferior_fp_registers;
  int wanna_store = INT_REGS + STACK_REGS + FP_REGS;
  long save_g0;

  /* First decide which pieces of machine-state we need to modify.  
     Default for regno == -1 case is all pieces.  */
  if (regno >= 0)
    if (FP0_REGNUM <= regno && regno < FP0_REGNUM + 63)
      {
	wanna_store = FP_REGS;
      }
    else 
      {
	if (regno == SP_REGNUM)
	  wanna_store = INT_REGS + STACK_REGS;
	else if (regno < L0_REGNUM || regno > I7_REGNUM)
	  wanna_store = INT_REGS;
	else if (regno == FSR_REGNUM)
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
      CORE_ADDR sp = *(CORE_ADDR *)&registers[REGISTER_BYTE (SP_REGNUM)];

      if (regno < 0 || regno == SP_REGNUM)
	{
	  if (!register_valid[L0_REGNUM+5]) abort();
	  if (sp & 0x1)
	    {
	      sp += BIAS;
	      target_write_memory (sp, &registers[REGISTER_BYTE (L0_REGNUM)],
	                           16 * REGISTER_RAW_SIZE (L0_REGNUM));
	    }
	  else
	    {
	      int i, tmp[16];

	      sp &= 0x0ffffffffL;
	      for (i = L0_REGNUM; i <= I7_REGNUM; i++)
	        tmp[i] = *(long *) &registers[REGISTER_BYTE (i)];
	      target_write_memory (sp, (void *)&tmp, sizeof(tmp));
	    }
	}
      else
	{
	  if (!register_valid[regno]) abort();
	  if (sp & 0x1)
	    {
	      sp += BIAS;
	      target_write_memory ((sp + REGISTER_BYTE (regno) -
	                            REGISTER_BYTE (L0_REGNUM)),
	                           &registers[REGISTER_BYTE (regno)],
	                           REGISTER_RAW_SIZE (regno));
	    }
	  else
	    {
	      int tmp;

	      sp &= 0x0ffffffffL;
	      tmp = *(long *)&registers[REGISTER_BYTE (regno)];
	      target_write_memory (sp + sizeof(tmp) * (regno - L0_REGNUM),
	                           (void *)&tmp, sizeof(tmp));
	    }
	}
	
    }

  if (wanna_store & INT_REGS)
    {
      if (!register_valid[G1_REGNUM]) abort();

      /* The G0 slot really holds %tt (leave it alone). */
      save_g0 = inferior_registers.r_global[0];
      memcpy (&inferior_registers.r_global[0],
	      &registers[REGISTER_BYTE (G0_REGNUM)],
	      sizeof(inferior_registers.r_global));
      inferior_registers.r_global[0] = save_g0;
      memcpy (&inferior_registers.r_out[0],
	      &registers[REGISTER_BYTE (O0_REGNUM)],
	      sizeof(inferior_registers.r_out));

      inferior_registers.r_tstate =
	*(long *)&registers[REGISTER_BYTE (TSTATE_REGNUM)];
      inferior_registers.r_pc =
	*(long *)&registers[REGISTER_BYTE (PC_REGNUM)];
      inferior_registers.r_npc =
	*(long *)&registers[REGISTER_BYTE (NPC_REGNUM)];
      inferior_registers.r_y =
	*(int *)&registers[REGISTER_BYTE (Y_REGNUM)];

      if (0 != ptrace (PT_SETREGS, inferior_pid,
		       (PTRACE_ARG3_TYPE) &inferior_registers, 0))
	perror("ptrace_setregs");
    }

  if (wanna_store & FP_REGS)
    {
      if (!register_valid[FP0_REGNUM+9]) abort();
      memcpy (&inferior_fp_registers.fr_regs[0],
	      &registers[REGISTER_BYTE (FP0_REGNUM)],
	      sizeof(inferior_fp_registers.fr_regs));
      memcpy (&inferior_fp_registers.fr_fsr, 
	      &registers[REGISTER_BYTE (FSR_REGNUM)],
	      sizeof(inferior_fp_registers.fr_fsr));
      if (0 !=
	 ptrace (PT_SETFPREGS, inferior_pid,
		 (PTRACE_ARG3_TYPE) &inferior_fp_registers, 0))
	 perror("ptrace_setfpregs");
    }
}

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, reg_addr)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     unsigned int reg_addr;	/* Unused in this version */
{
  struct md_coredump *core_reg;
  struct trapframe64 *tf;
  struct reg64 reg64;

  core_reg = (struct md_coredump *)core_reg_sect;
  tf = &core_reg->md_tf;

  /* We get everything from the .reg section. */
  if (which != 0)
    return;

  if (core_reg_size < sizeof(*core_reg))
    {
      fprintf_unfiltered (gdb_stderr, "Couldn't read regs from core file\n");
      return;
    }

  /* Convert to a reg64 structure and use supply_regs (). */
  reg64.r_tstate = tf->tf_tstate;
  reg64.r_pc = tf->tf_pc;
  reg64.r_npc = tf->tf_npc;
  reg64.r_y = tf->tf_y;
  memcpy(reg64.r_global, tf->tf_global, sizeof (reg64.r_global));
  memcpy(reg64.r_out, tf->tf_out, sizeof (reg64.r_out));

  /* Integer registers */
  supply_regs64 ((char *) &reg64);
  supply_fakeregs ();

  /* Floating point registers */
  supply_fpregs64 ((char *) &core_reg->md_fpstate);
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
      if (core_reg_size == sizeof (struct reg64))
	{
	  supply_regs64 (core_reg_sect);
	  supply_fakeregs ();
	}
      else
	warning ("Wrong size register set in core file.");
      break;

    case 2:  /* Floating point registers */
      if (core_reg_size == sizeof (struct fpreg64))
	supply_fpregs64 (core_reg_sect);
      else
	warning ("Wrong size FP register set in core file.");
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it. */
      break;
    }
}

/* Register that we are able to handle sparcnbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns sp64nat_core_fns =
{
  bfd_target_unknown_flavour,
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,
  NULL
};

static struct core_fns sp64nat_elfcore_fns =
{
  bfd_target_elf_flavour,
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,
  NULL
};

void
_initialize_sp64nbsd_nat ()
{
  add_core_fns (&sp64nat_core_fns);
  add_core_fns (&sp64nat_elfcore_fns);
}


#ifdef	FETCH_KCORE_REGISTERS
/*
 * Get registers from a kernel crash dump or live kernel.
 * Called by kcore-nbsd.c:get_kcore_registers().
 */
void
fetch_kcore_registers (pcb)
     struct pcb *pcb;
{
  struct rwindow64 win;
  int i;
  u_long sp;

  /* We only do integer registers */
  sp = pcb->pcb_sp;

  supply_register(SP_REGNUM, (char *)&pcb->pcb_sp);
  supply_register(PC_REGNUM, (char *)&pcb->pcb_pc);
  supply_register(O7_REGNUM, (char *)&pcb->pcb_pc);
  supply_register(PSTATE_REGNUM, (char *)&pcb->pcb_pstate);
  supply_register(CWP_REGNUM, (char *)&pcb->pcb_cwp);
  /*
   * Read last register window saved on stack.
   */
  if (sp & 1)
	  sp += BIAS;
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
   *
   * XXXXX This is completely bogus for sparc64.
   */
  sp += CC64FSZ; /* XXX - MINFRAME + R_Y */
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
