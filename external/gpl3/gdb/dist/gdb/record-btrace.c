/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2015 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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
#include "record.h"
#include "gdbthread.h"
#include "target.h"
#include "gdbcmd.h"
#include "disasm.h"
#include "observer.h"
#include "cli/cli-utils.h"
#include "source.h"
#include "ui-out.h"
#include "symtab.h"
#include "filenames.h"
#include "regcache.h"
#include "frame-unwind.h"
#include "hashtab.h"
#include "infrun.h"
#include "event-loop.h"
#include "inf-loop.h"

/* The target_ops of record-btrace.  */
static struct target_ops record_btrace_ops;

/* A new thread observer enabling branch tracing for the new thread.  */
static struct observer *record_btrace_thread_observer;

/* Memory access types used in set/show record btrace replay-memory-access.  */
static const char replay_memory_access_read_only[] = "read-only";
static const char replay_memory_access_read_write[] = "read-write";
static const char *const replay_memory_access_types[] =
{
  replay_memory_access_read_only,
  replay_memory_access_read_write,
  NULL
};

/* The currently allowed replay memory access type.  */
static const char *replay_memory_access = replay_memory_access_read_only;

/* Command lists for "set/show record btrace".  */
static struct cmd_list_element *set_record_btrace_cmdlist;
static struct cmd_list_element *show_record_btrace_cmdlist;

/* The execution direction of the last resume we got.  See record-full.c.  */
static enum exec_direction_kind record_btrace_resume_exec_dir = EXEC_FORWARD;

/* The async event handler for reverse/replay execution.  */
static struct async_event_handler *record_btrace_async_inferior_event_handler;

/* A flag indicating that we are currently generating a core file.  */
static int record_btrace_generating_corefile;

/* Print a record-btrace debug message.  Use do ... while (0) to avoid
   ambiguities when used in if statements.  */

#define DEBUG(msg, args...)						\
  do									\
    {									\
      if (record_debug != 0)						\
        fprintf_unfiltered (gdb_stdlog,					\
			    "[record-btrace] " msg "\n", ##args);	\
    }									\
  while (0)


/* Update the branch trace for the current thread and return a pointer to its
   thread_info.

   Throws an error if there is no thread or no trace.  This function never
   returns NULL.  */

static struct thread_info *
require_btrace_thread (void)
{
  struct thread_info *tp;

  DEBUG ("require");

  tp = find_thread_ptid (inferior_ptid);
  if (tp == NULL)
    error (_("No thread."));

  btrace_fetch (tp);

  if (btrace_is_empty (tp))
    error (_("No trace."));

  return tp;
}

/* Update the branch trace for the current thread and return a pointer to its
   branch trace information struct.

   Throws an error if there is no thread or no trace.  This function never
   returns NULL.  */

static struct btrace_thread_info *
require_btrace (void)
{
  struct thread_info *tp;

  tp = require_btrace_thread ();

  return &tp->btrace;
}

/* Enable branch tracing for one thread.  Warn on errors.  */

static void
record_btrace_enable_warn (struct thread_info *tp)
{
  volatile struct gdb_exception error;

  TRY_CATCH (error, RETURN_MASK_ERROR)
    btrace_enable (tp);

  if (error.message != NULL)
    warning ("%s", error.message);
}

/* Callback function to disable branch tracing for one thread.  */

static void
record_btrace_disable_callback (void *arg)
{
  struct thread_info *tp;

  tp = arg;

  btrace_disable (tp);
}

/* Enable automatic tracing of new threads.  */

static void
record_btrace_auto_enable (void)
{
  DEBUG ("attach thread observer");

  record_btrace_thread_observer
    = observer_attach_new_thread (record_btrace_enable_warn);
}

/* Disable automatic tracing of new threads.  */

static void
record_btrace_auto_disable (void)
{
  /* The observer may have been detached, already.  */
  if (record_btrace_thread_observer == NULL)
    return;

  DEBUG ("detach thread observer");

  observer_detach_new_thread (record_btrace_thread_observer);
  record_btrace_thread_observer = NULL;
}

/* The record-btrace async event handler function.  */

static void
record_btrace_handle_async_inferior_event (gdb_client_data data)
{
  inferior_event_handler (INF_REG_EVENT, NULL);
}

/* The to_open method of target record-btrace.  */

static void
record_btrace_open (const char *args, int from_tty)
{
  struct cleanup *disable_chain;
  struct thread_info *tp;

  DEBUG ("open");

  record_preopen ();

  if (!target_has_execution)
    error (_("The program is not being run."));

  if (!target_supports_btrace ())
    error (_("Target does not support branch tracing."));

  if (non_stop)
    error (_("Record btrace can't debug inferior in non-stop mode."));

  gdb_assert (record_btrace_thread_observer == NULL);

  disable_chain = make_cleanup (null_cleanup, NULL);
  ALL_NON_EXITED_THREADS (tp)
    if (args == NULL || *args == 0 || number_is_in_list (args, tp->num))
      {
	btrace_enable (tp);

	make_cleanup (record_btrace_disable_callback, tp);
      }

  record_btrace_auto_enable ();

  push_target (&record_btrace_ops);

  record_btrace_async_inferior_event_handler
    = create_async_event_handler (record_btrace_handle_async_inferior_event,
				  NULL);
  record_btrace_generating_corefile = 0;

  observer_notify_record_changed (current_inferior (),  1);

  discard_cleanups (disable_chain);
}

/* The to_stop_recording method of target record-btrace.  */

static void
record_btrace_stop_recording (struct target_ops *self)
{
  struct thread_info *tp;

  DEBUG ("stop recording");

  record_btrace_auto_disable ();

  ALL_NON_EXITED_THREADS (tp)
    if (tp->btrace.target != NULL)
      btrace_disable (tp);
}

/* The to_close method of target record-btrace.  */

static void
record_btrace_close (struct target_ops *self)
{
  struct thread_info *tp;

  if (record_btrace_async_inferior_event_handler != NULL)
    delete_async_event_handler (&record_btrace_async_inferior_event_handler);

  /* Make sure automatic recording gets disabled even if we did not stop
     recording before closing the record-btrace target.  */
  record_btrace_auto_disable ();

  /* We should have already stopped recording.
     Tear down btrace in case we have not.  */
  ALL_NON_EXITED_THREADS (tp)
    btrace_teardown (tp);
}

/* The to_async method of target record-btrace.  */

static void
record_btrace_async (struct target_ops *ops,
		     void (*callback) (enum inferior_event_type event_type,
				       void *context),
		     void *context)
{
  if (callback != NULL)
    mark_async_event_handler (record_btrace_async_inferior_event_handler);
  else
    clear_async_event_handler (record_btrace_async_inferior_event_handler);

  ops->beneath->to_async (ops->beneath, callback, context);
}

/* The to_info_record method of target record-btrace.  */

static void
record_btrace_info (struct target_ops *self)
{
  struct btrace_thread_info *btinfo;
  struct thread_info *tp;
  unsigned int insns, calls;

  DEBUG ("info");

  tp = find_thread_ptid (inferior_ptid);
  if (tp == NULL)
    error (_("No thread."));

  btrace_fetch (tp);

  insns = 0;
  calls = 0;

  btinfo = &tp->btrace;

  if (!btrace_is_empty (tp))
    {
      struct btrace_call_iterator call;
      struct btrace_insn_iterator insn;

      btrace_call_end (&call, btinfo);
      btrace_call_prev (&call, 1);
      calls = btrace_call_number (&call);

      btrace_insn_end (&insn, btinfo);
      btrace_insn_prev (&insn, 1);
      insns = btrace_insn_number (&insn);
    }

  printf_unfiltered (_("Recorded %u instructions in %u functions for thread "
		       "%d (%s).\n"), insns, calls, tp->num,
		     target_pid_to_str (tp->ptid));

  if (btrace_is_replaying (tp))
    printf_unfiltered (_("Replay in progress.  At instruction %u.\n"),
		       btrace_insn_number (btinfo->replay));
}

/* Print an unsigned int.  */

static void
ui_out_field_uint (struct ui_out *uiout, const char *fld, unsigned int val)
{
  ui_out_field_fmt (uiout, fld, "%u", val);
}

/* Disassemble a section of the recorded instruction trace.  */

static void
btrace_insn_history (struct ui_out *uiout,
		     const struct btrace_insn_iterator *begin,
		     const struct btrace_insn_iterator *end, int flags)
{
  struct gdbarch *gdbarch;
  struct btrace_insn_iterator it;

  DEBUG ("itrace (0x%x): [%u; %u)", flags, btrace_insn_number (begin),
	 btrace_insn_number (end));

  gdbarch = target_gdbarch ();

  for (it = *begin; btrace_insn_cmp (&it, end) != 0; btrace_insn_next (&it, 1))
    {
      const struct btrace_insn *insn;

      insn = btrace_insn_get (&it);

      /* Print the instruction index.  */
      ui_out_field_uint (uiout, "index", btrace_insn_number (&it));
      ui_out_text (uiout, "\t");

      /* Disassembly with '/m' flag may not produce the expected result.
	 See PR gdb/11833.  */
      gdb_disassembly (gdbarch, uiout, NULL, flags, 1, insn->pc, insn->pc + 1);
    }
}

/* The to_insn_history method of target record-btrace.  */

static void
record_btrace_insn_history (struct target_ops *self, int size, int flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_insn_history *history;
  struct btrace_insn_iterator begin, end;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int context, covered;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "insn history");
  context = abs (size);
  if (context == 0)
    error (_("Bad record instruction-history-size."));

  btinfo = require_btrace ();
  history = btinfo->insn_history;
  if (history == NULL)
    {
      struct btrace_insn_iterator *replay;

      DEBUG ("insn-history (0x%x): %d", flags, size);

      /* If we're replaying, we start at the replay position.  Otherwise, we
	 start at the tail of the trace.  */
      replay = btinfo->replay;
      if (replay != NULL)
	begin = *replay;
      else
	btrace_insn_end (&begin, btinfo);

      /* We start from here and expand in the requested direction.  Then we
	 expand in the other direction, as well, to fill up any remaining
	 context.  */
      end = begin;
      if (size < 0)
	{
	  /* We want the current position covered, as well.  */
	  covered = btrace_insn_next (&end, 1);
	  covered += btrace_insn_prev (&begin, context - covered);
	  covered += btrace_insn_next (&end, context - covered);
	}
      else
	{
	  covered = btrace_insn_next (&end, context);
	  covered += btrace_insn_prev (&begin, context - covered);
	}
    }
  else
    {
      begin = history->begin;
      end = history->end;

      DEBUG ("insn-history (0x%x): %d, prev: [%u; %u)", flags, size,
	     btrace_insn_number (&begin), btrace_insn_number (&end));

      if (size < 0)
	{
	  end = begin;
	  covered = btrace_insn_prev (&begin, context);
	}
      else
	{
	  begin = end;
	  covered = btrace_insn_next (&end, context);
	}
    }

  if (covered > 0)
    btrace_insn_history (uiout, &begin, &end, flags);
  else
    {
      if (size < 0)
	printf_unfiltered (_("At the start of the branch trace record.\n"));
      else
	printf_unfiltered (_("At the end of the branch trace record.\n"));
    }

  btrace_set_insn_history (btinfo, &begin, &end);
  do_cleanups (uiout_cleanup);
}

/* The to_insn_history_range method of target record-btrace.  */

static void
record_btrace_insn_history_range (struct target_ops *self,
				  ULONGEST from, ULONGEST to, int flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_insn_history *history;
  struct btrace_insn_iterator begin, end;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int low, high;
  int found;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "insn history");
  low = from;
  high = to;

  DEBUG ("insn-history (0x%x): [%u; %u)", flags, low, high);

  /* Check for wrap-arounds.  */
  if (low != from || high != to)
    error (_("Bad range."));

  if (high < low)
    error (_("Bad range."));

  btinfo = require_btrace ();

  found = btrace_find_insn_by_number (&begin, btinfo, low);
  if (found == 0)
    error (_("Range out of bounds."));

  found = btrace_find_insn_by_number (&end, btinfo, high);
  if (found == 0)
    {
      /* Silently truncate the range.  */
      btrace_insn_end (&end, btinfo);
    }
  else
    {
      /* We want both begin and end to be inclusive.  */
      btrace_insn_next (&end, 1);
    }

  btrace_insn_history (uiout, &begin, &end, flags);
  btrace_set_insn_history (btinfo, &begin, &end);

  do_cleanups (uiout_cleanup);
}

/* The to_insn_history_from method of target record-btrace.  */

static void
record_btrace_insn_history_from (struct target_ops *self,
				 ULONGEST from, int size, int flags)
{
  ULONGEST begin, end, context;

  context = abs (size);
  if (context == 0)
    error (_("Bad record instruction-history-size."));

  if (size < 0)
    {
      end = from;

      if (from < context)
	begin = 0;
      else
	begin = from - context + 1;
    }
  else
    {
      begin = from;
      end = from + context - 1;

      /* Check for wrap-around.  */
      if (end < begin)
	end = ULONGEST_MAX;
    }

  record_btrace_insn_history_range (self, begin, end, flags);
}

/* Print the instruction number range for a function call history line.  */

static void
btrace_call_history_insn_range (struct ui_out *uiout,
				const struct btrace_function *bfun)
{
  unsigned int begin, end, size;

  size = VEC_length (btrace_insn_s, bfun->insn);
  gdb_assert (size > 0);

  begin = bfun->insn_offset;
  end = begin + size - 1;

  ui_out_field_uint (uiout, "insn begin", begin);
  ui_out_text (uiout, ",");
  ui_out_field_uint (uiout, "insn end", end);
}

/* Print the source line information for a function call history line.  */

static void
btrace_call_history_src_line (struct ui_out *uiout,
			      const struct btrace_function *bfun)
{
  struct symbol *sym;
  int begin, end;

  sym = bfun->sym;
  if (sym == NULL)
    return;

  ui_out_field_string (uiout, "file",
		       symtab_to_filename_for_display (symbol_symtab (sym)));

  begin = bfun->lbegin;
  end = bfun->lend;

  if (end < begin)
    return;

  ui_out_text (uiout, ":");
  ui_out_field_int (uiout, "min line", begin);

  if (end == begin)
    return;

  ui_out_text (uiout, ",");
  ui_out_field_int (uiout, "max line", end);
}

/* Get the name of a branch trace function.  */

static const char *
btrace_get_bfun_name (const struct btrace_function *bfun)
{
  struct minimal_symbol *msym;
  struct symbol *sym;

  if (bfun == NULL)
    return "??";

  msym = bfun->msym;
  sym = bfun->sym;

  if (sym != NULL)
    return SYMBOL_PRINT_NAME (sym);
  else if (msym != NULL)
    return MSYMBOL_PRINT_NAME (msym);
  else
    return "??";
}

/* Disassemble a section of the recorded function trace.  */

static void
btrace_call_history (struct ui_out *uiout,
		     const struct btrace_thread_info *btinfo,
		     const struct btrace_call_iterator *begin,
		     const struct btrace_call_iterator *end,
		     enum record_print_flag flags)
{
  struct btrace_call_iterator it;

  DEBUG ("ftrace (0x%x): [%u; %u)", flags, btrace_call_number (begin),
	 btrace_call_number (end));

  for (it = *begin; btrace_call_cmp (&it, end) < 0; btrace_call_next (&it, 1))
    {
      const struct btrace_function *bfun;
      struct minimal_symbol *msym;
      struct symbol *sym;

      bfun = btrace_call_get (&it);
      sym = bfun->sym;
      msym = bfun->msym;

      /* Print the function index.  */
      ui_out_field_uint (uiout, "index", bfun->number);
      ui_out_text (uiout, "\t");

      if ((flags & RECORD_PRINT_INDENT_CALLS) != 0)
	{
	  int level = bfun->level + btinfo->level, i;

	  for (i = 0; i < level; ++i)
	    ui_out_text (uiout, "  ");
	}

      if (sym != NULL)
	ui_out_field_string (uiout, "function", SYMBOL_PRINT_NAME (sym));
      else if (msym != NULL)
	ui_out_field_string (uiout, "function", MSYMBOL_PRINT_NAME (msym));
      else if (!ui_out_is_mi_like_p (uiout))
	ui_out_field_string (uiout, "function", "??");

      if ((flags & RECORD_PRINT_INSN_RANGE) != 0)
	{
	  ui_out_text (uiout, _("\tinst "));
	  btrace_call_history_insn_range (uiout, bfun);
	}

      if ((flags & RECORD_PRINT_SRC_LINE) != 0)
	{
	  ui_out_text (uiout, _("\tat "));
	  btrace_call_history_src_line (uiout, bfun);
	}

      ui_out_text (uiout, "\n");
    }
}

/* The to_call_history method of target record-btrace.  */

static void
record_btrace_call_history (struct target_ops *self, int size, int flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_call_history *history;
  struct btrace_call_iterator begin, end;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int context, covered;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "insn history");
  context = abs (size);
  if (context == 0)
    error (_("Bad record function-call-history-size."));

  btinfo = require_btrace ();
  history = btinfo->call_history;
  if (history == NULL)
    {
      struct btrace_insn_iterator *replay;

      DEBUG ("call-history (0x%x): %d", flags, size);

      /* If we're replaying, we start at the replay position.  Otherwise, we
	 start at the tail of the trace.  */
      replay = btinfo->replay;
      if (replay != NULL)
	{
	  begin.function = replay->function;
	  begin.btinfo = btinfo;
	}
      else
	btrace_call_end (&begin, btinfo);

      /* We start from here and expand in the requested direction.  Then we
	 expand in the other direction, as well, to fill up any remaining
	 context.  */
      end = begin;
      if (size < 0)
	{
	  /* We want the current position covered, as well.  */
	  covered = btrace_call_next (&end, 1);
	  covered += btrace_call_prev (&begin, context - covered);
	  covered += btrace_call_next (&end, context - covered);
	}
      else
	{
	  covered = btrace_call_next (&end, context);
	  covered += btrace_call_prev (&begin, context- covered);
	}
    }
  else
    {
      begin = history->begin;
      end = history->end;

      DEBUG ("call-history (0x%x): %d, prev: [%u; %u)", flags, size,
	     btrace_call_number (&begin), btrace_call_number (&end));

      if (size < 0)
	{
	  end = begin;
	  covered = btrace_call_prev (&begin, context);
	}
      else
	{
	  begin = end;
	  covered = btrace_call_next (&end, context);
	}
    }

  if (covered > 0)
    btrace_call_history (uiout, btinfo, &begin, &end, flags);
  else
    {
      if (size < 0)
	printf_unfiltered (_("At the start of the branch trace record.\n"));
      else
	printf_unfiltered (_("At the end of the branch trace record.\n"));
    }

  btrace_set_call_history (btinfo, &begin, &end);
  do_cleanups (uiout_cleanup);
}

/* The to_call_history_range method of target record-btrace.  */

static void
record_btrace_call_history_range (struct target_ops *self,
				  ULONGEST from, ULONGEST to, int flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_call_history *history;
  struct btrace_call_iterator begin, end;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int low, high;
  int found;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "func history");
  low = from;
  high = to;

  DEBUG ("call-history (0x%x): [%u; %u)", flags, low, high);

  /* Check for wrap-arounds.  */
  if (low != from || high != to)
    error (_("Bad range."));

  if (high < low)
    error (_("Bad range."));

  btinfo = require_btrace ();

  found = btrace_find_call_by_number (&begin, btinfo, low);
  if (found == 0)
    error (_("Range out of bounds."));

  found = btrace_find_call_by_number (&end, btinfo, high);
  if (found == 0)
    {
      /* Silently truncate the range.  */
      btrace_call_end (&end, btinfo);
    }
  else
    {
      /* We want both begin and end to be inclusive.  */
      btrace_call_next (&end, 1);
    }

  btrace_call_history (uiout, btinfo, &begin, &end, flags);
  btrace_set_call_history (btinfo, &begin, &end);

  do_cleanups (uiout_cleanup);
}

/* The to_call_history_from method of target record-btrace.  */

static void
record_btrace_call_history_from (struct target_ops *self,
				 ULONGEST from, int size, int flags)
{
  ULONGEST begin, end, context;

  context = abs (size);
  if (context == 0)
    error (_("Bad record function-call-history-size."));

  if (size < 0)
    {
      end = from;

      if (from < context)
	begin = 0;
      else
	begin = from - context + 1;
    }
  else
    {
      begin = from;
      end = from + context - 1;

      /* Check for wrap-around.  */
      if (end < begin)
	end = ULONGEST_MAX;
    }

  record_btrace_call_history_range (self, begin, end, flags);
}

/* The to_record_is_replaying method of target record-btrace.  */

static int
record_btrace_is_replaying (struct target_ops *self)
{
  struct thread_info *tp;

  ALL_NON_EXITED_THREADS (tp)
    if (btrace_is_replaying (tp))
      return 1;

  return 0;
}

/* The to_xfer_partial method of target record-btrace.  */

static enum target_xfer_status
record_btrace_xfer_partial (struct target_ops *ops, enum target_object object,
			    const char *annex, gdb_byte *readbuf,
			    const gdb_byte *writebuf, ULONGEST offset,
			    ULONGEST len, ULONGEST *xfered_len)
{
  struct target_ops *t;

  /* Filter out requests that don't make sense during replay.  */
  if (replay_memory_access == replay_memory_access_read_only
      && !record_btrace_generating_corefile
      && record_btrace_is_replaying (ops))
    {
      switch (object)
	{
	case TARGET_OBJECT_MEMORY:
	  {
	    struct target_section *section;

	    /* We do not allow writing memory in general.  */
	    if (writebuf != NULL)
	      {
		*xfered_len = len;
		return TARGET_XFER_UNAVAILABLE;
	      }

	    /* We allow reading readonly memory.  */
	    section = target_section_by_addr (ops, offset);
	    if (section != NULL)
	      {
		/* Check if the section we found is readonly.  */
		if ((bfd_get_section_flags (section->the_bfd_section->owner,
					    section->the_bfd_section)
		     & SEC_READONLY) != 0)
		  {
		    /* Truncate the request to fit into this section.  */
		    len = min (len, section->endaddr - offset);
		    break;
		  }
	      }

	    *xfered_len = len;
	    return TARGET_XFER_UNAVAILABLE;
	  }
	}
    }

  /* Forward the request.  */
  ops = ops->beneath;
  return ops->to_xfer_partial (ops, object, annex, readbuf, writebuf,
			       offset, len, xfered_len);
}

/* The to_insert_breakpoint method of target record-btrace.  */

static int
record_btrace_insert_breakpoint (struct target_ops *ops,
				 struct gdbarch *gdbarch,
				 struct bp_target_info *bp_tgt)
{
  volatile struct gdb_exception except;
  const char *old;
  int ret;

  /* Inserting breakpoints requires accessing memory.  Allow it for the
     duration of this function.  */
  old = replay_memory_access;
  replay_memory_access = replay_memory_access_read_write;

  ret = 0;
  TRY_CATCH (except, RETURN_MASK_ALL)
    ret = ops->beneath->to_insert_breakpoint (ops->beneath, gdbarch, bp_tgt);

  replay_memory_access = old;

  if (except.reason < 0)
    throw_exception (except);

  return ret;
}

/* The to_remove_breakpoint method of target record-btrace.  */

static int
record_btrace_remove_breakpoint (struct target_ops *ops,
				 struct gdbarch *gdbarch,
				 struct bp_target_info *bp_tgt)
{
  volatile struct gdb_exception except;
  const char *old;
  int ret;

  /* Removing breakpoints requires accessing memory.  Allow it for the
     duration of this function.  */
  old = replay_memory_access;
  replay_memory_access = replay_memory_access_read_write;

  ret = 0;
  TRY_CATCH (except, RETURN_MASK_ALL)
    ret = ops->beneath->to_remove_breakpoint (ops->beneath, gdbarch, bp_tgt);

  replay_memory_access = old;

  if (except.reason < 0)
    throw_exception (except);

  return ret;
}

/* The to_fetch_registers method of target record-btrace.  */

static void
record_btrace_fetch_registers (struct target_ops *ops,
			       struct regcache *regcache, int regno)
{
  struct btrace_insn_iterator *replay;
  struct thread_info *tp;

  tp = find_thread_ptid (inferior_ptid);
  gdb_assert (tp != NULL);

  replay = tp->btrace.replay;
  if (replay != NULL && !record_btrace_generating_corefile)
    {
      const struct btrace_insn *insn;
      struct gdbarch *gdbarch;
      int pcreg;

      gdbarch = get_regcache_arch (regcache);
      pcreg = gdbarch_pc_regnum (gdbarch);
      if (pcreg < 0)
	return;

      /* We can only provide the PC register.  */
      if (regno >= 0 && regno != pcreg)
	return;

      insn = btrace_insn_get (replay);
      gdb_assert (insn != NULL);

      regcache_raw_supply (regcache, regno, &insn->pc);
    }
  else
    {
      struct target_ops *t = ops->beneath;

      t->to_fetch_registers (t, regcache, regno);
    }
}

/* The to_store_registers method of target record-btrace.  */

static void
record_btrace_store_registers (struct target_ops *ops,
			       struct regcache *regcache, int regno)
{
  struct target_ops *t;

  if (!record_btrace_generating_corefile && record_btrace_is_replaying (ops))
    error (_("This record target does not allow writing registers."));

  gdb_assert (may_write_registers != 0);

  t = ops->beneath;
  t->to_store_registers (t, regcache, regno);
}

/* The to_prepare_to_store method of target record-btrace.  */

static void
record_btrace_prepare_to_store (struct target_ops *ops,
				struct regcache *regcache)
{
  struct target_ops *t;

  if (!record_btrace_generating_corefile && record_btrace_is_replaying (ops))
    return;

  t = ops->beneath;
  t->to_prepare_to_store (t, regcache);
}

/* The branch trace frame cache.  */

struct btrace_frame_cache
{
  /* The thread.  */
  struct thread_info *tp;

  /* The frame info.  */
  struct frame_info *frame;

  /* The branch trace function segment.  */
  const struct btrace_function *bfun;
};

/* A struct btrace_frame_cache hash table indexed by NEXT.  */

static htab_t bfcache;

/* hash_f for htab_create_alloc of bfcache.  */

static hashval_t
bfcache_hash (const void *arg)
{
  const struct btrace_frame_cache *cache = arg;

  return htab_hash_pointer (cache->frame);
}

/* eq_f for htab_create_alloc of bfcache.  */

static int
bfcache_eq (const void *arg1, const void *arg2)
{
  const struct btrace_frame_cache *cache1 = arg1;
  const struct btrace_frame_cache *cache2 = arg2;

  return cache1->frame == cache2->frame;
}

/* Create a new btrace frame cache.  */

static struct btrace_frame_cache *
bfcache_new (struct frame_info *frame)
{
  struct btrace_frame_cache *cache;
  void **slot;

  cache = FRAME_OBSTACK_ZALLOC (struct btrace_frame_cache);
  cache->frame = frame;

  slot = htab_find_slot (bfcache, cache, INSERT);
  gdb_assert (*slot == NULL);
  *slot = cache;

  return cache;
}

/* Extract the branch trace function from a branch trace frame.  */

static const struct btrace_function *
btrace_get_frame_function (struct frame_info *frame)
{
  const struct btrace_frame_cache *cache;
  const struct btrace_function *bfun;
  struct btrace_frame_cache pattern;
  void **slot;

  pattern.frame = frame;

  slot = htab_find_slot (bfcache, &pattern, NO_INSERT);
  if (slot == NULL)
    return NULL;

  cache = *slot;
  return cache->bfun;
}

/* Implement stop_reason method for record_btrace_frame_unwind.  */

static enum unwind_stop_reason
record_btrace_frame_unwind_stop_reason (struct frame_info *this_frame,
					void **this_cache)
{
  const struct btrace_frame_cache *cache;
  const struct btrace_function *bfun;

  cache = *this_cache;
  bfun = cache->bfun;
  gdb_assert (bfun != NULL);

  if (bfun->up == NULL)
    return UNWIND_UNAVAILABLE;

  return UNWIND_NO_REASON;
}

/* Implement this_id method for record_btrace_frame_unwind.  */

static void
record_btrace_frame_this_id (struct frame_info *this_frame, void **this_cache,
			     struct frame_id *this_id)
{
  const struct btrace_frame_cache *cache;
  const struct btrace_function *bfun;
  CORE_ADDR code, special;

  cache = *this_cache;

  bfun = cache->bfun;
  gdb_assert (bfun != NULL);

  while (bfun->segment.prev != NULL)
    bfun = bfun->segment.prev;

  code = get_frame_func (this_frame);
  special = bfun->number;

  *this_id = frame_id_build_unavailable_stack_special (code, special);

  DEBUG ("[frame] %s id: (!stack, pc=%s, special=%s)",
	 btrace_get_bfun_name (cache->bfun),
	 core_addr_to_string_nz (this_id->code_addr),
	 core_addr_to_string_nz (this_id->special_addr));
}

/* Implement prev_register method for record_btrace_frame_unwind.  */

static struct value *
record_btrace_frame_prev_register (struct frame_info *this_frame,
				   void **this_cache,
				   int regnum)
{
  const struct btrace_frame_cache *cache;
  const struct btrace_function *bfun, *caller;
  const struct btrace_insn *insn;
  struct gdbarch *gdbarch;
  CORE_ADDR pc;
  int pcreg;

  gdbarch = get_frame_arch (this_frame);
  pcreg = gdbarch_pc_regnum (gdbarch);
  if (pcreg < 0 || regnum != pcreg)
    throw_error (NOT_AVAILABLE_ERROR,
		 _("Registers are not available in btrace record history"));

  cache = *this_cache;
  bfun = cache->bfun;
  gdb_assert (bfun != NULL);

  caller = bfun->up;
  if (caller == NULL)
    throw_error (NOT_AVAILABLE_ERROR,
		 _("No caller in btrace record history"));

  if ((bfun->flags & BFUN_UP_LINKS_TO_RET) != 0)
    {
      insn = VEC_index (btrace_insn_s, caller->insn, 0);
      pc = insn->pc;
    }
  else
    {
      insn = VEC_last (btrace_insn_s, caller->insn);
      pc = insn->pc;

      pc += gdb_insn_length (gdbarch, pc);
    }

  DEBUG ("[frame] unwound PC in %s on level %d: %s",
	 btrace_get_bfun_name (bfun), bfun->level,
	 core_addr_to_string_nz (pc));

  return frame_unwind_got_address (this_frame, regnum, pc);
}

/* Implement sniffer method for record_btrace_frame_unwind.  */

static int
record_btrace_frame_sniffer (const struct frame_unwind *self,
			     struct frame_info *this_frame,
			     void **this_cache)
{
  const struct btrace_function *bfun;
  struct btrace_frame_cache *cache;
  struct thread_info *tp;
  struct frame_info *next;

  /* THIS_FRAME does not contain a reference to its thread.  */
  tp = find_thread_ptid (inferior_ptid);
  gdb_assert (tp != NULL);

  bfun = NULL;
  next = get_next_frame (this_frame);
  if (next == NULL)
    {
      const struct btrace_insn_iterator *replay;

      replay = tp->btrace.replay;
      if (replay != NULL)
	bfun = replay->function;
    }
  else
    {
      const struct btrace_function *callee;

      callee = btrace_get_frame_function (next);
      if (callee != NULL && (callee->flags & BFUN_UP_LINKS_TO_TAILCALL) == 0)
	bfun = callee->up;
    }

  if (bfun == NULL)
    return 0;

  DEBUG ("[frame] sniffed frame for %s on level %d",
	 btrace_get_bfun_name (bfun), bfun->level);

  /* This is our frame.  Initialize the frame cache.  */
  cache = bfcache_new (this_frame);
  cache->tp = tp;
  cache->bfun = bfun;

  *this_cache = cache;
  return 1;
}

/* Implement sniffer method for record_btrace_tailcall_frame_unwind.  */

static int
record_btrace_tailcall_frame_sniffer (const struct frame_unwind *self,
				      struct frame_info *this_frame,
				      void **this_cache)
{
  const struct btrace_function *bfun, *callee;
  struct btrace_frame_cache *cache;
  struct frame_info *next;

  next = get_next_frame (this_frame);
  if (next == NULL)
    return 0;

  callee = btrace_get_frame_function (next);
  if (callee == NULL)
    return 0;

  if ((callee->flags & BFUN_UP_LINKS_TO_TAILCALL) == 0)
    return 0;

  bfun = callee->up;
  if (bfun == NULL)
    return 0;

  DEBUG ("[frame] sniffed tailcall frame for %s on level %d",
	 btrace_get_bfun_name (bfun), bfun->level);

  /* This is our frame.  Initialize the frame cache.  */
  cache = bfcache_new (this_frame);
  cache->tp = find_thread_ptid (inferior_ptid);
  cache->bfun = bfun;

  *this_cache = cache;
  return 1;
}

static void
record_btrace_frame_dealloc_cache (struct frame_info *self, void *this_cache)
{
  struct btrace_frame_cache *cache;
  void **slot;

  cache = this_cache;

  slot = htab_find_slot (bfcache, cache, NO_INSERT);
  gdb_assert (slot != NULL);

  htab_remove_elt (bfcache, cache);
}

/* btrace recording does not store previous memory content, neither the stack
   frames content.  Any unwinding would return errorneous results as the stack
   contents no longer matches the changed PC value restored from history.
   Therefore this unwinder reports any possibly unwound registers as
   <unavailable>.  */

const struct frame_unwind record_btrace_frame_unwind =
{
  NORMAL_FRAME,
  record_btrace_frame_unwind_stop_reason,
  record_btrace_frame_this_id,
  record_btrace_frame_prev_register,
  NULL,
  record_btrace_frame_sniffer,
  record_btrace_frame_dealloc_cache
};

const struct frame_unwind record_btrace_tailcall_frame_unwind =
{
  TAILCALL_FRAME,
  record_btrace_frame_unwind_stop_reason,
  record_btrace_frame_this_id,
  record_btrace_frame_prev_register,
  NULL,
  record_btrace_tailcall_frame_sniffer,
  record_btrace_frame_dealloc_cache
};

/* Implement the to_get_unwinder method.  */

static const struct frame_unwind *
record_btrace_to_get_unwinder (struct target_ops *self)
{
  return &record_btrace_frame_unwind;
}

/* Implement the to_get_tailcall_unwinder method.  */

static const struct frame_unwind *
record_btrace_to_get_tailcall_unwinder (struct target_ops *self)
{
  return &record_btrace_tailcall_frame_unwind;
}

/* Indicate that TP should be resumed according to FLAG.  */

static void
record_btrace_resume_thread (struct thread_info *tp,
			     enum btrace_thread_flag flag)
{
  struct btrace_thread_info *btinfo;

  DEBUG ("resuming %d (%s): %u", tp->num, target_pid_to_str (tp->ptid), flag);

  btinfo = &tp->btrace;

  if ((btinfo->flags & BTHR_MOVE) != 0)
    error (_("Thread already moving."));

  /* Fetch the latest branch trace.  */
  btrace_fetch (tp);

  btinfo->flags |= flag;
}

/* Find the thread to resume given a PTID.  */

static struct thread_info *
record_btrace_find_resume_thread (ptid_t ptid)
{
  struct thread_info *tp;

  /* When asked to resume everything, we pick the current thread.  */
  if (ptid_equal (minus_one_ptid, ptid) || ptid_is_pid (ptid))
    ptid = inferior_ptid;

  return find_thread_ptid (ptid);
}

/* Start replaying a thread.  */

static struct btrace_insn_iterator *
record_btrace_start_replaying (struct thread_info *tp)
{
  volatile struct gdb_exception except;
  struct btrace_insn_iterator *replay;
  struct btrace_thread_info *btinfo;
  int executing;

  btinfo = &tp->btrace;
  replay = NULL;

  /* We can't start replaying without trace.  */
  if (btinfo->begin == NULL)
    return NULL;

  /* Clear the executing flag to allow changes to the current frame.
     We are not actually running, yet.  We just started a reverse execution
     command or a record goto command.
     For the latter, EXECUTING is false and this has no effect.
     For the former, EXECUTING is true and we're in to_wait, about to
     move the thread.  Since we need to recompute the stack, we temporarily
     set EXECUTING to flase.  */
  executing = is_executing (tp->ptid);
  set_executing (tp->ptid, 0);

  /* GDB stores the current frame_id when stepping in order to detects steps
     into subroutines.
     Since frames are computed differently when we're replaying, we need to
     recompute those stored frames and fix them up so we can still detect
     subroutines after we started replaying.  */
  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct frame_info *frame;
      struct frame_id frame_id;
      int upd_step_frame_id, upd_step_stack_frame_id;

      /* The current frame without replaying - computed via normal unwind.  */
      frame = get_current_frame ();
      frame_id = get_frame_id (frame);

      /* Check if we need to update any stepping-related frame id's.  */
      upd_step_frame_id = frame_id_eq (frame_id,
				       tp->control.step_frame_id);
      upd_step_stack_frame_id = frame_id_eq (frame_id,
					     tp->control.step_stack_frame_id);

      /* We start replaying at the end of the branch trace.  This corresponds
	 to the current instruction.  */
      replay = xmalloc (sizeof (*replay));
      btrace_insn_end (replay, btinfo);

      /* We're not replaying, yet.  */
      gdb_assert (btinfo->replay == NULL);
      btinfo->replay = replay;

      /* Make sure we're not using any stale registers.  */
      registers_changed_ptid (tp->ptid);

      /* The current frame with replaying - computed via btrace unwind.  */
      frame = get_current_frame ();
      frame_id = get_frame_id (frame);

      /* Replace stepping related frames where necessary.  */
      if (upd_step_frame_id)
	tp->control.step_frame_id = frame_id;
      if (upd_step_stack_frame_id)
	tp->control.step_stack_frame_id = frame_id;
    }

  /* Restore the previous execution state.  */
  set_executing (tp->ptid, executing);

  if (except.reason < 0)
    {
      xfree (btinfo->replay);
      btinfo->replay = NULL;

      registers_changed_ptid (tp->ptid);

      throw_exception (except);
    }

  return replay;
}

/* Stop replaying a thread.  */

static void
record_btrace_stop_replaying (struct thread_info *tp)
{
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;

  xfree (btinfo->replay);
  btinfo->replay = NULL;

  /* Make sure we're not leaving any stale registers.  */
  registers_changed_ptid (tp->ptid);
}

/* The to_resume method of target record-btrace.  */

static void
record_btrace_resume (struct target_ops *ops, ptid_t ptid, int step,
		      enum gdb_signal signal)
{
  struct thread_info *tp, *other;
  enum btrace_thread_flag flag;

  DEBUG ("resume %s: %s", target_pid_to_str (ptid), step ? "step" : "cont");

  /* Store the execution direction of the last resume.  */
  record_btrace_resume_exec_dir = execution_direction;

  tp = record_btrace_find_resume_thread (ptid);
  if (tp == NULL)
    error (_("Cannot find thread to resume."));

  /* Stop replaying other threads if the thread to resume is not replaying.  */
  if (!btrace_is_replaying (tp) && execution_direction != EXEC_REVERSE)
    ALL_NON_EXITED_THREADS (other)
      record_btrace_stop_replaying (other);

  /* As long as we're not replaying, just forward the request.  */
  if (!record_btrace_is_replaying (ops) && execution_direction != EXEC_REVERSE)
    {
      ops = ops->beneath;
      return ops->to_resume (ops, ptid, step, signal);
    }

  /* Compute the btrace thread flag for the requested move.  */
  if (step == 0)
    flag = execution_direction == EXEC_REVERSE ? BTHR_RCONT : BTHR_CONT;
  else
    flag = execution_direction == EXEC_REVERSE ? BTHR_RSTEP : BTHR_STEP;

  /* At the moment, we only move a single thread.  We could also move
     all threads in parallel by single-stepping each resumed thread
     until the first runs into an event.
     When we do that, we would want to continue all other threads.
     For now, just resume one thread to not confuse to_wait.  */
  record_btrace_resume_thread (tp, flag);

  /* We just indicate the resume intent here.  The actual stepping happens in
     record_btrace_wait below.  */

  /* Async support.  */
  if (target_can_async_p ())
    {
      target_async (inferior_event_handler, 0);
      mark_async_event_handler (record_btrace_async_inferior_event_handler);
    }
}

/* Find a thread to move.  */

static struct thread_info *
record_btrace_find_thread_to_move (ptid_t ptid)
{
  struct thread_info *tp;

  /* First check the parameter thread.  */
  tp = find_thread_ptid (ptid);
  if (tp != NULL && (tp->btrace.flags & BTHR_MOVE) != 0)
    return tp;

  /* Otherwise, find one other thread that has been resumed.  */
  ALL_NON_EXITED_THREADS (tp)
    if ((tp->btrace.flags & BTHR_MOVE) != 0)
      return tp;

  return NULL;
}

/* Return a target_waitstatus indicating that we ran out of history.  */

static struct target_waitstatus
btrace_step_no_history (void)
{
  struct target_waitstatus status;

  status.kind = TARGET_WAITKIND_NO_HISTORY;

  return status;
}

/* Return a target_waitstatus indicating that a step finished.  */

static struct target_waitstatus
btrace_step_stopped (void)
{
  struct target_waitstatus status;

  status.kind = TARGET_WAITKIND_STOPPED;
  status.value.sig = GDB_SIGNAL_TRAP;

  return status;
}

/* Clear the record histories.  */

static void
record_btrace_clear_histories (struct btrace_thread_info *btinfo)
{
  xfree (btinfo->insn_history);
  xfree (btinfo->call_history);

  btinfo->insn_history = NULL;
  btinfo->call_history = NULL;
}

/* Step a single thread.  */

static struct target_waitstatus
record_btrace_step_thread (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay, end;
  struct btrace_thread_info *btinfo;
  struct address_space *aspace;
  struct inferior *inf;
  enum btrace_thread_flag flags;
  unsigned int steps;

  /* We can't step without an execution history.  */
  if (btrace_is_empty (tp))
    return btrace_step_no_history ();

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  flags = btinfo->flags & BTHR_MOVE;
  btinfo->flags &= ~BTHR_MOVE;

  DEBUG ("stepping %d (%s): %u", tp->num, target_pid_to_str (tp->ptid), flags);

  switch (flags)
    {
    default:
      internal_error (__FILE__, __LINE__, _("invalid stepping type."));

    case BTHR_STEP:
      /* We're done if we're not replaying.  */
      if (replay == NULL)
	return btrace_step_no_history ();

      /* We are always able to step at least once.  */
      steps = btrace_insn_next (replay, 1);
      gdb_assert (steps == 1);

      /* Determine the end of the instruction trace.  */
      btrace_insn_end (&end, btinfo);

      /* We stop replaying if we reached the end of the trace.  */
      if (btrace_insn_cmp (replay, &end) == 0)
	record_btrace_stop_replaying (tp);

      return btrace_step_stopped ();

    case BTHR_RSTEP:
      /* Start replaying if we're not already doing so.  */
      if (replay == NULL)
	replay = record_btrace_start_replaying (tp);

      /* If we can't step any further, we reached the end of the history.  */
      steps = btrace_insn_prev (replay, 1);
      if (steps == 0)
	return btrace_step_no_history ();

      return btrace_step_stopped ();

    case BTHR_CONT:
      /* We're done if we're not replaying.  */
      if (replay == NULL)
	return btrace_step_no_history ();

      inf = find_inferior_ptid (tp->ptid);
      aspace = inf->aspace;

      /* Determine the end of the instruction trace.  */
      btrace_insn_end (&end, btinfo);

      for (;;)
	{
	  const struct btrace_insn *insn;

	  /* We are always able to step at least once.  */
	  steps = btrace_insn_next (replay, 1);
	  gdb_assert (steps == 1);

	  /* We stop replaying if we reached the end of the trace.  */
	  if (btrace_insn_cmp (replay, &end) == 0)
	    {
	      record_btrace_stop_replaying (tp);
	      return btrace_step_no_history ();
	    }

	  insn = btrace_insn_get (replay);
	  gdb_assert (insn);

	  DEBUG ("stepping %d (%s) ... %s", tp->num,
		 target_pid_to_str (tp->ptid),
		 core_addr_to_string_nz (insn->pc));

	  if (breakpoint_here_p (aspace, insn->pc))
	    return btrace_step_stopped ();
	}

    case BTHR_RCONT:
      /* Start replaying if we're not already doing so.  */
      if (replay == NULL)
	replay = record_btrace_start_replaying (tp);

      inf = find_inferior_ptid (tp->ptid);
      aspace = inf->aspace;

      for (;;)
	{
	  const struct btrace_insn *insn;

	  /* If we can't step any further, we're done.  */
	  steps = btrace_insn_prev (replay, 1);
	  if (steps == 0)
	    return btrace_step_no_history ();

	  insn = btrace_insn_get (replay);
	  gdb_assert (insn);

	  DEBUG ("reverse-stepping %d (%s) ... %s", tp->num,
		 target_pid_to_str (tp->ptid),
		 core_addr_to_string_nz (insn->pc));

	  if (breakpoint_here_p (aspace, insn->pc))
	    return btrace_step_stopped ();
	}
    }
}

/* The to_wait method of target record-btrace.  */

static ptid_t
record_btrace_wait (struct target_ops *ops, ptid_t ptid,
		    struct target_waitstatus *status, int options)
{
  struct thread_info *tp, *other;

  DEBUG ("wait %s (0x%x)", target_pid_to_str (ptid), options);

  /* As long as we're not replaying, just forward the request.  */
  if (!record_btrace_is_replaying (ops) && execution_direction != EXEC_REVERSE)
    {
      ops = ops->beneath;
      return ops->to_wait (ops, ptid, status, options);
    }

  /* Let's find a thread to move.  */
  tp = record_btrace_find_thread_to_move (ptid);
  if (tp == NULL)
    {
      DEBUG ("wait %s: no thread", target_pid_to_str (ptid));

      status->kind = TARGET_WAITKIND_IGNORE;
      return minus_one_ptid;
    }

  /* We only move a single thread.  We're not able to correlate threads.  */
  *status = record_btrace_step_thread (tp);

  /* Stop all other threads. */
  if (!non_stop)
    ALL_NON_EXITED_THREADS (other)
      other->btrace.flags &= ~BTHR_MOVE;

  /* Start record histories anew from the current position.  */
  record_btrace_clear_histories (&tp->btrace);

  /* We moved the replay position but did not update registers.  */
  registers_changed_ptid (tp->ptid);

  return tp->ptid;
}

/* The to_can_execute_reverse method of target record-btrace.  */

static int
record_btrace_can_execute_reverse (struct target_ops *self)
{
  return 1;
}

/* The to_decr_pc_after_break method of target record-btrace.  */

static CORE_ADDR
record_btrace_decr_pc_after_break (struct target_ops *ops,
				   struct gdbarch *gdbarch)
{
  /* When replaying, we do not actually execute the breakpoint instruction
     so there is no need to adjust the PC after hitting a breakpoint.  */
  if (record_btrace_is_replaying (ops))
    return 0;

  return ops->beneath->to_decr_pc_after_break (ops->beneath, gdbarch);
}

/* The to_update_thread_list method of target record-btrace.  */

static void
record_btrace_update_thread_list (struct target_ops *ops)
{
  /* We don't add or remove threads during replay.  */
  if (record_btrace_is_replaying (ops))
    return;

  /* Forward the request.  */
  ops = ops->beneath;
  ops->to_update_thread_list (ops);
}

/* The to_thread_alive method of target record-btrace.  */

static int
record_btrace_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  /* We don't add or remove threads during replay.  */
  if (record_btrace_is_replaying (ops))
    return find_thread_ptid (ptid) != NULL;

  /* Forward the request.  */
  ops = ops->beneath;
  return ops->to_thread_alive (ops, ptid);
}

/* Set the replay branch trace instruction iterator.  If IT is NULL, replay
   is stopped.  */

static void
record_btrace_set_replay (struct thread_info *tp,
			  const struct btrace_insn_iterator *it)
{
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;

  if (it == NULL || it->function == NULL)
    record_btrace_stop_replaying (tp);
  else
    {
      if (btinfo->replay == NULL)
	record_btrace_start_replaying (tp);
      else if (btrace_insn_cmp (btinfo->replay, it) == 0)
	return;

      *btinfo->replay = *it;
      registers_changed_ptid (tp->ptid);
    }

  /* Start anew from the new replay position.  */
  record_btrace_clear_histories (btinfo);
}

/* The to_goto_record_begin method of target record-btrace.  */

static void
record_btrace_goto_begin (struct target_ops *self)
{
  struct thread_info *tp;
  struct btrace_insn_iterator begin;

  tp = require_btrace_thread ();

  btrace_insn_begin (&begin, &tp->btrace);
  record_btrace_set_replay (tp, &begin);

  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* The to_goto_record_end method of target record-btrace.  */

static void
record_btrace_goto_end (struct target_ops *ops)
{
  struct thread_info *tp;

  tp = require_btrace_thread ();

  record_btrace_set_replay (tp, NULL);

  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* The to_goto_record method of target record-btrace.  */

static void
record_btrace_goto (struct target_ops *self, ULONGEST insn)
{
  struct thread_info *tp;
  struct btrace_insn_iterator it;
  unsigned int number;
  int found;

  number = insn;

  /* Check for wrap-arounds.  */
  if (number != insn)
    error (_("Instruction number out of range."));

  tp = require_btrace_thread ();

  found = btrace_find_insn_by_number (&it, &tp->btrace, number);
  if (found == 0)
    error (_("No such instruction."));

  record_btrace_set_replay (tp, &it);

  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* The to_execution_direction target method.  */

static enum exec_direction_kind
record_btrace_execution_direction (struct target_ops *self)
{
  return record_btrace_resume_exec_dir;
}

/* The to_prepare_to_generate_core target method.  */

static void
record_btrace_prepare_to_generate_core (struct target_ops *self)
{
  record_btrace_generating_corefile = 1;
}

/* The to_done_generating_core target method.  */

static void
record_btrace_done_generating_core (struct target_ops *self)
{
  record_btrace_generating_corefile = 0;
}

/* Initialize the record-btrace target ops.  */

static void
init_record_btrace_ops (void)
{
  struct target_ops *ops;

  ops = &record_btrace_ops;
  ops->to_shortname = "record-btrace";
  ops->to_longname = "Branch tracing target";
  ops->to_doc = "Collect control-flow trace and provide the execution history.";
  ops->to_open = record_btrace_open;
  ops->to_close = record_btrace_close;
  ops->to_async = record_btrace_async;
  ops->to_detach = record_detach;
  ops->to_disconnect = record_disconnect;
  ops->to_mourn_inferior = record_mourn_inferior;
  ops->to_kill = record_kill;
  ops->to_stop_recording = record_btrace_stop_recording;
  ops->to_info_record = record_btrace_info;
  ops->to_insn_history = record_btrace_insn_history;
  ops->to_insn_history_from = record_btrace_insn_history_from;
  ops->to_insn_history_range = record_btrace_insn_history_range;
  ops->to_call_history = record_btrace_call_history;
  ops->to_call_history_from = record_btrace_call_history_from;
  ops->to_call_history_range = record_btrace_call_history_range;
  ops->to_record_is_replaying = record_btrace_is_replaying;
  ops->to_xfer_partial = record_btrace_xfer_partial;
  ops->to_remove_breakpoint = record_btrace_remove_breakpoint;
  ops->to_insert_breakpoint = record_btrace_insert_breakpoint;
  ops->to_fetch_registers = record_btrace_fetch_registers;
  ops->to_store_registers = record_btrace_store_registers;
  ops->to_prepare_to_store = record_btrace_prepare_to_store;
  ops->to_get_unwinder = &record_btrace_to_get_unwinder;
  ops->to_get_tailcall_unwinder = &record_btrace_to_get_tailcall_unwinder;
  ops->to_resume = record_btrace_resume;
  ops->to_wait = record_btrace_wait;
  ops->to_update_thread_list = record_btrace_update_thread_list;
  ops->to_thread_alive = record_btrace_thread_alive;
  ops->to_goto_record_begin = record_btrace_goto_begin;
  ops->to_goto_record_end = record_btrace_goto_end;
  ops->to_goto_record = record_btrace_goto;
  ops->to_can_execute_reverse = record_btrace_can_execute_reverse;
  ops->to_decr_pc_after_break = record_btrace_decr_pc_after_break;
  ops->to_execution_direction = record_btrace_execution_direction;
  ops->to_prepare_to_generate_core = record_btrace_prepare_to_generate_core;
  ops->to_done_generating_core = record_btrace_done_generating_core;
  ops->to_stratum = record_stratum;
  ops->to_magic = OPS_MAGIC;
}

/* Alias for "target record".  */

static void
cmd_record_btrace_start (char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  execute_command ("target record-btrace", from_tty);
}

/* The "set record btrace" command.  */

static void
cmd_set_record_btrace (char *args, int from_tty)
{
  cmd_show_list (set_record_btrace_cmdlist, from_tty, "");
}

/* The "show record btrace" command.  */

static void
cmd_show_record_btrace (char *args, int from_tty)
{
  cmd_show_list (show_record_btrace_cmdlist, from_tty, "");
}

/* The "show record btrace replay-memory-access" command.  */

static void
cmd_show_replay_memory_access (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (gdb_stdout, _("Replay memory access is %s.\n"),
		    replay_memory_access);
}

void _initialize_record_btrace (void);

/* Initialize btrace commands.  */

void
_initialize_record_btrace (void)
{
  add_cmd ("btrace", class_obscure, cmd_record_btrace_start,
	   _("Start branch trace recording."),
	   &record_cmdlist);
  add_alias_cmd ("b", "btrace", class_obscure, 1, &record_cmdlist);

  add_prefix_cmd ("btrace", class_support, cmd_set_record_btrace,
		  _("Set record options"), &set_record_btrace_cmdlist,
		  "set record btrace ", 0, &set_record_cmdlist);

  add_prefix_cmd ("btrace", class_support, cmd_show_record_btrace,
		  _("Show record options"), &show_record_btrace_cmdlist,
		  "show record btrace ", 0, &show_record_cmdlist);

  add_setshow_enum_cmd ("replay-memory-access", no_class,
			replay_memory_access_types, &replay_memory_access, _("\
Set what memory accesses are allowed during replay."), _("\
Show what memory accesses are allowed during replay."),
			   _("Default is READ-ONLY.\n\n\
The btrace record target does not trace data.\n\
The memory therefore corresponds to the live target and not \
to the current replay position.\n\n\
When READ-ONLY, allow accesses to read-only memory during replay.\n\
When READ-WRITE, allow accesses to read-only and read-write memory during \
replay."),
			   NULL, cmd_show_replay_memory_access,
			   &set_record_btrace_cmdlist,
			   &show_record_btrace_cmdlist);

  init_record_btrace_ops ();
  add_target (&record_btrace_ops);

  bfcache = htab_create_alloc (50, bfcache_hash, bfcache_eq, NULL,
			       xcalloc, xfree);
}
