/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2017 Free Software Foundation, Inc.

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
#include "record-btrace.h"
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
#include "vec.h"
#include <algorithm>

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

/* The current branch trace configuration.  */
static struct btrace_config record_btrace_conf;

/* Command list for "record btrace".  */
static struct cmd_list_element *record_btrace_cmdlist;

/* Command lists for "set/show record btrace bts".  */
static struct cmd_list_element *set_record_btrace_bts_cmdlist;
static struct cmd_list_element *show_record_btrace_bts_cmdlist;

/* Command lists for "set/show record btrace pt".  */
static struct cmd_list_element *set_record_btrace_pt_cmdlist;
static struct cmd_list_element *show_record_btrace_pt_cmdlist;

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

  validate_registers_access ();

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
  TRY
    {
      btrace_enable (tp, &record_btrace_conf);
    }
  CATCH (error, RETURN_MASK_ERROR)
    {
      warning ("%s", error.message);
    }
  END_CATCH
}

/* Callback function to disable branch tracing for one thread.  */

static void
record_btrace_disable_callback (void *arg)
{
  struct thread_info *tp = (struct thread_info *) arg;

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

/* See record-btrace.h.  */

void
record_btrace_push_target (void)
{
  const char *format;

  record_btrace_auto_enable ();

  push_target (&record_btrace_ops);

  record_btrace_async_inferior_event_handler
    = create_async_event_handler (record_btrace_handle_async_inferior_event,
				  NULL);
  record_btrace_generating_corefile = 0;

  format = btrace_format_short_string (record_btrace_conf.format);
  observer_notify_record_changed (current_inferior (), 1, "btrace", format);
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

  gdb_assert (record_btrace_thread_observer == NULL);

  disable_chain = make_cleanup (null_cleanup, NULL);
  ALL_NON_EXITED_THREADS (tp)
    if (args == NULL || *args == 0 || number_is_in_list (args, tp->global_num))
      {
	btrace_enable (tp, &record_btrace_conf);

	make_cleanup (record_btrace_disable_callback, tp);
      }

  record_btrace_push_target ();

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

/* The to_disconnect method of target record-btrace.  */

static void
record_btrace_disconnect (struct target_ops *self, const char *args,
			  int from_tty)
{
  struct target_ops *beneath = self->beneath;

  /* Do not stop recording, just clean up GDB side.  */
  unpush_target (self);

  /* Forward disconnect.  */
  beneath->to_disconnect (beneath, args, from_tty);
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
record_btrace_async (struct target_ops *ops, int enable)
{
  if (enable)
    mark_async_event_handler (record_btrace_async_inferior_event_handler);
  else
    clear_async_event_handler (record_btrace_async_inferior_event_handler);

  ops->beneath->to_async (ops->beneath, enable);
}

/* Adjusts the size and returns a human readable size suffix.  */

static const char *
record_btrace_adjust_size (unsigned int *size)
{
  unsigned int sz;

  sz = *size;

  if ((sz & ((1u << 30) - 1)) == 0)
    {
      *size = sz >> 30;
      return "GB";
    }
  else if ((sz & ((1u << 20) - 1)) == 0)
    {
      *size = sz >> 20;
      return "MB";
    }
  else if ((sz & ((1u << 10) - 1)) == 0)
    {
      *size = sz >> 10;
      return "kB";
    }
  else
    return "";
}

/* Print a BTS configuration.  */

static void
record_btrace_print_bts_conf (const struct btrace_config_bts *conf)
{
  const char *suffix;
  unsigned int size;

  size = conf->size;
  if (size > 0)
    {
      suffix = record_btrace_adjust_size (&size);
      printf_unfiltered (_("Buffer size: %u%s.\n"), size, suffix);
    }
}

/* Print an Intel Processor Trace configuration.  */

static void
record_btrace_print_pt_conf (const struct btrace_config_pt *conf)
{
  const char *suffix;
  unsigned int size;

  size = conf->size;
  if (size > 0)
    {
      suffix = record_btrace_adjust_size (&size);
      printf_unfiltered (_("Buffer size: %u%s.\n"), size, suffix);
    }
}

/* Print a branch tracing configuration.  */

static void
record_btrace_print_conf (const struct btrace_config *conf)
{
  printf_unfiltered (_("Recording format: %s.\n"),
		     btrace_format_string (conf->format));

  switch (conf->format)
    {
    case BTRACE_FORMAT_NONE:
      return;

    case BTRACE_FORMAT_BTS:
      record_btrace_print_bts_conf (&conf->bts);
      return;

    case BTRACE_FORMAT_PT:
      record_btrace_print_pt_conf (&conf->pt);
      return;
    }

  internal_error (__FILE__, __LINE__, _("Unkown branch trace format."));
}

/* The to_info_record method of target record-btrace.  */

static void
record_btrace_info (struct target_ops *self)
{
  struct btrace_thread_info *btinfo;
  const struct btrace_config *conf;
  struct thread_info *tp;
  unsigned int insns, calls, gaps;

  DEBUG ("info");

  tp = find_thread_ptid (inferior_ptid);
  if (tp == NULL)
    error (_("No thread."));

  validate_registers_access ();

  btinfo = &tp->btrace;

  conf = btrace_conf (btinfo);
  if (conf != NULL)
    record_btrace_print_conf (conf);

  btrace_fetch (tp);

  insns = 0;
  calls = 0;
  gaps = 0;

  if (!btrace_is_empty (tp))
    {
      struct btrace_call_iterator call;
      struct btrace_insn_iterator insn;

      btrace_call_end (&call, btinfo);
      btrace_call_prev (&call, 1);
      calls = btrace_call_number (&call);

      btrace_insn_end (&insn, btinfo);
      insns = btrace_insn_number (&insn);

      /* If the last instruction is not a gap, it is the current instruction
	 that is not actually part of the record.  */
      if (btrace_insn_get (&insn) != NULL)
	insns -= 1;

      gaps = btinfo->ngaps;
    }

  printf_unfiltered (_("Recorded %u instructions in %u functions (%u gaps) "
		       "for thread %s (%s).\n"), insns, calls, gaps,
		     print_thread_id (tp), target_pid_to_str (tp->ptid));

  if (btrace_is_replaying (tp))
    printf_unfiltered (_("Replay in progress.  At instruction %u.\n"),
		       btrace_insn_number (btinfo->replay));
}

/* Print a decode error.  */

static void
btrace_ui_out_decode_error (struct ui_out *uiout, int errcode,
			    enum btrace_format format)
{
  const char *errstr = btrace_decode_error (format, errcode);

  uiout->text (_("["));
  /* ERRCODE > 0 indicates notifications on BTRACE_FORMAT_PT.  */
  if (!(format == BTRACE_FORMAT_PT && errcode > 0))
    {
      uiout->text (_("decode error ("));
      uiout->field_int ("errcode", errcode);
      uiout->text (_("): "));
    }
  uiout->text (errstr);
  uiout->text (_("]\n"));
}

/* Print an unsigned int.  */

static void
ui_out_field_uint (struct ui_out *uiout, const char *fld, unsigned int val)
{
  uiout->field_fmt (fld, "%u", val);
}

/* A range of source lines.  */

struct btrace_line_range
{
  /* The symtab this line is from.  */
  struct symtab *symtab;

  /* The first line (inclusive).  */
  int begin;

  /* The last line (exclusive).  */
  int end;
};

/* Construct a line range.  */

static struct btrace_line_range
btrace_mk_line_range (struct symtab *symtab, int begin, int end)
{
  struct btrace_line_range range;

  range.symtab = symtab;
  range.begin = begin;
  range.end = end;

  return range;
}

/* Add a line to a line range.  */

static struct btrace_line_range
btrace_line_range_add (struct btrace_line_range range, int line)
{
  if (range.end <= range.begin)
    {
      /* This is the first entry.  */
      range.begin = line;
      range.end = line + 1;
    }
  else if (line < range.begin)
    range.begin = line;
  else if (range.end < line)
    range.end = line;

  return range;
}

/* Return non-zero if RANGE is empty, zero otherwise.  */

static int
btrace_line_range_is_empty (struct btrace_line_range range)
{
  return range.end <= range.begin;
}

/* Return non-zero if LHS contains RHS, zero otherwise.  */

static int
btrace_line_range_contains_range (struct btrace_line_range lhs,
				  struct btrace_line_range rhs)
{
  return ((lhs.symtab == rhs.symtab)
	  && (lhs.begin <= rhs.begin)
	  && (rhs.end <= lhs.end));
}

/* Find the line range associated with PC.  */

static struct btrace_line_range
btrace_find_line_range (CORE_ADDR pc)
{
  struct btrace_line_range range;
  struct linetable_entry *lines;
  struct linetable *ltable;
  struct symtab *symtab;
  int nlines, i;

  symtab = find_pc_line_symtab (pc);
  if (symtab == NULL)
    return btrace_mk_line_range (NULL, 0, 0);

  ltable = SYMTAB_LINETABLE (symtab);
  if (ltable == NULL)
    return btrace_mk_line_range (symtab, 0, 0);

  nlines = ltable->nitems;
  lines = ltable->item;
  if (nlines <= 0)
    return btrace_mk_line_range (symtab, 0, 0);

  range = btrace_mk_line_range (symtab, 0, 0);
  for (i = 0; i < nlines - 1; i++)
    {
      if ((lines[i].pc == pc) && (lines[i].line != 0))
	range = btrace_line_range_add (range, lines[i].line);
    }

  return range;
}

/* Print source lines in LINES to UIOUT.

   UI_ITEM_CHAIN is a cleanup chain for the last source line and the
   instructions corresponding to that source line.  When printing a new source
   line, we do the cleanups for the open chain and open a new cleanup chain for
   the new source line.  If the source line range in LINES is not empty, this
   function will leave the cleanup chain for the last printed source line open
   so instructions can be added to it.  */

static void
btrace_print_lines (struct btrace_line_range lines, struct ui_out *uiout,
		    struct cleanup **ui_item_chain, int flags)
{
  print_source_lines_flags psl_flags;
  int line;

  psl_flags = 0;
  if (flags & DISASSEMBLY_FILENAME)
    psl_flags |= PRINT_SOURCE_LINES_FILENAME;

  for (line = lines.begin; line < lines.end; ++line)
    {
      if (*ui_item_chain != NULL)
	do_cleanups (*ui_item_chain);

      *ui_item_chain
	= make_cleanup_ui_out_tuple_begin_end (uiout, "src_and_asm_line");

      print_source_lines (lines.symtab, line, line + 1, psl_flags);

      make_cleanup_ui_out_list_begin_end (uiout, "line_asm_insn");
    }
}

/* Disassemble a section of the recorded instruction trace.  */

static void
btrace_insn_history (struct ui_out *uiout,
		     const struct btrace_thread_info *btinfo,
		     const struct btrace_insn_iterator *begin,
		     const struct btrace_insn_iterator *end, int flags)
{
  struct cleanup *cleanups, *ui_item_chain;
  struct gdbarch *gdbarch;
  struct btrace_insn_iterator it;
  struct btrace_line_range last_lines;

  DEBUG ("itrace (0x%x): [%u; %u)", flags, btrace_insn_number (begin),
	 btrace_insn_number (end));

  flags |= DISASSEMBLY_SPECULATIVE;

  gdbarch = target_gdbarch ();
  last_lines = btrace_mk_line_range (NULL, 0, 0);

  cleanups = make_cleanup_ui_out_list_begin_end (uiout, "asm_insns");

  /* UI_ITEM_CHAIN is a cleanup chain for the last source line and the
     instructions corresponding to that line.  */
  ui_item_chain = NULL;

  gdb_pretty_print_disassembler disasm (gdbarch);

  for (it = *begin; btrace_insn_cmp (&it, end) != 0; btrace_insn_next (&it, 1))
    {
      const struct btrace_insn *insn;

      insn = btrace_insn_get (&it);

      /* A NULL instruction indicates a gap in the trace.  */
      if (insn == NULL)
	{
	  const struct btrace_config *conf;

	  conf = btrace_conf (btinfo);

	  /* We have trace so we must have a configuration.  */
	  gdb_assert (conf != NULL);

	  uiout->field_fmt ("insn-number", "%u",
			    btrace_insn_number (&it));
	  uiout->text ("\t");

	  btrace_ui_out_decode_error (uiout, btrace_insn_get_error (&it),
				      conf->format);
	}
      else
	{
	  struct disasm_insn dinsn;

	  if ((flags & DISASSEMBLY_SOURCE) != 0)
	    {
	      struct btrace_line_range lines;

	      lines = btrace_find_line_range (insn->pc);
	      if (!btrace_line_range_is_empty (lines)
		  && !btrace_line_range_contains_range (last_lines, lines))
		{
		  btrace_print_lines (lines, uiout, &ui_item_chain, flags);
		  last_lines = lines;
		}
	      else if (ui_item_chain == NULL)
		{
		  ui_item_chain
		    = make_cleanup_ui_out_tuple_begin_end (uiout,
							   "src_and_asm_line");
		  /* No source information.  */
		  make_cleanup_ui_out_list_begin_end (uiout, "line_asm_insn");
		}

	      gdb_assert (ui_item_chain != NULL);
	    }

	  memset (&dinsn, 0, sizeof (dinsn));
	  dinsn.number = btrace_insn_number (&it);
	  dinsn.addr = insn->pc;

	  if ((insn->flags & BTRACE_INSN_FLAG_SPECULATIVE) != 0)
	    dinsn.is_speculative = 1;

	  disasm.pretty_print_insn (uiout, &dinsn, flags);
	}
    }

  do_cleanups (cleanups);
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
    btrace_insn_history (uiout, btinfo, &begin, &end, flags);
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

  btrace_insn_history (uiout, btinfo, &begin, &end, flags);
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
  uiout->text (",");
  ui_out_field_uint (uiout, "insn end", end);
}

/* Compute the lowest and highest source line for the instructions in BFUN
   and return them in PBEGIN and PEND.
   Ignore instructions that can't be mapped to BFUN, e.g. instructions that
   result from inlining or macro expansion.  */

static void
btrace_compute_src_line_range (const struct btrace_function *bfun,
			       int *pbegin, int *pend)
{
  struct btrace_insn *insn;
  struct symtab *symtab;
  struct symbol *sym;
  unsigned int idx;
  int begin, end;

  begin = INT_MAX;
  end = INT_MIN;

  sym = bfun->sym;
  if (sym == NULL)
    goto out;

  symtab = symbol_symtab (sym);

  for (idx = 0; VEC_iterate (btrace_insn_s, bfun->insn, idx, insn); ++idx)
    {
      struct symtab_and_line sal;

      sal = find_pc_line (insn->pc, 0);
      if (sal.symtab != symtab || sal.line == 0)
	continue;

      begin = std::min (begin, sal.line);
      end = std::max (end, sal.line);
    }

 out:
  *pbegin = begin;
  *pend = end;
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

  uiout->field_string ("file",
		       symtab_to_filename_for_display (symbol_symtab (sym)));

  btrace_compute_src_line_range (bfun, &begin, &end);
  if (end < begin)
    return;

  uiout->text (":");
  uiout->field_int ("min line", begin);

  if (end == begin)
    return;

  uiout->text (",");
  uiout->field_int ("max line", end);
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
		     int int_flags)
{
  struct btrace_call_iterator it;
  record_print_flags flags = (enum record_print_flag) int_flags;

  DEBUG ("ftrace (0x%x): [%u; %u)", int_flags, btrace_call_number (begin),
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
      uiout->text ("\t");

      /* Indicate gaps in the trace.  */
      if (bfun->errcode != 0)
	{
	  const struct btrace_config *conf;

	  conf = btrace_conf (btinfo);

	  /* We have trace so we must have a configuration.  */
	  gdb_assert (conf != NULL);

	  btrace_ui_out_decode_error (uiout, bfun->errcode, conf->format);

	  continue;
	}

      if ((flags & RECORD_PRINT_INDENT_CALLS) != 0)
	{
	  int level = bfun->level + btinfo->level, i;

	  for (i = 0; i < level; ++i)
	    uiout->text ("  ");
	}

      if (sym != NULL)
	uiout->field_string ("function", SYMBOL_PRINT_NAME (sym));
      else if (msym != NULL)
	uiout->field_string ("function", MSYMBOL_PRINT_NAME (msym));
      else if (!uiout->is_mi_like_p ())
	uiout->field_string ("function", "??");

      if ((flags & RECORD_PRINT_INSN_RANGE) != 0)
	{
	  uiout->text (_("\tinst "));
	  btrace_call_history_insn_range (uiout, bfun);
	}

      if ((flags & RECORD_PRINT_SRC_LINE) != 0)
	{
	  uiout->text (_("\tat "));
	  btrace_call_history_src_line (uiout, bfun);
	}

      uiout->text ("\n");
    }
}

/* The to_call_history method of target record-btrace.  */

static void
record_btrace_call_history (struct target_ops *self, int size, int int_flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_call_history *history;
  struct btrace_call_iterator begin, end;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int context, covered;
  record_print_flags flags = (enum record_print_flag) int_flags;

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

      DEBUG ("call-history (0x%x): %d", int_flags, size);

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

      DEBUG ("call-history (0x%x): %d, prev: [%u; %u)", int_flags, size,
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
				  ULONGEST from, ULONGEST to,
				  int int_flags)
{
  struct btrace_thread_info *btinfo;
  struct btrace_call_history *history;
  struct btrace_call_iterator begin, end;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int low, high;
  int found;
  record_print_flags flags = (enum record_print_flag) int_flags;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "func history");
  low = from;
  high = to;

  DEBUG ("call-history (0x%x): [%u; %u)", int_flags, low, high);

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
				 ULONGEST from, int size,
				 int int_flags)
{
  ULONGEST begin, end, context;
  record_print_flags flags = (enum record_print_flag) int_flags;

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

/* The to_record_method method of target record-btrace.  */

static enum record_method
record_btrace_record_method (struct target_ops *self, ptid_t ptid)
{
  const struct btrace_config *config;
  struct thread_info * const tp = find_thread_ptid (ptid);

  if (tp == NULL)
    error (_("No thread."));

  if (tp->btrace.target == NULL)
    return RECORD_METHOD_NONE;

  return RECORD_METHOD_BTRACE;
}

/* The to_record_is_replaying method of target record-btrace.  */

static int
record_btrace_is_replaying (struct target_ops *self, ptid_t ptid)
{
  struct thread_info *tp;

  ALL_NON_EXITED_THREADS (tp)
    if (ptid_match (tp->ptid, ptid) && btrace_is_replaying (tp))
      return 1;

  return 0;
}

/* The to_record_will_replay method of target record-btrace.  */

static int
record_btrace_will_replay (struct target_ops *self, ptid_t ptid, int dir)
{
  return dir == EXEC_REVERSE || record_btrace_is_replaying (self, ptid);
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
      && record_btrace_is_replaying (ops, inferior_ptid))
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
		    len = std::min (len, section->endaddr - offset);
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
  const char *old;
  int ret;

  /* Inserting breakpoints requires accessing memory.  Allow it for the
     duration of this function.  */
  old = replay_memory_access;
  replay_memory_access = replay_memory_access_read_write;

  ret = 0;
  TRY
    {
      ret = ops->beneath->to_insert_breakpoint (ops->beneath, gdbarch, bp_tgt);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      replay_memory_access = old;
      throw_exception (except);
    }
  END_CATCH
  replay_memory_access = old;

  return ret;
}

/* The to_remove_breakpoint method of target record-btrace.  */

static int
record_btrace_remove_breakpoint (struct target_ops *ops,
				 struct gdbarch *gdbarch,
				 struct bp_target_info *bp_tgt,
				 enum remove_bp_reason reason)
{
  const char *old;
  int ret;

  /* Removing breakpoints requires accessing memory.  Allow it for the
     duration of this function.  */
  old = replay_memory_access;
  replay_memory_access = replay_memory_access_read_write;

  ret = 0;
  TRY
    {
      ret = ops->beneath->to_remove_breakpoint (ops->beneath, gdbarch, bp_tgt,
						reason);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      replay_memory_access = old;
      throw_exception (except);
    }
  END_CATCH
  replay_memory_access = old;

  return ret;
}

/* The to_fetch_registers method of target record-btrace.  */

static void
record_btrace_fetch_registers (struct target_ops *ops,
			       struct regcache *regcache, int regno)
{
  struct btrace_insn_iterator *replay;
  struct thread_info *tp;

  tp = find_thread_ptid (regcache_get_ptid (regcache));
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

  if (!record_btrace_generating_corefile
      && record_btrace_is_replaying (ops, regcache_get_ptid (regcache)))
    error (_("Cannot write registers while replaying."));

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

  if (!record_btrace_generating_corefile
      && record_btrace_is_replaying (ops, regcache_get_ptid (regcache)))
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
  const struct btrace_frame_cache *cache
    = (const struct btrace_frame_cache *) arg;

  return htab_hash_pointer (cache->frame);
}

/* eq_f for htab_create_alloc of bfcache.  */

static int
bfcache_eq (const void *arg1, const void *arg2)
{
  const struct btrace_frame_cache *cache1
    = (const struct btrace_frame_cache *) arg1;
  const struct btrace_frame_cache *cache2
    = (const struct btrace_frame_cache *) arg2;

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

  cache = (const struct btrace_frame_cache *) *slot;
  return cache->bfun;
}

/* Implement stop_reason method for record_btrace_frame_unwind.  */

static enum unwind_stop_reason
record_btrace_frame_unwind_stop_reason (struct frame_info *this_frame,
					void **this_cache)
{
  const struct btrace_frame_cache *cache;
  const struct btrace_function *bfun;

  cache = (const struct btrace_frame_cache *) *this_cache;
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

  cache = (const struct btrace_frame_cache *) *this_cache;

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

  cache = (const struct btrace_frame_cache *) *this_cache;
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

  cache = (struct btrace_frame_cache *) this_cache;

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

/* Return a human-readable string for FLAG.  */

static const char *
btrace_thread_flag_to_str (enum btrace_thread_flag flag)
{
  switch (flag)
    {
    case BTHR_STEP:
      return "step";

    case BTHR_RSTEP:
      return "reverse-step";

    case BTHR_CONT:
      return "cont";

    case BTHR_RCONT:
      return "reverse-cont";

    case BTHR_STOP:
      return "stop";
    }

  return "<invalid>";
}

/* Indicate that TP should be resumed according to FLAG.  */

static void
record_btrace_resume_thread (struct thread_info *tp,
			     enum btrace_thread_flag flag)
{
  struct btrace_thread_info *btinfo;

  DEBUG ("resuming thread %s (%s): %x (%s)", print_thread_id (tp),
	 target_pid_to_str (tp->ptid), flag, btrace_thread_flag_to_str (flag));

  btinfo = &tp->btrace;

  /* Fetch the latest branch trace.  */
  btrace_fetch (tp);

  /* A resume request overwrites a preceding resume or stop request.  */
  btinfo->flags &= ~(BTHR_MOVE | BTHR_STOP);
  btinfo->flags |= flag;
}

/* Get the current frame for TP.  */

static struct frame_info *
get_thread_current_frame (struct thread_info *tp)
{
  struct frame_info *frame;
  ptid_t old_inferior_ptid;
  int executing;

  /* Set INFERIOR_PTID, which is implicitly used by get_current_frame.  */
  old_inferior_ptid = inferior_ptid;
  inferior_ptid = tp->ptid;

  /* Clear the executing flag to allow changes to the current frame.
     We are not actually running, yet.  We just started a reverse execution
     command or a record goto command.
     For the latter, EXECUTING is false and this has no effect.
     For the former, EXECUTING is true and we're in to_wait, about to
     move the thread.  Since we need to recompute the stack, we temporarily
     set EXECUTING to flase.  */
  executing = is_executing (inferior_ptid);
  set_executing (inferior_ptid, 0);

  frame = NULL;
  TRY
    {
      frame = get_current_frame ();
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      /* Restore the previous execution state.  */
      set_executing (inferior_ptid, executing);

      /* Restore the previous inferior_ptid.  */
      inferior_ptid = old_inferior_ptid;

      throw_exception (except);
    }
  END_CATCH

  /* Restore the previous execution state.  */
  set_executing (inferior_ptid, executing);

  /* Restore the previous inferior_ptid.  */
  inferior_ptid = old_inferior_ptid;

  return frame;
}

/* Start replaying a thread.  */

static struct btrace_insn_iterator *
record_btrace_start_replaying (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;
  replay = NULL;

  /* We can't start replaying without trace.  */
  if (btinfo->begin == NULL)
    return NULL;

  /* GDB stores the current frame_id when stepping in order to detects steps
     into subroutines.
     Since frames are computed differently when we're replaying, we need to
     recompute those stored frames and fix them up so we can still detect
     subroutines after we started replaying.  */
  TRY
    {
      struct frame_info *frame;
      struct frame_id frame_id;
      int upd_step_frame_id, upd_step_stack_frame_id;

      /* The current frame without replaying - computed via normal unwind.  */
      frame = get_thread_current_frame (tp);
      frame_id = get_frame_id (frame);

      /* Check if we need to update any stepping-related frame id's.  */
      upd_step_frame_id = frame_id_eq (frame_id,
				       tp->control.step_frame_id);
      upd_step_stack_frame_id = frame_id_eq (frame_id,
					     tp->control.step_stack_frame_id);

      /* We start replaying at the end of the branch trace.  This corresponds
	 to the current instruction.  */
      replay = XNEW (struct btrace_insn_iterator);
      btrace_insn_end (replay, btinfo);

      /* Skip gaps at the end of the trace.  */
      while (btrace_insn_get (replay) == NULL)
	{
	  unsigned int steps;

	  steps = btrace_insn_prev (replay, 1);
	  if (steps == 0)
	    error (_("No trace."));
	}

      /* We're not replaying, yet.  */
      gdb_assert (btinfo->replay == NULL);
      btinfo->replay = replay;

      /* Make sure we're not using any stale registers.  */
      registers_changed_ptid (tp->ptid);

      /* The current frame with replaying - computed via btrace unwind.  */
      frame = get_thread_current_frame (tp);
      frame_id = get_frame_id (frame);

      /* Replace stepping related frames where necessary.  */
      if (upd_step_frame_id)
	tp->control.step_frame_id = frame_id;
      if (upd_step_stack_frame_id)
	tp->control.step_stack_frame_id = frame_id;
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      xfree (btinfo->replay);
      btinfo->replay = NULL;

      registers_changed_ptid (tp->ptid);

      throw_exception (except);
    }
  END_CATCH

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

/* Stop replaying TP if it is at the end of its execution history.  */

static void
record_btrace_stop_replaying_at_end (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay, end;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  if (replay == NULL)
    return;

  btrace_insn_end (&end, btinfo);

  if (btrace_insn_cmp (replay, &end) == 0)
    record_btrace_stop_replaying (tp);
}

/* The to_resume method of target record-btrace.  */

static void
record_btrace_resume (struct target_ops *ops, ptid_t ptid, int step,
		      enum gdb_signal signal)
{
  struct thread_info *tp;
  enum btrace_thread_flag flag, cflag;

  DEBUG ("resume %s: %s%s", target_pid_to_str (ptid),
	 execution_direction == EXEC_REVERSE ? "reverse-" : "",
	 step ? "step" : "cont");

  /* Store the execution direction of the last resume.

     If there is more than one to_resume call, we have to rely on infrun
     to not change the execution direction in-between.  */
  record_btrace_resume_exec_dir = execution_direction;

  /* As long as we're not replaying, just forward the request.

     For non-stop targets this means that no thread is replaying.  In order to
     make progress, we may need to explicitly move replaying threads to the end
     of their execution history.  */
  if ((execution_direction != EXEC_REVERSE)
      && !record_btrace_is_replaying (ops, minus_one_ptid))
    {
      ops = ops->beneath;
      ops->to_resume (ops, ptid, step, signal);
      return;
    }

  /* Compute the btrace thread flag for the requested move.  */
  if (execution_direction == EXEC_REVERSE)
    {
      flag = step == 0 ? BTHR_RCONT : BTHR_RSTEP;
      cflag = BTHR_RCONT;
    }
  else
    {
      flag = step == 0 ? BTHR_CONT : BTHR_STEP;
      cflag = BTHR_CONT;
    }

  /* We just indicate the resume intent here.  The actual stepping happens in
     record_btrace_wait below.

     For all-stop targets, we only step INFERIOR_PTID and continue others.  */
  if (!target_is_non_stop_p ())
    {
      gdb_assert (ptid_match (inferior_ptid, ptid));

      ALL_NON_EXITED_THREADS (tp)
	if (ptid_match (tp->ptid, ptid))
	  {
	    if (ptid_match (tp->ptid, inferior_ptid))
	      record_btrace_resume_thread (tp, flag);
	    else
	      record_btrace_resume_thread (tp, cflag);
	  }
    }
  else
    {
      ALL_NON_EXITED_THREADS (tp)
	if (ptid_match (tp->ptid, ptid))
	  record_btrace_resume_thread (tp, flag);
    }

  /* Async support.  */
  if (target_can_async_p ())
    {
      target_async (1);
      mark_async_event_handler (record_btrace_async_inferior_event_handler);
    }
}

/* The to_commit_resume method of target record-btrace.  */

static void
record_btrace_commit_resume (struct target_ops *ops)
{
  if ((execution_direction != EXEC_REVERSE)
      && !record_btrace_is_replaying (ops, minus_one_ptid))
    ops->beneath->to_commit_resume (ops->beneath);
}

/* Cancel resuming TP.  */

static void
record_btrace_cancel_resume (struct thread_info *tp)
{
  enum btrace_thread_flag flags;

  flags = tp->btrace.flags & (BTHR_MOVE | BTHR_STOP);
  if (flags == 0)
    return;

  DEBUG ("cancel resume thread %s (%s): %x (%s)",
	 print_thread_id (tp),
	 target_pid_to_str (tp->ptid), flags,
	 btrace_thread_flag_to_str (flags));

  tp->btrace.flags &= ~(BTHR_MOVE | BTHR_STOP);
  record_btrace_stop_replaying_at_end (tp);
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

/* Return a target_waitstatus indicating that a thread was stopped as
   requested.  */

static struct target_waitstatus
btrace_step_stopped_on_request (void)
{
  struct target_waitstatus status;

  status.kind = TARGET_WAITKIND_STOPPED;
  status.value.sig = GDB_SIGNAL_0;

  return status;
}

/* Return a target_waitstatus indicating a spurious stop.  */

static struct target_waitstatus
btrace_step_spurious (void)
{
  struct target_waitstatus status;

  status.kind = TARGET_WAITKIND_SPURIOUS;

  return status;
}

/* Return a target_waitstatus indicating that the thread was not resumed.  */

static struct target_waitstatus
btrace_step_no_resumed (void)
{
  struct target_waitstatus status;

  status.kind = TARGET_WAITKIND_NO_RESUMED;

  return status;
}

/* Return a target_waitstatus indicating that we should wait again.  */

static struct target_waitstatus
btrace_step_again (void)
{
  struct target_waitstatus status;

  status.kind = TARGET_WAITKIND_IGNORE;

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

/* Check whether TP's current replay position is at a breakpoint.  */

static int
record_btrace_replay_at_breakpoint (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay;
  struct btrace_thread_info *btinfo;
  const struct btrace_insn *insn;
  struct inferior *inf;

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  if (replay == NULL)
    return 0;

  insn = btrace_insn_get (replay);
  if (insn == NULL)
    return 0;

  inf = find_inferior_ptid (tp->ptid);
  if (inf == NULL)
    return 0;

  return record_check_stopped_by_breakpoint (inf->aspace, insn->pc,
					     &btinfo->stop_reason);
}

/* Step one instruction in forward direction.  */

static struct target_waitstatus
record_btrace_single_step_forward (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay, end, start;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  /* We're done if we're not replaying.  */
  if (replay == NULL)
    return btrace_step_no_history ();

  /* Check if we're stepping a breakpoint.  */
  if (record_btrace_replay_at_breakpoint (tp))
    return btrace_step_stopped ();

  /* Skip gaps during replay.  If we end up at a gap (at the end of the trace),
     jump back to the instruction at which we started.  */
  start = *replay;
  do
    {
      unsigned int steps;

      /* We will bail out here if we continue stepping after reaching the end
	 of the execution history.  */
      steps = btrace_insn_next (replay, 1);
      if (steps == 0)
	{
	  *replay = start;
	  return btrace_step_no_history ();
	}
    }
  while (btrace_insn_get (replay) == NULL);

  /* Determine the end of the instruction trace.  */
  btrace_insn_end (&end, btinfo);

  /* The execution trace contains (and ends with) the current instruction.
     This instruction has not been executed, yet, so the trace really ends
     one instruction earlier.  */
  if (btrace_insn_cmp (replay, &end) == 0)
    return btrace_step_no_history ();

  return btrace_step_spurious ();
}

/* Step one instruction in backward direction.  */

static struct target_waitstatus
record_btrace_single_step_backward (struct thread_info *tp)
{
  struct btrace_insn_iterator *replay, start;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;
  replay = btinfo->replay;

  /* Start replaying if we're not already doing so.  */
  if (replay == NULL)
    replay = record_btrace_start_replaying (tp);

  /* If we can't step any further, we reached the end of the history.
     Skip gaps during replay.  If we end up at a gap (at the beginning of
     the trace), jump back to the instruction at which we started.  */
  start = *replay;
  do
    {
      unsigned int steps;

      steps = btrace_insn_prev (replay, 1);
      if (steps == 0)
	{
	  *replay = start;
	  return btrace_step_no_history ();
	}
    }
  while (btrace_insn_get (replay) == NULL);

  /* Check if we're stepping a breakpoint.

     For reverse-stepping, this check is after the step.  There is logic in
     infrun.c that handles reverse-stepping separately.  See, for example,
     proceed and adjust_pc_after_break.

     This code assumes that for reverse-stepping, PC points to the last
     de-executed instruction, whereas for forward-stepping PC points to the
     next to-be-executed instruction.  */
  if (record_btrace_replay_at_breakpoint (tp))
    return btrace_step_stopped ();

  return btrace_step_spurious ();
}

/* Step a single thread.  */

static struct target_waitstatus
record_btrace_step_thread (struct thread_info *tp)
{
  struct btrace_thread_info *btinfo;
  struct target_waitstatus status;
  enum btrace_thread_flag flags;

  btinfo = &tp->btrace;

  flags = btinfo->flags & (BTHR_MOVE | BTHR_STOP);
  btinfo->flags &= ~(BTHR_MOVE | BTHR_STOP);

  DEBUG ("stepping thread %s (%s): %x (%s)", print_thread_id (tp),
	 target_pid_to_str (tp->ptid), flags,
	 btrace_thread_flag_to_str (flags));

  /* We can't step without an execution history.  */
  if ((flags & BTHR_MOVE) != 0 && btrace_is_empty (tp))
    return btrace_step_no_history ();

  switch (flags)
    {
    default:
      internal_error (__FILE__, __LINE__, _("invalid stepping type."));

    case BTHR_STOP:
      return btrace_step_stopped_on_request ();

    case BTHR_STEP:
      status = record_btrace_single_step_forward (tp);
      if (status.kind != TARGET_WAITKIND_SPURIOUS)
	break;

      return btrace_step_stopped ();

    case BTHR_RSTEP:
      status = record_btrace_single_step_backward (tp);
      if (status.kind != TARGET_WAITKIND_SPURIOUS)
	break;

      return btrace_step_stopped ();

    case BTHR_CONT:
      status = record_btrace_single_step_forward (tp);
      if (status.kind != TARGET_WAITKIND_SPURIOUS)
	break;

      btinfo->flags |= flags;
      return btrace_step_again ();

    case BTHR_RCONT:
      status = record_btrace_single_step_backward (tp);
      if (status.kind != TARGET_WAITKIND_SPURIOUS)
	break;

      btinfo->flags |= flags;
      return btrace_step_again ();
    }

  /* We keep threads moving at the end of their execution history.  The to_wait
     method will stop the thread for whom the event is reported.  */
  if (status.kind == TARGET_WAITKIND_NO_HISTORY)
    btinfo->flags |= flags;

  return status;
}

/* A vector of threads.  */

typedef struct thread_info * tp_t;
DEF_VEC_P (tp_t);

/* Announce further events if necessary.  */

static void
record_btrace_maybe_mark_async_event (const VEC (tp_t) *moving,
				      const VEC (tp_t) *no_history)
{
  int more_moving, more_no_history;

  more_moving = !VEC_empty (tp_t, moving);
  more_no_history = !VEC_empty (tp_t, no_history);

  if (!more_moving && !more_no_history)
    return;

  if (more_moving)
    DEBUG ("movers pending");

  if (more_no_history)
    DEBUG ("no-history pending");

  mark_async_event_handler (record_btrace_async_inferior_event_handler);
}

/* The to_wait method of target record-btrace.  */

static ptid_t
record_btrace_wait (struct target_ops *ops, ptid_t ptid,
		    struct target_waitstatus *status, int options)
{
  VEC (tp_t) *moving, *no_history;
  struct thread_info *tp, *eventing;
  struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);

  DEBUG ("wait %s (0x%x)", target_pid_to_str (ptid), options);

  /* As long as we're not replaying, just forward the request.  */
  if ((execution_direction != EXEC_REVERSE)
      && !record_btrace_is_replaying (ops, minus_one_ptid))
    {
      ops = ops->beneath;
      return ops->to_wait (ops, ptid, status, options);
    }

  moving = NULL;
  no_history = NULL;

  make_cleanup (VEC_cleanup (tp_t), &moving);
  make_cleanup (VEC_cleanup (tp_t), &no_history);

  /* Keep a work list of moving threads.  */
  ALL_NON_EXITED_THREADS (tp)
    if (ptid_match (tp->ptid, ptid)
	&& ((tp->btrace.flags & (BTHR_MOVE | BTHR_STOP)) != 0))
      VEC_safe_push (tp_t, moving, tp);

  if (VEC_empty (tp_t, moving))
    {
      *status = btrace_step_no_resumed ();

      DEBUG ("wait ended by %s: %s", target_pid_to_str (null_ptid),
	     target_waitstatus_to_string (status));

      do_cleanups (cleanups);
      return null_ptid;
    }

  /* Step moving threads one by one, one step each, until either one thread
     reports an event or we run out of threads to step.

     When stepping more than one thread, chances are that some threads reach
     the end of their execution history earlier than others.  If we reported
     this immediately, all-stop on top of non-stop would stop all threads and
     resume the same threads next time.  And we would report the same thread
     having reached the end of its execution history again.

     In the worst case, this would starve the other threads.  But even if other
     threads would be allowed to make progress, this would result in far too
     many intermediate stops.

     We therefore delay the reporting of "no execution history" until we have
     nothing else to report.  By this time, all threads should have moved to
     either the beginning or the end of their execution history.  There will
     be a single user-visible stop.  */
  eventing = NULL;
  while ((eventing == NULL) && !VEC_empty (tp_t, moving))
    {
      unsigned int ix;

      ix = 0;
      while ((eventing == NULL) && VEC_iterate (tp_t, moving, ix, tp))
	{
	  *status = record_btrace_step_thread (tp);

	  switch (status->kind)
	    {
	    case TARGET_WAITKIND_IGNORE:
	      ix++;
	      break;

	    case TARGET_WAITKIND_NO_HISTORY:
	      VEC_safe_push (tp_t, no_history,
			     VEC_ordered_remove (tp_t, moving, ix));
	      break;

	    default:
	      eventing = VEC_unordered_remove (tp_t, moving, ix);
	      break;
	    }
	}
    }

  if (eventing == NULL)
    {
      /* We started with at least one moving thread.  This thread must have
	 either stopped or reached the end of its execution history.

	 In the former case, EVENTING must not be NULL.
	 In the latter case, NO_HISTORY must not be empty.  */
      gdb_assert (!VEC_empty (tp_t, no_history));

      /* We kept threads moving at the end of their execution history.  Stop
	 EVENTING now that we are going to report its stop.  */
      eventing = VEC_unordered_remove (tp_t, no_history, 0);
      eventing->btrace.flags &= ~BTHR_MOVE;

      *status = btrace_step_no_history ();
    }

  gdb_assert (eventing != NULL);

  /* We kept threads replaying at the end of their execution history.  Stop
     replaying EVENTING now that we are going to report its stop.  */
  record_btrace_stop_replaying_at_end (eventing);

  /* Stop all other threads. */
  if (!target_is_non_stop_p ())
    ALL_NON_EXITED_THREADS (tp)
      record_btrace_cancel_resume (tp);

  /* In async mode, we need to announce further events.  */
  if (target_is_async_p ())
    record_btrace_maybe_mark_async_event (moving, no_history);

  /* Start record histories anew from the current position.  */
  record_btrace_clear_histories (&eventing->btrace);

  /* We moved the replay position but did not update registers.  */
  registers_changed_ptid (eventing->ptid);

  DEBUG ("wait ended by thread %s (%s): %s",
	 print_thread_id (eventing),
	 target_pid_to_str (eventing->ptid),
	 target_waitstatus_to_string (status));

  do_cleanups (cleanups);
  return eventing->ptid;
}

/* The to_stop method of target record-btrace.  */

static void
record_btrace_stop (struct target_ops *ops, ptid_t ptid)
{
  DEBUG ("stop %s", target_pid_to_str (ptid));

  /* As long as we're not replaying, just forward the request.  */
  if ((execution_direction != EXEC_REVERSE)
      && !record_btrace_is_replaying (ops, minus_one_ptid))
    {
      ops = ops->beneath;
      ops->to_stop (ops, ptid);
    }
  else
    {
      struct thread_info *tp;

      ALL_NON_EXITED_THREADS (tp)
       if (ptid_match (tp->ptid, ptid))
         {
           tp->btrace.flags &= ~BTHR_MOVE;
           tp->btrace.flags |= BTHR_STOP;
         }
    }
 }

/* The to_can_execute_reverse method of target record-btrace.  */

static int
record_btrace_can_execute_reverse (struct target_ops *self)
{
  return 1;
}

/* The to_stopped_by_sw_breakpoint method of target record-btrace.  */

static int
record_btrace_stopped_by_sw_breakpoint (struct target_ops *ops)
{
  if (record_btrace_is_replaying (ops, minus_one_ptid))
    {
      struct thread_info *tp = inferior_thread ();

      return tp->btrace.stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT;
    }

  return ops->beneath->to_stopped_by_sw_breakpoint (ops->beneath);
}

/* The to_supports_stopped_by_sw_breakpoint method of target
   record-btrace.  */

static int
record_btrace_supports_stopped_by_sw_breakpoint (struct target_ops *ops)
{
  if (record_btrace_is_replaying (ops, minus_one_ptid))
    return 1;

  return ops->beneath->to_supports_stopped_by_sw_breakpoint (ops->beneath);
}

/* The to_stopped_by_sw_breakpoint method of target record-btrace.  */

static int
record_btrace_stopped_by_hw_breakpoint (struct target_ops *ops)
{
  if (record_btrace_is_replaying (ops, minus_one_ptid))
    {
      struct thread_info *tp = inferior_thread ();

      return tp->btrace.stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT;
    }

  return ops->beneath->to_stopped_by_hw_breakpoint (ops->beneath);
}

/* The to_supports_stopped_by_hw_breakpoint method of target
   record-btrace.  */

static int
record_btrace_supports_stopped_by_hw_breakpoint (struct target_ops *ops)
{
  if (record_btrace_is_replaying (ops, minus_one_ptid))
    return 1;

  return ops->beneath->to_supports_stopped_by_hw_breakpoint (ops->beneath);
}

/* The to_update_thread_list method of target record-btrace.  */

static void
record_btrace_update_thread_list (struct target_ops *ops)
{
  /* We don't add or remove threads during replay.  */
  if (record_btrace_is_replaying (ops, minus_one_ptid))
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
  if (record_btrace_is_replaying (ops, minus_one_ptid))
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

  stop_pc = regcache_read_pc (get_current_regcache ());
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* The to_goto_record_begin method of target record-btrace.  */

static void
record_btrace_goto_begin (struct target_ops *self)
{
  struct thread_info *tp;
  struct btrace_insn_iterator begin;

  tp = require_btrace_thread ();

  btrace_insn_begin (&begin, &tp->btrace);

  /* Skip gaps at the beginning of the trace.  */
  while (btrace_insn_get (&begin) == NULL)
    {
      unsigned int steps;

      steps = btrace_insn_next (&begin, 1);
      if (steps == 0)
	error (_("No trace."));
    }

  record_btrace_set_replay (tp, &begin);
}

/* The to_goto_record_end method of target record-btrace.  */

static void
record_btrace_goto_end (struct target_ops *ops)
{
  struct thread_info *tp;

  tp = require_btrace_thread ();

  record_btrace_set_replay (tp, NULL);
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

  /* Check if the instruction could not be found or is a gap.  */
  if (found == 0 || btrace_insn_get (&it) == NULL)
    error (_("No such instruction."));

  record_btrace_set_replay (tp, &it);
}

/* The to_record_stop_replaying method of target record-btrace.  */

static void
record_btrace_stop_replaying_all (struct target_ops *self)
{
  struct thread_info *tp;

  ALL_NON_EXITED_THREADS (tp)
    record_btrace_stop_replaying (tp);
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
  ops->to_disconnect = record_btrace_disconnect;
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
  ops->to_record_method = record_btrace_record_method;
  ops->to_record_is_replaying = record_btrace_is_replaying;
  ops->to_record_will_replay = record_btrace_will_replay;
  ops->to_record_stop_replaying = record_btrace_stop_replaying_all;
  ops->to_xfer_partial = record_btrace_xfer_partial;
  ops->to_remove_breakpoint = record_btrace_remove_breakpoint;
  ops->to_insert_breakpoint = record_btrace_insert_breakpoint;
  ops->to_fetch_registers = record_btrace_fetch_registers;
  ops->to_store_registers = record_btrace_store_registers;
  ops->to_prepare_to_store = record_btrace_prepare_to_store;
  ops->to_get_unwinder = &record_btrace_to_get_unwinder;
  ops->to_get_tailcall_unwinder = &record_btrace_to_get_tailcall_unwinder;
  ops->to_resume = record_btrace_resume;
  ops->to_commit_resume = record_btrace_commit_resume;
  ops->to_wait = record_btrace_wait;
  ops->to_stop = record_btrace_stop;
  ops->to_update_thread_list = record_btrace_update_thread_list;
  ops->to_thread_alive = record_btrace_thread_alive;
  ops->to_goto_record_begin = record_btrace_goto_begin;
  ops->to_goto_record_end = record_btrace_goto_end;
  ops->to_goto_record = record_btrace_goto;
  ops->to_can_execute_reverse = record_btrace_can_execute_reverse;
  ops->to_stopped_by_sw_breakpoint = record_btrace_stopped_by_sw_breakpoint;
  ops->to_supports_stopped_by_sw_breakpoint
    = record_btrace_supports_stopped_by_sw_breakpoint;
  ops->to_stopped_by_hw_breakpoint = record_btrace_stopped_by_hw_breakpoint;
  ops->to_supports_stopped_by_hw_breakpoint
    = record_btrace_supports_stopped_by_hw_breakpoint;
  ops->to_execution_direction = record_btrace_execution_direction;
  ops->to_prepare_to_generate_core = record_btrace_prepare_to_generate_core;
  ops->to_done_generating_core = record_btrace_done_generating_core;
  ops->to_stratum = record_stratum;
  ops->to_magic = OPS_MAGIC;
}

/* Start recording in BTS format.  */

static void
cmd_record_btrace_bts_start (char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  record_btrace_conf.format = BTRACE_FORMAT_BTS;

  TRY
    {
      execute_command ((char *) "target record-btrace", from_tty);
    }
  CATCH (exception, RETURN_MASK_ALL)
    {
      record_btrace_conf.format = BTRACE_FORMAT_NONE;
      throw_exception (exception);
    }
  END_CATCH
}

/* Start recording in Intel Processor Trace format.  */

static void
cmd_record_btrace_pt_start (char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  record_btrace_conf.format = BTRACE_FORMAT_PT;

  TRY
    {
      execute_command ((char *) "target record-btrace", from_tty);
    }
  CATCH (exception, RETURN_MASK_ALL)
    {
      record_btrace_conf.format = BTRACE_FORMAT_NONE;
      throw_exception (exception);
    }
  END_CATCH
}

/* Alias for "target record".  */

static void
cmd_record_btrace_start (char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  record_btrace_conf.format = BTRACE_FORMAT_PT;

  TRY
    {
      execute_command ((char *) "target record-btrace", from_tty);
    }
  CATCH (exception, RETURN_MASK_ALL)
    {
      record_btrace_conf.format = BTRACE_FORMAT_BTS;

      TRY
	{
	  execute_command ((char *) "target record-btrace", from_tty);
	}
      CATCH (exception, RETURN_MASK_ALL)
	{
	  record_btrace_conf.format = BTRACE_FORMAT_NONE;
	  throw_exception (exception);
	}
      END_CATCH
    }
  END_CATCH
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

/* The "set record btrace bts" command.  */

static void
cmd_set_record_btrace_bts (char *args, int from_tty)
{
  printf_unfiltered (_("\"set record btrace bts\" must be followed "
		       "by an appropriate subcommand.\n"));
  help_list (set_record_btrace_bts_cmdlist, "set record btrace bts ",
	     all_commands, gdb_stdout);
}

/* The "show record btrace bts" command.  */

static void
cmd_show_record_btrace_bts (char *args, int from_tty)
{
  cmd_show_list (show_record_btrace_bts_cmdlist, from_tty, "");
}

/* The "set record btrace pt" command.  */

static void
cmd_set_record_btrace_pt (char *args, int from_tty)
{
  printf_unfiltered (_("\"set record btrace pt\" must be followed "
		       "by an appropriate subcommand.\n"));
  help_list (set_record_btrace_pt_cmdlist, "set record btrace pt ",
	     all_commands, gdb_stdout);
}

/* The "show record btrace pt" command.  */

static void
cmd_show_record_btrace_pt (char *args, int from_tty)
{
  cmd_show_list (show_record_btrace_pt_cmdlist, from_tty, "");
}

/* The "record bts buffer-size" show value function.  */

static void
show_record_bts_buffer_size_value (struct ui_file *file, int from_tty,
				   struct cmd_list_element *c,
				   const char *value)
{
  fprintf_filtered (file, _("The record/replay bts buffer size is %s.\n"),
		    value);
}

/* The "record pt buffer-size" show value function.  */

static void
show_record_pt_buffer_size_value (struct ui_file *file, int from_tty,
				  struct cmd_list_element *c,
				  const char *value)
{
  fprintf_filtered (file, _("The record/replay pt buffer size is %s.\n"),
		    value);
}

void _initialize_record_btrace (void);

/* Initialize btrace commands.  */

void
_initialize_record_btrace (void)
{
  add_prefix_cmd ("btrace", class_obscure, cmd_record_btrace_start,
		  _("Start branch trace recording."), &record_btrace_cmdlist,
		  "record btrace ", 0, &record_cmdlist);
  add_alias_cmd ("b", "btrace", class_obscure, 1, &record_cmdlist);

  add_cmd ("bts", class_obscure, cmd_record_btrace_bts_start,
	   _("\
Start branch trace recording in Branch Trace Store (BTS) format.\n\n\
The processor stores a from/to record for each branch into a cyclic buffer.\n\
This format may not be available on all processors."),
	   &record_btrace_cmdlist);
  add_alias_cmd ("bts", "btrace bts", class_obscure, 1, &record_cmdlist);

  add_cmd ("pt", class_obscure, cmd_record_btrace_pt_start,
	   _("\
Start branch trace recording in Intel Processor Trace format.\n\n\
This format may not be available on all processors."),
	   &record_btrace_cmdlist);
  add_alias_cmd ("pt", "btrace pt", class_obscure, 1, &record_cmdlist);

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

  add_prefix_cmd ("bts", class_support, cmd_set_record_btrace_bts,
		  _("Set record btrace bts options"),
		  &set_record_btrace_bts_cmdlist,
		  "set record btrace bts ", 0, &set_record_btrace_cmdlist);

  add_prefix_cmd ("bts", class_support, cmd_show_record_btrace_bts,
		  _("Show record btrace bts options"),
		  &show_record_btrace_bts_cmdlist,
		  "show record btrace bts ", 0, &show_record_btrace_cmdlist);

  add_setshow_uinteger_cmd ("buffer-size", no_class,
			    &record_btrace_conf.bts.size,
			    _("Set the record/replay bts buffer size."),
			    _("Show the record/replay bts buffer size."), _("\
When starting recording request a trace buffer of this size.  \
The actual buffer size may differ from the requested size.  \
Use \"info record\" to see the actual buffer size.\n\n\
Bigger buffers allow longer recording but also take more time to process \
the recorded execution trace.\n\n\
The trace buffer size may not be changed while recording."), NULL,
			    show_record_bts_buffer_size_value,
			    &set_record_btrace_bts_cmdlist,
			    &show_record_btrace_bts_cmdlist);

  add_prefix_cmd ("pt", class_support, cmd_set_record_btrace_pt,
		  _("Set record btrace pt options"),
		  &set_record_btrace_pt_cmdlist,
		  "set record btrace pt ", 0, &set_record_btrace_cmdlist);

  add_prefix_cmd ("pt", class_support, cmd_show_record_btrace_pt,
		  _("Show record btrace pt options"),
		  &show_record_btrace_pt_cmdlist,
		  "show record btrace pt ", 0, &show_record_btrace_cmdlist);

  add_setshow_uinteger_cmd ("buffer-size", no_class,
			    &record_btrace_conf.pt.size,
			    _("Set the record/replay pt buffer size."),
			    _("Show the record/replay pt buffer size."), _("\
Bigger buffers allow longer recording but also take more time to process \
the recorded execution.\n\
The actual buffer size may differ from the requested size.  Use \"info record\" \
to see the actual buffer size."), NULL, show_record_pt_buffer_size_value,
			    &set_record_btrace_pt_cmdlist,
			    &show_record_btrace_pt_cmdlist);

  init_record_btrace_ops ();
  add_target (&record_btrace_ops);

  bfcache = htab_create_alloc (50, bfcache_hash, bfcache_eq, NULL,
			       xcalloc, xfree);

  record_btrace_conf.bts.size = 64 * 1024;
  record_btrace_conf.pt.size = 16 * 1024;
}
