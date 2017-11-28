/* GNU/Linux/x86-64 specific low level interface, for the remote server
   for GDB.
   Copyright (C) 2002-2017 Free Software Foundation, Inc.

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
#include <signal.h>
#include <limits.h>
#include <inttypes.h>
#include "linux-low.h"
#include "i387-fp.h"
#include "x86-low.h"
#include "x86-xstate.h"
#include "nat/gdb_ptrace.h"

#ifdef __x86_64__
#include "nat/amd64-linux-siginfo.h"
#endif

#include "gdb_proc_service.h"
/* Don't include elf/common.h if linux/elf.h got included by
   gdb_proc_service.h.  */
#ifndef ELFMAG0
#include "elf/common.h"
#endif

#include "agent.h"
#include "tdesc.h"
#include "tracepoint.h"
#include "ax.h"
#include "nat/linux-nat.h"
#include "nat/x86-linux.h"
#include "nat/x86-linux-dregs.h"
#include "linux-x86-tdesc.h"

#ifdef __x86_64__
static struct target_desc *tdesc_amd64_linux_no_xml;
#endif
static struct target_desc *tdesc_i386_linux_no_xml;


static unsigned char jump_insn[] = { 0xe9, 0, 0, 0, 0 };
static unsigned char small_jump_insn[] = { 0x66, 0xe9, 0, 0 };

/* Backward compatibility for gdb without XML support.  */

static const char *xmltarget_i386_linux_no_xml = "@<target>\
<architecture>i386</architecture>\
<osabi>GNU/Linux</osabi>\
</target>";

#ifdef __x86_64__
static const char *xmltarget_amd64_linux_no_xml = "@<target>\
<architecture>i386:x86-64</architecture>\
<osabi>GNU/Linux</osabi>\
</target>";
#endif

#include <sys/reg.h>
#include <sys/procfs.h>
#include "nat/gdb_ptrace.h"
#include <sys/uio.h>

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

/* This definition comes from prctl.h, but some kernels may not have it.  */
#ifndef PTRACE_ARCH_PRCTL
#define PTRACE_ARCH_PRCTL      30
#endif

/* The following definitions come from prctl.h, but may be absent
   for certain configurations.  */
#ifndef ARCH_GET_FS
#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004
#endif

/* Per-process arch-specific data we want to keep.  */

struct arch_process_info
{
  struct x86_debug_reg_state debug_reg_state;
};

#ifdef __x86_64__

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register array layout.
   Note that the transfer layout uses 64-bit regs.  */
static /*const*/ int i386_regmap[] = 
{
  RAX * 8, RCX * 8, RDX * 8, RBX * 8,
  RSP * 8, RBP * 8, RSI * 8, RDI * 8,
  RIP * 8, EFLAGS * 8, CS * 8, SS * 8,
  DS * 8, ES * 8, FS * 8, GS * 8
};

#define I386_NUM_REGS (sizeof (i386_regmap) / sizeof (i386_regmap[0]))

/* So code below doesn't have to care, i386 or amd64.  */
#define ORIG_EAX ORIG_RAX
#define REGSIZE 8

static const int x86_64_regmap[] =
{
  RAX * 8, RBX * 8, RCX * 8, RDX * 8,
  RSI * 8, RDI * 8, RBP * 8, RSP * 8,
  R8 * 8, R9 * 8, R10 * 8, R11 * 8,
  R12 * 8, R13 * 8, R14 * 8, R15 * 8,
  RIP * 8, EFLAGS * 8, CS * 8, SS * 8,
  DS * 8, ES * 8, FS * 8, GS * 8,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  ORIG_RAX * 8,
#ifdef HAVE_STRUCT_USER_REGS_STRUCT_FS_BASE
  21 * 8,  22 * 8,
#else
  -1, -1,
#endif
  -1, -1, -1, -1,			/* MPX registers BND0 ... BND3.  */
  -1, -1,				/* MPX registers BNDCFGU, BNDSTATUS.  */
  -1, -1, -1, -1, -1, -1, -1, -1,       /* xmm16 ... xmm31 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,       /* ymm16 ... ymm31 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,       /* k0 ... k7 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1,       /* zmm0 ... zmm31 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1					/* pkru  */
};

#define X86_64_NUM_REGS (sizeof (x86_64_regmap) / sizeof (x86_64_regmap[0]))
#define X86_64_USER_REGS (GS + 1)

#else /* ! __x86_64__ */

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register array layout.  */
static /*const*/ int i386_regmap[] = 
{
  EAX * 4, ECX * 4, EDX * 4, EBX * 4,
  UESP * 4, EBP * 4, ESI * 4, EDI * 4,
  EIP * 4, EFL * 4, CS * 4, SS * 4,
  DS * 4, ES * 4, FS * 4, GS * 4
};

#define I386_NUM_REGS (sizeof (i386_regmap) / sizeof (i386_regmap[0]))

#define REGSIZE 4

#endif

#ifdef __x86_64__

/* Returns true if the current inferior belongs to a x86-64 process,
   per the tdesc.  */

static int
is_64bit_tdesc (void)
{
  struct regcache *regcache = get_thread_regcache (current_thread, 0);

  return register_size (regcache->tdesc, 0) == 8;
}

#endif


/* Called by libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
#ifdef __x86_64__
  int use_64bit = is_64bit_tdesc ();

  if (use_64bit)
    {
      switch (idx)
	{
	case FS:
	  if (ptrace (PTRACE_ARCH_PRCTL, lwpid, base, ARCH_GET_FS) == 0)
	    return PS_OK;
	  break;
	case GS:
	  if (ptrace (PTRACE_ARCH_PRCTL, lwpid, base, ARCH_GET_GS) == 0)
	    return PS_OK;
	  break;
	default:
	  return PS_BADADDR;
	}
      return PS_ERR;
    }
#endif

  {
    unsigned int desc[4];

    if (ptrace (PTRACE_GET_THREAD_AREA, lwpid,
		(void *) (intptr_t) idx, (unsigned long) &desc) < 0)
      return PS_ERR;

    /* Ensure we properly extend the value to 64-bits for x86_64.  */
    *base = (void *) (uintptr_t) desc[1];
    return PS_OK;
  }
}

/* Get the thread area address.  This is used to recognize which
   thread is which when tracing with the in-process agent library.  We
   don't read anything from the address, and treat it as opaque; it's
   the address itself that we assume is unique per-thread.  */

static int
x86_get_thread_area (int lwpid, CORE_ADDR *addr)
{
#ifdef __x86_64__
  int use_64bit = is_64bit_tdesc ();

  if (use_64bit)
    {
      void *base;
      if (ptrace (PTRACE_ARCH_PRCTL, lwpid, &base, ARCH_GET_FS) == 0)
	{
	  *addr = (CORE_ADDR) (uintptr_t) base;
	  return 0;
	}

      return -1;
    }
#endif

  {
    struct lwp_info *lwp = find_lwp_pid (pid_to_ptid (lwpid));
    struct thread_info *thr = get_lwp_thread (lwp);
    struct regcache *regcache = get_thread_regcache (thr, 1);
    unsigned int desc[4];
    ULONGEST gs = 0;
    const int reg_thread_area = 3; /* bits to scale down register value.  */
    int idx;

    collect_register_by_name (regcache, "gs", &gs);

    idx = gs >> reg_thread_area;

    if (ptrace (PTRACE_GET_THREAD_AREA,
		lwpid_of (thr),
		(void *) (long) idx, (unsigned long) &desc) < 0)
      return -1;

    *addr = desc[1];
    return 0;
  }
}



static int
x86_cannot_store_register (int regno)
{
#ifdef __x86_64__
  if (is_64bit_tdesc ())
    return 0;
#endif

  return regno >= I386_NUM_REGS;
}

static int
x86_cannot_fetch_register (int regno)
{
#ifdef __x86_64__
  if (is_64bit_tdesc ())
    return 0;
#endif

  return regno >= I386_NUM_REGS;
}

static void
x86_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

#ifdef __x86_64__
  if (register_size (regcache->tdesc, 0) == 8)
    {
      for (i = 0; i < X86_64_NUM_REGS; i++)
	if (x86_64_regmap[i] != -1)
	  collect_register (regcache, i, ((char *) buf) + x86_64_regmap[i]);

#ifndef HAVE_STRUCT_USER_REGS_STRUCT_FS_BASE
      {
        unsigned long base;
        int lwpid = lwpid_of (current_thread);

        collect_register_by_name (regcache, "fs_base", &base);
        ptrace (PTRACE_ARCH_PRCTL, lwpid, &base, ARCH_SET_FS);

        collect_register_by_name (regcache, "gs_base", &base);
        ptrace (PTRACE_ARCH_PRCTL, lwpid, &base, ARCH_SET_GS);
      }
#endif

      return;
    }

  /* 32-bit inferior registers need to be zero-extended.
     Callers would read uninitialized memory otherwise.  */
  memset (buf, 0x00, X86_64_USER_REGS * 8);
#endif

  for (i = 0; i < I386_NUM_REGS; i++)
    collect_register (regcache, i, ((char *) buf) + i386_regmap[i]);

  collect_register_by_name (regcache, "orig_eax",
			    ((char *) buf) + ORIG_EAX * REGSIZE);
}

static void
x86_store_gregset (struct regcache *regcache, const void *buf)
{
  int i;

#ifdef __x86_64__
  if (register_size (regcache->tdesc, 0) == 8)
    {
      for (i = 0; i < X86_64_NUM_REGS; i++)
	if (x86_64_regmap[i] != -1)
	  supply_register (regcache, i, ((char *) buf) + x86_64_regmap[i]);

#ifndef HAVE_STRUCT_USER_REGS_STRUCT_FS_BASE
      {
        unsigned long base;
        int lwpid = lwpid_of (current_thread);

        if (ptrace (PTRACE_ARCH_PRCTL, lwpid, &base, ARCH_GET_FS) == 0)
          supply_register_by_name (regcache, "fs_base", &base);

        if (ptrace (PTRACE_ARCH_PRCTL, lwpid, &base, ARCH_GET_GS) == 0)
          supply_register_by_name (regcache, "gs_base", &base);
      }
#endif
      return;
    }
#endif

  for (i = 0; i < I386_NUM_REGS; i++)
    supply_register (regcache, i, ((char *) buf) + i386_regmap[i]);

  supply_register_by_name (regcache, "orig_eax",
			   ((char *) buf) + ORIG_EAX * REGSIZE);
}

static void
x86_fill_fpregset (struct regcache *regcache, void *buf)
{
#ifdef __x86_64__
  i387_cache_to_fxsave (regcache, buf);
#else
  i387_cache_to_fsave (regcache, buf);
#endif
}

static void
x86_store_fpregset (struct regcache *regcache, const void *buf)
{
#ifdef __x86_64__
  i387_fxsave_to_cache (regcache, buf);
#else
  i387_fsave_to_cache (regcache, buf);
#endif
}

#ifndef __x86_64__

static void
x86_fill_fpxregset (struct regcache *regcache, void *buf)
{
  i387_cache_to_fxsave (regcache, buf);
}

static void
x86_store_fpxregset (struct regcache *regcache, const void *buf)
{
  i387_fxsave_to_cache (regcache, buf);
}

#endif

static void
x86_fill_xstateregset (struct regcache *regcache, void *buf)
{
  i387_cache_to_xsave (regcache, buf);
}

static void
x86_store_xstateregset (struct regcache *regcache, const void *buf)
{
  i387_xsave_to_cache (regcache, buf);
}

/* ??? The non-biarch i386 case stores all the i387 regs twice.
   Once in i387_.*fsave.* and once in i387_.*fxsave.*.
   This is, presumably, to handle the case where PTRACE_[GS]ETFPXREGS
   doesn't work.  IWBN to avoid the duplication in the case where it
   does work.  Maybe the arch_setup routine could check whether it works
   and update the supported regsets accordingly.  */

static struct regset_info x86_regsets[] =
{
#ifdef HAVE_PTRACE_GETREGS
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, sizeof (elf_gregset_t),
    GENERAL_REGS,
    x86_fill_gregset, x86_store_gregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_X86_XSTATE, 0,
    EXTENDED_REGS, x86_fill_xstateregset, x86_store_xstateregset },
# ifndef __x86_64__
#  ifdef HAVE_PTRACE_GETFPXREGS
  { PTRACE_GETFPXREGS, PTRACE_SETFPXREGS, 0, sizeof (elf_fpxregset_t),
    EXTENDED_REGS,
    x86_fill_fpxregset, x86_store_fpxregset },
#  endif
# endif
  { PTRACE_GETFPREGS, PTRACE_SETFPREGS, 0, sizeof (elf_fpregset_t),
    FP_REGS,
    x86_fill_fpregset, x86_store_fpregset },
#endif /* HAVE_PTRACE_GETREGS */
  NULL_REGSET
};

static CORE_ADDR
x86_get_pc (struct regcache *regcache)
{
  int use_64bit = register_size (regcache->tdesc, 0) == 8;

  if (use_64bit)
    {
      uint64_t pc;

      collect_register_by_name (regcache, "rip", &pc);
      return (CORE_ADDR) pc;
    }
  else
    {
      uint32_t pc;

      collect_register_by_name (regcache, "eip", &pc);
      return (CORE_ADDR) pc;
    }
}

static void
x86_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  int use_64bit = register_size (regcache->tdesc, 0) == 8;

  if (use_64bit)
    {
      uint64_t newpc = pc;

      supply_register_by_name (regcache, "rip", &newpc);
    }
  else
    {
      uint32_t newpc = pc;

      supply_register_by_name (regcache, "eip", &newpc);
    }
}

static const gdb_byte x86_breakpoint[] = { 0xCC };
#define x86_breakpoint_len 1

static int
x86_breakpoint_at (CORE_ADDR pc)
{
  unsigned char c;

  (*the_target->read_memory) (pc, &c, 1);
  if (c == 0xCC)
    return 1;

  return 0;
}

/* Low-level function vector.  */
struct x86_dr_low_type x86_dr_low =
  {
    x86_linux_dr_set_control,
    x86_linux_dr_set_addr,
    x86_linux_dr_get_addr,
    x86_linux_dr_get_status,
    x86_linux_dr_get_control,
    sizeof (void *),
  };

/* Breakpoint/Watchpoint support.  */

static int
x86_supports_z_point_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_SW_BP:
    case Z_PACKET_HW_BP:
    case Z_PACKET_WRITE_WP:
    case Z_PACKET_ACCESS_WP:
      return 1;
    default:
      return 0;
    }
}

static int
x86_insert_point (enum raw_bkpt_type type, CORE_ADDR addr,
		  int size, struct raw_breakpoint *bp)
{
  struct process_info *proc = current_process ();

  switch (type)
    {
    case raw_bkpt_type_hw:
    case raw_bkpt_type_write_wp:
    case raw_bkpt_type_access_wp:
      {
	enum target_hw_bp_type hw_type
	  = raw_bkpt_type_to_target_hw_bp_type (type);
	struct x86_debug_reg_state *state
	  = &proc->priv->arch_private->debug_reg_state;

	return x86_dr_insert_watchpoint (state, hw_type, addr, size);
      }

    default:
      /* Unsupported.  */
      return 1;
    }
}

static int
x86_remove_point (enum raw_bkpt_type type, CORE_ADDR addr,
		  int size, struct raw_breakpoint *bp)
{
  struct process_info *proc = current_process ();

  switch (type)
    {
    case raw_bkpt_type_hw:
    case raw_bkpt_type_write_wp:
    case raw_bkpt_type_access_wp:
      {
	enum target_hw_bp_type hw_type
	  = raw_bkpt_type_to_target_hw_bp_type (type);
	struct x86_debug_reg_state *state
	  = &proc->priv->arch_private->debug_reg_state;

	return x86_dr_remove_watchpoint (state, hw_type, addr, size);
      }
    default:
      /* Unsupported.  */
      return 1;
    }
}

static int
x86_stopped_by_watchpoint (void)
{
  struct process_info *proc = current_process ();
  return x86_dr_stopped_by_watchpoint (&proc->priv->arch_private->debug_reg_state);
}

static CORE_ADDR
x86_stopped_data_address (void)
{
  struct process_info *proc = current_process ();
  CORE_ADDR addr;
  if (x86_dr_stopped_data_address (&proc->priv->arch_private->debug_reg_state,
				   &addr))
    return addr;
  return 0;
}

/* Called when a new process is created.  */

static struct arch_process_info *
x86_linux_new_process (void)
{
  struct arch_process_info *info = XCNEW (struct arch_process_info);

  x86_low_init_dregs (&info->debug_reg_state);

  return info;
}

/* Target routine for linux_new_fork.  */

static void
x86_linux_new_fork (struct process_info *parent, struct process_info *child)
{
  /* These are allocated by linux_add_process.  */
  gdb_assert (parent->priv != NULL
	      && parent->priv->arch_private != NULL);
  gdb_assert (child->priv != NULL
	      && child->priv->arch_private != NULL);

  /* Linux kernel before 2.6.33 commit
     72f674d203cd230426437cdcf7dd6f681dad8b0d
     will inherit hardware debug registers from parent
     on fork/vfork/clone.  Newer Linux kernels create such tasks with
     zeroed debug registers.

     GDB core assumes the child inherits the watchpoints/hw
     breakpoints of the parent, and will remove them all from the
     forked off process.  Copy the debug registers mirrors into the
     new process so that all breakpoints and watchpoints can be
     removed together.  The debug registers mirror will become zeroed
     in the end before detaching the forked off process, thus making
     this compatible with older Linux kernels too.  */

  *child->priv->arch_private = *parent->priv->arch_private;
}

/* See nat/x86-dregs.h.  */

struct x86_debug_reg_state *
x86_debug_reg_state (pid_t pid)
{
  struct process_info *proc = find_process_pid (pid);

  return &proc->priv->arch_private->debug_reg_state;
}

/* When GDBSERVER is built as a 64-bit application on linux, the
   PTRACE_GETSIGINFO data is always presented in 64-bit layout.  Since
   debugging a 32-bit inferior with a 64-bit GDBSERVER should look the same
   as debugging it with a 32-bit GDBSERVER, we do the 32-bit <-> 64-bit
   conversion in-place ourselves.  */

/* Convert a ptrace/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  Returns true if any
   conversion was done; false otherwise.  If DIRECTION is 1, then copy
   from INF to PTRACE.  If DIRECTION is 0, copy from PTRACE to
   INF.  */

static int
x86_siginfo_fixup (siginfo_t *ptrace, gdb_byte *inf, int direction)
{
#ifdef __x86_64__
  unsigned int machine;
  int tid = lwpid_of (current_thread);
  int is_elf64 = linux_pid_exe_is_elf_64_file (tid, &machine);

  /* Is the inferior 32-bit?  If so, then fixup the siginfo object.  */
  if (!is_64bit_tdesc ())
      return amd64_linux_siginfo_fixup_common (ptrace, inf, direction,
					       FIXUP_32);
  /* No fixup for native x32 GDB.  */
  else if (!is_elf64 && sizeof (void *) == 8)
    return amd64_linux_siginfo_fixup_common (ptrace, inf, direction,
					     FIXUP_X32);
#endif

  return 0;
}

static int use_xml;

/* Format of XSAVE extended state is:
	struct
	{
	  fxsave_bytes[0..463]
	  sw_usable_bytes[464..511]
	  xstate_hdr_bytes[512..575]
	  avx_bytes[576..831]
	  future_state etc
	};

  Same memory layout will be used for the coredump NT_X86_XSTATE
  representing the XSAVE extended state registers.

  The first 8 bytes of the sw_usable_bytes[464..467] is the OS enabled
  extended state mask, which is the same as the extended control register
  0 (the XFEATURE_ENABLED_MASK register), XCR0.  We can use this mask
  together with the mask saved in the xstate_hdr_bytes to determine what
  states the processor/OS supports and what state, used or initialized,
  the process/thread is in.  */
#define I386_LINUX_XSAVE_XCR0_OFFSET 464

/* Does the current host support the GETFPXREGS request?  The header
   file may or may not define it, and even if it is defined, the
   kernel will return EIO if it's running on a pre-SSE processor.  */
int have_ptrace_getfpxregs =
#ifdef HAVE_PTRACE_GETFPXREGS
  -1
#else
  0
#endif
;

/* Get Linux/x86 target description from running target.  */

static const struct target_desc *
x86_linux_read_description (void)
{
  unsigned int machine;
  int is_elf64;
  int xcr0_features;
  int tid;
  static uint64_t xcr0;
  struct regset_info *regset;

  tid = lwpid_of (current_thread);

  is_elf64 = linux_pid_exe_is_elf_64_file (tid, &machine);

  if (sizeof (void *) == 4)
    {
      if (is_elf64 > 0)
       error (_("Can't debug 64-bit process with 32-bit GDBserver"));
#ifndef __x86_64__
      else if (machine == EM_X86_64)
       error (_("Can't debug x86-64 process with 32-bit GDBserver"));
#endif
    }

#if !defined __x86_64__ && defined HAVE_PTRACE_GETFPXREGS
  if (machine == EM_386 && have_ptrace_getfpxregs == -1)
    {
      elf_fpxregset_t fpxregs;

      if (ptrace (PTRACE_GETFPXREGS, tid, 0, (long) &fpxregs) < 0)
	{
	  have_ptrace_getfpxregs = 0;
	  have_ptrace_getregset = 0;
	  return tdesc_i386_mmx_linux;
	}
      else
	have_ptrace_getfpxregs = 1;
    }
#endif

  if (!use_xml)
    {
      x86_xcr0 = X86_XSTATE_SSE_MASK;

      /* Don't use XML.  */
#ifdef __x86_64__
      if (machine == EM_X86_64)
	return tdesc_amd64_linux_no_xml;
      else
#endif
	return tdesc_i386_linux_no_xml;
    }

  if (have_ptrace_getregset == -1)
    {
      uint64_t xstateregs[(X86_XSTATE_SSE_SIZE / sizeof (uint64_t))];
      struct iovec iov;

      iov.iov_base = xstateregs;
      iov.iov_len = sizeof (xstateregs);

      /* Check if PTRACE_GETREGSET works.  */
      if (ptrace (PTRACE_GETREGSET, tid,
		  (unsigned int) NT_X86_XSTATE, (long) &iov) < 0)
	have_ptrace_getregset = 0;
      else
	{
	  have_ptrace_getregset = 1;

	  /* Get XCR0 from XSAVE extended state.  */
	  xcr0 = xstateregs[(I386_LINUX_XSAVE_XCR0_OFFSET
			     / sizeof (uint64_t))];

	  /* Use PTRACE_GETREGSET if it is available.  */
	  for (regset = x86_regsets;
	       regset->fill_function != NULL; regset++)
	    if (regset->get_request == PTRACE_GETREGSET)
	      regset->size = X86_XSTATE_SIZE (xcr0);
	    else if (regset->type != GENERAL_REGS)
	      regset->size = 0;
	}
    }

  /* Check the native XCR0 only if PTRACE_GETREGSET is available.  */
  xcr0_features = (have_ptrace_getregset
         && (xcr0 & X86_XSTATE_ALL_MASK));

  if (xcr0_features)
    x86_xcr0 = xcr0;

  if (machine == EM_X86_64)
    {
#ifdef __x86_64__
      if (is_elf64)
	{
	  if (xcr0_features)
	    {
	      switch (xcr0 & X86_XSTATE_ALL_MASK)
	        {
		case X86_XSTATE_AVX_MPX_AVX512_PKU_MASK:
		  return tdesc_amd64_avx_mpx_avx512_pku_linux;

		case X86_XSTATE_AVX_AVX512_MASK:
		  return tdesc_amd64_avx_avx512_linux;

		case X86_XSTATE_AVX_MPX_MASK:
		  return tdesc_amd64_avx_mpx_linux;

		case X86_XSTATE_MPX_MASK:
		  return tdesc_amd64_mpx_linux;

		case X86_XSTATE_AVX_MASK:
		  return tdesc_amd64_avx_linux;

		default:
		  return tdesc_amd64_linux;
		}
	    }
	  else
	    return tdesc_amd64_linux;
	}
      else
	{
	  if (xcr0_features)
	    {
	      switch (xcr0 & X86_XSTATE_ALL_MASK)
	        {
		case X86_XSTATE_AVX_MPX_AVX512_PKU_MASK:
		  /* No x32 MPX and PKU, fall back to avx_avx512.  */
		  return tdesc_x32_avx_avx512_linux;

		case X86_XSTATE_AVX_AVX512_MASK:
		  return tdesc_x32_avx_avx512_linux;

		case X86_XSTATE_MPX_MASK: /* No MPX on x32.  */
		case X86_XSTATE_AVX_MASK:
		  return tdesc_x32_avx_linux;

		default:
		  return tdesc_x32_linux;
		}
	    }
	  else
	    return tdesc_x32_linux;
	}
#endif
    }
  else
    {
      if (xcr0_features)
	{
	  switch (xcr0 & X86_XSTATE_ALL_MASK)
	    {
	    case X86_XSTATE_AVX_MPX_AVX512_PKU_MASK:
	      return tdesc_i386_avx_mpx_avx512_pku_linux;

	    case (X86_XSTATE_AVX_AVX512_MASK):
	      return tdesc_i386_avx_avx512_linux;

	    case (X86_XSTATE_MPX_MASK):
	      return tdesc_i386_mpx_linux;

	    case (X86_XSTATE_AVX_MPX_MASK):
	      return tdesc_i386_avx_mpx_linux;

	    case (X86_XSTATE_AVX_MASK):
	      return tdesc_i386_avx_linux;

	    default:
	      return tdesc_i386_linux;
	    }
	}
      else
	return tdesc_i386_linux;
    }

  gdb_assert_not_reached ("failed to return tdesc");
}

/* Callback for find_inferior.  Stops iteration when a thread with a
   given PID is found.  */

static int
same_process_callback (struct inferior_list_entry *entry, void *data)
{
  int pid = *(int *) data;

  return (ptid_get_pid (entry->id) == pid);
}

/* Callback for for_each_inferior.  Calls the arch_setup routine for
   each process.  */

static void
x86_arch_setup_process_callback (struct inferior_list_entry *entry)
{
  int pid = ptid_get_pid (entry->id);

  /* Look up any thread of this processes.  */
  current_thread
    = (struct thread_info *) find_inferior (&all_threads,
					    same_process_callback, &pid);

  the_low_target.arch_setup ();
}

/* Update all the target description of all processes; a new GDB
   connected, and it may or not support xml target descriptions.  */

static void
x86_linux_update_xmltarget (void)
{
  struct thread_info *saved_thread = current_thread;

  /* Before changing the register cache's internal layout, flush the
     contents of the current valid caches back to the threads, and
     release the current regcache objects.  */
  regcache_release ();

  for_each_inferior (&all_processes, x86_arch_setup_process_callback);

  current_thread = saved_thread;
}

/* Process qSupported query, "xmlRegisters=".  Update the buffer size for
   PTRACE_GETREGSET.  */

static void
x86_linux_process_qsupported (char **features, int count)
{
  int i;

  /* Return if gdb doesn't support XML.  If gdb sends "xmlRegisters="
     with "i386" in qSupported query, it supports x86 XML target
     descriptions.  */
  use_xml = 0;
  for (i = 0; i < count; i++)
    {
      const char *feature = features[i];

      if (startswith (feature, "xmlRegisters="))
	{
	  char *copy = xstrdup (feature + 13);
	  char *p;

	  for (p = strtok (copy, ","); p != NULL; p = strtok (NULL, ","))
	    {
	      if (strcmp (p, "i386") == 0)
		{
		  use_xml = 1;
		  break;
		}
	    }

	  free (copy);
	}
    }
  x86_linux_update_xmltarget ();
}

/* Common for x86/x86-64.  */

static struct regsets_info x86_regsets_info =
  {
    x86_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

#ifdef __x86_64__
static struct regs_info amd64_linux_regs_info =
  {
    NULL, /* regset_bitmap */
    NULL, /* usrregs_info */
    &x86_regsets_info
  };
#endif
static struct usrregs_info i386_linux_usrregs_info =
  {
    I386_NUM_REGS,
    i386_regmap,
  };

static struct regs_info i386_linux_regs_info =
  {
    NULL, /* regset_bitmap */
    &i386_linux_usrregs_info,
    &x86_regsets_info
  };

const struct regs_info *
x86_linux_regs_info (void)
{
#ifdef __x86_64__
  if (is_64bit_tdesc ())
    return &amd64_linux_regs_info;
  else
#endif
    return &i386_linux_regs_info;
}

/* Initialize the target description for the architecture of the
   inferior.  */

static void
x86_arch_setup (void)
{
  current_process ()->tdesc = x86_linux_read_description ();
}

/* Fill *SYSNO and *SYSRET with the syscall nr trapped and the syscall return
   code.  This should only be called if LWP got a SYSCALL_SIGTRAP.  */

static void
x86_get_syscall_trapinfo (struct regcache *regcache, int *sysno)
{
  int use_64bit = register_size (regcache->tdesc, 0) == 8;

  if (use_64bit)
    {
      long l_sysno;

      collect_register_by_name (regcache, "orig_rax", &l_sysno);
      *sysno = (int) l_sysno;
    }
  else
    collect_register_by_name (regcache, "orig_eax", sysno);
}

static int
x86_supports_tracepoints (void)
{
  return 1;
}

static void
append_insns (CORE_ADDR *to, size_t len, const unsigned char *buf)
{
  write_inferior_memory (*to, buf, len);
  *to += len;
}

static int
push_opcode (unsigned char *buf, const char *op)
{
  unsigned char *buf_org = buf;

  while (1)
    {
      char *endptr;
      unsigned long ul = strtoul (op, &endptr, 16);

      if (endptr == op)
	break;

      *buf++ = ul;
      op = endptr;
    }

  return buf - buf_org;
}

#ifdef __x86_64__

/* Build a jump pad that saves registers and calls a collection
   function.  Writes a jump instruction to the jump pad to
   JJUMPAD_INSN.  The caller is responsible to write it in at the
   tracepoint address.  */

static int
amd64_install_fast_tracepoint_jump_pad (CORE_ADDR tpoint, CORE_ADDR tpaddr,
					CORE_ADDR collector,
					CORE_ADDR lockaddr,
					ULONGEST orig_size,
					CORE_ADDR *jump_entry,
					CORE_ADDR *trampoline,
					ULONGEST *trampoline_size,
					unsigned char *jjump_pad_insn,
					ULONGEST *jjump_pad_insn_size,
					CORE_ADDR *adjusted_insn_addr,
					CORE_ADDR *adjusted_insn_addr_end,
					char *err)
{
  unsigned char buf[40];
  int i, offset;
  int64_t loffset;

  CORE_ADDR buildaddr = *jump_entry;

  /* Build the jump pad.  */

  /* First, do tracepoint data collection.  Save registers.  */
  i = 0;
  /* Need to ensure stack pointer saved first.  */
  buf[i++] = 0x54; /* push %rsp */
  buf[i++] = 0x55; /* push %rbp */
  buf[i++] = 0x57; /* push %rdi */
  buf[i++] = 0x56; /* push %rsi */
  buf[i++] = 0x52; /* push %rdx */
  buf[i++] = 0x51; /* push %rcx */
  buf[i++] = 0x53; /* push %rbx */
  buf[i++] = 0x50; /* push %rax */
  buf[i++] = 0x41; buf[i++] = 0x57; /* push %r15 */
  buf[i++] = 0x41; buf[i++] = 0x56; /* push %r14 */
  buf[i++] = 0x41; buf[i++] = 0x55; /* push %r13 */
  buf[i++] = 0x41; buf[i++] = 0x54; /* push %r12 */
  buf[i++] = 0x41; buf[i++] = 0x53; /* push %r11 */
  buf[i++] = 0x41; buf[i++] = 0x52; /* push %r10 */
  buf[i++] = 0x41; buf[i++] = 0x51; /* push %r9 */
  buf[i++] = 0x41; buf[i++] = 0x50; /* push %r8 */
  buf[i++] = 0x9c; /* pushfq */
  buf[i++] = 0x48; /* movabs <addr>,%rdi */
  buf[i++] = 0xbf;
  memcpy (buf + i, &tpaddr, 8);
  i += 8;
  buf[i++] = 0x57; /* push %rdi */
  append_insns (&buildaddr, i, buf);

  /* Stack space for the collecting_t object.  */
  i = 0;
  i += push_opcode (&buf[i], "48 83 ec 18");	/* sub $0x18,%rsp */
  i += push_opcode (&buf[i], "48 b8");          /* mov <tpoint>,%rax */
  memcpy (buf + i, &tpoint, 8);
  i += 8;
  i += push_opcode (&buf[i], "48 89 04 24");    /* mov %rax,(%rsp) */
  i += push_opcode (&buf[i],
		    "64 48 8b 04 25 00 00 00 00"); /* mov %fs:0x0,%rax */
  i += push_opcode (&buf[i], "48 89 44 24 08"); /* mov %rax,0x8(%rsp) */
  append_insns (&buildaddr, i, buf);

  /* spin-lock.  */
  i = 0;
  i += push_opcode (&buf[i], "48 be");		/* movl <lockaddr>,%rsi */
  memcpy (&buf[i], (void *) &lockaddr, 8);
  i += 8;
  i += push_opcode (&buf[i], "48 89 e1");       /* mov %rsp,%rcx */
  i += push_opcode (&buf[i], "31 c0");		/* xor %eax,%eax */
  i += push_opcode (&buf[i], "f0 48 0f b1 0e"); /* lock cmpxchg %rcx,(%rsi) */
  i += push_opcode (&buf[i], "48 85 c0");	/* test %rax,%rax */
  i += push_opcode (&buf[i], "75 f4");		/* jne <again> */
  append_insns (&buildaddr, i, buf);

  /* Set up the gdb_collect call.  */
  /* At this point, (stack pointer + 0x18) is the base of our saved
     register block.  */

  i = 0;
  i += push_opcode (&buf[i], "48 89 e6");	/* mov %rsp,%rsi */
  i += push_opcode (&buf[i], "48 83 c6 18");	/* add $0x18,%rsi */

  /* tpoint address may be 64-bit wide.  */
  i += push_opcode (&buf[i], "48 bf");		/* movl <addr>,%rdi */
  memcpy (buf + i, &tpoint, 8);
  i += 8;
  append_insns (&buildaddr, i, buf);

  /* The collector function being in the shared library, may be
     >31-bits away off the jump pad.  */
  i = 0;
  i += push_opcode (&buf[i], "48 b8");          /* mov $collector,%rax */
  memcpy (buf + i, &collector, 8);
  i += 8;
  i += push_opcode (&buf[i], "ff d0");          /* callq *%rax */
  append_insns (&buildaddr, i, buf);

  /* Clear the spin-lock.  */
  i = 0;
  i += push_opcode (&buf[i], "31 c0");		/* xor %eax,%eax */
  i += push_opcode (&buf[i], "48 a3");		/* mov %rax, lockaddr */
  memcpy (buf + i, &lockaddr, 8);
  i += 8;
  append_insns (&buildaddr, i, buf);

  /* Remove stack that had been used for the collect_t object.  */
  i = 0;
  i += push_opcode (&buf[i], "48 83 c4 18");	/* add $0x18,%rsp */
  append_insns (&buildaddr, i, buf);

  /* Restore register state.  */
  i = 0;
  buf[i++] = 0x48; /* add $0x8,%rsp */
  buf[i++] = 0x83;
  buf[i++] = 0xc4;
  buf[i++] = 0x08;
  buf[i++] = 0x9d; /* popfq */
  buf[i++] = 0x41; buf[i++] = 0x58; /* pop %r8 */
  buf[i++] = 0x41; buf[i++] = 0x59; /* pop %r9 */
  buf[i++] = 0x41; buf[i++] = 0x5a; /* pop %r10 */
  buf[i++] = 0x41; buf[i++] = 0x5b; /* pop %r11 */
  buf[i++] = 0x41; buf[i++] = 0x5c; /* pop %r12 */
  buf[i++] = 0x41; buf[i++] = 0x5d; /* pop %r13 */
  buf[i++] = 0x41; buf[i++] = 0x5e; /* pop %r14 */
  buf[i++] = 0x41; buf[i++] = 0x5f; /* pop %r15 */
  buf[i++] = 0x58; /* pop %rax */
  buf[i++] = 0x5b; /* pop %rbx */
  buf[i++] = 0x59; /* pop %rcx */
  buf[i++] = 0x5a; /* pop %rdx */
  buf[i++] = 0x5e; /* pop %rsi */
  buf[i++] = 0x5f; /* pop %rdi */
  buf[i++] = 0x5d; /* pop %rbp */
  buf[i++] = 0x5c; /* pop %rsp */
  append_insns (&buildaddr, i, buf);

  /* Now, adjust the original instruction to execute in the jump
     pad.  */
  *adjusted_insn_addr = buildaddr;
  relocate_instruction (&buildaddr, tpaddr);
  *adjusted_insn_addr_end = buildaddr;

  /* Finally, write a jump back to the program.  */

  loffset = (tpaddr + orig_size) - (buildaddr + sizeof (jump_insn));
  if (loffset > INT_MAX || loffset < INT_MIN)
    {
      sprintf (err,
	       "E.Jump back from jump pad too far from tracepoint "
	       "(offset 0x%" PRIx64 " > int32).", loffset);
      return 1;
    }

  offset = (int) loffset;
  memcpy (buf, jump_insn, sizeof (jump_insn));
  memcpy (buf + 1, &offset, 4);
  append_insns (&buildaddr, sizeof (jump_insn), buf);

  /* The jump pad is now built.  Wire in a jump to our jump pad.  This
     is always done last (by our caller actually), so that we can
     install fast tracepoints with threads running.  This relies on
     the agent's atomic write support.  */
  loffset = *jump_entry - (tpaddr + sizeof (jump_insn));
  if (loffset > INT_MAX || loffset < INT_MIN)
    {
      sprintf (err,
	       "E.Jump pad too far from tracepoint "
	       "(offset 0x%" PRIx64 " > int32).", loffset);
      return 1;
    }

  offset = (int) loffset;

  memcpy (buf, jump_insn, sizeof (jump_insn));
  memcpy (buf + 1, &offset, 4);
  memcpy (jjump_pad_insn, buf, sizeof (jump_insn));
  *jjump_pad_insn_size = sizeof (jump_insn);

  /* Return the end address of our pad.  */
  *jump_entry = buildaddr;

  return 0;
}

#endif /* __x86_64__ */

/* Build a jump pad that saves registers and calls a collection
   function.  Writes a jump instruction to the jump pad to
   JJUMPAD_INSN.  The caller is responsible to write it in at the
   tracepoint address.  */

static int
i386_install_fast_tracepoint_jump_pad (CORE_ADDR tpoint, CORE_ADDR tpaddr,
				       CORE_ADDR collector,
				       CORE_ADDR lockaddr,
				       ULONGEST orig_size,
				       CORE_ADDR *jump_entry,
				       CORE_ADDR *trampoline,
				       ULONGEST *trampoline_size,
				       unsigned char *jjump_pad_insn,
				       ULONGEST *jjump_pad_insn_size,
				       CORE_ADDR *adjusted_insn_addr,
				       CORE_ADDR *adjusted_insn_addr_end,
				       char *err)
{
  unsigned char buf[0x100];
  int i, offset;
  CORE_ADDR buildaddr = *jump_entry;

  /* Build the jump pad.  */

  /* First, do tracepoint data collection.  Save registers.  */
  i = 0;
  buf[i++] = 0x60; /* pushad */
  buf[i++] = 0x68; /* push tpaddr aka $pc */
  *((int *)(buf + i)) = (int) tpaddr;
  i += 4;
  buf[i++] = 0x9c; /* pushf */
  buf[i++] = 0x1e; /* push %ds */
  buf[i++] = 0x06; /* push %es */
  buf[i++] = 0x0f; /* push %fs */
  buf[i++] = 0xa0;
  buf[i++] = 0x0f; /* push %gs */
  buf[i++] = 0xa8;
  buf[i++] = 0x16; /* push %ss */
  buf[i++] = 0x0e; /* push %cs */
  append_insns (&buildaddr, i, buf);

  /* Stack space for the collecting_t object.  */
  i = 0;
  i += push_opcode (&buf[i], "83 ec 08");	/* sub    $0x8,%esp */

  /* Build the object.  */
  i += push_opcode (&buf[i], "b8");		/* mov    <tpoint>,%eax */
  memcpy (buf + i, &tpoint, 4);
  i += 4;
  i += push_opcode (&buf[i], "89 04 24");	   /* mov %eax,(%esp) */

  i += push_opcode (&buf[i], "65 a1 00 00 00 00"); /* mov %gs:0x0,%eax */
  i += push_opcode (&buf[i], "89 44 24 04");	   /* mov %eax,0x4(%esp) */
  append_insns (&buildaddr, i, buf);

  /* spin-lock.  Note this is using cmpxchg, which leaves i386 behind.
     If we cared for it, this could be using xchg alternatively.  */

  i = 0;
  i += push_opcode (&buf[i], "31 c0");		/* xor %eax,%eax */
  i += push_opcode (&buf[i], "f0 0f b1 25");    /* lock cmpxchg
						   %esp,<lockaddr> */
  memcpy (&buf[i], (void *) &lockaddr, 4);
  i += 4;
  i += push_opcode (&buf[i], "85 c0");		/* test %eax,%eax */
  i += push_opcode (&buf[i], "75 f2");		/* jne <again> */
  append_insns (&buildaddr, i, buf);


  /* Set up arguments to the gdb_collect call.  */
  i = 0;
  i += push_opcode (&buf[i], "89 e0");		/* mov %esp,%eax */
  i += push_opcode (&buf[i], "83 c0 08");	/* add $0x08,%eax */
  i += push_opcode (&buf[i], "89 44 24 fc");	/* mov %eax,-0x4(%esp) */
  append_insns (&buildaddr, i, buf);

  i = 0;
  i += push_opcode (&buf[i], "83 ec 08");	/* sub $0x8,%esp */
  append_insns (&buildaddr, i, buf);

  i = 0;
  i += push_opcode (&buf[i], "c7 04 24");       /* movl <addr>,(%esp) */
  memcpy (&buf[i], (void *) &tpoint, 4);
  i += 4;
  append_insns (&buildaddr, i, buf);

  buf[0] = 0xe8; /* call <reladdr> */
  offset = collector - (buildaddr + sizeof (jump_insn));
  memcpy (buf + 1, &offset, 4);
  append_insns (&buildaddr, 5, buf);
  /* Clean up after the call.  */
  buf[0] = 0x83; /* add $0x8,%esp */
  buf[1] = 0xc4;
  buf[2] = 0x08;
  append_insns (&buildaddr, 3, buf);


  /* Clear the spin-lock.  This would need the LOCK prefix on older
     broken archs.  */
  i = 0;
  i += push_opcode (&buf[i], "31 c0");		/* xor %eax,%eax */
  i += push_opcode (&buf[i], "a3");		/* mov %eax, lockaddr */
  memcpy (buf + i, &lockaddr, 4);
  i += 4;
  append_insns (&buildaddr, i, buf);


  /* Remove stack that had been used for the collect_t object.  */
  i = 0;
  i += push_opcode (&buf[i], "83 c4 08");	/* add $0x08,%esp */
  append_insns (&buildaddr, i, buf);

  i = 0;
  buf[i++] = 0x83; /* add $0x4,%esp (no pop of %cs, assume unchanged) */
  buf[i++] = 0xc4;
  buf[i++] = 0x04;
  buf[i++] = 0x17; /* pop %ss */
  buf[i++] = 0x0f; /* pop %gs */
  buf[i++] = 0xa9;
  buf[i++] = 0x0f; /* pop %fs */
  buf[i++] = 0xa1;
  buf[i++] = 0x07; /* pop %es */
  buf[i++] = 0x1f; /* pop %ds */
  buf[i++] = 0x9d; /* popf */
  buf[i++] = 0x83; /* add $0x4,%esp (pop of tpaddr aka $pc) */
  buf[i++] = 0xc4;
  buf[i++] = 0x04;
  buf[i++] = 0x61; /* popad */
  append_insns (&buildaddr, i, buf);

  /* Now, adjust the original instruction to execute in the jump
     pad.  */
  *adjusted_insn_addr = buildaddr;
  relocate_instruction (&buildaddr, tpaddr);
  *adjusted_insn_addr_end = buildaddr;

  /* Write the jump back to the program.  */
  offset = (tpaddr + orig_size) - (buildaddr + sizeof (jump_insn));
  memcpy (buf, jump_insn, sizeof (jump_insn));
  memcpy (buf + 1, &offset, 4);
  append_insns (&buildaddr, sizeof (jump_insn), buf);

  /* The jump pad is now built.  Wire in a jump to our jump pad.  This
     is always done last (by our caller actually), so that we can
     install fast tracepoints with threads running.  This relies on
     the agent's atomic write support.  */
  if (orig_size == 4)
    {
      /* Create a trampoline.  */
      *trampoline_size = sizeof (jump_insn);
      if (!claim_trampoline_space (*trampoline_size, trampoline))
	{
	  /* No trampoline space available.  */
	  strcpy (err,
		  "E.Cannot allocate trampoline space needed for fast "
		  "tracepoints on 4-byte instructions.");
	  return 1;
	}

      offset = *jump_entry - (*trampoline + sizeof (jump_insn));
      memcpy (buf, jump_insn, sizeof (jump_insn));
      memcpy (buf + 1, &offset, 4);
      write_inferior_memory (*trampoline, buf, sizeof (jump_insn));

      /* Use a 16-bit relative jump instruction to jump to the trampoline.  */
      offset = (*trampoline - (tpaddr + sizeof (small_jump_insn))) & 0xffff;
      memcpy (buf, small_jump_insn, sizeof (small_jump_insn));
      memcpy (buf + 2, &offset, 2);
      memcpy (jjump_pad_insn, buf, sizeof (small_jump_insn));
      *jjump_pad_insn_size = sizeof (small_jump_insn);
    }
  else
    {
      /* Else use a 32-bit relative jump instruction.  */
      offset = *jump_entry - (tpaddr + sizeof (jump_insn));
      memcpy (buf, jump_insn, sizeof (jump_insn));
      memcpy (buf + 1, &offset, 4);
      memcpy (jjump_pad_insn, buf, sizeof (jump_insn));
      *jjump_pad_insn_size = sizeof (jump_insn);
    }

  /* Return the end address of our pad.  */
  *jump_entry = buildaddr;

  return 0;
}

static int
x86_install_fast_tracepoint_jump_pad (CORE_ADDR tpoint, CORE_ADDR tpaddr,
				      CORE_ADDR collector,
				      CORE_ADDR lockaddr,
				      ULONGEST orig_size,
				      CORE_ADDR *jump_entry,
				      CORE_ADDR *trampoline,
				      ULONGEST *trampoline_size,
				      unsigned char *jjump_pad_insn,
				      ULONGEST *jjump_pad_insn_size,
				      CORE_ADDR *adjusted_insn_addr,
				      CORE_ADDR *adjusted_insn_addr_end,
				      char *err)
{
#ifdef __x86_64__
  if (is_64bit_tdesc ())
    return amd64_install_fast_tracepoint_jump_pad (tpoint, tpaddr,
						   collector, lockaddr,
						   orig_size, jump_entry,
						   trampoline, trampoline_size,
						   jjump_pad_insn,
						   jjump_pad_insn_size,
						   adjusted_insn_addr,
						   adjusted_insn_addr_end,
						   err);
#endif

  return i386_install_fast_tracepoint_jump_pad (tpoint, tpaddr,
						collector, lockaddr,
						orig_size, jump_entry,
						trampoline, trampoline_size,
						jjump_pad_insn,
						jjump_pad_insn_size,
						adjusted_insn_addr,
						adjusted_insn_addr_end,
						err);
}

/* Return the minimum instruction length for fast tracepoints on x86/x86-64
   architectures.  */

static int
x86_get_min_fast_tracepoint_insn_len (void)
{
  static int warned_about_fast_tracepoints = 0;

#ifdef __x86_64__
  /*  On x86-64, 5-byte jump instructions with a 4-byte offset are always
      used for fast tracepoints.  */
  if (is_64bit_tdesc ())
    return 5;
#endif

  if (agent_loaded_p ())
    {
      char errbuf[IPA_BUFSIZ];

      errbuf[0] = '\0';

      /* On x86, if trampolines are available, then 4-byte jump instructions
	 with a 2-byte offset may be used, otherwise 5-byte jump instructions
	 with a 4-byte offset are used instead.  */
      if (have_fast_tracepoint_trampoline_buffer (errbuf))
	return 4;
      else
	{
	  /* GDB has no channel to explain to user why a shorter fast
	     tracepoint is not possible, but at least make GDBserver
	     mention that something has gone awry.  */
	  if (!warned_about_fast_tracepoints)
	    {
	      warning ("4-byte fast tracepoints not available; %s\n", errbuf);
	      warned_about_fast_tracepoints = 1;
	    }
	  return 5;
	}
    }
  else
    {
      /* Indicate that the minimum length is currently unknown since the IPA
	 has not loaded yet.  */
      return 0;
    }
}

static void
add_insns (unsigned char *start, int len)
{
  CORE_ADDR buildaddr = current_insn_ptr;

  if (debug_threads)
    debug_printf ("Adding %d bytes of insn at %s\n",
		  len, paddress (buildaddr));

  append_insns (&buildaddr, len, start);
  current_insn_ptr = buildaddr;
}

/* Our general strategy for emitting code is to avoid specifying raw
   bytes whenever possible, and instead copy a block of inline asm
   that is embedded in the function.  This is a little messy, because
   we need to keep the compiler from discarding what looks like dead
   code, plus suppress various warnings.  */

#define EMIT_ASM(NAME, INSNS)						\
  do									\
    {									\
      extern unsigned char start_ ## NAME, end_ ## NAME;		\
      add_insns (&start_ ## NAME, &end_ ## NAME - &start_ ## NAME);	\
      __asm__ ("jmp end_" #NAME "\n"					\
	       "\t" "start_" #NAME ":"					\
	       "\t" INSNS "\n"						\
	       "\t" "end_" #NAME ":");					\
    } while (0)

#ifdef __x86_64__

#define EMIT_ASM32(NAME,INSNS)						\
  do									\
    {									\
      extern unsigned char start_ ## NAME, end_ ## NAME;		\
      add_insns (&start_ ## NAME, &end_ ## NAME - &start_ ## NAME);	\
      __asm__ (".code32\n"						\
	       "\t" "jmp end_" #NAME "\n"				\
	       "\t" "start_" #NAME ":\n"				\
	       "\t" INSNS "\n"						\
	       "\t" "end_" #NAME ":\n"					\
	       ".code64\n");						\
    } while (0)

#else

#define EMIT_ASM32(NAME,INSNS) EMIT_ASM(NAME,INSNS)

#endif

#ifdef __x86_64__

static void
amd64_emit_prologue (void)
{
  EMIT_ASM (amd64_prologue,
	    "pushq %rbp\n\t"
	    "movq %rsp,%rbp\n\t"
	    "sub $0x20,%rsp\n\t"
	    "movq %rdi,-8(%rbp)\n\t"
	    "movq %rsi,-16(%rbp)");
}


static void
amd64_emit_epilogue (void)
{
  EMIT_ASM (amd64_epilogue,
	    "movq -16(%rbp),%rdi\n\t"
	    "movq %rax,(%rdi)\n\t"
	    "xor %rax,%rax\n\t"
	    "leave\n\t"
	    "ret");
}

static void
amd64_emit_add (void)
{
  EMIT_ASM (amd64_add,
	    "add (%rsp),%rax\n\t"
	    "lea 0x8(%rsp),%rsp");
}

static void
amd64_emit_sub (void)
{
  EMIT_ASM (amd64_sub,
	    "sub %rax,(%rsp)\n\t"
	    "pop %rax");
}

static void
amd64_emit_mul (void)
{
  emit_error = 1;
}

static void
amd64_emit_lsh (void)
{
  emit_error = 1;
}

static void
amd64_emit_rsh_signed (void)
{
  emit_error = 1;
}

static void
amd64_emit_rsh_unsigned (void)
{
  emit_error = 1;
}

static void
amd64_emit_ext (int arg)
{
  switch (arg)
    {
    case 8:
      EMIT_ASM (amd64_ext_8,
		"cbtw\n\t"
		"cwtl\n\t"
		"cltq");
      break;
    case 16:
      EMIT_ASM (amd64_ext_16,
		"cwtl\n\t"
		"cltq");
      break;
    case 32:
      EMIT_ASM (amd64_ext_32,
		"cltq");
      break;
    default:
      emit_error = 1;
    }
}

static void
amd64_emit_log_not (void)
{
  EMIT_ASM (amd64_log_not,
	    "test %rax,%rax\n\t"
	    "sete %cl\n\t"
	    "movzbq %cl,%rax");
}

static void
amd64_emit_bit_and (void)
{
  EMIT_ASM (amd64_and,
	    "and (%rsp),%rax\n\t"
	    "lea 0x8(%rsp),%rsp");
}

static void
amd64_emit_bit_or (void)
{
  EMIT_ASM (amd64_or,
	    "or (%rsp),%rax\n\t"
	    "lea 0x8(%rsp),%rsp");
}

static void
amd64_emit_bit_xor (void)
{
  EMIT_ASM (amd64_xor,
	    "xor (%rsp),%rax\n\t"
	    "lea 0x8(%rsp),%rsp");
}

static void
amd64_emit_bit_not (void)
{
  EMIT_ASM (amd64_bit_not,
	    "xorq $0xffffffffffffffff,%rax");
}

static void
amd64_emit_equal (void)
{
  EMIT_ASM (amd64_equal,
	    "cmp %rax,(%rsp)\n\t"
	    "je .Lamd64_equal_true\n\t"
	    "xor %rax,%rax\n\t"
	    "jmp .Lamd64_equal_end\n\t"
	    ".Lamd64_equal_true:\n\t"
	    "mov $0x1,%rax\n\t"
	    ".Lamd64_equal_end:\n\t"
	    "lea 0x8(%rsp),%rsp");
}

static void
amd64_emit_less_signed (void)
{
  EMIT_ASM (amd64_less_signed,
	    "cmp %rax,(%rsp)\n\t"
	    "jl .Lamd64_less_signed_true\n\t"
	    "xor %rax,%rax\n\t"
	    "jmp .Lamd64_less_signed_end\n\t"
	    ".Lamd64_less_signed_true:\n\t"
	    "mov $1,%rax\n\t"
	    ".Lamd64_less_signed_end:\n\t"
	    "lea 0x8(%rsp),%rsp");
}

static void
amd64_emit_less_unsigned (void)
{
  EMIT_ASM (amd64_less_unsigned,
	    "cmp %rax,(%rsp)\n\t"
	    "jb .Lamd64_less_unsigned_true\n\t"
	    "xor %rax,%rax\n\t"
	    "jmp .Lamd64_less_unsigned_end\n\t"
	    ".Lamd64_less_unsigned_true:\n\t"
	    "mov $1,%rax\n\t"
	    ".Lamd64_less_unsigned_end:\n\t"
	    "lea 0x8(%rsp),%rsp");
}

static void
amd64_emit_ref (int size)
{
  switch (size)
    {
    case 1:
      EMIT_ASM (amd64_ref1,
		"movb (%rax),%al");
      break;
    case 2:
      EMIT_ASM (amd64_ref2,
		"movw (%rax),%ax");
      break;
    case 4:
      EMIT_ASM (amd64_ref4,
		"movl (%rax),%eax");
      break;
    case 8:
      EMIT_ASM (amd64_ref8,
		"movq (%rax),%rax");
      break;
    }
}

static void
amd64_emit_if_goto (int *offset_p, int *size_p)
{
  EMIT_ASM (amd64_if_goto,
	    "mov %rax,%rcx\n\t"
	    "pop %rax\n\t"
	    "cmp $0,%rcx\n\t"
	    ".byte 0x0f, 0x85, 0x0, 0x0, 0x0, 0x0");
  if (offset_p)
    *offset_p = 10;
  if (size_p)
    *size_p = 4;
}

static void
amd64_emit_goto (int *offset_p, int *size_p)
{
  EMIT_ASM (amd64_goto,
	    ".byte 0xe9, 0x0, 0x0, 0x0, 0x0");
  if (offset_p)
    *offset_p = 1;
  if (size_p)
    *size_p = 4;
}

static void
amd64_write_goto_address (CORE_ADDR from, CORE_ADDR to, int size)
{
  int diff = (to - (from + size));
  unsigned char buf[sizeof (int)];

  if (size != 4)
    {
      emit_error = 1;
      return;
    }

  memcpy (buf, &diff, sizeof (int));
  write_inferior_memory (from, buf, sizeof (int));
}

static void
amd64_emit_const (LONGEST num)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr = current_insn_ptr;

  i = 0;
  buf[i++] = 0x48;  buf[i++] = 0xb8; /* mov $<n>,%rax */
  memcpy (&buf[i], &num, sizeof (num));
  i += 8;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
}

static void
amd64_emit_call (CORE_ADDR fn)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr;
  LONGEST offset64;

  /* The destination function being in the shared library, may be
     >31-bits away off the compiled code pad.  */

  buildaddr = current_insn_ptr;

  offset64 = fn - (buildaddr + 1 /* call op */ + 4 /* 32-bit offset */);

  i = 0;

  if (offset64 > INT_MAX || offset64 < INT_MIN)
    {
      /* Offset is too large for a call.  Use callq, but that requires
	 a register, so avoid it if possible.  Use r10, since it is
	 call-clobbered, we don't have to push/pop it.  */
      buf[i++] = 0x48; /* mov $fn,%r10 */
      buf[i++] = 0xba;
      memcpy (buf + i, &fn, 8);
      i += 8;
      buf[i++] = 0xff; /* callq *%r10 */
      buf[i++] = 0xd2;
    }
  else
    {
      int offset32 = offset64; /* we know we can't overflow here.  */

      buf[i++] = 0xe8; /* call <reladdr> */
      memcpy (buf + i, &offset32, 4);
      i += 4;
    }

  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
}

static void
amd64_emit_reg (int reg)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr;

  /* Assume raw_regs is still in %rdi.  */
  buildaddr = current_insn_ptr;
  i = 0;
  buf[i++] = 0xbe; /* mov $<n>,%esi */
  memcpy (&buf[i], &reg, sizeof (reg));
  i += 4;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
  amd64_emit_call (get_raw_reg_func_addr ());
}

static void
amd64_emit_pop (void)
{
  EMIT_ASM (amd64_pop,
	    "pop %rax");
}

static void
amd64_emit_stack_flush (void)
{
  EMIT_ASM (amd64_stack_flush,
	    "push %rax");
}

static void
amd64_emit_zero_ext (int arg)
{
  switch (arg)
    {
    case 8:
      EMIT_ASM (amd64_zero_ext_8,
		"and $0xff,%rax");
      break;
    case 16:
      EMIT_ASM (amd64_zero_ext_16,
		"and $0xffff,%rax");
      break;
    case 32:
      EMIT_ASM (amd64_zero_ext_32,
		"mov $0xffffffff,%rcx\n\t"
		"and %rcx,%rax");
      break;
    default:
      emit_error = 1;
    }
}

static void
amd64_emit_swap (void)
{
  EMIT_ASM (amd64_swap,
	    "mov %rax,%rcx\n\t"
	    "pop %rax\n\t"
	    "push %rcx");
}

static void
amd64_emit_stack_adjust (int n)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr = current_insn_ptr;

  i = 0;
  buf[i++] = 0x48; /* lea $<n>(%rsp),%rsp */
  buf[i++] = 0x8d;
  buf[i++] = 0x64;
  buf[i++] = 0x24;
  /* This only handles adjustments up to 16, but we don't expect any more.  */
  buf[i++] = n * 8;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
}

/* FN's prototype is `LONGEST(*fn)(int)'.  */

static void
amd64_emit_int_call_1 (CORE_ADDR fn, int arg1)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr;

  buildaddr = current_insn_ptr;
  i = 0;
  buf[i++] = 0xbf; /* movl $<n>,%edi */
  memcpy (&buf[i], &arg1, sizeof (arg1));
  i += 4;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
  amd64_emit_call (fn);
}

/* FN's prototype is `void(*fn)(int,LONGEST)'.  */

static void
amd64_emit_void_call_2 (CORE_ADDR fn, int arg1)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr;

  buildaddr = current_insn_ptr;
  i = 0;
  buf[i++] = 0xbf; /* movl $<n>,%edi */
  memcpy (&buf[i], &arg1, sizeof (arg1));
  i += 4;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
  EMIT_ASM (amd64_void_call_2_a,
	    /* Save away a copy of the stack top.  */
	    "push %rax\n\t"
	    /* Also pass top as the second argument.  */
	    "mov %rax,%rsi");
  amd64_emit_call (fn);
  EMIT_ASM (amd64_void_call_2_b,
	    /* Restore the stack top, %rax may have been trashed.  */
	    "pop %rax");
}

void
amd64_emit_eq_goto (int *offset_p, int *size_p)
{
  EMIT_ASM (amd64_eq,
	    "cmp %rax,(%rsp)\n\t"
	    "jne .Lamd64_eq_fallthru\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax\n\t"
	    /* jmp, but don't trust the assembler to choose the right jump */
	    ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	    ".Lamd64_eq_fallthru:\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax");

  if (offset_p)
    *offset_p = 13;
  if (size_p)
    *size_p = 4;
}

void
amd64_emit_ne_goto (int *offset_p, int *size_p)
{
  EMIT_ASM (amd64_ne,
	    "cmp %rax,(%rsp)\n\t"
	    "je .Lamd64_ne_fallthru\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax\n\t"
	    /* jmp, but don't trust the assembler to choose the right jump */
	    ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	    ".Lamd64_ne_fallthru:\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax");

  if (offset_p)
    *offset_p = 13;
  if (size_p)
    *size_p = 4;
}

void
amd64_emit_lt_goto (int *offset_p, int *size_p)
{
  EMIT_ASM (amd64_lt,
	    "cmp %rax,(%rsp)\n\t"
	    "jnl .Lamd64_lt_fallthru\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax\n\t"
	    /* jmp, but don't trust the assembler to choose the right jump */
	    ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	    ".Lamd64_lt_fallthru:\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax");

  if (offset_p)
    *offset_p = 13;
  if (size_p)
    *size_p = 4;
}

void
amd64_emit_le_goto (int *offset_p, int *size_p)
{
  EMIT_ASM (amd64_le,
	    "cmp %rax,(%rsp)\n\t"
	    "jnle .Lamd64_le_fallthru\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax\n\t"
	    /* jmp, but don't trust the assembler to choose the right jump */
	    ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	    ".Lamd64_le_fallthru:\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax");

  if (offset_p)
    *offset_p = 13;
  if (size_p)
    *size_p = 4;
}

void
amd64_emit_gt_goto (int *offset_p, int *size_p)
{
  EMIT_ASM (amd64_gt,
	    "cmp %rax,(%rsp)\n\t"
	    "jng .Lamd64_gt_fallthru\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax\n\t"
	    /* jmp, but don't trust the assembler to choose the right jump */
	    ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	    ".Lamd64_gt_fallthru:\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax");

  if (offset_p)
    *offset_p = 13;
  if (size_p)
    *size_p = 4;
}

void
amd64_emit_ge_goto (int *offset_p, int *size_p)
{
  EMIT_ASM (amd64_ge,
	    "cmp %rax,(%rsp)\n\t"
	    "jnge .Lamd64_ge_fallthru\n\t"
	    ".Lamd64_ge_jump:\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax\n\t"
	    /* jmp, but don't trust the assembler to choose the right jump */
	    ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	    ".Lamd64_ge_fallthru:\n\t"
	    "lea 0x8(%rsp),%rsp\n\t"
	    "pop %rax");

  if (offset_p)
    *offset_p = 13;
  if (size_p)
    *size_p = 4;
}

struct emit_ops amd64_emit_ops =
  {
    amd64_emit_prologue,
    amd64_emit_epilogue,
    amd64_emit_add,
    amd64_emit_sub,
    amd64_emit_mul,
    amd64_emit_lsh,
    amd64_emit_rsh_signed,
    amd64_emit_rsh_unsigned,
    amd64_emit_ext,
    amd64_emit_log_not,
    amd64_emit_bit_and,
    amd64_emit_bit_or,
    amd64_emit_bit_xor,
    amd64_emit_bit_not,
    amd64_emit_equal,
    amd64_emit_less_signed,
    amd64_emit_less_unsigned,
    amd64_emit_ref,
    amd64_emit_if_goto,
    amd64_emit_goto,
    amd64_write_goto_address,
    amd64_emit_const,
    amd64_emit_call,
    amd64_emit_reg,
    amd64_emit_pop,
    amd64_emit_stack_flush,
    amd64_emit_zero_ext,
    amd64_emit_swap,
    amd64_emit_stack_adjust,
    amd64_emit_int_call_1,
    amd64_emit_void_call_2,
    amd64_emit_eq_goto,
    amd64_emit_ne_goto,
    amd64_emit_lt_goto,
    amd64_emit_le_goto,
    amd64_emit_gt_goto,
    amd64_emit_ge_goto
  };

#endif /* __x86_64__ */

static void
i386_emit_prologue (void)
{
  EMIT_ASM32 (i386_prologue,
	    "push %ebp\n\t"
	    "mov %esp,%ebp\n\t"
	    "push %ebx");
  /* At this point, the raw regs base address is at 8(%ebp), and the
     value pointer is at 12(%ebp).  */
}

static void
i386_emit_epilogue (void)
{
  EMIT_ASM32 (i386_epilogue,
	    "mov 12(%ebp),%ecx\n\t"
	    "mov %eax,(%ecx)\n\t"
	    "mov %ebx,0x4(%ecx)\n\t"
	    "xor %eax,%eax\n\t"
	    "pop %ebx\n\t"
	    "pop %ebp\n\t"
	    "ret");
}

static void
i386_emit_add (void)
{
  EMIT_ASM32 (i386_add,
	    "add (%esp),%eax\n\t"
	    "adc 0x4(%esp),%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

static void
i386_emit_sub (void)
{
  EMIT_ASM32 (i386_sub,
	    "subl %eax,(%esp)\n\t"
	    "sbbl %ebx,4(%esp)\n\t"
	    "pop %eax\n\t"
	    "pop %ebx\n\t");
}

static void
i386_emit_mul (void)
{
  emit_error = 1;
}

static void
i386_emit_lsh (void)
{
  emit_error = 1;
}

static void
i386_emit_rsh_signed (void)
{
  emit_error = 1;
}

static void
i386_emit_rsh_unsigned (void)
{
  emit_error = 1;
}

static void
i386_emit_ext (int arg)
{
  switch (arg)
    {
    case 8:
      EMIT_ASM32 (i386_ext_8,
		"cbtw\n\t"
		"cwtl\n\t"
		"movl %eax,%ebx\n\t"
		"sarl $31,%ebx");
      break;
    case 16:
      EMIT_ASM32 (i386_ext_16,
		"cwtl\n\t"
		"movl %eax,%ebx\n\t"
		"sarl $31,%ebx");
      break;
    case 32:
      EMIT_ASM32 (i386_ext_32,
		"movl %eax,%ebx\n\t"
		"sarl $31,%ebx");
      break;
    default:
      emit_error = 1;
    }
}

static void
i386_emit_log_not (void)
{
  EMIT_ASM32 (i386_log_not,
	    "or %ebx,%eax\n\t"
	    "test %eax,%eax\n\t"
	    "sete %cl\n\t"
	    "xor %ebx,%ebx\n\t"
	    "movzbl %cl,%eax");
}

static void
i386_emit_bit_and (void)
{
  EMIT_ASM32 (i386_and,
	    "and (%esp),%eax\n\t"
	    "and 0x4(%esp),%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

static void
i386_emit_bit_or (void)
{
  EMIT_ASM32 (i386_or,
	    "or (%esp),%eax\n\t"
	    "or 0x4(%esp),%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

static void
i386_emit_bit_xor (void)
{
  EMIT_ASM32 (i386_xor,
	    "xor (%esp),%eax\n\t"
	    "xor 0x4(%esp),%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

static void
i386_emit_bit_not (void)
{
  EMIT_ASM32 (i386_bit_not,
	    "xor $0xffffffff,%eax\n\t"
	    "xor $0xffffffff,%ebx\n\t");
}

static void
i386_emit_equal (void)
{
  EMIT_ASM32 (i386_equal,
	    "cmpl %ebx,4(%esp)\n\t"
	    "jne .Li386_equal_false\n\t"
	    "cmpl %eax,(%esp)\n\t"
	    "je .Li386_equal_true\n\t"
	    ".Li386_equal_false:\n\t"
	    "xor %eax,%eax\n\t"
	    "jmp .Li386_equal_end\n\t"
	    ".Li386_equal_true:\n\t"
	    "mov $1,%eax\n\t"
	    ".Li386_equal_end:\n\t"
	    "xor %ebx,%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

static void
i386_emit_less_signed (void)
{
  EMIT_ASM32 (i386_less_signed,
	    "cmpl %ebx,4(%esp)\n\t"
	    "jl .Li386_less_signed_true\n\t"
	    "jne .Li386_less_signed_false\n\t"
	    "cmpl %eax,(%esp)\n\t"
	    "jl .Li386_less_signed_true\n\t"
	    ".Li386_less_signed_false:\n\t"
	    "xor %eax,%eax\n\t"
	    "jmp .Li386_less_signed_end\n\t"
	    ".Li386_less_signed_true:\n\t"
	    "mov $1,%eax\n\t"
	    ".Li386_less_signed_end:\n\t"
	    "xor %ebx,%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

static void
i386_emit_less_unsigned (void)
{
  EMIT_ASM32 (i386_less_unsigned,
	    "cmpl %ebx,4(%esp)\n\t"
	    "jb .Li386_less_unsigned_true\n\t"
	    "jne .Li386_less_unsigned_false\n\t"
	    "cmpl %eax,(%esp)\n\t"
	    "jb .Li386_less_unsigned_true\n\t"
	    ".Li386_less_unsigned_false:\n\t"
	    "xor %eax,%eax\n\t"
	    "jmp .Li386_less_unsigned_end\n\t"
	    ".Li386_less_unsigned_true:\n\t"
	    "mov $1,%eax\n\t"
	    ".Li386_less_unsigned_end:\n\t"
	    "xor %ebx,%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

static void
i386_emit_ref (int size)
{
  switch (size)
    {
    case 1:
      EMIT_ASM32 (i386_ref1,
		"movb (%eax),%al");
      break;
    case 2:
      EMIT_ASM32 (i386_ref2,
		"movw (%eax),%ax");
      break;
    case 4:
      EMIT_ASM32 (i386_ref4,
		"movl (%eax),%eax");
      break;
    case 8:
      EMIT_ASM32 (i386_ref8,
		"movl 4(%eax),%ebx\n\t"
		"movl (%eax),%eax");
      break;
    }
}

static void
i386_emit_if_goto (int *offset_p, int *size_p)
{
  EMIT_ASM32 (i386_if_goto,
	    "mov %eax,%ecx\n\t"
	    "or %ebx,%ecx\n\t"
	    "pop %eax\n\t"
	    "pop %ebx\n\t"
	    "cmpl $0,%ecx\n\t"
	    /* Don't trust the assembler to choose the right jump */
	    ".byte 0x0f, 0x85, 0x0, 0x0, 0x0, 0x0");

  if (offset_p)
    *offset_p = 11; /* be sure that this matches the sequence above */
  if (size_p)
    *size_p = 4;
}

static void
i386_emit_goto (int *offset_p, int *size_p)
{
  EMIT_ASM32 (i386_goto,
	    /* Don't trust the assembler to choose the right jump */
	    ".byte 0xe9, 0x0, 0x0, 0x0, 0x0");
  if (offset_p)
    *offset_p = 1;
  if (size_p)
    *size_p = 4;
}

static void
i386_write_goto_address (CORE_ADDR from, CORE_ADDR to, int size)
{
  int diff = (to - (from + size));
  unsigned char buf[sizeof (int)];

  /* We're only doing 4-byte sizes at the moment.  */
  if (size != 4)
    {
      emit_error = 1;
      return;
    }

  memcpy (buf, &diff, sizeof (int));
  write_inferior_memory (from, buf, sizeof (int));
}

static void
i386_emit_const (LONGEST num)
{
  unsigned char buf[16];
  int i, hi, lo;
  CORE_ADDR buildaddr = current_insn_ptr;

  i = 0;
  buf[i++] = 0xb8; /* mov $<n>,%eax */
  lo = num & 0xffffffff;
  memcpy (&buf[i], &lo, sizeof (lo));
  i += 4;
  hi = ((num >> 32) & 0xffffffff);
  if (hi)
    {
      buf[i++] = 0xbb; /* mov $<n>,%ebx */
      memcpy (&buf[i], &hi, sizeof (hi));
      i += 4;
    }
  else
    {
      buf[i++] = 0x31; buf[i++] = 0xdb; /* xor %ebx,%ebx */
    }
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
}

static void
i386_emit_call (CORE_ADDR fn)
{
  unsigned char buf[16];
  int i, offset;
  CORE_ADDR buildaddr;

  buildaddr = current_insn_ptr;
  i = 0;
  buf[i++] = 0xe8; /* call <reladdr> */
  offset = ((int) fn) - (buildaddr + 5);
  memcpy (buf + 1, &offset, 4);
  append_insns (&buildaddr, 5, buf);
  current_insn_ptr = buildaddr;
}

static void
i386_emit_reg (int reg)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr;

  EMIT_ASM32 (i386_reg_a,
	    "sub $0x8,%esp");
  buildaddr = current_insn_ptr;
  i = 0;
  buf[i++] = 0xb8; /* mov $<n>,%eax */
  memcpy (&buf[i], &reg, sizeof (reg));
  i += 4;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
  EMIT_ASM32 (i386_reg_b,
	    "mov %eax,4(%esp)\n\t"
	    "mov 8(%ebp),%eax\n\t"
	    "mov %eax,(%esp)");
  i386_emit_call (get_raw_reg_func_addr ());
  EMIT_ASM32 (i386_reg_c,
	    "xor %ebx,%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

static void
i386_emit_pop (void)
{
  EMIT_ASM32 (i386_pop,
	    "pop %eax\n\t"
	    "pop %ebx");
}

static void
i386_emit_stack_flush (void)
{
  EMIT_ASM32 (i386_stack_flush,
	    "push %ebx\n\t"
	    "push %eax");
}

static void
i386_emit_zero_ext (int arg)
{
  switch (arg)
    {
    case 8:
      EMIT_ASM32 (i386_zero_ext_8,
		"and $0xff,%eax\n\t"
		"xor %ebx,%ebx");
      break;
    case 16:
      EMIT_ASM32 (i386_zero_ext_16,
		"and $0xffff,%eax\n\t"
		"xor %ebx,%ebx");
      break;
    case 32:
      EMIT_ASM32 (i386_zero_ext_32,
		"xor %ebx,%ebx");
      break;
    default:
      emit_error = 1;
    }
}

static void
i386_emit_swap (void)
{
  EMIT_ASM32 (i386_swap,
	    "mov %eax,%ecx\n\t"
	    "mov %ebx,%edx\n\t"
	    "pop %eax\n\t"
	    "pop %ebx\n\t"
	    "push %edx\n\t"
	    "push %ecx");
}

static void
i386_emit_stack_adjust (int n)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr = current_insn_ptr;

  i = 0;
  buf[i++] = 0x8d; /* lea $<n>(%esp),%esp */
  buf[i++] = 0x64;
  buf[i++] = 0x24;
  buf[i++] = n * 8;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
}

/* FN's prototype is `LONGEST(*fn)(int)'.  */

static void
i386_emit_int_call_1 (CORE_ADDR fn, int arg1)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr;

  EMIT_ASM32 (i386_int_call_1_a,
	    /* Reserve a bit of stack space.  */
	    "sub $0x8,%esp");
  /* Put the one argument on the stack.  */
  buildaddr = current_insn_ptr;
  i = 0;
  buf[i++] = 0xc7;  /* movl $<arg1>,(%esp) */
  buf[i++] = 0x04;
  buf[i++] = 0x24;
  memcpy (&buf[i], &arg1, sizeof (arg1));
  i += 4;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
  i386_emit_call (fn);
  EMIT_ASM32 (i386_int_call_1_c,
	    "mov %edx,%ebx\n\t"
	    "lea 0x8(%esp),%esp");
}

/* FN's prototype is `void(*fn)(int,LONGEST)'.  */

static void
i386_emit_void_call_2 (CORE_ADDR fn, int arg1)
{
  unsigned char buf[16];
  int i;
  CORE_ADDR buildaddr;

  EMIT_ASM32 (i386_void_call_2_a,
	    /* Preserve %eax only; we don't have to worry about %ebx.  */
	    "push %eax\n\t"
	    /* Reserve a bit of stack space for arguments.  */
	    "sub $0x10,%esp\n\t"
	    /* Copy "top" to the second argument position.  (Note that
	       we can't assume function won't scribble on its
	       arguments, so don't try to restore from this.)  */
	    "mov %eax,4(%esp)\n\t"
	    "mov %ebx,8(%esp)");
  /* Put the first argument on the stack.  */
  buildaddr = current_insn_ptr;
  i = 0;
  buf[i++] = 0xc7;  /* movl $<arg1>,(%esp) */
  buf[i++] = 0x04;
  buf[i++] = 0x24;
  memcpy (&buf[i], &arg1, sizeof (arg1));
  i += 4;
  append_insns (&buildaddr, i, buf);
  current_insn_ptr = buildaddr;
  i386_emit_call (fn);
  EMIT_ASM32 (i386_void_call_2_b,
	    "lea 0x10(%esp),%esp\n\t"
	    /* Restore original stack top.  */
	    "pop %eax");
}


void
i386_emit_eq_goto (int *offset_p, int *size_p)
{
  EMIT_ASM32 (eq,
	      /* Check low half first, more likely to be decider  */
	      "cmpl %eax,(%esp)\n\t"
	      "jne .Leq_fallthru\n\t"
	      "cmpl %ebx,4(%esp)\n\t"
	      "jne .Leq_fallthru\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx\n\t"
	      /* jmp, but don't trust the assembler to choose the right jump */
	      ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	      ".Leq_fallthru:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx");

  if (offset_p)
    *offset_p = 18;
  if (size_p)
    *size_p = 4;
}

void
i386_emit_ne_goto (int *offset_p, int *size_p)
{
  EMIT_ASM32 (ne,
	      /* Check low half first, more likely to be decider  */
	      "cmpl %eax,(%esp)\n\t"
	      "jne .Lne_jump\n\t"
	      "cmpl %ebx,4(%esp)\n\t"
	      "je .Lne_fallthru\n\t"
	      ".Lne_jump:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx\n\t"
	      /* jmp, but don't trust the assembler to choose the right jump */
	      ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	      ".Lne_fallthru:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx");

  if (offset_p)
    *offset_p = 18;
  if (size_p)
    *size_p = 4;
}

void
i386_emit_lt_goto (int *offset_p, int *size_p)
{
  EMIT_ASM32 (lt,
	      "cmpl %ebx,4(%esp)\n\t"
	      "jl .Llt_jump\n\t"
	      "jne .Llt_fallthru\n\t"
	      "cmpl %eax,(%esp)\n\t"
	      "jnl .Llt_fallthru\n\t"
	      ".Llt_jump:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx\n\t"
	      /* jmp, but don't trust the assembler to choose the right jump */
	      ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	      ".Llt_fallthru:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx");

  if (offset_p)
    *offset_p = 20;
  if (size_p)
    *size_p = 4;
}

void
i386_emit_le_goto (int *offset_p, int *size_p)
{
  EMIT_ASM32 (le,
	      "cmpl %ebx,4(%esp)\n\t"
	      "jle .Lle_jump\n\t"
	      "jne .Lle_fallthru\n\t"
	      "cmpl %eax,(%esp)\n\t"
	      "jnle .Lle_fallthru\n\t"
	      ".Lle_jump:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx\n\t"
	      /* jmp, but don't trust the assembler to choose the right jump */
	      ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	      ".Lle_fallthru:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx");

  if (offset_p)
    *offset_p = 20;
  if (size_p)
    *size_p = 4;
}

void
i386_emit_gt_goto (int *offset_p, int *size_p)
{
  EMIT_ASM32 (gt,
	      "cmpl %ebx,4(%esp)\n\t"
	      "jg .Lgt_jump\n\t"
	      "jne .Lgt_fallthru\n\t"
	      "cmpl %eax,(%esp)\n\t"
	      "jng .Lgt_fallthru\n\t"
	      ".Lgt_jump:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx\n\t"
	      /* jmp, but don't trust the assembler to choose the right jump */
	      ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	      ".Lgt_fallthru:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx");

  if (offset_p)
    *offset_p = 20;
  if (size_p)
    *size_p = 4;
}

void
i386_emit_ge_goto (int *offset_p, int *size_p)
{
  EMIT_ASM32 (ge,
	      "cmpl %ebx,4(%esp)\n\t"
	      "jge .Lge_jump\n\t"
	      "jne .Lge_fallthru\n\t"
	      "cmpl %eax,(%esp)\n\t"
	      "jnge .Lge_fallthru\n\t"
	      ".Lge_jump:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx\n\t"
	      /* jmp, but don't trust the assembler to choose the right jump */
	      ".byte 0xe9, 0x0, 0x0, 0x0, 0x0\n\t"
	      ".Lge_fallthru:\n\t"
	      "lea 0x8(%esp),%esp\n\t"
	      "pop %eax\n\t"
	      "pop %ebx");

  if (offset_p)
    *offset_p = 20;
  if (size_p)
    *size_p = 4;
}

struct emit_ops i386_emit_ops =
  {
    i386_emit_prologue,
    i386_emit_epilogue,
    i386_emit_add,
    i386_emit_sub,
    i386_emit_mul,
    i386_emit_lsh,
    i386_emit_rsh_signed,
    i386_emit_rsh_unsigned,
    i386_emit_ext,
    i386_emit_log_not,
    i386_emit_bit_and,
    i386_emit_bit_or,
    i386_emit_bit_xor,
    i386_emit_bit_not,
    i386_emit_equal,
    i386_emit_less_signed,
    i386_emit_less_unsigned,
    i386_emit_ref,
    i386_emit_if_goto,
    i386_emit_goto,
    i386_write_goto_address,
    i386_emit_const,
    i386_emit_call,
    i386_emit_reg,
    i386_emit_pop,
    i386_emit_stack_flush,
    i386_emit_zero_ext,
    i386_emit_swap,
    i386_emit_stack_adjust,
    i386_emit_int_call_1,
    i386_emit_void_call_2,
    i386_emit_eq_goto,
    i386_emit_ne_goto,
    i386_emit_lt_goto,
    i386_emit_le_goto,
    i386_emit_gt_goto,
    i386_emit_ge_goto
  };


static struct emit_ops *
x86_emit_ops (void)
{
#ifdef __x86_64__
  if (is_64bit_tdesc ())
    return &amd64_emit_ops;
  else
#endif
    return &i386_emit_ops;
}

/* Implementation of linux_target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
x86_sw_breakpoint_from_kind (int kind, int *size)
{
  *size = x86_breakpoint_len;
  return x86_breakpoint;
}

static int
x86_supports_range_stepping (void)
{
  return 1;
}

/* Implementation of linux_target_ops method "supports_hardware_single_step".
 */

static int
x86_supports_hardware_single_step (void)
{
  return 1;
}

static int
x86_get_ipa_tdesc_idx (void)
{
  struct regcache *regcache = get_thread_regcache (current_thread, 0);
  const struct target_desc *tdesc = regcache->tdesc;

#ifdef __x86_64__
  if (tdesc == tdesc_amd64_linux || tdesc == tdesc_amd64_linux_no_xml
      || tdesc == tdesc_x32_linux)
    return X86_TDESC_SSE;
  if (tdesc == tdesc_amd64_avx_linux || tdesc == tdesc_x32_avx_linux)
    return X86_TDESC_AVX;
  if (tdesc == tdesc_amd64_mpx_linux)
    return X86_TDESC_MPX;
  if (tdesc == tdesc_amd64_avx_mpx_linux)
    return X86_TDESC_AVX_MPX;
  if (tdesc == tdesc_amd64_avx_mpx_avx512_pku_linux || tdesc == tdesc_x32_avx_avx512_linux)
    return X86_TDESC_AVX_MPX_AVX512_PKU;
  if (tdesc == tdesc_amd64_avx_avx512_linux)
    return X86_TDESC_AVX_AVX512;
#endif

  if (tdesc == tdesc_i386_mmx_linux)
    return X86_TDESC_MMX;
  if (tdesc == tdesc_i386_linux || tdesc == tdesc_i386_linux_no_xml)
    return X86_TDESC_SSE;
  if (tdesc == tdesc_i386_avx_linux)
    return X86_TDESC_AVX;
  if (tdesc == tdesc_i386_mpx_linux)
    return X86_TDESC_MPX;
  if (tdesc == tdesc_i386_avx_mpx_linux)
    return X86_TDESC_AVX_MPX;
  if (tdesc == tdesc_i386_avx_mpx_avx512_pku_linux)
    return X86_TDESC_AVX_MPX_AVX512_PKU;
  if (tdesc == tdesc_i386_avx_avx512_linux)
    return X86_TDESC_AVX_AVX512;

  return 0;
}

/* This is initialized assuming an amd64 target.
   x86_arch_setup will correct it for i386 or amd64 targets.  */

struct linux_target_ops the_low_target =
{
  x86_arch_setup,
  x86_linux_regs_info,
  x86_cannot_fetch_register,
  x86_cannot_store_register,
  NULL, /* fetch_register */
  x86_get_pc,
  x86_set_pc,
  NULL, /* breakpoint_kind_from_pc */
  x86_sw_breakpoint_from_kind,
  NULL,
  1,
  x86_breakpoint_at,
  x86_supports_z_point_type,
  x86_insert_point,
  x86_remove_point,
  x86_stopped_by_watchpoint,
  x86_stopped_data_address,
  /* collect_ptrace_register/supply_ptrace_register are not needed in the
     native i386 case (no registers smaller than an xfer unit), and are not
     used in the biarch case (HAVE_LINUX_USRREGS is not defined).  */
  NULL,
  NULL,
  /* need to fix up i386 siginfo if host is amd64 */
  x86_siginfo_fixup,
  x86_linux_new_process,
  x86_linux_new_thread,
  x86_linux_new_fork,
  x86_linux_prepare_to_resume,
  x86_linux_process_qsupported,
  x86_supports_tracepoints,
  x86_get_thread_area,
  x86_install_fast_tracepoint_jump_pad,
  x86_emit_ops,
  x86_get_min_fast_tracepoint_insn_len,
  x86_supports_range_stepping,
  NULL, /* breakpoint_kind_from_current_state */
  x86_supports_hardware_single_step,
  x86_get_syscall_trapinfo,
  x86_get_ipa_tdesc_idx,
};

void
initialize_low_arch (void)
{
  /* Initialize the Linux target descriptions.  */
#ifdef __x86_64__
  init_registers_amd64_linux ();
  init_registers_amd64_avx_linux ();
  init_registers_amd64_mpx_linux ();
  init_registers_amd64_avx_mpx_linux ();
  init_registers_amd64_avx_avx512_linux ();
  init_registers_amd64_avx_mpx_avx512_pku_linux ();

  init_registers_x32_linux ();
  init_registers_x32_avx_linux ();
  init_registers_x32_avx_avx512_linux ();

  tdesc_amd64_linux_no_xml = XNEW (struct target_desc);
  copy_target_description (tdesc_amd64_linux_no_xml, tdesc_amd64_linux);
  tdesc_amd64_linux_no_xml->xmltarget = xmltarget_amd64_linux_no_xml;
#endif
  init_registers_i386_linux ();
  init_registers_i386_mmx_linux ();
  init_registers_i386_avx_linux ();
  init_registers_i386_mpx_linux ();
  init_registers_i386_avx_mpx_linux ();
  init_registers_i386_avx_avx512_linux ();
  init_registers_i386_avx_mpx_avx512_pku_linux ();

  tdesc_i386_linux_no_xml = XNEW (struct target_desc);
  copy_target_description (tdesc_i386_linux_no_xml, tdesc_i386_linux);
  tdesc_i386_linux_no_xml->xmltarget = xmltarget_i386_linux_no_xml;

  initialize_regsets_info (&x86_regsets_info);
}
