/* Process record and replay target for GDB, the GNU debugger.

   Copyright (C) 2013-2017 Free Software Foundation, Inc.

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
#include "gdbcmd.h"
#include "regcache.h"
#include "gdbthread.h"
#include "event-top.h"
#include "completer.h"
#include "arch-utils.h"
#include "gdbcore.h"
#include "exec.h"
#include "record.h"
#include "record-full.h"
#include "elf-bfd.h"
#include "gcore.h"
#include "event-loop.h"
#include "inf-loop.h"
#include "gdb_bfd.h"
#include "observer.h"
#include "infrun.h"
#include "common/gdb_unlinker.h"

#include <signal.h>

/* This module implements "target record-full", also known as "process
   record and replay".  This target sits on top of a "normal" target
   (a target that "has execution"), and provides a record and replay
   functionality, including reverse debugging.

   Target record has two modes: recording, and replaying.

   In record mode, we intercept the to_resume and to_wait methods.
   Whenever gdb resumes the target, we run the target in single step
   mode, and we build up an execution log in which, for each executed
   instruction, we record all changes in memory and register state.
   This is invisible to the user, to whom it just looks like an
   ordinary debugging session (except for performance degredation).

   In replay mode, instead of actually letting the inferior run as a
   process, we simulate its execution by playing back the recorded
   execution log.  For each instruction in the log, we simulate the
   instruction's side effects by duplicating the changes that it would
   have made on memory and registers.  */

#define DEFAULT_RECORD_FULL_INSN_MAX_NUM	200000

#define RECORD_FULL_IS_REPLAY \
     (record_full_list->next || execution_direction == EXEC_REVERSE)

#define RECORD_FULL_FILE_MAGIC	netorder32(0x20091016)

/* These are the core structs of the process record functionality.

   A record_full_entry is a record of the value change of a register
   ("record_full_reg") or a part of memory ("record_full_mem").  And each
   instruction must have a struct record_full_entry ("record_full_end")
   that indicates that this is the last struct record_full_entry of this
   instruction.

   Each struct record_full_entry is linked to "record_full_list" by "prev"
   and "next" pointers.  */

struct record_full_mem_entry
{
  CORE_ADDR addr;
  int len;
  /* Set this flag if target memory for this entry
     can no longer be accessed.  */
  int mem_entry_not_accessible;
  union
  {
    gdb_byte *ptr;
    gdb_byte buf[sizeof (gdb_byte *)];
  } u;
};

struct record_full_reg_entry
{
  unsigned short num;
  unsigned short len;
  union 
  {
    gdb_byte *ptr;
    gdb_byte buf[2 * sizeof (gdb_byte *)];
  } u;
};

struct record_full_end_entry
{
  enum gdb_signal sigval;
  ULONGEST insn_num;
};

enum record_full_type
{
  record_full_end = 0,
  record_full_reg,
  record_full_mem
};

/* This is the data structure that makes up the execution log.

   The execution log consists of a single linked list of entries
   of type "struct record_full_entry".  It is doubly linked so that it
   can be traversed in either direction.

   The start of the list is anchored by a struct called
   "record_full_first".  The pointer "record_full_list" either points
   to the last entry that was added to the list (in record mode), or to
   the next entry in the list that will be executed (in replay mode).

   Each list element (struct record_full_entry), in addition to next
   and prev pointers, consists of a union of three entry types: mem,
   reg, and end.  A field called "type" determines which entry type is
   represented by a given list element.

   Each instruction that is added to the execution log is represented
   by a variable number of list elements ('entries').  The instruction
   will have one "reg" entry for each register that is changed by 
   executing the instruction (including the PC in every case).  It 
   will also have one "mem" entry for each memory change.  Finally,
   each instruction will have an "end" entry that separates it from
   the changes associated with the next instruction.  */

struct record_full_entry
{
  struct record_full_entry *prev;
  struct record_full_entry *next;
  enum record_full_type type;
  union
  {
    /* reg */
    struct record_full_reg_entry reg;
    /* mem */
    struct record_full_mem_entry mem;
    /* end */
    struct record_full_end_entry end;
  } u;
};

/* If true, query if PREC cannot record memory
   change of next instruction.  */
int record_full_memory_query = 0;

struct record_full_core_buf_entry
{
  struct record_full_core_buf_entry *prev;
  struct target_section *p;
  bfd_byte *buf;
};

/* Record buf with core target.  */
static gdb_byte *record_full_core_regbuf = NULL;
static struct target_section *record_full_core_start;
static struct target_section *record_full_core_end;
static struct record_full_core_buf_entry *record_full_core_buf_list = NULL;

/* The following variables are used for managing the linked list that
   represents the execution log.

   record_full_first is the anchor that holds down the beginning of
   the list.

   record_full_list serves two functions:
     1) In record mode, it anchors the end of the list.
     2) In replay mode, it traverses the list and points to
        the next instruction that must be emulated.

   record_full_arch_list_head and record_full_arch_list_tail are used
   to manage a separate list, which is used to build up the change
   elements of the currently executing instruction during record mode.
   When this instruction has been completely annotated in the "arch
   list", it will be appended to the main execution log.  */

static struct record_full_entry record_full_first;
static struct record_full_entry *record_full_list = &record_full_first;
static struct record_full_entry *record_full_arch_list_head = NULL;
static struct record_full_entry *record_full_arch_list_tail = NULL;

/* 1 ask user. 0 auto delete the last struct record_full_entry.  */
static int record_full_stop_at_limit = 1;
/* Maximum allowed number of insns in execution log.  */
static unsigned int record_full_insn_max_num
	= DEFAULT_RECORD_FULL_INSN_MAX_NUM;
/* Actual count of insns presently in execution log.  */
static unsigned int record_full_insn_num = 0;
/* Count of insns logged so far (may be larger
   than count of insns presently in execution log).  */
static ULONGEST record_full_insn_count;

/* The target_ops of process record.  */
static struct target_ops record_full_ops;
static struct target_ops record_full_core_ops;

/* See record-full.h.  */

int
record_full_is_used (void)
{
  struct target_ops *t;

  t = find_record_target ();
  return (t == &record_full_ops
	  || t == &record_full_core_ops);
}


/* Command lists for "set/show record full".  */
static struct cmd_list_element *set_record_full_cmdlist;
static struct cmd_list_element *show_record_full_cmdlist;

/* Command list for "record full".  */
static struct cmd_list_element *record_full_cmdlist;

static void record_full_goto_insn (struct record_full_entry *entry,
				   enum exec_direction_kind dir);
static void record_full_save (struct target_ops *self,
			      const char *recfilename);

/* Alloc and free functions for record_full_reg, record_full_mem, and
   record_full_end entries.  */

/* Alloc a record_full_reg record entry.  */

static inline struct record_full_entry *
record_full_reg_alloc (struct regcache *regcache, int regnum)
{
  struct record_full_entry *rec;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  rec = XCNEW (struct record_full_entry);
  rec->type = record_full_reg;
  rec->u.reg.num = regnum;
  rec->u.reg.len = register_size (gdbarch, regnum);
  if (rec->u.reg.len > sizeof (rec->u.reg.u.buf))
    rec->u.reg.u.ptr = (gdb_byte *) xmalloc (rec->u.reg.len);

  return rec;
}

/* Free a record_full_reg record entry.  */

static inline void
record_full_reg_release (struct record_full_entry *rec)
{
  gdb_assert (rec->type == record_full_reg);
  if (rec->u.reg.len > sizeof (rec->u.reg.u.buf))
    xfree (rec->u.reg.u.ptr);
  xfree (rec);
}

/* Alloc a record_full_mem record entry.  */

static inline struct record_full_entry *
record_full_mem_alloc (CORE_ADDR addr, int len)
{
  struct record_full_entry *rec;

  rec = XCNEW (struct record_full_entry);
  rec->type = record_full_mem;
  rec->u.mem.addr = addr;
  rec->u.mem.len = len;
  if (rec->u.mem.len > sizeof (rec->u.mem.u.buf))
    rec->u.mem.u.ptr = (gdb_byte *) xmalloc (len);

  return rec;
}

/* Free a record_full_mem record entry.  */

static inline void
record_full_mem_release (struct record_full_entry *rec)
{
  gdb_assert (rec->type == record_full_mem);
  if (rec->u.mem.len > sizeof (rec->u.mem.u.buf))
    xfree (rec->u.mem.u.ptr);
  xfree (rec);
}

/* Alloc a record_full_end record entry.  */

static inline struct record_full_entry *
record_full_end_alloc (void)
{
  struct record_full_entry *rec;

  rec = XCNEW (struct record_full_entry);
  rec->type = record_full_end;

  return rec;
}

/* Free a record_full_end record entry.  */

static inline void
record_full_end_release (struct record_full_entry *rec)
{
  xfree (rec);
}

/* Free one record entry, any type.
   Return entry->type, in case caller wants to know.  */

static inline enum record_full_type
record_full_entry_release (struct record_full_entry *rec)
{
  enum record_full_type type = rec->type;

  switch (type) {
  case record_full_reg:
    record_full_reg_release (rec);
    break;
  case record_full_mem:
    record_full_mem_release (rec);
    break;
  case record_full_end:
    record_full_end_release (rec);
    break;
  }
  return type;
}

/* Free all record entries in list pointed to by REC.  */

static void
record_full_list_release (struct record_full_entry *rec)
{
  if (!rec)
    return;

  while (rec->next)
    rec = rec->next;

  while (rec->prev)
    {
      rec = rec->prev;
      record_full_entry_release (rec->next);
    }

  if (rec == &record_full_first)
    {
      record_full_insn_num = 0;
      record_full_first.next = NULL;
    }
  else
    record_full_entry_release (rec);
}

/* Free all record entries forward of the given list position.  */

static void
record_full_list_release_following (struct record_full_entry *rec)
{
  struct record_full_entry *tmp = rec->next;

  rec->next = NULL;
  while (tmp)
    {
      rec = tmp->next;
      if (record_full_entry_release (tmp) == record_full_end)
	{
	  record_full_insn_num--;
	  record_full_insn_count--;
	}
      tmp = rec;
    }
}

/* Delete the first instruction from the beginning of the log, to make
   room for adding a new instruction at the end of the log.

   Note -- this function does not modify record_full_insn_num.  */

static void
record_full_list_release_first (void)
{
  struct record_full_entry *tmp;

  if (!record_full_first.next)
    return;

  /* Loop until a record_full_end.  */
  while (1)
    {
      /* Cut record_full_first.next out of the linked list.  */
      tmp = record_full_first.next;
      record_full_first.next = tmp->next;
      tmp->next->prev = &record_full_first;

      /* tmp is now isolated, and can be deleted.  */
      if (record_full_entry_release (tmp) == record_full_end)
	break;	/* End loop at first record_full_end.  */

      if (!record_full_first.next)
	{
	  gdb_assert (record_full_insn_num == 1);
	  break;	/* End loop when list is empty.  */
	}
    }
}

/* Add a struct record_full_entry to record_full_arch_list.  */

static void
record_full_arch_list_add (struct record_full_entry *rec)
{
  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: record_full_arch_list_add %s.\n",
			host_address_to_string (rec));

  if (record_full_arch_list_tail)
    {
      record_full_arch_list_tail->next = rec;
      rec->prev = record_full_arch_list_tail;
      record_full_arch_list_tail = rec;
    }
  else
    {
      record_full_arch_list_head = rec;
      record_full_arch_list_tail = rec;
    }
}

/* Return the value storage location of a record entry.  */
static inline gdb_byte *
record_full_get_loc (struct record_full_entry *rec)
{
  switch (rec->type) {
  case record_full_mem:
    if (rec->u.mem.len > sizeof (rec->u.mem.u.buf))
      return rec->u.mem.u.ptr;
    else
      return rec->u.mem.u.buf;
  case record_full_reg:
    if (rec->u.reg.len > sizeof (rec->u.reg.u.buf))
      return rec->u.reg.u.ptr;
    else
      return rec->u.reg.u.buf;
  case record_full_end:
  default:
    gdb_assert_not_reached ("unexpected record_full_entry type");
    return NULL;
  }
}

/* Record the value of a register NUM to record_full_arch_list.  */

int
record_full_arch_list_add_reg (struct regcache *regcache, int regnum)
{
  struct record_full_entry *rec;

  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: add register num = %d to "
			"record list.\n",
			regnum);

  rec = record_full_reg_alloc (regcache, regnum);

  regcache_raw_read (regcache, regnum, record_full_get_loc (rec));

  record_full_arch_list_add (rec);

  return 0;
}

/* Record the value of a region of memory whose address is ADDR and
   length is LEN to record_full_arch_list.  */

int
record_full_arch_list_add_mem (CORE_ADDR addr, int len)
{
  struct record_full_entry *rec;

  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: add mem addr = %s len = %d to "
			"record list.\n",
			paddress (target_gdbarch (), addr), len);

  if (!addr)	/* FIXME: Why?  Some arch must permit it...  */
    return 0;

  rec = record_full_mem_alloc (addr, len);

  if (record_read_memory (target_gdbarch (), addr,
			  record_full_get_loc (rec), len))
    {
      record_full_mem_release (rec);
      return -1;
    }

  record_full_arch_list_add (rec);

  return 0;
}

/* Add a record_full_end type struct record_full_entry to
   record_full_arch_list.  */

int
record_full_arch_list_add_end (void)
{
  struct record_full_entry *rec;

  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: add end to arch list.\n");

  rec = record_full_end_alloc ();
  rec->u.end.sigval = GDB_SIGNAL_0;
  rec->u.end.insn_num = ++record_full_insn_count;

  record_full_arch_list_add (rec);

  return 0;
}

static void
record_full_check_insn_num (void)
{
  if (record_full_insn_num == record_full_insn_max_num)
    {
      /* Ask user what to do.  */
      if (record_full_stop_at_limit)
	{
	  if (!yquery (_("Do you want to auto delete previous execution "
			"log entries when record/replay buffer becomes "
			"full (record full stop-at-limit)?")))
	    error (_("Process record: stopped by user."));
	  record_full_stop_at_limit = 0;
	}
    }
}

static void
record_full_arch_list_cleanups (void *ignore)
{
  record_full_list_release (record_full_arch_list_tail);
}

/* Before inferior step (when GDB record the running message, inferior
   only can step), GDB will call this function to record the values to
   record_full_list.  This function will call gdbarch_process_record to
   record the running message of inferior and set them to
   record_full_arch_list, and add it to record_full_list.  */

static int
record_full_message (struct regcache *regcache, enum gdb_signal signal)
{
  int ret;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct cleanup *old_cleanups
    = make_cleanup (record_full_arch_list_cleanups, 0);

  record_full_arch_list_head = NULL;
  record_full_arch_list_tail = NULL;

  /* Check record_full_insn_num.  */
  record_full_check_insn_num ();

  /* If gdb sends a signal value to target_resume,
     save it in the 'end' field of the previous instruction.

     Maybe process record should record what really happened,
     rather than what gdb pretends has happened.

     So if Linux delivered the signal to the child process during
     the record mode, we will record it and deliver it again in
     the replay mode.

     If user says "ignore this signal" during the record mode, then
     it will be ignored again during the replay mode (no matter if
     the user says something different, like "deliver this signal"
     during the replay mode).

     User should understand that nothing he does during the replay
     mode will change the behavior of the child.  If he tries,
     then that is a user error.

     But we should still deliver the signal to gdb during the replay,
     if we delivered it during the recording.  Therefore we should
     record the signal during record_full_wait, not
     record_full_resume.  */
  if (record_full_list != &record_full_first)  /* FIXME better way to check */
    {
      gdb_assert (record_full_list->type == record_full_end);
      record_full_list->u.end.sigval = signal;
    }

  if (signal == GDB_SIGNAL_0
      || !gdbarch_process_record_signal_p (gdbarch))
    ret = gdbarch_process_record (gdbarch,
				  regcache,
				  regcache_read_pc (regcache));
  else
    ret = gdbarch_process_record_signal (gdbarch,
					 regcache,
					 signal);

  if (ret > 0)
    error (_("Process record: inferior program stopped."));
  if (ret < 0)
    error (_("Process record: failed to record execution log."));

  discard_cleanups (old_cleanups);

  record_full_list->next = record_full_arch_list_head;
  record_full_arch_list_head->prev = record_full_list;
  record_full_list = record_full_arch_list_tail;

  if (record_full_insn_num == record_full_insn_max_num)
    record_full_list_release_first ();
  else
    record_full_insn_num++;

  return 1;
}

struct record_full_message_args {
  struct regcache *regcache;
  enum gdb_signal signal;
};

static int
record_full_message_wrapper (void *args)
{
  struct record_full_message_args *record_full_args
    = (struct record_full_message_args *) args;

  return record_full_message (record_full_args->regcache,
			      record_full_args->signal);
}

static int
record_full_message_wrapper_safe (struct regcache *regcache,
				  enum gdb_signal signal)
{
  struct record_full_message_args args;

  args.regcache = regcache;
  args.signal = signal;

  return catch_errors (record_full_message_wrapper, &args, "",
		       RETURN_MASK_ALL);
}

/* Set to 1 if record_full_store_registers and record_full_xfer_partial
   doesn't need record.  */

static int record_full_gdb_operation_disable = 0;

struct cleanup *
record_full_gdb_operation_disable_set (void)
{
  struct cleanup *old_cleanups = NULL;

  old_cleanups =
    make_cleanup_restore_integer (&record_full_gdb_operation_disable);
  record_full_gdb_operation_disable = 1;

  return old_cleanups;
}

/* Flag set to TRUE for target_stopped_by_watchpoint.  */
static enum target_stop_reason record_full_stop_reason
  = TARGET_STOPPED_BY_NO_REASON;

/* Execute one instruction from the record log.  Each instruction in
   the log will be represented by an arbitrary sequence of register
   entries and memory entries, followed by an 'end' entry.  */

static inline void
record_full_exec_insn (struct regcache *regcache,
		       struct gdbarch *gdbarch,
		       struct record_full_entry *entry)
{
  switch (entry->type)
    {
    case record_full_reg: /* reg */
      {
        gdb_byte reg[MAX_REGISTER_SIZE];

        if (record_debug > 1)
          fprintf_unfiltered (gdb_stdlog,
                              "Process record: record_full_reg %s to "
                              "inferior num = %d.\n",
                              host_address_to_string (entry),
                              entry->u.reg.num);

        regcache_cooked_read (regcache, entry->u.reg.num, reg);
        regcache_cooked_write (regcache, entry->u.reg.num, 
			       record_full_get_loc (entry));
        memcpy (record_full_get_loc (entry), reg, entry->u.reg.len);
      }
      break;

    case record_full_mem: /* mem */
      {
	/* Nothing to do if the entry is flagged not_accessible.  */
        if (!entry->u.mem.mem_entry_not_accessible)
          {
            gdb_byte *mem = (gdb_byte *) xmalloc (entry->u.mem.len);
            struct cleanup *cleanup = make_cleanup (xfree, mem);

            if (record_debug > 1)
              fprintf_unfiltered (gdb_stdlog,
                                  "Process record: record_full_mem %s to "
                                  "inferior addr = %s len = %d.\n",
                                  host_address_to_string (entry),
                                  paddress (gdbarch, entry->u.mem.addr),
                                  entry->u.mem.len);

            if (record_read_memory (gdbarch,
				    entry->u.mem.addr, mem, entry->u.mem.len))
	      entry->u.mem.mem_entry_not_accessible = 1;
            else
              {
                if (target_write_memory (entry->u.mem.addr, 
					 record_full_get_loc (entry),
					 entry->u.mem.len))
                  {
                    entry->u.mem.mem_entry_not_accessible = 1;
                    if (record_debug)
                      warning (_("Process record: error writing memory at "
				 "addr = %s len = %d."),
                               paddress (gdbarch, entry->u.mem.addr),
                               entry->u.mem.len);
                  }
                else
		  {
		    memcpy (record_full_get_loc (entry), mem,
			    entry->u.mem.len);

		    /* We've changed memory --- check if a hardware
		       watchpoint should trap.  Note that this
		       presently assumes the target beneath supports
		       continuable watchpoints.  On non-continuable
		       watchpoints target, we'll want to check this
		       _before_ actually doing the memory change, and
		       not doing the change at all if the watchpoint
		       traps.  */
		    if (hardware_watchpoint_inserted_in_range
			(get_regcache_aspace (regcache),
			 entry->u.mem.addr, entry->u.mem.len))
		      record_full_stop_reason = TARGET_STOPPED_BY_WATCHPOINT;
		  }
              }

	    do_cleanups (cleanup);
          }
      }
      break;
    }
}

static void record_full_restore (void);

/* Asynchronous signal handle registered as event loop source for when
   we have pending events ready to be passed to the core.  */

static struct async_event_handler *record_full_async_inferior_event_token;

static void
record_full_async_inferior_event_handler (gdb_client_data data)
{
  inferior_event_handler (INF_REG_EVENT, NULL);
}

/* Open the process record target.  */

static void
record_full_core_open_1 (const char *name, int from_tty)
{
  struct regcache *regcache = get_current_regcache ();
  int regnum = gdbarch_num_regs (get_regcache_arch (regcache));
  int i;

  /* Get record_full_core_regbuf.  */
  target_fetch_registers (regcache, -1);
  record_full_core_regbuf = (gdb_byte *) xmalloc (MAX_REGISTER_SIZE * regnum);
  for (i = 0; i < regnum; i ++)
    regcache_raw_collect (regcache, i,
			  record_full_core_regbuf + MAX_REGISTER_SIZE * i);

  /* Get record_full_core_start and record_full_core_end.  */
  if (build_section_table (core_bfd, &record_full_core_start,
			   &record_full_core_end))
    {
      xfree (record_full_core_regbuf);
      record_full_core_regbuf = NULL;
      error (_("\"%s\": Can't find sections: %s"),
	     bfd_get_filename (core_bfd), bfd_errmsg (bfd_get_error ()));
    }

  push_target (&record_full_core_ops);
  record_full_restore ();
}

/* "to_open" target method for 'live' processes.  */

static void
record_full_open_1 (const char *name, int from_tty)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_full_open_1\n");

  /* check exec */
  if (!target_has_execution)
    error (_("Process record: the program is not being run."));
  if (non_stop)
    error (_("Process record target can't debug inferior in non-stop mode "
	     "(non-stop)."));

  if (!gdbarch_process_record_p (target_gdbarch ()))
    error (_("Process record: the current architecture doesn't support "
	     "record function."));

  push_target (&record_full_ops);
}

static void record_full_init_record_breakpoints (void);

/* "to_open" target method.  Open the process record target.  */

static void
record_full_open (const char *name, int from_tty)
{
  struct target_ops *t;

  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_full_open\n");

  record_preopen ();

  /* Reset */
  record_full_insn_num = 0;
  record_full_insn_count = 0;
  record_full_list = &record_full_first;
  record_full_list->next = NULL;

  if (core_bfd)
    record_full_core_open_1 (name, from_tty);
  else
    record_full_open_1 (name, from_tty);

  /* Register extra event sources in the event loop.  */
  record_full_async_inferior_event_token
    = create_async_event_handler (record_full_async_inferior_event_handler,
				  NULL);

  record_full_init_record_breakpoints ();

  observer_notify_record_changed (current_inferior (),  1, "full", NULL);
}

/* "to_close" target method.  Close the process record target.  */

static void
record_full_close (struct target_ops *self)
{
  struct record_full_core_buf_entry *entry;

  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_full_close\n");

  record_full_list_release (record_full_list);

  /* Release record_full_core_regbuf.  */
  if (record_full_core_regbuf)
    {
      xfree (record_full_core_regbuf);
      record_full_core_regbuf = NULL;
    }

  /* Release record_full_core_buf_list.  */
  if (record_full_core_buf_list)
    {
      for (entry = record_full_core_buf_list->prev; entry;
	   entry = entry->prev)
	{
	  xfree (record_full_core_buf_list);
	  record_full_core_buf_list = entry;
	}
      record_full_core_buf_list = NULL;
    }

  if (record_full_async_inferior_event_token)
    delete_async_event_handler (&record_full_async_inferior_event_token);
}

/* "to_async" target method.  */

static void
record_full_async (struct target_ops *ops, int enable)
{
  if (enable)
    mark_async_event_handler (record_full_async_inferior_event_token);
  else
    clear_async_event_handler (record_full_async_inferior_event_token);

  ops->beneath->to_async (ops->beneath, enable);
}

static int record_full_resume_step = 0;

/* True if we've been resumed, and so each record_full_wait call should
   advance execution.  If this is false, record_full_wait will return a
   TARGET_WAITKIND_IGNORE.  */
static int record_full_resumed = 0;

/* The execution direction of the last resume we got.  This is
   necessary for async mode.  Vis (order is not strictly accurate):

   1. user has the global execution direction set to forward
   2. user does a reverse-step command
   3. record_full_resume is called with global execution direction
      temporarily switched to reverse
   4. GDB's execution direction is reverted back to forward
   5. target record notifies event loop there's an event to handle
   6. infrun asks the target which direction was it going, and switches
      the global execution direction accordingly (to reverse)
   7. infrun polls an event out of the record target, and handles it
   8. GDB goes back to the event loop, and goto #4.
*/
static enum exec_direction_kind record_full_execution_dir = EXEC_FORWARD;

/* "to_resume" target method.  Resume the process record target.  */

static void
record_full_resume (struct target_ops *ops, ptid_t ptid, int step,
		    enum gdb_signal signal)
{
  record_full_resume_step = step;
  record_full_resumed = 1;
  record_full_execution_dir = execution_direction;

  if (!RECORD_FULL_IS_REPLAY)
    {
      struct gdbarch *gdbarch = target_thread_architecture (ptid);

      record_full_message (get_current_regcache (), signal);

      if (!step)
        {
          /* This is not hard single step.  */
          if (!gdbarch_software_single_step_p (gdbarch))
            {
              /* This is a normal continue.  */
              step = 1;
            }
          else
            {
              /* This arch supports soft single step.  */
              if (thread_has_single_step_breakpoints_set (inferior_thread ()))
                {
                  /* This is a soft single step.  */
                  record_full_resume_step = 1;
                }
              else
		step = !insert_single_step_breakpoints (gdbarch);
            }
        }

      /* Make sure the target beneath reports all signals.  */
      target_pass_signals (0, NULL);

      ops->beneath->to_resume (ops->beneath, ptid, step, signal);
    }

  /* We are about to start executing the inferior (or simulate it),
     let's register it with the event loop.  */
  if (target_can_async_p ())
    target_async (1);
}

/* "to_commit_resume" method for process record target.  */

static void
record_full_commit_resume (struct target_ops *ops)
{
  if (!RECORD_FULL_IS_REPLAY)
    ops->beneath->to_commit_resume (ops->beneath);
}

static int record_full_get_sig = 0;

/* SIGINT signal handler, registered by "to_wait" method.  */

static void
record_full_sig_handler (int signo)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: get a signal\n");

  /* It will break the running inferior in replay mode.  */
  record_full_resume_step = 1;

  /* It will let record_full_wait set inferior status to get the signal
     SIGINT.  */
  record_full_get_sig = 1;
}

static void
record_full_wait_cleanups (void *ignore)
{
  if (execution_direction == EXEC_REVERSE)
    {
      if (record_full_list->next)
	record_full_list = record_full_list->next;
    }
  else
    record_full_list = record_full_list->prev;
}

/* "to_wait" target method for process record target.

   In record mode, the target is always run in singlestep mode
   (even when gdb says to continue).  The to_wait method intercepts
   the stop events and determines which ones are to be passed on to
   gdb.  Most stop events are just singlestep events that gdb is not
   to know about, so the to_wait method just records them and keeps
   singlestepping.

   In replay mode, this function emulates the recorded execution log, 
   one instruction at a time (forward or backward), and determines 
   where to stop.  */

static ptid_t
record_full_wait_1 (struct target_ops *ops,
		    ptid_t ptid, struct target_waitstatus *status,
		    int options)
{
  struct cleanup *set_cleanups = record_full_gdb_operation_disable_set ();

  if (record_debug)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: record_full_wait "
			"record_full_resume_step = %d, "
			"record_full_resumed = %d, direction=%s\n",
			record_full_resume_step, record_full_resumed,
			record_full_execution_dir == EXEC_FORWARD
			? "forward" : "reverse");

  if (!record_full_resumed)
    {
      gdb_assert ((options & TARGET_WNOHANG) != 0);

      /* No interesting event.  */
      status->kind = TARGET_WAITKIND_IGNORE;
      return minus_one_ptid;
    }

  record_full_get_sig = 0;
  signal (SIGINT, record_full_sig_handler);

  record_full_stop_reason = TARGET_STOPPED_BY_NO_REASON;

  if (!RECORD_FULL_IS_REPLAY && ops != &record_full_core_ops)
    {
      if (record_full_resume_step)
	{
	  /* This is a single step.  */
	  return ops->beneath->to_wait (ops->beneath, ptid, status, options);
	}
      else
	{
	  /* This is not a single step.  */
	  ptid_t ret;
	  CORE_ADDR tmp_pc;
	  struct gdbarch *gdbarch = target_thread_architecture (inferior_ptid);

	  while (1)
	    {
	      struct thread_info *tp;

	      ret = ops->beneath->to_wait (ops->beneath, ptid, status, options);
	      if (status->kind == TARGET_WAITKIND_IGNORE)
		{
		  if (record_debug)
		    fprintf_unfiltered (gdb_stdlog,
					"Process record: record_full_wait "
					"target beneath not done yet\n");
		  return ret;
		}

	      ALL_NON_EXITED_THREADS (tp)
                delete_single_step_breakpoints (tp);

	      if (record_full_resume_step)
		return ret;

	      /* Is this a SIGTRAP?  */
	      if (status->kind == TARGET_WAITKIND_STOPPED
		  && status->value.sig == GDB_SIGNAL_TRAP)
		{
		  struct regcache *regcache;
		  struct address_space *aspace;
		  enum target_stop_reason *stop_reason_p
		    = &record_full_stop_reason;

		  /* Yes -- this is likely our single-step finishing,
		     but check if there's any reason the core would be
		     interested in the event.  */

		  registers_changed ();
		  regcache = get_current_regcache ();
		  tmp_pc = regcache_read_pc (regcache);
		  aspace = get_regcache_aspace (regcache);

		  if (target_stopped_by_watchpoint ())
		    {
		      /* Always interested in watchpoints.  */
		    }
		  else if (record_check_stopped_by_breakpoint (aspace, tmp_pc,
							       stop_reason_p))
		    {
		      /* There is a breakpoint here.  Let the core
			 handle it.  */
		    }
		  else
		    {
		      /* This is a single-step trap.  Record the
		         insn and issue another step.
                         FIXME: this part can be a random SIGTRAP too.
                         But GDB cannot handle it.  */
                      int step = 1;

		      if (!record_full_message_wrapper_safe (regcache,
							     GDB_SIGNAL_0))
  			{
                           status->kind = TARGET_WAITKIND_STOPPED;
                           status->value.sig = GDB_SIGNAL_0;
                           break;
  			}

                      if (gdbarch_software_single_step_p (gdbarch))
			{
			  /* Try to insert the software single step breakpoint.
			     If insert success, set step to 0.  */
			  set_executing (inferior_ptid, 0);
			  reinit_frame_cache ();

			  step = !insert_single_step_breakpoints (gdbarch);

			  set_executing (inferior_ptid, 1);
			}

		      if (record_debug)
			fprintf_unfiltered (gdb_stdlog,
					    "Process record: record_full_wait "
					    "issuing one more step in the "
					    "target beneath\n");
		      ops->beneath->to_resume (ops->beneath, ptid, step,
					       GDB_SIGNAL_0);
		      ops->beneath->to_commit_resume (ops->beneath);
		      continue;
		    }
		}

	      /* The inferior is broken by a breakpoint or a signal.  */
	      break;
	    }

	  return ret;
	}
    }
  else
    {
      struct regcache *regcache = get_current_regcache ();
      struct gdbarch *gdbarch = get_regcache_arch (regcache);
      struct address_space *aspace = get_regcache_aspace (regcache);
      int continue_flag = 1;
      int first_record_full_end = 1;
      struct cleanup *old_cleanups
	= make_cleanup (record_full_wait_cleanups, 0);
      CORE_ADDR tmp_pc;

      record_full_stop_reason = TARGET_STOPPED_BY_NO_REASON;
      status->kind = TARGET_WAITKIND_STOPPED;

      /* Check breakpoint when forward execute.  */
      if (execution_direction == EXEC_FORWARD)
	{
	  tmp_pc = regcache_read_pc (regcache);
	  if (record_check_stopped_by_breakpoint (aspace, tmp_pc,
						  &record_full_stop_reason))
	    {
	      if (record_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "Process record: break at %s.\n",
				    paddress (gdbarch, tmp_pc));
	      goto replay_out;
	    }
	}

      /* If GDB is in terminal_inferior mode, it will not get the signal.
         And in GDB replay mode, GDB doesn't need to be in terminal_inferior
         mode, because inferior will not executed.
         Then set it to terminal_ours to make GDB get the signal.  */
      target_terminal_ours ();

      /* In EXEC_FORWARD mode, record_full_list points to the tail of prev
         instruction.  */
      if (execution_direction == EXEC_FORWARD && record_full_list->next)
	record_full_list = record_full_list->next;

      /* Loop over the record_full_list, looking for the next place to
	 stop.  */
      do
	{
	  /* Check for beginning and end of log.  */
	  if (execution_direction == EXEC_REVERSE
	      && record_full_list == &record_full_first)
	    {
	      /* Hit beginning of record log in reverse.  */
	      status->kind = TARGET_WAITKIND_NO_HISTORY;
	      break;
	    }
	  if (execution_direction != EXEC_REVERSE && !record_full_list->next)
	    {
	      /* Hit end of record log going forward.  */
	      status->kind = TARGET_WAITKIND_NO_HISTORY;
	      break;
	    }

          record_full_exec_insn (regcache, gdbarch, record_full_list);

	  if (record_full_list->type == record_full_end)
	    {
	      if (record_debug > 1)
		fprintf_unfiltered (gdb_stdlog,
				    "Process record: record_full_end %s to "
				    "inferior.\n",
				    host_address_to_string (record_full_list));

	      if (first_record_full_end && execution_direction == EXEC_REVERSE)
		{
		  /* When reverse excute, the first record_full_end is the
		     part of current instruction.  */
		  first_record_full_end = 0;
		}
	      else
		{
		  /* In EXEC_REVERSE mode, this is the record_full_end of prev
		     instruction.
		     In EXEC_FORWARD mode, this is the record_full_end of
		     current instruction.  */
		  /* step */
		  if (record_full_resume_step)
		    {
		      if (record_debug > 1)
			fprintf_unfiltered (gdb_stdlog,
					    "Process record: step.\n");
		      continue_flag = 0;
		    }

		  /* check breakpoint */
		  tmp_pc = regcache_read_pc (regcache);
		  if (record_check_stopped_by_breakpoint (aspace, tmp_pc,
							  &record_full_stop_reason))
		    {
		      if (record_debug)
			fprintf_unfiltered (gdb_stdlog,
					    "Process record: break "
					    "at %s.\n",
					    paddress (gdbarch, tmp_pc));

		      continue_flag = 0;
		    }

		  if (record_full_stop_reason == TARGET_STOPPED_BY_WATCHPOINT)
		    {
		      if (record_debug)
			fprintf_unfiltered (gdb_stdlog,
					    "Process record: hit hw "
					    "watchpoint.\n");
		      continue_flag = 0;
		    }
		  /* Check target signal */
		  if (record_full_list->u.end.sigval != GDB_SIGNAL_0)
		    /* FIXME: better way to check */
		    continue_flag = 0;
		}
	    }

	  if (continue_flag)
	    {
	      if (execution_direction == EXEC_REVERSE)
		{
		  if (record_full_list->prev)
		    record_full_list = record_full_list->prev;
		}
	      else
		{
		  if (record_full_list->next)
		    record_full_list = record_full_list->next;
		}
	    }
	}
      while (continue_flag);

replay_out:
      if (record_full_get_sig)
	status->value.sig = GDB_SIGNAL_INT;
      else if (record_full_list->u.end.sigval != GDB_SIGNAL_0)
	/* FIXME: better way to check */
	status->value.sig = record_full_list->u.end.sigval;
      else
	status->value.sig = GDB_SIGNAL_TRAP;

      discard_cleanups (old_cleanups);
    }

  signal (SIGINT, handle_sigint);

  do_cleanups (set_cleanups);
  return inferior_ptid;
}

static ptid_t
record_full_wait (struct target_ops *ops,
		  ptid_t ptid, struct target_waitstatus *status,
		  int options)
{
  ptid_t return_ptid;

  return_ptid = record_full_wait_1 (ops, ptid, status, options);
  if (status->kind != TARGET_WAITKIND_IGNORE)
    {
      /* We're reporting a stop.  Make sure any spurious
	 target_wait(WNOHANG) doesn't advance the target until the
	 core wants us resumed again.  */
      record_full_resumed = 0;
    }
  return return_ptid;
}

static int
record_full_stopped_by_watchpoint (struct target_ops *ops)
{
  if (RECORD_FULL_IS_REPLAY)
    return record_full_stop_reason == TARGET_STOPPED_BY_WATCHPOINT;
  else
    return ops->beneath->to_stopped_by_watchpoint (ops->beneath);
}

static int
record_full_stopped_data_address (struct target_ops *ops, CORE_ADDR *addr_p)
{
  if (RECORD_FULL_IS_REPLAY)
    return 0;
  else
    return ops->beneath->to_stopped_data_address (ops->beneath, addr_p);
}

/* The to_stopped_by_sw_breakpoint method of target record-full.  */

static int
record_full_stopped_by_sw_breakpoint (struct target_ops *ops)
{
  return record_full_stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT;
}

/* The to_supports_stopped_by_sw_breakpoint method of target
   record-full.  */

static int
record_full_supports_stopped_by_sw_breakpoint (struct target_ops *ops)
{
  return 1;
}

/* The to_stopped_by_hw_breakpoint method of target record-full.  */

static int
record_full_stopped_by_hw_breakpoint (struct target_ops *ops)
{
  return record_full_stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT;
}

/* The to_supports_stopped_by_sw_breakpoint method of target
   record-full.  */

static int
record_full_supports_stopped_by_hw_breakpoint (struct target_ops *ops)
{
  return 1;
}

/* Record registers change (by user or by GDB) to list as an instruction.  */

static void
record_full_registers_change (struct regcache *regcache, int regnum)
{
  /* Check record_full_insn_num.  */
  record_full_check_insn_num ();

  record_full_arch_list_head = NULL;
  record_full_arch_list_tail = NULL;

  if (regnum < 0)
    {
      int i;

      for (i = 0; i < gdbarch_num_regs (get_regcache_arch (regcache)); i++)
	{
	  if (record_full_arch_list_add_reg (regcache, i))
	    {
	      record_full_list_release (record_full_arch_list_tail);
	      error (_("Process record: failed to record execution log."));
	    }
	}
    }
  else
    {
      if (record_full_arch_list_add_reg (regcache, regnum))
	{
	  record_full_list_release (record_full_arch_list_tail);
	  error (_("Process record: failed to record execution log."));
	}
    }
  if (record_full_arch_list_add_end ())
    {
      record_full_list_release (record_full_arch_list_tail);
      error (_("Process record: failed to record execution log."));
    }
  record_full_list->next = record_full_arch_list_head;
  record_full_arch_list_head->prev = record_full_list;
  record_full_list = record_full_arch_list_tail;

  if (record_full_insn_num == record_full_insn_max_num)
    record_full_list_release_first ();
  else
    record_full_insn_num++;
}

/* "to_store_registers" method for process record target.  */

static void
record_full_store_registers (struct target_ops *ops,
			     struct regcache *regcache,
			     int regno)
{
  if (!record_full_gdb_operation_disable)
    {
      if (RECORD_FULL_IS_REPLAY)
	{
	  int n;

	  /* Let user choose if he wants to write register or not.  */
	  if (regno < 0)
	    n =
	      query (_("Because GDB is in replay mode, changing the "
		       "value of a register will make the execution "
		       "log unusable from this point onward.  "
		       "Change all registers?"));
	  else
	    n =
	      query (_("Because GDB is in replay mode, changing the value "
		       "of a register will make the execution log unusable "
		       "from this point onward.  Change register %s?"),
		      gdbarch_register_name (get_regcache_arch (regcache),
					       regno));

	  if (!n)
	    {
	      /* Invalidate the value of regcache that was set in function
	         "regcache_raw_write".  */
	      if (regno < 0)
		{
		  int i;

		  for (i = 0;
		       i < gdbarch_num_regs (get_regcache_arch (regcache));
		       i++)
		    regcache_invalidate (regcache, i);
		}
	      else
		regcache_invalidate (regcache, regno);

	      error (_("Process record canceled the operation."));
	    }

	  /* Destroy the record from here forward.  */
	  record_full_list_release_following (record_full_list);
	}

      record_full_registers_change (regcache, regno);
    }
  ops->beneath->to_store_registers (ops->beneath, regcache, regno);
}

/* "to_xfer_partial" method.  Behavior is conditional on
   RECORD_FULL_IS_REPLAY.
   In replay mode, we cannot write memory unles we are willing to
   invalidate the record/replay log from this point forward.  */

static enum target_xfer_status
record_full_xfer_partial (struct target_ops *ops, enum target_object object,
			  const char *annex, gdb_byte *readbuf,
			  const gdb_byte *writebuf, ULONGEST offset,
			  ULONGEST len, ULONGEST *xfered_len)
{
  if (!record_full_gdb_operation_disable
      && (object == TARGET_OBJECT_MEMORY
	  || object == TARGET_OBJECT_RAW_MEMORY) && writebuf)
    {
      if (RECORD_FULL_IS_REPLAY)
	{
	  /* Let user choose if he wants to write memory or not.  */
	  if (!query (_("Because GDB is in replay mode, writing to memory "
		        "will make the execution log unusable from this "
		        "point onward.  Write memory at address %s?"),
		       paddress (target_gdbarch (), offset)))
	    error (_("Process record canceled the operation."));

	  /* Destroy the record from here forward.  */
	  record_full_list_release_following (record_full_list);
	}

      /* Check record_full_insn_num */
      record_full_check_insn_num ();

      /* Record registers change to list as an instruction.  */
      record_full_arch_list_head = NULL;
      record_full_arch_list_tail = NULL;
      if (record_full_arch_list_add_mem (offset, len))
	{
	  record_full_list_release (record_full_arch_list_tail);
	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"Process record: failed to record "
				"execution log.");
	  return TARGET_XFER_E_IO;
	}
      if (record_full_arch_list_add_end ())
	{
	  record_full_list_release (record_full_arch_list_tail);
	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"Process record: failed to record "
				"execution log.");
	  return TARGET_XFER_E_IO;
	}
      record_full_list->next = record_full_arch_list_head;
      record_full_arch_list_head->prev = record_full_list;
      record_full_list = record_full_arch_list_tail;

      if (record_full_insn_num == record_full_insn_max_num)
	record_full_list_release_first ();
      else
	record_full_insn_num++;
    }

  return ops->beneath->to_xfer_partial (ops->beneath, object, annex,
					readbuf, writebuf, offset,
					len, xfered_len);
}

/* This structure represents a breakpoint inserted while the record
   target is active.  We use this to know when to install/remove
   breakpoints in/from the target beneath.  For example, a breakpoint
   may be inserted while recording, but removed when not replaying nor
   recording.  In that case, the breakpoint had not been inserted on
   the target beneath, so we should not try to remove it there.  */

struct record_full_breakpoint
{
  /* The address and address space the breakpoint was set at.  */
  struct address_space *address_space;
  CORE_ADDR addr;

  /* True when the breakpoint has been also installed in the target
     beneath.  This will be false for breakpoints set during replay or
     when recording.  */
  int in_target_beneath;
};

typedef struct record_full_breakpoint *record_full_breakpoint_p;
DEF_VEC_P(record_full_breakpoint_p);

/* The list of breakpoints inserted while the record target is
   active.  */
VEC(record_full_breakpoint_p) *record_full_breakpoints = NULL;

static void
record_full_sync_record_breakpoints (struct bp_location *loc, void *data)
{
  if (loc->loc_type != bp_loc_software_breakpoint)
      return;

  if (loc->inserted)
    {
      struct record_full_breakpoint *bp = XNEW (struct record_full_breakpoint);

      bp->addr = loc->target_info.placed_address;
      bp->address_space = loc->target_info.placed_address_space;

      bp->in_target_beneath = 1;

      VEC_safe_push (record_full_breakpoint_p, record_full_breakpoints, bp);
    }
}

/* Sync existing breakpoints to record_full_breakpoints.  */

static void
record_full_init_record_breakpoints (void)
{
  VEC_free (record_full_breakpoint_p, record_full_breakpoints);

  iterate_over_bp_locations (record_full_sync_record_breakpoints);
}

/* Behavior is conditional on RECORD_FULL_IS_REPLAY.  We will not actually
   insert or remove breakpoints in the real target when replaying, nor
   when recording.  */

static int
record_full_insert_breakpoint (struct target_ops *ops,
			       struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  struct record_full_breakpoint *bp;
  int in_target_beneath = 0;
  int ix;

  if (!RECORD_FULL_IS_REPLAY)
    {
      /* When recording, we currently always single-step, so we don't
	 really need to install regular breakpoints in the inferior.
	 However, we do have to insert software single-step
	 breakpoints, in case the target can't hardware step.  To keep
	 things simple, we always insert.  */
      struct cleanup *old_cleanups;
      int ret;

      old_cleanups = record_full_gdb_operation_disable_set ();
      ret = ops->beneath->to_insert_breakpoint (ops->beneath, gdbarch, bp_tgt);
      do_cleanups (old_cleanups);

      if (ret != 0)
	return ret;

      in_target_beneath = 1;
    }

  /* Use the existing entries if found in order to avoid duplication
     in record_full_breakpoints.  */

  for (ix = 0;
       VEC_iterate (record_full_breakpoint_p,
		    record_full_breakpoints, ix, bp);
       ++ix)
    {
      if (bp->addr == bp_tgt->placed_address
	  && bp->address_space == bp_tgt->placed_address_space)
	{
	  gdb_assert (bp->in_target_beneath == in_target_beneath);
	  return 0;
	}
    }

  bp = XNEW (struct record_full_breakpoint);
  bp->addr = bp_tgt->placed_address;
  bp->address_space = bp_tgt->placed_address_space;
  bp->in_target_beneath = in_target_beneath;
  VEC_safe_push (record_full_breakpoint_p, record_full_breakpoints, bp);
  return 0;
}

/* "to_remove_breakpoint" method for process record target.  */

static int
record_full_remove_breakpoint (struct target_ops *ops,
			       struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt,
			       enum remove_bp_reason reason)
{
  struct record_full_breakpoint *bp;
  int ix;

  for (ix = 0;
       VEC_iterate (record_full_breakpoint_p,
		    record_full_breakpoints, ix, bp);
       ++ix)
    {
      if (bp->addr == bp_tgt->placed_address
	  && bp->address_space == bp_tgt->placed_address_space)
	{
	  if (bp->in_target_beneath)
	    {
	      struct cleanup *old_cleanups;
	      int ret;

	      old_cleanups = record_full_gdb_operation_disable_set ();
	      ret = ops->beneath->to_remove_breakpoint (ops->beneath, gdbarch,
							bp_tgt, reason);
	      do_cleanups (old_cleanups);

	      if (ret != 0)
		return ret;
	    }

	  if (reason == REMOVE_BREAKPOINT)
	    {
	      VEC_unordered_remove (record_full_breakpoint_p,
				    record_full_breakpoints, ix);
	    }
	  return 0;
	}
    }

  gdb_assert_not_reached ("removing unknown breakpoint");
}

/* "to_can_execute_reverse" method for process record target.  */

static int
record_full_can_execute_reverse (struct target_ops *self)
{
  return 1;
}

/* "to_get_bookmark" method for process record and prec over core.  */

static gdb_byte *
record_full_get_bookmark (struct target_ops *self, const char *args,
			  int from_tty)
{
  char *ret = NULL;

  /* Return stringified form of instruction count.  */
  if (record_full_list && record_full_list->type == record_full_end)
    ret = xstrdup (pulongest (record_full_list->u.end.insn_num));

  if (record_debug)
    {
      if (ret)
	fprintf_unfiltered (gdb_stdlog,
			    "record_full_get_bookmark returns %s\n", ret);
      else
	fprintf_unfiltered (gdb_stdlog,
			    "record_full_get_bookmark returns NULL\n");
    }
  return (gdb_byte *) ret;
}

/* "to_goto_bookmark" method for process record and prec over core.  */

static void
record_full_goto_bookmark (struct target_ops *self,
			   const gdb_byte *raw_bookmark, int from_tty)
{
  const char *bookmark = (const char *) raw_bookmark;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);

  if (record_debug)
    fprintf_unfiltered (gdb_stdlog,
			"record_full_goto_bookmark receives %s\n", bookmark);

  if (bookmark[0] == '\'' || bookmark[0] == '\"')
    {
      char *copy;

      if (bookmark[strlen (bookmark) - 1] != bookmark[0])
	error (_("Unbalanced quotes: %s"), bookmark);


      copy = savestring (bookmark + 1, strlen (bookmark) - 2);
      make_cleanup (xfree, copy);
      bookmark = copy;
    }

  record_goto (bookmark);

  do_cleanups (cleanup);
}

static enum exec_direction_kind
record_full_execution_direction (struct target_ops *self)
{
  return record_full_execution_dir;
}

/* The to_record_method method of target record-full.  */

enum record_method
record_full_record_method (struct target_ops *self, ptid_t ptid)
{
  return RECORD_METHOD_FULL;
}

static void
record_full_info (struct target_ops *self)
{
  struct record_full_entry *p;

  if (RECORD_FULL_IS_REPLAY)
    printf_filtered (_("Replay mode:\n"));
  else
    printf_filtered (_("Record mode:\n"));

  /* Find entry for first actual instruction in the log.  */
  for (p = record_full_first.next;
       p != NULL && p->type != record_full_end;
       p = p->next)
    ;

  /* Do we have a log at all?  */
  if (p != NULL && p->type == record_full_end)
    {
      /* Display instruction number for first instruction in the log.  */
      printf_filtered (_("Lowest recorded instruction number is %s.\n"),
		       pulongest (p->u.end.insn_num));

      /* If in replay mode, display where we are in the log.  */
      if (RECORD_FULL_IS_REPLAY)
	printf_filtered (_("Current instruction number is %s.\n"),
			 pulongest (record_full_list->u.end.insn_num));

      /* Display instruction number for last instruction in the log.  */
      printf_filtered (_("Highest recorded instruction number is %s.\n"),
		       pulongest (record_full_insn_count));

      /* Display log count.  */
      printf_filtered (_("Log contains %u instructions.\n"),
		       record_full_insn_num);
    }
  else
    printf_filtered (_("No instructions have been logged.\n"));

  /* Display max log size.  */
  printf_filtered (_("Max logged instructions is %u.\n"),
		   record_full_insn_max_num);
}

/* The "to_record_delete" target method.  */

static void
record_full_delete (struct target_ops *self)
{
  record_full_list_release_following (record_full_list);
}

/* The "to_record_is_replaying" target method.  */

static int
record_full_is_replaying (struct target_ops *self, ptid_t ptid)
{
  return RECORD_FULL_IS_REPLAY;
}

/* The "to_record_will_replay" target method.  */

static int
record_full_will_replay (struct target_ops *self, ptid_t ptid, int dir)
{
  /* We can currently only record when executing forwards.  Should we be able
     to record when executing backwards on targets that support reverse
     execution, this needs to be changed.  */

  return RECORD_FULL_IS_REPLAY || dir == EXEC_REVERSE;
}

/* Go to a specific entry.  */

static void
record_full_goto_entry (struct record_full_entry *p)
{
  if (p == NULL)
    error (_("Target insn not found."));
  else if (p == record_full_list)
    error (_("Already at target insn."));
  else if (p->u.end.insn_num > record_full_list->u.end.insn_num)
    {
      printf_filtered (_("Go forward to insn number %s\n"),
		       pulongest (p->u.end.insn_num));
      record_full_goto_insn (p, EXEC_FORWARD);
    }
  else
    {
      printf_filtered (_("Go backward to insn number %s\n"),
		       pulongest (p->u.end.insn_num));
      record_full_goto_insn (p, EXEC_REVERSE);
    }

  registers_changed ();
  reinit_frame_cache ();
  stop_pc = regcache_read_pc (get_current_regcache ());
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* The "to_goto_record_begin" target method.  */

static void
record_full_goto_begin (struct target_ops *self)
{
  struct record_full_entry *p = NULL;

  for (p = &record_full_first; p != NULL; p = p->next)
    if (p->type == record_full_end)
      break;

  record_full_goto_entry (p);
}

/* The "to_goto_record_end" target method.  */

static void
record_full_goto_end (struct target_ops *self)
{
  struct record_full_entry *p = NULL;

  for (p = record_full_list; p->next != NULL; p = p->next)
    ;
  for (; p!= NULL; p = p->prev)
    if (p->type == record_full_end)
      break;

  record_full_goto_entry (p);
}

/* The "to_goto_record" target method.  */

static void
record_full_goto (struct target_ops *self, ULONGEST target_insn)
{
  struct record_full_entry *p = NULL;

  for (p = &record_full_first; p != NULL; p = p->next)
    if (p->type == record_full_end && p->u.end.insn_num == target_insn)
      break;

  record_full_goto_entry (p);
}

/* The "to_record_stop_replaying" target method.  */

static void
record_full_stop_replaying (struct target_ops *self)
{
  record_full_goto_end (self);
}

static void
init_record_full_ops (void)
{
  record_full_ops.to_shortname = "record-full";
  record_full_ops.to_longname = "Process record and replay target";
  record_full_ops.to_doc =
    "Log program while executing and replay execution from log.";
  record_full_ops.to_open = record_full_open;
  record_full_ops.to_close = record_full_close;
  record_full_ops.to_async = record_full_async;
  record_full_ops.to_resume = record_full_resume;
  record_full_ops.to_commit_resume = record_full_commit_resume;
  record_full_ops.to_wait = record_full_wait;
  record_full_ops.to_disconnect = record_disconnect;
  record_full_ops.to_detach = record_detach;
  record_full_ops.to_mourn_inferior = record_mourn_inferior;
  record_full_ops.to_kill = record_kill;
  record_full_ops.to_store_registers = record_full_store_registers;
  record_full_ops.to_xfer_partial = record_full_xfer_partial;
  record_full_ops.to_insert_breakpoint = record_full_insert_breakpoint;
  record_full_ops.to_remove_breakpoint = record_full_remove_breakpoint;
  record_full_ops.to_stopped_by_watchpoint = record_full_stopped_by_watchpoint;
  record_full_ops.to_stopped_data_address = record_full_stopped_data_address;
  record_full_ops.to_stopped_by_sw_breakpoint
    = record_full_stopped_by_sw_breakpoint;
  record_full_ops.to_supports_stopped_by_sw_breakpoint
    = record_full_supports_stopped_by_sw_breakpoint;
  record_full_ops.to_stopped_by_hw_breakpoint
    = record_full_stopped_by_hw_breakpoint;
  record_full_ops.to_supports_stopped_by_hw_breakpoint
    = record_full_supports_stopped_by_hw_breakpoint;
  record_full_ops.to_can_execute_reverse = record_full_can_execute_reverse;
  record_full_ops.to_stratum = record_stratum;
  /* Add bookmark target methods.  */
  record_full_ops.to_get_bookmark = record_full_get_bookmark;
  record_full_ops.to_goto_bookmark = record_full_goto_bookmark;
  record_full_ops.to_execution_direction = record_full_execution_direction;
  record_full_ops.to_record_method = record_full_record_method;
  record_full_ops.to_info_record = record_full_info;
  record_full_ops.to_save_record = record_full_save;
  record_full_ops.to_delete_record = record_full_delete;
  record_full_ops.to_record_is_replaying = record_full_is_replaying;
  record_full_ops.to_record_will_replay = record_full_will_replay;
  record_full_ops.to_record_stop_replaying = record_full_stop_replaying;
  record_full_ops.to_goto_record_begin = record_full_goto_begin;
  record_full_ops.to_goto_record_end = record_full_goto_end;
  record_full_ops.to_goto_record = record_full_goto;
  record_full_ops.to_magic = OPS_MAGIC;
}

/* "to_resume" method for prec over corefile.  */

static void
record_full_core_resume (struct target_ops *ops, ptid_t ptid, int step,
			 enum gdb_signal signal)
{
  record_full_resume_step = step;
  record_full_resumed = 1;
  record_full_execution_dir = execution_direction;

  /* We are about to start executing the inferior (or simulate it),
     let's register it with the event loop.  */
  if (target_can_async_p ())
    target_async (1);
}

/* "to_kill" method for prec over corefile.  */

static void
record_full_core_kill (struct target_ops *ops)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_full_core_kill\n");

  unpush_target (&record_full_core_ops);
}

/* "to_fetch_registers" method for prec over corefile.  */

static void
record_full_core_fetch_registers (struct target_ops *ops,
				  struct regcache *regcache,
				  int regno)
{
  if (regno < 0)
    {
      int num = gdbarch_num_regs (get_regcache_arch (regcache));
      int i;

      for (i = 0; i < num; i ++)
        regcache_raw_supply (regcache, i,
                             record_full_core_regbuf + MAX_REGISTER_SIZE * i);
    }
  else
    regcache_raw_supply (regcache, regno,
                         record_full_core_regbuf + MAX_REGISTER_SIZE * regno);
}

/* "to_prepare_to_store" method for prec over corefile.  */

static void
record_full_core_prepare_to_store (struct target_ops *self,
				   struct regcache *regcache)
{
}

/* "to_store_registers" method for prec over corefile.  */

static void
record_full_core_store_registers (struct target_ops *ops,
                             struct regcache *regcache,
                             int regno)
{
  if (record_full_gdb_operation_disable)
    regcache_raw_collect (regcache, regno,
                          record_full_core_regbuf + MAX_REGISTER_SIZE * regno);
  else
    error (_("You can't do that without a process to debug."));
}

/* "to_xfer_partial" method for prec over corefile.  */

static enum target_xfer_status
record_full_core_xfer_partial (struct target_ops *ops,
			       enum target_object object,
			       const char *annex, gdb_byte *readbuf,
			       const gdb_byte *writebuf, ULONGEST offset,
			       ULONGEST len, ULONGEST *xfered_len)
{
  if (object == TARGET_OBJECT_MEMORY)
    {
      if (record_full_gdb_operation_disable || !writebuf)
	{
	  struct target_section *p;

	  for (p = record_full_core_start; p < record_full_core_end; p++)
	    {
	      if (offset >= p->addr)
		{
		  struct record_full_core_buf_entry *entry;
		  ULONGEST sec_offset;

		  if (offset >= p->endaddr)
		    continue;

		  if (offset + len > p->endaddr)
		    len = p->endaddr - offset;

		  sec_offset = offset - p->addr;

		  /* Read readbuf or write writebuf p, offset, len.  */
		  /* Check flags.  */
		  if (p->the_bfd_section->flags & SEC_CONSTRUCTOR
		      || (p->the_bfd_section->flags & SEC_HAS_CONTENTS) == 0)
		    {
		      if (readbuf)
			memset (readbuf, 0, len);

		      *xfered_len = len;
		      return TARGET_XFER_OK;
		    }
		  /* Get record_full_core_buf_entry.  */
		  for (entry = record_full_core_buf_list; entry;
		       entry = entry->prev)
		    if (entry->p == p)
		      break;
		  if (writebuf)
		    {
		      if (!entry)
			{
			  /* Add a new entry.  */
			  entry = XNEW (struct record_full_core_buf_entry);
			  entry->p = p;
			  if (!bfd_malloc_and_get_section
			        (p->the_bfd_section->owner,
				 p->the_bfd_section,
				 &entry->buf))
			    {
			      xfree (entry);
			      return TARGET_XFER_EOF;
			    }
			  entry->prev = record_full_core_buf_list;
			  record_full_core_buf_list = entry;
			}

		      memcpy (entry->buf + sec_offset, writebuf,
			      (size_t) len);
		    }
		  else
		    {
		      if (!entry)
			return ops->beneath->to_xfer_partial (ops->beneath,
							      object, annex,
							      readbuf, writebuf,
							      offset, len,
							      xfered_len);

		      memcpy (readbuf, entry->buf + sec_offset,
			      (size_t) len);
		    }

		  *xfered_len = len;
		  return TARGET_XFER_OK;
		}
	    }

	  return TARGET_XFER_E_IO;
	}
      else
	error (_("You can't do that without a process to debug."));
    }

  return ops->beneath->to_xfer_partial (ops->beneath, object, annex,
					readbuf, writebuf, offset, len,
					xfered_len);
}

/* "to_insert_breakpoint" method for prec over corefile.  */

static int
record_full_core_insert_breakpoint (struct target_ops *ops,
				    struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt)
{
  return 0;
}

/* "to_remove_breakpoint" method for prec over corefile.  */

static int
record_full_core_remove_breakpoint (struct target_ops *ops,
				    struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt,
				    enum remove_bp_reason reason)
{
  return 0;
}

/* "to_has_execution" method for prec over corefile.  */

static int
record_full_core_has_execution (struct target_ops *ops, ptid_t the_ptid)
{
  return 1;
}

static void
init_record_full_core_ops (void)
{
  record_full_core_ops.to_shortname = "record-core";
  record_full_core_ops.to_longname = "Process record and replay target";
  record_full_core_ops.to_doc =
    "Log program while executing and replay execution from log.";
  record_full_core_ops.to_open = record_full_open;
  record_full_core_ops.to_close = record_full_close;
  record_full_core_ops.to_async = record_full_async;
  record_full_core_ops.to_resume = record_full_core_resume;
  record_full_core_ops.to_wait = record_full_wait;
  record_full_core_ops.to_kill = record_full_core_kill;
  record_full_core_ops.to_fetch_registers = record_full_core_fetch_registers;
  record_full_core_ops.to_prepare_to_store = record_full_core_prepare_to_store;
  record_full_core_ops.to_store_registers = record_full_core_store_registers;
  record_full_core_ops.to_xfer_partial = record_full_core_xfer_partial;
  record_full_core_ops.to_insert_breakpoint
    = record_full_core_insert_breakpoint;
  record_full_core_ops.to_remove_breakpoint
    = record_full_core_remove_breakpoint;
  record_full_core_ops.to_stopped_by_watchpoint
    = record_full_stopped_by_watchpoint;
  record_full_core_ops.to_stopped_data_address
    = record_full_stopped_data_address;
  record_full_core_ops.to_stopped_by_sw_breakpoint
    = record_full_stopped_by_sw_breakpoint;
  record_full_core_ops.to_supports_stopped_by_sw_breakpoint
    = record_full_supports_stopped_by_sw_breakpoint;
  record_full_core_ops.to_stopped_by_hw_breakpoint
    = record_full_stopped_by_hw_breakpoint;
  record_full_core_ops.to_supports_stopped_by_hw_breakpoint
    = record_full_supports_stopped_by_hw_breakpoint;
  record_full_core_ops.to_can_execute_reverse
    = record_full_can_execute_reverse;
  record_full_core_ops.to_has_execution = record_full_core_has_execution;
  record_full_core_ops.to_stratum = record_stratum;
  /* Add bookmark target methods.  */
  record_full_core_ops.to_get_bookmark = record_full_get_bookmark;
  record_full_core_ops.to_goto_bookmark = record_full_goto_bookmark;
  record_full_core_ops.to_execution_direction
    = record_full_execution_direction;
  record_full_core_ops.to_record_method = record_full_record_method;
  record_full_core_ops.to_info_record = record_full_info;
  record_full_core_ops.to_delete_record = record_full_delete;
  record_full_core_ops.to_record_is_replaying = record_full_is_replaying;
  record_full_core_ops.to_record_will_replay = record_full_will_replay;
  record_full_core_ops.to_goto_record_begin = record_full_goto_begin;
  record_full_core_ops.to_goto_record_end = record_full_goto_end;
  record_full_core_ops.to_goto_record = record_full_goto;
  record_full_core_ops.to_magic = OPS_MAGIC;
}

/* Record log save-file format
   Version 1 (never released)

   Header:
     4 bytes: magic number htonl(0x20090829).
       NOTE: be sure to change whenever this file format changes!

   Records:
     record_full_end:
       1 byte:  record type (record_full_end, see enum record_full_type).
     record_full_reg:
       1 byte:  record type (record_full_reg, see enum record_full_type).
       8 bytes: register id (network byte order).
       MAX_REGISTER_SIZE bytes: register value.
     record_full_mem:
       1 byte:  record type (record_full_mem, see enum record_full_type).
       8 bytes: memory length (network byte order).
       8 bytes: memory address (network byte order).
       n bytes: memory value (n == memory length).

   Version 2
     4 bytes: magic number netorder32(0x20091016).
       NOTE: be sure to change whenever this file format changes!

   Records:
     record_full_end:
       1 byte:  record type (record_full_end, see enum record_full_type).
       4 bytes: signal
       4 bytes: instruction count
     record_full_reg:
       1 byte:  record type (record_full_reg, see enum record_full_type).
       4 bytes: register id (network byte order).
       n bytes: register value (n == actual register size).
                (eg. 4 bytes for x86 general registers).
     record_full_mem:
       1 byte:  record type (record_full_mem, see enum record_full_type).
       4 bytes: memory length (network byte order).
       8 bytes: memory address (network byte order).
       n bytes: memory value (n == memory length).

*/

/* bfdcore_read -- read bytes from a core file section.  */

static inline void
bfdcore_read (bfd *obfd, asection *osec, void *buf, int len, int *offset)
{
  int ret = bfd_get_section_contents (obfd, osec, buf, *offset, len);

  if (ret)
    *offset += len;
  else
    error (_("Failed to read %d bytes from core file %s ('%s')."),
	   len, bfd_get_filename (obfd),
	   bfd_errmsg (bfd_get_error ()));
}

static inline uint64_t
netorder64 (uint64_t input)
{
  uint64_t ret;

  store_unsigned_integer ((gdb_byte *) &ret, sizeof (ret), 
			  BFD_ENDIAN_BIG, input);
  return ret;
}

static inline uint32_t
netorder32 (uint32_t input)
{
  uint32_t ret;

  store_unsigned_integer ((gdb_byte *) &ret, sizeof (ret), 
			  BFD_ENDIAN_BIG, input);
  return ret;
}

static inline uint16_t
netorder16 (uint16_t input)
{
  uint16_t ret;

  store_unsigned_integer ((gdb_byte *) &ret, sizeof (ret), 
			  BFD_ENDIAN_BIG, input);
  return ret;
}

/* Restore the execution log from a core_bfd file.  */
static void
record_full_restore (void)
{
  uint32_t magic;
  struct cleanup *old_cleanups;
  struct record_full_entry *rec;
  asection *osec;
  uint32_t osec_size;
  int bfd_offset = 0;
  struct regcache *regcache;

  /* We restore the execution log from the open core bfd,
     if there is one.  */
  if (core_bfd == NULL)
    return;

  /* "record_full_restore" can only be called when record list is empty.  */
  gdb_assert (record_full_first.next == NULL);
 
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Restoring recording from core file.\n");

  /* Now need to find our special note section.  */
  osec = bfd_get_section_by_name (core_bfd, "null0");
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Find precord section %s.\n",
			osec ? "succeeded" : "failed");
  if (osec == NULL)
    return;
  osec_size = bfd_section_size (core_bfd, osec);
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "%s", bfd_section_name (core_bfd, osec));

  /* Check the magic code.  */
  bfdcore_read (core_bfd, osec, &magic, sizeof (magic), &bfd_offset);
  if (magic != RECORD_FULL_FILE_MAGIC)
    error (_("Version mis-match or file format error in core file %s."),
	   bfd_get_filename (core_bfd));
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog,
			"  Reading 4-byte magic cookie "
			"RECORD_FULL_FILE_MAGIC (0x%s)\n",
			phex_nz (netorder32 (magic), 4));

  /* Restore the entries in recfd into record_full_arch_list_head and
     record_full_arch_list_tail.  */
  record_full_arch_list_head = NULL;
  record_full_arch_list_tail = NULL;
  record_full_insn_num = 0;
  old_cleanups = make_cleanup (record_full_arch_list_cleanups, 0);
  regcache = get_current_regcache ();

  while (1)
    {
      uint8_t rectype;
      uint32_t regnum, len, signal, count;
      uint64_t addr;

      /* We are finished when offset reaches osec_size.  */
      if (bfd_offset >= osec_size)
	break;
      bfdcore_read (core_bfd, osec, &rectype, sizeof (rectype), &bfd_offset);

      switch (rectype)
        {
        case record_full_reg: /* reg */
          /* Get register number to regnum.  */
          bfdcore_read (core_bfd, osec, &regnum,
			sizeof (regnum), &bfd_offset);
	  regnum = netorder32 (regnum);

          rec = record_full_reg_alloc (regcache, regnum);

          /* Get val.  */
          bfdcore_read (core_bfd, osec, record_full_get_loc (rec),
			rec->u.reg.len, &bfd_offset);

	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"  Reading register %d (1 "
				"plus %lu plus %d bytes)\n",
				rec->u.reg.num,
				(unsigned long) sizeof (regnum),
				rec->u.reg.len);
          break;

        case record_full_mem: /* mem */
          /* Get len.  */
          bfdcore_read (core_bfd, osec, &len, 
			sizeof (len), &bfd_offset);
	  len = netorder32 (len);

          /* Get addr.  */
          bfdcore_read (core_bfd, osec, &addr,
			sizeof (addr), &bfd_offset);
	  addr = netorder64 (addr);

          rec = record_full_mem_alloc (addr, len);

          /* Get val.  */
          bfdcore_read (core_bfd, osec, record_full_get_loc (rec),
			rec->u.mem.len, &bfd_offset);

	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"  Reading memory %s (1 plus "
				"%lu plus %lu plus %d bytes)\n",
				paddress (get_current_arch (),
					  rec->u.mem.addr),
				(unsigned long) sizeof (addr),
				(unsigned long) sizeof (len),
				rec->u.mem.len);
          break;

        case record_full_end: /* end */
          rec = record_full_end_alloc ();
          record_full_insn_num ++;

	  /* Get signal value.  */
	  bfdcore_read (core_bfd, osec, &signal, 
			sizeof (signal), &bfd_offset);
	  signal = netorder32 (signal);
	  rec->u.end.sigval = (enum gdb_signal) signal;

	  /* Get insn count.  */
	  bfdcore_read (core_bfd, osec, &count, 
			sizeof (count), &bfd_offset);
	  count = netorder32 (count);
	  rec->u.end.insn_num = count;
	  record_full_insn_count = count + 1;
	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"  Reading record_full_end (1 + "
				"%lu + %lu bytes), offset == %s\n",
				(unsigned long) sizeof (signal),
				(unsigned long) sizeof (count),
				paddress (get_current_arch (),
					  bfd_offset));
          break;

        default:
          error (_("Bad entry type in core file %s."),
		 bfd_get_filename (core_bfd));
          break;
        }

      /* Add rec to record arch list.  */
      record_full_arch_list_add (rec);
    }

  discard_cleanups (old_cleanups);

  /* Add record_full_arch_list_head to the end of record list.  */
  record_full_first.next = record_full_arch_list_head;
  record_full_arch_list_head->prev = &record_full_first;
  record_full_arch_list_tail->next = NULL;
  record_full_list = &record_full_first;

  /* Update record_full_insn_max_num.  */
  if (record_full_insn_num > record_full_insn_max_num)
    {
      record_full_insn_max_num = record_full_insn_num;
      warning (_("Auto increase record/replay buffer limit to %u."),
               record_full_insn_max_num);
    }

  /* Succeeded.  */
  printf_filtered (_("Restored records from core file %s.\n"),
		   bfd_get_filename (core_bfd));

  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* bfdcore_write -- write bytes into a core file section.  */

static inline void
bfdcore_write (bfd *obfd, asection *osec, void *buf, int len, int *offset)
{
  int ret = bfd_set_section_contents (obfd, osec, buf, *offset, len);

  if (ret)
    *offset += len;
  else
    error (_("Failed to write %d bytes to core file %s ('%s')."),
	   len, bfd_get_filename (obfd),
	   bfd_errmsg (bfd_get_error ()));
}

/* Restore the execution log from a file.  We use a modified elf
   corefile format, with an extra section for our data.  */

static void
cmd_record_full_restore (char *args, int from_tty)
{
  core_file_command (args, from_tty);
  record_full_open (args, from_tty);
}

/* Save the execution log to a file.  We use a modified elf corefile
   format, with an extra section for our data.  */

static void
record_full_save (struct target_ops *self, const char *recfilename)
{
  struct record_full_entry *cur_record_full_list;
  uint32_t magic;
  struct regcache *regcache;
  struct gdbarch *gdbarch;
  struct cleanup *set_cleanups;
  int save_size = 0;
  asection *osec = NULL;
  int bfd_offset = 0;

  /* Open the save file.  */
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Saving execution log to core file '%s'\n",
			recfilename);

  /* Open the output file.  */
  gdb_bfd_ref_ptr obfd (create_gcore_bfd (recfilename));

  /* Arrange to remove the output file on failure.  */
  gdb::unlinker unlink_file (recfilename);

  /* Save the current record entry to "cur_record_full_list".  */
  cur_record_full_list = record_full_list;

  /* Get the values of regcache and gdbarch.  */
  regcache = get_current_regcache ();
  gdbarch = get_regcache_arch (regcache);

  /* Disable the GDB operation record.  */
  set_cleanups = record_full_gdb_operation_disable_set ();

  /* Reverse execute to the begin of record list.  */
  while (1)
    {
      /* Check for beginning and end of log.  */
      if (record_full_list == &record_full_first)
        break;

      record_full_exec_insn (regcache, gdbarch, record_full_list);

      if (record_full_list->prev)
        record_full_list = record_full_list->prev;
    }

  /* Compute the size needed for the extra bfd section.  */
  save_size = 4;	/* magic cookie */
  for (record_full_list = record_full_first.next; record_full_list;
       record_full_list = record_full_list->next)
    switch (record_full_list->type)
      {
      case record_full_end:
	save_size += 1 + 4 + 4;
	break;
      case record_full_reg:
	save_size += 1 + 4 + record_full_list->u.reg.len;
	break;
      case record_full_mem:
	save_size += 1 + 4 + 8 + record_full_list->u.mem.len;
	break;
      }

  /* Make the new bfd section.  */
  osec = bfd_make_section_anyway_with_flags (obfd.get (), "precord",
                                             SEC_HAS_CONTENTS
                                             | SEC_READONLY);
  if (osec == NULL)
    error (_("Failed to create 'precord' section for corefile %s: %s"),
	   recfilename,
           bfd_errmsg (bfd_get_error ()));
  bfd_set_section_size (obfd.get (), osec, save_size);
  bfd_set_section_vma (obfd.get (), osec, 0);
  bfd_set_section_alignment (obfd.get (), osec, 0);
  bfd_section_lma (obfd.get (), osec) = 0;

  /* Save corefile state.  */
  write_gcore_file (obfd.get ());

  /* Write out the record log.  */
  /* Write the magic code.  */
  magic = RECORD_FULL_FILE_MAGIC;
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog,
			"  Writing 4-byte magic cookie "
			"RECORD_FULL_FILE_MAGIC (0x%s)\n",
		      phex_nz (magic, 4));
  bfdcore_write (obfd.get (), osec, &magic, sizeof (magic), &bfd_offset);

  /* Save the entries to recfd and forward execute to the end of
     record list.  */
  record_full_list = &record_full_first;
  while (1)
    {
      /* Save entry.  */
      if (record_full_list != &record_full_first)
        {
	  uint8_t type;
	  uint32_t regnum, len, signal, count;
          uint64_t addr;

	  type = record_full_list->type;
          bfdcore_write (obfd.get (), osec, &type, sizeof (type), &bfd_offset);

          switch (record_full_list->type)
            {
            case record_full_reg: /* reg */
	      if (record_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "  Writing register %d (1 "
				    "plus %lu plus %d bytes)\n",
				    record_full_list->u.reg.num,
				    (unsigned long) sizeof (regnum),
				    record_full_list->u.reg.len);

              /* Write regnum.  */
              regnum = netorder32 (record_full_list->u.reg.num);
              bfdcore_write (obfd.get (), osec, &regnum,
			     sizeof (regnum), &bfd_offset);

              /* Write regval.  */
              bfdcore_write (obfd.get (), osec,
			     record_full_get_loc (record_full_list),
			     record_full_list->u.reg.len, &bfd_offset);
              break;

            case record_full_mem: /* mem */
	      if (record_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "  Writing memory %s (1 plus "
				    "%lu plus %lu plus %d bytes)\n",
				    paddress (gdbarch,
					      record_full_list->u.mem.addr),
				    (unsigned long) sizeof (addr),
				    (unsigned long) sizeof (len),
				    record_full_list->u.mem.len);

	      /* Write memlen.  */
	      len = netorder32 (record_full_list->u.mem.len);
	      bfdcore_write (obfd.get (), osec, &len, sizeof (len),
			     &bfd_offset);

	      /* Write memaddr.  */
	      addr = netorder64 (record_full_list->u.mem.addr);
	      bfdcore_write (obfd.get (), osec, &addr, 
			     sizeof (addr), &bfd_offset);

	      /* Write memval.  */
	      bfdcore_write (obfd.get (), osec,
			     record_full_get_loc (record_full_list),
			     record_full_list->u.mem.len, &bfd_offset);
              break;

              case record_full_end:
		if (record_debug)
		  fprintf_unfiltered (gdb_stdlog,
				      "  Writing record_full_end (1 + "
				      "%lu + %lu bytes)\n", 
				      (unsigned long) sizeof (signal),
				      (unsigned long) sizeof (count));
		/* Write signal value.  */
		signal = netorder32 (record_full_list->u.end.sigval);
		bfdcore_write (obfd.get (), osec, &signal,
			       sizeof (signal), &bfd_offset);

		/* Write insn count.  */
		count = netorder32 (record_full_list->u.end.insn_num);
		bfdcore_write (obfd.get (), osec, &count,
			       sizeof (count), &bfd_offset);
                break;
            }
        }

      /* Execute entry.  */
      record_full_exec_insn (regcache, gdbarch, record_full_list);

      if (record_full_list->next)
        record_full_list = record_full_list->next;
      else
        break;
    }

  /* Reverse execute to cur_record_full_list.  */
  while (1)
    {
      /* Check for beginning and end of log.  */
      if (record_full_list == cur_record_full_list)
        break;

      record_full_exec_insn (regcache, gdbarch, record_full_list);

      if (record_full_list->prev)
        record_full_list = record_full_list->prev;
    }

  do_cleanups (set_cleanups);
  unlink_file.keep ();

  /* Succeeded.  */
  printf_filtered (_("Saved core file %s with execution log.\n"),
		   recfilename);
}

/* record_full_goto_insn -- rewind the record log (forward or backward,
   depending on DIR) to the given entry, changing the program state
   correspondingly.  */

static void
record_full_goto_insn (struct record_full_entry *entry,
		       enum exec_direction_kind dir)
{
  struct cleanup *set_cleanups = record_full_gdb_operation_disable_set ();
  struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  /* Assume everything is valid: we will hit the entry,
     and we will not hit the end of the recording.  */

  if (dir == EXEC_FORWARD)
    record_full_list = record_full_list->next;

  do
    {
      record_full_exec_insn (regcache, gdbarch, record_full_list);
      if (dir == EXEC_REVERSE)
	record_full_list = record_full_list->prev;
      else
	record_full_list = record_full_list->next;
    } while (record_full_list != entry);
  do_cleanups (set_cleanups);
}

/* Alias for "target record-full".  */

static void
cmd_record_full_start (char *args, int from_tty)
{
  execute_command ((char *) "target record-full", from_tty);
}

static void
set_record_full_insn_max_num (char *args, int from_tty,
			      struct cmd_list_element *c)
{
  if (record_full_insn_num > record_full_insn_max_num)
    {
      /* Count down record_full_insn_num while releasing records from list.  */
      while (record_full_insn_num > record_full_insn_max_num)
       {
         record_full_list_release_first ();
         record_full_insn_num--;
       }
    }
}

/* The "set record full" command.  */

static void
set_record_full_command (char *args, int from_tty)
{
  printf_unfiltered (_("\"set record full\" must be followed "
		       "by an apporpriate subcommand.\n"));
  help_list (set_record_full_cmdlist, "set record full ", all_commands,
	     gdb_stdout);
}

/* The "show record full" command.  */

static void
show_record_full_command (char *args, int from_tty)
{
  cmd_show_list (show_record_full_cmdlist, from_tty, "");
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_record_full;

void
_initialize_record_full (void)
{
  struct cmd_list_element *c;

  /* Init record_full_first.  */
  record_full_first.prev = NULL;
  record_full_first.next = NULL;
  record_full_first.type = record_full_end;

  init_record_full_ops ();
  add_target (&record_full_ops);
  add_deprecated_target_alias (&record_full_ops, "record");
  init_record_full_core_ops ();
  add_target (&record_full_core_ops);

  add_prefix_cmd ("full", class_obscure, cmd_record_full_start,
		  _("Start full execution recording."), &record_full_cmdlist,
		  "record full ", 0, &record_cmdlist);

  c = add_cmd ("restore", class_obscure, cmd_record_full_restore,
	       _("Restore the execution log from a file.\n\
Argument is filename.  File must be created with 'record save'."),
	       &record_full_cmdlist);
  set_cmd_completer (c, filename_completer);

  /* Deprecate the old version without "full" prefix.  */
  c = add_alias_cmd ("restore", "full restore", class_obscure, 1,
		     &record_cmdlist);
  set_cmd_completer (c, filename_completer);
  deprecate_cmd (c, "record full restore");

  add_prefix_cmd ("full", class_support, set_record_full_command,
		  _("Set record options"), &set_record_full_cmdlist,
		  "set record full ", 0, &set_record_cmdlist);

  add_prefix_cmd ("full", class_support, show_record_full_command,
		  _("Show record options"), &show_record_full_cmdlist,
		  "show record full ", 0, &show_record_cmdlist);

  /* Record instructions number limit command.  */
  add_setshow_boolean_cmd ("stop-at-limit", no_class,
			   &record_full_stop_at_limit, _("\
Set whether record/replay stops when record/replay buffer becomes full."), _("\
Show whether record/replay stops when record/replay buffer becomes full."),
			   _("Default is ON.\n\
When ON, if the record/replay buffer becomes full, ask user what to do.\n\
When OFF, if the record/replay buffer becomes full,\n\
delete the oldest recorded instruction to make room for each new one."),
			   NULL, NULL,
			   &set_record_full_cmdlist, &show_record_full_cmdlist);

  c = add_alias_cmd ("stop-at-limit", "full stop-at-limit", no_class, 1,
		     &set_record_cmdlist);
  deprecate_cmd (c, "set record full stop-at-limit");

  c = add_alias_cmd ("stop-at-limit", "full stop-at-limit", no_class, 1,
		     &show_record_cmdlist);
  deprecate_cmd (c, "show record full stop-at-limit");

  add_setshow_uinteger_cmd ("insn-number-max", no_class,
			    &record_full_insn_max_num,
			    _("Set record/replay buffer limit."),
			    _("Show record/replay buffer limit."), _("\
Set the maximum number of instructions to be stored in the\n\
record/replay buffer.  A value of either \"unlimited\" or zero means no\n\
limit.  Default is 200000."),
			    set_record_full_insn_max_num,
			    NULL, &set_record_full_cmdlist,
			    &show_record_full_cmdlist);

  c = add_alias_cmd ("insn-number-max", "full insn-number-max", no_class, 1,
		     &set_record_cmdlist);
  deprecate_cmd (c, "set record full insn-number-max");

  c = add_alias_cmd ("insn-number-max", "full insn-number-max", no_class, 1,
		     &show_record_cmdlist);
  deprecate_cmd (c, "show record full insn-number-max");

  add_setshow_boolean_cmd ("memory-query", no_class,
			   &record_full_memory_query, _("\
Set whether query if PREC cannot record memory change of next instruction."),
                           _("\
Show whether query if PREC cannot record memory change of next instruction."),
                           _("\
Default is OFF.\n\
When ON, query if PREC cannot record memory change of next instruction."),
			   NULL, NULL,
			   &set_record_full_cmdlist,
			   &show_record_full_cmdlist);

  c = add_alias_cmd ("memory-query", "full memory-query", no_class, 1,
		     &set_record_cmdlist);
  deprecate_cmd (c, "set record full memory-query");

  c = add_alias_cmd ("memory-query", "full memory-query", no_class, 1,
		     &show_record_cmdlist);
  deprecate_cmd (c, "show record full memory-query");
}
