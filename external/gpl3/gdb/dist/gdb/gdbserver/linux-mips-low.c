/* GNU/Linux/MIPS specific low level interface, for the remote server for GDB.
   Copyright (C) 1995-2016 Free Software Foundation, Inc.

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

#include "nat/gdb_ptrace.h"
#include <endian.h>

#include "nat/mips-linux-watch.h"
#include "gdb_proc_service.h"

/* Defined in auto-generated file mips-linux.c.  */
void init_registers_mips_linux (void);
extern const struct target_desc *tdesc_mips_linux;

/* Defined in auto-generated file mips-dsp-linux.c.  */
void init_registers_mips_dsp_linux (void);
extern const struct target_desc *tdesc_mips_dsp_linux;

/* Defined in auto-generated file mips64-linux.c.  */
void init_registers_mips64_linux (void);
extern const struct target_desc *tdesc_mips64_linux;

/* Defined in auto-generated file mips64-dsp-linux.c.  */
void init_registers_mips64_dsp_linux (void);
extern const struct target_desc *tdesc_mips64_dsp_linux;

#ifdef __mips64
#define tdesc_mips_linux tdesc_mips64_linux
#define tdesc_mips_dsp_linux tdesc_mips64_dsp_linux
#endif

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define mips_num_regs 73
#define mips_dsp_num_regs 80

#include <asm/ptrace.h>

#ifndef DSP_BASE
#define DSP_BASE 71
#define DSP_CONTROL 77
#endif

union mips_register
{
  unsigned char buf[8];

  /* Deliberately signed, for proper sign extension.  */
  int reg32;
  long long reg64;
};

/* Return the ptrace ``address'' of register REGNO. */

#define mips_base_regs							\
  -1,  1,  2,  3,  4,  5,  6,  7,					\
  8,  9,  10, 11, 12, 13, 14, 15,					\
  16, 17, 18, 19, 20, 21, 22, 23,					\
  24, 25, 26, 27, 28, 29, 30, 31,					\
									\
  -1, MMLO, MMHI, BADVADDR, CAUSE, PC,					\
									\
  FPR_BASE,      FPR_BASE + 1,  FPR_BASE + 2,  FPR_BASE + 3,		\
  FPR_BASE + 4,  FPR_BASE + 5,  FPR_BASE + 6,  FPR_BASE + 7,		\
  FPR_BASE + 8,  FPR_BASE + 9,  FPR_BASE + 10, FPR_BASE + 11,		\
  FPR_BASE + 12, FPR_BASE + 13, FPR_BASE + 14, FPR_BASE + 15,		\
  FPR_BASE + 16, FPR_BASE + 17, FPR_BASE + 18, FPR_BASE + 19,		\
  FPR_BASE + 20, FPR_BASE + 21, FPR_BASE + 22, FPR_BASE + 23,		\
  FPR_BASE + 24, FPR_BASE + 25, FPR_BASE + 26, FPR_BASE + 27,		\
  FPR_BASE + 28, FPR_BASE + 29, FPR_BASE + 30, FPR_BASE + 31,		\
  FPC_CSR, FPC_EIR

#define mips_dsp_regs							\
  DSP_BASE,      DSP_BASE + 1,  DSP_BASE + 2,  DSP_BASE + 3,		\
  DSP_BASE + 4,  DSP_BASE + 5,						\
  DSP_CONTROL

static int mips_regmap[mips_num_regs] = {
  mips_base_regs,
  0
};

static int mips_dsp_regmap[mips_dsp_num_regs] = {
  mips_base_regs,
  mips_dsp_regs,
  0
};

/* DSP registers are not in any regset and can only be accessed
   individually.  */

static unsigned char mips_dsp_regset_bitmap[(mips_dsp_num_regs + 7) / 8] = {
  0xfe, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0x80
};

static int have_dsp = -1;

/* Try peeking at an arbitrarily chosen DSP register and pick the available
   user register set accordingly.  */

static const struct target_desc *
mips_read_description (void)
{
  if (have_dsp < 0)
    {
      int pid = lwpid_of (current_thread);

      errno = 0;
      ptrace (PTRACE_PEEKUSER, pid, DSP_CONTROL, 0);
      switch (errno)
	{
	case 0:
	  have_dsp = 1;
	  break;
	case EIO:
	  have_dsp = 0;
	  break;
	default:
	  perror_with_name ("ptrace");
	  break;
	}
    }

  return have_dsp ? tdesc_mips_dsp_linux : tdesc_mips_linux;
}

static void
mips_arch_setup (void)
{
  current_process ()->tdesc = mips_read_description ();
}

static struct usrregs_info *
get_usrregs_info (void)
{
  const struct regs_info *regs_info = the_low_target.regs_info ();

  return regs_info->usrregs;
}

/* Per-process arch-specific data we want to keep.  */

struct arch_process_info
{
  /* -1 if the kernel and/or CPU do not support watch registers.
      1 if watch_readback is valid and we can read style, num_valid
	and the masks.
      0 if we need to read the watch_readback.  */

  int watch_readback_valid;

  /* Cached watch register read values.  */

  struct pt_watch_regs watch_readback;

  /* Current watchpoint requests for this process.  */

  struct mips_watchpoint *current_watches;

  /* The current set of watch register values for writing the
     registers.  */

  struct pt_watch_regs watch_mirror;
};

/* Per-thread arch-specific data we want to keep.  */

struct arch_lwp_info
{
  /* Non-zero if our copy differs from what's recorded in the thread.  */
  int watch_registers_changed;
};

/* From mips-linux-nat.c.  */

/* Pseudo registers can not be read.  ptrace does not provide a way to
   read (or set) PS_REGNUM, and there's no point in reading or setting
   ZERO_REGNUM.  We also can not set BADVADDR, CAUSE, or FCRIR via
   ptrace().  */

static int
mips_cannot_fetch_register (int regno)
{
  const struct target_desc *tdesc;

  if (get_usrregs_info ()->regmap[regno] == -1)
    return 1;

  tdesc = current_process ()->tdesc;

  if (find_regno (tdesc, "r0") == regno)
    return 1;

  return 0;
}

static int
mips_cannot_store_register (int regno)
{
  const struct target_desc *tdesc;

  if (get_usrregs_info ()->regmap[regno] == -1)
    return 1;

  tdesc = current_process ()->tdesc;

  if (find_regno (tdesc, "r0") == regno)
    return 1;

  if (find_regno (tdesc, "cause") == regno)
    return 1;

  if (find_regno (tdesc, "badvaddr") == regno)
    return 1;

  if (find_regno (tdesc, "fir") == regno)
    return 1;

  return 0;
}

static CORE_ADDR
mips_get_pc (struct regcache *regcache)
{
  union mips_register pc;
  collect_register_by_name (regcache, "pc", pc.buf);
  return register_size (regcache->tdesc, 0) == 4 ? pc.reg32 : pc.reg64;
}

static void
mips_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  union mips_register newpc;
  if (register_size (regcache->tdesc, 0) == 4)
    newpc.reg32 = pc;
  else
    newpc.reg64 = pc;

  supply_register_by_name (regcache, "pc", newpc.buf);
}

/* Correct in either endianness.  */
static const unsigned int mips_breakpoint = 0x0005000d;
#define mips_breakpoint_len 4

/* Implementation of linux_target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
mips_sw_breakpoint_from_kind (int kind, int *size)
{
  *size = mips_breakpoint_len;
  return (const gdb_byte *) &mips_breakpoint;
}

static int
mips_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
  if (insn == mips_breakpoint)
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return 0;
}

/* Mark the watch registers of lwp, represented by ENTRY, as changed,
   if the lwp's process id is *PID_P.  */

static int
update_watch_registers_callback (struct inferior_list_entry *entry,
				 void *pid_p)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);
  int pid = *(int *) pid_p;

  /* Only update the threads of this process.  */
  if (pid_of (thread) == pid)
    {
      /* The actual update is done later just before resuming the lwp,
	 we just mark that the registers need updating.  */
      lwp->arch_private->watch_registers_changed = 1;

      /* If the lwp isn't stopped, force it to momentarily pause, so
	 we can update its watch registers.  */
      if (!lwp->stopped)
	linux_stop_lwp (lwp);
    }

  return 0;
}

/* This is the implementation of linux_target_ops method
   new_process.  */

static struct arch_process_info *
mips_linux_new_process (void)
{
  struct arch_process_info *info = XCNEW (struct arch_process_info);

  return info;
}

/* This is the implementation of linux_target_ops method new_thread.
   Mark the watch registers as changed, so the threads' copies will
   be updated.  */

static void
mips_linux_new_thread (struct lwp_info *lwp)
{
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);

  info->watch_registers_changed = 1;

  lwp->arch_private = info;
}

/* Create a new mips_watchpoint and add it to the list.  */

static void
mips_add_watchpoint (struct arch_process_info *priv, CORE_ADDR addr, int len,
		     enum target_hw_bp_type watch_type)
{
  struct mips_watchpoint *new_watch;
  struct mips_watchpoint **pw;

  new_watch = XNEW (struct mips_watchpoint);
  new_watch->addr = addr;
  new_watch->len = len;
  new_watch->type = watch_type;
  new_watch->next = NULL;

  pw = &priv->current_watches;
  while (*pw != NULL)
    pw = &(*pw)->next;
  *pw = new_watch;
}

/* Hook to call when a new fork is attached.  */

static void
mips_linux_new_fork (struct process_info *parent,
			struct process_info *child)
{
  struct arch_process_info *parent_private;
  struct arch_process_info *child_private;
  struct mips_watchpoint *wp;

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

  parent_private = parent->priv->arch_private;
  child_private = child->priv->arch_private;

  child_private->watch_readback_valid = parent_private->watch_readback_valid;
  child_private->watch_readback = parent_private->watch_readback;

  for (wp = parent_private->current_watches; wp != NULL; wp = wp->next)
    mips_add_watchpoint (child_private, wp->addr, wp->len, wp->type);

  child_private->watch_mirror = parent_private->watch_mirror;
}
/* This is the implementation of linux_target_ops method
   prepare_to_resume.  If the watch regs have changed, update the
   thread's copies.  */

static void
mips_linux_prepare_to_resume (struct lwp_info *lwp)
{
  ptid_t ptid = ptid_of (get_lwp_thread (lwp));
  struct process_info *proc = find_process_pid (ptid_get_pid (ptid));
  struct arch_process_info *priv = proc->priv->arch_private;

  if (lwp->arch_private->watch_registers_changed)
    {
      /* Only update the watch registers if we have set or unset a
	 watchpoint already.  */
      if (mips_linux_watch_get_num_valid (&priv->watch_mirror) > 0)
	{
	  /* Write the mirrored watch register values.  */
	  int tid = ptid_get_lwp (ptid);

	  if (-1 == ptrace (PTRACE_SET_WATCH_REGS, tid,
			    &priv->watch_mirror, NULL))
	    perror_with_name ("Couldn't write watch register");
	}

      lwp->arch_private->watch_registers_changed = 0;
    }
}

static int
mips_supports_z_point_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_WRITE_WP:
    case Z_PACKET_READ_WP:
    case Z_PACKET_ACCESS_WP:
      return 1;
    default:
      return 0;
    }
}

/* This is the implementation of linux_target_ops method
   insert_point.  */

static int
mips_insert_point (enum raw_bkpt_type type, CORE_ADDR addr,
		   int len, struct raw_breakpoint *bp)
{
  struct process_info *proc = current_process ();
  struct arch_process_info *priv = proc->priv->arch_private;
  struct pt_watch_regs regs;
  int pid;
  long lwpid;
  enum target_hw_bp_type watch_type;
  uint32_t irw;

  lwpid = lwpid_of (current_thread);
  if (!mips_linux_read_watch_registers (lwpid,
					&priv->watch_readback,
					&priv->watch_readback_valid,
					0))
    return -1;

  if (len <= 0)
    return -1;

  regs = priv->watch_readback;
  /* Add the current watches.  */
  mips_linux_watch_populate_regs (priv->current_watches, &regs);

  /* Now try to add the new watch.  */
  watch_type = raw_bkpt_type_to_target_hw_bp_type (type);
  irw = mips_linux_watch_type_to_irw (watch_type);
  if (!mips_linux_watch_try_one_watch (&regs, addr, len, irw))
    return -1;

  /* It fit.  Stick it on the end of the list.  */
  mips_add_watchpoint (priv, addr, len, watch_type);

  priv->watch_mirror = regs;

  /* Only update the threads of this process.  */
  pid = pid_of (proc);
  find_inferior (&all_threads, update_watch_registers_callback, &pid);

  return 0;
}

/* This is the implementation of linux_target_ops method
   remove_point.  */

static int
mips_remove_point (enum raw_bkpt_type type, CORE_ADDR addr,
		   int len, struct raw_breakpoint *bp)
{
  struct process_info *proc = current_process ();
  struct arch_process_info *priv = proc->priv->arch_private;

  int deleted_one;
  int pid;
  enum target_hw_bp_type watch_type;

  struct mips_watchpoint **pw;
  struct mips_watchpoint *w;

  /* Search for a known watch that matches.  Then unlink and free it.  */
  watch_type = raw_bkpt_type_to_target_hw_bp_type (type);
  deleted_one = 0;
  pw = &priv->current_watches;
  while ((w = *pw))
    {
      if (w->addr == addr && w->len == len && w->type == watch_type)
	{
	  *pw = w->next;
	  free (w);
	  deleted_one = 1;
	  break;
	}
      pw = &(w->next);
    }

  if (!deleted_one)
    return -1;  /* We don't know about it, fail doing nothing.  */

  /* At this point watch_readback is known to be valid because we
     could not have added the watch without reading it.  */
  gdb_assert (priv->watch_readback_valid == 1);

  priv->watch_mirror = priv->watch_readback;
  mips_linux_watch_populate_regs (priv->current_watches,
				  &priv->watch_mirror);

  /* Only update the threads of this process.  */
  pid = pid_of (proc);
  find_inferior (&all_threads, update_watch_registers_callback, &pid);
  return 0;
}

/* This is the implementation of linux_target_ops method
   stopped_by_watchpoint.  The watchhi R and W bits indicate
   the watch register triggered. */

static int
mips_stopped_by_watchpoint (void)
{
  struct process_info *proc = current_process ();
  struct arch_process_info *priv = proc->priv->arch_private;
  int n;
  int num_valid;
  long lwpid = lwpid_of (current_thread);

  if (!mips_linux_read_watch_registers (lwpid,
					&priv->watch_readback,
					&priv->watch_readback_valid,
					1))
    return 0;

  num_valid = mips_linux_watch_get_num_valid (&priv->watch_readback);

  for (n = 0; n < MAX_DEBUG_REGISTER && n < num_valid; n++)
    if (mips_linux_watch_get_watchhi (&priv->watch_readback, n)
	& (R_MASK | W_MASK))
      return 1;

  return 0;
}

/* This is the implementation of linux_target_ops method
   stopped_data_address.  */

static CORE_ADDR
mips_stopped_data_address (void)
{
  struct process_info *proc = current_process ();
  struct arch_process_info *priv = proc->priv->arch_private;
  int n;
  int num_valid;
  long lwpid = lwpid_of (current_thread);

  /* On MIPS we don't know the low order 3 bits of the data address.
     GDB does not support remote targets that can't report the
     watchpoint address.  So, make our best guess; return the starting
     address of a watchpoint request which overlaps the one that
     triggered.  */

  if (!mips_linux_read_watch_registers (lwpid,
					&priv->watch_readback,
					&priv->watch_readback_valid,
					0))
    return 0;

  num_valid = mips_linux_watch_get_num_valid (&priv->watch_readback);

  for (n = 0; n < MAX_DEBUG_REGISTER && n < num_valid; n++)
    if (mips_linux_watch_get_watchhi (&priv->watch_readback, n)
	& (R_MASK | W_MASK))
      {
	CORE_ADDR t_low, t_hi;
	int t_irw;
	struct mips_watchpoint *watch;

	t_low = mips_linux_watch_get_watchlo (&priv->watch_readback, n);
	t_irw = t_low & IRW_MASK;
	t_hi = (mips_linux_watch_get_watchhi (&priv->watch_readback, n)
		| IRW_MASK);
	t_low &= ~(CORE_ADDR)t_hi;

	for (watch = priv->current_watches;
	     watch != NULL;
	     watch = watch->next)
	  {
	    CORE_ADDR addr = watch->addr;
	    CORE_ADDR last_byte = addr + watch->len - 1;

	    if ((t_irw & mips_linux_watch_type_to_irw (watch->type)) == 0)
	      {
		/* Different type.  */
		continue;
	      }
	    /* Check for overlap of even a single byte.  */
	    if (last_byte >= t_low && addr <= t_low + t_hi)
	      return addr;
	  }
      }

  /* Shouldn't happen.  */
  return 0;
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
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

#ifdef HAVE_PTRACE_GETREGS

static void
mips_collect_register (struct regcache *regcache,
		       int use_64bit, int regno, union mips_register *reg)
{
  union mips_register tmp_reg;

  if (use_64bit)
    {
      collect_register (regcache, regno, &tmp_reg.reg64);
      *reg = tmp_reg;
    }
  else
    {
      collect_register (regcache, regno, &tmp_reg.reg32);
      reg->reg64 = tmp_reg.reg32;
    }
}

static void
mips_supply_register (struct regcache *regcache,
		      int use_64bit, int regno, const union mips_register *reg)
{
  int offset = 0;

  /* For big-endian 32-bit targets, ignore the high four bytes of each
     eight-byte slot.  */
  if (__BYTE_ORDER == __BIG_ENDIAN && !use_64bit)
    offset = 4;

  supply_register (regcache, regno, reg->buf + offset);
}

static void
mips_collect_register_32bit (struct regcache *regcache,
			     int use_64bit, int regno, unsigned char *buf)
{
  union mips_register tmp_reg;
  int reg32;

  mips_collect_register (regcache, use_64bit, regno, &tmp_reg);
  reg32 = tmp_reg.reg64;
  memcpy (buf, &reg32, 4);
}

static void
mips_supply_register_32bit (struct regcache *regcache,
			    int use_64bit, int regno, const unsigned char *buf)
{
  union mips_register tmp_reg;
  int reg32;

  memcpy (&reg32, buf, 4);
  tmp_reg.reg64 = reg32;
  mips_supply_register (regcache, use_64bit, regno, &tmp_reg);
}

static void
mips_fill_gregset (struct regcache *regcache, void *buf)
{
  union mips_register *regset = (union mips_register *) buf;
  int i, use_64bit;
  const struct target_desc *tdesc = regcache->tdesc;

  use_64bit = (register_size (tdesc, 0) == 8);

  for (i = 1; i < 32; i++)
    mips_collect_register (regcache, use_64bit, i, regset + i);

  mips_collect_register (regcache, use_64bit,
			 find_regno (tdesc, "lo"), regset + 32);
  mips_collect_register (regcache, use_64bit,
			 find_regno (tdesc, "hi"), regset + 33);
  mips_collect_register (regcache, use_64bit,
			 find_regno (tdesc, "pc"), regset + 34);
  mips_collect_register (regcache, use_64bit,
			 find_regno (tdesc, "badvaddr"), regset + 35);
  mips_collect_register (regcache, use_64bit,
			 find_regno (tdesc, "status"), regset + 36);
  mips_collect_register (regcache, use_64bit,
			 find_regno (tdesc, "cause"), regset + 37);

  mips_collect_register (regcache, use_64bit,
			 find_regno (tdesc, "restart"), regset + 0);
}

static void
mips_store_gregset (struct regcache *regcache, const void *buf)
{
  const union mips_register *regset = (const union mips_register *) buf;
  int i, use_64bit;

  use_64bit = (register_size (regcache->tdesc, 0) == 8);

  for (i = 0; i < 32; i++)
    mips_supply_register (regcache, use_64bit, i, regset + i);

  mips_supply_register (regcache, use_64bit,
			find_regno (regcache->tdesc, "lo"), regset + 32);
  mips_supply_register (regcache, use_64bit,
			find_regno (regcache->tdesc, "hi"), regset + 33);
  mips_supply_register (regcache, use_64bit,
			find_regno (regcache->tdesc, "pc"), regset + 34);
  mips_supply_register (regcache, use_64bit,
			find_regno (regcache->tdesc, "badvaddr"), regset + 35);
  mips_supply_register (regcache, use_64bit,
			find_regno (regcache->tdesc, "status"), regset + 36);
  mips_supply_register (regcache, use_64bit,
			find_regno (regcache->tdesc, "cause"), regset + 37);

  mips_supply_register (regcache, use_64bit,
			find_regno (regcache->tdesc, "restart"), regset + 0);
}

static void
mips_fill_fpregset (struct regcache *regcache, void *buf)
{
  union mips_register *regset = (union mips_register *) buf;
  int i, use_64bit, first_fp, big_endian;

  use_64bit = (register_size (regcache->tdesc, 0) == 8);
  first_fp = find_regno (regcache->tdesc, "f0");
  big_endian = (__BYTE_ORDER == __BIG_ENDIAN);

  /* See GDB for a discussion of this peculiar layout.  */
  for (i = 0; i < 32; i++)
    if (use_64bit)
      collect_register (regcache, first_fp + i, regset[i].buf);
    else
      collect_register (regcache, first_fp + i,
			regset[i & ~1].buf + 4 * (big_endian != (i & 1)));

  mips_collect_register_32bit (regcache, use_64bit,
			       find_regno (regcache->tdesc, "fcsr"), regset[32].buf);
  mips_collect_register_32bit (regcache, use_64bit,
			       find_regno (regcache->tdesc, "fir"),
			       regset[32].buf + 4);
}

static void
mips_store_fpregset (struct regcache *regcache, const void *buf)
{
  const union mips_register *regset = (const union mips_register *) buf;
  int i, use_64bit, first_fp, big_endian;

  use_64bit = (register_size (regcache->tdesc, 0) == 8);
  first_fp = find_regno (regcache->tdesc, "f0");
  big_endian = (__BYTE_ORDER == __BIG_ENDIAN);

  /* See GDB for a discussion of this peculiar layout.  */
  for (i = 0; i < 32; i++)
    if (use_64bit)
      supply_register (regcache, first_fp + i, regset[i].buf);
    else
      supply_register (regcache, first_fp + i,
		       regset[i & ~1].buf + 4 * (big_endian != (i & 1)));

  mips_supply_register_32bit (regcache, use_64bit,
			      find_regno (regcache->tdesc, "fcsr"),
			      regset[32].buf);
  mips_supply_register_32bit (regcache, use_64bit,
			      find_regno (regcache->tdesc, "fir"),
			      regset[32].buf + 4);
}
#endif /* HAVE_PTRACE_GETREGS */

static struct regset_info mips_regsets[] = {
#ifdef HAVE_PTRACE_GETREGS
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, 38 * 8, GENERAL_REGS,
    mips_fill_gregset, mips_store_gregset },
  { PTRACE_GETFPREGS, PTRACE_SETFPREGS, 0, 33 * 8, FP_REGS,
    mips_fill_fpregset, mips_store_fpregset },
#endif /* HAVE_PTRACE_GETREGS */
  NULL_REGSET
};

static struct regsets_info mips_regsets_info =
  {
    mips_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info mips_dsp_usrregs_info =
  {
    mips_dsp_num_regs,
    mips_dsp_regmap,
  };

static struct usrregs_info mips_usrregs_info =
  {
    mips_num_regs,
    mips_regmap,
  };

static struct regs_info dsp_regs_info =
  {
    mips_dsp_regset_bitmap,
    &mips_dsp_usrregs_info,
    &mips_regsets_info
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    &mips_usrregs_info,
    &mips_regsets_info
  };

static const struct regs_info *
mips_regs_info (void)
{
  if (have_dsp)
    return &dsp_regs_info;
  else
    return &regs_info;
}

struct linux_target_ops the_low_target = {
  mips_arch_setup,
  mips_regs_info,
  mips_cannot_fetch_register,
  mips_cannot_store_register,
  NULL, /* fetch_register */
  mips_get_pc,
  mips_set_pc,
  NULL, /* breakpoint_kind_from_pc */
  mips_sw_breakpoint_from_kind,
  NULL, /* get_next_pcs */
  0,
  mips_breakpoint_at,
  mips_supports_z_point_type,
  mips_insert_point,
  mips_remove_point,
  mips_stopped_by_watchpoint,
  mips_stopped_data_address,
  NULL,
  NULL,
  NULL, /* siginfo_fixup */
  mips_linux_new_process,
  mips_linux_new_thread,
  mips_linux_new_fork,
  mips_linux_prepare_to_resume
};

void
initialize_low_arch (void)
{
  /* Initialize the Linux target descriptions.  */
  init_registers_mips_linux ();
  init_registers_mips_dsp_linux ();
  init_registers_mips64_linux ();
  init_registers_mips64_dsp_linux ();

  initialize_regsets_info (&mips_regsets_info);
}
