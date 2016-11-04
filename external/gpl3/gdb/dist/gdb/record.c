/* Process record and replay target for GDB, the GNU debugger.

   Copyright (C) 2008-2016 Free Software Foundation, Inc.

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
#include "completer.h"
#include "record.h"
#include "observer.h"
#include "inferior.h"
#include "common/common-utils.h"
#include "cli/cli-utils.h"
#include "disasm.h"

#include <ctype.h>

/* This is the debug switch for process record.  */
unsigned int record_debug = 0;

/* The number of instructions to print in "record instruction-history".  */
static unsigned int record_insn_history_size = 10;

/* The variable registered as control variable in the "record
   instruction-history" command.  Necessary for extra input
   validation.  */
static unsigned int record_insn_history_size_setshow_var;

/* The number of functions to print in "record function-call-history".  */
static unsigned int record_call_history_size = 10;

/* The variable registered as control variable in the "record
   call-history" command.  Necessary for extra input validation.  */
static unsigned int record_call_history_size_setshow_var;

struct cmd_list_element *record_cmdlist = NULL;
struct cmd_list_element *record_goto_cmdlist = NULL;
struct cmd_list_element *set_record_cmdlist = NULL;
struct cmd_list_element *show_record_cmdlist = NULL;
struct cmd_list_element *info_record_cmdlist = NULL;

#define DEBUG(msg, args...)						\
  if (record_debug)							\
    fprintf_unfiltered (gdb_stdlog, "record: " msg "\n", ##args)

/* See record.h.  */

struct target_ops *
find_record_target (void)
{
  return find_target_at (record_stratum);
}

/* Check that recording is active.  Throw an error, if it isn't.  */

static struct target_ops *
require_record_target (void)
{
  struct target_ops *t;

  t = find_record_target ();
  if (t == NULL)
    error (_("No record target is currently active.\n"
	     "Use one of the \"target record-<tab><tab>\" commands first."));

  return t;
}

/* See record.h.  */

void
record_preopen (void)
{
  /* Check if a record target is already running.  */
  if (find_record_target () != NULL)
    error (_("The process is already being recorded.  Use \"record stop\" to "
	     "stop recording first."));
}

/* See record.h.  */

int
record_read_memory (struct gdbarch *gdbarch,
		    CORE_ADDR memaddr, gdb_byte *myaddr,
		    ssize_t len)
{
  int ret = target_read_memory (memaddr, myaddr, len);

  if (ret != 0)
    DEBUG ("error reading memory at addr %s len = %ld.\n",
	   paddress (gdbarch, memaddr), (long) len);

  return ret;
}

/* Stop recording.  */

static void
record_stop (struct target_ops *t)
{
  DEBUG ("stop %s", t->to_shortname);

  t->to_stop_recording (t);
}

/* Unpush the record target.  */

static void
record_unpush (struct target_ops *t)
{
  DEBUG ("unpush %s", t->to_shortname);

  unpush_target (t);
}

/* See record.h.  */

void
record_disconnect (struct target_ops *t, const char *args, int from_tty)
{
  gdb_assert (t->to_stratum == record_stratum);

  DEBUG ("disconnect %s", t->to_shortname);

  record_stop (t);
  record_unpush (t);

  target_disconnect (args, from_tty);
}

/* See record.h.  */

void
record_detach (struct target_ops *t, const char *args, int from_tty)
{
  gdb_assert (t->to_stratum == record_stratum);

  DEBUG ("detach %s", t->to_shortname);

  record_stop (t);
  record_unpush (t);

  target_detach (args, from_tty);
}

/* See record.h.  */

void
record_mourn_inferior (struct target_ops *t)
{
  gdb_assert (t->to_stratum == record_stratum);

  DEBUG ("mourn inferior %s", t->to_shortname);

  /* It is safer to not stop recording.  Resources will be freed when
     threads are discarded.  */
  record_unpush (t);

  target_mourn_inferior ();
}

/* See record.h.  */

void
record_kill (struct target_ops *t)
{
  gdb_assert (t->to_stratum == record_stratum);

  DEBUG ("kill %s", t->to_shortname);

  /* It is safer to not stop recording.  Resources will be freed when
     threads are discarded.  */
  record_unpush (t);

  target_kill ();
}

/* See record.h.  */

int
record_check_stopped_by_breakpoint (struct address_space *aspace, CORE_ADDR pc,
				    enum target_stop_reason *reason)
{
  if (breakpoint_inserted_here_p (aspace, pc))
    {
      if (hardware_breakpoint_inserted_here_p (aspace, pc))
	*reason = TARGET_STOPPED_BY_HW_BREAKPOINT;
      else
	*reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
      return 1;
    }

  *reason = TARGET_STOPPED_BY_NO_REASON;
  return 0;
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
  execute_command ("target record-full", from_tty);
}

/* Truncate the record log from the present point
   of replay until the end.  */

static void
cmd_record_delete (char *args, int from_tty)
{
  require_record_target ();

  if (!target_record_is_replaying (inferior_ptid))
    {
      printf_unfiltered (_("Already at end of record list.\n"));
      return;
    }

  if (!target_supports_delete_record ())
    {
      printf_unfiltered (_("The current record target does not support "
			   "this operation.\n"));
      return;
    }

  if (!from_tty || query (_("Delete the log from this point forward "
			    "and begin to record the running message "
			    "at current PC?")))
    target_delete_record ();
}

/* Implement the "stoprecord" or "record stop" command.  */

static void
cmd_record_stop (char *args, int from_tty)
{
  struct target_ops *t;

  t = require_record_target ();

  record_stop (t);
  record_unpush (t);

  printf_unfiltered (_("Process record is stopped and all execution "
		       "logs are deleted.\n"));

  observer_notify_record_changed (current_inferior (), 0, NULL, NULL);
}

/* The "set record" command.  */

static void
set_record_command (char *args, int from_tty)
{
  printf_unfiltered (_("\"set record\" must be followed "
		       "by an apporpriate subcommand.\n"));
  help_list (set_record_cmdlist, "set record ", all_commands, gdb_stdout);
}

/* The "show record" command.  */

static void
show_record_command (char *args, int from_tty)
{
  cmd_show_list (show_record_cmdlist, from_tty, "");
}

/* The "info record" command.  */

static void
info_record_command (char *args, int from_tty)
{
  struct target_ops *t;

  t = find_record_target ();
  if (t == NULL)
    {
      printf_filtered (_("No record target is currently active.\n"));
      return;
    }

  printf_filtered (_("Active record target: %s\n"), t->to_shortname);
  t->to_info_record (t);
}

/* The "record save" command.  */

static void
cmd_record_save (char *args, int from_tty)
{
  char *recfilename, recfilename_buffer[40];

  require_record_target ();

  if (args != NULL && *args != 0)
    recfilename = args;
  else
    {
      /* Default recfile name is "gdb_record.PID".  */
      xsnprintf (recfilename_buffer, sizeof (recfilename_buffer),
                "gdb_record.%d", ptid_get_pid (inferior_ptid));
      recfilename = recfilename_buffer;
    }

  target_save_record (recfilename);
}

/* See record.h.  */

void
record_goto (const char *arg)
{
  ULONGEST insn;

  if (arg == NULL || *arg == '\0')
    error (_("Command requires an argument (insn number to go to)."));

  insn = parse_and_eval_long (arg);

  require_record_target ();
  target_goto_record (insn);
}

/* "record goto" command.  Argument is an instruction number,
   as given by "info record".

   Rewinds the recording (forward or backward) to the given instruction.  */

static void
cmd_record_goto (char *arg, int from_tty)
{
  record_goto (arg);
}

/* The "record goto begin" command.  */

static void
cmd_record_goto_begin (char *arg, int from_tty)
{
  if (arg != NULL && *arg != '\0')
    error (_("Junk after argument: %s."), arg);

  require_record_target ();
  target_goto_record_begin ();
}

/* The "record goto end" command.  */

static void
cmd_record_goto_end (char *arg, int from_tty)
{
  if (arg != NULL && *arg != '\0')
    error (_("Junk after argument: %s."), arg);

  require_record_target ();
  target_goto_record_end ();
}

/* Read an instruction number from an argument string.  */

static ULONGEST
get_insn_number (char **arg)
{
  ULONGEST number;
  const char *begin, *end, *pos;

  begin = *arg;
  pos = skip_spaces_const (begin);

  if (!isdigit (*pos))
    error (_("Expected positive number, got: %s."), pos);

  number = strtoulst (pos, &end, 10);

  *arg += (end - begin);

  return number;
}

/* Read a context size from an argument string.  */

static int
get_context_size (char **arg)
{
  char *pos;
  int number;

  pos = skip_spaces (*arg);

  if (!isdigit (*pos))
    error (_("Expected positive number, got: %s."), pos);

  return strtol (pos, arg, 10);
}

/* Complain about junk at the end of an argument string.  */

static void
no_chunk (char *arg)
{
  if (*arg != 0)
    error (_("Junk after argument: %s."), arg);
}

/* Read instruction-history modifiers from an argument string.  */

static int
get_insn_history_modifiers (char **arg)
{
  int modifiers;
  char *args;

  modifiers = 0;
  args = *arg;

  if (args == NULL)
    return modifiers;

  while (*args == '/')
    {
      ++args;

      if (*args == '\0')
	error (_("Missing modifier."));

      for (; *args; ++args)
	{
	  if (isspace (*args))
	    break;

	  if (*args == '/')
	    continue;

	  switch (*args)
	    {
	    case 'm':
	    case 's':
	      modifiers |= DISASSEMBLY_SOURCE;
	      modifiers |= DISASSEMBLY_FILENAME;
	      break;
	    case 'r':
	      modifiers |= DISASSEMBLY_RAW_INSN;
	      break;
	    case 'f':
	      modifiers |= DISASSEMBLY_OMIT_FNAME;
	      break;
	    case 'p':
	      modifiers |= DISASSEMBLY_OMIT_PC;
	      break;
	    default:
	      error (_("Invalid modifier: %c."), *args);
	    }
	}

      args = skip_spaces (args);
    }

  /* Update the argument string.  */
  *arg = args;

  return modifiers;
}

/* The "set record instruction-history-size / set record
   function-call-history-size" commands are unsigned, with UINT_MAX
   meaning unlimited.  The target interfaces works with signed int
   though, to indicate direction, so map "unlimited" to INT_MAX, which
   is about the same as unlimited in practice.  If the user does have
   a log that huge, she can fetch it in chunks across several requests,
   but she'll likely have other problems first...  */

static int
command_size_to_target_size (unsigned int size)
{
  gdb_assert (size <= INT_MAX || size == UINT_MAX);

  if (size == UINT_MAX)
    return INT_MAX;
  else
    return size;
}

/* The "record instruction-history" command.  */

static void
cmd_record_insn_history (char *arg, int from_tty)
{
  int flags, size;

  require_record_target ();

  flags = get_insn_history_modifiers (&arg);

  size = command_size_to_target_size (record_insn_history_size);

  if (arg == NULL || *arg == 0 || strcmp (arg, "+") == 0)
    target_insn_history (size, flags);
  else if (strcmp (arg, "-") == 0)
    target_insn_history (-size, flags);
  else
    {
      ULONGEST begin, end;

      begin = get_insn_number (&arg);

      if (*arg == ',')
	{
	  arg = skip_spaces (++arg);

	  if (*arg == '+')
	    {
	      arg += 1;
	      size = get_context_size (&arg);

	      no_chunk (arg);

	      target_insn_history_from (begin, size, flags);
	    }
	  else if (*arg == '-')
	    {
	      arg += 1;
	      size = get_context_size (&arg);

	      no_chunk (arg);

	      target_insn_history_from (begin, -size, flags);
	    }
	  else
	    {
	      end = get_insn_number (&arg);

	      no_chunk (arg);

	      target_insn_history_range (begin, end, flags);
	    }
	}
      else
	{
	  no_chunk (arg);

	  target_insn_history_from (begin, size, flags);
	}

      dont_repeat ();
    }
}

/* Read function-call-history modifiers from an argument string.  */

static int
get_call_history_modifiers (char **arg)
{
  int modifiers;
  char *args;

  modifiers = 0;
  args = *arg;

  if (args == NULL)
    return modifiers;

  while (*args == '/')
    {
      ++args;

      if (*args == '\0')
	error (_("Missing modifier."));

      for (; *args; ++args)
	{
	  if (isspace (*args))
	    break;

	  if (*args == '/')
	    continue;

	  switch (*args)
	    {
	    case 'l':
	      modifiers |= RECORD_PRINT_SRC_LINE;
	      break;
	    case 'i':
	      modifiers |= RECORD_PRINT_INSN_RANGE;
	      break;
	    case 'c':
	      modifiers |= RECORD_PRINT_INDENT_CALLS;
	      break;
	    default:
	      error (_("Invalid modifier: %c."), *args);
	    }
	}

      args = skip_spaces (args);
    }

  /* Update the argument string.  */
  *arg = args;

  return modifiers;
}

/* The "record function-call-history" command.  */

static void
cmd_record_call_history (char *arg, int from_tty)
{
  int flags, size;

  require_record_target ();

  flags = get_call_history_modifiers (&arg);

  size = command_size_to_target_size (record_call_history_size);

  if (arg == NULL || *arg == 0 || strcmp (arg, "+") == 0)
    target_call_history (size, flags);
  else if (strcmp (arg, "-") == 0)
    target_call_history (-size, flags);
  else
    {
      ULONGEST begin, end;

      begin = get_insn_number (&arg);

      if (*arg == ',')
	{
	  arg = skip_spaces (++arg);

	  if (*arg == '+')
	    {
	      arg += 1;
	      size = get_context_size (&arg);

	      no_chunk (arg);

	      target_call_history_from (begin, size, flags);
	    }
	  else if (*arg == '-')
	    {
	      arg += 1;
	      size = get_context_size (&arg);

	      no_chunk (arg);

	      target_call_history_from (begin, -size, flags);
	    }
	  else
	    {
	      end = get_insn_number (&arg);

	      no_chunk (arg);

	      target_call_history_range (begin, end, flags);
	    }
	}
      else
	{
	  no_chunk (arg);

	  target_call_history_from (begin, size, flags);
	}

      dont_repeat ();
    }
}

/* Helper for "set record instruction-history-size" and "set record
   function-call-history-size" input validation.  COMMAND_VAR is the
   variable registered in the command as control variable.  *SETTING
   is the real setting the command allows changing.  */

static void
validate_history_size (unsigned int *command_var, unsigned int *setting)
{
  if (*command_var != UINT_MAX && *command_var > INT_MAX)
    {
      unsigned int new_value = *command_var;

      /* Restore previous value.  */
      *command_var = *setting;
      error (_("integer %u out of range"), new_value);
    }

  /* Commit new value.  */
  *setting = *command_var;
}

/* Called by do_setshow_command.  We only want values in the
   [0..INT_MAX] range, while the command's machinery accepts
   [0..UINT_MAX].  See command_size_to_target_size.  */

static void
set_record_insn_history_size (char *args, int from_tty,
			      struct cmd_list_element *c)
{
  validate_history_size (&record_insn_history_size_setshow_var,
			 &record_insn_history_size);
}

/* Called by do_setshow_command.  We only want values in the
   [0..INT_MAX] range, while the command's machinery accepts
   [0..UINT_MAX].  See command_size_to_target_size.  */

static void
set_record_call_history_size (char *args, int from_tty,
			      struct cmd_list_element *c)
{
  validate_history_size (&record_call_history_size_setshow_var,
			 &record_call_history_size);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_record;

void
_initialize_record (void)
{
  struct cmd_list_element *c;

  add_setshow_zuinteger_cmd ("record", no_class, &record_debug,
			     _("Set debugging of record/replay feature."),
			     _("Show debugging of record/replay feature."),
			     _("When enabled, debugging output for "
			       "record/replay feature is displayed."),
			     NULL, show_record_debug, &setdebuglist,
			     &showdebuglist);

  add_setshow_uinteger_cmd ("instruction-history-size", no_class,
			    &record_insn_history_size_setshow_var, _("\
Set number of instructions to print in \"record instruction-history\"."), _("\
Show number of instructions to print in \"record instruction-history\"."), _("\
A size of \"unlimited\" means unlimited instructions.  The default is 10."),
			    set_record_insn_history_size, NULL,
			    &set_record_cmdlist, &show_record_cmdlist);

  add_setshow_uinteger_cmd ("function-call-history-size", no_class,
			    &record_call_history_size_setshow_var, _("\
Set number of function to print in \"record function-call-history\"."), _("\
Show number of functions to print in \"record function-call-history\"."), _("\
A size of \"unlimited\" means unlimited lines.  The default is 10."),
			    set_record_call_history_size, NULL,
			    &set_record_cmdlist, &show_record_cmdlist);

  c = add_prefix_cmd ("record", class_obscure, cmd_record_start,
		      _("Start recording."),
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

  add_cmd ("delete", class_obscure, cmd_record_delete,
	   _("Delete the rest of execution log and start recording it anew."),
           &record_cmdlist);
  add_alias_cmd ("d", "delete", class_obscure, 1, &record_cmdlist);
  add_alias_cmd ("del", "delete", class_obscure, 1, &record_cmdlist);

  add_cmd ("stop", class_obscure, cmd_record_stop,
	   _("Stop the record/replay target."),
           &record_cmdlist);
  add_alias_cmd ("s", "stop", class_obscure, 1, &record_cmdlist);

  add_prefix_cmd ("goto", class_obscure, cmd_record_goto, _("\
Restore the program to its state at instruction number N.\n\
Argument is instruction number, as shown by 'info record'."),
		  &record_goto_cmdlist, "record goto ", 1, &record_cmdlist);

  add_cmd ("begin", class_obscure, cmd_record_goto_begin,
	   _("Go to the beginning of the execution log."),
	   &record_goto_cmdlist);
  add_alias_cmd ("start", "begin", class_obscure, 1, &record_goto_cmdlist);

  add_cmd ("end", class_obscure, cmd_record_goto_end,
	   _("Go to the end of the execution log."),
	   &record_goto_cmdlist);

  add_cmd ("instruction-history", class_obscure, cmd_record_insn_history, _("\
Print disassembled instructions stored in the execution log.\n\
With a /m or /s modifier, source lines are included (if available).\n\
With a /r modifier, raw instructions in hex are included.\n\
With a /f modifier, function names are omitted.\n\
With a /p modifier, current position markers are omitted.\n\
With no argument, disassembles ten more instructions after the previous \
disassembly.\n\
\"record instruction-history -\" disassembles ten instructions before a \
previous disassembly.\n\
One argument specifies an instruction number as shown by 'info record', and \
ten instructions are disassembled after that instruction.\n\
Two arguments with comma between them specify starting and ending instruction \
numbers to disassemble.\n\
If the second argument is preceded by '+' or '-', it specifies the distance \
from the first argument.\n\
The number of instructions to disassemble can be defined with \"set record \
instruction-history-size\"."),
           &record_cmdlist);

  add_cmd ("function-call-history", class_obscure, cmd_record_call_history, _("\
Prints the execution history at function granularity.\n\
It prints one line for each sequence of instructions that belong to the same \
function.\n\
Without modifiers, it prints the function name.\n\
With a /l modifier, the source file and line number range is included.\n\
With a /i modifier, the instruction number range is included.\n\
With a /c modifier, the output is indented based on the call stack depth.\n\
With no argument, prints ten more lines after the previous ten-line print.\n\
\"record function-call-history -\" prints ten lines before a previous ten-line \
print.\n\
One argument specifies a function number as shown by 'info record', and \
ten lines are printed after that function.\n\
Two arguments with comma between them specify a range of functions to print.\n\
If the second argument is preceded by '+' or '-', it specifies the distance \
from the first argument.\n\
The number of functions to print can be defined with \"set record \
function-call-history-size\"."),
           &record_cmdlist);

  /* Sync command control variables.  */
  record_insn_history_size_setshow_var = record_insn_history_size;
  record_call_history_size_setshow_var = record_call_history_size;
}
