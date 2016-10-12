/* Tracing functionality for remote targets in custom GDB protocol

   Copyright (C) 1997-2015 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "frame.h"
#include "gdbtypes.h"
#include "expression.h"
#include "gdbcmd.h"
#include "value.h"
#include "target.h"
#include "target-dcache.h"
#include "language.h"
#include "inferior.h"
#include "breakpoint.h"
#include "tracepoint.h"
#include "linespec.h"
#include "regcache.h"
#include "completer.h"
#include "block.h"
#include "dictionary.h"
#include "observer.h"
#include "user-regs.h"
#include "valprint.h"
#include "gdbcore.h"
#include "objfiles.h"
#include "filenames.h"
#include "gdbthread.h"
#include "stack.h"
#include "remote.h"
#include "source.h"
#include "ax.h"
#include "ax-gdb.h"
#include "memrange.h"
#include "cli/cli-utils.h"
#include "probe.h"
#include "ctf.h"
#include "filestuff.h"
#include "rsp-low.h"
#include "tracefile.h"

/* readline include files */
#include "readline/readline.h"
#include "readline/history.h"

/* readline defines this.  */
#undef savestring

#include <unistd.h>

/* Maximum length of an agent aexpression.
   This accounts for the fact that packets are limited to 400 bytes
   (which includes everything -- including the checksum), and assumes
   the worst case of maximum length for each of the pieces of a
   continuation packet.

   NOTE: expressions get mem2hex'ed otherwise this would be twice as
   large.  (400 - 31)/2 == 184 */
#define MAX_AGENT_EXPR_LEN	184

/* A hook used to notify the UI of tracepoint operations.  */

void (*deprecated_trace_find_hook) (char *arg, int from_tty);
void (*deprecated_trace_start_stop_hook) (int start, int from_tty);

/* 
   Tracepoint.c:

   This module defines the following debugger commands:
   trace            : set a tracepoint on a function, line, or address.
   info trace       : list all debugger-defined tracepoints.
   delete trace     : delete one or more tracepoints.
   enable trace     : enable one or more tracepoints.
   disable trace    : disable one or more tracepoints.
   actions          : specify actions to be taken at a tracepoint.
   passcount        : specify a pass count for a tracepoint.
   tstart           : start a trace experiment.
   tstop            : stop a trace experiment.
   tstatus          : query the status of a trace experiment.
   tfind            : find a trace frame in the trace buffer.
   tdump            : print everything collected at the current tracepoint.
   save-tracepoints : write tracepoint setup into a file.

   This module defines the following user-visible debugger variables:
   $trace_frame : sequence number of trace frame currently being debugged.
   $trace_line  : source line of trace frame currently being debugged.
   $trace_file  : source file of trace frame currently being debugged.
   $tracepoint  : tracepoint number of trace frame currently being debugged.
 */


/* ======= Important global variables: ======= */

/* The list of all trace state variables.  We don't retain pointers to
   any of these for any reason - API is by name or number only - so it
   works to have a vector of objects.  */

typedef struct trace_state_variable tsv_s;
DEF_VEC_O(tsv_s);

static VEC(tsv_s) *tvariables;

/* The next integer to assign to a variable.  */

static int next_tsv_number = 1;

/* Number of last traceframe collected.  */
static int traceframe_number;

/* Tracepoint for last traceframe collected.  */
static int tracepoint_number;

/* The traceframe info of the current traceframe.  NULL if we haven't
   yet attempted to fetch it, or if the target does not support
   fetching this object, or if we're not inspecting a traceframe
   presently.  */
static struct traceframe_info *traceframe_info;

/* Tracing command lists.  */
static struct cmd_list_element *tfindlist;

/* List of expressions to collect by default at each tracepoint hit.  */
char *default_collect = "";

static int disconnected_tracing;

/* This variable controls whether we ask the target for a linear or
   circular trace buffer.  */

static int circular_trace_buffer;

/* This variable is the requested trace buffer size, or -1 to indicate
   that we don't care and leave it up to the target to set a size.  */

static int trace_buffer_size = -1;

/* Textual notes applying to the current and/or future trace runs.  */

char *trace_user = NULL;

/* Textual notes applying to the current and/or future trace runs.  */

char *trace_notes = NULL;

/* Textual notes applying to the stopping of a trace.  */

char *trace_stop_notes = NULL;

/* ======= Important command functions: ======= */
static void trace_actions_command (char *, int);
static void trace_start_command (char *, int);
static void trace_stop_command (char *, int);
static void trace_status_command (char *, int);
static void trace_find_command (char *, int);
static void trace_find_pc_command (char *, int);
static void trace_find_tracepoint_command (char *, int);
static void trace_find_line_command (char *, int);
static void trace_find_range_command (char *, int);
static void trace_find_outside_command (char *, int);
static void trace_dump_command (char *, int);

/* support routines */

struct collection_list;
static void add_aexpr (struct collection_list *, struct agent_expr *);
static char *mem2hex (gdb_byte *, char *, int);
static void add_register (struct collection_list *collection,
			  unsigned int regno);

static struct command_line *
  all_tracepoint_actions_and_cleanup (struct breakpoint *t);

extern void _initialize_tracepoint (void);

static struct trace_status trace_status;

const char *stop_reason_names[] = {
  "tunknown",
  "tnotrun",
  "tstop",
  "tfull",
  "tdisconnected",
  "tpasscount",
  "terror"
};

struct trace_status *
current_trace_status (void)
{
  return &trace_status;
}

/* Destroy INFO.  */

static void
free_traceframe_info (struct traceframe_info *info)
{
  if (info != NULL)
    {
      VEC_free (mem_range_s, info->memory);
      VEC_free (int, info->tvars);

      xfree (info);
    }
}

/* Free and clear the traceframe info cache of the current
   traceframe.  */

static void
clear_traceframe_info (void)
{
  free_traceframe_info (traceframe_info);
  traceframe_info = NULL;
}

/* Set traceframe number to NUM.  */
static void
set_traceframe_num (int num)
{
  traceframe_number = num;
  set_internalvar_integer (lookup_internalvar ("trace_frame"), num);
}

/* Set tracepoint number to NUM.  */
static void
set_tracepoint_num (int num)
{
  tracepoint_number = num;
  set_internalvar_integer (lookup_internalvar ("tracepoint"), num);
}

/* Set externally visible debug variables for querying/printing
   the traceframe context (line, function, file).  */

static void
set_traceframe_context (struct frame_info *trace_frame)
{
  CORE_ADDR trace_pc;
  struct symbol *traceframe_fun;
  struct symtab_and_line traceframe_sal;

  /* Save as globals for internal use.  */
  if (trace_frame != NULL
      && get_frame_pc_if_available (trace_frame, &trace_pc))
    {
      traceframe_sal = find_pc_line (trace_pc, 0);
      traceframe_fun = find_pc_function (trace_pc);

      /* Save linenumber as "$trace_line", a debugger variable visible to
	 users.  */
      set_internalvar_integer (lookup_internalvar ("trace_line"),
			       traceframe_sal.line);
    }
  else
    {
      init_sal (&traceframe_sal);
      traceframe_fun = NULL;
      set_internalvar_integer (lookup_internalvar ("trace_line"), -1);
    }

  /* Save func name as "$trace_func", a debugger variable visible to
     users.  */
  if (traceframe_fun == NULL
      || SYMBOL_LINKAGE_NAME (traceframe_fun) == NULL)
    clear_internalvar (lookup_internalvar ("trace_func"));
  else
    set_internalvar_string (lookup_internalvar ("trace_func"),
			    SYMBOL_LINKAGE_NAME (traceframe_fun));

  /* Save file name as "$trace_file", a debugger variable visible to
     users.  */
  if (traceframe_sal.symtab == NULL)
    clear_internalvar (lookup_internalvar ("trace_file"));
  else
    set_internalvar_string (lookup_internalvar ("trace_file"),
			symtab_to_filename_for_display (traceframe_sal.symtab));
}

/* Create a new trace state variable with the given name.  */

struct trace_state_variable *
create_trace_state_variable (const char *name)
{
  struct trace_state_variable tsv;

  memset (&tsv, 0, sizeof (tsv));
  tsv.name = xstrdup (name);
  tsv.number = next_tsv_number++;
  return VEC_safe_push (tsv_s, tvariables, &tsv);
}

/* Look for a trace state variable of the given name.  */

struct trace_state_variable *
find_trace_state_variable (const char *name)
{
  struct trace_state_variable *tsv;
  int ix;

  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    if (strcmp (name, tsv->name) == 0)
      return tsv;

  return NULL;
}

/* Look for a trace state variable of the given number.  Return NULL if
   not found.  */

struct trace_state_variable *
find_trace_state_variable_by_number (int number)
{
  struct trace_state_variable *tsv;
  int ix;

  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    if (tsv->number == number)
      return tsv;

  return NULL;
}

static void
delete_trace_state_variable (const char *name)
{
  struct trace_state_variable *tsv;
  int ix;

  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    if (strcmp (name, tsv->name) == 0)
      {
	observer_notify_tsv_deleted (tsv);

	xfree ((void *)tsv->name);
	VEC_unordered_remove (tsv_s, tvariables, ix);

	return;
      }

  warning (_("No trace variable named \"$%s\", not deleting"), name);
}

/* Throws an error if NAME is not valid syntax for a trace state
   variable's name.  */

void
validate_trace_state_variable_name (const char *name)
{
  const char *p;

  if (*name == '\0')
    error (_("Must supply a non-empty variable name"));

  /* All digits in the name is reserved for value history
     references.  */
  for (p = name; isdigit (*p); p++)
    ;
  if (*p == '\0')
    error (_("$%s is not a valid trace state variable name"), name);

  for (p = name; isalnum (*p) || *p == '_'; p++)
    ;
  if (*p != '\0')
    error (_("$%s is not a valid trace state variable name"), name);
}

/* The 'tvariable' command collects a name and optional expression to
   evaluate into an initial value.  */

static void
trace_variable_command (char *args, int from_tty)
{
  struct cleanup *old_chain;
  LONGEST initval = 0;
  struct trace_state_variable *tsv;
  char *name, *p;

  if (!args || !*args)
    error_no_arg (_("Syntax is $NAME [ = EXPR ]"));

  /* Only allow two syntaxes; "$name" and "$name=value".  */
  p = skip_spaces (args);

  if (*p++ != '$')
    error (_("Name of trace variable should start with '$'"));

  name = p;
  while (isalnum (*p) || *p == '_')
    p++;
  name = savestring (name, p - name);
  old_chain = make_cleanup (xfree, name);

  p = skip_spaces (p);
  if (*p != '=' && *p != '\0')
    error (_("Syntax must be $NAME [ = EXPR ]"));

  validate_trace_state_variable_name (name);

  if (*p == '=')
    initval = value_as_long (parse_and_eval (++p));

  /* If the variable already exists, just change its initial value.  */
  tsv = find_trace_state_variable (name);
  if (tsv)
    {
      if (tsv->initial_value != initval)
	{
	  tsv->initial_value = initval;
	  observer_notify_tsv_modified (tsv);
	}
      printf_filtered (_("Trace state variable $%s "
			 "now has initial value %s.\n"),
		       tsv->name, plongest (tsv->initial_value));
      do_cleanups (old_chain);
      return;
    }

  /* Create a new variable.  */
  tsv = create_trace_state_variable (name);
  tsv->initial_value = initval;

  observer_notify_tsv_created (tsv);

  printf_filtered (_("Trace state variable $%s "
		     "created, with initial value %s.\n"),
		   tsv->name, plongest (tsv->initial_value));

  do_cleanups (old_chain);
}

static void
delete_trace_variable_command (char *args, int from_tty)
{
  int ix;
  char **argv;
  struct cleanup *back_to;

  if (args == NULL)
    {
      if (query (_("Delete all trace state variables? ")))
	VEC_free (tsv_s, tvariables);
      dont_repeat ();
      observer_notify_tsv_deleted (NULL);
      return;
    }

  argv = gdb_buildargv (args);
  back_to = make_cleanup_freeargv (argv);

  for (ix = 0; argv[ix] != NULL; ix++)
    {
      if (*argv[ix] == '$')
	delete_trace_state_variable (argv[ix] + 1);
      else
	warning (_("Name \"%s\" not prefixed with '$', ignoring"), argv[ix]);
    }

  do_cleanups (back_to);

  dont_repeat ();
}

void
tvariables_info_1 (void)
{
  struct trace_state_variable *tsv;
  int ix;
  int count = 0;
  struct cleanup *back_to;
  struct ui_out *uiout = current_uiout;

  if (VEC_length (tsv_s, tvariables) == 0 && !ui_out_is_mi_like_p (uiout))
    {
      printf_filtered (_("No trace state variables.\n"));
      return;
    }

  /* Try to acquire values from the target.  */
  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix, ++count)
    tsv->value_known = target_get_trace_state_variable_value (tsv->number,
							      &(tsv->value));

  back_to = make_cleanup_ui_out_table_begin_end (uiout, 3,
                                                 count, "trace-variables");
  ui_out_table_header (uiout, 15, ui_left, "name", "Name");
  ui_out_table_header (uiout, 11, ui_left, "initial", "Initial");
  ui_out_table_header (uiout, 11, ui_left, "current", "Current");

  ui_out_table_body (uiout);

  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    {
      struct cleanup *back_to2;
      char *c;
      char *name;

      back_to2 = make_cleanup_ui_out_tuple_begin_end (uiout, "variable");

      name = concat ("$", tsv->name, (char *) NULL);
      make_cleanup (xfree, name);
      ui_out_field_string (uiout, "name", name);
      ui_out_field_string (uiout, "initial", plongest (tsv->initial_value));

      if (tsv->value_known)
        c = plongest (tsv->value);
      else if (ui_out_is_mi_like_p (uiout))
        /* For MI, we prefer not to use magic string constants, but rather
           omit the field completely.  The difference between unknown and
           undefined does not seem important enough to represent.  */
        c = NULL;
      else if (current_trace_status ()->running || traceframe_number >= 0)
	/* The value is/was defined, but we don't have it.  */
        c = "<unknown>";
      else
	/* It is not meaningful to ask about the value.  */
        c = "<undefined>";
      if (c)
        ui_out_field_string (uiout, "current", c);
      ui_out_text (uiout, "\n");

      do_cleanups (back_to2);
    }

  do_cleanups (back_to);
}

/* List all the trace state variables.  */

static void
tvariables_info (char *args, int from_tty)
{
  tvariables_info_1 ();
}

/* Stash definitions of tsvs into the given file.  */

void
save_trace_state_variables (struct ui_file *fp)
{
  struct trace_state_variable *tsv;
  int ix;

  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    {
      fprintf_unfiltered (fp, "tvariable $%s", tsv->name);
      if (tsv->initial_value)
	fprintf_unfiltered (fp, " = %s", plongest (tsv->initial_value));
      fprintf_unfiltered (fp, "\n");
    }
}

/* ACTIONS functions: */

/* The three functions:
   collect_pseudocommand, 
   while_stepping_pseudocommand, and 
   end_actions_pseudocommand
   are placeholders for "commands" that are actually ONLY to be used
   within a tracepoint action list.  If the actual function is ever called,
   it means that somebody issued the "command" at the top level,
   which is always an error.  */

static void
end_actions_pseudocommand (char *args, int from_tty)
{
  error (_("This command cannot be used at the top level."));
}

static void
while_stepping_pseudocommand (char *args, int from_tty)
{
  error (_("This command can only be used in a tracepoint actions list."));
}

static void
collect_pseudocommand (char *args, int from_tty)
{
  error (_("This command can only be used in a tracepoint actions list."));
}

static void
teval_pseudocommand (char *args, int from_tty)
{
  error (_("This command can only be used in a tracepoint actions list."));
}

/* Parse any collection options, such as /s for strings.  */

const char *
decode_agent_options (const char *exp, int *trace_string)
{
  struct value_print_options opts;

  *trace_string = 0;

  if (*exp != '/')
    return exp;

  /* Call this to borrow the print elements default for collection
     size.  */
  get_user_print_options (&opts);

  exp++;
  if (*exp == 's')
    {
      if (target_supports_string_tracing ())
	{
	  /* Allow an optional decimal number giving an explicit maximum
	     string length, defaulting it to the "print elements" value;
	     so "collect/s80 mystr" gets at most 80 bytes of string.  */
	  *trace_string = opts.print_max;
	  exp++;
	  if (*exp >= '0' && *exp <= '9')
	    *trace_string = atoi (exp);
	  while (*exp >= '0' && *exp <= '9')
	    exp++;
	}
      else
	error (_("Target does not support \"/s\" option for string tracing."));
    }
  else
    error (_("Undefined collection format \"%c\"."), *exp);

  exp = skip_spaces_const (exp);

  return exp;
}

/* Enter a list of actions for a tracepoint.  */
static void
trace_actions_command (char *args, int from_tty)
{
  struct tracepoint *t;
  struct command_line *l;

  t = get_tracepoint_by_number (&args, NULL);
  if (t)
    {
      char *tmpbuf =
	xstrprintf ("Enter actions for tracepoint %d, one per line.",
		    t->base.number);
      struct cleanup *cleanups = make_cleanup (xfree, tmpbuf);

      l = read_command_lines (tmpbuf, from_tty, 1,
			      check_tracepoint_command, t);
      do_cleanups (cleanups);
      breakpoint_set_commands (&t->base, l);
    }
  /* else just return */
}

/* Report the results of checking the agent expression, as errors or
   internal errors.  */

static void
report_agent_reqs_errors (struct agent_expr *aexpr)
{
  /* All of the "flaws" are serious bytecode generation issues that
     should never occur.  */
  if (aexpr->flaw != agent_flaw_none)
    internal_error (__FILE__, __LINE__, _("expression is malformed"));

  /* If analysis shows a stack underflow, GDB must have done something
     badly wrong in its bytecode generation.  */
  if (aexpr->min_height < 0)
    internal_error (__FILE__, __LINE__,
		    _("expression has min height < 0"));

  /* Issue this error if the stack is predicted to get too deep.  The
     limit is rather arbitrary; a better scheme might be for the
     target to report how much stack it will have available.  The
     depth roughly corresponds to parenthesization, so a limit of 20
     amounts to 20 levels of expression nesting, which is actually
     a pretty big hairy expression.  */
  if (aexpr->max_height > 20)
    error (_("Expression is too complicated."));
}

/* worker function */
void
validate_actionline (const char *line, struct breakpoint *b)
{
  struct cmd_list_element *c;
  struct expression *exp = NULL;
  struct cleanup *old_chain = NULL;
  const char *tmp_p;
  const char *p;
  struct bp_location *loc;
  struct agent_expr *aexpr;
  struct tracepoint *t = (struct tracepoint *) b;

  /* If EOF is typed, *line is NULL.  */
  if (line == NULL)
    return;

  p = skip_spaces_const (line);

  /* Symbol lookup etc.  */
  if (*p == '\0')	/* empty line: just prompt for another line.  */
    return;

  if (*p == '#')		/* comment line */
    return;

  c = lookup_cmd (&p, cmdlist, "", -1, 1);
  if (c == 0)
    error (_("`%s' is not a tracepoint action, or is ambiguous."), p);

  if (cmd_cfunc_eq (c, collect_pseudocommand))
    {
      int trace_string = 0;

      if (*p == '/')
	p = decode_agent_options (p, &trace_string);

      do
	{			/* Repeat over a comma-separated list.  */
	  QUIT;			/* Allow user to bail out with ^C.  */
	  p = skip_spaces_const (p);

	  if (*p == '$')	/* Look for special pseudo-symbols.  */
	    {
	      if (0 == strncasecmp ("reg", p + 1, 3)
		  || 0 == strncasecmp ("arg", p + 1, 3)
		  || 0 == strncasecmp ("loc", p + 1, 3)
		  || 0 == strncasecmp ("_ret", p + 1, 4)
		  || 0 == strncasecmp ("_sdata", p + 1, 6))
		{
		  p = strchr (p, ',');
		  continue;
		}
	      /* else fall thru, treat p as an expression and parse it!  */
	    }
	  tmp_p = p;
	  for (loc = t->base.loc; loc; loc = loc->next)
	    {
	      p = tmp_p;
	      exp = parse_exp_1 (&p, loc->address,
				 block_for_pc (loc->address), 1);
	      old_chain = make_cleanup (free_current_contents, &exp);

	      if (exp->elts[0].opcode == OP_VAR_VALUE)
		{
		  if (SYMBOL_CLASS (exp->elts[2].symbol) == LOC_CONST)
		    {
		      error (_("constant `%s' (value %s) "
			       "will not be collected."),
			     SYMBOL_PRINT_NAME (exp->elts[2].symbol),
			     plongest (SYMBOL_VALUE (exp->elts[2].symbol)));
		    }
		  else if (SYMBOL_CLASS (exp->elts[2].symbol)
			   == LOC_OPTIMIZED_OUT)
		    {
		      error (_("`%s' is optimized away "
			       "and cannot be collected."),
			     SYMBOL_PRINT_NAME (exp->elts[2].symbol));
		    }
		}

	      /* We have something to collect, make sure that the expr to
		 bytecode translator can handle it and that it's not too
		 long.  */
	      aexpr = gen_trace_for_expr (loc->address, exp, trace_string);
	      make_cleanup_free_agent_expr (aexpr);

	      if (aexpr->len > MAX_AGENT_EXPR_LEN)
		error (_("Expression is too complicated."));

	      ax_reqs (aexpr);

	      report_agent_reqs_errors (aexpr);

	      do_cleanups (old_chain);
	    }
	}
      while (p && *p++ == ',');
    }

  else if (cmd_cfunc_eq (c, teval_pseudocommand))
    {
      do
	{			/* Repeat over a comma-separated list.  */
	  QUIT;			/* Allow user to bail out with ^C.  */
	  p = skip_spaces_const (p);

	  tmp_p = p;
	  for (loc = t->base.loc; loc; loc = loc->next)
	    {
	      p = tmp_p;

	      /* Only expressions are allowed for this action.  */
	      exp = parse_exp_1 (&p, loc->address,
				 block_for_pc (loc->address), 1);
	      old_chain = make_cleanup (free_current_contents, &exp);

	      /* We have something to evaluate, make sure that the expr to
		 bytecode translator can handle it and that it's not too
		 long.  */
	      aexpr = gen_eval_for_expr (loc->address, exp);
	      make_cleanup_free_agent_expr (aexpr);

	      if (aexpr->len > MAX_AGENT_EXPR_LEN)
		error (_("Expression is too complicated."));

	      ax_reqs (aexpr);
	      report_agent_reqs_errors (aexpr);

	      do_cleanups (old_chain);
	    }
	}
      while (p && *p++ == ',');
    }

  else if (cmd_cfunc_eq (c, while_stepping_pseudocommand))
    {
      char *endp;

      p = skip_spaces_const (p);
      t->step_count = strtol (p, &endp, 0);
      if (endp == p || t->step_count == 0)
	error (_("while-stepping step count `%s' is malformed."), line);
      p = endp;
    }

  else if (cmd_cfunc_eq (c, end_actions_pseudocommand))
    ;

  else
    error (_("`%s' is not a supported tracepoint action."), line);
}

enum {
  memrange_absolute = -1
};

/* MEMRANGE functions: */

static int memrange_cmp (const void *, const void *);

/* Compare memranges for qsort.  */
static int
memrange_cmp (const void *va, const void *vb)
{
  const struct memrange *a = va, *b = vb;

  if (a->type < b->type)
    return -1;
  if (a->type > b->type)
    return 1;
  if (a->type == memrange_absolute)
    {
      if ((bfd_vma) a->start < (bfd_vma) b->start)
	return -1;
      if ((bfd_vma) a->start > (bfd_vma) b->start)
	return 1;
    }
  else
    {
      if (a->start < b->start)
	return -1;
      if (a->start > b->start)
	return 1;
    }
  return 0;
}

/* Sort the memrange list using qsort, and merge adjacent memranges.  */
static void
memrange_sortmerge (struct collection_list *memranges)
{
  int a, b;

  qsort (memranges->list, memranges->next_memrange,
	 sizeof (struct memrange), memrange_cmp);
  if (memranges->next_memrange > 0)
    {
      for (a = 0, b = 1; b < memranges->next_memrange; b++)
	{
	  /* If memrange b overlaps or is adjacent to memrange a,
	     merge them.  */
	  if (memranges->list[a].type == memranges->list[b].type
	      && memranges->list[b].start <= memranges->list[a].end)
	    {
	      if (memranges->list[b].end > memranges->list[a].end)
		memranges->list[a].end = memranges->list[b].end;
	      continue;		/* next b, same a */
	    }
	  a++;			/* next a */
	  if (a != b)
	    memcpy (&memranges->list[a], &memranges->list[b],
		    sizeof (struct memrange));
	}
      memranges->next_memrange = a + 1;
    }
}

/* Add a register to a collection list.  */
static void
add_register (struct collection_list *collection, unsigned int regno)
{
  if (info_verbose)
    printf_filtered ("collect register %d\n", regno);
  if (regno >= (8 * sizeof (collection->regs_mask)))
    error (_("Internal: register number %d too large for tracepoint"),
	   regno);
  collection->regs_mask[regno / 8] |= 1 << (regno % 8);
}

/* Add a memrange to a collection list.  */
static void
add_memrange (struct collection_list *memranges, 
	      int type, bfd_signed_vma base,
	      unsigned long len)
{
  if (info_verbose)
    {
      printf_filtered ("(%d,", type);
      printf_vma (base);
      printf_filtered (",%ld)\n", len);
    }

  /* type: memrange_absolute == memory, other n == basereg */
  memranges->list[memranges->next_memrange].type = type;
  /* base: addr if memory, offset if reg relative.  */
  memranges->list[memranges->next_memrange].start = base;
  /* len: we actually save end (base + len) for convenience */
  memranges->list[memranges->next_memrange].end = base + len;
  memranges->next_memrange++;
  if (memranges->next_memrange >= memranges->listsize)
    {
      memranges->listsize *= 2;
      memranges->list = xrealloc (memranges->list,
				  memranges->listsize);
    }

  if (type != memrange_absolute)    /* Better collect the base register!  */
    add_register (memranges, type);
}

/* Add a symbol to a collection list.  */
static void
collect_symbol (struct collection_list *collect, 
		struct symbol *sym,
		struct gdbarch *gdbarch,
		long frame_regno, long frame_offset,
		CORE_ADDR scope,
		int trace_string)
{
  unsigned long len;
  unsigned int reg;
  bfd_signed_vma offset;
  int treat_as_expr = 0;

  len = TYPE_LENGTH (check_typedef (SYMBOL_TYPE (sym)));
  switch (SYMBOL_CLASS (sym))
    {
    default:
      printf_filtered ("%s: don't know symbol class %d\n",
		       SYMBOL_PRINT_NAME (sym),
		       SYMBOL_CLASS (sym));
      break;
    case LOC_CONST:
      printf_filtered ("constant %s (value %s) will not be collected.\n",
		       SYMBOL_PRINT_NAME (sym), plongest (SYMBOL_VALUE (sym)));
      break;
    case LOC_STATIC:
      offset = SYMBOL_VALUE_ADDRESS (sym);
      if (info_verbose)
	{
	  char tmp[40];

	  sprintf_vma (tmp, offset);
	  printf_filtered ("LOC_STATIC %s: collect %ld bytes at %s.\n",
			   SYMBOL_PRINT_NAME (sym), len,
			   tmp /* address */);
	}
      /* A struct may be a C++ class with static fields, go to general
	 expression handling.  */
      if (TYPE_CODE (SYMBOL_TYPE (sym)) == TYPE_CODE_STRUCT)
	treat_as_expr = 1;
      else
	add_memrange (collect, memrange_absolute, offset, len);
      break;
    case LOC_REGISTER:
      reg = SYMBOL_REGISTER_OPS (sym)->register_number (sym, gdbarch);
      if (info_verbose)
	printf_filtered ("LOC_REG[parm] %s: ", 
			 SYMBOL_PRINT_NAME (sym));
      add_register (collect, reg);
      /* Check for doubles stored in two registers.  */
      /* FIXME: how about larger types stored in 3 or more regs?  */
      if (TYPE_CODE (SYMBOL_TYPE (sym)) == TYPE_CODE_FLT &&
	  len > register_size (gdbarch, reg))
	add_register (collect, reg + 1);
      break;
    case LOC_REF_ARG:
      printf_filtered ("Sorry, don't know how to do LOC_REF_ARG yet.\n");
      printf_filtered ("       (will not collect %s)\n",
		       SYMBOL_PRINT_NAME (sym));
      break;
    case LOC_ARG:
      reg = frame_regno;
      offset = frame_offset + SYMBOL_VALUE (sym);
      if (info_verbose)
	{
	  printf_filtered ("LOC_LOCAL %s: Collect %ld bytes at offset ",
			   SYMBOL_PRINT_NAME (sym), len);
	  printf_vma (offset);
	  printf_filtered (" from frame ptr reg %d\n", reg);
	}
      add_memrange (collect, reg, offset, len);
      break;
    case LOC_REGPARM_ADDR:
      reg = SYMBOL_VALUE (sym);
      offset = 0;
      if (info_verbose)
	{
	  printf_filtered ("LOC_REGPARM_ADDR %s: Collect %ld bytes at offset ",
			   SYMBOL_PRINT_NAME (sym), len);
	  printf_vma (offset);
	  printf_filtered (" from reg %d\n", reg);
	}
      add_memrange (collect, reg, offset, len);
      break;
    case LOC_LOCAL:
      reg = frame_regno;
      offset = frame_offset + SYMBOL_VALUE (sym);
      if (info_verbose)
	{
	  printf_filtered ("LOC_LOCAL %s: Collect %ld bytes at offset ",
			   SYMBOL_PRINT_NAME (sym), len);
	  printf_vma (offset);
	  printf_filtered (" from frame ptr reg %d\n", reg);
	}
      add_memrange (collect, reg, offset, len);
      break;

    case LOC_UNRESOLVED:
      treat_as_expr = 1;
      break;

    case LOC_OPTIMIZED_OUT:
      printf_filtered ("%s has been optimized out of existence.\n",
		       SYMBOL_PRINT_NAME (sym));
      break;

    case LOC_COMPUTED:
      treat_as_expr = 1;
      break;
    }

  /* Expressions are the most general case.  */
  if (treat_as_expr)
    {
      struct agent_expr *aexpr;
      struct cleanup *old_chain1 = NULL;

      aexpr = gen_trace_for_var (scope, gdbarch, sym, trace_string);

      /* It can happen that the symbol is recorded as a computed
	 location, but it's been optimized away and doesn't actually
	 have a location expression.  */
      if (!aexpr)
	{
	  printf_filtered ("%s has been optimized out of existence.\n",
			   SYMBOL_PRINT_NAME (sym));
	  return;
	}

      old_chain1 = make_cleanup_free_agent_expr (aexpr);

      ax_reqs (aexpr);

      report_agent_reqs_errors (aexpr);

      discard_cleanups (old_chain1);
      add_aexpr (collect, aexpr);

      /* Take care of the registers.  */
      if (aexpr->reg_mask_len > 0)
	{
	  int ndx1, ndx2;

	  for (ndx1 = 0; ndx1 < aexpr->reg_mask_len; ndx1++)
	    {
	      QUIT;	/* Allow user to bail out with ^C.  */
	      if (aexpr->reg_mask[ndx1] != 0)
		{
		  /* Assume chars have 8 bits.  */
		  for (ndx2 = 0; ndx2 < 8; ndx2++)
		    if (aexpr->reg_mask[ndx1] & (1 << ndx2))
		      /* It's used -- record it.  */
		      add_register (collect, ndx1 * 8 + ndx2);
		}
	    }
	}
    }
}

/* Data to be passed around in the calls to the locals and args
   iterators.  */

struct add_local_symbols_data
{
  struct collection_list *collect;
  struct gdbarch *gdbarch;
  CORE_ADDR pc;
  long frame_regno;
  long frame_offset;
  int count;
  int trace_string;
};

/* The callback for the locals and args iterators.  */

static void
do_collect_symbol (const char *print_name,
		   struct symbol *sym,
		   void *cb_data)
{
  struct add_local_symbols_data *p = cb_data;

  collect_symbol (p->collect, sym, p->gdbarch, p->frame_regno,
		  p->frame_offset, p->pc, p->trace_string);
  p->count++;

  VEC_safe_push (char_ptr, p->collect->wholly_collected,
		 xstrdup (print_name));
}

/* Add all locals (or args) symbols to collection list.  */
static void
add_local_symbols (struct collection_list *collect,
		   struct gdbarch *gdbarch, CORE_ADDR pc,
		   long frame_regno, long frame_offset, int type,
		   int trace_string)
{
  const struct block *block;
  struct add_local_symbols_data cb_data;

  cb_data.collect = collect;
  cb_data.gdbarch = gdbarch;
  cb_data.pc = pc;
  cb_data.frame_regno = frame_regno;
  cb_data.frame_offset = frame_offset;
  cb_data.count = 0;
  cb_data.trace_string = trace_string;

  if (type == 'L')
    {
      block = block_for_pc (pc);
      if (block == NULL)
	{
	  warning (_("Can't collect locals; "
		     "no symbol table info available.\n"));
	  return;
	}

      iterate_over_block_local_vars (block, do_collect_symbol, &cb_data);
      if (cb_data.count == 0)
	warning (_("No locals found in scope."));
    }
  else
    {
      pc = get_pc_function_start (pc);
      block = block_for_pc (pc);
      if (block == NULL)
	{
	  warning (_("Can't collect args; no symbol table info available."));
	  return;
	}

      iterate_over_block_arg_vars (block, do_collect_symbol, &cb_data);
      if (cb_data.count == 0)
	warning (_("No args found in scope."));
    }
}

static void
add_static_trace_data (struct collection_list *collection)
{
  if (info_verbose)
    printf_filtered ("collect static trace data\n");
  collection->strace_data = 1;
}

/* worker function */
static void
clear_collection_list (struct collection_list *list)
{
  int ndx;

  list->next_memrange = 0;
  for (ndx = 0; ndx < list->next_aexpr_elt; ndx++)
    {
      free_agent_expr (list->aexpr_list[ndx]);
      list->aexpr_list[ndx] = NULL;
    }
  list->next_aexpr_elt = 0;
  memset (list->regs_mask, 0, sizeof (list->regs_mask));
  list->strace_data = 0;

  xfree (list->aexpr_list);
  xfree (list->list);

  VEC_free (char_ptr, list->wholly_collected);
  VEC_free (char_ptr, list->computed);
}

/* A cleanup wrapper for function clear_collection_list.  */

static void
do_clear_collection_list (void *list)
{
  struct collection_list *l = list;

  clear_collection_list (l);
}

/* Initialize collection_list CLIST.  */

static void
init_collection_list (struct collection_list *clist)
{
  memset (clist, 0, sizeof *clist);

  clist->listsize = 128;
  clist->list = xcalloc (clist->listsize,
			 sizeof (struct memrange));

  clist->aexpr_listsize = 128;
  clist->aexpr_list = xcalloc (clist->aexpr_listsize,
			       sizeof (struct agent_expr *));
}

/* Reduce a collection list to string form (for gdb protocol).  */
static char **
stringify_collection_list (struct collection_list *list)
{
  char temp_buf[2048];
  char tmp2[40];
  int count;
  int ndx = 0;
  char *(*str_list)[];
  char *end;
  long i;

  count = 1 + 1 + list->next_memrange + list->next_aexpr_elt + 1;
  str_list = (char *(*)[]) xmalloc (count * sizeof (char *));

  if (list->strace_data)
    {
      if (info_verbose)
	printf_filtered ("\nCollecting static trace data\n");
      end = temp_buf;
      *end++ = 'L';
      (*str_list)[ndx] = savestring (temp_buf, end - temp_buf);
      ndx++;
    }

  for (i = sizeof (list->regs_mask) - 1; i > 0; i--)
    if (list->regs_mask[i] != 0)    /* Skip leading zeroes in regs_mask.  */
      break;
  if (list->regs_mask[i] != 0)	/* Prepare to send regs_mask to the stub.  */
    {
      if (info_verbose)
	printf_filtered ("\nCollecting registers (mask): 0x");
      end = temp_buf;
      *end++ = 'R';
      for (; i >= 0; i--)
	{
	  QUIT;			/* Allow user to bail out with ^C.  */
	  if (info_verbose)
	    printf_filtered ("%02X", list->regs_mask[i]);
	  sprintf (end, "%02X", list->regs_mask[i]);
	  end += 2;
	}
      (*str_list)[ndx] = xstrdup (temp_buf);
      ndx++;
    }
  if (info_verbose)
    printf_filtered ("\n");
  if (list->next_memrange > 0 && info_verbose)
    printf_filtered ("Collecting memranges: \n");
  for (i = 0, count = 0, end = temp_buf; i < list->next_memrange; i++)
    {
      QUIT;			/* Allow user to bail out with ^C.  */
      sprintf_vma (tmp2, list->list[i].start);
      if (info_verbose)
	{
	  printf_filtered ("(%d, %s, %ld)\n", 
			   list->list[i].type, 
			   tmp2, 
			   (long) (list->list[i].end - list->list[i].start));
	}
      if (count + 27 > MAX_AGENT_EXPR_LEN)
	{
	  (*str_list)[ndx] = savestring (temp_buf, count);
	  ndx++;
	  count = 0;
	  end = temp_buf;
	}

      {
        bfd_signed_vma length = list->list[i].end - list->list[i].start;

        /* The "%X" conversion specifier expects an unsigned argument,
           so passing -1 (memrange_absolute) to it directly gives you
           "FFFFFFFF" (or more, depending on sizeof (unsigned)).
           Special-case it.  */
        if (list->list[i].type == memrange_absolute)
          sprintf (end, "M-1,%s,%lX", tmp2, (long) length);
        else
          sprintf (end, "M%X,%s,%lX", list->list[i].type, tmp2, (long) length);
      }

      count += strlen (end);
      end = temp_buf + count;
    }

  for (i = 0; i < list->next_aexpr_elt; i++)
    {
      QUIT;			/* Allow user to bail out with ^C.  */
      if ((count + 10 + 2 * list->aexpr_list[i]->len) > MAX_AGENT_EXPR_LEN)
	{
	  (*str_list)[ndx] = savestring (temp_buf, count);
	  ndx++;
	  count = 0;
	  end = temp_buf;
	}
      sprintf (end, "X%08X,", list->aexpr_list[i]->len);
      end += 10;		/* 'X' + 8 hex digits + ',' */
      count += 10;

      end = mem2hex (list->aexpr_list[i]->buf, 
		     end, list->aexpr_list[i]->len);
      count += 2 * list->aexpr_list[i]->len;
    }

  if (count != 0)
    {
      (*str_list)[ndx] = savestring (temp_buf, count);
      ndx++;
      count = 0;
      end = temp_buf;
    }
  (*str_list)[ndx] = NULL;

  if (ndx == 0)
    {
      xfree (str_list);
      return NULL;
    }
  else
    return *str_list;
}

/* Add the printed expression EXP to *LIST.  */

static void
append_exp (struct expression *exp, VEC(char_ptr) **list)
{
  struct ui_file *tmp_stream = mem_fileopen ();
  char *text;

  print_expression (exp, tmp_stream);

  text = ui_file_xstrdup (tmp_stream, NULL);

  VEC_safe_push (char_ptr, *list, text);
  ui_file_delete (tmp_stream);
}

static void
encode_actions_1 (struct command_line *action,
		  struct bp_location *tloc,
		  int frame_reg,
		  LONGEST frame_offset,
		  struct collection_list *collect,
		  struct collection_list *stepping_list)
{
  const char *action_exp;
  struct expression *exp = NULL;
  int i;
  struct value *tempval;
  struct cmd_list_element *cmd;
  struct agent_expr *aexpr;

  for (; action; action = action->next)
    {
      QUIT;			/* Allow user to bail out with ^C.  */
      action_exp = action->line;
      action_exp = skip_spaces_const (action_exp);

      cmd = lookup_cmd (&action_exp, cmdlist, "", -1, 1);
      if (cmd == 0)
	error (_("Bad action list item: %s"), action_exp);

      if (cmd_cfunc_eq (cmd, collect_pseudocommand))
	{
	  int trace_string = 0;

	  if (*action_exp == '/')
	    action_exp = decode_agent_options (action_exp, &trace_string);

	  do
	    {			/* Repeat over a comma-separated list.  */
	      QUIT;		/* Allow user to bail out with ^C.  */
	      action_exp = skip_spaces_const (action_exp);

	      if (0 == strncasecmp ("$reg", action_exp, 4))
		{
		  for (i = 0; i < gdbarch_num_regs (tloc->gdbarch); i++)
		    add_register (collect, i);
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else if (0 == strncasecmp ("$arg", action_exp, 4))
		{
		  add_local_symbols (collect,
				     tloc->gdbarch,
				     tloc->address,
				     frame_reg,
				     frame_offset,
				     'A',
				     trace_string);
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else if (0 == strncasecmp ("$loc", action_exp, 4))
		{
		  add_local_symbols (collect,
				     tloc->gdbarch,
				     tloc->address,
				     frame_reg,
				     frame_offset,
				     'L',
				     trace_string);
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else if (0 == strncasecmp ("$_ret", action_exp, 5))
		{
		  struct cleanup *old_chain1 = NULL;

		  aexpr = gen_trace_for_return_address (tloc->address,
							tloc->gdbarch,
							trace_string);

		  old_chain1 = make_cleanup_free_agent_expr (aexpr);

		  ax_reqs (aexpr);
		  report_agent_reqs_errors (aexpr);

		  discard_cleanups (old_chain1);
		  add_aexpr (collect, aexpr);

		  /* take care of the registers */
		  if (aexpr->reg_mask_len > 0)
		    {
		      int ndx1, ndx2;

		      for (ndx1 = 0; ndx1 < aexpr->reg_mask_len; ndx1++)
			{
			  QUIT;	/* allow user to bail out with ^C */
			  if (aexpr->reg_mask[ndx1] != 0)
			    {
			      /* assume chars have 8 bits */
			      for (ndx2 = 0; ndx2 < 8; ndx2++)
				if (aexpr->reg_mask[ndx1] & (1 << ndx2))
				  /* it's used -- record it */
				  add_register (collect, 
						ndx1 * 8 + ndx2);
			    }
			}
		    }

		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else if (0 == strncasecmp ("$_sdata", action_exp, 7))
		{
		  add_static_trace_data (collect);
		  action_exp = strchr (action_exp, ',');	/* more? */
		}
	      else
		{
		  unsigned long addr;
		  struct cleanup *old_chain = NULL;
		  struct cleanup *old_chain1 = NULL;

		  exp = parse_exp_1 (&action_exp, tloc->address,
				     block_for_pc (tloc->address), 1);
		  old_chain = make_cleanup (free_current_contents, &exp);

		  switch (exp->elts[0].opcode)
		    {
		    case OP_REGISTER:
		      {
			const char *name = &exp->elts[2].string;

			i = user_reg_map_name_to_regnum (tloc->gdbarch,
							 name, strlen (name));
			if (i == -1)
			  internal_error (__FILE__, __LINE__,
					  _("Register $%s not available"),
					  name);
			if (info_verbose)
			  printf_filtered ("OP_REGISTER: ");
			add_register (collect, i);
			break;
		      }

		    case UNOP_MEMVAL:
		      /* Safe because we know it's a simple expression.  */
		      tempval = evaluate_expression (exp);
		      addr = value_address (tempval);
		      /* Initialize the TYPE_LENGTH if it is a typedef.  */
		      check_typedef (exp->elts[1].type);
		      add_memrange (collect, memrange_absolute, addr,
				    TYPE_LENGTH (exp->elts[1].type));
		      append_exp (exp, &collect->computed);
		      break;

		    case OP_VAR_VALUE:
		      {
			struct symbol *sym = exp->elts[2].symbol;
			char_ptr name = (char_ptr) SYMBOL_NATURAL_NAME (sym);

			collect_symbol (collect,
					exp->elts[2].symbol,
					tloc->gdbarch,
					frame_reg,
					frame_offset,
					tloc->address,
					trace_string);
			VEC_safe_push (char_ptr,
				       collect->wholly_collected,
				       name);
		      }
		      break;

		    default:	/* Full-fledged expression.  */
		      aexpr = gen_trace_for_expr (tloc->address, exp,
						  trace_string);

		      old_chain1 = make_cleanup_free_agent_expr (aexpr);

		      ax_reqs (aexpr);

		      report_agent_reqs_errors (aexpr);

		      discard_cleanups (old_chain1);
		      add_aexpr (collect, aexpr);

		      /* Take care of the registers.  */
		      if (aexpr->reg_mask_len > 0)
			{
			  int ndx1;
			  int ndx2;

			  for (ndx1 = 0; ndx1 < aexpr->reg_mask_len; ndx1++)
			    {
			      QUIT;	/* Allow user to bail out with ^C.  */
			      if (aexpr->reg_mask[ndx1] != 0)
				{
				  /* Assume chars have 8 bits.  */
				  for (ndx2 = 0; ndx2 < 8; ndx2++)
				    if (aexpr->reg_mask[ndx1] & (1 << ndx2))
				      /* It's used -- record it.  */
				      add_register (collect, 
						    ndx1 * 8 + ndx2);
				}
			    }
			}

		      append_exp (exp, &collect->computed);
		      break;
		    }		/* switch */
		  do_cleanups (old_chain);
		}		/* do */
	    }
	  while (action_exp && *action_exp++ == ',');
	}			/* if */
      else if (cmd_cfunc_eq (cmd, teval_pseudocommand))
	{
	  do
	    {			/* Repeat over a comma-separated list.  */
	      QUIT;		/* Allow user to bail out with ^C.  */
	      action_exp = skip_spaces_const (action_exp);

		{
		  struct cleanup *old_chain = NULL;
		  struct cleanup *old_chain1 = NULL;

		  exp = parse_exp_1 (&action_exp, tloc->address,
				     block_for_pc (tloc->address), 1);
		  old_chain = make_cleanup (free_current_contents, &exp);

		  aexpr = gen_eval_for_expr (tloc->address, exp);
		  old_chain1 = make_cleanup_free_agent_expr (aexpr);

		  ax_reqs (aexpr);
		  report_agent_reqs_errors (aexpr);

		  discard_cleanups (old_chain1);
		  /* Even though we're not officially collecting, add
		     to the collect list anyway.  */
		  add_aexpr (collect, aexpr);

		  do_cleanups (old_chain);
		}		/* do */
	    }
	  while (action_exp && *action_exp++ == ',');
	}			/* if */
      else if (cmd_cfunc_eq (cmd, while_stepping_pseudocommand))
	{
	  /* We check against nested while-stepping when setting
	     breakpoint action, so no way to run into nested
	     here.  */
	  gdb_assert (stepping_list);

	  encode_actions_1 (action->body_list[0], tloc, frame_reg,
			    frame_offset, stepping_list, NULL);
	}
      else
	error (_("Invalid tracepoint command '%s'"), action->line);
    }				/* for */
}

/* Encode actions of tracepoint TLOC->owner and fill TRACEPOINT_LIST
   and STEPPING_LIST.  Return a cleanup pointer to clean up both
   TRACEPOINT_LIST and STEPPING_LIST.  */

struct cleanup *
encode_actions_and_make_cleanup (struct bp_location *tloc,
				 struct collection_list *tracepoint_list,
				 struct collection_list *stepping_list)
{
  char *default_collect_line = NULL;
  struct command_line *actions;
  struct command_line *default_collect_action = NULL;
  int frame_reg;
  LONGEST frame_offset;
  struct cleanup *back_to, *return_chain;

  return_chain = make_cleanup (null_cleanup, NULL);
  init_collection_list (tracepoint_list);
  init_collection_list (stepping_list);

  make_cleanup (do_clear_collection_list, tracepoint_list);
  make_cleanup (do_clear_collection_list, stepping_list);

  back_to = make_cleanup (null_cleanup, NULL);
  gdbarch_virtual_frame_pointer (tloc->gdbarch,
				 tloc->address, &frame_reg, &frame_offset);

  actions = all_tracepoint_actions_and_cleanup (tloc->owner);

  encode_actions_1 (actions, tloc, frame_reg, frame_offset,
		    tracepoint_list, stepping_list);

  memrange_sortmerge (tracepoint_list);
  memrange_sortmerge (stepping_list);

  do_cleanups (back_to);
  return return_chain;
}

/* Render all actions into gdb protocol.  */

void
encode_actions_rsp (struct bp_location *tloc, char ***tdp_actions,
		    char ***stepping_actions)
{
  struct collection_list tracepoint_list, stepping_list;
  struct cleanup *cleanup;

  *tdp_actions = NULL;
  *stepping_actions = NULL;

  cleanup = encode_actions_and_make_cleanup (tloc, &tracepoint_list,
					     &stepping_list);

  *tdp_actions = stringify_collection_list (&tracepoint_list);
  *stepping_actions = stringify_collection_list (&stepping_list);

  do_cleanups (cleanup);
}

static void
add_aexpr (struct collection_list *collect, struct agent_expr *aexpr)
{
  if (collect->next_aexpr_elt >= collect->aexpr_listsize)
    {
      collect->aexpr_list =
	xrealloc (collect->aexpr_list,
		  2 * collect->aexpr_listsize * sizeof (struct agent_expr *));
      collect->aexpr_listsize *= 2;
    }
  collect->aexpr_list[collect->next_aexpr_elt] = aexpr;
  collect->next_aexpr_elt++;
}

static void
process_tracepoint_on_disconnect (void)
{
  VEC(breakpoint_p) *tp_vec = NULL;
  int ix;
  struct breakpoint *b;
  int has_pending_p = 0;

  /* Check whether we still have pending tracepoint.  If we have, warn the
     user that pending tracepoint will no longer work.  */
  tp_vec = all_tracepoints ();
  for (ix = 0; VEC_iterate (breakpoint_p, tp_vec, ix, b); ix++)
    {
      if (b->loc == NULL)
	{
	  has_pending_p = 1;
	  break;
	}
      else
	{
	  struct bp_location *loc1;

	  for (loc1 = b->loc; loc1; loc1 = loc1->next)
	    {
	      if (loc1->shlib_disabled)
		{
		  has_pending_p = 1;
		  break;
		}
	    }

	  if (has_pending_p)
	    break;
	}
    }
  VEC_free (breakpoint_p, tp_vec);

  if (has_pending_p)
    warning (_("Pending tracepoints will not be resolved while"
	       " GDB is disconnected\n"));
}

/* Reset local state of tracing.  */

void
trace_reset_local_state (void)
{
  set_traceframe_num (-1);
  set_tracepoint_num (-1);
  set_traceframe_context (NULL);
  clear_traceframe_info ();
}

void
start_tracing (char *notes)
{
  VEC(breakpoint_p) *tp_vec = NULL;
  int ix;
  struct breakpoint *b;
  struct trace_state_variable *tsv;
  int any_enabled = 0, num_to_download = 0;
  int ret;

  tp_vec = all_tracepoints ();

  /* No point in tracing without any tracepoints...  */
  if (VEC_length (breakpoint_p, tp_vec) == 0)
    {
      VEC_free (breakpoint_p, tp_vec);
      error (_("No tracepoints defined, not starting trace"));
    }

  for (ix = 0; VEC_iterate (breakpoint_p, tp_vec, ix, b); ix++)
    {
      struct tracepoint *t = (struct tracepoint *) b;
      struct bp_location *loc;

      if (b->enable_state == bp_enabled)
	any_enabled = 1;

      if ((b->type == bp_fast_tracepoint
	   ? may_insert_fast_tracepoints
	   : may_insert_tracepoints))
	++num_to_download;
      else
	warning (_("May not insert %stracepoints, skipping tracepoint %d"),
		 (b->type == bp_fast_tracepoint ? "fast " : ""), b->number);
    }

  if (!any_enabled)
    {
      if (target_supports_enable_disable_tracepoint ())
	warning (_("No tracepoints enabled"));
      else
	{
	  /* No point in tracing with only disabled tracepoints that
	     cannot be re-enabled.  */
	  VEC_free (breakpoint_p, tp_vec);
	  error (_("No tracepoints enabled, not starting trace"));
	}
    }

  if (num_to_download <= 0)
    {
      VEC_free (breakpoint_p, tp_vec);
      error (_("No tracepoints that may be downloaded, not starting trace"));
    }

  target_trace_init ();

  for (ix = 0; VEC_iterate (breakpoint_p, tp_vec, ix, b); ix++)
    {
      struct tracepoint *t = (struct tracepoint *) b;
      struct bp_location *loc;
      int bp_location_downloaded = 0;

      /* Clear `inserted' flag.  */
      for (loc = b->loc; loc; loc = loc->next)
	loc->inserted = 0;

      if ((b->type == bp_fast_tracepoint
	   ? !may_insert_fast_tracepoints
	   : !may_insert_tracepoints))
	continue;

      t->number_on_target = 0;

      for (loc = b->loc; loc; loc = loc->next)
	{
	  /* Since tracepoint locations are never duplicated, `inserted'
	     flag should be zero.  */
	  gdb_assert (!loc->inserted);

	  target_download_tracepoint (loc);

	  loc->inserted = 1;
	  bp_location_downloaded = 1;
	}

      t->number_on_target = b->number;

      for (loc = b->loc; loc; loc = loc->next)
	if (loc->probe.probe != NULL
	    && loc->probe.probe->pops->set_semaphore != NULL)
	  loc->probe.probe->pops->set_semaphore (loc->probe.probe,
						 loc->probe.objfile,
						 loc->gdbarch);

      if (bp_location_downloaded)
	observer_notify_breakpoint_modified (b);
    }
  VEC_free (breakpoint_p, tp_vec);

  /* Send down all the trace state variables too.  */
  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    {
      target_download_trace_state_variable (tsv);
    }
  
  /* Tell target to treat text-like sections as transparent.  */
  target_trace_set_readonly_regions ();
  /* Set some mode flags.  */
  target_set_disconnected_tracing (disconnected_tracing);
  target_set_circular_trace_buffer (circular_trace_buffer);
  target_set_trace_buffer_size (trace_buffer_size);

  if (!notes)
    notes = trace_notes;
  ret = target_set_trace_notes (trace_user, notes, NULL);

  if (!ret && (trace_user || notes))
    warning (_("Target does not support trace user/notes, info ignored"));

  /* Now insert traps and begin collecting data.  */
  target_trace_start ();

  /* Reset our local state.  */
  trace_reset_local_state ();
  current_trace_status()->running = 1;
}

/* The tstart command requests the target to start a new trace run.
   The command passes any arguments it has to the target verbatim, as
   an optional "trace note".  This is useful as for instance a warning
   to other users if the trace runs disconnected, and you don't want
   anybody else messing with the target.  */

static void
trace_start_command (char *args, int from_tty)
{
  dont_repeat ();	/* Like "run", dangerous to repeat accidentally.  */

  if (current_trace_status ()->running)
    {
      if (from_tty
	  && !query (_("A trace is running already.  Start a new run? ")))
	error (_("New trace run not started."));
    }

  start_tracing (args);
}

/* The tstop command stops the tracing run.  The command passes any
   supplied arguments to the target verbatim as a "stop note"; if the
   target supports trace notes, then it will be reported back as part
   of the trace run's status.  */

static void
trace_stop_command (char *args, int from_tty)
{
  if (!current_trace_status ()->running)
    error (_("Trace is not running."));

  stop_tracing (args);
}

void
stop_tracing (char *note)
{
  int ret;
  VEC(breakpoint_p) *tp_vec = NULL;
  int ix;
  struct breakpoint *t;

  target_trace_stop ();

  tp_vec = all_tracepoints ();
  for (ix = 0; VEC_iterate (breakpoint_p, tp_vec, ix, t); ix++)
    {
      struct bp_location *loc;

      if ((t->type == bp_fast_tracepoint
	   ? !may_insert_fast_tracepoints
	   : !may_insert_tracepoints))
	continue;

      for (loc = t->loc; loc; loc = loc->next)
	{
	  /* GDB can be totally absent in some disconnected trace scenarios,
	     but we don't really care if this semaphore goes out of sync.
	     That's why we are decrementing it here, but not taking care
	     in other places.  */
	  if (loc->probe.probe != NULL
	      && loc->probe.probe->pops->clear_semaphore != NULL)
	    loc->probe.probe->pops->clear_semaphore (loc->probe.probe,
						     loc->probe.objfile,
						     loc->gdbarch);
	}
    }

  VEC_free (breakpoint_p, tp_vec);

  if (!note)
    note = trace_stop_notes;
  ret = target_set_trace_notes (NULL, NULL, note);

  if (!ret && note)
    warning (_("Target does not support trace notes, note ignored"));

  /* Should change in response to reply?  */
  current_trace_status ()->running = 0;
}

/* tstatus command */
static void
trace_status_command (char *args, int from_tty)
{
  struct trace_status *ts = current_trace_status ();
  int status, ix;
  VEC(breakpoint_p) *tp_vec = NULL;
  struct breakpoint *t;
  
  status = target_get_trace_status (ts);

  if (status == -1)
    {
      if (ts->filename != NULL)
	printf_filtered (_("Using a trace file.\n"));
      else
	{
	  printf_filtered (_("Trace can not be run on this target.\n"));
	  return;
	}
    }

  if (!ts->running_known)
    {
      printf_filtered (_("Run/stop status is unknown.\n"));
    }
  else if (ts->running)
    {
      printf_filtered (_("Trace is running on the target.\n"));
    }
  else
    {
      switch (ts->stop_reason)
	{
	case trace_never_run:
	  printf_filtered (_("No trace has been run on the target.\n"));
	  break;
	case tstop_command:
	  if (ts->stop_desc)
	    printf_filtered (_("Trace stopped by a tstop command (%s).\n"),
			     ts->stop_desc);
	  else
	    printf_filtered (_("Trace stopped by a tstop command.\n"));
	  break;
	case trace_buffer_full:
	  printf_filtered (_("Trace stopped because the buffer was full.\n"));
	  break;
	case trace_disconnected:
	  printf_filtered (_("Trace stopped because of disconnection.\n"));
	  break;
	case tracepoint_passcount:
	  printf_filtered (_("Trace stopped by tracepoint %d.\n"),
			   ts->stopping_tracepoint);
	  break;
	case tracepoint_error:
	  if (ts->stopping_tracepoint)
	    printf_filtered (_("Trace stopped by an "
			       "error (%s, tracepoint %d).\n"),
			     ts->stop_desc, ts->stopping_tracepoint);
	  else
	    printf_filtered (_("Trace stopped by an error (%s).\n"),
			     ts->stop_desc);
	  break;
	case trace_stop_reason_unknown:
	  printf_filtered (_("Trace stopped for an unknown reason.\n"));
	  break;
	default:
	  printf_filtered (_("Trace stopped for some other reason (%d).\n"),
			   ts->stop_reason);
	  break;
	}
    }

  if (ts->traceframes_created >= 0
      && ts->traceframe_count != ts->traceframes_created)
    {
      printf_filtered (_("Buffer contains %d trace "
			 "frames (of %d created total).\n"),
		       ts->traceframe_count, ts->traceframes_created);
    }
  else if (ts->traceframe_count >= 0)
    {
      printf_filtered (_("Collected %d trace frames.\n"),
		       ts->traceframe_count);
    }

  if (ts->buffer_free >= 0)
    {
      if (ts->buffer_size >= 0)
	{
	  printf_filtered (_("Trace buffer has %d bytes of %d bytes free"),
			   ts->buffer_free, ts->buffer_size);
	  if (ts->buffer_size > 0)
	    printf_filtered (_(" (%d%% full)"),
			     ((int) ((((long long) (ts->buffer_size
						    - ts->buffer_free)) * 100)
				     / ts->buffer_size)));
	  printf_filtered (_(".\n"));
	}
      else
	printf_filtered (_("Trace buffer has %d bytes free.\n"),
			 ts->buffer_free);
    }

  if (ts->disconnected_tracing)
    printf_filtered (_("Trace will continue if GDB disconnects.\n"));
  else
    printf_filtered (_("Trace will stop if GDB disconnects.\n"));

  if (ts->circular_buffer)
    printf_filtered (_("Trace buffer is circular.\n"));

  if (ts->user_name && strlen (ts->user_name) > 0)
    printf_filtered (_("Trace user is %s.\n"), ts->user_name);

  if (ts->notes && strlen (ts->notes) > 0)
    printf_filtered (_("Trace notes: %s.\n"), ts->notes);

  /* Now report on what we're doing with tfind.  */
  if (traceframe_number >= 0)
    printf_filtered (_("Looking at trace frame %d, tracepoint %d.\n"),
		     traceframe_number, tracepoint_number);
  else
    printf_filtered (_("Not looking at any trace frame.\n"));

  /* Report start/stop times if supplied.  */
  if (ts->start_time)
    {
      if (ts->stop_time)
	{
	  LONGEST run_time = ts->stop_time - ts->start_time;

	  /* Reporting a run time is more readable than two long numbers.  */
	  printf_filtered (_("Trace started at %ld.%06ld secs, stopped %ld.%06ld secs later.\n"),
			   (long int) (ts->start_time / 1000000),
			   (long int) (ts->start_time % 1000000),
			   (long int) (run_time / 1000000),
			   (long int) (run_time % 1000000));
	}
      else
	printf_filtered (_("Trace started at %ld.%06ld secs.\n"),
			 (long int) (ts->start_time / 1000000),
			 (long int) (ts->start_time % 1000000));
    }
  else if (ts->stop_time)
    printf_filtered (_("Trace stopped at %ld.%06ld secs.\n"),
		     (long int) (ts->stop_time / 1000000),
		     (long int) (ts->stop_time % 1000000));

  /* Now report any per-tracepoint status available.  */
  tp_vec = all_tracepoints ();

  for (ix = 0; VEC_iterate (breakpoint_p, tp_vec, ix, t); ix++)
    target_get_tracepoint_status (t, NULL);

  VEC_free (breakpoint_p, tp_vec);
}

/* Report the trace status to uiout, in a way suitable for MI, and not
   suitable for CLI.  If ON_STOP is true, suppress a few fields that
   are not meaningful in the -trace-stop response.

   The implementation is essentially parallel to trace_status_command, but
   merging them will result in unreadable code.  */
void
trace_status_mi (int on_stop)
{
  struct ui_out *uiout = current_uiout;
  struct trace_status *ts = current_trace_status ();
  int status;

  status = target_get_trace_status (ts);

  if (status == -1 && ts->filename == NULL)
    {
      ui_out_field_string (uiout, "supported", "0");
      return;
    }

  if (ts->filename != NULL)
    ui_out_field_string (uiout, "supported", "file");
  else if (!on_stop)
    ui_out_field_string (uiout, "supported", "1");

  if (ts->filename != NULL)
    ui_out_field_string (uiout, "trace-file", ts->filename);

  gdb_assert (ts->running_known);

  if (ts->running)
    {
      ui_out_field_string (uiout, "running", "1");

      /* Unlike CLI, do not show the state of 'disconnected-tracing' variable.
	 Given that the frontend gets the status either on -trace-stop, or from
	 -trace-status after re-connection, it does not seem like this
	 information is necessary for anything.  It is not necessary for either
	 figuring the vital state of the target nor for navigation of trace
	 frames.  If the frontend wants to show the current state is some
	 configure dialog, it can request the value when such dialog is
	 invoked by the user.  */
    }
  else
    {
      char *stop_reason = NULL;
      int stopping_tracepoint = -1;

      if (!on_stop)
	ui_out_field_string (uiout, "running", "0");

      if (ts->stop_reason != trace_stop_reason_unknown)
	{
	  switch (ts->stop_reason)
	    {
	    case tstop_command:
	      stop_reason = "request";
	      break;
	    case trace_buffer_full:
	      stop_reason = "overflow";
	      break;
	    case trace_disconnected:
	      stop_reason = "disconnection";
	      break;
	    case tracepoint_passcount:
	      stop_reason = "passcount";
	      stopping_tracepoint = ts->stopping_tracepoint;
	      break;
	    case tracepoint_error:
	      stop_reason = "error";
	      stopping_tracepoint = ts->stopping_tracepoint;
	      break;
	    }
	  
	  if (stop_reason)
	    {
	      ui_out_field_string (uiout, "stop-reason", stop_reason);
	      if (stopping_tracepoint != -1)
		ui_out_field_int (uiout, "stopping-tracepoint",
				  stopping_tracepoint);
	      if (ts->stop_reason == tracepoint_error)
		ui_out_field_string (uiout, "error-description",
				     ts->stop_desc);
	    }
	}
    }

  if (ts->traceframe_count != -1)
    ui_out_field_int (uiout, "frames", ts->traceframe_count);
  if (ts->traceframes_created != -1)
    ui_out_field_int (uiout, "frames-created", ts->traceframes_created);
  if (ts->buffer_size != -1)
    ui_out_field_int (uiout, "buffer-size", ts->buffer_size);
  if (ts->buffer_free != -1)
    ui_out_field_int (uiout, "buffer-free", ts->buffer_free);

  ui_out_field_int (uiout, "disconnected",  ts->disconnected_tracing);
  ui_out_field_int (uiout, "circular",  ts->circular_buffer);

  ui_out_field_string (uiout, "user-name", ts->user_name);
  ui_out_field_string (uiout, "notes", ts->notes);

  {
    char buf[100];

    xsnprintf (buf, sizeof buf, "%ld.%06ld",
	       (long int) (ts->start_time / 1000000),
	       (long int) (ts->start_time % 1000000));
    ui_out_field_string (uiout, "start-time", buf);
    xsnprintf (buf, sizeof buf, "%ld.%06ld",
	       (long int) (ts->stop_time / 1000000),
	       (long int) (ts->stop_time % 1000000));
    ui_out_field_string (uiout, "stop-time", buf);
  }
}

/* Check if a trace run is ongoing.  If so, and FROM_TTY, query the
   user if she really wants to detach.  */

void
query_if_trace_running (int from_tty)
{
  if (!from_tty)
    return;

  /* It can happen that the target that was tracing went away on its
     own, and we didn't notice.  Get a status update, and if the
     current target doesn't even do tracing, then assume it's not
     running anymore.  */
  if (target_get_trace_status (current_trace_status ()) < 0)
    current_trace_status ()->running = 0;

  /* If running interactively, give the user the option to cancel and
     then decide what to do differently with the run.  Scripts are
     just going to disconnect and let the target deal with it,
     according to how it's been instructed previously via
     disconnected-tracing.  */
  if (current_trace_status ()->running)
    {
      process_tracepoint_on_disconnect ();

      if (current_trace_status ()->disconnected_tracing)
	{
	  if (!query (_("Trace is running and will "
			"continue after detach; detach anyway? ")))
	    error (_("Not confirmed."));
	}
      else
	{
	  if (!query (_("Trace is running but will "
			"stop on detach; detach anyway? ")))
	    error (_("Not confirmed."));
	}
    }
}

/* This function handles the details of what to do about an ongoing
   tracing run if the user has asked to detach or otherwise disconnect
   from the target.  */

void
disconnect_tracing (void)
{
  /* Also we want to be out of tfind mode, otherwise things can get
     confusing upon reconnection.  Just use these calls instead of
     full tfind_1 behavior because we're in the middle of detaching,
     and there's no point to updating current stack frame etc.  */
  trace_reset_local_state ();
}

/* Worker function for the various flavors of the tfind command.  */
void
tfind_1 (enum trace_find_type type, int num,
	 CORE_ADDR addr1, CORE_ADDR addr2,
	 int from_tty)
{
  int target_frameno = -1, target_tracept = -1;
  struct frame_id old_frame_id = null_frame_id;
  struct tracepoint *tp;
  struct ui_out *uiout = current_uiout;

  /* Only try to get the current stack frame if we have a chance of
     succeeding.  In particular, if we're trying to get a first trace
     frame while all threads are running, it's not going to succeed,
     so leave it with a default value and let the frame comparison
     below (correctly) decide to print out the source location of the
     trace frame.  */
  if (!(type == tfind_number && num == -1)
      && (has_stack_frames () || traceframe_number >= 0))
    old_frame_id = get_frame_id (get_current_frame ());

  target_frameno = target_trace_find (type, num, addr1, addr2,
				      &target_tracept);
  
  if (type == tfind_number
      && num == -1
      && target_frameno == -1)
    {
      /* We told the target to get out of tfind mode, and it did.  */
    }
  else if (target_frameno == -1)
    {
      /* A request for a non-existent trace frame has failed.
	 Our response will be different, depending on FROM_TTY:

	 If FROM_TTY is true, meaning that this command was 
	 typed interactively by the user, then give an error
	 and DO NOT change the state of traceframe_number etc.

	 However if FROM_TTY is false, meaning that we're either
	 in a script, a loop, or a user-defined command, then 
	 DON'T give an error, but DO change the state of
	 traceframe_number etc. to invalid.

	 The rationalle is that if you typed the command, you
	 might just have committed a typo or something, and you'd
	 like to NOT lose your current debugging state.  However
	 if you're in a user-defined command or especially in a
	 loop, then you need a way to detect that the command
	 failed WITHOUT aborting.  This allows you to write
	 scripts that search thru the trace buffer until the end,
	 and then continue on to do something else.  */
  
      if (from_tty)
	error (_("Target failed to find requested trace frame."));
      else
	{
	  if (info_verbose)
	    printf_filtered ("End of trace buffer.\n");
#if 0 /* dubious now?  */
	  /* The following will not recurse, since it's
	     special-cased.  */
	  trace_find_command ("-1", from_tty);
#endif
	}
    }
  
  tp = get_tracepoint_by_number_on_target (target_tracept);

  reinit_frame_cache ();
  target_dcache_invalidate ();

  set_tracepoint_num (tp ? tp->base.number : target_tracept);

  if (target_frameno != get_traceframe_number ())
    observer_notify_traceframe_changed (target_frameno, tracepoint_number);

  set_current_traceframe (target_frameno);

  if (target_frameno == -1)
    set_traceframe_context (NULL);
  else
    set_traceframe_context (get_current_frame ());

  if (traceframe_number >= 0)
    {
      /* Use different branches for MI and CLI to make CLI messages
	 i18n-eable.  */
      if (ui_out_is_mi_like_p (uiout))
	{
	  ui_out_field_string (uiout, "found", "1");
	  ui_out_field_int (uiout, "tracepoint", tracepoint_number);
	  ui_out_field_int (uiout, "traceframe", traceframe_number);
	}
      else
	{
	  printf_unfiltered (_("Found trace frame %d, tracepoint %d\n"),
			     traceframe_number, tracepoint_number);
	}
    }
  else
    {
      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_string (uiout, "found", "0");
      else if (type == tfind_number && num == -1)
	printf_unfiltered (_("No longer looking at any trace frame\n"));
      else /* This case may never occur, check.  */
	printf_unfiltered (_("No trace frame found\n"));
    }

  /* If we're in nonstop mode and getting out of looking at trace
     frames, there won't be any current frame to go back to and
     display.  */
  if (from_tty
      && (has_stack_frames () || traceframe_number >= 0))
    {
      enum print_what print_what;

      /* NOTE: in imitation of the step command, try to determine
         whether we have made a transition from one function to
         another.  If so, we'll print the "stack frame" (ie. the new
         function and it's arguments) -- otherwise we'll just show the
         new source line.  */

      if (frame_id_eq (old_frame_id,
		       get_frame_id (get_current_frame ())))
	print_what = SRC_LINE;
      else
	print_what = SRC_AND_LOC;

      print_stack_frame (get_selected_frame (NULL), 1, print_what, 1);
      do_displays ();
    }
}

/* Error on looking at traceframes while trace is running.  */

void
check_trace_running (struct trace_status *status)
{
  if (status->running && status->filename == NULL)
    error (_("May not look at trace frames while trace is running."));
}

/* trace_find_command takes a trace frame number n, 
   sends "QTFrame:<n>" to the target, 
   and accepts a reply that may contain several optional pieces
   of information: a frame number, a tracepoint number, and an
   indication of whether this is a trap frame or a stepping frame.

   The minimal response is just "OK" (which indicates that the 
   target does not give us a frame number or a tracepoint number).
   Instead of that, the target may send us a string containing
   any combination of:
   F<hexnum>    (gives the selected frame number)
   T<hexnum>    (gives the selected tracepoint number)
 */

/* tfind command */
static void
trace_find_command (char *args, int from_tty)
{ /* This should only be called with a numeric argument.  */
  int frameno = -1;

  check_trace_running (current_trace_status ());
  
  if (args == 0 || *args == 0)
    { /* TFIND with no args means find NEXT trace frame.  */
      if (traceframe_number == -1)
	frameno = 0;	/* "next" is first one.  */
        else
	frameno = traceframe_number + 1;
    }
  else if (0 == strcmp (args, "-"))
    {
      if (traceframe_number == -1)
	error (_("not debugging trace buffer"));
      else if (from_tty && traceframe_number == 0)
	error (_("already at start of trace buffer"));
      
      frameno = traceframe_number - 1;
      }
  /* A hack to work around eval's need for fp to have been collected.  */
  else if (0 == strcmp (args, "-1"))
    frameno = -1;
  else
    frameno = parse_and_eval_long (args);

  if (frameno < -1)
    error (_("invalid input (%d is less than zero)"), frameno);

  tfind_1 (tfind_number, frameno, 0, 0, from_tty);
}

/* tfind end */
static void
trace_find_end_command (char *args, int from_tty)
{
  trace_find_command ("-1", from_tty);
}

/* tfind start */
static void
trace_find_start_command (char *args, int from_tty)
{
  trace_find_command ("0", from_tty);
}

/* tfind pc command */
static void
trace_find_pc_command (char *args, int from_tty)
{
  CORE_ADDR pc;

  check_trace_running (current_trace_status ());

  if (args == 0 || *args == 0)
    pc = regcache_read_pc (get_current_regcache ());
  else
    pc = parse_and_eval_address (args);

  tfind_1 (tfind_pc, 0, pc, 0, from_tty);
}

/* tfind tracepoint command */
static void
trace_find_tracepoint_command (char *args, int from_tty)
{
  int tdp;
  struct tracepoint *tp;

  check_trace_running (current_trace_status ());

  if (args == 0 || *args == 0)
    {
      if (tracepoint_number == -1)
	error (_("No current tracepoint -- please supply an argument."));
      else
	tdp = tracepoint_number;	/* Default is current TDP.  */
    }
  else
    tdp = parse_and_eval_long (args);

  /* If we have the tracepoint on hand, use the number that the
     target knows about (which may be different if we disconnected
     and reconnected).  */
  tp = get_tracepoint (tdp);
  if (tp)
    tdp = tp->number_on_target;

  tfind_1 (tfind_tp, tdp, 0, 0, from_tty);
}

/* TFIND LINE command:

   This command will take a sourceline for argument, just like BREAK
   or TRACE (ie. anything that "decode_line_1" can handle).

   With no argument, this command will find the next trace frame 
   corresponding to a source line OTHER THAN THE CURRENT ONE.  */

static void
trace_find_line_command (char *args, int from_tty)
{
  static CORE_ADDR start_pc, end_pc;
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  struct cleanup *old_chain;

  check_trace_running (current_trace_status ());

  if (args == 0 || *args == 0)
    {
      sal = find_pc_line (get_frame_pc (get_current_frame ()), 0);
      sals.nelts = 1;
      sals.sals = (struct symtab_and_line *)
	xmalloc (sizeof (struct symtab_and_line));
      sals.sals[0] = sal;
    }
  else
    {
      sals = decode_line_with_current_source (args, DECODE_LINE_FUNFIRSTLINE);
      sal = sals.sals[0];
    }
  
  old_chain = make_cleanup (xfree, sals.sals);
  if (sal.symtab == 0)
    error (_("No line number information available."));

  if (sal.line > 0 && find_line_pc_range (sal, &start_pc, &end_pc))
    {
      if (start_pc == end_pc)
  	{
	  printf_filtered ("Line %d of \"%s\"",
			   sal.line,
			   symtab_to_filename_for_display (sal.symtab));
	  wrap_here ("  ");
	  printf_filtered (" is at address ");
	  print_address (get_current_arch (), start_pc, gdb_stdout);
	  wrap_here ("  ");
	  printf_filtered (" but contains no code.\n");
	  sal = find_pc_line (start_pc, 0);
	  if (sal.line > 0
	      && find_line_pc_range (sal, &start_pc, &end_pc)
	      && start_pc != end_pc)
	    printf_filtered ("Attempting to find line %d instead.\n",
			     sal.line);
  	  else
	    error (_("Cannot find a good line."));
  	}
      }
    else
    /* Is there any case in which we get here, and have an address
       which the user would want to see?  If we have debugging
       symbols and no line numbers?  */
    error (_("Line number %d is out of range for \"%s\"."),
	   sal.line, symtab_to_filename_for_display (sal.symtab));

  /* Find within range of stated line.  */
  if (args && *args)
    tfind_1 (tfind_range, 0, start_pc, end_pc - 1, from_tty);
  else
    tfind_1 (tfind_outside, 0, start_pc, end_pc - 1, from_tty);
  do_cleanups (old_chain);
}

/* tfind range command */
static void
trace_find_range_command (char *args, int from_tty)
{
  static CORE_ADDR start, stop;
  char *tmp;

  check_trace_running (current_trace_status ());

  if (args == 0 || *args == 0)
    { /* XXX FIXME: what should default behavior be?  */
      printf_filtered ("Usage: tfind range <startaddr>,<endaddr>\n");
      return;
    }

  if (0 != (tmp = strchr (args, ',')))
    {
      *tmp++ = '\0';	/* Terminate start address.  */
      tmp = skip_spaces (tmp);
      start = parse_and_eval_address (args);
      stop = parse_and_eval_address (tmp);
    }
  else
    {			/* No explicit end address?  */
      start = parse_and_eval_address (args);
      stop = start + 1;	/* ??? */
    }

  tfind_1 (tfind_range, 0, start, stop, from_tty);
}

/* tfind outside command */
static void
trace_find_outside_command (char *args, int from_tty)
{
  CORE_ADDR start, stop;
  char *tmp;

  if (current_trace_status ()->running
      && current_trace_status ()->filename == NULL)
    error (_("May not look at trace frames while trace is running."));

  if (args == 0 || *args == 0)
    { /* XXX FIXME: what should default behavior be?  */
      printf_filtered ("Usage: tfind outside <startaddr>,<endaddr>\n");
      return;
    }

  if (0 != (tmp = strchr (args, ',')))
    {
      *tmp++ = '\0';	/* Terminate start address.  */
      tmp = skip_spaces (tmp);
      start = parse_and_eval_address (args);
      stop = parse_and_eval_address (tmp);
    }
  else
    {			/* No explicit end address?  */
      start = parse_and_eval_address (args);
      stop = start + 1;	/* ??? */
    }

  tfind_1 (tfind_outside, 0, start, stop, from_tty);
}

/* info scope command: list the locals for a scope.  */
static void
scope_info (char *args, int from_tty)
{
  struct symtabs_and_lines sals;
  struct symbol *sym;
  struct bound_minimal_symbol msym;
  const struct block *block;
  const char *symname;
  char *save_args = args;
  struct block_iterator iter;
  int j, count = 0;
  struct gdbarch *gdbarch;
  int regno;

  if (args == 0 || *args == 0)
    error (_("requires an argument (function, "
	     "line or *addr) to define a scope"));

  sals = decode_line_1 (&args, DECODE_LINE_FUNFIRSTLINE, NULL, 0);
  if (sals.nelts == 0)
    return;		/* Presumably decode_line_1 has already warned.  */

  /* Resolve line numbers to PC.  */
  resolve_sal_pc (&sals.sals[0]);
  block = block_for_pc (sals.sals[0].pc);

  while (block != 0)
    {
      QUIT;			/* Allow user to bail out with ^C.  */
      ALL_BLOCK_SYMBOLS (block, iter, sym)
	{
	  QUIT;			/* Allow user to bail out with ^C.  */
	  if (count == 0)
	    printf_filtered ("Scope for %s:\n", save_args);
	  count++;

	  symname = SYMBOL_PRINT_NAME (sym);
	  if (symname == NULL || *symname == '\0')
	    continue;		/* Probably botched, certainly useless.  */

	  gdbarch = symbol_arch (sym);

	  printf_filtered ("Symbol %s is ", symname);

	  if (SYMBOL_COMPUTED_OPS (sym) != NULL)
	    SYMBOL_COMPUTED_OPS (sym)->describe_location (sym,
							  BLOCK_START (block),
							  gdb_stdout);
	  else
	    {
	      switch (SYMBOL_CLASS (sym))
		{
		default:
		case LOC_UNDEF:	/* Messed up symbol?  */
		  printf_filtered ("a bogus symbol, class %d.\n",
				   SYMBOL_CLASS (sym));
		  count--;		/* Don't count this one.  */
		  continue;
		case LOC_CONST:
		  printf_filtered ("a constant with value %s (%s)",
				   plongest (SYMBOL_VALUE (sym)),
				   hex_string (SYMBOL_VALUE (sym)));
		  break;
		case LOC_CONST_BYTES:
		  printf_filtered ("constant bytes: ");
		  if (SYMBOL_TYPE (sym))
		    for (j = 0; j < TYPE_LENGTH (SYMBOL_TYPE (sym)); j++)
		      fprintf_filtered (gdb_stdout, " %02x",
					(unsigned) SYMBOL_VALUE_BYTES (sym)[j]);
		  break;
		case LOC_STATIC:
		  printf_filtered ("in static storage at address ");
		  printf_filtered ("%s", paddress (gdbarch,
						   SYMBOL_VALUE_ADDRESS (sym)));
		  break;
		case LOC_REGISTER:
		  /* GDBARCH is the architecture associated with the objfile
		     the symbol is defined in; the target architecture may be
		     different, and may provide additional registers.  However,
		     we do not know the target architecture at this point.
		     We assume the objfile architecture will contain all the
		     standard registers that occur in debug info in that
		     objfile.  */
		  regno = SYMBOL_REGISTER_OPS (sym)->register_number (sym,
								      gdbarch);

		  if (SYMBOL_IS_ARGUMENT (sym))
		    printf_filtered ("an argument in register $%s",
				     gdbarch_register_name (gdbarch, regno));
		  else
		    printf_filtered ("a local variable in register $%s",
				     gdbarch_register_name (gdbarch, regno));
		  break;
		case LOC_ARG:
		  printf_filtered ("an argument at stack/frame offset %s",
				   plongest (SYMBOL_VALUE (sym)));
		  break;
		case LOC_LOCAL:
		  printf_filtered ("a local variable at frame offset %s",
				   plongest (SYMBOL_VALUE (sym)));
		  break;
		case LOC_REF_ARG:
		  printf_filtered ("a reference argument at offset %s",
				   plongest (SYMBOL_VALUE (sym)));
		  break;
		case LOC_REGPARM_ADDR:
		  /* Note comment at LOC_REGISTER.  */
		  regno = SYMBOL_REGISTER_OPS (sym)->register_number (sym,
								      gdbarch);
		  printf_filtered ("the address of an argument, in register $%s",
				   gdbarch_register_name (gdbarch, regno));
		  break;
		case LOC_TYPEDEF:
		  printf_filtered ("a typedef.\n");
		  continue;
		case LOC_LABEL:
		  printf_filtered ("a label at address ");
		  printf_filtered ("%s", paddress (gdbarch,
						   SYMBOL_VALUE_ADDRESS (sym)));
		  break;
		case LOC_BLOCK:
		  printf_filtered ("a function at address ");
		  printf_filtered ("%s",
				   paddress (gdbarch, BLOCK_START (SYMBOL_BLOCK_VALUE (sym))));
		  break;
		case LOC_UNRESOLVED:
		  msym = lookup_minimal_symbol (SYMBOL_LINKAGE_NAME (sym),
						NULL, NULL);
		  if (msym.minsym == NULL)
		    printf_filtered ("Unresolved Static");
		  else
		    {
		      printf_filtered ("static storage at address ");
		      printf_filtered ("%s",
				       paddress (gdbarch,
						 BMSYMBOL_VALUE_ADDRESS (msym)));
		    }
		  break;
		case LOC_OPTIMIZED_OUT:
		  printf_filtered ("optimized out.\n");
		  continue;
		case LOC_COMPUTED:
		  gdb_assert_not_reached (_("LOC_COMPUTED variable missing a method"));
		}
	    }
	  if (SYMBOL_TYPE (sym))
	    printf_filtered (", length %d.\n",
			     TYPE_LENGTH (check_typedef (SYMBOL_TYPE (sym))));
	}
      if (BLOCK_FUNCTION (block))
	break;
      else
	block = BLOCK_SUPERBLOCK (block);
    }
  if (count <= 0)
    printf_filtered ("Scope for %s contains no locals or arguments.\n",
		     save_args);
}

/* Helper for trace_dump_command.  Dump the action list starting at
   ACTION.  STEPPING_ACTIONS is true if we're iterating over the
   actions of the body of a while-stepping action.  STEPPING_FRAME is
   set if the current traceframe was determined to be a while-stepping
   traceframe.  */

static void
trace_dump_actions (struct command_line *action,
		    int stepping_actions, int stepping_frame,
		    int from_tty)
{
  const char *action_exp, *next_comma;

  for (; action != NULL; action = action->next)
    {
      struct cmd_list_element *cmd;

      QUIT;			/* Allow user to bail out with ^C.  */
      action_exp = action->line;
      action_exp = skip_spaces_const (action_exp);

      /* The collection actions to be done while stepping are
         bracketed by the commands "while-stepping" and "end".  */

      if (*action_exp == '#')	/* comment line */
	continue;

      cmd = lookup_cmd (&action_exp, cmdlist, "", -1, 1);
      if (cmd == 0)
	error (_("Bad action list item: %s"), action_exp);

      if (cmd_cfunc_eq (cmd, while_stepping_pseudocommand))
	{
	  int i;

	  for (i = 0; i < action->body_count; ++i)
	    trace_dump_actions (action->body_list[i],
				1, stepping_frame, from_tty);
	}
      else if (cmd_cfunc_eq (cmd, collect_pseudocommand))
	{
	  /* Display the collected data.
	     For the trap frame, display only what was collected at
	     the trap.  Likewise for stepping frames, display only
	     what was collected while stepping.  This means that the
	     two boolean variables, STEPPING_FRAME and
	     STEPPING_ACTIONS should be equal.  */
	  if (stepping_frame == stepping_actions)
	    {
	      char *cmd = NULL;
	      struct cleanup *old_chain
		= make_cleanup (free_current_contents, &cmd);
	      int trace_string = 0;

	      if (*action_exp == '/')
		action_exp = decode_agent_options (action_exp, &trace_string);

	      do
		{		/* Repeat over a comma-separated list.  */
		  QUIT;		/* Allow user to bail out with ^C.  */
		  if (*action_exp == ',')
		    action_exp++;
		  action_exp = skip_spaces_const (action_exp);

		  next_comma = strchr (action_exp, ',');

		  if (0 == strncasecmp (action_exp, "$reg", 4))
		    registers_info (NULL, from_tty);
		  else if (0 == strncasecmp (action_exp, "$_ret", 5))
		    ;
		  else if (0 == strncasecmp (action_exp, "$loc", 4))
		    locals_info (NULL, from_tty);
		  else if (0 == strncasecmp (action_exp, "$arg", 4))
		    args_info (NULL, from_tty);
		  else
		    {		/* variable */
		      if (next_comma != NULL)
			{
			  size_t len = next_comma - action_exp;

			  cmd = xrealloc (cmd, len + 1);
			  memcpy (cmd, action_exp, len);
			  cmd[len] = 0;
			}
		      else
			{
			  size_t len = strlen (action_exp);

			  cmd = xrealloc (cmd, len + 1);
			  memcpy (cmd, action_exp, len + 1);
			}

		      printf_filtered ("%s = ", cmd);
		      output_command_const (cmd, from_tty);
		      printf_filtered ("\n");
		    }
		  action_exp = next_comma;
		}
	      while (action_exp && *action_exp == ',');

	      do_cleanups (old_chain);
	    }
	}
    }
}

/* Return bp_location of the tracepoint associated with the current
   traceframe.  Set *STEPPING_FRAME_P to 1 if the current traceframe
   is a stepping traceframe.  */

struct bp_location *
get_traceframe_location (int *stepping_frame_p)
{
  struct tracepoint *t;
  struct bp_location *tloc;
  struct regcache *regcache;

  if (tracepoint_number == -1)
    error (_("No current trace frame."));

  t = get_tracepoint (tracepoint_number);

  if (t == NULL)
    error (_("No known tracepoint matches 'current' tracepoint #%d."),
	   tracepoint_number);

  /* The current frame is a trap frame if the frame PC is equal to the
     tracepoint PC.  If not, then the current frame was collected
     during single-stepping.  */
  regcache = get_current_regcache ();

  /* If the traceframe's address matches any of the tracepoint's
     locations, assume it is a direct hit rather than a while-stepping
     frame.  (FIXME this is not reliable, should record each frame's
     type.)  */
  for (tloc = t->base.loc; tloc; tloc = tloc->next)
    if (tloc->address == regcache_read_pc (regcache))
      {
	*stepping_frame_p = 0;
	return tloc;
      }

  /* If this is a stepping frame, we don't know which location
     triggered.  The first is as good (or bad) a guess as any...  */
  *stepping_frame_p = 1;
  return t->base.loc;
}

/* Return all the actions, including default collect, of a tracepoint
   T.  It constructs cleanups into the chain, and leaves the caller to
   handle them (call do_cleanups).  */

static struct command_line *
all_tracepoint_actions_and_cleanup (struct breakpoint *t)
{
  struct command_line *actions;

  actions = breakpoint_commands (t);

  /* If there are default expressions to collect, make up a collect
     action and prepend to the action list to encode.  Note that since
     validation is per-tracepoint (local var "xyz" might be valid for
     one tracepoint and not another, etc), we make up the action on
     the fly, and don't cache it.  */
  if (*default_collect)
    {
      struct command_line *default_collect_action;
      char *default_collect_line;

      default_collect_line = xstrprintf ("collect %s", default_collect);
      make_cleanup (xfree, default_collect_line);

      validate_actionline (default_collect_line, t);
      default_collect_action = xmalloc (sizeof (struct command_line));
      make_cleanup (xfree, default_collect_action);
      default_collect_action->next = actions;
      default_collect_action->line = default_collect_line;
      actions = default_collect_action;
    }

  return actions;
}

/* The tdump command.  */

static void
trace_dump_command (char *args, int from_tty)
{
  int stepping_frame = 0;
  struct bp_location *loc;
  struct cleanup *old_chain;
  struct command_line *actions;

  /* This throws an error is not inspecting a trace frame.  */
  loc = get_traceframe_location (&stepping_frame);

  printf_filtered ("Data collected at tracepoint %d, trace frame %d:\n",
		   tracepoint_number, traceframe_number);

  old_chain = make_cleanup (null_cleanup, NULL);

  /* This command only makes sense for the current frame, not the
     selected frame.  */
  make_cleanup_restore_current_thread ();
  select_frame (get_current_frame ());

  actions = all_tracepoint_actions_and_cleanup (loc->owner);

  trace_dump_actions (actions, 0, stepping_frame, from_tty);

  do_cleanups (old_chain);
}

/* Encode a piece of a tracepoint's source-level definition in a form
   that is suitable for both protocol and saving in files.  */
/* This version does not do multiple encodes for long strings; it should
   return an offset to the next piece to encode.  FIXME  */

extern int
encode_source_string (int tpnum, ULONGEST addr,
		      char *srctype, char *src, char *buf, int buf_size)
{
  if (80 + strlen (srctype) > buf_size)
    error (_("Buffer too small for source encoding"));
  sprintf (buf, "%x:%s:%s:%x:%x:",
	   tpnum, phex_nz (addr, sizeof (addr)),
	   srctype, 0, (int) strlen (src));
  if (strlen (buf) + strlen (src) * 2 >= buf_size)
    error (_("Source string too long for buffer"));
  bin2hex ((gdb_byte *) src, buf + strlen (buf), strlen (src));
  return -1;
}

/* Tell the target what to do with an ongoing tracing run if GDB
   disconnects for some reason.  */

static void
set_disconnected_tracing (char *args, int from_tty,
			  struct cmd_list_element *c)
{
  target_set_disconnected_tracing (disconnected_tracing);
}

static void
set_circular_trace_buffer (char *args, int from_tty,
			   struct cmd_list_element *c)
{
  target_set_circular_trace_buffer (circular_trace_buffer);
}

static void
set_trace_buffer_size (char *args, int from_tty,
			   struct cmd_list_element *c)
{
  target_set_trace_buffer_size (trace_buffer_size);
}

static void
set_trace_user (char *args, int from_tty,
		struct cmd_list_element *c)
{
  int ret;

  ret = target_set_trace_notes (trace_user, NULL, NULL);

  if (!ret)
    warning (_("Target does not support trace notes, user ignored"));
}

static void
set_trace_notes (char *args, int from_tty,
		 struct cmd_list_element *c)
{
  int ret;

  ret = target_set_trace_notes (NULL, trace_notes, NULL);

  if (!ret)
    warning (_("Target does not support trace notes, note ignored"));
}

static void
set_trace_stop_notes (char *args, int from_tty,
		      struct cmd_list_element *c)
{
  int ret;

  ret = target_set_trace_notes (NULL, NULL, trace_stop_notes);

  if (!ret)
    warning (_("Target does not support trace notes, stop note ignored"));
}

/* Convert the memory pointed to by mem into hex, placing result in buf.
 * Return a pointer to the last char put in buf (null)
 * "stolen" from sparc-stub.c
 */

static const char hexchars[] = "0123456789abcdef";

static char *
mem2hex (gdb_byte *mem, char *buf, int count)
{
  gdb_byte ch;

  while (count-- > 0)
    {
      ch = *mem++;

      *buf++ = hexchars[ch >> 4];
      *buf++ = hexchars[ch & 0xf];
    }

  *buf = 0;

  return buf;
}

int
get_traceframe_number (void)
{
  return traceframe_number;
}

int
get_tracepoint_number (void)
{
  return tracepoint_number;
}

/* Make the traceframe NUM be the current trace frame.  Does nothing
   if NUM is already current.  */

void
set_current_traceframe (int num)
{
  int newnum;

  if (traceframe_number == num)
    {
      /* Nothing to do.  */
      return;
    }

  newnum = target_trace_find (tfind_number, num, 0, 0, NULL);

  if (newnum != num)
    warning (_("could not change traceframe"));

  set_traceframe_num (newnum);

  /* Changing the traceframe changes our view of registers and of the
     frame chain.  */
  registers_changed ();

  clear_traceframe_info ();
}

/* A cleanup used when switching away and back from tfind mode.  */

struct current_traceframe_cleanup
{
  /* The traceframe we were inspecting.  */
  int traceframe_number;
};

static void
do_restore_current_traceframe_cleanup (void *arg)
{
  struct current_traceframe_cleanup *old = arg;

  set_current_traceframe (old->traceframe_number);
}

static void
restore_current_traceframe_cleanup_dtor (void *arg)
{
  struct current_traceframe_cleanup *old = arg;

  xfree (old);
}

struct cleanup *
make_cleanup_restore_current_traceframe (void)
{
  struct current_traceframe_cleanup *old;

  old = xmalloc (sizeof (struct current_traceframe_cleanup));
  old->traceframe_number = traceframe_number;

  return make_cleanup_dtor (do_restore_current_traceframe_cleanup, old,
			    restore_current_traceframe_cleanup_dtor);
}

/* Given a number and address, return an uploaded tracepoint with that
   number, creating if necessary.  */

struct uploaded_tp *
get_uploaded_tp (int num, ULONGEST addr, struct uploaded_tp **utpp)
{
  struct uploaded_tp *utp;

  for (utp = *utpp; utp; utp = utp->next)
    if (utp->number == num && utp->addr == addr)
      return utp;
  utp = (struct uploaded_tp *) xmalloc (sizeof (struct uploaded_tp));
  memset (utp, 0, sizeof (struct uploaded_tp));
  utp->number = num;
  utp->addr = addr;
  utp->actions = NULL;
  utp->step_actions = NULL;
  utp->cmd_strings = NULL;
  utp->next = *utpp;
  *utpp = utp;
  return utp;
}

void
free_uploaded_tps (struct uploaded_tp **utpp)
{
  struct uploaded_tp *next_one;

  while (*utpp)
    {
      next_one = (*utpp)->next;
      xfree (*utpp);
      *utpp = next_one;
    }
}

/* Given a number and address, return an uploaded tracepoint with that
   number, creating if necessary.  */

struct uploaded_tsv *
get_uploaded_tsv (int num, struct uploaded_tsv **utsvp)
{
  struct uploaded_tsv *utsv;

  for (utsv = *utsvp; utsv; utsv = utsv->next)
    if (utsv->number == num)
      return utsv;
  utsv = (struct uploaded_tsv *) xmalloc (sizeof (struct uploaded_tsv));
  memset (utsv, 0, sizeof (struct uploaded_tsv));
  utsv->number = num;
  utsv->next = *utsvp;
  *utsvp = utsv;
  return utsv;
}

void
free_uploaded_tsvs (struct uploaded_tsv **utsvp)
{
  struct uploaded_tsv *next_one;

  while (*utsvp)
    {
      next_one = (*utsvp)->next;
      xfree (*utsvp);
      *utsvp = next_one;
    }
}

/* FIXME this function is heuristic and will miss the cases where the
   conditional is semantically identical but differs in whitespace,
   such as "x == 0" vs "x==0".  */

static int
cond_string_is_same (char *str1, char *str2)
{
  if (str1 == NULL || str2 == NULL)
    return (str1 == str2);

  return (strcmp (str1, str2) == 0);
}

/* Look for an existing tracepoint that seems similar enough to the
   uploaded one.  Enablement isn't compared, because the user can
   toggle that freely, and may have done so in anticipation of the
   next trace run.  Return the location of matched tracepoint.  */

static struct bp_location *
find_matching_tracepoint_location (struct uploaded_tp *utp)
{
  VEC(breakpoint_p) *tp_vec = all_tracepoints ();
  int ix;
  struct breakpoint *b;
  struct bp_location *loc;

  for (ix = 0; VEC_iterate (breakpoint_p, tp_vec, ix, b); ix++)
    {
      struct tracepoint *t = (struct tracepoint *) b;

      if (b->type == utp->type
	  && t->step_count == utp->step
	  && t->pass_count == utp->pass
	  && cond_string_is_same (t->base.cond_string, utp->cond_string)
	  /* FIXME also test actions.  */
	  )
	{
	  /* Scan the locations for an address match.  */
	  for (loc = b->loc; loc; loc = loc->next)
	    {
	      if (loc->address == utp->addr)
		return loc;
	    }
	}
    }
  return NULL;
}

/* Given a list of tracepoints uploaded from a target, attempt to
   match them up with existing tracepoints, and create new ones if not
   found.  */

void
merge_uploaded_tracepoints (struct uploaded_tp **uploaded_tps)
{
  struct uploaded_tp *utp;
  /* A set of tracepoints which are modified.  */
  VEC(breakpoint_p) *modified_tp = NULL;
  int ix;
  struct breakpoint *b;

  /* Look for GDB tracepoints that match up with our uploaded versions.  */
  for (utp = *uploaded_tps; utp; utp = utp->next)
    {
      struct bp_location *loc;
      struct tracepoint *t;

      loc = find_matching_tracepoint_location (utp);
      if (loc)
	{
	  int found = 0;

	  /* Mark this location as already inserted.  */
	  loc->inserted = 1;
	  t = (struct tracepoint *) loc->owner;
	  printf_filtered (_("Assuming tracepoint %d is same "
			     "as target's tracepoint %d at %s.\n"),
			   loc->owner->number, utp->number,
			   paddress (loc->gdbarch, utp->addr));

	  /* The tracepoint LOC->owner was modified (the location LOC
	     was marked as inserted in the target).  Save it in
	     MODIFIED_TP if not there yet.  The 'breakpoint-modified'
	     observers will be notified later once for each tracepoint
	     saved in MODIFIED_TP.  */
	  for (ix = 0;
	       VEC_iterate (breakpoint_p, modified_tp, ix, b);
	       ix++)
	    if (b == loc->owner)
	      {
		found = 1;
		break;
	      }
	  if (!found)
	    VEC_safe_push (breakpoint_p, modified_tp, loc->owner);
	}
      else
	{
	  t = create_tracepoint_from_upload (utp);
	  if (t)
	    printf_filtered (_("Created tracepoint %d for "
			       "target's tracepoint %d at %s.\n"),
			     t->base.number, utp->number,
			     paddress (get_current_arch (), utp->addr));
	  else
	    printf_filtered (_("Failed to create tracepoint for target's "
			       "tracepoint %d at %s, skipping it.\n"),
			     utp->number,
			     paddress (get_current_arch (), utp->addr));
	}
      /* Whether found or created, record the number used by the
	 target, to help with mapping target tracepoints back to their
	 counterparts here.  */
      if (t)
	t->number_on_target = utp->number;
    }

  /* Notify 'breakpoint-modified' observer that at least one of B's
     locations was changed.  */
  for (ix = 0; VEC_iterate (breakpoint_p, modified_tp, ix, b); ix++)
    observer_notify_breakpoint_modified (b);

  VEC_free (breakpoint_p, modified_tp);
  free_uploaded_tps (uploaded_tps);
}

/* Trace state variables don't have much to identify them beyond their
   name, so just use that to detect matches.  */

static struct trace_state_variable *
find_matching_tsv (struct uploaded_tsv *utsv)
{
  if (!utsv->name)
    return NULL;

  return find_trace_state_variable (utsv->name);
}

static struct trace_state_variable *
create_tsv_from_upload (struct uploaded_tsv *utsv)
{
  const char *namebase;
  char *buf;
  int try_num = 0;
  struct trace_state_variable *tsv;
  struct cleanup *old_chain;

  if (utsv->name)
    {
      namebase = utsv->name;
      buf = xstrprintf ("%s", namebase);
    }
  else
    {
      namebase = "__tsv";
      buf = xstrprintf ("%s_%d", namebase, try_num++);
    }

  /* Fish for a name that is not in use.  */
  /* (should check against all internal vars?)  */
  while (find_trace_state_variable (buf))
    {
      xfree (buf);
      buf = xstrprintf ("%s_%d", namebase, try_num++);
    }

  old_chain = make_cleanup (xfree, buf);

  /* We have an available name, create the variable.  */
  tsv = create_trace_state_variable (buf);
  tsv->initial_value = utsv->initial_value;
  tsv->builtin = utsv->builtin;

  observer_notify_tsv_created (tsv);

  do_cleanups (old_chain);

  return tsv;
}

/* Given a list of uploaded trace state variables, try to match them
   up with existing variables, or create additional ones.  */

void
merge_uploaded_trace_state_variables (struct uploaded_tsv **uploaded_tsvs)
{
  int ix;
  struct uploaded_tsv *utsv;
  struct trace_state_variable *tsv;
  int highest;

  /* Most likely some numbers will have to be reassigned as part of
     the merge, so clear them all in anticipation.  */
  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    tsv->number = 0;

  for (utsv = *uploaded_tsvs; utsv; utsv = utsv->next)
    {
      tsv = find_matching_tsv (utsv);
      if (tsv)
	{
	  if (info_verbose)
	    printf_filtered (_("Assuming trace state variable $%s "
			       "is same as target's variable %d.\n"),
			     tsv->name, utsv->number);
	}
      else
	{
	  tsv = create_tsv_from_upload (utsv);
	  if (info_verbose)
	    printf_filtered (_("Created trace state variable "
			       "$%s for target's variable %d.\n"),
			     tsv->name, utsv->number);
	}
      /* Give precedence to numberings that come from the target.  */
      if (tsv)
	tsv->number = utsv->number;
    }

  /* Renumber everything that didn't get a target-assigned number.  */
  highest = 0;
  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    if (tsv->number > highest)
      highest = tsv->number;

  ++highest;
  for (ix = 0; VEC_iterate (tsv_s, tvariables, ix, tsv); ++ix)
    if (tsv->number == 0)
      tsv->number = highest++;

  free_uploaded_tsvs (uploaded_tsvs);
}

/* Parse the part of trace status syntax that is shared between
   the remote protocol and the trace file reader.  */

void
parse_trace_status (char *line, struct trace_status *ts)
{
  char *p = line, *p1, *p2, *p3, *p_temp;
  int end;
  ULONGEST val;

  ts->running_known = 1;
  ts->running = (*p++ == '1');
  ts->stop_reason = trace_stop_reason_unknown;
  xfree (ts->stop_desc);
  ts->stop_desc = NULL;
  ts->traceframe_count = -1;
  ts->traceframes_created = -1;
  ts->buffer_free = -1;
  ts->buffer_size = -1;
  ts->disconnected_tracing = 0;
  ts->circular_buffer = 0;
  xfree (ts->user_name);
  ts->user_name = NULL;
  xfree (ts->notes);
  ts->notes = NULL;
  ts->start_time = ts->stop_time = 0;

  while (*p++)
    {
      p1 = strchr (p, ':');
      if (p1 == NULL)
	error (_("Malformed trace status, at %s\n\
Status line: '%s'\n"), p, line);
      p3 = strchr (p, ';');
      if (p3 == NULL)
	p3 = p + strlen (p);
      if (strncmp (p, stop_reason_names[trace_buffer_full], p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_reason = trace_buffer_full;
	}
      else if (strncmp (p, stop_reason_names[trace_never_run], p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_reason = trace_never_run;
	}
      else if (strncmp (p, stop_reason_names[tracepoint_passcount],
			p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_reason = tracepoint_passcount;
	  ts->stopping_tracepoint = val;
	}
      else if (strncmp (p, stop_reason_names[tstop_command], p1 - p) == 0)
	{
	  p2 = strchr (++p1, ':');
	  if (!p2 || p2 > p3)
	    {
	      /*older style*/
	      p2 = p1;
	    }
	  else if (p2 != p1)
	    {
	      ts->stop_desc = xmalloc (strlen (line));
	      end = hex2bin (p1, (gdb_byte *) ts->stop_desc, (p2 - p1) / 2);
	      ts->stop_desc[end] = '\0';
	    }
	  else
	    ts->stop_desc = xstrdup ("");

	  p = unpack_varlen_hex (++p2, &val);
	  ts->stop_reason = tstop_command;
	}
      else if (strncmp (p, stop_reason_names[trace_disconnected], p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_reason = trace_disconnected;
	}
      else if (strncmp (p, stop_reason_names[tracepoint_error], p1 - p) == 0)
	{
	  p2 = strchr (++p1, ':');
	  if (p2 != p1)
	    {
	      ts->stop_desc = xmalloc ((p2 - p1) / 2 + 1);
	      end = hex2bin (p1, (gdb_byte *) ts->stop_desc, (p2 - p1) / 2);
	      ts->stop_desc[end] = '\0';
	    }
	  else
	    ts->stop_desc = xstrdup ("");

	  p = unpack_varlen_hex (++p2, &val);
	  ts->stopping_tracepoint = val;
	  ts->stop_reason = tracepoint_error;
	}
      else if (strncmp (p, "tframes", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->traceframe_count = val;
	}
      else if (strncmp (p, "tcreated", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->traceframes_created = val;
	}
      else if (strncmp (p, "tfree", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->buffer_free = val;
	}
      else if (strncmp (p, "tsize", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->buffer_size = val;
	}
      else if (strncmp (p, "disconn", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->disconnected_tracing = val;
	}
      else if (strncmp (p, "circular", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->circular_buffer = val;
	}
      else if (strncmp (p, "starttime", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->start_time = val;
	}
      else if (strncmp (p, "stoptime", p1 - p) == 0)
	{
	  p = unpack_varlen_hex (++p1, &val);
	  ts->stop_time = val;
	}
      else if (strncmp (p, "username", p1 - p) == 0)
	{
	  ++p1;
	  ts->user_name = xmalloc (strlen (p) / 2);
	  end = hex2bin (p1, (gdb_byte *) ts->user_name, (p3 - p1)  / 2);
	  ts->user_name[end] = '\0';
	  p = p3;
	}
      else if (strncmp (p, "notes", p1 - p) == 0)
	{
	  ++p1;
	  ts->notes = xmalloc (strlen (p) / 2);
	  end = hex2bin (p1, (gdb_byte *) ts->notes, (p3 - p1) / 2);
	  ts->notes[end] = '\0';
	  p = p3;
	}
      else
	{
	  /* Silently skip unknown optional info.  */
	  p_temp = strchr (p1 + 1, ';');
	  if (p_temp)
	    p = p_temp;
	  else
	    /* Must be at the end.  */
	    break;
	}
    }
}

void
parse_tracepoint_status (char *p, struct breakpoint *bp,
			 struct uploaded_tp *utp)
{
  ULONGEST uval;
  struct tracepoint *tp = (struct tracepoint *) bp;

  p = unpack_varlen_hex (p, &uval);
  if (tp)
    tp->base.hit_count += uval;
  else
    utp->hit_count += uval;
  p = unpack_varlen_hex (p + 1, &uval);
  if (tp)
    tp->traceframe_usage += uval;
  else
    utp->traceframe_usage += uval;
  /* Ignore any extra, allowing for future extensions.  */
}

/* Given a line of text defining a part of a tracepoint, parse it into
   an "uploaded tracepoint".  */

void
parse_tracepoint_definition (char *line, struct uploaded_tp **utpp)
{
  char *p;
  char piece;
  ULONGEST num, addr, step, pass, orig_size, xlen, start;
  int enabled, end;
  enum bptype type;
  char *cond, *srctype, *buf;
  struct uploaded_tp *utp = NULL;

  p = line;
  /* Both tracepoint and action definitions start with the same number
     and address sequence.  */
  piece = *p++;
  p = unpack_varlen_hex (p, &num);
  p++;  /* skip a colon */
  p = unpack_varlen_hex (p, &addr);
  p++;  /* skip a colon */
  if (piece == 'T')
    {
      enabled = (*p++ == 'E');
      p++;  /* skip a colon */
      p = unpack_varlen_hex (p, &step);
      p++;  /* skip a colon */
      p = unpack_varlen_hex (p, &pass);
      type = bp_tracepoint;
      cond = NULL;
      /* Thumb through optional fields.  */
      while (*p == ':')
	{
	  p++;  /* skip a colon */
	  if (*p == 'F')
	    {
	      type = bp_fast_tracepoint;
	      p++;
	      p = unpack_varlen_hex (p, &orig_size);
	    }
	  else if (*p == 'S')
	    {
	      type = bp_static_tracepoint;
	      p++;
	    }
	  else if (*p == 'X')
	    {
	      p++;
	      p = unpack_varlen_hex (p, &xlen);
	      p++;  /* skip a comma */
	      cond = (char *) xmalloc (2 * xlen + 1);
	      strncpy (cond, p, 2 * xlen);
	      cond[2 * xlen] = '\0';
	      p += 2 * xlen;
	    }
	  else
	    warning (_("Unrecognized char '%c' in tracepoint "
		       "definition, skipping rest"), *p);
	}
      utp = get_uploaded_tp (num, addr, utpp);
      utp->type = type;
      utp->enabled = enabled;
      utp->step = step;
      utp->pass = pass;
      utp->cond = cond;
    }
  else if (piece == 'A')
    {
      utp = get_uploaded_tp (num, addr, utpp);
      VEC_safe_push (char_ptr, utp->actions, xstrdup (p));
    }
  else if (piece == 'S')
    {
      utp = get_uploaded_tp (num, addr, utpp);
      VEC_safe_push (char_ptr, utp->step_actions, xstrdup (p));
    }
  else if (piece == 'Z')
    {
      /* Parse a chunk of source form definition.  */
      utp = get_uploaded_tp (num, addr, utpp);
      srctype = p;
      p = strchr (p, ':');
      p++;  /* skip a colon */
      p = unpack_varlen_hex (p, &start);
      p++;  /* skip a colon */
      p = unpack_varlen_hex (p, &xlen);
      p++;  /* skip a colon */

      buf = alloca (strlen (line));

      end = hex2bin (p, (gdb_byte *) buf, strlen (p) / 2);
      buf[end] = '\0';

      if (startswith (srctype, "at:"))
	utp->at_string = xstrdup (buf);
      else if (startswith (srctype, "cond:"))
	utp->cond_string = xstrdup (buf);
      else if (startswith (srctype, "cmd:"))
	VEC_safe_push (char_ptr, utp->cmd_strings, xstrdup (buf));
    }
  else if (piece == 'V')
    {
      utp = get_uploaded_tp (num, addr, utpp);

      parse_tracepoint_status (p, NULL, utp);
    }
  else
    {
      /* Don't error out, the target might be sending us optional
	 info that we don't care about.  */
      warning (_("Unrecognized tracepoint piece '%c', ignoring"), piece);
    }
}

/* Convert a textual description of a trace state variable into an
   uploaded object.  */

void
parse_tsv_definition (char *line, struct uploaded_tsv **utsvp)
{
  char *p, *buf;
  ULONGEST num, initval, builtin;
  int end;
  struct uploaded_tsv *utsv = NULL;

  buf = alloca (strlen (line));

  p = line;
  p = unpack_varlen_hex (p, &num);
  p++; /* skip a colon */
  p = unpack_varlen_hex (p, &initval);
  p++; /* skip a colon */
  p = unpack_varlen_hex (p, &builtin);
  p++; /* skip a colon */
  end = hex2bin (p, (gdb_byte *) buf, strlen (p) / 2);
  buf[end] = '\0';

  utsv = get_uploaded_tsv (num, utsvp);
  utsv->initial_value = initval;
  utsv->builtin = builtin;
  utsv->name = xstrdup (buf);
}

void
free_current_marker (void *arg)
{
  struct static_tracepoint_marker **marker_p = arg;

  if (*marker_p != NULL)
    {
      release_static_tracepoint_marker (*marker_p);
      xfree (*marker_p);
    }
  else
    *marker_p = NULL;
}

/* Given a line of text defining a static tracepoint marker, parse it
   into a "static tracepoint marker" object.  Throws an error is
   parsing fails.  If PP is non-null, it points to one past the end of
   the parsed marker definition.  */

void
parse_static_tracepoint_marker_definition (char *line, char **pp,
					   struct static_tracepoint_marker *marker)
{
  char *p, *endp;
  ULONGEST addr;
  int end;

  p = line;
  p = unpack_varlen_hex (p, &addr);
  p++;  /* skip a colon */

  marker->gdbarch = target_gdbarch ();
  marker->address = (CORE_ADDR) addr;

  endp = strchr (p, ':');
  if (endp == NULL)
    error (_("bad marker definition: %s"), line);

  marker->str_id = xmalloc (endp - p + 1);
  end = hex2bin (p, (gdb_byte *) marker->str_id, (endp - p + 1) / 2);
  marker->str_id[end] = '\0';

  p += 2 * end;
  p++;  /* skip a colon */

  marker->extra = xmalloc (strlen (p) + 1);
  end = hex2bin (p, (gdb_byte *) marker->extra, strlen (p) / 2);
  marker->extra[end] = '\0';

  if (pp)
    *pp = p;
}

/* Release a static tracepoint marker's contents.  Note that the
   object itself isn't released here.  There objects are usually on
   the stack.  */

void
release_static_tracepoint_marker (struct static_tracepoint_marker *marker)
{
  xfree (marker->str_id);
  marker->str_id = NULL;
}

/* Print MARKER to gdb_stdout.  */

static void
print_one_static_tracepoint_marker (int count,
				    struct static_tracepoint_marker *marker)
{
  struct command_line *l;
  struct symbol *sym;

  char wrap_indent[80];
  char extra_field_indent[80];
  struct ui_out *uiout = current_uiout;
  struct cleanup *bkpt_chain;
  VEC(breakpoint_p) *tracepoints;

  struct symtab_and_line sal;

  init_sal (&sal);

  sal.pc = marker->address;

  tracepoints = static_tracepoints_here (marker->address);

  bkpt_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "marker");

  /* A counter field to help readability.  This is not a stable
     identifier!  */
  ui_out_field_int (uiout, "count", count);

  ui_out_field_string (uiout, "marker-id", marker->str_id);

  ui_out_field_fmt (uiout, "enabled", "%c",
		    !VEC_empty (breakpoint_p, tracepoints) ? 'y' : 'n');
  ui_out_spaces (uiout, 2);

  strcpy (wrap_indent, "                                   ");

  if (gdbarch_addr_bit (marker->gdbarch) <= 32)
    strcat (wrap_indent, "           ");
  else
    strcat (wrap_indent, "                   ");

  strcpy (extra_field_indent, "         ");

  ui_out_field_core_addr (uiout, "addr", marker->gdbarch, marker->address);

  sal = find_pc_line (marker->address, 0);
  sym = find_pc_sect_function (marker->address, NULL);
  if (sym)
    {
      ui_out_text (uiout, "in ");
      ui_out_field_string (uiout, "func",
			   SYMBOL_PRINT_NAME (sym));
      ui_out_wrap_hint (uiout, wrap_indent);
      ui_out_text (uiout, " at ");
    }
  else
    ui_out_field_skip (uiout, "func");

  if (sal.symtab != NULL)
    {
      ui_out_field_string (uiout, "file",
			   symtab_to_filename_for_display (sal.symtab));
      ui_out_text (uiout, ":");

      if (ui_out_is_mi_like_p (uiout))
	{
	  const char *fullname = symtab_to_fullname (sal.symtab);

	  ui_out_field_string (uiout, "fullname", fullname);
	}
      else
	ui_out_field_skip (uiout, "fullname");

      ui_out_field_int (uiout, "line", sal.line);
    }
  else
    {
      ui_out_field_skip (uiout, "fullname");
      ui_out_field_skip (uiout, "line");
    }

  ui_out_text (uiout, "\n");
  ui_out_text (uiout, extra_field_indent);
  ui_out_text (uiout, _("Data: \""));
  ui_out_field_string (uiout, "extra-data", marker->extra);
  ui_out_text (uiout, "\"\n");

  if (!VEC_empty (breakpoint_p, tracepoints))
    {
      struct cleanup *cleanup_chain;
      int ix;
      struct breakpoint *b;

      cleanup_chain = make_cleanup_ui_out_tuple_begin_end (uiout,
							   "tracepoints-at");

      ui_out_text (uiout, extra_field_indent);
      ui_out_text (uiout, _("Probed by static tracepoints: "));
      for (ix = 0; VEC_iterate(breakpoint_p, tracepoints, ix, b); ix++)
	{
	  if (ix > 0)
	    ui_out_text (uiout, ", ");
	  ui_out_text (uiout, "#");
	  ui_out_field_int (uiout, "tracepoint-id", b->number);
	}

      do_cleanups (cleanup_chain);

      if (ui_out_is_mi_like_p (uiout))
	ui_out_field_int (uiout, "number-of-tracepoints",
			  VEC_length(breakpoint_p, tracepoints));
      else
	ui_out_text (uiout, "\n");
    }
  VEC_free (breakpoint_p, tracepoints);

  do_cleanups (bkpt_chain);
}

static void
info_static_tracepoint_markers_command (char *arg, int from_tty)
{
  VEC(static_tracepoint_marker_p) *markers;
  struct cleanup *old_chain;
  struct static_tracepoint_marker *marker;
  struct ui_out *uiout = current_uiout;
  int i;

  /* We don't have to check target_can_use_agent and agent's capability on
     static tracepoint here, in order to be compatible with older GDBserver.
     We don't check USE_AGENT is true or not, because static tracepoints
     don't work without in-process agent, so we don't bother users to type
     `set agent on' when to use static tracepoint.  */

  old_chain
    = make_cleanup_ui_out_table_begin_end (uiout, 5, -1,
					   "StaticTracepointMarkersTable");

  ui_out_table_header (uiout, 7, ui_left, "counter", "Cnt");

  ui_out_table_header (uiout, 40, ui_left, "marker-id", "ID");

  ui_out_table_header (uiout, 3, ui_left, "enabled", "Enb");
  if (gdbarch_addr_bit (target_gdbarch ()) <= 32)
    ui_out_table_header (uiout, 10, ui_left, "addr", "Address");
  else
    ui_out_table_header (uiout, 18, ui_left, "addr", "Address");
  ui_out_table_header (uiout, 40, ui_noalign, "what", "What");

  ui_out_table_body (uiout);

  markers = target_static_tracepoint_markers_by_strid (NULL);
  make_cleanup (VEC_cleanup (static_tracepoint_marker_p), &markers);

  for (i = 0;
       VEC_iterate (static_tracepoint_marker_p,
		    markers, i, marker);
       i++)
    {
      print_one_static_tracepoint_marker (i + 1, marker);
      release_static_tracepoint_marker (marker);
    }

  do_cleanups (old_chain);
}

/* The $_sdata convenience variable is a bit special.  We don't know
   for sure type of the value until we actually have a chance to fetch
   the data --- the size of the object depends on what has been
   collected.  We solve this by making $_sdata be an internalvar that
   creates a new value on access.  */

/* Return a new value with the correct type for the sdata object of
   the current trace frame.  Return a void value if there's no object
   available.  */

static struct value *
sdata_make_value (struct gdbarch *gdbarch, struct internalvar *var,
		  void *ignore)
{
  LONGEST size;
  gdb_byte *buf;

  /* We need to read the whole object before we know its size.  */
  size = target_read_alloc (&current_target,
			    TARGET_OBJECT_STATIC_TRACE_DATA,
			    NULL, &buf);
  if (size >= 0)
    {
      struct value *v;
      struct type *type;

      type = init_vector_type (builtin_type (gdbarch)->builtin_true_char,
			       size);
      v = allocate_value (type);
      memcpy (value_contents_raw (v), buf, size);
      xfree (buf);
      return v;
    }
  else
    return allocate_value (builtin_type (gdbarch)->builtin_void);
}

#if !defined(HAVE_LIBEXPAT)

struct traceframe_info *
parse_traceframe_info (const char *tframe_info)
{
  static int have_warned;

  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML trace frame info; XML support "
		 "was disabled at compile time"));
    }

  return NULL;
}

#else /* HAVE_LIBEXPAT */

#include "xml-support.h"

/* Handle the start of a <memory> element.  */

static void
traceframe_info_start_memory (struct gdb_xml_parser *parser,
			      const struct gdb_xml_element *element,
			      void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct traceframe_info *info = user_data;
  struct mem_range *r = VEC_safe_push (mem_range_s, info->memory, NULL);
  ULONGEST *start_p, *length_p;

  start_p = xml_find_attribute (attributes, "start")->value;
  length_p = xml_find_attribute (attributes, "length")->value;

  r->start = *start_p;
  r->length = *length_p;
}

/* Handle the start of a <tvar> element.  */

static void
traceframe_info_start_tvar (struct gdb_xml_parser *parser,
			     const struct gdb_xml_element *element,
			     void *user_data,
			     VEC(gdb_xml_value_s) *attributes)
{
  struct traceframe_info *info = user_data;
  const char *id_attrib = xml_find_attribute (attributes, "id")->value;
  int id = gdb_xml_parse_ulongest (parser, id_attrib);

  VEC_safe_push (int, info->tvars, id);
}

/* Discard the constructed trace frame info (if an error occurs).  */

static void
free_result (void *p)
{
  struct traceframe_info *result = p;

  free_traceframe_info (result);
}

/* The allowed elements and attributes for an XML memory map.  */

static const struct gdb_xml_attribute memory_attributes[] = {
  { "start", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "length", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute tvar_attributes[] = {
  { "id", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element traceframe_info_children[] = {
  { "memory", memory_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    traceframe_info_start_memory, NULL },
  { "tvar", tvar_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    traceframe_info_start_tvar, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element traceframe_info_elements[] = {
  { "traceframe-info", NULL, traceframe_info_children, GDB_XML_EF_NONE,
    NULL, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

/* Parse a traceframe-info XML document.  */

struct traceframe_info *
parse_traceframe_info (const char *tframe_info)
{
  struct traceframe_info *result;
  struct cleanup *back_to;

  result = XCNEW (struct traceframe_info);
  back_to = make_cleanup (free_result, result);

  if (gdb_xml_parse_quick (_("trace frame info"),
			   "traceframe-info.dtd", traceframe_info_elements,
			   tframe_info, result) == 0)
    {
      /* Parsed successfully, keep the result.  */
      discard_cleanups (back_to);

      return result;
    }

  do_cleanups (back_to);
  return NULL;
}

#endif /* HAVE_LIBEXPAT */

/* Returns the traceframe_info object for the current traceframe.
   This is where we avoid re-fetching the object from the target if we
   already have it cached.  */

struct traceframe_info *
get_traceframe_info (void)
{
  if (traceframe_info == NULL)
    traceframe_info = target_traceframe_info ();

  return traceframe_info;
}

/* If the target supports the query, return in RESULT the set of
   collected memory in the current traceframe, found within the LEN
   bytes range starting at MEMADDR.  Returns true if the target
   supports the query, otherwise returns false, and RESULT is left
   undefined.  */

int
traceframe_available_memory (VEC(mem_range_s) **result,
			     CORE_ADDR memaddr, ULONGEST len)
{
  struct traceframe_info *info = get_traceframe_info ();

  if (info != NULL)
    {
      struct mem_range *r;
      int i;

      *result = NULL;

      for (i = 0; VEC_iterate (mem_range_s, info->memory, i, r); i++)
	if (mem_ranges_overlap (r->start, r->length, memaddr, len))
	  {
	    ULONGEST lo1, hi1, lo2, hi2;
	    struct mem_range *nr;

	    lo1 = memaddr;
	    hi1 = memaddr + len;

	    lo2 = r->start;
	    hi2 = r->start + r->length;

	    nr = VEC_safe_push (mem_range_s, *result, NULL);

	    nr->start = max (lo1, lo2);
	    nr->length = min (hi1, hi2) - nr->start;
	  }

      normalize_mem_ranges (*result);
      return 1;
    }

  return 0;
}

/* Implementation of `sdata' variable.  */

static const struct internalvar_funcs sdata_funcs =
{
  sdata_make_value,
  NULL,
  NULL
};

/* module initialization */
void
_initialize_tracepoint (void)
{
  struct cmd_list_element *c;

  /* Explicitly create without lookup, since that tries to create a
     value with a void typed value, and when we get here, gdbarch
     isn't initialized yet.  At this point, we're quite sure there
     isn't another convenience variable of the same name.  */
  create_internalvar_type_lazy ("_sdata", &sdata_funcs, NULL);

  traceframe_number = -1;
  tracepoint_number = -1;

  add_info ("scope", scope_info,
	    _("List the variables local to a scope"));

  add_cmd ("tracepoints", class_trace, NULL,
	   _("Tracing of program execution without stopping the program."),
	   &cmdlist);

  add_com ("tdump", class_trace, trace_dump_command,
	   _("Print everything collected at the current tracepoint."));

  c = add_com ("tvariable", class_trace, trace_variable_command,_("\
Define a trace state variable.\n\
Argument is a $-prefixed name, optionally followed\n\
by '=' and an expression that sets the initial value\n\
at the start of tracing."));
  set_cmd_completer (c, expression_completer);

  add_cmd ("tvariable", class_trace, delete_trace_variable_command, _("\
Delete one or more trace state variables.\n\
Arguments are the names of the variables to delete.\n\
If no arguments are supplied, delete all variables."), &deletelist);
  /* FIXME add a trace variable completer.  */

  add_info ("tvariables", tvariables_info, _("\
Status of trace state variables and their values.\n\
"));

  add_info ("static-tracepoint-markers",
	    info_static_tracepoint_markers_command, _("\
List target static tracepoints markers.\n\
"));

  add_prefix_cmd ("tfind", class_trace, trace_find_command, _("\
Select a trace frame;\n\
No argument means forward by one frame; '-' means backward by one frame."),
		  &tfindlist, "tfind ", 1, &cmdlist);

  add_cmd ("outside", class_trace, trace_find_outside_command, _("\
Select a trace frame whose PC is outside the given range (exclusive).\n\
Usage: tfind outside addr1, addr2"),
	   &tfindlist);

  add_cmd ("range", class_trace, trace_find_range_command, _("\
Select a trace frame whose PC is in the given range (inclusive).\n\
Usage: tfind range addr1,addr2"),
	   &tfindlist);

  add_cmd ("line", class_trace, trace_find_line_command, _("\
Select a trace frame by source line.\n\
Argument can be a line number (with optional source file),\n\
a function name, or '*' followed by an address.\n\
Default argument is 'the next source line that was traced'."),
	   &tfindlist);

  add_cmd ("tracepoint", class_trace, trace_find_tracepoint_command, _("\
Select a trace frame by tracepoint number.\n\
Default is the tracepoint for the current trace frame."),
	   &tfindlist);

  add_cmd ("pc", class_trace, trace_find_pc_command, _("\
Select a trace frame by PC.\n\
Default is the current PC, or the PC of the current trace frame."),
	   &tfindlist);

  add_cmd ("end", class_trace, trace_find_end_command, _("\
De-select any trace frame and resume 'live' debugging."),
	   &tfindlist);

  add_alias_cmd ("none", "end", class_trace, 0, &tfindlist);

  add_cmd ("start", class_trace, trace_find_start_command,
	   _("Select the first trace frame in the trace buffer."),
	   &tfindlist);

  add_com ("tstatus", class_trace, trace_status_command,
	   _("Display the status of the current trace data collection."));

  add_com ("tstop", class_trace, trace_stop_command, _("\
Stop trace data collection.\n\
Usage: tstop [ <notes> ... ]\n\
Any arguments supplied are recorded with the trace as a stop reason and\n\
reported by tstatus (if the target supports trace notes)."));

  add_com ("tstart", class_trace, trace_start_command, _("\
Start trace data collection.\n\
Usage: tstart [ <notes> ... ]\n\
Any arguments supplied are recorded with the trace as a note and\n\
reported by tstatus (if the target supports trace notes)."));

  add_com ("end", class_trace, end_actions_pseudocommand, _("\
Ends a list of commands or actions.\n\
Several GDB commands allow you to enter a list of commands or actions.\n\
Entering \"end\" on a line by itself is the normal way to terminate\n\
such a list.\n\n\
Note: the \"end\" command cannot be used at the gdb prompt."));

  add_com ("while-stepping", class_trace, while_stepping_pseudocommand, _("\
Specify single-stepping behavior at a tracepoint.\n\
Argument is number of instructions to trace in single-step mode\n\
following the tracepoint.  This command is normally followed by\n\
one or more \"collect\" commands, to specify what to collect\n\
while single-stepping.\n\n\
Note: this command can only be used in a tracepoint \"actions\" list."));

  add_com_alias ("ws", "while-stepping", class_alias, 0);
  add_com_alias ("stepping", "while-stepping", class_alias, 0);

  add_com ("collect", class_trace, collect_pseudocommand, _("\
Specify one or more data items to be collected at a tracepoint.\n\
Accepts a comma-separated list of (one or more) expressions.  GDB will\n\
collect all data (variables, registers) referenced by that expression.\n\
Also accepts the following special arguments:\n\
    $regs   -- all registers.\n\
    $args   -- all function arguments.\n\
    $locals -- all variables local to the block/function scope.\n\
    $_sdata -- static tracepoint data (ignored for non-static tracepoints).\n\
Note: this command can only be used in a tracepoint \"actions\" list."));

  add_com ("teval", class_trace, teval_pseudocommand, _("\
Specify one or more expressions to be evaluated at a tracepoint.\n\
Accepts a comma-separated list of (one or more) expressions.\n\
The result of each evaluation will be discarded.\n\
Note: this command can only be used in a tracepoint \"actions\" list."));

  add_com ("actions", class_trace, trace_actions_command, _("\
Specify the actions to be taken at a tracepoint.\n\
Tracepoint actions may include collecting of specified data,\n\
single-stepping, or enabling/disabling other tracepoints,\n\
depending on target's capabilities."));

  default_collect = xstrdup ("");
  add_setshow_string_cmd ("default-collect", class_trace,
			  &default_collect, _("\
Set the list of expressions to collect by default"), _("\
Show the list of expressions to collect by default"), NULL,
			  NULL, NULL,
			  &setlist, &showlist);

  add_setshow_boolean_cmd ("disconnected-tracing", no_class,
			   &disconnected_tracing, _("\
Set whether tracing continues after GDB disconnects."), _("\
Show whether tracing continues after GDB disconnects."), _("\
Use this to continue a tracing run even if GDB disconnects\n\
or detaches from the target.  You can reconnect later and look at\n\
trace data collected in the meantime."),
			   set_disconnected_tracing,
			   NULL,
			   &setlist,
			   &showlist);

  add_setshow_boolean_cmd ("circular-trace-buffer", no_class,
			   &circular_trace_buffer, _("\
Set target's use of circular trace buffer."), _("\
Show target's use of circular trace buffer."), _("\
Use this to make the trace buffer into a circular buffer,\n\
which will discard traceframes (oldest first) instead of filling\n\
up and stopping the trace run."),
			   set_circular_trace_buffer,
			   NULL,
			   &setlist,
			   &showlist);

  add_setshow_zuinteger_unlimited_cmd ("trace-buffer-size", no_class,
				       &trace_buffer_size, _("\
Set requested size of trace buffer."), _("\
Show requested size of trace buffer."), _("\
Use this to choose a size for the trace buffer.  Some targets\n\
may have fixed or limited buffer sizes.  Specifying \"unlimited\" or -1\n\
disables any attempt to set the buffer size and lets the target choose."),
				       set_trace_buffer_size, NULL,
				       &setlist, &showlist);

  add_setshow_string_cmd ("trace-user", class_trace,
			  &trace_user, _("\
Set the user name to use for current and future trace runs"), _("\
Show the user name to use for current and future trace runs"), NULL,
			  set_trace_user, NULL,
			  &setlist, &showlist);

  add_setshow_string_cmd ("trace-notes", class_trace,
			  &trace_notes, _("\
Set notes string to use for current and future trace runs"), _("\
Show the notes string to use for current and future trace runs"), NULL,
			  set_trace_notes, NULL,
			  &setlist, &showlist);

  add_setshow_string_cmd ("trace-stop-notes", class_trace,
			  &trace_stop_notes, _("\
Set notes string to use for future tstop commands"), _("\
Show the notes string to use for future tstop commands"), NULL,
			  set_trace_stop_notes, NULL,
			  &setlist, &showlist);
}
