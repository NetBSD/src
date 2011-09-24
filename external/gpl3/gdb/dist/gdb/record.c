/* Process record and replay target for GDB, the GNU debugger.

   Copyright (C) 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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
#include "exceptions.h"
#include "completer.h"
#include "arch-utils.h"
#include "gdbcore.h"
#include "exec.h"
#include "record.h"
#include "elf-bfd.h"
#include "gcore.h"

#include <signal.h>

/* This module implements "target record", also known as "process
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

#define DEFAULT_RECORD_INSN_MAX_NUM	200000

#define RECORD_IS_REPLAY \
     (record_list->next || execution_direction == EXEC_REVERSE)

#define RECORD_FILE_MAGIC	netorder32(0x20091016)

/* These are the core structs of the process record functionality.

   A record_entry is a record of the value change of a register
   ("record_reg") or a part of memory ("record_mem").  And each
   instruction must have a struct record_entry ("record_end") that
   indicates that this is the last struct record_entry of this
   instruction.

   Each struct record_entry is linked to "record_list" by "prev" and
   "next" pointers.  */

struct record_mem_entry
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

struct record_reg_entry
{
  unsigned short num;
  unsigned short len;
  union 
  {
    gdb_byte *ptr;
    gdb_byte buf[2 * sizeof (gdb_byte *)];
  } u;
};

struct record_end_entry
{
  enum target_signal sigval;
  ULONGEST insn_num;
};

enum record_type
{
  record_end = 0,
  record_reg,
  record_mem
};

/* This is the data structure that makes up the execution log.

   The execution log consists of a single linked list of entries
   of type "struct record_entry".  It is doubly linked so that it
   can be traversed in either direction.

   The start of the list is anchored by a struct called
   "record_first".  The pointer "record_list" either points to the
   last entry that was added to the list (in record mode), or to the
   next entry in the list that will be executed (in replay mode).

   Each list element (struct record_entry), in addition to next and
   prev pointers, consists of a union of three entry types: mem, reg,
   and end.  A field called "type" determines which entry type is
   represented by a given list element.

   Each instruction that is added to the execution log is represented
   by a variable number of list elements ('entries').  The instruction
   will have one "reg" entry for each register that is changed by 
   executing the instruction (including the PC in every case).  It 
   will also have one "mem" entry for each memory change.  Finally,
   each instruction will have an "end" entry that separates it from
   the changes associated with the next instruction.  */

struct record_entry
{
  struct record_entry *prev;
  struct record_entry *next;
  enum record_type type;
  union
  {
    /* reg */
    struct record_reg_entry reg;
    /* mem */
    struct record_mem_entry mem;
    /* end */
    struct record_end_entry end;
  } u;
};

/* This is the debug switch for process record.  */
int record_debug = 0;

/* If true, query if PREC cannot record memory
   change of next instruction.  */
int record_memory_query = 0;

struct record_core_buf_entry
{
  struct record_core_buf_entry *prev;
  struct target_section *p;
  bfd_byte *buf;
};

/* Record buf with core target.  */
static gdb_byte *record_core_regbuf = NULL;
static struct target_section *record_core_start;
static struct target_section *record_core_end;
static struct record_core_buf_entry *record_core_buf_list = NULL;

/* The following variables are used for managing the linked list that
   represents the execution log.

   record_first is the anchor that holds down the beginning of the list.

   record_list serves two functions:
     1) In record mode, it anchors the end of the list.
     2) In replay mode, it traverses the list and points to
        the next instruction that must be emulated.

   record_arch_list_head and record_arch_list_tail are used to manage
   a separate list, which is used to build up the change elements of
   the currently executing instruction during record mode.  When this
   instruction has been completely annotated in the "arch list", it 
   will be appended to the main execution log.  */

static struct record_entry record_first;
static struct record_entry *record_list = &record_first;
static struct record_entry *record_arch_list_head = NULL;
static struct record_entry *record_arch_list_tail = NULL;

/* 1 ask user. 0 auto delete the last struct record_entry.  */
static int record_stop_at_limit = 1;
/* Maximum allowed number of insns in execution log.  */
static unsigned int record_insn_max_num = DEFAULT_RECORD_INSN_MAX_NUM;
/* Actual count of insns presently in execution log.  */
static int record_insn_num = 0;
/* Count of insns logged so far (may be larger
   than count of insns presently in execution log).  */
static ULONGEST record_insn_count;

/* The target_ops of process record.  */
static struct target_ops record_ops;
static struct target_ops record_core_ops;

/* The beneath function pointers.  */
static struct target_ops *record_beneath_to_resume_ops;
static void (*record_beneath_to_resume) (struct target_ops *, ptid_t, int,
                                         enum target_signal);
static struct target_ops *record_beneath_to_wait_ops;
static ptid_t (*record_beneath_to_wait) (struct target_ops *, ptid_t,
					 struct target_waitstatus *,
					 int);
static struct target_ops *record_beneath_to_store_registers_ops;
static void (*record_beneath_to_store_registers) (struct target_ops *,
                                                  struct regcache *,
						  int regno);
static struct target_ops *record_beneath_to_xfer_partial_ops;
static LONGEST (*record_beneath_to_xfer_partial) (struct target_ops *ops,
						  enum target_object object,
						  const char *annex,
						  gdb_byte *readbuf,
						  const gdb_byte *writebuf,
						  ULONGEST offset,
						  LONGEST len);
static int (*record_beneath_to_insert_breakpoint) (struct gdbarch *,
						   struct bp_target_info *);
static int (*record_beneath_to_remove_breakpoint) (struct gdbarch *,
						   struct bp_target_info *);
static int (*record_beneath_to_stopped_by_watchpoint) (void);
static int (*record_beneath_to_stopped_data_address) (struct target_ops *,
						      CORE_ADDR *);

/* Alloc and free functions for record_reg, record_mem, and record_end 
   entries.  */

/* Alloc a record_reg record entry.  */

static inline struct record_entry *
record_reg_alloc (struct regcache *regcache, int regnum)
{
  struct record_entry *rec;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  rec = (struct record_entry *) xcalloc (1, sizeof (struct record_entry));
  rec->type = record_reg;
  rec->u.reg.num = regnum;
  rec->u.reg.len = register_size (gdbarch, regnum);
  if (rec->u.reg.len > sizeof (rec->u.reg.u.buf))
    rec->u.reg.u.ptr = (gdb_byte *) xmalloc (rec->u.reg.len);

  return rec;
}

/* Free a record_reg record entry.  */

static inline void
record_reg_release (struct record_entry *rec)
{
  gdb_assert (rec->type == record_reg);
  if (rec->u.reg.len > sizeof (rec->u.reg.u.buf))
    xfree (rec->u.reg.u.ptr);
  xfree (rec);
}

/* Alloc a record_mem record entry.  */

static inline struct record_entry *
record_mem_alloc (CORE_ADDR addr, int len)
{
  struct record_entry *rec;

  rec = (struct record_entry *) xcalloc (1, sizeof (struct record_entry));
  rec->type = record_mem;
  rec->u.mem.addr = addr;
  rec->u.mem.len = len;
  if (rec->u.mem.len > sizeof (rec->u.mem.u.buf))
    rec->u.mem.u.ptr = (gdb_byte *) xmalloc (len);

  return rec;
}

/* Free a record_mem record entry.  */

static inline void
record_mem_release (struct record_entry *rec)
{
  gdb_assert (rec->type == record_mem);
  if (rec->u.mem.len > sizeof (rec->u.mem.u.buf))
    xfree (rec->u.mem.u.ptr);
  xfree (rec);
}

/* Alloc a record_end record entry.  */

static inline struct record_entry *
record_end_alloc (void)
{
  struct record_entry *rec;

  rec = (struct record_entry *) xcalloc (1, sizeof (struct record_entry));
  rec->type = record_end;

  return rec;
}

/* Free a record_end record entry.  */

static inline void
record_end_release (struct record_entry *rec)
{
  xfree (rec);
}

/* Free one record entry, any type.
   Return entry->type, in case caller wants to know.  */

static inline enum record_type
record_entry_release (struct record_entry *rec)
{
  enum record_type type = rec->type;

  switch (type) {
  case record_reg:
    record_reg_release (rec);
    break;
  case record_mem:
    record_mem_release (rec);
    break;
  case record_end:
    record_end_release (rec);
    break;
  }
  return type;
}

/* Free all record entries in list pointed to by REC.  */

static void
record_list_release (struct record_entry *rec)
{
  if (!rec)
    return;

  while (rec->next)
    rec = rec->next;

  while (rec->prev)
    {
      rec = rec->prev;
      record_entry_release (rec->next);
    }

  if (rec == &record_first)
    {
      record_insn_num = 0;
      record_first.next = NULL;
    }
  else
    record_entry_release (rec);
}

/* Free all record entries forward of the given list position.  */

static void
record_list_release_following (struct record_entry *rec)
{
  struct record_entry *tmp = rec->next;

  rec->next = NULL;
  while (tmp)
    {
      rec = tmp->next;
      if (record_entry_release (tmp) == record_end)
	{
	  record_insn_num--;
	  record_insn_count--;
	}
      tmp = rec;
    }
}

/* Delete the first instruction from the beginning of the log, to make
   room for adding a new instruction at the end of the log.

   Note -- this function does not modify record_insn_num.  */

static void
record_list_release_first (void)
{
  struct record_entry *tmp;

  if (!record_first.next)
    return;

  /* Loop until a record_end.  */
  while (1)
    {
      /* Cut record_first.next out of the linked list.  */
      tmp = record_first.next;
      record_first.next = tmp->next;
      tmp->next->prev = &record_first;

      /* tmp is now isolated, and can be deleted.  */
      if (record_entry_release (tmp) == record_end)
	break;	/* End loop at first record_end.  */

      if (!record_first.next)
	{
	  gdb_assert (record_insn_num == 1);
	  break;	/* End loop when list is empty.  */
	}
    }
}

/* Add a struct record_entry to record_arch_list.  */

static void
record_arch_list_add (struct record_entry *rec)
{
  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: record_arch_list_add %s.\n",
			host_address_to_string (rec));

  if (record_arch_list_tail)
    {
      record_arch_list_tail->next = rec;
      rec->prev = record_arch_list_tail;
      record_arch_list_tail = rec;
    }
  else
    {
      record_arch_list_head = rec;
      record_arch_list_tail = rec;
    }
}

/* Return the value storage location of a record entry.  */
static inline gdb_byte *
record_get_loc (struct record_entry *rec)
{
  switch (rec->type) {
  case record_mem:
    if (rec->u.mem.len > sizeof (rec->u.mem.u.buf))
      return rec->u.mem.u.ptr;
    else
      return rec->u.mem.u.buf;
  case record_reg:
    if (rec->u.reg.len > sizeof (rec->u.reg.u.buf))
      return rec->u.reg.u.ptr;
    else
      return rec->u.reg.u.buf;
  case record_end:
  default:
    gdb_assert_not_reached ("unexpected record_entry type");
    return NULL;
  }
}

/* Record the value of a register NUM to record_arch_list.  */

int
record_arch_list_add_reg (struct regcache *regcache, int regnum)
{
  struct record_entry *rec;

  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: add register num = %d to "
			"record list.\n",
			regnum);

  rec = record_reg_alloc (regcache, regnum);

  regcache_raw_read (regcache, regnum, record_get_loc (rec));

  record_arch_list_add (rec);

  return 0;
}

/* Record the value of a region of memory whose address is ADDR and
   length is LEN to record_arch_list.  */

int
record_arch_list_add_mem (CORE_ADDR addr, int len)
{
  struct record_entry *rec;

  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: add mem addr = %s len = %d to "
			"record list.\n",
			paddress (target_gdbarch, addr), len);

  if (!addr)	/* FIXME: Why?  Some arch must permit it...  */
    return 0;

  rec = record_mem_alloc (addr, len);

  if (target_read_memory (addr, record_get_loc (rec), len))
    {
      if (record_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "Process record: error reading memory at "
			    "addr = %s len = %d.\n",
			    paddress (target_gdbarch, addr), len);
      record_mem_release (rec);
      return -1;
    }

  record_arch_list_add (rec);

  return 0;
}

/* Add a record_end type struct record_entry to record_arch_list.  */

int
record_arch_list_add_end (void)
{
  struct record_entry *rec;

  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: add end to arch list.\n");

  rec = record_end_alloc ();
  rec->u.end.sigval = TARGET_SIGNAL_0;
  rec->u.end.insn_num = ++record_insn_count;

  record_arch_list_add (rec);

  return 0;
}

static void
record_check_insn_num (int set_terminal)
{
  if (record_insn_max_num)
    {
      gdb_assert (record_insn_num <= record_insn_max_num);
      if (record_insn_num == record_insn_max_num)
	{
	  /* Ask user what to do.  */
	  if (record_stop_at_limit)
	    {
	      int q;

	      if (set_terminal)
		target_terminal_ours ();
	      q = yquery (_("Do you want to auto delete previous execution "
			    "log entries when record/replay buffer becomes "
			    "full (record stop-at-limit)?"));
	      if (set_terminal)
		target_terminal_inferior ();
	      if (q)
		record_stop_at_limit = 0;
	      else
		error (_("Process record: stopped by user."));
	    }
	}
    }
}

static void
record_arch_list_cleanups (void *ignore)
{
  record_list_release (record_arch_list_tail);
}

/* Before inferior step (when GDB record the running message, inferior
   only can step), GDB will call this function to record the values to
   record_list.  This function will call gdbarch_process_record to
   record the running message of inferior and set them to
   record_arch_list, and add it to record_list.  */

static int
record_message (struct regcache *regcache, enum target_signal signal)
{
  int ret;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct cleanup *old_cleanups = make_cleanup (record_arch_list_cleanups, 0);

  record_arch_list_head = NULL;
  record_arch_list_tail = NULL;

  /* Check record_insn_num.  */
  record_check_insn_num (1);

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
     record the signal during record_wait, not record_resume.  */
  if (record_list != &record_first)    /* FIXME better way to check */
    {
      gdb_assert (record_list->type == record_end);
      record_list->u.end.sigval = signal;
    }

  if (signal == TARGET_SIGNAL_0
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

  record_list->next = record_arch_list_head;
  record_arch_list_head->prev = record_list;
  record_list = record_arch_list_tail;

  if (record_insn_num == record_insn_max_num && record_insn_max_num)
    record_list_release_first ();
  else
    record_insn_num++;

  return 1;
}

struct record_message_args {
  struct regcache *regcache;
  enum target_signal signal;
};

static int
record_message_wrapper (void *args)
{
  struct record_message_args *record_args = args;

  return record_message (record_args->regcache, record_args->signal);
}

static int
record_message_wrapper_safe (struct regcache *regcache,
                             enum target_signal signal)
{
  struct record_message_args args;

  args.regcache = regcache;
  args.signal = signal;

  return catch_errors (record_message_wrapper, &args, NULL, RETURN_MASK_ALL);
}

/* Set to 1 if record_store_registers and record_xfer_partial
   doesn't need record.  */

static int record_gdb_operation_disable = 0;

struct cleanup *
record_gdb_operation_disable_set (void)
{
  struct cleanup *old_cleanups = NULL;

  old_cleanups =
    make_cleanup_restore_integer (&record_gdb_operation_disable);
  record_gdb_operation_disable = 1;

  return old_cleanups;
}

/* Flag set to TRUE for target_stopped_by_watchpoint.  */
static int record_hw_watchpoint = 0;

/* Execute one instruction from the record log.  Each instruction in
   the log will be represented by an arbitrary sequence of register
   entries and memory entries, followed by an 'end' entry.  */

static inline void
record_exec_insn (struct regcache *regcache, struct gdbarch *gdbarch,
		  struct record_entry *entry)
{
  switch (entry->type)
    {
    case record_reg: /* reg */
      {
        gdb_byte reg[MAX_REGISTER_SIZE];

        if (record_debug > 1)
          fprintf_unfiltered (gdb_stdlog,
                              "Process record: record_reg %s to "
                              "inferior num = %d.\n",
                              host_address_to_string (entry),
                              entry->u.reg.num);

        regcache_cooked_read (regcache, entry->u.reg.num, reg);
        regcache_cooked_write (regcache, entry->u.reg.num, 
			       record_get_loc (entry));
        memcpy (record_get_loc (entry), reg, entry->u.reg.len);
      }
      break;

    case record_mem: /* mem */
      {
	/* Nothing to do if the entry is flagged not_accessible.  */
        if (!entry->u.mem.mem_entry_not_accessible)
          {
            gdb_byte *mem = alloca (entry->u.mem.len);

            if (record_debug > 1)
              fprintf_unfiltered (gdb_stdlog,
                                  "Process record: record_mem %s to "
                                  "inferior addr = %s len = %d.\n",
                                  host_address_to_string (entry),
                                  paddress (gdbarch, entry->u.mem.addr),
                                  entry->u.mem.len);

            if (target_read_memory (entry->u.mem.addr, mem, entry->u.mem.len))
              {
                entry->u.mem.mem_entry_not_accessible = 1;
                if (record_debug)
                  warning (_("Process record: error reading memory at "
			     "addr = %s len = %d."),
                           paddress (gdbarch, entry->u.mem.addr),
                           entry->u.mem.len);
              }
            else
              {
                if (target_write_memory (entry->u.mem.addr, 
					 record_get_loc (entry),
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
		    memcpy (record_get_loc (entry), mem, entry->u.mem.len);

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
		      record_hw_watchpoint = 1;
		  }
              }
          }
      }
      break;
    }
}

static struct target_ops *tmp_to_resume_ops;
static void (*tmp_to_resume) (struct target_ops *, ptid_t, int,
			      enum target_signal);
static struct target_ops *tmp_to_wait_ops;
static ptid_t (*tmp_to_wait) (struct target_ops *, ptid_t,
			      struct target_waitstatus *,
			      int);
static struct target_ops *tmp_to_store_registers_ops;
static void (*tmp_to_store_registers) (struct target_ops *,
				       struct regcache *,
				       int regno);
static struct target_ops *tmp_to_xfer_partial_ops;
static LONGEST (*tmp_to_xfer_partial) (struct target_ops *ops,
				       enum target_object object,
				       const char *annex,
				       gdb_byte *readbuf,
				       const gdb_byte *writebuf,
				       ULONGEST offset,
				       LONGEST len);
static int (*tmp_to_insert_breakpoint) (struct gdbarch *,
					struct bp_target_info *);
static int (*tmp_to_remove_breakpoint) (struct gdbarch *,
					struct bp_target_info *);
static int (*tmp_to_stopped_by_watchpoint) (void);
static int (*tmp_to_stopped_data_address) (struct target_ops *, CORE_ADDR *);

static void record_restore (void);

/* Open the process record target.  */

static void
record_core_open_1 (char *name, int from_tty)
{
  struct regcache *regcache = get_current_regcache ();
  int regnum = gdbarch_num_regs (get_regcache_arch (regcache));
  int i;

  /* Get record_core_regbuf.  */
  target_fetch_registers (regcache, -1);
  record_core_regbuf = xmalloc (MAX_REGISTER_SIZE * regnum);
  for (i = 0; i < regnum; i ++)
    regcache_raw_collect (regcache, i,
			  record_core_regbuf + MAX_REGISTER_SIZE * i);

  /* Get record_core_start and record_core_end.  */
  if (build_section_table (core_bfd, &record_core_start, &record_core_end))
    {
      xfree (record_core_regbuf);
      record_core_regbuf = NULL;
      error (_("\"%s\": Can't find sections: %s"),
	     bfd_get_filename (core_bfd), bfd_errmsg (bfd_get_error ()));
    }

  push_target (&record_core_ops);
  record_restore ();
}

/* "to_open" target method for 'live' processes.  */

static void
record_open_1 (char *name, int from_tty)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_open\n");

  /* check exec */
  if (!target_has_execution)
    error (_("Process record: the program is not being run."));
  if (non_stop)
    error (_("Process record target can't debug inferior in non-stop mode "
	     "(non-stop)."));
  if (target_async_permitted)
    error (_("Process record target can't debug inferior in asynchronous "
	     "mode (target-async)."));

  if (!gdbarch_process_record_p (target_gdbarch))
    error (_("Process record: the current architecture doesn't support "
	     "record function."));

  if (!tmp_to_resume)
    error (_("Could not find 'to_resume' method on the target stack."));
  if (!tmp_to_wait)
    error (_("Could not find 'to_wait' method on the target stack."));
  if (!tmp_to_store_registers)
    error (_("Could not find 'to_store_registers' "
	     "method on the target stack."));
  if (!tmp_to_insert_breakpoint)
    error (_("Could not find 'to_insert_breakpoint' "
	     "method on the target stack."));
  if (!tmp_to_remove_breakpoint)
    error (_("Could not find 'to_remove_breakpoint' "
	     "method on the target stack."));
  if (!tmp_to_stopped_by_watchpoint)
    error (_("Could not find 'to_stopped_by_watchpoint' "
	     "method on the target stack."));
  if (!tmp_to_stopped_data_address)
    error (_("Could not find 'to_stopped_data_address' "
	     "method on the target stack."));

  push_target (&record_ops);
}

/* "to_open" target method.  Open the process record target.  */

static void
record_open (char *name, int from_tty)
{
  struct target_ops *t;

  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_open\n");

  /* Check if record target is already running.  */
  if (current_target.to_stratum == record_stratum)
    error (_("Process record target already running.  Use \"record stop\" to "
             "stop record target first."));

  /* Reset the tmp beneath pointers.  */
  tmp_to_resume_ops = NULL;
  tmp_to_resume = NULL;
  tmp_to_wait_ops = NULL;
  tmp_to_wait = NULL;
  tmp_to_store_registers_ops = NULL;
  tmp_to_store_registers = NULL;
  tmp_to_xfer_partial_ops = NULL;
  tmp_to_xfer_partial = NULL;
  tmp_to_insert_breakpoint = NULL;
  tmp_to_remove_breakpoint = NULL;
  tmp_to_stopped_by_watchpoint = NULL;
  tmp_to_stopped_data_address = NULL;

  /* Set the beneath function pointers.  */
  for (t = current_target.beneath; t != NULL; t = t->beneath)
    {
      if (!tmp_to_resume)
        {
	  tmp_to_resume = t->to_resume;
	  tmp_to_resume_ops = t;
        }
      if (!tmp_to_wait)
        {
	  tmp_to_wait = t->to_wait;
	  tmp_to_wait_ops = t;
        }
      if (!tmp_to_store_registers)
        {
	  tmp_to_store_registers = t->to_store_registers;
	  tmp_to_store_registers_ops = t;
        }
      if (!tmp_to_xfer_partial)
        {
	  tmp_to_xfer_partial = t->to_xfer_partial;
	  tmp_to_xfer_partial_ops = t;
        }
      if (!tmp_to_insert_breakpoint)
	tmp_to_insert_breakpoint = t->to_insert_breakpoint;
      if (!tmp_to_remove_breakpoint)
	tmp_to_remove_breakpoint = t->to_remove_breakpoint;
      if (!tmp_to_stopped_by_watchpoint)
	tmp_to_stopped_by_watchpoint = t->to_stopped_by_watchpoint;
      if (!tmp_to_stopped_data_address)
	tmp_to_stopped_data_address = t->to_stopped_data_address;
    }
  if (!tmp_to_xfer_partial)
    error (_("Could not find 'to_xfer_partial' method on the target stack."));

  /* Reset */
  record_insn_num = 0;
  record_insn_count = 0;
  record_list = &record_first;
  record_list->next = NULL;

  /* Set the tmp beneath pointers to beneath pointers.  */
  record_beneath_to_resume_ops = tmp_to_resume_ops;
  record_beneath_to_resume = tmp_to_resume;
  record_beneath_to_wait_ops = tmp_to_wait_ops;
  record_beneath_to_wait = tmp_to_wait;
  record_beneath_to_store_registers_ops = tmp_to_store_registers_ops;
  record_beneath_to_store_registers = tmp_to_store_registers;
  record_beneath_to_xfer_partial_ops = tmp_to_xfer_partial_ops;
  record_beneath_to_xfer_partial = tmp_to_xfer_partial;
  record_beneath_to_insert_breakpoint = tmp_to_insert_breakpoint;
  record_beneath_to_remove_breakpoint = tmp_to_remove_breakpoint;
  record_beneath_to_stopped_by_watchpoint = tmp_to_stopped_by_watchpoint;
  record_beneath_to_stopped_data_address = tmp_to_stopped_data_address;

  if (core_bfd)
    record_core_open_1 (name, from_tty);
  else
    record_open_1 (name, from_tty);
}

/* "to_close" target method.  Close the process record target.  */

static void
record_close (int quitting)
{
  struct record_core_buf_entry *entry;

  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_close\n");

  record_list_release (record_list);

  /* Release record_core_regbuf.  */
  if (record_core_regbuf)
    {
      xfree (record_core_regbuf);
      record_core_regbuf = NULL;
    }

  /* Release record_core_buf_list.  */
  if (record_core_buf_list)
    {
      for (entry = record_core_buf_list->prev; entry; entry = entry->prev)
	{
	  xfree (record_core_buf_list);
	  record_core_buf_list = entry;
	}
      record_core_buf_list = NULL;
    }
}

static int record_resume_step = 0;

/* "to_resume" target method.  Resume the process record target.  */

static void
record_resume (struct target_ops *ops, ptid_t ptid, int step,
               enum target_signal signal)
{
  record_resume_step = step;

  if (!RECORD_IS_REPLAY)
    {
      struct gdbarch *gdbarch = target_thread_architecture (ptid);

      record_message (get_current_regcache (), signal);

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
              /* This arch support soft sigle step.  */
              if (single_step_breakpoints_inserted ())
                {
                  /* This is a soft single step.  */
                  record_resume_step = 1;
                }
              else
                {
                  /* This is a continue.
                     Try to insert a soft single step breakpoint.  */
                  if (!gdbarch_software_single_step (gdbarch,
                                                     get_current_frame ()))
                    {
                      /* This system don't want use soft single step.
                         Use hard sigle step.  */
                      step = 1;
                    }
                }
            }
        }

      record_beneath_to_resume (record_beneath_to_resume_ops,
                                ptid, step, signal);
    }
}

static int record_get_sig = 0;

/* SIGINT signal handler, registered by "to_wait" method.  */

static void
record_sig_handler (int signo)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: get a signal\n");

  /* It will break the running inferior in replay mode.  */
  record_resume_step = 1;

  /* It will let record_wait set inferior status to get the signal
     SIGINT.  */
  record_get_sig = 1;
}

static void
record_wait_cleanups (void *ignore)
{
  if (execution_direction == EXEC_REVERSE)
    {
      if (record_list->next)
	record_list = record_list->next;
    }
  else
    record_list = record_list->prev;
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
record_wait (struct target_ops *ops,
	     ptid_t ptid, struct target_waitstatus *status,
	     int options)
{
  struct cleanup *set_cleanups = record_gdb_operation_disable_set ();

  if (record_debug)
    fprintf_unfiltered (gdb_stdlog,
			"Process record: record_wait "
			"record_resume_step = %d\n",
			record_resume_step);

  record_get_sig = 0;
  signal (SIGINT, record_sig_handler);

  if (!RECORD_IS_REPLAY && ops != &record_core_ops)
    {
      if (record_resume_step)
	{
	  /* This is a single step.  */
	  return record_beneath_to_wait (record_beneath_to_wait_ops,
					 ptid, status, options);
	}
      else
	{
	  /* This is not a single step.  */
	  ptid_t ret;
	  CORE_ADDR tmp_pc;
	  struct gdbarch *gdbarch = target_thread_architecture (inferior_ptid);

	  while (1)
	    {
	      ret = record_beneath_to_wait (record_beneath_to_wait_ops,
					    ptid, status, options);

              if (single_step_breakpoints_inserted ())
                remove_single_step_breakpoints ();

	      if (record_resume_step)
	        return ret;

	      /* Is this a SIGTRAP?  */
	      if (status->kind == TARGET_WAITKIND_STOPPED
		  && status->value.sig == TARGET_SIGNAL_TRAP)
		{
		  struct regcache *regcache;
		  struct address_space *aspace;

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
		  else if (breakpoint_inserted_here_p (aspace, tmp_pc))
		    {
		      /* There is a breakpoint here.  Let the core
			 handle it.  */
		      if (software_breakpoint_inserted_here_p (aspace, tmp_pc))
			{
			  struct gdbarch *gdbarch
			    = get_regcache_arch (regcache);
			  CORE_ADDR decr_pc_after_break
			    = gdbarch_decr_pc_after_break (gdbarch);
			  if (decr_pc_after_break)
			    regcache_write_pc (regcache,
					       tmp_pc + decr_pc_after_break);
			}
		    }
		  else
		    {
		      /* This is a single-step trap.  Record the
		         insn and issue another step.
                         FIXME: this part can be a random SIGTRAP too.
                         But GDB cannot handle it.  */
                      int step = 1;

		      if (!record_message_wrapper_safe (regcache,
                                                        TARGET_SIGNAL_0))
  			{
                           status->kind = TARGET_WAITKIND_STOPPED;
                           status->value.sig = TARGET_SIGNAL_0;
                           break;
  			}

                      if (gdbarch_software_single_step_p (gdbarch))
			{
			  /* Try to insert the software single step breakpoint.
			     If insert success, set step to 0.  */
			  set_executing (inferior_ptid, 0);
			  reinit_frame_cache ();
			  if (gdbarch_software_single_step (gdbarch,
                                                            get_current_frame ()))
			    step = 0;
			  set_executing (inferior_ptid, 1);
			}

		      record_beneath_to_resume (record_beneath_to_resume_ops,
						ptid, step,
						TARGET_SIGNAL_0);
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
      int first_record_end = 1;
      struct cleanup *old_cleanups = make_cleanup (record_wait_cleanups, 0);
      CORE_ADDR tmp_pc;

      record_hw_watchpoint = 0;
      status->kind = TARGET_WAITKIND_STOPPED;

      /* Check breakpoint when forward execute.  */
      if (execution_direction == EXEC_FORWARD)
	{
	  tmp_pc = regcache_read_pc (regcache);
	  if (breakpoint_inserted_here_p (aspace, tmp_pc))
	    {
	      int decr_pc_after_break = gdbarch_decr_pc_after_break (gdbarch);

	      if (record_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "Process record: break at %s.\n",
				    paddress (gdbarch, tmp_pc));

	      if (decr_pc_after_break
		  && !record_resume_step
		  && software_breakpoint_inserted_here_p (aspace, tmp_pc))
		regcache_write_pc (regcache,
				   tmp_pc + decr_pc_after_break);
	      goto replay_out;
	    }
	}

      /* If GDB is in terminal_inferior mode, it will not get the signal.
         And in GDB replay mode, GDB doesn't need to be in terminal_inferior
         mode, because inferior will not executed.
         Then set it to terminal_ours to make GDB get the signal.  */
      target_terminal_ours ();

      /* In EXEC_FORWARD mode, record_list points to the tail of prev
         instruction.  */
      if (execution_direction == EXEC_FORWARD && record_list->next)
	record_list = record_list->next;

      /* Loop over the record_list, looking for the next place to
	 stop.  */
      do
	{
	  /* Check for beginning and end of log.  */
	  if (execution_direction == EXEC_REVERSE
	      && record_list == &record_first)
	    {
	      /* Hit beginning of record log in reverse.  */
	      status->kind = TARGET_WAITKIND_NO_HISTORY;
	      break;
	    }
	  if (execution_direction != EXEC_REVERSE && !record_list->next)
	    {
	      /* Hit end of record log going forward.  */
	      status->kind = TARGET_WAITKIND_NO_HISTORY;
	      break;
	    }

          record_exec_insn (regcache, gdbarch, record_list);

	  if (record_list->type == record_end)
	    {
	      if (record_debug > 1)
		fprintf_unfiltered (gdb_stdlog,
				    "Process record: record_end %s to "
				    "inferior.\n",
				    host_address_to_string (record_list));

	      if (first_record_end && execution_direction == EXEC_REVERSE)
		{
		  /* When reverse excute, the first record_end is the part of
		     current instruction.  */
		  first_record_end = 0;
		}
	      else
		{
		  /* In EXEC_REVERSE mode, this is the record_end of prev
		     instruction.
		     In EXEC_FORWARD mode, this is the record_end of current
		     instruction.  */
		  /* step */
		  if (record_resume_step)
		    {
		      if (record_debug > 1)
			fprintf_unfiltered (gdb_stdlog,
					    "Process record: step.\n");
		      continue_flag = 0;
		    }

		  /* check breakpoint */
		  tmp_pc = regcache_read_pc (regcache);
		  if (breakpoint_inserted_here_p (aspace, tmp_pc))
		    {
		      int decr_pc_after_break
			= gdbarch_decr_pc_after_break (gdbarch);

		      if (record_debug)
			fprintf_unfiltered (gdb_stdlog,
					    "Process record: break "
					    "at %s.\n",
					    paddress (gdbarch, tmp_pc));
		      if (decr_pc_after_break
			  && execution_direction == EXEC_FORWARD
			  && !record_resume_step
			  && software_breakpoint_inserted_here_p (aspace,
								  tmp_pc))
			regcache_write_pc (regcache,
					   tmp_pc + decr_pc_after_break);
		      continue_flag = 0;
		    }

		  if (record_hw_watchpoint)
		    {
		      if (record_debug)
			fprintf_unfiltered (gdb_stdlog,
					    "Process record: hit hw "
					    "watchpoint.\n");
		      continue_flag = 0;
		    }
		  /* Check target signal */
		  if (record_list->u.end.sigval != TARGET_SIGNAL_0)
		    /* FIXME: better way to check */
		    continue_flag = 0;
		}
	    }

	  if (continue_flag)
	    {
	      if (execution_direction == EXEC_REVERSE)
		{
		  if (record_list->prev)
		    record_list = record_list->prev;
		}
	      else
		{
		  if (record_list->next)
		    record_list = record_list->next;
		}
	    }
	}
      while (continue_flag);

replay_out:
      if (record_get_sig)
	status->value.sig = TARGET_SIGNAL_INT;
      else if (record_list->u.end.sigval != TARGET_SIGNAL_0)
	/* FIXME: better way to check */
	status->value.sig = record_list->u.end.sigval;
      else
	status->value.sig = TARGET_SIGNAL_TRAP;

      discard_cleanups (old_cleanups);
    }

  signal (SIGINT, handle_sigint);

  do_cleanups (set_cleanups);
  return inferior_ptid;
}

static int
record_stopped_by_watchpoint (void)
{
  if (RECORD_IS_REPLAY)
    return record_hw_watchpoint;
  else
    return record_beneath_to_stopped_by_watchpoint ();
}

static int
record_stopped_data_address (struct target_ops *ops, CORE_ADDR *addr_p)
{
  if (RECORD_IS_REPLAY)
    return 0;
  else
    return record_beneath_to_stopped_data_address (ops, addr_p);
}

/* "to_disconnect" method for process record target.  */

static void
record_disconnect (struct target_ops *target, char *args, int from_tty)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_disconnect\n");

  unpush_target (&record_ops);
  target_disconnect (args, from_tty);
}

/* "to_detach" method for process record target.  */

static void
record_detach (struct target_ops *ops, char *args, int from_tty)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_detach\n");

  unpush_target (&record_ops);
  target_detach (args, from_tty);
}

/* "to_mourn_inferior" method for process record target.  */

static void
record_mourn_inferior (struct target_ops *ops)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: "
			            "record_mourn_inferior\n");

  unpush_target (&record_ops);
  target_mourn_inferior ();
}

/* Close process record target before killing the inferior process.  */

static void
record_kill (struct target_ops *ops)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_kill\n");

  unpush_target (&record_ops);
  target_kill ();
}

/* Record registers change (by user or by GDB) to list as an instruction.  */

static void
record_registers_change (struct regcache *regcache, int regnum)
{
  /* Check record_insn_num.  */
  record_check_insn_num (0);

  record_arch_list_head = NULL;
  record_arch_list_tail = NULL;

  if (regnum < 0)
    {
      int i;

      for (i = 0; i < gdbarch_num_regs (get_regcache_arch (regcache)); i++)
	{
	  if (record_arch_list_add_reg (regcache, i))
	    {
	      record_list_release (record_arch_list_tail);
	      error (_("Process record: failed to record execution log."));
	    }
	}
    }
  else
    {
      if (record_arch_list_add_reg (regcache, regnum))
	{
	  record_list_release (record_arch_list_tail);
	  error (_("Process record: failed to record execution log."));
	}
    }
  if (record_arch_list_add_end ())
    {
      record_list_release (record_arch_list_tail);
      error (_("Process record: failed to record execution log."));
    }
  record_list->next = record_arch_list_head;
  record_arch_list_head->prev = record_list;
  record_list = record_arch_list_tail;

  if (record_insn_num == record_insn_max_num && record_insn_max_num)
    record_list_release_first ();
  else
    record_insn_num++;
}

/* "to_store_registers" method for process record target.  */

static void
record_store_registers (struct target_ops *ops, struct regcache *regcache,
                        int regno)
{
  if (!record_gdb_operation_disable)
    {
      if (RECORD_IS_REPLAY)
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
	  record_list_release_following (record_list);
	}

      record_registers_change (regcache, regno);
    }
  record_beneath_to_store_registers (record_beneath_to_store_registers_ops,
                                     regcache, regno);
}

/* "to_xfer_partial" method.  Behavior is conditional on RECORD_IS_REPLAY.
   In replay mode, we cannot write memory unles we are willing to
   invalidate the record/replay log from this point forward.  */

static LONGEST
record_xfer_partial (struct target_ops *ops, enum target_object object,
		     const char *annex, gdb_byte *readbuf,
		     const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  if (!record_gdb_operation_disable
      && (object == TARGET_OBJECT_MEMORY
	  || object == TARGET_OBJECT_RAW_MEMORY) && writebuf)
    {
      if (RECORD_IS_REPLAY)
	{
	  /* Let user choose if he wants to write memory or not.  */
	  if (!query (_("Because GDB is in replay mode, writing to memory "
		        "will make the execution log unusable from this "
		        "point onward.  Write memory at address %s?"),
		       paddress (target_gdbarch, offset)))
	    error (_("Process record canceled the operation."));

	  /* Destroy the record from here forward.  */
	  record_list_release_following (record_list);
	}

      /* Check record_insn_num */
      record_check_insn_num (0);

      /* Record registers change to list as an instruction.  */
      record_arch_list_head = NULL;
      record_arch_list_tail = NULL;
      if (record_arch_list_add_mem (offset, len))
	{
	  record_list_release (record_arch_list_tail);
	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"Process record: failed to record "
				"execution log.");
	  return -1;
	}
      if (record_arch_list_add_end ())
	{
	  record_list_release (record_arch_list_tail);
	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"Process record: failed to record "
				"execution log.");
	  return -1;
	}
      record_list->next = record_arch_list_head;
      record_arch_list_head->prev = record_list;
      record_list = record_arch_list_tail;

      if (record_insn_num == record_insn_max_num && record_insn_max_num)
	record_list_release_first ();
      else
	record_insn_num++;
    }

  return record_beneath_to_xfer_partial (record_beneath_to_xfer_partial_ops,
                                         object, annex, readbuf, writebuf,
                                         offset, len);
}

/* Behavior is conditional on RECORD_IS_REPLAY.
   We will not actually insert or remove breakpoints when replaying,
   nor when recording.  */

static int
record_insert_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  if (!RECORD_IS_REPLAY)
    {
      struct cleanup *old_cleanups = record_gdb_operation_disable_set ();
      int ret = record_beneath_to_insert_breakpoint (gdbarch, bp_tgt);

      do_cleanups (old_cleanups);

      return ret;
    }

  return 0;
}

/* "to_remove_breakpoint" method for process record target.  */

static int
record_remove_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  if (!RECORD_IS_REPLAY)
    {
      struct cleanup *old_cleanups = record_gdb_operation_disable_set ();
      int ret = record_beneath_to_remove_breakpoint (gdbarch, bp_tgt);

      do_cleanups (old_cleanups);

      return ret;
    }

  return 0;
}

/* "to_can_execute_reverse" method for process record target.  */

static int
record_can_execute_reverse (void)
{
  return 1;
}

/* "to_get_bookmark" method for process record and prec over core.  */

static gdb_byte *
record_get_bookmark (char *args, int from_tty)
{
  gdb_byte *ret = NULL;

  /* Return stringified form of instruction count.  */
  if (record_list && record_list->type == record_end)
    ret = xstrdup (pulongest (record_list->u.end.insn_num));

  if (record_debug)
    {
      if (ret)
	fprintf_unfiltered (gdb_stdlog,
			    "record_get_bookmark returns %s\n", ret);
      else
	fprintf_unfiltered (gdb_stdlog,
			    "record_get_bookmark returns NULL\n");
    }
  return ret;
}

/* The implementation of the command "record goto".  */
static void cmd_record_goto (char *, int);

/* "to_goto_bookmark" method for process record and prec over core.  */

static void
record_goto_bookmark (gdb_byte *bookmark, int from_tty)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog,
			"record_goto_bookmark receives %s\n", bookmark);

  if (bookmark[0] == '\'' || bookmark[0] == '\"')
    {
      if (bookmark[strlen (bookmark) - 1] != bookmark[0])
	error (_("Unbalanced quotes: %s"), bookmark);

      /* Strip trailing quote.  */
      bookmark[strlen (bookmark) - 1] = '\0';
      /* Strip leading quote.  */
      bookmark++;
      /* Pass along to cmd_record_goto.  */
    }

  cmd_record_goto ((char *) bookmark, from_tty);
  return;
}

static void
init_record_ops (void)
{
  record_ops.to_shortname = "record";
  record_ops.to_longname = "Process record and replay target";
  record_ops.to_doc =
    "Log program while executing and replay execution from log.";
  record_ops.to_open = record_open;
  record_ops.to_close = record_close;
  record_ops.to_resume = record_resume;
  record_ops.to_wait = record_wait;
  record_ops.to_disconnect = record_disconnect;
  record_ops.to_detach = record_detach;
  record_ops.to_mourn_inferior = record_mourn_inferior;
  record_ops.to_kill = record_kill;
  record_ops.to_create_inferior = find_default_create_inferior;
  record_ops.to_store_registers = record_store_registers;
  record_ops.to_xfer_partial = record_xfer_partial;
  record_ops.to_insert_breakpoint = record_insert_breakpoint;
  record_ops.to_remove_breakpoint = record_remove_breakpoint;
  record_ops.to_stopped_by_watchpoint = record_stopped_by_watchpoint;
  record_ops.to_stopped_data_address = record_stopped_data_address;
  record_ops.to_can_execute_reverse = record_can_execute_reverse;
  record_ops.to_stratum = record_stratum;
  /* Add bookmark target methods.  */
  record_ops.to_get_bookmark = record_get_bookmark;
  record_ops.to_goto_bookmark = record_goto_bookmark;
  record_ops.to_magic = OPS_MAGIC;
}

/* "to_resume" method for prec over corefile.  */

static void
record_core_resume (struct target_ops *ops, ptid_t ptid, int step,
                    enum target_signal signal)
{
  record_resume_step = step;
}

/* "to_kill" method for prec over corefile.  */

static void
record_core_kill (struct target_ops *ops)
{
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Process record: record_core_kill\n");

  unpush_target (&record_core_ops);
}

/* "to_fetch_registers" method for prec over corefile.  */

static void
record_core_fetch_registers (struct target_ops *ops,
                             struct regcache *regcache,
                             int regno)
{
  if (regno < 0)
    {
      int num = gdbarch_num_regs (get_regcache_arch (regcache));
      int i;

      for (i = 0; i < num; i ++)
        regcache_raw_supply (regcache, i,
                             record_core_regbuf + MAX_REGISTER_SIZE * i);
    }
  else
    regcache_raw_supply (regcache, regno,
                         record_core_regbuf + MAX_REGISTER_SIZE * regno);
}

/* "to_prepare_to_store" method for prec over corefile.  */

static void
record_core_prepare_to_store (struct regcache *regcache)
{
}

/* "to_store_registers" method for prec over corefile.  */

static void
record_core_store_registers (struct target_ops *ops,
                             struct regcache *regcache,
                             int regno)
{
  if (record_gdb_operation_disable)
    regcache_raw_collect (regcache, regno,
                          record_core_regbuf + MAX_REGISTER_SIZE * regno);
  else
    error (_("You can't do that without a process to debug."));
}

/* "to_xfer_partial" method for prec over corefile.  */

static LONGEST
record_core_xfer_partial (struct target_ops *ops, enum target_object object,
		          const char *annex, gdb_byte *readbuf,
		          const gdb_byte *writebuf, ULONGEST offset,
                          LONGEST len)
{
  if (object == TARGET_OBJECT_MEMORY)
    {
      if (record_gdb_operation_disable || !writebuf)
	{
	  struct target_section *p;

	  for (p = record_core_start; p < record_core_end; p++)
	    {
	      if (offset >= p->addr)
		{
		  struct record_core_buf_entry *entry;
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
		      return len;
		    }
		  /* Get record_core_buf_entry.  */
		  for (entry = record_core_buf_list; entry;
		       entry = entry->prev)
		    if (entry->p == p)
		      break;
		  if (writebuf)
		    {
		      if (!entry)
			{
			  /* Add a new entry.  */
			  entry = (struct record_core_buf_entry *)
			    xmalloc (sizeof (struct record_core_buf_entry));
			  entry->p = p;
			  if (!bfd_malloc_and_get_section (p->bfd,
							   p->the_bfd_section,
							   &entry->buf))
			    {
			      xfree (entry);
			      return 0;
			    }
			  entry->prev = record_core_buf_list;
			  record_core_buf_list = entry;
			}

		      memcpy (entry->buf + sec_offset, writebuf,
			      (size_t) len);
		    }
		  else
		    {
		      if (!entry)
			return record_beneath_to_xfer_partial
			  (record_beneath_to_xfer_partial_ops,
			   object, annex, readbuf, writebuf,
			   offset, len);

		      memcpy (readbuf, entry->buf + sec_offset,
			      (size_t) len);
		    }

		  return len;
		}
	    }

	  return -1;
	}
      else
	error (_("You can't do that without a process to debug."));
    }

  return record_beneath_to_xfer_partial (record_beneath_to_xfer_partial_ops,
                                         object, annex, readbuf, writebuf,
                                         offset, len);
}

/* "to_insert_breakpoint" method for prec over corefile.  */

static int
record_core_insert_breakpoint (struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  return 0;
}

/* "to_remove_breakpoint" method for prec over corefile.  */

static int
record_core_remove_breakpoint (struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  return 0;
}

/* "to_has_execution" method for prec over corefile.  */

static int
record_core_has_execution (struct target_ops *ops, ptid_t the_ptid)
{
  return 1;
}

static void
init_record_core_ops (void)
{
  record_core_ops.to_shortname = "record-core";
  record_core_ops.to_longname = "Process record and replay target";
  record_core_ops.to_doc =
    "Log program while executing and replay execution from log.";
  record_core_ops.to_open = record_open;
  record_core_ops.to_close = record_close;
  record_core_ops.to_resume = record_core_resume;
  record_core_ops.to_wait = record_wait;
  record_core_ops.to_kill = record_core_kill;
  record_core_ops.to_fetch_registers = record_core_fetch_registers;
  record_core_ops.to_prepare_to_store = record_core_prepare_to_store;
  record_core_ops.to_store_registers = record_core_store_registers;
  record_core_ops.to_xfer_partial = record_core_xfer_partial;
  record_core_ops.to_insert_breakpoint = record_core_insert_breakpoint;
  record_core_ops.to_remove_breakpoint = record_core_remove_breakpoint;
  record_core_ops.to_stopped_by_watchpoint = record_stopped_by_watchpoint;
  record_core_ops.to_stopped_data_address = record_stopped_data_address;
  record_core_ops.to_can_execute_reverse = record_can_execute_reverse;
  record_core_ops.to_has_execution = record_core_has_execution;
  record_core_ops.to_stratum = record_stratum;
  /* Add bookmark target methods.  */
  record_core_ops.to_get_bookmark = record_get_bookmark;
  record_core_ops.to_goto_bookmark = record_goto_bookmark;
  record_core_ops.to_magic = OPS_MAGIC;
}

/* Implement "show record debug" command.  */

static void
show_record_debug (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of process record target is %s.\n"),
		    value);
}

/* Alias for "target record".  */

static void
cmd_record_start (char *args, int from_tty)
{
  execute_command ("target record", from_tty);
}

/* Truncate the record log from the present point
   of replay until the end.  */

static void
cmd_record_delete (char *args, int from_tty)
{
  if (current_target.to_stratum == record_stratum)
    {
      if (RECORD_IS_REPLAY)
	{
	  if (!from_tty || query (_("Delete the log from this point forward "
		                    "and begin to record the running message "
		                    "at current PC?")))
	    record_list_release_following (record_list);
	}
      else
	  printf_unfiltered (_("Already at end of record list.\n"));

    }
  else
    printf_unfiltered (_("Process record is not started.\n"));
}

/* Implement the "stoprecord" or "record stop" command.  */

static void
cmd_record_stop (char *args, int from_tty)
{
  if (current_target.to_stratum == record_stratum)
    {
      unpush_target (&record_ops);
      printf_unfiltered (_("Process record is stopped and all execution "
                           "logs are deleted.\n"));
    }
  else
    printf_unfiltered (_("Process record is not started.\n"));
}

/* Set upper limit of record log size.  */

static void
set_record_insn_max_num (char *args, int from_tty, struct cmd_list_element *c)
{
  if (record_insn_num > record_insn_max_num && record_insn_max_num)
    {
      /* Count down record_insn_num while releasing records from list.  */
      while (record_insn_num > record_insn_max_num)
	{
	  record_list_release_first ();
	  record_insn_num--;
	}
    }
}

static struct cmd_list_element *record_cmdlist, *set_record_cmdlist,
			       *show_record_cmdlist, *info_record_cmdlist;

static void
set_record_command (char *args, int from_tty)
{
  printf_unfiltered (_("\"set record\" must be followed "
		       "by an apporpriate subcommand.\n"));
  help_list (set_record_cmdlist, "set record ", all_commands, gdb_stdout);
}

static void
show_record_command (char *args, int from_tty)
{
  cmd_show_list (show_record_cmdlist, from_tty, "");
}

/* Display some statistics about the execution log.  */

static void
info_record_command (char *args, int from_tty)
{
  struct record_entry *p;

  if (current_target.to_stratum == record_stratum)
    {
      if (RECORD_IS_REPLAY)
	printf_filtered (_("Replay mode:\n"));
      else
	printf_filtered (_("Record mode:\n"));

      /* Find entry for first actual instruction in the log.  */
      for (p = record_first.next;
	   p != NULL && p->type != record_end;
	   p = p->next)
	;

      /* Do we have a log at all?  */
      if (p != NULL && p->type == record_end)
	{
	  /* Display instruction number for first instruction in the log.  */
	  printf_filtered (_("Lowest recorded instruction number is %s.\n"),
			   pulongest (p->u.end.insn_num));

	  /* If in replay mode, display where we are in the log.  */
	  if (RECORD_IS_REPLAY)
	    printf_filtered (_("Current instruction number is %s.\n"),
			     pulongest (record_list->u.end.insn_num));

	  /* Display instruction number for last instruction in the log.  */
	  printf_filtered (_("Highest recorded instruction number is %s.\n"), 
			   pulongest (record_insn_count));

	  /* Display log count.  */
	  printf_filtered (_("Log contains %d instructions.\n"), 
			   record_insn_num);
	}
      else
	{
	  printf_filtered (_("No instructions have been logged.\n"));
	}
    }
  else
    {
      printf_filtered (_("target record is not active.\n"));
    }

  /* Display max log size.  */
  printf_filtered (_("Max logged instructions is %d.\n"),
		   record_insn_max_num);
}

/* Record log save-file format
   Version 1 (never released)

   Header:
     4 bytes: magic number htonl(0x20090829).
       NOTE: be sure to change whenever this file format changes!

   Records:
     record_end:
       1 byte:  record type (record_end, see enum record_type).
     record_reg:
       1 byte:  record type (record_reg, see enum record_type).
       8 bytes: register id (network byte order).
       MAX_REGISTER_SIZE bytes: register value.
     record_mem:
       1 byte:  record type (record_mem, see enum record_type).
       8 bytes: memory length (network byte order).
       8 bytes: memory address (network byte order).
       n bytes: memory value (n == memory length).

   Version 2
     4 bytes: magic number netorder32(0x20091016).
       NOTE: be sure to change whenever this file format changes!

   Records:
     record_end:
       1 byte:  record type (record_end, see enum record_type).
       4 bytes: signal
       4 bytes: instruction count
     record_reg:
       1 byte:  record type (record_reg, see enum record_type).
       4 bytes: register id (network byte order).
       n bytes: register value (n == actual register size).
                (eg. 4 bytes for x86 general registers).
     record_mem:
       1 byte:  record type (record_mem, see enum record_type).
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
record_restore (void)
{
  uint32_t magic;
  struct cleanup *old_cleanups;
  struct record_entry *rec;
  asection *osec;
  uint32_t osec_size;
  int bfd_offset = 0;
  struct regcache *regcache;

  /* We restore the execution log from the open core bfd,
     if there is one.  */
  if (core_bfd == NULL)
    return;

  /* "record_restore" can only be called when record list is empty.  */
  gdb_assert (record_first.next == NULL);
 
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
  if (magic != RECORD_FILE_MAGIC)
    error (_("Version mis-match or file format error in core file %s."),
	   bfd_get_filename (core_bfd));
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog,
			"  Reading 4-byte magic cookie "
			"RECORD_FILE_MAGIC (0x%s)\n",
			phex_nz (netorder32 (magic), 4));

  /* Restore the entries in recfd into record_arch_list_head and
     record_arch_list_tail.  */
  record_arch_list_head = NULL;
  record_arch_list_tail = NULL;
  record_insn_num = 0;
  old_cleanups = make_cleanup (record_arch_list_cleanups, 0);
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
        case record_reg: /* reg */
          /* Get register number to regnum.  */
          bfdcore_read (core_bfd, osec, &regnum,
			sizeof (regnum), &bfd_offset);
	  regnum = netorder32 (regnum);

          rec = record_reg_alloc (regcache, regnum);

          /* Get val.  */
          bfdcore_read (core_bfd, osec, record_get_loc (rec),
			rec->u.reg.len, &bfd_offset);

	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"  Reading register %d (1 "
				"plus %lu plus %d bytes)\n",
				rec->u.reg.num,
				(unsigned long) sizeof (regnum),
				rec->u.reg.len);
          break;

        case record_mem: /* mem */
          /* Get len.  */
          bfdcore_read (core_bfd, osec, &len, 
			sizeof (len), &bfd_offset);
	  len = netorder32 (len);

          /* Get addr.  */
          bfdcore_read (core_bfd, osec, &addr,
			sizeof (addr), &bfd_offset);
	  addr = netorder64 (addr);

          rec = record_mem_alloc (addr, len);

          /* Get val.  */
          bfdcore_read (core_bfd, osec, record_get_loc (rec),
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

        case record_end: /* end */
          rec = record_end_alloc ();
          record_insn_num ++;

	  /* Get signal value.  */
	  bfdcore_read (core_bfd, osec, &signal, 
			sizeof (signal), &bfd_offset);
	  signal = netorder32 (signal);
	  rec->u.end.sigval = signal;

	  /* Get insn count.  */
	  bfdcore_read (core_bfd, osec, &count, 
			sizeof (count), &bfd_offset);
	  count = netorder32 (count);
	  rec->u.end.insn_num = count;
	  record_insn_count = count + 1;
	  if (record_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"  Reading record_end (1 + "
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
      record_arch_list_add (rec);
    }

  discard_cleanups (old_cleanups);

  /* Add record_arch_list_head to the end of record list.  */
  record_first.next = record_arch_list_head;
  record_arch_list_head->prev = &record_first;
  record_arch_list_tail->next = NULL;
  record_list = &record_first;

  /* Update record_insn_max_num.  */
  if (record_insn_num > record_insn_max_num)
    {
      record_insn_max_num = record_insn_num;
      warning (_("Auto increase record/replay buffer limit to %d."),
               record_insn_max_num);
    }

  /* Succeeded.  */
  printf_filtered (_("Restored records from core file %s.\n"),
		   bfd_get_filename (core_bfd));

  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC);
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
cmd_record_restore (char *args, int from_tty)
{
  core_file_command (args, from_tty);
  record_open (args, from_tty);
}

static void
record_save_cleanups (void *data)
{
  bfd *obfd = data;
  char *pathname = xstrdup (bfd_get_filename (obfd));

  bfd_close (obfd);
  unlink (pathname);
  xfree (pathname);
}

/* Save the execution log to a file.  We use a modified elf corefile
   format, with an extra section for our data.  */

static void
cmd_record_save (char *args, int from_tty)
{
  char *recfilename, recfilename_buffer[40];
  struct record_entry *cur_record_list;
  uint32_t magic;
  struct regcache *regcache;
  struct gdbarch *gdbarch;
  struct cleanup *old_cleanups;
  struct cleanup *set_cleanups;
  bfd *obfd;
  int save_size = 0;
  asection *osec = NULL;
  int bfd_offset = 0;

  if (strcmp (current_target.to_shortname, "record") != 0)
    error (_("This command can only be used with target 'record'.\n"
	     "Use 'target record' first.\n"));

  if (args && *args)
    recfilename = args;
  else
    {
      /* Default recfile name is "gdb_record.PID".  */
      snprintf (recfilename_buffer, sizeof (recfilename_buffer),
                "gdb_record.%d", PIDGET (inferior_ptid));
      recfilename = recfilename_buffer;
    }

  /* Open the save file.  */
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog, "Saving execution log to core file '%s'\n",
			recfilename);

  /* Open the output file.  */
  obfd = create_gcore_bfd (recfilename);
  old_cleanups = make_cleanup (record_save_cleanups, obfd);

  /* Save the current record entry to "cur_record_list".  */
  cur_record_list = record_list;

  /* Get the values of regcache and gdbarch.  */
  regcache = get_current_regcache ();
  gdbarch = get_regcache_arch (regcache);

  /* Disable the GDB operation record.  */
  set_cleanups = record_gdb_operation_disable_set ();

  /* Reverse execute to the begin of record list.  */
  while (1)
    {
      /* Check for beginning and end of log.  */
      if (record_list == &record_first)
        break;

      record_exec_insn (regcache, gdbarch, record_list);

      if (record_list->prev)
        record_list = record_list->prev;
    }

  /* Compute the size needed for the extra bfd section.  */
  save_size = 4;	/* magic cookie */
  for (record_list = record_first.next; record_list;
       record_list = record_list->next)
    switch (record_list->type)
      {
      case record_end:
	save_size += 1 + 4 + 4;
	break;
      case record_reg:
	save_size += 1 + 4 + record_list->u.reg.len;
	break;
      case record_mem:
	save_size += 1 + 4 + 8 + record_list->u.mem.len;
	break;
      }

  /* Make the new bfd section.  */
  osec = bfd_make_section_anyway_with_flags (obfd, "precord",
                                             SEC_HAS_CONTENTS
                                             | SEC_READONLY);
  if (osec == NULL)
    error (_("Failed to create 'precord' section for corefile %s: %s"),
	   recfilename,
           bfd_errmsg (bfd_get_error ()));
  bfd_set_section_size (obfd, osec, save_size);
  bfd_set_section_vma (obfd, osec, 0);
  bfd_set_section_alignment (obfd, osec, 0);
  bfd_section_lma (obfd, osec) = 0;

  /* Save corefile state.  */
  write_gcore_file (obfd);

  /* Write out the record log.  */
  /* Write the magic code.  */
  magic = RECORD_FILE_MAGIC;
  if (record_debug)
    fprintf_unfiltered (gdb_stdlog,
			"  Writing 4-byte magic cookie "
			"RECORD_FILE_MAGIC (0x%s)\n",
		      phex_nz (magic, 4));
  bfdcore_write (obfd, osec, &magic, sizeof (magic), &bfd_offset);

  /* Save the entries to recfd and forward execute to the end of
     record list.  */
  record_list = &record_first;
  while (1)
    {
      /* Save entry.  */
      if (record_list != &record_first)
        {
	  uint8_t type;
	  uint32_t regnum, len, signal, count;
          uint64_t addr;

	  type = record_list->type;
          bfdcore_write (obfd, osec, &type, sizeof (type), &bfd_offset);

          switch (record_list->type)
            {
            case record_reg: /* reg */
	      if (record_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "  Writing register %d (1 "
				    "plus %lu plus %d bytes)\n",
				    record_list->u.reg.num,
				    (unsigned long) sizeof (regnum),
				    record_list->u.reg.len);

              /* Write regnum.  */
              regnum = netorder32 (record_list->u.reg.num);
              bfdcore_write (obfd, osec, &regnum,
			     sizeof (regnum), &bfd_offset);

              /* Write regval.  */
              bfdcore_write (obfd, osec, record_get_loc (record_list),
			     record_list->u.reg.len, &bfd_offset);
              break;

            case record_mem: /* mem */
	      if (record_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "  Writing memory %s (1 plus "
				    "%lu plus %lu plus %d bytes)\n",
				    paddress (gdbarch,
					      record_list->u.mem.addr),
				    (unsigned long) sizeof (addr),
				    (unsigned long) sizeof (len),
				    record_list->u.mem.len);

	      /* Write memlen.  */
	      len = netorder32 (record_list->u.mem.len);
	      bfdcore_write (obfd, osec, &len, sizeof (len), &bfd_offset);

	      /* Write memaddr.  */
	      addr = netorder64 (record_list->u.mem.addr);
	      bfdcore_write (obfd, osec, &addr, 
			     sizeof (addr), &bfd_offset);

	      /* Write memval.  */
	      bfdcore_write (obfd, osec, record_get_loc (record_list),
			     record_list->u.mem.len, &bfd_offset);
              break;

              case record_end:
		if (record_debug)
		  fprintf_unfiltered (gdb_stdlog,
				      "  Writing record_end (1 + "
				      "%lu + %lu bytes)\n", 
				      (unsigned long) sizeof (signal),
				      (unsigned long) sizeof (count));
		/* Write signal value.  */
		signal = netorder32 (record_list->u.end.sigval);
		bfdcore_write (obfd, osec, &signal,
			       sizeof (signal), &bfd_offset);

		/* Write insn count.  */
		count = netorder32 (record_list->u.end.insn_num);
		bfdcore_write (obfd, osec, &count,
			       sizeof (count), &bfd_offset);
                break;
            }
        }

      /* Execute entry.  */
      record_exec_insn (regcache, gdbarch, record_list);

      if (record_list->next)
        record_list = record_list->next;
      else
        break;
    }

  /* Reverse execute to cur_record_list.  */
  while (1)
    {
      /* Check for beginning and end of log.  */
      if (record_list == cur_record_list)
        break;

      record_exec_insn (regcache, gdbarch, record_list);

      if (record_list->prev)
        record_list = record_list->prev;
    }

  do_cleanups (set_cleanups);
  bfd_close (obfd);
  discard_cleanups (old_cleanups);

  /* Succeeded.  */
  printf_filtered (_("Saved core file %s with execution log.\n"),
		   recfilename);
}

/* record_goto_insn -- rewind the record log (forward or backward,
   depending on DIR) to the given entry, changing the program state
   correspondingly.  */

static void
record_goto_insn (struct record_entry *entry,
		  enum exec_direction_kind dir)
{
  struct cleanup *set_cleanups = record_gdb_operation_disable_set ();
  struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  /* Assume everything is valid: we will hit the entry,
     and we will not hit the end of the recording.  */

  if (dir == EXEC_FORWARD)
    record_list = record_list->next;

  do
    {
      record_exec_insn (regcache, gdbarch, record_list);
      if (dir == EXEC_REVERSE)
	record_list = record_list->prev;
      else
	record_list = record_list->next;
    } while (record_list != entry);
  do_cleanups (set_cleanups);
}

/* "record goto" command.  Argument is an instruction number,
   as given by "info record".

   Rewinds the recording (forward or backward) to the given instruction.  */

static void
cmd_record_goto (char *arg, int from_tty)
{
  struct record_entry *p = NULL;
  ULONGEST target_insn = 0;

  if (arg == NULL || *arg == '\0')
    error (_("Command requires an argument (insn number to go to)."));

  if (strncmp (arg, "start", strlen ("start")) == 0
      || strncmp (arg, "begin", strlen ("begin")) == 0)
    {
      /* Special case.  Find first insn.  */
      for (p = &record_first; p != NULL; p = p->next)
	if (p->type == record_end)
	  break;
      if (p)
	target_insn = p->u.end.insn_num;
    }
  else if (strncmp (arg, "end", strlen ("end")) == 0)
    {
      /* Special case.  Find last insn.  */
      for (p = record_list; p->next != NULL; p = p->next)
	;
      for (; p!= NULL; p = p->prev)
	if (p->type == record_end)
	  break;
      if (p)
	target_insn = p->u.end.insn_num;
    }
  else
    {
      /* General case.  Find designated insn.  */
      target_insn = parse_and_eval_long (arg);

      for (p = &record_first; p != NULL; p = p->next)
	if (p->type == record_end && p->u.end.insn_num == target_insn)
	  break;
    }

  if (p == NULL)
    error (_("Target insn '%s' not found."), arg);
  else if (p == record_list)
    error (_("Already at insn '%s'."), arg);
  else if (p->u.end.insn_num > record_list->u.end.insn_num)
    {
      printf_filtered (_("Go forward to insn number %s\n"),
		       pulongest (target_insn));
      record_goto_insn (p, EXEC_FORWARD);
    }
  else
    {
      printf_filtered (_("Go backward to insn number %s\n"),
		       pulongest (target_insn));
      record_goto_insn (p, EXEC_REVERSE);
    }
  registers_changed ();
  reinit_frame_cache ();
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC);
}

void
_initialize_record (void)
{
  struct cmd_list_element *c;

  /* Init record_first.  */
  record_first.prev = NULL;
  record_first.next = NULL;
  record_first.type = record_end;

  init_record_ops ();
  add_target (&record_ops);
  init_record_core_ops ();
  add_target (&record_core_ops);

  add_setshow_zinteger_cmd ("record", no_class, &record_debug,
			    _("Set debugging of record/replay feature."),
			    _("Show debugging of record/replay feature."),
			    _("When enabled, debugging output for "
			      "record/replay feature is displayed."),
			    NULL, show_record_debug, &setdebuglist,
			    &showdebuglist);

  c = add_prefix_cmd ("record", class_obscure, cmd_record_start,
		      _("Abbreviated form of \"target record\" command."),
		      &record_cmdlist, "record ", 0, &cmdlist);
  set_cmd_completer (c, filename_completer);

  add_com_alias ("rec", "record", class_obscure, 1);
  add_prefix_cmd ("record", class_support, set_record_command,
		  _("Set record options"), &set_record_cmdlist,
		  "set record ", 0, &setlist);
  add_alias_cmd ("rec", "record", class_obscure, 1, &setlist);
  add_prefix_cmd ("record", class_support, show_record_command,
		  _("Show record options"), &show_record_cmdlist,
		  "show record ", 0, &showlist);
  add_alias_cmd ("rec", "record", class_obscure, 1, &showlist);
  add_prefix_cmd ("record", class_support, info_record_command,
		  _("Info record options"), &info_record_cmdlist,
		  "info record ", 0, &infolist);
  add_alias_cmd ("rec", "record", class_obscure, 1, &infolist);

  c = add_cmd ("save", class_obscure, cmd_record_save,
	       _("Save the execution log to a file.\n\
Argument is optional filename.\n\
Default filename is 'gdb_record.<process_id>'."),
	       &record_cmdlist);
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("restore", class_obscure, cmd_record_restore,
	       _("Restore the execution log from a file.\n\
Argument is filename.  File must be created with 'record save'."),
	       &record_cmdlist);
  set_cmd_completer (c, filename_completer);

  add_cmd ("delete", class_obscure, cmd_record_delete,
	   _("Delete the rest of execution log and start recording it anew."),
           &record_cmdlist);
  add_alias_cmd ("d", "delete", class_obscure, 1, &record_cmdlist);
  add_alias_cmd ("del", "delete", class_obscure, 1, &record_cmdlist);

  add_cmd ("stop", class_obscure, cmd_record_stop,
	   _("Stop the record/replay target."),
           &record_cmdlist);
  add_alias_cmd ("s", "stop", class_obscure, 1, &record_cmdlist);

  /* Record instructions number limit command.  */
  add_setshow_boolean_cmd ("stop-at-limit", no_class,
			   &record_stop_at_limit, _("\
Set whether record/replay stops when record/replay buffer becomes full."), _("\
Show whether record/replay stops when record/replay buffer becomes full."),
			   _("Default is ON.\n\
When ON, if the record/replay buffer becomes full, ask user what to do.\n\
When OFF, if the record/replay buffer becomes full,\n\
delete the oldest recorded instruction to make room for each new one."),
			   NULL, NULL,
			   &set_record_cmdlist, &show_record_cmdlist);
  add_setshow_uinteger_cmd ("insn-number-max", no_class,
			    &record_insn_max_num,
			    _("Set record/replay buffer limit."),
			    _("Show record/replay buffer limit."), _("\
Set the maximum number of instructions to be stored in the\n\
record/replay buffer.  Zero means unlimited.  Default is 200000."),
			    set_record_insn_max_num,
			    NULL, &set_record_cmdlist, &show_record_cmdlist);

  add_cmd ("goto", class_obscure, cmd_record_goto, _("\
Restore the program to its state at instruction number N.\n\
Argument is instruction number, as shown by 'info record'."),
	   &record_cmdlist);

  add_setshow_boolean_cmd ("memory-query", no_class,
			   &record_memory_query, _("\
Set whether query if PREC cannot record memory change of next instruction."),
                           _("\
Show whether query if PREC cannot record memory change of next instruction."),
                           _("\
Default is OFF.\n\
When ON, query if PREC cannot record memory change of next instruction."),
			   NULL, NULL,
			   &set_record_cmdlist, &show_record_cmdlist);

}
