/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2014 Free Software Foundation, Inc.

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
#include "exceptions.h"
#include "cli/cli-utils.h"
#include "source.h"
#include "ui-out.h"
#include "symtab.h"
#include "filenames.h"

/* The target_ops of record-btrace.  */
static struct target_ops record_btrace_ops;

/* A new thread observer enabling branch tracing for the new thread.  */
static struct observer *record_btrace_thread_observer;

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
   branch trace information struct.

   Throws an error if there is no thread or no trace.  This function never
   returns NULL.  */

static struct btrace_thread_info *
require_btrace (void)
{
  struct thread_info *tp;
  struct btrace_thread_info *btinfo;

  DEBUG ("require");

  tp = find_thread_ptid (inferior_ptid);
  if (tp == NULL)
    error (_("No thread."));

  btrace_fetch (tp);

  btinfo = &tp->btrace;

  if (VEC_empty (btrace_inst_s, btinfo->itrace))
    error (_("No trace."));

  return btinfo;
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

/* The to_open method of target record-btrace.  */

static void
record_btrace_open (char *args, int from_tty)
{
  struct cleanup *disable_chain;
  struct thread_info *tp;

  DEBUG ("open");

  if (RECORD_IS_USED)
    error (_("The process is already being recorded."));

  if (!target_has_execution)
    error (_("The program is not being run."));

  if (!target_supports_btrace ())
    error (_("Target does not support branch tracing."));

  gdb_assert (record_btrace_thread_observer == NULL);

  disable_chain = make_cleanup (null_cleanup, NULL);
  ALL_THREADS (tp)
    if (args == NULL || *args == 0 || number_is_in_list (args, tp->num))
      {
	btrace_enable (tp);

	make_cleanup (record_btrace_disable_callback, tp);
      }

  record_btrace_auto_enable ();

  push_target (&record_btrace_ops);

  observer_notify_record_changed (current_inferior (),  1);

  discard_cleanups (disable_chain);
}

/* The to_stop_recording method of target record-btrace.  */

static void
record_btrace_stop_recording (void)
{
  struct thread_info *tp;

  DEBUG ("stop recording");

  record_btrace_auto_disable ();

  ALL_THREADS (tp)
    if (tp->btrace.target != NULL)
      btrace_disable (tp);
}

/* The to_close method of target record-btrace.  */

static void
record_btrace_close (void)
{
  /* Make sure automatic recording gets disabled even if we did not stop
     recording before closing the record-btrace target.  */
  record_btrace_auto_disable ();

  /* We already stopped recording.  */
}

/* The to_info_record method of target record-btrace.  */

static void
record_btrace_info (void)
{
  struct btrace_thread_info *btinfo;
  struct thread_info *tp;
  unsigned int insts, funcs;

  DEBUG ("info");

  tp = find_thread_ptid (inferior_ptid);
  if (tp == NULL)
    error (_("No thread."));

  btrace_fetch (tp);

  btinfo = &tp->btrace;
  insts = VEC_length (btrace_inst_s, btinfo->itrace);
  funcs = VEC_length (btrace_func_s, btinfo->ftrace);

  printf_unfiltered (_("Recorded %u instructions in %u functions for thread "
		       "%d (%s).\n"), insts, funcs, tp->num,
		     target_pid_to_str (tp->ptid));
}

/* Print an unsigned int.  */

static void
ui_out_field_uint (struct ui_out *uiout, const char *fld, unsigned int val)
{
  ui_out_field_fmt (uiout, fld, "%u", val);
}

/* Disassemble a section of the recorded instruction trace.  */

static void
btrace_insn_history (struct btrace_thread_info *btinfo, struct ui_out *uiout,
		     unsigned int begin, unsigned int end, int flags)
{
  struct gdbarch *gdbarch;
  struct btrace_inst *inst;
  unsigned int idx;

  DEBUG ("itrace (0x%x): [%u; %u[", flags, begin, end);

  gdbarch = target_gdbarch ();

  for (idx = begin; VEC_iterate (btrace_inst_s, btinfo->itrace, idx, inst)
	 && idx < end; ++idx)
    {
      /* Print the instruction index.  */
      ui_out_field_uint (uiout, "index", idx);
      ui_out_text (uiout, "\t");

      /* Disassembly with '/m' flag may not produce the expected result.
	 See PR gdb/11833.  */
      gdb_disassembly (gdbarch, uiout, NULL, flags, 1, inst->pc, inst->pc + 1);
    }
}

/* The to_insn_history method of target record-btrace.  */

static void
record_btrace_insn_history (int size, int flags)
{
  struct btrace_thread_info *btinfo;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int context, last, begin, end;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "insn history");
  btinfo = require_btrace ();
  last = VEC_length (btrace_inst_s, btinfo->itrace);

  context = abs (size);
  begin = btinfo->insn_iterator.begin;
  end = btinfo->insn_iterator.end;

  DEBUG ("insn-history (0x%x): %d, prev: [%u; %u[", flags, size, begin, end);

  if (context == 0)
    error (_("Bad record instruction-history-size."));

  /* We start at the end.  */
  if (end < begin)
    {
      /* Truncate the context, if necessary.  */
      context = min (context, last);

      end = last;
      begin = end - context;
    }
  else if (size < 0)
    {
      if (begin == 0)
	{
	  printf_unfiltered (_("At the start of the branch trace record.\n"));

	  btinfo->insn_iterator.end = 0;
	  return;
	}

      /* Truncate the context, if necessary.  */
      context = min (context, begin);

      end = begin;
      begin -= context;
    }
  else
    {
      if (end == last)
	{
	  printf_unfiltered (_("At the end of the branch trace record.\n"));

	  btinfo->insn_iterator.begin = last;
	  return;
	}

      /* Truncate the context, if necessary.  */
      context = min (context, last - end);

      begin = end;
      end += context;
    }

  btrace_insn_history (btinfo, uiout, begin, end, flags);

  btinfo->insn_iterator.begin = begin;
  btinfo->insn_iterator.end = end;

  do_cleanups (uiout_cleanup);
}

/* The to_insn_history_range method of target record-btrace.  */

static void
record_btrace_insn_history_range (ULONGEST from, ULONGEST to, int flags)
{
  struct btrace_thread_info *btinfo;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int last, begin, end;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "insn history");
  btinfo = require_btrace ();
  last = VEC_length (btrace_inst_s, btinfo->itrace);

  begin = (unsigned int) from;
  end = (unsigned int) to;

  DEBUG ("insn-history (0x%x): [%u; %u[", flags, begin, end);

  /* Check for wrap-arounds.  */
  if (begin != from || end != to)
    error (_("Bad range."));

  if (end <= begin)
    error (_("Bad range."));

  if (last <= begin)
    error (_("Range out of bounds."));

  /* Truncate the range, if necessary.  */
  if (last < end)
    end = last;

  btrace_insn_history (btinfo, uiout, begin, end, flags);

  btinfo->insn_iterator.begin = begin;
  btinfo->insn_iterator.end = end;

  do_cleanups (uiout_cleanup);
}

/* The to_insn_history_from method of target record-btrace.  */

static void
record_btrace_insn_history_from (ULONGEST from, int size, int flags)
{
  ULONGEST begin, end, context;

  context = abs (size);

  if (size < 0)
    {
      end = from;

      if (from < context)
	begin = 0;
      else
	begin = from - context;
    }
  else
    {
      begin = from;
      end = from + context;

      /* Check for wrap-around.  */
      if (end < begin)
	end = ULONGEST_MAX;
    }

  record_btrace_insn_history_range (begin, end, flags);
}

/* Print the instruction number range for a function call history line.  */

static void
btrace_func_history_insn_range (struct ui_out *uiout, struct btrace_func *bfun)
{
  ui_out_field_uint (uiout, "insn begin", bfun->ibegin);

  if (bfun->ibegin == bfun->iend)
    return;

  ui_out_text (uiout, "-");
  ui_out_field_uint (uiout, "insn end", bfun->iend);
}

/* Print the source line information for a function call history line.  */

static void
btrace_func_history_src_line (struct ui_out *uiout, struct btrace_func *bfun)
{
  struct symbol *sym;

  sym = bfun->sym;
  if (sym == NULL)
    return;

  ui_out_field_string (uiout, "file",
		       symtab_to_filename_for_display (sym->symtab));

  if (bfun->lend == 0)
    return;

  ui_out_text (uiout, ":");
  ui_out_field_int (uiout, "min line", bfun->lbegin);

  if (bfun->lend == bfun->lbegin)
    return;

  ui_out_text (uiout, "-");
  ui_out_field_int (uiout, "max line", bfun->lend);
}

/* Disassemble a section of the recorded function trace.  */

static void
btrace_func_history (struct btrace_thread_info *btinfo, struct ui_out *uiout,
		     unsigned int begin, unsigned int end,
		     enum record_print_flag flags)
{
  struct btrace_func *bfun;
  unsigned int idx;

  DEBUG ("ftrace (0x%x): [%u; %u[", flags, begin, end);

  for (idx = begin; VEC_iterate (btrace_func_s, btinfo->ftrace, idx, bfun)
	 && idx < end; ++idx)
    {
      /* Print the function index.  */
      ui_out_field_uint (uiout, "index", idx);
      ui_out_text (uiout, "\t");

      if ((flags & RECORD_PRINT_INSN_RANGE) != 0)
	{
	  btrace_func_history_insn_range (uiout, bfun);
	  ui_out_text (uiout, "\t");
	}

      if ((flags & RECORD_PRINT_SRC_LINE) != 0)
	{
	  btrace_func_history_src_line (uiout, bfun);
	  ui_out_text (uiout, "\t");
	}

      if (bfun->sym != NULL)
	ui_out_field_string (uiout, "function", SYMBOL_PRINT_NAME (bfun->sym));
      else if (bfun->msym != NULL)
	ui_out_field_string (uiout, "function", SYMBOL_PRINT_NAME (bfun->msym));
      ui_out_text (uiout, "\n");
    }
}

/* The to_call_history method of target record-btrace.  */

static void
record_btrace_call_history (int size, int flags)
{
  struct btrace_thread_info *btinfo;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int context, last, begin, end;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "insn history");
  btinfo = require_btrace ();
  last = VEC_length (btrace_func_s, btinfo->ftrace);

  context = abs (size);
  begin = btinfo->func_iterator.begin;
  end = btinfo->func_iterator.end;

  DEBUG ("func-history (0x%x): %d, prev: [%u; %u[", flags, size, begin, end);

  if (context == 0)
    error (_("Bad record function-call-history-size."));

  /* We start at the end.  */
  if (end < begin)
    {
      /* Truncate the context, if necessary.  */
      context = min (context, last);

      end = last;
      begin = end - context;
    }
  else if (size < 0)
    {
      if (begin == 0)
	{
	  printf_unfiltered (_("At the start of the branch trace record.\n"));

	  btinfo->func_iterator.end = 0;
	  return;
	}

      /* Truncate the context, if necessary.  */
      context = min (context, begin);

      end = begin;
      begin -= context;
    }
  else
    {
      if (end == last)
	{
	  printf_unfiltered (_("At the end of the branch trace record.\n"));

	  btinfo->func_iterator.begin = last;
	  return;
	}

      /* Truncate the context, if necessary.  */
      context = min (context, last - end);

      begin = end;
      end += context;
    }

  btrace_func_history (btinfo, uiout, begin, end, flags);

  btinfo->func_iterator.begin = begin;
  btinfo->func_iterator.end = end;

  do_cleanups (uiout_cleanup);
}

/* The to_call_history_range method of target record-btrace.  */

static void
record_btrace_call_history_range (ULONGEST from, ULONGEST to, int flags)
{
  struct btrace_thread_info *btinfo;
  struct cleanup *uiout_cleanup;
  struct ui_out *uiout;
  unsigned int last, begin, end;

  uiout = current_uiout;
  uiout_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout,
						       "func history");
  btinfo = require_btrace ();
  last = VEC_length (btrace_func_s, btinfo->ftrace);

  begin = (unsigned int) from;
  end = (unsigned int) to;

  DEBUG ("func-history (0x%x): [%u; %u[", flags, begin, end);

  /* Check for wrap-arounds.  */
  if (begin != from || end != to)
    error (_("Bad range."));

  if (end <= begin)
    error (_("Bad range."));

  if (last <= begin)
    error (_("Range out of bounds."));

  /* Truncate the range, if necessary.  */
  if (last < end)
    end = last;

  btrace_func_history (btinfo, uiout, begin, end, flags);

  btinfo->func_iterator.begin = begin;
  btinfo->func_iterator.end = end;

  do_cleanups (uiout_cleanup);
}

/* The to_call_history_from method of target record-btrace.  */

static void
record_btrace_call_history_from (ULONGEST from, int size, int flags)
{
  ULONGEST begin, end, context;

  context = abs (size);

  if (size < 0)
    {
      end = from;

      if (from < context)
	begin = 0;
      else
	begin = from - context;
    }
  else
    {
      begin = from;
      end = from + context;

      /* Check for wrap-around.  */
      if (end < begin)
	end = ULONGEST_MAX;
    }

  record_btrace_call_history_range (begin, end, flags);
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
  ops->to_detach = record_detach;
  ops->to_disconnect = record_disconnect;
  ops->to_mourn_inferior = record_mourn_inferior;
  ops->to_kill = record_kill;
  ops->to_create_inferior = find_default_create_inferior;
  ops->to_stop_recording = record_btrace_stop_recording;
  ops->to_info_record = record_btrace_info;
  ops->to_insn_history = record_btrace_insn_history;
  ops->to_insn_history_from = record_btrace_insn_history_from;
  ops->to_insn_history_range = record_btrace_insn_history_range;
  ops->to_call_history = record_btrace_call_history;
  ops->to_call_history_from = record_btrace_call_history_from;
  ops->to_call_history_range = record_btrace_call_history_range;
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

void _initialize_record_btrace (void);

/* Initialize btrace commands.  */

void
_initialize_record_btrace (void)
{
  add_cmd ("btrace", class_obscure, cmd_record_btrace_start,
	   _("Start branch trace recording."),
	   &record_cmdlist);
  add_alias_cmd ("b", "btrace", class_obscure, 1, &record_cmdlist);

  init_record_btrace_ops ();
  add_target (&record_btrace_ops);
}
