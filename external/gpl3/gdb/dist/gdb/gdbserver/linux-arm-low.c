/* GNU/Linux/ARM specific low level interface, for the remote server for GDB.
   Copyright (C) 1995-2013 Free Software Foundation, Inc.

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
#include "linux-low.h"

/* Don't include elf.h if linux/elf.h got included by gdb_proc_service.h.
   On Bionic elf.h and linux/elf.h have conflicting definitions.  */
#ifndef ELFMAG0
#include <elf.h>
#endif
#include <sys/ptrace.h>
#include <signal.h>

/* Defined in auto-generated files.  */
void init_registers_arm (void);
void init_registers_arm_with_iwmmxt (void);
void init_registers_arm_with_vfpv2 (void);
void init_registers_arm_with_vfpv3 (void);
void init_registers_arm_with_neon (void);

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 22
#endif

#ifndef PTRACE_GETWMMXREGS
# define PTRACE_GETWMMXREGS 18
# define PTRACE_SETWMMXREGS 19
#endif

#ifndef PTRACE_GETVFPREGS
# define PTRACE_GETVFPREGS 27
# define PTRACE_SETVFPREGS 28
#endif

#ifndef PTRACE_GETHBPREGS
#define PTRACE_GETHBPREGS 29
#define PTRACE_SETHBPREGS 30
#endif

/* Information describing the hardware breakpoint capabilities.  */
static struct
{
  unsigned char arch;
  unsigned char max_wp_length;
  unsigned char wp_count;
  unsigned char bp_count;
} arm_linux_hwbp_cap;

/* Enum describing the different types of ARM hardware break-/watch-points.  */
typedef enum
{
  arm_hwbp_break = 0,
  arm_hwbp_load = 1,
  arm_hwbp_store = 2,
  arm_hwbp_access = 3
} arm_hwbp_type;

/* Type describing an ARM Hardware Breakpoint Control register value.  */
typedef unsigned int arm_hwbp_control_t;

/* Structure used to keep track of hardware break-/watch-points.  */
struct arm_linux_hw_breakpoint
{
  /* Address to break on, or being watched.  */
  unsigned int address;
  /* Control register for break-/watch- point.  */
  arm_hwbp_control_t control;
};

/* Since we cannot dynamically allocate subfields of arch_process_info,
   assume a maximum number of supported break-/watchpoints.  */
#define MAX_BPTS 32
#define MAX_WPTS 32

/* Per-process arch-specific data we want to keep.  */
struct arch_process_info
{
  /* Hardware breakpoints for this process.  */
  struct arm_linux_hw_breakpoint bpts[MAX_BPTS];
  /* Hardware watchpoints for this process.  */
  struct arm_linux_hw_breakpoint wpts[MAX_WPTS];
};

/* Per-thread arch-specific data we want to keep.  */
struct arch_lwp_info
{
  /* Non-zero if our copy differs from what's recorded in the thread.  */
  char bpts_changed[MAX_BPTS];
  char wpts_changed[MAX_WPTS];
  /* Cached stopped data address.  */
  CORE_ADDR stopped_data_address;
};

static unsigned long arm_hwcap;

/* These are in <asm/elf.h> in current kernels.  */
#define HWCAP_VFP       64
#define HWCAP_IWMMXT    512
#define HWCAP_NEON      4096
#define HWCAP_VFPv3     8192
#define HWCAP_VFPv3D16  16384

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define arm_num_regs 26

static int arm_regmap[] = {
  0, 4, 8, 12, 16, 20, 24, 28,
  32, 36, 40, 44, 48, 52, 56, 60,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  64
};

static int
arm_cannot_store_register (int regno)
{
  return (regno >= arm_num_regs);
}

static int
arm_cannot_fetch_register (int regno)
{
  return (regno >= arm_num_regs);
}

static void
arm_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < arm_num_regs; i++)
    if (arm_regmap[i] != -1)
      collect_register (regcache, i, ((char *) buf) + arm_regmap[i]);
}

static void
arm_store_gregset (struct regcache *regcache, const void *buf)
{
  int i;
  char zerobuf[8];

  memset (zerobuf, 0, 8);
  for (i = 0; i < arm_num_regs; i++)
    if (arm_regmap[i] != -1)
      supply_register (regcache, i, ((char *) buf) + arm_regmap[i]);
    else
      supply_register (regcache, i, zerobuf);
}

static void
arm_fill_wmmxregset (struct regcache *regcache, void *buf)
{
  int i;

  if (!(arm_hwcap & HWCAP_IWMMXT))
    return;

  for (i = 0; i < 16; i++)
    collect_register (regcache, arm_num_regs + i, (char *) buf + i * 8);

  /* We only have access to wcssf, wcasf, and wcgr0-wcgr3.  */
  for (i = 0; i < 6; i++)
    collect_register (regcache, arm_num_regs + i + 16,
		      (char *) buf + 16 * 8 + i * 4);
}

static void
arm_store_wmmxregset (struct regcache *regcache, const void *buf)
{
  int i;

  if (!(arm_hwcap & HWCAP_IWMMXT))
    return;

  for (i = 0; i < 16; i++)
    supply_register (regcache, arm_num_regs + i, (char *) buf + i * 8);

  /* We only have access to wcssf, wcasf, and wcgr0-wcgr3.  */
  for (i = 0; i < 6; i++)
    supply_register (regcache, arm_num_regs + i + 16,
		     (char *) buf + 16 * 8 + i * 4);
}

static void
arm_fill_vfpregset (struct regcache *regcache, void *buf)
{
  int i, num, base;

  if (!(arm_hwcap & HWCAP_VFP))
    return;

  if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
    num = 32;
  else
    num = 16;

  base = find_regno ("d0");
  for (i = 0; i < num; i++)
    collect_register (regcache, base + i, (char *) buf + i * 8);

  collect_register_by_name (regcache, "fpscr", (char *) buf + 32 * 8);
}

static void
arm_store_vfpregset (struct regcache *regcache, const void *buf)
{
  int i, num, base;

  if (!(arm_hwcap & HWCAP_VFP))
    return;

  if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
    num = 32;
  else
    num = 16;

  base = find_regno ("d0");
  for (i = 0; i < num; i++)
    supply_register (regcache, base + i, (char *) buf + i * 8);

  supply_register_by_name (regcache, "fpscr", (char *) buf + 32 * 8);
}

extern int debug_threads;

static CORE_ADDR
arm_get_pc (struct regcache *regcache)
{
  unsigned long pc;
  collect_register_by_name (regcache, "pc", &pc);
  if (debug_threads)
    fprintf (stderr, "stop pc is %08lx\n", pc);
  return pc;
}

static void
arm_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  unsigned long newpc = pc;
  supply_register_by_name (regcache, "pc", &newpc);
}

/* Correct in either endianness.  */
static const unsigned long arm_breakpoint = 0xef9f0001;
#define arm_breakpoint_len 4
static const unsigned short thumb_breakpoint = 0xde01;
static const unsigned short thumb2_breakpoint[] = { 0xf7f0, 0xa000 };

/* For new EABI binaries.  We recognize it regardless of which ABI
   is used for gdbserver, so single threaded debugging should work
   OK, but for multi-threaded debugging we only insert the current
   ABI's breakpoint instruction.  For now at least.  */
static const unsigned long arm_eabi_breakpoint = 0xe7f001f0;

static int
arm_breakpoint_at (CORE_ADDR where)
{
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);
  unsigned long cpsr;

  collect_register_by_name (regcache, "cpsr", &cpsr);

  if (cpsr & 0x20)
    {
      /* Thumb mode.  */
      unsigned short insn;

      (*the_target->read_memory) (where, (unsigned char *) &insn, 2);
      if (insn == thumb_breakpoint)
	return 1;

      if (insn == thumb2_breakpoint[0])
	{
	  (*the_target->read_memory) (where + 2, (unsigned char *) &insn, 2);
	  if (insn == thumb2_breakpoint[1])
	    return 1;
	}
    }
  else
    {
      /* ARM mode.  */
      unsigned long insn;

      (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
      if (insn == arm_breakpoint)
	return 1;

      if (insn == arm_eabi_breakpoint)
	return 1;
    }

  return 0;
}

/* We only place breakpoints in empty marker functions, and thread locking
   is outside of the function.  So rather than importing software single-step,
   we can just run until exit.  */
static CORE_ADDR
arm_reinsert_addr (void)
{
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);
  unsigned long pc;
  collect_register_by_name (regcache, "lr", &pc);
  return pc;
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *)*base - idx);

  return PS_OK;
}


/* Query Hardware Breakpoint information for the target we are attached to
   (using PID as ptrace argument) and set up arm_linux_hwbp_cap.  */
static void
arm_linux_init_hwbp_cap (int pid)
{
  unsigned int val;

  if (ptrace (PTRACE_GETHBPREGS, pid, 0, &val) < 0)
    return;

  arm_linux_hwbp_cap.arch = (unsigned char)((val >> 24) & 0xff);
  if (arm_linux_hwbp_cap.arch == 0)
    return;

  arm_linux_hwbp_cap.max_wp_length = (unsigned char)((val >> 16) & 0xff);
  arm_linux_hwbp_cap.wp_count = (unsigned char)((val >> 8) & 0xff);
  arm_linux_hwbp_cap.bp_count = (unsigned char)(val & 0xff);

  if (arm_linux_hwbp_cap.wp_count > MAX_WPTS)
    internal_error (__FILE__, __LINE__, "Unsupported number of watchpoints");
  if (arm_linux_hwbp_cap.bp_count > MAX_BPTS)
    internal_error (__FILE__, __LINE__, "Unsupported number of breakpoints");
}

/* How many hardware breakpoints are available?  */
static int
arm_linux_get_hw_breakpoint_count (void)
{
  return arm_linux_hwbp_cap.bp_count;
}

/* How many hardware watchpoints are available?  */
static int
arm_linux_get_hw_watchpoint_count (void)
{
  return arm_linux_hwbp_cap.wp_count;
}

/* Maximum length of area watched by hardware watchpoint.  */
static int
arm_linux_get_hw_watchpoint_max_length (void)
{
  return arm_linux_hwbp_cap.max_wp_length;
}

/* Initialize an ARM hardware break-/watch-point control register value.
   BYTE_ADDRESS_SELECT is the mask of bytes to trigger on; HWBP_TYPE is the
   type of break-/watch-point; ENABLE indicates whether the point is enabled.
   */
static arm_hwbp_control_t
arm_hwbp_control_initialize (unsigned byte_address_select,
			     arm_hwbp_type hwbp_type,
			     int enable)
{
  gdb_assert ((byte_address_select & ~0xffU) == 0);
  gdb_assert (hwbp_type != arm_hwbp_break
	      || ((byte_address_select & 0xfU) != 0));

  return (byte_address_select << 5) | (hwbp_type << 3) | (3 << 1) | enable;
}

/* Does the breakpoint control value CONTROL have the enable bit set?  */
static int
arm_hwbp_control_is_enabled (arm_hwbp_control_t control)
{
  return control & 0x1;
}

/* Is the breakpoint control value CONTROL initialized?  */
static int
arm_hwbp_control_is_initialized (arm_hwbp_control_t control)
{
  return control != 0;
}

/* Change a breakpoint control word so that it is in the disabled state.  */
static arm_hwbp_control_t
arm_hwbp_control_disable (arm_hwbp_control_t control)
{
  return control & ~0x1;
}

/* Are two break-/watch-points equal?  */
static int
arm_linux_hw_breakpoint_equal (const struct arm_linux_hw_breakpoint *p1,
			       const struct arm_linux_hw_breakpoint *p2)
{
  return p1->address == p2->address && p1->control == p2->control;
}

/* Initialize the hardware breakpoint structure P for a breakpoint or
   watchpoint at ADDR to LEN.  The type of watchpoint is given in TYPE.
   Returns -1 if TYPE is unsupported, or -2 if the particular combination
   of ADDR and LEN cannot be implemented.  Otherwise, returns 0 if TYPE
   represents a breakpoint and 1 if type represents a watchpoint.  */
static int
arm_linux_hw_point_initialize (char type, CORE_ADDR addr, int len,
			       struct arm_linux_hw_breakpoint *p)
{
  arm_hwbp_type hwbp_type;
  unsigned mask;

  /* Breakpoint/watchpoint types (GDB terminology):
     0 = memory breakpoint for instructions
     (not supported; done via memory write instead)
     1 = hardware breakpoint for instructions (supported)
     2 = write watchpoint (supported)
     3 = read watchpoint (supported)
     4 = access watchpoint (supported).  */
  switch (type)
    {
    case '1':
      hwbp_type = arm_hwbp_break;
      break;
    case '2':
      hwbp_type = arm_hwbp_store;
      break;
    case '3':
      hwbp_type = arm_hwbp_load;
      break;
    case '4':
      hwbp_type = arm_hwbp_access;
      break;
    default:
      /* Unsupported.  */
      return -1;
    }

  if (hwbp_type == arm_hwbp_break)
    {
      /* For breakpoints, the length field encodes the mode.  */
      switch (len)
	{
	case 2:	 /* 16-bit Thumb mode breakpoint */
	case 3:  /* 32-bit Thumb mode breakpoint */
	  mask = 0x3;
	  addr &= ~1;
	  break;
	case 4:  /* 32-bit ARM mode breakpoint */
	  mask = 0xf;
	  addr &= ~3;
	  break;
	default:
	  /* Unsupported. */
	  return -2;
	}
    }
  else
    {
      CORE_ADDR max_wp_length = arm_linux_get_hw_watchpoint_max_length ();
      CORE_ADDR aligned_addr;

      /* Can not set watchpoints for zero or negative lengths.  */
      if (len <= 0)
	return -2;
      /* The current ptrace interface can only handle watchpoints that are a
	 power of 2.  */
      if ((len & (len - 1)) != 0)
	return -2;

      /* Test that the range [ADDR, ADDR + LEN) fits into the largest address
	 range covered by a watchpoint.  */
      aligned_addr = addr & ~(max_wp_length - 1);
      if (aligned_addr + max_wp_length < addr + len)
	return -2;

      mask = (1 << len) - 1;
    }

  p->address = (unsigned int) addr;
  p->control = arm_hwbp_control_initialize (mask, hwbp_type, 1);

  return hwbp_type != arm_hwbp_break;
}

/* Callback to mark a watch-/breakpoint to be updated in all threads of
   the current process.  */

struct update_registers_data
{
  int watch;
  int i;
};

static int
update_registers_callback (struct inferior_list_entry *entry, void *arg)
{
  struct lwp_info *lwp = (struct lwp_info *) entry;
  struct update_registers_data *data = (struct update_registers_data *) arg;

  /* Only update the threads of the current process.  */
  if (pid_of (lwp) == pid_of (get_thread_lwp (current_inferior)))
    {
      /* The actual update is done later just before resuming the lwp,
         we just mark that the registers need updating.  */
      if (data->watch)
	lwp->arch_private->wpts_changed[data->i] = 1;
      else
	lwp->arch_private->bpts_changed[data->i] = 1;

      /* If the lwp isn't stopped, force it to momentarily pause, so
         we can update its breakpoint registers.  */
      if (!lwp->stopped)
        linux_stop_lwp (lwp);
    }

  return 0;
}

/* Insert hardware break-/watchpoint.  */
static int
arm_insert_point (char type, CORE_ADDR addr, int len)
{
  struct process_info *proc = current_process ();
  struct arm_linux_hw_breakpoint p, *pts;
  int watch, i, count;

  watch = arm_linux_hw_point_initialize (type, addr, len, &p);
  if (watch < 0)
    {
      /* Unsupported.  */
      return watch == -1 ? 1 : -1;
    }

  if (watch)
    {
      count = arm_linux_get_hw_watchpoint_count ();
      pts = proc->private->arch_private->wpts;
    }
  else
    {
      count = arm_linux_get_hw_breakpoint_count ();
      pts = proc->private->arch_private->bpts;
    }

  for (i = 0; i < count; i++)
    if (!arm_hwbp_control_is_enabled (pts[i].control))
      {
	struct update_registers_data data = { watch, i };
	pts[i] = p;
	find_inferior (&all_lwps, update_registers_callback, &data);
	return 0;
      }

  /* We're out of watchpoints.  */
  return -1;
}

/* Remove hardware break-/watchpoint.  */
static int
arm_remove_point (char type, CORE_ADDR addr, int len)
{
  struct process_info *proc = current_process ();
  struct arm_linux_hw_breakpoint p, *pts;
  int watch, i, count;

  watch = arm_linux_hw_point_initialize (type, addr, len, &p);
  if (watch < 0)
    {
      /* Unsupported.  */
      return -1;
    }

  if (watch)
    {
      count = arm_linux_get_hw_watchpoint_count ();
      pts = proc->private->arch_private->wpts;
    }
  else
    {
      count = arm_linux_get_hw_breakpoint_count ();
      pts = proc->private->arch_private->bpts;
    }

  for (i = 0; i < count; i++)
    if (arm_linux_hw_breakpoint_equal (&p, pts + i))
      {
	struct update_registers_data data = { watch, i };
	pts[i].control = arm_hwbp_control_disable (pts[i].control);
	find_inferior (&all_lwps, update_registers_callback, &data);
	return 0;
      }

  /* No watchpoint matched.  */
  return -1;
}

/* Return whether current thread is stopped due to a watchpoint.  */
static int
arm_stopped_by_watchpoint (void)
{
  struct lwp_info *lwp = get_thread_lwp (current_inferior);
  siginfo_t siginfo;

  /* We must be able to set hardware watchpoints.  */
  if (arm_linux_get_hw_watchpoint_count () == 0)
    return 0;

  /* Retrieve siginfo.  */
  errno = 0;
  ptrace (PTRACE_GETSIGINFO, lwpid_of (lwp), 0, &siginfo);
  if (errno != 0)
    return 0;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return 0;

  /* If we are in a positive slot then we're looking at a breakpoint and not
     a watchpoint.  */
  if (siginfo.si_errno >= 0)
    return 0;

  /* Cache stopped data address for use by arm_stopped_data_address.  */
  lwp->arch_private->stopped_data_address
    = (CORE_ADDR) (uintptr_t) siginfo.si_addr;

  return 1;
}

/* Return data address that triggered watchpoint.  Called only if
   arm_stopped_by_watchpoint returned true.  */
static CORE_ADDR
arm_stopped_data_address (void)
{
  struct lwp_info *lwp = get_thread_lwp (current_inferior);
  return lwp->arch_private->stopped_data_address;
}

/* Called when a new process is created.  */
static struct arch_process_info *
arm_new_process (void)
{
  struct arch_process_info *info = xcalloc (1, sizeof (*info));
  return info;
}

/* Called when a new thread is detected.  */
static struct arch_lwp_info *
arm_new_thread (void)
{
  struct arch_lwp_info *info = xcalloc (1, sizeof (*info));
  int i;

  for (i = 0; i < MAX_BPTS; i++)
    info->bpts_changed[i] = 1;
  for (i = 0; i < MAX_WPTS; i++)
    info->wpts_changed[i] = 1;

  return info;
}

/* Called when resuming a thread.
   If the debug regs have changed, update the thread's copies.  */
static void
arm_prepare_to_resume (struct lwp_info *lwp)
{
  int pid = lwpid_of (lwp);
  struct process_info *proc = find_process_pid (pid_of (lwp));
  struct arch_process_info *proc_info = proc->private->arch_private;
  struct arch_lwp_info *lwp_info = lwp->arch_private;
  int i;

  for (i = 0; i < arm_linux_get_hw_breakpoint_count (); i++)
    if (lwp_info->bpts_changed[i])
      {
	errno = 0;

	if (arm_hwbp_control_is_enabled (proc_info->bpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
		      (PTRACE_ARG3_TYPE) ((i << 1) + 1),
		      &proc_info->bpts[i].address) < 0)
	    perror_with_name ("Unexpected error setting breakpoint address");

	if (arm_hwbp_control_is_initialized (proc_info->bpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
		      (PTRACE_ARG3_TYPE) ((i << 1) + 2),
		      &proc_info->bpts[i].control) < 0)
	    perror_with_name ("Unexpected error setting breakpoint");

	lwp_info->bpts_changed[i] = 0;
      }

  for (i = 0; i < arm_linux_get_hw_watchpoint_count (); i++)
    if (lwp_info->wpts_changed[i])
      {
	errno = 0;

	if (arm_hwbp_control_is_enabled (proc_info->wpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
		      (PTRACE_ARG3_TYPE) -((i << 1) + 1),
		      &proc_info->wpts[i].address) < 0)
	    perror_with_name ("Unexpected error setting watchpoint address");

	if (arm_hwbp_control_is_initialized (proc_info->wpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
		      (PTRACE_ARG3_TYPE) -((i << 1) + 2),
		      &proc_info->wpts[i].control) < 0)
	    perror_with_name ("Unexpected error setting watchpoint");

	lwp_info->wpts_changed[i] = 0;
      }
}


static int
arm_get_hwcap (unsigned long *valp)
{
  unsigned char *data = alloca (8);
  int offset = 0;

  while ((*the_target->read_auxv) (offset, data, 8) == 8)
    {
      unsigned int *data_p = (unsigned int *)data;
      if (data_p[0] == AT_HWCAP)
	{
	  *valp = data_p[1];
	  return 1;
	}

      offset += 8;
    }

  *valp = 0;
  return 0;
}

static void
arm_arch_setup (void)
{
  int pid = lwpid_of (get_thread_lwp (current_inferior));

  /* Query hardware watchpoint/breakpoint capabilities.  */
  arm_linux_init_hwbp_cap (pid);

  arm_hwcap = 0;
  if (arm_get_hwcap (&arm_hwcap) == 0)
    {
      init_registers_arm ();
      return;
    }

  if (arm_hwcap & HWCAP_IWMMXT)
    {
      init_registers_arm_with_iwmmxt ();
      return;
    }

  if (arm_hwcap & HWCAP_VFP)
    {
      char *buf;

      /* NEON implies either no VFP, or VFPv3-D32.  We only support
	 it with VFP.  */
      if (arm_hwcap & HWCAP_NEON)
	init_registers_arm_with_neon ();
      else if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
	init_registers_arm_with_vfpv3 ();
      else
	init_registers_arm_with_vfpv2 ();

      /* Now make sure that the kernel supports reading these
	 registers.  Support was added in 2.6.30.  */
      errno = 0;
      buf = xmalloc (32 * 8 + 4);
      if (ptrace (PTRACE_GETVFPREGS, pid, 0, buf) < 0
	  && errno == EIO)
	{
	  arm_hwcap = 0;
	  init_registers_arm ();
	}
      free (buf);

      return;
    }

  /* The default configuration uses legacy FPA registers, probably
     simulated.  */
  init_registers_arm ();
}

struct regset_info target_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, 18 * 4,
    GENERAL_REGS,
    arm_fill_gregset, arm_store_gregset },
  { PTRACE_GETWMMXREGS, PTRACE_SETWMMXREGS, 0, 16 * 8 + 6 * 4,
    EXTENDED_REGS,
    arm_fill_wmmxregset, arm_store_wmmxregset },
  { PTRACE_GETVFPREGS, PTRACE_SETVFPREGS, 0, 32 * 8 + 4,
    EXTENDED_REGS,
    arm_fill_vfpregset, arm_store_vfpregset },
  { 0, 0, 0, -1, -1, NULL, NULL }
};

struct linux_target_ops the_low_target = {
  arm_arch_setup,
  arm_num_regs,
  arm_regmap,
  NULL,
  arm_cannot_fetch_register,
  arm_cannot_store_register,
  NULL, /* fetch_register */
  arm_get_pc,
  arm_set_pc,

  /* Define an ARM-mode breakpoint; we only set breakpoints in the C
     library, which is most likely to be ARM.  If the kernel supports
     clone events, we will never insert a breakpoint, so even a Thumb
     C library will work; so will mixing EABI/non-EABI gdbserver and
     application.  */
#ifndef __ARM_EABI__
  (const unsigned char *) &arm_breakpoint,
#else
  (const unsigned char *) &arm_eabi_breakpoint,
#endif
  arm_breakpoint_len,
  arm_reinsert_addr,
  0,
  arm_breakpoint_at,
  arm_insert_point,
  arm_remove_point,
  arm_stopped_by_watchpoint,
  arm_stopped_data_address,
  NULL, /* collect_ptrace_register */
  NULL, /* supply_ptrace_register */
  NULL, /* siginfo_fixup */
  arm_new_process,
  arm_new_thread,
  arm_prepare_to_resume,
};
