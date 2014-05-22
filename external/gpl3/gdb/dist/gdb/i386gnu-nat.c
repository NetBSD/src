/* Low level interface to i386 running the GNU Hurd.

   Copyright (C) 1992-2013 Free Software Foundation, Inc.

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
#include "floatformat.h"
#include "regcache.h"

#include "gdb_assert.h"
#include <errno.h>
#include <stdio.h>
#include "gdb_string.h"

#include <mach.h>
#include <mach_error.h>
#include <mach/message.h>
#include <mach/exception.h>

#include "i386-tdep.h"

#include "gnu-nat.h"
#include "i387-tdep.h"

#ifdef HAVE_SYS_PROCFS_H
# include <sys/procfs.h>
# include "gregset.h"
#endif

/* Offset to the thread_state_t location where REG is stored.  */
#define REG_OFFSET(reg) offsetof (struct i386_thread_state, reg)

/* At REG_OFFSET[N] is the offset to the thread_state_t location where
   the GDB register N is stored.  */
static int reg_offset[] =
{
  REG_OFFSET (eax), REG_OFFSET (ecx), REG_OFFSET (edx), REG_OFFSET (ebx),
  REG_OFFSET (uesp), REG_OFFSET (ebp), REG_OFFSET (esi), REG_OFFSET (edi),
  REG_OFFSET (eip), REG_OFFSET (efl), REG_OFFSET (cs), REG_OFFSET (ss),
  REG_OFFSET (ds), REG_OFFSET (es), REG_OFFSET (fs), REG_OFFSET (gs)
};

#define REG_ADDR(state, regnum) ((char *)(state) + reg_offset[regnum])
#define CREG_ADDR(state, regnum) ((const char *)(state) + reg_offset[regnum])


/* Get the whole floating-point state of THREAD and record the values
   of the corresponding (pseudo) registers.  */

static void
fetch_fpregs (struct regcache *regcache, struct proc *thread)
{
  mach_msg_type_number_t count = i386_FLOAT_STATE_COUNT;
  struct i386_float_state state;
  error_t err;

  err = thread_get_state (thread->port, i386_FLOAT_STATE,
			  (thread_state_t) &state, &count);
  if (err)
    {
      warning (_("Couldn't fetch floating-point state from %s"),
	       proc_string (thread));
      return;
    }

  if (!state.initialized)
    {
      /* The floating-point state isn't initialized.  */
      i387_supply_fsave (regcache, -1, NULL);
    }
  else
    {
      /* Supply the floating-point registers.  */
      i387_supply_fsave (regcache, -1, state.hw_state);
    }
}

#ifdef HAVE_SYS_PROCFS_H
/* These two calls are used by the core-regset.c code for
   reading ELF core files.  */
void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregs)
{
  int i;
  for (i = 0; i < I386_NUM_GREGS; i++)
    regcache_raw_supply (regcache, i, CREG_ADDR (gregs, i));
}

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregs)
{
  i387_supply_fsave (regcache, -1, fpregs);
}
#endif

/* Fetch register REGNO, or all regs if REGNO is -1.  */
static void
gnu_fetch_registers (struct target_ops *ops,
		     struct regcache *regcache, int regno)
{
  struct proc *thread;

  /* Make sure we know about new threads.  */
  inf_update_procs (gnu_current_inf);

  thread = inf_tid_to_thread (gnu_current_inf,
			      ptid_get_tid (inferior_ptid));
  if (!thread)
    error (_("Can't fetch registers from thread %s: No such thread"),
	   target_pid_to_str (inferior_ptid));

  if (regno < I386_NUM_GREGS || regno == -1)
    {
      thread_state_t state;

      /* This does the dirty work for us.  */
      state = proc_get_state (thread, 0);
      if (!state)
	{
	  warning (_("Couldn't fetch registers from %s"),
		   proc_string (thread));
	  return;
	}

      if (regno == -1)
	{
	  int i;

	  proc_debug (thread, "fetching all register");

	  for (i = 0; i < I386_NUM_GREGS; i++)
	    regcache_raw_supply (regcache, i, REG_ADDR (state, i));
	  thread->fetched_regs = ~0;
	}
      else
	{
	  proc_debug (thread, "fetching register %s",
		      gdbarch_register_name (get_regcache_arch (regcache),
					     regno));

	  regcache_raw_supply (regcache, regno,
			       REG_ADDR (state, regno));
	  thread->fetched_regs |= (1 << regno);
	}
    }

  if (regno >= I386_NUM_GREGS || regno == -1)
    {
      proc_debug (thread, "fetching floating-point registers");

      fetch_fpregs (regcache, thread);
    }
}


/* Store the whole floating-point state into THREAD using information
   from the corresponding (pseudo) registers.  */
static void
store_fpregs (const struct regcache *regcache, struct proc *thread, int regno)
{
  mach_msg_type_number_t count = i386_FLOAT_STATE_COUNT;
  struct i386_float_state state;
  error_t err;

  err = thread_get_state (thread->port, i386_FLOAT_STATE,
			  (thread_state_t) &state, &count);
  if (err)
    {
      warning (_("Couldn't fetch floating-point state from %s"),
	       proc_string (thread));
      return;
    }

  /* FIXME: kettenis/2001-07-15: Is this right?  Should we somehow
     take into account DEPRECATED_REGISTER_VALID like the old code did?  */
  i387_collect_fsave (regcache, regno, state.hw_state);

  err = thread_set_state (thread->port, i386_FLOAT_STATE,
			  (thread_state_t) &state, i386_FLOAT_STATE_COUNT);
  if (err)
    {
      warning (_("Couldn't store floating-point state into %s"),
	       proc_string (thread));
      return;
    }
}

/* Store at least register REGNO, or all regs if REGNO == -1.  */
static void
gnu_store_registers (struct target_ops *ops,
		     struct regcache *regcache, int regno)
{
  struct proc *thread;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  /* Make sure we know about new threads.  */
  inf_update_procs (gnu_current_inf);

  thread = inf_tid_to_thread (gnu_current_inf,
			      ptid_get_tid (inferior_ptid));
  if (!thread)
    error (_("Couldn't store registers into thread %s: No such thread"),
	   target_pid_to_str (inferior_ptid));

  if (regno < I386_NUM_GREGS || regno == -1)
    {
      thread_state_t state;
      thread_state_data_t old_state;
      int was_aborted = thread->aborted;
      int was_valid = thread->state_valid;
      int trace;

      if (!was_aborted && was_valid)
	memcpy (&old_state, &thread->state, sizeof (old_state));

      state = proc_get_state (thread, 1);
      if (!state)
	{
	  warning (_("Couldn't store registers into %s"),
		   proc_string (thread));
	  return;
	}

      /* Save the T bit.  We might try to restore the %eflags register
         below, but changing the T bit would seriously confuse GDB.  */
      trace = ((struct i386_thread_state *)state)->efl & 0x100;

      if (!was_aborted && was_valid)
	/* See which registers have changed after aborting the thread.  */
	{
	  int check_regno;

	  for (check_regno = 0; check_regno < I386_NUM_GREGS; check_regno++)
	    if ((thread->fetched_regs & (1 << check_regno))
		&& memcpy (REG_ADDR (&old_state, check_regno),
			   REG_ADDR (state, check_regno),
			   register_size (gdbarch, check_regno)))
	      /* Register CHECK_REGNO has changed!  Ack!  */
	      {
		warning (_("Register %s changed after the thread was aborted"),
			 gdbarch_register_name (gdbarch, check_regno));
		if (regno >= 0 && regno != check_regno)
		  /* Update GDB's copy of the register.  */
		  regcache_raw_supply (regcache, check_regno,
				       REG_ADDR (state, check_regno));
		else
		  warning (_("... also writing this register!  "
			     "Suspicious..."));
	      }
	}

      if (regno == -1)
	{
	  int i;

	  proc_debug (thread, "storing all registers");

	  for (i = 0; i < I386_NUM_GREGS; i++)
	    if (REG_VALID == regcache_register_status (regcache, i))
	      regcache_raw_collect (regcache, i, REG_ADDR (state, i));
	}
      else
	{
	  proc_debug (thread, "storing register %s",
		      gdbarch_register_name (gdbarch, regno));

	  gdb_assert (REG_VALID == regcache_register_status (regcache, regno));
	  regcache_raw_collect (regcache, regno, REG_ADDR (state, regno));
	}

      /* Restore the T bit.  */
      ((struct i386_thread_state *)state)->efl &= ~0x100;
      ((struct i386_thread_state *)state)->efl |= trace;
    }

  if (regno >= I386_NUM_GREGS || regno == -1)
    {
      proc_debug (thread, "storing floating-point registers");

      store_fpregs (regcache, thread, regno);
    }
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_i386gnu_nat;

void
_initialize_i386gnu_nat (void)
{
  struct target_ops *t;

  /* Fill in the generic GNU/Hurd methods.  */
  t = gnu_target ();

  t->to_fetch_registers = gnu_fetch_registers;
  t->to_store_registers = gnu_store_registers;

  /* Register the target.  */
  add_target (t);
}
