/* Everything about breakpoints, for GDB.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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
#include "infrun.h"
#include "gdbthread.h"
#include "target.h"
#include "language.h"
#include "gdb-demangle.h"
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
#include "block.h"
#include "solib.h"
#include "solist.h"
#include "observer.h"
#include "memattr.h"
#include "ada-lang.h"
#include "top.h"
#include "valprint.h"
#include "jit.h"
#include "parser-defs.h"
#include "gdb_regex.h"
#include "probe.h"
#include "cli/cli-utils.h"
#include "continuations.h"
#include "stack.h"
#include "skip.h"
#include "ax-gdb.h"
#include "dummy-frame.h"
#include "interps.h"
#include "format.h"
#include "location.h"
#include "thread-fsm.h"
#include "tid-parse.h"

/* readline include files */
#include "readline/readline.h"
#include "readline/history.h"

/* readline defines this.  */
#undef savestring

#include "mi/mi-common.h"
#include "extension.h"

/* Enums for exception-handling support.  */
enum exception_event_kind
{
  EX_EVENT_THROW,
  EX_EVENT_RETHROW,
  EX_EVENT_CATCH
};

/* Prototypes for local functions.  */

static void enable_delete_command (char *, int);

static void enable_once_command (char *, int);

static void enable_count_command (char *, int);

static void disable_command (char *, int);

static void enable_command (char *, int);

static void map_breakpoint_numbers (char *, void (*) (struct breakpoint *,
						      void *),
				    void *);

static void ignore_command (char *, int);

static int breakpoint_re_set_one (void *);

static void breakpoint_re_set_default (struct breakpoint *);

static void
  create_sals_from_location_default (const struct event_location *location,
				     struct linespec_result *canonical,
				     enum bptype type_wanted);

static void create_breakpoints_sal_default (struct gdbarch *,
					    struct linespec_result *,
					    char *, char *, enum bptype,
					    enum bpdisp, int, int,
					    int,
					    const struct breakpoint_ops *,
					    int, int, int, unsigned);

static void decode_location_default (struct breakpoint *b,
				     const struct event_location *location,
				     struct program_space *search_pspace,
				     struct symtabs_and_lines *sals);

static void clear_command (char *, int);

static void catch_command (char *, int);

static int can_use_hardware_watchpoint (struct value *);

static void break_command_1 (char *, int, int);

static void mention (struct breakpoint *);

static struct breakpoint *set_raw_breakpoint_without_location (struct gdbarch *,
							       enum bptype,
							       const struct breakpoint_ops *);
static struct bp_location *add_location_to_breakpoint (struct breakpoint *,
						       const struct symtab_and_line *);

/* This function is used in gdbtk sources and thus can not be made
   static.  */
struct breakpoint *set_raw_breakpoint (struct gdbarch *gdbarch,
				       struct symtab_and_line,
				       enum bptype,
				       const struct breakpoint_ops *);

static struct breakpoint *
  momentary_breakpoint_from_master (struct breakpoint *orig,
				    enum bptype type,
				    const struct breakpoint_ops *ops,
				    int loc_enabled);

static void breakpoint_adjustment_warning (CORE_ADDR, CORE_ADDR, int, int);

static CORE_ADDR adjust_breakpoint_address (struct gdbarch *gdbarch,
					    CORE_ADDR bpaddr,
                                            enum bptype bptype);

static void describe_other_breakpoints (struct gdbarch *,
					struct program_space *, CORE_ADDR,
					struct obj_section *, int);

static int watchpoint_locations_match (struct bp_location *loc1,
				       struct bp_location *loc2);

static int breakpoint_location_address_match (struct bp_location *bl,
					      struct address_space *aspace,
					      CORE_ADDR addr);

static int breakpoint_location_address_range_overlap (struct bp_location *,
						      struct address_space *,
						      CORE_ADDR, int);

static void breakpoints_info (char *, int);

static void watchpoints_info (char *, int);

static int breakpoint_1 (char *, int, 
			 int (*) (const struct breakpoint *));

static int breakpoint_cond_eval (void *);

static void cleanup_executing_breakpoints (void *);

static void commands_command (char *, int);

static void condition_command (char *, int);

static int remove_breakpoint (struct bp_location *);
static int remove_breakpoint_1 (struct bp_location *, enum remove_bp_reason);

static enum print_stop_action print_bp_stop_message (bpstat bs);

static int watchpoint_check (void *);

static void maintenance_info_breakpoints (char *, int);

static int hw_breakpoint_used_count (void);

static int hw_watchpoint_use_count (struct breakpoint *);

static int hw_watchpoint_used_count_others (struct breakpoint *except,
					    enum bptype type,
					    int *other_type_used);

static void hbreak_command (char *, int);

static void thbreak_command (char *, int);

static void enable_breakpoint_disp (struct breakpoint *, enum bpdisp,
				    int count);

static void stop_command (char *arg, int from_tty);

static void stopin_command (char *arg, int from_tty);

static void stopat_command (char *arg, int from_tty);

static void tcatch_command (char *arg, int from_tty);

static void free_bp_location (struct bp_location *loc);
static void incref_bp_location (struct bp_location *loc);
static void decref_bp_location (struct bp_location **loc);

static struct bp_location *allocate_bp_location (struct breakpoint *bpt);

/* update_global_location_list's modes of operation wrt to whether to
   insert locations now.  */
enum ugll_insert_mode
{
  /* Don't insert any breakpoint locations into the inferior, only
     remove already-inserted locations that no longer should be
     inserted.  Functions that delete a breakpoint or breakpoints
     should specify this mode, so that deleting a breakpoint doesn't
     have the side effect of inserting the locations of other
     breakpoints that are marked not-inserted, but should_be_inserted
     returns true on them.

     This behavior is useful is situations close to tear-down -- e.g.,
     after an exec, while the target still has execution, but
     breakpoint shadows of the previous executable image should *NOT*
     be restored to the new image; or before detaching, where the
     target still has execution and wants to delete breakpoints from
     GDB's lists, and all breakpoints had already been removed from
     the inferior.  */
  UGLL_DONT_INSERT,

  /* May insert breakpoints iff breakpoints_should_be_inserted_now
     claims breakpoints should be inserted now.  */
  UGLL_MAY_INSERT,

  /* Insert locations now, irrespective of
     breakpoints_should_be_inserted_now.  E.g., say all threads are
     stopped right now, and the user did "continue".  We need to
     insert breakpoints _before_ resuming the target, but
     UGLL_MAY_INSERT wouldn't insert them, because
     breakpoints_should_be_inserted_now returns false at that point,
     as no thread is running yet.  */
  UGLL_INSERT
};

static void update_global_location_list (enum ugll_insert_mode);

static void update_global_location_list_nothrow (enum ugll_insert_mode);

static int is_hardware_watchpoint (const struct breakpoint *bpt);

static void insert_breakpoint_locations (void);

static void tracepoints_info (char *, int);

static void delete_trace_command (char *, int);

static void enable_trace_command (char *, int);

static void disable_trace_command (char *, int);

static void trace_pass_command (char *, int);

static void set_tracepoint_count (int num);

static int is_masked_watchpoint (const struct breakpoint *b);

static struct bp_location **get_first_locp_gte_addr (CORE_ADDR address);

/* Return 1 if B refers to a static tracepoint set by marker ("-m"), zero
   otherwise.  */

static int strace_marker_p (struct breakpoint *b);

/* The breakpoint_ops structure to be inherited by all breakpoint_ops
   that are implemented on top of software or hardware breakpoints
   (user breakpoints, internal and momentary breakpoints, etc.).  */
static struct breakpoint_ops bkpt_base_breakpoint_ops;

/* Internal breakpoints class type.  */
static struct breakpoint_ops internal_breakpoint_ops;

/* Momentary breakpoints class type.  */
static struct breakpoint_ops momentary_breakpoint_ops;

/* Momentary breakpoints for bp_longjmp and bp_exception class type.  */
static struct breakpoint_ops longjmp_breakpoint_ops;

/* The breakpoint_ops structure to be used in regular user created
   breakpoints.  */
struct breakpoint_ops bkpt_breakpoint_ops;

/* Breakpoints set on probes.  */
static struct breakpoint_ops bkpt_probe_breakpoint_ops;

/* Dynamic printf class type.  */
struct breakpoint_ops dprintf_breakpoint_ops;

/* The style in which to perform a dynamic printf.  This is a user
   option because different output options have different tradeoffs;
   if GDB does the printing, there is better error handling if there
   is a problem with any of the arguments, but using an inferior
   function lets you have special-purpose printers and sending of
   output to the same place as compiled-in print functions.  */

static const char dprintf_style_gdb[] = "gdb";
static const char dprintf_style_call[] = "call";
static const char dprintf_style_agent[] = "agent";
static const char *const dprintf_style_enums[] = {
  dprintf_style_gdb,
  dprintf_style_call,
  dprintf_style_agent,
  NULL
};
static const char *dprintf_style = dprintf_style_gdb;

/* The function to use for dynamic printf if the preferred style is to
   call into the inferior.  The value is simply a string that is
   copied into the command, so it can be anything that GDB can
   evaluate to a callable address, not necessarily a function name.  */

static char *dprintf_function = "";

/* The channel to use for dynamic printf if the preferred style is to
   call into the inferior; if a nonempty string, it will be passed to
   the call as the first argument, with the format string as the
   second.  As with the dprintf function, this can be anything that
   GDB knows how to evaluate, so in addition to common choices like
   "stderr", this could be an app-specific expression like
   "mystreams[curlogger]".  */

static char *dprintf_channel = "";

/* True if dprintf commands should continue to operate even if GDB
   has disconnected.  */
static int disconnected_dprintf = 1;

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

/* If on, GDB keeps breakpoints inserted even if the inferior is
   stopped, and immediately inserts any new breakpoints as soon as
   they're created.  If off (default), GDB keeps breakpoints off of
   the target as long as possible.  That is, it delays inserting
   breakpoints until the next resume, and removes them again when the
   target fully stops.  This is a bit safer in case GDB crashes while
   processing user input.  */
static int always_inserted_mode = 0;

static void
show_always_inserted_mode (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Always inserted breakpoint mode is %s.\n"),
		    value);
}

/* See breakpoint.h.  */

int
breakpoints_should_be_inserted_now (void)
{
  if (gdbarch_has_global_breakpoints (target_gdbarch ()))
    {
      /* If breakpoints are global, they should be inserted even if no
	 thread under gdb's control is running, or even if there are
	 no threads under GDB's control yet.  */
      return 1;
    }
  else if (target_has_execution)
    {
      struct thread_info *tp;

      if (always_inserted_mode)
	{
	  /* The user wants breakpoints inserted even if all threads
	     are stopped.  */
	  return 1;
	}

      if (threads_are_executing ())
	return 1;

      /* Don't remove breakpoints yet if, even though all threads are
	 stopped, we still have events to process.  */
      ALL_NON_EXITED_THREADS (tp)
	if (tp->resumed
	    && tp->suspend.waitstatus_pending_p)
	  return 1;
    }
  return 0;
}

static const char condition_evaluation_both[] = "host or target";

/* Modes for breakpoint condition evaluation.  */
static const char condition_evaluation_auto[] = "auto";
static const char condition_evaluation_host[] = "host";
static const char condition_evaluation_target[] = "target";
static const char *const condition_evaluation_enums[] = {
  condition_evaluation_auto,
  condition_evaluation_host,
  condition_evaluation_target,
  NULL
};

/* Global that holds the current mode for breakpoint condition evaluation.  */
static const char *condition_evaluation_mode_1 = condition_evaluation_auto;

/* Global that we use to display information to the user (gets its value from
   condition_evaluation_mode_1.  */
static const char *condition_evaluation_mode = condition_evaluation_auto;

/* Translate a condition evaluation mode MODE into either "host"
   or "target".  This is used mostly to translate from "auto" to the
   real setting that is being used.  It returns the translated
   evaluation mode.  */

static const char *
translate_condition_evaluation_mode (const char *mode)
{
  if (mode == condition_evaluation_auto)
    {
      if (target_supports_evaluation_of_breakpoint_conditions ())
	return condition_evaluation_target;
      else
	return condition_evaluation_host;
    }
  else
    return mode;
}

/* Discovers what condition_evaluation_auto translates to.  */

static const char *
breakpoint_condition_evaluation_mode (void)
{
  return translate_condition_evaluation_mode (condition_evaluation_mode);
}

/* Return true if GDB should evaluate breakpoint conditions or false
   otherwise.  */

static int
gdb_evaluates_breakpoint_condition_p (void)
{
  const char *mode = breakpoint_condition_evaluation_mode ();

  return (mode == condition_evaluation_host);
}

void _initialize_breakpoint (void);

/* Are we executing breakpoint commands?  */
static int executing_breakpoint_commands;

/* Are overlay event breakpoints enabled? */
static int overlay_events_enabled;

/* See description in breakpoint.h. */
int target_exact_watchpoints = 0;

/* Walk the following statement or block through all breakpoints.
   ALL_BREAKPOINTS_SAFE does so even if the statement deletes the
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

/* Iterates through locations with address ADDRESS for the currently selected
   program space.  BP_LOCP_TMP points to each object.  BP_LOCP_START points
   to where the loop should start from.
   If BP_LOCP_START is a NULL pointer, the macro automatically seeks the
   appropriate location to start with.  */

#define ALL_BP_LOCATIONS_AT_ADDR(BP_LOCP_TMP, BP_LOCP_START, ADDRESS)	\
	for (BP_LOCP_START = BP_LOCP_START == NULL ? get_first_locp_gte_addr (ADDRESS) : BP_LOCP_START, \
	     BP_LOCP_TMP = BP_LOCP_START;				\
	     BP_LOCP_START						\
	     && (BP_LOCP_TMP < bp_location + bp_location_count		\
	     && (*BP_LOCP_TMP)->address == ADDRESS);			\
	     BP_LOCP_TMP++)

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

/* See declaration at breakpoint.h.  */

struct breakpoint *
breakpoint_find_if (int (*func) (struct breakpoint *b, void *d),
		    void *user_data)
{
  struct breakpoint *b = NULL;

  ALL_BREAKPOINTS (b)
    {
      if (func (b, user_data) != 0)
	break;
    }

  return b;
}

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
  struct counted_command_line *result = XNEW (struct counted_command_line);

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
  decref_counted_command_line ((struct counted_command_line **) arg);
}

/* Create a cleanup that calls decref_counted_command_line on the
   argument.  */

static struct cleanup *
make_cleanup_decref_counted_command_line (struct counted_command_line **cmdp)
{
  return make_cleanup (do_cleanup_counted_command_line, cmdp);
}


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



/* Mark locations as "conditions have changed" in case the target supports
   evaluating conditions on its side.  */

static void
mark_breakpoint_modified (struct breakpoint *b)
{
  struct bp_location *loc;

  /* This is only meaningful if the target is
     evaluating conditions and if the user has
     opted for condition evaluation on the target's
     side.  */
  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return;

  if (!is_breakpoint (b))
    return;

  for (loc = b->loc; loc; loc = loc->next)
    loc->condition_changed = condition_modified;
}

/* Mark location as "conditions have changed" in case the target supports
   evaluating conditions on its side.  */

static void
mark_breakpoint_location_modified (struct bp_location *loc)
{
  /* This is only meaningful if the target is
     evaluating conditions and if the user has
     opted for condition evaluation on the target's
     side.  */
  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())

    return;

  if (!is_breakpoint (loc->owner))
    return;

  loc->condition_changed = condition_modified;
}

/* Sets the condition-evaluation mode using the static global
   condition_evaluation_mode.  */

static void
set_condition_evaluation_mode (char *args, int from_tty,
			       struct cmd_list_element *c)
{
  const char *old_mode, *new_mode;

  if ((condition_evaluation_mode_1 == condition_evaluation_target)
      && !target_supports_evaluation_of_breakpoint_conditions ())
    {
      condition_evaluation_mode_1 = condition_evaluation_mode;
      warning (_("Target does not support breakpoint condition evaluation.\n"
		 "Using host evaluation mode instead."));
      return;
    }

  new_mode = translate_condition_evaluation_mode (condition_evaluation_mode_1);
  old_mode = translate_condition_evaluation_mode (condition_evaluation_mode);

  /* Flip the switch.  Flip it even if OLD_MODE == NEW_MODE as one of the
     settings was "auto".  */
  condition_evaluation_mode = condition_evaluation_mode_1;

  /* Only update the mode if the user picked a different one.  */
  if (new_mode != old_mode)
    {
      struct bp_location *loc, **loc_tmp;
      /* If the user switched to a different evaluation mode, we
	 need to synch the changes with the target as follows:

	 "host" -> "target": Send all (valid) conditions to the target.
	 "target" -> "host": Remove all the conditions from the target.
      */

      if (new_mode == condition_evaluation_target)
	{
	  /* Mark everything modified and synch conditions with the
	     target.  */
	  ALL_BP_LOCATIONS (loc, loc_tmp)
	    mark_breakpoint_location_modified (loc);
  	}
      else
	{
	  /* Manually mark non-duplicate locations to synch conditions
	     with the target.  We do this to remove all the conditions the
	     target knows about.  */
	  ALL_BP_LOCATIONS (loc, loc_tmp)
	    if (is_breakpoint (loc->owner) && loc->inserted)
	      loc->needs_update = 1;
	}

      /* Do the update.  */
      update_global_location_list (UGLL_MAY_INSERT);
    }

  return;
}

/* Shows the current mode of breakpoint condition evaluation.  Explicitly shows
   what "auto" is translating to.  */

static void
show_condition_evaluation_mode (struct ui_file *file, int from_tty,
				struct cmd_list_element *c, const char *value)
{
  if (condition_evaluation_mode == condition_evaluation_auto)
    fprintf_filtered (file,
		      _("Breakpoint condition evaluation "
			"mode is %s (currently %s).\n"),
		      value,
		      breakpoint_condition_evaluation_mode ());
  else
    fprintf_filtered (file, _("Breakpoint condition evaluation mode is %s.\n"),
		      value);
}

/* A comparison function for bp_location AP and BP that is used by
   bsearch.  This comparison function only cares about addresses, unlike
   the more general bp_location_compare function.  */

static int
bp_location_compare_addrs (const void *ap, const void *bp)
{
  const struct bp_location *a = *(const struct bp_location **) ap;
  const struct bp_location *b = *(const struct bp_location **) bp;

  if (a->address == b->address)
    return 0;
  else
    return ((a->address > b->address) - (a->address < b->address));
}

/* Helper function to skip all bp_locations with addresses
   less than ADDRESS.  It returns the first bp_location that
   is greater than or equal to ADDRESS.  If none is found, just
   return NULL.  */

static struct bp_location **
get_first_locp_gte_addr (CORE_ADDR address)
{
  struct bp_location dummy_loc;
  struct bp_location *dummy_locp = &dummy_loc;
  struct bp_location **locp_found = NULL;

  /* Initialize the dummy location's address field.  */
  memset (&dummy_loc, 0, sizeof (struct bp_location));
  dummy_loc.address = address;

  /* Find a close match to the first location at ADDRESS.  */
  locp_found = ((struct bp_location **)
		bsearch (&dummy_locp, bp_location, bp_location_count,
			 sizeof (struct bp_location **),
			 bp_location_compare_addrs));

  /* Nothing was found, nothing left to do.  */
  if (locp_found == NULL)
    return NULL;

  /* We may have found a location that is at ADDRESS but is not the first in the
     location's list.  Go backwards (if possible) and locate the first one.  */
  while ((locp_found - 1) >= bp_location
	 && (*(locp_found - 1))->address == address)
    locp_found--;

  return locp_found;
}

void
set_breakpoint_condition (struct breakpoint *b, const char *exp,
			  int from_tty)
{
  xfree (b->cond_string);
  b->cond_string = NULL;

  if (is_watchpoint (b))
    {
      struct watchpoint *w = (struct watchpoint *) b;

      xfree (w->cond_exp);
      w->cond_exp = NULL;
    }
  else
    {
      struct bp_location *loc;

      for (loc = b->loc; loc; loc = loc->next)
	{
	  xfree (loc->cond);
	  loc->cond = NULL;

	  /* No need to free the condition agent expression
	     bytecode (if we have one).  We will handle this
	     when we go through update_global_location_list.  */
	}
    }

  if (*exp == 0)
    {
      if (from_tty)
	printf_filtered (_("Breakpoint %d now unconditional.\n"), b->number);
    }
  else
    {
      const char *arg = exp;

      /* I don't know if it matters whether this is the string the user
	 typed in or the decompiled expression.  */
      b->cond_string = xstrdup (arg);
      b->condition_not_parsed = 0;

      if (is_watchpoint (b))
	{
	  struct watchpoint *w = (struct watchpoint *) b;

	  innermost_block = NULL;
	  arg = exp;
	  w->cond_exp = parse_exp_1 (&arg, 0, 0, 0);
	  if (*arg)
	    error (_("Junk at end of expression"));
	  w->cond_exp_valid_block = innermost_block;
	}
      else
	{
	  struct bp_location *loc;

	  for (loc = b->loc; loc; loc = loc->next)
	    {
	      arg = exp;
	      loc->cond =
		parse_exp_1 (&arg, loc->address,
			     block_for_pc (loc->address), 0);
	      if (*arg)
		error (_("Junk at end of expression"));
	    }
	}
    }
  mark_breakpoint_modified (b);

  observer_notify_breakpoint_modified (b);
}

/* Completion for the "condition" command.  */

static VEC (char_ptr) *
condition_completer (struct cmd_list_element *cmd,
		     const char *text, const char *word)
{
  const char *space;

  text = skip_spaces_const (text);
  space = skip_to_space_const (text);
  if (*space == '\0')
    {
      int len;
      struct breakpoint *b;
      VEC (char_ptr) *result = NULL;

      if (text[0] == '$')
	{
	  /* We don't support completion of history indices.  */
	  if (isdigit (text[1]))
	    return NULL;
	  return complete_internalvar (&text[1]);
	}

      /* We're completing the breakpoint number.  */
      len = strlen (text);

      ALL_BREAKPOINTS (b)
	{
	  char number[50];

	  xsnprintf (number, sizeof (number), "%d", b->number);

	  if (strncmp (number, text, len) == 0)
	    VEC_safe_push (char_ptr, result, xstrdup (number));
	}

      return result;
    }

  /* We're completing the expression part.  */
  text = skip_spaces_const (space);
  return expression_completer (cmd, text, word);
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
	/* Check if this breakpoint has a "stop" method implemented in an
	   extension language.  This method and conditions entered into GDB
	   from the CLI are mutually exclusive.  */
	const struct extension_language_defn *extlang
	  = get_breakpoint_cond_ext_lang (b, EXT_LANG_NONE);

	if (extlang != NULL)
	  {
	    error (_("Only one stop condition allowed.  There is currently"
		     " a %s stop condition defined for this breakpoint."),
		   ext_lang_capitalized_name (extlang));
	  }
	set_breakpoint_condition (b, p, from_tty);

	if (is_breakpoint (b))
	  update_global_location_list (UGLL_MAY_INSERT);

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

static int
is_tracepoint_type (enum bptype type)
{
  return (type == bp_tracepoint
	  || type == bp_fast_tracepoint
	  || type == bp_static_tracepoint);
}

int
is_tracepoint (const struct breakpoint *b)
{
  return is_tracepoint_type (b->type);
}

/* A helper function that validates that COMMANDS are valid for a
   breakpoint.  This function will throw an exception if a problem is
   found.  */

static void
validate_commands_for_breakpoint (struct breakpoint *b,
				  struct command_line *commands)
{
  if (is_tracepoint (b))
    {
      struct tracepoint *t = (struct tracepoint *) b;
      struct command_line *c;
      struct command_line *while_stepping = 0;

      /* Reset the while-stepping step count.  The previous commands
         might have included a while-stepping action, while the new
         ones might not.  */
      t->step_count = 0;

      /* We need to verify that each top-level element of commands is
	 valid for tracepoints, that there's at most one
	 while-stepping element, and that the while-stepping's body
	 has valid tracing commands excluding nested while-stepping.
	 We also need to validate the tracepoint action line in the
	 context of the tracepoint --- validate_actionline actually
	 has side effects, like setting the tracepoint's
	 while-stepping STEP_COUNT, in addition to checking if the
	 collect/teval actions parse and make sense in the
	 tracepoint's context.  */
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

	  validate_actionline (c->line, b);
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
  observer_notify_breakpoint_modified (b);
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
    observer_notify_breakpoint_modified (b);
}

/* Set the thread for this breakpoint.  If THREAD is -1, make the
   breakpoint work for any thread.  */

void
breakpoint_set_thread (struct breakpoint *b, int thread)
{
  int old_thread = b->thread;

  b->thread = thread;
  if (old_thread != thread)
    observer_notify_breakpoint_modified (b);
}

/* Set the task for this breakpoint.  If TASK is 0, make the
   breakpoint work for any task.  */

void
breakpoint_set_task (struct breakpoint *b, int task)
{
  int old_task = b->task;

  b->task = task;
  if (old_task != task)
    observer_notify_breakpoint_modified (b);
}

void
check_tracepoint_command (char *line, void *closure)
{
  struct breakpoint *b = (struct breakpoint *) closure;

  validate_actionline (line, b);
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
  struct commands_info *info = (struct commands_info *) data;

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
      observer_notify_breakpoint_modified (b);
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
    /* BL isn't valid, or doesn't shadow memory.  */
    return 0;
  return 1;
}

/* Update BUF, which is LEN bytes read from the target address
   MEMADDR, by replacing a memory breakpoint with its shadowed
   contents.

   If READBUF is not NULL, this buffer must not overlap with the of
   the breakpoint location's shadow_contents buffer.  Otherwise, a
   failed assertion internal error will be raised.  */

static void
one_breakpoint_xfer_memory (gdb_byte *readbuf, gdb_byte *writebuf,
			    const gdb_byte *writebuf_org,
			    ULONGEST memaddr, LONGEST len,
			    struct bp_target_info *target_info,
			    struct gdbarch *gdbarch)
{
  /* Now do full processing of the found relevant range of elements.  */
  CORE_ADDR bp_addr = 0;
  int bp_size = 0;
  int bptoffset = 0;

  if (!breakpoint_address_match (target_info->placed_address_space, 0,
				 current_program_space->aspace, 0))
    {
      /* The breakpoint is inserted in a different address space.  */
      return;
    }

  /* Addresses and length of the part of the breakpoint that
     we need to copy.  */
  bp_addr = target_info->placed_address;
  bp_size = target_info->shadow_len;

  if (bp_addr + bp_size <= memaddr)
    {
      /* The breakpoint is entirely before the chunk of memory we are
	 reading.  */
      return;
    }

  if (bp_addr >= memaddr + len)
    {
      /* The breakpoint is entirely after the chunk of memory we are
	 reading.  */
      return;
    }

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

  if (readbuf != NULL)
    {
      /* Verify that the readbuf buffer does not overlap with the
	 shadow_contents buffer.  */
      gdb_assert (target_info->shadow_contents >= readbuf + len
		  || readbuf >= (target_info->shadow_contents
				 + target_info->shadow_len));

      /* Update the read buffer with this inserted breakpoint's
	 shadow.  */
      memcpy (readbuf + bp_addr - memaddr,
	      target_info->shadow_contents + bptoffset, bp_size);
    }
  else
    {
      const unsigned char *bp;
      CORE_ADDR addr = target_info->reqstd_address;
      int placed_size;

      /* Update the shadow with what we want to write to memory.  */
      memcpy (target_info->shadow_contents + bptoffset,
	      writebuf_org + bp_addr - memaddr, bp_size);

      /* Determine appropriate breakpoint contents and size for this
	 address.  */
      bp = gdbarch_breakpoint_from_pc (gdbarch, &addr, &placed_size);

      /* Update the final write buffer with this inserted
	 breakpoint's INSN.  */
      memcpy (writebuf + bp_addr - memaddr, bp + bptoffset, bp_size);
    }
}

/* Update BUF, which is LEN bytes read from the target address MEMADDR,
   by replacing any memory breakpoints with their shadowed contents.

   If READBUF is not NULL, this buffer must not overlap with any of
   the breakpoint location's shadow_contents buffers.  Otherwise,
   a failed assertion internal error will be raised.

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
breakpoint_xfer_memory (gdb_byte *readbuf, gdb_byte *writebuf,
			const gdb_byte *writebuf_org,
			ULONGEST memaddr, LONGEST len)
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

    /* bp_location array has BL->OWNER always non-NULL.  */
    if (bl->owner->type == bp_none)
      warning (_("reading through apparently deleted breakpoint #%d?"),
	       bl->owner->number);

    /* Performance optimization: any further element can no longer affect BUF
       content.  */

    if (bl->address >= bp_location_placed_address_before_address_max
        && memaddr + len <= (bl->address
			     - bp_location_placed_address_before_address_max))
      break;

    if (!bp_location_has_shadow (bl))
      continue;

    one_breakpoint_xfer_memory (readbuf, writebuf, writebuf_org,
				memaddr, len, &bl->target_info, bl->gdbarch);
  }
}



/* Return true if BPT is either a software breakpoint or a hardware
   breakpoint.  */

int
is_breakpoint (const struct breakpoint *bpt)
{
  return (bpt->type == bp_breakpoint
	  || bpt->type == bp_hardware_breakpoint
	  || bpt->type == bp_dprintf);
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

int
is_watchpoint (const struct breakpoint *bpt)
{
  return (is_hardware_watchpoint (bpt)
	  || bpt->type == bp_watchpoint);
}

/* Returns true if the current thread and its running state are safe
   to evaluate or update watchpoint B.  Watchpoints on local
   expressions need to be evaluated in the context of the thread that
   was current when the watchpoint was created, and, that thread needs
   to be stopped to be able to select the correct frame context.
   Watchpoints on global expressions can be evaluated on any thread,
   and in any state.  It is presently left to the target allowing
   memory accesses when threads are running.  */

static int
watchpoint_in_thread_scope (struct watchpoint *b)
{
  return (b->base.pspace == current_program_space
	  && (ptid_equal (b->watchpoint_thread, null_ptid)
	      || (ptid_equal (inferior_ptid, b->watchpoint_thread)
		  && !is_executing (inferior_ptid))));
}

/* Set watchpoint B to disp_del_at_next_stop, even including its possible
   associated bp_watchpoint_scope breakpoint.  */

static void
watchpoint_del_at_next_stop (struct watchpoint *w)
{
  struct breakpoint *b = &w->base;

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

/* Extract a bitfield value from value VAL using the bit parameters contained in
   watchpoint W.  */

static struct value *
extract_bitfield_from_watchpoint_value (struct watchpoint *w, struct value *val)
{
  struct value *bit_val;

  if (val == NULL)
    return NULL;

  bit_val = allocate_value (value_type (val));

  unpack_value_bitfield (bit_val,
			 w->val_bitpos,
			 w->val_bitsize,
			 value_contents_for_printing (val),
			 value_offset (val),
			 val);

  return bit_val;
}

/* Allocate a dummy location and add it to B, which must be a software
   watchpoint.  This is required because even if a software watchpoint
   is not watching any memory, bpstat_stop_status requires a location
   to be able to report stops.  */

static void
software_watchpoint_add_no_memory_location (struct breakpoint *b,
					    struct program_space *pspace)
{
  gdb_assert (b->type == bp_watchpoint && b->loc == NULL);

  b->loc = allocate_bp_location (b);
  b->loc->pspace = pspace;
  b->loc->address = -1;
  b->loc->length = -1;
}

/* Returns true if B is a software watchpoint that is not watching any
   memory (e.g., "watch $pc").  */

static int
is_no_memory_software_watchpoint (struct breakpoint *b)
{
  return (b->type == bp_watchpoint
	  && b->loc != NULL
	  && b->loc->next == NULL
	  && b->loc->address == -1
	  && b->loc->length == -1);
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
update_watchpoint (struct watchpoint *b, int reparse)
{
  int within_current_scope;
  struct frame_id saved_frame_id;
  int frame_saved;

  /* If this is a local watchpoint, we only want to check if the
     watchpoint frame is in scope if the current thread is the thread
     that was used to create the watchpoint.  */
  if (!watchpoint_in_thread_scope (b))
    return;

  if (b->base.disposition == disp_del_at_next_stop)
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

      /* If we're at a point where the stack has been destroyed
	 (e.g. in a function epilogue), unwinding may not work
	 properly. Do not attempt to recreate locations at this
	 point.  See similar comments in watchpoint_check.  */
      if (gdbarch_stack_frame_destroyed_p (frame_arch, frame_pc))
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
  b->base.loc = NULL;

  if (within_current_scope && reparse)
    {
      const char *s;

      if (b->exp)
	{
	  xfree (b->exp);
	  b->exp = NULL;
	}
      s = b->exp_string_reparse ? b->exp_string_reparse : b->exp_string;
      b->exp = parse_exp_1 (&s, 0, b->exp_valid_block, 0);
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
      if (b->base.cond_string != NULL)
	{
	  if (b->cond_exp != NULL)
	    {
	      xfree (b->cond_exp);
	      b->cond_exp = NULL;
	    }

	  s = b->base.cond_string;
	  b->cond_exp = parse_exp_1 (&s, 0, b->cond_exp_valid_block, 0);
	}
    }

  /* If we failed to parse the expression, for example because
     it refers to a global variable in a not-yet-loaded shared library,
     don't try to insert watchpoint.  We don't automatically delete
     such watchpoint, though, since failure to parse expression
     is different from out-of-scope watchpoint.  */
  if (!target_has_execution)
    {
      /* Without execution, memory can't change.  No use to try and
	 set watchpoint locations.  The watchpoint will be reset when
	 the target gains execution, through breakpoint_re_set.  */
      if (!can_use_hw_watchpoints)
	{
	  if (b->base.ops->works_in_software_mode (&b->base))
	    b->base.type = bp_watchpoint;
	  else
	    error (_("Can't set read/access watchpoint when "
		     "hardware watchpoints are disabled."));
	}
    }
  else if (within_current_scope && b->exp)
    {
      int pc = 0;
      struct value *val_chain, *v, *result, *next;
      struct program_space *frame_pspace;

      fetch_subexp_value (b->exp, &pc, &v, &result, &val_chain, 0);

      /* Avoid setting b->val if it's already set.  The meaning of
	 b->val is 'the last value' user saw, and we should update
	 it only if we reported that last value to user.  As it
	 happens, the code that reports it updates b->val directly.
	 We don't keep track of the memory value for masked
	 watchpoints.  */
      if (!b->val_valid && !is_masked_watchpoint (&b->base))
	{
	  if (b->val_bitsize != 0)
	    {
	      v = extract_bitfield_from_watchpoint_value (b, v);
	      if (v != NULL)
		release_value (v);
	    }
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
		  enum target_hw_bp_type type;
		  struct bp_location *loc, **tmp;
		  int bitpos = 0, bitsize = 0;

		  if (value_bitsize (v) != 0)
		    {
		      /* Extract the bit parameters out from the bitfield
			 sub-expression.  */
		      bitpos = value_bitpos (v);
		      bitsize = value_bitsize (v);
		    }
		  else if (v == result && b->val_bitsize != 0)
		    {
		     /* If VAL_BITSIZE != 0 then RESULT is actually a bitfield
			lvalue whose bit parameters are saved in the fields
			VAL_BITPOS and VAL_BITSIZE.  */
		      bitpos = b->val_bitpos;
		      bitsize = b->val_bitsize;
		    }

		  addr = value_address (v);
		  if (bitsize != 0)
		    {
		      /* Skip the bytes that don't contain the bitfield.  */
		      addr += bitpos / 8;
		    }

		  type = hw_write;
		  if (b->base.type == bp_read_watchpoint)
		    type = hw_read;
		  else if (b->base.type == bp_access_watchpoint)
		    type = hw_access;

		  loc = allocate_bp_location (&b->base);
		  for (tmp = &(b->base.loc); *tmp != NULL; tmp = &((*tmp)->next))
		    ;
		  *tmp = loc;
		  loc->gdbarch = get_type_arch (value_type (v));

		  loc->pspace = frame_pspace;
		  loc->address = addr;

		  if (bitsize != 0)
		    {
		      /* Just cover the bytes that make up the bitfield.  */
		      loc->length = ((bitpos % 8) + bitsize + 7) / 8;
		    }
		  else
		    loc->length = TYPE_LENGTH (value_type (v));

		  loc->watchpoint_type = type;
		}
	    }
	}

      /* Change the type of breakpoint between hardware assisted or
	 an ordinary watchpoint depending on the hardware support
	 and free hardware slots.  REPARSE is set when the inferior
	 is started.  */
      if (reparse)
	{
	  int reg_cnt;
	  enum bp_loc_type loc_type;
	  struct bp_location *bl;

	  reg_cnt = can_use_hardware_watchpoint (val_chain);

	  if (reg_cnt)
	    {
	      int i, target_resources_ok, other_type_used;
	      enum bptype type;

	      /* Use an exact watchpoint when there's only one memory region to be
		 watched, and only one debug register is needed to watch it.  */
	      b->exact = target_exact_watchpoints && reg_cnt == 1;

	      /* We need to determine how many resources are already
		 used for all other hardware watchpoints plus this one
		 to see if we still have enough resources to also fit
		 this watchpoint in as well.  */

	      /* If this is a software watchpoint, we try to turn it
		 to a hardware one -- count resources as if B was of
		 hardware watchpoint type.  */
	      type = b->base.type;
	      if (type == bp_watchpoint)
		type = bp_hardware_watchpoint;

	      /* This watchpoint may or may not have been placed on
		 the list yet at this point (it won't be in the list
		 if we're trying to create it for the first time,
		 through watch_command), so always account for it
		 manually.  */

	      /* Count resources used by all watchpoints except B.  */
	      i = hw_watchpoint_used_count_others (&b->base, type, &other_type_used);

	      /* Add in the resources needed for B.  */
	      i += hw_watchpoint_use_count (&b->base);

	      target_resources_ok
		= target_can_use_hardware_watchpoint (type, i, other_type_used);
	      if (target_resources_ok <= 0)
		{
		  int sw_mode = b->base.ops->works_in_software_mode (&b->base);

		  if (target_resources_ok == 0 && !sw_mode)
		    error (_("Target does not support this type of "
			     "hardware watchpoint."));
		  else if (target_resources_ok < 0 && !sw_mode)
		    error (_("There are not enough available hardware "
			     "resources for this watchpoint."));

		  /* Downgrade to software watchpoint.  */
		  b->base.type = bp_watchpoint;
		}
	      else
		{
		  /* If this was a software watchpoint, we've just
		     found we have enough resources to turn it to a
		     hardware watchpoint.  Otherwise, this is a
		     nop.  */
		  b->base.type = type;
		}
	    }
	  else if (!b->base.ops->works_in_software_mode (&b->base))
	    {
	      if (!can_use_hw_watchpoints)
		error (_("Can't set read/access watchpoint when "
			 "hardware watchpoints are disabled."));
	      else
		error (_("Expression cannot be implemented with "
			 "read/access watchpoint."));
	    }
	  else
	    b->base.type = bp_watchpoint;

	  loc_type = (b->base.type == bp_watchpoint? bp_loc_other
		      : bp_loc_hardware_watchpoint);
	  for (bl = b->base.loc; bl; bl = bl->next)
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
      if (b->base.type == bp_watchpoint && b->base.loc == NULL)
	software_watchpoint_add_no_memory_location (&b->base, frame_pspace);
    }
  else if (!within_current_scope)
    {
      printf_filtered (_("\
Watchpoint %d deleted because the program has left the block\n\
in which its expression is valid.\n"),
		       b->base.number);
      watchpoint_del_at_next_stop (b);
    }

  /* Restore the selected frame.  */
  if (frame_saved)
    select_frame (frame_find_by_id (saved_frame_id));
}


/* Returns 1 iff breakpoint location should be
   inserted in the inferior.  We don't differentiate the type of BL's owner
   (breakpoint vs. tracepoint), although insert_location in tracepoint's
   breakpoint_ops is not defined, because in insert_bp_location,
   tracepoint's insert_location will not be called.  */
static int
should_be_inserted (struct bp_location *bl)
{
  if (bl->owner == NULL || !breakpoint_enabled (bl->owner))
    return 0;

  if (bl->owner->disposition == disp_del_at_next_stop)
    return 0;

  if (!bl->enabled || bl->shlib_disabled || bl->duplicate)
    return 0;

  if (user_breakpoint_p (bl->owner) && bl->pspace->executing_startup)
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

  /* Don't insert a breakpoint if we're trying to step past its
     location, except if the breakpoint is a single-step breakpoint,
     and the breakpoint's thread is the thread which is stepping past
     a breakpoint.  */
  if ((bl->loc_type == bp_loc_software_breakpoint
       || bl->loc_type == bp_loc_hardware_breakpoint)
      && stepping_past_instruction_at (bl->pspace->aspace,
				       bl->address)
      /* The single-step breakpoint may be inserted at the location
	 we're trying to step if the instruction branches to itself.
	 However, the instruction won't be executed at all and it may
	 break the semantics of the instruction, for example, the
	 instruction is a conditional branch or updates some flags.
	 We can't fix it unless GDB is able to emulate the instruction
	 or switch to displaced stepping.  */
      && !(bl->owner->type == bp_single_step
	   && thread_is_stepping_over_breakpoint (bl->owner->thread)))
    {
      if (debug_infrun)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "infrun: skipping breakpoint: "
			      "stepping past insn at: %s\n",
			      paddress (bl->gdbarch, bl->address));
	}
      return 0;
    }

  /* Don't insert watchpoints if we're trying to step past the
     instruction that triggered one.  */
  if ((bl->loc_type == bp_loc_hardware_watchpoint)
      && stepping_past_nonsteppable_watchpoint ())
    {
      if (debug_infrun)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "infrun: stepping past non-steppable watchpoint. "
			      "skipping watchpoint at %s:%d\n",
			      paddress (bl->gdbarch, bl->address),
			      bl->length);
	}
      return 0;
    }

  return 1;
}

/* Same as should_be_inserted but does the check assuming
   that the location is not duplicated.  */

static int
unduplicated_should_be_inserted (struct bp_location *bl)
{
  int result;
  const int save_duplicate = bl->duplicate;

  bl->duplicate = 0;
  result = should_be_inserted (bl);
  bl->duplicate = save_duplicate;
  return result;
}

/* Parses a conditional described by an expression COND into an
   agent expression bytecode suitable for evaluation
   by the bytecode interpreter.  Return NULL if there was
   any error during parsing.  */

static struct agent_expr *
parse_cond_to_aexpr (CORE_ADDR scope, struct expression *cond)
{
  struct agent_expr *aexpr = NULL;

  if (!cond)
    return NULL;

  /* We don't want to stop processing, so catch any errors
     that may show up.  */
  TRY
    {
      aexpr = gen_eval_for_expr (scope, cond);
    }

  CATCH (ex, RETURN_MASK_ERROR)
    {
      /* If we got here, it means the condition could not be parsed to a valid
	 bytecode expression and thus can't be evaluated on the target's side.
	 It's no use iterating through the conditions.  */
      return NULL;
    }
  END_CATCH

  /* We have a valid agent expression.  */
  return aexpr;
}

/* Based on location BL, create a list of breakpoint conditions to be
   passed on to the target.  If we have duplicated locations with different
   conditions, we will add such conditions to the list.  The idea is that the
   target will evaluate the list of conditions and will only notify GDB when
   one of them is true.  */

static void
build_target_condition_list (struct bp_location *bl)
{
  struct bp_location **locp = NULL, **loc2p;
  int null_condition_or_parse_error = 0;
  int modified = bl->needs_update;
  struct bp_location *loc;

  /* Release conditions left over from a previous insert.  */
  VEC_free (agent_expr_p, bl->target_info.conditions);

  /* This is only meaningful if the target is
     evaluating conditions and if the user has
     opted for condition evaluation on the target's
     side.  */
  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return;

  /* Do a first pass to check for locations with no assigned
     conditions or conditions that fail to parse to a valid agent expression
     bytecode.  If any of these happen, then it's no use to send conditions
     to the target since this location will always trigger and generate a
     response back to GDB.  */
  ALL_BP_LOCATIONS_AT_ADDR (loc2p, locp, bl->address)
    {
      loc = (*loc2p);
      if (is_breakpoint (loc->owner) && loc->pspace->num == bl->pspace->num)
	{
	  if (modified)
	    {
	      struct agent_expr *aexpr;

	      /* Re-parse the conditions since something changed.  In that
		 case we already freed the condition bytecodes (see
		 force_breakpoint_reinsertion).  We just
		 need to parse the condition to bytecodes again.  */
	      aexpr = parse_cond_to_aexpr (bl->address, loc->cond);
	      loc->cond_bytecode = aexpr;
	    }

	  /* If we have a NULL bytecode expression, it means something
	     went wrong or we have a null condition expression.  */
	  if (!loc->cond_bytecode)
	    {
	      null_condition_or_parse_error = 1;
	      break;
	    }
	}
    }

  /* If any of these happened, it means we will have to evaluate the conditions
     for the location's address on gdb's side.  It is no use keeping bytecodes
     for all the other duplicate locations, thus we free all of them here.

     This is so we have a finer control over which locations' conditions are
     being evaluated by GDB or the remote stub.  */
  if (null_condition_or_parse_error)
    {
      ALL_BP_LOCATIONS_AT_ADDR (loc2p, locp, bl->address)
	{
	  loc = (*loc2p);
	  if (is_breakpoint (loc->owner) && loc->pspace->num == bl->pspace->num)
	    {
	      /* Only go as far as the first NULL bytecode is
		 located.  */
	      if (!loc->cond_bytecode)
		return;

	      free_agent_expr (loc->cond_bytecode);
	      loc->cond_bytecode = NULL;
	    }
	}
    }

  /* No NULL conditions or failed bytecode generation.  Build a condition list
     for this location's address.  */
  ALL_BP_LOCATIONS_AT_ADDR (loc2p, locp, bl->address)
    {
      loc = (*loc2p);
      if (loc->cond
	  && is_breakpoint (loc->owner)
	  && loc->pspace->num == bl->pspace->num
	  && loc->owner->enable_state == bp_enabled
	  && loc->enabled)
	/* Add the condition to the vector.  This will be used later to send the
	   conditions to the target.  */
	VEC_safe_push (agent_expr_p, bl->target_info.conditions,
		       loc->cond_bytecode);
    }

  return;
}

/* Parses a command described by string CMD into an agent expression
   bytecode suitable for evaluation by the bytecode interpreter.
   Return NULL if there was any error during parsing.  */

static struct agent_expr *
parse_cmd_to_aexpr (CORE_ADDR scope, char *cmd)
{
  struct cleanup *old_cleanups = 0;
  struct expression *expr, **argvec;
  struct agent_expr *aexpr = NULL;
  const char *cmdrest;
  const char *format_start, *format_end;
  struct format_piece *fpieces;
  int nargs;
  struct gdbarch *gdbarch = get_current_arch ();

  if (!cmd)
    return NULL;

  cmdrest = cmd;

  if (*cmdrest == ',')
    ++cmdrest;
  cmdrest = skip_spaces_const (cmdrest);

  if (*cmdrest++ != '"')
    error (_("No format string following the location"));

  format_start = cmdrest;

  fpieces = parse_format_string (&cmdrest);

  old_cleanups = make_cleanup (free_format_pieces_cleanup, &fpieces);

  format_end = cmdrest;

  if (*cmdrest++ != '"')
    error (_("Bad format string, non-terminated '\"'."));
  
  cmdrest = skip_spaces_const (cmdrest);

  if (!(*cmdrest == ',' || *cmdrest == '\0'))
    error (_("Invalid argument syntax"));

  if (*cmdrest == ',')
    cmdrest++;
  cmdrest = skip_spaces_const (cmdrest);

  /* For each argument, make an expression.  */

  argvec = (struct expression **) alloca (strlen (cmd)
					 * sizeof (struct expression *));

  nargs = 0;
  while (*cmdrest != '\0')
    {
      const char *cmd1;

      cmd1 = cmdrest;
      expr = parse_exp_1 (&cmd1, scope, block_for_pc (scope), 1);
      argvec[nargs++] = expr;
      cmdrest = cmd1;
      if (*cmdrest == ',')
	++cmdrest;
    }

  /* We don't want to stop processing, so catch any errors
     that may show up.  */
  TRY
    {
      aexpr = gen_printf (scope, gdbarch, 0, 0,
			  format_start, format_end - format_start,
			  fpieces, nargs, argvec);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      /* If we got here, it means the command could not be parsed to a valid
	 bytecode expression and thus can't be evaluated on the target's side.
	 It's no use iterating through the other commands.  */
      aexpr = NULL;
    }
  END_CATCH

  do_cleanups (old_cleanups);

  /* We have a valid agent expression, return it.  */
  return aexpr;
}

/* Based on location BL, create a list of breakpoint commands to be
   passed on to the target.  If we have duplicated locations with
   different commands, we will add any such to the list.  */

static void
build_target_command_list (struct bp_location *bl)
{
  struct bp_location **locp = NULL, **loc2p;
  int null_command_or_parse_error = 0;
  int modified = bl->needs_update;
  struct bp_location *loc;

  /* Release commands left over from a previous insert.  */
  VEC_free (agent_expr_p, bl->target_info.tcommands);

  if (!target_can_run_breakpoint_commands ())
    return;

  /* For now, limit to agent-style dprintf breakpoints.  */
  if (dprintf_style != dprintf_style_agent)
    return;

  /* For now, if we have any duplicate location that isn't a dprintf,
     don't install the target-side commands, as that would make the
     breakpoint not be reported to the core, and we'd lose
     control.  */
  ALL_BP_LOCATIONS_AT_ADDR (loc2p, locp, bl->address)
    {
      loc = (*loc2p);
      if (is_breakpoint (loc->owner)
	  && loc->pspace->num == bl->pspace->num
	  && loc->owner->type != bp_dprintf)
	return;
    }

  /* Do a first pass to check for locations with no assigned
     conditions or conditions that fail to parse to a valid agent expression
     bytecode.  If any of these happen, then it's no use to send conditions
     to the target since this location will always trigger and generate a
     response back to GDB.  */
  ALL_BP_LOCATIONS_AT_ADDR (loc2p, locp, bl->address)
    {
      loc = (*loc2p);
      if (is_breakpoint (loc->owner) && loc->pspace->num == bl->pspace->num)
	{
	  if (modified)
	    {
	      struct agent_expr *aexpr;

	      /* Re-parse the commands since something changed.  In that
		 case we already freed the command bytecodes (see
		 force_breakpoint_reinsertion).  We just
		 need to parse the command to bytecodes again.  */
	      aexpr = parse_cmd_to_aexpr (bl->address,
					  loc->owner->extra_string);
	      loc->cmd_bytecode = aexpr;
	    }

	  /* If we have a NULL bytecode expression, it means something
	     went wrong or we have a null command expression.  */
	  if (!loc->cmd_bytecode)
	    {
	      null_command_or_parse_error = 1;
	      break;
	    }
	}
    }

  /* If anything failed, then we're not doing target-side commands,
     and so clean up.  */
  if (null_command_or_parse_error)
    {
      ALL_BP_LOCATIONS_AT_ADDR (loc2p, locp, bl->address)
	{
	  loc = (*loc2p);
	  if (is_breakpoint (loc->owner)
	      && loc->pspace->num == bl->pspace->num)
	    {
	      /* Only go as far as the first NULL bytecode is
		 located.  */
	      if (loc->cmd_bytecode == NULL)
		return;

	      free_agent_expr (loc->cmd_bytecode);
	      loc->cmd_bytecode = NULL;
	    }
	}
    }

  /* No NULL commands or failed bytecode generation.  Build a command list
     for this location's address.  */
  ALL_BP_LOCATIONS_AT_ADDR (loc2p, locp, bl->address)
    {
      loc = (*loc2p);
      if (loc->owner->extra_string
	  && is_breakpoint (loc->owner)
	  && loc->pspace->num == bl->pspace->num
	  && loc->owner->enable_state == bp_enabled
	  && loc->enabled)
	/* Add the command to the vector.  This will be used later
	   to send the commands to the target.  */
	VEC_safe_push (agent_expr_p, bl->target_info.tcommands,
		       loc->cmd_bytecode);
    }

  bl->target_info.persist = 0;
  /* Maybe flag this location as persistent.  */
  if (bl->owner->type == bp_dprintf && disconnected_dprintf)
    bl->target_info.persist = 1;
}

/* Insert a low-level "breakpoint" of some type.  BL is the breakpoint
   location.  Any error messages are printed to TMP_ERROR_STREAM; and
   DISABLED_BREAKS, and HW_BREAKPOINT_ERROR are used to report problems.
   Returns 0 for success, 1 if the bp_location type is not supported or
   -1 for failure.

   NOTE drow/2003-09-09: This routine could be broken down to an
   object-style method for each breakpoint or catchpoint type.  */
static int
insert_bp_location (struct bp_location *bl,
		    struct ui_file *tmp_error_stream,
		    int *disabled_breaks,
		    int *hw_breakpoint_error,
		    int *hw_bp_error_explained_already)
{
  enum errors bp_err = GDB_NO_ERROR;
  const char *bp_err_message = NULL;

  if (!should_be_inserted (bl) || (bl->inserted && !bl->needs_update))
    return 0;

  /* Note we don't initialize bl->target_info, as that wipes out
     the breakpoint location's shadow_contents if the breakpoint
     is still inserted at that location.  This in turn breaks
     target_read_memory which depends on these buffers when
     a memory read is requested at the breakpoint location:
     Once the target_info has been wiped, we fail to see that
     we have a breakpoint inserted at that address and thus
     read the breakpoint instead of returning the data saved in
     the breakpoint location's shadow contents.  */
  bl->target_info.reqstd_address = bl->address;
  bl->target_info.placed_address_space = bl->pspace->aspace;
  bl->target_info.length = bl->length;

  /* When working with target-side conditions, we must pass all the conditions
     for the same breakpoint address down to the target since GDB will not
     insert those locations.  With a list of breakpoint conditions, the target
     can decide when to stop and notify GDB.  */

  if (is_breakpoint (bl->owner))
    {
      build_target_condition_list (bl);
      build_target_command_list (bl);
      /* Reset the modification marker.  */
      bl->needs_update = 0;
    }

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
	    = lookup_mem_region (bl->target_info.reqstd_address);
	  
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
		{
		  fprintf_unfiltered (tmp_error_stream,
				      _("Cannot insert breakpoint %d.\n"
					"Cannot set software breakpoint "
					"at read-only address %s\n"),
				      bl->owner->number,
				      paddress (bl->gdbarch, bl->address));
		  return 1;
		}
	    }
	}
        
      /* First check to see if we have to handle an overlay.  */
      if (overlay_debugging == ovly_off
	  || bl->section == NULL
	  || !(section_is_overlay (bl->section)))
	{
	  /* No overlay handling: just set the breakpoint.  */
	  TRY
	    {
	      int val;

	      val = bl->owner->ops->insert_location (bl);
	      if (val)
		bp_err = GENERIC_ERROR;
	    }
	  CATCH (e, RETURN_MASK_ALL)
	    {
	      bp_err = e.error;
	      bp_err_message = e.message;
	    }
	  END_CATCH
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
		  bl->overlay_target_info.reqstd_address = addr;

		  /* No overlay handling: just set the breakpoint.  */
		  TRY
		    {
		      int val;

		      val = target_insert_breakpoint (bl->gdbarch,
						      &bl->overlay_target_info);
		      if (val)
			bp_err = GENERIC_ERROR;
		    }
		  CATCH (e, RETURN_MASK_ALL)
		    {
		      bp_err = e.error;
		      bp_err_message = e.message;
		    }
		  END_CATCH

		  if (bp_err != GDB_NO_ERROR)
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
	      TRY
	        {
		  int val;

	          val = bl->owner->ops->insert_location (bl);
		  if (val)
		    bp_err = GENERIC_ERROR;
	        }
	      CATCH (e, RETURN_MASK_ALL)
	        {
		  bp_err = e.error;
		  bp_err_message = e.message;
	        }
	      END_CATCH
	    }
	  else
	    {
	      /* No.  This breakpoint will not be inserted.  
		 No error, but do not mark the bp as 'inserted'.  */
	      return 0;
	    }
	}

      if (bp_err != GDB_NO_ERROR)
	{
	  /* Can't set the breakpoint.  */

	  /* In some cases, we might not be able to insert a
	     breakpoint in a shared library that has already been
	     removed, but we have not yet processed the shlib unload
	     event.  Unfortunately, some targets that implement
	     breakpoint insertion themselves can't tell why the
	     breakpoint insertion failed (e.g., the remote target
	     doesn't define error codes), so we must treat generic
	     errors as memory errors.  */
	  if ((bp_err == GENERIC_ERROR || bp_err == MEMORY_ERROR)
	      && bl->loc_type == bp_loc_software_breakpoint
	      && (solib_name_from_address (bl->pspace, bl->address)
		  || shared_objfile_contains_address_p (bl->pspace,
							bl->address)))
	    {
	      /* See also: disable_breakpoints_in_shlibs.  */
	      bl->shlib_disabled = 1;
	      observer_notify_breakpoint_modified (bl->owner);
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
	      return 0;
	    }
	  else
	    {
	      if (bl->loc_type == bp_loc_hardware_breakpoint)
		{
		  *hw_breakpoint_error = 1;
		  *hw_bp_error_explained_already = bp_err_message != NULL;
                  fprintf_unfiltered (tmp_error_stream,
                                      "Cannot insert hardware breakpoint %d%s",
                                      bl->owner->number, bp_err_message ? ":" : ".\n");
                  if (bp_err_message != NULL)
                    fprintf_unfiltered (tmp_error_stream, "%s.\n", bp_err_message);
		}
	      else
		{
		  if (bp_err_message == NULL)
		    {
		      char *message
			= memory_error_message (TARGET_XFER_E_IO,
						bl->gdbarch, bl->address);
		      struct cleanup *old_chain = make_cleanup (xfree, message);

		      fprintf_unfiltered (tmp_error_stream,
					  "Cannot insert breakpoint %d.\n"
					  "%s\n",
					  bl->owner->number, message);
		      do_cleanups (old_chain);
		    }
		  else
		    {
		      fprintf_unfiltered (tmp_error_stream,
					  "Cannot insert breakpoint %d: %s\n",
					  bl->owner->number,
					  bp_err_message);
		    }
		}
	      return 1;

	    }
	}
      else
	bl->inserted = 1;

      return 0;
    }

  else if (bl->loc_type == bp_loc_hardware_watchpoint
	   /* NOTE drow/2003-09-08: This state only exists for removing
	      watchpoints.  It's not clear that it's necessary...  */
	   && bl->owner->disposition != disp_del_at_next_stop)
    {
      int val;

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
      int val;

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
  update_global_location_list (UGLL_DONT_INSERT);
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
      {
	struct watchpoint *w = (struct watchpoint *) bpt;

	update_watchpoint (w, 0 /* don't reparse.  */);
      }

  /* Updating watchpoints creates new locations, so update the global
     location list.  Explicitly tell ugll to insert locations and
     ignore breakpoints_always_inserted_mode.  */
  update_global_location_list (UGLL_INSERT);
}

/* Invoke CALLBACK for each of bp_location.  */

void
iterate_over_bp_locations (walk_bp_location_callback callback)
{
  struct bp_location *loc, **loc_tmp;

  ALL_BP_LOCATIONS (loc, loc_tmp)
    {
      callback (loc, NULL);
    }
}

/* This is used when we need to synch breakpoint conditions between GDB and the
   target.  It is the case with deleting and disabling of breakpoints when using
   always-inserted mode.  */

static void
update_inserted_breakpoint_locations (void)
{
  struct bp_location *bl, **blp_tmp;
  int error_flag = 0;
  int val = 0;
  int disabled_breaks = 0;
  int hw_breakpoint_error = 0;
  int hw_bp_details_reported = 0;

  struct ui_file *tmp_error_stream = mem_fileopen ();
  struct cleanup *cleanups = make_cleanup_ui_file_delete (tmp_error_stream);

  /* Explicitly mark the warning -- this will only be printed if
     there was an error.  */
  fprintf_unfiltered (tmp_error_stream, "Warning:\n");

  save_current_space_and_thread ();

  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      /* We only want to update software breakpoints and hardware
	 breakpoints.  */
      if (!is_breakpoint (bl->owner))
	continue;

      /* We only want to update locations that are already inserted
	 and need updating.  This is to avoid unwanted insertion during
	 deletion of breakpoints.  */
      if (!bl->inserted || (bl->inserted && !bl->needs_update))
	continue;

      switch_to_program_space_and_thread (bl->pspace);

      /* For targets that support global breakpoints, there's no need
	 to select an inferior to insert breakpoint to.  In fact, even
	 if we aren't attached to any process yet, we should still
	 insert breakpoints.  */
      if (!gdbarch_has_global_breakpoints (target_gdbarch ())
	  && ptid_equal (inferior_ptid, null_ptid))
	continue;

      val = insert_bp_location (bl, tmp_error_stream, &disabled_breaks,
				    &hw_breakpoint_error, &hw_bp_details_reported);
      if (val)
	error_flag = val;
    }

  if (error_flag)
    {
      target_terminal_ours_for_output ();
      error_stream (tmp_error_stream);
    }

  do_cleanups (cleanups);
}

/* Used when starting or continuing the program.  */

static void
insert_breakpoint_locations (void)
{
  struct breakpoint *bpt;
  struct bp_location *bl, **blp_tmp;
  int error_flag = 0;
  int val = 0;
  int disabled_breaks = 0;
  int hw_breakpoint_error = 0;
  int hw_bp_error_explained_already = 0;

  struct ui_file *tmp_error_stream = mem_fileopen ();
  struct cleanup *cleanups = make_cleanup_ui_file_delete (tmp_error_stream);
  
  /* Explicitly mark the warning -- this will only be printed if
     there was an error.  */
  fprintf_unfiltered (tmp_error_stream, "Warning:\n");

  save_current_space_and_thread ();

  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      if (!should_be_inserted (bl) || (bl->inserted && !bl->needs_update))
	continue;

      /* There is no point inserting thread-specific breakpoints if
	 the thread no longer exists.  ALL_BP_LOCATIONS bp_location
	 has BL->OWNER always non-NULL.  */
      if (bl->owner->thread != -1
	  && !valid_global_thread_id (bl->owner->thread))
	continue;

      switch_to_program_space_and_thread (bl->pspace);

      /* For targets that support global breakpoints, there's no need
	 to select an inferior to insert breakpoint to.  In fact, even
	 if we aren't attached to any process yet, we should still
	 insert breakpoints.  */
      if (!gdbarch_has_global_breakpoints (target_gdbarch ())
	  && ptid_equal (inferior_ptid, null_ptid))
	continue;

      val = insert_bp_location (bl, tmp_error_stream, &disabled_breaks,
				    &hw_breakpoint_error, &hw_bp_error_explained_already);
      if (val)
	error_flag = val;
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
	      remove_breakpoint (loc);

	  hw_breakpoint_error = 1;
	  fprintf_unfiltered (tmp_error_stream,
			      "Could not insert hardware watchpoint %d.\n", 
			      bpt->number);
	  error_flag = -1;
	}
    }

  if (error_flag)
    {
      /* If a hardware breakpoint or watchpoint was inserted, add a
         message about possibly exhausted resources.  */
      if (hw_breakpoint_error && !hw_bp_error_explained_already)
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

/* Used when the program stops.
   Returns zero if successful, or non-zero if there was a problem
   removing a breakpoint location.  */

int
remove_breakpoints (void)
{
  struct bp_location *bl, **blp_tmp;
  int val = 0;

  ALL_BP_LOCATIONS (bl, blp_tmp)
  {
    if (bl->inserted && !is_tracepoint (bl->owner))
      val |= remove_breakpoint (bl);
  }
  return val;
}

/* When a thread exits, remove breakpoints that are related to
   that thread.  */

static void
remove_threaded_breakpoints (struct thread_info *tp, int silent)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    {
      if (b->thread == tp->global_num && user_breakpoint_p (b))
	{
	  b->disposition = disp_del_at_next_stop;

	  printf_filtered (_("\
Thread-specific breakpoint %d deleted - thread %s no longer in the thread list.\n"),
			   b->number, print_thread_id (tp));

	  /* Hide it from the user.  */
	  b->number = 0;
       }
    }
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

    if (bl->inserted && !bl->target_info.persist)
      {
	val = remove_breakpoint (bl);
	if (val != 0)
	  return val;
      }
  }
  return 0;
}

int
reattach_breakpoints (int pid)
{
  struct cleanup *old_chain;
  struct bp_location *bl, **blp_tmp;
  int val;
  struct ui_file *tmp_error_stream;
  int dummy1 = 0, dummy2 = 0, dummy3 = 0;
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
	val = insert_bp_location (bl, tmp_error_stream, &dummy1, &dummy2, &dummy3);
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
   Otherwise the breakpoint number will be populated from
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
			    CORE_ADDR address, enum bptype type,
			    const struct breakpoint_ops *ops)
{
  struct symtab_and_line sal;
  struct breakpoint *b;

  init_sal (&sal);		/* Initialize to zeroes.  */

  sal.pc = address;
  sal.section = find_pc_overlay (sal.pc);
  sal.pspace = current_program_space;

  b = set_raw_breakpoint (gdbarch, sal, type, ops);
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
  struct bound_minimal_symbol overlay_msym;

  /* Minimal symbol(s) for "longjmp", "siglongjmp", etc. (if any).  */
  struct bound_minimal_symbol longjmp_msym[NUM_LONGJMP_NAMES];

  /* True if we have looked for longjmp probes.  */
  int longjmp_searched;

  /* SystemTap probe points for longjmp (if any).  */
  VEC (probe_p) *longjmp_probes;

  /* Minimal symbol for "std::terminate()" (if any).  */
  struct bound_minimal_symbol terminate_msym;

  /* Minimal symbol for "_Unwind_DebugHook" (if any).  */
  struct bound_minimal_symbol exception_msym;

  /* True if we have looked for exception probes.  */
  int exception_searched;

  /* SystemTap probe points for unwinding (if any).  */
  VEC (probe_p) *exception_probes;
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

  bp_objfile_data = ((struct breakpoint_objfile_data *)
		     objfile_data (objfile, breakpoint_objfile_key));
  if (bp_objfile_data == NULL)
    {
      bp_objfile_data =
	XOBNEW (&objfile->objfile_obstack, struct breakpoint_objfile_data);

      memset (bp_objfile_data, 0, sizeof (*bp_objfile_data));
      set_objfile_data (objfile, breakpoint_objfile_key, bp_objfile_data);
    }
  return bp_objfile_data;
}

static void
free_breakpoint_probes (struct objfile *obj, void *data)
{
  struct breakpoint_objfile_data *bp_objfile_data
    = (struct breakpoint_objfile_data *) data;

  VEC_free (probe_p, bp_objfile_data->longjmp_probes);
  VEC_free (probe_p, bp_objfile_data->exception_probes);
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
      struct explicit_location explicit_loc;

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      if (msym_not_found_p (bp_objfile_data->overlay_msym.minsym))
	continue;

      if (bp_objfile_data->overlay_msym.minsym == NULL)
	{
	  struct bound_minimal_symbol m;

	  m = lookup_minimal_symbol_text (func_name, objfile);
	  if (m.minsym == NULL)
	    {
	      /* Avoid future lookups in this objfile.  */
	      bp_objfile_data->overlay_msym.minsym = &msym_not_found;
	      continue;
	    }
	  bp_objfile_data->overlay_msym = m;
	}

      addr = BMSYMBOL_VALUE_ADDRESS (bp_objfile_data->overlay_msym);
      b = create_internal_breakpoint (get_objfile_arch (objfile), addr,
                                      bp_overlay_event,
				      &internal_breakpoint_ops);
      initialize_explicit_location (&explicit_loc);
      explicit_loc.function_name = ASTRDUP (func_name);
      b->location = new_explicit_location (&explicit_loc);

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

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      if (!bp_objfile_data->longjmp_searched)
	{
	  VEC (probe_p) *ret;

	  ret = find_probes_in_objfile (objfile, "libc", "longjmp");
	  if (ret != NULL)
	    {
	      /* We are only interested in checking one element.  */
	      struct probe *p = VEC_index (probe_p, ret, 0);

	      if (!can_evaluate_probe_arguments (p))
		{
		  /* We cannot use the probe interface here, because it does
		     not know how to evaluate arguments.  */
		  VEC_free (probe_p, ret);
		  ret = NULL;
		}
	    }
	  bp_objfile_data->longjmp_probes = ret;
	  bp_objfile_data->longjmp_searched = 1;
	}

      if (bp_objfile_data->longjmp_probes != NULL)
	{
	  int i;
	  struct probe *probe;
	  struct gdbarch *gdbarch = get_objfile_arch (objfile);

	  for (i = 0;
	       VEC_iterate (probe_p,
			    bp_objfile_data->longjmp_probes,
			    i, probe);
	       ++i)
	    {
	      struct breakpoint *b;

	      b = create_internal_breakpoint (gdbarch,
					      get_probe_address (probe,
								 objfile),
					      bp_longjmp_master,
					      &internal_breakpoint_ops);
	      b->location
		= new_probe_location ("-probe-stap libc:longjmp");
	      b->enable_state = bp_disabled;
	    }

	  continue;
	}

      if (!gdbarch_get_longjmp_target_p (gdbarch))
	continue;

      for (i = 0; i < NUM_LONGJMP_NAMES; i++)
	{
	  struct breakpoint *b;
	  const char *func_name;
	  CORE_ADDR addr;
	  struct explicit_location explicit_loc;

	  if (msym_not_found_p (bp_objfile_data->longjmp_msym[i].minsym))
	    continue;

	  func_name = longjmp_names[i];
	  if (bp_objfile_data->longjmp_msym[i].minsym == NULL)
	    {
	      struct bound_minimal_symbol m;

	      m = lookup_minimal_symbol_text (func_name, objfile);
	      if (m.minsym == NULL)
		{
		  /* Prevent future lookups in this objfile.  */
		  bp_objfile_data->longjmp_msym[i].minsym = &msym_not_found;
		  continue;
		}
	      bp_objfile_data->longjmp_msym[i] = m;
	    }

	  addr = BMSYMBOL_VALUE_ADDRESS (bp_objfile_data->longjmp_msym[i]);
	  b = create_internal_breakpoint (gdbarch, addr, bp_longjmp_master,
					  &internal_breakpoint_ops);
	  initialize_explicit_location (&explicit_loc);
	  explicit_loc.function_name = ASTRDUP (func_name);
	  b->location = new_explicit_location (&explicit_loc);
	  b->enable_state = bp_disabled;
	}
    }
  }

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
      struct explicit_location explicit_loc;

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      if (msym_not_found_p (bp_objfile_data->terminate_msym.minsym))
	continue;

      if (bp_objfile_data->terminate_msym.minsym == NULL)
	{
	  struct bound_minimal_symbol m;

	  m = lookup_minimal_symbol (func_name, NULL, objfile);
	  if (m.minsym == NULL || (MSYMBOL_TYPE (m.minsym) != mst_text
				   && MSYMBOL_TYPE (m.minsym) != mst_file_text))
	    {
	      /* Prevent future lookups in this objfile.  */
	      bp_objfile_data->terminate_msym.minsym = &msym_not_found;
	      continue;
	    }
	  bp_objfile_data->terminate_msym = m;
	}

      addr = BMSYMBOL_VALUE_ADDRESS (bp_objfile_data->terminate_msym);
      b = create_internal_breakpoint (get_objfile_arch (objfile), addr,
                                      bp_std_terminate_master,
				      &internal_breakpoint_ops);
      initialize_explicit_location (&explicit_loc);
      explicit_loc.function_name = ASTRDUP (func_name);
      b->location = new_explicit_location (&explicit_loc);
      b->enable_state = bp_disabled;
    }
  }

  do_cleanups (old_chain);
}

/* Install a master breakpoint on the unwinder's debug hook.  */

static void
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
      struct explicit_location explicit_loc;

      bp_objfile_data = get_breakpoint_objfile_data (objfile);

      /* We prefer the SystemTap probe point if it exists.  */
      if (!bp_objfile_data->exception_searched)
	{
	  VEC (probe_p) *ret;

	  ret = find_probes_in_objfile (objfile, "libgcc", "unwind");

	  if (ret != NULL)
	    {
	      /* We are only interested in checking one element.  */
	      struct probe *p = VEC_index (probe_p, ret, 0);

	      if (!can_evaluate_probe_arguments (p))
		{
		  /* We cannot use the probe interface here, because it does
		     not know how to evaluate arguments.  */
		  VEC_free (probe_p, ret);
		  ret = NULL;
		}
	    }
	  bp_objfile_data->exception_probes = ret;
	  bp_objfile_data->exception_searched = 1;
	}

      if (bp_objfile_data->exception_probes != NULL)
	{
	  struct gdbarch *gdbarch = get_objfile_arch (objfile);
	  int i;
	  struct probe *probe;

	  for (i = 0;
	       VEC_iterate (probe_p,
			    bp_objfile_data->exception_probes,
			    i, probe);
	       ++i)
	    {
	      struct breakpoint *b;

	      b = create_internal_breakpoint (gdbarch,
					      get_probe_address (probe,
								 objfile),
					      bp_exception_master,
					      &internal_breakpoint_ops);
	      b->location
		= new_probe_location ("-probe-stap libgcc:unwind");
	      b->enable_state = bp_disabled;
	    }

	  continue;
	}

      /* Otherwise, try the hook function.  */

      if (msym_not_found_p (bp_objfile_data->exception_msym.minsym))
	continue;

      gdbarch = get_objfile_arch (objfile);

      if (bp_objfile_data->exception_msym.minsym == NULL)
	{
	  struct bound_minimal_symbol debug_hook;

	  debug_hook = lookup_minimal_symbol (func_name, NULL, objfile);
	  if (debug_hook.minsym == NULL)
	    {
	      bp_objfile_data->exception_msym.minsym = &msym_not_found;
	      continue;
	    }

	  bp_objfile_data->exception_msym = debug_hook;
	}

      addr = BMSYMBOL_VALUE_ADDRESS (bp_objfile_data->exception_msym);
      addr = gdbarch_convert_from_func_ptr_addr (gdbarch, addr,
						 &current_target);
      b = create_internal_breakpoint (gdbarch, addr, bp_exception_master,
				      &internal_breakpoint_ops);
      initialize_explicit_location (&explicit_loc);
      explicit_loc.function_name = ASTRDUP (func_name);
      b->location = new_explicit_location (&explicit_loc);
      b->enable_state = bp_disabled;
    }
}

/* Does B have a location spec?  */

static int
breakpoint_event_location_empty_p (const struct breakpoint *b)
{
  return b->location != NULL && event_location_empty_p (b->location);
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
    if (b->type == bp_step_resume || b->type == bp_hp_step_resume)
      {
	delete_breakpoint (b);
	continue;
      }

    /* Just like single-step breakpoints.  */
    if (b->type == bp_single_step)
      {
	delete_breakpoint (b);
	continue;
      }

    /* Longjmp and longjmp-resume breakpoints are also meaningless
       after an exec.  */
    if (b->type == bp_longjmp || b->type == bp_longjmp_resume
	|| b->type == bp_longjmp_call_dummy
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

       In the absence of a general solution for the "how do we know
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
    if (breakpoint_event_location_empty_p (b))
      {
	delete_breakpoint (b);
	continue;
      }
  }
}

int
detach_breakpoints (ptid_t ptid)
{
  struct bp_location *bl, **blp_tmp;
  int val = 0;
  struct cleanup *old_chain = save_inferior_ptid ();
  struct inferior *inf = current_inferior ();

  if (ptid_get_pid (ptid) == ptid_get_pid (inferior_ptid))
    error (_("Cannot detach breakpoints of inferior_ptid"));

  /* Set inferior_ptid; remove_breakpoint_1 uses this global.  */
  inferior_ptid = ptid;
  ALL_BP_LOCATIONS (bl, blp_tmp)
  {
    if (bl->pspace != inf->pspace)
      continue;

    /* This function must physically remove breakpoints locations
       from the specified ptid, without modifying the breakpoint
       package's state.  Locations of type bp_loc_other are only
       maintained at GDB side.  So, there is no need to remove
       these bp_loc_other locations.  Moreover, removing these
       would modify the breakpoint package's state.  */
    if (bl->loc_type == bp_loc_other)
      continue;

    if (bl->inserted)
      val |= remove_breakpoint_1 (bl, DETACH_BREAKPOINT);
  }

  do_cleanups (old_chain);
  return val;
}

/* Remove the breakpoint location BL from the current address space.
   Note that this is used to detach breakpoints from a child fork.
   When we get here, the child isn't in the inferior list, and neither
   do we have objects to represent its address space --- we should
   *not* look at bl->pspace->aspace here.  */

static int
remove_breakpoint_1 (struct bp_location *bl, enum remove_bp_reason reason)
{
  int val;

  /* BL is never in moribund_locations by our callers.  */
  gdb_assert (bl->owner != NULL);

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

	  /* If we're trying to uninsert a memory breakpoint that we
	     know is set in a dynamic object that is marked
	     shlib_disabled, then either the dynamic object was
	     removed with "remove-symbol-file" or with
	     "nosharedlibrary".  In the former case, we don't know
	     whether another dynamic object might have loaded over the
	     breakpoint's address -- the user might well let us know
	     about it next with add-symbol-file (the whole point of
	     add-symbol-file is letting the user manually maintain a
	     list of dynamically loaded objects).  If we have the
	     breakpoint's shadow memory, that is, this is a software
	     breakpoint managed by GDB, check whether the breakpoint
	     is still inserted in memory, to avoid overwriting wrong
	     code with stale saved shadow contents.  Note that HW
	     breakpoints don't have shadow memory, as they're
	     implemented using a mechanism that is not dependent on
	     being able to modify the target's memory, and as such
	     they should always be removed.  */
	  if (bl->shlib_disabled
	      && bl->target_info.shadow_len != 0
	      && !memory_validate_breakpoint (bl->gdbarch, &bl->target_info))
	    val = 0;
	  else
	    val = bl->owner->ops->remove_location (bl, reason);
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
					    &bl->overlay_target_info,
					    reason);
	      }
	  /* Did we set a breakpoint at the VMA? 
	     If so, we will have marked the breakpoint 'inserted'.  */
	  if (bl->inserted)
	    {
	      /* Yes -- remove it.  Previously we did not bother to
		 remove the breakpoint if the section had been
		 unmapped, but let's not rely on that being safe.  We
		 don't know what the overlay manager might do.  */

	      /* However, we should remove *software* breakpoints only
		 if the section is still mapped, or else we overwrite
		 wrong code with the saved shadow contents.  */
	      if (bl->loc_type == bp_loc_hardware_breakpoint
		  || section_is_mapped (bl->section))
		val = bl->owner->ops->remove_location (bl, reason);
	      else
		val = 0;
	    }
	  else
	    {
	      /* No -- not inserted, so no need to remove.  No error.  */
	      val = 0;
	    }
	}

      /* In some cases, we might not be able to remove a breakpoint in
	 a shared library that has already been removed, but we have
	 not yet processed the shlib unload event.  Similarly for an
	 unloaded add-symbol-file object - the user might not yet have
	 had the chance to remove-symbol-file it.  shlib_disabled will
	 be set if the library/object has already been removed, but
	 the breakpoint hasn't been uninserted yet, e.g., after
	 "nosharedlibrary" or "remove-symbol-file" with breakpoints
	 always-inserted mode.  */
      if (val
	  && (bl->loc_type == bp_loc_software_breakpoint
	      && (bl->shlib_disabled
		  || solib_name_from_address (bl->pspace, bl->address)
		  || shared_objfile_contains_address_p (bl->pspace,
							bl->address))))
	val = 0;

      if (val)
	return val;
      bl->inserted = (reason == DETACH_BREAKPOINT);
    }
  else if (bl->loc_type == bp_loc_hardware_watchpoint)
    {
      gdb_assert (bl->owner->ops != NULL
		  && bl->owner->ops->remove_location != NULL);

      bl->inserted = (reason == DETACH_BREAKPOINT);
      bl->owner->ops->remove_location (bl, reason);

      /* Failure to remove any of the hardware watchpoints comes here.  */
      if (reason == REMOVE_BREAKPOINT && bl->inserted)
	warning (_("Could not remove hardware watchpoint %d."),
		 bl->owner->number);
    }
  else if (bl->owner->type == bp_catchpoint
           && breakpoint_enabled (bl->owner)
           && !bl->duplicate)
    {
      gdb_assert (bl->owner->ops != NULL
		  && bl->owner->ops->remove_location != NULL);

      val = bl->owner->ops->remove_location (bl, reason);
      if (val)
	return val;

      bl->inserted = (reason == DETACH_BREAKPOINT);
    }

  return 0;
}

static int
remove_breakpoint (struct bp_location *bl)
{
  int ret;
  struct cleanup *old_chain;

  /* BL is never in moribund_locations by our callers.  */
  gdb_assert (bl->owner != NULL);

  /* The type of none suggests that owner is actually deleted.
     This should not ever happen.  */
  gdb_assert (bl->owner->type != bp_none);

  old_chain = save_current_space_and_thread ();

  switch_to_program_space_and_thread (bl->pspace);

  ret = remove_breakpoint_1 (bl, REMOVE_BREAKPOINT);

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
  struct bp_location *bl;
  int ix;
  struct program_space *pspace = current_program_space;

  /* If breakpoint locations are shared across processes, then there's
     nothing to do.  */
  if (gdbarch_has_global_breakpoints (target_gdbarch ()))
    return;

  mark_breakpoints_out ();

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
  {
    if (b->loc && b->loc->pspace != pspace)
      continue;

    switch (b->type)
      {
      case bp_call_dummy:
      case bp_longjmp_call_dummy:

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

      case bp_step_resume:

	/* Also remove step-resume breakpoints.  */

      case bp_single_step:

	/* Also remove single-step breakpoints.  */

	delete_breakpoint (b);
	break;

      case bp_watchpoint:
      case bp_hardware_watchpoint:
      case bp_read_watchpoint:
      case bp_access_watchpoint:
	{
	  struct watchpoint *w = (struct watchpoint *) b;

	  /* Likewise for watchpoints on local expressions.  */
	  if (w->exp_valid_block != NULL)
	    delete_breakpoint (b);
	  else
	    {
	      /* Get rid of existing locations, which are no longer
		 valid.  New ones will be created in
		 update_watchpoint, when the inferior is restarted.
		 The next update_global_location_list call will
		 garbage collect them.  */
	      b->loc = NULL;

	      if (context == inf_starting)
		{
		  /* Reset val field to force reread of starting value in
		     insert_breakpoints.  */
		  if (w->val)
		    value_free (w->val);
		  w->val = NULL;
		  w->val_valid = 0;
		}
	    }
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
   - When continuing from a location with a permanent breakpoint, we
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
	   || bl->permanent)
	  && breakpoint_location_address_match (bl, aspace, pc))
	{
	  if (overlay_debugging 
	      && section_is_overlay (bl->section)
	      && !section_is_mapped (bl->section))
	    continue;		/* unmapped overlay -- can't be a match */
	  else if (bl->permanent)
	    return permanent_breakpoint_here;
	  else
	    any_breakpoint_here = 1;
	}
    }

  return any_breakpoint_here ? ordinary_breakpoint_here : no_breakpoint_here;
}

/* See breakpoint.h.  */

int
breakpoint_in_range_p (struct address_space *aspace,
		       CORE_ADDR addr, ULONGEST len)
{
  struct bp_location *bl, **blp_tmp;

  ALL_BP_LOCATIONS (bl, blp_tmp)
    {
      if (bl->loc_type != bp_loc_software_breakpoint
	  && bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      if ((breakpoint_enabled (bl->owner)
	   || bl->permanent)
	  && breakpoint_location_address_range_overlap (bl, aspace,
							addr, len))
	{
	  if (overlay_debugging
	      && section_is_overlay (bl->section)
	      && !section_is_mapped (bl->section))
	    {
	      /* Unmapped overlay -- can't be a match.  */
	      continue;
	    }

	  return 1;
	}
    }

  return 0;
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

/* Returns non-zero iff BL is inserted at PC, in address space
   ASPACE.  */

static int
bp_location_inserted_here_p (struct bp_location *bl,
			     struct address_space *aspace, CORE_ADDR pc)
{
  if (bl->inserted
      && breakpoint_address_match (bl->pspace->aspace, bl->address,
				   aspace, pc))
    {
      if (overlay_debugging
	  && section_is_overlay (bl->section)
	  && !section_is_mapped (bl->section))
	return 0;		/* unmapped overlay -- can't be a match */
      else
	return 1;
    }
  return 0;
}

/* Returns non-zero iff there's a breakpoint inserted at PC.  */

int
breakpoint_inserted_here_p (struct address_space *aspace, CORE_ADDR pc)
{
  struct bp_location **blp, **blp_tmp = NULL;

  ALL_BP_LOCATIONS_AT_ADDR (blp, blp_tmp, pc)
    {
      struct bp_location *bl = *blp;

      if (bl->loc_type != bp_loc_software_breakpoint
	  && bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      if (bp_location_inserted_here_p (bl, aspace, pc))
	return 1;
    }
  return 0;
}

/* This function returns non-zero iff there is a software breakpoint
   inserted at PC.  */

int
software_breakpoint_inserted_here_p (struct address_space *aspace,
				     CORE_ADDR pc)
{
  struct bp_location **blp, **blp_tmp = NULL;

  ALL_BP_LOCATIONS_AT_ADDR (blp, blp_tmp, pc)
    {
      struct bp_location *bl = *blp;

      if (bl->loc_type != bp_loc_software_breakpoint)
	continue;

      if (bp_location_inserted_here_p (bl, aspace, pc))
	return 1;
    }

  return 0;
}

/* See breakpoint.h.  */

int
hardware_breakpoint_inserted_here_p (struct address_space *aspace,
				     CORE_ADDR pc)
{
  struct bp_location **blp, **blp_tmp = NULL;

  ALL_BP_LOCATIONS_AT_ADDR (blp, blp_tmp, pc)
    {
      struct bp_location *bl = *blp;

      if (bl->loc_type != bp_loc_hardware_breakpoint)
	continue;

      if (bp_location_inserted_here_p (bl, aspace, pc))
	return 1;
    }

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


/* bpstat stuff.  External routines' interfaces are documented
   in breakpoint.h.  */

int
is_catchpoint (struct breakpoint *ep)
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

/* See breakpoint.h.  */

int
bpstat_explains_signal (bpstat bsp, enum gdb_signal sig)
{
  for (; bsp != NULL; bsp = bsp->next)
    {
      if (bsp->breakpoint_at == NULL)
	{
	  /* A moribund location can never explain a signal other than
	     GDB_SIGNAL_TRAP.  */
	  if (sig == GDB_SIGNAL_TRAP)
	    return 1;
	}
      else
	{
	  if (bsp->breakpoint_at->ops->explains_signal (bsp->breakpoint_at,
							sig))
	    return 1;
	}
    }

  return 0;
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

/* See breakpoint.h.  */

void
bpstat_clear_actions (void)
{
  struct thread_info *tp;
  bpstat bs;

  if (ptid_equal (inferior_ptid, null_ptid))
    return;

  tp = find_thread_ptid (inferior_ptid);
  if (tp == NULL)
    return;

  for (bs = tp->control.stop_bpstat; bs != NULL; bs = bs->next)
    {
      decref_counted_command_line (&bs->commands);

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

/* Return non-zero iff CMD as the first line of a command sequence is `silent'
   or its equivalent.  */

static int
command_line_is_silent (struct command_line *cmd)
{
  return cmd && (strcmp ("silent", cmd->line) == 0);
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

  prevent_dont_repeat ();

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
      this_cmd_tree_chain = make_cleanup_decref_counted_command_line (&ccmd);
      cmd = ccmd ? ccmd->commands : NULL;
      if (command_line_is_silent (cmd))
	{
	  /* The action has been already done by bpstat_stop_status.  */
	  cmd = cmd->next;
	}

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
	  if (current_ui->async)
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
  struct cleanup *cleanup_if_error = make_bpstat_clear_actions_cleanup ();

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

  discard_cleanups (cleanup_if_error);
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

/* Print the "Thread ID hit" part of "Thread ID hit Breakpoint N" if
   debugging multiple threads.  */

void
maybe_print_thread_hit_breakpoint (struct ui_out *uiout)
{
  if (ui_out_is_mi_like_p (uiout))
    return;

  ui_out_text (uiout, "\n");

  if (show_thread_that_caused_stop ())
    {
      const char *name;
      struct thread_info *thr = inferior_thread ();

      ui_out_text (uiout, "Thread ");
      ui_out_field_fmt (uiout, "thread-id", "%s", print_thread_id (thr));

      name = thr->name != NULL ? thr->name : target_thread_name (thr);
      if (name != NULL)
	{
	  ui_out_text (uiout, " \"");
	  ui_out_field_fmt (uiout, "name", "%s", name);
	  ui_out_text (uiout, "\"");
	}

      ui_out_text (uiout, " hit ");
    }
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

	/* Normal case.  Call the breakpoint's print_it method.  */
	return b->ops->print_it (bs);
      }
      break;

    default:
      internal_error (__FILE__, __LINE__,
		      _("print_bp_stop_message: unrecognized enum value"));
      break;
    }
}

/* A helper function that prints a shared library stopped event.  */

static void
print_solib_event (int is_catchpoint)
{
  int any_deleted
    = !VEC_empty (char_ptr, current_program_space->deleted_solibs);
  int any_added
    = !VEC_empty (so_list_ptr, current_program_space->added_solibs);

  if (!is_catchpoint)
    {
      if (any_added || any_deleted)
	ui_out_text (current_uiout,
		     _("Stopped due to shared library event:\n"));
      else
	ui_out_text (current_uiout,
		     _("Stopped due to shared library event (no "
		       "libraries added or removed)\n"));
    }

  if (ui_out_is_mi_like_p (current_uiout))
    ui_out_field_string (current_uiout, "reason",
			 async_reason_lookup (EXEC_ASYNC_SOLIB_EVENT));

  if (any_deleted)
    {
      struct cleanup *cleanup;
      char *name;
      int ix;

      ui_out_text (current_uiout, _("  Inferior unloaded "));
      cleanup = make_cleanup_ui_out_list_begin_end (current_uiout,
						    "removed");
      for (ix = 0;
	   VEC_iterate (char_ptr, current_program_space->deleted_solibs,
			ix, name);
	   ++ix)
	{
	  if (ix > 0)
	    ui_out_text (current_uiout, "    ");
	  ui_out_field_string (current_uiout, "library", name);
	  ui_out_text (current_uiout, "\n");
	}

      do_cleanups (cleanup);
    }

  if (any_added)
    {
      struct so_list *iter;
      int ix;
      struct cleanup *cleanup;

      ui_out_text (current_uiout, _("  Inferior loaded "));
      cleanup = make_cleanup_ui_out_list_begin_end (current_uiout,
						    "added");
      for (ix = 0;
	   VEC_iterate (so_list_ptr, current_program_space->added_solibs,
			ix, iter);
	   ++ix)
	{
	  if (ix > 0)
	    ui_out_text (current_uiout, "    ");
	  ui_out_field_string (current_uiout, "library", iter->so_name);
	  ui_out_text (current_uiout, "\n");
	}

      do_cleanups (cleanup);
    }
}

/* Print a message indicating what happened.  This is called from
   normal_stop().  The input to this routine is the head of the bpstat
   list - a list of the eventpoints that caused this stop.  KIND is
   the target_waitkind for the stopping event.  This
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
bpstat_print (bpstat bs, int kind)
{
  enum print_stop_action val;

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

  /* If we had hit a shared library event breakpoint,
     print_bp_stop_message would print out this message.  If we hit an
     OS-level shared library event, do the same thing.  */
  if (kind == TARGET_WAITKIND_LOADED)
    {
      print_solib_event (0);
      return PRINT_NOTHING;
    }

  /* We reached the end of the chain, or we got a null BS to start
     with and nothing was printed.  */
  return PRINT_UNKNOWN;
}

/* Evaluate the expression EXP and return 1 if value is zero.
   This returns the inverse of the condition because it is called
   from catch_errors which returns 0 if an exception happened, and if an
   exception happens we want execution to stop.
   The argument is a "struct expression *" that has been cast to a
   "void *" to make it pass through catch_errors.  */

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
	  {
	    struct watchpoint *w = (struct watchpoint *) b;

	    w->watchpoint_triggered = watch_triggered_no;
	  }

      return 0;
    }

  if (!target_stopped_data_address (&current_target, &addr))
    {
      /* We were stopped by a watchpoint, but we don't know where.
	 Mark all watchpoints as unknown.  */
      ALL_BREAKPOINTS (b)
	if (is_hardware_watchpoint (b))
	  {
	    struct watchpoint *w = (struct watchpoint *) b;

	    w->watchpoint_triggered = watch_triggered_unknown;
	  }

      return 1;
    }

  /* The target could report the data address.  Mark watchpoints
     affected by this data address as triggered, and all others as not
     triggered.  */

  ALL_BREAKPOINTS (b)
    if (is_hardware_watchpoint (b))
      {
	struct watchpoint *w = (struct watchpoint *) b;
	struct bp_location *loc;

	w->watchpoint_triggered = watch_triggered_no;
	for (loc = b->loc; loc; loc = loc->next)
	  {
	    if (is_masked_watchpoint (b))
	      {
		CORE_ADDR newaddr = addr & w->hw_wp_mask;
		CORE_ADDR start = loc->address & w->hw_wp_mask;

		if (newaddr == start)
		  {
		    w->watchpoint_triggered = watch_triggered_yes;
		    break;
		  }
	      }
	    /* Exact match not required.  Within range is sufficient.  */
	    else if (target_watchpoint_addr_within_range (&current_target,
							 addr, loc->address,
							 loc->length))
	      {
		w->watchpoint_triggered = watch_triggered_yes;
		break;
	      }
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
  struct watchpoint *b;
  struct frame_info *fr;
  int within_current_scope;

  /* BS is built from an existing struct breakpoint.  */
  gdb_assert (bs->breakpoint_at != NULL);
  b = (struct watchpoint *) bs->breakpoint_at;

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

      /* stack_frame_destroyed_p() returns a non-zero value if we're
	 still in the function but the stack frame has already been
	 invalidated.  Since we can't rely on the values of local
	 variables after the stack has been destroyed, we are treating
	 the watchpoint in that state as `not changed' without further
	 checking.  Don't mark watchpoints as changed if the current
	 frame is in an epilogue - even if they are in some other
	 frame, our view of the stack is likely to be wrong and
	 frame_find_by_id could error out.  */
      if (gdbarch_stack_frame_destroyed_p (frame_arch, frame_pc))
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
      struct value *mark;
      struct value *new_val;

      if (is_masked_watchpoint (&b->base))
	/* Since we don't know the exact trigger address (from
	   stopped_data_address), just tell the user we've triggered
	   a mask watchpoint.  */
	return WP_VALUE_CHANGED;

      mark = value_mark ();
      fetch_subexp_value (b->exp, &pc, &new_val, NULL, NULL, 0);

      if (b->val_bitsize != 0)
	new_val = extract_bitfield_from_watchpoint_value (b, new_val);

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
      struct switch_thru_all_uis state;

      /* This seems like the only logical thing to do because
         if we temporarily ignored the watchpoint, then when
         we reenter the block in which it is valid it contains
         garbage (in the case of a function, it may have two
         garbage values, one before and one after the prologue).
         So we can't even detect the first assignment to it and
         watch after that (since the garbage may or may not equal
         the first value assigned).  */
      /* We print all the stop information in
	 breakpoint_ops->print_it, but in this case, by the time we
	 call breakpoint_ops->print_it this bp will be deleted
	 already.  So we have no choice but print the information
	 here.  */

      SWITCH_THRU_ALL_UIS (state)
        {
	  struct ui_out *uiout = current_uiout;

	  if (ui_out_is_mi_like_p (uiout))
	    ui_out_field_string
	      (uiout, "reason", async_reason_lookup (EXEC_ASYNC_WATCHPOINT_SCOPE));
	  ui_out_text (uiout, "\nWatchpoint ");
	  ui_out_field_int (uiout, "wpnum", b->base.number);
	  ui_out_text (uiout,
		       " deleted because the program has left the block in\n"
		       "which its expression is valid.\n");
	}

      /* Make sure the watchpoint's commands aren't executed.  */
      decref_counted_command_line (&b->base.commands);
      watchpoint_del_at_next_stop (b);

      return WP_DELETED;
    }
}

/* Return true if it looks like target has stopped due to hitting
   breakpoint location BL.  This function does not check if we should
   stop, only if BL explains the stop.  */

static int
bpstat_check_location (const struct bp_location *bl,
		       struct address_space *aspace, CORE_ADDR bp_addr,
		       const struct target_waitstatus *ws)
{
  struct breakpoint *b = bl->owner;

  /* BL is from an existing breakpoint.  */
  gdb_assert (b != NULL);

  return b->ops->breakpoint_hit (bl, aspace, bp_addr, ws);
}

/* Determine if the watched values have actually changed, and we
   should stop.  If not, set BS->stop to 0.  */

static void
bpstat_check_watchpoint (bpstat bs)
{
  const struct bp_location *bl;
  struct watchpoint *b;

  /* BS is built for existing struct breakpoint.  */
  bl = bs->bp_location_at;
  gdb_assert (bl != NULL);
  b = (struct watchpoint *) bs->breakpoint_at;
  gdb_assert (b != NULL);

    {
      int must_check_value = 0;
      
      if (b->base.type == bp_watchpoint)
	/* For a software watchpoint, we must always check the
	   watched value.  */
	must_check_value = 1;
      else if (b->watchpoint_triggered == watch_triggered_yes)
	/* We have a hardware watchpoint (read, write, or access)
	   and the target earlier reported an address watched by
	   this watchpoint.  */
	must_check_value = 1;
      else if (b->watchpoint_triggered == watch_triggered_unknown
	       && b->base.type == bp_hardware_watchpoint)
	/* We were stopped by a hardware watchpoint, but the target could
	   not report the data address.  We must check the watchpoint's
	   value.  Access and read watchpoints are out of luck; without
	   a data address, we can't figure it out.  */
	must_check_value = 1;

      if (must_check_value)
	{
	  char *message
	    = xstrprintf ("Error evaluating expression for watchpoint %d\n",
			  b->base.number);
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
	      if (b->base.type == bp_read_watchpoint)
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
			if (other_b->type == bp_hardware_watchpoint
			    || other_b->type == bp_access_watchpoint)
			  {
			    struct watchpoint *other_w =
			      (struct watchpoint *) other_b;

			    if (other_w->watchpoint_triggered
				== watch_triggered_yes)
			      {
				other_write_watchpoint = 1;
				break;
			      }
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
	      if (b->base.type == bp_hardware_watchpoint
		  || b->base.type == bp_watchpoint)
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
	      {
		struct switch_thru_all_uis state;

		SWITCH_THRU_ALL_UIS (state)
	          {
		    printf_filtered (_("Watchpoint %d deleted.\n"),
				     b->base.number);
		  }
		watchpoint_del_at_next_stop (b);
		/* We've already printed what needs to be printed.  */
		bs->print_it = print_it_done;
	      }
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

/* For breakpoints that are currently marked as telling gdb to stop,
   check conditions (condition proper, frame, thread and ignore count)
   of breakpoint referred to by BS.  If we should not stop for this
   breakpoint, set BS->stop to 0.  */

static void
bpstat_check_breakpoint_conditions (bpstat bs, ptid_t ptid)
{
  const struct bp_location *bl;
  struct breakpoint *b;
  int value_is_zero = 0;
  struct expression *cond;

  gdb_assert (bs->stop);

  /* BS is built for existing struct breakpoint.  */
  bl = bs->bp_location_at;
  gdb_assert (bl != NULL);
  b = bs->breakpoint_at;
  gdb_assert (b != NULL);

  /* Even if the target evaluated the condition on its end and notified GDB, we
     need to do so again since GDB does not know if we stopped due to a
     breakpoint or a single step breakpoint.  */

  if (frame_id_p (b->frame_id)
      && !frame_id_eq (b->frame_id, get_stack_frame_id (get_current_frame ())))
    {
      bs->stop = 0;
      return;
    }

  /* If this is a thread/task-specific breakpoint, don't waste cpu
     evaluating the condition if this isn't the specified
     thread/task.  */
  if ((b->thread != -1 && b->thread != ptid_to_global_thread_id (ptid))
      || (b->task != 0 && b->task != ada_get_task_number (ptid)))

    {
      bs->stop = 0;
      return;
    }

  /* Evaluate extension language breakpoints that have a "stop" method
     implemented.  */
  bs->stop = breakpoint_ext_lang_cond_says_stop (b);

  if (is_watchpoint (b))
    {
      struct watchpoint *w = (struct watchpoint *) b;

      cond = w->cond_exp;
    }
  else
    cond = bl->cond;

  if (cond && b->disposition != disp_del_at_next_stop)
    {
      int within_current_scope = 1;
      struct watchpoint * w;

      /* We use value_mark and value_free_to_mark because it could
	 be a long time before we return to the command level and
	 call free_all_values.  We can't call free_all_values
	 because we might be in the middle of evaluating a
	 function call.  */
      struct value *mark = value_mark ();

      if (is_watchpoint (b))
	w = (struct watchpoint *) b;
      else
	w = NULL;

      /* Need to select the frame, with all that implies so that
	 the conditions will have the right context.  Because we
	 use the frame, we will not see an inlined function's
	 variables when we arrive at a breakpoint at the start
	 of the inlined function; the current frame will be the
	 call site.  */
      if (w == NULL || w->cond_exp_valid_block == NULL)
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
	  frame = block_innermost_frame (w->cond_exp_valid_block);
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
  else if (b->ignore_count > 0)
    {
      b->ignore_count--;
      bs->stop = 0;
      /* Increase the hit count even though we don't stop.  */
      ++(b->hit_count);
      observer_notify_breakpoint_modified (b);
    }	
}

/* Returns true if we need to track moribund locations of LOC's type
   on the current target.  */

static int
need_moribund_for_location_type (struct bp_location *loc)
{
  return ((loc->loc_type == bp_loc_software_breakpoint
	   && !target_supports_stopped_by_sw_breakpoint ())
	  || (loc->loc_type == bp_loc_hardware_breakpoint
	      && !target_supports_stopped_by_hw_breakpoint ()));
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
		    CORE_ADDR bp_addr, ptid_t ptid,
		    const struct target_waitstatus *ws)
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
      if (!breakpoint_enabled (b))
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

	  if (!bl->enabled || bl->shlib_disabled)
	    continue;

	  if (!bpstat_check_location (bl, aspace, bp_addr, ws))
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
	    {
	      struct watchpoint *w = (struct watchpoint *) b->related_breakpoint;

	      w->watchpoint_triggered = watch_triggered_yes;
	    }
	}
    }

  /* Check if a moribund breakpoint explains the stop.  */
  if (!target_supports_stopped_by_sw_breakpoint ()
      || !target_supports_stopped_by_hw_breakpoint ())
    {
      for (ix = 0; VEC_iterate (bp_location_p, moribund_locations, ix, loc); ++ix)
	{
	  if (breakpoint_location_address_match (loc, aspace, bp_addr)
	      && need_moribund_for_location_type (loc))
	    {
	      bs = bpstat_alloc (loc, &bs_link);
	      /* For hits of moribund locations, we should just proceed.  */
	      bs->stop = 0;
	      bs->print = 0;
	      bs->print_it = print_it_noop;
	    }
	}
    }

  /* A bit of special processing for shlib breakpoints.  We need to
     process solib loading here, so that the lists of loaded and
     unloaded libraries are correct before we handle "catch load" and
     "catch unload".  */
  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      if (bs->breakpoint_at && bs->breakpoint_at->type == bp_shlib_event)
	{
	  handle_solib_event ();
	  break;
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

      b = bs->breakpoint_at;
      b->ops->check_status (bs);
      if (bs->stop)
	{
	  bpstat_check_breakpoint_conditions (bs, ptid);

	  if (bs->stop)
	    {
	      ++(b->hit_count);
	      observer_notify_breakpoint_modified (b);

	      /* We will stop here.  */
	      if (b->disposition == disp_disable)
		{
		  --(b->enable_count);
		  if (b->enable_count <= 0)
		    b->enable_state = bp_disabled;
		  removed_any = 1;
		}
	      if (b->silent)
		bs->print = 0;
	      bs->commands = b->commands;
	      incref_counted_command_line (bs->commands);
	      if (command_line_is_silent (bs->commands
					  ? bs->commands->commands : NULL))
		bs->print = 0;

	      b->ops->after_condition_true (bs);
	    }

	}

      /* Print nothing for this entry if we don't stop or don't
	 print.  */
      if (!bs->stop || !bs->print)
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
	  struct watchpoint *w = (struct watchpoint *) bs->breakpoint_at;

	  update_watchpoint (w, 0 /* don't reparse.  */);
	  need_remove_insert = 1;
	}

  if (need_remove_insert)
    update_global_location_list (UGLL_MAY_INSERT);
  else if (removed_any)
    update_global_location_list (UGLL_DONT_INSERT);

  return bs_head;
}

static void
handle_jit_event (void)
{
  struct frame_info *frame;
  struct gdbarch *gdbarch;

  if (debug_infrun)
    fprintf_unfiltered (gdb_stdlog, "handling bp_jit_event\n");

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
      else
	bptype = bs->breakpoint_at->type;

      switch (bptype)
	{
	case bp_none:
	  break;
	case bp_breakpoint:
	case bp_hardware_breakpoint:
	case bp_single_step:
	case bp_until:
	case bp_finish:
	case bp_shlib_event:
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
	case bp_longjmp_call_dummy:
	case bp_exception:
	  if (bs->stop)
	    {
	      this_action = BPSTAT_WHAT_SET_LONGJMP_RESUME;
	      retval.is_longjmp = bptype != bp_exception;
	    }
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
	  break;
	case bp_longjmp_resume:
	case bp_exception_resume:
	  if (bs->stop)
	    {
	      this_action = BPSTAT_WHAT_CLEAR_LONGJMP_RESUME;
	      retval.is_longjmp = bptype == bp_longjmp_resume;
	    }
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
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
	case bp_hp_step_resume:
	  if (bs->stop)
	    this_action = BPSTAT_WHAT_HP_STEP_RESUME;
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
	case bp_jit_event:
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

	case bp_dprintf:
	  if (bs->stop)
	    this_action = BPSTAT_WHAT_STOP_SILENT;
	  else
	    this_action = BPSTAT_WHAT_SINGLE;
	  break;

	default:
	  internal_error (__FILE__, __LINE__,
			  _("bpstat_what: unhandled bptype %d"), (int) bptype);
	}

      retval.main_action = max (retval.main_action, this_action);
    }

  return retval;
}

void
bpstat_run_callbacks (bpstat bs_head)
{
  bpstat bs;

  for (bs = bs_head; bs != NULL; bs = bs->next)
    {
      struct breakpoint *b = bs->breakpoint_at;

      if (b == NULL)
	continue;
      switch (b->type)
	{
	case bp_jit_event:
	  handle_jit_event ();
	  break;
	case bp_gnu_ifunc_resolver:
	  gnu_ifunc_resolver_stop (b);
	  break;
	case bp_gnu_ifunc_resolver_return:
	  gnu_ifunc_resolver_return_stop (b);
	  break;
	}
    }
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

/* Determine if the locations of this breakpoint will have their conditions
   evaluated by the target, host or a mix of both.  Returns the following:

    "host": Host evals condition.
    "host or target": Host or Target evals condition.
    "target": Target evals condition.
*/

static const char *
bp_condition_evaluator (struct breakpoint *b)
{
  struct bp_location *bl;
  char host_evals = 0;
  char target_evals = 0;

  if (!b)
    return NULL;

  if (!is_breakpoint (b))
    return NULL;

  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return condition_evaluation_host;

  for (bl = b->loc; bl; bl = bl->next)
    {
      if (bl->cond_bytecode)
	target_evals++;
      else
	host_evals++;
    }

  if (host_evals && target_evals)
    return condition_evaluation_both;
  else if (target_evals)
    return condition_evaluation_target;
  else
    return condition_evaluation_host;
}

/* Determine the breakpoint location's condition evaluator.  This is
   similar to bp_condition_evaluator, but for locations.  */

static const char *
bp_location_condition_evaluator (struct bp_location *bl)
{
  if (bl && !is_breakpoint (bl->owner))
    return NULL;

  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return condition_evaluation_host;

  if (bl && bl->cond_bytecode)
    return condition_evaluation_target;
  else
    return condition_evaluation_host;
}

/* Print the LOC location out of the list of B->LOC locations.  */

static void
print_breakpoint_location (struct breakpoint *b,
			   struct bp_location *loc)
{
  struct ui_out *uiout = current_uiout;
  struct cleanup *old_chain = save_current_program_space ();

  if (loc != NULL && loc->shlib_disabled)
    loc = NULL;

  if (loc != NULL)
    set_current_program_space (loc->pspace);

  if (b->display_canonical)
    ui_out_field_string (uiout, "what",
			 event_location_to_string (b->location));
  else if (loc && loc->symtab)
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
      ui_out_field_string (uiout, "file",
			   symtab_to_filename_for_display (loc->symtab));
      ui_out_text (uiout, ":");

      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string (uiout, "fullname",
			     symtab_to_fullname (loc->symtab));
      
      ui_out_field_int (uiout, "line", loc->line_number);
    }
  else if (loc)
    {
      struct ui_file *stb = mem_fileopen ();
      struct cleanup *stb_chain = make_cleanup_ui_file_delete (stb);

      print_address_symbolic (loc->gdbarch, loc->address, stb,
			      demangle, "");
      ui_out_field_stream (uiout, "at", stb);

      do_cleanups (stb_chain);
    }
  else
    {
      ui_out_field_string (uiout, "pending",
			   event_location_to_string (b->location));
      /* If extra_string is available, it could be holding a condition
	 or dprintf arguments.  In either case, make sure it is printed,
	 too, but only for non-MI streams.  */
      if (!ui_out_is_mi_like_p (uiout) && b->extra_string != NULL)
	{
	  if (b->type == bp_dprintf)
	    ui_out_text (uiout, ",");
	  else
	    ui_out_text (uiout, " ");
	  ui_out_text (uiout, b->extra_string);
	}
    }

  if (loc && is_breakpoint (b)
      && breakpoint_condition_evaluation_mode () == condition_evaluation_target
      && bp_condition_evaluator (b) == condition_evaluation_both)
    {
      ui_out_text (uiout, " (");
      ui_out_field_string (uiout, "evaluated-by",
			   bp_location_condition_evaluator (loc));
      ui_out_text (uiout, ")");
    }

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
    {bp_single_step, "sw single-step"},
    {bp_until, "until"},
    {bp_finish, "finish"},
    {bp_watchpoint, "watchpoint"},
    {bp_hardware_watchpoint, "hw watchpoint"},
    {bp_read_watchpoint, "read watchpoint"},
    {bp_access_watchpoint, "acc watchpoint"},
    {bp_longjmp, "longjmp"},
    {bp_longjmp_resume, "longjmp resume"},
    {bp_longjmp_call_dummy, "longjmp for call dummy"},
    {bp_exception, "exception"},
    {bp_exception_resume, "exception resume"},
    {bp_step_resume, "step resume"},
    {bp_hp_step_resume, "high-priority step resume"},
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
    {bp_dprintf, "dprintf"},
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

/* For MI, output a field named 'thread-groups' with a list as the value.
   For CLI, prefix the list with the string 'inf'. */

static void
output_thread_groups (struct ui_out *uiout,
		      const char *field_name,
		      VEC(int) *inf_num,
		      int mi_only)
{
  struct cleanup *back_to;
  int is_mi = ui_out_is_mi_like_p (uiout);
  int inf;
  int i;

  /* For backward compatibility, don't display inferiors in CLI unless
     there are several.  Always display them for MI. */
  if (!is_mi && mi_only)
    return;

  back_to = make_cleanup_ui_out_list_begin_end (uiout, field_name);

  for (i = 0; VEC_iterate (int, inf_num, i, inf); ++i)
    {
      if (is_mi)
	{
	  char mi_group[10];

	  xsnprintf (mi_group, sizeof (mi_group), "i%d", inf);
	  ui_out_field_string (uiout, NULL, mi_group);
	}
      else
	{
	  if (i == 0)
	    ui_out_text (uiout, " inf ");
	  else
	    ui_out_text (uiout, ", ");
	
	  ui_out_text (uiout, plongest (inf));
	}
    }

  do_cleanups (back_to);
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

  struct ui_out *uiout = current_uiout;
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
	{
	  struct watchpoint *w = (struct watchpoint *) b;

	  /* Field 4, the address, is omitted (which makes the columns
	     not line up too nicely with the headers, but the effect
	     is relatively readable).  */
	  if (opts.addressprint)
	    ui_out_field_skip (uiout, "addr");
	  annotate_field (5);
	  ui_out_field_string (uiout, "what", w->exp_string);
	}
	break;

      case bp_breakpoint:
      case bp_hardware_breakpoint:
      case bp_single_step:
      case bp_until:
      case bp_finish:
      case bp_longjmp:
      case bp_longjmp_resume:
      case bp_longjmp_call_dummy:
      case bp_exception:
      case bp_exception_resume:
      case bp_step_resume:
      case bp_hp_step_resume:
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
      case bp_dprintf:
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


  if (loc != NULL && !header_of_multiple)
    {
      struct inferior *inf;
      VEC(int) *inf_num = NULL;
      int mi_only = 1;

      ALL_INFERIORS (inf)
	{
	  if (inf->pspace == loc->pspace)
	    VEC_safe_push (int, inf_num, inf->num);
	}

        /* For backward compatibility, don't display inferiors in CLI unless
	   there are several.  Always display for MI. */
	if (allflag
	    || (!gdbarch_has_global_breakpoints (target_gdbarch ())
		&& (number_of_program_spaces () > 1
		    || number_of_inferiors () > 1)
		/* LOC is for existing B, it cannot be in
		   moribund_locations and thus having NULL OWNER.  */
		&& loc->owner->type != bp_catchpoint))
	mi_only = 0;
      output_thread_groups (uiout, "thread-groups", inf_num, mi_only);
      VEC_free (int, inf_num);
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

  if (!part_of_multiple)
    b->ops->print_one_detail (b, uiout);

  if (part_of_multiple && frame_id_p (b->frame_id))
    {
      annotate_field (6);
      ui_out_text (uiout, "\tstop only in stack frame at ");
      /* FIXME: cagney/2002-12-01: Shouldn't be poking around inside
         the frame ID.  */
      ui_out_field_core_addr (uiout, "frame",
			      b->gdbarch, b->frame_id.stack_addr);
      ui_out_text (uiout, "\n");
    }
  
  if (!part_of_multiple && b->cond_string)
    {
      annotate_field (7);
      if (is_tracepoint (b))
	ui_out_text (uiout, "\ttrace only if ");
      else
	ui_out_text (uiout, "\tstop only if ");
      ui_out_field_string (uiout, "cond", b->cond_string);

      /* Print whether the target is doing the breakpoint's condition
	 evaluation.  If GDB is doing the evaluation, don't print anything.  */
      if (is_breakpoint (b)
	  && breakpoint_condition_evaluation_mode ()
	  == condition_evaluation_target)
	{
	  ui_out_text (uiout, " (");
	  ui_out_field_string (uiout, "evaluated-by",
			       bp_condition_evaluator (b));
	  ui_out_text (uiout, " evals)");
	}
      ui_out_text (uiout, "\n");
    }

  if (!part_of_multiple && b->thread != -1)
    {
      /* FIXME should make an annotation for this.  */
      ui_out_text (uiout, "\tstop only in thread ");
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_int (uiout, "thread", b->thread);
      else
	{
	  struct thread_info *thr = find_thread_global_id (b->thread);

	  ui_out_field_string (uiout, "thread", print_thread_id (thr));
	}
      ui_out_text (uiout, "\n");
    }
  
  if (!part_of_multiple)
    {
      if (b->hit_count)
	{
	  /* FIXME should make an annotation for this.  */
	  if (is_catchpoint (b))
	    ui_out_text (uiout, "\tcatchpoint");
	  else if (is_tracepoint (b))
	    ui_out_text (uiout, "\ttracepoint");
	  else
	    ui_out_text (uiout, "\tbreakpoint");
	  ui_out_text (uiout, " already hit ");
	  ui_out_field_int (uiout, "times", b->hit_count);
	  if (b->hit_count == 1)
	    ui_out_text (uiout, " time\n");
	  else
	    ui_out_text (uiout, " times\n");
	}
      else
	{
	  /* Output the count also if it is zero, but only if this is mi.  */
	  if (ui_out_is_mi_like_p (uiout))
	    ui_out_field_int (uiout, "times", b->hit_count);
	}
    }

  if (!part_of_multiple && b->ignore_count)
    {
      annotate_field (8);
      ui_out_text (uiout, "\tignore next ");
      ui_out_field_int (uiout, "ignore", b->ignore_count);
      ui_out_text (uiout, " hits\n");
    }

  /* Note that an enable count of 1 corresponds to "enable once"
     behavior, which is reported by the combination of enablement and
     disposition, so we don't need to mention it here.  */
  if (!part_of_multiple && b->enable_count > 1)
    {
      annotate_field (8);
      ui_out_text (uiout, "\tdisable after ");
      /* Tweak the wording to clarify that ignore and enable counts
	 are distinct, and have additive effect.  */
      if (b->ignore_count)
	ui_out_text (uiout, "additional ");
      else
	ui_out_text (uiout, "next ");
      ui_out_field_int (uiout, "enable", b->enable_count);
      ui_out_text (uiout, " hits\n");
    }

  if (!part_of_multiple && is_tracepoint (b))
    {
      struct tracepoint *tp = (struct tracepoint *) b;

      if (tp->traceframe_usage)
	{
	  ui_out_text (uiout, "\ttrace buffer usage ");
	  ui_out_field_int (uiout, "traceframe-usage", tp->traceframe_usage);
	  ui_out_text (uiout, " bytes\n");
	}
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

  if (is_tracepoint (b))
    {
      struct tracepoint *t = (struct tracepoint *) b;

      if (!part_of_multiple && t->pass_count)
	{
	  annotate_field (10);
	  ui_out_text (uiout, "\tpass count ");
	  ui_out_field_int (uiout, "pass", t->pass_count);
	  ui_out_text (uiout, " \n");
	}

      /* Don't display it when tracepoint or tracepoint location is
	 pending.   */
      if (!header_of_multiple && loc != NULL && !loc->shlib_disabled)
	{
	  annotate_field (11);

	  if (ui_out_is_mi_like_p (uiout))
	    ui_out_field_string (uiout, "installed",
				 loc->inserted ? "y" : "n");
	  else
	    {
	      if (loc->inserted)
		ui_out_text (uiout, "\t");
	      else
		ui_out_text (uiout, "\tnot ");
	      ui_out_text (uiout, "installed on target\n");
	    }
	}
    }

  if (ui_out_is_mi_like_p (uiout) && !part_of_multiple)
    {
      if (is_watchpoint (b))
	{
	  struct watchpoint *w = (struct watchpoint *) b;

	  ui_out_field_string (uiout, "original-location", w->exp_string);
	}
      else if (b->location != NULL
	       && event_location_to_string (b->location) != NULL)
	ui_out_field_string (uiout, "original-location",
			     event_location_to_string (b->location));
    }
}

static void
print_one_breakpoint (struct breakpoint *b,
		      struct bp_location **last_loc, 
		      int allflag)
{
  struct cleanup *bkpt_chain;
  struct ui_out *uiout = current_uiout;

  bkpt_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "bkpt");

  print_one_breakpoint_location (b, NULL, 0, last_loc, allflag);
  do_cleanups (bkpt_chain);

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
	  && (b->loc->next || !b->loc->enabled))
	{
	  struct bp_location *loc;
	  int n = 1;

	  for (loc = b->loc; loc; loc = loc->next, ++n)
	    {
	      struct cleanup *inner2 =
		make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
	      print_one_breakpoint_location (b, loc, n, last_loc, allflag);
	      do_cleanups (inner2);
	    }
	}
    }
}

static int
breakpoint_address_bits (struct breakpoint *b)
{
  int print_address_bits = 0;
  struct bp_location *loc;

  /* Software watchpoints that aren't watching memory don't have an
     address to print.  */
  if (is_no_memory_software_watchpoint (b))
    return 0;

  for (loc = b->loc; loc; loc = loc->next)
    {
      int addr_bit;

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
  struct captured_breakpoint_query_args *args
    = (struct captured_breakpoint_query_args *) data;
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

/* Return true if this breakpoint was set by the user, false if it is
   internal or momentary.  */

int
user_breakpoint_p (struct breakpoint *b)
{
  return b->number > 0;
}

/* See breakpoint.h.  */

int
pending_breakpoint_p (struct breakpoint *b)
{
  return b->loc == NULL;
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
  struct ui_out *uiout = current_uiout;

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
  struct ui_out *uiout = current_uiout;

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
  struct ui_out *uiout = current_uiout;

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

/* Print a message describing any user-breakpoints set at PC.  This
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
    others += (user_breakpoint_p (b)
               && breakpoint_has_pc (b, pspace, pc, section));
  if (others > 0)
    {
      if (others == 1)
	printf_filtered (_("Note: breakpoint "));
      else /* if (others == ???) */
	printf_filtered (_("Note: breakpoints "));
      ALL_BREAKPOINTS (b)
	if (user_breakpoint_p (b) && breakpoint_has_pc (b, pspace, pc, section))
	  {
	    others--;
	    printf_filtered ("%d", b->number);
	    if (b->thread == -1 && thread != -1)
	      printf_filtered (" (all threads)");
	    else if (b->thread != -1)
	      printf_filtered (" (thread %d)", b->thread);
	    printf_filtered ("%s%s ",
			     ((b->enable_state == bp_disabled
			       || b->enable_state == bp_call_disabled)
			      ? " (disabled)"
			      : ""),
			     (others > 1) ? "," 
			     : ((others == 1) ? " and" : ""));
	  }
      printf_filtered (_("also set at pc "));
      fputs_filtered (paddress (gdbarch, pc), gdb_stdout);
      printf_filtered (".\n");
    }
}


/* Return true iff it is meaningful to use the address member of
   BPT locations.  For some breakpoint types, the locations' address members
   are irrelevant and it makes no sense to attempt to compare them to other
   addresses (or use them for any other purpose either).

   More specifically, each of the following breakpoint types will
   always have a zero valued location address and we don't want to mark
   breakpoints of any of these types to be a duplicate of an actual
   breakpoint location at address zero:

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
  struct watchpoint *w1 = (struct watchpoint *) loc1->owner;
  struct watchpoint *w2 = (struct watchpoint *) loc2->owner;

  /* Both of them must exist.  */
  gdb_assert (w1 != NULL);
  gdb_assert (w2 != NULL);

  /* If the target can evaluate the condition expression in hardware,
     then we we need to insert both watchpoints even if they are at
     the same place.  Otherwise the watchpoint will only trigger when
     the condition of whichever watchpoint was inserted evaluates to
     true, not giving a chance for GDB to check the condition of the
     other watchpoint.  */
  if ((w1->cond_exp
       && target_can_accel_watchpoint_condition (loc1->address, 
						 loc1->length,
						 loc1->watchpoint_type,
						 w1->cond_exp))
      || (w2->cond_exp
	  && target_can_accel_watchpoint_condition (loc2->address, 
						    loc2->length,
						    loc2->watchpoint_type,
						    w2->cond_exp)))
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

/* See breakpoint.h.  */

int
breakpoint_address_match (struct address_space *aspace1, CORE_ADDR addr1,
			  struct address_space *aspace2, CORE_ADDR addr2)
{
  return ((gdbarch_has_global_breakpoints (target_gdbarch ())
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
  return ((gdbarch_has_global_breakpoints (target_gdbarch ())
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

/* Returns true if the [ADDR,ADDR+LEN) range in ASPACE overlaps
   breakpoint BL.  BL may be a ranged breakpoint.  In most targets, a
   match happens only if ASPACE matches the breakpoint's address
   space.  On targets that have global breakpoints, the address space
   doesn't really matter.  */

static int
breakpoint_location_address_range_overlap (struct bp_location *bl,
					   struct address_space *aspace,
					   CORE_ADDR addr, int len)
{
  if (gdbarch_has_global_breakpoints (target_gdbarch ())
      || bl->pspace->aspace == aspace)
    {
      int bl_len = bl->length != 0 ? bl->length : 1;

      if (mem_ranges_overlap (addr, len, bl->address, bl_len))
	return 1;
    }
  return 0;
}

/* If LOC1 and LOC2's owners are not tracepoints, returns false directly.
   Then, if LOC1 and LOC2 represent the same tracepoint location, returns
   true, otherwise returns false.  */

static int
tracepoint_locations_match (struct bp_location *loc1,
			    struct bp_location *loc2)
{
  if (is_tracepoint (loc1->owner) && is_tracepoint (loc2->owner))
    /* Since tracepoint locations are never duplicated with others', tracepoint
       locations at the same address of different tracepoints are regarded as
       different locations.  */
    return (loc1->address == loc2->address && loc1->owner == loc2->owner);
  else
    return 0;
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
  else if (is_tracepoint (loc1->owner) || is_tracepoint (loc2->owner))
    return tracepoint_locations_match (loc1, loc2);
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
  else if (bptype == bp_single_step)
    {
      /* Single-step breakpoints should not have their addresses
	 modified.  If there's any architectural constrain that
	 applies to this address, then it should have already been
	 taken into account when the breakpoint was created in the
	 first place.  If we didn't do this, stepping through e.g.,
	 Thumb-2 IT blocks would break.  */
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

void
init_bp_location (struct bp_location *loc, const struct bp_location_ops *ops,
		  struct breakpoint *owner)
{
  memset (loc, 0, sizeof (*loc));

  gdb_assert (ops != NULL);

  loc->ops = ops;
  loc->owner = owner;
  loc->cond = NULL;
  loc->cond_bytecode = NULL;
  loc->shlib_disabled = 0;
  loc->enabled = 1;

  switch (owner->type)
    {
    case bp_breakpoint:
    case bp_single_step:
    case bp_until:
    case bp_finish:
    case bp_longjmp:
    case bp_longjmp_resume:
    case bp_longjmp_call_dummy:
    case bp_exception:
    case bp_exception_resume:
    case bp_step_resume:
    case bp_hp_step_resume:
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
    case bp_dprintf:
      loc->loc_type = bp_loc_software_breakpoint;
      mark_breakpoint_location_modified (loc);
      break;
    case bp_hardware_breakpoint:
      loc->loc_type = bp_loc_hardware_breakpoint;
      mark_breakpoint_location_modified (loc);
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
}

/* Allocate a struct bp_location.  */

static struct bp_location *
allocate_bp_location (struct breakpoint *bpt)
{
  return bpt->ops->allocate_location (bpt);
}

static void
free_bp_location (struct bp_location *loc)
{
  loc->ops->dtor (loc);
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

/* Add breakpoint B at the end of the global breakpoint chain.  */

static void
add_to_breakpoint_chain (struct breakpoint *b)
{
  struct breakpoint *b1;

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
}

/* Initializes breakpoint B with type BPTYPE and no locations yet.  */

static void
init_raw_breakpoint_without_location (struct breakpoint *b,
				      struct gdbarch *gdbarch,
				      enum bptype bptype,
				      const struct breakpoint_ops *ops)
{
  memset (b, 0, sizeof (*b));

  gdb_assert (ops != NULL);

  b->ops = ops;
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
  b->condition_not_parsed = 0;
  b->py_bp_object = NULL;
  b->related_breakpoint = b;
  b->location = NULL;
}

/* Helper to set_raw_breakpoint below.  Creates a breakpoint
   that has type BPTYPE and has no locations as yet.  */

static struct breakpoint *
set_raw_breakpoint_without_location (struct gdbarch *gdbarch,
				     enum bptype bptype,
				     const struct breakpoint_ops *ops)
{
  struct breakpoint *b = XNEW (struct breakpoint);

  init_raw_breakpoint_without_location (b, gdbarch, bptype, ops);
  add_to_breakpoint_chain (b);
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
      const char *function_name;
      CORE_ADDR func_addr;

      find_pc_partial_function_gnu_ifunc (loc->address, &function_name,
					  &func_addr, NULL, &is_gnu_ifunc);

      if (is_gnu_ifunc && !explicit_loc)
	{
	  struct breakpoint *b = loc->owner;

	  gdb_assert (loc->pspace == current_program_space);
	  if (gnu_ifunc_resolve_name (function_name,
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
	      /* Remember the resolver's address for use by the return
	         breakpoint.  */
	      loc->related_address = func_addr;
	    }
	}

      if (function_name)
	loc->function_name = xstrdup (function_name);
    }
}

/* Attempt to determine architecture of location identified by SAL.  */
struct gdbarch *
get_sal_arch (struct symtab_and_line sal)
{
  if (sal.section)
    return get_objfile_arch (sal.section->objfile);
  if (sal.symtab)
    return get_objfile_arch (SYMTAB_OBJFILE (sal.symtab));

  return NULL;
}

/* Low level routine for partially initializing a breakpoint of type
   BPTYPE.  The newly created breakpoint's address, section, source
   file name, and line number are provided by SAL.

   It is expected that the caller will complete the initialization of
   the newly created breakpoint struct as well as output any status
   information regarding the creation of a new breakpoint.  */

static void
init_raw_breakpoint (struct breakpoint *b, struct gdbarch *gdbarch,
		     struct symtab_and_line sal, enum bptype bptype,
		     const struct breakpoint_ops *ops)
{
  init_raw_breakpoint_without_location (b, gdbarch, bptype, ops);

  add_location_to_breakpoint (b, &sal);

  if (bptype != bp_catchpoint)
    gdb_assert (sal.pspace != NULL);

  /* Store the program space that was used to set the breakpoint,
     except for ordinary breakpoints, which are independent of the
     program space.  */
  if (bptype != bp_breakpoint && bptype != bp_hardware_breakpoint)
    b->pspace = sal.pspace;
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
		    struct symtab_and_line sal, enum bptype bptype,
		    const struct breakpoint_ops *ops)
{
  struct breakpoint *b = XNEW (struct breakpoint);

  init_raw_breakpoint (b, gdbarch, sal, bptype, ops);
  add_to_breakpoint_chain (b);
  return b;
}

/* Call this routine when stepping and nexting to enable a breakpoint
   if we do a longjmp() or 'throw' in TP.  FRAME is the frame which
   initiated the operation.  */

void
set_longjmp_breakpoint (struct thread_info *tp, struct frame_id frame)
{
  struct breakpoint *b, *b_tmp;
  int thread = tp->global_num;

  /* To avoid having to rescan all objfile symbols at every step,
     we maintain a list of continually-inserted but always disabled
     longjmp "master" breakpoints.  Here, we simply create momentary
     clones of those and enable them for the requested thread.  */
  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->pspace == current_program_space
	&& (b->type == bp_longjmp_master
	    || b->type == bp_exception_master))
      {
	enum bptype type = b->type == bp_longjmp_master ? bp_longjmp : bp_exception;
	struct breakpoint *clone;

	/* longjmp_breakpoint_ops ensures INITIATING_FRAME is cleared again
	   after their removal.  */
	clone = momentary_breakpoint_from_master (b, type,
						  &longjmp_breakpoint_ops, 1);
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
delete_longjmp_breakpoint_at_next_stop (int thread)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->type == bp_longjmp || b->type == bp_exception)
      {
	if (b->thread == thread)
	  b->disposition = disp_del_at_next_stop;
      }
}

/* Place breakpoints of type bp_longjmp_call_dummy to catch longjmp for
   INFERIOR_PTID thread.  Chain them all by RELATED_BREAKPOINT and return
   pointer to any of them.  Return NULL if this system cannot place longjmp
   breakpoints.  */

struct breakpoint *
set_longjmp_breakpoint_for_call_dummy (void)
{
  struct breakpoint *b, *retval = NULL;

  ALL_BREAKPOINTS (b)
    if (b->pspace == current_program_space && b->type == bp_longjmp_master)
      {
	struct breakpoint *new_b;

	new_b = momentary_breakpoint_from_master (b, bp_longjmp_call_dummy,
						  &momentary_breakpoint_ops,
						  1);
	new_b->thread = ptid_to_global_thread_id (inferior_ptid);

	/* Link NEW_B into the chain of RETVAL breakpoints.  */

	gdb_assert (new_b->related_breakpoint == new_b);
	if (retval == NULL)
	  retval = new_b;
	new_b->related_breakpoint = retval;
	while (retval->related_breakpoint != new_b->related_breakpoint)
	  retval = retval->related_breakpoint;
	retval->related_breakpoint = new_b;
      }

  return retval;
}

/* Verify all existing dummy frames and their associated breakpoints for
   TP.  Remove those which can no longer be found in the current frame
   stack.

   You should call this function only at places where it is safe to currently
   unwind the whole stack.  Failed stack unwind would discard live dummy
   frames.  */

void
check_longjmp_breakpoint_for_call_dummy (struct thread_info *tp)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->type == bp_longjmp_call_dummy && b->thread == tp->global_num)
      {
	struct breakpoint *dummy_b = b->related_breakpoint;

	while (dummy_b != b && dummy_b->type != bp_call_dummy)
	  dummy_b = dummy_b->related_breakpoint;
	if (dummy_b->type != bp_call_dummy
	    || frame_find_by_id (dummy_b->frame_id) != NULL)
	  continue;
	
	dummy_frame_discard (dummy_b->frame_id, tp->ptid);

	while (b->related_breakpoint != b)
	  {
	    if (b_tmp == b->related_breakpoint)
	      b_tmp = b->related_breakpoint->next;
	    delete_breakpoint (b->related_breakpoint);
	  }
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
      update_global_location_list (UGLL_MAY_INSERT);
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
      update_global_location_list (UGLL_DONT_INSERT);
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
	momentary_breakpoint_from_master (b, bp_std_terminate,
					  &momentary_breakpoint_ops, 1);
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

  b = create_internal_breakpoint (gdbarch, address, bp_thread_event,
				  &internal_breakpoint_ops);

  b->enable_state = bp_enabled;
  /* location has to be used or breakpoint_re_set will delete me.  */
  b->location = new_address_location (b->loc->address, NULL, 0);

  update_global_location_list_nothrow (UGLL_MAY_INSERT);

  return b;
}

struct lang_and_radix
  {
    enum language lang;
    int radix;
  };

/* Create a breakpoint for JIT code registration and unregistration.  */

struct breakpoint *
create_jit_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  return create_internal_breakpoint (gdbarch, address, bp_jit_event,
				     &internal_breakpoint_ops);
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

/* See breakpoint.h.  */

void
remove_solib_event_breakpoints_at_next_stop (void)
{
  struct breakpoint *b, *b_tmp;

  ALL_BREAKPOINTS_SAFE (b, b_tmp)
    if (b->type == bp_shlib_event
	&& b->loc->pspace == current_program_space)
      b->disposition = disp_del_at_next_stop;
}

/* Helper for create_solib_event_breakpoint /
   create_and_insert_solib_event_breakpoint.  Allows specifying which
   INSERT_MODE to pass through to update_global_location_list.  */

static struct breakpoint *
create_solib_event_breakpoint_1 (struct gdbarch *gdbarch, CORE_ADDR address,
				 enum ugll_insert_mode insert_mode)
{
  struct breakpoint *b;

  b = create_internal_breakpoint (gdbarch, address, bp_shlib_event,
				  &internal_breakpoint_ops);
  update_global_location_list_nothrow (insert_mode);
  return b;
}

struct breakpoint *
create_solib_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  return create_solib_event_breakpoint_1 (gdbarch, address, UGLL_MAY_INSERT);
}

/* See breakpoint.h.  */

struct breakpoint *
create_and_insert_solib_event_breakpoint (struct gdbarch *gdbarch, CORE_ADDR address)
{
  struct breakpoint *b;

  /* Explicitly tell update_global_location_list to insert
     locations.  */
  b = create_solib_event_breakpoint_1 (gdbarch, address, UGLL_INSERT);
  if (!b->loc->inserted)
    {
      delete_breakpoint (b);
      return NULL;
    }
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
	&& solib_name_from_address (loc->pspace, loc->address)
	)
      {
	loc->shlib_disabled = 1;
      }
  }
}

/* Disable any breakpoints and tracepoints that are in SOLIB upon
   notification of unloaded_shlib.  Only apply to enabled breakpoints,
   disabled ones can just stay disabled.  */

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

    if (solib->pspace == loc->pspace
	&& !loc->shlib_disabled
	&& (((b->type == bp_breakpoint
	      || b->type == bp_jit_event
	      || b->type == bp_hardware_breakpoint)
	     && (loc->loc_type == bp_loc_hardware_breakpoint
		 || loc->loc_type == bp_loc_software_breakpoint))
	    || is_tracepoint (b))
	&& solib_contains_address_p (solib, loc->address))
      {
	loc->shlib_disabled = 1;
	/* At this point, we cannot rely on remove_breakpoint
	   succeeding so we must mark the breakpoint as not inserted
	   to prevent future errors occurring in remove_breakpoints.  */
	loc->inserted = 0;

	/* This may cause duplicate notifications for the same breakpoint.  */
	observer_notify_breakpoint_modified (b);

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

/* Disable any breakpoints and tracepoints in OBJFILE upon
   notification of free_objfile.  Only apply to enabled breakpoints,
   disabled ones can just stay disabled.  */

static void
disable_breakpoints_in_freed_objfile (struct objfile *objfile)
{
  struct breakpoint *b;

  if (objfile == NULL)
    return;

  /* OBJF_SHARED|OBJF_USERLOADED objfiles are dynamic modules manually
     managed by the user with add-symbol-file/remove-symbol-file.
     Similarly to how breakpoints in shared libraries are handled in
     response to "nosharedlibrary", mark breakpoints in such modules
     shlib_disabled so they end up uninserted on the next global
     location list update.  Shared libraries not loaded by the user
     aren't handled here -- they're already handled in
     disable_breakpoints_in_unloaded_shlib, called by solib.c's
     solib_unloaded observer.  We skip objfiles that are not
     OBJF_SHARED as those aren't considered dynamic objects (e.g. the
     main objfile).  */
  if ((objfile->flags & OBJF_SHARED) == 0
      || (objfile->flags & OBJF_USERLOADED) == 0)
    return;

  ALL_BREAKPOINTS (b)
    {
      struct bp_location *loc;
      int bp_modified = 0;

      if (!is_breakpoint (b) && !is_tracepoint (b))
	continue;

      for (loc = b->loc; loc != NULL; loc = loc->next)
	{
	  CORE_ADDR loc_addr = loc->address;

	  if (loc->loc_type != bp_loc_hardware_breakpoint
	      && loc->loc_type != bp_loc_software_breakpoint)
	    continue;

	  if (loc->shlib_disabled != 0)
	    continue;

	  if (objfile->pspace != loc->pspace)
	    continue;

	  if (loc->loc_type != bp_loc_hardware_breakpoint
	      && loc->loc_type != bp_loc_software_breakpoint)
	    continue;

	  if (is_addr_in_objfile (loc_addr, objfile))
	    {
	      loc->shlib_disabled = 1;
	      /* At this point, we don't know whether the object was
		 unmapped from the inferior or not, so leave the
		 inserted flag alone.  We'll handle failure to
		 uninsert quietly, in case the object was indeed
		 unmapped.  */

	      mark_breakpoint_location_modified (loc);

	      bp_modified = 1;
	    }
	}

      if (bp_modified)
	observer_notify_breakpoint_modified (b);
    }
}

/* FORK & VFORK catchpoints.  */

/* An instance of this type is used to represent a fork or vfork
   catchpoint.  It includes a "struct breakpoint" as a kind of base
   class; users downcast to "struct breakpoint *" when needed.  A
   breakpoint is really of this type iff its ops pointer points to
   CATCH_FORK_BREAKPOINT_OPS.  */

struct fork_catchpoint
{
  /* The base class.  */
  struct breakpoint base;

  /* Process id of a child process whose forking triggered this
     catchpoint.  This field is only valid immediately after this
     catchpoint has triggered.  */
  ptid_t forked_inferior_pid;
};

/* Implement the "insert" breakpoint_ops method for fork
   catchpoints.  */

static int
insert_catch_fork (struct bp_location *bl)
{
  return target_insert_fork_catchpoint (ptid_get_pid (inferior_ptid));
}

/* Implement the "remove" breakpoint_ops method for fork
   catchpoints.  */

static int
remove_catch_fork (struct bp_location *bl, enum remove_bp_reason reason)
{
  return target_remove_fork_catchpoint (ptid_get_pid (inferior_ptid));
}

/* Implement the "breakpoint_hit" breakpoint_ops method for fork
   catchpoints.  */

static int
breakpoint_hit_catch_fork (const struct bp_location *bl,
			   struct address_space *aspace, CORE_ADDR bp_addr,
			   const struct target_waitstatus *ws)
{
  struct fork_catchpoint *c = (struct fork_catchpoint *) bl->owner;

  if (ws->kind != TARGET_WAITKIND_FORKED)
    return 0;

  c->forked_inferior_pid = ws->value.related_pid;
  return 1;
}

/* Implement the "print_it" breakpoint_ops method for fork
   catchpoints.  */

static enum print_stop_action
print_it_catch_fork (bpstat bs)
{
  struct ui_out *uiout = current_uiout;
  struct breakpoint *b = bs->breakpoint_at;
  struct fork_catchpoint *c = (struct fork_catchpoint *) bs->breakpoint_at;

  annotate_catchpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);
  if (b->disposition == disp_del)
    ui_out_text (uiout, "Temporary catchpoint ");
  else
    ui_out_text (uiout, "Catchpoint ");
  if (ui_out_is_mi_like_p (uiout))
    {
      ui_out_field_string (uiout, "reason",
			   async_reason_lookup (EXEC_ASYNC_FORK));
      ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
    }
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, " (forked process ");
  ui_out_field_int (uiout, "newpid", ptid_get_pid (c->forked_inferior_pid));
  ui_out_text (uiout, "), ");
  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" breakpoint_ops method for fork
   catchpoints.  */

static void
print_one_catch_fork (struct breakpoint *b, struct bp_location **last_loc)
{
  struct fork_catchpoint *c = (struct fork_catchpoint *) b;
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  get_user_print_options (&opts);

  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);
  ui_out_text (uiout, "fork");
  if (!ptid_equal (c->forked_inferior_pid, null_ptid))
    {
      ui_out_text (uiout, ", process ");
      ui_out_field_int (uiout, "what",
                        ptid_get_pid (c->forked_inferior_pid));
      ui_out_spaces (uiout, 1);
    }

  if (ui_out_is_mi_like_p (uiout))
    ui_out_field_string (uiout, "catch-type", "fork");
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
  print_recreate_thread (b, fp);
}

/* The breakpoint_ops structure to be used in fork catchpoints.  */

static struct breakpoint_ops catch_fork_breakpoint_ops;

/* Implement the "insert" breakpoint_ops method for vfork
   catchpoints.  */

static int
insert_catch_vfork (struct bp_location *bl)
{
  return target_insert_vfork_catchpoint (ptid_get_pid (inferior_ptid));
}

/* Implement the "remove" breakpoint_ops method for vfork
   catchpoints.  */

static int
remove_catch_vfork (struct bp_location *bl, enum remove_bp_reason reason)
{
  return target_remove_vfork_catchpoint (ptid_get_pid (inferior_ptid));
}

/* Implement the "breakpoint_hit" breakpoint_ops method for vfork
   catchpoints.  */

static int
breakpoint_hit_catch_vfork (const struct bp_location *bl,
			    struct address_space *aspace, CORE_ADDR bp_addr,
			    const struct target_waitstatus *ws)
{
  struct fork_catchpoint *c = (struct fork_catchpoint *) bl->owner;

  if (ws->kind != TARGET_WAITKIND_VFORKED)
    return 0;

  c->forked_inferior_pid = ws->value.related_pid;
  return 1;
}

/* Implement the "print_it" breakpoint_ops method for vfork
   catchpoints.  */

static enum print_stop_action
print_it_catch_vfork (bpstat bs)
{
  struct ui_out *uiout = current_uiout;
  struct breakpoint *b = bs->breakpoint_at;
  struct fork_catchpoint *c = (struct fork_catchpoint *) b;

  annotate_catchpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);
  if (b->disposition == disp_del)
    ui_out_text (uiout, "Temporary catchpoint ");
  else
    ui_out_text (uiout, "Catchpoint ");
  if (ui_out_is_mi_like_p (uiout))
    {
      ui_out_field_string (uiout, "reason",
			   async_reason_lookup (EXEC_ASYNC_VFORK));
      ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
    }
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, " (vforked process ");
  ui_out_field_int (uiout, "newpid", ptid_get_pid (c->forked_inferior_pid));
  ui_out_text (uiout, "), ");
  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" breakpoint_ops method for vfork
   catchpoints.  */

static void
print_one_catch_vfork (struct breakpoint *b, struct bp_location **last_loc)
{
  struct fork_catchpoint *c = (struct fork_catchpoint *) b;
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  get_user_print_options (&opts);
  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);
  ui_out_text (uiout, "vfork");
  if (!ptid_equal (c->forked_inferior_pid, null_ptid))
    {
      ui_out_text (uiout, ", process ");
      ui_out_field_int (uiout, "what",
                        ptid_get_pid (c->forked_inferior_pid));
      ui_out_spaces (uiout, 1);
    }

  if (ui_out_is_mi_like_p (uiout))
    ui_out_field_string (uiout, "catch-type", "vfork");
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
  print_recreate_thread (b, fp);
}

/* The breakpoint_ops structure to be used in vfork catchpoints.  */

static struct breakpoint_ops catch_vfork_breakpoint_ops;

/* An instance of this type is used to represent an solib catchpoint.
   It includes a "struct breakpoint" as a kind of base class; users
   downcast to "struct breakpoint *" when needed.  A breakpoint is
   really of this type iff its ops pointer points to
   CATCH_SOLIB_BREAKPOINT_OPS.  */

struct solib_catchpoint
{
  /* The base class.  */
  struct breakpoint base;

  /* True for "catch load", false for "catch unload".  */
  unsigned char is_load;

  /* Regular expression to match, if any.  COMPILED is only valid when
     REGEX is non-NULL.  */
  char *regex;
  regex_t compiled;
};

static void
dtor_catch_solib (struct breakpoint *b)
{
  struct solib_catchpoint *self = (struct solib_catchpoint *) b;

  if (self->regex)
    regfree (&self->compiled);
  xfree (self->regex);

  base_breakpoint_ops.dtor (b);
}

static int
insert_catch_solib (struct bp_location *ignore)
{
  return 0;
}

static int
remove_catch_solib (struct bp_location *ignore, enum remove_bp_reason reason)
{
  return 0;
}

static int
breakpoint_hit_catch_solib (const struct bp_location *bl,
			    struct address_space *aspace,
			    CORE_ADDR bp_addr,
			    const struct target_waitstatus *ws)
{
  struct solib_catchpoint *self = (struct solib_catchpoint *) bl->owner;
  struct breakpoint *other;

  if (ws->kind == TARGET_WAITKIND_LOADED)
    return 1;

  ALL_BREAKPOINTS (other)
  {
    struct bp_location *other_bl;

    if (other == bl->owner)
      continue;

    if (other->type != bp_shlib_event)
      continue;

    if (self->base.pspace != NULL && other->pspace != self->base.pspace)
      continue;

    for (other_bl = other->loc; other_bl != NULL; other_bl = other_bl->next)
      {
	if (other->ops->breakpoint_hit (other_bl, aspace, bp_addr, ws))
	  return 1;
      }
  }

  return 0;
}

static void
check_status_catch_solib (struct bpstats *bs)
{
  struct solib_catchpoint *self
    = (struct solib_catchpoint *) bs->breakpoint_at;
  int ix;

  if (self->is_load)
    {
      struct so_list *iter;

      for (ix = 0;
	   VEC_iterate (so_list_ptr, current_program_space->added_solibs,
			ix, iter);
	   ++ix)
	{
	  if (!self->regex
	      || regexec (&self->compiled, iter->so_name, 0, NULL, 0) == 0)
	    return;
	}
    }
  else
    {
      char *iter;

      for (ix = 0;
	   VEC_iterate (char_ptr, current_program_space->deleted_solibs,
			ix, iter);
	   ++ix)
	{
	  if (!self->regex
	      || regexec (&self->compiled, iter, 0, NULL, 0) == 0)
	    return;
	}
    }

  bs->stop = 0;
  bs->print_it = print_it_noop;
}

static enum print_stop_action
print_it_catch_solib (bpstat bs)
{
  struct breakpoint *b = bs->breakpoint_at;
  struct ui_out *uiout = current_uiout;

  annotate_catchpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);
  if (b->disposition == disp_del)
    ui_out_text (uiout, "Temporary catchpoint ");
  else
    ui_out_text (uiout, "Catchpoint ");
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, "\n");
  if (ui_out_is_mi_like_p (uiout))
    ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
  print_solib_event (1);
  return PRINT_SRC_AND_LOC;
}

static void
print_one_catch_solib (struct breakpoint *b, struct bp_location **locs)
{
  struct solib_catchpoint *self = (struct solib_catchpoint *) b;
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;
  char *msg;

  get_user_print_options (&opts);
  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    {
      annotate_field (4);
      ui_out_field_skip (uiout, "addr");
    }

  annotate_field (5);
  if (self->is_load)
    {
      if (self->regex)
	msg = xstrprintf (_("load of library matching %s"), self->regex);
      else
	msg = xstrdup (_("load of library"));
    }
  else
    {
      if (self->regex)
	msg = xstrprintf (_("unload of library matching %s"), self->regex);
      else
	msg = xstrdup (_("unload of library"));
    }
  ui_out_field_string (uiout, "what", msg);
  xfree (msg);

  if (ui_out_is_mi_like_p (uiout))
    ui_out_field_string (uiout, "catch-type",
			 self->is_load ? "load" : "unload");
}

static void
print_mention_catch_solib (struct breakpoint *b)
{
  struct solib_catchpoint *self = (struct solib_catchpoint *) b;

  printf_filtered (_("Catchpoint %d (%s)"), b->number,
		   self->is_load ? "load" : "unload");
}

static void
print_recreate_catch_solib (struct breakpoint *b, struct ui_file *fp)
{
  struct solib_catchpoint *self = (struct solib_catchpoint *) b;

  fprintf_unfiltered (fp, "%s %s",
		      b->disposition == disp_del ? "tcatch" : "catch",
		      self->is_load ? "load" : "unload");
  if (self->regex)
    fprintf_unfiltered (fp, " %s", self->regex);
  fprintf_unfiltered (fp, "\n");
}

static struct breakpoint_ops catch_solib_breakpoint_ops;

/* Shared helper function (MI and CLI) for creating and installing
   a shared object event catchpoint.  If IS_LOAD is non-zero then
   the events to be caught are load events, otherwise they are
   unload events.  If IS_TEMP is non-zero the catchpoint is a
   temporary one.  If ENABLED is non-zero the catchpoint is
   created in an enabled state.  */

void
add_solib_catchpoint (char *arg, int is_load, int is_temp, int enabled)
{
  struct solib_catchpoint *c;
  struct gdbarch *gdbarch = get_current_arch ();
  struct cleanup *cleanup;

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);

  c = XCNEW (struct solib_catchpoint);
  cleanup = make_cleanup (xfree, c);

  if (*arg != '\0')
    {
      int errcode;

      errcode = regcomp (&c->compiled, arg, REG_NOSUB);
      if (errcode != 0)
	{
	  char *err = get_regcomp_error (errcode, &c->compiled);

	  make_cleanup (xfree, err);
	  error (_("Invalid regexp (%s): %s"), err, arg);
	}
      c->regex = xstrdup (arg);
    }

  c->is_load = is_load;
  init_catchpoint (&c->base, gdbarch, is_temp, NULL,
		   &catch_solib_breakpoint_ops);

  c->base.enable_state = enabled ? bp_enabled : bp_disabled;

  discard_cleanups (cleanup);
  install_breakpoint (0, &c->base, 1);
}

/* A helper function that does all the work for "catch load" and
   "catch unload".  */

static void
catch_load_or_unload (char *arg, int from_tty, int is_load,
		      struct cmd_list_element *command)
{
  int tempflag;
  const int enabled = 1;

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  add_solib_catchpoint (arg, is_load, tempflag, enabled);
}

static void
catch_load_command_1 (char *arg, int from_tty,
		      struct cmd_list_element *command)
{
  catch_load_or_unload (arg, from_tty, 1, command);
}

static void
catch_unload_command_1 (char *arg, int from_tty,
			struct cmd_list_element *command)
{
  catch_load_or_unload (arg, from_tty, 0, command);
}

/* Initialize a new breakpoint of the bp_catchpoint kind.  If TEMPFLAG
   is non-zero, then make the breakpoint temporary.  If COND_STRING is
   not NULL, then store it in the breakpoint.  OPS, if not NULL, is
   the breakpoint_ops structure associated to the catchpoint.  */

void
init_catchpoint (struct breakpoint *b,
		 struct gdbarch *gdbarch, int tempflag,
		 char *cond_string,
		 const struct breakpoint_ops *ops)
{
  struct symtab_and_line sal;

  init_sal (&sal);
  sal.pspace = current_program_space;

  init_raw_breakpoint (b, gdbarch, sal, bp_catchpoint, ops);

  b->cond_string = (cond_string == NULL) ? NULL : xstrdup (cond_string);
  b->disposition = tempflag ? disp_del : disp_donttouch;
}

void
install_breakpoint (int internal, struct breakpoint *b, int update_gll)
{
  add_to_breakpoint_chain (b);
  set_breakpoint_number (internal, b);
  if (is_tracepoint (b))
    set_tracepoint_count (breakpoint_count);
  if (!internal)
    mention (b);
  observer_notify_breakpoint_created (b);

  if (update_gll)
    update_global_location_list (UGLL_MAY_INSERT);
}

static void
create_fork_vfork_event_catchpoint (struct gdbarch *gdbarch,
				    int tempflag, char *cond_string,
                                    const struct breakpoint_ops *ops)
{
  struct fork_catchpoint *c = XNEW (struct fork_catchpoint);

  init_catchpoint (&c->base, gdbarch, tempflag, cond_string, ops);

  c->forked_inferior_pid = null_ptid;

  install_breakpoint (0, &c->base, 1);
}

/* Exec catchpoints.  */

/* An instance of this type is used to represent an exec catchpoint.
   It includes a "struct breakpoint" as a kind of base class; users
   downcast to "struct breakpoint *" when needed.  A breakpoint is
   really of this type iff its ops pointer points to
   CATCH_EXEC_BREAKPOINT_OPS.  */

struct exec_catchpoint
{
  /* The base class.  */
  struct breakpoint base;

  /* Filename of a program whose exec triggered this catchpoint.
     This field is only valid immediately after this catchpoint has
     triggered.  */
  char *exec_pathname;
};

/* Implement the "dtor" breakpoint_ops method for exec
   catchpoints.  */

static void
dtor_catch_exec (struct breakpoint *b)
{
  struct exec_catchpoint *c = (struct exec_catchpoint *) b;

  xfree (c->exec_pathname);

  base_breakpoint_ops.dtor (b);
}

static int
insert_catch_exec (struct bp_location *bl)
{
  return target_insert_exec_catchpoint (ptid_get_pid (inferior_ptid));
}

static int
remove_catch_exec (struct bp_location *bl, enum remove_bp_reason reason)
{
  return target_remove_exec_catchpoint (ptid_get_pid (inferior_ptid));
}

static int
breakpoint_hit_catch_exec (const struct bp_location *bl,
			   struct address_space *aspace, CORE_ADDR bp_addr,
			   const struct target_waitstatus *ws)
{
  struct exec_catchpoint *c = (struct exec_catchpoint *) bl->owner;

  if (ws->kind != TARGET_WAITKIND_EXECD)
    return 0;

  c->exec_pathname = xstrdup (ws->value.execd_pathname);
  return 1;
}

static enum print_stop_action
print_it_catch_exec (bpstat bs)
{
  struct ui_out *uiout = current_uiout;
  struct breakpoint *b = bs->breakpoint_at;
  struct exec_catchpoint *c = (struct exec_catchpoint *) b;

  annotate_catchpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);
  if (b->disposition == disp_del)
    ui_out_text (uiout, "Temporary catchpoint ");
  else
    ui_out_text (uiout, "Catchpoint ");
  if (ui_out_is_mi_like_p (uiout))
    {
      ui_out_field_string (uiout, "reason",
			   async_reason_lookup (EXEC_ASYNC_EXEC));
      ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
    }
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, " (exec'd ");
  ui_out_field_string (uiout, "new-exec", c->exec_pathname);
  ui_out_text (uiout, "), ");

  return PRINT_SRC_AND_LOC;
}

static void
print_one_catch_exec (struct breakpoint *b, struct bp_location **last_loc)
{
  struct exec_catchpoint *c = (struct exec_catchpoint *) b;
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  get_user_print_options (&opts);

  /* Field 4, the address, is omitted (which makes the columns
     not line up too nicely with the headers, but the effect
     is relatively readable).  */
  if (opts.addressprint)
    ui_out_field_skip (uiout, "addr");
  annotate_field (5);
  ui_out_text (uiout, "exec");
  if (c->exec_pathname != NULL)
    {
      ui_out_text (uiout, ", program \"");
      ui_out_field_string (uiout, "what", c->exec_pathname);
      ui_out_text (uiout, "\" ");
    }

  if (ui_out_is_mi_like_p (uiout))
    ui_out_field_string (uiout, "catch-type", "exec");
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
  print_recreate_thread (b, fp);
}

static struct breakpoint_ops catch_exec_breakpoint_ops;

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
	  i += b->ops->resources_needed (bl);
	}
  }

  return i;
}

/* Returns the resources B would use if it were a hardware
   watchpoint.  */

static int
hw_watchpoint_use_count (struct breakpoint *b)
{
  int i = 0;
  struct bp_location *bl;

  if (!breakpoint_enabled (b))
    return 0;

  for (bl = b->loc; bl; bl = bl->next)
    {
      /* Special types of hardware watchpoints may use more than
	 one register.  */
      i += b->ops->resources_needed (bl);
    }

  return i;
}

/* Returns the sum the used resources of all hardware watchpoints of
   type TYPE in the breakpoints list.  Also returns in OTHER_TYPE_USED
   the sum of the used resources of all hardware watchpoints of other
   types _not_ TYPE.  */

static int
hw_watchpoint_used_count_others (struct breakpoint *except,
				 enum bptype type, int *other_type_used)
{
  int i = 0;
  struct breakpoint *b;

  *other_type_used = 0;
  ALL_BREAKPOINTS (b)
    {
      if (b == except)
	continue;
      if (!breakpoint_enabled (b))
	continue;

      if (b->type == type)
	i += hw_watchpoint_use_count (b);
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
	update_global_location_list (UGLL_DONT_INSERT);
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
	update_global_location_list (UGLL_MAY_INSERT);
      }
  }
}

void
disable_breakpoints_before_startup (void)
{
  current_program_space->executing_startup = 1;
  update_global_location_list (UGLL_DONT_INSERT);
}

void
enable_breakpoints_after_startup (void)
{
  current_program_space->executing_startup = 0;
  breakpoint_re_set ();
}

/* Create a new single-step breakpoint for thread THREAD, with no
   locations.  */

static struct breakpoint *
new_single_step_breakpoint (int thread, struct gdbarch *gdbarch)
{
  struct breakpoint *b = XNEW (struct breakpoint);

  init_raw_breakpoint_without_location (b, gdbarch, bp_single_step,
					&momentary_breakpoint_ops);

  b->disposition = disp_donttouch;
  b->frame_id = null_frame_id;

  b->thread = thread;
  gdb_assert (b->thread != 0);

  add_to_breakpoint_chain (b);

  return b;
}

/* Set a momentary breakpoint of type TYPE at address specified by
   SAL.  If FRAME_ID is valid, the breakpoint is restricted to that
   frame.  */

struct breakpoint *
set_momentary_breakpoint (struct gdbarch *gdbarch, struct symtab_and_line sal,
			  struct frame_id frame_id, enum bptype type)
{
  struct breakpoint *b;

  /* If FRAME_ID is valid, it should be a real frame, not an inlined or
     tail-called one.  */
  gdb_assert (!frame_id_artificial_p (frame_id));

  b = set_raw_breakpoint (gdbarch, sal, type, &momentary_breakpoint_ops);
  b->enable_state = bp_enabled;
  b->disposition = disp_donttouch;
  b->frame_id = frame_id;

  /* If we're debugging a multi-threaded program, then we want
     momentary breakpoints to be active in only a single thread of
     control.  */
  if (in_thread_list (inferior_ptid))
    b->thread = ptid_to_global_thread_id (inferior_ptid);

  update_global_location_list_nothrow (UGLL_MAY_INSERT);

  return b;
}

/* Make a momentary breakpoint based on the master breakpoint ORIG.
   The new breakpoint will have type TYPE, use OPS as its
   breakpoint_ops, and will set enabled to LOC_ENABLED.  */

static struct breakpoint *
momentary_breakpoint_from_master (struct breakpoint *orig,
				  enum bptype type,
				  const struct breakpoint_ops *ops,
				  int loc_enabled)
{
  struct breakpoint *copy;

  copy = set_raw_breakpoint_without_location (orig->gdbarch, type, ops);
  copy->loc = allocate_bp_location (copy);
  set_breakpoint_location_function (copy->loc, 1);

  copy->loc->gdbarch = orig->loc->gdbarch;
  copy->loc->requested_address = orig->loc->requested_address;
  copy->loc->address = orig->loc->address;
  copy->loc->section = orig->loc->section;
  copy->loc->pspace = orig->loc->pspace;
  copy->loc->probe = orig->loc->probe;
  copy->loc->line_number = orig->loc->line_number;
  copy->loc->symtab = orig->loc->symtab;
  copy->loc->enabled = loc_enabled;
  copy->frame_id = orig->frame_id;
  copy->thread = orig->thread;
  copy->pspace = orig->pspace;

  copy->enable_state = bp_enabled;
  copy->disposition = disp_donttouch;
  copy->number = internal_breakpoint_number--;

  update_global_location_list_nothrow (UGLL_DONT_INSERT);
  return copy;
}

/* Make a deep copy of momentary breakpoint ORIG.  Returns NULL if
   ORIG is NULL.  */

struct breakpoint *
clone_momentary_breakpoint (struct breakpoint *orig)
{
  /* If there's nothing to clone, then return nothing.  */
  if (orig == NULL)
    return NULL;

  return momentary_breakpoint_from_master (orig, orig->type, orig->ops, 0);
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
  b->ops->print_mention (b);
  if (ui_out_is_mi_like_p (current_uiout))
    return;
  printf_filtered ("\n");
}


static int bp_loc_is_permanent (struct bp_location *loc);

static struct bp_location *
add_location_to_breakpoint (struct breakpoint *b,
			    const struct symtab_and_line *sal)
{
  struct bp_location *loc, **tmp;
  CORE_ADDR adjusted_address;
  struct gdbarch *loc_gdbarch = get_sal_arch (*sal);

  if (loc_gdbarch == NULL)
    loc_gdbarch = b->gdbarch;

  /* Adjust the breakpoint's address prior to allocating a location.
     Once we call allocate_bp_location(), that mostly uninitialized
     location will be placed on the location chain.  Adjustment of the
     breakpoint may cause target_read_memory() to be called and we do
     not want its scan of the location chain to find a breakpoint and
     location that's only been partially initialized.  */
  adjusted_address = adjust_breakpoint_address (loc_gdbarch,
						sal->pc, b->type);

  /* Sort the locations by their ADDRESS.  */
  loc = allocate_bp_location (b);
  for (tmp = &(b->loc); *tmp != NULL && (*tmp)->address <= adjusted_address;
       tmp = &((*tmp)->next))
    ;
  loc->next = *tmp;
  *tmp = loc;

  loc->requested_address = sal->pc;
  loc->address = adjusted_address;
  loc->pspace = sal->pspace;
  loc->probe.probe = sal->probe;
  loc->probe.objfile = sal->objfile;
  gdb_assert (loc->pspace != NULL);
  loc->section = sal->section;
  loc->gdbarch = loc_gdbarch;
  loc->line_number = sal->line;
  loc->symtab = sal->symtab;

  set_breakpoint_location_function (loc,
				    sal->explicit_pc || sal->explicit_line);

  /* While by definition, permanent breakpoints are already present in the
     code, we don't mark the location as inserted.  Normally one would expect
     that GDB could rely on that breakpoint instruction to stop the program,
     thus removing the need to insert its own breakpoint, except that executing
     the breakpoint instruction can kill the target instead of reporting a
     SIGTRAP.  E.g., on SPARC, when interrupts are disabled, executing the
     instruction resets the CPU, so QEMU 2.0.0 for SPARC correspondingly dies
     with "Trap 0x02 while interrupts disabled, Error state".  Letting the
     breakpoint be inserted normally results in QEMU knowing about the GDB
     breakpoint, and thus trap before the breakpoint instruction is executed.
     (If GDB later needs to continue execution past the permanent breakpoint,
     it manually increments the PC, thus avoiding executing the breakpoint
     instruction.)  */
  if (bp_loc_is_permanent (loc))
    loc->permanent = 1;

  return loc;
}


/* See breakpoint.h.  */

int
program_breakpoint_here_p (struct gdbarch *gdbarch, CORE_ADDR address)
{
  int len;
  CORE_ADDR addr;
  const gdb_byte *bpoint;
  gdb_byte *target_mem;
  struct cleanup *cleanup;
  int retval = 0;

  addr = address;
  bpoint = gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  /* Software breakpoints unsupported?  */
  if (bpoint == NULL)
    return 0;

  target_mem = (gdb_byte *) alloca (len);

  /* Enable the automatic memory restoration from breakpoints while
     we read the memory.  Otherwise we could say about our temporary
     breakpoints they are permanent.  */
  cleanup = make_show_memory_breakpoints_cleanup (0);

  if (target_read_memory (address, target_mem, len) == 0
      && memcmp (target_mem, bpoint, len) == 0)
    retval = 1;

  do_cleanups (cleanup);

  return retval;
}

/* Return 1 if LOC is pointing to a permanent breakpoint,
   return 0 otherwise.  */

static int
bp_loc_is_permanent (struct bp_location *loc)
{
  struct cleanup *cleanup;
  int retval;

  gdb_assert (loc != NULL);

  /* If we have a catchpoint or a watchpoint, just return 0.  We should not
     attempt to read from the addresses the locations of these breakpoint types
     point to.  program_breakpoint_here_p, below, will attempt to read
     memory.  */
  if (!breakpoint_address_is_meaningful (loc->owner))
    return 0;

  cleanup = save_current_space_and_thread ();
  switch_to_program_space_and_thread (loc->pspace);

  retval = program_breakpoint_here_p (loc->gdbarch, loc->address);

  do_cleanups (cleanup);

  return retval;
}

/* Build a command list for the dprintf corresponding to the current
   settings of the dprintf style options.  */

static void
update_dprintf_command_list (struct breakpoint *b)
{
  char *dprintf_args = b->extra_string;
  char *printf_line = NULL;

  if (!dprintf_args)
    return;

  dprintf_args = skip_spaces (dprintf_args);

  /* Allow a comma, as it may have terminated a location, but don't
     insist on it.  */
  if (*dprintf_args == ',')
    ++dprintf_args;
  dprintf_args = skip_spaces (dprintf_args);

  if (*dprintf_args != '"')
    error (_("Bad format string, missing '\"'."));

  if (strcmp (dprintf_style, dprintf_style_gdb) == 0)
    printf_line = xstrprintf ("printf %s", dprintf_args);
  else if (strcmp (dprintf_style, dprintf_style_call) == 0)
    {
      if (!dprintf_function)
	error (_("No function supplied for dprintf call"));

      if (dprintf_channel && strlen (dprintf_channel) > 0)
	printf_line = xstrprintf ("call (void) %s (%s,%s)",
				  dprintf_function,
				  dprintf_channel,
				  dprintf_args);
      else
	printf_line = xstrprintf ("call (void) %s (%s)",
				  dprintf_function,
				  dprintf_args);
    }
  else if (strcmp (dprintf_style, dprintf_style_agent) == 0)
    {
      if (target_can_run_breakpoint_commands ())
	printf_line = xstrprintf ("agent-printf %s", dprintf_args);
      else
	{
	  warning (_("Target cannot run dprintf commands, falling back to GDB printf"));
	  printf_line = xstrprintf ("printf %s", dprintf_args);
	}
    }
  else
    internal_error (__FILE__, __LINE__,
		    _("Invalid dprintf style."));

  gdb_assert (printf_line != NULL);
  /* Manufacture a printf sequence.  */
  {
    struct command_line *printf_cmd_line = XNEW (struct command_line);

    printf_cmd_line->control_type = simple_control;
    printf_cmd_line->body_count = 0;
    printf_cmd_line->body_list = NULL;
    printf_cmd_line->next = NULL;
    printf_cmd_line->line = printf_line;

    breakpoint_set_commands (b, printf_cmd_line);
  }
}

/* Update all dprintf commands, making their command lists reflect
   current style settings.  */

static void
update_dprintf_commands (char *args, int from_tty,
			 struct cmd_list_element *c)
{
  struct breakpoint *b;

  ALL_BREAKPOINTS (b)
    {
      if (b->type == bp_dprintf)
	update_dprintf_command_list (b);
    }
}

/* Create a breakpoint with SAL as location.  Use LOCATION
   as a description of the location, and COND_STRING
   as condition expression.  If LOCATION is NULL then create an
   "address location" from the address in the SAL.  */

static void
init_breakpoint_sal (struct breakpoint *b, struct gdbarch *gdbarch,
		     struct symtabs_and_lines sals,
		     struct event_location *location,
		     char *filter, char *cond_string,
		     char *extra_string,
		     enum bptype type, enum bpdisp disposition,
		     int thread, int task, int ignore_count,
		     const struct breakpoint_ops *ops, int from_tty,
		     int enabled, int internal, unsigned flags,
		     int display_canonical)
{
  int i;

  if (type == bp_hardware_breakpoint)
    {
      int target_resources_ok;

      i = hw_breakpoint_used_count ();
      target_resources_ok =
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
	  init_raw_breakpoint (b, gdbarch, sal, type, ops);
	  b->thread = thread;
	  b->task = task;

	  b->cond_string = cond_string;
	  b->extra_string = extra_string;
	  b->ignore_count = ignore_count;
	  b->enable_state = enabled ? bp_enabled : bp_disabled;
	  b->disposition = disposition;

	  if ((flags & CREATE_BREAKPOINT_FLAGS_INSERTED) != 0)
	    b->loc->inserted = 1;

	  if (type == bp_static_tracepoint)
	    {
	      struct tracepoint *t = (struct tracepoint *) b;
	      struct static_tracepoint_marker marker;

	      if (strace_marker_p (b))
		{
		  /* We already know the marker exists, otherwise, we
		     wouldn't see a sal for it.  */
		  const char *p = &event_location_to_string (b->location)[3];
		  const char *endp;
		  char *marker_str;

		  p = skip_spaces_const (p);

		  endp = skip_to_space_const (p);

		  marker_str = savestring (p, endp - p);
		  t->static_trace_marker_id = marker_str;

		  printf_filtered (_("Probed static tracepoint "
				     "marker \"%s\"\n"),
				   t->static_trace_marker_id);
		}
	      else if (target_static_tracepoint_marker_at (sal.pc, &marker))
		{
		  t->static_trace_marker_id = xstrdup (marker.str_id);
		  release_static_tracepoint_marker (&marker);

		  printf_filtered (_("Probed static tracepoint "
				     "marker \"%s\"\n"),
				   t->static_trace_marker_id);
		}
	      else
		warning (_("Couldn't determine the static "
			   "tracepoint marker to probe"));
	    }

	  loc = b->loc;
	}
      else
	{
	  loc = add_location_to_breakpoint (b, &sal);
	  if ((flags & CREATE_BREAKPOINT_FLAGS_INSERTED) != 0)
	    loc->inserted = 1;
	}

      if (b->cond_string)
	{
	  const char *arg = b->cond_string;

	  loc->cond = parse_exp_1 (&arg, loc->address,
				   block_for_pc (loc->address), 0);
	  if (*arg)
              error (_("Garbage '%s' follows condition"), arg);
	}

      /* Dynamic printf requires and uses additional arguments on the
	 command line, otherwise it's an error.  */
      if (type == bp_dprintf)
	{
	  if (b->extra_string)
	    update_dprintf_command_list (b);
	  else
	    error (_("Format string required"));
	}
      else if (b->extra_string)
	error (_("Garbage '%s' at end of command"), b->extra_string);
    }

  b->display_canonical = display_canonical;
  if (location != NULL)
    b->location = location;
  else
    {
      const char *addr_string = NULL;
      int addr_string_len = 0;

      if (location != NULL)
	addr_string = event_location_to_string (location);
      if (addr_string != NULL)
	addr_string_len = strlen (addr_string);

      b->location = new_address_location (b->loc->address,
					  addr_string, addr_string_len);
    }
  b->filter = filter;
}

static void
create_breakpoint_sal (struct gdbarch *gdbarch,
		       struct symtabs_and_lines sals,
		       struct event_location *location,
		       char *filter, char *cond_string,
		       char *extra_string,
		       enum bptype type, enum bpdisp disposition,
		       int thread, int task, int ignore_count,
		       const struct breakpoint_ops *ops, int from_tty,
		       int enabled, int internal, unsigned flags,
		       int display_canonical)
{
  struct breakpoint *b;
  struct cleanup *old_chain;

  if (is_tracepoint_type (type))
    {
      struct tracepoint *t;

      t = XCNEW (struct tracepoint);
      b = &t->base;
    }
  else
    b = XNEW (struct breakpoint);

  old_chain = make_cleanup (xfree, b);

  init_breakpoint_sal (b, gdbarch,
		       sals, location,
		       filter, cond_string, extra_string,
		       type, disposition,
		       thread, task, ignore_count,
		       ops, from_tty,
		       enabled, internal, flags,
		       display_canonical);
  discard_cleanups (old_chain);

  install_breakpoint (internal, b, 0);
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
			struct linespec_result *canonical,
			char *cond_string, char *extra_string,
			enum bptype type, enum bpdisp disposition,
			int thread, int task, int ignore_count,
			const struct breakpoint_ops *ops, int from_tty,
			int enabled, int internal, unsigned flags)
{
  int i;
  struct linespec_sals *lsal;

  if (canonical->pre_expanded)
    gdb_assert (VEC_length (linespec_sals, canonical->sals) == 1);

  for (i = 0; VEC_iterate (linespec_sals, canonical->sals, i, lsal); ++i)
    {
      /* Note that 'location' can be NULL in the case of a plain
	 'break', without arguments.  */
      struct event_location *location
	= (canonical->location != NULL
	   ? copy_event_location (canonical->location) : NULL);
      char *filter_string = lsal->canonical ? xstrdup (lsal->canonical) : NULL;
      struct cleanup *inner = make_cleanup_delete_event_location (location);

      make_cleanup (xfree, filter_string);
      create_breakpoint_sal (gdbarch, lsal->sals,
			     location,
			     filter_string,
			     cond_string, extra_string,
			     type, disposition,
			     thread, task, ignore_count, ops,
			     from_tty, enabled, internal, flags,
			     canonical->special_display);
      discard_cleanups (inner);
    }
}

/* Parse LOCATION which is assumed to be a SAL specification possibly
   followed by conditionals.  On return, SALS contains an array of SAL
   addresses found.  LOCATION points to the end of the SAL (for
   linespec locations).

   The array and the line spec strings are allocated on the heap, it is
   the caller's responsibility to free them.  */

static void
parse_breakpoint_sals (const struct event_location *location,
		       struct linespec_result *canonical)
{
  struct symtab_and_line cursal;

  if (event_location_type (location) == LINESPEC_LOCATION)
    {
      const char *address = get_linespec_location (location);

      if (address == NULL)
	{
	  /* The last displayed codepoint, if it's valid, is our default
	     breakpoint address.  */
	  if (last_displayed_sal_is_valid ())
	    {
	      struct linespec_sals lsal;
	      struct symtab_and_line sal;
	      CORE_ADDR pc;

	      init_sal (&sal);		/* Initialize to zeroes.  */
	      lsal.sals.sals = XNEW (struct symtab_and_line);

	      /* Set sal's pspace, pc, symtab, and line to the values
		 corresponding to the last call to print_frame_info.
		 Be sure to reinitialize LINE with NOTCURRENT == 0
		 as the breakpoint line number is inappropriate otherwise.
		 find_pc_line would adjust PC, re-set it back.  */
	      get_last_displayed_sal (&sal);
	      pc = sal.pc;
	      sal = find_pc_line (pc, 0);

	      /* "break" without arguments is equivalent to "break *PC"
		 where PC is the last displayed codepoint's address.  So
		 make sure to set sal.explicit_pc to prevent GDB from
		 trying to expand the list of sals to include all other
		 instances with the same symtab and line.  */
	      sal.pc = pc;
	      sal.explicit_pc = 1;

	      lsal.sals.sals[0] = sal;
	      lsal.sals.nelts = 1;
	      lsal.canonical = NULL;

	      VEC_safe_push (linespec_sals, canonical->sals, &lsal);
	      return;
	    }
	  else
	    error (_("No default breakpoint address now."));
	}
    }

  /* Force almost all breakpoints to be in terms of the
     current_source_symtab (which is decode_line_1's default).
     This should produce the results we want almost all of the
     time while leaving default_breakpoint_* alone.

     ObjC: However, don't match an Objective-C method name which
     may have a '+' or '-' succeeded by a '['.  */
  cursal = get_current_source_symtab_and_line ();
  if (last_displayed_sal_is_valid ())
    {
      const char *address = NULL;

      if (event_location_type (location) == LINESPEC_LOCATION)
	address = get_linespec_location (location);

      if (!cursal.symtab
	  || (address != NULL
	      && strchr ("+-", address[0]) != NULL
	      && address[1] != '['))
	{
	  decode_line_full (location, DECODE_LINE_FUNFIRSTLINE, NULL,
			    get_last_displayed_symtab (),
			    get_last_displayed_line (),
			    canonical, NULL, NULL);
	  return;
	}
    }

  decode_line_full (location, DECODE_LINE_FUNFIRSTLINE, NULL,
		    cursal.symtab, cursal.line, canonical, NULL, NULL);
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
      struct gdbarch *sarch;

      sal = &sals->sals[i];

      sarch = get_sal_arch (*sal);
      /* We fall back to GDBARCH if there is no architecture
	 associated with SAL.  */
      if (sarch == NULL)
	sarch = gdbarch;
      rslt = gdbarch_fast_tracepoint_valid_at (sarch, sal->pc, &msg);
      old_chain = make_cleanup (xfree, msg);

      if (!rslt)
	error (_("May not have a fast tracepoint at 0x%s%s"),
	       paddress (sarch, sal->pc), (msg ? msg : ""));

      do_cleanups (old_chain);
    }
}

/* Given TOK, a string specification of condition and thread, as
   accepted by the 'break' command, extract the condition
   string and thread number and set *COND_STRING and *THREAD.
   PC identifies the context at which the condition should be parsed.
   If no condition is found, *COND_STRING is set to NULL.
   If no thread is found, *THREAD is set to -1.  */

static void
find_condition_and_thread (const char *tok, CORE_ADDR pc,
			   char **cond_string, int *thread, int *task,
			   char **rest)
{
  *cond_string = NULL;
  *thread = -1;
  *task = 0;
  *rest = NULL;

  while (tok && *tok)
    {
      const char *end_tok;
      int toklen;
      const char *cond_start = NULL;
      const char *cond_end = NULL;

      tok = skip_spaces_const (tok);

      if ((*tok == '"' || *tok == ',') && rest)
	{
	  *rest = savestring (tok, strlen (tok));
	  return;
	}

      end_tok = skip_to_space_const (tok);

      toklen = end_tok - tok;

      if (toklen >= 1 && strncmp (tok, "if", toklen) == 0)
	{
	  struct expression *expr;

	  tok = cond_start = end_tok + 1;
	  expr = parse_exp_1 (&tok, pc, block_for_pc (pc), 0);
	  xfree (expr);
	  cond_end = tok;
	  *cond_string = savestring (cond_start, cond_end - cond_start);
	}
      else if (toklen >= 1 && strncmp (tok, "thread", toklen) == 0)
	{
	  const char *tmptok;
	  struct thread_info *thr;

	  tok = end_tok + 1;
	  thr = parse_thread_id (tok, &tmptok);
	  if (tok == tmptok)
	    error (_("Junk after thread keyword."));
	  *thread = thr->global_num;
	  tok = tmptok;
	}
      else if (toklen >= 1 && strncmp (tok, "task", toklen) == 0)
	{
	  char *tmptok;

	  tok = end_tok + 1;
	  *task = strtol (tok, &tmptok, 0);
	  if (tok == tmptok)
	    error (_("Junk after task keyword."));
	  if (!valid_task_id (*task))
	    error (_("Unknown task %d."), *task);
	  tok = tmptok;
	}
      else if (rest)
	{
	  *rest = savestring (tok, strlen (tok));
	  return;
	}
      else
	error (_("Junk at end of arguments."));
    }
}

/* Decode a static tracepoint marker spec.  */

static struct symtabs_and_lines
decode_static_tracepoint_spec (const char **arg_p)
{
  VEC(static_tracepoint_marker_p) *markers = NULL;
  struct symtabs_and_lines sals;
  struct cleanup *old_chain;
  const char *p = &(*arg_p)[3];
  const char *endp;
  char *marker_str;
  int i;

  p = skip_spaces_const (p);

  endp = skip_to_space_const (p);

  marker_str = savestring (p, endp - p);
  old_chain = make_cleanup (xfree, marker_str);

  markers = target_static_tracepoint_markers_by_strid (marker_str);
  if (VEC_empty(static_tracepoint_marker_p, markers))
    error (_("No known static tracepoint marker named %s"), marker_str);

  sals.nelts = VEC_length(static_tracepoint_marker_p, markers);
  sals.sals = XNEWVEC (struct symtab_and_line, sals.nelts);

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

/* See breakpoint.h.  */

int
create_breakpoint (struct gdbarch *gdbarch,
		   const struct event_location *location, char *cond_string,
		   int thread, char *extra_string,
		   int parse_extra,
		   int tempflag, enum bptype type_wanted,
		   int ignore_count,
		   enum auto_boolean pending_break_support,
		   const struct breakpoint_ops *ops,
		   int from_tty, int enabled, int internal,
		   unsigned flags)
{
  struct linespec_result canonical;
  struct cleanup *old_chain;
  struct cleanup *bkpt_chain = NULL;
  int pending = 0;
  int task = 0;
  int prev_bkpt_count = breakpoint_count;

  gdb_assert (ops != NULL);

  /* If extra_string isn't useful, set it to NULL.  */
  if (extra_string != NULL && *extra_string == '\0')
    extra_string = NULL;

  init_linespec_result (&canonical);

  TRY
    {
      ops->create_sals_from_location (location, &canonical, type_wanted);
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
      /* If caller is interested in rc value from parse, set
	 value.  */
      if (e.error == NOT_FOUND_ERROR)
	{
	  /* If pending breakpoint support is turned off, throw
	     error.  */

	  if (pending_break_support == AUTO_BOOLEAN_FALSE)
	    throw_exception (e);

	  exception_print (gdb_stderr, e);

          /* If pending breakpoint support is auto query and the user
	     selects no, then simply return the error code.  */
	  if (pending_break_support == AUTO_BOOLEAN_AUTO
	      && !nquery (_("Make %s pending on future shared library load? "),
			  bptype_string (type_wanted)))
	    return 0;

	  /* At this point, either the user was queried about setting
	     a pending breakpoint and selected yes, or pending
	     breakpoint behavior is on and thus a pending breakpoint
	     is defaulted on behalf of the user.  */
	  pending = 1;
	}
      else
	throw_exception (e);
    }
  END_CATCH

  if (!pending && VEC_empty (linespec_sals, canonical.sals))
    return 0;

  /* Create a chain of things that always need to be cleaned up.  */
  old_chain = make_cleanup_destroy_linespec_result (&canonical);

  /* ----------------------------- SNIP -----------------------------
     Anything added to the cleanup chain beyond this point is assumed
     to be part of a breakpoint.  If the breakpoint create succeeds
     then the memory is not reclaimed.  */
  bkpt_chain = make_cleanup (null_cleanup, 0);

  /* Resolve all line numbers to PC's and verify that the addresses
     are ok for the target.  */
  if (!pending)
    {
      int ix;
      struct linespec_sals *iter;

      for (ix = 0; VEC_iterate (linespec_sals, canonical.sals, ix, iter); ++ix)
	breakpoint_sals_to_pc (&iter->sals);
    }

  /* Fast tracepoints may have additional restrictions on location.  */
  if (!pending && type_wanted == bp_fast_tracepoint)
    {
      int ix;
      struct linespec_sals *iter;

      for (ix = 0; VEC_iterate (linespec_sals, canonical.sals, ix, iter); ++ix)
	check_fast_tracepoint_sals (gdbarch, &iter->sals);
    }

  /* Verify that condition can be parsed, before setting any
     breakpoints.  Allocate a separate condition expression for each
     breakpoint.  */
  if (!pending)
    {
      if (parse_extra)
        {
	  char *rest;
	  struct linespec_sals *lsal;

	  lsal = VEC_index (linespec_sals, canonical.sals, 0);

	  /* Here we only parse 'arg' to separate condition
	     from thread number, so parsing in context of first
	     sal is OK.  When setting the breakpoint we'll
	     re-parse it in context of each sal.  */

	  find_condition_and_thread (extra_string, lsal->sals.sals[0].pc,
				     &cond_string, &thread, &task, &rest);
	  if (cond_string)
	    make_cleanup (xfree, cond_string);
	  if (rest)
	    make_cleanup (xfree, rest);
	  if (rest)
	    extra_string = rest;
	  else
	    extra_string = NULL;
        }
      else
        {
	  if (type_wanted != bp_dprintf
	      && extra_string != NULL && *extra_string != '\0')
		error (_("Garbage '%s' at end of location"), extra_string);

	  /* Create a private copy of condition string.  */
	  if (cond_string)
	    {
	      cond_string = xstrdup (cond_string);
	      make_cleanup (xfree, cond_string);
	    }
	  /* Create a private copy of any extra string.  */
	  if (extra_string)
	    {
	      extra_string = xstrdup (extra_string);
	      make_cleanup (xfree, extra_string);
	    }
        }

      ops->create_breakpoints_sal (gdbarch, &canonical,
				   cond_string, extra_string, type_wanted,
				   tempflag ? disp_del : disp_donttouch,
				   thread, task, ignore_count, ops,
				   from_tty, enabled, internal, flags);
    }
  else
    {
      struct breakpoint *b;

      if (is_tracepoint_type (type_wanted))
	{
	  struct tracepoint *t;

	  t = XCNEW (struct tracepoint);
	  b = &t->base;
	}
      else
	b = XNEW (struct breakpoint);

      init_raw_breakpoint_without_location (b, gdbarch, type_wanted, ops);
      b->location = copy_event_location (location);

      if (parse_extra)
	b->cond_string = NULL;
      else
	{
	  /* Create a private copy of condition string.  */
	  if (cond_string)
	    {
	      cond_string = xstrdup (cond_string);
	      make_cleanup (xfree, cond_string);
	    }
	  b->cond_string = cond_string;
	  b->thread = thread;
	}

      /* Create a private copy of any extra string.  */
      if (extra_string != NULL)
	{
	  extra_string = xstrdup (extra_string);
	  make_cleanup (xfree, extra_string);
	}
      b->extra_string = extra_string;
      b->ignore_count = ignore_count;
      b->disposition = tempflag ? disp_del : disp_donttouch;
      b->condition_not_parsed = 1;
      b->enable_state = enabled ? bp_enabled : bp_disabled;
      if ((type_wanted != bp_breakpoint
           && type_wanted != bp_hardware_breakpoint) || thread != -1)
	b->pspace = current_program_space;

      install_breakpoint (internal, b, 0);
    }
  
  if (VEC_length (linespec_sals, canonical.sals) > 1)
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
  update_global_location_list (UGLL_MAY_INSERT);

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
  struct breakpoint_ops *ops;
  struct event_location *location;
  struct cleanup *cleanup;

  location = string_to_event_location (&arg, current_language);
  cleanup = make_cleanup_delete_event_location (location);

  /* Matching breakpoints on probes.  */
  if (location != NULL
      && event_location_type (location) == PROBE_LOCATION)
    ops = &bkpt_probe_breakpoint_ops;
  else
    ops = &bkpt_breakpoint_ops;

  create_breakpoint (get_current_arch (),
		     location,
		     NULL, 0, arg, 1 /* parse arg */,
		     tempflag, type_wanted,
		     0 /* Ignore count */,
		     pending_break_support,
		     ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */,
		     0);
  do_cleanups (cleanup);
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
	       sal->line, symtab_to_filename_for_display (sal->symtab));
      sal->pc = pc;

      /* If this SAL corresponds to a breakpoint inserted using a line
         number, then skip the function prologue if necessary.  */
      if (sal->explicit_line)
	skip_prologue_sal (sal);
    }

  if (sal->section == 0 && sal->symtab != NULL)
    {
      const struct blockvector *bv;
      const struct block *b;
      struct symbol *sym;

      bv = blockvector_for_pc_sect (sal->pc, 0, &b,
				    SYMTAB_COMPUNIT (sal->symtab));
      if (bv != NULL)
	{
	  sym = block_linkage_function (b);
	  if (sym != NULL)
	    {
	      fixup_symbol_section (sym, SYMTAB_OBJFILE (sal->symtab));
	      sal->section = SYMBOL_OBJ_SECTION (SYMTAB_OBJFILE (sal->symtab),
						 sym);
	    }
	  else
	    {
	      /* It really is worthwhile to have the section, so we'll
	         just have to look harder. This case can be executed
	         if we have line numbers but no functions (as can
	         happen in assembly source).  */

	      struct bound_minimal_symbol msym;
	      struct cleanup *old_chain = save_current_space_and_thread ();

	      switch_to_program_space_and_thread (sal->pspace);

	      msym = lookup_minimal_symbol_by_pc (sal->pc);
	      if (msym.minsym)
		sal->section = MSYMBOL_OBJ_SECTION (msym.objfile, msym.minsym);

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

/* The dynamic printf command is mostly like a regular breakpoint, but
   with a prewired command list consisting of a single output command,
   built from extra arguments supplied on the dprintf command
   line.  */

static void
dprintf_command (char *arg, int from_tty)
{
  struct event_location *location;
  struct cleanup *cleanup;

  location = string_to_event_location (&arg, current_language);
  cleanup = make_cleanup_delete_event_location (location);

  /* If non-NULL, ARG should have been advanced past the location;
     the next character must be ','.  */
  if (arg != NULL)
    {
      if (arg[0] != ',' || arg[1] == '\0')
	error (_("Format string required"));
      else
	{
	  /* Skip the comma.  */
	  ++arg;
	}
    }

  create_breakpoint (get_current_arch (),
		     location,
		     NULL, 0, arg, 1 /* parse arg */,
		     0, bp_dprintf,
		     0 /* Ignore count */,
		     pending_break_support,
		     &dprintf_breakpoint_ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */,
		     0);
  do_cleanups (cleanup);
}

static void
agent_printf_command (char *arg, int from_tty)
{
  error (_("May only run agent-printf on the target"));
}

/* Implement the "breakpoint_hit" breakpoint_ops method for
   ranged breakpoints.  */

static int
breakpoint_hit_ranged_breakpoint (const struct bp_location *bl,
				  struct address_space *aspace,
				  CORE_ADDR bp_addr,
				  const struct target_waitstatus *ws)
{
  if (ws->kind != TARGET_WAITKIND_STOPPED
      || ws->value.sig != GDB_SIGNAL_TRAP)
    return 0;

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
print_it_ranged_breakpoint (bpstat bs)
{
  struct breakpoint *b = bs->breakpoint_at;
  struct bp_location *bl = b->loc;
  struct ui_out *uiout = current_uiout;

  gdb_assert (b->type == bp_hardware_breakpoint);

  /* Ranged breakpoints have only one location.  */
  gdb_assert (bl && bl->next == NULL);

  annotate_breakpoint (b->number);

  maybe_print_thread_hit_breakpoint (uiout);

  if (b->disposition == disp_del)
    ui_out_text (uiout, "Temporary ranged breakpoint ");
  else
    ui_out_text (uiout, "Ranged breakpoint ");
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
  struct ui_out *uiout = current_uiout;

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
  struct ui_file *stb = mem_fileopen ();
  struct cleanup *cleanup = make_cleanup_ui_file_delete (stb);

  gdb_assert (bl);

  address_start = bl->address;
  address_end = address_start + bl->length - 1;

  ui_out_text (uiout, "\taddress range: ");
  fprintf_unfiltered (stb, "[%s, %s]",
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
  struct ui_out *uiout = current_uiout;

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
  fprintf_unfiltered (fp, "break-range %s, %s",
		      event_location_to_string (b->location),
		      event_location_to_string (b->location_range_end));
  print_recreate_thread (b, fp);
}

/* The breakpoint_ops structure to be used in ranged breakpoints.  */

static struct breakpoint_ops ranged_breakpoint_ops;

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
  char *arg_start, *addr_string_start;
  struct linespec_result canonical_start, canonical_end;
  int bp_count, can_use_bp, length;
  CORE_ADDR end;
  struct breakpoint *b;
  struct symtab_and_line sal_start, sal_end;
  struct cleanup *cleanup_bkpt;
  struct linespec_sals *lsal_start, *lsal_end;
  struct event_location *start_location, *end_location;

  /* We don't support software ranged breakpoints.  */
  if (target_ranged_break_num_registers () < 0)
    error (_("This target does not support hardware ranged breakpoints."));

  bp_count = hw_breakpoint_used_count ();
  bp_count += target_ranged_break_num_registers ();
  can_use_bp = target_can_use_hardware_watchpoint (bp_hardware_breakpoint,
						   bp_count, 0);
  if (can_use_bp < 0)
    error (_("Hardware breakpoints used exceeds limit."));

  arg = skip_spaces (arg);
  if (arg == NULL || arg[0] == '\0')
    error(_("No address range specified."));

  init_linespec_result (&canonical_start);

  arg_start = arg;
  start_location = string_to_event_location (&arg, current_language);
  cleanup_bkpt = make_cleanup_delete_event_location (start_location);
  parse_breakpoint_sals (start_location, &canonical_start);
  make_cleanup_destroy_linespec_result (&canonical_start);

  if (arg[0] != ',')
    error (_("Too few arguments."));
  else if (VEC_empty (linespec_sals, canonical_start.sals))
    error (_("Could not find location of the beginning of the range."));

  lsal_start = VEC_index (linespec_sals, canonical_start.sals, 0);

  if (VEC_length (linespec_sals, canonical_start.sals) > 1
      || lsal_start->sals.nelts != 1)
    error (_("Cannot create a ranged breakpoint with multiple locations."));

  sal_start = lsal_start->sals.sals[0];
  addr_string_start = savestring (arg_start, arg - arg_start);
  make_cleanup (xfree, addr_string_start);

  arg++;	/* Skip the comma.  */
  arg = skip_spaces (arg);

  /* Parse the end location.  */

  init_linespec_result (&canonical_end);
  arg_start = arg;

  /* We call decode_line_full directly here instead of using
     parse_breakpoint_sals because we need to specify the start location's
     symtab and line as the default symtab and line for the end of the
     range.  This makes it possible to have ranges like "foo.c:27, +14",
     where +14 means 14 lines from the start location.  */
  end_location = string_to_event_location (&arg, current_language);
  make_cleanup_delete_event_location (end_location);
  decode_line_full (end_location, DECODE_LINE_FUNFIRSTLINE, NULL,
		    sal_start.symtab, sal_start.line,
		    &canonical_end, NULL, NULL);

  make_cleanup_destroy_linespec_result (&canonical_end);

  if (VEC_empty (linespec_sals, canonical_end.sals))
    error (_("Could not find location of the end of the range."));

  lsal_end = VEC_index (linespec_sals, canonical_end.sals, 0);
  if (VEC_length (linespec_sals, canonical_end.sals) > 1
      || lsal_end->sals.nelts != 1)
    error (_("Cannot create a ranged breakpoint with multiple locations."));

  sal_end = lsal_end->sals.sals[0];

  end = find_breakpoint_range_end (sal_end);
  if (sal_start.pc > end)
    error (_("Invalid address range, end precedes start."));

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
			  bp_hardware_breakpoint, &ranged_breakpoint_ops);
  set_breakpoint_count (breakpoint_count + 1);
  b->number = breakpoint_count;
  b->disposition = disp_donttouch;
  b->location = copy_event_location (start_location);
  b->location_range_end = copy_event_location (end_location);
  b->loc->length = length;

  do_cleanups (cleanup_bkpt);

  mention (b);
  observer_notify_breakpoint_created (b);
  update_global_location_list (UGLL_MAY_INSERT);
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
	case TERNOP_COND:
	case TERNOP_SLICE:

	case OP_LONG:
	case OP_DOUBLE:
	case OP_DECFLOAT:
	case OP_LAST:
	case OP_COMPLEX:
	case OP_STRING:
	case OP_ARRAY:
	case OP_TYPE:
	case OP_TYPEOF:
	case OP_DECLTYPE:
	case OP_TYPEID:
	case OP_NAME:
	case OP_OBJC_NSSTRING:

	case UNOP_NEG:
	case UNOP_LOGICAL_NOT:
	case UNOP_COMPLEMENT:
	case UNOP_ADDR:
	case UNOP_HIGH:
	case UNOP_CAST:

	case UNOP_CAST_TYPE:
	case UNOP_REINTERPRET_CAST:
	case UNOP_DYNAMIC_CAST:
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

/* Implement the "dtor" breakpoint_ops method for watchpoints.  */

static void
dtor_watchpoint (struct breakpoint *self)
{
  struct watchpoint *w = (struct watchpoint *) self;

  xfree (w->cond_exp);
  xfree (w->exp);
  xfree (w->exp_string);
  xfree (w->exp_string_reparse);
  value_free (w->val);

  base_breakpoint_ops.dtor (self);
}

/* Implement the "re_set" breakpoint_ops method for watchpoints.  */

static void
re_set_watchpoint (struct breakpoint *b)
{
  struct watchpoint *w = (struct watchpoint *) b;

  /* Watchpoint can be either on expression using entirely global
     variables, or it can be on local variables.

     Watchpoints of the first kind are never auto-deleted, and even
     persist across program restarts.  Since they can use variables
     from shared libraries, we need to reparse expression as libraries
     are loaded and unloaded.

     Watchpoints on local variables can also change meaning as result
     of solib event.  For example, if a watchpoint uses both a local
     and a global variables in expression, it's a local watchpoint,
     but unloading of a shared library will make the expression
     invalid.  This is not a very common use case, but we still
     re-evaluate expression, to avoid surprises to the user.

     Note that for local watchpoints, we re-evaluate it only if
     watchpoints frame id is still valid.  If it's not, it means the
     watchpoint is out of scope and will be deleted soon.  In fact,
     I'm not sure we'll ever be called in this case.

     If a local watchpoint's frame id is still valid, then
     w->exp_valid_block is likewise valid, and we can safely use it.

     Don't do anything about disabled watchpoints, since they will be
     reevaluated again when enabled.  */
  update_watchpoint (w, 1 /* reparse */);
}

/* Implement the "insert" breakpoint_ops method for hardware watchpoints.  */

static int
insert_watchpoint (struct bp_location *bl)
{
  struct watchpoint *w = (struct watchpoint *) bl->owner;
  int length = w->exact ? 1 : bl->length;

  return target_insert_watchpoint (bl->address, length, bl->watchpoint_type,
				   w->cond_exp);
}

/* Implement the "remove" breakpoint_ops method for hardware watchpoints.  */

static int
remove_watchpoint (struct bp_location *bl, enum remove_bp_reason reason)
{
  struct watchpoint *w = (struct watchpoint *) bl->owner;
  int length = w->exact ? 1 : bl->length;

  return target_remove_watchpoint (bl->address, length, bl->watchpoint_type,
				   w->cond_exp);
}

static int
breakpoint_hit_watchpoint (const struct bp_location *bl,
			   struct address_space *aspace, CORE_ADDR bp_addr,
			   const struct target_waitstatus *ws)
{
  struct breakpoint *b = bl->owner;
  struct watchpoint *w = (struct watchpoint *) b;

  /* Continuable hardware watchpoints are treated as non-existent if the
     reason we stopped wasn't a hardware watchpoint (we didn't stop on
     some data address).  Otherwise gdb won't stop on a break instruction
     in the code (not from a breakpoint) when a hardware watchpoint has
     been defined.  Also skip watchpoints which we know did not trigger
     (did not match the data address).  */
  if (is_hardware_watchpoint (b)
      && w->watchpoint_triggered == watch_triggered_no)
    return 0;

  return 1;
}

static void
check_status_watchpoint (bpstat bs)
{
  gdb_assert (is_watchpoint (bs->breakpoint_at));

  bpstat_check_watchpoint (bs);
}

/* Implement the "resources_needed" breakpoint_ops method for
   hardware watchpoints.  */

static int
resources_needed_watchpoint (const struct bp_location *bl)
{
  struct watchpoint *w = (struct watchpoint *) bl->owner;
  int length = w->exact? 1 : bl->length;

  return target_region_ok_for_hw_watchpoint (bl->address, length);
}

/* Implement the "works_in_software_mode" breakpoint_ops method for
   hardware watchpoints.  */

static int
works_in_software_mode_watchpoint (const struct breakpoint *b)
{
  /* Read and access watchpoints only work with hardware support.  */
  return b->type == bp_watchpoint || b->type == bp_hardware_watchpoint;
}

static enum print_stop_action
print_it_watchpoint (bpstat bs)
{
  struct cleanup *old_chain;
  struct breakpoint *b;
  struct ui_file *stb;
  enum print_stop_action result;
  struct watchpoint *w;
  struct ui_out *uiout = current_uiout;

  gdb_assert (bs->bp_location_at != NULL);

  b = bs->breakpoint_at;
  w = (struct watchpoint *) b;

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  annotate_watchpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);

  switch (b->type)
    {
    case bp_watchpoint:
    case bp_hardware_watchpoint:
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason",
	   async_reason_lookup (EXEC_ASYNC_WATCHPOINT_TRIGGER));
      mention (b);
      make_cleanup_ui_out_tuple_begin_end (uiout, "value");
      ui_out_text (uiout, "\nOld value = ");
      watchpoint_value_print (bs->old_val, stb);
      ui_out_field_stream (uiout, "old", stb);
      ui_out_text (uiout, "\nNew value = ");
      watchpoint_value_print (w->val, stb);
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
      watchpoint_value_print (w->val, stb);
      ui_out_field_stream (uiout, "value", stb);
      ui_out_text (uiout, "\n");
      result = PRINT_UNKNOWN;
      break;

    case bp_access_watchpoint:
      if (bs->old_val != NULL)
	{
	  if (ui_out_is_mi_like_p (uiout))
	    ui_out_field_string
	      (uiout, "reason",
	       async_reason_lookup (EXEC_ASYNC_ACCESS_WATCHPOINT_TRIGGER));
	  mention (b);
	  make_cleanup_ui_out_tuple_begin_end (uiout, "value");
	  ui_out_text (uiout, "\nOld value = ");
	  watchpoint_value_print (bs->old_val, stb);
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
      watchpoint_value_print (w->val, stb);
      ui_out_field_stream (uiout, "new", stb);
      ui_out_text (uiout, "\n");
      result = PRINT_UNKNOWN;
      break;
    default:
      result = PRINT_UNKNOWN;
    }

  do_cleanups (old_chain);
  return result;
}

/* Implement the "print_mention" breakpoint_ops method for hardware
   watchpoints.  */

static void
print_mention_watchpoint (struct breakpoint *b)
{
  struct cleanup *ui_out_chain;
  struct watchpoint *w = (struct watchpoint *) b;
  struct ui_out *uiout = current_uiout;

  switch (b->type)
    {
    case bp_watchpoint:
      ui_out_text (uiout, "Watchpoint ");
      ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "wpt");
      break;
    case bp_hardware_watchpoint:
      ui_out_text (uiout, "Hardware watchpoint ");
      ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "wpt");
      break;
    case bp_read_watchpoint:
      ui_out_text (uiout, "Hardware read watchpoint ");
      ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "hw-rwpt");
      break;
    case bp_access_watchpoint:
      ui_out_text (uiout, "Hardware access (read/write) watchpoint ");
      ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "hw-awpt");
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("Invalid hardware watchpoint type."));
    }

  ui_out_field_int (uiout, "number", b->number);
  ui_out_text (uiout, ": ");
  ui_out_field_string (uiout, "exp", w->exp_string);
  do_cleanups (ui_out_chain);
}

/* Implement the "print_recreate" breakpoint_ops method for
   watchpoints.  */

static void
print_recreate_watchpoint (struct breakpoint *b, struct ui_file *fp)
{
  struct watchpoint *w = (struct watchpoint *) b;

  switch (b->type)
    {
    case bp_watchpoint:
    case bp_hardware_watchpoint:
      fprintf_unfiltered (fp, "watch");
      break;
    case bp_read_watchpoint:
      fprintf_unfiltered (fp, "rwatch");
      break;
    case bp_access_watchpoint:
      fprintf_unfiltered (fp, "awatch");
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("Invalid watchpoint type."));
    }

  fprintf_unfiltered (fp, " %s", w->exp_string);
  print_recreate_thread (b, fp);
}

/* Implement the "explains_signal" breakpoint_ops method for
   watchpoints.  */

static int
explains_signal_watchpoint (struct breakpoint *b, enum gdb_signal sig)
{
  /* A software watchpoint cannot cause a signal other than
     GDB_SIGNAL_TRAP.  */
  if (b->type == bp_watchpoint && sig != GDB_SIGNAL_TRAP)
    return 0;

  return 1;
}

/* The breakpoint_ops structure to be used in hardware watchpoints.  */

static struct breakpoint_ops watchpoint_breakpoint_ops;

/* Implement the "insert" breakpoint_ops method for
   masked hardware watchpoints.  */

static int
insert_masked_watchpoint (struct bp_location *bl)
{
  struct watchpoint *w = (struct watchpoint *) bl->owner;

  return target_insert_mask_watchpoint (bl->address, w->hw_wp_mask,
					bl->watchpoint_type);
}

/* Implement the "remove" breakpoint_ops method for
   masked hardware watchpoints.  */

static int
remove_masked_watchpoint (struct bp_location *bl, enum remove_bp_reason reason)
{
  struct watchpoint *w = (struct watchpoint *) bl->owner;

  return target_remove_mask_watchpoint (bl->address, w->hw_wp_mask,
				        bl->watchpoint_type);
}

/* Implement the "resources_needed" breakpoint_ops method for
   masked hardware watchpoints.  */

static int
resources_needed_masked_watchpoint (const struct bp_location *bl)
{
  struct watchpoint *w = (struct watchpoint *) bl->owner;

  return target_masked_watch_num_registers (bl->address, w->hw_wp_mask);
}

/* Implement the "works_in_software_mode" breakpoint_ops method for
   masked hardware watchpoints.  */

static int
works_in_software_mode_masked_watchpoint (const struct breakpoint *b)
{
  return 0;
}

/* Implement the "print_it" breakpoint_ops method for
   masked hardware watchpoints.  */

static enum print_stop_action
print_it_masked_watchpoint (bpstat bs)
{
  struct breakpoint *b = bs->breakpoint_at;
  struct ui_out *uiout = current_uiout;

  /* Masked watchpoints have only one location.  */
  gdb_assert (b->loc && b->loc->next == NULL);

  annotate_watchpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);

  switch (b->type)
    {
    case bp_hardware_watchpoint:
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason",
	   async_reason_lookup (EXEC_ASYNC_WATCHPOINT_TRIGGER));
      break;

    case bp_read_watchpoint:
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason",
	   async_reason_lookup (EXEC_ASYNC_READ_WATCHPOINT_TRIGGER));
      break;

    case bp_access_watchpoint:
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string
	  (uiout, "reason",
	   async_reason_lookup (EXEC_ASYNC_ACCESS_WATCHPOINT_TRIGGER));
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("Invalid hardware watchpoint type."));
    }

  mention (b);
  ui_out_text (uiout, _("\n\
Check the underlying instruction at PC for the memory\n\
address and value which triggered this watchpoint.\n"));
  ui_out_text (uiout, "\n");

  /* More than one watchpoint may have been triggered.  */
  return PRINT_UNKNOWN;
}

/* Implement the "print_one_detail" breakpoint_ops method for
   masked hardware watchpoints.  */

static void
print_one_detail_masked_watchpoint (const struct breakpoint *b,
				    struct ui_out *uiout)
{
  struct watchpoint *w = (struct watchpoint *) b;

  /* Masked watchpoints have only one location.  */
  gdb_assert (b->loc && b->loc->next == NULL);

  ui_out_text (uiout, "\tmask ");
  ui_out_field_core_addr (uiout, "mask", b->loc->gdbarch, w->hw_wp_mask);
  ui_out_text (uiout, "\n");
}

/* Implement the "print_mention" breakpoint_ops method for
   masked hardware watchpoints.  */

static void
print_mention_masked_watchpoint (struct breakpoint *b)
{
  struct watchpoint *w = (struct watchpoint *) b;
  struct ui_out *uiout = current_uiout;
  struct cleanup *ui_out_chain;

  switch (b->type)
    {
    case bp_hardware_watchpoint:
      ui_out_text (uiout, "Masked hardware watchpoint ");
      ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "wpt");
      break;
    case bp_read_watchpoint:
      ui_out_text (uiout, "Masked hardware read watchpoint ");
      ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "hw-rwpt");
      break;
    case bp_access_watchpoint:
      ui_out_text (uiout, "Masked hardware access (read/write) watchpoint ");
      ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "hw-awpt");
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("Invalid hardware watchpoint type."));
    }

  ui_out_field_int (uiout, "number", b->number);
  ui_out_text (uiout, ": ");
  ui_out_field_string (uiout, "exp", w->exp_string);
  do_cleanups (ui_out_chain);
}

/* Implement the "print_recreate" breakpoint_ops method for
   masked hardware watchpoints.  */

static void
print_recreate_masked_watchpoint (struct breakpoint *b, struct ui_file *fp)
{
  struct watchpoint *w = (struct watchpoint *) b;
  char tmp[40];

  switch (b->type)
    {
    case bp_hardware_watchpoint:
      fprintf_unfiltered (fp, "watch");
      break;
    case bp_read_watchpoint:
      fprintf_unfiltered (fp, "rwatch");
      break;
    case bp_access_watchpoint:
      fprintf_unfiltered (fp, "awatch");
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("Invalid hardware watchpoint type."));
    }

  sprintf_vma (tmp, w->hw_wp_mask);
  fprintf_unfiltered (fp, " %s mask 0x%s", w->exp_string, tmp);
  print_recreate_thread (b, fp);
}

/* The breakpoint_ops structure to be used in masked hardware watchpoints.  */

static struct breakpoint_ops masked_watchpoint_breakpoint_ops;

/* Tell whether the given watchpoint is a masked hardware watchpoint.  */

static int
is_masked_watchpoint (const struct breakpoint *b)
{
  return b->ops == &masked_watchpoint_breakpoint_ops;
}

/* accessflag:  hw_write:  watch write, 
                hw_read:   watch read, 
		hw_access: watch access (read or write) */
static void
watch_command_1 (const char *arg, int accessflag, int from_tty,
		 int just_location, int internal)
{
  struct breakpoint *b, *scope_breakpoint = NULL;
  struct expression *exp;
  const struct block *exp_valid_block = NULL, *cond_exp_valid_block = NULL;
  struct value *val, *mark, *result;
  int saved_bitpos = 0, saved_bitsize = 0;
  struct frame_info *frame;
  const char *exp_start = NULL;
  const char *exp_end = NULL;
  const char *tok, *end_tok;
  int toklen = -1;
  const char *cond_start = NULL;
  const char *cond_end = NULL;
  enum bptype bp_type;
  int thread = -1;
  int pc = 0;
  /* Flag to indicate whether we are going to use masks for
     the hardware watchpoint.  */
  int use_mask = 0;
  CORE_ADDR mask = 0;
  struct watchpoint *w;
  char *expression;
  struct cleanup *back_to;

  /* Make sure that we actually have parameters to parse.  */
  if (arg != NULL && arg[0] != '\0')
    {
      const char *value_start;

      exp_end = arg + strlen (arg);

      /* Look for "parameter value" pairs at the end
	 of the arguments string.  */
      for (tok = exp_end - 1; tok > arg; tok--)
	{
	  /* Skip whitespace at the end of the argument list.  */
	  while (tok > arg && (*tok == ' ' || *tok == '\t'))
	    tok--;

	  /* Find the beginning of the last token.
	     This is the value of the parameter.  */
	  while (tok > arg && (*tok != ' ' && *tok != '\t'))
	    tok--;
	  value_start = tok + 1;

	  /* Skip whitespace.  */
	  while (tok > arg && (*tok == ' ' || *tok == '\t'))
	    tok--;

	  end_tok = tok;

	  /* Find the beginning of the second to last token.
	     This is the parameter itself.  */
	  while (tok > arg && (*tok != ' ' && *tok != '\t'))
	    tok--;
	  tok++;
	  toklen = end_tok - tok + 1;

	  if (toklen == 6 && startswith (tok, "thread"))
	    {
	      struct thread_info *thr;
	      /* At this point we've found a "thread" token, which means
		 the user is trying to set a watchpoint that triggers
		 only in a specific thread.  */
	      const char *endp;

	      if (thread != -1)
		error(_("You can specify only one thread."));

	      /* Extract the thread ID from the next token.  */
	      thr = parse_thread_id (value_start, &endp);

	      /* Check if the user provided a valid thread ID.  */
	      if (*endp != ' ' && *endp != '\t' && *endp != '\0')
		invalid_thread_id_error (value_start);

	      thread = thr->global_num;
	    }
	  else if (toklen == 4 && startswith (tok, "mask"))
	    {
	      /* We've found a "mask" token, which means the user wants to
		 create a hardware watchpoint that is going to have the mask
		 facility.  */
	      struct value *mask_value, *mark;

	      if (use_mask)
		error(_("You can specify only one mask."));

	      use_mask = just_location = 1;

	      mark = value_mark ();
	      mask_value = parse_to_comma_and_eval (&value_start);
	      mask = value_as_address (mask_value);
	      value_free_to_mark (mark);
	    }
	  else
	    /* We didn't recognize what we found.  We should stop here.  */
	    break;

	  /* Truncate the string and get rid of the "parameter value" pair before
	     the arguments string is parsed by the parse_exp_1 function.  */
	  exp_end = tok;
	}
    }
  else
    exp_end = arg;

  /* Parse the rest of the arguments.  From here on out, everything
     is in terms of a newly allocated string instead of the original
     ARG.  */
  innermost_block = NULL;
  expression = savestring (arg, exp_end - arg);
  back_to = make_cleanup (xfree, expression);
  exp_start = arg = expression;
  exp = parse_exp_1 (&arg, 0, 0, 0);
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
  fetch_subexp_value (exp, &pc, &val, &result, NULL, just_location);

  if (val != NULL && just_location)
    {
      saved_bitpos = value_bitpos (val);
      saved_bitsize = value_bitsize (val);
    }

  if (just_location)
    {
      int ret;

      exp_valid_block = NULL;
      val = value_addr (result);
      release_value (val);
      value_free_to_mark (mark);

      if (use_mask)
	{
	  ret = target_masked_watch_num_registers (value_as_address (val),
						   mask);
	  if (ret == -1)
	    error (_("This target does not support masked watchpoints."));
	  else if (ret == -2)
	    error (_("Invalid mask or memory region."));
	}
    }
  else if (val != NULL)
    release_value (val);

  tok = skip_spaces_const (arg);
  end_tok = skip_to_space_const (tok);

  toklen = end_tok - tok;
  if (toklen >= 1 && strncmp (tok, "if", toklen) == 0)
    {
      struct expression *cond;

      innermost_block = NULL;
      tok = cond_start = end_tok + 1;
      cond = parse_exp_1 (&tok, 0, 0, 0);

      /* The watchpoint expression may not be local, but the condition
	 may still be.  E.g.: `watch global if local > 0'.  */
      cond_exp_valid_block = innermost_block;

      xfree (cond);
      cond_end = tok;
    }
  if (*tok)
    error (_("Junk at end of command."));

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
					  bp_watchpoint_scope,
					  &momentary_breakpoint_ops);

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

  /* Now set up the breakpoint.  We create all watchpoints as hardware
     watchpoints here even if hardware watchpoints are turned off, a call
     to update_watchpoint later in this function will cause the type to
     drop back to bp_watchpoint (software watchpoint) if required.  */

  if (accessflag == hw_read)
    bp_type = bp_read_watchpoint;
  else if (accessflag == hw_access)
    bp_type = bp_access_watchpoint;
  else
    bp_type = bp_hardware_watchpoint;

  w = XCNEW (struct watchpoint);
  b = &w->base;
  if (use_mask)
    init_raw_breakpoint_without_location (b, NULL, bp_type,
					  &masked_watchpoint_breakpoint_ops);
  else
    init_raw_breakpoint_without_location (b, NULL, bp_type,
					  &watchpoint_breakpoint_ops);
  b->thread = thread;
  b->disposition = disp_donttouch;
  b->pspace = current_program_space;
  w->exp = exp;
  w->exp_valid_block = exp_valid_block;
  w->cond_exp_valid_block = cond_exp_valid_block;
  if (just_location)
    {
      struct type *t = value_type (val);
      CORE_ADDR addr = value_as_address (val);
      char *name;

      t = check_typedef (TYPE_TARGET_TYPE (check_typedef (t)));
      name = type_to_string (t);

      w->exp_string_reparse = xstrprintf ("* (%s *) %s", name,
					  core_addr_to_string (addr));
      xfree (name);

      w->exp_string = xstrprintf ("-location %.*s",
				  (int) (exp_end - exp_start), exp_start);

      /* The above expression is in C.  */
      b->language = language_c;
    }
  else
    w->exp_string = savestring (exp_start, exp_end - exp_start);

  if (use_mask)
    {
      w->hw_wp_mask = mask;
    }
  else
    {
      w->val = val;
      w->val_bitpos = saved_bitpos;
      w->val_bitsize = saved_bitsize;
      w->val_valid = 1;
    }

  if (cond_start)
    b->cond_string = savestring (cond_start, cond_end - cond_start);
  else
    b->cond_string = 0;

  if (frame)
    {
      w->watchpoint_frame = get_frame_id (frame);
      w->watchpoint_thread = inferior_ptid;
    }
  else
    {
      w->watchpoint_frame = null_frame_id;
      w->watchpoint_thread = null_ptid;
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

  TRY
    {
      /* Finally update the new watchpoint.  This creates the locations
	 that should be inserted.  */
      update_watchpoint (w, 1);
    }
  CATCH (e, RETURN_MASK_ALL)
    {
      delete_breakpoint (b);
      throw_exception (e);
    }
  END_CATCH

  install_breakpoint (internal, b, 1);
  do_cleanups (back_to);
}

/* Return count of debug registers needed to watch the given expression.
   If the watchpoint cannot be handled in hardware return zero.  */

static int
can_use_hardware_watchpoint (struct value *v)
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

		  len = (target_exact_watchpoints
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


/* Data for the FSM that manages the until(location)/advance commands
   in infcmd.c.  Here because it uses the mechanisms of
   breakpoints.  */

struct until_break_fsm
{
  /* The base class.  */
  struct thread_fsm thread_fsm;

  /* The thread that as current when the command was executed.  */
  int thread;

  /* The breakpoint set at the destination location.  */
  struct breakpoint *location_breakpoint;

  /* Breakpoint set at the return address in the caller frame.  May be
     NULL.  */
  struct breakpoint *caller_breakpoint;
};

static void until_break_fsm_clean_up (struct thread_fsm *self,
				      struct thread_info *thread);
static int until_break_fsm_should_stop (struct thread_fsm *self,
					struct thread_info *thread);
static enum async_reply_reason
  until_break_fsm_async_reply_reason (struct thread_fsm *self);

/* until_break_fsm's vtable.  */

static struct thread_fsm_ops until_break_fsm_ops =
{
  NULL, /* dtor */
  until_break_fsm_clean_up,
  until_break_fsm_should_stop,
  NULL, /* return_value */
  until_break_fsm_async_reply_reason,
};

/* Allocate a new until_break_command_fsm.  */

static struct until_break_fsm *
new_until_break_fsm (struct interp *cmd_interp, int thread,
		     struct breakpoint *location_breakpoint,
		     struct breakpoint *caller_breakpoint)
{
  struct until_break_fsm *sm;

  sm = XCNEW (struct until_break_fsm);
  thread_fsm_ctor (&sm->thread_fsm, &until_break_fsm_ops, cmd_interp);

  sm->thread = thread;
  sm->location_breakpoint = location_breakpoint;
  sm->caller_breakpoint = caller_breakpoint;

  return sm;
}

/* Implementation of the 'should_stop' FSM method for the
   until(location)/advance commands.  */

static int
until_break_fsm_should_stop (struct thread_fsm *self,
			     struct thread_info *tp)
{
  struct until_break_fsm *sm = (struct until_break_fsm *) self;

  if (bpstat_find_breakpoint (tp->control.stop_bpstat,
			      sm->location_breakpoint) != NULL
      || (sm->caller_breakpoint != NULL
	  && bpstat_find_breakpoint (tp->control.stop_bpstat,
				     sm->caller_breakpoint) != NULL))
    thread_fsm_set_finished (self);

  return 1;
}

/* Implementation of the 'clean_up' FSM method for the
   until(location)/advance commands.  */

static void
until_break_fsm_clean_up (struct thread_fsm *self,
			  struct thread_info *thread)
{
  struct until_break_fsm *sm = (struct until_break_fsm *) self;

  /* Clean up our temporary breakpoints.  */
  if (sm->location_breakpoint != NULL)
    {
      delete_breakpoint (sm->location_breakpoint);
      sm->location_breakpoint = NULL;
    }
  if (sm->caller_breakpoint != NULL)
    {
      delete_breakpoint (sm->caller_breakpoint);
      sm->caller_breakpoint = NULL;
    }
  delete_longjmp_breakpoint (sm->thread);
}

/* Implementation of the 'async_reply_reason' FSM method for the
   until(location)/advance commands.  */

static enum async_reply_reason
until_break_fsm_async_reply_reason (struct thread_fsm *self)
{
  return EXEC_ASYNC_LOCATION_REACHED;
}

void
until_break_command (char *arg, int from_tty, int anywhere)
{
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  struct frame_info *frame;
  struct gdbarch *frame_gdbarch;
  struct frame_id stack_frame_id;
  struct frame_id caller_frame_id;
  struct breakpoint *location_breakpoint;
  struct breakpoint *caller_breakpoint = NULL;
  struct cleanup *old_chain, *cleanup;
  int thread;
  struct thread_info *tp;
  struct event_location *location;
  struct until_break_fsm *sm;

  clear_proceed_status (0);

  /* Set a breakpoint where the user wants it and at return from
     this function.  */

  location = string_to_event_location (&arg, current_language);
  cleanup = make_cleanup_delete_event_location (location);

  if (last_displayed_sal_is_valid ())
    sals = decode_line_1 (location, DECODE_LINE_FUNFIRSTLINE, NULL,
			  get_last_displayed_symtab (),
			  get_last_displayed_line ());
  else
    sals = decode_line_1 (location, DECODE_LINE_FUNFIRSTLINE,
			  NULL, (struct symtab *) NULL, 0);

  if (sals.nelts != 1)
    error (_("Couldn't get information on specified line."));

  sal = sals.sals[0];
  xfree (sals.sals);	/* malloc'd, so freed.  */

  if (*arg)
    error (_("Junk at end of arguments."));

  resolve_sal_pc (&sal);

  tp = inferior_thread ();
  thread = tp->global_num;

  old_chain = make_cleanup (null_cleanup, NULL);

  /* Note linespec handling above invalidates the frame chain.
     Installing a breakpoint also invalidates the frame chain (as it
     may need to switch threads), so do any frame handling before
     that.  */

  frame = get_selected_frame (NULL);
  frame_gdbarch = get_frame_arch (frame);
  stack_frame_id = get_stack_frame_id (frame);
  caller_frame_id = frame_unwind_caller_id (frame);

  /* Keep within the current frame, or in frames called by the current
     one.  */

  if (frame_id_p (caller_frame_id))
    {
      struct symtab_and_line sal2;
      struct gdbarch *caller_gdbarch;

      sal2 = find_pc_line (frame_unwind_caller_pc (frame), 0);
      sal2.pc = frame_unwind_caller_pc (frame);
      caller_gdbarch = frame_unwind_caller_arch (frame);
      caller_breakpoint = set_momentary_breakpoint (caller_gdbarch,
						    sal2,
						    caller_frame_id,
						    bp_until);
      make_cleanup_delete_breakpoint (caller_breakpoint);

      set_longjmp_breakpoint (tp, caller_frame_id);
      make_cleanup (delete_longjmp_breakpoint_cleanup, &thread);
    }

  /* set_momentary_breakpoint could invalidate FRAME.  */
  frame = NULL;

  if (anywhere)
    /* If the user told us to continue until a specified location,
       we don't specify a frame at which we need to stop.  */
    location_breakpoint = set_momentary_breakpoint (frame_gdbarch, sal,
						    null_frame_id, bp_until);
  else
    /* Otherwise, specify the selected frame, because we want to stop
       only at the very same frame.  */
    location_breakpoint = set_momentary_breakpoint (frame_gdbarch, sal,
						    stack_frame_id, bp_until);
  make_cleanup_delete_breakpoint (location_breakpoint);

  sm = new_until_break_fsm (command_interp (), tp->global_num,
			    location_breakpoint, caller_breakpoint);
  tp->thread_fsm = &sm->thread_fsm;

  discard_cleanups (old_chain);

  proceed (-1, GDB_SIGNAL_DEFAULT);

  do_cleanups (cleanup);
}

/* This function attempts to parse an optional "if <cond>" clause
   from the arg string.  If one is not found, it returns NULL.

   Else, it returns a pointer to the condition string.  (It does not
   attempt to evaluate the string against a particular block.)  And,
   it updates arg to point to the first character following the parsed
   if clause in the arg string.  */

char *
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
  struct exec_catchpoint *c;
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

  c = XNEW (struct exec_catchpoint);
  init_catchpoint (&c->base, gdbarch, tempflag, cond_string,
		   &catch_exec_breakpoint_ops);
  c->exec_pathname = NULL;

  install_breakpoint (0, &c->base, 1);
}

void
init_ada_exception_breakpoint (struct breakpoint *b,
			       struct gdbarch *gdbarch,
			       struct symtab_and_line sal,
			       char *addr_string,
			       const struct breakpoint_ops *ops,
			       int tempflag,
			       int enabled,
			       int from_tty)
{
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
         inappropriate, we should use the word catchpoint, and tell
         the user what type of catchpoint it is.  The above is good
         enough for now, though.  */
    }

  init_raw_breakpoint (b, gdbarch, sal, bp_breakpoint, ops);

  b->enable_state = enabled ? bp_enabled : bp_disabled;
  b->disposition = tempflag ? disp_del : disp_donttouch;
  b->location = string_to_event_location (&addr_string,
					  language_def (language_ada));
  b->language = language_ada;
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

/* A qsort comparison function that sorts breakpoints in order.  */

static int
compare_breakpoints (const void *a, const void *b)
{
  const breakpoint_p *ba = (const breakpoint_p *) a;
  uintptr_t ua = (uintptr_t) *ba;
  const breakpoint_p *bb = (const breakpoint_p *) b;
  uintptr_t ub = (uintptr_t) *bb;

  if ((*ba)->number < (*bb)->number)
    return -1;
  else if ((*ba)->number > (*bb)->number)
    return 1;

  /* Now sort by address, in case we see, e..g, two breakpoints with
     the number 0.  */
  if (ua < ub)
    return -1;
  return ua > ub ? 1 : 0;
}

/* Delete breakpoints by address or line.  */

static void
clear_command (char *arg, int from_tty)
{
  struct breakpoint *b, *prev;
  VEC(breakpoint_p) *found = 0;
  int ix;
  int default_match;
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  int i;
  struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);

  if (arg)
    {
      sals = decode_line_with_current_source (arg,
					      (DECODE_LINE_FUNFIRSTLINE
					       | DECODE_LINE_LIST_MODE));
      make_cleanup (xfree, sals.sals);
      default_match = 0;
    }
  else
    {
      sals.sals = XNEW (struct symtab_and_line);
      make_cleanup (xfree, sals.sals);
      init_sal (&sal);		/* Initialize to zeroes.  */

      /* Set sal's line, symtab, pc, and pspace to the values
	 corresponding to the last call to print_frame_info.  If the
	 codepoint is not valid, this will set all the fields to 0.  */
      get_last_displayed_sal (&sal);
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
  make_cleanup (VEC_cleanup (breakpoint_p), &found);
  for (i = 0; i < sals.nelts; i++)
    {
      const char *sal_fullname;

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
      sal_fullname = (sal.symtab == NULL
		      ? NULL : symtab_to_fullname (sal.symtab));

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
		  /* If the user specified file:line, don't allow a PC
		     match.  This matches historical gdb behavior.  */
		  int pc_match = (!sal.explicit_line
				  && sal.pc
				  && (loc->pspace == sal.pspace)
				  && (loc->address == sal.pc)
				  && (!section_is_overlay (loc->section)
				      || loc->section == sal.section));
		  int line_match = 0;

		  if ((default_match || sal.explicit_line)
		      && loc->symtab != NULL
		      && sal_fullname != NULL
		      && sal.pspace == loc->pspace
		      && loc->line_number == sal.line
		      && filename_cmp (symtab_to_fullname (loc->symtab),
				       sal_fullname) == 0)
		    line_match = 1;

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

  /* Remove duplicates from the vec.  */
  qsort (VEC_address (breakpoint_p, found),
	 VEC_length (breakpoint_p, found),
	 sizeof (breakpoint_p),
	 compare_breakpoints);
  prev = VEC_index (breakpoint_p, found, 0);
  for (ix = 1; VEC_iterate (breakpoint_p, found, ix, b); ++ix)
    {
      if (b == prev)
	{
	  VEC_ordered_remove (breakpoint_p, found, ix);
	  --ix;
	}
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

  for (ix = 0; VEC_iterate(breakpoint_p, found, ix, b); ix++)
    {
      if (from_tty)
	printf_unfiltered ("%d ", b->number);
      delete_breakpoint (b);
    }
  if (from_tty)
    putchar_unfiltered ('\n');

  do_cleanups (cleanups);
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
   secondarily by ordering first permanent elements and
   terciarily just ensuring the array is sorted stable way despite
   qsort being an unstable algorithm.  */

static int
bp_location_compare (const void *ap, const void *bp)
{
  const struct bp_location *a = *(const struct bp_location **) ap;
  const struct bp_location *b = *(const struct bp_location **) bp;

  if (a->address != b->address)
    return (a->address > b->address) - (a->address < b->address);

  /* Sort locations at the same address by their pspace number, keeping
     locations of the same inferior (in a multi-inferior environment)
     grouped.  */

  if (a->pspace->num != b->pspace->num)
    return ((a->pspace->num > b->pspace->num)
	    - (a->pspace->num < b->pspace->num));

  /* Sort permanent breakpoints first.  */
  if (a->permanent != b->permanent)
    return (a->permanent < b->permanent) - (a->permanent > b->permanent);

  /* Make the internal GDB representation stable across GDB runs
     where A and B memory inside GDB can differ.  Breakpoint locations of
     the same type at the same address can be sorted in arbitrary order.  */

  if (a->owner->number != b->owner->number)
    return ((a->owner->number > b->owner->number)
	    - (a->owner->number < b->owner->number));

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

/* Download tracepoint locations if they haven't been.  */

static void
download_tracepoint_locations (void)
{
  struct breakpoint *b;
  struct cleanup *old_chain;
  enum tribool can_download_tracepoint = TRIBOOL_UNKNOWN;

  old_chain = save_current_space_and_thread ();

  ALL_TRACEPOINTS (b)
    {
      struct bp_location *bl;
      struct tracepoint *t;
      int bp_location_downloaded = 0;

      if ((b->type == bp_fast_tracepoint
	   ? !may_insert_fast_tracepoints
	   : !may_insert_tracepoints))
	continue;

      if (can_download_tracepoint == TRIBOOL_UNKNOWN)
	{
	  if (target_can_download_tracepoint ())
	    can_download_tracepoint = TRIBOOL_TRUE;
	  else
	    can_download_tracepoint = TRIBOOL_FALSE;
	}

      if (can_download_tracepoint == TRIBOOL_FALSE)
	break;

      for (bl = b->loc; bl; bl = bl->next)
	{
	  /* In tracepoint, locations are _never_ duplicated, so
	     should_be_inserted is equivalent to
	     unduplicated_should_be_inserted.  */
	  if (!should_be_inserted (bl) || bl->inserted)
	    continue;

	  switch_to_program_space_and_thread (bl->pspace);

	  target_download_tracepoint (bl);

	  bl->inserted = 1;
	  bp_location_downloaded = 1;
	}
      t = (struct tracepoint *) b;
      t->number_on_target = b->number;
      if (bp_location_downloaded)
	observer_notify_breakpoint_modified (b);
    }

  do_cleanups (old_chain);
}

/* Swap the insertion/duplication state between two locations.  */

static void
swap_insertion (struct bp_location *left, struct bp_location *right)
{
  const int left_inserted = left->inserted;
  const int left_duplicate = left->duplicate;
  const int left_needs_update = left->needs_update;
  const struct bp_target_info left_target_info = left->target_info;

  /* Locations of tracepoints can never be duplicated.  */
  if (is_tracepoint (left->owner))
    gdb_assert (!left->duplicate);
  if (is_tracepoint (right->owner))
    gdb_assert (!right->duplicate);

  left->inserted = right->inserted;
  left->duplicate = right->duplicate;
  left->needs_update = right->needs_update;
  left->target_info = right->target_info;
  right->inserted = left_inserted;
  right->duplicate = left_duplicate;
  right->needs_update = left_needs_update;
  right->target_info = left_target_info;
}

/* Force the re-insertion of the locations at ADDRESS.  This is called
   once a new/deleted/modified duplicate location is found and we are evaluating
   conditions on the target's side.  Such conditions need to be updated on
   the target.  */

static void
force_breakpoint_reinsertion (struct bp_location *bl)
{
  struct bp_location **locp = NULL, **loc2p;
  struct bp_location *loc;
  CORE_ADDR address = 0;
  int pspace_num;

  address = bl->address;
  pspace_num = bl->pspace->num;

  /* This is only meaningful if the target is
     evaluating conditions and if the user has
     opted for condition evaluation on the target's
     side.  */
  if (gdb_evaluates_breakpoint_condition_p ()
      || !target_supports_evaluation_of_breakpoint_conditions ())
    return;

  /* Flag all breakpoint locations with this address and
     the same program space as the location
     as "its condition has changed".  We need to
     update the conditions on the target's side.  */
  ALL_BP_LOCATIONS_AT_ADDR (loc2p, locp, address)
    {
      loc = *loc2p;

      if (!is_breakpoint (loc->owner)
	  || pspace_num != loc->pspace->num)
	continue;

      /* Flag the location appropriately.  We use a different state to
	 let everyone know that we already updated the set of locations
	 with addr bl->address and program space bl->pspace.  This is so
	 we don't have to keep calling these functions just to mark locations
	 that have already been marked.  */
      loc->condition_changed = condition_updated;

      /* Free the agent expression bytecode as well.  We will compute
	 it later on.  */
      if (loc->cond_bytecode)
	{
	  free_agent_expr (loc->cond_bytecode);
	  loc->cond_bytecode = NULL;
	}
    }
}
/* Called whether new breakpoints are created, or existing breakpoints
   deleted, to update the global location list and recompute which
   locations are duplicate of which.

   The INSERT_MODE flag determines whether locations may not, may, or
   shall be inserted now.  See 'enum ugll_insert_mode' for more
   info.  */

static void
update_global_location_list (enum ugll_insert_mode insert_mode)
{
  struct breakpoint *b;
  struct bp_location **locp, *loc;
  struct cleanup *cleanups;
  /* Last breakpoint location address that was marked for update.  */
  CORE_ADDR last_addr = 0;
  /* Last breakpoint location program space that was marked for update.  */
  int last_pspace_num = -1;

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

  bp_location = XNEWVEC (struct bp_location *, bp_location_count);
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

      /* Tells if 'old_loc' is found among the new locations.  If
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
	  /* Check if this is a new/duplicated location or a duplicated
	     location that had its condition modified.  If so, we want to send
	     its condition to the target if evaluation of conditions is taking
	     place there.  */
	  if ((*loc2p)->condition_changed == condition_modified
	      && (last_addr != old_loc->address
		  || last_pspace_num != old_loc->pspace->num))
	    {
	      force_breakpoint_reinsertion (*loc2p);
	      last_pspace_num = old_loc->pspace->num;
	    }

	  if (*loc2p == old_loc)
	    found_object = 1;
	}

      /* We have already handled this address, update it so that we don't
	 have to go through updates again.  */
      last_addr = old_loc->address;

      /* Target-side condition evaluation: Handle deleted locations.  */
      if (!found_object)
	force_breakpoint_reinsertion (old_loc);

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
	      /* This location still exists, but it won't be kept in the
		 target since it may have been disabled.  We proceed to
		 remove its target-side condition.  */

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
			  /* Read watchpoint locations are switched to
			     access watchpoints, if the former are not
			     supported, but the latter are.  */
			  if (is_hardware_watchpoint (old_loc->owner))
			    {
			      gdb_assert (is_hardware_watchpoint (loc2->owner));
			      loc2->watchpoint_type = old_loc->watchpoint_type;
			    }

			  /* loc2 is a duplicated location. We need to check
			     if it should be inserted in case it will be
			     unduplicated.  */
			  if (loc2 != old_loc
			      && unduplicated_should_be_inserted (loc2))
			    {
			      swap_insertion (old_loc, loc2);
			      keep_in_target = 1;
			      break;
			    }
			}
		    }
		}
	    }

	  if (!keep_in_target)
	    {
	      if (remove_breakpoint (old_loc))
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
	  if (removed && target_is_non_stop_p ()
	      && need_moribund_for_location_type (old_loc))
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
      struct bp_location **loc_first_p;
      b = loc->owner;

      if (!unduplicated_should_be_inserted (loc)
	  || !breakpoint_address_is_meaningful (b)
	  /* Don't detect duplicate for tracepoint locations because they are
	   never duplicated.  See the comments in field `duplicate' of
	   `struct bp_location'.  */
	  || is_tracepoint (b))
	{
	  /* Clear the condition modification flag.  */
	  loc->condition_changed = condition_unchanged;
	  continue;
	}

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

	  if (is_breakpoint (loc->owner) && loc->condition_changed)
	    {
	      loc->needs_update = 1;
	      /* Clear the condition modification flag.  */
	      loc->condition_changed = condition_unchanged;
	    }
	  continue;
	}


      /* This and the above ensure the invariant that the first location
	 is not duplicated, and is the inserted one.
	 All following are marked as duplicated, and are not inserted.  */
      if (loc->inserted)
	swap_insertion (loc, *loc_first_p);
      loc->duplicate = 1;

      /* Clear the condition modification flag.  */
      loc->condition_changed = condition_unchanged;
    }

  if (insert_mode == UGLL_INSERT || breakpoints_should_be_inserted_now ())
    {
      if (insert_mode != UGLL_DONT_INSERT)
	insert_breakpoint_locations ();
      else
	{
	  /* Even though the caller told us to not insert new
	     locations, we may still need to update conditions on the
	     target's side of breakpoints that were already inserted
	     if the target is evaluating breakpoint conditions.  We
	     only update conditions for locations that are marked
	     "needs_update".  */
	  update_inserted_breakpoint_locations ();
	}
    }

  if (insert_mode != UGLL_DONT_INSERT)
    download_tracepoint_locations ();

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
update_global_location_list_nothrow (enum ugll_insert_mode insert_mode)
{

  TRY
    {
      update_global_location_list (insert_mode);
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
    }
  END_CATCH
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
  struct breakpoint *bpt = (struct breakpoint *) data;

  bpstat_remove_bp_location (th->control.stop_bpstat, bpt);
  return 0;
}

/* Helper for breakpoint and tracepoint breakpoint_ops->mention
   callbacks.  */

static void
say_where (struct breakpoint *b)
{
  struct value_print_options opts;

  get_user_print_options (&opts);

  /* i18n: cagney/2005-02-11: Below needs to be merged into a
     single string.  */
  if (b->loc == NULL)
    {
      /* For pending locations, the output differs slightly based
	 on b->extra_string.  If this is non-NULL, it contains either
	 a condition or dprintf arguments.  */
      if (b->extra_string == NULL)
	{
	  printf_filtered (_(" (%s) pending."),
			   event_location_to_string (b->location));
	}
      else if (b->type == bp_dprintf)
	{
	  printf_filtered (_(" (%s,%s) pending."),
			   event_location_to_string (b->location),
			   b->extra_string);
	}
      else
	{
	  printf_filtered (_(" (%s %s) pending."),
			   event_location_to_string (b->location),
			   b->extra_string);
	}
    }
  else
    {
      if (opts.addressprint || b->loc->symtab == NULL)
	{
	  printf_filtered (" at ");
	  fputs_filtered (paddress (b->loc->gdbarch, b->loc->address),
			  gdb_stdout);
	}
      if (b->loc->symtab != NULL)
	{
	  /* If there is a single location, we can print the location
	     more nicely.  */
	  if (b->loc->next == NULL)
	    printf_filtered (": file %s, line %d.",
			     symtab_to_filename_for_display (b->loc->symtab),
			     b->loc->line_number);
	  else
	    /* This is not ideal, but each location may have a
	       different file name, and this at least reflects the
	       real situation somewhat.  */
	    printf_filtered (": %s.",
			     event_location_to_string (b->location));
	}

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

/* Default bp_location_ops methods.  */

static void
bp_location_dtor (struct bp_location *self)
{
  xfree (self->cond);
  if (self->cond_bytecode)
    free_agent_expr (self->cond_bytecode);
  xfree (self->function_name);

  VEC_free (agent_expr_p, self->target_info.conditions);
  VEC_free (agent_expr_p, self->target_info.tcommands);
}

static const struct bp_location_ops bp_location_ops =
{
  bp_location_dtor
};

/* Default breakpoint_ops methods all breakpoint_ops ultimately
   inherit from.  */

static void
base_breakpoint_dtor (struct breakpoint *self)
{
  decref_counted_command_line (&self->commands);
  xfree (self->cond_string);
  xfree (self->extra_string);
  xfree (self->filter);
  delete_event_location (self->location);
  delete_event_location (self->location_range_end);
}

static struct bp_location *
base_breakpoint_allocate_location (struct breakpoint *self)
{
  struct bp_location *loc;

  loc = XNEW (struct bp_location);
  init_bp_location (loc, &bp_location_ops, self);
  return loc;
}

static void
base_breakpoint_re_set (struct breakpoint *b)
{
  /* Nothing to re-set. */
}

#define internal_error_pure_virtual_called() \
  gdb_assert_not_reached ("pure virtual function called")

static int
base_breakpoint_insert_location (struct bp_location *bl)
{
  internal_error_pure_virtual_called ();
}

static int
base_breakpoint_remove_location (struct bp_location *bl,
				 enum remove_bp_reason reason)
{
  internal_error_pure_virtual_called ();
}

static int
base_breakpoint_breakpoint_hit (const struct bp_location *bl,
				struct address_space *aspace,
				CORE_ADDR bp_addr,
				const struct target_waitstatus *ws)
{
  internal_error_pure_virtual_called ();
}

static void
base_breakpoint_check_status (bpstat bs)
{
  /* Always stop.   */
}

/* A "works_in_software_mode" breakpoint_ops method that just internal
   errors.  */

static int
base_breakpoint_works_in_software_mode (const struct breakpoint *b)
{
  internal_error_pure_virtual_called ();
}

/* A "resources_needed" breakpoint_ops method that just internal
   errors.  */

static int
base_breakpoint_resources_needed (const struct bp_location *bl)
{
  internal_error_pure_virtual_called ();
}

static enum print_stop_action
base_breakpoint_print_it (bpstat bs)
{
  internal_error_pure_virtual_called ();
}

static void
base_breakpoint_print_one_detail (const struct breakpoint *self,
				  struct ui_out *uiout)
{
  /* nothing */
}

static void
base_breakpoint_print_mention (struct breakpoint *b)
{
  internal_error_pure_virtual_called ();
}

static void
base_breakpoint_print_recreate (struct breakpoint *b, struct ui_file *fp)
{
  internal_error_pure_virtual_called ();
}

static void
base_breakpoint_create_sals_from_location
  (const struct event_location *location,
   struct linespec_result *canonical,
   enum bptype type_wanted)
{
  internal_error_pure_virtual_called ();
}

static void
base_breakpoint_create_breakpoints_sal (struct gdbarch *gdbarch,
					struct linespec_result *c,
					char *cond_string,
					char *extra_string,
					enum bptype type_wanted,
					enum bpdisp disposition,
					int thread,
					int task, int ignore_count,
					const struct breakpoint_ops *o,
					int from_tty, int enabled,
					int internal, unsigned flags)
{
  internal_error_pure_virtual_called ();
}

static void
base_breakpoint_decode_location (struct breakpoint *b,
				 const struct event_location *location,
				 struct program_space *search_pspace,
				 struct symtabs_and_lines *sals)
{
  internal_error_pure_virtual_called ();
}

/* The default 'explains_signal' method.  */

static int
base_breakpoint_explains_signal (struct breakpoint *b, enum gdb_signal sig)
{
  return 1;
}

/* The default "after_condition_true" method.  */

static void
base_breakpoint_after_condition_true (struct bpstats *bs)
{
  /* Nothing to do.   */
}

struct breakpoint_ops base_breakpoint_ops =
{
  base_breakpoint_dtor,
  base_breakpoint_allocate_location,
  base_breakpoint_re_set,
  base_breakpoint_insert_location,
  base_breakpoint_remove_location,
  base_breakpoint_breakpoint_hit,
  base_breakpoint_check_status,
  base_breakpoint_resources_needed,
  base_breakpoint_works_in_software_mode,
  base_breakpoint_print_it,
  NULL,
  base_breakpoint_print_one_detail,
  base_breakpoint_print_mention,
  base_breakpoint_print_recreate,
  base_breakpoint_create_sals_from_location,
  base_breakpoint_create_breakpoints_sal,
  base_breakpoint_decode_location,
  base_breakpoint_explains_signal,
  base_breakpoint_after_condition_true,
};

/* Default breakpoint_ops methods.  */

static void
bkpt_re_set (struct breakpoint *b)
{
  /* FIXME: is this still reachable?  */
  if (breakpoint_event_location_empty_p (b))
    {
      /* Anything without a location can't be re-set.  */
      delete_breakpoint (b);
      return;
    }

  breakpoint_re_set_default (b);
}

static int
bkpt_insert_location (struct bp_location *bl)
{
  if (bl->loc_type == bp_loc_hardware_breakpoint)
    return target_insert_hw_breakpoint (bl->gdbarch, &bl->target_info);
  else
    return target_insert_breakpoint (bl->gdbarch, &bl->target_info);
}

static int
bkpt_remove_location (struct bp_location *bl, enum remove_bp_reason reason)
{
  if (bl->loc_type == bp_loc_hardware_breakpoint)
    return target_remove_hw_breakpoint (bl->gdbarch, &bl->target_info);
  else
    return target_remove_breakpoint (bl->gdbarch, &bl->target_info, reason);
}

static int
bkpt_breakpoint_hit (const struct bp_location *bl,
		     struct address_space *aspace, CORE_ADDR bp_addr,
		     const struct target_waitstatus *ws)
{
  if (ws->kind != TARGET_WAITKIND_STOPPED
      || ws->value.sig != GDB_SIGNAL_TRAP)
    return 0;

  if (!breakpoint_address_match (bl->pspace->aspace, bl->address,
				 aspace, bp_addr))
    return 0;

  if (overlay_debugging		/* unmapped overlay section */
      && section_is_overlay (bl->section)
      && !section_is_mapped (bl->section))
    return 0;

  return 1;
}

static int
dprintf_breakpoint_hit (const struct bp_location *bl,
			struct address_space *aspace, CORE_ADDR bp_addr,
			const struct target_waitstatus *ws)
{
  if (dprintf_style == dprintf_style_agent
      && target_can_run_breakpoint_commands ())
    {
      /* An agent-style dprintf never causes a stop.  If we see a trap
	 for this address it must be for a breakpoint that happens to
	 be set at the same address.  */
      return 0;
    }

  return bkpt_breakpoint_hit (bl, aspace, bp_addr, ws);
}

static int
bkpt_resources_needed (const struct bp_location *bl)
{
  gdb_assert (bl->owner->type == bp_hardware_breakpoint);

  return 1;
}

static enum print_stop_action
bkpt_print_it (bpstat bs)
{
  struct breakpoint *b;
  const struct bp_location *bl;
  int bp_temp;
  struct ui_out *uiout = current_uiout;

  gdb_assert (bs->bp_location_at != NULL);

  bl = bs->bp_location_at;
  b = bs->breakpoint_at;

  bp_temp = b->disposition == disp_del;
  if (bl->address != bl->requested_address)
    breakpoint_adjustment_warning (bl->requested_address,
				   bl->address,
				   b->number, 1);
  annotate_breakpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);

  if (bp_temp)
    ui_out_text (uiout, "Temporary breakpoint ");
  else
    ui_out_text (uiout, "Breakpoint ");
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

static void
bkpt_print_mention (struct breakpoint *b)
{
  if (ui_out_is_mi_like_p (current_uiout))
    return;

  switch (b->type)
    {
    case bp_breakpoint:
    case bp_gnu_ifunc_resolver:
      if (b->disposition == disp_del)
	printf_filtered (_("Temporary breakpoint"));
      else
	printf_filtered (_("Breakpoint"));
      printf_filtered (_(" %d"), b->number);
      if (b->type == bp_gnu_ifunc_resolver)
	printf_filtered (_(" at gnu-indirect-function resolver"));
      break;
    case bp_hardware_breakpoint:
      printf_filtered (_("Hardware assisted breakpoint %d"), b->number);
      break;
    case bp_dprintf:
      printf_filtered (_("Dprintf %d"), b->number);
      break;
    }

  say_where (b);
}

static void
bkpt_print_recreate (struct breakpoint *tp, struct ui_file *fp)
{
  if (tp->type == bp_breakpoint && tp->disposition == disp_del)
    fprintf_unfiltered (fp, "tbreak");
  else if (tp->type == bp_breakpoint)
    fprintf_unfiltered (fp, "break");
  else if (tp->type == bp_hardware_breakpoint
	   && tp->disposition == disp_del)
    fprintf_unfiltered (fp, "thbreak");
  else if (tp->type == bp_hardware_breakpoint)
    fprintf_unfiltered (fp, "hbreak");
  else
    internal_error (__FILE__, __LINE__,
		    _("unhandled breakpoint type %d"), (int) tp->type);

  fprintf_unfiltered (fp, " %s",
		      event_location_to_string (tp->location));

  /* Print out extra_string if this breakpoint is pending.  It might
     contain, for example, conditions that were set by the user.  */
  if (tp->loc == NULL && tp->extra_string != NULL)
    fprintf_unfiltered (fp, " %s", tp->extra_string);

  print_recreate_thread (tp, fp);
}

static void
bkpt_create_sals_from_location (const struct event_location *location,
				struct linespec_result *canonical,
				enum bptype type_wanted)
{
  create_sals_from_location_default (location, canonical, type_wanted);
}

static void
bkpt_create_breakpoints_sal (struct gdbarch *gdbarch,
			     struct linespec_result *canonical,
			     char *cond_string,
			     char *extra_string,
			     enum bptype type_wanted,
			     enum bpdisp disposition,
			     int thread,
			     int task, int ignore_count,
			     const struct breakpoint_ops *ops,
			     int from_tty, int enabled,
			     int internal, unsigned flags)
{
  create_breakpoints_sal_default (gdbarch, canonical,
				  cond_string, extra_string,
				  type_wanted,
				  disposition, thread, task,
				  ignore_count, ops, from_tty,
				  enabled, internal, flags);
}

static void
bkpt_decode_location (struct breakpoint *b,
		      const struct event_location *location,
		      struct program_space *search_pspace,
		      struct symtabs_and_lines *sals)
{
  decode_location_default (b, location, search_pspace, sals);
}

/* Virtual table for internal breakpoints.  */

static void
internal_bkpt_re_set (struct breakpoint *b)
{
  switch (b->type)
    {
      /* Delete overlay event and longjmp master breakpoints; they
	 will be reset later by breakpoint_re_set.  */
    case bp_overlay_event:
    case bp_longjmp_master:
    case bp_std_terminate_master:
    case bp_exception_master:
      delete_breakpoint (b);
      break;

      /* This breakpoint is special, it's set up when the inferior
         starts and we really don't want to touch it.  */
    case bp_shlib_event:

      /* Like bp_shlib_event, this breakpoint type is special.  Once
	 it is set up, we do not want to touch it.  */
    case bp_thread_event:
      break;
    }
}

static void
internal_bkpt_check_status (bpstat bs)
{
  if (bs->breakpoint_at->type == bp_shlib_event)
    {
      /* If requested, stop when the dynamic linker notifies GDB of
	 events.  This allows the user to get control and place
	 breakpoints in initializer routines for dynamically loaded
	 objects (among other things).  */
      bs->stop = stop_on_solib_events;
      bs->print = stop_on_solib_events;
    }
  else
    bs->stop = 0;
}

static enum print_stop_action
internal_bkpt_print_it (bpstat bs)
{
  struct breakpoint *b;

  b = bs->breakpoint_at;

  switch (b->type)
    {
    case bp_shlib_event:
      /* Did we stop because the user set the stop_on_solib_events
	 variable?  (If so, we report this as a generic, "Stopped due
	 to shlib event" message.) */
      print_solib_event (0);
      break;

    case bp_thread_event:
      /* Not sure how we will get here.
	 GDB should not stop for these breakpoints.  */
      printf_filtered (_("Thread Event Breakpoint: gdb should not stop!\n"));
      break;

    case bp_overlay_event:
      /* By analogy with the thread event, GDB should not stop for these.  */
      printf_filtered (_("Overlay Event Breakpoint: gdb should not stop!\n"));
      break;

    case bp_longjmp_master:
      /* These should never be enabled.  */
      printf_filtered (_("Longjmp Master Breakpoint: gdb should not stop!\n"));
      break;

    case bp_std_terminate_master:
      /* These should never be enabled.  */
      printf_filtered (_("std::terminate Master Breakpoint: "
			 "gdb should not stop!\n"));
      break;

    case bp_exception_master:
      /* These should never be enabled.  */
      printf_filtered (_("Exception Master Breakpoint: "
			 "gdb should not stop!\n"));
      break;
    }

  return PRINT_NOTHING;
}

static void
internal_bkpt_print_mention (struct breakpoint *b)
{
  /* Nothing to mention.  These breakpoints are internal.  */
}

/* Virtual table for momentary breakpoints  */

static void
momentary_bkpt_re_set (struct breakpoint *b)
{
  /* Keep temporary breakpoints, which can be encountered when we step
     over a dlopen call and solib_add is resetting the breakpoints.
     Otherwise these should have been blown away via the cleanup chain
     or by breakpoint_init_inferior when we rerun the executable.  */
}

static void
momentary_bkpt_check_status (bpstat bs)
{
  /* Nothing.  The point of these breakpoints is causing a stop.  */
}

static enum print_stop_action
momentary_bkpt_print_it (bpstat bs)
{
  return PRINT_UNKNOWN;
}

static void
momentary_bkpt_print_mention (struct breakpoint *b)
{
  /* Nothing to mention.  These breakpoints are internal.  */
}

/* Ensure INITIATING_FRAME is cleared when no such breakpoint exists.

   It gets cleared already on the removal of the first one of such placed
   breakpoints.  This is OK as they get all removed altogether.  */

static void
longjmp_bkpt_dtor (struct breakpoint *self)
{
  struct thread_info *tp = find_thread_global_id (self->thread);

  if (tp)
    tp->initiating_frame = null_frame_id;

  momentary_breakpoint_ops.dtor (self);
}

/* Specific methods for probe breakpoints.  */

static int
bkpt_probe_insert_location (struct bp_location *bl)
{
  int v = bkpt_insert_location (bl);

  if (v == 0)
    {
      /* The insertion was successful, now let's set the probe's semaphore
	 if needed.  */
      if (bl->probe.probe->pops->set_semaphore != NULL)
	bl->probe.probe->pops->set_semaphore (bl->probe.probe,
					      bl->probe.objfile,
					      bl->gdbarch);
    }

  return v;
}

static int
bkpt_probe_remove_location (struct bp_location *bl,
			    enum remove_bp_reason reason)
{
  /* Let's clear the semaphore before removing the location.  */
  if (bl->probe.probe->pops->clear_semaphore != NULL)
    bl->probe.probe->pops->clear_semaphore (bl->probe.probe,
					    bl->probe.objfile,
					    bl->gdbarch);

  return bkpt_remove_location (bl, reason);
}

static void
bkpt_probe_create_sals_from_location (const struct event_location *location,
				      struct linespec_result *canonical,
				      enum bptype type_wanted)
{
  struct linespec_sals lsal;

  lsal.sals = parse_probes (location, NULL, canonical);
  lsal.canonical = xstrdup (event_location_to_string (canonical->location));
  VEC_safe_push (linespec_sals, canonical->sals, &lsal);
}

static void
bkpt_probe_decode_location (struct breakpoint *b,
			    const struct event_location *location,
			    struct program_space *search_pspace,
			    struct symtabs_and_lines *sals)
{
  *sals = parse_probes (location, search_pspace, NULL);
  if (!sals->sals)
    error (_("probe not found"));
}

/* The breakpoint_ops structure to be used in tracepoints.  */

static void
tracepoint_re_set (struct breakpoint *b)
{
  breakpoint_re_set_default (b);
}

static int
tracepoint_breakpoint_hit (const struct bp_location *bl,
			   struct address_space *aspace, CORE_ADDR bp_addr,
			   const struct target_waitstatus *ws)
{
  /* By definition, the inferior does not report stops at
     tracepoints.  */
  return 0;
}

static void
tracepoint_print_one_detail (const struct breakpoint *self,
			     struct ui_out *uiout)
{
  struct tracepoint *tp = (struct tracepoint *) self;
  if (tp->static_trace_marker_id)
    {
      gdb_assert (self->type == bp_static_tracepoint);

      ui_out_text (uiout, "\tmarker id is ");
      ui_out_field_string (uiout, "static-tracepoint-marker-string-id",
			   tp->static_trace_marker_id);
      ui_out_text (uiout, "\n");
    }
}

static void
tracepoint_print_mention (struct breakpoint *b)
{
  if (ui_out_is_mi_like_p (current_uiout))
    return;

  switch (b->type)
    {
    case bp_tracepoint:
      printf_filtered (_("Tracepoint"));
      printf_filtered (_(" %d"), b->number);
      break;
    case bp_fast_tracepoint:
      printf_filtered (_("Fast tracepoint"));
      printf_filtered (_(" %d"), b->number);
      break;
    case bp_static_tracepoint:
      printf_filtered (_("Static tracepoint"));
      printf_filtered (_(" %d"), b->number);
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("unhandled tracepoint type %d"), (int) b->type);
    }

  say_where (b);
}

static void
tracepoint_print_recreate (struct breakpoint *self, struct ui_file *fp)
{
  struct tracepoint *tp = (struct tracepoint *) self;

  if (self->type == bp_fast_tracepoint)
    fprintf_unfiltered (fp, "ftrace");
  else if (self->type == bp_static_tracepoint)
    fprintf_unfiltered (fp, "strace");
  else if (self->type == bp_tracepoint)
    fprintf_unfiltered (fp, "trace");
  else
    internal_error (__FILE__, __LINE__,
		    _("unhandled tracepoint type %d"), (int) self->type);

  fprintf_unfiltered (fp, " %s",
		      event_location_to_string (self->location));
  print_recreate_thread (self, fp);

  if (tp->pass_count)
    fprintf_unfiltered (fp, "  passcount %d\n", tp->pass_count);
}

static void
tracepoint_create_sals_from_location (const struct event_location *location,
				      struct linespec_result *canonical,
				      enum bptype type_wanted)
{
  create_sals_from_location_default (location, canonical, type_wanted);
}

static void
tracepoint_create_breakpoints_sal (struct gdbarch *gdbarch,
				   struct linespec_result *canonical,
				   char *cond_string,
				   char *extra_string,
				   enum bptype type_wanted,
				   enum bpdisp disposition,
				   int thread,
				   int task, int ignore_count,
				   const struct breakpoint_ops *ops,
				   int from_tty, int enabled,
				   int internal, unsigned flags)
{
  create_breakpoints_sal_default (gdbarch, canonical,
				  cond_string, extra_string,
				  type_wanted,
				  disposition, thread, task,
				  ignore_count, ops, from_tty,
				  enabled, internal, flags);
}

static void
tracepoint_decode_location (struct breakpoint *b,
			    const struct event_location *location,
			    struct program_space *search_pspace,
			    struct symtabs_and_lines *sals)
{
  decode_location_default (b, location, search_pspace, sals);
}

struct breakpoint_ops tracepoint_breakpoint_ops;

/* The breakpoint_ops structure to be use on tracepoints placed in a
   static probe.  */

static void
tracepoint_probe_create_sals_from_location
  (const struct event_location *location,
   struct linespec_result *canonical,
   enum bptype type_wanted)
{
  /* We use the same method for breakpoint on probes.  */
  bkpt_probe_create_sals_from_location (location, canonical, type_wanted);
}

static void
tracepoint_probe_decode_location (struct breakpoint *b,
				  const struct event_location *location,
				  struct program_space *search_pspace,
				  struct symtabs_and_lines *sals)
{
  /* We use the same method for breakpoint on probes.  */
  bkpt_probe_decode_location (b, location, search_pspace, sals);
}

static struct breakpoint_ops tracepoint_probe_breakpoint_ops;

/* Dprintf breakpoint_ops methods.  */

static void
dprintf_re_set (struct breakpoint *b)
{
  breakpoint_re_set_default (b);

  /* extra_string should never be non-NULL for dprintf.  */
  gdb_assert (b->extra_string != NULL);

  /* 1 - connect to target 1, that can run breakpoint commands.
     2 - create a dprintf, which resolves fine.
     3 - disconnect from target 1
     4 - connect to target 2, that can NOT run breakpoint commands.

     After steps #3/#4, you'll want the dprintf command list to
     be updated, because target 1 and 2 may well return different
     answers for target_can_run_breakpoint_commands().
     Given absence of finer grained resetting, we get to do
     it all the time.  */
  if (b->extra_string != NULL)
    update_dprintf_command_list (b);
}

/* Implement the "print_recreate" breakpoint_ops method for dprintf.  */

static void
dprintf_print_recreate (struct breakpoint *tp, struct ui_file *fp)
{
  fprintf_unfiltered (fp, "dprintf %s,%s",
		      event_location_to_string (tp->location),
		      tp->extra_string);
  print_recreate_thread (tp, fp);
}

/* Implement the "after_condition_true" breakpoint_ops method for
   dprintf.

   dprintf's are implemented with regular commands in their command
   list, but we run the commands here instead of before presenting the
   stop to the user, as dprintf's don't actually cause a stop.  This
   also makes it so that the commands of multiple dprintfs at the same
   address are all handled.  */

static void
dprintf_after_condition_true (struct bpstats *bs)
{
  struct cleanup *old_chain;
  struct bpstats tmp_bs = { NULL };
  struct bpstats *tmp_bs_p = &tmp_bs;

  /* dprintf's never cause a stop.  This wasn't set in the
     check_status hook instead because that would make the dprintf's
     condition not be evaluated.  */
  bs->stop = 0;

  /* Run the command list here.  Take ownership of it instead of
     copying.  We never want these commands to run later in
     bpstat_do_actions, if a breakpoint that causes a stop happens to
     be set at same address as this dprintf, or even if running the
     commands here throws.  */
  tmp_bs.commands = bs->commands;
  bs->commands = NULL;
  old_chain = make_cleanup_decref_counted_command_line (&tmp_bs.commands);

  bpstat_do_actions_1 (&tmp_bs_p);

  /* 'tmp_bs.commands' will usually be NULL by now, but
     bpstat_do_actions_1 may return early without processing the whole
     list.  */
  do_cleanups (old_chain);
}

/* The breakpoint_ops structure to be used on static tracepoints with
   markers (`-m').  */

static void
strace_marker_create_sals_from_location (const struct event_location *location,
					 struct linespec_result *canonical,
					 enum bptype type_wanted)
{
  struct linespec_sals lsal;
  const char *arg_start, *arg;
  char *str;
  struct cleanup *cleanup;

  arg = arg_start = get_linespec_location (location);
  lsal.sals = decode_static_tracepoint_spec (&arg);

  str = savestring (arg_start, arg - arg_start);
  cleanup = make_cleanup (xfree, str);
  canonical->location = new_linespec_location (&str);
  do_cleanups (cleanup);

  lsal.canonical = xstrdup (event_location_to_string (canonical->location));
  VEC_safe_push (linespec_sals, canonical->sals, &lsal);
}

static void
strace_marker_create_breakpoints_sal (struct gdbarch *gdbarch,
				      struct linespec_result *canonical,
				      char *cond_string,
				      char *extra_string,
				      enum bptype type_wanted,
				      enum bpdisp disposition,
				      int thread,
				      int task, int ignore_count,
				      const struct breakpoint_ops *ops,
				      int from_tty, int enabled,
				      int internal, unsigned flags)
{
  int i;
  struct linespec_sals *lsal = VEC_index (linespec_sals,
					  canonical->sals, 0);

  /* If the user is creating a static tracepoint by marker id
     (strace -m MARKER_ID), then store the sals index, so that
     breakpoint_re_set can try to match up which of the newly
     found markers corresponds to this one, and, don't try to
     expand multiple locations for each sal, given than SALS
     already should contain all sals for MARKER_ID.  */

  for (i = 0; i < lsal->sals.nelts; ++i)
    {
      struct symtabs_and_lines expanded;
      struct tracepoint *tp;
      struct cleanup *old_chain;
      struct event_location *location;

      expanded.nelts = 1;
      expanded.sals = &lsal->sals.sals[i];

      location = copy_event_location (canonical->location);
      old_chain = make_cleanup_delete_event_location (location);

      tp = XCNEW (struct tracepoint);
      init_breakpoint_sal (&tp->base, gdbarch, expanded,
			   location, NULL,
			   cond_string, extra_string,
			   type_wanted, disposition,
			   thread, task, ignore_count, ops,
			   from_tty, enabled, internal, flags,
			   canonical->special_display);
      /* Given that its possible to have multiple markers with
	 the same string id, if the user is creating a static
	 tracepoint by marker id ("strace -m MARKER_ID"), then
	 store the sals index, so that breakpoint_re_set can
	 try to match up which of the newly found markers
	 corresponds to this one  */
      tp->static_trace_marker_id_idx = i;

      install_breakpoint (internal, &tp->base, 0);

      discard_cleanups (old_chain);
    }
}

static void
strace_marker_decode_location (struct breakpoint *b,
			       const struct event_location *location,
			       struct program_space *search_pspace,
			       struct symtabs_and_lines *sals)
{
  struct tracepoint *tp = (struct tracepoint *) b;
  const char *s = get_linespec_location (location);

  *sals = decode_static_tracepoint_spec (&s);
  if (sals->nelts > tp->static_trace_marker_id_idx)
    {
      sals->sals[0] = sals->sals[tp->static_trace_marker_id_idx];
      sals->nelts = 1;
    }
  else
    error (_("marker %s not found"), tp->static_trace_marker_id);
}

static struct breakpoint_ops strace_marker_breakpoint_ops;

static int
strace_marker_p (struct breakpoint *b)
{
  return b->ops == &strace_marker_breakpoint_ops;
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
      struct watchpoint *w;

      if (bpt->type == bp_watchpoint_scope)
	w = (struct watchpoint *) bpt->related_breakpoint;
      else if (bpt->related_breakpoint->type == bp_watchpoint_scope)
	w = (struct watchpoint *) bpt;
      else
	w = NULL;
      if (w != NULL)
	watchpoint_del_at_next_stop (w);

      /* Unlink bpt from the bpt->related_breakpoint ring.  */
      for (related = bpt; related->related_breakpoint != bpt;
	   related = related->related_breakpoint);
      related->related_breakpoint = bpt->related_breakpoint;
      bpt->related_breakpoint = bpt;
    }

  /* watch_command_1 creates a watchpoint but only sets its number if
     update_watchpoint succeeds in creating its bp_locations.  If there's
     a problem in that process, we'll be asked to delete the half-created
     watchpoint.  In that case, don't announce the deletion.  */
  if (bpt->number)
    observer_notify_breakpoint_deleted (bpt);

  if (breakpoint_chain == bpt)
    breakpoint_chain = bpt->next;

  ALL_BREAKPOINTS (b)
    if (b->next == bpt)
    {
      b->next = bpt->next;
      break;
    }

  /* Be sure no bpstat's are pointing at the breakpoint after it's
     been freed.  */
  /* FIXME, how can we find all bpstat's?  We just check stop_bpstat
     in all threads for now.  Note that we cannot just remove bpstats
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
  update_global_location_list (UGLL_DONT_INSERT);

  bpt->ops->dtor (bpt);
  /* On the chance that someone will soon try again to delete this
     same bp, we mark it as deleted before freeing its storage.  */
  bpt->type = bp_none;
  xfree (bpt);
}

static void
do_delete_breakpoint_cleanup (void *b)
{
  delete_breakpoint ((struct breakpoint *) b);
}

struct cleanup *
make_cleanup_delete_breakpoint (struct breakpoint *b)
{
  return make_cleanup (do_delete_breakpoint_cleanup, b);
}

/* Iterator function to call a user-provided callback function once
   for each of B and its related breakpoints.  */

static void
iterate_over_related_breakpoints (struct breakpoint *b,
				  void (*function) (struct breakpoint *,
						    void *),
				  void *data)
{
  struct breakpoint *related;

  related = b;
  do
    {
      struct breakpoint *next;

      /* FUNCTION may delete RELATED.  */
      next = related->related_breakpoint;

      if (next == related)
	{
	  /* RELATED is the last ring entry.  */
	  function (related, data);

	  /* FUNCTION may have deleted it, so we'd never reach back to
	     B.  There's nothing left to do anyway, so just break
	     out.  */
	  break;
	}
      else
	function (related, data);

      related = next;
    }
  while (related != b);
}

static void
do_delete_breakpoint (struct breakpoint *b, void *ignore)
{
  delete_breakpoint (b);
}

/* A callback for map_breakpoint_numbers that calls
   delete_breakpoint.  */

static void
do_map_delete_breakpoint (struct breakpoint *b, void *ignore)
{
  iterate_over_related_breakpoints (b, do_delete_breakpoint, NULL);
}

void
delete_command (char *arg, int from_tty)
{
  struct breakpoint *b, *b_tmp;

  dont_repeat ();

  if (arg == 0)
    {
      int breaks_to_delete = 0;

      /* Delete all breakpoints if no argument.  Do not delete
         internal breakpoints, these have to be deleted with an
         explicit breakpoint number argument.  */
      ALL_BREAKPOINTS (b)
	if (user_breakpoint_p (b))
	  {
	    breaks_to_delete = 1;
	    break;
	  }

      /* Ask user only if there are some breakpoints to delete.  */
      if (!from_tty
	  || (breaks_to_delete && query (_("Delete all breakpoints? "))))
	{
	  ALL_BREAKPOINTS_SAFE (b, b_tmp)
	    if (user_breakpoint_p (b))
	      delete_breakpoint (b);
	}
    }
  else
    map_breakpoint_numbers (arg, do_map_delete_breakpoint, NULL);
}

/* Return true if all locations of B bound to PSPACE are pending.  If
   PSPACE is NULL, all locations of all program spaces are
   considered.  */

static int
all_locations_are_pending (struct breakpoint *b, struct program_space *pspace)
{
  struct bp_location *loc;

  for (loc = b->loc; loc != NULL; loc = loc->next)
    if ((pspace == NULL
	 || loc->pspace == pspace)
	&& !loc->shlib_disabled
	&& !loc->pspace->executing_startup)
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
  struct tracepoint *tp = (struct tracepoint *) b;
  struct static_tracepoint_marker marker;
  CORE_ADDR pc;

  pc = sal.pc;
  if (sal.line)
    find_line_pc (sal.symtab, sal.line, &pc);

  if (target_static_tracepoint_marker_at (pc, &marker))
    {
      if (strcmp (tp->static_trace_marker_id, marker.str_id) != 0)
	warning (_("static tracepoint %d changed probed marker from %s to %s"),
		 b->number,
		 tp->static_trace_marker_id, marker.str_id);

      xfree (tp->static_trace_marker_id);
      tp->static_trace_marker_id = xstrdup (marker.str_id);
      release_static_tracepoint_marker (&marker);

      return sal;
    }

  /* Old marker wasn't found on target at lineno.  Try looking it up
     by string ID.  */
  if (!sal.explicit_pc
      && sal.line != 0
      && sal.symtab != NULL
      && tp->static_trace_marker_id != NULL)
    {
      VEC(static_tracepoint_marker_p) *markers;

      markers
	= target_static_tracepoint_markers_by_strid (tp->static_trace_marker_id);

      if (!VEC_empty(static_tracepoint_marker_p, markers))
	{
	  struct symtab_and_line sal2;
	  struct symbol *sym;
	  struct static_tracepoint_marker *tpmarker;
	  struct ui_out *uiout = current_uiout;
	  struct explicit_location explicit_loc;

	  tpmarker = VEC_index (static_tracepoint_marker_p, markers, 0);

	  xfree (tp->static_trace_marker_id);
	  tp->static_trace_marker_id = xstrdup (tpmarker->str_id);

	  warning (_("marker for static tracepoint %d (%s) not "
		     "found at previous line number"),
		   b->number, tp->static_trace_marker_id);

	  init_sal (&sal2);

	  sal2.pc = tpmarker->address;

	  sal2 = find_pc_line (tpmarker->address, 0);
	  sym = find_pc_sect_function (tpmarker->address, NULL);
	  ui_out_text (uiout, "Now in ");
	  if (sym)
	    {
	      ui_out_field_string (uiout, "func",
				   SYMBOL_PRINT_NAME (sym));
	      ui_out_text (uiout, " at ");
	    }
	  ui_out_field_string (uiout, "file",
			       symtab_to_filename_for_display (sal2.symtab));
	  ui_out_text (uiout, ":");

	  if (ui_out_is_mi_like_p (uiout))
	    {
	      const char *fullname = symtab_to_fullname (sal2.symtab);

	      ui_out_field_string (uiout, "fullname", fullname);
	    }

	  ui_out_field_int (uiout, "line", sal2.line);
	  ui_out_text (uiout, "\n");

	  b->loc->line_number = sal2.line;
	  b->loc->symtab = sym != NULL ? sal2.symtab : NULL;

	  delete_event_location (b->location);
	  initialize_explicit_location (&explicit_loc);
	  explicit_loc.source_filename
	    = ASTRDUP (symtab_to_filename_for_display (sal2.symtab));
	  explicit_loc.line_offset.offset = b->loc->line_number;
	  explicit_loc.line_offset.sign = LINE_OFFSET_NONE;
	  b->location = new_explicit_location (&explicit_loc);

	  /* Might be nice to check if function changed, and warn if
	     so.  */

	  release_static_tracepoint_marker (tpmarker);
	}
    }
  return sal;
}

/* Returns 1 iff locations A and B are sufficiently same that
   we don't need to report breakpoint as changed.  */

static int
locations_are_equal (struct bp_location *a, struct bp_location *b)
{
  while (a && b)
    {
      if (a->address != b->address)
	return 0;

      if (a->shlib_disabled != b->shlib_disabled)
	return 0;

      if (a->enabled != b->enabled)
	return 0;

      a = a->next;
      b = b->next;
    }

  if ((a == NULL) != (b == NULL))
    return 0;

  return 1;
}

/* Split all locations of B that are bound to PSPACE out of B's
   location list to a separate list and return that list's head.  If
   PSPACE is NULL, hoist out all locations of B.  */

static struct bp_location *
hoist_existing_locations (struct breakpoint *b, struct program_space *pspace)
{
  struct bp_location head;
  struct bp_location *i = b->loc;
  struct bp_location **i_link = &b->loc;
  struct bp_location *hoisted = &head;

  if (pspace == NULL)
    {
      i = b->loc;
      b->loc = NULL;
      return i;
    }

  head.next = NULL;

  while (i != NULL)
    {
      if (i->pspace == pspace)
	{
	  *i_link = i->next;
	  i->next = NULL;
	  hoisted->next = i;
	  hoisted = i;
	}
      else
	i_link = &i->next;
      i = *i_link;
    }

  return head.next;
}

/* Create new breakpoint locations for B (a hardware or software
   breakpoint) based on SALS and SALS_END.  If SALS_END.NELTS is not
   zero, then B is a ranged breakpoint.  Only recreates locations for
   FILTER_PSPACE.  Locations of other program spaces are left
   untouched.  */

void
update_breakpoint_locations (struct breakpoint *b,
			     struct program_space *filter_pspace,
			     struct symtabs_and_lines sals,
			     struct symtabs_and_lines sals_end)
{
  int i;
  struct bp_location *existing_locations;

  if (sals_end.nelts != 0 && (sals.nelts != 1 || sals_end.nelts != 1))
    {
      /* Ranged breakpoints have only one start location and one end
	 location.  */
      b->enable_state = bp_disabled;
      printf_unfiltered (_("Could not reset ranged breakpoint %d: "
			   "multiple locations found\n"),
			 b->number);
      return;
    }

  /* If there's no new locations, and all existing locations are
     pending, don't do anything.  This optimizes the common case where
     all locations are in the same shared library, that was unloaded.
     We'd like to retain the location, so that when the library is
     loaded again, we don't loose the enabled/disabled status of the
     individual locations.  */
  if (all_locations_are_pending (b, filter_pspace) && sals.nelts == 0)
    return;

  existing_locations = hoist_existing_locations (b, filter_pspace);

  for (i = 0; i < sals.nelts; ++i)
    {
      struct bp_location *new_loc;

      switch_to_program_space_and_thread (sals.sals[i].pspace);

      new_loc = add_location_to_breakpoint (b, &(sals.sals[i]));

      /* Reparse conditions, they might contain references to the
	 old symtab.  */
      if (b->cond_string != NULL)
	{
	  const char *s;

	  s = b->cond_string;
	  TRY
	    {
	      new_loc->cond = parse_exp_1 (&s, sals.sals[i].pc,
					   block_for_pc (sals.sals[i].pc), 
					   0);
	    }
	  CATCH (e, RETURN_MASK_ERROR)
	    {
	      warning (_("failed to reevaluate condition "
			 "for breakpoint %d: %s"), 
		       b->number, e.message);
	      new_loc->enabled = 0;
	    }
	  END_CATCH
	}

      if (sals_end.nelts)
	{
	  CORE_ADDR end = find_breakpoint_range_end (sals_end.sals[0]);

	  new_loc->length = end - sals.sals[0].pc + 1;
	}
    }

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

  if (!locations_are_equal (existing_locations, b->loc))
    observer_notify_breakpoint_modified (b);
}

/* Find the SaL locations corresponding to the given LOCATION.
   On return, FOUND will be 1 if any SaL was found, zero otherwise.  */

static struct symtabs_and_lines
location_to_sals (struct breakpoint *b, struct event_location *location,
		  struct program_space *search_pspace, int *found)
{
  struct symtabs_and_lines sals = {0};
  struct gdb_exception exception = exception_none;

  gdb_assert (b->ops != NULL);

  TRY
    {
      b->ops->decode_location (b, location, search_pspace, &sals);
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
      int not_found_and_ok = 0;

      exception = e;

      /* For pending breakpoints, it's expected that parsing will
	 fail until the right shared library is loaded.  User has
	 already told to create pending breakpoints and don't need
	 extra messages.  If breakpoint is in bp_shlib_disabled
	 state, then user already saw the message about that
	 breakpoint being disabled, and don't want to see more
	 errors.  */
      if (e.error == NOT_FOUND_ERROR
	  && (b->condition_not_parsed
	      || (b->loc != NULL
		  && search_pspace != NULL
		  && b->loc->pspace != search_pspace)
	      || (b->loc && b->loc->shlib_disabled)
	      || (b->loc && b->loc->pspace->executing_startup)
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
  END_CATCH

  if (exception.reason == 0 || exception.error != NOT_FOUND_ERROR)
    {
      int i;

      for (i = 0; i < sals.nelts; ++i)
	resolve_sal_pc (&sals.sals[i]);
      if (b->condition_not_parsed && b->extra_string != NULL)
	{
	  char *cond_string, *extra_string;
	  int thread, task;

	  find_condition_and_thread (b->extra_string, sals.sals[0].pc,
				     &cond_string, &thread, &task,
				     &extra_string);
	  gdb_assert (b->cond_string == NULL);
	  if (cond_string)
	    b->cond_string = cond_string;
	  b->thread = thread;
	  b->task = task;
	  if (extra_string)
	    {
	      xfree (b->extra_string);
	      b->extra_string = extra_string;
	    }
	  b->condition_not_parsed = 0;
	}

      if (b->type == bp_static_tracepoint && !strace_marker_p (b))
	sals.sals[0] = update_static_tracepoint (b, sals.sals[0]);

      *found = 1;
    }
  else
    *found = 0;

  return sals;
}

/* The default re_set method, for typical hardware or software
   breakpoints.  Reevaluate the breakpoint and recreate its
   locations.  */

static void
breakpoint_re_set_default (struct breakpoint *b)
{
  int found;
  struct symtabs_and_lines sals, sals_end;
  struct symtabs_and_lines expanded = {0};
  struct symtabs_and_lines expanded_end = {0};
  struct program_space *filter_pspace = current_program_space;

  sals = location_to_sals (b, b->location, filter_pspace, &found);
  if (found)
    {
      make_cleanup (xfree, sals.sals);
      expanded = sals;
    }

  if (b->location_range_end != NULL)
    {
      sals_end = location_to_sals (b, b->location_range_end,
				   filter_pspace, &found);
      if (found)
	{
	  make_cleanup (xfree, sals_end.sals);
	  expanded_end = sals_end;
	}
    }

  update_breakpoint_locations (b, filter_pspace, expanded, expanded_end);
}

/* Default method for creating SALs from an address string.  It basically
   calls parse_breakpoint_sals.  Return 1 for success, zero for failure.  */

static void
create_sals_from_location_default (const struct event_location *location,
				   struct linespec_result *canonical,
				   enum bptype type_wanted)
{
  parse_breakpoint_sals (location, canonical);
}

/* Call create_breakpoints_sal for the given arguments.  This is the default
   function for the `create_breakpoints_sal' method of
   breakpoint_ops.  */

static void
create_breakpoints_sal_default (struct gdbarch *gdbarch,
				struct linespec_result *canonical,
				char *cond_string,
				char *extra_string,
				enum bptype type_wanted,
				enum bpdisp disposition,
				int thread,
				int task, int ignore_count,
				const struct breakpoint_ops *ops,
				int from_tty, int enabled,
				int internal, unsigned flags)
{
  create_breakpoints_sal (gdbarch, canonical, cond_string,
			  extra_string,
			  type_wanted, disposition,
			  thread, task, ignore_count, ops, from_tty,
			  enabled, internal, flags);
}

/* Decode the line represented by S by calling decode_line_full.  This is the
   default function for the `decode_location' method of breakpoint_ops.  */

static void
decode_location_default (struct breakpoint *b,
			 const struct event_location *location,
			 struct program_space *search_pspace,
			 struct symtabs_and_lines *sals)
{
  struct linespec_result canonical;

  init_linespec_result (&canonical);
  decode_line_full (location, DECODE_LINE_FUNFIRSTLINE, search_pspace,
		    (struct symtab *) NULL, 0,
		    &canonical, multiple_symbols_all,
		    b->filter);

  /* We should get 0 or 1 resulting SALs.  */
  gdb_assert (VEC_length (linespec_sals, canonical.sals) < 2);

  if (VEC_length (linespec_sals, canonical.sals) > 0)
    {
      struct linespec_sals *lsal;

      lsal = VEC_index (linespec_sals, canonical.sals, 0);
      *sals = lsal->sals;
      /* Arrange it so the destructor does not free the
	 contents.  */
      lsal->sals.sals = NULL;
    }

  destroy_linespec_result (&canonical);
}

/* Prepare the global context for a re-set of breakpoint B.  */

static struct cleanup *
prepare_re_set_context (struct breakpoint *b)
{
  input_radix = b->input_radix;
  set_language (b->language);

  return make_cleanup (null_cleanup, NULL);
}

/* Reset a breakpoint given it's struct breakpoint * BINT.
   The value we return ends up being the return value from catch_errors.
   Unused in this case.  */

static int
breakpoint_re_set_one (void *bint)
{
  /* Get past catch_errs.  */
  struct breakpoint *b = (struct breakpoint *) bint;
  struct cleanup *cleanups;

  cleanups = prepare_re_set_context (b);
  b->ops->re_set (b);
  do_cleanups (cleanups);
  return 0;
}

/* Re-set breakpoint locations for the current program space.
   Locations bound to other program spaces are left untouched.  */

void
breakpoint_re_set (void)
{
  struct breakpoint *b, *b_tmp;
  enum language save_language;
  int save_input_radix;
  struct cleanup *old_chain;

  save_language = current_language->la_language;
  save_input_radix = input_radix;
  old_chain = save_current_space_and_thread ();

  /* Note: we must not try to insert locations until after all
     breakpoints have been re-set.  Otherwise, e.g., when re-setting
     breakpoint 1, we'd insert the locations of breakpoint 2, which
     hadn't been re-set yet, and thus may have stale locations.  */

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

  /* Now we can insert.  */
  update_global_location_list (UGLL_MAY_INSERT);
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
	b->thread = ptid_to_global_thread_id (inferior_ptid);

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
      observer_notify_breakpoint_modified (b);
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

  if (args == 0 || *args == '\0')
    error_no_arg (_("one or more breakpoint numbers"));

  init_number_or_range (&state, args);

  while (!state.finished)
    {
      const char *p = state.string;

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
		match = 1;
		function (b, data);
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

  bpt->enable_state = bp_disabled;

  /* Mark breakpoint locations modified.  */
  mark_breakpoint_modified (bpt);

  if (target_supports_enable_disable_tracepoint ()
      && current_trace_status ()->running && is_tracepoint (bpt))
    {
      struct bp_location *location;
     
      for (location = bpt->loc; location; location = location->next)
	target_disable_tracepoint (location);
    }

  update_global_location_list (UGLL_DONT_INSERT);

  observer_notify_breakpoint_modified (bpt);
}

/* A callback for iterate_over_related_breakpoints.  */

static void
do_disable_breakpoint (struct breakpoint *b, void *ignore)
{
  disable_breakpoint (b);
}

/* A callback for map_breakpoint_numbers that calls
   disable_breakpoint.  */

static void
do_map_disable_breakpoint (struct breakpoint *b, void *ignore)
{
  iterate_over_related_breakpoints (b, do_disable_breakpoint, NULL);
}

static void
disable_command (char *args, int from_tty)
{
  if (args == 0)
    {
      struct breakpoint *bpt;

      ALL_BREAKPOINTS (bpt)
	if (user_breakpoint_p (bpt))
	  disable_breakpoint (bpt);
    }
  else
    {
      char *num = extract_arg (&args);

      while (num)
	{
	  if (strchr (num, '.'))
	    {
	      struct bp_location *loc = find_location_by_number (num);

	      if (loc)
		{
		  if (loc->enabled)
		    {
		      loc->enabled = 0;
		      mark_breakpoint_location_modified (loc);
		    }
		  if (target_supports_enable_disable_tracepoint ()
		      && current_trace_status ()->running && loc->owner
		      && is_tracepoint (loc->owner))
		    target_disable_tracepoint (loc);
		}
	      update_global_location_list (UGLL_DONT_INSERT);
	    }
	  else
	    map_breakpoint_numbers (num, do_map_disable_breakpoint, NULL);
	  num = extract_arg (&args);
	}
    }
}

static void
enable_breakpoint_disp (struct breakpoint *bpt, enum bpdisp disposition,
			int count)
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
      /* Initialize it just to avoid a GCC false warning.  */
      enum enable_state orig_enable_state = bp_disabled;

      TRY
	{
	  struct watchpoint *w = (struct watchpoint *) bpt;

	  orig_enable_state = bpt->enable_state;
	  bpt->enable_state = bp_enabled;
	  update_watchpoint (w, 1 /* reparse */);
	}
      CATCH (e, RETURN_MASK_ALL)
	{
	  bpt->enable_state = orig_enable_state;
	  exception_fprintf (gdb_stderr, e, _("Cannot enable watchpoint %d: "),
			     bpt->number);
	  return;
	}
      END_CATCH
    }

  bpt->enable_state = bp_enabled;

  /* Mark breakpoint locations modified.  */
  mark_breakpoint_modified (bpt);

  if (target_supports_enable_disable_tracepoint ()
      && current_trace_status ()->running && is_tracepoint (bpt))
    {
      struct bp_location *location;

      for (location = bpt->loc; location; location = location->next)
	target_enable_tracepoint (location);
    }

  bpt->disposition = disposition;
  bpt->enable_count = count;
  update_global_location_list (UGLL_MAY_INSERT);

  observer_notify_breakpoint_modified (bpt);
}


void
enable_breakpoint (struct breakpoint *bpt)
{
  enable_breakpoint_disp (bpt, bpt->disposition, 0);
}

static void
do_enable_breakpoint (struct breakpoint *bpt, void *arg)
{
  enable_breakpoint (bpt);
}

/* A callback for map_breakpoint_numbers that calls
   enable_breakpoint.  */

static void
do_map_enable_breakpoint (struct breakpoint *b, void *ignore)
{
  iterate_over_related_breakpoints (b, do_enable_breakpoint, NULL);
}

/* The enable command enables the specified breakpoints (or all defined
   breakpoints) so they once again become (or continue to be) effective
   in stopping the inferior.  */

static void
enable_command (char *args, int from_tty)
{
  if (args == 0)
    {
      struct breakpoint *bpt;

      ALL_BREAKPOINTS (bpt)
	if (user_breakpoint_p (bpt))
	  enable_breakpoint (bpt);
    }
  else
    {
      char *num = extract_arg (&args);

      while (num)
	{
	  if (strchr (num, '.'))
	    {
	      struct bp_location *loc = find_location_by_number (num);

	      if (loc)
		{
		  if (!loc->enabled)
		    {
		      loc->enabled = 1;
		      mark_breakpoint_location_modified (loc);
		    }
		  if (target_supports_enable_disable_tracepoint ()
		      && current_trace_status ()->running && loc->owner
		      && is_tracepoint (loc->owner))
		    target_enable_tracepoint (loc);
		}
	      update_global_location_list (UGLL_MAY_INSERT);
	    }
	  else
	    map_breakpoint_numbers (num, do_map_enable_breakpoint, NULL);
	  num = extract_arg (&args);
	}
    }
}

/* This struct packages up disposition data for application to multiple
   breakpoints.  */

struct disp_data
{
  enum bpdisp disp;
  int count;
};

static void
do_enable_breakpoint_disp (struct breakpoint *bpt, void *arg)
{
  struct disp_data disp_data = *(struct disp_data *) arg;

  enable_breakpoint_disp (bpt, disp_data.disp, disp_data.count);
}

static void
do_map_enable_once_breakpoint (struct breakpoint *bpt, void *ignore)
{
  struct disp_data disp = { disp_disable, 1 };

  iterate_over_related_breakpoints (bpt, do_enable_breakpoint_disp, &disp);
}

static void
enable_once_command (char *args, int from_tty)
{
  map_breakpoint_numbers (args, do_map_enable_once_breakpoint, NULL);
}

static void
do_map_enable_count_breakpoint (struct breakpoint *bpt, void *countptr)
{
  struct disp_data disp = { disp_disable, *(int *) countptr };

  iterate_over_related_breakpoints (bpt, do_enable_breakpoint_disp, &disp);
}

static void
enable_count_command (char *args, int from_tty)
{
  int count;

  if (args == NULL)
    error_no_arg (_("hit count"));

  count = get_number (&args);

  map_breakpoint_numbers (args, do_map_enable_count_breakpoint, &count);
}

static void
do_map_enable_delete_breakpoint (struct breakpoint *bpt, void *ignore)
{
  struct disp_data disp = { disp_del, 1 };

  iterate_over_related_breakpoints (bpt, do_enable_breakpoint_disp, &disp);
}

static void
enable_delete_command (char *args, int from_tty)
{
  map_breakpoint_numbers (args, do_map_enable_delete_breakpoint, NULL);
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
invalidate_bp_value_on_memory_change (struct inferior *inferior,
				      CORE_ADDR addr, ssize_t len,
				      const bfd_byte *data)
{
  struct breakpoint *bp;

  ALL_BREAKPOINTS (bp)
    if (bp->enable_state == bp_enabled
	&& bp->type == bp_hardware_watchpoint)
      {
	struct watchpoint *wp = (struct watchpoint *) bp;

	if (wp->val_valid && wp->val)
	  {
	    struct bp_location *loc;

	    for (loc = bp->loc; loc != NULL; loc = loc->next)
	      if (loc->loc_type == bp_loc_hardware_watchpoint
		  && loc->address + loc->length > addr
		  && addr + len > loc->address)
		{
		  value_free (wp->val);
		  wp->val = NULL;
		  wp->val_valid = 0;
		}
	  }
      }
}

/* Create and insert a breakpoint for software single step.  */

void
insert_single_step_breakpoint (struct gdbarch *gdbarch,
			       struct address_space *aspace, 
			       CORE_ADDR next_pc)
{
  struct thread_info *tp = inferior_thread ();
  struct symtab_and_line sal;
  CORE_ADDR pc = next_pc;

  if (tp->control.single_step_breakpoints == NULL)
    {
      tp->control.single_step_breakpoints
	= new_single_step_breakpoint (tp->global_num, gdbarch);
    }

  sal = find_pc_line (pc, 0);
  sal.pc = pc;
  sal.section = find_pc_overlay (pc);
  sal.explicit_pc = 1;
  add_location_to_breakpoint (tp->control.single_step_breakpoints, &sal);

  update_global_location_list (UGLL_INSERT);
}

/* See breakpoint.h.  */

int
breakpoint_has_location_inserted_here (struct breakpoint *bp,
				       struct address_space *aspace,
				       CORE_ADDR pc)
{
  struct bp_location *loc;

  for (loc = bp->loc; loc != NULL; loc = loc->next)
    if (loc->inserted
	&& breakpoint_location_address_match (loc, aspace, pc))
      return 1;

  return 0;
}

/* Check whether a software single-step breakpoint is inserted at
   PC.  */

int
single_step_breakpoint_inserted_here_p (struct address_space *aspace,
					CORE_ADDR pc)
{
  struct breakpoint *bpt;

  ALL_BREAKPOINTS (bpt)
    {
      if (bpt->type == bp_single_step
	  && breakpoint_has_location_inserted_here (bpt, aspace, pc))
	return 1;
    }
  return 0;
}

/* Tracepoint-specific operations.  */

/* Set tracepoint count to NUM.  */
static void
set_tracepoint_count (int num)
{
  tracepoint_count = num;
  set_internalvar_integer (lookup_internalvar ("tpnum"), num);
}

static void
trace_command (char *arg, int from_tty)
{
  struct breakpoint_ops *ops;
  struct event_location *location;
  struct cleanup *back_to;

  location = string_to_event_location (&arg, current_language);
  back_to = make_cleanup_delete_event_location (location);
  if (location != NULL
      && event_location_type (location) == PROBE_LOCATION)
    ops = &tracepoint_probe_breakpoint_ops;
  else
    ops = &tracepoint_breakpoint_ops;

  create_breakpoint (get_current_arch (),
		     location,
		     NULL, 0, arg, 1 /* parse arg */,
		     0 /* tempflag */,
		     bp_tracepoint /* type_wanted */,
		     0 /* Ignore count */,
		     pending_break_support,
		     ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */, 0);
  do_cleanups (back_to);
}

static void
ftrace_command (char *arg, int from_tty)
{
  struct event_location *location;
  struct cleanup *back_to;

  location = string_to_event_location (&arg, current_language);
  back_to = make_cleanup_delete_event_location (location);
  create_breakpoint (get_current_arch (),
		     location,
		     NULL, 0, arg, 1 /* parse arg */,
		     0 /* tempflag */,
		     bp_fast_tracepoint /* type_wanted */,
		     0 /* Ignore count */,
		     pending_break_support,
		     &tracepoint_breakpoint_ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */, 0);
  do_cleanups (back_to);
}

/* strace command implementation.  Creates a static tracepoint.  */

static void
strace_command (char *arg, int from_tty)
{
  struct breakpoint_ops *ops;
  struct event_location *location;
  struct cleanup *back_to;

  /* Decide if we are dealing with a static tracepoint marker (`-m'),
     or with a normal static tracepoint.  */
  if (arg && startswith (arg, "-m") && isspace (arg[2]))
    {
      ops = &strace_marker_breakpoint_ops;
      location = new_linespec_location (&arg);
    }
  else
    {
      ops = &tracepoint_breakpoint_ops;
      location = string_to_event_location (&arg, current_language);
    }

  back_to = make_cleanup_delete_event_location (location);
  create_breakpoint (get_current_arch (),
		     location,
		     NULL, 0, arg, 1 /* parse arg */,
		     0 /* tempflag */,
		     bp_static_tracepoint /* type_wanted */,
		     0 /* Ignore count */,
		     pending_break_support,
		     ops,
		     from_tty,
		     1 /* enabled */,
		     0 /* internal */, 0);
  do_cleanups (back_to);
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
  
struct tracepoint *
create_tracepoint_from_upload (struct uploaded_tp *utp)
{
  char *addr_str, small_buf[100];
  struct tracepoint *tp;
  struct event_location *location;
  struct cleanup *cleanup;

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
      xsnprintf (small_buf, sizeof (small_buf), "*%s", hex_string (utp->addr));
      addr_str = small_buf;
    }

  /* There's not much we can do with a sequence of bytecodes.  */
  if (utp->cond && !utp->cond_string)
    warning (_("Uploaded tracepoint %d condition "
	       "has no source form, ignoring it"),
	     utp->number);

  location = string_to_event_location (&addr_str, current_language);
  cleanup = make_cleanup_delete_event_location (location);
  if (!create_breakpoint (get_current_arch (),
			  location,
			  utp->cond_string, -1, addr_str,
			  0 /* parse cond/thread */,
			  0 /* tempflag */,
			  utp->type /* type_wanted */,
			  0 /* Ignore count */,
			  pending_break_support,
			  &tracepoint_breakpoint_ops,
			  0 /* from_tty */,
			  utp->enabled /* enabled */,
			  0 /* internal */,
			  CREATE_BREAKPOINT_FLAGS_INSERTED))
    {
      do_cleanups (cleanup);
      return NULL;
    }

  do_cleanups (cleanup);

  /* Get the tracepoint we just created.  */
  tp = get_tracepoint (tracepoint_count);
  gdb_assert (tp != NULL);

  if (utp->pass > 0)
    {
      xsnprintf (small_buf, sizeof (small_buf), "%d %d", utp->pass,
		 tp->base.number);

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

      breakpoint_set_commands (&tp->base, cmd_list);
    }
  else if (!VEC_empty (char_ptr, utp->actions)
	   || !VEC_empty (char_ptr, utp->step_actions))
    warning (_("Uploaded tracepoint %d actions "
	       "have no source form, ignoring them"),
	     utp->number);

  /* Copy any status information that might be available.  */
  tp->base.hit_count = utp->hit_count;
  tp->traceframe_usage = utp->traceframe_usage;

  return tp;
}
  
/* Print information on tracepoint number TPNUM_EXP, or all if
   omitted.  */

static void
tracepoints_info (char *args, int from_tty)
{
  struct ui_out *uiout = current_uiout;
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
	if (is_tracepoint (b) && user_breakpoint_p (b))
	  {
	    breaks_to_delete = 1;
	    break;
	  }

      /* Ask user only if there are some breakpoints to delete.  */
      if (!from_tty
	  || (breaks_to_delete && query (_("Delete all tracepoints? "))))
	{
	  ALL_BREAKPOINTS_SAFE (b, b_tmp)
	    if (is_tracepoint (b) && user_breakpoint_p (b))
	      delete_breakpoint (b);
	}
    }
  else
    map_breakpoint_numbers (arg, do_map_delete_breakpoint, NULL);
}

/* Helper function for trace_pass_command.  */

static void
trace_pass_set_count (struct tracepoint *tp, int count, int from_tty)
{
  tp->pass_count = count;
  observer_notify_breakpoint_modified (&tp->base);
  if (from_tty)
    printf_filtered (_("Setting tracepoint %d's passcount to %d\n"),
		     tp->base.number, count);
}

/* Set passcount for tracepoint.

   First command argument is passcount, second is tracepoint number.
   If tracepoint number omitted, apply to most recently defined.
   Also accepts special argument "all".  */

static void
trace_pass_command (char *args, int from_tty)
{
  struct tracepoint *t1;
  unsigned int count;

  if (args == 0 || *args == 0)
    error (_("passcount command requires an "
	     "argument (count + optional TP num)"));

  count = strtoul (args, &args, 10);	/* Count comes first, then TP num.  */

  args = skip_spaces (args);
  if (*args && strncasecmp (args, "all", 3) == 0)
    {
      struct breakpoint *b;

      args += 3;			/* Skip special argument "all".  */
      if (*args)
	error (_("Junk at end of arguments."));

      ALL_TRACEPOINTS (b)
      {
	t1 = (struct tracepoint *) b;
	trace_pass_set_count (t1, count, from_tty);
      }
    }
  else if (*args == '\0')
    {
      t1 = get_tracepoint_by_number (&args, NULL);
      if (t1)
	trace_pass_set_count (t1, count, from_tty);
    }
  else
    {
      struct get_number_or_range_state state;

      init_number_or_range (&state, args);
      while (!state.finished)
	{
	  t1 = get_tracepoint_by_number (&args, &state);
	  if (t1)
	    trace_pass_set_count (t1, count, from_tty);
	}
    }
}

struct tracepoint *
get_tracepoint (int num)
{
  struct breakpoint *t;

  ALL_TRACEPOINTS (t)
    if (t->number == num)
      return (struct tracepoint *) t;

  return NULL;
}

/* Find the tracepoint with the given target-side number (which may be
   different from the tracepoint number after disconnecting and
   reconnecting).  */

struct tracepoint *
get_tracepoint_by_number_on_target (int num)
{
  struct breakpoint *b;

  ALL_TRACEPOINTS (b)
    {
      struct tracepoint *t = (struct tracepoint *) b;

      if (t->number_on_target == num)
	return t;
    }

  return NULL;
}

/* Utility: parse a tracepoint number and look it up in the list.
   If STATE is not NULL, use, get_number_or_range_state and ignore ARG.
   If the argument is missing, the most recent tracepoint
   (tracepoint_count) is returned.  */

struct tracepoint *
get_tracepoint_by_number (char **arg,
			  struct get_number_or_range_state *state)
{
  struct breakpoint *t;
  int tpnum;
  char *instring = arg == NULL ? NULL : *arg;

  if (state)
    {
      gdb_assert (!state->finished);
      tpnum = get_number_or_range (state);
    }
  else if (arg == NULL || *arg == NULL || ! **arg)
    tpnum = tracepoint_count;
  else
    tpnum = get_number (arg);

  if (tpnum <= 0)
    {
      if (instring && *instring)
	printf_filtered (_("bad tracepoint number at or near '%s'\n"), 
			 instring);
      else
	printf_filtered (_("No previous tracepoint\n"));
      return NULL;
    }

  ALL_TRACEPOINTS (t)
    if (t->number == tpnum)
    {
      return (struct tracepoint *) t;
    }

  printf_unfiltered ("No tracepoint number %d.\n", tpnum);
  return NULL;
}

void
print_recreate_thread (struct breakpoint *b, struct ui_file *fp)
{
  if (b->thread != -1)
    fprintf_unfiltered (fp, " thread %d", b->thread);

  if (b->task != 0)
    fprintf_unfiltered (fp, " task %d", b->task);

  fprintf_unfiltered (fp, "\n");
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

  filename = tilde_expand (filename);
  cleanup = make_cleanup (xfree, filename);
  fp = gdb_fopen (filename, "w");
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

    tp->ops->print_recreate (tp, fp);

    /* Note, we can't rely on tp->number for anything, as we can't
       assume the recreated breakpoint numbers will match.  Use $bpnum
       instead.  */

    if (tp->cond_string)
      fprintf_unfiltered (fp, "  condition $bpnum %s\n", tp->cond_string);

    if (tp->ignore_count)
      fprintf_unfiltered (fp, "  ignore $bpnum %d\n", tp->ignore_count);

    if (tp->type != bp_dprintf && tp->commands)
      {
	fprintf_unfiltered (fp, "  commands\n");
	
	ui_out_redirect (current_uiout, fp);
	TRY
	  {
	    print_command_lines (current_uiout, tp->commands->commands, 2);
	  }
	CATCH (ex, RETURN_MASK_ALL)
	  {
	    ui_out_redirect (current_uiout, NULL);
	    throw_exception (ex);
	  }
	END_CATCH

	ui_out_redirect (current_uiout, NULL);
	fprintf_unfiltered (fp, "  end\n");
      }

    if (tp->enable_state == bp_disabled)
      fprintf_unfiltered (fp, "disable $bpnum\n");

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

  if (from_tty)
    printf_filtered (_("Saved to file '%s'.\n"), filename);
  do_cleanups (cleanup);
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


/* This help string is used to consolidate all the help string for specifying
   locations used by several commands.  */

#define LOCATION_HELP_STRING \
"Linespecs are colon-separated lists of location parameters, such as\n\
source filename, function name, label name, and line number.\n\
Example: To specify the start of a label named \"the_top\" in the\n\
function \"fact\" in the file \"factorial.c\", use\n\
\"factorial.c:fact:the_top\".\n\
\n\
Address locations begin with \"*\" and specify an exact address in the\n\
program.  Example: To specify the fourth byte past the start function\n\
\"main\", use \"*main + 4\".\n\
\n\
Explicit locations are similar to linespecs but use an option/argument\n\
syntax to specify location parameters.\n\
Example: To specify the start of the label named \"the_top\" in the\n\
function \"fact\" in the file \"factorial.c\", use \"-source factorial.c\n\
-function fact -label the_top\".\n"

/* This help string is used for the break, hbreak, tbreak and thbreak
   commands.  It is defined as a macro to prevent duplication.
   COMMAND should be a string constant containing the name of the
   command.  */

#define BREAK_ARGS_HELP(command) \
command" [PROBE_MODIFIER] [LOCATION] [thread THREADNUM] [if CONDITION]\n\
PROBE_MODIFIER shall be present if the command is to be placed in a\n\
probe point.  Accepted values are `-probe' (for a generic, automatically\n\
guessed probe type), `-probe-stap' (for a SystemTap probe) or \n\
`-probe-dtrace' (for a DTrace probe).\n\
LOCATION may be a linespec, address, or explicit location as described\n\
below.\n\
\n\
With no LOCATION, uses current execution address of the selected\n\
stack frame.  This is useful for breaking on return to a stack frame.\n\
\n\
THREADNUM is the number from \"info threads\".\n\
CONDITION is a boolean expression.\n\
\n" LOCATION_HELP_STRING "\n\
Multiple breakpoints at one place are permitted, and useful if their\n\
conditions are different.\n\
\n\
Do \"help breakpoints\" for info on other commands dealing with breakpoints."

/* List of subcommands for "catch".  */
static struct cmd_list_element *catch_cmdlist;

/* List of subcommands for "tcatch".  */
static struct cmd_list_element *tcatch_cmdlist;

void
add_catch_command (char *name, char *docstring,
		   cmd_sfunc_ftype *sfunc,
		   completer_ftype *completer,
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
save_command (char *arg, int from_tty)
{
  printf_unfiltered (_("\"save\" must be followed by "
		       "the name of a save subcommand.\n"));
  help_list (save_cmdlist, "save ", all_commands, gdb_stdout);
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

/* Zero if any of the breakpoint's locations could be a location where
   functions have been inlined, nonzero otherwise.  */

static int
is_non_inline_function (struct breakpoint *b)
{
  /* The shared library event breakpoint is set on the address of a
     non-inline function.  */
  if (b->type == bp_shlib_event)
    return 1;

  return 0;
}

/* Nonzero if the specified PC cannot be a location where functions
   have been inlined.  */

int
pc_at_non_inline_function (struct address_space *aspace, CORE_ADDR pc,
			   const struct target_waitstatus *ws)
{
  struct breakpoint *b;
  struct bp_location *bl;

  ALL_BREAKPOINTS (b)
    {
      if (!is_non_inline_function (b))
	continue;

      for (bl = b->loc; bl != NULL; bl = bl->next)
	{
	  if (!bl->shlib_disabled
	      && bpstat_check_location (bl, aspace, pc, ws))
	    return 1;
	}
    }

  return 0;
}

/* Remove any references to OBJFILE which is going to be freed.  */

void
breakpoint_free_objfile (struct objfile *objfile)
{
  struct bp_location **locp, *loc;

  ALL_BP_LOCATIONS (loc, locp)
    if (loc->symtab != NULL && SYMTAB_OBJFILE (loc->symtab) == objfile)
      loc->symtab = NULL;
}

void
initialize_breakpoint_ops (void)
{
  static int initialized = 0;

  struct breakpoint_ops *ops;

  if (initialized)
    return;
  initialized = 1;

  /* The breakpoint_ops structure to be inherit by all kinds of
     breakpoints (real breakpoints, i.e., user "break" breakpoints,
     internal and momentary breakpoints, etc.).  */
  ops = &bkpt_base_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->re_set = bkpt_re_set;
  ops->insert_location = bkpt_insert_location;
  ops->remove_location = bkpt_remove_location;
  ops->breakpoint_hit = bkpt_breakpoint_hit;
  ops->create_sals_from_location = bkpt_create_sals_from_location;
  ops->create_breakpoints_sal = bkpt_create_breakpoints_sal;
  ops->decode_location = bkpt_decode_location;

  /* The breakpoint_ops structure to be used in regular breakpoints.  */
  ops = &bkpt_breakpoint_ops;
  *ops = bkpt_base_breakpoint_ops;
  ops->re_set = bkpt_re_set;
  ops->resources_needed = bkpt_resources_needed;
  ops->print_it = bkpt_print_it;
  ops->print_mention = bkpt_print_mention;
  ops->print_recreate = bkpt_print_recreate;

  /* Ranged breakpoints.  */
  ops = &ranged_breakpoint_ops;
  *ops = bkpt_breakpoint_ops;
  ops->breakpoint_hit = breakpoint_hit_ranged_breakpoint;
  ops->resources_needed = resources_needed_ranged_breakpoint;
  ops->print_it = print_it_ranged_breakpoint;
  ops->print_one = print_one_ranged_breakpoint;
  ops->print_one_detail = print_one_detail_ranged_breakpoint;
  ops->print_mention = print_mention_ranged_breakpoint;
  ops->print_recreate = print_recreate_ranged_breakpoint;

  /* Internal breakpoints.  */
  ops = &internal_breakpoint_ops;
  *ops = bkpt_base_breakpoint_ops;
  ops->re_set = internal_bkpt_re_set;
  ops->check_status = internal_bkpt_check_status;
  ops->print_it = internal_bkpt_print_it;
  ops->print_mention = internal_bkpt_print_mention;

  /* Momentary breakpoints.  */
  ops = &momentary_breakpoint_ops;
  *ops = bkpt_base_breakpoint_ops;
  ops->re_set = momentary_bkpt_re_set;
  ops->check_status = momentary_bkpt_check_status;
  ops->print_it = momentary_bkpt_print_it;
  ops->print_mention = momentary_bkpt_print_mention;

  /* Momentary breakpoints for bp_longjmp and bp_exception.  */
  ops = &longjmp_breakpoint_ops;
  *ops = momentary_breakpoint_ops;
  ops->dtor = longjmp_bkpt_dtor;

  /* Probe breakpoints.  */
  ops = &bkpt_probe_breakpoint_ops;
  *ops = bkpt_breakpoint_ops;
  ops->insert_location = bkpt_probe_insert_location;
  ops->remove_location = bkpt_probe_remove_location;
  ops->create_sals_from_location = bkpt_probe_create_sals_from_location;
  ops->decode_location = bkpt_probe_decode_location;

  /* Watchpoints.  */
  ops = &watchpoint_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->dtor = dtor_watchpoint;
  ops->re_set = re_set_watchpoint;
  ops->insert_location = insert_watchpoint;
  ops->remove_location = remove_watchpoint;
  ops->breakpoint_hit = breakpoint_hit_watchpoint;
  ops->check_status = check_status_watchpoint;
  ops->resources_needed = resources_needed_watchpoint;
  ops->works_in_software_mode = works_in_software_mode_watchpoint;
  ops->print_it = print_it_watchpoint;
  ops->print_mention = print_mention_watchpoint;
  ops->print_recreate = print_recreate_watchpoint;
  ops->explains_signal = explains_signal_watchpoint;

  /* Masked watchpoints.  */
  ops = &masked_watchpoint_breakpoint_ops;
  *ops = watchpoint_breakpoint_ops;
  ops->insert_location = insert_masked_watchpoint;
  ops->remove_location = remove_masked_watchpoint;
  ops->resources_needed = resources_needed_masked_watchpoint;
  ops->works_in_software_mode = works_in_software_mode_masked_watchpoint;
  ops->print_it = print_it_masked_watchpoint;
  ops->print_one_detail = print_one_detail_masked_watchpoint;
  ops->print_mention = print_mention_masked_watchpoint;
  ops->print_recreate = print_recreate_masked_watchpoint;

  /* Tracepoints.  */
  ops = &tracepoint_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->re_set = tracepoint_re_set;
  ops->breakpoint_hit = tracepoint_breakpoint_hit;
  ops->print_one_detail = tracepoint_print_one_detail;
  ops->print_mention = tracepoint_print_mention;
  ops->print_recreate = tracepoint_print_recreate;
  ops->create_sals_from_location = tracepoint_create_sals_from_location;
  ops->create_breakpoints_sal = tracepoint_create_breakpoints_sal;
  ops->decode_location = tracepoint_decode_location;

  /* Probe tracepoints.  */
  ops = &tracepoint_probe_breakpoint_ops;
  *ops = tracepoint_breakpoint_ops;
  ops->create_sals_from_location = tracepoint_probe_create_sals_from_location;
  ops->decode_location = tracepoint_probe_decode_location;

  /* Static tracepoints with marker (`-m').  */
  ops = &strace_marker_breakpoint_ops;
  *ops = tracepoint_breakpoint_ops;
  ops->create_sals_from_location = strace_marker_create_sals_from_location;
  ops->create_breakpoints_sal = strace_marker_create_breakpoints_sal;
  ops->decode_location = strace_marker_decode_location;

  /* Fork catchpoints.  */
  ops = &catch_fork_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->insert_location = insert_catch_fork;
  ops->remove_location = remove_catch_fork;
  ops->breakpoint_hit = breakpoint_hit_catch_fork;
  ops->print_it = print_it_catch_fork;
  ops->print_one = print_one_catch_fork;
  ops->print_mention = print_mention_catch_fork;
  ops->print_recreate = print_recreate_catch_fork;

  /* Vfork catchpoints.  */
  ops = &catch_vfork_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->insert_location = insert_catch_vfork;
  ops->remove_location = remove_catch_vfork;
  ops->breakpoint_hit = breakpoint_hit_catch_vfork;
  ops->print_it = print_it_catch_vfork;
  ops->print_one = print_one_catch_vfork;
  ops->print_mention = print_mention_catch_vfork;
  ops->print_recreate = print_recreate_catch_vfork;

  /* Exec catchpoints.  */
  ops = &catch_exec_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->dtor = dtor_catch_exec;
  ops->insert_location = insert_catch_exec;
  ops->remove_location = remove_catch_exec;
  ops->breakpoint_hit = breakpoint_hit_catch_exec;
  ops->print_it = print_it_catch_exec;
  ops->print_one = print_one_catch_exec;
  ops->print_mention = print_mention_catch_exec;
  ops->print_recreate = print_recreate_catch_exec;

  /* Solib-related catchpoints.  */
  ops = &catch_solib_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->dtor = dtor_catch_solib;
  ops->insert_location = insert_catch_solib;
  ops->remove_location = remove_catch_solib;
  ops->breakpoint_hit = breakpoint_hit_catch_solib;
  ops->check_status = check_status_catch_solib;
  ops->print_it = print_it_catch_solib;
  ops->print_one = print_one_catch_solib;
  ops->print_mention = print_mention_catch_solib;
  ops->print_recreate = print_recreate_catch_solib;

  ops = &dprintf_breakpoint_ops;
  *ops = bkpt_base_breakpoint_ops;
  ops->re_set = dprintf_re_set;
  ops->resources_needed = bkpt_resources_needed;
  ops->print_it = bkpt_print_it;
  ops->print_mention = bkpt_print_mention;
  ops->print_recreate = dprintf_print_recreate;
  ops->after_condition_true = dprintf_after_condition_true;
  ops->breakpoint_hit = dprintf_breakpoint_hit;
}

/* Chain containing all defined "enable breakpoint" subcommands.  */

static struct cmd_list_element *enablebreaklist = NULL;

void
_initialize_breakpoint (void)
{
  struct cmd_list_element *c;

  initialize_breakpoint_ops ();

  observer_attach_solib_unloaded (disable_breakpoints_in_unloaded_shlib);
  observer_attach_free_objfile (disable_breakpoints_in_freed_objfile);
  observer_attach_memory_changed (invalidate_bp_value_on_memory_change);

  breakpoint_objfile_key
    = register_objfile_data_with_cleanup (NULL, free_breakpoint_probes);

  breakpoint_chain = 0;
  /* Don't bother to call set_breakpoint_count.  $bpnum isn't useful
     before a breakpoint is set.  */
  breakpoint_count = 0;

  tracepoint_count = 0;

  add_com ("ignore", class_breakpoint, ignore_command, _("\
Set ignore-count of breakpoint number N to COUNT.\n\
Usage is `ignore N COUNT'."));

  add_com ("commands", class_breakpoint, commands_command, _("\
Set commands to be executed when a breakpoint is hit.\n\
Give breakpoint number as argument after \"commands\".\n\
With no argument, the targeted breakpoint is the last one set.\n\
The commands themselves follow starting on the next line.\n\
Type a line containing \"end\" to indicate the end of them.\n\
Give \"silent\" as the first line to make the breakpoint silent;\n\
then no output is printed when it is hit, except what the commands print."));

  c = add_com ("condition", class_breakpoint, condition_command, _("\
Specify breakpoint number N to break only if COND is true.\n\
Usage is `condition N COND', where N is an integer and COND is an\n\
expression to be evaluated whenever breakpoint N is reached."));
  set_cmd_completer (c, condition_completer);

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

  add_cmd ("count", no_class, enable_count_command, _("\
Enable breakpoints for COUNT hits.  Give count and then breakpoint numbers.\n\
If a breakpoint is hit while enabled in this fashion,\n\
the count is decremented; when it reaches zero, the breakpoint is disabled."),
	   &enablebreaklist);

  add_cmd ("delete", no_class, enable_delete_command, _("\
Enable breakpoints and delete when hit.  Give breakpoint numbers.\n\
If a breakpoint is hit while enabled in this fashion, it is deleted."),
	   &enablelist);

  add_cmd ("once", no_class, enable_once_command, _("\
Enable breakpoints for one hit.  Give breakpoint numbers.\n\
If a breakpoint is hit while enabled in this fashion, it becomes disabled."),
	   &enablelist);

  add_cmd ("count", no_class, enable_count_command, _("\
Enable breakpoints for COUNT hits.  Give count and then breakpoint numbers.\n\
If a breakpoint is hit while enabled in this fashion,\n\
the count is decremented; when it reaches zero, the breakpoint is disabled."),
	   &enablelist);

  add_prefix_cmd ("disable", class_breakpoint, disable_command, _("\
Disable some breakpoints.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To disable all breakpoints, give no argument.\n\
A disabled breakpoint is not forgotten, but has no effect until re-enabled."),
		  &disablelist, "disable ", 1, &cmdlist);
  add_com_alias ("dis", "disable", class_breakpoint, 1);
  add_com_alias ("disa", "disable", class_breakpoint, 1);

  add_cmd ("breakpoints", class_alias, disable_command, _("\
Disable some breakpoints.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To disable all breakpoints, give no argument.\n\
A disabled breakpoint is not forgotten, but has no effect until re-enabled.\n\
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

  add_cmd ("breakpoints", class_alias, delete_command, _("\
Delete some breakpoints or auto-display expressions.\n\
Arguments are breakpoint numbers with spaces in between.\n\
To delete all breakpoints, give no argument.\n\
This command may be abbreviated \"delete\"."),
	   &deletelist);

  add_com ("clear", class_breakpoint, clear_command, _("\
Clear breakpoint at specified location.\n\
Argument may be a linespec, explicit, or address location as described below.\n\
\n\
With no argument, clears all breakpoints in the line that the selected frame\n\
is executing in.\n"
"\n" LOCATION_HELP_STRING "\n\
See also the \"delete\" command which clears breakpoints by number."));
  add_com_alias ("cl", "clear", class_breakpoint, 1);

  c = add_com ("break", class_breakpoint, break_command, _("\
Set breakpoint at specified location.\n"
BREAK_ARGS_HELP ("break")));
  set_cmd_completer (c, location_completer);

  add_com_alias ("b", "break", class_run, 1);
  add_com_alias ("br", "break", class_run, 1);
  add_com_alias ("bre", "break", class_run, 1);
  add_com_alias ("brea", "break", class_run, 1);

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
  add_catch_command ("load", _("Catch loads of shared libraries.\n\
Usage: catch load [REGEX]\n\
If REGEX is given, only stop for libraries matching the regular expression."),
		     catch_load_command_1,
		     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("unload", _("Catch unloads of shared libraries.\n\
Usage: catch unload [REGEX]\n\
If REGEX is given, only stop for libraries matching the regular expression."),
		     catch_unload_command_1,
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
Set a tracepoint at specified location.\n\
\n"
BREAK_ARGS_HELP ("trace") "\n\
Do \"help tracepoints\" for info on other tracepoint commands."));
  set_cmd_completer (c, location_completer);

  add_com_alias ("tp", "trace", class_alias, 0);
  add_com_alias ("tr", "trace", class_alias, 1);
  add_com_alias ("tra", "trace", class_alias, 1);
  add_com_alias ("trac", "trace", class_alias, 1);

  c = add_com ("ftrace", class_breakpoint, ftrace_command, _("\
Set a fast tracepoint at specified location.\n\
\n"
BREAK_ARGS_HELP ("ftrace") "\n\
Do \"help tracepoints\" for info on other tracepoint commands."));
  set_cmd_completer (c, location_completer);

  c = add_com ("strace", class_breakpoint, strace_command, _("\
Set a static tracepoint at location or marker.\n\
\n\
strace [LOCATION] [if CONDITION]\n\
LOCATION may be a linespec, explicit, or address location (described below) \n\
or -m MARKER_ID.\n\n\
If a marker id is specified, probe the marker with that name.  With\n\
no LOCATION, uses current execution address of the selected stack frame.\n\
Static tracepoints accept an extra collect action -- ``collect $_sdata''.\n\
This collects arbitrary user data passed in the probe point call to the\n\
tracing library.  You can inspect it when analyzing the trace buffer,\n\
by printing the $_sdata variable like any other convenience variable.\n\
\n\
CONDITION is a boolean expression.\n\
\n" LOCATION_HELP_STRING "\n\
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
  add_alias_cmd ("tr", "tracepoints", class_trace, 1, &deletelist);

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

  add_setshow_boolean_cmd ("always-inserted", class_support,
			   &always_inserted_mode, _("\
Set mode for inserting breakpoints."), _("\
Show mode for inserting breakpoints."), _("\
When this mode is on, breakpoints are inserted immediately as soon as\n\
they're created, kept inserted even when execution stops, and removed\n\
only when the user deletes them.  When this mode is off (the default),\n\
breakpoints are inserted only when execution continues, and removed\n\
when execution stops."),
				NULL,
				&show_always_inserted_mode,
				&breakpoint_set_cmdlist,
				&breakpoint_show_cmdlist);

  add_setshow_enum_cmd ("condition-evaluation", class_breakpoint,
			condition_evaluation_enums,
			&condition_evaluation_mode_1, _("\
Set mode of breakpoint condition evaluation."), _("\
Show mode of breakpoint condition evaluation."), _("\
When this is set to \"host\", breakpoint conditions will be\n\
evaluated on the host's side by GDB.  When it is set to \"target\",\n\
breakpoint conditions will be downloaded to the target (if the target\n\
supports such feature) and conditions will be evaluated on the target's side.\n\
If this is set to \"auto\" (default), this will be automatically set to\n\
\"target\" if it supports condition evaluation, otherwise it will\n\
be set to \"gdb\""),
			   &set_condition_evaluation_mode,
			   &show_condition_evaluation_mode,
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

  c = add_com ("dprintf", class_breakpoint, dprintf_command, _("\
Set a dynamic printf at specified location.\n\
dprintf location,format string,arg1,arg2,...\n\
location may be a linespec, explicit, or address location.\n"
"\n" LOCATION_HELP_STRING));
  set_cmd_completer (c, location_completer);

  add_setshow_enum_cmd ("dprintf-style", class_support,
			dprintf_style_enums, &dprintf_style, _("\
Set the style of usage for dynamic printf."), _("\
Show the style of usage for dynamic printf."), _("\
This setting chooses how GDB will do a dynamic printf.\n\
If the value is \"gdb\", then the printing is done by GDB to its own\n\
console, as with the \"printf\" command.\n\
If the value is \"call\", the print is done by calling a function in your\n\
program; by default printf(), but you can choose a different function or\n\
output stream by setting dprintf-function and dprintf-channel."),
			update_dprintf_commands, NULL,
			&setlist, &showlist);

  dprintf_function = xstrdup ("printf");
  add_setshow_string_cmd ("dprintf-function", class_support,
			  &dprintf_function, _("\
Set the function to use for dynamic printf"), _("\
Show the function to use for dynamic printf"), NULL,
			  update_dprintf_commands, NULL,
			  &setlist, &showlist);

  dprintf_channel = xstrdup ("");
  add_setshow_string_cmd ("dprintf-channel", class_support,
			  &dprintf_channel, _("\
Set the channel to use for dynamic printf"), _("\
Show the channel to use for dynamic printf"), NULL,
			  update_dprintf_commands, NULL,
			  &setlist, &showlist);

  add_setshow_boolean_cmd ("disconnected-dprintf", no_class,
			   &disconnected_dprintf, _("\
Set whether dprintf continues after GDB disconnects."), _("\
Show whether dprintf continues after GDB disconnects."), _("\
Use this to let dprintf commands continue to hit and produce output\n\
even if GDB disconnects or detaches from the target."),
			   NULL,
			   NULL,
			   &setlist, &showlist);

  add_com ("agent-printf", class_vars, agent_printf_command, _("\
agent-printf \"printf format string\", arg1, arg2, arg3, ..., argn\n\
(target agent only) This is useful for formatted output in user-defined commands."));

  automatic_hardware_breakpoints = 1;

  observer_attach_about_to_proceed (breakpoint_about_to_proceed);
  observer_attach_thread_exit (remove_threaded_breakpoints);
}
