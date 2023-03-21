/* Native-dependent code for FreeBSD/amd64.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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

#include "defs.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <machine/reg.h>

#include "fbsd-nat.h"
#include "amd64-tdep.h"
#include "amd64-nat.h"
#include "amd64-bsd-nat.h"
#include "x86-nat.h"
#include "gdbsupport/x86-xstate.h"


class amd64_fbsd_nat_target final
  : public amd64_bsd_nat_target<fbsd_nat_target>
{
public:
  /* Add some extra features to the common *BSD/amd64 target.  */
  const struct target_desc *read_description () override;

#if defined(HAVE_PT_GETDBREGS) && defined(USE_SIGTRAP_SIGINFO)
  bool supports_stopped_by_hw_breakpoint () override;
#endif
};

static amd64_fbsd_nat_target the_amd64_fbsd_nat_target;

/* Offset in `struct reg' where MEMBER is stored.  */
#define REG_OFFSET(member) offsetof (struct reg, member)

/* At amd64fbsd64_r_reg_offset[REGNUM] you'll find the offset in
   `struct reg' location where the GDB register REGNUM is stored.
   Unsupported registers are marked with `-1'.  */
static int amd64fbsd64_r_reg_offset[] =
{
  REG_OFFSET (r_rax),
  REG_OFFSET (r_rbx),
  REG_OFFSET (r_rcx),
  REG_OFFSET (r_rdx),
  REG_OFFSET (r_rsi),
  REG_OFFSET (r_rdi),
  REG_OFFSET (r_rbp),
  REG_OFFSET (r_rsp),
  REG_OFFSET (r_r8),
  REG_OFFSET (r_r9),
  REG_OFFSET (r_r10),
  REG_OFFSET (r_r11),
  REG_OFFSET (r_r12),
  REG_OFFSET (r_r13),
  REG_OFFSET (r_r14),
  REG_OFFSET (r_r15),
  REG_OFFSET (r_rip),
  REG_OFFSET (r_rflags),
  REG_OFFSET (r_cs),
  REG_OFFSET (r_ss),
  -1,
  -1,
  -1,
  -1
};


/* Mapping between the general-purpose registers in FreeBSD/amd64
   `struct reg' format and GDB's register cache layout for
   FreeBSD/i386.

   Note that most FreeBSD/amd64 registers are 64-bit, while the
   FreeBSD/i386 registers are all 32-bit, but since we're
   little-endian we get away with that.  */

/* From <machine/reg.h>.  */
static int amd64fbsd32_r_reg_offset[I386_NUM_GREGS] =
{
  14 * 8, 13 * 8,		/* %eax, %ecx */
  12 * 8, 11 * 8,		/* %edx, %ebx */
  20 * 8, 10 * 8,		/* %esp, %ebp */
  9 * 8, 8 * 8,			/* %esi, %edi */
  17 * 8, 19 * 8,		/* %eip, %eflags */
  18 * 8, 21 * 8,		/* %cs, %ss */
  -1, -1, -1, -1		/* %ds, %es, %fs, %gs */
};


/* Support for debugging kernel virtual memory images.  */

#include <machine/pcb.h>
#include <osreldate.h>

#include "bsd-kvm.h"

static int
amd64fbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  /* The following is true for FreeBSD 5.2:

     The pcb contains %rip, %rbx, %rsp, %rbp, %r12, %r13, %r14, %r15,
     %ds, %es, %fs and %gs.  This accounts for all callee-saved
     registers specified by the psABI and then some.  Here %esp
     contains the stack pointer at the point just after the call to
     cpu_switch().  From this information we reconstruct the register
     state as it would like when we just returned from cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_rsp == 0)
    return 0;

  pcb->pcb_rsp += 8;
  regcache->raw_supply (AMD64_RIP_REGNUM, &pcb->pcb_rip);
  regcache->raw_supply (AMD64_RBX_REGNUM, &pcb->pcb_rbx);
  regcache->raw_supply (AMD64_RSP_REGNUM, &pcb->pcb_rsp);
  regcache->raw_supply (AMD64_RBP_REGNUM, &pcb->pcb_rbp);
  regcache->raw_supply (12, &pcb->pcb_r12);
  regcache->raw_supply (13, &pcb->pcb_r13);
  regcache->raw_supply (14, &pcb->pcb_r14);
  regcache->raw_supply (15, &pcb->pcb_r15);
#if (__FreeBSD_version < 800075) && (__FreeBSD_kernel_version < 800075)
  /* struct pcb provides the pcb_ds/pcb_es/pcb_fs/pcb_gs fields only
     up until __FreeBSD_version 800074: The removal of these fields
     occurred on 2009-04-01 while the __FreeBSD_version number was
     bumped to 800075 on 2009-04-06.  So 800075 is the closest version
     number where we should not try to access these fields.  */
  regcache->raw_supply (AMD64_DS_REGNUM, &pcb->pcb_ds);
  regcache->raw_supply (AMD64_ES_REGNUM, &pcb->pcb_es);
  regcache->raw_supply (AMD64_FS_REGNUM, &pcb->pcb_fs);
  regcache->raw_supply (AMD64_GS_REGNUM, &pcb->pcb_gs);
#endif

  return 1;
}


/* Implement the read_description method.  */

const struct target_desc *
amd64_fbsd_nat_target::read_description ()
{
#ifdef PT_GETXSTATE_INFO
  static int xsave_probed;
  static uint64_t xcr0;
#endif
  struct reg regs;
  int is64;

  if (ptrace (PT_GETREGS, inferior_ptid.pid (),
	      (PTRACE_TYPE_ARG3) &regs, 0) == -1)
    perror_with_name (_("Couldn't get registers"));
  is64 = (regs.r_cs == GSEL (GUCODE_SEL, SEL_UPL));
#ifdef PT_GETXSTATE_INFO
  if (!xsave_probed)
    {
      struct ptrace_xstate_info info;

      if (ptrace (PT_GETXSTATE_INFO, inferior_ptid.pid (),
		  (PTRACE_TYPE_ARG3) &info, sizeof (info)) == 0)
	{
	  x86bsd_xsave_len = info.xsave_len;
	  xcr0 = info.xsave_mask;
	}
      xsave_probed = 1;
    }

  if (x86bsd_xsave_len != 0)
    {
      if (is64)
	return amd64_target_description (xcr0, true);
      else
	return i386_target_description (xcr0, true);
    }
#endif
  if (is64)
    return amd64_target_description (X86_XSTATE_SSE_MASK, true);
  else
    return i386_target_description (X86_XSTATE_SSE_MASK, true);
}

#if defined(HAVE_PT_GETDBREGS) && defined(USE_SIGTRAP_SIGINFO)
/* Implement the supports_stopped_by_hw_breakpoints method.  */

bool
amd64_fbsd_nat_target::supports_stopped_by_hw_breakpoint ()
{
  return true;
}
#endif

void _initialize_amd64fbsd_nat ();
void
_initialize_amd64fbsd_nat ()
{
  int offset;

  amd64_native_gregset32_reg_offset = amd64fbsd32_r_reg_offset;
  amd64_native_gregset64_reg_offset = amd64fbsd64_r_reg_offset;

  add_inf_child_target (&the_amd64_fbsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (amd64fbsd_supply_pcb);

  /* To support the recognition of signal handlers, i386-bsd-tdep.c
     hardcodes some constants.  Inclusion of this file means that we
     are compiling a native debugger, which means that we can use the
     system header files and sysctl(3) to get at the relevant
     information.  */

#define SC_REG_OFFSET amd64fbsd_sc_reg_offset

  /* We only check the program counter, stack pointer and frame
     pointer since these members of `struct sigcontext' are essential
     for providing backtraces.  */

#define SC_RIP_OFFSET SC_REG_OFFSET[AMD64_RIP_REGNUM]
#define SC_RSP_OFFSET SC_REG_OFFSET[AMD64_RSP_REGNUM]
#define SC_RBP_OFFSET SC_REG_OFFSET[AMD64_RBP_REGNUM]

  /* Override the default value for the offset of the program counter
     in the sigcontext structure.  */
  offset = offsetof (struct sigcontext, sc_rip);

  if (SC_RIP_OFFSET != offset)
    {
      warning (_("\
offsetof (struct sigcontext, sc_rip) yields %d instead of %d.\n\
Please report this to <bug-gdb@gnu.org>."),
	       offset, SC_RIP_OFFSET);
    }

  SC_RIP_OFFSET = offset;

  /* Likewise for the stack pointer.  */
  offset = offsetof (struct sigcontext, sc_rsp);

  if (SC_RSP_OFFSET != offset)
    {
      warning (_("\
offsetof (struct sigcontext, sc_rsp) yields %d instead of %d.\n\
Please report this to <bug-gdb@gnu.org>."),
	       offset, SC_RSP_OFFSET);
    }

  SC_RSP_OFFSET = offset;

  /* And the frame pointer.  */
  offset = offsetof (struct sigcontext, sc_rbp);

  if (SC_RBP_OFFSET != offset)
    {
      warning (_("\
offsetof (struct sigcontext, sc_rbp) yields %d instead of %d.\n\
Please report this to <bug-gdb@gnu.org>."),
	       offset, SC_RBP_OFFSET);
    }

  SC_RBP_OFFSET = offset;

#ifdef KERN_PROC_SIGTRAMP
  /* Normally signal frames are detected via amd64fbsd_sigtramp_p.
     However, FreeBSD 9.2 through 10.1 do not include the page holding
     the signal code in core dumps.  These releases do provide a
     kern.proc.sigtramp.<pid> sysctl that returns the location of the
     signal trampoline for a running process.  We fetch the location
     of the current (gdb) process and use this to identify signal
     frames in core dumps from these releases.  Note that this only
     works for core dumps of 64-bit (FreeBSD/amd64) processes and does
     not handle core dumps of 32-bit (FreeBSD/i386) processes.  */
  {
    int mib[4];
    struct kinfo_sigtramp kst;
    size_t len;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_SIGTRAMP;
    mib[3] = getpid ();
    len = sizeof (kst);
    if (sysctl (mib, 4, &kst, &len, NULL, 0) == 0)
      {
	amd64fbsd_sigtramp_start_addr = (uintptr_t) kst.ksigtramp_start;
	amd64fbsd_sigtramp_end_addr = (uintptr_t) kst.ksigtramp_end;
      }
  }
#endif
}
