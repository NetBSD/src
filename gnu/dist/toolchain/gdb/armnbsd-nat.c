/* Native-dependent code for BSD Unix running on i386's, for GDB.
   Copyright 1988, 1989, 1991, 1992, 1994, 1996 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <machine/reg.h>
#include "inferior.h"
#include "gdbcore.h"
#include "target.h"

void
fetch_inferior_registers(regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg fp_registers;
  int loop;

  /* integer registers */
  ptrace(PT_GETREGS, inferior_pid, (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  memcpy(&registers[REGISTER_BYTE(0)], &inferior_registers, 4*16);
  memcpy(&registers[REGISTER_BYTE(PS_REGNUM)], &inferior_registers.r_cpsr, 4);

  /* floating point registers */
  ptrace(PT_GETFPREGS, inferior_pid, (PTRACE_ARG3_TYPE) &fp_registers, 0);
  memcpy(&registers[REGISTER_BYTE(F0_REGNUM)], &fp_registers.fpr[0], 12*8);
  memcpy(&registers[REGISTER_BYTE(FPS_REGNUM)], &fp_registers.fpr_fpsr, 4);

  registers_fetched ();
}

void
store_inferior_registers(regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg fp_registers;

  /* integer registers */
  memcpy(&inferior_registers.r_cpsr, &registers[REGISTER_BYTE(PS_REGNUM)], 4);
  memcpy(&inferior_registers, &registers[REGISTER_BYTE(0)], 4*16);
  ptrace(PT_SETREGS, inferior_pid, (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  /* floating point registers */
  memcpy(&fp_registers.fpr_fpsr, &registers[REGISTER_BYTE(FPS_REGNUM)], 4);
  memcpy(&fp_registers.fpr[0], &registers[REGISTER_BYTE(F0_REGNUM)], 12*8);
  ptrace(PT_SETFPREGS, inferior_pid, (PTRACE_ARG3_TYPE) &fp_registers, 0);
}

struct md_core {
  struct reg intreg;
  struct fpreg freg;
};

void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     unsigned int ignore;
{
  struct md_core *core_reg = (struct md_core *)core_reg_sect;

  /* integer registers */
  memcpy(&registers[REGISTER_BYTE(0)], &core_reg->intreg, 4*16);
  memcpy(&registers[REGISTER_BYTE(PS_REGNUM)], &core_reg->intreg.r_cpsr, 4);
  
  /* floating point registers */
  memcpy(&registers[REGISTER_BYTE(F0_REGNUM)], &core_reg->freg.fpr[0], 12*8);
  memcpy(&registers[REGISTER_BYTE(FPS_REGNUM)], &core_reg->freg.fpr_fpsr, 4);
}

int
kernel_u_size ()
{
  return (sizeof (struct user));
}

/*
 * NetBSD core stuf should be in netbsd-core.c
 */

static struct core_fns netbsd_core_fns =
{
  bfd_target_unknown_flavour,
  fetch_core_registers,
  NULL
};

void
_initialize_core_netbsd ()
{
  add_core_fns (&netbsd_core_fns);
}

/* Single stepping support */

static char breakpoint_shadow[BREAKPOINT_MAX];
static CORE_ADDR next_pc;

/* Non-zero if we just simulated a single-step ptrace call.  This is
 * needed because we cannot remove the breakpoints in the inferior
 * process until after the `wait' in `wait_for_inferior'.
 */

int one_stepped;

/* single_step() is called just before we want to resume the inferior,
 * if we want to single-step it but there is no hardware or kernel
 * single-step support.  We find all the possible targets of the
 * coming instruction and breakpoint them.
 *
 * single_step is also called just after the inferior stops.  If we had
 * set up a simulated single-step, we undo our damage.
 */

void
single_step (ignore)
     enum target_signal ignore; /* signal, but we don't need it */
{
  CORE_ADDR arm_pc;

  if (!one_stepped)
    {
      /*
       * Ok arm_pc is the address of the instruction will will run
       * when we resume.
       * Analyse the instruction at this address to work out the
       * address of the next instruction.
       */

      arm_pc = read_register(PC_REGNUM);
      next_pc = arm_get_next_pc(arm_pc);
      
      target_insert_breakpoint(next_pc, breakpoint_shadow);
/*      printf_unfiltered("pc=%x: set break at %x\n", arm_pc, next_pc);*/

      /* We are ready to let it go */
      one_stepped = 1;
      return;
    }
  else
    {
      /* Remove breakpoints */
      target_remove_breakpoint(next_pc, breakpoint_shadow);

      one_stepped = 0;
    }
}

/*
 * Temporary routine to warn folks this code is still experimental
 */

extern char *target_name;
void
_initialize_armnbsd_nat ()
{
}
