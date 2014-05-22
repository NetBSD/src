/* Top level of GCC compilers (cc1, cc1plus, etc.)
   Copyright (C) 1987-2013 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* This is the top level of cc1/c++.
   It parses command args, opens files, invokes the various passes
   in the proper order, and counts the time used by each.
   Error messages and low-level interface to malloc also handled here.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "line-map.h"
#include "input.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "flags.h"
#include "insn-attr.h"
#include "insn-config.h"
#include "insn-flags.h"
#include "hard-reg-set.h"
#include "recog.h"
#include "output.h"
#include "except.h"
#include "function.h"
#include "toplev.h"
#include "expr.h"
#include "basic-block.h"
#include "intl.h"
#include "ggc.h"
#include "graph.h"
#include "regs.h"
#include "diagnostic-core.h"
#include "params.h"
#include "reload.h"
#include "debug.h"
#include "target.h"
#include "langhooks.h"
#include "cfgloop.h"
#include "hosthooks.h"
#include "cgraph.h"
#include "opts.h"
#include "coverage.h"
#include "value-prof.h"
#include "tree-inline.h"
#include "tree-flow.h"
#include "tree-pass.h"
#include "tree-dump.h"
#include "df.h"
#include "predict.h"
#include "lto-streamer.h"
#include "plugin.h"
#include "ipa-utils.h"
#include "tree-pretty-print.h" /* for dump_function_header */

/* This is used for debugging.  It allows the current pass to printed
   from anywhere in compilation.
   The variable current_pass is also used for statistics and plugins.  */
struct opt_pass *current_pass;

static void register_pass_name (struct opt_pass *, const char *);

/* Call from anywhere to find out what pass this is.  Useful for
   printing out debugging information deep inside an service
   routine.  */
void
print_current_pass (FILE *file)
{
  if (current_pass)
    fprintf (file, "current pass = %s (%d)\n",
	     current_pass->name, current_pass->static_pass_number);
  else
    fprintf (file, "no current pass.\n");
}


/* Call from the debugger to get the current pass name.  */
DEBUG_FUNCTION void
debug_pass (void)
{
  print_current_pass (stderr);
}



/* Global variables used to communicate with passes.  */
bool in_gimple_form;
bool first_pass_instance;


/* This is called from various places for FUNCTION_DECL, VAR_DECL,
   and TYPE_DECL nodes.

   This does nothing for local (non-static) variables, unless the
   variable is a register variable with DECL_ASSEMBLER_NAME set.  In
   that case, or if the variable is not an automatic, it sets up the
   RTL and outputs any assembler code (label definition, storage
   allocation and initialization).

   DECL is the declaration.  TOP_LEVEL is nonzero
   if this declaration is not within a function.  */

void
rest_of_decl_compilation (tree decl,
			  int top_level,
			  int at_end)
{
  /* We deferred calling assemble_alias so that we could collect
     other attributes such as visibility.  Emit the alias now.  */
  if (!in_lto_p)
  {
    tree alias;
    alias = lookup_attribute ("alias", DECL_ATTRIBUTES (decl));
    if (alias)
      {
	alias = TREE_VALUE (TREE_VALUE (alias));
	alias = get_identifier (TREE_STRING_POINTER (alias));
	/* A quirk of the initial implementation of aliases required that the
	   user add "extern" to all of them.  Which is silly, but now
	   historical.  Do note that the symbol is in fact locally defined.  */
	if (!lookup_attribute ("weakref", DECL_ATTRIBUTES (decl)))
	  DECL_EXTERNAL (decl) = 0;
	assemble_alias (decl, alias);
      }
  }

  /* Can't defer this, because it needs to happen before any
     later function definitions are processed.  */
  if (DECL_ASSEMBLER_NAME_SET_P (decl) && DECL_REGISTER (decl))
    make_decl_rtl (decl);

  /* Forward declarations for nested functions are not "external",
     but we need to treat them as if they were.  */
  if (TREE_STATIC (decl) || DECL_EXTERNAL (decl)
      || TREE_CODE (decl) == FUNCTION_DECL)
    {
      timevar_push (TV_VARCONST);

      /* Don't output anything when a tentative file-scope definition
	 is seen.  But at end of compilation, do output code for them.

	 We do output all variables and rely on
	 callgraph code to defer them except for forward declarations
	 (see gcc.c-torture/compile/920624-1.c) */
      if ((at_end
	   || !DECL_DEFER_OUTPUT (decl)
	   || DECL_INITIAL (decl))
	  && (TREE_CODE (decl) != VAR_DECL || !DECL_HAS_VALUE_EXPR_P (decl))
	  && !DECL_EXTERNAL (decl))
	{
	  /* When reading LTO unit, we also read varpool, so do not
	     rebuild it.  */
	  if (in_lto_p && !at_end)
	    ;
	  else if (TREE_CODE (decl) != FUNCTION_DECL)
	    varpool_finalize_decl (decl);
	}

#ifdef ASM_FINISH_DECLARE_OBJECT
      if (decl == last_assemble_variable_decl)
	{
	  ASM_FINISH_DECLARE_OBJECT (asm_out_file, decl,
				     top_level, at_end);
	}
#endif

      timevar_pop (TV_VARCONST);
    }
  else if (TREE_CODE (decl) == TYPE_DECL
	   /* Like in rest_of_type_compilation, avoid confusing the debug
	      information machinery when there are errors.  */
	   && !seen_error ())
    {
      timevar_push (TV_SYMOUT);
      debug_hooks->type_decl (decl, !top_level);
      timevar_pop (TV_SYMOUT);
    }

  /* Let cgraph know about the existence of variables.  */
  if (in_lto_p && !at_end)
    ;
  else if (TREE_CODE (decl) == VAR_DECL && !DECL_EXTERNAL (decl)
	   && TREE_STATIC (decl))
    varpool_node_for_decl (decl);
}

/* Called after finishing a record, union or enumeral type.  */

void
rest_of_type_compilation (tree type, int toplev)
{
  /* Avoid confusing the debug information machinery when there are
     errors.  */
  if (seen_error ())
    return;

  timevar_push (TV_SYMOUT);
  debug_hooks->type_decl (TYPE_STUB_DECL (type), !toplev);
  timevar_pop (TV_SYMOUT);
}



void
finish_optimization_passes (void)
{
  int i;
  struct dump_file_info *dfi;
  char *name;

  timevar_push (TV_DUMP);
  if (profile_arc_flag || flag_test_coverage || flag_branch_probabilities)
    {
      dump_start (pass_profile.pass.static_pass_number, NULL);
      end_branch_prob ();
      dump_finish (pass_profile.pass.static_pass_number);
    }

  if (optimize > 0)
    {
      dump_start (pass_profile.pass.static_pass_number, NULL);
      print_combine_total_stats ();
      dump_finish (pass_profile.pass.static_pass_number);
    }

  /* Do whatever is necessary to finish printing the graphs.  */
  for (i = TDI_end; (dfi = get_dump_file_info (i)) != NULL; ++i)
    if (dump_initialized_p (i)
	&& (dfi->pflags & TDF_GRAPH) != 0
	&& (name = get_dump_file_name (i)) != NULL)
      {
	finish_graph_dump_file (name);
	free (name);
      }

  timevar_pop (TV_DUMP);
}

static unsigned int
execute_all_early_local_passes (void)
{
  /* Once this pass (and its sub-passes) are complete, all functions
     will be in SSA form.  Technically this state change is happening
     a tad early, since the sub-passes have not yet run, but since
     none of the sub-passes are IPA passes and do not create new
     functions, this is ok.  We're setting this value for the benefit
     of IPA passes that follow.  */
  if (cgraph_state < CGRAPH_STATE_IPA_SSA)
    cgraph_state = CGRAPH_STATE_IPA_SSA;
  return 0;
}

/* Gate: execute, or not, all of the non-trivial optimizations.  */

static bool
gate_all_early_local_passes (void)
{
	  /* Don't bother doing anything if the program has errors.  */
  return (!seen_error () && !in_lto_p);
}

struct simple_ipa_opt_pass pass_early_local_passes =
{
 {
  SIMPLE_IPA_PASS,
  "early_local_cleanups",		/* name */
  OPTGROUP_NONE,                        /* optinfo_flags */
  gate_all_early_local_passes,		/* gate */
  execute_all_early_local_passes,	/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  TV_EARLY_LOCAL,			/* tv_id */
  0,					/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_remove_functions	 		/* todo_flags_finish */
 }
};

/* Gate: execute, or not, all of the non-trivial optimizations.  */

static bool
gate_all_early_optimizations (void)
{
  return (optimize >= 1
	  /* Don't bother doing anything if the program has errors.  */
	  && !seen_error ());
}

static struct gimple_opt_pass pass_all_early_optimizations =
{
 {
  GIMPLE_PASS,
  "early_optimizations",		/* name */
  OPTGROUP_NONE,                        /* optinfo_flags */
  gate_all_early_optimizations,		/* gate */
  NULL,					/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  TV_NONE,				/* tv_id */
  0,					/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  0					/* todo_flags_finish */
 }
};

/* Gate: execute, or not, all of the non-trivial optimizations.  */

static bool
gate_all_optimizations (void)
{
  return optimize >= 1 && !optimize_debug;
}

static struct gimple_opt_pass pass_all_optimizations =
{
 {
  GIMPLE_PASS,
  "*all_optimizations",			/* name */
  OPTGROUP_NONE,                        /* optinfo_flags */
  gate_all_optimizations,		/* gate */
  NULL,					/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  TV_OPTIMIZE,				/* tv_id */
  0,					/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  0					/* todo_flags_finish */
 }
};

/* Gate: execute, or not, all of the non-trivial optimizations.  */

static bool
gate_all_optimizations_g (void)
{
  return optimize >= 1 && optimize_debug;
}

static struct gimple_opt_pass pass_all_optimizations_g =
{
 {
  GIMPLE_PASS,
  "*all_optimizations_g",		/* name */
  OPTGROUP_NONE,                        /* optinfo_flags */
  gate_all_optimizations_g,		/* gate */
  NULL,					/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  TV_OPTIMIZE,				/* tv_id */
  0,					/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  0					/* todo_flags_finish */
 }
};

static bool
gate_rest_of_compilation (void)
{
  /* Early return if there were errors.  We can run afoul of our
     consistency checks, and there's not really much point in fixing them.  */
  return !(rtl_dump_and_exit || flag_syntax_only || seen_error ());
}

static struct rtl_opt_pass pass_rest_of_compilation =
{
 {
  RTL_PASS,
  "*rest_of_compilation",               /* name */
  OPTGROUP_NONE,                        /* optinfo_flags */
  gate_rest_of_compilation,             /* gate */
  NULL,                                 /* execute */
  NULL,                                 /* sub */
  NULL,                                 /* next */
  0,                                    /* static_pass_number */
  TV_REST_OF_COMPILATION,               /* tv_id */
  PROP_rtl,                             /* properties_required */
  0,                                    /* properties_provided */
  0,                                    /* properties_destroyed */
  0,                                    /* todo_flags_start */
  TODO_ggc_collect                      /* todo_flags_finish */
 }
};

static bool
gate_postreload (void)
{
  return reload_completed;
}

static struct rtl_opt_pass pass_postreload =
{
 {
  RTL_PASS,
  "*all-postreload",                        /* name */
  OPTGROUP_NONE,                        /* optinfo_flags */
  gate_postreload,                      /* gate */
  NULL,                                 /* execute */
  NULL,                                 /* sub */
  NULL,                                 /* next */
  0,                                    /* static_pass_number */
  TV_POSTRELOAD,                        /* tv_id */
  PROP_rtl,                             /* properties_required */
  0,                                    /* properties_provided */
  0,                                    /* properties_destroyed */
  0,                                    /* todo_flags_start */
  TODO_ggc_collect | TODO_verify_rtl_sharing /* todo_flags_finish */
 }
};



/* The root of the compilation pass tree, once constructed.  */
struct opt_pass *all_passes, *all_small_ipa_passes, *all_lowering_passes,
  *all_regular_ipa_passes, *all_late_ipa_passes, *all_lto_gen_passes;

/* This is used by plugins, and should also be used in register_pass.  */
#define DEF_PASS_LIST(LIST) &LIST,
struct opt_pass **gcc_pass_lists[] = { GCC_PASS_LISTS NULL };
#undef DEF_PASS_LIST

/* A map from static pass id to optimization pass.  */
struct opt_pass **passes_by_id;
int passes_by_id_size;

/* Set the static pass number of pass PASS to ID and record that
   in the mapping from static pass number to pass.  */

static void
set_pass_for_id (int id, struct opt_pass *pass)
{
  pass->static_pass_number = id;
  if (passes_by_id_size <= id)
    {
      passes_by_id = XRESIZEVEC (struct opt_pass *, passes_by_id, id + 1);
      memset (passes_by_id + passes_by_id_size, 0,
	      (id + 1 - passes_by_id_size) * sizeof (void *));
      passes_by_id_size = id + 1;
    }
  passes_by_id[id] = pass;
}

/* Return the pass with the static pass number ID.  */

struct opt_pass *
get_pass_for_id (int id)
{
  if (id >= passes_by_id_size)
    return NULL;
  return passes_by_id[id];
}

/* Iterate over the pass tree allocating dump file numbers.  We want
   to do this depth first, and independent of whether the pass is
   enabled or not.  */

void
register_one_dump_file (struct opt_pass *pass)
{
  char *dot_name, *flag_name, *glob_name;
  const char *name, *full_name, *prefix;
  char num[10];
  int flags, id;
  int optgroup_flags = OPTGROUP_NONE;

  /* See below in next_pass_1.  */
  num[0] = '\0';
  if (pass->static_pass_number != -1)
    sprintf (num, "%d", ((int) pass->static_pass_number < 0
			 ? 1 : pass->static_pass_number));

  /* The name is both used to identify the pass for the purposes of plugins,
     and to specify dump file name and option.
     The latter two might want something short which is not quite unique; for
     that reason, we may have a disambiguating prefix, followed by a space
     to mark the start of the following dump file name / option string.  */
  name = strchr (pass->name, ' ');
  name = name ? name + 1 : pass->name;
  dot_name = concat (".", name, num, NULL);
  if (pass->type == SIMPLE_IPA_PASS || pass->type == IPA_PASS)
    {
      prefix = "ipa-";
      flags = TDF_IPA;
      optgroup_flags |= OPTGROUP_IPA;
    }
  else if (pass->type == GIMPLE_PASS)
    {
      prefix = "tree-";
      flags = TDF_TREE;
    }
  else
    {
      prefix = "rtl-";
      flags = TDF_RTL;
    }

  flag_name = concat (prefix, name, num, NULL);
  glob_name = concat (prefix, name, NULL);
  optgroup_flags |= pass->optinfo_flags;
  id = dump_register (dot_name, flag_name, glob_name, flags, optgroup_flags);
  set_pass_for_id (id, pass);
  full_name = concat (prefix, pass->name, num, NULL);
  register_pass_name (pass, full_name);
  free (CONST_CAST (char *, full_name));
}

/* Recursive worker function for register_dump_files.  */

static int
register_dump_files_1 (struct opt_pass *pass, int properties)
{
  do
    {
      int new_properties = (properties | pass->properties_provided)
			   & ~pass->properties_destroyed;

      if (pass->name && pass->name[0] != '*')
        register_one_dump_file (pass);

      if (pass->sub)
        new_properties = register_dump_files_1 (pass->sub, new_properties);

      /* If we have a gate, combine the properties that we could have with
         and without the pass being examined.  */
      if (pass->gate)
        properties &= new_properties;
      else
        properties = new_properties;

      pass = pass->next;
    }
  while (pass);

  return properties;
}

/* Register the dump files for the pipeline starting at PASS.
   PROPERTIES reflects the properties that are guaranteed to be available at
   the beginning of the pipeline.  */

static void
register_dump_files (struct opt_pass *pass,int properties)
{
  pass->properties_required |= properties;
  register_dump_files_1 (pass, properties);
}

struct pass_registry
{
  const char* unique_name;
  struct opt_pass *pass;
};

/* Pass registry hash function.  */

static hashval_t
passr_hash (const void *p)
{
  const struct pass_registry *const s = (const struct pass_registry *const) p;
  return htab_hash_string (s->unique_name);
}

/* Hash equal function  */

static int
passr_eq (const void *p1, const void *p2)
{
  const struct pass_registry *const s1 = (const struct pass_registry *const) p1;
  const struct pass_registry *const s2 = (const struct pass_registry *const) p2;

  return !strcmp (s1->unique_name, s2->unique_name);
}

static htab_t name_to_pass_map = NULL;

/* Register PASS with NAME.  */

static void
register_pass_name (struct opt_pass *pass, const char *name)
{
  struct pass_registry **slot;
  struct pass_registry pr;

  if (!name_to_pass_map)
    name_to_pass_map = htab_create (256, passr_hash, passr_eq, NULL);

  pr.unique_name = name;
  slot = (struct pass_registry **) htab_find_slot (name_to_pass_map, &pr, INSERT);
  if (!*slot)
    {
      struct pass_registry *new_pr;

      new_pr = XCNEW (struct pass_registry);
      new_pr->unique_name = xstrdup (name);
      new_pr->pass = pass;
      *slot = new_pr;
    }
  else
    return; /* Ignore plugin passes.  */
}

/* Map from pass id to canonicalized pass name.  */

typedef const char *char_ptr;
static vec<char_ptr> pass_tab = vNULL;

/* Callback function for traversing NAME_TO_PASS_MAP.  */

static int
pass_traverse (void **slot, void *data ATTRIBUTE_UNUSED)
{
  struct pass_registry **p = (struct pass_registry **)slot;
  struct opt_pass *pass = (*p)->pass;

  gcc_assert (pass->static_pass_number > 0);
  gcc_assert (pass_tab.exists ());

  pass_tab[pass->static_pass_number] = (*p)->unique_name;

  return 1;
}

/* The function traverses NAME_TO_PASS_MAP and creates a pass info
   table for dumping purpose.  */

static void
create_pass_tab (void)
{
  if (!flag_dump_passes)
    return;

  pass_tab.safe_grow_cleared (passes_by_id_size + 1);
  htab_traverse (name_to_pass_map, pass_traverse, NULL);
}

static bool override_gate_status (struct opt_pass *, tree, bool);

/* Dump the instantiated name for PASS. IS_ON indicates if PASS
   is turned on or not.  */

static void
dump_one_pass (struct opt_pass *pass, int pass_indent)
{
  int indent = 3 * pass_indent;
  const char *pn;
  bool is_on, is_really_on;

  is_on = (pass->gate == NULL) ? true : pass->gate();
  is_really_on = override_gate_status (pass, current_function_decl, is_on);

  if (pass->static_pass_number <= 0)
    pn = pass->name;
  else
    pn = pass_tab[pass->static_pass_number];

  fprintf (stderr, "%*s%-40s%*s:%s%s\n", indent, " ", pn,
           (15 - indent < 0 ? 0 : 15 - indent), " ",
           is_on ? "  ON" : "  OFF",
           ((!is_on) == (!is_really_on) ? ""
            : (is_really_on ? " (FORCED_ON)" : " (FORCED_OFF)")));
}

/* Dump pass list PASS with indentation INDENT.  */

static void
dump_pass_list (struct opt_pass *pass, int indent)
{
  do
    {
      dump_one_pass (pass, indent);
      if (pass->sub)
        dump_pass_list (pass->sub, indent + 1);
      pass = pass->next;
    }
  while (pass);
}

/* Dump all optimization passes.  */

void
dump_passes (void)
{
  struct cgraph_node *n, *node = NULL;

  create_pass_tab();

  FOR_EACH_DEFINED_FUNCTION (n)
    if (DECL_STRUCT_FUNCTION (n->symbol.decl))
      {
	node = n;
	break;
      }

  if (!node)
    return;

  push_cfun (DECL_STRUCT_FUNCTION (node->symbol.decl));

  dump_pass_list (all_lowering_passes, 1);
  dump_pass_list (all_small_ipa_passes, 1);
  dump_pass_list (all_regular_ipa_passes, 1);
  dump_pass_list (all_lto_gen_passes, 1);
  dump_pass_list (all_late_ipa_passes, 1);
  dump_pass_list (all_passes, 1);

  pop_cfun ();
}


/* Returns the pass with NAME.  */

static struct opt_pass *
get_pass_by_name (const char *name)
{
  struct pass_registry **slot, pr;

  pr.unique_name = name;
  slot = (struct pass_registry **) htab_find_slot (name_to_pass_map,
                                                   &pr, NO_INSERT);

  if (!slot || !*slot)
    return NULL;

  return (*slot)->pass;
}


/* Range [start, last].  */

struct uid_range
{
  unsigned int start;
  unsigned int last;
  const char *assem_name;
  struct uid_range *next;
};

typedef struct uid_range *uid_range_p;


static vec<uid_range_p>
      enabled_pass_uid_range_tab = vNULL;
static vec<uid_range_p>
      disabled_pass_uid_range_tab = vNULL;


/* Parse option string for -fdisable- and -fenable-
   The syntax of the options:

   -fenable-<pass_name>
   -fdisable-<pass_name>

   -fenable-<pass_name>=s1:e1,s2:e2,...
   -fdisable-<pass_name>=s1:e1,s2:e2,...
*/

static void
enable_disable_pass (const char *arg, bool is_enable)
{
  struct opt_pass *pass;
  char *range_str, *phase_name;
  char *argstr = xstrdup (arg);
  vec<uid_range_p> *tab = 0;

  range_str = strchr (argstr,'=');
  if (range_str)
    {
      *range_str = '\0';
      range_str++;
    }

  phase_name = argstr;
  if (!*phase_name)
    {
      if (is_enable)
        error ("unrecognized option -fenable");
      else
        error ("unrecognized option -fdisable");
      free (argstr);
      return;
    }
  pass = get_pass_by_name (phase_name);
  if (!pass || pass->static_pass_number == -1)
    {
      if (is_enable)
        error ("unknown pass %s specified in -fenable", phase_name);
      else
        error ("unknown pass %s specified in -fdisable", phase_name);
      free (argstr);
      return;
    }

  if (is_enable)
    tab = &enabled_pass_uid_range_tab;
  else
    tab = &disabled_pass_uid_range_tab;

  if ((unsigned) pass->static_pass_number >= tab->length ())
    tab->safe_grow_cleared (pass->static_pass_number + 1);

  if (!range_str)
    {
      uid_range_p slot;
      uid_range_p new_range = XCNEW (struct uid_range);

      new_range->start = 0;
      new_range->last = (unsigned)-1;

      slot = (*tab)[pass->static_pass_number];
      new_range->next = slot;
      (*tab)[pass->static_pass_number] = new_range;
      if (is_enable)
        inform (UNKNOWN_LOCATION, "enable pass %s for functions in the range "
                "of [%u, %u]", phase_name, new_range->start, new_range->last);
      else
        inform (UNKNOWN_LOCATION, "disable pass %s for functions in the range "
                "of [%u, %u]", phase_name, new_range->start, new_range->last);
    }
  else
    {
      char *next_range = NULL;
      char *one_range = range_str;
      char *end_val = NULL;

      do
	{
	  uid_range_p slot;
	  uid_range_p new_range;
	  char *invalid = NULL;
	  long start;
	  char *func_name = NULL;

	  next_range = strchr (one_range, ',');
	  if (next_range)
	    {
	      *next_range = '\0';
	      next_range++;
	    }

	  end_val = strchr (one_range, ':');
	  if (end_val)
	    {
	      *end_val = '\0';
	      end_val++;
	    }
	  start = strtol (one_range, &invalid, 10);
	  if (*invalid || start < 0)
	    {
              if (end_val || (one_range[0] >= '0'
			      && one_range[0] <= '9'))
                {
                  error ("Invalid range %s in option %s",
                         one_range,
                         is_enable ? "-fenable" : "-fdisable");
                  free (argstr);
                  return;
                }
	      func_name = one_range;
	    }
	  if (!end_val)
	    {
	      new_range = XCNEW (struct uid_range);
              if (!func_name)
                {
                  new_range->start = (unsigned) start;
                  new_range->last = (unsigned) start;
                }
              else
                {
                  new_range->start = (unsigned) -1;
                  new_range->last = (unsigned) -1;
                  new_range->assem_name = xstrdup (func_name);
                }
	    }
	  else
	    {
	      long last = strtol (end_val, &invalid, 10);
	      if (*invalid || last < start)
		{
		  error ("Invalid range %s in option %s",
			 end_val,
			 is_enable ? "-fenable" : "-fdisable");
		  free (argstr);
		  return;
		}
	      new_range = XCNEW (struct uid_range);
	      new_range->start = (unsigned) start;
	      new_range->last = (unsigned) last;
	    }

          slot = (*tab)[pass->static_pass_number];
          new_range->next = slot;
          (*tab)[pass->static_pass_number] = new_range;
          if (is_enable)
            {
              if (new_range->assem_name)
                inform (UNKNOWN_LOCATION,
                        "enable pass %s for function %s",
                        phase_name, new_range->assem_name);
              else
                inform (UNKNOWN_LOCATION,
                        "enable pass %s for functions in the range of [%u, %u]",
                        phase_name, new_range->start, new_range->last);
            }
          else
            {
              if (new_range->assem_name)
                inform (UNKNOWN_LOCATION,
                        "disable pass %s for function %s",
                        phase_name, new_range->assem_name);
              else
                inform (UNKNOWN_LOCATION,
                        "disable pass %s for functions in the range of [%u, %u]",
                        phase_name, new_range->start, new_range->last);
            }

	  one_range = next_range;
	} while (next_range);
    }

  free (argstr);
}

/* Enable pass specified by ARG.  */

void
enable_pass (const char *arg)
{
  enable_disable_pass (arg, true);
}

/* Disable pass specified by ARG.  */

void
disable_pass (const char *arg)
{
  enable_disable_pass (arg, false);
}

/* Returns true if PASS is explicitly enabled/disabled for FUNC.  */

static bool
is_pass_explicitly_enabled_or_disabled (struct opt_pass *pass,
					tree func,
					vec<uid_range_p> tab)
{
  uid_range_p slot, range;
  int cgraph_uid;
  const char *aname = NULL;

  if (!tab.exists ()
      || (unsigned) pass->static_pass_number >= tab.length ()
      || pass->static_pass_number == -1)
    return false;

  slot = tab[pass->static_pass_number];
  if (!slot)
    return false;

  cgraph_uid = func ? cgraph_get_node (func)->uid : 0;
  if (func && DECL_ASSEMBLER_NAME_SET_P (func))
    aname = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (func));

  range = slot;
  while (range)
    {
      if ((unsigned) cgraph_uid >= range->start
	  && (unsigned) cgraph_uid <= range->last)
	return true;
      if (range->assem_name && aname
          && !strcmp (range->assem_name, aname))
        return true;
      range = range->next;
    }

  return false;
}

/* Look at the static_pass_number and duplicate the pass
   if it is already added to a list. */

static struct opt_pass *
make_pass_instance (struct opt_pass *pass, bool track_duplicates)
{
  /* A nonzero static_pass_number indicates that the
     pass is already in the list.  */
  if (pass->static_pass_number)
    {
      struct opt_pass *new_pass;

      if (pass->type == GIMPLE_PASS
          || pass->type == RTL_PASS
          || pass->type == SIMPLE_IPA_PASS)
        {
          new_pass = XNEW (struct opt_pass);
          memcpy (new_pass, pass, sizeof (struct opt_pass));
        }
      else if (pass->type == IPA_PASS)
        {
          new_pass = (struct opt_pass *)XNEW (struct ipa_opt_pass_d);
          memcpy (new_pass, pass, sizeof (struct ipa_opt_pass_d));
        }
      else
        gcc_unreachable ();

      new_pass->next = NULL;

      new_pass->todo_flags_start &= ~TODO_mark_first_instance;

      /* Indicate to register_dump_files that this pass has duplicates,
         and so it should rename the dump file.  The first instance will
         be -1, and be number of duplicates = -static_pass_number - 1.
         Subsequent instances will be > 0 and just the duplicate number.  */
      if ((pass->name && pass->name[0] != '*') || track_duplicates)
        {
          pass->static_pass_number -= 1;
          new_pass->static_pass_number = -pass->static_pass_number;
	}
      return new_pass;
    }
  else
    {
      pass->todo_flags_start |= TODO_mark_first_instance;
      pass->static_pass_number = -1;

      invoke_plugin_callbacks (PLUGIN_NEW_PASS, pass);
    }
  return pass;
}

/* Add a pass to the pass list. Duplicate the pass if it's already
   in the list.  */

static struct opt_pass **
next_pass_1 (struct opt_pass **list, struct opt_pass *pass)
{
  /* Every pass should have a name so that plugins can refer to them.  */
  gcc_assert (pass->name != NULL);

  *list = make_pass_instance (pass, false);

  return &(*list)->next;
}

/* List node for an inserted pass instance. We need to keep track of all
   the newly-added pass instances (with 'added_pass_nodes' defined below)
   so that we can register their dump files after pass-positioning is finished.
   Registering dumping files needs to be post-processed or the
   static_pass_number of the opt_pass object would be modified and mess up
   the dump file names of future pass instances to be added.  */

struct pass_list_node
{
  struct opt_pass *pass;
  struct pass_list_node *next;
};

static struct pass_list_node *added_pass_nodes = NULL;
static struct pass_list_node *prev_added_pass_node;

/* Insert the pass at the proper position. Return true if the pass
   is successfully added.

   NEW_PASS_INFO - new pass to be inserted
   PASS_LIST - root of the pass list to insert the new pass to  */

static bool
position_pass (struct register_pass_info *new_pass_info,
               struct opt_pass **pass_list)
{
  struct opt_pass *pass = *pass_list, *prev_pass = NULL;
  bool success = false;

  for ( ; pass; prev_pass = pass, pass = pass->next)
    {
      /* Check if the current pass is of the same type as the new pass and
         matches the name and the instance number of the reference pass.  */
      if (pass->type == new_pass_info->pass->type
          && pass->name
          && !strcmp (pass->name, new_pass_info->reference_pass_name)
          && ((new_pass_info->ref_pass_instance_number == 0)
              || (new_pass_info->ref_pass_instance_number ==
                  pass->static_pass_number)
              || (new_pass_info->ref_pass_instance_number == 1
                  && pass->todo_flags_start & TODO_mark_first_instance)))
        {
          struct opt_pass *new_pass;
          struct pass_list_node *new_pass_node;

	  new_pass = make_pass_instance (new_pass_info->pass, true);

          /* Insert the new pass instance based on the positioning op.  */
          switch (new_pass_info->pos_op)
            {
              case PASS_POS_INSERT_AFTER:
                new_pass->next = pass->next;
                pass->next = new_pass;

		/* Skip newly inserted pass to avoid repeated
		   insertions in the case where the new pass and the
		   existing one have the same name.  */
                pass = new_pass;
                break;
              case PASS_POS_INSERT_BEFORE:
                new_pass->next = pass;
                if (prev_pass)
                  prev_pass->next = new_pass;
                else
                  *pass_list = new_pass;
                break;
              case PASS_POS_REPLACE:
                new_pass->next = pass->next;
                if (prev_pass)
                  prev_pass->next = new_pass;
                else
                  *pass_list = new_pass;
                new_pass->sub = pass->sub;
                new_pass->tv_id = pass->tv_id;
                pass = new_pass;
                break;
              default:
                error ("invalid pass positioning operation");
                return false;
            }

          /* Save the newly added pass (instance) in the added_pass_nodes
             list so that we can register its dump file later. Note that
             we cannot register the dump file now because doing so will modify
             the static_pass_number of the opt_pass object and therefore
             mess up the dump file name of future instances.  */
          new_pass_node = XCNEW (struct pass_list_node);
          new_pass_node->pass = new_pass;
          if (!added_pass_nodes)
            added_pass_nodes = new_pass_node;
          else
            prev_added_pass_node->next = new_pass_node;
          prev_added_pass_node = new_pass_node;

          success = true;
        }

      if (pass->sub && position_pass (new_pass_info, &pass->sub))
        success = true;
    }

  return success;
}

/* Hooks a new pass into the pass lists.

   PASS_INFO   - pass information that specifies the opt_pass object,
                 reference pass, instance number, and how to position
                 the pass  */

void
register_pass (struct register_pass_info *pass_info)
{
  bool all_instances, success;

  /* The checks below could fail in buggy plugins.  Existing GCC
     passes should never fail these checks, so we mention plugin in
     the messages.  */
  if (!pass_info->pass)
      fatal_error ("plugin cannot register a missing pass");

  if (!pass_info->pass->name)
      fatal_error ("plugin cannot register an unnamed pass");

  if (!pass_info->reference_pass_name)
      fatal_error
	("plugin cannot register pass %qs without reference pass name",
	 pass_info->pass->name);

  /* Try to insert the new pass to the pass lists.  We need to check
     all five lists as the reference pass could be in one (or all) of
     them.  */
  all_instances = pass_info->ref_pass_instance_number == 0;
  success = position_pass (pass_info, &all_lowering_passes);
  if (!success || all_instances)
    success |= position_pass (pass_info, &all_small_ipa_passes);
  if (!success || all_instances)
    success |= position_pass (pass_info, &all_regular_ipa_passes);
  if (!success || all_instances)
    success |= position_pass (pass_info, &all_lto_gen_passes);
  if (!success || all_instances)
    success |= position_pass (pass_info, &all_late_ipa_passes);
  if (!success || all_instances)
    success |= position_pass (pass_info, &all_passes);
  if (!success)
    fatal_error
      ("pass %qs not found but is referenced by new pass %qs",
       pass_info->reference_pass_name, pass_info->pass->name);

  /* OK, we have successfully inserted the new pass. We need to register
     the dump files for the newly added pass and its duplicates (if any).
     Because the registration of plugin/backend passes happens after the
     command-line options are parsed, the options that specify single
     pass dumping (e.g. -fdump-tree-PASSNAME) cannot be used for new
     passes. Therefore we currently can only enable dumping of
     new passes when the 'dump-all' flags (e.g. -fdump-tree-all)
     are specified. While doing so, we also delete the pass_list_node
     objects created during pass positioning.  */
  while (added_pass_nodes)
    {
      struct pass_list_node *next_node = added_pass_nodes->next;
      enum tree_dump_index tdi;
      register_one_dump_file (added_pass_nodes->pass);
      if (added_pass_nodes->pass->type == SIMPLE_IPA_PASS
          || added_pass_nodes->pass->type == IPA_PASS)
        tdi = TDI_ipa_all;
      else if (added_pass_nodes->pass->type == GIMPLE_PASS)
        tdi = TDI_tree_all;
      else
        tdi = TDI_rtl_all;
      /* Check if dump-all flag is specified.  */
      if (get_dump_file_info (tdi)->pstate)
        get_dump_file_info (added_pass_nodes->pass->static_pass_number)
            ->pstate = get_dump_file_info (tdi)->pstate;
      XDELETE (added_pass_nodes);
      added_pass_nodes = next_node;
    }
}

/* Construct the pass tree.  The sequencing of passes is driven by
   the cgraph routines:

   finalize_compilation_unit ()
       for each node N in the cgraph
	   cgraph_analyze_function (N)
	       cgraph_lower_function (N) -> all_lowering_passes

   If we are optimizing, compile is then invoked:

   compile ()
       ipa_passes () 			-> all_small_ipa_passes
					-> Analysis of all_regular_ipa_passes
	* possible LTO streaming at copmilation time *
					-> Execution of all_regular_ipa_passes
	* possible LTO streaming at link time *
					-> all_late_ipa_passes
       expand_all_functions ()
           for each node N in the cgraph
	       expand_function (N)      -> Transformation of all_regular_ipa_passes
				        -> all_passes
*/

void
init_optimization_passes (void)
{
  struct opt_pass **p;

#define NEXT_PASS(PASS)  (p = next_pass_1 (p, &((PASS).pass)))

 /* All passes needed to lower the function into shape optimizers can
    operate on.  These passes are always run first on the function, but
    backend might produce already lowered functions that are not processed
    by these passes.  */
  p = &all_lowering_passes;
  NEXT_PASS (pass_warn_unused_result);
  NEXT_PASS (pass_diagnose_omp_blocks);
  NEXT_PASS (pass_diagnose_tm_blocks);
  NEXT_PASS (pass_mudflap_1);
  NEXT_PASS (pass_lower_omp);
  NEXT_PASS (pass_lower_cf);
  NEXT_PASS (pass_lower_tm);
  NEXT_PASS (pass_refactor_eh);
  NEXT_PASS (pass_lower_eh);
  NEXT_PASS (pass_build_cfg);
  NEXT_PASS (pass_warn_function_return);
  NEXT_PASS (pass_build_cgraph_edges);
  *p = NULL;

  /* Interprocedural optimization passes.  */
  p = &all_small_ipa_passes;
  NEXT_PASS (pass_ipa_free_lang_data);
  NEXT_PASS (pass_ipa_function_and_variable_visibility);
  NEXT_PASS (pass_early_local_passes);
    {
      struct opt_pass **p = &pass_early_local_passes.pass.sub;
      NEXT_PASS (pass_fixup_cfg);
      NEXT_PASS (pass_init_datastructures);
      NEXT_PASS (pass_expand_omp);

      NEXT_PASS (pass_build_ssa);
      NEXT_PASS (pass_lower_vector);
      NEXT_PASS (pass_early_warn_uninitialized);
      NEXT_PASS (pass_rebuild_cgraph_edges);
      NEXT_PASS (pass_inline_parameters);
      NEXT_PASS (pass_early_inline);
      NEXT_PASS (pass_all_early_optimizations);
	{
	  struct opt_pass **p = &pass_all_early_optimizations.pass.sub;
	  NEXT_PASS (pass_remove_cgraph_callee_edges);
	  NEXT_PASS (pass_rename_ssa_copies);
	  NEXT_PASS (pass_ccp);
	  /* After CCP we rewrite no longer addressed locals into SSA
	     form if possible.  */
	  NEXT_PASS (pass_forwprop);
	  /* pass_build_ealias is a dummy pass that ensures that we
	     execute TODO_rebuild_alias at this point.  */
	  NEXT_PASS (pass_build_ealias);
	  NEXT_PASS (pass_sra_early);
	  NEXT_PASS (pass_fre);
	  NEXT_PASS (pass_copy_prop);
	  NEXT_PASS (pass_merge_phi);
	  NEXT_PASS (pass_cd_dce);
	  NEXT_PASS (pass_early_ipa_sra);
	  NEXT_PASS (pass_tail_recursion);
	  NEXT_PASS (pass_convert_switch);
          NEXT_PASS (pass_cleanup_eh);
          NEXT_PASS (pass_profile);
          NEXT_PASS (pass_local_pure_const);
	  /* Split functions creates parts that are not run through
	     early optimizations again.  It is thus good idea to do this
	     late.  */
          NEXT_PASS (pass_split_functions);
	}
      NEXT_PASS (pass_release_ssa_names);
      NEXT_PASS (pass_rebuild_cgraph_edges);
      NEXT_PASS (pass_inline_parameters);
    }
  NEXT_PASS (pass_ipa_free_inline_summary);
  NEXT_PASS (pass_ipa_tree_profile);
    {
      struct opt_pass **p = &pass_ipa_tree_profile.pass.sub;
      NEXT_PASS (pass_feedback_split_functions);
    }
  NEXT_PASS (pass_ipa_increase_alignment);
  NEXT_PASS (pass_ipa_tm);
  NEXT_PASS (pass_ipa_lower_emutls);
  *p = NULL;

  p = &all_regular_ipa_passes;
  NEXT_PASS (pass_ipa_whole_program_visibility);
  NEXT_PASS (pass_ipa_profile);
  NEXT_PASS (pass_ipa_cp);
  NEXT_PASS (pass_ipa_cdtor_merge);
  NEXT_PASS (pass_ipa_inline);
  NEXT_PASS (pass_ipa_pure_const);
  NEXT_PASS (pass_ipa_reference);
  *p = NULL;

  p = &all_lto_gen_passes;
  NEXT_PASS (pass_ipa_lto_gimple_out);
  NEXT_PASS (pass_ipa_lto_finish_out);  /* This must be the last LTO pass.  */
  *p = NULL;

  /* Simple IPA passes executed after the regular passes.  In WHOPR mode the
     passes are executed after partitioning and thus see just parts of the
     compiled unit.  */
  p = &all_late_ipa_passes;
  NEXT_PASS (pass_ipa_pta);
  *p = NULL;

  /* These passes are run after IPA passes on every function that is being
     output to the assembler file.  */
  p = &all_passes;
  NEXT_PASS (pass_fixup_cfg);
  NEXT_PASS (pass_lower_eh_dispatch);
  NEXT_PASS (pass_all_optimizations);
    {
      struct opt_pass **p = &pass_all_optimizations.pass.sub;
      NEXT_PASS (pass_remove_cgraph_callee_edges);
      /* Initial scalar cleanups before alias computation.
	 They ensure memory accesses are not indirect wherever possible.  */
      NEXT_PASS (pass_strip_predict_hints);
      NEXT_PASS (pass_rename_ssa_copies);
      NEXT_PASS (pass_complete_unrolli);
      NEXT_PASS (pass_ccp);
      /* After CCP we rewrite no longer addressed locals into SSA
	 form if possible.  */
      NEXT_PASS (pass_forwprop);
      NEXT_PASS (pass_object_sizes);
      /* pass_build_alias is a dummy pass that ensures that we
	 execute TODO_rebuild_alias at this point.  */
      NEXT_PASS (pass_build_alias);
      NEXT_PASS (pass_return_slot);
      NEXT_PASS (pass_phiprop);
      NEXT_PASS (pass_fre);
      NEXT_PASS (pass_copy_prop);
      NEXT_PASS (pass_merge_phi);
      NEXT_PASS (pass_vrp);
      NEXT_PASS (pass_dce);
      NEXT_PASS (pass_call_cdce);
      NEXT_PASS (pass_cselim);
      NEXT_PASS (pass_tree_ifcombine);
      NEXT_PASS (pass_phiopt);
      NEXT_PASS (pass_tail_recursion);
      NEXT_PASS (pass_ch);
      NEXT_PASS (pass_stdarg);
      NEXT_PASS (pass_lower_complex);
      NEXT_PASS (pass_sra);
      NEXT_PASS (pass_rename_ssa_copies);
      /* The dom pass will also resolve all __builtin_constant_p calls
         that are still there to 0.  This has to be done after some
	 propagations have already run, but before some more dead code
	 is removed, and this place fits nicely.  Remember this when
	 trying to move or duplicate pass_dominator somewhere earlier.  */
      NEXT_PASS (pass_dominator);
      /* The only const/copy propagation opportunities left after
	 DOM should be due to degenerate PHI nodes.  So rather than
	 run the full propagators, run a specialized pass which
	 only examines PHIs to discover const/copy propagation
	 opportunities.  */
      NEXT_PASS (pass_phi_only_cprop);
      NEXT_PASS (pass_dse);
      NEXT_PASS (pass_reassoc);
      NEXT_PASS (pass_dce);
      NEXT_PASS (pass_forwprop);
      NEXT_PASS (pass_phiopt);
      NEXT_PASS (pass_strlen);
      NEXT_PASS (pass_ccp);
      /* After CCP we rewrite no longer addressed locals into SSA
	 form if possible.  */
      NEXT_PASS (pass_copy_prop);
      NEXT_PASS (pass_cse_sincos);
      NEXT_PASS (pass_optimize_bswap);
      NEXT_PASS (pass_split_crit_edges);
      NEXT_PASS (pass_pre);
      NEXT_PASS (pass_sink_code);
      NEXT_PASS (pass_asan);
      NEXT_PASS (pass_tsan);
      NEXT_PASS (pass_tree_loop);
	{
	  struct opt_pass **p = &pass_tree_loop.pass.sub;
	  NEXT_PASS (pass_tree_loop_init);
	  NEXT_PASS (pass_lim);
	  NEXT_PASS (pass_copy_prop);
	  NEXT_PASS (pass_dce_loop);
	  NEXT_PASS (pass_tree_unswitch);
	  NEXT_PASS (pass_scev_cprop);
	  NEXT_PASS (pass_record_bounds);
	  NEXT_PASS (pass_check_data_deps);
	  NEXT_PASS (pass_loop_distribution);
	  NEXT_PASS (pass_copy_prop);
	  NEXT_PASS (pass_graphite);
	    {
	      struct opt_pass **p = &pass_graphite.pass.sub;
	      NEXT_PASS (pass_graphite_transforms);
	      NEXT_PASS (pass_lim);
	      NEXT_PASS (pass_copy_prop);
	      NEXT_PASS (pass_dce_loop);
	    }
	  NEXT_PASS (pass_iv_canon);
	  NEXT_PASS (pass_if_conversion);
	  NEXT_PASS (pass_vectorize);
	    {
	      struct opt_pass **p = &pass_vectorize.pass.sub;
	      NEXT_PASS (pass_dce_loop);
	    }
          NEXT_PASS (pass_predcom);
	  NEXT_PASS (pass_complete_unroll);
	  NEXT_PASS (pass_slp_vectorize);
	  NEXT_PASS (pass_parallelize_loops);
	  NEXT_PASS (pass_loop_prefetch);
	  NEXT_PASS (pass_iv_optimize);
	  NEXT_PASS (pass_lim);
	  NEXT_PASS (pass_tree_loop_done);
	}
      NEXT_PASS (pass_lower_vector_ssa);
      NEXT_PASS (pass_cse_reciprocals);
      NEXT_PASS (pass_reassoc);
      NEXT_PASS (pass_vrp);
      NEXT_PASS (pass_strength_reduction);
      NEXT_PASS (pass_dominator);
      /* The only const/copy propagation opportunities left after
	 DOM should be due to degenerate PHI nodes.  So rather than
	 run the full propagators, run a specialized pass which
	 only examines PHIs to discover const/copy propagation
	 opportunities.  */
      NEXT_PASS (pass_phi_only_cprop);
      NEXT_PASS (pass_cd_dce);
      NEXT_PASS (pass_tracer);

      /* FIXME: If DCE is not run before checking for uninitialized uses,
	 we may get false warnings (e.g., testsuite/gcc.dg/uninit-5.c).
	 However, this also causes us to misdiagnose cases that should be
	 real warnings (e.g., testsuite/gcc.dg/pr18501.c).

	 To fix the false positives in uninit-5.c, we would have to
	 account for the predicates protecting the set and the use of each
	 variable.  Using a representation like Gated Single Assignment
	 may help.  */
      NEXT_PASS (pass_late_warn_uninitialized);
      NEXT_PASS (pass_dse);
      NEXT_PASS (pass_forwprop);
      NEXT_PASS (pass_phiopt);
      NEXT_PASS (pass_fold_builtins);
      NEXT_PASS (pass_optimize_widening_mul);
      NEXT_PASS (pass_tail_calls);
      NEXT_PASS (pass_rename_ssa_copies);
      NEXT_PASS (pass_uncprop);
      NEXT_PASS (pass_local_pure_const);
    }
  NEXT_PASS (pass_all_optimizations_g);
    {
      struct opt_pass **p = &pass_all_optimizations_g.pass.sub;
      NEXT_PASS (pass_remove_cgraph_callee_edges);
      NEXT_PASS (pass_strip_predict_hints);
      /* Lower remaining pieces of GIMPLE.  */
      NEXT_PASS (pass_lower_complex);
      NEXT_PASS (pass_lower_vector_ssa);
      /* Perform simple scalar cleanup which is constant/copy propagation.  */
      NEXT_PASS (pass_ccp);
      NEXT_PASS (pass_object_sizes);
      /* Fold remaining builtins.  */
      NEXT_PASS (pass_fold_builtins);
      /* Copy propagation also copy-propagates constants, this is necessary
         to forward object-size and builtin folding results properly.  */
      NEXT_PASS (pass_copy_prop);
      NEXT_PASS (pass_dce);
      NEXT_PASS (pass_asan);
      NEXT_PASS (pass_tsan);
      NEXT_PASS (pass_rename_ssa_copies);
      /* ???  We do want some kind of loop invariant motion, but we possibly
         need to adjust LIM to be more friendly towards preserving accurate
	 debug information here.  */
      /* Split critical edges before late uninit warning to reduce the
         number of false positives from it.  */
      NEXT_PASS (pass_split_crit_edges);
      NEXT_PASS (pass_late_warn_uninitialized);
      NEXT_PASS (pass_uncprop);
      NEXT_PASS (pass_local_pure_const);
    }
  NEXT_PASS (pass_tm_init);
    {
      struct opt_pass **p = &pass_tm_init.pass.sub;
      NEXT_PASS (pass_tm_mark);
      NEXT_PASS (pass_tm_memopt);
      NEXT_PASS (pass_tm_edges);
    }
  NEXT_PASS (pass_lower_complex_O0);
  NEXT_PASS (pass_asan_O0);
  NEXT_PASS (pass_tsan_O0);
  NEXT_PASS (pass_cleanup_eh);
  NEXT_PASS (pass_lower_resx);
  NEXT_PASS (pass_nrv);
  NEXT_PASS (pass_mudflap_2);
  NEXT_PASS (pass_cleanup_cfg_post_optimizing);
  NEXT_PASS (pass_warn_function_noreturn);

  NEXT_PASS (pass_expand);

  NEXT_PASS (pass_rest_of_compilation);
    {
      struct opt_pass **p = &pass_rest_of_compilation.pass.sub;
      NEXT_PASS (pass_instantiate_virtual_regs);
      NEXT_PASS (pass_into_cfg_layout_mode);
      NEXT_PASS (pass_jump);
      NEXT_PASS (pass_lower_subreg);
      NEXT_PASS (pass_df_initialize_opt);
      NEXT_PASS (pass_cse);
      NEXT_PASS (pass_rtl_fwprop);
      NEXT_PASS (pass_rtl_cprop);
      NEXT_PASS (pass_rtl_pre);
      NEXT_PASS (pass_rtl_hoist);
      NEXT_PASS (pass_rtl_cprop);
      NEXT_PASS (pass_rtl_store_motion);
      NEXT_PASS (pass_cse_after_global_opts);
      NEXT_PASS (pass_rtl_ifcvt);
      NEXT_PASS (pass_reginfo_init);
      /* Perform loop optimizations.  It might be better to do them a bit
	 sooner, but we want the profile feedback to work more
	 efficiently.  */
      NEXT_PASS (pass_loop2);
	{
	  struct opt_pass **p = &pass_loop2.pass.sub;
	  NEXT_PASS (pass_rtl_loop_init);
	  NEXT_PASS (pass_rtl_move_loop_invariants);
	  NEXT_PASS (pass_rtl_unswitch);
	  NEXT_PASS (pass_rtl_unroll_and_peel_loops);
	  NEXT_PASS (pass_rtl_doloop);
	  NEXT_PASS (pass_rtl_loop_done);
	  *p = NULL;
	}
      NEXT_PASS (pass_web);
      NEXT_PASS (pass_rtl_cprop);
      NEXT_PASS (pass_cse2);
      NEXT_PASS (pass_rtl_dse1);
      NEXT_PASS (pass_rtl_fwprop_addr);
      NEXT_PASS (pass_inc_dec);
      NEXT_PASS (pass_initialize_regs);
      NEXT_PASS (pass_ud_rtl_dce);
      NEXT_PASS (pass_combine);
      NEXT_PASS (pass_if_after_combine);
      NEXT_PASS (pass_partition_blocks);
      NEXT_PASS (pass_regmove);
      NEXT_PASS (pass_outof_cfg_layout_mode);
      NEXT_PASS (pass_split_all_insns);
      NEXT_PASS (pass_lower_subreg2);
      NEXT_PASS (pass_df_initialize_no_opt);
      NEXT_PASS (pass_stack_ptr_mod);
      NEXT_PASS (pass_mode_switching);
      NEXT_PASS (pass_match_asm_constraints);
      NEXT_PASS (pass_sms);
      NEXT_PASS (pass_sched);
      NEXT_PASS (pass_ira);
      NEXT_PASS (pass_reload);
      NEXT_PASS (pass_postreload);
	{
	  struct opt_pass **p = &pass_postreload.pass.sub;
	  NEXT_PASS (pass_postreload_cse);
	  NEXT_PASS (pass_gcse2);
	  NEXT_PASS (pass_split_after_reload);
	  NEXT_PASS (pass_ree);
	  NEXT_PASS (pass_compare_elim_after_reload);
	  NEXT_PASS (pass_branch_target_load_optimize1);
	  NEXT_PASS (pass_thread_prologue_and_epilogue);
	  NEXT_PASS (pass_rtl_dse2);
	  NEXT_PASS (pass_stack_adjustments);
	  NEXT_PASS (pass_jump2);
	  NEXT_PASS (pass_peephole2);
	  NEXT_PASS (pass_if_after_reload);
	  NEXT_PASS (pass_regrename);
	  NEXT_PASS (pass_cprop_hardreg);
	  NEXT_PASS (pass_fast_rtl_dce);
	  NEXT_PASS (pass_reorder_blocks);
	  NEXT_PASS (pass_branch_target_load_optimize2);
	  NEXT_PASS (pass_leaf_regs);
	  NEXT_PASS (pass_split_before_sched2);
	  NEXT_PASS (pass_sched2);
	  NEXT_PASS (pass_stack_regs);
	    {
	      struct opt_pass **p = &pass_stack_regs.pass.sub;
	      NEXT_PASS (pass_split_before_regstack);
	      NEXT_PASS (pass_stack_regs_run);
	    }
	  NEXT_PASS (pass_compute_alignments);
	  NEXT_PASS (pass_duplicate_computed_gotos);
	  NEXT_PASS (pass_variable_tracking);
	  NEXT_PASS (pass_free_cfg);
	  NEXT_PASS (pass_machine_reorg);
	  NEXT_PASS (pass_cleanup_barriers);
	  NEXT_PASS (pass_delay_slots);
	  NEXT_PASS (pass_split_for_shorten_branches);
	  NEXT_PASS (pass_convert_to_eh_region_ranges);
	  NEXT_PASS (pass_shorten_branches);
	  NEXT_PASS (pass_set_nothrow_function_flags);
	  NEXT_PASS (pass_dwarf2_frame);
	  NEXT_PASS (pass_final);
	}
      NEXT_PASS (pass_df_finish);
    }
  NEXT_PASS (pass_clean_state);
  *p = NULL;

#undef NEXT_PASS

  /* Register the passes with the tree dump code.  */
  register_dump_files (all_lowering_passes, PROP_gimple_any);
  register_dump_files (all_small_ipa_passes,
		       PROP_gimple_any | PROP_gimple_lcf | PROP_gimple_leh
		       | PROP_cfg);
  register_dump_files (all_regular_ipa_passes,
		       PROP_gimple_any | PROP_gimple_lcf | PROP_gimple_leh
		       | PROP_cfg);
  register_dump_files (all_lto_gen_passes,
		       PROP_gimple_any | PROP_gimple_lcf | PROP_gimple_leh
		       | PROP_cfg);
  register_dump_files (all_late_ipa_passes,
		       PROP_gimple_any | PROP_gimple_lcf | PROP_gimple_leh
		       | PROP_cfg);
  register_dump_files (all_passes,
		       PROP_gimple_any | PROP_gimple_lcf | PROP_gimple_leh
		       | PROP_cfg);
}

/* If we are in IPA mode (i.e., current_function_decl is NULL), call
   function CALLBACK for every function in the call graph.  Otherwise,
   call CALLBACK on the current function.  */

static void
do_per_function (void (*callback) (void *data), void *data)
{
  if (current_function_decl)
    callback (data);
  else
    {
      struct cgraph_node *node;
      FOR_EACH_DEFINED_FUNCTION (node)
	if (gimple_has_body_p (node->symbol.decl)
	    && (!node->clone_of || node->symbol.decl != node->clone_of->symbol.decl))
	  {
	    push_cfun (DECL_STRUCT_FUNCTION (node->symbol.decl));
	    callback (data);
	    if (!flag_wpa)
	      {
	        free_dominance_info (CDI_DOMINATORS);
	        free_dominance_info (CDI_POST_DOMINATORS);
	      }
	    pop_cfun ();
	    ggc_collect ();
	  }
    }
}

/* Because inlining might remove no-longer reachable nodes, we need to
   keep the array visible to garbage collector to avoid reading collected
   out nodes.  */
static int nnodes;
static GTY ((length ("nnodes"))) cgraph_node_ptr *order;

/* If we are in IPA mode (i.e., current_function_decl is NULL), call
   function CALLBACK for every function in the call graph.  Otherwise,
   call CALLBACK on the current function.
   This function is global so that plugins can use it.  */
void
do_per_function_toporder (void (*callback) (void *data), void *data)
{
  int i;

  if (current_function_decl)
    callback (data);
  else
    {
      gcc_assert (!order);
      order = ggc_alloc_vec_cgraph_node_ptr (cgraph_n_nodes);
      nnodes = ipa_reverse_postorder (order);
      for (i = nnodes - 1; i >= 0; i--)
        order[i]->process = 1;
      for (i = nnodes - 1; i >= 0; i--)
	{
	  struct cgraph_node *node = order[i];

	  /* Allow possibly removed nodes to be garbage collected.  */
	  order[i] = NULL;
	  node->process = 0;
	  if (cgraph_function_with_gimple_body_p (node))
	    {
	      push_cfun (DECL_STRUCT_FUNCTION (node->symbol.decl));
	      callback (data);
	      free_dominance_info (CDI_DOMINATORS);
	      free_dominance_info (CDI_POST_DOMINATORS);
	      pop_cfun ();
	      ggc_collect ();
	    }
	}
    }
  ggc_free (order);
  order = NULL;
  nnodes = 0;
}

/* Helper function to perform function body dump.  */

static void
execute_function_dump (void *data ATTRIBUTE_UNUSED)
{
  if (dump_file && current_function_decl)
    {
      if (cfun->curr_properties & PROP_trees)
        dump_function_to_file (current_function_decl, dump_file, dump_flags);
      else
	print_rtl_with_bb (dump_file, get_insns (), dump_flags);

      /* Flush the file.  If verification fails, we won't be able to
	 close the file before aborting.  */
      fflush (dump_file);

      if ((cfun->curr_properties & PROP_cfg)
	  && (dump_flags & TDF_GRAPH))
	print_graph_cfg (dump_file_name, cfun);
    }
}

static struct profile_record *profile_record;

/* Do profile consistency book-keeping for the pass with static number INDEX.
   If SUBPASS is zero, we run _before_ the pass, and if SUBPASS is one, then
   we run _after_ the pass.  RUN is true if the pass really runs, or FALSE
   if we are only book-keeping on passes that may have selectively disabled
   themselves on a given function.  */
static void
check_profile_consistency (int index, int subpass, bool run)
{
  if (index == -1)
    return;
  if (!profile_record)
    profile_record = XCNEWVEC (struct profile_record,
			       passes_by_id_size);
  gcc_assert (index < passes_by_id_size && index >= 0);
  gcc_assert (subpass < 2);
  profile_record[index].run |= run;
  account_profile_record (&profile_record[index], subpass);
}

/* Output profile consistency.  */

void
dump_profile_report (void)
{
  int i, j;
  int last_freq_in = 0, last_count_in = 0, last_freq_out = 0, last_count_out = 0;
  gcov_type last_time = 0, last_size = 0;
  double rel_time_change, rel_size_change;
  int last_reported = 0;

  if (!profile_record)
    return;
  fprintf (stderr, "\nProfile consistency report:\n\n");
  fprintf (stderr, "Pass name                        |mismatch in |mismated out|Overall\n");
  fprintf (stderr, "                                 |freq count  |freq count  |size      time\n");
	   
  for (i = 0; i < passes_by_id_size; i++)
    for (j = 0 ; j < 2; j++)
      if (profile_record[i].run)
	{
	  if (last_time)
	    rel_time_change = (profile_record[i].time[j]
			       - (double)last_time) * 100 / (double)last_time;
	  else
	    rel_time_change = 0;
	  if (last_size)
	    rel_size_change = (profile_record[i].size[j]
			       - (double)last_size) * 100 / (double)last_size;
	  else
	    rel_size_change = 0;

	  if (profile_record[i].num_mismatched_freq_in[j] != last_freq_in
	      || profile_record[i].num_mismatched_freq_out[j] != last_freq_out
	      || profile_record[i].num_mismatched_count_in[j] != last_count_in
	      || profile_record[i].num_mismatched_count_out[j] != last_count_out
	      || rel_time_change || rel_size_change)
	    {
	      last_reported = i;
              fprintf (stderr, "%-20s %s",
		       passes_by_id [i]->name,
		       j ? "(after TODO)" : "            ");
	      if (profile_record[i].num_mismatched_freq_in[j] != last_freq_in)
		fprintf (stderr, "| %+5i",
		         profile_record[i].num_mismatched_freq_in[j]
			  - last_freq_in);
	      else
		fprintf (stderr, "|      ");
	      if (profile_record[i].num_mismatched_count_in[j] != last_count_in)
		fprintf (stderr, " %+5i",
		         profile_record[i].num_mismatched_count_in[j]
			  - last_count_in);
	      else
		fprintf (stderr, "      ");
	      if (profile_record[i].num_mismatched_freq_out[j] != last_freq_out)
		fprintf (stderr, "| %+5i",
		         profile_record[i].num_mismatched_freq_out[j]
			  - last_freq_out);
	      else
		fprintf (stderr, "|      ");
	      if (profile_record[i].num_mismatched_count_out[j] != last_count_out)
		fprintf (stderr, " %+5i",
		         profile_record[i].num_mismatched_count_out[j]
			  - last_count_out);
	      else
		fprintf (stderr, "      ");

	      /* Size/time units change across gimple and RTL.  */
	      if (i == pass_expand.pass.static_pass_number)
		fprintf (stderr, "|----------");
	      else
		{
		  if (rel_size_change)
		    fprintf (stderr, "| %+8.4f%%", rel_size_change);
		  else
		    fprintf (stderr, "|          ");
		  if (rel_time_change)
		    fprintf (stderr, " %+8.4f%%", rel_time_change);
		}
	      fprintf (stderr, "\n");
	      last_freq_in = profile_record[i].num_mismatched_freq_in[j];
	      last_freq_out = profile_record[i].num_mismatched_freq_out[j];
	      last_count_in = profile_record[i].num_mismatched_count_in[j];
	      last_count_out = profile_record[i].num_mismatched_count_out[j];
	    }
	  else if (j && last_reported != i)
	    {
	      last_reported = i;
              fprintf (stderr, "%-20s ------------|            |            |\n",
		       passes_by_id [i]->name);
	    }
	  last_time = profile_record[i].time[j];
	  last_size = profile_record[i].size[j];
	}
}

/* Perform all TODO actions that ought to be done on each function.  */

static void
execute_function_todo (void *data)
{
  unsigned int flags = (size_t)data;
  flags &= ~cfun->last_verified;
  if (!flags)
    return;

  /* Always cleanup the CFG before trying to update SSA.  */
  if (flags & TODO_cleanup_cfg)
    {
      cleanup_tree_cfg ();

      /* When cleanup_tree_cfg merges consecutive blocks, it may
	 perform some simplistic propagation when removing single
	 valued PHI nodes.  This propagation may, in turn, cause the
	 SSA form to become out-of-date (see PR 22037).  So, even
	 if the parent pass had not scheduled an SSA update, we may
	 still need to do one.  */
      if (!(flags & TODO_update_ssa_any) && need_ssa_update_p (cfun))
	flags |= TODO_update_ssa;
    }

  if (flags & TODO_update_ssa_any)
    {
      unsigned update_flags = flags & TODO_update_ssa_any;
      update_ssa (update_flags);
      cfun->last_verified &= ~TODO_verify_ssa;
    }

  if (flag_tree_pta && (flags & TODO_rebuild_alias))
    compute_may_aliases ();

  if (optimize && (flags & TODO_update_address_taken))
    execute_update_addresses_taken ();

  if (flags & TODO_remove_unused_locals)
    remove_unused_locals ();

  if (flags & TODO_rebuild_frequencies)
    rebuild_frequencies ();

  if (flags & TODO_rebuild_cgraph_edges)
    rebuild_cgraph_edges ();

  /* If we've seen errors do not bother running any verifiers.  */
  if (seen_error ())
    return;

#if defined ENABLE_CHECKING
  if (flags & TODO_verify_ssa
      || (current_loops && loops_state_satisfies_p (LOOP_CLOSED_SSA)))
    {
      verify_gimple_in_cfg (cfun);
      verify_ssa (true);
    }
  else if (flags & TODO_verify_stmts)
    verify_gimple_in_cfg (cfun);
  if (flags & TODO_verify_flow)
    verify_flow_info ();
  if (current_loops && loops_state_satisfies_p (LOOP_CLOSED_SSA))
    verify_loop_closed_ssa (false);
  if (flags & TODO_verify_rtl_sharing)
    verify_rtl_sharing ();
#endif

  cfun->last_verified = flags & TODO_verify_all;
}

/* Perform all TODO actions.  */
static void
execute_todo (unsigned int flags)
{
#if defined ENABLE_CHECKING
  if (cfun
      && need_ssa_update_p (cfun))
    gcc_assert (flags & TODO_update_ssa_any);
#endif

  timevar_push (TV_TODO);

  /* Inform the pass whether it is the first time it is run.  */
  first_pass_instance = (flags & TODO_mark_first_instance) != 0;

  statistics_fini_pass ();

  do_per_function (execute_function_todo, (void *)(size_t) flags);

  /* Always remove functions just as before inlining: IPA passes might be
     interested to see bodies of extern inline functions that are not inlined
     to analyze side effects.  The full removal is done just at the end
     of IPA pass queue.  */
  if (flags & TODO_remove_functions)
    {
      gcc_assert (!cfun);
      symtab_remove_unreachable_nodes (true, dump_file);
    }

  if ((flags & TODO_dump_symtab) && dump_file && !current_function_decl)
    {
      gcc_assert (!cfun);
      dump_symtab (dump_file);
      /* Flush the file.  If verification fails, we won't be able to
	 close the file before aborting.  */
      fflush (dump_file);
    }

  if (flags & TODO_ggc_collect)
    ggc_collect ();

  /* Now that the dumping has been done, we can get rid of the optional
     df problems.  */
  if (flags & TODO_df_finish)
    df_finish_pass ((flags & TODO_df_verify) != 0);

  timevar_pop (TV_TODO);
}

/* Verify invariants that should hold between passes.  This is a place
   to put simple sanity checks.  */

static void
verify_interpass_invariants (void)
{
  gcc_checking_assert (!fold_deferring_overflow_warnings_p ());
}

/* Clear the last verified flag.  */

static void
clear_last_verified (void *data ATTRIBUTE_UNUSED)
{
  cfun->last_verified = 0;
}

/* Helper function. Verify that the properties has been turn into the
   properties expected by the pass.  */

#ifdef ENABLE_CHECKING
static void
verify_curr_properties (void *data)
{
  unsigned int props = (size_t)data;
  gcc_assert ((cfun->curr_properties & props) == props);
}
#endif

/* Initialize pass dump file.  */
/* This is non-static so that the plugins can use it.  */

bool
pass_init_dump_file (struct opt_pass *pass)
{
  /* If a dump file name is present, open it if enabled.  */
  if (pass->static_pass_number != -1)
    {
      timevar_push (TV_DUMP);
      bool initializing_dump = !dump_initialized_p (pass->static_pass_number);
      dump_file_name = get_dump_file_name (pass->static_pass_number);
      dump_start (pass->static_pass_number, &dump_flags);
      if (dump_file && current_function_decl)
        dump_function_header (dump_file, current_function_decl, dump_flags);
      if (initializing_dump
	  && dump_file && (dump_flags & TDF_GRAPH)
	  && cfun && (cfun->curr_properties & PROP_cfg))
	clean_graph_dump_file (dump_file_name);
      timevar_pop (TV_DUMP);
      return initializing_dump;
    }
  else
    return false;
}

/* Flush PASS dump file.  */
/* This is non-static so that plugins can use it.  */

void
pass_fini_dump_file (struct opt_pass *pass)
{
  timevar_push (TV_DUMP);

  /* Flush and close dump file.  */
  if (dump_file_name)
    {
      free (CONST_CAST (char *, dump_file_name));
      dump_file_name = NULL;
    }

  dump_finish (pass->static_pass_number);
  timevar_pop (TV_DUMP);
}

/* After executing the pass, apply expected changes to the function
   properties. */

static void
update_properties_after_pass (void *data)
{
  struct opt_pass *pass = (struct opt_pass *) data;
  cfun->curr_properties = (cfun->curr_properties | pass->properties_provided)
		           & ~pass->properties_destroyed;
}

/* Execute summary generation for all of the passes in IPA_PASS.  */

void
execute_ipa_summary_passes (struct ipa_opt_pass_d *ipa_pass)
{
  while (ipa_pass)
    {
      struct opt_pass *pass = &ipa_pass->pass;

      /* Execute all of the IPA_PASSes in the list.  */
      if (ipa_pass->pass.type == IPA_PASS
	  && (!pass->gate || pass->gate ())
	  && ipa_pass->generate_summary)
	{
	  pass_init_dump_file (pass);

	  /* If a timevar is present, start it.  */
	  if (pass->tv_id)
	    timevar_push (pass->tv_id);

	  ipa_pass->generate_summary ();

	  /* Stop timevar.  */
	  if (pass->tv_id)
	    timevar_pop (pass->tv_id);

	  pass_fini_dump_file (pass);
	}
      ipa_pass = (struct ipa_opt_pass_d *)ipa_pass->pass.next;
    }
}

/* Execute IPA_PASS function transform on NODE.  */

static void
execute_one_ipa_transform_pass (struct cgraph_node *node,
				struct ipa_opt_pass_d *ipa_pass)
{
  struct opt_pass *pass = &ipa_pass->pass;
  unsigned int todo_after = 0;

  current_pass = pass;
  if (!ipa_pass->function_transform)
    return;

  /* Note that the folders should only create gimple expressions.
     This is a hack until the new folder is ready.  */
  in_gimple_form = (cfun && (cfun->curr_properties & PROP_trees)) != 0;

  pass_init_dump_file (pass);

  /* Run pre-pass verification.  */
  execute_todo (ipa_pass->function_transform_todo_flags_start);

  /* If a timevar is present, start it.  */
  if (pass->tv_id != TV_NONE)
    timevar_push (pass->tv_id);

  /* Do it!  */
  todo_after = ipa_pass->function_transform (node);

  /* Stop timevar.  */
  if (pass->tv_id != TV_NONE)
    timevar_pop (pass->tv_id);

  if (profile_report && cfun && (cfun->curr_properties & PROP_cfg))
    check_profile_consistency (pass->static_pass_number, 0, true);

  /* Run post-pass cleanup and verification.  */
  execute_todo (todo_after);
  verify_interpass_invariants ();
  if (profile_report && cfun && (cfun->curr_properties & PROP_cfg))
    check_profile_consistency (pass->static_pass_number, 1, true);

  do_per_function (execute_function_dump, NULL);
  pass_fini_dump_file (pass);

  current_pass = NULL;
}

/* For the current function, execute all ipa transforms. */

void
execute_all_ipa_transforms (void)
{
  struct cgraph_node *node;
  if (!cfun)
    return;
  node = cgraph_get_node (current_function_decl);

  if (node->ipa_transforms_to_apply.exists ())
    {
      unsigned int i;

      for (i = 0; i < node->ipa_transforms_to_apply.length (); i++)
	execute_one_ipa_transform_pass (node, node->ipa_transforms_to_apply[i]);
      node->ipa_transforms_to_apply.release ();
    }
}

/* Callback for do_per_function to apply all IPA transforms.  */

static void
apply_ipa_transforms (void *data)
{
  struct cgraph_node *node = cgraph_get_node (current_function_decl);
  if (!node->global.inlined_to && node->ipa_transforms_to_apply.exists ())
    {
      *(bool *)data = true;
      execute_all_ipa_transforms();
      rebuild_cgraph_edges ();
    }
}

/* Check if PASS is explicitly disabled or enabled and return
   the gate status.  FUNC is the function to be processed, and
   GATE_STATUS is the gate status determined by pass manager by
   default.  */

static bool
override_gate_status (struct opt_pass *pass, tree func, bool gate_status)
{
  bool explicitly_enabled = false;
  bool explicitly_disabled = false;

  explicitly_enabled
   = is_pass_explicitly_enabled_or_disabled (pass, func,
                                             enabled_pass_uid_range_tab);
  explicitly_disabled
   = is_pass_explicitly_enabled_or_disabled (pass, func,
                                             disabled_pass_uid_range_tab);

  gate_status = !explicitly_disabled && (gate_status || explicitly_enabled);

  return gate_status;
}


/* Execute PASS. */

bool
execute_one_pass (struct opt_pass *pass)
{
  unsigned int todo_after = 0;

  bool gate_status;

  /* IPA passes are executed on whole program, so cfun should be NULL.
     Other passes need function context set.  */
  if (pass->type == SIMPLE_IPA_PASS || pass->type == IPA_PASS)
    gcc_assert (!cfun && !current_function_decl);
  else
    gcc_assert (cfun && current_function_decl);

  current_pass = pass;

  /* Check whether gate check should be avoided.
     User controls the value of the gate through the parameter "gate_status". */
  gate_status = (pass->gate == NULL) ? true : pass->gate();
  gate_status = override_gate_status (pass, current_function_decl, gate_status);

  /* Override gate with plugin.  */
  invoke_plugin_callbacks (PLUGIN_OVERRIDE_GATE, &gate_status);

  if (!gate_status)
    {
      /* Run so passes selectively disabling themselves on a given function
	 are not miscounted.  */
      if (profile_report && cfun && (cfun->curr_properties & PROP_cfg))
	{
          check_profile_consistency (pass->static_pass_number, 0, false);
          check_profile_consistency (pass->static_pass_number, 1, false);
	}
      current_pass = NULL;
      return false;
    }

  /* Pass execution event trigger: useful to identify passes being
     executed.  */
  invoke_plugin_callbacks (PLUGIN_PASS_EXECUTION, pass);

  /* SIPLE IPA passes do not handle callgraphs with IPA transforms in it.
     Apply all trnasforms first.  */
  if (pass->type == SIMPLE_IPA_PASS)
    {
      bool applied = false;
      do_per_function (apply_ipa_transforms, (void *)&applied);
      if (applied)
        symtab_remove_unreachable_nodes (true, dump_file);
      /* Restore current_pass.  */
      current_pass = pass;
    }

  if (!quiet_flag && !cfun)
    fprintf (stderr, " <%s>", pass->name ? pass->name : "");

  /* Note that the folders should only create gimple expressions.
     This is a hack until the new folder is ready.  */
  in_gimple_form = (cfun && (cfun->curr_properties & PROP_trees)) != 0;

  pass_init_dump_file (pass);

  /* Run pre-pass verification.  */
  execute_todo (pass->todo_flags_start);

#ifdef ENABLE_CHECKING
  do_per_function (verify_curr_properties,
		   (void *)(size_t)pass->properties_required);
#endif

  /* If a timevar is present, start it.  */
  if (pass->tv_id != TV_NONE)
    timevar_push (pass->tv_id);

  /* Do it!  */
  if (pass->execute)
    {
      todo_after = pass->execute ();
      do_per_function (clear_last_verified, NULL);
    }

  /* Stop timevar.  */
  if (pass->tv_id != TV_NONE)
    timevar_pop (pass->tv_id);

  do_per_function (update_properties_after_pass, pass);

  if (profile_report && cfun && (cfun->curr_properties & PROP_cfg))
    check_profile_consistency (pass->static_pass_number, 0, true);

  /* Run post-pass cleanup and verification.  */
  execute_todo (todo_after | pass->todo_flags_finish);
  if (profile_report && cfun && (cfun->curr_properties & PROP_cfg))
    check_profile_consistency (pass->static_pass_number, 1, true);

  verify_interpass_invariants ();
  do_per_function (execute_function_dump, NULL);
  if (pass->type == IPA_PASS)
    {
      struct cgraph_node *node;
      FOR_EACH_FUNCTION_WITH_GIMPLE_BODY (node)
	node->ipa_transforms_to_apply.safe_push ((struct ipa_opt_pass_d *)pass);
    }

  if (!current_function_decl)
    cgraph_process_new_functions ();

  pass_fini_dump_file (pass);

  if (pass->type != SIMPLE_IPA_PASS && pass->type != IPA_PASS)
    gcc_assert (!(cfun->curr_properties & PROP_trees)
		|| pass->type != RTL_PASS);

  current_pass = NULL;

  return true;
}

void
execute_pass_list (struct opt_pass *pass)
{
  do
    {
      gcc_assert (pass->type == GIMPLE_PASS
		  || pass->type == RTL_PASS);
      if (execute_one_pass (pass) && pass->sub)
        execute_pass_list (pass->sub);
      pass = pass->next;
    }
  while (pass);
}

/* Same as execute_pass_list but assume that subpasses of IPA passes
   are local passes. If SET is not NULL, write out summaries of only
   those node in SET. */

static void
ipa_write_summaries_2 (struct opt_pass *pass, struct lto_out_decl_state *state)
{
  while (pass)
    {
      struct ipa_opt_pass_d *ipa_pass = (struct ipa_opt_pass_d *)pass;
      gcc_assert (!current_function_decl);
      gcc_assert (!cfun);
      gcc_assert (pass->type == SIMPLE_IPA_PASS || pass->type == IPA_PASS);
      if (pass->type == IPA_PASS
	  && ipa_pass->write_summary
	  && (!pass->gate || pass->gate ()))
	{
	  /* If a timevar is present, start it.  */
	  if (pass->tv_id)
	    timevar_push (pass->tv_id);

          pass_init_dump_file (pass);

	  ipa_pass->write_summary ();

          pass_fini_dump_file (pass);

	  /* If a timevar is present, start it.  */
	  if (pass->tv_id)
	    timevar_pop (pass->tv_id);
	}

      if (pass->sub && pass->sub->type != GIMPLE_PASS)
	ipa_write_summaries_2 (pass->sub, state);

      pass = pass->next;
    }
}

/* Helper function of ipa_write_summaries. Creates and destroys the
   decl state and calls ipa_write_summaries_2 for all passes that have
   summaries.  SET is the set of nodes to be written.  */

static void
ipa_write_summaries_1 (lto_symtab_encoder_t encoder)
{
  struct lto_out_decl_state *state = lto_new_out_decl_state ();
  state->symtab_node_encoder = encoder;

  lto_push_out_decl_state (state);

  gcc_assert (!flag_wpa);
  ipa_write_summaries_2 (all_regular_ipa_passes, state);
  ipa_write_summaries_2 (all_lto_gen_passes, state);

  gcc_assert (lto_get_out_decl_state () == state);
  lto_pop_out_decl_state ();
  lto_delete_out_decl_state (state);
}

/* Write out summaries for all the nodes in the callgraph.  */

void
ipa_write_summaries (void)
{
  lto_symtab_encoder_t encoder;
  int i, order_pos;
  struct varpool_node *vnode;
  struct cgraph_node **order;

  if (!flag_generate_lto || seen_error ())
    return;

  encoder = lto_symtab_encoder_new (false);

  /* Create the callgraph set in the same order used in
     cgraph_expand_all_functions.  This mostly facilitates debugging,
     since it causes the gimple file to be processed in the same order
     as the source code.  */
  order = XCNEWVEC (struct cgraph_node *, cgraph_n_nodes);
  order_pos = ipa_reverse_postorder (order);
  gcc_assert (order_pos == cgraph_n_nodes);

  for (i = order_pos - 1; i >= 0; i--)
    {
      struct cgraph_node *node = order[i];

      if (cgraph_function_with_gimple_body_p (node))
	{
	  /* When streaming out references to statements as part of some IPA
	     pass summary, the statements need to have uids assigned and the
	     following does that for all the IPA passes here. Naturally, this
	     ordering then matches the one IPA-passes get in their stmt_fixup
	     hooks.  */

	  push_cfun (DECL_STRUCT_FUNCTION (node->symbol.decl));
	  renumber_gimple_stmt_uids ();
	  pop_cfun ();
	}
      if (node->analyzed)
        lto_set_symtab_encoder_in_partition (encoder, (symtab_node)node);
    }

  FOR_EACH_DEFINED_VARIABLE (vnode)
    if ((!vnode->alias || vnode->alias_of))
      lto_set_symtab_encoder_in_partition (encoder, (symtab_node)vnode);

  ipa_write_summaries_1 (compute_ltrans_boundary (encoder));

  free (order);
}

/* Same as execute_pass_list but assume that subpasses of IPA passes
   are local passes. If SET is not NULL, write out optimization summaries of
   only those node in SET. */

static void
ipa_write_optimization_summaries_1 (struct opt_pass *pass, struct lto_out_decl_state *state)
{
  while (pass)
    {
      struct ipa_opt_pass_d *ipa_pass = (struct ipa_opt_pass_d *)pass;
      gcc_assert (!current_function_decl);
      gcc_assert (!cfun);
      gcc_assert (pass->type == SIMPLE_IPA_PASS || pass->type == IPA_PASS);
      if (pass->type == IPA_PASS
	  && ipa_pass->write_optimization_summary
	  && (!pass->gate || pass->gate ()))
	{
	  /* If a timevar is present, start it.  */
	  if (pass->tv_id)
	    timevar_push (pass->tv_id);

          pass_init_dump_file (pass);

	  ipa_pass->write_optimization_summary ();

          pass_fini_dump_file (pass);

	  /* If a timevar is present, start it.  */
	  if (pass->tv_id)
	    timevar_pop (pass->tv_id);
	}

      if (pass->sub && pass->sub->type != GIMPLE_PASS)
	ipa_write_optimization_summaries_1 (pass->sub, state);

      pass = pass->next;
    }
}

/* Write all the optimization summaries for the cgraph nodes in SET.  If SET is
   NULL, write out all summaries of all nodes. */

void
ipa_write_optimization_summaries (lto_symtab_encoder_t encoder)
{
  struct lto_out_decl_state *state = lto_new_out_decl_state ();
  lto_symtab_encoder_iterator lsei;
  state->symtab_node_encoder = encoder;

  lto_push_out_decl_state (state);
  for (lsei = lsei_start_function_in_partition (encoder);
       !lsei_end_p (lsei); lsei_next_function_in_partition (&lsei))
    {
      struct cgraph_node *node = lsei_cgraph_node (lsei);
      /* When streaming out references to statements as part of some IPA
	 pass summary, the statements need to have uids assigned.

	 For functions newly born at WPA stage we need to initialize
	 the uids here.  */
      if (node->analyzed
	  && gimple_has_body_p (node->symbol.decl))
	{
	  push_cfun (DECL_STRUCT_FUNCTION (node->symbol.decl));
	  renumber_gimple_stmt_uids ();
	  pop_cfun ();
	}
    }

  gcc_assert (flag_wpa);
  ipa_write_optimization_summaries_1 (all_regular_ipa_passes, state);
  ipa_write_optimization_summaries_1 (all_lto_gen_passes, state);

  gcc_assert (lto_get_out_decl_state () == state);
  lto_pop_out_decl_state ();
  lto_delete_out_decl_state (state);
}

/* Same as execute_pass_list but assume that subpasses of IPA passes
   are local passes.  */

static void
ipa_read_summaries_1 (struct opt_pass *pass)
{
  while (pass)
    {
      struct ipa_opt_pass_d *ipa_pass = (struct ipa_opt_pass_d *) pass;

      gcc_assert (!current_function_decl);
      gcc_assert (!cfun);
      gcc_assert (pass->type == SIMPLE_IPA_PASS || pass->type == IPA_PASS);

      if (pass->gate == NULL || pass->gate ())
	{
	  if (pass->type == IPA_PASS && ipa_pass->read_summary)
	    {
	      /* If a timevar is present, start it.  */
	      if (pass->tv_id)
		timevar_push (pass->tv_id);

	      pass_init_dump_file (pass);

	      ipa_pass->read_summary ();

	      pass_fini_dump_file (pass);

	      /* Stop timevar.  */
	      if (pass->tv_id)
		timevar_pop (pass->tv_id);
	    }

	  if (pass->sub && pass->sub->type != GIMPLE_PASS)
	    ipa_read_summaries_1 (pass->sub);
	}
      pass = pass->next;
    }
}


/* Read all the summaries for all_regular_ipa_passes and all_lto_gen_passes.  */

void
ipa_read_summaries (void)
{
  ipa_read_summaries_1 (all_regular_ipa_passes);
  ipa_read_summaries_1 (all_lto_gen_passes);
}

/* Same as execute_pass_list but assume that subpasses of IPA passes
   are local passes.  */

static void
ipa_read_optimization_summaries_1 (struct opt_pass *pass)
{
  while (pass)
    {
      struct ipa_opt_pass_d *ipa_pass = (struct ipa_opt_pass_d *) pass;

      gcc_assert (!current_function_decl);
      gcc_assert (!cfun);
      gcc_assert (pass->type == SIMPLE_IPA_PASS || pass->type == IPA_PASS);

      if (pass->gate == NULL || pass->gate ())
	{
	  if (pass->type == IPA_PASS && ipa_pass->read_optimization_summary)
	    {
	      /* If a timevar is present, start it.  */
	      if (pass->tv_id)
		timevar_push (pass->tv_id);

	      pass_init_dump_file (pass);

	      ipa_pass->read_optimization_summary ();

	      pass_fini_dump_file (pass);

	      /* Stop timevar.  */
	      if (pass->tv_id)
		timevar_pop (pass->tv_id);
	    }

	  if (pass->sub && pass->sub->type != GIMPLE_PASS)
	    ipa_read_optimization_summaries_1 (pass->sub);
	}
      pass = pass->next;
    }
}

/* Read all the summaries for all_regular_ipa_passes and all_lto_gen_passes.  */

void
ipa_read_optimization_summaries (void)
{
  ipa_read_optimization_summaries_1 (all_regular_ipa_passes);
  ipa_read_optimization_summaries_1 (all_lto_gen_passes);
}

/* Same as execute_pass_list but assume that subpasses of IPA passes
   are local passes.  */
void
execute_ipa_pass_list (struct opt_pass *pass)
{
  do
    {
      gcc_assert (!current_function_decl);
      gcc_assert (!cfun);
      gcc_assert (pass->type == SIMPLE_IPA_PASS || pass->type == IPA_PASS);
      if (execute_one_pass (pass) && pass->sub)
	{
	  if (pass->sub->type == GIMPLE_PASS)
	    {
	      invoke_plugin_callbacks (PLUGIN_EARLY_GIMPLE_PASSES_START, NULL);
	      do_per_function_toporder ((void (*)(void *))execute_pass_list,
					pass->sub);
	      invoke_plugin_callbacks (PLUGIN_EARLY_GIMPLE_PASSES_END, NULL);
	    }
	  else if (pass->sub->type == SIMPLE_IPA_PASS
		   || pass->sub->type == IPA_PASS)
	    execute_ipa_pass_list (pass->sub);
	  else
	    gcc_unreachable ();
	}
      gcc_assert (!current_function_decl);
      cgraph_process_new_functions ();
      pass = pass->next;
    }
  while (pass);
}

/* Execute stmt fixup hooks of all passes in PASS for NODE and STMTS.  */

static void
execute_ipa_stmt_fixups (struct opt_pass *pass,
			  struct cgraph_node *node, gimple *stmts)
{
  while (pass)
    {
      /* Execute all of the IPA_PASSes in the list.  */
      if (pass->type == IPA_PASS
	  && (!pass->gate || pass->gate ()))
	{
	  struct ipa_opt_pass_d *ipa_pass = (struct ipa_opt_pass_d *) pass;

	  if (ipa_pass->stmt_fixup)
	    {
	      pass_init_dump_file (pass);
	      /* If a timevar is present, start it.  */
	      if (pass->tv_id)
		timevar_push (pass->tv_id);

	      ipa_pass->stmt_fixup (node, stmts);

	      /* Stop timevar.  */
	      if (pass->tv_id)
		timevar_pop (pass->tv_id);
	      pass_fini_dump_file (pass);
	    }
	  if (pass->sub)
	    execute_ipa_stmt_fixups (pass->sub, node, stmts);
	}
      pass = pass->next;
    }
}

/* Execute stmt fixup hooks of all IPA passes for NODE and STMTS.  */

void
execute_all_ipa_stmt_fixups (struct cgraph_node *node, gimple *stmts)
{
  execute_ipa_stmt_fixups (all_regular_ipa_passes, node, stmts);
}


extern void debug_properties (unsigned int);
extern void dump_properties (FILE *, unsigned int);

DEBUG_FUNCTION void
dump_properties (FILE *dump, unsigned int props)
{
  fprintf (dump, "Properties:\n");
  if (props & PROP_gimple_any)
    fprintf (dump, "PROP_gimple_any\n");
  if (props & PROP_gimple_lcf)
    fprintf (dump, "PROP_gimple_lcf\n");
  if (props & PROP_gimple_leh)
    fprintf (dump, "PROP_gimple_leh\n");
  if (props & PROP_cfg)
    fprintf (dump, "PROP_cfg\n");
  if (props & PROP_ssa)
    fprintf (dump, "PROP_ssa\n");
  if (props & PROP_no_crit_edges)
    fprintf (dump, "PROP_no_crit_edges\n");
  if (props & PROP_rtl)
    fprintf (dump, "PROP_rtl\n");
  if (props & PROP_gimple_lomp)
    fprintf (dump, "PROP_gimple_lomp\n");
  if (props & PROP_gimple_lcx)
    fprintf (dump, "PROP_gimple_lcx\n");
  if (props & PROP_cfglayout)
    fprintf (dump, "PROP_cfglayout\n");
}

DEBUG_FUNCTION void
debug_properties (unsigned int props)
{
  dump_properties (stderr, props);
}

/* Called by local passes to see if function is called by already processed nodes.
   Because we process nodes in topological order, this means that function is
   in recursive cycle or we introduced new direct calls.  */
bool
function_called_by_processed_nodes_p (void)
{
  struct cgraph_edge *e;
  for (e = cgraph_get_node (current_function_decl)->callers;
       e;
       e = e->next_caller)
    {
      if (e->caller->symbol.decl == current_function_decl)
        continue;
      if (!cgraph_function_with_gimple_body_p (e->caller))
        continue;
      if (TREE_ASM_WRITTEN (e->caller->symbol.decl))
        continue;
      if (!e->caller->process && !e->caller->global.inlined_to)
      	break;
    }
  if (dump_file && e)
    {
      fprintf (dump_file, "Already processed call to:\n");
      dump_cgraph_node (dump_file, e->caller);
    }
  return e != NULL;
}

#include "gt-passes.h"
