/* Everything about breakpoints, for GDB.

   Copyright (C) 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995,
   1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
   2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "hashtab.h"
#include "symtab.h"
#include "frame.h"
#include "breakpoint.h"
#include "tracepoint.h"
#include "gdbtypes.h"
#include "expression.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "value.h"
#include "command.h"
#include "inferior.h"
#include "gdbthread.h"
#include "target.h"
#include "language.h"
#include "gdb_string.h"
#include "demangle.h"
#include "filenames.h"
#include "annotate.h"
#include "symfile.h"
#include "objfiles.h"
#include "source.h"
#include "linespec.h"
#include "completer.h"
#include "gdb.h"
#include "ui-out.h"
#include "cli/cli-script.h"
#include "gdb_assert.h"
#include "block.h"
#include "solib.h"
#include "solist.h"
#include "observer.h"
#include "exceptions.h"
#include "memattr.h"
#include "ada-lang.h"
#include "top.h"
#include "wrapper.h"
#include "valprint.h"
#include "jit.h"
#include "xml-syscall.h"
#include "parser-defs.h"
#include "cli/cli-utils.h"

/* readline include files */
#include "readline/readline.h"
#include "readline/history.h"

/* readline defines this.  */
#undef savestring

#include "mi/mi-common.h"
#include "python/python.h"

/* Arguments to pass as context to some catch command handlers.  */
#define CATCH_PERMANENT ((void *) (uintptr_t) 0)
#define CATCH_TEMPORARY ((void *) (uintptr_t) 1)

/* Prototypes for local functions.  */

static void enable_delete_command (char *, int);

static void enable_once_command (char *, int);

static void disable_command (char *, int);

static void enable_command (char *, int);

static void map_breakpoint_numbers (char *, void (*) (struct breakpoint *,
						      void *),
				    void *);

static void ignore_command (char *, int);

static int breakpoint_re_set_one (void *);

static void clear_command (char *, int);

static void catch_command (char *, int);

static int can_use_hardware_watchpoint (struct value *, int);

static void break_command_1 (char *, int, int);

static void mention (struct breakpoint *);

/* This function is used in gdbtk sources and thus can not be made
   static.  */
struct breakpoint *set_raw_breakpoint (struct gdbarch *gdbarch,
					      struct symtab_and_line,
					      enum bptype);

static void breakpoint_adjustment_warning (CORE_ADDR, CORE_ADDR, int, int);

static CORE_ADDR adjust_breakpoint_address (struct gdbarch *gdbarch,
					    CORE_ADDR bpaddr,
                                            enum bptype bptype);

static void describe_other_breakpoints (struct gdbarch *,
					struct program_space *, CORE_ADDR,
					struct obj_section *, int);

static int breakpoint_address_match (struct address_space *aspace1,
				     CORE_ADDR addr1,
				     struct address_space *aspace2,
				     CORE_ADDR addr2);

static int watchpoint_locations_match (struct bp_location *loc1,
				       struct bp_location *loc2);

static int breakpoint_location_address_match (struct bp_location *bl,
					      struct address_space *aspace,
					      CORE_ADDR addr);

static void breakpoints_info (char *, int);

static void watchpoints_info (char *, int);

static int breakpoint_1 (char *, int, 
			 int (*) (const struct breakpoint *));

static int breakpoint_cond_eval (void *);

static void cleanup_executing_breakpoints (void *);

static void commands_command (char *, int);

static void condition_command (char *, int);

typedef enum
  {
    mark_inserted,
    mark_uninserted
  }
insertion_state_t;

static int remove_breakpoint (struct bp_location *, insertion_state_t);
static int remove_breakpoint_1 (struct bp_location *, insertion_state_t);

static enum print_stop_action print_it_typical (bpstat);

static enum print_stop_action print_bp_stop_message (bpstat bs);

static int watchpoint_check (void *);

static void maintenance_info_breakpoints (char *, int);

static int hw_breakpoint_used_count (void);

static int hw_watchpoint_used_count (enum bptype, int *);

static void hbreak_command (char *, int);

static void thbreak_command (char *, int);

static void do_enable_breakpoint (struct breakpoint *, enum bpdisp);

static void stop_command (char *arg, int from_tty);

static void stopin_command (char *arg, int from_tty);

static void stopat_command (char *arg, int from_tty);

static char *ep_parse_optional_if_clause (char **arg);

static void catch_exception_command_1 (enum exception_event_kind ex_event, 
				       char *arg, int tempflag, int from_tty);

static void tcatch_command (char *arg, int from_tty);

static void detach_single_step_breakpoints (void);

static int single_step_breakpoint_inserted_here_p (struct address_space *,
						   CORE_ADDR pc);

static void free_bp_location (struct bp_location *loc);
static void incref_bp_location (struct bp_location *loc);
static void decref_bp_location (struct bp_location **loc);

static struct bp_location *allocate_bp_location (struct breakpoint *bpt);

static void update_global_location_list (int);

static void update_global_location_list_nothrow (int);

static int is_hardware_watchpoint (const struct breakpoint *bpt);

static int is_watchpoint (const struct breakpoint *bpt);

static void insert_breakpoint_locations (void);

static int syscall_catchpoint_p (struct breakpoint *b);

static void tracepoints_info (char *, int);

static void delete_trace_command (char *, int);

static void enable_trace_command (char *, int);

static void disable_trace_command (char *, int);

static void trace_pass_command (char *, int);

/* Assuming we're creating a static tracepoint, does S look like a
   static tracepoint marker spec ("-m MARKER_ID")?  */
#define is_marker_spec(s)						\
  (s != NULL && strncmp (s, "-m", 2) == 0 && ((s)[2] == ' ' || (s)[2] == '\t'))

/* A reference-counted struct command_line.  This lets multiple
   breakpoints share a single command list.  */
struct counted_command_line
{
  /* The reference count.  */
  int refc;

  /* The command list.  */
  struct command_line *commands;
};

struct command_line *
breakpoint_commands (struct breakpoint *b)
{
  return b->commands ? b->commands->commands : NULL;
}

/* Flag indicating that a command has proceeded the inferior past the
   current breakpoint.  */

static int breakpoint_proceeded;

const char *
bpdisp_text (enum bpdisp disp)
{
  /* NOTE: the following values are a part of MI protocol and
     represent values of 'disp' field returned when inferior stops at
     a breakpoint.  */
  static const char * const bpdisps[] = {"del", "dstp", "dis", "keep"};

  return bpdisps[(int) disp];
}

/* Prototypes for exported functions.  */
/* If FALSE, gdb will not use hardware support for watchpoints, even
   if such is available.  */
static int can_use_hw_watchpoints;

static void
show_can_use_hw_watchpoints (struct ui_file *file, int from_tty,
			     struct cmd_list_element *c,
			     const char *value)
{
  fprintf_filtered (file,
		    _("Debugger's willingness to use "
		      "watchpoint hardware is %s.\n"),
		    value);
}

/* If AUTO_BOOLEAN_FALSE, gdb will not attempt to create pending breakpoints.
   If AUTO_BOOLEAN_TRUE, gdb will automatically create pending breakpoints
   for unrecognized breakpoint locations.
   If AUTO_BOOLEAN_AUTO, gdb will query when breakpoints are unrecognized.  */
static enum auto_boolean pending_break_support;
static void
show_pending_break_support (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c,
			    const char *value)
{
  fprintf_filtered (file,
		    _("Debugger's behavior regarding "
		      "pending breakpoints is %s.\n"),
		    value);
}

/* If 1, gdb will automatically use hardware breakpoints for breakpoints
   set with "break" but falling in read-only memory.
   If 0, gdb will warn about such breakpoints, but won't automatically
   use hardware breakpoints.  */
static int automatic_hardware_breakpoints;
static void
show_automatic_hardware_breakpoints (struct ui_file *file, int from_tty,
				     struct cmd_list_element *c,
				     const char *value)
{
  fprintf_filtered (file,
		    _("Automatic usage of hardware breakpoints is %s.\n"),
		    value);
}

/* If on, gdb will keep breakpoints inserted even as inferior is
   stopped, and immediately insert any new breakpoints.  If off, gdb
   will insert breakpoints into inferior only when resuming it, and
   will remove breakpoints upon stop.  If auto, GDB will behave as ON
   if in non-stop mode, and as OFF if all-stop mode.*/

static const char always_inserted_auto[] = "auto";
static const char always_inserted_on[] = "on";
static const char always_inserted_off[] = "off";
static const char *always_inserted_enums[] = {
  always_inserted_auto,
  always_inserted_off,
  always_inserted_on,
  NULL
};
static const char *always_inserted_mode = always_inserted_auto;
static void
show_always_inserted_mode (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  if (always_inserted_mode == always_inserted_auto)
    fprintf_filtered (file,
		      _("Always inserted breakpoint "
			"mode is %s (currently %s).\n"),
		      value,
		      breakpoints_always_inserted_mode () ? "on" : "off");
  else
    fprintf_filtered (file, _("Always inserted breakpoint mode is %s.\n"),
		      value);
}

int
breakpoints_always_inserted_mode (void)
{
  return (always_inserted_mode == always_inserted_on
	  || (always_inserted_mode == always_inserted_auto && non_stop));
}

void _initialize_breakpoint (void);

/* Are we executing breakpoint commands?  */
static int executing_breakpoint_commands;

/* Are overlay event breakpoints enabled? */
static int overlay_events_enabled;

/* See description in breakpoint.h. */
int target_exact_watchpoints = 0;

/* Walk the following statement or block through all breakpoints.
   ALL_BREAKPOINTS_SAFE does so even if the statment deletes the
   current breakpoint.  */

#define ALL_BREAKPOINTS(B)  for (B = breakpoint_chain; B; B = B->next)

#define ALL_BREAKPOINTS_SAFE(B,TMP)	\
	for (B = breakpoint_chain;	\
	     B ? (TMP=B->next, 1): 0;	\
	     B = TMP)

/* Similar iterator for the low-level breakpoints.  SAFE variant is
   not provided so update_global_location_list must not be called
   while executing the block of ALL_BP_LOCATIONS.  */

#define ALL_BP_LOCATIONS(B,BP_TMP)					\
	for (BP_TMP = bp_location;					\
	     BP_TMP < bp_location + bp_location_count && (B = *BP_TMP);	\
	     BP_TMP++)

/* Iterator for tracepoints only.  */

#define ALL_TRACEPOINTS(B)  \
  for (B = breakpoint_chain; B; B = B->next)  \
    if (is_tracepoint (B))

/* Chains of all breakpoints defined.  */

struct breakpoint *breakpoint_chain;

/* Array is sorted by bp_location_compare - primarily by the ADDRESS.  */

static struct bp_location **bp_location;

/* Number of elements of BP_LOCATION.  */

static unsigned bp_location_count;

/* Maximum alignment offset between bp_target_info.PLACED_ADDRESS and
   ADDRESS for the current elements of BP_LOCATION which get a valid
   result from bp_location_has_shadow.  You can use it for roughly
   limiting the subrange of BP_LOCATION to scan for shadow bytes for
   an address you need to read.  */

static CORE_ADDR bp_location_placed_address_before_address_max;

/* Maximum offset plus alignment between bp_target_info.PLACED_ADDRESS
   + bp_target_info.SHADOW_LEN and ADDRESS for the current elements of
   BP_LOCATION which get a valid result from bp_location_has_shadow.
   You can use it for roughly limiting the subrange of BP_LOCATION to
   scan for shadow bytes for an address you need to read.  */

static CORE_ADDR bp_location_shadow_len_after_address_max;

/* The locations that no longer correspond to any breakpoint, unlinked
   from bp_location array, but for which a hit may still be reported
   by a target.  */
VEC(bp_location_p) *moribund_locations = NULL;

/* Number of last breakpoint made.  */

static int breakpoint_count;

/* The value of `breakpoint_count' before the last command that
   created breakpoints.  If the last (break-like) command created more
   than one breakpoint, then the difference between BREAKPOINT_COUNT
   and PREV_BREAKPOINT_COUNT is more than one.  */
static int prev_breakpoint_count;

/* Number of last tracepoint made.  */

static int tracepoint_count;

static struct cmd_list_element *breakpoint_set_cmdlist;
static struct cmd_list_element *breakpoint_show_cmdlist;
struct cmd_list_element *save_cmdlist;

/* Return whether a breakpoint is an active enabled breakpoint.  */
static int
breakpoint_enabled (struct breakpoint *b)
{
  return (b->enable_state == bp_enabled);
}

/* Set breakpoint count to NUM.  */

static void
set_breakpoint_count (int num)
{
  prev_breakpoint_count = breakpoint_count;
  breakpoint_count = num;
  set_internalvar_integer (lookup_internalvar ("bpnum"), num);
}

/* Used by `start_rbreak_breakpoints' below, to record the current
   breakpoint count before "rbreak" creates any breakpoint.  */
static int rbreak_start_breakpoint_count;

/* Called at the start an "rbreak" command to record the first
   breakpoint made.  */

void
start_rbreak_breakpoints (void)
{
  rbreak_start_breakpoint_count = breakpoint_count;
}

/* Called at the end of an "rbreak" command to record the last
   breakpoint made.  */

void
end_rbreak_breakpoints (void)
{
  prev_breakpoint_count = rbreak_start_breakpoint_count;
}

/* Used in run_command to zero the hit count when a new run starts.  */

void
clear_breakpoint_hit_counts (void)
{
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
    b->hit_count = 0;
}

/* Allocate a new counted_command_line with reference count of 1.
   The new structure owns COMMANDS.  */

static struct counted_command_line *
alloc_counted_command_line (struct command_line *commands)
{
  struct counted_command_line *result
    = xmalloc (sizeof (struct counted_command_line));

  result->refc = 1;
  result->commands = commands;
  return result;
}

/* Increment reference count.  This does nothing if CMD is NULL.  */

static void
incref_counted_command_line (struct counted_command_line *cmd)
{
  if (cmd)
    ++cmd->refc;
}

/* Decrement reference count.  If the reference count reaches 0,
   destroy the counted_command_line.  Sets *CMDP to NULL.  This does
   nothing if *CMDP is NULL.  */

static void
decref_counted_command_line (struct counted_command_line **cmdp)
{
  if (*cmdp)
    {
      if (--(*cmdp)->refc == 0)
	{
	  free_command_lines (&(*cmdp)->commands);
	  xfree (*cmdp);
	}
      *cmdp = NULL;
    }
}

/* A cleanup function that calls decref_counted_command_line.  */

static void
do_cleanup_counted_command_line (void *arg)
{
  decref_counted_command_line (arg);
}

/* Create a cleanup that calls decref_counted_command_line on the
   argument.  */

static struct cleanup *
make_cleanup_decref_counted_command_line (struct counted_command_line **cmdp)
{
  return make_cleanup (do_cleanup_counted_command_line, cmdp);
}

/* Default address, symtab and line to put a breakpoint at
   for "break" command with no arg.
   If default_breakpoint_valid is zero, the other three are
   not valid, and "break" with no arg is an error.

   This set by print_stack_frame, which calls set_default_breakpoint.  */

int default_breakpoint_valid;
CORE_ADDR default_breakpoint_address;
struct symtab *default_breakpoint_symtab;
int default_breakpoint_line;
struct program_space *default_breakpoint_pspace;


/* Return the breakpoint with the specified number, or NULL
   if the number does not refer to an existing breakpoint.  */

struct breakpoint *
get_breakpoint (int num)
{
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
    if (b->number == num)
      return b;
  
  return NULL;
}



void
set_breakpoint_condition (struct breakpoint *b, char *exp,
			  int from_tty)
{
  struct bp_location *loc = b->loc;

  for (; loc; loc = loc->next)
    {
      xfree (loc->cond);
      loc->cond = NULL;
    }
  xfree (b->cond_string);
  b->cond_string = NULL;
  xfree (b->cond_exp);
  b->cond_exp = NULL;

  if (*exp == 0)
    {
      if (from_tty)
	printf_filtered (_("Breakpoint %d now unconditional.\n"), b->number);
    }
  else
    {
      char *arg = exp;

      /* I don't know if it matters whether this is the string the user
	 typed in or the decompiled expression.  */
      b->cond_string = xstrdup (arg);
      b->condition_not_parsed = 0;

      if (is_watchpoint (b))
	{
	  innermost_block = NULL;
	  arg = exp;
	  b->cond_exp = parse_exp_1 (&arg, 0, 0);
	  if (*arg)
	    error (_("Junk at end of expression"));
	  b->cond_exp_valid_block = innermost_block;
	}
      else
	{
	  for (loc = b->loc; loc; loc = loc->next)
	    {
	      arg = exp;
	      loc->cond =
		parse_exp_1 (&arg, block_for_pc (loc->address), 0);
	      if (*arg)
		error (_("Junk at end of expression"));
	    }
	}
    }
  breakpoints_changed ();
  observer_notify_breakpoint_modified (b->number);
}

/* condition N EXP -- set break condition of breakpoint N to EXP.  */

static void
condition_command (char *arg, int from_tty)
{
  struct breakpoint *b;
  char *p;
  int bnum;

  if (arg == 0)
    error_no_arg (_("breakpoint number"));

  p = arg;
  bnum = get_number (&p);
  if (bnum == 0)
    error (_("Bad breakpoint argument: '%s'"), arg);

  ALL_BREAKPOINTS (b)
    if (b->number == bnum)
      {
	/* Check if this breakpoint has a Python object assigned to
	   it, and if it has a definition of the "stop"
	   method.  This method and conditions entered into GDB from
	   the CLI are mutually exclusive.  */
	if (b->py_bp_object
	    && gdbpy_breakpoint_has_py_cond (b->py_bp_object))
	  error (_("Cannot set a condition where a Python 'stop' "
		   "method has been defined in the breakpoint."));
	set_breakpoint_condition (b, p, from_tty);
	return;
      }

  error (_("No breakpoint number %d."), bnum);
}

/* Check that COMMAND do not contain commands that are suitable
   only for tracepoints and not suitable for ordinary breakpoints.
   Throw if any such commands is found.  */

static void
check_no_tracepoint_commands (struct command_line *commands)
{
  struct command_line *c;

  for (c = commands; c; c = c->next)
    {
      int i;

      if (c->control_type == while_stepping_control)
	error (_("The 'while-stepping' command can "
		 "only be used for tracepoints"));

      for (i = 0; i < c->body_count; ++i)
	check_no_tracepoint_commands ((c->body_list)[i]);

      /* Not that command parsing removes leading whitespace and comment
	 lines and also empty lines.  So, we only need to check for
	 command directly.  */
      if (strstr (c->line, "collect ") == c->line)
	error (_("The 'collect' command can only be used for tracepoints"));

      if (strstr (c->line, "teval ") == c->line)
	error (_("The 'teval' command can only be used for tracepoints"));
    }
}

/* Encapsulate tests for different types of tracepoints.  */

int
is_tracepoint (const struct breakpoint *b)
{
  return (b->type == bp_tracepoint
	  || b->type == bp_fast_tracepoint
	  || b->type == bp_static_tracepoint);
}
  
/* A helper function that validsates that COMMANDS are valid for a
   breakpoint.  This function will throw an exception if a problem is
   found.  */

static void
validate_commands_for_breakpoint (struct breakpoint *b,
				  struct command_line *commands)
{
  if (is_tracepoint (b))
    {
      /* We need to verify that each top-level element of commands is
	 valid for tracepoints, that there's at most one
	 while-stepping element, and that while-stepping's body has
	 valid tracing commands excluding nested while-stepping.  */
      struct command_line *c;
      struct command_line *while_stepping = 0;
      for (c = commands; c; c = c->next)
	{
	  if (c->control_type == while_stepping_control)
	    {
	      if (b->type == bp_fast_tracepoint)
		error (_("The 'while-stepping' command "
			 "cannot be used for fast tracepoint"));
	      else if (b->type == bp_static_tracepoint)
		error (_("The 'while-stepping' command "
			 "cannot be used for static tracepoint"));

	      if (while_stepping)
		error (_("The 'while-stepping' command "
			 "can be used only once"));
	      else
		while_stepping = c;
	    }
	}
      if (while_stepping)
	{
	  struct command_line *c2;

	  gdb_assert (while_stepping->body_count == 1);
	  c2 = while_stepping->body_list[0];
	  for (; c2; c2 = c2->next)
	    {
	      if (c2->control_type == while_stepping_control)
		error (_("The 'while-stepping' command cannot be nested"));
	    }
	}
    }
  else
    {
      check_no_tracepoint_commands (commands);
    }
}

/* Return a vector of all the static tracepoints set at ADDR.  The
   caller is responsible for releasing the vector.  */

VEC(breakpoint_p) *
static_tracepoints_here (CORE_ADDR addr)
{
  struct breakpoint *b;
  VEC(breakpoint_p) *found = 0;
  struct bp_location *loc;

  ALL_BREAKPOINTS (b)
    if (b->type == bp_static_tracepoint)
      {
	for (loc = b->loc; loc; loc = loc->next)
	  if (loc->address == addr)
	    VEC_safe_push(breakpoint_p, found, b);
      }

  return found;
}

/* Set the command list of B to COMMANDS.  If breakpoint is tracepoint,
   validate that only allowed commands are included.  */

void
breakpoint_set_commands (struct breakpoint *b, 
			 struct command_line *commands)
{
  validate_commands_for_breakpoint (b, commands);

  decref_counted_command_line (&b->commands);
  b->commands = alloc_counted_command_line (commands);
  breakpoints_changed ();
  observer_notify_breakpoint_modified (b->number);
}

/* Set the internal `silent' flag on the breakpoint.  Note that this
   is not the same as the "silent" that may appear in the breakpoint's
   commands.  */

void
breakpoint_set_silent (struct breakpoint *b, int silent)
{
  int old_silent = b->silent;

  b->silent = silent;
  if (old_silent != silent)
    observer_notify_breakpoint_modified (b->number);
}

/* Set the thread for this breakpoint.  If THREAD is -1, make the
   breakpoint work for any thread.  */

void
breakpoint_set_thread (struct breakpoint *b, int thread)
{
  int old_thread = b->thread;

  b->thread = thread;
  if (old_thread != thread)
    observer_notify_breakpoint_modified (b->number);
}

/* Set the task for this breakpoint.  If TASK is 0, make the
   breakpoint work for any task.  */

void
breakpoint_set_task (struct breakpoint *b, int task)
{
  int old_task = b->task;

  b->task = task;
  if (old_task != task)
    observer_notify_breakpoint_modified (b->number);
}

void
check_tracepoint_command (char *line, void *closure)
{
  struct breakpoint *b = closure;

  validate_actionline (&line, b);
}

/* A structure used to pass information through
   map_breakpoint_numbers.  */

struct commands_info
{
  /* True if the command was typed at a tty.  */
  int from_tty;

  /* The breakpoint range spec.  */
  char *arg;

  /* Non-NULL if the body of the commands are being read from this
     already-parsed command.  */
  struct command_line *control;

  /* The command lines read from the user, or NULL if they have not
     yet been read.  */
  struct counted_command_line *cmd;
};

/* A callback for map_breakpoint_numbers that sets the commands for
   commands_command.  */

static void
do_map_commands_command (struct breakpoint *b, void *data)
{
  struct commands_info *info = data;

  if (info->cmd == NULL)
    {
      struct command_line *l;

      if (info->control != NULL)
	l = copy_command_lines (info->control->body_list[0]);
      else
	{
	  struct cleanup *old_chain;
	  char *str;

	  str = xstrprintf (_("Type commands for breakpoint(s) "
			      "%s, one per line."),
			    info->arg);

	  old_chain = make_cleanup (xfree, str);

	  l = read_command_lines (str,
				  info->from_tty, 1,
				  (is_tracepoint (b)
				   ? check_tracepoint_command : 0),
				  b);

	  do_cleanups (old_chain);
	}

      info->cmd = alloc_counted_command_line (l);
    }

  /* If a breakpoint was on the list more than once, we don't need to
     do anything.  */
  if (b->commands != info->cmd)
    {
      validate_commands_for_breakpoint (b, info->cmd->commands);
      incref_counted_command_line (info->cmd);
      decref_counted_command_line (&b->commands);
      b->commands = info->cmd;
      breakpoints_changed ();
      observer_notify_breakpoint_modified (b->number);
    }
}

static void
commands_command_1 (char *arg, int from_tty, 
		    struct command_line *control)
{
  struct cleanup *cleanups;
  struct commands_info info;

  info.from_tty = from_tty;
  info.control = control;
  info.cmd = NULL;
  /* If we read command lines from the user, then `info' will hold an
     extra reference to the commands that we must clean up.  */
  cleanups = make_cleanup_decref_counted_command_line (&info.cmd);

  if (arg == NULL || !*arg)
    {
      if (breakpoint_count - prev_breakpoint_count > 1)
	arg = xstrprintf ("%d-%d", prev_breakpoint_count + 1, 
			  breakpoint_count);
      else if (breakpoint_count > 0)
	arg = xstrprintf ("%d", breakpoint_count);
      else
	{
	  /* So that we don't try to free the incoming non-NULL
	     argument in the cleanup below.  Mapping breakpoint
	     numbers will fail in this case.  */
	  arg = NULL;
	}
    }
  else
    /* The command loop has some static state, so we need to preserve
       our argument.  */
    arg = xstrdup (arg);

  if (arg != NULL)
    make_cleanup (xfree, arg);

  info.arg = arg;

  map_breakpoint_numbers (arg, do_map_commands_command, &info);

  if (info.cmd == NULL)
    error (_("No breakpoints specified."));

  do_cleanups (cleanups);
}

static void
commands_command (char *arg, int from_tty)
{
  commands_command_1 (arg, from_tty, NULL);
}

/* Like commands_command, but instead of reading the commands from
   input stream, takes them from an already parsed command structure.

   This is used by cli-script.c to DTRT with breakpoint commands
   that are part of if and while bodies.  */
enum command_control_type
commands_from_control_command (char *arg, struct command_line *cmd)
{
  commands_command_1 (arg, 0, cmd);
  return simple_control;
}

/* Return non-zero if BL->TARGET_INFO contains valid information.  */

static int
bp_location_has_shadow (struct bp_location *bl)
{
  if (bl->loc_type != bp_loc_software_breakpoint)
    return 0;
  if (!bl->inserted)
    return 0;
  if (bl->target_info.shadow_len == 0)
    /* bp isn't valid, or doesn't shadow memory.  */
    return 0;
  return 1;
}

/* Update BUF, which is LEN bytes read from the target address MEMADDR,
   by replacing any memory breakpoints with their shadowed contents.

   The range of shadowed area by each bp_location is:
     bl->address - bp_location_placed_address_before_address_max
     up to bl->address + bp_location_shadow_len_after_address_max
   The range we were requested to resolve shadows for is:
     memaddr ... memaddr + len
   Thus the safe cutoff boundaries for performance optimization are
     memaddr + len <= (bl->address
		       - bp_location_placed_address_before_address_max)
   and:
     bl->address + bp_location_shadow_len_after_address_max <= memaddr  */

void
breakpoint_restore_shadows (gdb_byte *buf, ULONGEST memaddr, LONGEST len)
{
  /* Left boundary, right boundary and median element of our binary
     search.  */
  unsigned bc_l, bc_r, bc;

  /* Find BC_L which is a leftmost element which may affect BUF
     content.  It is safe to report lower value but a failure to
     report higher one.  */

  bc_l = 0;
  bc_r = bp_location_count;
  while (bc_l + 1 < bc_r)
    {
      struct bp_location *bl;

      bc = (bc_l + bc_r) / 2;
      bl = bp_location[bc];

      /* Check first BL->ADDRESS will not overflow due to the added
	 constant.  Then advance the left boundary only if we are sure
	 the BC element can in no way affect the BUF content (MEMADDR
	 to MEMADDR + LEN range).

	 Use the BP_LOCATION_SHADOW_LEN_AFTER_ADDRESS_MAX safety
	 offset so that we cannot miss a breakpoint with its shadow
	 range tail still reaching MEMADDR.  */

      if ((bl->address + bp_location_shadow_len_after_address_max
	   >= bl->address)
	  && (bl->address + bp_location_shadow_len_after_address_max
	      <= memaddr))
	bc_l = bc;
      else
	bc_r = bc;
    }

  /* Due to the binary search above, we need to make sure we pick the
     first location that's at BC_L's address.  E.g., if there are
     multiple locations at the same address, BC_L may end up pointing
     at a duplicate location, and miss the "master"/"inserted"
     location.  Say, given locations L1, L2 and L3 at addresses A and
     B:

      L1@A, L2@A, L3@B, ...

     BC_L could end up pointing at location L2, while the "master"
     location could be L1.  Since the `loc->inserted' flag is only set
     on "master" locations, we'd forget to restore the shadow of L1
     and L2.  */
  while (bc_l > 0
	 && bp_location[bc_l]->address == bp_location[bc_l - 1]->address)
    bc_l--;

  /* Now do full processing of the found relevant range of elements.  */

  for (bc = bc_l; bc < bp_location_count; bc++)
  {
    struct bp_location *bl = bp_location[bc];
    CORE_ADDR bp_addr = 0;
    int bp_size = 0;
    int bptoffset = 0;

    /* bp_location array has BL->OWNER always non-NULL.  */
    if (bl->owner->type == bp_none)
      warning (_("reading through apparently deleted breakpoint #%d?"),
	       bl->owner->number);

    /* Performance optimization: any futher element can no longer affect BUF
       content.  */

    if (bl->address >= bp_location_placed_address_before_address_max
        && memaddr + len <= (bl->address
			     - bp_location_placed_address_before_address_max))
      break;

    if (!bp_location_has_shadow (bl))
      continue;
    if (!breakpoint_address_match (bl->target_info.placed_address_space, 0,
				   current_program_space->aspace, 0))
      continue;

    /* Addresses and length of the part of the breakpoint that
       we need to copy.  */
    bp_addr = bl->target_info.placed_address;
    bp_size = bl->target_info.shadow_len;

    if (bp_addr + bp_size <= memaddr)
      /* The breakpoint is entirely before the chunk of memory we
         are reading.  */
      continue;

    if (bp_addr >= memaddr + len)
      /* The breakpoint is entirely after the chunk of memory we are
         reading.  */
      continue;

    /* Offset within shadow_contents.  */
    if (bp_addr < memaddr)
      {
	/* Only copy the second part of the breakpoint.  */
	bp_size -= memaddr - bp_addr;
	bptoffset = memaddr - bp_addr;
	bp_addr = memaddr;
      }

    if (bp_addr + bp_size > memaddr + len)
      {
	/* Only copy the first part of the breakpoint.  */
	bp_size -= (bp_addr + bp_size) - (memaddr + len);
      }

    memcpy (buf + bp_addr - memaddr,
	    bl->target_info.shadow_contents + bptoffset, bp_size);
  }
}


/* Return true if BPT is of any hardware watchpoint kind.  */

static int
is_hardware_watchpoint (const struct breakpoint *bpt)
{
  return (bpt->type == bp_hardware_watchpoint
	  || bpt->type == bp_read_watchpoint
	  || bpt->type == bp_access_watchpoint);
}

/* Return true if BPT is of any watchpoint kind, hardware or
   software.  */

static int
is_watchpoint (const struct breakpoint *bpt)
{
  return (is_hardware_watchpoint (bpt)
	  || bpt->type == bp_watchpoint);
}

/* Assuming that B is a watchpoint: returns true if the current thread
   and its running state are safe to evaluate or update watchpoint B.
   Watchpoints on local expressions need to be evaluated in the
   context of the thread that was current when the watchpoint was
   created, and, that thread needs to be stopped to be able to select
   the correct frame context.  Watchpoints on global expressions can
   be evaluated on any thread, and in any state.  It is presently left
   to the target allowing memory accesses when threads are
   running.  */

static int
watchpoint_in_thread_scope (struct breakpoint *b)
{
  return (ptid_equal (b->watchpoint_thread, null_ptid)
	  || (ptid_equal (inferior_ptid, b->watchpoint_thread)
	      && !is_executing (inferior_ptid)));
}

/* Set watchpoint B to disp_del_at_next_stop, even including its possible
   associated bp_watchpoint_scope breakpoint.  */

static void
watchpoint_del_at_next_stop (struct breakpoint *b)
{
  gdb_assert (is_watchpoint (b));

  if (b->related_breakpoint != b)
    {
      gdb_assert (b->related_breakpoint->type == bp_watchpoint_scope);
      gdb_assert (b->related_breakpoint->related_breakpoint == b);
      b->related_breakpoint->disposition = disp_del_at_next_stop;
      b->related_breakpoint->related_breakpoint = b->related_breakpoint;
      b->related_breakpoint = b;
    }
  b->disposition = disp_del_at_next_stop;
}

/* Assuming that B is a watchpoint:
   - Reparse watchpoint expression, if REPARSE is non-zero
   - Evaluate expression and store the result in B->val
   - Evaluate the condition if there is one, and store the result
     in b->loc->cond.
   - Update the list of values that must be watched in B->loc.

   If the watchpoint disposition is disp_del_at_next_stop, then do
   nothing.  If this is local watchpoint that is out of scope, delete
   it.

   Even with `set breakpoint always-inserted on' the watchpoints are
   removed + inserted on each stop here.  Normal breakpoints must
   never be removed because they might be missed by a running thread
   when debugging in non-stop mode.  On the other hand, hardware
   watchpoints (is_hardware_watchpoint; processed here) are specific
   to each LWP since they are stored in each LWP's hardware debug
   registers.  Therefore, such LWP must be stopped first in order to
   be able to modify its hardware watchpoints.

   Hardware watchpoints must be reset exactly once after being
   presented to the user.  It cannot be done sooner, because it would
   reset the data used to present the watchpoint hit to the user.  And
   it must not be done later because it could display the same single
   watchpoint hit during multiple GDB stops.  Note that the latter is
   relevant only to the hardware watchpoint types bp_read_watchpoint
   and bp_access_watchpoint.  False hit by bp_hardware_watchpoint is
   not user-visible - its hit is suppressed if the memory content has
   not changed.

   The following constraints influence the location where we can reset
   hardware watchpoints:

   * target_stopped_by_watchpoint and target_stopped_data_address are
     called several times when GDB stops.

   [linux] 
   * Multiple hardware watchpoints can be hit at the same time,
     causing GDB to stop.  GDB only presents one hardware watchpoint
     hit at a time as the reason for stopping, and all the other hits
     are presented later, one after the other, each time the user
     requests the execution to be resumed.  Execution is not resumed
     for the threads still having pending hit event stored in
     LWP_INFO->STATUS.  While the watchpoint is already removed from
     the inferior on the first stop the thread hit event is kept being
     reported from its cached value by linux_nat_stopped_data_address
     until the real thread resume happens after the watchpoint gets
     presented and thus its LWP_INFO->STATUS gets reset.

   Therefore the hardware watchpoint hit can get safely reset on the
   watchpoint removal from inferior.  */

static void
update_watchpoint (struct breakpoint *b, int reparse)
{
  int within_current_scope;
  struct frame_id saved_frame_id;
  int frame_saved;

  gdb_assert (is_watchpoint (b));

  /* If this is a local watchpoint, we only want to check if the
     watchpoint frame is in scope if the current thread is the thread
     that was used to create the watchpoint.  */
  if (!watchpoint_in_thread_scope (b))
    return;

  if (b->disposition == disp_del_at_next_stop)
    return;
 
  frame_saved = 0;

  /* Determine if the watchpoint is within scope.  */
  if (b->exp_valid_block == NULL)
    within_current_scope = 1;
  else
    {
      struct frame_info *fi = get_current_frame ();
      struct gdbarch *frame_arch = get_frame_arch (fi);
      CORE_ADDR frame_pc = get_frame_pc (fi);

      /* If we're in a function epilogue, unwinding may not work
	 properly, so do not attempt to recreate locations at this
	 point.  See similar comments in watchpoint_check.  */
      if (gdbarch_in_function_epilogue_p (frame_arch, frame_pc))
	return;

      /* Save the current frame's ID so we can restore it after
         evaluating the watchpoint expression on its own frame.  */
      /* FIXME drow/2003-09-09: It would be nice if evaluate_expression
         took a frame parameter, so that we didn't have to change the
         selected frame.  */
      frame_saved = 1;
      saved_frame_id = get_frame_id (get_selected_frame (NULL));

      fi = frame_find_by_id (b->watchpoint_frame);
      within_current_scope = (fi != NULL);
      if (within_current_scope)
	select_frame (fi);
    }

  /* We don't free locations.  They are stored in the bp_location array
     and update_global_location_list will eventually delete them and
     remove breakpoints if needed.  */
  b->loc = NULL;

  if (within_current_scope && reparse)
    {
      char *s;

      if (b->exp)
	{
	  xfree (b->exp);
	  b->exp = NULL;
	}
      s = b->exp_string_reparse ? b->exp_string_reparse : b->exp_string;
      b->exp = parse_exp_1 (&s, b->exp_valid_block, 0);
      /* If the meaning of expression itself changed, the old value is
	 no longer relevant.  We don't want to report a watchpoint hit
	 to the user when the old value and the new value may actually
	 be completely different objects.  */
      value_free (b->val);
      b->val = NULL;
      b->val_valid = 0;

      /* Note that unlike with breakpoints, the watchpoint's condition
	 expression is stored in the breakpoint object, not in the
	 locations (re)created below.  */
      if (b->cond_string != NULL)
	{
	  if (b->cond_exp != NULL)
	    {
	      xfree (b->cond_exp);
	      b->cond_exp = NULL;
	    }

	  s = b->cond_string;
	  b->cond_exp = parse_exp_1 (&s, b->cond_exp_valid_block, 0);
	}
    }

  /* If we failed to parse the expression, for example because
     it refers to a global variable in a not-yet-loaded shared library,
     don't try to insert watchpoint.  We don't automatically delete
     such watchpoint, though, since failure to parse expression
     is different from out-of-scope watchpoint.  */
  if ( !target_has_execution)
    {
      /* Without execution, memory can't change.  No use to try and
	 set watchpoint locations.  The watchpoint will be reset when
	 the target gains execution, through breakpoint_re_set.  */
    }
  else if (within_current_scope && b->exp)
    {
      int pc = 0;
      struct value *val_chain, *v, *result, *next;
      struct program_space *frame_pspace;

      fetch_subexp_value (b->exp, &pc, &v, &result, &val_chain);

      /* Avoid setting b->val if it's already set.  The meaning of
	 b->val is 'the last value' user saw, and we should update
	 it only if we reported that last value to user.  As it
	 happens, the code that reports it updates b->val directly.  */
      if (!b->val_valid)
	{
	  b->val = v;
	  b->val_valid = 1;
	}

      frame_pspace = get_frame_program_space (get_selected_frame (NULL));

      /* Look at each value on the value chain.  */
      for (v = val_chain; v; v = value_next (v))
	{
	  /* If it's a memory location, and GDB actually needed
	     its contents to evaluate the expression, then we
	     must watch it.  If the first value returned is
	     still lazy, that means an error occurred reading it;
	     watch it anyway in case it becomes readable.  */
	  if (VALUE_LVAL (v) == lval_memory
	      && (v == val_chain || ! value_lazy (v)))
	    {
	      struct type *vtype = check_typedef (value_type (v));

	      /* We only watch structs and arrays if user asked
		 for it explicitly, never if they just happen to
		 appear in the middle of some value chain.  */
	      if (v == result
		  || (TYPE_CODE (vtype) != TYPE_CODE_STRUCT
		      && TYPE_CODE (vtype) != TYPE_CODE_ARRAY))
		{
		  CORE_ADDR addr;
		  int len, type;
		  struct bp_location *loc, **tmp;

		  addr = value_address (v);
		  len = TYPE_LENGTH (value_type (v));
		  type = hw_write;
		  if (b->type == bp_read_watchpoint)
		    type = hw_read;
		  else if (b->type == bp_access_watchpoint)
		    type = hw_access;
		  
		  loc = allocate_bp_location (b);
		  for (tmp = &(b->loc); *tmp != NULL; tmp = &((*tmp)->next))
		    ;
		  *tmp = loc;
		  loc->gdbarch = get_type_arch (value_type (v));

		  loc->pspace = frame_pspace;
		  loc->address = addr;
		  loc->length = len;
		  loc->watchpoint_type = type;
		}
	    }
	}

      /* Change the type of breakpoint between hardware assisted or
	 an ordinary watchpoint depending on the hardware support
	 and free hardware slots.  REPARSE is set when the inferior
	 is started.  */
      if ((b->type == bp_watchpoint || b->type == bp_hardware_watchpoint)
	  && reparse)
	{
	  int reg_cnt;
	  enum bp_loc_type loc_type;
	  struct bp_location *bl;

	  reg_cnt = can_use_hardware_watchpoint (val_chain, b->exact);

	  if (reg_cnt)
	    {
	      int i, target_resources_ok, other_type_used;
	      enum enable_state orig_enable_state;

	      /* We need to determine how many resources are already
		 used for all other hardware watchpoints plus this one
		 to see if we still have enough resources to also fit
		 this watchpoint in as well.  To guarantee the
		 hw_watchpoint_used_count call below counts this
		 watchpoint, make sure that it is marked as a hardware
		 watchpoint.  */
	      b->type = bp_hardware_watchpoint;

	      /* hw_watchpoint_used_count ignores disabled watchpoints,
		 and b might be disabled if we're being called from
		 do_enable_breakpoint.  */
	      orig_enable_state = b->enable_state;
	      b->enable_state = bp_enabled;

	      i = hw_watchpoint_used_count (bp_hardware_watchpoint,
					    &other_type_used);

	      b->enable_state = orig_enable_state;

	      target_resources_ok = target_can_use_hardware_watchpoint
		    (bp_hardware_watchpoint, i, other_type_used);
	      if (target_resources_ok <= 0)
		b->type = bp_watchpoint;
	    }
	  else
	    b->type = bp_watchpoint;

	  loc_type = (b->type == bp_watchpoint? bp_loc_other
		      : bp_loc_hardware_watchpoint);
	  for (bl = b->loc; bl; bl = bl->next)
	    bl->loc_type = loc_type;
	}

      for (v = val_chain; v; v = next)
	{
	  next = value_next (v);
	  if (v != b->val)
	    value_free (v);
	}

      /* If a software watchpoint is not watching any memory, then the
	 above left it without any location set up.  But,
	 bpstat_stop_status requires a location to be able to report
	 stops, so make sure there's at least a dummy one.  */
      if (b->type == bp_watchpoint && b->loc == NULL)
	{
	  b->loc = allocate_bp_location (b);
	  b->loc->pspace = frame_pspace;
	  b->loc->address = -1;
	  b->loc->length = -1;
	  b->loc->watchpoint_type = -1;
	}
    }
  else if (!within_current_scope)
    {
      printf_filtered (_("\
Watchpoint %d deleted because the program has left the block\n\
in which its expression is valid.\n"),
		       b->number);
      watchpoint_del_at_next_stop (b);
    }

  /* Restore the selected frame.  */
  if (frame_saved)
    select_frame (frame_find_by_id (saved_frame_id));
}


/* Returns 1 iff breakpoint location should be
   inserted in the inferior.  */
static int
should_be_inserted (struct bp_location *bl)
{
  if (bl->owner == NULL || !breakpoint_enabled (bl->owner))
    return 0;

  if (bl->owner->disposition == disp_del_at_next_stop)
    return 0;

  if (!bl->enabled || bl->shlib_disabled || bl->duplicate)
    return 0;

  /* This is set for example, when we're attached to the parent of a
     vfork, and have detached from the child.  The child is running
     free, and we expect it to do an exec or exit, at which point the
     OS makes the parent schedulable again (and the target reports
     that the vfork is done).  Until the child is done with the shared
     memory region, do not insert breakpoints in the parent, otherwise
     the child could still trip on the parent's breakpoints.  Since
     the parent is blocked anyway, it won't miss any breakpoint.  */
  if (bl->pspace->breakpoints_not_allowed)
    return 0;

  /* Tracepoints are inserted by the target at a time of its choosing,
     not by us.  */
  if (is_tracepoint (bl->owner))
    return 0;

  return 1;
}

/* Insert a low-level "breakpoint" of some type.  BL is the breakpoint
   location.  Any error messages are printed to TMP_ERROR_STREAM; and
   DISABLED_BREAKS, and HW_BREAKPOINT_ERROR are used to report problems.

   NOTE drow/2003-09-09: This routine could be broken down to an
   object-style method for each breakpoint or catchpoint type.  */
static int
insert_bp_location (struct bp_location *bl,
		    struct ui_file *tmp_error_stream,
		    int *disabled_breaks,
		    int *hw_breakpoint_error)
{
  int val = 0;

  if (!should_be_inserted (bl) || bl->inserted)
    return 0;

  /* Initialize the target-specific information.  */
  memset (&bl->target_info, 0, sizeof (bl->target_info));
  bl->target_info.placed_address = bl->address;
  bl->target_info.placed_address_space = bl->pspace->aspace;
  bl->target_info.length = bl->length;

  if (bl->loc_type == bp_loc_software_breakpoint
      || bl->loc_type == bp_loc_hardware_breakpoint)
    {
      if (bl->owner->type != bp_hardware_breakpoint)
	{
	  /* If the explicitly specified breakpoint type
	     is not hardware breakpoint, check the memory map to see
	     if the breakpoint address is in read only memory or not.

	     Two important cases are:
	     - location type is not hardware breakpoint, memory
	     is readonly.  We change the type of the location to
	     hardware breakpoint.
	     - location type is hardware breakpoint, memory is
	     read-write.  This means we've previously made the
	     location hardware one, but then the memory map changed,
	     so we undo.
	     
	     When breakpoints are removed, remove_breakpoints will use
	     location types we've just set here, the only possible
	     problem is that memory map has changed during running
	     program, but it's not going to work anyway with current
	     gdb.  */
	  struct mem_region *mr 
	    = lookup_mem_region (bl->target_info.placed_address);
	  
	  if (mr)
	    {
	      if (automatic_hardware_breakpoints)
		{
		  enum bp_loc_type new_type;
		  
		  if (mr->attrib.mode != MEM_RW)
		    new_type = bp_loc_hardware_breakpoint;
		  else 
		    new_type = bp_loc_software_breakpoint;
		  
		  if (new_type != bl->loc_type)
		    {
		      static int said = 0;

		      bl->loc_type = new_type;
		      if (!said)
			{
			  fprintf_filtered (gdb_stdout,
					    _("Note: automatically using "
					      "hardware breakpoints for "
					      "read-only addresses.\n"));
			  said = 1;
			}
		    }
		}
	      else if (bl->loc_type == bp_loc_software_breakpoint
		       && mr->attrib.mode != MEM_RW)	    
		warning (_("cannot set software breakpoint "
			   "at readonly address %s"),
			 paddress (bl->gdbarch, bl->address));
	    }
	}
        
      /* First check to see if we have to handle an overlay.  */
      if (overlay_debugging == ovly_off
	  || bl->section == NULL
	  || !(section_is_overlay (bl->section)))
	{
	  /* No overlay handling: just set the breakpoint.  */

	  if (bl->loc_type == bp_loc_hardware_breakpoint)
	    val = target_insert_hw_breakpoint (bl->gdbarch,
					       &bl->target_info);
	  else
	    val = target_insert_breakpoint (bl->gdbarch,
					    &bl->target_info);
	}
      else
	{
	  /* This breakpoint is in an overlay section.
	     Shall we set a breakpoint at the LMA?  */
	  if (!overlay_events_enabled)
	    {
	      /* Yes -- overlay event support is not active, 
		 so we must try to set a breakpoint at the LMA.
		 This will not work for a hardware breakpoint.  */
	      if (bl->loc_type == bp_loc_hardware_breakpoint)
		warning (_("hardware breakpoint %d not supported in overlay!"),
			 bl->owner->number);
	      else
		{
		  CORE_ADDR addr = overlay_unmapped_address (bl->address,
							     bl->section);
		  /* Set a software (trap) breakpoint at the LMA.  */
		  bl->overlay_target_info = bl->target_info;
		  bl->overlay_target_info.placed_address = addr;
		  val = target_insert_breakpoint (bl->gdbarch,
						  &bl->overlay_target_info);
		  if (val != 0)
		    fprintf_unfiltered (tmp_error_stream,
					"Overlay breakpoint %d "
					"failed: in ROM?\n",
					bl->owner->number);
		}
	    }
	  /* Shall we set a breakpoint at the VMA? */
	  if (section_is_mapped (bl->section))
	    {
	      /* Yes.  This overlay section is mapped into memory.  */
	      if (bl->loc_type == bp_loc_hardware_breakpoint)
		val = target_insert_hw_breakpoint (bl->gdbarch,
						   &bl->target_info);
	      else
		val = target_insert_breakpoint (bl->gdbarch,
						&bl->target_info);
	    }
	  else
	    {
	      /* No.  This breakpoint will not be inserted.  
		 No error, but do not mark the bp as 'inserted'.  */
	      return 0;
	    }
	}

      if (val)
	{
	  /* Can't set the breakpoint.  */
	  if (solib_name_from_address (bl->pspace, bl->address))
	    {
	      /* See also: disable_breakpoints_in_shlibs.  */
	      val = 0;
	      bl->shlib_disabled = 1;
	      if (!*disabled_breaks)
		{
		  fprintf_unfiltered (tmp_error_stream, 
				      "Cannot insert breakpoint %d.\n", 
				      bl->owner->number);
		  fprintf_unfiltered (tmp_error_stream, 
				      "Temporarily disabling shared "
				      "library breakpoints:\n");
		}
	      *disabled_breaks = 1;
	      fprintf_unfiltered (tmp_error_stream,
				  "breakpoint #%d\n", bl->owner->number);
	    }
	  else
	    {
	      if (bl->loc_type == bp_loc_hardware_breakpoint)
		{
		  *hw_breakpoint_error = 1;
		  fprintf_unfiltered (tmp_error_stream,
				      "Cannot insert hardware "
				      "breakpoint %d.\n",
				      bl->owner->number);
		}
	      else
		{
		  fprintf_unfiltered (tmp_error_stream, 
				      "Cannot insert breakpoint %d.\n", 
				      bl->owner->number);
		  fprintf_filtered (tmp_error_stream, 
				    "Error accessing memory address ");
		  fputs_filtered (paddress (bl->gdbarch, bl->address),
				  tmp_error_stream);
		  fprintf_filtered (tmp_error_stream, ": %s.\n",
				    safe_strerror (val));
		}

	    }
	}
      else
	bl->inserted = 1;

      return val;
    }

  else if (bl->loc_type == bp_loc_hardware_watchpoint
	   /* NOTE drow/2003-09-08: This state only exists for removing
	      watchpoints.  It's not clear that it's necessary...  */
	   && bl->owner->disposition != disp_del_at_next_stop)
    {
      gdb_assert (bl->owner->ops != NULL
		  && bl->owner->ops->insert_location != NULL);

      val = bl->owner->ops->insert_location (bl);

      /* If trying to set a read-watchpoint, and it turns out it's not
	 supported, try emulating one with an access watchpoint.  */
      if (val == 1 && bl->watchpoint_type == hw_read)
	{
	  struct bp_location *loc, **loc_temp;

	  /* But don't try to insert it, if there's already another
	     hw_access location that would be considered a duplicate
	     of this one.  */
	  ALL_BP_LOCATIONS (loc, loc_temp)
	    if (loc != bl
		&& loc->watchpoint_type == hw_access
		&& watchpoint_locations_match (bl, loc))
	      {
		bl->duplicate = 1;
		bl->inserted = 1;
		bl->target_info = loc->target_info;
		bl->watchpoint_type = hw_access;
		val = 0;
		break;
	      }

	  if (val == 1)
	    {
	      bl->watchpoint_type = hw_access;
	      val = bl->owner->ops->insert_location (bl);

	      if (val)
		/* Back to the original value.  */
		bl->watchpoint_type = hw_read;
	    }
	}

      bl->inserted = (val == 0);
    }

  else if (bl->owner->type == bp_catchpoint)
    {
      gdb_assert (bl->owner->ops != NULL
		  && bl->owner->ops->insert_location != NULL);

      val = bl->owner->ops->insert_location (bl);
      if (val)
	{
	  bl->owner->enable_state = bp_disabled;

	  if (val == 1)
	    warning (_("\
Error inserting catchpoint %d: Your system does not support this type\n\
of catchpoint."), bl->owner->number);
	  else
	    warning (_("Error inserting catchpoint %d."), bl->owner->number);
	}

      bl->inserted = (val == 0);

      /* We've already printed an error message if there was a problem
	 inserting this catchpoint, and we've disabled the catchpoint,
	 so just return success.  */
      return 0;
    }

  return 0;
}

/* This function is called when program space PSPACE is about to be
   deleted.  It takes care of updating breakpoints to not reference
   PSPACE anymore.  */

void
breakpoint_program_space_exit (struct program_space *pspace)
{
  struct breakpoint *b, *b_temp;
  struct bp_location *loc, **loc_temp;

  /* Remove any breakpoint that was set through this program space.  */
  ALL_BREAKPOINTS_SAFE (b, b_temp)
    {
      if (b->pspace == pspace)
	delete_breakpoint (b);
    }

  /* Breakpoints set through other program spaces could have locations
     bound to PSPACE as well.  Remove those.  */
  ALL_BP_LOCATIONS (loc, loc_temp)
    {
      struct bp_location *tmp;

      if (loc->pspace == pspace)
	{
	  /* ALL_BP_LOCATIONS bp_location has LOC->OWNER always non-NULL.  */
	  if (loc->owner->loc == loc)
	    loc->owner->loc = loc->next;
	  else
	    for (tmp = loc->owner->loc; tmp->next != NULL; tmp = tmp->next)
	      if (tmp->next == loc)
		{
		  tmp->next = loc->next;
		  break;
		}
	}
    }

  /* Now update the global location list to permanently delete the
     removed locations above.  */
  update_global_location_list (0);
}

/* Make sure all breakpoints are inserted in inferior.
   Throws exception on any error.
   A breakpoint that is already inserted won't be inserted
   again, so calling this function twice is safe.  */
void
insert_breakpoints (void)
{
  struct breakpoint *bpt;

  ALL_BREAKPOINTS (bpt)
    if (is_hardware_watchpoint (bpt))
      update_watchpoint (bpt, 0 /* don't reparse.  */);

  update_global_location_list (1);

  /* update_global_location_list does not insert breakpoints when
     always_inserted_mode is not enabled.  Explicitly insert them
     now.  */
  if (!breakpoints_always_inserted_mode ())
    insert_breakpoint_locations ();
}

/* insert_breakpoints is used when starting or continuing the program.
   remove_breakpoints is used when the program stops.
   Both return zero if successful,
   or an `errno' value if could not write the inferior.  */

static void
insert_breakpoint_locations (void)
{
  struct breakpoint *bpt;
  struct bp_location *bl, **blp_tmp;
  int error = 0;
  int val = 0;
  int disabled_breaks = 0;
  int hw_breakpoint_error = 0;

  struct ui_file *tmp_error_stream = mem_fileopen ();
  struct cleanup *cleanups = make_cleanup_ui_file_delete (tmp_error_stream);
  
  /* Explicitly mark the warning -- this will only be printed if
     there was an error.  */
  fprintf_unfiltered (tmp_error_stream, "Warning:\n");

  save_current_space_and_thread ();

  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      if (!should_be_inserted (bl) || bl->inserted)
	continue;

      /* There is no point inserting thread-specific breakpoints if
	 the thread no longer exists.  ALL_BP_LOCATIONS bp_location
	 has BL->OWNER always non-NULL.  */
      if (bl->owner->thread != -1
	  && !valid_thread_id (bl->owner->thread))
	continue;

      switch_to_program_space_and_thread (bl->pspace);

      /* For targets that support global breakpoints, there's no need
	 to select an inferior to insert breakpoint to.  In fact, even
	 if we aren't attached to any process yet, we should still
	 insert breakpoints.  */
      if (!gdbarch_has_global_breakpoints (target_gdbarch)
	  && ptid_equal (inferior_ptid, null_ptid))
	continue;

      val = insert_bp_location (bl, tmp_error_stream, &disabled_breaks,
				    &hw_breakpoint_error);
      if (val)
	error = val;
    }

  /* If we failed to insert all locations of a watchpoint, remove
     them, as half-inserted watchpoint is of limited use.  */
  ALL_BREAKPOINTS (bpt)  
    {
      int some_failed = 0;
      struct bp_location *loc;

      if (!is_hardware_watchpoint (bpt))
	continue;

      if (!breakpoint_enabled (bpt))
	continue;

      if (bpt->disposition == disp_del_at_next_stop)
	continue;
      
      for (loc = bpt->loc; loc; loc = loc->next)
	if (!loc->inserted && should_be_inserted (loc))
	  {
	    some_failed = 1;
	    break;
	  }
      if (some_failed)
	{
	  for (loc = bpt->loc; loc; loc = loc->next)
	    if (loc->inserted)
	      remove_breakpoint (loc, mark_uninserted);

	  hw_breakpoint_error = 1;
	  fprintf_unfiltered (tmp_error_stream,
			      "Could not insert hardware watchpoint %d.\n", 
			      bpt->number);
	  error = -1;
	}
    }

  if (error)
    {
      /* If a hardware breakpoint or watchpoint was inserted, add a
         message about possibly exhausted resources.  */
      if (hw_breakpoint_error)
	{
	  fprintf_unfiltered (tmp_error_stream, 
			      "Could not insert hardware breakpoints:\n\
You may have requested too many hardware breakpoints/watchpoints.\n");
	}
      target_terminal_ours_for_output ();
      error_stream (tmp_error_stream);
    }

  do_cleanups (cleanups);
}

int
remove_breakpoints (void)
{
  struct bp_location *bl, **blp_tmp;
  int val = 0;

  ALL_BP_LOCATIONS (bl, blp_tmp)
  {
    if (bl->inserted)
      val |= remove_breakpoint (bl, mark_uninserted);
  }
  return val;
}

/* Remove breakpoints of process PID.  */

int
remove_breakpoints_pid (int pid)
{
  struct bp_location *bl, **blp_tmp;
  int val;
  struct inferior *inf = find_inferior_pid (pid);

  ALL_BP_LOCATIONS (bl, blp_tmp)
  {
    if (bl->pspace != inf->pspace)
      continue;

    if (bl->inserted)
      {
	val = remove_breakpoint (bl, mark_uninserted);
	if (val != 0)
	  return val;
      }
  }
  return 0;
}

int
remove_hw_watchpoints (void)
{
  struct bp_location *bl, **blp_tmp;
  int val = 0;

  ALL_BP_LOCATIONS (bl, blp_tmp)
  {
    if (bl->inserted && bl->loc_type == bp_loc_hardware_watchpoint)
      val |= remove_breakpoint (bl, mark_uninserted);
  }
  return val;
}

int
reattach_breakpoints (int pid)
{
  struct cleanup *old_chain;
  struct bp_location *bl, **blp_tmp;
  int val;
  struct ui_file *tmp_error_stream;
  int dummy1 = 0, dummy2 = 0;
  struct inferior *inf;
  struct thread_info *tp;

  tp = any_live_thread_of_process (pid);
  if (tp == NULL)
    return 1;

  inf = find_inferior_pid (pid);
  old_chain = save_inferior_ptid ();

  inferior_ptid = tp->ptid;

  tmp_error_stream = mem_fileopen ();
  make_cleanup_ui_file_delete (tmp_error_stream);

  ALL_BP_LOCATIONS (bl, blp_tmp)
  {
    if (bl->pspace != inf->pspace)
      continue;

    if (bl->inserted)
      {
	bl->inserted = 0;
	val = insert_bp_location (bl, tmp_error_stream, &dummy1, &dummy2);
	if (val != 0)
	  {
	    do_cleanups (old_chain);
	    return val;
	  }
      }
  }
  do_cleanups (old_chain);
  return 0;
}

static int internal_breakpoint_number = -1;

/* Set the breakpoint number of B, depending on the value of INTERNAL.
   If INTERNAL is non-zero, the breakpoint number will be populated
   from internal_breakpoint_number and that variable decremented.
   Otherwis the breakpoint number will be populated from
   breakpoint_count and that value incremented.  Internal breakpoints
   do not set the internal var bpnum.  */
static void
set_breakpoint_number (int internal, struct breakpoint *b)
{
  if (internal)
    b->number = internal_breakpoint_number--;
  else
    {
      set_breakpoint_count (breakpoint_count + 1);
      b->number = breakpoint_count;
    }
}

static struct breakpoint *
create_internal_breakpoint (struct gdbarch *gdbarch,
			    CORE_ADDR address, enum bptype type)
{
  struct symtab_and_line sal;
  struct breakpoint *b;

  init_sal (&sal);		/* Initialize to zeroes.  */

  sal.pc = address;
  sal.section = find_pc_overlay (sal.pc);
  sal.pspace = current_program_space;

  b = set_raw_breakpoint (gdbarch, sal, type);
  b->number = internal_breakpoint_number--;
  b->disposition = disp_donttouch;

  return b;
}

static const char *const longjmp_names[] =
  {
    "longjmp", "_longjmp", "siglongjmp", "_siglongjmp"
  };
#define NUM_LONGJMP_NAMES ARRAY_SIZE(longjmp_names)

/* Per-objfile data private to breakpoint.c.  */
struct breakpoint_objfile_data
{
  /* Minimal symbol for "_ovly_debug_event" (if any).  */
  struct minimal_symbol *overlay_msym;

  /* Minimal symbol(s) for "longjmp", "siglongjmp", etc. (if any).  */
  struct minimal_symbol *longjmp_msym[NUM_LONGJMP_NAMES];

  /* Minimal symbol for "std::terminate()" (if any).  */
  struct minimal_symbol *terminate_msym;

  /* Minimal symbol for "_Unwind_DebugHook" (if any).  */
  struct minimal_symbol *exception_msym;
};

static const struct objfile_data *breakpoint_objfile_key;

/* Minimal symbol not found sentinel.  */
static struct minimal_symbol msym_not_found;

/* Returns TRUE if MSYM point to the "not found" sentinel.  */

static int
msym_not_found_p (const struct minimal_symbol *msym)
{
  return msym == &msym_not_found;
}

/* Return per-objfile data needed by breakpoint.c.
   Allocate the data if necessary.  */

static struct breakpoint_objfile_data *
get_breakpoint_objfile_data (struct objfile *objfile)
{
  struct breakpoint_objfile_data *bp_objfile_data;

  bp_objfile_data = objfile_data (objfile, breakpoint_objfile_key);
  if (bp_objfile_data == NULL)
    {
      bp_objfile_data = obstack_alloc (&objfile->objfile_obstack,
				       sizeof (*bp_objfile_data));

      memset (bp_objfile_data, 0, sizeof (*bp_objfile_data));
      set_objfile_data (objfile, breakpoint_objfile_key, bp_objfile_data);
    }
  return bp_objfile_data;
}

static void
create_overlay_event_breakpoint (void)
{
  struct objfile *objfile;
  const char *const func_name = "_ovly_debug_event";

  ALL_OBJFILES (objfile)
    {
      struct breakpoint *b;
      struct breakpoint_objfile_data *bp_objfile_data;
      CORE_ADDR addr;

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      if (msym_not_found_p (bp_objfile_data->overlay_msym))
	continue;

      if (bp_objfile_data->overlay_msym == NULL)
	{
	  struct minimal_symbol *m;

	  m = lookup_minimal_symbol_text (func_name, objfile);
	  if (m == NULL)
	    {
	      /* Avoid future lookups in this objfile.  */
	      bp_objfile_data->overlay_msym = &msym_not_found;
	      continue;
	    }
	  bp_objfile_data->overlay_msym = m;
	}

      addr = SYMBOL_VALUE_ADDRESS (bp_objfile_data->overlay_msym);
      b = create_internal_breakpoint (get_objfile_arch (objfile), addr,
                                      bp_overlay_event);
      b->addr_string = xstrdup (func_name);

      if (overlay_debugging == ovly_auto)
        {
          b->enable_state = bp_enabled;
          overlay_events_enabled = 1;
        }
      else
       {
         b->enable_state = bp_disabled;
         overlay_events_enabled = 0;
       }
    }
  update_global_location_list (1);
}

static void
create_longjmp_master_breakpoint (void)
{
  struct program_space *pspace;
  struct cleanup *old_chain;

  old_chain = save_current_program_space ();

  ALL_PSPACES (pspace)
  {
    struct objfile *objfile;

    set_current_program_space (pspace);

    ALL_OBJFILES (objfile)
    {
      int i;
      struct gdbarch *gdbarch;
      struct breakpoint_objfile_data *bp_objfile_data;

      gdbarch = get_objfile_arch (objfile);
      if (!gdbarch_get_longjmp_target_p (gdbarch))
	continue;

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      for (i = 0; i < NUM_LONGJMP_NAMES; i++)
	{
	  struct breakpoint *b;
	  const char *func_name;
	  CORE_ADDR addr;

	  if (msym_not_found_p (bp_objfile_data->longjmp_msym[i]))
	    continue;

	  func_name = longjmp_names[i];
	  if (bp_objfile_data->longjmp_msym[i] == NULL)
	    {
	      struct minimal_symbol *m;

	      m = lookup_minimal_symbol_text (func_name, objfile);
	      if (m == NULL)
		{
		  /* Prevent future lookups in this objfile.  */
		  bp_objfile_data->longjmp_msym[i] = &msym_not_found;
		  continue;
		}
	      bp_objfile_data->longjmp_msym[i] = m;
	    }

	  addr = SYMBOL_VALUE_ADDRESS (bp_objfile_data->longjmp_msym[i]);
	  b = create_internal_breakpoint (gdbarch, addr, bp_longjmp_master);
	  b->addr_string = xstrdup (func_name);
	  b->enable_state = bp_disabled;
	}
    }
  }
  update_global_location_list (1);

  do_cleanups (old_chain);
}

/* Create a master std::terminate breakpoint.  */
static void
create_std_terminate_master_breakpoint (void)
{
  struct program_space *pspace;
  struct cleanup *old_chain;
  const char *const func_name = "std::terminate()";

  old_chain = save_current_program_space ();

  ALL_PSPACES (pspace)
  {
    struct objfile *objfile;
    CORE_ADDR addr;

    set_current_program_space (pspace);

    ALL_OBJFILES (objfile)
    {
      struct breakpoint *b;
      struct breakpoint_objfile_data *bp_objfile_data;

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      if (msym_not_found_p (bp_objfile_data->terminate_msym))
	continue;

      if (bp_objfile_data->terminate_msym == NULL)
	{
	  struct minimal_symbol *m;

	  m = lookup_minimal_symbol (func_name, NULL, objfile);
	  if (m == NULL || (MSYMBOL_TYPE (m) != mst_text
			    && MSYMBOL_TYPE (m) != mst_file_text))
	    {
	      /* Prevent future lookups in this objfile.  */
	      bp_objfile_data->terminate_msym = &msym_not_found;
	      continue;
	    }
	  bp_objfile_data->terminate_msym = m;
	}

      addr = SYMBOL_VALUE_ADDRESS (bp_objfile_data->terminate_msym);
      b = create_internal_breakpoint (get_objfile_arch (objfile), addr,
                                      bp_std_terminate_master);
      b->addr_string = xstrdup (func_name);
      b->enable_state = bp_disabled;
    }
  }

  update_global_location_list (1);

  do_cleanups (old_chain);
}

/* Install a master breakpoint on the unwinder's debug hook.  */

void
create_exception_master_breakpoint (void)
{
  struct objfile *objfile;
  const char *const func_name = "_Unwind_DebugHook";

  ALL_OBJFILES (objfile)
    {
      struct breakpoint *b;
      struct gdbarch *gdbarch;
      struct breakpoint_objfile_data *bp_objfile_data;
      CORE_ADDR addr;

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      if (msym_not_found_p (bp_objfile_data->exception_msym))
	continue;

      gdbarch = get_objfile_arch (objfile);

      if (bp_objfile_data->exception_msym == NULL)
	{
	  struct minimal_symbol *debug_hook;

	  debug_hook = lookup_minimal_symbol (func_name, NULL, objfile);
	  if (debug_hook == NULL)
	    {
	      bp_objfile_data->exception_msym = &msym_not_found;
	      continue;
	    }

	  bp_objfile_data->exception_msym = debug_hook;
	}

      addr = SYMBOL_VALUE_ADDRESS (bp_objfile_data->exception_msym);
      addr = gdbarch_convert_from_func_ptr_addr (gdbarch, addr,
						 &current_target);
      b = create_internal_breakpoint (gdbarch, addr, bp_exception_master);
      b->addr_string = xstrdup (func_name);
      b->enable_state = bp_disabled;
    }

  update_global_location_list (1);
}

void
update_breakpoints_after_exec (void)
{
  struct breakpoint *b, *b_tmp;
  struct bp_location *bploc, **bplocp_tmp;

  /* We're about to delete breakpoints from GDB's lists.  If the
     INSERTED flag is true, GDB will try to lift the breakpoints by
     writing the breakpoints' "shadow contents" back into memory.  The
     "shadow contents" are NOT valid after an exec, so GDB should not
     do that.  Instead, the target is responsible from marking
     breakpoints out as soon as it detects an exec.  We don't do that
     here instead, because there may be other attempts to delete
     breakpoints after detecting an exec and before reaching here.  */
  ALL_BP_LOCATIONS (bploc, bplocp_tmp)
    if (bploc->pspace == current_program_space)
      gdb_assert (!bploc->inserted);

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
  {
    if (b->pspace != current_program_space)
      continue;

    /* Solib breakpoints must be explicitly reset after an exec().  */
    if (b->type == bp_shlib_event)
      {
	delete_breakpoint (b);
	continue;
      }

    /* JIT breakpoints must be explicitly reset after an exec().  */
    if (b->type == bp_jit_event)
      {
	delete_breakpoint (b);
	continue;
      }

    /* Thread event breakpoints must be set anew after an exec(),
       as must overlay event and longjmp master breakpoints.  */
    if (b->type == bp_thread_event || b->type == bp_overlay_event
	|| b->type == bp_longjmp_master || b->type == bp_std_terminate_master
	|| b->type == bp_exception_master)
      {
	delete_breakpoint (b);
	continue;
      }

    /* Step-resume breakpoints are meaningless after an exec().  */
    if (b->type == bp_step_resume)
      {
	delete_breakpoint (b);
	continue;
      }

    /* Longjmp and longjmp-resume breakpoints are also meaningless
       after an exec.  */
    if (b->type == bp_longjmp || b->type == bp_longjmp_resume
	|| b->type == bp_exception || b->type == bp_exception_resume)
      {
	delete_breakpoint (b);
	continue;
      }

    if (b->type == bp_catchpoint)
      {
        /* For now, none of the bp_catchpoint breakpoints need to
           do anything at this point.  In the future, if some of
           the catchpoints need to something, we will need to add
           a new method, and call this method from here.  */
        continue;
      }

    /* bp_finish is a special case.  The only way we ought to be able
       to see one of these when an exec() has happened, is if the user
       caught a vfork, and then said "finish".  Ordinarily a finish just
       carries them to the call-site of the current callee, by setting
       a temporary bp there and resuming.  But in this case, the finish
       will carry them entirely through the vfork & exec.

       We don't want to allow a bp_finish to remain inserted now.  But
       we can't safely delete it, 'cause finish_command has a handle to
       the bp on a bpstat, and will later want to delete it.  There's a
       chance (and I've seen it happen) that if we delete the bp_finish
       here, that its storage will get reused by the time finish_command
       gets 'round to deleting the "use to be a bp_finish" breakpoint.
       We really must allow finish_command to delete a bp_finish.

       In the absense of a general solution for the "how do we know
       it's safe to delete something others may have handles to?"
       problem, what we'll do here is just uninsert the bp_finish, and
       let finish_command delete it.

       (We know the bp_finish is "doomed" in the sense that it's
       momentary, and will be deleted as soon as finish_command sees
       the inferior stopped.  So it doesn't matter that the bp's
       address is probably bogus in the new a.out, unlike e.g., the
       solib breakpoints.)  */

    if (b->type == bp_finish)
      {
	continue;
      }

    /* Without a symbolic address, we have little hope of the
       pre-exec() address meaning the same thing in the post-exec()
       a.out.  */
    if (b->addr_string == NULL)
      {
	delete_breakpoint (b);
	continue;
      }
  }
  /* FIXME what about longjmp breakpoints?  Re-create them here?  */
  create_overlay_event_breakpoint ();
  create_longjmp_master_breakpoint ();
  create_std_terminate_master_breakpoint ();
  create_exception_master_breakpoint ();
}

int
detach_breakpoints (int pid)
{
  struct bp_location *bl, **blp_tmp;
  int val = 0;
  struct cleanup *old_chain = save_inferior_ptid ();
  struct inferior *inf = current_inferior ();

  if (pid == PIDGET (inferior_ptid))
    error (_("Cannot detach breakpoints of inferior_ptid"));

  /* Set inferior_ptid; remove_breakpoint_1 uses this global.  */
  inferior_ptid = pid_to_ptid (pid);
  ALL_BP_LOCATIONS (bl, blp_tmp)
  {
    if (bl->pspace != inf->pspace)
      continue;

    if (bl->inserted)
      val |= remove_breakpoint_1 (bl, mark_inserted);
  }

  /* Detach single-step breakpoints as well.  */
  detach_single_step_breakpoints ();

  do_cleanups (old_chain);
  return val;
}

/* Remove the breakpoint location BL from the current address space.
   Note that this is used to detach breakpoints from a child fork.
   When we get here, the child isn't in the inferior list, and neither
   do we have objects to represent its address space --- we should
   *not* look at bl->pspace->aspace here.  */

static int
remove_breakpoint_1 (struct bp_location *bl, insertion_state_t is)
{
  int val;

  /* BL is never in moribund_locations by our callers.  */
  gdb_assert (bl->owner != NULL);

  if (bl->owner->enable_state == bp_permanent)
    /* Permanent breakpoints cannot be inserted or removed.  */
    return 0;

  /* The type of none suggests that owner is actually deleted.
     This should not ever happen.  */
  gdb_assert (bl->owner->type != bp_none);

  if (bl->loc_type == bp_loc_software_breakpoint
      || bl->loc_type == bp_loc_hardware_breakpoint)
    {
      /* "Normal" instruction breakpoint: either the standard
	 trap-instruction bp (bp_breakpoint), or a
	 bp_hardware_breakpoint.  */

      /* First check to see if we have to handle an overlay.  */
      if (overlay_debugging == ovly_off
	  || bl->section == NULL
	  || !(section_is_overlay (bl->section)))
	{
	  /* No overlay handling: just remove the breakpoint.  */

	  if (bl->loc_type == bp_loc_hardware_breakpoint)
	    val = target_remove_hw_breakpoint (bl->gdbarch, &bl->target_info);
	  else
	    val = target_remove_breakpoint (bl->gdbarch, &bl->target_info);
	}
      else
	{
	  /* This breakpoint is in an overlay section.
	     Did we set a breakpoint at the LMA?  */
	  if (!overlay_events_enabled)
	      {
		/* Yes -- overlay event support is not active, so we
		   should have set a breakpoint at the LMA.  Remove it.  
		*/
		/* Ignore any failures: if the LMA is in ROM, we will
		   have already warned when we failed to insert it.  */
		if (bl->loc_type == bp_loc_hardware_breakpoint)
		  target_remove_hw_breakpoint (bl->gdbarch,
					       &bl->overlay_target_info);
		else
		  target_remove_breakpoint (bl->gdbarch,
					    &bl->overlay_target_info);
	      }
	  /* Did we set a breakpoint at the VMA? 
	     If so, we will have marked the breakpoint 'inserted'.  */
	  if (bl->inserted)
	    {
	      /* Yes -- remove it.  Previously we did not bother to
		 remove the breakpoint if the section had been
		 unmapped, but let's not rely on that being safe.  We
		 don't know what the overlay manager might do.  */
	      if (bl->loc_type == bp_loc_hardware_breakpoint)
		val = target_remove_hw_breakpoint (bl->gdbarch,
						   &bl->target_info);

	      /* However, we should remove *software* breakpoints only
		 if the section is still mapped, or else we overwrite
		 wrong code with the saved shadow contents.  */
	      else if (section_is_mapped (bl->section))
		val = target_remove_breakpoint (bl->gdbarch,
						&bl->target_info);
	      else
		val = 0;
	    }
	  else
	    {
	      /* No -- not inserted, so no need to remove.  No error.  */
	      val = 0;
	    }
	}

      /* In some cases, we might not be able to remove a breakpoint
	 in a shared library that has already been removed, but we
	 have not yet processed the shlib unload event.  */
      if (val && solib_name_from_address (bl->pspace, bl->address))
	val = 0;

      if (val)
	return val;
      bl->inserted = (is == mark_inserted);
    }
  else if (bl->loc_type == bp_loc_hardware_watchpoint)
    {
      gdb_assert (bl->owner->ops != NULL
		  && bl->owner->ops->remove_location != NULL);

      bl->inserted = (is == mark_inserted);
      bl->owner->ops->remove_location (bl);

      /* Failure to remove any of the hardware watchpoints comes here.  */
      if ((is == mark_uninserted) && (bl->inserted))
	warning (_("Could not remove hardware watchpoint %d."),
		 bl->owner->number);
    }
  else if (bl->owner->type == bp_catchpoint
           && breakpoint_enabled (bl->owner)
           && !bl->duplicate)
    {
      gdb_assert (bl->owner->ops != NULL
		  && bl->owner->ops->remove_location != NULL);

      val = bl->owner->ops->remove_location (bl);
      if (val)
	return val;

      bl->inserted = (is == mark_inserted);
    }

  return 0;
}

static int
remove_breakpoint (struct bp_location *bl, insertion_state_t is)
{
  int ret;
  struct cleanup *old_chain;

  /* BL is never in moribund_locations by our callers.  */
  gdb_assert (bl->owner != NULL);

  if (bl->owner->enable_state == bp_permanent)
    /* Permanent breakpoints cannot be inserted or removed.  */
    return 0;

  /* The type of none suggests that owner is actually deleted.
     This should not ever happen.  */
  gdb_assert (bl->owner->type != bp_none);

  old_chain = save_current_space_and_thread ();

  switch_to_program_space_and_thread (bl->pspace);

  ret = remove_breakpoint_1 (bl, is);

  do_cleanups (old_chain);
  return ret;
}

/* Clear the "inserted" flag in all breakpoints.  */

void
mark_breakpoints_out (void)
{
  struct bp_location *bl, **blp_tmp;

  ALL_BP_LOCATIONS (bl, blp_tmp)
    if (bl->pspace == current_program_space)
      bl->inserted = 0;
}

/* Clear the "inserted" flag in all breakpoints and delete any
   breakpoints which should go away between runs of the program.

   Plus other such housekeeping that has to be done for breakpoints
   between runs.

   Note: this function gets called at the end of a run (by
   generic_mourn_inferior) and when a run begins (by
   init_wait_for_inferior).  */



void
breakpoint_init_inferior (enum inf_context context)
{
  struct breakpoint *b, *b_tmp;
  struct bp_location *bl, **blp_tmp;
  int ix;
  struct program_space *pspace = current_program_space;

  /* If breakpoint locations are shared across processes, then there's
     nothing to do.  */
  if (gdbarch_has_global_breakpoints (target_gdbarch))
    return;

  ALL_BP_LOCATIONS (bl, blp_tmp)
  {
    /* ALL_BP_LOCATIONS bp_location has BL->OWNER always non-NULL.  */
    if (bl->pspace == pspace
	&& bl->owner->enable_state != bp_permanent)
      bl->inserted = 0;
  }

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
  {
    if (b->loc && b->loc->pspace != pspace)
      continue;

    switch (b->type)
      {
      case bp_call_dummy:

	/* If the call dummy breakpoint is at the entry point it will
	   cause problems when the inferior is rerun, so we better get
	   rid of it.  */

      case bp_watchpoint_scope:

	/* Also get rid of scope breakpoints.  */

      case bp_shlib_event:

	/* Also remove solib event breakpoints.  Their addresses may
	   have changed since the last time we ran the program.
	   Actually we may now be debugging against different target;
	   and so the solib backend that installed this breakpoint may
	   not be used in by the target.  E.g.,

	   (gdb) file prog-linux
	   (gdb) run               # native linux target
	   ...
	   (gdb) kill
	   (gdb) file prog-win.exe
	   (gdb) tar rem :9999     # remote Windows gdbserver.
	*/

	delete_breakpoint (b);
	break;

      case bp_watchpoint:
      case bp_hardware_watchpoint:
      case bp_read_watchpoint:
      case bp_access_watchpoint:

	/* Likewise for watchpoints on local expressions.  */
	if (b->exp_valid_block != NULL)
	  delete_breakpoint (b);
	else if (context == inf_starting) 
	  {
	    /* Reset val field to force reread of starting value in
	       insert_breakpoints.  */
	    if (b->val)
	      value_free (b->val);
	    b->val = NULL;
	    b->val_valid = 0;
	  }
	break;
      default:
	break;
      }
  }

  /* Get rid of the moribund locations.  */
  for (ix = 0; VEC_iterate (bp_location_p, moribund_locations, ix, bl); ++ix)
    decref_bp_location (&bl);
  VEC_free (bp_location_p, moribund_locations);
}

/* These functions concern about actual breakpoints inserted in the
   target --- to e.g. check if we need to do decr_pc adjustment or if
   we need to hop over the bkpt --- so we check for address space
   match, not program space.  */

/* breakpoint_here_p (PC) returns non-zero if an enabled breakpoint
   exists at PC.  It returns ordinary_breakpoint_here if it's an
   ordinary breakpoint, or permanent_breakpoint_here if it's a
   permanent breakpoint.
   - When continuing from a location with an ordinary breakpoint, we
     actually single step once before calling insert_breakpoints.
   - When continuing from a localion with a permanent breakpoint, we
     need to use the `SKIP_PERMANENT_BREAKPOINT' macro, provided by
     the target, to advance the PC past the breakpoint.  */

enum breakpoint_here
breakpoint_here_p (struct address_space *aspace, CORE_ADDR pc)
{
  struct bp_location *bl, **blp_tmp;
  int any_breakpoint_here = 0;

  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      if (bl->loc_type != bp_loc_software_breakpoint
	  && bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      /* ALL_BP_LOCATIONS bp_location has BL->OWNER always non-NULL.  */
      if ((breakpoint_enabled (bl->owner)
	   || bl->owner->enable_state == bp_permanent)
	  && breakpoint_location_address_match (bl, aspace, pc))
	{
	  if (overlay_debugging 
	      && section_is_overlay (bl->section)
	      && !section_is_mapped (bl->section))
	    continue;		/* unmapped overlay -- can't be a match */
	  else if (bl->owner->enable_state == bp_permanent)
	    return permanent_breakpoint_here;
	  else
	    any_breakpoint_here = 1;
	}
    }

  return any_breakpoint_here ? ordinary_breakpoint_here : 0;
}

/* Return true if there's a moribund breakpoint at PC.  */

int
moribund_breakpoint_here_p (struct address_space *aspace, CORE_ADDR pc)
{
  struct bp_location *loc;
  int ix;

  for (ix = 0; VEC_iterate (bp_location_p, moribund_locations, ix, loc); ++ix)
    if (breakpoint_location_address_match (loc, aspace, pc))
      return 1;

  return 0;
}

/* Returns non-zero if there's a breakpoint inserted at PC, which is
   inserted using regular breakpoint_chain / bp_location array
   mechanism.  This does not check for single-step breakpoints, which
   are inserted and removed using direct target manipulation.  */

int
regular_breakpoint_inserted_here_p (struct address_space *aspace, 
				    CORE_ADDR pc)
{
  struct bp_location *bl, **blp_tmp;

  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      if (bl->loc_type != bp_loc_software_breakpoint
	  && bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      if (bl->inserted
	  && breakpoint_location_address_match (bl, aspace, pc))
	{
	  if (overlay_debugging 
	      && section_is_overlay (bl->section)
	      && !section_is_mapped (bl->section))
	    continue;		/* unmapped overlay -- can't be a match */
	  else
	    return 1;
	}
    }
  return 0;
}

/* Returns non-zero iff there's either regular breakpoint
   or a single step breakpoint inserted at PC.  */

int
breakpoint_inserted_here_p (struct address_space *aspace, CORE_ADDR pc)
{
  if (regular_breakpoint_inserted_here_p (aspace, pc))
    return 1;

  if (single_step_breakpoint_inserted_here_p (aspace, pc))
    return 1;

  return 0;
}

/* This function returns non-zero iff there is a software breakpoint
   inserted at PC.  */

int
software_breakpoint_inserted_here_p (struct address_space *aspace,
				     CORE_ADDR pc)
{
  struct bp_location *bl, **blp_tmp;

  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      if (bl->loc_type != bp_loc_software_breakpoint)
	continue;

      if (bl->inserted
	  && breakpoint_address_match (bl->pspace->aspace, bl->address,
				       aspace, pc))
	{
	  if (overlay_debugging 
	      && section_is_overlay (bl->section)
	      && !section_is_mapped (bl->section))
	    continue;		/* unmapped overlay -- can't be a match */
	  else
	    return 1;
	}
    }

  /* Also check for software single-step breakpoints.  */
  if (single_step_breakpoint_inserted_here_p (aspace, pc))
    return 1;

  return 0;
}

int
hardware_watchpoint_inserted_in_range (struct address_space *aspace,
				       CORE_ADDR addr, ULONGEST len)
{
  struct breakpoint *bpt;

  ALL_BREAKPOINTS (bpt)
    {
      struct bp_location *loc;

      if (bpt->type != bp_hardware_watchpoint
	  && bpt->type != bp_access_watchpoint)
	continue;

      if (!breakpoint_enabled (bpt))
	continue;

      for (loc = bpt->loc; loc; loc = loc->next)
	if (loc->pspace->aspace == aspace && loc->inserted)
	  {
	    CORE_ADDR l, h;

	    /* Check for intersection.  */
	    l = max (loc->address, addr);
	    h = min (loc->address + loc->length, addr + len);
	    if (l < h)
	      return 1;
	  }
    }
  return 0;
}

/* breakpoint_thread_match (PC, PTID) returns true if the breakpoint at
   PC is valid for process/thread PTID.  */

int
breakpoint_thread_match (struct address_space *aspace, CORE_ADDR pc,
			 ptid_t ptid)
{
  struct bp_location *bl, **blp_tmp;
  /* The thread and task IDs associated to PTID, computed lazily.  */
  int thread = -1;
  int task = 0;
  
  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      if (bl->loc_type != bp_loc_software_breakpoint
	  && bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      /* ALL_BP_LOCATIONS bp_location has bl->OWNER always non-NULL.  */
      if (!breakpoint_enabled (bl->owner)
	  && bl->owner->enable_state != bp_permanent)
	continue;

      if (!breakpoint_location_address_match (bl, aspace, pc))
	continue;

      if (bl->owner->thread != -1)
	{
	  /* This is a thread-specific breakpoint.  Check that ptid
	     matches that thread.  If thread hasn't been computed yet,
	     it is now time to do so.  */
	  if (thread == -1)
	    thread = pid_to_thread_id (ptid);
	  if (bl->owner->thread != thread)
	    continue;
	}

      if (bl->owner->task != 0)
        {
	  /* This is a task-specific breakpoint.  Check that ptid
	     matches that task.  If task hasn't been computed yet,
	     it is now time to do so.  */
	  if (task == 0)
	    task = ada_get_task_number (ptid);
	  if (bl->owner->task != task)
	    continue;
        }

      if (overlay_debugging 
	  && section_is_overlay (bl->section)
	  && !section_is_mapped (bl->section))
	continue;	    /* unmapped overlay -- can't be a match */

      return 1;
    }

  return 0;
}


/* bpstat stuff.  External routines' interfaces are documented
   in breakpoint.h.  */

int
ep_is_catchpoint (struct breakpoint *ep)
{
  return (ep->type == bp_catchpoint);
}

/* Frees any storage that is part of a bpstat.  Does not walk the
   'next' chain.  */

static void
bpstat_free (bpstat bs)
{
  if (bs->old_val != NULL)
    value_free (bs->old_val);
  decref_counted_command_line (&bs->commands);
  decref_bp_location (&bs->bp_location_at);
  xfree (bs);
}

/* Clear a bpstat so that it says we are not at any breakpoint.
   Also free any storage that is part of a bpstat.  */

void
bpstat_clear (bpstat *bsp)
{
  bpstat p;
  bpstat q;

  if (bsp == 0)
    return;
  p = *bsp;
  while (p != NULL)
    {
      q = p->next;
      bpstat_free (p);
      p = q;
    }
  *bsp = NULL;
}

/* Return a copy of a bpstat.  Like "bs1 = bs2" but all storage that
   is part of the bpstat is copied as well.  */

bpstat
bpstat_copy (bpstat bs)
{
  bpstat p = NULL;
  bpstat tmp;
  bpstat retval = NULL;

  if (bs == NULL)
    return bs;

  for (; bs != NULL; bs = bs->next)
    {
      tmp = (bpstat) xmalloc (sizeof (*tmp));
      memcpy (tmp, bs, sizeof (*tmp));
      incref_counted_command_line (tmp->commands);
      incref_bp_location (tmp->bp_location_at);
      if (bs->old_val != NULL)
	{
	  tmp->old_val = value_copy (bs->old_val);
	  release_value (tmp->old_val);
	}

      if (p == NULL)
	/* This is the first thing in the chain.  */
	retval = tmp;
      else
	p->next = tmp;
      p = tmp;
    }
  p->next = NULL;
  return retval;
}

/* Find the bpstat associated with this breakpoint.  */

bpstat
bpstat_find_breakpoint (bpstat bsp, struct breakpoint *breakpoint)
{
  if (bsp == NULL)
    return NULL;

  for (; bsp != NULL; bsp = bsp->next)
    {
      if (bsp->breakpoint_at == breakpoint)
	return bsp;
    }
  return NULL;
}

/* Put in *NUM the breakpoint number of the first breakpoint we are
   stopped at.  *BSP upon return is a bpstat which points to the
   remaining breakpoints stopped at (but which is not guaranteed to be
   good for anything but further calls to bpstat_num).

   Return 0 if passed a bpstat which does not indicate any breakpoints.
   Return -1 if stopped at a breakpoint that has been deleted since
   we set it.
   Return 1 otherwise.  */

int
bpstat_num (bpstat *bsp, int *num)
{
  struct breakpoint *b;

  if ((*bsp) == NULL)
    return 0;			/* No more breakpoint values */

  /* We assume we'll never have several bpstats that correspond to a
     single breakpoint -- otherwise, this function might return the
     same number more than once and this will look ugly.  */
  b = (*bsp)->breakpoint_at;
  *bsp = (*bsp)->next;
  if (b == NULL)
    return -1;			/* breakpoint that's been deleted since */

  *num = b->number;		/* We have its number */
  return 1;
}

/* Modify BS so that the actions will not be performed.  */

void
bpstat_clear_actions (bpstat bs)
{
  for (; bs != NULL; bs = bs->next)
    {
      decref_counted_command_line (&bs->commands);
      bs->commands_left = NULL;
      if (bs->old_val != NULL)
	{
	  value_free (bs->old_val);
	  bs->old_val = NULL;
	}
    }
}

/* Called when a command is about to proceed the inferior.  */

static void
breakpoint_about_to_proceed (void)
{
  if (!ptid_equal (inferior_ptid, null_ptid))
    {
      struct thread_info *tp = inferior_thread ();

      /* Allow inferior function calls in breakpoint commands to not
	 interrupt the command list.  When the call finishes
	 successfully, the inferior will be standing at the same
	 breakpoint as if nothing happened.  */
      if (tp->control.in_infcall)
	return;
    }

  breakpoint_proceeded = 1;
}

/* Stub for cleaning up our state if we error-out of a breakpoint
   command.  */
static void
cleanup_executing_breakpoints (void *ignore)
{
  executing_breakpoint_commands = 0;
}

/* Execute all the commands associated with all the breakpoints at
   this location.  Any of these commands could cause the process to
   proceed beyond this point, etc.  We look out for such changes by
   checking the global "breakpoint_proceeded" after each command.

   Returns true if a breakpoint command resumed the inferior.  In that
   case, it is the caller's responsibility to recall it again with the
   bpstat of the current thread.  */

static int
bpstat_do_actions_1 (bpstat *bsp)
{
  bpstat bs;
  struct cleanup *old_chain;
  int again = 0;

  /* Avoid endless recursion if a `source' command is contained
     in bs->commands.  */
  if (executing_breakpoint_commands)
    return 0;

  executing_breakpoint_commands = 1;
  old_chain = make_cleanup (cleanup_executing_breakpoints, 0);

  /* This pointer will iterate over the list of bpstat's.  */
  bs = *bsp;

  breakpoint_proceeded = 0;
  for (; bs != NULL; bs = bs->next)
    {
      struct counted_command_line *ccmd;
      struct command_line *cmd;
      struct cleanup *this_cmd_tree_chain;

      /* Take ownership of the BSP's command tree, if it has one.

         The command tree could legitimately contain commands like
         'step' and 'next', which call clear_proceed_status, which
         frees stop_bpstat's command tree.  To make sure this doesn't
         free the tree we're executing out from under us, we need to
         take ownership of the tree ourselves.  Since a given bpstat's
         commands are only executed once, we don't need to copy it; we
         can clear the pointer in the bpstat, and make sure we free
         the tree when we're done.  */
      ccmd = bs->commands;
      bs->commands = NULL;
      this_cmd_tree_chain
	= make_cleanup_decref_counted_command_line (&ccmd);
      cmd = bs->commands_left;
      bs->commands_left = NULL;

      while (cmd != NULL)
	{
	  execute_control_command (cmd);

	  if (breakpoint_proceeded)
	    break;
	  else
	    cmd = cmd->next;
	}

      /* We can free this command tree now.  */
      do_cleanups (this_cmd_tree_chain);

      if (breakpoint_proceeded)
	{
	  if (target_can_async_p ())
	    /* If we are in async mode, then the target might be still
	       running, not stopped at any breakpoint, so nothing for
	       us to do here -- just return to the event loop.  */
	    ;
	  else
	    /* In sync mode, when execute_control_command returns
	       we're already standing on the next breakpoint.
	       Breakpoint commands for that stop were not run, since
	       execute_command does not run breakpoint commands --
	       only command_line_handler does, but that one is not
	       involved in execution of breakpoint commands.  So, we
	       can now execute breakpoint commands.  It should be
	       noted that making execute_command do bpstat actions is
	       not an option -- in this case we'll have recursive
	       invocation of bpstat for each breakpoint with a
	       command, and can easily blow up GDB stack.  Instead, we
	       return true, which will trigger the caller to recall us
	       with the new stop_bpstat.  */
	    again = 1;
	  break;
	}
    }
  do_cleanups (old_chain);
  return again;
}

void
bpstat_do_actions (void)
{
  /* Do any commands attached to breakpoint we are stopped at.  */
  while (!ptid_equal (inferior_ptid, null_ptid)
	 && target_has_execution
	 && !is_exited (inferior_ptid)
	 && !is_executing (inferior_ptid))
    /* Since in sync mode, bpstat_do_actions may resume the inferior,
       and only return when it is stopped at the next breakpoint, we
       keep doing breakpoint actions until it returns false to
       indicate the inferior was not resumed.  */
    if (!bpstat_do_actions_1 (&inferior_thread ()->control.stop_bpstat))
      break;
}

/* Print out the (old or new) value associated with a watchpoint.  */

static void
watchpoint_value_print (struct value *val, struct ui_file *stream)
{
  if (val == NULL)
    fprintf_unfiltered (stream, _("<unreadable>"));
  else
    {
      struct value_print_options opts;
      get_user_print_options (&opts);
      value_print (val, stream, &opts);
    }
}

/* This is the normal print function for a bpstat.  In the future,
   much of this logic could (should?) be moved to bpstat_stop_status,
   by having it set different print_it values.

   Current scheme: When we stop, bpstat_print() is called.  It loops
   through the bpstat list of things causing this stop, calling the
   print_bp_stop_message function on each one.  The behavior of the
   print_bp_stop_message function depends on the print_it field of
   bpstat.  If such field so indicates, call this function here.

   Return values from this routine (ultimately used by bpstat_print()
   and normal_stop() to decide what to do): 
   PRINT_NOTHING: Means we already printed all we needed to print,
   don't print anything else.
   PRINT_SRC_ONLY: Means we printed something, and we do *not* desire
   that something to be followed by a location.
   PRINT_SCR_AND_LOC: Means we printed something, and we *do* desire
   that something to be followed by a location.
   PRINT_UNKNOWN: Means we printed nothing or we need to do some more
   analysis.  */

static enum print_stop_action
print_it_typical (bpstat bs)
{
  struct cleanup *old_chain;
  struct breakpoint *b;
  const struct bp_location *bl;
  struct ui_stream *stb;
  int bp_temp = 0;
  enum print_stop_action result;

  gdb_assert (bs->bp_location_at != NULL);

  bl = bs->bp_location_at;
  b = bs->breakpoint_at;

  stb = ui_out_stream_new (uiout);
  old_chain = make_cleanup_ui_out_stream_delete (stb);

  switch (b->type)
    {
    case bp_breakpoint:
    case bp_hardware_breakpoint:
      bp_temp = b->disposition == disp_del;
      if (bl->address != bl->requested_address)
	breakpoint_adjustment_warning (bl->requested_address,
	                               bl->address,
				       b->number, 1);
      annotate_breakpoint (b->number);
      if (bp_temp) 
	ui_out_text (uiout, "\nTemporary breakpoint ");
      else
	ui_out_text (uiout, "\nBreakpoint ");
      if (ui_out_is_mi_like_p (uiout))
	{
	  ui_out_field_string (uiout, "reason", 
			  async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
	  ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
	}
      ui_out_field_int (uiout, "bkptno", b->number);
      ui_out_text (uiout, ", ");
      result = PRINT_SRC_AND_LOC;
      break;

    case bp_shlib_event:
      /* Did we stop because the user set the stop_on_solib_events
	 variable?  (If so, we report this as a generic, "Stopped due
	 to shlib event" message.) */
      printf_filtered (_("Stopped due to shared library event\n"));
      result = PRINT_NOTHING;
      break;

    case bp_thread_event:
      /* Not sure how we will get here.
	 GDB should not stop for these breakpoints.  */
      printf_filtered (_("Thread Event Breakpoint: gdb should not stop!\n"));
      result = PRINT_NOTHING;
      break;

    case bp_overlay_event:
      /* By analogy with the thread event, GDB should not stop for these.  */
      printf_filtered (_("Overlay Event Breakpoint: gdb should not stop!\n"));
      result = PRINT_NOTHING;
      break;

    case bp_longjmp_master:
      /* These should never be enabled.  */
      printf_filtered (_("Longjmp Master Breakpoint: gdb should not stop!\n"));
      result = PRINT_NOTHING;
      break;

    case bp_std_terminate_master:
      /* These should never be enabled.  */
      printf_filtered (_("std::terminate Master Breakpoint: "
			 "gdb should not stop!\n"));
      result = PRINT_NOTHING;
      break;

    case bp_exception_master:
      /* These should never be enabled.  */
      printf_filtered (_("Exception Master Breakpoint: "
			 "gdb should not stop!\n"));
      result = PRINT_NOTHING;
      break;

    case bp_watchpoint:
    case bp_hardware_watchpoint:
      annotate_watchpoint (b->number);
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason",
	   async_reason_lookup (EXEC_ASYNC_WATCHPOINT_TRIGGER));
      mention (b);
      make_cleanup_ui_out_tuple_begin_end (uiout, "value");
      ui_out_text (uiout, "\nOld value = ");
      watchpoint_value_print (bs->old_val, stb->stream);
      ui_out_field_stream (uiout, "old", stb);
      ui_out_text (uiout, "\nNew value = ");
      watchpoint_value_print (b->val, stb->stream);
      ui_out_field_stream (uiout, "new", stb);
      ui_out_text (uiout, "\n");
      /* More than one watchpoint may have been triggered.  */
      result = PRINT_UNKNOWN;
      break;

    case bp_read_watchpoint:
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason",
	   async_reason_lookup (EXEC_ASYNC_READ_WATCHPOINT_TRIGGER));
      mention (b);
      make_cleanup_ui_out_tuple_begin_end (uiout, "value");
      ui_out_text (uiout, "\nValue = ");
      watchpoint_value_print (b->val, stb->stream);
      ui_out_field_stream (uiout, "value", stb);
      ui_out_text (uiout, "\n");
      result = PRINT_UNKNOWN;
      break;

    case bp_access_watchpoint:
      if (bs->old_val != NULL)
	{
	  annotate_watchpoint (b->number);
	  if (ui_out_is_mi_like_p (uiout))
	    ui_out_field_string
	      (uiout, "reason",
	       async_reason_lookup (EXEC_ASYNC_ACCESS_WATCHPOINT_TRIGGER));
	  mention (b);
	  make_cleanup_ui_out_tuple_begin_end (uiout, "value");
	  ui_out_text (uiout, "\nOld value = ");
	  watchpoint_value_print (bs->old_val, stb->stream);
	  ui_out_field_stream (uiout, "old", stb);
	  ui_out_text (uiout, "\nNew value = ");
	}
      else 
	{
	  mention (b);
	  if (ui_out_is_mi_like_p (uiout))
	    ui_out_field_string
	      (uiout, "reason",
	       async_reason_lookup (EXEC_ASYNC_ACCESS_WATCHPOINT_TRIGGER));
	  make_cleanup_ui_out_tuple_begin_end (uiout, "value");
	  ui_out_text (uiout, "\nValue = ");
	}
      watchpoint_value_print (b->val, stb->stream);
      ui_out_field_stream (uiout, "new", stb);
      ui_out_text (uiout, "\n");
      result = PRINT_UNKNOWN;
      break;

    /* Fall through, we don't deal with these types of breakpoints
       here.  */

    case bp_finish:
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason",
	   async_reason_lookup (EXEC_ASYNC_FUNCTION_FINISHED));
      result = PRINT_UNKNOWN;
      break;

    case bp_until:
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason",
	   async_reason_lookup (EXEC_ASYNC_LOCATION_REACHED));
      result = PRINT_UNKNOWN;
      break;

    case bp_none:
    case bp_longjmp:
    case bp_longjmp_resume:
    case bp_exception:
    case bp_exception_resume:
    case bp_step_resume:
    case bp_watchpoint_scope:
    case bp_call_dummy:
    case bp_std_terminate:
    case bp_tracepoint:
    case bp_fast_tracepoint:
    case bp_jit_event:
    case bp_gnu_ifunc_resolver:
    case bp_gnu_ifunc_resolver_return:
    default:
      result = PRINT_UNKNOWN;
      break;
    }

  do_cleanups (old_chain);
  return result;
}

/* Generic routine for printing messages indicating why we
   stopped.  The behavior of this function depends on the value
   'print_it' in the bpstat structure.  Under some circumstances we
   may decide not to print anything here and delegate the task to
   normal_stop().  */

static enum print_stop_action
print_bp_stop_message (bpstat bs)
{
  switch (bs->print_it)
    {
    case print_it_noop:
      /* Nothing should be printed for this bpstat entry.  */
      return PRINT_UNKNOWN;
      break;

    case print_it_done:
      /* We still want to print the frame, but we already printed the
         relevant messages.  */
      return PRINT_SRC_AND_LOC;
      break;

    case print_it_normal:
      {
	struct breakpoint *b = bs->breakpoint_at;

	/* bs->breakpoint_at can be NULL if it was a momentary breakpoint
	   which has since been deleted.  */
	if (b == NULL)
	  return PRINT_UNKNOWN;

	/* Normal case.  Call the breakpoint's print_it method, or
	   print_it_typical.  */
	if (b->ops != NULL && b->ops->print_it != NULL)
	  return b->ops->print_it (b);
	else
	  return print_it_typical (bs);
      }
	break;

    default:
      internal_error (__FILE__, __LINE__,
		      _("print_bp_stop_message: unrecognized enum value"));
      break;
    }
}

/* Print a message indicating what happened.  This is called from
   normal_stop().  The input to this routine is the head of the bpstat
   list - a list of the eventpoints that caused this stop.  This
   routine calls the generic print routine for printing a message
   about reasons for stopping.  This will print (for example) the
   "Breakpoint n," part of the output.  The return value of this
   routine is one of:

   PRINT_UNKNOWN: Means we printed nothing.
   PRINT_SRC_AND_LOC: Means we printed something, and expect subsequent
   code to print the location.  An example is 
   "Breakpoint 1, " which should be followed by
   the location.
   PRINT_SRC_ONLY: Means we printed something, but there is no need
   to also print the location part of the message.
   An example is the catch/throw messages, which
   don't require a location appended to the end.
   PRINT_NOTHING: We have done some printing and we don't need any 
   further info to be printed.  */

enum print_stop_action
bpstat_print (bpstat bs)
{
  int val;

  /* Maybe another breakpoint in the chain caused us to stop.
     (Currently all watchpoints go on the bpstat whether hit or not.
     That probably could (should) be changed, provided care is taken
     with respect to bpstat_explains_signal).  */
  for (; bs; bs = bs->next)
    {
      val = print_bp_stop_message (bs);
      if (val == PRINT_SRC_ONLY 
	  || val == PRINT_SRC_AND_LOC 
	  || val == PRINT_NOTHING)
	return val;
    }

  /* We reached the end of the chain, or we got a null BS to start
     with and nothing was printed.  */
  return PRINT_UNKNOWN;
}

/* Evaluate the expression EXP and return 1 if value is zero.  This is
   used inside a catch_errors to evaluate the breakpoint condition.
   The argument is a "struct expression *" that has been cast to a
   "char *" to make it pass through catch_errors.  */

static int
breakpoint_cond_eval (void *exp)
{
  struct value *mark = value_mark ();
  int i = !value_true (evaluate_expression ((struct expression *) exp));

  value_free_to_mark (mark);
  return i;
}

/* Allocate a new bpstat.  Link it to the FIFO list by BS_LINK_POINTER.  */

static bpstat
bpstat_alloc (struct bp_location *bl, bpstat **bs_link_pointer)
{
  bpstat bs;

  bs = (bpstat) xmalloc (sizeof (*bs));
  bs->next = NULL;
  **bs_link_pointer = bs;
  *bs_link_pointer = &bs->next;
  bs->breakpoint_at = bl->owner;
  bs->bp_location_at = bl;
  incref_bp_location (bl);
  /* If the condition is false, etc., don't do the commands.  */
  bs->commands = NULL;
  bs->commands_left = NULL;
  bs->old_val = NULL;
  bs->print_it = print_it_normal;
  return bs;
}

/* The target has stopped with waitstatus WS.  Check if any hardware
   watchpoints have triggered, according to the target.  */

int
watchpoints_triggered (struct target_waitstatus *ws)
{
  int stopped_by_watchpoint = target_stopped_by_watchpoint ();
  CORE_ADDR addr;
  struct breakpoint *b;

  if (!stopped_by_watchpoint)
    {
      /* We were not stopped by a watchpoint.  Mark all watchpoints
	 as not triggered.  */
      ALL_BREAKPOINTS (b)
	if (is_hardware_watchpoint (b))
	  b->watchpoint_triggered = watch_triggered_no;

      return 0;
    }

  if (!target_stopped_data_address (&current_target, &addr))
    {
      /* We were stopped by a watchpoint, but we don't know where.
	 Mark all watchpoints as unknown.  */
      ALL_BREAKPOINTS (b)
	if (is_hardware_watchpoint (b))
	  b->watchpoint_triggered = watch_triggered_unknown;

      return stopped_by_watchpoint;
    }

  /* The target could report the data address.  Mark watchpoints
     affected by this data address as triggered, and all others as not
     triggered.  */

  ALL_BREAKPOINTS (b)
    if (is_hardware_watchpoint (b))
      {
	struct bp_location *loc;

	b->watchpoint_triggered = watch_triggered_no;
	for (loc = b->loc; loc; loc = loc->next)
	  /* Exact match not required.  Within range is
	     sufficient.  */
	  if (target_watchpoint_addr_within_range (&current_target,
						   addr, loc->address,
						   loc->length))
	    {
	      b->watchpoint_triggered = watch_triggered_yes;
	      break;
	    }
      }

  return 1;
}

/* Possible return values for watchpoint_check (this can't be an enum
   because of check_errors).  */
/* The watchpoint has been deleted.  */
#define WP_DELETED 1
/* The value has changed.  */
#define WP_VALUE_CHANGED 2
/* The value has not changed.  */
#define WP_VALUE_NOT_CHANGED 3
/* Ignore this watchpoint, no matter if the value changed or not.  */
#define WP_IGNORE 4

#define BP_TEMPFLAG 1
#define BP_HARDWAREFLAG 2

/* Evaluate watchpoint condition expression and check if its value
   changed.

   P should be a pointer to struct bpstat, but is defined as a void *
   in order for this function to be usable with catch_errors.  */

static int
watchpoint_check (void *p)
{
  bpstat bs = (bpstat) p;
  struct breakpoint *b;
  struct frame_info *fr;
  int within_current_scope;

  /* BS is built from an existing struct breakpoint.  */
  gdb_assert (bs->breakpoint_at != NULL);
  b = bs->breakpoint_at;

  gdb_assert (is_watchpoint (b));

  /* If this is a local watchpoint, we only want to check if the
     watchpoint frame is in scope if the current thread is the thread
     that was used to create the watchpoint.  */
  if (!watchpoint_in_thread_scope (b))
    return WP_IGNORE;

  if (b->exp_valid_block == NULL)
    within_current_scope = 1;
  else
    {
      struct frame_info *frame = get_current_frame ();
      struct gdbarch *frame_arch = get_frame_arch (frame);
      CORE_ADDR frame_pc = get_frame_pc (frame);

      /* in_function_epilogue_p() returns a non-zero value if we're
	 still in the function but the stack frame has already been
	 invalidated.  Since we can't rely on the values of local
	 variables after the stack has been destroyed, we are treating
	 the watchpoint in that state as `not changed' without further
	 checking.  Don't mark watchpoints as changed if the current
	 frame is in an epilogue - even if they are in some other
	 frame, our view of the stack is likely to be wrong and
	 frame_find_by_id could error out.  */
      if (gdbarch_in_function_epilogue_p (frame_arch, frame_pc))
	return WP_IGNORE;

      fr = frame_find_by_id (b->watchpoint_frame);
      within_current_scope = (fr != NULL);

      /* If we've gotten confused in the unwinder, we might have
	 returned a frame that can't describe this variable.  */
      if (within_current_scope)
	{
	  struct symbol *function;

	  function = get_frame_function (fr);
	  if (function == NULL
	      || !contained_in (b->exp_valid_block,
				SYMBOL_BLOCK_VALUE (function)))
	    within_current_scope = 0;
	}

      if (within_current_scope)
	/* If we end up stopping, the current frame will get selected
	   in normal_stop.  So this call to select_frame won't affect
	   the user.  */
	select_frame (fr);
    }

  if (within_current_scope)
    {
      /* We use value_{,free_to_}mark because it could be a *long*
         time before we return to the command level and call
         free_all_values.  We can't call free_all_values because we
         might be in the middle of evaluating a function call.  */

      int pc = 0;
      struct value *mark = value_mark ();
      struct value *new_val;

      fetch_subexp_value (b->exp, &pc, &new_val, NULL, NULL);

      /* We use value_equal_contents instead of value_equal because
	 the latter coerces an array to a pointer, thus comparing just
	 the address of the array instead of its contents.  This is
	 not what we want.  */
      if ((b->val != NULL) != (new_val != NULL)
	  || (b->val != NULL && !value_equal_contents (b->val, new_val)))
	{
	  if (new_val != NULL)
	    {
	      release_value (new_val);
	      value_free_to_mark (mark);
	    }
	  bs->old_val = b->val;
	  b->val = new_val;
	  b->val_valid = 1;
	  return WP_VALUE_CHANGED;
	}
      else
	{
	  /* Nothing changed.  */
	  value_free_to_mark (mark);
	  return WP_VALUE_NOT_CHANGED;
	}
    }
  else
    {
      /* This seems like the only logical thing to do because
         if we temporarily ignored the watchpoint, then when
         we reenter the block in which it is valid it contains
         garbage (in the case of a function, it may have two
         garbage values, one before and one after the prologue).
         So we can't even detect the first assignment to it and
         watch after that (since the garbage may or may not equal
         the first value assigned).  */
      /* We print all the stop information in print_it_typical(), but
	 in this case, by the time we call print_it_typical() this bp
	 will be deleted already.  So we have no choice but print the
	 information here.  */
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason", async_reason_lookup (EXEC_ASYNC_WATCHPOINT_SCOPE));
      ui_out_text (uiout, "\nWatchpoint ");
      ui_out_field_int (uiout, "wpnum", b->number);
      ui_out_text (uiout,
		   " deleted because the program has left the block in\n\
which its expression is valid.\n");     

      watchpoint_del_at_next_stop (b);

      return WP_DELETED;
    }
}

/* Return true if it looks like target has stopped due to hitting
   breakpoint location BL.  This function does not check if we
   should stop, only if BL explains the stop.   */
static int
bpstat_check_location (const struct bp_location *bl,
		       struct address_space *aspace, CORE_ADDR bp_addr)
{
  struct breakpoint *b = bl->owner;

  /* BL is from existing struct breakpoint.  */
  gdb_assert (b != NULL);

  if (b->ops && b->ops->breakpoint_hit)
    return b->ops->breakpoint_hit (bl, aspace, bp_addr);

  /* By definition, the inferior does not report stops at
     tracepoints.  */
  if (is_tracepoint (b))
    return 0;

  if (!is_watchpoint (b)
      && b->type != bp_hardware_breakpoint
      && b->type != bp_catchpoint)	/* a non-watchpoint bp */
    {
      if (!breakpoint_address_match (bl->pspace->aspace, bl->address,
				     aspace, bp_addr))
	return 0;
      if (overlay_debugging		/* unmapped overlay section */
	  && section_is_overlay (bl->section)
	  && !section_is_mapped (bl->section))
	return 0;
    }

  /* Continuable hardware watchpoints are treated as non-existent if the
     reason we stopped wasn't a hardware watchpoint (we didn't stop on
     some data address).  Otherwise gdb won't stop on a break instruction
     in the code (not from a breakpoint) when a hardware watchpoint has
     been defined.  Also skip watchpoints which we know did not trigger
     (did not match the data address).  */

  if (is_hardware_watchpoint (b)
      && b->watchpoint_triggered == watch_triggered_no)
    return 0;

  if (b->type == bp_hardware_breakpoint)
    {
      if (bl->address != bp_addr)
	return 0;
      if (overlay_debugging		/* unmapped overlay section */
	  && section_is_overlay (bl->section)
	  && !section_is_mapped (bl->section))
	return 0;
    }

  return 1;
}

/* If BS refers to a watchpoint, determine if the watched values
   has actually changed, and we should stop.  If not, set BS->stop
   to 0.  */
static void
bpstat_check_watchpoint (bpstat bs)
{
  const struct bp_location *bl;
  struct breakpoint *b;

  /* BS is built for existing struct breakpoint.  */
  bl = bs->bp_location_at;
  gdb_assert (bl != NULL);
  b = bs->breakpoint_at;
  gdb_assert (b != NULL);

  if (is_watchpoint (b))
    {
      int must_check_value = 0;
      
      if (b->type == bp_watchpoint)
	/* For a software watchpoint, we must always check the
	   watched value.  */
	must_check_value = 1;
      else if (b->watchpoint_triggered == watch_triggered_yes)
	/* We have a hardware watchpoint (read, write, or access)
	   and the target earlier reported an address watched by
	   this watchpoint.  */
	must_check_value = 1;
      else if (b->watchpoint_triggered == watch_triggered_unknown
	       && b->type == bp_hardware_watchpoint)
	/* We were stopped by a hardware watchpoint, but the target could
	   not report the data address.  We must check the watchpoint's
	   value.  Access and read watchpoints are out of luck; without
	   a data address, we can't figure it out.  */
	must_check_value = 1;
      
      if (must_check_value)
	{
	  char *message
	    = xstrprintf ("Error evaluating expression for watchpoint %d\n",
			  b->number);
	  struct cleanup *cleanups = make_cleanup (xfree, message);
	  int e = catch_errors (watchpoint_check, bs, message,
				RETURN_MASK_ALL);
	  do_cleanups (cleanups);
	  switch (e)
	    {
	    case WP_DELETED:
	      /* We've already printed what needs to be printed.  */
	      bs->print_it = print_it_done;
	      /* Stop.  */
	      break;
	    case WP_IGNORE:
	      bs->print_it = print_it_noop;
	      bs->stop = 0;
	      break;
	    case WP_VALUE_CHANGED:
	      if (b->type == bp_read_watchpoint)
		{
		  /* There are two cases to consider here:

		     1. We're watching the triggered memory for reads.
		     In that case, trust the target, and always report
		     the watchpoint hit to the user.  Even though
		     reads don't cause value changes, the value may
		     have changed since the last time it was read, and
		     since we're not trapping writes, we will not see
		     those, and as such we should ignore our notion of
		     old value.

		     2. We're watching the triggered memory for both
		     reads and writes.  There are two ways this may
		     happen:

		     2.1. This is a target that can't break on data
		     reads only, but can break on accesses (reads or
		     writes), such as e.g., x86.  We detect this case
		     at the time we try to insert read watchpoints.

		     2.2. Otherwise, the target supports read
		     watchpoints, but, the user set an access or write
		     watchpoint watching the same memory as this read
		     watchpoint.

		     If we're watching memory writes as well as reads,
		     ignore watchpoint hits when we find that the
		     value hasn't changed, as reads don't cause
		     changes.  This still gives false positives when
		     the program writes the same value to memory as
		     what there was already in memory (we will confuse
		     it for a read), but it's much better than
		     nothing.  */

		  int other_write_watchpoint = 0;

		  if (bl->watchpoint_type == hw_read)
		    {
		      struct breakpoint *other_b;

		      ALL_BREAKPOINTS (other_b)
			if ((other_b->type == bp_hardware_watchpoint
			     || other_b->type == bp_access_watchpoint)
			    && (other_b->watchpoint_triggered
				== watch_triggered_yes))
			  {
			    other_write_watchpoint = 1;
			    break;
			  }
		    }

		  if (other_write_watchpoint
		      || bl->watchpoint_type == hw_access)
		    {
		      /* We're watching the same memory for writes,
			 and the value changed since the last time we
			 updated it, so this trap must be for a write.
			 Ignore it.  */
		      bs->print_it = print_it_noop;
		      bs->stop = 0;
		    }
		}
	      break;
	    case WP_VALUE_NOT_CHANGED:
	      if (b->type == bp_hardware_watchpoint
		  || b->type == bp_watchpoint)
		{
		  /* Don't stop: write watchpoints shouldn't fire if
		     the value hasn't changed.  */
		  bs->print_it = print_it_noop;
		  bs->stop = 0;
		}
	      /* Stop.  */
	      break;
	    default:
	      /* Can't happen.  */
	    case 0:
	      /* Error from catch_errors.  */
	      printf_filtered (_("Watchpoint %d deleted.\n"), b->number);
	      watchpoint_del_at_next_stop (b);
	      /* We've already printed what needs to be printed.  */
	      bs->print_it = print_it_done;
	      break;
	    }
	}
      else	/* must_check_value == 0 */
	{
	  /* This is a case where some watchpoint(s) triggered, but
	     not at the address of this watchpoint, or else no
	     watchpoint triggered after all.  So don't print
	     anything for this watchpoint.  */
	  bs->print_it = print_it_noop;
	  bs->stop = 0;
	}
    }
}


/* Check conditions (condition proper, frame, thread and ignore count)
   of breakpoint referred to by BS.  If we should not stop for this
   breakpoint, set BS->stop to 0.  */

static void
bpstat_check_breakpoint_conditions (bpstat bs, ptid_t ptid)
{
  int thread_id = pid_to_thread_id (ptid);
  const struct bp_location *bl;
  struct breakpoint *b;

  /* BS is built for existing struct breakpoint.  */
  bl = bs->bp_location_at;
  gdb_assert (bl != NULL);
  b = bs->breakpoint_at;
  gdb_assert (b != NULL);

  if (frame_id_p (b->frame_id)
      && !frame_id_eq (b->frame_id, get_stack_frame_id (get_current_frame ())))
    bs->stop = 0;
  else if (bs->stop)
    {
      int value_is_zero = 0;
      struct expression *cond;

      /* Evaluate Python breakpoints that have a "stop"
	 method implemented.  */
      if (b->py_bp_object)
	bs->stop = gdbpy_should_stop (b->py_bp_object);

      if (is_watchpoint (b))
	cond = b->cond_exp;
      else
	cond = bl->cond;

      if (cond && b->disposition != disp_del_at_next_stop)
	{
	  int within_current_scope = 1;

	  /* We use value_mark and value_free_to_mark because it could
	     be a long time before we return to the command level and
	     call free_all_values.  We can't call free_all_values
	     because we might be in the middle of evaluating a
	     function call.  */
	  struct value *mark = value_mark ();

	  /* Need to select the frame, with all that implies so that
	     the conditions will have the right context.  Because we
	     use the frame, we will not see an inlined function's
	     variables when we arrive at a breakpoint at the start
	     of the inlined function; the current frame will be the
	     call site.  */
	  if (!is_watchpoint (b) || b->cond_exp_valid_block == NULL)
	    select_frame (get_current_frame ());
	  else
	    {
	      struct frame_info *frame;

	      /* For local watchpoint expressions, which particular
		 instance of a local is being watched matters, so we
		 keep track of the frame to evaluate the expression
		 in.  To evaluate the condition however, it doesn't
		 really matter which instantiation of the function
		 where the condition makes sense triggers the
		 watchpoint.  This allows an expression like "watch
		 global if q > 10" set in `func', catch writes to
		 global on all threads that call `func', or catch
		 writes on all recursive calls of `func' by a single
		 thread.  We simply always evaluate the condition in
		 the innermost frame that's executing where it makes
		 sense to evaluate the condition.  It seems
		 intuitive.  */
	      frame = block_innermost_frame (b->cond_exp_valid_block);
	      if (frame != NULL)
		select_frame (frame);
	      else
		within_current_scope = 0;
	    }
	  if (within_current_scope)
	    value_is_zero
	      = catch_errors (breakpoint_cond_eval, cond,
			      "Error in testing breakpoint condition:\n",
			      RETURN_MASK_ALL);
	  else
	    {
	      warning (_("Watchpoint condition cannot be tested "
			 "in the current scope"));
	      /* If we failed to set the right context for this
		 watchpoint, unconditionally report it.  */
	      value_is_zero = 0;
	    }
	  /* FIXME-someday, should give breakpoint #.  */
	  value_free_to_mark (mark);
	}

      if (cond && value_is_zero)
	{
	  bs->stop = 0;
	}
      else if (b->thread != -1 && b->thread != thread_id)
	{
	  bs->stop = 0;
	}
      else if (b->ignore_count > 0)
	{
	  b->ignore_count--;
	  annotate_ignore_count_change ();
	  bs->stop = 0;
	  /* Increase the hit count even though we don't stop.  */
	  ++(b->hit_count);
	}	
    }
}


/* Get a bpstat associated with having just stopped at address
   BP_ADDR in thread PTID.

   Determine whether we stopped at a breakpoint, etc, or whether we
   don't understand this stop.  Result is a chain of bpstat's such
   that:

   if we don't understand the stop, the result is a null pointer.

   if we understand why we stopped, the result is not null.

   Each element of the chain refers to a particular breakpoint or
   watchpoint at which we have stopped.  (We may have stopped for
   several reasons concurrently.)

   Each element of the chain has valid next, breakpoint_at,
   commands, FIXME??? fields.  */

bpstat
bpstat_stop_status (struct address_space *aspace,
		    CORE_ADDR bp_addr, ptid_t ptid)
{
  struct breakpoint *b = NULL;
  struct bp_location *bl;
  struct bp_location *loc;
  /* First item of allocated bpstat's.  */
  bpstat bs_head = NULL, *bs_link = &bs_head;
  /* Pointer to the last thing in the chain currently.  */
  bpstat bs;
  int ix;
  int need_remove_insert;
  int removed_any;

  /* First, build the bpstat chain with locations that explain a
     target stop, while being careful to not set the target running,
     as that may invalidate locations (in particular watchpoint
     locations are recreated).  Resuming will happen here with
     breakpoint conditions or watchpoint expressions that include
     inferior function calls.  */

  ALL_BREAKPOINTS (b)
    {
      if (!breakpoint_enabled (b) && b->enable_state != bp_permanent)
	continue;

      for (bl = b->loc; bl != NULL; bl = bl->next)
	{
	  /* For hardware watchpoints, we look only at the first
	     location.  The watchpoint_check function will work on the
	     entire expression, not the individual locations.  For
	     read watchpoints, the watchpoints_triggered function has
	     checked all locations already.  */
	  if (b->type == bp_hardware_watchpoint && bl != b->loc)
	    break;

	  if (bl->shlib_disabled)
	    continue;

	  if (!bpstat_check_location (bl, aspace, bp_addr))
	    continue;

	  /* Come here if it's a watchpoint, or if the break address
	     matches.  */

	  bs = bpstat_alloc (bl, &bs_link);	/* Alloc a bpstat to
						   explain stop.  */

	  /* Assume we stop.  Should we find a watchpoint that is not
	     actually triggered, or if the condition of the breakpoint
	     evaluates as false, we'll reset 'stop' to 0.  */
	  bs->stop = 1;
	  bs->print = 1;

	  /* If this is a scope breakpoint, mark the associated
	     watchpoint as triggered so that we will handle the
	     out-of-scope event.  We'll get to the watchpoint next
	     iteration.  */
	  if (b->type == bp_watchpoint_scope && b->related_breakpoint != b)
	    b->related_breakpoint->watchpoint_triggered = watch_triggered_yes;
	}
    }

  for (ix = 0; VEC_iterate (bp_location_p, moribund_locations, ix, loc); ++ix)
    {
      if (breakpoint_location_address_match (loc, aspace, bp_addr))
	{
	  bs = bpstat_alloc (loc, &bs_link);
	  /* For hits of moribund locations, we should just proceed.  */
	  bs->stop = 0;
	  bs->print = 0;
	  bs->print_it = print_it_noop;
	}
    }

  /* Now go through the locations that caused the target to stop, and
     check whether we're interested in reporting this stop to higher
     layers, or whether we should resume the target transparently.  */

  removed_any = 0;

  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      if (!bs->stop)
	continue;

      bpstat_check_watchpoint (bs);
      if (!bs->stop)
	continue;

      b = bs->breakpoint_at;

	  if (b->type == bp_thread_event || b->type == bp_overlay_event
	      || b->type == bp_longjmp_master
	      || b->type == bp_std_terminate_master
	      || b->type == bp_exception_master)
	    /* We do not stop for these.  */
	    bs->stop = 0;
	  else
	    bpstat_check_breakpoint_conditions (bs, ptid);

	  if (bs->stop)
	    {
	      ++(b->hit_count);

	      /* We will stop here.  */
	      if (b->disposition == disp_disable)
		{
		  if (b->enable_state != bp_permanent)
		    b->enable_state = bp_disabled;
		  removed_any = 1;
		}
	      if (b->silent)
		bs->print = 0;
	      bs->commands = b->commands;
	      incref_counted_command_line (bs->commands);
	      bs->commands_left = bs->commands ? bs->commands->commands : NULL;
	      if (bs->commands_left
		  && (strcmp ("silent", bs->commands_left->line) == 0
		      || (xdb_commands
			  && strcmp ("Q",
				     bs->commands_left->line) == 0)))
		{
		  bs->commands_left = bs->commands_left->next;
		  bs->print = 0;
		}
	    }

	  /* Print nothing for this entry if we dont stop or dont print.  */
	  if (bs->stop == 0 || bs->print == 0)
	    bs->print_it = print_it_noop;
    }

  /* If we aren't stopping, the value of some hardware watchpoint may
     not have changed, but the intermediate memory locations we are
     watching may have.  Don't bother if we're stopping; this will get
     done later.  */
  need_remove_insert = 0;
  if (! bpstat_causes_stop (bs_head))
    for (bs = bs_head; bs != NULL; bs = bs->next)
      if (!bs->stop
	  && bs->breakpoint_at
	  && is_hardware_watchpoint (bs->breakpoint_at))
	{
	  update_watchpoint (bs->breakpoint_at, 0 /* don't reparse.  */);
	  need_remove_insert = 1;
	}

  if (need_remove_insert)
    update_global_location_list (1);
  else if (removed_any)
    update_global_location_list (0);

  return bs_head;
}

static void
handle_jit_event (void)
{
  struct frame_info *frame;
  struct gdbarch *gdbarch;

  /* Switch terminal for any messages produced by
     breakpoint_re_set.  */
  target_terminal_ours_for_output ();

  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);

  jit_event_handler (gdbarch);

  target_terminal_inferior ();
}

/* Prepare WHAT final decision for infrun.  */

/* Decide what infrun needs to do with this bpstat.  */

struct bpstat_what
bpstat_what (bpstat bs_head)
{
  struct bpstat_what retval;
  /* We need to defer calling `solib_add', as adding new symbols
     resets breakpoints, which in turn deletes breakpoint locations,
     and hence may clear unprocessed entries in the BS chain.  */
  int shlib_event = 0;
  int jit_event = 0;
  bpstat bs;

  retval.main_action = BPSTAT_WHAT_KEEP_CHECKING;
  retval.call_dummy = STOP_NONE;
  retval.is_longjmp = 0;

  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      /* Extract this BS's action.  After processing each BS, we check
	 if its action overrides all we've seem so far.  */
      enum bpstat_what_main_action this_action = BPSTAT_WHAT_KEEP_CHECKING;
      enum bptype bptype;

      if (bs->breakpoint_at == NULL)
	{
	  /* I suspect this can happen if it was a momentary
	     breakpoint which has since been deleted.  */
	  bptype = bp_none;
	}
      else if (bs->breakpoint_at == NULL)
	bptype = bp_none;
      else
	bptype = bs->breakpoint_at->type;

      switch (bptype)
	{
	case bp_none:
	  break;
	case bp_breakpoint:
	case bp_hardware_breakpoint:
	case bp_until:
	case bp_finish:
	  if (bs->stop)
	    {
	      if (bs->print)
		this_action = BPSTAT_WHAT_STOP_NOISY;
	      else
		this_action = BPSTAT_WHAT_STOP_SILENT;
	    }
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_watchpoint:
	case bp_hardware_watchpoint:
	case bp_read_watchpoint:
	case bp_access_watchpoint:
	  if (bs->stop)
	    {
	      if (bs->print)
		this_action = BPSTAT_WHAT_STOP_NOISY;
	      else
		this_action = BPSTAT_WHAT_STOP_SILENT;
	    }
	  else
	    {
	      /* There was a watchpoint, but we're not stopping.
		 This requires no further action.  */
	    }
	  break;
	case bp_longjmp:
	case bp_exception:
	  this_action = BPSTAT_WHAT_SET_LONGJMP_RESUME;
	  retval.is_longjmp = bptype == bp_longjmp;
	  break;
	case bp_longjmp_resume:
	case bp_exception_resume:
	  this_action = BPSTAT_WHAT_CLEAR_LONGJMP_RESUME;
	  retval.is_longjmp = bptype == bp_longjmp_resume;
	  break;
	case bp_step_resume:
	  if (bs->stop)
	    this_action = BPSTAT_WHAT_STEP_RESUME;
	  else
	    {
	      /* It is for the wrong frame.  */
	      this_action = BPSTAT_WHAT_SINGLE;
	    }
	  break;
	case bp_watchpoint_scope:
	case bp_thread_event:
	case bp_overlay_event:
	case bp_longjmp_master:
	case bp_std_terminate_master:
	case bp_exception_master:
	  this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_catchpoint:
	  if (bs->stop)
	    {
	      if (bs->print)
		this_action = BPSTAT_WHAT_STOP_NOISY;
	      else
		this_action = BPSTAT_WHAT_STOP_SILENT;
	    }
	  else
	    {
	      /* There was a catchpoint, but we're not stopping.
		 This requires no further action.  */
	    }
	  break;
	case bp_shlib_event:
	  shlib_event = 1;

	  /* If requested, stop when the dynamic linker notifies GDB
	     of events.  This allows the user to get control and place
	     breakpoints in initializer routines for dynamically
	     loaded objects (among other things).  */
	  if (stop_on_solib_events)
	    this_action = BPSTAT_WHAT_STOP_NOISY;
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_jit_event:
	  jit_event = 1;
	  this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_call_dummy:
	  /* Make sure the action is stop (silent or noisy),
	     so infrun.c pops the dummy frame.  */
	  retval.call_dummy = STOP_STACK_DUMMY;
	  this_action = BPSTAT_WHAT_STOP_SILENT;
	  break;
	case bp_std_terminate:
	  /* Make sure the action is stop (silent or noisy),
	     so infrun.c pops the dummy frame.  */
	  retval.call_dummy = STOP_STD_TERMINATE;
	  this_action = BPSTAT_WHAT_STOP_SILENT;
	  break;
	case bp_tracepoint:
	case bp_fast_tracepoint:
	case bp_static_tracepoint:
	  /* Tracepoint hits should not be reported back to GDB, and
	     if one got through somehow, it should have been filtered
	     out already.  */
	  internal_error (__FILE__, __LINE__,
			  _("bpstat_what: tracepoint encountered"));
	  break;
	case bp_gnu_ifunc_resolver:
	  /* Step over it (and insert bp_gnu_ifunc_resolver_return).  */
	  this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_gnu_ifunc_resolver_return:
	  /* The breakpoint will be removed, execution will restart from the
	     PC of the former breakpoint.  */
	  this_action = BPSTAT_WHAT_KEEP_CHECKING;
	  break;
	default:
	  internal_error (__FILE__, __LINE__,
			  _("bpstat_what: unhandled bptype %d"), (int) bptype);
	}

      retval.main_action = max (retval.main_action, this_action);
    }

  /* These operations may affect the bs->breakpoint_at state so they are
     delayed after MAIN_ACTION is decided above.  */

  if (shlib_event)
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "bpstat_what: bp_shlib_event\n");

      /* Check for any newly added shared libraries if we're supposed
	 to be adding them automatically.  */

      /* Switch terminal for any messages produced by
	 breakpoint_re_set.  */
      target_terminal_ours_for_output ();

#ifdef SOLIB_ADD
      SOLIB_ADD (NULL, 0, &current_target, auto_solib_add);
#else
      solib_add (NULL, 0, &current_target, auto_solib_add);
#endif

      target_terminal_inferior ();
    }

  if (jit_event)
    {
      if (debug_infrun)
	fprintf_unfiltered (gdb_stdlog, "bpstat_what: bp_jit_event\n");

      handle_jit_event ();
    }

  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      struct breakpoint *b = bs->breakpoint_at;

      if (b == NULL)
	continue;
      switch (b->type)
	{
	case bp_gnu_ifunc_resolver:
	  gnu_ifunc_resolver_stop (b);
	  break;
	case bp_gnu_ifunc_resolver_return:
	  gnu_ifunc_resolver_return_stop (b);
	  break;
	}
    }

  return retval;
}

/* Nonzero if we should step constantly (e.g. watchpoints on machines
   without hardware support).  This isn't related to a specific bpstat,
   just to things like whether watchpoints are set.  */

int
bpstat_should_step (void)
{
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
    if (breakpoint_enabled (b) && b->type == bp_watchpoint && b->loc != NULL)
      return 1;
  return 0;
}

int
bpstat_causes_stop (bpstat bs)
{
  for (; bs != NULL; bs = bs->next)
    if (bs->stop)
      return 1;

  return 0;
}



/* Compute a string of spaces suitable to indent the next line
   so it starts at the position corresponding to the table column
   named COL_NAME in the currently active table of UIOUT.  */

static char *
wrap_indent_at_field (struct ui_out *uiout, const char *col_name)
{
  static char wrap_indent[80];
  int i, total_width, width, align;
  char *text;

  total_width = 0;
  for (i = 1; ui_out_query_field (uiout, i, &width, &align, &text); i++)
    {
      if (strcmp (text, col_name) == 0)
	{
	  gdb_assert (total_width < sizeof wrap_indent);
	  memset (wrap_indent, ' ', total_width);
	  wrap_indent[total_width] = 0;

	  return wrap_indent;
	}

      total_width += width + 1;
    }

  return NULL;
}

/* Print the LOC location out of the list of B->LOC locations.  */

static void
print_breakpoint_location (struct breakpoint *b,
			   struct bp_location *loc)
{
  struct cleanup *old_chain = save_current_program_space ();

  if (loc != NULL && loc->shlib_disabled)
    loc = NULL;

  if (loc != NULL)
    set_current_program_space (loc->pspace);

  if (b->display_canonical)
    ui_out_field_string (uiout, "what", b->addr_string);
  else if (b->source_file && loc)
    {
      struct symbol *sym 
	= find_pc_sect_function (loc->address, loc->section);
      if (sym)
	{
	  ui_out_text (uiout, "in ");
	  ui_out_field_string (uiout, "func",
			       SYMBOL_PRINT_NAME (sym));
	  ui_out_text (uiout, " ");
	  ui_out_wrap_hint (uiout, wrap_indent_at_field (uiout, "what"));
	  ui_out_text (uiout, "at ");
	}
      ui_out_field_string (uiout, "file", b->source_file);
      ui_out_text (uiout, ":");
      
      if (ui_out_is_mi_like_p (uiout))
	{
	  struct symtab_and_line sal = find_pc_line (loc->address, 0);
	  char *fullname = symtab_to_fullname (sal.symtab);
	  
	  if (fullname)
	    ui_out_field_string (uiout, "fullname", fullname);
	}
      
      ui_out_field_int (uiout, "line", b->line_number);
    }
  else if (loc)
    {
      struct ui_stream *stb = ui_out_stream_new (uiout);
      struct cleanup *stb_chain = make_cleanup_ui_out_stream_delete (stb);

      print_address_symbolic (loc->gdbarch, loc->address, stb->stream,
			      demangle, "");
      ui_out_field_stream (uiout, "at", stb);

      do_cleanups (stb_chain);
    }
  else
    ui_out_field_string (uiout, "pending", b->addr_string);

  do_cleanups (old_chain);
}

static const char *
bptype_string (enum bptype type)
{
  struct ep_type_description
    {
      enum bptype type;
      char *description;
    };
  static struct ep_type_description bptypes[] =
  {
    {bp_none, "?deleted?"},
    {bp_breakpoint, "breakpoint"},
    {bp_hardware_breakpoint, "hw breakpoint"},
    {bp_until, "until"},
    {bp_finish, "finish"},
    {bp_watchpoint, "watchpoint"},
    {bp_hardware_watchpoint, "hw watchpoint"},
    {bp_read_watchpoint, "read watchpoint"},
    {bp_access_watchpoint, "acc watchpoint"},
    {bp_longjmp, "longjmp"},
    {bp_longjmp_resume, "longjmp resume"},
    {bp_exception, "exception"},
    {bp_exception_resume, "exception resume"},
    {bp_step_resume, "step resume"},
    {bp_watchpoint_scope, "watchpoint scope"},
    {bp_call_dummy, "call dummy"},
    {bp_std_terminate, "std::terminate"},
    {bp_shlib_event, "shlib events"},
    {bp_thread_event, "thread events"},
    {bp_overlay_event, "overlay events"},
    {bp_longjmp_master, "longjmp master"},
    {bp_std_terminate_master, "std::terminate master"},
    {bp_exception_master, "exception master"},
    {bp_catchpoint, "catchpoint"},
    {bp_tracepoint, "tracepoint"},
    {bp_fast_tracepoint, "fast tracepoint"},
    {bp_static_tracepoint, "static tracepoint"},
    {bp_jit_event, "jit events"},
    {bp_gnu_ifunc_resolver, "STT_GNU_IFUNC resolver"},
    {bp_gnu_ifunc_resolver_return, "STT_GNU_IFUNC resolver return"},
  };

  if (((int) type >= (sizeof (bptypes) / sizeof (bptypes[0])))
      || ((int) type != bptypes[(int) type].type))
    internal_error (__FILE__, __LINE__,
		    _("bptypes table does not describe type #%d."),
		    (int) type);

  return bptypes[(int) type].description;
}

/* Print B to gdb_stdout.  */

static void
print_one_breakpoint_location (struct breakpoint *b,
			       struct bp_location *loc,
			       int loc_number,
			       struct bp_location **last_loc,
			       int allflag)
{
  struct command_line *l;
  static char bpenables[] = "nynny";
  struct cleanup *bkpt_chain;

  int header_of_multiple = 0;
  int part_of_multiple = (loc != NULL);
  struct value_print_options opts;

  get_user_print_options (&opts);

  gdb_assert (!loc || loc_number != 0);
  /* See comment in print_one_breakpoint concerning treatment of
     breakpoints with single disabled location.  */
  if (loc == NULL 
      && (b->loc != NULL 
	  && (b->loc->next != NULL || !b->loc->enabled)))
    header_of_multiple = 1;
  if (loc == NULL)
    loc = b->loc;

  annotate_record ();
  bkpt_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "bkpt");

  /* 1 */
  annotate_field (0);
  if (part_of_multiple)
    {
      char *formatted;
      formatted = xstrprintf ("%d.%d", b->number, loc_number);
      ui_out_field_string (uiout, "number", formatted);
      xfree (formatted);
    }
  else
    {
      ui_out_field_int (uiout, "number", b->number);
    }

  /* 2 */
  annotate_field (1);
  if (part_of_multiple)
    ui_out_field_skip (uiout, "type");
  else
    ui_out_field_string (uiout, "type", bptype_string (b->type));

  /* 3 */
  annotate_field (2);
  if (part_of_multiple)
    ui_out_field_skip (uiout, "disp");
  else
    ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));


  /* 4 */
  annotate_field (3);
  if (part_of_multiple)
    ui_out_field_string (uiout, "enabled", loc->enabled ? "y" : "n");
  else
    ui_out_field_fmt (uiout, "enabled", "%c", 
		      bpenables[(int) b->enable_state]);
  ui_out_spaces (uiout, 2);

  
  /* 5 and 6 */
  if (b->ops != NULL && b->ops->print_one != NULL)
    {
      /* Although the print_one can possibly print all locations,
	 calling it here is not likely to get any nice result.  So,
	 make sure there's just one location.  */
      gdb_assert (b->loc == NULL || b->loc->next == NULL);
      b->ops->print_one (b, last_loc);
    }
  else
    switch (b->type)
      {
      case bp_none:
	internal_error (__FILE__, __LINE__,
			_("print_one_breakpoint: bp_none encountered\n"));
	break;

      case bp_watchpoint:
      case bp_hardware_watchpoint:
      case bp_read_watchpoint:
      case bp_access_watchpoint:
	/* Field 4, the address, is omitted (which makes the columns
	   not line up too nicely with the headers, but the effect
	   is relatively readable).  */
	if (opts.addressprint)
	  ui_out_field_skip (uiout, "addr");
	annotate_field (5);
	ui_out_field_string (uiout, "what", b->exp_string);
	break;

      case bp_breakpoint:
      case bp_hardware_breakpoint:
      case bp_until:
      case bp_finish:
      case bp_longjmp:
      case bp_longjmp_resume:
      case bp_exception:
      case bp_exception_resume:
      case bp_step_resume:
      case bp_watchpoint_scope:
      case bp_call_dummy:
      case bp_std_terminate:
      case bp_shlib_event:
      case bp_thread_event:
      case bp_overlay_event:
      case bp_longjmp_master:
      case bp_std_terminate_master:
      case bp_exception_master:
      case bp_tracepoint:
      case bp_fast_tracepoint:
      case bp_static_tracepoint:
      case bp_jit_event:
      case bp_gnu_ifunc_resolver:
      case bp_gnu_ifunc_resolver_return:
	if (opts.addressprint)
	  {
	    annotate_field (4);
	    if (header_of_multiple)
	      ui_out_field_string (uiout, "addr", "<MULTIPLE>");
	    else if (b->loc == NULL || loc->shlib_disabled)
	      ui_out_field_string (uiout, "addr", "<PENDING>");
	    else
	      ui_out_field_core_addr (uiout, "addr",
				      loc->gdbarch, loc->address);
	  }
	annotate_field (5);
	if (!header_of_multiple)
	  print_breakpoint_location (b, loc);
	if (b->loc)
	  *last_loc = b->loc;
	break;
      }


  /* For backward compatibility, don't display inferiors unless there
     are several.  */
  if (loc != NULL
      && !header_of_multiple
      && (allflag
	  || (!gdbarch_has_global_breakpoints (target_gdbarch)
	      && (number_of_program_spaces () > 1
		  || number_of_inferiors () > 1)
	      /* LOC is for existing B, it cannot be in
		 moribund_locations and thus having NULL OWNER.  */
	      && loc->owner->type != bp_catchpoint)))
    {
      struct inferior *inf;
      int first = 1;

      for (inf = inferior_list; inf != NULL; inf = inf->next)
	{
	  if (inf->pspace == loc->pspace)
	    {
	      if (first)
		{
		  first = 0;
		  ui_out_text (uiout, " inf ");
		}
	      else
		ui_out_text (uiout, ", ");
	      ui_out_text (uiout, plongest (inf->num));
	    }
	}
    }

  if (!part_of_multiple)
    {
      if (b->thread != -1)
	{
	  /* FIXME: This seems to be redundant and lost here; see the
	     "stop only in" line a little further down.  */
	  ui_out_text (uiout, " thread ");
	  ui_out_field_int (uiout, "thread", b->thread);
	}
      else if (b->task != 0)
	{
	  ui_out_text (uiout, " task ");
	  ui_out_field_int (uiout, "task", b->task);
	}
    }

  ui_out_text (uiout, "\n");

  if (!part_of_multiple && b->ops && b->ops->print_one_detail)
    b->ops->print_one_detail (b, uiout);

  if (!part_of_multiple && b->static_trace_marker_id)
    {
      gdb_assert (b->type == bp_static_tracepoint);

      ui_out_text (uiout, "\tmarker id is ");
      ui_out_field_string (uiout, "static-tracepoint-marker-string-id",
			   b->static_trace_marker_id);
      ui_out_text (uiout, "\n");
    }

  if (part_of_multiple && frame_id_p (b->frame_id))
    {
      annotate_field (6);
      ui_out_text (uiout, "\tstop only in stack frame at ");
      /* FIXME: cagney/2002-12-01: Shouldn't be poeking around inside
         the frame ID.  */
      ui_out_field_core_addr (uiout, "frame",
			      b->gdbarch, b->frame_id.stack_addr);
      ui_out_text (uiout, "\n");
    }
  
  if (!part_of_multiple && b->cond_string && !ada_exception_catchpoint_p (b))
    {
      /* We do not print the condition for Ada exception catchpoints
         because the condition is an internal implementation detail
         that we do not want to expose to the user.  */
      annotate_field (7);
      if (is_tracepoint (b))
	ui_out_text (uiout, "\ttrace only if ");
      else
	ui_out_text (uiout, "\tstop only if ");
      ui_out_field_string (uiout, "cond", b->cond_string);
      ui_out_text (uiout, "\n");
    }

  if (!part_of_multiple && b->thread != -1)
    {
      /* FIXME should make an annotation for this.  */
      ui_out_text (uiout, "\tstop only in thread ");
      ui_out_field_int (uiout, "thread", b->thread);
      ui_out_text (uiout, "\n");
    }
  
  if (!part_of_multiple && b->hit_count)
    {
      /* FIXME should make an annotation for this.  */
      if (ep_is_catchpoint (b))
	ui_out_text (uiout, "\tcatchpoint");
      else
	ui_out_text (uiout, "\tbreakpoint");
      ui_out_text (uiout, " already hit ");
      ui_out_field_int (uiout, "times", b->hit_count);
      if (b->hit_count == 1)
	ui_out_text (uiout, " time\n");
      else
	ui_out_text (uiout, " times\n");
    }
  
  /* Output the count also if it is zero, but only if this is mi.
     FIXME: Should have a better test for this.  */
  if (ui_out_is_mi_like_p (uiout))
    if (!part_of_multiple && b->hit_count == 0)
      ui_out_field_int (uiout, "times", b->hit_count);

  if (!part_of_multiple && b->ignore_count)
    {
      annotate_field (8);
      ui_out_text (uiout, "\tignore next ");
      ui_out_field_int (uiout, "ignore", b->ignore_count);
      ui_out_text (uiout, " hits\n");
    }

  l = b->commands ? b->commands->commands : NULL;
  if (!part_of_multiple && l)
    {
      struct cleanup *script_chain;

      annotate_field (9);
      script_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "script");
      print_command_lines (uiout, l, 4);
      do_cleanups (script_chain);
    }

  if (!part_of_multiple && b->pass_count)
    {
      annotate_field (10);
      ui_out_text (uiout, "\tpass count ");
      ui_out_field_int (uiout, "pass", b->pass_count);
      ui_out_text (uiout, " \n");
    }

  if (ui_out_is_mi_like_p (uiout) && !part_of_multiple)
    {
      if (b->addr_string)
	ui_out_field_string (uiout, "original-location", b->addr_string);
      else if (b->exp_string)
	ui_out_field_string (uiout, "original-location", b->exp_string);
    }
	
  do_cleanups (bkpt_chain);
}

static void
print_one_breakpoint (struct breakpoint *b,
		      struct bp_location **last_loc, 
		      int allflag)
{
  print_one_breakpoint_location (b, NULL, 0, last_loc, allflag);

  /* If this breakpoint has custom print function,
     it's already printed.  Otherwise, print individual
     locations, if any.  */
  if (b->ops == NULL || b->ops->print_one == NULL)
    {
      /* If breakpoint has a single location that is disabled, we
	 print it as if it had several locations, since otherwise it's
	 hard to represent "breakpoint enabled, location disabled"
	 situation.

	 Note that while hardware watchpoints have several locations
	 internally, that's not a property exposed to user.  */
      if (b->loc 
	  && !is_hardware_watchpoint (b)
	  && (b->loc->next || !b->loc->enabled)
	  && !ui_out_is_mi_like_p (uiout)) 
	{
	  struct bp_location *loc;
	  int n = 1;
	  for (loc = b->loc; loc; loc = loc->next, ++n)
	    print_one_breakpoint_location (b, loc, n, last_loc, allflag);
	}
    }
}

static int
breakpoint_address_bits (struct breakpoint *b)
{
  int print_address_bits = 0;
  struct bp_location *loc;

  for (loc = b->loc; loc; loc = loc->next)
    {
      int addr_bit;

      /* Software watchpoints that aren't watching memory don't have
	 an address to print.  */
      if (b->type == bp_watchpoint && loc->watchpoint_type == -1)
	continue;

      addr_bit = gdbarch_addr_bit (loc->gdbarch);
      if (addr_bit > print_address_bits)
	print_address_bits = addr_bit;
    }

  return print_address_bits;
}

struct captured_breakpoint_query_args
  {
    int bnum;
  };

static int
do_captured_breakpoint_query (struct ui_out *uiout, void *data)
{
  struct captured_breakpoint_query_args *args = data;
  struct breakpoint *b;
  struct bp_location *dummy_loc = NULL;

  ALL_BREAKPOINTS (b)
    {
      if (args->bnum == b->number)
	{
	  print_one_breakpoint (b, &dummy_loc, 0);
	  return GDB_RC_OK;
	}
    }
  return GDB_RC_NONE;
}

enum gdb_rc
gdb_breakpoint_query (struct ui_out *uiout, int bnum, 
		      char **error_message)
{
  struct captured_breakpoint_query_args args;

  args.bnum = bnum;
  /* For the moment we don't trust print_one_breakpoint() to not throw
     an error.  */
  if (catch_exceptions_with_msg (uiout, do_captured_breakpoint_query, &args,
				 error_message, RETURN_MASK_ALL) < 0)
    return GDB_RC_FAIL;
  else
    return GDB_RC_OK;
}

/* Return non-zero if B is user settable (breakpoints, watchpoints,
   catchpoints, et.al.).  */

static int
user_settable_breakpoint (const struct breakpoint *b)
{
  return (b->type == bp_breakpoint
	  || b->type == bp_catchpoint
	  || b->type == bp_hardware_breakpoint
	  || is_tracepoint (b)
	  || is_watchpoint (b)
	  || b->type == bp_gnu_ifunc_resolver);
}

/* Return true if this breakpoint was set by the user, false if it is
   internal or momentary.  */

int
user_breakpoint_p (struct breakpoint *b)
{
  return user_settable_breakpoint (b) && b->number > 0;
}

/* Print information on user settable breakpoint (watchpoint, etc)
   number BNUM.  If BNUM is -1 print all user-settable breakpoints.
   If ALLFLAG is non-zero, include non-user-settable breakpoints.  If
   FILTER is non-NULL, call it on each breakpoint and only include the
   ones for which it returns non-zero.  Return the total number of
   breakpoints listed.  */

static int
breakpoint_1 (char *args, int allflag, 
	      int (*filter) (const struct breakpoint *))
{
  struct breakpoint *b;
  struct bp_location *last_loc = NULL;
  int nr_printable_breakpoints;
  struct cleanup *bkpttbl_chain;
  struct value_print_options opts;
  int print_address_bits = 0;
  int print_type_col_width = 14;

  get_user_print_options (&opts);

  /* Compute the number of rows in the table, as well as the size
     required for address fields.  */
  nr_printable_breakpoints = 0;
  ALL_BREAKPOINTS (b)
    {
      /* If we have a filter, only list the breakpoints it accepts.  */
      if (filter && !filter (b))
	continue;

      /* If we have an "args" string, it is a list of breakpoints to 
	 accept.  Skip the others.  */
      if (args != NULL && *args != '\0')
	{
	  if (allflag && parse_and_eval_long (args) != b->number)
	    continue;
	  if (!allflag && !number_is_in_list (args, b->number))
	    continue;
	}

      if (allflag || user_breakpoint_p (b))
	{
	  int addr_bit, type_len;

	  addr_bit = breakpoint_address_bits (b);
	  if (addr_bit > print_address_bits)
	    print_address_bits = addr_bit;

	  type_len = strlen (bptype_string (b->type));
	  if (type_len > print_type_col_width)
	    print_type_col_width = type_len;

	  nr_printable_breakpoints++;
	}
    }

  if (opts.addressprint)
    bkpttbl_chain 
      = make_cleanup_ui_out_table_begin_end (uiout, 6,
					     nr_printable_breakpoints,
                                             "BreakpointTable");
  else
    bkpttbl_chain 
      = make_cleanup_ui_out_table_begin_end (uiout, 5,
					     nr_printable_breakpoints,
                                             "BreakpointTable");

  if (nr_printable_breakpoints > 0)
    annotate_breakpoints_headers ();
  if (nr_printable_breakpoints > 0)
    annotate_field (0);
  ui_out_table_header (uiout, 7, ui_left, "number", "Num");	/* 1 */
  if (nr_printable_breakpoints > 0)
    annotate_field (1);
  ui_out_table_header (uiout, print_type_col_width, ui_left,
		       "type", "Type");				/* 2 */
  if (nr_printable_breakpoints > 0)
    annotate_field (2);
  ui_out_table_header (uiout, 4, ui_left, "disp", "Disp");	/* 3 */
  if (nr_printable_breakpoints > 0)
    annotate_field (3);
  ui_out_table_header (uiout, 3, ui_left, "enabled", "Enb");	/* 4 */
  if (opts.addressprint)
    {
      if (nr_printable_breakpoints > 0)
	annotate_field (4);
      if (print_address_bits <= 32)
	ui_out_table_header (uiout, 10, ui_left, 
			     "addr", "Address");		/* 5 */
      else
	ui_out_table_header (uiout, 18, ui_left, 
			     "addr", "Address");		/* 5 */
    }
  if (nr_printable_breakpoints > 0)
    annotate_field (5);
  ui_out_table_header (uiout, 40, ui_noalign, "what", "What");	/* 6 */
  ui_out_table_body (uiout);
  if (nr_printable_breakpoints > 0)
    annotate_breakpoints_table ();

  ALL_BREAKPOINTS (b)
    {
      QUIT;
      /* If we have a filter, only list the breakpoints it accepts.  */
      if (filter && !filter (b))
	continue;

      /* If we have an "args" string, it is a list of breakpoints to 
	 accept.  Skip the others.  */

      if (args != NULL && *args != '\0')
	{
	  if (allflag)	/* maintenance info breakpoint */
	    {
	      if (parse_and_eval_long (args) != b->number)
		continue;
	    }
	  else		/* all others */
	    {
	      if (!number_is_in_list (args, b->number))
		continue;
	    }
	}
      /* We only print out user settable breakpoints unless the
	 allflag is set.  */
      if (allflag || user_breakpoint_p (b))
	print_one_breakpoint (b, &last_loc, allflag);
    }

  do_cleanups (bkpttbl_chain);

  if (nr_printable_breakpoints == 0)
    {
      /* If there's a filter, let the caller decide how to report
	 empty list.  */
      if (!filter)
	{
	  if (args == NULL || *args == '\0')
	    ui_out_message (uiout, 0, "No breakpoints or watchpoints.\n");
	  else
	    ui_out_message (uiout, 0, 
			    "No breakpoint or watchpoint matching '%s'.\n",
			    args);
	}
    }
  else
    {
      if (last_loc && !server_command)
	set_next_address (last_loc->gdbarch, last_loc->address);
    }

  /* FIXME?  Should this be moved up so that it is only called when
     there have been breakpoints? */
  annotate_breakpoints_table_end ();

  return nr_printable_breakpoints;
}

/* Display the value of default-collect in a way that is generally
   compatible with the breakpoint list.  */

static void
default_collect_info (void)
{
  /* If it has no value (which is frequently the case), say nothing; a
     message like "No default-collect." gets in user's face when it's
     not wanted.  */
  if (!*default_collect)
    return;

  /* The following phrase lines up nicely with per-tracepoint collect
     actions.  */
  ui_out_text (uiout, "default collect ");
  ui_out_field_string (uiout, "default-collect", default_collect);
  ui_out_text (uiout, " \n");
}
  
static void
breakpoints_info (char *args, int from_tty)
{
  breakpoint_1 (args, 0, NULL);

  default_collect_info ();
}

static void
watchpoints_info (char *args, int from_tty)
{
  int num_printed = breakpoint_1 (args, 0, is_watchpoint);

  if (num_printed == 0)
    {
      if (args == NULL || *args == '\0')
	ui_out_message (uiout, 0, "No watchpoints.\n");
      else
	ui_out_message (uiout, 0, "No watchpoint matching '%s'.\n", args);
    }
}

static void
maintenance_info_breakpoints (char *args, int from_tty)
{
  breakpoint_1 (args, 1, NULL);

  default_collect_info ();
}

static int
breakpoint_has_pc (struct breakpoint *b,
		   struct program_space *pspace,
		   CORE_ADDR pc, struct obj_section *section)
{
  struct bp_location *bl = b->loc;

  for (; bl; bl = bl->next)
    {
      if (bl->pspace == pspace
	  && bl->address == pc
	  && (!overlay_debugging || bl->section == section))
	return 1;	  
    }
  return 0;
}

/* Print a message describing any breakpoints set at PC.  This
   concerns with logical breakpoints, so we match program spaces, not
   address spaces.  */

static void
describe_other_breakpoints (struct gdbarch *gdbarch,
			    struct program_space *pspace, CORE_ADDR pc,
			    struct obj_section *section, int thread)
{
  int others = 0;
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
    others += breakpoint_has_pc (b, pspace, pc, section);
  if (others > 0)
    {
      if (others == 1)
	printf_filtered (_("Note: breakpoint "));
      else /* if (others == ???) */
	printf_filtered (_("Note: breakpoints "));
      ALL_BREAKPOINTS (b)
	if (breakpoint_has_pc (b, pspace, pc, section))
	  {
	    others--;
	    printf_filtered ("%d", b->number);
	    if (b->thread == -1 && thread != -1)
	      printf_filtered (" (all threads)");
	    else if (b->thread != -1)
	      printf_filtered (" (thread %d)", b->thread);
	    printf_filtered ("%s%s ",
			     ((b->enable_state == bp_disabled
			       || b->enable_state == bp_call_disabled
			       || b->enable_state == bp_startup_disabled)
			      ? " (disabled)"
			      : b->enable_state == bp_permanent 
			      ? " (permanent)"
			      : ""),
			     (others > 1) ? "," 
			     : ((others == 1) ? " and" : ""));
	  }
      printf_filtered (_("also set at pc "));
      fputs_filtered (paddress (gdbarch, pc), gdb_stdout);
      printf_filtered (".\n");
    }
}

/* Set the default place to put a breakpoint
   for the `break' command with no arguments.  */

void
set_default_breakpoint (int valid, struct program_space *pspace,
			CORE_ADDR addr, struct symtab *symtab,
			int line)
{
  default_breakpoint_valid = valid;
  default_breakpoint_pspace = pspace;
  default_breakpoint_address = addr;
  default_breakpoint_symtab = symtab;
  default_breakpoint_line = line;
}

/* Return true iff it is meaningful to use the address member of
   BPT.  For some breakpoint types, the address member is irrelevant
   and it makes no sense to attempt to compare it to other addresses
   (or use it for any other purpose either).

   More specifically, each of the following breakpoint types will
   always have a zero valued address and we don't want to mark
   breakpoints of any of these types to be a duplicate of an actual
   breakpoint at address zero:

      bp_watchpoint
      bp_catchpoint

*/

static int
breakpoint_address_is_meaningful (struct breakpoint *bpt)
{
  enum bptype type = bpt->type;

  return (type != bp_watchpoint && type != bp_catchpoint);
}

/* Assuming LOC1 and LOC2's owners are hardware watchpoints, returns
   true if LOC1 and LOC2 represent the same watchpoint location.  */

static int
watchpoint_locations_match (struct bp_location *loc1, 
			    struct bp_location *loc2)
{
  /* Both of them must not be in moribund_locations.  */
  gdb_assert (loc1->owner != NULL);
  gdb_assert (loc2->owner != NULL);

  /* If the target can evaluate the condition expression in hardware,
     then we we need to insert both watchpoints even if they are at
     the same place.  Otherwise the watchpoint will only trigger when
     the condition of whichever watchpoint was inserted evaluates to
     true, not giving a chance for GDB to check the condition of the
     other watchpoint.  */
  if ((loc1->owner->cond_exp
       && target_can_accel_watchpoint_condition (loc1->address, 
						 loc1->length,
						 loc1->watchpoint_type,
						 loc1->owner->cond_exp))
      || (loc2->owner->cond_exp
	  && target_can_accel_watchpoint_condition (loc2->address, 
						    loc2->length,
						    loc2->watchpoint_type,
						    loc2->owner->cond_exp)))
    return 0;

  /* Note that this checks the owner's type, not the location's.  In
     case the target does not support read watchpoints, but does
     support access watchpoints, we'll have bp_read_watchpoint
     watchpoints with hw_access locations.  Those should be considered
     duplicates of hw_read locations.  The hw_read locations will
     become hw_access locations later.  */
  return (loc1->owner->type == loc2->owner->type
	  && loc1->pspace->aspace == loc2->pspace->aspace
	  && loc1->address == loc2->address
	  && loc1->length == loc2->length);
}

/* Returns true if {ASPACE1,ADDR1} and {ASPACE2,ADDR2} represent the
   same breakpoint location.  In most targets, this can only be true
   if ASPACE1 matches ASPACE2.  On targets that have global
   breakpoints, the address space doesn't really matter.  */

static int
breakpoint_address_match (struct address_space *aspace1, CORE_ADDR addr1,
			  struct address_space *aspace2, CORE_ADDR addr2)
{
  return ((gdbarch_has_global_breakpoints (target_gdbarch)
	   || aspace1 == aspace2)
	  && addr1 == addr2);
}

/* Returns true if {ASPACE2,ADDR2} falls within the range determined by
   {ASPACE1,ADDR1,LEN1}.  In most targets, this can only be true if ASPACE1
   matches ASPACE2.  On targets that have global breakpoints, the address
   space doesn't really matter.  */

static int
breakpoint_address_match_range (struct address_space *aspace1, CORE_ADDR addr1,
				int len1, struct address_space *aspace2,
				CORE_ADDR addr2)
{
  return ((gdbarch_has_global_breakpoints (target_gdbarch)
	   || aspace1 == aspace2)
	  && addr2 >= addr1 && addr2 < addr1 + len1);
}

/* Returns true if {ASPACE,ADDR} matches the breakpoint BL.  BL may be
   a ranged breakpoint.  In most targets, a match happens only if ASPACE
   matches the breakpoint's address space.  On targets that have global
   breakpoints, the address space doesn't really matter.  */

static int
breakpoint_location_address_match (struct bp_location *bl,
				   struct address_space *aspace,
				   CORE_ADDR addr)
{
  return (breakpoint_address_match (bl->pspace->aspace, bl->address,
				    aspace, addr)
	  || (bl->length
	      && breakpoint_address_match_range (bl->pspace->aspace,
						 bl->address, bl->length,
						 aspace, addr)));
}

/* Assuming LOC1 and LOC2's types' have meaningful target addresses
   (breakpoint_address_is_meaningful), returns true if LOC1 and LOC2
   represent the same location.  */

static int
breakpoint_locations_match (struct bp_location *loc1, 
			    struct bp_location *loc2)
{
  int hw_point1, hw_point2;

  /* Both of them must not be in moribund_locations.  */
  gdb_assert (loc1->owner != NULL);
  gdb_assert (loc2->owner != NULL);

  hw_point1 = is_hardware_watchpoint (loc1->owner);
  hw_point2 = is_hardware_watchpoint (loc2->owner);

  if (hw_point1 != hw_point2)
    return 0;
  else if (hw_point1)
    return watchpoint_locations_match (loc1, loc2);
  else
    /* We compare bp_location.length in order to cover ranged breakpoints.  */
    return (breakpoint_address_match (loc1->pspace->aspace, loc1->address,
				     loc2->pspace->aspace, loc2->address)
	    && loc1->length == loc2->length);
}

static void
breakpoint_adjustment_warning (CORE_ADDR from_addr, CORE_ADDR to_addr,
                               int bnum, int have_bnum)
{
  /* The longest string possibly returned by hex_string_custom
     is 50 chars.  These must be at least that big for safety.  */
  char astr1[64];
  char astr2[64];

  strcpy (astr1, hex_string_custom ((unsigned long) from_addr, 8));
  strcpy (astr2, hex_string_custom ((unsigned long) to_addr, 8));
  if (have_bnum)
    warning (_("Breakpoint %d address previously adjusted from %s to %s."),
             bnum, astr1, astr2);
  else
    warning (_("Breakpoint address adjusted from %s to %s."), astr1, astr2);
}

/* Adjust a breakpoint's address to account for architectural
   constraints on breakpoint placement.  Return the adjusted address.
   Note: Very few targets require this kind of adjustment.  For most
   targets, this function is simply the identity function.  */

static CORE_ADDR
adjust_breakpoint_address (struct gdbarch *gdbarch,
			   CORE_ADDR bpaddr, enum bptype bptype)
{
  if (!gdbarch_adjust_breakpoint_address_p (gdbarch))
    {
      /* Very few targets need any kind of breakpoint adjustment.  */
      return bpaddr;
    }
  else if (bptype == bp_watchpoint
           || bptype == bp_hardware_watchpoint
           || bptype == bp_read_watchpoint
           || bptype == bp_access_watchpoint
           || bptype == bp_catchpoint)
    {
      /* Watchpoints and the various bp_catch_* eventpoints should not
         have their addresses modified.  */
      return bpaddr;
    }
  else
    {
      CORE_ADDR adjusted_bpaddr;

      /* Some targets have architectural constraints on the placement
         of breakpoint instructions.  Obtain the adjusted address.  */
      adjusted_bpaddr = gdbarch_adjust_breakpoint_address (gdbarch, bpaddr);

      /* An adjusted breakpoint address can significantly alter
         a user's expectations.  Print a warning if an adjustment
	 is required.  */
      if (adjusted_bpaddr != bpaddr)
	breakpoint_adjustment_warning (bpaddr, adjusted_bpaddr, 0, 0);

      return adjusted_bpaddr;
    }
}

/* Allocate a struct bp_location.  */

static struct bp_location *
allocate_bp_location (struct breakpoint *bpt)
{
  struct bp_location *loc;

  loc = xmalloc (sizeof (struct bp_location));
  memset (loc, 0, sizeof (*loc));

  loc->owner = bpt;
  loc->cond = NULL;
  loc->shlib_disabled = 0;
  loc->enabled = 1;

  switch (bpt->type)
    {
    case bp_breakpoint:
    case bp_until:
    case bp_finish:
    case bp_longjmp:
    case bp_longjmp_resume:
    case bp_exception:
    case bp_exception_resume:
    case bp_step_resume:
    case bp_watchpoint_scope:
    case bp_call_dummy:
    case bp_std_terminate:
    case bp_shlib_event:
    case bp_thread_event:
    case bp_overlay_event:
    case bp_jit_event:
    case bp_longjmp_master:
    case bp_std_terminate_master:
    case bp_exception_master:
    case bp_gnu_ifunc_resolver:
    case bp_gnu_ifunc_resolver_return:
      loc->loc_type = bp_loc_software_breakpoint;
      break;
    case bp_hardware_breakpoint:
      loc->loc_type = bp_loc_hardware_breakpoint;
      break;
    case bp_hardware_watchpoint:
    case bp_read_watchpoint:
    case bp_access_watchpoint:
      loc->loc_type = bp_loc_hardware_watchpoint;
      break;
    case bp_watchpoint:
    case bp_catchpoint:
    case bp_tracepoint:
    case bp_fast_tracepoint:
    case bp_static_tracepoint:
      loc->loc_type = bp_loc_other;
      break;
    default:
      internal_error (__FILE__, __LINE__, _("unknown breakpoint type"));
    }

  loc->refc = 1;
  return loc;
}

static void
free_bp_location (struct bp_location *loc)
{
  if (loc->cond)
    xfree (loc->cond);

  if (loc->function_name)
    xfree (loc->function_name);

  xfree (loc);
}

/* Increment reference count.  */

static void
incref_bp_location (struct bp_location *bl)
{
  ++bl->refc;
}

/* Decrement reference count.  If the reference count reaches 0,
   destroy the bp_location.  Sets *BLP to NULL.  */

static void
decref_bp_location (struct bp_location **blp)
{
  gdb_assert ((*blp)->refc > 0);

  if (--(*blp)->refc == 0)
    free_bp_location (*blp);
  *blp = NULL;
}

/* Helper to set_raw_breakpoint below.  Creates a breakpoint that has
   type BPTYPE and has no locations as yet.  */
/* This function is used in gdbtk sources and thus can not be made
   static.  */

static struct breakpoint *
set_raw_breakpoint_without_location (struct gdbarch *gdbarch,
				     enum bptype bptype)
{
  struct breakpoint *b, *b1;

  b = (struct breakpoint *) xmalloc (sizeof (struct breakpoint));
  memset (b, 0, sizeof (*b));

  b->type = bptype;
  b->gdbarch = gdbarch;
  b->language = current_language->la_language;
  b->input_radix = input_radix;
  b->thread = -1;
  b->enable_state = bp_enabled;
  b->next = 0;
  b->silent = 0;
  b->ignore_count = 0;
  b->commands = NULL;
  b->frame_id = null_frame_id;
  b->forked_inferior_pid = null_ptid;
  b->exec_pathname = NULL;
  b->syscalls_to_be_caught = NULL;
  b->ops = NULL;
  b->condition_not_parsed = 0;
  b->py_bp_object = NULL;
  b->related_breakpoint = b;

  /* Add this breakpoint to the end of the chain so that a list of
     breakpoints will come out in order of increasing numbers.  */

  b1 = breakpoint_chain;
  if (b1 == 0)
    breakpoint_chain = b;
  else
    {
      while (b1->next)
	b1 = b1->next;
      b1->next = b;
    }
  return b;
}

/* Initialize loc->function_name.  EXPLICIT_LOC says no indirect function
   resolutions should be made as the user specified the location explicitly
   enough.  */

static void
set_breakpoint_location_function (struct bp_location *loc, int explicit_loc)
{
  gdb_assert (loc->owner != NULL);

  if (loc->owner->type == bp_breakpoint
      || loc->owner->type == bp_hardware_breakpoint
      || is_tracepoint (loc->owner))
    {
      int is_gnu_ifunc;

      find_pc_partial_function_gnu_ifunc (loc->address, &loc->function_name,
					  NULL, NULL, &is_gnu_ifunc);

      if (is_gnu_ifunc && !explicit_loc)
	{
	  struct breakpoint *b = loc->owner;

	  gdb_assert (loc->pspace == current_program_space);
	  if (gnu_ifunc_resolve_name (loc->function_name,
				      &loc->requested_address))
	    {
	      /* Recalculate ADDRESS based on new REQUESTED_ADDRESS.  */
	      loc->address = adjust_breakpoint_address (loc->gdbarch,
							loc->requested_address,
							b->type);
	    }
	  else if (b->type == bp_breakpoint && b->loc == loc
	           && loc->next == NULL && b->related_breakpoint == b)
	    {
	      /* Create only the whole new breakpoint of this type but do not
		 mess more complicated breakpoints with multiple locations.  */
	      b->type = bp_gnu_ifunc_resolver;
	    }
	}

      if (loc->function_name)
	loc->function_name = xstrdup (loc->function_name);
    }
}

/* Attempt to determine architecture of location identified by SAL.  */
static struct gdbarch *
get_sal_arch (struct symtab_and_line sal)
{
  if (sal.section)
    return get_objfile_arch (sal.section->objfile);
  if (sal.symtab)
    return get_objfile_arch (sal.symtab->objfile);

  return NULL;
}

/* set_raw_breakpoint is a low level routine for allocating and
   partially initializing a breakpoint of type BPTYPE.  The newly
   created breakpoint's address, section, source file name, and line
   number are provided by SAL.  The newly created and partially
   initialized breakpoint is added to the breakpoint chain and
   is also returned as the value of this function.

   It is expected that the caller will complete the initialization of
   the newly created breakpoint struct as well as output any status
   information regarding the creation of a new breakpoint.  In
   particular, set_raw_breakpoint does NOT set the breakpoint
   number!  Care should be taken to not allow an error to occur
   prior to completing the initialization of the breakpoint.  If this
   should happen, a bogus breakpoint will be left on the chain.  */

struct breakpoint *
set_raw_breakpoint (struct gdbarch *gdbarch,
		    struct symtab_and_line sal, enum bptype bptype)
{
  struct breakpoint *b = set_raw_breakpoint_without_location (gdbarch, 
							      bptype);
  CORE_ADDR adjusted_address;
  struct gdbarch *loc_gdbarch;

  loc_gdbarch = get_sal_arch (sal);
  if (!loc_gdbarch)
    loc_gdbarch = b->gdbarch;

  if (bptype != bp_catchpoint)
    gdb_assert (sal.pspace != NULL);

  /* Adjust the breakpoint's address prior to allocating a location.
     Once we call allocate_bp_location(), that mostly uninitialized
     location will be placed on the location chain.  Adjustment of the
     breakpoint may cause target_read_memory() to be called and we do
     not want its scan of the location chain to find a breakpoint and
     location that's only been partially initialized.  */
  adjusted_address = adjust_breakpoint_address (loc_gdbarch, 
						sal.pc, b->type);

  b->loc = allocate_bp_location (b);
  b->loc->gdbarch = loc_gdbarch;
  b->loc->requested_address = sal.pc;
  b->loc->address = adjusted_address;
  b->loc->pspace = sal.pspace;

  /* Store the program space that was used to set the breakpoint, for
     breakpoint resetting.  */
  b->pspace = sal.pspace;

  if (sal.symtab == NULL)
    b->source_file = NULL;
  else
    b->source_file = xstrdup (sal.symtab->filename);
  b->loc->section = sal.section;
  b->line_number = sal.line;

  set_breakpoint_location_function (b->loc,
				    sal.explicit_pc || sal.explicit_line);

  breakpoints_changed ();

  return b;
}


/* Note that the breakpoint object B describes a permanent breakpoint
   instruction, hard-wired into the inferior's code.  */
void
make_breakpoint_permanent (struct breakpoint *b)
{
  struct bp_location *bl;

  b->enable_state = bp_permanent;

  /* By definition, permanent breakpoints are already present in the
     code.  Mark all locations as inserted.  For now,
     make_breakpoint_permanent is called in just one place, so it's
     hard to say if it's reasonable to have permanent breakpoint with
     multiple locations or not, but it's easy to implmement.  */
  for (bl = b->loc; bl; bl = bl->next)
    bl->inserted = 1;
}

/* Call this routine when stepping and nexting to enable a breakpoint
   if we do a longjmp() or 'throw' in TP.  FRAME is the frame which
   initiated the operation.  */

void
set_longjmp_breakpoint (struct thread_info *tp, struct frame_id frame)
{
  struct breakpoint *b, *b_tmp;
  int thread = tp->num;

  /* To avoid having to rescan all objfile symbols at every step,
     we maintain a list of continually-inserted but always disabled
     longjmp "master" breakpoints.  Here, we simply create momentary
     clones of those and enable them for the requested thread.  */
  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->pspace == current_program_space
	&& (b->type == bp_longjmp_master
	    || b->type == bp_exception_master))
      {
	struct breakpoint *clone = clone_momentary_breakpoint (b);

	clone->type = b->type == bp_longjmp_master ? bp_longjmp : bp_exception;
	clone->thread = thread;
      }

  tp->initiating_frame = frame;
}

/* Delete all longjmp breakpoints from THREAD.  */
void
delete_longjmp_breakpoint (int thread)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->type == bp_longjmp || b->type == bp_exception)
      {
	if (b->thread == thread)
	  delete_breakpoint (b);
      }
}

void
enable_overlay_breakpoints (void)
{
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
    if (b->type == bp_overlay_event)
    {
      b->enable_state = bp_enabled;
      update_global_location_list (1);
      overlay_events_enabled = 1;
    }
}

void
disable_overlay_breakpoints (void)
{
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
    if (b->type == bp_overlay_event)
    {
      b->enable_state = bp_disabled;
      update_global_location_list (0);
      overlay_events_enabled = 0;
    }
}

/* Set an active std::terminate breakpoint for each std::terminate
   master breakpoint.  */
void
set_std_terminate_breakpoint (void)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->pspace == current_program_space
	&& b->type == bp_std_terminate_master)
      {
	struct breakpoint *clone = clone_momentary_breakpoint (b);
	clone->type = bp_std_terminate;
      }
}

/* Delete all the std::terminate breakpoints.  */
void
delete_std_terminate_breakpoint (void)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->type == bp_std_terminate)
      delete_breakpoint (b);
}

struct breakpoint *
create_thread_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  struct breakpoint *b;

  b = create_internal_breakpoint (gdbarch, address, bp_thread_event);
  
  b->enable_state = bp_enabled;
  /* addr_string has to be used or breakpoint_re_set will delete me.  */
  b->addr_string
    = xstrprintf ("*%s", paddress (b->loc->gdbarch, b->loc->address));

  update_global_location_list_nothrow (1);

  return b;
}

void
remove_thread_event_breakpoints (void)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->type == bp_thread_event
	&& b->loc->pspace == current_program_space)
      delete_breakpoint (b);
}

struct captured_parse_breakpoint_args
  {
    char **arg_p;
    struct symtabs_and_lines *sals_p;
    struct linespec_result *canonical_p;
  };

struct lang_and_radix
  {
    enum language lang;
    int radix;
  };

/* Create a breakpoint for JIT code registration and unregistration.  */

struct breakpoint *
create_jit_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  struct breakpoint *b;

  b = create_internal_breakpoint (gdbarch, address, bp_jit_event);
  update_global_location_list_nothrow (1);
  return b;
}

/* Remove JIT code registration and unregistration breakpoint(s).  */

void
remove_jit_event_breakpoints (void)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->type == bp_jit_event
	&& b->loc->pspace == current_program_space)
      delete_breakpoint (b);
}

void
remove_solib_event_breakpoints (void)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->type == bp_shlib_event
	&& b->loc->pspace == current_program_space)
      delete_breakpoint (b);
}

struct breakpoint *
create_solib_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  struct breakpoint *b;

  b = create_internal_breakpoint (gdbarch, address, bp_shlib_event);
  update_global_location_list_nothrow (1);
  return b;
}

/* Disable any breakpoints that are on code in shared libraries.  Only
   apply to enabled breakpoints, disabled ones can just stay disabled.  */

void
disable_breakpoints_in_shlibs (void)
{
  struct bp_location *loc, **locp_tmp;

  ALL_BP_LOCATIONS (loc, locp_tmp)
  {
    /* ALL_BP_LOCATIONS bp_location has LOC->OWNER always non-NULL.  */
    struct breakpoint *b = loc->owner;

    /* We apply the check to all breakpoints, including disabled for
       those with loc->duplicate set.  This is so that when breakpoint
       becomes enabled, or the duplicate is removed, gdb will try to
       insert all breakpoints.  If we don't set shlib_disabled here,
       we'll try to insert those breakpoints and fail.  */
    if (((b->type == bp_breakpoint)
	 || (b->type == bp_jit_event)
	 || (b->type == bp_hardware_breakpoint)
	 || (is_tracepoint (b)))
	&& loc->pspace == current_program_space
	&& !loc->shlib_disabled
#ifdef PC_SOLIB
	&& PC_SOLIB (loc->address)
#else
	&& solib_name_from_address (loc->pspace, loc->address)
#endif
	)
      {
	loc->shlib_disabled = 1;
      }
  }
}

/* Disable any breakpoints that are in in an unloaded shared library.
   Only apply to enabled breakpoints, disabled ones can just stay
   disabled.  */

static void
disable_breakpoints_in_unloaded_shlib (struct so_list *solib)
{
  struct bp_location *loc, **locp_tmp;
  int disabled_shlib_breaks = 0;

  /* SunOS a.out shared libraries are always mapped, so do not
     disable breakpoints; they will only be reported as unloaded
     through clear_solib when GDB discards its shared library
     list.  See clear_solib for more information.  */
  if (exec_bfd != NULL
      && bfd_get_flavour (exec_bfd) == bfd_target_aout_flavour)
    return;

  ALL_BP_LOCATIONS (loc, locp_tmp)
  {
    /* ALL_BP_LOCATIONS bp_location has LOC->OWNER always non-NULL.  */
    struct breakpoint *b = loc->owner;

    if ((loc->loc_type == bp_loc_hardware_breakpoint
	 || loc->loc_type == bp_loc_software_breakpoint)
	&& solib->pspace == loc->pspace
	&& !loc->shlib_disabled
	&& (b->type == bp_breakpoint
	    || b->type == bp_jit_event
	    || b->type == bp_hardware_breakpoint)
	&& solib_contains_address_p (solib, loc->address))
      {
	loc->shlib_disabled = 1;
	/* At this point, we cannot rely on remove_breakpoint
	   succeeding so we must mark the breakpoint as not inserted
	   to prevent future errors occurring in remove_breakpoints.  */
	loc->inserted = 0;
	if (!disabled_shlib_breaks)
	  {
	    target_terminal_ours_for_output ();
	    warning (_("Temporarily disabling breakpoints "
		       "for unloaded shared library \"%s\""),
		     solib->so_name);
	  }
	disabled_shlib_breaks = 1;
      }
  }
}

/* FORK & VFORK catchpoints.  */

/* Implement the "insert" breakpoint_ops method for fork
   catchpoints.  */

static int
insert_catch_fork (struct bp_location *bl)
{
  return target_insert_fork_catchpoint (PIDGET (inferior_ptid));
}

/* Implement the "remove" breakpoint_ops method for fork
   catchpoints.  */

static int
remove_catch_fork (struct bp_location *bl)
{
  return target_remove_fork_catchpoint (PIDGET (inferior_ptid));
}

/* Implement the "breakpoint_hit" breakpoint_ops method for fork
   catchpoints.  */

static int
breakpoint_hit_catch_fork (const struct bp_location *bl,
			   struct address_space *aspace, CORE_ADDR bp_addr)
{
  return inferior_has_forked (inferior_ptid, &bl->owner->forked_inferior_pid);
}

/* Implement the "print_it" breakpoint_ops method for fork
   catchpoints.  */

static enum print_stop_action
print_it_catch_fork (struct breakpoint *b)
{
  annotate_catchpoint (b->number);
  printf_filtered (_("\nCatchpoint %d (forked process %d), "),
		   b->number, ptid_get_pid (b->forked_inferior_pid));
  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" breakpoint_ops method for fork
   catchpoints.  */

static void
print_one_catch_fork (struct breakpoint *b, struct bp_location **last_loc)
{
  struct value_print_options opts;

  get_user_print_options (&opts);

  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);
  ui_out_text (uiout, "fork");
  if (!ptid_equal (b->forked_inferior_pid, null_ptid))
    {
      ui_out_text (uiout, ", process ");
      ui_out_field_int (uiout, "what",
                        ptid_get_pid (b->forked_inferior_pid));
      ui_out_spaces (uiout, 1);
    }
}

/* Implement the "print_mention" breakpoint_ops method for fork
   catchpoints.  */

static void
print_mention_catch_fork (struct breakpoint *b)
{
  printf_filtered (_("Catchpoint %d (fork)"), b->number);
}

/* Implement the "print_recreate" breakpoint_ops method for fork
   catchpoints.  */

static void
print_recreate_catch_fork (struct breakpoint *b, struct ui_file *fp)
{
  fprintf_unfiltered (fp, "catch fork");
}

/* The breakpoint_ops structure to be used in fork catchpoints.  */

static struct breakpoint_ops catch_fork_breakpoint_ops =
{
  insert_catch_fork,
  remove_catch_fork,
  breakpoint_hit_catch_fork,
  NULL, /* resources_needed */
  print_it_catch_fork,
  print_one_catch_fork,
  NULL, /* print_one_detail */
  print_mention_catch_fork,
  print_recreate_catch_fork
};

/* Implement the "insert" breakpoint_ops method for vfork
   catchpoints.  */

static int
insert_catch_vfork (struct bp_location *bl)
{
  return target_insert_vfork_catchpoint (PIDGET (inferior_ptid));
}

/* Implement the "remove" breakpoint_ops method for vfork
   catchpoints.  */

static int
remove_catch_vfork (struct bp_location *bl)
{
  return target_remove_vfork_catchpoint (PIDGET (inferior_ptid));
}

/* Implement the "breakpoint_hit" breakpoint_ops method for vfork
   catchpoints.  */

static int
breakpoint_hit_catch_vfork (const struct bp_location *bl,
			    struct address_space *aspace, CORE_ADDR bp_addr)
{
  return inferior_has_vforked (inferior_ptid, &bl->owner->forked_inferior_pid);
}

/* Implement the "print_it" breakpoint_ops method for vfork
   catchpoints.  */

static enum print_stop_action
print_it_catch_vfork (struct breakpoint *b)
{
  annotate_catchpoint (b->number);
  printf_filtered (_("\nCatchpoint %d (vforked process %d), "),
		   b->number, ptid_get_pid (b->forked_inferior_pid));
  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" breakpoint_ops method for vfork
   catchpoints.  */

static void
print_one_catch_vfork (struct breakpoint *b, struct bp_location **last_loc)
{
  struct value_print_options opts;

  get_user_print_options (&opts);
  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);
  ui_out_text (uiout, "vfork");
  if (!ptid_equal (b->forked_inferior_pid, null_ptid))
    {
      ui_out_text (uiout, ", process ");
      ui_out_field_int (uiout, "what",
                        ptid_get_pid (b->forked_inferior_pid));
      ui_out_spaces (uiout, 1);
    }
}

/* Implement the "print_mention" breakpoint_ops method for vfork
   catchpoints.  */

static void
print_mention_catch_vfork (struct breakpoint *b)
{
  printf_filtered (_("Catchpoint %d (vfork)"), b->number);
}

/* Implement the "print_recreate" breakpoint_ops method for vfork
   catchpoints.  */

static void
print_recreate_catch_vfork (struct breakpoint *b, struct ui_file *fp)
{
  fprintf_unfiltered (fp, "catch vfork");
}

/* The breakpoint_ops structure to be used in vfork catchpoints.  */

static struct breakpoint_ops catch_vfork_breakpoint_ops =
{
  insert_catch_vfork,
  remove_catch_vfork,
  breakpoint_hit_catch_vfork,
  NULL, /* resources_needed */
  print_it_catch_vfork,
  print_one_catch_vfork,
  NULL, /* print_one_detail */
  print_mention_catch_vfork,
  print_recreate_catch_vfork
};

/* Implement the "insert" breakpoint_ops method for syscall
   catchpoints.  */

static int
insert_catch_syscall (struct bp_location *bl)
{
  struct inferior *inf = current_inferior ();

  ++inf->total_syscalls_count;
  if (!bl->owner->syscalls_to_be_caught)
    ++inf->any_syscall_count;
  else
    {
      int i, iter;

      for (i = 0;
           VEC_iterate (int, bl->owner->syscalls_to_be_caught, i, iter);
           i++)
	{
          int elem;

	  if (iter >= VEC_length (int, inf->syscalls_counts))
	    {
              int old_size = VEC_length (int, inf->syscalls_counts);
              uintptr_t vec_addr_offset
		= old_size * ((uintptr_t) sizeof (int));
              uintptr_t vec_addr;
              VEC_safe_grow (int, inf->syscalls_counts, iter + 1);
              vec_addr = (uintptr_t) VEC_address (int, inf->syscalls_counts) +
		vec_addr_offset;
              memset ((void *) vec_addr, 0,
                      (iter + 1 - old_size) * sizeof (int));
	    }
          elem = VEC_index (int, inf->syscalls_counts, iter);
          VEC_replace (int, inf->syscalls_counts, iter, ++elem);
	}
    }

  return target_set_syscall_catchpoint (PIDGET (inferior_ptid),
					inf->total_syscalls_count != 0,
					inf->any_syscall_count,
					VEC_length (int, inf->syscalls_counts),
					VEC_address (int, inf->syscalls_counts));
}

/* Implement the "remove" breakpoint_ops method for syscall
   catchpoints.  */

static int
remove_catch_syscall (struct bp_location *bl)
{
  struct inferior *inf = current_inferior ();

  --inf->total_syscalls_count;
  if (!bl->owner->syscalls_to_be_caught)
    --inf->any_syscall_count;
  else
    {
      int i, iter;

      for (i = 0;
           VEC_iterate (int, bl->owner->syscalls_to_be_caught, i, iter);
           i++)
	{
          int elem;
	  if (iter >= VEC_length (int, inf->syscalls_counts))
	    /* Shouldn't happen.  */
	    continue;
          elem = VEC_index (int, inf->syscalls_counts, iter);
          VEC_replace (int, inf->syscalls_counts, iter, --elem);
        }
    }

  return target_set_syscall_catchpoint (PIDGET (inferior_ptid),
					inf->total_syscalls_count != 0,
					inf->any_syscall_count,
					VEC_length (int, inf->syscalls_counts),
					VEC_address (int,
						     inf->syscalls_counts));
}

/* Implement the "breakpoint_hit" breakpoint_ops method for syscall
   catchpoints.  */

static int
breakpoint_hit_catch_syscall (const struct bp_location *bl,
			      struct address_space *aspace, CORE_ADDR bp_addr)
{
  /* We must check if we are catching specific syscalls in this
     breakpoint.  If we are, then we must guarantee that the called
     syscall is the same syscall we are catching.  */
  int syscall_number = 0;
  const struct breakpoint *b = bl->owner;

  if (!inferior_has_called_syscall (inferior_ptid, &syscall_number))
    return 0;

  /* Now, checking if the syscall is the same.  */
  if (b->syscalls_to_be_caught)
    {
      int i, iter;

      for (i = 0;
           VEC_iterate (int, b->syscalls_to_be_caught, i, iter);
           i++)
	if (syscall_number == iter)
	  break;
      /* Not the same.  */
      if (!iter)
	return 0;
    }

  return 1;
}

/* Implement the "print_it" breakpoint_ops method for syscall
   catchpoints.  */

static enum print_stop_action
print_it_catch_syscall (struct breakpoint *b)
{
  /* These are needed because we want to know in which state a
     syscall is.  It can be in the TARGET_WAITKIND_SYSCALL_ENTRY
     or TARGET_WAITKIND_SYSCALL_RETURN, and depending on it we
     must print "called syscall" or "returned from syscall".  */
  ptid_t ptid;
  struct target_waitstatus last;
  struct syscall s;
  struct cleanup *old_chain;
  char *syscall_id;

  get_last_target_status (&ptid, &last);

  get_syscall_by_number (last.value.syscall_number, &s);

  annotate_catchpoint (b->number);

  if (s.name == NULL)
    syscall_id = xstrprintf ("%d", last.value.syscall_number);
  else
    syscall_id = xstrprintf ("'%s'", s.name);

  old_chain = make_cleanup (xfree, syscall_id);

  if (last.kind == TARGET_WAITKIND_SYSCALL_ENTRY)
    printf_filtered (_("\nCatchpoint %d (call to syscall %s), "),
                     b->number, syscall_id);
  else if (last.kind == TARGET_WAITKIND_SYSCALL_RETURN)
    printf_filtered (_("\nCatchpoint %d (returned from syscall %s), "),
                     b->number, syscall_id);

  do_cleanups (old_chain);

  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" breakpoint_ops method for syscall
   catchpoints.  */

static void
print_one_catch_syscall (struct breakpoint *b,
			 struct bp_location **last_loc)
{
  struct value_print_options opts;

  get_user_print_options (&opts);
  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);

  if (b->syscalls_to_be_caught
      && VEC_length (int, b->syscalls_to_be_caught) > 1)
    ui_out_text (uiout, "syscalls \"");
  else
    ui_out_text (uiout, "syscall \"");

  if (b->syscalls_to_be_caught)
    {
      int i, iter;
      char *text = xstrprintf ("%s", "");

      for (i = 0;
           VEC_iterate (int, b->syscalls_to_be_caught, i, iter);
           i++)
        {
          char *x = text;
          struct syscall s;
          get_syscall_by_number (iter, &s);

          if (s.name != NULL)
            text = xstrprintf ("%s%s, ", text, s.name);
          else
            text = xstrprintf ("%s%d, ", text, iter);

          /* We have to xfree the last 'text' (now stored at 'x')
             because xstrprintf dinamically allocates new space for it
             on every call.  */
	  xfree (x);
        }
      /* Remove the last comma.  */
      text[strlen (text) - 2] = '\0';
      ui_out_field_string (uiout, "what", text);
    }
  else
    ui_out_field_string (uiout, "what", "<any syscall>");
  ui_out_text (uiout, "\" ");
}

/* Implement the "print_mention" breakpoint_ops method for syscall
   catchpoints.  */

static void
print_mention_catch_syscall (struct breakpoint *b)
{
  if (b->syscalls_to_be_caught)
    {
      int i, iter;

      if (VEC_length (int, b->syscalls_to_be_caught) > 1)
        printf_filtered (_("Catchpoint %d (syscalls"), b->number);
      else
        printf_filtered (_("Catchpoint %d (syscall"), b->number);

      for (i = 0;
           VEC_iterate (int, b->syscalls_to_be_caught, i, iter);
           i++)
        {
          struct syscall s;
          get_syscall_by_number (iter, &s);

          if (s.name)
            printf_filtered (" '%s' [%d]", s.name, s.number);
          else
            printf_filtered (" %d", s.number);
        }
      printf_filtered (")");
    }
  else
    printf_filtered (_("Catchpoint %d (any syscall)"),
                     b->number);
}

/* Implement the "print_recreate" breakpoint_ops method for syscall
   catchpoints.  */

static void
print_recreate_catch_syscall (struct breakpoint *b, struct ui_file *fp)
{
  fprintf_unfiltered (fp, "catch syscall");

  if (b->syscalls_to_be_caught)
    {
      int i, iter;

      for (i = 0;
           VEC_iterate (int, b->syscalls_to_be_caught, i, iter);
           i++)
        {
          struct syscall s;

          get_syscall_by_number (iter, &s);
          if (s.name)
            fprintf_unfiltered (fp, " %s", s.name);
          else
            fprintf_unfiltered (fp, " %d", s.number);
        }
    }
}

/* The breakpoint_ops structure to be used in syscall catchpoints.  */

static struct breakpoint_ops catch_syscall_breakpoint_ops =
{
  insert_catch_syscall,
  remove_catch_syscall,
  breakpoint_hit_catch_syscall,
  NULL, /* resources_needed */
  print_it_catch_syscall,
  print_one_catch_syscall,
  NULL, /* print_one_detail */
  print_mention_catch_syscall,
  print_recreate_catch_syscall
};

/* Returns non-zero if 'b' is a syscall catchpoint.  */

static int
syscall_catchpoint_p (struct breakpoint *b)
{
  return (b->ops == &catch_syscall_breakpoint_ops);
}

/* Create a new breakpoint of the bp_catchpoint kind and return it,
   but does NOT mention it nor update the global location list.
   This is useful if you need to fill more fields in the
   struct breakpoint before calling mention.
 
   If TEMPFLAG is non-zero, then make the breakpoint temporary.
   If COND_STRING is not NULL, then store it in the breakpoint.
   OPS, if not NULL, is the breakpoint_ops structure associated
   to the catchpoint.  */

static struct breakpoint *
create_catchpoint_without_mention (struct gdbarch *gdbarch, int tempflag,
				   char *cond_string,
				   struct breakpoint_ops *ops)
{
  struct symtab_and_line sal;
  struct breakpoint *b;

  init_sal (&sal);
  sal.pspace = current_program_space;

  b = set_raw_breakpoint (gdbarch, sal, bp_catchpoint);
  set_breakpoint_count (breakpoint_count + 1);
  b->number = breakpoint_count;

  b->cond_string = (cond_string == NULL) ? NULL : xstrdup (cond_string);
  b->thread = -1;
  b->addr_string = NULL;
  b->enable_state = bp_enabled;
  b->disposition = tempflag ? disp_del : disp_donttouch;
  b->ops = ops;

  return b;
}

/* Create a new breakpoint of the bp_catchpoint kind and return it.
 
   If TEMPFLAG is non-zero, then make the breakpoint temporary.
   If COND_STRING is not NULL, then store it in the breakpoint.
   OPS, if not NULL, is the breakpoint_ops structure associated
   to the catchpoint.  */

static struct breakpoint *
create_catchpoint (struct gdbarch *gdbarch, int tempflag,
		   char *cond_string, struct breakpoint_ops *ops)
{
  struct breakpoint *b =
    create_catchpoint_without_mention (gdbarch, tempflag, cond_string, ops);

  mention (b);
  update_global_location_list (1);

  return b;
}

static void
create_fork_vfork_event_catchpoint (struct gdbarch *gdbarch,
				    int tempflag, char *cond_string,
                                    struct breakpoint_ops *ops)
{
  struct breakpoint *b
    = create_catchpoint (gdbarch, tempflag, cond_string, ops);

  /* FIXME: We should put this information in a breakpoint private data
     area.  */
  b->forked_inferior_pid = null_ptid;
}

/* Exec catchpoints.  */

static int
insert_catch_exec (struct bp_location *bl)
{
  return target_insert_exec_catchpoint (PIDGET (inferior_ptid));
}

static int
remove_catch_exec (struct bp_location *bl)
{
  return target_remove_exec_catchpoint (PIDGET (inferior_ptid));
}

static int
breakpoint_hit_catch_exec (const struct bp_location *bl,
			   struct address_space *aspace, CORE_ADDR bp_addr)
{
  return inferior_has_execd (inferior_ptid, &bl->owner->exec_pathname);
}

static enum print_stop_action
print_it_catch_exec (struct breakpoint *b)
{
  annotate_catchpoint (b->number);
  printf_filtered (_("\nCatchpoint %d (exec'd %s), "), b->number,
		   b->exec_pathname);
  return PRINT_SRC_AND_LOC;
}

static void
print_one_catch_exec (struct breakpoint *b, struct bp_location **last_loc)
{
  struct value_print_options opts;

  get_user_print_options (&opts);

  /* Field 4, the address, is omitted (which makes the columns
     not line up too nicely with the headers, but the effect
     is relatively readable).  */
  if (opts.addressprint)
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);
  ui_out_text (uiout, "exec");
  if (b->exec_pathname != NULL)
    {
      ui_out_text (uiout, ", program \"");
      ui_out_field_string (uiout, "what", b->exec_pathname);
      ui_out_text (uiout, "\" ");
    }
}

static void
print_mention_catch_exec (struct breakpoint *b)
{
  printf_filtered (_("Catchpoint %d (exec)"), b->number);
}

/* Implement the "print_recreate" breakpoint_ops method for exec
   catchpoints.  */

static void
print_recreate_catch_exec (struct breakpoint *b, struct ui_file *fp)
{
  fprintf_unfiltered (fp, "catch exec");
}

static struct breakpoint_ops catch_exec_breakpoint_ops =
{
  insert_catch_exec,
  remove_catch_exec,
  breakpoint_hit_catch_exec,
  NULL, /* resources_needed */
  print_it_catch_exec,
  print_one_catch_exec,
  NULL, /* print_one_detail */
  print_mention_catch_exec,
  print_recreate_catch_exec
};

static void
create_syscall_event_catchpoint (int tempflag, VEC(int) *filter,
                                 struct breakpoint_ops *ops)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct breakpoint *b =
    create_catchpoint_without_mention (gdbarch, tempflag, NULL, ops);

  b->syscalls_to_be_caught = filter;

  /* Now, we have to mention the breakpoint and update the global
     location list.  */
  mention (b);
  update_global_location_list (1);
}

static int
hw_breakpoint_used_count (void)
{
  int i = 0;
  struct breakpoint *b;
  struct bp_location *bl;

  ALL_BREAKPOINTS (b)
  {
    if (b->type == bp_hardware_breakpoint && breakpoint_enabled (b))
      for (bl = b->loc; bl; bl = bl->next)
	{
	  /* Special types of hardware breakpoints may use more than
	     one register.  */
	  if (b->ops && b->ops->resources_needed)
	    i += b->ops->resources_needed (bl);
	  else
	    i++;
	}
  }

  return i;
}

static int
hw_watchpoint_used_count (enum bptype type, int *other_type_used)
{
  int i = 0;
  struct breakpoint *b;
  struct bp_location *bl;

  *other_type_used = 0;
  ALL_BREAKPOINTS (b)
    {
      if (!breakpoint_enabled (b))
	continue;

	if (b->type == type)
	  for (bl = b->loc; bl; bl = bl->next)
	    {
	      /* Special types of hardware watchpoints may use more than
		 one register.  */
	      if (b->ops && b->ops->resources_needed)
		i += b->ops->resources_needed (bl);
	      else
		i++;
	    }
	else if (is_hardware_watchpoint (b))
	  *other_type_used = 1;
    }

  return i;
}

void
disable_watchpoints_before_interactive_call_start (void)
{
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
  {
    if (is_watchpoint (b) && breakpoint_enabled (b))
      {
	b->enable_state = bp_call_disabled;
	update_global_location_list (0);
      }
  }
}

void
enable_watchpoints_after_interactive_call_stop (void)
{
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
  {
    if (is_watchpoint (b) && b->enable_state == bp_call_disabled)
      {
	b->enable_state = bp_enabled;
	update_global_location_list (1);
      }
  }
}

void
disable_breakpoints_before_startup (void)
{
  struct breakpoint *b;
  int found = 0;

  ALL_BREAKPOINTS (b)
    {
      if (b->pspace != current_program_space)
	continue;

      if ((b->type == bp_breakpoint
	   || b->type == bp_hardware_breakpoint)
	  && breakpoint_enabled (b))
	{
	  b->enable_state = bp_startup_disabled;
	  found = 1;
	}
    }

  if (found)
    update_global_location_list (0);

  current_program_space->executing_startup = 1;
}

void
enable_breakpoints_after_startup (void)
{
  struct breakpoint *b;
  int found = 0;

  current_program_space->executing_startup = 0;

  ALL_BREAKPOINTS (b)
    {
      if (b->pspace != current_program_space)
	continue;

      if ((b->type == bp_breakpoint
	   || b->type == bp_hardware_breakpoint)
	  && b->enable_state == bp_startup_disabled)
	{
	  b->enable_state = bp_enabled;
	  found = 1;
	}
    }

  if (found)
    breakpoint_re_set ();
}


/* Set a breakpoint that will evaporate an end of command
   at address specified by SAL.
   Restrict it to frame FRAME if FRAME is nonzero.  */

struct breakpoint *
set_momentary_breakpoint (struct gdbarch *gdbarch, struct symtab_and_line sal,
			  struct frame_id frame_id, enum bptype type)
{
  struct breakpoint *b;

  /* If FRAME_ID is valid, it should be a real frame, not an inlined
     one.  */
  gdb_assert (!frame_id_inlined_p (frame_id));

  b = set_raw_breakpoint (gdbarch, sal, type);
  b->enable_state = bp_enabled;
  b->disposition = disp_donttouch;
  b->frame_id = frame_id;

  /* If we're debugging a multi-threaded program, then we want
     momentary breakpoints to be active in only a single thread of
     control.  */
  if (in_thread_list (inferior_ptid))
    b->thread = pid_to_thread_id (inferior_ptid);

  update_global_location_list_nothrow (1);

  return b;
}

/* Make a deep copy of momentary breakpoint ORIG.  Returns NULL if
   ORIG is NULL.  */

struct breakpoint *
clone_momentary_breakpoint (struct breakpoint *orig)
{
  struct breakpoint *copy;

  /* If there's nothing to clone, then return nothing.  */
  if (orig == NULL)
    return NULL;

  copy = set_raw_breakpoint_without_location (orig->gdbarch, orig->type);
  copy->loc = allocate_bp_location (copy);
  set_breakpoint_location_function (copy->loc, 1);

  copy->loc->gdbarch = orig->loc->gdbarch;
  copy->loc->requested_address = orig->loc->requested_address;
  copy->loc->address = orig->loc->address;
  copy->loc->section = orig->loc->section;
  copy->loc->pspace = orig->loc->pspace;

  if (orig->source_file == NULL)
    copy->source_file = NULL;
  else
    copy->source_file = xstrdup (orig->source_file);

  copy->line_number = orig->line_number;
  copy->frame_id = orig->frame_id;
  copy->thread = orig->thread;
  copy->pspace = orig->pspace;

  copy->enable_state = bp_enabled;
  copy->disposition = disp_donttouch;
  copy->number = internal_breakpoint_number--;

  update_global_location_list_nothrow (0);
  return copy;
}

struct breakpoint *
set_momentary_breakpoint_at_pc (struct gdbarch *gdbarch, CORE_ADDR pc,
				enum bptype type)
{
  struct symtab_and_line sal;

  sal = find_pc_line (pc, 0);
  sal.pc = pc;
  sal.section = find_pc_overlay (pc);
  sal.explicit_pc = 1;

  return set_momentary_breakpoint (gdbarch, sal, null_frame_id, type);
}


/* Tell the user we have just set a breakpoint B.  */

static void
mention (struct breakpoint *b)
{
  int say_where = 0;
  struct cleanup *ui_out_chain;
  struct value_print_options opts;

  get_user_print_options (&opts);

  /* FIXME: This is misplaced; mention() is called by things (like
     hitting a watchpoint) other than breakpoint creation.  It should
     be possible to clean this up and at the same time replace the
     random calls to breakpoint_changed with this hook.  */
  observer_notify_breakpoint_created (b->number);

  if (b->ops != NULL && b->ops->print_mention != NULL)
    b->ops->print_mention (b);
  else
    switch (b->type)
      {
      case bp_none:
	printf_filtered (_("(apparently deleted?) Eventpoint %d: "),
			 b->number);
	break;
      case bp_watchpoint:
	ui_out_text (uiout, "Watchpoint ");
	ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "wpt");
	ui_out_field_int (uiout, "number", b->number);
	ui_out_text (uiout, ": ");
	ui_out_field_string (uiout, "exp", b->exp_string);
	do_cleanups (ui_out_chain);
	break;
      case bp_hardware_watchpoint:
	ui_out_text (uiout, "Hardware watchpoint ");
	ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "wpt");
	ui_out_field_int (uiout, "number", b->number);
	ui_out_text (uiout, ": ");
	ui_out_field_string (uiout, "exp", b->exp_string);
	do_cleanups (ui_out_chain);
	break;
      case bp_read_watchpoint:
	ui_out_text (uiout, "Hardware read watchpoint ");
	ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "hw-rwpt");
	ui_out_field_int (uiout, "number", b->number);
	ui_out_text (uiout, ": ");
	ui_out_field_string (uiout, "exp", b->exp_string);
	do_cleanups (ui_out_chain);
	break;
      case bp_access_watchpoint:
	ui_out_text (uiout, "Hardware access (read/write) watchpoint ");
	ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "hw-awpt");
	ui_out_field_int (uiout, "number", b->number);
	ui_out_text (uiout, ": ");
	ui_out_field_string (uiout, "exp", b->exp_string);
	do_cleanups (ui_out_chain);
	break;
      case bp_breakpoint:
      case bp_gnu_ifunc_resolver:
	if (ui_out_is_mi_like_p (uiout))
	  {
	    say_where = 0;
	    break;
	  }
	if (b->disposition == disp_del)
	  printf_filtered (_("Temporary breakpoint"));
	else
	  printf_filtered (_("Breakpoint"));
	printf_filtered (_(" %d"), b->number);
	if (b->type == bp_gnu_ifunc_resolver)
	  printf_filtered (_(" at gnu-indirect-function resolver"));
	say_where = 1;
	break;
      case bp_hardware_breakpoint:
	if (ui_out_is_mi_like_p (uiout))
	  {
	    say_where = 0;
	    break;
	  }
	printf_filtered (_("Hardware assisted breakpoint %d"), b->number);
	say_where = 1;
	break;
      case bp_tracepoint:
	if (ui_out_is_mi_like_p (uiout))
	  {
	    say_where = 0;
	    break;
	  }
	printf_filtered (_("Tracepoint"));
	printf_filtered (_(" %d"), b->number);
	say_where = 1;
	break;
      case bp_fast_tracepoint:
	if (ui_out_is_mi_like_p (uiout))
	  {
	    say_where = 0;
	    break;
	  }
	printf_filtered (_("Fast tracepoint"));
	printf_filtered (_(" %d"), b->number);
	say_where = 1;
	break;
      case bp_static_tracepoint:
	if (ui_out_is_mi_like_p (uiout))
	  {
	    say_where = 0;
	    break;
	  }
	printf_filtered (_("Static tracepoint"));
	printf_filtered (_(" %d"), b->number);
	say_where = 1;
	break;

      case bp_until:
      case bp_finish:
      case bp_longjmp:
      case bp_longjmp_resume:
      case bp_exception:
      case bp_exception_resume:
      case bp_step_resume:
      case bp_call_dummy:
      case bp_std_terminate:
      case bp_watchpoint_scope:
      case bp_shlib_event:
      case bp_thread_event:
      case bp_overlay_event:
      case bp_jit_event:
      case bp_longjmp_master:
      case bp_std_terminate_master:
      case bp_exception_master:
      case bp_gnu_ifunc_resolver_return:
	break;
      }

  if (say_where)
    {
      /* i18n: cagney/2005-02-11: Below needs to be merged into a
	 single string.  */
      if (b->loc == NULL)
	{
	  printf_filtered (_(" (%s) pending."), b->addr_string);
	}
      else
	{
	  if (opts.addressprint || b->source_file == NULL)
	    {
	      printf_filtered (" at ");
	      fputs_filtered (paddress (b->loc->gdbarch, b->loc->address),
			      gdb_stdout);
	    }
	  if (b->source_file)
	    printf_filtered (": file %s, line %d.",
			     b->source_file, b->line_number);
	  
	  if (b->loc->next)
	    {
	      struct bp_location *loc = b->loc;
	      int n = 0;
	      for (; loc; loc = loc->next)
		++n;
	      printf_filtered (" (%d locations)", n);		
	    }

	}
    }
  if (ui_out_is_mi_like_p (uiout))
    return;
  printf_filtered ("\n");
}


static struct bp_location *
add_location_to_breakpoint (struct breakpoint *b,
			    const struct symtab_and_line *sal)
{
  struct bp_location *loc, **tmp;

  loc = allocate_bp_location (b);
  for (tmp = &(b->loc); *tmp != NULL; tmp = &((*tmp)->next))
    ;
  *tmp = loc;
  loc->gdbarch = get_sal_arch (*sal);
  if (!loc->gdbarch)
    loc->gdbarch = b->gdbarch;
  loc->requested_address = sal->pc;
  loc->address = adjust_breakpoint_address (loc->gdbarch,
					    loc->requested_address, b->type);
  loc->pspace = sal->pspace;
  gdb_assert (loc->pspace != NULL);
  loc->section = sal->section;

  set_breakpoint_location_function (loc,
				    sal->explicit_pc || sal->explicit_line);
  return loc;
}


/* Return 1 if LOC is pointing to a permanent breakpoint, 
   return 0 otherwise.  */

static int
bp_loc_is_permanent (struct bp_location *loc)
{
  int len;
  CORE_ADDR addr;
  const gdb_byte *brk;
  gdb_byte *target_mem;
  struct cleanup *cleanup;
  int retval = 0;

  gdb_assert (loc != NULL);

  addr = loc->address;
  brk = gdbarch_breakpoint_from_pc (loc->gdbarch, &addr, &len);

  /* Software breakpoints unsupported?  */
  if (brk == NULL)
    return 0;

  target_mem = alloca (len);

  /* Enable the automatic memory restoration from breakpoints while
     we read the memory.  Otherwise we could say about our temporary
     breakpoints they are permanent.  */
  cleanup = save_current_space_and_thread ();

  switch_to_program_space_and_thread (loc->pspace);
  make_show_memory_breakpoints_cleanup (0);

  if (target_read_memory (loc->address, target_mem, len) == 0
      && memcmp (target_mem, brk, len) == 0)
    retval = 1;

  do_cleanups (cleanup);

  return retval;
}



/* Create a breakpoint with SAL as location.  Use ADDR_STRING
   as textual description of the location, and COND_STRING
   as condition expression.  */

static void
create_breakpoint_sal (struct gdbarch *gdbarch,
		       struct symtabs_and_lines sals, char *addr_string,
		       char *cond_string,
		       enum bptype type, enum bpdisp disposition,
		       int thread, int task, int ignore_count,
		       struct breakpoint_ops *ops, int from_tty,
		       int enabled, int internal, int display_canonical)
{
  struct breakpoint *b = NULL;
  int i;

  if (type == bp_hardware_breakpoint)
    {
      int i = hw_breakpoint_used_count ();
      int target_resources_ok = 
	target_can_use_hardware_watchpoint (bp_hardware_breakpoint, 
					    i + 1, 0);
      if (target_resources_ok == 0)
	error (_("No hardware breakpoint support in the target."));
      else if (target_resources_ok < 0)
	error (_("Hardware breakpoints used exceeds limit."));
    }

  gdb_assert (sals.nelts > 0);

  for (i = 0; i < sals.nelts; ++i)
    {
      struct symtab_and_line sal = sals.sals[i];
      struct bp_location *loc;

      if (from_tty)
	{
	  struct gdbarch *loc_gdbarch = get_sal_arch (sal);
	  if (!loc_gdbarch)
	    loc_gdbarch = gdbarch;

	  describe_other_breakpoints (loc_gdbarch,
				      sal.pspace, sal.pc, sal.section, thread);
	}

      if (i == 0)
	{
	  b = set_raw_breakpoint (gdbarch, sal, type);
	  set_breakpoint_number (internal, b);
	  b->thread = thread;
	  b->task = task;
  
	  b->cond_string = cond_string;
	  b->ignore_count = ignore_count;
	  b->enable_state = enabled ? bp_enabled : bp_disabled;
	  b->disposition = disposition;
	  b->pspace = sals.sals[0].pspace;

	  if (type == bp_static_tracepoint)
	    {
	      struct static_tracepoint_marker marker;

	      if (is_marker_spec (addr_string))
		{
		  /* We already know the marker exists, otherwise, we
		     wouldn't see a sal for it.  */
		  char *p = &addr_string[3];
		  char *endp;
		  char *marker_str;
		  int i;

		  p = skip_spaces (p);

		  endp = skip_to_space (p);

		  marker_str = savestring (p, endp - p);
		  b->static_trace_marker_id = marker_str;

		  printf_filtered (_("Probed static tracepoint "
				     "marker \"%s\"\n"),
				   b->static_trace_marker_id);
		}
	      else if (target_static_tracepoint_marker_at (sal.pc, &marker))
		{
		  b->static_trace_marker_id = xstrdup (marker.str_id);
		  release_static_tracepoint_marker (&marker);

		  printf_filtered (_("Probed static tracepoint "
				     "marker \"%s\"\n"),
				   b->static_trace_marker_id);
		}
	      else
		warning (_("Couldn't determine the static "
			   "tracepoint marker to probe"));
	    }

	  if (enabled && b->pspace->executing_startup
	      && (b->type == bp_breakpoint
		  || b->type == bp_hardware_breakpoint))
	    b->enable_state = bp_startup_disabled;

	  loc = b->loc;
	}
      else
	{
	  loc = add_location_to_breakpoint (b, &sal);
	}

      if (bp_loc_is_permanent (loc))
	make_breakpoint_permanent (b);

      if (b->cond_string)
	{
	  char *arg = b->cond_string;
	  loc->cond = parse_exp_1 (&arg, block_for_pc (loc->address), 0);
	  if (*arg)
              error (_("Garbage %s follows condition"), arg);
	}
    }   

  b->display_canonical = display_canonical;
  if (addr_string)
    b->addr_string = addr_string;
  else
    /* addr_string has to be used or breakpoint_re_set will delete
       me.  */
    b->addr_string
      = xstrprintf ("*%s", paddress (b->loc->gdbarch, b->loc->address));

  b->ops = ops;
  if (internal)
    /* Do not mention breakpoints with a negative number, but do
       notify observers.  */
    observer_notify_breakpoint_created (b->number);
  else
    mention (b);
}

/* Remove element at INDEX_TO_REMOVE from SAL, shifting other
   elements to fill the void space.  */
static void
remove_sal (struct symtabs_and_lines *sal, int index_to_remove)
{
  int i = index_to_remove+1;
  int last_index = sal->nelts-1;

  for (;i <= last_index; ++i)
    sal->sals[i-1] = sal->sals[i];

  --(sal->nelts);
}

/* If appropriate, obtains all sals that correspond to the same file
   and line as SAL, in all program spaces.  Users debugging with IDEs,
   will want to set a breakpoint at foo.c:line, and not really care
   about program spaces.  This is done only if SAL does not have
   explicit PC and has line and file information.  If we got just a
   single expanded sal, return the original.

   Otherwise, if SAL.explicit_line is not set, filter out all sals for
   which the name of enclosing function is different from SAL.  This
   makes sure that if we have breakpoint originally set in template
   instantiation, say foo<int>(), we won't expand SAL to locations at
   the same line in all existing instantiations of 'foo'.  */

static struct symtabs_and_lines
expand_line_sal_maybe (struct symtab_and_line sal)
{
  struct symtabs_and_lines expanded;
  CORE_ADDR original_pc = sal.pc;
  char *original_function = NULL;
  int found;
  int i;
  struct cleanup *old_chain;

  /* If we have explicit pc, don't expand.
     If we have no line number, we can't expand.  */
  if (sal.explicit_pc || sal.line == 0 || sal.symtab == NULL)
    {
      expanded.nelts = 1;
      expanded.sals = xmalloc (sizeof (struct symtab_and_line));
      expanded.sals[0] = sal;
      return expanded;
    }

  sal.pc = 0;

  old_chain = save_current_space_and_thread ();

  switch_to_program_space_and_thread (sal.pspace);

  find_pc_partial_function (original_pc, &original_function, NULL, NULL);

  /* Note that expand_line_sal visits *all* program spaces.  */
  expanded = expand_line_sal (sal);

  if (expanded.nelts == 1)
    {
      /* We had one sal, we got one sal.  Return that sal, adjusting it
         past the function prologue if necessary.  */
      xfree (expanded.sals);
      expanded.nelts = 1;
      expanded.sals = xmalloc (sizeof (struct symtab_and_line));
      sal.pc = original_pc;
      expanded.sals[0] = sal;
      skip_prologue_sal (&expanded.sals[0]);
      do_cleanups (old_chain);
      return expanded;      
    }

  if (!sal.explicit_line)
    {
      CORE_ADDR func_addr, func_end;
      for (i = 0; i < expanded.nelts; ++i)
	{
	  CORE_ADDR pc = expanded.sals[i].pc;
	  char *this_function;

	  /* We need to switch threads as well since we're about to
	     read memory.  */
	  switch_to_program_space_and_thread (expanded.sals[i].pspace);

	  if (find_pc_partial_function (pc, &this_function, 
					&func_addr, &func_end))
	    {
	      if (this_function
		  && strcmp (this_function, original_function) != 0)
		{
		  remove_sal (&expanded, i);
		  --i;
		}
	    }
	}
    }

  /* Skip the function prologue if necessary.  */
  for (i = 0; i < expanded.nelts; ++i)
    skip_prologue_sal (&expanded.sals[i]);

  do_cleanups (old_chain);

  if (expanded.nelts <= 1)
    {
      /* This is un ugly workaround.  If we get zero expanded sals
         then something is really wrong.  Fix that by returning the
         original sal.  */

      xfree (expanded.sals);
      expanded.nelts = 1;
      expanded.sals = xmalloc (sizeof (struct symtab_and_line));
      sal.pc = original_pc;
      expanded.sals[0] = sal;
      return expanded;      
    }

  if (original_pc)
    {
      found = 0;
      for (i = 0; i < expanded.nelts; ++i)
	if (expanded.sals[i].pc == original_pc)
	  {
	    found = 1;
	    break;
	  }
      gdb_assert (found);
    }

  return expanded;
}

/* Add SALS.nelts breakpoints to the breakpoint table.  For each
   SALS.sal[i] breakpoint, include the corresponding ADDR_STRING[i]
   value.  COND_STRING, if not NULL, specified the condition to be
   used for all breakpoints.  Essentially the only case where
   SALS.nelts is not 1 is when we set a breakpoint on an overloaded
   function.  In that case, it's still not possible to specify
   separate conditions for different overloaded functions, so
   we take just a single condition string.
   
   NOTE: If the function succeeds, the caller is expected to cleanup
   the arrays ADDR_STRING, COND_STRING, and SALS (but not the
   array contents).  If the function fails (error() is called), the
   caller is expected to cleanups both the ADDR_STRING, COND_STRING,
   COND and SALS arrays and each of those arrays contents.  */

static void
create_breakpoints_sal (struct gdbarch *gdbarch,
			struct symtabs_and_lines sals,
			struct linespec_result *canonical,
			char *cond_string,
			enum bptype type, enum bpdisp disposition,
			int thread, int task, int ignore_count,
			struct breakpoint_ops *ops, int from_tty,
			int enabled, int internal)
{
  int i;

  for (i = 0; i < sals.nelts; ++i)
    {
      struct symtabs_and_lines expanded = 
	expand_line_sal_maybe (sals.sals[i]);

      create_breakpoint_sal (gdbarch, expanded, canonical->canonical[i],
			     cond_string, type, disposition,
			     thread, task, ignore_count, ops,
			     from_tty, enabled, internal,
			     canonical->special_display);
    }
}

/* Parse ADDRESS which is assumed to be a SAL specification possibly
   followed by conditionals.  On return, SALS contains an array of SAL
   addresses found.  ADDR_STRING contains a vector of (canonical)
   address strings.  ADDRESS points to the end of the SAL.

   The array and the line spec strings are allocated on the heap, it is
   the caller's responsibility to free them.  */

static void
parse_breakpoint_sals (char **address,
		       struct symtabs_and_lines *sals,
		       struct linespec_result *canonical)
{
  char *addr_start = *address;

  /* If no arg given, or if first arg is 'if ', use the default
     breakpoint.  */
  if ((*address) == NULL
      || (strncmp ((*address), "if", 2) == 0 && isspace ((*address)[2])))
    {
      if (default_breakpoint_valid)
	{
	  struct symtab_and_line sal;

	  init_sal (&sal);		/* Initialize to zeroes.  */
	  sals->sals = (struct symtab_and_line *)
	    xmalloc (sizeof (struct symtab_and_line));
	  sal.pc = default_breakpoint_address;
	  sal.line = default_breakpoint_line;
	  sal.symtab = default_breakpoint_symtab;
	  sal.pspace = default_breakpoint_pspace;
	  sal.section = find_pc_overlay (sal.pc);

	  /* "break" without arguments is equivalent to "break *PC"
	     where PC is the default_breakpoint_address.  So make sure
	     to set sal.explicit_pc to prevent GDB from trying to
	     expand the list of sals to include all other instances
	     with the same symtab and line.  */
	  sal.explicit_pc = 1;

	  sals->sals[0] = sal;
	  sals->nelts = 1;
	}
      else
	error (_("No default breakpoint address now."));
    }
  else
    {
      /* Force almost all breakpoints to be in terms of the
         current_source_symtab (which is decode_line_1's default).
         This should produce the results we want almost all of the
         time while leaving default_breakpoint_* alone.

         ObjC: However, don't match an Objective-C method name which
         may have a '+' or '-' succeeded by a '[' */
	 
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();
			
      if (default_breakpoint_valid
	  && (!cursal.symtab
 	      || ((strchr ("+-", (*address)[0]) != NULL)
 		  && ((*address)[1] != '['))))
	*sals = decode_line_1 (address, 1, default_breakpoint_symtab,
			       default_breakpoint_line, canonical);
      else
	*sals = decode_line_1 (address, 1, (struct symtab *) NULL, 0,
		               canonical);
    }
  /* For any SAL that didn't have a canonical string, fill one in.  */
  if (sals->nelts > 0 && canonical->canonical == NULL)
    canonical->canonical = xcalloc (sals->nelts, sizeof (char **));
  if (addr_start != (*address))
    {
      int i;

      for (i = 0; i < sals->nelts; i++)
	{
	  /* Add the string if not present.  */
	  if (canonical->canonical[i] == NULL)
	    canonical->canonical[i] = savestring (addr_start, 
						  (*address) - addr_start);
	}
    }
}


/* Convert each SAL into a real PC.  Verify that the PC can be
   inserted as a breakpoint.  If it can't throw an error.  */

static void
breakpoint_sals_to_pc (struct symtabs_and_lines *sals)
{    
  int i;

  for (i = 0; i < sals->nelts; i++)
    resolve_sal_pc (&sals->sals[i]);
}

/* Fast tracepoints may have restrictions on valid locations.  For
   instance, a fast tracepoint using a jump instead of a trap will
   likely have to overwrite more bytes than a trap would, and so can
   only be placed where the instruction is longer than the jump, or a
   multi-instruction sequence does not have a jump into the middle of
   it, etc.  */

static void
check_fast_tracepoint_sals (struct gdbarch *gdbarch,
			    struct symtabs_and_lines *sals)
{
  int i, rslt;
  struct symtab_and_line *sal;
  char *msg;
  struct cleanup *old_chain;

  for (i = 0; i < sals->nelts; i++)
    {
      sal = &sals->sals[i];

      rslt = gdbarch_fast_tracepoint_valid_at (gdbarch, sal->pc,
					       NULL, &msg);
      old_chain = make_cleanup (xfree, msg);

      if (!rslt)
	error (_("May not have a fast tracepoint at 0x%s%s"),
	       paddress (gdbarch, sal->pc), (msg ? msg : ""));

      do_cleanups (old_chain);
    }
}

static void
do_captured_parse_breakpoint (struct ui_out *ui, void *data)
{
  struct captured_parse_breakpoint_args *args = data;
  
  parse_breakpoint_sals (args->arg_p, args->sals_p, args->canonical_p);
}

/* Given TOK, a string specification of condition and thread, as
   accepted by the 'break' command, extract the condition
   string and thread number and set *COND_STRING and *THREAD.
   PC identifies the context at which the condition should be parsed.
   If no condition is found, *COND_STRING is set to NULL.
   If no thread is found, *THREAD is set to -1.  */
static void 
find_condition_and_thread (char *tok, CORE_ADDR pc, 
			   char **cond_string, int *thread, int *task)
{
  *cond_string = NULL;
  *thread = -1;
  while (tok && *tok)
    {
      char *end_tok;
      int toklen;
      char *cond_start = NULL;
      char *cond_end = NULL;

      tok = skip_spaces (tok);
      
      end_tok = skip_to_space (tok);
      
      toklen = end_tok - tok;
      
      if (toklen >= 1 && strncmp (tok, "if", toklen) == 0)
	{
	  struct expression *expr;

	  tok = cond_start = end_tok + 1;
	  expr = parse_exp_1 (&tok, block_for_pc (pc), 0);
	  xfree (expr);
	  cond_end = tok;
	  *cond_string = savestring (cond_start, 
				     cond_end - cond_start);
	}
      else if (toklen >= 1 && strncmp (tok, "thread", toklen) == 0)
	{
	  char *tmptok;
	  
	  tok = end_tok + 1;
	  tmptok = tok;
	  *thread = strtol (tok, &tok, 0);
	  if (tok == tmptok)
	    error (_("Junk after thread keyword."));
	  if (!valid_thread_id (*thread))
	    error (_("Unknown thread %d."), *thread);
	}
      else if (toklen >= 1 && strncmp (tok, "task", toklen) == 0)
	{
	  char *tmptok;

	  tok = end_tok + 1;
	  tmptok = tok;
	  *task = strtol (tok, &tok, 0);
	  if (tok == tmptok)
	    error (_("Junk after task keyword."));
	  if (!valid_task_id (*task))
	    error (_("Unknown task %d."), *task);
	}
      else
	error (_("Junk at end of arguments."));
    }
}

/* Decode a static tracepoint marker spec.  */

static struct symtabs_and_lines
decode_static_tracepoint_spec (char **arg_p)
{
  VEC(static_tracepoint_marker_p) *markers = NULL;
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  struct symbol *sym;
  struct cleanup *old_chain;
  char *p = &(*arg_p)[3];
  char *endp;
  char *marker_str;
  int i;

  p = skip_spaces (p);

  endp = skip_to_space (p);

  marker_str = savestring (p, endp - p);
  old_chain = make_cleanup (xfree, marker_str);

  markers = target_static_tracepoint_markers_by_strid (marker_str);
  if (VEC_empty(static_tracepoint_marker_p, markers))
    error (_("No known static tracepoint marker named %s"), marker_str);

  sals.nelts = VEC_length(static_tracepoint_marker_p, markers);
  sals.sals = xmalloc (sizeof *sals.sals * sals.nelts);

  for (i = 0; i < sals.nelts; i++)
    {
      struct static_tracepoint_marker *marker;

      marker = VEC_index (static_tracepoint_marker_p, markers, i);

      init_sal (&sals.sals[i]);

      sals.sals[i] = find_pc_line (marker->address, 0);
      sals.sals[i].pc = marker->address;

      release_static_tracepoint_marker (marker);
    }

  do_cleanups (old_chain);

  *arg_p = endp;
  return sals;
}

/* Set a breakpoint.  This function is shared between CLI and MI
   functions for setting a breakpoint.  This function has two major
   modes of operations, selected by the PARSE_CONDITION_AND_THREAD
   parameter.  If non-zero, the function will parse arg, extracting
   breakpoint location, address and thread.  Otherwise, ARG is just
   the location of breakpoint, with condition and thread specified by
   the COND_STRING and THREAD parameters.  If INTERNAL is non-zero,
   the breakpoint number will be allocated from the internal
   breakpoint count.  Returns true if any breakpoint was created;
   false otherwise.  */

int
create_breakpoint (struct gdbarch *gdbarch,
		   char *arg, char *cond_string, int thread,
		   int parse_condition_and_thread,
		   int tempflag, enum bptype type_wanted,
		   int ignore_count,
		   enum auto_boolean pending_break_support,
		   struct breakpoint_ops *ops,
		   int from_tty, int enabled, int internal)
{
  struct gdb_exception e;
  struct symtabs_and_lines sals;
  struct symtab_and_line pending_sal;
  char *copy_arg;
  char *addr_start = arg;
  struct linespec_result canonical;
  struct cleanup *old_chain;
  struct cleanup *bkpt_chain = NULL;
  struct captured_parse_breakpoint_args parse_args;
  int i;
  int pending = 0;
  int task = 0;
  int prev_bkpt_count = breakpoint_count;

  sals.sals = NULL;
  sals.nelts = 0;
  init_linespec_result (&canonical);

  parse_args.arg_p = &arg;
  parse_args.sals_p = &sals;
  parse_args.canonical_p = &canonical;

  if (type_wanted == bp_static_tracepoint && is_marker_spec (arg))
    {
      int i;

      sals = decode_static_tracepoint_spec (&arg);

      copy_arg = savestring (addr_start, arg - addr_start);
      canonical.canonical = xcalloc (sals.nelts, sizeof (char **));
      for (i = 0; i < sals.nelts; i++)
	canonical.canonical[i] = xstrdup (copy_arg);
      goto done;
    }

  e = catch_exception (uiout, do_captured_parse_breakpoint, 
		       &parse_args, RETURN_MASK_ALL);

  /* If caller is interested in rc value from parse, set value.  */
  switch (e.reason)
    {
    case RETURN_QUIT:
      throw_exception (e);
    case RETURN_ERROR:
      switch (e.error)
	{
	case NOT_FOUND_ERROR:

	  /* If pending breakpoint support is turned off, throw
	     error.  */

	  if (pending_break_support == AUTO_BOOLEAN_FALSE)
	    throw_exception (e);

	  exception_print (gdb_stderr, e);

          /* If pending breakpoint support is auto query and the user
	     selects no, then simply return the error code.  */
	  if (pending_break_support == AUTO_BOOLEAN_AUTO
	      && !nquery (_("Make breakpoint pending on "
			    "future shared library load? ")))
	    return 0;

	  /* At this point, either the user was queried about setting
	     a pending breakpoint and selected yes, or pending
	     breakpoint behavior is on and thus a pending breakpoint
	     is defaulted on behalf of the user.  */
	  copy_arg = xstrdup (addr_start);
	  canonical.canonical = &copy_arg;
	  sals.nelts = 1;
	  sals.sals = &pending_sal;
	  pending_sal.pc = 0;
	  pending = 1;
	  break;
	default:
	  throw_exception (e);
	}
      break;
    default:
      if (!sals.nelts)
	return 0;
    }

  done:

  /* Create a chain of things that always need to be cleaned up.  */
  old_chain = make_cleanup (null_cleanup, 0);

  if (!pending)
    {
      /* Make sure that all storage allocated to SALS gets freed.  */
      make_cleanup (xfree, sals.sals);
      
      /* Cleanup the canonical array but not its contents.  */
      make_cleanup (xfree, canonical.canonical);
    }

  /* ----------------------------- SNIP -----------------------------
     Anything added to the cleanup chain beyond this point is assumed
     to be part of a breakpoint.  If the breakpoint create succeeds
     then the memory is not reclaimed.  */
  bkpt_chain = make_cleanup (null_cleanup, 0);

  /* Mark the contents of the canonical for cleanup.  These go on
     the bkpt_chain and only occur if the breakpoint create fails.  */
  for (i = 0; i < sals.nelts; i++)
    {
      if (canonical.canonical[i] != NULL)
	make_cleanup (xfree, canonical.canonical[i]);
    }

  /* Resolve all line numbers to PC's and verify that the addresses
     are ok for the target.  */
  if (!pending)
    breakpoint_sals_to_pc (&sals);

  /* Fast tracepoints may have additional restrictions on location.  */
  if (type_wanted == bp_fast_tracepoint)
    check_fast_tracepoint_sals (gdbarch, &sals);

  /* Verify that condition can be parsed, before setting any
     breakpoints.  Allocate a separate condition expression for each
     breakpoint.  */
  if (!pending)
    {
      if (parse_condition_and_thread)
        {
            /* Here we only parse 'arg' to separate condition
               from thread number, so parsing in context of first
               sal is OK.  When setting the breakpoint we'll 
               re-parse it in context of each sal.  */
            cond_string = NULL;
            thread = -1;
            find_condition_and_thread (arg, sals.sals[0].pc, &cond_string,
                                       &thread, &task);
            if (cond_string)
                make_cleanup (xfree, cond_string);
        }
      else
        {
            /* Create a private copy of condition string.  */
            if (cond_string)
            {
                cond_string = xstrdup (cond_string);
                make_cleanup (xfree, cond_string);
            }
        }

      /* If the user is creating a static tracepoint by marker id
	 (strace -m MARKER_ID), then store the sals index, so that
	 breakpoint_re_set can try to match up which of the newly
	 found markers corresponds to this one, and, don't try to
	 expand multiple locations for each sal, given than SALS
	 already should contain all sals for MARKER_ID.  */
      if (type_wanted == bp_static_tracepoint
	  && is_marker_spec (canonical.canonical[0]))
	{
	  int i;

	  for (i = 0; i < sals.nelts; ++i)
	    {
	      struct symtabs_and_lines expanded;
	      struct breakpoint *tp;
	      struct cleanup *old_chain;

	      expanded.nelts = 1;
	      expanded.sals = xmalloc (sizeof (struct symtab_and_line));
	      expanded.sals[0] = sals.sals[i];
	      old_chain = make_cleanup (xfree, expanded.sals);

	      create_breakpoint_sal (gdbarch, expanded, canonical.canonical[i],
				     cond_string, type_wanted,
				     tempflag ? disp_del : disp_donttouch,
				     thread, task, ignore_count, ops,
				     from_tty, enabled, internal,
				     canonical.special_display);

	      do_cleanups (old_chain);

	      /* Get the tracepoint we just created.  */
	      if (internal)
		tp = get_breakpoint (internal_breakpoint_number);
	      else
		tp = get_breakpoint (breakpoint_count);
	      gdb_assert (tp != NULL);

	      /* Given that its possible to have multiple markers with
		 the same string id, if the user is creating a static
		 tracepoint by marker id ("strace -m MARKER_ID"), then
		 store the sals index, so that breakpoint_re_set can
		 try to match up which of the newly found markers
		 corresponds to this one  */
	      tp->static_trace_marker_id_idx = i;
	    }
	}
      else
	create_breakpoints_sal (gdbarch, sals, &canonical, cond_string,
				type_wanted,
				tempflag ? disp_del : disp_donttouch,
				thread, task, ignore_count, ops, from_tty,
				enabled, internal);
    }
  else
    {
      struct breakpoint *b;

      make_cleanup (xfree, copy_arg);

      b = set_raw_breakpoint_without_location (gdbarch, type_wanted);
      set_breakpoint_number (internal, b);
      b->thread = -1;
      b->addr_string = canonical.canonical[0];
      b->cond_string = NULL;
      b->ignore_count = ignore_count;
      b->disposition = tempflag ? disp_del : disp_donttouch;
      b->condition_not_parsed = 1;
      b->ops = ops;
      b->enable_state = enabled ? bp_enabled : bp_disabled;
      b->pspace = current_program_space;
      b->py_bp_object = NULL;

      if (enabled && b->pspace->executing_startup
	  && (b->type == bp_breakpoint
	      || b->type == bp_hardware_breakpoint))
	b->enable_state = bp_startup_disabled;

      if (internal)
        /* Do not mention breakpoints with a negative number, 
	   but do notify observers.  */
        observer_notify_breakpoint_created (b->number);
      else
        mention (b);
    }
  
  if (sals.nelts > 1)
    {
      warning (_("Multiple breakpoints were set.\nUse the "
		 "\"delete\" command to delete unwanted breakpoints."));
      prev_breakpoint_count = prev_bkpt_count;
    }

  /* That's it.  Discard the cleanups for data inserted into the
     breakpoint.  */
  discard_cleanups (bkpt_chain);
  /* But cleanup everything else.  */
  do_cleanups (old_chain);

  /* error call may happen here - have BKPT_CHAIN already discarded.  */
  update_global_location_list (1);

  return 1;
}

/* Set a breakpoint. 
   ARG is a string describing breakpoint address,
   condition, and thread.
   FLAG specifies if a breakpoint is hardware on,
   and if breakpoint is temporary, using BP_HARDWARE_FLAG
   and BP_TEMPFLAG.  */
   
static void
break_command_1 (char *arg, int flag, int from_tty)
{
  int tempflag = flag & BP_TEMPFLAG;
  enum bptype type_wanted = (flag & BP_HARDWAREFLAG
			     ? bp_hardware_breakpoint
			     : bp_breakpoint);

  create_breakpoint (get_current_arch (),
		     arg,
		     NULL, 0, 1 /* parse arg */,
		     tempflag, type_wanted,
		     0 /* Ignore count */,
		     pending_break_support,
		     NULL /* breakpoint_ops */,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */);
}


/* Helper function for break_command_1 and disassemble_command.  */

void
resolve_sal_pc (struct symtab_and_line *sal)
{
  CORE_ADDR pc;

  if (sal->pc == 0 && sal->symtab != NULL)
    {
      if (!find_line_pc (sal->symtab, sal->line, &pc))
	error (_("No line %d in file \"%s\"."),
	       sal->line, sal->symtab->filename);
      sal->pc = pc;

      /* If this SAL corresponds to a breakpoint inserted using a line
         number, then skip the function prologue if necessary.  */
      if (sal->explicit_line)
	skip_prologue_sal (sal);
    }

  if (sal->section == 0 && sal->symtab != NULL)
    {
      struct blockvector *bv;
      struct block *b;
      struct symbol *sym;

      bv = blockvector_for_pc_sect (sal->pc, 0, &b, sal->symtab);
      if (bv != NULL)
	{
	  sym = block_linkage_function (b);
	  if (sym != NULL)
	    {
	      fixup_symbol_section (sym, sal->symtab->objfile);
	      sal->section = SYMBOL_OBJ_SECTION (sym);
	    }
	  else
	    {
	      /* It really is worthwhile to have the section, so we'll
	         just have to look harder. This case can be executed
	         if we have line numbers but no functions (as can
	         happen in assembly source).  */

	      struct minimal_symbol *msym;
	      struct cleanup *old_chain = save_current_space_and_thread ();

	      switch_to_program_space_and_thread (sal->pspace);

	      msym = lookup_minimal_symbol_by_pc (sal->pc);
	      if (msym)
		sal->section = SYMBOL_OBJ_SECTION (msym);

	      do_cleanups (old_chain);
	    }
	}
    }
}

void
break_command (char *arg, int from_tty)
{
  break_command_1 (arg, 0, from_tty);
}

void
tbreak_command (char *arg, int from_tty)
{
  break_command_1 (arg, BP_TEMPFLAG, from_tty);
}

static void
hbreak_command (char *arg, int from_tty)
{
  break_command_1 (arg, BP_HARDWAREFLAG, from_tty);
}

static void
thbreak_command (char *arg, int from_tty)
{
  break_command_1 (arg, (BP_TEMPFLAG | BP_HARDWAREFLAG), from_tty);
}

static void
stop_command (char *arg, int from_tty)
{
  printf_filtered (_("Specify the type of breakpoint to set.\n\
Usage: stop in <function | address>\n\
       stop at <line>\n"));
}

static void
stopin_command (char *arg, int from_tty)
{
  int badInput = 0;

  if (arg == (char *) NULL)
    badInput = 1;
  else if (*arg != '*')
    {
      char *argptr = arg;
      int hasColon = 0;

      /* Look for a ':'.  If this is a line number specification, then
         say it is bad, otherwise, it should be an address or
         function/method name.  */
      while (*argptr && !hasColon)
	{
	  hasColon = (*argptr == ':');
	  argptr++;
	}

      if (hasColon)
	badInput = (*argptr != ':');	/* Not a class::method */
      else
	badInput = isdigit (*arg);	/* a simple line number */
    }

  if (badInput)
    printf_filtered (_("Usage: stop in <function | address>\n"));
  else
    break_command_1 (arg, 0, from_tty);
}

static void
stopat_command (char *arg, int from_tty)
{
  int badInput = 0;

  if (arg == (char *) NULL || *arg == '*')	/* no line number */
    badInput = 1;
  else
    {
      char *argptr = arg;
      int hasColon = 0;

      /* Look for a ':'.  If there is a '::' then get out, otherwise
         it is probably a line number.  */
      while (*argptr && !hasColon)
	{
	  hasColon = (*argptr == ':');
	  argptr++;
	}

      if (hasColon)
	badInput = (*argptr == ':');	/* we have class::method */
      else
	badInput = !isdigit (*arg);	/* not a line number */
    }

  if (badInput)
    printf_filtered (_("Usage: stop at <line>\n"));
  else
    break_command_1 (arg, 0, from_tty);
}

/* Implement the "breakpoint_hit" breakpoint_ops method for
   ranged breakpoints.  */

static int
breakpoint_hit_ranged_breakpoint (const struct bp_location *bl,
				  struct address_space *aspace,
				  CORE_ADDR bp_addr)
{
  return breakpoint_address_match_range (bl->pspace->aspace, bl->address,
					 bl->length, aspace, bp_addr);
}

/* Implement the "resources_needed" breakpoint_ops method for
   ranged breakpoints.  */

static int
resources_needed_ranged_breakpoint (const struct bp_location *bl)
{
  return target_ranged_break_num_registers ();
}

/* Implement the "print_it" breakpoint_ops method for
   ranged breakpoints.  */

static enum print_stop_action
print_it_ranged_breakpoint (struct breakpoint *b)
{
  struct bp_location *bl = b->loc;

  gdb_assert (b->type == bp_hardware_breakpoint);

  /* Ranged breakpoints have only one location.  */
  gdb_assert (bl && bl->next == NULL);

  annotate_breakpoint (b->number);
  if (b->disposition == disp_del)
    ui_out_text (uiout, "\nTemporary ranged breakpoint ");
  else
    ui_out_text (uiout, "\nRanged breakpoint ");
  if (ui_out_is_mi_like_p (uiout))
    {
      ui_out_field_string (uiout, "reason",
		      async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
      ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
    }
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, ", ");

  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" breakpoint_ops method for
   ranged breakpoints.  */

static void
print_one_ranged_breakpoint (struct breakpoint *b,
			     struct bp_location **last_loc)
{
  struct bp_location *bl = b->loc;
  struct value_print_options opts;

  /* Ranged breakpoints have only one location.  */
  gdb_assert (bl && bl->next == NULL);

  get_user_print_options (&opts);

  if (opts.addressprint)
    /* We don't print the address range here, it will be printed later
       by print_one_detail_ranged_breakpoint.  */
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);
  print_breakpoint_location (b, bl);
  *last_loc = bl;
}

/* Implement the "print_one_detail" breakpoint_ops method for
   ranged breakpoints.  */

static void
print_one_detail_ranged_breakpoint (const struct breakpoint *b,
				    struct ui_out *uiout)
{
  CORE_ADDR address_start, address_end;
  struct bp_location *bl = b->loc;
  struct ui_stream *stb = ui_out_stream_new (uiout);
  struct cleanup *cleanup = make_cleanup_ui_out_stream_delete (stb);

  gdb_assert (bl);

  address_start = bl->address;
  address_end = address_start + bl->length - 1;

  ui_out_text (uiout, "\taddress range: ");
  fprintf_unfiltered (stb->stream, "[%s, %s]",
		      print_core_address (bl->gdbarch, address_start),
		      print_core_address (bl->gdbarch, address_end));
  ui_out_field_stream (uiout, "addr", stb);
  ui_out_text (uiout, "\n");

  do_cleanups (cleanup);
}

/* Implement the "print_mention" breakpoint_ops method for
   ranged breakpoints.  */

static void
print_mention_ranged_breakpoint (struct breakpoint *b)
{
  struct bp_location *bl = b->loc;

  gdb_assert (bl);
  gdb_assert (b->type == bp_hardware_breakpoint);

  if (ui_out_is_mi_like_p (uiout))
    return;

  printf_filtered (_("Hardware assisted ranged breakpoint %d from %s to %s."),
		   b->number, paddress (bl->gdbarch, bl->address),
		   paddress (bl->gdbarch, bl->address + bl->length - 1));
}

/* Implement the "print_recreate" breakpoint_ops method for
   ranged breakpoints.  */

static void
print_recreate_ranged_breakpoint (struct breakpoint *b, struct ui_file *fp)
{
  fprintf_unfiltered (fp, "break-range %s, %s", b->addr_string,
		      b->addr_string_range_end);
}

/* The breakpoint_ops structure to be used in ranged breakpoints.  */

static struct breakpoint_ops ranged_breakpoint_ops =
{
  NULL, /* insert */
  NULL, /* remove */
  breakpoint_hit_ranged_breakpoint,
  resources_needed_ranged_breakpoint,
  print_it_ranged_breakpoint,
  print_one_ranged_breakpoint,
  print_one_detail_ranged_breakpoint,
  print_mention_ranged_breakpoint,
  print_recreate_ranged_breakpoint
};

/* Find the address where the end of the breakpoint range should be
   placed, given the SAL of the end of the range.  This is so that if
   the user provides a line number, the end of the range is set to the
   last instruction of the given line.  */

static CORE_ADDR
find_breakpoint_range_end (struct symtab_and_line sal)
{
  CORE_ADDR end;

  /* If the user provided a PC value, use it.  Otherwise,
     find the address of the end of the given location.  */
  if (sal.explicit_pc)
    end = sal.pc;
  else
    {
      int ret;
      CORE_ADDR start;

      ret = find_line_pc_range (sal, &start, &end);
      if (!ret)
	error (_("Could not find location of the end of the range."));

      /* find_line_pc_range returns the start of the next line.  */
      end--;
    }

  return end;
}

/* Implement the "break-range" CLI command.  */

static void
break_range_command (char *arg, int from_tty)
{
  char *arg_start, *addr_string_start, *addr_string_end;
  struct linespec_result canonical_start, canonical_end;
  int bp_count, can_use_bp, length;
  CORE_ADDR end;
  struct breakpoint *b;
  struct symtab_and_line sal_start, sal_end;
  struct symtabs_and_lines sals_start, sals_end;
  struct cleanup *cleanup_bkpt;

  /* We don't support software ranged breakpoints.  */
  if (target_ranged_break_num_registers () < 0)
    error (_("This target does not support hardware ranged breakpoints."));

  bp_count = hw_breakpoint_used_count ();
  bp_count += target_ranged_break_num_registers ();
  can_use_bp = target_can_use_hardware_watchpoint (bp_hardware_breakpoint,
						   bp_count, 0);
  if (can_use_bp < 0)
    error (_("Hardware breakpoints used exceeds limit."));

  if (arg == NULL || arg[0] == '\0')
    error(_("No address range specified."));

  sals_start.sals = NULL;
  sals_start.nelts = 0;
  init_linespec_result (&canonical_start);

  while (*arg == ' ' || *arg == '\t')
    arg++;

  parse_breakpoint_sals (&arg, &sals_start, &canonical_start);

  sal_start = sals_start.sals[0];
  addr_string_start = canonical_start.canonical[0];
  cleanup_bkpt = make_cleanup (xfree, addr_string_start);
  xfree (sals_start.sals);
  xfree (canonical_start.canonical);

  if (arg[0] != ',')
    error (_("Too few arguments."));
  else if (sals_start.nelts == 0)
    error (_("Could not find location of the beginning of the range."));
  else if (sals_start.nelts != 1)
    error (_("Cannot create a ranged breakpoint with multiple locations."));

  resolve_sal_pc (&sal_start);

  arg++;	/* Skip the comma.  */
  while (*arg == ' ' || *arg == '\t')
    arg++;

  /* Parse the end location.  */

  sals_end.sals = NULL;
  sals_end.nelts = 0;
  init_linespec_result (&canonical_end);
  arg_start = arg;

  /* We call decode_line_1 directly here instead of using
     parse_breakpoint_sals because we need to specify the start location's
     symtab and line as the default symtab and line for the end of the
     range.  This makes it possible to have ranges like "foo.c:27, +14",
     where +14 means 14 lines from the start location.  */
  sals_end = decode_line_1 (&arg, 1, sal_start.symtab, sal_start.line,
			    &canonical_end);

  /* canonical_end can be NULL if it was of the form "*0xdeadbeef".  */
  if (canonical_end.canonical == NULL)
    canonical_end.canonical = xcalloc (1, sizeof (char **));
  /* Add the string if not present.  */
  if (arg_start != arg && canonical_end.canonical[0] == NULL)
    canonical_end.canonical[0] = savestring (arg_start, arg - arg_start);

  sal_end = sals_end.sals[0];
  addr_string_end = canonical_end.canonical[0];
  make_cleanup (xfree, addr_string_end);
  xfree (sals_end.sals);
  xfree (canonical_end.canonical);

  if (sals_end.nelts == 0)
    error (_("Could not find location of the end of the range."));
  else if (sals_end.nelts != 1)
    error (_("Cannot create a ranged breakpoint with multiple locations."));

  resolve_sal_pc (&sal_end);

  end = find_breakpoint_range_end (sal_end);
  if (sal_start.pc > end)
    error (_("Invalid address range, end preceeds start."));

  length = end - sal_start.pc + 1;
  if (length < 0)
    /* Length overflowed.  */
    error (_("Address range too large."));
  else if (length == 1)
    {
      /* This range is simple enough to be handled by
	 the `hbreak' command.  */
      hbreak_command (addr_string_start, 1);

      do_cleanups (cleanup_bkpt);

      return;
    }

  /* Now set up the breakpoint.  */
  b = set_raw_breakpoint (get_current_arch (), sal_start,
			  bp_hardware_breakpoint);
  set_breakpoint_count (breakpoint_count + 1);
  b->number = breakpoint_count;
  b->disposition = disp_donttouch;
  b->addr_string = addr_string_start;
  b->addr_string_range_end = addr_string_end;
  b->ops = &ranged_breakpoint_ops;
  b->loc->length = length;

  discard_cleanups (cleanup_bkpt);

  mention (b);
  update_global_location_list (1);
}

/*  Return non-zero if EXP is verified as constant.  Returned zero
    means EXP is variable.  Also the constant detection may fail for
    some constant expressions and in such case still falsely return
    zero.  */
static int
watchpoint_exp_is_const (const struct expression *exp)
{
  int i = exp->nelts;

  while (i > 0)
    {
      int oplenp, argsp;

      /* We are only interested in the descriptor of each element.  */
      operator_length (exp, i, &oplenp, &argsp);
      i -= oplenp;

      switch (exp->elts[i].opcode)
	{
	case BINOP_ADD:
	case BINOP_SUB:
	case BINOP_MUL:
	case BINOP_DIV:
	case BINOP_REM:
	case BINOP_MOD:
	case BINOP_LSH:
	case BINOP_RSH:
	case BINOP_LOGICAL_AND:
	case BINOP_LOGICAL_OR:
	case BINOP_BITWISE_AND:
	case BINOP_BITWISE_IOR:
	case BINOP_BITWISE_XOR:
	case BINOP_EQUAL:
	case BINOP_NOTEQUAL:
	case BINOP_LESS:
	case BINOP_GTR:
	case BINOP_LEQ:
	case BINOP_GEQ:
	case BINOP_REPEAT:
	case BINOP_COMMA:
	case BINOP_EXP:
	case BINOP_MIN:
	case BINOP_MAX:
	case BINOP_INTDIV:
	case BINOP_CONCAT:
	case BINOP_IN:
	case BINOP_RANGE:
	case TERNOP_COND:
	case TERNOP_SLICE:
	case TERNOP_SLICE_COUNT:

	case OP_LONG:
	case OP_DOUBLE:
	case OP_DECFLOAT:
	case OP_LAST:
	case OP_COMPLEX:
	case OP_STRING:
	case OP_BITSTRING:
	case OP_ARRAY:
	case OP_TYPE:
	case OP_NAME:
	case OP_OBJC_NSSTRING:

	case UNOP_NEG:
	case UNOP_LOGICAL_NOT:
	case UNOP_COMPLEMENT:
	case UNOP_ADDR:
	case UNOP_HIGH:
	  /* Unary, binary and ternary operators: We have to check
	     their operands.  If they are constant, then so is the
	     result of that operation.  For instance, if A and B are
	     determined to be constants, then so is "A + B".

	     UNOP_IND is one exception to the rule above, because the
	     value of *ADDR is not necessarily a constant, even when
	     ADDR is.  */
	  break;

	case OP_VAR_VALUE:
	  /* Check whether the associated symbol is a constant.

	     We use SYMBOL_CLASS rather than TYPE_CONST because it's
	     possible that a buggy compiler could mark a variable as
	     constant even when it is not, and TYPE_CONST would return
	     true in this case, while SYMBOL_CLASS wouldn't.

	     We also have to check for function symbols because they
	     are always constant.  */
	  {
	    struct symbol *s = exp->elts[i + 2].symbol;

	    if (SYMBOL_CLASS (s) != LOC_BLOCK
		&& SYMBOL_CLASS (s) != LOC_CONST
		&& SYMBOL_CLASS (s) != LOC_CONST_BYTES)
	      return 0;
	    break;
	  }

	/* The default action is to return 0 because we are using
	   the optimistic approach here: If we don't know something,
	   then it is not a constant.  */
	default:
	  return 0;
	}
    }

  return 1;
}

/* Implement the "insert" breakpoint_ops method for hardware watchpoints.  */

static int
insert_watchpoint (struct bp_location *bl)
{
  int length = bl->owner->exact? 1 : bl->length;

  return target_insert_watchpoint (bl->address, length, bl->watchpoint_type,
				   bl->owner->cond_exp);
}

/* Implement the "remove" breakpoint_ops method for hardware watchpoints.  */

static int
remove_watchpoint (struct bp_location *bl)
{
  int length = bl->owner->exact? 1 : bl->length;

  return target_remove_watchpoint (bl->address, length, bl->watchpoint_type,
				   bl->owner->cond_exp);
}

/* Implement the "resources_needed" breakpoint_ops method for
   hardware watchpoints.  */

static int
resources_needed_watchpoint (const struct bp_location *bl)
{
  int length = bl->owner->exact? 1 : bl->length;

  return target_region_ok_for_hw_watchpoint (bl->address, length);
}

/* The breakpoint_ops structure to be used in hardware watchpoints.  */

static struct breakpoint_ops watchpoint_breakpoint_ops =
{
  insert_watchpoint,
  remove_watchpoint,
  NULL, /* breakpoint_hit */
  resources_needed_watchpoint,
  NULL, /* print_it */
  NULL, /* print_one */
  NULL, /* print_one_detail */
  NULL, /* print_mention */
  NULL  /* print_recreate */
};

/* accessflag:  hw_write:  watch write, 
                hw_read:   watch read, 
		hw_access: watch access (read or write) */
static void
watch_command_1 (char *arg, int accessflag, int from_tty,
		 int just_location, int internal)
{
  struct breakpoint *b, *scope_breakpoint = NULL;
  struct expression *exp;
  struct block *exp_valid_block = NULL, *cond_exp_valid_block = NULL;
  struct value *val, *mark, *result;
  struct frame_info *frame;
  char *exp_start = NULL;
  char *exp_end = NULL;
  char *tok, *id_tok_start, *end_tok;
  int toklen;
  char *cond_start = NULL;
  char *cond_end = NULL;
  int i, other_type_used, target_resources_ok = 0;
  enum bptype bp_type;
  int reg_cnt = 0;
  int thread = -1;
  int pc = 0;

  /* Make sure that we actually have parameters to parse.  */
  if (arg != NULL && arg[0] != '\0')
    {
      toklen = strlen (arg); /* Size of argument list.  */

      /* Points tok to the end of the argument list.  */
      tok = arg + toklen - 1;

      /* Go backwards in the parameters list.  Skip the last
         parameter.  If we're expecting a 'thread <thread_num>'
         parameter, this should be the thread identifier.  */
      while (tok > arg && (*tok == ' ' || *tok == '\t'))
        tok--;
      while (tok > arg && (*tok != ' ' && *tok != '\t'))
        tok--;

      /* Points end_tok to the beginning of the last token.  */
      id_tok_start = tok + 1;

      /* Go backwards in the parameters list.  Skip one more
         parameter.  If we're expecting a 'thread <thread_num>'
         parameter, we should reach a "thread" token.  */
      while (tok > arg && (*tok == ' ' || *tok == '\t'))
        tok--;

      end_tok = tok;

      while (tok > arg && (*tok != ' ' && *tok != '\t'))
        tok--;

      /* Move the pointer forward to skip the whitespace and
         calculate the length of the token.  */
      tok++;
      toklen = end_tok - tok;

      if (toklen >= 1 && strncmp (tok, "thread", toklen) == 0)
        {
          /* At this point we've found a "thread" token, which means
             the user is trying to set a watchpoint that triggers
             only in a specific thread.  */
          char *endp;

          /* Extract the thread ID from the next token.  */
          thread = strtol (id_tok_start, &endp, 0);

          /* Check if the user provided a valid numeric value for the
             thread ID.  */
          if (*endp != ' ' && *endp != '\t' && *endp != '\0')
            error (_("Invalid thread ID specification %s."), id_tok_start);

          /* Check if the thread actually exists.  */
          if (!valid_thread_id (thread))
            error (_("Unknown thread %d."), thread);

          /* Truncate the string and get rid of the thread <thread_num>
             parameter before the parameter list is parsed by the
             evaluate_expression() function.  */
          *tok = '\0';
        }
    }

  /* Parse the rest of the arguments.  */
  innermost_block = NULL;
  exp_start = arg;
  exp = parse_exp_1 (&arg, 0, 0);
  exp_end = arg;
  /* Remove trailing whitespace from the expression before saving it.
     This makes the eventual display of the expression string a bit
     prettier.  */
  while (exp_end > exp_start && (exp_end[-1] == ' ' || exp_end[-1] == '\t'))
    --exp_end;

  /* Checking if the expression is not constant.  */
  if (watchpoint_exp_is_const (exp))
    {
      int len;

      len = exp_end - exp_start;
      while (len > 0 && isspace (exp_start[len - 1]))
	len--;
      error (_("Cannot watch constant value `%.*s'."), len, exp_start);
    }

  exp_valid_block = innermost_block;
  mark = value_mark ();
  fetch_subexp_value (exp, &pc, &val, &result, NULL);

  if (just_location)
    {
      exp_valid_block = NULL;
      val = value_addr (result);
      release_value (val);
      value_free_to_mark (mark);
    }
  else if (val != NULL)
    release_value (val);

  tok = skip_spaces (arg);
  end_tok = skip_to_space (tok);

  toklen = end_tok - tok;
  if (toklen >= 1 && strncmp (tok, "if", toklen) == 0)
    {
      struct expression *cond;

      innermost_block = NULL;
      tok = cond_start = end_tok + 1;
      cond = parse_exp_1 (&tok, 0, 0);

      /* The watchpoint expression may not be local, but the condition
	 may still be.  E.g.: `watch global if local > 0'.  */
      cond_exp_valid_block = innermost_block;

      xfree (cond);
      cond_end = tok;
    }
  if (*tok)
    error (_("Junk at end of command."));

  if (accessflag == hw_read)
    bp_type = bp_read_watchpoint;
  else if (accessflag == hw_access)
    bp_type = bp_access_watchpoint;
  else
    bp_type = bp_hardware_watchpoint;

  reg_cnt = can_use_hardware_watchpoint (val, target_exact_watchpoints);
  if (reg_cnt == 0 && bp_type != bp_hardware_watchpoint)
    error (_("Expression cannot be implemented with read/access watchpoint."));
  if (reg_cnt != 0)
    {
      i = hw_watchpoint_used_count (bp_type, &other_type_used);
      target_resources_ok = 
	target_can_use_hardware_watchpoint (bp_type, i + reg_cnt,
					    other_type_used);
      if (target_resources_ok == 0 && bp_type != bp_hardware_watchpoint)
	error (_("Target does not support this type of hardware watchpoint."));

      if (target_resources_ok < 0 && bp_type != bp_hardware_watchpoint)
	error (_("Target can only support one kind "
		 "of HW watchpoint at a time."));
    }

  /* Change the type of breakpoint to an ordinary watchpoint if a
     hardware watchpoint could not be set.  */
  if (!reg_cnt || target_resources_ok <= 0)
    bp_type = bp_watchpoint;

  frame = block_innermost_frame (exp_valid_block);

  /* If the expression is "local", then set up a "watchpoint scope"
     breakpoint at the point where we've left the scope of the watchpoint
     expression.  Create the scope breakpoint before the watchpoint, so
     that we will encounter it first in bpstat_stop_status.  */
  if (exp_valid_block && frame)
    {
      if (frame_id_p (frame_unwind_caller_id (frame)))
	{
 	  scope_breakpoint
	    = create_internal_breakpoint (frame_unwind_caller_arch (frame),
					  frame_unwind_caller_pc (frame),
					  bp_watchpoint_scope);

	  scope_breakpoint->enable_state = bp_enabled;

	  /* Automatically delete the breakpoint when it hits.  */
	  scope_breakpoint->disposition = disp_del;

	  /* Only break in the proper frame (help with recursion).  */
	  scope_breakpoint->frame_id = frame_unwind_caller_id (frame);

	  /* Set the address at which we will stop.  */
	  scope_breakpoint->loc->gdbarch
	    = frame_unwind_caller_arch (frame);
	  scope_breakpoint->loc->requested_address
	    = frame_unwind_caller_pc (frame);
	  scope_breakpoint->loc->address
	    = adjust_breakpoint_address (scope_breakpoint->loc->gdbarch,
					 scope_breakpoint->loc->requested_address,
					 scope_breakpoint->type);
	}
    }

  /* Now set up the breakpoint.  */
  b = set_raw_breakpoint_without_location (NULL, bp_type);
  set_breakpoint_number (internal, b);
  b->thread = thread;
  b->disposition = disp_donttouch;
  b->exp = exp;
  b->exp_valid_block = exp_valid_block;
  b->cond_exp_valid_block = cond_exp_valid_block;
  if (just_location)
    {
      struct type *t = value_type (val);
      CORE_ADDR addr = value_as_address (val);
      char *name;

      t = check_typedef (TYPE_TARGET_TYPE (check_typedef (t)));
      name = type_to_string (t);

      b->exp_string_reparse = xstrprintf ("* (%s *) %s", name,
					  core_addr_to_string (addr));
      xfree (name);

      b->exp_string = xstrprintf ("-location %.*s",
				  (int) (exp_end - exp_start), exp_start);

      /* The above expression is in C.  */
      b->language = language_c;
    }
  else
    b->exp_string = savestring (exp_start, exp_end - exp_start);
  b->val = val;
  b->val_valid = 1;
  b->ops = &watchpoint_breakpoint_ops;

  /* Use an exact watchpoint when there's only one memory region to be
     watched, and only one debug register is needed to watch it.  */
  b->exact = target_exact_watchpoints && reg_cnt == 1;

  if (cond_start)
    b->cond_string = savestring (cond_start, cond_end - cond_start);
  else
    b->cond_string = 0;

  if (frame)
    {
      b->watchpoint_frame = get_frame_id (frame);
      b->watchpoint_thread = inferior_ptid;
    }
  else
    {
      b->watchpoint_frame = null_frame_id;
      b->watchpoint_thread = null_ptid;
    }

  if (scope_breakpoint != NULL)
    {
      /* The scope breakpoint is related to the watchpoint.  We will
	 need to act on them together.  */
      b->related_breakpoint = scope_breakpoint;
      scope_breakpoint->related_breakpoint = b;
    }

  if (!just_location)
    value_free_to_mark (mark);

  /* Finally update the new watchpoint.  This creates the locations
     that should be inserted.  */
  update_watchpoint (b, 1);
  if (internal)
    /* Do not mention breakpoints with a negative number, but do
       notify observers.  */
    observer_notify_breakpoint_created (b->number);
  else
    mention (b);
  update_global_location_list (1);
}

/* Return count of debug registers needed to watch the given expression.
   If EXACT_WATCHPOINTS is 1, then consider that only the address of
   the start of the watched region will be monitored (i.e., all accesses
   will be aligned).  This uses less debug registers on some targets.

   If the watchpoint cannot be handled in hardware return zero.  */

static int
can_use_hardware_watchpoint (struct value *v, int exact_watchpoints)
{
  int found_memory_cnt = 0;
  struct value *head = v;

  /* Did the user specifically forbid us to use hardware watchpoints? */
  if (!can_use_hw_watchpoints)
    return 0;

  /* Make sure that the value of the expression depends only upon
     memory contents, and values computed from them within GDB.  If we
     find any register references or function calls, we can't use a
     hardware watchpoint.

     The idea here is that evaluating an expression generates a series
     of values, one holding the value of every subexpression.  (The
     expression a*b+c has five subexpressions: a, b, a*b, c, and
     a*b+c.)  GDB's values hold almost enough information to establish
     the criteria given above --- they identify memory lvalues,
     register lvalues, computed values, etcetera.  So we can evaluate
     the expression, and then scan the chain of values that leaves
     behind to decide whether we can detect any possible change to the
     expression's final value using only hardware watchpoints.

     However, I don't think that the values returned by inferior
     function calls are special in any way.  So this function may not
     notice that an expression involving an inferior function call
     can't be watched with hardware watchpoints.  FIXME.  */
  for (; v; v = value_next (v))
    {
      if (VALUE_LVAL (v) == lval_memory)
	{
	  if (v != head && value_lazy (v))
	    /* A lazy memory lvalue in the chain is one that GDB never
	       needed to fetch; we either just used its address (e.g.,
	       `a' in `a.b') or we never needed it at all (e.g., `a'
	       in `a,b').  This doesn't apply to HEAD; if that is
	       lazy then it was not readable, but watch it anyway.  */
	    ;
	  else
	    {
	      /* Ahh, memory we actually used!  Check if we can cover
                 it with hardware watchpoints.  */
	      struct type *vtype = check_typedef (value_type (v));

	      /* We only watch structs and arrays if user asked for it
		 explicitly, never if they just happen to appear in a
		 middle of some value chain.  */
	      if (v == head
		  || (TYPE_CODE (vtype) != TYPE_CODE_STRUCT
		      && TYPE_CODE (vtype) != TYPE_CODE_ARRAY))
		{
		  CORE_ADDR vaddr = value_address (v);
		  int len;
		  int num_regs;

		  len = (exact_watchpoints
			 && is_scalar_type_recursive (vtype))?
		    1 : TYPE_LENGTH (value_type (v));

		  num_regs = target_region_ok_for_hw_watchpoint (vaddr, len);
		  if (!num_regs)
		    return 0;
		  else
		    found_memory_cnt += num_regs;
		}
	    }
	}
      else if (VALUE_LVAL (v) != not_lval
	       && deprecated_value_modifiable (v) == 0)
	return 0;	/* These are values from the history (e.g., $1).  */
      else if (VALUE_LVAL (v) == lval_register)
	return 0;	/* Cannot watch a register with a HW watchpoint.  */
    }

  /* The expression itself looks suitable for using a hardware
     watchpoint, but give the target machine a chance to reject it.  */
  return found_memory_cnt;
}

void
watch_command_wrapper (char *arg, int from_tty, int internal)
{
  watch_command_1 (arg, hw_write, from_tty, 0, internal);
}

/* A helper function that looks for an argument at the start of a
   string.  The argument must also either be at the end of the string,
   or be followed by whitespace.  Returns 1 if it finds the argument,
   0 otherwise.  If the argument is found, it updates *STR.  */

static int
check_for_argument (char **str, char *arg, int arg_len)
{
  if (strncmp (*str, arg, arg_len) == 0
      && ((*str)[arg_len] == '\0' || isspace ((*str)[arg_len])))
    {
      *str += arg_len;
      return 1;
    }
  return 0;
}

/* A helper function that looks for the "-location" argument and then
   calls watch_command_1.  */

static void
watch_maybe_just_location (char *arg, int accessflag, int from_tty)
{
  int just_location = 0;

  if (arg
      && (check_for_argument (&arg, "-location", sizeof ("-location") - 1)
	  || check_for_argument (&arg, "-l", sizeof ("-l") - 1)))
    {
      arg = skip_spaces (arg);
      just_location = 1;
    }

  watch_command_1 (arg, accessflag, from_tty, just_location, 0);
}

static void
watch_command (char *arg, int from_tty)
{
  watch_maybe_just_location (arg, hw_write, from_tty);
}

void
rwatch_command_wrapper (char *arg, int from_tty, int internal)
{
  watch_command_1 (arg, hw_read, from_tty, 0, internal);
}

static void
rwatch_command (char *arg, int from_tty)
{
  watch_maybe_just_location (arg, hw_read, from_tty);
}

void
awatch_command_wrapper (char *arg, int from_tty, int internal)
{
  watch_command_1 (arg, hw_access, from_tty, 0, internal);
}

static void
awatch_command (char *arg, int from_tty)
{
  watch_maybe_just_location (arg, hw_access, from_tty);
}


/* Helper routines for the until_command routine in infcmd.c.  Here
   because it uses the mechanisms of breakpoints.  */

struct until_break_command_continuation_args
{
  struct breakpoint *breakpoint;
  struct breakpoint *breakpoint2;
  int thread_num;
};

/* This function is called by fetch_inferior_event via the
   cmd_continuation pointer, to complete the until command.  It takes
   care of cleaning up the temporary breakpoints set up by the until
   command.  */
static void
until_break_command_continuation (void *arg)
{
  struct until_break_command_continuation_args *a = arg;

  delete_breakpoint (a->breakpoint);
  if (a->breakpoint2)
    delete_breakpoint (a->breakpoint2);
  delete_longjmp_breakpoint (a->thread_num);
}

void
until_break_command (char *arg, int from_tty, int anywhere)
{
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  struct frame_info *frame = get_selected_frame (NULL);
  struct breakpoint *breakpoint;
  struct breakpoint *breakpoint2 = NULL;
  struct cleanup *old_chain;
  int thread;
  struct thread_info *tp;

  clear_proceed_status ();

  /* Set a breakpoint where the user wants it and at return from
     this function.  */

  if (default_breakpoint_valid)
    sals = decode_line_1 (&arg, 1, default_breakpoint_symtab,
			  default_breakpoint_line, NULL);
  else
    sals = decode_line_1 (&arg, 1, (struct symtab *) NULL, 0, NULL);

  if (sals.nelts != 1)
    error (_("Couldn't get information on specified line."));

  sal = sals.sals[0];
  xfree (sals.sals);	/* malloc'd, so freed.  */

  if (*arg)
    error (_("Junk at end of arguments."));

  resolve_sal_pc (&sal);

  if (anywhere)
    /* If the user told us to continue until a specified location,
       we don't specify a frame at which we need to stop.  */
    breakpoint = set_momentary_breakpoint (get_frame_arch (frame), sal,
					   null_frame_id, bp_until);
  else
    /* Otherwise, specify the selected frame, because we want to stop
       only at the very same frame.  */
    breakpoint = set_momentary_breakpoint (get_frame_arch (frame), sal,
					   get_stack_frame_id (frame),
					   bp_until);

  old_chain = make_cleanup_delete_breakpoint (breakpoint);

  tp = inferior_thread ();
  thread = tp->num;

  /* Keep within the current frame, or in frames called by the current
     one.  */

  if (frame_id_p (frame_unwind_caller_id (frame)))
    {
      sal = find_pc_line (frame_unwind_caller_pc (frame), 0);
      sal.pc = frame_unwind_caller_pc (frame);
      breakpoint2 = set_momentary_breakpoint (frame_unwind_caller_arch (frame),
					      sal,
					      frame_unwind_caller_id (frame),
					      bp_until);
      make_cleanup_delete_breakpoint (breakpoint2);

      set_longjmp_breakpoint (tp, frame_unwind_caller_id (frame));
      make_cleanup (delete_longjmp_breakpoint_cleanup, &thread);
    }

  proceed (-1, TARGET_SIGNAL_DEFAULT, 0);

  /* If we are running asynchronously, and proceed call above has
     actually managed to start the target, arrange for breakpoints to
     be deleted when the target stops.  Otherwise, we're already
     stopped and delete breakpoints via cleanup chain.  */

  if (target_can_async_p () && is_running (inferior_ptid))
    {
      struct until_break_command_continuation_args *args;
      args = xmalloc (sizeof (*args));

      args->breakpoint = breakpoint;
      args->breakpoint2 = breakpoint2;
      args->thread_num = thread;

      discard_cleanups (old_chain);
      add_continuation (inferior_thread (),
			until_break_command_continuation, args,
			xfree);
    }
  else
    do_cleanups (old_chain);
}

/* This function attempts to parse an optional "if <cond>" clause
   from the arg string.  If one is not found, it returns NULL.

   Else, it returns a pointer to the condition string.  (It does not
   attempt to evaluate the string against a particular block.)  And,
   it updates arg to point to the first character following the parsed
   if clause in the arg string.  */

static char *
ep_parse_optional_if_clause (char **arg)
{
  char *cond_string;

  if (((*arg)[0] != 'i') || ((*arg)[1] != 'f') || !isspace ((*arg)[2]))
    return NULL;

  /* Skip the "if" keyword.  */
  (*arg) += 2;

  /* Skip any extra leading whitespace, and record the start of the
     condition string.  */
  *arg = skip_spaces (*arg);
  cond_string = *arg;

  /* Assume that the condition occupies the remainder of the arg
     string.  */
  (*arg) += strlen (cond_string);

  return cond_string;
}

/* Commands to deal with catching events, such as signals, exceptions,
   process start/exit, etc.  */

typedef enum
{
  catch_fork_temporary, catch_vfork_temporary,
  catch_fork_permanent, catch_vfork_permanent
}
catch_fork_kind;

static void
catch_fork_command_1 (char *arg, int from_tty, 
		      struct cmd_list_element *command)
{
  struct gdbarch *gdbarch = get_current_arch ();
  char *cond_string = NULL;
  catch_fork_kind fork_kind;
  int tempflag;

  fork_kind = (catch_fork_kind) (uintptr_t) get_cmd_context (command);
  tempflag = (fork_kind == catch_fork_temporary
	      || fork_kind == catch_vfork_temporary);

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);

  /* The allowed syntax is:
     catch [v]fork
     catch [v]fork if <cond>

     First, check if there's an if clause.  */
  cond_string = ep_parse_optional_if_clause (&arg);

  if ((*arg != '\0') && !isspace (*arg))
    error (_("Junk at end of arguments."));

  /* If this target supports it, create a fork or vfork catchpoint
     and enable reporting of such events.  */
  switch (fork_kind)
    {
    case catch_fork_temporary:
    case catch_fork_permanent:
      create_fork_vfork_event_catchpoint (gdbarch, tempflag, cond_string,
                                          &catch_fork_breakpoint_ops);
      break;
    case catch_vfork_temporary:
    case catch_vfork_permanent:
      create_fork_vfork_event_catchpoint (gdbarch, tempflag, cond_string,
                                          &catch_vfork_breakpoint_ops);
      break;
    default:
      error (_("unsupported or unknown fork kind; cannot catch it"));
      break;
    }
}

static void
catch_exec_command_1 (char *arg, int from_tty, 
		      struct cmd_list_element *command)
{
  struct gdbarch *gdbarch = get_current_arch ();
  int tempflag;
  char *cond_string = NULL;

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);

  /* The allowed syntax is:
     catch exec
     catch exec if <cond>

     First, check if there's an if clause.  */
  cond_string = ep_parse_optional_if_clause (&arg);

  if ((*arg != '\0') && !isspace (*arg))
    error (_("Junk at end of arguments."));

  /* If this target supports it, create an exec catchpoint
     and enable reporting of such events.  */
  create_catchpoint (gdbarch, tempflag, cond_string,
		     &catch_exec_breakpoint_ops);
}

static enum print_stop_action
print_exception_catchpoint (struct breakpoint *b)
{
  int bp_temp, bp_throw;

  annotate_catchpoint (b->number);

  bp_throw = strstr (b->addr_string, "throw") != NULL;
  if (b->loc->address != b->loc->requested_address)
    breakpoint_adjustment_warning (b->loc->requested_address,
	                           b->loc->address,
				   b->number, 1);
  bp_temp = b->disposition == disp_del;
  ui_out_text (uiout, 
	       bp_temp ? "Temporary catchpoint "
		       : "Catchpoint ");
  if (!ui_out_is_mi_like_p (uiout))
    ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout,
	       bp_throw ? " (exception thrown), "
		        : " (exception caught), ");
  if (ui_out_is_mi_like_p (uiout))
    {
      ui_out_field_string (uiout, "reason", 
			   async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
      ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
      ui_out_field_int (uiout, "bkptno", b->number);
    }
  return PRINT_SRC_AND_LOC;
}

static void
print_one_exception_catchpoint (struct breakpoint *b, 
				struct bp_location **last_loc)
{
  struct value_print_options opts;

  get_user_print_options (&opts);
  if (opts.addressprint)
    {
      annotate_field (4);
      if (b->loc == NULL || b->loc->shlib_disabled)
	ui_out_field_string (uiout, "addr", "<PENDING>");
      else
	ui_out_field_core_addr (uiout, "addr",
				b->loc->gdbarch, b->loc->address);
    }
  annotate_field (5);
  if (b->loc)
    *last_loc = b->loc;
  if (strstr (b->addr_string, "throw") != NULL)
    ui_out_field_string (uiout, "what", "exception throw");
  else
    ui_out_field_string (uiout, "what", "exception catch");
}

static void
print_mention_exception_catchpoint (struct breakpoint *b)
{
  int bp_temp;
  int bp_throw;

  bp_temp = b->disposition == disp_del;
  bp_throw = strstr (b->addr_string, "throw") != NULL;
  ui_out_text (uiout, bp_temp ? _("Temporary catchpoint ")
			      : _("Catchpoint "));
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, bp_throw ? _(" (throw)")
			       : _(" (catch)"));
}

/* Implement the "print_recreate" breakpoint_ops method for throw and
   catch catchpoints.  */

static void
print_recreate_exception_catchpoint (struct breakpoint *b, 
				     struct ui_file *fp)
{
  int bp_temp;
  int bp_throw;

  bp_temp = b->disposition == disp_del;
  bp_throw = strstr (b->addr_string, "throw") != NULL;
  fprintf_unfiltered (fp, bp_temp ? "tcatch " : "catch ");
  fprintf_unfiltered (fp, bp_throw ? "throw" : "catch");
}

static struct breakpoint_ops gnu_v3_exception_catchpoint_ops = {
  NULL, /* insert */
  NULL, /* remove */
  NULL, /* breakpoint_hit */
  NULL, /* resources_needed */
  print_exception_catchpoint,
  print_one_exception_catchpoint,
  NULL, /* print_one_detail */
  print_mention_exception_catchpoint,
  print_recreate_exception_catchpoint
};

static int
handle_gnu_v3_exceptions (int tempflag, char *cond_string,
			  enum exception_event_kind ex_event, int from_tty)
{
  char *trigger_func_name;
 
  if (ex_event == EX_EVENT_CATCH)
    trigger_func_name = "__cxa_begin_catch";
  else
    trigger_func_name = "__cxa_throw";

  create_breakpoint (get_current_arch (),
		     trigger_func_name, cond_string, -1,
		     0 /* condition and thread are valid.  */,
		     tempflag, bp_breakpoint,
		     0,
		     AUTO_BOOLEAN_TRUE /* pending */,
		     &gnu_v3_exception_catchpoint_ops, from_tty,
		     1 /* enabled */,
		     0 /* internal */);

  return 1;
}

/* Deal with "catch catch" and "catch throw" commands.  */

static void
catch_exception_command_1 (enum exception_event_kind ex_event, char *arg,
			   int tempflag, int from_tty)
{
  char *cond_string = NULL;

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);

  cond_string = ep_parse_optional_if_clause (&arg);

  if ((*arg != '\0') && !isspace (*arg))
    error (_("Junk at end of arguments."));

  if (ex_event != EX_EVENT_THROW
      && ex_event != EX_EVENT_CATCH)
    error (_("Unsupported or unknown exception event; cannot catch it"));

  if (handle_gnu_v3_exceptions (tempflag, cond_string, ex_event, from_tty))
    return;

  warning (_("Unsupported with this platform/compiler combination."));
}

/* Implementation of "catch catch" command.  */

static void
catch_catch_command (char *arg, int from_tty, struct cmd_list_element *command)
{
  int tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  catch_exception_command_1 (EX_EVENT_CATCH, arg, tempflag, from_tty);
}

/* Implementation of "catch throw" command.  */

static void
catch_throw_command (char *arg, int from_tty, struct cmd_list_element *command)
{
  int tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  catch_exception_command_1 (EX_EVENT_THROW, arg, tempflag, from_tty);
}

/* Create a breakpoint struct for Ada exception catchpoints.  */

static void
create_ada_exception_breakpoint (struct gdbarch *gdbarch,
				 struct symtab_and_line sal,
                                 char *addr_string,
                                 char *exp_string,
                                 char *cond_string,
                                 struct expression *cond,
                                 struct breakpoint_ops *ops,
                                 int tempflag,
                                 int from_tty)
{
  struct breakpoint *b;

  if (from_tty)
    {
      struct gdbarch *loc_gdbarch = get_sal_arch (sal);
      if (!loc_gdbarch)
	loc_gdbarch = gdbarch;

      describe_other_breakpoints (loc_gdbarch,
				  sal.pspace, sal.pc, sal.section, -1);
      /* FIXME: brobecker/2006-12-28: Actually, re-implement a special
         version for exception catchpoints, because two catchpoints
         used for different exception names will use the same address.
         In this case, a "breakpoint ... also set at..." warning is
         unproductive.  Besides, the warning phrasing is also a bit
         inapropriate, we should use the word catchpoint, and tell
         the user what type of catchpoint it is.  The above is good
         enough for now, though.  */
    }

  b = set_raw_breakpoint (gdbarch, sal, bp_breakpoint);
  set_breakpoint_count (breakpoint_count + 1);

  b->enable_state = bp_enabled;
  b->disposition = tempflag ? disp_del : disp_donttouch;
  b->number = breakpoint_count;
  b->ignore_count = 0;
  b->loc->cond = cond;
  b->addr_string = addr_string;
  b->language = language_ada;
  b->cond_string = cond_string;
  b->exp_string = exp_string;
  b->thread = -1;
  b->ops = ops;

  mention (b);
  update_global_location_list (1);
}

/* Implement the "catch exception" command.  */

static void
catch_ada_exception_command (char *arg, int from_tty,
			     struct cmd_list_element *command)
{
  struct gdbarch *gdbarch = get_current_arch ();
  int tempflag;
  struct symtab_and_line sal;
  char *addr_string = NULL;
  char *exp_string = NULL;
  char *cond_string = NULL;
  struct expression *cond = NULL;
  struct breakpoint_ops *ops = NULL;

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  sal = ada_decode_exception_location (arg, &addr_string, &exp_string,
                                       &cond_string, &cond, &ops);
  create_ada_exception_breakpoint (gdbarch, sal, addr_string, exp_string,
                                   cond_string, cond, ops, tempflag,
                                   from_tty);
}

/* Cleanup function for a syscall filter list.  */
static void
clean_up_filters (void *arg)
{
  VEC(int) *iter = *(VEC(int) **) arg;
  VEC_free (int, iter);
}

/* Splits the argument using space as delimiter.  Returns an xmalloc'd
   filter list, or NULL if no filtering is required.  */
static VEC(int) *
catch_syscall_split_args (char *arg)
{
  VEC(int) *result = NULL;
  struct cleanup *cleanup = make_cleanup (clean_up_filters, &result);

  while (*arg != '\0')
    {
      int i, syscall_number;
      char *endptr;
      char cur_name[128];
      struct syscall s;

      /* Skip whitespace.  */
      while (isspace (*arg))
	arg++;

      for (i = 0; i < 127 && arg[i] && !isspace (arg[i]); ++i)
	cur_name[i] = arg[i];
      cur_name[i] = '\0';
      arg += i;

      /* Check if the user provided a syscall name or a number.  */
      syscall_number = (int) strtol (cur_name, &endptr, 0);
      if (*endptr == '\0')
	get_syscall_by_number (syscall_number, &s);
      else
	{
	  /* We have a name.  Let's check if it's valid and convert it
	     to a number.  */
	  get_syscall_by_name (cur_name, &s);

	  if (s.number == UNKNOWN_SYSCALL)
	    /* Here we have to issue an error instead of a warning,
	       because GDB cannot do anything useful if there's no
	       syscall number to be caught.  */
	    error (_("Unknown syscall name '%s'."), cur_name);
	}

      /* Ok, it's valid.  */
      VEC_safe_push (int, result, s.number);
    }

  discard_cleanups (cleanup);
  return result;
}

/* Implement the "catch syscall" command.  */

static void
catch_syscall_command_1 (char *arg, int from_tty, 
			 struct cmd_list_element *command)
{
  int tempflag;
  VEC(int) *filter;
  struct syscall s;
  struct gdbarch *gdbarch = get_current_arch ();

  /* Checking if the feature if supported.  */
  if (gdbarch_get_syscall_number_p (gdbarch) == 0)
    error (_("The feature 'catch syscall' is not supported on \
this architecture yet."));

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  arg = skip_spaces (arg);

  /* We need to do this first "dummy" translation in order
     to get the syscall XML file loaded or, most important,
     to display a warning to the user if there's no XML file
     for his/her architecture.  */
  get_syscall_by_number (0, &s);

  /* The allowed syntax is:
     catch syscall
     catch syscall <name | number> [<name | number> ... <name | number>]

     Let's check if there's a syscall name.  */

  if (arg != NULL)
    filter = catch_syscall_split_args (arg);
  else
    filter = NULL;

  create_syscall_event_catchpoint (tempflag, filter,
				   &catch_syscall_breakpoint_ops);
}

/* Implement the "catch assert" command.  */

static void
catch_assert_command (char *arg, int from_tty,
		      struct cmd_list_element *command)
{
  struct gdbarch *gdbarch = get_current_arch ();
  int tempflag;
  struct symtab_and_line sal;
  char *addr_string = NULL;
  struct breakpoint_ops *ops = NULL;

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  sal = ada_decode_assert_location (arg, &addr_string, &ops);
  create_ada_exception_breakpoint (gdbarch, sal, addr_string, NULL, NULL, NULL,
				   ops, tempflag, from_tty);
}

static void
catch_command (char *arg, int from_tty)
{
  error (_("Catch requires an event name."));
}


static void
tcatch_command (char *arg, int from_tty)
{
  error (_("Catch requires an event name."));
}

/* Delete breakpoints by address or line.  */

static void
clear_command (char *arg, int from_tty)
{
  struct breakpoint *b;
  VEC(breakpoint_p) *found = 0;
  int ix;
  int default_match;
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  int i;

  if (arg)
    {
      sals = decode_line_spec (arg, 1);
      default_match = 0;
    }
  else
    {
      sals.sals = (struct symtab_and_line *)
	xmalloc (sizeof (struct symtab_and_line));
      make_cleanup (xfree, sals.sals);
      init_sal (&sal);		/* Initialize to zeroes.  */
      sal.line = default_breakpoint_line;
      sal.symtab = default_breakpoint_symtab;
      sal.pc = default_breakpoint_address;
      sal.pspace = default_breakpoint_pspace;
      if (sal.symtab == 0)
	error (_("No source file specified."));

      sals.sals[0] = sal;
      sals.nelts = 1;

      default_match = 1;
    }

  /* We don't call resolve_sal_pc here.  That's not as bad as it
     seems, because all existing breakpoints typically have both
     file/line and pc set.  So, if clear is given file/line, we can
     match this to existing breakpoint without obtaining pc at all.

     We only support clearing given the address explicitly 
     present in breakpoint table.  Say, we've set breakpoint 
     at file:line.  There were several PC values for that file:line,
     due to optimization, all in one block.

     We've picked one PC value.  If "clear" is issued with another
     PC corresponding to the same file:line, the breakpoint won't
     be cleared.  We probably can still clear the breakpoint, but 
     since the other PC value is never presented to user, user
     can only find it by guessing, and it does not seem important
     to support that.  */

  /* For each line spec given, delete bps which correspond to it.  Do
     it in two passes, solely to preserve the current behavior that
     from_tty is forced true if we delete more than one
     breakpoint.  */

  found = NULL;
  for (i = 0; i < sals.nelts; i++)
    {
      /* If exact pc given, clear bpts at that pc.
         If line given (pc == 0), clear all bpts on specified line.
         If defaulting, clear all bpts on default line
         or at default pc.

         defaulting    sal.pc != 0    tests to do

         0              1             pc
         1              1             pc _and_ line
         0              0             line
         1              0             <can't happen> */

      sal = sals.sals[i];

      /* Find all matching breakpoints and add them to 'found'.  */
      ALL_BREAKPOINTS (b)
	{
	  int match = 0;
	  /* Are we going to delete b?  */
	  if (b->type != bp_none && !is_watchpoint (b))
	    {
	      struct bp_location *loc = b->loc;
	      for (; loc; loc = loc->next)
		{
		  int pc_match = sal.pc
		    && (loc->pspace == sal.pspace)
		    && (loc->address == sal.pc)
		    && (!section_is_overlay (loc->section)
			|| loc->section == sal.section);
		  int line_match = ((default_match || (0 == sal.pc))
				    && b->source_file != NULL
				    && sal.symtab != NULL
				    && sal.pspace == loc->pspace
				    && filename_cmp (b->source_file,
						     sal.symtab->filename) == 0
				    && b->line_number == sal.line);
		  if (pc_match || line_match)
		    {
		      match = 1;
		      break;
		    }
		}
	    }

	  if (match)
	    VEC_safe_push(breakpoint_p, found, b);
	}
    }
  /* Now go thru the 'found' chain and delete them.  */
  if (VEC_empty(breakpoint_p, found))
    {
      if (arg)
	error (_("No breakpoint at %s."), arg);
      else
	error (_("No breakpoint at this line."));
    }

  if (VEC_length(breakpoint_p, found) > 1)
    from_tty = 1;	/* Always report if deleted more than one.  */
  if (from_tty)
    {
      if (VEC_length(breakpoint_p, found) == 1)
	printf_unfiltered (_("Deleted breakpoint "));
      else
	printf_unfiltered (_("Deleted breakpoints "));
    }
  breakpoints_changed ();

  for (ix = 0; VEC_iterate(breakpoint_p, found, ix, b); ix++)
    {
      if (from_tty)
	printf_unfiltered ("%d ", b->number);
      delete_breakpoint (b);
    }
  if (from_tty)
    putchar_unfiltered ('\n');
}

/* Delete breakpoint in BS if they are `delete' breakpoints and
   all breakpoints that are marked for deletion, whether hit or not.
   This is called after any breakpoint is hit, or after errors.  */

void
breakpoint_auto_delete (bpstat bs)
{
  struct breakpoint *b, *b_tmp;

  for (; bs; bs = bs->next)
    if (bs->breakpoint_at
	&& bs->breakpoint_at->disposition == disp_del
	&& bs->stop)
      delete_breakpoint (bs->breakpoint_at);

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
  {
    if (b->disposition == disp_del_at_next_stop)
      delete_breakpoint (b);
  }
}

/* A comparison function for bp_location AP and BP being interfaced to
   qsort.  Sort elements primarily by their ADDRESS (no matter what
   does breakpoint_address_is_meaningful say for its OWNER),
   secondarily by ordering first bp_permanent OWNERed elements and
   terciarily just ensuring the array is sorted stable way despite
   qsort being an instable algorithm.  */

static int
bp_location_compare (const void *ap, const void *bp)
{
  struct bp_location *a = *(void **) ap;
  struct bp_location *b = *(void **) bp;
  /* A and B come from existing breakpoints having non-NULL OWNER.  */
  int a_perm = a->owner->enable_state == bp_permanent;
  int b_perm = b->owner->enable_state == bp_permanent;

  if (a->address != b->address)
    return (a->address > b->address) - (a->address < b->address);

  /* Sort permanent breakpoints first.  */
  if (a_perm != b_perm)
    return (a_perm < b_perm) - (a_perm > b_perm);

  /* Make the user-visible order stable across GDB runs.  Locations of
     the same breakpoint can be sorted in arbitrary order.  */

  if (a->owner->number != b->owner->number)
    return (a->owner->number > b->owner->number)
           - (a->owner->number < b->owner->number);

  return (a > b) - (a < b);
}

/* Set bp_location_placed_address_before_address_max and
   bp_location_shadow_len_after_address_max according to the current
   content of the bp_location array.  */

static void
bp_location_target_extensions_update (void)
{
  struct bp_location *bl, **blp_tmp;

  bp_location_placed_address_before_address_max = 0;
  bp_location_shadow_len_after_address_max = 0;

  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      CORE_ADDR start, end, addr;

      if (!bp_location_has_shadow (bl))
	continue;

      start = bl->target_info.placed_address;
      end = start + bl->target_info.shadow_len;

      gdb_assert (bl->address >= start);
      addr = bl->address - start;
      if (addr > bp_location_placed_address_before_address_max)
	bp_location_placed_address_before_address_max = addr;

      /* Zero SHADOW_LEN would not pass bp_location_has_shadow.  */

      gdb_assert (bl->address < end);
      addr = end - bl->address;
      if (addr > bp_location_shadow_len_after_address_max)
	bp_location_shadow_len_after_address_max = addr;
    }
}

/* If SHOULD_INSERT is false, do not insert any breakpoint locations
   into the inferior, only remove already-inserted locations that no
   longer should be inserted.  Functions that delete a breakpoint or
   breakpoints should pass false, so that deleting a breakpoint
   doesn't have the side effect of inserting the locations of other
   breakpoints that are marked not-inserted, but should_be_inserted
   returns true on them.

   This behaviour is useful is situations close to tear-down -- e.g.,
   after an exec, while the target still has execution, but breakpoint
   shadows of the previous executable image should *NOT* be restored
   to the new image; or before detaching, where the target still has
   execution and wants to delete breakpoints from GDB's lists, and all
   breakpoints had already been removed from the inferior.  */

static void
update_global_location_list (int should_insert)
{
  struct breakpoint *b;
  struct bp_location **locp, *loc;
  struct cleanup *cleanups;

  /* Used in the duplicates detection below.  When iterating over all
     bp_locations, points to the first bp_location of a given address.
     Breakpoints and watchpoints of different types are never
     duplicates of each other.  Keep one pointer for each type of
     breakpoint/watchpoint, so we only need to loop over all locations
     once.  */
  struct bp_location *bp_loc_first;  /* breakpoint */
  struct bp_location *wp_loc_first;  /* hardware watchpoint */
  struct bp_location *awp_loc_first; /* access watchpoint */
  struct bp_location *rwp_loc_first; /* read watchpoint */

  /* Saved former bp_location array which we compare against the newly
     built bp_location from the current state of ALL_BREAKPOINTS.  */
  struct bp_location **old_location, **old_locp;
  unsigned old_location_count;

  old_location = bp_location;
  old_location_count = bp_location_count;
  bp_location = NULL;
  bp_location_count = 0;
  cleanups = make_cleanup (xfree, old_location);

  ALL_BREAKPOINTS (b)
    for (loc = b->loc; loc; loc = loc->next)
      bp_location_count++;

  bp_location = xmalloc (sizeof (*bp_location) * bp_location_count);
  locp = bp_location;
  ALL_BREAKPOINTS (b)
    for (loc = b->loc; loc; loc = loc->next)
      *locp++ = loc;
  qsort (bp_location, bp_location_count, sizeof (*bp_location),
	 bp_location_compare);

  bp_location_target_extensions_update ();

  /* Identify bp_location instances that are no longer present in the
     new list, and therefore should be freed.  Note that it's not
     necessary that those locations should be removed from inferior --
     if there's another location at the same address (previously
     marked as duplicate), we don't need to remove/insert the
     location.
     
     LOCP is kept in sync with OLD_LOCP, each pointing to the current
     and former bp_location array state respectively.  */

  locp = bp_location;
  for (old_locp = old_location; old_locp < old_location + old_location_count;
       old_locp++)
    {
      struct bp_location *old_loc = *old_locp;
      struct bp_location **loc2p;

      /* Tells if 'old_loc' is found amoung the new locations.  If
	 not, we have to free it.  */
      int found_object = 0;
      /* Tells if the location should remain inserted in the target.  */
      int keep_in_target = 0;
      int removed = 0;

      /* Skip LOCP entries which will definitely never be needed.
	 Stop either at or being the one matching OLD_LOC.  */
      while (locp < bp_location + bp_location_count
	     && (*locp)->address < old_loc->address)
	locp++;

      for (loc2p = locp;
	   (loc2p < bp_location + bp_location_count
	    && (*loc2p)->address == old_loc->address);
	   loc2p++)
	{
	  if (*loc2p == old_loc)
	    {
	      found_object = 1;
	      break;
	    }
	}

      /* If this location is no longer present, and inserted, look if
	 there's maybe a new location at the same address.  If so,
	 mark that one inserted, and don't remove this one.  This is
	 needed so that we don't have a time window where a breakpoint
	 at certain location is not inserted.  */

      if (old_loc->inserted)
	{
	  /* If the location is inserted now, we might have to remove
	     it.  */

	  if (found_object && should_be_inserted (old_loc))
	    {
	      /* The location is still present in the location list,
		 and still should be inserted.  Don't do anything.  */
	      keep_in_target = 1;
	    }
	  else
	    {
	      /* The location is either no longer present, or got
		 disabled.  See if there's another location at the
		 same address, in which case we don't need to remove
		 this one from the target.  */

	      /* OLD_LOC comes from existing struct breakpoint.  */
	      if (breakpoint_address_is_meaningful (old_loc->owner))
		{
		  for (loc2p = locp;
		       (loc2p < bp_location + bp_location_count
			&& (*loc2p)->address == old_loc->address);
		       loc2p++)
		    {
		      struct bp_location *loc2 = *loc2p;

		      if (breakpoint_locations_match (loc2, old_loc))
			{
			  /* For the sake of should_be_inserted.
			     Duplicates check below will fix up this
			     later.  */
			  loc2->duplicate = 0;

			  /* Read watchpoint locations are switched to
			     access watchpoints, if the former are not
			     supported, but the latter are.  */
			  if (is_hardware_watchpoint (old_loc->owner))
			    {
			      gdb_assert (is_hardware_watchpoint (loc2->owner));
			      loc2->watchpoint_type = old_loc->watchpoint_type;
			    }

			  if (loc2 != old_loc && should_be_inserted (loc2))
			    {
			      loc2->inserted = 1;
			      loc2->target_info = old_loc->target_info;
			      keep_in_target = 1;
			      break;
			    }
			}
		    }
		}
	    }

	  if (!keep_in_target)
	    {
	      if (remove_breakpoint (old_loc, mark_uninserted))
		{
		  /* This is just about all we can do.  We could keep
		     this location on the global list, and try to
		     remove it next time, but there's no particular
		     reason why we will succeed next time.
		     
		     Note that at this point, old_loc->owner is still
		     valid, as delete_breakpoint frees the breakpoint
		     only after calling us.  */
		  printf_filtered (_("warning: Error removing "
				     "breakpoint %d\n"), 
				   old_loc->owner->number);
		}
	      removed = 1;
	    }
	}

      if (!found_object)
	{
	  if (removed && non_stop
	      && breakpoint_address_is_meaningful (old_loc->owner)
	      && !is_hardware_watchpoint (old_loc->owner))
	    {
	      /* This location was removed from the target.  In
		 non-stop mode, a race condition is possible where
		 we've removed a breakpoint, but stop events for that
		 breakpoint are already queued and will arrive later.
		 We apply an heuristic to be able to distinguish such
		 SIGTRAPs from other random SIGTRAPs: we keep this
		 breakpoint location for a bit, and will retire it
		 after we see some number of events.  The theory here
		 is that reporting of events should, "on the average",
		 be fair, so after a while we'll see events from all
		 threads that have anything of interest, and no longer
		 need to keep this breakpoint location around.  We
		 don't hold locations forever so to reduce chances of
		 mistaking a non-breakpoint SIGTRAP for a breakpoint
		 SIGTRAP.

		 The heuristic failing can be disastrous on
		 decr_pc_after_break targets.

		 On decr_pc_after_break targets, like e.g., x86-linux,
		 if we fail to recognize a late breakpoint SIGTRAP,
		 because events_till_retirement has reached 0 too
		 soon, we'll fail to do the PC adjustment, and report
		 a random SIGTRAP to the user.  When the user resumes
		 the inferior, it will most likely immediately crash
		 with SIGILL/SIGBUS/SIGSEGV, or worse, get silently
		 corrupted, because of being resumed e.g., in the
		 middle of a multi-byte instruction, or skipped a
		 one-byte instruction.  This was actually seen happen
		 on native x86-linux, and should be less rare on
		 targets that do not support new thread events, like
		 remote, due to the heuristic depending on
		 thread_count.

		 Mistaking a random SIGTRAP for a breakpoint trap
		 causes similar symptoms (PC adjustment applied when
		 it shouldn't), but then again, playing with SIGTRAPs
		 behind the debugger's back is asking for trouble.

		 Since hardware watchpoint traps are always
		 distinguishable from other traps, so we don't need to
		 apply keep hardware watchpoint moribund locations
		 around.  We simply always ignore hardware watchpoint
		 traps we can no longer explain.  */

	      old_loc->events_till_retirement = 3 * (thread_count () + 1);
	      old_loc->owner = NULL;

	      VEC_safe_push (bp_location_p, moribund_locations, old_loc);
	    }
	  else
	    {
	      old_loc->owner = NULL;
	      decref_bp_location (&old_loc);
	    }
	}
    }

  /* Rescan breakpoints at the same address and section, marking the
     first one as "first" and any others as "duplicates".  This is so
     that the bpt instruction is only inserted once.  If we have a
     permanent breakpoint at the same place as BPT, make that one the
     official one, and the rest as duplicates.  Permanent breakpoints
     are sorted first for the same address.

     Do the same for hardware watchpoints, but also considering the
     watchpoint's type (regular/access/read) and length.  */

  bp_loc_first = NULL;
  wp_loc_first = NULL;
  awp_loc_first = NULL;
  rwp_loc_first = NULL;
  ALL_BP_LOCATIONS (loc, locp)
    {
      /* ALL_BP_LOCATIONS bp_location has LOC->OWNER always
	 non-NULL.  */
      struct breakpoint *b = loc->owner;
      struct bp_location **loc_first_p;

      if (b->enable_state == bp_disabled
	  || b->enable_state == bp_call_disabled
	  || b->enable_state == bp_startup_disabled
	  || !loc->enabled
	  || loc->shlib_disabled
	  || !breakpoint_address_is_meaningful (b)
	  || is_tracepoint (b))
	continue;

      /* Permanent breakpoint should always be inserted.  */
      if (b->enable_state == bp_permanent && ! loc->inserted)
	internal_error (__FILE__, __LINE__,
			_("allegedly permanent breakpoint is not "
			"actually inserted"));

      if (b->type == bp_hardware_watchpoint)
	loc_first_p = &wp_loc_first;
      else if (b->type == bp_read_watchpoint)
	loc_first_p = &rwp_loc_first;
      else if (b->type == bp_access_watchpoint)
	loc_first_p = &awp_loc_first;
      else
	loc_first_p = &bp_loc_first;

      if (*loc_first_p == NULL
	  || (overlay_debugging && loc->section != (*loc_first_p)->section)
	  || !breakpoint_locations_match (loc, *loc_first_p))
	{
	  *loc_first_p = loc;
	  loc->duplicate = 0;
	  continue;
	}

      loc->duplicate = 1;

      if ((*loc_first_p)->owner->enable_state == bp_permanent && loc->inserted
	  && b->enable_state != bp_permanent)
	internal_error (__FILE__, __LINE__,
			_("another breakpoint was inserted on top of "
			"a permanent breakpoint"));
    }

  if (breakpoints_always_inserted_mode () && should_insert
      && (have_live_inferiors ()
	  || (gdbarch_has_global_breakpoints (target_gdbarch))))
    insert_breakpoint_locations ();

  do_cleanups (cleanups);
}

void
breakpoint_retire_moribund (void)
{
  struct bp_location *loc;
  int ix;

  for (ix = 0; VEC_iterate (bp_location_p, moribund_locations, ix, loc); ++ix)
    if (--(loc->events_till_retirement) == 0)
      {
	decref_bp_location (&loc);
	VEC_unordered_remove (bp_location_p, moribund_locations, ix);
	--ix;
      }
}

static void
update_global_location_list_nothrow (int inserting)
{
  struct gdb_exception e;

  TRY_CATCH (e, RETURN_MASK_ERROR)
    update_global_location_list (inserting);
}

/* Clear BKP from a BPS.  */

static void
bpstat_remove_bp_location (bpstat bps, struct breakpoint *bpt)
{
  bpstat bs;

  for (bs = bps; bs; bs = bs->next)
    if (bs->breakpoint_at == bpt)
      {
	bs->breakpoint_at = NULL;
	bs->old_val = NULL;
	/* bs->commands will be freed later.  */
      }
}

/* Callback for iterate_over_threads.  */
static int
bpstat_remove_breakpoint_callback (struct thread_info *th, void *data)
{
  struct breakpoint *bpt = data;

  bpstat_remove_bp_location (th->control.stop_bpstat, bpt);
  return 0;
}

/* Delete a breakpoint and clean up all traces of it in the data
   structures.  */

void
delete_breakpoint (struct breakpoint *bpt)
{
  struct breakpoint *b;

  gdb_assert (bpt != NULL);

  /* Has this bp already been deleted?  This can happen because
     multiple lists can hold pointers to bp's.  bpstat lists are
     especial culprits.

     One example of this happening is a watchpoint's scope bp.  When
     the scope bp triggers, we notice that the watchpoint is out of
     scope, and delete it.  We also delete its scope bp.  But the
     scope bp is marked "auto-deleting", and is already on a bpstat.
     That bpstat is then checked for auto-deleting bp's, which are
     deleted.

     A real solution to this problem might involve reference counts in
     bp's, and/or giving them pointers back to their referencing
     bpstat's, and teaching delete_breakpoint to only free a bp's
     storage when no more references were extent.  A cheaper bandaid
     was chosen.  */
  if (bpt->type == bp_none)
    return;

  /* At least avoid this stale reference until the reference counting
     of breakpoints gets resolved.  */
  if (bpt->related_breakpoint != bpt)
    {
      struct breakpoint *related;

      if (bpt->type == bp_watchpoint_scope)
	watchpoint_del_at_next_stop (bpt->related_breakpoint);
      else if (bpt->related_breakpoint->type == bp_watchpoint_scope)
	watchpoint_del_at_next_stop (bpt);

      /* Unlink bpt from the bpt->related_breakpoint ring.  */
      for (related = bpt; related->related_breakpoint != bpt;
	   related = related->related_breakpoint);
      related->related_breakpoint = bpt->related_breakpoint;
      bpt->related_breakpoint = bpt;
    }

  observer_notify_breakpoint_deleted (bpt->number);

  if (breakpoint_chain == bpt)
    breakpoint_chain = bpt->next;

  ALL_BREAKPOINTS (b)
    if (b->next == bpt)
    {
      b->next = bpt->next;
      break;
    }

  decref_counted_command_line (&bpt->commands);
  xfree (bpt->cond_string);
  xfree (bpt->cond_exp);
  xfree (bpt->addr_string);
  xfree (bpt->addr_string_range_end);
  xfree (bpt->exp);
  xfree (bpt->exp_string);
  xfree (bpt->exp_string_reparse);
  value_free (bpt->val);
  xfree (bpt->source_file);
  xfree (bpt->exec_pathname);
  clean_up_filters (&bpt->syscalls_to_be_caught);


  /* Be sure no bpstat's are pointing at the breakpoint after it's
     been freed.  */
  /* FIXME, how can we find all bpstat's?  We just check stop_bpstat
     in all threeds for now.  Note that we cannot just remove bpstats
     pointing at bpt from the stop_bpstat list entirely, as breakpoint
     commands are associated with the bpstat; if we remove it here,
     then the later call to bpstat_do_actions (&stop_bpstat); in
     event-top.c won't do anything, and temporary breakpoints with
     commands won't work.  */

  iterate_over_threads (bpstat_remove_breakpoint_callback, bpt);

  /* Now that breakpoint is removed from breakpoint list, update the
     global location list.  This will remove locations that used to
     belong to this breakpoint.  Do this before freeing the breakpoint
     itself, since remove_breakpoint looks at location's owner.  It
     might be better design to have location completely
     self-contained, but it's not the case now.  */
  update_global_location_list (0);


  /* On the chance that someone will soon try again to delete this
     same bp, we mark it as deleted before freeing its storage.  */
  bpt->type = bp_none;

  xfree (bpt);
}

static void
do_delete_breakpoint_cleanup (void *b)
{
  delete_breakpoint (b);
}

struct cleanup *
make_cleanup_delete_breakpoint (struct breakpoint *b)
{
  return make_cleanup (do_delete_breakpoint_cleanup, b);
}

/* A callback for map_breakpoint_numbers that calls
   delete_breakpoint.  */

static void
do_delete_breakpoint (struct breakpoint *b, void *ignore)
{
  delete_breakpoint (b);
}

void
delete_command (char *arg, int from_tty)
{
  struct breakpoint *b, *b_tmp;

  dont_repeat ();

  if (arg == 0)
    {
      int breaks_to_delete = 0;

      /* Delete all breakpoints if no argument.
         Do not delete internal or call-dummy breakpoints, these have
         to be deleted with an explicit breakpoint number argument.  */
      ALL_BREAKPOINTS (b)
      {
	if (b->type != bp_call_dummy
	    && b->type != bp_std_terminate
	    && b->type != bp_shlib_event
	    && b->type != bp_jit_event
	    && b->type != bp_thread_event
	    && b->type != bp_overlay_event
	    && b->type != bp_longjmp_master
	    && b->type != bp_std_terminate_master
	    && b->type != bp_exception_master
	    && b->number >= 0)
	  {
	    breaks_to_delete = 1;
	    break;
	  }
      }

      /* Ask user only if there are some breakpoints to delete.  */
      if (!from_tty
	  || (breaks_to_delete && query (_("Delete all breakpoints? "))))
	{
	  ALL_BREAKPOINTS_SAFE (b, b_tmp)
	  {
	    if (b->type != bp_call_dummy
		&& b->type != bp_std_terminate
		&& b->type != bp_shlib_event
		&& b->type != bp_thread_event
		&& b->type != bp_jit_event
		&& b->type != bp_overlay_event
		&& b->type != bp_longjmp_master
		&& b->type != bp_std_terminate_master
		&& b->type != bp_exception_master
		&& b->number >= 0)
	      delete_breakpoint (b);
	  }
	}
    }
  else
    map_breakpoint_numbers (arg, do_delete_breakpoint, NULL);
}

static int
all_locations_are_pending (struct bp_location *loc)
{
  for (; loc; loc = loc->next)
    if (!loc->shlib_disabled)
      return 0;
  return 1;
}

/* Subroutine of update_breakpoint_locations to simplify it.
   Return non-zero if multiple fns in list LOC have the same name.
   Null names are ignored.  */

static int
ambiguous_names_p (struct bp_location *loc)
{
  struct bp_location *l;
  htab_t htab = htab_create_alloc (13, htab_hash_string,
				   (int (*) (const void *, 
					     const void *)) streq,
				   NULL, xcalloc, xfree);

  for (l = loc; l != NULL; l = l->next)
    {
      const char **slot;
      const char *name = l->function_name;

      /* Allow for some names to be NULL, ignore them.  */
      if (name == NULL)
	continue;

      slot = (const char **) htab_find_slot (htab, (const void *) name,
					     INSERT);
      /* NOTE: We can assume slot != NULL here because xcalloc never
	 returns NULL.  */
      if (*slot != NULL)
	{
	  htab_delete (htab);
	  return 1;
	}
      *slot = name;
    }

  htab_delete (htab);
  return 0;
}

/* When symbols change, it probably means the sources changed as well,
   and it might mean the static tracepoint markers are no longer at
   the same address or line numbers they used to be at last we
   checked.  Losing your static tracepoints whenever you rebuild is
   undesirable.  This function tries to resync/rematch gdb static
   tracepoints with the markers on the target, for static tracepoints
   that have not been set by marker id.  Static tracepoint that have
   been set by marker id are reset by marker id in breakpoint_re_set.
   The heuristic is:

   1) For a tracepoint set at a specific address, look for a marker at
   the old PC.  If one is found there, assume to be the same marker.
   If the name / string id of the marker found is different from the
   previous known name, assume that means the user renamed the marker
   in the sources, and output a warning.

   2) For a tracepoint set at a given line number, look for a marker
   at the new address of the old line number.  If one is found there,
   assume to be the same marker.  If the name / string id of the
   marker found is different from the previous known name, assume that
   means the user renamed the marker in the sources, and output a
   warning.

   3) If a marker is no longer found at the same address or line, it
   may mean the marker no longer exists.  But it may also just mean
   the code changed a bit.  Maybe the user added a few lines of code
   that made the marker move up or down (in line number terms).  Ask
   the target for info about the marker with the string id as we knew
   it.  If found, update line number and address in the matching
   static tracepoint.  This will get confused if there's more than one
   marker with the same ID (possible in UST, although unadvised
   precisely because it confuses tools).  */

static struct symtab_and_line
update_static_tracepoint (struct breakpoint *b, struct symtab_and_line sal)
{
  struct static_tracepoint_marker marker;
  CORE_ADDR pc;
  int i;

  pc = sal.pc;
  if (sal.line)
    find_line_pc (sal.symtab, sal.line, &pc);

  if (target_static_tracepoint_marker_at (pc, &marker))
    {
      if (strcmp (b->static_trace_marker_id, marker.str_id) != 0)
	warning (_("static tracepoint %d changed probed marker from %s to %s"),
		 b->number,
		 b->static_trace_marker_id, marker.str_id);

      xfree (b->static_trace_marker_id);
      b->static_trace_marker_id = xstrdup (marker.str_id);
      release_static_tracepoint_marker (&marker);

      return sal;
    }

  /* Old marker wasn't found on target at lineno.  Try looking it up
     by string ID.  */
  if (!sal.explicit_pc
      && sal.line != 0
      && sal.symtab != NULL
      && b->static_trace_marker_id != NULL)
    {
      VEC(static_tracepoint_marker_p) *markers;

      markers
	= target_static_tracepoint_markers_by_strid (b->static_trace_marker_id);

      if (!VEC_empty(static_tracepoint_marker_p, markers))
	{
	  struct symtab_and_line sal;
	  struct symbol *sym;
	  struct static_tracepoint_marker *marker;

	  marker = VEC_index (static_tracepoint_marker_p, markers, 0);

	  xfree (b->static_trace_marker_id);
	  b->static_trace_marker_id = xstrdup (marker->str_id);

	  warning (_("marker for static tracepoint %d (%s) not "
		     "found at previous line number"),
		   b->number, b->static_trace_marker_id);

	  init_sal (&sal);

	  sal.pc = marker->address;

	  sal = find_pc_line (marker->address, 0);
	  sym = find_pc_sect_function (marker->address, NULL);
	  ui_out_text (uiout, "Now in ");
	  if (sym)
	    {
	      ui_out_field_string (uiout, "func",
				   SYMBOL_PRINT_NAME (sym));
	      ui_out_text (uiout, " at ");
	    }
	  ui_out_field_string (uiout, "file", sal.symtab->filename);
	  ui_out_text (uiout, ":");

	  if (ui_out_is_mi_like_p (uiout))
	    {
	      char *fullname = symtab_to_fullname (sal.symtab);

	      if (fullname)
		ui_out_field_string (uiout, "fullname", fullname);
	    }

	  ui_out_field_int (uiout, "line", sal.line);
	  ui_out_text (uiout, "\n");

	  b->line_number = sal.line;

	  xfree (b->source_file);
	  if (sym)
	    b->source_file = xstrdup (sal.symtab->filename);
	  else
	    b->source_file = NULL;

	  xfree (b->addr_string);
	  b->addr_string = xstrprintf ("%s:%d",
				       sal.symtab->filename, b->line_number);

	  /* Might be nice to check if function changed, and warn if
	     so.  */

	  release_static_tracepoint_marker (marker);
	}
    }
  return sal;
}

/* Create new breakpoint locations for B (a hardware or software breakpoint)
   based on SALS and SALS_END.  If SALS_END.NELTS is not zero, then B is
   a ranged breakpoint.  */

void
update_breakpoint_locations (struct breakpoint *b,
			     struct symtabs_and_lines sals,
			     struct symtabs_and_lines sals_end)
{
  int i;
  struct bp_location *existing_locations = b->loc;

  /* Ranged breakpoints have only one start location and one end location.  */
  gdb_assert (sals_end.nelts == 0 || (sals.nelts == 1 && sals_end.nelts == 1));

  /* If there's no new locations, and all existing locations are
     pending, don't do anything.  This optimizes the common case where
     all locations are in the same shared library, that was unloaded.
     We'd like to retain the location, so that when the library is
     loaded again, we don't loose the enabled/disabled status of the
     individual locations.  */
  if (all_locations_are_pending (existing_locations) && sals.nelts == 0)
    return;

  b->loc = NULL;

  for (i = 0; i < sals.nelts; ++i)
    {
      struct bp_location *new_loc = 
	add_location_to_breakpoint (b, &(sals.sals[i]));

      /* Reparse conditions, they might contain references to the
	 old symtab.  */
      if (b->cond_string != NULL)
	{
	  char *s;
	  struct gdb_exception e;

	  s = b->cond_string;
	  TRY_CATCH (e, RETURN_MASK_ERROR)
	    {
	      new_loc->cond = parse_exp_1 (&s, block_for_pc (sals.sals[i].pc), 
					   0);
	    }
	  if (e.reason < 0)
	    {
	      warning (_("failed to reevaluate condition "
			 "for breakpoint %d: %s"), 
		       b->number, e.message);
	      new_loc->enabled = 0;
	    }
	}

      if (b->source_file != NULL)
	xfree (b->source_file);
      if (sals.sals[i].symtab == NULL)
	b->source_file = NULL;
      else
	b->source_file = xstrdup (sals.sals[i].symtab->filename);

      if (b->line_number == 0)
	b->line_number = sals.sals[i].line;

      if (sals_end.nelts)
	{
	  CORE_ADDR end = find_breakpoint_range_end (sals_end.sals[0]);

	  new_loc->length = end - sals.sals[0].pc + 1;
	}
    }

  /* Update locations of permanent breakpoints.  */
  if (b->enable_state == bp_permanent)
    make_breakpoint_permanent (b);

  /* If possible, carry over 'disable' status from existing
     breakpoints.  */
  {
    struct bp_location *e = existing_locations;
    /* If there are multiple breakpoints with the same function name,
       e.g. for inline functions, comparing function names won't work.
       Instead compare pc addresses; this is just a heuristic as things
       may have moved, but in practice it gives the correct answer
       often enough until a better solution is found.  */
    int have_ambiguous_names = ambiguous_names_p (b->loc);

    for (; e; e = e->next)
      {
	if (!e->enabled && e->function_name)
	  {
	    struct bp_location *l = b->loc;
	    if (have_ambiguous_names)
	      {
		for (; l; l = l->next)
		  if (breakpoint_locations_match (e, l))
		    {
		      l->enabled = 0;
		      break;
		    }
	      }
	    else
	      {
		for (; l; l = l->next)
		  if (l->function_name
		      && strcmp (e->function_name, l->function_name) == 0)
		    {
		      l->enabled = 0;
		      break;
		    }
	      }
	  }
      }
  }

  update_global_location_list (1);
}

/* Find the SaL locations corresponding to the given ADDR_STRING.
   On return, FOUND will be 1 if any SaL was found, zero otherwise.  */

static struct symtabs_and_lines
addr_string_to_sals (struct breakpoint *b, char *addr_string, int *found)
{
  char *s;
  int marker_spec;
  struct symtabs_and_lines sals = {0};
  struct gdb_exception e;

  s = addr_string;
  marker_spec = b->type == bp_static_tracepoint && is_marker_spec (s);

  TRY_CATCH (e, RETURN_MASK_ERROR)
    {
      if (marker_spec)
	{
	  sals = decode_static_tracepoint_spec (&s);
	  if (sals.nelts > b->static_trace_marker_id_idx)
	    {
	      sals.sals[0] = sals.sals[b->static_trace_marker_id_idx];
	      sals.nelts = 1;
	    }
	  else
	    error (_("marker %s not found"), b->static_trace_marker_id);
	}
      else
	sals = decode_line_1 (&s, 1, (struct symtab *) NULL, 0, NULL);
    }
  if (e.reason < 0)
    {
      int not_found_and_ok = 0;
      /* For pending breakpoints, it's expected that parsing will
	 fail until the right shared library is loaded.  User has
	 already told to create pending breakpoints and don't need
	 extra messages.  If breakpoint is in bp_shlib_disabled
	 state, then user already saw the message about that
	 breakpoint being disabled, and don't want to see more
	 errors.  */
      if (e.error == NOT_FOUND_ERROR
	  && (b->condition_not_parsed 
	      || (b->loc && b->loc->shlib_disabled)
	      || b->enable_state == bp_disabled))
	not_found_and_ok = 1;

      if (!not_found_and_ok)
	{
	  /* We surely don't want to warn about the same breakpoint
	     10 times.  One solution, implemented here, is disable
	     the breakpoint on error.  Another solution would be to
	     have separate 'warning emitted' flag.  Since this
	     happens only when a binary has changed, I don't know
	     which approach is better.  */
	  b->enable_state = bp_disabled;
	  throw_exception (e);
	}
    }

  if (e.reason == 0 || e.error != NOT_FOUND_ERROR)
    {
      gdb_assert (sals.nelts == 1);

      resolve_sal_pc (&sals.sals[0]);
      if (b->condition_not_parsed && s && s[0])
	{
	  char *cond_string = 0;
	  int thread = -1;
	  int task = 0;

	  find_condition_and_thread (s, sals.sals[0].pc,
				     &cond_string, &thread, &task);
	  if (cond_string)
	    b->cond_string = cond_string;
	  b->thread = thread;
	  b->task = task;
	  b->condition_not_parsed = 0;
	}

      if (b->type == bp_static_tracepoint && !marker_spec)
	sals.sals[0] = update_static_tracepoint (b, sals.sals[0]);

      *found = 1;
    }
  else
    *found = 0;

  return sals;
}

/* Reevaluate a hardware or software breakpoint and recreate its locations.
   This is necessary after symbols are read (e.g., an executable or DSO
   was loaded, or the inferior just started).  */

static void
re_set_breakpoint (struct breakpoint *b)
{
  int found;
  struct symtabs_and_lines sals, sals_end;
  struct symtabs_and_lines expanded = {0};
  struct symtabs_and_lines expanded_end = {0};
  struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);

  input_radix = b->input_radix;
  save_current_space_and_thread ();
  switch_to_program_space_and_thread (b->pspace);
  set_language (b->language);

  sals = addr_string_to_sals (b, b->addr_string, &found);
  if (found)
    {
      make_cleanup (xfree, sals.sals);
      expanded = expand_line_sal_maybe (sals.sals[0]);
    }

  if (b->addr_string_range_end)
    {
      sals_end = addr_string_to_sals (b, b->addr_string_range_end, &found);
      if (found)
	{
	  make_cleanup (xfree, sals_end.sals);
	  expanded_end = expand_line_sal_maybe (sals_end.sals[0]);
	}
    }

  update_breakpoint_locations (b, expanded, expanded_end);
  do_cleanups (cleanups);
}

/* Reset a breakpoint given it's struct breakpoint * BINT.
   The value we return ends up being the return value from catch_errors.
   Unused in this case.  */

static int
breakpoint_re_set_one (void *bint)
{
  /* Get past catch_errs.  */
  struct breakpoint *b = (struct breakpoint *) bint;

  switch (b->type)
    {
    case bp_none:
      warning (_("attempted to reset apparently deleted breakpoint #%d?"),
	       b->number);
      return 0;
    case bp_breakpoint:
    case bp_hardware_breakpoint:
    case bp_tracepoint:
    case bp_fast_tracepoint:
    case bp_static_tracepoint:
    case bp_gnu_ifunc_resolver:
      /* Do not attempt to re-set breakpoints disabled during startup.  */
      if (b->enable_state == bp_startup_disabled)
	return 0;

      if (b->addr_string == NULL)
	{
	  /* Anything without a string can't be re-set.  */
	  delete_breakpoint (b);
	  return 0;
	}

      re_set_breakpoint (b);
      break;

    case bp_watchpoint:
    case bp_hardware_watchpoint:
    case bp_read_watchpoint:
    case bp_access_watchpoint:
      /* Watchpoint can be either on expression using entirely global
	 variables, or it can be on local variables.

	 Watchpoints of the first kind are never auto-deleted, and
	 even persist across program restarts. Since they can use
	 variables from shared libraries, we need to reparse
	 expression as libraries are loaded and unloaded.

	 Watchpoints on local variables can also change meaning as
	 result of solib event.  For example, if a watchpoint uses
	 both a local and a global variables in expression, it's a
	 local watchpoint, but unloading of a shared library will make
	 the expression invalid.  This is not a very common use case,
	 but we still re-evaluate expression, to avoid surprises to
	 the user.

	 Note that for local watchpoints, we re-evaluate it only if
	 watchpoints frame id is still valid.  If it's not, it means
	 the watchpoint is out of scope and will be deleted soon.  In
	 fact, I'm not sure we'll ever be called in this case.

	 If a local watchpoint's frame id is still valid, then
	 b->exp_valid_block is likewise valid, and we can safely use it.
	 
	 Don't do anything about disabled watchpoints, since they will
	 be reevaluated again when enabled.  */
      update_watchpoint (b, 1 /* reparse */);
      break;
      /* We needn't really do anything to reset these, since the mask
         that requests them is unaffected by e.g., new libraries being
         loaded.  */
    case bp_catchpoint:
      break;

    default:
      printf_filtered (_("Deleting unknown breakpoint type %d\n"), b->type);
      /* fall through */
      /* Delete overlay event and longjmp master breakpoints; they will be
	 reset later by breakpoint_re_set.  */
    case bp_overlay_event:
    case bp_longjmp_master:
    case bp_std_terminate_master:
    case bp_exception_master:
      delete_breakpoint (b);
      break;

      /* This breakpoint is special, it's set up when the inferior
         starts and we really don't want to touch it.  */
    case bp_shlib_event:

      /* Like bp_shlib_event, this breakpoint type is special.
	 Once it is set up, we do not want to touch it.  */
    case bp_thread_event:

      /* Keep temporary breakpoints, which can be encountered when we
         step over a dlopen call and SOLIB_ADD is resetting the
         breakpoints.  Otherwise these should have been blown away via
         the cleanup chain or by breakpoint_init_inferior when we
         rerun the executable.  */
    case bp_until:
    case bp_finish:
    case bp_watchpoint_scope:
    case bp_call_dummy:
    case bp_std_terminate:
    case bp_step_resume:
    case bp_longjmp:
    case bp_longjmp_resume:
    case bp_exception:
    case bp_exception_resume:
    case bp_jit_event:
    case bp_gnu_ifunc_resolver_return:
      break;
    }

  return 0;
}

/* Re-set all breakpoints after symbols have been re-loaded.  */
void
breakpoint_re_set (void)
{
  struct breakpoint *b, *b_tmp;
  enum language save_language;
  int save_input_radix;
  struct cleanup *old_chain;

  save_language = current_language->la_language;
  save_input_radix = input_radix;
  old_chain = save_current_program_space ();

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
  {
    /* Format possible error msg.  */
    char *message = xstrprintf ("Error in re-setting breakpoint %d: ",
				b->number);
    struct cleanup *cleanups = make_cleanup (xfree, message);
    catch_errors (breakpoint_re_set_one, b, message, RETURN_MASK_ALL);
    do_cleanups (cleanups);
  }
  set_language (save_language);
  input_radix = save_input_radix;

  jit_breakpoint_re_set ();

  do_cleanups (old_chain);

  create_overlay_event_breakpoint ();
  create_longjmp_master_breakpoint ();
  create_std_terminate_master_breakpoint ();
  create_exception_master_breakpoint ();
}

/* Reset the thread number of this breakpoint:

   - If the breakpoint is for all threads, leave it as-is.
   - Else, reset it to the current thread for inferior_ptid.  */
void
breakpoint_re_set_thread (struct breakpoint *b)
{
  if (b->thread != -1)
    {
      if (in_thread_list (inferior_ptid))
	b->thread = pid_to_thread_id (inferior_ptid);

      /* We're being called after following a fork.  The new fork is
	 selected as current, and unless this was a vfork will have a
	 different program space from the original thread.  Reset that
	 as well.  */
      b->loc->pspace = current_program_space;
    }
}

/* Set ignore-count of breakpoint number BPTNUM to COUNT.
   If from_tty is nonzero, it prints a message to that effect,
   which ends with a period (no newline).  */

void
set_ignore_count (int bptnum, int count, int from_tty)
{
  struct breakpoint *b;

  if (count < 0)
    count = 0;

  ALL_BREAKPOINTS (b)
    if (b->number == bptnum)
    {
      if (is_tracepoint (b))
	{
	  if (from_tty && count != 0)
	    printf_filtered (_("Ignore count ignored for tracepoint %d."),
			     bptnum);
	  return;
	}
      
      b->ignore_count = count;
      if (from_tty)
	{
	  if (count == 0)
	    printf_filtered (_("Will stop next time "
			       "breakpoint %d is reached."),
			     bptnum);
	  else if (count == 1)
	    printf_filtered (_("Will ignore next crossing of breakpoint %d."),
			     bptnum);
	  else
	    printf_filtered (_("Will ignore next %d "
			       "crossings of breakpoint %d."),
			     count, bptnum);
	}
      breakpoints_changed ();
      observer_notify_breakpoint_modified (b->number);
      return;
    }

  error (_("No breakpoint number %d."), bptnum);
}

/* Command to set ignore-count of breakpoint N to COUNT.  */

static void
ignore_command (char *args, int from_tty)
{
  char *p = args;
  int num;

  if (p == 0)
    error_no_arg (_("a breakpoint number"));

  num = get_number (&p);
  if (num == 0)
    error (_("bad breakpoint number: '%s'"), args);
  if (*p == 0)
    error (_("Second argument (specified ignore-count) is missing."));

  set_ignore_count (num,
		    longest_to_int (value_as_long (parse_and_eval (p))),
		    from_tty);
  if (from_tty)
    printf_filtered ("\n");
}

/* Call FUNCTION on each of the breakpoints
   whose numbers are given in ARGS.  */

static void
map_breakpoint_numbers (char *args, void (*function) (struct breakpoint *,
						      void *),
			void *data)
{
  int num;
  struct breakpoint *b, *tmp;
  int match;
  struct get_number_or_range_state state;

  if (args == 0)
    error_no_arg (_("one or more breakpoint numbers"));

  init_number_or_range (&state, args);

  while (!state.finished)
    {
      char *p = state.string;

      match = 0;

      num = get_number_or_range (&state);
      if (num == 0)
	{
	  warning (_("bad breakpoint number at or near '%s'"), p);
	}
      else
	{
	  ALL_BREAKPOINTS_SAFE (b, tmp)
	    if (b->number == num)
	      {
		struct breakpoint *related_breakpoint;

		match = 1;
		related_breakpoint = b;
		do
		  {
		    struct breakpoint *next_related_b;

		    /* FUNCTION can be also delete_breakpoint.  */
		    next_related_b = related_breakpoint->related_breakpoint;
		    function (related_breakpoint, data);

		    /* For delete_breakpoint of the last entry of the ring we
		       were traversing we would never get back to B.  */
		    if (next_related_b == related_breakpoint)
		      break;
		    related_breakpoint = next_related_b;
		  }
		while (related_breakpoint != b);
		break;
	      }
	  if (match == 0)
	    printf_unfiltered (_("No breakpoint number %d.\n"), num);
	}
    }
}

static struct bp_location *
find_location_by_number (char *number)
{
  char *dot = strchr (number, '.');
  char *p1;
  int bp_num;
  int loc_num;
  struct breakpoint *b;
  struct bp_location *loc;  

  *dot = '\0';

  p1 = number;
  bp_num = get_number (&p1);
  if (bp_num == 0)
    error (_("Bad breakpoint number '%s'"), number);

  ALL_BREAKPOINTS (b)
    if (b->number == bp_num)
      {
	break;
      }

  if (!b || b->number != bp_num)
    error (_("Bad breakpoint number '%s'"), number);
  
  p1 = dot+1;
  loc_num = get_number (&p1);
  if (loc_num == 0)
    error (_("Bad breakpoint location number '%s'"), number);

  --loc_num;
  loc = b->loc;
  for (;loc_num && loc; --loc_num, loc = loc->next)
    ;
  if (!loc)
    error (_("Bad breakpoint location number '%s'"), dot+1);
    
  return loc;  
}


/* Set ignore-count of breakpoint number BPTNUM to COUNT.
   If from_tty is nonzero, it prints a message to that effect,
   which ends with a period (no newline).  */

void
disable_breakpoint (struct breakpoint *bpt)
{
  /* Never disable a watchpoint scope breakpoint; we want to
     hit them when we leave scope so we can delete both the
     watchpoint and its scope breakpoint at that time.  */
  if (bpt->type == bp_watchpoint_scope)
    return;

  /* You can't disable permanent breakpoints.  */
  if (bpt->enable_state == bp_permanent)
    return;

  bpt->enable_state = bp_disabled;

  update_global_location_list (0);

  observer_notify_breakpoint_modified (bpt->number);
}

/* A callback for map_breakpoint_numbers that calls
   disable_breakpoint.  */

static void
do_map_disable_breakpoint (struct breakpoint *b, void *ignore)
{
  disable_breakpoint (b);
}

static void
disable_command (char *args, int from_tty)
{
  struct breakpoint *bpt;

  if (args == 0)
    ALL_BREAKPOINTS (bpt)
      switch (bpt->type)
      {
      case bp_none:
	warning (_("attempted to disable apparently deleted breakpoint #%d?"),
		 bpt->number);
	break;
      case bp_breakpoint:
      case bp_tracepoint:
      case bp_fast_tracepoint:
      case bp_static_tracepoint:
      case bp_catchpoint:
      case bp_hardware_breakpoint:
      case bp_watchpoint:
      case bp_hardware_watchpoint:
      case bp_read_watchpoint:
      case bp_access_watchpoint:
	disable_breakpoint (bpt);
	break;
      default:
	break;
      }
  else if (strchr (args, '.'))
    {
      struct bp_location *loc = find_location_by_number (args);
      if (loc)
	loc->enabled = 0;
      update_global_location_list (0);
    }
  else
    map_breakpoint_numbers (args, do_map_disable_breakpoint, NULL);
}

static void
do_enable_breakpoint (struct breakpoint *bpt, enum bpdisp disposition)
{
  int target_resources_ok;

  if (bpt->type == bp_hardware_breakpoint)
    {
      int i;
      i = hw_breakpoint_used_count ();
      target_resources_ok = 
	target_can_use_hardware_watchpoint (bp_hardware_breakpoint, 
					    i + 1, 0);
      if (target_resources_ok == 0)
	error (_("No hardware breakpoint support in the target."));
      else if (target_resources_ok < 0)
	error (_("Hardware breakpoints used exceeds limit."));
    }

  if (is_watchpoint (bpt))
    {
      struct gdb_exception e;

      TRY_CATCH (e, RETURN_MASK_ALL)
	{
	  update_watchpoint (bpt, 1 /* reparse */);
	}
      if (e.reason < 0)
	{
	  exception_fprintf (gdb_stderr, e, _("Cannot enable watchpoint %d: "),
			     bpt->number);
	  return;
	}
    }

  if (bpt->enable_state != bp_permanent)
    bpt->enable_state = bp_enabled;
  bpt->disposition = disposition;
  update_global_location_list (1);
  breakpoints_changed ();
  
  observer_notify_breakpoint_modified (bpt->number);
}


void
enable_breakpoint (struct breakpoint *bpt)
{
  do_enable_breakpoint (bpt, bpt->disposition);
}

/* A callback for map_breakpoint_numbers that calls
   enable_breakpoint.  */

static void
do_map_enable_breakpoint (struct breakpoint *b, void *ignore)
{
  enable_breakpoint (b);
}

/* The enable command enables the specified breakpoints (or all defined
   breakpoints) so they once again become (or continue to be) effective
   in stopping the inferior.  */

static void
enable_command (char *args, int from_tty)
{
  struct breakpoint *bpt;

  if (args == 0)
    ALL_BREAKPOINTS (bpt)
      switch (bpt->type)
      {
      case bp_none:
	warning (_("attempted to enable apparently deleted breakpoint #%d?"),
		 bpt->number);
	break;
      case bp_breakpoint:
      case bp_tracepoint:
      case bp_fast_tracepoint:
      case bp_static_tracepoint:
      case bp_catchpoint:
      case bp_hardware_breakpoint:
      case bp_watchpoint:
      case bp_hardware_watchpoint:
      case bp_read_watchpoint:
      case bp_access_watchpoint:
	enable_breakpoint (bpt);
	break;
      default:
	break;
      }
  else if (strchr (args, '.'))
    {
      struct bp_location *loc = find_location_by_number (args);
      if (loc)
	loc->enabled = 1;
      update_global_location_list (1);
    }
  else
    map_breakpoint_numbers (args, do_map_enable_breakpoint, NULL);
}

static void
enable_once_breakpoint (struct breakpoint *bpt, void *ignore)
{
  do_enable_breakpoint (bpt, disp_disable);
}

static void
enable_once_command (char *args, int from_tty)
{
  map_breakpoint_numbers (args, enable_once_breakpoint, NULL);
}

static void
enable_delete_breakpoint (struct breakpoint *bpt, void *ignore)
{
  do_enable_breakpoint (bpt, disp_del);
}

static void
enable_delete_command (char *args, int from_tty)
{
  map_breakpoint_numbers (args, enable_delete_breakpoint, NULL);
}

static void
set_breakpoint_cmd (char *args, int from_tty)
{
}

static void
show_breakpoint_cmd (char *args, int from_tty)
{
}

/* Invalidate last known value of any hardware watchpoint if
   the memory which that value represents has been written to by
   GDB itself.  */

static void
invalidate_bp_value_on_memory_change (CORE_ADDR addr, int len,
				      const bfd_byte *data)
{
  struct breakpoint *bp;

  ALL_BREAKPOINTS (bp)
    if (bp->enable_state == bp_enabled
	&& bp->type == bp_hardware_watchpoint
	&& bp->val_valid && bp->val)
      {
	struct bp_location *loc;

	for (loc = bp->loc; loc != NULL; loc = loc->next)
	  if (loc->loc_type == bp_loc_hardware_watchpoint
	      && loc->address + loc->length > addr
	      && addr + len > loc->address)
	    {
	      value_free (bp->val);
	      bp->val = NULL;
	      bp->val_valid = 0;
	    }
      }
}

/* Use default_breakpoint_'s, or nothing if they aren't valid.  */

struct symtabs_and_lines
decode_line_spec_1 (char *string, int funfirstline)
{
  struct symtabs_and_lines sals;

  if (string == 0)
    error (_("Empty line specification."));
  if (default_breakpoint_valid)
    sals = decode_line_1 (&string, funfirstline,
			  default_breakpoint_symtab,
			  default_breakpoint_line,
			  NULL);
  else
    sals = decode_line_1 (&string, funfirstline,
			  (struct symtab *) NULL, 0, NULL);
  if (*string)
    error (_("Junk at end of line specification: %s"), string);
  return sals;
}

/* Create and insert a raw software breakpoint at PC.  Return an
   identifier, which should be used to remove the breakpoint later.
   In general, places which call this should be using something on the
   breakpoint chain instead; this function should be eliminated
   someday.  */

void *
deprecated_insert_raw_breakpoint (struct gdbarch *gdbarch,
				  struct address_space *aspace, CORE_ADDR pc)
{
  struct bp_target_info *bp_tgt;

  bp_tgt = XZALLOC (struct bp_target_info);

  bp_tgt->placed_address_space = aspace;
  bp_tgt->placed_address = pc;

  if (target_insert_breakpoint (gdbarch, bp_tgt) != 0)
    {
      /* Could not insert the breakpoint.  */
      xfree (bp_tgt);
      return NULL;
    }

  return bp_tgt;
}

/* Remove a breakpoint BP inserted by
   deprecated_insert_raw_breakpoint.  */

int
deprecated_remove_raw_breakpoint (struct gdbarch *gdbarch, void *bp)
{
  struct bp_target_info *bp_tgt = bp;
  int ret;

  ret = target_remove_breakpoint (gdbarch, bp_tgt);
  xfree (bp_tgt);

  return ret;
}

/* One (or perhaps two) breakpoints used for software single
   stepping.  */

static void *single_step_breakpoints[2];
static struct gdbarch *single_step_gdbarch[2];

/* Create and insert a breakpoint for software single step.  */

void
insert_single_step_breakpoint (struct gdbarch *gdbarch,
			       struct address_space *aspace, 
			       CORE_ADDR next_pc)
{
  void **bpt_p;

  if (single_step_breakpoints[0] == NULL)
    {
      bpt_p = &single_step_breakpoints[0];
      single_step_gdbarch[0] = gdbarch;
    }
  else
    {
      gdb_assert (single_step_breakpoints[1] == NULL);
      bpt_p = &single_step_breakpoints[1];
      single_step_gdbarch[1] = gdbarch;
    }

  /* NOTE drow/2006-04-11: A future improvement to this function would
     be to only create the breakpoints once, and actually put them on
     the breakpoint chain.  That would let us use set_raw_breakpoint.
     We could adjust the addresses each time they were needed.  Doing
     this requires corresponding changes elsewhere where single step
     breakpoints are handled, however.  So, for now, we use this.  */

  *bpt_p = deprecated_insert_raw_breakpoint (gdbarch, aspace, next_pc);
  if (*bpt_p == NULL)
    error (_("Could not insert single-step breakpoint at %s"),
	     paddress (gdbarch, next_pc));
}

/* Check if the breakpoints used for software single stepping
   were inserted or not.  */

int
single_step_breakpoints_inserted (void)
{
  return (single_step_breakpoints[0] != NULL
          || single_step_breakpoints[1] != NULL);
}

/* Remove and delete any breakpoints used for software single step.  */

void
remove_single_step_breakpoints (void)
{
  gdb_assert (single_step_breakpoints[0] != NULL);

  /* See insert_single_step_breakpoint for more about this deprecated
     call.  */
  deprecated_remove_raw_breakpoint (single_step_gdbarch[0],
				    single_step_breakpoints[0]);
  single_step_gdbarch[0] = NULL;
  single_step_breakpoints[0] = NULL;

  if (single_step_breakpoints[1] != NULL)
    {
      deprecated_remove_raw_breakpoint (single_step_gdbarch[1],
					single_step_breakpoints[1]);
      single_step_gdbarch[1] = NULL;
      single_step_breakpoints[1] = NULL;
    }
}

/* Delete software single step breakpoints without removing them from
   the inferior.  This is intended to be used if the inferior's address
   space where they were inserted is already gone, e.g. after exit or
   exec.  */

void
cancel_single_step_breakpoints (void)
{
  int i;

  for (i = 0; i < 2; i++)
    if (single_step_breakpoints[i])
      {
	xfree (single_step_breakpoints[i]);
	single_step_breakpoints[i] = NULL;
	single_step_gdbarch[i] = NULL;
      }
}

/* Detach software single-step breakpoints from INFERIOR_PTID without
   removing them.  */

static void
detach_single_step_breakpoints (void)
{
  int i;

  for (i = 0; i < 2; i++)
    if (single_step_breakpoints[i])
      target_remove_breakpoint (single_step_gdbarch[i],
				single_step_breakpoints[i]);
}

/* Check whether a software single-step breakpoint is inserted at
   PC.  */

static int
single_step_breakpoint_inserted_here_p (struct address_space *aspace, 
					CORE_ADDR pc)
{
  int i;

  for (i = 0; i < 2; i++)
    {
      struct bp_target_info *bp_tgt = single_step_breakpoints[i];
      if (bp_tgt
	  && breakpoint_address_match (bp_tgt->placed_address_space,
				       bp_tgt->placed_address,
				       aspace, pc))
	return 1;
    }

  return 0;
}

/* Returns 0 if 'bp' is NOT a syscall catchpoint,
   non-zero otherwise.  */
static int
is_syscall_catchpoint_enabled (struct breakpoint *bp)
{
  if (syscall_catchpoint_p (bp)
      && bp->enable_state != bp_disabled
      && bp->enable_state != bp_call_disabled)
    return 1;
  else
    return 0;
}

int
catch_syscall_enabled (void)
{
  struct inferior *inf = current_inferior ();

  return inf->total_syscalls_count != 0;
}

int
catching_syscall_number (int syscall_number)
{
  struct breakpoint *bp;

  ALL_BREAKPOINTS (bp)
    if (is_syscall_catchpoint_enabled (bp))
      {
	if (bp->syscalls_to_be_caught)
	  {
            int i, iter;
            for (i = 0;
                 VEC_iterate (int, bp->syscalls_to_be_caught, i, iter);
                 i++)
	      if (syscall_number == iter)
		return 1;
	  }
	else
	  return 1;
      }

  return 0;
}

/* Complete syscall names.  Used by "catch syscall".  */
static char **
catch_syscall_completer (struct cmd_list_element *cmd,
                         char *text, char *word)
{
  const char **list = get_syscall_names ();
  char **retlist
    = (list == NULL) ? NULL : complete_on_enum (list, text, word);

  xfree (list);
  return retlist;
}

/* Tracepoint-specific operations.  */

/* Set tracepoint count to NUM.  */
static void
set_tracepoint_count (int num)
{
  tracepoint_count = num;
  set_internalvar_integer (lookup_internalvar ("tpnum"), num);
}

void
trace_command (char *arg, int from_tty)
{
  if (create_breakpoint (get_current_arch (),
			 arg,
			 NULL, 0, 1 /* parse arg */,
			 0 /* tempflag */,
			 bp_tracepoint /* type_wanted */,
			 0 /* Ignore count */,
			 pending_break_support,
			 NULL,
			 from_tty,
			 1 /* enabled */,
			 0 /* internal */))
    set_tracepoint_count (breakpoint_count);
}

void
ftrace_command (char *arg, int from_tty)
{
  if (create_breakpoint (get_current_arch (),
			 arg,
			 NULL, 0, 1 /* parse arg */,
			 0 /* tempflag */,
			 bp_fast_tracepoint /* type_wanted */,
			 0 /* Ignore count */,
			 pending_break_support,
			 NULL,
			 from_tty,
			 1 /* enabled */,
			 0 /* internal */))
    set_tracepoint_count (breakpoint_count);
}

/* strace command implementation.  Creates a static tracepoint.  */

void
strace_command (char *arg, int from_tty)
{
  if (create_breakpoint (get_current_arch (),
			 arg,
			 NULL, 0, 1 /* parse arg */,
			 0 /* tempflag */,
			 bp_static_tracepoint /* type_wanted */,
			 0 /* Ignore count */,
			 pending_break_support,
			 NULL,
			 from_tty,
			 1 /* enabled */,
			 0 /* internal */))
    set_tracepoint_count (breakpoint_count);
}

/* Set up a fake reader function that gets command lines from a linked
   list that was acquired during tracepoint uploading.  */

static struct uploaded_tp *this_utp;
static int next_cmd;

static char *
read_uploaded_action (void)
{
  char *rslt;

  VEC_iterate (char_ptr, this_utp->cmd_strings, next_cmd, rslt);

  next_cmd++;

  return rslt;
}

/* Given information about a tracepoint as recorded on a target (which
   can be either a live system or a trace file), attempt to create an
   equivalent GDB tracepoint.  This is not a reliable process, since
   the target does not necessarily have all the information used when
   the tracepoint was originally defined.  */
  
struct breakpoint *
create_tracepoint_from_upload (struct uploaded_tp *utp)
{
  char *addr_str, small_buf[100];
  struct breakpoint *tp;

  if (utp->at_string)
    addr_str = utp->at_string;
  else
    {
      /* In the absence of a source location, fall back to raw
	 address.  Since there is no way to confirm that the address
	 means the same thing as when the trace was started, warn the
	 user.  */
      warning (_("Uploaded tracepoint %d has no "
		 "source location, using raw address"),
	       utp->number);
      sprintf (small_buf, "*%s", hex_string (utp->addr));
      addr_str = small_buf;
    }

  /* There's not much we can do with a sequence of bytecodes.  */
  if (utp->cond && !utp->cond_string)
    warning (_("Uploaded tracepoint %d condition "
	       "has no source form, ignoring it"),
	     utp->number);

  if (!create_breakpoint (get_current_arch (),
			  addr_str,
			  utp->cond_string, -1, 0 /* parse cond/thread */,
			  0 /* tempflag */,
			  utp->type /* type_wanted */,
			  0 /* Ignore count */,
			  pending_break_support,
			  NULL,
			  0 /* from_tty */,
			  utp->enabled /* enabled */,
			  0 /* internal */))
    return NULL;

  set_tracepoint_count (breakpoint_count);
  
  /* Get the tracepoint we just created.  */
  tp = get_tracepoint (tracepoint_count);
  gdb_assert (tp != NULL);

  if (utp->pass > 0)
    {
      sprintf (small_buf, "%d %d", utp->pass, tp->number);

      trace_pass_command (small_buf, 0);
    }

  /* If we have uploaded versions of the original commands, set up a
     special-purpose "reader" function and call the usual command line
     reader, then pass the result to the breakpoint command-setting
     function.  */
  if (!VEC_empty (char_ptr, utp->cmd_strings))
    {
      struct command_line *cmd_list;

      this_utp = utp;
      next_cmd = 0;

      cmd_list = read_command_lines_1 (read_uploaded_action, 1, NULL, NULL);

      breakpoint_set_commands (tp, cmd_list);
    }
  else if (!VEC_empty (char_ptr, utp->actions)
	   || !VEC_empty (char_ptr, utp->step_actions))
    warning (_("Uploaded tracepoint %d actions "
	       "have no source form, ignoring them"),
	     utp->number);

  return tp;
  }
  
/* Print information on tracepoint number TPNUM_EXP, or all if
   omitted.  */

static void
tracepoints_info (char *args, int from_tty)
{
  int num_printed;

  num_printed = breakpoint_1 (args, 0, is_tracepoint);

  if (num_printed == 0)
    {
      if (args == NULL || *args == '\0')
	ui_out_message (uiout, 0, "No tracepoints.\n");
      else
	ui_out_message (uiout, 0, "No tracepoint matching '%s'.\n", args);
    }

  default_collect_info ();
}

/* The 'enable trace' command enables tracepoints.
   Not supported by all targets.  */
static void
enable_trace_command (char *args, int from_tty)
{
  enable_command (args, from_tty);
}

/* The 'disable trace' command disables tracepoints.
   Not supported by all targets.  */
static void
disable_trace_command (char *args, int from_tty)
{
  disable_command (args, from_tty);
}

/* Remove a tracepoint (or all if no argument).  */
static void
delete_trace_command (char *arg, int from_tty)
{
  struct breakpoint *b, *b_tmp;

  dont_repeat ();

  if (arg == 0)
    {
      int breaks_to_delete = 0;

      /* Delete all breakpoints if no argument.
         Do not delete internal or call-dummy breakpoints, these
         have to be deleted with an explicit breakpoint number 
	 argument.  */
      ALL_TRACEPOINTS (b)
      {
	if (b->number >= 0)
	  {
	    breaks_to_delete = 1;
	    break;
	  }
      }

      /* Ask user only if there are some breakpoints to delete.  */
      if (!from_tty
	  || (breaks_to_delete && query (_("Delete all tracepoints? "))))
	{
	  ALL_BREAKPOINTS_SAFE (b, b_tmp)
	  {
	    if (is_tracepoint (b)
		&& b->number >= 0)
	      delete_breakpoint (b);
	  }
	}
    }
  else
    map_breakpoint_numbers (arg, do_delete_breakpoint, NULL);
}

/* Helper function for trace_pass_command.  */

static void
trace_pass_set_count (struct breakpoint *bp, int count, int from_tty)
{
  bp->pass_count = count;
  observer_notify_tracepoint_modified (bp->number);
  if (from_tty)
    printf_filtered (_("Setting tracepoint %d's passcount to %d\n"),
		     bp->number, count);
}

/* Set passcount for tracepoint.

   First command argument is passcount, second is tracepoint number.
   If tracepoint number omitted, apply to most recently defined.
   Also accepts special argument "all".  */

static void
trace_pass_command (char *args, int from_tty)
{
  struct breakpoint *t1;
  unsigned int count;

  if (args == 0 || *args == 0)
    error (_("passcount command requires an "
	     "argument (count + optional TP num)"));

  count = strtoul (args, &args, 10);	/* Count comes first, then TP num.  */

  while (*args && isspace ((int) *args))
    args++;

  if (*args && strncasecmp (args, "all", 3) == 0)
    {
      args += 3;			/* Skip special argument "all".  */
      if (*args)
	error (_("Junk at end of arguments."));

      ALL_TRACEPOINTS (t1)
      {
	trace_pass_set_count (t1, count, from_tty);
      }
    }
  else if (*args == '\0')
    {
      t1 = get_tracepoint_by_number (&args, NULL, 1);
      if (t1)
	trace_pass_set_count (t1, count, from_tty);
    }
  else
    {
      struct get_number_or_range_state state;

      init_number_or_range (&state, args);
      while (!state.finished)
	{
	  t1 = get_tracepoint_by_number (&args, &state, 1);
	  if (t1)
	    trace_pass_set_count (t1, count, from_tty);
	}
    }
}

struct breakpoint *
get_tracepoint (int num)
{
  struct breakpoint *t;

  ALL_TRACEPOINTS (t)
    if (t->number == num)
      return t;

  return NULL;
}

/* Find the tracepoint with the given target-side number (which may be
   different from the tracepoint number after disconnecting and
   reconnecting).  */

struct breakpoint *
get_tracepoint_by_number_on_target (int num)
{
  struct breakpoint *t;

  ALL_TRACEPOINTS (t)
    if (t->number_on_target == num)
      return t;

  return NULL;
}

/* Utility: parse a tracepoint number and look it up in the list.
   If STATE is not NULL, use, get_number_or_range_state and ignore ARG.
   If OPTIONAL_P is true, then if the argument is missing, the most
   recent tracepoint (tracepoint_count) is returned.  */
struct breakpoint *
get_tracepoint_by_number (char **arg,
			  struct get_number_or_range_state *state,
			  int optional_p)
{
  extern int tracepoint_count;
  struct breakpoint *t;
  int tpnum;
  char *instring = arg == NULL ? NULL : *arg;

  if (state)
    {
      gdb_assert (!state->finished);
      tpnum = get_number_or_range (state);
    }
  else if (arg == NULL || *arg == NULL || ! **arg)
    {
      if (optional_p)
	tpnum = tracepoint_count;
      else
	error_no_arg (_("tracepoint number"));
    }
  else
    tpnum = get_number (arg);

  if (tpnum <= 0)
    {
      if (instring && *instring)
	printf_filtered (_("bad tracepoint number at or near '%s'\n"), 
			 instring);
      else
	printf_filtered (_("Tracepoint argument missing "
			   "and no previous tracepoint\n"));
      return NULL;
    }

  ALL_TRACEPOINTS (t)
    if (t->number == tpnum)
    {
      return t;
    }

  printf_unfiltered ("No tracepoint number %d.\n", tpnum);
  return NULL;
}

/* Save information on user settable breakpoints (watchpoints, etc) to
   a new script file named FILENAME.  If FILTER is non-NULL, call it
   on each breakpoint and only include the ones for which it returns
   non-zero.  */

static void
save_breakpoints (char *filename, int from_tty,
		  int (*filter) (const struct breakpoint *))
{
  struct breakpoint *tp;
  int any = 0;
  char *pathname;
  struct cleanup *cleanup;
  struct ui_file *fp;
  int extra_trace_bits = 0;

  if (filename == 0 || *filename == 0)
    error (_("Argument required (file name in which to save)"));

  /* See if we have anything to save.  */
  ALL_BREAKPOINTS (tp)
  {
    /* Skip internal and momentary breakpoints.  */
    if (!user_breakpoint_p (tp))
      continue;

    /* If we have a filter, only save the breakpoints it accepts.  */
    if (filter && !filter (tp))
      continue;

    any = 1;

    if (is_tracepoint (tp))
      {
	extra_trace_bits = 1;

	/* We can stop searching.  */
	break;
      }
  }

  if (!any)
    {
      warning (_("Nothing to save."));
      return;
    }

  pathname = tilde_expand (filename);
  cleanup = make_cleanup (xfree, pathname);
  fp = gdb_fopen (pathname, "w");
  if (!fp)
    error (_("Unable to open file '%s' for saving (%s)"),
	   filename, safe_strerror (errno));
  make_cleanup_ui_file_delete (fp);

  if (extra_trace_bits)
    save_trace_state_variables (fp);

  ALL_BREAKPOINTS (tp)
  {
    /* Skip internal and momentary breakpoints.  */
    if (!user_breakpoint_p (tp))
      continue;

    /* If we have a filter, only save the breakpoints it accepts.  */
    if (filter && !filter (tp))
      continue;

    if (tp->ops != NULL && tp->ops->print_recreate != NULL)
      (tp->ops->print_recreate) (tp, fp);
    else
      {
	if (tp->type == bp_fast_tracepoint)
	  fprintf_unfiltered (fp, "ftrace");
	if (tp->type == bp_static_tracepoint)
	  fprintf_unfiltered (fp, "strace");
	else if (tp->type == bp_tracepoint)
	  fprintf_unfiltered (fp, "trace");
	else if (tp->type == bp_breakpoint && tp->disposition == disp_del)
	  fprintf_unfiltered (fp, "tbreak");
	else if (tp->type == bp_breakpoint)
	  fprintf_unfiltered (fp, "break");
	else if (tp->type == bp_hardware_breakpoint
		 && tp->disposition == disp_del)
	  fprintf_unfiltered (fp, "thbreak");
	else if (tp->type == bp_hardware_breakpoint)
	  fprintf_unfiltered (fp, "hbreak");
	else if (tp->type == bp_watchpoint)
	  fprintf_unfiltered (fp, "watch");
	else if (tp->type == bp_hardware_watchpoint)
	  fprintf_unfiltered (fp, "watch");
	else if (tp->type == bp_read_watchpoint)
	  fprintf_unfiltered (fp, "rwatch");
	else if (tp->type == bp_access_watchpoint)
	  fprintf_unfiltered (fp, "awatch");
	else
	  internal_error (__FILE__, __LINE__,
			  _("unhandled breakpoint type %d"), (int) tp->type);

	if (tp->exp_string)
	  fprintf_unfiltered (fp, " %s", tp->exp_string);
	else if (tp->addr_string)
	  fprintf_unfiltered (fp, " %s", tp->addr_string);
	else
	  {
	    char tmp[40];

	    sprintf_vma (tmp, tp->loc->address);
	    fprintf_unfiltered (fp, " *0x%s", tmp);
	  }
      }

    if (tp->thread != -1)
      fprintf_unfiltered (fp, " thread %d", tp->thread);

    if (tp->task != 0)
      fprintf_unfiltered (fp, " task %d", tp->task);

    fprintf_unfiltered (fp, "\n");

    /* Note, we can't rely on tp->number for anything, as we can't
       assume the recreated breakpoint numbers will match.  Use $bpnum
       instead.  */

    if (tp->cond_string)
      fprintf_unfiltered (fp, "  condition $bpnum %s\n", tp->cond_string);

    if (tp->ignore_count)
      fprintf_unfiltered (fp, "  ignore $bpnum %d\n", tp->ignore_count);

    if (tp->pass_count)
      fprintf_unfiltered (fp, "  passcount %d\n", tp->pass_count);

    if (tp->commands)
      {
	volatile struct gdb_exception ex;	

	fprintf_unfiltered (fp, "  commands\n");
	
	ui_out_redirect (uiout, fp);
	TRY_CATCH (ex, RETURN_MASK_ALL)
	  {
	    print_command_lines (uiout, tp->commands->commands, 2);
	  }
	ui_out_redirect (uiout, NULL);

	if (ex.reason < 0)
	  throw_exception (ex);

	fprintf_unfiltered (fp, "  end\n");
      }

    if (tp->enable_state == bp_disabled)
      fprintf_unfiltered (fp, "disable\n");

    /* If this is a multi-location breakpoint, check if the locations
       should be individually disabled.  Watchpoint locations are
       special, and not user visible.  */
    if (!is_watchpoint (tp) && tp->loc && tp->loc->next)
      {
	struct bp_location *loc;
	int n = 1;

	for (loc = tp->loc; loc != NULL; loc = loc->next, n++)
	  if (!loc->enabled)
	    fprintf_unfiltered (fp, "disable $bpnum.%d\n", n);
      }
  }

  if (extra_trace_bits && *default_collect)
    fprintf_unfiltered (fp, "set default-collect %s\n", default_collect);

  do_cleanups (cleanup);
  if (from_tty)
    printf_filtered (_("Saved to file '%s'.\n"), filename);
}

/* The `save breakpoints' command.  */

static void
save_breakpoints_command (char *args, int from_tty)
{
  save_breakpoints (args, from_tty, NULL);
}

/* The `save tracepoints' command.  */

static void
save_tracepoints_command (char *args, int from_tty)
{
  save_breakpoints (args, from_tty, is_tracepoint);
}

/* Create a vector of all tracepoints.  */

VEC(breakpoint_p) *
all_tracepoints (void)
{
  VEC(breakpoint_p) *tp_vec = 0;
  struct breakpoint *tp;

  ALL_TRACEPOINTS (tp)
  {
    VEC_safe_push (breakpoint_p, tp_vec, tp);
  }

  return tp_vec;
}


/* This help string is used for the break, hbreak, tbreak and thbreak
   commands.  It is defined as a macro to prevent duplication.
   COMMAND should be a string constant containing the name of the
   command.  */
#define BREAK_ARGS_HELP(command) \
command" [LOCATION] [thread THREADNUM] [if CONDITION]\n\
LOCATION may be a line number, function name, or \"*\" and an address.\n\
If a line number is specified, break at start of code for that line.\n\
If a function is specified, break at start of code for that function.\n\
If an address is specified, break at that exact address.\n\
With no LOCATION, uses current execution address of the selected\n\
stack frame.  This is useful for breaking on return to a stack frame.\n\
\n\
THREADNUM is the number from \"info threads\".\n\
CONDITION is a boolean expression.\n\
\n\
Multiple breakpoints at one place are permitted, and useful if their\n\
conditions are different.\n\
\n\
Do \"help breakpoints\" for info on other commands dealing with breakpoints."

/* List of subcommands for "catch".  */
static struct cmd_list_element *catch_cmdlist;

/* List of subcommands for "tcatch".  */
static struct cmd_list_element *tcatch_cmdlist;

/* Like add_cmd, but add the command to both the "catch" and "tcatch"
   lists, and pass some additional user data to the command function.  */
static void
add_catch_command (char *name, char *docstring,
		   void (*sfunc) (char *args, int from_tty,
				  struct cmd_list_element *command),
                   char **(*completer) (struct cmd_list_element *cmd,
                                         char *text, char *word),
		   void *user_data_catch,
		   void *user_data_tcatch)
{
  struct cmd_list_element *command;

  command = add_cmd (name, class_breakpoint, NULL, docstring,
		     &catch_cmdlist);
  set_cmd_sfunc (command, sfunc);
  set_cmd_context (command, user_data_catch);
  set_cmd_completer (command, completer);

  command = add_cmd (name, class_breakpoint, NULL, docstring,
		     &tcatch_cmdlist);
  set_cmd_sfunc (command, sfunc);
  set_cmd_context (command, user_data_tcatch);
  set_cmd_completer (command, completer);
}

static void
clear_syscall_counts (struct inferior *inf)
{
  inf->total_syscalls_count = 0;
  inf->any_syscall_count = 0;
  VEC_free (int, inf->syscalls_counts);
}

static void
save_command (char *arg, int from_tty)
{
  printf_unfiltered (_("\"save\" must be followed by "
		       "the name of a save subcommand.\n"));
  help_list (save_cmdlist, "save ", -1, gdb_stdout);
}

struct breakpoint *
iterate_over_breakpoints (int (*callback) (struct breakpoint *, void *),
			  void *data)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    {
      if ((*callback) (b, data))
	return b;
    }

  return NULL;
}

void
_initialize_breakpoint (void)
{
  struct cmd_list_element *c;

  observer_attach_solib_unloaded (disable_breakpoints_in_unloaded_shlib);
  observer_attach_inferior_exit (clear_syscall_counts);
  observer_attach_memory_changed (invalidate_bp_value_on_memory_change);

  breakpoint_objfile_key = register_objfile_data ();

  breakpoint_chain = 0;
  /* Don't bother to call set_breakpoint_count.  $bpnum isn't useful
     before a breakpoint is set.  */
  breakpoint_count = 0;

  tracepoint_count = 0;

  add_com ("ignore", class_breakpoint, ignore_command, _("\
Set ignore-count of breakpoint number N to COUNT.\n\
Usage is `ignore N COUNT'."));
  if (xdb_commands)
    add_com_alias ("bc", "ignore", class_breakpoint, 1);

  add_com ("commands", class_breakpoint, commands_command, _("\
Set commands to be executed when a breakpoint is hit.\n\
Give breakpoint number as argument after \"commands\".\n\
With no argument, the targeted breakpoint is the last one set.\n\
The commands themselves follow starting on the next line.\n\
Type a line containing \"end\" to indicate the end of them.\n\
Give \"silent\" as the first line to make the breakpoint silent;\n\
then no output is printed when it is hit, except what the commands print."));

  add_com ("condition", class_breakpoint, condition_command, _("\
Specify breakpoint number N to break only if COND is true.\n\
Usage is `condition N COND', where N is an integer and COND is an\n\
expression to be evaluated whenever breakpoint N is reached."));

  c = add_com ("tbreak", class_breakpoint, tbreak_command, _("\
Set a temporary breakpoint.\n\
Like \"break\" except the breakpoint is only temporary,\n\
so it will be deleted when hit.  Equivalent to \"break\" followed\n\
by using \"enable delete\" on the breakpoint number.\n\
\n"
BREAK_ARGS_HELP ("tbreak")));
  set_cmd_completer (c, location_completer);

  c = add_com ("hbreak", class_breakpoint, hbreak_command, _("\
Set a hardware assisted breakpoint.\n\
Like \"break\" except the breakpoint requires hardware support,\n\
some target hardware may not have this support.\n\
\n"
BREAK_ARGS_HELP ("hbreak")));
  set_cmd_completer (c, location_completer);

  c = add_com ("thbreak", class_breakpoint, thbreak_command, _("\
Set a temporary hardware assisted breakpoint.\n\
Like \"hbreak\" except the breakpoint is only temporary,\n\
so it will be deleted when hit.\n\
\n"
BREAK_ARGS_HELP ("thbreak")));
  set_cmd_completer (c, location_completer);

  add_prefix_cmd ("enable", class_breakpoint, enable_command, _("\
Enable some breakpoints.\n\
Give breakpoint numbers (separated by spaces) as arguments.\n\
With no subcommand, breakpoints are enabled until you command otherwise.\n\
This is used to cancel the effect of the \"disable\" command.\n\
With a subcommand you can enable temporarily."),
		  &enablelist, "enable ", 1, &cmdlist);
  if (xdb_commands)
    add_com ("ab", class_breakpoint, enable_command, _("\
Enable some breakpoints.\n\
Give breakpoint numbers (separated by spaces) as arguments.\n\
With no subcommand, breakpoints are enabled until you command otherwise.\n\
This is used to cancel the effect of the \"disable\" command.\n\
With a subcommand you can enable temporarily."));

  add_com_alias ("en", "enable", class_breakpoint, 1);

  add_prefix_cmd ("breakpoints", class_breakpoint, enable_command, _("\
Enable some breakpoints.\n\
Give breakpoint numbers (separated by spaces) as arguments.\n\
This is used to cancel the effect of the \"disable\" command.\n\
May be abbreviated to simply \"enable\".\n"),
		   &enablebreaklist, "enable breakpoints ", 1, &enablelist);

  add_cmd ("once", no_class, enable_once_command, _("\
Enable breakpoints for one hit.  Give breakpoint numbers.\n\
If a breakpoint is hit while enabled in this fashion, it becomes disabled."),
	   &enablebreaklist);

  add_cmd ("delete", no_class, enable_delete_command, _("\
Enable breakpoints and delete when hit.  Give breakpoint numbers.\n\
If a breakpoint is hit while enabled in this fashion, it is deleted."),
	   &enablebreaklist);

  add_cmd ("delete", no_class, enable_delete_command, _("\
Enable breakpoints and delete when hit.  Give breakpoint numbers.\n\
If a breakpoint is hit while enabled in this fashion, it is deleted."),
	   &enablelist);

  add_cmd ("once", no_class, enable_once_command, _("\
Enable breakpoints for one hit.  Give breakpoint numbers.\n\
If a breakpoint is hit while enabled in this fashion, it becomes disabled."),
	   &enablelist);

  add_prefix_cmd ("disable", class_breakpoint, disable_command, _("\
Disable some breakpoints.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To disable all breakpoints, give no argument.\n\
A disabled breakpoint is not forgotten, but has no effect until reenabled."),
		  &disablelist, "disable ", 1, &cmdlist);
  add_com_alias ("dis", "disable", class_breakpoint, 1);
  add_com_alias ("disa", "disable", class_breakpoint, 1);
  if (xdb_commands)
    add_com ("sb", class_breakpoint, disable_command, _("\
Disable some breakpoints.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To disable all breakpoints, give no argument.\n\
A disabled breakpoint is not forgotten, but has no effect until reenabled."));

  add_cmd ("breakpoints", class_alias, disable_command, _("\
Disable some breakpoints.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To disable all breakpoints, give no argument.\n\
A disabled breakpoint is not forgotten, but has no effect until reenabled.\n\
This command may be abbreviated \"disable\"."),
	   &disablelist);

  add_prefix_cmd ("delete", class_breakpoint, delete_command, _("\
Delete some breakpoints or auto-display expressions.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To delete all breakpoints, give no argument.\n\
\n\
Also a prefix command for deletion of other GDB objects.\n\
The \"unset\" command is also an alias for \"delete\"."),
		  &deletelist, "delete ", 1, &cmdlist);
  add_com_alias ("d", "delete", class_breakpoint, 1);
  add_com_alias ("del", "delete", class_breakpoint, 1);
  if (xdb_commands)
    add_com ("db", class_breakpoint, delete_command, _("\
Delete some breakpoints.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To delete all breakpoints, give no argument.\n"));

  add_cmd ("breakpoints", class_alias, delete_command, _("\
Delete some breakpoints or auto-display expressions.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To delete all breakpoints, give no argument.\n\
This command may be abbreviated \"delete\"."),
	   &deletelist);

  add_com ("clear", class_breakpoint, clear_command, _("\
Clear breakpoint at specified line or function.\n\
Argument may be line number, function name, or \"*\" and an address.\n\
If line number is specified, all breakpoints in that line are cleared.\n\
If function is specified, breakpoints at beginning of function are cleared.\n\
If an address is specified, breakpoints at that address are cleared.\n\
\n\
With no argument, clears all breakpoints in the line that the selected frame\n\
is executing in.\n\
\n\
See also the \"delete\" command which clears breakpoints by number."));
  add_com_alias ("cl", "clear", class_breakpoint, 1);

  c = add_com ("break", class_breakpoint, break_command, _("\
Set breakpoint at specified line or function.\n"
BREAK_ARGS_HELP ("break")));
  set_cmd_completer (c, location_completer);

  add_com_alias ("b", "break", class_run, 1);
  add_com_alias ("br", "break", class_run, 1);
  add_com_alias ("bre", "break", class_run, 1);
  add_com_alias ("brea", "break", class_run, 1);

  if (xdb_commands)
   add_com_alias ("ba", "break", class_breakpoint, 1);

  if (dbx_commands)
    {
      add_abbrev_prefix_cmd ("stop", class_breakpoint, stop_command, _("\
Break in function/address or break at a line in the current file."),
			     &stoplist, "stop ", 1, &cmdlist);
      add_cmd ("in", class_breakpoint, stopin_command,
	       _("Break in function or address."), &stoplist);
      add_cmd ("at", class_breakpoint, stopat_command,
	       _("Break at a line in the current file."), &stoplist);
      add_com ("status", class_info, breakpoints_info, _("\
Status of user-settable breakpoints, or breakpoint number NUMBER.\n\
The \"Type\" column indicates one of:\n\
\tbreakpoint     - normal breakpoint\n\
\twatchpoint     - watchpoint\n\
The \"Disp\" column contains one of \"keep\", \"del\", or \"dis\" to indicate\n\
the disposition of the breakpoint after it gets hit.  \"dis\" means that the\n\
breakpoint will be disabled.  The \"Address\" and \"What\" columns indicate the\n\
address and file/line number respectively.\n\
\n\
Convenience variable \"$_\" and default examine address for \"x\"\n\
are set to the address of the last breakpoint listed unless the command\n\
is prefixed with \"server \".\n\n\
Convenience variable \"$bpnum\" contains the number of the last\n\
breakpoint set."));
    }

  add_info ("breakpoints", breakpoints_info, _("\
Status of specified breakpoints (all user-settable breakpoints if no argument).\n\
The \"Type\" column indicates one of:\n\
\tbreakpoint     - normal breakpoint\n\
\twatchpoint     - watchpoint\n\
The \"Disp\" column contains one of \"keep\", \"del\", or \"dis\" to indicate\n\
the disposition of the breakpoint after it gets hit.  \"dis\" means that the\n\
breakpoint will be disabled.  The \"Address\" and \"What\" columns indicate the\n\
address and file/line number respectively.\n\
\n\
Convenience variable \"$_\" and default examine address for \"x\"\n\
are set to the address of the last breakpoint listed unless the command\n\
is prefixed with \"server \".\n\n\
Convenience variable \"$bpnum\" contains the number of the last\n\
breakpoint set."));

  add_info_alias ("b", "breakpoints", 1);

  if (xdb_commands)
    add_com ("lb", class_breakpoint, breakpoints_info, _("\
Status of user-settable breakpoints, or breakpoint number NUMBER.\n\
The \"Type\" column indicates one of:\n\
\tbreakpoint     - normal breakpoint\n\
\twatchpoint     - watchpoint\n\
The \"Disp\" column contains one of \"keep\", \"del\", or \"dis\" to indicate\n\
the disposition of the breakpoint after it gets hit.  \"dis\" means that the\n\
breakpoint will be disabled.  The \"Address\" and \"What\" columns indicate the\n\
address and file/line number respectively.\n\
\n\
Convenience variable \"$_\" and default examine address for \"x\"\n\
are set to the address of the last breakpoint listed unless the command\n\
is prefixed with \"server \".\n\n\
Convenience variable \"$bpnum\" contains the number of the last\n\
breakpoint set."));

  add_cmd ("breakpoints", class_maintenance, maintenance_info_breakpoints, _("\
Status of all breakpoints, or breakpoint number NUMBER.\n\
The \"Type\" column indicates one of:\n\
\tbreakpoint     - normal breakpoint\n\
\twatchpoint     - watchpoint\n\
\tlongjmp        - internal breakpoint used to step through longjmp()\n\
\tlongjmp resume - internal breakpoint at the target of longjmp()\n\
\tuntil          - internal breakpoint used by the \"until\" command\n\
\tfinish         - internal breakpoint used by the \"finish\" command\n\
The \"Disp\" column contains one of \"keep\", \"del\", or \"dis\" to indicate\n\
the disposition of the breakpoint after it gets hit.  \"dis\" means that the\n\
breakpoint will be disabled.  The \"Address\" and \"What\" columns indicate the\n\
address and file/line number respectively.\n\
\n\
Convenience variable \"$_\" and default examine address for \"x\"\n\
are set to the address of the last breakpoint listed unless the command\n\
is prefixed with \"server \".\n\n\
Convenience variable \"$bpnum\" contains the number of the last\n\
breakpoint set."),
	   &maintenanceinfolist);

  add_prefix_cmd ("catch", class_breakpoint, catch_command, _("\
Set catchpoints to catch events."),
		  &catch_cmdlist, "catch ",
		  0/*allow-unknown*/, &cmdlist);

  add_prefix_cmd ("tcatch", class_breakpoint, tcatch_command, _("\
Set temporary catchpoints to catch events."),
		  &tcatch_cmdlist, "tcatch ",
		  0/*allow-unknown*/, &cmdlist);

  /* Add catch and tcatch sub-commands.  */
  add_catch_command ("catch", _("\
Catch an exception, when caught.\n\
With an argument, catch only exceptions with the given name."),
		     catch_catch_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("throw", _("\
Catch an exception, when thrown.\n\
With an argument, catch only exceptions with the given name."),
		     catch_throw_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("fork", _("Catch calls to fork."),
		     catch_fork_command_1,
                     NULL,
		     (void *) (uintptr_t) catch_fork_permanent,
		     (void *) (uintptr_t) catch_fork_temporary);
  add_catch_command ("vfork", _("Catch calls to vfork."),
		     catch_fork_command_1,
                     NULL,
		     (void *) (uintptr_t) catch_vfork_permanent,
		     (void *) (uintptr_t) catch_vfork_temporary);
  add_catch_command ("exec", _("Catch calls to exec."),
		     catch_exec_command_1,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("syscall", _("\
Catch system calls by their names and/or numbers.\n\
Arguments say which system calls to catch.  If no arguments\n\
are given, every system call will be caught.\n\
Arguments, if given, should be one or more system call names\n\
(if your system supports that), or system call numbers."),
		     catch_syscall_command_1,
		     catch_syscall_completer,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("exception", _("\
Catch Ada exceptions, when raised.\n\
With an argument, catch only exceptions with the given name."),
		     catch_ada_exception_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("assert", _("\
Catch failed Ada assertions, when raised.\n\
With an argument, catch only exceptions with the given name."),
		     catch_assert_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);

  c = add_com ("watch", class_breakpoint, watch_command, _("\
Set a watchpoint for an expression.\n\
Usage: watch [-l|-location] EXPRESSION\n\
A watchpoint stops execution of your program whenever the value of\n\
an expression changes.\n\
If -l or -location is given, this evaluates EXPRESSION and watches\n\
the memory to which it refers."));
  set_cmd_completer (c, expression_completer);

  c = add_com ("rwatch", class_breakpoint, rwatch_command, _("\
Set a read watchpoint for an expression.\n\
Usage: rwatch [-l|-location] EXPRESSION\n\
A watchpoint stops execution of your program whenever the value of\n\
an expression is read.\n\
If -l or -location is given, this evaluates EXPRESSION and watches\n\
the memory to which it refers."));
  set_cmd_completer (c, expression_completer);

  c = add_com ("awatch", class_breakpoint, awatch_command, _("\
Set a watchpoint for an expression.\n\
Usage: awatch [-l|-location] EXPRESSION\n\
A watchpoint stops execution of your program whenever the value of\n\
an expression is either read or written.\n\
If -l or -location is given, this evaluates EXPRESSION and watches\n\
the memory to which it refers."));
  set_cmd_completer (c, expression_completer);

  add_info ("watchpoints", watchpoints_info, _("\
Status of specified watchpoints (all watchpoints if no argument)."));

  /* XXX: cagney/2005-02-23: This should be a boolean, and should
     respond to changes - contrary to the description.  */
  add_setshow_zinteger_cmd ("can-use-hw-watchpoints", class_support,
			    &can_use_hw_watchpoints, _("\
Set debugger's willingness to use watchpoint hardware."), _("\
Show debugger's willingness to use watchpoint hardware."), _("\
If zero, gdb will not use hardware for new watchpoints, even if\n\
such is available.  (However, any hardware watchpoints that were\n\
created before setting this to nonzero, will continue to use watchpoint\n\
hardware.)"),
			    NULL,
			    show_can_use_hw_watchpoints,
			    &setlist, &showlist);

  can_use_hw_watchpoints = 1;

  /* Tracepoint manipulation commands.  */

  c = add_com ("trace", class_breakpoint, trace_command, _("\
Set a tracepoint at specified line or function.\n\
\n"
BREAK_ARGS_HELP ("trace") "\n\
Do \"help tracepoints\" for info on other tracepoint commands."));
  set_cmd_completer (c, location_completer);

  add_com_alias ("tp", "trace", class_alias, 0);
  add_com_alias ("tr", "trace", class_alias, 1);
  add_com_alias ("tra", "trace", class_alias, 1);
  add_com_alias ("trac", "trace", class_alias, 1);

  c = add_com ("ftrace", class_breakpoint, ftrace_command, _("\
Set a fast tracepoint at specified line or function.\n\
\n"
BREAK_ARGS_HELP ("ftrace") "\n\
Do \"help tracepoints\" for info on other tracepoint commands."));
  set_cmd_completer (c, location_completer);

  c = add_com ("strace", class_breakpoint, strace_command, _("\
Set a static tracepoint at specified line, function or marker.\n\
\n\
strace [LOCATION] [if CONDITION]\n\
LOCATION may be a line number, function name, \"*\" and an address,\n\
or -m MARKER_ID.\n\
If a line number is specified, probe the marker at start of code\n\
for that line.  If a function is specified, probe the marker at start\n\
of code for that function.  If an address is specified, probe the marker\n\
at that exact address.  If a marker id is specified, probe the marker\n\
with that name.  With no LOCATION, uses current execution address of\n\
the selected stack frame.\n\
Static tracepoints accept an extra collect action -- ``collect $_sdata''.\n\
This collects arbitrary user data passed in the probe point call to the\n\
tracing library.  You can inspect it when analyzing the trace buffer,\n\
by printing the $_sdata variable like any other convenience variable.\n\
\n\
CONDITION is a boolean expression.\n\
\n\
Multiple tracepoints at one place are permitted, and useful if their\n\
conditions are different.\n\
\n\
Do \"help breakpoints\" for info on other commands dealing with breakpoints.\n\
Do \"help tracepoints\" for info on other tracepoint commands."));
  set_cmd_completer (c, location_completer);

  add_info ("tracepoints", tracepoints_info, _("\
Status of specified tracepoints (all tracepoints if no argument).\n\
Convenience variable \"$tpnum\" contains the number of the\n\
last tracepoint set."));

  add_info_alias ("tp", "tracepoints", 1);

  add_cmd ("tracepoints", class_trace, delete_trace_command, _("\
Delete specified tracepoints.\n\
Arguments are tracepoint numbers, separated by spaces.\n\
No argument means delete all tracepoints."),
	   &deletelist);

  c = add_cmd ("tracepoints", class_trace, disable_trace_command, _("\
Disable specified tracepoints.\n\
Arguments are tracepoint numbers, separated by spaces.\n\
No argument means disable all tracepoints."),
	   &disablelist);
  deprecate_cmd (c, "disable");

  c = add_cmd ("tracepoints", class_trace, enable_trace_command, _("\
Enable specified tracepoints.\n\
Arguments are tracepoint numbers, separated by spaces.\n\
No argument means enable all tracepoints."),
	   &enablelist);
  deprecate_cmd (c, "enable");

  add_com ("passcount", class_trace, trace_pass_command, _("\
Set the passcount for a tracepoint.\n\
The trace will end when the tracepoint has been passed 'count' times.\n\
Usage: passcount COUNT TPNUM, where TPNUM may also be \"all\";\n\
if TPNUM is omitted, passcount refers to the last tracepoint defined."));

  add_prefix_cmd ("save", class_breakpoint, save_command,
		  _("Save breakpoint definitions as a script."),
		  &save_cmdlist, "save ",
		  0/*allow-unknown*/, &cmdlist);

  c = add_cmd ("breakpoints", class_breakpoint, save_breakpoints_command, _("\
Save current breakpoint definitions as a script.\n\
This includes all types of breakpoints (breakpoints, watchpoints,\n\
catchpoints, tracepoints).  Use the 'source' command in another debug\n\
session to restore them."),
	       &save_cmdlist);
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("tracepoints", class_trace, save_tracepoints_command, _("\
Save current tracepoint definitions as a script.\n\
Use the 'source' command in another debug session to restore them."),
	       &save_cmdlist);
  set_cmd_completer (c, filename_completer);

  c = add_com_alias ("save-tracepoints", "save tracepoints", class_trace, 0);
  deprecate_cmd (c, "save tracepoints");

  add_prefix_cmd ("breakpoint", class_maintenance, set_breakpoint_cmd, _("\
Breakpoint specific settings\n\
Configure various breakpoint-specific variables such as\n\
pending breakpoint behavior"),
		  &breakpoint_set_cmdlist, "set breakpoint ",
		  0/*allow-unknown*/, &setlist);
  add_prefix_cmd ("breakpoint", class_maintenance, show_breakpoint_cmd, _("\
Breakpoint specific settings\n\
Configure various breakpoint-specific variables such as\n\
pending breakpoint behavior"),
		  &breakpoint_show_cmdlist, "show breakpoint ",
		  0/*allow-unknown*/, &showlist);

  add_setshow_auto_boolean_cmd ("pending", no_class,
				&pending_break_support, _("\
Set debugger's behavior regarding pending breakpoints."), _("\
Show debugger's behavior regarding pending breakpoints."), _("\
If on, an unrecognized breakpoint location will cause gdb to create a\n\
pending breakpoint.  If off, an unrecognized breakpoint location results in\n\
an error.  If auto, an unrecognized breakpoint location results in a\n\
user-query to see if a pending breakpoint should be created."),
				NULL,
				show_pending_break_support,
				&breakpoint_set_cmdlist,
				&breakpoint_show_cmdlist);

  pending_break_support = AUTO_BOOLEAN_AUTO;

  add_setshow_boolean_cmd ("auto-hw", no_class,
			   &automatic_hardware_breakpoints, _("\
Set automatic usage of hardware breakpoints."), _("\
Show automatic usage of hardware breakpoints."), _("\
If set, the debugger will automatically use hardware breakpoints for\n\
breakpoints set with \"break\" but falling in read-only memory.  If not set,\n\
a warning will be emitted for such breakpoints."),
			   NULL,
			   show_automatic_hardware_breakpoints,
			   &breakpoint_set_cmdlist,
			   &breakpoint_show_cmdlist);

  add_setshow_enum_cmd ("always-inserted", class_support,
			always_inserted_enums, &always_inserted_mode, _("\
Set mode for inserting breakpoints."), _("\
Show mode for inserting breakpoints."), _("\
When this mode is off, breakpoints are inserted in inferior when it is\n\
resumed, and removed when execution stops.  When this mode is on,\n\
breakpoints are inserted immediately and removed only when the user\n\
deletes the breakpoint.  When this mode is auto (which is the default),\n\
the behaviour depends on the non-stop setting (see help set non-stop).\n\
In this case, if gdb is controlling the inferior in non-stop mode, gdb\n\
behaves as if always-inserted mode is on; if gdb is controlling the\n\
inferior in all-stop mode, gdb behaves as if always-inserted mode is off."),
			   NULL,
			   &show_always_inserted_mode,
			   &breakpoint_set_cmdlist,
			   &breakpoint_show_cmdlist);

  add_com ("break-range", class_breakpoint, break_range_command, _("\
Set a breakpoint for an address range.\n\
break-range START-LOCATION, END-LOCATION\n\
where START-LOCATION and END-LOCATION can be one of the following:\n\
  LINENUM, for that line in the current file,\n\
  FILE:LINENUM, for that line in that file,\n\
  +OFFSET, for that number of lines after the current line\n\
           or the start of the range\n\
  FUNCTION, for the first line in that function,\n\
  FILE:FUNCTION, to distinguish among like-named static functions.\n\
  *ADDRESS, for the instruction at that address.\n\
\n\
The breakpoint will stop execution of the inferior whenever it executes\n\
an instruction at any address within the [START-LOCATION, END-LOCATION]\n\
range (including START-LOCATION and END-LOCATION)."));

  automatic_hardware_breakpoints = 1;

  observer_attach_about_to_proceed (breakpoint_about_to_proceed);
}
