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
#include <sys/sysctl.h>
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
  default_check_format,
  default_core_sniffer,
  fetch_core_registers,
  NULL
};

void
_initialize_core_netbsd ()
{
  add_core_fns (&netbsd_core_fns);
}

/*
 * Temporary routine to warn folks this code is still experimental
 */

extern int arm_apcs_32;

extern char *target_name;
void
_initialize_armnbsd_nat ()
{
  int mib[2];
  char machine[16];
  size_t len;

  /* XXX Grotty hack to guess whether this is a 26-bit system.  This
     should really be determined on the fly, to allow debugging a
     32-bit core on a 26-bit machine, or a 26-bit process on a 32-bit
     machine.  For now, users will just have to use "set apcs32" as
     appropriate. */
  mib[0] = CTL_HW;
  mib[1] = HW_MACHINE;
  len = sizeof(machine);
  if (sysctl(mib, 2, machine, &len, NULL, 0) == 0 &&
      strcmp(machine, "arm26") == 0)
    arm_apcs_32 = 0;
}

