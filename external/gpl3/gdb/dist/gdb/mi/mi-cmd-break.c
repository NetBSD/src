/* MI Command Set - breakpoint and watchpoint commands.
   Copyright (C) 2000-2013 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "arch-utils.h"
#include "mi-cmds.h"
#include "ui-out.h"
#include "mi-out.h"
#include "breakpoint.h"
#include "gdb_string.h"
#include "mi-getopt.h"
#include "gdb.h"
#include "exceptions.h"
#include "observer.h"
#include "mi-main.h"
#include "mi-cmd-break.h"

enum
  {
    FROM_TTY = 0
  };

/* True if MI breakpoint observers have been registered.  */

static int mi_breakpoint_observers_installed;

/* Control whether breakpoint_notify may act.  */

static int mi_can_breakpoint_notify;

/* Output a single breakpoint, when allowed.  */

static void
breakpoint_notify (struct breakpoint *b)
{
  if (mi_can_breakpoint_notify)
    gdb_breakpoint_query (current_uiout, b->number, NULL);
}

enum bp_type
  {
    REG_BP,
    HW_BP,
    REGEXP_BP
  };

/* Arrange for all new breakpoints and catchpoints to be reported to
   CURRENT_UIOUT until the cleanup returned by this function is run.

   Note that MI output will be probably invalid if more than one
   breakpoint is created inside one MI command.  */

struct cleanup *
setup_breakpoint_reporting (void)
{
  struct cleanup *rev_flag;

  if (! mi_breakpoint_observers_installed)
    {
      observer_attach_breakpoint_created (breakpoint_notify);
      mi_breakpoint_observers_installed = 1;
    }

  rev_flag = make_cleanup_restore_integer (&mi_can_breakpoint_notify);
  mi_can_breakpoint_notify = 1;

  return rev_flag;
}


/* Implements the -break-insert command.
   See the MI manual for the list of possible options.  */

void
mi_cmd_break_insert (char *command, char **argv, int argc)
{
  char *address = NULL;
  int hardware = 0;
  int temp_p = 0;
  int thread = -1;
  int ignore_count = 0;
  char *condition = NULL;
  int pending = 0;
  int enabled = 1;
  int tracepoint = 0;
  struct cleanup *back_to;
  enum bptype type_wanted;
  struct breakpoint_ops *ops;

  enum opt
    {
      HARDWARE_OPT, TEMP_OPT, CONDITION_OPT,
      IGNORE_COUNT_OPT, THREAD_OPT, PENDING_OPT, DISABLE_OPT,
      TRACEPOINT_OPT,
    };
  static const struct mi_opt opts[] =
  {
    {"h", HARDWARE_OPT, 0},
    {"t", TEMP_OPT, 0},
    {"c", CONDITION_OPT, 1},
    {"i", IGNORE_COUNT_OPT, 1},
    {"p", THREAD_OPT, 1},
    {"f", PENDING_OPT, 0},
    {"d", DISABLE_OPT, 0},
    {"a", TRACEPOINT_OPT, 0},
    { 0, 0, 0 }
  };

  /* Parse arguments. It could be -r or -h or -t, <location> or ``--''
     to denote the end of the option list. */
  int oind = 0;
  char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-break-insert", argc, argv,
			   opts, &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case TEMP_OPT:
	  temp_p = 1;
	  break;
	case HARDWARE_OPT:
	  hardware = 1;
	  break;
	case CONDITION_OPT:
	  condition = oarg;
	  break;
	case IGNORE_COUNT_OPT:
	  ignore_count = atol (oarg);
	  break;
	case THREAD_OPT:
	  thread = atol (oarg);
	  break;
	case PENDING_OPT:
	  pending = 1;
	  break;
	case DISABLE_OPT:
	  enabled = 0;
	  break;
	case TRACEPOINT_OPT:
	  tracepoint = 1;
	  break;
	}
    }

  if (oind >= argc)
    error (_("-break-insert: Missing <location>"));
  if (oind < argc - 1)
    error (_("-break-insert: Garbage following <location>"));
  address = argv[oind];

  /* Now we have what we need, let's insert the breakpoint!  */
  back_to = setup_breakpoint_reporting ();

  /* Note that to request a fast tracepoint, the client uses the
     "hardware" flag, although there's nothing of hardware related to
     fast tracepoints -- one can implement slow tracepoints with
     hardware breakpoints, but fast tracepoints are always software.
     "fast" is a misnomer, actually, "jump" would be more appropriate.
     A simulator or an emulator could conceivably implement fast
     regular non-jump based tracepoints.  */
  type_wanted = (tracepoint
		 ? (hardware ? bp_fast_tracepoint : bp_tracepoint)
		 : (hardware ? bp_hardware_breakpoint : bp_breakpoint));
  ops = tracepoint ? &tracepoint_breakpoint_ops : &bkpt_breakpoint_ops;

  create_breakpoint (get_current_arch (), address, condition, thread,
		     NULL,
		     0 /* condition and thread are valid.  */,
		     temp_p, type_wanted,
		     ignore_count,
		     pending ? AUTO_BOOLEAN_TRUE : AUTO_BOOLEAN_FALSE,
		     ops, 0, enabled, 0, 0);
  do_cleanups (back_to);

}

enum wp_type
{
  REG_WP,
  READ_WP,
  ACCESS_WP
};

void
mi_cmd_break_passcount (char *command, char **argv, int argc)
{
  int n;
  int p;
  struct tracepoint *t;

  if (argc != 2)
    error (_("Usage: tracepoint-number passcount"));

  n = atoi (argv[0]);
  p = atoi (argv[1]);
  t = get_tracepoint (n);

  if (t)
    {
      t->pass_count = p;
      observer_notify_breakpoint_modified (&t->base);
    }
  else
    {
      error (_("Could not find tracepoint %d"), n);
    }
}

/* Insert a watchpoint. The type of watchpoint is specified by the
   first argument: 
   -break-watch <expr> --> insert a regular wp.  
   -break-watch -r <expr> --> insert a read watchpoint.
   -break-watch -a <expr> --> insert an access wp.  */

void
mi_cmd_break_watch (char *command, char **argv, int argc)
{
  char *expr = NULL;
  enum wp_type type = REG_WP;
  enum opt
    {
      READ_OPT, ACCESS_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"r", READ_OPT, 0},
    {"a", ACCESS_OPT, 0},
    { 0, 0, 0 }
  };

  /* Parse arguments. */
  int oind = 0;
  char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-break-watch", argc, argv,
			   opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case READ_OPT:
	  type = READ_WP;
	  break;
	case ACCESS_OPT:
	  type = ACCESS_WP;
	  break;
	}
    }
  if (oind >= argc)
    error (_("-break-watch: Missing <expression>"));
  if (oind < argc - 1)
    error (_("-break-watch: Garbage following <expression>"));
  expr = argv[oind];

  /* Now we have what we need, let's insert the watchpoint!  */
  switch (type)
    {
    case REG_WP:
      watch_command_wrapper (expr, FROM_TTY, 0);
      break;
    case READ_WP:
      rwatch_command_wrapper (expr, FROM_TTY, 0);
      break;
    case ACCESS_WP:
      awatch_command_wrapper (expr, FROM_TTY, 0);
      break;
    default:
      error (_("-break-watch: Unknown watchpoint type."));
    }
}

/* The mi_read_next_line consults these variable to return successive
   command lines.  While it would be clearer to use a closure pointer,
   it is not expected that any future code will use read_command_lines_1,
   therefore no point of overengineering.  */

static char **mi_command_line_array;
static int mi_command_line_array_cnt;
static int mi_command_line_array_ptr;

static char *
mi_read_next_line (void)
{
  if (mi_command_line_array_ptr == mi_command_line_array_cnt)
    return NULL;
  else
    return mi_command_line_array[mi_command_line_array_ptr++];
}

void
mi_cmd_break_commands (char *command, char **argv, int argc)
{
  struct command_line *break_command;
  char *endptr;
  int bnum;
  struct breakpoint *b;

  if (argc < 1)
    error (_("USAGE: %s <BKPT> [<COMMAND> [<COMMAND>...]]"), command);

  bnum = strtol (argv[0], &endptr, 0);
  if (endptr == argv[0])
    error (_("breakpoint number argument \"%s\" is not a number."),
	   argv[0]);
  else if (*endptr != '\0')
    error (_("junk at the end of breakpoint number argument \"%s\"."),
	   argv[0]);

  b = get_breakpoint (bnum);
  if (b == NULL)
    error (_("breakpoint %d not found."), bnum);

  mi_command_line_array = argv;
  mi_command_line_array_ptr = 1;
  mi_command_line_array_cnt = argc;

  if (is_tracepoint (b))
    break_command = read_command_lines_1 (mi_read_next_line, 1,
					  check_tracepoint_command, b);
  else
    break_command = read_command_lines_1 (mi_read_next_line, 1, 0, 0);

  breakpoint_set_commands (b, break_command);
}

