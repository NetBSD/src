/* BSD user-level threads support.

   Copyright (C) 2005-2016 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "gdbthread.h"
#include "inferior.h"
#include "objfiles.h"
#include "observer.h"
#include "regcache.h"
#include "solib.h"
#include "solist.h"
#include "symfile.h"
#include "target.h"

#include "gdb_obstack.h"

#include "bsd-uthread.h"

/* HACK: Save the bsd_uthreads ops returned by bsd_uthread_target.  */
static struct target_ops *bsd_uthread_ops_hack;


/* Architecture-specific operations.  */

/* Per-architecture data key.  */
static struct gdbarch_data *bsd_uthread_data;

struct bsd_uthread_ops
{
  /* Supply registers for an inactive thread to a register cache.  */
  void (*supply_uthread)(struct regcache *, int, CORE_ADDR);

  /* Collect registers for an inactive thread from a register cache.  */
  void (*collect_uthread)(const struct regcache *, int, CORE_ADDR);
};

static void *
bsd_uthread_init (struct obstack *obstack)
{
  struct bsd_uthread_ops *ops;

  ops = OBSTACK_ZALLOC (obstack, struct bsd_uthread_ops);
  return ops;
}

/* Set the function that supplies registers from an inactive thread
   for architecture GDBARCH to SUPPLY_UTHREAD.  */

void
bsd_uthread_set_supply_uthread (struct gdbarch *gdbarch,
				void (*supply_uthread) (struct regcache *,
							int, CORE_ADDR))
{
  struct bsd_uthread_ops *ops
    = (struct bsd_uthread_ops *) gdbarch_data (gdbarch, bsd_uthread_data);

  ops->supply_uthread = supply_uthread;
}

/* Set the function that collects registers for an inactive thread for
   architecture GDBARCH to SUPPLY_UTHREAD.  */

void
bsd_uthread_set_collect_uthread (struct gdbarch *gdbarch,
			 void (*collect_uthread) (const struct regcache *,
						  int, CORE_ADDR))
{
  struct bsd_uthread_ops *ops
    = (struct bsd_uthread_ops *) gdbarch_data (gdbarch, bsd_uthread_data);

  ops->collect_uthread = collect_uthread;
}

/* Magic number to help recognize a valid thread structure.  */
#define BSD_UTHREAD_PTHREAD_MAGIC	0xd09ba115

/* Check whether the thread structure at ADDR is valid.  */

static void
bsd_uthread_check_magic (CORE_ADDR addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  ULONGEST magic = read_memory_unsigned_integer (addr, 4, byte_order);

  if (magic != BSD_UTHREAD_PTHREAD_MAGIC)
    error (_("Bad magic"));
}

/* Thread states.  */
#define BSD_UTHREAD_PS_RUNNING	0
#define BSD_UTHREAD_PS_DEAD	18

/* Address of the pointer to the thread structure for the running
   thread.  */
static CORE_ADDR bsd_uthread_thread_run_addr;

/* Address of the list of all threads.  */
static CORE_ADDR bsd_uthread_thread_list_addr;

/* Offsets of various "interesting" bits in the thread structure.  */
static int bsd_uthread_thread_state_offset = -1;
static int bsd_uthread_thread_next_offset = -1;
static int bsd_uthread_thread_ctx_offset;

/* Name of shared threads library.  */
static const char *bsd_uthread_solib_name;

/* Non-zero if the thread startum implemented by this module is active.  */
static int bsd_uthread_active;

static CORE_ADDR
bsd_uthread_lookup_address (const char *name, struct objfile *objfile)
{
  struct bound_minimal_symbol sym;

  sym = lookup_minimal_symbol (name, NULL, objfile);
  if (sym.minsym)
    return BMSYMBOL_VALUE_ADDRESS (sym);

  return 0;
}

static int
bsd_uthread_lookup_offset (const char *name, struct objfile *objfile)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  CORE_ADDR addr;

  addr = bsd_uthread_lookup_address (name, objfile);
  if (addr == 0)
    return 0;

  return read_memory_unsigned_integer (addr, 4, byte_order);
}

static CORE_ADDR
bsd_uthread_read_memory_address (CORE_ADDR addr)
{
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  return read_memory_typed_address (addr, ptr_type);
}

/* If OBJFILE contains the symbols corresponding to one of the
   supported user-level threads libraries, activate the thread stratum
   implemented by this module.  */

static int
bsd_uthread_activate (struct objfile *objfile)
{
  struct gdbarch *gdbarch = target_gdbarch ();
  struct bsd_uthread_ops *ops
    = (struct bsd_uthread_ops *) gdbarch_data (gdbarch, bsd_uthread_data);

  /* Skip if the thread stratum has already been activated.  */
  if (bsd_uthread_active)
    return 0;

  /* There's no point in enabling this module if no
     architecture-specific operations are provided.  */
  if (!ops->supply_uthread)
    return 0;

  bsd_uthread_thread_run_addr =
    bsd_uthread_lookup_address ("_thread_run", objfile);
  if (bsd_uthread_thread_run_addr == 0)
    return 0;

  bsd_uthread_thread_list_addr =
    bsd_uthread_lookup_address ("_thread_list", objfile);
  if (bsd_uthread_thread_list_addr == 0)
    return 0;

  bsd_uthread_thread_state_offset =
    bsd_uthread_lookup_offset ("_thread_state_offset", objfile);
  if (bsd_uthread_thread_state_offset == 0)
    return 0;

  bsd_uthread_thread_next_offset =
    bsd_uthread_lookup_offset ("_thread_next_offset", objfile);
  if (bsd_uthread_thread_next_offset == 0)
    return 0;

  bsd_uthread_thread_ctx_offset =
    bsd_uthread_lookup_offset ("_thread_ctx_offset", objfile);

  push_target (bsd_uthread_ops_hack);
  bsd_uthread_active = 1;
  return 1;
}

/* Cleanup due to deactivation.  */

static void
bsd_uthread_close (struct target_ops *self)
{
  bsd_uthread_active = 0;
  bsd_uthread_thread_run_addr = 0;
  bsd_uthread_thread_list_addr = 0;
  bsd_uthread_thread_state_offset = 0;
  bsd_uthread_thread_next_offset = 0;
  bsd_uthread_thread_ctx_offset = 0;
  bsd_uthread_solib_name = NULL;
}

/* Deactivate the thread stratum implemented by this module.  */

static void
bsd_uthread_deactivate (void)
{
  /* Skip if the thread stratum has already been deactivated.  */
  if (!bsd_uthread_active)
    return;

  unpush_target (bsd_uthread_ops_hack);
}

static void
bsd_uthread_inferior_created (struct target_ops *ops, int from_tty)
{
  bsd_uthread_activate (NULL);
}

/* Likely candidates for the threads library.  */
static const char *bsd_uthread_solib_names[] =
{
  "/usr/lib/libc_r.so",		/* FreeBSD */
  "/usr/lib/libpthread.so",	/* OpenBSD */
  NULL
};

static void
bsd_uthread_solib_loaded (struct so_list *so)
{
  const char **names = bsd_uthread_solib_names;

  for (names = bsd_uthread_solib_names; *names; names++)
    {
      if (startswith (so->so_original_name, *names))
	{
	  solib_read_symbols (so, 0);

	  if (bsd_uthread_activate (so->objfile))
	    {
	      bsd_uthread_solib_name = so->so_original_name;
	      return;
	    }
	}
    }
}

static void
bsd_uthread_solib_unloaded (struct so_list *so)
{
  if (!bsd_uthread_solib_name)
    return;

  if (strcmp (so->so_original_name, bsd_uthread_solib_name) == 0)
    bsd_uthread_deactivate ();
}

static void
bsd_uthread_mourn_inferior (struct target_ops *ops)
{
  struct target_ops *beneath = find_target_beneath (ops);
  beneath->to_mourn_inferior (beneath);
  bsd_uthread_deactivate ();
}

static void
bsd_uthread_fetch_registers (struct target_ops *ops,
			     struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct bsd_uthread_ops *uthread_ops
    = (struct bsd_uthread_ops *) gdbarch_data (gdbarch, bsd_uthread_data);
  CORE_ADDR addr = ptid_get_tid (inferior_ptid);
  struct target_ops *beneath = find_target_beneath (ops);
  CORE_ADDR active_addr;

  /* Always fetch the appropriate registers from the layer beneath.  */
  beneath->to_fetch_registers (beneath, regcache, regnum);

  /* FIXME: That might have gotten us more than we asked for.  Make
     sure we overwrite all relevant registers with values from the
     thread structure.  This can go once we fix the underlying target.  */
  regnum = -1;

  active_addr = bsd_uthread_read_memory_address (bsd_uthread_thread_run_addr);
  if (addr != 0 && addr != active_addr)
    {
      bsd_uthread_check_magic (addr);
      uthread_ops->supply_uthread (regcache, regnum,
				   addr + bsd_uthread_thread_ctx_offset);
    }
}

static void
bsd_uthread_store_registers (struct target_ops *ops,
			     struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct bsd_uthread_ops *uthread_ops
    = (struct bsd_uthread_ops *) gdbarch_data (gdbarch, bsd_uthread_data);
  struct target_ops *beneath = find_target_beneath (ops);
  CORE_ADDR addr = ptid_get_tid (inferior_ptid);
  CORE_ADDR active_addr;

  active_addr = bsd_uthread_read_memory_address (bsd_uthread_thread_run_addr);
  if (addr != 0 && addr != active_addr)
    {
      bsd_uthread_check_magic (addr);
      uthread_ops->collect_uthread (regcache, regnum,
				    addr + bsd_uthread_thread_ctx_offset);
    }
  else
    {
      /* Updating the thread that is currently running; pass the
         request to the layer beneath.  */
      beneath->to_store_registers (beneath, regcache, regnum);
    }
}

static ptid_t
bsd_uthread_wait (struct target_ops *ops,
		  ptid_t ptid, struct target_waitstatus *status, int options)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  CORE_ADDR addr;
  struct target_ops *beneath = find_target_beneath (ops);

  /* Pass the request to the layer beneath.  */
  ptid = beneath->to_wait (beneath, ptid, status, options);

  /* If the process is no longer alive, there's no point in figuring
     out the thread ID.  It will fail anyway.  */
  if (status->kind == TARGET_WAITKIND_SIGNALLED
      || status->kind == TARGET_WAITKIND_EXITED)
    return ptid;

  /* Fetch the corresponding thread ID, and augment the returned
     process ID with it.  */
  addr = bsd_uthread_read_memory_address (bsd_uthread_thread_run_addr);
  if (addr != 0)
    {
      gdb_byte buf[4];

      /* FIXME: For executables linked statically with the threads
         library, we end up here before the program has actually been
         executed.  In that case ADDR will be garbage since it has
         been read from the wrong virtual memory image.  */
      if (target_read_memory (addr, buf, 4) == 0)
	{
	  ULONGEST magic = extract_unsigned_integer (buf, 4, byte_order);
	  if (magic == BSD_UTHREAD_PTHREAD_MAGIC)
	    ptid = ptid_build (ptid_get_pid (ptid), 0, addr);
	}
    }

  /* If INFERIOR_PTID doesn't have a tid member yet, and we now have a
     ptid with tid set, then ptid is still the initial thread of
     the process.  Notify GDB core about it.  */
  if (ptid_get_tid (inferior_ptid) == 0
      && ptid_get_tid (ptid) != 0 && !in_thread_list (ptid))
    thread_change_ptid (inferior_ptid, ptid);

  /* Don't let the core see a ptid without a corresponding thread.  */
  if (!in_thread_list (ptid) || is_exited (ptid))
    add_thread (ptid);

  return ptid;
}

static void
bsd_uthread_resume (struct target_ops *ops,
		    ptid_t ptid, int step, enum gdb_signal sig)
{
  /* Pass the request to the layer beneath.  */
  struct target_ops *beneath = find_target_beneath (ops);
  beneath->to_resume (beneath, ptid, step, sig);
}

static int
bsd_uthread_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  struct target_ops *beneath = find_target_beneath (ops);
  CORE_ADDR addr = ptid_get_tid (inferior_ptid);

  if (addr != 0)
    {
      int offset = bsd_uthread_thread_state_offset;
      ULONGEST state;

      bsd_uthread_check_magic (addr);

      state = read_memory_unsigned_integer (addr + offset, 4, byte_order);
      if (state == BSD_UTHREAD_PS_DEAD)
	return 0;
    }

  return beneath->to_thread_alive (beneath, ptid);
}

static void
bsd_uthread_update_thread_list (struct target_ops *ops)
{
  pid_t pid = ptid_get_pid (inferior_ptid);
  int offset = bsd_uthread_thread_next_offset;
  CORE_ADDR addr;

  prune_threads ();

  addr = bsd_uthread_read_memory_address (bsd_uthread_thread_list_addr);
  while (addr != 0)
    {
      ptid_t ptid = ptid_build (pid, 0, addr);

      if (!in_thread_list (ptid) || is_exited (ptid))
	{
	  /* If INFERIOR_PTID doesn't have a tid member yet, then ptid
	     is still the initial thread of the process.  Notify GDB
	     core about it.  */
	  if (ptid_get_tid (inferior_ptid) == 0)
	    thread_change_ptid (inferior_ptid, ptid);
	  else
	    add_thread (ptid);
	}

      addr = bsd_uthread_read_memory_address (addr + offset);
    }
}

/* Possible states a thread can be in.  */
static char *bsd_uthread_state[] =
{
  "RUNNING",
  "SIGTHREAD",
  "MUTEX_WAIT",
  "COND_WAIT",
  "FDLR_WAIT",
  "FDLW_WAIT",
  "FDR_WAIT",
  "FDW_WAIT",
  "FILE_WAIT",
  "POLL_WAIT",
  "SELECT_WAIT",
  "SLEEP_WAIT",
  "WAIT_WAIT",
  "SIGSUSPEND",
  "SIGWAIT",
  "SPINBLOCK",
  "JOIN",
  "SUSPENDED",
  "DEAD",
  "DEADLOCK"
};

/* Return a string describing th state of the thread specified by
   INFO.  */

static char *
bsd_uthread_extra_thread_info (struct target_ops *self,
			       struct thread_info *info)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  CORE_ADDR addr = ptid_get_tid (info->ptid);

  if (addr != 0)
    {
      int offset = bsd_uthread_thread_state_offset;
      ULONGEST state;

      state = read_memory_unsigned_integer (addr + offset, 4, byte_order);
      if (state < ARRAY_SIZE (bsd_uthread_state))
	return bsd_uthread_state[state];
    }

  return NULL;
}

static char *
bsd_uthread_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  if (ptid_get_tid (ptid) != 0)
    {
      static char buf[64];

      xsnprintf (buf, sizeof buf, "process %d, thread 0x%lx",
		 ptid_get_pid (ptid), ptid_get_tid (ptid));
      return buf;
    }

  return normal_pid_to_str (ptid);
}

static struct target_ops *
bsd_uthread_target (void)
{
  struct target_ops *t = XCNEW (struct target_ops);

  t->to_shortname = "bsd-uthreads";
  t->to_longname = "BSD user-level threads";
  t->to_doc = "BSD user-level threads";
  t->to_close = bsd_uthread_close;
  t->to_mourn_inferior = bsd_uthread_mourn_inferior;
  t->to_fetch_registers = bsd_uthread_fetch_registers;
  t->to_store_registers = bsd_uthread_store_registers;
  t->to_wait = bsd_uthread_wait;
  t->to_resume = bsd_uthread_resume;
  t->to_thread_alive = bsd_uthread_thread_alive;
  t->to_update_thread_list = bsd_uthread_update_thread_list;
  t->to_extra_thread_info = bsd_uthread_extra_thread_info;
  t->to_pid_to_str = bsd_uthread_pid_to_str;
  t->to_stratum = thread_stratum;
  t->to_magic = OPS_MAGIC;
  bsd_uthread_ops_hack = t;

  return t;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_bsd_uthread;

void
_initialize_bsd_uthread (void)
{
  complete_target_initialization (bsd_uthread_target ());

  bsd_uthread_data = gdbarch_data_register_pre_init (bsd_uthread_init);

  observer_attach_inferior_created (bsd_uthread_inferior_created);
  observer_attach_solib_loaded (bsd_uthread_solib_loaded);
  observer_attach_solib_unloaded (bsd_uthread_solib_unloaded);
}
